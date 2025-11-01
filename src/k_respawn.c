// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_respawn.c
/// \brief Respawning logic

#include "k_respawn.h"
#include "doomdef.h"
#include "d_player.h"
#include "k_kart.h"
#include "k_battle.h"
#include "k_objects.h" // Obj_FindCheckpoint, etc
#include "g_game.h"
#include "p_local.h"
#include "p_tick.h"
#include "p_setup.h"
#include "r_main.h"
#include "s_sound.h"
#include "p_slopes.h"
#include "r_defs.h"

/*--------------------------------------------------
	fixed_t K_RespawnOffset(player_t *player, boolean flip)

		See header file for description.
--------------------------------------------------*/
fixed_t K_RespawnOffset(player_t *player, boolean flip)
{
	fixed_t z = 0;

	if (flip == true)
	{
		z -= ((128 * mapobjectscale) + (player->mo->height));
	}
	else
	{
		z += (128 * mapobjectscale);
	}

	return z;
}

/*--------------------------------------------------
	static void K_FudgeRespawn(player_t *player, const waypoint_t *const waypoint)

		Fudges respawn coordinates to slightly before the waypoint if it would
		be exactly on a line. See K_GetWaypointIsOnLine.
--------------------------------------------------*/
static void K_FudgeRespawn(player_t *player, const waypoint_t *const waypoint)
{
	const angle_t from = R_PointToAngle2(waypoint->mobj->x, waypoint->mobj->y,
			player->mo->x, player->mo->y) >> ANGLETOFINESHIFT;

	player->respawn.pointx += FixedMul(16, FINECOSINE(from));
	player->respawn.pointy += FixedMul(16, FINESINE(from));
}

/*--------------------------------------------------
	void K_RespawnAtWaypoint(player_t *player, waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
void K_RespawnAtWaypoint(player_t *player, waypoint_t *waypoint)
{
	if (player == NULL || player->mo == NULL || P_MobjWasRemoved(player->mo))
	{
		return;
	}

	if (waypoint == NULL || waypoint->mobj == NULL || P_MobjWasRemoved(waypoint->mobj))
	{
		return;
	}

	player->respawn.pointx = waypoint->mobj->x;
	player->respawn.pointy = waypoint->mobj->y;
	player->respawn.pointz = waypoint->mobj->z;
	player->respawn.flip = (waypoint->mobj->flags2 & MF2_OBJECTFLIP) ? true : false; // K_RespawnOffset wants a boolean!
	player->respawn.pointz += K_RespawnOffset(player, player->respawn.flip);

	if (waypoint->onaline)
	{
		K_FudgeRespawn(player, waypoint);
	}
}

/*--------------------------------------------------
	void K_DoFault(player_t *player)

		See header file for description.
--------------------------------------------------*/

void K_DoFault(player_t *player)
{
	player->nocontrol = (starttime - leveltime) + TICRATE/2;
	if (!(player->pflags & PF_FAULT))
	{
		S_StartSound(player->mo, sfx_s3k83);
		player->karthud[khud_fault] = 1;
		player->pflags |= PF_FAULT;

		if (P_IsDisplayPlayer(player))
		{
			S_StartSound(player->mo, sfx_s3kb2);
		}

		player->mo->renderflags |= RF_DONTDRAW;
		player->mo->flags |= MF_NOCLIPTHING;

		if (player->roundconditions.faulted == false)
		{
			player->roundconditions.faulted = true;
			player->roundconditions.checkthisframe = true;
		}
	}
}

/*--------------------------------------------------
	void K_DoIngameRespawn(player_t *player)

		See header file for description.
--------------------------------------------------*/
void K_DoIngameRespawn(player_t *player)
{
	boolean faultstartfaulting = false;

	if (!player->mo || P_MobjWasRemoved(player->mo))
	{
		return;
	}

	if (player->finalfailsafe < FAILSAFETIME)
	{
		if (player->respawn.state != RESPAWNST_NONE &&
				( player->pflags & PF_FAULT ) == 0)
		{
			return;
		}
	}


	if (leveltime <= introtime)
	{
		return;
	}

	// FAULT
	if ((gametyperules & GTR_CIRCUIT) && leveltime < starttime)
	{
		const waypoint_t *finish = K_GetFinishLineWaypoint();

		if (numfaultstarts > 0 && faultstart)
		{
			subsector_t *subs;
			if ((subs = R_PointInSubsectorOrNull(faultstart->x << FRACBITS, faultstart->y << FRACBITS)) != NULL)
			{
				faultstartfaulting = true;
				player->respawn.wp = NULL;
				player->respawn.flip = false;
				player->respawn.pointx = faultstart->x << FRACBITS;
				player->respawn.pointy = faultstart->y << FRACBITS;
				player->respawn.pointz =
					P_GetSectorFloorZAt(subs->sector, faultstart->x << FRACBITS, faultstart->y << FRACBITS)
					+ (faultstart->z << FRACBITS)
					+ K_RespawnOffset(player, player->respawn.flip);
				player->respawn.pointangle = FixedAngle(faultstart->angle << FRACBITS);
			}
		}
		else if (!(mapheaderinfo[gamemap-1]->levelflags & LF_SECTIONRACE) && finish != NULL)
			player->respawn.wp = finish->prevwaypoints[0];
		K_DoFault(player);
	}

	if (player->rings <= -20 && !player->respawn.fromRingShooter)
	{
		P_KillMobj(player->mo, NULL, NULL, DMG_INSTAKILL);
		return;
	}

	player->ringboost = 0;
	player->driftboost = player->strongdriftboost = 0;
	player->gateBoost = 0;
	player->trickcharge = 0;
	player->infinitether = 0;
	player->wavedash = player -> wavedashleft = player->wavedashright = player->wavedashboost = player->wavedashdelay = 0;

	K_TumbleInterrupt(player);
	P_ResetPlayer(player);

	mobj_t *checkpoint;
	vector3_t pos;

	// Set up respawn position if invalid
	if (player->respawn.manual == true)
	{
		player->respawn.distanceleft = 0;
		player->respawn.pointz += K_RespawnOffset(player, player->respawn.flip);
		player->respawn.manual = false; // one respawn only!
	}
	else if (player->respawn.wp != NULL)
	{
		if (player->respawn.fromRingShooter == true)
		{
			waypoint_t *prevWP = player->respawn.wp;

			const UINT32 dist = (player->airtime * 48);
			player->respawn.distanceleft = (dist * mapobjectscale) / FRACUNIT;

			K_RespawnAtWaypoint(player, prevWP);
		}
		else
		{
			const UINT32 dist = RESPAWN_DIST + (player->airtime * 48);
			player->respawn.distanceleft = (dist * mapobjectscale) / FRACUNIT;
			K_RespawnAtWaypoint(player, player->respawn.wp);
		}
	}
	else if (faultstartfaulting)
	{
		; // Do nothing, position was already set
	}
	else if ((gametyperules & GTR_CHECKPOINTS)
		&& player->checkpointId
		&& (checkpoint = Obj_FindCheckpoint(player->checkpointId))
		&& Obj_GetCheckpointRespawnPosition(checkpoint, &pos))
	{
		player->respawn.wp = NULL;
		player->respawn.flip = (checkpoint->flags2 & MF2_OBJECTFLIP) ? true : false; // K_RespawnOffset wants a boolean!
		player->respawn.pointx = pos.x;
		player->respawn.pointy = pos.y;
		player->respawn.pointz = pos.z + K_RespawnOffset(player, player->respawn.flip);

		player->respawn.pointangle = Obj_GetCheckpointRespawnAngle(checkpoint);

		player->respawn.distanceleft = 0;
	}
	else
	{
		UINT32 bestdist = UINT32_MAX;
		mapthing_t *beststart = NULL;
		UINT8 numstarts = 0;
		mapthing_t **starts;

		if (gametyperules & GTR_BATTLESTARTS)
		{
			numstarts = numdmstarts;
			starts = deathmatchstarts;
		}
		else
		{
			numstarts = numcoopstarts;
			starts = playerstarts;
		}

		if (numstarts > 0)
		{
			UINT8 i = 0;

			if (gametype == GT_TUTORIAL)
			{
				// In tutorial, spawnpoints are player ID locked.
				// ...but returning from Test Track can do funny things,
				// so we use relative ID instead of literal slot number.
				UINT8 spos = 0;
				for (; i < MAXPLAYERS; i++)
				{
					if (i == player-players)
						break;
					if (!playeringame[i])
						continue;
					spos++;
				}

				beststart = starts[spos % numstarts];
			}
			else for (i = 0; i < numstarts; i++)
			{
				UINT32 dist = UINT32_MAX;
				mapthing_t *checkstart = NULL;

				checkstart = starts[i];

				dist = (UINT32)P_AproxDistance((player->mo->x >> FRACBITS) - checkstart->x,
					(player->mo->y >> FRACBITS) - checkstart->y);

				if (dist < bestdist)
				{
					beststart = checkstart;
					bestdist = dist;
				}
			}
		}

		if (beststart == NULL)
		{
			CONS_Alert(CONS_WARNING, "No respawn points!\n");
			player->respawn.pointx = 0;
			player->respawn.pointy = 0;
			player->respawn.pointz = 0;
			player->respawn.pointangle = 0;
			player->respawn.flip = false;
		}
		else
		{
			sector_t *s;
			fixed_t z = beststart->z * FRACUNIT;

			player->respawn.pointx = beststart->x << FRACBITS;
			player->respawn.pointy = beststart->y << FRACBITS;

			player->respawn.pointangle = ( beststart->angle * ANG1 );

			s = R_PointInSubsector(beststart->x << FRACBITS, beststart->y << FRACBITS)->sector;

			player->respawn.flip = (beststart->options & MTF_OBJECTFLIP) != 0;

			if (player->respawn.flip == true)
			{
				player->respawn.pointz = (
#ifdef ESLOPE
				s->c_slope ? P_GetZAt(s->c_slope, player->respawn.pointx, player->respawn.pointy) :
#endif
				s->ceilingheight);

				if (z != 0)
				{
					player->respawn.pointz -= z;
				}
			}
			else
			{
				player->respawn.pointz = (
#ifdef ESLOPE
				s->f_slope ? P_GetZAt(s->f_slope, player->respawn.pointx, player->respawn.pointy) :
#endif
				s->floorheight);

				if (z)
				{
					player->respawn.pointz += z;
				}
			}
		}

		player->respawn.pointz += K_RespawnOffset(player, player->respawn.flip);
		player->respawn.distanceleft = 0;
	}

	player->respawn.timer = RESPAWN_TIME;
	player->respawn.state = RESPAWNST_MOVE;
	player->respawn.init = true;
	player->respawn.fast = true;
	player->respawn.returnspeed = 0;

	player->respawn.airtimer = player->airtime;
	player->respawn.truedeath = !!(player->pflags & PF_FAULT);

	player->botvars.respawnconfirm = 0;

	player->mo->flags |= MF_NOCLIPTHING;
}

/*--------------------------------------------------
	size_t K_NextRespawnWaypointIndex(waypoint_t *waypoint)

		See header file for description.
--------------------------------------------------*/
size_t K_NextRespawnWaypointIndex(waypoint_t *waypoint)
{
	size_t           i = 0U;
	size_t newwaypoint = SIZE_MAX;

	// Set to the first valid nextwaypoint, for simplicity's sake.
	// If we reach the last waypoint and it's still not valid, just use it anyway. Someone needs to fix their map!
	for (i = 0U; i < waypoint->numnextwaypoints; i++)
	{
		newwaypoint = i;

		if ((i == waypoint->numnextwaypoints - 1U)
		|| ((K_GetWaypointIsEnabled(waypoint->nextwaypoints[newwaypoint]) == true)
		&& (K_GetWaypointIsSpawnpoint(waypoint->nextwaypoints[newwaypoint]) == true)))
		{
			break;
		}
	}

	return newwaypoint;
}

/*--------------------------------------------------
	static void K_MovePlayerToRespawnPoint(player_t *player)

		Handles the movement state of the respawn animation.

	Input Arguments:-
		player - Player to preform for.

	Return:-
		None
--------------------------------------------------*/
static void K_MovePlayerToRespawnPoint(player_t *player)
{
	const int airCompensation = 128;
	fixed_t realstepamt = (64 * mapobjectscale);
	UINT32 returntime = TICRATE;
	fixed_t stepamt;

	vector3_t dest, step, laser;
	angle_t stepha, stepva;
	fixed_t dist, fulldist;

	UINT8 lasersteps = 4;
	UINT32 laserdist;
	waypoint_t *laserwp;
	boolean laserflip;

	/* speed up if in the air for a long time */
	realstepamt += FixedMul(realstepamt,
			(player->respawn.airtimer * FRACUNIT)
			/ airCompensation);

	stepamt = realstepamt;

	player->mo->momx = player->mo->momy = player->mo->momz = 0;

	// 3 because this timer counts down afterward, in
	// P_PlayerThink. flashing must be > 1 after it has
	// counted down in order to flicker the player sprite.
	player->flashing = 3;
	//player->nocontrol = max(2, player->nocontrol);

	if (leveltime % 8 == 0 && !mapreset)
	{
		S_StartSound(player->mo, sfx_s3kcas);
	}

	dest.x = player->respawn.pointx;
	dest.y = player->respawn.pointy;
	dest.z = player->respawn.pointz;

	dist = P_AproxDistance(P_AproxDistance(
		player->mo->x - dest.x,
		player->mo->y - dest.y),
		player->mo->z - dest.z
	);

	// Traveling from death location to first waypoint? Set speed to get there in a fixed time.
	if (player->respawn.fast)
	{
		if (player->respawn.returnspeed == 0)
			player->respawn.returnspeed = dist / returntime;
		stepamt = max(stepamt, player->respawn.returnspeed);
	}

	if (dist <= stepamt)
	{
		// Reduce by the amount we needed to get to this waypoint
		stepamt -= dist;

		fixed_t oldx = player->mo->x;
		fixed_t oldy = player->mo->y;

		// We've reached the destination point,
		P_UnsetThingPosition(player->mo);
		player->mo->x = dest.x;
		player->mo->y = dest.y;
		player->mo->z = dest.z;
		P_SetThingPosition(player->mo);

		// Did we cross a checkpoint during our last step?
		Obj_CrossCheckpoints(player, oldx, oldy);

		// We are no longer traveling from death location to 1st waypoint, so use standard timings
		if (player->respawn.fast)
			player->respawn.fast = false;

		// At the first valid waypoint, permit extra player control options.
		player->respawn.init = false;

		// Find the next waypoint to head towards
		if (player->respawn.wp != NULL)
		{
			size_t nwp = K_NextRespawnWaypointIndex(player->respawn.wp);

			if (nwp == SIZE_MAX || player->respawn.wp->nextwaypoints[nwp]->mobj->movefactor) // movefactor: Block Lightsnake
			{
				player->respawn.state = RESPAWNST_DROP;
				return;
			}

			// Set angle, regardless of if we're done or not
			P_SetPlayerAngle(player, R_PointToAngle2(
				player->respawn.wp->mobj->x,
				player->respawn.wp->mobj->y,
				player->respawn.wp->nextwaypoints[nwp]->mobj->x,
				player->respawn.wp->nextwaypoints[nwp]->mobj->y
			));
			player->drawangle = R_PointToAngle2(
				player->mo->x, player->mo->y,
				dest.x, dest.y
			);

			if ((player->respawn.distanceleft == 0 && K_GetWaypointIsSpawnpoint(player->respawn.wp) == true)
			|| (player->respawn.wp == K_GetFinishLineWaypoint()
			|| player->respawn.wp->nextwaypoints[nwp] == K_GetFinishLineWaypoint())) // Try not to allow you to pass the finish line while respawning, because it's janky
			{
				// Alright buddy, that's the end of the ride.
				player->respawn.state = RESPAWNST_DROP;
				return;
			}

			if (player->respawn.distanceleft > player->respawn.wp->nextwaypointdistances[nwp])
			{
				player->respawn.distanceleft -= player->respawn.wp->nextwaypointdistances[nwp];
			}
			else
			{
				player->respawn.distanceleft = 0;
			}

			// Almost all legitimate driving, no matter how clumsy, should be faster than death in TA.
			// Advance only as far as we need to prevent respawn loops!
			if (modeattacking)
			{
				player->respawn.distanceleft = 0;
			}

			player->respawn.wp = player->respawn.wp->nextwaypoints[nwp];
			K_RespawnAtWaypoint(player, player->respawn.wp);

			player->mo->eflags &= ~(MFE_VERTICALFLIP);

			if (player->respawn.flip)
			{
				player->mo->eflags |= MFE_VERTICALFLIP;
			}

			dest.x = player->respawn.pointx;
			dest.y = player->respawn.pointy;
			dest.z = player->respawn.pointz;
		}
		else
		{
			// We can now drop!
			if (gametyperules & GTR_CHECKPOINTS)
			{
				// Of course, in gametypes where there's a clear and intended progression, set our direction.
				P_SetPlayerAngle(player, (player->drawangle = player->respawn.pointangle));
			}
			player->respawn.state = RESPAWNST_DROP;
			return;
		}
	}

	stepha = R_PointToAngle2(
		player->mo->x, player->mo->y,
		dest.x, dest.y
	);

	stepva = R_PointToAngle2(
		0, player->mo->z,
		P_AproxDistance(player->mo->x - dest.x, player->mo->y - dest.y), dest.z
	);

	// Move toward the respawn point
	player->drawangle = stepha;

	step.x = FixedMul(FixedMul(FINECOSINE(stepha >> ANGLETOFINESHIFT), stepamt), FINECOSINE(stepva >> ANGLETOFINESHIFT));
	step.y = FixedMul(FixedMul(FINESINE(stepha >> ANGLETOFINESHIFT), stepamt), FINECOSINE(stepva >> ANGLETOFINESHIFT));
	step.z = FixedMul(FINESINE(stepva >> ANGLETOFINESHIFT), 2*stepamt);

	if (stepamt > 0)
	{
		player->mo->momx = step.x;
		player->mo->momy = step.y;
		player->mo->momz = step.z;
	}

	if (player->respawn.init == false
		&& player->respawn.fromRingShooter == false
		&& K_PressingEBrake(player) == true)
	{
		// Manual drop!
		player->respawn.state = RESPAWNST_DROP;
		return;
	}

	// NOW THEN, time for loads of dumb duplication!
	// "Emulate" the rest of the path, that way we can spawn a particle a certain distance ahead of you.

	if (stepamt != realstepamt)
	{
		// Reset back to default
		stepamt = realstepamt;

		step.x = FixedMul(FixedMul(FINECOSINE(stepha >> ANGLETOFINESHIFT), stepamt), FINECOSINE(stepva >> ANGLETOFINESHIFT));
		step.y = FixedMul(FixedMul(FINESINE(stepha >> ANGLETOFINESHIFT), stepamt), FINECOSINE(stepva >> ANGLETOFINESHIFT));
		step.z = FixedMul(FINESINE(stepva >> ANGLETOFINESHIFT), 2*stepamt);
	}

	laserdist = player->respawn.distanceleft;
	laserwp = player->respawn.wp;
	laserflip = player->respawn.flip;

	laser.x = player->mo->x + (step.x / 2);
	laser.y = player->mo->y + (step.y / 2);
	laser.z = player->mo->z + (step.z / 2);

	dist = P_AproxDistance(P_AproxDistance(
		laser.x - dest.x,
		laser.y - dest.y),
		laser.z - dest.z
	);
	fulldist = dist + (laserdist * FRACUNIT);

	while (lasersteps > 0)
	{
		if (fulldist <= stepamt)
		{
			break;
		}

		if (dist <= stepamt)
		{
			size_t lnwp;

			laser.x = dest.x;
			laser.y = dest.y;
			laser.z = dest.z;

			if (laserdist <= 0)
			{
				break;
			}

			lnwp = K_NextRespawnWaypointIndex(laserwp);
			if (lnwp == SIZE_MAX)
			{
				break;
			}

			if (laserdist > laserwp->nextwaypointdistances[lnwp])
			{
				laserdist -= laserwp->nextwaypointdistances[lnwp];
			}
			else
			{
				laserdist = 0;
			}

			laserwp = laserwp->nextwaypoints[lnwp];

			dest.x = laserwp->mobj->x;
			dest.y = laserwp->mobj->y;
			dest.z = laserwp->mobj->z;
			laserflip = (laserwp->mobj->flags2 & MF2_OBJECTFLIP) ? true : false; // K_RespawnOffset wants a boolean!

			if (laserflip == true)
			{
				dest.z -= (128 * mapobjectscale) - (player->mo->height);
			}
			else
			{
				dest.z += (128 * mapobjectscale);
			}

			stepamt -= dist;

			stepha = R_PointToAngle2(laser.x, laser.y, dest.x, dest.y);
			stepva = R_PointToAngle2(0, laser.z, P_AproxDistance(laser.x - dest.x, laser.y - dest.y), dest.z);

			step.x = FixedMul(FixedMul(FINECOSINE(stepha >> ANGLETOFINESHIFT), stepamt), FINECOSINE(stepva >> ANGLETOFINESHIFT));
			step.y = FixedMul(FixedMul(FINESINE(stepha >> ANGLETOFINESHIFT), stepamt), FINECOSINE(stepva >> ANGLETOFINESHIFT));
			step.z = FixedMul(FINESINE(stepva >> ANGLETOFINESHIFT), 2*stepamt);
		}
		else if (stepamt != realstepamt)
		{
			// Reset back to default
			stepamt = realstepamt;

			step.x = FixedMul(FixedMul(FINECOSINE(stepha >> ANGLETOFINESHIFT), stepamt), FINECOSINE(stepva >> ANGLETOFINESHIFT));
			step.y = FixedMul(FixedMul(FINESINE(stepha >> ANGLETOFINESHIFT), stepamt), FINECOSINE(stepva >> ANGLETOFINESHIFT));
			step.z = FixedMul(FINESINE(stepva >> ANGLETOFINESHIFT), 2*stepamt);
		}

		if (stepamt > 0)
		{
			laser.x += step.x;
			laser.y += step.y;
			laser.z += step.z;
		}

		dist = P_AproxDistance(P_AproxDistance(
			laser.x - dest.x,
			laser.y - dest.y),
			laser.z - dest.z
		);
		fulldist = dist + (laserdist * FRACUNIT);

		lasersteps--;
	}

	// Respawning after death: everything about the player
	// is invisible
	if (!player->respawn.truedeath && lasersteps == 0) // Don't spawn them beyond the respawn point.
	{
		mobj_t *lasermo = P_SpawnMobj(laser.x, laser.y, laser.z + (player->mo->height / 2), MT_DEZLASER);

		if (lasermo && !P_MobjWasRemoved(lasermo))
		{
			P_SetMobjState(lasermo, S_DEZLASER_TRAIL1);

			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				lasermo->eflags |= MFE_VERTICALFLIP;
			}

			P_SetTarget(&lasermo->target, player->mo);

			lasermo->angle = stepha + ANGLE_90;
			P_SetScale(lasermo, (lasermo->destscale = player->mo->scale));
		}
	}
}

/*--------------------------------------------------
	static void K_HandleDropDash(player_t *player)

		Handles the visuals for the waiting period,
		before you're allowed to Drop Dash.

	Input Arguments:-
		player - Player to preform for.

	Return:-
		None
--------------------------------------------------*/
static void K_DropDashWait(player_t *player)
{
	if (player->nocontrol == 0)
		player->respawn.timer--;

	if (player->pflags & PF_FAULT)
		return;

	if (leveltime % 8 == 0)
	{
		const UINT8 ns = 8;
		const angle_t sidediff = FixedAngle((360 / ns) * FRACUNIT);
		UINT8 i;

		if (!mapreset)
		{
			S_StartSound(player->mo, sfx_s3kcas);
		}

		for (i = 0; i < ns; i++)
		{
			const angle_t newangle = sidediff * i;
			vector3_t spawn;
			mobj_t *laser;

			spawn.x = player->mo->x + P_ReturnThrustX(player->mo, newangle, 31 * player->mo->scale);
			spawn.y = player->mo->y + P_ReturnThrustY(player->mo, newangle, 31 * player->mo->scale);

			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				spawn.z = player->mo->z + player->mo->height;
			}
			else
			{
				spawn.z = player->mo->z;
			}

			laser = P_SpawnMobj(spawn.x, spawn.y, spawn.z, MT_DEZLASER);

			if (laser && !P_MobjWasRemoved(laser))
			{
				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					laser->eflags |= MFE_VERTICALFLIP;
				}

				P_SetTarget(&laser->target, player->mo);

				laser->angle = newangle + ANGLE_90;
				laser->momz = (8 * player->mo->scale) * P_MobjFlip(player->mo);
				P_SetScale(laser, (laser->destscale = player->mo->scale));
			}
		}
	}
}


/*--------------------------------------------------
	static boolean K_CanDropDash(player_t *player)

		Checks if you can use the Drop Dash maneuver.

	Input Arguments:-
		player - Player to check.

	Return:-
		Whether a Drop Dash should be allowed.
--------------------------------------------------*/
static boolean K_CanDropDash(player_t *player)
{
	const UINT16 buttons = K_GetKartButtons(player);

	if (!(buttons & BT_ACCELERATE))
	{
		return false;
	}

	// Since we're letting players spin out on respawn, don't let them charge a dropdash in this state. (It wouldn't work anyway)
	if (player->spinouttimer)
	{
		return false;
	}

	// Garden Top is overpowered enough
	if (player->curshield == KSHIELD_TOP)
	{
		return false;
	}

	return true;
}


/*--------------------------------------------------
	static void K_HandleDropDash(player_t *player)

		Handles input for the Drop Dash maneuver.

	Input Arguments:-
		player - Player to preform for.

	Return:-
		None
--------------------------------------------------*/
static void K_HandleDropDash(player_t *player)
{
	const UINT16 buttons = K_GetKartButtons(player);

	if (player->growshrinktimer < 0)
	{
		player->mo->scalespeed = mapobjectscale/TICRATE;
		player->mo->destscale = FixedMul(mapobjectscale, SHRINK_SCALE);

		if (K_PlayerShrinkCheat(player) == true)
		{
			player->mo->destscale = FixedMul(player->mo->destscale, SHRINK_SCALE);
		}
	}

	if (!P_IsObjectOnGround(player->mo))
	{
		if (mapreset)
		{
			return;
		}

		player->flashing = K_GetKartFlashing(player);

		// The old behavior was stupid and prone to accidental usage.
		// Let's rip off Mania instead, and turn this into a Drop Dash!

		if (K_CanDropDash(player))
		{
			player->respawn.dropdash++;
		}
		else
		{
			player->respawn.dropdash = 0;
		}

		if (player->respawn.dropdash == TICRATE/4)
		{
			S_StartSound(player->mo, sfx_ddash);
		}

		if ((player->respawn.dropdash >= TICRATE/4) && (player->respawn.dropdash & 1))
		{
			player->mo->colorized = true;
		}
		else
		{
			player->mo->colorized = false;
		}
		// if player got trapped inside a bubble but lost its bubble object in a unintended way, remove no gravity flag
		if (((P_MobjWasRemoved(player->mo->tracer) || player->mo->tracer == NULL || (!P_MobjWasRemoved(player->mo->tracer) && player->mo->tracer && player->mo->tracer->type != MT_BUBBLESHIELDTRAP)) && player->carry == CR_TRAPBUBBLE) && (player->mo->flags & MF_NOGRAVITY))
		{
			player->mo->flags &= ~MF_NOGRAVITY;
			player->carry = CR_NONE;
		}
	}
	else
	{
		if ((buttons & BT_ACCELERATE) && (player->respawn.dropdash >= TICRATE/4))
		{
			S_StartSound(player->mo, sfx_s23c);
			player->dropdashboost = 50;
			K_SpawnDashDustRelease(player);
		}

		player->mo->colorized = false;
		player->respawn.dropdash = 0;

		//P_PlayRinglossSound(player->mo);
		P_PlayerRingBurst(player, 3);

		player->respawn.state = RESPAWNST_NONE;

		player->mo->flags &= ~(MF_NOCLIPTHING);

		// Don't touch another Ring Shooter (still lets you summon a Ring Shooter yourself)
		player->freeRingShooterCooldown = 2*TICRATE;
	}
}

/*--------------------------------------------------
	void K_RespawnChecker(player_t *player)

		See header file for description.
--------------------------------------------------*/
void K_RespawnChecker(player_t *player)
{
	if (player->respawn.state == RESPAWNST_NONE)
	{
		return;
	}

	if (player->spectator)
	{
		player->respawn.state = RESPAWNST_NONE;
		return;
	}

	switch (player->respawn.state)
	{
		case RESPAWNST_MOVE:
			player->mo->momx = player->mo->momy = player->mo->momz = 0;
			K_MovePlayerToRespawnPoint(player);
			return;
		case RESPAWNST_DROP:
			player->respawn.fromRingShooter = false;
			player->mo->momx = player->mo->momy = 0;
			player->flashing = 3;
			if (player->respawn.timer > 0)
			{
				player->mo->momz = 0;
				K_DropDashWait(player);
			}
			else
			{
				K_HandleDropDash(player);
			}
			return;
		default:
			player->respawn.state = RESPAWNST_NONE;
			return;
	}
}
