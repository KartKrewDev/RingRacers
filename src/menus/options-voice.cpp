// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/options-voice.cpp
/// \brief Voice Options

#include "../m_easing.h"
#include "../k_menu.h"
#include "../s_sound.h"	// sounds consvars
#include "../v_video.h"

menuitem_t OPTIONS_Voice[] =
{
	{IT_STRING | IT_CVAR, "Mute Self", "Whether your voice is transmitted or not.",
		NULL, srb2::itemaction(&cv_voice_selfmute), 0, 0 },

	{IT_STRING | IT_CVAR, "Deafen Self", "Choose to opt-in to voice chat at all, for yourself.",
		NULL, srb2::itemaction(&cv_voice_selfdeafen), 0, 0},

	{IT_STRING | IT_CVAR, "Input Mode", "When to transmit your own voice.",
		NULL, srb2::itemaction(&cv_voice_mode), 0, 0},

	{IT_STRING | IT_CVAR, "Input Amplifier", "Amplify your voice, in decibels. Negative values are quieter.",
		NULL, srb2::itemaction(&cv_voice_inputamp), 0, 0},

	{IT_STRING | IT_CVAR, "Input Noise Suppression", "Suppress background noise from your voice.",
		NULL, srb2::itemaction(&cv_voice_denoise), 0, 0},

	{IT_STRING | IT_CVAR, "Input Sensitivity", "Voice higher than this threshold will transmit, in decibels.",
		NULL, srb2::itemaction(&cv_voice_activationthreshold), 0, 0 },

	{IT_STRING | IT_CVAR, "Voice Loopback", "Play your own voice back simultaneously.",
		NULL, srb2::itemaction(&cv_voice_loopback), 0, 0 },

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "Server Voice Options...",  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Server Voice Chat", "All voice chat will be enabled on your server.",
		NULL, srb2::itemaction(&cv_voice_allowservervoice), 0, 0 },

	{IT_STRING | IT_CVAR, "Proximity Effects", "Player voices will be adjusted relative to you.",
		NULL, srb2::itemaction(&cv_voice_proximity), 0, 0 },
};

static void draw_routine()
{
	M_DrawGenericOptions();

	int x = currentMenu->x - M_EaseWithTransition(Easing_Linear, 5 * 48);
	int y = currentMenu->y - 12;
	int range = 220;
	float last_peak = g_local_voice_last_peak * range;
	boolean detected = g_local_voice_detected;
	INT32 color = detected ? 65 : 23;

	V_DrawFill(x, y, range + 2, 10, 31);
	V_DrawFill(x + 1, y + 1, (int) last_peak, 8, color);
	if (!detected)
		V_DrawThinString(x+1, y+1, V_20TRANS, "Not transmitting...");
	else
		V_DrawThinString(x+1, y+1, V_20TRANS|V_GREENMAP, "Transmitting");
}

static void tick_routine()
{
	M_OptionsTick();
}

static boolean input_routine(INT32)
{
	return false;
}

static void init_routine(void)
{
	if (!netgame)
	{
		S_SoundInputSetEnabled(true);
	}
}

static boolean quit_routine(void)
{
	if (!netgame)
	{
		S_SoundInputSetEnabled(false);
	}

	return true;
}

menu_t OPTIONS_VoiceDef = {
	sizeof (OPTIONS_Voice) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_Voice,
	48, 80,
	SKINCOLOR_ULTRAMARINE, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	draw_routine,
	M_DrawOptionsCogs,
	tick_routine,
	init_routine,
	quit_routine,
	input_routine,
};
