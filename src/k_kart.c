// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2018 by ZarroTsu.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_kart.c
/// \brief SRB2kart general.
///        All of the SRB2kart-unique stuff.

// TODO: Break this up into more files.
// Files dedicated only for "general miscellanea"
// are straight-up bad coding practice.
// It's better to have niche files that are
// too short than one file that's too massive.

#include "k_kart.h"
#include "k_battle.h"
#include "k_pwrlv.h"
#include "k_color.h"
#include "k_respawn.h"
#include "doomdef.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "g_input.h"    // for device rumble
#include "m_random.h"
#include "p_local.h"
#include "p_slopes.h"
#include "p_setup.h"
#include "r_draw.h"
#include "r_local.h"
#include "r_things.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "z_zone.h"
#include "m_misc.h"
#include "m_cond.h"
#include "f_finale.h"
#include "lua_hud.h"	// For Lua hud checks
#include "lua_hook.h"	// For MobjDamage and ShouldDamage
#include "m_cheat.h"	// objectplacing
#include "p_spec.h"

#include "k_waypoint.h"
#include "k_bot.h"
#include "k_hud.h"
#include "k_terrain.h"
#include "k_director.h"
#include "k_collide.h"
#include "k_follower.h"
#include "k_objects.h"
#include "k_grandprix.h"
#include "k_boss.h"
#include "k_specialstage.h"
#include "k_roulette.h"
#include "k_podium.h"
#include "k_powerup.h"
#include "k_hitlag.h"
#include "k_tally.h"
#include "music.h"
#include "m_easing.h"
#include "k_endcam.h"

// SOME IMPORTANT VARIABLES DEFINED IN DOOMDEF.H:
// gamespeed is cc (0 for easy, 1 for normal, 2 for hard)
// franticitems is Frantic Mode items, bool
// encoremode is Encore Mode (duh), bool
// comeback is Battle Mode's karma comeback, also bool
// mapreset is set when enough players fill an empty server

boolean K_ThunderDome(void)
{
	if (K_CanChangeRules(true))
	{
		return (boolean)cv_thunderdome.value;
	}

	return false;
}

// lat: used for when the player is in some weird state where it wouldn't be wise for it to be overwritten by another object that does similarly wacky shit.
boolean K_isPlayerInSpecialState(player_t *p)
{
	return (
		p->rideroid
		|| p->rdnodepull
		|| p->bungee
		|| p->dlzrocket
		|| p->seasaw
		|| p->turbine
	);
}

boolean K_IsDuelItem(mobjtype_t type)
{
	switch (type)
	{
		case MT_DUELBOMB:
		case MT_BANANA:
		case MT_EGGMANITEM:
		case MT_SSMINE:
		case MT_LANDMINE:
		case MT_HYUDORO_CENTER:
		case MT_DROPTARGET:
		case MT_POGOSPRING:
			return true;

		default:
			return false;
	}
}

boolean K_DuelItemAlwaysSpawns(mapthing_t *mt)
{
	return !!(mt->thing_args[0]);
}

static void K_SpawnDuelOnlyItems(void)
{
	mapthing_t *mt = NULL;
	size_t i;

	mt = mapthings;
	for (i = 0; i < nummapthings; i++, mt++)
	{
		mobjtype_t type = P_GetMobjtype(mt->type);

		if (K_IsDuelItem(type) == true
			&& K_DuelItemAlwaysSpawns(mt) == false)
		{
			P_SpawnMapThing(mt);
		}
	}
}

void K_TimerReset(void)
{
	starttime = introtime = 0;
	memset(&g_darkness, 0, sizeof g_darkness);
	memset(&g_musicfade, 0, sizeof g_musicfade);
	numbulbs = 1;
	inDuel = rainbowstartavailable = false;
	linecrossed = 0;
	timelimitintics = extratimeintics = secretextratime = 0;
	g_pointlimit = 0;
}

static void K_SpawnItemCapsules(void)
{
	mapthing_t *mt = mapthings;
	size_t i = SIZE_MAX;

	for (i = 0; i < nummapthings; i++, mt++)
	{
		boolean isRingCapsule = false;
		INT32 modeFlags = 0;

		if (mt->type != mobjinfo[MT_ITEMCAPSULE].doomednum)
		{
			continue;
		}

		isRingCapsule = (mt->thing_args[0] < 1 || mt->thing_args[0] == KITEM_SUPERRING || mt->thing_args[0] >= NUMKARTITEMS);
		if (isRingCapsule == true && ((gametyperules & GTR_SPHERES) || (modeattacking & ATTACKING_SPB)))
		{
			// don't spawn ring capsules in ringless gametypes
			continue;
		}

		if (gametype != GT_TUTORIAL) // Don't prevent capsule spawn via modeflags in Tutorial
		{
			modeFlags = mt->thing_args[3];
			if (modeFlags == TMICM_DEFAULT)
			{
				if (isRingCapsule == true)
				{
					modeFlags = TMICM_MULTIPLAYER|TMICM_TIMEATTACK;
				}
				else
				{
					modeFlags = TMICM_MULTIPLAYER;
				}
			}

			if (K_CapsuleTimeAttackRules() == true)
			{
				if ((modeFlags & TMICM_TIMEATTACK) == 0)
				{
					continue;
				}
			}
			else
			{
				if ((modeFlags & TMICM_MULTIPLAYER) == 0)
				{
					continue;
				}
			}
		}


		P_SpawnMapThing(mt);
	}
}

void K_TimerInit(void)
{
	UINT8 i;
	UINT8 numPlayers = 0;
	boolean domodeattack = ((modeattacking != ATTACKING_NONE)
		|| (grandprixinfo.gp == true && grandprixinfo.eventmode != GPEVENT_NONE));

	if (K_PodiumSequence() == true)
	{
		// Leave it alone for podium
		return;
	}

	const boolean bossintro = K_CheckBossIntro();

	// Rooooooolllling staaaaaaart
	if ((gametyperules & (GTR_ROLLINGSTART|GTR_CIRCUIT)) == (GTR_ROLLINGSTART|GTR_CIRCUIT))
	{
		S_StartSound(NULL, sfx_s25f);
		// The actual push occours in P_InitPlayers
	}
	else if (skipstats != 0 && bossintro == false)
	{
		S_StartSound(NULL, sfx_s26c);
	}

	if ((gametyperules & (GTR_CATCHER|GTR_CIRCUIT)) == (GTR_CATCHER|GTR_CIRCUIT))
	{
		K_InitSpecialStage();
	}
	else if (bossintro == true)
		;
	else
	{
		if (!domodeattack)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator)
				{
					continue;
				}

				numPlayers++;
			}

			if (cv_kartdebugstart.value > 0)
				numPlayers = cv_kartdebugstart.value;

			if (numPlayers < 2)
			{
				domodeattack = true;
			}
			else
			{
				numbulbs = 5;
				rainbowstartavailable = true;

				// 1v1 activates DUEL rules!
				inDuel = (numPlayers == 2);

				if (!inDuel)
				{
					introtime = (108) + 5; // 108 for rotation, + 5 for white fade
					numbulbs += (numPlayers-2); // Extra POSITION!! time
				}
			}
		}

		starttime = introtime;
		if (!(gametyperules & GTR_NOPOSITION))
		{
			// Start countdown time + buffer time
			starttime += ((3*TICRATE) + ((2*TICRATE) + (numbulbs * bulbtime)));
		}
	}

	if (cv_kartdebugstart.value == -1 ? M_NotFreePlay() == false : cv_kartdebugstart.value == 0)
	{
		starttime = 0;
		introtime = 0;
	}

	if (G_TimeAttackStart())
	{
		starttime = 15*TICRATE; // Longest permitted start. No half-laps in reverse.
		// (Changed on finish line cross later, don't worry.)
	}

	K_SpawnItemCapsules();
	K_BattleInit(domodeattack);

	timelimitintics = K_TimeLimitForGametype();
	g_pointlimit = K_PointLimitForGametype();

	// K_TimerInit is called after all mapthings are spawned,
	// so they didn't know if it's supposed to be a duel
	// (inDuel is always false before K_TimerInit is called).
	if (inDuel)
	{
		K_SpawnDuelOnlyItems();
	}

	if (
		battleprisons == true
		&& grandprixinfo.gp == true
		&& netgame == false
		&& gamedata->thisprisoneggpickup_cached != NULL
		&& gamedata->prisoneggstothispickup == 0
		&& maptargets > 1
	)
	{
		// This calculation is like this so...
		// - You can't get a Prison Egg Drop on the last broken target
		// - If it were 0 at minimum there'd be a slight bias towards the start of the round
		//    - This is bad because it benefits CD farming like in Brawl :D
		gamedata->prisoneggstothispickup = 1 + M_RandomKey(maptargets - 1);
	}
}

UINT32 K_GetPlayerDontDrawFlag(player_t *player)
{
	UINT32 flag = 0;

	if (player == NULL)
		return flag;

	if (player == &players[displayplayers[0]])
		flag |= RF_DONTDRAWP1;
	if (r_splitscreen >= 1 && player == &players[displayplayers[1]])
		flag |= RF_DONTDRAWP2;
	if (r_splitscreen >= 2 && player == &players[displayplayers[2]])
		flag |= RF_DONTDRAWP3;
	if (r_splitscreen >= 3 && player == &players[displayplayers[3]])
		flag |= RF_DONTDRAWP4;

	return flag;
}

void K_ReduceVFXForEveryone(mobj_t *mo)
{
	if (cv_reducevfx.value == 0)
	{
		// Leave the visuals alone.
		return;
	}

	mo->renderflags |= RF_DONTDRAW;
}

// Angle reflection used by springs & speed pads
angle_t K_ReflectAngle(angle_t yourangle, angle_t theirangle, fixed_t yourspeed, fixed_t theirspeed)
{
	INT32 angoffset;
	boolean subtract = false;

	angoffset = yourangle - theirangle;

	if ((angle_t)angoffset > ANGLE_180)
	{
		// Flip on wrong side
		angoffset = InvAngle((angle_t)angoffset);
		subtract = !subtract;
	}

	// Fix going directly against the spring's angle sending you the wrong way
	if ((angle_t)angoffset > ANGLE_90)
	{
		angoffset = ANGLE_180 - angoffset;
	}

	// Offset is reduced to cap it (90 / 2 = max of 45 degrees)
	angoffset /= 2;

	// Reduce further based on how slow your speed is compared to the spring's speed
	// (set both to 0 to ignore this)
	if (theirspeed != 0 && yourspeed != 0)
	{
		if (theirspeed > yourspeed)
		{
			angoffset = FixedDiv(angoffset, FixedDiv(theirspeed, yourspeed));
		}
	}

	if (subtract)
		angoffset = (signed)(theirangle) - angoffset;
	else
		angoffset = (signed)(theirangle) + angoffset;

	return (angle_t)angoffset;
}

boolean K_IsPlayerLosing(player_t *player)
{
	INT32 winningpos = 1;
	UINT8 i, pcount = 0;

	if (K_PodiumSequence() == true)
	{
		// Need to be in top 3 to win.
		return (player->position > 3);
	}

	if (player->pflags & PF_NOCONTEST)
		return true;

	if (battleprisons && numtargets == 0)
		return true; // Didn't even TRY?

	if (player->position == 1)
		return false;

	if (specialstageinfo.valid == true)
		return false; // anything short of DNF is COOL

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;
		if (players[i].position > pcount)
			pcount = players[i].position;
	}

	if (pcount <= 1)
		return false;

	winningpos = pcount/2;
	if (pcount % 2) // any remainder?
		winningpos++;

	return (player->position > winningpos);
}

fixed_t K_GetKartGameSpeedScalar(SINT8 value)
{
	// Easy = 81.25%
	// Normal = 100%
	// Hard = 118.75%
	// Nightmare = 137.5% ?!?!

	// WARNING: This value is used instead of directly checking game speed in some
	// cases, where hard difficulty breakpoints are needed, but compatibility with
	// the "4th Gear" cheat seemed relevant. Sorry about the weird indirection!
	// At the time of writing:
	// K_UpdateOffroad (G3+ double offroad penalty speed)
	// P_ButteredSlope (G1- Slope Assist)

	if (cv_4thgear.value && !netgame && (!demo.playback || !demo.netgame) && !modeattacking)
		value = 3;

	return ((13 + (3*value)) << FRACBITS) / 16;
}

// Array of states to pick the starting point of the animation, based on the actual time left for invincibility.
static INT32 K_SparkleTrailStartStates[KART_NUMINVSPARKLESANIM][2] = {
	{S_KARTINVULN12, S_KARTINVULNB12},
	{S_KARTINVULN11, S_KARTINVULNB11},
	{S_KARTINVULN10, S_KARTINVULNB10},
	{S_KARTINVULN9, S_KARTINVULNB9},
	{S_KARTINVULN8, S_KARTINVULNB8},
	{S_KARTINVULN7, S_KARTINVULNB7},
	{S_KARTINVULN6, S_KARTINVULNB6},
	{S_KARTINVULN5, S_KARTINVULNB5},
	{S_KARTINVULN4, S_KARTINVULNB4},
	{S_KARTINVULN3, S_KARTINVULNB3},
	{S_KARTINVULN2, S_KARTINVULNB2},
	{S_KARTINVULN1, S_KARTINVULNB1}
};

INT32 K_GetShieldFromItem(INT32 item)
{
	switch (item)
	{
		case KITEM_LIGHTNINGSHIELD: return KSHIELD_LIGHTNING;
		case KITEM_BUBBLESHIELD: return KSHIELD_BUBBLE;
		case KITEM_FLAMESHIELD: return KSHIELD_FLAME;
		case KITEM_GARDENTOP: return KSHIELD_TOP;
		default: return KSHIELD_NONE;
	}
}

SINT8 K_ItemResultToType(SINT8 getitem)
{
	if (getitem <= 0 || getitem >= NUMKARTRESULTS) // Sad (Fallback)
	{
		if (getitem != 0)
		{
			CONS_Printf("ERROR: K_GetItemResultToItemType - Item roulette gave bad item (%d) :(\n", getitem);
		}

		return KITEM_SAD;
	}

	if (getitem >= NUMKARTITEMS)
	{
		switch (getitem)
		{
			case KRITEM_DUALSNEAKER:
			case KRITEM_TRIPLESNEAKER:
				return KITEM_SNEAKER;

			case KRITEM_TRIPLEBANANA:
				return KITEM_BANANA;

			case KRITEM_TRIPLEORBINAUT:
			case KRITEM_QUADORBINAUT:
				return KITEM_ORBINAUT;

			case KRITEM_DUALJAWZ:
				return KITEM_JAWZ;

			case KRITEM_TRIPLEGACHABOM:
				return KITEM_GACHABOM;

			default:
				I_Error("Bad item cooldown redirect for result %d\n", getitem);
				break;
		}
	}

	return getitem;
}

UINT8 K_ItemResultToAmount(SINT8 getitem)
{
	switch (getitem)
	{
		case KRITEM_DUALSNEAKER:
		case KRITEM_DUALJAWZ:
			return 2;

		case KRITEM_TRIPLESNEAKER:
		case KRITEM_TRIPLEBANANA:
		case KRITEM_TRIPLEORBINAUT:
		case KRITEM_TRIPLEGACHABOM:
			return 3;

		case KRITEM_QUADORBINAUT:
			return 4;

		case KITEM_BALLHOG: // Not a special result, but has a special amount
			return 5;

		default:
			return 1;
	}
}

tic_t K_GetItemCooldown(SINT8 itemResult)
{
	SINT8 itemType = K_ItemResultToType(itemResult);

	if (itemType < 1 || itemType >= NUMKARTITEMS)
	{
		return 0;
	}

	return itemCooldowns[itemType - 1];
}

void K_SetItemCooldown(SINT8 itemResult, tic_t time)
{
	SINT8 itemType = K_ItemResultToType(itemResult);

	if (itemType < 1 || itemType >= NUMKARTITEMS)
	{
		return;
	}

	itemCooldowns[itemType - 1] = max(itemCooldowns[itemType - 1], time);
}

void K_RunItemCooldowns(void)
{
	size_t i;

	for (i = 0; i < NUMKARTITEMS-1; i++)
	{
		if (itemCooldowns[i] > 0)
		{
			itemCooldowns[i]--;
		}
	}
}

boolean K_TimeAttackRules(void)
{
	UINT8 playing = 0;
	UINT8 i;

	if ((gametyperules & (GTR_CATCHER|GTR_CIRCUIT)) == (GTR_CATCHER|GTR_CIRCUIT))
	{
		// Kind of a hack -- Special Stages
		// are expected to be 1-player, so
		// we won't use the Time Attack changes
		return false;
	}

	if (modeattacking != ATTACKING_NONE)
	{
		// Time Attack obviously uses Time Attack rules :p
		return true;
	}

	if (battleprisons == true)
	{
		// Break the Capsules always uses Time Attack
		// rules, since you can bring 2-4 players in
		// via Grand Prix.
		return true;
	}

	if (gametype == GT_TUTORIAL)
	{
		// Tutorials are special. By default only one
		// player will be playing... but sometimes bots
		// can be spawned! So we still guarantee the
		// changed behaviour for consistency.
		return true;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == false || players[i].spectator == true)
		{
			continue;
		}

		playing++;
		if (playing > 1)
		{
			break;
		}
	}

	// Use Time Attack gameplay rules with only 1P.
	return (playing <= 1);
}

boolean K_CapsuleTimeAttackRules(void)
{
	switch (cv_capsuletest.value)
	{
		case CV_CAPSULETEST_MULTIPLAYER:
			return false;

		case CV_CAPSULETEST_TIMEATTACK:
			return true;

		default:
			return K_TimeAttackRules();
	}
}

//}

//{ SRB2kart p_user.c Stuff

static fixed_t K_PlayerWeight(mobj_t *mobj, mobj_t *against)
{
	fixed_t weight = 5*FRACUNIT;

	if (!mobj->player)
		return weight;

	if (against && (against->type == MT_GARDENTOP || (against->player && against->player->curshield == KSHIELD_TOP)))
	{
		/* Players bumping into a Top get zero weight -- the
			Top rider is immovable. */
		weight = 0;
	}
	else if (against && !P_MobjWasRemoved(against) && against->player
		&& ((!P_PlayerInPain(against->player) && P_PlayerInPain(mobj->player)) // You're hurt
		|| (against->player->itemtype == KITEM_BUBBLESHIELD && mobj->player->itemtype != KITEM_BUBBLESHIELD))) // They have a Bubble Shield
	{
		weight = 0; // This player does not cause any bump action
	}
	else if (against && against->type == MT_SPECIAL_UFO)
	{
		weight = 0;
	}
	else
	{
		// Applies rubberbanding, to prevent rubberbanding bots
		// from causing super crazy bumps.
		fixed_t spd = K_GetKartSpeed(mobj->player, false, true);

		fixed_t speedfactor = 8 * mapobjectscale;

		weight = (mobj->player->kartweight) * FRACUNIT;

		if (against && against->type == MT_MONITOR)
		{
			speedfactor /= 5; // speed matters more
		}
		else
		{
			if (mobj->player->itemtype == KITEM_BUBBLESHIELD)
				weight += 9*FRACUNIT;
		}

		if (mobj->player->speed > spd)
			weight += FixedDiv((mobj->player->speed - spd), speedfactor);
	}

	return weight;
}

fixed_t K_GetMobjWeight(mobj_t *mobj, mobj_t *against)
{
	fixed_t weight = 5*FRACUNIT;

	switch (mobj->type)
	{
		case MT_PLAYER:
			if (!mobj->player)
				break;
			weight = K_PlayerWeight(mobj, against);
			break;
		case MT_KART_LEFTOVER:
			weight = 5*FRACUNIT/2;

			if (mobj->extravalue1 > 0)
			{
				weight = mobj->extravalue1 * (FRACUNIT >> 1);
			}
			break;
		case MT_BUBBLESHIELD:
			weight = K_PlayerWeight(mobj->target, against);
			break;
		case MT_FALLINGROCK:
			if (against->player)
			{
				if (against->player->invincibilitytimer || K_IsBigger(against, mobj) == true)
					weight = 0;
				else
					weight = K_PlayerWeight(against, NULL);
			}
			break;
		case MT_ORBINAUT:
		case MT_ORBINAUT_SHIELD:
		case MT_GACHABOM:
		case MT_DUELBOMB:
			if (against->player)
				weight = K_PlayerWeight(against, NULL);
			break;
		case MT_JAWZ:
		case MT_JAWZ_SHIELD:
			if (against->player)
				weight = K_PlayerWeight(against, NULL) + (3*FRACUNIT);
			else
				weight += 3*FRACUNIT;
			break;
		case MT_DROPTARGET:
		case MT_DROPTARGET_SHIELD:
			if (against->player)
				weight = K_PlayerWeight(against, NULL);
		default:
			break;
	}

	return FixedMul(weight, mobj->scale);
}

static void K_SpawnBumpForObjs(mobj_t *mobj1, mobj_t *mobj2)
{
	if (mobj1->type == MT_KART_LEFTOVER && mobj1->health == 0)
	{
		return;
	}

	mobj_t *fx = P_SpawnMobj(
		mobj1->x/2 + mobj2->x/2,
		mobj1->y/2 + mobj2->y/2,
		mobj1->z/2 + mobj2->z/2,
		MT_BUMP
	);
	fixed_t avgScale = (mobj1->scale + mobj2->scale) / 2;

	if (mobj1->eflags & MFE_VERTICALFLIP)
	{
		fx->eflags |= MFE_VERTICALFLIP;
	}
	else
	{
		fx->eflags &= ~MFE_VERTICALFLIP;
	}

	P_SetScale(fx, (fx->destscale = avgScale));

	if ((mobj1->player && mobj1->player->itemtype == KITEM_BUBBLESHIELD)
	|| (mobj2->player && mobj2->player->itemtype == KITEM_BUBBLESHIELD))
	{
		S_StartSound(mobj1, sfx_s3k44);
	}
	else if (mobj1->type == MT_DROPTARGET || mobj1->type == MT_DROPTARGET_SHIELD) // no need to check the other way around
	{
		// Sound handled in K_DropTargetCollide
		// S_StartSound(mobj2, sfx_s258);
		fx->colorized = true;
		fx->color = mobj1->color;
	}
	else
	{
		S_StartSound(mobj1, sfx_s3k49);
	}
}

static void K_PlayerJustBumped(player_t *player)
{
	mobj_t *playerMobj = NULL;

	if (player == NULL)
	{
		return;
	}

	playerMobj = player->mo;

	if (playerMobj == NULL || P_MobjWasRemoved(playerMobj))
	{
		return;
	}

	if (abs(player->rmomx) < playerMobj->scale && abs(player->rmomy) < playerMobj->scale)
	{
		// Because this is done during collision now, rmomx and rmomy need to be recalculated
		// so that friction doesn't immediately decide to stop the player if they're at a standstill
		player->rmomx = playerMobj->momx - player->cmomx;
		player->rmomy = playerMobj->momy - player->cmomy;
	}

	player->justbumped = bumptime;
	player->noEbrakeMagnet = ebraketime;
	player->spindash = 0;

	// If spinouttimer is not set yet but could be set later,
	// this lets the bump still trigger wipeout friction. If
	// spinouttimer never gets set, then this has no effect on
	// friction and gets unset anyway.
	player->wipeoutslow = wipeoutslowtime+1;

	if (player->spinouttimer)
	{
		player->spinouttimer = max(wipeoutslowtime+1, player->spinouttimer);
		//player->spinouttype = KSPIN_WIPEOUT; // Enforce type
	}
}

static boolean K_JustBumpedException(mobj_t *mobj)
{
	switch (mobj->type)
	{
		case MT_SA2_CRATE:
			return Obj_SA2CrateIsMetal(mobj);
		case MT_WALLSPIKE:
			return true;
		case MT_BATTLECAPSULE:
		{
			if (gametype == GT_TUTORIAL // Remove gametype check whenever it's safe to break compatibility with ghosts in a post-release patch
			&& mobj->momx == 0
			&& mobj->momy == 0
			&& mobj->momz == 0)
			{
				return true;
			}
			break;
		}
		default:
			break;
	}

	if (mobj->flags & MF_PUSHABLE)
		return true;

	return false;
}

static fixed_t K_GetBounceForce(mobj_t *mobj1, mobj_t *mobj2, fixed_t distx, fixed_t disty)
{
	const fixed_t forceMul = (4 * FRACUNIT) / 10; // Multiply by this value to make it feel like old bumps.

	fixed_t momdifx, momdify;
	fixed_t dot;
	fixed_t force = 0;

	momdifx = mobj1->momx - mobj2->momx;
	momdify = mobj1->momy - mobj2->momy;

	if (distx == 0 && disty == 0)
	{
		// if there's no distance between the 2, they're directly on top of each other, don't run this
		return 0;
	}

	{ // Normalize distance to the sum of the two objects' radii, since in a perfect world that would be the distance at the point of collision...
		fixed_t dist = P_AproxDistance(distx, disty);
		fixed_t nx, ny;

		dist = dist ? dist : 1;

		nx = FixedDiv(distx, dist);
		ny = FixedDiv(disty, dist);

		distx = FixedMul(mobj1->radius + mobj2->radius, nx);
		disty = FixedMul(mobj1->radius + mobj2->radius, ny);

		if (momdifx == 0 && momdify == 0)
		{
			// If there's no momentum difference, they're moving at exactly the same rate. Pretend they moved into each other.
			momdifx = -nx;
			momdify = -ny;
		}
	}

	dot = FixedMul(momdifx, distx) + FixedMul(momdify, disty);

	if (dot >= 0)
	{
		// They're moving away from each other
		return 0;
	}

	// Return the push force!
	force = FixedDiv(dot, FixedMul(distx, distx) + FixedMul(disty, disty));

	return FixedMul(force, forceMul);
}

boolean K_KartBouncing(mobj_t *mobj1, mobj_t *mobj2)
{
	const fixed_t minBump = 25*mapobjectscale;
	mobj_t *goombaBounce = NULL;
	fixed_t distx, disty, dist;
	fixed_t force;
	fixed_t mass1, mass2;

	if ((!mobj1 || P_MobjWasRemoved(mobj1))
	|| (!mobj2 || P_MobjWasRemoved(mobj2)))
	{
		return false;
	}

	// Don't bump when you're being reborn
	if ((mobj1->player && mobj1->player->playerstate != PST_LIVE)
		|| (mobj2->player && mobj2->player->playerstate != PST_LIVE))
		return false;

	if ((mobj1->player && mobj1->player->respawn.state != RESPAWNST_NONE)
		|| (mobj2->player && mobj2->player->respawn.state != RESPAWNST_NONE))
		return false;

	if (mobj1->type != MT_DROPTARGET && mobj1->type != MT_DROPTARGET_SHIELD)
	{ // Don't bump if you're flashing
		INT32 flash;

		flash = K_GetKartFlashing(mobj1->player);
		if (mobj1->player && mobj1->player->flashing > 0 && mobj1->player->flashing < flash)
		{
			if (mobj1->player->flashing < flash-1)
				mobj1->player->flashing++;
			return false;
		}

		flash = K_GetKartFlashing(mobj2->player);
		if (mobj2->player && mobj2->player->flashing > 0 && mobj2->player->flashing < flash)
		{
			if (mobj2->player->flashing < flash-1)
				mobj2->player->flashing++;
			return false;
		}
	}

	// Don't bump if you've recently bumped
	if (mobj1->player && mobj1->player->justbumped && !K_JustBumpedException(mobj2))
	{
		mobj1->player->justbumped = bumptime;
		mobj1->player->noEbrakeMagnet = ebraketime;
		return false;
	}

	if (mobj2->player && mobj2->player->justbumped && !K_JustBumpedException(mobj1))
	{
		mobj2->player->justbumped = bumptime;
		mobj2->player->noEbrakeMagnet = ebraketime;
		return false;
	}

	// Adds the OTHER object's momentum times a bunch, for the best chance of getting the correct direction
	distx = (mobj1->x + mobj2->momx) - (mobj2->x + mobj1->momx);
	disty = (mobj1->y + mobj2->momy) - (mobj2->y + mobj1->momy);

	force = K_GetBounceForce(mobj1, mobj2, distx, disty);

	if (force == 0)
	{
		return false;
	}

	mass1 = K_GetMobjWeight(mobj1, mobj2);
	mass2 = K_GetMobjWeight(mobj2, mobj1);

	if ((P_IsObjectOnGround(mobj1) && mobj2->momz < 0) // Grounded
		|| (mass2 == 0 && mass1 > 0)) // The other party is immovable
	{
		goombaBounce = mobj2;
	}
	else if ((P_IsObjectOnGround(mobj2) && mobj1->momz < 0) // Grounded
		|| (mass1 == 0 && mass2 > 0)) // The other party is immovable
	{
		goombaBounce = mobj1;
	}

	if (goombaBounce != NULL)
	{
		// Perform a Goomba Bounce by reversing your z momentum.
		goombaBounce->momz = -goombaBounce->momz;
	}
	else
	{
		// Trade z momentum values.
		fixed_t newz = mobj1->momz;
		mobj1->momz = mobj2->momz;
		mobj2->momz = newz;
	}

	// Multiply by force
	distx = FixedMul(force, distx);
	disty = FixedMul(force, disty);
	dist = FixedHypot(distx, disty);

	// if the speed difference is less than this let's assume they're going proportionately faster from each other
	if (dist < minBump)
	{
		fixed_t normalisedx = FixedDiv(distx, dist);
		fixed_t normalisedy = FixedDiv(disty, dist);

		distx = FixedMul(minBump, normalisedx);
		disty = FixedMul(minBump, normalisedy);
	}

	if (mass2 > 0)
	{
		mobj1->momx = mobj1->momx - FixedMul(FixedDiv(2*mass2, mass1 + mass2), distx);
		mobj1->momy = mobj1->momy - FixedMul(FixedDiv(2*mass2, mass1 + mass2), disty);
	}

	if (mass1 > 0)
	{
		mobj2->momx = mobj2->momx - FixedMul(FixedDiv(2*mass1, mass1 + mass2), -distx);
		mobj2->momy = mobj2->momy - FixedMul(FixedDiv(2*mass1, mass1 + mass2), -disty);
	}

	K_SpawnBumpForObjs(mobj1, mobj2);

	if (mobj1->type == MT_PLAYER && mobj2->type == MT_PLAYER
		&& !mobj1->player->powerupVFXTimer && !mobj2->player->powerupVFXTimer)
	{
		boolean guard1 = K_PlayerGuard(mobj1->player);
		boolean guard2 = K_PlayerGuard(mobj2->player);

		if (guard1 && guard2)
			K_DoPowerClash(mobj1, mobj2);
		else if (guard1)
			K_DoGuardBreak(mobj1, mobj2);
		else if (guard2)
			K_DoGuardBreak(mobj2, mobj1);
	}

	K_PlayerJustBumped(mobj1->player);
	K_PlayerJustBumped(mobj2->player);

	return true;
}

// K_KartBouncing, but simplified to act like P_BouncePlayerMove
boolean K_KartSolidBounce(mobj_t *bounceMobj, mobj_t *solidMobj)
{
	const fixed_t minBump = 25*mapobjectscale;
	fixed_t distx, disty;
	fixed_t force;

	if ((!bounceMobj || P_MobjWasRemoved(bounceMobj))
	|| (!solidMobj || P_MobjWasRemoved(solidMobj)))
	{
		return false;
	}

	// Don't bump when you're being reborn
	if (bounceMobj->player && bounceMobj->player->playerstate != PST_LIVE)
		return false;

	if (bounceMobj->player && bounceMobj->player->respawn.state != RESPAWNST_NONE)
		return false;

	// Don't bump if you've recently bumped
	if (bounceMobj->player && bounceMobj->player->justbumped && !K_JustBumpedException(solidMobj))
	{
		bounceMobj->player->justbumped = bumptime;
		return false;
	}

	// Adds the OTHER object's momentum times a bunch, for the best chance of getting the correct direction
	{
		distx = (bounceMobj->x + solidMobj->momx) - (solidMobj->x + bounceMobj->momx);
		disty = (bounceMobj->y + solidMobj->momy) - (solidMobj->y + bounceMobj->momy);
	}

	force = K_GetBounceForce(bounceMobj, solidMobj, distx, disty);

	if (force == 0)
	{
		return false;
	}

	{
		// Normalize to the desired push value.
		fixed_t normalisedx;
		fixed_t normalisedy;
		fixed_t bounceSpeed;

		// Multiply by force
		distx = FixedMul(force, distx);
		disty = FixedMul(force, disty);
		fixed_t dist = FixedHypot(distx, disty);

		normalisedx = FixedDiv(distx, dist);
		normalisedy = FixedDiv(disty, dist);

		if (solidMobj->type == MT_WALLSPIKE)
		{
			fixed_t co = FCOS(solidMobj->angle);
			fixed_t si = FSIN(solidMobj->angle);

			// Always thrust out toward the tip
			normalisedx = FixedMul(normalisedx, abs(si)) - co;
			normalisedy = FixedMul(normalisedy, abs(co)) - si;
		}

		bounceSpeed = FixedHypot(bounceMobj->momx, bounceMobj->momy);
		bounceSpeed = FixedMul(bounceSpeed, (FRACUNIT - (FRACUNIT>>2) - (FRACUNIT>>3)));
		bounceSpeed += minBump;

		distx = FixedMul(bounceSpeed, normalisedx);
		disty = FixedMul(bounceSpeed, normalisedy);
	}

	bounceMobj->momx = bounceMobj->momx - distx;
	bounceMobj->momy = bounceMobj->momy - disty;
	bounceMobj->momz = -bounceMobj->momz;

	K_SpawnBumpForObjs(bounceMobj, solidMobj);
	K_PlayerJustBumped(bounceMobj->player);

	return true;
}

/**	\brief	Checks that the player is on an offroad subsector for realsies. Also accounts for line riding to prevent cheese.

	\param	mo	player mobj object

	\return	boolean
*/
static fixed_t K_CheckOffroadCollide(mobj_t *mo)
{
	// Check for sectors in touching_sectorlist
	msecnode_t *node;	// touching_sectorlist iter
	sector_t *s;		// main sector shortcut
	sector_t *s2;		// FOF sector shortcut
	ffloor_t *rover;	// FOF

	fixed_t flr;
	fixed_t cel;	// floor & ceiling for height checks to make sure we're touching the offroad sector.

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (!node->m_sector)
			break;	// shouldn't happen.

		s = node->m_sector;
		// 1: Check for the main sector, make sure we're on the floor of that sector and see if we can apply offroad.
		// Make arbitrary Z checks because we want to check for 1 sector in particular, we don't want to affect the player if the offroad sector is way below them and they're lineriding a normal sector above.

		flr = P_MobjFloorZ(mo, s, s, mo->x, mo->y, NULL, false, true);
		cel = P_MobjCeilingZ(mo, s, s, mo->x, mo->y, NULL, true, true);	// get Z coords of both floors and ceilings for this sector (this accounts for slopes properly.)
		// NOTE: we don't use P_GetZAt with our x/y directly because the mobj won't have the same height because of its hitbox on the slope. Complex garbage but tldr it doesn't work.

		if ( ((s->flags & MSF_FLIPSPECIAL_FLOOR) && mo->z == flr)	// floor check
			|| ((mo->eflags & MFE_VERTICALFLIP && (s->flags & MSF_FLIPSPECIAL_CEILING) && (mo->z + mo->height) == cel)) )	// ceiling check.
		{
			return s->offroad;
		}

		// 2: If we're here, we haven't found anything. So let's try looking for FOFs in the sectors using the same logic.
		for (rover = s->ffloors; rover; rover = rover->next)
		{
			if (!(rover->fofflags & FOF_EXISTS))	// This FOF doesn't exist anymore.
				continue;

			s2 = &sectors[rover->secnum];	// makes things easier for us

			flr = P_GetFOFBottomZ(mo, s, rover, mo->x, mo->y, NULL);
			cel = P_GetFOFTopZ(mo, s, rover, mo->x, mo->y, NULL);	// Z coords for fof top/bottom.

			// we will do essentially the same checks as above instead of bothering with top/bottom height of the FOF.
			// Reminder that an FOF's floor is its bottom, silly!
			if ( ((s2->flags & MSF_FLIPSPECIAL_FLOOR) && mo->z == cel)	// "floor" check
				|| ((s2->flags & MSF_FLIPSPECIAL_CEILING) && (mo->z + mo->height) == flr) )	// "ceiling" check.
			{
				return s2->offroad;
			}
		}
	}

	return 0; // couldn't find any offroad
}

/**	\brief	Updates the Player's offroad value once per frame

	\param	player	player object passed from K_KartPlayerThink

	\return	void
*/
static void K_UpdateOffroad(player_t *player)
{
	terrain_t *terrain = player->mo->terrain;
	fixed_t offroadstrength = 0;

	// If tiregrease is active, don't
	// If inside an ice cube, don't
	if (player->tiregrease == 0 && player->icecube.frozen == false)
	{
		// TODO: Make this use actual special touch code.
		if (terrain != NULL && terrain->offroad > 0)
		{
			offroadstrength = (terrain->offroad << FRACBITS);
		}
		else
		{
			offroadstrength = K_CheckOffroadCollide(player->mo);
		}
	}

	// If you are in offroad, a timer starts.
	if (offroadstrength)
	{
		UINT8 offramp = (K_GetKartGameSpeedScalar(gamespeed) > FRACUNIT ? 2 : 1);

		if (player->offroad < offroadstrength)
			player->offroad += offroadstrength * offramp / TICRATE;

		if (player->offroad > offroadstrength)
			player->offroad = offroadstrength;

		if (player->roundconditions.touched_offroad == false
			&& !(player->exiting || (player->pflags & PF_NOCONTEST))
			&& player->offroad > (2*offroadstrength) / TICRATE)
		{
			player->roundconditions.touched_offroad = true;
			player->roundconditions.checkthisframe = true;
		}
	}
	else
		player->offroad = 0;
}

static void K_DrawDraftCombiring(player_t *player, mobj_t *victim, fixed_t curdist, fixed_t maxdist, boolean transparent)
{
#define CHAOTIXBANDLEN 15
#define CHAOTIXBANDCOLORS 9
	static const UINT8 colors[CHAOTIXBANDCOLORS] = {
		SKINCOLOR_SAPPHIRE,
		SKINCOLOR_PLATINUM,
		SKINCOLOR_TEA,
		SKINCOLOR_GARDEN,
		SKINCOLOR_BANANA,
		SKINCOLOR_GOLD,
		SKINCOLOR_ORANGE,
		SKINCOLOR_SCARLET,
		SKINCOLOR_TAFFY
	};
	fixed_t minimumdist = FixedMul(RING_DIST>>1, player->mo->scale);
	UINT8 n = CHAOTIXBANDLEN;
	UINT8 offset = ((leveltime / 3) % 3);
	fixed_t stepx, stepy, stepz;
	fixed_t curx, cury, curz;
	UINT8 c;

	if (maxdist == 0)
	{
		c = leveltime % CHAOTIXBANDCOLORS;
	}
	else
	{
		fixed_t num = curdist - minimumdist;
		fixed_t den = maxdist - minimumdist;
		if (den < 1)
			den = 1;
		if (num < 0)
			num = 0;
		if (num > den)
			num = den;
		c = FixedMul((CHAOTIXBANDCOLORS - 1)<<FRACBITS, FixedDiv(num, den)) >> FRACBITS;
	}

	stepx = (victim->x - player->mo->x) / CHAOTIXBANDLEN;
	stepy = (victim->y - player->mo->y) / CHAOTIXBANDLEN;
	stepz = ((victim->z + (victim->height / 2)) - (player->mo->z + (player->mo->height / 2))) / CHAOTIXBANDLEN;

	curx = player->mo->x + stepx;
	cury = player->mo->y + stepy;
	curz = player->mo->z + stepz;

	while (n)
	{
		if (offset == 0)
		{
			mobj_t *band = P_SpawnMobj(curx + (P_RandomRange(PR_DECORATION, -12, 12)*mapobjectscale),
				cury + (P_RandomRange(PR_DECORATION, -12, 12)*mapobjectscale),
				curz + (P_RandomRange(PR_DECORATION, 24, 48)*mapobjectscale),
				MT_SIGNSPARKLE);

			if (maxdist == 0)
			{
				P_SetMobjState(band, S_KSPARK1 + (leveltime % 8));
				P_SetScale(band, (band->destscale = player->mo->scale));
			}
			else
			{
				P_SetMobjState(band, S_SIGNSPARK1 + (leveltime % 11));
				P_SetScale(band, (band->destscale = (3*player->mo->scale)/2));
			}

			band->color = colors[c];
			band->colorized = true;

			band->fuse = 2;

			if (transparent)
				band->renderflags |= RF_GHOSTLY;

			band->renderflags |= RF_DONTDRAW & ~(K_GetPlayerDontDrawFlag(player) | K_GetPlayerDontDrawFlag(victim->player));
		}

		curx += stepx;
		cury += stepy;
		curz += stepz;

		offset = abs(offset-1) % 3;
		n--;
	}
#undef CHAOTIXBANDLEN
}

static boolean K_HasInfiniteTether(player_t *player)
{
	switch (player->curshield)
	{
		case KSHIELD_LIGHTNING:
			return true;
	}

	if (player->eggmanexplode > 0)
		return true;

	if (player->trickcharge)
		return true;

	if (player->infinitether)
		return true;

	return false;
}

static boolean K_TryDraft(player_t *player, mobj_t *dest, fixed_t minDist, fixed_t draftdistance, UINT8 leniency)
{
//#define EASYDRAFTTEST
	fixed_t dist, olddraft;
	fixed_t theirSpeed = 0;
#ifndef EASYDRAFTTEST
	angle_t yourangle, theirangle, diff;
#endif

	if (K_PodiumSequence() == true)
	{
		return false;
	}

#ifndef EASYDRAFTTEST
	// Don't draft on yourself :V
	if (dest->player && dest->player == player)
	{
		return false;
	}
#endif

	if (dest->player != NULL)
	{
		// No tethering off of the guy who got the starting bonus :P
		if (dest->player->startboost > 0)
		{
			return false;
		}

		theirSpeed = dest->player->speed;
	}
	else
	{
		theirSpeed = R_PointToDist2(0, 0, dest->momx, dest->momy);
	}

	// They're not enough speed to draft off of them.
	if (theirSpeed < 20 * dest->scale)
	{
		return false;
	}

#ifndef EASYDRAFTTEST
	yourangle = K_MomentumAngle(player->mo);
	theirangle = K_MomentumAngle(dest);

	// Not in front of this player.
	diff = AngleDelta(R_PointToAngle2(player->mo->x, player->mo->y, dest->x, dest->y), yourangle);
	if (diff > ANG10)
	{
		return false;
	}

	// Not moving in the same direction.
	diff = AngleDelta(yourangle, theirangle);
	if (diff > ANGLE_90)
	{
		return false;
	}
#endif

	dist = P_AproxDistance(P_AproxDistance(dest->x - player->mo->x, dest->y - player->mo->y), dest->z - player->mo->z);

#ifndef EASYDRAFTTEST
	// TOO close to draft.
	if (dist < minDist)
	{
		return false;
	}

	// Not close enough to draft.
	if (dist > draftdistance && draftdistance > 0)
	{
		return false;
	}
#endif

	// Bots are unusually good at keeping their facing aligned on long, tight turns.
	// Force them to give up tether in these situations, like a drifting player typically would.
	UINT16 rejectThreshold = KART_FULLTURN/4;
	if (K_PlayerUsesBotMovement(player) && (abs(player->oldcmd.turning + player->cmd.turning) >= rejectThreshold))
	{
		return false;
	}

	olddraft = player->draftpower;

	player->draftleeway = leniency;

	if (dest->player != NULL)
	{
		player->lastdraft = dest->player - players;
	}
	else
	{
		player->lastdraft = MAXPLAYERS;
	}

	// Draft power is used later in K_GetKartBoostPower, ranging from 0 for normal speed and FRACUNIT for max draft speed.
	// How much this increments every tic biases toward acceleration! (min speed gets 1.5% per tic, max speed gets 0.5% per tic)
	if (player->draftpower < FRACUNIT)
	{
		fixed_t add = (FRACUNIT/200) + ((9 - player->kartspeed) * ((3*FRACUNIT)/1600));;
		player->draftpower += add;

		if (player->bot && (player->botvars.rival || cv_levelskull.value))
		{
			// Double speed for the rival!
			player->draftpower += add;
		}

		if (gametyperules & GTR_CLOSERPLAYERS)
		{
			// Double speed in smaller environments
			player->draftpower += add;
		}
	}

	if (player->draftpower > FRACUNIT)
	{
		player->draftpower = FRACUNIT;
	}

	// Play draft finish noise
	if (olddraft < FRACUNIT && player->draftpower >= FRACUNIT)
	{
		S_StartSound(player->mo, sfx_cdfm62);
	}

	// Spawn in the visual!
	K_DrawDraftCombiring(player, dest, dist, draftdistance, false);
	return true;
}

/**	\brief	Updates the player's drafting values once per frame

	\param	player	player object passed from K_KartPlayerThink

	\return	void
*/
static void K_UpdateDraft(player_t *player)
{
	mobj_t *addUfo = K_GetPossibleSpecialTarget();

	fixed_t topspd = K_GetKartSpeed(player, false, false);
	fixed_t draftdistance;
	fixed_t minDist;
	UINT8 leniency;
	UINT8 i;

	if (K_HasInfiniteTether(player))
	{
		// Lightning Shield gets infinite draft distance as its (other) passive effect.
		draftdistance = 0;
	}
	else
	{
		// Distance you have to be to draft. If you're still accelerating, then this distance is lessened.
		// This distance biases toward low weight! (min weight gets 4096 units, max weight gets 3072 units)
		// This distance is also scaled based on game speed.
		draftdistance = (3072 + (128 * (9 - player->kartweight))) * player->mo->scale;
		if (player->speed < topspd)
			draftdistance = FixedMul(draftdistance, FixedDiv(player->speed, topspd));
		draftdistance = FixedMul(draftdistance, K_GetKartGameSpeedScalar(gamespeed));
	}

	// On the contrary, the leniency period biases toward high weight.
	// (See also: the leniency variable in K_SpawnDraftDust)
	leniency = (3*TICRATE)/4 + ((player->kartweight-1) * (TICRATE/4));

	minDist = 640 * player->mo->scale;

	if (gametyperules & GTR_CLOSERPLAYERS)
	{
		minDist /= 4;
		draftdistance *= 2;
		leniency *= 4;
	}

	// You need speed and commitment to draft.
	if (player->speed >= 20 * player->mo->scale
		&& player->instaWhipCharge < INSTAWHIP_TETHERBLOCK && !player->defenseLockout)
	{
		if (addUfo != NULL)
		{
			// Tether off of the UFO!
			if (K_TryDraft(player, addUfo, minDist, draftdistance, leniency) == true)
			{
				return; // Finished doing our draft.
			}
		}

		// Let's hunt for players to draft off of!
		for (i = 0; i < MAXPLAYERS; i++)
		{
			player_t *otherPlayer = NULL;

			if (playeringame[i] == false)
			{
				continue;
			}

			otherPlayer = &players[i];

			if (otherPlayer->spectator == true)
			{
				continue;
			}

			if (otherPlayer->mo == NULL || P_MobjWasRemoved(otherPlayer->mo) == true)
			{
				continue;
			}

			if (K_TryDraft(player, otherPlayer->mo, minDist, draftdistance, leniency) == true)
			{
				//return;
				goto draftdurationhandling; // Finished doing our draft.
			}
		}
	}

	// No one to draft off of? Then you can knock that off.
	if (player->draftleeway > 0) // Prevent small disruptions from stopping your draft.
	{
		if (P_IsObjectOnGround(player->mo) == true)
		{
			// Allow maintaining tether in air setpieces.
			player->draftleeway--;
		}

		if (player->lastdraft >= 0
			&& player->lastdraft < MAXPLAYERS
			&& playeringame[player->lastdraft]
			&& !players[player->lastdraft].spectator
			&& players[player->lastdraft].mo)
		{
			player_t *victim = &players[player->lastdraft];
			fixed_t dist = P_AproxDistance(P_AproxDistance(victim->mo->x - player->mo->x, victim->mo->y - player->mo->y), victim->mo->z - player->mo->z);
			K_DrawDraftCombiring(player, victim->mo, dist, draftdistance, true);
		}
		else if (addUfo != NULL)
		{
			// kind of a hack to not have to mess with how lastdraft works
			fixed_t dist = P_AproxDistance(P_AproxDistance(addUfo->x - player->mo->x, addUfo->y - player->mo->y), addUfo->z - player->mo->z);
			K_DrawDraftCombiring(player, addUfo, dist, draftdistance, true);
		}
	}
	else // Remove draft speed boost.
	{
		player->draftpower = 0;
		player->lastdraft = -1;
		player->roundconditions.continuousdraft = 0;
		return;
	}

draftdurationhandling:
	player->roundconditions.continuousdraft++;
	if (player->roundconditions.continuousdraft > player->roundconditions.continuousdraft_best)
		player->roundconditions.continuousdraft_best = player->roundconditions.continuousdraft;
}

void K_KartPainEnergyFling(player_t *player)
{
	static const UINT8 numfling = 5;
	INT32 i;
	mobj_t *mo;
	angle_t fa;
	fixed_t ns;
	fixed_t z;

	// Better safe than sorry.
	if (!player)
		return;

	// P_PlayerRingBurst: "There's no ring spilling in kart, so I'm hijacking this for the same thing as TD"
	// :oh:

	for (i = 0; i < numfling; i++)
	{
		INT32 objType = mobjinfo[MT_FLINGENERGY].reactiontime;
		fixed_t momxy, momz; // base horizonal/vertical thrusts

		z = player->mo->z;
		if (player->mo->eflags & MFE_VERTICALFLIP)
			z += player->mo->height - mobjinfo[objType].height;

		mo = P_SpawnMobj(player->mo->x, player->mo->y, z, objType);

		mo->fuse = 8*TICRATE;
		P_SetTarget(&mo->target, player->mo);

		mo->destscale = player->mo->scale;
		P_SetScale(mo, player->mo->scale);

		// Angle offset by player angle, then slightly offset by amount of fling
		fa = ((i*FINEANGLES/16) + (player->mo->angle>>ANGLETOFINESHIFT) - ((numfling-1)*FINEANGLES/32)) & FINEMASK;

		if (i > 15)
		{
			momxy = 3*FRACUNIT;
			momz = 4*FRACUNIT;
		}
		else
		{
			momxy = 28*FRACUNIT;
			momz = 3*FRACUNIT;
		}

		ns = FixedMul(momxy, mo->scale);
		mo->momx = FixedMul(FINECOSINE(fa),ns);

		ns = momz;
		P_SetObjectMomZ(mo, ns, false);

		if (i & 1)
			P_SetObjectMomZ(mo, ns, true);

		if (player->mo->eflags & MFE_VERTICALFLIP)
			mo->momz *= -1;
	}
}

// Adds gravity flipping to an object relative to its master and shifts the z coordinate accordingly.
void K_FlipFromObject(mobj_t *mo, mobj_t *master)
{
	mo->eflags = (mo->eflags & ~MFE_VERTICALFLIP)|(master->eflags & MFE_VERTICALFLIP);
	mo->flags2 = (mo->flags2 & ~MF2_OBJECTFLIP)|(master->flags2 & MF2_OBJECTFLIP);

	if (mo->eflags & MFE_VERTICALFLIP)
		mo->z += master->height - FixedMul(master->scale, mo->height);
}

void K_MatchGenericExtraFlags(mobj_t *mo, mobj_t *master)
{
	// flipping
	// handle z shifting from there too. This is here since there's no reason not to flip us if needed when we do this anyway;
	K_FlipFromObject(mo, master);

	// visibility (usually for hyudoro)
	mo->renderflags = (mo->renderflags & ~RF_DONTDRAW) | (master->renderflags & RF_DONTDRAW);
}

// same as above, but does not adjust Z height when flipping
void K_GenericExtraFlagsNoZAdjust(mobj_t *mo, mobj_t *master)
{
	// flipping
	mo->eflags = (mo->eflags & ~MFE_VERTICALFLIP)|(master->eflags & MFE_VERTICALFLIP);
	mo->flags2 = (mo->flags2 & ~MF2_OBJECTFLIP)|(master->flags2 & MF2_OBJECTFLIP);

	// visibility (usually for hyudoro)
	mo->renderflags = (mo->renderflags & ~RF_DONTDRAW) | (master->renderflags & RF_DONTDRAW);
}


void K_SpawnDashDustRelease(player_t *player)
{
	fixed_t newx;
	fixed_t newy;
	mobj_t *dust;
	angle_t travelangle;
	INT32 i;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (!P_IsObjectOnGround(player->mo))
		return;

	if (!player->speed && !player->startboost && !player->spindash && !player->dropdashboost)
		return;

	travelangle = player->mo->angle;

	if (player->drift || (player->pflags & PF_DRIFTEND))
		travelangle -= (ANGLE_45/5)*player->drift;

	for (i = 0; i < 2; i++)
	{
		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_90, FixedMul(48*FRACUNIT, player->mo->scale));
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_90, FixedMul(48*FRACUNIT, player->mo->scale));
		dust = P_SpawnMobj(newx, newy, player->mo->z, MT_FASTDUST);

		P_SetTarget(&dust->target, player->mo);
		dust->angle = travelangle - (((i&1) ? -1 : 1) * ANGLE_45);
		dust->destscale = player->mo->scale;
		P_SetScale(dust, player->mo->scale);

		dust->momx = 3*player->mo->momx/5;
		dust->momy = 3*player->mo->momy/5;
		dust->momz = 3*P_GetMobjZMovement(player->mo)/5;

		K_MatchGenericExtraFlags(dust, player->mo);
	}
}

static fixed_t K_GetBrakeFXScale(player_t *player, fixed_t maxScale)
{
	fixed_t s = FixedDiv(player->speed,
			K_GetKartSpeed(player, false, false));

	s = max(s, FRACUNIT);
	s = min(s, maxScale);

	return s;
}

static void K_SpawnBrakeDriftSparks(player_t *player) // Be sure to update the mobj thinker case too!
{
	mobj_t *sparks;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	// Position & etc are handled in its thinker, and its spawned invisible.
	// This avoids needing to dupe code if we don't need it.
	sparks = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BRAKEDRIFT);
	P_SetTarget(&sparks->target, player->mo);
	P_SetScale(sparks, (sparks->destscale = FixedMul(K_GetBrakeFXScale(player, 3*FRACUNIT), player->mo->scale)));
	K_MatchGenericExtraFlags(sparks, player->mo);
	sparks->renderflags |= RF_DONTDRAW;
}

static void
spawn_brake_dust
(		mobj_t * master,
		angle_t aoff,
		fixed_t rad,
		fixed_t scale)
{
	const angle_t a = master->angle + aoff;

	mobj_t *spark = P_SpawnMobjFromMobj(master,
			P_ReturnThrustX(NULL, a, rad),
			P_ReturnThrustY(NULL, a, rad), 0,
			MT_BRAKEDUST);

	spark->momx = master->momx;
	spark->momy = master->momy;
	spark->momz = P_GetMobjZMovement(master);
	spark->angle = a - ANGLE_180;
	spark->pitch = master->pitch;
	spark->roll = master->roll;

	P_Thrust(spark, a, 16 * spark->scale);

	P_SetScale(spark, (spark->destscale =
				FixedMul(scale, spark->scale)));

	P_SetTarget(&spark->owner, master);
	spark->renderflags |= RF_REDUCEVFX;
}

static void K_SpawnBrakeVisuals(player_t *player)
{
	const fixed_t scale =
		K_GetBrakeFXScale(player, 2*FRACUNIT);

	if (leveltime & 1)
	{
		angle_t aoff;
		fixed_t radf;

		UINT8 wheel = 3;

		if (player->drift)
		{
			/* brake-drifting: dust flies from outer wheel */
			wheel ^= 1 << (player->drift < 0);

			aoff = 7 * ANG10;
			radf = 32 * FRACUNIT;
		}
		else
		{
			aoff = ANG30;
			radf = 24 * FRACUNIT;
		}

		if (wheel & 1)
		{
			spawn_brake_dust(player->mo,
					aoff, radf, scale);
		}

		if (wheel & 2)
		{
			spawn_brake_dust(player->mo,
					InvAngle(aoff), radf, scale);
		}
	}

	if (leveltime % 4 == 0)
		S_ReducedVFXSound(player->mo, sfx_s3k67, player);

	/* vertical shaking, scales with speed */
	player->mo->spriteyoffset = P_RandomFlip(2 * scale);
}

void K_SpawnDriftBoostClip(player_t *player)
{
	mobj_t *clip;
	fixed_t scale = 115*FRACUNIT/100;
	fixed_t momz = P_GetMobjZMovement(player->mo);
	fixed_t z;

	if (( player->mo->eflags & MFE_VERTICALFLIP ))
		z = player->mo->z;
	else
		z = player->mo->z + player->mo->height;

	clip = P_SpawnMobj(player->mo->x, player->mo->y, z, MT_DRIFTCLIP);

	P_SetTarget(&clip->target, player->mo);
	P_SetScale(clip, ( clip->destscale = FixedMul(scale, player->mo->scale) ));
	K_MatchGenericExtraFlags(clip, player->mo);

	clip->fuse = 105;
	clip->momz = 7 * P_MobjFlip(clip) * clip->scale;

	if (momz > 0)
		clip->momz += momz;

	P_InstaThrust(clip, player->mo->angle +
			P_RandomFlip(P_RandomRange(PR_DECORATION, FRACUNIT/2, FRACUNIT)),
			FixedMul(scale, player->speed));

	P_SetTarget(&clip->owner, player->mo);
	clip->renderflags |= RF_REDUCEVFX;
}

void K_SpawnDriftBoostClipSpark(mobj_t *clip)
{
	mobj_t *spark;

	spark = P_SpawnMobj(clip->x, clip->y, clip->z, MT_DRIFTCLIPSPARK);

	P_SetTarget(&spark->target, clip);
	P_SetScale(spark, ( spark->destscale = clip->scale ));
	K_MatchGenericExtraFlags(spark, clip);

	spark->momx = clip->momx/2;
	spark->momy = clip->momx/2;
}

static void K_SpawnGenericSpeedLines(player_t *player, boolean top)
{
	mobj_t *fast = P_SpawnMobj(player->mo->x + (P_RandomRange(PR_DECORATION,-36,36) * player->mo->scale),
		player->mo->y + (P_RandomRange(PR_DECORATION,-36,36) * player->mo->scale),
		player->mo->z + (player->mo->height/2) + (P_RandomRange(PR_DECORATION,-20,20) * player->mo->scale),
		MT_FASTLINE);

	P_SetTarget(&fast->target, player->mo);
	fast->momx = 3*player->mo->momx/4;
	fast->momy = 3*player->mo->momy/4;
	fast->momz = 3*P_GetMobjZMovement(player->mo)/4;

	fast->z += player->mo->sprzoff;

	if (top)
	{
		fast->angle = player->mo->angle;
		P_SetScale(fast, (fast->destscale =
					3 * fast->destscale / 2));

		fast->spritexscale = 3*FRACUNIT;
	}
	else
	{
		fast->angle = K_MomentumAngle(player->mo);
		if (player->ringboost)
		{
			fixed_t bunky = fast->scale;
			if (player->ringboost < 300)
				bunky /= (300 * player->ringboost);
			P_SetScale(fast, fast->scale + bunky);
		}
		if (player->tripwireLeniency)
		{
			fast->destscale = fast->destscale * 2;
			P_SetScale(fast, 3*fast->scale/2);
		}
	}

	K_MatchGenericExtraFlags(fast, player->mo);
	P_SetTarget(&fast->owner, player->mo);
	fast->renderflags |= RF_REDUCEVFX;

	if (top)
	{
		fast->color = SKINCOLOR_SUNSLAM;
		fast->colorized = true;
		fast->renderflags |= RF_ADD;
	}
	else if (player->eggmanexplode)
	{
		// Make it red when you have the eggman speed boost
		fast->color = SKINCOLOR_RED;
		fast->colorized = true;
	}
	else if (player->invincibilitytimer)
	{
		const tic_t defaultTime = itemtime+(2*TICRATE);
		if (player->invincibilitytimer > defaultTime)
		{
			fast->color = player->mo->color;
		}
		else
		{
			fast->color = SKINCOLOR_INVINCFLASH;
		}
		fast->colorized = true;
	}
	else if (player->tripwireLeniency)
	{
		// Make it pink+blue+big when you can go through tripwire
		fast->color = (leveltime & 1) ? SKINCOLOR_LILAC : SKINCOLOR_JAWZ;
		fast->colorized = true;
		fast->renderflags |= RF_ADD;
	}
	else if (player->wavedashboost)
	{
		fast->color = SKINCOLOR_WHITE;
		fast->colorized = true;
	}
	else if (player->ringboost)
	{
		UINT8 ringboostcolors[] = {SKINCOLOR_AQUAMARINE, SKINCOLOR_EMERALD, SKINCOLOR_GARDEN, SKINCOLOR_CROCODILE, SKINCOLOR_BANANA};
		UINT8 ringboostbreakpoint = min(player->ringboost / TICRATE / 6, sizeof(ringboostcolors) / sizeof(ringboostcolors[0]));
		if (ringboostbreakpoint > 0)
		{
			fast->color = ringboostcolors[ringboostbreakpoint - 1];
			fast->colorized = true;
			fast->renderflags |= RF_ADD;
		}
	}
}

void K_SpawnNormalSpeedLines(player_t *player)
{
	K_SpawnGenericSpeedLines(player, false);
}

void K_SpawnGardenTopSpeedLines(player_t *player)
{
	K_SpawnGenericSpeedLines(player, true);
}

void K_SpawnInvincibilitySpeedLines(mobj_t *mo)
{
	mobj_t *fast = P_SpawnMobjFromMobj(mo,
		P_RandomRange(PR_DECORATION, -48, 48) * FRACUNIT,
		P_RandomRange(PR_DECORATION, -48, 48) * FRACUNIT,
		P_RandomRange(PR_DECORATION, 0, 64) * FRACUNIT,
		MT_FASTLINE);
	P_SetMobjState(fast, S_KARTINVLINES1);

	P_SetTarget(&fast->target, mo);
	fast->angle = K_MomentumAngle(mo);

	fast->momx = 3*mo->momx/4;
	fast->momy = 3*mo->momy/4;
	fast->momz = 3*P_GetMobjZMovement(mo)/4;

	K_MatchGenericExtraFlags(fast, mo);
	P_SetTarget(&fast->owner, mo);
	fast->renderflags |= RF_REDUCEVFX;

	fast->color = mo->color;
	fast->colorized = true;

	if (mo->player->invincibilitytimer < 10*TICRATE)
		fast->destscale = 6*((mo->player->invincibilitytimer/TICRATE)*FRACUNIT)/8;
}

static void K_SpawnGrowShrinkParticles(mobj_t *mo, INT32 timer)
{
	const boolean shrink = (timer < 0);
	const INT32 maxTime = (10*TICRATE);
	const INT32 noTime = (2*TICRATE);
	INT32 spawnFreq = 1;

	mobj_t *particle = NULL;
	fixed_t particleScale = FRACUNIT;
	fixed_t particleSpeed = 0;

	spawnFreq = abs(timer);

	if (spawnFreq < noTime)
	{
		return;
	}

	spawnFreq -= noTime;

	if (spawnFreq > maxTime)
	{
		spawnFreq = maxTime;
	}

	spawnFreq = (maxTime - spawnFreq) / TICRATE / 4;
	if (spawnFreq == 0)
	{
		spawnFreq++;
	}

	if (leveltime % spawnFreq != 0)
	{
		return;
	}

	particle = P_SpawnMobjFromMobj(
		mo,
		P_RandomRange(PR_DECORATION, -32, 32) * FRACUNIT,
		P_RandomRange(PR_DECORATION, -32, 32) * FRACUNIT,
		(P_RandomRange(PR_DECORATION, 0, 24) + (shrink ? 48 : 0)) * FRACUNIT,
		MT_GROW_PARTICLE
	);

	P_SetTarget(&particle->target, mo);

	particle->momx = mo->momx;
	particle->momy = mo->momy;
	particle->momz = P_GetMobjZMovement(mo);

	K_MatchGenericExtraFlags(particle, mo);

	particleScale = FixedMul((shrink ? SHRINK_PHYSICS_SCALE : GROW_PHYSICS_SCALE), mapobjectscale);
	particleSpeed = mo->scale * 4 * P_MobjFlip(mo); // NOT particleScale

	particle->destscale = particleScale;
	P_SetScale(particle, particle->destscale);

	if (shrink == true)
	{
		particle->color = SKINCOLOR_KETCHUP;
		particle->momz -= particleSpeed;
		particle->renderflags |= RF_VERTICALFLIP;
	}
	else
	{
		particle->color = SKINCOLOR_SAPPHIRE;
		particle->momz += particleSpeed;
	}
}

void K_SpawnBumpEffect(mobj_t *mo)
{
	mobj_t *top = mo->player ? K_GetGardenTop(mo->player) : NULL;

	mobj_t *fx = P_SpawnMobj(mo->x, mo->y, mo->z, MT_BUMP);

	if (mo->eflags & MFE_VERTICALFLIP)
		fx->eflags |= MFE_VERTICALFLIP;
	else
		fx->eflags &= ~MFE_VERTICALFLIP;

	fx->scale = mo->scale;

	if (top)
		S_StartSound(mo, top->info->attacksound);
	else
		S_StartSound(mo, sfx_s3k49);
}

void K_SpawnMagicianParticles(mobj_t *mo, int spread)
{
	INT32 i;
	mobj_t *target = mo->target;

	if (!target || P_MobjWasRemoved(target))
		target = mo;

	for (i = 0; i < 16; i++)
	{
		fixed_t hmomentum = P_RandomRange(PR_DECORATION, spread * -1, spread) * mo->scale;
		fixed_t vmomentum = P_RandomRange(PR_DECORATION, spread * -1, spread) * mo->scale;
		UINT16 color = P_RandomKey(PR_DECORATION, numskincolors);

		fixed_t ang = FixedAngle(P_RandomRange(PR_DECORATION, 0, 359)*FRACUNIT);
		SINT8 flip = 1;

		mobj_t *dust;

		if (i & 1)
			ang -= ANGLE_90;
		else
			ang += ANGLE_90;

		// sprzoff for Garden Top!!
		dust = P_SpawnMobjFromMobjUnscaled(mo,
			FixedMul(mo->radius / 4, FINECOSINE(ang >> ANGLETOFINESHIFT)),
			FixedMul(mo->radius / 4, FINESINE(ang >> ANGLETOFINESHIFT)),
			(target->height / 4) + target->sprzoff, (i%3 == 0) ? MT_SIGNSPARKLE : MT_SPINDASHDUST
		);
		flip = P_MobjFlip(dust);

		dust->momx = target->momx + FixedMul(hmomentum, FINECOSINE(ang >> ANGLETOFINESHIFT));
		dust->momy = target->momy + FixedMul(hmomentum, FINESINE(ang >> ANGLETOFINESHIFT));
		dust->momz = vmomentum * flip;
		dust->scale = dust->scale*4;
		dust->frame |= FF_SUBTRACT|FF_TRANS90;
		dust->color = color;
		dust->colorized = true;
		dust->hitlag = 0;
	}
}

static SINT8 K_GlanceAtPlayers(player_t *glancePlayer, boolean horn)
{
	const fixed_t maxdistance = FixedMul(1280 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));
	const angle_t blindSpotSize = ANG10; // ANG5
	SINT8 glanceDir = 0;
	SINT8 lastValidGlance = 0;
	const boolean podiumspecial = (K_PodiumSequence() == true && glancePlayer->nextwaypoint == NULL && glancePlayer->speed == 0);
	boolean mysticmelodyspecial = false;

	if (podiumspecial)
	{
		if (glancePlayer->position > 3)
		{
			// Loser valley, focused on the mountain.
			return 0;
		}

		if (glancePlayer->position == 1)
		{
			// Sitting on the stand, I ammm thebest!
			return 0;
		}
	}

	// See if there's any players coming up behind us.
	// If so, your character will glance at 'em.
	mobj_t *victim = NULL, *victimnext = NULL;

	for (victim = trackercap; victim; victim = victimnext)
	{
		player_t *p = victim->player;
		angle_t back;
		angle_t diff;
		fixed_t distance;
		SINT8 dir = -1;

		victimnext = victim->itnext;

		if (p != NULL)
		{
			if (p == glancePlayer)
			{
				// FOOL! Don't glance at yerself!
				continue;
			}

			if (p->spectator || p->hyudorotimer > 0)
			{
				// Not playing / invisible
				continue;
			}

			if (podiumspecial && p->position >= glancePlayer->position)
			{
				// On the podium, only look with envy, not condesencion
				continue;
			}
		}
		else if (victim->type != MT_ANCIENTSHRINE)
		{
			// Ancient Shrines are a special exception to glance logic.
			continue;
		}

		if (!podiumspecial)
		{
			distance = R_PointToDist2(glancePlayer->mo->x, glancePlayer->mo->y, victim->x, victim->y);
			distance = R_PointToDist2(0, glancePlayer->mo->z, distance, victim->z);

			if (distance > maxdistance)
			{
				// Too far away
				continue;
			}
		}

		back = glancePlayer->mo->angle + ANGLE_180;
		diff = R_PointToAngle2(glancePlayer->mo->x, glancePlayer->mo->y, victim->x, victim->y) - back;

		if (diff > ANGLE_180)
		{
			diff = InvAngle(diff);
			dir = -dir;
		}

		if (diff > (podiumspecial ? (ANGLE_180 - blindSpotSize) : ANGLE_90))
		{
			// Not behind the player
			continue;
		}

		if (diff < blindSpotSize)
		{
			// Small blindspot directly behind your back, gives the impression of smoothly turning.
			continue;
		}

		if (!podiumspecial && P_CheckSight(glancePlayer->mo, victim) == false)
		{
			// Blocked by a wall, we can't glance at 'em!
			continue;
		}

		// Adds, so that if there's more targets on one of your sides, it'll glance on that side.
		glanceDir += dir;

		// That poses a limitation if there's an equal number of targets on both sides...
		// In that case, we'll pick the last chosen glance direction.
		lastValidGlance = dir;

		if (horn == true)
		{
			if (p != NULL)
			{
				K_FollowerHornTaunt(glancePlayer, p, false);
			}
			else if (victim->type == MT_ANCIENTSHRINE)
			{
				mysticmelodyspecial = true;
			}
		}
	}

	if (horn == true && lastValidGlance != 0)
	{
		const boolean tasteful = (glancePlayer->karthud[khud_taunthorns] == 0);

		K_FollowerHornTaunt(glancePlayer, glancePlayer, mysticmelodyspecial);

		if (tasteful && glancePlayer->karthud[khud_taunthorns] < 2*TICRATE)
			glancePlayer->karthud[khud_taunthorns] = 2*TICRATE;
	}

	if (glanceDir > 0)
	{
		return 1;
	}
	else if (glanceDir < 0)
	{
		return -1;
	}

	return lastValidGlance;
}

/**	\brief Handles the state changing for moving players, moved here to eliminate duplicate code

	\param	player	player data

	\return	void
*/
void K_KartMoveAnimation(player_t *player)
{
	const INT16 minturn = KART_FULLTURN/8;

	const fixed_t fastspeed = (K_GetKartSpeed(player, false, true) * 17) / 20; // 85%
	const fixed_t speedthreshold = player->mo->scale / 8;

	const boolean onground = P_IsObjectOnGround(player->mo);

	UINT16 buttons = K_GetKartButtons(player);
	const boolean spinningwheels = (((buttons & BT_ACCELERATE) == BT_ACCELERATE) || (onground && player->speed > 0));
	const boolean lookback = ((buttons & BT_LOOKBACK) == BT_LOOKBACK);

	SINT8 turndir = 0;
	SINT8 destGlanceDir = 0;
	SINT8 drift = player->drift;

	if (!lookback)
	{
		player->pflags &= ~PF_GAINAX;

		// Uses turning over steering -- it's important to show player feedback immediately.
		if (player->cmd.turning < -minturn)
		{
			turndir = -1;
		}
		else if (player->cmd.turning > minturn)
		{
			turndir = 1;
		}
	}
	else if (drift == 0)
	{
		// Prioritize looking back frames over turning
		turndir = 0;
	}

	// Sliptides: drift -> lookback frames
	if (abs(player->aizdriftturn) >= ANGLE_90)
	{
		destGlanceDir = -(2*intsign(player->aizdriftturn));
		player->glanceDir = destGlanceDir;
		drift = turndir = 0;
		player->pflags &= ~PF_GAINAX;
	}
	else if (player->aizdriftturn)
	{
		drift = intsign(player->aizdriftturn);
		turndir = 0;
	}
	else if (player->curshield == KSHIELD_TOP)
	{
		drift = -turndir;
	}
	else if (turndir == 0 && drift == 0)
	{
		// Only try glancing if you're driving straight.
		// This avoids all-players loops when we don't need it.
		const boolean horn = lookback && !(player->pflags & PF_GAINAX);
		destGlanceDir = K_GlanceAtPlayers(player, horn);

		if (lookback == true)
		{
			statenum_t gainaxstate = S_GAINAX_TINY;
			if (destGlanceDir == 0)
			{
				if (player->glanceDir != 0)
				{
					// Keep to the side you were already on.
					if (player->glanceDir < 0)
					{
						destGlanceDir = -1;
					}
					else
					{
						destGlanceDir = 1;
					}
				}
				else
				{
					// Look to your right by default
					destGlanceDir = -1;
				}
			}
			else
			{
				// Looking back AND glancing? Amplify the look!
				destGlanceDir *= 2;
				if (player->itemamount && player->itemtype)
					gainaxstate = S_GAINAX_HUGE;
				else
					gainaxstate = S_GAINAX_MID1;
			}

			if (destGlanceDir && !(player->pflags & PF_GAINAX))
			{
				mobj_t *gainax = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_GAINAX);
				gainax->movedir = (destGlanceDir < 0) ? (ANGLE_270-ANG10) : (ANGLE_90+ANG10);
				P_SetTarget(&gainax->target, player->mo);
				P_SetMobjState(gainax, gainaxstate);
				gainax->flags2 |= MF2_AMBUSH;
				player->pflags |= PF_GAINAX;
			}
		}
		else if (K_GetForwardMove(player) < 0 && destGlanceDir == 0)
		{
			// Reversing -- like looking back, but doesn't stack on the other glances.
			if (player->glanceDir != 0)
			{
				// Keep to the side you were already on.
				if (player->glanceDir < 0)
				{
					destGlanceDir = -1;
				}
				else
				{
					destGlanceDir = 1;
				}
			}
			else
			{
				// Look to your right by default
				destGlanceDir = -1;
			}
		}
	}
	else
	{
		// Not glancing
		destGlanceDir = 0;
		player->glanceDir = 0;
	}

#define SetState(sn) \
	if (player->mo->state != &states[sn]) \
		P_SetPlayerMobjState(player->mo, sn)

	if (onground == false)
	{
		// Only use certain frames in the air, to make it look like your tires are spinning fruitlessly!

		if (drift > 0)
		{
			// Neutral drift
			SetState(S_KART_DRIFT_L);
		}
		else if (drift < 0)
		{
			// Neutral drift
			SetState(S_KART_DRIFT_R);
		}
		else
		{
			if (turndir == -1)
			{
				SetState(S_KART_FAST_R);
			}
			else if (turndir == 1)
			{
				SetState(S_KART_FAST_L);
			}
			else
			{
				switch (player->glanceDir)
				{
					case -2:
						SetState(S_KART_FAST_LOOK_R);
						break;
					case 2:
						SetState(S_KART_FAST_LOOK_L);
						break;
					case -1:
						SetState(S_KART_FAST_GLANCE_R);
						break;
					case 1:
						SetState(S_KART_FAST_GLANCE_L);
						break;
					default:
						SetState(S_KART_FAST);
						break;
				}
			}
		}

		if (!spinningwheels)
		{
			// TODO: The "tires still in the air" states should have it's own SPR2s.
			// This was a quick hack to get the same functionality with less work,
			// but it's really dunderheaded & isn't customizable at all.
			player->mo->frame = (player->mo->frame & ~FF_FRAMEMASK);
			player->mo->tics++; // Makes it properly use frame 0
		}
	}
	else
	{
		if (drift > 0)
		{
			// Drifting LEFT!

			if (turndir == -1)
			{
				// Right -- outwards drift
				SetState(S_KART_DRIFT_L_OUT);
			}
			else if (turndir == 1)
			{
				// Left -- inwards drift
				SetState(S_KART_DRIFT_L_IN);
			}
			else
			{
				// Neutral drift
				SetState(S_KART_DRIFT_L);
			}
		}
		else if (drift < 0)
		{
			// Drifting RIGHT!

			if (turndir == -1)
			{
				// Right -- inwards drift
				SetState(S_KART_DRIFT_R_IN);
			}
			else if (turndir == 1)
			{
				// Left -- outwards drift
				SetState(S_KART_DRIFT_R_OUT);
			}
			else
			{
				// Neutral drift
				SetState(S_KART_DRIFT_R);
			}
		}
		else
		{
			if (player->speed >= fastspeed && player->speed >= (player->lastspeed - speedthreshold))
			{
				// Going REAL fast!

				if (turndir == -1)
				{
					SetState(S_KART_FAST_R);
				}
				else if (turndir == 1)
				{
					SetState(S_KART_FAST_L);
				}
				else
				{
					switch (player->glanceDir)
					{
						case -2:
							SetState(S_KART_FAST_LOOK_R);
							break;
						case 2:
							SetState(S_KART_FAST_LOOK_L);
							break;
						case -1:
							SetState(S_KART_FAST_GLANCE_R);
							break;
						case 1:
							SetState(S_KART_FAST_GLANCE_L);
							break;
						default:
							SetState(S_KART_FAST);
							break;
					}
				}
			}
			else
			{
				if (spinningwheels)
				{
					// Drivin' slow.

					if (turndir == -1)
					{
						SetState(S_KART_SLOW_R);
					}
					else if (turndir == 1)
					{
						SetState(S_KART_SLOW_L);
					}
					else
					{
						switch (player->glanceDir)
						{
							case -2:
								SetState(S_KART_SLOW_LOOK_R);
								break;
							case 2:
								SetState(S_KART_SLOW_LOOK_L);
								break;
							case -1:
								SetState(S_KART_SLOW_GLANCE_R);
								break;
							case 1:
								SetState(S_KART_SLOW_GLANCE_L);
								break;
							default:
								SetState(S_KART_SLOW);
								break;
						}
					}
				}
				else
				{
					// Completely still.

					if (turndir == -1)
					{
						SetState(S_KART_STILL_R);
					}
					else if (turndir == 1)
					{
						SetState(S_KART_STILL_L);
					}
					else
					{
						switch (player->glanceDir)
						{
							case -2:
								SetState(S_KART_STILL_LOOK_R);
								break;
							case 2:
								SetState(S_KART_STILL_LOOK_L);
								break;
							case -1:
								SetState(S_KART_STILL_GLANCE_R);
								break;
							case 1:
								SetState(S_KART_STILL_GLANCE_L);
								break;
							default:
								SetState(S_KART_STILL);
								break;
						}
					}
				}
			}
		}
	}

#undef SetState

	// Update your glance value to smooth it out.
	if (player->glanceDir > destGlanceDir)
	{
		player->glanceDir--;
	}
	else if (player->glanceDir < destGlanceDir)
	{
		player->glanceDir++;
	}

	if (!player->glanceDir)
		player->pflags &= ~PF_GAINAX;

	// Update lastspeed value -- we use to display slow driving frames instead of fast driving when slowing down.
	player->lastspeed = player->speed;
}

static void K_TauntVoiceTimers(player_t *player)
{
	if (!player)
		return;

	player->karthud[khud_tauntvoices] = 6*TICRATE;
	player->karthud[khud_voices] = 4*TICRATE;
}

static void K_RegularVoiceTimers(player_t *player)
{
	if (!player)
		return;

	player->karthud[khud_voices] = 4*TICRATE;

	if (player->karthud[khud_tauntvoices] < 4*TICRATE)
		player->karthud[khud_tauntvoices] = 4*TICRATE;
}

static UINT8 K_ObjectToSkinIDForSounds(mobj_t *source)
{
	if (source->player)
		return source->player->skin;

	if (!source->skin)
		return MAXSKINS;

	return ((skin_t *)source->skin)-skins;
}

static void K_PlayGenericTastefulTaunt(mobj_t *source, sfxenum_t sfx_id)
{
	UINT8 skinid = K_ObjectToSkinIDForSounds(source);
	if (skinid >= numskins)
		return;

	boolean tasteful = (!source->player || !source->player->karthud[khud_tauntvoices]);

	if (
		(
			(cv_kartvoices.value && tasteful)
			|| cv_tastelesstaunts.value
		)
		&& R_CanShowSkinInDemo(skinid)
	)
	{
		S_StartSound(source, sfx_id);
	}

	if (!tasteful)
		return;

	K_TauntVoiceTimers(source->player);
}

void K_PlayAttackTaunt(mobj_t *source)
{
	// Gotta roll the RNG every time this is called for sync reasons
	sfxenum_t pick = P_RandomKey(PR_VOICES, 2);
	K_PlayGenericTastefulTaunt(source, sfx_kattk1+pick);
}

void K_PlayBoostTaunt(mobj_t *source)
{
	// Gotta roll the RNG every time this is called for sync reasons
	sfxenum_t pick = P_RandomKey(PR_VOICES, 2);
	K_PlayGenericTastefulTaunt(source, sfx_kbost1+pick);
}

void K_PlayOvertakeSound(mobj_t *source)
{
	UINT8 skinid = K_ObjectToSkinIDForSounds(source);
	if (skinid >= numskins)
		return;

	boolean tasteful = (!source->player || !source->player->karthud[khud_voices]);

	if (
		(
			(cv_kartvoices.value && tasteful)
			|| cv_tastelesstaunts.value
		)
		&& R_CanShowSkinInDemo(skinid)
	)
	{
		S_StartSound(source, sfx_kslow);
	}

	if (!tasteful)
		return;

	K_RegularVoiceTimers(source->player);
}

static void K_PlayGenericCombatSound(mobj_t *source, mobj_t *other, sfxenum_t sfx_id)
{
	UINT8 skinid = K_ObjectToSkinIDForSounds(source);
	if (skinid >= numskins)
		return;

	boolean alwaysHear = false;

	if (other != NULL && P_MobjWasRemoved(other) == false && other->player != NULL)
	{
		alwaysHear = P_IsDisplayPlayer(other->player);
	}

	if (
		(cv_kartvoices.value || cv_tastelesstaunts.value)
		&& R_CanShowSkinInDemo(skinid)
	)
	{
		S_StartSound(
			alwaysHear ? NULL : source,
			skins[skinid].soundsid[S_sfx[sfx_id].skinsound]
		);
	}

	K_RegularVoiceTimers(source->player);
}

void K_PlayPainSound(mobj_t *source, mobj_t *other)
{
	sfxenum_t pick = P_RandomKey(PR_VOICES, 2); // Gotta roll the RNG every time this is called for sync reasons
	K_PlayGenericCombatSound(source, other, sfx_khurt1 + pick);
}

void K_PlayHitEmSound(mobj_t *source, mobj_t *other)
{
	K_PlayGenericCombatSound(source, other, sfx_khitem);
}

void K_TryHurtSoundExchange(mobj_t *victim, mobj_t *attacker)
{
	if (victim == NULL || P_MobjWasRemoved(victim) == true || victim->player == NULL)
	{
		return;
	}

	// In a perfect world we could move this here, but there's
	// a few niche situations where we want a pain sound from
	// the victim, but no confirm sound from the attacker.
	// (ex: DMG_STING)

	//K_PlayPainSound(victim, attacker);

	if (attacker == NULL || P_MobjWasRemoved(attacker) == true || attacker->player == NULL)
	{
		return;
	}

	attacker->player->confirmVictim = (victim->player - players);
	attacker->player->confirmVictimDelay = TICRATE/2;

	const INT32 followerskin = K_GetEffectiveFollowerSkin(attacker->player);
	if (attacker->player->follower != NULL
		&& followerskin >= 0
		&& followerskin < numfollowers)
	{
		const follower_t *fl = &followers[followerskin];
		attacker->player->follower->movecount = fl->hitconfirmtime; // movecount is used to play the hitconfirm animation for followers.
	}
}

void K_PlayPowerGloatSound(mobj_t *source)
{
	UINT8 skinid = K_ObjectToSkinIDForSounds(source);
	if (skinid >= numskins)
		return;

	if (
		(cv_kartvoices.value || cv_tastelesstaunts.value)
		&& R_CanShowSkinInDemo(skinid)
	)
	{
		S_StartSound(source, sfx_kgloat);
	}

	K_RegularVoiceTimers(source->player);
}

// MOVED so we don't have to extern K_ObjectToSkinID
void P_PlayVictorySound(mobj_t *source)
{
	UINT8 skinid = K_ObjectToSkinIDForSounds(source);
	if (skinid >= numskins)
		return;

	if (
		(cv_kartvoices.value || cv_tastelesstaunts.value)
		&& R_CanShowSkinInDemo(skinid)
	)
	{
		S_StartSound(source, sfx_kwin);
	}
}

static void K_HandleDelayedHitByEm(player_t *player)
{
	if (player->confirmVictimDelay == 0)
	{
		return;
	}

	player->confirmVictimDelay--;

	if (player->confirmVictimDelay == 0)
	{
		mobj_t *victim = NULL;

		if (player->confirmVictim < MAXPLAYERS && playeringame[player->confirmVictim])
		{
			player_t *victimPlayer = &players[player->confirmVictim];

			if (victimPlayer != NULL && victimPlayer->spectator == false)
			{
				victim = victimPlayer->mo;
			}
		}

		K_PlayHitEmSound(player->mo, victim);
	}
}

void K_MomentumToFacing(player_t *player)
{
	angle_t dangle = player->mo->angle - K_MomentumAngleReal(player->mo);

	if (dangle > ANGLE_180)
		dangle = InvAngle(dangle);

	// If you aren't on the ground or are moving in too different of a direction don't do this
	if (player->mo->eflags & MFE_JUSTHITFLOOR)
		; // Just hit floor ALWAYS redirects
	else if (!P_IsObjectOnGround(player->mo) || dangle > ANGLE_90)
		return;

	P_Thrust(player->mo, player->mo->angle, player->speed - FixedMul(player->speed, player->mo->friction));
	player->mo->momx = FixedMul(player->mo->momx - player->cmomx, player->mo->friction) + player->cmomx;
	player->mo->momy = FixedMul(player->mo->momy - player->cmomy, player->mo->friction) + player->cmomy;
}

boolean K_ApplyOffroad(const player_t *player)
{
	if (player->invincibilitytimer || player->hyudorotimer || player->sneakertimer)
		return false;
	if (K_IsRidingFloatingTop(player))
		return false;
	return true;
}

boolean K_SlopeResistance(const player_t *player)
{
	if (player->invincibilitytimer || player->sneakertimer || player->tiregrease || player->flamedash)
		return true;
	if (player->curshield == KSHIELD_TOP)
		return true;
	return false;
}

fixed_t K_PlayerTripwireSpeedThreshold(const player_t *player)
{
	fixed_t required_speed = 2 * K_GetKartSpeed(player, false, false); // 200%

	if (player->offroad && K_ApplyOffroad(player))
	{
		// Increase to 300% if you're lawnmowering.
		required_speed = (required_speed * 3) / 2;
	}

	if (player->botvars.rubberband > FRACUNIT && K_PlayerUsesBotMovement(player) == true)
	{
		// Make it harder for bots to do this when rubberbanding.

		// This is actually biased really hard against the bot,
		// because the bot rubberbanding speed increase is
		// decreased with other boosts.

		required_speed = FixedMul(required_speed, player->botvars.rubberband);
	}

	return required_speed;
}

tripwirepass_t K_TripwirePassConditions(const player_t *player)
{
	if (
			player->invincibilitytimer ||
			player->sneakertimer
		)
		return TRIPWIRE_BLASTER;

	if (
			player->flamedash ||
			((player->speed > K_PlayerTripwireSpeedThreshold(player)) && player->tripwireReboundDelay == 0) ||
			player->fakeBoost
	)
		return TRIPWIRE_BOOST;

	if (
			player->growshrinktimer > 0 ||
			player->hyudorotimer
		)
		return TRIPWIRE_IGNORE;

	// TRIPWIRE_CONSUME should always be checked last; this category should be
	// used for tripwire states that are partially detrimental, and check
	// leniency from OTHER states, not themselves.
	if (player->curshield == KSHIELD_TOP)
		return TRIPWIRE_CONSUME;

	return TRIPWIRE_NONE;
}

boolean K_TripwirePass(const player_t *player)
{
	return (player->tripwirePass != TRIPWIRE_NONE);
}

boolean K_MovingHorizontally(mobj_t *mobj)
{
	return (P_AproxDistance(mobj->momx, mobj->momy) / 4 > abs(mobj->momz));
}

boolean K_WaterRun(mobj_t *mobj)
{
	switch (mobj->type)
	{
		case MT_ORBINAUT:
		case MT_GACHABOM:
		{
			if (Obj_OrbinautCanRunOnWater(mobj))
			{
				return true;
			}

			return false;
		}

		case MT_JAWZ:
		{
			if (mobj->tracer != NULL && P_MobjWasRemoved(mobj->tracer) == false)
			{
				fixed_t jawzFeet = P_GetMobjFeet(mobj);
				fixed_t chaseFeet = P_GetMobjFeet(mobj->tracer);
				fixed_t footDiff = (chaseFeet - jawzFeet) * P_MobjFlip(mobj);

				// Water run if the player we're chasing is above/equal to us.
				// Start water skipping if they're underneath the water.
				return (footDiff > -mobj->tracer->height);
			}

			return false;
		}

		case MT_PLAYER:
		{
			fixed_t minspeed = 0;

			if (mobj->player == NULL)
			{
				return false;
			}

			if (mobj->player->curshield == KSHIELD_TOP)
			{
				return K_IsHoldingDownTop(mobj->player) == false;
			}

			minspeed = K_PlayerTripwireSpeedThreshold(mobj->player);

			if (mobj->player->speed < minspeed / 5) // 40%
			{
				return false;
			}

			if (mobj->player->invincibilitytimer
				|| mobj->player->sneakertimer
				|| mobj->player->tiregrease
				|| mobj->player->flamedash
				|| mobj->player->speed > minspeed)
			{
				return true;
			}

			return false;
		}

		default:
		{
			return false;
		}
	}
}

boolean K_WaterSkip(mobj_t *mobj)
{
	if (mobj->waterskip >= 2)
	{
		// Already finished waterskipping.
		return false;
	}

	switch (mobj->type)
	{
		case MT_PLAYER:
		{
			if (mobj->player != NULL && mobj->player->curshield == KSHIELD_TOP)
			{
				// Don't allow
				return false;
			}

			if (K_PlayerEBrake(mobj->player))
			{
				return false;
			}

			// Allow
			break;
		}

		case MT_ORBINAUT:
		case MT_JAWZ:
		case MT_BALLHOG:
		{
			// Allow
			break;
		}

		default:
		{
			// Don't allow
			return false;
		}
	}

	if (mobj->waterskip > 0)
	{
		// Already waterskipping.
		// Simply make sure you haven't slowed down drastically.
		return (P_AproxDistance(mobj->momx, mobj->momy) > 20 * mapobjectscale);
	}
	else
	{
		// Need to be moving horizontally and not vertically
		// to be able to start a water skip.
		return K_MovingHorizontally(mobj);
	}
}

void K_SpawnWaterRunParticles(mobj_t *mobj)
{
	fixed_t runSpeed = 14 * mobj->scale;
	fixed_t curSpeed = INT32_MAX;
	fixed_t topSpeed = INT32_MAX;
	fixed_t trailScale = FRACUNIT;

	if (mobj->momz != 0)
	{
		// Only while touching ground.
		return;
	}

	if (mobj->watertop == INT32_MAX || mobj->waterbottom == INT32_MIN)
	{
		// Invalid water plane.
		return;
	}

	if (mobj->player != NULL)
	{
		if (mobj->player->spectator)
		{
			// Not as spectator.
			return;
		}

		if (mobj->player->carry == CR_SLIDING)
		{
			// Not in water slides.
			return;
		}

		topSpeed = K_GetKartSpeed(mobj->player, false, false);
		runSpeed = FixedMul(runSpeed, mobj->movefactor);
	}
	else
	{
		topSpeed = FixedMul(mobj->scale, K_GetKartSpeedFromStat(5));
	}

	curSpeed = P_AproxDistance(mobj->momx, mobj->momy);

	if (curSpeed <= runSpeed)
	{
		// Not fast enough.
		return;
	}

	// Near the water plane.
	if ((!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z + mobj->height >= mobj->watertop && mobj->z <= mobj->watertop)
		|| (mobj->eflags & MFE_VERTICALFLIP && mobj->z + mobj->height >= mobj->waterbottom && mobj->z <= mobj->waterbottom))
	{
		if (topSpeed > runSpeed)
		{
			trailScale = FixedMul(FixedDiv(curSpeed - runSpeed, topSpeed - runSpeed), mapobjectscale);
		}
		else
		{
			trailScale = mapobjectscale; // Scaling is based off difference between runspeed and top speed
		}

		if (trailScale > 0)
		{
			const angle_t forwardangle = K_MomentumAngle(mobj);
			const fixed_t playerVisualRadius = mobj->radius + (8 * mobj->scale);
			const size_t numFrames = S_WATERTRAIL8 - S_WATERTRAIL1;
			const statenum_t curOverlayFrame = S_WATERTRAIL1 + (leveltime % numFrames);
			const statenum_t curUnderlayFrame = S_WATERTRAILUNDERLAY1 + (leveltime % numFrames);
			fixed_t x1, x2, y1, y2;
			mobj_t *water;

			x1 = mobj->x + mobj->momx + P_ReturnThrustX(mobj, forwardangle + ANGLE_90, playerVisualRadius);
			y1 = mobj->y + mobj->momy + P_ReturnThrustY(mobj, forwardangle + ANGLE_90, playerVisualRadius);
			x1 = x1 + P_ReturnThrustX(mobj, forwardangle, playerVisualRadius);
			y1 = y1 + P_ReturnThrustY(mobj, forwardangle, playerVisualRadius);

			x2 = mobj->x + mobj->momx + P_ReturnThrustX(mobj, forwardangle - ANGLE_90, playerVisualRadius);
			y2 = mobj->y + mobj->momy + P_ReturnThrustY(mobj, forwardangle - ANGLE_90, playerVisualRadius);
			x2 = x2 + P_ReturnThrustX(mobj, forwardangle, playerVisualRadius);
			y2 = y2 + P_ReturnThrustY(mobj, forwardangle, playerVisualRadius);

			// Left
			// underlay
			water = P_SpawnMobj(x1, y1,
				((mobj->eflags & MFE_VERTICALFLIP) ? mobj->waterbottom - FixedMul(mobjinfo[MT_WATERTRAILUNDERLAY].height, mobj->scale) : mobj->watertop), MT_WATERTRAILUNDERLAY);
			water->angle = forwardangle - ANGLE_180 - ANGLE_22h;
			water->destscale = trailScale;
			water->momx = mobj->momx;
			water->momy = mobj->momy;
			water->momz = mobj->momz;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curUnderlayFrame);
			P_SetTarget(&water->owner, mobj);
			water->renderflags |= RF_REDUCEVFX;

			// overlay
			water = P_SpawnMobj(x1, y1,
				((mobj->eflags & MFE_VERTICALFLIP) ? mobj->waterbottom - FixedMul(mobjinfo[MT_WATERTRAIL].height, mobj->scale) : mobj->watertop), MT_WATERTRAIL);
			water->angle = forwardangle - ANGLE_180 - ANGLE_22h;
			water->destscale = trailScale;
			water->momx = mobj->momx;
			water->momy = mobj->momy;
			water->momz = mobj->momz;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curOverlayFrame);
			P_SetTarget(&water->owner, mobj);
			water->renderflags |= RF_REDUCEVFX;

			// Right
			// Underlay
			water = P_SpawnMobj(x2, y2,
				((mobj->eflags & MFE_VERTICALFLIP) ? mobj->waterbottom - FixedMul(mobjinfo[MT_WATERTRAILUNDERLAY].height, mobj->scale) : mobj->watertop), MT_WATERTRAILUNDERLAY);
			water->angle = forwardangle - ANGLE_180 + ANGLE_22h;
			water->destscale = trailScale;
			water->momx = mobj->momx;
			water->momy = mobj->momy;
			water->momz = mobj->momz;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curUnderlayFrame);
			P_SetTarget(&water->owner, mobj);
			water->renderflags |= RF_REDUCEVFX;

			// Overlay
			water = P_SpawnMobj(x2, y2,
				((mobj->eflags & MFE_VERTICALFLIP) ? mobj->waterbottom - FixedMul(mobjinfo[MT_WATERTRAIL].height, mobj->scale) : mobj->watertop), MT_WATERTRAIL);
			water->angle = forwardangle - ANGLE_180 + ANGLE_22h;
			water->destscale = trailScale;
			water->momx = mobj->momx;
			water->momy = mobj->momy;
			water->momz = mobj->momz;
			P_SetScale(water, trailScale);
			P_SetMobjState(water, curOverlayFrame);
			P_SetTarget(&water->owner, mobj);
			water->renderflags |= RF_REDUCEVFX;

			if (!S_SoundPlaying(mobj, sfx_s3kdbs))
			{
				const INT32 volume = (min(trailScale, FRACUNIT) * 255) / FRACUNIT;
				S_ReducedVFXSoundAtVolume(mobj, sfx_s3kdbs, volume, mobj->player);
			}
		}

		// Little water sound while touching water - just a nicety.
		if ((mobj->eflags & MFE_TOUCHWATER) && !(mobj->eflags & MFE_UNDERWATER))
		{
			if (P_RandomChance(PR_BUBBLE, FRACUNIT/2) && leveltime % TICRATE == 0)
			{
				S_ReducedVFXSound(mobj, sfx_floush, mobj->player);
			}
		}
	}
}

boolean K_IsRidingFloatingTop(const player_t *player)
{
	if (player->curshield != KSHIELD_TOP)
	{
		return false;
	}

	return !Obj_GardenTopPlayerIsGrinding(player);
}

boolean K_IsHoldingDownTop(const player_t *player)
{
	if (player->curshield != KSHIELD_TOP)
	{
		return false;
	}

	if ((K_GetKartButtons(player) & BT_DRIFT) != BT_DRIFT)
	{
		return false;
	}

	return true;
}

mobj_t *K_GetGardenTop(const player_t *player)
{
	if (player->curshield != KSHIELD_TOP)
	{
		return NULL;
	}

	if (player->mo == NULL)
	{
		return NULL;
	}

	return player->mo->hnext;
}

static fixed_t K_FlameShieldDashVar(INT32 val)
{
	// 1 second = 75% + 50% top speed
	return (3*FRACUNIT/4) + (((val * FRACUNIT) / TICRATE));
}

INT16 K_GetSpindashChargeTime(const player_t *player)
{
	// more charge time for higher speed
	// Tails = 1.7s, Knuckles = 2.2s, Metal = 2.7s
	return ((player->kartspeed + 8) * TICRATE) / 6;
}

fixed_t K_GetSpindashChargeSpeed(const player_t *player)
{
	// more speed for higher weight & speed
	// Tails = +16.94%, Fang = +34.94%, Mighty = +34.94%, Metal = +43.61%
	// (can be higher than this value when overcharged)

	// The above comment is now strictly incorrect and I can't be assed to do the math properly.
	// 2.2 introduces a power fudge to compensate for the removal of spindash overcharge. -Tyron
	fixed_t val = (10*FRACUNIT/277) + (((player->kartspeed + player->kartweight) + 2) * FRACUNIT) / 45;

	// 2.2 - Improved Spindash
	if (!G_CompatLevel(0x000A))
	{
		if (gametyperules & GTR_CIRCUIT)
			val = 5 * val / 4;
	}

	// Old behavior before desperation spindash
	// return (gametyperules & GTR_CLOSERPLAYERS) ? (4 * val) : val;
	return val;
}

static fixed_t K_RingDurationBoost(const player_t *player)
{
	fixed_t ret = FRACUNIT;

	if (K_PlayerUsesBotMovement(player))
	{
		// x2.0 for Lv. 9
		const fixed_t modifier = K_BotMapModifier();
		fixed_t add = ((player->botvars.difficulty-1) * modifier) / (DIFFICULTBOT-1);

		ret += add;

		if (player->botvars.rival == true || cv_levelskull.value)
		{
			// x2.0 for Rival
			ret *= 2;
		}
	}

	return ret;
}

// v2 almost broke sliptiding when it fixed turning bugs!
// This value is fine-tuned to feel like v1 again without reverting any of those changes.
#define SLIPTIDEHANDLING 7*FRACUNIT/8

// sets boostpower, speedboost, accelboost, and handleboost to whatever we need it to be
static void K_GetKartBoostPower(player_t *player)
{
	// Light weights have stronger boost stacking -- aka, better metabolism than heavies XD
	const fixed_t maxmetabolismincrease = FRACUNIT/2;
	const fixed_t metabolism = FRACUNIT - ((9-player->kartweight) * maxmetabolismincrease / 8);

	fixed_t boostpower = FRACUNIT;
	fixed_t speedboost = 0, accelboost = 0, handleboost = 0;
	UINT8 numboosts = 0;

	if (player->spinouttimer && player->wipeoutslow == 1) // Slow down after you've been bumped
	{
		player->boostpower = player->speedboost = player->accelboost = 0;
		return;
	}

	// Offroad is separate, it's difficult to factor it in with a variable value anyway.
	if (K_ApplyOffroad(player) && player->offroad >= 0)
		boostpower = FixedDiv(boostpower, FixedMul(player->offroad, K_GetKartGameSpeedScalar(gamespeed)) + FRACUNIT);

	if (player->bananadrag > TICRATE)
		boostpower = (4*boostpower)/5;

	// Note: Handling will ONLY stack when sliptiding!
	// > (NB 2023-03-06: This was previously unintentionally applied while drifting as well.)
	// > (This only affected drifts where you were under the effect of multiple handling boosts.)
	// > (Revisit if Growvinciblity or sneaker-panels + power items feel too heavy while drifting!)
	// When you're not, it just uses the best instead of adding together, like the old behavior.
#define ADDBOOST(s,a,h) { \
	numboosts++; \
	speedboost += FixedDiv(s, FRACUNIT + (metabolism * (numboosts-1))); \
	accelboost += FixedDiv(a, FRACUNIT + (metabolism * (numboosts-1))); \
	handleboost = max(h, handleboost); \
}

	if (player->sneakertimer) // Sneaker
	{
		UINT8 i;
		for (i = 0; i < player->numsneakers; i++)
		{
			ADDBOOST(FRACUNIT/2, 8*FRACUNIT, SLIPTIDEHANDLING); // + 50% top speed, + 800% acceleration, +50% handling
		}
	}

	if (player->invincibilitytimer) // Invincibility
	{
		// S-Monitor: no extra %
		fixed_t extra = FRACUNIT / 1400 * (player->invincibilitytimer - K_PowerUpRemaining(player, POWERUP_SMONITOR));
		ADDBOOST(3*FRACUNIT/8 + extra, 3*FRACUNIT, SLIPTIDEHANDLING/2); // + 37.5 + ?% top speed, + 300% acceleration, +25% handling
	}

	if (player->growshrinktimer > 0) // Grow
	{
		ADDBOOST(0, 0, SLIPTIDEHANDLING/2); // + 0% top speed, + 0% acceleration, +25% handling
	}

	if (player->flamedash) // Flame Shield dash
	{
		fixed_t dash = K_FlameShieldDashVar(player->flamedash);
		ADDBOOST(
			dash, // + infinite top speed
			3*FRACUNIT, // + 300% acceleration
			FixedMul(FixedDiv(dash, FRACUNIT/2), SLIPTIDEHANDLING/2) // + infinite handling
		);
	}

	if (player->counterdash) // "Fake Flame" (bubble, voltage)
	{
		fixed_t dash = K_FlameShieldDashVar(player->counterdash);
		ADDBOOST(
			dash, // + infinite top speed
			3*FRACUNIT, // + 300% acceleration
			FixedMul(FixedDiv(dash, FRACUNIT/2), SLIPTIDEHANDLING/2) // + infinite handling
		);
	}

	if (player->wavedashboost)
	{
		// NB: This is intentionally under the 25% handleboost threshold required to initiate a sliptide
		ADDBOOST(
			Easing_InCubic(
				player->wavedashpower,
				0,
				8*FRACUNIT/10
			),
			Easing_InSine(
				player->wavedashpower,
				0,
				4*FRACUNIT
			),
			2*SLIPTIDEHANDLING/5
		);  // + 80% top speed (peak), +400% acceleration (peak), +20% handling
	}

	if (player->spindashboost) // Spindash boost
	{
		const fixed_t MAXCHARGESPEED = K_GetSpindashChargeSpeed(player);
		const fixed_t exponent = FixedMul(player->spindashspeed, player->spindashspeed);

		// character & charge dependent
		ADDBOOST(
			FixedMul(MAXCHARGESPEED, exponent), // + 0 to K_GetSpindashChargeSpeed()% top speed
			(40 * exponent), // + 0% to 4000% acceleration
			0 // + 0% handling
		);
	}

	if (player->startboost) // Startup Boost
	{
		ADDBOOST(FRACUNIT, 4*FRACUNIT, SLIPTIDEHANDLING); // + 100% top speed, + 400% acceleration, +50% handling
	}

	if (player->dropdashboost) // Drop dash
	{
		ADDBOOST(FRACUNIT/3, 4*FRACUNIT, SLIPTIDEHANDLING); // + 33% top speed, + 400% acceleration, +50% handling
	}

	if (player->driftboost) // Drift Boost
	{
		// Rebuff Eggman's stat block corner
		// const INT32 heavyAccel = ((9 - player->kartspeed) * 2) + (player->kartweight - 1);
		// const fixed_t heavyAccelBonus = FRACUNIT + ((heavyAccel * maxmetabolismincrease * 2) / 24);

		// hello commit from 18 months ago, The Situation Has Changed.
		// We buffed rings so many times that weight needs a totally different class of change!
		// I've left the old formulas in, in case I'm smoking dick, but this was sorely needed in TA especially.
		const fixed_t herbalfolkmedicine = FRACUNIT + FRACUNIT*(player->kartweight-1)/12 + FRACUNIT*(8-player->kartspeed)/32;

		fixed_t driftSpeed = FRACUNIT/4; // 25% base

		if (player->strongdriftboost > 0)
		{
			// Purple/Rainbow drift boost
			driftSpeed = FixedMul(driftSpeed, 4*FRACUNIT/3); // 25% -> 33%
		}

		// Bottom-left bonus
		// driftSpeed = FixedMul(driftSpeed, heavyAccelBonus);

		// Fucking bonus ever
		driftSpeed = FixedMul(driftSpeed, herbalfolkmedicine);

		ADDBOOST(driftSpeed, 4*FRACUNIT, 0); // + variable top speed, + 400% acceleration, +0% handling
	}

	if (player->trickboost)	// Trick pannel up-boost
	{
		ADDBOOST(player->trickboostpower, 5*FRACUNIT, 0);	// <trickboostpower>% speed, 500% accel, 0% handling
	}

	if (player->gateBoost) // SPB Juicebox boost
	{
		ADDBOOST(3*FRACUNIT/4, 4*FRACUNIT, SLIPTIDEHANDLING/2); // + 75% top speed, + 400% acceleration, +25% handling
	}

	if (player->ringboost) // Ring Boost
	{
		// This one's a little special: we add extra top speed per tic of ringboost stored up, to allow for Ring Box to really rocket away.
		// (We compensate when decrementing ringboost to avoid runaway exponential scaling hell.)
		fixed_t rb = FixedDiv(player->ringboost * FRACUNIT, max(FRACUNIT, K_RingDurationBoost(player)));
		ADDBOOST(
			FRACUNIT/4 + FixedMul(FRACUNIT / 1750, rb),
			4*FRACUNIT,
			Easing_InCubic(min(FRACUNIT, rb / (TICRATE*12)), 0, 2*SLIPTIDEHANDLING/5)
		); // + 20% + ???% top speed, + 400% acceleration, +???% handling
	}

	if (player->eggmanexplode) // Ready-to-explode
	{
		ADDBOOST(6*FRACUNIT/20, FRACUNIT, 0); // + 30% top speed, + 100% acceleration, +0% handling
	}

	if (player->trickcharge)
	{
		// NB: This is an acceleration-only boost.
		// If this is applied earlier in the chain, it will diminish real speed boosts.
		ADDBOOST(0, FRACUNIT, 2*SLIPTIDEHANDLING/10); // 0% speed 100% accel 20% handle
	}

	// This should always remain the last boost stack before tethering
	if (player->botvars.rubberband > FRACUNIT && K_PlayerUsesBotMovement(player) == true)
	{
		ADDBOOST(player->botvars.rubberband - FRACUNIT, 0, 0);
	}

	if (player->draftpower > 0) // Drafting
	{
		// 30% - 44%, each point of speed adds 1.75%
		fixed_t draftspeed = ((3*FRACUNIT)/10) + ((player->kartspeed-1) * ((7*FRACUNIT)/400));

		if (gametyperules & GTR_CLOSERPLAYERS)
		{
			draftspeed *= 2;
		}

		if (K_HasInfiniteTether(player))
		{
			// infinite tether
			draftspeed *= 2;
		}

		speedboost += FixedMul(draftspeed, player->draftpower); // (Drafting suffers no boost stack penalty.)
		numboosts++;
	}

	player->boostpower = boostpower;

	// value smoothing
	if (speedboost > player->speedboost)
	{
		player->speedboost = speedboost;
	}
	else
	{
		player->speedboost += (speedboost - player->speedboost) / (TICRATE/2);
	}

	player->accelboost = accelboost;
	player->handleboost = handleboost;

	player->numboosts = numboosts;
}

fixed_t K_GrowShrinkSpeedMul(const player_t *player)
{
	fixed_t scaleDiff = player->mo->scale - mapobjectscale;
	fixed_t playerScale = FixedDiv(player->mo->scale, mapobjectscale);
	fixed_t speedMul = FRACUNIT;

	if (scaleDiff > 0)
	{
		// Grown
		// Change x2 speed into x1.5
		speedMul = FixedDiv(FixedMul(playerScale, GROW_PHYSICS_SCALE), GROW_SCALE);
	}
	else if (scaleDiff < 0)
	{
		// Shrunk
		// Change x0.5 speed into x0.75
		speedMul = FixedDiv(FixedMul(playerScale, SHRINK_PHYSICS_SCALE), SHRINK_SCALE);
	}

	return speedMul;
}

// Returns kart speed from a stat. Boost power and scale are NOT taken into account, no player or object is necessary.
fixed_t K_GetKartSpeedFromStat(UINT8 kartspeed)
{
	const fixed_t xspd = (3*FRACUNIT)/64;
	fixed_t g_cc = K_GetKartGameSpeedScalar(gamespeed) + xspd;
	fixed_t k_speed = 148;
	fixed_t finalspeed;

	k_speed += kartspeed*4; // 152 - 184

	finalspeed = FixedMul(k_speed<<14, g_cc);
	return finalspeed;
}

fixed_t K_GetKartSpeed(const player_t *player, boolean doboostpower, boolean dorubberband)
{
	const boolean mobjValid = (player->mo != NULL && P_MobjWasRemoved(player->mo) == false);
	const fixed_t physicsScale = mobjValid ? K_GrowShrinkSpeedMul(player) : FRACUNIT;
	fixed_t finalspeed = 0;

	if (K_PodiumSequence() == true)
	{
		// Make 1st reach their podium faster!
		finalspeed = K_GetKartSpeedFromStat(max(1, 11 - (player->position * 3)));

		// Ignore other speed boosts.
		doboostpower = dorubberband = false;
	}
	else
	{
		finalspeed = K_GetKartSpeedFromStat(player->kartspeed);

		if (player->spheres > 0)
		{
			fixed_t sphereAdd = (FRACUNIT/60); // 66% at max
			finalspeed = FixedMul(finalspeed, FRACUNIT + (sphereAdd * player->spheres));
		}

		if (K_PlayerUsesBotMovement(player))
		{
			// Increase bot speed by 0-10% depending on difficulty
			const fixed_t modifier = K_BotMapModifier();
			fixed_t add = ((player->botvars.difficulty-1) * FixedMul(FRACUNIT / 10, modifier)) / (DIFFICULTBOT-1);
			finalspeed = FixedMul(finalspeed, FRACUNIT + add);

			if (player->bot && (player->botvars.rival || cv_levelskull.value))
			{
				// +10% top speed for the rival
				finalspeed = FixedMul(finalspeed, 11*FRACUNIT/10);
			}
		}
	}

	finalspeed = FixedMul(finalspeed, mapobjectscale);

	if (dorubberband == true && player->botvars.rubberband < FRACUNIT && K_PlayerUsesBotMovement(player) == true)
	{
		finalspeed = FixedMul(finalspeed, player->botvars.rubberband);
	}

	if (doboostpower == true)
	{
		// Scale with the player.
		finalspeed = FixedMul(finalspeed, physicsScale);

		// Add speed boosts.
		finalspeed = FixedMul(finalspeed, player->boostpower + player->speedboost);
	}

	if (player->outrun != 0)
	{
		// Milky Way's roads
		finalspeed += FixedMul(player->outrun, physicsScale);
	}

	return finalspeed;
}

fixed_t K_GetKartAccel(const player_t *player)
{
	fixed_t k_accel = 121;
	UINT8 stat = (9 - player->kartspeed);

	if (K_PodiumSequence() == true)
	{
		// Normalize to Metal's accel
		stat = 1;
	}

	k_accel += 17 * stat; // 121 - 257

	// Marble Garden Top gets 1200% accel
	if (player->curshield == KSHIELD_TOP)
	{
		k_accel = FixedMul(k_accel, player->topAccel);
	}

	if (K_PodiumSequence() == true)
	{
		k_accel = FixedMul(k_accel, FRACUNIT / 4);
	}
	else
	{
		k_accel = FixedMul(k_accel, (FRACUNIT + player->accelboost) / 4);
	}

	return k_accel;
}

UINT16 K_GetKartFlashing(const player_t *player)
{
	UINT16 tics = flashingtics;

	if (gametyperules & GTR_BUMPERS)
	{
		return 1;
	}

	if (player == NULL)
	{
		return tics;
	}

	tics += (tics/8) * (player->kartspeed);
	return tics;
}

boolean K_PlayerShrinkCheat(const player_t *player)
{
	return (
		(player->pflags & PF_SHRINKACTIVE)
		&& (player->bot == false)
		&& (modeattacking == ATTACKING_NONE) // Anyone want to make another record attack category?
	);
}

void K_UpdateShrinkCheat(player_t *player)
{
	const boolean mobjValid = (player->mo != NULL && P_MobjWasRemoved(player->mo) == false);

	if (player->pflags & PF_SHRINKME)
	{
		player->pflags |= PF_SHRINKACTIVE;
	}
	else
	{
		player->pflags &= ~PF_SHRINKACTIVE;
	}

	if (mobjValid == true && K_PlayerShrinkCheat(player) == true)
	{
		player->mo->destscale = FixedMul(mapobjectscale, SHRINK_SCALE);
	}
}

boolean K_KartKickstart(const player_t *player)
{
	return ((player->pflags & PF_KICKSTARTACCEL)
		&& (!K_PlayerUsesBotMovement(player))
		&& (player->kickstartaccel >= ACCEL_KICKSTART));
}

UINT16 K_GetKartButtons(const player_t *player)
{
	return (player->cmd.buttons |
		(K_KartKickstart(player) ? BT_ACCELERATE : 0));
}

SINT8 K_GetForwardMove(const player_t *player)
{
	SINT8 forwardmove = player->cmd.forwardmove;

	if ((player->pflags & PF_STASIS)
		|| (player->carry == CR_SLIDING)
		|| Obj_PlayerRingShooterFreeze(player) == true)
	{
		return 0;
	}

	if (player->sneakertimer || player->spindashboost
		|| (((gametyperules & (GTR_ROLLINGSTART|GTR_CIRCUIT)) == (GTR_ROLLINGSTART|GTR_CIRCUIT)) && (leveltime < TICRATE/2)))
	{
		return MAXPLMOVE;
	}

	if (player->spinouttimer != 0
		|| player->icecube.frozen
		|| K_PressingEBrake(player) == true
		|| K_PlayerEBrake(player) == true)
	{
		return 0;
	}

	if (K_KartKickstart(player)) // unlike the brute forward of sneakers, allow for backwards easing here
	{
		forwardmove += MAXPLMOVE;
		if (forwardmove > MAXPLMOVE)
			forwardmove = MAXPLMOVE;
	}

	if (player->curshield == KSHIELD_TOP)
	{
		if (forwardmove < 0 ||
				(K_GetKartButtons(player) & BT_DRIFT))
		{
			forwardmove = 0;
		}
		else
		{
			// forwardmove = MAXPLMOVE;

			UINT8 minmove = MAXPLMOVE/10;
			fixed_t assistmove = (MAXPLMOVE - minmove) * FRACUNIT;

			angle_t topdelta = player->mo->angle - K_MomentumAngle(player->mo);
			fixed_t topmult = FINECOSINE(topdelta >> ANGLETOFINESHIFT);
			topmult = (topmult/2) + (FRACUNIT/2);
			assistmove = FixedMul(topmult, assistmove);

			forwardmove = minmove + FixedInt(assistmove);
		}
	}

	return forwardmove;
}

fixed_t K_GetNewSpeed(const player_t *player)
{
	const fixed_t accelmax = 4000;
	fixed_t p_speed = K_GetKartSpeed(player, true, true);
	fixed_t p_accel = K_GetKartAccel(player);

	fixed_t newspeed, oldspeed, finalspeed;

	if (player->curshield == KSHIELD_TOP)
	{
		p_speed = 15 * p_speed / 10;
	}

	if (K_PlayerUsesBotMovement(player) == true && player->botvars.rubberband > 0)
	{
		// Acceleration is tied to top speed...
		// so if we want JUST a top speed boost, we have to do this...
		p_accel = FixedDiv(p_accel, player->botvars.rubberband);
	}

	oldspeed = R_PointToDist2(0, 0, player->rmomx, player->rmomy);
	// Don't calculate the acceleration as ever being above top speed
	if (oldspeed > p_speed)
		oldspeed = p_speed;
	newspeed = FixedDiv(FixedDiv(FixedMul(oldspeed, accelmax - p_accel) + FixedMul(p_speed, p_accel), accelmax), K_PlayerBaseFriction(player, ORIG_FRICTION));

	finalspeed = newspeed - oldspeed;

	return finalspeed;
}

fixed_t K_3dKartMovement(const player_t *player)
{
	fixed_t finalspeed = K_GetNewSpeed(player);

	fixed_t movemul = FRACUNIT;
	SINT8 forwardmove = K_GetForwardMove(player);

	movemul = abs(forwardmove * FRACUNIT) / 50;

	// forwardmove is:
	//  50 while accelerating,
	//   0 with no gas, and
	// -25 when only braking.
	if (forwardmove >= 0)
	{
		finalspeed = FixedMul(finalspeed, movemul);
	}
	else
	{
		finalspeed = FixedMul(-mapobjectscale/2, movemul);
	}

	return finalspeed;
}

angle_t K_MomentumAngleEx(const mobj_t *mo, const fixed_t threshold)
{
	if (FixedHypot(mo->momx, mo->momy) > threshold)
	{
		return R_PointToAngle2(0, 0, mo->momx, mo->momy);
	}
	else
	{
		return mo->angle; // default to facing angle, rather than 0
	}
}

angle_t K_MomentumAngleReal(const mobj_t *mo)
{
	if (mo->momx || mo->momy)
	{
		return R_PointToAngle2(0, 0, mo->momx, mo->momy);
	}
	else
	{
		return mo->angle; // default to facing angle, rather than 0
	}
}

void K_AwardPlayerRings(player_t *player, UINT16 rings, boolean overload)
{
	UINT16 superring;

	if (!overload)
	{
		INT32 totalrings =
			RINGTOTAL(player) + (player->superring);

		/* capped at 20 rings */
		if ((totalrings + rings) > 20)
		{
			if (totalrings >= 20)
				return; // woah dont let that go negative buster
			rings = (20 - totalrings);
		}
	}

	superring = player->superring + rings;

	/* check if not overflow */
	if (superring > player->superring)
		player->superring = superring;
}

void K_DoInstashield(player_t *player)
{
	mobj_t *layera;
	mobj_t *layerb;

	if (player->instashield > 0)
		return;

	player->instashield = 15; // length of instashield animation
	S_StartSound(player->mo, sfx_cdpcm9);

	layera = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_INSTASHIELDA);
	layera->old_x = player->mo->old_x;
	layera->old_y = player->mo->old_y;
	layera->old_z = player->mo->old_z;
	P_SetTarget(&layera->target, player->mo);

	layerb = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_INSTASHIELDB);
	layerb->old_x = player->mo->old_x;
	layerb->old_y = player->mo->old_y;
	layerb->old_z = player->mo->old_z;
	P_SetTarget(&layerb->target, player->mo);
}

void K_DoPowerClash(mobj_t *t1, mobj_t *t2) {
	mobj_t *clash;
	UINT8 lag1 = 5;
	UINT8 lag2 = 5;

	// short-circuit instashield for vfx visibility
	if (t1->player)
	{
		t1->player->instashield = 1;
		t1->player->speedpunt += 20;
		lag1 -= min(lag1, t1->player->speedpunt/10);
	}

	if (t2->player)
	{
		t2->player->instashield = 1;
		t2->player->speedpunt += 20;
		lag2 -= min(lag1, t2->player->speedpunt/10);
	}

	S_StartSound(t1, sfx_parry);
	K_AddHitLag(t1, lag1+1, false);
	K_AddHitLag(t2, lag2+1, false);

	clash = P_SpawnMobj((t1->x/2) + (t2->x/2), (t1->y/2) + (t2->y/2), (t1->z/2) + (t2->z/2), MT_POWERCLASH);

	// needs to handle mixed scale collisions (t1 grow t2 invinc)...
	clash->z = clash->z + (t1->height/4) + (t2->height/4);
	clash->angle = R_PointToAngle2(clash->x, clash->y, t1->x, t1->y) + ANGLE_90;

	// Shrink over time (accidental behavior that looked good)
	clash->destscale = (t1->scale) + (t2->scale);
	P_SetScale(clash, 3*clash->destscale/2);
}

void K_DoGuardBreak(mobj_t *t1, mobj_t *t2) {
	mobj_t *clash;

	if (!(t1->player && t2->player))
		return;

	if (P_PlayerInPain(t2->player))
		return;

	t1->player->defenseLockout = 1;

	S_StartSound(t1, sfx_gbrk);
	K_AddHitLag(t1, 24, true);

	K_AddMessageForPlayer(t2->player, "Smashed 'em!", false, false);
	K_AddMessageForPlayer(t1->player, "BARRIER BREAK!!", false, false);

	angle_t thrangle = R_PointToAngle2(t2->x, t2->y, t1->x, t1->y);
	P_Thrust(t1, thrangle, 7*mapobjectscale);

	P_DamageMobj(t1, t2, t2, 1, DMG_TUMBLE);

	clash = P_SpawnMobj((t1->x/2) + (t2->x/2), (t1->y/2) + (t2->y/2), (t1->z/2) + (t2->z/2), MT_GUARDBREAK);

	// needs to handle mixed scale collisions
	clash->z = clash->z + (t1->height/4) + (t2->height/4);
	clash->angle = R_PointToAngle2(clash->x, clash->y, t1->x, t1->y) + ANGLE_90;
	clash->color = t1->color;

	clash->destscale = 3*((t1->scale) + (t2->scale))/2;
}

void K_BattleAwardHit(player_t *player, player_t *victim, mobj_t *inflictor, UINT8 damage)
{
	UINT8 points = 1;
	boolean trapItem = false;
	boolean finishOff = (victim->mo->health > 0) && (victim->mo->health <= damage);

	if (!(gametyperules & GTR_POINTLIMIT))
	{
		// No points in this gametype.
		return;
	}

	if (player == NULL || victim == NULL)
	{
		// Invalid player or victim
		return;
	}

	if (player == victim)
	{
		// You cannot give yourself points
		return;
	}

	if (player->exiting)
	{
		// The round has already ended, don't mess with points
		return;
	}

	if ((inflictor && !P_MobjWasRemoved(inflictor)) && (inflictor->type == MT_BANANA && inflictor->health > 1))
	{
		trapItem = true;
	}

	// Only apply score bonuses to non-bananas
	if (trapItem == false)
	{
		if (K_IsPlayerWanted(victim))
		{
			// +3 points for hitting a wanted player
			points = 3;
		}
		else if (gametyperules & GTR_BUMPERS)
		{
			if (finishOff)
			{
				// +2 points for finishing off a player
				points = 2;
			}
		}
	}

	// Check this before adding to player score
	if ((gametyperules & GTR_BUMPERS) && finishOff && g_pointlimit <= player->roundscore)
	{
		K_EndBattleRound(player);

		mobj_t *source = !P_MobjWasRemoved(inflictor) ? inflictor : player->mo;

		K_StartRoundWinCamera(
			victim->mo,
			R_PointToAngle2(source->x, source->y, victim->mo->x, victim->mo->y) + ANGLE_135,
			200*mapobjectscale,
			8*TICRATE,
			FRACUNIT/512
		);
	}

	K_GivePointsToPlayer(player, victim, points);
}

void K_SpinPlayer(player_t *player, mobj_t *inflictor, mobj_t *source, INT32 type)
{
	(void)inflictor;
	(void)source;

	K_DirectorFollowAttack(player, inflictor, source);

	player->spinouttype = type;

	if (( player->spinouttype & KSPIN_THRUST ))
	{
		// At spinout, player speed is increased to 1/4 their regular speed, moving them forward
		fixed_t spd = K_GetKartSpeed(player, true, true) / 4;

		if (player->speed < spd)
			P_InstaThrust(player->mo, player->mo->angle, FixedMul(spd, player->mo->scale));

		S_StartSound(player->mo, sfx_slip);
	}

	player->spinouttimer = (3*TICRATE/2)+2;
	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
}

void K_RemoveGrowShrink(player_t *player)
{
	if (player->mo && !P_MobjWasRemoved(player->mo))
	{
		if (player->growshrinktimer > 0) // Play Shrink noise
			S_StartSound(player->mo, sfx_kc59);
		else if (player->growshrinktimer < 0) // Play Grow noise
			S_StartSound(player->mo, sfx_kc5a);

		K_KartResetPlayerColor(player);

		player->mo->scalespeed = mapobjectscale/TICRATE;
		player->mo->destscale = mapobjectscale;

		if (K_PlayerShrinkCheat(player) == true)
		{
			player->mo->destscale = FixedMul(player->mo->destscale, SHRINK_SCALE);
		}
	}

	player->growshrinktimer = 0;
	player->roundconditions.consecutive_grow_lasers = 0;
}

// Should this object actually scale check?
// Scale-to-scale comparisons only make sense for objects that expect to have dynamic scale.
static boolean K_IsScaledItem(mobj_t *mobj)
{
	return mobj && !P_MobjWasRemoved(mobj) &&
		(mobj->type == MT_ORBINAUT || mobj->type == MT_JAWZ || mobj->type == MT_GACHABOM
		|| mobj->type == MT_BANANA || mobj->type == MT_EGGMANITEM || mobj->type == MT_BALLHOG
		|| mobj->type == MT_SSMINE || mobj->type == MT_LANDMINE || mobj->type == MT_SINK
		|| mobj->type == MT_GARDENTOP || mobj->type == MT_DROPTARGET || mobj->type == MT_PLAYER);
}


boolean K_IsBigger(mobj_t *compare, mobj_t *other)
{
	fixed_t compareScale, otherScale;
	const fixed_t requiredDifference = (mapobjectscale/4);

	if ((compare == NULL || P_MobjWasRemoved(compare) == true)
		|| (other == NULL || P_MobjWasRemoved(other) == true))
	{
		return false;
	}

	// If a player is colliding with a non-kartitem object, we don't care about what scale that object is:
	// mappers are liable to fuck with the scale for their own reasons, and we need to compare against the
	// player's base scale instead to match expectations.
	if (K_IsScaledItem(compare) != K_IsScaledItem(other))
	{
		if (compare->type == MT_PLAYER)
			return (compare->scale > requiredDifference + FixedMul(mapobjectscale, P_GetMobjDefaultScale(compare)));
		else if (other->type == MT_PLAYER)
			return false; // Haha what the fuck are you doing
		// fallthrough
	}

	if ((compareScale = P_GetMobjDefaultScale(compare)) != FRACUNIT)
	{
		compareScale = FixedDiv(compare->scale, compareScale);
	}
	else
	{
		compareScale = compare->scale;
	}

	if ((otherScale = P_GetMobjDefaultScale(other)) != FRACUNIT)
	{
		otherScale = FixedDiv(other->scale, otherScale);
	}
	else
	{
		otherScale = other->scale;
	}

	return (compareScale > otherScale + requiredDifference);
}

static fixed_t K_TumbleZ(mobj_t *mo, fixed_t input)
{
	// Scales base tumble gravity to FRACUNIT
	const fixed_t baseGravity = FixedMul(DEFAULT_GRAVITY, TUMBLEGRAVITY);

	// Adapt momz w/ gravity
	fixed_t gravityAdjust = FixedDiv(P_GetMobjGravity(mo), baseGravity);

	if (mo->eflags & MFE_UNDERWATER)
	{
		// Reverse doubled falling speed.
		gravityAdjust /= 2;
	}

	return FixedMul(input, -gravityAdjust);
}

void K_TumblePlayer(player_t *player, mobj_t *inflictor, mobj_t *source, boolean soften)
{
	(void)source;

	K_DirectorFollowAttack(player, inflictor, source);

	player->tumbleBounces = 1;

	if (player->tripwireState == TRIPSTATE_PASSED)
	{
		player->tumbleHeight = 50;
	}
	else
	{
		player->mo->momx = 2 * player->mo->momx / 3;
		player->mo->momy = 2 * player->mo->momy / 3;

		player->tumbleHeight = 30;
	}

	player->pflags &= ~PF_TUMBLESOUND;

	if (inflictor && !P_MobjWasRemoved(inflictor))
	{
		fixed_t addHeight = FixedHypot(FixedHypot(inflictor->momx, inflictor->momy) / 2, FixedHypot(player->mo->momx, player->mo->momy) / 2);
		if (soften)
			addHeight = FixedMul(addHeight, 6*FRACUNIT/10);
		player->tumbleHeight += (addHeight / player->mo->scale);
		player->tumbleHeight = min(200, player->tumbleHeight);
	}

	if (soften)
	{
		player->tumbleBounces = 2;
	}

	S_StartSound(player->mo, sfx_s3k9b);

	player->mo->momz = K_TumbleZ(player->mo, player->tumbleHeight * FRACUNIT);

	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
}

angle_t K_StumbleSlope(angle_t angle, angle_t pitch, angle_t roll)
{
	fixed_t pitchMul = -FINESINE(angle >> ANGLETOFINESHIFT);
	fixed_t rollMul = FINECOSINE(angle >> ANGLETOFINESHIFT);

	angle_t slope = FixedMul(pitch, pitchMul) + FixedMul(roll, rollMul);

	if (slope > ANGLE_180)
	{
		slope = InvAngle(slope);
	}

	return slope;
}

void K_StumblePlayer(player_t *player)
{
	P_ResetPlayer(player);

#if 0
	// Single, medium bounce
	player->tumbleBounces = TUMBLEBOUNCES;
	player->tumbleHeight = 30;
#else
	// Two small bounces
	player->tumbleBounces = TUMBLEBOUNCES-1;
	player->tumbleHeight = 20;
#endif

	player->pflags &= ~PF_TUMBLESOUND;
	S_StartSound(player->mo, sfx_s3k9b);

	// and then modulate momz like that...
	player->mo->momz = K_TumbleZ(player->mo, player->tumbleHeight * FRACUNIT);

	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);

	// Reset slope.
	P_ResetPitchRoll(player->mo);
}

boolean K_CheckStumble(player_t *player, angle_t oldPitch, angle_t oldRoll, boolean fromAir)
{
	angle_t steepVal = ANGLE_MAX;
	angle_t oldSlope, newSlope;
	angle_t slopeDelta;

	// If you don't land upright on a slope, then you tumble,
	// kinda like Kirby Air Ride

	if (player->tumbleBounces)
	{
		// Already tumbling.
		return false;
	}

	if (fromAir && player->airtime < STUMBLE_AIRTIME
		&& player->airtime > 1) // ACHTUNG HACK, sorry. Ground-to-ground transitions sometimes have 1-tic airtime because collision blows
	{
		// Short airtime with no reaction window, probably a track traversal setpiece.
		// Don't punish for these.
		return false;
	}

	if ((player->mo->pitch == oldPitch)
		&& (player->mo->roll == oldRoll))
	{
		// No change.
		return false;
	}

	if (fromAir == true)
	{
		steepVal = STUMBLE_STEEP_VAL_AIR;
	}
	else
	{
		steepVal = STUMBLE_STEEP_VAL;
	}

	oldSlope = K_StumbleSlope(player->mo->angle, oldPitch, oldRoll);

	if (oldSlope <= steepVal)
	{
		// Transferring from flat ground to a steep slope
		// is a free action. (The other way around isn't, though.)
		return false;
	}

	newSlope = K_StumbleSlope(player->mo->angle, player->mo->pitch, player->mo->roll);
	slopeDelta = AngleDelta(oldSlope, newSlope);

	if (slopeDelta <= steepVal)
	{
		// Needs to be VERY steep before we'll punish this.
		return false;
	}

	// Oh jeez, you landed on your side.
	// You get to tumble.

	K_StumblePlayer(player);
	return true;
}

void K_InitStumbleIndicator(player_t *player)
{
	mobj_t *new = NULL;

	if (player == NULL)
	{
		return;
	}

	if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
	{
		return;
	}

	if (P_MobjWasRemoved(player->stumbleIndicator) == false)
	{
		P_RemoveMobj(player->stumbleIndicator);
	}

	new = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_SMOOTHLANDING);

	P_SetTarget(&player->stumbleIndicator, new);
	P_SetTarget(&new->target, player->mo);
	new->renderflags |= RF_DONTDRAW;
}

void K_InitWavedashIndicator(player_t *player)
{
	mobj_t *new = NULL;

	if (player == NULL)
	{
		return;
	}

	if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
	{
		return;
	}

	if (P_MobjWasRemoved(player->wavedashIndicator) == false)
	{
		P_RemoveMobj(player->wavedashIndicator);
	}

	new = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_WAVEDASH);

	P_SetTarget(&player->wavedashIndicator, new);
	P_SetTarget(&new->target, player->mo);
	new->renderflags |= RF_DONTDRAW;
}

void K_InitTrickIndicator(player_t *player)
{
	mobj_t *new = NULL;

	if (player == NULL)
	{
		return;
	}

	if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
	{
		return;
	}

	if (P_MobjWasRemoved(player->trickIndicator) == false)
	{
		if (P_MobjWasRemoved(player->trickIndicator->tracer) == false)
		{
			P_RemoveMobj(player->trickIndicator->tracer);
		}

		P_RemoveMobj(player->trickIndicator);
	}

	UINT32 invis = (RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(player));

	new = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_TRICKINDICATOR);

	P_SetMobjState(new, S_INVISIBLE);
	P_SetTarget(&player->trickIndicator, new);
	P_SetTarget(&new->target, player->mo);
	new->renderflags |= invis;

	mobj_t *secondlayer = P_SpawnMobjFromMobj(new, 0, 0, 0, MT_OVERLAY);

	P_SetMobjState(secondlayer, S_INVISIBLE);
	P_SetTarget(&new->tracer, secondlayer);
	P_SetTarget(&secondlayer->target, new);
	secondlayer->renderflags |= invis;

	secondlayer->dispoffset = 1;
	secondlayer->flags |= MF_DONTENCOREMAP;
}

void K_UpdateStumbleIndicator(player_t *player)
{
	const angle_t fudge = ANG15;
	mobj_t *mobj = NULL;

	boolean air = false;
	angle_t steepVal = STUMBLE_STEEP_VAL;
	angle_t slopeSteep = 0;
	angle_t steepRange = ANGLE_90;

	INT32 delta = 0;
	INT32 trans = 0;

	if (player == NULL)
	{
		return;
	}

	if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
	{
		return;
	}

	if (player->stumbleIndicator == NULL || P_MobjWasRemoved(player->stumbleIndicator) == true)
	{
		K_InitStumbleIndicator(player);
		return;
	}

	mobj = player->stumbleIndicator;

	P_MoveOrigin(mobj, player->mo->x, player->mo->y, player->mo->z + (player->mo->height / 2));

	air = !P_IsObjectOnGround(player->mo);
	steepVal = (air ? STUMBLE_STEEP_VAL_AIR : STUMBLE_STEEP_VAL) - fudge;
	slopeSteep = max(AngleDelta(player->mo->pitch, 0), AngleDelta(player->mo->roll, 0));

	delta = 0;

	if (slopeSteep > steepVal)
	{
		angle_t testAngles[2];
		INT32 testDeltas[2];
		UINT8 i;

		testAngles[0] = R_PointToAngle2(0, 0, player->mo->pitch, player->mo->roll);
		testAngles[1] = R_PointToAngle2(0, 0, -player->mo->pitch, -player->mo->roll);

		for (i = 0; i < 2; i++)
		{
			testDeltas[i] = AngleDeltaSigned(player->mo->angle, testAngles[i]);
		}

		if (abs(testDeltas[1]) < abs(testDeltas[0]))
		{
			delta = testDeltas[1];
		}
		else
		{
			delta = testDeltas[0];
		}
	}

	if (delta < 0)
	{
		mobj->renderflags |= RF_HORIZONTALFLIP;
	}
	else
	{
		mobj->renderflags &= ~RF_HORIZONTALFLIP;
	}

	if (air && player->airtime < STUMBLE_AIRTIME)
		delta = 0;

	steepRange = ANGLE_90 - steepVal;
	delta = max(0, abs(delta) - ((signed)steepVal));
	trans = ((FixedDiv(AngleFixed(delta), AngleFixed(steepRange)) * (NUMTRANSMAPS - 2)) + (FRACUNIT/2)) / FRACUNIT;

	if (trans < 0)
	{
		trans = 0;
	}

	if (trans > (NUMTRANSMAPS - 2))
	{
		trans = (NUMTRANSMAPS - 2);
	}

	// invert
	trans = (NUMTRANSMAPS - 2) - trans;

	mobj->renderflags |= RF_DONTDRAW;

	if (trans < (NUMTRANSMAPS - 2))
	{
		mobj->renderflags &= ~(RF_TRANSMASK | K_GetPlayerDontDrawFlag(player));

		if (trans != 0)
		{
			mobj->renderflags |= (trans << RF_TRANSSHIFT);
		}
	}
}

#define MIN_WAVEDASH_CHARGE ((11*TICRATE/16)*9)

static boolean K_IsLosingWavedash(player_t *player)
{
	if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
		return true;
	if (!K_Sliptiding(player) && player->wavedash < MIN_WAVEDASH_CHARGE)
		return true;
	if (!K_Sliptiding(player) && player->drift == 0
		&& P_IsObjectOnGround(player->mo) && player->sneakertimer == 0
		&& player->driftboost == 0)
		return true;
	return false;
}

void K_UpdateWavedashIndicator(player_t *player)
{
	mobj_t *mobj = NULL;

	if (player == NULL)
	{
		return;
	}

	if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
	{
		return;
	}

	if (player->wavedashIndicator == NULL || P_MobjWasRemoved(player->wavedashIndicator) == true)
	{
		K_InitWavedashIndicator(player);
		return;
	}

	mobj = player->wavedashIndicator;
	angle_t momentumAngle = K_MomentumAngle(player->mo);

	P_MoveOrigin(mobj, player->mo->x - FixedMul(40*mapobjectscale, FINECOSINE(momentumAngle >> ANGLETOFINESHIFT)),
		player->mo->y - FixedMul(40*mapobjectscale, FINESINE(momentumAngle >> ANGLETOFINESHIFT)),
		player->mo->z + (player->mo->height / 2));
	mobj->angle = momentumAngle + ANGLE_90;
	P_SetScale(mobj, 3 * player->mo->scale / 2);

	// No stored boost (or negligible enough that it might be a mistake)
	if (player->wavedash <= HIDEWAVEDASHCHARGE)
	{
		mobj->renderflags |= RF_DONTDRAW;
		mobj->frame = 7;
		return;
	}

	mobj->renderflags &= ~RF_DONTDRAW;

	UINT32 chargeFrame = 7 - min(7, player->wavedash / 100);
	UINT32 decayFrame = min(7, player->wavedashdelay / 2);
	if (max(chargeFrame, decayFrame) > mobj->frame)
		mobj->frame++;
	else if (max(chargeFrame, decayFrame) < mobj->frame)
		mobj->frame--;

	mobj->renderflags &= ~RF_TRANSMASK;
	mobj->renderflags |= RF_PAPERSPRITE;

	if (player->wavedash < MIN_WAVEDASH_CHARGE)
		mobj->renderflags |= RF_TRANS50;

	if (K_IsLosingWavedash(player))
	{
		// Decay timer's ticking
		mobj->rollangle += 3*ANG30/4;
		if (leveltime % 2 == 0)
			mobj->renderflags |= RF_TRANS50;
	}
	else
	{
		// Storing boost
		mobj->rollangle += 3*ANG15/4;
	}
}

static mobj_t *K_TrickCatholocismBlast(mobj_t *trickIndicator, fixed_t destscale, angle_t rollangle)
{
	// It's my last minute visual effect and I get to choose the ridiculous function name - toast 061123

	mobj_t *catholocismBlast = P_SpawnGhostMobj(trickIndicator); // HOLY?
	catholocismBlast->height = 1;
	catholocismBlast->destscale = destscale;
	catholocismBlast->fuse = 12;
	catholocismBlast->rollangle = rollangle;
	catholocismBlast->dispoffset = -1;

	return catholocismBlast;
}

void K_UpdateTrickIndicator(player_t *player)
{
	mobj_t *mobj = NULL;

	if (player == NULL)
	{
		return;
	}

	if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
	{
		return;
	}

	if (player->trickIndicator == NULL
	|| P_MobjWasRemoved(player->trickIndicator) == true
	|| player->trickIndicator->tracer == NULL
	|| P_MobjWasRemoved(player->trickIndicator->tracer) == true)
	{
		K_InitTrickIndicator(player);
		return;
	}

	mobj = player->trickIndicator;

	statenum_t test = (mobj->state-states);

	if (test >= S_TRICKINDICATOR_UNDERLAY_ARROW)
		return;

	const fixed_t onidistance = 150*mapobjectscale;

	P_MoveOrigin(
		mobj,
		player->mo->x + P_ReturnThrustX(player->mo, player->mo->angle, onidistance),
		player->mo->y + P_ReturnThrustY(player->mo, player->mo->angle, onidistance),
		player->mo->z + (player->mo->height / 2));
	mobj->angle = player->mo->angle + ANGLE_90;

	if (player->trickpanel == TRICKSTATE_NONE
		&& test != S_INVISIBLE)
	{
		K_TrickCatholocismBlast(mobj, 1, ANGLE_22h);

		P_SetMobjState(mobj, S_INVISIBLE);
		P_SetMobjState(mobj->tracer, S_INVISIBLE);
	}
}

static boolean K_LastTumbleBounceCondition(player_t *player)
{
	return (player->tumbleBounces > TUMBLEBOUNCES && player->tumbleHeight < 60);
}

// Bumpers give you bonus launch height and speed, strengthening your DI to help evade combos.
// bumperinflate visuals are handled by MT_BATTLEBUMPER, but the effects are in K_KartPlayerThink.
void K_BumperInflate(player_t *player)
{
	if (!player || P_MobjWasRemoved(player->mo))
		return;

	if (!(gametyperules & GTR_BUMPERS))
		return;

	player->bumperinflate = 3;

	if (player->mo->health > 1)
		S_StartSound(player->mo, sfx_cdpcm9);
}

static void K_HandleTumbleBounce(player_t *player)
{
	player->tumbleBounces++;
	player->tumbleHeight = (player->tumbleHeight * ((player->tumbleHeight > 100) ? 3 : 4)) / 5;
	player->pflags &= ~PF_TUMBLESOUND;

	if (player->markedfordeath)
	{
		if (player->exiting == false && specialstageinfo.valid == true)
		{
			// markedfordeath is when player's die at -20 rings
			HU_DoTitlecardCEcho(player, "NOT ENOUGH\\RINGS...", false);
		}

		player->markedfordeath = false;
		P_StartQuakeFromMobj(5, 64 * player->mo->scale, 4096 * player->mo->scale, player->mo);
		P_DamageMobj(player->mo, NULL, NULL, 1, DMG_INSTAKILL);
		return;
	}

	if (player->tumbleHeight < 10)
	{
		// 10 minimum bounce height
		player->tumbleHeight = 10;
	}

	if (K_LastTumbleBounceCondition(player))
	{
		// Leave tumble state when below 40 height, and have bounced off the ground enough

		if (player->pflags & PF_TUMBLELASTBOUNCE)
		{
			// End tumble state
			player->tumbleBounces = 0;
			player->pflags &= ~PF_TUMBLELASTBOUNCE; // Reset for next time
			return;
		}
		else
		{
			// One last bounce at the minimum height, to reset the animation
			player->tumbleHeight = 10;
			player->pflags |= PF_TUMBLELASTBOUNCE;
			player->mo->rollangle = 0;	// p_user.c will stop rotating the player automatically
			P_ResetPitchRoll(player->mo); // Prevent Kodachrome Void infinite
		}
	}

	K_BumperInflate(player);

	// A bit of damage hitlag.
	// This gives a window for DI!!
	K_AddHitLag(player->mo, 3, true);

	if (player->tumbleHeight >= 40)
	{
		// funny earthquakes for the FEEL
		P_StartQuakeFromMobj(6, (player->tumbleHeight * 3 * player->mo->scale) / 2, 512 * player->mo->scale, player->mo);
	}

	S_StartSound(player->mo, (player->tumbleHeight < 40) ? sfx_s3k5d : sfx_s3k5f);	// s3k5d is bounce < 50, s3k5f otherwise!

	player->mo->momx = player->mo->momx / 2;
	player->mo->momy = player->mo->momy / 2;

	// and then modulate momz like that...
	player->mo->momz = K_TumbleZ(player->mo, player->tumbleHeight * FRACUNIT);
}

// Play a falling sound when you start falling while tumbling and you're nowhere near done bouncing
static void K_HandleTumbleSound(player_t *player)
{
	fixed_t momz;
	momz = player->mo->momz * P_MobjFlip(player->mo);

	if (!K_LastTumbleBounceCondition(player) &&
			!(player->pflags & PF_TUMBLESOUND) && momz < -10*player->mo->scale)
	{
		S_StartSound(player->mo, sfx_s3k51);
		player->pflags |= PF_TUMBLESOUND;
	}
}

void K_TumbleInterrupt(player_t *player)
{
	// If player was tumbling, set variables so that they don't tumble like crazy after they're done respawning
	if (player->tumbleBounces > 0)
	{
		player->tumbleBounces = 0; // MAXBOUNCES-1;
		player->pflags &= ~PF_TUMBLELASTBOUNCE;
		//players->tumbleHeight = 20;

		player->mo->rollangle = 0;
		player->spinouttype = KSPIN_WIPEOUT;
		player->spinouttimer = player->wipeoutslow = TICRATE+2;
	}
}

void K_ApplyTripWire(player_t *player, tripwirestate_t state)
{
	// We are either softlocked or wildly misbehaving. Stop that!
	if (state == TRIPSTATE_BLOCKED && player->tripwireReboundDelay && (player->speed > 5 * K_GetKartSpeed(player, false, false)))
		K_TumblePlayer(player, NULL, NULL, false);

	if (state == TRIPSTATE_PASSED)
	{
		S_StartSound(player->mo, sfx_ssa015);

		if (player->roundconditions.tripwire_hyuu == false
			&& player->hyudorotimer > 0)
		{
			player->roundconditions.tripwire_hyuu = true;
			player->roundconditions.checkthisframe = true;
		}

		if (player->tripwirePass == TRIPWIRE_CONSUME && player->tripwireLeniency == 0)
		{
			if (player->curshield == KSHIELD_TOP)
			{
				S_StartSound(player->mo, sfx_kc65); // Player's handling is about to change, alert them!
				Obj_GardenTopDestroy(player);
			}
		}

		player->tripwireLeniency += TICRATE/2;
	}
	else if (state == TRIPSTATE_BLOCKED)
	{
		S_StartSound(player->mo, sfx_kc40);
		player->tripwireReboundDelay = 60;
		if (player->curshield == KSHIELD_BUBBLE)
			player->tripwireReboundDelay *= 2;
	}

	player->tripwireState = state;

	if (player->hyudorotimer <= 0)
	{
		K_AddHitLag(player->mo, 10, false);
		player->mo->hitlag -= min(player->mo->hitlag, player->tripwireUnstuck/4);
	}

	if (state == TRIPSTATE_PASSED && player->spinouttimer &&
			player->speed > K_PlayerTripwireSpeedThreshold(player))
	{
		K_TumblePlayer(player, NULL, NULL, false);
	}

	player->tripwireUnstuck += 10;
}

INT32 K_ExplodePlayer(player_t *player, mobj_t *inflictor, mobj_t *source) // A bit of a hack, we just throw the player up higher here and extend their spinout timer
{
	INT32 ringburst = 10;
	fixed_t spbMultiplier = FRACUNIT;

	(void)source;

	K_DirectorFollowAttack(player, inflictor, source);

	if (inflictor != NULL && P_MobjWasRemoved(inflictor) == false)
	{
		if (inflictor->type == MT_SPBEXPLOSION && inflictor->movefactor)
		{
			if (modeattacking & ATTACKING_SPB)
			{
				P_DamageMobj(player->mo, inflictor, source, 1, DMG_INSTAKILL);
				player->SPBdistance = 0;
				Music_StopAll();
			}

			spbMultiplier = inflictor->movefactor;

			if (spbMultiplier <= 0)
			{
				// Convert into stumble.
				K_StumblePlayer(player);
				return 0;
			}
			else if (spbMultiplier < FRACUNIT)
			{
				spbMultiplier = FRACUNIT;
			}
		}
	}

	player->mo->momz = 18 * mapobjectscale * P_MobjFlip(player->mo); // please stop forgetting mobjflip checks!!!!
	player->mo->momx = player->mo->momy = 0;

	player->spinouttype = KSPIN_EXPLOSION;
	player->spinouttimer = (3*TICRATE/2)+2;

	if (spbMultiplier != FRACUNIT)
	{
		player->mo->momz = FixedMul(player->mo->momz, spbMultiplier);
		player->spinouttimer = FixedMul(player->spinouttimer, spbMultiplier + ((spbMultiplier - FRACUNIT) / 2));

		ringburst = FixedMul(ringburst * FRACUNIT, spbMultiplier) / FRACUNIT;
		if (ringburst > 20)
		{
			ringburst = 20;
		}
	}

	if (player->mo->eflags & MFE_UNDERWATER)
		player->mo->momz = (117 * player->mo->momz) / 200;

	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);

	return ringburst;
}

// This kind of wipeout happens with no rings -- doesn't remove a bumper, has no invulnerability, and is much shorter.
void K_DebtStingPlayer(player_t *player, mobj_t *source)
{
	INT32 length = TICRATE;

	if (source->player)
	{
		length += (4 * (source->player->kartweight - player->kartweight));
	}

	player->spinouttype = KSPIN_STUNG;
	player->spinouttimer = length;
	player->wipeoutslow = min(length-1, wipeoutslowtime+1);

	player->ringvisualwarning = TICRATE*2;
	player->stingfx = true;

	if (P_IsDisplayPlayer(player) && !player->exiting)
		S_StartSoundAtVolume(NULL, sfx_sting0, 200);

	P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
}

void K_GiveBumpersToPlayer(player_t *player, player_t *victim, UINT8 amount)
{
	const UINT8 oldPlayerBumpers = K_Bumpers(player);

	UINT8 tookBumpers = 0;

	while (tookBumpers < amount)
	{
		const UINT8 newbumper = (oldPlayerBumpers + tookBumpers);

		angle_t newangle, diff;
		fixed_t newx, newy;

		mobj_t *newmo;

		if (newbumper <= 1)
		{
			diff = 0;
		}
		else
		{
			diff = FixedAngle(360*FRACUNIT/newbumper);
		}

		newangle = player->mo->angle;
		newx = player->mo->x + P_ReturnThrustX(player->mo, newangle + ANGLE_180, 64*FRACUNIT);
		newy = player->mo->y + P_ReturnThrustY(player->mo, newangle + ANGLE_180, 64*FRACUNIT);

		newmo = P_SpawnMobj(newx, newy, player->mo->z, MT_BATTLEBUMPER);
		newmo->threshold = newbumper;

		if (victim)
		{
			P_SetTarget(&newmo->tracer, victim->mo);
		}

		P_SetTarget(&newmo->target, player->mo);

		newmo->angle = (diff * (newbumper-1));
		newmo->color = (victim ? victim : player)->skincolor;

		if (newbumper+1 < 2)
		{
			P_SetMobjState(newmo, S_BATTLEBUMPER3);
		}
		else if (newbumper+1 < 3)
		{
			P_SetMobjState(newmo, S_BATTLEBUMPER2);
		}
		else
		{
			P_SetMobjState(newmo, S_BATTLEBUMPER1);
		}

		tookBumpers++;
	}

	// :jartcookiedance:
	player->mo->health += tookBumpers;
}

void K_TakeBumpersFromPlayer(player_t *player, player_t *victim, UINT8 amount)
{
	amount = min(amount, K_Bumpers(victim));

	if (amount == 0)
	{
		return;
	}

	K_GiveBumpersToPlayer(player, victim, amount);

	// Play steal sound
	S_StartSound(player->mo, sfx_3db06);
}

void K_GivePointsToPlayer(player_t *player, player_t *victim, UINT8 amount)
{
	P_AddPlayerScore(player, amount);
	K_SpawnBattlePoints(player, victim, amount);
}

#define MINEQUAKEDIST 4096

// Does the proximity screen flash and quake for explosions
void K_MineFlashScreen(mobj_t *source)
{
	INT32 pnum;
	player_t *p;

	if (P_MobjWasRemoved(source))
	{
		return;
	}

	S_StartSound(source, sfx_s3k4e);
	P_StartQuakeFromMobj(18, 55 * source->scale, MINEQUAKEDIST * source->scale, source);

	// check for potential display players near the source so we can have a sick flashpal.
	for (pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		p = &players[pnum];

		if (!playeringame[pnum] || !P_IsDisplayPlayer(p))
		{
			continue;
		}

		if (R_PointToDist2(p->mo->x, p->mo->y, source->x, source->y) < source->scale * MINEQUAKEDIST)
		{
			if (!bombflashtimer && P_CheckSight(p->mo, source))
			{
				bombflashtimer = TICRATE*2;
				P_FlashPal(p, PAL_WHITE, 1);
			}
		}
	}
}

// Spawns the purely visual explosion
void K_SpawnMineExplosion(mobj_t *source, skincolornum_t color, tic_t delay)
{
	INT32 i, radius, height;
	mobj_t *smoldering = P_SpawnMobj(source->x, source->y, source->z, MT_SMOLDERING);
	mobj_t *dust;
	mobj_t *truc;
	INT32 speed, speed2;

	K_MatchGenericExtraFlags(smoldering, source);
	smoldering->tics = TICRATE*3;
	smoldering->hitlag += delay;
	radius = source->radius>>FRACBITS;
	height = source->height>>FRACBITS;

	if (!color)
		color = SKINCOLOR_KETCHUP;

	for (i = 0; i < 32; i++)
	{
		dust = P_SpawnMobj(source->x, source->y, source->z, MT_SMOKE);
		P_SetMobjState(dust, S_OPAQUESMOKE1);
		dust->angle = (ANGLE_180/16) * i;
		P_SetScale(dust, source->scale);
		dust->destscale = source->scale*10;
		dust->scalespeed = source->scale/12;
		P_InstaThrust(dust, dust->angle, FixedMul(20*FRACUNIT, source->scale));
		dust->hitlag += delay;
		dust->renderflags |= RF_DONTDRAW;

		truc = P_SpawnMobj(source->x + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->y + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->z + P_RandomRange(PR_EXPLOSION, 0, height)*FRACUNIT, MT_BOOMEXPLODE);
		K_MatchGenericExtraFlags(truc, source);
		P_SetScale(truc, source->scale);
		truc->destscale = source->scale*6;
		truc->scalespeed = source->scale/12;
		speed = FixedMul(10*FRACUNIT, source->scale)>>FRACBITS;
		truc->momx = P_RandomRange(PR_EXPLOSION, -speed, speed)*FRACUNIT;
		truc->momy = P_RandomRange(PR_EXPLOSION, -speed, speed)*FRACUNIT;
		speed = FixedMul(20*FRACUNIT, source->scale)>>FRACBITS;
		truc->momz = P_RandomRange(PR_EXPLOSION, -speed, speed)*FRACUNIT*P_MobjFlip(truc);
		if (truc->eflags & MFE_UNDERWATER)
			truc->momz = (117 * truc->momz) / 200;
		truc->color = color;
		truc->hitlag += delay;
		truc->renderflags |= RF_DONTDRAW;
	}

	for (i = 0; i < 16; i++)
	{
		dust = P_SpawnMobj(source->x + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->y + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->z + P_RandomRange(PR_EXPLOSION, 0, height)*FRACUNIT, MT_SMOKE);
		P_SetMobjState(dust, S_OPAQUESMOKE1);
		P_SetScale(dust, source->scale);
		dust->destscale = source->scale*10;
		dust->scalespeed = source->scale/12;
		dust->tics = 30;
		dust->momz = P_RandomRange(PR_EXPLOSION, FixedMul(3*FRACUNIT, source->scale)>>FRACBITS, FixedMul(7*FRACUNIT, source->scale)>>FRACBITS)*FRACUNIT;
		dust->hitlag += delay;
		dust->renderflags |= RF_DONTDRAW;

		truc = P_SpawnMobj(source->x + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->y + P_RandomRange(PR_EXPLOSION, -radius, radius)*FRACUNIT,
			source->z + P_RandomRange(PR_EXPLOSION, 0, height)*FRACUNIT, MT_BOOMPARTICLE);
		K_MatchGenericExtraFlags(truc, source);
		P_SetScale(truc, source->scale);
		truc->destscale = source->scale*5;
		truc->scalespeed = source->scale/12;
		speed = FixedMul(20*FRACUNIT, source->scale)>>FRACBITS;
		truc->momx = P_RandomRange(PR_EXPLOSION, -speed, speed)*FRACUNIT;
		truc->momy = P_RandomRange(PR_EXPLOSION, -speed, speed)*FRACUNIT;
		speed = FixedMul(15*FRACUNIT, source->scale)>>FRACBITS;
		speed2 = FixedMul(45*FRACUNIT, source->scale)>>FRACBITS;
		truc->momz = P_RandomRange(PR_EXPLOSION, speed, speed2)*FRACUNIT*P_MobjFlip(truc);
		if (P_RandomChance(PR_EXPLOSION, FRACUNIT/2))
			truc->momz = -truc->momz;
		if (truc->eflags & MFE_UNDERWATER)
			truc->momz = (117 * truc->momz) / 200;
		truc->tics = TICRATE*2;
		truc->color = color;
		truc->hitlag += delay;
		truc->renderflags |= RF_DONTDRAW;
	}

	Obj_SpawnBrolyKi(source, delay);
}

#undef MINEQUAKEDIST

void K_SpawnLandMineExplosion(mobj_t *source, skincolornum_t color, tic_t delay)
{
	mobj_t *smoldering;
	mobj_t *expl;
	UINT8 i;

	// Spawn smoke remains:
	smoldering = P_SpawnMobj(source->x, source->y, source->z, MT_SMOLDERING);
	P_SetScale(smoldering, source->scale);
	smoldering->tics = TICRATE*3;
	smoldering->hitlag = delay;

	// spawn a few physics explosions
	for (i = 0; i < 15; i++)
	{
		expl = P_SpawnMobj(source->x, source->y, source->z + source->scale, MT_BOOMEXPLODE);
		expl->color = color;
		expl->tics = (i+1);
		expl->hitlag = delay;
		expl->renderflags |= RF_DONTDRAW;

		//K_MatchGenericExtraFlags(expl, actor);
		P_SetScale(expl, source->scale*4);

		expl->momx = P_RandomRange(PR_EXPLOSION, -3, 3)*source->scale/2;
		expl->momy = P_RandomRange(PR_EXPLOSION, -3, 3)*source->scale/2;

		// 100/45 = 2.22 fu/t
		expl->momz = ((i+1)*source->scale*5/2)*P_MobjFlip(expl);
	}
}

fixed_t K_ItemScaleForPlayer(player_t *player)
{
	switch (player->itemscale)
	{
		case ITEMSCALE_GROW:
			return FixedMul(GROW_SCALE, mapobjectscale);

		case ITEMSCALE_SHRINK:
			return FixedMul(SHRINK_SCALE, mapobjectscale);

		default:
			return mapobjectscale;
	}
}

fixed_t K_DefaultPlayerRadius(player_t *player)
{
	mobj_t *top = K_GetGardenTop(player);

	if (top)
	{
		return top->radius;
	}

	return FixedMul(player->mo->scale,
			player->mo->info->radius);
}

static mobj_t *K_SpawnKartMissile(mobj_t *source, mobjtype_t type, angle_t an, INT32 flags2, fixed_t speed, SINT8 dir)
{
	mobj_t *th;
	fixed_t x, y, z;
	fixed_t topspeed = K_GetKartSpeed(source->player, false, false);
	fixed_t finalspeed = speed;
	fixed_t finalscale = mapobjectscale;
	mobj_t *throwmo;

	if (source->player != NULL)
	{
		const angle_t delta = AngleDelta(source->angle, an);
		 // Correct for angle difference when applying missile speed boosts. (Don't boost backshots!)
		const fixed_t deltaFactor = FixedDiv(AngleFixed(ANGLE_180 - delta), 180 * FRACUNIT);

		if (source->player->itemscale == ITEMSCALE_SHRINK)
		{
			// Nerf the base item speed a bit.
			speed = finalspeed = FixedMul(speed, SHRINK_PHYSICS_SCALE);
		}

		if (source->player->speed > topspeed)
		{
			// Multiply speed to be proportional to your own, boosted maxspeed.
			// (Dramatic "railgun" effect when fast players fire missiles.)
			finalspeed = max(speed, FixedMul(
				speed,
				FixedMul(
					FixedDiv(source->player->speed, topspeed),
					deltaFactor
				)
			));
		}

		// ...and add player speed on top, to make sure you're never traveling faster than an item you throw.
		finalspeed += FixedMul(source->player->speed, deltaFactor);

		finalscale = K_ItemScaleForPlayer(source->player);

		if (type == MT_GARDENTOP)
		{
			mobj_t *top = K_GetGardenTop(source->player);
			if (top)
			{
				finalscale = top->scale;
			}
		}
	}

	if (type == MT_BUBBLESHIELDTRAP)
	{
		finalscale = source->scale;
	}

	if (dir == -1)
	{
		fixed_t nerf = FRACUNIT;

		// Backwards nerfs
		switch (type)
		{
			case MT_ORBINAUT:
			case MT_GACHABOM:
				// These items orbit in place.
				// Look for a tight radius...
				nerf = FRACUNIT/4;
				break;

			case MT_BALLHOG:
				nerf = FRACUNIT/8;
				break;

			default:
				break;
		}

		if (finalspeed != FRACUNIT)
		{
			// Scale to gamespeed for consistency
			finalspeed = FixedMul(finalspeed, FixedDiv(nerf, K_GetKartGameSpeedScalar(gamespeed)));
		}
	}

	x = source->x + source->momx + FixedMul(finalspeed, FINECOSINE(an>>ANGLETOFINESHIFT));
	y = source->y + source->momy + FixedMul(finalspeed, FINESINE(an>>ANGLETOFINESHIFT));
	z = P_GetZAt(source->standingslope, x, y, source->z); // spawn on the ground please

	th = P_SpawnMobj(x, y, z, type); // this will never return null because collision isn't processed here

	K_FlipFromObject(th, source);

	th->flags2 |= flags2;
	th->threshold = 10;

	if (th->info->seesound)
		S_StartSound(source, th->info->seesound);

	P_SetTarget(&th->target, source);

	P_SetScale(th, finalscale);
	th->destscale = finalscale;

	th->angle = an;

	th->momx = FixedMul(finalspeed, FINECOSINE(an>>ANGLETOFINESHIFT));
	th->momy = FixedMul(finalspeed, FINESINE(an>>ANGLETOFINESHIFT));
	th->momz = source->momz;

	if (source->player != NULL)
	{
		th->cusval = source->player->itemscale;
	}

	switch (type)
	{
		case MT_ORBINAUT:
			Obj_OrbinautThrown(th, finalspeed, dir);
			break;
		case MT_JAWZ:
			Obj_JawzThrown(th, finalspeed, dir);
			break;
		case MT_SPB:
			Obj_SPBThrown(th, finalspeed);
			break;
		case MT_BUBBLESHIELDTRAP:
			P_SetScale(th, ((5*th->destscale)>>2)*4);
			th->destscale = (5*th->destscale)>>2;
			S_StartSound(th, sfx_s3kbfl);
			S_StartSound(th, sfx_cdfm35);
			break;
		case MT_BALLHOG:
			// Contra spread shot scale up
			th->destscale = th->destscale << 1;
			th->scalespeed = abs(th->destscale - th->scale) / (2*TICRATE);
			break;
		case MT_GARDENTOP:
			th->movefactor = finalspeed;
			break;
		case MT_GACHABOM:
			Obj_GachaBomThrown(th, finalspeed, dir);
			break;
		default:
			break;
	}

	// I'm calling P_SetOrigin to update the floorz if this
	// object can run on water. However, P_CanRunOnWater
	// requires that the object is already on the ground, so
	// floorz needs to be set beforehand too.
	th->floorz = source->floorz;
	th->ceilingz = source->ceilingz;

	// Get floorz and ceilingz
	P_SetOrigin(th, x, y, z);

	if (P_MobjWasRemoved(th))
		return NULL;

	if ((P_IsObjectFlipped(th)
			? th->ceilingz - source->ceilingz
			: th->floorz - source->floorz) > P_GetThingStepUp(th, x, y))
	{
		// Assuming this is on the boundary of a sector and
		// the wall is too tall... I'm not bothering with
		// trying to find where the line is. Just nudge this
		// object back a bit so it (hopefully) doesn't
		// teleport on top of the ledge.

		const fixed_t r = abs(th->radius - source->radius);

		x = source->x - FixedMul(r, FSIN(an));
		y = source->y - FixedMul(r, FCOS(an));
		z = P_GetZAt(source->standingslope, x, y, source->z);

		P_SetOrigin(th, x, y, z);

		if (P_MobjWasRemoved(th))
			return NULL;
	}

	if (P_IsObjectOnGround(source))
	{
		// If the player is on the ground, make sure the
		// missile spawns on the ground, so it smoothly
		// travels down stairs.

		// FIXME: This needs a more elegant solution because
		// if multiple stairs are crossed, the height
		// difference in the end is too great (this affects
		// slopes too).

		const fixed_t tz = P_IsObjectFlipped(th) ? th->ceilingz - th->height : th->floorz;

		if (abs(tz - z) <= P_GetThingStepUp(th, x, y))
		{
			z = tz;
			th->z = z;
		}
	}

	if (type != MT_BUBBLESHIELDTRAP)
	{
		x = x + P_ReturnThrustX(source, an, source->radius + th->radius);
		y = y + P_ReturnThrustY(source, an, source->radius + th->radius);
		throwmo = P_SpawnMobj(x, y, z, MT_FIREDITEM);
		throwmo->movecount = 1;
		throwmo->movedir = source->angle - an;
		P_SetTarget(&throwmo->target, source);
	}

	return th;
}

UINT16 K_DriftSparkColor(player_t *player, INT32 charge)
{
	const INT32 dsone = K_GetKartDriftSparkValueForStage(player, 1);
	const INT32 dstwo = K_GetKartDriftSparkValueForStage(player, 2);
	const INT32 dsthree = K_GetKartDriftSparkValueForStage(player, 3);
	const INT32 dsfour = K_GetKartDriftSparkValueForStage(player, 4);

	UINT16 color = SKINCOLOR_NONE;

	if (charge < 0)
	{
		// Stage 0: Grey
		color = SKINCOLOR_SILVER;
	}
	else if (charge >= dsfour)
	{
		// Stage 4: Rainbow
		if (charge <= dsfour+(32*3))
		{
			// transition
			color = SKINCOLOR_SILVER;
		}
		else
		{
			color = K_RainbowColor(leveltime);
		}
	}
	else if (charge >= dsthree)
	{
		// Stage 3: Blue
		if (charge <= dsthree+(16*3))
		{
			// transition 1
			color = SKINCOLOR_TAFFY;
		}
		else if (charge <= dsthree+(32*3))
		{
			// transition 2
			color = SKINCOLOR_NOVA;
		}
		else
		{
			color = SKINCOLOR_BLUE;
		}
	}
	else if (charge >= dstwo)
	{
		// Stage 2: Red
		if (charge <= dstwo+(32*3))
		{
			// transition
			color = SKINCOLOR_TANGERINE;
		}
		else
		{
			color = SKINCOLOR_KETCHUP;
		}
	}
	else if (charge >= dsone)
	{
		// Stage 1: Yellow
		if (charge <= dsone+(32*3))
		{
			// transition
			color = SKINCOLOR_TAN;
		}
		else
		{
			color = SKINCOLOR_GOLD;
		}
	}

	return color;
}

static void K_SpawnDriftElectricity(player_t *player)
{
	UINT8 i;
	UINT16 color = K_DriftSparkColor(player, player->driftcharge);
	mobj_t *mo = player->mo;
	fixed_t vr = FixedDiv(mo->radius/3, mo->scale); // P_SpawnMobjFromMobj will rescale
	fixed_t horizontalradius = FixedDiv(5*mo->radius/3, mo->scale);
	angle_t verticalangle = K_MomentumAngle(mo) + ANGLE_180; // points away from the momentum angle

	for (i = 0; i < 2; i++)
	{
		// i == 0 is right, i == 1 is left
		mobj_t *spark;
		angle_t horizonatalangle = verticalangle + (i ? ANGLE_90 : ANGLE_270);
		angle_t sparkangle = verticalangle + ANGLE_180;
		fixed_t verticalradius = vr; // local version of the above so we can modify it
		fixed_t scalefactor = 0; // positive values enlarge sparks, negative values shrink them
		fixed_t x, y;

		if (player->drift == 0)
			; // idk what you're doing spawning drift sparks when you're not drifting but you do you
		else
		{
			scalefactor = -(2*i - 1) * min(max(player->steering, -1), 1) * FRACUNIT;
			if ((player->drift > 0) == !(i)) // inwards spark should be closer to the player
				verticalradius = 0;
		}

		x = P_ReturnThrustX(mo, verticalangle, verticalradius)
			+ P_ReturnThrustX(mo, horizonatalangle, horizontalradius);
		y = P_ReturnThrustY(mo, verticalangle, verticalradius)
			+ P_ReturnThrustY(mo, horizonatalangle, horizontalradius);
		spark = P_SpawnMobjFromMobj(mo, x, y, 0, MT_DRIFTELECTRICITY);
		spark->angle = sparkangle;
		spark->momx = mo->momx;
		spark->momy = mo->momy;
		spark->momz = mo->momz;
		spark->color = color;
		K_GenericExtraFlagsNoZAdjust(spark, mo);
		P_SetTarget(&spark->owner, mo);
		spark->renderflags |= RF_REDUCEVFX;

		spark->spritexscale += scalefactor/3;
		spark->spriteyscale += scalefactor/8;
	}
}

void K_SpawnDriftElectricSparks(player_t *player, int color, boolean shockwave)
{
	SINT8 hdir, vdir, i;
	int shockscale = shockwave ? 2 : 1;

	mobj_t *mo = player->mo;
	angle_t momangle = K_MomentumAngle(mo) + ANGLE_180;
	fixed_t radius = 2 * FixedDiv(mo->radius, mo->scale); // P_SpawnMobjFromMobj will rescale
	fixed_t x = P_ReturnThrustX(mo, momangle, radius);
	fixed_t y = P_ReturnThrustY(mo, momangle, radius);
	fixed_t z = FixedDiv(mo->height, 2 * mo->scale); // P_SpawnMobjFromMobj will rescale

	fixed_t sparkspeed = mobjinfo[MT_DRIFTELECTRICSPARK].speed;
	fixed_t sparkradius = 2 * shockscale * mobjinfo[MT_DRIFTELECTRICSPARK].radius;

	if (player->trickcharge && !shockwave)
	{
		mobj_t *release = P_SpawnMobjFromMobj(mo, 0, 0, 0, MT_CHARGERELEASE);
		P_SetTarget(&release->target, mo);
		release->tics = 40;
		release->scale /= 5;
		release->destscale *= 2;
		release->scalespeed = release->scale/2;
	}

	for (hdir = -1; hdir <= 1; hdir += 2)
	{
		for (vdir = -1; vdir <= 1; vdir += 2)
		{
			fixed_t hspeed = FixedMul(hdir * sparkspeed, mo->scale); // P_InstaThrust treats speed as absolute
			fixed_t vspeed = vdir * sparkspeed; // P_SetObjectMomZ scales speed with object scale
			angle_t sparkangle = mo->angle + ANGLE_45;
			mobj_t *spark;

			for (i = 0; i < 4; i++)
			{
				fixed_t xoff = P_ReturnThrustX(mo, sparkangle, sparkradius);
				fixed_t yoff = P_ReturnThrustY(mo, sparkangle, sparkradius);
				if (player->trickcharge && !shockwave)
					spark = P_SpawnMobjFromMobj(mo, x + xoff, y + yoff, z, MT_CHARGEEXTRA);
				else
					spark = P_SpawnMobjFromMobj(mo, x + xoff, y + yoff, z, MT_DRIFTELECTRICSPARK);

				spark->angle = sparkangle;
				spark->color = color;
				P_InstaThrust(spark, mo->angle + ANGLE_90, hspeed);
				P_SetObjectMomZ(spark, vspeed, false);
				spark->momx += mo->momx; // copy player speed
				spark->momy += mo->momy;
				spark->momz += P_GetMobjZMovement(mo);
				spark->destscale = shockscale * spark->scale;
				P_SetScale(spark, shockscale * spark->scale);

				if (shockwave)
				{
					spark->frame |= FF_ADD;
				}
				else if (player->trickcharge)
				{
					spark->tics = 20;
				}


				sparkangle += ANGLE_90;
				P_SetTarget(&spark->owner, mo);
				spark->renderflags |= RF_REDUCEVFX;
			}
		}
	}
	S_ReducedVFXSound(mo, sfx_s3k45, player);
}

static void K_SpawnDriftSparks(player_t *player)
{
	const INT32 dsone = K_GetKartDriftSparkValueForStage(player, 1);
	const INT32 dstwo = K_GetKartDriftSparkValueForStage(player, 2);
	const INT32 dsthree = K_GetKartDriftSparkValueForStage(player, 3);
	const INT32 dsfour = K_GetKartDriftSparkValueForStage(player, 4);

	fixed_t newx;
	fixed_t newy;
	mobj_t *spark;
	angle_t travelangle;
	INT32 i;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (leveltime % 2 == 1)
		return;

	if (!player->drift
		|| (player->driftcharge < dsone && !(player->driftcharge < 0)))
		return;

	travelangle = player->mo->angle-(ANGLE_45/5)*player->drift;

	for (i = 0; i < 2; i++)
	{
		SINT8 size = 1;
		UINT8 trail = 0;

		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_135, FixedMul(32*FRACUNIT, player->mo->scale));
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_135, FixedMul(32*FRACUNIT, player->mo->scale));
		spark = P_SpawnMobj(newx, newy, player->mo->z, MT_DRIFTSPARK);

		P_SetTarget(&spark->target, player->mo);
		spark->angle = travelangle-((ANGLE_45/5)*player->drift);
		spark->destscale = player->mo->scale;
		P_SetScale(spark, player->mo->scale);

		spark->momx = player->mo->momx/2;
		spark->momy = player->mo->momy/2;
		spark->momz = P_GetMobjZMovement(player->mo)/2;

		spark->color = K_DriftSparkColor(player, player->driftcharge);

		if (player->driftcharge < 0)
		{
			// Stage 0: Yellow
			size = 0;
		}
		else if (player->driftcharge >= dsfour)
		{
			// Stage 4: Rainbow
			size = 2;
			trail = 2;

			if (player->driftcharge <= (dsfour)+(32*3))
			{
				// transition
				P_SetScale(spark, (spark->destscale = spark->scale*3/2));
				S_StartSound(player->mo, sfx_cock);
			}
			else
			{
				spark->colorized = true;
			}
		}
		else if (player->driftcharge >= dsthree)
		{
			// Stage 3: Purple
			size = 2;
			trail = 1;

			if (player->driftcharge <= dsthree+(32*3))
			{
				// transition
				P_SetScale(spark, (spark->destscale = spark->scale*3/2));
			}
		}
		else if (player->driftcharge >= dstwo)
		{
			// Stage 2: Blue
			size = 2;
			trail = 1;

			if (player->driftcharge <= dstwo+(32*3))
			{
				// transition
				P_SetScale(spark, (spark->destscale = spark->scale*3/2));
			}
		}
		else
		{
			// Stage 1: Red
			size = 1;

			if (player->driftcharge <= dsone+(32*3))
			{
				// transition
				P_SetScale(spark, (spark->destscale = spark->scale*2));
			}
		}

		if ((player->drift > 0 && player->steering > 0) // Inward drifts
			|| (player->drift < 0 && player->steering < 0))
		{
			if ((player->drift < 0 && (i & 1))
				|| (player->drift > 0 && !(i & 1)))
			{
				size++;
			}
			else if ((player->drift < 0 && !(i & 1))
				|| (player->drift > 0 && (i & 1)))
			{
				size--;
			}
		}
		else if ((player->drift > 0 && player->steering < 0) // Outward drifts
			|| (player->drift < 0 && player->steering > 0))
		{
			if ((player->drift < 0 && (i & 1))
				|| (player->drift > 0 && !(i & 1)))
			{
				size--;
			}
			else if ((player->drift < 0 && !(i & 1))
				|| (player->drift > 0 && (i & 1)))
			{
				size++;
			}
		}

		if (size == 2)
			P_SetMobjState(spark, S_DRIFTSPARK_A1);
		else if (size < 1)
			P_SetMobjState(spark, S_DRIFTSPARK_C1);
		else if (size > 2)
			P_SetMobjState(spark, S_DRIFTSPARK_D1);

		if (trail > 0)
			spark->tics += trail;

		K_MatchGenericExtraFlags(spark, player->mo);
		P_SetTarget(&spark->owner, player->mo);
		spark->renderflags |= RF_REDUCEVFX;
	}

	if (player->driftcharge >= dsthree)
	{
		K_SpawnDriftElectricity(player);
	}
}

static void K_SpawnAIZDust(player_t *player)
{
	fixed_t newx;
	fixed_t newy;
	mobj_t *spark;
	angle_t travelangle;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (leveltime % 2 == 1)
		return;

	if (!P_IsObjectOnGround(player->mo))
		return;

	if (player->speed <= K_GetKartSpeed(player, false, true))
		return;

	travelangle = K_MomentumAngle(player->mo);
	//S_StartSound(player->mo, sfx_s3k47);

	{
		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle - (player->aizdriftstrat*ANGLE_45), FixedMul(24*FRACUNIT, player->mo->scale));
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle - (player->aizdriftstrat*ANGLE_45), FixedMul(24*FRACUNIT, player->mo->scale));
		spark = P_SpawnMobj(newx, newy, player->mo->z, MT_AIZDRIFTSTRAT);

		spark->angle = travelangle+(player->aizdriftstrat*ANGLE_90);
		P_SetScale(spark, (spark->destscale = (3*player->mo->scale)>>2));

		spark->momx = (6*player->mo->momx)/5;
		spark->momy = (6*player->mo->momy)/5;
		spark->momz = P_GetMobjZMovement(player->mo);

		K_MatchGenericExtraFlags(spark, player->mo);
	}
}

void K_SpawnBoostTrail(player_t *player)
{
	fixed_t newx, newy, newz;
	fixed_t ground;
	mobj_t *flame;
	angle_t travelangle;
	INT32 i;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (!P_IsObjectOnGround(player->mo)
		|| player->hyudorotimer != 0)
		return;

	if (player->mo->eflags & MFE_VERTICALFLIP)
		ground = player->mo->ceilingz;
	else
		ground = player->mo->floorz;

	if (player->drift != 0)
		travelangle = player->mo->angle;
	else
		travelangle = K_MomentumAngle(player->mo);

	for (i = 0; i < 2; i++)
	{
		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_135, FixedMul(24*FRACUNIT, player->mo->scale));
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_135, FixedMul(24*FRACUNIT, player->mo->scale));
		newz = P_GetZAt(player->mo->standingslope, newx, newy, ground);

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			newz -= FixedMul(mobjinfo[MT_SNEAKERTRAIL].height, player->mo->scale);
		}

		flame = P_SpawnMobj(newx, newy, ground, MT_SNEAKERTRAIL);

		P_SetTarget(&flame->target, player->mo);
		flame->angle = travelangle;
		flame->fuse = TICRATE*2;
		flame->destscale = player->mo->scale;
		P_SetScale(flame, player->mo->scale);
		// not K_MatchGenericExtraFlags so that a stolen sneaker can be seen
		K_FlipFromObject(flame, player->mo);

		flame->momx = 8;
		P_XYMovement(flame);
		if (P_MobjWasRemoved(flame))
			continue;

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if (flame->z + flame->height < flame->ceilingz)
				P_RemoveMobj(flame);
		}
		else if (flame->z > flame->floorz)
			P_RemoveMobj(flame);
	}
}

void K_SpawnSparkleTrail(mobj_t *mo)
{
	const INT32 rad = (mo->radius*3)/FRACUNIT;
	mobj_t *sparkle;
	UINT8 invanimnum; // Current sparkle animation number
	INT32 invtime;// Invincibility time left, in seconds
	UINT8 index = 0;
	fixed_t newx, newy, newz;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (leveltime & 2)
		index = 1;

	invtime = mo->player->invincibilitytimer/TICRATE+1;

	//CONS_Printf("%d\n", index);

	newx = mo->x + (P_RandomRange(PR_DECORATION, -rad, rad)*FRACUNIT);
	newy = mo->y + (P_RandomRange(PR_DECORATION, -rad, rad)*FRACUNIT);
	newz = mo->z + (P_RandomRange(PR_DECORATION, 0, mo->height>>FRACBITS)*FRACUNIT);

	sparkle = P_SpawnMobj(newx, newy, newz, MT_SPARKLETRAIL);

	sparkle->angle = R_PointToAngle2(mo->x, mo->y, sparkle->x, sparkle->y);

	sparkle->movefactor = R_PointToDist2(mo->x, mo->y, sparkle->x, sparkle->y);	// Save the distance we spawned away from the player.
	//CONS_Printf("movefactor: %d\n", sparkle->movefactor/FRACUNIT);

	sparkle->extravalue1 = (sparkle->z - mo->z);			// Keep track of our Z position relative to the player's, I suppose.
	sparkle->extravalue2 = P_RandomRange(PR_DECORATION, 0, 1) ? 1 : -1;	// Rotation direction?
	sparkle->cvmem = P_RandomRange(PR_DECORATION, -25, 25)*mo->scale;		// Vertical "angle"

	K_FlipFromObject(sparkle, mo);
	P_SetTarget(&sparkle->target, mo);

	sparkle->destscale = mo->destscale;
	P_SetScale(sparkle, mo->scale);

	invanimnum = (invtime >= 11) ? 11 : invtime;
	//CONS_Printf("%d\n", invanimnum);

	P_SetMobjState(sparkle, K_SparkleTrailStartStates[invanimnum][index]);

	if (mo->player->invincibilitytimer > itemtime+(2*TICRATE))
	{
		sparkle->color = mo->color;
		sparkle->colorized = true;
	}
}

void K_SpawnWipeoutTrail(mobj_t *mo)
{
	mobj_t *dust;
	angle_t aoff;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (mo->player)
		aoff = (mo->player->drawangle + ANGLE_180);
	else
		aoff = (mo->angle + ANGLE_180);

	if ((leveltime / 2) & 1)
		aoff -= ANGLE_45;
	else
		aoff += ANGLE_45;

	dust = P_SpawnMobj(mo->x + FixedMul(24*mo->scale, FINECOSINE(aoff>>ANGLETOFINESHIFT)) + (P_RandomRange(PR_DECORATION,-8,8) << FRACBITS),
		mo->y + FixedMul(24*mo->scale, FINESINE(aoff>>ANGLETOFINESHIFT)) + (P_RandomRange(PR_DECORATION,-8,8) << FRACBITS),
		mo->z, MT_WIPEOUTTRAIL);

	P_SetTarget(&dust->target, mo);
	dust->angle = K_MomentumAngle(mo);
	dust->destscale = mo->scale;
	P_SetScale(dust, mo->scale);
	K_FlipFromObject(dust, mo);
}

void K_SpawnDraftDust(mobj_t *mo)
{
	UINT8 i;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	for (i = 0; i < 2; i++)
	{
		angle_t ang, aoff;
		SINT8 sign = 1;
		UINT8 foff = 0;
		mobj_t *dust;
		boolean drifting = false;

		if (mo->player)
		{
			UINT8 leniency = (3*TICRATE)/4 + ((mo->player->kartweight-1) * (TICRATE/4));

			if (gametyperules & GTR_CLOSERPLAYERS)
				leniency *= 4;

			ang = mo->player->drawangle;

			if (mo->player->drift != 0)
			{
				drifting = true;
				ang += (mo->player->drift * ((ANGLE_270 + ANGLE_22h) / 5)); // -112.5 doesn't work. I fucking HATE SRB2 angles
				if (mo->player->drift < 0)
					sign = 1;
				else
					sign = -1;
			}

			foff = 5 - ((mo->player->draftleeway * 5) / leniency);

			// this shouldn't happen
			if (foff > 4)
				foff = 4;
		}
		else
			ang = mo->angle;

		if (!drifting)
		{
			if (i & 1)
				sign = -1;
			else
				sign = 1;
		}

		aoff = (ang + ANGLE_180) + (ANGLE_45 * sign);

		dust = P_SpawnMobj(mo->x + FixedMul(24*mo->scale, FINECOSINE(aoff>>ANGLETOFINESHIFT)),
			mo->y + FixedMul(24*mo->scale, FINESINE(aoff>>ANGLETOFINESHIFT)),
			mo->z, MT_DRAFTDUST);

		P_SetMobjState(dust, S_DRAFTDUST1 + foff);

		P_SetTarget(&dust->target, mo);
		dust->angle = ang - (ANGLE_90 * sign); // point completely perpendicular from the player
		dust->destscale = mo->scale;
		P_SetScale(dust, mo->scale);
		K_FlipFromObject(dust, mo);

		if (leveltime & 1)
			dust->tics++; // "randomize" animation

		dust->momx = (4*mo->momx)/5;
		dust->momy = (4*mo->momy)/5;
		dust->momz = (4*P_GetMobjZMovement(mo))/5;

		P_Thrust(dust, dust->angle, 4*mo->scale);

		if (drifting) // only 1 trail while drifting
			break;
	}
}

//	K_DriftDustHandling
//	Parameters:
//		spawner: The map object that is spawning the drift dust
//	Description: Spawns the drift dust for objects, players use rmomx/y, other objects use regular momx/y.
//		Also plays the drift sound.
//		Other objects should be angled towards where they're trying to go so they don't randomly spawn dust
//		Do note that most of the function won't run in odd intervals of frames
void K_DriftDustHandling(mobj_t *spawner)
{
	angle_t anglediff;
	const INT16 spawnrange = spawner->radius >> FRACBITS;

	if (!P_IsObjectOnGround(spawner) || leveltime % 2 != 0)
		return;

	if (spawner->player)
	{
		if (spawner->player->pflags & PF_FAULT)
		{
			anglediff = abs((signed)(spawner->angle - spawner->player->drawangle));
			if (leveltime % 6 == 0)
				S_StartSound(spawner, sfx_screec); // repeated here because it doesn't always happen to be within the range when this is the case
		}
		else
		{
			angle_t playerangle = spawner->angle;

			if (spawner->player->speed < 5*spawner->scale)
				return;

			if (K_GetForwardMove(spawner->player) < 0)
				playerangle += ANGLE_180;

			anglediff = abs((signed)(playerangle - R_PointToAngle2(0, 0, spawner->player->rmomx, spawner->player->rmomy)));
		}
	}
	else
	{
		if (P_AproxDistance(spawner->momx, spawner->momy) < 5*spawner->scale)
			return;

		anglediff = abs((signed)(spawner->angle - K_MomentumAngle(spawner)));
	}

	if (anglediff > ANGLE_180)
		anglediff = InvAngle(anglediff);

	if (anglediff > ANG10*4) // Trying to turn further than 40 degrees
	{
		fixed_t spawnx = P_RandomRange(PR_DECORATION, -spawnrange, spawnrange) << FRACBITS;
		fixed_t spawny = P_RandomRange(PR_DECORATION, -spawnrange, spawnrange) << FRACBITS;
		INT32 speedrange = 2;
		mobj_t *dust = P_SpawnMobj(spawner->x + spawnx, spawner->y + spawny, spawner->z, MT_DRIFTDUST);
		dust->momx = FixedMul(spawner->momx + (P_RandomRange(PR_DECORATION, -speedrange, speedrange) * spawner->scale), 3*FRACUNIT/4);
		dust->momy = FixedMul(spawner->momy + (P_RandomRange(PR_DECORATION, -speedrange, speedrange) * spawner->scale), 3*FRACUNIT/4);
		dust->momz = P_MobjFlip(spawner) * (P_RandomRange(PR_DECORATION, 1, 4) * (spawner->scale));
		P_SetScale(dust, spawner->scale/2);
		dust->destscale = spawner->scale * 3;
		dust->scalespeed = spawner->scale/12;

		if (!spawner->player || !K_GetGardenTop(spawner->player))
		{
			if (leveltime % 6 == 0)
				S_StartSound(spawner, sfx_screec);
		}

		K_MatchGenericExtraFlags(dust, spawner);

		// Sparkle-y warning for when you're about to change drift sparks!
		if (spawner->player && spawner->player->drift)
		{
			INT32 driftval = K_GetKartDriftSparkValue(spawner->player);
			INT32 warntime = driftval/3;
			INT32 dc = spawner->player->driftcharge;
			UINT8 c = SKINCOLOR_NONE;
			boolean rainbow = false;

			if (dc >= 0)
			{
				dc += warntime;
			}

			c = K_DriftSparkColor(spawner->player, dc);

			if (dc > (4*driftval)+(32*3))
			{
				rainbow = true;
			}

			if (c != SKINCOLOR_NONE)
			{
				P_SetMobjState(dust, S_DRIFTWARNSPARK1);
				dust->color = c;
				dust->colorized = rainbow;
			}
		}
	}
}

void K_Squish(mobj_t *mo)
{
	const fixed_t maxstretch = 4*FRACUNIT;
	const fixed_t factor = 5 * mo->height / 4;
	const fixed_t threshold = factor / 6;

	fixed_t old3dspeed = abs(mo->lastmomz);
	fixed_t new3dspeed = abs(mo->momz);

	fixed_t delta = abs(old3dspeed - new3dspeed);
	fixed_t grav = mo->height/3;
	fixed_t add = abs(grav - new3dspeed);

	if (R_ThingIsFloorSprite(mo))
		return;

	if (delta < 2 * add && new3dspeed > grav)
		delta += add;

	if (delta > threshold)
	{
		mo->spritexscale =
			FRACUNIT + FixedDiv(delta, factor);

		if (mo->spritexscale > maxstretch)
			mo->spritexscale = maxstretch;

		if (new3dspeed > old3dspeed || new3dspeed > grav)
		{
			mo->spritexscale =
				FixedDiv(FRACUNIT, mo->spritexscale);
		}
	}
	else
	{
		mo->spritexscale -=
			(mo->spritexscale - FRACUNIT)
			/ (mo->spritexscale < FRACUNIT ? 8 : 3);
	}

	mo->spriteyscale =
		FixedDiv(FRACUNIT, mo->spritexscale);

	if (cv_bighead.value && (mo->type == MT_PLAYER || (!P_MobjWasRemoved(mo->target) && mo->target->type == MT_PLAYER)))
		mo->spriteyscale *= 2;
}

static mobj_t *K_FindLastTrailMobj(player_t *player)
{
	mobj_t *trail;

	if (!player || !(trail = player->mo) || !player->mo->hnext || !player->mo->hnext->health)
		return NULL;

	while (trail->hnext && !P_MobjWasRemoved(trail->hnext) && trail->hnext->health)
	{
		trail = trail->hnext;
	}

	return trail;
}

mobj_t *K_ThrowKartItem(player_t *player, boolean missile, mobjtype_t mapthing, INT32 defaultDir, INT32 altthrow, angle_t angleOffset)
{
	mobj_t *mo;
	INT32 dir;
	fixed_t PROJSPEED;
	angle_t newangle;
	fixed_t newx, newy, newz;
	mobj_t *throwmo;

	if (!player)
		return NULL;

	if (altthrow)
	{
		if (altthrow == 2) // Kitchen sink throwing
		{
#if 0
			if (player->throwdir == 1)
				dir = 3;
			else if (player->throwdir == -1)
				dir = 1;
			else
				dir = 2;
#else
			if (player->throwdir == 1)
				dir = 2;
			else
				dir = 1;
#endif
		}
		else
		{
			if (player->throwdir == 1)
				dir = 2;
			else if (player->throwdir == -1)
				dir = -1;
			else
				dir = 1;
		}
	}
	else
	{
		if (player->throwdir != 0)
			dir = player->throwdir;
		else
			dir = defaultDir;
	}

	if (mapthing == MT_GACHABOM && dir > 0)
	{
		// This item is both a missile and not!
		missile = false;
	}

	// Figure out projectile speed by game speed
	if (missile)
	{
		// Use info->speed for missiles
		PROJSPEED = FixedMul(mobjinfo[mapthing].speed, K_GetKartGameSpeedScalar(gamespeed));
	}
	else
	{
		// Use pre-determined speed for tossing
		PROJSPEED = FixedMul(82 * FRACUNIT, K_GetKartGameSpeedScalar(gamespeed));
	}

	// Scale to map scale
	// Intentionally NOT player scale, that doesn't work.
	PROJSPEED = FixedMul(PROJSPEED, mapobjectscale);

	if (missile) // Shootables
	{
		if (dir < 0 && mapthing != MT_SPB && mapthing != MT_GARDENTOP)
		{
			// Shoot backward
			mo = K_SpawnKartMissile(player->mo, mapthing, (player->mo->angle + ANGLE_180) + angleOffset, 0, PROJSPEED, dir);
		}
		else
		{
			// Shoot forward
			mo = K_SpawnKartMissile(player->mo, mapthing, player->mo->angle + angleOffset, 0, PROJSPEED, dir);
		}

		if (mapthing == MT_DROPTARGET && mo)
		{
			mo->health++;
			mo->color = SKINCOLOR_WHITE;
			mo->reactiontime = TICRATE/2;
			P_SetMobjState(mo, mo->info->painstate);
		}
	}
	else
	{
		fixed_t finalscale = K_ItemScaleForPlayer(player);

		player->bananadrag = 0; // RESET timer, for multiple bananas

		if (dir > 0)
		{
			// Shoot forward
			mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height/2, mapthing);
			mo->angle = player->mo->angle;

			// These are really weird so let's make it a very specific case to make SURE it works...
			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				mo->z -= player->mo->height;
				mo->eflags |= MFE_VERTICALFLIP;
				mo->flags2 |= (player->mo->flags2 & MF2_OBJECTFLIP);
			}

			mo->threshold = 10;
			P_SetTarget(&mo->target, player->mo);

			S_StartSound(player->mo, mo->info->seesound);

			{
				angle_t fa = player->mo->angle>>ANGLETOFINESHIFT;
				fixed_t HEIGHT = ((20 + (dir*10)) * FRACUNIT) + (FixedDiv(player->mo->momz, mapobjectscale)*P_MobjFlip(player->mo)); // Also intentionally not player scale

				P_SetObjectMomZ(mo, HEIGHT, false);
				mo->momx = player->mo->momx + FixedMul(FINECOSINE(fa), PROJSPEED*dir);
				mo->momy = player->mo->momy + FixedMul(FINESINE(fa), PROJSPEED*dir);
			}

			mo->extravalue2 = dir;

			if (mo->eflags & MFE_UNDERWATER)
				mo->momz = (117 * mo->momz) / 200;

			P_SetScale(mo, finalscale);
			mo->destscale = finalscale;

			if (mapthing == MT_BANANA)
			{
				mo->angle = FixedAngle(P_RandomRange(PR_DECORATION, -180, 180) << FRACBITS);
				mo->rollangle = FixedAngle(P_RandomRange(PR_DECORATION, -180, 180) << FRACBITS);
			}

			if (mapthing == MT_GACHABOM)
			{
				Obj_GachaBomThrown(mo, mo->radius, dir);
			}

			// this is the small graphic effect that plops in you when you throw an item:
			throwmo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height/2, MT_FIREDITEM);
			P_SetTarget(&throwmo->target, player->mo);
			// Ditto:
			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				throwmo->z -= player->mo->height;
				throwmo->eflags |= MFE_VERTICALFLIP;
				mo->flags2 |= (player->mo->flags2 & MF2_OBJECTFLIP);
			}

			throwmo->movecount = 0; // above player

			P_SetScale(throwmo, finalscale);
			throwmo->destscale = finalscale;
		}
		else
		{
			mobj_t *lasttrail = K_FindLastTrailMobj(player);

			if (mapthing == MT_BUBBLESHIELDTRAP) // Drop directly on top of you.
			{
				newangle = player->mo->angle;
				newx = player->mo->x + player->mo->momx;
				newy = player->mo->y + player->mo->momy;
				newz = player->mo->z;
			}
			else if (lasttrail)
			{
				newangle = lasttrail->angle;
				newx = lasttrail->x;
				newy = lasttrail->y;
				newz = lasttrail->z;
			}
			else
			{
				// Drop it directly behind you.
				fixed_t dropradius = FixedHypot(player->mo->radius, player->mo->radius) + FixedHypot(mobjinfo[mapthing].radius, mobjinfo[mapthing].radius);

				newangle = player->mo->angle;

				newx = player->mo->x + P_ReturnThrustX(player->mo, newangle + ANGLE_180, dropradius);
				newy = player->mo->y + P_ReturnThrustY(player->mo, newangle + ANGLE_180, dropradius);
				newz = player->mo->z;
			}

			mo = P_SpawnMobj(newx, newy, newz, mapthing); // this will never return null because collision isn't processed here
			K_FlipFromObject(mo, player->mo);

			mo->threshold = 10;
			P_SetTarget(&mo->target, player->mo);

			P_SetScale(mo, finalscale);
			mo->destscale = finalscale;

			if (P_IsObjectOnGround(player->mo))
			{
				// floorz and ceilingz aren't properly set to account for FOFs and Polyobjects on spawn
				// This should set it for FOFs
				P_SetOrigin(mo, mo->x, mo->y, mo->z); // however, THIS can fuck up your day. just absolutely ruin you.
				if (P_MobjWasRemoved(mo))
					return NULL;

				if (P_MobjFlip(mo) > 0)
				{
					if (mo->floorz > mo->target->z - mo->height)
					{
						mo->z = mo->floorz;
					}
				}
				else
				{
					if (mo->ceilingz < mo->target->z + mo->target->height + mo->height)
					{
						mo->z = mo->ceilingz - mo->height;
					}
				}
			}

			if (player->mo->eflags & MFE_VERTICALFLIP)
				mo->eflags |= MFE_VERTICALFLIP;

			mo->angle = newangle;

			if (mapthing == MT_SSMINE)
				mo->extravalue1 = 49; // Pads the start-up length from 21 frames to a full 2 seconds
			else if (mapthing == MT_BUBBLESHIELDTRAP)
			{
				P_SetScale(mo, ((5*mo->destscale)>>2)*4);
				mo->destscale = (5*mo->destscale)>>2;
				S_StartSound(mo, sfx_s3kbfl);
			}
		}
	}

	return mo;
}

void K_PuntMine(mobj_t *origMine, mobj_t *punter)
{
	angle_t fa = K_MomentumAngle(punter);
	fixed_t z = (punter->momz * P_MobjFlip(punter)) + (30 * FRACUNIT);
	fixed_t spd;
	mobj_t *mine;

	if (!origMine || P_MobjWasRemoved(origMine))
		return;

	if (punter->hitlag > 0)
		return;

	// This guarantees you hit a mine being dragged
	if (origMine->type == MT_SSMINE_SHIELD) // Create a new mine, and clean up the old one
	{
		mobj_t *mineOwner = origMine->target;

		mine = P_SpawnMobj(origMine->x, origMine->y, origMine->z, MT_SSMINE);

		P_SetTarget(&mine->target, mineOwner);

		mine->angle = origMine->angle;
		mine->flags2 = origMine->flags2;
		mine->floorz = origMine->floorz;
		mine->ceilingz = origMine->ceilingz;

		P_SetScale(mine, origMine->scale);
		mine->destscale = origMine->destscale;
		mine->scalespeed = origMine->scalespeed;

		// Copy interp data
		mine->old_angle = origMine->old_angle;
		mine->old_x = origMine->old_x;
		mine->old_y = origMine->old_y;
		mine->old_z = origMine->old_z;

		// Since we aren't using P_KillMobj, we need to clean up the hnext reference
		P_SetTarget(&mineOwner->hnext, NULL);
		K_UnsetItemOut(mineOwner->player);

		if (mineOwner->player->itemamount)
		{
			mineOwner->player->itemamount--;
		}

		if (!mineOwner->player->itemamount)
		{
			mineOwner->player->itemtype = KITEM_NONE;
		}

		P_RemoveMobj(origMine);
	}
	else
	{
		mine = origMine;
	}

	if (!mine || P_MobjWasRemoved(mine))
		return;

	if (mine->threshold > 0 || mine->hitlag > 0)
		return;

	spd = FixedMul(82 * punter->scale, K_GetKartGameSpeedScalar(gamespeed)); // Avg Speed is 41 in Normal

	mine->flags |= (MF_NOCLIP|MF_NOCLIPTHING);

	P_SetMobjState(mine, S_SSMINE_AIR1);
	mine->threshold = 10;
	mine->reactiontime = mine->info->reactiontime;

	mine->momx = punter->momx + FixedMul(FINECOSINE(fa >> ANGLETOFINESHIFT), spd);
	mine->momy = punter->momy + FixedMul(FINESINE(fa >> ANGLETOFINESHIFT), spd);
	P_SetObjectMomZ(mine, z, false);

	//K_SetHitLagForObjects(punter, mine, mine->target, 5);

	mine->flags &= ~(MF_NOCLIP|MF_NOCLIPTHING);
}

#define THUNDERRADIUS 320

// Rough size of the outer-rim sprites, after scaling.
// (The hitbox is already pretty strict due to only 1 active frame,
// we don't need to have it disjointedly small too...)
#define THUNDERSPRITE 80

static void K_DoLightningShield(player_t *player)
{
	mobj_t *mo;
	int i = 0;
	fixed_t sx;
	fixed_t sy;
	angle_t an;

	S_StartSound(player->mo, sfx_zio3);
	K_LightningShieldAttack(player->mo, (THUNDERRADIUS + THUNDERSPRITE) * FRACUNIT);

	// spawn vertical bolt
	mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_THOK);
	P_SetTarget(&mo->target, player->mo);
	P_SetMobjState(mo, S_LZIO11);
	mo->color = SKINCOLOR_TEAL;
	mo->scale = player->mo->scale*3 + (player->mo->scale/2);

	mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_THOK);
	P_SetTarget(&mo->target, player->mo);
	P_SetMobjState(mo, S_LZIO21);
	mo->color = SKINCOLOR_CYAN;
	mo->scale = player->mo->scale*3 + (player->mo->scale/2);

	// spawn horizontal bolts;
	for (i=0; i<7; i++)
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_THOK);
		mo->angle = P_RandomRange(PR_DECORATION, 0, 359)*ANG1;
		mo->fuse = P_RandomRange(PR_DECORATION, 20, 50);
		P_SetTarget(&mo->target, player->mo);
		P_SetMobjState(mo, S_KLIT1);
	}

	// spawn the radius thing:
	an = ANGLE_22h;
	for (i=0; i<15; i++)
	{
		sx = player->mo->x + FixedMul((player->mo->scale*THUNDERRADIUS), FINECOSINE((an*i)>>ANGLETOFINESHIFT));
		sy = player->mo->y + FixedMul((player->mo->scale*THUNDERRADIUS), FINESINE((an*i)>>ANGLETOFINESHIFT));
		mo = P_SpawnMobj(sx, sy, player->mo->z, MT_THOK);
		mo->angle = an*i;
		mo->extravalue1 = THUNDERRADIUS;	// Used to know whether we should teleport by radius or something.
		mo->scale = player->mo->scale*3;
		P_SetTarget(&mo->target, player->mo);
		P_SetMobjState(mo, S_KSPARK1);
	}
}

#undef THUNDERRADIUS
#undef THUNDERSPRITE

static void K_FlameDashLeftoverSmoke(mobj_t *src)
{
	UINT8 i;

	for (i = 0; i < 2; i++)
	{
		mobj_t *smoke = P_SpawnMobj(src->x, src->y, src->z+(8<<FRACBITS), MT_BOOSTSMOKE);

		P_SetScale(smoke, src->scale);
		smoke->destscale = 3*src->scale/2;
		smoke->scalespeed = src->scale/12;

		smoke->momx = 3*src->momx/4;
		smoke->momy = 3*src->momy/4;
		smoke->momz = 3*P_GetMobjZMovement(src)/4;

		P_Thrust(smoke, src->angle + FixedAngle(P_RandomRange(PR_DECORATION, 135, 225)<<FRACBITS), P_RandomRange(PR_DECORATION, 0, 8) * src->scale);
		smoke->momz += P_RandomRange(PR_DECORATION, 0, 4) * src->scale;
	}
}

void K_DoSneaker(player_t *player, INT32 type)
{
	const fixed_t intendedboost = FRACUNIT/2;

	if (player->roundconditions.touched_sneakerpanel == false
		&& !(player->exiting || (player->pflags & PF_NOCONTEST))
		&& player->floorboost != 0)
	{
		player->roundconditions.touched_sneakerpanel = true;
		player->roundconditions.checkthisframe = true;
	}

	if (player->floorboost == 0 || player->floorboost == 3)
	{
		const sfxenum_t normalsfx = sfx_cdfm01;
		const sfxenum_t smallsfx = sfx_cdfm40;
		sfxenum_t sfx = normalsfx;

		if (player->numsneakers)
		{
			// Use a less annoying sound when stacking sneakers.
			sfx = smallsfx;
		}

		S_StopSoundByID(player->mo, normalsfx);
		S_StopSoundByID(player->mo, smallsfx);
		S_StartSound(player->mo, sfx);

		K_SpawnDashDustRelease(player);
		if (intendedboost > player->speedboost)
			player->karthud[khud_destboostcam] = FixedMul(FRACUNIT, FixedDiv((intendedboost - player->speedboost), intendedboost));

		player->numsneakers++;
	}

	if (player->sneakertimer == 0)
	{
		if (type == 2)
		{
			if (player->mo->hnext)
			{
				mobj_t *cur = player->mo->hnext;
				while (cur && !P_MobjWasRemoved(cur))
				{
					if (!cur->tracer)
					{
						mobj_t *overlay = P_SpawnMobj(cur->x, cur->y, cur->z, MT_BOOSTFLAME);
						P_SetTarget(&overlay->target, cur);
						P_SetTarget(&cur->tracer, overlay);
						P_SetScale(overlay, (overlay->destscale = 3*cur->scale/4));
						K_FlipFromObject(overlay, cur);
					}
					cur = cur->hnext;
				}
			}
		}
		else
		{
			mobj_t *overlay = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BOOSTFLAME);
			P_SetTarget(&overlay->target, player->mo);
			P_SetScale(overlay, (overlay->destscale = player->mo->scale));
			K_FlipFromObject(overlay, player->mo);
		}
	}

	player->sneakertimer = sneakertime;

	// set angle for spun out players:
	player->boostangle = player->mo->angle;
}

static void K_DoShrink(player_t *user)
{
	S_StartSound(user->mo, sfx_kc46); // Sound the BANG!

	Obj_CreateShrinkPohbees(user);

#if 0
	{
		mobj_t *mobj, *next;

		// kill everything in the kitem list while we're at it:
		for (mobj = trackercap; mobj; mobj = next)
		{
			next = mobj->itnext;

			if (mobj->type == MT_SPB
				|| mobj->type == MT_BATTLECAPSULE
				|| mobj->type == MT_CDUFO)
			{
				continue;
			}

			// check if the item is being held by a player behind us before removing it.
			// check if the item is a "shield" first, bc i'm p sure thrown items keep the player that threw em as target anyway

			if (mobj->type == MT_BANANA_SHIELD || mobj->type == MT_JAWZ_SHIELD ||
			mobj->type == MT_SSMINE_SHIELD || mobj->type == MT_EGGMANITEM_SHIELD ||
			mobj->type == MT_SINK_SHIELD || mobj->type == MT_ORBINAUT_SHIELD ||
			mobj->type == MT_DROPTARGET_SHIELD)
			{
				if (mobj->target && mobj->target->player)
				{
					if (mobj->target->player->position > user->position)
						continue; // this guy's behind us, don't take his stuff away!
				}
			}

			mobj->destscale = 0;
			mobj->flags &= ~(MF_SOLID|MF_SHOOTABLE|MF_SPECIAL);
			mobj->flags |= MF_NOCLIPTHING; // Just for safety
		}
	}
#endif
}

void K_DoPogoSpring(mobj_t *mo, fixed_t vertispeed, UINT8 sound)
{
	fixed_t thrust = 0;
	boolean dontapplymomz = false;

	if (mo->player && mo->player->spectator)
		return;

	if (mo->eflags & MFE_SPRUNG)
		return;

	mo->standingslope = NULL;
	mo->terrain = NULL;

	mo->eflags |= MFE_SPRUNG;

	if (vertispeed == 0)
	{
		vertispeed = P_AproxDistance(mo->momx, mo->momy);
		vertispeed = FixedMul(vertispeed, FINESINE(ANGLE_22h >> ANGLETOFINESHIFT));
	}
	else if (vertispeed < 0)
	{
		dontapplymomz = 0;
		vertispeed = -vertispeed;
	}

	thrust = vertispeed * P_MobjFlip(mo);

	if (mo->player)
	{
		if (!P_PlayerInPain(mo->player))
		{
			mo->player->trickpanel = TRICKSTATE_READY;
			mo->player->pflags |= PF_TRICKDELAY;

			if (P_MobjWasRemoved(mo->player->trickIndicator) == false)
			{
				mobj_t *trickIndicator = mo->player->trickIndicator;

				P_SetScale(trickIndicator,
					trickIndicator->destscale
					= trickIndicator->old_scale
					= trickIndicator->old_scale2
						= mo->scale/4);
				trickIndicator->rollangle = 0;

				static const skincolornum_t trick_colors[] = {
					SKINCOLOR_WHITE,       // trickPanel == 1 -- was SKINCOLOR_GREY
					SKINCOLOR_TAN,
					SKINCOLOR_YELLOW,      // trickPanel == 2
					SKINCOLOR_TANGERINE,
					SKINCOLOR_KETCHUP,     // trickPanel == 3
					SKINCOLOR_MOONSET,
					SKINCOLOR_ULTRAMARINE, // trickPanel == 4
				};
				static const UINT8 numColors = sizeof(trick_colors) / sizeof(skincolornum_t);

				const fixed_t step = 8*FRACUNIT;
				fixed_t trickcol = ((vertispeed - (step/2)) / step) - 1;
				if (trickcol < 0)
					trickcol = 0;
				trickIndicator->color = trick_colors[min(trickcol, numColors - 1)];

				P_SetMobjState(trickIndicator, S_TRICKINDICATOR_UNDERLAY);

				if (P_MobjWasRemoved(trickIndicator->tracer) == false)
				{
					P_SetScale(trickIndicator->tracer,
						trickIndicator->tracer->destscale
						= trickIndicator->tracer->old_scale
						= trickIndicator->tracer->old_scale2
							= trickIndicator->destscale);
					trickIndicator->tracer->rollangle = 0;

					trickIndicator->tracer->color = mo->player->trickIndicator->color;

					P_SetMobjState(trickIndicator->tracer, S_TRICKINDICATOR_OVERLAY);
				}
			}
		}

		if (mo->player->sneakertimer || mo->player->invincibilitytimer)
		{
			thrust = FixedMul(thrust, (3*FRACUNIT)/2);
		}

		mo->player->tricktime = 0; // Reset post-hitlag timer
		// Setup the boost for potential upwards trick, at worse, make it your regular max speed. (boost = curr speed*1.25)
		mo->player->trickboostpower = max(FixedDiv(mo->player->speed, K_GetKartSpeed(mo->player, false, false)) - FRACUNIT, 0)*125/100;
		mo->player->trickboostpower = FixedDiv(mo->player->trickboostpower, K_GrowShrinkSpeedMul(mo->player));
		//CONS_Printf("Got boost: %d%\n", mo->player->trickboostpower*100 / FRACUNIT);
		mo->player->fastfall = 0;
	}

	if (dontapplymomz == false)
	{
		mo->momz = FixedMul(thrust, mapobjectscale);

		if (mo->eflags & MFE_UNDERWATER)
		{
			mo->momz = FixedDiv(mo->momz, FixedSqrt(3*FRACUNIT));
		}
	}

	P_ResetPitchRoll(mo);

	if (sound)
	{
		S_StartSound(mo, (sound == 1 ? sfx_kc2f : sfx_kpogos));
	}
}

static void K_ThrowLandMine(player_t *player)
{
	mobj_t *landMine;
	mobj_t *throwmo;

	landMine = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height/2, MT_LANDMINE);
	K_FlipFromObject(landMine, player->mo);
	landMine->threshold = 10;

	if (landMine->info->seesound)
		S_StartSound(player->mo, landMine->info->seesound);

	P_SetTarget(&landMine->target, player->mo);

	P_SetScale(landMine, player->mo->scale);
	landMine->destscale = player->mo->destscale;

	landMine->angle = player->mo->angle;

	landMine->momz = (30 * mapobjectscale * P_MobjFlip(player->mo)) + player->mo->momz;
	landMine->color = player->skincolor;

	throwmo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height/2, MT_FIREDITEM);
	P_SetTarget(&throwmo->target, player->mo);
	// Ditto:
	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		throwmo->z -= player->mo->height;
		throwmo->eflags |= MFE_VERTICALFLIP;
	}

	throwmo->movecount = 0; // above player
}

void K_DoInvincibility(player_t *player, tic_t time)
{
	if (!player->invincibilitytimer)
	{
		mobj_t *overlay = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_INVULNFLASH);
		P_SetTarget(&overlay->target, player->mo);
		overlay->destscale = player->mo->scale;
		P_SetScale(overlay, player->mo->scale);
	}

	if (P_IsPartyPlayer(player) == false)
	{
		S_StartSound(player->mo, sfx_alarmi);
	}

	player->invincibilitytimer = time;
}

void K_KillBananaChain(mobj_t *banana, mobj_t *inflictor, mobj_t *source)
{
	mobj_t *cachenext;

killnext:
	if (P_MobjWasRemoved(banana))
		return;

	cachenext = banana->hnext;

	if (banana->health)
	{
		if (banana->eflags & MFE_VERTICALFLIP)
			banana->z -= banana->height;
		else
			banana->z += banana->height;

		S_StartSound(banana, banana->info->deathsound);
		P_KillMobj(banana, inflictor, source, DMG_NORMAL);

		P_SetObjectMomZ(banana, 24*FRACUNIT, false);
		if (inflictor)
			P_InstaThrust(banana, R_PointToAngle2(inflictor->x, inflictor->y, banana->x, banana->y)+ANGLE_90, 16*FRACUNIT);
	}

	if ((banana = cachenext))
		goto killnext;
}

// Just for firing/dropping items.
void K_UpdateHnextList(player_t *player, boolean clean)
{
	mobj_t *work = player->mo, *nextwork;

	if (!work)
		return;

	nextwork = work->hnext;

	while ((work = nextwork) && !(work == NULL || P_MobjWasRemoved(work)))
	{
		nextwork = work->hnext;

		if (!clean && (!work->movedir || work->movedir <= (UINT16)player->itemamount))
		{
			continue;
		}

		P_RemoveMobj(work);
	}

	if (player->mo->hnext == NULL || P_MobjWasRemoved(player->mo->hnext))
	{
		// Like below, try to clean up the pointer if it's NULL.
		// Maybe this was a cause of the shrink/eggbox fails?
		P_SetTarget(&player->mo->hnext, NULL);
	}
}

// For getting hit!
void K_PopPlayerShield(player_t *player)
{
	INT32 shield = K_GetShieldFromItem(player->itemtype);

	// Doesn't apply if player is invalid.
	if (player->mo == NULL || P_MobjWasRemoved(player->mo))
	{
		return;
	}

	switch (shield)
	{
		case KSHIELD_NONE:
			// Doesn't apply to non-S3K shields.
			return;

		case KSHIELD_TOP:
			if (player->curshield == KSHIELD_TOP)
			{
				Obj_GardenTopDestroy(player);
			}
			return; // everything is handled by Obj_GardenTopDestroy

		case KSHIELD_LIGHTNING:
			S_StartSound(player->mo, sfx_s3k7c);
			// K_DoLightningShield(player);
			break;
	}

	player->curshield = KSHIELD_NONE;
	player->itemtype = KITEM_NONE;
	player->itemamount = 0;
	K_UnsetItemOut(player);
}

void K_DropHnextList(player_t *player)
{
	mobj_t *work = player->mo, *nextwork, *dropwork;
	INT32 flip;
	mobjtype_t type;
	boolean orbit, ponground, dropall = true;

	if (work == NULL || P_MobjWasRemoved(work))
	{
		return;
	}

	flip = P_MobjFlip(player->mo);
	ponground = P_IsObjectOnGround(player->mo);

	nextwork = work->hnext;

	while ((work = nextwork) && !(work == NULL || P_MobjWasRemoved(work)))
	{
		nextwork = work->hnext;

		if (!work->health)
			continue; // taking care of itself

		switch (work->type)
		{
			// Kart orbit items
			case MT_ORBINAUT_SHIELD:
				orbit = true;
				type = MT_ORBINAUT;
				break;
			case MT_JAWZ_SHIELD:
				orbit = true;
				type = MT_JAWZ;
				break;
			// Kart trailing items
			case MT_BANANA_SHIELD:
				orbit = false;
				type = MT_BANANA;
				break;
			case MT_SSMINE_SHIELD:
				orbit = false;
				dropall = false;
				type = MT_SSMINE;
				break;
			case MT_DROPTARGET_SHIELD:
				orbit = false;
				dropall = false;
				type = MT_DROPTARGET;
				break;
			case MT_EGGMANITEM_SHIELD:
				orbit = false;
				type = MT_EGGMANITEM;
				break;
			// intentionally do nothing
			case MT_ROCKETSNEAKER:
			case MT_SINK_SHIELD:
			case MT_GARDENTOP:
				return;
			default:
				continue;
		}

		dropwork = P_SpawnMobj(work->x, work->y, work->z, type);

		P_SetTarget(&dropwork->target, player->mo);

		dropwork->angle = work->angle;

		P_SetScale(dropwork, work->scale);
		dropwork->destscale = K_ItemScaleForPlayer(player); //work->destscale;
		dropwork->scalespeed = work->scalespeed;
		dropwork->spritexscale = work->spritexscale;
		dropwork->spriteyscale = work->spriteyscale;

		dropwork->flags |= MF_NOCLIPTHING;
		dropwork->flags2 = work->flags2;
		dropwork->eflags = work->eflags;

		dropwork->renderflags = work->renderflags;
		dropwork->color = work->color;
		dropwork->colorized = work->colorized;
		dropwork->whiteshadow = work->whiteshadow;

		dropwork->floorz = work->floorz;
		dropwork->ceilingz = work->ceilingz;

		dropwork->health = work->health; // will never be set to 0 as long as above guard exists
		dropwork->hitlag = work->hitlag;

		if (orbit == true)
		{
			// Projectile item; set fuse
			dropwork->fuse = RR_PROJECTILE_FUSE;
		}

		// Copy interp data
		dropwork->old_angle = work->old_angle;
		dropwork->old_x = work->old_x;
		dropwork->old_y = work->old_y;
		dropwork->old_z = work->old_z;

		if (ponground)
		{
			// floorz and ceilingz aren't properly set to account for FOFs and Polyobjects on spawn
			// This should set it for FOFs
			//P_SetOrigin(dropwork, dropwork->x, dropwork->y, dropwork->z); -- handled better by above floorz/ceilingz passing

			if (flip == 1)
			{
				if (dropwork->floorz > dropwork->target->z - dropwork->height)
				{
					dropwork->z = dropwork->floorz;
				}
			}
			else
			{
				if (dropwork->ceilingz < dropwork->target->z + dropwork->target->height + dropwork->height)
				{
					dropwork->z = dropwork->ceilingz - dropwork->height;
				}
			}
		}

		if (orbit) // splay out
		{
			if (dropwork->destscale > work->destscale)
			{
				fixed_t radius = FixedMul(work->info->radius, dropwork->destscale);
				radius = FixedHypot(player->mo->radius, player->mo->radius) + FixedHypot(radius, radius); // mobj's distance from its Target, or Radius.
				dropwork->flags |= (MF_NOCLIP|MF_NOCLIPTHING);
				work->momx = FixedMul(FINECOSINE(work->angle>>ANGLETOFINESHIFT), radius);
				work->momy = FixedMul(FINESINE(work->angle>>ANGLETOFINESHIFT), radius);
				P_MoveOrigin(dropwork, player->mo->x + work->momx, player->mo->y + work->momy, player->mo->z);
				dropwork->flags &= ~(MF_NOCLIP|MF_NOCLIPTHING);
			}

			if (type == MT_ORBINAUT)
			{
				Obj_OrbinautDrop(dropwork);
			}
			else
			{
				dropwork->flags2 |= MF2_AMBUSH;
			}

			dropwork->z += flip;

			dropwork->momx = player->mo->momx>>1;
			dropwork->momy = player->mo->momy>>1;
			dropwork->momz = 3*flip*mapobjectscale;

			if (dropwork->eflags & MFE_UNDERWATER)
				dropwork->momz = (117 * dropwork->momz) / 200;

			P_Thrust(dropwork, work->angle - ANGLE_90, 6*mapobjectscale);

			dropwork->movecount = 2;

			// TODO: movedir doesn't seem to be used by
			// anything. It conflicts with orbinaut_flags so
			// is commented out.
			//dropwork->movedir = work->angle - ANGLE_90;

			P_SetMobjState(dropwork, dropwork->info->deathstate);

			dropwork->tics = -1;

			if (type == MT_JAWZ)
			{
				dropwork->z += 20*flip*dropwork->scale;
			}
			else
			{
				dropwork->angle -= ANGLE_90;
			}
		}
		else // plop on the ground
		{
			dropwork->threshold = 10;
			dropwork->flags &= ~MF_NOCLIPTHING;

			if (type == MT_DROPTARGET && dropwork->hitlag)
			{
				dropwork->momx = work->momx;
				dropwork->momy = work->momy;
				dropwork->momz = work->momz;
				dropwork->reactiontime = work->reactiontime;
				P_SetMobjState(dropwork, mobjinfo[type].painstate);
			}
		}

		P_RemoveMobj(work);
	}

	// we need this here too because this is done in afterthink - pointers are cleaned up at the START of each tic...
	P_SetTarget(&player->mo->hnext, NULL);

	player->bananadrag = 0;

	if (player->itemflags & IF_EGGMANOUT)
	{
		player->itemflags &= ~IF_EGGMANOUT;
	}
	else if ((player->itemflags & IF_ITEMOUT)
		&& (dropall || (--player->itemamount <= 0)))
	{
		player->itemamount = 0;
		K_UnsetItemOut(player);
		player->itemtype = KITEM_NONE;
	}
}

SINT8 K_GetTotallyRandomResult(UINT8 useodds)
{
	INT32 spawnchance[NUMKARTRESULTS];
	INT32 totalspawnchance = 0;
	INT32 i;

	memset(spawnchance, 0, sizeof (spawnchance));

	for (i = 1; i < NUMKARTRESULTS; i++)
	{
		// Avoid calling K_FillItemRouletteData since that
		// function resets PR_ITEM_ROULETTE.
		spawnchance[i] = (
			totalspawnchance += K_KartGetItemOdds(NULL, NULL, useodds, i)
		);
	}

	if (totalspawnchance > 0)
	{
		totalspawnchance = P_RandomKey(PR_ITEM_ROULETTE, totalspawnchance);
		for (i = 0; i < NUMKARTRESULTS && spawnchance[i] <= totalspawnchance; i++);
	}
	else
	{
		i = KITEM_SAD;
	}

	return i;
}

mobj_t *K_CreatePaperItem(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT8 type, UINT16 amount)
{
	mobj_t *drop = P_SpawnMobj(x, y, z, MT_FLOATINGITEM);

	// FIXME: due to linkdraw sucking major ass, I was unable
	// to make a backdrop render behind dropped power-ups
	// (which use a smaller sprite than normal items). So
	// dropped power-ups have the backdrop baked into the
	// sprite for now.
	if (type < FIRSTPOWERUP)
	{
		mobj_t *backdrop = P_SpawnMobjFromMobj(drop, 0, 0, 0, MT_OVERLAY);

		P_SetTarget(&backdrop->target, drop);
		P_SetMobjState(backdrop, S_ITEMBACKDROP);

		backdrop->dispoffset = 1;
		P_SetTarget(&backdrop->tracer, drop);
		backdrop->flags2 |= MF2_LINKDRAW;
	}

	P_SetScale(drop, drop->scale>>4);
	drop->destscale = (3*drop->destscale)/2;

	drop->angle = angle;

	if (type == 0)
	{
		const SINT8 i = K_GetTotallyRandomResult(amount);

		// TODO: this is bad!
		// K_KartGetItemResult requires a player
		// but item roulette will need rewritten to change this

		const SINT8 newType = K_ItemResultToType(i);
		const UINT8 newAmount = K_ItemResultToAmount(i);

		if (newAmount > 1)
		{
			UINT8 j;

			for (j = 0; j < newAmount-1; j++)
			{
				K_CreatePaperItem(
					x, y, z,
					angle, flip,
					newType, 1
				);
			}
		}

		drop->threshold = newType;
		drop->movecount = 1;
	}
	else
	{
		drop->threshold = type;
		drop->movecount = amount;
	}

	if (type < FIRSTPOWERUP)
	{
		// Pick up power-ups immediately
		drop->flags |= MF_NOCLIPTHING;
	}

	if (gametyperules & GTR_CLOSERPLAYERS)
	{
		drop->fuse = BATTLE_DESPAWN_TIME;
	}

	return drop;
}

mobj_t *K_FlingPaperItem(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT8 type, UINT16 amount)
{
	mobj_t *drop = K_CreatePaperItem(x, y, z, angle, flip, type, amount);

	P_Thrust(drop,
		FixedAngle(P_RandomFixed(PR_ITEM_ROULETTE) * 180) + angle,
		16*mapobjectscale);

	drop->momz = flip * 3 * mapobjectscale;
	if (drop->eflags & MFE_UNDERWATER)
		drop->momz = (117 * drop->momz) / 200;

	return drop;
}

void K_DropPaperItem(player_t *player, UINT8 itemtype, UINT16 itemamount)
{
	if (!player->mo || P_MobjWasRemoved(player->mo))
	{
		return;
	}

	mobj_t *drop = K_FlingPaperItem(
		player->mo->x, player->mo->y, player->mo->z + player->mo->height/2,
		player->mo->angle + ANGLE_90, P_MobjFlip(player->mo),
		itemtype, itemamount
	);

	K_FlipFromObject(drop, player->mo);
}

// For getting EXTRA hit!
void K_DropItems(player_t *player)
{
	K_DropHnextList(player);

	if (player->itemamount > 0)
	{
		K_DropPaperItem(player, player->itemtype, player->itemamount);
	}

	K_StripItems(player);
}

void K_DropRocketSneaker(player_t *player)
{
	mobj_t *shoe = player->mo;
	fixed_t flingangle;
	boolean leftshoe = true; //left shoe is first

	if (!(player->mo && !P_MobjWasRemoved(player->mo) && player->mo->hnext && !P_MobjWasRemoved(player->mo->hnext)))
		return;

	while ((shoe = shoe->hnext) && !P_MobjWasRemoved(shoe))
	{
		if (shoe->type != MT_ROCKETSNEAKER)
			return; //woah, not a rocketsneaker, bail! safeguard in case this gets used when you're holding non-rocketsneakers

		shoe->renderflags &= ~RF_DONTDRAW;
		shoe->flags &= ~MF_NOGRAVITY;
		shoe->angle += ANGLE_45;

		if (shoe->eflags & MFE_VERTICALFLIP)
			shoe->z -= shoe->height;
		else
			shoe->z += shoe->height;

		//left shoe goes off tot eh left, right shoe off to the right
		if (leftshoe)
			flingangle = -(ANG60);
		else
			flingangle = ANG60;

		S_StartSound(shoe, shoe->info->deathsound);
		P_SetObjectMomZ(shoe, 24*FRACUNIT, false);
		P_InstaThrust(shoe, R_PointToAngle2(shoe->target->x, shoe->target->y, shoe->x, shoe->y)+flingangle, 16*FRACUNIT);
		shoe->momx += shoe->target->momx;
		shoe->momy += shoe->target->momy;
		shoe->momz += shoe->target->momz;
		shoe->extravalue2 = 1;

		leftshoe = false;
	}
	P_SetTarget(&player->mo->hnext, NULL);
	player->rocketsneakertimer = 0;
}

void K_DropKitchenSink(player_t *player)
{
	if (!(player->mo && !P_MobjWasRemoved(player->mo) && player->mo->hnext && !P_MobjWasRemoved(player->mo->hnext)))
		return;

	if (player->mo->hnext->type != MT_SINK_SHIELD)
		return; //so we can just call this function regardless of what is being held

	P_KillMobj(player->mo->hnext, NULL, NULL, DMG_NORMAL);

	P_SetTarget(&player->mo->hnext, NULL);
}

// When an item in the hnext chain dies.
void K_RepairOrbitChain(mobj_t *orbit)
{
	mobj_t *cachenext = orbit->hnext;

	// First, repair the chain
	if (orbit->hnext && orbit->hnext->health && !P_MobjWasRemoved(orbit->hnext))
	{
		P_SetTarget(&orbit->hnext->hprev, orbit->hprev);
		P_SetTarget(&orbit->hnext, NULL);
	}

	if (orbit->hprev && orbit->hprev->health && !P_MobjWasRemoved(orbit->hprev))
	{
		P_SetTarget(&orbit->hprev->hnext, cachenext);
		P_SetTarget(&orbit->hprev, NULL);
	}

	// Then recount to make sure item amount is correct
	if (orbit->target && orbit->target->player && !P_MobjWasRemoved(orbit->target))
	{
		INT32 num = 0;

		mobj_t *cur = orbit->target->hnext;
		mobj_t *prev = NULL;

		while (cur && !P_MobjWasRemoved(cur))
		{
			prev = cur;
			cur = cur->hnext;
			if (++num > orbit->target->player->itemamount)
				P_RemoveMobj(prev);
			else
				prev->movedir = num;
		}

		if (orbit->target && !P_MobjWasRemoved(orbit->target) && orbit->target->player->itemamount != num)
			orbit->target->player->itemamount = num;
	}
}

// Simplified version of a code bit in P_MobjFloorZ
static fixed_t K_BananaSlopeZ(pslope_t *slope, fixed_t x, fixed_t y, fixed_t z, fixed_t radius, boolean ceiling)
{
	fixed_t testx, testy;

	if (slope == NULL)
	{
		testx = x;
		testy = y;
	}
	else
	{
		if (slope->d.x < 0)
			testx = radius;
		else
			testx = -radius;

		if (slope->d.y < 0)
			testy = radius;
		else
			testy = -radius;

		if ((slope->zdelta > 0) ^ !!(ceiling))
		{
			testx = -testx;
			testy = -testy;
		}

		testx += x;
		testy += y;
	}

	return P_GetZAt(slope, testx, testy, z);
}

void K_CalculateBananaSlope(mobj_t *mobj, fixed_t x, fixed_t y, fixed_t z, fixed_t radius, fixed_t height, boolean flip, boolean player)
{
	fixed_t newz;
	sector_t *sec;
	pslope_t *slope = NULL;

	sec = R_PointInSubsector(x, y)->sector;

	if (flip)
	{
		slope = sec->c_slope;
		newz = K_BananaSlopeZ(slope, x, y, sec->ceilingheight, radius, true);
	}
	else
	{
		slope = sec->f_slope;
		newz = K_BananaSlopeZ(slope, x, y, sec->floorheight, radius, true);
	}

	// Check FOFs for a better suited slope
	if (sec->ffloors)
	{
		ffloor_t *rover;

		for (rover = sec->ffloors; rover; rover = rover->next)
		{
			fixed_t top, bottom;
			fixed_t d1, d2;

			if (!(rover->fofflags & FOF_EXISTS))
				continue;

			if ((!(((rover->fofflags & FOF_BLOCKPLAYER && player)
				|| (rover->fofflags & FOF_BLOCKOTHERS && !player))
				|| (rover->fofflags & FOF_QUICKSAND))
				|| (rover->fofflags & FOF_SWIMMABLE)))
				continue;

			top = K_BananaSlopeZ(*rover->t_slope, x, y, *rover->topheight, radius, false);
			bottom = K_BananaSlopeZ(*rover->b_slope, x, y, *rover->bottomheight, radius, true);

			if (flip)
			{
				if (rover->fofflags & FOF_QUICKSAND)
				{
					if (z < top && (z + height) > bottom)
					{
						if (newz > (z + height))
						{
							newz = (z + height);
							slope = NULL;
						}
					}
					continue;
				}

				d1 = (z + height) - (top + ((bottom - top)/2));
				d2 = z - (top + ((bottom - top)/2));

				if (bottom < newz && abs(d1) < abs(d2))
				{
					newz = bottom;
					slope = *rover->b_slope;
				}
			}
			else
			{
				if (rover->fofflags & FOF_QUICKSAND)
				{
					if (z < top && (z + height) > bottom)
					{
						if (newz < z)
						{
							newz = z;
							slope = NULL;
						}
					}
					continue;
				}

				d1 = z - (bottom + ((top - bottom)/2));
				d2 = (z + height) - (bottom + ((top - bottom)/2));

				if (top > newz && abs(d1) < abs(d2))
				{
					newz = top;
					slope = *rover->t_slope;
				}
			}
		}
	}

	//mobj->standingslope = slope;
	P_SetPitchRollFromSlope(mobj, slope);
}

// Move the hnext chain!
static void K_MoveHeldObjects(player_t *player)
{
	fixed_t finalscale = INT32_MAX;

	if (!player->mo)
		return;

	if (!player->mo->hnext)
	{
		player->bananadrag = 0;

		if (player->itemflags & IF_EGGMANOUT)
		{
			player->itemflags &= ~IF_EGGMANOUT;
		}
		else if (player->itemflags & IF_ITEMOUT)
		{
			player->itemamount = 0;
			K_UnsetItemOut(player);
			player->itemtype = KITEM_NONE;
		}
		return;
	}

	if (P_MobjWasRemoved(player->mo->hnext))
	{
		// we need this here too because this is done in afterthink - pointers are cleaned up at the START of each tic...
		P_SetTarget(&player->mo->hnext, NULL);
		player->bananadrag = 0;

		if (player->itemflags & IF_EGGMANOUT)
		{
			player->itemflags &= ~IF_EGGMANOUT;
		}
		else if (player->itemflags & IF_ITEMOUT)
		{
			player->itemamount = 0;
			K_UnsetItemOut(player);
			player->itemtype = KITEM_NONE;
		}

		return;
	}

	finalscale = K_ItemScaleForPlayer(player);

	switch (player->mo->hnext->type)
	{
		case MT_ORBINAUT_SHIELD: // Kart orbit items
		case MT_JAWZ_SHIELD:
			{
				Obj_OrbinautJawzMoveHeld(player);
				break;
			}
		case MT_BANANA_SHIELD: // Kart trailing items
		case MT_SSMINE_SHIELD:
		case MT_DROPTARGET_SHIELD:
		case MT_EGGMANITEM_SHIELD:
		case MT_SINK_SHIELD:
			{
				mobj_t *cur = player->mo->hnext;
				mobj_t *curnext;
				mobj_t *targ = player->mo;

				if (P_IsObjectOnGround(player->mo) && player->speed > 0)
					player->bananadrag++;

				while (cur && !P_MobjWasRemoved(cur))
				{
					const fixed_t radius = FixedHypot(targ->radius, targ->radius) + FixedHypot(cur->radius, cur->radius);
					angle_t ang;
					fixed_t targx, targy, targz;
					fixed_t speed, dist;

					curnext = cur->hnext;

					if (cur->type == MT_EGGMANITEM_SHIELD)
					{
						Obj_RandomItemVisuals(cur);

						// Decided that this should use their "canon" color.
						cur->color = SKINCOLOR_BLACK;
					}
					else if (cur->type == MT_DROPTARGET_SHIELD)
					{
						cur->renderflags = (cur->renderflags|RF_FULLBRIGHT) ^ RF_FULLDARK; // the difference between semi and fullbright
					}

					cur->flags &= ~MF_NOCLIPTHING;

					if ((player->mo->eflags & MFE_VERTICALFLIP) != (cur->eflags & MFE_VERTICALFLIP))
						K_FlipFromObject(cur, player->mo);

					if (!cur->health)
					{
						cur = curnext;
						continue;
					}

					if (cur->extravalue1 < radius)
						cur->extravalue1 += FixedMul(P_AproxDistance(cur->extravalue1, radius), FRACUNIT/12);
					if (cur->extravalue1 > radius)
						cur->extravalue1 = radius;

					if (cur != player->mo->hnext)
					{
						targ = cur->hprev;
						dist = cur->extravalue1/4;
					}
					else
						dist = cur->extravalue1/2;

					if (!targ || P_MobjWasRemoved(targ))
					{
						cur = curnext;
						continue;
					}

					// Shrink your items if the player shrunk too.
					P_SetScale(cur, (cur->destscale = FixedMul(FixedDiv(cur->extravalue1, radius), finalscale)));

					ang = targ->angle;
					targx = targ->x + P_ReturnThrustX(cur, ang + ANGLE_180, dist);
					targy = targ->y + P_ReturnThrustY(cur, ang + ANGLE_180, dist);
					targz = targ->z;

					speed = FixedMul(R_PointToDist2(cur->x, cur->y, targx, targy), 3*FRACUNIT/4);
					if (P_IsObjectOnGround(targ))
						targz = cur->floorz;

					cur->angle = R_PointToAngle2(cur->x, cur->y, targx, targy);

					/*
					if (P_IsObjectOnGround(player->mo) && player->speed > 0 && player->bananadrag > TICRATE
						&& P_RandomChance(PR_UNDEFINED, min(FRACUNIT/2, FixedDiv(player->speed, K_GetKartSpeed(player, false, false))/2)))
					{
						if (leveltime & 1)
							targz += 8*(2*FRACUNIT)/7;
						else
							targz -= 8*(2*FRACUNIT)/7;
					}
					*/

					if (speed > dist)
						P_InstaThrust(cur, cur->angle, speed-dist);

					P_SetObjectMomZ(cur, FixedMul(targz - cur->z, 7*FRACUNIT/8) - gravity, false);

					if (R_PointToDist2(cur->x, cur->y, targx, targy) > 768*FRACUNIT)
					{
						P_MoveOrigin(cur, targx, targy, cur->z);
						if (P_MobjWasRemoved(cur))
						{
							cur = curnext;
							continue;
						}
					}

					if (P_IsObjectOnGround(cur))
					{
						K_CalculateBananaSlope(cur, cur->x, cur->y, cur->z,
							cur->radius, cur->height, (cur->eflags & MFE_VERTICALFLIP), false);
					}

					cur = curnext;
				}
			}
			break;
		case MT_ROCKETSNEAKER: // Special rocket sneaker stuff
			{
				mobj_t *cur = player->mo->hnext;
				INT32 num = 0;

				while (cur && !P_MobjWasRemoved(cur))
				{
					const fixed_t radius = FixedHypot(player->mo->radius, player->mo->radius) + FixedHypot(cur->radius, cur->radius);
					boolean vibrate = ((leveltime & 1) && !cur->tracer);
					angle_t angoffset;
					fixed_t targx, targy, targz;

					cur->flags &= ~MF_NOCLIPTHING;

					if (player->rocketsneakertimer <= TICRATE && (leveltime & 1))
						cur->renderflags |= RF_DONTDRAW;
					else
						cur->renderflags &= ~RF_DONTDRAW;

					if (num & 1)
						P_SetMobjStateNF(cur, (vibrate ? S_ROCKETSNEAKER_LVIBRATE : S_ROCKETSNEAKER_L));
					else
						P_SetMobjStateNF(cur, (vibrate ? S_ROCKETSNEAKER_RVIBRATE : S_ROCKETSNEAKER_R));

					if (!player->rocketsneakertimer || cur->extravalue2 || !cur->health)
					{
						num = (num+1) % 2;
						cur = cur->hnext;
						continue;
					}

					if (cur->extravalue1 < radius)
						cur->extravalue1 += FixedMul(P_AproxDistance(cur->extravalue1, radius), FRACUNIT/12);
					if (cur->extravalue1 > radius)
						cur->extravalue1 = radius;

					// Shrink your items if the player shrunk too.
					P_SetScale(cur, (cur->destscale = FixedMul(FixedDiv(cur->extravalue1, radius), player->mo->scale)));

#if 1
					{
						angle_t input = player->drawangle - cur->angle;
						boolean invert = (input > ANGLE_180);
						if (invert)
							input = InvAngle(input);

						input = FixedAngle(AngleFixed(input)/4);
						if (invert)
							input = InvAngle(input);

						cur->angle = cur->angle + input;
					}
#else
					cur->angle = player->drawangle;
#endif

					angoffset = ANGLE_90 + (ANGLE_180 * num);

					targx = player->mo->x + P_ReturnThrustX(cur, cur->angle + angoffset, cur->extravalue1);
					targy = player->mo->y + P_ReturnThrustY(cur, cur->angle + angoffset, cur->extravalue1);

					{ // bobbing, copy pasted from my kimokawaiii entry
						fixed_t sine = FixedMul(player->mo->scale, 8 * FINESINE((((M_TAU_FIXED * (4*TICRATE)) * leveltime) >> ANGLETOFINESHIFT) & FINEMASK));
						targz = (player->mo->z + (player->mo->height/2)) + sine;
						if (player->mo->eflags & MFE_VERTICALFLIP)
							targz += (player->mo->height/2 - 32*player->mo->scale)*6;
					}

					if (cur->tracer && !P_MobjWasRemoved(cur->tracer))
					{
						fixed_t diffx, diffy, diffz;

						diffx = targx - cur->x;
						diffy = targy - cur->y;
						diffz = targz - cur->z;

						P_MoveOrigin(cur->tracer, cur->tracer->x + diffx + P_ReturnThrustX(cur, cur->angle + angoffset, 6*cur->scale),
							cur->tracer->y + diffy + P_ReturnThrustY(cur, cur->angle + angoffset, 6*cur->scale), cur->tracer->z + diffz);
						P_SetScale(cur->tracer, (cur->tracer->destscale = 3*cur->scale/4));
					}

					P_MoveOrigin(cur, targx, targy, targz);
					K_FlipFromObject(cur, player->mo);	// Update graviflip in real time thanks.

					cur->roll = player->mo->roll;
					cur->pitch = player->mo->pitch;

					num = (num+1) % 2;
					cur = cur->hnext;
				}
			}
			break;
		default:
			break;
	}
}

mobj_t *K_FindJawzTarget(mobj_t *actor, player_t *source, angle_t range)
{
	fixed_t best = INT32_MAX;
	mobj_t *wtarg = NULL;
	INT32 i;

	if (specialstageinfo.valid == true)
	{
		// Always target the UFO (but not the emerald)
		return K_GetPossibleSpecialTarget();
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		angle_t thisang = ANGLE_MAX;
		fixed_t thisdist = INT32_MAX;
		fixed_t thisScore = INT32_MAX;
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];

		// Don't target yourself, stupid.
		if (source != NULL && player == source)
		{
			continue;
		}

		if (player->spectator)
		{
			// Spectators
			continue;
		}

		if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
		{
			// Invalid mobj
			continue;
		}

		if (player->mo->health <= 0)
		{
			// dead
			continue;
		}

		if (G_GametypeHasTeams() && source != NULL && source->ctfteam == player->ctfteam)
		{
			// Don't home in on teammates.
			continue;
		}

		if (player->hyudorotimer > 0)
		{
			// Invisible player
			continue;
		}

		thisdist = P_AproxDistance(player->mo->x - (actor->x + actor->momx), player->mo->y - (actor->y + actor->momy));

		if (gametyperules & GTR_CIRCUIT)
		{
			if (source != NULL && player->position >= source->position)
			{
				// Don't pay attention to people who aren't above your position
				continue;
			}
		}
		else
		{
			// Z pos too high/low
			if (abs(player->mo->z - (actor->z + actor->momz)) > FixedMul(RING_DIST/8, mapobjectscale))
			{
				continue;
			}

			// Distance too far away
			if (thisdist > FixedMul(RING_DIST*2, mapobjectscale))
			{
				continue;
			}
		}

		// Find the angle, see who's got the best.
		thisang = AngleDelta(actor->angle, R_PointToAngle2(actor->x, actor->y, player->mo->x, player->mo->y));

		// Don't go for people who are behind you
		if (thisang > range)
		{
			continue;
		}

		thisScore = (AngleFixed(thisang) * 8) + (thisdist / 32);

		//CONS_Printf("got score %f from player # %d\n", FixedToFloat(thisScore), i);

		if (thisScore < best)
		{
			wtarg = player->mo;
			best = thisScore;
		}
	}

	return wtarg;
}

// Engine Sounds.
static void K_UpdateEngineSounds(player_t *player)
{
	const INT32 numsnds = 13;

	const fixed_t closedist = 160*FRACUNIT;
	const fixed_t fardist = 1536*FRACUNIT;

	const UINT8 dampenval = 48; // 255 * 48 = close enough to FRACUNIT/6

	const UINT16 buttons = K_GetKartButtons(player);

	INT32 class; // engine class number

	UINT8 volume = 255;
	fixed_t volumedampen = FRACUNIT;

	INT32 targetsnd = 0;
	INT32 i;

	if (leveltime < 8 || player->spectator || gamestate != GS_LEVEL || player->exiting)
	{
		// Silence the engines, and reset sound number while we're at it.
		player->karthud[khud_enginesnd] = 0;
		return;
	}

	class = R_GetEngineClass(player->kartspeed, player->kartweight, 0); // there are no unique sounds for ENGINECLASS_J

#if 0
	if ((leveltime % 8) != ((player-players) % 8)) // Per-player offset, to make engines sound distinct!
#else
	if (leveltime % 8)
#endif
	{
		// .25 seconds of wait time between each engine sound playback
		return;
	}

	if (player->respawn.state == RESPAWNST_DROP) // Dropdashing
	{
		// Dropdashing
		targetsnd = ((buttons & BT_ACCELERATE) ? 12 : 0);
	}
	else if (K_PlayerEBrake(player) == true)
	{
		// Spindashing
		targetsnd = ((buttons & BT_DRIFT) ? 12 : 0);
	}
	else
	{
		// Average out the value of forwardmove and the speed that you're moving at.
		targetsnd = (((6 * K_GetForwardMove(player)) / 25) + ((player->speed / mapobjectscale) / 5)) / 2;
	}

	if (targetsnd < 0) { targetsnd = 0; }
	if (targetsnd > 12) { targetsnd = 12; }

	if (player->karthud[khud_enginesnd] < targetsnd) { player->karthud[khud_enginesnd]++; }
	if (player->karthud[khud_enginesnd] > targetsnd) { player->karthud[khud_enginesnd]--; }

	if (player->karthud[khud_enginesnd] < 0) { player->karthud[khud_enginesnd] = 0; }
	if (player->karthud[khud_enginesnd] > 12) { player->karthud[khud_enginesnd] = 12; }

	// This code calculates how many players (and thus, how many engine sounds) are within ear shot,
	// and rebalances the volume of your engine sound based on how far away they are.

	// This results in multiple things:
	// - When on your own, you will hear your own engine sound extremely clearly.
	// - When you were alone but someone is gaining on you, yours will go quiet, and you can hear theirs more clearly.
	// - When around tons of people, engine sounds will try to rebalance to not be as obnoxious.

	for (i = 0; i < MAXPLAYERS; i++)
	{
		UINT8 thisvol = 0;
		fixed_t dist;

		if (!playeringame[i] || !players[i].mo)
		{
			// This player doesn't exist.
			continue;
		}

		if (players[i].spectator)
		{
			// This player isn't playing an engine sound.
			continue;
		}

		if (P_IsDisplayPlayer(&players[i]))
		{
			// Don't dampen yourself!
			continue;
		}

		dist = P_AproxDistance(
			P_AproxDistance(
				player->mo->x - players[i].mo->x,
				player->mo->y - players[i].mo->y),
				player->mo->z - players[i].mo->z) / 2;

		dist = FixedDiv(dist, mapobjectscale);

		if (dist > fardist)
		{
			// ENEMY OUT OF RANGE !
			continue;
		}
		else if (dist < closedist)
		{
			// engine sounds' approx. range
			thisvol = 255;
		}
		else
		{
			thisvol = (15 * ((closedist - dist) / FRACUNIT)) / ((fardist - closedist) >> (FRACBITS+4));
		}

		volumedampen += (thisvol * dampenval);
	}

	if (volumedampen > FRACUNIT)
	{
		volume = FixedDiv(volume * FRACUNIT, volumedampen) / FRACUNIT;
	}

	if (volume <= 0)
	{
		// Don't need to play the sound at all.
		return;
	}

	S_StartSoundAtVolume(player->mo, (sfx_krta00 + player->karthud[khud_enginesnd]) + (class * numsnds), volume);
}

static void K_UpdateInvincibilitySounds(player_t *player)
{
	INT32 sfxnum = sfx_None;

	if (player->mo->health > 0 && !P_IsPartyPlayer(player)) // used to be !P_IsDisplayPlayer(player)
	{
		if (player->invincibilitytimer > 0) // Prioritize invincibility
			sfxnum = sfx_alarmi;
		else if (player->growshrinktimer > 0)
			sfxnum = sfx_alarmg;
	}

	if (sfxnum != sfx_None && !S_SoundPlaying(player->mo, sfxnum))
		S_StartSound(player->mo, sfxnum);

#define STOPTHIS(this) \
	if (sfxnum != this && S_SoundPlaying(player->mo, this)) \
		S_StopSoundByID(player->mo, this);
	STOPTHIS(sfx_alarmi);
	STOPTHIS(sfx_alarmg);
#undef STOPTHIS
}

// This function is not strictly for non-netsynced properties.
// It's just a convenient name for things that don't stop during hitlag.
void K_KartPlayerHUDUpdate(player_t *player)
{
	if (K_PlayerTallyActive(player) == true)
	{
		K_TickPlayerTally(player);
	}

	if (player->karthud[khud_lapanimation])
		player->karthud[khud_lapanimation]--;

	if (player->karthud[khud_yougotem])
		player->karthud[khud_yougotem]--;

	if (player->karthud[khud_voices])
		player->karthud[khud_voices]--;

	if (player->karthud[khud_tauntvoices])
		player->karthud[khud_tauntvoices]--;

	if (player->karthud[khud_taunthorns])
		player->karthud[khud_taunthorns]--;

	if (player->karthud[khud_trickcool])
		player->karthud[khud_trickcool]--;

	if (player->positiondelay)
		player->positiondelay--;

	if (!(player->pflags & PF_FAULT || player->pflags & PF_VOID))
		player->karthud[khud_fault] = 0;
	else if (player->karthud[khud_fault] > 0 && player->karthud[khud_fault] <= 2*TICRATE)
		player->karthud[khud_fault]++;

	if (player->karthud[khud_itemblink] && player->karthud[khud_itemblink]-- <= 0)
	{
		player->karthud[khud_itemblinkmode] = 0;
		player->karthud[khud_itemblink] = 0;
	}

	if (player->karthud[khud_rouletteoffset] != 0)
	{
		if (abs(player->karthud[khud_rouletteoffset]) < (FRACUNIT >> 1))
		{
			// Snap to 0, since the change is getting very small.
			player->karthud[khud_rouletteoffset] = 0;
		}
		else
		{
			// Lerp to 0.
			player->karthud[khud_rouletteoffset] = FixedMul(player->karthud[khud_rouletteoffset], FRACUNIT*3/4);
		}
	}

	if (!(gametyperules & GTR_SPHERES))
	{
		if (!player->exiting
		&& !(player->pflags & (PF_NOCONTEST|PF_ELIMINATED)))
		{
			player->hudrings = player->rings;
		}

		if (player->mo && player->mo->hitlag <= 0)
		{
			// 0 is the fast spin animation, set at 30 tics of ring boost or higher!
			if (player->ringboost >= 30)
				player->karthud[khud_ringdelay] = 0;
			else
				player->karthud[khud_ringdelay] = ((RINGANIM_DELAYMAX+1) * (30 - player->ringboost)) / 30;

			if (player->karthud[khud_ringframe] == 0 && player->karthud[khud_ringdelay] > RINGANIM_DELAYMAX)
			{
				player->karthud[khud_ringframe] = 0;
				player->karthud[khud_ringtics] = 0;
			}
			else if ((player->karthud[khud_ringtics]--) <= 0)
			{
				if (player->karthud[khud_ringdelay] == 0) // fast spin animation
				{
					player->karthud[khud_ringframe] = ((player->karthud[khud_ringframe]+2) % RINGANIM_NUMFRAMES);
					player->karthud[khud_ringtics] = 0;
				}
				else
				{
					player->karthud[khud_ringframe] = ((player->karthud[khud_ringframe]+1) % RINGANIM_NUMFRAMES);
					player->karthud[khud_ringtics] = min(RINGANIM_DELAYMAX, player->karthud[khud_ringdelay])-1;
				}
			}
		}

		if (player->pflags & PF_RINGLOCK)
		{
			UINT8 normalanim = (leveltime % 14);
			UINT8 debtanim = 14 + (leveltime % 2);

			if (player->karthud[khud_ringspblock] >= 14) // debt animation
			{
				if ((player->hudrings > 0) // Get out of 0 ring animation
					&& (normalanim == 3 || normalanim == 10)) // on these transition frames.
					player->karthud[khud_ringspblock] = normalanim;
				else
					player->karthud[khud_ringspblock] = debtanim;
			}
			else // normal animation
			{
				if ((player->hudrings <= 0) // Go into 0 ring animation
					&& (player->karthud[khud_ringspblock] == 1 || player->karthud[khud_ringspblock] == 8)) // on these transition frames.
					player->karthud[khud_ringspblock] = debtanim;
				else
					player->karthud[khud_ringspblock] = normalanim;
			}
		}
		else
			player->karthud[khud_ringspblock] = (leveltime % 14); // reset to normal anim next time
	}

	if (player->exiting && !(player->pflags & PF_NOCONTEST))
	{
		if (player->karthud[khud_finish] <= 2*TICRATE)
			player->karthud[khud_finish]++;
	}
	else
		player->karthud[khud_finish] = 0;

	// Tumble time stat update
	if (demo.playback == false && P_IsMachineLocalPlayer(player) == true)
	{
		if (player->tumbleBounces != 0 && gamedata->totaltumbletime != UINT32_MAX)
		{
			gamedata->totaltumbletime++;

			if (player->skin >= 0 && player->skin < numskins)
			{
				skin_t *playerskin;
				playerskin = &skins[player->skin];
				playerskin->records.tumbletime++;
			}
		}
	}
}

#undef RINGANIM_DELAYMAX

// SRB2Kart: blockmap iterate for attraction shield users
static mobj_t *attractmo;
static fixed_t attractdist;
static fixed_t attractzdist;

static inline BlockItReturn_t PIT_AttractingRings(mobj_t *thing)
{
	if (attractmo == NULL || P_MobjWasRemoved(attractmo) || attractmo->player == NULL)
	{
		return BMIT_ABORT;
	}

	if (thing == NULL || P_MobjWasRemoved(thing))
	{
		return BMIT_CONTINUE; // invalid
	}

	if (thing == attractmo)
	{
		return BMIT_CONTINUE; // invalid
	}

	if (!(thing->type == MT_RING || thing->type == MT_FLINGRING || thing->type == MT_EMERALD))
	{
		return BMIT_CONTINUE; // not a ring
	}

	if (thing->health <= 0)
	{
		return BMIT_CONTINUE; // dead
	}

	if (thing->extravalue1 && thing->type != MT_EMERALD)
	{
		return BMIT_CONTINUE; // in special ring animation
	}

	if (thing->tracer != NULL && P_MobjWasRemoved(thing->tracer) == false)
	{
		return BMIT_CONTINUE; // already attracted
	}

	// see if it went over / under
	if (attractmo->z - attractzdist > thing->z + thing->height)
	{
		return BMIT_CONTINUE; // overhead
	}

	if (attractmo->z + attractmo->height + attractzdist < thing->z)
	{
		return BMIT_CONTINUE; // underneath
	}

	if (P_AproxDistance(attractmo->x - thing->x, attractmo->y - thing->y) > attractdist + thing->radius)
	{
		return BMIT_CONTINUE; // Too far away
	}

	if (RINGTOTAL(attractmo->player) >= 20 || (attractmo->player->pflags & PF_RINGLOCK))
	{
		// Already reached max -- just joustle rings around.

		// Regular ring -> fling ring
		if (thing->info->reactiontime && thing->type != (mobjtype_t)thing->info->reactiontime)
		{
			thing->type = thing->info->reactiontime;
			thing->info = &mobjinfo[thing->type];
			thing->flags = thing->info->flags;

			P_InstaThrust(thing, P_RandomRange(PR_ITEM_RINGS, 0, 7) * ANGLE_45, 2 * thing->scale);
			P_SetObjectMomZ(thing, 8<<FRACBITS, false);
			thing->fuse = 120*TICRATE;

			thing->cusval = 0; // Reset attraction flag
		}
	}
	else
	{
		// set target
		P_SetTarget(&thing->tracer, attractmo);
	}

	return BMIT_CONTINUE; // find other rings
}

/** Looks for rings near a player in the blockmap.
  *
  * \param pmo Player object looking for rings to attract
  * \sa A_AttractChase
  */
static void K_LookForRings(mobj_t *pmo)
{
	INT32 bx, by, xl, xh, yl, yh;

	attractmo = pmo;
	attractdist = (400 * pmo->scale);
	attractzdist = attractdist >> 2;

	// Use blockmap to check for nearby rings
	yh = (unsigned)(pmo->y + (attractdist + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (unsigned)(pmo->y - (attractdist + MAXRADIUS) - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (unsigned)(pmo->x + (attractdist + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (unsigned)(pmo->x - (attractdist + MAXRADIUS) - bmaporgx)>>MAPBLOCKSHIFT;

	BMBOUNDFIX(xl, xh, yl, yh);

	for (by = yl; by <= yh; by++)
		for (bx = xl; bx <= xh; bx++)
			P_BlockThingsIterator(bx, by, PIT_AttractingRings);
}

static void K_UpdateTripwire(player_t *player)
{
	fixed_t speedThreshold = (3*K_GetKartSpeed(player, false, true))/4;
	boolean goodSpeed = (player->speed >= speedThreshold);
	boolean boostExists = (player->tripwireLeniency > 0); // can't be checked later because of subtractions...
	tripwirepass_t triplevel = K_TripwirePassConditions(player);

	if (triplevel != TRIPWIRE_NONE)
	{
		if (!boostExists)
		{
			mobj_t *front = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_TRIPWIREBOOST);
			mobj_t *back = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_TRIPWIREBOOST);

			P_SetTarget(&front->target, player->mo);
			P_SetTarget(&back->target, player->mo);

			P_SetTarget(&front->punt_ref, player->mo);
			P_SetTarget(&back->punt_ref, player->mo);

			front->dispoffset = 1;
			front->old_angle = back->old_angle = K_MomentumAngle(player->mo);
			back->dispoffset = -1;
			back->extravalue1 = 1;
			P_SetMobjState(back, S_TRIPWIREBOOST_BOTTOM);
		}

		player->tripwirePass = triplevel;

		if (triplevel != TRIPWIRE_CONSUME)
			player->tripwireLeniency = max(player->tripwireLeniency, TRIPWIRETIME);
	}

	// TRIPWIRE_CONSUME is only applied in very specific cases (currently, riding Garden Top)
	// and doesn't need leniency; however, it should track leniency from other pass conditions,
	// so that stripping Garden Top feels consistent.
	if (triplevel == TRIPWIRE_NONE || triplevel == TRIPWIRE_CONSUME)
	{
		if (boostExists)
		{
			player->tripwireLeniency--;
			if (goodSpeed == false && player->tripwireLeniency > 0)
			{
				// Decrease at double speed when your speed is bad.
				player->tripwireLeniency--;
			}
		}

		if (player->tripwireLeniency <= 0 && triplevel == TRIPWIRE_NONE)
		{
			player->tripwirePass = TRIPWIRE_NONE;
		}
	}
}

boolean K_PressingEBrake(const player_t *player)
{
	return ((K_GetKartButtons(player) & BT_EBRAKEMASK) == BT_EBRAKEMASK);
}

/**	\brief	Decreases various kart timers and powers per frame. Called in P_PlayerThink in p_user.c

	\param	player	player object passed from P_PlayerThink
	\param	cmd		control input from player

	\return	void
*/
void K_KartPlayerThink(player_t *player, ticcmd_t *cmd)
{
	const boolean onground = P_IsObjectOnGround(player->mo);

	/* reset sprite offsets :) */
	player->mo->sprxoff = 0;
	player->mo->spryoff = 0;
	player->mo->sprzoff = 0;
	player->mo->spritexoffset = 0;
	player->mo->spriteyoffset = 0;

	player->cameraOffset = 0;

	player->pflags &= ~(PF_CASTSHADOW);

	if (player->curshield == KSHIELD_TOP)
	{
		mobj_t *top = K_GetGardenTop(player);

		if (top)
		{
			/* FIXME: I cannot figure out how offset the
			   player correctly in real time to pivot around
			   the BOTTOM of the Top. This hack plus the one
			   in R_PlayerSpriteRotation. */
			player->mo->spritexoffset += FixedMul(
					FixedDiv(top->height, top->scale),
					FINESINE(top->rollangle >> ANGLETOFINESHIFT));

			player->mo->sprzoff += top->sprzoff + (
					P_GetMobjHead(top) -
					P_GetMobjFeet(player->mo));
		}
	}

	if (!P_MobjWasRemoved(player->whip) && (player->whip->flags2 & MF2_AMBUSH))
	{
		// Linear acceleration and deceleration to a peak.
		// There is a constant total time to complete but the
		// acceleration and deceleration times can be made
		// asymmetrical.
		const fixed_t hop = 24 * mapobjectscale;
		const INT32 duration = 12;
		const INT32 mid = (duration / 2) - 1;
		const INT32 t = (duration - mid) - player->whip->fuse;

		player->cameraOffset = hop - (abs(t * hop) / (t < 0 ? mid : duration - mid));
		player->mo->sprzoff += player->cameraOffset;
	}

	if (player->loop.radius)
	{
		// Offset sprite Z position so wheels touch top of
		// hitbox when rotated 180 degrees.
		// TODO: this should be generalized for pitch/roll
		angle_t pitch = FixedAngle(player->loop.revolution * 360) / 2;
		player->mo->sprzoff += FixedMul(player->mo->height, FSIN(pitch));
	}

	K_UpdateOffroad(player);
	K_UpdateDraft(player);
	K_UpdateEngineSounds(player); // Thanks, VAda!

	Obj_DashRingPlayerThink(player);

	// update boost angle if not spun out
	if (!player->spinouttimer && !player->wipeoutslow)
		player->boostangle = player->mo->angle;

	K_GetKartBoostPower(player);

	// Special effect objects!
	if (player->mo && !player->spectator)
	{
		if (player->dashpadcooldown) // Twinkle Circuit afterimages
		{
			mobj_t *ghost;
			ghost = P_SpawnGhostMobj(player->mo);
			ghost->fuse = player->dashpadcooldown+1;
			ghost->momx = player->mo->momx / (player->dashpadcooldown+1);
			ghost->momy = player->mo->momy / (player->dashpadcooldown+1);
			ghost->momz = player->mo->momz / (player->dashpadcooldown+1);
			player->dashpadcooldown--;
		}

		if (player->speed > 0)
		{
			// Speed lines
			if (player->sneakertimer || player->ringboost
				|| player->driftboost || player->startboost
				|| player->eggmanexplode || player->trickboost
				|| player->gateBoost || player->wavedashboost)
			{
#if 0
				if (player->invincibilitytimer)
					K_SpawnInvincibilitySpeedLines(player->mo);
				else
#endif
					K_SpawnNormalSpeedLines(player);
			}

			if (player->numboosts > 0) // Boosting after images
			{
				mobj_t *ghost;
				ghost = P_SpawnGhostMobj(player->mo);
				ghost->extravalue1 = player->numboosts+1;
				ghost->extravalue2 = (leveltime % ghost->extravalue1);
				ghost->fuse = ghost->extravalue1;
				ghost->renderflags |= RF_FULLBRIGHT;
				ghost->colorized = true;
				//ghost->color = player->skincolor;
				//ghost->momx = (3*player->mo->momx)/4;
				//ghost->momy = (3*player->mo->momy)/4;
				//ghost->momz = (3*player->mo->momz)/4;
				if (leveltime & 1)
					ghost->renderflags |= RF_DONTDRAW;
			}

			if (P_IsObjectOnGround(player->mo))
			{
				// Draft dust
				if (player->draftpower >= FRACUNIT)
				{
					K_SpawnDraftDust(player->mo);
					/*if (leveltime % 23 == 0 || !S_SoundPlaying(player->mo, sfx_s265))
						S_StartSound(player->mo, sfx_s265);*/
				}
			}
		}

		if (player->growshrinktimer != 0)
		{
			K_SpawnGrowShrinkParticles(player->mo, player->growshrinktimer);
		}

		// Race: spawn ring debt indicator
		// Battle: spawn zero-bumpers indicator
		if ((gametyperules & GTR_SPHERES) ? player->mo->health <= 1 : player->rings <= 0)
		{
			UINT8 doubler;

			// GROSS. In order to have a transparent version of this for a splitscreen local player, we actually need to spawn two!
			for (doubler = 0; doubler < 2; doubler++)
			{
				fixed_t heightOffset = player->mo->height + (24*player->mo->scale);
				if (P_IsObjectFlipped(player->mo)) 
				{
					// This counteracts the offset added by K_FlipFromObject so it looks seamless from non-flipped.
					heightOffset += player->mo->height - FixedMul(player->mo->scale, player->mo->height);
					heightOffset *= P_MobjFlip(player->mo); // Fleep.
				}
				
				mobj_t *debtflag = P_SpawnMobj(player->mo->x + player->mo->momx, player->mo->y + player->mo->momy,
					player->mo->z + P_GetMobjZMovement(player->mo) + heightOffset, MT_THOK);

				debtflag->old_x = player->mo->old_x;
				debtflag->old_y = player->mo->old_y;
				debtflag->old_z = player->mo->old_z + P_GetMobjZMovement(player->mo) + heightOffset;

				P_SetMobjState(debtflag, S_RINGDEBT);
				P_SetScale(debtflag, (debtflag->destscale = player->mo->scale));

				K_MatchGenericExtraFlags(debtflag, player->mo);
				debtflag->frame += (leveltime % 4);

				if ((leveltime/12) & 1)
					debtflag->frame += 4;

				debtflag->color = player->skincolor;
				debtflag->fuse = 2;

				if (doubler == 0) // Real copy. Draw for everyone but us.
				{
					debtflag->renderflags |= K_GetPlayerDontDrawFlag(player);
				}
				else if (doubler == 1) // Fake copy. Draw for only us, and go transparent after a bit.
				{
					debtflag->renderflags |= (RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(player));
					if (player->ringvisualwarning <= 1 || gametyperules & GTR_SPHERES)
						debtflag->renderflags |= RF_TRANS50;
				}
			}

		}

		if (player->springstars && (leveltime & 1))
		{
			fixed_t randx = P_RandomRange(PR_DECORATION, -40, 40) * player->mo->scale;
			fixed_t randy = P_RandomRange(PR_DECORATION, -40, 40) * player->mo->scale;
			fixed_t randz = P_RandomRange(PR_DECORATION, 0, player->mo->height >> FRACBITS) << FRACBITS;
			mobj_t *star = P_SpawnMobj(
				player->mo->x + randx,
				player->mo->y + randy,
				player->mo->z + randz,
				MT_KARMAFIREWORK);

			star->color = player->springcolor;
			star->flags |= MF_NOGRAVITY;
			star->momx = player->mo->momx / 2;
			star->momy = player->mo->momy / 2;
			star->momz = P_GetMobjZMovement(player->mo) / 2;
			star->fuse = 12;
			star->scale = player->mo->scale;
			star->destscale = star->scale / 2;

			player->springstars--;
		}
	}

	if (player->itemtype == KITEM_NONE)
		player->itemflags &= ~IF_HOLDREADY;

	// DKR style camera for boosting
	if (player->karthud[khud_boostcam] != 0 || player->karthud[khud_destboostcam] != 0)
	{
		if (player->karthud[khud_boostcam] < player->karthud[khud_destboostcam]
			&& player->karthud[khud_destboostcam] != 0)
		{
			player->karthud[khud_boostcam] += FRACUNIT/(TICRATE/4);
			if (player->karthud[khud_boostcam] >= player->karthud[khud_destboostcam])
				player->karthud[khud_destboostcam] = 0;
		}
		else
		{
			player->karthud[khud_boostcam] -= FRACUNIT/TICRATE;
			if (player->karthud[khud_boostcam] < player->karthud[khud_destboostcam])
				player->karthud[khud_boostcam] = player->karthud[khud_destboostcam] = 0;
		}
		//CONS_Printf("cam: %d, dest: %d\n", player->karthud[khud_boostcam], player->karthud[khud_destboostcam]);
	}

	if (onground)
	{
		if (player->karthud[khud_aircam] > 0)
		{
			player->karthud[khud_aircam] -= FRACUNIT / 5;

			if (player->karthud[khud_aircam] < 0)
			{
				player->karthud[khud_aircam] = 0;
			}

			//CONS_Printf("cam: %f\n", FixedToFloat(player->karthud[khud_aircam]));
		}
	}
	else
	{
		if (player->karthud[khud_aircam] < FRACUNIT)
		{
			player->karthud[khud_aircam] += FRACUNIT / TICRATE;

			if (player->karthud[khud_aircam] > FRACUNIT)
			{
				player->karthud[khud_aircam] = FRACUNIT;
			}

			//CONS_Printf("cam: %f\n", FixedToFloat(player->karthud[khud_aircam]));
		}
	}

	// Make ABSOLUTELY SURE that your flashing tics don't get set WHILE you're still in hit animations.
	if (player->spinouttimer != 0)
	{
		if (( player->spinouttype & KSPIN_IFRAMES ) == 0)
			player->flashing = 0;
		else
			player->flashing = K_GetKartFlashing(player);
	}

	if (player->spinouttimer)
	{
		if (((P_IsObjectOnGround(player->mo)
			|| ( player->spinouttype & KSPIN_AIRTIMER ))
			&& (!player->sneakertimer))
		|| (player->respawn.state != RESPAWNST_NONE
			&& player->spinouttimer > 1
			&& (leveltime & 1)))
		{
			player->spinouttimer--;
			if (player->wipeoutslow > 1)
				player->wipeoutslow--;
		}
	}
	else
	{
		if (player->wipeoutslow >= 1)
			player->mo->friction = ORIG_FRICTION;
		player->wipeoutslow = 0;
	}

	if (player->rings > 20)
		player->rings = 20;
	else if (player->rings < -20)
		player->rings = -20;

	if (cv_kartdebugbotwhip.value)
	{
		if (player->bot)
		{
			player->rings = 0;
			player->itemtype = 0;
			player->itemamount = 0;
			player->itemRoulette.active = false;
		}
	}

	if (player->spheres > 40)
		player->spheres = 40;
	// where's the < 0 check? see below the following block!

	{
		tic_t spheredigestion = TICRATE*2; // Base rate of 1 every 2 seconds when playing.
		tic_t digestionpower = ((10 - player->kartspeed) + (10 - player->kartweight))-1; // 1 to 17

		// currently 0-34
		digestionpower = ((player->spheres*digestionpower)/20);

		if (digestionpower >= spheredigestion)
		{
			spheredigestion = 1;
		}
		else
		{
			spheredigestion -= digestionpower/2;
		}

		if ((player->spheres > 0) && (player->spheredigestion > 0))
		{
			// If you got a massive boost in spheres, catch up digestion as necessary.
			if (spheredigestion < player->spheredigestion)
			{
				player->spheredigestion = (spheredigestion + player->spheredigestion)/2;
			}

			player->spheredigestion--;

			if (player->spheredigestion == 0)
			{
				if (player->spheres > 5)
					player->spheres--;
				player->spheredigestion = spheredigestion;
			}

			if (K_PlayerGuard(player) && !K_PowerUpRemaining(player, POWERUP_BARRIER) && (player->ebrakefor%6 == 0))
				player->spheres--;

			if (player->instaWhipCharge && !K_PowerUpRemaining(players, POWERUP_BADGE) && leveltime%6 == 0 && !P_PlayerInPain(player))
				player->spheres--;
		}
		else
		{
			player->spheredigestion = spheredigestion;
		}
	}

	// where's the > 40 check? see above the previous block!
	if (player->spheres < 0)
		player->spheres = 0;

	if (!(gametyperules & GTR_KARMA) || (player->pflags & PF_ELIMINATED))
	{
		player->karmadelay = 0;
	}
	else if (player->karmadelay > 0 && !P_PlayerInPain(player))
	{
		player->karmadelay--;
		if (P_IsDisplayPlayer(player) && player->karmadelay <= 0)
			comebackshowninfo = true; // client has already seen the message
	}

	if (player->shrinkLaserDelay)
		player->shrinkLaserDelay--;

	if (player->eggmanTransferDelay)
		player->eggmanTransferDelay--;

	if (player->tripwireReboundDelay)
		player->tripwireReboundDelay--;

	if (player->ringdelay)
		player->ringdelay--;

	if (player->trickpanel == TRICKSTATE_READY)
	{
		if (!player->throwdir && !cmd->turning)
			player->tricklock = TICRATE/10;
		else if (player->tricklock)
			player->tricklock--;
	}
	else
	{
		player->tricklock = 0;
	}

	if (P_PlayerInPain(player))
	{
		player->ringboost = 0;
	}
	else if (player->ringboost)
	{
		// If a Ring Box or Super Ring isn't paying out, aggressively reduce
		// extreme ringboost duration. Less aggressive for accel types, so they
		// retain more speed for small payouts.

		UINT8 roller = TICRATE*2;
		roller += 4*(8-player->kartspeed);

		if (player->superring == 0)
			player->ringboost -= max((player->ringboost / roller), 1);
		else
			player->ringboost--;
	}

	if (player->sneakertimer)
	{
		player->sneakertimer--;

		if (player->sneakertimer <= 0)
		{
			player->numsneakers = 0;
		}
	}

	if (player->trickboost)
		player->trickboost--;

	if (player->flamedash)
	{
		player->flamedash--;

		if (player->flamedash == 0)
			S_StopSoundByID(player->mo, sfx_fshld1);
		else if (player->flamedash == 3 && player->curshield == KSHIELD_FLAME) // "Why 3?" We can't blend sounds so this is the best shit I've got
			S_StartSoundAtVolume(player->mo, sfx_fshld3, 255/3);
	}

	if (player->counterdash)
		player->counterdash--;

	if (player->sneakertimer && player->wipeoutslow > 0 && player->wipeoutslow < wipeoutslowtime+1)
		player->wipeoutslow = wipeoutslowtime+1;

	if (player->floorboost > 0)
		player->floorboost--;

	if (player->driftboost)
		player->driftboost--;

	if (player->strongdriftboost)
		player->strongdriftboost--;

	if (player->gateBoost)
		player->gateBoost--;

	if (player->powerup.rhythmBadgeTimer > 0)
		player->powerup.rhythmBadgeTimer--;

	if (player->powerup.barrierTimer > 0)
	{
		player->powerup.barrierTimer--;
	}

	if (player->powerup.superTimer > 0)
	{
		player->powerup.superTimer--;
	}

	if (K_PlayerGuard(player))
	{
		if (!player->oldGuard)
			S_StartSound(player->mo, sfx_s1af);

		player->oldGuard = true;
	}
	else if (player->oldGuard)
	{
		player->defenseLockout = PUNISHWINDOW;
		player->oldGuard = false;
	}

	if (player->startboost > 0 && onground == true)
	{
		player->startboost--;
	}
	if (player->dropdashboost)
		player->dropdashboost--;

	if (player->wavedashboost > 0 && onground == true)
	{
		player->wavedashboost--;
	}

	if (player->wavedashboost == 0 || player->wavedashpower > FRACUNIT)
	{
		player->wavedashpower = FRACUNIT; // Safety
	}

	if (player->speedpunt)
		player->speedpunt--;

	// This timer can get out of control fast, clamp to match player expectations about "new" hazards
	if (player->speedpunt > TICRATE*4)
		player->speedpunt = TICRATE*4;

	if (player->trickcharge > 0 && onground == true)
	{
		player->trickcharge--;
		if (player->drift)
			player->trickcharge = max(player->trickcharge, 1);
		if (gametyperules & GTR_SPHERES && (leveltime % 10 == 0))
			player->spheres++;
	}

	if (player->infinitether > 0)
	{
		player->infinitether--;
	}

	if (player->spindashboost)
	{
		player->spindashboost--;

		if (player->spindashboost <= 0)
		{
			player->spindashspeed = player->spindashboost = 0;
		}
	}

	if (player->invincibilitytimer && (player->ignoreAirtimeLeniency > 0 || onground == true || K_PowerUpRemaining(player, POWERUP_SMONITOR)))
		player->invincibilitytimer--;

	if (!player->invincibilitytimer)
		player->invincibilityextensions = 0;

	if (player->preventfailsafe)
		player->preventfailsafe--;

	if (player->tripwireUnstuck > 150)
	{
		player->tripwireUnstuck = 0;
		K_DoIngameRespawn(player);
	}

	// Don't tick down while in damage state.
	// There may be some maps where the timer activates for
	// a moment during normal play, but would quickly correct
	// itself when the player drives forward.
	// If the player is in a damage state, they may not be
	// able to move in time.
	// Always let the respawn prompt appear.
	if (player->bigwaypointgap && (player->bigwaypointgap > AUTORESPAWN_THRESHOLD || !P_PlayerInPain(player)))
	{
		player->bigwaypointgap--;
		if (!player->bigwaypointgap)
			K_DoIngameRespawn(player);
		else if (player->bigwaypointgap == AUTORESPAWN_THRESHOLD)
			K_AddMessageForPlayer(player, "Press \xAE to respawn", true, false);
	}

	if (player->tripwireUnstuck && !player->mo->hitlag)
		player->tripwireUnstuck--;

	if ((player->respawn.state == RESPAWNST_NONE) && player->growshrinktimer != 0)
	{
		if (player->growshrinktimer > 0 && (onground == true || player->ignoreAirtimeLeniency > 0))
			player->growshrinktimer--;
		if (player->growshrinktimer < 0)
			player->growshrinktimer++;

		// Back to normal
		if (player->growshrinktimer == 0)
			K_RemoveGrowShrink(player);
	}

	if (player->respawn.state != RESPAWNST_MOVE && (player->cmd.buttons & BT_RESPAWN) == BT_RESPAWN)
	{
		player->finalfailsafe++; // Decremented by ringshooter to "freeze" this timer
		// Part-way through the auto-respawn timer, you can tap Ring Shooter to respawn early
		if (player->finalfailsafe >= 4*TICRATE ||
			(player->bigwaypointgap && player->bigwaypointgap < AUTORESPAWN_THRESHOLD))
		{
			K_DoIngameRespawn(player);
			player->finalfailsafe = 0;
		}
	}
	else
	{
		player->finalfailsafe = 0;
	}

	if (player->ignoreAirtimeLeniency)
		player->ignoreAirtimeLeniency--;

	if (player->freeRingShooterCooldown && !player->mo->hitlag)
		player->freeRingShooterCooldown--;

	if (player->superring)
	{
		player->nextringaward++;
		UINT8 ringrate = 3 - min(2, player->superring / 20); // Used to consume fat stacks of cash faster.
		if (player->nextringaward >= ringrate)
		{
			mobj_t *ring = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_RING);
			ring->extravalue1 = 1; // Ring collect animation timer
			ring->angle = player->mo->angle; // animation angle
			P_SetTarget(&ring->target, player->mo); // toucher for thinker
			player->pickuprings++;
			if (player->superring == 1)
				ring->cvmem = 1; // play caching when collected
			player->nextringaward = 0;
			player->superring--;
		}
	}
	else
	{
		player->nextringaward = 99; // Next time we need to award superring, spawn the first one instantly.
	}

	if (player->pflags & PF_VOID && player->mo->hitlag == 0) // Returning from FAULT VOID
	{
		player->pflags &= ~PF_VOID;
		player->mo->renderflags &= ~RF_DONTDRAW;
		player->mo->flags &= ~MF_NOCLIPTHING;
		player->mo->momx = 0;
		player->mo->momy = 0;
		player->mo->momz = 0;
		player->nocontrol = 0;
		player->driftboost = 0;
		player->strongdriftboost = 0;
		player->tiregrease = 0;
		player->sneakertimer = 0;
		player->spindashboost = 0;
		player->flashing = TICRATE/2;
		player->ringboost = 0;
		player->driftboost = player->strongdriftboost = 0;
		player->gateBoost = 0;
	}

	if (player->pflags & PF_FAULT && player->nocontrol) // Hold player on respawn platform, no fair skipping long POSITION areas
	{
		if (rainbowstartavailable && ((leveltime <= starttime) || (leveltime - starttime < 10*TICRATE)))
		{
			player->nocontrol = TICRATE/2;
			player->mo->renderflags |= RF_DONTDRAW;
			player->mo->flags |= MF_NOCLIPTHING;
		}
	}

	// Players that bounce far off walls get reduced Top accel, to give them some time to get their bearings.
	if ((player->mo->eflags & MFE_JUSTBOUNCEDWALL) && player->curshield == KSHIELD_TOP)
	{
		angle_t topdelta = player->mo->angle - K_MomentumAngle(player->mo);
		fixed_t topmult = FINECOSINE(topdelta >> ANGLETOFINESHIFT);
		topmult = (topmult/2) + (FRACUNIT/2); // 0 to original
		player->topAccel = FixedMul(topmult, player->topAccel);
	}

	player->topAccel = min(player->topAccel + TOPACCELREGEN, MAXTOPACCEL);

	if (player->stealingtimer == 0
		&& player->rocketsneakertimer
		&& onground == true)
		player->rocketsneakertimer--;

	if (player->hyudorotimer)
		player->hyudorotimer--;

	if (player->bumpUnstuck > 30*5)
	{
		player->bumpUnstuck = 0;
		K_DoIngameRespawn(player);
	}
	else if (player->bumpUnstuck)
	{
		player->bumpUnstuck--;
	}

	if (player->fakeBoost)
		player->fakeBoost--;

	if (player->bumperinflate && player->mo->hitlag == 0)
	{
		fixed_t thrustdelta = MAXCOMBOTHRUST - MINCOMBOTHRUST;
		fixed_t floatdelta = MAXCOMBOFLOAT - MINCOMBOFLOAT;

		fixed_t thrustpertic = thrustdelta / MAXCOMBOTIME;
		fixed_t floatpertic = floatdelta / MAXCOMBOTIME;

		fixed_t totalthrust = thrustpertic * player->progressivethrust + MINCOMBOTHRUST;
		fixed_t totalfloat = floatpertic * player->progressivethrust + MINCOMBOFLOAT;

		if (player->speed > K_GetKartSpeed(player, false, false))
			totalthrust = 0;

		if (player->tumbleBounces && player->tumbleBounces <= TUMBLEBOUNCES)
		{
			player->mo->momz += totalfloat;
			P_Thrust(player->mo, K_MomentumAngle(player->mo), totalthrust/2);
		}
		else
		{
			P_Thrust(player->mo, K_MomentumAngle(player->mo), totalthrust);
		}

		player->bumperinflate--;
	}

	if (player->ringvolume < MINRINGVOLUME)
		player->ringvolume = MINRINGVOLUME;
	else if (MAXRINGVOLUME - player->ringvolume < RINGVOLUMEREGEN)
		player->ringvolume = MAXRINGVOLUME;
	else
		player->ringvolume += RINGVOLUMEREGEN;

	// :D
	if (player->ringtransparency < MINRINGTRANSPARENCY)
		player->ringtransparency = MINRINGTRANSPARENCY;
	else if (MAXRINGTRANSPARENCY - player->ringtransparency < RINGTRANSPARENCYREGEN)
		player->ringtransparency = MAXRINGTRANSPARENCY;
	else
		player->ringtransparency += RINGTRANSPARENCYREGEN;

	if (player->sadtimer)
		player->sadtimer--;

	if (player->stealingtimer > 0)
		player->stealingtimer--;
	else if (player->stealingtimer < 0)
		player->stealingtimer++;

	if (player->justbumped > 0)
		player->justbumped--;

	if (player->noEbrakeMagnet > 0)
		player->noEbrakeMagnet--;

	if (player->defenseLockout)
	{
		player->instaWhipCharge = 0;
		player->defenseLockout--;
	}

	UINT16 normalturn = abs(cmd->turning);
	UINT16 normalaim = abs(cmd->throwdir);

	if (normalturn != 0 || normalaim != 0)
	{
		if (normalturn != KART_FULLTURN && normalturn != KART_FULLTURN/2 && normalturn != 0)
			player->analoginput = true;
		if (normalaim != KART_FULLTURN && normalaim != KART_FULLTURN/2 && normalaim != 0)
			player->analoginput = true;
		if (normalturn == KART_FULLTURN/2 && normalaim == KART_FULLTURN)
			player->analoginput = false;
	}

	if (player->powerupVFXTimer > 0)
	{
		player->powerupVFXTimer--;
	}

	if (player->dotrickfx && !player->mo->hitlag)
	{
		int i;
		S_StartSoundAtVolume(player->mo, sfx_trick1, 255/2);

		if (!player->trickcharge)
		{
			for(i = 0;i < 5;i++)
			{
				mobj_t *aura = P_SpawnMobjFromMobj(player->mo, 0, 0, player->mo->height/2, MT_CHARGEAURA);
				aura->eflags &= ~MFE_VERTICALFLIP;
				aura->angle = player->mo->angle + i*ANG15;
				P_SetTarget(&aura->target, player->mo);
				if (i == 0)
					aura->extravalue2 = 1;
				else
					aura->renderflags |= RF_TRANS50;
				aura->cvmem = leveltime;
			}
		}

		player->trickcharge = 8*TICRATE;

		player->dotrickfx = false;
	}

	if (player->stingfx && !player->mo->hitlag)
	{
		S_StartSound(player->mo, sfx_s226l);
		player->stingfx = false;
	}

	// Don't screw up chain ring pickup/usage with instawhip charge.
	// If the button stays held, delay charge a bit.
	if (player->instaWhipChargeLockout)
		player->instaWhipChargeLockout--;
	if (player->rings > 0 || player->itemamount || player->ringdelay || player->rocketsneakertimer || player->ringboxdelay)
		player->instaWhipChargeLockout = INSTAWHIP_HOLD_DELAY;
	else if (!(player->cmd.buttons & BT_ATTACK)) // Deliberate Item button release, no need to protect you from lockout
		player->instaWhipChargeLockout = 0;

	// OOPSIES FIXME pt.2: See OOPSIES FIXME pt.1.
	// Don't draw attention to a useless, invisible whip charge!
	if (player->instaWhipCharge && player->instaWhipCharge < INSTAWHIP_CHARGETIME && !player->itemRoulette.active)
	{
		if (!S_SoundPlaying(player->mo, sfx_wchrg1))
			S_StartSoundAtVolume(player->mo, sfx_wchrg1, 255/2);
	}
	else
	{
		S_StopSoundByID(player->mo, sfx_wchrg1);
	}

	if (player->instaWhipCharge >= INSTAWHIP_CHARGETIME)
	{
		if (!S_SoundPlaying(player->mo, sfx_wchrg2))
			S_StartSoundAtVolume(player->mo, sfx_wchrg2, 255/3);
	}
	else
	{
		S_StopSoundByID(player->mo, sfx_wchrg2);
	}

	if (player->itemamount || player->respawn.state != RESPAWNST_NONE || player->itemflags & (IF_ITEMOUT|IF_EGGMANOUT) || player->rocketsneakertimer || player->ringboxdelay)
		player->instaWhipCharge = 0;

	if (player->tiregrease)
	{
		// Remove grease faster if players are moving slower; players that are recovering
		// from mistakes (or who got sprung purely for track traversal) need steering!
		// Up to 4x degrease speed below 10FU/t (speed at which you lose drift sparks).
		INT16 toDegrease = 1;
		INT16 driftSpeedIncrements = player->speed / (10 * player->mo->scale); // Same breakpoints used for driftcharge stages.
		toDegrease += max(3 - driftSpeedIncrements, 0);

		if (player->tiregrease <= toDegrease)
			player->tiregrease = 0;
		else
			player->tiregrease -= toDegrease;
	}

	if (player->spinouttimer || player->tumbleBounces)
	{
		if (player->ballhogcharge)
			player->ballhogcharge = 0;

		if (player->progressivethrust < MAXCOMBOTIME)
			player->progressivethrust++;
		if (player->incontrol > 0)
			player->incontrol = 0;
		player->incontrol--;
	}
	else
	{
		if (player->progressivethrust)
			player->progressivethrust--;
		if (player->incontrol < 0)
			player->incontrol = 0;
		player->incontrol++;
	}

	if (player->rings <= 0)
	{
		if (player->ringvisualwarning > 1)
			player->ringvisualwarning--;
	}
	else
	{
		player->ringvisualwarning = 0;
	}

	if (player->ringvisualwarning == 0 && player->rings <= 0)
	{
		player->ringvisualwarning = 6*TICRATE/2;
	}

	player->incontrol = min(player->incontrol, 5*TICRATE);
	player->incontrol = max(player->incontrol, -5*TICRATE);

	if (player->incontrol == 3*TICRATE)
		player->pitblame = -1;

	if (P_PlayerInPain(player) || player->respawn.state != RESPAWNST_NONE)
	{
		player->lastpickuptype = -1; // got your ass beat, go grab anything
		player->defenseLockout = 0;	// and reenable defensive tools just in case
	}


	if (player->tumbleBounces > 0)
	{
		K_HandleTumbleSound(player);
		if (P_IsObjectOnGround(player->mo) && player->mo->momz * P_MobjFlip(player->mo) <= 0)
			K_HandleTumbleBounce(player);
	}

	K_UpdateTripwire(player);

	if (battleovertime.enabled)
	{
		fixed_t distanceToCenter = 0;

		if (battleovertime.radius > 0)
		{
			distanceToCenter = R_PointToDist2(player->mo->x, player->mo->y, battleovertime.x, battleovertime.y);
		}

		if (distanceToCenter + player->mo->radius > battleovertime.radius)
		{
			if (distanceToCenter - (player->mo->radius * 2) > battleovertime.radius &&
				(battleovertime.enabled >= 10*TICRATE) &&
				!(player->pflags & PF_ELIMINATED) &&
				!player->exiting)
			{
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_TIMEOVER);
			}

			if (!player->exiting && !(player->pflags & PF_ELIMINATED))
			{
				if (leveltime < player->darkness_end)
				{
					if (leveltime > player->darkness_end - DARKNESS_FADE_TIME)
					{
						player->darkness_start = leveltime - (player->darkness_end - leveltime);
					}
				}
				else
				{
					player->darkness_start = leveltime;
				}

				player->darkness_end = leveltime + (2 * DARKNESS_FADE_TIME);
			}
		}
	}

	extern consvar_t cv_fuzz;
	if (cv_fuzz.value && P_CanPickupItem(player, 1))
	{
		K_StartItemRoulette(player, P_RandomRange(PR_FUZZ, 0, 1));
	}

	if (player->instashield)
		player->instashield--;

	if (player->justDI)
	{
		player->justDI--;

		// return turning if player is fully actionable, no matter when!
		if (!P_PlayerInPain(player))
			player->justDI = 0;
	}

	if (player->eggmanexplode)
	{
		if (player->spectator)
			player->eggmanexplode = 0;
		else
		{
			player->eggmanexplode--;

			if (!S_SoundPlaying(player->mo, sfx_kc51))
				S_StartSound(player->mo, sfx_kc51);
			if (player->eggmanexplode == 5*TICRATE/2)
				S_StartSound(player->mo, sfx_s3k53);

			if (player->eggmanexplode <= 0)
			{
				mobj_t *eggsexplode;

				K_KartResetPlayerColor(player);

				//player->flashing = 0;
				eggsexplode = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SPBEXPLOSION);
				eggsexplode->height = 2 * player->mo->height;
				K_FlipFromObject(eggsexplode, player->mo);

				S_StopSoundByID(player->mo, sfx_s3k53);
				S_StopSoundByID(player->mo, sfx_kc51);

				eggsexplode->threshold = KITEM_EGGMAN;

				P_SetTarget(&eggsexplode->tracer, player->mo);

				if (player->eggmanblame >= 0
				&& player->eggmanblame < MAXPLAYERS
				&& playeringame[player->eggmanblame]
				&& !players[player->eggmanblame].spectator
				&& players[player->eggmanblame].mo)
					P_SetTarget(&eggsexplode->target, players[player->eggmanblame].mo);
			}
		}
	}

	if (player->itemtype == KITEM_BUBBLESHIELD)
	{
		if (player->bubblecool)
			player->bubblecool--;
	}
	else
	{
		player->bubbleblowup = 0;
		player->bubblecool = 0;
	}

	if (player->itemtype != KITEM_FLAMESHIELD)
	{
		if (player->flamedash)
			K_FlameDashLeftoverSmoke(player->mo);
	}

	if (P_IsObjectOnGround(player->mo) && player->trickpanel != TRICKSTATE_NONE)
	{
		if (P_MobjFlip(player->mo) * player->mo->momz <= 0)
		{
			player->trickpanel = TRICKSTATE_NONE;
		}
	}

	if (cmd->buttons & BT_DRIFT)
	{
		if (player->curshield == KSHIELD_TOP)
		{
			if (player->topdriftheld <= GARDENTOP_MAXGRINDTIME)
				player->topdriftheld++;

			// Squish :)
			player->mo->spritexscale = 6*FRACUNIT/4;
			player->mo->spriteyscale = 2*FRACUNIT/4;

			if (leveltime & 1)
				K_SpawnGardenTopSpeedLines(player);
		}
		// Only allow drifting while NOT trying to do an spindash input.
		else if (K_PressingEBrake(player) == false)
		{
			player->pflags |= PF_DRIFTINPUT;
		}
		// else, keep the previous value, because it might be brake-drifting.
	}
	else
	{
		player->pflags &= ~PF_DRIFTINPUT;
	}

	// Roulette Code
	K_KartItemRoulette(player, cmd);

	// Handle invincibility sfx
	K_UpdateInvincibilitySounds(player); // Also thanks, VAda!

	if (player->tripwireState != TRIPSTATE_NONE)
	{
		if (player->tripwireState == TRIPSTATE_PASSED)
			S_StartSound(player->mo, sfx_cdfm63);
		else if (player->tripwireState == TRIPSTATE_BLOCKED)
			S_StartSound(player->mo, sfx_kc4c);

		player->tripwireState = TRIPSTATE_NONE;
	}

	// If the player is out of the game, these visuals may
	// look really strange.
	if (player->spectator == false && !(player->pflags & PF_NOCONTEST))
	{
		K_KartEbrakeVisuals(player);

		Obj_ServantHandSpawning(player);
	}

	if (K_GetKartButtons(player) & BT_BRAKE &&
			P_IsObjectOnGround(player->mo) &&
			K_GetKartSpeed(player, false, false) / 2 <= player->speed)
	{
		K_SpawnBrakeVisuals(player);
	}
	else
	{
		player->mo->spriteyoffset = 0;
	}

	K_HandleDelayedHitByEm(player);

	player->pflags &= ~PF_POINTME;

	if (player->icecube.frozen && player->icecube.shaketimer)
	{
		player->mo->sprxoff += P_RandomRange(PR_DECORATION, -4, 4) * player->mo->scale;
		player->mo->spryoff += P_RandomRange(PR_DECORATION, -4, 4) * player->mo->scale;
		player->mo->sprzoff += P_RandomRange(PR_DECORATION, -4, 4) * player->mo->scale;

		player->icecube.shaketimer--;
	}

	if ((player->mo->eflags & MFE_UNDERWATER) && player->curshield != KSHIELD_BUBBLE)
	{
		if (player->breathTimer < UINT16_MAX)
			player->breathTimer++;
	}

	if (spbplace == -1 && modeattacking & ATTACKING_SPB && !player->mo->hitlag && player->mo->health && !P_PlayerInPain(player) && player->laps > 0 && !player->exiting && !(player->pflags & PF_NOCONTEST))
	{
		// I'd like this to play a sound to make the situation clearer, but this codepath seems
		// to run even when you make standard contact with the SPB. Not sure why. Oh well!
		// S_StartSound(NULL, sfx_s26d);
		P_DamageMobj(player->mo, NULL, NULL, 1, DMG_INSTAKILL);
	}
}

void K_KartResetPlayerColor(player_t *player)
{
	boolean fullbright = false;

	if (!player->mo || P_MobjWasRemoved(player->mo)) // Can't do anything
		return;

	if (player->mo->health <= 0 || player->playerstate == PST_DEAD || (player->respawn.state == RESPAWNST_MOVE)) // Override everything
	{
		player->mo->colorized = (player->dye != 0);
		player->mo->color = player->dye ? player->dye : player->skincolor;
		goto finalise;
	}

	if (player->eggmanexplode) // You're gonna diiiiie
	{
		const INT32 flashtime = 4<<(player->eggmanexplode/TICRATE);
		if (player->eggmanexplode % (flashtime/2) != 0)
		{
			;
		}
		else if (player->eggmanexplode % flashtime == 0)
		{
			player->mo->colorized = true;
			player->mo->color = SKINCOLOR_BLACK;
			fullbright = true;
			goto finalise;
		}
		else
		{
			player->mo->colorized = true;
			player->mo->color = SKINCOLOR_CRIMSON;
			fullbright = true;
			goto finalise;
		}
	}

	if (player->invincibilitytimer) // You're gonna kiiiiill
	{
		const tic_t defaultTime = itemtime+(2*TICRATE);
		tic_t flicker = 2;
		boolean skip = false;

		fullbright = true;

		if (player->invincibilitytimer > defaultTime)
		{
			player->mo->color = K_RainbowColor(leveltime / 2);
			player->mo->colorized = true;
			skip = true;
		}
		else
		{
			flicker += (defaultTime - player->invincibilitytimer) / TICRATE / 2;
		}

		if (leveltime % flicker == 0)
		{
			player->mo->color = SKINCOLOR_INVINCFLASH;
			player->mo->colorized = true;
			skip = true;
		}

		if (skip)
		{
			goto finalise;
		}
	}

	if (player->growshrinktimer) // Ditto, for grow/shrink
	{
		if (player->growshrinktimer % 5 == 0)
		{
			player->mo->colorized = true;
			player->mo->color = (player->growshrinktimer < 0 ? SKINCOLOR_CREAMSICLE : SKINCOLOR_PERIWINKLE);
			fullbright = true;
			goto finalise;
		}
	}

	if (player->eggmanTransferDelay)
	{
		player->mo->colorized = true;
		if (player->eggmanTransferDelay % 2)
		{
			player->mo->color = SKINCOLOR_BLACK;
		}
		else
		{
			player->mo->color = SKINCOLOR_CRIMSON;
		}
		goto finalise;
	}

	if (player->ringboost && (leveltime & 1)) // ring boosting
	{
		player->mo->colorized = true;
		fullbright = true;
		goto finalise;
	}

	if (player->icecube.frozen)
	{
		player->mo->colorized = true;
		player->mo->color = SKINCOLOR_CYAN;
		goto finalise;
	}

	player->mo->colorized = (player->dye != 0);
	player->mo->color = player->dye ? player->dye : player->skincolor;

finalise:

	if (player->curshield && player->curshield != KSHIELD_TOP)
	{
		fullbright = true;
	}

	if (fullbright == true)
	{
		player->mo->frame |= FF_FULLBRIGHT;
	}
	else
	{
		if (!(player->mo->state->frame & FF_FULLBRIGHT))
			player->mo->frame &= ~FF_FULLBRIGHT;
	}
}

void K_KartPlayerAfterThink(player_t *player)
{
	K_KartResetPlayerColor(player);

	K_UpdateStumbleIndicator(player);
	K_UpdateWavedashIndicator(player);
	K_UpdateTrickIndicator(player);

	// Move held objects (Bananas, Orbinaut, etc)
	K_MoveHeldObjects(player);

	// Jawz reticule (seeking)
	if (player->itemtype == KITEM_JAWZ && (player->itemflags & IF_ITEMOUT))
	{
		const INT32 lastTargID = player->lastjawztarget;
		mobj_t *lastTarg = NULL;

		INT32 targID = MAXPLAYERS;
		mobj_t *targ = NULL;

		mobj_t *ret = NULL;

		if (specialstageinfo.valid == true
			&& lastTargID == MAXPLAYERS)
		{
			// Aiming at the UFO (but never the emerald).
			lastTarg = K_GetPossibleSpecialTarget();
		}
		else if ((lastTargID >= 0 && lastTargID <= MAXPLAYERS)
			&& playeringame[lastTargID] == true)
		{
			if (players[lastTargID].spectator == false)
			{
				lastTarg = players[lastTargID].mo;
			}
		}

		if (player->throwdir == -1)
		{
			// Backwards Jawz targets yourself.
			targ = player->mo;
			player->jawztargetdelay = 0;
		}
		else
		{
			// Find a new target.
			targ = K_FindJawzTarget(player->mo, player, ANGLE_45);
		}

		if (targ != NULL && P_MobjWasRemoved(targ) == false)
		{
			if (targ->player != NULL)
			{
				targID = targ->player - players;
			}

			if (targID == lastTargID)
			{
				// Increment delay.
				if (player->jawztargetdelay < 10)
				{
					player->jawztargetdelay++;
				}
			}
			else
			{
				if (player->jawztargetdelay > 0)
				{
					// Wait a bit before swapping...
					player->jawztargetdelay--;
					targ = lastTarg;
				}
				else
				{
					// Allow a swap.
					if (P_IsDisplayPlayer(player) || P_IsDisplayPlayer(targ->player))
					{
						S_StartSound(NULL, sfx_s3k89);
					}
					else
					{
						S_StartSound(targ, sfx_s3k89);
					}

					player->lastjawztarget = targID;
					player->jawztargetdelay = 5;
				}
			}
		}

		if (targ == NULL || P_MobjWasRemoved(targ) == true)
		{
			player->lastjawztarget = -1;
			player->jawztargetdelay = 0;
			return;
		}

		ret = P_SpawnMobj(targ->x, targ->y, targ->z, MT_PLAYERRETICULE);
		ret->sprzoff = targ->sprzoff;
		ret->old_x = targ->old_x;
		ret->old_y = targ->old_y;
		ret->old_z = targ->old_z;
		P_SetTarget(&ret->target, targ);
		ret->frame |= ((leveltime % 10) / 2);
		ret->tics = 1;
		ret->color = player->skincolor;
	}
	else
	{
		player->lastjawztarget = -1;
		player->jawztargetdelay = 0;
	}

	if (player->itemtype == KITEM_LIGHTNINGSHIELD || ((gametyperules & GTR_POWERSTONES) && K_IsPlayerWanted(player)))
	{
		K_LookForRings(player->mo);
	}

	if (player->nullHitlag > 0)
	{
		if (!P_MobjWasRemoved(player->mo))
		{
			player->mo->hitlag -= player->nullHitlag;
		}

		player->nullHitlag = 0;
	}
}

/*--------------------------------------------------
	static boolean K_SetPlayerNextWaypoint(player_t *player)

		Sets the next waypoint of a player, by finding their closest waypoint, then checking which of itself and next or
		previous waypoints are infront of the player.
		Also sets the current waypoint.

	Input Arguments:-
		player - The player the next waypoint is being found for

	Return:-
		Whether it is safe to update the respawn waypoint
--------------------------------------------------*/
static boolean K_SetPlayerNextWaypoint(player_t *player)
{
	waypoint_t *finishline = K_GetFinishLineWaypoint();
	waypoint_t *bestwaypoint = NULL;
	boolean    updaterespawn = false;

	if ((player != NULL) && (player->mo != NULL) && (P_MobjWasRemoved(player->mo) == false))
	{
		waypoint_t *waypoint     = K_GetBestWaypointForMobj(player->mo, player->currentwaypoint);

		// Our current waypoint.
		bestwaypoint = waypoint;

		if (bestwaypoint != NULL)
		{
			player->currentwaypoint = bestwaypoint;
		}

		// check the waypoint's location in relation to the player
		// If it's generally in front, it's fine, otherwise, use the best next/previous waypoint.
		// EXCEPTION: If our best waypoint is the finishline AND we're facing towards it, don't do this.
		// Otherwise it breaks the distance calculations.
		if (waypoint != NULL)
		{
			angle_t playerangle     = player->mo->angle;
			angle_t momangle        = K_MomentumAngle(player->mo);
			angle_t angletowaypoint =
				R_PointToAngle2(player->mo->x, player->mo->y, waypoint->mobj->x, waypoint->mobj->y);
			angle_t angledelta      = ANGLE_180;
			angle_t momdelta        = ANGLE_180;

			angledelta = playerangle - angletowaypoint;
			if (angledelta > ANGLE_180)
			{
				angledelta = InvAngle(angledelta);
			}

			momdelta = momangle - angletowaypoint;
			if (momdelta > ANGLE_180)
			{
				momdelta = InvAngle(momdelta);
			}

			// We're using a lot of angle calculations here, because only using facing angle or only using momentum angle both have downsides.
			// nextwaypoints will be picked if you're facing OR moving forward.
			// prevwaypoints will be picked if you're facing AND moving backward.
#if 0
			if (angledelta > ANGLE_45 || momdelta > ANGLE_45)
#endif
			{
				angle_t nextbestdelta = ANGLE_90;
				angle_t nextbestmomdelta = ANGLE_90;
				angle_t nextbestanydelta = ANGLE_MAX;
				size_t i = 0U;

				if ((waypoint->nextwaypoints != NULL) && (waypoint->numnextwaypoints > 0U))
				{
					for (i = 0U; i < waypoint->numnextwaypoints; i++)
					{
						if (!K_GetWaypointIsEnabled(waypoint->nextwaypoints[i]))
						{
							continue;
						}

						if (K_PlayerUsesBotMovement(player) == true
						&& K_GetWaypointIsShortcut(waypoint->nextwaypoints[i]) == true
						&& K_BotCanTakeCut(player) == false)
						{
							// Bots that aren't able to take a shortcut will ignore shortcut waypoints.
							// (However, if they're already on a shortcut, then we want them to keep going.)

							if (player->nextwaypoint != NULL
							&& K_GetWaypointIsShortcut(player->nextwaypoint) == false)
							{
								continue;
							}
						}

						angletowaypoint = R_PointToAngle2(
							player->mo->x, player->mo->y,
							waypoint->nextwaypoints[i]->mobj->x, waypoint->nextwaypoints[i]->mobj->y);

						angledelta = playerangle - angletowaypoint;
						if (angledelta > ANGLE_180)
						{
							angledelta = InvAngle(angledelta);
						}

						momdelta = momangle - angletowaypoint;
						if (momdelta > ANGLE_180)
						{
							momdelta = InvAngle(momdelta);
						}

						if (angledelta < nextbestanydelta || momdelta < nextbestanydelta)
						{
							nextbestanydelta = min(angledelta, momdelta);
							player->besthanddirection = angletowaypoint;

							if (nextbestanydelta >= ANGLE_90)
								continue;

							// Wanted to use a next waypoint, so remove WRONG WAY flag.
							// Done here instead of when set, because of finish line
							// hacks meaning we might not actually use this one, but
							// we still want to acknowledge we're facing the right way.
							player->pflags &= ~PF_WRONGWAY;

							if (waypoint->nextwaypoints[i] != finishline) // Allow finish line.
							{
								if (P_TraceWaypointTraversal(player->mo, waypoint->nextwaypoints[i]->mobj) == false)
								{
									// Save sight checks when all of the other checks pass, so we only do it if we have to
									continue;
								}
							}

							bestwaypoint = waypoint->nextwaypoints[i];

							if (angledelta < nextbestdelta)
							{
								nextbestdelta = angledelta;
							}
							if (momdelta < nextbestmomdelta)
							{
								nextbestmomdelta = momdelta;
							}

							updaterespawn = true;
						}
					}
				}

				if ((waypoint->prevwaypoints != NULL) && (waypoint->numprevwaypoints > 0U)
				&& !(K_PlayerUsesBotMovement(player))) // Bots do not need prev waypoints
				{
					for (i = 0U; i < waypoint->numprevwaypoints; i++)
					{
						if (!K_GetWaypointIsEnabled(waypoint->prevwaypoints[i]))
						{
							continue;
						}

						angletowaypoint = R_PointToAngle2(
							player->mo->x, player->mo->y,
							waypoint->prevwaypoints[i]->mobj->x, waypoint->prevwaypoints[i]->mobj->y);

						angledelta = playerangle - angletowaypoint;
						if (angledelta > ANGLE_180)
						{
							angledelta = InvAngle(angledelta);
						}

						momdelta = momangle - angletowaypoint;
						if (momdelta > ANGLE_180)
						{
							momdelta = InvAngle(momdelta);
						}

						if (angledelta < nextbestdelta && momdelta < nextbestmomdelta)
						{
							if (waypoint->prevwaypoints[i] == finishline) // NEVER allow finish line.
							{
								continue;
							}

							if (P_TraceWaypointTraversal(player->mo, waypoint->prevwaypoints[i]->mobj) == false)
							{
								// Save sight checks when all of the other checks pass, so we only do it if we have to
								continue;
							}

							bestwaypoint = waypoint->prevwaypoints[i];

							nextbestdelta = angledelta;
							nextbestmomdelta = momdelta;

							// Set wrong way flag if we're using prevwaypoints
							player->pflags |= PF_WRONGWAY;
							updaterespawn = false;
						}
					}
				}
			}
		}

		if (!P_IsObjectOnGround(player->mo))
		{
			updaterespawn = false;
		}

		if (player->pflags & PF_UPDATEMYRESPAWN)
		{
			updaterespawn = true;
			player->pflags &= ~PF_UPDATEMYRESPAWN;
		}

		// If nextwaypoint is NULL, it means we don't want to update the waypoint until we touch another one.
		// player->nextwaypoint will keep its previous value in this case.
		if (bestwaypoint != NULL)
		{
			player->nextwaypoint = bestwaypoint;
		}
	}

	return updaterespawn;
}

/*--------------------------------------------------
	static void K_UpdateDistanceFromFinishLine(player_t *const player)

		Updates the distance a player has to the finish line.

	Input Arguments:-
		player - The player the distance is being updated for

	Return:-
		None
--------------------------------------------------*/
static void K_UpdateDistanceFromFinishLine(player_t *const player)
{
	if ((player != NULL) && (player->mo != NULL))
	{
		waypoint_t *finishline   = K_GetFinishLineWaypoint();

		// nextwaypoint is now the waypoint that is in front of us
		if ((player->exiting && !(player->pflags & PF_NOCONTEST)) || player->spectator)
		{
			// Player has finished, we don't need to calculate this
			player->distancetofinish = 0U;
		}
		else if (player->pflags & PF_NOCONTEST)
		{
			// We also don't need to calculate this, but there's also no need to destroy the data...
			;
		}
		else if ((player->currentwaypoint != NULL) && (player->nextwaypoint != NULL) && (finishline != NULL))
		{
			const boolean useshortcuts = false;
			const boolean huntbackwards = false;
			boolean pathfindsuccess = false;
			path_t pathtofinish = {0};

			pathfindsuccess =
				K_PathfindToWaypoint(player->nextwaypoint, finishline, &pathtofinish, useshortcuts, huntbackwards);

			// Update the player's distance to the finish line if a path was found.
			// Using shortcuts won't find a path, so distance won't be updated until the player gets back on track
			if (pathfindsuccess == true)
			{
				const boolean pathBackwardsReverse = ((player->pflags & PF_WRONGWAY) == 0);
				boolean pathBackwardsSuccess = false;
				path_t pathBackwards = {0};

				fixed_t disttonext = 0;
				UINT32 traveldist = 0;
				UINT32 adddist = 0;

				disttonext =
					P_AproxDistance(
						(player->mo->x >> FRACBITS) - (player->nextwaypoint->mobj->x >> FRACBITS),
						(player->mo->y >> FRACBITS) - (player->nextwaypoint->mobj->y >> FRACBITS));
				disttonext = P_AproxDistance(disttonext, (player->mo->z >> FRACBITS) - (player->nextwaypoint->mobj->z >> FRACBITS));

				traveldist = ((UINT32)disttonext) * 2;
				pathBackwardsSuccess =
					K_PathfindThruCircuit(player->nextwaypoint, traveldist, &pathBackwards, false, pathBackwardsReverse);

				if (pathBackwardsSuccess == true)
				{
					if (pathBackwards.numnodes > 1)
					{
						// Find the closest segment, and add the distance to reach it.
						vector3_t point;
						size_t i;

						vector3_t best;
						fixed_t bestPoint = INT32_MAX;
						fixed_t bestDist = INT32_MAX;
						UINT32 bestGScore = UINT32_MAX;

						point.x = player->mo->x;
						point.y = player->mo->y;
						point.z = player->mo->z;

						best.x = point.x;
						best.y = point.y;
						best.z = point.z;

						for (i = 1; i < pathBackwards.numnodes; i++)
						{
							vector3_t line[2];
							vector3_t result;

							waypoint_t *pwp = (waypoint_t *)pathBackwards.array[i - 1].nodedata;
							waypoint_t *wp = (waypoint_t *)pathBackwards.array[i].nodedata;

							fixed_t pDist = 0;
							UINT32 g = pathBackwards.array[i - 1].gscore;

							line[0].x = pwp->mobj->x;
							line[0].y = pwp->mobj->y;
							line[0].z = pwp->mobj->z;

							line[1].x = wp->mobj->x;
							line[1].y = wp->mobj->y;
							line[1].z = wp->mobj->z;

							P_ClosestPointOnLine3D(&point, line, &result);

							pDist = P_AproxDistance(point.x - result.x, point.y - result.y);
							pDist = P_AproxDistance(pDist, point.z - result.z);

							if (pDist < bestPoint)
							{
								FV3_Copy(&best, &result);

								bestPoint = pDist;

								bestDist =
									P_AproxDistance(
										(result.x >> FRACBITS) - (line[0].x >> FRACBITS),
										(result.y >> FRACBITS) - (line[0].y >> FRACBITS));
								bestDist = P_AproxDistance(bestDist, (result.z >> FRACBITS) - (line[0].z >> FRACBITS));

								bestGScore = g + ((UINT32)bestDist);
							}
						}

#if 0
						if (cv_kartdebugwaypoints.value)
						{
							mobj_t *debugmobj = P_SpawnMobj(best.x, best.y, best.z, MT_SPARK);
							P_SetMobjState(debugmobj, S_WAYPOINTORB);

							debugmobj->frame &= ~FF_TRANSMASK;
							debugmobj->frame |= FF_FULLBRIGHT; //FF_TRANS20

							debugmobj->tics = 1;
							debugmobj->color = SKINCOLOR_BANANA;
						}
#endif

						adddist = bestGScore;
					}
					/*
					else
					{
						// Only one point to work with, so just add your euclidean distance to that.
						waypoint_t *wp = (waypoint_t *)pathBackwards.array[0].nodedata;
						fixed_t disttowaypoint =
							P_AproxDistance(
								(player->mo->x >> FRACBITS) - (wp->mobj->x >> FRACBITS),
								(player->mo->y >> FRACBITS) - (wp->mobj->y >> FRACBITS));
						disttowaypoint = P_AproxDistance(disttowaypoint, (player->mo->z >> FRACBITS) - (wp->mobj->z >> FRACBITS));

						adddist = (UINT32)disttowaypoint;
					}
					*/
					Z_Free(pathBackwards.array);
				}
				/*
				else
				{
					// Fallback to adding euclidean distance to the next waypoint to the distancetofinish
					adddist = (UINT32)disttonext;
				}
				*/

				if (pathBackwardsReverse == false)
				{
					if (pathtofinish.totaldist > adddist)
					{
						player->distancetofinish = pathtofinish.totaldist - adddist;
					}
					else
					{
						player->distancetofinish = 0;
					}
				}
				else
				{
					player->distancetofinish = pathtofinish.totaldist + adddist;
				}
				Z_Free(pathtofinish.array);

				// distancetofinish is currently a flat distance to the finish line, but in order to be fully
				// correct we need to add to it the length of the entire circuit multiplied by the number of laps
				// left after this one. This will give us the total distance to the finish line, and allow item
				// distance calculation to work easily
				const mapheader_t *mapheader = mapheaderinfo[gamemap - 1];
				if ((mapheader->levelflags & LF_SECTIONRACE) == 0U)
				{
					const UINT8 numfulllapsleft = ((UINT8)numlaps - player->laps) / mapheader->lapspersection;
					player->distancetofinish += numfulllapsleft * K_GetCircuitLength();
				}
			}
		}
	}
}

static UINT32 u32_delta(UINT32 x, UINT32 y)
{
	return x > y ? x - y : y - x;
}

/*--------------------------------------------------
	static void K_UpdatePlayerWaypoints(player_t *const player)

		Updates the player's waypoints and finish line distance.

	Input Arguments:-
		player - The player to update

	Return:-
		None
--------------------------------------------------*/
static void K_UpdatePlayerWaypoints(player_t *const player)
{
	if (player->pflags & PF_FREEZEWAYPOINTS)
	{
		player->pflags &= ~PF_FREEZEWAYPOINTS;
		return;
	}

	const UINT32 distance_threshold = FixedMul(32768, mapobjectscale);

	waypoint_t *const old_currentwaypoint = player->currentwaypoint;
	waypoint_t *const old_nextwaypoint = player->nextwaypoint;

	boolean updaterespawn = K_SetPlayerNextWaypoint(player);

	// Update prev value (used for grief prevention code)
	player->distancetofinishprev = player->distancetofinish;
	K_UpdateDistanceFromFinishLine(player);

	UINT32 delta = u32_delta(player->distancetofinish, player->distancetofinishprev);
	if (delta > distance_threshold &&
		player->respawn.state == RESPAWNST_NONE && // Respawning should be a full reset.
		old_currentwaypoint != NULL && // So should touching the first waypoint ever.
		player->laps != 0 && // POSITION rooms may have unorthodox waypoints to guide bots.
		!(player->pflags & PF_TRUSTWAYPOINTS)) // Special exception.
	{
		extern consvar_t cv_debuglapcheat;
#define debug_args "Player %s: waypoint ID %d too far away (%u > %u)\n", \
		sizeu1(player - players), K_GetWaypointID(player->nextwaypoint), delta, distance_threshold
		if (cv_debuglapcheat.value)
			CONS_Printf(debug_args);
		else
			CONS_Debug(DBG_GAMELOGIC, debug_args);
#undef debug_args

		if (!cv_debuglapcheat.value)
		{
			// Distance jump is too great, keep the old waypoints and old distance.
			player->currentwaypoint = old_currentwaypoint;
			player->nextwaypoint = old_nextwaypoint;
			player->distancetofinish = player->distancetofinishprev;

			// Start the auto respawn timer when the distance jumps.
			if (!player->bigwaypointgap)
			{
				player->bigwaypointgap = AUTORESPAWN_TIME;
			}
		}
	}
	else
	{
		// Reset the auto respawn timer if distance changes are back to normal.
		if (player->bigwaypointgap && player->bigwaypointgap <= AUTORESPAWN_THRESHOLD + 1)
		{
			player->bigwaypointgap = 0;

			// While the player was in the "bigwaypointgap" state, laps did not change from crossing finish lines.
			// So reset the lap back to normal, in case they were able to get behind the line.
			player->laps = player->lastsafelap;
			player->cheatchecknum = player->lastsafecheatcheck;
		}
	}

	// Respawn point should only be updated when we're going to a nextwaypoint
	if ((updaterespawn) &&
	(player->bigwaypointgap == 0) &&
	(player->respawn.state == RESPAWNST_NONE) &&
	(player->nextwaypoint != old_nextwaypoint) &&
	(K_GetWaypointIsSpawnpoint(player->nextwaypoint)) &&
	(K_GetWaypointIsEnabled(player->nextwaypoint) == true))
	{
		player->respawn.wp = player->nextwaypoint;
		player->lastsafelap = player->laps;
		player->lastsafecheatcheck = player->cheatchecknum;
	}

	player->pflags &= ~PF_TRUSTWAYPOINTS; // clear special exception
}

INT32 K_GetKartRingPower(const player_t *player, boolean boosted)
{
	fixed_t ringPower = ((9 - player->kartspeed) + (9 - player->kartweight)) * (FRACUNIT/2);

	if (boosted == true)
	{
		ringPower = FixedMul(ringPower, K_RingDurationBoost(player));
	}

	return max(ringPower / FRACUNIT, 1);
}

// Returns false if this player being placed here causes them to collide with any other player
// Used in g_game.c for match etc. respawning
// This does not check along the z because the z is not correctly set for the spawnee at this point
boolean K_CheckPlayersRespawnColliding(INT32 playernum, fixed_t x, fixed_t y)
{
	INT32 i;
	fixed_t p1radius = players[playernum].mo->radius;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playernum == i || !playeringame[i] || players[i].spectator || !players[i].mo || players[i].mo->health <= 0
			|| players[i].playerstate != PST_LIVE || (players[i].mo->flags & MF_NOCLIP) || (players[i].mo->flags & MF_NOCLIPTHING))
			continue;

		if (abs(x - players[i].mo->x) < (p1radius + players[i].mo->radius)
			&& abs(y - players[i].mo->y) < (p1radius + players[i].mo->radius))
		{
			return false;
		}
	}
	return true;
}

// countersteer is how strong the controls are telling us we are turning
// turndir is the direction the controls are telling us to turn, -1 if turning right and 1 if turning left
static INT16 K_GetKartDriftValue(const player_t *player, fixed_t countersteer)
{
	INT16 basedrift, driftadjust;
	fixed_t driftweight = player->kartweight*14; // 12

	if (player->drift == 0 || !P_IsObjectOnGround(player->mo))
	{
		// If they aren't drifting or on the ground, this doesn't apply
		return 0;
	}

	if (player->pflags & PF_DRIFTEND)
	{
		// Drift has ended and we are tweaking their angle back a bit
		return -266*player->drift;
	}

	basedrift = (83 * player->drift) - (((driftweight - 14) * player->drift) / 5); // 415 - 303
	driftadjust = abs((252 - driftweight) * player->drift / 5);

	if (player->tiregrease > 0) // Buff drift-steering while in greasemode
	{
		basedrift += (basedrift / greasetics) * player->tiregrease;
	}

#if 0
	if (player->mo->eflags & MFE_UNDERWATER)
	{
		countersteer = FixedMul(countersteer, 3*FRACUNIT/2);
	}
#endif

	// Compat level for 2.0 staff ghosts
	if (G_CompatLevel(0x000A))
	{
		return basedrift + (FixedMul(driftadjust * FRACUNIT, countersteer) / FRACUNIT);
	}
	else
	{
		return basedrift + FixedMul(driftadjust, countersteer);
	}
}

INT16 K_UpdateSteeringValue(INT16 inputSteering, INT16 destSteering)
{
	// player->steering is the turning value, but with easing applied.
	// Keeps micro-turning from old easing, but isn't controller dependent.

	INT16 amount = KART_FULLTURN/3;
	INT16 diff = destSteering - inputSteering;
	INT16 outputSteering = inputSteering;


	// We switched steering directions, lighten up on easing for a more responsive countersteer.
	// (Don't do this for steering 0, let digital inputs tap-adjust!)
	if ((inputSteering > 0 && destSteering < 0) || (inputSteering < 0 && destSteering > 0))
	{
		// Don't let small turns in direction X allow instant turns in direction Y.
		INT16 countersteer = min(KART_FULLTURN, abs(inputSteering));  // The farthest we should go is to 0 -- neutral.
		amount = max(countersteer, amount); // But don't reduce turning strength from baseline either.
	}


	if (abs(diff) <= amount)
	{
		// Reached the intended value, set instantly.
		outputSteering = destSteering;
	}
	else
	{
		// Go linearly towards the value we wanted.
		if (diff < 0)
		{
			outputSteering -= amount;
		}
		else
		{
			outputSteering += amount;
		}
	}

	return outputSteering;
}

static fixed_t K_GetUnderwaterStrafeMul(const player_t *player)
{
	const fixed_t minSpeed = 11 * player->mo->scale;
	fixed_t baseline = INT32_MAX;

	baseline = 2 * K_GetKartSpeed(player, true, true) / 3;

	return max(0, FixedDiv(max(player->speed, minSpeed) - minSpeed, baseline - minSpeed));
}

INT16 K_GetKartTurnValue(const player_t *player, INT16 turnvalue)
{
	fixed_t turnfixed = turnvalue * FRACUNIT;

	fixed_t currentSpeed = 0;
	fixed_t p_maxspeed = INT32_MAX, p_speed = INT32_MAX;

	fixed_t weightadjust = INT32_MAX;

	if (player->mo == NULL || P_MobjWasRemoved(player->mo) || player->spectator || objectplacing)
	{
		// Invalid object, or incorporeal player. Return the value exactly.
		return turnvalue;
	}

	if (leveltime < introtime)
	{
		// No turning during the intro
		return 0;
	}

	if (player->respawn.state == RESPAWNST_MOVE)
	{
		// No turning during respawn
		return 0;
	}

	// Staff ghosts - direction-only trickpanel behavior
	if (G_CompatLevel(0x000A) || K_PlayerUsesBotMovement(player))
	{
		if (player->trickpanel == TRICKSTATE_READY || player->trickpanel == TRICKSTATE_FORWARD)
		{
			// Forward trick or rising from trickpanel
			return 0;
		}
	}

	if (player->justDI > 0)
	{
		// No turning until you let go after DI-ing.
		return 0;
	}

	if (Obj_PlayerRingShooterFreeze(player) == true)
	{
		// No turning while using Ring Shooter
		return 0;
	}

	currentSpeed = FixedHypot(player->mo->momx, player->mo->momy);

	if ((currentSpeed <= 0) // Not moving
	&& (K_PressingEBrake(player) == false) // Not e-braking
	&& (player->respawn.state == RESPAWNST_NONE) // Not respawning
	&& (player->curshield != KSHIELD_TOP) // Not riding a Top
	&& (P_IsObjectOnGround(player->mo) == true)) // On the ground
	{
		return 0;
	}

	p_maxspeed = K_GetKartSpeed(player, false, true);

	if (player->curshield == KSHIELD_TOP)
	{
		// Do not downscale turning speed with faster
		// movement speed; behaves as if turning in place.
		p_speed = 0;
	}
	else
	{
		if (G_CompatLevel(0x000A))
		{
			// Compat level for 2.0 staff ghosts
			p_speed = min(currentSpeed, p_maxspeed * 2);
		}
		else
		{
			// Turning dampens as you go faster, but at extremely high speeds, keeping some control is important.
			// Dampening is applied in two stages, one harsh and one soft.
			// The harsh window is larger for characters with better baseline maneuverability.
			fixed_t stageSpeed = min(currentSpeed, (110 + 2*(9-player->kartweight)) * p_maxspeed/100);
			if (stageSpeed < currentSpeed)
			{
				stageSpeed += (currentSpeed - stageSpeed) / 2;
			}

			p_speed = min(stageSpeed, p_maxspeed * 2);
		}
	}

	if (K_PodiumSequence() == true)
	{
		// Normalize turning for podium
		weightadjust = FixedDiv((p_maxspeed * 3), (p_maxspeed * 3) + (2 * FRACUNIT));
		turnfixed = FixedMul(turnfixed, weightadjust);
		turnfixed *= 2;
		return (turnfixed / FRACUNIT);
	}

	weightadjust = FixedDiv((p_maxspeed * 3) - p_speed, (p_maxspeed * 3) + (player->kartweight * FRACUNIT));

	if (K_PlayerUsesBotMovement(player))
	{
		turnfixed = FixedMul(turnfixed, 2*FRACUNIT); // Base increase to turning
	}

	if (player->drift != 0 && P_IsObjectOnGround(player->mo))
	{
		if (G_CompatLevel(0x000A))
		{
			// Compat level for 2.0 staff ghosts
			fixed_t countersteer = FixedDiv(turnfixed, KART_FULLTURN * FRACUNIT);

			if (player->pflags & PF_DRIFTEND)
			{
				countersteer = FRACUNIT;
			}

			return K_GetKartDriftValue(player, countersteer);
		}
		else
		{
			if (player->pflags & PF_DRIFTEND)
			{
				// Sal: This was an unintended control regression from SRB2Kart, but we
				// kind of prefer how it feels. It kind of sucks because the original
				// eats turning entirely for a few tics. Let's do a healthy medium between
				// SRB2Kart and RR: the kick-out value is eased towards normal turning control.

				fixed_t drift_end_term = K_GetKartDriftValue(player, FRACUNIT) * FRACUNIT;
				fixed_t drift_exit_frac = (abs(player->drift) * FRACUNIT) / 5;

				turnfixed = Easing_InSine(
					drift_exit_frac,
					turnfixed,
					drift_end_term
				);

				return (turnfixed / FRACUNIT);
			}
			else
			{
				// If we're drifting we have a completely different turning value
				fixed_t countersteer = FixedDiv(turnfixed, KART_FULLTURN * FRACUNIT);
				return K_GetKartDriftValue(player, countersteer);
			}
		}
	}

	fixed_t finalhandleboost = player->handleboost;

	// If you're sliptiding, don't interact with handling boosts.
	// You need turning power proportional to your speed, no matter what!
	fixed_t topspeed = K_GetKartSpeed(player, false, false);
	if (K_Sliptiding(player))
	{
		fixed_t sliptide_handle;

		if (G_CompatLevel(0x000A))
		{
			// Compat level for 2.0 staff ghosts
			sliptide_handle = 5 * SLIPTIDEHANDLING / 4;
		}
		else
		{
			sliptide_handle = 3 * SLIPTIDEHANDLING / 4;
		}

		finalhandleboost = FixedMul(sliptide_handle, FixedDiv(player->speed, topspeed));
	}

	if (finalhandleboost > 0 && player->respawn.state == RESPAWNST_NONE)
	{
		turnfixed = FixedMul(turnfixed, FRACUNIT + finalhandleboost);
	}

	if (player->curshield == KSHIELD_TOP)
		;
	else if (player->mo->eflags & MFE_UNDERWATER)
	{
		fixed_t div = min(FRACUNIT + K_GetUnderwaterStrafeMul(player), 2*FRACUNIT);
		turnfixed = FixedDiv(turnfixed, div);
	}

	// Weight has a small effect on turning
	turnfixed = FixedMul(turnfixed, weightadjust);

	// Side trick
	if (player->trickpanel == TRICKSTATE_LEFT || player->trickpanel == TRICKSTATE_RIGHT)
	{
		turnfixed /= 2;
	}

	// 2.2 - Presteering allowed in trickpanels
	if (!G_CompatLevel(0x000A) && !K_PlayerUsesBotMovement(player))
	{
		if (player->trickpanel == TRICKSTATE_READY || player->trickpanel == TRICKSTATE_FORWARD)
		{
			// Forward trick or rising from trickpanel
			turnfixed /= 2;
			if (player->tricklock)
				turnfixed /= (player->tricklock/2 + 1);
		}
	}

	return (turnfixed / FRACUNIT);
}

INT32 K_GetUnderwaterTurnAdjust(const player_t *player)
{
	if (player->mo->eflags & MFE_UNDERWATER)
	{
		INT32 steer = (K_GetKartTurnValue(player,
					player->steering) << TICCMD_REDUCE);

		if (!player->drift)
			steer = 9 * steer / 5;

		return FixedMul(steer, 8 * K_GetUnderwaterStrafeMul(player));
	}
	else
		return 0;
}

INT32 K_GetKartDriftSparkValue(const player_t *player)
{
	return (26*4 + player->kartspeed*2 + (9 - player->kartweight))*8;
}

INT32 K_GetKartDriftSparkValueForStage(const player_t *player, UINT8 stage)
{
	fixed_t mul = FRACUNIT;

	// This code is function is pretty much useless now that the timing changes are linear but bleh.
	switch (stage)
	{
		case 2:
			mul = 2*FRACUNIT; // x2
			break;
		case 3:
			mul = 3*FRACUNIT; // x3
			break;
		case 4:
			mul = 4*FRACUNIT; // x4
			break;
	}

	return (FixedMul(K_GetKartDriftSparkValue(player) * FRACUNIT, mul) / FRACUNIT);
}

/*
Stage 1: yellow sparks
Stage 2: red sparks
Stage 3: blue sparks
Stage 4: big large rainbow sparks
Stage 0: air failsafe
*/
void K_SpawnDriftBoostExplosion(player_t *player, int stage)
{
	mobj_t *overlay = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_DRIFTEXPLODE);

	overlay->angle = K_MomentumAngle(player->mo);
	P_SetTarget(&overlay->target, player->mo);
	P_SetScale(overlay, (overlay->destscale = player->mo->scale));
	K_FlipFromObject(overlay, player->mo);

	switch (stage)
	{
		case 1:
			overlay->fuse = 16;
			break;

		case 2:

			overlay->fuse = 32;
			S_StartSound(player->mo, sfx_kc5b);
			break;

		case 3:
			overlay->fuse = 48;

			S_StartSound(player->mo, sfx_kc5b);
			break;

		case 4:
			overlay->fuse = 120;

			S_StartSound(player->mo, sfx_kc5b);
			S_StartSound(player->mo, sfx_s3kc4l);
			break;

		case 0:
			overlay->fuse = 16;
			break;
	}

	overlay->extravalue1 = stage;
}

static void K_KartDrift(player_t *player, boolean onground)
{
	const fixed_t minspeed = (10 * player->mo->scale);

	const INT32 dsone = K_GetKartDriftSparkValueForStage(player, 1);
	const INT32 dstwo = K_GetKartDriftSparkValueForStage(player, 2);
	const INT32 dsthree = K_GetKartDriftSparkValueForStage(player, 3);
	const INT32 dsfour = K_GetKartDriftSparkValueForStage(player, 4);

	const UINT16 buttons = K_GetKartButtons(player);

	boolean dokicker = false;

	// Drifting is actually straffing + automatic turning.
	// Holding the Jump button will enable drifting.
	// (This comment is extremely funny)

	// Drift Release (Moved here so you can't "chain" drifts)
	if (player->drift != -5 && player->drift != 5)
	{
		if (player->driftcharge < 0 || player->driftcharge >= dsone)
		{
			angle_t pushdir = K_MomentumAngle(player->mo);

			S_StartSound(player->mo, sfx_s23c);
			//K_SpawnDashDustRelease(player);

			// Used to detect useful driftboosts.
			UINT8 oldDriftBoost = player->driftboost;

			// Airtime means we're not gaining speed. Get grounded!
			if (!onground)
				player->mo->momz -= player->speed/2;

			if (player->driftcharge < 0)
			{
				// Stage 0: Grey sparks
				if (!onground)
					P_Thrust(player->mo, pushdir, player->speed / 8);

				if (player->driftboost < 15)
					player->driftboost = 15;
			}
			else if (player->driftcharge >= dsone && player->driftcharge < dstwo)
			{
				// Stage 1: Yellow sparks
				if (!onground)
					P_Thrust(player->mo, pushdir, player->speed / 3);

				if (player->driftboost < 20)
					player->driftboost = 20;

				K_SpawnDriftBoostExplosion(player, 1);
			}
			else if (player->driftcharge < dsthree)
			{
				// Stage 2: Red sparks
				if (!onground)
					P_Thrust(player->mo, pushdir, player->speed / 2);

				if (player->driftboost < 50)
					player->driftboost = 50;

				K_SpawnDriftBoostExplosion(player, 2);
			}
			else if (player->driftcharge < dsfour)
			{
				// Stage 3: Blue sparks
				if (!onground)
					P_Thrust(player->mo, pushdir, player->speed);

				if (player->driftboost < 85)
					player->driftboost = 85;
				if (player->strongdriftboost < 85)
					player->strongdriftboost = 85;

				K_SpawnDriftBoostExplosion(player, 3);
				K_SpawnDriftElectricSparks(player, K_DriftSparkColor(player, player->driftcharge), false);
				dokicker = true;
			}
			else if (player->driftcharge >= dsfour)
			{
				// Stage 4: Rainbow sparks
				if (!onground)
					P_Thrust(player->mo, pushdir, (5 * player->speed / 4));

				if (player->driftboost < 125)
					player->driftboost = 125;
				if (player->strongdriftboost < 125)
					player->strongdriftboost = 125;

				K_SpawnDriftBoostExplosion(player, 4);
				K_SpawnDriftElectricSparks(player, K_DriftSparkColor(player, player->driftcharge), false);
				dokicker = true;
			}

			if (player->trickcharge && dokicker)
			{
				// 2.2 - Egg-friendly trick stuff
				if (G_CompatLevel(0x000B))
				{
					player->driftboost += 20;
					player->wavedashboost += 10;
					player->wavedashpower = FRACUNIT;
					P_Thrust(player->mo, pushdir, player->speed / 2);
				}
				else
				{
					player->driftboost += TICRATE;
					player->counterdash += TICRATE/2;
					P_Thrust(player->mo, pushdir, player->speed / 6);
				}

				S_StartSound(player->mo, sfx_gshba);
				player->trickcharge = 0;
				player->infinitether = TICRATE*2;
			}

			// If you actually used a useful driftboost, adjust the added boost, biasing towards bottom-right.
			// Everyone else has speed-retention mechanics they can chain into themselves: Metal needs help!
			if (player->driftboost > oldDriftBoost)
			{
				player->driftboost = (38 + player->kartweight + player->kartspeed) * player->driftboost / 40;
			}
		}

		// Remove charge
		player->driftcharge = 0;
	}

	// Drifting: left or right?
	if (!(player->pflags & PF_DRIFTINPUT))
	{
		// drift is not being performed so if we're just finishing set driftend and decrement counters
		if (player->drift > 0)
		{
			player->drift--;
			player->pflags |= PF_DRIFTEND;
		}
		else if (player->drift < 0)
		{
			player->drift++;
			player->pflags |= PF_DRIFTEND;
		}
		else
			player->pflags &= ~PF_DRIFTEND;
	}
	else if (player->speed > minspeed
		&& (player->drift == 0 || (player->pflags & PF_DRIFTEND)))
	{
		// Uses turning over steering, since this is very binary.
		// Using steering would cause a lot more "wrong drifts".
		if (player->cmd.turning > 0)
		{
			// Starting left drift
			player->drift = 1;
			player->driftcharge = 0;
			player->pflags &= ~PF_DRIFTEND;
		}
		else if (player->cmd.turning < 0)
		{
			// Starting right drift
			player->drift = -1;
			player->driftcharge = 0;
			player->pflags &= ~PF_DRIFTEND;
		}
	}

	if (P_PlayerInPain(player) || player->speed <= 0)
	{
		// Stop drifting
		player->drift = player->driftcharge = player->aizdriftstrat = 0;
		player->pflags &= ~(PF_BRAKEDRIFT|PF_GETSPARKS);
		// And take away wavedash properties: advanced cornering demands advanced finesse
		player->wavedash = 0;
		player->wavedashboost = 0;
		player->trickcharge = 0;
	}
	else if ((player->pflags & PF_DRIFTINPUT) && player->drift != 0)
	{
		// Incease/decrease the drift value to continue drifting in that direction
		fixed_t driftadditive = 24;
		boolean playsound = false;

		if (onground)
		{
			if (player->drift >= 1) // Drifting to the left
			{
				player->drift++;
				if (player->drift > 5)
					player->drift = 5;

				if (player->steering > 0) // Inward
					driftadditive += abs(player->steering)/100;
				if (player->steering < 0) // Outward
					driftadditive -= abs(player->steering)/75;
			}
			else if (player->drift <= -1) // Drifting to the right
			{
				player->drift--;
				if (player->drift < -5)
					player->drift = -5;

				if (player->steering < 0) // Inward
					driftadditive += abs(player->steering)/100;
				if (player->steering > 0) // Outward
					driftadditive -= abs(player->steering)/75;
			}

			// Disable drift-sparks until you're going fast enough
			if (!(player->pflags & PF_GETSPARKS)
				|| (player->offroad && K_ApplyOffroad(player)))
				driftadditive = 0;

			// Inbetween minspeed and minspeed*2, it'll keep your previous drift-spark state.
			if (player->speed > minspeed*2)
			{
				player->pflags |= PF_GETSPARKS;

				if (player->driftcharge <= -1)
				{
					player->driftcharge = dsone; // Back to red
					playsound = true;
				}
			}
			else if (player->speed <= minspeed)
			{
				player->pflags &= ~PF_GETSPARKS;
				driftadditive = 0;

				if (player->driftcharge >= dsone)
				{
					player->driftcharge = -1; // Set yellow sparks
					playsound = true;
				}
			}
		}
		else
		{
			driftadditive = 0;
		}

		if (player->trickcharge && driftadditive)
			driftadditive += 16;

		// This spawns the drift sparks
		if ((player->driftcharge + driftadditive >= dsone)
			|| (player->driftcharge < 0))
		{
			K_SpawnDriftSparks(player);
		}

		if ((player->driftcharge < dsone && player->driftcharge+driftadditive >= dsone)
			|| (player->driftcharge < dstwo && player->driftcharge+driftadditive >= dstwo)
			|| (player->driftcharge < dsthree && player->driftcharge+driftadditive >= dsthree))
		{
			playsound = true;
		}

		// Sound whenever you get a different tier of sparks
		if (playsound && P_IsDisplayPlayer(player))
		{
			if (player->driftcharge == -1)
				S_StartSoundAtVolume(player->mo, sfx_sploss, 192); // Yellow spark sound
			else
				S_StartSoundAtVolume(player->mo, sfx_s3ka2, 192);
		}

		player->driftcharge += driftadditive;
		player->pflags &= ~PF_DRIFTEND;
	}

	// No longer meet the conditions to sliptide?
	// We'll spot you the sliptide as long as you keep turning, but no charging wavedashes.
	boolean extendedSliptide = false;

	// We don't meet sliptide conditions!
	if ((player->handleboost < (SLIPTIDEHANDLING/2))
	|| (!player->steering)
	|| (!player->aizdriftstrat)
	|| (player->steering > 0) != (player->aizdriftstrat > 0))
	{
		if (!player->drift && player->steering && player->aizdriftextend // If we were sliptiding last tic,
			&& (player->steering > 0) == (player->aizdriftextend > 0) // we're steering in the right direction,
			&& player->speed >= K_GetKartSpeed(player, false, true)) // and we're above the threshold to spawn dust...
		{
			extendedSliptide = true; // Then keep your current sliptide, but note the behavior change for wavedash handling.
		}
		else // Otherwise, update sliptide status as usual.
		{
			if (!player->drift)
				player->aizdriftstrat = 0;
			else
				player->aizdriftstrat = ((player->drift > 0) ? 1 : -1);
		}
	}

	if (player->airtime > 2) // Arbitrary number. Small discontinuities due to Super Jank shouldn't thrash your handling properties.
	{
		player->aizdriftstrat = 0;
		extendedSliptide = false;
	}

	// If we're sliptiding, whether through an extension or otherwise, allow sliptide extensions next tic.
	if (K_Sliptiding(player))
		player->aizdriftextend = player->aizdriftstrat;
	else
		player->aizdriftextend = 0;


	if ((player->aizdriftstrat && !player->drift)
		|| (extendedSliptide))
	{
		K_SpawnAIZDust(player);

		if (!extendedSliptide)
		{
			// Give charge proportional to your angle. Sharp turns are rewarding, slow analog slides are not—remember, this is giving back the speed you gave up.
			UINT16 addCharge = FixedInt(
				FixedMul(10*FRACUNIT,
					FixedDiv(abs(player->steering)*FRACUNIT, (9*KART_FULLTURN/10)*FRACUNIT)
				));
			addCharge = min(10, max(addCharge, 1));
			// "Why 9*KART_FULLTURN/10?" For bullshit turn solver reasons, it's extremely common to steer at like 99% of FULLTURN even when at the edge of your analog range.
			// This makes wavedash charge noticeably slower on even modest delay, despite the magnitude of the turn seeming the same.
			// So we only require 90% of a turn to get full charge strength.

			player->wavedash += addCharge;

			if (player->wavedash >= MIN_WAVEDASH_CHARGE && (player->wavedash - addCharge) < MIN_WAVEDASH_CHARGE)
				S_StartSound(player->mo, sfx_waved5);
		}

		if (abs(player->aizdrifttilt) < ANGLE_22h)
		{
			player->aizdrifttilt =
				(abs(player->aizdrifttilt) + ANGLE_11hh / 4) *
				player->aizdriftstrat;
		}

		if (abs(player->aizdriftturn) < ANGLE_112h)
		{
			player->aizdriftturn =
				(abs(player->aizdriftturn) + ANGLE_11hh) *
				player->aizdriftstrat;
		}
	}

	/*
	if (player->mo->eflags & MFE_UNDERWATER)
		player->aizdriftstrat = 0;
	*/

	if (!K_Sliptiding(player) || extendedSliptide)
	{
		if (!extendedSliptide && K_IsLosingWavedash(player) && player->wavedash > 0)
		{
			if (player->wavedash > HIDEWAVEDASHCHARGE && !S_SoundPlaying(player->mo, sfx_waved2))
				S_StartSoundAtVolume(player->mo, sfx_waved2,
					Easing_InSine(
						min(FRACUNIT, FRACUNIT * player->wavedash / MIN_WAVEDASH_CHARGE),
						120,
						255
					)
				); // Losing combo time, going to boost
			S_StopSoundByID(player->mo, sfx_waved1);
			S_StopSoundByID(player->mo, sfx_waved4);
			player->wavedashdelay++;
			if (player->wavedashdelay > TICRATE/2)
			{
				if (player->wavedash > HIDEWAVEDASHCHARGE)
				{
					fixed_t maxZipPower = 2*FRACUNIT;
					fixed_t minZipPower = 1*FRACUNIT;
					fixed_t powerSpread = maxZipPower - minZipPower;

					int minPenalty = 2*1 + (9-9); // Kinda doing a similar thing to driftspark stage timers here.
					int maxPenalty = 2*9 + (9-1); // 1/9 gets max, 9/1 gets min, everyone else gets something in between.
					int penaltySpread = maxPenalty - minPenalty;
					int yourPenalty = 2*player->kartspeed + (9 - player->kartweight); // And like driftsparks, speed hurts double.

					yourPenalty -= minPenalty; // Normalize; minimum penalty should take away 0 power.

					fixed_t yourPowerReduction = FixedDiv(yourPenalty * FRACUNIT, penaltySpread * FRACUNIT);
					fixed_t yourPower = maxZipPower - FixedMul(yourPowerReduction, powerSpread);
					int yourBoost = FixedInt(FixedMul(yourPower, player->wavedash/10 * FRACUNIT));

					// Award boost.
					player->wavedashboost += yourBoost;

					// Set power of the resulting boost.
					player->wavedashpower = min(FRACUNIT, FRACUNIT * player->wavedash / MIN_WAVEDASH_CHARGE);

					// Scale boost sound to power.
					S_StartSoundAtVolume(player->mo, sfx_waved3,
						Easing_InSine(
							player->wavedashpower,
							120,
							255
						)
					);

					K_SpawnDriftBoostExplosion(player, 0);
				}
				S_StopSoundByID(player->mo, sfx_waved1);
				S_StopSoundByID(player->mo, sfx_waved2);
				S_StopSoundByID(player->mo, sfx_waved4);
				player->wavedash = 0;
				player->wavedashdelay = 0;
			}
		}
		else
		{
			S_StopSoundByID(player->mo, sfx_waved1);
			S_StopSoundByID(player->mo, sfx_waved2);
			if (player->wavedash > 0 && !S_SoundPlaying(player->mo, sfx_waved4))
				S_StartSoundAtVolume(player->mo, sfx_waved4, 255); // Passive woosh
		}

		player->aizdrifttilt -= player->aizdrifttilt / 4;
		player->aizdriftturn -= player->aizdriftturn / 4;

		if (abs(player->aizdrifttilt) < ANGLE_11hh / 4)
			player->aizdrifttilt = 0;
		if (abs(player->aizdriftturn) < ANGLE_11hh)
			player->aizdriftturn = 0;
	}
	else
	{
		player->wavedashdelay = 0;
		S_StopSoundByID(player->mo, sfx_waved2);
		S_StopSoundByID(player->mo, sfx_waved4);
		if (player->wavedash > HIDEWAVEDASHCHARGE && !S_SoundPlaying(player->mo, sfx_waved1))
			S_StartSoundAtVolume(player->mo, sfx_waved1, 255); // Charging
	}

	if (player->drift
		&& ((buttons & BT_BRAKE)
		|| !(buttons & BT_ACCELERATE))
		&& P_IsObjectOnGround(player->mo))
	{
		if (!(player->pflags & PF_BRAKEDRIFT))
			K_SpawnBrakeDriftSparks(player);
		player->pflags |= PF_BRAKEDRIFT;
	}
	else
		player->pflags &= ~PF_BRAKEDRIFT;
}
//
// K_KartUpdatePosition
//
void K_KartUpdatePosition(player_t *player)
{
	fixed_t position = 1;
	fixed_t oldposition = player->position;
	fixed_t i;
	INT32 realplayers = 0;

	if (player->spectator || !player->mo)
	{
		// Ensure these are reset for spectators
		player->position = 0;
		player->positiondelay = 0;
		return;
	}

	if (K_PodiumSequence() == true)
	{
		position = K_GetPodiumPosition(player);

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator || !players[i].mo)
				continue;

			realplayers++;
		}
	}
	else
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator || !players[i].mo)
				continue;

			realplayers++;

			if (gametyperules & GTR_CIRCUIT)
			{
				if (player->exiting) // End of match standings
				{
					// Only time matters
					if (players[i].realtime < player->realtime)
						position++;
				}
				else
				{
					// I'm a lap behind this player OR
					// My distance to the finish line is higher, so I'm behind
					if ((players[i].laps > player->laps)
						|| (players[i].distancetofinish < player->distancetofinish))
					{
						position++;
					}
				}
			}
			else
			{
				if (player->exiting) // End of match standings
				{
					// Only score matters
					if (players[i].roundscore > player->roundscore)
						position++;
				}
				else
				{
					UINT8 myEmeralds = K_NumEmeralds(player);
					UINT8 yourEmeralds = K_NumEmeralds(&players[i]);

					// First compare all points
					if (players[i].roundscore > player->roundscore)
					{
						position++;
					}
					else if (players[i].roundscore == player->roundscore)
					{
						// Emeralds are a tie breaker
						if (yourEmeralds > myEmeralds)
						{
							position++;
						}
						else if (yourEmeralds == myEmeralds)
						{
							// Bumpers are the second tier tie breaker
							if (K_Bumpers(&players[i]) > K_Bumpers(player))
							{
								position++;
							}
						}
					}
				}
			}
		}
	}

	if (leveltime < starttime || oldposition == 0)
		oldposition = position;

	if (position != oldposition) // Changed places?
	{
		if (!K_Cooperative() && player->positiondelay <= 0 && position < oldposition && P_IsDisplayPlayer(player) == true)
		{
			// Play sound when getting closer to 1st.
			UINT32 soundpos = (max(0, position - 1) * MAXPLAYERS)/realplayers; // always 1-15 despite there being 16 players at max...
#if MAXPLAYERS > 16
			if (soundpos < 15)
			{
				soundpos = 15;
			}
#endif
			S_ReducedVFXSound(player->mo, sfx_pass02 + soundpos, NULL); // ...which is why we can start at index 2 for a lower general pitch
		}

		player->positiondelay = POS_DELAY_TIME + 4; // Position number growth
	}

	/* except in FREE PLAY */
	if (player->curshield == KSHIELD_TOP &&
			(gametyperules & GTR_CIRCUIT) &&
			realplayers > 1)
	{
		/* grace period so you don't fall off INSTANTLY */
		if (K_GetItemRouletteDistance(player, 8) < 2000 && player->topinfirst < 2*TICRATE) // "Why 8?" Literally no reason, but since we intend for constant-ish distance we choose a fake fixed playercount.
		{
			player->topinfirst++;
		}
		else
		{
			if (position == 1)
			{
				Obj_GardenTopThrow(player);
			}
			else
			{
				player->topinfirst = 0;
			}
		}
	}
	else
	{
		player->topinfirst = 0;
	}

	player->position = position;
}

void K_UpdateAllPlayerPositions(void)
{
	INT32 i;

	// First loop: Ensure all players' distance to the finish line are all accurate
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = &players[i];
		if (!playeringame[i] || player->spectator || !player->mo || P_MobjWasRemoved(player->mo))
		{
			continue;
		}

		if (K_PodiumSequence() == true)
		{
			K_UpdatePodiumWaypoints(player);
			continue;
		}

		if (player->respawn.state == RESPAWNST_MOVE &&
			player->respawn.init == true &&
			player->lastsafelap != player->laps)
		{
			player->laps = player->lastsafelap;
			player->cheatchecknum = player->lastsafecheatcheck;
		}

		K_UpdatePlayerWaypoints(player);
	}

	// Second loop: Ensure all player positions reflect everyone's distances
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo))
		{
			K_KartUpdatePosition(&players[i]);
		}
	}
}

//
// K_StripItems
//
void K_StripItems(player_t *player)
{
	K_DropRocketSneaker(player);
	K_DropKitchenSink(player);
	player->itemtype = KITEM_NONE;
	player->itemamount = 0;
	player->itemflags &= ~(IF_ITEMOUT|IF_EGGMANOUT);

	if (player->itemRoulette.eggman == false)
	{
		K_StopRoulette(&player->itemRoulette);
	}

	player->hyudorotimer = 0;
	player->stealingtimer = 0;

	player->curshield = KSHIELD_NONE;
	player->bananadrag = 0;
	player->ballhogcharge = 0;
	player->sadtimer = 0;

	K_UpdateHnextList(player, true);
}

void K_StripOther(player_t *player)
{
	K_StopRoulette(&player->itemRoulette);

	player->invincibilitytimer = 0;
	if (player->growshrinktimer)
	{
		K_RemoveGrowShrink(player);
	}

	if (player->eggmanexplode)
	{
		player->eggmanexplode = 0;
		player->eggmanblame = -1;

		K_KartResetPlayerColor(player);
	}
}

static INT32 K_FlameShieldMax(player_t *player)
{
	UINT32 disttofinish = 0;
	UINT32 distv = 1024; // Pre no-scams: 2048
	distv = distv * 16 / FLAMESHIELD_MAX; // Old distv was based on a 16-segment bar
	UINT8 numplayers = 0;
	UINT32 scamradius = 1500; // How close is close enough that we shouldn't be allowed to scam 1st?
	UINT8 i;

	if (gametyperules & GTR_CIRCUIT)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].spectator)
				numplayers++;
			if (players[i].position == 1)
				disttofinish = players[i].distancetofinish;
		}
	}

	disttofinish = player->distancetofinish - disttofinish;
	distv = FixedMul(distv, mapobjectscale);

	if (numplayers <= 1)
	{
		return FLAMESHIELD_MAX; // max when alone, for testing
		// and when in battle, for chaos
	}
	else if (player->position == 1 || disttofinish < scamradius)
	{
		return 0; // minimum for first
	}

	disttofinish = disttofinish - scamradius;

	return min(FLAMESHIELD_MAX, (FLAMESHIELD_MAX / 16) + (disttofinish / distv)); // Ditto for this minimum, old value was 1/16
}

boolean K_PlayerEBrake(const player_t *player)
{
	if (player->respawn.state != RESPAWNST_NONE
		&& (player->respawn.init == true || player->respawn.fromRingShooter == true))
	{
		return false;
	}

	if (Obj_PlayerRingShooterFreeze(player) == true)
	{
		return false;
	}

	if (player->fastfall != 0)
	{
		return true;
	}

	if (K_PressingEBrake(player) == true
		&& (player->drift == 0 || P_IsObjectOnGround(player->mo) == false)
		&& P_PlayerInPain(player) == false
		&& player->justbumped == 0
		&& player->spindashboost == 0
		&& player->nocontrol == 0)
	{
		return true;
	}

	return false;
}

boolean K_PlayerGuard(const player_t *player)
{
	if (player->defenseLockout != 0)
	{
		return false;
	}

	if (P_PlayerInPain(player) == true)
	{
		return false;
	}

	if (K_PowerUpRemaining(player, POWERUP_BARRIER))
	{
		return true;
	}

	if (player->instaWhipCharge != 0)
	{
		return false;
	}

	if (player->spheres == 0)
		return false;

	// Ugh. Duplicating a lot of this because while Guard _superficially_ looks like it's
	// restricted similarly to ebrake, it's actually _really_ bad if we can't guard after item bumps.

	if (player->respawn.state != RESPAWNST_NONE
		&& (player->respawn.init == true || player->respawn.fromRingShooter == true))
	{
		return false;
	}

	if (Obj_PlayerRingShooterFreeze(player) == true)
	{
		return false;
	}

	if (K_PressingEBrake(player) == true
		&& (player->drift == 0 || P_IsObjectOnGround(player->mo) == false)
		&& player->spindashboost == 0
		&& player->nocontrol == 0)
	{
		return true;
	}

	return false;
}

SINT8 K_Sliptiding(const player_t *player)
{
	/*
	if (player->mo->eflags & MFE_UNDERWATER)
		return 0;
	*/
	return player->drift ? 0 : player->aizdriftstrat;
}

INT32 K_StairJankFlip(INT32 value)
{
	return P_AltFlip(value, 2);
}

// Ebraking visuals for mo
// we use mo->hprev for the hold bubble. If another hprev exists for some reason, remove it.

void K_KartEbrakeVisuals(player_t *p)
{
	mobj_t *wave;
	mobj_t *spdl;
	fixed_t sx, sy;

	if (K_PlayerEBrake(p) == true)
	{
		if (p->ebrakefor % 20 == 0)
		{
			wave = P_SpawnMobj(p->mo->x, p->mo->y, P_GetMobjGround(p->mo), MT_SOFTLANDING);
			P_InstaScale(wave, p->mo->scale);
			P_SetTarget(&wave->target, p->mo);
			P_SetTarget(&wave->owner, p->mo);
			wave->renderflags |= RF_REDUCEVFX;
		}

		// sound
		if (!S_SoundPlaying(p->mo, sfx_s3kd9s) && (leveltime > starttime || P_IsDisplayPlayer(p)))
			S_ReducedVFXSound(p->mo, sfx_s3kd9s, p);

		// HOLD! bubble.
		if (!p->ebrakefor)
		{
			if (p->mo->hprev && !P_MobjWasRemoved(p->mo->hprev))
			{
				// for some reason, there's already an hprev. Remove it.
				P_RemoveMobj(p->mo->hprev);
			}

			P_SetTarget(&p->mo->hprev, P_SpawnMobj(p->mo->x, p->mo->y, p->mo->z, MT_HOLDBUBBLE));
			p->mo->hprev->renderflags |= (RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(p));
			if (encoremode)
			{
				// Don't render this text/digit mirrored.
				p->mo->hprev->renderflags ^= RF_HORIZONTALFLIP;
			}

		}

		// Update HOLD bubble.
		if (p->mo->hprev && !P_MobjWasRemoved(p->mo->hprev))
		{
			P_MoveOrigin(p->mo->hprev, p->mo->x, p->mo->y, p->mo->z);
			p->mo->hprev->angle = p->mo->angle;
			p->mo->hprev->fuse = TICRATE/2;		// When we leave spindash for any reason, make sure this bubble goes away soon after.
			K_FlipFromObject(p->mo->hprev, p->mo);
			P_SetTarget(&p->mo->hprev->owner, p->mo);
			p->mo->hprev->renderflags |= RF_REDUCEVFX;
			p->mo->hprev->sprzoff = p->mo->sprzoff;

			p->mo->hprev->colorized = false;
			p->mo->hprev->spritexoffset = 0;
			p->mo->hprev->spriteyoffset = 0;
		}

		if (!p->spindash)
		{
			// Spawn downwards fastline
			sx = p->mo->x + P_RandomRange(PR_DECORATION, -48, 48)*p->mo->scale;
			sy = p->mo->y + P_RandomRange(PR_DECORATION, -48, 48)*p->mo->scale;

			spdl = P_SpawnMobj(sx, sy, p->mo->z, MT_DOWNLINE);
			spdl->colorized = true;
			spdl->color = SKINCOLOR_WHITE;
			K_MatchGenericExtraFlags(spdl, p->mo);
			P_SetTarget(&spdl->owner, p->mo);
			spdl->renderflags |= RF_REDUCEVFX;
			P_SetScale(spdl, p->mo->scale);

			// squish the player a little bit.
			p->mo->spritexscale = FRACUNIT*115/100;
			p->mo->spriteyscale = FRACUNIT*85/100;
		}
		else
		{
			const UINT16 MAXCHARGETIME = K_GetSpindashChargeTime(p);
			const fixed_t MAXSHAKE = FRACUNIT;

			// update HOLD bubble with numbers based on charge.
			if (p->mo->hprev && !P_MobjWasRemoved(p->mo->hprev))
			{
				const INT16 overcharge = (p->spindash - MAXCHARGETIME);
				const boolean desperation = (p->rings <= 0); // desperation spindash

				UINT8 frame = min(1 + ((p->spindash*3) / MAXCHARGETIME), 4);

				// ?! limit.
				if (overcharge >= TICRATE)
					frame = 5;

				p->mo->hprev->frame = frame|FF_FULLBRIGHT;

				if (overcharge >= 0)
				{
					// overcharged spindash flashes red.
					p->mo->hprev->color = SKINCOLOR_MAROON;
					p->mo->hprev->colorized = (overcharge % 12) >= 6;

					p->mo->hprev->spritexoffset = P_AltFlip(2*FRACUNIT, 2);
				}

				if (desperation)
				{
					// desperation spindash shakes VIOLENTLY.
					p->mo->hprev->spritexoffset = P_AltFlip(4*FRACUNIT, 2);
					p->mo->hprev->spriteyoffset = P_AltFlip(2*FRACUNIT, 1);
				}
			}

			// shake the player as they charge their spindash!

			// "gentle" shaking as we start...
			if (p->spindash < MAXCHARGETIME)
			{
				fixed_t shake = FixedMul(((p->spindash)*FRACUNIT/MAXCHARGETIME), MAXSHAKE);
				SINT8 mult = leveltime & 1 ? 1 : -1;

				p->mo->spritexoffset = shake*mult;
			}
			else	// get VIOLENT on overcharge :)
			{
				fixed_t shake = MAXSHAKE + FixedMul(((p->spindash-MAXCHARGETIME)*FRACUNIT/TICRATE), MAXSHAKE)*3;
				SINT8 mult = leveltime & 1 ? 1 : -1;

				p->mo->spritexoffset = shake*mult;
			}

			// sqish them a little MORE....
			p->mo->spritexscale = FRACUNIT*12/10;
			p->mo->spriteyscale = FRACUNIT*8/10;
		}


		p->ebrakefor++;
	}
	else if (p->ebrakefor)	// cancel effects
	{
		// reset scale
		p->mo->spritexscale = FRACUNIT;
		p->mo->spriteyscale = FRACUNIT;

		// reset shake
		p->mo->spritexoffset = 0;

		// remove the bubble instantly unless it's in the !? state
		if (p->mo->hprev && !P_MobjWasRemoved(p->mo->hprev) && (p->mo->hprev->frame & FF_FRAMEMASK) != 5)
		{
			P_RemoveMobj(p->mo->hprev);
			P_SetTarget(&p->mo->hprev, NULL);
		}

		p->ebrakefor = 0;
	}
}

static void K_KartSpindashDust(mobj_t *parent)
{
	fixed_t rad = FixedDiv(FixedHypot(parent->radius, parent->radius), parent->scale);
	INT32 i;

	for (i = 0; i < 2; i++)
	{
		fixed_t hmomentum = P_RandomRange(PR_DECORATION, 6, 12) * parent->scale;
		fixed_t vmomentum = P_RandomRange(PR_DECORATION, 2, 6) * parent->scale;

		angle_t ang = parent->player->drawangle + ANGLE_180;
		SINT8 flip = 1;

		mobj_t *dust;

		if (i & 1)
			ang -= ANGLE_45;
		else
			ang += ANGLE_45;

		dust = P_SpawnMobjFromMobj(parent,
			FixedMul(rad, FINECOSINE(ang >> ANGLETOFINESHIFT)),
			FixedMul(rad, FINESINE(ang >> ANGLETOFINESHIFT)),
			0, MT_SPINDASHDUST
		);
		flip = P_MobjFlip(dust);

		dust->momx = FixedMul(hmomentum, FINECOSINE(ang >> ANGLETOFINESHIFT));
		dust->momy = FixedMul(hmomentum, FINESINE(ang >> ANGLETOFINESHIFT));
		dust->momz = vmomentum * flip;
	}
}

static void K_KartSpindashWind(mobj_t *parent)
{
	mobj_t *wind = P_SpawnMobjFromMobj(parent,
		P_RandomRange(PR_DECORATION,-36,36) * FRACUNIT,
		P_RandomRange(PR_DECORATION,-36,36) * FRACUNIT,
		FixedDiv(parent->height / 2, parent->scale) + (P_RandomRange(PR_DECORATION,-20,20) * FRACUNIT),
		MT_SPINDASHWIND
	);

	P_SetTarget(&wind->target, parent);

	if (parent->player && parent->player->wavedashboost)
		P_SetScale(wind, wind->scale * 2);

	if (parent->momx || parent->momy)
		wind->angle = R_PointToAngle2(0, 0, parent->momx, parent->momy);
	else
		wind->angle = parent->player->drawangle;

	wind->momx = 3 * parent->momx / 4;
	wind->momy = 3 * parent->momy / 4;
	wind->momz = 3 * P_GetMobjZMovement(parent) / 4;

	K_MatchGenericExtraFlags(wind, parent);
	P_SetTarget(&wind->owner, parent);
	wind->renderflags |= RF_REDUCEVFX;
}

// Time after which you get a thrust for releasing spindash
#define SPINDASHTHRUSTTIME 20

static void K_KartSpindash(player_t *player)
{
	const boolean onGround = P_IsObjectOnGround(player->mo);
	const INT16 MAXCHARGETIME = K_GetSpindashChargeTime(player);
	UINT16 buttons = K_GetKartButtons(player);
	boolean spawnWind = (leveltime % 2 == 0);

	if (player->mo->hitlag > 0 || P_PlayerInPain(player))
	{
		player->spindash = 0;
	}

	if (player->spindash > 0 && (buttons & (BT_DRIFT|BT_BRAKE|BT_ACCELERATE)) != (BT_DRIFT|BT_BRAKE|BT_ACCELERATE))
	{
		player->spindashspeed = (min(player->spindash, MAXCHARGETIME) * FRACUNIT) / MAXCHARGETIME;
		player->spindashboost = TICRATE;

		// if spindash was charged enough, give a small thrust.
		if (player->spindash >= SPINDASHTHRUSTTIME)
		{
			fixed_t thrust = FixedMul(player->mo->scale, player->spindash*FRACUNIT/5);

			// Old behavior, before emergency zero-ring spindash
			/*
			if (gametyperules & GTR_CLOSERPLAYERS)
				thrust *= 2;
			*/

			// Give a bit of a boost depending on charge.
			P_InstaThrust(player->mo, player->mo->angle, thrust);
		}

		K_SetTireGrease(player, 2*TICRATE);

		player->spindash = 0;
		S_ReducedVFXSound(player->mo, sfx_s23c, player);
		S_StopSoundByID(player->mo, sfx_kc38);
	}


	if ((player->spindashboost > 0) && (spawnWind == true))
	{
		K_KartSpindashWind(player->mo);
	}

	if ((player->wavedashboost > 0) && (spawnWind == true))
	{
		K_KartSpindashWind(player->mo);
	}

	if (player->spindashboost > (TICRATE/2))
	{
		K_KartSpindashDust(player->mo);
	}

	// 2.2 - Driftbrake slideoff fastfall prevention
	if (!G_CompatLevel(0x000A))
	{
		if (player->drift && onGround && player->cmd.buttons & BT_BRAKE)
		{
			player->pflags |= PF_NOFASTFALL;
			return;
		}
	}

	if (K_PlayerEBrake(player) == false)
	{
		player->spindash = 0;
		player->pflags &= ~PF_NOFASTFALL;
		return;
	}
	else
	{
		// 2.2 - More responsive ebrake
		if (!G_CompatLevel(0x000A))
		{
			if (onGround && player->noEbrakeMagnet == 0 && (FixedHypot(player->mo->momx, player->mo->momy) < 20*player->mo->scale))
			{
				P_Thrust(player->mo, K_MomentumAngleReal(player->mo) + ANGLE_180, FixedHypot(player->mo->momx, player->mo->momy)/8);
			}
		}

	}

	// Handle fast falling behaviors first.
	if (player->respawn.state != RESPAWNST_NONE)
	{
		// This is handled in K_MovePlayerToRespawnPoint.
		return;
	}
	else if (onGround == false)
	{
		if (player->pflags & PF_NOFASTFALL)
			return;

		if (player->fastfall == 0)
		{
			// Factors 3D momentum.
			player->fastfallBase = FixedHypot(player->speed, player->mo->momz);
		}

		// Update fastfall.
		player->fastfall = player->mo->momz;
		player->spindash = 0;
		P_ResetPitchRoll(player->mo);

		return;
	}
	else if (player->fastfall != 0)
	{
		// Still handling fast-fall bounce.
		player->pflags |= PF_NOFASTFALL;
		return;
	}

	player->pflags |= PF_NOFASTFALL;

	if (player->speed == 0 && player->steering != 0 && leveltime % 8 == 0)
	{
		// Rubber burn turn sfx
		S_ReducedVFXSound(player->mo, sfx_ruburn, player);
	}

	if (player->speed < 6*player->mo->scale)
	{
		if ((buttons & (BT_DRIFT|BT_BRAKE)) == (BT_DRIFT|BT_BRAKE))
		{
			UINT8 ringdropframes = 2 + (player->kartspeed + player->kartweight);
			boolean spawnOldEffect = true;

			INT16 chargetime = MAXCHARGETIME - ++player->spindash;

			if (player->rings <= 0 && chargetime >= 0) // Desperation spindash
			{
				player->spindash++;
				if (!S_SoundPlaying(player->mo, sfx_kc38))
					S_StartSound(player->mo, sfx_kc38);
			}

			if (player->spindash >= SPINDASHTHRUSTTIME)
			{
				K_KartSpindashDust(player->mo);
				spawnOldEffect = false;
			}

			if (chargetime <= (MAXCHARGETIME / 4) && spawnWind == true)
			{
				K_KartSpindashWind(player->mo);
			}

			if (player->flashing > 0 && (player->spindash % ringdropframes == 0) && player->hyudorotimer == 0)
			{
				// Every frame that you're invisible from flashing, spill a ring.
				// Intentionally a lop-sided trade-off, so the game doesn't become
				// Funky Kong's Ring Racers.

				// 2.2 - No extended ring debt for recovery spindash
				if (G_CompatLevel(0x000A))
				{
					P_PlayerRingBurst(player, 1);
				}
				else
				{
					if (player->rings > 0)
						P_PlayerRingBurst(player, 1);
				}

			}

			if (chargetime > 0)
			{
				UINT16 soundcharge = 0;
				UINT8 add = 0;

				while ((soundcharge += ++add) < chargetime);

				if (soundcharge == chargetime)
				{
					if (spawnOldEffect == true)
						K_SpawnDashDustRelease(player);
					S_ReducedVFXSound(player->mo, sfx_s3kab, player);
				}
			}
			else if (chargetime < -TICRATE)
			{
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_NORMAL);
			}
		}
	}
	else
	{
		if (leveltime % 4 == 0)
			S_ReducedVFXSound(player->mo, sfx_kc2b, player);
	}
}

#undef SPINDASHTHRUSTTIME

boolean K_FastFallBounce(player_t *player)
{
	// Handle fastfall bounce.
	if (player->fastfall != 0)
	{
		const fixed_t maxBounce = mapobjectscale * 10;
		const fixed_t minBounce = mapobjectscale;
		fixed_t bounce = 2 * abs(player->fastfall) / 3;

		if (player->curshield != KSHIELD_BUBBLE && bounce <= 2 * maxBounce)
		{
			// Lose speed on bad bounce.
			// Slow down more as horizontal momentum shrinks
			// compared to vertical momentum.
			angle_t a = R_PointToAngle2(0, 0, 4 * maxBounce, player->speed);
			fixed_t f = FSIN(a);
			player->mo->momx = FixedMul(player->mo->momx, f);
			player->mo->momy = FixedMul(player->mo->momy, f);
		}

		if (bounce > maxBounce)
		{
			bounce = maxBounce;
		}
		else if (bounce < minBounce)
		{
			bounce = minBounce;
		}

		if (player->curshield == KSHIELD_BUBBLE)
		{
			S_StartSound(player->mo, sfx_s3k44);

			// This is a slightly irritating way of doing this, but because ground contact while
			// bubblebouncing gives you 1 tic of ground friction, naively using a factor of player
			// speed makes your sustained speed heavily gamespeed dependent.
			fixed_t minspeed = 12*K_GetKartSpeed(player, false, false)/10;
			fixed_t fallspeed = abs(player->fastfall);
			P_InstaThrust(player->mo, player->mo->angle, 11*max(minspeed, fallspeed)/10);

			player->ignoreAirtimeLeniency = max(player->ignoreAirtimeLeniency, TICRATE);

			bounce += 3 * mapobjectscale;

			UINT8 i;
			UINT8 numplayers = 0;
			if (gametyperules & GTR_CIRCUIT)
			{
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (playeringame[i] && !players[i].spectator)
						numplayers++;
				}
			}
			else
			{
				numplayers = 1; // solo behavior
			}

			if (player->position == 1 && player->positiondelay <= 0 && numplayers != 1)
			{
				S_StartSound(player->mo, sfx_kc31);
				K_StripItems(player);
				K_AddHitLag(player->mo, 4, false);
				vector3_t offset = { 0, 0, 0 };
				K_SpawnSingleHitLagSpark(player->mo, &offset, player->mo->scale*2, 4, 0, player->skincolor);
			}

			if (player->tripwireReboundDelay)
				bounce /= 2;
		}
		else
		{
			S_StartSound(player->mo, sfx_ffbonc);
		}

		if (player->mo->eflags & MFE_UNDERWATER)
			bounce = (117 * bounce) / 200;

		player->pflags |= PF_UPDATEMYRESPAWN;

		player->fastfall = 0;

		// 2.2 - More lenient fastfall
		if (!G_CompatLevel(0x000A))
		{
			if (player->curshield != KSHIELD_BUBBLE)
			{
				// Nudge the player in their facing angle, and provide a little starting momentum if they need it.
				// The bounce is already a strong tradeoff, so this allows it to be used for saves and get you back into flow.
				angle_t momangle = K_MomentumAngle(player->mo);
				fixed_t minspeed = K_GetKartSpeed(player, false, false)/2;
				fixed_t returnspeed = max(FixedHypot(player->mo->momx, player->mo->momy), minspeed);

				// Initial momentum set uses real speed to avoid some weird twitchy behavior at low XY speed
				P_InstaThrust(player->mo, momangle, FixedHypot(player->mo->momx, player->mo->momy)/2);
				P_Thrust(player->mo, player->mo->angle, returnspeed/2);
			}
		}

		player->mo->momz = bounce * P_MobjFlip(player->mo);

		return true;
	}

	return false;
}

static void K_AirFailsafe(player_t *player)
{
	const fixed_t maxSpeed = 6*player->mo->scale;
	const fixed_t thrustSpeed = 6*player->mo->scale; // 10*player->mo->scale

	if (player->speed > maxSpeed // Above the max speed that you're allowed to use this technique.
		|| player->respawn.state != RESPAWNST_NONE // Respawning, you don't need this AND drop dash :V
		|| player->preventfailsafe) // You just got hit or interacted with something committal, no mashing for distance
	{
		player->pflags &= ~PF_AIRFAILSAFE;
		return;
	}

	// Maps with "drop-in" POSITION areas (Dragonspire, Hydrocity) cause problems if we allow failsafe.
	if (leveltime < introtime)
		return;

	UINT8 buttons = K_GetKartButtons(player);

	// Accel inputs queue air-failsafe for when they're released,
	// as long as they're not part of a fastfall attempt.
	if ((buttons & (BT_ACCELERATE|BT_BRAKE)) == BT_ACCELERATE || K_GetForwardMove(player) != 0)
	{
		player->pflags |= PF_AIRFAILSAFE;
		return;
	}

	if (player->pflags & PF_AIRFAILSAFE)
	{
		// Push the player forward
		P_Thrust(player->mo, K_MomentumAngle(player->mo), thrustSpeed);

		S_StartSound(player->mo, sfx_s23c);
		K_SpawnDriftBoostExplosion(player, 0);

		player->pflags &= ~PF_AIRFAILSAFE;
	}
}

//
// K_PlayerBaseFriction
//
fixed_t K_PlayerBaseFriction(const player_t *player, fixed_t original)
{
	const fixed_t factor = FixedMul(
		FixedDiv(FRACUNIT - original, FRACUNIT - ORIG_FRICTION),
		K_GetKartGameSpeedScalar(gamespeed)
	);
	fixed_t frict = original;

	if (player->dashpadcooldown == 0) // attempt to fix Hot Shelter
	{
		if (K_PodiumSequence() == true)
		{
			frict -= FixedMul(FRACUNIT >> 4, factor);
		}
		else if (K_PlayerUsesBotMovement(player) == true)
		{
			const fixed_t speedPercent = min(FRACUNIT, FixedDiv(player->speed, K_GetKartSpeed(player, false, false)));
			const fixed_t extraFriction = FixedMul(FixedMul(FRACUNIT >> 5, factor), speedPercent);

			// A bit extra friction to help them without drifting.
			// Remove this line once they can drift.
			frict -= extraFriction;

			// Bots gain more traction as they rubberband.
			const fixed_t traction_value = FixedMul(player->botvars.rubberband, max(FRACUNIT, K_BotMapModifier()));
			if (traction_value > FRACUNIT)
			{
				const fixed_t traction_mul = traction_value - FRACUNIT;
				frict -= FixedMul(extraFriction, traction_mul);
			}
		}
	}

	if (frict > FRACUNIT) { frict = FRACUNIT; }
	if (frict < 0) { frict = 0; }

	return frict;
}

//
// K_AdjustPlayerFriction
//
void K_AdjustPlayerFriction(player_t *player)
{
	const fixed_t prevfriction = K_PlayerBaseFriction(player, player->mo->friction);

	if (P_IsObjectOnGround(player->mo) == false)
	{
		return;
	}

	player->mo->friction = prevfriction;

	// Reduce friction after hitting a spring
	if (player->tiregrease)
	{
		player->mo->friction += ((FRACUNIT - prevfriction) / greasetics) * player->tiregrease;
	}

	// Less friction on Top unless grinding
	if (player->curshield == KSHIELD_TOP &&
			K_GetForwardMove(player) > 0 &&
			player->speed < 2 * K_GetKartSpeed(player, false, false))
	{
		player->mo->friction += 1024;
	}

	/*
	if (K_PlayerEBrake(player) == true)
	{
		player->mo->friction -= 1024;
	}
	else if (player->speed > 0 && K_GetForwardMove(player) < 0)
	{
		player->mo->friction -= 512;
	}
	*/

	// Water gets ice physics too
	if ((player->mo->eflags & MFE_TOUCHWATER) &&
			!player->offroad)
	{
		player->mo->friction += 614;
	}
	else if ((player->mo->eflags & MFE_UNDERWATER))
	{
		if (!K_Sliptiding(player))
			player->mo->friction += 312;
	}

	// Wipeout slowdown
	if (player->speed > 0 && player->spinouttimer && player->wipeoutslow)
	{
		if (player->offroad)
			player->mo->friction -= 4912;
		if (player->wipeoutslow == 1)
			player->mo->friction -= 9824;
	}

	if (player->icecube.frozen)
	{
		player->mo->friction = FRACUNIT;
	}

	// Cap between intended values
	if (player->mo->friction > FRACUNIT)
		player->mo->friction = FRACUNIT;
	if (player->mo->friction < 0)
		player->mo->friction = 0;

	// Friction was changed, so we must recalculate movefactor
	if (player->mo->friction != prevfriction)
	{
		player->mo->movefactor = FixedDiv(ORIG_FRICTION, player->mo->friction);

		if (player->mo->movefactor < FRACUNIT)
			player->mo->movefactor = 19*player->mo->movefactor - 18*FRACUNIT;
		else
			player->mo->movefactor = FRACUNIT;
	}
}

//
// K_trickPanelTimingVisual
// Spawns the timing visual for trick panels depending on the given player's momz.
// If the player has tricked and is not in hitlag, this will send the half circles flying out.
// if you tumble, they'll fall off instead.
//

#define RADIUSSCALING 6
#define MINRADIUS 12

static void K_trickPanelTimingVisual(player_t *player, fixed_t momz)
{

	fixed_t pos, tx, ty, tz;
	mobj_t *flame;

	angle_t hang = R_PointToAnglePlayer(player, player->mo->x, player->mo->y) + ANG1*90;			// horizontal angle
	angle_t vang = -FixedAngle(momz)*12 + (ANG1*45);												// vertical angle dependant on momz, we want it to line up at 45 degrees at the perfect frame to trick at
	fixed_t dist = FixedMul(max(MINRADIUS<<FRACBITS, abs(momz)*RADIUSSCALING), player->mo->scale);	// distance.

	UINT8 i;

	// Do you like trig? cool, me neither.
	for (i=0; i < 2; i++)
	{
		pos = FixedMul(dist, FINESINE(vang>>ANGLETOFINESHIFT));
		tx = player->mo->x + FixedMul(pos, FINECOSINE(hang>>ANGLETOFINESHIFT));
		ty = player->mo->y + FixedMul(pos, FINESINE(hang>>ANGLETOFINESHIFT));
		tz = player->mo->z + player->mo->height/2 + FixedMul(dist, FINECOSINE(vang>>ANGLETOFINESHIFT));

		// All coordinates set, spawn our fire, now.
		flame = P_SpawnMobj(tx, ty, tz, MT_THOK);

		P_SetScale(flame, player->mo->scale);

		// Visuals
		flame->sprite = SPR_TRCK;
		flame->frame = i|FF_FULLBRIGHT;

		if (player->trickpanel <= TRICKSTATE_READY && !player->tumbleBounces)
		{
			flame->tics = 2;
			flame->momx = player->mo->momx;
			flame->momy = player->mo->momy;
			flame->momz = player->mo->momz;
		}
		else
		{
			flame->tics = TICRATE;

			if (player->trickpanel > TRICKSTATE_READY)	// we tricked
			{
				// Send the thing outwards via ghetto maths which involves redoing the whole 3d sphere again, witht the "vertical" angle shifted by 90 degrees.
				// There's probably a simplier way to do this the way I want to but this works.
				pos = FixedMul(48*player->mo->scale, FINESINE((vang +ANG1*90)>>ANGLETOFINESHIFT));
				tx = player->mo->x + FixedMul(pos, FINECOSINE(hang>>ANGLETOFINESHIFT));
				ty = player->mo->y + FixedMul(pos, FINESINE(hang>>ANGLETOFINESHIFT));
				tz = player->mo->z + player->mo->height/2 + FixedMul(48*player->mo->scale, FINECOSINE((vang +ANG1*90)>>ANGLETOFINESHIFT));

				flame->momx = tx -player->mo->x;
				flame->momy = ty -player->mo->y;
				flame->momz = tz -(player->mo->z+player->mo->height/2);
			}
			else	// we failed the trick, drop the half circles, it'll be funny I promise.
			{
				flame->flags &= ~MF_NOGRAVITY;
				P_SetObjectMomZ(flame, 4<<FRACBITS, false);
				P_InstaThrust(flame, R_PointToAngle2(player->mo->x, player->mo->y, flame->x, flame->y), 8*mapobjectscale);
				flame->momx += player->mo->momx;
				flame->momy += player->mo->momy;
				flame->momz += player->mo->momz;
			}
		}

		// make sure this is only drawn for our local player
		flame->renderflags |= (RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(player));

		vang += FixedAngle(180<<FRACBITS);	// Avoid overflow warnings...

	}
}

#undef RADIUSSCALING
#undef MINRADIUS

void K_SetItemOut(player_t *player)
{
	player->itemflags |= IF_ITEMOUT;

	if (player->mo->scale >= FixedMul(GROW_PHYSICS_SCALE, mapobjectscale))
	{
		player->itemscale = ITEMSCALE_GROW;
	}
	else if (player->mo->scale <= FixedMul(SHRINK_PHYSICS_SCALE, mapobjectscale))
	{
		player->itemscale = ITEMSCALE_SHRINK;
	}
	else
	{
		player->itemscale = ITEMSCALE_NORMAL;
	}
}

void K_UnsetItemOut(player_t *player)
{
	player->itemflags &= ~IF_ITEMOUT;
	player->itemscale = ITEMSCALE_NORMAL;
	player->bananadrag = 0;
}

//
// K_MoveKartPlayer
//
void K_MoveKartPlayer(player_t *player, boolean onground)
{
	ticcmd_t *cmd = &player->cmd;
	boolean ATTACK_IS_DOWN = ((cmd->buttons & BT_ATTACK) && !(player->oldcmd.buttons & BT_ATTACK) && (player->respawn.state == RESPAWNST_NONE));
	boolean HOLDING_ITEM = (player->itemflags & (IF_ITEMOUT|IF_EGGMANOUT));
	boolean NO_HYUDORO = (player->stealingtimer == 0);

	// Play overtake sounds, but only if
	// - your place changed
	// - not exiting
	// - in relevant gametype
	// - more than 10 seconds after initial scramble
	if (player->oldposition != player->position
	&& !player->exiting
	&& (gametyperules & GTR_CIRCUIT)
	&& !K_Cooperative()
	&& leveltime >= starttime+(10*TICRATE))
	{
		if (player->oldposition < player->position) // But first, if you lost a place,
		{
			// then the other player taunts.
			K_RegularVoiceTimers(player); // and you can't for a bit
		}
		else // Otherwise,
		{
			K_PlayOvertakeSound(player->mo); // Say "YOU'RE TOO SLOW!"
		}
	}
	player->oldposition = player->position;

	// Prevent ring misfire
	if (!(cmd->buttons & BT_ATTACK))
	{
		if (player->itemtype == KITEM_NONE
			&& NO_HYUDORO && !(HOLDING_ITEM
			|| player->itemamount
			|| player->itemRoulette.active == true
			|| player->rocketsneakertimer
			|| player->eggmanexplode))
			player->itemflags |= IF_USERINGS;
		else
			player->itemflags &= ~IF_USERINGS;
	}

	if (player->ringboxdelay)
	{
		player->ringboxdelay--;
		if (player->ringboxdelay == 0)
		{
			UINT32 award = 5*player->ringboxaward + 10;
			if (!K_ThunderDome())
				award = 3 * award / 2;

			if (modeattacking & ATTACKING_SPB)
			{
				// SPB Attack is hard.
				award = award / 2;
			}
			else if (modeattacking)
			{
				// At high distance values, the power of Ring Box is mainly an extra source of speed, to be
				// stacked with power items (or itself!) during the payout period.
				// Low-dist Ring Box follows some special rules, to somewhat normalize the reward between stat
				// blocks that respond to rings differently; here, variance in payout period counts for a lot!

				UINT8 accel = 10-player->kartspeed;
				UINT8 weight = player->kartweight;

				// Fixed point math can suck a dick.

				if (accel > weight)
				{
					accel *= 10;
					weight *= 3;
				}
				else
				{
					accel *= 3;
					weight *= 10;
				}

				award = (110 + accel + weight) * award / 120;
			}
			else
			{
				UINT32 behind = K_GetItemRouletteDistance(player, player->itemRoulette.playing);
				UINT32 behindMulti = behind / 500;
				behindMulti = min(behindMulti, 60);
				award = award * (behindMulti + 10) / 10;
			}

			K_AwardPlayerRings(player, award, true);
			player->ringboxaward = 0;
		}
	}

	// This looks a lot like the check that's right under it, but this check is specifically for instawhip charge,
	// which is allowed during painstate as a last-ditch defensive option.
	if (player && player->mo && player->mo->health > 0 && !player->spectator && !mapreset && leveltime > introtime)
	{
		boolean chargingwhip = (cmd->buttons & BT_ATTACK) && (player->rings <= 0) && (!player->instaWhipChargeLockout);
		boolean releasedwhip = (!(cmd->buttons & BT_ATTACK)) && (player->rings <= 0 && player->instaWhipCharge) && !(P_PlayerInPain(player));

		if (K_PowerUpRemaining(player, POWERUP_BADGE))
		{
			if (P_PlayerInPain(player))
			{
				releasedwhip = false;
				player->instaWhipCharge = 0;
			}
			else
			{
				releasedwhip = (ATTACK_IS_DOWN && player->rings <= 0 && player->itemflags & IF_USERINGS);
				player->instaWhipCharge = INSTAWHIP_CHARGETIME;
			}

			chargingwhip = false;
		}

		if (leveltime < starttime || player->itemflags & (IF_ITEMOUT|IF_EGGMANOUT) || player->rocketsneakertimer || (player->defenseLockout && !K_PowerUpRemaining(player, POWERUP_BADGE)))
		{
			chargingwhip = false;
			player->instaWhipCharge = 0;
		}

		if (chargingwhip && (K_PressingEBrake(player) && player->drift == 0))
		{
			// 1) E-braking on the ground: cancels Insta-Whip.
			//    Still lets you keep your Whip while fast-falling.
			// 2) Do not interrupt Guard.
			if (P_IsObjectOnGround(player->mo) || K_PlayerGuard(player))
			{
				if (player->instaWhipCharge)
					player->defenseLockout = PUNISHWINDOW;
				player->instaWhipCharge = 0;
			}
		}
		else if (chargingwhip)
		{
			player->instaWhipCharge = min(player->instaWhipCharge + 1, INSTAWHIP_TETHERBLOCK + 1);

			// OOPSIES FIXME pt.1: You can charge instawhip between picking up and confirming an item box.
			// This is vanishingly rare in race and impossible in battle, but SUPER common in prisons!
			//
			// ...But this was discovered after staff ghost recording started, and charging whip prevents
			// flingring pickup, meaning it has a small but desync-capable gameplay effect.
			//
			// So instead of stopping charge, we hide it. How embarrassing!
			if (player->instaWhipCharge == 1 && !player->itemRoulette.active)
			{
				Obj_SpawnInstaWhipRecharge(player, 0);
				Obj_SpawnInstaWhipRecharge(player, ANGLE_120);
				Obj_SpawnInstaWhipRecharge(player, ANGLE_240);
			}

			if (player->instaWhipCharge == INSTAWHIP_CHARGETIME)
			{
				Obj_SpawnInstaWhipReject(player);
			}

			if (player->instaWhipCharge > INSTAWHIP_CHARGETIME)
			{
				if ((leveltime%(INSTAWHIP_RINGDRAINEVERY)) == 0 && !(gametyperules & GTR_SPHERES))
				{
					if (player->rings > -20 && P_IsDisplayPlayer(player))
						S_StartSound(player->mo, sfx_antiri);
					player->rings--;
				}
			}
		}
		else if (releasedwhip)
		{
			if (player->instaWhipCharge < INSTAWHIP_CHARGETIME)
			{
				// OOPSIES FIXME pt.3: See OOPSIES FIXME pt.1.
				if (!player->itemRoulette.active)
					S_StartSound(player->mo, sfx_kc50);
				player->instaWhipCharge = 0;
				player->botvars.itemconfirm = 0;
			}
			else
			{
				player->instaWhipCharge = 0;
				if (!K_PowerUpRemaining(player, POWERUP_BARRIER))
				{
					player->defenseLockout = PUNISHWINDOW;
				}

				S_StartSound(player->mo, sfx_iwhp);
				mobj_t *whip = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_INSTAWHIP);
				P_SetTarget(&player->whip, whip);
				P_SetScale(whip, player->mo->scale);
				P_SetTarget(&whip->target, player->mo);
				K_MatchGenericExtraFlags(whip, player->mo);
				P_SpawnFakeShadow(whip, 20);
				whip->fuse = INSTAWHIP_DURATION;
				player->flashing = max(player->flashing, INSTAWHIP_DURATION);

				if (P_IsObjectOnGround(player->mo))
				{
					whip->flags2 |= MF2_AMBUSH;
				}

				player->botvars.itemconfirm = 0;
			}
		}
		else if (!(player->instaWhipCharge >= INSTAWHIP_CHARGETIME && P_PlayerInPain(player))) // Allow reversal whip
			player->instaWhipCharge = 0;
	}

	// OOPSIES FIXME pt.4: This one is kind of esoteric and only theoretically abusable, but
	// if a player picks up an item during the instawhip input safety window—the one that triggers
	// after you burn to 0 rings—they can continue to hold the input, then charge a usable whip
	// without stopping the roulette and acquiring an item, which cancels it.
	//
	// No ghosts use this technique, but your least favorite tournament player might.
	if (player->itemRoulette.active)
	{
		player->instaWhipCharge = min(player->instaWhipCharge, INSTAWHIP_CHARGETIME - 2);
	}

	if (player && player->mo && K_PlayerCanUseItem(player))
	{
		// First, the really specific, finicky items that function without the item being directly in your item slot.
		{
			// Ring boosting
			if (player->itemflags & IF_USERINGS)
			{
				// Auto-Ring
				UINT8 tiereddelay = 5;
				player->autoring = false;
				if (
					player->pflags & PF_AUTORING
					&& leveltime > starttime
					&& K_GetForwardMove(player) > 0
					&& P_IsObjectOnGround(player->mo)
				)
				{
					fixed_t pspeed = FixedDiv(player->speed * 100, K_GetKartSpeed(player, false, true));

					if (player->rings >= 18 && pspeed < 100*FRACUNIT)
					{
						player->autoring = true;
						tiereddelay = 3;
					}
					else if (player->rings >= 10 && pspeed < 85*FRACUNIT)
					{
						player->autoring = true;
						tiereddelay = 4;
					}
					else if (player->rings >= 4 && pspeed < 35*FRACUNIT)
					{
						player->autoring = true;
						tiereddelay = 5;
					}
					else
						player->autoring = false;
				}

				if (((cmd->buttons & BT_ATTACK) || player->autoring) && !player->ringdelay && player->rings > 0)
				{
					mobj_t *ring = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_RING);
					P_SetMobjState(ring, S_FASTRING1);

					if (P_IsDisplayPlayer(player))
					{
						UINT8 startfade = 220;
						UINT8 transfactor = 10 * (min(startfade, player->ringtransparency)) / startfade;
						if (transfactor < 10)
						{
							transfactor = max(transfactor, 4);
							ring->renderflags |= ((10-transfactor) << RF_TRANSSHIFT);
							ring->renderflags |= RF_ADD;
						}
					}
					player->ringtransparency -= RINGTRANSPARENCYUSEPENALTY;

					ring->extravalue1 = 1; // Ring use animation timer
					ring->extravalue2 = 1; // Ring use animation flag
					ring->shadowscale = 0;
					P_SetTarget(&ring->target, player->mo); // user

					const INT32 followerskin = K_GetEffectiveFollowerSkin(player);
					if (player->autoring
						&& player->follower != NULL
						&& P_MobjWasRemoved(player->follower) == false
						&& followerskin >= 0
						&& followerskin < numfollowers)
					{
						const follower_t *fl = &followers[followerskin];

						ring->cusval = player->follower->x - player->mo->x;
						ring->cvmem = player->follower->y - player->mo->y;
						ring->movefactor = P_GetMobjHead(player->follower) - P_GetMobjHead(player->mo);

						// cvmem is used to play the ring animation for followers
						player->follower->cvmem = fl->ringtime;
					}
					else
					{
						ring->cusval = ring->cvmem = ring->movefactor = 0;
					}

					// really silly stupid dumb HACK to fix interp
					// without needing to duplicate any code
					A_AttractChase(ring);
					P_SetOrigin(ring, ring->x, ring->y, ring->z);
					ring->extravalue1 = 1;

					player->rings--;
					if (player->autoring && !(cmd->buttons & BT_ATTACK))
						player->ringdelay = tiereddelay;
					else
						player->ringdelay = 3;
				}

			}
			// Other items
			else
			{
				// Eggman Monitor exploding
				if (player->eggmanexplode)
				{
					if (ATTACK_IS_DOWN && player->eggmanexplode <= 3*TICRATE && player->eggmanexplode > 1)
					{
						player->eggmanexplode = 1;
						player->botvars.itemconfirm = 0;
					}
				}
				// Eggman Monitor throwing
				else if (player->itemflags & IF_EGGMANOUT)
				{
					if (ATTACK_IS_DOWN)
					{
						K_ThrowKartItem(player, false, MT_EGGMANITEM, -1, 0, 0);
						K_PlayAttackTaunt(player->mo);
						player->itemflags &= ~IF_EGGMANOUT;
						K_UpdateHnextList(player, true);
						player->botvars.itemconfirm = 0;
					}
				}
				// Rocket Sneaker usage
				else if (player->rocketsneakertimer > 1)
				{
					if (ATTACK_IS_DOWN && !HOLDING_ITEM && onground && NO_HYUDORO)
					{
						K_DoSneaker(player, 2);
						K_PlayBoostTaunt(player->mo);
						if (player->rocketsneakertimer <= 3*TICRATE)
							player->rocketsneakertimer = 1;
						else
							player->rocketsneakertimer -= 3*TICRATE;
						player->botvars.itemconfirm = 2*TICRATE;
					}
				}
				else if (player->itemamount == 0)
				{
					K_UnsetItemOut(player);
				}
				else
				{
					switch (player->itemtype)
					{
						case KITEM_SNEAKER:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && onground && NO_HYUDORO)
							{
								K_DoSneaker(player, 1);
								K_PlayBoostTaunt(player->mo);
								player->itemamount--;
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_ROCKETSNEAKER:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && onground && NO_HYUDORO
								&& player->rocketsneakertimer == 0)
							{
								INT32 moloop;
								mobj_t *mo = NULL;
								mobj_t *prev = player->mo;

								K_PlayBoostTaunt(player->mo);
								S_StartSound(player->mo, sfx_s3k3a);

								//K_DoSneaker(player, 2);

								player->rocketsneakertimer = (itemtime*3);
								player->itemamount--;
								K_UpdateHnextList(player, true);

								for (moloop = 0; moloop < 2; moloop++)
								{
									mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_ROCKETSNEAKER);
									K_MatchGenericExtraFlags(mo, player->mo);
									mo->flags |= MF_NOCLIPTHING;
									mo->angle = player->mo->angle;
									mo->threshold = 10;
									mo->movecount = moloop%2;
									mo->movedir = mo->lastlook = moloop+1;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&mo->hprev, prev);
									P_SetTarget(&prev->hnext, mo);
									prev = mo;
								}
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_INVINCIBILITY:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO) // Doesn't hold your item slot hostage normally, so you're free to waste it if you have multiple
							{
								UINT32 behind = K_GetItemRouletteDistance(player, player->itemRoulette.playing);
								UINT32 behindScaled = behind * TICRATE / 4500;
								behindScaled = min(behindScaled, 10*TICRATE);

								K_DoInvincibility(player,
									max(7u * TICRATE + behindScaled, player->invincibilitytimer + 5u*TICRATE));
								K_PlayPowerGloatSound(player->mo);

								player->itemamount--;
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_BANANA:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								INT32 moloop;
								mobj_t *mo;
								mobj_t *prev = player->mo;

								//K_PlayAttackTaunt(player->mo);
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s254);

								for (moloop = 0; moloop < player->itemamount; moloop++)
								{
									mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BANANA_SHIELD);
									if (!mo)
									{
										player->itemamount = moloop;
										break;
									}
									mo->flags |= MF_NOCLIPTHING;
									mo->threshold = 10;
									mo->movecount = player->itemamount;
									mo->movedir = moloop+1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&mo->hprev, prev);
									P_SetTarget(&prev->hnext, mo);
									prev = mo;
								}
								player->botvars.itemconfirm = 0;
							}
							else if (ATTACK_IS_DOWN && (player->itemflags & IF_ITEMOUT)) // Banana x3 thrown
							{
								player->itemamount--;
								K_ThrowKartItem(player, false, MT_BANANA, -1, 0, 0);
								K_PlayAttackTaunt(player->mo);
								K_UpdateHnextList(player, false);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_EGGMAN:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								mobj_t *mo;
								player->itemamount--;
								player->itemflags |= IF_EGGMANOUT;
								S_StartSound(player->mo, sfx_s254);
								mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_EGGMANITEM_SHIELD);
								if (mo)
								{
									K_FlipFromObject(mo, player->mo);
									mo->flags |= MF_NOCLIPTHING;
									mo->threshold = 10;
									mo->movecount = 1;
									mo->movedir = 1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&player->mo->hnext, mo);
								}
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_ORBINAUT:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								angle_t newangle;
								INT32 moloop;
								mobj_t *mo = NULL;
								mobj_t *prev = player->mo;

								//K_PlayAttackTaunt(player->mo);
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s3k3a);

								for (moloop = 0; moloop < player->itemamount; moloop++)
								{
									newangle = (player->mo->angle + ANGLE_157h) + FixedAngle(((360 / player->itemamount) * moloop) << FRACBITS) + ANGLE_90;
									mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_ORBINAUT_SHIELD);
									if (!mo)
									{
										player->itemamount = moloop;
										break;
									}
									mo->flags |= MF_NOCLIPTHING;
									mo->angle = newangle;
									mo->threshold = 10;
									mo->movecount = player->itemamount;
									mo->movedir = mo->lastlook = moloop+1;
									mo->color = player->skincolor;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&mo->hprev, prev);
									P_SetTarget(&prev->hnext, mo);
									prev = mo;
								}
								player->botvars.itemconfirm = 0;
							}
							else if (ATTACK_IS_DOWN && (player->itemflags & IF_ITEMOUT)) // Orbinaut x3 thrown
							{
								player->itemamount--;
								K_ThrowKartItem(player, true, MT_ORBINAUT, 1, 0, 0);
								K_PlayAttackTaunt(player->mo);
								K_UpdateHnextList(player, false);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_JAWZ:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								angle_t newangle;
								INT32 moloop;
								mobj_t *mo = NULL;
								mobj_t *prev = player->mo;

								//K_PlayAttackTaunt(player->mo);
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s3k3a);

								for (moloop = 0; moloop < player->itemamount; moloop++)
								{
									newangle = (player->mo->angle + ANGLE_157h) + FixedAngle(((360 / player->itemamount) * moloop) << FRACBITS) + ANGLE_90;
									mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_JAWZ_SHIELD);
									if (!mo)
									{
										player->itemamount = moloop;
										break;
									}
									mo->flags |= MF_NOCLIPTHING;
									mo->angle = newangle;
									mo->threshold = 10;
									mo->movecount = player->itemamount;
									mo->movedir = mo->lastlook = moloop+1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&mo->hprev, prev);
									P_SetTarget(&prev->hnext, mo);
									prev = mo;
								}
								player->botvars.itemconfirm = 0;
							}
							else if (ATTACK_IS_DOWN && HOLDING_ITEM && (player->itemflags & IF_ITEMOUT)) // Jawz thrown
							{
								player->itemamount--;
								K_ThrowKartItem(player, true, MT_JAWZ, 1, 0, 0);
								K_PlayAttackTaunt(player->mo);
								K_UpdateHnextList(player, false);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_MINE:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								mobj_t *mo;
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s254);
								mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SSMINE_SHIELD);
								if (mo)
								{
									mo->flags |= MF_NOCLIPTHING;
									mo->threshold = 10;
									mo->movecount = 1;
									mo->movedir = 1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&player->mo->hnext, mo);
								}
								player->botvars.itemconfirm = 0;
							}
							else if (ATTACK_IS_DOWN && (player->itemflags & IF_ITEMOUT))
							{
								player->itemamount--;
								K_ThrowKartItem(player, false, MT_SSMINE, 1, 1, 0);
								K_PlayAttackTaunt(player->mo);
								player->itemflags &= ~IF_ITEMOUT;
								K_UpdateHnextList(player, true);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_LANDMINE:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								player->itemamount--;
								K_ThrowLandMine(player);
								K_PlayAttackTaunt(player->mo);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_DROPTARGET:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								mobj_t *mo;
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s254);
								mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_DROPTARGET_SHIELD);
								if (mo)
								{
									mo->flags |= MF_NOCLIPTHING;
									mo->threshold = 10;
									mo->movecount = 1;
									mo->movedir = 1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&player->mo->hnext, mo);
								}
								player->botvars.itemconfirm = 0;
							}
							else if (ATTACK_IS_DOWN && (player->itemflags & IF_ITEMOUT))
							{
								player->itemamount--;
								K_ThrowKartItem(player, (player->throwdir > 0), MT_DROPTARGET, -1, 0, 0);
								K_PlayAttackTaunt(player->mo);
								player->itemflags &= ~IF_ITEMOUT;
								K_UpdateHnextList(player, true);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_BALLHOG:
							if (!HOLDING_ITEM && NO_HYUDORO)
							{
								INT32 ballhogmax = (player->itemamount) * BALLHOGINCREMENT;

								// This construct looks a little goofy, but we're basically just
								// trying to prevent rapid taps from restarting a charge, while
								// still allowing quick tapfire.
								// (The player still has to pace their shots like this, it's not
								// semi-auto, but that's probably kind of okay.)
								if (player->ballhogcharge && !(cmd->buttons & BT_ATTACK))
									player->ballhogtap = true;

								if (player->ballhogcharge == 0)
									player->ballhogtap = false;

								boolean realcharge = (cmd->buttons & BT_ATTACK) && (player->itemflags & IF_HOLDREADY) && (player->ballhogcharge < ballhogmax);
								if ((realcharge && !player->ballhogtap) || (player->ballhogtap && player->ballhogcharge < BALLHOGINCREMENT))
								{
									player->ballhogcharge++;
									if (player->ballhogcharge % BALLHOGINCREMENT == 0)
									{
										sfxenum_t hogsound[] =
										{
											sfx_bhog00,
											sfx_bhog01,
											sfx_bhog02,
											sfx_bhog03,
											sfx_bhog04,
											sfx_bhog05
										};
										UINT8 chargesound = max(1, min(player->ballhogcharge / BALLHOGINCREMENT, 6));
										S_StartSound(player->mo, hogsound[chargesound-1]);
									}
								}
								else
								{
									if (cmd->buttons & BT_ATTACK)
									{
										player->itemflags &= ~IF_HOLDREADY;
									}
									else
									{
										player->itemflags |= IF_HOLDREADY;
									}

									if (player->ballhogcharge > 0)
									{
										INT32 numhogs = min((player->ballhogcharge / BALLHOGINCREMENT), player->itemamount);

										K_SetItemOut(player); // need this to set itemscale

										if (numhogs <= 0)
										{
											// no tapfire scams
										}
										else if (numhogs == 1)
										{
											player->itemamount--;
											K_ThrowKartItem(player, true, MT_BALLHOG, 1, 0, 0);
											K_PlayAttackTaunt(player->mo);
										}
										else
										{
											angle_t cone = 0x01800000 * (numhogs-1);
											angle_t offsetAmt = (cone * 2) / (numhogs-1);
											angle_t angleOffset = cone;
											INT32 i;

											player->itemamount -= numhogs;

											for (i = 0; i < numhogs; i++)
											{
												K_ThrowKartItem(player, true, MT_BALLHOG, 1, 0, angleOffset);
												angleOffset -= offsetAmt;
											}

											K_PlayAttackTaunt(player->mo);
										}

										K_UnsetItemOut(player);
										player->ballhogcharge = 0;
										player->itemflags &= ~IF_HOLDREADY;
										player->botvars.itemconfirm = 0;
									}
								}
							}
							break;
						case KITEM_SPB:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								player->itemamount--;
								K_SetItemOut(player);
								K_ThrowKartItem(player, true, MT_SPB, 1, 0, 0);
								K_UnsetItemOut(player);
								K_PlayAttackTaunt(player->mo);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_GROW:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								if (player->growshrinktimer < 0)
								{
									// Old v1 behavior was to have Grow counter Shrink,
									// but Shrink is now so ephemeral that it should just work.
									K_RemoveGrowShrink(player);
									// So we fall through here.
								}

								K_PlayPowerGloatSound(player->mo);

								player->mo->scalespeed = mapobjectscale/TICRATE;
								player->mo->destscale = FixedMul(mapobjectscale, GROW_SCALE);

								if (K_PlayerShrinkCheat(player) == true)
								{
									player->mo->destscale = FixedMul(player->mo->destscale, SHRINK_SCALE);
								}

								if (P_IsPartyPlayer(player) == false && player->invincibilitytimer == 0)
								{
									// don't play this if the player has invincibility -- that takes priority
									S_StartSound(player->mo, sfx_alarmg);
								}

								player->growshrinktimer = max(0, player->growshrinktimer);
								player->growshrinktimer += ((gametyperules & GTR_CLOSERPLAYERS) ? 8 : 12) * TICRATE;

								S_StartSound(player->mo, sfx_kc5a);

								player->itemamount--;
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_SHRINK:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								K_DoShrink(player);
								player->itemamount--;
								K_PlayPowerGloatSound(player->mo);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_LIGHTNINGSHIELD:
							if (player->curshield != KSHIELD_LIGHTNING)
							{
								mobj_t *shield = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_LIGHTNINGSHIELD);
								P_SetScale(shield, (shield->destscale = (5*shield->destscale)>>2));
								P_SetTarget(&shield->target, player->mo);
								S_StartSound(player->mo, sfx_s3k41);
								player->curshield = KSHIELD_LIGHTNING;
							}

							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								K_DoLightningShield(player);
								if (player->itemamount > 0)
								{
									// Why is this a conditional?
									// Lightning shield: the only item that allows you to
									// activate a mine while you're out of its radius,
									// the SAME tic it sets your itemamount to 0
									// ...:dumbestass:
									player->itemamount--;
									K_PlayAttackTaunt(player->mo);
									player->botvars.itemconfirm = 0;
								}
							}
							break;
						case KITEM_GARDENTOP:
							if (player->curshield == KSHIELD_TOP && K_GetGardenTop(player) == NULL)
							{
								Obj_GardenTopDeploy(player->mo);
								player->botvars.itemconfirm = 0;
							}
							else if (ATTACK_IS_DOWN && NO_HYUDORO)
							{
								if (player->curshield != KSHIELD_TOP)
								{
									player->topinfirst = 0;
									Obj_GardenTopDeploy(player->mo);
								}
								else
								{
									if (player->throwdir == -1)
									{
										const angle_t angle = P_IsObjectOnGround(player->mo) ?
											player->mo->angle : K_MomentumAngle(player->mo);

										mobj_t *top = Obj_GardenTopDestroy(player);

										// Fly off the Top at high speed
										P_InstaThrust(player->mo, angle, player->speed + (80 * mapobjectscale));
										P_SetObjectMomZ(player->mo, player->mo->info->height / 8, true);

										if (top != NULL)
										{
											top->momx = player->mo->momx;
											top->momy = player->mo->momy;
											top->momz = player->mo->momz;
										}
									}
									else
									{
										Obj_GardenTopThrow(player);
										S_StartSound(player->mo, sfx_tossed); // play only when actually thrown :^,J
										K_PlayAttackTaunt(player->mo);
									}
								}

								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_BUBBLESHIELD:
							if (player->curshield != KSHIELD_BUBBLE)
							{
								mobj_t *shield = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BUBBLESHIELD);
								// MT_BUBBLESHIELD doesn't have MF_NOBLOCKMAP so we need to remove this manually.	
								// Otherwise if you roll a bubble shield while flipped, the visuals look too mismatched.
								shield->eflags &= ~MFE_VERTICALFLIP;
								P_SetScale(shield, (shield->destscale = (5*shield->destscale)>>2));
								P_SetTarget(&shield->target, player->mo);
								S_StartSound(player->mo, sfx_s3k3f);
								player->curshield = KSHIELD_BUBBLE;
							}

							if (!HOLDING_ITEM && NO_HYUDORO)
							{
								if ((cmd->buttons & BT_ATTACK) && (player->itemflags & IF_HOLDREADY))
								{
									if (player->bubbleblowup == 0)
										S_StartSound(player->mo, sfx_s3k75);

									player->bubbleblowup++;
									player->bubblecool = player->bubbleblowup*4;

									if (player->bubbleblowup > bubbletime*2)
									{
										player->itemamount--;
										K_ThrowKartItem(player, (player->throwdir > 0), MT_BUBBLESHIELDTRAP, -1, 0, 0);
										if (player->throwdir == -1)
										{
											P_InstaThrust(player->mo, player->mo->angle, player->speed + (80 * mapobjectscale));
											player->wavedashboost += TICRATE;
											player->wavedashpower = FRACUNIT;
											player->fakeBoost += TICRATE/2;
										}
										K_PlayAttackTaunt(player->mo);
										player->bubbleblowup = 0;
										player->bubblecool = 0;
										player->itemflags &= ~IF_HOLDREADY;
										player->botvars.itemconfirm = 0;
									}
								}
								else
								{
									if (player->bubbleblowup > bubbletime)
										player->bubbleblowup = bubbletime;

									if (player->bubbleblowup)
										player->bubbleblowup--;

									if (player->bubblecool)
										player->itemflags &= ~IF_HOLDREADY;
									else
										player->itemflags |= IF_HOLDREADY;
								}
							}
							break;
						case KITEM_FLAMESHIELD:
							if (player->curshield != KSHIELD_FLAME)
							{
								mobj_t *shield = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_FLAMESHIELD);
								P_SetScale(shield, (shield->destscale = (5*shield->destscale)>>2));
								P_SetTarget(&shield->target, player->mo);
								S_StartSound(player->mo, sfx_s3k3e);
								player->curshield = KSHIELD_FLAME;
							}

							if (!HOLDING_ITEM && NO_HYUDORO)
							{
								INT32 destlen = K_FlameShieldMax(player);
								INT32 flamemax = 0;

								if (player->flamelength < destlen)
									player->flamelength = min(destlen, player->flamelength + 7); // Allows gauge to grow quickly when first acquired. 120/16 = ~7

								flamemax = player->flamelength + TICRATE; // TICRATE leniency period, but we block most effects at flamelength 0 down below

								if ((cmd->buttons & BT_ATTACK) && (player->itemflags & IF_HOLDREADY))
								{
									const INT32 incr = (gametyperules & GTR_CLOSERPLAYERS) ? 4 : 2;
									player->flamemeter += incr;

									if (player->flamelength)
									{

										if (player->flamedash == 0)
										{
											//S_StartSound(player->mo, sfx_s3k43);
											K_PlayBoostTaunt(player->mo);
											S_StartSoundAtVolume(player->mo, sfx_fshld0, 255/3);
											S_StartSoundAtVolume(player->mo, sfx_fshld1, 255/3);
										}

										S_StopSoundByID(player->mo, sfx_fshld3);

										player->flamedash += incr;

										if (!onground)
										{
											P_Thrust(
												player->mo, K_MomentumAngle(player->mo),
												FixedMul(player->mo->scale, K_GetKartGameSpeedScalar(gamespeed))
											);
										}
									}

									if (player->flamemeter > flamemax)
									{
										P_Thrust(
											player->mo, player->mo->angle,
											FixedMul((50*player->mo->scale), K_GetKartGameSpeedScalar(gamespeed))
										);

										player->wavedashboost += TICRATE;
										player->wavedashpower = FRACUNIT;
										player->fakeBoost = TICRATE/3;

										S_StopSoundByID(player->mo, sfx_fshld1);
										S_StopSoundByID(player->mo, sfx_fshld0);
										S_StartSoundAtVolume(player->mo, sfx_fshld2, 255/3);

										player->flamemeter = 0;
										player->flamelength = 0;
										player->itemflags &= ~IF_HOLDREADY;
										player->itemamount--;
									}
								}
								else
								{
									player->itemflags |= IF_HOLDREADY;

									if (!(gametyperules & GTR_CLOSERPLAYERS) || leveltime % 6 == 0)
									{
										if (player->flamemeter > 0)
										{
											player->flamemeter--;
											if (!player->flamemeter)
												S_StopSoundByID(player->mo, sfx_fshld3);
										}
									}

									if (player->flamelength > destlen)
									{
										player->flamelength--; // Can ONLY go down if you're not using it

										flamemax = player->flamelength;
										if (flamemax > 0)
											flamemax += TICRATE; // leniency period
									}

									if (player->flamemeter > flamemax)
										player->flamemeter = flamemax;
								}
							}
							break;
						case KITEM_HYUDORO:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								player->itemamount--;
								//K_DoHyudoroSteal(player); // yes. yes they do.
								Obj_HyudoroDeploy(player->mo);
								K_PlayAttackTaunt(player->mo);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_POGOSPRING:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && onground && NO_HYUDORO && player->trickpanel == TRICKSTATE_NONE)
							{
								K_PlayBoostTaunt(player->mo);
								//K_DoPogoSpring(player->mo, 32<<FRACBITS, 2);
								P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_POGOSPRING);
								player->itemamount--;
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_SUPERRING:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								K_AwardPlayerRings(player, 20, true);
								player->itemamount--;
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_KITCHENSINK:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								mobj_t *mo;
								K_SetItemOut(player);
								S_StartSound(player->mo, sfx_s254);
								mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SINK_SHIELD);
								if (mo)
								{
									mo->flags |= MF_NOCLIPTHING;
									mo->threshold = 10;
									mo->movecount = 1;
									mo->movedir = 1;
									mo->cusval = player->itemscale;
									P_SetTarget(&mo->target, player->mo);
									P_SetTarget(&player->mo->hnext, mo);
								}
								player->botvars.itemconfirm = 0;
							}
							else if (ATTACK_IS_DOWN && HOLDING_ITEM && (player->itemflags & IF_ITEMOUT)) // Sink thrown
							{
								player->itemamount--;
								K_ThrowKartItem(player, false, MT_SINK, 1, 2, 0);
								K_PlayAttackTaunt(player->mo);
								player->itemflags &= ~IF_ITEMOUT;
								K_UpdateHnextList(player, true);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_GACHABOM:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO)
							{
								player->itemamount--;
								K_SetItemOut(player); // need this to set itemscale
								K_ThrowKartItem(player, true, MT_GACHABOM, 0, 0, 0);
								K_UnsetItemOut(player);
								K_PlayAttackTaunt(player->mo);
								player->roundconditions.gachabom_miser = (
									(player->roundconditions.gachabom_miser == 0)
										? 1 : 0xFF
								);
								K_UpdateHnextList(player, false);
								player->botvars.itemconfirm = 0;
							}
							break;
						case KITEM_SAD:
							if (ATTACK_IS_DOWN && !HOLDING_ITEM && NO_HYUDORO
								&& !player->sadtimer)
							{
								player->sadtimer = stealtime;
								player->itemamount--;
								player->botvars.itemconfirm = 0;
							}
							break;
						default:
							break;
					}
				}
			}
		}

		// No more!
		if (!player->itemamount)
		{
			player->itemflags &= ~IF_ITEMOUT;
			player->itemtype = KITEM_NONE;
		}

		if (K_GetShieldFromItem(player->itemtype) == KSHIELD_NONE)
		{
			player->curshield = KSHIELD_NONE; // RESET shield type
			player->bubbleblowup = 0;
			player->bubblecool = 0;
			player->flamelength = 0;
			player->flamemeter = 0;
		}

		if (spbplace == -1 || player->position != spbplace)
		{
			player->pflags &= ~PF_RINGLOCK; // reset ring lock
		}


		if (K_ItemSingularity(player->itemtype) == true)
		{
			K_SetItemCooldown(player->itemtype, 20*TICRATE);
		}

		if (player->hyudorotimer > 0)
		{
			player->mo->renderflags |= RF_DONTDRAW | RF_MODULATE;
			player->mo->renderflags &= ~K_GetPlayerDontDrawFlag(player);

			if (!(leveltime & 1) && (player->hyudorotimer < (TICRATE/2) || player->hyudorotimer > hyudorotime-(TICRATE/2)))
			{
				player->mo->renderflags &= ~(RF_DONTDRAW | RF_BLENDMASK);
			}

			player->flashing = player->hyudorotimer; // We'll do this for now, let's people know about the invisible people through subtle hints
		}
		else if (player->hyudorotimer == 0)
		{
			player->mo->renderflags &= ~RF_BLENDMASK;
		}

		if (player->trickpanel == TRICKSTATE_READY)
		{
			const angle_t lr = ANGLE_45;
			fixed_t momz = FixedDiv(player->mo->momz, mapobjectscale);	// bring momz back to scale...
			fixed_t invertscale = FixedDiv(FRACUNIT, K_GrowShrinkSpeedMul(player));
			fixed_t speedmult = max(0, FRACUNIT - abs(momz)/TRICKMOMZRAMP);				// TRICKMOMZRAMP momz is minimum speed (Should be 20)
			fixed_t basespeed = FixedMul(invertscale, K_GetKartSpeed(player, false, false));	// at WORSE, keep your normal speed when tricking.
			fixed_t speed = FixedMul(invertscale, FixedMul(speedmult, P_AproxDistance(player->mo->momx, player->mo->momy)));

			if (P_MobjWasRemoved(player->trickIndicator) == false)
			{
				const fixed_t indicatormult = 3*(mapobjectscale/5);
				player->trickIndicator->destscale = FixedMul(speedmult + FRACUNIT, indicatormult);

				fixed_t trans = ((player->trickIndicator->scale * 9)/indicatormult) - 9;
				if (trans < 10) // it's fine if it stays barely visible imo
				{
					UINT32 renderflags = player->trickIndicator->renderflags & ~RF_TRANSMASK;
					if (trans > 0)
						renderflags |= (trans << RF_TRANSSHIFT);

					player->trickIndicator->renderflags = renderflags;
				}
			}

			// streaks:
			if (momz*P_MobjFlip(player->mo) > 0)	// only spawn those while you're going upwards relative to your current gravity
			{
				// these are all admittedly arbitrary numbers...
				INT32 n;
				INT32 maxlines = max(1, (momz/FRACUNIT)/16);
				INT32 frequency = max(1, 5-(momz/FRACUNIT)/4);
				fixed_t sx, sy, sz;
				mobj_t *spdl;

				if (!(leveltime % frequency))
				{
					for (n=0; n < maxlines; n++)
					{
						sx = player->mo->x + P_RandomRange(PR_DECORATION, -24, 24)*player->mo->scale;
						sy = player->mo->y + P_RandomRange(PR_DECORATION, -24, 24)*player->mo->scale;
						sz = player->mo->z + P_RandomRange(PR_DECORATION, 0, 48)*player->mo->scale;

						spdl = P_SpawnMobj(sx, sy, sz, MT_FASTLINE);
						P_SetTarget(&spdl->target, player->mo);
						spdl->angle = R_PointToAngle2(spdl->x, spdl->y, player->mo->x, player->mo->y);
						spdl->rollangle = -ANG1*90*P_MobjFlip(player->mo);		// angle them downwards relative to the player's gravity...
						spdl->spriteyscale = player->trickboostpower+FRACUNIT;
						spdl->momx = player->mo->momx;
						spdl->momy = player->mo->momy;
					}

				}

			}

			// We'll never need to go above that.
			if (player->tricktime <= TRICKDELAY)
			{
				// 2.3 - Prevent accidental fastfalls during trickdelay
				if (!G_CompatLevel(0x000C))
					player->pflags |= PF_NOFASTFALL;

				player->tricktime++;
			}

			// debug shit
			//CONS_Printf("%d\n", player->mo->momz / mapobjectscale);
			if (momz * P_MobjFlip(player->mo) < -10*FRACUNIT)	// :youfuckedup:
			{
				// tumble if you let your chance pass!!
				player->tumbleBounces = 1;
				player->pflags &= ~PF_TUMBLESOUND;
				player->tumbleHeight = 30;	// Base tumble bounce height
				player->trickpanel = TRICKSTATE_NONE;
				P_SetPlayerMobjState(player->mo, S_KART_SPINOUT);
				if (gametype != GT_TUTORIAL)
					K_AddMessageForPlayer(player, "Press <dpad> + <a> to trick!", true, false);
				if (player->itemflags & (IF_ITEMOUT|IF_EGGMANOUT))
				{
					//K_PopPlayerShield(player); // shield is just being yeeted, don't pop
					K_DropHnextList(player);
				}
			}

			else if (!(player->pflags & PF_TRICKDELAY))	// don't allow tricking at the same frame you tumble obv
			{
				// For tornado trick effects
				angle_t tornadotrickspeed = ANG30;
				const angle_t angledelta = FixedAngle(36*FRACUNIT);
				angle_t baseangle = player->mo->angle + angledelta/2;

				// Old ambiguous-input filter, no longer needed for 2.2 tricks
				// INT16 aimingcompare = abs(cmd->throwdir) - abs(cmd->turning);

				boolean cantrick = true;
				UINT16 buttons = player->cmd.buttons;
				INT16 TRICKTHRESHOLD = 2*KART_FULLTURN/3;

				// 2.3 - aimingcompare
				if (!!G_CompatLevel(0x000C))
				{
					TRICKTHRESHOLD = KART_FULLTURN/2;
					INT16 aimingcompare = abs(cmd->throwdir) - abs(cmd->turning);
					if (abs(aimingcompare) < TRICKTHRESHOLD)
						cantrick = false;
				}

				// 2.2 - Pre-steering trickpanels
				if (!G_CompatLevel(0x000A) && !K_PlayerUsesBotMovement(player))
				{
					if (!(buttons & BT_ACCELERATE))
					{
						cantrick = false;
					}
					// 2.3 - also allow tricking with the Spindash button
					else if (!G_CompatLevel(0x000C) && ((buttons & BT_SPINDASHMASK) == BT_SPINDASHMASK))
					{
						player->pflags |= PF_NOFASTFALL;
					}
				}

				// Uses cmd->turning over steering intentionally.
				if (cantrick && abs(cmd->turning) > TRICKTHRESHOLD) // side trick
				{
					S_StartSoundAtVolume(player->mo, sfx_trick0, 255/2);
					player->dotrickfx = true;

					// Calculate speed boost decay:
					// Base speed boost duration is 35 tics.
					// At most, lose 3/4th of your boost.
					player->trickboostdecay = min(TICRATE*3/4, abs(momz/FRACUNIT));

					if (cmd->turning > 0)
					{
						P_InstaThrust(player->mo, player->mo->angle + lr, max(basespeed, speed*5/2));
						player->trickpanel = TRICKSTATE_RIGHT;

						if (P_MobjWasRemoved(player->trickIndicator) == false)
						{
							player->trickIndicator->rollangle = ANGLE_270;
						}

						player->drawangle -= ANGLE_45;
						P_SetPlayerMobjState(player->mo, S_KART_FAST_LOOK_L);
					}
					else
					{
						P_InstaThrust(player->mo, player->mo->angle - lr, max(basespeed, speed*5/2));
						player->trickpanel = TRICKSTATE_LEFT;

						if (P_MobjWasRemoved(player->trickIndicator) == false)
						{
							player->trickIndicator->rollangle = ANGLE_90;
						}

						tornadotrickspeed = InvAngle(tornadotrickspeed);

						player->drawangle += ANGLE_45;
						P_SetPlayerMobjState(player->mo, S_KART_FAST_LOOK_R);
					}
				}
				else if (cantrick && abs(cmd->throwdir) > TRICKTHRESHOLD) // forward/back trick
				{
					S_StartSoundAtVolume(player->mo, sfx_trick0, 255/2);
					player->dotrickfx = true;

					// Calculate speed boost decay:
					// Base speed boost duration is 35 tics.
					// At most, lose 3/4th of your boost.
					player->trickboostdecay = min(TICRATE*3/4, abs(momz/FRACUNIT));

					if (cmd->throwdir > 0) // forward trick
					{

						if (player->mo->momz * P_MobjFlip(player->mo) > 0)
						{
							player->mo->momz = 0;
						}

						P_InstaThrust(player->mo, player->mo->angle, max(basespeed, speed*3));
						player->trickpanel = TRICKSTATE_FORWARD;

						if (P_MobjWasRemoved(player->trickIndicator) == false)
						{
							player->trickIndicator->rollangle = 0;
						}

						P_SetPlayerMobjState(player->mo, S_KART_FAST);
					}
					else if (cmd->throwdir < 0) // back trick
					{
						player->mo->momx /= 3;
						player->mo->momy /= 3;

						if (player->mo->momz * P_MobjFlip(player->mo) <= 0)
						{
							player->mo->momz = 0; // relative = false;
						}

						player->mo->momz += P_MobjFlip(player->mo)*48*mapobjectscale;
						player->trickpanel = TRICKSTATE_BACK;

						if (P_MobjWasRemoved(player->trickIndicator) == false)
						{
							player->trickIndicator->rollangle = ANGLE_180;
						}

						//tornadotrickspeed = InvAngle(tornadotrickspeed);

						//player->drawangle += ANGLE_45;
						P_SetPlayerMobjState(player->mo, S_KART_FAST);
					}
				}

				// Finalise everything.
				if (player->trickpanel != TRICKSTATE_READY) // just changed from 1?
				{
					player->mo->hitlag = TRICKLAG;
					player->mo->eflags &= ~MFE_DAMAGEHITLAG;

					if (abs(momz) < FRACUNIT*99)	// Let's use that as baseline for PERFECT trick.
					{
						player->karthud[khud_trickcool] = TICRATE;
					}

					INT32 j;
					skincolornum_t trickcolor = SKINCOLOR_NONE;

					if (P_MobjWasRemoved(player->trickIndicator) == false)
						trickcolor = player->trickIndicator->color;

					if (player->trickpanel == TRICKSTATE_FORWARD)
					{
						for (j = 0; j < 2; j++)
						{
							mobj_t *fwush = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_FORWARDTRICK);

							P_SetTarget(&fwush->target, player->mo);
							fwush->hitlag = TRICKLAG;
							fwush->color = trickcolor;
							fwush->renderflags |= RF_DONTDRAW;
							fwush->flags2 |= MF2_AMBUSH; // don't interp on first think
							fwush->threshold = 0;

							fwush->movedir = player->mo->angle;
							if (j == 0)
							{
								fwush->angle = fwush->old_angle = fwush->movedir + ANGLE_135;
								fwush->movefactor = 1;
							}
							else
							{
								fwush->angle = fwush->old_angle = fwush->movedir - ANGLE_135;
								fwush->movefactor = -1;
							}
						}
					}
					else for (j = 0; j < 8; j++, baseangle += angledelta)
					{
						mobj_t *swipe = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_SIDETRICK);

						if (player->trickpanel == TRICKSTATE_BACK)
							P_SetMobjState(swipe, S_BACKTRICK);

						P_SetTarget(&swipe->target, player->mo);
						swipe->hitlag = TRICKLAG;
						swipe->color = trickcolor;
						swipe->angle = baseangle + ANGLE_90;
						swipe->renderflags |= RF_DONTDRAW;
						swipe->flags2 |= MF2_AMBUSH; // don't interp on first think
						swipe->movedir = tornadotrickspeed;
						swipe->frame |= (j % 4);
						swipe->threshold = 0;

						// This is so they make a 10-sided shape with one-sprite gap
						if (j != 3)
							continue;

						baseangle += angledelta;
					}

					if (P_MobjWasRemoved(player->trickIndicator) == false)
					{
						K_TrickCatholocismBlast(player->trickIndicator, player->trickIndicator->scale*10, 0);

						player->trickIndicator->renderflags &= ~RF_TRANSMASK;

						P_InstaScale(player->trickIndicator, 3*mapobjectscale/2);
						player->trickIndicator->old_scale = player->trickIndicator->scale;

						P_SetMobjState(player->trickIndicator, S_TRICKINDICATOR_UNDERLAY_ARROW);
						if (P_MobjWasRemoved(player->trickIndicator->tracer) == false)
						{
							P_InstaScale(player->trickIndicator->tracer, player->trickIndicator->scale);
							player->trickIndicator->tracer->old_scale = player->trickIndicator->tracer->scale;

							P_SetMobjState(player->trickIndicator->tracer, S_TRICKINDICATOR_OVERLAY_ARROW);
						}
					}
				}
			}

			K_trickPanelTimingVisual(player, momz);
		}
		else if ((player->trickpanel != TRICKSTATE_NONE) && P_IsObjectOnGround(player->mo))	// Landed from trick
		{
			K_SpawnDashDustRelease(player);

			if (player->fastfall)
			{
				if (player->curshield != KSHIELD_BUBBLE) // Allow bubblebounce (it's cute) but don't give standard trick rewards
				{
					P_InstaThrust(player->mo, player->mo->angle, 2*abs(player->fastfall)/3 + 15*FRACUNIT);
					player->mo->hitlag = 3;
					S_StartSound(player->mo, sfx_gshba);
					player->fastfall = 0; // intentionally skip bounce
					player->trickcharge = 0;

					UINT8 i;
					for (i = 0; i < 4; i++)
					{
						mobj_t *arc = P_SpawnMobjFromMobj(player->mo, 0, 0, 0, MT_CHARGEFALL);
						P_SetTarget(&arc->target, player->mo);
						arc->extravalue1 = i;
					}
				}
			}
			else
			{
				S_StartSound(player->mo, sfx_s23c);

				UINT8 award = TICRATE - player->trickboostdecay;

				player->trickboost = award;
				if (!(gametyperules & GTR_SPHERES))
					K_AwardPlayerRings(player,
						(TICRATE-player->trickboostdecay) * player->lastairtime/3 / TICRATE, // Scale ring award by same amount as trickboost
					true);

				if (player->trickpanel == TRICKSTATE_FORWARD)
					player->trickboostpower /= 18;
				else if (player->trickpanel != TRICKSTATE_BACK)
					player->trickboostpower /= 9;
			}

			player->trickpanel = TRICKSTATE_NONE;
			player->trickboostdecay = 0;
		}

		// 2.2 - Lenient trickpanels
		if (G_CompatLevel(0x000A) || K_PlayerUsesBotMovement(player))
		{
			// Wait until we let go off the control stick to remove the delay
			// buttons must be neutral after the initial trick delay. This prevents weirdness where slight nudges after blast off would send you flying.
			if ((player->pflags & PF_TRICKDELAY) && !player->throwdir && !cmd->turning && (player->tricktime >= TRICKDELAY))
			{
				player->pflags &= ~PF_TRICKDELAY;
			}
		}
		else
		{
			if (G_CompatLevel(0x000C))
			{
				if ((player->pflags & PF_TRICKDELAY) && !(player->cmd.buttons & BT_ACCELERATE) && (player->tricktime >= TRICKDELAY))
				{
					player->pflags &= ~PF_TRICKDELAY;
				}
			}
			else
			// 2.3 - Spindash to trick
			{
				// Ignore pre-existing Accel inputs if not pressing Spindash. Always ignore pre-existing Spindash inputs to prevent accidental tricking.
				if ((player->pflags & PF_TRICKDELAY) && (!(player->cmd.buttons & BT_ACCELERATE) || (((player->cmd.buttons & BT_SPINDASHMASK) == BT_SPINDASHMASK) && (player->oldcmd.buttons & BT_SPINDASHMASK) != BT_SPINDASHMASK)) && (player->tricktime >= TRICKDELAY))
				{
					player->pflags &= ~PF_TRICKDELAY;
					player->pflags |= PF_NOFASTFALL;
				}
			}
		}

	}

	K_KartDrift(player, onground);
	K_KartSpindash(player);

	if (onground == false
	&& !player->bungee		// if this list of condition ever gets bigger, maybe this should become a function.
	)
	{
		K_AirFailsafe(player);
	}
	else
	{
		player->pflags &= ~PF_AIRFAILSAFE;
	}

	Obj_RingShooterInput(player);

	if (player->bungee)
		Obj_playerBungeeThink(player);

	if (player->dlzrocket)
		Obj_playerDLZRocket(player);

	if (player->seasawcooldown && !player->seasaw)
		player->seasawcooldown--;

	if (player->turbine)
	{
		if (player->mo->tracer && !P_MobjWasRemoved(player->mo->tracer))
			Obj_playerWPZTurbine(player);
		else
			player->turbine--;	// acts as a cooldown
	}

	if (player->icecube.frozen)
	{
		Obj_IceCubeInput(player);
	}

	Obj_PlayerCloudThink(player);

	Obj_PlayerBulbThink(player);

}

void K_CheckSpectateStatus(boolean considermapreset)
{
	UINT8 respawnlist[MAXPLAYERS];
	UINT8 i, j, numingame = 0, numjoiners = 0;
	UINT8 numhumans = 0, numbots = 0;

	// Maintain spectate wait timer
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
		{
			continue;
		}

		if (!players[i].spectator)
		{
			numingame++;

			if (players[i].bot)
			{
				numbots++;
			}
			else
			{
				numhumans++;
			}

			players[i].spectatewait = 0;
			players[i].spectatorReentry = 0;
			continue;
		}

		if ((players[i].pflags & PF_WANTSTOJOIN))
		{
			players[i].spectatewait++;
		}
		else
		{
			players[i].spectatewait = 0;
		}

		if (gamestate != GS_LEVEL || considermapreset == false)
		{
			players[i].spectatorReentry = 0;
		}
		else if (players[i].spectatorReentry > 0)
		{
			players[i].spectatorReentry--;
		}
	}

	// No one's allowed to join
	if (!cv_allowteamchange.value)
		return;

	// DON'T allow if you've hit the in-game player cap
	if (cv_maxplayers.value && numhumans >= cv_maxplayers.value)
		return;

	// Get the number of players in game, and the players to be de-spectated.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (!players[i].spectator)
		{
			// Allow if you're not in a level
			if (gamestate != GS_LEVEL)
				continue;

			// DON'T allow if anyone's exiting
			if (players[i].exiting)
				return;

			// Allow if the match hasn't started yet
			if (numingame < 2 || leveltime < starttime || mapreset)
				continue;

			// DON'T allow if the match is 20 seconds in
			if (leveltime > (starttime + 20*TICRATE))
				return;

			// DON'T allow if the race is at 2 laps
			if ((gametyperules & GTR_CIRCUIT) && players[i].laps >= 2)
				return;

			continue;
		}

		if (players[i].bot)
		{
			// Spectating bots are controlled by other mechanisms.
			continue;
		}

		if (!(players[i].pflags & PF_WANTSTOJOIN))
		{
			// This spectator does not want to join.
			continue;
		}

		if (netgame && numingame > 0 && players[i].spectatorReentry > 0)
		{
			// This person has their reentry cooldown active.
			continue;
		}

		respawnlist[numjoiners++] = i;
	}

	// Literally zero point in going any further if nobody is joining.
	if (!numjoiners)
		return;

	// Organize by spectate wait timer (if there's more than one to sort)
	if (cv_maxplayers.value && numjoiners > 1)
	{
		UINT8 oldrespawnlist[MAXPLAYERS];
		memcpy(oldrespawnlist, respawnlist, numjoiners);
		for (i = 0; i < numjoiners; i++)
		{
			UINT8 pos = 0;
			INT32 ispecwait = players[oldrespawnlist[i]].spectatewait;

			for (j = 0; j < numjoiners; j++)
			{
				INT32 jspecwait = players[oldrespawnlist[j]].spectatewait;
				if (j == i)
					continue;
				if (jspecwait > ispecwait)
					pos++;
				else if (jspecwait == ispecwait && j < i)
					pos++;
			}

			respawnlist[pos] = oldrespawnlist[i];
		}
	}

	const UINT8 previngame = numingame;
	INT16 removeBotID = MAXPLAYERS - 1;

	// Finally, we can de-spectate everyone!
	for (i = 0; i < numjoiners; i++)
	{
		// Hit the in-game player cap while adding people?
		if (cv_maxplayers.value && numingame >= cv_maxplayers.value)
		{
			if (numbots > 0)
			{
				// Find a bot to kill to make room
				while (removeBotID >= 0)
				{
					if (playeringame[removeBotID] && players[removeBotID].bot)
					{
						//CONS_Printf("bot %s kicked to make room on tic %d\n", player_names[removeBotID], leveltime);
						CL_RemovePlayer(removeBotID, KR_LEAVE);
						numbots--;
						numingame--;
						break;
					}

					removeBotID--;
				}

				if (removeBotID < 0)
				{
					break;
				}
			}
			else
			{
				break;
			}
		}

		//CONS_Printf("player %s is joining on tic %d\n", player_names[respawnlist[i]], leveltime);
		P_SpectatorJoinGame(&players[respawnlist[i]]);
		numhumans++;
		numingame++;
	}

	if (considermapreset == false)
		return;

	// Reset the match when 2P joins 1P, DUEL mode
	// Reset the match when 3P joins 1P and 2P, DUEL mode must be disabled
	extern consvar_t cv_debugnewchallenger;
	if (i > 0 && !mapreset && gamestate == GS_LEVEL && (previngame < 3 && numingame >= 2) && !cv_debugnewchallenger.value)
	{
		Music_Play("comeon"); // COME ON
		mapreset = 3*TICRATE; // Even though only the server uses this for game logic, set for everyone for HUD
	}
}

UINT8 K_GetInvincibilityItemFrame(void)
{
	return ((leveltime % (7*3)) / 3);
}

UINT8 K_GetOrbinautItemFrame(UINT8 count)
{
	return min(count - 1, 3);
}

boolean K_IsSPBInGame(void)
{
	// is there an SPB chasing anyone?
	if (spbplace != -1)
		return true;

	// do any players have an SPB in their item slot?
	UINT8 i;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		if (players[i].itemtype == KITEM_SPB)
			return true;
	}


	// spbplace is still -1 until a fired SPB finds a target, so look for an in-map SPB just in case
	mobj_t *mobj;
	for (mobj = trackercap; mobj; mobj = mobj->itnext)
	{
		if (mobj->type != MT_SPB)
			continue;

		return true;
	}

	return false;
}

void K_HandleDirectionalInfluence(player_t *player)
{
	fixed_t strength = FRACUNIT >> 1; // 1.0 == 45 degrees

	ticcmd_t *cmd = NULL;
	angle_t sideAngle = ANGLE_MAX;

	INT16 inputX, inputY;
	INT16 inputLen;

	fixed_t diX, diY;
	fixed_t diLen;

	fixed_t dot, invDot;

	fixed_t finalX, finalY;
	fixed_t finalLen;
	fixed_t speed;

	if (player->playerstate != PST_LIVE || player->spectator)
	{
		// ded
		return;
	}

	// DI attempted!!
	player->justDI = MAXHITLAGTICS;

	cmd = &player->cmd;

	inputX = cmd->throwdir;
	inputY = -cmd->turning;

	if (player->flipDI == true)
	{
		// Bananas flip the DI direction.
		// Otherwise, DIing bananas is a little brain-dead easy :p
		inputX = -inputX;
		inputY = -inputY;
	}

	if (inputX == 0 && inputY == 0)
	{
		// No DI input, no need to do anything else.
		return;
	}

	inputLen = FixedHypot(inputX, inputY);
	if (inputLen > KART_FULLTURN)
	{
		inputLen = KART_FULLTURN;
	}

	// Shallow inputs = less angle change.
	strength = FixedMul(strength, (inputLen * FRACUNIT) / KART_FULLTURN);

	if (player->tumbleBounces > 0)
	{
		// Very strong DI for tumble.
		strength *= 3;
	}

	sideAngle = player->mo->angle - ANGLE_90;

	diX = FixedMul(inputX, FINECOSINE(player->mo->angle >> ANGLETOFINESHIFT)) + FixedMul(inputY, FINECOSINE(sideAngle >> ANGLETOFINESHIFT));
	diY = FixedMul(inputX, FINESINE(player->mo->angle >> ANGLETOFINESHIFT)) + FixedMul(inputY, FINESINE(sideAngle >> ANGLETOFINESHIFT));
	diLen = FixedHypot(diX, diY);

	// Normalize
	if (diLen > 0)
	{
		diX = FixedDiv(diX, diLen);
		diY = FixedDiv(diY, diLen);
	}

	// Now that we got the DI direction, we can
	// actually preform the velocity redirection.

	speed = FixedHypot(player->mo->momx, player->mo->momy);
	finalX = FixedDiv(player->mo->momx, speed);
	finalY = FixedDiv(player->mo->momy, speed);

	dot = FixedMul(diX, finalX) + FixedMul(diY, finalY);
	invDot = FRACUNIT - abs(dot);

	finalX += FixedMul(FixedMul(diX, invDot), strength);
	finalY += FixedMul(FixedMul(diY, invDot), strength);
	finalLen = FixedHypot(finalX, finalY);

	if (finalLen > 0)
	{
		finalX = FixedDiv(finalX, finalLen);
		finalY = FixedDiv(finalY, finalLen);
	}

	player->mo->momx = FixedMul(speed, finalX);
	player->mo->momy = FixedMul(speed, finalY);
}

void K_UpdateMobjItemOverlay(mobj_t *part, SINT8 itemType, UINT8 itemCount)
{
	switch (itemType)
	{
		case KITEM_ORBINAUT:
			part->sprite = SPR_ITMO;
			part->frame = FF_FULLBRIGHT|FF_PAPERSPRITE|K_GetOrbinautItemFrame(itemCount);
			break;
		case KITEM_INVINCIBILITY:
			part->sprite = SPR_ITMI;
			part->frame = FF_FULLBRIGHT|FF_PAPERSPRITE|K_GetInvincibilityItemFrame();
			break;
		case KITEM_SAD:
			part->sprite = SPR_ITEM;
			part->frame = FF_FULLBRIGHT|FF_PAPERSPRITE;
			break;
		default:
			if (itemType >= FIRSTPOWERUP)
			{
				part->sprite = SPR_PWRB;
				// Not a papersprite. See K_CreatePaperItem for why.
				part->frame = FF_FULLBRIGHT|(itemType - FIRSTPOWERUP);
			}
			else
			{
				part->sprite = SPR_ITEM;
				part->frame = FF_FULLBRIGHT|FF_PAPERSPRITE|(itemType);
			}
			break;
	}
}

void K_EggmanTransfer(player_t *source, player_t *victim)
{
	if (victim->eggmanTransferDelay)
		return;
	if (source->eggmanTransferDelay)
		return;
	if (victim->eggmanexplode)
		return;

	K_AddHitLag(victim->mo, 5, false);
	K_DropItems(victim);
	victim->eggmanexplode = 6*TICRATE;
	victim->eggmanblame = (source - players);
	K_StopRoulette(&victim->itemRoulette);

	if (P_IsDisplayPlayer(victim))
		S_StartSound(NULL, sfx_itrole);

	K_AddHitLag(source->mo, 5, false);
	source->eggmanexplode = 0;
	source->eggmanblame = -1;
	K_StopRoulette(&source->itemRoulette);

	source->eggmanTransferDelay = 25;
	victim->eggmanTransferDelay = 15;

	S_StopSoundByID(source->mo, sfx_s3k53);
	S_StopSoundByID(source->mo, sfx_kc51);
}

tic_t K_TimeLimitForGametype(void)
{
	const tic_t gametypeDefault = gametypes[gametype]->timelimit * (60*TICRATE);

	if (!(gametyperules & GTR_TIMELIMIT))
	{
		return 0;
	}

	if (modeattacking)
	{
		return 0;
	}

	// Grand Prix
	if (!K_CanChangeRules(true))
	{
		if (battleprisons)
		{
			if (grandprixinfo.gp)
			{
				if (grandprixinfo.gamespeed == KARTSPEED_EASY)
					return 30*TICRATE;
			}
			return 20*TICRATE;
		}

		return gametypeDefault;
	}

	if (cv_timelimit.value != -1)
	{
		return cv_timelimit.value * TICRATE;
	}

	// No time limit for Break the Capsules FREE PLAY
	if (battleprisons)
	{
		return 0;
	}

	return gametypeDefault;
}

UINT32 K_PointLimitForGametype(void)
{
	const UINT32 gametypeDefault = gametypes[gametype]->pointlimit;
	const UINT32 battleRules = GTR_BUMPERS|GTR_CLOSERPLAYERS|GTR_PAPERITEMS;

	UINT32 ptsCap = gametypeDefault;

	if (!(gametyperules & GTR_POINTLIMIT))
	{
		return 0;
	}

	if (K_Cooperative())
	{
		return 0;
	}

	if (K_CanChangeRules(true) == true && cv_pointlimit.value != -1)
	{
		return cv_pointlimit.value;
	}

	if ((gametyperules & battleRules) == battleRules) // why isn't this just another GTR_??
	{
		INT32 i;

		// It's frustrating that this shitty for-loop needs to
		// be duplicated every time the players need to be
		// counted.
		for (i = 0; i < MAXPLAYERS; ++i)
		{
			if (D_IsPlayerHumanAndGaming(i))
			{
				ptsCap += 3;
			}
		}

		if (ptsCap > 16)
		{
			ptsCap = 16;
		}
	}

	return ptsCap;
}

boolean K_Cooperative(void)
{
	if (battleprisons)
	{
		return true;
	}

	if (bossinfo.valid)
	{
		return true;
	}

	if (specialstageinfo.valid)
	{
		return true;
	}

	if (gametype == GT_TUTORIAL)
	{
		// Maybe this should be a rule. Eventually?
		return true;
	}

	return false;
}

void K_SetTireGrease(player_t *player, tic_t tics)
{
	if (!player->tiregrease)
	{
		UINT8 i;
		for (i = 0; i < 2; i++)
		{
			mobj_t *grease;
			grease = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_TIREGREASE);
			P_SetTarget(&grease->target, player->mo);
			grease->angle = K_MomentumAngle(player->mo);
			grease->extravalue1 = i;
			P_SetTarget(&grease->owner, player->mo);
			grease->renderflags |= RF_REDUCEVFX;
		}
	}

	player->tiregrease = tics;
}

// somewhat sensible check for HUD sounds in a post-bot-takeover world
boolean K_IsPlayingDisplayPlayer(player_t *player)
{
	return P_IsDisplayPlayer(player) && (!player->exiting);
}

boolean K_PlayerCanPunt(player_t *player)
{
	if (player->trickpanel > TRICKSTATE_READY)
	{
		return true;
	}

	if (player->invincibilitytimer > 0)
	{
		return true;
	}

	if (player->flamedash > 0 && player->itemtype == KITEM_FLAMESHIELD)
	{
		return true;
	}

	if (player->growshrinktimer > 0)
	{
		return true;
	}

	if (player->tripwirePass >= TRIPWIRE_BLASTER && player->speed >= K_PlayerTripwireSpeedThreshold(player))
	{
		return true;
	}

	return false;
}

void K_MakeObjectReappear(mobj_t *mo)
{
	(!P_MobjWasRemoved(mo->punt_ref) ? mo->punt_ref : mo)->reappear = leveltime + (30*TICRATE);
}

boolean K_PlayerCanUseItem(player_t *player)
{
	return (player->mo->health > 0 && !player->spectator && !P_PlayerInPain(player) && !mapreset && leveltime > introtime);
}

//}
