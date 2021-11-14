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
	{IT_STRING | IT_CALL, "Play",
		"Cut to the chase and start the race!", NULL,
		M_CharacterSelectInit, 0, 0},

	{IT_STRING, "Extra",
		"Check out some bonus features.", "MENUI001",
		NULL, 0, 0},

	{IT_STRING, "Option",
		"Configure your controls, settings, and preferences.", NULL,
		NULL, 0, 0},

	{IT_STRING | IT_CALL, "Quit",
		"Exit \"Dr. Robotnik's Ring Racers\".", NULL,
		M_QuitSRB2, 0, 0},
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
	M_CharacterSelectTick,
	M_CharacterSelectQuit
};

menuitem_t PLAY_MainMenu[] =
{
	{IT_STRING | IT_SUBMENU, "Local Play", "Play only on this computer.",
		NULL, &PLAY_GamemodesDef, 0, 0},

	{IT_STRING | IT_CALL, "Online", "Connect to other computers.",
		NULL, M_MPOptSelectInit, /*M_MPRoomSelectInit,*/ 0, 0},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, M_GoBack, 0, 0},
};

menu_t PLAY_MainDef = KARTGAMEMODEMENU(PLAY_MainMenu, &PLAY_CharSelectDef);

menuitem_t PLAY_GamemodesMenu[] =
{
	{IT_STRING | IT_SUBMENU, "Race", "A contest to see who's the fastest of them all!",
		NULL, &PLAY_RaceGamemodesDef, 0, 0},

	{IT_STRING | IT_SUBMENU, "Battle", "Sharpen your item usage in these special Battle zones!",
		NULL, &PLAY_BattleGamemodesDef, 0, 0},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, M_GoBack, 0, 0},
};

menu_t PLAY_GamemodesDef = KARTGAMEMODEMENU(PLAY_GamemodesMenu, &PLAY_MainDef);

// RACE

menuitem_t PLAY_RaceGamemodesMenu[] =
{
	{IT_STRING | IT_CALL, "Grand Prix", "Compete for the best rank over five races!",
		NULL, M_LevelSelectInit, 2, GT_RACE},

	{IT_STRING | IT_CALL, "Match Race", "Play by your own rules in a specialized, single race!",
		"MENIMG01", M_LevelSelectInit, 0, GT_RACE},

	{IT_STRING | IT_CALL, "Time Attack", "Record your best time on any track!",
		NULL, M_LevelSelectInit, 1, GT_RACE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, M_GoBack, 0, 0},
};

menu_t PLAY_RaceGamemodesDef = KARTGAMEMODEMENU(PLAY_RaceGamemodesMenu, &PLAY_GamemodesDef);

menuitem_t PLAY_CupSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_CupSelectHandler, 0, 0},
};

menu_t PLAY_CupSelectDef = {
	sizeof(PLAY_CupSelect) / sizeof(menuitem_t),
	&PLAY_RaceGamemodesDef,
	0,
	PLAY_CupSelect,
	0, 0,
	2, 10,
	M_DrawCupSelect,
	M_CupSelectTick,
	NULL
};

menuitem_t PLAY_LevelSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_LevelSelectHandler, 0, 0},
};

menu_t PLAY_LevelSelectDef = {
	sizeof(PLAY_LevelSelect) / sizeof(menuitem_t),
	&PLAY_CupSelectDef,
	0,
	PLAY_LevelSelect,
	0, 0,
	2, 10,
	M_DrawLevelSelect,
	M_LevelSelectTick,
	NULL
};

menuitem_t PLAY_TimeAttack[] =
{
	{IT_STRING, "Replay...", NULL, NULL, NULL, 0, 0},
	{IT_STRING, "Ghosts...", NULL, NULL, NULL, 0, 0},
	{IT_SPACE, NULL, NULL, NULL, NULL, 0, 0},
	{IT_STRING, "Start", NULL, NULL, NULL, 0, 0},
};

menu_t PLAY_TimeAttackDef = {
	sizeof(PLAY_TimeAttack) / sizeof(menuitem_t),
	&PLAY_LevelSelectDef,
	0,
	PLAY_TimeAttack,
	0, 0,
	2, 10,
	M_DrawTimeAttack,
	NULL,
	NULL
};

// BATTLE

menuitem_t PLAY_BattleGamemodesMenu[] =
{
	{IT_STRING | IT_CALL, "Survival", "It's last hedgehog standing in this free-for-all!",
		"MENIMG00", M_LevelSelectInit, 0, GT_BATTLE},

	{IT_STRING | IT_CALL, "Time Attack", "Bust up all of the capsules in record time!",
		NULL, M_LevelSelectInit, 1, GT_BATTLE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, M_GoBack, 0, 0},
};

menu_t PLAY_BattleGamemodesDef = KARTGAMEMODEMENU(PLAY_BattleGamemodesMenu, &PLAY_GamemodesDef);

// MULTIPLAYER OPTION SELECT
menuitem_t PLAY_MP_OptSelect[] =
{
	//{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPOptSelect, 0, 0},
	{IT_STRING | IT_CALL, "Host Game", "Start your own online game!",
		NULL, M_MPHostInit, 0, 0},

	{IT_STRING | IT_CALL, "Join by IP", "Join an online game by its IP address.",
		NULL, M_MPJoinIPInit, 0, 0},

	{IT_STRING | IT_CALL, "Server Browser", "Search for game servers to play in.",
		NULL, M_MPRoomSelectInit, 0, 0},
};

menu_t PLAY_MP_OptSelectDef = {
	sizeof (PLAY_MP_OptSelect) / sizeof (menuitem_t),
	&PLAY_MainDef,
	0,
	PLAY_MP_OptSelect,
	0, 0,
	-1, 1,
	M_DrawMPOptSelect,
	M_MPOptSelectTick,
	NULL
};

// MULTIPLAYER HOST SCREEN
menuitem_t PLAY_MP_Host[] =
{
	//{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPOptSelect, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Name", "Display name for your game online. Other players will see this.",
		NULL, &cv_servername, 0, 0},

	{IT_STRING | IT_CVAR, "Public Server", "Display or not your game in the Server Browser for other players.",
		NULL, &cv_advertise, 0, 0},

	{IT_STRING | IT_CVAR, "Max. Players", "Set how many players can play at once. Others will spectate.",
		NULL, &cv_ingamecap, 0, 0},

	{IT_STRING | IT_CVAR, "Gamemode", "Are we racing? Or perhaps battling?",
		NULL, &cv_dummygametype, 0, 0},

	{IT_STRING | IT_CALL, "GO", "Select a map with the currently selected gamemode",
		NULL, M_MPSetupNetgameMapSelect, 0, 0},

};

menu_t PLAY_MP_HostDef = {
	sizeof (PLAY_MP_Host) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_Host,
	0, 0,
	-1, 1,	// 1 frame transition.... This is really just because I don't want the black fade when we press esc, hehe
	M_DrawMPHost,
	M_MPOptSelectTick,	// This handles the unfolding options
	M_MPResetOpts
};

// MULTIPLAYER JOIN BY IP
menuitem_t PLAY_MP_JoinIP[] =
{
	//{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPOptSelect, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Address: ", "Type the IPv4 address of the server you wish to connect to.",
		NULL, &cv_dummyip, 0, 0},

	{IT_STRING | IT_CALL, "GO", "Select a map with the currently selected gamemode",
		NULL, M_JoinIP, 0, 0},
};

menu_t PLAY_MP_JoinIPDef = {
	sizeof (PLAY_MP_JoinIP) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_JoinIP,
	0, 0,
	-1, 1,	// 1 frame transition.... This is really just because I don't want the black fade when we press esc, hehe
	M_DrawMPJoinIP,
	M_MPOptSelectTick,	// This handles the unfolding options
	M_MPResetOpts
};

// MULTIPLAYER ROOM SELECT (CORE / MODDED)
menuitem_t PLAY_MP_RoomSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPRoomSelect, 0, 0},
};

menu_t PLAY_MP_RoomSelectDef = {
	sizeof (PLAY_MP_RoomSelect) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_RoomSelect,
	0, 0,
	0, 0,
	M_DrawMPRoomSelect,
	M_MPRoomSelectTick,
	NULL
};


// -------------------
// In-game/pause menus
// -------------------

menuitem_t PAUSE_PlaybackMenu[] =
{
	{IT_CALL   | IT_STRING, "Hide Menu (Esc)",			NULL, "M_PHIDE",	M_SelectableClearMenus,		  0, 0},

	{IT_CALL   | IT_STRING, "Rewind ([)",				NULL, "M_PREW",		M_PlaybackRewind,			 20, 0},
	{IT_CALL   | IT_STRING, "Pause (\\)",				NULL, "M_PPAUSE",	M_PlaybackPause,			 36, 0},
	{IT_CALL   | IT_STRING, "Fast-Forward (])",			NULL, "M_PFFWD",	M_PlaybackFastForward,		 52, 0},
	{IT_CALL   | IT_STRING, "Backup Frame ([)",			NULL, "M_PSTEPB",	M_PlaybackRewind,			 20, 0},
	{IT_CALL   | IT_STRING, "Resume",					NULL, "M_PRESUM",	M_PlaybackPause,			 36, 0},
	{IT_CALL   | IT_STRING, "Advance Frame (])",		NULL, "M_PFADV",	M_PlaybackAdvance,			 52, 0},

	{IT_ARROWS | IT_STRING, "View Count (- and =)",		NULL, "M_PVIEWS",	M_PlaybackSetViews,			 72, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint (1)",			NULL, "M_PNVIEW",	M_PlaybackAdjustView,		 88, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 2 (2)",			NULL, "M_PNVIEW",	M_PlaybackAdjustView,		104, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 3 (3)",			NULL, "M_PNVIEW",	M_PlaybackAdjustView,		120, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 4 (4)",			NULL, "M_PNVIEW",	M_PlaybackAdjustView,		136, 0},

	{IT_CALL   | IT_STRING, "Toggle Free Camera (')",	NULL, "M_PVIEWS",	M_PlaybackToggleFreecam,	156, 0},
	{IT_CALL   | IT_STRING, "Stop Playback",			NULL, "M_PEXIT",	M_PlaybackQuit,				172, 0},
};

menu_t PAUSE_PlaybackMenuDef = {
	sizeof (PAUSE_PlaybackMenu) / sizeof (menuitem_t),
	NULL,
	0,
	PAUSE_PlaybackMenu,
	BASEVIDWIDTH/2 - 88, 2,
	0, 0,
	M_DrawPlaybackMenu,
	NULL,
	NULL
};
