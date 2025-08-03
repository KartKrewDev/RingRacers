// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/main-1.c
/// \brief Main Menu

// ==========================================================================
// ORGANIZATION START.
// ==========================================================================
// Note: Never should we be jumping from one category of menu options to another
//       without first going to the Main Menu.
// Note: Ignore the above if you're working with the Pause menu.
// Note: (Prefix)_MainMenu should be the target of all Main Menu options that
//       point to submenus.

#include "../k_menu.h"
#include "../m_random.h"
#include "../s_sound.h"
#include "../i_time.h"
#include "../v_video.h"
#include "../z_zone.h"
#include "../i_video.h" // I_FinishUpdate
#include "../i_system.h" // I_Sleep
#include "../m_cond.h" // M_GameTrulyStarted

menuitem_t MainMenu[] =
{
	{IT_STRING | IT_CALL, "Play",
		"Cut to the chase and start the race!", NULL,
		{.routine = M_CharacterSelect}, 0, 0},

	{IT_STRING | IT_CALL, "Extras",
		"Check out some bonus features.", "MENUI001",
		{.routine = M_InitExtras}, 0, 0},

	{IT_STRING | IT_CALL, "Options",
		"Configure your controls, settings, and preferences.", "MENUI010",
		{.routine = M_InitOptions}, 0, 0},

	{IT_STRING | IT_CALL, "Quit",
		"Exit \"Dr. Robotnik's Ring Racers\".", NULL,
		{.routine = M_QuitSRB2}, 0, 0},
};

menu_t MainDef = KARTGAMEMODEMENU(MainMenu, NULL);

// Quit Game
static INT32 quitsounds[] =
{
	// holy shit we're changing things up!
	// srb2kart: you ain't seen nothing yet
	sfx_kc2e,
	sfx_kc2f,
	sfx_cdfm01,
	sfx_ddash,
	sfx_s3ka2,
	sfx_s3k49,
	sfx_slip,
	sfx_tossed,
	sfx_s3k7b,
	sfx_itrolf,
	sfx_itrole,
	sfx_cdpcm9,
	sfx_s3k4e,
	sfx_s259,
	sfx_3db06,
	sfx_s3k3a,
	sfx_peel,
	sfx_cdfm28,
	sfx_s3k96,
	sfx_s3kc0s,
	sfx_cdfm39,
	sfx_hogbom,
	sfx_kc5a,
	sfx_kc46,
	sfx_s3k92,
	sfx_s3k42,
	sfx_kpogos,
	sfx_screec
};

void M_QuitSRB2(INT32 choice)
{
	// We pick index 0 which is language sensitive, or one at random,
	// between 1 and maximum number.
	// ------------------------------------------------------------//
	// ...no we don't! We haven't for ages!
	// But I'm leaving that comment in, unmodified, because it dates
	// ALL the way back to the original 1993 Doom source publication.
	// One act of kindness has far-reaching consequences for so many
	// people. It's a week until christmas as I'm writing this --
	// for those who read this, what act of kindness can you bring
	// to others? ~toast 181223

	(void)choice;

	if (!M_GameAboutToStart() && M_GameTrulyStarted())
	{
		INT32 mrand = M_RandomKey(sizeof(quitsounds) / sizeof(INT32));
		if (quitsounds[mrand])
			S_StartSound(NULL, quitsounds[mrand]);

		M_StartMessage(
			"Quit Game",
			"Are you sure you want to quit playing?\n",
			&M_QuitResponse, MM_YESNO,
			"Leave the game",
			"No, I want to go back!"
		);

		return;
	}

	M_StartMessage(
		"Exit Program",
		"Are you sure you want to quit?\n",
		&M_QuitResponse, MM_YESNO,
		"Yes",
		"Cancel"
	);
}

void M_QuitResponse(INT32 ch)
{
	if (ch == MA_YES)
	{
		I_Quit();
	}
}
