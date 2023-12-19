/// \file  menus/options-sound.c
/// \brief Sound Options

#include "../k_menu.h"
#include "../s_sound.h"	// sounds consvars
#include "../g_game.h" // cv_chatnotifications

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
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};
