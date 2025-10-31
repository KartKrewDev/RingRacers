// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "music_manager.hpp"
#include "music_tune.hpp"

#include "doomtype.h"
#include "music.h"

using namespace srb2::music;

namespace
{

TuneManager g_tunes;

}; // namespace

void Music_Init(void)
{
	// Many tunes below now have their default songs set in Music_TuneReset. Check there first for changing those.
	{
		Tune& tune = g_tunes.insert("level");

		tune.priority = 1;
		tune.fade_out = 1500;
		tune.fade_out_inclusive = false;
		tune.resume_fade_in = 750;
		tune.use_level_volume = true;
		tune.sync = true;
		tune.credit = true;
		tune.vapes = true;
		tune.nightcoreable = true;
	}

	{
		Tune& tune = g_tunes.insert("level_nosync", g_tunes.find("level"));

		tune.sync = false;
	}

	{
		Tune& tune = g_tunes.insert("position");

		tune.priority = 10;
		tune.fade_out = 3500;
	}

	{
		Tune& tune = g_tunes.insert("battle_overtime", g_tunes.find("level"));

		tune.song = ""; // Music_TuneReset
		tune.priority = 11;
	}

	{
		Tune& tune = g_tunes.insert("battle_overtime_stress", g_tunes.find("battle_overtime"));

		tune.song = ""; // Music_TuneReset
		tune.priority = 10;
	}

	{
		Tune& tune = g_tunes.insert("grow");

		tune.song = ""; // Music_TuneReset
		tune.priority = 20;
		tune.resume_fade_in = 200;
		tune.use_level_volume = true;
	}

	{
		Tune& tune = g_tunes.insert("invinc");

		tune.song = ""; // Music_TuneReset
		tune.priority = 21;
		tune.use_level_volume = true;
	}

	{
		Tune& tune = g_tunes.insert("finish_silence");

		tune.song = ""; // Music_TuneReset
		tune.priority = 30;
	}

	{
		Tune& tune = g_tunes.insert("finish");

		tune.priority = 30;
		tune.loop = false;
	}

	{
		Tune& tune = g_tunes.insert("comeon");

		tune.song = ""; // Music_TuneReset
		tune.priority = 35;
		tune.loop = false;
	}

	{
		Tune& tune = g_tunes.insert("intermission");

		tune.song = "racent";
		tune.priority = 40;
		tune.vapes = true;
	}

	{
		Tune& tune = g_tunes.insert("vote");

		tune.song = ""; // Music_TuneReset
		tune.priority = 50;
		tune.credit = true;
	}

	{
		Tune& tune = g_tunes.insert("vote_suspense");

		tune.song = ""; // Music_TuneReset
		tune.priority = 51;
	}

	{
		Tune& tune = g_tunes.insert("vote_end");

		tune.song = ""; // Music_TuneReset
		tune.priority = 52;
		tune.loop = false;
	}

	{
		Tune& tune = g_tunes.insert("wait");

		tune.song = ""; // Music_TuneReset
		tune.priority = 60;
	}

	{
		Tune& tune = g_tunes.insert("title");

		tune.song = ""; // Music_TuneReset
		tune.priority = 100;
		tune.resist = true;
	}

	{
		Tune& tune = g_tunes.insert("menu");

		tune.priority = 100;
		tune.credit = true;
	}

	{
		Tune& tune = g_tunes.insert("menu_nocred", g_tunes.find("menu"));

		tune.credit = false;
	}

	{
		Tune& tune = g_tunes.insert("credits_silence");

		tune.song = ""; // Music_TuneReset
		tune.priority = 100;
	}

	{
		Tune& tune = g_tunes.insert("credits");

		tune.priority = 101;
		tune.song = ""; // F_NewCreditsMusic
		tune.credit = true;
	}

	{
		Tune& tune = g_tunes.insert("shore");

		tune.priority = 101;
		tune.loop = false;
	}

	{
		Tune& tune = g_tunes.insert("stereo");

		tune.priority = 1000;
		tune.resist = true;
		tune.keep_open = true;
		tune.credit = true;
	}

	{
		Tune& tune = g_tunes.insert("stereo_fade", g_tunes.find("stereo"));

		tune.fade_out = 5000;
		tune.fade_out_inclusive = false;
	}

	{
		Tune& tune = g_tunes.insert("challenge_altmusic");

		tune.priority = 100;
		tune.resist = true;
		tune.credit = true;
	}

	{
		Tune& tune = g_tunes.insert("lawyer");

		tune.song = ""; // Music_TuneReset
		tune.priority = 35;
		tune.loop = false;
	}
	
	Music_TuneReset();
}



void Music_Tick(void)
{
	g_tunes.tick();
}

void Music_Flip(void)
{
	g_tunes.flip();
}

void Music_AddTune(const char* id, int priority, int tuneflags)
{
	Tune& tune = g_tunes.insert(id);

	tune.song = "";
	tune.priority = priority;

	tune.loop = (tuneflags & TN_LOOPING);
	tune.fade_out_inclusive = (tuneflags & TN_INCLUSIVEFADE);
	tune.use_level_volume = (tuneflags & TN_USEMAPVOLUME);
	tune.sync = (tuneflags & TN_SYNCMUSIC);
	tune.credit = (tuneflags & TN_MUSICCRED);
	tune.vapes = (tuneflags & TN_VAPES);
	tune.nightcoreable = (tuneflags & TN_NIGHTCOREABLE);
}

void Music_Play(const char* id)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		tune->play();
		g_tunes.tick(); // play this immediately
	}
}

void Music_SetFadeOut(const char* id, int fade_out)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		tune->fade_out = fade_out;

		if (tune->time_remaining() <= detail::msec_to_tics(tune->fade_out))
		{
			// If this action would cause a fade out, start
			// fading immediately.
			g_tunes.tick();
		}
	}
}

void Music_SetFadeIn(const char* id, int fade_in, boolean resume)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		if (resume)
		{
			tune->resume_fade_in = fade_in;
		}
		else
		{
			tune->fade_in = fade_in;
		}

		if (tune->elapsed() <= detail::msec_to_tics(fade_in))
		{
			// If this action would cause a fade in, start
			// fading immediately.
			g_tunes.tick();
		}
	}
}

void Music_DelayEnd(const char* id, tic_t duration)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		tune->delay_end(duration);

		if (tune->time_remaining() <= detail::msec_to_tics(tune->fade_out))
		{
			// If this action would cause a fade out, start
			// fading immediately.
			g_tunes.tick();
		}
	}
}

void Music_Seek(const char* id, UINT32 set)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		tune->seek = set;
		tune->needs_seek = true;
	}
}

void Music_Stop(const char* id)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		g_tunes.stop(*tune);
		g_tunes.tick();
	}
}

void Music_Pause(const char* id)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		tune->pause();
		g_tunes.pause_unpause();
	}
}

void Music_UnPause(const char* id)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		tune->unpause();
		g_tunes.pause_unpause();
	}
}

void Music_Suspend(const char* id)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		tune->suspend = true;
		g_tunes.tick();
	}
}

void Music_UnSuspend(const char* id)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		tune->suspend = false;
		g_tunes.tick();
	}
}

void Music_PauseAll(void)
{
	g_tunes.for_each([](Tune& tune) { tune.pause(); });
	g_tunes.pause_unpause();
}

void Music_UnPauseAll(void)
{
	g_tunes.for_each([](Tune& tune) { tune.unpause(); });
	g_tunes.pause_unpause();
}

void Music_StopAll(void)
{
	g_tunes.stop_all();
	g_tunes.tick();
}

void Music_Remap(const char* id, const char* song)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		tune->song = song;
	}
}

boolean Music_TuneExists(const char* id)
{
	const Tune* tune = g_tunes.find(id);

	if (tune)
	{
		return true;
	}
	return false;
}

boolean Music_Playing(const char* id)
{
	const Tune* tune = g_tunes.find(id);
	return tune && tune->playing();
}

boolean Music_Paused(const char* id)
{
	const Tune* tune = g_tunes.find(id);
	return tune && tune->paused();
}

boolean Music_Suspended(const char* id)
{
	const Tune* tune = g_tunes.find(id);
	return tune && tune->suspend;
}

tic_t Music_Elapsed(const char* id)
{
	const Tune* tune = g_tunes.find(id);
	return tune ? tune->elapsed() : 0u;
}

tic_t Music_DurationLeft(const char* id)
{
	const Tune* tune = g_tunes.find(id);
	return tune ? tune->time_remaining() : 0u;
}

tic_t Music_TotalDuration(const char* id)
{
	const Tune* tune = g_tunes.find(id);
	return tune ? tune->duration() : 0u;
}

unsigned int Music_FadeOutDuration(const char* id)
{
	const Tune* tune = g_tunes.find(id);
	return tune ? tune->fade_out : 0;
}

void Music_Loop(const char* id, boolean loop)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		// FIXME: this has no effect if the song is already
		// playing
		tune->loop = loop;
	}
}

boolean Music_CanLoop(const char* id)
{
	const Tune* tune = g_tunes.find(id);
	return tune && tune->loop;
}

boolean Music_CanEnd(const char* id)
{
	const Tune* tune = g_tunes.find(id);
	return tune && tune->can_end();
}

const char* Music_Song(const char* id)
{
	const Tune* tune = g_tunes.find(id);
	return tune ? tune->song.c_str() : "";
}

const char* Music_CurrentSong(void)
{
	return g_tunes.current_song().c_str();
}

const char* Music_CurrentId(void)
{
	return g_tunes.current_id();
}

void Music_BatchExempt(const char* id)
{
	Tune* tune = g_tunes.find(id);

	if (tune)
	{
		tune->resist_once = true;
	}
}

void Music_LevelVolume(int volume)
{
	g_tunes.level_volume(volume, false);
}

void Music_ResetLevelVolume(void)
{
	g_tunes.level_volume(100, true);
}

void Music_TuneReset(void)
{
	Music_Remap("battle_overtime", "shwdwn");
	Music_Remap("battle_overtime_stress", "shwdn2");
	Music_Remap("grow", "kgrow");
	Music_Remap("invinc", "kinvnc");
	Music_Remap("finish_silence", "");
	Music_Remap("comeon", "chalng");
	Music_Remap("vote", "vote");
	Music_Remap("vote_suspense", "voteea");
	Music_Remap("vote_end", "voteeb");
	Music_Remap("wait", "WAIT2J");
	Music_Remap("title", "_title");
	Music_Remap("credits_silence", "");
	Music_Remap("lawyer", "lawyer");
}
