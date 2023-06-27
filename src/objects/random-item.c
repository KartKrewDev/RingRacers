// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  random-item.c
/// \brief Random item boxes

#include "../doomdef.h"
#include "../doomstat.h"
#include "../k_objects.h"
#include "../g_game.h"
#include "../p_local.h"
#include "../r_defs.h"
#include "../k_battle.h"
#include "../m_random.h"

#define FLOAT_HEIGHT ( 12 * FRACUNIT )
#define FLOAT_TIME ( 2 * TICRATE )
#define FLOAT_ANGLE ( ANGLE_MAX / FLOAT_TIME )

#define SCALE_LO ( FRACUNIT * 2 / 3 )
#define SCALE_HI ( FRACUNIT )
#define SCALE_TIME ( 5 * TICRATE / 2 )
#define SCALE_ANGLE ( ANGLE_MAX / SCALE_TIME )

#define item_vfxtimer(o) ((o)->cvmem)

static player_t *GetItemBoxPlayer(mobj_t *mobj)
{
	fixed_t closest = INT32_MAX;
	player_t *player = NULL;
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!(playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo) && !players[i].spectator))
		{
			continue;
		}

		// Always use normal item box rules -- could pass in "2" for fakes but they blend in better like this
		if (P_CanPickupItem(&players[i], 1))
		{
			fixed_t dist = P_AproxDistance(P_AproxDistance(
				players[i].mo->x - mobj->x,
				players[i].mo->y - mobj->y),
				players[i].mo->z - mobj->z
			);

			if (dist > 8192*mobj->scale)
			{
				continue;
			}

			if (dist < closest)
			{
				player = &players[i];
				closest = dist;
			}
		}
	}

	return player;
}

static void ItemBoxColor(mobj_t *mobj)
{
	player_t *player = GetItemBoxPlayer(mobj);
	skincolornum_t color = SKINCOLOR_BLACK;

	if (player != NULL)
	{
		color = player->skincolor;
	}

	mobj->color = color;
	mobj->colorized = false;
}

static void ItemBoxBob(mobj_t *mobj)
{
	const fixed_t sine = FINESINE((FLOAT_ANGLE * item_vfxtimer(mobj)) >> ANGLETOFINESHIFT);
	const fixed_t bob = FixedMul(FLOAT_HEIGHT, sine);
	mobj->spriteyoffset = FLOAT_HEIGHT + bob;
}

static void ItemBoxScaling(mobj_t *mobj)
{
	const fixed_t sine = FINESINE((SCALE_ANGLE * item_vfxtimer(mobj)) >> ANGLETOFINESHIFT);
	const fixed_t newScale = SCALE_LO + FixedMul(SCALE_HI - SCALE_LO, (sine + FRACUNIT) >> 1);
	mobj->spritexscale = mobj->spriteyscale = newScale;
}

void Obj_RandomItemVisuals(mobj_t *mobj)
{
	ItemBoxColor(mobj);
	ItemBoxBob(mobj);
	ItemBoxScaling(mobj);
	item_vfxtimer(mobj)++;

	if (mobj->type != MT_RANDOMITEM)
		return;

	// Respawn flow, documented by a dumb asshole:
	// P_TouchSpecialThing -> P_ItemPop sets fuse, NOCLIPTHING and DONTDRAW.
	// P_FuseThink does visual flicker, and when fuse is 0, unsets NOCLIPTHING/DONTDRAW/etc...
	// ...unless it's a map-start box from Battle, in which case it does nothing and waits for
	// P_RespawnBattleBoxes to trigger the effect instead, since Battle boxes don't respawn until
	// the player's cleared out a good portion of the map.
	//
	// Then extraval1 starts ticking up and triggers the transformation from Ringbox to Random Item.
	if (mobj->fuse == 0 && !(mobj->flags & MF_NOCLIPTHING) && !cv_thunderdome.value)
	{
		mobj->extravalue1++;
		if (mobj->extravalue1 == RINGBOX_TIME)
		{
			// Sync the position in RINGBOX and RANDOMITEM animations.
			statenum_t animDelta = mobj->state - states - S_RINGBOX1;
			P_SetMobjState(mobj, S_RANDOMITEM1 + (animDelta%12));
		}
	}

}

boolean Obj_RandomItemSpawnIn(mobj_t *mobj)
{
	if ((leveltime == starttime) && !(gametyperules & GTR_CIRCUIT) && (mobj->flags2 & MF2_BOSSNOTRAP)) // here on map start?
	{
		if (gametyperules & GTR_PAPERITEMS)
		{
			if (battleprisons == true)
			{
				;
			}
			else
			{
				// Spawn a battle monitor in your place and Fucking Die
				mobj_t *paperspawner = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_PAPERITEMSPOT);
				paperspawner->spawnpoint = mobj->spawnpoint;
				mobj->spawnpoint->mobj = paperspawner;
				P_RemoveMobj(mobj);
				return false;
			}
		}

		// poof into existance
		P_UnsetThingPosition(mobj);
		mobj->flags &= ~(MF_NOCLIPTHING|MF_NOBLOCKMAP);
		mobj->renderflags &= ~RF_DONTDRAW;
		P_SetThingPosition(mobj);
		P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_EXPLODE);
		nummapboxes++;
	}

	return true;
}

fixed_t Obj_RandomItemScale(fixed_t oldScale)
{
	const fixed_t intendedScale = oldScale * 3;
	const fixed_t maxScale = FixedDiv(MAPBLOCKSIZE, mobjinfo[MT_RANDOMITEM].radius); // don't make them larger than the blockmap can handle

	return min(intendedScale, maxScale);
}

void Obj_RandomItemSpawn(mobj_t *mobj)
{
	item_vfxtimer(mobj) = P_RandomRange(PR_DECORATION, 0, SCALE_TIME - 1);

	mobj->destscale = Obj_RandomItemScale(mobj->destscale);
	P_SetScale(mobj, mobj->destscale);
}
