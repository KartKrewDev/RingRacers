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

// Battle overtime info
struct battleovertime battleovertime;

// Capsules mode enabled for this map?
boolean battlecapsules = false;

// box respawning in battle mode
INT32 nummapboxes = 0;
INT32 numgotboxes = 0;

// Capsule counters
UINT8 maptargets = 0; // Capsules in map
UINT8 numtargets = 0; // Capsules busted

INT32 K_StartingBumperCount(void)
{
	if (battlecapsules)
		return 1; // always 1 hit in Break the Capsules

	return cv_kartbumpers.value;
}

boolean K_IsPlayerWanted(player_t *player)
{
#if 1
	return (player->kartstuff[k_position] == 1);
#else
	UINT8 i;

	if (!(gametyperules & GTR_WANTED))
		return false;

	for (i = 0; i < 4; i++)
	{
		if (battlewanted[i] == -1)
			break;
		if (player == &players[battlewanted[i]])
			return true;
	}
	return false;
#endif
}

void K_CalculateBattleWanted(void)
{
	UINT8 numingame = 0, numwanted = 0;
	SINT8 camppos[MAXPLAYERS]; // who is the biggest camper
	UINT8 ties = 0, nextcamppos = 0;
	UINT8 i, j;

#if 0
	if (!(gametyperules & GTR_WANTED))
#endif
	{
		memset(battlewanted, -1, sizeof (battlewanted));
		return;
	}

	wantedcalcdelay = wantedfrequency;
	memset(camppos, -1, sizeof (camppos)); // initialize

	for (i = 0; i < MAXPLAYERS; i++)
	{
		UINT8 position = 1;

		if (!playeringame[i] || players[i].spectator) // Not playing
			continue;

		if (players[i].exiting) // We're done, don't calculate.
			return;

		if (players[i].bumpers <= 0) // Not alive, so don't do anything else
			continue;

		numingame++;

		for (j = 0; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j] || players[j].spectator)
				continue;

			if (players[j].bumpers <= 0)
				continue;

			if (j == i)
				continue;

			if (K_NumEmeralds(&players[j]) > K_NumEmeralds(&players[i]))
			{
				position++;
			}
			else if (players[j].bumpers > players[i].bumpers)
			{
				position++;
			}
			else if (players[j].marescore > players[i].marescore)
			{
				position++;
			}
			else if (players[j].kartstuff[k_wanted] > players[i].kartstuff[k_wanted])
			{
				position++;
			}
		}

		position--; // Make zero based

		while (camppos[position] != -1) // Port priority!
			position++;

		camppos[position] = i;
	}

	if (numingame <= 2) // In 1v1s then there's no need for WANTED.
		numwanted = 0;
	else
		numwanted = min(4, 1 + ((numingame-2) / 4));

	for (i = 0; i < 4; i++)
	{
		if (i+1 > numwanted) // Not enough players for this slot to be wanted!
		{
			battlewanted[i] = -1;
		}
		else
		{
			// Do not add *any* more people if there's too many times that are tied with others.
			// This could theoretically happen very easily if people don't hit each other for a while after the start of a match.
			// (I will be sincerely impressed if more than 2 people tie after people start hitting each other though)

			if (camppos[nextcamppos] == -1 // Out of entries
				|| ties >= (numwanted-i)) // Already counted ties
			{
				battlewanted[i] = -1;
				continue;
			}

			if (ties < (numwanted-i))
			{
				ties = 0; // Reset
				for (j = 0; j < 2; j++)
				{
					if (camppos[nextcamppos+(j+1)] == -1) // Nothing beyond, cancel
						break;
					if (players[camppos[nextcamppos]].kartstuff[k_wanted] == players[camppos[nextcamppos+(j+1)]].kartstuff[k_wanted])
						ties++;
				}
			}

			if (ties < (numwanted-i)) // Is it still low enough after counting?
			{
				battlewanted[i] = camppos[nextcamppos];
				nextcamppos++;
			}
			else
				battlewanted[i] = -1;
		}
	}
}

void K_SpawnBattlePoints(player_t *source, player_t *victim, UINT8 amount)
{
	statenum_t st;
	mobj_t *pt;

	if (!source || !source->mo)
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
}

void K_CheckBumpers(void)
{
	UINT8 i;
	UINT8 numingame = 0;
	SINT8 winnernum = -1;
	INT32 winnerscoreadd = 0;
	boolean nobumpers = false;

	if (!(gametyperules & GTR_BUMPERS))
		return;

	if (gameaction == ga_completed)
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator) // not even in-game
			continue;

		if (players[i].exiting) // we're already exiting! stop!
			return;

		numingame++;
		winnerscoreadd += players[i].marescore;

		if (players[i].bumpers <= 0) // if you don't have any bumpers, you're probably not a winner
		{
			nobumpers = true;
			continue;
		}
		else if (winnernum != -1) // TWO winners? that's dumb :V
			return;

		winnernum = i;
		winnerscoreadd -= players[i].marescore;
	}

	if (numingame <= 1)
	{
		if (!battlecapsules)
		{
			// Reset map to turn on battle capsules
			D_MapChange(gamemap, gametype, encoremode, true, 0, false, false);
		}
		else
		{
			if (nobumpers)
			{
				for (i = 0; i < MAXPLAYERS; i++)
				{
					players[i].pflags |= PF_GAMETYPEOVER;
					P_DoPlayerExit(&players[i]);
				}
			}
		}

		return;
	}

	if (winnernum > -1 && playeringame[winnernum])
	{
		players[winnernum].marescore += winnerscoreadd;
		CONS_Printf(M_GetText("%s recieved %d point%s for winning!\n"), player_names[winnernum], winnerscoreadd, (winnerscoreadd == 1 ? "" : "s"));
	}

	for (i = 0; i < MAXPLAYERS; i++) // This can't go in the earlier loop because winning adds points
		K_KartUpdatePosition(&players[i]);

	for (i = 0; i < MAXPLAYERS; i++) // and it can't be merged with this loop because it needs to be all updated before exiting... multi-loops suck...
		P_DoPlayerExit(&players[i]);
}

void K_CheckEmeralds(player_t *player)
{
	UINT8 i;

	if (!ALLCHAOSEMERALDS(player->powers[pw_emeralds]))
	{
		return;
	}

	player->marescore++; // lol

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		if (&players[i] == player)
		{
			continue;
		}

		players[i].bumpers = 0;
	}

	K_CheckBumpers();
}

mobj_t *K_SpawnChaosEmerald(mobj_t *parent, angle_t angle, SINT8 flip, UINT32 emeraldType)
{
	boolean validEmerald = true;
	mobj_t *emerald = P_SpawnMobjFromMobj(parent, 0, 0, 0, MT_EMERALD);
	mobj_t *overlay;

	P_Thrust(emerald,
		FixedAngle(P_RandomFixed() * 180) + angle,
		32 * mapobjectscale);

	emerald->momz = flip * 24 * mapobjectscale;
	if (emerald->eflags & MFE_UNDERWATER)
		emerald->momz = (117 * emerald->momz) / 200;

	emerald->threshold = 10;

	switch (emeraldType)
	{
		case EMERALD_CHAOS1:
			emerald->color = SKINCOLOR_CHAOSEMERALD1;
			break;
		case EMERALD_CHAOS2:
			emerald->color = SKINCOLOR_CHAOSEMERALD2;
			break;
		case EMERALD_CHAOS3:
			emerald->color = SKINCOLOR_CHAOSEMERALD3;
			break;
		case EMERALD_CHAOS4:
			emerald->color = SKINCOLOR_CHAOSEMERALD4;
			break;
		case EMERALD_CHAOS5:
			emerald->color = SKINCOLOR_CHAOSEMERALD5;
			break;
		case EMERALD_CHAOS6:
			emerald->color = SKINCOLOR_CHAOSEMERALD6;
			break;
		case EMERALD_CHAOS7:
			emerald->color = SKINCOLOR_CHAOSEMERALD7;
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

	return emerald;
}

void K_DropEmeraldsFromPlayer(player_t *player, UINT32 emeraldType)
{
	UINT8 i;
	SINT8 flip = P_MobjFlip(player->mo);

	for (i = 0; i < 14; i++)
	{
		UINT32 emeraldFlag = (1 << i);

		if ((player->powers[pw_emeralds] & emeraldFlag) && (emeraldFlag & emeraldType))
		{
			mobj_t *emerald = K_SpawnChaosEmerald(player->mo, player->mo->angle - ANGLE_90, flip, emeraldFlag);
			P_SetTarget(&emerald->target, player->mo);

			player->powers[pw_emeralds] &= ~emeraldFlag;
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

		if (player->powers[pw_emeralds] & emeraldFlag)
		{
			num++;
		}
	}

	return num;
}

void K_RunPaperItemSpawners(void)
{
	const boolean overtime = (battleovertime.enabled >= 10*TICRATE);
	tic_t interval = 8*TICRATE;
	UINT32 emeraldsSpawned = 0;

	if (leveltime < starttime)
	{
		// Round hasn't started yet!
		return;
	}

	if ((battleovertime.enabled > 0) && (battleovertime.radius < 256*mapobjectscale))
	{
		// Barrier has closed in too much
		return;
	}

	if (overtime == true)
	{
		// Double frequency of items
		interval /= 2;

		// Even if this isn't true, we pretend it is, because it's too late to do anything about it :p
		emeraldsSpawned = EMERALD_ALLCHAOS;
	}

	if (((leveltime - starttime) % interval) != 0)
	{
		return;
	}

	if (overtime == true)
	{
		SINT8 flip = 1;

		K_CreatePaperItem(
			battleovertime.x, battleovertime.y, battleovertime.z + (128 * mapobjectscale * flip),
			FixedAngle(P_RandomRange(0, 359) * FRACUNIT), flip,
			0, 0
		);
	}
	else
	{
		UINT8 pcount = 0;
		INT16 i;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
			{
				continue;
			}

			emeraldsSpawned |= players[i].powers[pw_emeralds];

			if ((players[i].exiting > 0 || players[i].eliminated)
				|| ((gametyperules & GTR_BUMPERS) && players[i].bumpers <= 0))
			{
				continue;
			}

			pcount++;
		}

		if (pcount > 0)
		{
#define MAXITEM 64
			UINT8 item = 0;
			mobj_t *spotList[MAXITEM];
			boolean spotUsed[MAXITEM];

			UINT32 firstUnspawnedEmerald = 0;
			INT16 starti = 0;

			thinker_t *th;
			mobj_t *mo;

			memset(spotUsed, false, sizeof(spotUsed));

			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				mo = (mobj_t *)th;

				if (mo->type == MT_PAPERITEMSPOT)
				{
					if (item >= MAXITEM)
						continue;

					spotList[item] = mo;
					item++;
				}
				else if (mo->type == MT_EMERALD)
				{
					emeraldsSpawned |= mo->extravalue1;
				}
			}

			if (item <= 0)
			{
				return;
			}

			for (i = 0; i < 7; i++)
			{
				UINT32 emeraldFlag = (1 << i);

				if (!(emeraldsSpawned & emeraldFlag))
				{
					firstUnspawnedEmerald = emeraldFlag;
					starti = -1;
					break;
				}
			}

			for (i = starti; i < min(item + starti, pcount); i++)
			{
				UINT8 r = P_RandomKey(item);
				UINT8 recursion = 0;
				mobj_t *drop = NULL;
				SINT8 flip = 1;

				while (spotUsed[r] == true)
				{
					r = P_RandomKey(item);

					if ((recursion++) > MAXITEM)
					{
						// roll with it anyway I guess
						break;
					}
				}

				flip = P_MobjFlip(spotList[r]);

				// When -1, we're spawning a Chaos Emerald.
				if (i == -1)
				{
					drop = K_SpawnChaosEmerald(
						spotList[r],
						FixedAngle(P_RandomRange(0, 359) * FRACUNIT), flip,
						firstUnspawnedEmerald
					);
				}
				else
				{
					drop = K_CreatePaperItem(
						spotList[r]->x, spotList[r]->y, spotList[r]->z + (128 * mapobjectscale * flip),
						FixedAngle(P_RandomRange(0, 359) * FRACUNIT), flip,
						0, 0
					);
				}

				K_FlipFromObject(drop, spotList[r]);
				spotUsed[r] = true;
			}
		}
	}
}

static void K_SpawnOvertimeLaser(fixed_t x, fixed_t y, fixed_t scale)
{
	UINT8 i, j;

	for (i = 0; i <= r_splitscreen; i++)
	{
		player_t *player = &players[displayplayers[i]];
		fixed_t zpos;
		SINT8 flip;

		if (player == NULL || player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
		{
			continue;
		}

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			zpos = player->mo->z + player->mo->height;
		}
		else
		{
			zpos = player->mo->z;
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
			mo->drawflags |= (MFD_DONTDRAW & ~(K_GetPlayerDontDrawFlag(player)));

			P_SetScale(mo, scale);

			switch (j)
			{
				case 0:
					P_SetMobjState(mo, S_OVERTIME_BULB1);

					if (leveltime & 1)
						mo->frame += 1;

					//P_SetScale(mo, mapobjectscale);
					zpos += 35 * mo->scale * flip;
					break;
				case 1:
					P_SetMobjState(mo, S_OVERTIME_LASER);

					if (leveltime & 1)
						mo->frame += 3;
					else
						mo->frame += (leveltime / 2) % 3;

					//P_SetScale(mo, scale);
					zpos += 346 * mo->scale * flip;

					if (battleovertime.enabled < 10*TICRATE)
						mo->drawflags |= MFD_TRANS50;
					break;
				case 2:
					P_SetMobjState(mo, S_OVERTIME_BULB2);

					if (leveltime & 1)
						mo->frame += 1;

					//P_SetScale(mo, mapobjectscale);
					break;
				default:
					I_Error("Bruh moment has occured\n");
					return;
			}
		}
	}
}

void K_RunBattleOvertime(void)
{
	if (battleovertime.enabled < 10*TICRATE)
	{
		battleovertime.enabled++;
		if (battleovertime.enabled == TICRATE)
			S_StartSound(NULL, sfx_bhurry);
		if (battleovertime.enabled == 10*TICRATE)
			S_StartSound(NULL, sfx_kc40);
	}
	else if (battleovertime.radius > 0)
	{
		if (battleovertime.radius > 2*mapobjectscale)
			battleovertime.radius -= 2*mapobjectscale;
		else
			battleovertime.radius = 0;
	}

	if (battleovertime.radius > 0)
	{
		const fixed_t pi = (22 * FRACUNIT) / 7; // loose approximation, this doesn't need to be incredibly precise
		const INT32 orbs = 32;
		const angle_t angoff = ANGLE_MAX / orbs;
		const UINT8 spriteSpacing = 128;

		fixed_t circumference = FixedMul(pi, battleovertime.radius * 2);
		fixed_t scale = max(circumference / spriteSpacing / orbs, mapobjectscale);

		fixed_t size = FixedMul(mobjinfo[MT_OVERTIME_PARTICLE].radius, scale);
		fixed_t posOffset = max(battleovertime.radius - size, 0);

		UINT32 i;

		for (i = 0; i < orbs; i++)
		{
			angle_t ang = (i * angoff) + FixedAngle((leveltime * FRACUNIT) / 4);

			fixed_t x = battleovertime.x + P_ReturnThrustX(NULL, ang, posOffset);
			fixed_t y = battleovertime.y + P_ReturnThrustY(NULL, ang, posOffset);

			K_SpawnOvertimeLaser(x, y, scale);
		}
	}
}

void K_SetupMovingCapsule(mapthing_t *mt, mobj_t *mobj)
{
	UINT8 sequence = mt->args[0] - 1;
	fixed_t speed = (FRACUNIT >> 3) * mt->args[1];
	boolean backandforth = (mt->options & MTF_AMBUSH);
	boolean reverse = (mt->options & MTF_OBJECTSPECIAL);
	mobj_t *mo2;
	mobj_t *target = NULL;
	thinker_t *th;

	// TODO: This and the movement stuff in the thinker should both be using
	// 2.2's new optimized functions for doing things with tube waypoints

	// Find the inital target
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type != MT_TUBEWAYPOINT)
			continue;

		if (mo2->threshold == sequence)
		{
			if (reverse) // Use the highest waypoint number as first
			{
				if (mo2->health != 0)
				{
					if (target == NULL)
						target = mo2;
					else if (mo2->health > target->health)
						target = mo2;
				}
			}
			else // Use the lowest waypoint number as first
			{
				if (mo2->health == 0)
					target = mo2;
			}
		}
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

void K_SpawnBattleCapsules(void)
{
	mapthing_t *mt;
	size_t i;

	if (battlecapsules)
		return;

	if (!(gametyperules & GTR_CAPSULES))
		return;

	if (modeattacking != ATTACKING_CAPSULES)
	{
		UINT8 n = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].spectator)
				n++;
			if (players[i].exiting)
				return;
			if (n > 1)
				break;
		}

		if (n > 1)
			return;
	}

	mt = mapthings;
	for (i = 0; i < nummapthings; i++, mt++)
	{
		if (mt->type == mobjinfo[MT_BATTLECAPSULE].doomednum)
			P_SpawnMapThing(mt);
	}

	battlecapsules = true;
}
