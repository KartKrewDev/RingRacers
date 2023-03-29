/// \file  menus/transient/sound-test.c
/// \brief Stereo Mode menu

#include "../../k_menu.h"
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
			//soundtest.sequence = true;
			S_UpdateSoundTestDef(false, false, false);
		}

		return;
	}

	if (currentMenu->menuitems[itemOn].mvar1 == 1) // Play
	{
		if (soundtest.paused == true)
		{
			S_SoundTestTogglePause();
		}
		else if (soundtest.playing == false)
		{
			S_SoundTestPlay();
		}
	}
	else if (soundtest.playing == true)
	{
		if (currentMenu->menuitems[itemOn].mvar1 == 2) // Pause
		{
			if (soundtest.paused == false)
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
	}
}

static void M_SoundTestNextPrev(INT32 choice)
{
	(void)choice;

	S_UpdateSoundTestDef((currentMenu->menuitems[itemOn].mvar1 < 0), true, false);
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

menuitem_t MISC_SoundTest[] =
{
	{IT_STRING | IT_CALL,   "Back",  NULL, NULL, {.routine = M_GoBack},               0,  0},
	{IT_STRING | IT_CALL,   "Stop",  NULL, NULL, {.routine = M_SoundTestMainControl}, 0,  0},
	{IT_STRING | IT_CALL,   "Pause", NULL, NULL, {.routine = M_SoundTestMainControl}, 2,  0},
	{IT_STRING | IT_CALL,   "Play",  NULL, NULL, {.routine = M_SoundTestMainControl}, 1,  0},
	{IT_STRING | IT_ARROWS, "Track", NULL, NULL, {.routine = M_SoundTestTrack},       0,  0},
	{IT_STRING | IT_CALL,   "Prev",  NULL, NULL, {.routine = M_SoundTestNextPrev},   -1,  0},
	{IT_STRING | IT_CALL,   "Next",  NULL, NULL, {.routine = M_SoundTestNextPrev},    1,  0},
};

menu_t MISC_SoundTestDef = {
	sizeof (MISC_SoundTest)/sizeof (menuitem_t),
	&MainDef,
	0,
	MISC_SoundTest,
	42, BASEVIDHEIGHT/2,
	0, 0,
	MBF_UD_LR_FLIPPED|MBF_SOUNDLESS,
	".",
	98, 0,
	M_DrawSoundTest,
	NULL,
	NULL,
	NULL,
	NULL,
};

void M_SoundTest(INT32 choice)
{
	(void)choice;

	// I reserve the right to add some sort of setup here -- toast 250323

	MISC_SoundTestDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_SoundTestDef, false);
}
