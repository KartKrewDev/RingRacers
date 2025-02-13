// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/transient/sound-test.c
/// \brief Stereo Mode menu

#include "../../k_menu.h"
#include "../../music.h"
#include "../../s_sound.h"

static void M_SoundTestMainControl(INT32 choice)
{
	(void)choice;

	// Sound test exception
	if (soundtest.current == NULL || soundtest.current->numtracks == 0)
	{
		if (cv_soundtest.value != 0)
		{
			S_StopSounds();

			if (currentMenu->menuitems[itemOn].mvar1 == 0) // Stop
			{
				CV_SetValue(&cv_soundtest, 0);
			}
			else if (currentMenu->menuitems[itemOn].mvar1 == 1) // Play
			{
				S_StartSound(NULL, cv_soundtest.value);
			}
		}
		else if (currentMenu->menuitems[itemOn].mvar1 == 1) // Play
		{
			soundtest.playing = true;
			soundtest.autosequence = true;
			S_UpdateSoundTestDef(false, false, false);
		}

		return;
	}

	if (currentMenu->menuitems[itemOn].mvar1 == 1) // Play
	{
		if (Music_Paused(S_SoundTestTune(0)) == true)
		{
			S_SoundTestTogglePause();
		}
		else
		{
			S_SoundTestPlay();
		}
	}
	else if (soundtest.playing == true)
	{
		if (currentMenu->menuitems[itemOn].mvar1 == 2) // Pause
		{
			if (Music_Paused(S_SoundTestTune(0)) == false)
			{
				S_SoundTestTogglePause();
			}
		}
		else // Stop
		{
			S_SoundTestStop();
		}
	}
	else if (currentMenu->menuitems[itemOn].mvar1 == 0) // Stop while stopped?
	{
		soundtest.current = NULL;
		soundtest.currenttrack = 0;
		CV_SetValue(&cv_soundtest, 0);
	}
}

static void M_SoundTestNextPrev(INT32 choice)
{
	(void)choice;

	S_UpdateSoundTestDef((currentMenu->menuitems[itemOn].mvar1 < 0), true, false);
}

static void M_SoundTestSeq(INT32 choice)
{
	(void)choice;

	soundtest.autosequence ^= true;

	if (soundtest.playing && S_SoundTestCanSequenceFade())
	{
		// 1) You cannot cancel a fade once it has started
		// 2) However, if the fade wasn't heard, switching
		//    over just skips to the next song
		if (Music_DurationLeft("stereo_fade") <= Music_FadeOutDuration("stereo_fade") * TICRATE / 1000)
		{
			if (Music_Suspended("stereo_fade"))
			{
				S_UpdateSoundTestDef((currentMenu->menuitems[itemOn].mvar1 < 0), true, false);
			}
		}
		else
		{
			soundtest.tune ^= 1;
			Music_UnSuspend(S_SoundTestTune(0));
			Music_Suspend(S_SoundTestTune(1));
		}
	}
}

static void M_SoundTestShf(INT32 choice)
{
	(void)choice;

	if (soundtest.shuffle)
	{
		soundtest.shuffle = false;
		soundtest.sequence.shuffleinfo = 0;
	}
	else
	{
		S_SoundTestStop();

		soundtest.playing = true;
		soundtest.autosequence = true;
		soundtest.shuffle = true;
		S_UpdateSoundTestDef(false, false, false);
	}
}

consvar_t *M_GetSoundTestVolumeCvar(void)
{
	if (soundtest.current == NULL)
	{
		if (cv_soundtest.value == 0)
			return NULL;

		return &cv_soundvolume;
	}

	return &cv_digmusicvolume;
}

static void M_SoundTestVol(INT32 choice)
{
	consvar_t *voltoadjust = M_GetSoundTestVolumeCvar();

	if (!voltoadjust)
		return;

	M_ChangeCvarDirect(choice, voltoadjust);
}

static void M_SoundTestTrack(INT32 choice)
{
	const UINT8 numtracks = (soundtest.current != NULL) ? soundtest.current->numtracks : 0;

	if (numtracks == 1 // No cycling
		|| choice == -1) // Extra
	{
		return;
	}

	// Confirm is generally treated as Up.

	// Soundtest exception
	if (numtracks == 0)
	{
		S_StopSounds();
		CV_AddValue(&cv_soundtest, ((choice == 0) ? -1 : 1));

		return;
	}

	if (choice == 0) // Down
	{
		soundtest.currenttrack--;
		if (soundtest.currenttrack < 0)
			soundtest.currenttrack = numtracks-1;
	}
	else //if (choice == 1) -- Up
	{
		soundtest.currenttrack++;
		if (soundtest.currenttrack >= numtracks)
			soundtest.currenttrack = 0;
	}

	if (soundtest.playing)
	{
		S_SoundTestPlay();
	}
}

static boolean M_SoundTestInputs(INT32 ch)
{
	(void)ch;
	soundtest.justopened = false;
	return false;
}

static void M_SoundTestTick(void)
{
	soundtest.menutick++;
}

menuitem_t MISC_SoundTest[] =
{
	{IT_STRING | IT_CALL,   "Back",  "STER_IC0", NULL, {.routine = M_GoBack},               0,  stereospecial_back},
	{IT_SPACE, NULL, NULL, NULL, {NULL}, 6, 0},
	{IT_STRING | IT_CALL,   "Stop",  "STER_IC1", NULL, {.routine = M_SoundTestMainControl}, 0,  0},
	{IT_SPACE, NULL, NULL, NULL, {NULL},  6, 0},
	{IT_STRING | IT_CALL,   "Pause", "STER_IC2", NULL, {.routine = M_SoundTestMainControl}, 2,  stereospecial_pause},
	{IT_STRING | IT_CALL,   "Play",  "STER_IC3", NULL, {.routine = M_SoundTestMainControl}, 1,  stereospecial_play},
	{IT_SPACE, NULL, NULL, NULL, {NULL},  6, 0},
	{IT_STRING | IT_CALL,   "Prev",  "STER_IC4", NULL, {.routine = M_SoundTestNextPrev},   -1,  0},
	{IT_STRING | IT_CALL,   "Next",  "STER_IC5", NULL, {.routine = M_SoundTestNextPrev},    1,  0},
	{IT_SPACE, NULL, NULL, NULL, {NULL},  6, 0},
	{IT_STRING | IT_ARROWS, "Seq",   "STER_IC6", NULL, {.routine = M_SoundTestSeq},         0,  stereospecial_seq},
	{IT_STRING | IT_ARROWS, "Shf",  "STER_IC7", NULL, {.routine = M_SoundTestShf},        0,  stereospecial_shf},
	{IT_SPACE, NULL, NULL, NULL, {NULL}, 0, 244},
	{IT_STRING | IT_ARROWS, "Vol",   NULL,       NULL, {.routine = M_SoundTestVol},         0,  stereospecial_vol},
	{IT_SPACE, NULL, NULL, NULL, {NULL},  2, 0},
	{IT_STRING | IT_ARROWS, "Track", NULL,       NULL, {.routine = M_SoundTestTrack},       0,  stereospecial_track},
};

menu_t MISC_SoundTestDef = {
	sizeof (MISC_SoundTest)/sizeof (menuitem_t),
	&MainDef,
	0,
	MISC_SoundTest,
	19, 140,
	0, 0,
	MBF_UD_LR_FLIPPED|MBF_SOUNDLESS,
	".",
	98, 0,
	M_DrawSoundTest,
	M_DrawExtrasBack,
	M_SoundTestTick,
	NULL,
	NULL,
	M_SoundTestInputs,
};

void M_SoundTest(INT32 choice)
{
	(void)choice;

	// I reserve the right to add some sort of setup here -- toast 250323
	soundtest.menutick = 0;
	soundtest.justopened = true;

	MISC_SoundTestDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_SoundTestDef, false);
}
