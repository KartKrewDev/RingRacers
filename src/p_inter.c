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

	/*if (G_BattleGametype() && player->kartstuff[k_bumper] <= 0) // No bumpers in Match
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

		switch (special->type)
		{
			case MT_BLACKEGGMAN:
			{
				P_DamageMobj(toucher, special, special, 1, 0); // ouch
				return;
			}

			case MT_BIGMINE:
			{
				special->momx = toucher->momx/3;
				special->momy = toucher->momy/3;
				special->momz = toucher->momz/3;
				toucher->momx /= -8;
				toucher->momy /= -8;
				toucher->momz /= -8;
				special->flags &= ~MF_SPECIAL;
				if (special->info->activesound)
					S_StartSound(special, special->info->activesound);
				P_SetTarget(&special->tracer, toucher);
				player->homing = 0;
				return;
			}

			case MT_GSNAPPER:
				if (toucher->z < special->z + special->height
				&& toucher->z + toucher->height > special->z
				&& P_DamageMobj(toucher, special, special, 1, DMG_SPIKE))
					return; // Can only hit snapper from above
				break;

			case MT_SPINCUSHION:
				if (P_MobjFlip(toucher)*(toucher->z - (special->z + special->height/2)) > 0)
				{
					if (P_DamageMobj(toucher, special, special, 1, DMG_SPIKE))
						return; // Cannot hit sharp from above
				}
				break;

			case MT_FANG:
				if (!player->powers[pw_flashing])
				{
					if ((special->state == &states[S_FANG_BOUNCE3]
					  || special->state == &states[S_FANG_BOUNCE4]
					  || special->state == &states[S_FANG_PINCHBOUNCE3]
					  || special->state == &states[S_FANG_PINCHBOUNCE4])
					&& P_MobjFlip(special)*((special->z + special->height/2) - (toucher->z + toucher->height/2)) > (toucher->height/2))
					{
						P_DamageMobj(toucher, special, special, 1, 0);
						P_SetTarget(&special->tracer, toucher);

						if (special->state == &states[S_FANG_PINCHBOUNCE3]
						 || special->state == &states[S_FANG_PINCHBOUNCE4])
							P_SetMobjState(special, S_FANG_PINCHPATHINGSTART2);
						else
						{
							var1 = var2 = 4;
							A_Boss5ExtraRepeat(special);
							P_SetMobjState(special, S_FANG_PATHINGCONT2); //S_FANG_PATHINGCONT1 if you want him to drop a bomb on the player
						}
						if (special->eflags & MFE_VERTICALFLIP)
							special->z = toucher->z - special->height;
						else
							special->z = toucher->z + toucher->height;
						return;
					}
				}
				break;

			case MT_PYREFLY:
				if (special->extravalue2 == 2 && P_DamageMobj(player->mo, special, special, 1, DMG_FIRE))
					return;

			default:
				break;
		}

<<<<<<< HEAD
		if (special->type == MT_PTERABYTE && special->target == player->mo)
			return; // Don't hurt the player you're trying to grab
=======
		if (P_PlayerCanDamage(player, special)) // Do you possess the ability to subdue the object?
		{
			if (special->type == MT_PTERABYTE && special->target == player->mo && special->extravalue1 == 1)
				return; // Can't hurt a Pterabyte if it's trying to pick you up

			if ((P_MobjFlip(toucher)*toucher->momz < 0) && (elementalpierce != 1))
			{
				if (!(player->charability2 == CA2_MELEE && player->panim == PA_ABILITY2))
				{
					fixed_t setmomz = -toucher->momz; // Store this, momz get changed by P_DoJump within P_DoBubbleBounce
					
					if (elementalpierce == 2) // Reset bubblewrap, part 1
						P_DoBubbleBounce(player);
					toucher->momz = setmomz;
					if (elementalpierce == 2) // Reset bubblewrap, part 2
					{
						boolean underwater = toucher->eflags & MFE_UNDERWATER;
							
						if (underwater)
							toucher->momz /= 2;
						toucher->momz -= (toucher->momz/(underwater ? 8 : 4)); // Cap the height!
					}
				}
			}
			if (player->pflags & PF_BOUNCING)
				P_DoAbilityBounce(player, false);
			if (special->info->spawnhealth > 1) // Multi-hit? Bounce back!
			{
				toucher->momx = -toucher->momx;
				toucher->momy = -toucher->momy;
				if (player->charability == CA_FLY && player->panim == PA_ABILITY)
					toucher->momz = -toucher->momz/2;
				else if (player->pflags & PF_GLIDING && !P_IsObjectOnGround(toucher))
				{
					player->pflags &= ~(PF_GLIDING|PF_JUMPED|PF_NOJUMPDAMAGE);
					P_SetPlayerMobjState(toucher, S_PLAY_FALL);
					toucher->momz += P_MobjFlip(toucher) * (player->speed >> 3);
					toucher->momx = 7*toucher->momx>>3;
					toucher->momy = 7*toucher->momy>>3;
				}
				else if (player->dashmode >= DASHMODE_THRESHOLD && (player->charflags & (SF_DASHMODE|SF_MACHINE)) == (SF_DASHMODE|SF_MACHINE)
					&& player->panim == PA_DASH)
					P_DoPlayerPain(player, special, special);
			}
			P_DamageMobj(special, toucher, toucher, 1, 0);
			if (player->charability == CA_TWINSPIN && player->panim == PA_ABILITY)
				P_TwinSpinRejuvenate(player, player->thokitem);
		}
		else
		{
			if (special->type == MT_PTERABYTE && special->target == player->mo)
				return; // Don't hurt the player you're trying to grab

			P_DamageMobj(toucher, special, special, 1, 0);
		}
>>>>>>> srb2/next

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

			if (G_BattleGametype() && player->kartstuff[k_bumper] <= 0)
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

			if (G_BattleGametype() && player->kartstuff[k_bumper] <= 0)
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
			if (special->fuse || !P_CanPickupItem(player, 1) || (G_BattleGametype() && player->kartstuff[k_bumper] <= 0))
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
				M_UpdateUnlockablesAndExtraEmblems(false);
				G_SaveGameData(false);
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

<<<<<<< HEAD
=======
// ********************************** //
// NiGHTS gameplay items and powerups //
// ********************************** //
		case MT_NIGHTSDRONE:
			{
				boolean spec = G_IsSpecialStage(gamemap);
				boolean cangiveemmy = false;
				if (player->bot)
					return;
				if (player->exiting)
					return;
				if (player->bonustime)
				{
					if (spec) //After-mare bonus time/emerald reward in special stages.
					{
						// only allow the player with the emerald in-hand to leave.
						if (toucher->tracer
						&& toucher->tracer->type == MT_GOTEMERALD)
						{}
						else // Make sure that SOMEONE has the emerald, at least!
						{
							for (i = 0; i < MAXPLAYERS; i++)
								if (playeringame[i] && players[i].playerstate == PST_LIVE
								&& players[i].mo->tracer
								&& players[i].mo->tracer->type == MT_GOTEMERALD)
									return;
							// Well no one has an emerald, so exit anyway!
						}
						cangiveemmy = true;
						// Don't play Ideya sound in special stage mode
					}
					else
						S_StartSound(toucher, special->info->activesound);
				}
				else //Initial transformation. Don't allow second chances in special stages!
				{
					if (player->powers[pw_carry] == CR_NIGHTSMODE)
						return;

					S_StartSound(toucher, sfx_supert);
				}
				P_SwitchSpheresBonusMode(false);
				if (!(netgame || multiplayer) && !(player->powers[pw_carry] == CR_NIGHTSMODE))
					P_SetTarget(&special->tracer, toucher);
				P_SetTarget(&player->drone, special); // Mark the player as 'center into the drone'
				P_NightserizePlayer(player, special->health); // Transform!
				if (!spec)
				{
					if (toucher->tracer) // Move the Ideya to an anchor!
					{
						mobj_t *orbittarget = special->target ? special->target : special;
						mobj_t *hnext = orbittarget->hnext, *anchorpoint = NULL, *anchorpoint2 = NULL;
						mobj_t *mo2;
						thinker_t *th;

						// The player might have two Ideyas: toucher->tracer and toucher->tracer->hnext
						// so handle their anchorpoints accordingly.
						// scan the thinkers to find the corresponding anchorpoint
						for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
						{
							if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
								continue;

							mo2 = (mobj_t *)th;

							if (mo2->type != MT_IDEYAANCHOR)
								continue;

							if (mo2->health == toucher->tracer->health) // do ideya numberes match?
								anchorpoint = mo2;
							else if (toucher->tracer->hnext && mo2->health == toucher->tracer->hnext->health)
								anchorpoint2 = mo2;

							if ((!toucher->tracer->hnext && anchorpoint)
								|| (toucher->tracer->hnext && anchorpoint && anchorpoint2))
								break;
						}

						if (anchorpoint)
						{
							toucher->tracer->flags |= MF_GRENADEBOUNCE; // custom radius factors
							toucher->tracer->threshold = 8 << 20; // X factor 0, Y factor 0, Z factor 8
						}

						if (anchorpoint2)
						{
							toucher->tracer->hnext->flags |= MF_GRENADEBOUNCE; // custom radius factors
							toucher->tracer->hnext->threshold = 8 << 20; // X factor 0, Y factor 0, Z factor 8
						}

						P_SetTarget(&orbittarget->hnext, toucher->tracer);
						if (!orbittarget->hnext->hnext)
							P_SetTarget(&orbittarget->hnext->hnext, hnext); // Buffalo buffalo Buffalo buffalo buffalo buffalo Buffalo buffalo.
						else
							P_SetTarget(&orbittarget->hnext->hnext->target, anchorpoint2 ? anchorpoint2 : orbittarget);
						P_SetTarget(&orbittarget->hnext->target, anchorpoint ? anchorpoint : orbittarget);
						P_SetTarget(&toucher->tracer, NULL);

						if (hnext)
						{
							orbittarget->hnext->extravalue1 = (angle_t)(hnext->extravalue1 - 72*ANG1);
							if (orbittarget->hnext->extravalue1 > hnext->extravalue1)
								orbittarget->hnext->extravalue1 -= (72*ANG1)/orbittarget->hnext->extravalue1;
						}
					}
					if (player->exiting) // ...then move it back?
					{
						mobj_t *hnext = special->target ? special->target : special; // goalpost
						while ((hnext = hnext->hnext))
						{
							hnext->flags &= ~MF_GRENADEBOUNCE;
							hnext->threshold = 0;
							P_SetTarget(&hnext->target, toucher);
						}
					}
					return;
				}

				if (!cangiveemmy)
					return;

				if (player->exiting)
					P_GiveEmerald(false);
				else if (player->mo->tracer && player->mare)
				{
					P_KillMobj(toucher->tracer, NULL, NULL, 0); // No emerald for you just yet!
					S_StartSound(NULL, sfx_ghosty);
					special->flags2 |= MF2_DONTDRAW;
				}

				return;
			}
		case MT_NIGHTSLOOPHELPER:
			// One second delay
			if (special->fuse < toucher->fuse - TICRATE)
			{
				thinker_t *th;
				mobj_t *mo2;
				INT32 count;
				fixed_t x,y,z, gatherradius;
				angle_t d;
				statenum_t sparklestate = S_NULL;

				if (special->target != toucher) // These ain't your helpers, pal!
					return;

				x = special->x>>FRACBITS;
				y = special->y>>FRACBITS;
				z = special->z>>FRACBITS;
				count = 1;

				// scan the remaining thinkers
				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
						continue;

					mo2 = (mobj_t *)th;

					if (mo2 == special)
						continue;

					// Not our stuff!
					if (mo2->target != toucher)
						continue;

					if (mo2->type == MT_NIGHTSPARKLE)
						mo2->tics = 1;
					else if (mo2->type == MT_NIGHTSLOOPHELPER)
					{
						if (mo2->fuse >= special->fuse)
						{
							count++;
							x += mo2->x>>FRACBITS;
							y += mo2->y>>FRACBITS;
							z += mo2->z>>FRACBITS;
						}
						P_RemoveMobj(mo2);
					}
				}
				x = (x/count)<<FRACBITS;
				y = (y/count)<<FRACBITS;
				z = (z/count)<<FRACBITS;
				gatherradius = P_AproxDistance(P_AproxDistance(special->x - x, special->y - y), special->z - z);
				P_RemoveMobj(special);

				if (player->powers[pw_nights_superloop])
				{
					gatherradius *= 2;
					sparklestate = mobjinfo[MT_NIGHTSPARKLE].seestate;
				}

				if (gatherradius < 30*FRACUNIT) // Player is probably just sitting there.
					return;

				for (d = 0; d < 16; d++)
					P_SpawnParaloop(x, y, z, gatherradius, 16, MT_NIGHTSPARKLE, sparklestate, d*ANGLE_22h, false);

				S_StartSound(toucher, sfx_prloop);

				// Now we RE-scan all the thinkers to find close objects to pull
				// in from the paraloop. Isn't this just so efficient?
				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
						continue;

					mo2 = (mobj_t *)th;

					if (P_AproxDistance(P_AproxDistance(mo2->x - x, mo2->y - y), mo2->z - z) > gatherradius)
						continue;

					if (mo2->flags & MF_SHOOTABLE)
					{
						P_DamageMobj(mo2, toucher, toucher, 1, 0);
						continue;
					}

					// Make these APPEAR!
					// Tails 12-15-2003
					if (mo2->flags & MF_NIGHTSITEM)
					{
						// Requires Bonus Time
						if ((mo2->flags2 & MF2_STRONGBOX) && !player->bonustime)
							continue;

						if (!(mo2->flags & MF_SPECIAL) && mo2->health)
						{
							mo2->flags2 &= ~MF2_DONTDRAW;
							mo2->flags |= MF_SPECIAL;
							mo2->flags &= ~MF_NIGHTSITEM;
							S_StartSound(toucher, sfx_hidden);
							continue;
						}
					}

					if (!(mo2->type == MT_RING || mo2->type == MT_COIN
						|| mo2->type == MT_BLUESPHERE || mo2->type == MT_BOMBSPHERE
						|| mo2->type == MT_NIGHTSCHIP || mo2->type == MT_NIGHTSSTAR
						|| ((mo2->type == MT_EMBLEM) && (mo2->reactiontime & GE_NIGHTSPULL))))
						continue;

					// Yay! The thing's in reach! Pull it in!
					mo2->flags |= MF_NOCLIP|MF_NOCLIPHEIGHT;
					mo2->flags2 |= MF2_NIGHTSPULL;
					// New NiGHTS attract speed dummied out because the older behavior
					// is exploited as a mechanic. Uncomment to enable.
					mo2->movefactor = 0; // 32*FRACUNIT; // initialize the NightsItemChase timer
					P_SetTarget(&mo2->tracer, toucher);
				}
			}
			return;
		case MT_EGGCAPSULE:
			if (player->bot)
				return;

			// make sure everything is as it should be, THEN take rings from players in special stages
			if (player->powers[pw_carry] == CR_NIGHTSMODE && !toucher->target)
				return;

			if (toucher->tracer && toucher->tracer->health > 0)
				return; // Don't have multiple ideya, unless it's the first one given (health = 0)

			if (player->mare != special->threshold) // wrong mare
				return;

			if (special->reactiontime > 0) // capsule already has a player attacking it, ignore
				return;

			if (G_IsSpecialStage(gamemap) && !player->exiting)
			{ // In special stages, share spheres. Everyone gives up theirs to the player who touched the capsule
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && (&players[i] != player) && players[i].spheres > 0)
					{
						player->spheres += players[i].spheres;
						players[i].spheres = 0;
					}
			}

			if (player->spheres <= 0 || player->exiting)
				return;

			// Mark the player as 'pull into the capsule'
			P_SetTarget(&player->capsule, special);
			special->reactiontime = (player-players)+1;
			P_SetTarget(&special->target, NULL);

			// Clear text
			player->texttimer = 0;
			return;
		case MT_NIGHTSBUMPER:
			// Don't trigger if the stage is ended/failed
			if (player->exiting)
				return;

			if (player->bumpertime <= (TICRATE/2)-5)
			{
				S_StartSound(toucher, special->info->seesound);
				if (player->powers[pw_carry] == CR_NIGHTSMODE)
				{
					player->bumpertime = TICRATE/2;
					if (special->threshold > 0)
						player->flyangle = (special->threshold*30)-1;
					else
						player->flyangle = special->threshold;

					player->speed = FixedMul(special->info->speed, special->scale);
					P_SetTarget(&player->mo->hnext, special); // Reference bumper for position correction on next tic
				}
				else // More like a spring
				{
					angle_t fa;
					fixed_t xspeed, yspeed;
					const fixed_t speed = FixedMul(FixedDiv(special->info->speed*FRACUNIT,75*FRACUNIT), FixedSqrt(FixedMul(toucher->scale,special->scale)));

					player->bumpertime = TICRATE/2;

					P_UnsetThingPosition(toucher);
					toucher->x = special->x;
					toucher->y = special->y;
					P_SetThingPosition(toucher);
					toucher->z = special->z+(special->height/4);

					if (special->threshold > 0)
						fa = (FixedAngle(((special->threshold*30)-1)*FRACUNIT)>>ANGLETOFINESHIFT) & FINEMASK;
					else
						fa = 0;

					xspeed = FixedMul(FINECOSINE(fa),speed);
					yspeed = FixedMul(FINESINE(fa),speed);

					P_InstaThrust(toucher, special->angle, xspeed/10);
					toucher->momz = yspeed/11;

					toucher->angle = special->angle;

					P_SetPlayerAngle(player, toucher->angle);

					P_ResetPlayer(player);

					P_SetPlayerMobjState(toucher, S_PLAY_FALL);
				}
			}
			return;
		case MT_NIGHTSSUPERLOOP:
			if (player->bot || !(player->powers[pw_carry] == CR_NIGHTSMODE))
				return;
			if (!G_IsSpecialStage(gamemap))
				player->powers[pw_nights_superloop] = (UINT16)special->info->speed;
			else
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
						players[i].powers[pw_nights_superloop] = (UINT16)special->info->speed;
				if (special->info->deathsound != sfx_None)
					S_StartSound(NULL, special->info->deathsound);
			}

			// CECHO showing you what this item is
			if (player == &players[displayplayer] || G_IsSpecialStage(gamemap))
			{
				HU_SetCEchoFlags(V_AUTOFADEOUT);
				HU_SetCEchoDuration(4);
				HU_DoCEcho(M_GetText("\\\\\\\\\\\\\\\\Super Paraloop"));
			}
			break;
		case MT_NIGHTSDRILLREFILL:
			if (player->bot || !(player->powers[pw_carry] == CR_NIGHTSMODE))
				return;
			if (!G_IsSpecialStage(gamemap))
				player->drillmeter = special->info->speed;
			else
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
						players[i].drillmeter = special->info->speed;
				if (special->info->deathsound != sfx_None)
					S_StartSound(NULL, special->info->deathsound);
			}

			// CECHO showing you what this item is
			if (player == &players[displayplayer] || G_IsSpecialStage(gamemap))
			{
				HU_SetCEchoFlags(V_AUTOFADEOUT);
				HU_SetCEchoDuration(4);
				HU_DoCEcho(M_GetText("\\\\\\\\\\\\\\\\Drill Refill"));
			}
			break;
		case MT_NIGHTSHELPER:
			if (player->bot || !(player->powers[pw_carry] == CR_NIGHTSMODE))
				return;
			if (!G_IsSpecialStage(gamemap))
			{
				// A flicky orbits us now
				mobj_t *flickyobj = P_SpawnMobj(toucher->x, toucher->y, toucher->z + toucher->info->height, MT_NIGHTOPIANHELPER);
				P_SetTarget(&flickyobj->target, toucher);

				player->powers[pw_nights_helper] = (UINT16)special->info->speed;
			}
			else
			{
				mobj_t *flickyobj;
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].mo && players[i].powers[pw_carry] == CR_NIGHTSMODE) {
						players[i].powers[pw_nights_helper] = (UINT16)special->info->speed;
						flickyobj = P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_NIGHTOPIANHELPER);
						P_SetTarget(&flickyobj->target, players[i].mo);
					}
				if (special->info->deathsound != sfx_None)
					S_StartSound(NULL, special->info->deathsound);
			}

			// CECHO showing you what this item is
			if (player == &players[displayplayer] || G_IsSpecialStage(gamemap))
			{
				HU_SetCEchoFlags(V_AUTOFADEOUT);
				HU_SetCEchoDuration(4);
				HU_DoCEcho(M_GetText("\\\\\\\\\\\\\\\\Nightopian Helper"));
			}
			break;
		case MT_NIGHTSEXTRATIME:
			if (player->bot || !(player->powers[pw_carry] == CR_NIGHTSMODE))
				return;
			if (!G_IsSpecialStage(gamemap))
			{
				player->nightstime += special->info->speed;
				player->startedtime += special->info->speed;
				player->lapstartedtime += special->info->speed;
				P_RestoreMusic(player);
			}
			else
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
					{
						players[i].nightstime += special->info->speed;
						players[i].startedtime += special->info->speed;
						players[i].lapstartedtime += special->info->speed;
						P_RestoreMusic(&players[i]);
					}
				if (special->info->deathsound != sfx_None)
					S_StartSound(NULL, special->info->deathsound);
			}

			// CECHO showing you what this item is
			if (player == &players[displayplayer] || G_IsSpecialStage(gamemap))
			{
				HU_SetCEchoFlags(V_AUTOFADEOUT);
				HU_SetCEchoDuration(4);
				HU_DoCEcho(M_GetText("\\\\\\\\\\\\\\\\Extra Time"));
			}
			break;
		case MT_NIGHTSLINKFREEZE:
			if (player->bot || !(player->powers[pw_carry] == CR_NIGHTSMODE))
				return;
			if (!G_IsSpecialStage(gamemap))
			{
				player->powers[pw_nights_linkfreeze] = (UINT16)special->info->speed;
				player->linktimer = nightslinktics;
			}
			else
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
					{
						players[i].powers[pw_nights_linkfreeze] += (UINT16)special->info->speed;
						players[i].linktimer = nightslinktics;
					}
				if (special->info->deathsound != sfx_None)
					S_StartSound(NULL, special->info->deathsound);
			}

			// CECHO showing you what this item is
			if (player == &players[displayplayer] || G_IsSpecialStage(gamemap))
			{
				HU_SetCEchoFlags(V_AUTOFADEOUT);
				HU_SetCEchoDuration(4);
				HU_DoCEcho(M_GetText("\\\\\\\\\\\\\\\\Link Freeze"));
			}
			break;
		case MT_HOOPCOLLIDE:
			// This produces a kind of 'domino effect' with the hoop's pieces.
			for (; special->hprev != NULL; special = special->hprev); // Move to the first sprite in the hoop
			i = 0;
			for (; special->type == MT_HOOP; special = special->hnext)
			{
				special->fuse = 11;
				special->movedir = i;
				special->extravalue1 = special->target->extravalue1;
				special->extravalue2 = special->target->extravalue2;
				special->target->threshold = 4242;
				i++;
			}
			// Make the collision detectors disappear.
			{
				mobj_t *hnext;
				for (; special != NULL; special = hnext)
				{
					hnext = special->hnext;
					P_RemoveMobj(special);
				}
			}

			P_DoNightsScore(player);

			// Hoops are the only things that should add to the drill meter
			// Also, one tic's worth of drill is too much.
			if (G_IsSpecialStage(gamemap))
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE)
						players[i].drillmeter += TICRATE/2;
			}
			else if (player->bot)
				players[consoleplayer].drillmeter += TICRATE/2;
			else
				player->drillmeter += TICRATE/2;

			// Play hoop sound -- pick one depending on the current link.
			if (player->linkcount <= 5)
				S_StartSound(toucher, sfx_hoop1);
			else if (player->linkcount <= 10)
				S_StartSound(toucher, sfx_hoop2);
			else
				S_StartSound(toucher, sfx_hoop3);
			return;

// ***** //
// Mario //
// ***** //
		case MT_SHELL:
			{
				boolean bounceon = ((P_MobjFlip(toucher)*(toucher->z - (special->z + special->height/2)) > 0) && (P_MobjFlip(toucher)*toucher->momz < 0));
				if (special->threshold == TICRATE) // it's moving
				{
					if (bounceon)
					{
						// Stop it!
						special->momx = special->momy = 0;
						S_StartSound(toucher, sfx_mario2);
						P_SetTarget(&special->target, NULL);
						special->threshold = TICRATE - 1;
						toucher->momz = -toucher->momz;
					}
					else // can't handle in PIT_CheckThing because of landing-on causing it to stop
						P_DamageMobj(toucher, special, special->target, 1, 0);
				}
				else if (special->threshold == 0)
				{
					// Kick that sucker around!
					special->movedir = ((special->movedir == 1) ? -1 : 1);
					P_InstaThrust(special, toucher->angle, (special->info->speed*special->scale));
					S_StartSound(toucher, sfx_mario2);
					P_SetTarget(&special->target, toucher);
					special->threshold = (3*TICRATE)/2;
					if (bounceon)
						toucher->momz = -toucher->momz;
				}
			}
			return;
		case MT_AXE:
			{
				line_t junk;
				thinker_t  *th;
				mobj_t *mo2;

				if (player->bot)
					return;

				junk.tag = LE_AXE;
				EV_DoElevator(&junk, bridgeFall, false);

				// scan the remaining thinkers to find koopa
				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
						continue;

					mo2 = (mobj_t *)th;

					if (mo2->type != MT_KOOPA)
						continue;

					mo2->momz = 5*FRACUNIT;
					break;
				}
			}
			break;
		case MT_LETTER:
		{
			if (special->health && !player->bot)
			{
				F_StartTextPrompt(199, 0, toucher, 0, true, false);
				special->health = 0;
				if (ultimatemode && player->continues < 99)
					player->continues++;
			}
			return;
		}
		case MT_FIREFLOWER:
			if (player->bot)
				return;

			S_StartSound(toucher, sfx_mario3);

			player->powers[pw_shield] = (player->powers[pw_shield] & SH_NOSTACK)|SH_FIREFLOWER;

			if (!(player->powers[pw_super] || (mariomode && player->powers[pw_invulnerability])))
			{
				player->mo->color = SKINCOLOR_WHITE;
				G_GhostAddColor(GHC_FIREFLOWER);
			}

			break;

// *************** //
// Misc touchables //
// *************** //
>>>>>>> srb2/next
		case MT_STARPOST:
			P_TouchStarPost(special, player, special->spawnpoint && (special->spawnpoint->options & MTF_OBJECTSPECIAL));
			return;

<<<<<<< HEAD
=======
		case MT_FAKEMOBILE:
			{
				fixed_t touchx, touchy, touchspeed;
				angle_t angle;

				if (P_AproxDistance(toucher->x-special->x, toucher->y-special->y) >
					P_AproxDistance((toucher->x-toucher->momx)-special->x, (toucher->y-toucher->momy)-special->y))
				{
					touchx = toucher->x + toucher->momx;
					touchy = toucher->y + toucher->momy;
				}
				else
				{
					touchx = toucher->x;
					touchy = toucher->y;
				}

				angle = R_PointToAngle2(special->x, special->y, touchx, touchy);
				touchspeed = P_AproxDistance(toucher->momx, toucher->momy);

				toucher->momx = P_ReturnThrustX(special, angle, touchspeed);
				toucher->momy = P_ReturnThrustY(special, angle, touchspeed);
				toucher->momz = -toucher->momz;
				if (player->pflags & PF_GLIDING && !P_IsObjectOnGround(toucher))
				{
					player->pflags &= ~(PF_GLIDING|PF_JUMPED|PF_NOJUMPDAMAGE);
					P_SetPlayerMobjState(toucher, S_PLAY_FALL);
					toucher->momz += P_MobjFlip(toucher) * (player->speed >> 3);
					toucher->momx = 7*toucher->momx>>3;
					toucher->momy = 7*toucher->momy>>3;
				}
				player->homing = 0;

				// Play a bounce sound?
				S_StartSound(toucher, special->info->painsound);
			}
			return;

		case MT_BLACKEGGMAN_GOOPFIRE:
			if (!player->powers[pw_flashing] && !(player->powers[pw_ignorelatch] & (1<<15)))
			{
				toucher->momx = 0;
				toucher->momy = 0;

				if (toucher->momz != 0)
					special->momz = toucher->momz;

				player->powers[pw_carry] = CR_BRAKGOOP;
				P_SetTarget(&toucher->tracer, special);

				P_ResetPlayer(player);

				if (special->target && special->target->state == &states[S_BLACKEGG_SHOOT1])
				{
					if (special->target->health <= 2 && P_RandomChance(FRACUNIT/2))
						P_SetMobjState(special->target, special->target->info->missilestate);
					else
						P_SetMobjState(special->target, special->target->info->raisestate);
				}
			}
			return;
		case MT_EGGSHIELD:
			{
				angle_t angle = R_PointToAngle2(special->x, special->y, toucher->x, toucher->y) - special->angle;
				fixed_t touchspeed = P_AproxDistance(toucher->momx, toucher->momy);
				if (touchspeed < special->scale)
					touchspeed = special->scale;

				// Blocked by the shield?
				if (!(angle > ANGLE_90 && angle < ANGLE_270))
				{
					toucher->momx = P_ReturnThrustX(special, special->angle, touchspeed);
					toucher->momy = P_ReturnThrustY(special, special->angle, touchspeed);
					toucher->momz = -toucher->momz;
					if (player->pflags & PF_GLIDING && !P_IsObjectOnGround(toucher))
					{
						player->pflags &= ~(PF_GLIDING|PF_JUMPED|PF_NOJUMPDAMAGE);
						P_SetPlayerMobjState(toucher, S_PLAY_FALL);
						toucher->momz += P_MobjFlip(toucher) * (player->speed >> 3);
						toucher->momx = 7*toucher->momx>>3;
						toucher->momy = 7*toucher->momy>>3;
					}
					player->homing = 0;

					// Play a bounce sound?
					S_StartSound(toucher, special->info->painsound);

					// experimental bounce
					if (special->target)
						special->target->extravalue1 = -special->target->info->speed;
				}
				else
				{
					// Shatter the shield!
					toucher->momx = -toucher->momx/2;
					toucher->momy = -toucher->momy/2;
					toucher->momz = -toucher->momz;
					break;
				}
			}
			return;

		case MT_EGGROBO1:
			if (special->state == &states[special->info->deathstate])
				return;
			if (P_PlayerInPain(player))
				return;

			P_SetMobjState(special, special->info->meleestate);
			special->angle = special->movedir;
			special->momx = special->momy = 0;

			// Buenos Dias Mandy
			P_SetPlayerMobjState(toucher, S_PLAY_STUN);
			player->pflags &= ~PF_APPLYAUTOBRAKE;
			P_ResetPlayer(player);
			player->drawangle = special->angle + ANGLE_180;
			P_InstaThrust(toucher, special->angle, FixedMul(3*special->info->speed, special->scale/2));
			toucher->z += P_MobjFlip(toucher);
			if (toucher->eflags & MFE_UNDERWATER) // unlikely.
				P_SetObjectMomZ(toucher, FixedDiv(10511*FRACUNIT,2600*FRACUNIT), false);
			else
				P_SetObjectMomZ(toucher, FixedDiv(69*FRACUNIT,10*FRACUNIT), false);
			if (P_IsLocalPlayer(player))
			{
				quake.intensity = 9*FRACUNIT;
				quake.time = TICRATE/2;
				quake.epicenter = NULL;
			}

#if 0 // camera redirection - deemed unnecessary
			toucher->angle = special->angle;
			P_SetPlayerAngle(player, toucher->angle);
#endif

			S_StartSound(toucher, special->info->attacksound); // home run

			return;

>>>>>>> srb2/next
		case MT_BIGTUMBLEWEED:
		case MT_LITTLETUMBLEWEED:
			if (toucher->momx || toucher->momy)
			{
				special->momx = toucher->momx;
				special->momy = toucher->momy;
				special->momz = P_AproxDistance(toucher->momx, toucher->momy)/4;
<<<<<<< HEAD
=======

				if (toucher->momz > 0)
					special->momz += toucher->momz/8;

				P_SetMobjState(special, special->info->seestate);
			}
			return;
		case MT_SMALLGRABCHAIN:
		case MT_BIGGRABCHAIN:
			{
				boolean macespin = false;
				if (P_MobjFlip(toucher)*toucher->momz > 0
					|| (player->powers[pw_carry]))
					return;

				if (toucher->z > special->z + special->height/2)
					return;

				if (toucher->z + toucher->height/2 < special->z)
					return;

				if (player->powers[pw_flashing])
					return;

				if (special->tracer && !(special->tracer->flags2 & MF2_STRONGBOX))
					macespin = true;
				
				if (macespin ? (player->powers[pw_ignorelatch] & (1<<15)) : (player->powers[pw_ignorelatch]))
					return;

				if (special->movefactor && special->tracer && special->tracer->angle != ANGLE_90 && special->tracer->angle != ANGLE_270)
				{ // I don't expect you to understand this, Mr Bond...
					angle_t ang = R_PointToAngle2(special->x, special->y, toucher->x, toucher->y) - special->tracer->angle;
					if ((special->movefactor > 0) == (special->tracer->angle > ANGLE_90 && special->tracer->angle < ANGLE_270))
						ang += ANGLE_180;
					if (ang < ANGLE_180)
						return; // I expect you to die.
				}

				P_ResetPlayer(player);
				P_SetTarget(&toucher->tracer, special);

				if (macespin)
				{
					player->powers[pw_carry] = CR_MACESPIN;
					S_StartSound(toucher, sfx_spin);
					P_SetPlayerMobjState(toucher, S_PLAY_ROLL);
				}
				else
					player->powers[pw_carry] = CR_GENERIC;

				// Can't jump first frame
				player->pflags |= PF_JUMPSTASIS;

				return;
			}
		case MT_EGGMOBILE2_POGO:
			// sanity checks
			if (!special->target || !special->target->health)
				return;
			// Goomba Stomp'd!
			if (special->target->momz < 0)
			{
				P_DamageMobj(toucher, special, special->target, 1, 0);
				//special->target->momz = -special->target->momz;
				special->target->momx = special->target->momy = 0;
				special->target->momz = 0;
				special->target->flags |= MF_NOGRAVITY;
				P_SetMobjState(special->target, special->info->raisestate);
				S_StartSound(special->target, special->info->activesound);
				P_RemoveMobj(special);
			}
			return;

		case MT_EXTRALARGEBUBBLE:
			if (player->powers[pw_shield] & SH_PROTECTWATER)
				return;
			if (maptol & TOL_NIGHTS)
				return;
			if (mariomode)
				return;
			if (special->state-states != S_EXTRALARGEBUBBLE)
				return; // Don't grab the bubble during its spawn animation
			else if (toucher->eflags & MFE_VERTICALFLIP)
			{
				if (special->z+special->height < toucher->z
					|| special->z+special->height > toucher->z + (toucher->height*2/3))
					return; // Only go in the mouth
			}
			else if (special->z < toucher->z
				|| special->z > toucher->z + (toucher->height*2/3))
				return; // Only go in the mouth

			// Eaten by player!
			if ((!player->bot) && (player->powers[pw_underwater] && player->powers[pw_underwater] <= 12*TICRATE + 1))
			{
				player->powers[pw_underwater] = underwatertics + 1;
				P_RestoreMusic(player);
			}

			if (player->powers[pw_underwater] < underwatertics + 1)
				player->powers[pw_underwater] = underwatertics + 1;

			if (!player->climbing)
			{
				if (player->bot && toucher->state-states != S_PLAY_GASP)
					S_StartSound(toucher, special->info->deathsound); // Force it to play a sound for bots
				P_SetPlayerMobjState(toucher, S_PLAY_GASP);
				P_ResetPlayer(player);
			}
>>>>>>> srb2/next

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

<<<<<<< HEAD
=======
		case MT_CANARIVORE_GAS:
			// if player and gas touch, attach gas to player (overriding any gas that already attached) and apply slowdown effect
			special->flags |= MF_NOGRAVITY|MF_NOCLIPHEIGHT;
			P_UnsetThingPosition(special);
			special->x = toucher->x - toucher->momx/2;
			special->y = toucher->y - toucher->momy/2;
			special->z = toucher->z - toucher->momz/2;
			P_SetThingPosition(special);
			toucher->momx = FixedMul(toucher->momx, 50*FRACUNIT/51);
			toucher->momy = FixedMul(toucher->momy, 50*FRACUNIT/51);
			special->momx = toucher->momx;
			special->momy = toucher->momy;
			special->momz = toucher->momz;
			return;

		case MT_MINECARTSPAWNER:
			if (!player->bot && special->fuse <= TICRATE && player->powers[pw_carry] != CR_MINECART && !(player->powers[pw_ignorelatch] & (1<<15)))
			{
				mobj_t *mcart = P_SpawnMobj(special->x, special->y, special->z, MT_MINECART);
				P_SetTarget(&mcart->target, toucher);
				mcart->angle = toucher->angle = player->drawangle = special->angle;
				mcart->friction = FRACUNIT;

				P_ResetPlayer(player);
				player->pflags |= PF_JUMPDOWN;
				player->powers[pw_carry] = CR_MINECART;
				player->pflags &= ~PF_APPLYAUTOBRAKE;
				P_SetTarget(&toucher->tracer, mcart);
				toucher->momx = toucher->momy = toucher->momz = 0;

				special->fuse = 3*TICRATE;
				special->flags2 |= MF2_DONTDRAW;
			}
			return;

		case MT_MINECARTEND:
			if (player->powers[pw_carry] == CR_MINECART && toucher->tracer && !P_MobjWasRemoved(toucher->tracer) && toucher->tracer->health)
			{
				fixed_t maxz = max(toucher->z, special->z + 35*special->scale);

				toucher->momx = toucher->tracer->momx/2;
				toucher->momy = toucher->tracer->momy/2;
				toucher->momz = toucher->tracer->momz + P_AproxDistance(toucher->tracer->momx, toucher->tracer->momy)/2;
				P_ResetPlayer(player);
				player->pflags &= ~PF_APPLYAUTOBRAKE;
				P_SetPlayerMobjState(toucher, S_PLAY_FALL);
				P_SetTarget(&toucher->tracer->target, NULL);
				P_KillMobj(toucher->tracer, toucher, special, 0);
				P_SetTarget(&toucher->tracer, NULL);
				player->powers[pw_carry] = CR_NONE;
				P_UnsetThingPosition(toucher);
				toucher->x = special->x;
				toucher->y = special->y;
				toucher->z = maxz;
				P_SetThingPosition(toucher);
			}
			return;

		case MT_MINECARTSWITCHPOINT:
			if (player->powers[pw_carry] == CR_MINECART && toucher->tracer && !P_MobjWasRemoved(toucher->tracer) && toucher->tracer->health)
			{
				mobjflag2_t destambush = special->flags2 & MF2_AMBUSH;
				angle_t angdiff = toucher->tracer->angle - special->angle;
				if (angdiff > ANGLE_90 && angdiff < ANGLE_270)
					destambush ^= MF2_AMBUSH;
				toucher->tracer->flags2 = (toucher->tracer->flags2 & ~MF2_AMBUSH) | destambush;
			}
			return;
>>>>>>> srb2/next
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

	// Save the player's time and position.
	player->starposttime = leveltime;
	player->starpostx = checkbase->x>>FRACBITS;
	player->starposty = checkbase->y>>FRACBITS;
	player->starpostz = post->z>>FRACBITS;
	player->starpostangle = post->angle;
	player->starpostscale = player->mo->destscale;
	if (post->flags2 & MF2_OBJECTFLIP)
	{
		player->starpostscale *= -1;
		player->starpostz += post->height>>FRACBITS;
	}
	player->starpostnum = post->health;
	//S_StartSound(toucher, post->info->painsound);

	P_ClearStarPost(post->health);
}

/** Prints death messages relating to a dying or hit player.
  *
  * \param player    Affected player.
  * \param inflictor The attack weapon used, can be NULL.
  * \param source    The attacker, can be NULL.
  * \param damagetype The type of damage dealt to the player. If bit 7 (0x80) is set, this was an instant-kill.
  */
static void P_HitDeathMessages(player_t *player, mobj_t *inflictor, mobj_t *source, UINT8 damagetype)
{
	const char *str = NULL;
	boolean deathonly = false;
	boolean deadsource = false;
	boolean deadtarget = false;
	// player names complete with control codes
	char targetname[MAXPLAYERNAME+4];
	char sourcename[MAXPLAYERNAME+4];

	if (!(gametyperules & (GTR_RINGSLINGER|GTR_HURTMESSAGES)))
		return; // Not in coop, etc.

	if (!player)
		return; // Impossible!

	if (!player->mo)
		return; // Also impossible!

	if (player->spectator)
		return; // No messages for dying (crushed) spectators.

	if (!netgame)
		return; // Presumably it's obvious what's happening in splitscreen.

	if (LUAh_HurtMsg(player, inflictor, source, damagetype))
		return;

	deadtarget = (player->mo->health <= 0);

	// Don't log every hazard hit if they don't want us to.
	if (!deadtarget && !cv_hazardlog.value)
		return;

	// Target's name
	snprintf(targetname, sizeof(targetname), "%s%s%s",
	         CTFTEAMCODE(player),
	         player_names[player - players],
	         CTFTEAMENDCODE(player));

	if (source)
	{
		// inflictor shouldn't be NULL if source isn't
		I_Assert(inflictor != NULL);

		if (source->player)
		{
			// Source's name (now that we know there is one)
			snprintf(sourcename, sizeof(sourcename), "%s%s%s",
					 CTFTEAMCODE(source->player),
					 player_names[source->player - players],
					 CTFTEAMENDCODE(source->player));

			// We don't care if it's us.
			// "Player 1's [redacted] killed Player 1."
			if (source->player->playerstate == PST_DEAD && source->player != player &&
			 (inflictor->flags2 & MF2_BEYONDTHEGRAVE))
				deadsource = true;

			if (inflictor->flags & MF_PUSHABLE)
			{
				str = M_GetText("%s%s's playtime with heavy objects %s %s.\n");
			}
			else switch (inflictor->type)
			{
				case MT_PLAYER:
					if (damagetype == DMG_NUKE) // SH_ARMAGEDDON, armageddon shield
						str = M_GetText("%s%s's armageddon blast %s %s.\n");
					else if ((inflictor->player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL && (inflictor->player->pflags & PF_SHIELDABILITY))
						str = M_GetText("%s%s's elemental stomp %s %s.\n");
					else if (inflictor->player->powers[pw_invulnerability])
						str = M_GetText("%s%s's invincibility aura %s %s.\n");
					else if (inflictor->player->powers[pw_super])
						str = M_GetText("%s%s's super aura %s %s.\n");
					else
						str = M_GetText("%s%s's tagging hand %s %s.\n");
					break;
				case MT_SPINFIRE:
					str = M_GetText("%s%s's elemental fire trail %s %s.\n");
					break;
				case MT_THROWNBOUNCE:
					str = M_GetText("%s%s's bounce ring %s %s.\n");
					break;
				case MT_THROWNINFINITY:
					str = M_GetText("%s%s's infinity ring %s %s.\n");
					break;
				case MT_THROWNAUTOMATIC:
					str = M_GetText("%s%s's automatic ring %s %s.\n");
					break;
				case MT_THROWNSCATTER:
					str = M_GetText("%s%s's scatter ring %s %s.\n");
					break;
				// TODO: For next two, figure out how to determine if it was a direct hit or splash damage. -SH
				case MT_THROWNEXPLOSION:
					str = M_GetText("%s%s's explosion ring %s %s.\n");
					break;
				case MT_THROWNGRENADE:
					str = M_GetText("%s%s's grenade ring %s %s.\n");
					break;
				case MT_REDRING:
					if (inflictor->flags2 & MF2_RAILRING)
						str = M_GetText("%s%s's rail ring %s %s.\n");
					else
						str = M_GetText("%s%s's thrown ring %s %s.\n");
					break;
				default:
					str = M_GetText("%s%s %s %s.\n");
					break;
			}

			CONS_Printf(str,
				deadsource ? M_GetText("The late ") : "",
				sourcename,
				deadtarget ? M_GetText("killed") : M_GetText("hit"),
				targetname);
			return;
		}
		else switch (source->type)
		{
			case MT_EGGMAN_ICON:
				str = M_GetText("%s was %s by Eggman's nefarious TV magic.\n");
				break;
			case MT_SPIKE:
			case MT_WALLSPIKE:
				str = M_GetText("%s was %s by spikes.\n");
				break;
			default:
				str = M_GetText("%s was %s by an environmental hazard.\n");
				break;
		}
	}
	else
	{
		// null source, environment kills
		switch (damagetype)
		{
			case DMG_WATER:
				str = M_GetText("%s was %s by dangerous water.\n");
				break;
			case DMG_FIRE:
				str = M_GetText("%s was %s by molten lava.\n");
				break;
			case DMG_ELECTRIC:
				str = M_GetText("%s was %s by electricity.\n");
				break;
			case DMG_SPIKE:
				str = M_GetText("%s was %s by spikes.\n");
				break;
			case DMG_DROWNED:
				deathonly = true;
				str = M_GetText("%s drowned.\n");
				break;
			case DMG_CRUSHED:
				deathonly = true;
				str = M_GetText("%s was crushed.\n");
				break;
			case DMG_DEATHPIT:
				if (deadtarget)
				{
					deathonly = true;
					str = M_GetText("%s fell into a bottomless pit.\n");
				}
				break;
			case DMG_SPACEDROWN:
				if (deadtarget)
				{
					deathonly = true;
					str = M_GetText("%s asphyxiated in space.\n");
				}
				break;
			default:
				if (deadtarget)
				{
					deathonly = true;
					str = M_GetText("%s died.\n");
				}
				break;
		}
		if (!str)
			str = M_GetText("%s was %s by an environmental hazard.\n");
	}

	if (!str) // Should not happen! Unless we missed catching something above.
		return;

	if (deathonly)
	{
		if (!deadtarget)
			return;
		CONS_Printf(str, targetname);
	}
	else
		CONS_Printf(str, targetname, deadtarget ? M_GetText("killed") : M_GetText("hit"));
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
	INT32 i, j, numplayersingame = 0, numexiting = 0;
	boolean griefed = false;

	// Check if all the players in the race have finished. If so, end the level.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator || players[i].exiting || players[i].bot || !players[i].lives)
			continue;

		break;
	}

	if (i == MAXPLAYERS) // finished
	{
<<<<<<< HEAD
		racecountdown = exitcountdown = 0;
		return true;
	}
=======
		// Exception for hide and seek. If a round has started and the IT player leaves, end the round.
		if ((gametyperules & GTR_HIDEFROZEN) && (leveltime >= (hidetime * TICRATE)))
		{
			CONS_Printf(M_GetText("The IT player has left the game.\n"));
			if (server)
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
>>>>>>> srb2/next

	for (j = 0; j < MAXPLAYERS; j++)
	{
		if (nospectategrief[j] != -1) // prevent spectate griefing
			griefed = true;
		if (!playeringame[j] || players[j].spectator)
			continue;
		numplayersingame++;
		if (players[j].exiting)
			numexiting++;
	}

	if (cv_karteliminatelast.value && numplayersingame > 1 && !griefed)
	{
		// check if we just got unlucky and there was only one guy who was a problem
		for (j = i+1; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j] || players[j].spectator || players[j].exiting || !players[j].lives)
				continue;
			break;
		}

		if (j == MAXPLAYERS) // finish anyways, force a time over
		{
			P_DoTimeOver(&players[i]);
			racecountdown = exitcountdown = 0;
			return true;
		}
	}

	if (!racecountdown) // Check to see if the winners have finished, to set countdown.
	{
		UINT8 winningpos = 1;

		winningpos = max(1, numplayersingame/2);
		if (numplayersingame % 2) // any remainder?
			winningpos++;

		if (numexiting >= winningpos)
			racecountdown = (((netgame || multiplayer) ? cv_countdowntime.value : 30)*TICRATE) + 1; // 30 seconds to finish, get going!
	}

	if (numplayersingame < 2) // reset nospectategrief in free play
	{
		for (j = 0; j < MAXPLAYERS; j++)
			nospectategrief[j] = -1;
	}

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
		target->flags2 &= ~MF2_DONTDRAW;
	}

	// if killed by a player
	if (source && source->player)
	{
		if (target->flags & MF_MONITOR || target->type == MT_RANDOMITEM)
		{
			P_SetTarget(&target->target, source);
			source->player->numboxes++;

			if (cv_itemrespawn.value && (netgame || multiplayer))
			{
				target->fuse = cv_itemrespawntime.value*TICRATE + 2; // Random box generation
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

<<<<<<< HEAD
		if (!target->player->bot && !G_IsSpecialStage(gamemap) && G_GametypeUsesLives())
=======
		if (target->player->powers[pw_super])
		{
			target->player->powers[pw_super] = 0;
			if (P_IsLocalPlayer(target->player))
			{
				music_stack_noposition = true; // HACK: Do not reposition next music
				music_stack_fadeout = MUSICRATE/2; // HACK: Fade out current music
			}
			P_RestoreMusic(target->player);

			if (!G_CoopGametype())
			{
				HU_SetCEchoFlags(0);
				HU_SetCEchoDuration(5);
				HU_DoCEcho(va("%s\\is no longer super.\\\\\\\\", player_names[target->player-players]));
			}
		}

		target->color = target->player->skincolor;
		target->colorized = false;
		G_GhostAddColor(GHC_NORMAL);

		if ((target->player->lives <= 1) && (netgame || multiplayer) && G_GametypeUsesCoopLives() && (cv_cooplives.value == 0))
			;
		else if (!target->player->bot && !target->player->spectator && (target->player->lives != INFLIVES)
		 && G_GametypeUsesLives())
>>>>>>> srb2/next
		{
			if (!(target->player->pflags & PF_FINISHED))
				target->player->lives -= 1; // Lose a life Tails 03-11-2000

			if (target->player->lives <= 0) // Tails 03-14-2000
			{
				boolean gameovermus = false;
				if ((netgame || multiplayer) && G_GametypeUsesCoopLives() && (cv_cooplives.value != 1))
				{
					INT32 i;
					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;

						if (players[i].lives > 0)
							break;
					}
					if (i == MAXPLAYERS)
						gameovermus = true;
				}
				else if (P_IsLocalPlayer(target->player))
					gameovermus = true;

				if (gameovermus) // Yousa dead now, Okieday? Tails 03-14-2000
					S_ChangeMusicEx("_gover", 0, 0, 0, (2*MUSICRATE) - (MUSICRATE/25), 0); // 1.96 seconds
					//P_PlayJingle(target->player, JT_GOVER); // can't be used because incompatible with track fadeout

				if (!(netgame || multiplayer || demoplayback || demorecording || metalrecording || modeattacking) && numgameovers < maxgameovers)
				{
					numgameovers++;
					if ((!modifiedgame || savemoddata) && cursaveslot > 0)
						G_SaveGameOver((UINT32)cursaveslot, (target->player->continues <= 0));
				}
			}
		}

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

<<<<<<< HEAD
		if (target->player == &players[displayplayers[1]]) localaiming[1] = 0;
		if (target->player == &players[displayplayers[2]]) localaiming[2] = 0;
		if (target->player == &players[displayplayers[3]]) localaiming[3] = 0;
=======
		//tag deaths handled differently in suicide cases. Don't count spectators!
		if (G_TagGametype()
		 && !(target->player->pflags & PF_TAGIT) && (!source || !source->player) && !(target->player->spectator))
		{
			// if you accidentally die before you run out of time to hide, ignore it.
			// allow them to try again, rather than sitting the whole thing out.
			if (leveltime >= hidetime * TICRATE)
			{
				if (!(gametyperules & GTR_HIDEFROZEN))//suiciding in survivor makes you IT.
				{
					target->player->pflags |= PF_TAGIT;
					CONS_Printf(M_GetText("%s is now IT!\n"), player_names[target->player-players]); // Tell everyone who is it!
					P_CheckSurvivors();
				}
				else
				{
					if (!(target->player->pflags & PF_GAMETYPEOVER))
					{
						//otherwise, increment the tagger's score.
						//in hide and seek, suiciding players are counted as found.
						INT32 w;
>>>>>>> srb2/next

		if (G_BattleGametype())
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

				if (target->player && target->player->pflags & PF_TIMEOVER)
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

<<<<<<< HEAD
=======
static void P_NiGHTSDamage(mobj_t *target, mobj_t *source)
{
	player_t *player = target->player;
	tic_t oldnightstime = player->nightstime;

	(void)source; // unused

	if (!player->powers[pw_flashing])
	{
		angle_t fa;

		player->angle_pos = player->old_angle_pos;
		player->speed /= 5;
		player->flyangle += 180; // Shuffle's BETTERNIGHTSMOVEMENT?
		player->flyangle %= 360;

		if (gametyperules & GTR_RACE)
			player->drillmeter -= 5*20;
		else
		{
			if (player->nightstime > 5*TICRATE)
				player->nightstime -= 5*TICRATE;
			else
				player->nightstime = 1;
		}

		if (player->pflags & PF_TRANSFERTOCLOSEST)
		{
			target->momx = -target->momx;
			target->momy = -target->momy;
		}
		else
		{
			fa = player->old_angle_pos>>ANGLETOFINESHIFT;

			target->momx = FixedMul(FINECOSINE(fa),target->target->radius);
			target->momy = FixedMul(FINESINE(fa),target->target->radius);
		}

		player->powers[pw_flashing] = flashingtics;
		P_SetPlayerMobjState(target, S_PLAY_NIGHTS_STUN);
		S_StartSound(target, sfx_nghurt);

		player->mo->rollangle = 0;

		if (oldnightstime > 10*TICRATE
			&& player->nightstime < 10*TICRATE)
		{
			if ((mapheaderinfo[gamemap-1]->levelflags & LF_MIXNIGHTSCOUNTDOWN)
#ifdef _WIN32
				// win32 MIDI volume hack means we cannot fade down the music
				&& S_MusicType() != MU_MID
#endif
			)
			{
				S_FadeMusic(0, 10*MUSICRATE);
				S_StartSound(NULL, sfx_timeup); // that creepy "out of time" music from NiGHTS.
			}
			else
				P_PlayJingle(player, ((maptol & TOL_NIGHTS) && !G_IsSpecialStage(gamemap)) ? JT_NIGHTSTIMEOUT : JT_SSTIMEOUT);
		}
	}
}

static boolean P_TagDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	player_t *player = target->player;
	(void)damage; //unused parm

	// If flashing or invulnerable, ignore the tag,
	if (player->powers[pw_flashing] || player->powers[pw_invulnerability])
		return false;

	// Don't allow any damage before the round starts.
	if (leveltime <= hidetime * TICRATE)
		return false;

	// Ignore IT players shooting each other, unless friendlyfire is on.
	if ((player->pflags & PF_TAGIT && !((cv_friendlyfire.value || (gametyperules & GTR_FRIENDLYFIRE) || (damagetype & DMG_CANHURTSELF)) &&
		source && source->player && source->player->pflags & PF_TAGIT)))
	{
		if (inflictor->type == MT_LHRT && !(player->powers[pw_shield] & SH_NOSTACK))
		{
			if (player->revitem != MT_LHRT && player->spinitem != MT_LHRT && player->thokitem != MT_LHRT) // Healers do not get to heal other healers.
			{
				P_SwitchShield(player, SH_PINK);
				S_StartSound(target, mobjinfo[MT_PITY_ICON].seesound);
			}
		}
		return false;
	}

	// Don't allow players on the same team to hurt one another,
	// unless cv_friendlyfire is on.
	if (!(cv_friendlyfire.value || (gametyperules & GTR_FRIENDLYFIRE) || (damagetype & DMG_CANHURTSELF)) && (player->pflags & PF_TAGIT) == (source->player->pflags & PF_TAGIT))
	{
		if (inflictor->type == MT_LHRT && !(player->powers[pw_shield] & SH_NOSTACK))
		{
			if (player->revitem != MT_LHRT && player->spinitem != MT_LHRT && player->thokitem != MT_LHRT) // Healers do not get to heal other healers.
			{
				P_SwitchShield(player, SH_PINK);
				S_StartSound(target, mobjinfo[MT_PITY_ICON].seesound);
			}
		}
		else if (!(inflictor->flags & MF_FIRE))
			P_GivePlayerRings(player, 1);
		if (inflictor->flags2 & MF2_BOUNCERING)
			inflictor->fuse = 0; // bounce ring disappears at -1 not 0
		return false;
	}

	if (inflictor->type == MT_LHRT)
		return false;

	// The tag occurs so long as you aren't shooting another tagger with friendlyfire on.
	if (source->player->pflags & PF_TAGIT && !(player->pflags & PF_TAGIT))
	{
		P_AddPlayerScore(source->player, 100); //award points to tagger.
		P_HitDeathMessages(player, inflictor, source, 0);

		if (!(gametyperules & GTR_HIDEFROZEN)) //survivor
		{
			player->pflags |= PF_TAGIT; //in survivor, the player becomes IT and helps hunt down the survivors.
			CONS_Printf(M_GetText("%s is now IT!\n"), player_names[player-players]); // Tell everyone who is it!
		}
		else
		{
			player->pflags |= PF_GAMETYPEOVER; //in hide and seek, the player is tagged and stays stationary.
			CONS_Printf(M_GetText("%s was found!\n"), player_names[player-players]); // Tell everyone who is it!
		}

		//checks if tagger has tagged all players, if so, end round early.
		P_CheckSurvivors();
	}

	P_DoPlayerPain(player, source, inflictor);

	// Check for a shield
	if (player->powers[pw_shield])
	{
		P_RemoveShield(player);
		S_StartSound(target, sfx_shldls);
		return true;
	}

	if (player->powers[pw_carry] == CR_NIGHTSFALL)
	{
		if (player->spheres > 0)
		{
			P_PlayRinglossSound(target);
			P_PlayerRingBurst(player, player->spheres);
			player->spheres = 0;
		}
	}
	else if (player->rings > 0) // Ring loss
	{
		P_PlayRinglossSound(target);
		P_PlayerRingBurst(player, player->rings);
		player->rings = 0;
	}
	else // Death
	{
		P_PlayDeathSound(target);
		P_PlayVictorySound(source); // Killer laughs at you! LAUGHS! BWAHAHAHHAHAA!!
	}
	return true;
}

>>>>>>> srb2/next
static boolean P_PlayerHitsPlayer(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	player_t *player = target->player;

	// SRB2Kart: We want to hurt ourselves
	if (damagetype & DMG_CANTHURTSELF)
	{
		// You can't kill yourself, idiot...
		if (source == target)
			return false;

		// In COOP/RACE, you can't hurt other players unless cv_friendlyfire is on
		if (!(cv_friendlyfire.value || (gametyperules & GTR_FRIENDLYFIRE)) && (gametyperules & GTR_FRIENDLY))
		{
			if ((gametyperules & GTR_FRIENDLY) && inflictor->type == MT_LHRT && !(player->powers[pw_shield] & SH_NOSTACK)) // co-op only
			{
				if (player->revitem != MT_LHRT && player->spinitem != MT_LHRT && player->thokitem != MT_LHRT) // Healers do not get to heal other healers.
				{
					P_SwitchShield(player, SH_PINK);
					S_StartSound(target, mobjinfo[MT_PITY_ICON].seesound);
				}
			}
			return false;
		}
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

<<<<<<< HEAD
	if (player->pflags & PF_TIMEOVER)
=======
	// If the player was super, tell them he/she ain't so super nomore.
	if (!G_CoopGametype() && player->powers[pw_super])
>>>>>>> srb2/next
	{
		mobj_t *boom;
		player->mo->flags |= (MF_NOGRAVITY|MF_NOCLIP);
		player->mo->flags2 |= MF2_DONTDRAW;
		boom = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_FZEROBOOM);
		boom->scale = player->mo->scale;
		boom->angle = player->mo->angle;
		P_SetTarget(&boom->target, player->mo);
	}

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

			if ((maptol & TOL_NIGHTS) && target->player->powers[pw_carry] != CR_NIGHTSMODE && target->player->powers[pw_carry] != CR_NIGHTSFALL)
				return false;
<<<<<<< HEAD
=======

			switch (damagetype)
			{
#define DAMAGECASE(type)\
				case DMG_##type:\
					if (player->powers[pw_shield] & SH_PROTECT##type)\
						return false;\
					break
				DAMAGECASE(WATER);
				DAMAGECASE(FIRE);
				DAMAGECASE(ELECTRIC);
				DAMAGECASE(SPIKE);
#undef DAMAGECASE
				default:
					break;
			}
		}

		if (player->powers[pw_carry] == CR_NIGHTSMODE) // NiGHTS damage handling
		{
			if (!force)
			{
				if (source == target)
					return false; // Don't hit yourself with your own paraloop, baka
				if (source && source->player && !(cv_friendlyfire.value || (gametyperules & GTR_FRIENDLYFIRE))
				&& ((gametyperules & GTR_FRIENDLY)
				|| (G_GametypeHasTeams() && player->ctfteam == source->player->ctfteam)))
					return false; // Don't run eachother over in special stages and team games and such
			}
			if (LUAh_MobjDamage(target, inflictor, source, damage, damagetype))
				return true;
			P_NiGHTSDamage(target, source); // -5s :(
			return true;
		}

		if (G_IsSpecialStage(gamemap) && !(damagetype & DMG_DEATHMASK))
		{
			P_SpecialStageDamage(player, inflictor, source);
			return true;
		}

		if (!force && inflictor && inflictor->flags & MF_FIRE)
		{
			if (player->powers[pw_shield] & SH_PROTECTFIRE)
				return false; // Invincible to fire objects

			if (G_PlatformGametype() && inflictor && source && source->player)
				return false; // Don't get hurt by fire generated from friends.
>>>>>>> srb2/next
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
	if (G_BattleGametype())
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
