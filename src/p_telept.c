// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_telept.c
/// \brief Teleportation

#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"
#include "r_main.h"
#include "r_fps.h"

/**	\brief	The P_MixUp function

	\param	thing	mobj_t to mix up
	\param	x	new x pos
	\param	y	new y pos
	\param	z	new y pos
	\param	angle	new angle to look at

	\return	void


*/
void P_MixUp(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, angle_t angle,
			INT16 cheatcheckx, INT16 cheatchecky, INT16 cheatcheckz,
			INT32 cheatchecknum, tic_t cheatchecktime, angle_t cheatcheckangle,
			fixed_t cheatcheckscale, angle_t drawangle, INT32 flags2)
{
	const INT32 takeflags2 = MF2_OBJECTFLIP;
	UINT8 i;

	(void)cheatchecktime;
	(void)cheatcheckangle;
	(void)cheatcheckscale;

	// the move is ok,
	// so link the thing into its new position
	P_UnsetThingPosition(thing);

	// Remove touching_sectorlist from mobj.
	if (sector_list)
	{
		P_DelSeclist(sector_list);
		sector_list = NULL;
	}

	thing->x = x;
	thing->y = y;
	thing->z = z;

	if (thing->player)
	{
		if (thing->eflags & MFE_VERTICALFLIP)
			thing->player->viewz = thing->z + thing->height - thing->player->viewheight;
		else
			thing->player->viewz = thing->z + thing->player->viewheight;

		if (!thing->tracer)
			thing->reactiontime = TICRATE/2; // don't move for about half a second

		// absolute angle position
		P_SetPlayerAngle(thing->player, angle);

		// move chasecam at new player location
		for (i = 0; i <= r_splitscreen; i++)
		{
			if (thing->player != &players[displayplayers[i]])
				continue;
			if (camera[i].chase)
				P_ResetCamera(thing->player, &camera[i]);
			R_ResetViewInterpolation(i + 1);
		}

		// don't run in place after a teleport
		thing->player->cmomx = thing->player->cmomy = 0;
		thing->player->rmomx = thing->player->rmomy = 0;
		if (!thing->tracer)
			thing->player->speed = 0;

		// Cheatcheck information
		thing->player->respawn.pointx = cheatcheckx;
		thing->player->respawn.pointy = cheatchecky;
		thing->player->respawn.pointz = cheatcheckz;
		thing->player->cheatchecknum = cheatchecknum;

		thing->player->drawangle = drawangle;

		P_ResetPlayer(thing->player);
		P_SetPlayerMobjState(thing, S_KART_STILL); // SRB2kart - was S_PLAY_STND

		P_FlashPal(thing->player, PAL_MIXUP, 10);
	}

	thing->old_angle += (angle-thing->angle);
	thing->angle = angle;

	thing->momx = thing->momy = thing->momz = 0;

	thing->flags2 = (thing->flags2 & ~takeflags2) | (flags2 & takeflags2);
}

/**	\brief	The P_Teleport function

	\param	thing	mobj_t to teleport
	\param	x	new x pos
	\param	y	new y pos
	\param	z	new y pos
	\param	angle	new angle to look at

	\return	if true, the thing "teleported"


*/
boolean P_Teleport(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, angle_t angle, boolean flash, boolean dontstopmove)
{
	UINT8 i;

	if (!P_SetOrigin(thing, x, y, z))
		return false;

	if (!dontstopmove)
		thing->momx = thing->momy = thing->momz = 0;
	else // Change speed to match direction
		P_InstaThrust(thing, angle, FixedHypot(thing->momx, thing->momy));

	if (thing->player)
	{
		if (thing->eflags & MFE_VERTICALFLIP)
			thing->player->viewz = thing->z + thing->height - thing->player->viewheight;
		else
			thing->player->viewz = thing->z + thing->player->viewheight;

		// don't run in place after a teleport
		if (!dontstopmove)
		{
			thing->player->cmomx = thing->player->cmomy = 0;
			thing->player->rmomx = thing->player->rmomy = 0;
			thing->player->speed = 0;
			P_ResetPlayer(thing->player);
			P_SetPlayerMobjState(thing, S_KART_STILL); // SRB2kart - was S_PLAY_STND

			thing->reactiontime = TICRATE/2; // don't move for about half a second
			thing->player->drawangle = angle;
		}
		else
			thing->player->drawangle += (angle - thing->angle);

		// absolute angle position
		P_SetPlayerAngle(thing->player, angle);

		for (i = 0; i <= r_splitscreen; i++)
		{
			if (thing == players[displayplayers[i]].mo)
			{
				if (camera[i].chase)
				{
					// move chasecam at new player location
					P_ResetCamera(thing->player, &camera[i]);
				}

				R_ResetViewInterpolation(1 + i);
			}
		}

		if (flash)
			P_FlashPal(thing->player, PAL_MIXUP, 10);
	}

	return true;
}
