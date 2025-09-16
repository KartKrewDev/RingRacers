// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ufo.c
/// \brief Special Stage UFO + Emerald handler

#include "../command.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../m_cond.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../g_game.h"
#include "../z_zone.h"
#include "../k_waypoint.h"
#include "../k_specialstage.h"
#include "../r_skins.h"
#include "../k_hitlag.h"
#include "../acs/interface.h"
#include "../hu_stuff.h"
#include "../k_grandprix.h"

#define UFO_BASE_SPEED (42 * FRACUNIT) // UFO's slowest speed.
#define UFO_SPEEDUP (FRACUNIT >> 1) // Acceleration
#define UFO_SLOWDOWN (FRACUNIT >> 1) // Deceleration
#define UFO_SPACING (768 * FRACUNIT) // How far the UFO wants to stay in front
#define UFO_DEADZONE (2048 * FRACUNIT) // Deadzone where it won't update it's speed as much.
#define UFO_SPEEDFACTOR (FRACUNIT * 3 / 4) // Factor of player's best speed, to make it more fair.
#define UFO_DAMAGED_SPEED (UFO_BASE_SPEED >> 1) // Speed to add when UFO takes damage.
#define UFO_START_SPEED (UFO_BASE_SPEED << 1) // Speed when the map starts.

#define UFO_PITY_DIST (10000) // Let's aim for an exciting finish! Try to stick closer to the player once they're past this threshold.
#define UFO_PITY_BRAKES (600 * FRACUNIT) // Subtract this amount from UFO_SFACING, starting at UFO_PITY_DIST and ending at the finish line.

#define UFO_NUMARMS (3)
#define UFO_ARMDELTA (ANGLE_MAX / UFO_NUMARMS)
#define UFO_START_GLASSFRAMES (1)
#define UFO_NUM_GLASSFRAMES (10)

#define ufo_emeraldnum(o) ((o)->cvmem)
#define ufo_waypoint(o) ((o)->extravalue1)
#define ufo_distancetofinish(o) ((o)->extravalue2)
#define ufo_speed(o) ((o)->watertop)
#define ufo_collectdelay(o) ((o)->threshold)

#define ufo_pieces(o) ((o)->hnext)

#define ufo_piece_type(o) ((o)->extravalue1)

#define ufo_piece_glass_flickerframe(o) ((o)->cusval)

#define ufo_piece_owner(o) ((o)->target)
#define ufo_piece_next(o) ((o)->hnext)
#define ufo_piece_prev(o) ((o)->hprev)

#define ufo_intangible(o) ((o)->cusval)

#define ufo_emerald(o) ((o)->tracer)

enum
{
	UFO_PIECE_TYPE_POD,
	UFO_PIECE_TYPE_GLASS,
	UFO_PIECE_TYPE_GLASS_UNDER,
	UFO_PIECE_TYPE_ARM,
	UFO_PIECE_TYPE_STEM,
};

static sfxenum_t hums[16] = {sfx_claw01, sfx_claw02, sfx_claw03, sfx_claw04, sfx_claw05, sfx_claw06, sfx_claw07, sfx_claw08, sfx_claw09, sfx_claw10, sfx_claw11, sfx_claw12, sfx_claw13, sfx_claw14, sfx_claw15, sfx_claw16};
static int maxhum = sizeof(hums) / sizeof(hums[0]) - 1;

static void SpawnUFOSpeedLines(mobj_t *ufo)
{
	// note: determinate random argument eval order
	fixed_t rand_z = P_RandomRange(PR_DECORATION, -24, 24);
	fixed_t rand_y = P_RandomRange(PR_DECORATION, -120, 120);
	fixed_t rand_x = P_RandomRange(PR_DECORATION, -120, 120);
	mobj_t *fast = P_SpawnMobjFromMobj(ufo,
		rand_x * FRACUNIT,
		rand_y * FRACUNIT,
		(ufo->info->height / 2) + (rand_z * FRACUNIT),
		MT_FASTLINE
	);

	fast->scale *= 3;

	P_SetTarget(&fast->target, ufo);
	fast->angle = K_MomentumAngle(ufo);

	fast->color = SKINCOLOR_WHITE;
	fast->colorized = true;

	K_MatchGenericExtraFlagsNoZAdjust(fast, ufo);
}

static void SpawnEmeraldSpeedLines(mobj_t *mo)
{
	// note: determinate random argument eval order
	fixed_t rand_z = P_RandomRange(PR_DECORATION, 0, 64);
	fixed_t rand_y = P_RandomRange(PR_DECORATION, -48, 48);
	fixed_t rand_x = P_RandomRange(PR_DECORATION, -48, 48);
	mobj_t *fast = P_SpawnMobjFromMobj(mo,
		rand_x * FRACUNIT,
		rand_y * FRACUNIT,
		rand_z * FRACUNIT,
		MT_FASTLINE);
	P_SetMobjState(fast, S_KARTINVLINES1);

	P_SetTarget(&fast->target, mo);
	fast->angle = K_MomentumAngle(mo);

	fast->momx = 3*mo->momx/4;
	fast->momy = 3*mo->momy/4;
	fast->momz = 3*P_GetMobjZMovement(mo)/4;

	K_MatchGenericExtraFlagsNoZAdjust(fast, mo);
	P_SetTarget(&fast->owner, mo);
	fast->renderflags |= RF_REDUCEVFX;

	fast->color = mo->color;
	fast->colorized = true;
}

static void UFOMoveTo(mobj_t *ufo, fixed_t destx, fixed_t desty, fixed_t destz)
{
	ufo->momx = destx - ufo->x;
	ufo->momy = desty - ufo->y;
	ufo->momz = destz - ufo->z;
}

static fixed_t GenericDistance(
	fixed_t curx, fixed_t cury, fixed_t curz,
	fixed_t destx, fixed_t desty, fixed_t destz)
{
	return P_AproxDistance(P_AproxDistance(destx - curx, desty - cury), destz - curz);
}

static boolean UFOEmeraldChase(mobj_t *ufo)
{
	return (ufo->health <= 1);
}

static boolean UFOPieceValid(mobj_t *piece)
{
	return (piece != NULL && P_MobjWasRemoved(piece) == false && piece->health > 0);
}

static void UFOUpdateDistanceToFinish(mobj_t *ufo)
{
	waypoint_t *finishLine = K_GetFinishLineWaypoint();
	waypoint_t *nextWaypoint = K_GetWaypointFromIndex((size_t)ufo_waypoint(ufo));

	if (nextWaypoint != NULL && finishLine != NULL)
	{
		const boolean useshortcuts = false;
		const boolean huntbackwards = false;
		boolean pathfindsuccess = false;
		path_t pathtofinish = {0};

		pathfindsuccess =
			K_PathfindToWaypoint(nextWaypoint, finishLine, &pathtofinish, useshortcuts, huntbackwards);

		// Update the UFO's distance to the finish line if a path was found.
		if (pathfindsuccess == true)
		{
			// Add euclidean distance to the next waypoint to the distancetofinish
			UINT32 adddist;
			fixed_t disttowaypoint =
				P_AproxDistance(
					(ufo->x >> FRACBITS) - (nextWaypoint->mobj->x >> FRACBITS),
					(ufo->y >> FRACBITS) - (nextWaypoint->mobj->y >> FRACBITS));
			disttowaypoint = P_AproxDistance(disttowaypoint, (ufo->z >> FRACBITS) - (nextWaypoint->mobj->z >> FRACBITS));

			adddist = (UINT32)disttowaypoint;

			ufo_distancetofinish(ufo) = pathtofinish.totaldist + adddist;
			Z_Free(pathtofinish.array);
		}
	}
}

static void UFOUpdateSpeed(mobj_t *ufo)
{
	const fixed_t mapspeedscale = FixedMul(mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	const fixed_t baseSpeed = FixedMul(UFO_BASE_SPEED, K_GetKartGameSpeedScalar(gamespeed));
	const UINT32 deadzone = FixedMul(UFO_DEADZONE, mapspeedscale) >> FRACBITS;

	UINT32 spacing = FixedMul(UFO_SPACING, mapspeedscale) >> FRACBITS;
	UINT32 distanceNerf = FixedMul(UFO_PITY_BRAKES, mapspeedscale) >> FRACBITS;

	// Best values of all of the players.
	UINT32 bestDist = UINT32_MAX;
	fixed_t bestSpeed = 0;

	// Desired values for the UFO itself.
	UINT32 wantedDist = UINT32_MAX;
	fixed_t wantedSpeed = ufo_speed(ufo);
	fixed_t speedDelta = 0;

	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];
		if (player->spectator == true)
		{
			continue;
		}

		if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
		{
			continue;
		}

		if (player->distancetofinish < bestDist)
		{
			bestDist = player->distancetofinish;

			// Doesn't matter if a splitscreen player behind is moving faster behind the one most caught up.
			bestSpeed = R_PointToDist2(0, 0, player->rmomx, player->rmomy);
			bestSpeed = min(bestSpeed, K_GetKartSpeed(player, false, false)); // Don't become unfair with Sneakers.
			bestSpeed = FixedDiv(bestSpeed, mapobjectscale); // Unscale from mapobjectscale to FRACUNIT
			bestSpeed = FixedMul(bestSpeed, UFO_SPEEDFACTOR); // Make it a bit more lenient
		}
	}

	if (bestDist == UINT32_MAX)
	{
		// Invalid, lets go back to base speed.
		wantedSpeed = baseSpeed;
	}
	else
	{
		INT32 distDelta = 0;

		if (bestDist < UFO_PITY_DIST && UFOEmeraldChase(ufo))
		{
			INT32 brakeDelta = UFO_PITY_DIST - bestDist;
			INT32 distPerNerf = UFO_PITY_DIST / distanceNerf; // Doing this in the sensible way integer overflows. Sorry.
			spacing = spacing - (brakeDelta / distPerNerf);
		}

		if (bestDist > spacing)
		{
			wantedDist = bestDist - spacing;
		}
		else
		{
			wantedDist = 0;
		}

		distDelta = ufo_distancetofinish(ufo) - wantedDist;

		if (distDelta > 0)
		{
			// Too far behind! Start speeding up!
			wantedSpeed = max(bestSpeed, baseSpeed << 2);
		}
		else
		{
			if (abs(distDelta) <= deadzone)
			{
				// We're in a good spot, try to match the player.
				wantedSpeed = max(bestSpeed >> 1, baseSpeed);
			}
			else
			{
				// Too far ahead! Start slowing down!
				wantedSpeed = baseSpeed;
			}
		}
	}

	// Slowly accelerate or decelerate to
	// get to our desired speed.
	speedDelta = wantedSpeed - ufo_speed(ufo);
	if (speedDelta > 0)
	{
		if (abs(speedDelta) <= UFO_SPEEDUP)
		{
			ufo_speed(ufo) = wantedSpeed;
		}
		else
		{
			ufo_speed(ufo) += UFO_SPEEDUP;
		}

		// these number are primarily vibes based and not empirically derived
		if (UFOEmeraldChase(ufo))
		{
			if (ufo_speed(ufo) > 50*FRACUNIT)
				SpawnEmeraldSpeedLines(ufo);
		}
		else if (ufo_speed(ufo) > 70*FRACUNIT && !S_SoundPlaying(ufo, sfx_clawzm))
		{
			S_StartSound(ufo, sfx_clawzm);
		}
	}
	else if (speedDelta < 0)
	{
		if (abs(speedDelta) <= UFO_SLOWDOWN)
		{
			ufo_speed(ufo) = wantedSpeed;
		}
		else
		{
			ufo_speed(ufo) -= UFO_SLOWDOWN;
		}
	}
}

static void UFOUpdateAngle(mobj_t *ufo)
{
	angle_t dest = K_MomentumAngle(ufo);
	INT32 delta = AngleDeltaSigned(ufo->angle, dest);
	ufo->angle += delta >> 2;
}

waypoint_t *K_GetSpecialUFOWaypoint(mobj_t *ufo)
{
	if ((ufo == NULL) && (specialstageinfo.valid == true))
	{
		ufo = specialstageinfo.ufo;
	}

	if (ufo != NULL && P_MobjWasRemoved(ufo) == false
		&& ufo->type == MT_SPECIAL_UFO)
	{
		if (ufo_waypoint(ufo) >= 0)
		{
			return K_GetWaypointFromIndex((size_t)ufo_waypoint(ufo));
		}
	}

	return NULL;
}

static void UFOMoveToDistance(mobj_t *ufo, UINT32 distancetofinish)
{
	waypoint_t *finishline = K_GetFinishLineWaypoint();
	const boolean useshortcuts = false;
	const boolean huntbackwards = true;
	path_t pathtofinish = {0};

	if (finishline == NULL)
	{
		return;
	}

	boolean pathfindsuccess = K_PathfindThruCircuit(
		finishline,
		distancetofinish,
		&pathtofinish,
		useshortcuts,
		huntbackwards
	);

	if (pathfindsuccess == false)
	{
		return;
	}

	pathfindnode_t *node = &pathtofinish.array[pathtofinish.numnodes - 1];

	if (node->camefrom != NULL)
	{
		UINT32 a_to_b = (node->gscore - node->camefrom->gscore);
		UINT32 overshot = (node->gscore - distancetofinish);
		fixed_t f = FixedDiv(overshot, max(1, a_to_b));

		mobj_t *a = ((waypoint_t*)node->camefrom->nodedata)->mobj;
		mobj_t *b = ((waypoint_t*)node->nodedata)->mobj;

		UFOMoveTo(
			ufo,
			b->x - FixedMul(f, b->x - a->x),
			b->y - FixedMul(f, b->y - a->y),
			b->z - FixedMul(f, b->z - a->z)
		);
	}

	Z_Free(pathtofinish.array);
}

static void UFOMove(mobj_t *ufo)
{
	extern consvar_t cv_ufo_follow;

	if (cv_ufo_follow.value)
	{
		if (playeringame[cv_ufo_follow.value - 1])
		{
			UFOMoveToDistance(ufo, players[cv_ufo_follow.value - 1].distancetofinish);
		}
		return;
	}

	waypoint_t *curWaypoint = NULL;
	waypoint_t *destWaypoint = NULL;

	fixed_t distLeft = INT32_MAX;
	fixed_t newX = ufo->x;
	fixed_t newY = ufo->y;
	fixed_t newZ = ufo->z;
	const fixed_t floatHeight = 24 * ufo->scale;

	const boolean useshortcuts = false;
	const boolean huntbackwards = false;
	boolean pathfindsuccess = false;
	path_t pathtofinish = {0};
	size_t pathIndex = 0;

	boolean reachedEnd = false;

	curWaypoint = K_GetSpecialUFOWaypoint(ufo);
	destWaypoint = K_GetFinishLineWaypoint();

	if (curWaypoint == NULL || destWaypoint == NULL)
	{
		// Waypoints aren't valid.
		// Just go straight up.
		// :japanese_ogre: : "Abrupt and funny is the funniest way to end the special stage anyways"
		ufo->momx = 0;
		ufo->momy = 0;
		ufo->momz = ufo_speed(ufo);
		ufo_distancetofinish(ufo) = 0;
		return;
	}

	distLeft = FixedMul(ufo_speed(ufo), mapobjectscale);

	while (distLeft > 0)
	{
		fixed_t wpX = curWaypoint->mobj->x;
		fixed_t wpY = curWaypoint->mobj->y;
		fixed_t wpZ = curWaypoint->mobj->z + floatHeight;

		fixed_t distToNext = GenericDistance(
			newX, newY, newZ,
			wpX, wpY, wpZ
		);

		if (distToNext > distLeft)
		{
			// Only made it partially there.
			newX += FixedMul(FixedDiv(wpX - newX, distToNext), distLeft);
			newY += FixedMul(FixedDiv(wpY - newY, distToNext), distLeft);
			newZ += FixedMul(FixedDiv(wpZ - newZ, distToNext), distLeft);

			distLeft = 0;
		}
		else
		{
			// Close enough to the next waypoint,
			// move there and remove the distance.
			newX = wpX;
			newY = wpY;
			newZ = wpZ;

			distLeft -= distToNext;

			if (curWaypoint == destWaypoint)
			{
				// Reached the end.
				reachedEnd = true;
				break;
			}

			// Create waypoint path to our destination.
			// Crazy over-engineered, just to catch when
			// waypoints are insanely close to each other :P
			if (pathfindsuccess == false)
			{
				pathfindsuccess = K_PathfindToWaypoint(
					curWaypoint, destWaypoint,
					&pathtofinish,
					useshortcuts, huntbackwards
				);

				if (pathfindsuccess == false)
				{
					// Path isn't valid.
					// Just keep going.
					break;
				}
			}

			pathIndex++;

			if (pathIndex >= pathtofinish.numnodes)
			{
				// Successfully reached the end of the path.
				reachedEnd = true;
				break;
			}

			// Now moving to the next waypoint.
			curWaypoint = (waypoint_t *)pathtofinish.array[pathIndex].nodedata;
			ufo_waypoint(ufo) = (INT32)K_GetWaypointHeapIndex(curWaypoint);
		}
	}

	UFOMoveTo(ufo, newX, newY, newZ);

	if (reachedEnd == true)
	{
		// Invalidate UFO/emerald
		ufo_waypoint(ufo) = -1;
		ufo->flags &= ~(MF_SPECIAL|MF_PICKUPFROMBELOW);

		// Disable player
		P_DoAllPlayersExit(PF_NOCONTEST, false);

		HU_DoTitlecardCEcho(NULL, "TOO LATE...", false);
	}

	if (pathfindsuccess == true)
	{
		Z_Free(pathtofinish.array);
	}
}

static void UFOEmeraldVFX(mobj_t *emerald)
{
	const INT32 bobS = 32;
	const angle_t bobA = (leveltime & (bobS - 1)) * (ANGLE_MAX / bobS);
	const fixed_t bobH = 16 * emerald->scale;

	emerald->sprzoff = FixedMul(bobH, FINESINE(bobA >> ANGLETOFINESHIFT));

	Obj_SpawnEmeraldSparks(emerald);
}

static boolean UFOHumPlaying(mobj_t *ufo) {
	INT32 i;
	for (i = 0; i <= maxhum; i++)
	{
		if (S_SoundPlaying(ufo, hums[i]))
			return true;
	}
	return false;
}

static void UFOUpdateSound(mobj_t *ufo) {
	INT32 maxhealth = max(mobjinfo[MT_SPECIAL_UFO].spawnhealth, 1);
	INT32 healthlevel = maxhum * ufo->health / maxhealth;

	if (!UFOEmeraldChase(ufo) && !UFOHumPlaying(ufo))
	{
		healthlevel = min(max(healthlevel, 1), maxhum);
		S_StartSound(ufo, hums[maxhum - healthlevel]);
	}
}

static void UFODebugSetHealth(mobj_t *ufo, UINT8 health)
{
	if (ufo->health == health + 1 || UFOEmeraldChase(ufo) == true)
	{
		return;
	}

	extern consvar_t cv_ufo_follow;

	UINT8 pnum = max(1, cv_ufo_follow.value) - 1;
	mobj_t *source = players[pnum].mo;

	if (playeringame[pnum] == false || P_MobjWasRemoved(source) == true)
	{
		return;
	}

	ufo->health = health + 2;
	Obj_SpecialUFODamage(ufo, ufo, source, DMG_NORMAL); // does 1 damage, updates pieces
}

void Obj_SpecialUFOThinker(mobj_t *ufo)
{
	{
		extern consvar_t cv_ufo_health;
		if (cv_ufo_health.value != -1)
		{
			UFODebugSetHealth(ufo, cv_ufo_health.value);
		}
	}

	UFOMove(ufo);
	UFOUpdateAngle(ufo);
	UFOUpdateDistanceToFinish(ufo);
	UFOUpdateSpeed(ufo);
	UFOUpdateSound(ufo);

	if (ufo_intangible(ufo))
		ufo_intangible(ufo)--;

	if (UFOEmeraldChase(ufo) == true)
	{
		ufo_collectdelay(ufo)--;
	}
	else
	{
		ufo_collectdelay(ufo) = TICRATE;
	}
}

// The following is adapted from monitor.c for UFO Catcher damage
// I couldn't just exose the relevant things via k_object.h
// because they're *just* too specific to Monitors... ~toast 070423

#define shard_can_roll(o) ((o)->extravalue1)

static inline boolean
can_shard_state_roll (statenum_t state)
{
	switch (state)
	{
		case S_MONITOR_BIG_SHARD:
		case S_MONITOR_SMALL_SHARD:
			return true;

		default:
			return false;
	}
}

static void
spawn_shard
(		mobj_t * part,
		statenum_t state)
{
	mobj_t *ufo = ufo_piece_owner(part);

	// These divisions and multiplications are done on the
	// offsets to give bigger increments of randomness.

	const fixed_t h = FixedDiv(
			ufo->height, ufo->scale);

	const UINT16 rad = (ufo->radius / ufo->scale) / 4;
	const UINT16 tall = (h / FRACUNIT);

	// note: determinate random argument eval order
	fixed_t rand_z = P_RandomKey(PR_ITEM_DEBRIS, tall + 1);
	fixed_t rand_y = P_RandomRange(PR_ITEM_DEBRIS, -(rad), rad);
	fixed_t rand_x = P_RandomRange(PR_ITEM_DEBRIS, -(rad), rad);
	mobj_t *p = P_SpawnMobjFromMobj(ufo,
			rand_x * 8 * FRACUNIT,
			rand_y * 8 * FRACUNIT,
			rand_z * 4 * FRACUNIT,
			MT_MONITOR_SHARD);

	P_SetScale(p, (p->destscale = p->destscale * 3));

	angle_t th = R_PointToAngle2(ufo->x, ufo->y, p->x, p->y);

	th -= P_RandomKey(PR_ITEM_DEBRIS, ANGLE_45) - ANGLE_22h;

	p->hitlag = 0;

	P_Thrust(p, th, 6 * p->scale);
	p->momz = P_RandomRange(PR_ITEM_DEBRIS, 3, 10) * p->scale;

	P_SetMobjState(p, state);

	shard_can_roll(p) = can_shard_state_roll(state);

	if (shard_can_roll(p))
	{
		p->rollangle = P_Random(PR_ITEM_DEBRIS);
	}

	if (P_RandomChance(PR_ITEM_DEBRIS, FRACUNIT/2))
	{
		p->renderflags |= RF_DONTDRAW;
	}
}

static void
set_flickerframe (mobj_t *ufo, mobj_t *piece)
{
	INT32 healthcalc = (UFO_NUM_GLASSFRAMES - 1);

	if (ufo && !P_MobjWasRemoved(ufo))
	{
		INT32 maxhealth = mobjinfo[MT_SPECIAL_UFO].spawnhealth;
		healthcalc = (maxhealth - ufo->health);

		if (healthcalc > 0)
		{
			maxhealth /= UFO_NUM_GLASSFRAMES;
			if (maxhealth <= 0)
				maxhealth = 1;
			healthcalc /= maxhealth;
			if (healthcalc >= UFO_NUM_GLASSFRAMES)
				healthcalc = (UFO_NUM_GLASSFRAMES - 1);
			if (healthcalc < 0)
				healthcalc = 0;
		}
		else
		{
			healthcalc = 0;
		}
	}

	healthcalc = (healthcalc|FF_FULLBRIGHT) + UFO_START_GLASSFRAMES;

	ufo_piece_glass_flickerframe(piece) = healthcalc;
	piece->frame = healthcalc;
}


static void
spawn_debris (mobj_t *part)
{
	mobj_t *ufo = ufo_piece_owner(part);

	INT32 i;

	for (i = ufo->health;
		i <= mobjinfo[ufo->type].spawnhealth; i += 5)
	{
		spawn_shard(part, S_MONITOR_BIG_SHARD);
		spawn_shard(part, S_MONITOR_SMALL_SHARD);
		spawn_shard(part, S_MONITOR_TWINKLE);
	}
}

static void UFOCopyHitlagToPieces(mobj_t *ufo)
{
	mobj_t *piece = NULL;

	piece = ufo_pieces(ufo);
	while (UFOPieceValid(piece) == true)
	{
		piece->hitlag = ufo->hitlag;
		piece->eflags = (piece->eflags & ~MFE_DAMAGEHITLAG) | (ufo->eflags & MFE_DAMAGEHITLAG);

		if (ufo_piece_type(piece) == UFO_PIECE_TYPE_GLASS)
		{
			set_flickerframe (ufo, piece);
			spawn_debris (piece);
		}

		piece = ufo_piece_next(piece);
	}
}

static void UFOKillPiece(mobj_t *piece)
{
	angle_t dir = ANGLE_MAX;
	fixed_t thrust = 0;

	if (UFOPieceValid(piece) == false)
	{
		return;
	}

	piece->health = 0;
	piece->tics = TICRATE;
	piece->flags &= ~MF_NOGRAVITY;

	switch (ufo_piece_type(piece))
	{
		case UFO_PIECE_TYPE_GLASS:
		{
			set_flickerframe(NULL, piece);
			piece->tics = 1;
			return;
		}
		case UFO_PIECE_TYPE_GLASS_UNDER:
		case UFO_PIECE_TYPE_STEM:
		{
			piece->tics = 1;
			return;
		}
		case UFO_PIECE_TYPE_ARM:
		{
			dir = piece->angle;
			thrust = 12 * piece->scale;
			break;
		}
		default:
		{
			dir = FixedAngle(P_RandomRange(PR_DECORATION, 0, 359) << FRACBITS);
			thrust = 4 * piece->scale;
			break;
		}
	}

	P_Thrust(piece, dir, -thrust);
	P_SetObjectMomZ(piece, 12*FRACUNIT, true);
}

static void UFOKillPieces(mobj_t *ufo)
{
	mobj_t *piece = NULL;

	piece = ufo_pieces(ufo);
	while (UFOPieceValid(piece) == true)
	{
		UFOKillPiece(piece);
		piece = ufo_piece_next(piece);
	}
}

static UINT8 GetUFODamage(mobj_t *inflictor, UINT8 damageType)
{
	UINT8 ret = 0;
	targetdamaging_t targetdamaging = UFOD_GENERIC;

	if (inflictor != NULL && P_MobjWasRemoved(inflictor) == false)
	{
		switch (inflictor->type)
		{
			// Shields deal chip damage.
			case MT_JAWZ_SHIELD:
			{
				targetdamaging = UFOD_JAWZ;
				ret = 10;
				break;
			}
			case MT_ORBINAUT_SHIELD:
			{
				targetdamaging = UFOD_ORBINAUT;
				ret = 10;
				break;
			}
			case MT_INSTAWHIP:
			{
				targetdamaging = UFOD_WHIP;
				ret = 10;
				inflictor->extravalue2 = 1; // Disable whip collision
				break;
			}
			case MT_JAWZ:
			{
				// Thrown Jawz deal a bit extra.
				targetdamaging = UFOD_JAWZ;
				ret = 15;
				break;
			}
			case MT_ORBINAUT:
			{
				// Thrown orbinauts deal double damage.
				targetdamaging = UFOD_ORBINAUT;
				ret = 20;
				break;
			}
			case MT_GACHABOM:
			{
				// Thrown gachabom need to be tracked, but have no special damage value as of yet.
				targetdamaging = UFOD_GACHABOM;
				break;
			}
			case MT_SPB:
			case MT_SPBEXPLOSION:
			{
				if (inflictor->type != MT_SPBEXPLOSION || inflictor->threshold == KITEM_SPB)
				{
					targetdamaging |= UFOD_SPB;
				}
				// SPB deals triple damage.
				ret = 30;
				break;
			}
			case MT_BANANA:
			{
				targetdamaging = UFOD_BANANA;

				// Banana snipes deal triple damage,
				// laid down bananas deal regular damage.
				if (inflictor->health > 1)
				{
					ret = 30;
					break;
				}

				ret = 10;
				break;
			}
			case MT_GARDENTOP:
			{
				// Garden Top is not classified as a "field
				// item" because the player can ride it. So
				// an explicit case is necessary.
				ret = 10;
				break;
			}
			case MT_PLAYER:
			{
				// Players deal damage relative to how many sneakers they used.
				targetdamaging = UFOD_BOOST;
				ret = 15 * max(1, inflictor->player->numsneakers + inflictor->player->numpanelsneakers + inflictor->player->numweaksneakers);
				break;
			}
			case MT_SPECIAL_UFO:
			{
				// UFODebugSetHealth
				ret = 1;
				break;
			}
			default:
			{
				// General hazards cannot damage the UFO
				if (P_IsKartFieldItem(inflictor->type) == false)
				{
					return 0;
				}
				break;
			}
		}
	}

	P_TrackRoundConditionTargetDamage(targetdamaging);

	if (ret != 0)
		return ret;

	// Guess from damage type.
	switch (damageType & DMG_TYPEMASK)
	{
		case DMG_NORMAL:
		case DMG_STING:
		default:
		{
			return 10;
		}
		case DMG_WIPEOUT:
		{
			return 20;
		}
		case DMG_EXPLODE:
		case DMG_TUMBLE:
		{
			return 30;
		}
		case DMG_VOLTAGE:
		{
			return 15;
		}
	}
}

boolean Obj_SpecialUFODamage(mobj_t *ufo, mobj_t *inflictor, mobj_t *source, UINT8 damageType)
{
	const fixed_t addSpeed = FixedMul(UFO_DAMAGED_SPEED, K_GetKartGameSpeedScalar(gamespeed));
	UINT8 damage = 1;

	if (UFOEmeraldChase(ufo) == true)
	{
		// Damaged fully already, no need for any more.
		return false;
	}

	damage = GetUFODamage(inflictor, damageType);

	if (damage <= 0)
	{
		return false;
	}

	if (source->player)
	{
		UINT32 skinflags = (demo.playback)
			? demo.skinlist[demo.currentskinid[(source->player-players)]].flags
			: skins[source->player->skin]->flags;
		if (skinflags & SF_IRONMAN)
			SetRandomFakePlayerSkin(source->player, true, false);
	}

	ufo_intangible(ufo) = 60;

	// Speed up on damage!
	ufo_speed(ufo) += addSpeed;

	ufo->health = max(1, ufo->health - damage);

	if (grandprixinfo.gp)
	{
		grandprixinfo.specialDamage += damage;
	}

	K_SetHitLagForObjects(ufo, inflictor, source, (damage / 3) + 2, true);
	UFOCopyHitlagToPieces(ufo);

	if (ufo->health == 1)
	{
		// Destroy the UFO parts, and make the emerald collectible!
		UFOKillPieces(ufo);

		gamedata->deferredconditioncheck = true; // Check Challenges!

		ufo->flags = (ufo->flags & ~MF_SHOOTABLE) | (MF_SPECIAL|MF_PICKUPFROMBELOW);
		ufo->shadowscale = FRACUNIT/3;

		ACS_RunCatcherScript(source);

		S_StopSound(ufo);
		S_StartSound(ufo, sfx_gbrk);
		S_StartSound(ufo, sfx_clawk2);
		P_StartQuake(30, 96 * ufo->scale, 0, NULL);

		ufo_speed(ufo) += addSpeed; // Even more speed!
		return true;
	}

	S_StartSound(ufo, sfx_clawht);
	S_StopSoundByID(ufo, sfx_clawzm);

	for (int i = 0; i <= maxhum; i++)
	{
		S_StopSoundByID(ufo, hums[i]);
	}

	UFOUpdateSound(ufo);

	P_StartQuake(10, 64 * ufo->scale, 0, NULL);

	return true;
}

void Obj_PlayerUFOCollide(mobj_t *ufo, mobj_t *other)
{
	if (other->player == NULL)
	{
		return;
	}

	if (other->z > ufo->z + ufo->height)
	{
		return; // overhead
	}

	if (other->z + other->height < ufo->z)
	{
		return; // underneath
	}

	if (ufo_intangible(ufo))
	{
		return; // We were just hit!
	}

	if (other->player->tripwirePass >= TRIPWIRE_BOOST
		&& !P_PlayerInPain(other->player)
		&& (other->player->flashing == 0))
	{
		// Bump and deal damage.
		Obj_SpecialUFODamage(ufo, other, other, DMG_STEAL);
		other->player->sneakertimer = 0;
		other->player->numsneakers = 0;
		other->player->panelsneakertimer = 0;
		other->player->numpanelsneakers = 0;
		other->player->weaksneakertimer = 0;
		other->player->numweaksneakers = 0;

		// Copied from Obj_OrbinautThrown
		const ffloor_t *rover = P_IsObjectFlipped(other) ? other->ceilingrover : other->floorrover;
		if (rover && (rover->fofflags & FOF_SWIMMABLE))
		{
			// Player is waterskiing so use different math to
			// reduce their speed some but keep them skiing
			// at high speeds.
			fixed_t linear = K_GetKartSpeed(other->player, false, false) / 2;
			if ((other->player->speed - linear) < other->player->speed / 4)
			{
				other->momx /= 4;
				other->momy /= 4;
			}
			else
			{
				angle_t mom = R_PointToAngle2(0, 0, other->momx, other->momy);
				P_Thrust(other, mom, -linear);
			}
		}
		else
		{
			K_KartBouncing(other, ufo);
		}
	}
	else
	{
		const angle_t moveAngle = K_MomentumAngle(ufo);
		const angle_t clipAngle = R_PointToAngle2(ufo->x, ufo->y, other->x, other->y);

		if (AngleDelta(moveAngle, clipAngle) < ANG60)
		{
			// in front
			K_StumblePlayer(other->player);
		}

		K_KartBouncing(other, ufo);
	}
}

boolean Obj_UFOEmeraldCollect(mobj_t *ufo, mobj_t *toucher)
{
	mobj_t *emerald = ufo_emerald(ufo);

	if (toucher->player != NULL)
	{
		if (toucher->player->exiting || mapreset || (toucher->player->pflags & PF_ELIMINATED))
		{
			return false;
		}
	}

	if (ufo_collectdelay(ufo) > 0)
	{
		return false;
	}

	if (toucher->hitlag > 0)
	{
		return false;
	}

	ACS_RunEmeraldScript(toucher);

	if (!P_MobjWasRemoved(emerald))
	{
		const int kScaleTics = 16;

		// Emerald will now orbit the player
		Obj_BeginEmeraldOrbit(emerald, toucher, 100 * mapobjectscale, 64, 0);

		// Scale down because the emerald is huge
		// Super Emerald needs to be scaled down further
		emerald->destscale = emerald->scale / (ufo_emeraldnum(ufo) > 7 ? 3 : 2);
		emerald->scalespeed = abs(emerald->destscale - emerald->scale) / kScaleTics;
	}

	return true;
}

void Obj_UFOPieceThink(mobj_t *piece)
{
	mobj_t *ufo = ufo_piece_owner(piece);

	if (ufo == NULL || P_MobjWasRemoved(ufo) == true)
	{
		P_KillMobj(piece, NULL, NULL, DMG_NORMAL);
		return;
	}

	piece->scalespeed = ufo->scalespeed;

	switch (ufo_piece_type(piece))
	{
		case UFO_PIECE_TYPE_POD:
		{
			piece->destscale = 3 * ufo->destscale / 2;
			UFOMoveTo(piece, ufo->x, ufo->y, ufo->z + (132 * piece->scale));
			if (S_SoundPlaying(ufo, sfx_clawzm) && ufo_speed(ufo) > 70*FRACUNIT)
				SpawnUFOSpeedLines(piece);
			break;
		}
		case UFO_PIECE_TYPE_GLASS:
		{
			// Flicker glass cracks for visibility
			if (piece->frame == FF_SEMIBRIGHT)
			{
				piece->frame = ufo_piece_glass_flickerframe(piece);
			}
			else
			{
				piece->frame = FF_SEMIBRIGHT;
			}

			piece->destscale = 5 * ufo->destscale / 3;
			UFOMoveTo(piece, ufo->x, ufo->y, ufo->z);
			break;
		}
		case UFO_PIECE_TYPE_GLASS_UNDER:
		{
			piece->destscale = 5 * ufo->destscale / 3;
			UFOMoveTo(piece, ufo->x, ufo->y, ufo->z);
			break;
		}
		case UFO_PIECE_TYPE_ARM:
		{
			fixed_t dis = (88 * piece->scale);

			fixed_t x = ufo->x - FixedMul(dis, FINECOSINE(piece->angle >> ANGLETOFINESHIFT));
			fixed_t y = ufo->y - FixedMul(dis, FINESINE(piece->angle >> ANGLETOFINESHIFT));

			piece->destscale = 3 * ufo->destscale / 2;
			UFOMoveTo(piece, x, y, ufo->z + (24 * piece->scale));

			piece->angle -= FixedMul(ANG2, FixedDiv(ufo_speed(ufo), UFO_BASE_SPEED));
			break;
		}
		case UFO_PIECE_TYPE_STEM:
		{
			fixed_t stemZ = ufo->z + (294 * piece->scale);
			fixed_t sc = FixedDiv(FixedDiv(ufo->ceilingz - stemZ, piece->scale), 15 * FRACUNIT);

			piece->destscale = 3 * ufo->destscale / 2;
			UFOMoveTo(piece, ufo->x, ufo->y, stemZ);
			if (sc > 0)
			{
				piece->spriteyscale = sc;
			}
			break;
		}
		default:
		{
			P_RemoveMobj(piece);
			return;
		}
	}
}

void Obj_UFOPieceDead(mobj_t *piece)
{
	piece->renderflags ^= RF_DONTDRAW;
}

void Obj_UFOPieceRemoved(mobj_t *piece)
{
	// Repair piece list.
	mobj_t *ufo = ufo_piece_owner(piece);
	mobj_t *next = ufo_piece_next(piece);
	mobj_t *prev = ufo_piece_prev(piece);

	if (prev != NULL && P_MobjWasRemoved(prev) == false)
	{
		P_SetTarget(
			&ufo_piece_next(prev),
			(next != NULL && P_MobjWasRemoved(next) == false) ? next : NULL
		);
	}

	if (next != NULL && P_MobjWasRemoved(next) == false)
	{
		P_SetTarget(
			&ufo_piece_prev(next),
			(prev != NULL && P_MobjWasRemoved(prev) == false) ? prev : NULL
		);
	}

	if (ufo != NULL && P_MobjWasRemoved(ufo) == false)
	{
		if (piece == ufo_pieces(ufo))
		{
			P_SetTarget(
				&ufo_pieces(ufo),
				(next != NULL && P_MobjWasRemoved(next) == false) ? next : NULL
			);
		}
	}

	P_SetTarget(&ufo_piece_next(piece), NULL);
	P_SetTarget(&ufo_piece_prev(piece), NULL);
}

static mobj_t *InitSpecialUFO(waypoint_t *start)
{
	mobj_t *ufo = NULL;
	mobj_t *overlay = NULL;
	mobj_t *piece = NULL;
	mobj_t *prevPiece = NULL;
	size_t i;

	if (start == NULL)
	{
		// Simply create at the origin with default values.
		ufo = P_SpawnMobj(0, 0, 0, MT_SPECIAL_UFO);
		ufo_waypoint(ufo) = -1; // Invalidate
		ufo_distancetofinish(ufo) = INT32_MAX;
	}
	else
	{
		// Create with a proper waypoint track!
		ufo = P_SpawnMobj(start->mobj->x, start->mobj->y, start->mobj->z, MT_SPECIAL_UFO);
		ufo_waypoint(ufo) = (INT32)K_GetWaypointHeapIndex(start);
		UFOUpdateDistanceToFinish(ufo);
		specialstageinfo.maxDist = ufo_distancetofinish(ufo);
	}

	// Set specialDamage as early as possible, for glass ball's sake
	// (...Except if you're on Master difficulty!)
	if (grandprixinfo.gp && grandprixinfo.specialDamage && grandprixinfo.masterbots == false)
	{
		ufo->health -= min(2*(UINT32)mobjinfo[MT_SPECIAL_UFO].spawnhealth/10, grandprixinfo.specialDamage/12);
		// Use this if you want to spy on what the health ends up being:
		//CONS_Printf("the UFO weeps: %d hp\n", ufo->health );
	}

	ufo_speed(ufo) = FixedMul(UFO_START_SPEED, K_GetKartGameSpeedScalar(gamespeed));

	// Adjustable Special Stage emerald color/shape
	{
		mobj_t *emerald = P_SpawnMobjFromMobj(ufo, 0, 0, 0, MT_EMERALD);

		emerald->flags |= MF_NOGRAVITY | MF_NOCLIP | MF_NOCLIPTHING | MF_NOCLIPHEIGHT;

		overlay = P_SpawnMobjFromMobj(emerald, 0, 0, 0, MT_OVERLAY);

		emerald->color = SKINCOLOR_CHAOSEMERALD1;
		i = ufo_emeraldnum(ufo) = P_GetNextEmerald();
		if (i > 0)
		{
			emerald->color += (i - 1) % 7;
			if (i > 7)
			{
				// Super Emeralds
				P_SetMobjState(emerald, S_SUPEREMERALD1);
				P_SetMobjState(overlay, S_SUPEREMERALD_UNDER);
			}
			else
			{
				// Chaos Emerald
				P_SetMobjState(emerald, S_CHAOSEMERALD1);
				P_SetMobjState(overlay, S_CHAOSEMERALD_UNDER);
			}
		}
		else
		{
			// Prize -- todo, currently using fake Emerald
			P_SetMobjState(emerald, S_CHAOSEMERALD1);
			P_SetMobjState(overlay, S_CHAOSEMERALD_UNDER);
			emerald->color = SKINCOLOR_GOLD;
		}

		P_SetTarget(&emerald->target, ufo);
		P_SetTarget(&ufo_emerald(ufo), emerald);

		ufo->color = emerald->color; // for minimap
		overlay->color = emerald->color;

		P_SetTarget(&overlay->target, emerald);

		// UFO needs this so Jawz reticle lines up!
		ufo->sprzoff = 32 * mapobjectscale;

		emerald->sprzoff = ufo->sprzoff;
	}

	// Create UFO pieces.
	// First: UFO center.
	piece = P_SpawnMobjFromMobj(ufo, 0, 0, 0, MT_SPECIAL_UFO_PIECE);
	P_SetTarget(&ufo_piece_owner(piece), ufo);

	P_SetMobjState(piece, S_SPECIAL_UFO_POD);
	ufo_piece_type(piece) = UFO_PIECE_TYPE_POD;

	overlay = P_SpawnMobjFromMobj(piece, 0, 0, 0, MT_OVERLAY);
	P_SetTarget(&overlay->target, piece);
	P_SetMobjState(overlay, S_SPECIAL_UFO_OVERLAY);

	P_SetTarget(&ufo_pieces(ufo), piece);
	prevPiece = piece;

	// Next, the glass ball.
	{
		piece = P_SpawnMobjFromMobj(ufo, 0, 0, 0, MT_SPECIAL_UFO_PIECE);
		P_SetTarget(&ufo_piece_owner(piece), ufo);

		P_SetMobjState(piece, S_SPECIAL_UFO_GLASS);
		ufo_piece_type(piece) = UFO_PIECE_TYPE_GLASS;

		set_flickerframe(ufo, piece);

		/*overlay = P_SpawnMobjFromMobj(piece, 0, 0, 0, MT_OVERLAY);
		P_SetTarget(&overlay->target, piece);
		P_SetMobjState(overlay, S_SPECIAL_UFO_GLASS_UNDER);
		overlay->dispoffset = -20;*/

		P_SetTarget(&ufo_piece_next(prevPiece), piece);
		P_SetTarget(&ufo_piece_prev(piece), prevPiece);
		prevPiece = piece;
	}

	// This SHOULD have been an MT_OVERLAY... but it simply doesn't
	// draw-order stack with the Emerald correctly any other way.
	{
		piece = P_SpawnMobjFromMobj(ufo, 0, 0, 0, MT_SPECIAL_UFO_PIECE);
		P_SetTarget(&ufo_piece_owner(piece), ufo);

		P_SetMobjState(piece, S_SPECIAL_UFO_GLASS_UNDER);
		ufo_piece_type(piece) = UFO_PIECE_TYPE_GLASS_UNDER;
		piece->dispoffset = -2;

		P_SetTarget(&ufo_piece_next(prevPiece), piece);
		P_SetTarget(&ufo_piece_prev(piece), prevPiece);
		prevPiece = piece;
	}

	// Add the catcher arms.
	for (i = 0; i < UFO_NUMARMS; i++)
	{
		piece = P_SpawnMobjFromMobj(ufo, 0, 0, 0, MT_SPECIAL_UFO_PIECE);
		P_SetTarget(&ufo_piece_owner(piece), ufo);

		P_SetMobjState(piece, S_SPECIAL_UFO_ARM);
		ufo_piece_type(piece) = UFO_PIECE_TYPE_ARM;

		piece->angle = UFO_ARMDELTA * i;

		P_SetTarget(&ufo_piece_next(prevPiece), piece);
		P_SetTarget(&ufo_piece_prev(piece), prevPiece);
		prevPiece = piece;
	}

	// Add the stem.
	piece = P_SpawnMobjFromMobj(ufo, 0, 0, 0, MT_SPECIAL_UFO_PIECE);
	P_SetTarget(&ufo_piece_owner(piece), ufo);

	P_SetMobjState(piece, S_SPECIAL_UFO_STEM);
	ufo_piece_type(piece) = UFO_PIECE_TYPE_STEM;

	P_SetTarget(&ufo_piece_next(prevPiece), piece);
	P_SetTarget(&ufo_piece_prev(piece), prevPiece);
	prevPiece = piece;

	return ufo;
}

mobj_t *Obj_CreateSpecialUFO(void)
{
	return InitSpecialUFO(K_GetStartingWaypoint());
}

UINT32 K_GetSpecialUFODistance(void)
{
	if (specialstageinfo.valid == true)
	{
		if (specialstageinfo.ufo != NULL && P_MobjWasRemoved(specialstageinfo.ufo) == false)
		{
			return (UINT32)ufo_distancetofinish(specialstageinfo.ufo);
		}
	}

	return UINT32_MAX;
}

void Obj_UFOEmeraldThink(mobj_t *emerald)
{
	mobj_t *ufo = emerald->target;

	P_MoveOrigin(emerald, ufo->x, ufo->y, ufo->z);

	if (UFOEmeraldChase(ufo) == true)
	{
		// Spawn emerald sparkles
		UFOEmeraldVFX(emerald);
	}
}
