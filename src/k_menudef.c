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

	{IT_STRING, "Options",
		"Configure your controls, settings, and preferences.", NULL,
		M_InitOptions, 0, 0},

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
	{IT_NOTHING, NULL, NULL, NULL, NULL, 0, 0},
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
	M_CharacterSelectQuit,
	M_CharacterSelectHandler
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
	NULL,
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
	NULL,
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

	{IT_STRING | IT_CALL, "Server Browser", "Search for game servers to play in.",
		NULL, M_MPRoomSelectInit, 0, 0},

	{IT_STRING | IT_CALL, "Join by IP", "Join an online game by its IP address.",
		NULL, M_MPJoinIPInit, 0, 0},
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
	NULL,
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
	M_MPResetOpts,
	NULL
};

// MULTIPLAYER JOIN BY IP
menuitem_t PLAY_MP_JoinIP[] =
{
	//{IT_NOTHING | IT_KEYHANDLER, NULL, NULL, NULL, M_MPOptSelect, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_STRING, "IP: ", "Type the IPv4 address of the server then press enter to attempt connection.",
		NULL, &cv_dummyip, 0, 0},

	{IT_STRING | IT_SPACE, "LAST IPs JOINED:", "Kanade best waifu :)",
		NULL, NULL, 0, 0},

	{IT_STRING, "servip1", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, NULL, 0, 0},

	{IT_STRING, "servip2", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, NULL, 0, 0},

	{IT_STRING, "servip3", "The last 3 IPs you've succesfully joined are displayed here.",
		NULL, NULL, 0, 0},

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
	M_MPResetOpts,
	M_JoinIPInputs
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
	NULL,
	NULL
};

// options menu
menuitem_t OPTIONS_Main[] =
{

	{IT_STRING | IT_SUBMENU, "Control Setup", "Remap keys & buttons to your likings.",
		NULL, NULL, 0, 0},

	{IT_STRING | IT_SUBMENU, "Video Options", "Change video settings such as the resolution.",
		NULL, &OPTIONS_VideoDef, 0, 0},

	{IT_STRING | IT_SUBMENU, "Sound Options", "Adjust various sound settings such as the volume.",
		NULL, &OPTIONS_SoundDef, 0, 0},

	{IT_STRING | IT_SUBMENU, "HUD Options", "Options related to the Heads-Up Display.",
		NULL, &OPTIONS_HUDDef, 0, 0},

	{IT_STRING | IT_SUBMENU, "Gameplay Options", "Change various game related options",
		NULL, &OPTIONS_GameplayDef, 0, 0},

	{IT_STRING | IT_SUBMENU, "Server Options", "Change various specific options for your game server.",
		NULL, &OPTIONS_ServerDef, 0, 0},

	{IT_STRING | IT_SUBMENU, "Data Options", "Miscellaneous data options such as the screenshot format.",
		NULL, NULL, 0, 0},

	{IT_STRING | IT_SUBMENU, "Tricks & Secrets", "Those who bother reading a game manual always get the edge over those who don't!",
		NULL, NULL, 0, 0},
};

menu_t OPTIONS_MainDef = {
	sizeof (OPTIONS_Main) / sizeof (menuitem_t),
	&MainDef,
	0,
	OPTIONS_Main,
	0, 0,
	2, 10,
	M_DrawOptions,
	M_OptionsTick,
	NULL,
	M_OptionsInputs
};

// video options menu...
// options menu
menuitem_t OPTIONS_Video[] =
{

	{IT_STRING | IT_CALL, "Set Resolution...", "Change the screen resolution for the game.",
		NULL, M_VideoModeMenu, 0, 0},

// A check to see if you're not running on a fucking antique potato powered stone i guess???????

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	{IT_STRING | IT_CVAR, "Fullscreen", "Set whether you want to use fullscreen or windowed mode.",
		NULL, &cv_fullscreen, 0, 0},
#endif

	{IT_NOTHING|IT_SPACE, NULL, "Kanade best waifu! I promise!",
		NULL, NULL, 0, 0},

	// Everytime I see a screenshot at max gamma I die inside
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Gamma", "Adjusts the overall brightness of the game.",
		NULL, &cv_globalgamma, 0, 0},

	{IT_STRING | IT_CVAR, "Vertical Sync", "Locks the framerate to your monitor's refresh rate.",
		NULL, &cv_vidwait, 0, 0},

	{IT_STRING | IT_CVAR, "Enable Skyboxes", "Turning this off will improve performance at the detriment of visuals for many maps.",
		NULL, &cv_skybox, 0, 0},

	{IT_STRING | IT_CVAR, "Draw Distance", "How far objects can be drawn. Lower values may improve performance at the cost of visibility.",
		NULL, &cv_drawdist, 0, 0},

	{IT_STRING | IT_CVAR, "Weather Draw Distance", "Affects how far weather visuals can be drawn. Lower values improve performance.",
		NULL, &cv_drawdist_precip, 0, 0},

	{IT_STRING | IT_CVAR, "Show FPS", "Displays the game framerate at the lower right corner of the screen.",
		NULL, &cv_ticrate, 0, 0},

	{IT_NOTHING|IT_SPACE, NULL, "Kanade best waifu! I promise!",
		NULL, NULL, 0, 0},

#ifdef HWRENDER
	{IT_STRING | IT_SUBMENU, "Hardware Options...", "For usage and configuration of the OpenGL renderer.",
		NULL, &OPTIONS_VideoOGLDef, 0, 0},
#endif

};

menu_t OPTIONS_VideoDef = {
	sizeof (OPTIONS_Video) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Video,
	32, 80,
	2, 10,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
};

menuitem_t OPTIONS_VideoModes[] = {

	{IT_KEYHANDLER | IT_NOTHING, NULL, "Select a resolution.",
		NULL, M_HandleVideoModes, 0, 0},     // dummy menuitem for the control func

};

menu_t OPTIONS_VideoModesDef = {
	sizeof (OPTIONS_VideoModes) / sizeof (menuitem_t),
	&OPTIONS_VideoDef,
	0,
	OPTIONS_VideoModes,
	48, 80,
	2, 10,
	M_DrawVideoModes,
	M_OptionsTick,
	NULL,
	NULL,
};

#ifdef HWRENDER
menuitem_t OPTIONS_VideoOGL[] =
{

	{IT_STRING | IT_CVAR, "Renderer", "Change renderers between Software and OpenGL",
		NULL, &cv_renderer, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_SPACE | IT_NOTHING | IT_STRING, "OPTIONS BELOW ARE OPENGL ONLY!", "Watch people get confused anyway!!",
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "3D Models", "Use 3D models instead of sprites when applicable.",
		NULL, &cv_glmodels, 0, 0},

	{IT_STRING | IT_CVAR, "Shaders", "Use GLSL Shaders. Turning them off increases performance at the expanse of visual quality.",
		NULL, &cv_glshaders, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Texture Quality", "Texture depth. Higher values are recommended.",
		NULL, &cv_scr_depth, 0, 0},

	{IT_STRING | IT_CVAR, "Texture Filter", "Texture Filter. Nearest is recommended.",
		NULL, &cv_glfiltermode, 0, 0},

	{IT_STRING | IT_CVAR, "Anisotropic", "Lower values will improve performance at a minor quality loss.",
		NULL, &cv_glanisotropicmode, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Wall Contrast Style", "Allows faking or not Software wall colour contrast.",
		NULL, &cv_glfakecontrast, 0, 0},

	{IT_STRING | IT_CVAR, "Sprite Billboarding", "Adjusts sprites when viewed from above or below to not make them appear flat.",
		NULL, &cv_glspritebillboarding, 0, 0},

	{IT_STRING | IT_CVAR, "Software Perspective", "Emulates Software shearing when looking up or down. Not recommended.",
		NULL, &cv_glshearing, 0, 0},
};

menu_t OPTIONS_VideoOGLDef = {
	sizeof (OPTIONS_VideoOGL) / sizeof (menuitem_t),
	&OPTIONS_VideoDef,
	0,
	OPTIONS_VideoOGL,
	32, 80,
	2, 10,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
};
#endif

menuitem_t OPTIONS_Sound[] =
{

	{IT_STRING | IT_CVAR, "SFX", "Enable or disable sound effect playback.",
		NULL, &cv_gamesounds, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "SFX Volume", "Adjust the volume of sound effects.",
		NULL, &cv_soundvolume, 0, 0},

	{IT_STRING | IT_CVAR, "Music", "Enable or disable music playback.",
		NULL, &cv_gamedigimusic, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Music Volume", "Adjust the volume of music playback.",
		NULL, &cv_digmusicvolume, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Reverse L/R Channels", "Reverse left & right channels for Stereo playback.",
		NULL, &stereoreverse, 0, 0},

	{IT_STRING | IT_CVAR, "Surround", "Enables or disable Surround sound playback.",
		NULL, &surround, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Chat Notifications", "Set when to play notification sounds when chat messages are received.",
		NULL, &cv_chatnotifications, 0, 0},

	{IT_STRING | IT_CVAR, "Character Voices", "Set how often to play character voices in game.",
		NULL, &cv_kartvoices, 0, 0},

	{IT_STRING | IT_CVAR, "Powerup Warning", "Set how to warn you from other player's powerups such as Invincibility.",
		NULL, &cv_kartinvinsfx, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Play Music While Unfocused", "Keeps playing music even if the game is not the active window.",
		NULL, &cv_playmusicifunfocused, 0, 0},

	{IT_STRING | IT_CVAR, "Play SFX While Unfocused", "Keeps playing sound effects even if the game is not the active window.",
		NULL, &cv_playsoundifunfocused, 0, 0},

	// @TODO: Sound test (there's currently no space on this menu, might be better to throw it in extras?)
};

menu_t OPTIONS_SoundDef = {
	sizeof (OPTIONS_Sound) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Sound,
	48, 80,
	2, 10,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
};

menuitem_t OPTIONS_HUD[] =
{

	{IT_STRING | IT_CVAR, "Show HUD (F3)", "Toggles HUD display. Great for taking screenshots!",
		NULL, &cv_showhud, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "HUD Opacity", "Non opaque values may have performance impacts in software mode.",
		NULL, &cv_translucenthud, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Minimap Opacity", "Changes the opacity of the minimap.",
		NULL, &cv_kartminimap, 0, 0},

	{IT_STRING | IT_CVAR, "Speedometer", "Choose to what speed unit to display or toggle off the speedometer.",
		NULL, &cv_kartspeedometer, 0, 0},

	{IT_STRING | IT_CVAR, "Display \"CHECK\"", "Displays an icon when a player is tailing you.",
		NULL, &cv_kartcheck, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Console Text Size", "Size of the text within the console.",
		NULL, &cv_constextsize, 0, 0},

	// we spell words properly here.
	{IT_STRING | IT_CVAR, "Console Tint", "Change the background colour of the console.",
		NULL, &cons_backcolor, 0, 0},

	{IT_STRING | IT_CVAR, "Show \"FOCUS LOST\"", "Displays \"FOCUS LOST\" when the game window isn't the active window.",
		NULL, &cv_showfocuslost, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_SUBMENU, "Online HUD Options...", "HUD options related to the online chat box and other features.",
		NULL, &OPTIONS_HUDOnlineDef, 0, 0},
};

menu_t OPTIONS_HUDDef = {
	sizeof (OPTIONS_HUD) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_HUD,
	48, 80,
	2, 10,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
};

menuitem_t OPTIONS_HUDOnline[] =
{

	{IT_STRING | IT_CVAR, "Chat Mode", "Choose whether to display chat in its own window or the console.",
		NULL, &cv_consolechat, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Chat Box Tint", "Changes the background colour of the chat box.",
		NULL, &cv_chatbacktint, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Chat Box Width", "Change the width of the Chat Box",
		NULL, &cv_chatwidth, 0, 0},

	{IT_STRING | IT_CVAR | IT_CV_SLIDER, "Chat Box Height", "Change the height of the Chat Box",
		NULL, &cv_chatheight, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Message Fadeout Time", "How long chat messages stay displayed with the chat closed.",
		NULL, &cv_chattime, 0, 0},

	{IT_STRING | IT_CVAR, "Spam Protection", "Prevents too many message from a single player from being displayed.",
		NULL, &cv_chatspamprotection, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Local Ping Display", "In netgames, displays your ping at the lower right corner of the screen.",
		NULL, &cv_showping, 0, 0},

};

menu_t OPTIONS_HUDOnlineDef = {
	sizeof (OPTIONS_HUDOnline) / sizeof (menuitem_t),
	&OPTIONS_HUDDef,
	0,
	OPTIONS_HUDOnline,
	48, 80,
	2, 10,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
};


menuitem_t OPTIONS_Gameplay[] =
{

	{IT_STRING | IT_CVAR, "Game Speed", "Change Game Speed for the next map.",
		NULL, &cv_kartspeed, 0, 0},

	{IT_STRING | IT_CVAR, "Base Lap Count", "Change how many laps must be completed per race.",
		NULL, &cv_kartspeed, 0, 0},

	{IT_STRING | IT_CVAR, "Frantic Items", "Make item odds crazier with more powerful items!",
		NULL, &cv_kartfrantic, 0, 0},

	{IT_STRING | IT_CVAR, "Encore Mode", "Forces Encore Mode on for the next map.",
		NULL, &cv_kartencore, 0, 0},

	{IT_STRING | IT_CVAR, "Exit Countdown", "How long players have to finish after 1st place finishes.",
		NULL, &cv_countdowntime, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Time Limit", "Change the time limit for Battle rounds.",
		NULL, &cv_timelimit, 0, 0},

	{IT_STRING | IT_CVAR, "Starting Bumpers", "Change how many bumpers player start with in Battle.",
		NULL, &cv_kartbumpers, 0, 0},

	{IT_STRING | IT_CVAR, "Karma Comeback", "Enable Karma Comeback in Battle mode.",
		NULL, &cv_kartcomeback, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_SUBMENU, "Random Item Toggles...", "Change which items to enable for your games.",
		NULL, &OPTIONS_GameplayItemsDef, 0, 0},

};

menu_t OPTIONS_GameplayDef = {
	sizeof (OPTIONS_Gameplay) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Gameplay,
	48, 80,
	2, 10,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
};

menuitem_t OPTIONS_GameplayItems[] =
{
	// Mostly handled by the drawing function.
	{IT_KEYHANDLER | IT_NOTHING, "Sneakers",				NULL, NULL, M_HandleItemToggles, KITEM_SNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Sneakers x3",				NULL, NULL, M_HandleItemToggles, KRITEM_TRIPLESNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Toggle All",				NULL, NULL, M_HandleItemToggles, 0, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Rocket Sneakers",			NULL, NULL, M_HandleItemToggles, KITEM_ROCKETSNEAKER, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Bananas",					NULL, NULL, M_HandleItemToggles, KITEM_BANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Bananas x3",				NULL, NULL, M_HandleItemToggles, KRITEM_TRIPLEBANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Bananas x10",				NULL, NULL, M_HandleItemToggles, KRITEM_TENFOLDBANANA, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Eggman Monitors",			NULL, NULL, M_HandleItemToggles, KITEM_EGGMAN, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Orbinauts",				NULL, NULL, M_HandleItemToggles, KITEM_ORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Orbinauts x3",			NULL, NULL, M_HandleItemToggles, KRITEM_TRIPLEORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Orbinauts x4",			NULL, NULL, M_HandleItemToggles, KRITEM_QUADORBINAUT, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Mines",					NULL, NULL, M_HandleItemToggles, KITEM_MINE, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Jawz",					NULL, NULL, M_HandleItemToggles, KITEM_JAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Jawz x2",					NULL, NULL, M_HandleItemToggles, KRITEM_DUALJAWZ, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Ballhogs",				NULL, NULL, M_HandleItemToggles, KITEM_BALLHOG, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Self-Propelled Bombs",	NULL, NULL, M_HandleItemToggles, KITEM_SPB, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Invinciblity",			NULL, NULL, M_HandleItemToggles, KITEM_INVINCIBILITY, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Grow",					NULL, NULL, M_HandleItemToggles, KITEM_GROW, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Shrink",					NULL, NULL, M_HandleItemToggles, KITEM_SHRINK, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Thunder Shields",			NULL, NULL, M_HandleItemToggles, KITEM_THUNDERSHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Bubble Shields",			NULL, NULL, M_HandleItemToggles, KITEM_BUBBLESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Flame Shields",			NULL, NULL, M_HandleItemToggles, KITEM_FLAMESHIELD, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Hyudoros",				NULL, NULL, M_HandleItemToggles, KITEM_HYUDORO, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Pogo Springs",		 	NULL, NULL, M_HandleItemToggles, KITEM_POGOSPRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Super Rings",				NULL, NULL, M_HandleItemToggles, KITEM_SUPERRING, 0},
	{IT_KEYHANDLER | IT_NOTHING, "Kitchen Sinks",			NULL, NULL, M_HandleItemToggles, KITEM_KITCHENSINK, 0},
};

menu_t OPTIONS_GameplayItemsDef = {
	sizeof (OPTIONS_GameplayItems) / sizeof (menuitem_t),
	&OPTIONS_GameplayDef,
	0,
	OPTIONS_GameplayItems,
	0, 75,
	2, 10,
	M_DrawItemToggles,
	M_OptionsTick,
	NULL,
	NULL,
};

menuitem_t OPTIONS_Server[] =
{

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Name", "Change the name of your server.",
		NULL, &cv_servername, 0, 0},

	{IT_STRING | IT_CVAR, "Intermission", "Set how long to stay on the result screen.",
		NULL, &cv_inttime, 0, 0},

	{IT_STRING | IT_CVAR, "Map Progression", "Set how the next map is chosen.",
		NULL, &cv_advancemap, 0, 0},

	{IT_STRING | IT_CVAR, "Vote Timer", "Set how long players have to vote.",
		NULL, &cv_votetime, 0, 0},

	{IT_STRING | IT_CVAR, "Vote Mode Change", "Set how often voting proposes a different gamemode.",
		NULL, &cv_kartvoterulechanges, 0, 0},

#ifndef NONET

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Ingame Max. Players", "How many players can play at once. 0 Allows everyone who joins.",
		NULL, &cv_ingamecap, 0, 0},

	{IT_STRING | IT_CVAR, "Server Max. Players", "How many players can connect to the server.",
		NULL, &cv_maxplayers, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Joining", "Sets whether players can connect to your server.",
		NULL, &cv_allownewplayer, 0, 0},

	{IT_STRING | IT_CVAR, "Allow Downloads", "Allows joiners to download missing files from you.",
		NULL, &cv_downloading, 0, 0},

	{IT_STRING | IT_CVAR, "Pause Permissions", "Sets who can pause the game.",
		NULL, &cv_pause, 0, 0},

	{IT_STRING | IT_CVAR, "Mute Chat", "Prevents non-admins from sending chat messages.",
		NULL, &cv_mute, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_SUBMENU, "Advanced...", "Advanced options. Be careful when messing with these!",
		NULL, &OPTIONS_ServerAdvancedDef, 0, 0},

#endif
};

menu_t OPTIONS_ServerDef = {
	sizeof (OPTIONS_Server) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Server,
	48, 70,	// This menu here is slightly higher because there's a lot of options...
	2, 10,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
};

#ifndef NONET
menuitem_t OPTIONS_ServerAdvanced[] =
{

	{IT_STRING | IT_CVAR | IT_CV_STRING, "Server Browser Address", "Default is \'https://ms.kartkew.org/ms/api\'",
		NULL, &cv_masterserver, 0, 0},

	{IT_STRING | IT_CVAR, "Resynch. Attempts", "How many times to attempt sending data to desynchronized players.",
		NULL, &cv_resynchattempts, 0, 0},

	{IT_STRING | IT_CVAR, "Ping Limit (ms)", "Players above the ping limit will get kicked from the server.",
		NULL, &cv_maxping, 0, 0},

	{IT_STRING | IT_CVAR, "Ping Timeout (s)", "Players must be above the ping limit for this long before being kicked.",
		NULL, &cv_pingtimeout, 0, 0},

	{IT_STRING | IT_CVAR, "Connection Timeout (tics)", "Players not giving any netowrk activity for this long are kicked.",
		NULL, &cv_nettimeout, 0, 0},

	{IT_STRING | IT_CVAR, "Join Timeout (tics)", "Players taking too long to join are kicked.",
		NULL, &cv_jointimeout, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Max File Transfer", "Maximum size of the files that can be downloaded from joining clients. (KB)",
		NULL, &cv_maxsend, 0, 0},

	{IT_STRING | IT_CVAR, "File Transfer Speed", "File transfer packet rate. Larger values send more data.",
		NULL, &cv_downloadspeed, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CVAR, "Log Joiner IPs", "Shows the IP of connecting players.",
		NULL, &cv_showjoinaddress, 0, 0},

	{IT_STRING | IT_CVAR, "Log Resynch", "Shows which players need resynchronization.",
		NULL, &cv_blamecfail, 0, 0},

	{IT_STRING | IT_CVAR, "Log Transfers", "Shows when clients are downloading files from you.",
		NULL, &cv_noticedownload, 0, 0},
};

menu_t OPTIONS_ServerAdvancedDef = {
	sizeof (OPTIONS_ServerAdvanced) / sizeof (menuitem_t),
	&OPTIONS_ServerDef,
	0,
	OPTIONS_ServerAdvanced,
	48, 70,	// This menu here is slightly higher because there's a lot of options...
	2, 10,
	M_DrawGenericOptions,
	M_OptionsTick,
	NULL,
	NULL,
};
#endif

// -------------------
// In-game/pause menus
// -------------------

// ESC pause menu
// Since there's no descriptions to each item, we'll use the descriptions as the names of the patches we want to draw for each option :)

menuitem_t PAUSE_Main[] =
{

	{IT_STRING | IT_CALL, "ADDONS", "M_ICOADD",
		NULL, M_Addons, 0, 0},

	{IT_STRING | IT_SUBMENU, "CHANGE MAP", "M_ICOMAP",
		NULL, &PAUSE_GamemodesDef, 0, 0},

#ifdef HAVE_DISCORDRPC
	{IT_STRING | IT_CALL, "DISCORD REQUESTS", "M_ICODIS",
		NULL, NULL, 0, 0},
#endif

	{IT_STRING | IT_CALL, "RESUME GAME", "M_ICOUNP",
		NULL, M_QuitPauseMenu, 0, 0},

	{IT_STRING | IT_CALL, "SPECTATE", "M_ICOSPC",
		NULL, M_ConfirmSpectate, 0, 0},

	{IT_STRING | IT_CALL, "ENTER GAME", "M_ICOENT",
		NULL, M_ConfirmEnterGame, 0, 0},

	{IT_STRING | IT_CALL, "CANCEL JOIN", "M_ICOSPC",
		NULL, M_ConfirmSpectate, 0, 0},

	{IT_STRING | IT_SUBMENU, "JOIN OR SPECTATE", "M_ICOENT",
		NULL, NULL, 0, 0},

	{IT_STRING | IT_CALL, "PLAYER SETUP", "M_ICOCHR",
		NULL, M_CharacterSelectInit, 0, 0},

	{IT_STRING | IT_CALL, "OPTIONS", "M_ICOOPT",
		NULL, M_InitOptions, 0, 0},

	{IT_STRING | IT_CALL, "EXIT GAME", "M_ICOEXT",
		NULL, M_EndGame, 0, 0},
};

menu_t PAUSE_MainDef = {
	sizeof (PAUSE_Main) / sizeof (menuitem_t),
	NULL,
	0,
	PAUSE_Main,
	0, 0,
	1, 10,	// For transition with some menus!
	M_DrawPause,
	M_PauseTick,
	NULL,
	M_PauseInputs
};

// PAUSE : Map switching gametype selection (In case you want to pick from battle / race...)
menuitem_t PAUSE_GamemodesMenu[] =
{
	{IT_STRING | IT_CALL, "Race", "Select which gamemode to choose a new map from.",
		NULL, M_LevelSelectInit, 0, GT_RACE},

	{IT_STRING | IT_CALL, "Battle", "Select which gamemode to choose a new map from.",
		NULL, M_LevelSelectInit, 0, GT_BATTLE},

	{IT_STRING | IT_CALL, "Back", NULL, NULL, M_GoBack, 0, 0},
};

menu_t PAUSE_GamemodesDef = KARTGAMEMODEMENU(PAUSE_GamemodesMenu, &PAUSE_MainDef);

// Replay popup menu
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
	NULL,
	NULL
};


// Other misc menus:

// Addons menu! (Just a straight port for now)
menuitem_t MISC_AddonsMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, NULL,
		NULL, M_HandleAddons, 0, 0},     // dummy menuitem for the control func
};

menu_t MISC_AddonsDef = {
	sizeof (MISC_AddonsMenu)/sizeof (menuitem_t),
	NULL,
	0,
	MISC_AddonsMenu,
	50, 28,
	0, 0,
	M_DrawAddons,
	NULL,
	NULL,
	NULL
};
