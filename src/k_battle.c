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
}

void K_CalculateBattleWanted(void)
{
	UINT8 numingame = 0, numplaying = 0, numwanted = 0;
	SINT8 bestbumperplayer = -1, bestbumper = -1;
	SINT8 camppos[MAXPLAYERS]; // who is the biggest camper
	UINT8 ties = 0, nextcamppos = 0;
	boolean setbumper = false;
	UINT8 i, j;

	if (!(gametyperules & GTR_WANTED))
	{
		for (i = 0; i < 4; i++)
			battlewanted[i] = -1;
		return;
	}

	wantedcalcdelay = wantedfrequency;

	for (i = 0; i < MAXPLAYERS; i++)
		camppos[i] = -1; // initialize

	for (i = 0; i < MAXPLAYERS; i++)
	{
		UINT8 position = 1;

		if (!playeringame[i] || players[i].spectator) // Not playing
			continue;

		if (players[i].exiting) // We're done, don't calculate.
			return;

		numplaying++;

		if (players[i].bumpers <= 0) // Not alive, so don't do anything else
			continue;

		numingame++;

		if (bestbumper == -1 || players[i].bumpers > bestbumper)
		{
			bestbumper = players[i].bumpers;
			bestbumperplayer = i;
		}
		else if (players[i].bumpers == bestbumper)
			bestbumperplayer = -1; // Tie, no one has best bumper.

		for (j = 0; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j] || players[j].spectator)
				continue;
			if (players[j].bumpers <= 0)
				continue;
			if (j == i)
				continue;
			if (players[j].kartstuff[k_wanted] == players[i].kartstuff[k_wanted] && players[j].marescore > players[i].marescore)
				position++;
			else if (players[j].kartstuff[k_wanted] > players[i].kartstuff[k_wanted])
				position++;
		}

		position--; // Make zero based

		while (camppos[position] != -1) // Port priority!
			position++;

		camppos[position] = i;
	}

	if (numplaying <= 2 || (numingame <= 2 && bestbumper == 1)) // In 1v1s then there's no need for WANTED. In bigger netgames, don't show anyone as WANTED when they're equally matched.
		numwanted = 0;
	else
		numwanted = min(4, 1 + ((numingame-2) / 4));

	for (i = 0; i < 4; i++)
	{
		if (i+1 > numwanted) // Not enough players for this slot to be wanted!
			battlewanted[i] = -1;
		else if (bestbumperplayer != -1 && !setbumper) // If there's a player who has an untied bumper lead over everyone else, they are the first to be wanted.
		{
			battlewanted[i] = bestbumperplayer;
			setbumper = true; // Don't set twice
		}
		else
		{
			// Don't accidentally set the same player, if the bestbumperplayer is also a huge camper.
			while (bestbumperplayer != -1 && camppos[nextcamppos] != -1
				&& bestbumperplayer == camppos[nextcamppos])
				nextcamppos++;

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

void K_RunPaperItemSpawners(void)
{
	const boolean overtime = (battleovertime.enabled >= 10*TICRATE);
	tic_t interval = 8*TICRATE;

	if (leveltime <= starttime)
	{
		return;
	}

	if ((battleovertime.enabled > 0) && (battleovertime.radius < 256*mapobjectscale))
	{
		return;
	}

	if (overtime == true)
	{
		interval /= 2;
	}

	if (((leveltime - starttime - (interval / 2)) % interval) != 0)
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
		UINT8 i;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator
				|| players[i].exiting > 0
				|| players[i].eliminated)
			{
				continue;
			}

			if ((gametyperules & GTR_BUMPERS) && players[i].bumpers <= 0)
			{
				continue;
			}

			pcount++;
		}

		if (pcount > 0)
		{
#define MAXITEM 64
			UINT16 item = 0;
			mobj_t *spotList[MAXITEM];
			boolean spotUsed[MAXITEM];

			thinker_t *th;
			mobj_t *mo;

			memset(spotUsed, false, sizeof(spotUsed));

			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (item >= MAXITEM)
					break;

				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				mo = (mobj_t *)th;

				if (mo->type == MT_PAPERITEMSPOT)
				{
					spotList[item] = mo;
					item++;
				}
			}

			if (item <= 0)
			{
				return;
			}

			for (i = 0; i < min(item, pcount); i++)
			{
				UINT8 r = P_RandomRange(0, item-1);
				UINT8 recursion = 0;
				mobj_t *drop = NULL;
				SINT8 flip = 1;

				while (spotUsed[r] == true)
				{
					r = P_RandomRange(0, item-1);

					if ((recursion++) > 32)
					{
						break;
					}
				}

				flip = P_MobjFlip(spotList[r]);

				drop = K_CreatePaperItem(
					spotList[r]->x, spotList[r]->y, spotList[r]->z + (128 * mapobjectscale * flip),
					FixedAngle(P_RandomRange(0, 359) * FRACUNIT), flip,
					0, 0
				);

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
