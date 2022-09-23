/// \file  k_menudef.c
/// \brief SRB2Kart menu definitions

#include "k_menu.h"
#include "screen.h" // BASEVIDWIDTH
#include "r_main.h"	// cv_skybox
#include "v_video.h" // cv_globalgamma
#include "hardware/hw_main.h"	// gl consvars
#include "s_sound.h"	// sounds consvars
#include "g_game.h" // cv_chatnotifications
#include "console.h" // console cvars
#include "filesrch.h" // addons cvars
#include "m_misc.h" // screenshot cvars
#include "r_fps.h" // fps cvars
#include "discord.h" // discord rpc cvars

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
		{.routine = M_CharacterSelect}, 0, 0},

	{IT_STRING | IT_CALL, "Extras",
		"Check out some bonus features.", "MENUI001",
		{.routine = M_InitExtras}, 0, 0},

	{IT_STRING, "Options",
		"Configure your controls, settings, and preferences.", NULL,
		{.routine = M_InitOptions}, 0, 0},

	{IT_STRING | IT_CALL, "Quit",
		"Exit \"Dr. Robotnik's Ring Racers\".", NULL,
		{.routine = M_QuitSRB2}, 0, 0},
};

menu_t MainDef = KARTGAMEMODEMENU(MainMenu, NULL);

// ---------
// Play Menu
// ---------

menuitem_t PLAY_CharSelect[] =
{
	{IT_NOTHING, NULL, NULL, NULL, {NULL}, 0, 0},
};

menu_t PLAY_CharSelectDef = {
	sizeof (PLAY_CharSelect) / sizeof (menuitem_t),
	&MainDef,
	0,
	PLAY_CharSelect,
	0, 0,
	0, 0,
	0, 0,
	M_DrawCharacterSelect,
	M_CharacterSelectTick,
	M_CharacterSelectInit,
	M_CharacterSelectQuit,
	M_CharacterSelectHandler
};

menuitem_t PLAY_MainMenu[] =
{
	{IT_STRING | IT_CALL, "Local Play", "Play only on this computer.",
		NULL, {.routine = M_SetupGametypeMenu}, 0, 0},

	{IT_STRING | IT_CALL, "Online", "Connect to other computers.",
		NULL, {.routine = M_MPOptSelectInit}, /*M_MPRoomSelectInit,*/ 0, 0},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_MainDef = KARTGAMEMODEMENU(PLAY_MainMenu, &PLAY_CharSelectDef);

menuitem_t PLAY_GamemodesMenu[] =
{
	{IT_STRING | IT_CALL, "Race", "A contest to see who's the fastest of them all!",
		NULL, {.routine = M_SetupRaceMenu}, 0, 0},

	{IT_STRING | IT_CALL, "Battle", "It's last hedgehog standing in this free-for-all!",
		"MENIMG00", {.routine = M_LevelSelectInit}, 0, GT_BATTLE},

	{IT_STRING | IT_CALL, "Capsules", "Bust up all of the capsules in record time!",
		NULL, {.routine = M_LevelSelectInit}, 1, GT_BATTLE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_GamemodesDef = KARTGAMEMODEMENU(PLAY_GamemodesMenu, &PLAY_MainDef);

// RACE

menuitem_t PLAY_RaceGamemodesMenu[] =
{
	{IT_STRING | IT_CALL, "Grand Prix", "Compete for the best rank over five races!",
		NULL, {.routine = M_SetupDifficultySelect}, 0, 0},

	{IT_STRING | IT_CALL, "Match Race", "Play by your own rules in a specialized, single race!",
		"MENIMG01", {.routine = M_SetupDifficultySelect}, 1, 0},

	{IT_STRING | IT_CALL, "Time Attack", "Record your best time on any track!",
		NULL, {.routine = M_LevelSelectInit}, 1, GT_RACE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_RaceGamemodesDef = KARTGAMEMODEMENU(PLAY_RaceGamemodesMenu, &PLAY_GamemodesDef);


// difficulty selection -- see drace_e
menuitem_t PLAY_RaceDifficulty[] =
{
	// For GP
	{IT_STRING | IT_CVAR, "Difficulty", "Select the game difficulty",
		NULL, {.cvar = &cv_dummygpdifficulty}, 0, 0},

	// Match Race
	{IT_STRING | IT_CVAR, "Difficulty", "Select the game speed",
		NULL, {.cvar = &cv_dummykartspeed}, 0, 0},

	// DISABLE THAT OPTION OUTSIDE OF MATCH RACE
	{IT_STRING2 | IT_CVAR, "CPU", "Set the difficulty of CPU players.",
		NULL, {.cvar = &cv_dummymatchbots}, 0, 0},
	{IT_STRING2 | IT_CVAR, "Racers", "Sets the number of racers, including players and CPU.",
		NULL, {.cvar = &cv_maxplayers}, 0, 0},

	{IT_STRING2 | IT_CVAR, "Encore", "Enable or disable Encore mode",
		NULL, {.cvar = &cv_dummygpencore}, 0, 0},

	// For GP
	{IT_STRING | IT_CALL, "Cup Select", "Go on and select a cup!", NULL, {.routine = M_LevelSelectInit}, 2, GT_RACE},

	// Match Race
	{IT_STRING | IT_CALL, "Map Select", "Go on and select a race track!", NULL, {.routine = M_LevelSelectInit}, 0, GT_RACE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PLAY_RaceDifficultyDef = {
	sizeof(PLAY_RaceDifficulty) / sizeof(menuitem_t),
	&PLAY_RaceGamemodesDef,
	0,
	PLAY_RaceDifficulty,
	0, 0,
	0, 0,
	1, 5,
	M_DrawRaceDifficulty,
	NULL,
	NULL,
	NULL,
	NULL
};


menuitem_t PLAY_CupSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_CupSelectHandler}, 0, 0},
};

menu_t PLAY_CupSelectDef = {
	sizeof(PLAY_CupSelect) / sizeof(menuitem_t),
	&PLAY_RaceGamemodesDef,
	0,
	PLAY_CupSelect,
	0, 0,
	0, 0,
	2, 5,
	M_DrawCupSelect,
	M_CupSelectTick,
	NULL,
	NULL,
	NULL
};

menuitem_t PLAY_LevelSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_LevelSelectHandler}, 0, 0},
};

menu_t PLAY_LevelSelectDef = {
	sizeof(PLAY_LevelSelect) / sizeof(menuitem_t),
	&PLAY_CupSelectDef,
	0,
	PLAY_LevelSelect,
	0, 0,
	0, 0,
	2, 5,
	M_DrawLevelSelect,
	M_LevelSelectTick,
	NULL,
	NULL,
	NULL
};

menuitem_t PLAY_TimeAttack[] =
{
	{IT_STRING | IT_SUBMENU, "Replay...", NULL, NULL, {.submenu = &PLAY_TAReplayDef}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Guest...", NULL, NULL, {.submenu = &PLAY_TAReplayGuestDef}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Ghosts...", NULL, NULL, {.submenu = &PLAY_TAGhostsDef}, 0, 0},
	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_CALL, "Start", NULL, NULL, {.routine = M_StartTimeAttack}, 0, 0},
};

menu_t PLAY_TimeAttackDef = {
	sizeof(PLAY_TimeAttack) / sizeof(menuitem_t),
	&PLAY_LevelSelectDef,
	0,
	PLAY_TimeAttack,
	0, 0,
	0, 0,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	NULL,
	NULL,
	NULL
};


menuitem_t PLAY_TAReplay[] =
{
	{IT_STRING | IT_CALL, "Replay Best Time", NULL, NULL, {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Best Lap", NULL, NULL, {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Last", NULL, NULL, {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Guest", NULL, NULL, {.routine = M_ReplayTimeAttack}, 0, 0},
	{IT_STRING | IT_CALL, "Replay Staff", NULL, NULL, {.routine = M_HandleStaffReplay}, 0, 0},
	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Back", NULL, NULL, {.submenu = &PLAY_TimeAttackDef}, 0, 0},
};

menu_t PLAY_TAReplayDef = {
	sizeof(PLAY_TAReplay) / sizeof(menuitem_t),
	&PLAY_TimeAttackDef,
	0,
	PLAY_TAReplay,
	0, 0,
	0, 0,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	NULL,
	NULL,
	NULL
};

menuitem_t PLAY_TAReplayGuest[] =
{
	{IT_HEADERTEXT|IT_HEADER, "Save as guest...", NULL, NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "Best Time", NULL, NULL, {.routine = M_SetGuestReplay}, 0, 0},
	{IT_STRING | IT_CALL, "Best Lap", NULL, NULL, {.routine = M_SetGuestReplay}, 0, 0},
	{IT_STRING | IT_CALL, "Last Run", NULL, NULL, {.routine = M_SetGuestReplay}, 0, 0},

	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_CALL, "Delete Guest", NULL, NULL, {.routine = M_SetGuestReplay}, 0, 0},

	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Back", NULL, NULL, {.submenu = &PLAY_TimeAttackDef}, 0, 0},

};

menu_t PLAY_TAReplayGuestDef = {
	sizeof(PLAY_TAReplayGuest) / sizeof(menuitem_t),
	&PLAY_TimeAttackDef,
	0,
	PLAY_TAReplayGuest,
	0, 0,
	0, 0,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	NULL,
	NULL,
	NULL
};

menuitem_t PLAY_TAGhosts[] =
{
	{IT_STRING | IT_CVAR, "Best Time", NULL, NULL, {.cvar = &cv_ghost_besttime}, 0, 0},
	{IT_STRING | IT_CVAR, "Best Lap", NULL, NULL, {.cvar = &cv_ghost_bestlap}, 0, 0},
	{IT_STRING | IT_CVAR, "Last", NULL, NULL, {.cvar = &cv_ghost_last}, 0, 0},
	{IT_DISABLED, "Guest", NULL, NULL, {.cvar = &cv_ghost_guest}, 0, 0},
	{IT_DISABLED, "Staff", NULL, NULL, {.cvar = &cv_ghost_staff}, 0, 0},

	{IT_HEADERTEXT|IT_HEADER, "", NULL, NULL, {NULL}, 0, 0},
	{IT_STRING | IT_SUBMENU, "Back", NULL, NULL, {.submenu = &PLAY_TimeAttackDef}, 0, 0},
};

menu_t PLAY_TAGhostsDef = {
	sizeof(PLAY_TAGhosts) / sizeof(menuitem_t),
	&PLAY_TimeAttackDef,
	0,
	PLAY_TAGhosts,
	0, 0,
	0, 0,
	2, 5,
	M_DrawTimeAttack,
	NULL,
	NULL,
	NULL,
	NULL
};

// MULTIPLAYER OPTION SELECT
menuitem_t PLAY_MP_OptSelect[] =
{
	//{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPOptSelect, 0, 0},
	{IT_STRING | IT_CALL, "Host Game", "Start your own online game!",
		NULL, {.routine = M_MPHostInit}, 0, 0},

	{IT_STRING | IT_CALL, "Server Browser", "Search for game servers to play in.",
		NULL, {.routine = M_MPRoomSelectInit}, 0, 0},

	{IT_STRING | IT_CALL, "Join by IP", "Join an online game by its IP address.",
		NULL, {.routine = M_MPJoinIPInit}, 0, 0},
};

menu_t PLAY_MP_OptSelectDef = {
	sizeof (PLAY_MP_OptSelect) / sizeof (menuitem_t),
	&PLAY_MainDef,
	0,
	PLAY_MP_OptSelect,
	0, 0,
	0, 0,
	-1, 1,
	M_DrawMPOptSelect,
	M_MPOptSelectTick,
	NULL,
	NULL,
	NULL
};

// MULTIPLAYER HOST SCREEN -- see mhost_e
menuitem_t PLAY_MP_Host[] =
{
	//{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPOptSelect, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Name", "Display name for your game online. Other players will see this.",
		NULL, {.cvar = &cv_servername}, 0, 0},

	{IT_STRING | IT_CVAR, "Public Server", "Display or not your game in the Server Browser for other players.",
		NULL, {.cvar = &cv_advertise}, 0, 0},

	{IT_STRING | IT_CVAR, "Max. Players", "Set how many players can play at once. Others will spectate.",
		NULL, {.cvar = &cv_maxplayers}, 0, 0},

	{IT_STRING | IT_CVAR, "Gamemode", "Are we racing? Or perhaps battling?",
	NULL, {.cvar = &cv_dummygametype}, 0, 0},

	{IT_STRING | IT_CALL, "GO", "Select a map with the currently selected gamemode",
		NULL, {.routine = M_MPSetupNetgameMapSelect}, 0, 0},

};

menu_t PLAY_MP_HostDef = {
	sizeof (PLAY_MP_Host) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_Host,
	0, 0,
	0, 0,
	-1, 1,	// 1 frame transition.... This is really just because I don't want the black fade when we press esc, hehe
	M_DrawMPHost,
	M_MPOptSelectTick,	// This handles the unfolding options
	NULL,
	M_MPResetOpts,
	NULL
};

// MULTIPLAYER JOIN BY IP
menuitem_t PLAY_MP_JoinIP[] =
{
	//{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPOptSelect, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "IP: ", "Type the IPv4 address of the server.",
		NULL, {.cvar = &cv_dummyip}, 0, 0},

	{IT_STRING, "CONNECT ", "Attempt to connect to the server you entered the IP for.",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SPACE, "LAST IPs JOINED:", "Kanade best waifu :)",
		NULL, {NULL}, 0, 0},

	{IT_STRING, "servip1", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, {NULL}, 0, 0},

	{IT_STRING, "servip2", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, {NULL}, 0, 0},

	{IT_STRING, "servip3", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, {NULL}, 0, 0},

};

menu_t PLAY_MP_JoinIPDef = {
	sizeof (PLAY_MP_JoinIP) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_JoinIP,
	0, 0,
	0, 0,
	-1, 1,	// 1 frame transition.... This is really just because I don't want the black fade when we press esc, hehe
	M_DrawMPJoinIP,
	M_MPOptSelectTick,	// This handles the unfolding options
	NULL,
	M_MPResetOpts,
	M_JoinIPInputs
};

// MULTIPLAYER ROOM SELECT (CORE / MODDED)
menuitem_t PLAY_MP_RoomSelect[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_MPRoomSelect}, 0, 0},
};

menu_t PLAY_MP_RoomSelectDef = {
	sizeof (PLAY_MP_RoomSelect) / sizeof (menuitem_t),
	&PLAY_MP_OptSelectDef,
	0,
	PLAY_MP_RoomSelect,
	0, 0,
	0, 0,
	0, 0,
	M_DrawMPRoomSelect,
	M_MPRoomSelectTick,
	NULL,
	NULL,
	NULL
};

// SERVER BROWSER
menuitem_t PLAY_MP_ServerBrowser[] =
{

	{IT_STRING | IT_CVAR, "SORT BY", NULL,	// tooltip MUST be null.
		NULL, {.cvar = &cv_serversort}, 0, 0},

	{IT_STRING, "REFRESH", NULL,
		NULL, {NULL}, 0, 0},

	{IT_NOTHING, NULL, NULL, NULL, {NULL}, 0, 0},
};

menu_t PLAY_MP_ServerBrowserDef = {
	sizeof (PLAY_MP_ServerBrowser) / sizeof (menuitem_t),
	&PLAY_MP_RoomSelectDef,
	0,
	PLAY_MP_ServerBrowser,
	32, 36,
	0, 0,
	0, 0,
	M_DrawMPServerBrowser,
	M_MPServerBrowserTick,
	NULL,
	NULL,
	M_ServerBrowserInputs
};

// options menu --  see mopt_e
menuitem_t OPTIONS_Main[] =
{

	{IT_STRING | IT_CALL, "Profile Setup", "Remap keys & buttons to your likings.",
		NULL, {.routine = M_ProfileSelectInit}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Video Options", "Change video settings such as the resolution.",
		NULL, {.submenu = &OPTIONS_VideoDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Sound Options", "Adjust various sound settings such as the volume.",
		NULL, {.submenu = &OPTIONS_SoundDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "HUD Options", "Options related to the Heads-Up Display.",
		NULL, {.submenu = &OPTIONS_HUDDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Gameplay Options", "Change various game related options",
		NULL, {.submenu = &OPTIONS_GameplayDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Server Options", "Change various specific options for your game server.",
		NULL, {.submenu = &OPTIONS_ServerDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Data Options", "Miscellaneous data options such as the screenshot format.",
		NULL, {.submenu = &OPTIONS_DataDef}, 0, 0},

	{IT_STRING | IT_CALL, "Tricks & Secrets", "Those who bother reading a game manual always get the edge over those who don't!",
		NULL, {.routine = M_Manual}, 0, 0},
};

// For options menu, the 'extra1' field will determine the background colour to use for... the background! (What a concept!)
menu_t OPTIONS_MainDef = {
	sizeof (OPTIONS_Main) / sizeof (menuitem_t),
	&MainDef,
	0,
	OPTIONS_Main,
	0, 0,
	SKINCOLOR_SLATE, 0,
	2, 5,
	M_DrawOptions,
	M_OptionsTick,
	NULL,
	NULL,
	M_OptionsInputs
};

// profiles menu
// profile select
menuitem_t OPTIONS_Profiles[] = {
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Select a Profile.",
		NULL, {.routine = M_HandleProfileSelect}, 0, 0},     // dummy menuitem for the control func
};

menu_t OPTIONS_ProfilesDef = {
	sizeof (OPTIONS_Profiles) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Profiles,
	32, 80,
	SKINCOLOR_ULTRAMARINE, 0,
	2, 5,
	M_DrawProfileSelect,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

// Duplicate for main profile select.
menuitem_t MAIN_Profiles[] = {
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Select a profile to use or create a new Profile.",
		NULL, {.routine = M_HandleProfileSelect}, 0, 0},     // dummy menuitem for the control func
};

menu_t MAIN_ProfilesDef = {
	sizeof (MAIN_Profiles) / sizeof (menuitem_t),
	NULL,
	0,
	MAIN_Profiles,
	32, 80,
	SKINCOLOR_ULTRAMARINE, 0,
	2, 5,
	M_DrawProfileSelect,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};


menuitem_t OPTIONS_EditProfile[] = {
	{IT_STRING | IT_CVAR | IT_CV_STRING, "Profile Name", "6-character long name to identify this Profile.",
		NULL, {.cvar = &cv_dummyprofilename}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Player Name", "Name displayed online when using this Profile.",
	NULL, {.cvar = &cv_dummyprofileplayername}, 0, 0},

	{IT_STRING | IT_CALL, "Character", "Default character and color for this Profile.",
		NULL, {.routine = M_CharacterSelect}, 0, 0},

	{IT_STRING | IT_CALL, "Controls", "Select the button mappings for this Profile.",
	NULL, {.routine = M_ProfileDeviceSelect}, 0, 0},

	{IT_STRING | IT_CALL, "Confirm", "Confirm changes.",
	NULL, {.routine = M_ConfirmProfile}, 0, 0},

};

menu_t OPTIONS_EditProfileDef = {
	sizeof (OPTIONS_EditProfile) / sizeof (menuitem_t),
	&OPTIONS_ProfilesDef,
	0,
	OPTIONS_EditProfile,
	32, 80,
	SKINCOLOR_ULTRAMARINE, 0,
	2, 5,
	M_DrawEditProfile,
	M_HandleProfileEdit,
	NULL,
	NULL,
	M_ProfileEditInputs,
};

menuitem_t OPTIONS_ProfileControls[] = {

	{IT_HEADER, "MAIN CONTROLS", "That's the stuff on the controller!!",
		NULL, {NULL}, 0, 0},

	{IT_CONTROL, "A", "Accelerate / Confirm",
		"PR_BTA", {.routine = M_ProfileSetControl}, gc_a, 0},

	{IT_CONTROL, "B", "Look backwards / Back",
		"PR_BTB", {.routine = M_ProfileSetControl}, gc_b, 0},

	{IT_CONTROL, "C", "Spindash / Extra",
		"PR_BTC", {.routine = M_ProfileSetControl}, gc_c, 0},

	{IT_CONTROL, "X", "Brake / Back",
		"PR_BTX", {.routine = M_ProfileSetControl}, gc_x, 0},

	// @TODO What does this do???
	{IT_CONTROL, "Y", "N/A ?",
		"PR_BTY", {.routine = M_ProfileSetControl}, gc_y, 0},

	{IT_CONTROL, "Z", "N/A ?",
		"PR_BTZ", {.routine = M_ProfileSetControl}, gc_z, 0},

	{IT_CONTROL, "L", "Use item",
		"PR_BTL", {.routine = M_ProfileSetControl}, gc_l, 0},

	{IT_CONTROL, "R", "Drift",
		"PR_BTR", {.routine = M_ProfileSetControl}, gc_r, 0},

	{IT_CONTROL, "Turn Left", "Turn left",
		"PR_PADL", {.routine = M_ProfileSetControl}, gc_left, 0},

	{IT_CONTROL, "Turn Right", "Turn right",
		"PR_PADR", {.routine = M_ProfileSetControl}, gc_right, 0},

	{IT_CONTROL, "Aim Forward", "Aim forwards",
		"PR_PADU", {.routine = M_ProfileSetControl}, gc_up, 0},

	{IT_CONTROL, "Aim Backwards", "Aim backwards",
		"PR_PADD", {.routine = M_ProfileSetControl}, gc_down, 0},

	{IT_CONTROL, "Start", "Open pause menu",
		"PR_BTS", {.routine = M_ProfileSetControl}, gc_start, 0},

	{IT_HEADER, "OPTIONAL CONTROLS", "Take a screenshot, chat...",
		NULL, {NULL}, 0, 0},

	{IT_CONTROL, "SCREENSHOT", "Also usable with F8 on Keyboard.",
		NULL, {.routine = M_ProfileSetControl}, gc_screenshot, 0},

	{IT_CONTROL, "GIF CAPTURE", "Also usable with F9 on Keyboard.",
		NULL, {.routine = M_ProfileSetControl}, gc_recordgif, 0},

	{IT_CONTROL, "OPEN CHAT", "Opens chatbox in online games.",
		NULL, {.routine = M_ProfileSetControl}, gc_talk, 0},

	{IT_CONTROL, "OPEN TEAM CHAT", "Do we even have team gamemodes?",
		NULL, {.routine = M_ProfileSetControl}, gc_teamtalk, 0},

	{IT_CONTROL, "OPEN CONSOLE", "Opens the developer options console.",
		NULL, {.routine = M_ProfileSetControl}, gc_console, 0},

	{IT_CONTROL, "LUA/A", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_luaa, 0},

	{IT_CONTROL, "LUA/B", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_luab, 0},

	{IT_CONTROL, "LUA/C", "May be used by add-ons.",
		NULL, {.routine = M_ProfileSetControl}, gc_luac, 0},

	{IT_HEADER, "TOGGLES", "For per-player commands",
		NULL, {NULL}, 0, 0},

	{IT_CONTROL | IT_CVAR, "KICKSTART ACCEL", "Hold A to auto-accel. Tap it to cancel.",
		NULL, {.cvar = &cv_dummyprofilekickstart}, 0, 0},

	{IT_HEADER, "EXTRA", "",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "TRY MAPPINGS", "Test your controls.",
		NULL, {.routine = M_ProfileTryController}, 0, 0},

	{IT_STRING | IT_CALL, "CONFIRM", "Go back to profile setup.",
		NULL, {.routine = M_ProfileControlsConfirm}, 0, 0},
};



menu_t OPTIONS_ProfileControlsDef = {
	sizeof (OPTIONS_ProfileControls) / sizeof (menuitem_t),
	&OPTIONS_EditProfileDef,
	0,
	OPTIONS_ProfileControls,
	32, 80,
	SKINCOLOR_ULTRAMARINE, 0,
	3, 5,
	M_DrawProfileControls,
	M_HandleProfileControls,
	NULL,
	NULL,
	M_ProfileControlsInputs,
};

// video options menu...
// options menu
menuitem_t OPTIONS_Video[] =
{

	{IT_STRING | IT_CALL, "Set Resolution...", "Change the screen resolution for the game.",
		NULL, {.routine = M_VideoModeMenu}, 0, 0},

// A check to see if you're not running on a fucking antique potato powered stone i guess???????

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	{IT_STRING | IT_CVAR, "Fullscreen", "Set whether you want to use fullscreen or windowed mode.",
		NULL, {.cvar = &cv_fullscreen}, 0, 0},
#endif

	{IT_NOTHING|IT_SPACE, NULL, "Kanade best waifu! I promise!",
		NULL, {NULL}, 0, 0},

	// Everytime I see a screenshot at max gamma I die inside
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Gamma", "Adjusts the overall brightness of the game.",
		NULL, {.cvar = &cv_globalgamma}, 0, 0},

	{IT_STRING | IT_CVAR, "FPS Cap", "Handles the refresh rate of the game (does not affect gamelogic).",
		NULL, {.cvar = &cv_fpscap}, 0, 0},

	{IT_STRING | IT_CVAR, "Enable Skyboxes", "Turning this off will improve performance at the detriment of visuals for many maps.",
		NULL, {.cvar = &cv_skybox}, 0, 0},

	{IT_STRING | IT_CVAR, "Draw Distance", "How far objects can be drawn. Lower values may improve performance at the cost of visibility.",
		NULL, {.cvar = &cv_drawdist}, 0, 0},

	{IT_STRING | IT_CVAR, "Weather Draw Distance", "Affects how far weather visuals can be drawn. Lower values improve performance.",
		NULL, {.cvar = &cv_drawdist_precip}, 0, 0},

	{IT_STRING | IT_CVAR, "Show FPS", "Displays the game framerate at the lower right corner of the screen.",
		NULL, {.cvar = &cv_ticrate}, 0, 0},

	{IT_NOTHING|IT_SPACE, NULL, "Kanade best waifu! I promise!",
		NULL, {NULL}, 0, 0},

#ifdef HWRENDER
	{IT_STRING | IT_SUBMENU, "Hardware Options...", "For usage and configuration of the OpenGL renderer.",
		NULL, {.submenu = &OPTIONS_VideoOGLDef}, 0, 0},
#endif

};

menu_t OPTIONS_VideoDef = {
	sizeof (OPTIONS_Video) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Video,
	32, 80,
	SKINCOLOR_PLAGUE, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

menuitem_t OPTIONS_VideoModes[] = {

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Select a resolution.",
		NULL, {.routine = M_HandleVideoModes}, 0, 0},     // dummy menuitem for the control func

};

menu_t OPTIONS_VideoModesDef = {
	sizeof (OPTIONS_VideoModes) / sizeof (menuitem_t),
	&OPTIONS_VideoDef,
	0,
	OPTIONS_VideoModes,
	48, 80,
	SKINCOLOR_PLAGUE, 0,
	2, 5,
	M_DrawVideoModes,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

#ifdef HWRENDER
menuitem_t OPTIONS_VideoOGL[] =
{

	{IT_STRING | IT_CVAR, "Renderer", "Change renderers between Software and OpenGL",
		NULL, {.cvar = &cv_renderer}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "OPTIONS BELOW ARE OPENGL ONLY!", "Watch people get confused anyway!!",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "3D Models", "Use 3D models instead of sprites when applicable.",
		NULL, {.cvar = &cv_glmodels}, 0, 0},

	{IT_STRING | IT_CVAR, "Shaders", "Use GLSL Shaders. Turning them off increases performance at the expanse of visual quality.",
		NULL, {.cvar = &cv_glshaders}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Texture Quality", "Texture depth. Higher values are recommended.",
		NULL, {.cvar = &cv_scr_depth}, 0, 0},

	{IT_STRING | IT_CVAR, "Texture Filter", "Texture Filter. Nearest is recommended.",
		NULL, {.cvar = &cv_glfiltermode}, 0, 0},

	{IT_STRING | IT_CVAR, "Anisotropic", "Lower values will improve performance at a minor quality loss.",
		NULL, {.cvar = &cv_glanisotropicmode}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Sprite Billboarding", "Adjusts sprites when viewed from above or below to not make them appear flat.",
		NULL, {.cvar = &cv_glspritebillboarding}, 0, 0},

	{IT_STRING | IT_CVAR, "Software Perspective", "Emulates Software shearing when looking up or down. Not recommended.",
		NULL, {.cvar = &cv_glshearing}, 0, 0},
};

menu_t OPTIONS_VideoOGLDef = {
	sizeof (OPTIONS_VideoOGL) / sizeof (menuitem_t),
	&OPTIONS_VideoDef,
	0,
	OPTIONS_VideoOGL,
	32, 80,
	SKINCOLOR_PLAGUE, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
#endif

menuitem_t OPTIONS_Sound[] =
{

	{IT_STRING | IT_CVAR, "SFX", "Enable or disable sound effect playback.",
		NULL, {.cvar = &cv_gamesounds}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "SFX Volume", "Adjust the volume of sound effects.",
		NULL, {.cvar = &cv_soundvolume}, 0, 0},

	{IT_STRING | IT_CVAR, "Music", "Enable or disable music playback.",
		NULL, {.cvar = &cv_gamedigimusic}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Music Volume", "Adjust the volume of music playback.",
		NULL, {.cvar = &cv_digmusicvolume}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Reverse L/R Channels", "Reverse left & right channels for Stereo playback.",
		NULL, {.cvar = &stereoreverse}, 0, 0},

	{IT_STRING | IT_CVAR, "Surround", "Enables or disable Surround sound playback.",
		NULL, {.cvar = &surround}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Chat Notifications", "Set when to play notification sounds when chat messages are received.",
		NULL, {.cvar = &cv_chatnotifications}, 0, 0},

	{IT_STRING | IT_CVAR, "Character Voices", "Set how often to play character voices in game.",
		NULL, {.cvar = &cv_kartvoices}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Play Music While Unfocused", "Keeps playing music even if the game is not the active window.",
		NULL, {.cvar = &cv_playmusicifunfocused}, 0, 0},

	{IT_STRING | IT_CVAR, "Play SFX While Unfocused", "Keeps playing sound effects even if the game is not the active window.",
		NULL, {.cvar = &cv_playsoundifunfocused}, 0, 0},

	// @TODO: Sound test (there's currently no space on this menu, might be better to throw it in extras?)
};

menu_t OPTIONS_SoundDef = {
	sizeof (OPTIONS_Sound) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Sound,
	48, 80,
	SKINCOLOR_THUNDER, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

menuitem_t OPTIONS_HUD[] =
{

	{IT_STRING | IT_CVAR, "Show HUD (F3)", "Toggles HUD display. Great for taking screenshots!",
		NULL, {.cvar = &cv_showhud}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "HUD Opacity", "Non opaque values may have performance impacts in software mode.",
		NULL, {.cvar = &cv_translucenthud}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Minimap Opacity", "Changes the opacity of the minimap.",
		NULL, {.cvar = &cv_kartminimap}, 0, 0},

	{IT_STRING | IT_CVAR, "Speedometer", "Choose to what speed unit to display or toggle off the speedometer.",
		NULL, {.cvar = &cv_kartspeedometer}, 0, 0},

	{IT_STRING | IT_CVAR, "Display \"CHECK\"", "Displays an icon when a player is tailing you.",
		NULL, {.cvar = &cv_kartcheck}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Console Text Size", "Size of the text within the console.",
		NULL, {.cvar = &cv_constextsize}, 0, 0},

	// we spell words properly here.
	{IT_STRING | IT_CVAR, "Console Tint", "Change the background colour of the console.",
		NULL, {.cvar = &cons_backcolor}, 0, 0},

	{IT_STRING | IT_CVAR, "Show \"FOCUS LOST\"", "Displays \"FOCUS LOST\" when the game window isn't the active window.",
		NULL, {.cvar = &cv_showfocuslost}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Online HUD Options...", "HUD options related to the online chat box and other features.",
		NULL, {.submenu = &OPTIONS_HUDOnlineDef}, 0, 0},
};

menu_t OPTIONS_HUDDef = {
	sizeof (OPTIONS_HUD) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_HUD,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

menuitem_t OPTIONS_HUDOnline[] =
{

	{IT_STRING | IT_CVAR, "Chat Mode", "Choose whether to display chat in its own window or the console.",
		NULL, {.cvar = &cv_consolechat}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Chat Box Tint", "Changes the background colour of the chat box.",
		NULL, {.cvar = &cv_chatbacktint}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Chat Box Width", "Change the width of the Chat Box",
		NULL, {.cvar = &cv_chatwidth}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Chat Box Height", "Change the height of the Chat Box",
		NULL, {.cvar = &cv_chatheight}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Message Fadeout Time", "How long chat messages stay displayed with the chat closed.",
		NULL, {.cvar = &cv_chattime}, 0, 0},

	{IT_STRING | IT_CVAR, "Spam Protection", "Prevents too many message from a single player from being displayed.",
		NULL, {.cvar = &cv_chatspamprotection}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Local Ping Display", "In netgames, displays your ping at the lower right corner of the screen.",
		NULL, {.cvar = &cv_showping}, 0, 0},

};

menu_t OPTIONS_HUDOnlineDef = {
	sizeof (OPTIONS_HUDOnline) / sizeof (menuitem_t),
	&OPTIONS_HUDDef,
	0,
	OPTIONS_HUDOnline,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

// Gameplay options -- see gopt_e
menuitem_t OPTIONS_Gameplay[] =
{

	{IT_STRING | IT_CVAR, "Game Speed", "Change Game Speed for the next map.",
		NULL, {.cvar = &cv_kartspeed}, 0, 0},

	{IT_STRING | IT_CVAR, "Base Lap Count", "Change how many laps must be completed per race.",
		NULL, {.cvar = &cv_numlaps}, 0, 0},

	{IT_STRING | IT_CVAR, "Frantic Items", "Make item odds crazier with more powerful items!",
		NULL, {.cvar = &cv_kartfrantic}, 0, 0},

	{IT_STRING | IT_CVAR, "Encore Mode", "Forces Encore Mode on for the next map.",
		NULL, {.cvar = &cv_kartencore}, 0, 0},

	{IT_STRING | IT_CVAR, "Exit Countdown", "How long players have to finish after 1st place finishes.",
		NULL, {.cvar = &cv_countdowntime}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Time Limit", "Change the time limit for Battle rounds.",
		NULL, {.cvar = &cv_timelimit}, 0, 0},

	{IT_STRING | IT_CVAR, "Starting Bumpers", "Change how many bumpers player start with in Battle.",
		NULL, {.cvar = &cv_kartbumpers}, 0, 0},

	{IT_STRING | IT_CVAR, "Karma Comeback", "Enable Karma Comeback in Battle mode.",
		NULL, {.cvar = &cv_kartcomeback}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},
	
	{IT_STRING | IT_CVAR, "Minimum Input Delay", "Practice for online play! Higher = more delay.",
		NULL, {.cvar = &cv_mindelay}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Random Item Toggles...", "Change which items to enable for your games.",
		NULL, {.submenu = &OPTIONS_GameplayItemsDef}, 0, 0},

};

menu_t OPTIONS_GameplayDef = {
	sizeof (OPTIONS_Gameplay) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Gameplay,
	48, 80,
	SKINCOLOR_SCARLET, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

menuitem_t OPTIONS_GameplayItems[] =
{
	// Mostly handled by the drawing function.
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Super Rings",			NULL, {.routine = M_HandleItemToggles}, KITEM_SUPERRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Self-Propelled Bombs",	NULL, {.routine = M_HandleItemToggles}, KITEM_SPB, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Eggman Marks",			NULL, {.routine = M_HandleItemToggles}, KITEM_EGGMAN, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Toggle All",			NULL, {.routine = M_HandleItemToggles}, 0, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneakers",				NULL, {.routine = M_HandleItemToggles}, KITEM_SNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneakers x2",			NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALSNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Sneakers x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLESNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Rocket Sneakers",		NULL, {.routine = M_HandleItemToggles}, KITEM_ROCKETSNEAKER, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bananas",				NULL, {.routine = M_HandleItemToggles}, KITEM_BANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bananas x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEBANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bananas x10",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TENFOLDBANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Proximity Mines",		NULL, {.routine = M_HandleItemToggles}, KITEM_MINE, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinauts",				NULL, {.routine = M_HandleItemToggles}, KITEM_ORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinauts x3",			NULL, {.routine = M_HandleItemToggles}, KRITEM_TRIPLEORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Orbinauts x4",			NULL, {.routine = M_HandleItemToggles}, KRITEM_QUADORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Land Mines",			NULL, {.routine = M_HandleItemToggles}, KITEM_LANDMINE, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Jawz",					NULL, {.routine = M_HandleItemToggles}, KITEM_JAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Jawz x2",				NULL, {.routine = M_HandleItemToggles}, KRITEM_DUALJAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Ballhogs",				NULL, {.routine = M_HandleItemToggles}, KITEM_BALLHOG, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Drop Targets",			NULL, {.routine = M_HandleItemToggles}, KITEM_DROPTARGET, sfx_s258},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Lightning Shields",		NULL, {.routine = M_HandleItemToggles}, KITEM_LIGHTNINGSHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Bubble Shields",		NULL, {.routine = M_HandleItemToggles}, KITEM_BUBBLESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Flame Shields",			NULL, {.routine = M_HandleItemToggles}, KITEM_FLAMESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Hyudoros",				NULL, {.routine = M_HandleItemToggles}, KITEM_HYUDORO, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Invinciblity",			NULL, {.routine = M_HandleItemToggles}, KITEM_INVINCIBILITY, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Grow",					NULL, {.routine = M_HandleItemToggles}, KITEM_GROW, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Shrink",				NULL, {.routine = M_HandleItemToggles}, KITEM_SHRINK, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL, NULL, {.routine = M_HandleItemToggles}, 255, 0},

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Pogo Springs",		 	NULL, {.routine = M_HandleItemToggles}, KITEM_POGOSPRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "Kitchen Sinks",			NULL, {.routine = M_HandleItemToggles}, KITEM_KITCHENSINK, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL, NULL, {.routine = M_HandleItemToggles}, 255, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL, NULL, {.routine = M_HandleItemToggles}, 255, 0}
};

menu_t OPTIONS_GameplayItemsDef = {
	sizeof (OPTIONS_GameplayItems) / sizeof (menuitem_t),
	&OPTIONS_GameplayDef,
	0,
	OPTIONS_GameplayItems,
	14, 40,
	SKINCOLOR_SCARLET, 0,
	2, 5,
	M_DrawItemToggles,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

menuitem_t OPTIONS_Server[] =
{

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Name", "Change the name of your server.",
		NULL, {.cvar = &cv_servername}, 0, 0},

	{IT_STRING | IT_CVAR, "Intermission", "Set how long to stay on the result screen.",
		NULL, {.cvar = &cv_inttime}, 0, 0},

	{IT_STRING | IT_CVAR, "Map Progression", "Set how the next map is chosen.",
		NULL, {.cvar = &cv_advancemap}, 0, 0},

	{IT_STRING | IT_CVAR, "Vote Timer", "Set how long players have to vote.",
		NULL, {.cvar = &cv_votetime}, 0, 0},

	{IT_STRING | IT_CVAR, "Vote Mode Change", "Set how often voting proposes a different gamemode.",
		NULL, {.cvar = &cv_kartvoterulechanges}, 0, 0},

#ifndef NONET

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Maximum Players", "How many players can play at once.",
		NULL, {.cvar = &cv_maxplayers}, 0, 0},

	{IT_STRING | IT_CVAR, "Maximum Connections", "How many players & spectators can connect to the server.",
		NULL, {.cvar = &cv_maxconnections}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Joining", "Sets whether players can connect to your server.",
		NULL, {.cvar = &cv_allownewplayer}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Downloads", "Allows joiners to download missing files from you.",
		NULL, {.cvar = &cv_downloading}, 0, 0},

	{IT_STRING | IT_CVAR, "Pause Permissions", "Sets who can pause the game.",
		NULL, {.cvar = &cv_pause}, 0, 0},

	{IT_STRING | IT_CVAR, "Mute Chat", "Prevents non-admins from sending chat messages.",
		NULL, {.cvar = &cv_mute}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Advanced...", "Advanced options. Be careful when messing with these!",
		NULL, {.submenu = &OPTIONS_ServerAdvancedDef}, 0, 0},

#endif
};

menu_t OPTIONS_ServerDef = {
	sizeof (OPTIONS_Server) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Server,
	48, 70,	// This menu here is slightly higher because there's a lot of options...
	SKINCOLOR_VIOLET, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

#ifndef NONET
menuitem_t OPTIONS_ServerAdvanced[] =
{

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Browser Address", "Default is \'https://ms.kartkrew.org/ms/api\'",
		NULL, {.cvar = &cv_masterserver}, 0, 0},

	{IT_STRING | IT_CVAR, "Resynch. Attempts", "How many times to attempt sending data to desynchronized players.",
		NULL, {.cvar = &cv_resynchattempts}, 0, 0},

	{IT_STRING | IT_CVAR, "Ping Limit (ms)", "Players above the ping limit will get kicked from the server.",
		NULL, {.cvar = &cv_maxping}, 0, 0},

	{IT_STRING | IT_CVAR, "Ping Timeout (s)", "Players must be above the ping limit for this long before being kicked.",
		NULL, {.cvar = &cv_pingtimeout}, 0, 0},

	{IT_STRING | IT_CVAR, "Connection Timeout (tics)", "Players not giving any netowrk activity for this long are kicked.",
		NULL, {.cvar = &cv_nettimeout}, 0, 0},

	{IT_STRING | IT_CVAR, "Join Timeout (tics)", "Players taking too long to join are kicked.",
		NULL, {.cvar = &cv_jointimeout}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Max File Transfer", "Maximum size of the files that can be downloaded from joining clients. (KB)",
		NULL, {.cvar = &cv_maxsend}, 0, 0},

	{IT_STRING | IT_CVAR, "File Transfer Speed", "File transfer packet rate. Larger values send more data.",
		NULL, {.cvar = &cv_downloadspeed}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Log Joiner IPs", "Shows the IP of connecting players.",
		NULL, {.cvar = &cv_showjoinaddress}, 0, 0},

	{IT_STRING | IT_CVAR, "Log Resynch", "Shows which players need resynchronization.",
		NULL, {.cvar = &cv_blamecfail}, 0, 0},

	{IT_STRING | IT_CVAR, "Log Transfers", "Shows when clients are downloading files from you.",
		NULL, {.cvar = &cv_noticedownload}, 0, 0},
};

menu_t OPTIONS_ServerAdvancedDef = {
	sizeof (OPTIONS_ServerAdvanced) / sizeof (menuitem_t),
	&OPTIONS_ServerDef,
	0,
	OPTIONS_ServerAdvanced,
	48, 70,	// This menu here is slightly higher because there's a lot of options...
	SKINCOLOR_VIOLET, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
#endif

// data options menu -- see dopt_e
menuitem_t OPTIONS_Data[] =
{

	{IT_STRING | IT_SUBMENU, "Screenshot Options...", "Set options relative to screenshot and GIF capture.",
		NULL, {.submenu = &OPTIONS_DataScreenshotDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Addon Options...", "Set options relative to the addons menu.",
		NULL, {.submenu = &OPTIONS_DataAddonDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Replay Options...", "Set options relative to replays.",
		NULL, {.submenu = &OPTIONS_DataReplayDef}, 0, 0},

#ifdef HAVE_DISCORDRPC
	{IT_STRING | IT_SUBMENU, "Discord Options...", "Set options relative to Discord Rich Presence.",
		NULL, {.submenu = &OPTIONS_DataDiscordDef}, 0, 0},
#endif

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_SUBMENU, "\x85""Erase Data...", "Erase specific data. Be careful, what's deleted is gone forever!",
		NULL, {.submenu = &OPTIONS_DataEraseDef}, 0, 0},

};

menu_t OPTIONS_DataDef = {
	sizeof (OPTIONS_Data) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Data,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

menuitem_t OPTIONS_DataAddon[] =
{

	{IT_HEADER, "MENU", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Location", "Where to start searching addons from in the menu.",
		NULL, {.cvar = &cv_addons_option}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Custom Folder", "Specify which folder to start searching from if the location is set to custom.",
		NULL, {.cvar = &cv_addons_folder}, 24, 0},

	{IT_STRING | IT_CVAR, "Identify Addons via", "Set whether to consider the extension or contents of a file.",
		NULL, {.cvar = &cv_addons_md5}, 0, 0},

	{IT_STRING | IT_CVAR, "Show Unsupported Files", "Sets whether non-addon files should be shown.",
		NULL, {.cvar = &cv_addons_showall}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "SEARCH", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Matching", "Set where to check for the text pattern when looking up addons via name.",
		NULL, {.cvar = &cv_addons_search_type}, 0, 0},

	{IT_STRING | IT_CVAR, "Case Sensitivity", "Set whether to consider the case when searching for addons..",
		NULL, {.cvar = &cv_addons_search_case}, 0, 0},

};

menu_t OPTIONS_DataAddonDef = {
	sizeof (OPTIONS_DataAddon) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataAddon,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

menuitem_t OPTIONS_DataScreenshot[] =
{

	{IT_HEADER, "SCREENSHOTS (F8)", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Storage Location", "Sets where to store screenshots.",
		NULL, {.cvar = &cv_screenshot_option}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Custom Folder", "Specify which folder to save screenshots in.",
		NULL, {.cvar = &cv_screenshot_folder}, 24, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "GIF RECORDING (F9)", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Storage Location", "Sets where to store GIFs",
		NULL, {.cvar = &cv_movie_option}, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Custom Folder", "Specify which folder to save GIFs in.",
		NULL, {.cvar = &cv_movie_folder}, 24, 0},

};

menu_t OPTIONS_DataScreenshotDef = {
	sizeof (OPTIONS_DataScreenshot) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataScreenshot,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

menuitem_t OPTIONS_DataReplay[] =
{
	{IT_STRING | IT_CVAR, "Record Replays", "Select when to save replays.",
		NULL, {.cvar = &cv_recordmultiplayerdemos}, 0, 0},

	{IT_STRING | IT_CVAR, "Synch. Check Interval", "How often to check for synchronization while playing back a replay.",
		NULL, {.cvar = &cv_netdemosyncquality}, 0, 0},
};

menu_t OPTIONS_DataReplayDef = {
	sizeof (OPTIONS_DataReplay) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataReplay,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

#ifdef HAVE_DISCORDRPC
menuitem_t OPTIONS_DataDiscord[] =
{
	{IT_STRING | IT_CVAR, "Rich Presence", "Allow Discord to display game info on your status.",
		NULL, {.cvar = &cv_discordrp}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "RICH PRESENCE SETTINGS", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Streamer Mode", "Prevents the logging of some account information such as your tag in the console.",
		NULL, {.cvar = &cv_discordstreamer}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Ask to Join", "Allow other people to request joining your game from Discord.",
		NULL, {.cvar = &cv_discordasks}, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Invites", "Set who is allowed to generate Discord invites to your game.",
		NULL, {.cvar = &cv_discordinvites}, 0, 0},

};

menu_t OPTIONS_DataDiscordDef = {
	sizeof (OPTIONS_DataDiscord) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataDiscord,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
#endif


menuitem_t OPTIONS_DataErase[] =
{

	{IT_STRING | IT_CALL, "Erase Time Attack Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, 0, 0},

	{IT_STRING | IT_CALL, "Erase Unlockable Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "Erase Profile Data...", "Select a Profile to erase.",
		NULL, {.routine = M_CheckProfileData}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "\x85\x45rase all Data", "Be careful! What's deleted is gone forever!",
		NULL, {.routine = M_EraseData}, 0, 0},

};

menu_t OPTIONS_DataEraseDef = {
	sizeof (OPTIONS_DataErase) / sizeof (menuitem_t),
	&OPTIONS_DataDef,
	0,
	OPTIONS_DataErase,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

menuitem_t OPTIONS_DataProfileErase[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, {.routine = M_HandleProfileErase}, 0, 0},
};

menu_t OPTIONS_DataProfileEraseDef = {
	sizeof (OPTIONS_DataProfileErase) / sizeof (menuitem_t),
	&OPTIONS_DataEraseDef,
	0,
	OPTIONS_DataProfileErase,
	48, 80,
	SKINCOLOR_BLUEBERRY, 0,
	2, 5,
	M_DrawProfileErase,
	M_OptionsTick,
	NULL,
	NULL,
	NULL
};

// extras menu
menuitem_t EXTRAS_Main[] =
{

	{IT_STRING | IT_CALL, "Addons", "Add files to customize your experience.",
		NULL, {.routine = M_Addons}, 0, 0},

	{IT_STRING | IT_CALL, "Replay Hut", "Play the replays you've saved throughout your many races & battles!",
		NULL, {.routine = M_ReplayHut}, 0, 0},

	{IT_STRING | IT_CALL, "Statistics", "Look back on some of your greatest achievements such as your playtime and wins!",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_TRANSTEXT, "Extras Checklist", "View the requirement for some of the secret content you can unlock!",
		NULL, {NULL}, 0, 0},
};

// the extras menu essentially reuses the options menu stuff
menu_t EXTRAS_MainDef = {
	sizeof (EXTRAS_Main) / sizeof (menuitem_t),
	&MainDef,
	0,
	EXTRAS_Main,
	0, 0,
	0, 0,
	2, 5,
	M_DrawExtras,
	M_ExtrasTick,
	NULL,
	NULL,
	M_ExtrasInputs
};

// extras menu: replay hut
menuitem_t EXTRAS_ReplayHut[] =
{
	{IT_KEYHANDLER|IT_NOTHING, "", "",			// Dummy menuitem for the replay list
		NULL, {.routine = M_HandleReplayHutList}, 0, 0},

	{IT_NOTHING, "", "",						// Dummy for handling wrapping to the top of the menu..
		NULL, {NULL}, 0, 0},
};

menu_t EXTRAS_ReplayHutDef =
{
	sizeof (EXTRAS_ReplayHut)/sizeof (menuitem_t),
	&EXTRAS_MainDef,
	0,
	EXTRAS_ReplayHut,
	30, 80,
	0, 0,
	0, 0,
	M_DrawReplayHut,
	NULL,
	NULL,
	M_QuitReplayHut,
	NULL
};

menuitem_t EXTRAS_ReplayStart[] =
{
	{IT_CALL |IT_STRING,  "Load Addons and Watch", NULL,
		NULL, {.routine = M_HutStartReplay}, 0, 0},

	{IT_CALL |IT_STRING,  "Load Without Addons", NULL,
		NULL, {.routine = M_HutStartReplay}, 10, 0},

	{IT_CALL |IT_STRING,  "Watch Replay", NULL,
		NULL, {.routine = M_HutStartReplay}, 10, 0},

	{IT_SUBMENU |IT_STRING,  "Go Back", NULL,
		NULL, {.submenu = &EXTRAS_ReplayHutDef}, 30, 0},
};


menu_t EXTRAS_ReplayStartDef =
{
	sizeof (EXTRAS_ReplayStart)/sizeof (menuitem_t),
	&EXTRAS_ReplayHutDef,
	0,
	EXTRAS_ReplayStart,
	27, 80,
	0, 0,
	0, 0,
	M_DrawReplayStartMenu,
	NULL,
	NULL,
	NULL,
	NULL
};

// -------------------
// In-game/pause menus
// -------------------

// ESC pause menu
// Since there's no descriptions to each item, we'll use the descriptions as the names of the patches we want to draw for each option :)

menuitem_t PAUSE_Main[] =
{

	{IT_STRING | IT_CALL, "ADDONS", "M_ICOADD",
		NULL, {.routine = M_Addons}, 0, 0},

	{IT_STRING | IT_SUBMENU, "CHANGE MAP", "M_ICOMAP",
		NULL, {.submenu = &PAUSE_GamemodesDef}, 0, 0},

#ifdef HAVE_DISCORDRPC
	{IT_STRING | IT_CALL, "DISCORD REQUESTS", "M_ICODIS",
		NULL, {NULL}, 0, 0},
#endif

	{IT_STRING | IT_CALL, "RESUME GAME", "M_ICOUNP",
		NULL, {.routine = M_QuitPauseMenu}, 0, 0},

	{IT_STRING | IT_CALL, "SPECTATE", "M_ICOSPC",
		NULL, {.routine = M_ConfirmSpectate}, 0, 0},

	{IT_STRING | IT_CALL, "ENTER GAME", "M_ICOENT",
		NULL, {.routine = M_ConfirmEnterGame}, 0, 0},

	{IT_STRING | IT_CALL, "CANCEL JOIN", "M_ICOSPC",
		NULL, {.routine = M_ConfirmSpectate}, 0, 0},

	{IT_STRING | IT_SUBMENU, "JOIN OR SPECTATE", "M_ICOENT",
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CALL, "PLAYER SETUP", "M_ICOCHR",
		NULL, {.routine = M_CharacterSelect}, 0, 0},

	{IT_STRING | IT_CALL, "OPTIONS", "M_ICOOPT",
		NULL, {.routine = M_InitOptions}, 0, 0},

	{IT_STRING | IT_CALL, "EXIT GAME", "M_ICOEXT",
		NULL, {.routine = M_EndGame}, 0, 0},
};

menu_t PAUSE_MainDef = {
	sizeof (PAUSE_Main) / sizeof (menuitem_t),
	NULL,
	0,
	PAUSE_Main,
	0, 0,
	0, 0,
	1, 10,	// For transition with some menus!
	M_DrawPause,
	M_PauseTick,
	NULL,
	NULL,
	M_PauseInputs
};

// PAUSE : Map switching gametype selection (In case you want to pick from battle / race...)
menuitem_t PAUSE_GamemodesMenu[] =
{
	{IT_STRING | IT_CALL, "Race", "Select which gamemode to choose a new map from.",
		NULL, {.routine = M_LevelSelectInit}, 0, GT_RACE},

	{IT_STRING | IT_CALL, "Battle", "Select which gamemode to choose a new map from.",
		NULL, {.routine = M_LevelSelectInit}, 0, GT_BATTLE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, {.routine = M_GoBack}, 0, 0},
};

menu_t PAUSE_GamemodesDef = KARTGAMEMODEMENU(PAUSE_GamemodesMenu, &PAUSE_MainDef);

// Replay popup menu
menuitem_t PAUSE_PlaybackMenu[] =
{
	{IT_CALL   | IT_STRING, "Hide Menu (Esc)",			NULL, "M_PHIDE",	{.routine = M_SelectableClearMenus},	  0, 0},

	{IT_CALL   | IT_STRING, "Rewind ([)",				NULL, "M_PREW",		{.routine = M_PlaybackRewind},			 20, 0},
	{IT_CALL   | IT_STRING, "Pause (\\)",				NULL, "M_PPAUSE",	{.routine = M_PlaybackPause},			 36, 0},
	{IT_CALL   | IT_STRING, "Fast-Forward (])",			NULL, "M_PFFWD",	{.routine = M_PlaybackFastForward},		 52, 0},
	{IT_CALL   | IT_STRING, "Backup Frame ([)",			NULL, "M_PSTEPB",	{.routine = M_PlaybackRewind},			 20, 0},
	{IT_CALL   | IT_STRING, "Resume",					NULL, "M_PRESUM",	{.routine = M_PlaybackPause},			 36, 0},
	{IT_CALL   | IT_STRING, "Advance Frame (])",		NULL, "M_PFADV",	{.routine = M_PlaybackAdvance},			 52, 0},

	{IT_ARROWS | IT_STRING, "View Count (- and =)",		NULL, "M_PVIEWS",	{.routine = M_PlaybackSetViews},		 72, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint (1)",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		 88, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 2 (2)",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		104, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 3 (3)",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		120, 0},
	{IT_ARROWS | IT_STRING, "Viewpoint 4 (4)",			NULL, "M_PNVIEW",	{.routine = M_PlaybackAdjustView},		136, 0},

	{IT_CALL   | IT_STRING, "Toggle Free Camera (')",	NULL, "M_PVIEWS",	{.routine = M_PlaybackToggleFreecam},	156, 0},
	{IT_CALL   | IT_STRING, "Stop Playback",			NULL, "M_PEXIT",	{.routine = M_PlaybackQuit},			172, 0},
};

menu_t PAUSE_PlaybackMenuDef = {
	sizeof (PAUSE_PlaybackMenu) / sizeof (menuitem_t),
	NULL,
	0,
	PAUSE_PlaybackMenu,
	BASEVIDWIDTH/2 - 88, 2,
	0, 0,
	0, 0,
	M_DrawPlaybackMenu,
	NULL,
	NULL,
	NULL,
	NULL
};


// Other misc menus:

// Manual
menuitem_t MISC_Manual[] = {
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL00", NULL, NULL, {.routine = M_HandleImageDef}, 0, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL01", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL02", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL03", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL04", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL05", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL06", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL07", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL08", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL09", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL10", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL11", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL12", NULL, NULL, {.routine = M_HandleImageDef}, 1, 0},
		{IT_NOTHING | IT_KEYHANDLER, "MANUAL99", NULL, NULL, {.routine = M_HandleImageDef}, 0, 0},
};

menu_t MISC_ManualDef = IMAGEDEF(MISC_Manual);

// Addons menu!
menuitem_t MISC_AddonsMenu[] =
{
	{IT_STRING | IT_CVAR | IT_CV_STRING, NULL, NULL,
		NULL, {.cvar = &cv_dummyaddonsearch}, 0, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL,
		NULL, {.routine = M_HandleAddons}, 0, 0},     // dummy menuitem for the control func
};

menu_t MISC_AddonsDef = {
	sizeof (MISC_AddonsMenu)/sizeof (menuitem_t),
	NULL,
	0,
	MISC_AddonsMenu,
	50, 28,
	0, 0,
	0, 0,
	M_DrawAddons,
	M_AddonsRefresh,
	NULL,
	NULL,
	NULL
};
