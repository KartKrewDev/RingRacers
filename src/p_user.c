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
#include "k_bot.h"
// Objectplace
#include "m_cheat.h"
// Thok camera snap (ctrl-f "chalupa")
#include "g_input.h"

// SRB2kart
#include "m_cond.h" // M_UpdateUnlockablesAndExtraEmblems
#include "k_kart.h"
#include "console.h" // CON_LogMessage

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

	if (!(twodlevel || (mo->flags2 & MF2_TWOD)))
		mo->momy += FixedMul(move, FINESINE(angle));
}

#if 0
static inline void P_ThrustEvenIn2D(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx += FixedMul(move, FINECOSINE(angle));
	mo->momy += FixedMul(move, FINESINE(angle));
}

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

	if (!(twodlevel || (mo->flags2 & MF2_TWOD)))
		mo->momy = FixedMul(move,FINESINE(angle));
}

void P_InstaThrustEvenIn2D(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx = FixedMul(move, FINECOSINE(angle));
	mo->momy = FixedMul(move, FINESINE(angle));
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
	pviewheight = FixedMul(41*player->height/48, mo->scale); // default eye view height

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
			if (players[i].powers[pw_carry] != CR_NIGHTSMODE)
				players[i].powers[pw_carry] = CR_NONE;

			P_SetTarget(&players[i].mo->tracer, emmo);

			if (pnum == 255)
			{
				pnum = i;
				continue;
			}

			if (i == pnum)
				continue;

			emmo->flags2 |= MF2_DONTDRAW;
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

	if (gametyperules & GTR_RACE)
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

	if (!G_RaceGametype())
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
	// no silly, sliding isn't pain
	if (!(player->pflags & PF_SLIDING) && player->mo->state == &states[player->mo->info->painstate] && player->powers[pw_flashing])
		return true;

	if (player->mo->state == &states[S_PLAY_STUN])
		return true;

	return false;
}

//
// P_ResetPlayer
//
// Useful when you want to kill everything the player is doing.
void P_ResetPlayer(player_t *player)
{
	player->pflags &= ~(PF_SPINNING|PF_STARTDASH|PF_STARTJUMP|PF_JUMPED|PF_NOJUMPDAMAGE|PF_GLIDING|PF_THOKKED|PF_CANCARRY|PF_SHIELDABILITY|PF_BOUNCING);

	if (player->powers[pw_carry] == CR_ROLLOUT)
	{
		if (player->mo->tracer && !P_MobjWasRemoved(player->mo->tracer))
		{
			player->mo->tracer->flags |= MF_PUSHABLE;
			P_SetTarget(&player->mo->tracer->tracer, NULL);
		}
		P_SetTarget(&player->mo->tracer, NULL);
		player->powers[pw_carry] = CR_NONE;
	}

	if (!(player->powers[pw_carry] == CR_NIGHTSMODE || player->powers[pw_carry] == CR_NIGHTSFALL || player->powers[pw_carry] == CR_BRAKGOOP || player->powers[pw_carry] == CR_MINECART))
		player->powers[pw_carry] = CR_NONE;

	player->secondjump = 0;
	player->glidetime = 0;
	player->homing = 0;
	player->climbing = 0;
	player->powers[pw_tailsfly] = 0;
	player->onconveyor = 0;
	player->skidtime = 0;
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

	if (G_BattleGametype()) // No rings in Battle Mode
		return;

	player->kartstuff[k_rings] += num_rings;
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
	UINT8 prevlives = player->lives;
	if (!player)
		return;

	if (player->bot)
		player = &players[consoleplayer];

	if (gamestate == GS_LEVEL)
	{
		if (player->lives == INFLIVES || !(gametyperules & GTR_LIVES))
		{
			P_GivePlayerRings(player, 100*numlives);
			return;
		}

		if ((netgame || multiplayer) && G_GametypeUsesCoopLives() && cv_cooplives.value == 0)
		{
			P_GivePlayerRings(player, 100*numlives);
			if (player->lives - prevlives >= numlives)
				goto docooprespawn;

			numlives = (numlives + prevlives - player->lives);
		}
	}
	else if (player->lives == INFLIVES)
		return;

	player->lives += numlives;

	if (player->lives > 99)
		player->lives = 99;
	else if (player->lives < 1)
		player->lives = 1;

docooprespawn:
	if (cv_coopstarposts.value)
		return;
	if (prevlives > 0)
		return;
	if (!player->spectator)
		return;
	P_SpectatorJoinGame(player);
}

// Adds to the player's score
void P_AddPlayerScore(player_t *player, UINT32 amount)
{
	//UINT32 oldscore;

	if (!(G_BattleGametype()))
		return;

	if (player->exiting) // srb2kart
		return;

	//oldscore = player->score;

	// Don't go above MAXSCORE.
	if (player->marescore + amount < MAXSCORE)
		player->marescore += amount;
	else
		player->marescore = MAXSCORE;

	// In team match, all awarded points are incremented to the team's running score.
	if (gametype == GT_TEAMMATCH)
	{
		if (player->ctfteam == 1)
			redscore += amount;
		else if (player->ctfteam == 2)
			bluescore += amount;
	}
}

//
// P_PlayLivesJingle
//
void P_PlayLivesJingle(player_t *player)
{
	if (player && !P_IsLocalPlayer(player))
		return;

	if (use1upSound || cv_1upsound.value)
		S_StartSound(NULL, sfx_oneup);
	else if (mariomode)
		S_StartSound(NULL, sfx_marioa);
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
				result = (players[i].powers[pw_super] && !(mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC));
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
	else if (source->player && source->player->kartstuff[k_rings] <= 0)
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
		if (!((players[displayplayers[0]].exiting || (players[displayplayers[0]].pflags & PF_TIMEOVER))
			|| (players[displayplayers[1]].exiting || (players[displayplayers[1]].pflags & PF_TIMEOVER))
			|| ((r_splitscreen < 2) && (players[displayplayers[2]].exiting || (players[displayplayers[2]].pflags & PF_TIMEOVER)))
			|| ((r_splitscreen < 3) && (players[displayplayers[3]].exiting || (players[displayplayers[3]].pflags & PF_TIMEOVER)))))
			return false;

		bestlocalplayer = &players[displayplayers[0]];
		bestlocalpos = ((players[displayplayers[0]].pflags & PF_TIMEOVER) ? MAXPLAYERS+1 : players[displayplayers[0]].kartstuff[k_position]);
#define setbests(p) \
	if (((players[p].pflags & PF_TIMEOVER) ? MAXPLAYERS+1 : players[p].kartstuff[k_position]) < bestlocalpos) \
	{ \
		bestlocalplayer = &players[p]; \
		bestlocalpos = ((players[p].pflags & PF_TIMEOVER) ? MAXPLAYERS+1 : players[p].kartstuff[k_position]); \
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
		if (!(player->exiting || (player->pflags & PF_TIMEOVER)))
			return false;

		bestlocalplayer = player;
		bestlocalpos = ((player->pflags & PF_TIMEOVER) ? MAXPLAYERS+1 : player->kartstuff[k_position]);
	}

	if (G_RaceGametype() && bestlocalpos == MAXPLAYERS+1)
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

	if (G_RaceGametype())
		buffer[1] = 'r';
	else if (G_BattleGametype())
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
			if (G_RaceGametype() && player->laps >= (UINT8)(cv_numlaps.value))
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

<<<<<<< HEAD
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
=======
	// Point penalty for hitting a hazard during tag.
	// Discourages players from intentionally hurting themselves to avoid being tagged.
	if (((gametyperules & (GTR_TAG|GTR_HIDEFROZEN)) == GTR_TAG)
	&& (!(player->pflags & PF_GAMETYPEOVER) && !(player->pflags & PF_TAGIT)))
	{
		if (player->score >= 50)
			player->score -= 50;
		else
			player->score = 0;
	}
>>>>>>> srb2/next

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

	if (r_splitscreen > splitscreen)
	{
		for (i = 0; i <= r_splitscreen; ++i)
		{
			if (player == &players[displayplayers[i]])
				return true;
		}
	}
	else if (player == &players[consoleplayer])
		return true;
	else if (splitscreen)
	{
		for (i = 1; i <= splitscreen; i++) // Skip P1
		{
			if (player == &players[displayplayers[i]])
				return true;
		}
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

<<<<<<< HEAD
	for (i = 0; i <= r_splitscreen; i++) // DON'T skip P1
=======
	if (!G_CoopGametype())
>>>>>>> srb2/next
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

	if (LUAh_ShieldSpawn(player))
		return;

<<<<<<< HEAD
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
=======
			if (!ultimatemode && continuesInSession && G_IsSpecialStage(gamemap)
			&& player->marescore >= 50000 && oldscore < 50000)
			{
				player->continues += 1;
				player->gotcontinue = true;
				if (P_IsLocalPlayer(player))
					S_StartSound(NULL, sfx_s3kac);
			}
		}

		if (G_CoopGametype())
			return;
>>>>>>> srb2/next
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

<<<<<<< HEAD
	if (shieldobj->info->seestate)
=======
	// In team match, all awarded points are incremented to the team's running score.
	if ((gametyperules & (GTR_TEAMS|GTR_TEAMFLAGS)) == GTR_TEAMS)
>>>>>>> srb2/next
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
				shieldobj->flags2 |= MF2_SHADOW;
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

	// If you already have a bomb shield, use it!
	if ((shieldtype == SH_ARMAGEDDON) && (player->powers[pw_shield] & SH_NOSTACK) == SH_ARMAGEDDON)
		P_BlackOw(player);

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
<<<<<<< HEAD

		player->powers[pw_shield] = shieldtype|(player->powers[pw_shield] & SH_STACK);
		P_SpawnShieldOrb(player);

		if (shieldtype & SH_PROTECTWATER)
=======
	}
	if (stolen > 0)
	{
		// In team match, all stolen points are removed from the enemy team's running score.
		if ((gametyperules & (GTR_TEAMS|GTR_TEAMFLAGS)) == GTR_TEAMS)
>>>>>>> srb2/next
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
	ghost->frame &= ~FF_TRANSMASK;
	ghost->frame |= tr_trans50<<FF_TRANSSHIFT;
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
	if (player->exiting || mapreset)
		return;

	if (P_IsLocalPlayer(player) && (!player->spectator && !demo.playback))
		legitimateexit = true;

	if (G_RaceGametype()) // If in Race Mode, allow
	{
		player->exiting = raceexittime+2;
		K_KartUpdatePosition(player);

		if (cv_kartvoices.value)
		{
			if (P_IsDisplayPlayer(player))
			{
				sfxenum_t sfx_id;
				if (K_IsPlayerLosing(player))
					sfx_id = ((skin_t *)player->mo->skin)->soundsid[S_sfx[sfx_klose].skinsound];
				else
					sfx_id = ((skin_t *)player->mo->skin)->soundsid[S_sfx[sfx_kwin].skinsound];
				S_StartSound(NULL, sfx_id);
			}
			else
			{
				if (K_IsPlayerLosing(player))
					S_StartSound(player->mo, sfx_klose);
				else
					S_StartSound(player->mo, sfx_kwin);
			}
		}

		if (cv_inttime.value > 0)
			P_EndingMusic(player);

		// SRB2kart 120217
		//if (!exitcountdown)
			//exitcountdown = racecountdown + 8*TICRATE;

		if (P_CheckRacers())
			player->exiting = raceexittime+1;
	}
	else if (G_BattleGametype()) // Battle Mode exiting
	{
		player->exiting = battleexittime+1;
		P_EndingMusic(player);
	}
	else
		player->exiting = raceexittime+2; // Accidental death safeguard???

	//player->pflags &= ~PF_GLIDING;
	/*	// SRB2kart - don't need
	if (player->climbing)
	{
		player->climbing = 0;
		player->pflags |= PF_JUMPED;
		P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
	}
	*/
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

					// You can always break it if you have CA_GLIDEANDCLIMB
					// or if you are bouncing on it
					// or you are using CA_TWINSPIN/CA2_MELEE.
					if (player->charability == CA_GLIDEANDCLIMB
						|| (player->pflags & PF_BOUNCING)
						|| ((player->charability == CA_TWINSPIN) && (player->panim == PA_ABILITY))
						|| (player->charability2 == CA2_MELEE && player->panim == PA_ABILITY2))
						goto bust;

					if (rover->flags & FF_STRONGBUST)
						continue;

<<<<<<< HEAD
					// If it's not an FF_STRONGBUST, you can break if you are spinning (and not jumping)
					// or you are super
					// or you are in dashmode with SF_DASHMODE
					// or you are drilling in NiGHTS
					// or you are recording for Metal Sonic
					if (!((player->pflags & PF_SPINNING) && !(player->pflags & PF_JUMPED))
						&& !(player->powers[pw_super])
						&& !(((player->charflags & (SF_DASHMODE|SF_MACHINE)) == (SF_DASHMODE|SF_MACHINE)) && (player->dashmode >= DASHMODE_THRESHOLD))
						&& !(player->pflags & PF_DRILLING)
						&& !metalrecording)
						continue;
=======
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
	return ((splitscreen && player == &players[secondarydisplayplayer]) || player == &players[consoleplayer]);
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
				shieldobj->flags2 |= MF2_SHADOW;
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

	// If you already have a bomb shield, use it!
	if ((shieldtype == SH_ARMAGEDDON) && (player->powers[pw_shield] & SH_NOSTACK) == SH_ARMAGEDDON)
		P_BlackOw(player);

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
	ghost->colorized = mobj->colorized; // alternatively, "true" for sonic advance style colourisation

	ghost->angle = (mobj->player ? mobj->player->drawangle : mobj->angle);
	ghost->sprite = mobj->sprite;
	ghost->sprite2 = mobj->sprite2;
	ghost->frame = mobj->frame;
	ghost->tics = -1;
	ghost->frame &= ~FF_TRANSMASK;
	ghost->frame |= tr_trans50<<FF_TRANSSHIFT;
	ghost->fuse = ghost->info->damage;
	ghost->skin = mobj->skin;

	if (mobj->flags2 & MF2_OBJECTFLIP)
		ghost->flags |= MF2_OBJECTFLIP;

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
// P_SpawnThokMobj
//
// Spawns the appropriate thok object on the player
//
void P_SpawnThokMobj(player_t *player)
{
	mobj_t *mobj;
	mobjtype_t type = player->thokitem;
	fixed_t zheight;

	if (player->skincolor == 0)
		return;

	if (player->spectator)
		return;

	if (!type)
		return;

	if (type == MT_GHOST)
		mobj = P_SpawnGhostMobj(player->mo); // virtually does everything here for us
	else
	{
		if (player->mo->eflags & MFE_VERTICALFLIP)
			zheight = player->mo->z + player->mo->height + FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT) - FixedMul(mobjinfo[type].height, player->mo->scale);
		else
			zheight = player->mo->z - FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT);

		if (!(player->mo->eflags & MFE_VERTICALFLIP) && zheight < player->mo->floorz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->floorz;
		else if (player->mo->eflags & MFE_VERTICALFLIP && zheight + FixedMul(mobjinfo[type].height, player->mo->scale) > player->mo->ceilingz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->ceilingz - FixedMul(mobjinfo[type].height, player->mo->scale);

		mobj = P_SpawnMobj(player->mo->x, player->mo->y, zheight, type);

		// set to player's angle, just in case
		mobj->angle = player->drawangle;

		// color and skin
		mobj->color = player->mo->color;
		mobj->skin = player->mo->skin;

		// vertical flip
		if (player->mo->eflags & MFE_VERTICALFLIP)
			mobj->flags2 |= MF2_OBJECTFLIP;
		mobj->eflags |= (player->mo->eflags & MFE_VERTICALFLIP);

		// scale
		P_SetScale(mobj, (mobj->destscale = player->mo->scale));

		if (type == MT_THOK) // spintrail-specific modification for MT_THOK
		{
			mobj->frame = FF_TRANS70;
			mobj->fuse = mobj->tics;
		}
	}

	P_SetTarget(&mobj->target, player->mo); // the one thing P_SpawnGhostMobj doesn't do
	G_GhostAddThok();
}

//
// P_SpawnSpinMobj
//
// Spawns the appropriate spin object on the player
//
void P_SpawnSpinMobj(player_t *player, mobjtype_t type)
{
	mobj_t *mobj;
	fixed_t zheight;

	if (player->skincolor == 0)
		return;

	if (player->spectator)
		return;

	if (!type)
		return;

	if (type == MT_GHOST)
		mobj = P_SpawnGhostMobj(player->mo); // virtually does everything here for us
	else
	{
		if (player->mo->eflags & MFE_VERTICALFLIP)
			zheight = player->mo->z + player->mo->height + FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT) - FixedMul(mobjinfo[type].height, player->mo->scale);
		else
			zheight = player->mo->z - FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT);

		if (!(player->mo->eflags & MFE_VERTICALFLIP) && zheight < player->mo->floorz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->floorz;
		else if (player->mo->eflags & MFE_VERTICALFLIP && zheight + FixedMul(mobjinfo[type].height, player->mo->scale) > player->mo->ceilingz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->ceilingz - FixedMul(mobjinfo[type].height, player->mo->scale);

		mobj = P_SpawnMobj(player->mo->x, player->mo->y, zheight, type);

		// set to player's angle, just in case
		mobj->angle = player->drawangle;

		// color and skin
		mobj->color = player->mo->color;
		mobj->skin = player->mo->skin;

		// vertical flip
		if (player->mo->eflags & MFE_VERTICALFLIP)
			mobj->flags2 |= MF2_OBJECTFLIP;
		mobj->eflags |= (player->mo->eflags & MFE_VERTICALFLIP);

		// scale
		P_SetScale(mobj, player->mo->scale);
		mobj->destscale = player->mo->scale;

		if (type == MT_THOK) // spintrail-specific modification for MT_THOK
		{
			mobj->frame = FF_TRANS70;
			mobj->fuse = mobj->tics;
		}
	}

	P_SetTarget(&mobj->target, player->mo); // the one thing P_SpawnGhostMobj doesn't do
}

/** Called when \p player finishes the level.
  *
  * Only use for cases where the player should be able to move
  * while waiting for others to finish. Otherwise, use P_DoPlayerExit().
  *
  * In single player or if ::cv_exitmove is disabled, this will also cause
  * P_PlayerThink() to call P_DoPlayerExit(), so you do not need to
  * make a special cases for those.
  *
  * \param player The player who finished the level.
  * \sa P_DoPlayerExit
  *
  */
void P_DoPlayerFinish(player_t *player)
{
	if (player->pflags & PF_FINISHED)
		return;

	player->pflags |= PF_FINISHED;
	P_GiveFinishFlags(player);

	if (netgame)
		CONS_Printf(M_GetText("%s has completed the level.\n"), player_names[player-players]);

	player->powers[pw_underwater] = 0;
	player->powers[pw_spacetime] = 0;
	P_RestoreMusic(player);
}

//
// P_DoPlayerExit
//
// Player exits the map via sector trigger
void P_DoPlayerExit(player_t *player)
{
	if (player->exiting)
		return;

	if (cv_allowexitlevel.value == 0 && !G_PlatformGametype())
		return;
	else if (gametyperules & GTR_RACE) // If in Race Mode, allow
	{
		if (!countdown) // a 60-second wait ala Sonic 2.
			countdown = (cv_countdowntime.value - 1)*TICRATE + 1; // Use cv_countdowntime

		player->exiting = 3*TICRATE;

		if (!countdown2)
			countdown2 = (8 + cv_countdowntime.value)*TICRATE + 1; // 8 sec more than countdowntime -- 11 is too much

		if (P_CheckRacers())
			player->exiting = (14*TICRATE)/5 + 1;
	}
	else
		player->exiting = (14*TICRATE)/5 + 2; // Accidental death safeguard???

	//player->pflags &= ~PF_GLIDING;
	if (player->climbing)
	{
		player->climbing = 0;
		player->pflags |= P_GetJumpFlags(player);
		P_SetPlayerMobjState(player->mo, S_PLAY_JUMP);
	}
	else if (player->pflags & PF_STARTDASH)
	{
		player->pflags &= ~PF_STARTDASH;
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);
	}
	player->powers[pw_underwater] = 0;
	player->powers[pw_spacetime] = 0;
	P_RestoreMusic(player);
}

#define SPACESPECIAL 12
boolean P_InSpaceSector(mobj_t *mo) // Returns true if you are in space
{
	sector_t *sector = mo->subsector->sector;
	fixed_t topheight, bottomheight;

	if (GETSECSPECIAL(sector->special, 1) == SPACESPECIAL)
		return true;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (GETSECSPECIAL(rover->master->frontsector->special, 1) != SPACESPECIAL)
				continue;
			topheight    = P_GetFFloorTopZAt   (rover, mo->x, mo->y);
			bottomheight = P_GetFFloorBottomZAt(rover, mo->x, mo->y);

			if (mo->z + (mo->height/2) > topheight)
				continue;

			if (mo->z + (mo->height/2) < bottomheight)
				continue;

			return true;
		}
	}

	return false; // No vacuum here, Captain!
}

//
// P_PlayerHitFloor
//
// Handles player hitting floor surface.
// Returns whether to clip momz.
boolean P_PlayerHitFloor(player_t *player, boolean dorollstuff)
{
	boolean clipmomz;

	I_Assert(player->mo != NULL);

	if (player->powers[pw_carry] == CR_NIGHTSMODE)
		return true;

	if ((clipmomz = !(P_CheckDeathPitCollide(player->mo))) && player->mo->health && !player->spectator)
	{
		if (dorollstuff)
		{
			if ((player->charability2 == CA2_SPINDASH) && !((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_THOKKED) && !(player->charability == CA_THOK && player->secondjump)
			&& (player->cmd.buttons & BT_USE) && (FixedHypot(player->mo->momx, player->mo->momy) > (5*player->mo->scale)))
				player->pflags = (player->pflags|PF_SPINNING) & ~PF_THOKKED;
			else if (!(player->pflags & PF_STARTDASH))
				player->pflags &= ~PF_SPINNING;
		}

		if (player->pflags & PF_BOUNCING)
		{
			if (dorollstuff && player->mo->state-states != S_PLAY_BOUNCE_LANDING)
			{
				P_MobjCheckWater(player->mo);
				player->mo->momz *= -1;
				P_DoAbilityBounce(player, true);
				if (player->scoreadd)
					player->scoreadd--;
			}
			else
				player->mo->z += P_MobjFlip(player->mo);
			clipmomz = false;
		}
		else
		{
			P_MobjCheckWater(player->mo);
			if (player->pflags & PF_SPINNING)
			{
				if (player->mo->state-states != S_PLAY_ROLL && !(player->pflags & PF_STARTDASH))
				{
					P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
					S_StartSound(player->mo, sfx_spin);
				}
			}
			else if (player->pflags & PF_GLIDING) // ground gliding
			{
				if (dorollstuff)
				{
					player->skidtime = TICRATE;
					P_SetPlayerMobjState(player->mo, S_PLAY_GLIDE);
					P_SpawnSkidDust(player, player->mo->radius, true); // make sure the player knows they landed
					player->mo->tics = -1;
				}
				else if (!player->skidtime)
					player->pflags &= ~PF_GLIDING;
			}
			else if (player->charability == CA_GLIDEANDCLIMB && player->pflags & PF_THOKKED && !(player->pflags & (PF_JUMPED|PF_SHIELDABILITY))
			&& (player->mo->floorz != player->mo->watertop) && player->mo->state-states == S_PLAY_FALL)
			{
				if (player->mo->state-states != S_PLAY_GLIDE_LANDING)
				{
					P_ResetPlayer(player);
					P_SetPlayerMobjState(player->mo, S_PLAY_GLIDE_LANDING);
					player->pflags |= PF_STASIS;
					if (player->speed > FixedMul(player->runspeed, player->mo->scale))
						player->skidtime += player->mo->tics;
					player->mo->momx = ((player->mo->momx - player->cmomx)/2) + player->cmomx;
					player->mo->momy = ((player->mo->momy - player->cmomy)/2) + player->cmomy;
					if (player->powers[pw_super])
					{
						P_Earthquake(player->mo, player->mo, 256*FRACUNIT);
						S_StartSound(player->mo, sfx_s3k49);
					}
					else
						S_StartSound(player->mo, sfx_s3k4c);
				}
			}
			else if (player->charability2 == CA2_MELEE
				&& ((player->panim == PA_ABILITY2) || (player->charability == CA_TWINSPIN && player->panim == PA_ABILITY && player->cmd.buttons & (BT_JUMP|BT_USE))))
			{
				if (player->mo->state-states != S_PLAY_MELEE_LANDING)
				{
					mobjtype_t type = player->revitem;
					P_SetPlayerMobjState(player->mo, S_PLAY_MELEE_LANDING);
					player->mo->tics = (player->mo->movefactor == FRACUNIT) ? TICRATE/2 : (FixedDiv(35<<(FRACBITS-1), FixedSqrt(player->mo->movefactor)))>>FRACBITS;
					S_StartSound(player->mo, sfx_s3k8b);
					player->pflags |= PF_FULLSTASIS;

					// hearticles
					if (type)
					{
						UINT8 i = 0;
						angle_t throwang = -(2*ANG30);
						fixed_t xo = P_ReturnThrustX(player->mo, player->drawangle, 16*player->mo->scale);
						fixed_t yo = P_ReturnThrustY(player->mo, player->drawangle, 16*player->mo->scale);
						fixed_t zo = 6*player->mo->scale;
						fixed_t mu = FixedMul(player->maxdash, player->mo->scale);
						fixed_t mu2 = FixedHypot(player->mo->momx, player->mo->momy);
						fixed_t ev;
						mobj_t *missile = NULL;
						if (mu2 < mu)
							mu2 = mu;
						ev = (50*FRACUNIT - (mu/25))/50;
						while (i < 5)
						{
							missile = P_SpawnMobjFromMobj(player->mo, xo, yo, zo, type);
							P_SetTarget(&missile->target, player->mo);
							missile->angle = throwang + player->drawangle;
							P_Thrust(missile, player->drawangle + ANGLE_90,
								P_ReturnThrustY(missile, throwang, mu)); // side to side component
							P_Thrust(missile, player->drawangle, mu2); // forward component
							P_SetObjectMomZ(missile, (4 + ((i&1)<<1))*FRACUNIT, true);
							missile->momz += player->mo->pmomz;
							missile->fuse = TICRATE/2;
							missile->extravalue2 = ev;

							i++;
							throwang += ANG30;
						}
						if (mobjinfo[type].seesound && missile)
							S_StartSound(missile, missile->info->seesound);
					}
				}
			}
			else if (player->charability == CA_GLIDEANDCLIMB && (player->mo->state-states == S_PLAY_GLIDE_LANDING))
				;
			else if (player->charability2 == CA2_GUNSLINGER && player->panim == PA_ABILITY2)
				;
			else if (dorollstuff && player->panim != PA_IDLE && player->panim != PA_WALK && player->panim != PA_RUN && player->panim != PA_DASH)
			{
				fixed_t runspd = FixedMul(player->runspeed, player->mo->scale);

				// See comments in P_MovePlayer for explanation of changes.

				if (player->powers[pw_super])
					runspd = FixedMul(runspd, 5*FRACUNIT/3);

				runspd = FixedMul(runspd, player->mo->movefactor);

				if (maptol & TOL_2D)
					runspd = FixedMul(runspd, 2*FRACUNIT/3);

				if (player->cmomx || player->cmomy)
				{
					if (player->charflags & SF_DASHMODE && player->dashmode >= DASHMODE_THRESHOLD && player->panim != PA_DASH)
						P_SetPlayerMobjState(player->mo, S_PLAY_DASH);
					else if (player->speed >= runspd
					&& (player->panim != PA_RUN || player->mo->state-states == S_PLAY_FLOAT_RUN))
						P_SetPlayerMobjState(player->mo, S_PLAY_RUN);
					else if ((player->rmomx || player->rmomy)
					&& (player->panim != PA_WALK || player->mo->state-states == S_PLAY_FLOAT))
						P_SetPlayerMobjState(player->mo, S_PLAY_WALK);
					else if (!player->rmomx && !player->rmomy && player->panim != PA_IDLE)
						P_SetPlayerMobjState(player->mo, S_PLAY_STND);
				}
				else
				{
					if (player->charflags & SF_DASHMODE && player->dashmode >= DASHMODE_THRESHOLD && player->panim != PA_DASH)
						P_SetPlayerMobjState(player->mo, S_PLAY_DASH);
					else if (player->speed >= runspd
					&& (player->panim != PA_RUN || player->mo->state-states == S_PLAY_FLOAT_RUN))
						P_SetPlayerMobjState(player->mo, S_PLAY_RUN);
					else if ((player->mo->momx || player->mo->momy)
					&& (player->panim != PA_WALK || player->mo->state-states == S_PLAY_FLOAT))
						P_SetPlayerMobjState(player->mo, S_PLAY_WALK);
					else if (!player->mo->momx && !player->mo->momy && player->panim != PA_IDLE)
						P_SetPlayerMobjState(player->mo, S_PLAY_STND);
				}
			}

			if (!(player->pflags & PF_GLIDING))
				player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE);
			player->pflags &= ~(PF_STARTJUMP|PF_THOKKED|PF_CANCARRY/*|PF_GLIDING*/);
			player->secondjump = 0;
			player->glidetime = 0;
			player->climbing = 0;
			player->powers[pw_tailsfly] = 0;

			if (player->pflags & PF_SHIELDABILITY)
			{
				player->pflags &= ~PF_SHIELDABILITY;

				if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL) // Elemental shield's stomp attack.
				{
					if (player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)) // play a blunt sound
						S_StartSound(player->mo, sfx_s3k4c);
					else // create a fire pattern on the ground
					{
						S_StartSound(player->mo, sfx_s3k47);
						P_ElementalFire(player, true);
					}
					P_SetObjectMomZ(player->mo,
					(player->mo->eflags & MFE_UNDERWATER)
					? 6*FRACUNIT/5
					: 5*FRACUNIT/2,
					false);
					P_SetPlayerMobjState(player->mo, S_PLAY_FALL);
					player->mo->momx = player->mo->momy = 0;
					clipmomz = false;
				}
				else if ((player->powers[pw_shield] & SH_NOSTACK) == SH_BUBBLEWRAP) // Bubble shield's bounce attack.
				{
					P_DoBubbleBounce(player);
					clipmomz = false;
				}
			}
		}
	}

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

static boolean P_PlayerCanBust(player_t *player, ffloor_t *rover)
{
	if (!(rover->flags & FF_EXISTS))
		return false;

	if (!(rover->flags & FF_BUSTUP))
		return false;

	/*if (rover->master->frontsector->crumblestate != CRUMBLE_NONE)
		return false;*/

	// If it's an FF_SHATTER, you can break it just by touching it.
	if (rover->flags & FF_SHATTER)
		return true;

	// If it's an FF_SPINBUST, you can break it if you are in your spinning frames
	// (either from jumping or spindashing).
	if (rover->flags & FF_SPINBUST)
	{
		if ((player->pflags & PF_SPINNING) && !(player->pflags & PF_STARTDASH))
			return true;

		if ((player->pflags & PF_JUMPED) && !(player->pflags & PF_NOJUMPDAMAGE))
			return true;
	}

	// Strong abilities can break even FF_STRONGBUST.
	if (player->charability == CA_GLIDEANDCLIMB)
		return true;

	if (player->pflags & PF_BOUNCING)
		return true;

	if (player->charability == CA_TWINSPIN && player->panim == PA_ABILITY)
		return true;

	if (player->charability2 == CA2_MELEE && player->panim == PA_ABILITY2)
		return true;

	// Everyone else is out of luck.
	if (rover->flags & FF_STRONGBUST)
		return false;

	// Spinning (and not jumping)
	if ((player->pflags & PF_SPINNING) && !(player->pflags & PF_JUMPED))
		return true;

	// Super
	if (player->powers[pw_super])
		return true;

	// Dashmode
	if ((player->charflags & (SF_DASHMODE|SF_MACHINE)) == (SF_DASHMODE|SF_MACHINE) && player->dashmode >= DASHMODE_THRESHOLD)
		return true;

	// NiGHTS drill
	if (player->pflags & PF_DRILLING)
		return true;

	// Recording for Metal Sonic
	if (metalrecording)
		return true;

	return false;
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

	if (!(player->pflags & PF_BOUNCING)) // Bouncers only get to break downwards, not sideways
	{
		P_UnsetThingPosition(player->mo);
		player->mo->x += player->mo->momx;
		player->mo->y += player->mo->momy;
		P_SetThingPosition(player->mo);
	}

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

			if (((player->charability == CA_TWINSPIN) && (player->panim == PA_ABILITY))
			|| ((P_MobjFlip(player->mo)*player->mo->momz < 0) && (player->pflags & PF_BOUNCING || ((player->charability2 == CA2_MELEE) && (player->panim == PA_ABILITY2)))))
			{
				topheight -= player->mo->momz;
				bottomheight -= player->mo->momz;
			}

			// Height checks
			if (rover->flags & FF_SHATTERBOTTOM)
			{
				if (player->mo->z + player->mo->momz + player->mo->height < bottomheight)
					continue;

				if (player->mo->z + player->mo->height > bottomheight)
					continue;
			}
			else if (rover->flags & FF_SPINBUST)
			{
				if (player->mo->z + player->mo->momz > topheight)
					continue;

				if (player->mo->z + player->mo->height < bottomheight)
					continue;
			}
			else if (rover->flags & FF_SHATTER)
			{
				if (player->mo->z + player->mo->momz > topheight)
					continue;

				if (player->mo->z + player->mo->momz + player->mo->height < bottomheight)
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
bustupdone:
	if (!(player->pflags & PF_BOUNCING))
	{
		P_UnsetThingPosition(player->mo);
		player->mo->x = oldx;
		player->mo->y = oldy;
		P_SetThingPosition(player->mo);
	}
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
			fixed_t bouncestrength;
			fixed_t topheight, bottomheight;

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

			bouncestrength = P_AproxDistance(rover->master->dx, rover->master->dy)/100;

			if (oldz < P_GetFOFTopZ(player->mo, node->m_sector, rover, oldx, oldy, NULL)
					&& oldz + player->mo->height > P_GetFOFBottomZ(player->mo, node->m_sector, rover, oldx, oldy, NULL))
			{
				player->mo->momx = -FixedMul(player->mo->momx,bouncestrength);
				player->mo->momy = -FixedMul(player->mo->momy,bouncestrength);

				if (player->pflags & PF_SPINNING)
				{
					player->pflags &= ~PF_SPINNING;
					player->pflags |= P_GetJumpFlags(player);
					player->pflags |= PF_THOKKED;
				}
			}
			else
			{
				fixed_t newmom;
				pslope_t *slope = (abs(oldz - topheight) < abs(oldz + player->mo->height - bottomheight)) ? *rover->t_slope : *rover->b_slope;

				momentum.x = player->mo->momx;
				momentum.y = player->mo->momy;
				momentum.z = player->mo->momz*2;

				if (slope)
					P_ReverseQuantizeMomentumToSlope(&momentum, slope);

				newmom = momentum.z = -FixedMul(momentum.z,bouncestrength)/2;

				if (abs(newmom) < (bouncestrength*2))
					goto bouncydone;

				if (!(rover->master->flags & ML_BOUNCY))
				{
					if (newmom > 0)
					{
						if (newmom < 8*FRACUNIT)
							newmom = 8*FRACUNIT;
					}
					else if (newmom < 0)
					{
						if (newmom > -8*FRACUNIT)
							newmom = -8*FRACUNIT;
					}
				}

				if (newmom > P_GetPlayerHeight(player)/2)
					newmom = P_GetPlayerHeight(player)/2;
				else if (newmom < -P_GetPlayerHeight(player)/2)
					newmom = -P_GetPlayerHeight(player)/2;

				momentum.z = newmom*2;

				if (slope)
					P_QuantizeMomentumToSlope(&momentum, slope);

				player->mo->momx = momentum.x;
				player->mo->momy = momentum.y;
				player->mo->momz = momentum.z/2;

				if (player->pflags & PF_SPINNING)
				{
					player->pflags &= ~PF_SPINNING;
					player->pflags |= P_GetJumpFlags(player);
					player->pflags |= PF_THOKKED;
				}
			}

			if ((player->pflags & PF_SPINNING) && player->speed < FixedMul(1<<FRACBITS, player->mo->scale) && player->mo->momz)
			{
				player->pflags &= ~PF_SPINNING;
				player->pflags |= P_GetJumpFlags(player);
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
// P_CheckSneakerAndLivesTimer
//
// Restores music from sneaker and life fanfares
//
static void P_CheckSneakerAndLivesTimer(player_t *player)
{
	if (player->powers[pw_extralife] == 1) // Extra Life!
		P_RestoreMusic(player);

	if (player->powers[pw_sneakers] == 1)
		P_RestoreMusic(player);
}

//
// P_CheckUnderwaterAndSpaceTimer
//
// Restores music from underwater and space warnings, and handles number generation
//
static void P_CheckUnderwaterAndSpaceTimer(player_t *player)
{
	tic_t timeleft = (player->powers[pw_spacetime]) ? player->powers[pw_spacetime] : player->powers[pw_underwater];

	if (player->exiting || (player->pflags & PF_FINISHED))
		player->powers[pw_underwater] = player->powers[pw_spacetime] = 0;

	timeleft--; // The original code was all n*TICRATE + 1, so let's remove 1 tic for simplicity

	if ((timeleft == 11*TICRATE) // 5
	 || (timeleft ==  9*TICRATE) // 4
	 || (timeleft ==  7*TICRATE) // 3
	 || (timeleft ==  5*TICRATE) // 2
	 || (timeleft ==  3*TICRATE) // 1
	 || (timeleft ==  1*TICRATE) // 0
	) {
		fixed_t height = (player->mo->eflags & MFE_VERTICALFLIP)
		? player->mo->z - FixedMul(8*FRACUNIT + mobjinfo[MT_DROWNNUMBERS].height, FixedMul(player->mo->scale, player->shieldscale))
		: player->mo->z + player->mo->height + FixedMul(8*FRACUNIT, FixedMul(player->mo->scale, player->shieldscale));

		mobj_t *numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, height, MT_DROWNNUMBERS);

		timeleft /= (2*TICRATE); // To be strictly accurate it'd need to be ((timeleft/TICRATE) - 1)/2, but integer division rounds down for us

		if (player->charflags & SF_MACHINE)
		{
			S_StartSound(player->mo, sfx_buzz1);
			timeleft += 6;
		}
		else
			S_StartSound(player->mo, sfx_dwnind);

		if (timeleft) // Don't waste time setting the state if the time is 0.
			P_SetMobjState(numbermobj, numbermobj->info->spawnstate+timeleft);

		P_SetTarget(&numbermobj->target, player->mo);
		numbermobj->threshold = 40;
		numbermobj->destscale = player->mo->scale;
		P_SetScale(numbermobj, player->mo->scale);
	}
	// Underwater timer runs out
	else if (timeleft == 1)
	{
		if ((netgame || multiplayer) && P_IsLocalPlayer(player))
			S_ChangeMusic(mapmusname, mapmusflags, true);

		if (player->powers[pw_spacetime] == 1)
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_SPACEDROWN);
		else
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_DROWNED);
	}

	if (!(player->mo->eflags & MFE_UNDERWATER) && player->powers[pw_underwater])
	{
		if (player->powers[pw_underwater] <= 12*TICRATE + 1)
		{
			player->powers[pw_underwater] = 0;
			P_RestoreMusic(player);
		}
		else
			player->powers[pw_underwater] = 0;
	}

	if (player->powers[pw_spacetime] > 1 && !P_InSpaceSector(player->mo))
		player->powers[pw_spacetime] = 0;

	// Underwater audio cues
	if (P_IsLocalPlayer(player) && !player->bot)
	{
		if ((player->powers[pw_underwater] == 25*TICRATE + 1)
		|| (player->powers[pw_underwater] == 20*TICRATE + 1)
		|| (player->powers[pw_underwater] == 15*TICRATE + 1))
			S_StartSound(NULL, sfx_wtrdng);

		if (player->powers[pw_underwater] == 11*TICRATE + 1
		&& player == &players[consoleplayer])
		{
			P_PlayJingle(player, JT_DROWN);
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
	if (!player->powers[pw_invulnerability])
		return;

	if (mariomode && !player->powers[pw_super])
		player->mo->color = (UINT16)(SKINCOLOR_RUBY + (leveltime % (numskincolors - SKINCOLOR_RUBY))); // Passes through all saturated colours
	else if (leveltime % (TICRATE/7) == 0)
	{
		mobj_t *sparkle = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_IVSP);
		sparkle->destscale = player->mo->scale;
		P_SetScale(sparkle, player->mo->scale);
	}

	// Resume normal music stuff.
	if (player->powers[pw_invulnerability] == 1)
	{
		if (!player->powers[pw_super])
		{
			if (mariomode)
			{
				if ((player->powers[pw_shield] & SH_STACK) == SH_FIREFLOWER)
				{
					player->mo->color = SKINCOLOR_WHITE;
					G_GhostAddColor(GHC_FIREFLOWER);
				}
				else
				{
					player->mo->color = player->skincolor;
					G_GhostAddColor(GHC_NORMAL);
				}
			}

			// If you had a shield, restore its visual significance
			P_SpawnShieldOrb(player);
		}

		if (!player->powers[pw_super] || (mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC))
			P_RestoreMusic(player);
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

	if (!(player->mo->eflags & MFE_UNDERWATER) || ((player->powers[pw_shield] & SH_PROTECTWATER) && !(player->powers[pw_carry] == CR_NIGHTSMODE)) || player->spectator)
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

	// Tails stirs up the water while flying in it
	if (player->powers[pw_tailsfly] && (leveltime & 1) && player->charability != CA_SWIM)
	{
		fixed_t radius = player->mo->radius;
		angle_t fa = ((leveltime%45)*FINEANGLES/8) & FINEMASK;
		fixed_t stirwaterx = FixedMul(FINECOSINE(fa),radius);
		fixed_t stirwatery = FixedMul(FINESINE(fa),radius);
		fixed_t stirwaterz;

		if (player->mo->eflags & MFE_VERTICALFLIP)
			stirwaterz = player->mo->z + player->mo->height - (4<<FRACBITS);
		else
			stirwaterz = player->mo->z + (4<<FRACBITS);

		bubble = P_SpawnMobj(
			player->mo->x + stirwaterx,
			player->mo->y + stirwatery,
			stirwaterz, MT_SMALLBUBBLE);
		bubble->destscale = player->mo->scale;
		P_SetScale(bubble,bubble->destscale);

		bubble = P_SpawnMobj(
			player->mo->x - stirwaterx,
			player->mo->y - stirwatery,
			stirwaterz, MT_SMALLBUBBLE);
		bubble->destscale = player->mo->scale;
		P_SetScale(bubble,bubble->destscale);
	}
}

//
// P_DoPlayerHeadSigns
//
// Spawns "IT" and "GOT FLAG" signs for Tag and CTF respectively
//
static void P_DoPlayerHeadSigns(player_t *player)
{
	if (G_TagGametype())
	{
		// If you're "IT", show a big "IT" over your head for others to see.
		if (player->pflags & PF_TAGIT)
		{
			if (!P_IsLocalPlayer(player)) // Don't display it on your own view.
			{
				if (!(player->mo->eflags & MFE_VERTICALFLIP))
					P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height, MT_TAG);
				else
					P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z - mobjinfo[MT_TAG].height, MT_TAG)->eflags |= MFE_VERTICALFLIP;
			}
		}
	}
	else if ((gametyperules & GTR_TEAMFLAGS) && (player->gotflag & (GF_REDFLAG|GF_BLUEFLAG))) // If you have the flag (duh).
	{
		// Spawn a got-flag message over the head of the player that
		// has it (but not on your own screen if you have the flag).
		if (splitscreen || player != &players[consoleplayer])
		{
			mobj_t *sign = P_SpawnMobj(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy,
					player->mo->z+player->mo->momz, MT_GOTFLAG);
			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				sign->z += player->mo->height-P_GetPlayerHeight(player)-mobjinfo[MT_GOTFLAG].height-FixedMul(16*FRACUNIT, player->mo->scale);
				sign->eflags |= MFE_VERTICALFLIP;
			}
			else
				sign->z += P_GetPlayerHeight(player)+FixedMul(16*FRACUNIT, player->mo->scale);

			if (player->gotflag & GF_REDFLAG)
				sign->frame = 1|FF_FULLBRIGHT;
			else //if (player->gotflag & GF_BLUEFLAG)
				sign->frame = 2|FF_FULLBRIGHT;
		}
	}
}

//
// P_DoClimbing
//
// Handles player climbing
//
static void P_DoClimbing(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;
	fixed_t platx;
	fixed_t platy;
	subsector_t *glidesector;
	boolean climb = true;

	platx = P_ReturnThrustX(player->mo, player->mo->angle, player->mo->radius + FixedMul(8*FRACUNIT, player->mo->scale));
	platy = P_ReturnThrustY(player->mo, player->mo->angle, player->mo->radius + FixedMul(8*FRACUNIT, player->mo->scale));

	glidesector = R_PointInSubsectorOrNull(player->mo->x + platx, player->mo->y + platy);

	{
		boolean floorclimb = false;
		boolean thrust = false;
		boolean boostup = false;
		boolean skyclimber = false;
		fixed_t floorheight, ceilingheight;

		if (!glidesector)
			floorclimb = true;
		else
		{
			floorheight   = P_GetSectorFloorZAt  (glidesector->sector, player->mo->x, player->mo->y);
			ceilingheight = P_GetSectorCeilingZAt(glidesector->sector, player->mo->x, player->mo->y);

			if (glidesector->sector->ffloors)
			{
				ffloor_t *rover;
				fixed_t topheight, bottomheight;

				for (rover = glidesector->sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_BLOCKPLAYER) || (rover->flags & FF_BUSTUP))
						continue;

					floorclimb = true;

					topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
					bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);

					// Only supports rovers that are moving like an 'elevator', not just the top or bottom.
					if (rover->master->frontsector->floorspeed && rover->master->frontsector->ceilspeed == 42)
					{
						if ((!(player->mo->eflags & MFE_VERTICALFLIP) && (bottomheight < player->mo->z+player->mo->height)
							&& (topheight >= player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale)))
						|| ((player->mo->eflags & MFE_VERTICALFLIP) && (topheight > player->mo->z)
							&& (bottomheight <= player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale))))
						{
							if (cmd->forwardmove != 0)
								player->mo->momz += rover->master->frontsector->floorspeed;
							else
							{
								player->mo->momz = rover->master->frontsector->floorspeed;
								climb = false;
							}
						}
					}

					// Gravity is flipped, so the comments are, too.
					if (player->mo->eflags & MFE_VERTICALFLIP)
					{
						// Trying to climb down past the bottom of the FOF
						if ((topheight >= player->mo->z + player->mo->height) && ((player->mo->z + player->mo->height + player->mo->momz) >= topheight))
						{
							fixed_t bottomheight2;
							ffloor_t *roverbelow;
							boolean foundfof = false;
							floorclimb = true;
							boostup = false;

							// Is there a FOF directly below this one that we can move onto?
							for (roverbelow = glidesector->sector->ffloors; roverbelow; roverbelow = roverbelow->next)
							{
								if (!(roverbelow->flags & FF_EXISTS) || !(roverbelow->flags & FF_BLOCKPLAYER) || (roverbelow->flags & FF_BUSTUP))
									continue;

								if (roverbelow == rover)
									continue;

								bottomheight2 = P_GetFFloorBottomZAt(roverbelow, player->mo->x, player->mo->y);
								if (bottomheight2 < topheight + FixedMul(16*FRACUNIT, player->mo->scale))
									foundfof = true;
							}

							if (!foundfof)
								player->mo->momz = 0;
						}

						// Below the FOF
						if (topheight <= player->mo->z)
						{
							floorclimb = false;
							boostup = false;
							thrust = false;
						}

						// Above the FOF
						if (bottomheight > player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale))
						{
							floorclimb = false;
							thrust = true;
							boostup = true;
						}
					}
					else
					{
						// Trying to climb down past the bottom of a FOF
						if ((bottomheight <= player->mo->z) && ((player->mo->z + player->mo->momz) <= bottomheight))
						{
							fixed_t topheight2;
							ffloor_t *roverbelow;
							boolean foundfof = false;
							floorclimb = true;
							boostup = false;

							// Is there a FOF directly below this one that we can move onto?
							for (roverbelow = glidesector->sector->ffloors; roverbelow; roverbelow = roverbelow->next)
							{
								if (!(roverbelow->flags & FF_EXISTS) || !(roverbelow->flags & FF_BLOCKPLAYER) || (roverbelow->flags & FF_BUSTUP))
									continue;

								if (roverbelow == rover)
									continue;

								topheight2 = P_GetFFloorTopZAt(roverbelow, player->mo->x, player->mo->y);
								if (topheight2 > bottomheight - FixedMul(16*FRACUNIT, player->mo->scale))
									foundfof = true;
							}

							if (!foundfof)
								player->mo->momz = 0;
						}

						// Below the FOF
						if (bottomheight >= player->mo->z + player->mo->height)
						{
							floorclimb = false;
							boostup = false;
							thrust = false;
						}

						// Above the FOF
						if (topheight < player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale))
						{
							floorclimb = false;
							thrust = true;
							boostup = true;
						}
					}

					if (floorclimb)
					{
						if (rover->flags & FF_CRUMBLE && !(netgame && player->spectator))
							EV_StartCrumble(rover->master->frontsector, rover, (rover->flags & FF_FLOATBOB), player, rover->alpha, !(rover->flags & FF_NORETURN));
						break;
					}
				}
			}

			// Gravity is flipped, so are comments.
			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				// Trying to climb down past the upper texture area
				if ((floorheight >= player->mo->z + player->mo->height) && ((player->mo->z + player->mo->height + player->mo->momz) >= floorheight))
				{
					boolean foundfof = false;
					floorclimb = true;

					// Is there a FOF directly below that we can move onto?
					if (glidesector->sector->ffloors)
					{
						fixed_t bottomheight;
						ffloor_t *rover;
						for (rover = glidesector->sector->ffloors; rover; rover = rover->next)
						{
							if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_BLOCKPLAYER) || (rover->flags & FF_BUSTUP))
								continue;

							bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);
							if (bottomheight < floorheight + FixedMul(16*FRACUNIT, player->mo->scale))
							{
								foundfof = true;
								break;
							}
						}
					}

					if (!foundfof)
						player->mo->momz = 0;
				}

				// Reached the top of the lower texture area
				if (!floorclimb && ceilingheight > player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale)
					&& (glidesector->sector->ceilingpic == skyflatnum || floorheight < (player->mo->z - FixedMul(8*FRACUNIT, player->mo->scale))))
				{
					thrust = true;
					boostup = true;
					// Play climb-up animation here
				}
			}
			else
			{
				// Trying to climb down past the upper texture area
				if ((ceilingheight <= player->mo->z) && ((player->mo->z + player->mo->momz) <= ceilingheight))
				{
					boolean foundfof = false;
					floorclimb = true;

					// Is there a FOF directly below that we can move onto?
					if (glidesector->sector->ffloors)
					{
						fixed_t topheight;
						ffloor_t *rover;
						for (rover = glidesector->sector->ffloors; rover; rover = rover->next)
						{
							if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_BLOCKPLAYER) || (rover->flags & FF_BUSTUP))
								continue;

							topheight = P_GetFFloorTopZAt(rover, player->mo->x, player->mo->y);
							if (topheight > ceilingheight - FixedMul(16*FRACUNIT, player->mo->scale))
							{
								foundfof = true;
								break;
							}
						}
					}

					if (!foundfof)
						player->mo->momz = 0;
				}

				// Allow climbing from a FOF or lower texture onto the upper texture and vice versa.
				if (player->mo->z > ceilingheight - FixedMul(16*FRACUNIT, player->mo->scale))
				{
					floorclimb = true;
					thrust = false;
					boostup = false;
				}

				// Reached the top of the lower texture area
				if (!floorclimb && floorheight < player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale)
					&& (glidesector->sector->ceilingpic == skyflatnum || ceilingheight > (player->mo->z + player->mo->height + FixedMul(8*FRACUNIT, player->mo->scale))))
				{
					thrust = true;
					boostup = true;
					// Play climb-up animation here
				}
			}

			// Trying to climb on the sky
			if ((ceilingheight < player->mo->z) && glidesector->sector->ceilingpic == skyflatnum)
			{
				skyclimber = true;
			}

			// Climbing on the lower texture area?
			if ((!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale) < floorheight)
				|| ((player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + player->mo->height <= floorheight))
			{
				floorclimb = true;

				if (glidesector->sector->floorspeed)
				{
					if (cmd->forwardmove != 0)
						player->mo->momz += glidesector->sector->floorspeed;
					else
					{
						player->mo->momz = glidesector->sector->floorspeed;
						climb = false;
					}
				}
			}
			// Climbing on the upper texture area?
			else if ((!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z >= ceilingheight)
				|| ((player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale) > ceilingheight))
			{
				floorclimb = true;

				if (glidesector->sector->ceilspeed)
				{
					if (cmd->forwardmove != 0)
						player->mo->momz += glidesector->sector->ceilspeed;
					else
					{
						player->mo->momz = glidesector->sector->ceilspeed;
						climb = false;
					}
				}
			}
		}

		if (player->lastsidehit != -1 && player->lastlinehit != -1)
		{
			thinker_t *think;
			scroll_t *scroller;
			angle_t sideangle;
			fixed_t dx, dy;

			for (think = thlist[THINK_MAIN].next; think != &thlist[THINK_MAIN]; think = think->next)
			{
				if (think->function.acp1 != (actionf_p1)T_Scroll)
					continue;

				scroller = (scroll_t *)think;

				if (scroller->type != sc_side)
					continue;

				if (scroller->affectee != player->lastsidehit)
					continue;

				if (scroller->accel)
				{
					dx = scroller->vdx;
					dy = scroller->vdy;
				}
				else
				{
					dx = scroller->dx;
					dy = scroller->dy;
				}

				if (cmd->forwardmove != 0)
				{
					player->mo->momz += dy;
					climb = true;
				}
				else
				{
					player->mo->momz = dy;
					climb = false;
				}

				sideangle = R_PointToAngle2(lines[player->lastlinehit].v2->x,lines[player->lastlinehit].v2->y,lines[player->lastlinehit].v1->x,lines[player->lastlinehit].v1->y);

				if (cmd->sidemove != 0)
				{
					P_Thrust(player->mo, sideangle, dx);
					climb = true;
				}
				else
				{
					P_InstaThrust(player->mo, sideangle, dx);
					climb = false;
				}
			}
		}

		if (cmd->sidemove != 0 || cmd->forwardmove != 0)
			climb = true;
		else
			climb = false;

		if (player->climbing && climb && (player->mo->momx || player->mo->momy || player->mo->momz)
			&& player->mo->state-states != S_PLAY_CLIMB)
			P_SetPlayerMobjState(player->mo, S_PLAY_CLIMB);
		else if ((!(player->mo->momx || player->mo->momy || player->mo->momz) || !climb) && player->mo->state-states != S_PLAY_CLING)
			P_SetPlayerMobjState(player->mo, S_PLAY_CLING);

		if (!floorclimb)
		{
			if (boostup)
			{
				P_SetObjectMomZ(player->mo, 2*FRACUNIT, true);
				if (cmd->forwardmove)
					player->mo->momz = 2*player->mo->momz/3;
			}
			if (thrust)
				P_Thrust(player->mo, player->mo->angle, FixedMul(4*FRACUNIT, player->mo->scale)); // Lil' boost up.

			player->climbing = 0;
			player->pflags |= P_GetJumpFlags(player);
			P_SetPlayerMobjState(player->mo, S_PLAY_JUMP);
		}

		if (skyclimber)
		{
			player->climbing = 0;
			player->pflags |= P_GetJumpFlags(player);
			P_SetPlayerMobjState(player->mo, S_PLAY_JUMP);
		}
	}

	if (cmd->sidemove != 0 || cmd->forwardmove != 0)
		climb = true;
	else
		climb = false;

	if (player->climbing && climb && (player->mo->momx || player->mo->momy || player->mo->momz)
		&& player->mo->state-states != S_PLAY_CLIMB)
		P_SetPlayerMobjState(player->mo, S_PLAY_CLIMB);
	else if ((!(player->mo->momx || player->mo->momy || player->mo->momz) || !climb) && player->mo->state-states != S_PLAY_CLING)
		P_SetPlayerMobjState(player->mo, S_PLAY_CLING);

	if (cmd->buttons & BT_USE && !(player->pflags & PF_JUMPSTASIS))
	{
		player->climbing = 0;
		player->pflags |= P_GetJumpFlags(player);
		P_SetPlayerMobjState(player->mo, S_PLAY_JUMP);
		P_SetObjectMomZ(player->mo, 4*FRACUNIT, false);
		P_Thrust(player->mo, player->mo->angle, FixedMul(-4*FRACUNIT, player->mo->scale));
	}

#define CLIMBCONEMAX FixedAngle(90*FRACUNIT)
	if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
	{
		angle_t angdiff = P_GetLocalAngle(player) - player->mo->angle;
		if (angdiff < ANGLE_180 && angdiff > CLIMBCONEMAX)
			P_SetLocalAngle(player, player->mo->angle + CLIMBCONEMAX);
		else if (angdiff > ANGLE_180 && angdiff < InvAngle(CLIMBCONEMAX))
			P_SetLocalAngle(player, player->mo->angle - CLIMBCONEMAX);
	}

	if (player->climbing == 0)
		P_SetPlayerMobjState(player->mo, S_PLAY_JUMP);

	if (player->climbing && P_IsObjectOnGround(player->mo))
	{
		P_ResetPlayer(player);
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);
	}
}

//
// PIT_CheckSolidsTeeter
// AKA: PIT_HacksToStopPlayersTeeteringOnGargoyles
//

static mobj_t *teeterer; // the player checking for teetering
static boolean solidteeter; // saves whether the player can teeter on this or not
static fixed_t highesttop; // highest floor found so far
// reserved for standing on multiple objects
static boolean couldteeter;
static fixed_t teeterxl, teeterxh;
static fixed_t teeteryl, teeteryh;

static boolean PIT_CheckSolidsTeeter(mobj_t *thing)
{
	fixed_t blockdist;
	fixed_t tiptop = FixedMul(MAXSTEPMOVE, teeterer->scale);
	fixed_t thingtop = thing->z + thing->height;
	fixed_t teeterertop = teeterer->z + teeterer->height;

	if (!teeterer || !thing)
		return true;

	if (!(thing->flags & MF_SOLID))
		return true;

	if (thing->flags & MF_NOCLIP)
		return true;

	if (thing == teeterer)
		return true;

	if (thing->player && cv_tailspickup.value && !(gametyperules & GTR_HIDEFROZEN))
		return true;

	blockdist = teeterer->radius + thing->radius;

	if (abs(thing->x - teeterer->x) >= blockdist || abs(thing->y - teeterer->y) >= blockdist)
		return true; // didn't hit it

	if (teeterer->eflags & MFE_VERTICALFLIP)
	{
		if (thingtop < teeterer->z)
			return true;
		if (thing->z > highesttop)
			return true;
		highesttop = thing->z;
		if (thing->z > teeterertop + tiptop)
		{
			solidteeter = true;
			return true;
		}
	}
	else
	{
		if (thing->z > teeterertop)
			return true;
		if (thingtop < highesttop)
			return true;
		highesttop = thingtop;
		if (thingtop < teeterer->z - tiptop)
		{
			solidteeter = true;
			return true;
		}
	}

	// are you standing on this?
	if ((teeterer->eflags & MFE_VERTICALFLIP && thing->z - FixedMul(FRACUNIT, teeterer->scale) == teeterertop)
	|| (!(teeterer->eflags & MFE_VERTICALFLIP) && thingtop + FixedMul(FRACUNIT, teeterer->scale) == teeterer->z))
	{
		fixed_t teeterdist = thing->radius - FixedMul(5*FRACUNIT, teeterer->scale);
		// how far are you from the edge?
		if (abs(teeterer->x - thing->x) > teeterdist || abs(teeterer->y - thing->y) > teeterdist)
		{
			if (couldteeter) // player is standing on another object already, see if we can stand on both and not teeter!
			{
				if (thing->x - teeterdist < teeterxl)
					teeterxl = thing->x - teeterdist;
				if (thing->x + teeterdist > teeterxh)
					teeterxh = thing->x + teeterdist;
				if (thing->y - teeterdist < teeteryl)
					teeteryl = thing->y - teeterdist;
				if (thing->y + teeterdist > teeteryh)
					teeteryh = thing->y + teeterdist;

				if (teeterer->x < teeterxl)
					return true;
				if (teeterer->x > teeterxh)
					return true;
				if (teeterer->y < teeteryl)
					return true;
				if (teeterer->y > teeteryh)
					return true;

				solidteeter = false; // you can stop teetering now!
				couldteeter = false; // just in case...
				return false;
			}
			else
			{
				// too far! don't change teeter status though
				// save teeter x/y limits to bring up later
				teeterxl = thing->x - teeterdist;
				teeterxh = thing->x + teeterdist;
				teeteryl = thing->y - teeterdist;
				teeteryh = thing->y + teeterdist;
			}
			couldteeter = true;
			return true;
		}
		solidteeter = false;
		couldteeter = false;
		return false; // you're definitely not teetering, that's the end of the matter
	}
	solidteeter = false;
	return true; // you're not teetering but it's not neccessarily over, YET
}

//
// P_DoTeeter
//
// Handles player teetering
//
static void P_DoTeeter(player_t *player)
{
	boolean teeter = false;
	boolean roverfloor; // solid 3d floors?
	fixed_t floorheight, ceilingheight;
	fixed_t topheight, bottomheight; // for 3d floor usage
	const fixed_t tiptop = FixedMul(MAXSTEPMOVE, player->mo->scale); // Distance you have to be above the ground in order to teeter.

	if (player->mo->standingslope && player->mo->standingslope->zdelta >= (FRACUNIT/2)) // Always teeter if the slope is too steep.
		teeter = true;
	else // Let's do some checks...
	{
		UINT8 i;
		sector_t *sec;
		fixed_t highestceilingheight = INT32_MIN;
		fixed_t lowestfloorheight = INT32_MAX;

		teeter = false;
		roverfloor = false;
		for (i = 0; i < 4; i++)
		{
			ffloor_t *rover;

#define xsign ((i & 1) ? -1 : 1) // 0 -> 1 | 1 -> -1 | 2 -> 1 | 3 -> -1
#define ysign ((i & 2) ? 1 : -1) // 0 -> 1 | 1 -> 1 | 2 -> -1 | 3 -> -1
			fixed_t checkx = player->mo->x + (xsign*FixedMul(5*FRACUNIT, player->mo->scale));
			fixed_t checky = player->mo->y + (ysign*FixedMul(5*FRACUNIT, player->mo->scale));
#undef xsign
#undef ysign

			sec = R_PointInSubsector(checkx, checky)->sector;

			ceilingheight = P_GetSectorCeilingZAt(sec, checkx, checky);
			floorheight   = P_GetSectorFloorZAt  (sec, checkx, checky);
			highestceilingheight = (ceilingheight > highestceilingheight) ? ceilingheight : highestceilingheight;
			lowestfloorheight = (floorheight < lowestfloorheight) ? floorheight : lowestfloorheight;

			if (!(sec->ffloors))
				continue; // move on to the next subsector

			for (rover = sec->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS)) continue;

				topheight    = P_GetFFloorTopZAt   (rover, player->mo->x, player->mo->y);
				bottomheight = P_GetFFloorBottomZAt(rover, player->mo->x, player->mo->y);

				if (P_CheckSolidLava(rover))
					;
				else if (!(rover->flags & FF_BLOCKPLAYER || rover->flags & FF_QUICKSAND))
					continue; // intangible 3d floor

				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					if (bottomheight > ceilingheight) // Above the ceiling
						continue;

					if (bottomheight > player->mo->z + player->mo->height + tiptop
						|| (topheight < player->mo->z
						&& player->mo->z + player->mo->height < ceilingheight - tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
				else
				{
					if (topheight < floorheight) // Below the floor
						continue;

					if (topheight < player->mo->z - tiptop
						|| (bottomheight > player->mo->z + player->mo->height
						&& player->mo->z > floorheight + tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
			}
			break; // break out of loop now, we're done
		}

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if (!teeter && !roverfloor && (highestceilingheight > player->mo->ceilingz + tiptop))
					teeter = true;
		}
		else
		{
			if (!teeter && !roverfloor && (lowestfloorheight < player->mo->floorz - tiptop))
					teeter = true;
		}
	}

	{
		INT32 bx, by, xl, xh, yl, yh;

		yh = (unsigned)(player->mo->y + player->mo->radius - bmaporgy)>>MAPBLOCKSHIFT;
		yl = (unsigned)(player->mo->y - player->mo->radius - bmaporgy)>>MAPBLOCKSHIFT;
		xh = (unsigned)(player->mo->x + player->mo->radius - bmaporgx)>>MAPBLOCKSHIFT;
		xl = (unsigned)(player->mo->x - player->mo->radius - bmaporgx)>>MAPBLOCKSHIFT;

		BMBOUNDFIX(xl, xh, yl, yh);

	// Polyobjects
		validcount++;

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
						fixed_t polytop, polybottom;

						po->validcount = validcount;

						if (!(po->flags & POF_SOLID))
						{
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}

						if (!P_MobjInsidePolyobj(po, player->mo))
						{
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}

						// We're inside it! Yess...
						polysec = po->lines[0]->backsector;

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

						if (player->mo->eflags & MFE_VERTICALFLIP)
						{
							if (polybottom > player->mo->ceilingz) // Above the ceiling
							{
								plink = (polymaplink_t *)(plink->link.next);
								continue;
							}

							if (polybottom > player->mo->z + player->mo->height + tiptop
							|| (polytop < player->mo->z
							&& player->mo->z + player->mo->height < player->mo->ceilingz - tiptop))
								teeter = true;
							else
							{
								teeter = false;
								break;
							}
						}
						else
						{
							if (polytop < player->mo->floorz) // Below the floor
							{
								plink = (polymaplink_t *)(plink->link.next);
								continue;
							}

							if (polytop < player->mo->z - tiptop
							|| (polybottom > player->mo->z + player->mo->height
							&& player->mo->z > player->mo->floorz + tiptop))
								teeter = true;
							else
							{
								teeter = false;
								break;
							}
						}
					}
					plink = (polymaplink_t *)(plink->link.next);
				}
			}
		if (teeter) // only bother with objects as a last resort if you were already teetering
		{
			mobj_t *oldtmthing = tmthing;
			teeterer = player->mo;
			P_SetTarget(&tmthing, teeterer);
			teeterxl = teeterxh = player->mo->x;
			teeteryl = teeteryh = player->mo->y;
			couldteeter = false;
			solidteeter = teeter;
			for (by = yl; by <= yh; by++)
				for (bx = xl; bx <= xh; bx++)
				{
					highesttop = INT32_MIN;
					if (!P_BlockThingsIterator(bx, by, PIT_CheckSolidsTeeter))
						goto teeterdone; // we've found something that stops us teetering at all, how about we stop already
				}
teeterdone:
			teeter = solidteeter;
			P_SetTarget(&tmthing, oldtmthing); // restore old tmthing, goodness knows what the game does with this before mobj thinkers
		}
	}
	if (teeter)
	{
		if (player->panim == PA_IDLE)
			P_SetPlayerMobjState(player->mo, S_PLAY_EDGE);
	}
	else if (player->panim == PA_EDGE)
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);
}

//
// P_SetWeaponDelay
//
// Sets weapon delay.  Properly accounts for Knux's firing rate bonus.
//
static void P_SetWeaponDelay(player_t *player, INT32 delay)
{
	player->weapondelay = delay;

	if (player->skin == 2) // Knuckles
	{
		// Multiply before dividing.
		// Loss of precision can make a surprisingly large difference.
		player->weapondelay *= 2;
		player->weapondelay /= 3;
	}
}

//
// P_DrainWeaponAmmo
//
// Reduces rings and weapon ammo. Also penalizes the player
// for using weapon rings with no normal rings! >:V
//
static void P_DrainWeaponAmmo (player_t *player, INT32 power)
{
	player->powers[power]--;

	if (player->rings < 1)
	{
		player->ammoremovalweapon = player->currentweapon;
		player->ammoremovaltimer  = ammoremovaltics;

		if (player->powers[power] > 0) // can't take a ring that doesn't exist
		{
			player->powers[power]--;
			player->ammoremoval = 2;
		}
		else
			player->ammoremoval = 1;
	}
	else
		player->rings--;
}

//
// P_DoFiring()
//
// Handles firing ring weapons and Mario fireballs.
//
static void P_DoFiring(player_t *player, ticcmd_t *cmd)
{
	INT32 i;
	mobj_t *mo = NULL;

	I_Assert(player != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (!(cmd->buttons & (BT_ATTACK|BT_FIRENORMAL)))
	{
		// Not holding any firing buttons anymore.
		// Release the grenade / whatever.
		player->pflags &= ~PF_ATTACKDOWN;
		return;
	}

	if (player->pflags & PF_ATTACKDOWN || player->climbing || (G_TagGametype() && !(player->pflags & PF_TAGIT)))
		return;

	if (((player->powers[pw_shield] & SH_STACK) == SH_FIREFLOWER) && !(player->weapondelay))
	{
		player->pflags |= PF_ATTACKDOWN;
		mo = P_SpawnPlayerMissile(player->mo, MT_FIREBALL, 0);
		if (mo)
			P_InstaThrust(mo, player->mo->angle, ((mo->info->speed>>FRACBITS)*player->mo->scale) + player->speed);
		S_StartSound(player->mo, sfx_mario7);
		P_SetWeaponDelay(player, TICRATE); // Short delay between fireballs so you can't spam them everywhere
		return;
	}

	if (!G_RingSlingerGametype() || player->weapondelay)
		return;

	player->pflags |= PF_ATTACKDOWN;

	if (cmd->buttons & BT_FIRENORMAL) // No powers, just a regular ring.
		goto firenormal; //code repetition sucks.
	// Bounce ring
	else if (player->currentweapon == WEP_BOUNCE && player->powers[pw_bouncering])
	{
		P_DrainWeaponAmmo(player, pw_bouncering);
		P_SetWeaponDelay(player, TICRATE/4);

		mo = P_SpawnPlayerMissile(player->mo, MT_THROWNBOUNCE, MF2_BOUNCERING);

		if (mo)
			mo->fuse = 3*TICRATE; // Bounce Ring time
	}
	// Rail ring
	else if (player->currentweapon == WEP_RAIL && player->powers[pw_railring])
	{
		P_DrainWeaponAmmo(player, pw_railring);
		P_SetWeaponDelay(player, (3*TICRATE)/2);

		mo = P_SpawnPlayerMissile(player->mo, MT_REDRING, MF2_RAILRING|MF2_DONTDRAW);

		// Rail has no unique thrown object, therefore its sound plays here.
		S_StartSound(player->mo, sfx_rail1);
	}
	// Automatic
	else if (player->currentweapon == WEP_AUTO && player->powers[pw_automaticring])
	{
		P_DrainWeaponAmmo(player, pw_automaticring);
		player->pflags &= ~PF_ATTACKDOWN;
		P_SetWeaponDelay(player, 2);

		mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATIC, MF2_AUTOMATIC);
	}
	// Explosion
	else if (player->currentweapon == WEP_EXPLODE && player->powers[pw_explosionring])
	{
		P_DrainWeaponAmmo(player, pw_explosionring);
		P_SetWeaponDelay(player, (3*TICRATE)/2);

		mo = P_SpawnPlayerMissile(player->mo, MT_THROWNEXPLOSION, MF2_EXPLOSION);
	}
	// Grenade
	else if (player->currentweapon == WEP_GRENADE && player->powers[pw_grenadering])
	{
		P_DrainWeaponAmmo(player, pw_grenadering);
		P_SetWeaponDelay(player, TICRATE/3);

		mo = P_SpawnPlayerMissile(player->mo, MT_THROWNGRENADE, MF2_EXPLOSION);

		if (mo)
		{
			//P_InstaThrust(mo, player->mo->angle, FixedMul(mo->info->speed, player->mo->scale));
			mo->fuse = mo->info->reactiontime;
		}
	}
	// Scatter
	// Note: Ignores MF2_RAILRING
	else if (player->currentweapon == WEP_SCATTER && player->powers[pw_scatterring])
	{
		fixed_t oldz = player->mo->z;
		angle_t shotangle = player->mo->angle;
		angle_t oldaiming = player->aiming;

		P_DrainWeaponAmmo(player, pw_scatterring);
		P_SetWeaponDelay(player, (2*TICRATE)/3);

		// Center
		mo = P_SpawnPlayerMissile(player->mo, MT_THROWNSCATTER, MF2_SCATTER);
		if (mo)
			shotangle = R_PointToAngle2(player->mo->x, player->mo->y, mo->x, mo->y);

		// Left
		mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle-ANG2, true, MF2_SCATTER);

		// Right
		mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle+ANG2, true, MF2_SCATTER);

		// Down
		player->mo->z += FixedMul(12*FRACUNIT, player->mo->scale);
		player->aiming += ANG1;
		mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle, true, MF2_SCATTER);

		// Up
		player->mo->z -= FixedMul(24*FRACUNIT, player->mo->scale);
		player->aiming -= ANG2;
		mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle, true, MF2_SCATTER);

		player->mo->z = oldz;
		player->aiming = oldaiming;
		return;
	}
	// No powers, just a regular ring.
	else
	{
firenormal:
		// Infinity ring was selected.
		// Mystic wants this ONLY to happen specifically if it's selected,
		// and to not be able to get around it EITHER WAY with firenormal.

		// Infinity Ring
		if (player->currentweapon == 0
		&& player->powers[pw_infinityring])
		{
			P_SetWeaponDelay(player, TICRATE/4);

			mo = P_SpawnPlayerMissile(player->mo, MT_THROWNINFINITY, 0);

			player->powers[pw_infinityring]--;
		}
		// Red Ring
		else
		{
			if (player->rings <= 0)
				return;
			P_SetWeaponDelay(player, TICRATE/4);

			mo = P_SpawnPlayerMissile(player->mo, MT_REDRING, 0);

			if (mo)
				P_ColorTeamMissile(mo, player);

			player->rings--;
		}
	}

	if (mo)
	{
		if (mo->flags & MF_MISSILE && mo->flags2 & MF2_RAILRING)
		{
			const boolean nblockmap = !(mo->flags & MF_NOBLOCKMAP);
			for (i = 0; i < 256; i++)
			{
				if (nblockmap)
				{
					P_UnsetThingPosition(mo);
					mo->flags |= MF_NOBLOCKMAP;
					P_SetThingPosition(mo);
				}

				if (i&1)
					P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

				if (P_RailThinker(mo))
					break; // mobj was removed (missile hit a wall) or couldn't move
			}

			// Other rail sound plays at contact point.
			S_StartSound(mo, sfx_rail2);
		}
	}
}

//
// P_DoSuperStuff()
//
// Handle related superform functionality.
//
static void P_DoSuperStuff(player_t *player)
{
	mobj_t *spark;
	ticcmd_t *cmd = &player->cmd;
	if (player->mo->state >= &states[S_PLAY_SUPER_TRANS1]
	&& player->mo->state < &states[S_PLAY_SUPER_TRANS6])
		return; // don't do anything right now, we're in the middle of transforming!

	if (player->powers[pw_carry] == CR_NIGHTSMODE)
		return; // NiGHTS Super doesn't mix with normal super

	if (player->powers[pw_super])
	{
		// If you're super and not Sonic, de-superize!
		if (!(ALL7EMERALDS(emeralds) && player->charflags & SF_SUPER))
		{
			player->powers[pw_super] = 0;
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
			if (P_IsLocalPlayer(player))
			{
				music_stack_noposition = true; // HACK: Do not reposition next music
				music_stack_fadeout = MUSICRATE/2; // HACK: Fade out current music
			}
			P_RestoreMusic(player);
			P_SpawnShieldOrb(player);

			// Restore color
			if ((player->powers[pw_shield] & SH_STACK) == SH_FIREFLOWER)
			{
				player->mo->color = SKINCOLOR_WHITE;
				G_GhostAddColor(GHC_FIREFLOWER);
			}
			else
			{
				player->mo->color = player->skincolor;
				G_GhostAddColor(GHC_NORMAL);
			}

			if (!G_CoopGametype())
			{
				HU_SetCEchoFlags(0);
				HU_SetCEchoDuration(5);
				HU_DoCEcho(va("%s\\is no longer super.\\\\\\\\", player_names[player-players]));
			}
			return;
		}

		player->mo->color = (player->pflags & PF_GODMODE && cv_debug == 0)
		? (SKINCOLOR_SUPERSILVER1 + 5*(((signed)leveltime >> 1) % 7)) // A wholesome easter egg.
		: skins[player->skin].supercolor + abs( ( (player->powers[pw_super] >> 1) % 9) - 4); // This is where super flashing is handled.

		G_GhostAddColor(GHC_SUPER);

		if (player->mo->state == &states[S_PLAY_SUPER_TRANS6]) // stop here for now
			return;

		// Deplete one ring every second while super
		if ((leveltime % TICRATE == 0) && !(player->exiting))
			player->rings--;

		if ((cmd->forwardmove != 0 || cmd->sidemove != 0 || player->powers[pw_carry])
		&& !(leveltime % TICRATE) && (player->mo->momx || player->mo->momy))
		{
			spark = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SUPERSPARK);
			spark->destscale = player->mo->scale;
			P_SetScale(spark, player->mo->scale);
		}

		// Ran out of rings while super!
		if (player->rings <= 0 || player->exiting)
		{
			player->powers[pw_emeralds] = 0; // lost the power stones
			P_SpawnGhostMobj(player->mo);

			player->powers[pw_super] = 0;

			// Restore color
			if ((player->powers[pw_shield] & SH_STACK) == SH_FIREFLOWER)
			{
				player->mo->color = SKINCOLOR_WHITE;
				G_GhostAddColor(GHC_FIREFLOWER);
			}
			else
			{
				player->mo->color = player->skincolor;
				G_GhostAddColor(GHC_NORMAL);
			}

			if (!G_CoopGametype())
				player->powers[pw_flashing] = flashingtics-1;

			if (player->mo->sprite2 & FF_SPR2SUPER)
				P_SetPlayerMobjState(player->mo, player->mo->state-states);

			// Inform the netgame that the champion has fallen in the heat of battle.
			if (!G_CoopGametype())
			{
				S_StartSound(NULL, sfx_s3k66); //let all players hear it.
				HU_SetCEchoFlags(0);
				HU_SetCEchoDuration(5);
				HU_DoCEcho(va("%s\\is no longer super.\\\\\\\\", player_names[player-players]));
			}

			// Resume normal music if you're the console player
			if (P_IsLocalPlayer(player))
			{
				music_stack_noposition = true; // HACK: Do not reposition next music
				music_stack_fadeout = MUSICRATE/2; // HACK: Fade out current music
			}
			P_RestoreMusic(player);

			// If you had a shield, restore its visual significance.
			P_SpawnShieldOrb(player);
		}
	}
}

//
// P_SuperReady
//
// Returns true if player is ready to turn super, duh
//
boolean P_SuperReady(player_t *player)
{
	if (!player->powers[pw_super]
	&& !player->powers[pw_invulnerability]
	&& !player->powers[pw_tailsfly]
	&& (player->charflags & SF_SUPER)
	&& (player->pflags & PF_JUMPED)
	&& !(player->powers[pw_shield] & SH_NOSTACK)
	&& !(maptol & TOL_NIGHTS)
	&& ALL7EMERALDS(emeralds)
	&& (player->rings >= 50))
		return true;

	return false;
}

//
// P_DoJump
//
// Jump routine for the player
//
void P_DoJump(player_t *player, boolean soundandstate)
{
	fixed_t factor;
	const fixed_t dist6 = FixedMul(FixedDiv(player->speed, player->mo->scale), player->actionspd)/20;

	if (player->pflags & PF_JUMPSTASIS)
		return;

	if (!player->jumpfactor)
		return;

	if (player->climbing)
	{
		// Jump this high.
		if (player->powers[pw_super])
			player->mo->momz = 5*FRACUNIT;
		else if (player->mo->eflags & MFE_UNDERWATER)
			player->mo->momz = 2*FRACUNIT;
		else
			player->mo->momz = 15*(FRACUNIT/4);

		player->drawangle = player->mo->angle = player->mo->angle - ANGLE_180; // Turn around from the wall you were climbing.

		if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
			P_SetPlayerAngle(player, player->mo->angle);

		player->climbing = 0; // Stop climbing, duh!
		P_InstaThrust(player->mo, player->mo->angle, FixedMul(6*FRACUNIT, player->mo->scale)); // Jump off the wall.
	}
	// Quicksand jumping.
	else if (P_InQuicksand(player->mo))
	{
		if (player->mo->ceilingz-player->mo->floorz <= player->mo->height-1)
			return;
		player->mo->momz += (39*(FRACUNIT/4))>>1;
		if (player->mo->momz >= 6*FRACUNIT)
			player->mo->momz = 6*FRACUNIT; //max momz in quicksand
		else if (player->mo->momz < 0) // still descending?
			player->mo->momz = (39*(FRACUNIT/4))>>1; // just default to the jump height.
	}
	else if (!(player->pflags & PF_JUMPED)) // Jump
	{
		if (player->mo->ceilingz-player->mo->floorz <= player->mo->height-1)
			return;

		if (player->powers[pw_carry] == CR_PTERABYTE)
		{
			S_StartSound(player->mo, sfx_s3kd7s);
			player->mo->tracer->cusval += 10;
			player->mo->tracer->watertop = P_RandomRange(-player->mo->tracer->cusval, player->mo->tracer->cusval) << (FRACBITS - 1);
			player->mo->tracer->waterbottom = P_RandomRange(-player->mo->tracer->cusval, player->mo->tracer->cusval) << (FRACBITS - 1);
			player->mo->tracer->cvmem = P_RandomRange(-player->mo->tracer->cusval, player->mo->tracer->cusval) << (FRACBITS - 1);
			return;
		}

		// Jump this high.
		if (player->powers[pw_carry] == CR_PLAYER)
		{
			player->mo->momz = 9*FRACUNIT;
			player->powers[pw_carry] = CR_NONE;
			P_SetTarget(&player->mo->tracer, NULL);
			if (player-players == consoleplayer && botingame)
				CV_SetValue(&cv_analog[1], true);
		}
		else if (player->powers[pw_carry] == CR_GENERIC)
		{
			player->mo->momz = 9*FRACUNIT;
			player->powers[pw_carry] = CR_NONE;
			if (!(player->mo->tracer->flags & MF_MISSILE)) // Missiles remember their owner!
				P_SetTarget(&player->mo->tracer->target, NULL);
			P_SetTarget(&player->mo->tracer, NULL);
		}
		else if (player->powers[pw_carry] == CR_ROPEHANG)
		{
			player->mo->momz = 12*FRACUNIT;
			player->powers[pw_carry] = CR_NONE;
			P_SetTarget(&player->mo->tracer, NULL);
		}
		else if (player->powers[pw_carry] == CR_ROLLOUT)
		{
			player->mo->momz = 9*FRACUNIT;
			if (player->mo->tracer)
			{
				if (P_MobjFlip(player->mo->tracer)*player->mo->tracer->momz > 0)
					player->mo->momz += player->mo->tracer->momz;
				if (!P_IsObjectOnGround(player->mo->tracer))
					P_SetObjectMomZ(player->mo->tracer, -9*FRACUNIT, true);
				player->mo->tracer->flags |= MF_PUSHABLE;
				P_SetTarget(&player->mo->tracer->tracer, NULL);
			}
			player->powers[pw_carry] = CR_NONE;
			P_SetTarget(&player->mo->tracer, NULL);
		}
		else if (player->mo->eflags & MFE_GOOWATER)
		{
			player->mo->momz = 7*FRACUNIT;
			if (player->charability == CA_JUMPBOOST && onground)
			{
				if (player->charflags & SF_MULTIABILITY)
					player->mo->momz += FixedMul(FRACUNIT/4, dist6);
				else
					player->mo->momz += FixedMul(FRACUNIT/8, dist6);
			}
		}
		else if (maptol & TOL_NIGHTS)
			player->mo->momz = 18*FRACUNIT;
		else if (player->powers[pw_super] && !(player->charflags & SF_NOSUPERJUMPBOOST))
		{
			player->mo->momz = 13*FRACUNIT;

			// Add a boost for super characters with float/slowfall and multiability.
			if (player->charability == CA_JUMPBOOST)
			{
				if (player->charflags & SF_MULTIABILITY)
					player->mo->momz += FixedMul(FRACUNIT/4, dist6);
				else
					player->mo->momz += FixedMul(FRACUNIT/8, dist6);
			}
		}
		else
		{
			player->mo->momz = 39*(FRACUNIT/4); // Default jump momentum.
			if (player->charability == CA_JUMPBOOST && onground)
			{
				if (player->charflags & SF_MULTIABILITY)
					player->mo->momz += FixedMul(FRACUNIT/4, dist6);
				else
					player->mo->momz += FixedMul(FRACUNIT/8, dist6);
			}
		}

		// Reduce player momz by 58.5% when underwater.
		if (player->mo->eflags & MFE_UNDERWATER)
			player->mo->momz = FixedMul(player->mo->momz, FixedDiv(117*FRACUNIT, 200*FRACUNIT));

		player->pflags |= PF_STARTJUMP;
	}

	factor = player->jumpfactor;

	if (twodlevel || (player->mo->flags2 & MF2_TWOD))
		factor += player->jumpfactor / 10;

	if (player->charflags & SF_MULTIABILITY && player->charability == CA_DOUBLEJUMP)
		factor -= max(0, player->secondjump * player->jumpfactor / ((player->actionspd >> FRACBITS) + 1)); // Reduce the jump height each time

	//if (maptol & TOL_NIGHTS)
	//	factor = player->jumpfactor; // all skins jump the same. if you nerf jumping abilities, you may want this.

	P_SetObjectMomZ(player->mo, FixedMul(factor, player->mo->momz), false); // Custom height

	// set just an eensy above the ground
	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		player->mo->z--;
		if (player->mo->pmomz < 0)
			player->mo->momz += player->mo->pmomz; // Add the platform's momentum to your jump.
		player->mo->pmomz = 0;
	}
	else
	{
		player->mo->z++;
		if (player->mo->pmomz > 0)
			player->mo->momz += player->mo->pmomz; // Add the platform's momentum to your jump.
		player->mo->pmomz = 0;
	}
	player->mo->eflags &= ~MFE_APPLYPMOMZ;

	player->pflags |= P_GetJumpFlags(player);;

	if (soundandstate)
	{
		if (!player->spectator)
			S_StartSound(player->mo, sfx_jump); // Play jump sound!

		P_SetPlayerMobjState(player->mo, S_PLAY_JUMP);
	}
}

static void P_DoSpinDashDust(player_t *player)
{
	UINT32 i;
	mobj_t *particle;
	INT32 prandom[3];
	for (i = 0; i <= (leveltime%7)/2; i++) { // 1, 2, 3 or 4 particles
		particle = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_SPINDUST);

		if (player->mo->eflags & (MFE_TOUCHWATER|MFE_UNDERWATER)) // overrides fire version
			P_SetMobjState(particle, S_SPINDUST_BUBBLE1);
		else if (player->powers[pw_shield] == SH_ELEMENTAL)
			P_SetMobjState(particle, S_SPINDUST_FIRE1);

		P_SetTarget(&particle->target, player->mo);
		particle->destscale = (2*player->mo->scale)/3;
		P_SetScale(particle, particle->destscale);
		if (player->mo->eflags & MFE_VERTICALFLIP) // readjust z position if needed
			particle->z = player->mo->z + player->mo->height - particle->height;
		prandom[0] = P_RandomFixed()<<2; // P_RandomByte()<<10
		prandom[1] = P_RandomRange(-30, 30); // P_RandomRange(-ANG30/FRACUNIT, ANG30/FRACUNIT)*FRACUNIT
		prandom[2] = P_RandomFixed()<<3; // P_RandomByte()<<11
		P_SetObjectMomZ(particle, player->dashspeed/50 + prandom[0], false);
		P_InstaThrust(particle,
				player->drawangle + (prandom[1]*ANG1),
				-FixedMul(player->dashspeed/12 + FRACUNIT + prandom[2], player->mo->scale));
		P_TryMove(particle, particle->x+particle->momx, particle->y+particle->momy, true);
	}
}

//
// P_DoSpinAbility
//
// Player spindash handling
//
static void P_DoSpinAbility(player_t *player, ticcmd_t *cmd)
{
	boolean canstand = true; // can we stand on the ground? (mostly relevant for slopes)
	if (player->pflags & PF_STASIS
		&& (player->pflags & PF_JUMPSTASIS || player->mo->state-states != S_PLAY_GLIDE_LANDING))
		return;

	if (cmd->buttons & BT_USE)
	{
		if (LUAh_SpinSpecial(player))
			return;
	}

	canstand = (!player->mo->standingslope || (player->mo->standingslope->flags & SL_NOPHYSICS) || abs(player->mo->standingslope->zdelta) < FRACUNIT/2);

	///////////////////////////////
	// ability-specific behavior //
	///////////////////////////////
	if (!(player->pflags & PF_SLIDING) && !player->exiting && !P_PlayerInPain(player))
	{
		switch (player->charability2)
		{
			case CA2_SPINDASH: // Spinning and Spindashing
				 // Start revving
				if ((cmd->buttons & BT_USE) && (player->speed < FixedMul(5<<FRACBITS, player->mo->scale) || player->mo->state - states == S_PLAY_GLIDE_LANDING)
					&& !player->mo->momz && onground && !(player->pflags & (PF_USEDOWN|PF_SPINNING))
						&& canstand)
				{
					player->mo->momx = player->cmomx;
					player->mo->momy = player->cmomy;
					player->pflags |= (PF_USEDOWN|PF_STARTDASH|PF_SPINNING);
					player->dashspeed = player->mindash;
					P_SetPlayerMobjState(player->mo, S_PLAY_SPINDASH);
					if (!player->spectator)
						S_StartSound(player->mo, sfx_spndsh); // Make the rev sound!
				}
				 // Revving
				else if ((cmd->buttons & BT_USE) && (player->pflags & PF_STARTDASH))
				{
					if (player->speed > 5*player->mo->scale)
					{
						player->pflags &= ~PF_STARTDASH;
						P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
						S_StartSound(player->mo, sfx_spin);
						break;
					}
					if (player->dashspeed < player->mindash)
						player->dashspeed = player->mindash;

					if (player->dashspeed > player->maxdash)
						player->dashspeed = player->maxdash;

					if (player->dashspeed < player->maxdash && player->mindash != player->maxdash)
					{
#define chargecalculation (6*(player->dashspeed - player->mindash))/(player->maxdash - player->mindash)
						fixed_t soundcalculation = chargecalculation;
						player->dashspeed += FRACUNIT;
						if (!player->spectator && soundcalculation != chargecalculation)
							S_StartSound(player->mo, sfx_spndsh); // Make the rev sound!
#undef chargecalculation
					}
					if (player->revitem && !(leveltime % 5)) // Now spawn the color thok circle.
					{
						P_SpawnSpinMobj(player, player->revitem);
						G_GhostAddRev();
					}
				}
				// If not moving up or down, and travelling faster than a speed of five while not holding
				// down the spin button and not spinning.
				// AKA Just go into a spin on the ground, you idiot. ;)
				else if ((cmd->buttons & BT_USE || ((twodlevel || (player->mo->flags2 & MF2_TWOD)) && cmd->forwardmove < -20))
					&& !player->climbing && !player->mo->momz && onground && (player->speed > FixedMul(5<<FRACBITS, player->mo->scale)
						|| !canstand) && !(player->pflags & (PF_USEDOWN|PF_SPINNING)))
				{
					player->pflags |= (PF_USEDOWN|PF_SPINNING);
					P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
					if (!player->spectator)
						S_StartSound(player->mo, sfx_spin);
				}
				else
				// Catapult the player from a spindash rev!
				if (onground && !(player->pflags & PF_USEDOWN) && (player->pflags & PF_STARTDASH) && (player->pflags & PF_SPINNING))
				{
					player->pflags &= ~PF_STARTDASH;
					if (player->powers[pw_carry] == CR_BRAKGOOP)
						player->dashspeed = 0;

					if (!((gametyperules & GTR_RACE) && leveltime < 4*TICRATE))
					{
						if (player->dashspeed)
						{
							P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
							P_InstaThrust(player->mo, player->mo->angle, (player->speed = FixedMul(player->dashspeed, player->mo->scale))); // catapult forward ho!!
						}
						else
						{
							P_SetPlayerMobjState(player->mo, S_PLAY_STND);
							player->pflags &= ~PF_SPINNING;
						}

						if (!player->spectator)
							S_StartSound(player->mo, sfx_zoom);
					}

					player->dashspeed = 0;
				}
				break;
			case CA2_GUNSLINGER:
				if (!player->mo->momz && onground && !player->weapondelay && canstand)
				{
					if (player->speed > FixedMul(10<<FRACBITS, player->mo->scale))
					{}
					else
					{
						mobj_t *lockon = P_LookForEnemies(player, false, true);
						if (lockon)
						{
							if (P_IsLocalPlayer(player)) // Only display it on your own view.
							{
								mobj_t *visual = P_SpawnMobj(lockon->x, lockon->y, lockon->z, MT_LOCKON); // positioning, flip handled in P_SceneryThinker
								P_SetTarget(&visual->target, lockon);
							}
						}
						if ((cmd->buttons & BT_USE) && !(player->pflags & PF_USEDOWN))
						{
							mobj_t *bullet;

							P_SetPlayerMobjState(player->mo, S_PLAY_FIRE);

#define zpos(posmo) (posmo->z + (posmo->height - mobjinfo[player->revitem].height)/2)
							if (lockon)
							{
								player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, lockon->x, lockon->y);
								bullet = P_SpawnPointMissile(player->mo, lockon->x, lockon->y, zpos(lockon), player->revitem, player->mo->x, player->mo->y, zpos(player->mo));
								if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
									P_SetPlayerAngle(player, player->mo->angle);
							}
							else
							{
								bullet = P_SpawnPointMissile(player->mo, player->mo->x + P_ReturnThrustX(NULL, player->mo->angle, FRACUNIT), player->mo->y + P_ReturnThrustY(NULL, player->mo->angle, FRACUNIT), zpos(player->mo), player->revitem, player->mo->x, player->mo->y, zpos(player->mo));
								if (bullet)
								{
									bullet->flags &= ~MF_NOGRAVITY;
									bullet->momx >>= 1;
									bullet->momy >>= 1;
								}
							}
							player->drawangle = player->mo->angle;
#undef zpos

							player->mo->momx >>= 1;
							player->mo->momy >>= 1;
							player->pflags |= PF_USEDOWN;
							P_SetWeaponDelay(player, TICRATE/2);
						}
					}
				}
				break;
			case CA2_MELEE: // Melee attack
				if (player->panim != PA_ABILITY2 && (cmd->buttons & BT_USE)
				&& !player->mo->momz && onground && !(player->pflags & PF_USEDOWN)
				&& canstand)
				{
					P_ResetPlayer(player);
					player->pflags |= PF_THOKKED;
#if 0
					if ((player->charability == CA_TWINSPIN) && (player->speed > FixedMul(player->runspeed, player->mo->scale)))
					{
						P_DoJump(player, false);
						player->pflags &= ~PF_STARTJUMP;
						player->mo->momz = FixedMul(player->mo->momz, 3*FRACUNIT/2); // NOT 1.5 times the jump height, but 2.25 times.
						P_SetPlayerMobjState(player->mo, S_PLAY_TWINSPIN);
						S_StartSound(player->mo, sfx_s3k8b);
					}
					else
#endif
					{
						player->mo->z += P_MobjFlip(player->mo);
						P_SetObjectMomZ(player->mo, player->mindash, false);
						if (P_MobjFlip(player->mo)*player->mo->pmomz > 0)
							player->mo->momz += player->mo->pmomz; // Add the platform's momentum to your jump.
						else
							player->mo->pmomz = 0;
						if (player->mo->eflags & MFE_UNDERWATER)
							player->mo->momz >>= 1;
#if 0
						if (FixedMul(player->speed, FINECOSINE(((player->mo->angle - R_PointToAngle2(0, 0, player->rmomx, player->rmomy)) >> ANGLETOFINESHIFT) & FINEMASK)) < FixedMul(player->maxdash, player->mo->scale))
#else
						if (player->speed < FixedMul(player->maxdash, player->mo->scale))
#endif
						{
							if (player->panim == PA_IDLE)
								player->drawangle = player->mo->angle;
							P_InstaThrust(player->mo, player->drawangle, FixedMul(player->maxdash, player->mo->scale));
						}
						player->mo->momx += player->cmomx;
						player->mo->momy += player->cmomy;
						P_SetPlayerMobjState(player->mo, S_PLAY_MELEE);
						S_StartSound(player->mo, sfx_s3k42);
					}
					player->pflags |= PF_USEDOWN;
				}
				break;
		}
	}

	///////////////////////////////
	// general spinning behavior //
	///////////////////////////////

	// Rolling normally
	if (onground && player->pflags & PF_SPINNING && !(player->pflags & PF_STARTDASH)
		&& player->speed < 5*player->mo->scale && canstand)
	{
		if (GETSECSPECIAL(player->mo->subsector->sector->special, 4) == 7 || (player->mo->ceilingz - player->mo->floorz < P_GetPlayerHeight(player)))
			P_InstaThrust(player->mo, player->mo->angle, 10*player->mo->scale);
		else
		{
			player->skidtime = 0;
			player->pflags &= ~PF_SPINNING;
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
			player->mo->momx = player->cmomx;
			player->mo->momy = player->cmomy;
		}
	}

	if (onground && player->pflags & PF_STARTDASH)
	{
		// Spawn spin dash dust
		if (!(player->charflags & SF_NOSPINDASHDUST) && !(player->mo->eflags & MFE_GOOWATER))
			P_DoSpinDashDust(player);
	}
}

//
// P_DoJumpShield
//
// Jump Shield Activation
//
void P_DoJumpShield(player_t *player)
{
	boolean electric = ((player->powers[pw_shield] & SH_PROTECTELECTRIC) == SH_PROTECTELECTRIC);

	if (player->pflags & PF_THOKKED)
		return;

	player->pflags &= ~PF_JUMPED;
	P_DoJump(player, false);
	player->secondjump = 0;
	player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
	player->pflags &= ~(PF_STARTJUMP|PF_SPINNING|PF_BOUNCING);
	if (electric)
	{
		mobj_t *spark;
		INT32 i;
#define numangles 6
#define limitangle (360/numangles)
		const angle_t travelangle = player->mo->angle + P_RandomRange(-limitangle, limitangle)*ANG1;
		for (i = 0; i < numangles; i++)
		{
			spark = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_THUNDERCOIN_SPARK);
			P_InstaThrust(spark, travelangle + i*(ANGLE_MAX/numangles), FixedMul(4*FRACUNIT, spark->scale));
			if (i % 2)
				P_SetObjectMomZ(spark, -4*FRACUNIT, false);
			spark->fuse = 18;
		}
#undef limitangle
#undef numangles
		S_StartSound(player->mo, sfx_s3k45);
	}
	else
	{
		player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE);
		P_SetPlayerMobjState(player->mo, S_PLAY_FALL);
		S_StartSound(player->mo, sfx_wdjump);
	}
}

//
// P_DoBubbleBounce
//
// Bubblewrap shield landing handling
//
void P_DoBubbleBounce(player_t *player)
{
	player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_SHIELDABILITY);
	S_StartSound(player->mo, sfx_s3k44);
	P_MobjCheckWater(player->mo);
	P_DoJump(player, false);
	if (player->charflags & SF_NOJUMPSPIN)
		P_SetPlayerMobjState(player->mo, S_PLAY_FALL);
	else
		P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
	player->pflags |= PF_THOKKED;
	player->pflags &= ~PF_STARTJUMP;
	player->secondjump = UINT8_MAX;
	player->mo->momz = FixedMul(player->mo->momz, 11*FRACUNIT/8);
}

//
// P_DoAbilityBounce
//
// CA_BOUNCE landing handling
//
void P_DoAbilityBounce(player_t *player, boolean changemomz)
{
	fixed_t prevmomz;
	if (player->mo->state-states == S_PLAY_BOUNCE_LANDING)
		return;
	if (changemomz)
	{
		fixed_t minmomz;
		prevmomz = player->mo->momz;
		if (P_MobjFlip(player->mo)*prevmomz < 0)
			prevmomz = 0;
		else if (player->mo->eflags & MFE_UNDERWATER)
			prevmomz /= 2;
		P_DoJump(player, false);
		player->pflags &= ~(PF_STARTJUMP|PF_JUMPED);
		minmomz = FixedMul(player->mo->momz, 3*FRACUNIT/2);
		player->mo->momz = max(minmomz, (minmomz + prevmomz)/2);
	}
	S_StartSound(player->mo, sfx_boingf);
	P_SetPlayerMobjState(player->mo, S_PLAY_BOUNCE_LANDING);
	player->pflags |= PF_BOUNCING|PF_THOKKED;
}

//
// P_TwinSpinRejuvenate
//
// CA_TWINSPIN landing handling
//
void P_TwinSpinRejuvenate(player_t *player, mobjtype_t type)
{
	fixed_t actionspd;
	angle_t movang, ang, fa;
	fixed_t v, h;
	UINT8 i;

	if (!player->mo || !type)
		return;

	actionspd = FixedMul(player->actionspd, player->mo->scale);

	fa = (R_PointToAngle2(0, 0, player->mo->momz, FixedHypot(player->mo->momx, player->mo->momy))>>ANGLETOFINESHIFT) & FINEMASK;
	movang = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
	ang = 0;

	v = FixedMul(actionspd, FINESINE(fa));
	h = actionspd - FixedMul(actionspd, FINECOSINE(fa));

	// hearticles
	for (i = 0; i <= 7; i++)
	{
		fixed_t side = actionspd - FixedMul(h, abs(FINESINE((ang>>ANGLETOFINESHIFT) & FINEMASK)));
		fixed_t xo = P_ReturnThrustX(NULL, ang + movang, side);
		fixed_t yo = P_ReturnThrustY(NULL, ang + movang, side);
		fixed_t zo = -FixedMul(FINECOSINE(((ang>>ANGLETOFINESHIFT) & FINEMASK)), v);
		mobj_t *missile = P_SpawnMobjFromMobj(player->mo,
			xo,
			yo,
			player->mo->height/2 + zo,
			type);
		P_SetTarget(&missile->target, player->mo);
		P_SetScale(missile, (missile->destscale >>= 1));
		missile->angle = ang + movang;
		missile->fuse = TICRATE/2;
		missile->extravalue2 = (99*FRACUNIT)/100;
		missile->momx = xo;
		missile->momy = yo;
		missile->momz = zo;

		ang += ANGLE_45;
	}

	player->pflags &= ~PF_THOKKED;
}

//
// P_Telekinesis
//
// Morph's fancy stuff-moving character ability
// +ve thrust pushes away, -ve thrust pulls in
//
void P_Telekinesis(player_t *player, fixed_t thrust, fixed_t range)
{
	thinker_t *th;
	mobj_t *mo2;
	fixed_t dist = 0;
	angle_t an;

	if (player->powers[pw_super]) // increase range when super
		range *= 2;

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2 == player->mo)
			continue;

		if (!((mo2->flags & MF_SHOOTABLE && mo2->flags & MF_ENEMY) || mo2->type == MT_EGGGUARD || mo2->player))
			continue;

		dist = P_AproxDistance(P_AproxDistance(player->mo->x-mo2->x, player->mo->y-mo2->y), player->mo->z-mo2->z);

		if (range < dist)
			continue;

		if (!P_CheckSight(player->mo, mo2))
			continue; // if your psychic powers can't "see" it don't bother

		an = R_PointToAngle2(player->mo->x, player->mo->y, mo2->x, mo2->y);

		if (mo2->health > 0)
		{
			P_Thrust(mo2, an, thrust);

			if (mo2->type == MT_GOLDBUZZ || mo2->type == MT_REDBUZZ)
				mo2->tics += 8;
		}
	}

	P_SpawnThokMobj(player);
	player->pflags |= PF_THOKKED;
}

static void P_DoTwinSpin(player_t *player)
{
	player->pflags &= ~PF_NOJUMPDAMAGE;
	player->pflags |= P_GetJumpFlags(player) | PF_THOKKED;
	S_StartSound(player->mo, sfx_s3k42);
	player->mo->frame = 0;
	P_SetPlayerMobjState(player->mo, S_PLAY_TWINSPIN);
}

//
// P_DoJumpStuff
//
// Handles player jumping
//
static void P_DoJumpStuff(player_t *player, ticcmd_t *cmd)
{
	mobj_t *lockonthok = NULL, *lockonshield = NULL, *visual = NULL;

	if (player->pflags & PF_JUMPSTASIS)
		return;

	if ((player->charability == CA_HOMINGTHOK) && !player->homing && (player->pflags & PF_JUMPED) && (!(player->pflags & PF_THOKKED) || (player->charflags & SF_MULTIABILITY)) && (lockonthok = P_LookForEnemies(player, true, false)))
	{
		if (P_IsLocalPlayer(player)) // Only display it on your own view.
		{
			visual = P_SpawnMobj(lockonthok->x, lockonthok->y, lockonthok->z, MT_LOCKON); // positioning, flip handled in P_SceneryThinker
			P_SetTarget(&visual->target, lockonthok);
		}
	}

	//////////////////
	//SHIELD ACTIVES//
	//& SUPER FLOAT!//
	//////////////////

	if ((player->pflags & PF_JUMPED) && !player->exiting && !P_PlayerInPain(player))
	{
		if (onground || player->climbing || player->powers[pw_carry])
			;
		else if ((gametyperules & GTR_TEAMFLAGS) && player->gotflag)
			;
		else if (player->pflags & (PF_GLIDING|PF_SLIDING|PF_SHIELDABILITY)) // If the player has used an ability previously
			;
		else if ((player->powers[pw_shield] & SH_NOSTACK) && !player->powers[pw_super] && !(player->pflags & PF_USEDOWN)
			&& ((!(player->pflags & PF_THOKKED) || ((player->powers[pw_shield] & SH_NOSTACK) == SH_BUBBLEWRAP && player->secondjump == UINT8_MAX)))) // thokked is optional if you're bubblewrapped
		{
			if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ATTRACT)
			{
				if ((lockonshield = P_LookForEnemies(player, false, false)))
				{
					if (P_IsLocalPlayer(player)) // Only display it on your own view.
					{
						boolean dovis = true;
						if (lockonshield == lockonthok)
						{
							if (leveltime & 2)
								dovis = false;
							else if (visual)
								P_RemoveMobj(visual);
						}
						if (dovis)
						{
							visual = P_SpawnMobj(lockonshield->x, lockonshield->y, lockonshield->z, MT_LOCKON); // positioning, flip handled in P_SceneryThinker
							P_SetTarget(&visual->target, lockonshield);
							P_SetMobjStateNF(visual, visual->info->spawnstate+1);
						}
					}
				}
			}
			if (cmd->buttons & BT_USE && !LUAh_ShieldSpecial(player)) // Spin button effects
			{
				// Force stop
				if ((player->powers[pw_shield] & ~(SH_FORCEHP|SH_STACK)) == SH_FORCE)
				{
					player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
					player->mo->momx = player->mo->momy = player->mo->momz = 0;
					S_StartSound(player->mo, sfx_ngskid);
				}
				else
				{
					switch (player->powers[pw_shield] & SH_NOSTACK)
					{
						// Whirlwind jump/Thunder jump
						case SH_WHIRLWIND:
						case SH_THUNDERCOIN:
							P_DoJumpShield(player);
							break;
						// Armageddon pow
						case SH_ARMAGEDDON:
							player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
							P_BlackOw(player);
							break;
						// Attraction blast
						case SH_ATTRACT:
							player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
							player->homing = 2;
							P_SetTarget(&player->mo->target, P_SetTarget(&player->mo->tracer, lockonshield));
							if (lockonshield)
								{
									player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, lockonshield->x, lockonshield->y);
										player->pflags &= ~PF_NOJUMPDAMAGE;
										P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
										S_StartSound(player->mo, sfx_s3k40);
										player->homing = 3*TICRATE;
								}
								else
									S_StartSound(player->mo, sfx_s3ka6);
								break;
							// Elemental stomp/Bubble bounce
							case SH_ELEMENTAL:
							case SH_BUBBLEWRAP:
								{
									boolean elem = ((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL);
									player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
									if (elem)
									{
										player->mo->momx = player->mo->momy = 0;
										S_StartSound(player->mo, sfx_s3k43);
									}
									else
									{
										player->mo->momx -= (player->mo->momx/3);
										player->mo->momy -= (player->mo->momy/3);
										player->pflags &= ~PF_NOJUMPDAMAGE;
										P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
										S_StartSound(player->mo, sfx_s3k44);
									}
									player->secondjump = 0;
									P_SetObjectMomZ(player->mo, -24*FRACUNIT, false);
									break;
								}
							// Flame burst
							case SH_FLAMEAURA:
								player->pflags |= PF_THOKKED|PF_SHIELDABILITY;
								P_Thrust(player->mo, player->mo->angle, FixedMul(30*FRACUNIT - FixedSqrt(FixedDiv(player->speed, player->mo->scale)), player->mo->scale));
								player->drawangle = player->mo->angle;
								S_StartSound(player->mo, sfx_s3k43);
							default:
								break;
					}
				}
			}
		}
		else if ((cmd->buttons & BT_USE))
		{
			if (!(player->pflags & PF_USEDOWN) && P_SuperReady(player))
			{
				// If you can turn super and aren't already,
				// and you don't have a shield, do it!
				P_DoSuperTransformation(player, false);
			}
			else if (!LUAh_JumpSpinSpecial(player))
				switch (player->charability)
				{
					case CA_THOK:
						if (player->powers[pw_super]) // Super Sonic float
						{
							if ((player->speed > 5*player->mo->scale) // FixedMul(5<<FRACBITS, player->mo->scale), but scale is FRACUNIT-based
							&& (P_MobjFlip(player->mo)*player->mo->momz <= 0))
							{
								if (player->panim != PA_RUN && player->panim != PA_WALK)
								{
									if (player->speed >= FixedMul(player->runspeed, player->mo->scale))
										P_SetPlayerMobjState(player->mo, S_PLAY_FLOAT_RUN);
									else
										P_SetPlayerMobjState(player->mo, S_PLAY_FLOAT);
								}

								player->mo->momz = 0;
								player->pflags &= ~(PF_STARTJUMP|PF_SPINNING);
								player->secondjump = 1;
							}
						}
						break;
					case CA_TELEKINESIS:
						if (!(player->pflags & (PF_THOKKED|PF_USEDOWN)) || (player->charflags & SF_MULTIABILITY))
						{
							P_Telekinesis(player,
								-FixedMul(player->actionspd, player->mo->scale), // -ve thrust (pulling towards player)
								FixedMul(384*FRACUNIT, player->mo->scale));
						}
						break;
					case CA_TWINSPIN:
						if ((player->charability2 == CA2_MELEE) && (!(player->pflags & (PF_THOKKED|PF_USEDOWN)) || player->charflags & SF_MULTIABILITY))
							P_DoTwinSpin(player);
						break;
					default:
						break;
				}
		}
	}

	if (player->charability == CA_AIRDRILL)
	{
		if (player->pflags & PF_JUMPED)
		{
			if (cmd->buttons & BT_USE && player->secondjump < 42) // speed up falling down
				player->secondjump++;

			if (player->flyangle > 0 && player->pflags & PF_THOKKED)
			{
				player->flyangle--;

				P_SetObjectMomZ(player->mo, ((player->flyangle-24 - player->secondjump*3)*((player->actionspd>>FRACBITS)/12 + 1)<<FRACBITS)/7, false);

				P_SpawnThokMobj(player);

				if ((player->mo->eflags & MFE_UNDERWATER))
					P_InstaThrust(player->mo, player->mo->angle, FixedMul(player->normalspeed, player->mo->scale)*(80-player->flyangle - (player->actionspd>>FRACBITS)/2)/80);
				else
					P_InstaThrust(player->mo, player->mo->angle, ((FixedMul(player->normalspeed - player->actionspd/4, player->mo->scale))*2)/3);

				player->drawangle = player->mo->angle;
			}
		}
	}

	///////////////
	// CHARACTER //
	// ABILITIES!//
	///////////////

	if (cmd->buttons & BT_JUMP && !player->exiting && !P_PlayerInPain(player))
	{
		if (LUAh_JumpSpecial(player))
			;
		// all situations below this require jump button not to be pressed already
		else if (player->pflags & PF_JUMPDOWN)
			;
		// Jump S3&K style while in quicksand.
		else if (P_InQuicksand(player->mo))
		{
			P_DoJump(player, true);
			player->secondjump = 0;
			player->pflags &= ~PF_THOKKED;
		}
		else if (player->powers[pw_carry] == CR_MACESPIN && player->mo->tracer)
		{
			player->powers[pw_carry] = CR_NONE;
			P_SetTarget(&player->mo->tracer, NULL);
			player->powers[pw_flashing] = TICRATE/4;
		}
		// can't jump while in air, can't jump while jumping
		else if (onground || player->climbing || player->powers[pw_carry])
		{
			P_DoJump(player, true);
			player->secondjump = 0;
			player->pflags &= ~PF_THOKKED;
		}
		else if (player->pflags & PF_SLIDING || ((gametyperules & GTR_TEAMFLAGS) && player->gotflag) || player->pflags & PF_SHIELDABILITY)
			;
		/*else if (P_SuperReady(player))
		{
			// If you can turn super and aren't already,
			// and you don't have a shield, do it!
			P_DoSuperTransformation(player, false);
		}*/
		else if (player->pflags & PF_JUMPED)
		{
			if (!LUAh_AbilitySpecial(player))
			switch (player->charability)
			{
				case CA_THOK:
				case CA_HOMINGTHOK:
				case CA_JUMPTHOK: // Credit goes to CZ64 and Sryder13 for the original
					// Now it's Sonic's abilities turn!
					// THOK!
					if (!(player->pflags & PF_THOKKED) || (player->charflags & SF_MULTIABILITY))
					{
						// Catapult the player
						fixed_t actionspd = player->actionspd;

						if (player->charflags & SF_DASHMODE)
							actionspd = max(player->normalspeed, FixedDiv(player->speed, player->mo->scale));

						if (player->mo->eflags & MFE_UNDERWATER)
							actionspd >>= 1;

						if ((player->charability == CA_JUMPTHOK) && !(player->pflags & PF_THOKKED))
						{
							player->pflags &= ~PF_JUMPED;
							P_DoJump(player, false);
						}

						P_InstaThrust(player->mo, player->mo->angle, FixedMul(actionspd, player->mo->scale));

						if (maptol & TOL_2D)
						{
							player->mo->momx /= 2;
							player->mo->momy /= 2;
						}
						if (player->charability == CA_HOMINGTHOK)
						{
							player->mo->momx /= 2;
							player->mo->momy /= 2;
						}

						if (player->charability == CA_HOMINGTHOK)
						{
							P_SetTarget(&player->mo->target, P_SetTarget(&player->mo->tracer, lockonthok));
							if (lockonthok)
							{
								P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
								player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, lockonthok->x, lockonthok->y);
								player->homing = 3*TICRATE;
							}
							else
							{
								P_SetPlayerMobjState(player->mo, S_PLAY_FALL);
								player->pflags &= ~PF_JUMPED;
								player->mo->height = P_GetPlayerHeight(player);
							}
							player->pflags &= ~PF_NOJUMPDAMAGE;
						}

						player->drawangle = player->mo->angle;

						if (player->mo->info->attacksound && !player->spectator)
							S_StartSound(player->mo, player->mo->info->attacksound); // Play the THOK sound

						P_SpawnThokMobj(player);

						player->pflags &= ~(PF_SPINNING|PF_STARTDASH);
						player->pflags |= PF_THOKKED;

						// Change localangle to match for simple controls? (P.S. chalupa)
						// disabled because it seemed to disorient people and Z-targeting exists now
						/*if (!demoplayback)
						{
							if (player == &players[consoleplayer] && cv_cam_turnfacingability[0].value > 0 && !(PLAYER1INPUTDOWN(gc_turnleft) || PLAYER1INPUTDOWN(gc_turnright)))
								P_SetPlayerAngle(player, player->mo->angle);;
							else if (player == &players[secondarydisplayplayer] && cv_cam_turnfacingability[1].value > 0 && !(PLAYER2INPUTDOWN(gc_turnleft) || PLAYER2INPUTDOWN(gc_turnright)))
								P_SetPlayerAngle(player, player->mo->angle);
						}*/
					}
					break;

				case CA_FLY:
				case CA_SWIM:
					// If currently in the air from a jump, and you pressed the
					// button again and have the ability to fly, do so!
					if (player->charability == CA_SWIM && !(player->mo->eflags & MFE_UNDERWATER))
						; // Can't do anything if you're a fish out of water!
					else if (!(player->pflags & PF_THOKKED) && !(player->powers[pw_tailsfly]))
					{
						P_SetPlayerMobjState(player->mo, S_PLAY_FLY); // Change to the flying animation

						player->powers[pw_tailsfly] = tailsflytics + 1; // Set the fly timer

						player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_SPINNING|PF_STARTDASH);
						if (player->bot == 1)
							player->pflags |= PF_THOKKED;
						else
							player->pflags |= (PF_THOKKED|PF_CANCARRY);
					}
					break;
				case CA_GLIDEANDCLIMB:
					// Now Knuckles-type abilities are checked.
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						fixed_t glidespeed = FixedMul(player->actionspd, player->mo->scale);
						fixed_t playerspeed = player->speed;

						if (player->mo->eflags & MFE_UNDERWATER)
						{
							glidespeed >>= 1;
							playerspeed = 2*playerspeed/3;
							if (!(player->powers[pw_super] || player->powers[pw_sneakers]))
							{
								player->mo->momx = (2*(player->mo->momx - player->cmomx)/3) + player->cmomx;
								player->mo->momy = (2*(player->mo->momy - player->cmomy)/3) + player->cmomy;
							}
						}

						player->pflags |= PF_GLIDING|PF_THOKKED;
						player->glidetime = 0;

						P_SetPlayerMobjState(player->mo, S_PLAY_GLIDE);
						if (playerspeed < glidespeed)
							P_Thrust(player->mo, player->mo->angle, glidespeed - playerspeed);
						player->pflags &= ~(PF_SPINNING|PF_STARTDASH);
					}
					break;
				case CA_DOUBLEJUMP: // Double-Jump
					if (!(player->pflags & PF_THOKKED) || ((player->charflags & SF_MULTIABILITY) && (player->secondjump < (player->actionspd >> FRACBITS))))
					{
						player->pflags |= PF_THOKKED;
						player->pflags &= ~PF_JUMPED;
						P_DoJump(player, true);
						player->secondjump++;
					}
					break;
				case CA_FLOAT: // Float
				case CA_SLOWFALL: // Slow descent hover
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						if (player->charflags & SF_DASHMODE && player->dashmode >= DASHMODE_THRESHOLD)
							P_SetPlayerMobjState(player->mo, S_PLAY_DASH);
						else if (player->speed >= FixedMul(player->runspeed, player->mo->scale))
							P_SetPlayerMobjState(player->mo, S_PLAY_FLOAT_RUN);
						else
							P_SetPlayerMobjState(player->mo, S_PLAY_FLOAT);
						player->pflags |= PF_THOKKED;
						player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_SPINNING);
						player->secondjump = 1;
					}
					break;
				case CA_TELEKINESIS:
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						P_Telekinesis(player,
							FixedMul(player->actionspd, player->mo->scale), // +ve thrust (pushing away from player)
							FixedMul(384*FRACUNIT, player->mo->scale));
					}
					break;
				case CA_FALLSWITCH:
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						player->mo->momz = -player->mo->momz;
						P_SpawnThokMobj(player);
						player->pflags |= PF_THOKKED;
					}
					break;
				case CA_AIRDRILL:
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						player->flyangle = 56 + (60-(player->actionspd>>FRACBITS))/3;
						player->pflags |= PF_THOKKED;
						S_StartSound(player->mo, sfx_spndsh);
					}
					break;
				case CA_BOUNCE:
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
					{
						P_SetPlayerMobjState(player->mo, S_PLAY_BOUNCE);
						player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE);
						player->pflags |= PF_THOKKED|PF_BOUNCING;
						player->mo->momx >>= 1;
						player->mo->momy >>= 1;
						player->mo->momz >>= 1;
					}
					break;
				case CA_TWINSPIN:
					if (!(player->pflags & PF_THOKKED) || player->charflags & SF_MULTIABILITY)
						P_DoTwinSpin(player);
					break;
				default:
					break;
			}
		}
		else if (player->pflags & PF_THOKKED)
		{
			if (!LUAh_AbilitySpecial(player))
				switch (player->charability)
				{
					case CA_FLY:
					case CA_SWIM: // Swim
						if (player->charability == CA_SWIM && !(player->mo->eflags & MFE_UNDERWATER))
							; // Can't do anything if you're a fish out of water!
						else if (player->powers[pw_tailsfly]) // If currently flying, give an ascend boost.
						{
							player->fly1 = 20;

							if (player->charability == CA_SWIM)
								player->fly1 /= 2;

							// Slow down!
							if (player->speed > FixedMul(8*FRACUNIT, player->mo->scale) && player->speed > FixedMul(player->normalspeed>>1, player->mo->scale))
								P_Thrust(player->mo, R_PointToAngle2(0,0,player->mo->momx,player->mo->momy), FixedMul(-4*FRACUNIT, player->mo->scale));
						}
						break;
					default:
						break;
				}
		}
		else if ((player->powers[pw_shield] & SH_NOSTACK) == SH_WHIRLWIND && !player->powers[pw_super])
			P_DoJumpShield(player);
	}

	// HOMING option.
	if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ATTRACT // Sonic 3D Blast.
	&& player->pflags & PF_SHIELDABILITY)
	{
		if (player->homing && player->mo->tracer)
		{
			if (!P_HomingAttack(player->mo, player->mo->tracer))
			{
				P_SetObjectMomZ(player->mo, 6*FRACUNIT, false);
				if (player->mo->eflags & MFE_UNDERWATER)
					player->mo->momz = FixedMul(player->mo->momz, FRACUNIT/3);
				player->homing = 0;
			}
		}

		// If you're not jumping, then you obviously wouldn't be homing.
		if (!(player->pflags & PF_JUMPED))
			player->homing = 0;
	}
	else if (player->charability == CA_HOMINGTHOK) // Sonic Adventure.
	{
		// If you've got a target, chase after it!
		if (player->homing && player->mo->tracer)
		{
			P_SpawnThokMobj(player);

			// But if you don't, then stop homing.
			if (!P_HomingAttack(player->mo, player->mo->tracer))
			{
				if (player->mo->eflags & MFE_UNDERWATER)
					P_SetObjectMomZ(player->mo, FixedDiv(457*FRACUNIT,72*FRACUNIT), false);
				else
					P_SetObjectMomZ(player->mo, 10*FRACUNIT, false);

				player->mo->momx = player->mo->momy = player->homing = 0;

				if (player->mo->tracer->flags2 & MF2_FRET)
					P_InstaThrust(player->mo, player->mo->angle, -(player->speed>>3));

				if (!(player->mo->tracer->flags & MF_BOSS))
					player->pflags &= ~PF_THOKKED;

				P_SetPlayerMobjState(player->mo, S_PLAY_SPRING);
				player->pflags |= PF_NOJUMPDAMAGE;
			}
		}

		// If you're not jumping, then you obviously wouldn't be homing.
		if (!(player->pflags & PF_JUMPED))
			player->homing = 0;
	}
	else
		player->homing = 0;

	if (cmd->buttons & BT_JUMP)
	{
		player->pflags |= PF_JUMPDOWN;

		if ((!(gametyperules & GTR_TEAMFLAGS) || !player->gotflag) && !player->exiting)
		{
			if (player->secondjump == 1 && player->charability != CA_DOUBLEJUMP && player->charability != CA_THOK)
			{
				fixed_t potentialmomz;
				if (player->charability == CA_SLOWFALL)
					potentialmomz = -gravity*4;
				else
					potentialmomz = ((player->speed < 10*player->mo->scale)
					? (player->speed - 10*player->mo->scale)/5
					: 0);
				if (P_MobjFlip(player->mo)*player->mo->momz < potentialmomz)
					player->mo->momz = P_MobjFlip(player->mo)*potentialmomz;
				player->pflags &= ~PF_SPINNING;
			}
		}
	}
	else // If not pressing the jump button
	{
		player->pflags &= ~PF_JUMPDOWN;

		// Repeat abilities, but not double jump!
		if (player->secondjump == 1 && player->charability != CA_DOUBLEJUMP && player->charability != CA_AIRDRILL && player->charability != CA_THOK)
		{
			if (player->charflags & SF_MULTIABILITY)
			{
				player->pflags |= (PF_JUMPED|PF_NOJUMPDAMAGE);
				player->secondjump = 0;
			}
			else
				player->secondjump = 2;

			P_SetPlayerMobjState(player->mo, S_PLAY_FALL);
		}

		// If letting go of the jump button while still on ascent, cut the jump height.
		if (((player->pflags & (PF_JUMPED|PF_STARTJUMP)) == (PF_JUMPED|PF_STARTJUMP)) && (P_MobjFlip(player->mo)*player->mo->momz > 0))
		{
			player->mo->momz >>= 1;
			player->pflags &= ~PF_STARTJUMP;
		}
	}
}

//
// P_GetPlayerControlDirection
//
// Determines if the player is pressing in the direction they are moving
//
// 0 = no controls pressed/no movement
// 1 = pressing in the direction of movement
// 2 = pressing in the opposite direction of movement
//
INT32 P_GetPlayerControlDirection(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;
	angle_t controllerdirection, controlplayerdirection;
	camera_t *thiscam;
	angle_t dangle;
	fixed_t tempx = 0, tempy = 0;
	angle_t tempangle, origtempangle;

	if (splitscreen && player == &players[secondarydisplayplayer])
		thiscam = &camera2;
	else
		thiscam = &camera;

	if (!cmd->forwardmove && !cmd->sidemove)
		return 0;

	if (!player->mo->momx && !player->mo->momy)
		return 0;

	if (twodlevel || player->mo->flags2 & MF2_TWOD)
	{
		if (!cmd->sidemove)
			return 0;
		if (!player->mo->momx)
			return 0;
		origtempangle = tempangle = 0; // relative to the axis rather than the player!
		controlplayerdirection = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
	}
	else if ((P_ControlStyle(player) & CS_LMAOGALOG) && thiscam->chase)
	{
		if (player->awayviewtics)
			origtempangle = tempangle = player->awayviewmobj->angle;
		else
			origtempangle = tempangle = thiscam->angle;
		controlplayerdirection = player->mo->angle;
	}
	else
	{
		origtempangle = tempangle = player->mo->angle;
		controlplayerdirection = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
	}

	// Calculate the angle at which the controls are pointing
	// to figure out the proper mforward and mbackward.
	tempangle >>= ANGLETOFINESHIFT;
	if (!(twodlevel || player->mo->flags2 & MF2_TWOD)) // in 2d mode, sidemove is treated as the forwards/backwards direction
	{
		tempx += FixedMul(cmd->forwardmove*FRACUNIT,FINECOSINE(tempangle));
		tempy += FixedMul(cmd->forwardmove*FRACUNIT,FINESINE(tempangle));

		tempangle = origtempangle-ANGLE_90;
		tempangle >>= ANGLETOFINESHIFT;
	}
	tempx += FixedMul(cmd->sidemove*FRACUNIT,FINECOSINE(tempangle));
	tempy += FixedMul(cmd->sidemove*FRACUNIT,FINESINE(tempangle));

	controllerdirection = R_PointToAngle2(0, 0, tempx, tempy);

	dangle = controllerdirection - controlplayerdirection;

	if (dangle > ANGLE_180) //flip to keep to one side
		dangle = InvAngle(dangle);

	if (dangle > ANGLE_90)
		return 2; // Controls pointing backwards from player
	else
		return 1; // Controls pointing in player's general direction
}

// Control scheme for 2d levels.
static void P_2dMovement(player_t *player)
{
	ticcmd_t *cmd;
	INT32 topspeed, acceleration, thrustfactor;
	fixed_t movepushforward = 0;
	angle_t movepushangle = 0;
	fixed_t normalspd = FixedMul(player->normalspeed, player->mo->scale);

	cmd = &player->cmd;

	if (player->exiting || player->pflags & PF_STASIS)
	{
		cmd->forwardmove = cmd->sidemove = 0;
		if (player->pflags & PF_GLIDING)
		{
			if (!player->skidtime)
				player->pflags &= ~PF_GLIDING;
			else if (player->exiting)
			{
				player->pflags &= ~PF_GLIDING;
				P_SetPlayerMobjState(player->mo, S_PLAY_WALK);
				player->skidtime = 0;
			}
		}
		if (player->pflags & PF_BOUNCING)
			player->pflags &= ~PF_BOUNCING;
		if (player->pflags & PF_SPINNING && !player->exiting)
		{
			player->pflags &= ~PF_SPINNING;
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
		}
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
	else if (player->onconveyor != 2 && player->onconveyor != 4 && player->onconveyor != 1)
		player->cmomx = player->cmomy = 0;

	player->rmomx = player->mo->momx - player->cmomx;
	player->rmomy = player->mo->momy - player->cmomy;

	// Calculates player's speed based on absolute-value-of-a-number formula
	player->speed = abs(player->rmomx);

	if (player->pflags & PF_GLIDING)
	{
		// Angle fix.
		if (player->mo->angle < ANGLE_180 && player->mo->angle > ANGLE_90)
			player->mo->angle = ANGLE_180;
		else if (player->mo->angle < ANGLE_90 && player->mo->angle > 0)
			player->mo->angle = 0;

		if (cmd->sidemove > 0 && player->mo->angle != 0 && player->mo->angle >= ANGLE_180)
			player->mo->angle += 1280<<FRACBITS;
		else if (cmd->sidemove < 0 && player->mo->angle != ANGLE_180 && (player->mo->angle > ANGLE_180 || player->mo->angle == 0))
			player->mo->angle -= 1280<<FRACBITS;
		else if (cmd->sidemove == 0)
		{
			if (player->mo->angle >= ANGLE_270)
				player->mo->angle += 1280<<FRACBITS;
			else if (player->mo->angle < ANGLE_270 && player->mo->angle > ANGLE_180)
				player->mo->angle -= 1280<<FRACBITS;
		}
	}
	else if (cmd->sidemove && !(player->climbing) && !P_PlayerInPain(player))
	{
		if (cmd->sidemove > 0)
			player->mo->angle = 0;
		else if (cmd->sidemove < 0)
			player->mo->angle = ANGLE_180;
	}

	P_SetPlayerAngle(player, player->mo->angle);

	if (player->pflags & PF_GLIDING)
		movepushangle = player->mo->angle;
	else
	{
		if (cmd->sidemove > 0)
			movepushangle = 0;
		else if (cmd->sidemove < 0)
			movepushangle = ANGLE_180;
		else
			movepushangle = player->mo->angle;
	}

	// Do not let the player control movement if not onground.
	onground = P_IsObjectOnGround(player->mo);

	player->aiming = cmd->aiming<<FRACBITS;

	// Set the player speeds.
	if (maptol & TOL_2D)
		normalspd = FixedMul(normalspd, 2*FRACUNIT/3);

	if (player->powers[pw_super] || player->powers[pw_sneakers])
	{
		thrustfactor = player->thrustfactor*2;
		acceleration = player->accelstart/2 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration/2;

		if (player->powers[pw_tailsfly])
			topspeed = normalspd;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER) && !(player->pflags & PF_SLIDING))
		{
			topspeed = normalspd;
			acceleration = 2*acceleration/3;
		}
		else
			topspeed = normalspd * 2;
	}
	else
	{
		thrustfactor = player->thrustfactor;
		acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;

		if (player->powers[pw_tailsfly])
			topspeed = normalspd/2;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER) && !(player->pflags & PF_SLIDING))
		{
			topspeed = normalspd/2;
			acceleration = 2*acceleration/3;
		}
		else
			topspeed = normalspd;
	}

//////////////////////////////////////
	if (player->climbing)
	{
		if (cmd->forwardmove != 0)
			P_SetObjectMomZ(player->mo, FixedDiv(cmd->forwardmove*FRACUNIT, player->powers[pw_super] ? 5*FRACUNIT : 15*FRACUNIT>>1), false); // 2/3 while super

		player->mo->momx = 0;
	}
	else if (cmd->sidemove != 0 && !(player->pflags & PF_GLIDING || player->exiting
		|| (P_PlayerInPain(player) && !onground)))
	{
		movepushforward = abs(cmd->sidemove) * (thrustfactor * acceleration);

		// allow very small movement while in air for gameplay
		if (!onground)
			movepushforward >>= 1; // Proper air movement

		// Allow a bit of movement while spinning
		if ((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING)
		{
			if (!(player->pflags & PF_STARTDASH))
				movepushforward = movepushforward/48;
			else
				movepushforward = 0;
		}

		movepushforward = FixedMul(movepushforward, player->mo->scale);
		if (player->rmomx < topspeed && cmd->sidemove > 0) // Sonic's Speed
			P_Thrust(player->mo, movepushangle, movepushforward);
		else if (player->rmomx > -topspeed && cmd->sidemove < 0)
			P_Thrust(player->mo, movepushangle, movepushforward);
	}
}

//#define OLD_MOVEMENT_CODE 1
static void P_3dMovement(player_t *player)
{
	ticcmd_t *cmd;
	angle_t movepushangle, movepushsideangle; // Analog
	INT32 topspeed, acceleration, thrustfactor;
	fixed_t movepushforward = 0, movepushside = 0;
	INT32 mforward = 0, mbackward = 0;
	angle_t dangle; // replaces old quadrants bits
	fixed_t normalspd = FixedMul(player->normalspeed, player->mo->scale);
	controlstyle_e controlstyle;
	boolean spin = ((onground = P_IsObjectOnGround(player->mo)) && (player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING && (player->rmomx || player->rmomy) && !(player->pflags & PF_STARTDASH));
	fixed_t oldMagnitude, newMagnitude;
	vector3_t totalthrust;

	totalthrust.x = totalthrust.y = 0; // I forget if this is needed
	totalthrust.z = FRACUNIT*P_MobjFlip(player->mo)/3; // A bit of extra push-back on slopes

	// Get the old momentum; this will be needed at the end of the function! -SH
	oldMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);

	controlstyle = P_ControlStyle(player);

	cmd = &player->cmd;

	if (player->exiting || player->pflags & PF_STASIS)
	{
		cmd->forwardmove = cmd->sidemove = 0;
		if (player->pflags & PF_GLIDING)
		{
			if (!player->skidtime)
				player->pflags &= ~PF_GLIDING;
			else if (player->exiting)
			{
				player->pflags &= ~PF_GLIDING;
				P_SetPlayerMobjState(player->mo, S_PLAY_WALK);
				player->skidtime = 0;
			}
		}
		if (player->pflags & PF_BOUNCING)
			player->pflags &= ~PF_BOUNCING;
		if (player->pflags & PF_SPINNING && !player->exiting)
		{
			player->pflags &= ~PF_SPINNING;
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
		}
	}

	if (controlstyle & CS_LMAOGALOG)
	{
		movepushangle = (cmd->angleturn<<16 /* not FRACBITS */);
	}
	else
	{
		movepushangle = player->mo->angle;
	}
	movepushsideangle = movepushangle-ANGLE_90;

	// cmomx/cmomy stands for the conveyor belt speed.
	if (player->onconveyor == 2) // Wind/Current
	{
		//if (player->mo->z > player->mo->watertop || player->mo->z + player->mo->height < player->mo->waterbottom)
		if (!(player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
			player->cmomx = player->cmomy = 0;
	}
	else if (player->onconveyor == 4 && !P_IsObjectOnGround(player->mo)) // Actual conveyor belt
		player->cmomx = player->cmomy = 0;
	else if (player->onconveyor != 2 && player->onconveyor != 4 && player->onconveyor != 1)
		player->cmomx = player->cmomy = 0;

	player->rmomx = player->mo->momx - player->cmomx;
	player->rmomy = player->mo->momy - player->cmomy;

	// Calculates player's speed based on distance-of-a-line formula
	player->speed = P_AproxDistance(player->rmomx, player->rmomy);

	// Monster Iestyn - 04-11-13
	// Quadrants are stupid, excessive and broken, let's do this a much simpler way!
	// Get delta angle from rmom angle and player angle first
	dangle = R_PointToAngle2(0,0, player->rmomx, player->rmomy) - player->mo->angle;
	if (dangle > ANGLE_180) //flip to keep to one side
		dangle = InvAngle(dangle);

	// now use it to determine direction!
	if (dangle <= ANGLE_45) // angles 0-45 or 315-360
		mforward = 1; // going forwards
	else if (dangle >= ANGLE_135) // angles 135-225
		mbackward = 1; // going backwards

	// anything else will leave both at 0, so no need to do anything else

	// When sliding, don't allow forward/back
	if (player->pflags & PF_SLIDING)
		cmd->forwardmove = 0;
	else if (onground && player->mo->state == states+S_PLAY_PAIN)
		P_SetPlayerMobjState(player->mo, S_PLAY_WALK);

	player->aiming = cmd->aiming<<FRACBITS;

	// Set the player speeds.
	if (player->pflags & PF_SLIDING)
	{
		normalspd = FixedMul(36<<FRACBITS, player->mo->scale);
		thrustfactor = 5;
		acceleration = 96 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * 40;
		topspeed = normalspd;
	}
	else if (player->bot)
	{ // Bot steals player 1's stats
		normalspd = FixedMul(players[consoleplayer].normalspeed, player->mo->scale);
		thrustfactor = players[consoleplayer].thrustfactor;
		acceleration = players[consoleplayer].accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * players[consoleplayer].acceleration;

		if (player->powers[pw_tailsfly])
			topspeed = normalspd/2;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER))
		{
			topspeed = normalspd/2;
			acceleration = 2*acceleration/3;
		}
		else
			topspeed = normalspd;
	}
	else
	{
		if (player->powers[pw_super] || player->powers[pw_sneakers])
		{
			topspeed = 5 * normalspd / 3; // 1.67x
			thrustfactor = player->thrustfactor*2;
			acceleration = player->accelstart/2 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration/2;
		}
		else
		{
			topspeed = normalspd;
			thrustfactor = player->thrustfactor;
			acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;
		}

		if (player->powers[pw_tailsfly])
			topspeed >>= 1;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER))
		{
			topspeed >>= 1;
			acceleration = 2*acceleration/3;
		}
	}

	if (spin) // Prevent gaining speed whilst rolling!
	{
		const fixed_t ns = FixedDiv(549*ORIG_FRICTION,500*FRACUNIT); // P_XYFriction
		topspeed = FixedMul(oldMagnitude, ns);
	}

	// Better maneuverability while flying
	if (player->powers[pw_tailsfly])
	{
		thrustfactor = player->thrustfactor*2;
		acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;
	}
	else
	{
		if (player->pflags & PF_BOUNCING)
		{
			if (player->mo->state-states == S_PLAY_BOUNCE_LANDING)
			{
				thrustfactor = player->thrustfactor*8;
				acceleration = player->accelstart/8 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration/8;
			}
			else
			{
				thrustfactor = (3*player->thrustfactor)/4;
				acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;
			}
		}

		if (player->mo->movefactor != FRACUNIT) // Friction-scaled acceleration...
			acceleration = FixedMul(acceleration<<FRACBITS, player->mo->movefactor)>>FRACBITS;
	}

	// Forward movement
	if (player->climbing)
	{
		if (cmd->forwardmove)
		{
			if (player->mo->eflags & MFE_UNDERWATER)
				P_SetObjectMomZ(player->mo, FixedDiv(cmd->forwardmove*FRACUNIT, player->powers[pw_super] ? 20*FRACUNIT/3 : 10*FRACUNIT), false); // 2/3 while super
			else
				P_SetObjectMomZ(player->mo, FixedDiv(cmd->forwardmove*FRACUNIT, player->powers[pw_super] ? 5*FRACUNIT : 15*FRACUNIT>>1), false); // 2/3 while super
		}
	}
	else if (!(controlstyle == CS_LMAOGALOG)
		&& cmd->forwardmove != 0 && !(player->pflags & PF_GLIDING || player->exiting
		|| (P_PlayerInPain(player) && !onground)))
	{
		movepushforward = cmd->forwardmove * (thrustfactor * acceleration);

		// Allow a bit of movement while spinning
		if ((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING)
		{
			if ((mforward && cmd->forwardmove > 0) || (mbackward && cmd->forwardmove < 0)
			|| (player->pflags & PF_STARTDASH))
				movepushforward = 0;
			else if (onground)
				movepushforward >>= 4;
			else
				movepushforward >>= 3;
		}
		// allow very small movement while in air for gameplay
		else if (!onground)
			movepushforward >>= 2; // proper air movement

		movepushforward = FixedMul(movepushforward, player->mo->scale);

		totalthrust.x += P_ReturnThrustX(player->mo, movepushangle, movepushforward);
		totalthrust.y += P_ReturnThrustY(player->mo, movepushangle, movepushforward);
	}
	// Sideways movement
	if (player->climbing)
	{
		if (player->mo->eflags & MFE_UNDERWATER)
			P_InstaThrust(player->mo, player->mo->angle-ANGLE_90, FixedDiv(cmd->sidemove*player->mo->scale, player->powers[pw_super] ? 20*FRACUNIT/3 : 10*FRACUNIT)); // 2/3 while super
		else
			P_InstaThrust(player->mo, player->mo->angle-ANGLE_90, FixedDiv(cmd->sidemove*player->mo->scale, player->powers[pw_super] ? 5*FRACUNIT : 15*FRACUNIT>>1)); // 2/3 while super
	}
	// Analog movement control
	else if (controlstyle == CS_LMAOGALOG)
	{
		if (!(player->pflags & PF_GLIDING || player->exiting || P_PlayerInPain(player)))
		{
			angle_t controldirection;

			// Calculate the angle at which the controls are pointing
			// to figure out the proper mforward and mbackward.
			// (Why was it so complicated before? ~Red)
			controldirection = R_PointToAngle2(0, 0, cmd->forwardmove*FRACUNIT, -cmd->sidemove*FRACUNIT)+movepushangle;

			movepushforward = FixedHypot(cmd->sidemove, cmd->forwardmove) * (thrustfactor * acceleration);

			// Allow a bit of movement while spinning
			if ((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING)
			{
				if ((mforward && cmd->forwardmove > 0) || (mbackward && cmd->forwardmove < 0)
				|| (player->pflags & PF_STARTDASH))
					movepushforward = 0;
				else if (onground)
					movepushforward >>= 4;
				else
					movepushforward >>= 3;
			}
			// allow very small movement while in air for gameplay
			else if (!onground)
				movepushforward >>= 2; // proper air movement

			movepushsideangle = controldirection;

			movepushforward = FixedMul(movepushforward, player->mo->scale);

			totalthrust.x += P_ReturnThrustX(player->mo, controldirection, movepushforward);
			totalthrust.y += P_ReturnThrustY(player->mo, controldirection, movepushforward);
		}
	}
	else if (cmd->sidemove && !(player->pflags & PF_GLIDING) && !player->exiting && !P_PlayerInPain(player))
	{
		movepushside = cmd->sidemove * (thrustfactor * acceleration);

		// allow very small movement while in air for gameplay
		if (!onground)
		{
			movepushside >>= 2; // proper air movement
			// Reduce movepushslide even more if over "max" flight speed
			if (((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING) || (player->powers[pw_tailsfly] && player->speed > topspeed))
				movepushside >>= 2;
		}
		// Allow a bit of movement while spinning
		else if ((player->pflags & (PF_SPINNING|PF_THOKKED)) == PF_SPINNING)
		{
			if (player->pflags & PF_STARTDASH)
				movepushside = 0;
			else if (onground)
				movepushside >>= 4;
			else
				movepushside >>= 3;
		}

		// Finally move the player now that their speed/direction has been decided.
		movepushside = FixedMul(movepushside, player->mo->scale);

		totalthrust.x += P_ReturnThrustX(player->mo, movepushsideangle, movepushside);
		totalthrust.y += P_ReturnThrustY(player->mo, movepushsideangle, movepushside);
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

	player->mo->momx += totalthrust.x;
	player->mo->momy += totalthrust.y;

	// Time to ask three questions:
	// 1) Are we over topspeed?
	// 2) If "yes" to 1, were we moving over topspeed to begin with?
	// 3) If "yes" to 2, are we now going faster?

	// If "yes" to 3, normalize to our initial momentum; this will allow thoks to stay as fast as they normally are.
	// If "no" to 3, ignore it; the player might be going too fast, but they're slowing down, so let them.
	// If "no" to 2, normalize to topspeed, so we can't suddenly run faster than it of our own accord.
	// If "no" to 1, we're not reaching any limits yet, so ignore this entirely!
	// -Shadow Hog
	newMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);
	if (newMagnitude > topspeed)
	{
		fixed_t tempmomx, tempmomy;
		if (oldMagnitude > topspeed && !spin)
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

//
// P_SpectatorMovement
//
// Control for spectators in multiplayer
//
static void P_SpectatorMovement(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;

	player->mo->angle = (cmd->angleturn<<16 /* not FRACBITS */);

	ticruned++;
	if (!(cmd->angleturn & TICCMD_RECEIVED))
		ticmiss++;

	if (cmd->buttons & BT_JUMP)
		player->mo->z += FRACUNIT*16;
	else if (cmd->buttons & BT_USE)
		player->mo->z -= FRACUNIT*16;

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
		P_Thrust(player->mo, player->mo->angle, cmd->forwardmove*(FRACUNIT/2));

		// Quake-style flying spectators :D
		player->mo->momz += FixedMul(cmd->forwardmove*(FRACUNIT/2), AIMINGTOSLOPE(player->aiming));
	}
	if (cmd->sidemove != 0)
	{
		P_Thrust(player->mo, player->mo->angle-ANGLE_90, cmd->sidemove*(FRACUNIT/2));
	}
}

//
// P_ShootLine
//
// Fun and fancy
// graphical indicator
// for building/debugging
// NiGHTS levels!
static void P_ShootLine(mobj_t *source, mobj_t *dest, fixed_t height)
{
	mobj_t *mo;
	INT32 i;
	fixed_t temp;
	INT32 speed, seesound;

	temp = dest->z;
	dest->z = height;

	seesound = mobjinfo[MT_REDRING].seesound;
	speed = mobjinfo[MT_REDRING].speed;
	mobjinfo[MT_REDRING].seesound = sfx_None;
	mobjinfo[MT_REDRING].speed = 20*FRACUNIT;

	mo = P_SpawnXYZMissile(source, dest, MT_REDRING, source->x, source->y, height);

	dest->z = temp;
	if (mo)
	{
		mo->flags2 |= MF2_RAILRING;
		mo->flags2 |= MF2_DONTDRAW;
		mo->flags |= MF_NOCLIPHEIGHT;
		mo->flags |= MF_NOCLIP;
		mo->flags &= ~MF_MISSILE;
		mo->fuse = 3;
	}

	for (i = 0; i < 32; i++)
	{
		if (mo)
		{
			if (!(mo->flags & MF_NOBLOCKMAP))
			{
				P_UnsetThingPosition(mo);
				mo->flags |= MF_NOBLOCKMAP;
				P_SetThingPosition(mo);
			}
			if (i&1)
				P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

			P_UnsetThingPosition(mo);
			mo->x += mo->momx;
			mo->y += mo->momy;
			mo->z += mo->momz;
			P_SetThingPosition(mo);
		}
		else
		{
			mobjinfo[MT_REDRING].seesound = seesound;
			mobjinfo[MT_REDRING].speed = speed;
			return;
		}
	}
	mobjinfo[MT_REDRING].seesound = seesound;
	mobjinfo[MT_REDRING].speed = speed;
}

#define MAXDRILLSPEED 14000
#define MAXNORMALSPEED 6250 //6000
>>>>>>> srb2/next

				bust:
					topheight = P_GetFOFTopZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
					bottomheight = P_GetFOFBottomZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);

					if (((player->charability == CA_TWINSPIN) && (player->panim == PA_ABILITY))
					|| ((P_MobjFlip(player->mo)*player->mo->momz < 0) && (player->pflags & PF_BOUNCING || ((player->charability2 == CA2_MELEE) && (player->panim == PA_ABILITY2)))))
					{
						topheight -= player->mo->momz;
						bottomheight -= player->mo->momz;
					}

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

						if (!(rover->master->flags & ML_BOUNCY))
						{
							if (newmom > 0)
							{
								if (newmom < 8*FRACUNIT)
									newmom = 8*FRACUNIT;
							}
							else if (newmom > -8*FRACUNIT && newmom != 0)
								newmom = -8*FRACUNIT;
						}

						if (newmom > P_GetPlayerHeight(player)/2)
							newmom = P_GetPlayerHeight(player)/2;
						else if (newmom < -P_GetPlayerHeight(player)/2)
							newmom = -P_GetPlayerHeight(player)/2;

						momentum.z = newmom*2;

						if (slope)
							P_QuantizeMomentumToSlope(&momentum, slope);

						player->mo->momx = momentum.x;
						player->mo->momy = momentum.y;
						player->mo->momz = momentum.z/2;

						if (player->pflags & PF_SPINNING)
						{
							player->pflags &= ~PF_SPINNING;
							player->pflags |= P_GetJumpFlags(player);
							player->pflags |= PF_THOKKED;
						}
					}
					else
					{
						player->mo->momx = -FixedMul(player->mo->momx,linedist);
						player->mo->momy = -FixedMul(player->mo->momy,linedist);

						if (player->pflags & PF_SPINNING)
						{
							player->pflags &= ~PF_SPINNING;
							player->pflags |= P_GetJumpFlags(player);
							player->pflags |= PF_THOKKED;
						}
					}

					if ((player->pflags & PF_SPINNING) && player->speed < FixedMul(1<<FRACBITS, player->mo->scale) && player->mo->momz)
					{
						player->pflags &= ~PF_SPINNING;
						player->pflags |= P_GetJumpFlags(player);
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
// P_CheckSneakerAndLivesTimer
//
// Restores music from sneaker and life fanfares
//
/* // SRB2kart - Can't drown.
static void P_CheckSneakerAndLivesTimer(player_t *player)
{
	if (player->powers[pw_extralife] == 1) // Extra Life!
		P_RestoreMusic(player);

	//if (player->powers[pw_sneakers] == 1) // SRB2kart
	//	P_RestoreMusic(player);
}
*/

//
// P_CheckUnderwaterAndSpaceTimer
//
// Restores music from underwater and space warnings, and handles number generation
//
/* // SRB2kart - Can't drown.
static void P_CheckUnderwaterAndSpaceTimer(player_t *player)
{
	tic_t timeleft = (player->powers[pw_spacetime]) ? player->powers[pw_spacetime] : player->powers[pw_underwater];

	if (player->exiting || (player->pflags & PF_FINISHED))
		player->powers[pw_underwater] = player->powers[pw_spacetime] = 0;

	timeleft--; // The original code was all n*TICRATE + 1, so let's remove 1 tic for simplicity

	if ((timeleft == 11*TICRATE) // 5
	 || (timeleft ==  9*TICRATE) // 4
	 || (timeleft ==  7*TICRATE) // 3
	 || (timeleft ==  5*TICRATE) // 2
	 || (timeleft ==  3*TICRATE) // 1
	 || (timeleft ==  1*TICRATE) // 0
	) {
		fixed_t height = (player->mo->eflags & MFE_VERTICALFLIP)
		? player->mo->z - FixedMul(8*FRACUNIT + mobjinfo[MT_DROWNNUMBERS].height, FixedMul(player->mo->scale, player->shieldscale))
		: player->mo->z + player->mo->height + FixedMul(8*FRACUNIT, FixedMul(player->mo->scale, player->shieldscale));

		mobj_t *numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, height, MT_DROWNNUMBERS);

		timeleft /= (2*TICRATE); // To be strictly accurate it'd need to be ((timeleft/TICRATE) - 1)/2, but integer division rounds down for us

		if (player->charflags & SF_MACHINE)
		{
			S_StartSound(player->mo, sfx_buzz1);
			timeleft += 6;
		}
		else
			S_StartSound(player->mo, sfx_dwnind);

		if (timeleft) // Don't waste time setting the state if the time is 0.
			P_SetMobjState(numbermobj, numbermobj->info->spawnstate+timeleft);

		P_SetTarget(&numbermobj->target, player->mo);
		numbermobj->threshold = 40;
		numbermobj->destscale = player->mo->scale;
		P_SetScale(numbermobj, player->mo->scale);
	}
	// Underwater timer runs out
	else if (timeleft == 1)
	{
		if ((netgame || multiplayer) && P_IsLocalPlayer(player))
			S_ChangeMusic(mapmusname, mapmusflags, true);

		if (player->powers[pw_spacetime] == 1)
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_SPACEDROWN);
		else
			P_DamageMobj(player->mo, NULL, NULL, 1, DMG_DROWNED);
	}

	if (!(player->mo->eflags & MFE_UNDERWATER) && player->powers[pw_underwater])
	{
		if (player->powers[pw_underwater] <= 12*TICRATE + 1)
		{
			player->powers[pw_underwater] = 0;
			P_RestoreMusic(player);
		}
		else
			player->powers[pw_underwater] = 0;
	}

	if (player->powers[pw_spacetime] > 1 && !P_InSpaceSector(player->mo))
		player->powers[pw_spacetime] = 0;

	// Underwater audio cues
	if (P_IsLocalPlayer(player))
	{
		if ((player->powers[pw_underwater] == 25*TICRATE + 1)
		|| (player->powers[pw_underwater] == 20*TICRATE + 1)
		|| (player->powers[pw_underwater] == 15*TICRATE + 1))
			S_StartSound(NULL, sfx_wtrdng);

		if (player->powers[pw_underwater] == 11*TICRATE + 1
		&& player == &players[consoleplayer])
		{
			P_PlayJingle(player, JT_DROWN);
		}
	}
}*/

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

	if (!(player->mo->eflags & MFE_UNDERWATER) || ((player->powers[pw_shield] & SH_PROTECTWATER) && !(player->powers[pw_carry] == CR_NIGHTSMODE)) || player->spectator)
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
	angle_t movepushangle, movepushsideangle;
	fixed_t movepushforward = 0, movepushside = 0;
	angle_t dangle; // replaces old quadrants bits
	//boolean dangleflip = false; // SRB2kart - toaster
	fixed_t oldMagnitude, newMagnitude;
	vector3_t totalthrust;

	totalthrust.x = totalthrust.y = 0; // I forget if this is needed
	totalthrust.z = FRACUNIT*P_MobjFlip(player->mo)/3; // A bit of extra push-back on slopes

	// Get the old momentum; this will be needed at the end of the function! -SH
	oldMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);

	cmd = &player->cmd;

	if (player->pflags & PF_STASIS || player->kartstuff[k_spinouttimer]) // pw_introcam?
	{
		cmd->forwardmove = cmd->sidemove = 0;
	}

	if (!(player->pflags & PF_FORCESTRAFE) && !player->kartstuff[k_pogospring])
		cmd->sidemove = 0;

	if (player->kartstuff[k_drift] != 0)
		movepushangle = player->mo->angle-(ANGLE_45/5)*player->kartstuff[k_drift];
	else if (player->kartstuff[k_spinouttimer] || player->kartstuff[k_wipeoutslow])	// if spun out, use the boost angle
		movepushangle = (angle_t)player->kartstuff[k_boostangle];
	else
		movepushangle = player->mo->angle;

	movepushsideangle = movepushangle-ANGLE_90;

	// cmomx/cmomy stands for the conveyor belt speed.
	if (player->onconveyor == 2) // Wind/Current
	{
		//if (player->mo->z > player->mo->watertop || player->mo->z + player->mo->height < player->mo->waterbottom)
		if (!(player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
			player->cmomx = player->cmomy = 0;
	}
	else if (player->onconveyor == 4 && !P_IsObjectOnGround(player->mo)) // Actual conveyor belt
		player->cmomx = player->cmomy = 0;
	else if (player->onconveyor != 2 && player->onconveyor != 4 && player->onconveyor != 1)
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
		//dangleflip = true;
	}

	// anything else will leave both at 0, so no need to do anything else

	//{ SRB2kart 220217 - Toaster Code for misplaced thrust
	/*
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
	*/
	//}

	// When sliding, don't allow forward/back
	if (player->pflags & PF_SLIDING)
		cmd->forwardmove = 0;

	// Do not let the player control movement if not onground.
	// SRB2Kart: pogo spring and speed bumps are supposed to control like you're on the ground
	onground = (P_IsObjectOnGround(player->mo) || (player->kartstuff[k_pogospring]));

<<<<<<< HEAD
	player->aiming = cmd->aiming<<FRACBITS;
=======
	P_SetPlayerAngle(player, player->mo->angle);
>>>>>>> srb2/next

	// Forward movement
	if (!(P_PlayerInPain(player) && !onground))
	{
		movepushforward = K_3dKartMovement(player, onground, cmd->forwardmove);

		if (player->mo->movefactor != FRACUNIT) // Friction-scaled acceleration...
			movepushforward = FixedMul(movepushforward, player->mo->movefactor);

		if (cmd->buttons & BT_BRAKE && !cmd->forwardmove) // SRB2kart - braking isn't instant
			movepushforward /= 64;

		if (cmd->forwardmove > 0)
			player->kartstuff[k_brakestop] = 0;
		else if (player->kartstuff[k_brakestop] < 6) // Don't start reversing with brakes until you've made a stop first
		{
			if (player->speed < 8*FRACUNIT)
				player->kartstuff[k_brakestop]++;
			movepushforward = 0;
		}
		// allow very small movement while in air for gameplay
		else if (!onground)
			movepushforward >>= 2; // proper air movement

		totalthrust.x += P_ReturnThrustX(player->mo, movepushangle, movepushforward);
		totalthrust.y += P_ReturnThrustY(player->mo, movepushangle, movepushforward);
	}
	else if (!(player->kartstuff[k_spinouttimer]))
	{
		K_MomentumToFacing(player);
	}

	// Sideways movement
	if (cmd->sidemove != 0 && !((player->exiting || mapreset) || player->kartstuff[k_spinouttimer]))
	{
		if (cmd->sidemove > 0)
			movepushside = (cmd->sidemove * FRACUNIT/128) + FixedDiv(player->speed, K_GetKartSpeed(player, true));
		else
			movepushside = (cmd->sidemove * FRACUNIT/128) - FixedDiv(player->speed, K_GetKartSpeed(player, true));

		totalthrust.x += P_ReturnThrustX(player->mo, movepushsideangle, movepushside);
		totalthrust.y += P_ReturnThrustY(player->mo, movepushsideangle, movepushside);
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

		if (speed > airspeedcap)
		{
			fixed_t div = 32*FRACUNIT;
			fixed_t newspeed;

			if (K_PlayerUsesBotMovement(player))
			{
				fixed_t baserubberband = K_BotRubberband(player);
				fixed_t rubberband = FixedMul(baserubberband,
					FixedMul(baserubberband,
					FixedMul(baserubberband,
					baserubberband
				))); // This looks extremely goofy, but we need this really high, but at the same time, proportional.

				if (rubberband > FRACUNIT)
				{
					div = FixedMul(div, rubberband);
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
<<<<<<< HEAD
// P_SpectatorMovement
//
// Control for spectators in multiplayer
//
static void P_SpectatorMovement(player_t *player)
=======
// P_SpawnSkidDust
//
// Spawns spindash dust randomly around the player within a certain radius.
//
void P_SpawnSkidDust(player_t *player, fixed_t radius, boolean sound)
{
	mobj_t *mo = player->mo;
	mobj_t *particle;

	particle = P_SpawnMobjFromMobj(mo, 0, 0, 0, MT_SPINDUST);
	if (radius >>= FRACBITS)
	{
		P_UnsetThingPosition(particle);
		particle->x += P_RandomRange(-radius, radius) << FRACBITS;
		particle->y += P_RandomRange(-radius, radius) << FRACBITS;
		P_SetThingPosition(particle);
	}
	particle->tics = 10;

	particle->destscale = (2*mo->scale)/3;
	P_SetScale(particle, particle->destscale);
	P_SetObjectMomZ(particle, FRACUNIT, false);

	if (mo->eflags & (MFE_TOUCHWATER|MFE_UNDERWATER)) // overrides fire version
		P_SetMobjState(particle, S_SPINDUST_BUBBLE1);
	else if (player->powers[pw_shield] == SH_ELEMENTAL)
		P_SetMobjState(particle, S_SPINDUST_FIRE1);

	if (sound)
		S_StartSound(mo, sfx_s3k7e); // the proper "Knuckles eats dirt" sfx.
}

static void P_SkidStuff(player_t *player)
>>>>>>> srb2/next
{
	ticcmd_t *cmd = &player->cmd;

<<<<<<< HEAD
	player->mo->angle = (cmd->angleturn<<16 /* not FRACBITS */);

	ticruned++;
	if (!(cmd->angleturn & TICCMD_RECEIVED))
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
=======
	// Knuckles glides into the dirt.
	if (player->pflags & PF_GLIDING && player->skidtime)
	{
		// Fell off a ledge...
		if (!onground)
		{
			player->skidtime = 0;
			player->pflags &= ~(PF_GLIDING|PF_JUMPED|PF_NOJUMPDAMAGE);
			player->pflags |= PF_THOKKED;
			P_SetPlayerMobjState(player->mo, S_PLAY_FALL);
		}
		// Get up and brush yourself off, idiot.
		else if (player->glidetime > 15 || !(player->cmd.buttons & BT_JUMP))
		{
			P_ResetPlayer(player);
			P_SetPlayerMobjState(player->mo, S_PLAY_GLIDE_LANDING);
			player->pflags |= PF_STASIS;
			if (player->speed > FixedMul(player->runspeed, player->mo->scale))
				player->skidtime += player->mo->tics;
			player->mo->momx = ((player->mo->momx - player->cmomx)/2) + player->cmomx;
			player->mo->momy = ((player->mo->momy - player->cmomy)/2) + player->cmomy;
		}
		// Didn't stop yet? Skid FOREVER!
		else if (player->skidtime == 1)
			player->skidtime = 3*TICRATE+1;
		// Spawn a particle every 3 tics.
		else if (!(player->skidtime % 3) && !(player->charflags & SF_NOSKID))
		{
			P_SpawnSkidDust(player, player->mo->radius, true);
		}
	}
	// Skidding!
	else if (onground && !(player->mo->eflags & MFE_GOOWATER) && !(player->pflags & (PF_JUMPED|PF_SPINNING|PF_SLIDING)) && !(player->charflags & SF_NOSKID))
	{
		if (player->skidtime)
		{
			// Spawn a particle every 3 tics.
			if (!(player->skidtime % 3))
			{
				if (player->mo->state-states == S_PLAY_GLIDE_LANDING)
					P_SpawnSkidDust(player, player->mo->radius, true);
				else
					P_SpawnSkidDust(player, 0, false);
			}
		}
		else if (P_AproxDistance(pmx, pmy) >= FixedMul(player->runspeed/2, player->mo->scale) // if you were moving faster than half your run speed last frame
		&& (player->mo->momx != pmx || player->mo->momy != pmy) // and you are moving differently this frame
		&& P_GetPlayerControlDirection(player) == 2) // and your controls are pointing in the opposite direction to your movement
		{ // check for skidding
			angle_t mang = R_PointToAngle2(0,0,pmx,pmy); // movement angle
			angle_t pang = R_PointToAngle2(pmx,pmy,player->mo->momx,player->mo->momy); // push angle
			angle_t dang = mang - pang; // delta angle

			if (dang > ANGLE_180) // Make delta angle always positive, invert it if it's negative.
				dang = InvAngle(dang);

			// If your push angle is more than this close to a full 180 degrees, trigger a skid.
			if (dang > ANGLE_157h)
			{
				if (player->mo->state-states != S_PLAY_SKID)
					P_SetPlayerMobjState(player->mo, S_PLAY_SKID);
				player->mo->tics = player->skidtime = (player->mo->movefactor == FRACUNIT) ? TICRATE/2 : (FixedDiv(35<<(FRACBITS-1), FixedSqrt(player->mo->movefactor)))>>FRACBITS;
				S_StartSound(player->mo, sfx_skid);
			}
		}
	}
	else {
		if (player->skidtime) {
			player->skidtime = 0;
			S_StopSound(player->mo);
		}
>>>>>>> srb2/next
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

<<<<<<< HEAD
=======
	if (!player->spectator && G_TagGametype())
	{
		// If we have stasis already here, it's because it's forced on us
		// by a linedef executor or what have you
		boolean forcestasis = false;

		//During hide time, taggers cannot move.
		if (leveltime < hidetime * TICRATE)
		{
			if (player->pflags & PF_TAGIT)
				forcestasis = true;
		}
		else if (gametyperules & GTR_HIDEFROZEN)
		{
			if (!(player->pflags & PF_TAGIT))
			{
				forcestasis = true;
				if (player->pflags & PF_GAMETYPEOVER) // Already hit.
					player->powers[pw_flashing] = 5;
			}
		}

		if (forcestasis)
		{
			player->pflags |= PF_FULLSTASIS;
			// If you're in stasis in tag, you don't drown.
			if (player->powers[pw_underwater] <= 12*TICRATE + 1)
				P_RestoreMusic(player);
			player->powers[pw_underwater] = player->powers[pw_spacetime] = 0;
		}
	}

>>>>>>> srb2/next
	if (player->spectator)
	{
		player->mo->eflags &= ~MFE_VERTICALFLIP; // deflip...
		P_SpectatorMovement(player);
		return;
	}

<<<<<<< HEAD
	//////////////////////
	// MOVEMENT CODE	//
	//////////////////////
=======
	// Locate the capsule for this mare.
	else if (maptol & TOL_NIGHTS)
	{
		if (P_PlayerFullbright(player))
		{
			player->mo->color = ((skin_t *)player->mo->skin)->supercolor
				+ ((player->nightstime == player->startedtime)
					? 4
					: abs((((signed)leveltime >> 1) % 9) - 4)); // This is where super flashing is handled.
			G_GhostAddColor(GHC_SUPER);
		}

		if (!player->capsule && !player->bonustime)
		{
			thinker_t *th;
			mobj_t *mo2;

			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				mo2 = (mobj_t *)th;
>>>>>>> srb2/next

	{
		INT16 angle_diff, max_left_turn, max_right_turn;
		boolean add_delta = true;

		// Kart: store the current turn range for later use
		if ((player->mo && player->speed > 0) // Moving
			|| (leveltime > starttime && (cmd->buttons & BT_ACCELERATE && cmd->buttons & BT_BRAKE)) // Rubber-burn turn
			|| (player->kartstuff[k_respawn]) // Respawning
			|| (player->spectator || objectplacing)) // Not a physical player
		{
			player->lturn_max[leveltime%MAXPREDICTTICS] = K_GetKartTurnValue(player, KART_FULLTURN)+1;
			player->rturn_max[leveltime%MAXPREDICTTICS] = K_GetKartTurnValue(player, -KART_FULLTURN)-1;
		} else {
			player->lturn_max[leveltime%MAXPREDICTTICS] = player->rturn_max[leveltime%MAXPREDICTTICS] = 0;
		}

		if (leveltime >= starttime)
		{
			// KART: Don't directly apply angleturn! It may have been either A) forged by a malicious client, or B) not be a smooth turn due to a player dropping frames.
			// Instead, turn the player only up to the amount they're supposed to turn accounting for latency. Allow exactly 1 extra turn unit to try to keep old replays synced.
			angle_diff = cmd->angleturn - (player->mo->angle>>16);
			max_left_turn = player->lturn_max[(leveltime + MAXPREDICTTICS - cmd->latency) % MAXPREDICTTICS];
			max_right_turn = player->rturn_max[(leveltime + MAXPREDICTTICS - cmd->latency) % MAXPREDICTTICS];

			//CONS_Printf("----------------\nangle diff: %d - turning options: %d to %d - ", angle_diff, max_left_turn, max_right_turn);

			if (angle_diff > max_left_turn)
				angle_diff = max_left_turn;
			else if (angle_diff < max_right_turn)
				angle_diff = max_right_turn;
			else
			{
				// Try to keep normal turning as accurate to 1.0.1 as possible to reduce replay desyncs.
				player->mo->angle = cmd->angleturn<<16;
				add_delta = false;
			}
			//CONS_Printf("applied turn: %d\n", angle_diff);

			if (add_delta) {
				player->mo->angle += angle_diff<<16;
				player->mo->angle &= ~0xFFFF; // Try to keep the turning somewhat similar to how it was before?
				//CONS_Printf("leftover turn (%s): %5d or %4d%%\n",
				//				player_names[player-players],
				//				(INT16) (cmd->angleturn - (player->mo->angle>>16)),
				//				(INT16) (cmd->angleturn - (player->mo->angle>>16)) * 100 / (angle_diff ? angle_diff : 1));
			}
		}

		ticruned++;
		if ((cmd->angleturn & TICCMD_RECEIVED) == 0)
			ticmiss++;

		P_3dMovement(player);
	}

<<<<<<< HEAD
	// Kart frames
	if (player->kartstuff[k_squishedtimer] > 0)
	{
		if (player->mo->state != &states[S_KART_SQUISH])
			P_SetPlayerMobjState(player->mo, S_KART_SQUISH);
	}
	else if (player->pflags & PF_SLIDING)
	{
		if (player->mo->state != &states[S_KART_SPIN])
			P_SetPlayerMobjState(player->mo, S_KART_SPIN);
		player->frameangle -= ANGLE_22h;
	}
	else if (player->kartstuff[k_spinouttimer] > 0)
	{
		INT32 speed = max(1, min(8, player->kartstuff[k_spinouttimer]/8));
=======
	if (maptol & TOL_2D)
		runspd = FixedMul(runspd, 2*FRACUNIT/3);

	P_SkidStuff(player);

	/////////////////////////
	// MOVEMENT ANIMATIONS //
	/////////////////////////

	if ((cmd->forwardmove != 0 || cmd->sidemove != 0) || (player->powers[pw_super] && !onground))
	{
		// If the player is in dashmode, here's their peelout.
		if (player->charflags & SF_DASHMODE && player->dashmode >= DASHMODE_THRESHOLD && player->panim == PA_RUN && !player->skidtime && (onground || ((player->charability == CA_FLOAT || player->charability == CA_SLOWFALL) && player->secondjump == 1) || player->powers[pw_super]))
			P_SetPlayerMobjState (player->mo, S_PLAY_DASH);
		// If the player is moving fast enough,
		// break into a run!
		else if (player->speed >= runspd && player->panim == PA_WALK && !player->skidtime
		&& (onground || ((player->charability == CA_FLOAT || player->charability == CA_SLOWFALL) && player->secondjump == 1) || player->powers[pw_super]))
		{
			if (!onground)
				P_SetPlayerMobjState(player->mo, S_PLAY_FLOAT_RUN);
			else
				P_SetPlayerMobjState(player->mo, S_PLAY_RUN);
		}

		// Floating at slow speeds has its own special animation.
		else if ((((player->charability == CA_FLOAT || player->charability == CA_SLOWFALL) && player->secondjump == 1) || player->powers[pw_super]) && player->panim == PA_IDLE && !onground)
			P_SetPlayerMobjState (player->mo, S_PLAY_FLOAT);

		// Otherwise, just walk.
		else if ((player->rmomx || player->rmomy) && player->panim == PA_IDLE)
			P_SetPlayerMobjState (player->mo, S_PLAY_WALK);
	}

	// If your peelout animation is playing, and you're
	// going too slow, switch back to the run.
	if (player->charflags & SF_DASHMODE && player->panim == PA_DASH && player->dashmode < DASHMODE_THRESHOLD)
		P_SetPlayerMobjState(player->mo, S_PLAY_RUN);

	// If your running animation is playing, and you're
	// going too slow, switch back to the walking frames.
	if (player->panim == PA_RUN && player->speed < runspd)
	{
		if (!onground && (((player->charability == CA_FLOAT || player->charability == CA_SLOWFALL) && player->secondjump == 1) || player->powers[pw_super]))
			P_SetPlayerMobjState(player->mo, S_PLAY_FLOAT);
		else
			P_SetPlayerMobjState(player->mo, S_PLAY_WALK);
	}

	// Correct floating when ending up on the ground.
	if (onground)
	{
		if (player->mo->state-states == S_PLAY_FLOAT)
			P_SetPlayerMobjState(player->mo, S_PLAY_WALK);
		else if (player->mo->state-states == S_PLAY_FLOAT_RUN)
			P_SetPlayerMobjState(player->mo, S_PLAY_RUN);
	}

	// If Springing (or nojumpspinning), but travelling DOWNWARD, change back!
	if ((player->panim == PA_SPRING && P_MobjFlip(player->mo)*player->mo->momz < 0)
		|| ((((player->charflags & SF_NOJUMPSPIN) && (player->pflags & PF_JUMPED) && player->panim == PA_JUMP))
			&& (P_MobjFlip(player->mo)*player->mo->momz < 0)))
		P_SetPlayerMobjState(player->mo, S_PLAY_FALL);
	// If doing an air animation but on the ground, change back!
	else if (onground && (player->panim == PA_SPRING || player->panim == PA_FALL || player->panim == PA_RIDE || player->panim == PA_JUMP) && !player->mo->momz)
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);

	// If you are stopped and are still walking, stand still!
	if (!player->mo->momx && !player->mo->momy && !player->mo->momz && player->panim == PA_WALK)
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);

//////////////////
//GAMEPLAY STUFF//
//////////////////

	// Make sure you're not "jumping" on the ground
	if (onground && player->pflags & PF_JUMPED && !(player->pflags & PF_GLIDING)
	&& P_MobjFlip(player->mo)*player->mo->momz < 0)
	{
		player->pflags &= ~(PF_STARTJUMP|PF_JUMPED|PF_NOJUMPDAMAGE|PF_THOKKED|PF_SHIELDABILITY);
		player->secondjump = 0;
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);
	}

	if ((!(player->charability == CA_GLIDEANDCLIMB) || player->gotflag) // If you can't glide, then why the heck would you be gliding?
		&& (player->pflags & PF_GLIDING || player->climbing))
	{
		if (onground)
			P_SetPlayerMobjState(player->mo, S_PLAY_WALK);
		else
		{
			player->pflags |= P_GetJumpFlags(player);
			P_SetPlayerMobjState(player->mo, S_PLAY_JUMP);
		}
		player->pflags &= ~PF_GLIDING;
		player->glidetime = 0;
		player->climbing = 0;
	}

	if ((!(player->charability == CA_BOUNCE) || player->gotflag) // If you can't bounce, then why the heck would you be bouncing?
		&& (player->pflags & PF_BOUNCING))
	{
		if (onground)
			P_SetPlayerMobjState(player->mo, S_PLAY_WALK);
		else
		{
			player->pflags |= P_GetJumpFlags(player);
			P_SetPlayerMobjState(player->mo, S_PLAY_JUMP);
		}
		player->pflags &= ~PF_BOUNCING;
	}

	// Glide MOMZ
	// AKA my own gravity. =)
	if (player->pflags & PF_GLIDING)
	{
		mobj_t *mo = player->mo; // seriously why isn't this at the top of the function hngngngng
		fixed_t glidespeed = player->actionspd;
		fixed_t momx = mo->momx - player->cmomx, momy = mo->momy - player->cmomy;
		angle_t angle, moveangle = R_PointToAngle2(0, 0, momx, momy);
		boolean swimming = mo->state - states == S_PLAY_SWIM;
		boolean in2d = mo->flags2 & MF2_TWOD || twodlevel;

		if (player->powers[pw_super] || player->powers[pw_sneakers])
			glidespeed *= 2;

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if (player->mo->momz > FixedMul(2*FRACUNIT, player->mo->scale))
				player->mo->momz -= FixedMul(3*(FRACUNIT/4), player->mo->scale);
		}
		else
		{
			if (player->mo->momz < FixedMul(-2*FRACUNIT, player->mo->scale))
				player->mo->momz += FixedMul(3*(FRACUNIT/4), player->mo->scale);
		}

		// Strafing while gliding.
		if ((P_ControlStyle(player) & CS_LMAOGALOG) || in2d)
			angle = mo->angle;
		else if (swimming)
			angle = mo->angle + R_PointToAngle2(0, 0, cmd->forwardmove<<FRACBITS, -cmd->sidemove<<FRACBITS);
		else
			angle = mo->angle - FixedAngle(cmd->sidemove * FRACUNIT);

		if (!player->skidtime) // TODO: make sure this works in 2D!
		{
			angle_t anglediff = angle - moveangle;
			fixed_t accelfactor = 4*FRACUNIT - 3*FINECOSINE((anglediff >> ANGLETOFINESHIFT) & FINEMASK);
			fixed_t speed, scale = mo->scale;

			if (in2d)
			{
				if (mo->eflags & MFE_UNDERWATER)
					speed = FixedMul((glidespeed>>1) + player->glidetime*750, scale);
				else
					speed = FixedMul(glidespeed + player->glidetime*1500, scale);
				P_InstaThrust(mo, angle, speed);
			}
			else if (swimming)
			{
				fixed_t minspeed;

				if (anglediff > ANGLE_180)
					anglediff = InvAngle(InvAngle(anglediff) >> 3);
				else
					anglediff = anglediff >> 3;

				minspeed = FixedMul((glidespeed>>1) + player->glidetime*750, scale); // underwater-specific
				speed = FixedHypot(momx, momy) - abs(P_ReturnThrustY(mo, anglediff, mo->scale));

				if (speed < minspeed)
				{
					momx += P_ReturnThrustX(mo, angle, FixedMul(accelfactor, scale));
					momy += P_ReturnThrustY(mo, angle, FixedMul(accelfactor, scale));
					speed = FixedHypot(momx, momy); // recalculate speed
				}

				mo->momx = P_ReturnThrustX(mo, moveangle + anglediff, speed) + player->cmomx;
				mo->momy = P_ReturnThrustY(mo, moveangle + anglediff, speed) + player->cmomy;
			}
			else
			{
				fixed_t newMagnitude, oldMagnitude = R_PointToDist2(momx, momy, 0, 0);

				if (mo->eflags & MFE_UNDERWATER)
					speed = FixedMul((glidespeed>>1) + player->glidetime*750, scale);
				else
					speed = FixedMul(glidespeed + player->glidetime*1500, scale);

				P_Thrust(mo, angle, FixedMul(accelfactor, scale));

				newMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);
				if (newMagnitude > speed)
				{
					fixed_t tempmomx, tempmomy;
					if (oldMagnitude > speed)
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
						tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), speed);
						tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), speed);
						player->mo->momx = tempmomx + player->cmomx;
						player->mo->momy = tempmomy + player->cmomy;
					}
				}
			}
		}
>>>>>>> srb2/next

		if (player->mo->state != &states[S_KART_SPIN])
			P_SetPlayerMobjState(player->mo, S_KART_SPIN);

		if (speed == 1 && abs((signed)(player->mo->angle - player->frameangle)) < ANGLE_22h)
			player->frameangle = player->mo->angle; // Face forward at the end of the animation
		else
			player->frameangle -= (ANGLE_11hh * speed);
	}
	else if (player->powers[pw_nocontrol] && player->pflags & PF_SKIDDOWN)
	{
		if (player->mo->state != &states[S_KART_SPIN])
			P_SetPlayerMobjState(player->mo, S_KART_SPIN);

		if (((player->powers[pw_nocontrol] + 5) % 20) < 10)
			player->frameangle += ANGLE_11hh;
		else
			player->frameangle -= ANGLE_11hh;
	}
	else
	{
		K_KartMoveAnimation(player);

		if (player->kartstuff[k_pogospring])
		{
			player->frameangle += ANGLE_22h;
		}
		else
		{
			player->frameangle = player->mo->angle;

			if (player->kartstuff[k_drift] != 0)
			{
				INT32 a = (ANGLE_45 / 5) * player->kartstuff[k_drift];
				player->frameangle += a;
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

	// Little water sound while touching water - just a nicety.
	if ((player->mo->eflags & MFE_TOUCHWATER) && !(player->mo->eflags & MFE_UNDERWATER) && !player->spectator)
	{
		if (P_RandomChance(FRACUNIT/2) && leveltime % TICRATE == 0)
			S_StartSound(player->mo, sfx_floush);
	}

	////////////////////////////
	//SPINNING AND SPINDASHING//
	////////////////////////////

<<<<<<< HEAD
	// SRB2kart - Drifting smoke and fire
	if ((EITHERSNEAKER(player) || player->kartstuff[k_flamedash])
		&& onground && (leveltime & 1))
		K_SpawnBoostTrail(player);
=======
	// If the player isn't on the ground, make sure they aren't in a "starting dash" position.
	if (!onground && player->powers[pw_carry] != CR_NIGHTSMODE)
	{
		player->pflags &= ~PF_STARTDASH;
		player->dashspeed = 0;
	}

	if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL
	&& (player->pflags & PF_SPINNING) && player->speed > FixedMul(4<<FRACBITS, player->mo->scale) && onground && (leveltime & 1)
	&& !(player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
		P_ElementalFire(player, false);

	P_DoSpinAbility(player, cmd);

	// jumping
	P_DoJumpStuff(player, cmd);

	// If you're not spinning, you'd better not be spindashing!
	if (!(player->pflags & PF_SPINNING) && player->powers[pw_carry] != CR_NIGHTSMODE)
		player->pflags &= ~PF_STARTDASH;

	//////////////////
	//ANALOG CONTROL//
	//////////////////

	// This really looks like it should be moved to P_3dMovement. -Red
	if (P_ControlStyle(player) == CS_LMAOGALOG
		&& (cmd->forwardmove != 0 || cmd->sidemove != 0) && !player->climbing && !twodlevel && !(player->mo->flags2 & MF2_TWOD))
	{
		// If travelling slow enough, face the way the controls
		// point and not your direction of movement.
		if (player->speed < FixedMul(5*FRACUNIT, player->mo->scale) || player->pflags & PF_GLIDING || !onground)
		{
			angle_t tempangle;

			tempangle = (cmd->angleturn << 16);

#ifdef REDSANALOG // Ease to it. Chillax. ~Red
			tempangle += R_PointToAngle2(0, 0, cmd->forwardmove*FRACUNIT, -cmd->sidemove*FRACUNIT);
			{
				fixed_t tweenvalue = max(abs(cmd->forwardmove), abs(cmd->sidemove));

				if (tweenvalue < 10 && (cmd->buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)) {
					tempangle = (cmd->angleturn << 16);
					tweenvalue = 16;
				}

				tweenvalue *= tweenvalue*tweenvalue*1536;

				//if (player->pflags & PF_GLIDING)
					//tweenvalue >>= 1;

				tempangle -= player->mo->angle;

				if (tempangle < ANGLE_180 && tempangle > tweenvalue)
					player->mo->angle += tweenvalue;
				else if (tempangle >= ANGLE_180 && InvAngle(tempangle) > tweenvalue)
					player->mo->angle -= tweenvalue;
				else
					player->mo->angle += tempangle;
			}
#else
			// Less math this way ~Red
			player->mo->angle = R_PointToAngle2(0, 0, cmd->forwardmove*FRACUNIT, -cmd->sidemove*FRACUNIT)+tempangle;
#endif
		}
		// Otherwise, face the direction you're travelling.
		else if (player->panim == PA_WALK || player->panim == PA_RUN || player->panim == PA_DASH || player->panim == PA_ROLL || player->panim == PA_JUMP
		|| (player->panim == PA_ABILITY && player->mo->state-states == S_PLAY_GLIDE))
			player->mo->angle = R_PointToAngle2(0, 0, player->rmomx, player->rmomy);

		// Update the local angle control.
		P_SetPlayerAngle(player, player->mo->angle);
	}

	if (player->climbing == 1)
		P_DoClimbing(player);

	if (player->climbing > 1)
	{
		P_InstaThrust(player->mo, player->mo->angle, FixedMul(4*FRACUNIT, player->mo->scale)); // Shove up against the wall
		player->climbing--;
	}

	if (!player->climbing)
	{
		player->lastsidehit = -1;
		player->lastlinehit = -1;
	}

	// Make sure you're not teetering when you shouldn't be.
	if (player->panim == PA_EDGE
	&& (player->mo->momx || player->mo->momy || player->mo->momz))
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);

	// Check for teeter!
	if (!(player->mo->momz || player->mo->momx || player->mo->momy) && !(player->mo->eflags & MFE_GOOWATER)
	&& player->panim == PA_IDLE && !(player->powers[pw_carry]))
		P_DoTeeter(player);

	// Toss a flag
	if (G_GametypeHasTeams() && (cmd->buttons & BT_TOSSFLAG) && !(player->powers[pw_super]) && !(player->tossdelay))
	{
		if (!(player->gotflag & (GF_REDFLAG|GF_BLUEFLAG)))
			P_PlayerEmeraldBurst(player, true); // Toss emeralds
		else
			P_PlayerFlagBurst(player, true);
	}

	// check for fire
	if (!player->exiting)
		P_DoFiring(player, cmd);
>>>>>>> srb2/next

	if (player->kartstuff[k_invincibilitytimer] > 0)
		K_SpawnSparkleTrail(player->mo);

<<<<<<< HEAD
	if (player->kartstuff[k_wipeoutslow] > 1 && (leveltime & 1))
		K_SpawnWipeoutTrail(player->mo, false);
=======
		// Less height while spinning. Good for spinning under things...?
		if ((player->mo->state == &states[player->mo->info->painstate])
		|| ((player->pflags & PF_JUMPED) && !(player->pflags & PF_NOJUMPDAMAGE))
		|| (player->pflags & PF_SPINNING)
		|| player->powers[pw_tailsfly] || player->pflags & PF_GLIDING
		|| (player->charability == CA_GLIDEANDCLIMB && player->mo->state-states == S_PLAY_GLIDE_LANDING)
		|| (player->charability == CA_FLY && player->mo->state-states == S_PLAY_FLY_TIRED))
			player->mo->height = P_GetPlayerSpinHeight(player);
		else
			player->mo->height = P_GetPlayerHeight(player);
>>>>>>> srb2/next

	K_DriftDustHandling(player->mo);

	// Crush test...
	if ((player->mo->ceilingz - player->mo->floorz < player->mo->height)
		&& !(player->mo->flags & MF_NOCLIP))
	{
		/* // SRB2kart - no, we're not making the playerspin
		if ((player->charability2 == CA2_SPINDASH) && !(player->pflags & PF_SPINNING))
		{
			player->pflags |= PF_SPINNING;
			P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
		}
		else */if (player->mo->ceilingz - player->mo->floorz < player->mo->height)
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
	}

<<<<<<< HEAD
=======
#ifdef HWRENDER
	if (rendermode == render_opengl && cv_fovchange.value)
	{
		fixed_t speed;
		const fixed_t runnyspeed = 20*FRACUNIT;

		speed = R_PointToDist2(player->rmomx, player->rmomy, 0, 0);

		if (speed > player->normalspeed-5*FRACUNIT)
			speed = player->normalspeed-5*FRACUNIT;

		if (speed >= runnyspeed)
			player->fovadd = speed-runnyspeed;
		else
			player->fovadd = 0;

		if (player->fovadd < 0)
			player->fovadd = 0;
	}
	else
		player->fovadd = 0;
#endif

>>>>>>> srb2/next
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
		waypoint = reverse ? P_GetPreviousWaypoint(player->mo->tracer, false) : P_GetNextWaypoint(player->mo->tracer, false);
<<<<<<< HEAD
=======

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
}

//
// P_DoRopeHang
//
// Kinda like P_DoZoomTube
// but a little different.
//
static void P_DoRopeHang(player_t *player)
{
	INT32 sequence;
	fixed_t speed;
	mobj_t *waypoint = NULL;
	fixed_t dist;
	fixed_t playerz;

	player->mo->height = P_GetPlayerHeight(player);

	// Play the 'clink' sound only if the player is moving.
	if (!(leveltime & 7) && player->speed)
		S_StartSound(player->mo, sfx_s3k55);

	playerz = player->mo->z + player->mo->height;

	speed = abs(player->speed);

	sequence = player->mo->tracer->threshold;

	// change slope
	dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - playerz);

	if (dist < 1)
		dist = 1;

	player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
	player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
	player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - playerz, dist), (speed));

	if (player->cmd.buttons & BT_USE && !(player->pflags & PF_STASIS)) // Drop off of the rope
	{
		player->pflags |= (P_GetJumpFlags(player)|PF_USEDOWN);
		P_SetPlayerMobjState(player->mo, S_PLAY_JUMP);

		P_SetTarget(&player->mo->tracer, NULL);
		player->powers[pw_carry] = CR_NONE;

		return;
	}

	if (player->mo->state-states != S_PLAY_RIDE)
		P_SetPlayerMobjState(player->mo, S_PLAY_RIDE);

	// If not allowed to move, we're done here.
	if (!speed)
		return;

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
		player->mo->z = player->mo->tracer->z - player->mo->height;
		playerz = player->mo->tracer->z;
		P_SetThingPosition(player->mo);

		CONS_Debug(DBG_GAMELOGIC, "Looking for next waypoint...\n");

		// Find next waypoint
		waypoint = P_GetNextWaypoint(player->mo->tracer, false);

		if (!(player->mo->tracer->flags & MF_SLIDEME) && !waypoint)
		{
			CONS_Debug(DBG_GAMELOGIC, "Next waypoint not found, wrapping to start...\n");

			// Wrap around back to first waypoint
			waypoint = P_GetFirstWaypoint(sequence);
		}
>>>>>>> srb2/next

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

		if (player == &players[consoleplayer])
			localangle[0] = player->mo->angle;
		else if (player == &players[displayplayers[1]])
			localangle[1] = player->mo->angle;
		else if (player == &players[displayplayers[2]])
			localangle[2] = player->mo->angle;
		else if (player == &players[displayplayers[3]])
			localangle[3] = player->mo->angle;
	}

#if 0
	if (player->mo->state != &states[S_KART_SPIN])
		P_SetPlayerMobjState(player->mo, S_KART_SPIN);
	player->frameangle -= ANGLE_22h;
#endif
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

		P_DamageMobj(players[i].mo, player->mo, player->mo, 1, 0);
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
	const fixed_t ns = 60 * mapobjectscale;
	mobj_t *mo;
	angle_t fa;
	thinker_t *think;
	INT32 i;

	radius = FixedMul(radius, mapobjectscale);

	for (i = 0; i < 16; i++)
	{
		fa = (i*(FINEANGLES/16));
		mo = P_SpawnMobj(inflictor->x, inflictor->y, inflictor->z, MT_SUPERSPARK);
		if (!P_MobjWasRemoved(mo))
		{
			mo->momx = FixedMul(FINESINE(fa),ns);
			mo->momy = FixedMul(FINECOSINE(fa),ns);
		}
	}

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
			P_DamageMobj(mo, inflictor, source, 1, DMG_NUKE);
		else
			P_DamageMobj(mo, inflictor, source, 1000, DMG_NUKE);
	}
}

<<<<<<< HEAD
=======
//
// P_Earthquake
// Used for Super Knuckles' landing - damages enemies within the given radius
//
void P_Earthquake(mobj_t *inflictor, mobj_t *source, fixed_t radius)
{
	const fixed_t scaledradius = FixedMul(radius, inflictor->scale);
	const fixed_t ns = scaledradius/12;
	mobj_t *mo;
	angle_t fa;
	INT32 i;
	boolean grounded = P_IsObjectOnGround(inflictor);

	for (i = 0; i < 16; i++)
	{
		fa = (i*(FINEANGLES/16));
		mo = P_SpawnMobjFromMobj(inflictor, 0, 0, 0, MT_SUPERSPARK);
		if (!P_MobjWasRemoved(mo))
		{
			if (grounded)
			{
				mo->momx = FixedMul(FINESINE(fa),ns);
				mo->momy = FixedMul(FINECOSINE(fa),ns);
			}
			else
			{
				P_InstaThrust(mo, inflictor->angle + ANGLE_90, FixedMul(FINECOSINE(fa),ns));
				mo->momz = FixedMul(FINESINE(fa),ns);
			}
		}
	}

	if (inflictor->player && P_IsLocalPlayer(inflictor->player))
	{
		quake.epicenter = NULL;
		quake.intensity = 8*inflictor->scale;
		quake.time = 8;
		quake.radius = scaledradius;
	}

	P_RadiusAttack(inflictor, source, radius, 0, false);
}

//
// P_LookForFocusTarget
// Looks for a target for a player to focus on, for Z-targeting etc.
// exclude would be the current focus target, to ignore.
// direction, if set, requires the target to be to the left (1) or right (-1) of the angle
// mobjflags can be used to limit the flags of objects that can be focused
//
mobj_t *P_LookForFocusTarget(player_t *player, mobj_t *exclude, SINT8 direction, UINT8 lockonflags)
{
	mobj_t *mo;
	thinker_t *think;
	mobj_t *closestmo = NULL;
	const fixed_t maxdist = 2560*player->mo->scale;
	const angle_t span = ANGLE_45;
	fixed_t dist, closestdist = 0;
	angle_t dangle, closestdangle = 0;

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)think;

		if (mo->flags & MF_NOCLIPTHING)
			continue;

		if (mo == player->mo || mo == exclude)
			continue;

		if (mo->health <= 0) // dead
			continue;

		switch (mo->type)
		{
		case MT_TNTBARREL:
			if (lockonflags & LOCK_INTERESTS)
				break;
			/*FALLTHRU*/
		case MT_PLAYER: // Don't chase other players!
		case MT_DETON:
			continue; // Don't be STUPID, Sonic!

		case MT_FAKEMOBILE:
			if (!(lockonflags & LOCK_BOSS))
				continue;
			break;

		case MT_EGGSHIELD:
			if (!(lockonflags & LOCK_ENEMY))
				continue;
			break;

		case MT_EGGSTATUE:
			if (tutorialmode)
				break; // Always focus egg statue in the tutorial
			/*FALLTHRU*/
		default:

			if ((lockonflags & LOCK_BOSS) && ((mo->flags & (MF_BOSS|MF_SHOOTABLE)) == (MF_BOSS|MF_SHOOTABLE))) // allows if it has the flags desired XOR it has the invert aimable flag
			{
				if (mo->flags2 & MF2_FRET)
					continue;
				break;
			}

			if ((lockonflags & LOCK_ENEMY) && (!((mo->flags & (MF_ENEMY|MF_SHOOTABLE)) == (MF_ENEMY|MF_SHOOTABLE)) != !(mo->flags2 & MF2_INVERTAIMABLE))) // allows if it has the flags desired XOR it has the invert aimable flag
				break;

			if ((lockonflags & LOCK_INTERESTS) && (mo->flags & (MF_PUSHABLE|MF_MONITOR))) // allows if it has the flags desired XOR it has the invert aimable flag
				break;

			continue; // not a valid object
		}

		{
			fixed_t zdist = (player->mo->z + player->mo->height/2) - (mo->z + mo->height/2);
			dist = P_AproxDistance(player->mo->x-mo->x, player->mo->y-mo->y);

			if (abs(zdist) > dist)
				continue; // Don't home outside of desired angle!

			dist = P_AproxDistance(dist, zdist);
			if (dist > maxdist)
				continue; // out of range
		}

		if ((twodlevel || player->mo->flags2 & MF2_TWOD)
		&& abs(player->mo->y-mo->y) > player->mo->radius)
			continue; // not in your 2d plane

		dangle = R_PointToAngle2(player->mo->x, player->mo->y, mo->x, mo->y) - (
			!exclude ? player->mo->angle : R_PointToAngle2(player->mo->x, player->mo->y, exclude->x, exclude->y)
		);

		if (direction)
		{
			if (direction == 1 && dangle > ANGLE_180)
				continue; // To the right of the player
			if (direction == -1 && dangle < ANGLE_180)
				continue; // To the left of the player
		}

		if (dangle > ANGLE_180)
			dangle = InvAngle(dangle);

		if (dangle > span)
			continue; // behind back

		// Inflate dist by angle difference to bias toward objects at a closer angle
		dist = FixedDiv(dist, FINECOSINE(dangle>>ANGLETOFINESHIFT)*3);

		if (closestmo && (exclude ? (dangle > closestdangle) : (dist > closestdist)))
			continue;

		if (!P_CheckSight(player->mo, mo))
			continue; // out of sight

		closestmo = mo;
		closestdist = dist;
		closestdangle = dangle;
	}

	return closestmo;
}

//
// P_LookForEnemies
// Looks for something you can hit - Used for homing attack
// If nonenemies is true, includes monitors and springs!
// If bullet is true, you can look up and the distance is further,
// but your total angle span you can look is limited to compensate. (Also, allows monitors.)
// If you modify this, please also modify P_HomingAttack.
//
mobj_t *P_LookForEnemies(player_t *player, boolean nonenemies, boolean bullet)
{
	mobj_t *mo;
	thinker_t *think;
	mobj_t *closestmo = NULL;
	const fixed_t maxdist = FixedMul((bullet ? RING_DIST*2 : RING_DIST), player->mo->scale);
	const angle_t span = (bullet ? ANG30 : ANGLE_90);
	fixed_t dist, closestdist = 0;
	const mobjflag_t nonenemiesdisregard = (bullet ? 0 : MF_MONITOR)|MF_SPRING;

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)think;

		if (mo->flags & MF_NOCLIPTHING)
			continue;

		if (mo->health <= 0) // dead
			continue;

		if (!((mo->flags & (MF_ENEMY|MF_BOSS|MF_MONITOR) && (mo->flags & MF_SHOOTABLE)) || (mo->flags & MF_SPRING)) == !(mo->flags2 & MF2_INVERTAIMABLE)) // allows if it has the flags desired XOR it has the invert aimable flag
			continue; // not a valid target

		if (mo == player->mo)
			continue;

		if (mo->flags2 & MF2_FRET)
			continue;

		if (!nonenemies && mo->flags & nonenemiesdisregard)
			continue;

		if (!bullet && mo->type == MT_DETON) // Don't be STUPID, Sonic!
			continue;

		{
			fixed_t zdist = (player->mo->z + player->mo->height/2) - (mo->z + mo->height/2);
			dist = P_AproxDistance(player->mo->x-mo->x, player->mo->y-mo->y);
			if (bullet)
			{
				if ((R_PointToAngle2(0, 0, dist, zdist) + span) > span*2)
					continue; // Don't home outside of desired angle!
			}
			else // Don't home upwards!
			{
				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					if (mo->z+mo->height < player->mo->z+player->mo->height-FixedMul(MAXSTEPMOVE, player->mo->scale))
							continue;
				}
				else if (mo->z > player->mo->z+FixedMul(MAXSTEPMOVE, player->mo->scale))
					continue;
			}

			dist = P_AproxDistance(dist, zdist);
			if (dist > maxdist)
				continue; // out of range
		}

		if ((twodlevel || player->mo->flags2 & MF2_TWOD)
		&& abs(player->mo->y-mo->y) > player->mo->radius)
			continue; // not in your 2d plane

		if (mo->type == MT_PLAYER) // Don't chase after other players!
			continue;

		if (closestmo && dist > closestdist)
			continue;

		if ((R_PointToAngle2(player->mo->x + P_ReturnThrustX(player->mo, player->mo->angle, player->mo->radius), player->mo->y + P_ReturnThrustY(player->mo, player->mo->angle, player->mo->radius), mo->x, mo->y) - player->mo->angle + span) > span*2)
			continue; // behind back

		if (!P_CheckSight(player->mo, mo))
			continue; // out of sight

		closestmo = mo;
		closestdist = dist;
	}

	return closestmo;
}

boolean P_HomingAttack(mobj_t *source, mobj_t *enemy) // Home in on your target
{
	fixed_t zdist;
	fixed_t dist;
	fixed_t ns = 0;

	if (!enemy)
		return false;

	if (enemy->flags & MF_NOCLIPTHING)
		return false;

	if (enemy->health <= 0) // dead
		return false;

	if (source->player && (!((enemy->flags & (MF_ENEMY|MF_BOSS|MF_MONITOR) && (enemy->flags & MF_SHOOTABLE)) || (enemy->flags & MF_SPRING)) == !(enemy->flags2 & MF2_INVERTAIMABLE))) // allows if it has the flags desired XOR it has the invert aimable flag
		return false;

	if (enemy->flags2 & MF2_FRET)
		return false;

	// change angle
	source->angle = R_PointToAngle2(source->x, source->y, enemy->x, enemy->y);
	if (source->player)
	{
		source->player->drawangle = source->angle;
		if (!demoplayback || P_ControlStyle(source->player) == CS_LMAOGALOG)
			P_SetPlayerAngle(source->player, source->angle);
	}

	// change slope
	zdist = ((P_MobjFlip(source) == -1) ? (enemy->z + enemy->height) - (source->z + source->height) : (enemy->z - source->z));
	dist = P_AproxDistance(P_AproxDistance(enemy->x - source->x, enemy->y - source->y), zdist);

	if (dist < 1)
		dist = 1;

	if (source->type == MT_DETON && enemy->player) // For Deton Chase (Unused)
		ns = FixedDiv(FixedMul(enemy->player->normalspeed, enemy->scale), FixedDiv(20*FRACUNIT,17*FRACUNIT));
	else if (source->type != MT_PLAYER)
	{
		if (source->threshold == 32000)
			ns = FixedMul(source->info->speed/2, source->scale);
		else
			ns = FixedMul(source->info->speed, source->scale);
	}
	else if (source->player)
	{
		if (source->player->charability == CA_HOMINGTHOK && !(source->player->pflags & PF_SHIELDABILITY))
			ns = FixedDiv(FixedMul(source->player->actionspd, source->scale), 3*FRACUNIT/2);
		else
			ns = FixedMul(45*FRACUNIT, source->scale);
	}

	source->momx = FixedMul(FixedDiv(enemy->x - source->x, dist), ns);
	source->momy = FixedMul(FixedDiv(enemy->y - source->y, dist), ns);
	source->momz = FixedMul(FixedDiv(zdist, dist), ns);

	return true;
}

// Search for emeralds
void P_FindEmerald(void)
{
	thinker_t *th;
	mobj_t *mo2;

	hunt1 = hunt2 = hunt3 = NULL;

	// scan the remaining thinkers
	// to find all emeralds
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo2 = (mobj_t *)th;
		if (mo2->type == MT_EMERHUNT)
		{
			if (!hunt1)
				hunt1 = mo2;
			else if (!hunt2)
				hunt2 = mo2;
			else if (!hunt3)
				hunt3 = mo2;
		}
	}
	return;
}

//
// P_GetLives
// Get extra lives in new co-op if you're allowed to.
//

boolean P_GetLives(player_t *player)
{
	INT32 i, maxlivesplayer = -1, livescheck = 1;
	if (!(netgame || multiplayer)
	|| !G_GametypeUsesCoopLives()
	|| (player->lives == INFLIVES))
		return true;

	if (cv_cooplives.value == 0) // infinite lives
	{
		if (player->lives < 1)
			player->lives = 1;
		return true;
	}

	if ((cv_cooplives.value == 2 || cv_cooplives.value == 1) && player->lives > 0)
		return true;

	if (cv_cooplives.value == 1)
		return false;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (players[i].lives > livescheck)
		{
			maxlivesplayer = i;
			livescheck = players[i].lives;
		}
	}
	if (maxlivesplayer != -1 && &players[maxlivesplayer] != player)
	{
		if (cv_cooplives.value == 2 && (P_IsLocalPlayer(player) || P_IsLocalPlayer(&players[maxlivesplayer])))
			S_StartSound(NULL, sfx_jshard); // placeholder
		if (players[maxlivesplayer].lives != INFLIVES)
			players[maxlivesplayer].lives--;
		player->lives++;
		if (player->lives < 1)
			player->lives = 1;
		return true;
	}
	return (player->lives > 0);
}
>>>>>>> srb2/next

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
	INT32 j = MAXPLAYERS;
<<<<<<< HEAD
=======

	ticcmd_t *cmd = &player->cmd;
	player->deltaviewheight = 0;

	if (player->deadtimer < INT32_MAX)
		player->deadtimer++;

	if (player->bot) // don't allow bots to do any of the below, B_CheckRespawn does all they need for respawning already
		goto notrealplayer;

	// continue logic
	if (!(netgame || multiplayer) && player->lives <= 0)
	{
		if (player->deadtimer > (3*TICRATE) && (cmd->buttons & BT_USE || cmd->buttons & BT_JUMP) && (!continuesInSession || player->continues > 0))
			G_UseContinue();
		else if (player->deadtimer >= gameovertics)
			G_UseContinue(); // Even if we don't have one this handles ending the game
	}

	if ((cv_cooplives.value != 1)
	&& (G_GametypeUsesCoopLives())
	&& (netgame || multiplayer)
	&& (player->lives <= 0))
	{
		for (j = 0; j < MAXPLAYERS; j++)
		{
			if (!playeringame[j])
				continue;

			if (players[j].lives > 1)
				break;
		}
	}

	// Force respawn if idle for more than 30 seconds in shooter modes.
	if (player->deadtimer > 30*TICRATE && !G_PlatformGametype())
		player->playerstate = PST_REBORN;
	else if ((player->lives > 0 || j != MAXPLAYERS) && !(!(netgame || multiplayer) && G_IsSpecialStage(gamemap))) // Don't allow "click to respawn" in special stages!
	{
		if (G_GametypeUsesCoopStarposts() && (netgame || multiplayer) && cv_coopstarposts.value == 2)
		{
			P_ConsiderAllGone();
			if ((player->deadtimer > TICRATE<<1) || ((cmd->buttons & BT_JUMP) && (player->deadtimer > TICRATE)))
			{
				//player->spectator = true;
				player->outofcoop = true;
				player->playerstate = PST_REBORN;
			}
		}
		else
		{
			// Respawn with jump button, force respawn time (3 second default, cheat protected) in shooter modes.
			if (cmd->buttons & BT_JUMP)
			{
				// You're a spectator, so respawn right away.
				if ((gametyperules & GTR_SPECTATORS) && player->spectator)
					player->playerstate = PST_REBORN;
				else
				{
					// Give me one second.
					INT32 respawndelay = TICRATE;
>>>>>>> srb2/next

	ticcmd_t *cmd = &player->cmd;
	player->deltaviewheight = 0;

	if (player->deadtimer < INT32_MAX)
		player->deadtimer++;

	if (player->bot) // don't allow bots to do any of the below, B_CheckRespawn does all they need for respawning already
		goto notrealplayer;

	if ((player->pflags & PF_TIMEOVER) && G_RaceGametype())
	{
		player->karthud[khud_timeovercam]++;

		if (player->mo)
		{
			player->mo->flags |= (MF_NOGRAVITY|MF_NOCLIP);
			player->mo->flags2 |= MF2_DONTDRAW;
		}
<<<<<<< HEAD
=======
		//else if (G_CoopGametype()) -- moved to G_DoReborn
>>>>>>> srb2/next
	}
	else
		player->karthud[khud_timeovercam] = 0;

	K_KartPlayerHUDUpdate(player);

<<<<<<< HEAD
	// Force respawn if idle for more than 30 seconds in shooter modes.
	if (player->lives > 0 /*&& leveltime >= starttime*/) // *could* you respawn?
=======
	if (G_CoopGametype() && (multiplayer || netgame) && (player->lives <= 0) && (player->deadtimer >= 8*TICRATE || ((cmd->buttons & BT_JUMP) && (player->deadtimer > TICRATE))))
>>>>>>> srb2/next
	{
		// SRB2kart - spawn automatically after 1 second
		if (player->deadtimer > ((netgame || multiplayer)
			? cv_respawntime.value*TICRATE
			: TICRATE)) // don't let them change it in record attack
				player->playerstate = PST_REBORN;
	}

<<<<<<< HEAD
	// Keep time rolling
	if (!(exitcountdown && !racecountdown) && !(player->exiting || mapreset) && !(player->pflags & PF_TIMEOVER))
=======
	if ((gametyperules & GTR_RACE) || (G_CoopGametype() && (multiplayer || netgame)))
>>>>>>> srb2/next
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
<<<<<<< HEAD
		else
		{
			player->realtime = 0;
			if (player == &players[consoleplayer])
				curlap = 0;
		}
=======

		// Return to level music
		if (!G_CoopGametype() && player->lives <= 0 && player->deadtimer == gameovertics)
			P_RestoreMultiMusic(player);
>>>>>>> srb2/next
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
	if (cv_cam_rotate.value < 0)
		CV_SetValue(&cv_cam_rotate, cv_cam_rotate.value + 360);
	else if (cv_cam_rotate.value > 359)
		CV_SetValue(&cv_cam_rotate, cv_cam_rotate.value % 360);
}

static void CV_CamRotate2_OnChange(void)
{
	if (cv_cam2_rotate.value < 0)
		CV_SetValue(&cv_cam2_rotate, cv_cam2_rotate.value + 360);
	else if (cv_cam2_rotate.value > 359)
		CV_SetValue(&cv_cam2_rotate, cv_cam2_rotate.value % 360);
}

static void CV_CamRotate3_OnChange(void)
{
	if (cv_cam3_rotate.value < 0)
		CV_SetValue(&cv_cam3_rotate, cv_cam3_rotate.value + 360);
	else if (cv_cam3_rotate.value > 359)
		CV_SetValue(&cv_cam3_rotate, cv_cam3_rotate.value % 360);
}

static void CV_CamRotate4_OnChange(void)
{
	if (cv_cam4_rotate.value < 0)
		CV_SetValue(&cv_cam4_rotate, cv_cam4_rotate.value + 360);
	else if (cv_cam4_rotate.value > 359)
		CV_SetValue(&cv_cam4_rotate, cv_cam4_rotate.value % 360);
}

static CV_PossibleValue_t CV_CamSpeed[] = {{0, "MIN"}, {1*FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t rotation_cons_t[] = {{1, "MIN"}, {25, "MAX"}, {0, NULL}};
static CV_PossibleValue_t CV_CamRotate[] = {{-720, "MIN"}, {720, "MAX"}, {0, NULL}};
static CV_PossibleValue_t multiplier_cons_t[] = {{0, "MIN"}, {3*FRACUNIT, "MAX"}, {0, NULL}};

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
	{"cam2_rotate", "0", CV_CALL|CV_NOINIT, CV_CamRotate, CV_CamRotate_OnChange, 0, NULL, NULL, 0, 0, NULL},
	{"cam3_rotate", "0", CV_CALL|CV_NOINIT, CV_CamRotate, CV_CamRotate_OnChange, 0, NULL, NULL, 0, 0, NULL},
	{"cam4_rotate", "0", CV_CALL|CV_NOINIT, CV_CamRotate, CV_CamRotate_OnChange, 0, NULL, NULL, 0, 0, NULL}
};

consvar_t cv_cam_rotspeed[MAXSPLITSCREENPLAYERS] = {
	{"cam_rotspeed", "10", CV_SAVE, rotation_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam2_rotspeed", "10", CV_SAVE, rotation_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam3_rotspeed", "10", CV_SAVE, rotation_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL},
	{"cam4_rotspeed", "10", CV_SAVE, rotation_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL}
};

fixed_t t_cam_dist[MAXSPLITSCREENPLAYERS] = -42;
fixed_t t_cam_height[MAXSPLITSCREENPLAYERS] = -42;
fixed_t t_cam_rotate[MAXSPLITSCREENPLAYERS] = -42;

// Heavily simplified version of G_BuildTicCmd that only takes the local first player's control input and converts it to readable ticcmd_t
// we then throw that ticcmd garbage in the camera and make it move

<<<<<<< HEAD
// redefine this
static fixed_t forwardmove[2] = {25<<FRACBITS>>16, 50<<FRACBITS>>16};
static fixed_t sidemove[2] = {2<<FRACBITS>>16, 4<<FRACBITS>>16};
static fixed_t angleturn[3] = {KART_FULLTURN/2, KART_FULLTURN, KART_FULLTURN/4}; // + slow turn
=======
	if ((thiscam == &camera && G_ControlStyle(1) == CS_SIMPLE)
	|| (thiscam == &camera2 && G_ControlStyle(2) == CS_SIMPLE))
	{
		thiscam->angle = P_GetLocalAngle(player);
		thiscam->aiming = (thiscam == &camera) ? localaiming : localaiming2;
	}
	else if (!(thiscam == &camera && (cv_cam_still.value || cv_analog[0].value))
	&& !(thiscam == &camera2 && (cv_cam2_still.value || cv_analog[1].value)))
	{
		thiscam->angle = player->mo->angle;
		thiscam->aiming = 0;
	}
	thiscam->relativex = 0;
>>>>>>> srb2/next

static ticcmd_t cameracmd;

struct demofreecam_s democam;

// called by m_menu to reinit cam input every time it's toggled
void P_InitCameraCmd(void)
{
	memset(&cameracmd, 0, sizeof(ticcmd_t));	// initialize cmd
}

static ticcmd_t *P_CameraCmd(camera_t *cam)
{
<<<<<<< HEAD
	INT32 laim, th, tspeed, forward, side, axis; //i
	const INT32 speed = 1;
	// these ones used for multiple conditions
	boolean turnleft, turnright, mouseaiming;
	boolean invertmouse, lookaxis, usejoystick, kbl;
	angle_t lang;
	INT32 player_invert;
	INT32 screen_invert;
=======
	angle_t angle = 0, focusangle = 0, focusaiming = 0;
	fixed_t x, y, z, dist, distxy, distz, checkdist, viewpointx, viewpointy, camspeed, camdist, camheight, pviewheight, slopez = 0;
	INT32 camrotate;
	boolean camstill, cameranoclip, camorbit;
	mobj_t *mo, *sign = NULL;
	subsector_t *newsubsec;
	fixed_t f1, f2;

	static fixed_t camsideshift[2] = {0, 0};
	fixed_t shiftx = 0, shifty = 0;

	// We probably shouldn't move the camera if there is no player or player mobj somehow
	if (!player || !player->mo)
		return true;

	mo = player->mo;

	if (player->playerstate == PST_REBORN)
	{
		P_CalcChasePostImg(player, thiscam);
		return true;
	}

	if (player->exiting)
	{
		if (mo->target && mo->target->type == MT_SIGN && mo->target->spawnpoint
		&& !((gametyperules & GTR_FRIENDLY) && (netgame || multiplayer) && cv_exitmove.value)
		&& !(twodlevel || (mo->flags2 & MF2_TWOD)))
			sign = mo->target;
		else if ((player->powers[pw_carry] == CR_NIGHTSMODE)
		&& !(player->mo->state >= &states[S_PLAY_NIGHTS_TRANS1]
		&& player->mo->state <= &states[S_PLAY_NIGHTS_TRANS6]))
		{
			P_CalcChasePostImg(player, thiscam);
			return true;
		}
	}

	cameranoclip = (sign || player->powers[pw_carry] == CR_NIGHTSMODE || player->pflags & PF_NOCLIP) || (mo->flags & (MF_NOCLIP|MF_NOCLIPHEIGHT)); // Noclipping player camera noclips too!!

	if (!(player->climbing || (player->powers[pw_carry] == CR_NIGHTSMODE) || player->playerstate == PST_DEAD || tutorialmode))
	{
		if (player->spectator) // force cam off for spectators
			return true;

		if (!cv_chasecam.value && thiscam == &camera)
			return true;

		if (!cv_chasecam2.value && thiscam == &camera2)
			return true;
	}

	if (!thiscam->chase && !resetcalled)
	{
		if (player == &players[consoleplayer])
			focusangle = localangle;
		else if (player == &players[secondarydisplayplayer])
			focusangle = localangle2;
		else
			focusangle = mo->angle;
		if (thiscam == &camera)
			camrotate = cv_cam_rotate.value;
		else if (thiscam == &camera2)
			camrotate = cv_cam2_rotate.value;
		else
			camrotate = 0;
		thiscam->angle = focusangle + FixedAngle(camrotate*FRACUNIT);
		P_ResetCamera(player, thiscam);
		return true;
	}

	thiscam->radius = FixedMul(20*FRACUNIT, mo->scale);
	thiscam->height = FixedMul(16*FRACUNIT, mo->scale);

	// Don't run while respawning from a starpost
	// Inu 4/8/13 Why not?!
//	if (leveltime > 0 && timeinmap <= 0)
//		return true;

	if (player->powers[pw_carry] == CR_NIGHTSMODE)
	{
		focusangle = mo->angle;
		focusaiming = 0;
	}
	else if (sign)
	{
		focusangle = FixedAngle(sign->spawnpoint->angle << FRACBITS) + ANGLE_180;
		focusaiming = 0;
	}
	else if (player == &players[consoleplayer])
	{
		focusangle = localangle;
		focusaiming = localaiming;
	}
	else if (player == &players[secondarydisplayplayer])
	{
		focusangle = localangle2;
		focusaiming = localaiming2;
	}
	else
	{
		focusangle = player->cmd.angleturn << 16;
		focusaiming = player->aiming;
	}

	if (P_CameraThinker(player, thiscam, resetcalled))
		return true;

	if (tutorialmode)
	{
		// force defaults because we have a camera look section
		camspeed = (INT32)(atof(cv_cam_speed.defaultvalue) * FRACUNIT);
		camstill = (!stricmp(cv_cam_still.defaultvalue, "off")) ? false : true;
		camorbit = (!stricmp(cv_cam_orbit.defaultvalue, "off")) ? false : true;
		camrotate = atoi(cv_cam_rotate.defaultvalue);
		camdist = FixedMul((INT32)(atof(cv_cam_dist.defaultvalue) * FRACUNIT), mo->scale);
		camheight = FixedMul((INT32)(atof(cv_cam_height.defaultvalue) * FRACUNIT), mo->scale);
	}
	else if (thiscam == &camera)
	{
		camspeed = cv_cam_speed.value;
		camstill = cv_cam_still.value;
		camorbit = cv_cam_orbit.value;
		camrotate = cv_cam_rotate.value;
		camdist = FixedMul(cv_cam_dist.value, mo->scale);
		camheight = FixedMul(cv_cam_height.value, mo->scale);
	}
	else // Camera 2
	{
		camspeed = cv_cam2_speed.value;
		camstill = cv_cam2_still.value;
		camorbit = cv_cam2_orbit.value;
		camrotate = cv_cam2_rotate.value;
		camdist = FixedMul(cv_cam2_dist.value, mo->scale);
		camheight = FixedMul(cv_cam2_height.value, mo->scale);
	}

	if (!(twodlevel || (mo->flags2 & MF2_TWOD)) && !(player->powers[pw_carry] == CR_NIGHTSMODE))
		camheight = FixedMul(camheight, player->camerascale);

#ifdef REDSANALOG
	if (P_ControlStyle(player) == CS_LMAOGALOG && (player->cmd.buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)) {
		camstill = true;

		if (camspeed < 4*FRACUNIT/5)
			camspeed = 4*FRACUNIT/5;
	}
#endif // REDSANALOG

	if (mo->eflags & MFE_VERTICALFLIP)
		camheight += thiscam->height;

	if (twodlevel || (mo->flags2 & MF2_TWOD))
		angle = ANGLE_90;
	else if (camstill || resetcalled || player->playerstate == PST_DEAD)
		angle = thiscam->angle;
	else if (player->powers[pw_carry] == CR_NIGHTSMODE) // NiGHTS Level
	{
		if ((player->pflags & PF_TRANSFERTOCLOSEST) && player->axis1 && player->axis2)
		{
			angle = R_PointToAngle2(player->axis1->x, player->axis1->y, player->axis2->x, player->axis2->y);
			angle += ANGLE_90;
		}
		else if (mo->target)
		{
			if (mo->target->flags2 & MF2_AMBUSH)
				angle = R_PointToAngle2(mo->target->x, mo->target->y, mo->x, mo->y);
			else
				angle = R_PointToAngle2(mo->x, mo->y, mo->target->x, mo->target->y);
		}
	}
	else if (P_ControlStyle(player) == CS_LMAOGALOG && !sign) // Analog
		angle = R_PointToAngle2(thiscam->x, thiscam->y, mo->x, mo->y);
	else if (demoplayback)
	{
		angle = focusangle;
		focusangle = R_PointToAngle2(thiscam->x, thiscam->y, mo->x, mo->y);
		if (player == &players[consoleplayer])
		{
			if (focusangle >= localangle)
				P_ForceLocalAngle(player, localangle + (abs((signed)(focusangle - localangle))>>5));
			else
				P_ForceLocalAngle(player, localangle - (abs((signed)(focusangle - localangle))>>5));
		}
	}
	else
		angle = focusangle + FixedAngle(camrotate*FRACUNIT);

	if (!resetcalled && (cv_analog[0].value || demoplayback) && ((thiscam == &camera && t_cam_rotate != -42) || (thiscam == &camera2
		&& t_cam2_rotate != -42)))
	{
		angle = FixedAngle(camrotate*FRACUNIT);
		thiscam->angle = angle;
	}

	if ((((thiscam == &camera) && cv_analog[0].value) || ((thiscam != &camera) && cv_analog[1].value) || demoplayback) && !sign && !objectplacing && !(twodlevel || (mo->flags2 & MF2_TWOD)) && (player->powers[pw_carry] != CR_NIGHTSMODE) && displayplayer == consoleplayer)
	{
#ifdef REDSANALOG
		if ((player->cmd.buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)); else
#endif
		if (player->cmd.buttons & BT_CAMRIGHT)
		{
			if (thiscam == &camera)
				angle -= FixedAngle(cv_cam_rotspeed.value*FRACUNIT);
			else
				angle -= FixedAngle(cv_cam2_rotspeed.value*FRACUNIT);
		}
		else if (player->cmd.buttons & BT_CAMLEFT)
		{
			if (thiscam == &camera)
				angle += FixedAngle(cv_cam_rotspeed.value*FRACUNIT);
			else
				angle += FixedAngle(cv_cam2_rotspeed.value*FRACUNIT);
		}
	}

	if (G_ControlStyle((thiscam == &camera) ? 1 : 2) == CS_SIMPLE && !sign)
	{
		// Shift the camera slightly to the sides depending on the player facing direction
		UINT8 forplayer = (thiscam == &camera) ? 0 : 1;
		fixed_t shift = FixedMul(FINESINE((player->mo->angle - angle) >> ANGLETOFINESHIFT), cv_cam_shiftfacing[forplayer].value);

		if (player->powers[pw_carry] == CR_NIGHTSMODE)
		{
			fixed_t cos = FINECOSINE((angle_t) (player->flyangle * ANG1)>>ANGLETOFINESHIFT);
			shift = FixedMul(shift, min(FRACUNIT, player->speed*abs(cos)/6000));
			shift += FixedMul(camsideshift[forplayer] - shift, FRACUNIT-(camspeed>>2));
		}
		else if (ticcmd_centerviewdown[(thiscam == &camera) ? 0 : 1])
			shift = FixedMul(camsideshift[forplayer], FRACUNIT-camspeed);
		else
			shift += FixedMul(camsideshift[forplayer] - shift, FRACUNIT-(camspeed>>3));
		camsideshift[forplayer] = shift;

		shift = FixedMul(shift, camdist);
		shiftx = -FixedMul(FINESINE(angle>>ANGLETOFINESHIFT), shift);
		shifty = FixedMul(FINECOSINE(angle>>ANGLETOFINESHIFT), shift);
	}
>>>>>>> srb2/next

	ticcmd_t *cmd = &cameracmd;

	(void)cam;

	if (!demo.playback)
		return cmd;	// empty cmd, no.

	lang = democam.localangle;
	laim = democam.localaiming;
	th = democam.turnheld;
	kbl = democam.keyboardlook;

	G_CopyTiccmd(cmd, I_BaseTiccmd(), 1); // empty, or external driver

	cmd->angleturn = (INT16)(lang >> 16);
	cmd->aiming = G_ClipAimingPitch(&laim);

	mouseaiming = true;
	invertmouse = cv_invertmouse.value;
	lookaxis = cv_lookaxis.value;

<<<<<<< HEAD
	usejoystick = true;
	turnright = InputDown(gc_turnright, 1);
	turnleft = InputDown(gc_turnleft, 1);
=======
	if (camorbit) //Sev here, I'm guessing this is where orbital cam lives
	{
#ifdef HWRENDER
		if (rendermode == render_opengl && !cv_glshearing.value)
			distxy = FixedMul(dist, FINECOSINE((focusaiming>>ANGLETOFINESHIFT) & FINEMASK));
		else
#endif
			distxy = dist;
		distz = -FixedMul(dist, FINESINE((focusaiming>>ANGLETOFINESHIFT) & FINEMASK)) + slopez;
	}
	else
	{
		distxy = dist;
		distz = slopez;
	}
>>>>>>> srb2/next

	axis = JoyAxis(AXISTURN, 1);

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
	forward = side = 0;

	// use two stage accelerative turning
	// on the keyboard and joystick
	if (turnleft || turnright)
		th += 1;
	else
		th = 0;

	if (th < SLOWTURNTICS)
		tspeed = 2; // slow turn
	else
		tspeed = speed;

	// let movement keys cancel each other out
	if (turnright && !(turnleft))
	{
		cmd->angleturn = (INT16)(cmd->angleturn - (angleturn[tspeed]));
		side += sidemove[1];
	}
	else if (turnleft && !(turnright))
	{
		cmd->angleturn = (INT16)(cmd->angleturn + (angleturn[tspeed]));
		side -= sidemove[1];
	}

	cmd->angleturn = (INT16)(cmd->angleturn - ((mousex*(encoremode ? -1 : 1)*8)));

	axis = JoyAxis(AXISMOVE, 1);
	if (InputDown(gc_accelerate, 1) || (usejoystick && axis > 0))
		cmd->buttons |= BT_ACCELERATE;
	axis = JoyAxis(AXISBRAKE, 1);
	if (InputDown(gc_brake, 1) || (usejoystick && axis > 0))
		cmd->buttons |= BT_BRAKE;
	axis = JoyAxis(AXISAIM, 1);
	if (InputDown(gc_aimforward, 1) || (usejoystick && axis < 0))
		forward += forwardmove[1];
	if (InputDown(gc_aimbackward, 1) || (usejoystick && axis > 0))
		forward -= forwardmove[1];

	// fire with any button/key
	axis = JoyAxis(AXISFIRE, 1);
	if (InputDown(gc_fire, 1) || (usejoystick && axis > 0))
		cmd->buttons |= BT_ATTACK;

	// spectator aiming shit, ahhhh...
	player_invert = invertmouse ? -1 : 1;
	screen_invert = 1;	// nope

	// mouse look stuff (mouse look is not the same as mouse aim)
	kbl = false;

	// looking up/down
	laim += (mlooky<<19)*player_invert*screen_invert;

	axis = JoyAxis(AXISLOOK, 1);

	// spring back if not using keyboard neither mouselookin'
	if (!kbl && !lookaxis && !mouseaiming)
		laim = 0;

	if (InputDown(gc_lookup, 1) || (axis < 0))
	{
		laim += KB_LOOKSPEED * screen_invert;
		kbl = true;
	}
	else if (InputDown(gc_lookdown, 1) || (axis > 0))
	{
		laim -= KB_LOOKSPEED * screen_invert;
		kbl = true;
	}

	if (InputDown(gc_centerview, 1)) // No need to put a spectator limit on this one though :V
		laim = 0;

	cmd->aiming = G_ClipAimingPitch(&laim);

	mousex = mousey = mlooky = 0;

	if (forward > MAXPLMOVE)
		forward = MAXPLMOVE;
	else if (forward < -MAXPLMOVE)
		forward = -MAXPLMOVE;

	if (side > MAXPLMOVE)
		side = MAXPLMOVE;
	else if (side < -MAXPLMOVE)
		side = -MAXPLMOVE;

	if (forward || side)
	{
		cmd->forwardmove = (SINT8)(cmd->forwardmove + forward);
		cmd->sidemove = (SINT8)(cmd->sidemove + side);
	}

	lang += (cmd->angleturn<<16);

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
	cam->angle = cmd->angleturn<<16;

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
	awayviewmobj_hack->flags2 |= MF2_DONTDRAW;

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
		z = player->mo->z + player->mo->height - (41*player->height/48) - 16*FRACUNIT;
	else
		z = player->mo->z + (41*player->height/48);

	// set bits for the camera
	thiscam->x = x;
	thiscam->y = y;
	thiscam->z = z;

	if (!(thiscam == &camera[0] && (cv_cam_still[0].value)
		&& !(thiscam == &camera[1] && cv_cam_still[1].value)
		&& !(thiscam == &camera[2] && cv_cam_still[2].value)
		&& !(thiscam == &camera[3] && cv_cam_still[3].value))
	{
		thiscam->angle = player->mo->angle;
		thiscam->aiming = 0;
	}
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

	static fixed_t camsideshift[2] = {0, 0};
	fixed_t shiftx = 0, shifty = 0;

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

	mo = player->mo;

#ifndef NOCLIPCAM
	cameranoclip = ((player->pflags & (PF_NOCLIP|PF_NIGHTSMODE))
		|| (mo->flags & (MF_NOCLIP|MF_NOCLIPHEIGHT)) // Noclipping player camera noclips too!!
		|| (leveltime < introtime)); // Kart intro cam
#endif

	if ((player->pflags & PF_TIMEOVER) && G_RaceGametype()) // 1 for momentum keep, 2 for turnaround
		timeover = (player->karthud[khud_timeovercam] > 2*TICRATE ? 2 : 1);
	else
		timeover = 0;

	if (!(player->playerstate == PST_DEAD || player->exiting))
	{
		if (player->spectator) // force cam off for spectators
			return true;

		if (!cv_chasecam.value && thiscam == &camera[0])
			return true;

		if (!cv_chasecam2.value && thiscam == &camera[1])
			return true;

		if (!cv_chasecam3.value && thiscam == &camera[2])
			return true;

<<<<<<< HEAD
		if (!cv_chasecam4.value && thiscam == &camera[3])
			return true;
=======
boolean P_SpectatorJoinGame(player_t *player)
{
	if (!G_CoopGametype() && !cv_allowteamchange.value)
	{
		if (P_IsLocalPlayer(player))
			CONS_Printf(M_GetText("Server does not allow team change.\n"));
		player->powers[pw_flashing] += 2*TICRATE; //to prevent message spam.
>>>>>>> srb2/next
	}

	if (!thiscam->chase && !resetcalled)
	{
		if (player == &players[consoleplayer])
			focusangle = localangle[0];
		else if (player == &players[displayplayers[1]])
			focusangle = localangle[1];
		else if (player == &players[displayplayers[2]])
			focusangle = localangle[2];
		else if (player == &players[displayplayers[3]])
			focusangle = localangle[3];
		else
			focusangle = mo->angle;

		if (thiscam == &camera[0])
			camrotate = cv_cam_rotate.value;
		else if (thiscam == &camera[1])
			camrotate = cv_cam2_rotate.value;
		else if (thiscam == &camera[2])
			camrotate = cv_cam3_rotate.value;
		else if (thiscam == &camera[3])
			camrotate = cv_cam4_rotate.value;
		else
			camrotate = 0;

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
	else if (sign)
	{
		focusangle = FixedAngle(sign->spawnpoint->angle << FRACBITS) + ANGLE_180;
		focusaiming = 0;
	}
	else if (player == &players[consoleplayer])
	{
		focusangle = localangle[0];
		focusaiming = localaiming[0];
	}
	else if (player == &players[g_localplayers[1]])
	{
		focusangle = localangle[1];
		focusaiming = localaiming[1];
	}
	else if (player == &players[g_localplayers[2]])
	{
		focusangle = localangle[2];
		focusaiming = localaiming[2];
	}
	else if (player == &players[g_localplayers[3]])
	{
		focusangle = localangle[3];
		focusaiming = localaiming[3];
	}
	else
	{
<<<<<<< HEAD
		focusangle = player->cmd.angleturn << 16;
		focusaiming = player->aiming;
=======
		// Exception for hide and seek. Don't join a game when you simply
		// respawn in place and sit there for the rest of the round.
		if (!((gametyperules & GTR_HIDEFROZEN) && leveltime > (hidetime * TICRATE)))
		{
			if (!LUAh_TeamSwitch(player, 3, true, false, false))
				return false;
			if (player->mo)
			{
				P_RemoveMobj(player->mo);
				player->mo = NULL;
			}
			player->spectator = player->outofcoop = false;
			player->playerstate = PST_REBORN;

			if ((gametyperules & (GTR_TAG|GTR_HIDEFROZEN)) == GTR_TAG)
			{
				//Make joining players "it" after hidetime.
				if (leveltime > (hidetime * TICRATE))
				{
					CONS_Printf(M_GetText("%s is now IT!\n"), player_names[player-players]); // Tell everyone who is it!
					player->pflags |= PF_TAGIT;
				}

				P_CheckSurvivors();
			}

			//Reset away view
			if (P_IsLocalPlayer(player) && displayplayer != consoleplayer)
			{
				// Call ViewpointSwitch hooks here.
				// The viewpoint was forcibly changed.
				LUAh_ViewpointSwitch(player, &players[consoleplayer], true);
				displayplayer = consoleplayer;
			}

			if (!G_CoopGametype())
				CONS_Printf(M_GetText("%s entered the game.\n"), player_names[player-players]);
			return true; // no more player->mo, cannot continue.
		}
		else
		{
			if (P_IsLocalPlayer(player))
				CONS_Printf(M_GetText("You must wait until next round to enter the game.\n"));
			player->powers[pw_flashing] += 2*TICRATE; //to prevent message spam.
		}
>>>>>>> srb2/next
	}

	if (P_CameraThinker(player, thiscam, resetcalled))
		return true;

	lookback = ( player->cmd.buttons & BT_LOOKBACK );

	if (thiscam == &camera[1]) // Camera 2
	{
		num = 1;
		camspeed = cv_cam2_speed.value;
		camstill = cv_cam2_still.value;
		camrotate = cv_cam2_rotate.value;
		camdist = FixedMul(cv_cam2_dist.value, mapobjectscale);
		camheight = FixedMul(cv_cam2_height.value, mapobjectscale);
	}
	else if (thiscam == &camera[2]) // Camera 3
	{
		num = 2;
		camspeed = cv_cam3_speed.value;
		camstill = cv_cam3_still.value;
		camrotate = cv_cam3_rotate.value;
		camdist = FixedMul(cv_cam3_dist.value, mapobjectscale);
		camheight = FixedMul(cv_cam3_height.value, mapobjectscale);
	}
	else if (thiscam == &camera[3]) // Camera 4
	{
		num = 3;
		camspeed = cv_cam4_speed.value;
		camstill = cv_cam4_still.value;
		camrotate = cv_cam4_rotate.value;
		camdist = FixedMul(cv_cam4_dist.value, mapobjectscale);
		camheight = FixedMul(cv_cam4_height.value, mapobjectscale);
	}
	else // Camera 1
	{
		num = 0;
		camspeed = cv_cam_speed.value;
		camstill = cv_cam_still.value;
		camorbit = cv_cam_orbit.value;
		camrotate = cv_cam_rotate.value;
		camdist = FixedMul(cv_cam_dist.value, mapobjectscale);
		camheight = FixedMul(cv_cam_height.value, mapobjectscale);
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
	else if (leveltime < starttime)
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

	if (!resetcalled && (leveltime > starttime && timeover != 2)
		&& ((thiscam == &camera[0] && t_cam_rotate[0] != -42)
		|| (thiscam == &camera[1] && t_cam_rotate[1] != -42)
		|| (thiscam == &camera[2] && t_cam_rotate[2] != -42)
		|| (thiscam == &camera[3] && t_cam_rotate[3] != -42)))
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

	if (rendermode == render_opengl
#ifdef GL_SHADERS/* just so we can't possibly forget about it */
			&& !cv_grshearing.value
#endif
	)
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

<<<<<<< HEAD
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
=======
		if (angdiff + minecart->angle != player->mo->angle && (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG))
		{
			angdiff = P_GetLocalAngle(player) - minecart->angle;
			if (angdiff < ANGLE_180 && angdiff > MINECARTCONEMAX)
				P_SetLocalAngle(player, minecart->angle + MINECARTCONEMAX);
			else if (angdiff > ANGLE_180 && angdiff < InvAngle(MINECARTCONEMAX))
				P_SetLocalAngle(player, minecart->angle - MINECARTCONEMAX);


>>>>>>> srb2/next
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
	else if (leveltime < starttime)
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
		angle = R_PointToAngle2(0, thiscam->z + thiscam->height, dist, mo->z + mo->height - P_GetPlayerHeight(player));
		if (thiscam->pitch < ANGLE_180 && thiscam->pitch > angle)
			angle += (thiscam->pitch - angle)/2;
	}
	else
	{
		angle = R_PointToAngle2(0, thiscam->z, dist, mo->z + P_GetPlayerHeight(player));
		if (thiscam->pitch >= ANGLE_180 && thiscam->pitch < angle)
			angle -= (angle - thiscam->pitch)/2;
	}

	if (player->playerstate != PST_DEAD && !((player->pflags & PF_NIGHTSMODE) && player->exiting))
		angle += (focusaiming < ANGLE_180 ? focusaiming/2 : InvAngle(InvAngle(focusaiming)/2)); // overcomplicated version of '((signed)focusaiming)/2;'

<<<<<<< HEAD
	if (twodlevel || (mo->flags2 & MF2_TWOD) || (!camstill && !timeover)) // Keep the view still...
	{
		G_ClipAimingPitch((INT32 *)&angle);
=======
			minecart->movefactor = 0;
			P_ResetScore(player);
			// Handle angle and position
			P_GetAxisPosition(minecart->x, minecart->y, axis, &newx, &newy, &targetangle, &grind);
			if (axis->type != MT_AXISTRANSFERLINE)
				P_SpawnSparks(minecart, grind);
			P_TryMove(minecart, newx, newy, true);

			// Set angle based on target
			prevangle = minecart->angle;
			angdiff = targetangle - minecart->angle;
			if (angdiff < ANGLE_90 + ANG2 || angdiff > ANGLE_270 - ANG2)
				minecart->angle = targetangle;
			else
				minecart->angle = targetangle + ANGLE_180;
			angdiff = (minecart->angle - prevangle);
			if (angdiff && (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG))  // maintain relative angle on turns
			{
				player->mo->angle += angdiff;
				P_SetPlayerAngle(player, (angle_t)(player->angleturn << 16) + angdiff);
			}
>>>>>>> srb2/next

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
		if (P_IsLocalPlayer(player) && g_localplayers[0] != consoleplayer)
			g_localplayers[0] = consoleplayer;

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
	if (netgame && player->health > 0)
		CON_LogMessage(va(M_GetText("%s ran out of time.\n"), player_names[player-players]));

	player->pflags |= PF_TIMEOVER;

	if (P_IsLocalPlayer(player) && !demo.playback)
		legitimateexit = true; // SRB2kart: losing a race is still seeing it through to the end :p

	if (player->mo)
	{
		S_StopSound(player->mo);
		P_DamageMobj(player->mo, NULL, NULL, 10000);
	}

	player->lives = 0;

	P_EndingMusic(player);

	if (!exitcountdown)
		exitcountdown = 5*TICRATE;
}

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

<<<<<<< HEAD
	mo->momx = FixedMul(c, speed);
	mo->momy = FixedMul(s, speed);
	mo->momz = FixedDiv(dh, 2*fixConst) + FixedDiv(dz, FixedDiv(dh, fixConst/2));
=======
	P_UnsetThingPosition(fume);
	fume->x = mo->x + P_ReturnThrustX(fume, angle, dist);
	fume->y = mo->y + P_ReturnThrustY(fume, angle, dist);
	fume->z = mo->z + ((mo->height - fume->height) >> 1);
	P_SetThingPosition(fume);

	// If dashmode is high enough, spawn a trail
	if (player->normalspeed >= skins[player->skin].normalspeed*2)
		P_SpawnGhostMobj(fume);
>>>>>>> srb2/next
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

		if (cv_seenames.value && cv_allowseenames.value &&
			!(G_TagGametype() && (player->pflags & PF_TAGIT)))
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

	if (player->flashcount)
		player->flashcount--;

	if (player->awayviewtics && player->awayviewtics != -1)
		player->awayviewtics--;

	cmd = &player->cmd;

	// SRB2kart
	// Save the dir the player is holding
	//  to allow items to be thrown forward or backward.
	if (cmd->buttons & BT_FORWARD)
		player->kartstuff[k_throwdir] = 1;
	else if (cmd->buttons & BT_BACKWARD)
		player->kartstuff[k_throwdir] = -1;
	else
		player->kartstuff[k_throwdir] = 0;

	// Add some extra randomization.
	if (cmd->forwardmove)
		P_RandomFixed();

#ifdef PARANOIA
	if (player->playerstate == PST_REBORN)
		I_Error("player %s is in PST_REBORN\n", sizeu1(playeri));
#endif

	if (!mapreset)
	{
		if (gametyperules & GTR_RACE)
		{
			INT32 i;

			// Check if all the players in the race have finished. If so, end the level.
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && !players[i].spectator)
				{
					if (!players[i].exiting && players[i].lives > 0)
						break;
				}
			}

			if (i == MAXPLAYERS && player->exiting == raceexittime+2) // finished
				player->exiting = raceexittime+1;

			// If 10 seconds are left on the timer,
			// begin the drown music for countdown!

			// SRB2Kart: despite how perfect this is, it's disabled FOR A REASON
#if 0
<<<<<<< HEAD
			if (racecountdown == 11*TICRATE - 1)
=======
	// Same check as below, just at 1 second before
	// so we can fade music
	if (!exitfadestarted &&
		player->exiting > 0 && player->exiting <= 1*TICRATE &&
		(!multiplayer || G_CoopGametype() ? !mapheaderinfo[gamemap-1]->musinterfadeout : true) &&
			// don't fade if we're fading during intermission. follows Y_StartIntermission intertype = int_coop
		((gametyperules & GTR_RACE) ? countdown2 == 0 : true) && // don't fade on timeout
		player->lives > 0 && // don't fade on game over (competition)
		P_IsLocalPlayer(player))
	{
		if (cv_playersforexit.value)
		{
			INT32 i;

			for (i = 0; i < MAXPLAYERS; i++)
>>>>>>> srb2/next
			{
				if (P_IsLocalPlayer(player))
					S_ChangeMusicInternal("drown", false);
			}
#endif

			// If you've hit the countdown and you haven't made
			//  it to the exit, you're a goner!
			else if (racecountdown == 1 && !player->exiting && !player->spectator && player->lives > 0)
			{
				P_DoTimeOver(player);

				if (player->playerstate == PST_DEAD)
					return;
			}
		}

		// If it is set, start subtracting
		// Don't allow it to go back to 0
		if (player->exiting > 1 && (player->exiting < raceexittime+2 || !G_RaceGametype())) // SRB2kart - "&& player->exiting > 1"
			player->exiting--;

		if (player->exiting && exitcountdown)
			player->exiting = 99; // SRB2kart
	}

	if (player->exiting == 2 || countdown2 == 2)
	{
		UINT8 numneeded = (G_IsSpecialStage(gamemap) ? 4 : cv_playersforexit.value);
		if (numneeded) // Count to be sure everyone's exited
		{
			INT32 i, total = 0, exiting = 0;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator || players[i].bot)
					continue;
				if (players[i].quittime > 30 * TICRATE)
					continue;
				if (players[i].lives <= 0)
					continue;

				total++;
				if (players[i].exiting && players[i].exiting < 4)
					exiting++;
			}

			if (!total || ((4*exiting)/total) >= numneeded)
			{
				if (server)
					SendNetXCmd(XD_EXITLEVEL, NULL, 0);
			}
		}
	}

	if (player->pflags & PF_FINISHED)
	{
		if (((gametyperules & GTR_FRIENDLY) && cv_exitmove.value) && !G_EnoughPlayersFinished())
			player->exiting = 0;
		else
			P_DoPlayerExit(player);
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
	G_GametypeUsesCoopStarposts() && (netgame || multiplayer) && cv_coopstarposts.value == 2)
		P_ConsiderAllGone();

	if (player->playerstate == PST_DEAD)
	{
		if (player->spectator)
			player->mo->flags2 |= MF2_SHADOW;
		else
			player->mo->flags2 &= ~MF2_SHADOW;
		P_DeathThink(player);
		LUAh_PlayerThink(player);
		return;
	}

	// Make sure spectators always have a score and ring count of 0.
	if (player->spectator)
	{
<<<<<<< HEAD
		//player->score = 0;
		player->mo->health = 1;
		player->health = 1;
	}

#if 0
	if ((netgame || multiplayer) && player->lives <= 0)
=======
		if (!(gametyperules & GTR_CAMPAIGN))
			player->score = 0;
	}
	else if ((netgame || multiplayer) && player->lives <= 0 && !G_CoopGametype())
>>>>>>> srb2/next
	{
		// Outside of Co-Op, replenish a user's lives if they are depleted.
		// of course, this is just a cheap hack, meh...
		player->lives = cv_startinglives.value;
	}
#else
	player->lives = 1; // SRB2Kart
#endif

	// SRB2kart 010217
	if (leveltime < starttime)
		player->powers[pw_nocontrol] = 2;

	// Synchronizes the "real" amount of time spent in the level.
	if (!player->exiting && !stoppedclock)
	{
		if (gametyperules & GTR_RACE)
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

	// Even if not NiGHTS, pull in nearby objects when walking around as John Q. Elliot.
	if (!objectplacing && !((netgame || multiplayer) && player->spectator)
	&& maptol & TOL_NIGHTS && (player->powers[pw_carry] != CR_NIGHTSMODE || player->powers[pw_nights_helper]))
	{
		thinker_t *th;
		mobj_t *mo2;
		fixed_t x = player->mo->x;
		fixed_t y = player->mo->y;
		fixed_t z = player->mo->z;

		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			mo2 = (mobj_t *)th;

			if (!(mo2->type == MT_RING || mo2->type == MT_COIN
				|| mo2->type == MT_BLUESPHERE || mo2->type == MT_BOMBSPHERE
				|| mo2->type == MT_NIGHTSCHIP || mo2->type == MT_NIGHTSSTAR))
				continue;

			if (mo2->flags2 & MF2_NIGHTSPULL)
				continue;

			if (P_AproxDistance(P_AproxDistance(mo2->x - x, mo2->y - y), mo2->z - z) > FixedMul(128*FRACUNIT, player->mo->scale))
				continue;

			// Yay! The thing's in reach! Pull it in!
			mo2->flags |= MF_NOCLIP|MF_NOCLIPHEIGHT;
			mo2->flags2 |= MF2_NIGHTSPULL;
			// New NiGHTS attract speed dummied out because the older behavior
			// is exploited as a mechanic. Uncomment to enable.
			mo2->movefactor = 0; // 40*FRACUNIT; // initialize the NightsItemChase timer
			P_SetTarget(&mo2->tracer, player->mo);
		}
	}

	if (player->linktimer && !player->powers[pw_nights_linkfreeze])
	{
		if (--player->linktimer <= 0) // Link timer
			player->linkcount = 0;
	}

	// Move around.
	// Reactiontime is used to prevent movement
	//  for a bit after a teleport.
	if (player->mo->reactiontime)
		player->mo->reactiontime--;
	else if (player->powers[pw_carry] == CR_MINECART)
	{
		P_DoZoomTube(player);
		player->rmomx = player->rmomy = 0; // no actual momentum from your controls
		P_ResetScore(player);
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

	//P_DoSuperStuff(player);
	//P_CheckSneakerAndLivesTimer(player);
	P_DoBubbleBreath(player); // Spawn Sonic's bubbles
	//P_CheckUnderwaterAndSpaceTimer(player); // Display the countdown drown numbers!
	P_CheckInvincibilityTimer(player); // Spawn Invincibility Sparkles
	P_DoPlayerHeadSigns(player); // Spawn Tag/CTF signs over player's head

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
				gmobj->flags2 |= MF2_DONTDRAW;
				break;
			}
		}
	}
#endif

	// check for use
	if (player->powers[pw_carry] != CR_NIGHTSMODE)
	{
		if (cmd->buttons & BT_BRAKE)
			player->pflags |= PF_USEDOWN;
		else
			player->pflags &= ~PF_USEDOWN;
	}

	// IF PLAYER NOT HERE THEN FLASH END IF
	if (player->quittime && player->powers[pw_flashing] < flashingtics - 1
	&& !(G_TagGametype() && !(player->pflags & PF_TAGIT)) && !player->gotflag)
		player->powers[pw_flashing] = flashingtics - 1;

	// Counters, time dependent power ups.
	// Time Bonus & Ring Bonus count settings

	if (player->ammoremovaltimer)
	{
		if (--player->ammoremovaltimer == 0)
			player->ammoremoval = 0;
	}

	// Strength counts up to diminish fade.
	if (player->powers[pw_flashing] && player->powers[pw_flashing] < UINT16_MAX &&
		(player->spectator || player->powers[pw_flashing] < K_GetKartFlashing(player)))
		player->powers[pw_flashing]--;

	if (player->powers[pw_nocontrol] & ((1<<15)-1) && player->powers[pw_nocontrol] < UINT16_MAX)
	{
		if (!(--player->powers[pw_nocontrol]))
			player->pflags &= ~PF_SKIDDOWN;
	}
	else
		player->powers[pw_nocontrol] = 0;

	if (player->powers[pw_ignorelatch] & ((1<<15)-1) && player->powers[pw_ignorelatch] < UINT16_MAX)
		player->powers[pw_ignorelatch]--;
	else
		player->powers[pw_ignorelatch] = 0;

	//pw_super acts as a timer now
	if (player->powers[pw_super]
	&& (player->mo->state < &states[S_PLAY_SUPER_TRANS1]
	|| player->mo->state > &states[S_PLAY_SUPER_TRANS6]))
		player->powers[pw_super]++;

	// Flash player after being hit.
	if (!(//player->pflags & PF_NIGHTSMODE ||
		player->kartstuff[k_hyudorotimer] // SRB2kart - fixes Hyudoro not flashing when it should.
		|| player->kartstuff[k_growshrinktimer] > 0 // Grow doesn't flash either.
		|| player->kartstuff[k_respawn] // Respawn timer (for drop dash effect)
		|| (player->pflags & PF_TIMEOVER) // NO CONTEST explosion
		|| (G_BattleGametype() && player->kartstuff[k_bumper] <= 0 && player->kartstuff[k_comebacktimer])
		|| leveltime < starttime)) // Level intro
	{
		if (player->powers[pw_flashing] > 0 && player->powers[pw_flashing] < K_GetKartFlashing(player)
			&& (leveltime & 1))
			player->mo->flags2 |= MF2_DONTDRAW;
		else
			player->mo->flags2 &= ~MF2_DONTDRAW;
	}

	player->pflags &= ~PF_SLIDING;

<<<<<<< HEAD
	K_KartPlayerThink(player, cmd); // SRB2kart
=======
#define dashmode player->dashmode
	// Dash mode - thanks be to VelocitOni
	if ((player->charflags & SF_DASHMODE) && !player->gotflag && !player->powers[pw_carry] && !player->exiting && !(maptol & TOL_NIGHTS) && !metalrecording) // woo, dashmode! no nights tho.
	{
		tic_t prevdashmode = dashmode;
		boolean totallyradical = player->speed >= FixedMul(player->runspeed, player->mo->scale);
		boolean floating = (player->secondjump == 1);

		if ((totallyradical && !floating) || (player->pflags & PF_STARTDASH))
		{
			if (dashmode < DASHMODE_MAX)
				dashmode++; // Counter. Adds 1 to dash mode per tic in top speed.
			if (dashmode == DASHMODE_THRESHOLD) // This isn't in the ">=" equation because it'd cause the sound to play infinitely.
				S_StartSound(player->mo, (player->charflags & SF_MACHINE) ? sfx_kc4d : sfx_cdfm40); // If the player enters dashmode, play this sound on the the tic it starts.
		}
		else if ((!totallyradical || !floating) && !(player->pflags & PF_SPINNING))
		{
			if (dashmode > 3)
			{
				dashmode -= 3; // Rather than lose it all, it gently counts back down!
				if ((dashmode+3) >= DASHMODE_THRESHOLD && dashmode < DASHMODE_THRESHOLD)
					S_StartSound(player->mo, sfx_kc65);
			}
			else
				dashmode = 0;
		}

		if (dashmode < DASHMODE_THRESHOLD) // Exits Dash Mode if you drop below speed/dash counter tics. Not in the above block so it doesn't keep disabling in midair.
		{
			if (prevdashmode >= DASHMODE_THRESHOLD)
			{
				player->normalspeed = skins[player->skin].normalspeed; // Reset to default if not capable of entering dash mode.
				player->jumpfactor = skins[player->skin].jumpfactor;
			}
		}
		else if (P_IsObjectOnGround(player->mo)) // Activate dash mode if we're on the ground.
		{
			if (player->normalspeed < skins[player->skin].normalspeed*2) // If the player normalspeed is not currently at normalspeed*2 in dash mode, add speed each tic
				player->normalspeed += FRACUNIT/5; // Enter Dash Mode smoothly.

			if (player->jumpfactor < FixedMul(skins[player->skin].jumpfactor, 5*FRACUNIT/4)) // Boost jump height.
				player->jumpfactor += FRACUNIT/300;
		}

		if (player->normalspeed >= skins[player->skin].normalspeed*2)
		{
			mobj_t *ghost = P_SpawnGhostMobj(player->mo); // Spawns afterimages
			ghost->fuse = 2; // Makes the images fade quickly
			if (ghost->tracer && !P_MobjWasRemoved(ghost->tracer))
				ghost->tracer->fuse = ghost->fuse;
		}
	}
	else if (dashmode)
	{
		if (dashmode >= DASHMODE_THRESHOLD) // catch getting the flag!
		{
			player->normalspeed = skins[player->skin].normalspeed;
			player->jumpfactor = skins[player->skin].jumpfactor;
			S_StartSound(player->mo, sfx_kc65);
		}
		dashmode = 0;
	}
#undef dashmode
>>>>>>> srb2/next

	LUAh_PlayerThink(player);
}

// Checks if the mobj is above lava. Used by Pterabyte.
static boolean P_MobjAboveLava(mobj_t *mobj)
{
	sector_t *sector = mobj->subsector->sector;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE) || GETSECSPECIAL(rover->master->frontsector->special, 1) != 3)
				continue;

			if (mobj->eflags & MFE_VERTICALFLIP)
			{
				if (*rover->bottomheight <= mobj->ceilingz && *rover->bottomheight >= mobj->z)
					return true;
			}
			else
			{
				if (*rover->topheight >= mobj->floorz && *rover->topheight <= mobj->z)
					return true;
			}
		}
	}

	return false;
}

//
// P_PlayerAfterThink
//
// Thinker for player after all other thinkers have run
//
void P_PlayerAfterThink(player_t *player)
{
	ticcmd_t *cmd;
	//INT32 oldweapon = player->currentweapon; // SRB2kart - unused
	camera_t *thiscam = NULL; // if not one of the displayed players, just don't bother
	UINT8 i;

#ifdef PARANOIA
	if (!player->mo)
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("P_PlayerAfterThink: players[%s].mo == NULL", sizeu1(playeri));
	}
#endif

	cmd = &player->cmd;

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

	if (player->powers[pw_carry] == CR_NIGHTSMODE)
	{
		player->powers[pw_gravityboots] = 0;
		//player->mo->eflags &= ~MFE_VERTICALFLIP;
	}

	if (player->pflags & PF_SLIDING)
		P_SetPlayerMobjState(player->mo, player->mo->info->painstate);

<<<<<<< HEAD
=======
	/* if (player->powers[pw_carry] == CR_NONE && player->mo->tracer && !player->homing)
		P_SetTarget(&player->mo->tracer, NULL);
	else */
	if (player->mo->tracer)
	{
		switch (player->powers[pw_carry])
		{
			case CR_PLAYER: // being carried by a flying character (such as Tails)
			{
				mobj_t *tails = player->mo->tracer;
				player->mo->height = FixedDiv(P_GetPlayerHeight(player), FixedDiv(14*FRACUNIT,10*FRACUNIT));

				if (tails->player && !(tails->player->pflags & PF_CANCARRY))
					player->powers[pw_carry] = CR_NONE;

				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					if ((tails->z + tails->height + player->mo->height + FixedMul(FRACUNIT, player->mo->scale)) <= tails->ceilingz
						&& (tails->eflags & MFE_VERTICALFLIP)) // Reverse gravity check for the carrier - Flame
						player->mo->z = tails->z + tails->height + 12*player->mo->scale;
					else
						player->powers[pw_carry] = CR_NONE;
				}
				else
				{
					if ((tails->z - player->mo->height - FixedMul(FRACUNIT, player->mo->scale)) >= tails->floorz
						&& !(tails->eflags & MFE_VERTICALFLIP)) // Correct gravity check for the carrier - Flame
						player->mo->z = tails->z - player->mo->height - 12*player->mo->scale;
					else
						player->powers[pw_carry] = CR_NONE;
				}

				if (tails->health <= 0)
					player->powers[pw_carry] = CR_NONE;
				else
				{
					P_TryMove(player->mo, tails->x + P_ReturnThrustX(tails, tails->player->drawangle, 4*FRACUNIT), tails->y + P_ReturnThrustY(tails, tails->player->drawangle, 4*FRACUNIT), true);
					player->mo->momx = tails->momx;
					player->mo->momy = tails->momy;
					player->mo->momz = tails->momz;
				}

				if (G_CoopGametype() && (!tails->player || tails->player->bot != 1))
				{
					player->mo->angle = tails->angle;

					if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
						P_SetPlayerAngle(player, player->mo->angle);
				}

				if (P_AproxDistance(player->mo->x - tails->x, player->mo->y - tails->y) > player->mo->radius)
					player->powers[pw_carry] = CR_NONE;

				if (player->powers[pw_carry] != CR_NONE)
				{
					if (player->mo->state-states != S_PLAY_RIDE)
						P_SetPlayerMobjState(player->mo, S_PLAY_RIDE);
					if ((tails->skin && ((skin_t *)(tails->skin))->sprites[SPR2_SWIM].numframes) && (tails->eflags & MFE_UNDERWATER))
						tails->player->powers[pw_tailsfly] = 0;
				}
				else
					P_SetTarget(&player->mo->tracer, NULL);

				if (player-players == consoleplayer && botingame)
					CV_SetValue(&cv_analog[1], (player->powers[pw_carry] != CR_PLAYER));
				break;
			}
			case CR_GENERIC: // being carried by some generic item
			{
				mobj_t *item = player->mo->tracer;
				// tracer is what you're hanging onto
				P_UnsetThingPosition(player->mo);
				player->mo->x = item->x;
				player->mo->y = item->y;
				if (player->mo->eflags & MFE_VERTICALFLIP)
					player->mo->z = item->z + item->height - FixedDiv(player->mo->height, 3*FRACUNIT);
				else
					player->mo->z = item->z - FixedDiv(player->mo->height, 3*FRACUNIT/2);
				player->mo->momx = player->mo->momy = player->mo->momz = 0;
				P_SetThingPosition(player->mo);
				if (player->mo->state-states != S_PLAY_RIDE)
					P_SetPlayerMobjState(player->mo, S_PLAY_RIDE);

				// Controllable missile
				if (item->type == MT_BLACKEGGMAN_MISSILE)
				{
					if (cmd->forwardmove > 0)
						item->momz += FixedMul(FRACUNIT/4, item->scale);
					else if (cmd->forwardmove < 0)
						item->momz -= FixedMul(FRACUNIT/4, item->scale);

					item->angle = player->mo->angle;
					P_InstaThrust(item, item->angle, FixedMul(item->info->speed, item->scale));

					if (player->mo->z <= player->mo->floorz || item->health <= 0)
					{
						player->powers[pw_carry] = CR_NONE;
						P_SetTarget(&player->mo->tracer, NULL);
					}
				}
				break;
			}
			case CR_MACESPIN: // being carried by a spinning chain
			{
				mobj_t *chain;
				mobj_t *macecenter;
				if (!player->mo->tracer->tracer) // can't spin around a point if... there is no point in doing so
					break;
				chain = player->mo->tracer;
				macecenter = player->mo->tracer->tracer;

				player->mo->height = P_GetPlayerSpinHeight(player);
				// tracer is what you're hanging onto....
				player->mo->momx = (chain->x - player->mo->x)*2;
				player->mo->momy = (chain->y - player->mo->y)*2;
				player->mo->momz = (chain->z - (player->mo->height-chain->height/2) - player->mo->z)*2;
				P_TeleportMove(player->mo, chain->x, chain->y, chain->z - (player->mo->height-chain->height/2));
				if (!player->powers[pw_flashing]) // handle getting hurt
				{
					player->pflags |= PF_JUMPED;
					player->pflags &= ~PF_NOJUMPDAMAGE;
					player->secondjump = 0;
					player->pflags &= ~PF_THOKKED;

					if ((macecenter->flags & MF_SLIDEME) // Noclimb on chain parameters gives this
					&& !(twodlevel || player->mo->flags2 & MF2_TWOD)) // why on earth would you want to turn them in 2D mode?
					{
						macecenter->angle += cmd->sidemove<<ANGLETOFINESHIFT;
						player->mo->angle += cmd->sidemove<<ANGLETOFINESHIFT; // 2048 --> ANGLE_MAX

						if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
							P_SetPlayerAngle(player, player->mo->angle);
					}
				}
				break;
			}
			case CR_DUSTDEVIL:
			{
				mobj_t *mo = player->mo, *dustdevil = player->mo->tracer;

				if (abs(mo->x - dustdevil->x) > dustdevil->radius || abs(mo->y - dustdevil->y) > dustdevil->radius)
				{
					P_SetTarget(&player->mo->tracer, NULL);
					player->powers[pw_carry] = CR_NONE;
					break;
				}

				break;
			}
			case CR_ROLLOUT:
			{
				mobj_t *mo = player->mo, *rock = player->mo->tracer;
				UINT8 walktics = mo->state->tics - P_GetPlayerControlDirection(player);

				if (!rock || P_MobjWasRemoved(rock))
				{
					P_SetTarget(&player->mo->tracer, NULL);
					player->powers[pw_carry] = CR_NONE;
					break;
				}

				if (player->cmd.forwardmove || player->cmd.sidemove)
				{
					rock->movedir = (player->cmd.angleturn << FRACBITS) + R_PointToAngle2(0, 0, player->cmd.forwardmove << FRACBITS, -player->cmd.sidemove << FRACBITS);
					P_Thrust(rock, rock->movedir, rock->scale >> 1);
				}

				mo->momx = rock->momx;
				mo->momy = rock->momy;
				mo->momz = 0;

				if (player->panim == PA_IDLE && (mo->momx || mo->momy))
				{
					P_SetPlayerMobjState(player->mo, S_PLAY_WALK);
				}

				if (player->panim == PA_WALK && mo->tics > walktics)
				{
					mo->tics = walktics;
				}

				P_TeleportMove(player->mo, rock->x, rock->y, rock->z + rock->height);
				break;
			}
			case CR_PTERABYTE: // being carried by a Pterabyte
			{
				mobj_t *ptera = player->mo->tracer;
				mobj_t *spawnpoint = ptera->tracer->tracer;
				player->mo->height = FixedDiv(P_GetPlayerHeight(player), FixedDiv(14 * FRACUNIT, 10 * FRACUNIT));

				if (ptera->health <= 0)
					goto dropoff;

				if (P_MobjAboveLava(ptera) && ptera->movefactor <= 3*TICRATE - 10)
					goto dropoff;

				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					if ((ptera->z + ptera->height + player->mo->height + FixedMul(FRACUNIT, player->mo->scale)) <= ptera->ceilingz
						&& (ptera->eflags & MFE_VERTICALFLIP)) // Reverse gravity check for the carrier - Flame
						player->mo->z = ptera->z + ptera->height + FixedMul(FRACUNIT, player->mo->scale);

					if (ptera->ceilingz - ptera->z > spawnpoint->ceilingz - spawnpoint->z + 512*FRACUNIT && ptera->movefactor <= 3 * TICRATE - 10)
						goto dropoff;
				}
				else
				{
					if ((ptera->z - player->mo->height - FixedMul(FRACUNIT, player->mo->scale)) >= ptera->floorz
						&& !(ptera->eflags & MFE_VERTICALFLIP)) // Correct gravity check for the carrier - Flame
						player->mo->z = ptera->z - player->mo->height - FixedMul(FRACUNIT, player->mo->scale);

					if (ptera->z - ptera->floorz > spawnpoint->z - spawnpoint->floorz + 512 * FRACUNIT && ptera->movefactor <= 3 * TICRATE - 10)
						goto dropoff;
				}

				ptera->movefactor--;
				if (!ptera->movefactor)
					goto dropoff;

				if (ptera->cusval >= 50)
				{
					player->powers[pw_carry] = CR_NONE;
					P_SetTarget(&player->mo->tracer, NULL);
					P_KillMobj(ptera, player->mo, player->mo, 0);
					player->mo->momz = 9*FRACUNIT;
					player->pflags |= PF_APPLYAUTOBRAKE|PF_JUMPED|PF_THOKKED;
					P_SetMobjState(player->mo, S_PLAY_ROLL);
					break;
				}

				if (ptera->cusval)
					ptera->cusval--;

				P_TryMove(player->mo, ptera->x + ptera->watertop, ptera->y + ptera->waterbottom, true);
				player->mo->z += ptera->cvmem;
				player->mo->momx = ptera->momx;
				player->mo->momy = ptera->momy;
				player->mo->momz = ptera->momz;

				if (P_AproxDistance(player->mo->x - ptera->x - ptera->watertop, player->mo->y - ptera->y - ptera->waterbottom) > player->mo->radius)
					goto dropoff;

				ptera->watertop >>= 1;
				ptera->waterbottom >>= 1;
				ptera->cvmem >>= 1;

				if (player->mo->state-states != S_PLAY_FALL)
					P_SetPlayerMobjState(player->mo, S_PLAY_FALL);
				break;

			dropoff:
				player->powers[pw_carry] = CR_NONE;
				P_SetTarget(&player->mo->tracer, NULL);
				ptera->movefactor = TICRATE;
				ptera->extravalue1 |= 4;
				break;
			}
			default:
				break;
		}
	}

>>>>>>> srb2/next
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
			player->viewheight = FixedMul(41*player->height/48, player->mo->scale);

			if (player->mo->eflags & MFE_VERTICALFLIP)
				player->viewz = player->mo->z + player->mo->height - player->viewheight;
			else
				player->viewz = player->mo->z + player->viewheight;
		}
	}

	// spectator invisibility and nogravity.
	if ((netgame || multiplayer) && player->spectator)
	{
		player->mo->flags2 |= MF2_DONTDRAW;
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
	INT16 delta = (INT16)(angle >> 16) - player->angleturn;

	P_ForceLocalAngle(player, P_GetLocalAngle(player) + (delta << 16));
	player->angleturn += delta;
}

void P_SetLocalAngle(player_t *player, angle_t angle)
{
	INT16 delta = (INT16)((angle - P_GetLocalAngle(player)) >> 16);

	P_ForceLocalAngle(player, P_GetLocalAngle(player) + (angle_t)(delta << 16));

	if (player == &players[consoleplayer])
		ticcmd_oldangleturn[0] += delta;
	else if (player == &players[secondarydisplayplayer])
		ticcmd_oldangleturn[1] += delta;
}

angle_t P_GetLocalAngle(player_t *player)
{
	if (player == &players[consoleplayer])
		return localangle;
	else if (player == &players[secondarydisplayplayer])
		return localangle2;
	else
		return 0;
}

void P_ForceLocalAngle(player_t *player, angle_t angle)
{
	angle = angle & ~UINT16_MAX;

	if (player == &players[consoleplayer])
		localangle = angle;
	else if (player == &players[secondarydisplayplayer])
		localangle2 = angle;
}

boolean P_PlayerFullbright(player_t *player)
{
	return (player->powers[pw_super]
		|| ((player->powers[pw_carry] == CR_NIGHTSMODE && (((skin_t *)player->mo->skin)->flags & (SF_SUPER|SF_NONIGHTSSUPER)) == SF_SUPER) // Super colours? Super bright!
		&& (player->exiting
			|| !(player->mo->state >= &states[S_PLAY_NIGHTS_TRANS1]
			&& player->mo->state < &states[S_PLAY_NIGHTS_TRANS6])))); // Note the < instead of <=
}
