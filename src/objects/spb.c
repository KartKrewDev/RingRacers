// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  spb.c
/// \brief Self Propelled Bomb item code.

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../g_game.h"
#include "../z_zone.h"
#include "../k_waypoint.h"
#include "../k_respawn.h"
#include "../k_specialstage.h"

#define SPB_SLIPTIDEDELTA (ANG1 * 3)
#define SPB_STEERDELTA (ANGLE_90 - ANG10)
#define SPB_DEFAULTSPEED (FixedMul(mapobjectscale, K_GetKartSpeedFromStat(9) * 2))
#define SPB_ACTIVEDIST (2048 * FRACUNIT)

#define SPB_HOTPOTATO (2*TICRATE)
#define SPB_MAXSWAPS (2)
#define SPB_FLASHING (TICRATE)

#define SPB_CHASETIMESCALE (60*TICRATE)
#define SPB_CHASETIMEMUL (3*FRACUNIT)

#define SPB_SEEKTURN (FRACUNIT/4)
#define SPB_CHASETURN (FRACUNIT/4)

#define SPB_MANTA_SPACING (2750 * FRACUNIT)

#define SPB_MANTA_VSTART (150)
#define SPB_MANTA_VRATE (60)
#define SPB_MANTA_VMAX (100)

enum
{
	SPB_MODE_SEEK,
	SPB_MODE_CHASE,
	SPB_MODE_WAIT,
};

#define spb_mode(o) ((o)->extravalue1)
#define spb_modetimer(o) ((o)->extravalue2)

#define spb_nothink(o) ((o)->threshold)
#define spb_intangible(o) ((o)->cvmem)

#define spb_lastplayer(o) ((o)->lastlook)
#define spb_speed(o) ((o)->movefactor)
#define spb_pitch(o) ((o)->movedir)

#define spb_chasetime(o) ((o)->watertop) // running out of variables here...
#define spb_swapcount(o) ((o)->health)

#define spb_curwaypoint(o) ((o)->cusval)

#define spb_manta_vscale(o) ((o)->movecount)
#define spb_manta_totaldist(o) ((o)->reactiontime)

#define spb_owner(o) ((o)->target)
#define spb_chase(o) ((o)->tracer)

void Obj_SPBEradicateCapsules(void)
{
	thinker_t *think;
	mobj_t *mo;

	// Expensive operation :D?
	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)think;

		if (mo->type != MT_ITEMCAPSULE)
			continue;

		if (!mo->health || mo->fuse || mo->threshold != KITEM_SPB)
			continue;

		P_KillMobj(mo, NULL, NULL, DMG_NORMAL);
	}
}

void Obj_SPBThrown(mobj_t *spb, fixed_t finalspeed)
{
	spb_speed(spb) = finalspeed;

	Obj_SPBEradicateCapsules();
}

static void SPBMantaRings(mobj_t *spb)
{
	fixed_t vScale = INT32_MAX;
	fixed_t spacing = INT32_MAX;
	fixed_t finalDist = INT32_MAX;

	const fixed_t floatHeight = 24 * spb->scale;
	fixed_t floorDist = INT32_MAX;

	if (modeattacking & ATTACKING_SPB)
		return; // no one else to use 'em

	if (leveltime % SPB_MANTA_VRATE == 0)
	{
		spb_manta_vscale(spb) = max(spb_manta_vscale(spb) - 1, SPB_MANTA_VMAX);
	}

	spacing = FixedMul(SPB_MANTA_SPACING, spb->scale);
	spacing = FixedMul(spacing, K_GetKartGameSpeedScalar(gamespeed));

	vScale = FixedDiv(spb_manta_vscale(spb) * FRACUNIT, 100 * FRACUNIT);
	finalDist = FixedMul(spacing, vScale);

	floorDist = abs(P_GetMobjFeet(spb) - P_GetMobjGround(spb));

	spb_manta_totaldist(spb) += P_AproxDistance(spb->momx, spb->momy);

	if (spb_manta_totaldist(spb) > finalDist
		&& floorDist <= floatHeight)
	{
		spb_manta_totaldist(spb) = 0;

		Obj_MantaRingCreate(
			spb,
			spb_owner(spb),
			cv_spbtest.value ? NULL : spb_chase(spb)
		);
	}
}

static void SpawnSPBDust(mobj_t *spb)
{
	// The easiest way to spawn a V shaped cone of dust from the SPB is simply to spawn 2 particles, and to both move them to the sides in opposite direction.
	mobj_t *dust;
	fixed_t sx;
	fixed_t sy;
	fixed_t sz = spb->floorz;
	angle_t sa = spb->angle - ANG1*60;
	INT32 i;

	if (spb->eflags & MFE_VERTICALFLIP)
	{
		sz = spb->ceilingz;
	}

	if ((leveltime & 1) && abs(spb->z - sz) < FRACUNIT*64) // Only every other frame. Also don't spawn it if we're way above the ground.
	{
		// Determine spawning position next to the SPB:
		for (i = 0; i < 2; i++)
		{
			sx = 96 * FINECOSINE(sa >> ANGLETOFINESHIFT);
			sy = 96 * FINESINE(sa >> ANGLETOFINESHIFT);

			dust = P_SpawnMobjFromMobj(spb, sx, sy, 0, MT_SPBDUST);
			dust->z = sz;

			dust->momx = spb->momx/2;
			dust->momy = spb->momy/2;
			dust->momz = spb->momz/2; // Give some of the momentum to the dust

			P_SetScale(dust, spb->scale * 2);

			dust->color = SKINCOLOR_RED;
			dust->colorized = true;

			dust->angle = spb->angle - FixedAngle(FRACUNIT*90 - FRACUNIT*180*i); // The first one will spawn to the right of the spb, the second one to the left.
			P_Thrust(dust, dust->angle, 6*dust->scale);

			K_MatchGenericExtraFlagsNoZAdjust(dust, spb);

			sa += ANG1*120;	// Add 120 degrees to get to mo->angle + ANG1*60
		}
	}
}

// Spawns SPB slip tide. To be used when the SPB is turning.
// Modified version of K_SpawnAIZDust. Maybe we could merge those to be cleaner?

// dir should be either 1 or -1 to determine where to spawn the dust.

static void SpawnSPBSliptide(mobj_t *spb, SINT8 dir)
{
	fixed_t newx;
	fixed_t newy;
	mobj_t *spark;
	angle_t travelangle;
	fixed_t sz = spb->floorz;

		if (spb->eflags & MFE_VERTICALFLIP)
		{
			sz = spb->ceilingz;
		}

		travelangle = K_MomentumAngle(spb);

		if ((leveltime & 1) && abs(spb->z - sz) < FRACUNIT*64)
		{
			newx = P_ReturnThrustX(spb, travelangle - (dir*ANGLE_45), 24*FRACUNIT);
			newy = P_ReturnThrustY(spb, travelangle - (dir*ANGLE_45), 24*FRACUNIT);

			spark = P_SpawnMobjFromMobj(spb, newx, newy, 0, MT_SPBDUST);
			spark->z = sz;

			P_SetMobjState(spark, S_KARTAIZDRIFTSTRAT);
			P_SetTarget(&spark->target, spb);

			spark->colorized = true;
			spark->color = SKINCOLOR_RED;

			spark->angle = travelangle + (dir * ANGLE_90);
			P_SetScale(spark, (spark->destscale = spb->scale*3/2));

			spark->momx = (6*spb->momx)/5;
			spark->momy = (6*spb->momy)/5;

			K_MatchGenericExtraFlagsNoZAdjust(spark, spb);
		}
	}

// Used for seeking and when SPB is trailing its target from way too close!
static void SpawnSPBSpeedLines(mobj_t *spb)
{
	// note: determinate random argument eval order
	fixed_t rand_z = P_RandomRange(PR_DECORATION, -24, 24);
	fixed_t rand_y = P_RandomRange(PR_DECORATION, -24, 24);
	fixed_t rand_x = P_RandomRange(PR_DECORATION, -24, 24);
	mobj_t *fast = P_SpawnMobjFromMobj(spb,
		rand_x * FRACUNIT,
		rand_y * FRACUNIT,
		(spb->info->height / 2) + (rand_z * FRACUNIT),
		MT_FASTLINE
	);

	P_SetTarget(&fast->target, spb);
	fast->angle = K_MomentumAngle(spb);

	fast->color = SKINCOLOR_RED;
	fast->colorized = true;

	K_MatchGenericExtraFlagsNoZAdjust(fast, spb);
}

static fixed_t SPBDist(mobj_t *a, mobj_t *b)
{
	return P_AproxDistance(P_AproxDistance(
		a->x - b->x,
		a->y - b->y),
		a->z - b->z
	);
}

static void SPBTurn(
	fixed_t destSpeed, angle_t destAngle,
	fixed_t *editSpeed, angle_t *editAngle,
	fixed_t lerp, SINT8 *returnSliptide)
{
	INT32 delta = AngleDeltaSigned(destAngle, *editAngle);
	fixed_t dampen = FRACUNIT;

	// Slow down when turning; it looks better and makes U-turns not unfair
	dampen = FixedDiv((180 * FRACUNIT) - AngleFixed(abs(delta)), 180 * FRACUNIT);
	*editSpeed = FixedMul(destSpeed, dampen);

	delta = FixedMul(delta, lerp);

	// Calculate sliptide effect during seeking.
	if (returnSliptide != NULL)
	{
		const boolean isSliptiding =  (abs(delta) >= SPB_SLIPTIDEDELTA);
		SINT8 sliptide = 0;

		if (isSliptiding == true)
		{
			if (delta < 0)
			{
				sliptide = -1;
			}
			else
			{
				sliptide = 1;
			}
		}

		*returnSliptide = sliptide;
	}

	*editAngle += delta;
}

static void SetSPBSpeed(mobj_t *spb, fixed_t xySpeed, fixed_t zSpeed)
{
	spb->momx = FixedMul(FixedMul(
		xySpeed,
		FINECOSINE(spb->angle >> ANGLETOFINESHIFT)),
		FINECOSINE(spb_pitch(spb) >> ANGLETOFINESHIFT)
	);

	spb->momy = FixedMul(FixedMul(
		xySpeed,
		FINESINE(spb->angle >> ANGLETOFINESHIFT)),
		FINECOSINE(spb_pitch(spb) >> ANGLETOFINESHIFT)
	);

	spb->momz = FixedMul(
		zSpeed,
		FINESINE(spb_pitch(spb) >> ANGLETOFINESHIFT)
	);
}

static boolean SPBSeekSoundPlaying(mobj_t *spb)
{
	return (S_SoundPlaying(spb, sfx_spbska)
		|| S_SoundPlaying(spb, sfx_spbskb)
		|| S_SoundPlaying(spb, sfx_spbskc));
}

static void SPBSeek(mobj_t *spb, mobj_t *bestMobj)
{
	const fixed_t desiredSpeed = SPB_DEFAULTSPEED*(2); // Seeks the player out 2x faster than its usual speed when locked in

	waypoint_t *curWaypoint = NULL;
	waypoint_t *destWaypoint = NULL;

	fixed_t dist = INT32_MAX;
	fixed_t activeDist = INT32_MAX;

	fixed_t destX = spb->x;
	fixed_t destY = spb->y;
	fixed_t destZ = spb->z;
	angle_t destAngle = spb->angle;
	angle_t destPitch = 0U;

	fixed_t xySpeed = desiredSpeed;
	fixed_t zSpeed = desiredSpeed;
	SINT8 sliptide = 0;

	fixed_t steerDist = INT32_MAX;
	mobj_t *steerMobj = NULL;

	boolean circling = false;

	size_t i;

	spb_lastplayer(spb) = -1; // Just make sure this is reset

	if (bestMobj == NULL
		|| P_MobjWasRemoved(bestMobj) == true
		|| bestMobj->health <= 0
		|| (bestMobj->player != NULL && bestMobj->player->respawn.state != RESPAWNST_NONE))
	{
		// No one there? Completely STOP.
		spb->momx = spb->momy = spb->momz = 0;

		if (bestMobj == NULL)
		{
			spbplace = -1;
		}

		return;
	}

	// Found someone, now get close enough to initiate the slaughter...
	P_SetTarget(&spb_chase(spb), bestMobj);

	if (bestMobj->player != NULL)
	{
		spbplace = bestMobj->player->position;
	}
	else
	{
		spbplace = 1;
	}

	dist = SPBDist(spb, spb_chase(spb));
	activeDist = FixedMul(SPB_ACTIVEDIST, spb_chase(spb)->scale);

	if (spb_swapcount(spb) > SPB_MAXSWAPS + 1)
	{
		// Too much hot potato.
		// Go past our target and explode instead.
		if (spb->fuse == 0)
		{
			spb_intangible(spb) = SPB_FLASHING;
			spb->fuse = 2*TICRATE;
		}
	}
	else if (!cv_spbtest.value)
	{
		if (dist <= activeDist)
		{
			S_StopSound(spb);
			S_StartSound(spb, spb->info->attacksound);

			spb_mode(spb) = SPB_MODE_CHASE; // TARGET ACQUIRED
			spb_swapcount(spb)++;

			spb_modetimer(spb) = SPB_HOTPOTATO;
			spb_intangible(spb) = SPB_FLASHING;

			spb_speed(spb) = desiredSpeed;
			return;
		}
	}

	if (SPBSeekSoundPlaying(spb) == false)
	{
		if (dist <= activeDist * 3)
		{
			S_StartSound(spb, sfx_spbskc);
		}
		else if (dist <= activeDist * 6)
		{
			S_StartSound(spb, sfx_spbskb);
		}
		else
		{
			S_StartSound(spb, sfx_spbska);
		}
	}

	// Move along the waypoints until you get close enough
	if (spb_curwaypoint(spb) == -1)
	{
		// Determine first waypoint.
		curWaypoint = K_GetBestWaypointForMobj(spb, NULL);
		spb_curwaypoint(spb) = (INT32)K_GetWaypointHeapIndex(curWaypoint);
	}
	else
	{
		curWaypoint = K_GetWaypointFromIndex( (size_t)spb_curwaypoint(spb) );
	}

	if (bestMobj->player != NULL)
	{
		destWaypoint = bestMobj->player->nextwaypoint;
	}
	else if (bestMobj->type == MT_SPECIAL_UFO)
	{
		destWaypoint = K_GetSpecialUFOWaypoint(bestMobj);
	}
	else
	{
		destWaypoint = K_GetBestWaypointForMobj(bestMobj, NULL);
	}

	if (curWaypoint != NULL)
	{
		fixed_t waypointDist = INT32_MAX;
		fixed_t waypointRad = INT32_MAX;

		destX = curWaypoint->mobj->x;
		destY = curWaypoint->mobj->y;
		destZ = curWaypoint->mobj->z;

		waypointDist = R_PointToDist2(spb->x, spb->y, destX, destY) / mapobjectscale;
		waypointRad = max(curWaypoint->mobj->radius / mapobjectscale, DEFAULT_WAYPOINT_RADIUS);

		if (waypointDist <= waypointRad)
		{
			boolean pathfindsuccess = false;

			if (destWaypoint != NULL)
			{
				// Go to next waypoint.
				const boolean useshortcuts  = K_GetWaypointIsShortcut(destWaypoint); // If the player is on a shortcut, use shortcuts. No escape.
				boolean huntbackwards = false;
				path_t pathtoplayer = {0};

				pathfindsuccess = K_PathfindToWaypoint(
					curWaypoint, destWaypoint,
					&pathtoplayer,
					useshortcuts, huntbackwards
				);

				if (pathfindsuccess == true)
				{
					if (cv_spbtest.value)
					{
						if (pathtoplayer.numnodes > 1)
						{
							// Go to the next waypoint.
							curWaypoint = (waypoint_t *)pathtoplayer.array[1].nodedata;
						}
						else if (destWaypoint->numnextwaypoints > 0)
						{
							// Run ahead.
							curWaypoint = destWaypoint->nextwaypoints[0];
						}
						else
						{
							// Sort of wait at the player's dest waypoint.
							circling = true;
							curWaypoint = destWaypoint;
						}
					}
					else
					{
						path_t reversepath = {0};
						boolean reversesuccess = false;

						huntbackwards = true;
						reversesuccess = K_PathfindToWaypoint(
							curWaypoint, destWaypoint,
							&reversepath,
							useshortcuts, huntbackwards
						);

						if (reversesuccess == true
							&& reversepath.totaldist < pathtoplayer.totaldist)
						{
							// It's faster to go backwards than to chase forward.
							// Keep curWaypoint the same, so the SPB waits around for them.
							circling = true;
						}
						else if (pathtoplayer.numnodes > 1)
						{
							// Go to the next waypoint.
							curWaypoint = (waypoint_t *)pathtoplayer.array[1].nodedata;
						}
						else if (spb->fuse > 0 && destWaypoint->numnextwaypoints > 0)
						{
							// Run ahead.
							curWaypoint = destWaypoint->nextwaypoints[0];
						}
						else
						{
							// Sort of wait at the player's dest waypoint.
							circling = true;
							curWaypoint = destWaypoint;
						}

						if (reversesuccess == true)
						{
							Z_Free(reversepath.array);
						}
					}
					Z_Free(pathtoplayer.array);
				}
			}

			if (pathfindsuccess == true && curWaypoint != NULL)
			{
				// Update again
				spb_curwaypoint(spb) = (INT32)K_GetWaypointHeapIndex(curWaypoint);
				destX = curWaypoint->mobj->x;
				destY = curWaypoint->mobj->y;
				destZ = curWaypoint->mobj->z;
			}
			else
			{
				spb_curwaypoint(spb) = -1;
				destX = spb_chase(spb)->x;
				destY = spb_chase(spb)->y;
				destZ = spb_chase(spb)->z;
			}
		}
	}
	else
	{
		spb_curwaypoint(spb) = -1;
		destX = spb_chase(spb)->x;
		destY = spb_chase(spb)->y;
		destZ = spb_chase(spb)->z;
	}

	destAngle = R_PointToAngle2(spb->x, spb->y, destX, destY);
	destPitch = R_PointToAngle2(0, spb->z, P_AproxDistance(spb->x - destX, spb->y - destY), destZ);

	SPBTurn(desiredSpeed, destAngle, &xySpeed, &spb->angle, SPB_SEEKTURN, &sliptide);
	SPBTurn(desiredSpeed, destPitch, &zSpeed, &spb_pitch(spb), SPB_SEEKTURN, NULL);

	SetSPBSpeed(spb, xySpeed, zSpeed);

	if (specialstageinfo.valid == false)
	{
		// see if a player is near us, if they are, try to hit them by slightly thrusting towards them, otherwise, bleh!
		steerDist = 1536 * mapobjectscale;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			fixed_t ourDist = INT32_MAX;
			INT32 ourDelta = INT32_MAX;

			if (playeringame[i] == false || players[i].spectator == true)
			{
				// Not in-game
				continue;
			}

			if (players[i].mo == NULL || P_MobjWasRemoved(players[i].mo) == true)
			{
				// Invalid mobj
				continue;
			}

			ourDelta = AngleDelta(spb->angle, R_PointToAngle2(spb->x, spb->y, players[i].mo->x, players[i].mo->y));
			if (ourDelta > SPB_STEERDELTA)
			{
				// Check if the angle wouldn't make us LOSE speed.
				continue;
			}

			ourDist = R_PointToDist2(spb->x, spb->y, players[i].mo->x, players[i].mo->y);
			if (ourDist < steerDist)
			{
				steerDist = ourDist;
				steerMobj = players[i].mo; // it doesn't matter if we override this guy now.
			}
		}

		// different player from our main target, try and ram into em~!
		if (steerMobj != NULL && steerMobj != spb_chase(spb))
		{
			P_Thrust(spb, R_PointToAngle2(spb->x, spb->y, steerMobj->x, steerMobj->y), spb_speed(spb) / 4);
		}
	}

	//CONS_Printf("%d: leveltime %d: SPB intangibility %d: SPBModeTimer\n", leveltime, spb_intangible(spb), spb_modetimer(spb));

	// Tired of this thing whacking people when switching targets.
	// I'm pretty sure checking mode timer doesn't work but, idk insurance!!

	if (spb_intangible(spb) <= 0 || (spb_modetimer(spb) > 0))
	 {
		if (sliptide != 0)
		{
			// 1 if turning left, -1 if turning right.
			// Angles work counterclockwise, remember!
			SpawnSPBSliptide(spb, sliptide);
		}
		else
		{
			// if we're mostly going straight, then spawn the V dust cone!
			SpawnSPBDust(spb);
		}
	}

	// Always spawn speed lines while seeking
	SpawnSPBSpeedLines(spb);

	// Don't run this while we're circling around one waypoint intentionally.
	if (circling == false)
	{
		// Spawn a trail of rings behind the SPB!
		SPBMantaRings(spb);
	}
}

static void SPBChase(mobj_t *spb, mobj_t *bestMobj)
{
	fixed_t baseSpeed = 0;
	fixed_t maxSpeed = 0;
	fixed_t desiredSpeed = 0;

	fixed_t range = INT32_MAX;
	fixed_t cx = 0, cy = 0;

	fixed_t dist = INT32_MAX;
	angle_t destAngle = spb->angle;
	angle_t destPitch = 0U;
	fixed_t xySpeed = 0;
	fixed_t zSpeed = 0;

	mobj_t *chase = NULL;
	player_t *chasePlayer = NULL;

	spb_curwaypoint(spb) = -1; // Reset waypoint

	chase = spb_chase(spb);

	if (chase == NULL || P_MobjWasRemoved(chase) == true || chase->health <= 0)
	{
		P_SetTarget(&spb_chase(spb), NULL);
		spb_mode(spb) = SPB_MODE_WAIT;
		spb_modetimer(spb) = 55; // Slightly over the respawn timer length
		return;
	}

	if (chase->hitlag > 0)
	{
		// If the player is frozen, the SPB should be too.
		spb->hitlag = max(spb->hitlag, chase->hitlag);
		return;
	}

	// Increment chase time
	spb_chasetime(spb)++;

	baseSpeed = SPB_DEFAULTSPEED;
	range = (160 * chase->scale);
	range = max(range, FixedMul(range, K_GetKartGameSpeedScalar(gamespeed)));

	// Play the intimidating gurgle
	if (S_SoundPlaying(spb, spb->info->activesound) == false)
	{
		S_StartSound(spb, spb->info->activesound);
	}

	dist = P_AproxDistance(P_AproxDistance(spb->x - chase->x, spb->y - chase->y), spb->z - chase->z);

	chasePlayer = chase->player;

	if (chasePlayer != NULL)
	{
		UINT8 fracmax = 32;
		UINT8 spark = ((10 - chasePlayer->kartspeed) + chasePlayer->kartweight) / 2;
		fixed_t easiness = ((chasePlayer->kartspeed + (10 - spark)) << FRACBITS) / 2;

		fixed_t scaleAdjust = FRACUNIT;
		if (chase->scale > mapobjectscale)
			scaleAdjust = GROW_PHYSICS_SCALE;
		if (chase->scale < mapobjectscale)
			scaleAdjust = SHRINK_PHYSICS_SCALE;

		spb_lastplayer(spb) = chasePlayer - players; // Save the player num for death scumming...
		spbplace = chasePlayer->position;

		chasePlayer->pflags |= PF_RINGLOCK; // set ring lock

		if (P_IsObjectOnGround(chase) == false)
		{
			// In the air you have no control; basically don't hit unless you make a near complete stop
			baseSpeed = (7 * chasePlayer->speed) / 8;
		}
		else
		{
			// 7/8ths max speed for Knuckles, 3/4ths max speed for min accel, exactly max speed for max accel
			baseSpeed = FixedMul(
				((fracmax+1) << FRACBITS) - easiness,
				FixedMul(K_GetKartSpeed(chasePlayer, false, false), scaleAdjust)
			) / fracmax;
		}

		if (chasePlayer->carry == CR_SLIDING)
		{
			baseSpeed = chasePlayer->speed/2;
		}

		// Be fairer on conveyors
		cx = chasePlayer->cmomx;
		cy = chasePlayer->cmomy;

		// Switch targets if you're no longer 1st for long enough
		if (bestMobj != NULL
			&& (bestMobj->player == NULL || chasePlayer->position <= bestMobj->player->position))
		{
			spb_modetimer(spb) = SPB_HOTPOTATO;
		}
		else
		{
			if (spb_modetimer(spb) > 0)
			{
				spb_modetimer(spb)--;
			}

			if (spb_modetimer(spb) <= 0)
			{
				spb_mode(spb) = SPB_MODE_SEEK; // back to SEEKING
				spb_intangible(spb) = SPB_FLASHING;
			}
		}

		chasePlayer->SPBdistance = dist;
	}
	else
	{
		spb_lastplayer(spb) = -1;
		spbplace = 1;
		spb_modetimer(spb) = SPB_HOTPOTATO;
	}

	desiredSpeed = FixedMul(baseSpeed, FRACUNIT + FixedDiv(dist - range, range));

	if (desiredSpeed < baseSpeed)
	{
		desiredSpeed = baseSpeed;
	}

	maxSpeed = (baseSpeed * 3) / 2;
	if (desiredSpeed > maxSpeed)
	{
		desiredSpeed = maxSpeed;
	}

	if (desiredSpeed < 20 * chase->scale)
	{
		desiredSpeed = 20 * chase->scale;
	}

	if (chasePlayer != NULL)
	{
		if (chasePlayer->carry == CR_SLIDING)
		{
			// Hack for current sections to make them fair.
			desiredSpeed = min(desiredSpeed, chasePlayer->speed / 2);
		}

		const mobj_t *waypoint = chasePlayer->currentwaypoint ? chasePlayer->currentwaypoint->mobj : NULL;
		// thing_args[3]: SPB speed (0-100)
		if (waypoint && waypoint->thing_args[3]) // 0 = default speed (unchanged)
		{
			desiredSpeed = desiredSpeed * waypoint->thing_args[3] / 100;
		}
	}

	destAngle = R_PointToAngle2(spb->x, spb->y, chase->x, chase->y);
	destPitch = R_PointToAngle2(0, spb->z, P_AproxDistance(spb->x - chase->x, spb->y - chase->y), chase->z);

	// Modify stored speed
	if (desiredSpeed > spb_speed(spb))
	{
		spb_speed(spb) += (desiredSpeed - spb_speed(spb)) / TICRATE;
	}
	else
	{
		spb_speed(spb) = desiredSpeed;
	}

	SPBTurn(spb_speed(spb), destAngle, &xySpeed, &spb->angle, SPB_CHASETURN, NULL);
	SPBTurn(spb_speed(spb), destPitch, &zSpeed, &spb_pitch(spb), SPB_CHASETURN, NULL);

	SetSPBSpeed(spb, xySpeed, zSpeed);
	spb->momx += cx;
	spb->momy += cy;

	// Spawn a trail of rings behind the SPB!
	SPBMantaRings(spb);

	// Red speed lines for when it's gaining on its target. A tell for when you're starting to lose too much speed!
	if (R_PointToDist2(0, 0, spb->momx, spb->momy) > (16 * R_PointToDist2(0, 0, chase->momx, chase->momy)) / 15 // Going faster than the target
		&& xySpeed > 20 * mapobjectscale) // Don't display speedup lines at pitifully low speeds
	{
		SpawnSPBSpeedLines(spb);
	}
}

static void SPBWait(mobj_t *spb)
{
	player_t *oldPlayer = NULL;

	spb->momx = spb->momy = spb->momz = 0; // Stoooop
	spb_curwaypoint(spb) = -1; // Reset waypoint

	if (spb_lastplayer(spb) != -1
		&& playeringame[spb_lastplayer(spb)] == true)
	{
		oldPlayer = &players[spb_lastplayer(spb)];
	}

	if (oldPlayer != NULL
		&& oldPlayer->spectator == false
		&& oldPlayer->exiting > 0)
	{
		spbplace = oldPlayer->position;
		oldPlayer->pflags |= PF_RINGLOCK;
	}

	if (spb_modetimer(spb) > 0)
	{
		spb_modetimer(spb)--;
	}

	if (spb_modetimer(spb) <= 0)
	{
		if (oldPlayer != NULL)
		{
			if (oldPlayer->mo != NULL && P_MobjWasRemoved(oldPlayer->mo) == false)
			{
				P_SetTarget(&spb_chase(spb), oldPlayer->mo);
				spb_mode(spb) = SPB_MODE_CHASE;
				spb_modetimer(spb) = SPB_HOTPOTATO;
				spb_intangible(spb) = SPB_FLASHING;
				spb_speed(spb) = SPB_DEFAULTSPEED;
			}
		}
		else
		{
			spb_mode(spb) = SPB_MODE_SEEK;
			spb_modetimer(spb) = 0;
			spb_intangible(spb) = SPB_FLASHING;
			spbplace = -1;
		}
	}
}

void Obj_SPBThink(mobj_t *spb)
{
	mobj_t *ghost = NULL;
	mobj_t *bestMobj = NULL;
	UINT8 bestRank = UINT8_MAX;
	size_t i;

	if (spb->health <= 0)
	{
		return;
	}

	K_SetItemCooldown(KITEM_SPB, 20*TICRATE);

	ghost = P_SpawnGhostMobj(spb);
	ghost->fuse = 3;

	if (spb_owner(spb) != NULL && P_MobjWasRemoved(spb_owner(spb)) == false && spb_owner(spb)->player != NULL)
	{
		ghost->color = spb_owner(spb)->player->skincolor;
		ghost->colorized = true;
	}

	if (spb_nothink(spb) <= 1)
	{
		if (specialstageinfo.valid == true)
		{
			bestRank = 0;

			if ((bestMobj = K_GetPossibleSpecialTarget()) == NULL)
			{
				// experimental - I think it's interesting IMO
				Obj_MantaRingCreate(
					spb,
					spb_owner(spb),
					NULL
				);

				spb->fuse = TICRATE/3;
				spb_nothink(spb) = spb->fuse + 2;
			}
		}
	}

	if (spb_nothink(spb) > 0)
	{
		// Init values, don't think yet.
		spb_lastplayer(spb) = -1;
		spb_curwaypoint(spb) = -1;
		spb_chasetime(spb) = 0;
		spbplace = -1;

		spb_manta_totaldist(spb) = 0; // 30000?
		spb_manta_vscale(spb) = SPB_MANTA_VSTART;

		P_InstaThrust(spb, spb->angle, SPB_DEFAULTSPEED);

		spb_nothink(spb)--;
	}
	else
	{
		// Find the player with the best rank
		for (i = 0; i < MAXPLAYERS; i++)
		{
			player_t *player = NULL;

			if (playeringame[i] == false)
			{
				// Not valid
				continue;
			}

			player = &players[i];

			if (player->spectator == true || player->exiting > 0)
			{
				// Not playing
				continue;
			}

			/*
			if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
			{
				// No mobj
				continue;
			}

			if (player->mo <= 0)
			{
				// Dead
				continue;
			}

			if (player->respawn.state != RESPAWNST_NONE)
			{
				// Respawning
				continue;
			}
			*/

			if (player->position < bestRank)
			{
				bestRank = player->position;
				bestMobj = player->mo;
			}
		}

		switch (spb_mode(spb))
		{
			case SPB_MODE_SEEK:
			default:
				SPBSeek(spb, bestMobj);
				break;

			case SPB_MODE_CHASE:
				SPBChase(spb, bestMobj);
				break;

			case SPB_MODE_WAIT:
				SPBWait(spb);
				break;
		}
	}

	// Flash on/off when intangible.
	if (spb_intangible(spb) > 0)
	{
		spb_intangible(spb)--;

		if (spb_intangible(spb) & 1)
		{
			spb->renderflags |= RF_DONTDRAW;
		}
		else
		{
			spb->renderflags &= ~RF_DONTDRAW;
		}
	}

	// Flash white when about to explode!
	if (spb->fuse > 0)
	{
		if (spb->fuse & 1)
		{
			spb->color = SKINCOLOR_INVINCFLASH;
			spb->colorized = true;
		}
		else
		{
			spb->color = SKINCOLOR_NONE;
			spb->colorized = false;
		}
		spb_intangible(spb) = SPB_FLASHING; // This is supposed to make it intangible when it's about to quit
	}

	// Clamp within level boundaries.
	if (spb->z < spb->floorz)
	{
		spb->z = spb->floorz;
	}
	else if (spb->z > spb->ceilingz - spb->height)
	{
		spb->z = spb->ceilingz - spb->height;
	}
}

void Obj_SPBExplode(mobj_t *spb)
{
	mobj_t *spbExplode = NULL;

	// Don't continue playing the gurgle or the siren
	S_StopSound(spb);

	spbExplode = P_SpawnMobjFromMobj(spb, 0, 0, 0, MT_SPBEXPLOSION);

	if (spb_owner(spb) != NULL && P_MobjWasRemoved(spb_owner(spb)) == false)
	{
		P_SetTarget(&spbExplode->target, spb_owner(spb));
	}

	spbExplode->threshold = KITEM_SPB;

	// Tell the explosion to use alternate knockback.
	spbExplode->movefactor = ((SPB_CHASETIMESCALE - spb_chasetime(spb)) * SPB_CHASETIMEMUL) / SPB_CHASETIMESCALE;

	P_RemoveMobj(spb);
}

void Obj_SPBTouch(mobj_t *spb, mobj_t *toucher)
{
	player_t *const player = toucher->player;
	mobj_t *owner = NULL;
	mobj_t *chase = NULL;

	if (spb_intangible(spb) > 0)
	{
		return;
	}

	owner = spb_owner(spb);

	if ((owner == toucher || owner == toucher->target)
		&& (spb_nothink(spb) > 0))
	{
		return;
	}

	if (spb->health <= 0 || toucher->health <= 0)
	{
		return;
	}

	if (player != NULL)
	{
		if (player->spectator == true)
		{
			return;
		}

		if (player->bubbleblowup > 0)
		{
			// Stun the SPB, and remove the shield.
			K_PopPlayerShield(player);
			K_DropHnextList(player);
			spb_mode(spb) = SPB_MODE_WAIT;
			spb_modetimer(spb) = 55; // Slightly over the respawn timer length
			return;
		}
	}

	chase = spb_chase(spb);
	if (chase != NULL && P_MobjWasRemoved(chase) == false
		&& toucher == chase)
	{
		// Cause the explosion.
		Obj_SPBExplode(spb);
		return;
	}
	else if (toucher->flags & MF_SHOOTABLE)
	{
		// Regular spinout, please.
		P_DamageMobj(toucher, spb, owner, 1, DMG_NORMAL);
	}
}
