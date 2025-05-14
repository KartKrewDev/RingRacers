// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
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
#include "../k_specialstage.h" // specialstageinfo
#include "../k_kart.h"

#define FLOAT_HEIGHT ( 12 * FRACUNIT )
#define FLOAT_TIME ( 2 * TICRATE )
#define FLOAT_ANGLE ( ANGLE_MAX / FLOAT_TIME )

#define SCALE_LO ( FRACUNIT * 2 / 3 )
#define SCALE_HI ( FRACUNIT )
#define SCALE_TIME ( 5 * TICRATE / 2 )
#define SCALE_ANGLE ( ANGLE_MAX / SCALE_TIME )

#define item_vfxtimer(o) ((o)->cvmem)

#define PREVIEWFLAGS (RF_ADD|RF_TRANS30)

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

		// Always use normal item box rules -- could pass in "PICKUP_EGGBOX" for fakes but they blend in better like this
		if (P_CanPickupItem(&players[i], PICKUP_ITEMBOX))
		{
			// Check for players who can take this pickup, but won't be allowed to (antifarming)
			UINT8 mytype = (mobj->flags2 & MF2_BOSSDEAD) ? CHEESE_RINGBOX : CHEESE_ITEMBOX;
			if (P_IsPickupCheesy(&players[i], mytype))
				continue;

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

	// Fade items in as we cross the first checkpoint, but don't touch their visibility otherwise!
	if (!((mobj->flags & MF_NOCLIPTHING) || mobj->fuse))
	{
		UINT8 maxgrab = 0;

		for (UINT8 i = 0; i <= r_splitscreen; i++)
		{
			maxgrab = max(maxgrab, players[displayplayers[i]].cangrabitems);
		}

		if (maxgrab == 0)
			mobj->renderflags |= RF_DONTDRAW;
		else
			mobj->renderflags &= ~RF_DONTDRAW;

		if (maxgrab > 0 && maxgrab <= EARLY_ITEM_FLICKER)
		{
			UINT8 maxtranslevel = NUMTRANSMAPS;

			UINT8 trans = maxgrab;
			if (trans > maxtranslevel)
				trans = maxtranslevel;
			trans = NUMTRANSMAPS - trans;

			mobj->renderflags &= ~(RF_TRANSMASK);

			if (trans != 0)
				mobj->renderflags |= (trans << RF_TRANSSHIFT);
		}
	}


	// Respawn flow, documented by a dumb asshole:
	// P_TouchSpecialThing -> P_ItemPop sets fuse, NOCLIPTHING and DONTDRAW.
	// P_FuseThink does visual flicker, and when fuse is 0, unsets NOCLIPTHING/DONTDRAW/etc...
	// ...unless it's a map-start box from Battle, in which case it does nothing and waits for
	// P_RespawnBattleBoxes to trigger the effect instead, since Battle boxes don't respawn until
	// the player's cleared out a good portion of the map.
	//
	// Then extraval1 starts ticking up and triggers the transformation from Ringbox to Random Item.
	if (mobj->fuse == 0 && !(mobj->flags & MF_NOCLIPTHING) && !(mobj->flags2 & MF2_BOSSDEAD) && !K_ThunderDome()
		&& (modeattacking == ATTACKING_NONE || !!(modeattacking & ATTACKING_SPB) || specialstageinfo.valid)) // Time Attacking in Special is a fucked-looking exception
	{
		mobj->extravalue1++;

		// Dumb, but in Attack starts (or POSITION from hell) you can reach early boxes before they transform.
		if (leveltime == 0)
			mobj->extravalue1 = RINGBOX_TIME;

		// Don't transform stuff that isn't a Ring Box, idiot
		statenum_t boxstate = mobj->state - states;
		if (boxstate < S_RINGBOX1 || boxstate > S_RINGBOX12)
			return;

		if (mobj->extravalue1 == RINGBOX_TIME || specialstageinfo.valid)
		{
			// Sync the position in RINGBOX and RANDOMITEM animations.
			statenum_t animDelta = mobj->state - states - S_RINGBOX1;
			P_SetMobjState(mobj, S_RANDOMITEM1 + (animDelta%12));
		}
	}
}

boolean Obj_RandomItemSpawnIn(mobj_t *mobj)
{
	// We don't want item spawnpoints to be visible during
	// POSITION in Battle.
	// battleprisons isn't set in time to do this on spawn. GROAN
	if ((mobj->flags2 & MF2_BOSSFLEE) && (gametyperules & (GTR_CIRCUIT|GTR_PAPERITEMS)) == GTR_PAPERITEMS && !battleprisons)
		mobj->renderflags |= RF_DONTDRAW;

	if ((leveltime == starttime) && !(gametyperules & GTR_CIRCUIT) && (mobj->flags2 & MF2_BOSSFLEE)) // here on map start?
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

		// Clear "hologram" appearance if it was set in RandomItemSpawn.
		if ((gametyperules & GTR_CIRCUIT) != GTR_CIRCUIT)
			mobj->renderflags &= ~PREVIEWFLAGS;

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

	// Respawns are handled by P_RespawnBattleBoxes,
	// but we do need to start MT_RANDOMITEMs in the right state...
	if (mobj->type == MT_RANDOMITEM && (gametyperules & GTR_BUMPERS))
	{
		mobj->extravalue1 = RINGBOX_TIME;
		P_SetMobjState(mobj, S_RANDOMITEM1);
	}

	if (leveltime == 0)
	{
		mobj->flags2 |= MF2_BOSSFLEE; // mark as here on map start
		if ((gametyperules & GTR_CIRCUIT) != GTR_CIRCUIT) // delayed
		{
			P_UnsetThingPosition(mobj);
			mobj->flags |= (MF_NOCLIPTHING|MF_NOBLOCKMAP);
			mobj->renderflags |= PREVIEWFLAGS;
			P_SetThingPosition(mobj);
		}
	}

	mobj->destscale = Obj_RandomItemScale(mobj->destscale);
	P_SetScale(mobj, mobj->destscale);
}
