// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_inter.c
/// \brief Handling interactions (i.e., collisions)

#include "doomdef.h"
#include "i_system.h"
#include "am_map.h"
#include "g_game.h"
#include "m_random.h"
#include "p_local.h"
#include "s_sound.h"
#include "r_main.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "lua_hook.h"
#include "m_cond.h" // unlockables, emblems, etc
#include "p_setup.h"
#include "m_cheat.h" // objectplace
#include "m_misc.h"
#include "v_video.h" // video flags for CEchos
#include "f_finale.h"

// SRB2kart
#include "k_kart.h"
#include "k_battle.h"
#include "k_pwrlv.h"
#include "k_grandprix.h"
#include "k_boss.h"
#include "k_respawn.h"
#include "p_spec.h"
#include "k_objects.h"

// CTF player names
#define CTFTEAMCODE(pl) pl->ctfteam ? (pl->ctfteam == 1 ? "\x85" : "\x84") : ""
#define CTFTEAMENDCODE(pl) pl->ctfteam ? "\x80" : ""

void P_ForceFeed(const player_t *player, INT32 attack, INT32 fade, tic_t duration, INT32 period)
{
	BasicFF_t Basicfeed;
	if (!player)
		return;
	Basicfeed.Duration = (UINT32)(duration * (100L/TICRATE));
	Basicfeed.ForceX = Basicfeed.ForceY = 1;
	Basicfeed.Gain = 25000;
	Basicfeed.Magnitude = period*10;
	Basicfeed.player = player;
	/// \todo test FFB
	P_RampConstant(&Basicfeed, attack, fade);
}

void P_ForceConstant(const BasicFF_t *FFInfo)
{
	JoyFF_t ConstantQuake;
	if (!FFInfo || !FFInfo->player)
		return;
	ConstantQuake.ForceX    = FFInfo->ForceX;
	ConstantQuake.ForceY    = FFInfo->ForceY;
	ConstantQuake.Duration  = FFInfo->Duration;
	ConstantQuake.Gain      = FFInfo->Gain;
	ConstantQuake.Magnitude = FFInfo->Magnitude;
	if (FFInfo->player == &players[consoleplayer])
		I_Tactile(ConstantForce, &ConstantQuake);
	else if (splitscreen && FFInfo->player == &players[g_localplayers[1]])
		I_Tactile2(ConstantForce, &ConstantQuake);
	else if (splitscreen > 1 && FFInfo->player == &players[g_localplayers[2]])
		I_Tactile3(ConstantForce, &ConstantQuake);
	else if (splitscreen > 2 && FFInfo->player == &players[g_localplayers[3]])
		I_Tactile4(ConstantForce, &ConstantQuake);
}
void P_RampConstant(const BasicFF_t *FFInfo, INT32 Start, INT32 End)
{
	JoyFF_t RampQuake;
	if (!FFInfo || !FFInfo->player)
		return;
	RampQuake.ForceX    = FFInfo->ForceX;
	RampQuake.ForceY    = FFInfo->ForceY;
	RampQuake.Duration  = FFInfo->Duration;
	RampQuake.Gain      = FFInfo->Gain;
	RampQuake.Magnitude = FFInfo->Magnitude;
	RampQuake.Start     = Start;
	RampQuake.End       = End;
	if (FFInfo->player == &players[consoleplayer])
		I_Tactile(ConstantForce, &RampQuake);
	else if (splitscreen && FFInfo->player == &players[g_localplayers[1]])
		I_Tactile2(ConstantForce, &RampQuake);
	else if (splitscreen > 1 && FFInfo->player == &players[g_localplayers[2]])
		I_Tactile3(ConstantForce, &RampQuake);
	else if (splitscreen > 2 && FFInfo->player == &players[g_localplayers[3]])
		I_Tactile4(ConstantForce, &RampQuake);
}


//
// GET STUFF
//

//
// P_CanPickupItem
//
// Returns true if the player is in a state where they can pick up items.
//
boolean P_CanPickupItem(player_t *player, UINT8 weapon)
{
	if (player->exiting || mapreset || (player->pflags & PF_ELIMINATED))
		return false;

	if ((gametyperules & GTR_BUMPERS) // No bumpers in Match
#ifndef OTHERKARMAMODES
	&& !weapon
#endif
	&& player->bumpers <= 0)
		return false;

	if (weapon)
	{
		// Item slot already taken up
		if (weapon == 2)
		{
			// Invulnerable
			if (player->flashing > 0)
				return false;

			// Already have fake
			if (player->roulettetype == 2
				|| player->eggmanexplode)
				return false;
		}
		else
		{
			// Item-specific timer going off
			if (player->stealingtimer
				|| player->rocketsneakertimer
				|| player->eggmanexplode)
				return false;

			// Item slot already taken up
			if (player->itemroulette
				|| (weapon != 3 && player->itemamount)
				|| (player->pflags & PF_ITEMOUT))
				return false;

			if (weapon == 3 && K_GetShieldFromItem(player->itemtype) != KSHIELD_NONE)
				return false; // No stacking shields!
		}
	}

	return true;
}

/** Takes action based on a ::MF_SPECIAL thing touched by a player.
  * Actually, this just checks a few things (heights, toucher->player, no
  * objectplace, no dead or disappearing things)
  *
  * The special thing may be collected and disappear, or a sound may play, or
  * both.
  *
  * \param special     The special thing.
  * \param toucher     The player's mobj.
  * \param heightcheck Whether or not to make sure the player and the object
  *                    are actually touching.
  */
void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher, boolean heightcheck)
{
	player_t *player;
	INT32 i;

	if (objectplacing)
		return;

	I_Assert(special != NULL);
	I_Assert(toucher != NULL);

	// Dead thing touching.
	// Can happen with a sliding player corpse.
	if (toucher->health <= 0)
		return;
	if (special->health <= 0)
		return;

	if (heightcheck)
	{
		fixed_t toucher_bottom = toucher->z;
		fixed_t special_bottom = special->z;

		if (toucher->flags & MF_PICKUPFROMBELOW)
			toucher_bottom -= toucher->height;

		if (special->flags & MF_PICKUPFROMBELOW)
			special_bottom -= special->height;

		if (toucher->momz < 0) {
			if (toucher_bottom + toucher->momz > special->z + special->height)
				return;
		} else if (toucher_bottom > special->z + special->height)
			return;
		if (toucher->momz > 0) {
			if (toucher->z + toucher->height + toucher->momz < special_bottom)
				return;
		} else if (toucher->z + toucher->height < special_bottom)
			return;
	}

	player = toucher->player;
	I_Assert(player != NULL); // Only players can touch stuff!

	if (player->spectator)
		return;

	// Ignore multihits in "ouchie" mode
	if (special->flags & (MF_ENEMY|MF_BOSS) && special->flags2 & MF2_FRET)
		return;

	if (LUAh_TouchSpecial(special, toucher) || P_MobjWasRemoved(special))
		return;

	if ((special->flags & (MF_ENEMY|MF_BOSS)) && !(special->flags & MF_MISSILE))
	{
		////////////////////////////////////////////////////////
		/////ENEMIES & BOSSES!!/////////////////////////////////
		////////////////////////////////////////////////////////

		P_DamageMobj(toucher, special, special, 1, DMG_NORMAL);
		return;
	}
	else
	{
	// We now identify by object type, not sprite! Tails 04-11-2001
	switch (special->type)
	{
		case MT_MEMENTOSTP:	 // Mementos teleport
			// Teleport player to the other teleporter (special->target). We'll assume there's always only ever 2.
			if (!special->target)
				return;	// foolproof crash prevention check!!!!!

			P_SetOrigin(player->mo, special->target->x, special->target->y, special->target->z + (48<<FRACBITS));
			player->mo->angle = special->target->angle;
			P_SetObjectMomZ(player->mo, 12<<FRACBITS, false);
			P_InstaThrust(player->mo, player->mo->angle, 20<<FRACBITS);
			return;
		case MT_FLOATINGITEM: // SRB2Kart
			if (!P_CanPickupItem(player, 3) || (player->itemamount && player->itemtype != special->threshold))
				return;

			if ((gametyperules & GTR_BUMPERS) && player->bumpers <= 0)
				return;

			player->itemtype = special->threshold;
			if ((UINT16)(player->itemamount) + special->movecount > 255)
				player->itemamount = 255;
			else
				player->itemamount += special->movecount;

			S_StartSound(special, special->info->deathsound);

			P_SetTarget(&special->tracer, toucher);
			special->flags2 |= MF2_NIGHTSPULL;
			special->destscale = mapobjectscale>>4;
			special->scalespeed <<= 1;

			special->flags &= ~MF_SPECIAL;
			return;
		case MT_RANDOMITEM:
			if (!P_CanPickupItem(player, 1))
				return;

			special->momx = special->momy = special->momz = 0;
			P_SetTarget(&special->target, toucher);
			P_KillMobj(special, toucher, toucher, DMG_NORMAL);
			break;
		case MT_SPHEREBOX:
			if (!P_CanPickupItem(player, 0))
				return;

			special->momx = special->momy = special->momz = 0;
			P_SetTarget(&special->target, toucher);
			P_KillMobj(special, toucher, toucher, DMG_NORMAL);
			break;
		case MT_ITEMCAPSULE:
			if ((gametyperules & GTR_BUMPERS) && player->bumpers <= 0)
				return;

			if (special->scale < special->extravalue1) // don't break it while it's respawning
				return;

			switch (special->threshold)
			{
				case KITEM_SPB:
					if (K_IsSPBInGame()) // don't spawn a second SPB
						return;
					break;
				case KITEM_SUPERRING:
					if (player->pflags & PF_RINGLOCK) // no cheaty rings
						return;
					break;
				default:
					if (!P_CanPickupItem(player, 1))
						return;
					break;
			}

			S_StartSound(toucher, special->info->deathsound);
			P_KillMobj(special, toucher, toucher, DMG_NORMAL);
			return;
		case MT_KARMAHITBOX:
			if (!special->target->player)
				return;
			if (player == special->target->player)
				return;
			if (player->bumpers <= 0)
				return;
			if (special->target->player->exiting || player->exiting)
				return;

			if (P_PlayerInPain(special->target->player))
				return;

			if (special->target->player->karmadelay > 0)
				return;

			{
				mobj_t *boom;

				if (P_DamageMobj(toucher, special, special->target, 1, DMG_KARMA) == false)
				{
					return;
				}

				boom = P_SpawnMobj(special->target->x, special->target->y, special->target->z, MT_BOOMEXPLODE);

				boom->scale = special->target->scale;
				boom->destscale = special->target->scale;
				boom->momz = 5*FRACUNIT;

				if (special->target->color)
					boom->color = special->target->color;
				else
					boom->color = SKINCOLOR_KETCHUP;

				S_StartSound(boom, special->info->attacksound);

				special->target->player->karthud[khud_yougotem] = 2*TICRATE;
				special->target->player->karmadelay = comebacktime;
			}
			return;
		case MT_SPB:
			if ((special->target == toucher || special->target == toucher->target) && (special->threshold > 0))
				return;

			if (special->health <= 0 || toucher->health <= 0)
				return;

			if (player->spectator)
				return;

			if (special->tracer && !P_MobjWasRemoved(special->tracer) && toucher == special->tracer)
			{
				mobj_t *spbexplode;

				if (player->bubbleblowup > 0)
				{
					K_DropHnextList(player, false);
					special->extravalue1 = 2; // WAIT...
					special->extravalue2 = 52; // Slightly over the respawn timer length
					return;
				}

				S_StopSound(special); // Don't continue playing the gurgle or the siren

				spbexplode = P_SpawnMobj(toucher->x, toucher->y, toucher->z, MT_SPBEXPLOSION);
				spbexplode->extravalue1 = 1; // Tell K_ExplodePlayer to use extra knockback
				if (special->target && !P_MobjWasRemoved(special->target))
					P_SetTarget(&spbexplode->target, special->target);

				P_RemoveMobj(special);
			}
			else
			{
				P_DamageMobj(player->mo, special, special->target, 1, DMG_NORMAL);
			}
			return;
		case MT_EMERALD:
			if (!P_CanPickupItem(player, 0))
				return;

			if (special->threshold > 0)
				return;

			if (toucher->hitlag > 0)
				return;

			player->emeralds |= special->extravalue1;
			K_CheckEmeralds(player);
			break;
		/*
		case MT_EERIEFOG:
			special->frame &= ~FF_TRANS80;
			special->frame |= FF_TRANS90;
			return;
		*/
		case MT_SMK_MOLE:
			if (special->target && !P_MobjWasRemoved(special->target))
				return;

			if (special->health <= 0 || toucher->health <= 0)
				return;

			if (!player->mo || player->spectator)
				return;

			// kill
			if (player->invincibilitytimer > 0
				|| player->growshrinktimer > 0
				|| player->flamedash > 0)
			{
				P_KillMobj(special, toucher, toucher, DMG_NORMAL);
				return;
			}

			// no interaction
			if (player->flashing > 0 || player->hyudorotimer > 0 || P_PlayerInPain(player))
				return;

			// attach to player!
			P_SetTarget(&special->target, toucher);
			S_StartSound(special, sfx_s1a2);
			return;
		case MT_CDUFO: // SRB2kart
			if (special->fuse || !P_CanPickupItem(player, 1) || ((gametyperules & GTR_BUMPERS) && player->bumpers <= 0))
				return;

			player->itemroulette = 1;
			player->roulettetype = 1;

			// Karma fireworks
			for (i = 0; i < 5; i++)
			{
				mobj_t *firework = P_SpawnMobj(special->x, special->y, special->z, MT_KARMAFIREWORK);
				firework->momx = toucher->momx;
				firework->momy = toucher->momy;
				firework->momz = toucher->momz;
				P_Thrust(firework, FixedAngle((72*i)<<FRACBITS), P_RandomRange(1,8)*special->scale);
				P_SetObjectMomZ(firework, P_RandomRange(1,8)*special->scale, false);
				firework->color = toucher->color;
			}

			S_StartSound(toucher, sfx_cdfm73); // they don't make this sound in the original game but it's nice to have a "reward" for good play

			//special->momx = special->momy = special->momz = 0;
			special->momz = -(3*special->scale)/2;
			//P_SetTarget(&special->target, toucher);
			special->fuse = 2*TICRATE;
			break;

		case MT_BALLOON: // SRB2kart
			P_SetObjectMomZ(toucher, 20<<FRACBITS, false);
			break;

		case MT_BUBBLESHIELDTRAP:
			if ((special->target == toucher || special->target == toucher->target) && (special->threshold > 0))
				return;

			if (special->tracer && !P_MobjWasRemoved(special->tracer))
				return;

			if (special->health <= 0 || toucher->health <= 0)
				return;

			if (!player->mo || player->spectator)
				return;

			// attach to player!
			P_SetTarget(&special->tracer, toucher);
			toucher->flags |= MF_NOGRAVITY;
			toucher->momz = (8*toucher->scale) * P_MobjFlip(toucher);
			S_StartSound(toucher, sfx_s1b2);
			return;

		case MT_HYUDORO:
			Obj_HyudoroCollide(special, toucher);
			return;

		case MT_RING:
		case MT_FLINGRING:
			if (special->extravalue1)
				return;

			// No picking up rings while SPB is targetting you
			if (player->pflags & PF_RINGLOCK)
				return;

			// Don't immediately pick up spilled rings
			if (special->threshold > 0 || P_PlayerInPain(player))
				return;

			if (!(P_CanPickupItem(player, 0)))
				return;

			// Reached the cap, don't waste 'em!
			if (RINGTOTAL(player) >= 20)
				return;

			special->momx = special->momy = special->momz = 0;

			special->extravalue1 = 1; // Ring collect animation timer
			special->angle = R_PointToAngle2(toucher->x, toucher->y, special->x, special->y); // animation angle
			P_SetTarget(&special->target, toucher); // toucher for thinker
			player->pickuprings++;

			return;

		case MT_BLUESPHERE:
			if (!(P_CanPickupItem(player, 0)))
				return;

			P_GivePlayerSpheres(player, 1);
			break;

		// Secret emblem thingy
		case MT_EMBLEM:
			{
				if (demo.playback || special->health > MAXEMBLEMS)
					return;

				emblemlocations[special->health-1].collected = true;
				M_UpdateUnlockablesAndExtraEmblems();
				G_SaveGameData();
				break;
			}

		// CTF Flags
		case MT_REDFLAG:
		case MT_BLUEFLAG:
			return;

		case MT_STARPOST:
			P_TouchStarPost(special, player, special->spawnpoint && (special->spawnpoint->options & MTF_OBJECTSPECIAL));
			return;

		case MT_BIGTUMBLEWEED:
		case MT_LITTLETUMBLEWEED:
			if (toucher->momx || toucher->momy)
			{
				special->momx = toucher->momx;
				special->momy = toucher->momy;
				special->momz = P_AproxDistance(toucher->momx, toucher->momy)/4;

				if (toucher->momz > 0)
					special->momz += toucher->momz/8;

				P_SetMobjState(special, special->info->seestate);
			}
			return;

		case MT_WATERDROP:
			if (special->state == &states[special->info->spawnstate])
			{
				special->z = toucher->z+toucher->height-FixedMul(8*FRACUNIT, special->scale);
				special->momz = 0;
				special->flags |= MF_NOGRAVITY;
				P_SetMobjState (special, special->info->deathstate);
				S_StartSound (special, special->info->deathsound+(P_RandomKey(special->info->mass)));
			}
			return;

		default: // SOC or script pickup
			P_SetTarget(&special->target, toucher);
			break;
		}
	}

	S_StartSound(toucher, special->info->deathsound); // was NULL, but changed to player so you could hear others pick up rings
	P_KillMobj(special, NULL, toucher, DMG_NORMAL);
	special->shadowscale = 0;
}

/** Saves a player's level progress at a star post
  *
  * \param post The star post to trigger
  * \param player The player that should receive the checkpoint
  * \param snaptopost If true, the respawn point will use the star post's position, otherwise player x/y and star post z
  */
void P_TouchStarPost(mobj_t *post, player_t *player, boolean snaptopost)
{
	mobj_t *toucher = player->mo;

	(void)snaptopost;

	// Player must have touched all previous starposts
	if (post->health - player->starpostnum > 1)
	{
		if (!player->checkskip)
			S_StartSound(toucher, sfx_lose);
		player->checkskip = 3;
		return;
	}

	// With the parameter + angle setup, we can go up to 1365 star posts. Who needs that many?
	if (post->health > 1365)
	{
		CONS_Debug(DBG_GAMELOGIC, "Bad Starpost Number!\n");
		return;
	}

	if (player->starpostnum >= post->health)
		return; // Already hit this post

	player->starpostnum = post->health;
}

// Easily make it so that overtime works offline
#define TESTOVERTIMEINFREEPLAY

/** Checks if the level timer is over the timelimit and the round should end,
  * unless you are in overtime. In which case leveltime may stretch out beyond
  * timelimitintics and overtime's status will be checked here each tick.
  * Verify that the value of ::cv_timelimit is greater than zero before
  * calling this function.
  *
  * \sa cv_timelimit, P_CheckPointLimit, P_UpdateSpecials
  */
void P_CheckTimeLimit(void)
{
	INT32 i;

	if (!cv_timelimit.value)
		return;

#ifndef TESTOVERTIMEINFREEPLAY
	if (battlecapsules) // capsules override any time limit settings
		return;
#endif

	if (!(gametyperules & GTR_TIMELIMIT))
		return;

	if (bossinfo.boss == true)
		return;

	if (leveltime < (timelimitintics + starttime))
		return;

	if (gameaction == ga_completed)
		return;

	if ((cv_overtime.value) && (gametyperules & GTR_OVERTIME))
	{
#ifndef TESTOVERTIMEINFREEPLAY
		boolean foundone = false; // Overtime is used for closing off down to a specific item.
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (foundone)
			{
#endif
				// Initiate the kill zone
				if (!battleovertime.enabled)
				{
					thinker_t *th;
					mobj_t *center = NULL;

					for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
					{
						mobj_t *thismo;

						if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
							continue;

						thismo = (mobj_t *)th;

						if (thismo->type == MT_OVERTIME_CENTER)
						{
							center = thismo;
							break;
						}
					}

					if (center == NULL || P_MobjWasRemoved(center))
					{
						CONS_Alert(CONS_WARNING, "No center point for overtime!\n");

						battleovertime.x = 0;
						battleovertime.y = 0;
						battleovertime.z = 0;
					}
					else
					{
						battleovertime.x = center->x;
						battleovertime.y = center->y;
						battleovertime.z = center->z;
					}

					battleovertime.radius = 4096 * mapobjectscale;
					battleovertime.enabled = 1;

					S_StartSound(NULL, sfx_kc47);
				}

				return;
#ifndef TESTOVERTIMEINFREEPLAY
			}
			else
				foundone = true;
		}
#endif
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;
		if (players[i].exiting)
			return;
		P_DoPlayerExit(&players[i]);
	}
}

/** Checks if a player's score is over the pointlimit and the round should end.
  * Verify that the value of ::cv_pointlimit is greater than zero before
  * calling this function.
  *
  * \sa cv_pointlimit, P_CheckTimeLimit, P_UpdateSpecials
  */
void P_CheckPointLimit(void)
{
	INT32 i;

	if (!cv_pointlimit.value)
		return;

	if (!(multiplayer || netgame))
		return;

	if (!(gametyperules & GTR_POINTLIMIT))
		return;

	if (bossinfo.boss == true)
		return;

	// pointlimit is nonzero, check if it's been reached by this player
	if (G_GametypeHasTeams())
	{
		// Just check both teams
		if ((UINT32)cv_pointlimit.value <= redscore || (UINT32)cv_pointlimit.value <= bluescore)
		{
			if (server)
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
		}
	}
	else
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if ((UINT32)cv_pointlimit.value <= players[i].roundscore)
			{
				for (i = 0; i < MAXPLAYERS; i++) // AAAAA nested loop using the same iteration variable ;;
				{
					if (!playeringame[i] || players[i].spectator)
						continue;
					if (players[i].exiting)
						return;
					P_DoPlayerExit(&players[i]);
				}

				/*if (server)
					SendNetXCmd(XD_EXITLEVEL, NULL, 0);*/
				return; // good thing we're leaving the function immediately instead of letting the loop get mangled!
			}
		}
	}
}

// Checks whether or not to end a race netgame.
boolean P_CheckRacers(void)
{
	UINT8 i;
	UINT8 numplayersingame = 0;
	UINT8 numexiting = 0;
	boolean eliminatelast = cv_karteliminatelast.value;
	boolean everyonedone = true;
	boolean eliminatebots = false;
	const boolean griefed = (spectateGriefed > 0);

	// Check if all the players in the race have finished. If so, end the level.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator || players[i].lives <= 0) // Not playing
		{
			// Y'all aren't even playing
			continue;
		}

		numplayersingame++;

		if (players[i].exiting || (players[i].pflags & PF_NOCONTEST))
		{
			numexiting++;
		}
		else
		{
			if (players[i].bot)
			{
				// Isn't a human, thus doesn't matter. (Sorry, robots.)
				// Set this so that we can check for bots that need to get eliminated, though!
				eliminatebots = true;
				continue;
			}

			everyonedone = false;
		}
	}

	// If we returned here with bots left, then the last place bot may have a chance to finish the map and NOT get time over.
	// Not that it affects anything, they didn't make the map take longer or even get any points from it. But... it's a bit inconsistent!
	// So if there's any bots, we'll let the game skip this, continue onto calculating eliminatelast, THEN we return true anyway.
	if (everyonedone && !eliminatebots)
	{
		// Everyone's finished, we're done here!
		racecountdown = exitcountdown = 0;
		return true;
	}

	if (numplayersingame <= 1)
	{
		// Never do this without enough players.
		eliminatelast = false;
	}
	else
	{
		if (grandprixinfo.gp == true)
		{
			// Always do this in GP
			eliminatelast = true;
		}
		else if (griefed)
		{
			// Don't do this if someone spectated
			eliminatelast = false;
		}
	}

	if (eliminatelast == true && (numexiting >= numplayersingame-1))
	{
		// Everyone's done playing but one guy apparently.
		// Just kill everyone who is still playing.

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator || players[i].lives <= 0)
			{
				// Y'all aren't even playing
				continue;
			}

			if (players[i].exiting || (players[i].pflags & PF_NOCONTEST))
			{
				// You're done, you're free to go.
				continue;
			}

			P_DoTimeOver(&players[i]);
		}

		// Everyone should be done playing at this point now.
		racecountdown = exitcountdown = 0;
		return true;
	}

	if (everyonedone)
	{
		// See above: there might be bots that are still going, but all players are done, so we can exit now.
		racecountdown = exitcountdown = 0;
		return true;
	}

	// SO, we're not done playing.
	// Let's see if it's time to start the death counter!

	if (!racecountdown)
	{
		// If the winners are all done, then start the death timer.
		UINT8 winningpos = 1;

		winningpos = max(1, numplayersingame/2);
		if (numplayersingame % 2) // any remainder?
		{
			winningpos++;
		}

		if (numexiting >= winningpos)
		{
			tic_t countdown = 30*TICRATE; // 30 seconds left to finish, get going!

			if (netgame)
			{
				// Custom timer
				countdown = cv_countdowntime.value * TICRATE;
			}

			racecountdown = countdown + 1;
		}
	}

	// We're still playing, but no one else is, so we need to reset spectator griefing.
	if (numplayersingame <= 1)
	{
		spectateGriefed = 0;
	}

	// Turns out we're still having a good time & playing the game, we didn't have to do anything :)
	return false;
}

/** Kills an object.
  *
  * \param target    The victim.
  * \param inflictor The attack weapon. May be NULL (environmental damage).
  * \param source    The attacker. May be NULL.
  * \param damagetype The type of damage dealt that killed the target. If bit 7 (0x80) was set, this was an instant-death.
  * \todo Cleanup, refactor, split up.
  * \sa P_DamageMobj
  */
void P_KillMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, UINT8 damagetype)
{
	mobj_t *mo;

	if (target->flags & (MF_ENEMY|MF_BOSS))
		target->momx = target->momy = target->momz = 0;

	// SRB2kart
	if (target->type != MT_PLAYER && !(target->flags & MF_MONITOR)
		 && !(target->type == MT_ORBINAUT || target->type == MT_ORBINAUT_SHIELD
		 || target->type == MT_JAWZ || target->type == MT_JAWZ_DUD || target->type == MT_JAWZ_SHIELD
		 || target->type == MT_BANANA || target->type == MT_BANANA_SHIELD
		 || target->type == MT_DROPTARGET || target->type == MT_DROPTARGET_SHIELD
		 || target->type == MT_EGGMANITEM || target->type == MT_EGGMANITEM_SHIELD
		 || target->type == MT_BALLHOG || target->type == MT_SPB)) // kart dead items
		target->flags |= MF_NOGRAVITY; // Don't drop Tails 03-08-2000
	else
		target->flags &= ~MF_NOGRAVITY; // lose it if you for whatever reason have it, I'm looking at you shields
	//

	if (target->flags2 & MF2_NIGHTSPULL)
	{
		P_SetTarget(&target->tracer, NULL);
		target->movefactor = 0; // reset NightsItemChase timer
	}

	// dead target is no more shootable
	target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SPECIAL);
	target->flags2 &= ~(MF2_SKULLFLY|MF2_NIGHTSPULL);
	target->health = 0; // This makes it easy to check if something's dead elsewhere.

	if (target->type != MT_BATTLEBUMPER)
	{
		target->shadowscale = 0;
	}

	if (LUAh_MobjDeath(target, inflictor, source, damagetype) || P_MobjWasRemoved(target))
		return;

	//K_SetHitLagForObjects(target, inflictor, MAXHITLAGTICS, true);

	// SRB2kart
	// I wish I knew a better way to do this
	if (target->target && target->target->player && target->target->player->mo)
	{
		if ((target->target->player->pflags & PF_EGGMANOUT) && target->type == MT_EGGMANITEM_SHIELD)
			target->target->player->pflags &= ~PF_EGGMANOUT;

		if (target->target->player->pflags & PF_ITEMOUT)
		{
			if ((target->type == MT_BANANA_SHIELD && target->target->player->itemtype == KITEM_BANANA) // trail items
				|| (target->type == MT_SSMINE_SHIELD && target->target->player->itemtype == KITEM_MINE)
				|| (target->type == MT_DROPTARGET_SHIELD && target->target->player->itemtype == KITEM_DROPTARGET)
				|| (target->type == MT_SINK_SHIELD && target->target->player->itemtype == KITEM_KITCHENSINK))
			{
				if (target->movedir != 0 && target->movedir < (UINT16)target->target->player->itemamount)
				{
					if (target->target->hnext)
						K_KillBananaChain(target->target->hnext, inflictor, source);
					target->target->player->itemamount = 0;
				}
				else if (target->target->player->itemamount)
					target->target->player->itemamount--;
			}
			else if ((target->type == MT_ORBINAUT_SHIELD && target->target->player->itemtype == KITEM_ORBINAUT) // orbit items
				|| (target->type == MT_JAWZ_SHIELD && target->target->player->itemtype == KITEM_JAWZ))
			{
				if (target->target->player->itemamount)
					target->target->player->itemamount--;
				if (target->lastlook != 0)
				{
					K_RepairOrbitChain(target);
				}
			}

			if (!target->target->player->itemamount)
				target->target->player->pflags &= ~PF_ITEMOUT;

			if (target->target->hnext == target)
				P_SetTarget(&target->target->hnext, NULL);
		}
	}
	// Above block does not clean up rocket sneakers when a player dies, so we need to do it here target->target is null when using rocket sneakers
	if (target->player)
		K_DropRocketSneaker(target->player);

	// Let EVERYONE know what happened to a player! 01-29-2002 Tails
	if (target->player && !target->player->spectator)
	{
		if (metalrecording) // Ack! Metal Sonic shouldn't die! Cut the tape, end recording!
			G_StopMetalRecording(true);

		target->renderflags &= ~RF_DONTDRAW;
	}

	// if killed by a player
	if (source && source->player)
	{
		if (target->flags & MF_MONITOR || target->type == MT_RANDOMITEM)
		{
			UINT8 i;

			P_SetTarget(&target->target, source);

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (&players[i] == source->player)
				{
					continue;
				}

				if (playeringame[i] && !players[i].spectator && players[i].lives != 0)
				{
					break;
				}
			}

			if (i < MAXPLAYERS)
			{
				// Respawn items in multiplayer, don't respawn them when alone
				target->fuse = 2*TICRATE + 2;
			}
		}
	}

	// if a player avatar dies...
	if (target->player)
	{
		UINT8 i;

		target->flags &= ~(MF_SOLID|MF_SHOOTABLE); // does not block
		P_UnsetThingPosition(target);
		target->flags |= MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY;
		P_SetThingPosition(target);
		target->standingslope = NULL;
		target->terrain = NULL;
		target->pmomz = 0;

		target->player->playerstate = PST_DEAD;

		// respawn from where you died
		target->player->respawn.pointx = target->x;
		target->player->respawn.pointy = target->y;
		target->player->respawn.pointz = target->z;

		if (target->player == &players[consoleplayer])
		{
			// don't die in auto map,
			// switch view prior to dying
			if (automapactive)
				AM_Stop();
		}

		//added : 22-02-98: recenter view for next life...
		for (i = 0; i <= r_splitscreen; i++)
		{
			if (target->player == &players[displayplayers[i]])
			{
				localaiming[i] = 0;
				break;
			}
		}

		if (gametyperules & GTR_BUMPERS)
			K_CheckBumpers();

		target->player->trickpanel = 0;
	}

	if (source && target && target->player && source->player)
		P_PlayVictorySound(source); // Killer laughs at you. LAUGHS! BWAHAHAHA!

	// Other death animation effects
	switch(target->type)
	{
		case MT_BOUNCEPICKUP:
		case MT_RAILPICKUP:
		case MT_AUTOPICKUP:
		case MT_EXPLODEPICKUP:
		case MT_SCATTERPICKUP:
		case MT_GRENADEPICKUP:
			P_SetObjectMomZ(target, FRACUNIT, false);
			target->fuse = target->info->damage;
			break;

		case MT_BUGGLE:
			if (inflictor && inflictor->player // did a player kill you? Spawn relative to the player so they're bound to get it
			&& P_AproxDistance(inflictor->x - target->x, inflictor->y - target->y) <= inflictor->radius + target->radius + FixedMul(8*FRACUNIT, inflictor->scale) // close enough?
			&& inflictor->z <= target->z + target->height + FixedMul(8*FRACUNIT, inflictor->scale)
			&& inflictor->z + inflictor->height >= target->z - FixedMul(8*FRACUNIT, inflictor->scale))
				mo = P_SpawnMobj(inflictor->x + inflictor->momx, inflictor->y + inflictor->momy, inflictor->z + (inflictor->height / 2) + inflictor->momz, MT_EXTRALARGEBUBBLE);
			else
				mo = P_SpawnMobj(target->x, target->y, target->z, MT_EXTRALARGEBUBBLE);
			mo->destscale = target->scale;
			P_SetScale(mo, mo->destscale);
			P_SetMobjState(mo, mo->info->raisestate);
			break;

		case MT_YELLOWSHELL:
			P_SpawnMobjFromMobj(target, 0, 0, 0, MT_YELLOWSPRING);
			break;

		case MT_CRAWLACOMMANDER:
			target->momx = target->momy = target->momz = 0;
			break;

		case MT_CRUSHSTACEAN:
			if (target->tracer)
			{
				mobj_t *chain = target->tracer->target, *chainnext;
				while (chain)
				{
					chainnext = chain->target;
					P_RemoveMobj(chain);
					chain = chainnext;
				}
				S_StopSound(target->tracer);
				P_KillMobj(target->tracer, inflictor, source, damagetype);
			}
			break;

		case MT_BANPYURA:
			if (target->tracer)
			{
				S_StopSound(target->tracer);
				P_KillMobj(target->tracer, inflictor, source, damagetype);
			}
			break;

		case MT_EGGSHIELD:
			P_SetObjectMomZ(target, 4*target->scale, false);
			P_InstaThrust(target, target->angle, 3*target->scale);
			target->flags = (target->flags|MF_NOCLIPHEIGHT) & ~MF_NOGRAVITY;
			break;

		case MT_DRAGONBOMBER:
			{
				mobj_t *segment = target;
				while (segment->tracer != NULL)
				{
					P_KillMobj(segment->tracer, NULL, NULL, DMG_NORMAL);
					segment = segment->tracer;
				}
				break;
			}

		case MT_EGGMOBILE3:
			{
				mobj_t *mo2;
				thinker_t *th;
				UINT32 i = 0; // to check how many clones we've removed

				// scan the thinkers to make sure all the old pinch dummies are gone on death
				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
						continue;

					mo = (mobj_t *)th;
					if (mo->type != (mobjtype_t)target->info->mass)
						continue;
					if (mo->tracer != target)
						continue;

					P_KillMobj(mo, inflictor, source, damagetype);
					mo->destscale = mo->scale/8;
					mo->scalespeed = (mo->scale - mo->destscale)/(2*TICRATE);
					mo->momz = mo->info->speed;
					mo->angle = FixedAngle((P_RandomKey(36)*10)<<FRACBITS);

					mo2 = P_SpawnMobjFromMobj(mo, 0, 0, 0, MT_BOSSJUNK);
					P_InitAngle(mo2, mo->angle);
					P_SetMobjState(mo2, S_BOSSSEBH2);

					if (++i == 2) // we've already removed 2 of these, let's stop now
						break;
					else
						S_StartSound(mo, mo->info->deathsound); // done once to prevent sound stacking
				}
			}
			break;

		case MT_BIGMINE:
			if (inflictor)
			{
				fixed_t dx = target->x - inflictor->x, dy = target->y - inflictor->y, dz = target->z - inflictor->z;
				fixed_t dm = FixedHypot(dz, FixedHypot(dy, dx));
				target->momx = FixedDiv(FixedDiv(dx, dm), dm)*512;
				target->momy = FixedDiv(FixedDiv(dy, dm), dm)*512;
				target->momz = FixedDiv(FixedDiv(dz, dm), dm)*512;
			}
			if (source)
				P_SetTarget(&target->tracer, source);
			break;

		case MT_BLASTEXECUTOR:
			if (target->spawnpoint)
				P_LinedefExecute(target->spawnpoint->angle, (source ? source : inflictor), target->subsector->sector);
			break;

		case MT_SPINBOBERT:
			if (target->hnext)
				P_KillMobj(target->hnext, inflictor, source, damagetype);
			if (target->hprev)
				P_KillMobj(target->hprev, inflictor, source, damagetype);
			break;

		case MT_EGGTRAP:
			// Time for birdies! Yaaaaaaaay!
			target->fuse = TICRATE;
			break;

		case MT_MINECART:
			A_Scream(target);
			target->momx = target->momy = target->momz = 0;
			if (target->target && target->target->health)
				P_KillMobj(target->target, target, source, DMG_NORMAL);
			break;

		case MT_PLAYER:
			if (damagetype != DMG_SPECTATOR)
			{
				angle_t flingAngle;
				mobj_t *kart;

				target->fuse = TICRATE*3; // timer before mobj disappears from view (even if not an actual player)
				target->momx = target->momy = target->momz = 0;

				kart = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_KART_LEFTOVER);

				if (kart && !P_MobjWasRemoved(kart))
				{
					kart->angle = target->angle;
					kart->color = target->color;
					kart->hitlag = target->hitlag;
					kart->eflags |= MFE_DAMAGEHITLAG;
					P_SetObjectMomZ(kart, 6*FRACUNIT, false);
					kart->extravalue1 = target->player->kartweight;

					// Copy interp data
					kart->old_angle = target->old_angle;
					kart->old_x = target->old_x;
					kart->old_y = target->old_y;
					kart->old_z = target->old_z;
				}

				if (source && !P_MobjWasRemoved(source))
				{
					flingAngle = R_PointToAngle2(
						source->x - source->momx, source->y - source->momy,
						target->x, target->y
					);
				}
				else
				{
					flingAngle = target->angle + ANGLE_180;

					if (P_RandomByte() & 1)
					{
						flingAngle -= ANGLE_45;
					}
					else
					{
						flingAngle += ANGLE_45;
					}
				}

				P_InstaThrust(target, flingAngle, 14 * target->scale);
				P_SetObjectMomZ(target, 14*FRACUNIT, false);

				P_PlayDeathSound(target);
			}
			break;

		case MT_METALSONIC_RACE:
			target->fuse = TICRATE*3;
			target->momx = target->momy = target->momz = 0;
			P_SetObjectMomZ(target, 14*FRACUNIT, false);
			target->flags = (target->flags & ~MF_NOGRAVITY)|(MF_NOCLIP|MF_NOCLIPTHING);
			break;

		// SRB2Kart:
		case MT_SMK_ICEBLOCK:
			{
				mobj_t *cur = target->hnext;
				while (cur && !P_MobjWasRemoved(cur))
				{
					P_SetMobjState(cur, S_SMK_ICEBLOCK2);
					cur = cur->hnext;
				}
				target->fuse = 10;
				S_StartSound(target, sfx_s3k80);
			}
			break;

		case MT_ITEMCAPSULE:
		{
			UINT8 i;
			mobj_t *attacker = inflictor ? inflictor : source;
			mobj_t *part = target->hnext;
			angle_t angle = FixedAngle(360*P_RandomFixed());
			INT16 spacing = (target->radius >> 1) / target->scale;

			// set respawn fuse
			if (modeattacking) // no respawns
				;
			else if (target->threshold == KITEM_SUPERRING)
				target->fuse = 20*TICRATE;
			else
				target->fuse = 40*TICRATE;

			// burst effects
			for (i = 0; i < 2; i++)
			{
				mobj_t *blast = P_SpawnMobjFromMobj(target, 0, 0, target->info->height >> 1, MT_BATTLEBUMPER_BLAST);
				P_InitAngle(blast, angle + i*ANGLE_90);
				P_SetScale(blast, 2*blast->scale/3);
				blast->destscale = 2*blast->scale;
			}

			// dust effects
			for (i = 0; i < 10; i++)
			{
				mobj_t *puff = P_SpawnMobjFromMobj(
					target,
					P_RandomRange(-spacing, spacing) * FRACUNIT,
					P_RandomRange(-spacing, spacing) * FRACUNIT,
					P_RandomRange(0, 4*spacing) * FRACUNIT,
					MT_SPINDASHDUST
				);

				P_SetScale(puff, (puff->destscale *= 2));
				puff->momz = puff->scale * P_MobjFlip(puff);

				P_Thrust(puff, R_PointToAngle2(target->x, target->y, puff->x, puff->y), 3*puff->scale);
				if (attacker)
				{
					puff->momx += attacker->momx;
					puff->momy += attacker->momy;
					puff->momz += attacker->momz;
				}
			}

			// remove inside item
			if (target->tracer && !P_MobjWasRemoved(target->tracer))
				P_RemoveMobj(target->tracer);

			// bust capsule caps
			while (part && !P_MobjWasRemoved(part))
			{
				P_InstaThrust(part, part->angle + ANGLE_90, 6 * part->target->scale);
				P_SetObjectMomZ(part, 6 * FRACUNIT, false);
				part->fuse = TICRATE/2;
				part->flags &= ~MF_NOGRAVITY;

				if (attacker)
				{
					part->momx += attacker->momx;
					part->momy += attacker->momy;
					part->momz += attacker->momz;
				}
				part = part->hnext;
			}

			// give the player an item!
			if (source && source->player)
			{
				player_t *player = source->player;

				// special behavior for ring capsules
				if (target->threshold == KITEM_SUPERRING)
				{
					K_AwardPlayerRings(player, 5 * target->movecount, true);
					break;
				}

				// special behavior for SPB capsules
				if (target->threshold == KITEM_SPB)
				{
					K_ThrowKartItem(player, true, MT_SPB, 1, 0);
					break;
				}

				if (target->threshold < 1 || target->threshold >= NUMKARTITEMS) // bruh moment prevention
				{
					player->itemtype = KITEM_SAD;
					player->itemamount = 1;
				}
				else
				{
					player->itemtype = target->threshold;
					if (K_GetShieldFromItem(player->itemtype) != KSHIELD_NONE) // never give more than 1 shield
						player->itemamount = 1;
					else
						player->itemamount = max(1, target->movecount);
				}
				player->karthud[khud_itemblink] = TICRATE;
				player->karthud[khud_itemblinkmode] = 0;
				player->itemroulette = 0;
				player->roulettetype = 0;
				if (P_IsDisplayPlayer(player))
					S_StartSound(NULL, sfx_itrolf);
			}
			break;
		}

		case MT_BATTLECAPSULE:
			{
				mobj_t *cur;

				numtargets++;
				target->fuse = 16;
				target->flags |= MF_NOCLIP|MF_NOCLIPTHING;

				cur = target->hnext;

				while (cur && !P_MobjWasRemoved(cur))
				{
					// Shoot every piece outward
					if (!(cur->x == target->x && cur->y == target->y))
					{
						P_InstaThrust(cur,
							R_PointToAngle2(target->x, target->y, cur->x, cur->y),
							R_PointToDist2(target->x, target->y, cur->x, cur->y) / 12
						);
					}

					cur->momz = 8 * target->scale * P_MobjFlip(target);

					cur->flags &= ~MF_NOGRAVITY;
					cur->tics = TICRATE;
					cur->frame &= ~FF_ANIMATE; // Stop animating the propellers

					cur = cur->hnext;
				}

				// All targets busted!
				if (numtargets >= maptargets)
				{
					UINT8 i;
					for (i = 0; i < MAXPLAYERS; i++)
						P_DoPlayerExit(&players[i]);
				}
			}
			break;

		case MT_BATTLEBUMPER:
			{
				mobj_t *owner = target->target;
				mobj_t *overlay;

				S_StartSound(target, sfx_kc52);
				target->flags &= ~MF_NOGRAVITY;

				target->destscale = (3 * target->destscale) / 2;
				target->scalespeed = FRACUNIT/100;

				if (owner && !P_MobjWasRemoved(owner))
				{
					P_Thrust(target, R_PointToAngle2(owner->x, owner->y, target->x, target->y), 4 * target->scale);
				}

				target->momz += (24 * target->scale) * P_MobjFlip(target);
				target->fuse = 8;

				overlay = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_OVERLAY);

				P_SetTarget(&target->tracer, overlay);
				P_SetTarget(&overlay->target, target);

				overlay->color = target->color;
				P_SetMobjState(overlay, S_INVISIBLE);
			}
			break;

		case MT_DROPTARGET:
		case MT_DROPTARGET_SHIELD:
			target->fuse = 1;
			break;

		default:
			break;
	}

	if ((target->type == MT_JAWZ || target->type == MT_JAWZ_DUD || target->type == MT_JAWZ_SHIELD) && !(target->flags2 & MF2_AMBUSH))
	{
		target->z += P_MobjFlip(target)*20*target->scale;
	}

	// kill tracer
	if (target->type == MT_FROGGER)
	{
		if (target->tracer && !P_MobjWasRemoved(target->tracer))
			P_KillMobj(target->tracer, inflictor, source, DMG_NORMAL);
	}

	if (target->type == MT_FROGGER || target->type == MT_ROBRA_HEAD || target->type == MT_BLUEROBRA_HEAD) // clean hnext list
	{
		mobj_t *cur = target->hnext;
		while (cur && !P_MobjWasRemoved(cur))
		{
			P_KillMobj(cur, inflictor, source, DMG_NORMAL);
			cur = cur->hnext;
		}
	}

	// Bounce up on death
	if (target->type == MT_SMK_PIPE || target->type == MT_SMK_MOLE || target->type == MT_SMK_THWOMP)
	{
		target->flags &= (~MF_NOGRAVITY);

		if (target->eflags & MFE_VERTICALFLIP)
			target->z -= target->height;
		else
			target->z += target->height;

		S_StartSound(target, target->info->deathsound);

		P_SetObjectMomZ(target, 8<<FRACBITS, false);
		if (inflictor)
			P_InstaThrust(target, R_PointToAngle2(inflictor->x, inflictor->y, target->x, target->y)+ANGLE_90, 16<<FRACBITS);
	}

	// Final state setting - do something instead of P_SetMobjState;
	// Final state setting - do something instead of P_SetMobjState;
	if (target->type == MT_SPIKE && target->info->deathstate != S_NULL)
	{
		const angle_t ang = ((inflictor) ? inflictor->angle : 0) + ANGLE_90;
		const fixed_t scale = target->scale;
		const fixed_t xoffs = P_ReturnThrustX(target, ang, 8*scale), yoffs = P_ReturnThrustY(target, ang, 8*scale);
		const UINT16 flip = (target->eflags & MFE_VERTICALFLIP);
		mobj_t *chunk;
		fixed_t momz;

		S_StartSound(target, target->info->deathsound);

		if (target->info->xdeathstate != S_NULL)
		{
			momz = 6*scale;
			if (flip)
				momz *= -1;
#define makechunk(angtweak, xmov, ymov) \
			chunk = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_SPIKE);\
			P_SetMobjState(chunk, target->info->xdeathstate);\
			chunk->health = 0;\
			P_InitAngle(chunk, angtweak);\
			P_UnsetThingPosition(chunk);\
			chunk->flags = MF_NOCLIP;\
			chunk->x += xmov;\
			chunk->y += ymov;\
			P_SetThingPosition(chunk);\
			P_InstaThrust(chunk,chunk->angle, 4*scale);\
			chunk->momz = momz

			makechunk(ang + ANGLE_180, -xoffs, -yoffs);
			makechunk(ang, xoffs, yoffs);

#undef makechunk
		}

		momz = 7*scale;
		if (flip)
			momz *= -1;

		chunk = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_SPIKE);
		P_SetMobjState(chunk, target->info->deathstate);
		chunk->health = 0;
		P_InitAngle(chunk, ang + ANGLE_180);
		P_UnsetThingPosition(chunk);
		chunk->flags = MF_NOCLIP;
		chunk->x -= xoffs;
		chunk->y -= yoffs;
		if (flip)
			chunk->z -= 12*scale;
		else
			chunk->z += 12*scale;
		P_SetThingPosition(chunk);
		P_InstaThrust(chunk, chunk->angle, 2*scale);
		chunk->momz = momz;

		P_SetMobjState(target, target->info->deathstate);
		target->health = 0;
		target->angle = ang;
		P_UnsetThingPosition(target);
		target->flags = MF_NOCLIP;
		target->x += xoffs;
		target->y += yoffs;
		target->z = chunk->z;
		P_SetThingPosition(target);
		P_InstaThrust(target, target->angle, 2*scale);
		target->momz = momz;
	}
	else if (target->type == MT_WALLSPIKE && target->info->deathstate != S_NULL)
	{
		const angle_t ang = (/*(inflictor) ? inflictor->angle : */target->angle) + ANGLE_90;
		const fixed_t scale = target->scale;
		const fixed_t xoffs = P_ReturnThrustX(target, ang, 8*scale), yoffs = P_ReturnThrustY(target, ang, 8*scale), forwardxoffs = P_ReturnThrustX(target, target->angle, 7*scale), forwardyoffs = P_ReturnThrustY(target, target->angle, 7*scale);
		const UINT16 flip = (target->eflags & MFE_VERTICALFLIP);
		mobj_t *chunk;
		boolean sprflip;

		S_StartSound(target, target->info->deathsound);
		if (!P_MobjWasRemoved(target->tracer))
			P_RemoveMobj(target->tracer);

		if (target->info->xdeathstate != S_NULL)
		{
			sprflip = P_RandomChance(FRACUNIT/2);

#define makechunk(angtweak, xmov, ymov) \
			chunk = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_WALLSPIKE);\
			P_SetMobjState(chunk, target->info->xdeathstate);\
			chunk->health = 0;\
			P_InitAngle(chunk, target->angle);\
			P_UnsetThingPosition(chunk);\
			chunk->flags = MF_NOCLIP;\
			chunk->x += xmov - forwardxoffs;\
			chunk->y += ymov - forwardyoffs;\
			P_SetThingPosition(chunk);\
			P_InstaThrust(chunk, angtweak, 4*scale);\
			chunk->momz = P_RandomRange(5, 7)*scale;\
			if (flip)\
				chunk->momz *= -1;\
			if (sprflip)\
				chunk->frame |= FF_VERTICALFLIP

			makechunk(ang + ANGLE_180, -xoffs, -yoffs);
			sprflip = !sprflip;
			makechunk(ang, xoffs, yoffs);

#undef makechunk
		}

		sprflip = P_RandomChance(FRACUNIT/2);

		chunk = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_WALLSPIKE);

		P_SetMobjState(chunk, target->info->deathstate);
		chunk->health = 0;
		P_InitAngle(chunk, target->angle);
		P_UnsetThingPosition(chunk);
		chunk->flags = MF_NOCLIP;
		chunk->x += forwardxoffs - xoffs;
		chunk->y += forwardyoffs - yoffs;
		P_SetThingPosition(chunk);
		P_InstaThrust(chunk, ang + ANGLE_180, 2*scale);
		chunk->momz = P_RandomRange(5, 7)*scale;
		if (flip)
			chunk->momz *= -1;
		if (sprflip)
			chunk->frame |= FF_VERTICALFLIP;

		P_SetMobjState(target, target->info->deathstate);
		target->health = 0;
		P_UnsetThingPosition(target);
		target->flags = MF_NOCLIP;
		target->x += forwardxoffs + xoffs;
		target->y += forwardyoffs + yoffs;
		P_SetThingPosition(target);
		P_InstaThrust(target, ang, 2*scale);
		target->momz = P_RandomRange(5, 7)*scale;
		if (flip)
			target->momz *= -1;
		if (!sprflip)
			target->frame |= FF_VERTICALFLIP;
	}
	else if (target->player)
	{
		P_SetPlayerMobjState(target, target->info->deathstate);
	}
	else
#ifdef DEBUG_NULL_DEATHSTATE
		P_SetMobjState(target, S_NULL);
#else
		P_SetMobjState(target, target->info->deathstate);
#endif

	/** \note For player, the above is redundant because of P_SetMobjState (target, S_PLAY_DIE1)
	   in P_DamageMobj()
	   Graue 12-22-2003 */
}

static boolean P_PlayerHitsPlayer(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	(void)inflictor;
	(void)damage;

	// SRB2Kart: We want to hurt ourselves, so it's now DMG_CANTHURTSELF
	if (damagetype & DMG_CANTHURTSELF)
	{
		// You can't kill yourself, idiot...
		if (source == target)
			return false;

		if (G_GametypeHasTeams())
		{
			// Don't hurt your team, either!
			if (source->player->ctfteam == target->player->ctfteam)
				return false;
		}
	}

	return true;
}

static boolean P_KillPlayer(player_t *player, mobj_t *inflictor, mobj_t *source, UINT8 type)
{
	(void)source;

	if (player->exiting)
	{
		player->mo->destscale = 1;
		player->mo->flags |= MF_NOCLIPTHING;
		return false;
	}

	K_DestroyBumpers(player, 1);

	switch (type)
	{
		case DMG_DEATHPIT:
			// Respawn kill types
			K_DoIngameRespawn(player);
			return false;
		case DMG_SPECTATOR:
			// disappearifies, but still gotta put items back in play
			break;
		default:
			// Everything else REALLY kills
			if (leveltime < starttime)
			{
				K_DoFault(player);
			}
			break;
	}

	K_DropEmeraldsFromPlayer(player, player->emeralds);
	K_SetHitLagForObjects(player->mo, inflictor, MAXHITLAGTICS, true);

	player->carry = CR_NONE;

	K_KartResetPlayerColor(player);

	P_ResetPlayer(player);

	if (player->spectator == false)
	{
		player->mo->renderflags &= ~RF_DONTDRAW;
	}

	P_SetPlayerMobjState(player->mo, player->mo->info->deathstate);

	if (type == DMG_TIMEOVER)
	{
		if (gametyperules & GTR_CIRCUIT)
		{
			mobj_t *boom;

			player->mo->flags |= (MF_NOGRAVITY|MF_NOCLIP);
			player->mo->renderflags |= RF_DONTDRAW;

			boom = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_FZEROBOOM);
			boom->scale = player->mo->scale;
			P_InitAngle(boom, player->mo->angle);
			P_SetTarget(&boom->target, player->mo);
		}

		K_DestroyBumpers(player, player->bumpers);
		player->pflags |= PF_ELIMINATED;
	}

	return true;
}

/** Damages an object, which may or may not be a player.
  * For melee attacks, source and inflictor are the same.
  *
  * \param target     The object being damaged.
  * \param inflictor  The thing that caused the damage: creature, missile,
  *                   gargoyle, and so forth. Can be NULL in the case of
  *                   environmental damage, such as slime or crushing.
  * \param source     The creature or person responsible. For example, if a
  *                   player is hit by a ring, the player who shot it. In some
  *                   cases, the target will go after this object after
  *                   receiving damage. This can be NULL.
  * \param damage     Amount of damage to be dealt.
  * \param damagetype Type of damage to be dealt. If bit 7 (0x80) is set, this is an instant-kill.
  * \return True if the target sustained damage, otherwise false.
  * \todo Clean up this mess, split into multiple functions.
  * \sa P_KillMobj
  */
boolean P_DamageMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	player_t *player;
	boolean force = false;

	INT32 laglength = 6;

	if (objectplacing)
		return false;

	if (target->health <= 0)
		return false;

	// Spectator handling
	if (damagetype != DMG_SPECTATOR && target->player && target->player->spectator)
		return false;

	if (source && source->player && source->player->spectator)
		return false;

	if (((damagetype & DMG_TYPEMASK) == DMG_STING)
	|| ((inflictor && !P_MobjWasRemoved(inflictor)) && inflictor->type == MT_BANANA && inflictor->health <= 1))
	{
		laglength = 2;
	}
	else if (target->type == MT_DROPTARGET || target->type == MT_DROPTARGET_SHIELD)
	{
		laglength = 0; // handled elsewhere
	}

	// Everything above here can't be forced.
	if (!metalrecording)
	{
		UINT8 shouldForce = LUAh_ShouldDamage(target, inflictor, source, damage, damagetype);
		if (P_MobjWasRemoved(target))
			return (shouldForce == 1); // mobj was removed
		if (shouldForce == 1)
			force = true;
		else if (shouldForce == 2)
			return false;
	}

	if (!force)
	{
		if (!(target->flags & MF_SHOOTABLE))
			return false; // shouldn't happen...

		if (!(damagetype & DMG_DEATHMASK) && target->hitlag > 0 && inflictor == NULL)
			return false;
	}

	if (target->flags2 & MF2_SKULLFLY)
		target->momx = target->momy = target->momz = 0;

	if (target->flags & (MF_ENEMY|MF_BOSS))
	{
		if (!force && target->flags2 & MF2_FRET) // Currently flashing from being hit
			return false;

		if (LUAh_MobjDamage(target, inflictor, source, damage, damagetype) || P_MobjWasRemoved(target))
			return true;

		if (target->health > 1)
			target->flags2 |= MF2_FRET;
	}

	player = target->player;

	if (player) // Player is the target
	{
		if (player->pflags & PF_GODMODE)
			return false;

		if (!force)
		{
			// Player hits another player
			if (source && source->player)
			{
				if (!P_PlayerHitsPlayer(target, inflictor, source, damage, damagetype))
					return false;
			}
		}

		// Instant-Death
		if ((damagetype & DMG_DEATHMASK))
		{
			if (!P_KillPlayer(player, inflictor, source, damagetype))
				return false;
		}
		else if (LUAh_MobjDamage(target, inflictor, source, damage, damagetype))
		{
			return true;
		}
		else
		{
			const UINT8 type = (damagetype & DMG_TYPEMASK);
			const boolean hardhit = (type == DMG_EXPLODE || type == DMG_KARMA || type == DMG_TUMBLE); // This damage type can do evil stuff like ALWAYS combo
			INT16 ringburst = 5;

			// Check if the player is allowed to be damaged!
			// If not, then spawn the instashield effect instead.
			if (!force && !(inflictor && inflictor->type == MT_SPBEXPLOSION && inflictor->extravalue1 == 1))
			{
				if (gametyperules & GTR_BUMPERS)
				{
					if (player->bumpers <= 0 && player->karmadelay)
					{
						// No bumpers & in WAIT, can't be hurt
						K_DoInstashield(player);
						return false;
					}
				}
				else
				{
					if (damagetype & DMG_STEAL)
					{
						// Gametype does not have bumpers, steal damage is intended to not do anything
						// (No instashield is intentional)
						return false;
					}
				}

				if (player->invincibilitytimer > 0 || player->growshrinktimer > 0 || player->hyudorotimer > 0)
				{
					// Full invulnerability
					K_DoInstashield(player);
					return false;
				}

				{
					// Check if we should allow wombo combos (hard hits by default, inverted by the presence of DMG_WOMBO).
					boolean allowcombo = (hardhit == !(damagetype & DMG_WOMBO));

					// Tumble is a special case.
					if (type == DMG_TUMBLE)
					{
						// don't allow constant combo
						if (player->tumbleBounces == 1 && (P_MobjFlip(target)*target->momz > 0))
							allowcombo = false;
					}

					if ((target->hitlag == 0 || allowcombo == false) && player->flashing > 0)
					{
						// Post-hit invincibility
						K_DoInstashield(player);
						return false;
					}
				}
			}

			// We successfully damaged them! Give 'em some bumpers!
			if (type != DMG_STING)
			{
				UINT8 takeBumpers = 1;

				if (damagetype & DMG_STEAL)
				{
					takeBumpers = 2;

					if (type == DMG_KARMA)
					{
						takeBumpers = player->bumpers;
					}
				}
				else
				{
					if (type == DMG_KARMA)
					{
						// Take half of their bumpers for karma comeback damage
						takeBumpers = max(1, player->bumpers / 2);
					}
				}

				if (source && source != player->mo && source->player)
				{
					// Extend the invincibility if the hit was a direct hit.
					if (inflictor == source && source->player->invincibilitytimer)
					{
						tic_t kinvextend;

						if (gametype == GT_BATTLE)
							kinvextend = 2*TICRATE;
						else
							kinvextend = 5*TICRATE;

						source->player->invincibilitytimer += kinvextend;
					}

					K_TryHurtSoundExchange(target, source);

					K_BattleAwardHit(source->player, player, inflictor, takeBumpers);
					K_TakeBumpersFromPlayer(source->player, player, takeBumpers);

					if (type == DMG_KARMA)
					{
						// Destroy any remainder bumpers from the player for karma comeback damage
						K_DestroyBumpers(player, player->bumpers);
					}
					else
					{
						source->player->overtimekarma += 5*TICRATE;
					}

					if (damagetype & DMG_STEAL)
					{
						// Give them ALL of your emeralds instantly :)
						source->player->emeralds |= player->emeralds;
						player->emeralds = 0;
						K_CheckEmeralds(source->player);
					}
				}
				else
				{
					K_DestroyBumpers(player, takeBumpers);
				}

				if (!(damagetype & DMG_STEAL))
				{
					// Drop all of your emeralds
					K_DropEmeraldsFromPlayer(player, player->emeralds);
				}
			}

			player->sneakertimer = player->numsneakers = 0;
			player->driftboost = player->strongdriftboost = 0;
			player->ringboost = 0;
			player->glanceDir = 0;
			player->pflags &= ~PF_LOOKDOWN;

			switch (type)
			{
				case DMG_STING:
					K_DebtStingPlayer(player, source);
					K_KartPainEnergyFling(player);
					ringburst = 0;
					break;
				case DMG_TUMBLE:
					K_TumblePlayer(player, inflictor, source);
					ringburst = 10;
					break;
				case DMG_EXPLODE:
				case DMG_KARMA:
					ringburst = K_ExplodePlayer(player, inflictor, source);
					break;
				case DMG_WIPEOUT:
					if (P_IsDisplayPlayer(player))
						P_StartQuake(32<<FRACBITS, 5);
					K_SpinPlayer(player, inflictor, source, KSPIN_WIPEOUT);
					K_KartPainEnergyFling(player);
					break;
				case DMG_NORMAL:
				default:
					K_SpinPlayer(player, inflictor, source, KSPIN_SPINOUT);
					break;
			}

			if (type != DMG_STING)
			{
				player->flashing = K_GetKartFlashing(player);
			}

			P_PlayRinglossSound(target);

			if (ringburst > 0)
			{
				P_PlayerRingBurst(player, ringburst);
			}

			K_PlayPainSound(target, source);

			if ((hardhit == true) || (cv_kartdebughuddrop.value && !modeattacking))
			{
				K_DropItems(player);
			}
			else
			{
				K_DropHnextList(player, false);
			}

			player->instashield = 15;
			K_SetHitLagForObjects(target, inflictor, laglength, true);

			if (inflictor && !P_MobjWasRemoved(inflictor) && inflictor->type == MT_BANANA)
			{
				player->flipDI = true;
			}

			return true;
		}
	}
	else
	{
		if (damagetype & DMG_STEAL)
		{
			// Not a player, steal damage is intended to not do anything
			return false;
		}
	}

	// do the damage
	if (damagetype & DMG_DEATHMASK)
		target->health = 0;
	else
		target->health -= damage;

	if (source && source->player && target)
		G_GhostAddHit((INT32) (source->player - players), target);

	K_SetHitLagForObjects(target, inflictor, laglength, true);

	if (target->health <= 0)
	{
		P_KillMobj(target, inflictor, source, damagetype);
		return true;
	}

	//K_SetHitLagForObjects(target, inflictor, laglength, true);

	if (player)
		P_ResetPlayer(target->player);
	else
		P_SetMobjState(target, target->info->painstate);

	if (!P_MobjWasRemoved(target))
	{
		// if not intent on another player,
		// chase after this one
		P_SetTarget(&target->target, source);
	}

	return true;
}

static void P_FlingBurst
(		player_t *player,
		angle_t fa,
		mobjtype_t objType,
		tic_t objFuse,
		fixed_t objScale,
		INT32 i)
{
	mobj_t *mo;
	fixed_t ns;
	fixed_t momxy = 5<<FRACBITS, momz = 12<<FRACBITS; // base horizonal/vertical thrusts
	INT32 mx = (i + 1) >> 1;

	mo = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, objType);

	mo->threshold = 10; // not useful for spikes
	mo->fuse = objFuse;
	P_SetTarget(&mo->target, player->mo);

	if (objScale != FRACUNIT)
	{
		P_SetScale(mo, FixedMul(objScale, mo->scale));
		mo->destscale = mo->scale;
	}

	/*
	0: 0
	1: 1 = (1+1)/2 = 1
	2: 1 = (2+1)/2 = 1
	3: 2 = (3+1)/2 = 2
	4: 2 = (4+1)/2 = 2
	5: 3 = (4+1)/2 = 2
	 */
	// Angle / height offset changes every other ring
	momxy -= mx * FRACUNIT;
	momz += mx * (2<<FRACBITS);

	if (i & 1)
		fa += ANGLE_180;

	ns = FixedMul(momxy, player->mo->scale);
	mo->momx = (mo->target->momx/2) + FixedMul(FINECOSINE(fa>>ANGLETOFINESHIFT), ns);
	mo->momy = (mo->target->momy/2) + FixedMul(FINESINE(fa>>ANGLETOFINESHIFT), ns);

	ns = FixedMul(momz, player->mo->scale);
	mo->momz = (mo->target->momz/2) + ((ns) * P_MobjFlip(mo));
}

/** Spills an injured player's rings.
  *
  * \param player    The player who is losing rings.
  * \param num_rings Number of rings lost. A maximum of 20 rings will be
  *                  spawned.
  * \sa P_PlayerFlagBurst
  */
void P_PlayerRingBurst(player_t *player, INT32 num_rings)
{
	INT32 num_fling_rings;
	INT32 i;
	angle_t fa;

	// Rings shouldn't be in Battle!
	if (gametyperules & GTR_SPHERES)
		return;

	// Better safe than sorry.
	if (!player)
		return;

	// Have a shield? You get hit, but don't lose your rings!
	if (K_GetShieldFromItem(player->itemtype) != KSHIELD_NONE)
		return;

	// 20 is the maximum number of rings that can be taken from you at once - half the span of your counter
	if (num_rings > 20)
		num_rings = 20;
	else if (num_rings <= 0)
		return;

	num_rings = -P_GivePlayerRings(player, -num_rings);
	num_fling_rings = num_rings+min(0, player->rings);

	// determine first angle
	fa = player->mo->angle + ((P_RandomByte() & 1) ? -ANGLE_90 : ANGLE_90);

	for (i = 0; i < num_fling_rings; i++)
	{
		P_FlingBurst(player, fa, MT_FLINGRING, 60*TICRATE, FRACUNIT, i);
	}

	while (i < num_rings)
	{
		P_FlingBurst(player, fa, MT_DEBTSPIKE, 0, 3 * FRACUNIT / 2, i++);
	}
}
