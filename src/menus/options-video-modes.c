// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-video-modes.c
/// \brief Video modes (resolutions)

#include "../k_menu.h"
#include "../i_video.h"
#include "../s_sound.h"

menuitem_t OPTIONS_VideoModes[] = {

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Select a resolution. Higher = sharper game, lower = higher FPS.",
		NULL, {.routine = M_HandleVideoModes}, 0, 0},     // dummy menuitem for the control func

};

static void M_VideoModeMenu(void);

menu_t OPTIONS_VideoModesDef = {
	sizeof (OPTIONS_VideoModes) / sizeof (menuitem_t),
	&OPTIONS_VideoDef,
	0,
	OPTIONS_VideoModes,
	48, 80,
	SKINCOLOR_PLAGUE, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawVideoModes,
	M_DrawOptionsCogs,
	M_OptionsTick,
	M_VideoModeMenu,
	NULL,
	NULL,
};

// setup video mode menu
static void M_VideoModeMenu(void)
{
	INT32 i, j, vdup, nummodes;
	UINT32 width, height;
	const char *desc;

	memset(optionsmenu.modedescs, 0, sizeof(optionsmenu.modedescs));

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	VID_PrepareModeList(); // FIXME: hack
#endif
	optionsmenu.vidm_nummodes = 0;
	optionsmenu.vidm_selected = 0;
	nummodes = VID_NumModes();

	// DOS does not skip mode 0, because mode 0 is ALWAYS present
	i = 0;
	for (; i < nummodes && optionsmenu.vidm_nummodes < MAXMODEDESCS; i++)
	{
		desc = VID_GetModeName(i);
		if (desc)
		{
			vdup = 0;

			// when a resolution exists both under VGA and VESA, keep the
			// VESA mode, which is always a higher modenum
			for (j = 0; j < optionsmenu.vidm_nummodes; j++)
			{
				if (!strcmp(optionsmenu.modedescs[j].desc, desc))
				{
					// mode(0): 320x200 is always standard VGA, not vesa
					if (optionsmenu.modedescs[j].modenum)
					{
						optionsmenu.modedescs[j].modenum = i;
						vdup = 1;

						if (i == vid.modenum)
							optionsmenu.vidm_selected = j;
					}
					else
						vdup = 1;

					break;
				}
			}

			if (!vdup)
			{
				optionsmenu.modedescs[optionsmenu.vidm_nummodes].modenum = i;
				optionsmenu.modedescs[optionsmenu.vidm_nummodes].desc = desc;

				if (i == vid.modenum)
					optionsmenu.vidm_selected = optionsmenu.vidm_nummodes;

				// Pull out the width and height
				sscanf(desc, "%u%*c%u", &width, &height);

				// Show multiples of 320x200 as green.
				if (SCR_IsAspectCorrect(width, height))
					optionsmenu.modedescs[optionsmenu.vidm_nummodes].goodratio = 1;

				optionsmenu.vidm_nummodes++;
			}
		}
	}

	optionsmenu.vidm_column_size = (optionsmenu.vidm_nummodes+2) / 3;
}

// special menuitem key handler for video mode list
void M_HandleVideoModes(INT32 ch)
{

	const UINT8 pid = 0;
	(void)ch;

	if (optionsmenu.vidm_testingmode > 0)
	{
		// change back to the previous mode quickly
		if (M_MenuBackPressed(pid))
		{
			setmodeneeded = optionsmenu.vidm_previousmode + 1;
			optionsmenu.vidm_testingmode = 0;
		}
		else if (M_MenuConfirmPressed(pid))
		{
			S_StartSound(NULL, sfx_s3k5b);
			optionsmenu.vidm_testingmode = 0; // stop testing
		}
	}

	else
	{
		if (menucmd[pid].dpad_ud > 0)
		{
			S_StartSound(NULL, sfx_s3k5b);
			if (++optionsmenu.vidm_selected >= optionsmenu.vidm_nummodes)
				optionsmenu.vidm_selected = 0;

			M_SetMenuDelay(pid);
		}

		else if (menucmd[pid].dpad_ud < 0)
		{
			S_StartSound(NULL, sfx_s3k5b);
			if (--optionsmenu.vidm_selected < 0)
				optionsmenu.vidm_selected = optionsmenu.vidm_nummodes - 1;

			M_SetMenuDelay(pid);
		}

		else if (menucmd[pid].dpad_lr < 0)
		{
			S_StartSound(NULL, sfx_s3k5b);
			optionsmenu.vidm_selected -= optionsmenu.vidm_column_size;
			if (optionsmenu.vidm_selected < 0)
				optionsmenu.vidm_selected = (optionsmenu.vidm_column_size*3) + optionsmenu.vidm_selected;
			if (optionsmenu.vidm_selected >= optionsmenu.vidm_nummodes)
				optionsmenu.vidm_selected = optionsmenu.vidm_nummodes - 1;

			M_SetMenuDelay(pid);
		}

		else if (menucmd[pid].dpad_lr > 0)
		{
			S_StartSound(NULL, sfx_s3k5b);
			optionsmenu.vidm_selected += optionsmenu.vidm_column_size;
			if (optionsmenu.vidm_selected >= (optionsmenu.vidm_column_size*3))
				optionsmenu.vidm_selected %= optionsmenu.vidm_column_size;
			if (optionsmenu.vidm_selected >= optionsmenu.vidm_nummodes)
				optionsmenu.vidm_selected = optionsmenu.vidm_nummodes - 1;

			M_SetMenuDelay(pid);
		}

		else if (M_MenuConfirmPressed(pid))
		{
			M_SetMenuDelay(pid);
			S_StartSound(NULL, sfx_s3k5b);
			if (vid.modenum == optionsmenu.modedescs[optionsmenu.vidm_selected].modenum)
				SCR_SetDefaultMode();
			else
			{
				optionsmenu.vidm_testingmode = 15*TICRATE;
				optionsmenu.vidm_previousmode = vid.modenum;
				if (!setmodeneeded) // in case the previous setmode was not finished
					setmodeneeded = optionsmenu.modedescs[optionsmenu.vidm_selected].modenum + 1;
			}
		}

		else if (M_MenuBackPressed(pid))
		{
			M_SetMenuDelay(pid);
			if (currentMenu->prevMenu)
				M_SetupNextMenu(currentMenu->prevMenu, false);
			else
				M_ClearMenus(true);
		}
	}
}
