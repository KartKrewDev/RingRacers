// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_user.c
/// \brief New stuff?
///        Player related stuff.
///        Bobbing POV/weapon, movement.
///        Pending weapon.

#include "doomdef.h"
#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "g_game.h"
#include "p_local.h"
#include "r_fps.h"
#include "r_main.h"
#include "s_sound.h"
#include "r_skins.h"
#include "d_think.h"
#include "r_sky.h"
#include "p_setup.h"
#include "m_random.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_joy.h"
#include "p_slopes.h"
#include "p_spec.h"
#include "r_splats.h"
#include "z_zone.h"
#include "w_wad.h"
#include "y_inter.h" // Y_DetermineIntermissionType
#include "hu_stuff.h"
// We need to affect the NiGHTS hud
#include "st_stuff.h"
#include "lua_script.h"
#include "lua_hook.h"
// Objectplace
#include "m_cheat.h"
// Thok camera snap (ctrl-f "chalupa")
#include "g_input.h"

// SRB2kart
#include "m_cond.h" // M_UpdateUnlockablesAndExtraEmblems
#include "k_kart.h"
#include "console.h" // CON_LogMessage
#include "k_respawn.h"
#include "k_bot.h"
#include "k_grandprix.h"
#include "k_boss.h"
#include "k_specialstage.h"
#include "k_terrain.h" // K_SpawnSplashForMobj
#include "k_color.h"
#include "k_follower.h"
#include "k_battle.h"
#include "k_rank.h"
#include "k_director.h"
#include "g_party.h"
#include "k_profiles.h"
#include "music.h"
#include "k_tally.h"
#include "k_objects.h"
#include "k_endcam.h"
#include "k_credits.h"
#include "k_hud.h" // K_AddMessage
#include "m_easing.h"
#include "acs/interface.h"

#ifdef HWRENDER
#include "hardware/hw_light.h"
#include "hardware/hw_main.h"
#endif

#if 0
static void P_NukeAllPlayers(player_t *player);
#endif

//
// Movement.
//

// 16 pixels of bob
//#define MAXBOB (0x10 << FRACBITS)

static boolean onground;

//
// P_Thrust
// Moves the given origin along a given angle.
//
void P_Thrust(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx += FixedMul(move, FINECOSINE(angle));
	mo->momy += FixedMul(move, FINESINE(angle));
}

#if 0
static inline void P_VectorInstaThrust(fixed_t xa, fixed_t xb, fixed_t xc, fixed_t ya, fixed_t yb, fixed_t yc,
	fixed_t za, fixed_t zb, fixed_t zc, fixed_t momentum, mobj_t *mo)
{
	fixed_t a1, b1, c1, a2, b2, c2, i, j, k;

	a1 = xb - xa;
	b1 = yb - ya;
	c1 = zb - za;
	a2 = xb - xc;
	b2 = yb - yc;
	c2 = zb - zc;
/*
	// Convert to unit vectors...
	a1 = FixedDiv(a1,FixedSqrt(FixedMul(a1,a1) + FixedMul(b1,b1) + FixedMul(c1,c1)));
	b1 = FixedDiv(b1,FixedSqrt(FixedMul(a1,a1) + FixedMul(b1,b1) + FixedMul(c1,c1)));
	c1 = FixedDiv(c1,FixedSqrt(FixedMul(c1,c1) + FixedMul(c1,c1) + FixedMul(c1,c1)));

	a2 = FixedDiv(a2,FixedSqrt(FixedMul(a2,a2) + FixedMul(c2,c2) + FixedMul(c2,c2)));
	b2 = FixedDiv(b2,FixedSqrt(FixedMul(a2,a2) + FixedMul(c2,c2) + FixedMul(c2,c2)));
	c2 = FixedDiv(c2,FixedSqrt(FixedMul(a2,a2) + FixedMul(c2,c2) + FixedMul(c2,c2)));
*/
	// Calculate the momx, momy, and momz
	i = FixedMul(momentum, FixedMul(b1, c2) - FixedMul(c1, b2));
	j = FixedMul(momentum, FixedMul(c1, a2) - FixedMul(a1, c2));
	k = FixedMul(momentum, FixedMul(a1, b2) - FixedMul(a1, c2));

	mo->momx = i;
	mo->momy = j;
	mo->momz = k;
}
#endif

//
// P_InstaThrust
// Moves the given origin along a given angle instantly.
//
// FIXTHIS: belongs in another file, not here
//
void P_InstaThrust(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx = FixedMul(move, FINECOSINE(angle));
	mo->momy = FixedMul(move,FINESINE(angle));
}

// Returns a location (hard to explain - go see how it is used)
fixed_t P_ReturnThrustX(mobj_t *mo, angle_t angle, fixed_t move)
{
	(void)mo;
	angle >>= ANGLETOFINESHIFT;
	return FixedMul(move, FINECOSINE(angle));
}
fixed_t P_ReturnThrustY(mobj_t *mo, angle_t angle, fixed_t move)
{
	(void)mo;
	angle >>= ANGLETOFINESHIFT;
	return FixedMul(move, FINESINE(angle));
}

//
// P_AutoPause
// Returns true when gameplay should be halted even if the game isn't necessarily paused.
//
boolean P_AutoPause(void)
{
	// Don't pause even on menu-up or focus-lost in netgames or record attack
	if (netgame || modeattacking || gamestate == GS_TITLESCREEN || gamestate == GS_MENU || con_startup)
		return false;

	return ((menuactive && !demo.playback) || ( window_notinfocus && cv_pauseifunfocused.value ));
}

//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
void P_CalcHeight(player_t *player)
{
	fixed_t bob = 0;
	fixed_t pviewheight;
	mobj_t *mo = player->mo;
	fixed_t bobmul = FRACUNIT - FixedDiv(FixedHypot(player->rmomx, player->rmomy), K_GetKartSpeed(player, false, false));

	// Regular movement bobbing.
	// Should not be calculated when not on ground (FIXTHIS?)
	// OPTIMIZE: tablify angle
	// Note: a LUT allows for effects
	//  like a ramp with low health.

	if (bobmul > FRACUNIT) bobmul = FRACUNIT;
	if (bobmul < FRACUNIT/8) bobmul = FRACUNIT/8;

	player->bob = FixedMul(cv_movebob.value, bobmul);

	if (!P_IsObjectOnGround(mo) || player->spectator)
	{
		if (mo->eflags & MFE_VERTICALFLIP)
		{
			player->viewz = mo->z + mo->height - player->viewheight;

			if (mo->flags & MF_NOCLIPHEIGHT)
				return;

			if (player->viewz < mo->floorz + FixedMul(FRACUNIT, mo->scale))
				player->viewz = mo->floorz + FixedMul(FRACUNIT, mo->scale);
		}
		else
		{
			player->viewz = mo->z + player->viewheight;

			if (mo->flags & MF_NOCLIPHEIGHT)
				return;

			if (player->viewz > mo->ceilingz - FixedMul(FRACUNIT, mo->scale))
				player->viewz = mo->ceilingz - FixedMul(FRACUNIT, mo->scale);
		}
		return;
	}

	// First person move bobbing in Kart is more of a "move jitter", to match how a go-kart would feel :p
	if (leveltime & 1)
	{
		bob = player->bob;
	}

	// move viewheight
	pviewheight = P_GetPlayerViewHeight(player); // default eye view height

	if (player->playerstate == PST_LIVE)
	{
		player->viewheight += player->deltaviewheight;

		if (player->viewheight > pviewheight)
		{
			player->viewheight = pviewheight;
			player->deltaviewheight = 0;
		}

		if (player->viewheight < pviewheight/2)
		{
			player->viewheight = pviewheight/2;
			if (player->deltaviewheight <= 0)
				player->deltaviewheight = 1;
		}

		if (player->deltaviewheight)
		{
			player->deltaviewheight += FixedMul(FRACUNIT/4, mo->scale);
			if (!player->deltaviewheight)
				player->deltaviewheight = 1;
		}
	}

	if (player->mo->eflags & MFE_VERTICALFLIP)
		player->viewz = mo->z + mo->height - player->viewheight - bob;
	else
		player->viewz = mo->z + player->viewheight + bob;

	if (player->viewz > mo->ceilingz-FixedMul(4*FRACUNIT, mo->scale))
		player->viewz = mo->ceilingz-FixedMul(4*FRACUNIT, mo->scale);
	if (player->viewz < mo->floorz+FixedMul(4*FRACUNIT, mo->scale))
		player->viewz = mo->floorz+FixedMul(4*FRACUNIT, mo->scale);
}

/** Decides if a player is moving.
  * \param pnum The player number to test.
  * \return True if the player is considered to be moving.
  * \author Graue <graue@oceanbase.org>
  */
boolean P_PlayerMoving(INT32 pnum)
{
	player_t *p = &players[pnum];

	if (!Playing())
		return false;

	if (p->jointime < 5*TICRATE || p->playerstate == PST_DEAD || p->playerstate == PST_REBORN || p->spectator)
		return false;

	return gamestate == GS_LEVEL && p->mo && p->mo->health > 0
		&& (abs(p->rmomx) >= FixedMul(FRACUNIT/2, p->mo->scale)
			|| abs(p->rmomy) >= FixedMul(FRACUNIT/2, p->mo->scale)
			|| abs(p->mo->momz) >= FixedMul(FRACUNIT/2, p->mo->scale));
}

// P_GetNextEmerald
//
// Gets the number (0 based) of the next emerald to obtain
//
UINT8 P_GetNextEmerald(void)
{
	cupheader_t *cup = NULL;

	if (grandprixinfo.gp == true && grandprixinfo.cup)
	{
		if (gamedata->sealedswaps[GDMAX_SEALEDSWAPS-1] != NULL // all found
		|| grandprixinfo.cup->id >= basenumkartcupheaders // custom content
		|| M_SecretUnlocked(SECRET_SPECIALATTACK, false)) // true order
		{
			cup = grandprixinfo.cup;
		}
		else
		{
			// Determine order from sealedswaps.
			UINT8 i;
			for (i = 0; (i < GDMAX_SEALEDSWAPS && gamedata->sealedswaps[i]); i++)
			{
				if (gamedata->sealedswaps[i] != grandprixinfo.cup)
					continue;

				// Repeat visit, grab the same ID.
				break;
			}

			return i+1;
		}
	}

	if (cup == NULL)
	{
		INT16 mapnum = gamemap-1;

		if (mapnum < nummapheaders && mapheaderinfo[mapnum])
		{
			cup = mapheaderinfo[mapnum]->cup;
		}
	}

	if (cup == NULL)
	{
		return 0;
	}

	return cup->emeraldnum;
}

//
// P_FindLowestLap
//
// SRB2Kart, a similar function as above for finding the lowest lap
//
UINT8 P_FindLowestLap(void)
{
	INT32 i;
	UINT8 lowest = UINT8_MAX;

	if (!(gametyperules & GTR_CIRCUIT))
		return 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		if (lowest == UINT8_MAX || players[i].laps < lowest)
		{
			lowest = players[i].laps;
		}
	}

	CONS_Debug(DBG_GAMELOGIC, "Lowest laps found: %d\n", lowest);

	return lowest;
}

//
// P_FindHighestLap
//
UINT8 P_FindHighestLap(void)
{
	INT32 i;
	UINT8 highest = 0;

	if (!(gametyperules & GTR_CIRCUIT))
		return 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		if (players[i].laps > highest)
			highest = players[i].laps;
	}

	CONS_Debug(DBG_GAMELOGIC, "Highest laps found: %d\n", highest);

	return highest;
}

//
// P_PlayerInPain
//
// Is player in pain??
// Checks for painstate and flashing, if both found return true
//
boolean P_PlayerInPain(const player_t *player)
{
	if (player->spinouttimer || (player->tumbleBounces > 0) || (player->pflags & PF_FAULT) || player->icecube.frozen)
		return true;

	return false;
}

//
// P_ResetPlayer
//
// Useful when you want to kill everything the player is doing.
void P_ResetPlayer(player_t *player)
{
	//player->pflags &= ~(PF_);

	player->carry = CR_NONE;
	player->onconveyor = 0;

	//player->drift = player->driftcharge = 0;
	player->trickpanel = TRICKSTATE_NONE;
	player->glanceDir = 0;
	player->fastfall = 0;
	player->turbine = 0;
	Obj_EndBungee(player);

	if (player->mo != NULL && P_MobjWasRemoved(player->mo) == false)
	{
		P_ResetPitchRoll(player->mo);
	}
}

//
// P_GivePlayerRings
//
// Gives rings to the player, and does any special things required.
// Call this function when you want to increment the player's health.
// Returns the number of rings successfully given (or taken).
//

INT32 P_GivePlayerRings(player_t *player, INT32 num_rings)
{
	INT32 test;

	if (!player->mo)
		return 0;

	if ((gametyperules & GTR_SPHERES)) // No rings in Battle Mode
		return 0;

	if (gamedata && num_rings > 0 && P_IsPartyPlayer(player) && gamedata->totalrings <= GDMAX_RINGS)
	{
		gamedata->totalrings += num_rings;
	}

	test = player->rings + num_rings;
	if (test > 20) // Caps at 20 rings, sorry!
		num_rings -= (test-20);
	else if (test < -20) // Chaotix ring debt!
		num_rings -= (test+20);

	player->rings += num_rings;

	if (player->roundconditions.debt_rings == false
		&& !(player->exiting || (player->pflags & PF_NOCONTEST))
		&& player->rings < 0)
	{
		player->roundconditions.debt_rings = true;
		player->roundconditions.checkthisframe = true;
	}

	return num_rings;
}

INT32 P_GivePlayerSpheres(player_t *player, INT32 num_spheres)
{
	num_spheres += player->spheres;

	if (!(gametyperules & GTR_SPHERES)) // No spheres in Race mode)
		return 0;

	if (num_spheres > 40) // Reached the cap, don't waste 'em!
		num_spheres = 40;
	else if (num_spheres < 0)
		num_spheres = 0;

	num_spheres -= player->spheres;

	player->spheres += num_spheres;

	return num_spheres;
}

//
// P_GivePlayerLives
//
// Gives the player an extra life.
// Call this function when you want to add lives to the player.
//
void P_GivePlayerLives(player_t *player, INT32 numlives)
{
	player->lives += numlives;

	if (player->lives > 10)
		player->lives = 10;
	else if (player->lives < 1)
		player->lives = 1;
}

// Adds to the player's score
void P_AddPlayerScore(player_t *player, INT32 amount)
{
	if (!((gametyperules & GTR_POINTLIMIT)))
		return;

	if (player->exiting) // srb2kart
		return;

	// Don't underflow.
	// Don't go above MAXSCORE.
	if (amount < 0 && (UINT32)-amount > player->roundscore)
		player->roundscore = 0;
	else if (player->roundscore + amount < MAXSCORE)
	{
		if (player->roundscore < g_pointlimit && g_pointlimit <= player->roundscore + amount)
			HU_DoTitlecardCEchoForDuration(player, "K.O. READY!", true, 5*TICRATE/2);

		player->roundscore += amount;
	}
	else
		player->roundscore = MAXSCORE;
}

void P_PlayRinglossSound(mobj_t *source)
{
	if (source->player && K_GetShieldFromItem(source->player->itemtype) != KSHIELD_NONE)
		S_StartSound(source, sfx_s1a3); // Shield hit (no ring loss)
	else if (source->player && source->player->rings <= 0)
		S_StartSound(source, sfx_s1a6); // Ring debt (lessened ring loss)
	else
		S_StartSound(source, sfx_s1c6); // Normal ring loss sound
}

void P_PlayDeathSound(mobj_t *source)
{
	S_StartSound(source, sfx_s3k35);
}

//
// P_StartPositionMusic
//
// Consistently sets starting music!
//
void P_StartPositionMusic(boolean exact)
{
	if (encoremode)
	{
		if (exact
			? (leveltime != 1)
			: (leveltime  < 1))
			return;

		Music_Remap("position", "encore");
	}
	else
	{
		if (exact
			? (leveltime != introtime)
			: (leveltime  < introtime))
			return;

		Music_Remap("position",
			(mapheaderinfo[gamemap-1]->positionmus[0]
				? mapheaderinfo[gamemap-1]->positionmus
				: "postn"
			));
	}

	Music_Play("position");
	Music_DelayEnd("position", (starttime + (TICRATE/2)) - leveltime);
}

//
// P_EndingMusic
//
// Consistently sets ending music!
//
void P_EndingMusic(void)
{
	const char *jingle = NULL;
	UINT8 bestPos = UINT8_MAX;
	player_t *bestPlayer = NULL;

	SINT8 i = MAXPLAYERS;

	// See G_DoCompleted and Y_DetermineIntermissionType
	boolean nointer = ((modeattacking && (players[consoleplayer].pflags & PF_NOCONTEST))
		|| (grandprixinfo.gp == true && grandprixinfo.eventmode != GPEVENT_NONE));

	if (K_CheckBossIntro())
		;
	else for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i]
		|| players[i].spectator)
			continue;

		// Battle powerstone win
		if ((gametyperules & GTR_POWERSTONES)
		&& ALLCHAOSEMERALDS(players[i].emeralds))
			break;

		// Special round?
		if (((gametyperules & GTR_SPECIALSTART)
		|| (grandprixinfo.gp == true
			&& grandprixinfo.eventmode == GPEVENT_SPECIAL)
		) == false)
			continue;

		// Any player has completed well?
		if (!players[i].exiting
		|| players[i].bot
		|| K_IsPlayerLosing(&players[i]))
			continue;

		// Special win
		break;
	}

	// Event - Emerald Finish
	if (i != MAXPLAYERS)
	{
		jingle = "EMRLD";
		goto skippingposition;
	}

	// Event - Level Finish
	// Check for if this is valid or not
	for (i = 0; i <= r_splitscreen; i++)
	{
		UINT8 pos = UINT8_MAX;
		player_t *checkPlayer = NULL;

		checkPlayer = &players[displayplayers[i]];
		if (!checkPlayer || checkPlayer->spectator == true)
		{
			continue;
		}

		if (checkPlayer->exiting)
		{
			// Standard exit, use their position
			pos = checkPlayer->position;
		}
		else if (checkPlayer->pflags & PF_NOCONTEST)
		{
			// No Contest without finishing, use special value
			;
		}
		else
		{
			// Not finished, ignore
			continue;
		}

		if (pos <= bestPos)
		{
			bestPlayer = checkPlayer;
			bestPos = pos;
		}
	}

	if (bestPlayer == NULL)
	{
		// No jingle for you
		return;
	}

	if (bestPos == UINT8_MAX)
	{
		jingle = "RETIRE";

		if (G_GametypeAllowsRetrying() == true)
		{
			// A retry will be happening
			nointer = true;
		}
	}
	else if (K_IsPlayerLosing(bestPlayer) == true)
	{
		jingle = "_lose";

		if (G_GametypeAllowsRetrying() == true)
		{
			// A retry will be happening
			nointer = true;
		}
	}
	else
	{
		if (K_CheckBossIntro() == true)
		{
			jingle = "VSENT";
		}
		else if (modeattacking)
		{
			if (players[consoleplayer].realtime < oldbest && oldbest != (tic_t)UINT32_MAX && !demo.playback)
				jingle = "newrec";
			else
				jingle = "norec";
		}
		else if (bestPlayer->position == 1)
		{
			jingle = "_first";
		}
		else
		{
			jingle = "_win";
		}
	}

skippingposition:

	if (nointer == true)
	{
		// Do not set "racent" in G_Ticker
		musiccountdown = 1;
	}

	if (jingle == NULL)
		return;

	Music_Remap("finish", jingle);
	Music_Play("finish");
}

void P_InvincGrowMusic(void)
{
	INT32 invinc = 0;
	INT32 grow = 0;

	UINT8 i;

	for (i = 0; i <= r_splitscreen; ++i)
	{
		player_t *player = &players[displayplayers[i]];

		if (!P_IsPartyPlayer(player))
		{
			// Director cam on another player? Don't play
			// this.
			continue;
		}

		// Find the longest running timer among splitscreen
		// players and use that.

		if (player->invincibilitytimer > invinc)
		{
			invinc = player->invincibilitytimer;
		}

		if (player->growshrinktimer > grow)
		{
			grow = player->growshrinktimer;
		}
	}

	if (invinc && !Music_Playing("invinc"))
	{
		Music_Play("invinc");
	}

	if (grow && !Music_Playing("grow"))
	{
		Music_Play("grow");
	}

	Music_DelayEnd("invinc", invinc);
	Music_DelayEnd("grow", grow);
}

//
// P_IsObjectInGoop
//
// Returns true if the object is inside goop water.
// (Spectators and objects otherwise without gravity cannot have goop gravity!)
//
boolean P_IsObjectInGoop(const mobj_t *mo)
{
	if (mo->player && mo->player->spectator)
		return false;

	if (mo->flags & MF_NOGRAVITY)
		return false;

	return ((mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER)) == (MFE_UNDERWATER|MFE_GOOWATER));
}

//
// P_IsObjectOnGround
//
// Returns true if the player is
// on the ground. Takes reverse
// gravity and FOFs into account.
//
boolean P_IsObjectOnGround(const mobj_t *mo)
{
	if (P_IsObjectInGoop(mo))
	{
/*
		// It's a crazy hack that checking if you're on the ground
		// would actually CHANGE your position and momentum,
		if (mo->z < mo->floorz)
		{
			mo->z = mo->floorz;
			mo->momz = 0;
		}
		else if (mo->z + mo->height > mo->ceilingz)
		{
			mo->z = mo->ceilingz - mo->height;
			mo->momz = 0;
		}
*/
		// but I don't want you to ever 'stand' while submerged in goo.
		// You're in constant vertical momentum, even if you get stuck on something.
		// No exceptions.
		return false;
	}

	if (mo->eflags & MFE_VERTICALFLIP)
	{
		if (mo->z+mo->height >= mo->ceilingz)
			return true;
	}
	else
	{
		if (mo->z <= mo->floorz)
			return true;
	}

	return false;
}

//
// P_IsObjectOnGroundIn
//
// Returns true if the player is
// on the ground in a specific sector. Takes reverse
// gravity and FOFs into account.
//
boolean P_IsObjectOnGroundIn(const mobj_t *mo, const sector_t *sec)
{
	ffloor_t *rover;

	// Is the object in reverse gravity?
	if (mo->eflags & MFE_VERTICALFLIP)
	{
		// Detect if the player is on the ceiling.
		if (mo->z+mo->height >= P_GetSpecialTopZ(mo, sec, sec))
			return true;
		// Otherwise, detect if the player is on the bottom of a FOF.
		else
		{
			for (rover = sec->ffloors; rover; rover = rover->next)
			{
				// If the FOF doesn't exist, continue.
				if (!(rover->fofflags & FOF_EXISTS))
					continue;

				// If the FOF is configured to let the object through, continue.
				if (!((rover->fofflags & FOF_BLOCKPLAYER && mo->player)
					|| (rover->fofflags & FOF_BLOCKOTHERS && !mo->player)))
					continue;

				// If the the platform is intangible from below, continue.
				if (rover->fofflags & FOF_PLATFORM)
					continue;

				// If the FOF is a water block, continue. (Unnecessary check?)
				if (rover->fofflags & FOF_SWIMMABLE)
					continue;

				// Actually check if the player is on the suitable FOF.
				if (mo->z+mo->height == P_GetSpecialBottomZ(mo, sectors + rover->secnum, sec))
					return true;
			}
		}
	}
	// Nope!
	else
	{
		// Detect if the player is on the floor.
		if (mo->z <= P_GetSpecialBottomZ(mo, sec, sec))
			return true;
		// Otherwise, detect if the player is on the top of a FOF.
		else
		{
			for (rover = sec->ffloors; rover; rover = rover->next)
			{
				// If the FOF doesn't exist, continue.
				if (!(rover->fofflags & FOF_EXISTS))
					continue;

				// If the FOF is configured to let the object through, continue.
				if (!((rover->fofflags & FOF_BLOCKPLAYER && mo->player)
					|| (rover->fofflags & FOF_BLOCKOTHERS && !mo->player)))
					continue;

				// If the the platform is intangible from above, continue.
				if (rover->fofflags & FOF_REVERSEPLATFORM)
					continue;

				// If the FOF is a water block, continue. (Unnecessary check?)
				if (rover->fofflags & FOF_SWIMMABLE)
					continue;

				// Actually check if the player is on the suitable FOF.
				if (mo->z == P_GetSpecialTopZ(mo, sectors + rover->secnum, sec))
					return true;
			}
		}
	}

	return false;
}

//
// P_IsObjectOnRealGround
//
// Helper function for T_EachTimeThinker
// Like P_IsObjectOnGroundIn, except ONLY THE REAL GROUND IS CONSIDERED, NOT FOFS
// I'll consider whether to make this a more globally accessible function or whatever in future
// -- Monster Iestyn
//
// Really simple, but personally I think it's also incredibly helpful. I think this is fine in p_user.c
// -- Sal

boolean P_IsObjectOnRealGround(const mobj_t *mo, const sector_t *sec)
{
	// Is the object in reverse gravity?
	if (mo->eflags & MFE_VERTICALFLIP)
	{
		// Detect if the player is on the ceiling.
		if (mo->z+mo->height >= P_GetSpecialTopZ(mo, sec, sec))
			return true;
	}
	// Nope!
	else
	{
		// Detect if the player is on the floor.
		if (mo->z <= P_GetSpecialBottomZ(mo, sec, sec))
			return true;
	}
	return false;
}

//
// P_SetObjectMomZ
//
// Sets the player momz appropriately.
// Takes reverse gravity into account.
//
void P_SetObjectMomZ(mobj_t *mo, fixed_t value, boolean relative)
{
	if (mo->eflags & MFE_VERTICALFLIP)
		value = -value;

	if (mo->scale != FRACUNIT)
		value = FixedMul(value, mo->scale);

	if (relative)
		mo->momz += value;
	else
		mo->momz = value;
}

//
// P_IsMachineLocalPlayer
//
// Returns true if player is
// ACTUALLY on the local machine
//
boolean P_IsMachineLocalPlayer(const player_t *player)
{
	UINT8 i;

	if (player == NULL)
	{
		return false;
	}

	for (i = 0; i <= splitscreen; i++)
	{
		if (player == &players[g_localplayers[i]])
			return true;
	}

	return false;
}

//
// P_IsPartyPlayer
//
// Returns true if player is
// on the local machine
// (or simulated party)
//
boolean P_IsPartyPlayer(const player_t *player)
{
	if (player == NULL)
	{
		return false;
	}

	// nobody is ever local when watching something back - you're a spectator there, even if your g_localplayers might say otherwise
	if (demo.playback)
		return false;

	// handles both online parties and local players (no need to call P_IsMachineLocalPlayer here)
	return G_IsPartyLocal(player-players);
}

//
// P_IsDisplayPlayer
//
// Returns true if player is
// currently being watched.
//
boolean P_IsDisplayPlayer(const player_t *player)
{
	UINT8 i;

	if (player == NULL)
	{
		return false;
	}

	for (i = 0; i <= r_splitscreen; i++) // DON'T skip P1
	{
		if (camera[i].freecam)
		{
			// Freecam still techically has a player in
			// displayplayers. But since the camera is
			// detached, it would be weird if sounds were
			// heard from that player's perspective.
			continue;
		}

		if (player == &players[displayplayers[i]])
			return true;
	}

	return false;
}

//
// P_SpawnGhostMobj
//
// Spawns a ghost object on the player
//
mobj_t *P_SpawnGhostMobj(mobj_t *mobj)
{
	mobj_t *ghost = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_GHOST);

	P_SetTarget(&ghost->target, mobj);

	P_SetScale(ghost, mobj->scale);
	ghost->scalespeed = mobj->scalespeed;
	ghost->destscale = mobj->scale;

	if (mobj->eflags & MFE_VERTICALFLIP)
	{
		ghost->eflags |= MFE_VERTICALFLIP;
		ghost->z += mobj->height - ghost->height;
	}

	ghost->color = mobj->color;
	ghost->colorized = mobj->colorized; // Kart: they should also be colorized if their origin is

	ghost->angle = (mobj->player ? mobj->player->drawangle : mobj->angle);
	ghost->roll = mobj->roll;
	ghost->pitch = mobj->pitch;
	ghost->sprite = mobj->sprite;
	ghost->sprite2 = mobj->sprite2;
	ghost->frame = mobj->frame;
	ghost->tics = -1;
	ghost->renderflags = (mobj->renderflags & ~RF_TRANSMASK)|RF_TRANS50;
	ghost->fuse = ghost->info->damage;
	ghost->skin = mobj->skin;
	ghost->standingslope = mobj->standingslope;

	ghost->sprxoff = mobj->sprxoff;
	ghost->spryoff = mobj->spryoff;
	ghost->sprzoff = mobj->sprzoff;
	ghost->rollangle = mobj->rollangle;

	ghost->spritexscale = mobj->spritexscale;
	ghost->spriteyscale = mobj->spriteyscale;
	ghost->spritexoffset = mobj->spritexoffset;
	ghost->spriteyoffset = mobj->spriteyoffset;

	if (mobj->flags2 & MF2_OBJECTFLIP)
		ghost->flags |= MF2_OBJECTFLIP;

	if (!(mobj->flags & MF_DONTENCOREMAP))
		ghost->flags &= ~MF_DONTENCOREMAP;

	if (mobj->player && mobj->player->followmobj)
	{
		mobj_t *ghost2 = P_SpawnGhostMobj(mobj->player->followmobj);
		P_SetTarget(&ghost2->tracer, ghost);
		P_SetTarget(&ghost->tracer, ghost2);
		ghost2->flags2 |= (mobj->player->followmobj->flags2 & MF2_LINKDRAW);
	}

	// Copy interpolation data :)
	ghost->old_x = mobj->old_x2;
	ghost->old_y = mobj->old_y2;
	ghost->old_z = mobj->old_z2;
	ghost->old_angle = (mobj->player ? mobj->player->old_drawangle2 : mobj->old_angle2);
	ghost->old_pitch = mobj->old_pitch2;
	ghost->old_roll = mobj->old_roll2;
	ghost->old_scale = mobj->old_scale2;

	P_SetTarget(&ghost->owner, mobj);
	ghost->renderflags |= RF_REDUCEVFX;

	ghost->reappear = mobj->reappear;
	P_SetTarget(&ghost->punt_ref, mobj->punt_ref);

	return ghost;
}

//
// P_SpawnFakeShadow
//
// Spawns a silhouette that copies its tracer and copies RF_DONTDRAW
//
mobj_t *P_SpawnFakeShadow(mobj_t *mobj, UINT8 offset)
{
	mobj_t *ghost = P_SpawnMobj(mobj->x, mobj->y, mobj->z - offset*mapobjectscale, MT_FAKESHADOW);
	ghost->threshold = offset;

	P_SetTarget(&ghost->target, mobj);

	P_SetScale(ghost, mobj->scale);
	ghost->destscale = mobj->scale;

	if (mobj->eflags & MFE_VERTICALFLIP)
	{
		ghost->eflags |= MFE_VERTICALFLIP;
		ghost->z += mobj->height - ghost->height;
	}

	ghost->color = mobj->color;
	ghost->colorized = mobj->colorized; // Kart: they should also be colorized if their origin is

	ghost->angle = (mobj->player ? mobj->player->drawangle : mobj->angle);
	ghost->roll = mobj->roll;
	ghost->pitch = mobj->pitch;
	ghost->sprite = mobj->sprite;
	ghost->sprite2 = mobj->sprite2;
	ghost->frame = mobj->frame;
	ghost->tics = -1;
	ghost->renderflags = mobj->renderflags;
	ghost->fuse = -1;
	ghost->skin = mobj->skin;
	ghost->standingslope = mobj->standingslope;

	ghost->sprxoff = mobj->sprxoff;
	ghost->spryoff = mobj->spryoff;
	ghost->sprzoff = mobj->sprzoff;
	ghost->rollangle = mobj->rollangle;

	ghost->spritexscale = mobj->spritexscale;
	ghost->spriteyscale = mobj->spriteyscale;
	ghost->spritexoffset = mobj->spritexoffset;
	ghost->spriteyoffset = mobj->spriteyoffset;

	if (mobj->flags2 & MF2_OBJECTFLIP)
		ghost->flags |= MF2_OBJECTFLIP;

	if (!(mobj->flags & MF_DONTENCOREMAP))
		ghost->flags &= ~MF_DONTENCOREMAP;

	if (mobj->player && mobj->player->followmobj)
	{
		mobj_t *ghost2 = P_SpawnGhostMobj(mobj->player->followmobj);
		P_SetTarget(&ghost2->tracer, ghost);
		P_SetTarget(&ghost->tracer, ghost2);
		ghost2->flags2 |= (mobj->player->followmobj->flags2 & MF2_LINKDRAW);
	}

	// Copy interpolation data :)
	ghost->old_x = mobj->old_x2;
	ghost->old_y = mobj->old_y2;
	ghost->old_z = mobj->old_z2;
	ghost->old_angle = (mobj->player ? mobj->player->old_drawangle2 : mobj->old_angle2);
	ghost->old_pitch = mobj->old_pitch2;
	ghost->old_roll = mobj->old_roll2;

	ghost->renderflags &= ~(RF_TRANSMASK|RF_FULLBRIGHT);
	ghost->renderflags |= RF_ABSOLUTELIGHTLEVEL;
	ghost->lightlevel = 0;

	ghost->flags2 |= MF2_LINKDRAW;
	P_SetTarget(&ghost->tracer, mobj);

	return ghost;
}


//
// P_DoPlayerExit
//
// Player exits the map via sector trigger
void P_DoPlayerExit(player_t *player, pflags_t flags)
{
	if (player->exiting || mapreset)
	{
		return;
	}

	player->pflags |= flags;

	if (P_IsPartyPlayer(player) && (!player->spectator && !demo.playback))
	{
		legitimateexit = true;
		player->roundconditions.checkthisframe = true;
		gamedata->deferredconditioncheck = true;
	}

	player->exiting = 1;

	if (!player->spectator && (gametyperules & GTR_CIRCUIT)) // Special Race-like handling
	{
		K_UpdateAllPlayerPositions();
	}

	const boolean losing = K_IsPlayerLosing(player); // HEY!!!! Set it AFTER K_UpdateAllPlayerPositions!!!!
	const boolean specialout = (specialstageinfo.valid == true && losing == true);

	if (losing)
	{
		// Remove a life from the losing player
		K_PlayerLoseLife(player);
	}

	if (!player->spectator)
	{
		ClearFakePlayerSkin(player);

		if ((gametyperules & GTR_CIRCUIT)) // Special Race-like handling
		{
			if (P_CheckRacers() && !exitcountdown)
			{
				G_BeginLevelExit();
			}
		}
		else if (!exitcountdown) // All other gametypes
		{
			G_BeginLevelExit();
		}
	}

	K_UpdatePowerLevelsFinalize(player, false);

	if (G_IsPartyLocal(player - players) && !specialout && musiccountdown == 0)
	{
		Music_Play("finish_silence");
		musiccountdown = MUSIC_COUNTDOWN_MAX;
	}

	if (!player->spectator)
	{
		if (!(gametyperules & GTR_SPHERES))
		{
			player->hudrings = RINGTOTAL(player);
			if (player->hudrings > 20)
				player->hudrings = 20;

			if (grandprixinfo.gp == true
				&& grandprixinfo.eventmode != GPEVENT_SPECIAL
				&& player->bot == false && losing == false)
			{
				const UINT8 lifethreshold = 20;

				const UINT8 oldExtra = player->totalring / lifethreshold;
				const UINT8 extra = (player->totalring + player->hudrings) / lifethreshold;

				if (extra > oldExtra)
				{
					player->xtralife = (extra - oldExtra);
				}
			}
		}

		if (specialstageinfo.valid == true && losing == false && P_MobjWasRemoved(player->mo) == false)
		{
			K_MakeObjectReappear(player->mo);
		}
	}

	//K_InitPlayerTally(player); -- we defer this to P_PlayerAfterThink

	if (demo.playback == false)
	{
		if (modeattacking)
		{
			G_UpdateRecords();
		}

		// Profile and skin record updates
		if (P_IsMachineLocalPlayer(player))
		{
			profile_t *pr = PR_GetPlayerProfile(player);

			// Profile records
			if (pr != NULL)
			{
				if (!losing)
				{
					pr->wins++;
				}
				pr->rounds++;
				PR_SaveProfiles();
			}

			// Skin records (saved to gamedata)
			if (player->skin < numskins)
			{
				skin_t *playerskin = &skins[player->skin];
				if (!losing)
				{
					playerskin->records.wins++;
				}
				playerskin->records.rounds++;
			}
		}

		if (!demo.savebutton)
		{
			UINT8 outstanding = splitscreen + 1;
			for (UINT8 i = 0; i <= splitscreen; ++i)
			{
				if (players[g_localplayers[i]].exiting)
					outstanding--;
			}
			// Once the entire local party finishes (not
			// online party), show the "Save Replay" button.
			if (!outstanding)
				demo.savebutton = leveltime;
		}
	}

	ACS_RunPlayerFinishScript(player);
}

//
// P_DoAllPlayersExit
//
// All players exit the map via event
void P_DoAllPlayersExit(pflags_t flags, boolean trygivelife)
{
	UINT8 i;
	const boolean dofinishsound = (musiccountdown == 0);

	if (grandprixinfo.gp == false
		|| grandprixinfo.eventmode == GPEVENT_SPECIAL
		|| (flags & PF_NOCONTEST))
	{
		trygivelife = false;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		if (players[i].exiting)
		{
			continue;
		}

		P_DoPlayerExit(&players[i], flags);

		if (trygivelife == false)
		{
			continue;
		}

		players[i].xtralife++;
	}

	if (!dofinishsound)
	{
		// You've already finished, don't play again
		;
	}
	else if (gametyperules & GTR_SPECIALSTART)
	{
		// Warp out
		S_StartSound(NULL, sfx_s3kb3);
	}
	else if (musiccountdown == 0)
	{
		// Other people finish
		S_StartSound(NULL, sfx_s253);
	}
	else if (musiccountdown > 1)
	{
		// Everyone finish sound
		S_StartSound(NULL, sfx_s3k6a);
	}
}

//
// P_PlayerHitFloor
//
// Handles player hitting floor surface.
// Returns whether to clip momz.
boolean P_PlayerHitFloor(player_t *player, boolean fromAir, angle_t oldPitch, angle_t oldRoll)
{
	boolean clipmomz;

	I_Assert(player->mo != NULL);

	clipmomz = !(P_CheckDeathPitCollide(player->mo));

	if (clipmomz == true)
	{
		if (fromAir == true)
		{
			K_SpawnSplashForMobj(player->mo, abs(player->mo->momz));
		}

		if (player->mo->health > 0)
		{
			boolean air = fromAir;

			if (P_IsObjectOnGround(player->mo) && (player->mo->eflags & MFE_JUSTHITFLOOR))
			{
				air = true;
			}

			if (K_CheckStumble(player, oldPitch, oldRoll, air) == true)
			{
				return false;
			}

			if (air == false && K_FastFallBounce(player) == true)
			{
				return false;
			}
		}
	}

	return clipmomz;
}

boolean P_InQuicksand(const mobj_t *mo) // Returns true if you are in quicksand
{
	sector_t *sector = mo->subsector->sector;
	fixed_t topheight, bottomheight;

	fixed_t flipoffset = ((mo->eflags & MFE_VERTICALFLIP) ? (mo->height/2) : 0);

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS))
				continue;

			if (!(rover->fofflags & FOF_QUICKSAND))
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, mo->x, mo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, mo->x, mo->y);

			if (mo->z + flipoffset > topheight)
				continue;

			if (mo->z + (mo->height/2) + flipoffset < bottomheight)
				continue;

			return true;
		}
	}

	return false; // No sand here, Captain!
}

static boolean P_PlayerCanBust(player_t *player, ffloor_t *rover)
{
	// TODO: Make these act like the Lua SA2 boxes.
	(void)player;
	(void)rover;

	if (!(rover->fofflags & FOF_EXISTS))
		return false;

	if (!(rover->fofflags & FOF_BUSTUP))
		return false;

	return true;
}

static void P_CheckBustableBlocks(player_t *player)
{
	msecnode_t *node;
	fixed_t oldx;
	fixed_t oldy;

	if ((netgame || multiplayer) && player->spectator)
		return;

	oldx = player->mo->x;
	oldy = player->mo->y;

	P_UnsetThingPosition(player->mo);
	player->mo->x += player->mo->momx;
	player->mo->y += player->mo->momy;
	P_SetThingPosition(player->mo);

	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		ffloor_t *rover;
		fixed_t topheight, bottomheight;

		if (!node->m_sector)
			break;

		if (!node->m_sector->ffloors)
			continue;

		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			if (!P_PlayerCanBust(player, rover))
				continue;

			topheight = P_GetFOFTopZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
			bottomheight = P_GetFOFBottomZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);

			// Height checks
			if (rover->bustflags & FB_ONLYBOTTOM)
			{
				if (player->mo->z + player->mo->momz + player->mo->height < bottomheight)
					continue;

				if (player->mo->z + player->mo->height > bottomheight)
					continue;
			}
			else
			{
				switch (rover->busttype)
				{
				case BT_TOUCH:
					if (player->mo->z + player->mo->momz > topheight)
						continue;

					if (player->mo->z + player->mo->momz + player->mo->height < bottomheight)
						continue;

					break;
				case BT_SPINBUST:
					if (player->mo->z + player->mo->momz > topheight)
						continue;

					if (player->mo->z + player->mo->height < bottomheight)
						continue;

					break;
				default:
					if (player->mo->z >= topheight)
						continue;

					if (player->mo->z + player->mo->height < bottomheight)
						continue;

					break;
				}
			}

			// Impede the player's fall a bit
			if (((rover->busttype == BT_TOUCH) || (rover->busttype == BT_SPINBUST)) && player->mo->z >= topheight)
				player->mo->momz >>= 1;
			else if (rover->busttype == BT_TOUCH)
			{
				player->mo->momx >>= 1;
				player->mo->momy >>= 1;
			}

			EV_CrumbleChain(NULL, rover); // node->m_sector

			// Run a linedef executor??
			if (rover->bustflags & FB_EXECUTOR)
				P_LinedefExecute(rover->busttag, player->mo, node->m_sector);

			goto bustupdone;
		}
	}

bustupdone:
	P_UnsetThingPosition(player->mo);
	player->mo->x = oldx;
	player->mo->y = oldy;
	P_SetThingPosition(player->mo);
}

static void P_CheckBouncySectors(player_t *player)
{
	msecnode_t *node;
	fixed_t oldx;
	fixed_t oldy;
	fixed_t oldz;
	vector3_t momentum;

	oldx = player->mo->x;
	oldy = player->mo->y;
	oldz = player->mo->z;

	P_UnsetThingPosition(player->mo);
	player->mo->x += player->mo->momx;
	player->mo->y += player->mo->momy;
	player->mo->z += player->mo->momz;
	P_SetThingPosition(player->mo);

	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		ffloor_t *rover;

		if (!node->m_sector)
			break;

		if (!node->m_sector->ffloors)
			continue;

		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			fixed_t topheight, bottomheight;

			if (!(rover->fofflags & FOF_EXISTS))
				continue; // FOFs should not be bouncy if they don't even "exist"

			// Handle deprecated bouncy FOF sector type
			if (!udmf && GETSECSPECIAL(rover->master->frontsector->special, 1) == 15)
			{
				rover->fofflags |= FOF_BOUNCY;
				rover->bouncestrength = P_AproxDistance(rover->master->dx, rover->master->dy)/100;
			}

			if (!(rover->fofflags & FOF_BOUNCY))
				continue;

			topheight = P_GetFOFTopZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
			bottomheight = P_GetFOFBottomZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);

			if (player->mo->z > topheight)
				continue;

			if (player->mo->z + player->mo->height < bottomheight)
				continue;

			if (oldz < P_GetFOFTopZ(player->mo, node->m_sector, rover, oldx, oldy, NULL)
					&& oldz + player->mo->height > P_GetFOFBottomZ(player->mo, node->m_sector, rover, oldx, oldy, NULL))
			{
				player->mo->momx = -FixedMul(player->mo->momx,rover->bouncestrength);
				player->mo->momy = -FixedMul(player->mo->momy,rover->bouncestrength);
			}
			else
			{
				pslope_t *slope = (abs(oldz - topheight) < abs(oldz + player->mo->height - bottomheight)) ? *rover->t_slope : *rover->b_slope;

				momentum.x = player->mo->momx;
				momentum.y = player->mo->momy;
				momentum.z = player->mo->momz*2;

				if (slope)
					P_ReverseQuantizeMomentumToSlope(&momentum, slope);

				momentum.z = -FixedMul(momentum.z,rover->bouncestrength)/2;

				if (abs(momentum.z) < (rover->bouncestrength*2))
					goto bouncydone;

				if (momentum.z > FixedMul(24*FRACUNIT, player->mo->scale)) //half of the default player height
					momentum.z = FixedMul(24*FRACUNIT, player->mo->scale);
				else if (momentum.z < -FixedMul(24*FRACUNIT, player->mo->scale))
					momentum.z = -FixedMul(24*FRACUNIT, player->mo->scale);

				if (slope)
					P_QuantizeMomentumToSlope(&momentum, slope);

				player->mo->momx = momentum.x;
				player->mo->momy = momentum.y;
				player->mo->momz = momentum.z;
			}
			goto bouncydone;
		}
	}

bouncydone:
	P_UnsetThingPosition(player->mo);
	player->mo->x = oldx;
	player->mo->y = oldy;
	player->mo->z = oldz;
	P_SetThingPosition(player->mo);
}

static void P_CheckQuicksand(player_t *player)
{
	ffloor_t *rover;
	fixed_t sinkspeed, friction;
	fixed_t topheight, bottomheight;

	if (!(player->mo->subsector->sector->ffloors && player->mo->momz <= 0))
		return;

	for (rover = player->mo->subsector->sector->ffloors; rover; rover = rover->next)
	{
		if (!(rover->fofflags & FOF_EXISTS)) continue;

		if (!(rover->fofflags & FOF_QUICKSAND))
			continue;

		topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
		bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);

		if (topheight >= player->mo->z && bottomheight < player->mo->z + player->mo->height)
		{
			sinkspeed = abs(rover->master->v1->x - rover->master->v2->x)>>1;

			sinkspeed = FixedDiv(sinkspeed,TICRATE*FRACUNIT);

			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				fixed_t ceilingheight = P_GetCeilingZ(player->mo, player->mo->subsector->sector, player->mo->x, player->mo->y, NULL);

				player->mo->z += sinkspeed;

				if (player->mo->z + player->mo->height >= ceilingheight)
					player->mo->z = ceilingheight - player->mo->height;

				if (player->mo->momz <= 0)
					P_PlayerHitFloor(player, false, player->mo->roll, player->mo->pitch);
			}
			else
			{
				fixed_t floorheight = P_GetFloorZ(player->mo, player->mo->subsector->sector, player->mo->x, player->mo->y, NULL);

				player->mo->z -= sinkspeed;

				if (player->mo->z <= floorheight)
					player->mo->z = floorheight;

				if (player->mo->momz >= 0)
					P_PlayerHitFloor(player, false, player->mo->roll, player->mo->pitch);
			}

			friction = abs(rover->master->v1->y - rover->master->v2->y)>>6;

			player->mo->momx = FixedMul(player->mo->momx, friction);
			player->mo->momy = FixedMul(player->mo->momy, friction);
		}
	}
}

//
// P_CheckInvincibilityTimer
//
// Restores music from invincibility, and handles invincibility sparkles
//
static void P_CheckInvincibilityTimer(player_t *player)
{
	if (!player->invincibilitytimer)
		return;

	// Resume normal music stuff.
	if (player->invincibilitytimer == 1)
	{
		//K_KartResetPlayerColor(player); -- this gets called every tic anyways
		G_GhostAddColor((INT32) (player - players), GHC_NORMAL);
		return;
	}
}

//
// P_DoBubbleBreath
//
// Handles bubbles spawned by the player
//
static void P_DoBubbleBreath(player_t *player)
{
	fixed_t x = player->mo->x;
	fixed_t y = player->mo->y;
	fixed_t z = player->mo->z;
	mobj_t *bubble = NULL;

	if (!(player->mo->eflags & MFE_UNDERWATER) || player->spectator || player->curshield == KSHIELD_BUBBLE)
		return;

#if 0
	if (player->charflags & SF_MACHINE)
	{
		if (P_RandomChance(PR_BUBBLE, FRACUNIT/5))
		{
			fixed_t r = player->mo->radius>>FRACBITS;
			x += (P_RandomRange(PR_BUBBLE, r, -r)<<FRACBITS);
			y += (P_RandomRange(PR_BUBBLE, r, -r)<<FRACBITS);
			z += (P_RandomKey(PR_BUBBLE, player->mo->height>>FRACBITS)<<FRACBITS);
			bubble = P_SpawnMobj(x, y, z, MT_WATERZAP);
			S_StartSound(bubble, sfx_beelec);
		}
	}
	else
#endif
	{
		fixed_t topspeed = K_GetKartSpeed(player, false, false);
		fixed_t f = FixedDiv(player->speed, topspeed/2);

		if (player->mo->eflags & MFE_VERTICALFLIP)
			z += player->mo->height - FixedDiv(player->mo->height,5*(FRACUNIT/4));
		else
			z += FixedDiv(player->mo->height,5*(FRACUNIT/4));

		if (P_RandomChance(PR_BUBBLE, FixedMul(FRACUNIT/16, FRACUNIT + 3*f)))
		{
			UINT32 seed = P_GetRandSeed(PR_BUBBLE);
			x += P_RandomRange(PR_BUBBLE, -16, 16) * player->mo->scale;
			y += P_RandomRange(PR_BUBBLE, -16, 16) * player->mo->scale;
			z += P_RandomRange(PR_BUBBLE, -16, 16) * player->mo->scale;
			P_SetRandSeed(PR_BUBBLE, seed);
			bubble = P_SpawnMobj(x, y, z, MT_SMALLBUBBLE);
		}
		else if (P_RandomChance(PR_BUBBLE, FixedMul(3*FRACUNIT/256, FRACUNIT + 3*f)))
		{
			UINT32 seed = P_GetRandSeed(PR_BUBBLE);
			x += P_RandomRange(PR_BUBBLE, -16, 16) * player->mo->scale;
			y += P_RandomRange(PR_BUBBLE, -16, 16) * player->mo->scale;
			z += P_RandomRange(PR_BUBBLE, -16, 16) * player->mo->scale;
			P_SetRandSeed(PR_BUBBLE, seed);
			bubble = P_SpawnMobj(x, y, z, MT_MEDIUMBUBBLE);
		}

		if (bubble)
		{
			bubble->color = SKINCOLOR_TEAL;
			bubble->colorized = true;

			f = min(f, 11*FRACUNIT/10);
			vector3_t v = {FixedMul(player->mo->momx, f), FixedMul(player->mo->momy, f), FixedMul(player->mo->momz, f)};
			if (player->mo->standingslope)
				P_QuantizeMomentumToSlope(&v, player->mo->standingslope);
			bubble->momx += v.x;
			bubble->momy += v.y;
			bubble->momz += v.z;
		}
	}

	if (bubble)
	{
		bubble->threshold = 42;
		bubble->destscale = 3 * player->mo->scale / 2;
		P_SetScale(bubble, bubble->destscale);
	}
}

static inline boolean P_IsMomentumAngleLocked(player_t *player)
{
	// This timer is used for the animation too and the
	// animation should continue for a bit after the physics
	// stop.

	if (player->stairjank > 8)
	{
		const angle_t th = K_MomentumAngle(player->mo);
		const angle_t d = AngleDelta(th, player->mo->angle);

		// A larger difference between momentum and facing
		// angles awards back control.
		//  <45 deg: 3/4 tics
		//  >45 deg: 2/4 tics
		//  >90 deg: 1/4 tics
		// >135 deg: 0/4 tics

		if ((leveltime & 3) > (d / ANGLE_45))
		{
			return true;
		}
	}

	if (K_IsRidingFloatingTop(player))
	{
		return true;
	}

	return false;
}

//#define OLD_MOVEMENT_CODE 1
static void P_3dMovement(player_t *player)
{
	angle_t movepushangle; // Analog
	fixed_t movepushforward = 0;
	angle_t dangle; // replaces old quadrants bits
	fixed_t oldMagnitude, newMagnitude;
	vector3_t totalthrust;

	totalthrust.x = totalthrust.y = 0; // I forget if this is needed

	if (G_CompatLevel(0x000B)) // Ring Racers 2.1 behavior
		totalthrust.z = FRACUNIT*P_MobjFlip(player->mo)/3; // A bit of extra push-back on slopes
	else
		totalthrust.z = FixedMul(mapobjectscale, K_GrowShrinkSpeedMul(player))*P_MobjFlip(player->mo)/3; // A bit of extra push-back on slopes, but scaled for mapobject and player size

	if (K_SlopeResistance(player) == true)
	{
		totalthrust.z = -(totalthrust.z);
	}

	// Get the old momentum; this will be needed at the end of the function! -SH
	oldMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);

	if (P_IsMomentumAngleLocked(player))
	{
		movepushangle = K_MomentumAngle(player->mo);
	}
	else if (player->drift != 0)
	{
		movepushangle = player->mo->angle - (ANGLE_45/5) * player->drift;
	}
	else if (player->spinouttimer || player->wipeoutslow) // if spun out, use the boost angle
	{
		movepushangle = (angle_t)player->boostangle;
	}
	else
	{
		movepushangle = player->mo->angle;
	}

	// cmomx/cmomy stands for the conveyor belt speed.
	if (player->onconveyor == 2) // Wind/Current
	{
		//if (player->mo->z > player->mo->watertop || player->mo->z + player->mo->height < player->mo->waterbottom)
		if (!(player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
			player->cmomx = player->cmomy = 0;
	}
	else if (player->onconveyor == 4 && !P_IsObjectOnGround(player->mo)) // Actual conveyor belt
		player->cmomx = player->cmomy = 0;
	else if (player->onconveyor != 2 && player->onconveyor != 4
#ifdef POLYOBJECTS
				&& player->onconveyor != 1
#endif
	)
		player->cmomx = player->cmomy = 0;

	player->rmomx = player->mo->momx - player->cmomx;
	player->rmomy = player->mo->momy - player->cmomy;

	// Calculates player's speed based on distance-of-a-line formula
	player->speed = R_PointToDist2(0, 0, player->rmomx, player->rmomy);

	const fixed_t topspeedometer = K_GetKartSpeed(player, false, true);

	if (player->speed > topspeedometer)
	{
		const fixed_t convSpeed = (player->speed * 100) / topspeedometer;

		if (convSpeed > player->roundconditions.maxspeed)
		{
			player->roundconditions.maxspeed = convSpeed;
			//player->roundconditions.checkthisframe = true; -- no, safe to leave until lapchange at worst
		}
	}

	// Monster Iestyn - 04-11-13
	// Quadrants are stupid, excessive and broken, let's do this a much simpler way!
	// Get delta angle from rmom angle and player angle first
	dangle = R_PointToAngle2(0,0, player->rmomx, player->rmomy) - player->mo->angle;
	if (dangle > ANGLE_180) //flip to keep to one side
	{
		dangle = InvAngle(dangle);
	}

	// anything else will leave both at 0, so no need to do anything else

	//{ SRB2kart 220217 - Toaster Code for misplaced thrust
#if 0
	if (!player->drift) // Not Drifting
	{
		angle_t difference = dangle/2;
		boolean reverse = (dangle >= ANGLE_90);

		if (dangleflip)
			difference = InvAngle(difference);

		if (reverse)
			difference += ANGLE_180;

		P_InstaThrust(player->mo, player->mo->angle + difference, player->speed);
	}
#endif
	//}

	// Do not let the player control movement if not onground.
	onground = P_IsObjectOnGround(player->mo);

	K_AdjustPlayerFriction(player);

	// Forward movement
	// If the player isn't on the ground, there is no change in speed
	// Smiley Face
	if (onground)
	{
		movepushforward = K_3dKartMovement(player);

		if (player->mo->movefactor != FRACUNIT) // Friction-scaled acceleration...
			movepushforward = FixedMul(movepushforward, player->mo->movefactor);

		if (player->curshield != KSHIELD_TOP)
		{
			INT32 a = K_GetUnderwaterTurnAdjust(player);
			INT32 adj = 0;

			if (a)
			{
				const fixed_t maxadj = ANG10/4;

				adj = a / 4;

				if (adj > 0)
				{
					if (adj > maxadj)
						adj = maxadj;
				}
				else if (adj < 0)
				{
					if (adj < -(maxadj))
						adj = -(maxadj);
				}

				if (abs(player->underwatertilt + adj) > abs(a))
					adj = (a - player->underwatertilt);

				if (abs(a) < abs(player->underwatertilt))
					adj = 0;

				movepushangle += a;
			}

			if (adj)
			{
				player->underwatertilt += adj;

				if (abs(player->underwatertilt) > ANG30)
				{
					player->underwatertilt =
						player->underwatertilt > 0 ? ANG30
						: -(ANG30);
				}
			}
			else
			{
				player->underwatertilt =
					FixedMul(player->underwatertilt,
							7*FRACUNIT/8);
			}
		}

		totalthrust.x += P_ReturnThrustX(player->mo, movepushangle, movepushforward);
		totalthrust.y += P_ReturnThrustY(player->mo, movepushangle, movepushforward);
	}

	if ((totalthrust.x || totalthrust.y)
		&& player->mo->standingslope != NULL
		&& (!(player->mo->standingslope->flags & SL_NOPHYSICS))
		&& abs(player->mo->standingslope->zdelta) > FRACUNIT/2)
	{
		// Factor thrust to slope, but only for the part pushing up it!
		// The rest is unaffected.
		angle_t thrustangle = R_PointToAngle2(0, 0, totalthrust.x, totalthrust.y) - player->mo->standingslope->xydirection;

		if (player->mo->standingslope->zdelta < 0)
		{
			// Direction goes down, so thrustangle needs to face toward
			if (thrustangle < ANGLE_90 || thrustangle > ANGLE_270)
			{
				P_QuantizeMomentumToSlope(&totalthrust, player->mo->standingslope);
			}
		}
		else
		{
			// Direction goes up, so thrustangle needs to face away
			if (thrustangle > ANGLE_90 && thrustangle < ANGLE_270)
			{
				P_QuantizeMomentumToSlope(&totalthrust, player->mo->standingslope);
			}
		}
	}

	player->mo->momx += totalthrust.x;
	player->mo->momy += totalthrust.y;

	// Releasing a drift while on the Top translates all your
	// momentum (and even then some) into whichever direction
	// you're facing
	if (onground && player->curshield == KSHIELD_TOP && (K_GetKartButtons(player) & BT_DRIFT) != BT_DRIFT && (player->oldcmd.buttons & BT_DRIFT))
	{
		const fixed_t gmin = FRACUNIT/4;
		const fixed_t gmax = 3*FRACUNIT;

		const fixed_t grindfactor = (gmax - gmin) / GARDENTOP_MAXGRINDTIME;
		const fixed_t grindscale = gmin + (player->topdriftheld * grindfactor);

		const fixed_t speed = R_PointToDist2(0, 0, player->mo->momx, player->mo->momy);
		const fixed_t minspeed = 3 * K_GetKartSpeed(player, false, false) / 5; // 60% top speed

		P_InstaThrust(player->mo, player->mo->angle, FixedMul(max(speed, minspeed), grindscale));

		player->topdriftheld = 0;/* reset after release */
	}

	if (!onground)
	{
		const fixed_t airspeedcap = (50*mapobjectscale);
		const fixed_t speed = R_PointToDist2(0, 0, player->mo->momx, player->mo->momy);

		// If you're going too fast in the air, ease back down to a certain speed.
		// Helps lots of jumps from breaking when using speed items, since you can't move in the air.
		if (speed > airspeedcap)
		{
			fixed_t div = 32*FRACUNIT;
			fixed_t newspeed;

			// Make rubberbanding bots slow down faster
			if (K_PlayerUsesBotMovement(player) && player->dashpadcooldown == 0)
			{
				fixed_t rubberband = player->botvars.rubberband - FRACUNIT;

				if (rubberband > 0)
				{
					div = FixedDiv(div, FRACUNIT + (rubberband * 2));
				}
			}

			newspeed = speed - FixedDiv((speed - airspeedcap), div);

			player->mo->momx = FixedMul(FixedDiv(player->mo->momx, speed), newspeed);
			player->mo->momy = FixedMul(FixedDiv(player->mo->momy, speed), newspeed);
		}
	}

	// Time to ask three questions:
	// 1) Are we over topspeed?
	// 2) If "yes" to 1, were we moving over topspeed to begin with?
	// 3) If "yes" to 2, are we now going faster?

	// If "yes" to 3, normalize to our initial momentum; this will allow thoks to stay as fast as they normally are.
	// If "no" to 3, ignore it; the player might be going too fast, but they're slowing down, so let them.
	// If "no" to 2, normalize to topspeed, so we can't suddenly run faster than it of our own accord.
	// If "no" to 1, we're not reaching any limits yet, so ignore this entirely!
	// -Shadow Hog
	// Only do this forced cap of speed when in midair, the kart acceleration code takes into account friction, and
	// doesn't let you accelerate past top speed, so this is unnecessary on the ground, but in the air is needed to
	// allow for being able to change direction on spring jumps without being accelerated into the void - Sryder
	if (!P_IsObjectOnGround(player->mo))
	{
		fixed_t topspeed = K_GetKartSpeed(player, true, true);
		newMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);
		if (newMagnitude > topspeed)
		{
			fixed_t tempmomx, tempmomy;
			if (oldMagnitude > topspeed)
			{
				if (newMagnitude > oldMagnitude)
				{
					tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), oldMagnitude);
					tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), oldMagnitude);
					player->mo->momx = tempmomx + player->cmomx;
					player->mo->momy = tempmomy + player->cmomy;
				}
				// else do nothing
			}
			else
			{
				tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), topspeed);
				tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), topspeed);
				player->mo->momx = tempmomx + player->cmomx;
				player->mo->momy = tempmomy + player->cmomy;
			}
		}
	}
}

// For turning correction in P_UpdatePlayerAngle.
// Given a range of possible steering inputs, finds a steering input that corresponds to the desired angle change.
static INT16 P_FindClosestTurningForAngle(player_t *player, INT32 targetAngle, INT16 lowBound, INT16 highBound)
{
	INT16 newBound;
	INT16 preferred = lowBound;
	int attempts = 0;

	// Only works if our low bound is actually our low bound.
	if (highBound < lowBound)
	{
		INT16 tmp = lowBound;
		lowBound = highBound;
		highBound = tmp;
	}

	// Slightly frumpy binary search for the ideal turning input.
	// We do this instead of reversing K_GetKartTurnValue so that future handling changes are automatically accounted for.

	while (attempts++ < 20) // Practical calls of this function search maximum 10 times, this is solely for safety.
	{
		// These need to be treated as signed, or situations where boundaries straddle 0 are a mess.
		INT32 lowAngle = K_GetKartTurnValue(player, lowBound) << TICCMD_REDUCE;
		INT32 highAngle = K_GetKartTurnValue(player, highBound) << TICCMD_REDUCE;

		// EXIT CONDITION 1: Hopeless search, target angle isn't between boundaries at all.
		if (lowAngle >= targetAngle)
			return lowBound;
		if (highAngle <= targetAngle)
			return highBound;

		// Test the middle of our steering range, so we can see which side is more promising.
		newBound = (lowBound + highBound) / 2;

		// EXIT CONDITION 2: Boundaries converged and we're all out of precision.
		if (newBound == lowBound || newBound == highBound)
			break;

		INT32 newAngle = K_GetKartTurnValue(player, newBound) << TICCMD_REDUCE;

		angle_t lowError = abs(targetAngle - lowAngle);
		angle_t highError = abs(targetAngle - highAngle);
		angle_t newError = abs(targetAngle - newAngle);

		// CONS_Printf("steering %d / %d / %d - angle %d / %d / %d - TA %d - error %d / %d / %d\n", lowBound, newBound, highBound, lowAngle, newAngle, highAngle, targetAngle, lowError, newError, highError);

		// EXIT CONDITION 3: We got lucky!
		if (lowError == 0)
			return lowBound;
		if (newError == 0)
			return newBound;
		if (highError == 0)
			return highBound;

		// If not, store the best estimate...
		if (lowError <= newError && lowError <= highError)
			preferred = lowBound;
		if (highError <= newError && highError <= lowError)
			preferred = highBound;
		if (newError <= lowError && newError <= highError)
			preferred = newBound;

		// ....and adjust the bounds for another run.
		if (lowAngle <= targetAngle && targetAngle <= newAngle)
			highBound = newBound;
		else
			lowBound = newBound;
	}

	return preferred;
}

//
// P_UpdatePlayerAngle
//
// Updates player angleturn with cmd->turning
//
static void P_UpdatePlayerAngle(player_t *player)
{
	angle_t angleChange = ANGLE_MAX;
	UINT8 p = UINT8_MAX;
	UINT8 i;

	for (i = 0; i <= splitscreen; i++)
	{
		if (player == &players[g_localplayers[i]])
		{
			p = i;
			break;
		}
	}

	// Don't apply steering just yet. If we make a correction, we'll need to adjust it.
	INT16 targetsteering = K_UpdateSteeringValue(player->steering, player->cmd.turning);
	angleChange = K_GetKartTurnValue(player, targetsteering) << TICCMD_REDUCE;

	if (K_PlayerUsesBotMovement(player))
	{
		// You're a bot. Go where you're supposed to go
		player->steering = targetsteering;
	}
	else if ((!(player->cmd.flags & TICCMD_RECEIVED)) && (!!(player->oldcmd.flags && TICCMD_RECEIVED)))
	{
		// Missed a single tic. This ticcmd is copied from their previous one
		// (less the TICCMD_RECEIVED flag), so it will include an old angle, and
		// steering towards that will turn unambitiously. A better guess is to
		// assume their inputs are the same, and turn based on those for 1 tic.
		player->steering = targetsteering;
		// "Why not use this for multiple consecutive dropped tics?" Oversimplification:
		// Clients have default netticbuffer 1, so missing more than 1 tic will freeze
		// your client, and with it, your local camera. Our goal then becomes not to
		// steer PAST the angle you can see, so the default turn solver behavior is best.
	}
	else
	{
		// With a full slam on the analog stick, how far could we steer in either direction?
		INT16 steeringRight =  K_UpdateSteeringValue(player->steering, KART_FULLTURN);
		INT16 steeringLeft =  K_UpdateSteeringValue(player->steering, -KART_FULLTURN);

		// When entering/leaving drifts, allow all legal turns with no easing.
		// This is the hardest case for the turn solver, because your handling properties on
		// client side are very different than your handling properties on server side—at least,
		// until your drift status makes the full round-trip and is reflected in your gamestate.
		if (player->drift && abs(player->drift) < 5)
		{
			steeringRight = KART_FULLTURN;
			steeringLeft = -KART_FULLTURN;
		}

		angle_t maxTurnRight = K_GetKartTurnValue(player, steeringRight) << TICCMD_REDUCE;
		angle_t maxTurnLeft = K_GetKartTurnValue(player, steeringLeft) << TICCMD_REDUCE;

		// Grab local camera angle from ticcmd. Where do we actually want to go?
		angle_t targetAngle = (player->cmd.angle) << TICCMD_REDUCE;
		angle_t targetDelta = targetAngle - (player->mo->angle);

		// Corrections via fake turn go through easing.
		// That means undoing them takes the same amount of time as doing them.
		// This can lead to oscillating death spiral states on a multi-tic correction, as we swing past the target angle.
		// So before we go into death-spirals, if our predicton is _almost_ right...
		angle_t leniency_base;
		if (G_CompatLevel(0x000A))
		{
			// Compat level for 2.0 staff ghosts
			leniency_base = 4 * ANG1 / 3;
		}
		else
		{
			leniency_base = 8 * ANG1 / 3;
		}

		// Gross. Take a look at sliptide starts properly for 2.4.
		// Yell at Tyron!
		if (!G_CompatLevel(0x000C))
		{
			leniency_base = 6 * ANG1 / 3;
		}

		angle_t leniency = leniency_base * min(player->cmd.latency, 6);
		// Don't force another turning tic, just give them the desired angle!

		if (targetDelta == angleChange || (maxTurnRight == 0 && maxTurnLeft == 0))
		{
			// Either we're dead on or we can't steer at all.
			player->steering = targetsteering;
		}
		else
		{
			// We're off. Try to legally steer the player towards their camera.

			if (K_Sliptiding(player) && P_IsObjectOnGround(player->mo) && (player->cmd.turning != 0) && ((player->cmd.turning > 0) == (player->aizdriftstrat > 0)))
			{
				// Don't change handling direction if someone's inputs are sliptiding, you'll break the sliptide!
				if (player->cmd.turning > 0)
				{
					steeringLeft = max(steeringLeft, 1);
					steeringRight = max(steeringRight, steeringLeft);
				}
				else
				{
					steeringRight = min(steeringRight, -1);
					steeringLeft = min(steeringLeft, steeringRight);
				}
			}

			player->steering = P_FindClosestTurningForAngle(player, targetDelta, steeringLeft, steeringRight);
			angleChange = K_GetKartTurnValue(player, player->steering) << TICCMD_REDUCE;

			// And if the resulting steering input is close enough, snap them exactly.
			if (min(targetDelta - angleChange, angleChange - targetDelta) <= leniency)
				angleChange = targetDelta;
		}
	}

	if (p == UINT8_MAX)
	{
		// When F12ing players, set local angle directly.
		P_SetPlayerAngle(player, player->angleturn + angleChange);
		player->mo->angle = player->angleturn;
	}
	else
	{
		player->angleturn += angleChange;
		player->mo->angle = player->angleturn;
	}

	if (!cv_allowmlook.value || player->spectator == false)
	{
		player->aiming = 0;
	}
	else
	{
		player->aiming += (player->cmd.aiming << TICCMD_REDUCE);
		player->aiming = G_ClipAimingPitch((INT32 *)&player->aiming);
	}

	if (p != UINT8_MAX)
	{
		localaiming[p] = player->aiming;
	}
}

//
// P_MovePlayer
void P_MovePlayer(player_t *player)
{
	ticcmd_t *cmd;
	//INT32 i;

	fixed_t runspd;

	cmd = &player->cmd;
	runspd = 14*player->mo->scale; //srb2kart

	// Let's have some movement speed fun on low-friction surfaces, JUST for players...
	// (high friction surfaces shouldn't have any adjustment, since the acceleration in
	// this game is super high and that ends up cheesing high-friction surfaces.)
	runspd = FixedMul(runspd, player->mo->movefactor);

	// Control relinquishing stuff!
	if (player->nocontrol || player->respawn.state == RESPAWNST_MOVE)
		player->pflags |= PF_STASIS;

	// note: don't unset stasis here

	if (player->spectator)
	{
		player->mo->eflags &= ~MFE_VERTICALFLIP; // deflip...
		return;
	}

	//////////////////////
	// MOVEMENT CODE	//
	//////////////////////

	P_UpdatePlayerAngle(player);

	ticruned++;
	if (!(cmd->flags & TICCMD_RECEIVED))
		ticmiss++;

	P_3dMovement(player);

	if (cmd->turning == 0)
	{
		player->justDI = 0;
	}

	// Kart frames
	if (player->icecube.frozen)
	{
		INT32 spd = FixedMul(player->mo->scale, FixedHypot(player->mo->momx, player->mo->momy)) / FRACUNIT;
		P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
		player->drawangle -= max(2, spd / 6) * ANG1;
		P_ResetPitchRoll(player->mo);
	}
	else if (player->tumbleBounces > 0)
	{
		fixed_t playerSpeed = P_AproxDistance(player->mo->momx, player->mo->momy); // maybe momz too?

		const UINT8 minSpinSpeed = 4;
		UINT8 spinSpeed = max(minSpinSpeed, min(8 + minSpinSpeed, (playerSpeed / player->mo->scale) * 2));

		UINT8 rollSpeed = max(1, min(8, player->tumbleHeight / 10));

		if (player->pflags & PF_TUMBLELASTBOUNCE)
			spinSpeed = 2;

		P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
		player->drawangle -= (ANGLE_11hh * spinSpeed);

		player->mo->rollangle -= (ANGLE_11hh * rollSpeed);

		if (player->pflags & PF_TUMBLELASTBOUNCE)
		{
			if (abs((signed)(player->mo->angle - player->drawangle)) < ANGLE_22h)
				player->drawangle = player->mo->angle;

			if (abs((signed)player->mo->rollangle) < ANGLE_22h)
				player->mo->rollangle = 0;
		}
	}
	else if (player->carry == CR_SLIDING)
	{
		P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
		player->drawangle -= ANGLE_22h;
		player->mo->rollangle = 0;
		player->glanceDir = 0;
		player->pflags &= ~PF_GAINAX;
	}
	else if ((player->pflags & PF_FAULT) || (player->spinouttimer > 0))
	{
		UINT16 speed = ((player->pflags & PF_FAULT) ? player->nocontrol : player->spinouttimer)/8;
		if (speed > 8)
			speed = 8;
		else if (speed < 1)
			speed = 1;

		P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);

		if (speed == 1 && abs((signed)(player->mo->angle - player->drawangle)) < ANGLE_22h)
			player->drawangle = player->mo->angle; // Face forward at the end of the animation
		else
			player->drawangle -= (ANGLE_11hh * speed);

		player->mo->rollangle = 0;
	}
	else
	{
		if (player->trickpanel > TRICKSTATE_READY)
		{
			if (player->trickpanel <= TRICKSTATE_RIGHT) // right/forward
			{
				player->drawangle += ANGLE_22h;
			}
			else //if (player->trickpanel >= TRICKSTATE_LEFT) // left/back
			{
				player->drawangle -= ANGLE_22h;
			}
		}
		else
		{
			K_KartMoveAnimation(player);

			player->drawangle = player->mo->angle;

			if (player->aizdriftturn)
			{
				player->drawangle += player->aizdriftturn;
			}
			else if (player->drift != 0)
			{
				INT32 a = (ANGLE_45 / 5) * player->drift;

				if (player->mo->eflags & MFE_UNDERWATER)
					a /= 2;

				player->drawangle += a;
			}
		}

		player->mo->rollangle = 0;
	}

	//{ SRB2kart

	// Drifting sound
	// Start looping the sound now.
	if (leveltime % 50 == 0 && onground && player->drift != 0)
		S_StartSound(player->mo, sfx_drift);
	// Leveltime being 50 might take a while at times. We'll start it up once, isntantly.
	else if (!S_SoundPlaying(player->mo, sfx_drift) && onground && player->drift != 0)
		S_StartSound(player->mo, sfx_drift);
	// Ok, we'll stop now.
	else if (player->drift == 0)
		S_StopSoundByID(player->mo, sfx_drift);

	K_MoveKartPlayer(player, onground);
	//}

//////////////////
//GAMEPLAY STUFF//
//////////////////

	////////////////////////////
	//SPINNING AND SPINDASHING//
	////////////////////////////

	// SRB2kart - Drifting smoke and fire
	if ((player->sneakertimer || player->flamedash)
		&& onground && (leveltime & 1))
		K_SpawnBoostTrail(player);

	if (player->invincibilitytimer > 0)
		K_SpawnSparkleTrail(player->mo);

	if (player->wipeoutslow > 1 && (leveltime & 1))
		K_SpawnWipeoutTrail(player->mo);

	K_DriftDustHandling(player->mo);

	// Crush test...
	if ((player->mo->ceilingz - player->mo->floorz < player->mo->height) && !(player->mo->flags & MF_NOCLIP))
	{
		if (player->spectator)
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_SPECTATOR); // Respawn crushed spectators
		else
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_CRUSHED);

		if (player->playerstate == PST_DEAD)
			return;
	}

#ifdef FLOORSPLATS
	if (cv_shadow.value && rendermode == render_soft)
		R_AddFloorSplat(player->mo->subsector, player->mo, "SHADOW", player->mo->x,
			player->mo->y, player->mo->floorz, SPLATDRAWMODE_OPAQUE);
#endif

	// Look for blocks to bust up
	// Because of FF_SHATTER, we should look for blocks constantly,
	// not just when spinning or playing as Knuckles
	if (CheckForBustableBlocks)
		P_CheckBustableBlocks(player);

	// Check for a BOUNCY sector!
	if (CheckForBouncySector)
		P_CheckBouncySectors(player);

	// Look for Quicksand!
	if (CheckForQuicksand)
		P_CheckQuicksand(player);
}

static void P_DoZoomTube(player_t *player)
{
	fixed_t speed;
	mobj_t *waypoint = NULL;
	fixed_t dist;
	boolean reverse;

	if (player->speed > 0)
		reverse = false;
	else
		reverse = true;

	player->flashing = 1;

	speed = abs(player->speed);

	// change slope
	dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - player->mo->z);

	if (dist < 1)
		dist = 1;

	player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
	player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
	player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - player->mo->z, dist), (speed));

	// Calculate the distance between the player and the waypoint
	// 'dist' already equals this.

	// Will the player go past the waypoint?
	if (speed > dist)
	{
		speed -= dist;
		// If further away, set XYZ of player to waypoint location
		P_UnsetThingPosition(player->mo);
		player->mo->x = player->mo->tracer->x;
		player->mo->y = player->mo->tracer->y;
		player->mo->z = player->mo->tracer->z;
		P_SetThingPosition(player->mo);

		// ugh, duh!!
		player->mo->floorz = player->mo->subsector->sector->floorheight;
		player->mo->ceilingz = player->mo->subsector->sector->ceilingheight;

		CONS_Debug(DBG_GAMELOGIC, "Looking for next waypoint...\n");

		// Find next waypoint
		waypoint = reverse ? P_GetPreviousTubeWaypoint(player->mo->tracer, false) : P_GetNextTubeWaypoint(player->mo->tracer, false);

		if (waypoint)
		{
			CONS_Debug(DBG_GAMELOGIC, "Found waypoint (sequence %d, number %d).\n", waypoint->threshold, waypoint->health);

			P_SetTarget(&player->mo->tracer, waypoint);

			// calculate MOMX/MOMY/MOMZ for next waypoint

			// change slope
			dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - player->mo->z);

			if (dist < 1)
				dist = 1;

			player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
			player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
			player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - player->mo->z, dist), (speed));
		}
		else
		{
			P_SetTarget(&player->mo->tracer, NULL); // Else, we just let them fly.
			player->carry = CR_NONE;

			CONS_Debug(DBG_GAMELOGIC, "Next waypoint not found, releasing from track...\n");
		}
	}

	// change angle
	if (player->mo->tracer)
	{
		player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, player->mo->tracer->x, player->mo->tracer->y);
		P_SetPlayerAngle(player, player->mo->angle);
	}

	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
	player->drawangle -= ANGLE_22h;
}

#if 0
//
// P_NukeAllPlayers
//
// Hurts all players
// source = guy who gets the credit
//
static void P_NukeAllPlayers(player_t *player)
{
	mobj_t *mo;
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		if (players[i].spectator)
			continue;
		if (!players[i].mo)
			continue;
		if (players[i].mo == player->mo)
			continue;
		if (players[i].mo->health <= 0)
			continue;

		P_DamageMobj(players[i].mo, player->mo, player->mo, 1, DMG_NORMAL);
	}

	CONS_Printf(M_GetText("%s caused a world of pain.\n"), player_names[player-players]);

	return;
}
#endif

//
// P_NukeEnemies
// Looks for something you can hit - Used for bomb shield
//
void P_NukeEnemies(mobj_t *inflictor, mobj_t *source, fixed_t radius)
{
	mobj_t *mo;
	thinker_t *think;

	radius = FixedMul(radius, mapobjectscale);

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)think;

		if (!(mo->flags & MF_SHOOTABLE) && (mo->type != MT_SPB)) // Don't want to give SPB MF_SHOOTABLE, to ensure it's undamagable through other means
			continue;

		if (abs(inflictor->x - mo->x) > radius || abs(inflictor->y - mo->y) > radius || abs(inflictor->z - mo->z) > radius)
			continue; // Workaround for possible integer overflow in the below -Red

		if (P_AproxDistance(P_AproxDistance(inflictor->x - mo->x, inflictor->y - mo->y), inflictor->z - mo->z) > radius)
			continue;

		if (mo->type == MT_SPB) // If you destroy a SPB, you don't get the luxury of a cooldown.
		{
			spbplace = -1;
			itemCooldowns[KITEM_SPB - 1] = 0;
		}

		if (mo->flags & MF_BOSS) //don't OHKO bosses nor players!
			P_DamageMobj(mo, inflictor, source, 1, DMG_NORMAL|DMG_CANTHURTSELF);
		else if (mo->type == MT_PLAYER)	// Lightning shield: Combo players.
			P_DamageMobj(mo, inflictor, source, 1, DMG_NORMAL|DMG_CANTHURTSELF|DMG_WOMBO);
		else
			P_DamageMobj(mo, inflictor, source, 1000, DMG_NORMAL|DMG_CANTHURTSELF);
	}
}

//
// P_ConsiderAllGone
// Shamelessly lifted from TD. Thanks, Sryder!
//

// SRB2Kart: Use for GP?
/*
static void P_ConsiderAllGone(void)
{
	INT32 i, lastdeadplayer = -1, deadtimercheck = INT32_MAX;

	if (countdown2)
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (players[i].playerstate != PST_DEAD && !players[i].spectator && players[i].mo && players[i].mo->health)
			break;

		if (players[i].spectator)
		{
			if (lastdeadplayer == -1)
				lastdeadplayer = i;
		}
		else if (players[i].lives > 0)
		{
			lastdeadplayer = i;
			if (players[i].deadtimer < deadtimercheck)
				deadtimercheck = players[i].deadtimer;
		}
	}

	if (i == MAXPLAYERS && lastdeadplayer != -1 && deadtimercheck > 2*TICRATE) // the last killed player will reset the level in G_DoReborn
	{
		//players[lastdeadplayer].spectator = true;
		players[lastdeadplayer].outofcoop = true;
		players[lastdeadplayer].playerstate = PST_REBORN;
	}
}
*/

//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//
static void P_DeathThink(player_t *player)
{
	boolean playerGone = false;

	player->deltaviewheight = 0;

	if (player->deadtimer < INT32_MAX)
		player->deadtimer++;

	if ((player->pflags & PF_NOCONTEST) && (gametyperules & GTR_CIRCUIT))
	{
		player->karthud[khud_timeovercam]++;

		if (player->mo)
		{
			player->mo->flags |= (MF_NOGRAVITY|MF_NOCLIP);
		}
	}
	else
		player->karthud[khud_timeovercam] = 0;

	if (player->pflags & PF_NOCONTEST)
	{
		playerGone = true;
	}
	else if (player->bot == false)
	{
		if (G_GametypeUsesLives() == true && player->lives == 0)
		{
			playerGone = true;
		}
	}

	if ((player->pflags & PF_ELIMINATED) || exitcountdown)
	{
		playerGone = true;
	}

	if (playerGone == false && player->deadtimer > TICRATE)
	{
		if (!netgame && !splitscreen
		&& player->bot == false
#ifdef DEVELOP
		&& player->spectator == false
#endif
		&& (gametyperules & GTR_CHECKPOINTS))
		{
			G_SetRetryFlag();
		}
		else
		{
			player->playerstate = PST_REBORN;
		}
	}

	// Spectate another player after 2 seconds
	if (G_IsPartyLocal(player - players) && playerGone == true &&
		(gametyperules & GTR_BUMPERS) && battleprisons == false &&
		player->deadtimer == 2*TICRATE)
	{
		K_ToggleDirector(G_PartyPosition(player - players), true);
	}

	if (!player->mo)
		return;

	//K_KartResetPlayerColor(player); -- called at death, don't think we need to re-establish

	P_CalcHeight(player);
}

//
// P_MoveCamera: make sure the camera is not outside the world and looks at the player avatar
//

camera_t camera[MAXSPLITSCREENPLAYERS]; // Four cameras, three for splitscreen

void CV_CamRotate_OnChange(void);
void CV_CamRotate_OnChange(void)
{
	if (cv_cam_rotate[0].value < 0)
		CV_SetValue(&cv_cam_rotate[0], cv_cam_rotate[0].value + 360);
	else if (cv_cam_rotate[0].value > 359)
		CV_SetValue(&cv_cam_rotate[0], cv_cam_rotate[0].value % 360);
}

void CV_CamRotate2_OnChange(void);
void CV_CamRotate2_OnChange(void)
{
	if (cv_cam_rotate[1].value < 0)
		CV_SetValue(&cv_cam_rotate[1], cv_cam_rotate[1].value + 360);
	else if (cv_cam_rotate[1].value > 359)
		CV_SetValue(&cv_cam_rotate[1], cv_cam_rotate[1].value % 360);
}

void CV_CamRotate3_OnChange(void);
void CV_CamRotate3_OnChange(void)
{
	if (cv_cam_rotate[2].value < 0)
		CV_SetValue(&cv_cam_rotate[2], cv_cam_rotate[2].value + 360);
	else if (cv_cam_rotate[2].value > 359)
		CV_SetValue(&cv_cam_rotate[2], cv_cam_rotate[2].value % 360);
}

void CV_CamRotate4_OnChange(void);
void CV_CamRotate4_OnChange(void)
{
	if (cv_cam_rotate[3].value < 0)
		CV_SetValue(&cv_cam_rotate[3], cv_cam_rotate[3].value + 360);
	else if (cv_cam_rotate[3].value > 359)
		CV_SetValue(&cv_cam_rotate[3], cv_cam_rotate[3].value % 360);
}

fixed_t t_cam_dist[MAXSPLITSCREENPLAYERS] = {-42,-42,-42,-42};
fixed_t t_cam_height[MAXSPLITSCREENPLAYERS] = {-42,-42,-42,-42};
fixed_t t_cam_rotate[MAXSPLITSCREENPLAYERS] = {-42,-42,-42,-42};

void P_DemoCameraMovement(camera_t *cam, UINT8 num)
{
	extern consvar_t cv_freecam_speed;

	ticcmd_t *cmd;
	angle_t thrustangle;
	player_t *lastp;
	angle_t turning;

	boolean moving = false;

	// first off we need to get button input
	cmd = D_LocalTiccmd(num);

	if (cmd->aiming != 0)
	{
		cam->aiming += cmd->aiming << TICCMD_REDUCE;

		cam->reset_aiming = false;
	}

	turning = cmd->turning << TICCMD_REDUCE;
	if (encoremode)
	{
		turning = -turning;
	}
	cam->angle += turning;

	// camera movement:
	if (!cam->button_a_held && cv_freecam_speed.value)
	{
		int dir = ((cmd->buttons & BT_ACCELERATE) ? 1 : 0) + ((cmd->buttons & BT_BRAKE) ? -1 : 0);

		fixed_t spd = 32*mapobjectscale;
		if (cv_freecam_speed.value > 1)
			spd *= cv_freecam_speed.value;
		else if (cv_freecam_speed.value < -1)
			spd /= -cv_freecam_speed.value;

		switch (dir)
		{
			case 1:
				cam->z += spd;
				moving = true;
				break;

			case -1:
				cam->z -= spd;
				moving = true;
				break;
		}
	}

	if (!(cmd->buttons & (BT_ACCELERATE | BT_DRIFT)) && cam->button_a_held)
	{
		cam->button_a_held--;
	}

	// if you hold Y, you will lock on to displayplayer. (The last player you were ""f12-ing"")
	if (cam->freecam && cmd->buttons & BT_RESPAWN)
	{
		lastp = &players[displayplayers[0]];	// Fun fact, I was trying displayplayers[0]->mo as if it was Lua like an absolute idiot.
		cam->angle = R_PointToAngle2(cam->x, cam->y, lastp->mo->x, lastp->mo->y);
		cam->aiming = R_PointToAngle2(0, cam->z, R_PointToDist2(cam->x, cam->y, lastp->mo->x, lastp->mo->y), lastp->mo->z + lastp->mo->scale*128*P_MobjFlip(lastp->mo));	// This is still unholy. Aim a bit above their heads.

		cam->reset_aiming = false;
	}

	if (cmd->forwardmove != 0)
	{
		moving = true;
	}

	// After switching to democam, the vertical angle of
	// chasecam is inherited. This is intentional because it
	// creates a smooth transition. However, moving
	// forward/back will have a slope. So, as long as democam
	// controls haven't been used to alter the vertical angle,
	// slowly reset it to flat.
	if ((cam->reset_aiming && moving) || ((cmd->buttons & BT_DRIFT) && !cam->button_a_held))
	{
		INT32 aiming = cam->aiming;
		INT32 smooth = FixedMul(ANGLE_11hh / 4, FCOS(cam->aiming));

		if (abs(smooth) < abs(aiming))
		{
			cam->aiming -= smooth * intsign(aiming);
		}
		else
		{
			cam->aiming = 0;
			cam->reset_aiming = false; // completely smoothed out
		}
	}

	if (rendermode == render_soft
#ifdef HWRENDER
		|| (rendermode == render_opengl && (cv_glshearing.value == 1))
#endif
		)
	{
		// Extra restriction on this so it's not possible to
		// distort the view too much.
		if ((INT32)cam->aiming > ANGLE_45)
			cam->aiming = ANGLE_45;
		else if ((INT32)cam->aiming < -ANGLE_45)
			cam->aiming = -ANGLE_45;
	}
	else
	{
		G_ClipAimingPitch((INT32 *)&cam->aiming);
	}

	cam->momx = cam->momy = cam->momz = 0;

	if (cmd->forwardmove != 0 && cv_freecam_speed.value)
	{
		fixed_t spd = cmd->forwardmove*mapobjectscale;
		if (cv_freecam_speed.value > 1)
			spd *= cv_freecam_speed.value;
		else if (cv_freecam_speed.value < -1)
			spd /= -cv_freecam_speed.value;

		thrustangle = cam->angle >> ANGLETOFINESHIFT;

		cam->x += FixedMul(spd, FINECOSINE(thrustangle));
		cam->y += FixedMul(spd, FINESINE(thrustangle));

		if (!cam->reset_aiming)
		{
			cam->z += FixedMul(spd, AIMINGTOSLOPE(cam->aiming));
		}
		// momentums are useless here, directly add to the coordinates

		// this.......... doesn't actually check for floors and walls and whatnot but the function to do that is a pure mess so fuck that.
		// besides freecam going inside walls sounds pretty cool on paper.
	}

	// update subsector to avoid crashes;
	cam->subsector = R_PointInSubsector(cam->x, cam->y);
}

void P_ToggleDemoCamera(UINT8 viewnum)
{
	camera_t *cam = &camera[viewnum];

	if (!cam->freecam)	// toggle on
	{
		cam->freecam = true;
		cam->button_a_held = 2;
		cam->reset_aiming = true;
	}
	else	// toggle off
	{
		cam->freecam = false;
	}
}

void P_ResetCamera(player_t *player, camera_t *thiscam)
{
	if (g_endcam.active)
	{
		return;
	}

	tic_t tries = 0;
	fixed_t x, y, z;

	if (thiscam->freecam)
		return;	// do not reset the camera there.

	if (!player->mo)
		return;

	if (thiscam->chase && player->mo->health <= 0)
		return;

	thiscam->chase = !player->spectator;
	x = player->mo->x - P_ReturnThrustX(player->mo, thiscam->angle, player->mo->radius);
	y = player->mo->y - P_ReturnThrustY(player->mo, thiscam->angle, player->mo->radius);
	if (player->mo->eflags & MFE_VERTICALFLIP)
		z = player->mo->z + player->mo->height - P_GetPlayerViewHeight(player) - 16*FRACUNIT;
	else
		z = player->mo->z + P_GetPlayerViewHeight(player);

	// set bits for the camera
	thiscam->x = x;
	thiscam->y = y;
	thiscam->z = z;
	thiscam->centerfloorz = player->mo->floorz;
	thiscam->centerceilingz = player->mo->ceilingz;

	thiscam->angle = player->mo->angle;
	thiscam->aiming = 0;
	thiscam->relativex = 0;

	thiscam->subsector = R_PointInSubsector(thiscam->x,thiscam->y);

	thiscam->radius = 20*FRACUNIT;
	thiscam->height = 16*FRACUNIT;

	thiscam->reset_aiming = true;

	while (!P_MoveChaseCamera(player,thiscam,true) && ++tries < 2*TICRATE);
}

boolean P_MoveChaseCamera(player_t *player, camera_t *thiscam, boolean resetcalled)
{
	static boolean lookbackactive[MAXSPLITSCREENPLAYERS];
	static UINT8 lookbackdelay[MAXSPLITSCREENPLAYERS];
	UINT8 num;
	angle_t angle = 0, focusangle = 0, focusaiming = 0, pitch = 0;
	fixed_t x, y, z, dist, distxy, distz, viewpointx, viewpointy, camspeed, camdist, camheight, pviewheight;
	fixed_t pan, xpan, ypan;
	INT32 camrotate;
	boolean camstill, lookback, lookbackdown;
	UINT8 timeover;
	mobj_t *mo;
	fixed_t f1, f2;
	fixed_t speed;

	fixed_t playerScale;
	fixed_t scaleDiff;
	fixed_t cameraScale = mapobjectscale;

	sonicloopcamvars_t *loop = &player->loop.camera;
	tic_t loop_out = leveltime - loop->enter_tic;
	tic_t loop_in = max(leveltime, loop->exit_tic) - loop->exit_tic;

	thiscam->old_x = thiscam->x;
	thiscam->old_y = thiscam->y;
	thiscam->old_z = thiscam->z;
	thiscam->old_angle = thiscam->angle;
	thiscam->old_aiming = thiscam->aiming;

	// We probably shouldn't move the camera if there is no player or player mobj somehow
	if (!player || !player->mo)
		return true;

	// This can happen when joining
	if (thiscam->subsector == NULL || thiscam->subsector->sector == NULL)
		return true;

	if (thiscam == &camera[1]) // Camera 2
	{
		num = 1;
	}
	else if (thiscam == &camera[2]) // Camera 3
	{
		num = 2;
	}
	else if (thiscam == &camera[3]) // Camera 4
	{
		num = 3;
	}
	else // Camera 1
	{
		num = 0;
	}

	if ((thiscam->freecam || player->spectator) && !g_endcam.active)
	{
		P_DemoCameraMovement(thiscam, num);
		return true;
	}

	if (paused || P_AutoPause())
		return true;

	if (g_endcam.active)
	{
		K_MoveEndCamera(thiscam);
		return true;
	}

	playerScale = FixedDiv(player->mo->scale, mapobjectscale);
	scaleDiff = playerScale - FRACUNIT;

	mo = player->mo;

	if (P_MobjIsFrozen(mo) || player->playerstate == PST_DEAD || F_CreditsDemoExitFade() >= 0)
	{
		// Do not move the camera while in hitlag!
		// The camera zooming out after you got hit makes it hard to focus on the vibration.
		// of course, if you're in chase, don't forget the postimage - otherwise encore will flip back
		if (thiscam->chase)
			P_CalcChasePostImg(player, thiscam);

		return true;
	}

	if ((player->pflags & PF_NOCONTEST) && (gametyperules & GTR_CIRCUIT) && player->karthud[khud_timeovercam] != 0) // 1 for momentum keep, 2 for turnaround
		timeover = (player->karthud[khud_timeovercam] > 2*TICRATE ? 2 : 1);
	else
		timeover = 0;

	if (!(player->playerstate == PST_DEAD || player->exiting || leveltime < introtime))
	{
		if (player->spectator) // force cam off for spectators
			return true;

		if (!cv_chasecam[num].value && thiscam == &camera[num])
			return true;
	}

	if (!thiscam->chase && !resetcalled)
	{
		focusangle = localangle[num];
		camrotate = cv_cam_rotate[num].value;

		if (leveltime < introtime) // Whoooshy camera!
		{
			const INT32 introcam = (introtime - leveltime);
			camrotate += introcam*5;
		}

		thiscam->angle = focusangle + FixedAngle(camrotate*FRACUNIT);
		P_ResetCamera(player, thiscam);
		return true;
	}

	// Adjust camera to match Grow/Shrink
	cameraScale = FixedMul(cameraScale, FRACUNIT + (scaleDiff / 3));

	thiscam->radius = 20*cameraScale;
	thiscam->height = 16*cameraScale;

	// Don't run while respawning from a cheatcheck
	// Inu 4/8/13 Why not?!
//	if (leveltime > 0 && timeinmap <= 0)
//		return true;

	if (demo.playback)
	{
		if (K_PlayerUsesBotMovement(player))
			focusangle = mo->angle; // Bots don't even send cmd angle; they always turn where they want to!
		else if (leveltime <= introtime)
			focusangle = mo->angle; // Can't turn yet. P_UpdatePlayerAngle will ignore angle in stale ticcmds, chasecam should too.
		else
			focusangle = player->cmd.angle << TICCMD_REDUCE;
		focusaiming = 0;
	}
	else
	{
		focusangle = localangle[num];
		focusaiming = localaiming[num];
	}

	if (abs(thiscam->dpad_y_held) >= 2*TICRATE)
	{
		focusaiming += ANGLE_45 * intsign(thiscam->dpad_y_held) * P_MobjFlip(mo);
	}

	if (P_CameraThinker(player, thiscam, resetcalled))
		return true;

	lookback = ( player->cmd.buttons & BT_LOOKBACK );

	camspeed = cv_cam_speed[num].value;
	camstill = cv_cam_still[num].value || player->seasaw;	// RR: seasaws lock the camera so that it isn't disorienting.
	camrotate = cv_cam_rotate[num].value;
	camdist = FixedMul(cv_cam_dist[num].value, cameraScale);
	camheight = FixedMul(cv_cam_height[num].value, cameraScale);

	if (loop_in < loop->zoom_in_speed)
	{
		fixed_t f = loop_out < loop->zoom_out_speed
			? (loop_out * FRACUNIT) / loop->zoom_out_speed
			: FRACUNIT - ((loop_in * FRACUNIT) / loop->zoom_in_speed);

		camspeed -= FixedMul(f, camspeed - (FRACUNIT/10));
		camdist += FixedMul(f, loop->dist);
	}

	if (loop_in < max(loop->pan_back, 1))
	{
		fixed_t f = (loop_in * FRACUNIT) / max(loop->pan_back, 1);

		fixed_t dx = mo->x - thiscam->x;
		fixed_t dy = mo->y - thiscam->y;

		angle_t th = R_PointToAngle2(0, 0, dx, dy);
		fixed_t d = AngleFixed(focusangle - th);

		if (d > 180*FRACUNIT)
		{
			d -= (360*FRACUNIT);
		}

		focusangle = th + FixedAngle(FixedMul(f, d));

		if (loop_in == 0)
		{
			focusaiming = R_PointToAngle2(0, thiscam->z, FixedHypot(dx, dy), mo->z);
		}
	}

	if (loop_in == 0)
	{
		tic_t accel = max(loop->pan_accel, 1);
		fixed_t f = (min(loop_out, accel) * FRACUNIT) / accel;

		INT32 turn = AngleDeltaSigned(focusangle, player->loop.yaw - loop->pan);
		INT32 turnspeed = FixedAngle(FixedMul(f, loop->pan_speed));

		if (turn > turnspeed)
		{
			// TODO: this code let the panning angle flip
			// depending on your camera angle when entering
			// the loop.
			// It caused just about every loop to look weird
			// sometimes, since the camera could move
			// differently than configured.
			// I don't know if this behavior should ever come
			// back, but in case it should, I'm leaving this
			// comment here.
#if 0
			if (turn < ANGLE_90)
			{
				turnspeed = -(turnspeed);
			}
#endif

			focusangle += turnspeed;
		}
	}

	if (timeover)
	{
		const INT32 timeovercam = max(0, min(180, (player->karthud[khud_timeovercam] - 2*TICRATE)*15));
		camrotate += timeovercam;
	}
	else if (leveltime < introtime && !(modeattacking && !demo.playback)) // Whoooshy camera! (don't do this in RA when we PLAY, still do it in replays however~)
	{
		const INT32 introcam = (introtime - leveltime);
		camrotate += introcam*5;
		camdist += (introcam * cameraScale)*3;
		camheight += (introcam * cameraScale)*2;
	}
	else if (player->exiting) // SRB2Kart: Leave the camera behind while exiting, for dramatic effect!
		camstill = true;
	else if (lookback || lookbackdelay[num]) // SRB2kart - Camera flipper
	{
#define MAXLOOKBACKDELAY 2
		camspeed = FRACUNIT;
		if (lookback)
		{
			camrotate += 180;
			lookbackdelay[num] = MAXLOOKBACKDELAY;
		}
		else
			lookbackdelay[num]--;
	}
	else if (player->respawn.state != RESPAWNST_NONE)
	{
		camspeed = 3*FRACUNIT/4;
	}
	lookbackdown = (lookbackdelay[num] == MAXLOOKBACKDELAY) != lookbackactive[num];
	lookbackactive[num] = (lookbackdelay[num] == MAXLOOKBACKDELAY);
#undef MAXLOOKBACKDELAY

	if (mo->eflags & MFE_VERTICALFLIP)
		camheight += thiscam->height;

	if (camspeed > FRACUNIT)
		camspeed = FRACUNIT;

	if (timeover)
		angle = mo->angle + FixedAngle(camrotate*FRACUNIT);
	else if (leveltime < introtime)
		angle = focusangle + FixedAngle(camrotate*FRACUNIT);
	else if (camstill || resetcalled || player->playerstate == PST_DEAD)
		angle = thiscam->angle;
	else
	{
		if (camspeed == FRACUNIT)
			angle = focusangle + FixedAngle(camrotate<<FRACBITS);
		else
		{
			angle_t input = focusangle + FixedAngle(camrotate<<FRACBITS) - thiscam->angle;
			boolean invert = (input > ANGLE_180);
			if (invert)
				input = InvAngle(input);

			input = FixedAngle(FixedMul(AngleFixed(input), camspeed));
			if (invert)
				input = InvAngle(input);

			angle = thiscam->angle + input;
		}
	}

	/* The Top is Big Large so zoom out */
	if (player->curshield == KSHIELD_TOP)
	{
		camdist += 40 * mapobjectscale;
		camheight += 40 * mapobjectscale;
	}

	if (!resetcalled && (leveltime >= introtime && timeover != 2)
		&& (t_cam_rotate[num] != -42))
	{
		angle = FixedAngle(camrotate*FRACUNIT);
		thiscam->angle = angle;
	}

	// sets ideal cam pos
	{
		const fixed_t speedthreshold = 48*cameraScale;
		const fixed_t olddist = P_AproxDistance(mo->x - thiscam->x, mo->y - thiscam->y);

		fixed_t lag, distoffset;

		dist = camdist;

		if (player->karthud[khud_boostcam])
		{
			dist -= FixedMul(11*dist/16, player->karthud[khud_boostcam]);
		}

		if (player->loop.radius)
		{
			speed = player->speed;
		}
		else
		{
			speed = P_AproxDistance(P_AproxDistance(mo->momx, mo->momy), mo->momz / 16);
		}

		lag = FRACUNIT - ((FixedDiv(speed, speedthreshold) - FRACUNIT) * 2);

		if (lag > FRACUNIT)
		{
			lag = FRACUNIT;
		}

		if (lag < camspeed)
		{
			lag = camspeed;
		}

		distoffset = dist - olddist;
		dist = olddist + FixedMul(distoffset, lag);

		if (dist < 0)
		{
			dist = 0;
		}
	}

	if (mo->standingslope)
	{
		pitch = (angle_t)FixedMul(P_ReturnThrustX(mo, thiscam->angle - mo->standingslope->xydirection, FRACUNIT), (fixed_t)mo->standingslope->zangle);

		if (mo->eflags & MFE_VERTICALFLIP)
		{
			if (pitch >= ANGLE_180)
				pitch = 0;
		}
		else
		{
			if (pitch < ANGLE_180)
				pitch = 0;
		}
	}

	pitch = thiscam->pitch + (angle_t)FixedMul(pitch - thiscam->pitch, camspeed/4);

	if (rendermode == render_opengl && !cv_glshearing.value)
		distxy = FixedMul(dist, FINECOSINE((pitch>>ANGLETOFINESHIFT) & FINEMASK));
	else
		distxy = dist;
	distz = -FixedMul(dist, FINESINE((pitch>>ANGLETOFINESHIFT) & FINEMASK));

	x = mo->x - FixedMul(FINECOSINE((angle>>ANGLETOFINESHIFT) & FINEMASK), distxy);
	y = mo->y - FixedMul(FINESINE((angle>>ANGLETOFINESHIFT) & FINEMASK), distxy);

	// SRB2Kart: set camera panning
	if (camstill || resetcalled || player->playerstate == PST_DEAD || player->loop.radius)
		pan = xpan = ypan = 0;
	else
	{
		if (player->drift != 0)
		{
			fixed_t panmax = (dist/5);
			INT32 driftval = K_GetKartDriftSparkValue(player);
			INT32 dc = player->driftcharge;

			if (dc > driftval || dc < 0)
				dc = driftval;

			pan = FixedDiv(FixedMul((fixed_t)dc, panmax), driftval);

			if (pan > panmax)
				pan = panmax;
			if (player->drift < 0)
				pan *= -1;
		}
		else
			pan = 0;

		pan = thiscam->pan + FixedMul(pan - thiscam->pan, camspeed/4);

		xpan = FixedMul(FINECOSINE(((angle+ANGLE_90)>>ANGLETOFINESHIFT) & FINEMASK), pan);
		ypan = FixedMul(FINESINE(((angle+ANGLE_90)>>ANGLETOFINESHIFT) & FINEMASK), pan);

		x += xpan;
		y += ypan;
	}

	pviewheight = FixedMul(32<<FRACBITS, mo->scale);

	if (mo->eflags & MFE_VERTICALFLIP)
	{
		distz = min(-camheight, distz);
		z = mo->z + mo->height - pviewheight + distz;
	}
	else
	{
		distz = max(camheight, distz);
		z = mo->z + pviewheight + distz;
	}

	z += player->cameraOffset;

	// point viewed by the camera
	// this point is just 64 unit forward the player
	dist = 64*cameraScale;
	viewpointx = mo->x + FixedMul(FINECOSINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist) + xpan;
	viewpointy = mo->y + FixedMul(FINESINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist) + ypan;

	if (timeover)
		thiscam->angle = angle;
	else if (!camstill && !resetcalled && !paused && timeover != 1)
		thiscam->angle = R_PointToAngle2(thiscam->x, thiscam->y, viewpointx, viewpointy);

	if (timeover == 1)
	{
		thiscam->momx = P_ReturnThrustX(NULL, mo->angle, 32*mo->scale); // Push forward
		thiscam->momy = P_ReturnThrustY(NULL, mo->angle, 32*mo->scale);
		thiscam->momz = 0;
	}
	else if (player->exiting || timeover == 2)
	{
		thiscam->momx = thiscam->momy = thiscam->momz = 0;
	}
	else if (leveltime < introtime)
	{
		thiscam->momx = FixedMul(x - thiscam->x, camspeed);
		thiscam->momy = FixedMul(y - thiscam->y, camspeed);
		thiscam->momz = FixedMul(z - thiscam->z, camspeed);
	}
	else
	{
		thiscam->momx = x - thiscam->x;
		thiscam->momy = y - thiscam->y;

		fixed_t z_speed = Easing_Linear(
			player->karthud[khud_aircam],
			camspeed * 3 / 5,
			camspeed
		);

		thiscam->momz = FixedMul(z - thiscam->z, z_speed);
	}

	thiscam->pan = pan;
	thiscam->pitch = pitch;

	// compute aming to look the viewed point
	f1 = viewpointx-thiscam->x;
	f2 = viewpointy-thiscam->y;
	dist = FixedHypot(f1, f2);

	if (mo->eflags & MFE_VERTICALFLIP)
	{
		angle = R_PointToAngle2(0, thiscam->z + thiscam->height, dist, mo->z + mo->height - player->mo->height + player->cameraOffset);
		if (thiscam->pitch < ANGLE_180 && thiscam->pitch > angle)
			angle += (thiscam->pitch - angle)/2;
	}
	else
	{
		angle = R_PointToAngle2(0, thiscam->z, dist, mo->z + player->mo->height + player->cameraOffset);
		if (thiscam->pitch >= ANGLE_180 && thiscam->pitch < angle)
			angle -= (angle - thiscam->pitch)/2;
	}

	if (player->playerstate != PST_DEAD)
		angle += (focusaiming < ANGLE_180 ? focusaiming/2 : InvAngle(InvAngle(focusaiming)/2)); // overcomplicated version of '((signed)focusaiming)/2;'

	if (!camstill && !timeover) // Keep the view still...
	{
		G_ClipAimingPitch((INT32 *)&angle);

		if (camspeed == FRACUNIT)
			thiscam->aiming = angle;
		else
		{
			angle_t input;
			boolean invert;

			input = thiscam->aiming - angle;
			invert = (input > ANGLE_180);
			if (invert)
				input = InvAngle(input);

			input = FixedAngle(FixedMul(AngleFixed(input), (5*camspeed)/16));
			if (invert)
				input = InvAngle(input);

			thiscam->aiming -= input;
		}
	}

	if (!resetcalled && (player->playerstate == PST_DEAD || player->playerstate == PST_REBORN))
	{
		// Don't let the camera match your movement.
		thiscam->momz = 0;
		if (player->spectator)
			thiscam->aiming = 0;
		// Only let the camera go a little bit downwards.
		else if (!(mo->eflags & MFE_VERTICALFLIP) && thiscam->aiming < ANGLE_337h && thiscam->aiming > ANGLE_180)
			thiscam->aiming = ANGLE_337h;
		else if (mo->eflags & MFE_VERTICALFLIP && thiscam->aiming > ANGLE_22h && thiscam->aiming < ANGLE_180)
			thiscam->aiming = ANGLE_22h;
	}

	if (lookbackdown)
	{
		P_MoveChaseCamera(player, thiscam, false);
		R_ResetViewInterpolation(num + 1);
	}

	thiscam->centerfloorz = mo->floorz;
	thiscam->centerceilingz = mo->ceilingz;

	return (x == thiscam->x && y == thiscam->y && z == thiscam->z && angle == thiscam->aiming);

}

boolean P_SpectatorJoinGame(player_t *player)
{
	INT32 changeto = 0;
	const char *text = NULL;

	// Team changing isn't allowed.
	if (!cv_allowteamchange.value)
		return false;

	// Team changing in Team Match and CTF
	// Pressing fire assigns you to a team that needs players if allowed.
	// Partial code reproduction from p_tick.c autobalance code.
	// a surprise tool that will help us later...
	if (G_GametypeHasTeams() && player->ctfteam == 0)
	{
		INT32 z, numplayersred = 0, numplayersblue = 0;

		//find a team by num players, score, or random if all else fails.
		for (z = 0; z < MAXPLAYERS; ++z)
			if (playeringame[z])
			{
				if (players[z].ctfteam == 1)
					++numplayersred;
				else if (players[z].ctfteam == 2)
					++numplayersblue;
			}
		// for z

		if (numplayersblue > numplayersred)
			changeto = 1;
		else if (numplayersred > numplayersblue)
			changeto = 2;
		else if (bluescore > redscore)
			changeto = 1;
		else if (redscore > bluescore)
			changeto = 2;
		else
			changeto = (P_RandomFixed(PR_RULESCRAMBLE) & 1) + 1;

		if (!LUA_HookTeamSwitch(player, changeto, true, false, false))
			return false;
	}

	// no conditions that could cause the gamejoin to fail below this line

	if (player->mo)
	{
		P_RemoveMobj(player->mo);
		P_SetTarget(&player->mo, NULL);
	}
	player->spectator = false;
	player->pflags &= ~PF_WANTSTOJOIN;
	player->spectatewait = 0;
	player->ctfteam = changeto;
	player->playerstate = PST_REBORN;
	player->enteredGame = true;

	// Reset away view (some code referenced from Got_Teamchange)
	if (G_IsPartyLocal(player - players))
	{
		LUA_HookViewpointSwitch(player, player, true);
		displayplayers[G_PartyPosition(player - players)] = (player-players);
	}

	// a surprise tool that will help us later...
	if (changeto == 1)
		text = va("\x82*%s switched to the %c%s%c team.\n", player_names[player-players], '\x85', "RED", '\x82');
	else if (changeto == 2)
		text = va("\x82*%s switched to the %c%s%c team.\n", player_names[player-players], '\x85', "BLU", '\x82');
	else
		text = va("\x82*%s entered the game.", player_names[player-players]);

	HU_AddChatText(text, false);
	return true; // no more player->mo, cannot continue.
}

// the below is first person only, if you're curious. check out P_CalcChasePostImg in p_mobj.c for chasecam
static void P_CalcPostImg(player_t *player, size_t viewnum)
{
	sector_t *sector = player->mo->subsector->sector;
	postimg_t *type = &postimgtype[viewnum];
	INT32 *param = &postimgparam[viewnum];
	fixed_t pviewheight;
	size_t i;

	if (player->mo->eflags & MFE_VERTICALFLIP)
		pviewheight = player->mo->z + player->mo->height - player->viewheight;
	else
		pviewheight = player->mo->z + player->viewheight;

	if (player->awayview.tics && player->awayview.mobj && !P_MobjWasRemoved(player->awayview.mobj))
	{
		sector = player->awayview.mobj->subsector->sector;
		pviewheight = player->awayview.mobj->z;
	}

	// see if we are in heat (no, not THAT kind of heat...)
	for (i = 0; i < sector->tags.count; i++)
	{
		if (Tag_FindLineSpecial(13, sector->tags.tags[i]) != -1)
		{
			*type = postimg_heat;
			break;
		}
		else if (sector->ffloors)
		{
			ffloor_t *rover;
			fixed_t topheight;
			fixed_t bottomheight;
			boolean gotres = false;

			for (rover = sector->ffloors; rover; rover = rover->next)
			{
				size_t j;

				if (!(rover->fofflags & FOF_EXISTS))
					continue;

				topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
				bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);

				if (pviewheight >= topheight || pviewheight <= bottomheight)
					continue;

				for (j = 0; j < rover->master->frontsector->tags.count; j++)
				{
					if (Tag_FindLineSpecial(13, rover->master->frontsector->tags.tags[j]) != -1)
					{
						*type = postimg_heat;
						gotres = true;
						break;
					}
				}
			}
			if (gotres)
				break;
		}
	}

	// see if we are in water (water trumps heat)
	if (sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t topheight;
		fixed_t bottomheight;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_SWIMMABLE) || rover->fofflags & FOF_BLOCKPLAYER)
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);

			if (pviewheight >= topheight || pviewheight <= bottomheight)
				continue;

			*type = postimg_water;
		}
	}

	if (player->mo->eflags & MFE_VERTICALFLIP)
		*type = postimg_flip;

#if 1
	(void)param;
#else
	// Motion blur
	if (player->speed > (35<<FRACBITS))
	{
		*type = postimg_motion;
		*param = (player->speed - 32)/4;

		if (*param > 5)
			*param = 5;
	}
#endif

	if (encoremode) // srb2kart
		*type = postimg_mirror;
}

void P_DoTimeOver(player_t *player)
{
	if (player->pflags & PF_NOCONTEST)
	{
		// NO! Don't do this!
		return;
	}

	if (P_IsPartyPlayer(player) && !demo.playback)
	{
		legitimateexit = true; // SRB2kart: losing a race is still seeing it through to the end :p
		player->roundconditions.checkthisframe = true;
		gamedata->deferredconditioncheck = true;
	}

	if (netgame && !player->bot && !(gametyperules & GTR_BOSS))
	{
		CON_LogMessage(va(M_GetText("%s ran out of time.\n"), player_names[player-players]));
	}

	player->pflags |= PF_NOCONTEST;
	K_UpdatePowerLevelsFinalize(player, false);

	if (G_GametypeUsesLives())
	{
		K_PlayerLoseLife(player);
	}

	if (player->mo)
	{
		S_StopSound(player->mo);
		P_DamageMobj(player->mo, NULL, NULL, 1, DMG_TIMEOVER);
	}

	if (G_IsPartyLocal(player - players) && musiccountdown == 0)
	{
		Music_Play("finish_silence");
		musiccountdown = MUSIC_COUNTDOWN_MAX;
	}

	if (!exitcountdown)
	{
		G_BeginLevelExit();
	}

	K_InitPlayerTally(player);
}

// SRB2Kart: These are useful functions, but we aren't using them yet.
#if 0

// Get an axis of a certain ID number
static mobj_t *P_GetAxis(INT32 num)
{
	thinker_t *th;
	mobj_t *mobj;

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mobj = (mobj_t *)th;

		// NiGHTS axes spawn before anything else. If this mobj doesn't have MF2_AXIS, it means we reached the axes' end.
		if (!(mobj->flags2 & MF2_AXIS))
			break;

		// Skip if this axis isn't the one we want.
		if (mobj->health != num)
			continue;

		return mobj;
	}

	CONS_Alert(CONS_WARNING, "P_GetAxis: Track segment %d is missing!\n", num);
	return NULL;
}

// Auxiliary function. For a given position and axis, it calculates the nearest "valid" snap-on position.
static void P_GetAxisPosition(fixed_t x, fixed_t y, mobj_t *amo, fixed_t *newx, fixed_t *newy, angle_t *targetangle, angle_t *grind)
{
	fixed_t ax = amo->x;
	fixed_t ay = amo->y;
	angle_t ang;
	angle_t gr = 0;

	if (amo->type == MT_AXISTRANSFERLINE)
	{
		ang = amo->angle;
		// Extra security for cardinal directions.
		if (ang == ANGLE_90 || ang == ANGLE_270) // Vertical lines
			x = ax;
		else if (ang == 0 || ang == ANGLE_180) // Horizontal lines
			y = ay;
		else // Diagonal lines
		{
			fixed_t distance = R_PointToDist2(ax, ay, x, y);
			angle_t fad = ((R_PointToAngle2(ax, ay, x, y) - ang) >> ANGLETOFINESHIFT) & FINEMASK;
			fixed_t cosine = FINECOSINE(fad);
			angle_t fa = (ang >> ANGLETOFINESHIFT) & FINEMASK;
			distance = FixedMul(distance, cosine);
			x = ax + FixedMul(distance, FINECOSINE(fa));
			y = ay + FixedMul(distance, FINESINE(fa));
		}
	}
	else // Keep minecart to circle
	{
		fixed_t rad = amo->radius;
		fixed_t distfactor = FixedDiv(rad, R_PointToDist2(ax, ay, x, y));

		gr = R_PointToAngle2(ax, ay, x, y);
		ang = gr + ANGLE_90;
		x = ax + FixedMul(x - ax, distfactor);
		y = ay + FixedMul(y - ay, distfactor);
	}

	*newx = x;
	*newy = y;
	*targetangle = ang;
	*grind = gr;
}

static void P_ParabolicMove(mobj_t *mo, fixed_t x, fixed_t y, fixed_t z, fixed_t g, fixed_t speed)
{
	fixed_t dx = x - mo->x;
	fixed_t dy = y - mo->y;
	fixed_t dz = z - mo->z;
	fixed_t dh = P_AproxDistance(dx, dy);
	fixed_t c = FixedDiv(dx, dh);
	fixed_t s = FixedDiv(dy, dh);
	fixed_t fixConst = FixedDiv(speed, g);

	mo->momx = FixedMul(c, speed);
	mo->momy = FixedMul(s, speed);
	mo->momz = FixedDiv(dh, 2*fixConst) + FixedDiv(dz, FixedDiv(dh, fixConst/2));
}

#endif

	/* gaysed script from me, based on Golden's sprite slope roll */

// holy SHIT
static INT32
Quaketilt (player_t *player)
{
	angle_t tilt;
	fixed_t lowb; // this threshold for speed
	angle_t moma = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
	INT32 delta = (INT32)( player->mo->angle - moma );
	fixed_t speed;

	boolean sliptiding = K_Sliptiding(player);

	if (delta == (INT32)ANGLE_180)/* FUCK YOU HAVE A HACK */
	{
		return 0;
	}

	// Hi! I'm "not a math guy"!
	if (abs(delta) > ANGLE_90)
		delta = (INT32)(( moma + ANGLE_180 ) - player->mo->angle );
	if (P_IsObjectOnGround(player->mo))
	{
		if (sliptiding)
		{
			tilt = ANGLE_45;
			lowb = 20*FRACUNIT;
		}
		else
		{
			tilt = ANGLE_11hh/2;
			lowb = 60*FRACUNIT;
		}
	}
	else
	{
		tilt = ANGLE_22h;
		lowb = 40*FRACUNIT;
	}
	lowb = FixedMul(lowb, player->mo->scale);
	moma = FixedMul(FixedDiv(delta, ANGLE_90), tilt);
	speed = player->speed;
	if (speed < lowb)
	{
		// ease out tilt as we slow...
		moma = FixedMul(moma, FixedDiv(speed, lowb));
	}
	return moma;
}

static void
DoABarrelRoll (player_t *player)
{
	angle_t slope;
	angle_t delta;

	fixed_t smoothing;

	if (player->exiting || F_CreditsDemoExitFade() >= 0)
	{
		return;
	}

	if (player->loop.radius)
	{
		return;
	}

	if (player->respawn.state != RESPAWNST_NONE)
	{
		player->tilt = 0;
		return;
	}

	slope = InvAngle(R_GetPitchRollAngle(player->mo, player));

	if (AbsAngle(slope) < ANGLE_11hh)
	{
		slope = 0;
	}

	if (AbsAngle(slope) > ANGLE_45)
	{
		slope = slope & ANGLE_180 ? InvAngle(ANGLE_45) : ANGLE_45;
	}

	slope -= Quaketilt(player);

	delta = slope - player->tilt;
	smoothing = FixedDiv(AbsAngle(slope), ANGLE_45);

	delta = FixedDiv(delta, 33 *
			FixedDiv(FRACUNIT, FRACUNIT + smoothing));

	if (delta)
		player->tilt += delta;
	else
		player->tilt  = slope;
}

void P_TickAltView(altview_t *view)
{
	if (view->mobj != NULL && P_MobjWasRemoved(view->mobj) == true)
	{
		P_SetTarget(&view->mobj, NULL); // remove view->mobj asap if invalid
		view->tics = 0; // reset to zero
	}

	if (view->tics > 0)
	{
		view->tics--;

		if (view->tics == 0)
		{
			P_SetTarget(&view->mobj, NULL);
		}
	}
}

//
// P_PlayerThink
//

void P_PlayerThink(player_t *player)
{
	ticcmd_t *cmd;
	const size_t playeri = (size_t)(player - players);

#ifdef PARANOIA
	if (!player->mo)
		I_Error("p_playerthink: players[%s].mo == NULL", sizeu1(playeri));
#endif

	// todo: Figure out what is actually causing these problems in the first place...
	if (player->mo->health <= 0 && player->playerstate == PST_LIVE) //you should be DEAD!
	{
		CONS_Debug(DBG_GAMELOGIC, "P_PlayerThink: Player %s in PST_LIVE with 0 health. (\"Zombie bug\")\n", sizeu1(playeri));
		player->playerstate = PST_DEAD;
	}

	if (G_GametypeUsesLives() == true && player->lives <= 0 && player->playerstate != PST_DEAD)
	{
		player->mo->flags &= ~(MF_SOLID|MF_SHOOTABLE); // does not block
		P_UnsetThingPosition(player->mo);
		player->mo->flags |= MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY;
		P_SetThingPosition(player->mo);
		player->mo->standingslope = NULL;
		player->mo->terrain = NULL;
		player->mo->pmomz = 0;

		player->playerstate = PST_DEAD;

		// hide the player sprite forever
		player->mo->hitlag = INT32_MAX;
		player->mo->renderflags |= RF_DONTDRAW;
		player->mo->reappear = INFTICS; // also hides the follower

		// respawn from where you died
		player->respawn.pointx = player->mo->x;
		player->respawn.pointy = player->mo->y;
		player->respawn.pointz = player->mo->z;

		player->pflags |= PF_LOSTLIFE|PF_ELIMINATED|PF_NOCONTEST;
		player->realtime = UINT32_MAX;
		K_InitPlayerTally(player);
	}

	// Erasing invalid player pointers
	{
#define PlayerPointerErase(field) \
		if (field && P_MobjWasRemoved(field)) \
			P_SetTarget(&field, NULL); \

		PlayerPointerErase(player->followmobj);
		PlayerPointerErase(player->stumbleIndicator);
		PlayerPointerErase(player->wavedashIndicator);
		PlayerPointerErase(player->trickIndicator);
		PlayerPointerErase(player->whip);
		PlayerPointerErase(player->hand);
		PlayerPointerErase(player->ringShooter);
		PlayerPointerErase(player->hoverhyudoro);
		PlayerPointerErase(player->flickyAttacker);
		PlayerPointerErase(player->powerup.flickyController);
		PlayerPointerErase(player->powerup.barrier);

#undef PlayerPointerErase
	}

	player->old_drawangle = player->drawangle;

	P_TickAltView(&player->awayview);

	if (player->flashcount)
		player->flashcount--;

	// Track airtime
	if (P_IsObjectOnGround(player->mo)
		&& !P_PlayerInPain(player)) // This isn't airtime, but it's control loss all the same.
	{
		player->lastairtime = player->airtime;
		player->airtime = 0;
	}
	else
	{
		player->airtime++;
	}

	cmd = &player->cmd;

	// SRB2kart
	// Save the dir the player is holding
	//  to allow items to be thrown forward or backward.
	{
		const INT16 threshold = 6*KART_FULLTURN/10;
		if (cmd->throwdir > threshold)
		{
			player->throwdir = 1;
		}
		else if (cmd->throwdir < -threshold)
		{
			player->throwdir = -1;
		}
		else
		{
			player->throwdir = 0;
		}
	}

	// Accessibility - kickstart your acceleration
	if (!(player->pflags & PF_KICKSTARTACCEL))
	{
		player->kickstartaccel = 0;
	}
	else if (cmd->buttons & BT_ACCELERATE)
	{
		if (!player->exiting && !(player->oldcmd.buttons & BT_ACCELERATE) && ((cmd->buttons & BT_SPINDASHMASK) != BT_SPINDASHMASK) && player->trickpanel != TRICKSTATE_READY)
		{
			player->kickstartaccel = 0;
		}
		else if (player->kickstartaccel < ACCEL_KICKSTART)
		{
			player->kickstartaccel++;
			if ((player->kickstartaccel == ACCEL_KICKSTART) && !K_PlayerUsesBotMovement(player) && P_IsDisplayPlayer(player))
			{
				S_StartSound(NULL, sfx_ding);
			}
		}
		else // for HUD
		{
			player->kickstartaccel = ACCEL_KICKSTART+1;
		}
	}
	else if (player->kickstartaccel < ACCEL_KICKSTART)
	{
		player->kickstartaccel = 0;
	}
	else // for HUD
	{
		player->kickstartaccel = ACCEL_KICKSTART+1;
	}

#ifdef PARANOIA
	if (player->playerstate == PST_REBORN)
		I_Error("player %s is in PST_REBORN\n", sizeu1(playeri));
#endif

	if (!mapreset)
	{
		if (gametyperules & GTR_CIRCUIT)
		{
#if 0
			// If 10 seconds are left on the timer,
			// begin the drown music for countdown!
			// SRB2Kart: despite how perfect this is, it's disabled FOR A REASON
			if (racecountdown == 11*TICRATE - 1)
			{
				if (P_IsPartyPlayer(player))
					S_ChangeMusicInternal("drown", false);
			}
#endif

			// If you've hit the countdown and you haven't made
			//  it to the exit, you're a goner!
			if (racecountdown == 1 && !player->spectator && !player->exiting && !(player->pflags & PF_NOCONTEST) && player->lives > 0)
			{
				P_DoTimeOver(player);

				if (player->playerstate == PST_DEAD)
				{
					LUA_HookPlayer(player, HOOK(PlayerThink));
					return;
				}
			}
		}
	}

	// Make sure spectators always have a score and ring count of 0.
	if (player->spectator)
	{
		//player->score = 0;
		player->rings = 0;
		player->mo->health = 1;
	}

	// SRB2kart 010217
	if (leveltime < introtime)
	{
		player->nocontrol = 2;
	}

	// Synchronizes the "real" amount of time spent in the level.
	if (!(player->exiting || mapreset) && !(player->pflags & PF_NOCONTEST) && !stoppedclock)
	{
		if (leveltime >= starttime)
		{
			player->realtime = leveltime - starttime;

			if (player->spectator)
				player->laptime[LAP_CUR] = 0;
			else if (player->laptime[LAP_CUR] != UINT32_MAX)
				player->laptime[LAP_CUR]++; // This is too complicated to sync to realtime, just sorta hope for the best :V
		}
		else
		{
			player->realtime = 0;
			player->laptime[LAP_CUR] = 0;
		}
	}

	if (cmd->flags & TICCMD_TYPING)
	{
		/*
		typing_duration is slow to start and slow to stop.

		typing_timer counts down a grace period before the player is not
		actually considered typing anymore.
		*/
		if (cmd->flags & TICCMD_KEYSTROKE)
		{
			/* speed up if we are typing quickly! */
			if (player->typing_duration > 0 && player->typing_timer > 12)
			{
				if (player->typing_duration < 16)
				{
					player->typing_duration = 24;
				}
				else
				{
					/* slows down a tiny bit as it approaches the next dot */
					const UINT8 step = (((player->typing_duration + 15) & ~15) -
							player->typing_duration) / 2;
					player->typing_duration += max(step, 4);
				}
			}

			player->typing_timer = 15;
		}
		else if (player->typing_timer > 0)
		{
			player->typing_timer--;
		}

		/* if we are in the grace period (including currently typing) */
		if (player->typing_timer + player->typing_duration > 0)
		{
			/* always end the cycle on two dots */
			if (player->typing_timer == 0 &&
					(player->typing_duration < 16 || player->typing_duration == 40))
			{
				player->typing_duration = 0;
			}
			else if (player->typing_duration < 63)
			{
				player->typing_duration++;
			}
			else
			{
				player->typing_duration = 16;
			}
		}
	}
	else
	{
		player->typing_timer = 0;
		player->typing_duration = 0;
	}

	/* ------------------------------------------ /
	ALL ABOVE THIS BLOCK OCCURS EVEN WITH HITLAG
	/ ------------------------------------------ */

	if (P_MobjIsFrozen(player->mo))
	{
		return;
	}

	/* ------------------------------------------ /
	ALL BELOW THIS BLOCK IS STOPPED DURING HITLAG
	/ ------------------------------------------ */

	player->pflags &= ~PF_HITFINISHLINE;

	// check water content, set stuff in mobj
	P_MobjCheckWater(player->mo);

#ifndef SECTORSPECIALSAFTERTHINK
	if (player->onconveyor != 1 || !P_IsObjectOnGround(player->mo))
		player->onconveyor = 0;
	// check special sectors : damage & secrets

	if (!player->spectator)
		P_PlayerInSpecialSector(player);
#endif

	if (player->playerstate == PST_DEAD)
	{
		if (player->spectator)
			player->mo->renderflags |= RF_GHOSTLY;
		else
			player->mo->renderflags &= ~RF_GHOSTLYMASK;
		P_DeathThink(player);
		LUA_HookPlayer(player, HOOK(PlayerThink));
		return;
	}

	if ((netgame || multiplayer) && player->spectator && !player->bot && cmd->buttons & BT_ATTACK && !player->flashing)
	{
		player->pflags ^= PF_WANTSTOJOIN;
		player->flashing = TICRATE/2 + 1;
		/*if (P_SpectatorJoinGame(player))
			return; // player->mo was removed.*/
		//CONS_Printf("player %s wants to join on tic %d\n", player_names[player-players], leveltime);
	}

	if (player->respawn.state != RESPAWNST_NONE)
	{
		K_RespawnChecker(player);
		player->rmomx = player->rmomy = 0;

		player->markedfordeath = false; // In case we got here via a death sector or something.

		if (player->respawn.state == RESPAWNST_DROP)
		{
			// Allows some turning
			P_MovePlayer(player);
		}
	}
	else if (player->mo->reactiontime)
	{
		// Reactiontime is used to prevent movement
		// for a bit after a teleport.
		player->mo->reactiontime--;
	}
	else if (player->carry == CR_ZOOMTUBE && player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT)
	{
		P_DoZoomTube(player);
		player->rmomx = player->rmomy = 0;
	}
	else if (player->loop.radius != 0)
	{
		P_PlayerOrbit(player);
		player->rmomx = player->mo->momx;
		player->rmomy = player->mo->momy;
	}
	else
	{
		// Move around.
		P_MovePlayer(player);
	}

	// Unset statis flag after moving.
	// In other words, if you manually set stasis via code,
	// it lasts for one tic.
	player->pflags &= ~PF_STASIS;

	if (player->onconveyor == 1)
		player->onconveyor = 3;
	else if (player->onconveyor == 3)
		player->cmomy = player->cmomx = 0;

	P_DoBubbleBreath(player); // Spawn Sonic's bubbles
	P_CheckInvincibilityTimer(player); // Spawn Invincibility Sparkles

	// Counters, time dependent power ups.
	// Time Bonus & Ring Bonus count settings

	// Strength counts up to diminish fade.
	if (player->flashing && player->flashing < UINT16_MAX &&
		(player->spectator || !P_PlayerInPain(player)) &&
		// Battle: flashing tics do not decrease in the air
		(!(gametyperules & GTR_BUMPERS) || P_IsObjectOnGround(player->mo)))
	{
		player->flashing--;
	}

	if (player->nocontrol && player->nocontrol < UINT16_MAX)
	{
		if (!(--player->nocontrol))
		{
			if (player->pflags & PF_FAULT)
			{
				player->pflags &= ~PF_FAULT;
				player->mo->renderflags &= ~RF_DONTDRAW;
				player->mo->flags &= ~MF_NOCLIPTHING;
			}
		}
	}

	boolean deathcontrolled = (player->respawn.state != RESPAWNST_NONE && player->respawn.truedeath == true)
		|| (player->pflags & PF_NOCONTEST) || (player->karmadelay);
	boolean powercontrolled = (player->hyudorotimer) || (player->growshrinktimer > 0);

	// Flash player after being hit.
	if (!deathcontrolled && !powercontrolled)
	{
		if (player->flashing > 1 && player->flashing < K_GetKartFlashing(player)
			&& (leveltime & 1))
			player->mo->renderflags |= RF_DONTDRAW;
		else
			player->mo->renderflags &= ~RF_DONTDRAW;
	}
	else if (!deathcontrolled)
	{
		player->mo->renderflags &= ~RF_DONTDRAW;
	}

	if (player->stairjank > 0)
	{
		player->stairjank--;
	}

	// Random skin / "ironman"
	{
		UINT32 skinflags = (demo.playback)
			? demo.skinlist[demo.currentskinid[playeri]].flags
			: skins[player->skin].flags;

		if (skinflags & SF_IRONMAN) // we are Heavy Magician
		{
			if (player->charflags & SF_IRONMAN) // no fakeskin yet
			{
				if (leveltime >= starttime
					&& !player->exiting
					&& player->mo->health > 0
					&& (player->respawn.state == RESPAWNST_NONE
						|| (player->respawn.state == RESPAWNST_DROP && !player->respawn.timer))
					&& !P_PlayerInPain(player))
				{
					if (player->fakeskin != MAXSKINS)
					{
						SetFakePlayerSkin(player, player->fakeskin);
						if (player->spectator == false)
						{
							S_StartSound(player->mo, sfx_kc33);
							K_SpawnMagicianParticles(player->mo, 5);
						}
					}
					else if (!(gametyperules & GTR_CIRCUIT))
					{
						SetRandomFakePlayerSkin(player, false, false);
					}
				}
			}
			else if (player->exiting) // wearing a fakeskin, but need to display signpost postrace etc
			{
				ClearFakePlayerSkin(player);
			}
		}
	}

	K_KartPlayerThink(player, cmd); // SRB2kart

	DoABarrelRoll(player);

	if (player->carry == CR_SLIDING)
		player->carry = CR_NONE;

	LUA_HookPlayer(player, HOOK(PlayerThink));
}

//
// P_PlayerAfterThink
//
// Thinker for player after all other thinkers have run
//
void P_PlayerAfterThink(player_t *player)
{
	UINT8 i;

#ifdef PARANOIA
	if (!player->mo)
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("P_PlayerAfterThink: players[%s].mo == NULL", sizeu1(playeri));
	}
#endif

	if (player->exiting && !K_PlayerTallyActive(player))
	{
		// We defer P_DoPlayerExit tallies to the end of the tic.
		K_InitPlayerTally(player);
	}

	if (P_IsObjectOnGround(player->mo) == true)
	{
		player->outrun = 0;
	}

#ifdef SECTORSPECIALSAFTERTHINK
	if (player->onconveyor != 1 || !P_IsObjectOnGround(player->mo))
		player->onconveyor = 0;
	// check special sectors : damage & secrets

	if (!player->spectator)
		P_PlayerInSpecialSector(player);
#endif

	if (player->playerstate == PST_DEAD)
	{
		// Followers need handled while dead.
		K_HandleFollower(player);

		if (player->followmobj)
		{
			P_RemoveMobj(player->followmobj);
			P_SetTarget(&player->followmobj, NULL);
		}

		return;
	}

	{
		boolean chase = true;

		for (i = 0; i <= r_splitscreen; i++)
		{
			if (player == &players[displayplayers[i]] && !camera[i].chase)
			{
				chase = false;
			}
		}

		if (!chase) // bob view only if looking through the player's eyes
		{
			P_CalcHeight(player);
		}
		else
		{
			// defaults to make sure 1st person cam doesn't do anything weird on startup
			player->deltaviewheight = 0;
			player->viewheight = P_GetPlayerViewHeight(player);

			if (player->mo->eflags & MFE_VERTICALFLIP)
				player->viewz = player->mo->z + player->mo->height - player->viewheight;
			else
				player->viewz = player->mo->z + player->viewheight;
		}

		for (i = 0; i <= r_splitscreen; i++)
		{
			if (player == &players[displayplayers[i]] && !camera[i].chase)
			{
				P_CalcPostImg(player, i);
			}
		}
	}

	// spectator invisibility and nogravity.
	if ((netgame || multiplayer) && player->spectator)
	{
		player->mo->renderflags |= RF_DONTDRAW;
		player->mo->flags |= MF_NOGRAVITY;
	}

	K_KartPlayerAfterThink(player);

	if (player->followmobj && (player->spectator || player->mo->health <= 0 || player->followmobj->type != player->followitem))
	{
		P_RemoveMobj(player->followmobj);
		P_SetTarget(&player->followmobj, NULL);
	}

	if (!player->spectator && player->mo->health && player->followitem)
	{
		if (!player->followmobj || P_MobjWasRemoved(player->followmobj))
		{
			P_SetTarget(&player->followmobj, P_SpawnMobjFromMobj(player->mo, 0, 0, 0, player->followitem));
			P_SetTarget(&player->followmobj->tracer, player->mo);
			switch (player->followmobj->type)
			{
				default:
					player->followmobj->flags2 |= MF2_LINKDRAW;
					break;
			}
		}

		if (player->followmobj)
		{
			if (LUA_HookFollowMobj(player, player->followmobj) || P_MobjWasRemoved(player->followmobj))
				{;}
			else
			{
				switch (player->followmobj->type)
				{
					default:
						var1 = 1;
						var2 = 0;
						A_CapeChase(player->followmobj);
						break;
				}
			}
		}
	}

	// Run followers in AfterThink, after the players have moved,
	// so a lag value of 1 is exactly attached to the player.
	K_HandleFollower(player);

	if (P_MobjWasRemoved(player->mo) || (player->mo->eflags & MFE_PAUSED) == 0)
	{
		player->timeshitprev = player->timeshit;
		player->timeshit = 0;
	}

	if (K_PlayerUsesBotMovement(player))
	{
		K_UpdateBotGameplayVars(player);
	}

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (player != &players[displayplayers[i]])
		{
			continue;
		}

		// Store before it gets 0'd out
		camera[i].pmomz = player->mo->pmomz;
	}

	if (P_IsObjectOnGround(player->mo))
		player->mo->pmomz = 0;
}

void P_IncrementGriefValue(player_t *player, UINT32 *grief, const UINT32 griefMax)
{
	const fixed_t requireDist = (12*player->mo->scale) / FRACUNIT;
	INT32 progress = player->distancetofinishprev - player->distancetofinish;
	boolean exceptions = (
		player->flashing != 0
		|| P_MobjIsFrozen(player->mo)
		|| player->airtime > 3*TICRATE/2
		|| (player->justbumped > 0 && player->justbumped < bumptime-1)
	);

	if (!exceptions && (progress < requireDist))
	{
		// If antigrief is disabled, we don't want the
		// player getting into a hole so deep no amount
		// of good behaviour could ever make up for it.
		if (*grief < griefMax)
		{
			// Making no progress, start counting against you.
			*grief = *grief + 1;
			if (progress < -requireDist && *grief < griefMax)
			{
				// Making NEGATIVE progress? Start counting even harder.
				*grief = *grief + 1;
			}
		}
	}
	else if (*grief > 0)
	{
		// Playing normally.
		*grief = *grief - 1;

		if (*grief == 0)
			player->griefWarned = false;
	}
}

void P_CheckRaceGriefing(player_t *player, boolean dopunishment)
{
	const UINT32 griefMax = cv_antigrief.value * TICRATE;
	const UINT8 n = player - players;

	// Don't punish if the cvar is turned off,
	// otherwise NOBODY would be able to play!
	if (griefMax == 0)
	{
		dopunishment = false;
	}

	P_IncrementGriefValue(player, &player->griefValue, griefMax);

	if (dopunishment && !player->griefWarned && player->griefValue >= (griefMax/2))
	{
		K_AddMessageForPlayer(player, "Get moving!", true, false);
		if (P_IsPartyPlayer(player))
			S_StartSound(NULL, sfx_cftbl0);
		player->griefWarned = true;
	}

	if (dopunishment && player->griefValue >= griefMax)
	{
		if (player->griefStrikes < 3)
		{
			player->griefStrikes++;
		}

		player->griefValue = 0;
		player->griefWarned = false;

		if (server)
		{
			if (player->griefStrikes == 3 && playernode[n] != servernode
#ifndef DEVELOP
				&& !IsPlayerAdmin(n)
#endif
				)
			{
				// Send kick
				SendKick(n, KICK_MSG_GRIEF);
			}
			else
			{
				// Send spectate
				changeteam_union NetPacket;
				UINT16 usvalue;

				NetPacket.value.l = NetPacket.value.b = 0;
				NetPacket.packet.newteam = 0;
				NetPacket.packet.playernum = n;
				NetPacket.packet.verification = true;

				usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
				SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
			}
		}
	}
}

void P_SetPlayerAngle(player_t *player, angle_t angle)
{
	P_ForceLocalAngle(player, angle);
	player->angleturn = angle;
}

void P_ForceLocalAngle(player_t *player, angle_t angle)
{
	UINT8 i;

	angle = angle & ~UINT16_MAX;

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (player == &players[displayplayers[i]])
		{
			localangle[i] = angle;
		}
	}

	// jartha: OK, I don't really know how ticcmds work. The
	// specific problem I'm trying to fix is that, on level
	// load, the player angle gets reset. But the ticcmds
	// copied in afterward don't match this angle, and they
	// influence the player steering.
	for (i = 0; i <= splitscreen; i++)
	{
		if (player == &players[g_localplayers[i]])
		{
			D_ResetTiccmdAngle(i, angle);
			localsteering[i] = angle;

			break;
		}
	}
}

boolean P_PlayerFullbright(player_t *player)
{
	return (player->invincibilitytimer > 0);
}

void P_ResetPlayerCheats(void)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = &players[i];
		mobj_t *thing = player->mo;

		if (!playeringame[i])
			continue;

		player->pflags &= ~(PF_GODMODE);
		player->respawn.manual = false;

		if (P_MobjWasRemoved(thing))
			continue;

		thing->flags &= ~(MF_NOCLIP);

		thing->destscale = mapobjectscale;
		P_SetScale(thing, thing->destscale);
	}
}
