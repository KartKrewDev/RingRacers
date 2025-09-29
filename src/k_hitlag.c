// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_hitlag.c
/// \brief Hitlag functions

#include "k_hitlag.h"

#include "doomdef.h"
#include "doomstat.h"
#include "k_battle.h"
#include "k_kart.h"
#include "m_random.h"
#include "p_local.h"
#include "r_main.h"
#include "s_sound.h"
#include "m_easing.h"

// Use for adding hitlag that should be mostly ignored by impervious players.
// (Currently only called in power clash, but in the future...?)
void K_AddHitLagFromCollision(mobj_t *mo, INT32 tics)
{
	boolean doAnything = true;

	if (mo->player == NULL || mo->type != MT_PLAYER)
		doAnything = false;
	else if (!K_PlayerCanPunt(mo->player))
		doAnything = false;

	if (!doAnything)
	{
		K_AddHitLag(mo, tics, false);
		return;
	}

	K_AddHitLag(mo, min(tics, 2), false);
}

/*--------------------------------------------------
	void K_AddHitLag(mobj_t *mo, INT32 tics, boolean fromDamage)

		See header file for description.
--------------------------------------------------*/
void K_AddHitLag(mobj_t *mo, INT32 tics, boolean fromDamage)
{
	if (mo == NULL || P_MobjWasRemoved(mo) || (mo->flags & MF_NOHITLAGFORME && mo->type != MT_PLAYER))
	{
		return;
	}

	mo->hitlag += tics;
	mo->hitlag = min(mo->hitlag, MAXHITLAGTICS);

	if (mo->player != NULL)
	{
		// Reset each time. We want to explicitly set this for bananas afterwards,
		// so make sure an old value doesn't possibly linger.
		mo->player->flipDI = false;
	}

	if (fromDamage == true)
	{
		// Dunno if this should flat-out &~ the flag out too.
		// Decided it probably just just keep it since it's "adding" hitlag.
		mo->eflags |= MFE_DAMAGEHITLAG;
	}
}

/*--------------------------------------------------
	static void K_SpawnSingleHitLagSpark(
		mobj_t *parent,
		vector3_t *offset, fixed_t scale,
		UINT8 tics, UINT8 pause,
		skincolornum_t color)

		Spawns a set of damage hitlag spark papersprites.

	Input Arguments:-
		parent - Object that the hitlag was added to.
		offset - Offset from the parent to spawn the spark.
		scale - The scale of the spark.
		tics - Which hitlag state to use.
		pause - How long to wait before it displays.
		color - The color of the damage source.

	Return:-
		N/A
--------------------------------------------------*/
void K_SpawnSingleHitLagSpark(
	mobj_t *parent,
	vector3_t *offset, fixed_t scale,
	UINT8 tics, UINT8 pause,
	skincolornum_t color)
{
	INT32 i;

	if (tics == 0)
	{
		return;
	}

	for (i = 0; i < 2; i++)
	{
		mobj_t *spark = P_SpawnMobj(
			parent->x + offset->x,
			parent->y + offset->y,
			parent->z + offset->z,
			MT_HITLAG
		);
		P_SetMobjState(spark, spark->info->spawnstate + (tics - 1));
		P_SetTarget(&spark->target, parent);

		spark->destscale = scale;
		P_SetScale(spark, scale);

		if (parent->eflags & MFE_VERTICALFLIP)
		{
			spark->eflags |= MFE_VERTICALFLIP;
			spark->flags2 |= MF2_OBJECTFLIP;
			spark->z = parent->z + parent->height - offset->z;
		}

		spark->angle = R_PointToAngle2(
			parent->x, parent->y,
			spark->x, spark->y
		);

		if (i & 1)
		{
			spark->angle += ANGLE_90;
		}

		if (pause > 0)
		{
			spark->hitlag = pause+1; // A little dumb, but hitlag is decremented on the same tic we spawn, so naively setting is off-by-one.
			// spark->renderflags |= RF_DONTDRAW; // "Re-visible" behavior in P_MobjThinker, under a type check. (Currently unused.)
		}

		if (color != SKINCOLOR_NONE)
		{
			spark->color = color;
			spark->colorized = true;
		}

		K_ReduceVFXForEveryone(spark);
	}
}

/*--------------------------------------------------
	static void K_PlayHitLagSFX(mobj_t *victim, UINT8 tics)

		Plays a damage sound for a player.

	Input Arguments:-
		victim - Object getting damaged.
		tics - How long the hitlag was.

	Return:-
		N/A
--------------------------------------------------*/
static void K_PlayHitLagSFX(mobj_t *victim, UINT8 tics)
{
	sfxenum_t soundID = sfx_dmga1;

	if (P_Random(PR_DECORATION) & 1) // might want to use this set for some other scenario, instead of randomized?
	{
		soundID = sfx_dmgb1;
	}

	soundID += Easing_Linear(
		min(FRACUNIT, FRACUNIT*tics/MAXHITLAGTICS),
		0,
		NUM_HITLAG_SOUNDS-1
	);
	S_StartSound(victim, soundID);
}

/*--------------------------------------------------
	static void K_SpawnHitLagEFX(mobj_t *victim, mobj_t *inflictor, mobj_t *source, UINT8 tics)

		Spawns several hitlag sparks for damage.

	Input Arguments:-
		victim - Object getting damaged.
		inflictor - Object touching the victim. May be NULL.
		source - Object that inflictor came from. May be NULL or same as inflictor.
		tics - How long the hitlag was.

	Return:-
		N/A
--------------------------------------------------*/
static void K_SpawnHitLagEFX(mobj_t *victim, mobj_t *inflictor, mobj_t *source, UINT8 tics)
{
	vector3_t offset = { 0, 0, 0 };
	fixed_t newScale = FRACUNIT;
	UINT8 startTics = 0, endTics = 0;
	skincolornum_t color = SKINCOLOR_NONE;

	I_Assert(P_MobjWasRemoved(victim) == false);

	K_PlayHitLagSFX(victim, tics);
	P_StartQuakeFromMobj(tics, tics * 2 * mapobjectscale, 1536 * mapobjectscale, victim);

	if (P_MobjWasRemoved(inflictor) == false)
	{
		offset.x = (inflictor->x - victim->x) / 2;
		offset.y = (inflictor->y - victim->y) / 2;
		offset.z = (P_GetMobjHead(inflictor) - P_GetMobjHead(victim)) / 2;

		newScale = (3 * (victim->destscale + inflictor->destscale) / 2);
	}
	else
	{
		offset.z = victim->height;
		newScale = 3 * victim->destscale;
	}

	if ((gametyperules & GTR_BUMPERS) && battleprisons == false)
	{
		newScale = 3 * newScale / 4;
	}

	if (P_MobjWasRemoved(source) == false)
	{
		color = (source->player != NULL) ? source->player->skincolor : source->color;
	}

	// CONS_Printf("== HITLAG VFX START: endTics %d tics %d ==\n", endTics, tics);

	while (endTics < tics)
	{
		UINT8 particle = max(1, FixedMul((tics * FRACUNIT) + (FRACUNIT/2), FRACUNIT*2/3) / FRACUNIT);

		// CONS_Printf("trying particle %d\n", particle);

		if (particle > NUM_HITLAG_STATES)
		{
			particle = NUM_HITLAG_STATES;
			// CONS_Printf("* corrected to %d (states - %d)\n", particle, NUM_HITLAG_STATES);
		}

		UINT8 ticsLeft = tics - endTics;
		// CONS_Printf("? ticsleft %d\n", ticsLeft);

		if (particle > ticsLeft)
		{
			particle = ticsLeft;
			// CONS_Printf("* corrected to %d (ticsleft - %d)\n", particle, ticsLeft);
		}

		// CONS_Printf("spawning, startTics %d\n", startTics);

		K_SpawnSingleHitLagSpark(victim, &offset, newScale, particle, startTics, color);

		startTics += max(1, FixedMul((particle * FRACUNIT) + (FRACUNIT/2), FRACUNIT/3) / FRACUNIT);
		endTics += particle;

		offset.x += P_RandomRange(PR_DECORATION, -45, 45) * newScale;
		offset.y += P_RandomRange(PR_DECORATION, -45, 45) * newScale;
		offset.z += P_RandomRange(PR_DECORATION, -45, 45) * newScale;

		newScale = (newScale * 2) / 3;

		// CONS_Printf("next: startTics %d, endTics %d, tics %d, newScale %d\n", startTics, endTics, tics, newScale);
	}
}

/*--------------------------------------------------
	void K_SetHitLagForObjects(mobj_t *victim, mobj_t *inflictor, mobj_t *source, INT32 tics, boolean fromDamage)

		See header file for description.
--------------------------------------------------*/
void K_SetHitLagForObjects(mobj_t *victim, mobj_t *inflictor, mobj_t *source, INT32 tics, boolean fromDamage)
{
	INT32 finalTics = tics;

	if (tics <= 0)
	{
		return;
	}

	if (P_MobjWasRemoved(inflictor) == false && inflictor->type == MT_BUBBLESHIELD)
	{
		// Bubble blow-up: hitlag is based on player's speed
		inflictor = source;
	}

	if (P_MobjWasRemoved(victim) == false && P_MobjWasRemoved(inflictor) == false)
	{
		const fixed_t speedTicFactor = (mapobjectscale * 8);
		const INT32 angleTicFactor = ANGLE_22h;

		const fixed_t victimSpeed = FixedHypot(FixedHypot(victim->momx, victim->momy), victim->momz);
		const fixed_t inflictorSpeed = FixedHypot(FixedHypot(inflictor->momx, inflictor->momy), inflictor->momz);
		const fixed_t speedDiff = abs(inflictorSpeed - victimSpeed);

		const fixed_t scaleDiff = abs(inflictor->scale - victim->scale);

		angle_t victimAngle = K_MomentumAngle(victim);
		angle_t inflictorAngle = K_MomentumAngle(inflictor);
		INT32 angleDiff = 0;

		if (victimSpeed > 0 && inflictorSpeed > 0)
		{
			// If either object is completely not moving, their speed doesn't matter.
			angleDiff = AngleDelta(victimAngle, inflictorAngle);
		}

		// Add extra "damage" based on what was happening to the objects on impact.
		finalTics += (FixedMul(speedDiff, FRACUNIT + scaleDiff) / speedTicFactor) + (angleDiff / angleTicFactor);

		// This shouldn't happen anymore, but just in case something funky happens.
		if (finalTics < tics)
		{
			finalTics = tics;
		}
	}

	K_AddHitLag(victim, finalTics, fromDamage);
	K_AddHitLag(inflictor, finalTics, false); // Don't use the damage property.

	if (P_MobjWasRemoved(victim) == false && fromDamage == true)
	{
		K_SpawnHitLagEFX(victim, inflictor, source, finalTics);
	}
}
