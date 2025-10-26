// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_battle.c
/// \brief SRB2Kart Battle Mode specific code

#include "k_battle.h"
#include "k_kart.h"
#include "doomtype.h"
#include "doomdata.h"
#include "g_game.h"
#include "p_mobj.h"
#include "p_local.h"
#include "p_setup.h"
#include "p_slopes.h" // P_GetZAt
#include "r_main.h"
#include "r_defs.h" // MAXFFLOORS
#include "info.h"
#include "s_sound.h"
#include "m_random.h"
#include "r_sky.h" // skyflatnum
#include "k_grandprix.h" // K_CanChangeRules
#include "k_boss.h" // bossinfo.valid
#include "p_spec.h"
#include "k_objects.h"
#include "k_rank.h"
#include "music.h"
#include "hu_stuff.h"
#include "m_easing.h"
#include "k_endcam.h"
#include "p_tick.h"

#define BARRIER_MIN_RADIUS (768 * mapobjectscale)

// Battle overtime info
struct battleovertime battleovertime;
struct battleufo g_battleufo;

// Capsules mode enabled for this map?
boolean battleprisons = false;

// box respawning in battle mode
INT32 nummapboxes = 0;
INT32 numgotboxes = 0;

// Capsule counters
UINT8 maptargets = 0; // Capsules in map
UINT8 numtargets = 0; // Capsules busted

// Battle: someone won by collecting all 7 Chaos Emeralds
tic_t g_emeraldWin = 0;

INT32 K_StartingBumperCount(void)
{
	if (tutorialchallenge == TUTORIALSKIP_INPROGRESS)
		return 0;

	if (battleprisons || K_CheckBossIntro() || !K_CanChangeRules(true))
	{
		if (grandprixinfo.gp)
		{
			switch (grandprixinfo.gamespeed)
			{
				case KARTSPEED_HARD:
					return (grandprixinfo.masterbots == true) ? 0 : 1;
				case KARTSPEED_NORMAL:
					return 2;
				case KARTSPEED_EASY:
					return 3;
			}

		}

		return 2; // Normal
	}

	return cv_kartbumpers.value;
}

boolean K_IsPlayerWanted(player_t *player)
{
	UINT8 i = 0, nump = 0, numfirst = 0;
	for (; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;
		nump++;
		if (players[i].position > 1)
			continue;
		numfirst++;
	}
	return ((numfirst < nump) && !player->spectator && (player->position == 1));
}

void K_SpawnBattlePoints(player_t *source, player_t *victim, UINT8 amount)
{
	statenum_t st;
	mobj_t *pt;

	if (!source || !source->mo)
		return;

	if (source->exiting)
		return;

	if (amount == 1)
		st = S_BATTLEPOINT1A;
	else if (amount == 2)
		st = S_BATTLEPOINT2A;
	else if (amount == 3)
		st = S_BATTLEPOINT3A;
	else
		return; // NO STATE!

	pt = P_SpawnMobj(source->mo->x, source->mo->y, source->mo->z, MT_BATTLEPOINT);
	P_SetTarget(&pt->target, source->mo);
	P_SetMobjState(pt, st);
	if (victim && victim->skincolor)
		pt->color = victim->skincolor;
	else
		pt->color = source->skincolor;

	if (encoremode)
		pt->renderflags ^= RF_HORIZONTALFLIP;
}

void K_CheckBumpers(void)
{
	UINT8 i;
	UINT8 numingame = 0;
	UINT8 rednumingame = 0;
	UINT8 bluenumingame = 0;
	UINT8 nobumpers = 0;
	UINT8 eliminated = 0;
	UINT8 redeliminated = 0;
	UINT8 blueeliminated = 0;
	SINT8 kingofthehill = -1;

	if (!(gametyperules & GTR_BUMPERS))
		return;

	if (gameaction == ga_completed)
		return;

	boolean team = G_GametypeHasTeams();

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator) // not even in-game
			continue;

		if (players[i].exiting) // we're already exiting! stop!
			return;

		numingame++;
		if (team)
		{
			if (players[i].team == 1) rednumingame++;
			if (players[i].team == 2) bluenumingame++;
		}

		if (!P_MobjWasRemoved(players[i].mo) && players[i].mo->health <= 0) // if you don't have any bumpers, you're probably not a winner
		{
			nobumpers++;
		}

		if (players[i].pflags & PF_ELIMINATED)
		{
			eliminated++;
			if (team)
			{
				if (players[i].team == 1) redeliminated++;
				else if (players[i].team == 2) blueeliminated++;
			}
		}
		else
		{
			kingofthehill = i;
		}
	}

	boolean teamwin = false;
	if (team && (rednumingame - redeliminated == 0 || bluenumingame - blueeliminated == 0))
	{
		teamwin = true;
	}

	if (numingame - eliminated == 2 && battleovertime.enabled && battleovertime.radius <= BARRIER_MIN_RADIUS)
	{
		Music_Stop("battle_overtime");
		S_StartSound(NULL, sfx_kc4b); // Loud noise helps mask transition
	}

	if (K_Cooperative())
	{
		if (nobumpers > 0 && nobumpers >= numingame)
		{
			P_DoAllPlayersExit(PF_NOCONTEST, false);
			return;
		}
	}
	else if (numingame > 1)
	{
		// If every other player is eliminated, the
		// last player standing wins by default.
		// Or, if an entire team is eliminated.
		if (eliminated >= numingame - 1 || teamwin)
		{
			if (teamwin)
			{
				// Find the player with the highest individual score
				UINT32 highestscore = 0;
				UINT32 highestplayer = 0;
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (playeringame[i] && players[i].score > highestscore)
					{
						highestplayer = i;
						highestscore = players[i].score;
					}
				}
				K_EndBattleRound(&players[highestplayer]);
			}
			else
			{
				K_EndBattleRound(kingofthehill != -1 ? &players[kingofthehill] : NULL);
			}
			return;
		}
	}
	else
	{
		if ((gametyperules & GTR_PRISONS) && !battleprisons && (K_CanChangeRules(true) == true))
		{
			// Reset map to turn on battle prisons
			if (server)
				D_MapChange(gamemap, gametype, encoremode, true, 1, false, false);
			return;
		}
	}
}

void K_CheckEmeralds(player_t *player)
{
	if (!(gametyperules & GTR_POWERSTONES))
	{
		return;
	}

	if (!ALLCHAOSEMERALDS(player->emeralds))
	{
		return;
	}

	if (!K_EndBattleRound(player))
	{
		return;
	}

	// TODO: this would be better if the timing lived in
	// Tally code. But I didn't do it that, so this just
	// shittily approximates syncing up with Tally.
	g_emeraldWin = leveltime + (3*TICRATE);

	if (!P_MobjWasRemoved(player->mo))
	{
		K_StartRoundWinCamera(
			player->mo,
			player->angleturn + ANGLE_180,
			400*mapobjectscale,
			6*TICRATE,
			FRACUNIT/16,
			3*TICRATE
		);

		g_emeraldWin += g_endcam.swirlDuration;
	}
}

UINT16 K_GetChaosEmeraldColor(UINT32 emeraldType)
{
	switch (emeraldType)
	{
		case EMERALD_CHAOS1:
			return SKINCOLOR_CHAOSEMERALD1;
		case EMERALD_CHAOS2:
			return SKINCOLOR_CHAOSEMERALD2;
		case EMERALD_CHAOS3:
			return SKINCOLOR_CHAOSEMERALD3;
		case EMERALD_CHAOS4:
			return SKINCOLOR_CHAOSEMERALD4;
		case EMERALD_CHAOS5:
			return SKINCOLOR_CHAOSEMERALD5;
		case EMERALD_CHAOS6:
			return SKINCOLOR_CHAOSEMERALD6;
		case EMERALD_CHAOS7:
			return SKINCOLOR_CHAOSEMERALD7;
		default:
			return SKINCOLOR_NONE;
	}
}

mobj_t *K_SpawnChaosEmerald(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT32 emeraldType)
{
	boolean validEmerald = true;
	mobj_t *emerald = P_SpawnMobj(x, y, z, MT_EMERALD);
	mobj_t *overlay;

	P_Thrust(emerald,
		FixedAngle(P_RandomFixed(PR_ITEM_SPAWNER) * 180) + angle,
		36 * mapobjectscale);

	emerald->momz = flip * 36 * mapobjectscale;
	if (emerald->eflags & MFE_UNDERWATER)
		emerald->momz = (117 * emerald->momz) / 200;

	emerald->threshold = 10;

	switch (emeraldType)
	{
		case EMERALD_CHAOS1:
		case EMERALD_CHAOS2:
		case EMERALD_CHAOS3:
		case EMERALD_CHAOS4:
		case EMERALD_CHAOS5:
		case EMERALD_CHAOS6:
		case EMERALD_CHAOS7:
			emerald->color = K_GetChaosEmeraldColor(emeraldType);
			break;
		default:
			CONS_Printf("Invalid emerald type %d\n", emeraldType);
			validEmerald = false;
			break;
	}

	if (validEmerald == true)
	{
		emerald->extravalue1 = emeraldType;
	}

	overlay = P_SpawnMobjFromMobj(emerald, 0, 0, 0, MT_OVERLAY);
	P_SetTarget(&overlay->target, emerald);
	P_SetMobjState(overlay, S_CHAOSEMERALD_UNDER);
	overlay->color = emerald->color;

	if (gametyperules & GTR_CLOSERPLAYERS)
	{
		emerald->fuse = BATTLE_DESPAWN_TIME;
	}

	return emerald;
}

mobj_t *K_SpawnSphereBox(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT8 amount)
{
	mobj_t *drop = P_SpawnMobj(x, y, z, MT_SPHEREBOX);
	fixed_t rand_move;
	angle_t rand_angle;

	drop->angle = angle;
	// note: determinate random argument eval order
	rand_move = P_RandomRange(PR_ITEM_SPAWNER, 4, 12) * mapobjectscale;
	rand_angle = FixedAngle(P_RandomFixed(PR_ITEM_SPAWNER) * 180) + angle;
	P_Thrust(drop, rand_angle, rand_move);

	drop->momz = flip * 12 * mapobjectscale;
	if (drop->eflags & MFE_UNDERWATER)
		drop->momz = (117 * drop->momz) / 200;

	drop->flags &= ~(MF_NOGRAVITY|MF_NOCLIPHEIGHT);

	drop->extravalue2 = amount;

	return drop;
}

void K_DropEmeraldsFromPlayer(player_t *player, UINT32 emeraldType)
{
	UINT8 i;
	SINT8 flip = P_MobjFlip(player->mo);

	if (player->incontrol < TICRATE)
		return;

	for (i = 0; i < 14; i++)
	{
		UINT32 emeraldFlag = (1 << i);

		if ((player->emeralds & emeraldFlag) && (emeraldFlag & emeraldType))
		{
			K_SpawnChaosEmerald(player->mo->x, player->mo->y, player->mo->z, player->mo->angle - ANGLE_90, flip, emeraldFlag);

			player->emeralds &= ~emeraldFlag;
			break; // Drop only one emerald. Emerald wins are hard enough!
		}
	}
}

UINT8 K_NumEmeralds(player_t *player)
{
	UINT8 i;
	UINT8 num = 0;

	for (i = 0; i < 14; i++)
	{
		UINT32 emeraldFlag = (1 << i);

		if (player->emeralds & emeraldFlag)
		{
			num++;
		}
	}

	return num;
}

static inline boolean IsOnInterval(tic_t interval)
{
	return ((leveltime - starttime) % interval) == 0;
}

static UINT32 CountEmeraldsSpawned(const mobj_t *mo)
{
	switch (mo->type)
	{
		case MT_EMERALD:
			return mo->extravalue1;

		case MT_MONITOR:
			return Obj_MonitorGetEmerald(mo);

		default:
			return 0U;
	}
}

void K_RunPaperItemSpawners(void)
{
	const boolean overtime = (battleovertime.enabled >= 10*TICRATE);
	const tic_t interval = BATTLE_SPAWN_INTERVAL;

	const boolean canmakeemeralds = (gametyperules & GTR_POWERSTONES);

	UINT32 emeraldsSpawned = 0;
	UINT32 firstUnspawnedEmerald = 0;

	thinker_t *th;
	mobj_t *mo;

	UINT8 pcount = 0;
	INT16 i;

	if (battleprisons)
	{
		// Gametype uses paper items, but this specific expression doesn't
		return;
	}

	if (leveltime < starttime)
	{
		// Round hasn't started yet!
		return;
	}

	if (leveltime == g_battleufo.due && overtime == false)
	{
		Obj_SpawnBattleUFOFromSpawner();
	}

	if (!IsOnInterval(interval))
	{
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		emeraldsSpawned |= players[i].emeralds;

		if ((players[i].exiting > 0 || (players[i].pflags & PF_ELIMINATED))
			|| ((gametyperules & GTR_BUMPERS) && !P_MobjWasRemoved(players[i].mo) && players[i].mo->health <= 0))
		{
			continue;
		}

		pcount++;
	}

	if (overtime == true)
	{
		SINT8 flip = 1;

		// Just find emeralds, no paper spots
		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			mo = (mobj_t *)th;

			emeraldsSpawned |= CountEmeraldsSpawned(mo);
		}

		if (canmakeemeralds)
		{
			for (i = 0; i < 7; i++)
			{
				UINT32 emeraldFlag = (1 << i);

				if (!(emeraldsSpawned & emeraldFlag))
				{
					firstUnspawnedEmerald = emeraldFlag;
					break;
				}
			}
		}

		if (firstUnspawnedEmerald != 0)
		{
			K_SpawnChaosEmerald(
				battleovertime.x, battleovertime.y, battleovertime.z + (128 * mapobjectscale * flip),
				FixedAngle(P_RandomRange(PR_ITEM_SPAWNER, 0, 359) * FRACUNIT), flip,
				firstUnspawnedEmerald
			);
		}
		else
		{
			K_FlingPaperItem(
				battleovertime.x, battleovertime.y, battleovertime.z + (128 * mapobjectscale * flip),
				FixedAngle(P_RandomRange(PR_ITEM_SPAWNER, 0, 359) * FRACUNIT), flip,
				0, 0
			);

			if (gametyperules & GTR_SPHERES)
			{
				K_SpawnSphereBox(
					battleovertime.x, battleovertime.y, battleovertime.z + (128 * mapobjectscale * flip),
					FixedAngle(P_RandomRange(PR_ITEM_SPAWNER, 0, 359) * FRACUNIT), flip,
					10
				);
			}
		}
	}
	else
	{
		if (pcount > 0)
		{
#define MAXITEM 64
			mobj_t *spotList[MAXITEM];
			UINT8 spotMap[MAXITEM];
			UINT8 spotCount = 0, spotBackup = 0, spotAvailable = 0;
			UINT8 monitorsSpawned = 0;

			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				mo = (mobj_t *)th;

				emeraldsSpawned |= CountEmeraldsSpawned(mo);

				if (mo->type != MT_PAPERITEMSPOT)
					continue;

				if (spotCount >= MAXITEM)
					continue;

				if (Obj_ItemSpotIsAvailable(mo))
				{
					// spotMap first only includes spots
					// where a monitor doesn't exist
					spotMap[spotAvailable] = spotCount;
					spotAvailable++;
				}
				else
				{
					monitorsSpawned++;
				}

				spotList[spotCount] = mo;
				spotCount++;
			}

			if (spotCount <= 0)
			{
				return;
			}

			if (canmakeemeralds)
			{
				for (i = 0; i < 7; i++)
				{
					UINT32 emeraldFlag = (1 << i);

					if (!(emeraldsSpawned & emeraldFlag))
					{
						firstUnspawnedEmerald = emeraldFlag;
						break;
					}
				}
			}

			//CONS_Printf("leveltime = %d ", leveltime);

			// Duel   =  2 + 1 =  3 / 2 = 1
			// Small  =  5 + 1 =  6 / 2 = 3
			// Medium = 10 + 1 = 11 / 2 = 5
			// Large  = 16 + 1 = 17 / 2 = 8
			if (spotAvailable > 0 && monitorsSpawned < (mapheaderinfo[gamemap - 1]->playerLimit + 1) / 2)
			{
				const UINT8 r = spotMap[P_RandomKey(PR_ITEM_SPAWNER, spotAvailable)];

				Obj_ItemSpotAssignMonitor(spotList[r], Obj_SpawnMonitor(
							spotList[r], 3, firstUnspawnedEmerald));
			}

			for (i = 0; i < spotCount; ++i)
			{
				// now spotMap includes every spot
				spotMap[i] = i;
			}

			if ((gametyperules & GTR_SPHERES) && IsOnInterval(16 * interval))
			{
				spotBackup = spotCount;
				for (i = 0; i < pcount; i++)
				{
					UINT8 r = 0, key = 0;
					mobj_t *drop = NULL;
					SINT8 flip = 1;

					if (spotCount == 0)
					{
						// all are accessible again
						spotCount = spotBackup;
					}

					if (spotCount == 1)
					{
						key = 0;
					}
					else
					{
						key = P_RandomKey(PR_ITEM_SPAWNER, spotCount);
					}

					r = spotMap[key];

					//CONS_Printf("[%d %d %d] ", i, key, r);

					flip = P_MobjFlip(spotList[r]);

					drop = K_SpawnSphereBox(
						spotList[r]->x, spotList[r]->y, spotList[r]->z + (128 * mapobjectscale),
							FixedAngle(P_RandomRange(PR_ITEM_SPAWNER, 0, 359) * FRACUNIT), flip,
							10
					);

					K_FlipFromObjectNoInterp(drop, spotList[r]);

					spotCount--;
					if (key != spotCount)
					{
						// So the core theory of what's going on is that we keep every
						// available option at the front of the array, so we don't have
						// to skip over any gaps or do recursion to avoid doubles.
						// But because spotCount can be reset in the case of a low
						// quanitity of item spawnpoints in a map, we still need every
						// entry in the array, even outside of the "visible" range.
						// A series of swaps allows us to adhere to both constraints.
						// -toast 22/03/22 (semipalindromic!)
						spotMap[key] = spotMap[spotCount];
						spotMap[spotCount] = r; // was set to spotMap[key] previously
					}
				}
			}
			//CONS_Printf("\n");
		}
	}
}

static void K_SpawnOvertimeLaser(fixed_t x, fixed_t y, fixed_t scale)
{
	const fixed_t heightPadding = 346 * scale;

	UINT8 i, j;

	for (i = 0; i <= r_splitscreen; i++)
	{
		camera_t *cam = &camera[i];
		player_t *player = &players[displayplayers[i]];
		fixed_t zpos;
		SINT8 flip;

		if (player == NULL || player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
		{
			continue;
		}

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			zpos = cam->z + player->mo->height;
			zpos = min(zpos + heightPadding, cam->centerceilingz);
		}
		else
		{
			zpos = cam->z;
			zpos = max(zpos - heightPadding, cam->centerfloorz);
		}

		flip = P_MobjFlip(player->mo);

		for (j = 0; j < 3; j++)
		{
			mobj_t *mo = P_SpawnMobj(x, y, zpos, MT_OVERTIME_PARTICLE);

			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				mo->flags2 |= MF2_OBJECTFLIP;
				mo->eflags |= MFE_VERTICALFLIP;
			}

			mo->angle = R_PointToAngle2(mo->x, mo->y, battleovertime.x, battleovertime.y) + ANGLE_90;
			mo->renderflags |= (RF_DONTDRAW & ~(K_GetPlayerDontDrawFlag(player))) | RF_HIDEINSKYBOX;

			P_SetScale(mo, scale);

			switch (j)
			{
				case 0:
					P_SetMobjState(mo, S_OVERTIME_BULB1);

					if (!cv_reducevfx.value)
					{
						if (leveltime & 1)
							mo->frame += 1;
					}

					//P_SetScale(mo, mapobjectscale);
					zpos += 35 * mo->scale * flip;
					break;
				case 1:
					P_SetMobjState(mo, S_OVERTIME_LASER);

					if (!cv_reducevfx.value)
					{
						if (leveltime & 1)
							mo->frame += 3;
						else
							mo->frame += (leveltime / 2) % 3;
					}

					//P_SetScale(mo, scale);
					zpos += 346 * mo->scale * flip;

					if (battleovertime.enabled < 10*TICRATE)
						mo->renderflags |= RF_TRANS50;
					break;
				case 2:
					P_SetMobjState(mo, S_OVERTIME_BULB2);

					if (!cv_reducevfx.value)
					{
						if (leveltime & 1)
							mo->frame += 1;
					}

					//P_SetScale(mo, mapobjectscale);
					break;
				default:
					I_Error("Bruh moment has occured\n");
					return;
			}
		}
	}
}

void K_SpawnOvertimeBarrier(void)
{
	if (battleovertime.radius <= 0)
	{
		return;
	}

	const INT32 orbs = 32;
	const angle_t angoff = ANGLE_MAX / orbs;
	const UINT8 spriteSpacing = 128;

	fixed_t circumference = FixedMul(M_PI_FIXED, battleovertime.radius * 2);
	fixed_t scale = max(circumference / spriteSpacing / orbs, mapobjectscale);

	fixed_t size = FixedMul(mobjinfo[MT_OVERTIME_PARTICLE].radius, scale);
	fixed_t posOffset = max(battleovertime.radius - size, 0);

	INT32 i;

	for (i = 0; i < orbs; i++)
	{
		angle_t ang = (i * angoff) + FixedAngle((leveltime * FRACUNIT) / 4);

		fixed_t x = battleovertime.x + P_ReturnThrustX(NULL, ang, posOffset);
		fixed_t y = battleovertime.y + P_ReturnThrustY(NULL, ang, posOffset);

		K_SpawnOvertimeLaser(x, y, scale);
	}
}

void K_RunBattleOvertime(void)
{
	if (battleovertime.enabled < 10*TICRATE)
	{
		battleovertime.enabled++;
		if (battleovertime.enabled == TICRATE)
		{
			S_StartSound(NULL, sfx_bhurry);
			HU_DoTitlecardCEchoForDuration(NULL, "HURRY UP!!", true, 2*TICRATE);
			Music_DelayEnd("level", 0);
		}
		else if (battleovertime.enabled == 10*TICRATE)
		{
			S_StartSound(NULL, sfx_kc40);
			P_StartQuake(5, 64 * mapobjectscale, 0, NULL);
			battleovertime.start = leveltime;
		}

		if (!Music_Playing("level") && !Music_Playing("battle_overtime"))
		{
			Music_Play("battle_overtime");
			Music_Play("battle_overtime_stress");

			// Sync approximately with looping section of
			// battle_overtime. (This is file dependant.)
			Music_Seek("battle_overtime_stress", 1756);
		}
	}
	else if (battleovertime.radius > 0)
	{
		const fixed_t minradius = BARRIER_MIN_RADIUS;
		const fixed_t oldradius = battleovertime.radius;

		if (battleovertime.radius > minradius)
		{
			extern consvar_t cv_barriertime;
			tic_t t = leveltime - battleovertime.start;
			const tic_t duration = cv_barriertime.value * TICRATE;
			battleovertime.radius = Easing_OutSine(min(t, duration) * FRACUNIT / duration, battleovertime.initial_radius, minradius);
		}

		if (battleovertime.radius <= minradius && oldradius > minradius)
		{
			battleovertime.radius = minradius;
			K_CheckBumpers();
			S_StartSound(NULL, sfx_kc40);
			P_StartQuake(5, 64 * mapobjectscale, 0, NULL);
		}

		// Subtract the 10 second grace period of the barrier
		if (battleovertime.enabled < 25*TICRATE)
		{
			battleovertime.enabled++;
			Obj_PointPlayersToXY(battleovertime.x, battleovertime.y);
		}
	}

	if (!P_LevelIsFrozen())
	{
		K_SpawnOvertimeBarrier();
	}
}

void K_SetupMovingCapsule(mapthing_t *mt, mobj_t *mobj)
{
	UINT8 sequence = mt->thing_args[0] - 1;
	fixed_t speed = (FRACUNIT >> 3) * mt->thing_args[1];
	boolean backandforth = (mt->thing_args[2] & TMBCF_BACKANDFORTH);
	boolean reverse = (mt->thing_args[2] & TMBCF_REVERSE);
	mobj_t *target = NULL;

	// Find the inital target
	if (reverse)
	{
		target = P_GetLastTubeWaypoint(sequence);
	}
	else
	{
		target = P_GetFirstTubeWaypoint(sequence);
	}

	if (!target)
	{
		CONS_Alert(CONS_WARNING, "No target waypoint found for moving capsule (seq: #%d)\n", sequence);
		return;
	}

	P_SetTarget(&mobj->target, target);
	mobj->lastlook = sequence;
	mobj->movecount = target->health;
	mobj->movefactor = speed;

	if (backandforth) {
		mobj->flags2 |= MF2_AMBUSH;
	} else {
		mobj->flags2 &= ~MF2_AMBUSH;
	}

	if (reverse) {
		mobj->cvmem = -1;
	} else {
		mobj->cvmem = 1;
	}
}

void K_SpawnPlayerBattleBumpers(player_t *p)
{
	const UINT8 bumpers = K_Bumpers(p);

	if (bumpers <= 0)
	{
		return;
	}

	{
		INT32 i;
		angle_t diff = FixedAngle(360*FRACUNIT / bumpers);
		angle_t newangle = p->mo->angle;
		mobj_t *bump;

		for (i = 0; i < bumpers; i++)
		{
			bump = P_SpawnMobjFromMobj(p->mo,
				P_ReturnThrustX(p->mo, newangle + ANGLE_180, 64*FRACUNIT),
				P_ReturnThrustY(p->mo, newangle + ANGLE_180, 64*FRACUNIT),
				0, MT_BATTLEBUMPER);
			bump->threshold = i;
			P_SetTarget(&bump->target, p->mo);
			bump->angle = newangle;
			bump->color = p->mo->color;
			if (p->mo->renderflags & RF_DONTDRAW)
				bump->renderflags |= RF_DONTDRAW;
			else
				bump->renderflags &= ~RF_DONTDRAW;
			newangle += diff;
		}
	}
}

void K_BattleInit(boolean singleplayercontext)
{
	size_t i;

	if ((gametyperules & GTR_PRISONS) && singleplayercontext && !battleprisons && !cv_battletest.value)
	{
		mapthing_t *mt = mapthings;
		for (i = 0; i < nummapthings; i++, mt++)
		{
			if (mt->type == mobjinfo[MT_BATTLECAPSULE].doomednum)
				P_SpawnMapThing(mt);
			else if (mt->type == mobjinfo[MT_CDUFO].doomednum)
				maptargets++;
		}

		battleprisons = true;
	}

	g_battleufo.due = starttime;
	g_battleufo.previousId = Obj_RandomBattleUFOSpawnerID() - 1;

	g_emeraldWin = 0;
}

UINT8 K_Bumpers(player_t *player)
{
	if ((gametyperules & GTR_BUMPERS) == 0)
	{
		return 0;
	}

	if (P_MobjWasRemoved(player->mo))
	{
		return 0;
	}

	if (player->mo->health < 1)
	{
		return 0;
	}

	if (player->mo->health > UINT8_MAX)
	{
		return UINT8_MAX;
	}

	return (player->mo->health - 1);
}

INT32 K_BumpersToHealth(UINT8 bumpers)
{
	return (bumpers + 1);
}

boolean K_BattleOvertimeKiller(mobj_t *mobj)
{
	if (battleovertime.enabled < 10*TICRATE)
	{
		return false;
	}

	fixed_t distance = R_PointToDist2(mobj->x, mobj->y, battleovertime.x, battleovertime.y);

	if (distance <= battleovertime.radius)
	{
		return false;
	}

	P_KillMobj(mobj, NULL, NULL, DMG_NORMAL);

	return true;
}

boolean K_EndBattleRound(player_t *victor)
{
	if (victor)
	{
		if (victor->exiting)
		{
			// In Battle, players always exit altogether.
			// So it can be assumed that if this player is
			// exiting, the round has already ended.
			return false;
		}
		
		UINT32 topscore = 0;

		if (gametyperules & GTR_POINTLIMIT)
		{
			// Lock the winner in before the round ends.

			// TODO: a "won the round" bool used for sorting
			// position / intermission, so we aren't completely
			// clobbering the individual scoring.
			
			// This isn't quite the above TODO but it's something?
			// For purposes of score-to-EXP conversion, we need to not lock the winner to an arbitrarily high score.
			// Instead, let's find the highest score, and if they're not the highest scoring player,
			// give them a bump so they *are* the highest scoring player.
			for (INT32 i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator)
				{
					continue;
				}
				
				if ((&players[i])->roundscore > topscore)
				{
					topscore = (&players[i])->roundscore;
				}
			}
			if (victor->roundscore <= topscore)
			{
				victor->roundscore = topscore + 3;
			}

			if (G_GametypeHasTeams() == true && victor->team != TEAM_UNASSIGNED)
			{
				g_teamscores[victor->team] = 100;
			}
		}
	}

	P_DoAllPlayersExit(0, false);

	return true;
}
