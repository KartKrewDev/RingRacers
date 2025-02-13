// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \brief Joypolis Trick Balloons

#include "../info.h"
#include "../doomdef.h"
#include "../g_game.h"
#include "../p_local.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../s_sound.h"

void Obj_TrickBalloonMobjSpawn(mobj_t* mobj)
{
	mobjtype_t spawntype;
	switch (mobj->type)
	{
		case MT_TRICKBALLOON_RED:
			spawntype = MT_TRICKBALLOON_RED_POINT;
			break;
		case MT_TRICKBALLOON_YELLOW:
			spawntype = MT_TRICKBALLOON_YELLOW_POINT;
			break;
		default:
			return;
	}
	P_SpawnMobj(mobj->x, mobj->y, mobj->z, spawntype);
}

void Obj_TrickBalloonTouchSpecial(mobj_t* special, mobj_t* toucher)
{

	if (special->state == &states[S_TRICKBALLOON_RED1] || special->state == &states[S_TRICKBALLOON_RED2])
	{
		fixed_t vspeed = 32 << FRACBITS;

		if (toucher->player && toucher->player)
		{
			toucher->player->trickpanel = 1;
			toucher->player->pflags |= PF_TRICKDELAY;
			toucher->player->tricktime = 0;
		}

		K_DoPogoSpring(toucher, vspeed, 0);
		P_InstaThrust(toucher, toucher->angle, 42 * special->scale);

		S_StartSound(special, special->info->deathsound);
		P_SetMobjState(special, S_TRICKBALLOON_RED_POP1);
		return;
	}

	if (special->state == &states[S_TRICKBALLOON_YELLOW1] || special->state == &states[S_TRICKBALLOON_YELLOW2])
	{
		fixed_t vspeed = 16 << FRACBITS;

		if (toucher->player && toucher->player)
		{
			toucher->player->trickpanel = 1;
			toucher->player->pflags |= PF_TRICKDELAY;
			toucher->player->tricktime = 0;
		}

		K_DoPogoSpring(toucher, vspeed, 0);
		P_InstaThrust(toucher, toucher->angle, 42 * special->scale);

		S_StartSound(special, special->info->deathsound);
		P_SetMobjState(special, S_TRICKBALLOON_YELLOW_POP1);
		return;
	}
}
