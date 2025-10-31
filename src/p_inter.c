// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
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
#include "k_specialstage.h"
#include "k_pwrlv.h"
#include "k_profiles.h"
#include "k_grandprix.h"
#include "k_respawn.h"
#include "p_spec.h"
#include "k_objects.h"
#include "k_roulette.h"
#include "k_boss.h"
#include "k_hitlag.h"
#include "acs/interface.h"
#include "k_powerup.h"
#include "k_collide.h"
#include "m_easing.h"
#include "k_hud.h" // K_AddMessage

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
	if (player->exiting || mapreset || (player->pflags & PF_ELIMINATED) || player->itemRoulette.reserved)
		return false;

	// See p_local.h for pickup types

	if (weapon != PICKUP_EGGBOX && player->instaWhipCharge)
		return false;

	if (weapon == PICKUP_ITEMBOX && !player->cangrabitems)
		return false;

	if (weapon == PICKUP_RINGORSPHERE)
	{
		// No picking up rings while SPB is targetting you
		if (player->pflags & PF_RINGLOCK)
		{
			return false;
		}

		// No picking up rings while stunned
		if (player->stunned > 0)
		{
			return false;
		}
	}
	else
	{
		// Item slot already taken up
		if (weapon == PICKUP_EGGBOX)
		{
			// Invulnerable
			if (player->flashing > 0)
				return false;

			// Already have fake
			if ((player->itemRoulette.active && player->itemRoulette.eggman) == true
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
			if (player->itemRoulette.active == true
				|| player->ringboxdelay > 0
				|| (weapon != PICKUP_PAPERITEM && player->itemamount)
				|| (player->itemflags & IF_ITEMOUT))
				return false;

			if (weapon == PICKUP_PAPERITEM && K_GetShieldFromItem(player->itemtype) != KSHIELD_NONE)
				return false; // No stacking shields!
		}
	}

	return true;
}

// Allow players to pick up only one pickup from each set of pickups.
// Anticheese pickup types are different than-P_CanPickupItem weapon, because that system is
// already slightly scary without introducing special cases for different types of the same pickup.
// See p_local.h for cheese types.
boolean P_IsPickupCheesy(player_t *player, UINT8 type)
{
	extern consvar_t cv_debugcheese;

	if (cv_debugcheese.value)
	{
		return false;
	}

	if (gametyperules & GTR_CATCHER)
	{
		return false;
	}

	if (player->lastpickupdistance && player->lastpickuptype == type)
	{
		UINT32 distancedelta = min(player->distancetofinish - player->lastpickupdistance, player->lastpickupdistance - player->distancetofinish);
		if (distancedelta < 2500)
			return true;
	}
	return false;
}

void P_UpdateLastPickup(player_t *player, UINT8 type)
{
	player->lastpickuptype = type;
	player->lastpickupdistance = player->distancetofinish;
}

boolean P_CanPickupEmblem(player_t *player, INT32 emblemID)
{
	if (emblemID < 0 || emblemID >= MAXEMBLEMS)
	{
		// Invalid emblem ID, can't pickup.
		return false;
	}

	if (demo.playback)
	{
		// Never collect emblems in replays.
		return false;
	}

	if (player != NULL)
	{
		if (player->bot)
		{
			// Your nefarious opponent puppy can't grab these for you.
			return false;
		}

		if (player->exiting)
		{
			// Yeah but YOU didn't actually do it now did you
			return false;
		}
	}

	return true;
}

boolean P_EmblemWasCollected(INT32 emblemID)
{
	if (emblemID < 0 || emblemID >= numemblems
	|| emblemlocations[emblemID].type == ET_NONE)
	{
		// Invalid emblem ID, can't pickup.
		return true;
	}

	return gamedata->collected[emblemID];
}

static void P_ItemPop(mobj_t *actor)
{
	/*
	INT32 locvar1 = var1;

	if (LUA_CallAction(A_ITEMPOP, actor))
		return;

	if (!(actor->target && actor->target->player))
	{
		if (cht_debug && !(actor->target && actor->target->player))
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}
	*/

	Obj_SpawnItemDebrisEffects(actor, actor->target);

	if (!specialstageinfo.valid
	&& (gametyperules & GTR_SPHERES) != GTR_SPHERES)
	{
		// Doesn't apply to Special
		P_SetMobjState(actor, S_RINGBOX1);
	}

	actor->extravalue1 = 0;

	// de-solidify
	// Do not set item boxes intangible, those are handled in fusethink for item pickup leniency
	// Sphere boxes still need to be set intangible here though
	if (actor->type != MT_RANDOMITEM)
		actor->flags |= MF_NOCLIPTHING;

	// RF_DONTDRAW will flicker as the object's fuse gets
	// closer to running out (see P_FuseThink)
	actor->renderflags |= RF_DONTDRAW|RF_TRANS50;
	actor->color = SKINCOLOR_GREY;
	actor->colorized = true;

	/*
	if (locvar1 == 1)
	{
		P_GivePlayerSpheres(actor->target->player, actor->extravalue1);
	}
	else if (locvar1 == 0)
	{
		if (actor->extravalue1 >= TICRATE)
			K_StartItemRoulette(actor->target->player, false);
		else
			K_StartItemRoulette(actor->target->player, true);
	}
	*/

	// Here at mapload in battle?
	if (gametype != GT_TUTORIAL && !(gametyperules & GTR_CIRCUIT) && (actor->flags2 & MF2_BOSSFLEE))
	{
		numgotboxes++;

		// do not flicker back in just yet, handled by
		// P_RespawnBattleBoxes eventually
		P_SetMobjState(actor, S_INVISIBLE);
	}
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

	if (LUA_HookTouchSpecial(special, toucher) || P_MobjWasRemoved(special))
		return;

	if ((special->flags & (MF_ENEMY|MF_BOSS)) && !(special->flags & MF_MISSILE))
	{
		////////////////////////////////////////////////////////
		/////ENEMIES & BOSSES!!/////////////////////////////////
		////////////////////////////////////////////////////////

		if (special->type == MT_BLENDEYE_MAIN)
		{
			if (!VS_BlendEye_Touched(special, toucher))
				return;
		}

		P_DamageMobj(toucher, special, special, 1, DMG_NORMAL);
		return;
	}
	else
	{
	// We now identify by object type, not sprite! Tails 04-11-2001
	switch (special->type)
	{
		case MT_FLOATINGITEM: // SRB2Kart
			if (special->extravalue1 > 0 && toucher != special->tracer)
			{
				if (special->tracer && !P_MobjWasRemoved(special->tracer) && special->tracer->player)
				{
					if (!G_SameTeam(special->tracer->player, player))
					{
						player->pflags |= PF_CASTSHADOW;
						return;
					}
				}

			}

			if (special->threshold >= FIRSTPOWERUP)
			{
				if (P_PlayerInPain(player))
					return;

				K_GivePowerUp(player, special->threshold, special->movecount);
			}
			else
			{
				// Avoid being picked up immediately
				if (special->scale < special->destscale/2)
					return;

				if (!P_CanPickupItem(player, PICKUP_PAPERITEM))
					return;

				if (special->threshold == KDROP_STONESHOETRAP)
				{
					if (K_TryPickMeUp(special, toucher, false))
						return;

					if (!P_MobjWasRemoved(player->stoneShoe))
					{
						player->pflags |= PF_CASTSHADOW;
						return;
					}

					P_SetTarget(&player->stoneShoe, Obj_SpawnStoneShoe(special->extravalue2, toucher));
					K_AddHitLag(toucher, 8, false);

					player_t *owner = Obj_StoneShoeOwnerPlayer(special);
					if (owner)
					{
						K_SpawnAmps(player, K_PvPAmpReward(20, owner, player), toucher);
						K_SpawnAmps(owner, K_PvPAmpReward(20, owner, player), toucher);
					}
				}
				else
				{
					if (player->itemamount && player->itemtype != special->threshold)
						return;

					player->itemtype = special->threshold;
					if ((UINT16)(player->itemamount) + special->movecount > 255)
						K_SetPlayerItemAmount(player, 255);
					else
						K_AdjustPlayerItemAmount(player, special->movecount);
				}
			}

			S_StartSound(special, special->info->deathsound);

			P_SetTarget(&special->tracer, toucher);
			special->flags2 |= MF2_NIGHTSPULL;
			special->destscale = mapobjectscale>>4;
			special->scalespeed <<= 1;

			special->flags &= ~MF_SPECIAL;
			return;
		case MT_RANDOMITEM: {
			UINT8 cheesetype = (special->flags2 & MF2_BOSSDEAD) ? CHEESE_RINGBOX : CHEESE_ITEMBOX; // perma ring box

			if (!P_CanPickupItem(player, PICKUP_ITEMBOX))
				return;
			if (P_IsPickupCheesy(player, cheesetype))
				return;

			special->momx = special->momy = special->momz = 0;
			P_SetTarget(&special->target, toucher);
			P_UpdateLastPickup(player, cheesetype);
			// P_KillMobj(special, toucher, toucher, DMG_NORMAL);

			statenum_t specialstate = special->state - states;

			if (special->fuse) // This box is respawning, but was broken very recently (see P_FuseThink)
			{
				// What was this box broken as?
				if (!K_ThunderDome() && special->cusval && !(special->flags2 & MF2_BOSSDEAD))
					K_StartItemRoulette(player, false);
				else
					K_StartItemRoulette(player, true);
			}
			else if (specialstate >= S_RANDOMITEM1 && specialstate <= S_RANDOMITEM12)
			{
				K_StartItemRoulette(player, false);
				special->cusval = 1; // Lenient pickup should be ITEM
			}
			else
			{
				K_StartItemRoulette(player, true);
				special->cusval = 0; // Lenient pickup should be RING
			}

			P_ItemPop(special);

			if (!special->fuse)
				special->fuse = TICRATE;
			return;
		}
		case MT_SPHEREBOX:
			if (!P_CanPickupItem(player, PICKUP_RINGORSPHERE))
				return;

			special->momx = special->momy = special->momz = 0;
			P_SetTarget(&special->target, toucher);
			// P_KillMobj(special, toucher, toucher, DMG_NORMAL);
			P_ItemPop(special);
			P_GivePlayerSpheres(player, special->extravalue2);
			return;
		case MT_ITEMCAPSULE:
			if (special->scale < special->extravalue1) // don't break it while it's respawning
				return;

			switch (special->threshold)
			{
				case KITEM_SPB:
					if (K_IsSPBInGame()) // don't spawn a second SPB
						return;
					break;
				case KCAPSULE_RING:
					if (!P_CanPickupItem(player, PICKUP_RINGORSPHERE)) // no cheaty rings
						return;
					break;
				default:
					if (!P_CanPickupItem(player, PICKUP_ITEMCAPSULE))
						return;
					if (P_IsPickupCheesy(player, CHEESE_ITEMCAPSULE))
						return;
					break;
			}

			// Ring Capsules shouldn't affect pickup cheese, they're just used as condensed ground-ring placements.
			if (special->threshold != KCAPSULE_RING)
				P_UpdateLastPickup(player, 3);

			S_StartSound(toucher, special->info->deathsound);
			P_KillMobj(special, toucher, toucher, DMG_NORMAL);
			return;
		case MT_KARMAHITBOX:
			if (!special->target->player)
				return;
			if (player == special->target->player)
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
		case MT_DUELBOMB:
			{
				Obj_DuelBombTouch(special, toucher);
				return;
			}
		case MT_EMERALD:
			if (!P_CanPickupItem(player, PICKUP_RINGORSPHERE) || P_PlayerInPain(player))
				return;

			if (special->threshold > 0)
				return;

			if (toucher->hitlag > 0)
				return;

			// Emerald will now orbit the player

			{
				const tic_t orbit = 2*TICRATE;
				Obj_BeginEmeraldOrbit(special, toucher, toucher->radius, orbit, orbit * 20);
				Obj_SetEmeraldAwardee(special, toucher);
			}

			// You have 6 emeralds and you touch the 7th: win instantly!
			if (ALLCHAOSEMERALDS((player->emeralds | special->extravalue1)))
			{
				player->emeralds |= special->extravalue1;
				K_CheckEmeralds(player);
			}

			return;
		case MT_SPECIAL_UFO:
			if (Obj_UFOEmeraldCollect(special, toucher) == false)
			{
				return;
			}

			break;
		/*
		case MT_EERIEFOG:
			special->frame &= ~FF_TRANS80;
			special->frame |= FF_TRANS90;
			return;
		*/

		case MT_SPECIALSTAGEBOMB:
			// only attempt to damage the player if they're not invincible
			if (!(player->invincibilitytimer > 0
				|| K_IsBigger(toucher, special) == true
				|| K_PlayerGuard(player) == true
				|| player->hyudorotimer > 0))
			{
				if (P_DamageMobj(toucher, special, special, 1, DMG_STUMBLE))
				{
					P_SetMobjState(special, special->info->painstate);
					special->eflags |= MFE_DAMAGEHITLAG;
					return;
				}
			}
			// if we didn't damage the player, just explode
			P_SetMobjState(special, special->info->painstate);
			P_SetMobjState(special, special->info->raisestate); // immediately explode visually
			return;

		case MT_CDUFO: // SRB2kart
			if (special->fuse || !P_CanPickupItem(player, PICKUP_ITEMBOX))
				return;

			K_StartItemRoulette(player, false);

			// Karma fireworks
			/*for (i = 0; i < 5; i++)
			{
				mobj_t *firework = P_SpawnMobj(special->x, special->y, special->z, MT_KARMAFIREWORK);
				firework->momx = toucher->momx;
				firework->momy = toucher->momy;
				firework->momz = toucher->momz;
				P_Thrust(firework, FixedAngle((72*i)<<FRACBITS), P_RandomRange(PR_ITEM_DEBRIS, 1,8)*special->scale);
				P_SetObjectMomZ(firework, P_RandomRange(PR_ITEM_DEBRIS, 1,8)*special->scale, false);
				firework->color = toucher->color;
			}*/

			K_SetHitLagForObjects(special, toucher, toucher, 2, true);

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

			if (K_TryPickMeUp(special, toucher, false))
				return;

			if (special->target && !P_MobjWasRemoved(special->target) && toucher->player && (toucher->player != (special->target->player))) // Last condition here is so you can't get your own amps
			{
				K_SpawnAmps(special->target->player, K_PvPAmpReward(20, special->target->player, toucher->player), toucher);
			}

			// attach to player!
			P_SetTarget(&special->tracer, toucher);
			toucher->flags |= MF_NOGRAVITY;
			toucher->momz = (8*toucher->scale) * P_MobjFlip(toucher);
			toucher->player->carry = CR_TRAPBUBBLE;
			P_SetTarget(&toucher->tracer, special); //use tracer to acces the object

			// Snap to the unfortunate player and quit moving laterally, or we can end up quite far away
			special->momx = 0;
			special->momy = 0;
			special->x = toucher->x;
			special->y = toucher->y;
			special->z = toucher->z;

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

			// Prepping instawhip? Don't ruin it by collecting rings
			if (player->instaWhipCharge)
				return;

			if (player->baildrop || player->bailcharge || player->defenseLockout > PUNISHWINDOW)
				return;

			// Don't immediately pick up spilled rings
			if (special->threshold > 0 || P_PlayerInPain(player) || player->spindash) // player->spindash: Otherwise, players can pick up rings that are thrown out of them from invinc spindash penalty
				return;

			if (!(P_CanPickupItem(player, PICKUP_RINGORSPHERE)))
				return;

			// Reached the cap, don't waste 'em!
			if (RINGTOTAL(player) >= 20)
				return;

			special->momx = special->momy = special->momz = 0;

			special->extravalue1 = 1; // Ring collect animation timer
			special->angle = R_PointToAngle2(toucher->x, toucher->y, special->x, special->y); // animation angle
			P_SetTarget(&special->target, toucher); // toucher for thinker

			// For MT_FLINGRING - don't delete yourself mid-pickup.
			special->renderflags &= ~RF_DONTDRAW;
			special->fuse = 0;

			player->pickuprings++;

			return;

		case MT_BLUESPHERE:
			if (!(P_CanPickupItem(player, PICKUP_RINGORSPHERE)))
				return;

			P_GivePlayerSpheres(player, 1);
			break;

		// Secret emblem thingy
		case MT_EMBLEM:
			{
				if (!P_CanPickupEmblem(player, special->health - 1))
					return;

				if (!P_IsPartyPlayer(player))
				{
					// Must be party.
					return;
				}

				if (!gamedata->collected[special->health-1])
				{
					gamedata->collected[special->health-1] = true;
					if (!M_UpdateUnlockablesAndExtraEmblems(true, true))
						S_StartSound(NULL, sfx_ncitem);
					gamedata->deferredsave = true;
				}

				// Don't delete the object, just fade it.
				return;
			}

		case MT_SPRAYCAN:
			{
				if (demo.playback)
				{
					// Never collect emblems in replays.
					return;
				}

				if (player->bot)
				{
					// Your nefarious opponent puppy can't grab these for you.
					return;
				}

				if (player->exiting)
				{
					// Yeah but YOU didn't actually do it now did you
					return;
				}

				if (!P_IsPartyPlayer(player))
				{
					// Must be party.
					return;
				}

				// See also P_SprayCanInit
				UINT16 can_id = mapheaderinfo[gamemap-1]->records.spraycan;

				if (can_id < gamedata->numspraycans || can_id == MCAN_BONUS)
				{
					// Assigned to this level, has been grabbed
					return;
				}

				if (
					(gamemap-1 >= basenummapheaders)
					|| (gamedata->gotspraycans >= gamedata->numspraycans)
				)
				{
					// Custom course OR we ran out of assignables.

					if (special->threshold != 0)
						return;

					can_id = MCAN_BONUS;
				}
				else
				{
					// Unassigned, get the next grabbable colour
					can_id = gamedata->gotspraycans;

					// Multiple cans in one map?
					if (special->threshold != 0)
					{
						UINT16 ref_id = can_id + (special->threshold & UINT8_MAX);
						if (ref_id >= gamedata->numspraycans)
							return;

						// Swap this specific can to the head of the list.
						UINT16 swapcol = gamedata->spraycans[ref_id].col;

						gamedata->spraycans[ref_id].col =
							gamedata->spraycans[can_id].col;
						skincolors[gamedata->spraycans[ref_id].col].cache_spraycan = ref_id;

						gamedata->spraycans[can_id].col = swapcol;
						skincolors[swapcol].cache_spraycan = can_id;
					}

					gamedata->spraycans[can_id].map = gamemap-1;

					if (gamedata->gotspraycans == 0
					&& gametype == GT_TUTORIAL
					&& cv_ttlprofilen.value > 0
					&& cv_ttlprofilen.value < PR_GetNumProfiles())
					{
						profile_t *p = PR_GetProfile(cv_ttlprofilen.value);
						if (p->color == SKINCOLOR_NONE)
						{
							// Apply your favourite colour to the profile!
							p->color = gamedata->spraycans[can_id].col;
						}
					}

					gamedata->gotspraycans++;
				}

				mapheaderinfo[gamemap-1]->records.spraycan = can_id;

				if (!M_UpdateUnlockablesAndExtraEmblems(true, true))
					S_StartSound(NULL, sfx_ncitem);
				gamedata->deferredsave = true;

				{
					mobj_t *canmo = NULL;
					mobj_t *next = NULL;

					for (canmo = trackercap; canmo; canmo = next)
					{
						next = canmo->itnext;

						if (canmo->type != MT_SPRAYCAN)
							continue;

						// Don't delete the object(s), just fade it.
						if (netgame || canmo == special)
						{
							P_SprayCanInit(canmo);
							continue;
						}

						// Get ready to get rid of these
						canmo->renderflags |= (tr_trans50 << RF_TRANSSHIFT);
						canmo->destscale = 0;
					}
				}

				return;
			}

		case MT_PRISONEGGDROP:
			{
				if (demo.playback)
				{
					// Never collect emblems in replays.
					return;
				}

				if (player->bot)
				{
					// Your nefarious opponent puppy can't grab these for you.
					return;
				}

				if (!P_IsPartyPlayer(player))
				{
					// Must be party.
					return;
				}

				if (special->hitlag || special->scale < mapobjectscale/2)
				{
					// Don't get during the initial activation
					return;
				}

				if (special->extravalue1)
				{
					// Don't get during destruction
					return;
				}

				if (special->scale > mapobjectscale)
				{
					// Short window so you can't pick it up instantly
					return;
				}

				if (
					grandprixinfo.gp == true // Bonus Round
					&& netgame == false // game design + makes it easier to implement
					&& gamedata->thisprisoneggpickup_cached != NULL
				)
				{
					gamedata->thisprisoneggpickupgrabbed = true;
					if (gamedata->prisoneggstothispickup < GDINIT_PRISONSTOPRIZE)
					{
						// Just in case it's set absurdly low for testing.
						gamedata->prisoneggstothispickup = GDINIT_PRISONSTOPRIZE;
					}

					if (!M_UpdateUnlockablesAndExtraEmblems(true, true))
						S_StartSound(NULL, sfx_ncitem);
					gamedata->deferredsave = true;
				}

				statenum_t teststate = (special->state-states);

				if (teststate == S_PRISONEGGDROP_CD)
				{
					if (P_IsObjectOnGround(special))
					{
						special->momz = P_MobjFlip(special) * 2 * mapobjectscale;
						special->flags = (special->flags & ~MF_SPECIAL) | (MF_NOGRAVITY|MF_NOCLIPHEIGHT);
					}

					special->extravalue1 = 1;

					special->renderflags = (special->renderflags & ~RF_BRIGHTMASK) | (RF_ADD | RF_FULLBRIGHT);

					return;
				}

				break;
			}

		case MT_LSZ_BUNGEE:
			Obj_BungeeSpecial(special, player);
			return;

		case MT_CHEATCHECK:
			P_TouchCheatcheck(special, player, special->thing_args[1]);
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
				S_StartSound (special, special->info->deathsound+(P_RandomKey(PR_DECORATION, special->info->mass)));
			}
			return;

		case MT_LOOPENDPOINT:
			Obj_LoopEndpointCollide(special, toucher);
			return;

		case MT_RINGSHOOTER:
			if (player->freeRingShooterCooldown)
				player->pflags |= PF_CASTSHADOW; // you can't use this right now!
			else
				Obj_PlayerUsedRingShooter(special, player);
			return;

		case MT_SUPER_FLICKY:
			Obj_SuperFlickyPlayerCollide(special, toucher);
			return;

		case MT_DASHRING:
		case MT_RAINBOWDASHRING:
			Obj_DashRingTouch(special, player);
			return;

		case MT_ADVENTUREAIRBOOSTER_HITBOX:
			Obj_AdventureAirBoosterHitboxTouch(special, player);
			return;

		case MT_DLZ_ROCKET:
			Obj_DLZRocketSpecial(special, player);
			return;

		case MT_AHZ_CLOUD:
		case MT_AGZ_CLOUD:
		case MT_SSZ_CLOUD:
			Obj_CloudTouched(special, toucher);
			return;

		case MT_AGZ_BULB:
			Obj_BulbTouched(special, toucher);
			return;

		case MT_BALLSWITCH_BALL:
		{
			Obj_BallSwitchTouched(special, toucher);
			return;
		}

		case MT_BLENDEYE_PUYO:
		{
			if (!VS_PuyoTouched(special, toucher))
				return;
			break;
		}

		case MT_GGZICEDUST:
		{
			Obj_IceDustCollide(special, toucher);
			return;
		}

		case MT_IVOBALL:
		case MT_AIRIVOBALL:
		{
			Obj_IvoBallTouch(special, toucher);
			return;
		}
		case MT_PATROLIVOBALL:
		{
			Obj_PatrolIvoBallTouch(special, toucher);
			return;
		}

		case MT_SA2_CRATE:
		case MT_ICECAPBLOCK:
		{
			Obj_TryCrateTouch(special, toucher);
			return;
		}

		case MT_BETA_PARTICLE_PHYSICAL:
		{
			Obj_FuelCanisterTouch(special, toucher);
			break;
		}

		case MT_BETA_PARTICLE_EXPLOSION:
		{
			Obj_FuelCanisterExplosionTouch(special, toucher);
			return;
		}

		case MT_AZROCKS:
		case MT_EMROCKS:
		{
			Obj_TouchRocks(special, toucher);
			return;
		}

		case MT_TRICKBALLOON_RED:
		case MT_TRICKBALLOON_YELLOW:
			Obj_TrickBalloonTouchSpecial(special, toucher);
			return;

		case MT_PULLUPHOOK:
			Obj_PulleyHookTouch(special, toucher);
			return;

		case MT_STONESHOE_CHAIN:
			Obj_CollideStoneShoe(toucher, special);
			return;

		case MT_TOXOMISTER_POLE:
			Obj_ToxomisterPoleCollide(special, toucher);
			return;

		case MT_TOXOMISTER_CLOUD:
			Obj_ToxomisterCloudCollide(special, toucher);
			return;

		case MT_ANCIENTGEAR:
			Obj_AncientGearTouch(special, toucher);
			return;

		case MT_MHPOLE:
			Obj_MushroomHillPoleTouch(special, toucher);
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

/** Saves a player's level progress at a Cheat Check
  *
  * \param post The Cheat Check to trigger
  * \param player The player that should receive the cheatcheck
  * \param snaptopost If true, the respawn point will use the cheatcheck's position, otherwise player x/y and star post z
  */
void P_TouchCheatcheck(mobj_t *post, player_t *player, boolean snaptopost)
{
	mobj_t *toucher = player->mo;

	(void)snaptopost;

	// Player must have touched all previous cheatchecks
	if (post->health - player->cheatchecknum > 1)
	{
		if (!player->checkskip)
			S_StartSound(toucher, sfx_lose);
		player->checkskip = 3;
		return;
	}

	// With the parameter + angle setup, we can go up to 1365 star posts. Who needs that many?
	if (post->health > 1365)
	{
		CONS_Debug(DBG_GAMELOGIC, "Bad Cheatcheck Number!\n");
		return;
	}

	if (player->cheatchecknum >= post->health)
		return; // Already hit this post

	player->cheatchecknum = post->health;
}

void P_TrackRoundConditionTargetDamage(targetdamaging_t targetdamaging)
{
	UINT8 i;
	for (i = 0; i <= splitscreen; i++)
	{
		if (!playeringame[g_localplayers[i]])
			continue;
		if (players[g_localplayers[i]].spectator)
			continue;
		players[g_localplayers[i]].roundconditions.targetdamaging |= targetdamaging;
		/* -- the following isn't needed because we can just check for targetdamaging == UFOD_GACHABOM
		if (targetdamaging != UFOD_GACHABOM)
			players[g_localplayers[i]].roundconditions.gachabom_miser = 0xFF;
		*/
	}
}

static void P_AddBrokenPrison(mobj_t *target, mobj_t *inflictor, mobj_t *source)
{
	if (!battleprisons)
		return;

	// Check to see if everyone's out.
	{
		UINT8 i = 0;

		for (; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator || players[i].exiting)
				continue;
			break;
		}

		if (i == MAXPLAYERS)
		{
			// Nobody can claim credit for this just-too-late hit!
			P_DoAllPlayersExit(0, false); // softlock prevention
			return;
		}
	}

	// If you CAN recieve points, get them!
	if ((gametyperules & GTR_POINTLIMIT)
		&& (source && !P_MobjWasRemoved(source) && source->player))
	{
		K_GivePointsToPlayer(source->player, NULL, 1);
	}

	targetdamaging_t targetdamaging = UFOD_GENERIC;
	if (!inflictor || P_MobjWasRemoved(inflictor) == true)
		;
	else switch (inflictor->type)
	{
		case MT_GACHABOM:
			targetdamaging = UFOD_GACHABOM;
			break;
		case MT_ORBINAUT:
		case MT_ORBINAUT_SHIELD:
			targetdamaging = UFOD_ORBINAUT;
			break;
		case MT_BANANA:
			targetdamaging = UFOD_BANANA;
			break;
		case MT_INSTAWHIP:
			targetdamaging = UFOD_WHIP;
			break;
		// This is only accessible for MT_CDUFO's touch!
		case MT_PLAYER:
			targetdamaging = UFOD_BOOST;
			break;
		// The following can't be accessed in standard play...
		// but the cost of tracking them here is trivial :D
		case MT_JAWZ:
		case MT_JAWZ_SHIELD:
			targetdamaging = UFOD_JAWZ;
			break;
		case MT_SPB:
			targetdamaging = UFOD_SPB;
			break;
		default:
			break;
	}

	P_TrackRoundConditionTargetDamage(targetdamaging);

	if (gamedata->prisoneggstothispickup)
	{
		gamedata->prisoneggstothispickup--;
	}

	// Standard progression.
	if (++numtargets >= maptargets)
	{
		// Yipue!

		P_DoAllPlayersExit(0, true);
	}
	else
	{
		S_StartSound(NULL, sfx_s221);

		// Time limit recovery
		if (timelimitintics)
		{
			UINT16 bonustime = 10*TICRATE;
			INT16 clamptime = 0; // Don't allow reserve time past this value (by much)...
			INT16 mintime = 5*TICRATE; // But give SOME reward for every hit. (This value used for Normal)

			if (grandprixinfo.gp)
			{
				if (grandprixinfo.masterbots)
				{
					clamptime = 10*TICRATE;
					mintime = 1*TICRATE;
				}
				else if (grandprixinfo.gamespeed == KARTSPEED_HARD)
				{
					clamptime = 15*TICRATE;
					mintime = 2*TICRATE;
				}
				else if (grandprixinfo.gamespeed == KARTSPEED_NORMAL)
				{
					clamptime = 20*TICRATE;
					mintime = 5*TICRATE;
				}
				else if (grandprixinfo.gamespeed == KARTSPEED_EASY)
				{
					// "I think Easy Mode should be about Trying Not To Kill Your Self" -VelocitOni
					clamptime = 45*TICRATE;
					mintime = 20*TICRATE;
				}
			}

			UINT16 effectivetime = timelimitintics + extratimeintics - leveltime + starttime;

			if (clamptime) // Lower bonus if you have more reserve, keep it tense.
			{
				bonustime = Easing_InOutSine(min(FRACUNIT, (effectivetime) * FRACUNIT / clamptime), bonustime, mintime);

				// Quicker rolloff if you're stacking time substantially past clamptime
				if ((effectivetime + bonustime) > clamptime)
					bonustime = Easing_InSine(min(FRACUNIT, (effectivetime + bonustime - clamptime) * FRACUNIT / clamptime), bonustime, 1);
			}

			extratimeintics += bonustime;
			secretextratime = TICRATE/2;
		}

		// Everything below dependent on our coords
		if (!target || P_MobjWasRemoved(target))
			return;

		// Prison Egg challenge drops (CDs, etc)
#ifdef DEVELOP
		extern consvar_t cv_debugprisoncd;
#endif
		if ((
			grandprixinfo.gp == true // Bonus Round
			&& demo.playback == false // Not playback
			&& netgame == false // game design + makes it easier to implement
			&& gamedata->thisprisoneggpickup_cached != NULL
			&& gamedata->prisoneggstothispickup == 0
			&& gamedata->thisprisoneggpickupgrabbed == false
			)
#ifdef DEVELOP
			|| (cv_debugprisoncd.value && gamedata->thisprisoneggpickup_cached != NULL)
#endif
		)
		{
			// Will be 0 for the next level
			gamedata->prisoneggstothispickup = (maptargets - numtargets);

			mobj_t *secretpickup = P_SpawnMobj(
				target->x, target->y,
				target->z + target->height/2,
				MT_PRISONEGGDROP
			);

			if (secretpickup)
			{
				secretpickup->hitlag = target->hitlag;

				secretpickup->z -= secretpickup->height/2;

				P_SetScale(secretpickup, 3*secretpickup->scale);
				secretpickup->scalespeed = (secretpickup->scale - secretpickup->destscale) / TICRATE;

				// flags are NOT from the target - just in case it's just been placed on the ceiling as a gimmick
				secretpickup->flags2 |= (source->flags2 & MF2_OBJECTFLIP);
				secretpickup->eflags |= (source->eflags & MFE_VERTICALFLIP);

				// Okay these have to use M_Random because replays...
				// The spawning of these won't be recorded back!
				const fixed_t dist = R_PointToDist2(target->x, target->y, source->x, source->y);
				const fixed_t maxDist = 640 * mapobjectscale;
				const fixed_t launchmomentum = Easing_Linear(
					FixedDiv(min(dist, maxDist), maxDist),
					5 * mapobjectscale,
					20 * mapobjectscale
				);
				secretpickup->momz = P_MobjFlip(target) * launchmomentum; // THIS one uses target!

				mobj_t *flare = P_SpawnMobj(
					target->x, target->y,
					target->z + target->height/2,
					MT_SPARK
				);

				if (flare)
				{
					// Will flicker in place until secretpickup exits hitlag.
					flare->colorized = true;
					flare->renderflags |= RF_ALWAYSONTOP;
					P_InstaScale(flare, 4 * flare->scale);
					P_SetTarget(&secretpickup->target, flare);
					P_SetMobjStateNF(flare, S_PRISONEGGDROP_FLAREA1);
				}

				// Darken the level for roughly how long it takes until the last sound effect stops playing.
				g_darkness.start = leveltime;
				g_darkness.end = leveltime + target->hitlag + TICRATE + DARKNESS_FADE_TIME;
			}
		}
	}
}

/** Checks if the level timer is over the timelimit and the round should end,
  * unless you are in overtime. In which case leveltime may stretch out beyond
  * timelimitintics and overtime's status will be checked here each tick.
  *
  * \sa cv_timelimit, P_CheckPointLimit, P_UpdateSpecials
  */
void P_CheckTimeLimit(void)
{
	if (exitcountdown)
		return;

	if (!timelimitintics)
		return;

	if (leveltime < starttime)
	{
		if (secretextratime)
			secretextratime--;
		return;
	}

	if (leveltime < (timelimitintics + starttime))
	{
		if (secretextratime)
		{
			secretextratime--;
			timelimitintics++;
		}
		else if (extratimeintics)
		{
			timelimitintics++;
			if (leveltime & 1)
				;
			else
			{
				if (extratimeintics > 20)
				{
					extratimeintics -= 20;
					timelimitintics += 20;
				}
				else
				{
					timelimitintics += extratimeintics;
					extratimeintics = 0;
				}
				S_StartSound(NULL, sfx_ptally);
			}
		}
		else
		{
			if (timelimitintics + starttime - leveltime <= 3*TICRATE)
			{
				if (((timelimitintics + starttime - leveltime) % TICRATE) == 0)
					S_StartSound(NULL, sfx_s3ka7);
			}
		}
		return;
	}

	if (gameaction == ga_completed)
		return;

	if ((grandprixinfo.gp == false) && (cv_overtime.value) && (gametyperules & GTR_OVERTIME))
	{
#ifndef TESTOVERTIMEINFREEPLAY
		UINT8 i;
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

					// Get largest radius from center point to minimap edges

					fixed_t r = 0;
					fixed_t n;
#define corner(px, py) ((n = FixedHypot(battleovertime.x - (px), battleovertime.y - (py))), r = max(r, n))
					corner(minimapinfo.min_x * FRACUNIT, minimapinfo.min_y * FRACUNIT);
					corner(minimapinfo.min_x * FRACUNIT, minimapinfo.max_y * FRACUNIT);
					corner(minimapinfo.max_x * FRACUNIT, minimapinfo.min_y * FRACUNIT);
					corner(minimapinfo.max_x * FRACUNIT, minimapinfo.max_y * FRACUNIT);
#undef corner

					battleovertime.initial_radius = min(
							max(r, 4096 * mapobjectscale),
							// Prevent overflow in K_RunBattleOvertime
							FixedDiv(INT32_MAX, M_PI_FIXED) / 2
					);

					battleovertime.radius = battleovertime.initial_radius;

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

	P_DoAllPlayersExit(0, false);
}

/** Checks if a player's score is over the pointlimit and the round should end.
  *
  * \sa cv_pointlimit, P_CheckTimeLimit, P_UpdateSpecials
  */
void P_CheckPointLimit(void)
{
	INT32 i;

	if (exitcountdown)
		return;

	if (!K_CanChangeRules(true))
		return;

	if (!g_pointlimit)
		return;

	if (!(gametyperules & GTR_POINTLIMIT))
		return;

	if (battleprisons)
		return;

	// This will be handled by P_KillPlayer
	if (gametyperules & GTR_BUMPERS)
		return;

	// pointlimit is nonzero, check if it's been reached by this player
	if (G_GametypeHasTeams() == true)
	{
		for (i = 0; i < TEAM__MAX; i++)
		{
			if (g_pointlimit <= g_teamscores[i])
			{
				P_DoAllPlayersExit(0, false);
				return;
			}
		}
	}
	else
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (g_pointlimit <= players[i].roundscore)
			{
				P_DoAllPlayersExit(0, false);
				return;
			}
		}
	}
}

// Checks whether or not to end a race netgame.
boolean P_CheckRacers(void)
{
	const boolean griefed = (spectateGriefed > 0);

	boolean eliminateLast = (!K_CanChangeRules(true) || (cv_karteliminatelast.value != 0));

	boolean allHumansDone = true;
	//boolean allBotsDone = true;

	UINT8 numPlaying = 0;
	UINT8 numExiting = 0;
	UINT8 numHumans = 0;
	UINT8 numBots = 0;

	UINT8 i;

	// Check if all the players in the race have finished. If so, end the level.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator || (players[i].lives <= 0 && !players[i].exiting))
		{
			// Y'all aren't even playing
			continue;
		}

		numPlaying++;

		if (players[i].bot)
		{
			numBots++;
		}
		else
		{
			numHumans++;
		}

		if (players[i].exiting || (players[i].pflags & PF_NOCONTEST))
		{
			numExiting++;
		}
		else
		{
			if (players[i].bot)
			{
				//allBotsDone = false;
			}
			else
			{
				allHumansDone = false;
			}
		}
	}

	if (numPlaying <= 1 || specialstageinfo.valid == true)
	{
		// Never do this without enough players.
		eliminateLast = false;
	}
	else
	{
		if (griefed == true && numHumans > 0)
		{
			// Don't do this if someone spectated
			eliminateLast = false;
		}
#ifndef DEVELOP
		else if (grandprixinfo.gp == true)
		{
			eliminateLast = true;
		}
#endif
	}

	if (grandprixinfo.gp && grandprixinfo.gamespeed == KARTSPEED_EASY)
		eliminateLast = false;

	if (eliminateLast == true && (numExiting >= numPlaying-1))
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
		racecountdown = 0;
		return true;
	}

	if (numHumans > 0 && allHumansDone == true)
	{
		// There might be bots that are still going,
		// but all of the humans are done, so we can exit now.
		racecountdown = 0;
		return true;
	}

	// SO, we're not done playing.
	// Let's see if it's time to start the death counter!

	if (racecountdown == 0 && K_Cooperative() == false)
	{
		// If the winners are all done, then start the death timer.
		UINT8 winningPos = max(1, numPlaying / 2);

		if (numPlaying % 2) // Any remainder? Then round up.
		{
			winningPos++;
		}

		if (numExiting >= winningPos)
		{
			tic_t countdown = 30*TICRATE; // 30 seconds left to finish, get going!

			if (K_CanChangeRules(true) == true)
			{
				// Custom timer
				countdown = cv_countdowntime.value * TICRATE;
			}

			racecountdown = countdown + 1;
		}
	}

	if (grandprixinfo.gp && !G_GametypeUsesLives()) // Relaxed
		racecountdown = 0;

	// We're still playing, but no one else is,
	// so we need to reset spectator griefing.
	if (numPlaying <= 1)
	{
		spectateGriefed = 0;
	}

	// We are still having fun and playing the game :)
	return false;
}

void P_UpdateRemovedOrbital(mobj_t *target, mobj_t *inflictor, mobj_t *source)
{
	// SRB2kart
	// I wish I knew a better way to do this
	if (!P_MobjWasRemoved(target->target) && target->target->player && !P_MobjWasRemoved(target->target->player->mo))
	{
		if ((target->target->player->itemflags & IF_EGGMANOUT) && target->type == MT_EGGMANITEM_SHIELD)
			target->target->player->itemflags &= ~IF_EGGMANOUT;

		if (target->target->player->itemflags & IF_ITEMOUT)
		{
			if ((target->type == MT_BANANA_SHIELD && target->target->player->itemtype == KITEM_BANANA) // trail items
				|| (target->type == MT_SSMINE_SHIELD && target->target->player->itemtype == KITEM_MINE)
				|| (target->type == MT_DROPTARGET_SHIELD && target->target->player->itemtype == KITEM_DROPTARGET)
				|| (target->type == MT_SINK_SHIELD && target->target->player->itemtype == KITEM_KITCHENSINK))
			{
				if (target->movedir != 0 && target->movedir < (UINT16)target->target->player->itemamount)
				{
					if (target->target->hnext && !P_MobjWasRemoved(target->target->hnext))
						K_KillBananaChain(target->target->hnext, inflictor, source);

					K_SetPlayerItemAmount(target->target->player, 0);
				}
				else if (target->target->player->itemamount)
					K_AdjustPlayerItemAmount(target->target->player, -1);
			}
			else if ((target->type == MT_ORBINAUT_SHIELD && target->target->player->itemtype == KITEM_ORBINAUT) // orbit items
				|| (target->type == MT_JAWZ_SHIELD && target->target->player->itemtype == KITEM_JAWZ))
			{
				if (target->target->player->itemamount)
					K_AdjustPlayerItemAmount(target->target->player, -1);
				if (target->lastlook != 0)
				{
					K_RepairOrbitChain(target);
				}
			}

			if (!target->target->player->itemamount)
				target->target->player->itemflags &= ~IF_ITEMOUT;

			if (target->target->hnext == target)
				P_SetTarget(&target->target->hnext, NULL);
		}
	}
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
	if (target->flags & (MF_ENEMY|MF_BOSS))
		target->momx = target->momy = target->momz = 0;

	// SRB2kart
	if (target->type != MT_PLAYER
		 && !(target->type == MT_ORBINAUT || target->type == MT_ORBINAUT_SHIELD
		 || target->type == MT_JAWZ || target->type == MT_JAWZ_SHIELD
		 || target->type == MT_BANANA || target->type == MT_BANANA_SHIELD
		 || target->type == MT_DROPTARGET || target->type == MT_DROPTARGET_SHIELD
		 || target->type == MT_EGGMANITEM || target->type == MT_EGGMANITEM_SHIELD
		 || target->type == MT_BALLHOG || target->type == MT_SPB
		 || target->type == MT_GACHABOM || target->type == MT_KART_LEFTOVER)) // kart dead items
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

	if (target->type != MT_BATTLEBUMPER && target->type != MT_PLAYER)
	{
		target->shadowscale = 0;
	}

	if (LUA_HookMobjDeath(target, inflictor, source, damagetype) || P_MobjWasRemoved(target))
		return;

	P_ActivateThingSpecial(target, source);

	//K_SetHitLagForObjects(target, inflictor, source, MAXHITLAGTICS, true);

	P_UpdateRemovedOrbital(target, inflictor, source);
	// Above block does not clean up rocket sneakers when a player dies, so we need to do it here target->target is null when using rocket sneakers
	if (target->player)
		K_DropRocketSneaker(target->player);

	// Let EVERYONE know what happened to a player! 01-29-2002 Tails
	if (target->player && !target->player->spectator)
	{
		target->renderflags &= ~RF_DONTDRAW;
	}

	// if killed by a player
	if (source && source->player)
	{
		if (target->type == MT_RANDOMITEM)
		{
			P_SetTarget(&target->target, source);

			if (!(gametyperules & GTR_CIRCUIT))
			{
				target->fuse = 2;
			}
			else
			{
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
		target->flags |= MF_NOBLOCKMAP|MF_NOCLIPTHING|MF_NOGRAVITY;
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
			}
		}

		if (target->player->spectator == false)
		{
			UINT32 skinflags = (demo.playback)
				? demo.skinlist[demo.currentskinid[(target->player-players)]].flags
				: skins[target->player->skin]->flags;

			if (skinflags & SF_IRONMAN)
			{
				target->skin = skins[target->player->skin];
				target->player->charflags = skinflags;
				K_SpawnMagicianParticles(target, 5);
				S_StartSound(target, sfx_slip);
			}

			target->renderflags &= ~RF_DONTDRAW;
		}

		K_DropEmeraldsFromPlayer(target->player, target->player->emeralds);

		target->player->carry = CR_NONE;

		K_KartResetPlayerColor(target->player);

		P_ResetPlayer(target->player);

#define PlayerPointerRemove(field) \
		if (P_MobjWasRemoved(field) == false) \
		{ \
			P_RemoveMobj(field); \
			P_SetTarget(&field, NULL); \
		}

		PlayerPointerRemove(target->player->stumbleIndicator);
		PlayerPointerRemove(target->player->wavedashIndicator);
		PlayerPointerRemove(target->player->trickIndicator);

#undef PlayerPointerRemove

		if (gametyperules & GTR_BUMPERS)
		{
			if (battleovertime.enabled >= 10*TICRATE) // Overtime Barrier is armed
			{
				target->player->pflags |= PF_ELIMINATED;
				if (target->player->darkness_end < leveltime)
				{
					target->player->darkness_start = leveltime;
				}
				target->player->darkness_end = INFTICS;
			}

			K_CheckBumpers();

			P_AddPlayerScore(target->player, -2);
		}

		target->player->trickpanel = TRICKSTATE_NONE;

		ACS_RunPlayerDeathScript(target->player);
	}

	if (source && target && target->player && source->player && (target->player != source->player))
		P_PlayVictorySound(source); // Killer laughs at you. LAUGHS! BWAHAHAHA!

	// Other death animation effects
	switch(target->type)
	{
		case MT_BLASTEXECUTOR:
			if (target->spawnpoint)
				P_LinedefExecute(target->spawnpoint->angle, (source ? source : inflictor), target->subsector->sector);
			break;

		case MT_EGGTRAP:
			// Time for birdies! Yaaaaaaaay!
			target->fuse = TICRATE;
			break;

		case MT_PLAYER:
			if (damagetype != DMG_SPECTATOR)
			{
				fixed_t flingSpeed = FixedHypot(target->momx, target->momy);
				angle_t flingAngle;

				target->fuse = TICRATE*3; // timer before mobj disappears from view (even if not an actual player)
				target->momx = target->momy = target->momz = 0;

				Obj_SpawnDestroyedKart(target);

				if (source && !P_MobjWasRemoved(source))
				{
					flingAngle = R_PointToAngle2(
						source->x - source->momx, source->y - source->momy,
						target->x, target->y
					);
				}
				else
				{
					flingAngle = target->angle;

					if (P_RandomByte(PR_ITEM_RINGS) & 1)
					{
						flingAngle -= ANGLE_45/2;
					}
					else
					{
						flingAngle += ANGLE_45/2;
					}
				}

				// On -20 ring deaths, you're guaranteed to be hitting the ground from Tumble,
				// so make sure that this draws at the correct angle.
				target->rollangle = 0;

				target->player->instaWhipCharge = 0;

				fixed_t inflictorSpeed = 0;
				if (!P_MobjWasRemoved(inflictor))
				{
					inflictorSpeed = FixedHypot(inflictor->momx, inflictor->momy);
					if (inflictorSpeed > flingSpeed)
					{
						flingSpeed = inflictorSpeed;
					}
				}

				boolean battle = (gametyperules & (GTR_BUMPERS | GTR_BOSS)) == GTR_BUMPERS;
				P_InstaThrust(target, flingAngle, max(flingSpeed, 6 * target->scale) / (battle ? 1 : 3));
				P_SetObjectMomZ(target, battle ? 20*FRACUNIT : 18*FRACUNIT, false);
			}

			// Prisons Free Play: don't eliminate P1 for
			// spectating. Because in Free Play, this player
			// can enter the game again, and these flags would
			// make them intangible.
			if (!(gametyperules & GTR_CHECKPOINTS) && K_Cooperative() && !target->player->spectator)
			{
				target->player->pflags |= PF_ELIMINATED;

				if (!target->player->exiting)
				{
					target->player->pflags |= PF_NOCONTEST;
					K_InitPlayerTally(target->player);
				}
			}
			break;

		case MT_KART_LEFTOVER:
			if (!P_MobjWasRemoved(inflictor))
			{
				K_KartSolidBounce(target, inflictor);
				target->momz = 20 * inflictor->scale * P_MobjFlip(inflictor);
			}
			target->z += P_MobjFlip(target);
			target->tics = 175;
			return;

		// SRB2Kart:

		case MT_ITEMCAPSULE:
		{
			UINT8 i;
			mobj_t *attacker = inflictor ? inflictor : source;
			mobj_t *part = target->hnext;
			angle_t angle = FixedAngle(360*P_RandomFixed(PR_ITEM_DEBRIS));
			INT16 spacing = (target->radius >> 1) / target->scale;

			// set respawn fuse
			if (damagetype == DMG_INSTAKILL)
				; // Don't respawn (external)
			else if (gametype == GT_TUTORIAL)
				target->fuse = 5*TICRATE;
			else if (K_CapsuleTimeAttackRules() == true)
				; // Don't respawn (internal)
			else if (target->threshold == KCAPSULE_RING)
				target->fuse = 20*TICRATE;
			else
				target->fuse = 40*TICRATE;

			// burst effects
			for (i = 0; i < 2; i++)
			{
				mobj_t *blast = P_SpawnMobjFromMobj(target, 0, 0, target->info->height >> 1, MT_BATTLEBUMPER_BLAST);
				blast->angle = angle + i*ANGLE_90;
				P_SetScale(blast, 2*blast->scale/3);
				blast->destscale = 2*blast->scale;
			}

			// dust effects
			for (i = 0; i < 10; i++)
			{
				fixed_t rand_x;
				fixed_t rand_y;
				fixed_t rand_z;

				// note: determinate random argument eval order
				rand_z = P_RandomRange(PR_ITEM_DEBRIS, 0, 4*spacing);
				rand_y = P_RandomRange(PR_ITEM_DEBRIS, -spacing, spacing);
				rand_x = P_RandomRange(PR_ITEM_DEBRIS, -spacing, spacing);
				mobj_t *puff = P_SpawnMobjFromMobj(
					target,
					rand_x * FRACUNIT,
					rand_y * FRACUNIT,
					rand_z * FRACUNIT,
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

				// MF2_STRONGBOX: always put the item right in the hotbar!
				if (!(target->flags2 & MF2_STRONGBOX))
				{
					// special behavior for ring capsules
					if (target->threshold == KCAPSULE_RING)
					{
						K_AwardPlayerRings(player, 5 * target->movecount, true);
						break;
					}

					// special behavior for SPB capsules
					if (target->threshold == KITEM_SPB)
					{
						K_ThrowKartItem(player, true, MT_SPB, 1, 0, 0);
						break;
					}
				}

				if (target->threshold < 1 || target->threshold >= NUMKARTITEMS) // bruh moment prevention
				{
					player->itemtype = KITEM_SAD;
					K_SetPlayerItemAmount(player, 1);
				}
				else
				{
					player->itemtype = target->threshold;
					if (K_GetShieldFromItem(player->itemtype) != KSHIELD_NONE) // never give more than 1 shield
						K_SetPlayerItemAmount(player, 1);
					else
						K_SetPlayerItemAmount(player, max(1, target->movecount));
				}
				player->karthud[khud_itemblink] = TICRATE;
				player->karthud[khud_itemblinkmode] = 0;
				K_StopRoulette(&player->itemRoulette);
				if (P_IsDisplayPlayer(player))
					S_StartSound(NULL, sfx_itrolf);
			}
			break;
		}

		case MT_BATTLECAPSULE:
			{
				mobj_t *cur;
				angle_t dir = 0;

				target->fuse = 16;
				target->flags |= MF_NOCLIP|MF_NOCLIPTHING;

				if (inflictor)
				{
					dir = R_PointToAngle2(inflictor->x, inflictor->y, target->x, target->y);
					P_Thrust(target, dir, P_AproxDistance(inflictor->momx, inflictor->momy)/12);
				}
				else if (source)
					dir = R_PointToAngle2(source->x, source->y, target->x, target->y);

				target->momz += 8 * target->scale * P_MobjFlip(target);
				target->flags &= ~MF_NOGRAVITY;

				cur = target->hnext;

				while (cur && !P_MobjWasRemoved(cur))
				{
					cur->momx = target->momx;
					cur->momy = target->momy;
					cur->momz = target->momz;

					// Shoot every piece outward
					if (!(cur->x == target->x && cur->y == target->y))
					{
						P_Thrust(cur,
							R_PointToAngle2(target->x, target->y, cur->x, cur->y),
							R_PointToDist2(target->x, target->y, cur->x, cur->y) / 12
						);
					}

					cur->flags &= ~MF_NOGRAVITY;
					cur->tics = TICRATE;
					cur->frame &= ~FF_ANIMATE; // Stop animating the propellers

					cur->hitlag = target->hitlag;
					cur->eflags |= MFE_DAMAGEHITLAG;

					cur = cur->hnext;
				}

				// Spawn three Followers (if possible)
				if (mapheaderinfo[gamemap-1]->numFollowers)
				{
					dir = FixedAngle(P_RandomKey(PR_RANDOMAUDIENCE, 360)*FRACUNIT);

					const fixed_t launchmomentum = 7 * mapobjectscale;
					const fixed_t jaggedness = 4;
					angle_t launchangle;
					UINT8 i;
					for (i = 0; i < 6; i++, dir += ANG60)
					{
						cur = P_SpawnMobj(
							target->x, target->y,
							target->z + target->height/2,
							MT_RANDOMAUDIENCE
						);

						// We check if you have some horrible Lua
						if (P_MobjWasRemoved(cur))
							break;

						Obj_AudienceInit(cur, NULL, -1);

						// We check again if the list is invalid
						if (P_MobjWasRemoved(cur))
							break;

						cur->hitlag = target->hitlag;

						cur->destscale /= 2;
						P_SetScale(cur, cur->destscale/TICRATE);
						cur->scalespeed = cur->destscale/TICRATE;
						cur->z -= cur->height/2;

						if (source && !P_MobjWasRemoved(source))
						{
							// flags are NOT from the target - just in case it's just been placed on the ceiling as a gimmick
							cur->flags2 |= (source->flags2 & MF2_OBJECTFLIP);
							cur->eflags |= (source->eflags & MFE_VERTICALFLIP);
						}
						else
						{
							// Welp, nothing to be done here
							cur->flags2 |= (target->flags2 & MF2_OBJECTFLIP);
							cur->eflags |= (target->eflags & MFE_VERTICALFLIP);
						}


						launchangle = FixedAngle(
							(
								(
									P_RandomRange(PR_RANDOMAUDIENCE, 12/jaggedness, 24/jaggedness) * jaggedness
								) + (i & 1)*16
							) * FRACUNIT
						);

						cur->momz = P_MobjFlip(target) // THIS one uses target!
							* P_ReturnThrustY(cur, launchangle, launchmomentum);

						cur->angle = dir;

						P_InstaThrust(
							cur, cur->angle,
							P_ReturnThrustX(cur, launchangle, launchmomentum)
						);

						cur->fuse = (3*TICRATE)/2;
						cur->flags |= MF_NOCLIPHEIGHT;
					}
				}

				S_StartSound(target, sfx_mbs60);

				P_AddBrokenPrison(target, inflictor, source);
			}
			break;

		case MT_CDUFO:
			S_StartSound(inflictor, sfx_mbs60);

			target->momz = -(3*mapobjectscale)/2;
			target->fuse = 2*TICRATE;

			P_AddBrokenPrison(target, inflictor, source);
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

				target->momz += (18 * target->scale) * P_MobjFlip(target);
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

		case MT_BANANA:
		case MT_BANANA_SHIELD:
		{
			const UINT8 numParticles = 8;
			const angle_t diff = ANGLE_MAX / numParticles;
			UINT8 i;

			for (i = 0; i < numParticles; i++)
			{
				mobj_t *spark = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_BANANA_SPARK);
				spark->angle = (diff * i) - (diff / 2);

				if (inflictor != NULL && P_MobjWasRemoved(inflictor) == false)
				{
					spark->angle += K_MomentumAngle(inflictor);
					spark->momx += inflictor->momx / 2;
					spark->momy += inflictor->momy / 2;
					spark->momz += inflictor->momz / 2;
				}

				P_SetObjectMomZ(spark, (12 + P_RandomRange(PR_DECORATION, -4, 4)) * FRACUNIT, true);
				P_Thrust(spark, spark->angle, (12 + P_RandomRange(PR_DECORATION, -4, 4)) * spark->scale);
			}
			break;
		}

		case MT_MONITOR:
			Obj_MonitorOnDeath(target, source);
			break;
		case MT_BATTLEUFO:
			Obj_BattleUFODeath(target, inflictor);
			break;
		case MT_BLENDEYE_MAIN:
			VS_BlendEye_Death(target);
			break;
		case MT_BLENDEYE_GLASS:
			VS_BlendEye_Glass_Death(target);
			break;
		case MT_BLENDEYE_PUYO:
			VS_PuyoDeath(target);
			break;
		case MT_EMFAUCET_DRIP:
			Obj_EMZDripDeath(target);
			break;
		case MT_FLYBOT767:
			Obj_FlybotDeath(target);
			break;
		case MT_ANCIENTGEAR:
			Obj_AncientGearDeath(target, source);
			break;
		default:
			break;
	}

	if ((target->type == MT_JAWZ || target->type == MT_JAWZ_SHIELD) && !(target->flags2 & MF2_AMBUSH))
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

		momz = 7*scale;
		if (flip)
			momz *= -1;

		chunk = P_SpawnMobjFromMobj(target, 0, 0, 0, MT_SPIKE);
		P_SetMobjState(chunk, target->info->deathstate);
		chunk->health = 0;
		chunk->angle = ang + ANGLE_180;
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
			sprflip = P_RandomChance(PR_DECORATION, FRACUNIT/2);

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
			chunk->momz = P_RandomRange(PR_DECORATION, 5, 7)*scale;\
			if (flip)\
				chunk->momz *= -1;\
			if (sprflip)\
				chunk->frame |= FF_VERTICALFLIP

			makechunk(ang + ANGLE_180, -xoffs, -yoffs);
			sprflip = !sprflip;
			makechunk(ang, xoffs, yoffs);

#undef makechunk
		}

		sprflip = P_RandomChance(PR_DECORATION, FRACUNIT/2);

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
		chunk->momz = P_RandomRange(PR_DECORATION, 5, 7)*scale;
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
		target->momz = P_RandomRange(PR_DECORATION, 5, 7)*scale;
		if (flip)
			target->momz *= -1;
		if (!sprflip)
			target->frame |= FF_VERTICALFLIP;
	}
	else if (target->type == MT_BLENDEYE_GENERATOR && !P_MobjWasRemoved(inflictor))
	{
		mobj_t *refobj = (inflictor->type == MT_INSTAWHIP) ? source : inflictor;
		angle_t impactangle = R_PointToAngle2(target->x, target->y, refobj->x - refobj->momx, refobj->y - refobj->momy) - (target->angle + ANGLE_90);

		if (P_MobjWasRemoved(target->tracer) == false)
		{
			target->tracer->flags2 &= ~MF2_FRET;
			target->tracer->flags |= MF_SHOOTABLE;
			P_DamageMobj(target->tracer, inflictor, source, 1, DMG_NORMAL);
			target->tracer->flags &= ~MF_SHOOTABLE;
		}

		P_SetMobjState(
			target,
			((impactangle < ANGLE_180)
				? target->info->deathstate
				: target->info->xdeathstate
			)
		);
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

#if 0
		// Don't hurt your team, either!
		if (G_SameTeam(source->player, target->player) == true)
			return false;
#endif
	}

	return true;
}

static boolean P_KillPlayer(player_t *player, mobj_t *inflictor, mobj_t *source, UINT8 type)
{
	(void)inflictor;
	(void)source;

	const boolean beforeexit = !(player->exiting || (player->pflags & PF_NOCONTEST));

	if (type == DMG_SPECTATOR && (G_GametypeHasTeams() || G_GametypeHasSpectators()))
	{
		P_SetPlayerSpectator(player-players);
	}
	else
	{
		// DMG_TIMEOVER: player explosion
		if (player->respawn.state != RESPAWNST_NONE && type != DMG_TIMEOVER)
		{
			K_DoInstashield(player);
			return false;
		}

		if (player->exiting == false && specialstageinfo.valid == true)
		{
			if (type == DMG_DEATHPIT)
			{
				HU_DoTitlecardCEcho(player, "FALL OUT!", false);
			}

			// This must be done before the condition to set
			// destscale = 1, so any special stage death
			// shrinks the player to a speck.
			P_DoPlayerExit(player, PF_NOCONTEST);
		}

		if (player->exiting && type == DMG_DEATHPIT)
		{
			// If the player already finished the race, and
			// they fall into a death pit afterward, their
			// body shrinks into nothingness.
			player->mo->destscale = 1;
			player->mo->flags |= MF_NOCLIPTHING;
			player->tumbleBounces = 0;

			return false;
		}

		if (modeattacking & ATTACKING_SPB)
		{
			// Death in SPB Attack is an instant loss.
			P_DoPlayerExit(player, PF_NOCONTEST);
		}
	}

	switch (type)
	{
		case DMG_DEATHPIT:
			// Fell off the stage
			if (player->roundconditions.fell_off == false
				&& beforeexit == true)
			{
				player->roundconditions.fell_off = true;
				player->roundconditions.checkthisframe = true;
			}

			if ((player->pitblame > -1) && (player->pitblame < MAXPLAYERS)
				&& (playeringame[player->pitblame]) && (!players[player->pitblame].spectator)
				&& (players[player->pitblame].mo) && (!P_MobjWasRemoved(players[player->pitblame].mo)))
			{
				if (gametyperules & (GTR_BUMPERS|GTR_CHECKPOINTS))
					P_DamageMobj(player->mo, players[player->pitblame].mo, players[player->pitblame].mo, 1, DMG_KARMA);
				else
					K_SpawnAmps(&players[player->pitblame], 20, player->mo);
				player->pitblame = -1;
			}
			else if (player->mo->health > 1 || K_Cooperative())
			{
				if (gametyperules & (GTR_BUMPERS|GTR_CHECKPOINTS))
					player->mo->health--;

			}

			if (modeattacking & ATTACKING_SPB)
			{
				return true;
			}

			if (player->mo->health <= 0)
			{
				return true;
			}

			// Quick respawn; does not kill
			return K_DoIngameRespawn(player), false;

		case DMG_SPECTATOR:
			// disappearifies, but still gotta put items back in play
			break;

		case DMG_TIMEOVER:
			player->pflags |= PF_ELIMINATED;
			//FALLTHRU
		default:
			// Everything else REALLY kills
			if (leveltime < starttime)
			{
				K_DoFault(player);
			}
			break;
	}

	return true;
}

static void AddTimesHit(player_t *player)
{
	const INT32 oldtimeshit = player->timeshit;

	player->timeshit++;

	// overflow prevention
	if (player->timeshit < oldtimeshit)
	{
		player->timeshit = oldtimeshit;
	}
}

static void AddNullHitlag(player_t *player, tic_t oldHitlag)
{
	if (player == NULL)
	{
		return;
	}

	// Hitlag from what would normally be damage but the
	// player was invulnerable.
	//
	// If we're constantly getting hit the same number of
	// times, we're probably standing on a damage floor.
	//
	// Checking if we're hit more than before ensures that:
	//
	// 1) repeating damage doesn't count
	// 2) new damage sources still count

	if (player->timeshit <= player->timeshitprev || player->hyudorotimer > 0)
	{
		player->nullHitlag += (player->mo->hitlag - oldHitlag);
	}
}

static boolean P_FlashingException(const player_t *player, const mobj_t *inflictor)
{
	if (!inflictor)
	{
		// Sector damage always behaves the same.
		return false;
	}

	if (inflictor->type == MT_SSMINE)
	{
		// Mine's first hit is DMG_EXPLODE.
		// Afterward, it leaves a spinout hitbox which remains for a short period.
		// If the spinout hitbox ignored flashing tics, you would be combod every tic and die instantly.
		// DMG_EXPLODE already ignores flashing tics (correct behavior).
		return false;
	}

	if (inflictor->type == MT_SPB)
	{
		// The SPB does not die on impact with players other than its intended target.
		// Ignoring flashing tics would cause an endless combo on anyone who gets in way of the SPB.
		// Upon hitting its target, DMG_EXPLODE will be used (which ignores flashing tics).
		return false;
	}

	if (!P_IsKartItem(inflictor->type) && inflictor->type != MT_PLAYER)
	{
		// Exception only applies to player items.
		// Also applies to players because of PvP collision.
		// Lightning Shield also uses the player object as inflictor.
		return false;
	}

	if (!P_PlayerInPain(player))
	{
		// Flashing tics is sometimes used in a way unrelated to damage.
		// E.g. picking up a power-up gives you flashing tics.
		// Respect this usage of flashing tics.
		return false;
	}

	// Flashing tics are ignored.
	return true;
}

// P_DamageMobj for 0x0010 compat.
// I know this sucks ass, but this function is legitimately too complicated to add more behavior switches.
static boolean P_DamageMobjCompat(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	player_t *player;
	player_t *playerInflictor;
	boolean force = false;
	boolean spbpop = false;
	boolean downgraded = false;

	INT32 laglength = 6;

	if (objectplacing)
		return false;

	if (target->health <= 0)
		return false;

	// Spectator handling
	if (damagetype != DMG_SPECTATOR && target->player && target->player->spectator)
		return false;

	// source is checked without a removal guard in so many places that it's genuinely less work to do it here.
	if (source && P_MobjWasRemoved(source))
		source = NULL;

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

	switch (target->type)
	{
		case MT_MONITOR:
			damage = Obj_MonitorGetDamage(target, inflictor, damagetype);
			Obj_MonitorOnDamage(target, inflictor, damage);
			break;
		case MT_CDUFO:
			// Make it possible to pick them up during race
			if (inflictor->type == MT_ORBINAUT_SHIELD || inflictor->type == MT_JAWZ_SHIELD)
				return false;
			break;

		case MT_SPB:
			spbpop = (damagetype & DMG_TYPEMASK) == DMG_VOLTAGE;
			if (spbpop && source && source->player
				&& source->player->roundconditions.spb_neuter == false)
			{
				source->player->roundconditions.spb_neuter = true;
				source->player->roundconditions.checkthisframe = true;
			}
			break;

		default:
			break;
	}

	// Everything above here can't be forced.
	{
		UINT8 shouldForce = LUA_HookShouldDamage(target, inflictor, source, damage, damagetype);
		if (P_MobjWasRemoved(target))
			return (shouldForce == 1); // mobj was removed
		if (shouldForce == 1)
			force = true;
		else if (shouldForce == 2)
			return false;
	}

	switch (target->type)
	{
		case MT_BALLSWITCH_BALL:
			Obj_BallSwitchDamaged(target, inflictor, source);
			return false;

		case MT_SA2_CRATE:
		case MT_ICECAPBLOCK:
			return Obj_TryCrateDamage(target, inflictor);

		case MT_KART_LEFTOVER:
			// intangible (do not let instawhip shred damage)
			if (Obj_DestroyKart(target))
				return false;

			P_SetObjectMomZ(target, 12*FRACUNIT, false);
			break;

		default:
			break;
	}

	if (!force)
	{
		if (!spbpop)
		{
			if (!(target->flags & MF_SHOOTABLE))
				return false; // shouldn't happen...
		}
	}

	if (target->flags2 & MF2_SKULLFLY)
		target->momx = target->momy = target->momz = 0;

	if (target->flags & (MF_ENEMY|MF_BOSS))
	{
		if (!force && target->flags2 & MF2_FRET) // Currently flashing from being hit
			return false;

		if (LUA_HookMobjDamage(target, inflictor, source, damage, damagetype) || P_MobjWasRemoved(target))
			return true;

		if (target->health > 1)
			target->flags2 |= MF2_FRET;
	}

	player = target->player;
	playerInflictor = inflictor ? inflictor->player : NULL;

	if (playerInflictor)
	{
		AddTimesHit(playerInflictor);
	}

	if (player) // Player is the target
	{
		AddTimesHit(player);

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

		if (source && source->player)
		{
			if (source->player->roundconditions.hit_midair == false
				&& source != target
				&& inflictor
				&& K_IsMissileOrKartItem(inflictor)
				&& target->player->airtime > TICRATE/2
				&& source->player->airtime > TICRATE/2)
			{
				source->player->roundconditions.hit_midair = true;
				source->player->roundconditions.checkthisframe = true;
			}

			if (source->player->roundconditions.hit_drafter_lookback == false
				&& source != target
				&& target->player->lastdraft == (source->player - players)
				&& (K_GetKartButtons(source->player) & BT_LOOKBACK) == BT_LOOKBACK
				/*&& (AngleDelta(K_MomentumAngle(source), R_PointToAngle2(source->x, source->y, target->x, target->y)) > ANGLE_90)*/)
			{
				source->player->roundconditions.hit_drafter_lookback = true;
				source->player->roundconditions.checkthisframe = true;
			}

			if (source->player->roundconditions.giant_foe_shrunken_orbi == false
				&& source != target
				&& player->growshrinktimer > 0
				&& !P_MobjWasRemoved(inflictor)
				&& inflictor->type == MT_ORBINAUT
				&& inflictor->scale < FixedMul((FRACUNIT + SHRINK_SCALE), mapobjectscale * 2)) // halfway between base scale and shrink scale, a little bit of leeway
			{
				source->player->roundconditions.giant_foe_shrunken_orbi = true;
				source->player->roundconditions.checkthisframe = true;
			}

			if (source == target
				&& !P_MobjWasRemoved(inflictor)
				&& inflictor->type == MT_SPBEXPLOSION
				&& inflictor->threshold == KITEM_EGGMAN
				&& !P_MobjWasRemoved(inflictor->tracer)
				&& inflictor->tracer != source
				&& inflictor->tracer->player
				&& inflictor->tracer->player->roundconditions.returntosender_mark == false)
			{
				inflictor->tracer->player->roundconditions.returntosender_mark = true;
				inflictor->tracer->player->roundconditions.checkthisframe = true;
			}
		}
		else if (!(inflictor && inflictor->player)
			&& !(player->exiting || player->laps > numlaps)
			&& damagetype != DMG_DEATHPIT)
		{
			// laps will never increment outside of GTR_CIRCUIT, so this is still fine
			const UINT8 requiredbit = 1<<(player->laps & 7);

			if (!(player->roundconditions.hittrackhazard[player->laps/8] & requiredbit))
			{
				player->roundconditions.hittrackhazard[player->laps/8] |= requiredbit;
				player->roundconditions.checkthisframe = true;
			}
		}

		// Instant-Death
		if ((damagetype & DMG_DEATHMASK))
		{
			if (!P_KillPlayer(player, inflictor, source, damagetype))
				return false;
		}
		else if (LUA_HookMobjDamage(target, inflictor, source, damage, damagetype))
		{
			return true;
		}
		else
		{
			UINT8 type = (damagetype & DMG_TYPEMASK);
			const boolean hardhit = (type == DMG_EXPLODE || type == DMG_KARMA || type == DMG_TUMBLE); // This damage type can do evil stuff like ALWAYS combo
			INT16 ringburst = 5;

			// Check if the player is allowed to be damaged!
			// If not, then spawn the instashield effect instead.
			if (!force)
			{
				boolean invincible = true;
				boolean clash = false;
				sfxenum_t sfx = sfx_None;

				if (!(gametyperules & GTR_BUMPERS))
				{
					if (damagetype & DMG_STEAL)
					{
						// Gametype does not have bumpers, steal damage is intended to not do anything
						// (No instashield is intentional)
						return false;
					}
				}

				if (player->invincibilitytimer > 0)
				{
					sfx = sfx_invind;
				}
				else if (K_IsBigger(target, inflictor) == true &&
					// SPB bypasses grow (K_IsBigger handles NULL check)
					(type != DMG_EXPLODE || inflictor->type != MT_SPBEXPLOSION || !inflictor->movefactor))
				{
					sfx = sfx_grownd;
				}
				else if (K_PlayerGuard(player))
				{
					sfx = sfx_s3k3a;
					clash = true;
				}
				else if (player->overshield &&
					(type != DMG_EXPLODE || inflictor->type != MT_SPBEXPLOSION || !inflictor->movefactor))
				{
					clash = true;
				}
				else if (player->hyudorotimer > 0)
					;
				else
				{
					invincible = false;
				}

				// Hack for instawhip-guard counter, lets invincible players lose to guard
				if (inflictor == target)
				{
					invincible = false;
				}

				if (player->pflags2 & PF2_ALWAYSDAMAGED)
				{
					invincible = false;
					clash = false;
				}

				// TODO: doing this from P_DamageMobj limits punting to objects that damage the player.
				// And it may be kind of yucky.
				// But this is easier than accounting for every condition in PIT_CheckThing!
				if (inflictor && K_PuntCollide(inflictor, target))
				{
					return false;
				}

				if (invincible && type != DMG_WHUMBLE)
				{
					const INT32 oldHitlag = target->hitlag;
					const INT32 oldHitlagInflictor = inflictor ? inflictor->hitlag : 0;

					// Damage during hitlag should be a no-op
					// for invincibility states because there
					// are no flashing tics. If the damage is
					// from a constant source, a deadlock
					// would occur.

					if (target->eflags & MFE_PAUSED)
					{
						player->timeshit--; // doesn't count

						if (playerInflictor)
						{
							playerInflictor->timeshit--;
						}

						return false;
					}

					laglength = max(laglength / 2, 1);
					K_SetHitLagForObjects(target, inflictor, source, laglength, false);

					AddNullHitlag(player, oldHitlag);
					AddNullHitlag(playerInflictor, oldHitlagInflictor);

					if (player->timeshit > player->timeshitprev)
					{
						S_StartSound(target, sfx);
					}

					if (clash)
					{
						player->spheres = max(player->spheres - 5, 0);

						if (inflictor)
						{
							K_DoPowerClash(target, inflictor);

							if (inflictor->type == MT_SUPER_FLICKY)
							{
								Obj_BlockSuperFlicky(inflictor);
							}
						}
						else if (source)
							K_DoPowerClash(target, source);
					}

					// Full invulnerability
					K_DoInstashield(player);
					return false;
				}
				{
					// Check if we should allow wombo combos (hard hits by default, inverted by the presence of DMG_WOMBO).
					boolean allowcombo = ((hardhit || (type == DMG_STUMBLE || type == DMG_WHUMBLE)) == !(damagetype & DMG_WOMBO));

					// Tumble/stumble is a special case.
					if (type == DMG_TUMBLE)
					{
						// don't allow constant combo
						if (player->tumbleBounces == 1 && (P_MobjFlip(target)*target->momz > 0))
							allowcombo = false;
					}
					else if (type == DMG_STUMBLE || type == DMG_WHUMBLE)
					{
						// don't allow constant combo
						if (player->tumbleBounces == TUMBLEBOUNCES-1 && (P_MobjFlip(target)*target->momz > 0))
						{
							if (type == DMG_STUMBLE)
								return false; // No-sell strings of stumble

							allowcombo = false;
						}
					}

					if (inflictor && !P_MobjWasRemoved(inflictor) && inflictor->momx == 0 && inflictor->momy == 0 && inflictor->momz == 0)
					{
						// Probably a map hazard.
						allowcombo = false;
					}

					if (allowcombo == false && (target->eflags & MFE_PAUSED))
					{
						return false;
					}

					// DMG_EXPLODE excluded from flashtic checks to prevent dodging eggbox/SPB with weak spinout
					if ((target->hitlag == 0 || allowcombo == false) &&
						player->flashing > 0 &&
						type != DMG_EXPLODE &&
						type != DMG_STUMBLE &&
						type != DMG_WHUMBLE &&
						P_FlashingException(player, inflictor) == false)
					{
						// Post-hit invincibility
						K_DoInstashield(player);
						return false;
					}
					else if (target->flags2 & MF2_ALREADYHIT) // do not deal extra damage in the same tic
					{
						K_SetHitLagForObjects(target, inflictor, source, laglength, true);
						return false;
					}
				}
			}

			if (gametyperules & GTR_BUMPERS)
			{
				if (damagetype & DMG_STEAL)
				{
					// Steals 2 bumpers
					damage = 2;
				}
			}
			else
			{
				// Do not die from damage outside of bumpers health system
				damage = 0;
			}

			boolean softenTumble = false;

			// Sting and stumble shouldn't be rewarding Battle hits.
			if (type == DMG_STING || type == DMG_STUMBLE)
			{
				damage = 0;

				if (source && source != player->mo && source->player)
				{
					if (!P_PlayerInPain(player) && (player->defenseLockout || player->instaWhipCharge))
					{
						K_SpawnAmps(source->player, 20, target);
					}
				}
			}
			else
			{
				// We successfully damaged them! Give 'em some bumpers!

				if (source && source != player->mo && source->player)
				{
					// Stone Shoe handles amps on its own, but this is also a good place to set soften tumble for it
					if (inflictor->type == MT_STONESHOE || inflictor->type == MT_STONESHOE_CHAIN)
						softenTumble = true;
					else
						K_SpawnAmps(source->player, K_PvPAmpReward((type == DMG_WHUMBLE) ? 30 : 20, source->player, player), target);


					K_BotHitPenalty(player);

					if (G_SameTeam(source->player, player))
					{
						if (type != DMG_EXPLODE)
						{
							type = DMG_STUMBLE;
							downgraded = true;
						}
					}
					else
					{
						for (UINT8 i = 0; i < MAXPLAYERS; i++)
						{
							if (!playeringame[i] || players[i].spectator || !players[i].mo || P_MobjWasRemoved(players[i].mo))
								continue;
							if (!G_SameTeam(source->player, &players[i]))
								continue;
							if (source->player == &players[i])
								continue;
							K_SpawnAmps(&players[i], FixedInt(FixedMul(5, K_TeamComebackMultiplier(player))), target);
						}
					}


					// Extend the invincibility if the hit was a direct hit.
					if (inflictor == source && source->player->invincibilitytimer &&
							!K_PowerUpRemaining(source->player, POWERUP_SMONITOR))
					{
						tic_t kinvextend;

						softenTumble = true;

						if (gametyperules & GTR_CLOSERPLAYERS)
							kinvextend = 2*TICRATE;
						else
							kinvextend = 3*TICRATE;

						// Reduce the value of subsequent invinc extensions
						kinvextend = kinvextend / (1 + source->player->invincibilityextensions); // 50%, 33%, 25%[...]
						kinvextend = max(kinvextend, TICRATE);

						source->player->invincibilityextensions++;

						source->player->invincibilitytimer += kinvextend;

						if (P_IsDisplayPlayer(source->player))
							S_StartSound(NULL, sfx_gsha7);
					}

					// if the inflictor is a landmine, its reactiontime will be non-zero if it is still moving
					if (inflictor->type == MT_LANDMINE && inflictor->reactiontime > 0)
					{
						// reduce tumble severity to account for getting beaned point blank sometimes
						softenTumble = true;
						// make it more consistent with set landmines
						inflictor->momx = 0;
						inflictor->momy = 0;
					}

					K_TryHurtSoundExchange(target, source);

					if (K_Cooperative() == false)
					{
						K_BattleAwardHit(source->player, player, inflictor, damage);
					}

					if (K_Bumpers(source->player) < K_StartingBumperCount() || (damagetype & DMG_STEAL))
					{
						K_TakeBumpersFromPlayer(source->player, player, damage);
					}

					if (damagetype & DMG_STEAL)
					{
						// Give them ALL of your emeralds instantly :)
						source->player->emeralds |= player->emeralds;
						player->emeralds = 0;
						K_CheckEmeralds(source->player);
					}
				}

				if (!(damagetype & DMG_STEAL))
				{
					// Drop all of your emeralds
					K_DropEmeraldsFromPlayer(player, player->emeralds);
				}
			}

			if (source && source != player->mo && source->player)
			{
				if (damagetype != DMG_DEATHPIT)
				{
					player->pitblame = source->player - players;
				}
			}

			player->sneakertimer = player->numsneakers = 0;
			player->panelsneakertimer = player->numpanelsneakers = 0;
			player->weaksneakertimer = player->numweaksneakers = 0;
			player->driftboost = player->strongdriftboost = 0;
			player->gateBoost = 0;
			player->fastfall = 0;
			player->ringboost = 0;
			player->glanceDir = 0;
			player->preventfailsafe = TICRATE*3;
			player->pflags &= ~PF_GAINAX;
			Obj_EndBungee(player);
			K_BumperInflate(target->player);

			UINT32 hurtskinflags = (demo.playback)
					? demo.skinlist[demo.currentskinid[(player-players)]].flags
					: skins[player->skin]->flags;
			if (hurtskinflags & SF_IRONMAN)
			{
				if (gametyperules & GTR_BUMPERS)
					SetRandomFakePlayerSkin(player, false, true);
			}

			// Explosions are explicit combo setups.
			if (damagetype & DMG_EXPLODE)
				player->bumperinflate = 0;

			if (player->spectator == false && !(player->charflags & SF_IRONMAN))
			{
				UINT32 skinflags = (demo.playback)
					? demo.skinlist[demo.currentskinid[(player-players)]].flags
					: skins[player->skin]->flags;

				if (skinflags & SF_IRONMAN)
				{
					player->mo->skin = skins[player->skin];
					player->charflags = skinflags;
					K_SpawnMagicianParticles(player->mo, 5);
				}
			}

			if (player->rings <= -20)
			{
				player->markedfordeath = true;
				damagetype = DMG_TUMBLE;
				type = DMG_TUMBLE;
				P_StartQuakeFromMobj(5, 44 * player->mo->scale, 2560 * player->mo->scale, player->mo);
				//P_KillPlayer(player, inflictor, source, damagetype);
			}

			// Death save! On your last hit, no matter what, demote to weakest damage type for one last escape chance.
			if (player->mo->health == 2 && damage && gametyperules & GTR_BUMPERS)
			{
				K_AddMessageForPlayer(player, "\x8DLast Chance!", false, false);
				S_StartSound(target, sfx_gshc7);
				player->flashing = TICRATE;
				type = DMG_STUMBLE;
			}

			if (inflictor && !P_MobjWasRemoved(inflictor) && P_IsKartItem(inflictor->type) && inflictor->cvmem
				&& inflictor->type != MT_BANANA) // Are there other designed trap items that can be deployed and dropped? If you add one, list it here!
			{
				type = DMG_STUMBLE;
				downgraded = true;
				player->ringburst += 5; // IT'S THE DAMAGE STUMBLE HACK AGAIN AAAAAAAAHHHHHHHHHHH
				K_PopPlayerShield(player);
			}

			if (!(gametyperules & GTR_SPHERES) && player->tripwireLeniency && !P_PlayerInPain(player))
			{
				switch (type)
				{
					case DMG_EXPLODE:
						type = DMG_TUMBLE;
						downgraded = true;
						softenTumble = true;
						break;
					case DMG_TUMBLE:
						softenTumble = true;
						break;
					case DMG_NORMAL:
					case DMG_WIPEOUT:
						downgraded = true;
						type = DMG_STUMBLE;
						player->ringburst += 5; // THERE IS SIMPLY NO HOPE AT THIS POINT
						K_PopPlayerShield(player);
						break;
					default:
						break;
				}
			}

			switch (type)
			{
				case DMG_STING:
					K_DebtStingPlayer(player, source);
					K_KartPainEnergyFling(player);
					ringburst = 0;
					break;
				case DMG_STUMBLE:
				case DMG_WHUMBLE:
					K_StumblePlayer(player);
					ringburst = 5;
					break;
				case DMG_TUMBLE:
					K_TumblePlayer(player, inflictor, source, softenTumble);
					ringburst = 10;
					break;
				case DMG_EXPLODE:
				case DMG_KARMA:
					ringburst = K_ExplodePlayer(player, inflictor, source);
					break;
				case DMG_WIPEOUT:
					K_SpinPlayer(player, inflictor, source, KSPIN_WIPEOUT);
					K_KartPainEnergyFling(player);
					break;
				case DMG_VOLTAGE:
				case DMG_NORMAL:
				default:
					K_SpinPlayer(player, inflictor, source, KSPIN_SPINOUT);
					break;
			}

			// Have a shield? You get hit, but don't lose your rings!
			if (player->curshield != KSHIELD_NONE)
			{
				ringburst = 0;
			}

			player->ringburst += ringburst;

			K_PopPlayerShield(player);

			if ((type != DMG_STUMBLE && type != DMG_WHUMBLE) || (type == DMG_STUMBLE && downgraded))
			{
				if (type != DMG_STING)
					player->flashing = K_GetKartFlashing(player);
				player->instashield = 15;
			}

			K_PlayPainSound(target, source);

			if (gametyperules & GTR_BUMPERS)
				player->spheres = min(player->spheres + 10, 40);

			if ((hardhit == true && !softenTumble) || cv_kartdebughuddrop.value)
			{
				K_DropItems(player);
			}
			else
			{
				K_DropHnextList(player);
			}

			if (inflictor && !P_MobjWasRemoved(inflictor) && inflictor->type == MT_BANANA)
			{
				player->flipDI = true;
			}

			// Apply stun!
			if (type != DMG_STING)
			{
				K_ApplyStun(player, inflictor, source, damage, damagetype);
			}

			K_DefensiveOverdrive(target->player);
		}
	}
	else
	{
		if (target->type == MT_SPECIAL_UFO)
		{
			return Obj_SpecialUFODamage(target, inflictor, source, damagetype);
		}
		else if (target->type == MT_BLENDEYE_MAIN)
		{
			VS_BlendEye_Damage(target, inflictor, source, damage);
		}

		if (damagetype & DMG_STEAL)
		{
			// Not a player, steal damage is intended to not do anything
			return false;
		}

		if ((target->flags & MF_BOSS) == MF_BOSS)
		{
			targetdamaging_t targetdamaging = UFOD_GENERIC;
			if (P_MobjWasRemoved(inflictor) == true)
				;
			else switch (inflictor->type)
			{
				case MT_GACHABOM:
					targetdamaging = UFOD_GACHABOM;
					break;
				case MT_ORBINAUT:
				case MT_ORBINAUT_SHIELD:
					targetdamaging = UFOD_ORBINAUT;
					break;
				case MT_BANANA:
					targetdamaging = UFOD_BANANA;
					break;
				case MT_INSTAWHIP:
					inflictor->extravalue2 = 1; // Disable whip collision
					targetdamaging = UFOD_WHIP;
					break;
				case MT_PLAYER:
					targetdamaging = UFOD_BOOST;
					break;
				case MT_JAWZ:
				case MT_JAWZ_SHIELD:
					targetdamaging = UFOD_JAWZ;
					break;
				case MT_SPB:
					targetdamaging = UFOD_SPB;
					break;
				default:
					break;
			}

			P_TrackRoundConditionTargetDamage(targetdamaging);
		}
	}

	// do the damage
	if (damagetype & DMG_DEATHMASK)
		target->health = 0;
	else
		target->health -= damage;

	if (source && source->player && target)
		G_GhostAddHit((INT32) (source->player - players), target);

	// Insta-Whip (DMG_WHUMBLE): do not reduce hitlag because
	// this can leave room for double-damage.
	if ((damagetype & DMG_TYPEMASK) != DMG_WHUMBLE && (gametyperules & GTR_BUMPERS) && !battleprisons)
		laglength /= 2;

	if (!(target->player && (damagetype & DMG_DEATHMASK)))
		K_SetHitLagForObjects(target, inflictor, source, laglength, true);

	target->flags2 |= MF2_ALREADYHIT;

	if (target->health <= 0)
	{
		P_KillMobj(target, inflictor, source, damagetype);
		return true;
	}

	//K_SetHitLagForObjects(target, inflictor, source, laglength, true);

	if (!player)
	{
		P_SetMobjState(target, target->info->painstate);

		if (!P_MobjWasRemoved(target))
		{
			// if not intent on another player,
			// chase after this one
			P_SetTarget(&target->target, source);
		}
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
	if (G_CompatLevel(0x0010))
		return P_DamageMobjCompat(target, inflictor, source, damage, damagetype);

	player_t *player;
	player_t *playerInflictor;
	boolean force = false;
	boolean spbpop = false;
	ATTRUNUSED boolean downgraded = false;
	boolean truewhumble = false; // Invincibility-ignoring DMG_WHUMBLE from the Insta-Whip itself.

	INT32 laglength = 6;

	if (objectplacing)
		return false;

	if (target->health <= 0)
		return false;

	// Spectator handling
	if (damagetype != DMG_SPECTATOR && target->player && target->player->spectator)
		return false;

	// source is checked without a removal guard in so many places that it's genuinely less work to do it here.
	if (source && P_MobjWasRemoved(source))
		source = NULL;

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

	switch (target->type)
	{
		case MT_MONITOR:
			damage = Obj_MonitorGetDamage(target, inflictor, damagetype);
			Obj_MonitorOnDamage(target, inflictor, damage);
			break;
		case MT_CDUFO:
			// Make it possible to pick them up during race
			if (inflictor->type == MT_ORBINAUT_SHIELD || inflictor->type == MT_JAWZ_SHIELD)
				return false;
			break;

		case MT_SPB:
			spbpop = (damagetype & DMG_TYPEMASK) == DMG_VOLTAGE;
			if (spbpop && source && source->player
				&& source->player->roundconditions.spb_neuter == false)
			{
				source->player->roundconditions.spb_neuter = true;
				source->player->roundconditions.checkthisframe = true;
			}
			break;

		default:
			break;
	}

	// Everything above here can't be forced.
	{
		UINT8 shouldForce = LUA_HookShouldDamage(target, inflictor, source, damage, damagetype);
		if (P_MobjWasRemoved(target))
			return (shouldForce == 1); // mobj was removed
		if (shouldForce == 1)
			force = true;
		else if (shouldForce == 2)
			return false;
	}

	switch (target->type)
	{
		case MT_BALLSWITCH_BALL:
			Obj_BallSwitchDamaged(target, inflictor, source);
			return false;

		case MT_SA2_CRATE:
		case MT_ICECAPBLOCK:
			return Obj_TryCrateDamage(target, inflictor);

		case MT_KART_LEFTOVER:
			// intangible (do not let instawhip shred damage)
			if (Obj_DestroyKart(target))
				return false;

			P_SetObjectMomZ(target, 12*FRACUNIT, false);
			break;

		default:
			break;
	}

	if (!force)
	{
		if (!spbpop)
		{
			if (!(target->flags & MF_SHOOTABLE))
				return false; // shouldn't happen...
		}
	}

	if (target->flags2 & MF2_SKULLFLY)
		target->momx = target->momy = target->momz = 0;

	if (target->flags & (MF_ENEMY|MF_BOSS))
	{
		if (!force && target->flags2 & MF2_FRET) // Currently flashing from being hit
			return false;

		if (LUA_HookMobjDamage(target, inflictor, source, damage, damagetype) || P_MobjWasRemoved(target))
			return true;

		if (target->health > 1)
			target->flags2 |= MF2_FRET;
	}

	player = target->player;
	playerInflictor = inflictor ? inflictor->player : NULL;

	if (playerInflictor)
	{
		AddTimesHit(playerInflictor);
	}

	if (player) // Player is the target
	{
		AddTimesHit(player);

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

		if (source && source->player)
		{
			if (source->player->roundconditions.hit_midair == false
				&& source != target
				&& inflictor
				&& K_IsMissileOrKartItem(inflictor)
				&& target->player->airtime > TICRATE/2
				&& source->player->airtime > TICRATE/2)
			{
				source->player->roundconditions.hit_midair = true;
				source->player->roundconditions.checkthisframe = true;
			}

			if (source->player->roundconditions.hit_drafter_lookback == false
				&& source != target
				&& target->player->lastdraft == (source->player - players)
				&& (K_GetKartButtons(source->player) & BT_LOOKBACK) == BT_LOOKBACK
				/*&& (AngleDelta(K_MomentumAngle(source), R_PointToAngle2(source->x, source->y, target->x, target->y)) > ANGLE_90)*/)
			{
				source->player->roundconditions.hit_drafter_lookback = true;
				source->player->roundconditions.checkthisframe = true;
			}

			if (source->player->roundconditions.giant_foe_shrunken_orbi == false
				&& source != target
				&& player->growshrinktimer > 0
				&& !P_MobjWasRemoved(inflictor)
				&& inflictor->type == MT_ORBINAUT
				&& inflictor->scale < FixedMul((FRACUNIT + SHRINK_SCALE), mapobjectscale * 2)) // halfway between base scale and shrink scale, a little bit of leeway
			{
				source->player->roundconditions.giant_foe_shrunken_orbi = true;
				source->player->roundconditions.checkthisframe = true;
			}

			if (source == target
				&& !P_MobjWasRemoved(inflictor)
				&& inflictor->type == MT_SPBEXPLOSION
				&& inflictor->threshold == KITEM_EGGMAN
				&& !P_MobjWasRemoved(inflictor->tracer)
				&& inflictor->tracer != source
				&& inflictor->tracer->player
				&& inflictor->tracer->player->roundconditions.returntosender_mark == false)
			{
				inflictor->tracer->player->roundconditions.returntosender_mark = true;
				inflictor->tracer->player->roundconditions.checkthisframe = true;
			}
		}
		else if (!(inflictor && inflictor->player)
			&& !(player->exiting || player->laps > numlaps)
			&& damagetype != DMG_DEATHPIT)
		{
			// laps will never increment outside of GTR_CIRCUIT, so this is still fine
			const UINT8 requiredbit = 1<<(player->laps & 7);

			if (!(player->roundconditions.hittrackhazard[player->laps/8] & requiredbit))
			{
				player->roundconditions.hittrackhazard[player->laps/8] |= requiredbit;
				player->roundconditions.checkthisframe = true;
			}
		}

		// Instant-Death
		if ((damagetype & DMG_DEATHMASK))
		{
			if (!P_KillPlayer(player, inflictor, source, damagetype))
				return false;
		}
		else if (LUA_HookMobjDamage(target, inflictor, source, damage, damagetype))
		{
			return true;
		}
		else
		{
			UINT8 type = (damagetype & DMG_TYPEMASK);
			const boolean hardhit = (type == DMG_EXPLODE || type == DMG_KARMA || type == DMG_TUMBLE); // This damage type can do evil stuff like ALWAYS combo
			INT16 ringburst = 5;

			if (inflictor && !P_MobjWasRemoved(inflictor) && inflictor->type == MT_INSTAWHIP && type == DMG_WHUMBLE)
				truewhumble = true;

			// Check if the player is allowed to be damaged!
			// If not, then spawn the instashield effect instead.
			if (!force)
			{
				boolean invincible = true;
				boolean clash = true; // This effect is cool and reads well, why not
				sfxenum_t sfx = sfx_None;

				if (!(gametyperules & GTR_BUMPERS))
				{
					if (damagetype & DMG_STEAL)
					{
						// Gametype does not have bumpers, steal damage is intended to not do anything
						// (No instashield is intentional)
						return false;
					}
				}

				if (player->invincibilitytimer > 0)
				{
					sfx = sfx_invind;
				}
				else if (K_IsBigger(target, inflictor) == true &&
					// SPB bypasses grow (K_IsBigger handles NULL check)
					(type != DMG_EXPLODE || inflictor->type != MT_SPBEXPLOSION || !inflictor->movefactor))
				{
					sfx = sfx_grownd;
				}
				else if (K_PlayerGuard(player))
				{
					sfx = sfx_s3k3a;
				}
				else if (player->overshield &&
					(type != DMG_EXPLODE || inflictor->type != MT_SPBEXPLOSION || !inflictor->movefactor))
				{
					;
				}
				else if (player->lightningcharge &&
					(type != DMG_EXPLODE || inflictor->type != MT_SPBEXPLOSION || !inflictor->movefactor))
				{
					;
					sfx = sfx_s3k45;
				}
				else if (player->hyudorotimer > 0)
				{
					clash = false;
				}
				else
				{
					invincible = false;
				}

				// Hack for instawhip-guard counter, lets invincible players lose to guard
				if (inflictor == target)
				{
					invincible = false;
				}

				if (player->pflags2 & PF2_ALWAYSDAMAGED)
				{
					invincible = false;
					clash = false;
				}

				// TODO: doing this from P_DamageMobj limits punting to objects that damage the player.
				// And it may be kind of yucky.
				// But this is easier than accounting for every condition in PIT_CheckThing!
				if (inflictor && K_PuntCollide(inflictor, target))
				{
					return false;
				}

				if (invincible && !truewhumble)
				{
					const INT32 oldHitlag = target->hitlag;
					const INT32 oldHitlagInflictor = inflictor ? inflictor->hitlag : 0;

					// Damage during hitlag should be a no-op
					// for invincibility states because there
					// are no flashing tics. If the damage is
					// from a constant source, a deadlock
					// would occur.

					if (target->eflags & MFE_PAUSED)
					{
						player->timeshit--; // doesn't count

						if (playerInflictor)
						{
							playerInflictor->timeshit--;
						}

						return false;
					}

					if (!clash) // Currently a no-op, damage floor hitlag kinda sucked ass
					{
						laglength = max(laglength / 2, 1);
						K_SetHitLagForObjects(target, inflictor, source, laglength, false);

						AddNullHitlag(player, oldHitlag);
						AddNullHitlag(playerInflictor, oldHitlagInflictor);
					}

					if (player->timeshit > player->timeshitprev)
					{
						S_StartSound(target, sfx);
					}

					if (clash)
					{
						player->spheres = max(player->spheres - 5, 0);

						if (inflictor)
						{
							K_DoPowerClash(target, inflictor);

							if (player->lightningcharge)
							{
								K_SpawnDriftElectricSparks(player, SKINCOLOR_PURPLE, true);
							}

							if (inflictor->type == MT_SUPER_FLICKY)
							{
								Obj_BlockSuperFlicky(inflictor);
							}

							S_StartSound(target, sfx);
						}
						else if (source)
						{
							K_DoPowerClash(target, source);
							S_StartSound(target, sfx);
						}

					}

					// Full invulnerability
					K_DoInstashield(player);
					return false;
				}
				{
					// Check if we should allow wombo combos (hard hits by default, inverted by the presence of DMG_WOMBO).
					boolean allowcombo = ((hardhit || (type == DMG_STUMBLE || type == DMG_WHUMBLE)) == !(damagetype & DMG_WOMBO));

					// Tumble/stumble is a special case.
					if (type == DMG_TUMBLE)
					{
						// don't allow constant combo
						if (player->tumbleBounces == 1 && (P_MobjFlip(target)*target->momz > 0))
							allowcombo = false;
					}
					else if (type == DMG_STUMBLE || type == DMG_WHUMBLE)
					{
						// don't allow constant combo
						if (player->tumbleBounces == TUMBLEBOUNCES-1 && (P_MobjFlip(target)*target->momz > 0))
						{
							if (type == DMG_STUMBLE)
								return false; // No-sell strings of stumble

							allowcombo = false;
						}
					}

					if (inflictor && !P_MobjWasRemoved(inflictor) && inflictor->momx == 0 && inflictor->momy == 0 && inflictor->momz == 0 && inflictor->type != MT_SPBEXPLOSION)
					{
						// Probably a map hazard.
						allowcombo = false;
					}

					if (allowcombo == false && (target->eflags & MFE_PAUSED))
					{
						return false;
					}

					// DMG_EXPLODE excluded from flashtic checks to prevent dodging eggbox/SPB with weak spinout
					if ((target->hitlag == 0 || allowcombo == false) &&
						player->flashing > 0 &&
						type != DMG_EXPLODE &&
						type != DMG_STUMBLE &&
						type != DMG_WHUMBLE &&
						P_FlashingException(player, inflictor) == false)
					{
						// Post-hit invincibility
						K_DoInstashield(player);
						return false;
					}
					else if (target->flags2 & MF2_ALREADYHIT) // do not deal extra damage in the same tic
					{
						K_SetHitLagForObjects(target, inflictor, source, laglength, true);
						return false;
					}
				}
			}

			if (gametyperules & GTR_BUMPERS)
			{
				if (damagetype & DMG_STEAL)
				{
					// Steals 2 bumpers
					damage = 2;
				}
			}
			else
			{
				// Do not die from damage outside of bumpers health system
				damage = 0;
			}

			boolean softenTumble = false;

			// Sting and stumble shouldn't be rewarding Battle hits.
			if (type == DMG_STING || type == DMG_STUMBLE)
			{
				damage = 0;

				if (source && source != player->mo && source->player)
				{
					if (!P_PlayerInPain(player) && (player->defenseLockout || player->instaWhipCharge))
					{
						K_SpawnAmps(source->player, 20, target);
					}
				}
			}
			else
			{
				// We successfully damaged them! Give 'em some bumpers!

				if (source && source != player->mo && source->player)
				{
					// Stone Shoe handles amps on its own, but this is also a good place to set soften tumble for it
					if (inflictor->type == MT_STONESHOE || inflictor->type == MT_STONESHOE_CHAIN)
						softenTumble = true;
					else
						K_SpawnAmps(source->player, K_PvPAmpReward((truewhumble) ? 30 : 20, source->player, player), target);


					K_BotHitPenalty(player);

					if (G_SameTeam(source->player, player))
					{
						if (type != DMG_EXPLODE)
						{
							type = DMG_STUMBLE;
							downgraded = true;
						}
					}
					else
					{
						for (UINT8 i = 0; i < MAXPLAYERS; i++)
						{
							if (!playeringame[i] || players[i].spectator || !players[i].mo || P_MobjWasRemoved(players[i].mo))
								continue;
							if (!G_SameTeam(source->player, &players[i]))
								continue;
							if (source->player == &players[i])
								continue;
							K_SpawnAmps(&players[i], FixedInt(FixedMul(5, K_TeamComebackMultiplier(player))), target);
						}
					}


					// Extend the invincibility if the hit was a direct hit.
					if (inflictor == source && source->player->invincibilitytimer &&
							!K_PowerUpRemaining(source->player, POWERUP_SMONITOR))
					{
						tic_t kinvextend;

						softenTumble = true;

						if (gametyperules & GTR_CLOSERPLAYERS)
							kinvextend = 2*TICRATE;
						else
							kinvextend = 3*TICRATE;

						// Reduce the value of subsequent invinc extensions
						kinvextend = kinvextend / (1 + source->player->invincibilityextensions); // 50%, 33%, 25%[...]
						kinvextend = max(kinvextend, TICRATE);

						source->player->invincibilityextensions++;

						source->player->invincibilitytimer += kinvextend;

						if (P_IsDisplayPlayer(source->player))
							S_StartSound(NULL, sfx_gsha7);
					}

					// if the inflictor is a landmine, its reactiontime will be non-zero if it is still moving
					if (inflictor->type == MT_LANDMINE && inflictor->reactiontime > 0)
					{
						// reduce tumble severity to account for getting beaned point blank sometimes
						softenTumble = true;
						// make it more consistent with set landmines
						inflictor->momx = 0;
						inflictor->momy = 0;
					}

					K_TryHurtSoundExchange(target, source);

					if (K_Cooperative() == false)
					{
						K_BattleAwardHit(source->player, player, inflictor, damage);
					}

					if (K_Bumpers(source->player) < K_StartingBumperCount() || (damagetype & DMG_STEAL))
					{
						K_TakeBumpersFromPlayer(source->player, player, damage);
					}

					if (damagetype & DMG_STEAL)
					{
						// Give them ALL of your emeralds instantly :)
						source->player->emeralds |= player->emeralds;
						player->emeralds = 0;
						K_CheckEmeralds(source->player);
					}
				}

				if (!(damagetype & DMG_STEAL))
				{
					// Drop all of your emeralds
					K_DropEmeraldsFromPlayer(player, player->emeralds);
				}
			}

			if (source && source != player->mo && source->player)
			{
				if (damagetype != DMG_DEATHPIT)
				{
					player->pitblame = source->player - players;
				}
			}

			player->sneakertimer = player->numsneakers = 0;
			player->panelsneakertimer = player->numpanelsneakers = 0;
			player->weaksneakertimer = player->numweaksneakers = 0;
			player->driftboost = player->strongdriftboost = 0;
			player->gateBoost = 0;
			player->fastfall = 0;
			player->ringboost = 0;
			player->glanceDir = 0;
			player->preventfailsafe = TICRATE*3;
			player->pflags &= ~PF_GAINAX;
			Obj_EndBungee(player);
			K_BumperInflate(target->player);

			UINT32 hurtskinflags = (demo.playback)
					? demo.skinlist[demo.currentskinid[(player-players)]].flags
					: skins[player->skin]->flags;
			if (hurtskinflags & SF_IRONMAN)
			{
				if (gametyperules & GTR_BUMPERS)
					SetRandomFakePlayerSkin(player, false, true);
			}

			// Explosions are explicit combo setups.
			if (damagetype & DMG_EXPLODE)
				player->bumperinflate = 0;

			if (player->spectator == false && !(player->charflags & SF_IRONMAN))
			{
				UINT32 skinflags = (demo.playback)
					? demo.skinlist[demo.currentskinid[(player-players)]].flags
					: skins[player->skin]->flags;

				if (skinflags & SF_IRONMAN)
				{
					player->mo->skin = skins[player->skin];
					player->charflags = skinflags;
					K_SpawnMagicianParticles(player->mo, 5);
				}
			}

			if (player->rings <= -20)
			{
				player->markedfordeath = true;
				damagetype = DMG_TUMBLE;
				type = DMG_TUMBLE;
				P_StartQuakeFromMobj(5, 44 * player->mo->scale, 2560 * player->mo->scale, player->mo);
				//P_KillPlayer(player, inflictor, source, damagetype);
			}

			// Death save! On your last hit, no matter what, demote to weakest damage type for one last escape chance.
			if (player->mo->health == 2 && damage && gametyperules & GTR_BUMPERS)
			{
				K_AddMessageForPlayer(player, "\x8DLast Chance!", false, false);
				S_StartSound(target, sfx_gshc7);
				player->flashing = TICRATE;
				type = DMG_STUMBLE;
				downgraded = true;
			}

			// Downgrade backthrown items that are not dedicated traps.
			if (inflictor && !P_MobjWasRemoved(inflictor) && P_IsKartItem(inflictor->type) && inflictor->cvmem
				&& inflictor->type != MT_BANANA)
			{
				type = DMG_WHUMBLE;
				downgraded = true;
			}

			// Downgrade orbital items.
			if (inflictor && !P_MobjWasRemoved(inflictor) && (inflictor->type == MT_ORBINAUT_SHIELD || inflictor->type == MT_JAWZ_SHIELD))
			{
				type = DMG_WHUMBLE;
				downgraded = true;
			}

			if (!(gametyperules & GTR_SPHERES) && player->tripwireLeniency && !P_PlayerInPain(player))
			{
				switch (type)
				{
					case DMG_EXPLODE:
						type = DMG_TUMBLE;
						downgraded = true;
						softenTumble = true;
						break;
					case DMG_TUMBLE:
						softenTumble = true;
						break;
					case DMG_NORMAL:
					case DMG_WIPEOUT:
						downgraded = true;
						type = DMG_WHUMBLE;
						break;
					default:
						break;
				}
			}

			switch (type)
			{
				case DMG_STING:
					K_DebtStingPlayer(player, source);
					K_KartPainEnergyFling(player);
					ringburst = 0;
					break;
				case DMG_STUMBLE:
				case DMG_WHUMBLE:
					K_StumblePlayer(player);
					ringburst = (type == DMG_WHUMBLE) ? 5 : 0;
					break;
				case DMG_TUMBLE:
					K_TumblePlayer(player, inflictor, source, softenTumble);
					ringburst = 10;
					break;
				case DMG_EXPLODE:
				case DMG_KARMA:
					ringburst = K_ExplodePlayer(player, inflictor, source);
					break;
				case DMG_WIPEOUT:
					K_SpinPlayer(player, inflictor, source, KSPIN_WIPEOUT);
					K_KartPainEnergyFling(player);
					break;
				case DMG_VOLTAGE:
				case DMG_NORMAL:
				default:
					K_SpinPlayer(player, inflictor, source, KSPIN_SPINOUT);
					break;
			}

			// Have a shield? You get hit, but don't lose your rings!
			if (player->curshield != KSHIELD_NONE)
			{
				ringburst = 0;
			}

			player->ringburst += ringburst;

			if (type != DMG_STUMBLE)
			{
				if (type != DMG_STING)
					player->flashing = K_GetKartFlashing(player);

				K_PopPlayerShield(player);
				player->instashield = 15;
				K_PlayPainSound(target, source);
				player->ringboost = 0;
			}

			if (gametyperules & GTR_BUMPERS)
				player->spheres = min(player->spheres + 10, 40);

			if ((hardhit == true && !softenTumble) || cv_kartdebughuddrop.value)
			{
				K_DropItems(player);
			}
			else
			{
				K_DropHnextList(player);
			}

			if (inflictor && !P_MobjWasRemoved(inflictor) && inflictor->type == MT_BANANA)
			{
				player->flipDI = true;
			}

			// Apply stun!
			if (type != DMG_STING)
			{
				K_ApplyStun(player, inflictor, source, damage, damagetype);
			}

			K_DefensiveOverdrive(target->player);
		}
	}
	else
	{
		if (target->type == MT_SPECIAL_UFO)
		{
			return Obj_SpecialUFODamage(target, inflictor, source, damagetype);
		}
		else if (target->type == MT_BLENDEYE_MAIN)
		{
			VS_BlendEye_Damage(target, inflictor, source, damage);
		}

		if (damagetype & DMG_STEAL)
		{
			// Not a player, steal damage is intended to not do anything
			return false;
		}

		if ((target->flags & MF_BOSS) == MF_BOSS)
		{
			targetdamaging_t targetdamaging = UFOD_GENERIC;
			if (P_MobjWasRemoved(inflictor) == true)
				;
			else switch (inflictor->type)
			{
				case MT_GACHABOM:
					targetdamaging = UFOD_GACHABOM;
					break;
				case MT_ORBINAUT:
				case MT_ORBINAUT_SHIELD:
					targetdamaging = UFOD_ORBINAUT;
					break;
				case MT_BANANA:
					targetdamaging = UFOD_BANANA;
					break;
				case MT_INSTAWHIP:
					inflictor->extravalue2 = 1; // Disable whip collision
					targetdamaging = UFOD_WHIP;
					break;
				case MT_PLAYER:
					targetdamaging = UFOD_BOOST;
					break;
				case MT_JAWZ:
				case MT_JAWZ_SHIELD:
					targetdamaging = UFOD_JAWZ;
					break;
				case MT_SPB:
					targetdamaging = UFOD_SPB;
					break;
				default:
					break;
			}

			P_TrackRoundConditionTargetDamage(targetdamaging);
		}
	}

	// do the damage
	if (damagetype & DMG_DEATHMASK)
		target->health = 0;
	else
		target->health -= damage;

	if (source && source->player && target)
		G_GhostAddHit((INT32) (source->player - players), target);

	// Insta-Whip (DMG_WHUMBLE): do not reduce hitlag because
	// this can leave room for double-damage.
	if (truewhumble && (gametyperules & GTR_BUMPERS) && !battleprisons)
		laglength /= 2;

	if (target->type == MT_PLAYER && inflictor && !P_MobjWasRemoved(inflictor)
		 && inflictor->type == MT_PLAYER && K_PlayerCanPunt(inflictor->player))
		laglength = max(laglength / 2, 2);

	if (!(target->player && (damagetype & DMG_DEATHMASK)))
		K_SetHitLagForObjects(target, inflictor, source, laglength, true);

	target->flags2 |= MF2_ALREADYHIT;

	if (target->health <= 0)
	{
		P_KillMobj(target, inflictor, source, damagetype);
		return true;
	}

	//K_SetHitLagForObjects(target, inflictor, source, laglength, true);

	if (!player)
	{
		P_SetMobjState(target, target->info->painstate);

		if (!P_MobjWasRemoved(target))
		{
			// if not intent on another player,
			// chase after this one
			P_SetTarget(&target->target, source);
		}
	}

	return true;
}

#define RING_LAYER_SIDE_SIZE (3)
#define RING_LAYER_SIZE (RING_LAYER_SIDE_SIZE * 2)

void P_FlingBurst
(		player_t *player,
		angle_t fa,
		mobjtype_t objType,
		tic_t objFuse,
		fixed_t objScale,
		INT32 i,
		fixed_t dampen)
{
	mobj_t *mo = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, objType);
	P_SetTarget(&mo->target, player->mo);

	mo->threshold = 10; // not useful for spikes
	mo->fuse = objFuse;

	// We want everything from P_SpawnMobjFromMobj except scale.
	objScale = FixedMul(objScale, FixedDiv(mapobjectscale, player->mo->scale));

	if (objScale != FRACUNIT)
	{
		P_SetScale(mo, FixedMul(objScale, mo->scale));
		mo->destscale = mo->scale;
	}

	if (i & 1)
	{
		fa += ANGLE_180;
	}

	// Pitch offset changes every other ring
	angle_t offset = ANGLE_90 / (RING_LAYER_SIDE_SIZE + 2);
	angle_t fp = offset + (((i / 2) % RING_LAYER_SIDE_SIZE) * (offset * 3 >> 1));

	const UINT8 layer = i / RING_LAYER_SIZE;
	fixed_t thrust = (13 * mo->scale) + (7 * mo->scale * layer);
	thrust = FixedDiv(thrust, dampen);
	mo->momx = (player->mo->momx / 2) + FixedMul(FixedMul(thrust, FINECOSINE(fp >> ANGLETOFINESHIFT)), FINECOSINE(fa >> ANGLETOFINESHIFT));
	mo->momy = (player->mo->momy / 2) + FixedMul(FixedMul(thrust, FINECOSINE(fp >> ANGLETOFINESHIFT)), FINESINE(fa >> ANGLETOFINESHIFT));
	mo->momz = (player->mo->momz / 2) + (FixedMul(thrust, FINESINE(fp >> ANGLETOFINESHIFT)) * P_MobjFlip(mo));
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
	INT32 spill_total, num_fling_rings;
	INT32 i;
	angle_t fa;

	// Rings shouldn't be in Battle!
	if (gametyperules & GTR_SPHERES)
		return;

	// Better safe than sorry.
	if (!player)
		return;

	// Have a shield? You get hit, but don't lose your rings!
	if (player->curshield != KSHIELD_NONE)
		return;

	// 20 is the maximum number of rings that can be taken from you at once - half the span of your counter
	if (num_rings > 20)
		num_rings = 20;
	else if (num_rings <= 0)
		return;

	spill_total = -P_GivePlayerRings(player, -num_rings);
	num_fling_rings = spill_total + min(0, player->rings);

	// determine first angle
	fa = player->mo->angle + ((P_RandomByte(PR_ITEM_RINGS) & 1) ? -ANGLE_90 : ANGLE_90);

	for (i = 0; i < num_fling_rings; i++)
	{
		P_FlingBurst(player, fa, MT_FLINGRING, 60*TICRATE, FRACUNIT, i, FRACUNIT);
	}

	while (i < spill_total)
	{
		P_FlingBurst(player, fa, MT_DEBTSPIKE, 0, 3 * FRACUNIT / 2, i++, FRACUNIT);
	}

	K_DefensiveOverdrive(player);
}
