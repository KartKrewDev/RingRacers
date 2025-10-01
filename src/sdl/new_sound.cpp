// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <cmath>
#include <memory>

#include <SDL.h>
#include <tracy/tracy/Tracy.hpp>

#include "../audio/chunk_load.hpp"
#include "../audio/gain.hpp"
#include "../audio/mixer.hpp"
#include "../audio/music_player.hpp"
#include "../audio/resample.hpp"
#include "../audio/sound_chunk.hpp"
#include "../audio/sound_effect_player.hpp"
#include "../cxxutil.hpp"
#include "../io/streams.hpp"

#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
#include "../m_avrecorder.hpp"
#endif

#include "../doomdef.h"
#include "../i_sound.h"
#include "../m_misc.h"
#include "../s_sound.h"
#include "../sounds.h"
#include "../w_wad.h"
#include "../z_zone.h"

using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

using srb2::audio::Gain;
using srb2::audio::Mixer;
using srb2::audio::MusicPlayer;
using srb2::audio::Resampler;
using srb2::audio::Sample;
using srb2::audio::SoundChunk;
using srb2::audio::SoundEffectPlayer;
using srb2::audio::Source;
using namespace srb2;
using namespace srb2::io;

namespace
{
class SdlAudioStream final
{
	SDL_AudioStream* stream_;
public:
	SdlAudioStream(const SDL_AudioFormat format, const Uint8 channels, const int src_rate, const SDL_AudioFormat dst_format, const Uint8 dst_channels, const int dst_rate) noexcept
	{
		stream_ = SDL_NewAudioStream(format, channels, src_rate, dst_format, dst_channels, dst_rate);
	}
	SdlAudioStream(const SdlAudioStream&) = delete;
	SdlAudioStream(SdlAudioStream&&) = default;
	SdlAudioStream& operator=(const SdlAudioStream&) = delete;
	SdlAudioStream& operator=(SdlAudioStream&&) = default;
	~SdlAudioStream()
	{
		SDL_FreeAudioStream(stream_);
	}

	void put(tcb::span<const std::byte> buf)
	{
		int result = SDL_AudioStreamPut(stream_, buf.data(), buf.size_bytes());
		if (result < 0)
		{
			char errbuf[512];
			SDL_GetErrorMsg(errbuf, sizeof(errbuf));
			throw std::runtime_error(errbuf);
		}
	}

	size_t available() const
	{
		int result = SDL_AudioStreamAvailable(stream_);
		if (result < 0)
		{
			char errbuf[512];
			SDL_GetErrorMsg(errbuf, sizeof(errbuf));
			throw std::runtime_error(errbuf);
		}
		return result;
	}

	size_t get(tcb::span<std::byte> out)
	{
		int result = SDL_AudioStreamGet(stream_, out.data(), out.size_bytes());
		if (result < 0)
		{
			char errbuf[512];
			SDL_GetErrorMsg(errbuf, sizeof(errbuf));
			throw std::runtime_error(errbuf);
		}
		return result;
	}

	void clear() noexcept
	{
		SDL_AudioStreamClear(stream_);
	}
};

class SdlVoiceStreamPlayer : public Source<2>
{
	SdlAudioStream stream_;
	float volume_ = 1.0f;
	float sep_ = 0.0f;
	bool terminal_ = true;

public:
	SdlVoiceStreamPlayer() : stream_(AUDIO_F32SYS, 1, 48000, AUDIO_F32SYS, 2, 44100) {}
	virtual ~SdlVoiceStreamPlayer() = default;

	virtual std::size_t generate(tcb::span<Sample<2>> buffer) override
	{
		size_t written = stream_.get(tcb::as_writable_bytes(buffer)) / sizeof(Sample<2>);

		for (size_t i = written; i < buffer.size(); i++)
		{
			buffer[i] = {0.f, 0.f};
		}

		// Apply gain de-popping if the last generation was terminal
		if (terminal_)
		{
			for (size_t i = 0; i < std::min<size_t>(16, written); i++)
			{
				buffer[i].amplitudes[0] *= (float)(i) / 16;
				buffer[i].amplitudes[1] *= (float)(i) / 16;
			}
			terminal_ = false;
		}

		if (written < buffer.size())
		{
			terminal_ = true;
		}

		for (size_t i = 0; i < written; i++)
		{
			float sep_pan = ((sep_ + 1.f) / 2.f) * (3.14159 / 2.f);

			float left_scale = std::cos(sep_pan);
			float right_scale = std::sin(sep_pan);
			buffer[i] = {std::clamp(buffer[i].amplitudes[0] * volume_ * left_scale, -1.f, 1.f), std::clamp(buffer[i].amplitudes[1] * volume_ * right_scale, -1.f, 1.f)};
		}

		return buffer.size();
	};

	SdlAudioStream& stream() noexcept { return stream_; }

	void set_properties(float volume, float sep) noexcept
	{
		volume_ = volume;
		sep_ = sep;
	}
};

} // namespace

// extern in i_sound.h
UINT8 sound_started = false;

static unique_ptr<Gain<2>> master_gain;
static shared_ptr<Mixer<2>> master;
static shared_ptr<Mixer<2>> mixer_sound_effects;
static shared_ptr<Mixer<2>> mixer_music;
static shared_ptr<Mixer<2>> mixer_voice;
static shared_ptr<MusicPlayer> music_player;
static shared_ptr<Resampler<2>> resample_music_player;
static shared_ptr<Gain<2>> gain_sound_effects;
static shared_ptr<Gain<2>> gain_music_player;
static shared_ptr<Gain<2>> gain_music_channel;
static shared_ptr<Gain<2>> gain_voice_channel;

static vector<shared_ptr<SoundEffectPlayer>> sound_effect_channels;
static vector<shared_ptr<SdlVoiceStreamPlayer>> player_voice_channels;

#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
static shared_ptr<srb2::media::AVRecorder> av_recorder;
#endif

static void (*music_fade_callback)();

static SDL_AudioDeviceID g_device_id;
static SDL_AudioDeviceID g_input_device_id;

void* I_GetSfx(sfxinfo_t* sfx)
{
	if (sfx->lumpnum == LUMPERROR)
		sfx->lumpnum = S_GetSfxLumpNum(sfx);
	sfx->length = W_LumpLength(sfx->lumpnum);

	std::byte* lump = static_cast<std::byte*>(W_CacheLumpNum(sfx->lumpnum, PU_SOUND));
	auto _ = srb2::finally([lump]() { Z_Free(lump); });

	tcb::span<std::byte> data_span(lump, sfx->length);
	std::optional<SoundChunk> chunk = srb2::audio::try_load_chunk(data_span);

	if (!chunk)
		return nullptr;

	SoundChunk* heap_chunk = new SoundChunk {std::move(*chunk)};

	return heap_chunk;
}

void I_FreeSfx(sfxinfo_t* sfx)
{
	if (sfx->data)
	{
		SoundChunk* chunk = static_cast<SoundChunk*>(sfx->data);
		auto _ = srb2::finally([chunk]() { delete chunk; });

		// Stop any channels playing this chunk
		for (auto& player : sound_effect_channels)
		{
			if (player->is_playing_chunk(chunk))
			{
				player->reset();
			}
		}
	}
	sfx->data = nullptr;
	sfx->lumpnum = LUMPERROR;
}

namespace
{

class SdlAudioLockHandle
{
public:
	SdlAudioLockHandle() { SDL_LockAudioDevice(g_device_id); }
	~SdlAudioLockHandle() { SDL_UnlockAudioDevice(g_device_id); }
};

#ifdef TRACY_ENABLE
static const char* kAudio = "Audio";
#endif

void audio_callback(void* userdata, Uint8* buffer, int len)
{
	tracy::SetThreadName("SDL Audio Thread");
	FrameMarkStart(kAudio);
	ZoneScoped;
	floatdenormalstate_t dtzstate = M_EnterFloatDenormalToZero();
	auto dtzrestore = srb2::finally([dtzstate] { M_ExitFloatDenormalToZero(dtzstate); });

	// The SDL Audio lock is implied to be held during callback.

	try
	{
		Sample<2>* float_buffer = reinterpret_cast<Sample<2>*>(buffer);
		size_t float_len = len / 8;

		for (size_t i = 0; i < float_len; i++)
		{
			float_buffer[i] = Sample<2> {0.f, 0.f};
		}

		if (!master_gain)
			return;

		master_gain->generate(tcb::span {float_buffer, float_len});

		for (size_t i = 0; i < float_len; i++)
		{
			float_buffer[i] = {
				std::clamp(float_buffer[i].amplitudes[0], -1.f, 1.f),
				std::clamp(float_buffer[i].amplitudes[1], -1.f, 1.f),
			};
		}
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
		if (av_recorder)
			av_recorder->push_audio_samples(tcb::span {float_buffer, float_len});
#endif
	}
	catch (...)
	{
	}

	FrameMarkEnd(kAudio);

	return;
}

void initialize_sound()
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		CONS_Alert(CONS_ERROR, "Error initializing SDL Audio: %s\n", SDL_GetError());
		return;
	}

	SDL_AudioSpec desired {};
	desired.format = AUDIO_F32SYS;
	desired.channels = 2;
	desired.samples = cv_soundmixingbuffersize.value;
	desired.freq = 44100;
	desired.callback = audio_callback;

	if ((g_device_id = SDL_OpenAudioDevice(NULL, SDL_FALSE, &desired, NULL, 0)) == 0)
	{
		CONS_Alert(CONS_ERROR, "Failed to open SDL Audio device: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return;
	}

	SDL_PauseAudioDevice(g_device_id, SDL_FALSE);

	{
		SdlAudioLockHandle _;

		master_gain = make_unique<Gain<2>>();
		master = make_shared<Mixer<2>>();
		master_gain->bind(master);
		mixer_sound_effects = make_shared<Mixer<2>>();
		mixer_music = make_shared<Mixer<2>>();
		mixer_voice = make_shared<Mixer<2>>();
		music_player = make_shared<MusicPlayer>();
		resample_music_player = make_shared<Resampler<2>>(music_player, 1.f);
		gain_sound_effects = make_shared<Gain<2>>();
		gain_music_player = make_shared<Gain<2>>();
		gain_music_channel = make_shared<Gain<2>>();
		gain_voice_channel = make_shared<Gain<2>>();
		gain_sound_effects->bind(mixer_sound_effects);
		gain_music_player->bind(resample_music_player);
		gain_music_channel->bind(mixer_music);
		gain_voice_channel->bind(mixer_voice);
		master->add_source(gain_sound_effects);
		master->add_source(gain_music_channel);
		master->add_source(gain_voice_channel);
		mixer_music->add_source(gain_music_player);
		sound_effect_channels.clear();
		for (size_t i = 0; i < static_cast<size_t>(cv_numChannels.value); i++)
		{
			shared_ptr<SoundEffectPlayer> player = make_shared<SoundEffectPlayer>();
			sound_effect_channels.push_back(player);
			mixer_sound_effects->add_source(player);
		}
		player_voice_channels.clear();
		for (size_t i = 0; i < MAXPLAYERS; i++)
		{
			shared_ptr<SdlVoiceStreamPlayer> player = make_shared<SdlVoiceStreamPlayer>();
			player_voice_channels.push_back(player);
			mixer_voice->add_source(player);
		}
	}

	sound_started = true;
}

} // namespace

void I_StartupSound(void)
{
	if (!sound_started)
		initialize_sound();
}

void I_ShutdownSound(void)
{
	if (g_device_id)
	{
		SDL_CloseAudioDevice(g_device_id);
		g_device_id = 0;
	}
	if (g_input_device_id)
	{
		SDL_CloseAudioDevice(g_input_device_id);
		g_input_device_id = 0;
	}

	master_gain = nullptr;
	master = nullptr;
	mixer_sound_effects = nullptr;
	mixer_music = nullptr;
	mixer_voice = nullptr;
	music_player = nullptr;
	resample_music_player = nullptr;
	gain_sound_effects = nullptr;
	gain_music_player = nullptr;
	gain_music_channel = nullptr;
	gain_voice_channel = nullptr;
	sound_effect_channels.clear();
	player_voice_channels.clear();

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	sound_started = false;
}

void I_UpdateSound(void)
{
	// The SDL audio lock is re-entrant, so it is safe to lock twice
	// for the "fade to stop music" callback later.
	SdlAudioLockHandle _;

	if (music_fade_callback && !music_player->fading())
	{
		auto old_callback = music_fade_callback;
		music_fade_callback = nullptr;
		(old_callback());
	}
	return;
}

//
//  SFX I/O
//

INT32 I_StartSound(sfxenum_t id, UINT8 vol, UINT8 sep, UINT8 pitch, UINT8 priority, INT32 channel)
{
	(void) pitch;
	(void) priority;

	SdlAudioLockHandle _;

	if (channel >= 0 && static_cast<size_t>(channel) >= sound_effect_channels.size())
		return -1;

	shared_ptr<SoundEffectPlayer> player_channel;
	if (channel < 0)
	{
		// find a free sfx channel
		for (size_t i = 0; i < sound_effect_channels.size(); i++)
		{
			if (sound_effect_channels[i]->finished())
			{
				player_channel = sound_effect_channels[i];
				channel = i;
				break;
			}
		}
	}
	else
	{
		player_channel = sound_effect_channels[channel];
	}

	if (!player_channel)
		return -1;

	SoundChunk* chunk = static_cast<SoundChunk*>(S_sfx[id].data);
	if (chunk == nullptr)
		return -1;

	float vol_float = static_cast<float>(vol) / 255.f;
	float sep_float = static_cast<float>(sep) / 127.f - 1.f;

	player_channel->start(chunk, vol_float, sep_float);

	return channel;
}

void I_StopSound(INT32 handle)
{
	SdlAudioLockHandle _;

	if (sound_effect_channels.empty())
		return;

	if (handle < 0)
		return;

	size_t index = handle;

	if (index >= sound_effect_channels.size())
		return;

	sound_effect_channels[index]->reset();
}

boolean I_SoundIsPlaying(INT32 handle)
{
	SdlAudioLockHandle _;

	// Handle is channel index
	if (sound_effect_channels.empty())
		return 0;

	if (handle < 0)
		return 0;

	size_t index = handle;

	if (index >= sound_effect_channels.size())
		return 0;

	return sound_effect_channels[index]->finished() ? 0 : 1;
}

void I_UpdateSoundParams(INT32 handle, UINT8 vol, UINT8 sep, UINT8 pitch)
{
	(void) pitch;

	SdlAudioLockHandle _;

	if (sound_effect_channels.empty())
		return;

	if (handle < 0)
		return;

	size_t index = handle;

	if (index >= sound_effect_channels.size())
		return;

	shared_ptr<SoundEffectPlayer>& channel = sound_effect_channels[index];
	if (!channel->finished())
	{
		float vol_float = static_cast<float>(vol) / 255.f;
		float sep_float = static_cast<float>(sep) / 127.f - 1.f;
		channel->update(vol_float, sep_float);
	}
}

void I_SetSfxVolume(int volume)
{
	SdlAudioLockHandle _;
	float vol = static_cast<float>(volume) / 100.f;

	if (gain_sound_effects)
	{
		gain_sound_effects->gain(std::clamp(vol * vol * vol, 0.f, 1.f));
	}
}

void I_SetVoiceVolume(int volume)
{
	SdlAudioLockHandle _;
	float vol = static_cast<float>(volume) / 100.f;

	if (gain_voice_channel)
	{
		gain_voice_channel->gain(std::clamp(vol * vol * vol, 0.f, 1.f));
	}
}

void I_SetMasterVolume(int volume)
{
	SdlAudioLockHandle _;
	float vol = static_cast<float>(volume) / 100.f;

	if (master_gain)
	{
		master_gain->gain(std::clamp(vol * vol * vol, 0.f, 1.f));
	}
}

/// ------------------------
//  MUSIC SYSTEM
/// ------------------------

void I_InitMusic(void)
{
	if (!sound_started)
		initialize_sound();

	SdlAudioLockHandle _;

	if (music_player != nullptr)
		*music_player = audio::MusicPlayer();
}

void I_ShutdownMusic(void)
{
	SdlAudioLockHandle _;

	if (music_player)
		*music_player = audio::MusicPlayer();
}

/// ------------------------
//  MUSIC PROPERTIES
/// ------------------------

const char* I_SongType(void)
{
	if (!music_player)
		return nullptr;

	SdlAudioLockHandle _;

	std::optional<audio::MusicType> music_type = music_player->music_type();

	if (music_type == std::nullopt)
	{
		return nullptr;
	}

	switch (*music_type)
	{
	case audio::MusicType::kOgg:
		return "OGG";
	case audio::MusicType::kMod:
		return "Mod";
	default:
		return nullptr;
	}
}

boolean I_SongPlaying(void)
{
	if (!music_player)
		return false;

	SdlAudioLockHandle _;

	return music_player->music_type().has_value();
}

boolean I_SongPaused(void)
{
	if (!music_player)
		return false;

	SdlAudioLockHandle _;

	return !music_player->playing();
}

/// ------------------------
//  MUSIC EFFECTS
/// ------------------------

boolean I_SetSongSpeed(float speed)
{
	if (resample_music_player)
	{
		resample_music_player->ratio(speed);
		return true;
	}

	return false;
}

/// ------------------------
//  MUSIC SEEKING
/// ------------------------

UINT32 I_GetSongLength(void)
{
	if (!music_player)
		return 0;

	SdlAudioLockHandle _;

	std::optional<float> duration = music_player->duration_seconds();

	if (!duration)
		return 0;

	return static_cast<UINT32>(std::round(*duration * 1000.f));
}

boolean I_SetSongLoopPoint(UINT32 looppoint)
{
	if (!music_player)
		return 0;

	SdlAudioLockHandle _;

	if (music_player->music_type() == audio::MusicType::kOgg)
	{
		music_player->loop_point_seconds(looppoint / 1000.f);
		return true;
	}

	return false;
}

UINT32 I_GetSongLoopPoint(void)
{
	if (!music_player)
		return 0;

	SdlAudioLockHandle _;

	std::optional<float> loop_point_seconds = music_player->loop_point_seconds();

	if (!loop_point_seconds)
		return 0;

	return static_cast<UINT32>(std::round(*loop_point_seconds * 1000.f));
}

boolean I_SetSongPosition(UINT32 position)
{
	if (!music_player)
		return false;

	SdlAudioLockHandle _;

	music_player->seek(position / 1000.f);
	return true;
}

UINT32 I_GetSongPosition(void)
{
	if (!music_player)
		return 0;

	SdlAudioLockHandle _;

	std::optional<float> position_seconds = music_player->position_seconds();

	if (!position_seconds)
		return 0;

	return static_cast<UINT32>(std::round(*position_seconds * 1000.f));
}

void I_UpdateSongLagThreshold(void)
{
}

void I_UpdateSongLagConditions(void)
{
}

/// ------------------------
//  MUSIC PLAYBACK
/// ------------------------

namespace
{
void print_walk_ex_stack(const std::exception& ex)
{
	CONS_Alert(CONS_WARNING, "  Caused by: %s\n", ex.what());
	try
	{
		std::rethrow_if_nested(ex);
	}
	catch (const std::exception& ex)
	{
		print_walk_ex_stack(ex);
	}
}

void print_ex(const std::exception& ex)
{
	CONS_Alert(CONS_WARNING, "Exception loading music: %s\n", ex.what());
	try
	{
		std::rethrow_if_nested(ex);
	}
	catch (const std::exception& ex)
	{
		print_walk_ex_stack(ex);
	}
}
} // namespace

boolean I_LoadSong(char* data, size_t len)
{
	if (!music_player)
		return false;

	tcb::span<std::byte> data_span(reinterpret_cast<std::byte*>(data), len);
	audio::MusicPlayer new_player;
	try
	{
		new_player = audio::MusicPlayer {data_span};
	}
	catch (const std::exception& ex)
	{
		print_ex(ex);
		return false;
	}

	if (music_fade_callback && music_player->fading())
	{
		auto old_callback = music_fade_callback;
		music_fade_callback = nullptr;
		(old_callback)();
	}

	SdlAudioLockHandle _;

	try
	{
		*music_player = std::move(new_player);
	}
	catch (const std::exception& ex)
	{
		print_ex(ex);
		return false;
	}

	if (gain_music_player)
	{
		// Reset song volume to 1.0 for newly loaded songs.
		gain_music_player->gain(1.0);
	}

	return true;
}

void I_UnloadSong(void)
{
	if (!music_player)
		return;

	if (music_fade_callback && music_player->fading())
	{
		auto old_callback = music_fade_callback;
		music_fade_callback = nullptr;
		(old_callback)();
	}

	SdlAudioLockHandle _;

	*music_player = audio::MusicPlayer();
}

boolean I_PlaySong(boolean looping)
{
	if (!music_player)
		return false;

	SdlAudioLockHandle _;

	music_player->play(looping);

	return true;
}

void I_StopSong(void)
{
	if (!music_player)
		return;

	SdlAudioLockHandle _;

	music_player->stop();
}

void I_PauseSong(void)
{
	if (!music_player)
		return;

	SdlAudioLockHandle _;

	music_player->pause();
}

void I_ResumeSong(void)
{
	if (!music_player)
		return;

	SdlAudioLockHandle _;

	music_player->unpause();
}

void I_SetMusicVolume(int volume)
{
	float vol = static_cast<float>(volume) / 100.f;

	if (gain_music_channel)
	{
		// Music channel volume is interpreted as logarithmic rather than linear.
		// We approximate by cubing the gain level so vol 50 roughly sounds half as loud.
		gain_music_channel->gain(std::clamp(vol * vol * vol, 0.f, 1.f));
	}
}

void I_SetCurrentSongVolume(int volume)
{
	float vol = static_cast<float>(volume) / 100.f;

	if (gain_music_player)
	{
		// However, different from music channel volume, musicdef volumes are explicitly linear.
		gain_music_player->gain(std::max(vol, 0.f));
	}
}

boolean I_SetSongTrack(int track)
{
	(void) track;
	return false;
}

/// ------------------------
//  MUSIC FADING
/// ------------------------

void I_SetInternalMusicVolume(UINT8 volume)
{
	if (!music_player)
		return;

	SdlAudioLockHandle _;

	float gain = volume / 100.f;
	music_player->internal_gain(gain);
}

void I_StopFadingSong(void)
{
	if (!music_player)
		return;

	SdlAudioLockHandle _;

	music_player->stop_fade();
}

boolean I_FadeSongFromVolume(UINT8 target_volume, UINT8 source_volume, UINT32 ms, void (*callback)(void))
{
	if (!music_player)
		return false;

	SdlAudioLockHandle _;

	float source_gain = source_volume / 100.f;
	float target_gain = target_volume / 100.f;
	float seconds = ms / 1000.f;

	music_player->fade_from_to(source_gain, target_gain, seconds);

	if (music_fade_callback)
		music_fade_callback();
	music_fade_callback = callback;

	return true;
}

boolean I_FadeSong(UINT8 target_volume, UINT32 ms, void (*callback)(void))
{
	if (!music_player)
		return false;

	SdlAudioLockHandle _;

	float target_gain = target_volume / 100.f;
	float seconds = ms / 1000.f;

	music_player->fade_to(target_gain, seconds);

	if (music_fade_callback)
		music_fade_callback();
	music_fade_callback = callback;

	return true;
}

static void stop_song_cb(void)
{
	if (!music_player)
		return;

	SdlAudioLockHandle _;

	music_player->stop();
}

boolean I_FadeOutStopSong(UINT32 ms)
{
	return I_FadeSong(0.f, ms, stop_song_cb);
}

boolean I_FadeInPlaySong(UINT32 ms, boolean looping)
{
	if (I_PlaySong(looping))
		return I_FadeSongFromVolume(100, 0, ms, nullptr);
	else
		return false;
}

void I_UpdateAudioRecorder(void)
{
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
	// must be locked since av_recorder is used by audio_callback
	SdlAudioLockHandle _;

	av_recorder = g_av_recorder;
#endif
}

boolean I_SoundInputIsEnabled(void)
{
	return g_input_device_id != 0;
}

boolean I_SoundInputSetEnabled(boolean enabled)
{
	if (g_input_device_id == 0 && enabled)
	{
		if (!sound_started || SDL_GetNumAudioDevices(true) == 0)
		{
			return false;
		}

		SDL_AudioSpec input_desired {};
		input_desired.format = AUDIO_F32SYS;
		input_desired.channels = 1;
		input_desired.samples = 1024;
		input_desired.freq = 48000;
		SDL_AudioSpec input_obtained {};
		g_input_device_id = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &input_desired, &input_obtained, 0);
		if (!g_input_device_id)
		{
			CONS_Alert(CONS_WARNING, "Failed to open input audio device: %s\n", SDL_GetError());
			return false;
		}
		if (input_obtained.freq != 48000 || input_obtained.format != AUDIO_F32SYS || input_obtained.channels != 1)
		{
			CONS_Alert(CONS_WARNING, "Input audio device has unexpected unusable format: %s\n", SDL_GetError());
			return false;
		}
		SDL_PauseAudioDevice(g_input_device_id, SDL_FALSE);
	}
	else if (g_input_device_id != 0 && !enabled)
	{
		SDL_PauseAudioDevice(g_input_device_id, SDL_TRUE);
		SDL_ClearQueuedAudio(g_input_device_id);
		SDL_CloseAudioDevice(g_input_device_id);
		g_input_device_id = 0;
	}

	return enabled;
}

UINT32 I_SoundInputDequeueSamples(void *data, UINT32 len)
{
	if (!g_input_device_id)
	{
		return 0;
	}

	UINT32 ret = SDL_DequeueAudio(g_input_device_id, data, len);
	return ret;
}

UINT32 I_SoundInputRemainingSamples(void)
{
	if (!g_input_device_id)
	{
		return 0;
	}
	UINT32 avail = SDL_GetQueuedAudioSize(g_input_device_id);
	return avail / sizeof(float);
}

void I_QueueVoiceFrameFromPlayer(INT32 playernum, void *data, UINT32 len, boolean terminal)
{
	if (!sound_started)
	{
		return;
	}

	SdlAudioLockHandle _;
	SdlVoiceStreamPlayer* player = player_voice_channels.at(playernum).get();
	player->stream().put(tcb::span((std::byte*)data, len));
}

void I_SetPlayerVoiceProperties(INT32 playernum, float volume, float sep)
{
	if (!sound_started)
	{
		return;
	}

	SdlAudioLockHandle _;
	SdlVoiceStreamPlayer* player = player_voice_channels.at(playernum).get();
	player->set_properties(volume * volume * volume, sep);
}

void I_ResetVoiceQueue(INT32 playernum)
{
	if (!sound_started)
	{
		return;
	}

	SdlAudioLockHandle _;
	SdlVoiceStreamPlayer* player = player_voice_channels.at(playernum).get();
	player->stream().clear();
}
