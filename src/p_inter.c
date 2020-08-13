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
	if (player->exiting || mapreset)
		return false;

	/*if ((gametyperules & GTR_BUMPERS) && player->kartstuff[k_bumper] <= 0) // No bumpers in Match
		return false;*/

	if (weapon)
	{
		// Item slot already taken up
		if (weapon == 2)
		{
			// Invulnerable
			if (player->powers[pw_flashing] > 0)
				return false;

			// Already have fake
			if (player->kartstuff[k_roulettetype] == 2
				|| player->kartstuff[k_eggmanexplode])
				return false;
		}
		else
		{
			// Item-specific timer going off
			if (player->kartstuff[k_stealingtimer] || player->kartstuff[k_stolentimer]
				|| player->kartstuff[k_rocketsneakertimer]
				|| player->kartstuff[k_eggmanexplode])
				return false;

			// Item slot already taken up
			if (player->kartstuff[k_itemroulette]
				|| (weapon != 3 && player->kartstuff[k_itemamount])
				|| player->kartstuff[k_itemheld])
				return false;

			if (weapon == 3 && K_GetShieldFromItem(player->kartstuff[k_itemtype]) != KSHIELD_NONE)
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
		if (toucher->momz < 0) {
			if (toucher->z + toucher->momz > special->z + special->height)
				return;
		} else if (toucher->z > special->z + special->height)
			return;
		if (toucher->momz > 0) {
			if (toucher->z + toucher->height + toucher->momz < special->z)
				return;
		} else if (toucher->z + toucher->height < special->z)
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

		P_DamageMobj(toucher, special, special, 1, 0);
		return;
	}
	else if (special->flags & MF_FIRE)
	{
		P_DamageMobj(toucher, special, special, 1, DMG_FIRE);
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

			P_TeleportMove(player->mo, special->target->x, special->target->y, special->target->z + (48<<FRACBITS));
			player->mo->angle = special->target->angle;
			P_SetObjectMomZ(player->mo, 12<<FRACBITS, false);
			P_InstaThrust(player->mo, player->mo->angle, 20<<FRACBITS);
			return;
		case MT_FLOATINGITEM: // SRB2kart
			if (!P_CanPickupItem(player, 3) || (player->kartstuff[k_itemamount] && player->kartstuff[k_itemtype] != special->threshold))
				return;

			if ((gametyperules & GTR_BUMPERS) && player->kartstuff[k_bumper] <= 0)
				return;

			player->kartstuff[k_itemtype] = special->threshold;
			player->kartstuff[k_itemamount] += special->movecount;
			if (player->kartstuff[k_itemamount] > 255)
				player->kartstuff[k_itemamount] = 255;

			S_StartSound(special, special->info->deathsound);

			P_SetTarget(&special->tracer, toucher);
			special->flags2 |= MF2_NIGHTSPULL;
			special->destscale = mapobjectscale>>4;
			special->scalespeed <<= 1;

			special->flags &= ~MF_SPECIAL;
			return;
		case MT_RANDOMITEM:			// SRB2kart
			if (!P_CanPickupItem(player, 1))
				return;

			if ((gametyperules & GTR_BUMPERS) && player->kartstuff[k_bumper] <= 0)
			{
				if (player->kartstuff[k_comebackmode] || player->kartstuff[k_comebacktimer])
					return;
				player->kartstuff[k_comebackmode] = 1;
			}

			special->momx = special->momy = special->momz = 0;
			P_SetTarget(&special->target, toucher);
			P_KillMobj(special, toucher, toucher);
			break;
		case MT_KARMAHITBOX:
			if (!special->target->player)
				return;
			if (player == special->target->player)
				return;
			if (player->kartstuff[k_bumper] <= 0)
				return;
			if (special->target->player->exiting || player->exiting)
				return;

			if (special->target->player->kartstuff[k_comebacktimer]
				|| special->target->player->kartstuff[k_spinouttimer]
				|| special->target->player->kartstuff[k_squishedtimer])
				return;

			if (!special->target->player->kartstuff[k_comebackmode])
			{
				if (player->kartstuff[k_growshrinktimer] || player->kartstuff[k_squishedtimer]
					|| player->kartstuff[k_hyudorotimer] || player->kartstuff[k_spinouttimer]
					|| player->kartstuff[k_invincibilitytimer] || player->powers[pw_flashing])
					return;
				else
				{
					mobj_t *boom = P_SpawnMobj(special->target->x, special->target->y, special->target->z, MT_BOOMEXPLODE);
					UINT8 ptadd = (K_IsPlayerWanted(player) ? 2 : 1);

					boom->scale = special->target->scale;
					boom->destscale = special->target->scale;
					boom->momz = 5*FRACUNIT;
					if (special->target->color)
						boom->color = special->target->color;
					else
						boom->color = SKINCOLOR_KETCHUP;
					S_StartSound(boom, special->info->attacksound);

					if (player->kartstuff[k_bumper] == 1) // If you have only one bumper left, and see if it's a 1v1
					{
						INT32 numingame = 0;

						for (i = 0; i < MAXPLAYERS; i++)
						{
							if (!playeringame[i] || players[i].spectator || players[i].kartstuff[k_bumper] <= 0)
								continue;
							numingame++;
						}

						if (numingame <= 2) // If so, then an extra karma point so they are 100% certain to switch places; it's annoying to end matches with a bomb kill
							ptadd++;
					}

					special->target->player->kartstuff[k_comebackpoints] += ptadd;

					if (ptadd > 1)
						special->target->player->karthud[khud_yougotem] = 2*TICRATE;

					if (special->target->player->kartstuff[k_comebackpoints] >= 2)
						K_StealBumper(special->target->player, player, true);

					special->target->player->kartstuff[k_comebacktimer] = comebacktime;

					K_ExplodePlayer(player, special->target, special);
				}
			}
			else if (special->target->player->kartstuff[k_comebackmode] == 1 && P_CanPickupItem(player, 1))
			{
				mobj_t *poof = P_SpawnMobj(special->x, special->y, special->z, MT_EXPLODE);
				S_StartSound(poof, special->info->seesound);

				// Karma fireworks
				for (i = 0; i < 5; i++)
				{
					mobj_t *firework = P_SpawnMobj(special->x, special->y, special->z, MT_KARMAFIREWORK);
					firework->momx = (special->target->momx + toucher->momx) / 2;
					firework->momy = (special->target->momy + toucher->momy) / 2;
					firework->momz = (special->target->momz + toucher->momz) / 2;
					P_Thrust(firework, FixedAngle((72*i)<<FRACBITS), P_RandomRange(1,8)*special->scale);
					P_SetObjectMomZ(firework, P_RandomRange(1,8)*special->scale, false);
					firework->color = special->target->color;
				}

				special->target->player->kartstuff[k_comebackmode] = 0;
				special->target->player->kartstuff[k_comebackpoints]++;

				if (special->target->player->kartstuff[k_comebackpoints] >= 2)
					K_StealBumper(special->target->player, player, true);
				special->target->player->kartstuff[k_comebacktimer] = comebacktime;

				player->kartstuff[k_itemroulette] = 1;
				player->kartstuff[k_roulettetype] = 1;
			}
			else if (special->target->player->kartstuff[k_comebackmode] == 2 && P_CanPickupItem(player, 2))
			{
				mobj_t *poof = P_SpawnMobj(special->x, special->y, special->z, MT_EXPLODE);
				UINT8 ptadd = 1; // No WANTED bonus for tricking

				S_StartSound(poof, special->info->seesound);

				if (player->kartstuff[k_bumper] == 1) // If you have only one bumper left, and see if it's a 1v1
				{
					INT32 numingame = 0;

					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i] || players[i].spectator || players[i].kartstuff[k_bumper] <= 0)
							continue;
						numingame++;
					}

					if (numingame <= 2) // If so, then an extra karma point so they are 100% certain to switch places; it's annoying to end matches with a fake kill
						ptadd++;
				}

				special->target->player->kartstuff[k_comebackmode] = 0;
				special->target->player->kartstuff[k_comebackpoints] += ptadd;

				if (ptadd > 1)
					special->target->player->karthud[khud_yougotem] = 2*TICRATE;

				if (special->target->player->kartstuff[k_comebackpoints] >= 2)
					K_StealBumper(special->target->player, player, true);

				special->target->player->kartstuff[k_comebacktimer] = comebacktime;

				K_DropItems(player); //K_StripItems(player);
				//K_StripOther(player);

				player->kartstuff[k_itemroulette] = 1;
				player->kartstuff[k_roulettetype] = 2;

				if (special->target->player->kartstuff[k_eggmanblame] >= 0
				&& special->target->player->kartstuff[k_eggmanblame] < MAXPLAYERS
				&& playeringame[special->target->player->kartstuff[k_eggmanblame]]
				&& !players[special->target->player->kartstuff[k_eggmanblame]].spectator)
					player->kartstuff[k_eggmanblame] = special->target->player->kartstuff[k_eggmanblame];
				else
					player->kartstuff[k_eggmanblame] = -1;

				special->target->player->kartstuff[k_eggmanblame] = -1;
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

				if (player->kartstuff[k_bubbleblowup] > 0)
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
				K_SpinPlayer(player, special->target, 0, special, false);
			return;
		/*case MT_EERIEFOG:
			special->frame &= ~FF_TRANS80;
			special->frame |= FF_TRANS90;
			return;*/
		case MT_SMK_MOLE:
			if (special->target && !P_MobjWasRemoved(special->target))
				return;

			if (special->health <= 0 || toucher->health <= 0)
				return;

			if (!player->mo || player->spectator)
				return;

			// kill
			if (player->kartstuff[k_invincibilitytimer] > 0
				|| player->kartstuff[k_growshrinktimer] > 0
				|| player->kartstuff[k_flamedash] > 0)
			{
				P_KillMobj(special, toucher, toucher);
				return;
			}

			// no interaction
			if (player->powers[pw_flashing] > 0 || player->kartstuff[k_hyudorotimer] > 0
				|| player->kartstuff[k_squishedtimer] > 0 || player->kartstuff[k_spinouttimer] > 0)
				return;

			// attach to player!
			P_SetTarget(&special->target, toucher);
			S_StartSound(special, sfx_s1a2);
			return;
		case MT_CDUFO: // SRB2kart
			if (special->fuse || !P_CanPickupItem(player, 1) || ((gametyperules & GTR_BUMPERS) && player->kartstuff[k_bumper] <= 0))
				return;

			player->kartstuff[k_itemroulette] = 1;
			player->kartstuff[k_roulettetype] = 1;

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

		case MT_RING:
		case MT_FLINGRING:
			if (special->extravalue1)
				return;

			// No picking up rings while SPB is targetting you
			if (player->kartstuff[k_ringlock])
				return;

			// Don't immediately pick up spilled rings
			if (special->threshold > 0
			|| player->kartstuff[k_squishedtimer]
			|| player->kartstuff[k_spinouttimer])
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
			player->kartstuff[k_pickuprings]++;

			return;

		// Secret emblem thingy
		case MT_EMBLEM:
			{
				if (demo.playback)
					return;

				emblemlocations[special->health-1].collected = true;
				M_UpdateUnlockablesAndExtraEmblems();
				G_SaveGameData();
				break;
			}

		// CTF Flags
		case MT_REDFLAG:
		case MT_BLUEFLAG:
			if (player->powers[pw_flashing] || player->tossdelay)
				return;
			if (!special->spawnpoint)
				return;
			if (special->fuse == 1)
				return;
//			if (special->momz > 0)
//				return;
			{
				UINT8 flagteam = (special->type == MT_REDFLAG) ? 1 : 2;
				const char *flagtext;
				char flagcolor;
				char plname[MAXPLAYERNAME+4];

				if (special->type == MT_REDFLAG)
				{
					flagtext = M_GetText("Red flag");
					flagcolor = '\x85';
				}
				else
				{
					flagtext = M_GetText("Blue flag");
					flagcolor = '\x84';
				}
				snprintf(plname, sizeof(plname), "%s%s%s",
						 CTFTEAMCODE(player),
						 player_names[player - players],
						 CTFTEAMENDCODE(player));

				if (player->ctfteam == flagteam) // Player is on the same team as the flag
				{
					// Ignore height, only check x/y for now
					// avoids stupid problems with some flags constantly returning
					if (special->x>>FRACBITS != special->spawnpoint->x
					    || special->y>>FRACBITS != special->spawnpoint->y)
					{
						special->fuse = 1;
						special->flags2 |= MF2_JUSTATTACKED;

						if (!P_MobjTouchingSectorSpecial(player->mo, 4, 2 + flagteam, false))
						{
							CONS_Printf(M_GetText("%s returned the %c%s%c to base.\n"), plname, flagcolor, flagtext, 0x80);

							// The fuse code plays this sound effect
							//if (players[consoleplayer].ctfteam == player->ctfteam)
							//	S_StartSound(NULL, sfx_hoop1);
						}
					}
				}
				else if (player->ctfteam) // Player is on the other team (and not a spectator)
				{
					UINT16 flagflag   = (special->type == MT_REDFLAG) ? GF_REDFLAG : GF_BLUEFLAG;
					mobj_t **flagmobj = (special->type == MT_REDFLAG) ? &redflag : &blueflag;

					if (player->powers[pw_super])
						return;

					player->gotflag |= flagflag;
					CONS_Printf(M_GetText("%s picked up the %c%s%c!\n"), plname, flagcolor, flagtext, 0x80);
					(*flagmobj) = NULL;
					// code for dealing with abilities is handled elsewhere now
					break;
				}
			}
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
	P_KillMobj(special, NULL, toucher, 0);
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
	size_t i;
	mobj_t *toucher = player->mo;
	mobj_t *checkbase = snaptopost ? post : toucher;

	if (player->bot)
		return;

	// Player must have touched all previous starposts
	if (post->health - player->starpostnum > 1)
	{
		// blatant reuse of a variable that's normally unused in circuit
		if (!player->tossdelay)
			S_StartSound(toucher, sfx_lose);
		player->tossdelay = 3;
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
//#define TESTOVERTIMEINFREEPLAY

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

	if (!(multiplayer || netgame))
		return;

	if (battlecapsules) // capsules override any time limit settings
		return;

	if (!(gametyperules & GTR_TIMELIMIT))
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
					INT32 b = 0;
					thinker_t *th;
					mobj_t *item = NULL;

					P_RespawnBattleBoxes(); // FORCE THESE TO BE RESPAWNED FOR THIS!!!!!!!

					// Find us an item box to center on.
					for (th = thinkercap.next; th != &thinkercap; th = th->next)
					{
						mobj_t *thismo;
						if (th->function.acp1 != (actionf_p1)P_MobjThinker)
							continue;
						thismo = (mobj_t *)th;

						if (thismo->type != MT_RANDOMITEM)
							continue;
						if (thismo->threshold == 69) // Disappears
							continue;

						b++;

						// Only select items that are on the ground, ignore ones in the air. Ambush flag inverts this rule.
						if ((!P_IsObjectOnGround(thismo)) != (thismo->flags2 & MF2_AMBUSH))
							continue;

						if (item == NULL || (b < nummapboxes && P_RandomChance(((nummapboxes-b)*FRACUNIT)/nummapboxes))) // This is to throw off the RNG some
							item = thismo;
						if (b >= nummapboxes) // end early if we've found them all already
							break;
					}

					if (item == NULL) // no item found, could happen if every item is in the air or has ambush flag, or the map has none
					{
						CONS_Alert(CONS_WARNING, "No usuable items for Battle overtime!\n");
						return;
					}

					item->threshold = 70; // Set constant respawn
					battleovertime.x = item->x;
					battleovertime.y = item->y;
					battleovertime.z = item->z;
					battleovertime.radius = 4096*mapobjectscale;
					battleovertime.minradius = (cv_overtime.value == 2 ? 40 : 512) * mapobjectscale;
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

			if ((UINT32)cv_pointlimit.value <= players[i].marescore)
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
	boolean griefed = false;

	// Check if all the players in the race have finished. If so, end the level.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (nospectategrief[i] != -1) // prevent spectate griefing
		{
			griefed = true;
		}

		if (!playeringame[i] || players[i].spectator || players[i].lives <= 0) // Not playing
		{
			// Y'all aren't even playing
			continue;
		}

		numplayersingame++;

		if (players[i].exiting || (players[i].pflags & PF_GAMETYPEOVER))
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

	if (eliminatelast == true && (numplayersingame <= numexiting-1))
	{
		// Everyone's done playing but one guy apparently.
		// Just kill everyone who is still playing.

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator || players[i].lives <= 0) // Not playing
			{
				// Y'all aren't even playing
				continue;
			}

			if (players[i].exiting || (players[i].pflags & PF_GAMETYPEOVER))
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
		memset(nospectategrief, -1, sizeof (nospectategrief));
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

	//if (inflictor && (inflictor->type == MT_SHELL || inflictor->type == MT_FIREBALL))
	//	P_SetTarget(&target->tracer, inflictor);

	if (!(maptol & TOL_NIGHTS) && G_IsSpecialStage(gamemap) && target->player && target->player->nightstime > 6)
		target->player->nightstime = 6; // Just let P_Ticker take care of the rest.

	if (target->flags & (MF_ENEMY|MF_BOSS))
		target->momx = target->momy = target->momz = 0;

	// SRB2kart
	if (target->type != MT_PLAYER && !(target->flags & MF_MONITOR)
		 && !(target->type == MT_ORBINAUT || target->type == MT_ORBINAUT_SHIELD
		 || target->type == MT_JAWZ || target->type == MT_JAWZ_DUD || target->type == MT_JAWZ_SHIELD
		 || target->type == MT_BANANA || target->type == MT_BANANA_SHIELD
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
	target->shadowscale = 0;

	if (LUAh_MobjDeath(target, inflictor, source, damagetype) || P_MobjWasRemoved(target))
		return;

	// SRB2kart
	// I wish I knew a better way to do this
	if (target->target && target->target->player && target->target->player->mo)
	{
		if (target->target->player->kartstuff[k_eggmanheld] && target->type == MT_EGGMANITEM_SHIELD)
			target->target->player->kartstuff[k_eggmanheld] = 0;

		if (target->target->player->kartstuff[k_itemheld])
		{
			if ((target->type == MT_BANANA_SHIELD && target->target->player->kartstuff[k_itemtype] == KITEM_BANANA) // trail items
				|| (target->type == MT_SSMINE_SHIELD && target->target->player->kartstuff[k_itemtype] == KITEM_MINE)
				|| (target->type == MT_SINK_SHIELD && target->target->player->kartstuff[k_itemtype] == KITEM_KITCHENSINK))
			{
				if (target->movedir != 0 && target->movedir < (UINT16)target->target->player->kartstuff[k_itemamount])
				{
					if (target->target->hnext)
						K_KillBananaChain(target->target->hnext, inflictor, source);
					target->target->player->kartstuff[k_itemamount] = 0;
				}
				else
					target->target->player->kartstuff[k_itemamount]--;
			}
			else if ((target->type == MT_ORBINAUT_SHIELD && target->target->player->kartstuff[k_itemtype] == KITEM_ORBINAUT) // orbit items
				|| (target->type == MT_JAWZ_SHIELD && target->target->player->kartstuff[k_itemtype] == KITEM_JAWZ))
			{
				target->target->player->kartstuff[k_itemamount]--;
				if (target->lastlook != 0)
				{
					K_RepairOrbitChain(target);
				}
			}

			if (target->target->player->kartstuff[k_itemamount] < 0)
				target->target->player->kartstuff[k_itemamount] = 0;

			if (!target->target->player->kartstuff[k_itemamount])
				target->target->player->kartstuff[k_itemheld] = 0;

			if (target->target->hnext == target)
				P_SetTarget(&target->target->hnext, NULL);
		}
	}
	//

	// Let EVERYONE know what happened to a player! 01-29-2002 Tails
	if (target->player && !target->player->spectator)
	{
		if (metalrecording) // Ack! Metal Sonic shouldn't die! Cut the tape, end recording!
			G_StopMetalRecording(true);

		target->drawflags &= ~MFD_DONTDRAW;
	}

	// if killed by a player
	if (source && source->player)
	{
		if (target->flags & MF_MONITOR || target->type == MT_RANDOMITEM)
		{
			UINT8 i;

			P_SetTarget(&target->target, source);
			source->player->numboxes++;

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
		target->flags &= ~(MF_SOLID|MF_SHOOTABLE); // does not block
		P_UnsetThingPosition(target);
		target->flags |= MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY;
		P_SetThingPosition(target);
		target->standingslope = NULL;
		target->pmomz = 0;

		target->player->playerstate = PST_DEAD;

		if (target->player == &players[consoleplayer])
		{
			// don't die in auto map,
			// switch view prior to dying
			if (automapactive)
				AM_Stop();

			//added : 22-02-98: recenter view for next life...
			localaiming[0] = 0;
		}

		if (target->player == &players[displayplayers[1]]) localaiming[1] = 0;
		if (target->player == &players[displayplayers[2]]) localaiming[2] = 0;
		if (target->player == &players[displayplayers[3]]) localaiming[3] = 0;

		if ((gametyperules & GTR_BUMPERS))
			K_CheckBumpers();

		target->player->kartstuff[k_pogospring] = 0;
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
					P_KillMobj(segment->tracer, NULL, NULL, 0);
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
					mo2->angle = mo->angle;
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
				P_KillMobj(target->target, target, source, 0);
			break;

		case MT_PLAYER:
			{
				target->fuse = TICRATE*3; // timer before mobj disappears from view (even if not an actual player)
				target->momx = target->momy = target->momz = 0;

				if (target->player && target->player->pflags & PF_GAMETYPEOVER)
					break;

				if (damagetype == DMG_DROWNED) // drowned
				{
					target->movedir = damagetype; // we're MOVING the Damage Into anotheR function... Okay, this is a bit of a hack.
					if (target->player->charflags & SF_MACHINE)
						S_StartSound(target, sfx_fizzle);
					else
						S_StartSound(target, sfx_drown);
					// Don't jump up when drowning
				}
				else
				{
					P_SetObjectMomZ(target, 14*FRACUNIT, false);
					if (damagetype == DMG_SPIKE) // Spikes
						S_StartSound(target, sfx_spkdth);
					else
						P_PlayDeathSound(target);
				}
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
			P_KillMobj(target->tracer, inflictor, source);
	}

	if (target->type == MT_FROGGER || target->type == MT_ROBRA_HEAD || target->type == MT_BLUEROBRA_HEAD) // clean hnext list
	{
		mobj_t *cur = target->hnext;
		while (cur && !P_MobjWasRemoved(cur))
		{
			P_KillMobj(cur, inflictor, source);
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
			chunk->angle = angtweak;\
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
			chunk->angle = target->angle;\
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
		chunk->angle = target->angle;
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
		if (damagetype == DMG_DROWNED || damagetype == DMG_SPACEDROWN)
			P_SetPlayerMobjState(target, target->info->xdeathstate);
		else
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
	player_t *player = target->player;

	// SRB2Kart: We want to hurt ourselves
	if (damagetype & DMG_CANTHURTSELF)
	{
		// You can't kill yourself, idiot...
		if (source == target)
			return false;
	}

	return true;
}

static void P_KillPlayer(player_t *player, mobj_t *source, INT32 damage)
{
	player->pflags &= ~PF_SLIDING;

	player->powers[pw_carry] = CR_NONE;

	// Get rid of shield
	player->powers[pw_shield] = SH_NONE;
	player->mo->color = player->skincolor;
	player->mo->colorized = false;

	P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);

	P_ResetPlayer(player);

	if (!player->spectator)
		player->mo->flags2 &= ~MF2_DONTDRAW;

	P_SetPlayerMobjState(player->mo, player->mo->info->deathstate);

	if (player->pflags & PF_GAMETYPEOVER)
	{
		mobj_t *boom;
		player->mo->flags |= (MF_NOGRAVITY|MF_NOCLIP);
		player->mo->drawflags |= MFD_DONTDRAW;
		boom = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_FZEROBOOM);
		boom->scale = player->mo->scale;
		boom->angle = player->mo->angle;
		P_SetTarget(&boom->target, player->mo);
	}

	if ((gametyperules & GTR_BUMPERS))
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

		K_CheckBumpers();
	}
}

void P_RemoveShield(player_t *player)
{
	if (player->powers[pw_shield] & SH_FORCE)
	{ // Multi-hit
		if (player->powers[pw_shield] & SH_FORCEHP)
			player->powers[pw_shield]--;
		else
			player->powers[pw_shield] &= SH_STACK;
	}
	else if (player->powers[pw_shield] & SH_NOSTACK)
	{ // First layer shields
		if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ARMAGEDDON) // Give them what's coming to them!
		{
			P_BlackOw(player); // BAM!
			player->pflags |= PF_JUMPDOWN;
		}
		else
			player->powers[pw_shield] &= SH_STACK;
	}
	else
	{ // Second layer shields
		if (((player->powers[pw_shield] & SH_STACK) == SH_FIREFLOWER) && !(player->powers[pw_super] || (mariomode && player->powers[pw_invulnerability])))
		{
			player->mo->color = player->skincolor;
			G_GhostAddColor((INT32) (player - players), GHC_NORMAL);
		}
		player->powers[pw_shield] = SH_NONE;
	}
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

	if (objectplacing)
		return false;

	if (target->health <= 0)
		return false;

	// Spectator handling
	if (multiplayer)
	{
		if (damagetype != DMG_SPECTATOR && target->player && target->player->spectator)
			return false;

		if (source && source->player && source->player->spectator)
			return false;
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

		// Make sure that boxes cannot be popped by enemies, red rings, etc.
		if (target->flags & MF_MONITOR && ((!source || !source->player) || (inflictor && !inflictor->player)))
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
		if (!force)
		{
			if (player->exiting)
				return false;

			if (player->pflags & PF_GODMODE)
				return false;
		}

		// Player hits another player
		if (!force && source && source->player)
		{
			if (!P_PlayerHitsPlayer(target, inflictor, source, damage, damagetype))
				return false;
		}

		// Instant-Death
		if (damagetype & DMG_DEATHMASK)
			P_KillPlayer(player, source, damage);
		else if (LUAh_MobjDamage(target, inflictor, source, damage, damagetype))
			return true;

		if (inflictor && (inflictor->type == MT_ORBINAUT || inflictor->type == MT_ORBINAUT_SHIELD
			|| inflictor->type == MT_JAWZ || inflictor->type == MT_JAWZ_SHIELD || inflictor->type == MT_JAWZ_DUD
			|| inflictor->type == MT_SMK_THWOMP || inflictor->player))
		{
			player->kartstuff[k_sneakertimer] = 0;
			player->kartstuff[k_numsneakers] = 0;

			K_SpinPlayer(player, source, 1, inflictor, false);
			K_KartPainEnergyFling(player);

			if (P_IsDisplayPlayer(player))
				P_StartQuake(32<<FRACBITS, 5);
		}
		else
		{
			K_SpinPlayer(player, source, 0, inflictor, false);
		}

		return true;
	}

	// do the damage
	if (damagetype & DMG_DEATHMASK)
		target->health = 0;
	else
		target->health -= damage;

	if (source && source->player && target)
		G_GhostAddHit((INT32) (source->player - players), target);

	if (target->health <= 0)
	{
		P_KillMobj(target, inflictor, source, damagetype);
		return true;
	}

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

/** Spills an injured player's rings.
  *
  * \param player    The player who is losing rings.
  * \param num_rings Number of rings lost. A maximum of 20 rings will be
  *                  spawned.
  * \sa P_PlayerFlagBurst
  */
void P_PlayerRingBurst(player_t *player, INT32 num_rings)
{
	INT32 i;
	mobj_t *mo;
	angle_t fa, va;
	fixed_t ns;
	fixed_t z;
	fixed_t momxy = 5<<FRACBITS, momz = 12<<FRACBITS; // base horizonal/vertical thrusts

	// Rings shouldn't be in Battle!
	if ((gametyperules & GTR_BUMPERS))
		return;

	// Better safe than sorry.
	if (!player)
		return;

	// Have a shield? You get hit, but don't lose your rings!
	if (K_GetShieldFromItem(player->kartstuff[k_itemtype]) != KSHIELD_NONE)
		return;

	// 20 is the ring cap in kart
	if (num_rings > 20)
		num_rings = 20;
	else if (num_rings <= 0)
		return;

	// Cap the maximum loss automatically to 2 in ring debt
	if (player->kartstuff[k_rings] <= 0 && num_rings > 2)
		num_rings = 2;

	P_GivePlayerRings(player, -num_rings);

	// determine first angle
	fa = player->mo->angle + ((P_RandomByte() & 1) ? -ANGLE_90 : ANGLE_90);

	for (i = 0; i < num_rings; i++)
	{
		INT32 objType = mobjinfo[MT_RING].reactiontime;

		z = player->mo->z;
		if (player->mo->eflags & MFE_VERTICALFLIP)
			z += player->mo->height - mobjinfo[objType].height;

		mo = P_SpawnMobj(player->mo->x, player->mo->y, z, objType);

		mo->threshold = 10;
		mo->fuse = 120*TICRATE;
		P_SetTarget(&mo->target, player->mo);

		mo->destscale = player->mo->scale;
		P_SetScale(mo, player->mo->scale);

		// Angle / height offset changes every other ring
		if (i != 0)
		{
			if (i & 1)
			{
				momxy -= FRACUNIT;
				momz += 2<<FRACBITS;
			}

			fa += ANGLE_180;
		}

		ns = FixedMul(momxy, mo->scale);
		mo->momx = (mo->target->momx/2) + FixedMul(FINECOSINE(fa>>ANGLETOFINESHIFT), ns);
		mo->momy = (mo->target->momy/2) + FixedMul(FINESINE(fa>>ANGLETOFINESHIFT), ns);

		ns = FixedMul(momz, mo->scale);
		P_SetObjectMomZ(mo, (mo->target->momz/2) + ns, false);

		if (player->mo->eflags & MFE_VERTICALFLIP)
			mo->momz *= -1;
	}
}
