// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <fmt/format.h>

#include "music_manager.hpp"
#include "music_tune.hpp"

#include "core/string.h"
#include "d_clisrv.h"
#include "doomtype.h"
#include "i_sound.h"
#include "i_time.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

using namespace srb2::music;

namespace
{

// How many tics music is allowed to drift away from game
// logic.
constexpr int kResyncTics = 2;

}; // namespace

void TuneManager::tick()
{
	if (S_MusicDisabled())
	{
		return;
	}

	Tune* tune = current_tune();

	srb2::String old_song = current_song_;
	current_song_ = tune && tune->playing() && !tune->suspend ? tune->song : srb2::String{};

	bool changed = current_song_ != old_song;

	if (stop_credit_)
	{
		if (changed)
		{
			// Only stop the music credit if the song actually
			// changed.
			S_StopMusicCredit();
			S_UnloadMusicCredit();
		}

		stop_credit_ = false;
	}

	if (!tune)
	{
		if (changed)
		{
			I_UnloadSong();
		}

		return;
	}

	if (changed)
	{
		if (load())
		{
			musicdef_t* def = find_musicdef();
			if (!cv_streamersafemusic.value || (def != nullptr && !def->contentidunsafe))
			{
				I_PlaySong(tune->loop);
			}

			I_FadeSongFromVolume(
				tune->use_level_volume ? level_volume_ : 100,
				0,
				tune->resume ? tune->resume_fade_in : tune->fade_in,
				nullptr
			);
			seek(tune);

			adjust_volume();
			I_SetSongSpeed(tune->speed());

			S_LoadMusicCredit();

			if (tune->credit && !tune->resume)
			{
				S_ShowMusicCredit();
			}

			tune->resume = true;
		}
		else
		{
			I_UnloadSong();
		}
	}
	else if (tune->needs_seek || (tune->sync && resync()))
	{
		seek(tune);
	}
	else if (tune->time_remaining() <= detail::msec_to_tics(tune->fade_out))
	{
		if (tune->can_fade_out)
		{
			I_FadeSong(0, tune->fade_out, nullptr);
			tune->can_fade_out = false;
		}

		if (!tune->keep_open)
		{
			tune->ending = true;
		}
	}

	if (level_volume_ != old_level_volume_ && tune->use_level_volume && tune->can_fade_out)
	{
		if (volume_fade_)
		{
			I_FadeSong(level_volume_, tune->resume_fade_in, nullptr);
		}
		else
		{
			I_SetInternalMusicVolume(level_volume_);
		}
		old_level_volume_ = level_volume_;
	}
}

void TuneManager::pause_unpause() const
{
	const Tune* tune = current_tune();

	if (tune)
	{
		if (tune->paused())
		{
			I_PauseSong();
		}
		else
		{
			musicdef_t* def = find_musicdef();
			if (!cv_streamersafemusic.value || (def != nullptr && !def->contentidunsafe))
			{
				I_ResumeSong();
			}
		}
	}
}

bool TuneManager::load() const
{
	srb2::String lumpstring = srb2::format("O_{}", current_song_);
	lumpnum_t lumpnum = W_CheckNumForLongName(lumpstring.c_str());

	if (lumpnum == LUMPERROR)
	{
		return false;
	}

	return I_LoadSong(static_cast<char*>(W_CacheLumpNum(lumpnum, PU_MUSIC)), W_LumpLength(lumpnum));
}

musicdef_t* TuneManager::find_musicdef() const
{
	uint8_t index = 0;
	return S_FindMusicDef(current_song_.c_str(), &index);
}

void TuneManager::adjust_volume() const
{
	UINT8 i;
	const musicdef_t* def = S_FindMusicDef(current_song_.c_str(), &i);

	if (!def)
	{
		return;
	}

	I_SetCurrentSongVolume(def->debug_volume != 0 ? def->debug_volume : def->volume);
}

bool TuneManager::resync()
{
	if (hu_stopped)
	{
		// The server is not sending updates. Don't resync
		// because we know game logic is not moving anyway.
		return false;
	}

	long d_local = I_GetTime() - time_local_;
	long d_sync = detail::tic_time() - time_sync_;

	if (std::abs(d_local - d_sync) >= kResyncTics)
	{
		time_sync_ = detail::tic_time();
		time_local_ = I_GetTime();

		return true;
	}

	return false;
}

void TuneManager::seek(Tune* tune)
{
	uint32_t end = I_GetSongLength();
	uint32_t loop = I_GetSongLoopPoint();

	uint32_t pos = (tune->seek + detail::tics_to_msec(tune->elapsed())) * tune->speed();

	if (pos > end && (end - loop) > 0u)
	{
		pos = loop + ((pos - end) % (end - loop));
	}

	I_SetSongPosition(pos);
	tune->needs_seek = false;
}
