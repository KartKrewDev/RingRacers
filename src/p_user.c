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

#ifdef HW3SOUND
#include "hardware/hw3sound.h"
#endif

#ifdef HWRENDER
#include "hardware/hw_light.h"
#include "hardware/hw_main.h"
#endif

#if 0
static void P_NukeAllPlayers(player_t *player);
#endif

//
// Jingle stuff.
//

jingle_t jingleinfo[NUMJINGLES] = {
	// {musname, looping, reset, nest}
	{""        , false}, // JT_NONE
	{""        , false}, // JT_OTHER
	{""        , false}, // JT_MASTER
	{"_1up"    , false},
	{"_shoes"  ,  true},
	{"_inv"    , false},
	{"_minv"   , false},
	{"_drown"  , false},
	{"_super"  ,  true},
	{"_gover"  , false},
	{"_ntime"  , false},  // JT_NIGHTSTIMEOUT
	{"_drown"  , false}   // JT_SSTIMEOUT
	// {"_clear"  , false},
	// {"_inter"  ,  true},
	// {"_conti"  ,  true}
};

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
	if (netgame || modeattacking || gamestate == GS_TITLESCREEN)
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
	fixed_t bobmul = FRACUNIT - FixedDiv(FixedHypot(player->rmomx, player->rmomy), K_GetKartSpeed(player, false));

	// Regular movement bobbing.
	// Should not be calculated when not on ground (FIXTHIS?)
	// OPTIMIZE: tablify angle
	// Note: a LUT allows for effects
	//  like a ramp with low health.

	if (bobmul > FRACUNIT) bobmul = FRACUNIT;
	if (bobmul < FRACUNIT/8) bobmul = FRACUNIT/8;

	player->bob = FixedMul(cv_movebob.value, bobmul);

	if (!P_IsObjectOnGround(mo))
	{
		if (mo->eflags & MFE_VERTICALFLIP)
		{
			player->viewz = mo->z + mo->height - player->viewheight;
			if (player->viewz < mo->floorz + FixedMul(FRACUNIT, mo->scale))
				player->viewz = mo->floorz + FixedMul(FRACUNIT, mo->scale);
		}
		else
		{
			player->viewz = mo->z + player->viewheight;
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
			|| abs(p->mo->momz) >= FixedMul(FRACUNIT/2, p->mo->scale)
			|| p->climbing || p->powers[pw_tailsfly]
			|| (p->pflags & PF_JUMPED) || (p->pflags & PF_SPINNING));
}

// P_GetNextEmerald
//
// Gets the number (0 based) of the next emerald to obtain
//
UINT8 P_GetNextEmerald(void)
{
	if (gamemap >= sstage_start && gamemap <= sstage_end)
		return (UINT8)(gamemap - sstage_start);
	if (gamemap >= smpstage_start || gamemap <= smpstage_end)
		return (UINT8)(gamemap - smpstage_start);
	return 0;
}

//
// P_GiveEmerald
//
// Award an emerald upon completion
// of a special stage.
//
void P_GiveEmerald(boolean spawnObj)
{
	UINT8 em = P_GetNextEmerald();

	S_StartSound(NULL, sfx_cgot); // Got the emerald!
	emeralds |= (1 << em);
	stagefailed = false;

	if (spawnObj)
	{
		// The Chaos Emerald begins to orbit us!
		// Only visibly give it to ONE person!
		UINT8 i, pnum = ((playeringame[consoleplayer]) && (!players[consoleplayer].spectator) && (players[consoleplayer].mo)) ? consoleplayer : 255;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			mobj_t *emmo;
			if (!playeringame[i])
				continue;
			if (players[i].spectator)
				continue;
			if (!players[i].mo)
				continue;

			emmo = P_SpawnMobjFromMobj(players[i].mo, 0, 0, players[i].mo->height, MT_GOTEMERALD);
			if (!emmo)
				continue;
			P_SetTarget(&emmo->target, players[i].mo);
			P_SetMobjState(emmo, mobjinfo[MT_GOTEMERALD].meleestate + em);

			// Make sure we're not being carried before our tracer is changed
			players[i].powers[pw_carry] = CR_NONE;

			P_SetTarget(&players[i].mo->tracer, emmo);

			if (pnum == 255)
			{
				pnum = i;
				continue;
			}

			if (i == pnum)
				continue;

			emmo->flags2 |= MFD_DONTDRAW;
		}
	}
}

//
// P_GiveFinishFlags
//
// Give the player visual indicators
// that they've finished the map.
//
void P_GiveFinishFlags(player_t *player)
{
	angle_t angle = FixedAngle(player->mo->angle << FRACBITS);
	UINT8 i;

	if (!player->mo)
		return;

	if (!(netgame||multiplayer))
		return;

	for (i = 0; i < 3; i++)
	{
		angle_t fa = (angle >> ANGLETOFINESHIFT) & FINEMASK;
		fixed_t xoffs = FINECOSINE(fa);
		fixed_t yoffs = FINESINE(fa);
		mobj_t* flag = P_SpawnMobjFromMobj(player->mo, xoffs, yoffs, 0, MT_FINISHFLAG);
		flag->angle = angle;
		angle += FixedAngle(120*FRACUNIT);

		P_SetTarget(&flag->target, player->mo);
	}
}

#if 0
//
// P_ResetScore
//
// This is called when your chain is reset.
void P_ResetScore(player_t *player)
{
	// Formally a host for Chaos mode behavior

	player->scoreadd = 0;
}
#endif

//
// P_FindLowestLap
//
// SRB2Kart, a similar function as above for finding the lowest lap
//
UINT8 P_FindLowestLap(void)
{
	INT32 i;
	UINT8 lowest = UINT8_MAX;

	if (gametyperules & GTR_CIRCUIT)
		return 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		if (lowest == 255)
			lowest = players[i].laps;
		else if (players[i].laps < lowest)
			lowest = players[i].laps;
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
// Checks for painstate and pw_flashing, if both found return true
//
boolean P_PlayerInPain(player_t *player)
{
	if (player->kartstuff[k_spinouttimer] || player->kartstuff[k_squishedtimer] || player->respawn.state != RESPAWNST_NONE)
		return true;

	if (gametyperules & GTR_KARMA)
	{
		if (player->kartstuff[k_bumper] <= 0 && player->kartstuff[k_comebacktimer])
			return true;
	}

	return false;
}

//
// P_ResetPlayer
//
// Useful when you want to kill everything the player is doing.
void P_ResetPlayer(player_t *player)
{
	//player->pflags &= ~(PF_);
	player->powers[pw_carry] = CR_NONE;
	player->onconveyor = 0;
}

//
// P_GivePlayerRings
//
// Gives rings to the player, and does any special things required.
// Call this function when you want to increment the player's health.
//

void P_GivePlayerRings(player_t *player, INT32 num_rings)
{
	if (!player->mo)
		return;

	if ((gametyperules & GTR_BUMPERS)) // No rings in Battle Mode
		return;

	player->rings += num_rings;
	//player->totalring += num_rings; // Used for GP lives later

	if (player->rings > 20)
		player->rings = 20; // Caps at 20 rings, sorry!
	else if (player->rings < -20)
		player->rings = -20; // Chaotix ring debt!
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

	if (player->lives > 9)
		player->lives = 9;
	else if (player->lives < 1)
		player->lives = 1;
}

// Adds to the player's score
void P_AddPlayerScore(player_t *player, UINT32 amount)
{
	//UINT32 oldscore;

	if (!((gametyperules & GTR_BUMPERS)))
		return;

	if (player->exiting) // srb2kart
		return;

	//oldscore = player->score;

	// Don't go above MAXSCORE.
	if (player->marescore + amount < MAXSCORE)
		player->marescore += amount;
	else
		player->marescore = MAXSCORE;
}

//
// P_PlayLivesJingle
//
void P_PlayLivesJingle(player_t *player)
{
	if (player && !P_IsLocalPlayer(player))
		return;

	if (use1upSound)
		S_StartSound(NULL, sfx_oneup);
	else
	{
		P_PlayJingle(player, JT_1UP);
		if (player)
			player->powers[pw_extralife] = extralifetics + 1;
		strlcpy(S_sfx[sfx_None].caption, "One-up", 7);
		S_StartCaption(sfx_None, -1, extralifetics+1);
	}
}

void P_PlayJingle(player_t *player, jingletype_t jingletype)
{
	const char *musname = jingleinfo[jingletype].musname;
	UINT16 musflags = 0;
	boolean looping = jingleinfo[jingletype].looping;

	char newmusic[7];
	strncpy(newmusic, musname, 7);
#ifdef HAVE_LUA_MUSICPLUS
 	if(LUAh_MusicJingle(jingletype, newmusic, &musflags, &looping))
 		return;
#endif
	newmusic[6] = 0;

	P_PlayJingleMusic(player, newmusic, musflags, looping, jingletype);
}

//
// P_PlayJingleMusic
//
void P_PlayJingleMusic(player_t *player, const char *musname, UINT16 musflags, boolean looping, UINT16 status)
{
	// If gamestate != GS_LEVEL, always play the jingle (1-up intermission)
	if (gamestate == GS_LEVEL && player && !P_IsLocalPlayer(player))
		return;

	S_RetainMusic(musname, musflags, looping, 0, status);
	S_StopMusic();
	S_ChangeMusicInternal(musname, looping);
}

boolean P_EvaluateMusicStatus(UINT16 status, const char *musname)
{
	// \todo lua hook
	int i;
	boolean result = false;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!P_IsLocalPlayer(&players[i]))
			continue;

		switch(status)
		{
			case JT_1UP: // Extra life
				result = (players[i].powers[pw_extralife] > 1);
				break;

			case JT_SHOES:  // Speed shoes
				if (players[i].powers[pw_sneakers] > 1 && !players[i].powers[pw_super])
				{
					//strlcpy(S_sfx[sfx_None].caption, "Speed shoes", 12);
					//S_StartCaption(sfx_None, -1, players[i].powers[pw_sneakers]);
					result = true;
				}
				else
					result = false;
				break;

			case JT_INV: // Invincibility
			case JT_MINV: // Mario Invincibility
				if (players[i].powers[pw_invulnerability] > 1)
				{
					//strlcpy(S_sfx[sfx_None].caption, "Invincibility", 14);
					//S_StartCaption(sfx_None, -1, players[i].powers[pw_invulnerability]);
					result = true;
				}
				else
					result = false;
				break;

			case JT_DROWN:  // Drowning
				result = (players[i].powers[pw_underwater] && players[i].powers[pw_underwater] <= 11*TICRATE + 1);
				break;

			case JT_SUPER:  // Super Sonic
				result = (players[i].powers[pw_super]);
				break;

			case JT_GOVER: // Game Over
				result = (players[i].lives <= 0);
				break;

			case JT_NIGHTSTIMEOUT: // NiGHTS Time Out (10 seconds)
			case JT_SSTIMEOUT:
				result = (players[i].nightstime && players[i].nightstime <= 10*TICRATE);
				break;

			case JT_OTHER:  // Other state
				result = LUAh_ShouldJingleContinue(&players[i], musname);
				break;

			case JT_NONE:   // Null state
			case JT_MASTER: // Main level music
			default:
				result = true;
		}

		if (result)
			break;
 	}

	return result;
}

void P_PlayRinglossSound(mobj_t *source)
{
	if (source->player && K_GetShieldFromItem(source->player->kartstuff[k_itemtype]) != KSHIELD_NONE)
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

void P_PlayVictorySound(mobj_t *source)
{
	if (cv_kartvoices.value)
		S_StartSound(source, sfx_kwin);
}

//
// P_EndingMusic
//
// Consistently sets ending music!
//
boolean P_EndingMusic(player_t *player)
{
	char buffer[9];
	boolean looping = true;
	INT32 bestlocalpos;
	player_t *bestlocalplayer;

	if (!P_IsLocalPlayer(player)) // Only applies to a local player
		return false;

	if (multiplayer && demo.playback) // Don't play this in multiplayer replays
		return false;

	// Event - Level Finish
	// Check for if this is valid or not
	if (r_splitscreen)
	{
		if (!((players[displayplayers[0]].exiting || (players[displayplayers[0]].pflags & PF_GAMETYPEOVER))
			|| (players[displayplayers[1]].exiting || (players[displayplayers[1]].pflags & PF_GAMETYPEOVER))
			|| ((r_splitscreen < 2) && (players[displayplayers[2]].exiting || (players[displayplayers[2]].pflags & PF_GAMETYPEOVER)))
			|| ((r_splitscreen < 3) && (players[displayplayers[3]].exiting || (players[displayplayers[3]].pflags & PF_GAMETYPEOVER)))))
			return false;

		bestlocalplayer = &players[displayplayers[0]];
		bestlocalpos = ((players[displayplayers[0]].pflags & PF_GAMETYPEOVER) ? MAXPLAYERS+1 : players[displayplayers[0]].kartstuff[k_position]);
#define setbests(p) \
	if (((players[p].pflags & PF_GAMETYPEOVER) ? MAXPLAYERS+1 : players[p].kartstuff[k_position]) < bestlocalpos) \
	{ \
		bestlocalplayer = &players[p]; \
		bestlocalpos = ((players[p].pflags & PF_GAMETYPEOVER) ? MAXPLAYERS+1 : players[p].kartstuff[k_position]); \
	}
		setbests(displayplayers[1]);
		if (r_splitscreen > 1)
			setbests(displayplayers[2]);
		if (r_splitscreen > 2)
			setbests(displayplayers[3]);
#undef setbests
	}
	else
	{
		if (!(player->exiting || (player->pflags & PF_GAMETYPEOVER)))
			return false;

		bestlocalplayer = player;
		bestlocalpos = ((player->pflags & PF_GAMETYPEOVER) ? MAXPLAYERS+1 : player->kartstuff[k_position]);
	}

	if ((gametyperules & GTR_CIRCUIT) && bestlocalpos == MAXPLAYERS+1)
		sprintf(buffer, "k*fail"); // F-Zero death results theme
	else
	{
		if (K_IsPlayerLosing(bestlocalplayer))
			sprintf(buffer, "k*lose");
		else if (bestlocalpos == 1)
			sprintf(buffer, "k*win");
		else
			sprintf(buffer, "k*ok");
	}

	S_SpeedMusic(1.0f);

	if ((gametyperules & GTR_CIRCUIT))
		buffer[1] = 'r';
	else if ((gametyperules & GTR_BUMPERS))
	{
		buffer[1] = 'b';
		looping = false;
	}

	S_ChangeMusicInternal(buffer, looping);

	return true;
}

//
// P_RestoreMusic
//
// Restores music after some special music change
//
void P_RestoreMusic(player_t *player)
{
	UINT32 position;

	if (!P_IsLocalPlayer(player)) // Only applies to a local player
		return;

	S_SpeedMusic(1.0f);

	// TO-DO: Use jingle system for Kart's stuff

	// Event - HERE COMES A NEW CHALLENGER
	if (mapreset)
	{
		S_ChangeMusicInternal("chalng", false); //S_StopMusic();
		return;
	}

	// Event - Level Ending
	if (P_EndingMusic(player))
		return;

	// Event - Level Start
	if (leveltime < (starttime + (TICRATE/2)))
		S_ChangeMusicInternal((encoremode ? "estart" : "kstart"), false); //S_StopMusic();
	else // see also where time overs are handled - search for "lives = 2" in this file
	{
		INT32 wantedmus = 0; // 0 is level music, 1 is invincibility, 2 is grow

		if (r_splitscreen)
		{
			INT32 bestlocaltimer = 1;

#define setbests(p) \
	if (players[p].playerstate == PST_LIVE) \
	{ \
		if (players[p].kartstuff[k_invincibilitytimer] > bestlocaltimer) \
		{ wantedmus = 1; bestlocaltimer = players[p].kartstuff[k_invincibilitytimer]; } \
		else if (players[p].kartstuff[k_growshrinktimer] > bestlocaltimer) \
		{ wantedmus = 2; bestlocaltimer = players[p].kartstuff[k_growshrinktimer]; } \
	}
			setbests(displayplayers[0]);
			setbests(displayplayers[1]);
			if (r_splitscreen > 1)
				setbests(displayplayers[2]);
			if (r_splitscreen > 2)
				setbests(displayplayers[3]);
#undef setbests
		}
		else
		{
			if (player->playerstate == PST_LIVE)
			{
				if (player->kartstuff[k_invincibilitytimer] > 1)
					wantedmus = 1;
				else if (player->kartstuff[k_growshrinktimer] > 1)
					wantedmus = 2;
			}
		}

		// Item - Grow
		if (wantedmus == 2)
		{
			S_ChangeMusicInternal("kgrow", true);
			S_SetRestoreMusicFadeInCvar(&cv_growmusicfade);
		}
		// Item - Invincibility
		else if (wantedmus == 1)
		{
			S_ChangeMusicInternal("kinvnc", true);
			S_SetRestoreMusicFadeInCvar(&cv_invincmusicfade);
		}
		else
		{
#if 0
			// Event - Final Lap
			// Still works for GME, but disabled for consistency
			if ((gametyperules & GTR_CIRCUIT) && player->laps >= (UINT8)(cv_numlaps.value))
				S_SpeedMusic(1.2f);
#endif
			if (mapmusresume && cv_resume.value)
				position = mapmusresume;
			else
				position = mapmusposition;

			S_ChangeMusicEx(mapmusname, mapmusflags, true, position, 0,
					S_GetRestoreMusicFadeIn());
			S_ClearRestoreMusicFadeInCvar();
			mapmusresume = 0;
		}
	}
}

//
// P_IsObjectInGoop
//
// Returns true if the object is inside goop water.
// (Spectators and objects otherwise without gravity cannot have goop gravity!)
//
boolean P_IsObjectInGoop(mobj_t *mo)
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
boolean P_IsObjectOnGround(mobj_t *mo)
{
	if (P_IsObjectInGoop(mo) && !(mo->player && mo->player->pflags & PF_BOUNCING))
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
boolean P_IsObjectOnGroundIn(mobj_t *mo, sector_t *sec)
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
				if (!(rover->flags & FF_EXISTS))
					continue;

				// If the FOF is configured to let the object through, continue.
				if (!((rover->flags & FF_BLOCKPLAYER && mo->player)
					|| (rover->flags & FF_BLOCKOTHERS && !mo->player)))
					continue;

				// If the the platform is intangible from below, continue.
				if (rover->flags & FF_PLATFORM)
					continue;

				// If the FOF is a water block, continue. (Unnecessary check?)
				if (rover->flags & FF_SWIMMABLE)
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
				if (!(rover->flags & FF_EXISTS))
					continue;

				// If the FOF is configured to let the object through, continue.
				if (!((rover->flags & FF_BLOCKPLAYER && mo->player)
					|| (rover->flags & FF_BLOCKOTHERS && !mo->player)))
					continue;

				// If the the platform is intangible from above, continue.
				if (rover->flags & FF_REVERSEPLATFORM)
					continue;

				// If the FOF is a water block, continue. (Unnecessary check?)
				if (rover->flags & FF_SWIMMABLE)
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

boolean P_IsObjectOnRealGround(mobj_t *mo, sector_t *sec)
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
// P_IsLocalPlayer
//
// Returns true if player is
// on the local machine.
//
boolean P_IsLocalPlayer(player_t *player)
{
	UINT8 i;

	for (i = 0; i <= r_splitscreen; i++) // DON'T skip P1
	{
		if (player == &players[g_localplayers[i]])
			return true;
	}

	return false;
}

//
// P_IsDisplayPlayer
//
// Returns true if player is
// currently being watched.
//
boolean P_IsDisplayPlayer(player_t *player)
{
	UINT8 i;

	for (i = 0; i <= r_splitscreen; i++) // DON'T skip P1
	{
		if (player == &players[displayplayers[i]])
			return true;
	}

	return false;
}

//
// P_SpawnShieldOrb
//
// Spawns the shield orb on the player
// depending on which shield they are
// supposed to have.
//
void P_SpawnShieldOrb(player_t *player)
{
	mobjtype_t orbtype;
	thinker_t *th;
	mobj_t *shieldobj, *ov;

#ifdef PARANOIA
	if (!player->mo)
		I_Error("P_SpawnShieldOrb: player->mo is NULL!\n");
#endif

	// SRB2Kart
	// TODO: Make our shields use this system

	if (LUAh_ShieldSpawn(player))
		return;

	if (player->powers[pw_shield] & SH_FORCE)
		orbtype = MT_FORCE_ORB;
	else switch (player->powers[pw_shield] & SH_NOSTACK)
	{
	case SH_WHIRLWIND:
		orbtype = MT_WHIRLWIND_ORB;
		break;
	case SH_ATTRACT:
		orbtype = MT_ATTRACT_ORB;
		break;
	case SH_ELEMENTAL:
		orbtype = MT_ELEMENTAL_ORB;
		break;
	case SH_ARMAGEDDON:
		orbtype = MT_ARMAGEDDON_ORB;
		break;
	case SH_PITY:
	case SH_PINK: // PITY IN PINK
		orbtype = MT_PITY_ORB;
		break;
	case SH_FLAMEAURA:
		orbtype = MT_FLAMEAURA_ORB;
		break;
	case SH_BUBBLEWRAP:
		orbtype = MT_BUBBLEWRAP_ORB;
		break;
	case SH_THUNDERCOIN:
		orbtype = MT_THUNDERCOIN_ORB;
		break;
	default:
		return;
	}

	// blaze through the thinkers to see if an orb already exists!
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		shieldobj = (mobj_t *)th;

		if (shieldobj->type == orbtype && shieldobj->target == player->mo)
			P_RemoveMobj(shieldobj); //kill the old one(s)
	}

	shieldobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, orbtype);
	shieldobj->flags2 |= MF2_SHIELD;
	P_SetTarget(&shieldobj->target, player->mo);
	if ((player->powers[pw_shield] & SH_NOSTACK) == SH_PINK)
	{
		shieldobj->color = SKINCOLOR_PINK;
		shieldobj->colorized = true;
	}
	else
		shieldobj->color = (UINT16)shieldobj->info->painchance;
	shieldobj->threshold = (player->powers[pw_shield] & SH_FORCE) ? SH_FORCE : (player->powers[pw_shield] & SH_NOSTACK);

	if (shieldobj->info->seestate)
	{
		ov = P_SpawnMobj(shieldobj->x, shieldobj->y, shieldobj->z, MT_OVERLAY);
		P_SetTarget(&ov->target, shieldobj);
		P_SetMobjState(ov, shieldobj->info->seestate);
		P_SetTarget(&shieldobj->tracer, ov);
	}
	if (shieldobj->info->meleestate)
	{
		ov = P_SpawnMobj(shieldobj->x, shieldobj->y, shieldobj->z, MT_OVERLAY);
		P_SetTarget(&ov->target, shieldobj);
		P_SetMobjState(ov, shieldobj->info->meleestate);
	}
	if (shieldobj->info->missilestate)
	{
		ov = P_SpawnMobj(shieldobj->x, shieldobj->y, shieldobj->z, MT_OVERLAY);
		P_SetTarget(&ov->target, shieldobj);
		P_SetMobjState(ov, shieldobj->info->missilestate);
	}
	if (player->powers[pw_shield] & SH_FORCE)
	{
		//Copy and pasted from P_ShieldLook in p_mobj.c
		shieldobj->movecount = (player->powers[pw_shield] & SH_FORCEHP);
		if (shieldobj->movecount < 1)
		{
			if (shieldobj->info->painstate)
				P_SetMobjState(shieldobj,shieldobj->info->painstate);
			else
				shieldobj->drawflags |= MFD_SHADOW;
		}
	}
}

//
// P_SwitchShield
//
// Handles the possibility of switching between
// the non-stack layer of shields thoroughly,
// then adds the desired one.
//
void P_SwitchShield(player_t *player, UINT16 shieldtype)
{
	boolean donthavealready;

	donthavealready = (shieldtype & SH_FORCE)
		? (!(player->powers[pw_shield] & SH_FORCE) || (player->powers[pw_shield] & SH_FORCEHP) < (shieldtype & ~SH_FORCE))
		: ((player->powers[pw_shield] & SH_NOSTACK) != shieldtype);

	if (donthavealready)
	{
		boolean stopshieldability = (shieldtype & SH_FORCE)
		? (!(player->powers[pw_shield] & SH_FORCE))
		: true;

		// Just in case.
		if (stopshieldability && player->pflags & PF_SHIELDABILITY)
		{
			player->pflags &= ~(PF_SPINNING|PF_SHIELDABILITY); // They'll still have PF_THOKKED...
			player->homing = 0;
		}

		player->powers[pw_shield] = shieldtype|(player->powers[pw_shield] & SH_STACK);
		P_SpawnShieldOrb(player);

		if (shieldtype & SH_PROTECTWATER)
		{
			if (player->powers[pw_underwater] && player->powers[pw_underwater] <= 12*TICRATE + 1)
			{
				player->powers[pw_underwater] = 0;
				P_RestoreMusic(player);
			}
			else
				player->powers[pw_underwater] = 0;

			if (player->powers[pw_spacetime] > 1)
			{
				player->powers[pw_spacetime] = 0;
				P_RestoreMusic(player);
			}
		}
	}
}

//
// P_SpawnGhostMobj
//
// Spawns a ghost object on the player
//
mobj_t *P_SpawnGhostMobj(mobj_t *mobj)
{
	mobj_t *ghost = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_GHOST);

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
	ghost->sprite = mobj->sprite;
	ghost->sprite2 = mobj->sprite2;
	ghost->frame = mobj->frame;
	ghost->tics = -1;
	ghost->drawflags |= tr_trans50 << MFD_TRANSSHIFT;
	ghost->fuse = ghost->info->damage;
	ghost->skin = mobj->skin;
	ghost->standingslope = mobj->standingslope;
#ifdef HWRENDER
	ghost->modeltilt = mobj->modeltilt;
#endif

	ghost->sprxoff = mobj->sprxoff;
	ghost->spryoff = mobj->spryoff;
	ghost->sprzoff = mobj->sprzoff;

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

	return ghost;
}

//
// P_DoPlayerExit
//
// Player exits the map via sector trigger
void P_DoPlayerExit(player_t *player)
{
	const boolean losing = K_IsPlayerLosing(player);

	if (player->exiting || mapreset)
		return;

	if (P_IsLocalPlayer(player) && (!player->spectator && !demo.playback))
		legitimateexit = true;

	if (G_GametypeUsesLives() && losing)
	{
		// Remove a life from the losing player
		K_PlayerLoseLife(player);
	}

	if ((gametyperules & GTR_CIRCUIT)) // If in Race Mode, allow
	{
		player->exiting = raceexittime+2;
		K_KartUpdatePosition(player);

		if (cv_kartvoices.value)
		{
			if (P_IsDisplayPlayer(player))
			{
				sfxenum_t sfx_id;
				if (losing)
					sfx_id = ((skin_t *)player->mo->skin)->soundsid[S_sfx[sfx_klose].skinsound];
				else
					sfx_id = ((skin_t *)player->mo->skin)->soundsid[S_sfx[sfx_kwin].skinsound];
				S_StartSound(NULL, sfx_id);
			}
			else
			{
				if (losing)
					S_StartSound(player->mo, sfx_klose);
				else
					S_StartSound(player->mo, sfx_kwin);
			}
		}

		if (cv_inttime.value > 0)
			P_EndingMusic(player);

		if (P_CheckRacers())
			player->exiting = raceexittime+1;
	}
	else if ((gametyperules & GTR_BUMPERS)) // Battle Mode exiting
	{
		player->exiting = battleexittime+1;
		P_EndingMusic(player);
	}
	else
		player->exiting = raceexittime+2; // Accidental death safeguard???

	if (grandprixinfo.gp == true)
	{
		if (player->bot)
		{
			// Bots are going to get harder... :)
			K_IncreaseBotDifficulty(player);
		}
		else if (!losing)
		{
			const UINT8 lifethreshold = 20;
			UINT8 extra = 0;

			// YOU WIN
			grandprixinfo.wonround = true;

			// Increase your total rings
			if (RINGTOTAL(player) > 0)
			{
				player->totalring += RINGTOTAL(player);

				extra = player->totalring / lifethreshold;

				if (extra > player->xtralife)
				{
					P_GivePlayerLives(player, extra - player->xtralife);
					S_StartSound(NULL, sfx_cdfm73);
					player->xtralife = extra;
				}
			}
		}
	}

	player->powers[pw_underwater] = 0;
	player->powers[pw_spacetime] = 0;
	player->karthud[khud_cardanimation] = 0; // srb2kart: reset battle animation

	if (player == &players[consoleplayer])
		demo.savebutton = leveltime;
}

//
// P_PlayerHitFloor
//
// Handles player hitting floor surface.
// Returns whether to clip momz.
boolean P_PlayerHitFloor(player_t *player, boolean dorollstuff)
{
	boolean clipmomz;

	(void)dorollstuff;

	I_Assert(player->mo != NULL);

	clipmomz = !(P_CheckDeathPitCollide(player->mo));

	// SRB2Kart: removed lots of really vanilla-specific code here

	return clipmomz;
}

boolean P_InQuicksand(mobj_t *mo) // Returns true if you are in quicksand
{
	sector_t *sector = mo->subsector->sector;
	fixed_t topheight, bottomheight;

	fixed_t flipoffset = ((mo->eflags & MFE_VERTICALFLIP) ? (mo->height/2) : 0);

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (!(rover->flags & FF_QUICKSAND))
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

static void P_CheckBustableBlocks(player_t *player)
{
	msecnode_t *node;
	fixed_t oldx;
	fixed_t oldy;

	if ((netgame || multiplayer) && player->spectator)
		return;

	oldx = player->mo->x;
	oldy = player->mo->y;

	// SRB2Kart TODO: make shatter blocks the default behavior, we don't need the hundreds of other types

	P_UnsetThingPosition(player->mo);
	player->mo->x += player->mo->momx;
	player->mo->y += player->mo->momy;
	P_SetThingPosition(player->mo);

	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (!node->m_sector)
			break;

		if (node->m_sector->ffloors)
		{
			ffloor_t *rover;
			fixed_t topheight, bottomheight;

			for (rover = node->m_sector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS)) continue;

				if ((rover->flags & FF_BUSTUP)/* && rover->master->frontsector->crumblestate == CRUMBLE_NONE*/)
				{
					// If it's an FF_SHATTER, you can break it just by touching it.
					if (rover->flags & FF_SHATTER)
						goto bust;

					// If it's an FF_SPINBUST, you can break it if you are in your spinning frames
					// (either from jumping or spindashing).
					if (rover->flags & FF_SPINBUST
						&& (((player->pflags & PF_SPINNING) && !(player->pflags & PF_STARTDASH))
							|| (player->pflags & PF_JUMPED && !(player->pflags & PF_NOJUMPDAMAGE))))
						goto bust;

					if (rover->flags & FF_STRONGBUST)
						continue;

					// If it's not an FF_STRONGBUST, you can break if you are spinning (and not jumping)
					// or you are super
					// or you are recording for Metal Sonic
					if (!((player->pflags & PF_SPINNING) && !(player->pflags & PF_JUMPED))
						&& !(player->powers[pw_super])
						&& !metalrecording)
						continue;

				bust:
					topheight = P_GetFOFTopZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
					bottomheight = P_GetFOFBottomZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);

					// Height checks
					if (rover->flags & FF_SHATTERBOTTOM)
					{
						if (player->mo->z+player->mo->momz + player->mo->height < bottomheight)
							continue;

						if (player->mo->z+player->mo->height > bottomheight)
							continue;
					}
					else if (rover->flags & FF_SPINBUST)
					{
						if (player->mo->z+player->mo->momz > topheight)
							continue;

						if (player->mo->z + player->mo->height < bottomheight)
							continue;
					}
					else if (rover->flags & FF_SHATTER)
					{
						if (player->mo->z + player->mo->momz > topheight)
							continue;

						if (player->mo->z+player->mo->momz + player->mo->height < bottomheight)
							continue;
					}
					else
					{
						if (player->mo->z >= topheight)
							continue;

						if (player->mo->z + player->mo->height < bottomheight)
							continue;
					}

					// Impede the player's fall a bit
					if (((rover->flags & FF_SPINBUST) || (rover->flags & FF_SHATTER)) && player->mo->z >= topheight)
						player->mo->momz >>= 1;
					else if (rover->flags & FF_SHATTER)
					{
						player->mo->momx >>= 1;
						player->mo->momy >>= 1;
					}

					//if (metalrecording)
					//	G_RecordBustup(rover);

					EV_CrumbleChain(NULL, rover); // node->m_sector

					// Run a linedef executor??
					if (rover->master->flags & ML_EFFECT5)
						P_LinedefExecute((INT16)(P_AproxDistance(rover->master->dx, rover->master->dy)>>FRACBITS), player->mo, node->m_sector);

					goto bustupdone;
				}
			}
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
		if (!node->m_sector)
			break;

		if (node->m_sector->ffloors)
		{
			ffloor_t *rover;
			boolean top = true;
			fixed_t topheight, bottomheight;

			for (rover = node->m_sector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS))
					continue; // FOFs should not be bouncy if they don't even "exist"

				if (GETSECSPECIAL(rover->master->frontsector->special, 1) != 15)
					continue; // this sector type is required for FOFs to be bouncy

				topheight = P_GetFOFTopZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
				bottomheight = P_GetFOFBottomZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);

				if (player->mo->z > topheight)
					continue;

				if (player->mo->z + player->mo->height < bottomheight)
					continue;

				if (oldz < P_GetFOFTopZ(player->mo, node->m_sector, rover, oldx, oldy, NULL)
						&& oldz + player->mo->height > P_GetFOFBottomZ(player->mo, node->m_sector, rover, oldx, oldy, NULL))
					top = false;

				{
					fixed_t linedist;

					linedist = P_AproxDistance(rover->master->v1->x-rover->master->v2->x, rover->master->v1->y-rover->master->v2->y);

					linedist = FixedDiv(linedist,100*FRACUNIT);

					if (top)
					{
						fixed_t newmom;

						pslope_t *slope;
						if (abs(oldz - topheight) < abs(oldz + player->mo->height - bottomheight)) { // Hit top
							slope = *rover->t_slope;
						} else { // Hit bottom
							slope = *rover->b_slope;
						}

						momentum.x = player->mo->momx;
						momentum.y = player->mo->momy;
						momentum.z = player->mo->momz*2;

						if (slope)
							P_ReverseQuantizeMomentumToSlope(&momentum, slope);

						newmom = momentum.z = -FixedMul(momentum.z,linedist)/2;

						if (abs(newmom) < (linedist*2))
						{
							goto bouncydone;
						}

						if (!(rover->master->flags & ML_NOTBOUNCY))
						{
							if (newmom > 0)
							{
								if (newmom < 8*FRACUNIT)
									newmom = 8*FRACUNIT;
							}
							else if (newmom > -8*FRACUNIT && newmom != 0)
								newmom = -8*FRACUNIT;
						}

						if (newmom > player->mo->height/2)
							newmom = player->mo->height/2;
						else if (newmom < -player->mo->height/2)
							newmom = -player->mo->height/2;

						momentum.z = newmom*2;

						if (slope)
							P_QuantizeMomentumToSlope(&momentum, slope);

						player->mo->momx = momentum.x;
						player->mo->momy = momentum.y;
						player->mo->momz = momentum.z/2;
					}
					else
					{
						player->mo->momx = -FixedMul(player->mo->momx,linedist);
						player->mo->momy = -FixedMul(player->mo->momy,linedist);
					}

					goto bouncydone;
				}
			}
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
		if (!(rover->flags & FF_EXISTS)) continue;

		if (!(rover->flags & FF_QUICKSAND))
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
					P_PlayerHitFloor(player, false);
			}
			else
			{
				fixed_t floorheight = P_GetFloorZ(player->mo, player->mo->subsector->sector, player->mo->x, player->mo->y, NULL);

				player->mo->z -= sinkspeed;

				if (player->mo->z <= floorheight)
					player->mo->z = floorheight;

				if (player->mo->momz >= 0)
					P_PlayerHitFloor(player, false);
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
	if (!player->powers[pw_invulnerability] && !player->kartstuff[k_invincibilitytimer])
		return;

	player->mo->color = (UINT16)(SKINCOLOR_PINK + (leveltime % (numskincolors - SKINCOLOR_PINK)));

	// Resume normal music stuff.
	if (player->powers[pw_invulnerability] == 1 || player->kartstuff[k_invincibilitytimer] == 1)
	{
		if (!player->powers[pw_super])
		{
			player->mo->color = player->skincolor;
			G_GhostAddColor((INT32) (player - players), GHC_NORMAL);
		}
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

	if (!(player->mo->eflags & MFE_UNDERWATER) || (player->powers[pw_shield] & SH_PROTECTWATER) || player->spectator)
		return;

	if (player->charflags & SF_MACHINE)
	{
		if (player->powers[pw_underwater] && P_RandomChance((128-(player->powers[pw_underwater]/4))*FRACUNIT/256))
		{
			fixed_t r = player->mo->radius>>FRACBITS;
			x += (P_RandomRange(r, -r)<<FRACBITS);
			y += (P_RandomRange(r, -r)<<FRACBITS);
			z += (P_RandomKey(player->mo->height>>FRACBITS)<<FRACBITS);
			bubble = P_SpawnMobj(x, y, z, MT_WATERZAP);
			S_StartSound(bubble, sfx_beelec);
		}
	}
	else
	{
		if (player->mo->eflags & MFE_VERTICALFLIP)
			z += player->mo->height - FixedDiv(player->mo->height,5*(FRACUNIT/4));
		else
			z += FixedDiv(player->mo->height,5*(FRACUNIT/4));

		if (P_RandomChance(FRACUNIT/16))
			bubble = P_SpawnMobj(x, y, z, MT_SMALLBUBBLE);
		else if (P_RandomChance(3*FRACUNIT/256))
			bubble = P_SpawnMobj(x, y, z, MT_MEDIUMBUBBLE);
	}

	if (bubble)
	{
		bubble->threshold = 42;
		bubble->destscale = player->mo->scale;
		P_SetScale(bubble, bubble->destscale);
	}
}

//#define OLD_MOVEMENT_CODE 1
static void P_3dMovement(player_t *player)
{
	ticcmd_t *cmd;
	angle_t movepushangle; // Analog
	fixed_t movepushforward = 0;
	angle_t dangle; // replaces old quadrants bits
	fixed_t oldMagnitude, newMagnitude;
	vector3_t totalthrust;

	totalthrust.x = totalthrust.y = 0; // I forget if this is needed
	totalthrust.z = FRACUNIT*P_MobjFlip(player->mo)/3; // A bit of extra push-back on slopes

	// Get the old momentum; this will be needed at the end of the function! -SH
	oldMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);

	cmd = &player->cmd;

	if (player->kartstuff[k_drift] != 0)
		movepushangle = player->mo->angle-(ANGLE_45/5)*player->kartstuff[k_drift];
	else if (player->kartstuff[k_spinouttimer] || player->kartstuff[k_wipeoutslow])	// if spun out, use the boost angle
		movepushangle = (angle_t)player->kartstuff[k_boostangle];
	else
		movepushangle = player->mo->angle;

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
	if (!player->kartstuff[k_drift]) // Not Drifting
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
	// SRB2Kart: pogo spring and speed bumps are supposed to control like you're on the ground
	onground = (P_IsObjectOnGround(player->mo) || (player->kartstuff[k_pogospring]));

	player->aiming = cmd->aiming<<FRACBITS;

	// Forward movement
	if (!P_PlayerInPain(player))
	{
		if (onground)
		{
			movepushforward = K_3dKartMovement(player, onground);

			if (player->mo->movefactor != FRACUNIT) // Friction-scaled acceleration...
				movepushforward = FixedMul(movepushforward, player->mo->movefactor);

			totalthrust.x += P_ReturnThrustX(player->mo, movepushangle, movepushforward);
			totalthrust.y += P_ReturnThrustY(player->mo, movepushangle, movepushforward);
		}
		else
		{
			K_MomentumToFacing(player);
		}
	}

	if ((totalthrust.x || totalthrust.y)
		&& player->mo->standingslope && (!(player->mo->standingslope->flags & SL_NOPHYSICS)) && abs(player->mo->standingslope->zdelta) > FRACUNIT/2) {
		// Factor thrust to slope, but only for the part pushing up it!
		// The rest is unaffected.
		angle_t thrustangle = R_PointToAngle2(0, 0, totalthrust.x, totalthrust.y)-player->mo->standingslope->xydirection;

		if (player->mo->standingslope->zdelta < 0) { // Direction goes down, so thrustangle needs to face toward
			if (thrustangle < ANGLE_90 || thrustangle > ANGLE_270) {
				P_QuantizeMomentumToSlope(&totalthrust, player->mo->standingslope);
			}
		} else { // Direction goes up, so thrustangle needs to face away
			if (thrustangle > ANGLE_90 && thrustangle < ANGLE_270) {
				P_QuantizeMomentumToSlope(&totalthrust, player->mo->standingslope);
			}
		}
	}

	if (K_PlayerUsesBotMovement(player))
	{
		K_MomentumToFacing(player);
	}

	player->mo->momx += totalthrust.x;
	player->mo->momy += totalthrust.y;

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
			if (K_PlayerUsesBotMovement(player))
			{
				fixed_t rubberband = K_BotRubberband(player) - FRACUNIT;

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
		newMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);
		if (newMagnitude > K_GetKartSpeed(player, true)) //topspeed)
		{
			fixed_t tempmomx, tempmomy;
			if (oldMagnitude > K_GetKartSpeed(player, true))
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
				tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), K_GetKartSpeed(player, true)); //topspeed)
				tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), K_GetKartSpeed(player, true)); //topspeed)
				player->mo->momx = tempmomx + player->cmomx;
				player->mo->momy = tempmomy + player->cmomy;
			}
		}
	}
}

//
// P_SpectatorMovement
//
// Control for spectators in multiplayer
//
static void P_SpectatorMovement(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;

	player->mo->angle = player->angleturn;

	ticruned++;
	if (!(cmd->flags & TICCMD_RECEIVED))
		ticmiss++;

	if (cmd->buttons & BT_ACCELERATE)
		player->mo->z += 32*mapobjectscale;
	else if (cmd->buttons & BT_BRAKE)
		player->mo->z -= 32*mapobjectscale;

	if (player->mo->z > player->mo->ceilingz - player->mo->height)
		player->mo->z = player->mo->ceilingz - player->mo->height;
	if (player->mo->z < player->mo->floorz)
		player->mo->z = player->mo->floorz;

	// Aiming needed for SEENAMES, etc.
	// We may not need to fire as a spectator, but this is still handy!
	player->aiming = cmd->aiming<<FRACBITS;

	player->mo->momx = player->mo->momy = player->mo->momz = 0;
	if (cmd->forwardmove != 0)
	{
		P_Thrust(player->mo, player->mo->angle, cmd->forwardmove*mapobjectscale);

		// Quake-style flying spectators :D
		player->mo->momz += FixedMul(cmd->forwardmove*mapobjectscale, AIMINGTOSLOPE(player->aiming));
	}
}

//
// P_MovePlayer
void P_MovePlayer(player_t *player)
{
	ticcmd_t *cmd;
	//INT32 i;

	fixed_t runspd;

	if (countdowntimeup)
		return;

	cmd = &player->cmd;
	runspd = 14*player->mo->scale; //srb2kart

	// Let's have some movement speed fun on low-friction surfaces, JUST for players...
	// (high friction surfaces shouldn't have any adjustment, since the acceleration in
	// this game is super high and that ends up cheesing high-friction surfaces.)
	runspd = FixedMul(runspd, player->mo->movefactor);

	// Control relinquishing stuff!
	if (player->powers[pw_nocontrol])
	{
		player->pflags |= PF_STASIS;
		if (!(player->powers[pw_nocontrol] & (1<<15)))
			player->pflags |= PF_JUMPSTASIS;
	}

	// note: don't unset stasis here

	if (player->spectator)
	{
		player->mo->eflags &= ~MFE_VERTICALFLIP; // deflip...
		P_SpectatorMovement(player);
		return;
	}

	//////////////////////
	// MOVEMENT CODE	//
	//////////////////////

	player->mo->angle = player->angleturn;

	ticruned++;
	if (!(cmd->flags & TICCMD_RECEIVED))
		ticmiss++;

	P_3dMovement(player);

	// Kart frames
	if (player->kartstuff[k_squishedtimer] > 0)
	{
		P_SetPlayerMobjState(player->mo, S_KART_SQUISH);
	}
	else if (player->pflags & PF_SLIDING)
	{
		P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
		player->drawangle -= ANGLE_22h;
	}
	else if (player->kartstuff[k_spinouttimer] > 0)
	{
		INT32 speed = max(1, min(8, player->kartstuff[k_spinouttimer]/8));

		P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);

		if (speed == 1 && abs((signed)(player->mo->angle - player->drawangle)) < ANGLE_22h)
			player->drawangle = player->mo->angle; // Face forward at the end of the animation
		else
			player->drawangle -= (ANGLE_11hh * speed);
	}
	else if (player->powers[pw_nocontrol] && player->pflags & PF_WPNDOWN)
	{
		P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);

		if (((player->powers[pw_nocontrol] + 5) % 20) < 10)
			player->drawangle += ANGLE_11hh;
		else
			player->drawangle -= ANGLE_11hh;
	}
	else
	{
		K_KartMoveAnimation(player);

		if (player->kartstuff[k_pogospring])
		{
			player->drawangle += ANGLE_22h;
		}
		else
		{
			player->drawangle = player->mo->angle;

			if (player->kartstuff[k_drift] != 0)
			{
				INT32 a = (ANGLE_45 / 5) * player->kartstuff[k_drift];
				player->drawangle += a;
			}
		}
	}

	player->mo->movefactor = FRACUNIT; // We're not going to do any more with this, so let's change it back for the next frame.

	//{ SRB2kart

	// Drifting sound
	// Start looping the sound now.
	if (leveltime % 50 == 0 && onground && player->kartstuff[k_drift] != 0)
		S_StartSound(player->mo, sfx_drift);
	// Leveltime being 50 might take a while at times. We'll start it up once, isntantly.
	else if (!S_SoundPlaying(player->mo, sfx_drift) && onground && player->kartstuff[k_drift] != 0)
		S_StartSound(player->mo, sfx_drift);
	// Ok, we'll stop now.
	else if (player->kartstuff[k_drift] == 0)
		S_StopSoundByID(player->mo, sfx_drift);

	K_MoveKartPlayer(player, onground);
	//}

//////////////////
//GAMEPLAY STUFF//
//////////////////

	// If you're running fast enough, you can create splashes as you run in shallow water.
#if 0
	if (!player->climbing
	&& ((!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + player->mo->height >= player->mo->watertop && player->mo->z <= player->mo->watertop)
		|| (player->mo->eflags & MFE_VERTICALFLIP && player->mo->z + player->mo->height >= player->mo->waterbottom && player->mo->z <= player->mo->waterbottom))
	&& (player->speed > runspd || (player->pflags & PF_STARTDASH))
	&& leveltime % (TICRATE/7) == 0 && player->mo->momz == 0 && !(player->pflags & PF_SLIDING) && !player->spectator)
	{
		mobjtype_t splishtype = (player->mo->eflags & MFE_TOUCHLAVA) ? MT_LAVASPLISH : MT_SPLISH;
		mobj_t *water = P_SpawnMobj(player->mo->x - P_ReturnThrustX(NULL, player->mo->angle, player->mo->radius), player->mo->y - P_ReturnThrustY(NULL, player->mo->angle, player->mo->radius),
			((player->mo->eflags & MFE_VERTICALFLIP) ? player->mo->waterbottom - FixedMul(mobjinfo[splishtype].height, player->mo->scale) : player->mo->watertop), splishtype);
		if (player->mo->eflags & MFE_GOOWATER)
			S_StartSound(water, sfx_ghit);
		else if (player->mo->eflags & MFE_TOUCHLAVA)
			S_StartSound(water, sfx_splash);
		else
			S_StartSound(water, sfx_wslap);
		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			water->flags2 |= MF2_OBJECTFLIP;
			water->eflags |= MFE_VERTICALFLIP;
		}
		water->destscale = player->mo->scale;
		P_SetScale(water, player->mo->scale);
	}
#endif

	if (!player->climbing
	&& ((!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + player->mo->height >= player->mo->watertop && player->mo->z <= player->mo->watertop)
		|| (player->mo->eflags & MFE_VERTICALFLIP && player->mo->z + player->mo->height >= player->mo->waterbottom && player->mo->z <= player->mo->waterbottom))
	&& (player->speed > runspd || (player->pflags & PF_STARTDASH))
	&& player->mo->momz == 0 && !(player->pflags & PF_SLIDING) && !player->spectator)
	{
		fixed_t trailScale = FixedMul(FixedDiv(player->speed - runspd, K_GetKartSpeed(player, false) - runspd), mapobjectscale);
		fixed_t playerTopSpeed = K_GetKartSpeed(player, false);

		if (playerTopSpeed > runspd)
			trailScale = FixedMul(FixedDiv(player->speed - runspd, playerTopSpeed - runspd), mapobjectscale);
		else
			trailScale = mapobjectscale; // Scaling is based off difference between runspeed and top speed

		if (trailScale > 0)
		{
			const angle_t forwardangle = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
			const fixed_t playerVisualRadius = player->mo->radius + 8*FRACUNIT;
			const size_t numFrames = S_WATERTRAIL8 - S_WATERTRAIL1;
			const statenum_t curOverlayFrame = S_WATERTRAIL1 + (leveltime % numFrames);
			const statenum_t curUnderlayFrame = S_WATERTRAILUNDERLAY1 + (leveltime % numFrames);
			fixed_t x1, x2, y1, y2;
			mobj_t *water;

			x1 = player->mo->x + player->mo->momx + P_ReturnThrustX(player->mo, forwardangle + ANGLE_90, playerVisualRadius);
			y1 = player->mo->y + player->mo->momy + P_ReturnThrustY(player->mo, forwardangle + ANGLE_90, playerVisualRadius);
			x1 = x1 + P_ReturnThrustX(player->mo, forwardangle, playerVisualRadius);
			y1 = y1 + P_ReturnThrustY(player->mo, forwardangle, playerVisualRadius);

			x2 = player->mo->x + player->mo->momx + P_ReturnThrustX(player->mo, forwardangle - ANGLE_90, playerVisualRadius);
			y2 = player->mo->y + player->mo->momy + P_ReturnThrustY(player->mo, forwardangle - ANGLE_90, playerVisualRadius);
			x2 = x2 + P_ReturnThrustX(player->mo, forwardangle, playerVisualRadius);
			y2 = y2 + P_ReturnThrustY(player->mo, forwardangle, playerVisualRadius);

			// Left
			// underlay
			water = P_SpawnMobj(x1, y1,
				((player->mo->eflags & MFE_VERTICALFLIP) ? player->mo->waterbottom - FixedMul(mobjinfo[MT_WATERTRAILUNDERLAY].height, player->mo->scale) : player->mo->watertop), MT_WATERTRAILUNDERLAY);
			water->angle = forwardangle - ANGLE_180 - ANGLE_22h;
			water->destscale = trailScale;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curUnderlayFrame);

			// overlay
			water = P_SpawnMobj(x1, y1,
				((player->mo->eflags & MFE_VERTICALFLIP) ? player->mo->waterbottom - FixedMul(mobjinfo[MT_WATERTRAIL].height, player->mo->scale) : player->mo->watertop), MT_WATERTRAIL);
			water->angle = forwardangle - ANGLE_180 - ANGLE_22h;
			water->destscale = trailScale;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curOverlayFrame);

			// Right
			// Underlay
			water = P_SpawnMobj(x2, y2,
				((player->mo->eflags & MFE_VERTICALFLIP) ? player->mo->waterbottom - FixedMul(mobjinfo[MT_WATERTRAILUNDERLAY].height, player->mo->scale) : player->mo->watertop), MT_WATERTRAILUNDERLAY);
			water->angle = forwardangle - ANGLE_180 + ANGLE_22h;
			water->destscale = trailScale;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curUnderlayFrame);

			// Overlay
			water = P_SpawnMobj(x2, y2,
				((player->mo->eflags & MFE_VERTICALFLIP) ? player->mo->waterbottom - FixedMul(mobjinfo[MT_WATERTRAIL].height, player->mo->scale) : player->mo->watertop), MT_WATERTRAIL);
			water->angle = forwardangle - ANGLE_180 + ANGLE_22h;
			water->destscale = trailScale;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curOverlayFrame);

			if (!S_SoundPlaying(player->mo, sfx_s3kdbs))
			{
				const INT32 volume = (min(trailScale, FRACUNIT) * 255) / FRACUNIT;
				S_StartSoundAtVolume(player->mo, sfx_s3kdbs, volume);
			}
		}
	}

	// Little water sound while touching water - just a nicety.
	if ((player->mo->eflags & MFE_TOUCHWATER) && !(player->mo->eflags & MFE_UNDERWATER) && !player->spectator)
	{
		if (P_RandomChance(FRACUNIT/2) && leveltime % TICRATE == 0)
			S_StartSound(player->mo, sfx_floush);
	}

	////////////////////////////
	//SPINNING AND SPINDASHING//
	////////////////////////////

	// SRB2kart - Drifting smoke and fire
	if ((player->kartstuff[k_sneakertimer] || player->kartstuff[k_flamedash])
		&& onground && (leveltime & 1))
		K_SpawnBoostTrail(player);

	if (player->kartstuff[k_invincibilitytimer] > 0)
		K_SpawnSparkleTrail(player->mo);

	if (player->kartstuff[k_wipeoutslow] > 1 && (leveltime & 1))
		K_SpawnWipeoutTrail(player->mo, false);

	K_DriftDustHandling(player->mo);

	// Crush test...
	if ((player->mo->ceilingz - player->mo->floorz < player->mo->height) && !(player->mo->flags & MF_NOCLIP))
	{
		if ((netgame || multiplayer) && player->spectator)
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_SPECTATOR); // Respawn crushed spectators
		else
		{
			K_SquishPlayer(player, NULL, NULL); // SRB2kart - we don't kill when squished, we squish when squished.
			// P_DamageMobj(player->mo, NULL, NULL, 1, DMG_CRUSHED);
		}

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

	if (P_IsObjectOnGround(player->mo))
		player->mo->pmomz = 0;
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

	player->powers[pw_flashing] = 1;

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
			player->powers[pw_carry] = CR_NONE;

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

		if (mo->flags & MF_MONITOR)
			continue; // Monitors cannot be 'nuked'.

		if (abs(inflictor->x - mo->x) > radius || abs(inflictor->y - mo->y) > radius || abs(inflictor->z - mo->z) > radius)
			continue; // Workaround for possible integer overflow in the below -Red

		if (P_AproxDistance(P_AproxDistance(inflictor->x - mo->x, inflictor->y - mo->y), inflictor->z - mo->z) > radius)
			continue;

		if (mo->flags & MF_BOSS || mo->type == MT_PLAYER) //don't OHKO bosses nor players!
			P_DamageMobj(mo, inflictor, source, 1, DMG_NORMAL|DMG_CANTHURTSELF);
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
	player->deltaviewheight = 0;

	if (player->deadtimer < INT32_MAX)
		player->deadtimer++;

	if (player->bot) // don't allow bots to do any of the below, B_CheckRespawn does all they need for respawning already
		goto notrealplayer;

	if ((player->pflags & PF_GAMETYPEOVER) && (gametyperules & GTR_CIRCUIT))
	{
		player->karthud[khud_timeovercam]++;

		if (player->mo)
		{
			player->mo->flags |= (MF_NOGRAVITY|MF_NOCLIP);
			player->mo->drawflags |= MFD_DONTDRAW;
		}
	}
	else
		player->karthud[khud_timeovercam] = 0;

	K_KartPlayerHUDUpdate(player);

	if (player->lives > 0 && !(player->pflags & PF_GAMETYPEOVER) && player->deadtimer > TICRATE)
	{
		player->playerstate = PST_REBORN;
	}

	// Keep time rolling
	if (!(exitcountdown && !racecountdown) && !(player->exiting || mapreset) && !(player->pflags & PF_GAMETYPEOVER) && !stoppedclock)
	{
		if (leveltime >= starttime)
		{
			player->realtime = leveltime - starttime;
			if (player == &players[consoleplayer])
			{
				if (player->spectator || !circuitmap)
					curlap = 0;
				else
					curlap++; // This is too complicated to sync to realtime, just sorta hope for the best :V
			}
		}
		else
		{
			player->realtime = 0;
			if (player == &players[consoleplayer])
				curlap = 0;
		}
	}

notrealplayer:

	if (!player->mo)
		return;

	player->mo->colorized = false;
	player->mo->color = player->skincolor;

	P_CalcHeight(player);
}

//
// P_MoveCamera: make sure the camera is not outside the world and looks at the player avatar
//

camera_t camera[MAXSPLITSCREENPLAYERS]; // Four cameras, three for splitscreen

static void CV_CamRotate_OnChange(void)
{
	if (cv_cam_rotate[0].value < 0)
		CV_SetValue(&cv_cam_rotate[0], cv_cam_rotate[0].value + 360);
	else if (cv_cam_rotate[0].value > 359)
		CV_SetValue(&cv_cam_rotate[0], cv_cam_rotate[0].value % 360);
}

static void CV_CamRotate2_OnChange(void)
{
	if (cv_cam_rotate[1].value < 0)
		CV_SetValue(&cv_cam_rotate[1], cv_cam_rotate[1].value + 360);
	else if (cv_cam_rotate[1].value > 359)
		CV_SetValue(&cv_cam_rotate[1], cv_cam_rotate[1].value % 360);
}

static void CV_CamRotate3_OnChange(void)
{
	if (cv_cam_rotate[2].value < 0)
		CV_SetValue(&cv_cam_rotate[2], cv_cam_rotate[2].value + 360);
	else if (cv_cam_rotate[2].value > 359)
		CV_SetValue(&cv_cam_rotate[2], cv_cam_rotate[2].value % 360);
}

static void CV_CamRotate4_OnChange(void)
{
	if (cv_cam_rotate[3].value < 0)
		CV_SetValue(&cv_cam_rotate[3], cv_cam_rotate[3].value + 360);
	else if (cv_cam_rotate[3].value > 359)
		CV_SetValue(&cv_cam_rotate[3], cv_cam_rotate[3].value % 360);
}

static CV_PossibleValue_t CV_CamSpeed[] = {{0, "MIN"}, {1*FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t CV_CamRotate[] = {{-720, "MIN"}, {720, "MAX"}, {0, NULL}};

consvar_t cv_cam_dist[MAXSPLITSCREENPLAYERS] = {
	{"cam_dist", "160", CV_FLOAT|CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam2_dist", "160", CV_FLOAT|CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam3_dist", "160", CV_FLOAT|CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam4_dist", "160", CV_FLOAT|CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL}
};

consvar_t cv_cam_height[MAXSPLITSCREENPLAYERS] = {
	{"cam_height", "50", CV_FLOAT|CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam2_height", "50", CV_FLOAT|CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam3_height", "50", CV_FLOAT|CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam4_height", "50", CV_FLOAT|CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL}
};

consvar_t cv_cam_still[MAXSPLITSCREENPLAYERS] = {
	{"cam_still", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam2_still", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam3_still", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam4_still", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL}
};

consvar_t cv_cam_speed[MAXSPLITSCREENPLAYERS] = {
	{"cam_speed", "0.4", CV_FLOAT|CV_SAVE, CV_CamSpeed, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam2_speed", "0.4", CV_FLOAT|CV_SAVE, CV_CamSpeed, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam3_speed", "0.4", CV_FLOAT|CV_SAVE, CV_CamSpeed, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam4_speed", "0.4", CV_FLOAT|CV_SAVE, CV_CamSpeed, NULL, 0, NULL, NULL, 0, 0, NULL}
};

consvar_t cv_cam_rotate[MAXSPLITSCREENPLAYERS] = {
	{"cam_rotate", "0", CV_CALL|CV_NOINIT, CV_CamRotate, CV_CamRotate_OnChange, 0, NULL, NULL, 0, 0, NULL},
	{"cam2_rotate", "0", CV_CALL|CV_NOINIT, CV_CamRotate, CV_CamRotate2_OnChange, 0, NULL, NULL, 0, 0, NULL},
	{"cam3_rotate", "0", CV_CALL|CV_NOINIT, CV_CamRotate, CV_CamRotate3_OnChange, 0, NULL, NULL, 0, 0, NULL},
	{"cam4_rotate", "0", CV_CALL|CV_NOINIT, CV_CamRotate, CV_CamRotate4_OnChange, 0, NULL, NULL, 0, 0, NULL}
};

static CV_PossibleValue_t slopeview_cons_t[] =
{
	{ 0, "Off" },
	{ 1, "On" },
	{ 2, "Inverse" },

	{0}
};
consvar_t cv_slopeview = {"slopeview", "On", CV_SAVE, slopeview_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_quaketilt = {"quaketilt", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_actionmovie = {"actionmovie", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

fixed_t t_cam_dist[MAXSPLITSCREENPLAYERS] = {-42,-42,-42,-42};
fixed_t t_cam_height[MAXSPLITSCREENPLAYERS] = {-42,-42,-42,-42};
fixed_t t_cam_rotate[MAXSPLITSCREENPLAYERS] = {-42,-42,-42,-42};

// Heavily simplified version of G_BuildTicCmd that only takes the local first player's control input and converts it to readable ticcmd_t
// we then throw that ticcmd garbage in the camera and make it move

// redefine this
static fixed_t angleturn[2] = {KART_FULLTURN, KART_FULLTURN/4}; // + slow turn

static ticcmd_t cameracmd;

struct demofreecam_s democam;

// called by m_menu to reinit cam input every time it's toggled
void P_InitCameraCmd(void)
{
	memset(&cameracmd, 0, sizeof(ticcmd_t));	// initialize cmd
}

static ticcmd_t *P_CameraCmd(camera_t *cam)
{
	INT32 laim, th, tspeed, forward, axis; //i
	// these ones used for multiple conditions
	boolean turnleft, turnright, mouseaiming;
	boolean invertmouse, lookaxis, usejoystick, kbl;
	angle_t lang;
	INT32 player_invert;
	INT32 screen_invert;
	ticcmd_t *cmd = &cameracmd;

	(void)cam;

	if (!demo.playback)
		return cmd;	// empty cmd, no.

	lang = democam.localangle;
	laim = democam.localaiming;
	th = democam.turnheld;
	kbl = democam.keyboardlook;

	G_CopyTiccmd(cmd, I_BaseTiccmd(), 1); // empty, or external driver

	//cmd->turning = (INT16)(lang >> 16);
	cmd->aiming = G_ClipAimingPitch(&laim);

	mouseaiming = true;
	invertmouse = cv_invertmouse.value;
	lookaxis = cv_lookaxis[0].value;

	usejoystick = true;
	turnright = PlayerInputDown(1, gc_turnright);
	turnleft = PlayerInputDown(1, gc_turnleft);

	axis = PlayerJoyAxis(1, AXISTURN);

	if (encoremode)
	{
		turnright ^= turnleft; // swap these using three XORs
		turnleft ^= turnright;
		turnright ^= turnleft;
		axis = -axis;
	}

	if (axis != 0)
	{
		turnright = turnright || (axis > 0);
		turnleft = turnleft || (axis < 0);
	}
	forward = 0;

	// use two stage accelerative turning
	// on the keyboard and joystick
	if (turnleft || turnright)
		th += 1;
	else
		th = 0;

	if (th < SLOWTURNTICS)
		tspeed = 1; // slow turn
	else
		tspeed = 0;

	cmd->turning = 0;

	// let movement keys cancel each other out
	if (turnright && !(turnleft))
	{
		cmd->turning = (INT16)(cmd->turning - (angleturn[tspeed]));
	}
	else if (turnleft && !(turnright))
	{
		cmd->turning = (INT16)(cmd->turning + (angleturn[tspeed]));
	}

	cmd->turning = (INT16)(cmd->turning - ((mousex*(encoremode ? -1 : 1)*8)));

	axis = PlayerJoyAxis(1, AXISMOVE);
	if (PlayerInputDown(1, gc_accelerate) || (usejoystick && axis > 0))
		cmd->buttons |= BT_ACCELERATE;
	axis = PlayerJoyAxis(1, AXISBRAKE);
	if (PlayerInputDown(1, gc_brake) || (usejoystick && axis > 0))
		cmd->buttons |= BT_BRAKE;
	axis = PlayerJoyAxis(1, AXISAIM);
	if (PlayerInputDown(1, gc_aimforward) || (usejoystick && axis < 0))
		forward += MAXPLMOVE;
	if (PlayerInputDown(1, gc_aimbackward) || (usejoystick && axis > 0))
		forward -= MAXPLMOVE;

	// fire with any button/key
	axis = PlayerJoyAxis(1, AXISFIRE);
	if (PlayerInputDown(1, gc_fire) || (usejoystick && axis > 0))
		cmd->buttons |= BT_ATTACK;

	// spectator aiming shit, ahhhh...
	player_invert = invertmouse ? -1 : 1;
	screen_invert = 1;	// nope

	// mouse look stuff (mouse look is not the same as mouse aim)
	kbl = false;

	// looking up/down
	laim += (mlooky<<19)*player_invert*screen_invert;

	axis = PlayerJoyAxis(1, AXISLOOK);

	// spring back if not using keyboard neither mouselookin'
	if (!kbl && !lookaxis && !mouseaiming)
		laim = 0;

	if (PlayerInputDown(1, gc_lookup) || (axis < 0))
	{
		laim += KB_LOOKSPEED * screen_invert;
		kbl = true;
	}
	else if (PlayerInputDown(1, gc_lookdown) || (axis > 0))
	{
		laim -= KB_LOOKSPEED * screen_invert;
		kbl = true;
	}

	if (PlayerInputDown(1, gc_centerview)) // No need to put a spectator limit on this one though :V
		laim = 0;

	cmd->aiming = G_ClipAimingPitch(&laim);

	mousex = mousey = mlooky = 0;

	cmd->forwardmove += (SINT8)forward;

	if (cmd->forwardmove > MAXPLMOVE)
		cmd->forwardmove = MAXPLMOVE;
	else if (cmd->forwardmove < -MAXPLMOVE)
		cmd->forwardmove = -MAXPLMOVE;

	lang += (cmd->turning << TICCMD_REDUCE);

	democam.localangle = lang;
	democam.localaiming = laim;
	democam.turnheld = th;
	democam.keyboardlook = kbl;

	return cmd;
}

void P_DemoCameraMovement(camera_t *cam)
{
	ticcmd_t *cmd;
	angle_t thrustangle;
	mobj_t *awayviewmobj_hack;
	player_t *lastp;

	// update democam stuff with what we got here:
	democam.cam = cam;
	democam.localangle = cam->angle;
	democam.localaiming = cam->aiming;

	// first off we need to get button input
	cmd = P_CameraCmd(cam);

	cam->aiming = cmd->aiming<<FRACBITS;
	//cam->angle = cmd->angleturn << TICCMD_REDUCE;

	// camera movement:

	if (cmd->buttons & BT_ACCELERATE)
		cam->z += 32*mapobjectscale;
	else if (cmd->buttons & BT_BRAKE)
		cam->z -= 32*mapobjectscale;

	// if you hold item, you will lock on to displayplayer. (The last player you were ""f12-ing"")
	if (cmd->buttons & BT_ATTACK)
	{
		lastp = &players[displayplayers[0]];	// Fun fact, I was trying displayplayers[0]->mo as if it was Lua like an absolute idiot.
		cam->angle = R_PointToAngle2(cam->x, cam->y, lastp->mo->x, lastp->mo->y);
		cam->aiming = R_PointToAngle2(0, cam->z, R_PointToDist2(cam->x, cam->y, lastp->mo->x, lastp->mo->y), lastp->mo->z + lastp->mo->scale*128*P_MobjFlip(lastp->mo));	// This is still unholy. Aim a bit above their heads.
	}

	cam->momx = cam->momy = cam->momz = 0;

	if (cmd->forwardmove != 0)
	{
		thrustangle = cam->angle >> ANGLETOFINESHIFT;

		cam->x += FixedMul(cmd->forwardmove*mapobjectscale, FINECOSINE(thrustangle));
		cam->y += FixedMul(cmd->forwardmove*mapobjectscale, FINESINE(thrustangle));
		cam->z += FixedMul(cmd->forwardmove*mapobjectscale, AIMINGTOSLOPE(cam->aiming));
		// momentums are useless here, directly add to the coordinates

		// this.......... doesn't actually check for floors and walls and whatnot but the function to do that is a pure mess so fuck that.
		// besides freecam going inside walls sounds pretty cool on paper.
	}

	// awayviewmobj hack; this is to prevent us from hearing sounds from the player's perspective

	awayviewmobj_hack = P_SpawnMobj(cam->x, cam->y, cam->z, MT_THOK);
	awayviewmobj_hack->tics = 2;
	awayviewmobj_hack->drawflags |= MFD_DONTDRAW;

	democam.soundmobj = awayviewmobj_hack;

	// update subsector to avoid crashes;
	cam->subsector = R_PointInSubsector(cam->x, cam->y);
}

void P_ResetCamera(player_t *player, camera_t *thiscam)
{
	tic_t tries = 0;
	fixed_t x, y, z;

	if (demo.freecam)
		return;	// do not reset the camera there.

	if (!player->mo)
		return;

	if (thiscam->chase && player->mo->health <= 0)
		return;

	thiscam->chase = true;
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

	thiscam->angle = player->mo->angle;
	thiscam->aiming = 0;
	thiscam->relativex = 0;

	thiscam->subsector = R_PointInSubsector(thiscam->x,thiscam->y);

	thiscam->radius = 20*FRACUNIT;
	thiscam->height = 16*FRACUNIT;

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
#ifndef NOCLIPCAM
	boolean cameranoclip;
	subsector_t *newsubsec;
#endif

	democam.soundmobj = NULL;	// reset this each frame, we don't want the game crashing for stupid reasons now do we

	// We probably shouldn't move the camera if there is no player or player mobj somehow
	if (!player || !player->mo)
		return true;

	// This can happen when joining
	if (thiscam->subsector == NULL || thiscam->subsector->sector == NULL)
		return true;

	if (demo.freecam)
	{
		P_DemoCameraMovement(thiscam);
		return true;
	}

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

	mo = player->mo;

#ifndef NOCLIPCAM
	cameranoclip = ((player->pflags & PF_NOCLIP)
		|| (mo->flags & (MF_NOCLIP|MF_NOCLIPHEIGHT)) // Noclipping player camera noclips too!!
		|| (leveltime < introtime)); // Kart intro cam
#endif

	if ((player->pflags & PF_GAMETYPEOVER) && (gametyperules & GTR_CIRCUIT)) // 1 for momentum keep, 2 for turnaround
		timeover = (player->karthud[khud_timeovercam] > 2*TICRATE ? 2 : 1);
	else
		timeover = 0;

	if (!(player->playerstate == PST_DEAD || player->exiting))
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

	thiscam->radius = 20*mapobjectscale;
	thiscam->height = 16*mapobjectscale;

	// Don't run while respawning from a starpost
	// Inu 4/8/13 Why not?!
//	if (leveltime > 0 && timeinmap <= 0)
//		return true;

	if (demo.playback)
	{
		focusangle = mo->angle;
		focusaiming = 0;
	}
	else
	{
		focusangle = localangle[num];
		focusaiming = localaiming[num];
	}

	if (P_CameraThinker(player, thiscam, resetcalled))
		return true;

	lookback = ( player->cmd.buttons & BT_LOOKBACK );

	camspeed = cv_cam_speed[num].value;
	camstill = cv_cam_still[num].value;
	camrotate = cv_cam_rotate[num].value;
	camdist = FixedMul(cv_cam_dist[num].value, mapobjectscale);
	camheight = FixedMul(cv_cam_height[num].value, mapobjectscale);

	if (timeover)
	{
		const INT32 timeovercam = max(0, min(180, (player->karthud[khud_timeovercam] - 2*TICRATE)*15));
		camrotate += timeovercam;
	}
	else if (leveltime < introtime && !(modeattacking && !demo.playback)) // Whoooshy camera! (don't do this in RA when we PLAY, still do it in replays however~)
	{
		const INT32 introcam = (introtime - leveltime);
		camrotate += introcam*5;
		camdist += (introcam * mapobjectscale)*3;
		camheight += (introcam * mapobjectscale)*2;
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

	if (!resetcalled && (leveltime >= introtime && timeover != 2)
		&& (t_cam_rotate[num] != -42))
	{
		angle = FixedAngle(camrotate*FRACUNIT);
		thiscam->angle = angle;
	}

	// sets ideal cam pos
	dist = camdist;

	/* player->speed subtracts conveyors, janks up the camera */
	speed = R_PointToDist2(0, 0, player->mo->momx, player->mo->momy);

	if (speed > K_GetKartSpeed(player, false))
		dist += 4*(speed - K_GetKartSpeed(player, false));
	dist += abs(thiscam->momz)/4;

	if (player->karthud[khud_boostcam])
		dist -= FixedMul(11*dist/16, player->karthud[khud_boostcam]);

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
	if (camstill || resetcalled || player->playerstate == PST_DEAD)
		pan = xpan = ypan = 0;
	else
	{
		if (player->kartstuff[k_drift] != 0)
		{
			fixed_t panmax = (dist/5);
			INT32 driftval = K_GetKartDriftSparkValue(player);
			INT32 dc = player->kartstuff[k_driftcharge];

			if (dc > driftval || dc < 0)
				dc = driftval;

			pan = FixedDiv(FixedMul((fixed_t)dc, panmax), driftval);

			if (pan > panmax)
				pan = panmax;
			if (player->kartstuff[k_drift] < 0)
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

#ifndef NOCLIPCAM // Disable all z-clipping for noclip cam
	// move camera down to move under lower ceilings
	newsubsec = R_PointInSubsectorOrNull(((mo->x>>FRACBITS) + (thiscam->x>>FRACBITS))<<(FRACBITS-1), ((mo->y>>FRACBITS) + (thiscam->y>>FRACBITS))<<(FRACBITS-1));

	if (!newsubsec)
		newsubsec = thiscam->subsector;

	if (newsubsec)
	{
		fixed_t myfloorz, myceilingz;
		fixed_t midz = thiscam->z + (thiscam->z - mo->z)/2;
		fixed_t midx = ((mo->x>>FRACBITS) + (thiscam->x>>FRACBITS))<<(FRACBITS-1);
		fixed_t midy = ((mo->y>>FRACBITS) + (thiscam->y>>FRACBITS))<<(FRACBITS-1);

		// Cameras use the heightsec's heights rather then the actual sector heights.
		// If you can see through it, why not move the camera through it too?
		if (newsubsec->sector->camsec >= 0)
		{
			myfloorz = sectors[newsubsec->sector->camsec].floorheight;
			myceilingz = sectors[newsubsec->sector->camsec].ceilingheight;
		}
		else if (newsubsec->sector->heightsec >= 0)
		{
			myfloorz = sectors[newsubsec->sector->heightsec].floorheight;
			myceilingz = sectors[newsubsec->sector->heightsec].ceilingheight;
		}
		else
		{
			myfloorz = P_CameraGetFloorZ(thiscam, newsubsec->sector, midx, midy, NULL);
			myceilingz = P_CameraGetCeilingZ(thiscam, newsubsec->sector, midx, midy, NULL);
		}

		// Check list of fake floors and see if floorz/ceilingz need to be altered.
		if (newsubsec->sector->ffloors)
		{
			ffloor_t *rover;
			fixed_t delta1, delta2;
			INT32 thingtop = midz + thiscam->height;

			for (rover = newsubsec->sector->ffloors; rover; rover = rover->next)
			{
				fixed_t topheight, bottomheight;
				if (!(rover->flags & FF_BLOCKOTHERS) || !(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERALL) || GETSECSPECIAL(rover->master->frontsector->special, 4) == 12)
					continue;

				topheight = P_CameraGetFOFTopZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);
				bottomheight = P_CameraGetFOFBottomZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);

				delta1 = midz - (bottomheight
					+ ((topheight - bottomheight)/2));
				delta2 = thingtop - (bottomheight
					+ ((topheight - bottomheight)/2));
				if (topheight > myfloorz && abs(delta1) < abs(delta2))
					myfloorz = topheight;
				if (bottomheight < myceilingz && abs(delta1) >= abs(delta2))
					myceilingz = bottomheight;
			}
		}

	// Check polyobjects and see if floorz/ceilingz need to be altered
	{
		INT32 xl, xh, yl, yh, bx, by;
		validcount++;

		xl = (unsigned)(tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
		xh = (unsigned)(tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
		yl = (unsigned)(tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
		yh = (unsigned)(tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

		BMBOUNDFIX(xl, xh, yl, yh);

		for (by = yl; by <= yh; by++)
			for (bx = xl; bx <= xh; bx++)
			{
				INT32 offset;
				polymaplink_t *plink; // haleyjd 02/22/06

				if (bx < 0 || by < 0 || bx >= bmapwidth || by >= bmapheight)
					continue;

				offset = by*bmapwidth + bx;

				// haleyjd 02/22/06: consider polyobject lines
				plink = polyblocklinks[offset];

				while (plink)
				{
					polyobj_t *po = plink->po;

					if (po->validcount != validcount) // if polyobj hasn't been checked
					{
						sector_t *polysec;
						fixed_t delta1, delta2, thingtop;
						fixed_t polytop, polybottom;

						po->validcount = validcount;

						if (!P_PointInsidePolyobj(po, x, y) || !(po->flags & POF_SOLID))
						{
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}

						// We're inside it! Yess...
						polysec = po->lines[0]->backsector;

						if (GETSECSPECIAL(polysec->special, 4) == 12)
						{ // Camera noclip polyobj.
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}

						if (po->flags & POF_CLIPPLANES)
						{
							polytop = polysec->ceilingheight;
							polybottom = polysec->floorheight;
						}
						else
						{
							polytop = INT32_MAX;
							polybottom = INT32_MIN;
						}

						thingtop = midz + thiscam->height;
						delta1 = midz - (polybottom + ((polytop - polybottom)/2));
						delta2 = thingtop - (polybottom + ((polytop - polybottom)/2));

						if (polytop > myfloorz && abs(delta1) < abs(delta2))
							myfloorz = polytop;

						if (polybottom < myceilingz && abs(delta1) >= abs(delta2))
							myceilingz = polybottom;
					}
					plink = (polymaplink_t *)(plink->link.next);
				}
			}
	}

		// crushed camera
		if (myceilingz <= myfloorz + thiscam->height && !resetcalled && !cameranoclip)
		{
			P_ResetCamera(player, thiscam);
			return true;
		}

		// camera fit?
		if (myceilingz != myfloorz
			&& myceilingz - thiscam->height < z)
		{
/*			// no fit
			if (!resetcalled && !cameranoclip)
			{
				P_ResetCamera(player, thiscam);
				return true;
			}
*/
			z = myceilingz - thiscam->height-FixedMul(11*FRACUNIT, mo->scale);
			// is the camera fit is there own sector
		}

		// Make the camera a tad smarter with 3d floors
		if (newsubsec->sector->ffloors && !cameranoclip)
		{
			ffloor_t *rover;

			for (rover = newsubsec->sector->ffloors; rover; rover = rover->next)
			{
				fixed_t topheight, bottomheight;
				if ((rover->flags & FF_BLOCKOTHERS) && (rover->flags & FF_RENDERALL) && (rover->flags & FF_EXISTS) && GETSECSPECIAL(rover->master->frontsector->special, 4) == 12)
				{
					topheight = P_CameraGetFOFTopZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);
					bottomheight = P_CameraGetFOFBottomZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);

					if (bottomheight - thiscam->height < z
						&& midz < bottomheight)
						z = bottomheight - thiscam->height-FixedMul(11*FRACUNIT, mo->scale);

					else if (topheight + thiscam->height > z
						&& midz > topheight)
						z = topheight;

					if ((mo->z >= topheight && midz < bottomheight)
						|| ((mo->z < bottomheight && mo->z+mo->height < topheight) && midz >= topheight))
					{
						// Can't see
						if (!resetcalled)
							P_ResetCamera(player, thiscam);
						return true;
					}
				}
			}
		}
	}

	if (thiscam->z < thiscam->floorz && !cameranoclip)
		thiscam->z = thiscam->floorz;
#endif // NOCLIPCAM

	// point viewed by the camera
	// this point is just 64 unit forward the player
	dist = 64*mapobjectscale;
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
		thiscam->momx = thiscam->momy = thiscam->momz = 0;
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
		thiscam->momz = FixedMul(z - thiscam->z, camspeed/2);
	}

	thiscam->pan = pan;
	thiscam->pitch = pitch;

	// compute aming to look the viewed point
	f1 = viewpointx-thiscam->x;
	f2 = viewpointy-thiscam->y;
	dist = FixedHypot(f1, f2);

	if (mo->eflags & MFE_VERTICALFLIP)
	{
		angle = R_PointToAngle2(0, thiscam->z + thiscam->height, dist, mo->z + mo->height - player->mo->height);
		if (thiscam->pitch < ANGLE_180 && thiscam->pitch > angle)
			angle += (thiscam->pitch - angle)/2;
	}
	else
	{
		angle = R_PointToAngle2(0, thiscam->z, dist, mo->z + player->mo->height);
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
		P_MoveChaseCamera(player, thiscam, false);

	return (x == thiscam->x && y == thiscam->y && z == thiscam->z && angle == thiscam->aiming);

}

boolean P_SpectatorJoinGame(player_t *player)
{
	// Team changing isn't allowed.
	if (!cv_allowteamchange.value)
	{
		if (P_IsLocalPlayer(player))
			CONS_Printf(M_GetText("Server does not allow team change.\n"));
		//player->powers[pw_flashing] = TICRATE + 1; //to prevent message spam.
	}
	// Team changing in Team Match and CTF
	// Pressing fire assigns you to a team that needs players if allowed.
	// Partial code reproduction from p_tick.c autobalance code.
	else if (G_GametypeHasTeams())
	{
		INT32 changeto = 0;
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
			changeto = (P_RandomFixed() & 1) + 1;

		if (!LUAh_TeamSwitch(player, changeto, true, false, false))
			return false;

		if (player->mo)
		{
			P_RemoveMobj(player->mo);
			player->mo = NULL;
		}
		player->spectator = false;
		player->pflags &= ~PF_WANTSTOJOIN;
		player->kartstuff[k_spectatewait] = 0;
		player->ctfteam = changeto;
		player->playerstate = PST_REBORN;

		//Reset away view
		if (P_IsLocalPlayer(player) && displayplayers[0] != consoleplayer)
		{
			// Call ViewpointSwitch hooks here.
			// The viewpoint was forcibly changed.
			LUAh_ViewpointSwitch(player, &players[consoleplayer], true);
			displayplayers[0] = consoleplayer;
		}

		if (changeto == 1)
			CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[player-players], '\x85', M_GetText("Red team"), '\x80');
		else if (changeto == 2)
			CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[player-players], '\x84', M_GetText("Blue team"), '\x80');

		return true; // no more player->mo, cannot continue.
	}
	// Joining in game from firing.
	else
	{
		if (player->mo)
		{
			P_RemoveMobj(player->mo);
			player->mo = NULL;
		}
		player->spectator = false;
		player->pflags &= ~PF_WANTSTOJOIN;
		player->kartstuff[k_spectatewait] = 0;
		player->playerstate = PST_REBORN;

		//Reset away view
		if (P_IsLocalPlayer(player) && displayplayers[0] != consoleplayer)
			displayplayers[0] = consoleplayer;

		HU_AddChatText(va(M_GetText("\x82*%s entered the game."), player_names[player-players]), false);
		return true; // no more player->mo, cannot continue.
	}
	return false;
}

// the below is first person only, if you're curious. check out P_CalcChasePostImg in p_mobj.c for chasecam
static void P_CalcPostImg(player_t *player)
{
	sector_t *sector = player->mo->subsector->sector;
	postimg_t *type = NULL;
	INT32 *param;
	fixed_t pviewheight;
	UINT8 i;

	if (player->mo->eflags & MFE_VERTICALFLIP)
		pviewheight = player->mo->z + player->mo->height - player->viewheight;
	else
		pviewheight = player->mo->z + player->viewheight;

	if (player->awayviewtics && player->awayviewmobj && !P_MobjWasRemoved(player->awayviewmobj))
	{
		sector = player->awayviewmobj->subsector->sector;
		pviewheight = player->awayviewmobj->z + 20*FRACUNIT;
	}

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (player == &players[displayplayers[i]])
		{
			type = &postimgtype[i];
			param = &postimgparam[i];
			break;
		}
	}

	// see if we are in heat (no, not THAT kind of heat...)

	if (P_FindSpecialLineFromTag(13, sector->tag, -1) != -1)
		*type = postimg_heat;
	else if (sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t topheight;
		fixed_t bottomheight;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);

			if (pviewheight >= topheight || pviewheight <= bottomheight)
				continue;

			if (P_FindSpecialLineFromTag(13, rover->master->frontsector->tag, -1) != -1)
				*type = postimg_heat;
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
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE) || rover->flags & FF_BLOCKPLAYER)
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
	if (player->pflags & PF_GAMETYPEOVER)
	{
		// NO! Don't do this!
		return;
	}

	if (P_IsLocalPlayer(player) && !demo.playback)
	{
		legitimateexit = true; // SRB2kart: losing a race is still seeing it through to the end :p
	}

	if (netgame && !player->bot)
	{
		CON_LogMessage(va(M_GetText("%s ran out of time.\n"), player_names[player-players]));
	}

	player->pflags |= PF_GAMETYPEOVER;

	if (G_GametypeUsesLives())
	{
		K_PlayerLoseLife(player);
	}

	if (player->mo)
	{
		S_StopSound(player->mo);
		P_DamageMobj(player->mo, NULL, NULL, 1, DMG_INSTAKILL);
	}

	P_EndingMusic(player);

	if (!exitcountdown)
		exitcountdown = 5*TICRATE;
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

/* 	set follower state with our weird hacks
	the reason we do this is to avoid followers ever using actions (majormods, yikes!)
	without having to touch p_mobj.c.
	so we give it 1more tic and change the state when tic == 1 instead of 0
	cool beans?
	cool beans.
*/
static void P_SetFollowerState(mobj_t *f, INT32 state)
{

	if (!f || P_MobjWasRemoved(f))
		return;		// safety net

	// No, do NOT set the follower to S_NULL. Set it to S_INVISIBLE.
	if (state == S_NULL)
	{
		state = S_INVISIBLE;
		f->threshold = 1;	// Threshold = 1 means stop doing anything related to setting states, so that we don't get out of S_INVISIBLE
	}

	// extravalue2 stores the last "first state" we used.
	// because states default to idlestates, if we use an animation that uses an "ongoing" state line, don't reset it!
	// this prevents it from looking very dumb
	if (state == f->extravalue2)
		return;

	// we will save the state into extravalue2.
	f->extravalue2 = state;

	P_SetMobjStateNF(f, state);
	if (f->state->tics > 0)
		f->tics++;
}

//
//P_HandleFollower
//
//Handle the follower's spawning and moving along with the player. Do note that some of the stuff like the removal if a player doesn't exist anymore is handled in MT_FOLLOWER's thinker.
static void P_HandleFollower(player_t *player)
{
	follower_t fl;
	angle_t an;
	fixed_t zoffs;
	fixed_t sx, sy, sz;
	UINT16 color;

	fixed_t bubble;	// bubble scale (0 if no bubble)
	mobj_t *bmobj;	// temp bubble mobj


	if (!player->followerready)
		return;	// we aren't ready to perform anything follower related yet.

	// How about making sure our follower exists and is added before trying to spawn it n' all?
	if (player->followerskin > numfollowers-1 || player->followerskin < -1)
	{
		//CONS_Printf("Follower skin invlaid. Setting to -1.\n");
		player->followerskin = -1;
		return;
	}

	// don't do anything if we can't have a follower to begin with. (It gets removed under those conditions)
	if (player->spectator)
		return;
	if (player->followerskin < 0)
		return;
	// Before we do anything, let's be sure of where we're supposed to be
	fl = followers[player->followerskin];

	an = player->mo->angle + (fl.atangle)*ANG1;		// it's aproximative but it really doesn't matter in the grand scheme of things...
	zoffs = (fl.zoffs)*FRACUNIT;
	bubble = fl.bubblescale;	// 0 if no bubble to spawn.

	// do you like angle maths? I certainly don't...
	sx = player->mo->x + FixedMul((player->mo->scale*fl.dist), FINECOSINE((an)>>ANGLETOFINESHIFT));
	sy = player->mo->y + FixedMul((player->mo->scale*fl.dist), FINESINE((an)>>ANGLETOFINESHIFT));

	// for the z coordinate, don't be a doof like Steel and forget that MFE_VERTICALFLIP exists :P
	sz = player->mo->z + FixedMul(player->mo->scale, zoffs)*P_MobjFlip(player->mo);
	if (player->mo->eflags & MFE_VERTICALFLIP)
		sz += fl.height*player->mo->scale;

	// finally, add a cool floating effect to the z height.
	// not stolen from k_kart I swear!!
	{
		const fixed_t pi = (22<<FRACBITS) / 7; // loose approximation, this doesn't need to be incredibly precise
		fixed_t sine = fl.bobamp * FINESINE((((8*pi*(fl.bobspeed)) * leveltime)>>ANGLETOFINESHIFT) & FINEMASK);
		sz += FixedMul(player->mo->scale, sine)*P_MobjFlip(player->mo);
	}

	// Set follower colour

	switch (player->followercolor)
	{
		case MAXSKINCOLORS: // "Match"
			color = player->skincolor;
			break;
		case MAXSKINCOLORS+1: // "Opposite"
			color = skincolors[player->skincolor].invcolor;
			break;
		default:

			color = player->followercolor;
			if (!color || color > MAXSKINCOLORS+2) // Make sure this isn't garbage
				color = player->skincolor; // "Match" as fallback.

			break;
	}



	if (!player->follower)	// follower doesn't exist / isn't valid
	{
		//CONS_Printf("Spawning follower...\n");
		// so let's spawn one!
		P_SetTarget(&player->follower, P_SpawnMobj(sx, sy, sz, MT_FOLLOWER));
		P_SetFollowerState(player->follower, fl.idlestate);
		P_SetTarget(&player->follower->target, player->mo);	// we need that to know when we need to disappear
		player->follower->angle = player->mo->angle;

		// This is safe to only spawn it here, the follower is removed then respawned when switched.
		if (bubble)
		{
			bmobj = P_SpawnMobj(player->follower->x, player->follower->y, player->follower->z, MT_FOLLOWERBUBBLE_FRONT);
			P_SetTarget(&player->follower->hnext, bmobj);
			P_SetTarget(&bmobj->target, player->follower);	// Used to know if we have to despawn at some point.

			bmobj = P_SpawnMobj(player->follower->x, player->follower->y, player->follower->z, MT_FOLLOWERBUBBLE_BACK);
			P_SetTarget(&player->follower->hnext->hnext, bmobj);	// this seems absolutely stupid, I know, but this will make updating the momentums/flags of these a bit easier.
			P_SetTarget(&bmobj->target, player->follower);	// Ditto
		}

		player->follower->extravalue1 = 0;	// extravalue1 is used to know what "state set" to use.
		/*
			0 = idle
			1 = forwards
			2 = hurt
			3 = win
			4 = lose
			5 = hitconfirm (< this one uses ->movecount as timer to know when to end, and goes back to normal states afterwards, unless hurt)
		*/
	}
	else	// follower exists, woo!
	{

		// Safety net (2)

		if (P_MobjWasRemoved(player->follower))
		{
			P_SetTarget(&player->follower, NULL);	// Remove this and respawn one, don't crash the game if Lua decides to P_RemoveMobj this thing.
			return;
		}

		// first of all, handle states following the same model as above:
		if (player->follower->tics == 1)
			P_SetFollowerState(player->follower, player->follower->state->nextstate);

		// move the follower next to us (yes, this is really basic maths but it looks pretty damn clean in practice)!
		player->follower->momx = (sx - player->follower->x)/fl.horzlag;
		player->follower->momy = (sy - player->follower->y)/fl.horzlag;
		player->follower->momz = (sz - player->follower->z)/fl.vertlag;
		player->follower->angle = player->mo->angle;

		if (player->mo->colorized)
			player->follower->color = player->mo->color;
		else
			player->follower->color = color;

		player->follower->colorized = player->mo->colorized;

		P_SetScale(player->follower, FixedMul(fl.scale, player->mo->scale));
		K_GenericExtraFlagsNoZAdjust(player->follower, player->mo);	// Not K_MatchGenericExtraFlag because the Z adjust it has only works properly if master & mo have the same Z height.

		// Match how the player is being drawn
		player->follower->drawflags = player->mo->drawflags;

		// Make the follower invisible if we no contest'd rather than removing it. No one will notice the diff seriously.
		// Also make the follower invisible if we choose not to have it displayed because it isn't ours. (also quick hacky check for f12)
		if (player->pflags & PF_GAMETYPEOVER || (!cv_showfollowers.value && (!P_IsDisplayPlayer(player) || displayplayers[0] != consoleplayer) ))
			player->follower->drawflags |= MFD_DONTDRAW;

		if (player->speed && (player->follower->momx || player->follower->momy))
			player->follower->angle = R_PointToAngle2(0, 0, player->follower->momx, player->follower->momy);
			// if we're moving let's make the angle the direction we're moving towards. This is to avoid drifting / reverse looking awkward.
			// Make sure the follower itself is also moving however, otherwise we'll be facing angle 0

		// Finally, if the follower has bubbles, move them, set their scale, etc....
		// This is what I meant earlier by it being easier, now we can just use this weird lil loop to get the job done!

		bmobj = player->follower->hnext;	// will be NULL if there's no bubble

		while (bmobj && !P_MobjWasRemoved(bmobj))
		{
			// match follower's momentums and (e)flags(2).
			bmobj->momx = player->follower->momx;
			bmobj->momy = player->follower->momy;
			bmobj->momz = player->follower->momz;

			P_SetScale(bmobj, FixedMul(bubble, player->mo->scale));
			K_GenericExtraFlagsNoZAdjust(bmobj, player->follower);
			bmobj->drawflags = player->mo->drawflags;

			if (player->follower->threshold)	// threshold means the follower was "despawned" with S_NULL (is actually just set to S_INVISIBLE)
				P_SetMobjState(bmobj, S_INVISIBLE);	// sooooo... let's do the same!

			bmobj = bmobj->hnext;	// switch to other bubble layer or exit
		}


		if (player->follower->threshold)
			return;	// Threshold means the follower was "despanwed" with S_NULL.

		// However with how the code is factored, this is just a special case of S_INVISBLE to avoid having to add other player variables.


		// handle follower animations. Could probably be better...
		// hurt or dead
		if (player->kartstuff[k_spinouttimer] || player->mo->state == &states[S_KART_SPINOUT] || player->mo->health <= 0)
		{
			player->follower->movecount = 0;	// cancel hit confirm.
			player->follower->angle = player->drawangle;	// spin out
			if (player->follower->extravalue1 != 2)
			{
				player->follower->extravalue1 = 2;
				P_SetFollowerState(player->follower, fl.hurtstate);
			}
			if (player->mo->health <= 0)	// if dead, follow the player's z momentum exactly so they both look like they die at the same speed.
				player->follower->momz = player->mo->momz;
		}
		else if (player->follower->movecount)
		{
			if (player->follower->extravalue1 != 5)
			{
				player->follower->extravalue1 = 5;
				P_SetFollowerState(player->follower, fl.hitconfirmstate);
			}
			player->follower->movecount--;
		}
		else if (player->speed > 10*player->mo->scale)	// animation for moving fast enough
		{

			if (player->follower->extravalue1 != 1)
			{
				player->follower->extravalue1 = 1;
				P_SetFollowerState(player->follower, fl.followstate);
			}
		}
		else	// animations when nearly still. This includes winning and losing.
		{
			if (player->follower->extravalue1 != 0)
			{

				if (player->exiting)	// win/ loss animations
				{
					if (K_IsPlayerLosing(player))	// L
					{
						if (player->follower->extravalue1 != 4)
						{
							player->follower->extravalue1 = 4;
							P_SetFollowerState(player->follower, fl.losestate);
						}
					}
					else	// W
					{
						if (player->follower->extravalue1 != 3)
						{
							player->follower->extravalue1 = 3;
							P_SetFollowerState(player->follower, fl.winstate);
						}
					}
				}
				else	// normal standstill
				{
					player->follower->extravalue1 = 0;
					P_SetFollowerState(player->follower, fl.idlestate);
				}
			}
		}
	}
}

	/* gaysed script from me, based on Golden's sprite slope roll */

// holy SHIT
static INT32
Quaketilt (player_t *player)
{
	angle_t tilt;
	fixed_t lowb; // this threshold for speed
	angle_t moma = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
	angle_t delta = (INT32)( player->mo->angle - moma );
	fixed_t speed;
	// Hi! I'm "not a math guy"!
	if (abs(delta) > ANGLE_90)
		delta = (INT32)(( moma + ANGLE_180 ) - player->mo->angle );
	if (P_IsObjectOnGround(player->mo))
	{
		tilt = 4*ANGLE_11hh/5;
		lowb = 15*FRACUNIT;
	}
	else
	{
		tilt = ANGLE_22h;
		lowb = 10*FRACUNIT;
	}
	moma = FixedMul(FixedDiv(delta, ANGLE_90), tilt);
	speed = abs( player->mo->momx + player->mo->momy );
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

	if (cv_slopeview.value && player->mo->standingslope)
	{
		delta = ( player->mo->angle - player->mo->standingslope->xydirection );
		slope = FixedMul(FINESINE (delta>>ANGLETOFINESHIFT),
				player->mo->standingslope->zangle);
		/* invert the screen tilt as if tilting the view instead */
		if (cv_slopeview.value != 2)/* :james: mode */
			slope = -slope;
	}
	else
		slope = 0;

	if (cv_quaketilt.value)
		slope += Quaketilt(player);

	delta = (INT32)( slope - player->viewrollangle )/ 16;

	if (delta)
		player->viewrollangle += delta;
	else
		player->viewrollangle  = slope;
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

#ifdef SEENAMES
	if (netgame && player == &players[displayplayers[0]] && !(leveltime % (TICRATE/5)) && !r_splitscreen)
	{
		seenplayer = NULL;

		if (cv_seenames.value && cv_allowseenames.value)
		{
			mobj_t *mo = P_SpawnNameFinder(player->mo, MT_NAMECHECK);

			if (mo)
			{
				short int i;
				mo->flags |= MF_NOCLIPHEIGHT;
				for (i = 0; i < 32; i++)
				{
					// Debug drawing
//					if (i&1)
//						P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);
					if (P_RailThinker(mo))
						break; // mobj was removed (missile hit a wall) or couldn't move
				}
			}
		}
	}
#endif

	if (player->awayviewmobj && P_MobjWasRemoved(player->awayviewmobj))
	{
		P_SetTarget(&player->awayviewmobj, NULL); // remove awayviewmobj asap if invalid
		player->awayviewtics = 0; // reset to zero
	}

	// Run followes here. We need them to run even when we're dead to follow through what we're doing.
	P_HandleFollower(player);

	if (player->flashcount)
		player->flashcount--;

	if (player->awayviewtics && player->awayviewtics != -1)
		player->awayviewtics--;

	// Track airtime
	if (P_IsObjectOnGround(player->mo))
	{
		player->airtime = 0;
	}
	else
	{
		player->airtime++;
	}

	cmd = &player->cmd;

	{
		angle_t angleChange = K_GetKartTurnValue(player, cmd->turning) << TICCMD_REDUCE;
		player->angleturn += angleChange;
		P_SetLocalAngle(player, P_GetLocalAngle(player) + angleChange);
	}

	// SRB2kart
	// Save the dir the player is holding
	//  to allow items to be thrown forward or backward.
	if (cmd->buttons & BT_FORWARD)
	{
		player->kartstuff[k_throwdir] = 1;
	}
	else if (cmd->buttons & BT_BACKWARD)
	{
		player->kartstuff[k_throwdir] = -1;
	}
	else
	{
		player->kartstuff[k_throwdir] = 0;
	}

#ifdef PARANOIA
	if (player->playerstate == PST_REBORN)
		I_Error("player %s is in PST_REBORN\n", sizeu1(playeri));
#endif

	if (!mapreset)
	{
		if (gametyperules & GTR_CIRCUIT)
		{
			INT32 i;

			// Check if all the players in the race have finished. If so, end the level.
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && !players[i].spectator)
				{
					if (!players[i].exiting && !(players[i].pflags & PF_GAMETYPEOVER) && players[i].lives > 0)
						break;
				}
			}

			if (i == MAXPLAYERS && player->exiting == raceexittime+2) // finished
				player->exiting = raceexittime+1;

#if 0
			// If 10 seconds are left on the timer,
			// begin the drown music for countdown!
			// SRB2Kart: despite how perfect this is, it's disabled FOR A REASON
			if (racecountdown == 11*TICRATE - 1)
			{
				if (P_IsLocalPlayer(player))
					S_ChangeMusicInternal("drown", false);
			}
#endif

			// If you've hit the countdown and you haven't made
			//  it to the exit, you're a goner!
			if (racecountdown == 1 && !player->spectator && !player->exiting && !(player->pflags & PF_GAMETYPEOVER) && player->lives > 0)
			{
				P_DoTimeOver(player);

				if (player->playerstate == PST_DEAD)
				{
					return;
				}
			}
		}

		// If it is set, start subtracting
		// Don't allow it to go back to 0
		if (player->exiting > 1 && (player->exiting < raceexittime+2 || !(gametyperules & GTR_CIRCUIT))) // SRB2kart - "&& player->exiting > 1"
			player->exiting--;

		if (player->exiting && exitcountdown)
			player->exiting = 99; // SRB2kart

		if (player->exiting == 2 || exitcountdown == 2)
		{
			if (server)
			{
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
			}
		}
	}

	// check water content, set stuff in mobj
	P_MobjCheckWater(player->mo);

#ifndef SECTORSPECIALSAFTERTHINK
	if (player->onconveyor != 1 || !P_IsObjectOnGround(player->mo))
		player->onconveyor = 0;
	// check special sectors : damage & secrets

	if (!player->spectator)
		P_PlayerInSpecialSector(player);
	else if (
#else
	if (player->spectator &&
#endif
		(gametyperules & GTR_LIVES))
	{
		/*P_ConsiderAllGone()*/;
	}

	if (player->playerstate == PST_DEAD)
	{
		if (player->spectator)
			player->mo->drawflags |= MFD_SHADOW;
		else
			player->mo->drawflags &= ~(MFD_TRANSMASK|MFD_BRIGHTMASK);
		P_DeathThink(player);
		LUAh_PlayerThink(player);
		return;
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
		player->powers[pw_nocontrol] = 2;
	}

	// Synchronizes the "real" amount of time spent in the level.
	if (!(exitcountdown && !racecountdown) && !(player->exiting || mapreset) && !(player->pflags & PF_GAMETYPEOVER) && !stoppedclock)
	{
		if (leveltime >= starttime)
		{
			player->realtime = leveltime - starttime;
			if (player == &players[consoleplayer])
			{
				if (player->spectator || !circuitmap)
					curlap = 0;
				else
					curlap++; // This is too complicated to sync to realtime, just sorta hope for the best :V
			}
		}
		else
		{
			player->realtime = 0;
			if (player == &players[consoleplayer])
				curlap = 0;
		}
	}

	if ((netgame || multiplayer) && player->spectator && cmd->buttons & BT_ATTACK && !player->powers[pw_flashing])
	{
		player->pflags ^= PF_WANTSTOJOIN;
		player->powers[pw_flashing] = TICRATE/2 + 1;
		/*if (P_SpectatorJoinGame(player))
			return; // player->mo was removed.*/
	}

	if (player->linktimer && !player->powers[pw_nights_linkfreeze])
	{
		if (--player->linktimer <= 0) // Link timer
			player->linkcount = 0;
	}

	if (player->respawn.state != RESPAWNST_NONE)
	{
		K_RespawnChecker(player);
		player->rmomx = player->rmomy = 0;

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
	else if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT)
	{
		P_DoZoomTube(player);
		player->rmomx = player->rmomy = 0;
		P_ResetScore(player);
	}
	else
	{
		// Move around.
		P_MovePlayer(player);
	}

	player->mo->movefactor = FRACUNIT; // We're not going to do any more with this, so let's change it back for the next frame.

	// Unset statis flags after moving.
	// In other words, if you manually set stasis via code,
	// it lasts for one tic.
	player->pflags &= ~PF_FULLSTASIS;

	if (player->onconveyor == 1)
		player->onconveyor = 3;
	else if (player->onconveyor == 3)
		player->cmomy = player->cmomx = 0;

	P_DoBubbleBreath(player); // Spawn Sonic's bubbles
	P_CheckInvincibilityTimer(player); // Spawn Invincibility Sparkles

#if 1
	// "Blur" a bit when you have speed shoes and are going fast enough
	if ((player->powers[pw_super] || player->powers[pw_sneakers])
		&& (player->speed + abs(player->mo->momz)) > FixedMul(20*FRACUNIT,player->mo->scale))
	{
		UINT8 i;
		mobj_t *gmobj = P_SpawnGhostMobj(player->mo);

		gmobj->fuse = 2;
		if (gmobj->tracer)
			gmobj->tracer->fuse = 2;
		if (leveltime & 1)
		{
			gmobj->frame &= ~FF_TRANSMASK;
			gmobj->frame |= tr_trans70<<FF_TRANSSHIFT;
			if (gmobj->tracer)
			{
				gmobj->tracer->frame &= ~FF_TRANSMASK;
				gmobj->tracer->frame |= tr_trans70<<FF_TRANSSHIFT;
			}
		}

		// Hide the mobj from our sights if we're the displayplayer and chasecam is off.
		// Why not just not spawn the mobj?  Well, I'd rather only flirt with
		// consistency so much...
		for (i = 0; i <= r_splitscreen; i++)
		{
			if (player == &players[displayplayers[i]] && !camera[i].chase)
			{
				gmobj->drawflags |= MFD_DONTDRAW;
				break;
			}
		}
	}
#endif

	// check for use
	if (cmd->buttons & BT_BRAKE)
		player->pflags |= PF_USEDOWN;
	else
		player->pflags &= ~PF_USEDOWN;

	// IF PLAYER NOT HERE THEN FLASH END IF
	if (player->quittime && player->powers[pw_flashing] < flashingtics - 1 && !player->gotflag)
		player->powers[pw_flashing] = flashingtics - 1;

	// Counters, time dependent power ups.
	// Time Bonus & Ring Bonus count settings

	// Strength counts up to diminish fade.
	if (player->powers[pw_flashing] && player->powers[pw_flashing] < UINT16_MAX &&
		(player->spectator || !P_PlayerInPain(player)))
		player->powers[pw_flashing]--;

	if (player->powers[pw_nocontrol] & ((1<<15)-1) && player->powers[pw_nocontrol] < UINT16_MAX)
	{
		if (!(--player->powers[pw_nocontrol]))
			player->pflags &= ~PF_WPNDOWN;
	}
	else
		player->powers[pw_nocontrol] = 0;

	if (player->powers[pw_ignorelatch] & ((1<<15)-1) && player->powers[pw_ignorelatch] < UINT16_MAX)
		player->powers[pw_ignorelatch]--;
	else
		player->powers[pw_ignorelatch] = 0;

	//pw_super acts as a timer now
	if (player->powers[pw_super])
		player->powers[pw_super]++;

	// Flash player after being hit.
	if (!(player->kartstuff[k_hyudorotimer] // SRB2kart - fixes Hyudoro not flashing when it should.
		|| player->kartstuff[k_growshrinktimer] > 0 // Grow doesn't flash either.
		|| (player->respawn.state != RESPAWNST_NONE) // Respawn timer (for drop dash effect)
		|| (player->pflags & PF_GAMETYPEOVER) // NO CONTEST explosion
		|| ((gametyperules & GTR_BUMPERS) && player->kartstuff[k_bumper] <= 0 && player->kartstuff[k_comebacktimer])
		|| leveltime < starttime)) // Level intro
	{
		if (player->powers[pw_flashing] > 0 && player->powers[pw_flashing] < K_GetKartFlashing(player)
			&& (leveltime & 1))
			player->mo->drawflags |= MFD_DONTDRAW;
		else
			player->mo->drawflags &= ~MFD_DONTDRAW;
	}

	player->pflags &= ~PF_SLIDING;

	K_KartPlayerThink(player, cmd); // SRB2kart

	DoABarrelRoll(player);

	LUAh_PlayerThink(player);
}

//
// P_PlayerAfterThink
//
// Thinker for player after all other thinkers have run
//
void P_PlayerAfterThink(player_t *player)
{
	camera_t *thiscam = NULL; // if not one of the displayed players, just don't bother
	UINT8 i;

#ifdef PARANOIA
	if (!player->mo)
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("P_PlayerAfterThink: players[%s].mo == NULL", sizeu1(playeri));
	}
#endif

#ifdef SECTORSPECIALSAFTERTHINK
	if (player->onconveyor != 1 || !P_IsObjectOnGround(player->mo))
		player->onconveyor = 0;
	// check special sectors : damage & secrets

	if (!player->spectator)
		P_PlayerInSpecialSector(player);
#endif

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (player == &players[displayplayers[i]])
		{
			thiscam = &camera[i];
			break;
		}
	}

	if (player->playerstate == PST_DEAD)
	{
		if (player->followmobj)
		{
			P_RemoveMobj(player->followmobj);
			P_SetTarget(&player->followmobj, NULL);
		}
		return;
	}

	if (player->pflags & PF_SLIDING)
		P_SetPlayerMobjState(player->mo, player->mo->info->painstate);

	if (thiscam)
	{
		if (!thiscam->chase) // bob view only if looking through the player's eyes
		{
			P_CalcHeight(player);
			P_CalcPostImg(player);
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
	}

	// spectator invisibility and nogravity.
	if ((netgame || multiplayer) && player->spectator)
	{
		player->mo->drawflags |= MFD_DONTDRAW;
		player->mo->flags |= MF_NOGRAVITY;
	}

	if (P_IsObjectOnGround(player->mo))
		player->mo->pmomz = 0;

	K_KartPlayerAfterThink(player);

	if (player->powers[pw_dye])
	{
		player->mo->colorized = true;
		player->mo->color = player->powers[pw_dye];
	}

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
			if (LUAh_FollowMobj(player, player->followmobj) || P_MobjWasRemoved(player->followmobj))
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
}

void P_SetPlayerAngle(player_t *player, angle_t angle)
{
	angle_t delta = angle - player->angleturn;

	P_ForceLocalAngle(player, P_GetLocalAngle(player) + delta);
	player->angleturn += delta;
}

void P_SetLocalAngle(player_t *player, angle_t angle)
{
	angle_t delta = (angle - P_GetLocalAngle(player));

	P_ForceLocalAngle(player, P_GetLocalAngle(player) + delta);
}

angle_t P_GetLocalAngle(player_t *player)
{
	// this function is from vanilla srb2. can you tell?
	// (hint: they have separate variables for all of this shit instead of arrays)
	UINT8 i;

	for (i = 0; i <= splitscreen; i++)
	{
		if (player == &players[g_localplayers[i]])
			return localangle[i];
	}

	return 0;
}

void P_ForceLocalAngle(player_t *player, angle_t angle)
{
	UINT8 i;

	angle = angle & ~UINT16_MAX;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		if (player == &players[g_localplayers[i]])
		{
			localangle[i] = angle;
			break;
		}
	}
}

boolean P_PlayerFullbright(player_t *player)
{
	return (player->kartstuff[k_invincibilitytimer] > 0);
}
