// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
// Copyright (C) 2023      by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <cmath>

#include "console.h"
#include "d_player.h"
#include "d_ticcmd.h"
#include "doomstat.h"
#include "doomtype.h"
#include "g_demo.h"
#include "g_game.h"
#include "g_input.h"
#include "g_state.h"
#include "g_party.h"
#include "hu_stuff.h"
#include "i_joy.h"
#include "i_system.h"
#include "k_bot.h"
#include "k_director.h"
#include "k_kart.h"
#include "k_menu.h"
#include "lua_hook.h"
#include "m_cheat.h"
#include "m_fixed.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_tick.h"
#include "tables.h"

namespace
{

struct joystickvector2_t
{
	INT32 xaxis;
	INT32 yaxis;
};

// Take a magnitude of two axes, and adjust it to take out the deadzone
// Will return a value between 0 and JOYAXISRANGE
INT32 G_BasicDeadZoneCalculation(INT32 magnitude, fixed_t deadZone)
{
	const INT32 jdeadzone = (JOYAXISRANGE * deadZone) / FRACUNIT;
	INT32 deadzoneAppliedValue = 0;
	INT32 adjustedMagnitude = std::abs(magnitude);

	if (jdeadzone >= JOYAXISRANGE && adjustedMagnitude >= JOYAXISRANGE) // If the deadzone and magnitude are both 100%...
		return JOYAXISRANGE; // ...return 100% input directly, to avoid dividing by 0
	else if (adjustedMagnitude > jdeadzone) // Otherwise, calculate how much the magnitude exceeds the deadzone
	{
		adjustedMagnitude = std::min(adjustedMagnitude, JOYAXISRANGE);

		adjustedMagnitude -= jdeadzone;

		deadzoneAppliedValue = (adjustedMagnitude * JOYAXISRANGE) / (JOYAXISRANGE - jdeadzone);
	}

	return deadzoneAppliedValue;
}

// Get the actual sensible radial value for a joystick axis when accounting for a deadzone
void G_HandleAxisDeadZone(UINT8 splitnum, joystickvector2_t *joystickvector)
{
	INT32 gamepadStyle = Joystick[splitnum].bGamepadStyle;
	fixed_t deadZone = cv_deadzone[splitnum].value;

	// When gamepadstyle is "true" the values are just -1, 0, or 1. This is done in the interface code.
	if (!gamepadStyle)
	{
		// Get the total magnitude of the 2 axes
		INT32 magnitude = (joystickvector->xaxis * joystickvector->xaxis) + (joystickvector->yaxis * joystickvector->yaxis);
		INT32 normalisedXAxis;
		INT32 normalisedYAxis;
		INT32 normalisedMagnitude;
		double dMagnitude = std::sqrt((double)magnitude);
		magnitude = (INT32)dMagnitude;

		// Get the normalised xy values from the magnitude
		normalisedXAxis = (joystickvector->xaxis * magnitude) / JOYAXISRANGE;
		normalisedYAxis = (joystickvector->yaxis * magnitude) / JOYAXISRANGE;

		// Apply the deadzone to the magnitude to give a correct value between 0 and JOYAXISRANGE
		normalisedMagnitude = G_BasicDeadZoneCalculation(magnitude, deadZone);

		// Apply the deadzone to the xy axes
		joystickvector->xaxis = (normalisedXAxis * normalisedMagnitude) / JOYAXISRANGE;
		joystickvector->yaxis = (normalisedYAxis * normalisedMagnitude) / JOYAXISRANGE;

		// Cap the values so they don't go above the correct maximum
		joystickvector->xaxis = std::min(joystickvector->xaxis, JOYAXISRANGE);
		joystickvector->xaxis = std::max(joystickvector->xaxis, -JOYAXISRANGE);
		joystickvector->yaxis = std::min(joystickvector->yaxis, JOYAXISRANGE);
		joystickvector->yaxis = std::max(joystickvector->yaxis, -JOYAXISRANGE);
	}
}

// Turning was removed from G_BuildTiccmd to prevent easy client hacking.
// This brings back the camera prediction that was lost.
void G_DoAnglePrediction(ticcmd_t *cmd, INT32 realtics, UINT8 ssplayer, UINT8 viewnum, player_t *player)
{
	angle_t angleChange = 0;

	// Chasecam stops in these situations, so local cam should stop too.
	// Otherwise it'll jerk when it resumes.
	if (player->playerstate == PST_DEAD)
		return;
	if (player->mo != NULL && !P_MobjWasRemoved(player->mo) && player->mo->hitlag > 0)
		return;

	while (realtics > 0)
	{
		localsteering[ssplayer - 1] = K_UpdateSteeringValue(localsteering[ssplayer - 1], cmd->turning);
		angleChange = K_GetKartTurnValue(player, localsteering[ssplayer - 1]) << TICCMD_REDUCE;

		realtics--;
	}

#if 0
	// Left here in case it needs unsealing later. This tried to replicate an old localcam function, but this behavior was unpopular in tests.
	//if (player->pflags & PF_DRIFTEND)
	{
		localangle[ssplayer - 1] = player->mo->angle;
	}
	else
#endif
	{
		localangle[viewnum] += angleChange;
	}
}

}; // namespace

void G_BuildTiccmd(ticcmd_t *cmd, INT32 realtics, UINT8 ssplayer)
{
	const UINT8 forplayer = ssplayer-1;
	const UINT8 viewnum = G_PartyPosition(g_localplayers[forplayer]);

	INT32 forward;

	joystickvector2_t joystickvector;

	player_t *player = &players[g_localplayers[forplayer]];
	//camera_t *thiscam = &camera[forplayer];
	//boolean *kbl = &keyboard_look[forplayer];
	//boolean *rd = &resetdown[forplayer];
	//const boolean mouseaiming = player->spectator;

	if (demo.playback) return;

	// Is there any reason this can't just be I_BaseTiccmd?
	switch (ssplayer)
	{
		case 2:
			G_CopyTiccmd(cmd, I_BaseTiccmd2(), 1);
			break;
		case 3:
			G_CopyTiccmd(cmd, I_BaseTiccmd3(), 1);
			break;
		case 4:
			G_CopyTiccmd(cmd, I_BaseTiccmd4(), 1);
			break;
		case 1:
		default:
			G_CopyTiccmd(cmd, I_BaseTiccmd(), 1); // empty, or external driver
			break;
	}

	cmd->angle = localangle[viewnum] >> TICCMD_REDUCE;

	// why build a ticcmd if we're paused?
	// Or, for that matter, if we're being reborn.
	if (paused || P_AutoPause() || (gamestate == GS_LEVEL && player->playerstate == PST_REBORN))
	{
		return;
	}

	cmd->flags = 0;

	if (menuactive || chat_on || CON_Ready())
	{
		cmd->flags |= TICCMD_TYPING;

		if (hu_keystrokes)
		{
			cmd->flags |= TICCMD_KEYSTROKE;
		}

		goto aftercmdinput;
	}

	if (G_IsPartyLocal(displayplayers[forplayer]) == false)
	{
		if (M_MenuButtonPressed(forplayer, MBT_A))
		{
			G_AdjustView(ssplayer, 1, true);
			K_ToggleDirector(false);
		}

		if (M_MenuButtonPressed(forplayer, MBT_X))
		{
			G_AdjustView(ssplayer, -1, true);
			K_ToggleDirector(false);
		}

		if (player->spectator == true)
		{
			// duplication of fire
			if (G_PlayerInputDown(forplayer, gc_item, 0))
			{
				cmd->buttons |= BT_ATTACK;
			}

			if (M_MenuButtonPressed(forplayer, MBT_R))
			{
				K_ToggleDirector(true);
			}
		}

		goto aftercmdinput;
	}

	if (K_PlayerUsesBotMovement(player))
	{
		// Bot ticcmd is generated by K_BuildBotTiccmd
		return;
	}

	joystickvector.xaxis = G_PlayerInputAnalog(forplayer, gc_right, 0) - G_PlayerInputAnalog(forplayer, gc_left, 0);
	joystickvector.yaxis = 0;
	G_HandleAxisDeadZone(forplayer, &joystickvector);

	// For kart, I've turned the aim axis into a digital axis because we only
	// use it for aiming to throw items forward/backward and the vote screen
	// This mean that the turn axis will still be gradient but up/down will be 0
	// until the stick is pushed far enough
	joystickvector.yaxis = G_PlayerInputAnalog(forplayer, gc_down, 0) - G_PlayerInputAnalog(forplayer, gc_up, 0);

	if (encoremode)
	{
		joystickvector.xaxis = -joystickvector.xaxis;
	}

	forward = 0;
	cmd->turning = 0;
	cmd->aiming = 0;

	if (joystickvector.xaxis != 0)
	{
		cmd->turning -= (joystickvector.xaxis * KART_FULLTURN) / JOYAXISRANGE;
	}

	if (player->spectator || objectplacing) // SRB2Kart: spectators need special controls
	{
		if (G_PlayerInputDown(forplayer, gc_accel, 0))
		{
			cmd->buttons |= BT_ACCELERATE;
		}

		if (G_PlayerInputDown(forplayer, gc_brake, 0))
		{
			cmd->buttons |= BT_BRAKE;
		}

		if (G_PlayerInputDown(forplayer, gc_lookback, 0))
		{
			cmd->aiming -= joystickvector.yaxis;
		}
		else
		{
			if (joystickvector.yaxis < 0)
			{
				forward += MAXPLMOVE;
			}

			if (joystickvector.yaxis > 0)
			{
				forward -= MAXPLMOVE;
			}
		}
	}
	else
	{
		// forward with key or button // SRB2kart - we use an accel/brake instead of forward/backward.
		INT32 value = G_PlayerInputAnalog(forplayer, gc_accel, 0);
		if (value != 0)
		{
			cmd->buttons |= BT_ACCELERATE;
			forward += ((value * MAXPLMOVE) / JOYAXISRANGE);
		}

		value = G_PlayerInputAnalog(forplayer, gc_brake, 0);
		if (value != 0)
		{
			cmd->buttons |= BT_BRAKE;
			forward -= ((value * MAXPLMOVE) / JOYAXISRANGE);
		}

		// But forward/backward IS used for aiming.
		if (joystickvector.yaxis != 0)
		{
			cmd->throwdir -= (joystickvector.yaxis * KART_FULLTURN) / JOYAXISRANGE;
		}
	}

	// drift
	if (G_PlayerInputDown(forplayer, gc_drift, 0))
	{
		cmd->buttons |= BT_DRIFT;
	}

	// C
	if (G_PlayerInputDown(forplayer, gc_spindash, 0))
	{
		cmd->buttons |= BT_SPINDASHMASK;
	}

	// fire
	if (G_PlayerInputDown(forplayer, gc_item, 0))
	{
		cmd->buttons |= BT_ATTACK;
	}

	// rear view
	if (G_PlayerInputDown(forplayer, gc_lookback, 0))
	{
		cmd->buttons |= BT_LOOKBACK;
	}

	// respawn
	if (G_PlayerInputDown(forplayer, gc_respawn, 0))
	{
		cmd->buttons |= (BT_RESPAWN | BT_EBRAKEMASK);
	}

	// mp general function button
	if (G_PlayerInputDown(forplayer, gc_vote, 0))
	{
		cmd->buttons |= BT_VOTE;
	}

	// lua buttons a thru c
	if (G_PlayerInputDown(forplayer, gc_luaa, 0)) { cmd->buttons |= BT_LUAA; }
	if (G_PlayerInputDown(forplayer, gc_luab, 0)) { cmd->buttons |= BT_LUAB; }
	if (G_PlayerInputDown(forplayer, gc_luac, 0)) { cmd->buttons |= BT_LUAC; }

	// spectator aiming shit, ahhhh...
	/*
	{
		INT32 player_invert = invertmouse ? -1 : 1;
		INT32 screen_invert =
			(player->mo && (player->mo->eflags & MFE_VERTICALFLIP)
			 && (!thiscam->chase)) //because chasecam's not inverted
			 ? -1 : 1; // set to -1 or 1 to multiply

		axis = PlayerJoyAxis(ssplayer, AXISLOOK);
		if (analogjoystickmove && axis != 0 && lookaxis && player->spectator)
			cmd->aiming += (axis<<16) * screen_invert;

		// spring back if not using keyboard neither mouselookin'
		if (*kbl == false && !lookaxis && !mouseaiming)
			cmd->aiming = 0;

		if (player->spectator)
		{
			if (PlayerInputDown(ssplayer, gc_lookup) || (gamepadjoystickmove && axis < 0))
			{
				cmd->aiming += KB_LOOKSPEED * screen_invert;
				*kbl = true;
			}
			else if (PlayerInputDown(ssplayer, gc_lookdown) || (gamepadjoystickmove && axis > 0))
			{
				cmd->aiming -= KB_LOOKSPEED * screen_invert;
				*kbl = true;
			}
		}

		if (PlayerInputDown(ssplayer, gc_centerview)) // No need to put a spectator limit on this one though :V
			cmd->aiming = 0;
	}
	*/

	cmd->forwardmove += (SINT8)forward;

aftercmdinput:

	/* 	Lua: Allow this hook to overwrite ticcmd.
		We check if we're actually in a level because for some reason this Hook would run in menus and on the titlescreen otherwise.
		Be aware that within this hook, nothing but this player's cmd can be edited (otherwise we'd run in some pretty bad synching problems since this is clientsided, or something)

		Possible usages for this are:
			-Forcing the player to perform an action, which could otherwise require terrible, terrible hacking to replicate.
			-Preventing the player to perform an action, which would ALSO require some weirdo hacks.
			-Making some galaxy brain autopilot Lua if you're a masochist
			-Making a Mario Kart 8 Deluxe tier baby mode that steers you away from walls and whatnot. You know what, do what you want!
	*/
	if (addedtogame && gamestate == GS_LEVEL)
	{
		LUA_HookTiccmd(player, cmd, HOOK(PlayerCmd));

		// Send leveltime when this tic was generated to the server for control lag calculations.
		// Only do this when in a level. Also do this after the hook, so that it can't overwrite this.
		cmd->latency = (leveltime & TICCMD_LATENCYMASK);
	}

	if (cmd->forwardmove > MAXPLMOVE)
		cmd->forwardmove = MAXPLMOVE;
	else if (cmd->forwardmove < -MAXPLMOVE)
		cmd->forwardmove = -MAXPLMOVE;

	if (cmd->turning > KART_FULLTURN)
		cmd->turning = KART_FULLTURN;
	else if (cmd->turning < -KART_FULLTURN)
		cmd->turning = -KART_FULLTURN;

	if (cmd->throwdir > KART_FULLTURN)
		cmd->throwdir = KART_FULLTURN;
	else if (cmd->throwdir < -KART_FULLTURN)
		cmd->throwdir = -KART_FULLTURN;

	G_DoAnglePrediction(cmd, realtics, ssplayer, viewnum, player);
}
