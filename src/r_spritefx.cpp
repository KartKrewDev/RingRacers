// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \brief Special effects for sprite rendering

#include "d_player.h"
#include "info.h"
#include "k_objects.h"
#include "p_tick.h"
#include "r_splats.h"
#include "r_things.h"

INT32 R_ThingLightLevel(mobj_t* thing)
{
	INT32 lightlevel = thing->lightlevel;

	player_t* player = thing->player;

	if (player)
	{
		if (player->powerupVFXTimer)
		{
			if ((leveltime & 1))
			{
				lightlevel -= 255;
			}
			else
			{
				lightlevel += 255;
			}
		}
		else if ((player->instaWhipCharge || player->defenseLockout) && !player->whip && (leveltime & 1))
		{
			// Darken on every other frame of instawhip cooldown
			lightlevel -= 128;
		}

		if (player->pflags & PF_CASTSHADOW)
		{
			lightlevel -= 255;
		}

		if (!R_CanShowSkinInDemo(((skin_t*)thing->skin)->skinnum)
		&& ((cv_reducevfx.value) || (!thing->colorized && !thing->hitlag)))
		{
			lightlevel -= 128;
		}
	}

	return lightlevel;
}

// Use this function to set the slope of a splat sprite.
//
// slope->o, slope->d and slope->zdelta must be set, none of
// the other fields on pslope_t are used.
//
// Return true if you want the slope to be used. The object
// must have RF_SLOPESPLAT and mobj_t.floorspriteslope must be
// NULL. (If RF_OBJECTSLOPESPLAT is set, then
// mobj_t.standingslope must also be NULL.)
boolean R_SplatSlope(mobj_t* mobj, vector3_t position, pslope_t* slope)
{
	switch (mobj->type)
	{
	case MT_INSTAWHIP_RECHARGE: {
		// Create an acute angle
		slope->o = position;
		FV2_Load(&slope->d, FCOS(mobj->angle) / 2, FSIN(mobj->angle) / 2);
		slope->zdelta = FRACUNIT;
		return true;
	}

	default:
		break;
	}

	return false;
}

boolean R_CustomShadowZ(mobj_t* thing, fixed_t *z, pslope_t** slope)
{
	switch (thing->type)
	{
	case MT_SHADOW:
		if (Obj_FakeShadowZ(thing, z, slope))
		{
			return true;
		}
		break;

	default:
		break;
	}

	return false;
}
