// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Lachlan "Lach" Wright
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ancient-gear.c
/// \brief Ancient Gear object code.

#include "../p_local.h"
#include "../k_objects.h"
#include "../s_sound.h"
#include "../k_endcam.h"
#include "../k_hud.h"
#include "../m_cond.h"
#include "../g_game.h"

#define DEATH_FREEZE_TIME (5*TICRATE)
#define DEATH_TIME (2*TICRATE)
#define DEATH_RISE_SPEED (3*FRACUNIT)

#define DELTA_YAW (-ANG2)
#define DELTA_ROLL (ANG2)
static const angle_t DELTA_YAW_ACCELERATION = (-ANG1 / 2);
static const angle_t DELTA_ROLL_ACCELERATION = (ANG1 / 3);
static const fixed_t DELTA_SPRITEXSCALE = ((FRACUNIT/4 - FRACUNIT) / DEATH_TIME);
static const fixed_t DELTA_SPRITEYSCALE = ((FRACUNIT*3 - FRACUNIT) / DEATH_TIME);
static const tic_t TRANS_SHIFT_RATE = (DEATH_TIME / 10);

#define MAX_GEARS 32

static UINT8 numGears = 0;
static UINT32 gearBank = 0;
static UINT8 gearBankIndex = 0;
static boolean allGearsCollected = false;
static player_t *collectingPlayer = NULL;
static mobj_t *minimapGear = NULL;

static void UpdateAncientGearPart(mobj_t *part)
{
	mobj_t *gear = part->target;
	fixed_t radius = FixedMul(FixedMul(part->movefactor, gear->scale), gear->spritexscale);
	fixed_t xOffset = P_ReturnThrustX(NULL, part->angle - ANGLE_90, radius);
	fixed_t yOffset = P_ReturnThrustY(NULL, part->angle - ANGLE_90, radius);

	P_InstaScale(part, gear->scale);
	P_MoveOrigin(part,
		gear->x + xOffset,
		gear->y + yOffset,
		gear->z + gear->height / 2
	);
	part->spritexscale = gear->spritexscale;
	part->spriteyscale = gear->spriteyscale;
}

void Obj_AncientGearSpawn(mobj_t *gear)
{
	UINT8 i;
	mobj_t *part = gear;

	numGears++;

	gear->extravalue1 = DELTA_YAW;
	gear->extravalue2 = DELTA_ROLL;

	for (i = 0; i < 6; i++)
	{
		P_SetTarget(&part->hnext, P_SpawnMobjFromMobj(gear, 0, 0, 0, MT_ANCIENTGEAR_PART));
		P_SetTarget(&part->hnext->hprev, part);
		part = part->hnext;
		P_SetTarget(&part->target, gear);

		part->angle += (i & 1) * ANGLE_180;

		if (i < 2) // middle parts
		{
			part->angle += ANGLE_90;
			part->movefactor = 10 * FRACUNIT; // horizontal offset from hitbox
			part->extravalue1 = 0; // direction to roll the sprite
			part->frame = (part->frame & ~FF_FRAMEMASK) | 1;
			P_SetTarget(&part->tracer, gear);
			part->flags2 |= MF2_LINKDRAW;
		}
		else // side parts
		{
			part->movefactor = 7 * FRACUNIT; // horizontal offset from hitbox
			part->extravalue1 = (i & 1) * 2 - 1; // direction to roll the sprite
			if (i > 3) // fake brightmaps
			{
				part->frame = (part->frame & ~FF_FRAMEMASK) | 3 | FF_FULLBRIGHT;
				part->dispoffset++;
			}
			else
			{
				part->frame = (part->frame & ~FF_FRAMEMASK) | 2;
			}
		}

		UpdateAncientGearPart(part);
		part->old_x = part->x;
		part->old_y = part->y;
		part->old_z = part->z;
		part->old_angle = part->angle;
		part->old_scale = part->scale;
	}
}

void Obj_AncientGearPartThink(mobj_t *part)
{
	mobj_t *gear = part->target;
	if (P_MobjWasRemoved(gear))
	{
		P_RemoveMobj(part);
		return;
	}
	part->angle += gear->extravalue1;
	part->rollangle += gear->extravalue2 * part->extravalue1;
	UpdateAncientGearPart(part);
}

void Obj_AncientGearRemoved(mobj_t *gear)
{
	if (gear == minimapGear)
	{
		minimapGear = NULL;
	}

	while (!P_MobjWasRemoved(gear->hnext))
	{
		P_RemoveMobj(gear->hnext);
	}
}

void Obj_AncientGearTouch(mobj_t *gear, mobj_t *toucher)
{
	P_KillMobj(gear, NULL, toucher, DMG_NORMAL);
}

void Obj_AncientGearDeath(mobj_t *gear, mobj_t *source)
{
	if (--numGears == 0)
	{
		allGearsCollected = true;
		M_UpdateUnlockablesAndExtraEmblems(true, true);
	}

	// if this gear has a bank account, mark it as collected so that it doesn't respawn upon retrying the map
	if (gear->threshold != 0)
	{
		gearBank |= (1 << (gear->threshold - 1));
	}

	gear->fuse = DEATH_TIME;
	gear->shadowscale = gear->spritexscale;

	// give the gear some upwards momentum
	gear->flags |= MF_NOCLIPHEIGHT;
	P_SetObjectMomZ(gear, DEATH_RISE_SPEED, false);

	// don't activate the round win camera if there is no camera target,
	// or if the round win camera is already active for any actual reason
	if (P_MobjWasRemoved(source) || source->player == NULL || g_endcam.active)
	{
		return;
	}

	// track the collecting player to display a message for them
	collectingPlayer = source->player;
	P_SetTarget(&gear->target, source);

	// play the collection jingle!
	S_StartSound(NULL, gear->info->seesound);

	// fade out the music for as long as the sound plays
	g_musicfade.start = leveltime;
	g_musicfade.end = g_musicfade.start + DEATH_FREEZE_TIME;
	g_musicfade.fade = 12;
	g_musicfade.ticked = false;

	// start the round win camera
	gear->flags2 |= MF2_BEYONDTHEGRAVE; // a gear with this flag will stop the round win camera upon next thinking
	K_StartRoundWinCamera(
		source,
		source->player->angleturn,
		FixedMul(cv_cam_dist[0].value, mapobjectscale),
		6*TICRATE,
		FRACUNIT/16,
		DEATH_FREEZE_TIME
	);
}

void Obj_AncientGearDeadThink(mobj_t *gear)
{
	mobj_t *part = gear;

	// if the round win camera was activated, tell it to stop focusing on the player,
	// and show the player a message
	if (gear->flags2 & MF2_BEYONDTHEGRAVE)
	{
		gear->flags2 &= ~MF2_BEYONDTHEGRAVE;
		K_StopRoundWinCamera();

		collectingPlayer = NULL;
		K_AddMessage(
			numGears > 0 ? va("%d Ancient Gear%s left!", numGears, numGears == 1 ? "" : "s")
			: "All Ancient Gears collected!"
			, true, false
		);
	}

	// play another sound once immediately after the round win camera finishes
	if (!(gear->flags2 & MF2_DONTRESPAWN))
	{
		gear->flags2 |= MF2_DONTRESPAWN;
		S_StartSound(gear, gear->info->deathsound);
	}

	// increase the translucency level every so often
	if (gear->fuse % TRANS_SHIFT_RATE == 0)
	{
		while (!P_MobjWasRemoved(part = part->hnext))
		{
			part->frame += FF_TRANS10;
		}
	}

	// accelerate the spinning and rotating
	gear->extravalue1 += DELTA_YAW_ACCELERATION;
	gear->extravalue2 += DELTA_ROLL_ACCELERATION;
	
	// stretch the gear
	gear->spritexscale += DELTA_SPRITEXSCALE;
	gear->spriteyscale += DELTA_SPRITEYSCALE;
	gear->shadowscale = gear->spritexscale;
}

boolean Obj_AllowNextAncientGearSpawn(void)
{
	// always allow gears spawned by objectplace
	if (objectplacing)
	{
		return true;
	}

	// don't allow the map to spawn more gears than we can bank
	if (gearBankIndex >= MAX_GEARS)
	{
		CONS_Alert(CONS_WARNING, "Map exceeds maximum possible number of Ancient Gears (%d)!\n", MAX_GEARS);
		return false;
	}

	// don't spawn a gear that was already collected prior to retrying
	if (gearBank & (1 << gearBankIndex++))
	{
		return false;
	}

	return true;
}

void Obj_AncientGearSetup(mobj_t *gear, mapthing_t *mt)
{
	(void)mt;

	mobj_t *part = gear;
	while ((part = part->hnext))
	{
		UpdateAncientGearPart(part); // update part scales to apply mapthing scales
	}

	if (!objectplacing)
	{
		gear->threshold = gearBankIndex; // allocate this gear a bank account slot
	}
}

void Obj_AncientGearLevelInit(void)
{
	if (!G_GetRetryFlag() || G_IsModeAttackRetrying())
	{
		gearBank = 0;
		allGearsCollected = false;
	}
	gearBankIndex = 0;
	numGears = 0;
	collectingPlayer = NULL;
	minimapGear = NULL;
}

player_t *Obj_GetAncientGearCollectingPlayer(void)
{
	return collectingPlayer;
}

boolean Obj_AllAncientGearsCollected(void)
{
	return allGearsCollected;
}

mobj_t *Obj_GetAncientGearMinimapMobj(void)
{
	UINT8 lowestTag = UINT8_MAX;
	UINT8 tag;
	mobj_t *mobj;

	// no gears in the map? nothing to display
	if (numGears == 0)
	{
		return NULL;
	}

	// if a gear is currently being tracked, display it on the minimap
	if (!P_MobjWasRemoved(minimapGear))
	{
		// only display the gear while uncollected,
		// but keep it tracked so there's some natural delay between when one gear disappears and when the next one is chosen
		if (minimapGear->health > 0)
		{
			return minimapGear;
		}
		return NULL;
	}

	minimapGear = NULL;

	// try to find a new gear to track
	for (mobj = trackercap; mobj; mobj = mobj->itnext)
	{
		if (
			mobj->type != MT_ANCIENTGEAR
			|| !(mobj->health > 0)
		)
		{
			continue;
		}

		tag = ((UINT8)(mobj->thing_args[0] - 1)) % MAX_GEARS; // 0 minus 1 wraps around to 31 so that untagged gears are chosen last

		if (tag < lowestTag)
		{
			lowestTag = tag;
			minimapGear = mobj;
		}
	}

	// display the tracked gear, if found
	return minimapGear;
}
