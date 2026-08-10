// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../i_sound.h"

uint8_t sound_started = 0;

void *I_GetSfx(sfxinfo_t *sfx)
{
        (void)sfx;
        return NULL;
}

void I_FreeSfx(sfxinfo_t *sfx)
{
        (void)sfx;
}

size_t I_GetSoundMemUsage(void)
{
        return 0;
}

void I_StartupSound(void){}

void I_ShutdownSound(void){}

//
//  SFX I/O
//

int32_t I_StartSound(sfxenum_t id, int32_t vol, int32_t sep, int32_t pitch, int32_t priority, int32_t channel)
{
        (void)id;
        (void)vol;
        (void)sep;
        (void)pitch;
        (void)priority;
        (void)channel;
        return -1;
}

void I_StopSound(int32_t handle)
{
        (void)handle;
}

int32_t I_SoundIsPlaying(int32_t handle)
{
        (void)handle;
        return false;
}

void I_UpdateSoundParams(int32_t handle, int32_t vol, int32_t sep, int32_t pitch)
{
        (void)handle;
        (void)vol;
        (void)sep;
        (void)pitch;
}

void I_SetSfxVolume(int32_t volume)
{
        (void)volume;
}

/// ------------------------
//  MUSIC SYSTEM
/// ------------------------

uint8_t music_started = 0;
uint8_t digmusic_started = 0;

void I_InitMusic(void){}

void I_ShutdownMusic(void){}

/// ------------------------
//  MUSIC PROPERTIES
/// ------------------------

musictype_t I_SongType(void)
{
	return MU_NONE;
}

dboolean I_SongPlaying(void)
{
	return false;
}

dboolean I_SongPaused(void)
{
	return false;
}

/// ------------------------
//  MUSIC EFFECTS
/// ------------------------

dboolean I_SetSongSpeed(float speed)
{
        (void)speed;
        return false;
}

/// ------------------------
//  MUSIC SEEKING
/// ------------------------

uint32_t I_GetSongLength(void)
{
        return 0;
}

dboolean I_SetSongLoopPoint(uint32_t looppoint)
{
        (void)looppoint;
        return false;
}

uint32_t I_GetSongLoopPoint(void)
{
	return 0;
}

dboolean I_SetSongPosition(uint32_t position)
{
        (void)position;
        return false;
}

uint32_t I_GetSongPosition(void)
{
        return 0;
}

/// ------------------------
//  MUSIC PLAYBACK
/// ------------------------

uint8_t midimusic_started = 0;

dboolean I_LoadSong(char *data, size_t len)
{
        (void)data;
        (void)len;
        return -1;
}

void I_UnloadSong()
{

}

dboolean I_PlaySong(dboolean looping)
{
        (void)handle;
        (void)looping;
        return false;
}

void I_StopSong(void)
{
        (void)handle;
}

void I_PauseSong(void)
{
        (void)handle;
}

void I_ResumeSong(void)
{
        (void)handle;
}

void I_SetMusicVolume(int32_t volume)
{
        (void)volume;
}

/// ------------------------
//  MUSIC FADING
/// ------------------------

void I_SetInternalMusicVolume(uint8_t volume)
{
	(void)volume;
}

void I_StopFadingSong(void)
{
}

dboolean I_FadeSongFromVolume(uint8_t target_volume, uint8_t source_volume, uint32_t ms, void (*callback)(void));
{
	(void)target_volume;
	(void)source_volume;
	(void)ms;
        return false;
}

dboolean I_FadeSong(uint8_t target_volume, uint32_t ms, void (*callback)(void));
{
	(void)target_volume;
	(void)ms;
	return false;
}

dboolean I_FadeOutStopSong(uint32_t ms)
{
        (void)ms;
        return false;
}

dboolean I_FadeInPlaySong(uint32_t ms, dboolean looping)
{
        (void)ms;
        (void)looping;
        return false;
}
