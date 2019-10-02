/// \file  k_menudef.c
/// \brief SRB2Kart menu definitions

#include "k_menu.h"
#include "screen.h" // BASEVIDWIDTH

// ==========================================================================
// ORGANIZATION START.
// ==========================================================================
// Note: Never should we be jumping from one category of menu options to another
//       without first going to the Main Menu.
// Note: Ignore the above if you're working with the Pause menu.
// Note: (Prefix)_MainMenu should be the target of all Main Menu options that
//       point to submenus.

// ---------
// Main Menu
// ---------
menuitem_t MainMenu[] =
{
	{IT_STRING | IT_CALL, "Play", "Cut to the chase and start playing!",
		NULL, M_CharacterSelectInit, 48, 0},

	{IT_STRING, "Extra", "Check out some bonus features.",
		NULL, NULL, 80, 0},

	{IT_STRING, "Option", "Configure your controls, settings, and preferences.",
		NULL, NULL, 112, 0},

	{IT_STRING | IT_CALL, "Quit", "Exit SRB2Kart.",
		NULL, M_QuitSRB2, 160, 0},
};

menu_t MainDef = KARTGAMEMODEMENU(MainMenu, NULL);

// ---------
// Play Menu
// ---------

menuitem_t PLAY_CharSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_CharacterSelectHandler, 0, 0},
};

menu_t PLAY_CharSelectDef = {
	sizeof (PLAY_CharSelect) / sizeof (menuitem_t),
	&MainDef,
	0,
	PLAY_CharSelect,
	0, 0,
	0, 0,
	M_DrawCharacterSelect,
	M_CharacterSelectQuit
};

menuitem_t PLAY_MainMenu[] =
{
	{IT_STRING | IT_SUBMENU, "Local Play", "Play only on this computer.",
		NULL, &PLAY_GamemodesDef, 64, 0},

	{IT_STRING, "Online", "Connect to other computers.",
		NULL, NULL, 96, 0},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, M_GoBack, 160, 0},
};

menu_t PLAY_MainDef = KARTGAMEMODEMENU(PLAY_MainMenu, &PLAY_CharSelectDef);

menuitem_t PLAY_GamemodesMenu[] =
{
	{IT_STRING | IT_SUBMENU, "Race", "A competition for the best time!",
		NULL, &PLAY_RaceGamemodesDef, 64, 0},

	{IT_STRING, "Battle", "Clash against other players in a survival match!",
		NULL, NULL, 96, 0},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, M_GoBack, 160, 0},
};

menu_t PLAY_GamemodesDef = KARTGAMEMODEMENU(PLAY_GamemodesMenu, &PLAY_MainDef);

menuitem_t PLAY_RaceGamemodesMenu[] =
{
	{IT_STRING, "Grand Prix", "Compete for the best rank over five races!",
		NULL, NULL, 48, 0},

	{IT_STRING, "Match Race", "Pick your own settings in a specialized single race.",
		NULL, NULL, 80, 0},

	{IT_STRING, "Time Attack", "Race against ghosts for the best time, no fluff.",
		NULL, NULL, 112, 0},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, M_GoBack, 160, 0},
};

menu_t PLAY_RaceGamemodesDef = KARTGAMEMODEMENU(PLAY_RaceGamemodesMenu, &PLAY_GamemodesDef);

// -------------------
// In-game/pause menus
// -------------------

menuitem_t PAUSE_PlaybackMenu[] =
{
	{IT_CALL   | IT_STRING, "Hide Menu", NULL, "M_PHIDE", M_SelectableClearMenus, 0, 0},

	{IT_CALL   | IT_STRING, "Rewind", NULL, "M_PREW",        M_PlaybackRewind,      20, 0},
	{IT_CALL   | IT_STRING, "Pause", NULL, "M_PPAUSE",       M_PlaybackPause,       36, 0},
	{IT_CALL   | IT_STRING, "Fast-Forward", NULL, "M_PFFWD", M_PlaybackFastForward, 52, 0},

	{IT_CALL   | IT_STRING, "Backup Frame", NULL, "M_PSTEPB", M_PlaybackRewind,  20, 0},
	{IT_CALL   | IT_STRING, "Resume", NULL, "M_PRESUM",       M_PlaybackPause,   36, 0},
	{IT_CALL   | IT_STRING, "Advance Frame", NULL, "M_PFADV", M_PlaybackAdvance, 52, 0},

	{IT_ARROWS | IT_STRING, "View Count", NULL, "M_PVIEWS",  M_PlaybackSetViews,    72, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint", NULL, "M_PNVIEW",   M_PlaybackAdjustView,  88, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 2", NULL, "M_PNVIEW", M_PlaybackAdjustView, 104, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 3", NULL, "M_PNVIEW", M_PlaybackAdjustView, 120, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 4", NULL, "M_PNVIEW", M_PlaybackAdjustView, 136, 0},

	{IT_CALL   | IT_STRING, "Stop Playback", NULL, "M_PEXIT", M_PlaybackQuit, 156, 0},
};

menu_t PAUSE_PlaybackMenuDef = {
	sizeof (PAUSE_PlaybackMenu) / sizeof (menuitem_t),
	NULL,
	0,
	PAUSE_PlaybackMenu,
	BASEVIDWIDTH/2 - 88, 2,
	0, 0,
	M_DrawPlaybackMenu,
	NULL
};
