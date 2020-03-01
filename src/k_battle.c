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

// Capsule counters
UINT8 maptargets = 0; // Capsules in map
UINT8 numtargets = 0; // Capsules busted

boolean K_IsPlayerWanted(player_t *player)
{
	UINT8 i;
	if (!(G_BattleGametype()))
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

	if (!G_BattleGametype())
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

		if (players[i].kartstuff[k_bumper] <= 0) // Not alive, so don't do anything else
			continue;

		numingame++;

		if (bestbumper == -1 || players[i].kartstuff[k_bumper] > bestbumper)
		{
			bestbumper = players[i].kartstuff[k_bumper];
			bestbumperplayer = i;
		}
		else if (players[i].kartstuff[k_bumper] == bestbumper)
			bestbumperplayer = -1; // Tie, no one has best bumper.

		for (j = 0; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j] || players[j].spectator)
				continue;
			if (players[j].kartstuff[k_bumper] <= 0)
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

	if (!multiplayer)
		return;

	if (!G_BattleGametype())
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

		if (players[i].kartstuff[k_bumper] <= 0) // if you don't have any bumpers, you're probably not a winner
			continue;
		else if (winnernum > -1) // TWO winners? that's dumb :V
			return;

		winnernum = i;
		winnerscoreadd -= players[i].marescore;
	}

	if (numingame <= 1)
	{
		if (!battlecapsules)
			D_MapChange(gamemap, gametype, encoremode, true, 0, false, false);
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

#define MAXPLANESPERSECTOR (MAXFFLOORS+1)*2

static void K_SpawnOvertimeParticles(fixed_t x, fixed_t y, fixed_t scale, mobjtype_t type, boolean ceiling)
{
	UINT8 i;
	fixed_t flatz[MAXPLANESPERSECTOR];
	boolean flip[MAXPLANESPERSECTOR];
	UINT8 numflats = 0;
	mobj_t *mo;
	subsector_t *ss = R_IsPointInSubsector(x, y);
	sector_t *sec;

	if (!ss)
		return;
	sec = ss->sector;

	// convoluted stuff JUST to get all of the planes we need to draw orbs on :V

	for (i = 0; i < MAXPLANESPERSECTOR; i++)
		flip[i] = false;

	if (sec->floorpic != skyflatnum)
	{
#ifdef ESLOPE
		flatz[numflats] = (sec->f_slope ? P_GetZAt(sec->f_slope, x, y) : sec->floorheight);
#else
		flatz[numflats] = (sec->floorheight);
#endif
		numflats++;
	}
	if (sec->ceilingpic != skyflatnum && ceiling)
	{
#ifdef ESLOPE
		flatz[numflats] = (sec->c_slope ? P_GetZAt(sec->c_slope, x, y) : sec->ceilingheight) - FixedMul(mobjinfo[type].height, scale);
#else
		flatz[numflats] = (sec->ceilingheight) - FixedMul(mobjinfo[type].height, scale);
#endif
		flip[numflats] = true;
		numflats++;
	}

	if (sec->ffloors)
	{
		ffloor_t *rover;
		for (rover = sec->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_BLOCKPLAYER))
				continue;
			if (*rover->toppic != skyflatnum)
			{
#ifdef ESLOPE
				flatz[numflats] = (*rover->t_slope ? P_GetZAt(*rover->t_slope, x, y) : *rover->topheight);
#else
				flatz[numflats] = (*rover->topheight);
#endif
				numflats++;
			}
			if (*rover->bottompic != skyflatnum && ceiling)
			{
#ifdef ESLOPE
				flatz[numflats] = (*rover->b_slope ? P_GetZAt(*rover->b_slope, x, y) : *rover->bottomheight) - FixedMul(mobjinfo[type].height, scale);
#else
				flatz[numflats] = (*rover->bottomheight) - FixedMul(mobjinfo[type].height, scale);
#endif
				flip[numflats] = true;
				numflats++;
			}
		}
	}

	if (numflats <= 0) // no flats
		return;

	for (i = 0; i < numflats; i++)
	{
		mo = P_SpawnMobj(x, y, flatz[i], type);

		// Lastly, if this can see the skybox mobj, then... we just wasted our time :V
		if (skyboxmo[0] && !P_MobjWasRemoved(skyboxmo[0]))
		{
			const fixed_t sbz = skyboxmo[0]->z;
			fixed_t checkz = sec->floorheight;

			while (checkz < sec->ceilingheight)
			{
				P_TeleportMove(skyboxmo[0], skyboxmo[0]->x, skyboxmo[0]->y, checkz);
				if (P_CheckSight(skyboxmo[0], mo))
				{
					P_RemoveMobj(mo);
					break;
				}
				else
					checkz += 32*mapobjectscale;
			}

			P_TeleportMove(skyboxmo[0], skyboxmo[0]->x, skyboxmo[0]->y, sbz);

			if (P_MobjWasRemoved(mo))
				continue;
		}

		P_SetScale(mo, scale);

		if (flip[i])
		{
			mo->flags2 |= MF2_OBJECTFLIP;
			mo->eflags |= MFE_VERTICALFLIP;
		}

		switch(type)
		{
			case MT_OVERTIMEFOG:
				mo->destscale = 8*mo->scale;
				mo->momz = P_RandomRange(1,8)*mo->scale;
				break;
			case MT_OVERTIMEORB:
				//mo->destscale = mo->scale/4;
				mo->frame += ((leveltime/4) % 8);
				/*if (battleovertime.enabled < 10*TICRATE)
					mo->flags2 |= MF2_SHADOW;*/
				mo->angle = R_PointToAngle2(mo->x, mo->y, battleovertime.x, battleovertime.y) + ANGLE_90;
				mo->z += P_RandomRange(0,48) * mo->scale;
				break;
			default:
				break;
		}
	}
}

#undef MAXPLANESPERSECTOR

void K_RunBattleOvertime(void)
{
	UINT16 i, j;

	if (battleovertime.enabled < 10*TICRATE)
	{
		battleovertime.enabled++;
		if (battleovertime.enabled == TICRATE)
			S_StartSound(NULL, sfx_bhurry);
		if (battleovertime.enabled == 10*TICRATE)
			S_StartSound(NULL, sfx_kc40);
	}
	else
	{
		if (battleovertime.radius > battleovertime.minradius)
			battleovertime.radius -= mapobjectscale;
		else
			battleovertime.radius = battleovertime.minradius;
	}

	if (leveltime & 1)
	{
		UINT8 transparency = tr_trans50;

		if (!splitscreen && players[displayplayers[0]].mo)
		{
			INT32 dist = P_AproxDistance(battleovertime.x-players[displayplayers[0]].mo->x, battleovertime.y-players[displayplayers[0]].mo->y);
			transparency = max(0, NUMTRANSMAPS - ((256 + (dist>>FRACBITS)) / 256));
		}

		if (transparency < NUMTRANSMAPS)
		{
			mobj_t *beam = P_SpawnMobj(battleovertime.x, battleovertime.y, battleovertime.z + (mobjinfo[MT_RANDOMITEM].height/2), MT_OVERTIMEBEAM);
			P_SetScale(beam, beam->scale*2);
			if (transparency > 0)
				beam->frame |= transparency<<FF_TRANSSHIFT;
		}
	}

	// 16 orbs at the normal minimum size of 512
	{
		const fixed_t pi = (22<<FRACBITS) / 7; // loose approximation, this doesn't need to be incredibly precise
		fixed_t scale = mapobjectscale + (battleovertime.radius/2048);
		fixed_t sprwidth = 32*scale;
		fixed_t circumference = FixedMul(pi, battleovertime.radius<<1);
		UINT16 orbs = circumference / sprwidth;
		angle_t angoff = ANGLE_MAX / orbs;

		for (i = 0; i < orbs; i++)
		{
			angle_t ang = (i * angoff) + FixedAngle((leveltime/2)<<FRACBITS);
			fixed_t x = battleovertime.x + P_ReturnThrustX(NULL, ang, battleovertime.radius - FixedMul(mobjinfo[MT_OVERTIMEORB].radius, scale));
			fixed_t y = battleovertime.y + P_ReturnThrustY(NULL, ang, battleovertime.radius - FixedMul(mobjinfo[MT_OVERTIMEORB].radius, scale));
			K_SpawnOvertimeParticles(x, y, scale, MT_OVERTIMEORB, false);
		}
	}

	if (battleovertime.enabled < 10*TICRATE)
		return;

	/*if (!S_IdPlaying(sfx_s3kd4s)) // global ambience
		S_StartSoundAtVolume(NULL, sfx_s3kd4s, min(255, ((4096*mapobjectscale) - battleovertime.radius)>>FRACBITS / 2));*/

	for (i = 0; i < 16; i++)
	{
		j = 0;
		while (j < 32) // max attempts
		{
			fixed_t x = battleovertime.x + ((P_RandomRange(-64,64) * 128)<<FRACBITS);
			fixed_t y = battleovertime.y + ((P_RandomRange(-64,64) * 128)<<FRACBITS);
			fixed_t closestdist = battleovertime.radius + (8*mobjinfo[MT_OVERTIMEFOG].radius);
			j++;
			if (P_AproxDistance(x-battleovertime.x, y-battleovertime.y) < closestdist)
				continue;
			K_SpawnOvertimeParticles(x, y, 4*mapobjectscale, MT_OVERTIMEFOG, false);
			break;
		}
	}
}

static void K_SetupMovingCapsule(mapthing_t *mt, mobj_t *mobj)
{
	UINT8 sequence = mt->extrainfo-1;
	fixed_t speed = (FRACUNIT >> 3) * mt->angle;
	boolean backandforth = (mt->options & MTF_AMBUSH);
	boolean reverse = (mt->options & MTF_OBJECTSPECIAL);
	mobj_t *mo2;
	mobj_t *target = NULL;
	thinker_t *th;

	// Find the inital target
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
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
	UINT8 n = 0;
	size_t i;

	if (battlecapsules)
		return;

	if (!G_BattleGametype())
		return;

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

	mt = mapthings;
	for (i = 0; i < nummapthings; i++, mt++)
	{
		if (mt->type == mobjinfo[MT_BATTLECAPSULE].doomednum)
		{
			sector_t *mtsector, *sec;
			fixed_t x, y, z;
			fixed_t floorheights[MAXFFLOORS+1];
			UINT8 numfloors = 1;
			mobj_t *mobj = NULL;
			boolean fly = true;
			UINT8 j;

			mt->mobj = NULL;

			mtsector = R_PointInSubsector(mt->x << FRACBITS, mt->y << FRACBITS)->sector;
			mt->z = (INT16)(
#ifdef ESLOPE
				mtsector->f_slope ? P_GetZAt(mtsector->f_slope, mt->x << FRACBITS, mt->y << FRACBITS) :
#endif
				mtsector->floorheight)>>FRACBITS;

			x = mt->x << FRACBITS;
			y = mt->y << FRACBITS;

			sec = R_PointInSubsector(x, y)->sector;

			if (mt->options & MTF_OBJECTFLIP)
			{
				z = (
#ifdef ESLOPE
					sec->c_slope ? P_GetZAt(sec->c_slope, x, y) :
#endif
					sec->ceilingheight) - mobjinfo[MT_BATTLECAPSULE].height;

				floorheights[0] = z;

				if (mt->options >> ZSHIFT)
					z -= ((mt->options >> ZSHIFT) << FRACBITS);
			}
			else
			{
				z =
#ifdef ESLOPE
					sec->f_slope ? P_GetZAt(sec->f_slope, x, y) :
#endif
					sec->floorheight;

				floorheights[0] = z;

				if (mt->options >> ZSHIFT)
					z += ((mt->options >> ZSHIFT) << FRACBITS);
			}


			if (sec->ffloors)
			{
				ffloor_t *rover;
				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if ((rover->flags & FF_EXISTS) && (rover->flags & FF_BLOCKOTHERS))
					{
						if (mt->options & MTF_OBJECTFLIP)
						{
							floorheights[numfloors] = (
#ifdef ESLOPE
								*rover->b_slope ? P_GetZAt(*rover->b_slope, x, y) :
#endif
								*rover->bottomheight) - mobjinfo[MT_BATTLECAPSULE].height;
						}
						else
						{
							floorheights[numfloors] = (
#ifdef ESLOPE
								*rover->t_slope ? P_GetZAt(*rover->t_slope, x, y) :
#endif
								*rover->topheight);
						}

						numfloors++;
					}
				}
			}

			mt->z = (INT16)(z>>FRACBITS);

			mobj = P_SpawnMobj(x, y, z, MT_BATTLECAPSULE);
			mobj->spawnpoint = mt;

			if (mt->options & MTF_OBJECTFLIP)
			{
				mobj->eflags |= MFE_VERTICALFLIP;
				mobj->flags2 |= MF2_OBJECTFLIP;
			}

			for (j = 0; j < numfloors; j++)
			{
				if (z == floorheights[j])
				{
					fly = false;
					break;
				}
			}

			// Flying capsules
			if (fly)
			{
				mobj->flags |= MF_NOGRAVITY;
				mobj->extravalue1 = 1; // Set extravalue1 for later reference
			}

			// Moving capsules!
			if (mt->extrainfo && mt->angle)
				K_SetupMovingCapsule(mt, mobj);

			// Moved from P_SpawnMobj due to order of operations mumbo jumbo
			{
				mobj_t *cur, *prev = mobj;

				// Init hnext list
				// Spherical top
				cur = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_BATTLECAPSULE_PIECE);
				P_SetMobjState(cur, S_BATTLECAPSULE_TOP);

				P_SetTarget(&cur->target, mobj);
				P_SetTarget(&cur->hprev, prev);
				P_SetTarget(&prev->hnext, cur);
				prev = cur;

				// Tippity-top decorational button
				cur = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_BATTLECAPSULE_PIECE);
				P_SetMobjState(cur, S_BATTLECAPSULE_BUTTON);

				P_SetTarget(&cur->target, mobj);
				P_SetTarget(&cur->hprev, prev);
				P_SetTarget(&prev->hnext, cur);
				prev = cur;

				// Supports on the bottom
				for (j = 0; j < 4; j++)
				{
					cur = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_BATTLECAPSULE_PIECE);
					cur->extravalue1 = j;

					if (mobj->extravalue1) // Flying capsule, moving or not
						P_SetMobjState(cur, S_BATTLECAPSULE_SUPPORTFLY);
					else if (mobj->target && !P_MobjWasRemoved(mobj->target)) // Grounded, moving capsule
						P_SetMobjState(cur, S_KARMAWHEEL);
					else
						P_SetMobjState(cur, S_BATTLECAPSULE_SUPPORT); // Grounded, stationary capsule

					P_SetTarget(&cur->target, mobj);
					P_SetTarget(&cur->hprev, prev);
					P_SetTarget(&prev->hnext, cur);
					prev = cur;
				}

				// Side paneling
				for (j = 0; j < 8; j++)
				{
					cur = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_BATTLECAPSULE_PIECE);
					cur->extravalue1 = j;

					if (j & 1)
						P_SetMobjState(cur, S_BATTLECAPSULE_SIDE2);
					else
						P_SetMobjState(cur, S_BATTLECAPSULE_SIDE1);

					P_SetTarget(&cur->target, mobj);
					P_SetTarget(&cur->hprev, prev);
					P_SetTarget(&prev->hnext, cur);
					prev = cur;
				}
			}

			mt->mobj = mobj;
		}
	}

	battlecapsules = true;
}
