// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Kart Krew
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
		player->mo->flags2 |= MF2_OBJECTFLIP;
		player->mo->eflags |= MFE_VERTICALFLIP;
		z -= (128 * mapobjectscale) - (player->mo->height);
	}
	else
	{
		player->mo->flags2 &= ~MF2_OBJECTFLIP;
		player->mo->eflags &= ~MFE_VERTICALFLIP;
		z += (128 * mapobjectscale);
	}

	return z;
}

/*--------------------------------------------------
	static void K_RespawnAtWaypoint(player_t *player, waypoint_t *waypoint)

		Updates a player's respawn variables to go to the provided waypoint.

	Input Arguments:-
		player - Player to preform for.
		waypoint - Waypoint to respawn to.

	Return:-
		None
--------------------------------------------------*/
static void K_RespawnAtWaypoint(player_t *player, waypoint_t *waypoint)
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
	player->respawn.flip = (waypoint->mobj->flags2 & MF2_OBJECTFLIP);
	player->respawn.pointz += K_RespawnOffset(player, player->respawn.flip);
}

/*--------------------------------------------------
	void K_DoIngameRespawn(player_t *player)

		See header file for description.
--------------------------------------------------*/
void K_DoIngameRespawn(player_t *player)
{
	if (!player->mo || P_MobjWasRemoved(player->mo))
	{
		return;
	}

	if (player->respawn.state != RESPAWNST_NONE)
	{
		return;
	}

	if (leveltime < introtime)
	{
		return;
	}

	if (leveltime < starttime)
	{
		player->powers[pw_nocontrol] = (starttime - leveltime) + 50;
		player->pflags |= PF_SKIDDOWN; // cheeky pflag reuse
		S_StartSound(player->mo, sfx_s3k83);
		player->karthud[khud_fault] = 1;
		player->mo->momx = player->mo->momy = 0;
	}

	player->kartstuff[k_ringboost] = 0;
	player->kartstuff[k_driftboost] = 0;
	player->kartstuff[k_drift] = 0;
	player->kartstuff[k_driftcharge] = 0;
	player->kartstuff[k_pogospring] = 0;

	// Set up respawn position if invalid
	if (player->respawn.wp != NULL && leveltime >= starttime)
	{
		const UINT32 dist = RESPAWN_DIST + (player->airtime * 48);
		player->respawn.distanceleft = (dist * mapobjectscale) / FRACUNIT;
		K_RespawnAtWaypoint(player, player->respawn.wp);
	}
	else
	{
		UINT32 bestdist = UINT32_MAX;
		mapthing_t *beststart = NULL;
		UINT8 numstarts = 0;

		if (G_RaceGametype())
		{
			numstarts = numcoopstarts;
		}
		else if (G_BattleGametype())
		{
			numstarts = numdmstarts;
		}

		if (numstarts > 0)
		{
			UINT8 i = 0;

			for (i = 0; i < numstarts; i++)
			{
				UINT32 dist = UINT32_MAX;
				mapthing_t *checkstart = NULL;

				if (G_RaceGametype())
				{
					checkstart = playerstarts[i];
				}
				else if (G_BattleGametype())
				{
					checkstart = deathmatchstarts[i];
				}
				else
				{
					break;
				}

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
			player->respawn.flip = false;
		}
		else
		{
			sector_t *s;
			fixed_t z = (beststart->options >> ZSHIFT) * FRACUNIT;

			player->respawn.pointx = beststart->x << FRACBITS;
			player->respawn.pointy = beststart->y << FRACBITS;

			s = R_PointInSubsector(beststart->x << FRACBITS, beststart->y << FRACBITS)->sector;

			player->respawn.flip = (beststart->options & MTF_OBJECTFLIP);

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
}

/*--------------------------------------------------
	static size_t K_NextRespawnWaypointIndex(waypoint_t *waypoint)

		Returns the index for the next respawn waypoint.

	Input Arguments:-
		waypoint - Waypoint to look after.

	Return:-
		An table index for waypoint_t -> nextwaypoints.
--------------------------------------------------*/
static size_t K_NextRespawnWaypointIndex(waypoint_t *waypoint)
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
	const fixed_t realstepamt = (64 * mapobjectscale);
	fixed_t stepamt = realstepamt;

	vertex_t dest, step, laser;
	angle_t stepha, stepva;
	fixed_t dist, fulldist;

	UINT8 lasersteps = 4;
	UINT32 laserdist;
	waypoint_t *laserwp;
	boolean laserflip;

	player->mo->momx = player->mo->momy = player->mo->momz = 0;

	player->powers[pw_flashing] = 2;
	player->powers[pw_nocontrol] = 2;

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

	if (dist <= stepamt)
	{
		// Reduce by the amount we needed to get to this waypoint
		stepamt -= dist;

		// We've reached the destination point, 
		P_UnsetThingPosition(player->mo);
		player->mo->x = dest.x;
		player->mo->y = dest.y;
		player->mo->z = dest.z;
		P_SetThingPosition(player->mo);

		// Find the next waypoint to head towards
		if (player->respawn.wp != NULL)
		{
			size_t nwp = K_NextRespawnWaypointIndex(player->respawn.wp);

			if (nwp == SIZE_MAX)
			{
				player->respawn.state = RESPAWNST_DROP;
				return;
			}

			// Set angle, regardless of if we're done or not
			player->frameangle = R_PointToAngle2(
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

			player->respawn.wp = player->respawn.wp->nextwaypoints[nwp];
			K_RespawnAtWaypoint(player, player->respawn.wp);

			dest.x = player->respawn.pointx;
			dest.y = player->respawn.pointy;
			dest.z = player->respawn.pointz;
		}
		else
		{
			// We can now drop!
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
	player->frameangle = stepha;

	step.x = FixedMul(FixedMul(FINECOSINE(stepha >> ANGLETOFINESHIFT), stepamt), FINECOSINE(stepva >> ANGLETOFINESHIFT));
	step.y = FixedMul(FixedMul(FINESINE(stepha >> ANGLETOFINESHIFT), stepamt), FINECOSINE(stepva >> ANGLETOFINESHIFT));
	step.z = FixedMul(FINESINE(stepva >> ANGLETOFINESHIFT), 2*stepamt);

	if (stepamt > 0)
	{
		player->mo->momx = step.x;
		player->mo->momy = step.y;
		player->mo->momz = step.z;
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
			laserflip = (laserwp->mobj->flags2 & MF2_OBJECTFLIP);

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

	if (lasersteps == 0) // Don't spawn them beyond the respawn point.
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
	player->respawn.timer--;

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
			vertex_t spawn;
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
	static void K_HandleDropDash(player_t *player)

		Handles input for the Drop Dash maneuver.

	Input Arguments:-
		player - Player to preform for.

	Return:-
		None
--------------------------------------------------*/
static void K_HandleDropDash(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;

	if (player->kartstuff[k_growshrinktimer] < 0)
	{
		player->mo->scalespeed = mapobjectscale/TICRATE;
		player->mo->destscale = (6*mapobjectscale)/8;

		if (cv_kartdebugshrink.value && !modeattacking && !player->bot)
		{
			player->mo->destscale = (6*player->mo->destscale)/8;
		}
	}

	if (!P_IsObjectOnGround(player->mo))
	{
		if (mapreset)
		{
			return;
		}

		player->powers[pw_flashing] = K_GetKartFlashing(player);

		// The old behavior was stupid and prone to accidental usage.
		// Let's rip off Mania instead, and turn this into a Drop Dash!

		if ((cmd->buttons & BT_ACCELERATE) && !player->kartstuff[k_spinouttimer]) // Since we're letting players spin out on respawn, don't let them charge a dropdash in this state. (It wouldn't work anyway)
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
	}
	else
	{
		if ((cmd->buttons & BT_ACCELERATE) && (player->respawn.dropdash >= TICRATE/4))
		{
			S_StartSound(player->mo, sfx_s23c);
			player->kartstuff[k_startboost] = 50;
			K_SpawnDashDustRelease(player);
		}

		player->mo->colorized = false;
		player->respawn.dropdash = 0;

		//P_PlayRinglossSound(player->mo);
		P_PlayerRingBurst(player, 3);

		if (G_BattleGametype())
		{
			if (player->kartstuff[k_bumper] > 0)
			{
				if (player->kartstuff[k_bumper] == 1)
				{
					mobj_t *karmahitbox = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_KARMAHITBOX); // Player hitbox is too small!!
					P_SetTarget(&karmahitbox->target, player->mo);
					karmahitbox->destscale = player->mo->scale;
					P_SetScale(karmahitbox, player->mo->scale);
					CONS_Printf(M_GetText("%s lost all of their bumpers!\n"), player_names[player-players]);
				}
				player->kartstuff[k_bumper]--;
				if (K_IsPlayerWanted(player))
					K_CalculateBattleWanted();
			}

			if (!player->kartstuff[k_bumper])
			{
				player->kartstuff[k_comebacktimer] = comebacktime;
				if (player->kartstuff[k_comebackmode] == 2)
				{
					mobj_t *poof = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_EXPLODE);
					S_StartSound(poof, mobjinfo[MT_KARMAHITBOX].seesound);
					player->kartstuff[k_comebackmode] = 0;
				}
			}

			K_CheckBumpers();
		}

		player->respawn.state = RESPAWNST_NONE;
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
			player->mo->momx = player->mo->momy = 0;
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
