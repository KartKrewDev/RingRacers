// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_bot.cpp
/// \brief Bot logic & ticcmd generation code

#include <algorithm>

#include <tracy/tracy/Tracy.hpp>

#include "cxxutil.hpp"

#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "r_main.h"
#include "p_local.h"
#include "k_bot.h"
#include "lua_hook.h"
#include "byteptr.h"
#include "d_net.h" // nodetoplayer
#include "k_kart.h"
#include "z_zone.h"
#include "i_system.h"
#include "p_maputl.h"
#include "d_ticcmd.h"
#include "m_random.h"
#include "r_things.h" // numskins
#include "k_race.h" // finishBeamLine
#include "m_perfstats.h"
#include "k_podium.h"
#include "k_respawn.h"
#include "m_easing.h"
#include "d_clisrv.h"
#include "g_party.h"
#include "k_grandprix.h" // K_CanChangeRules
#include "hu_stuff.h" // HU_AddChatText
#ifdef HAVE_DISCORDRPC
#include "discord.h" // DRPC_UpdatePresence
#endif
#include "i_net.h" // doomcom

extern "C" consvar_t cv_forcebots;

/*--------------------------------------------------
	void K_SetNameForBot(UINT8 playerNum, const char *realname)

		See header file for description.
--------------------------------------------------*/
void K_SetNameForBot(UINT8 newplayernum, const char *realname)
{
	// These names are generally sourced from skins.
	I_Assert(MAXPLAYERNAME >= SKINNAMESIZE+2);

	boolean canApplyNameChange = true;
	if (netgame == true)
	{
		canApplyNameChange = IsPlayerNameUnique(realname, newplayernum);
	}

	if (canApplyNameChange)
	{
		// No conflict detected!
		sprintf(player_names[newplayernum], "%s", realname);
		return;
	}

	// Ok, now we append on the end for duplicates...
	char namebuffer[MAXPLAYERNAME+1];
	sprintf(namebuffer, "%s %c", realname, 'A'+newplayernum);

	// ...and use the actual function, to handle more devious duplication.
	if (!EnsurePlayerNameIsGood(namebuffer, newplayernum))
	{
		// we can't bail from adding the bot...
		// this hopefully uncontroversial pick is all we CAN do
		sprintf(namebuffer, "Bot %u", newplayernum+1);
	}

	// And finally write.
	sprintf(player_names[newplayernum], "%s", namebuffer);
}

/*--------------------------------------------------
	void K_SetBot(UINT8 playerNum, UINT16 skinnum, UINT8 difficulty, botStyle_e style)

		See header file for description.
--------------------------------------------------*/
void K_SetBot(UINT8 newplayernum, UINT16 skinnum, UINT8 difficulty, botStyle_e style)
{
	CONS_Debug(DBG_NETPLAY, "addbot: %d\n", newplayernum);

	G_AddPlayer(newplayernum, newplayernum);

	if (newplayernum+1 > doomcom->numslots)
		doomcom->numslots = (INT16)(newplayernum+1);

	playernode[newplayernum] = servernode;

	// this will permit unlocks
	memcpy(&players[newplayernum].availabilities, R_GetSkinAvailabilities(false, skinnum), MAXAVAILABILITY*sizeof(UINT8));

	players[newplayernum].splitscreenindex = 0;
	players[newplayernum].bot = true;
	players[newplayernum].botvars.difficulty = difficulty;
	players[newplayernum].botvars.style = style;
	players[newplayernum].lives = 9;

	if (cv_levelskull.value)
		players[newplayernum].botvars.difficulty = MAXBOTDIFFICULTY;

	// The bot may immediately become a spectator AT THE START of a GP.
	// For each subsequent round of GP, K_UpdateGrandPrixBots will handle this.
	players[newplayernum].spectator = grandprixinfo.gp && grandprixinfo.initalize && K_BotDefaultSpectator();

	skincolornum_t color = static_cast<skincolornum_t>(skins[skinnum]->prefcolor);
	const char *realname = skins[skinnum]->realname;
	if (tutorialchallenge == TUTORIALSKIP_INPROGRESS)
	{
		// The ROYGBIV Rangers
		switch (newplayernum)
		{
			case 1:
				color = SKINCOLOR_RED;
				realname = "Champ";
				break;
			case 2:
				color = SKINCOLOR_ORANGE;
				realname = "Pharaoh";
				break;
			case 3:
				color = SKINCOLOR_YELLOW;
				realname = "Caesar";
				break;
			case 4:
				color = SKINCOLOR_GREEN;
				realname = "General";
				break;
			case 5:
				color = SKINCOLOR_CYAN; // blue (lighter than _BLUE)
				realname = "Shogun";
				break;
			case 6:
				color = SKINCOLOR_BLUEBERRY; // indigo
				realname = "Emperor";
				break;
			case 7:
				color = SKINCOLOR_VIOLET;
				realname = "King";
				break;
			default:
				color = SKINCOLOR_BLACK;
				realname = "Vizier"; // working in the shadows
				break;
		}
	}

	K_SetNameForBot(newplayernum, realname);

	LUA_HookPlayer(&players[newplayernum], HOOK(BotJoin));

	for (UINT8 i = 0; i < PWRLV_NUMTYPES; i++)
	{
		clientpowerlevels[newplayernum][i] = 0;
	}

	players[newplayernum].prefcolor = color;
	players[newplayernum].prefskin = skinnum;
	players[newplayernum].preffollower = -1;
	players[newplayernum].preffollowercolor = SKINCOLOR_NONE;
	G_UpdatePlayerPreferences(&players[newplayernum]);

	if (netgame)
	{
		HU_AddChatText(va("\x82*Bot %d has been added to the game", newplayernum+1), false);
	}

	LUA_HookInt(newplayernum, HOOK(PlayerJoin));
}

/*--------------------------------------------------
	boolean K_AddBot(UINT16 skin, UINT8 difficulty, botStyle_e style, UINT8 *p)

		See header file for description.
--------------------------------------------------*/
boolean K_AddBot(UINT16 skin, UINT8 difficulty, botStyle_e style, UINT8 *p)
{
	UINT8 newplayernum = *p;

	for (; newplayernum < MAXPLAYERS; newplayernum++)
	{
		if (playeringame[newplayernum] == false)
		{
			// free player slot
			break;
		}
	}

	if (newplayernum >= MAXPLAYERS)
	{
		// nothing is free
		*p = MAXPLAYERS;
		return false;
	}

	K_SetBot(newplayernum, skin, difficulty, style);
	DEBFILE(va("Everyone added bot %d\n", newplayernum));

	// use the next free slot
	*p = newplayernum+1;

	return true;
}

/*--------------------------------------------------
	void K_UpdateMatchRaceBots(void)

		See header file for description.
--------------------------------------------------*/
void K_UpdateMatchRaceBots(void)
{
	const UINT16 defaultbotskin = R_BotDefaultSkin();
	UINT8 difficulty;
	UINT8 pmax = (InADedicatedServer() ? MAXPLAYERS-1 : MAXPLAYERS);
	UINT8 numplayers = 0;
	UINT8 numbots = 0;
	UINT8 numwaiting = 0;
	SINT8 wantedbots = 0;
	UINT16 usableskins = 0, skincount = (demo.playback ? demo.numskins : numskins);;
	UINT16 grabskins[MAXSKINS+1];
	UINT16 i;

	// Init usable bot skins list
	for (i = 0; i < skincount; i++)
	{
		grabskins[usableskins++] = i;
	}
	grabskins[usableskins] = MAXSKINS;

	if (gamestate == GS_TITLESCREEN)
	{
		difficulty = 0;
	}
	else if ((gametyperules & GTR_BOTS) == 0 && !cv_forcebots.value)
	{
		difficulty = 0;
	}
	else if (tutorialchallenge == TUTORIALSKIP_INPROGRESS)
	{
		pmax = 8; // can you believe this is a nerf
		difficulty = 4;
	}
	else if (K_CanChangeRules(true) == false)
	{
		difficulty = 0;
	}
	else
	{
		difficulty = cv_kartbot.value;
		if (netgame)
		{
			pmax = std::min<UINT8>(pmax, static_cast<UINT8>(cv_maxconnections.value));
		}
		if (cv_maxplayers.value > 0)
		{
			pmax = std::min<UINT8>(pmax, static_cast<UINT8>(cv_maxplayers.value));
		}
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			if (!players[i].spectator)
			{
				grabskins[players[i].skin] = MAXSKINS;

				if (players[i].bot)
				{
					numbots++;

					// While we're here, we should update bot difficulty to the proper value.
					players[i].botvars.difficulty = difficulty;

					// Enforce normal style for Match Race
					players[i].botvars.style = BOT_STYLE_NORMAL;
				}
				else
				{
					numplayers++;
				}
			}
			else if (players[i].pflags & PF_WANTSTOJOIN)
			{
				numwaiting++;
			}
		}
	}

	if (difficulty == 0)
	{
		// Remove bots if there are any.
		wantedbots = 0;
	}
	else
	{
		// Add bots to fill up MAXPLAYERS
		wantedbots = pmax - numplayers - numwaiting;

		if (wantedbots < 0)
		{
			wantedbots = 0;
		}
	}

	auto clear_bots = [&numbots](UINT8 max)
	{
		UINT8 i = MAXPLAYERS;
		while (numbots > max && i > 0)
		{
			i--;

			if (playeringame[i] && players[i].bot)
			{
				CL_RemovePlayer(i, KR_LEAVE);
				numbots--;
			}
		}
	};

	if (tutorialchallenge == TUTORIALSKIP_INPROGRESS)
	{
		// Prevent Eggman bot carrying over from Tutorial
		clear_bots(0);
	}

	if (numbots < wantedbots)
	{
		// We require MORE bots!
		UINT8 newplayernum = InADedicatedServer() ? 1 : 0;

		// Rearrange usable bot skins list to prevent gaps for randomised selection
		if (tutorialchallenge == TUTORIALSKIP_INPROGRESS)
		{
			usableskins = 0; // force a crack team of Eggrobo
		}
		else for (i = 0; i < usableskins; i++)
		{
			if (!(grabskins[i] == MAXSKINS || !R_SkinUsable(-1, grabskins[i], true)))
			{
				continue;
			}

			while (usableskins > i && (grabskins[usableskins] == MAXSKINS || !R_SkinUsable(-1, grabskins[usableskins], true)))
			{
				usableskins--;
			}

			grabskins[i] = grabskins[usableskins];
			grabskins[usableskins] = MAXSKINS;
		}

		while (numbots < wantedbots)
		{
			UINT16 skinnum = defaultbotskin;

			if (usableskins > 0)
			{
				UINT16 index = P_RandomKey(PR_BOTS, usableskins);
				skinnum = grabskins[index];
				grabskins[index] = grabskins[--usableskins];
			}

			if (!K_AddBot(skinnum, difficulty, BOT_STYLE_NORMAL, &newplayernum))
			{
				// Not enough player slots to add the bot, break the loop.
				break;
			}

			numbots++;
		}
	}
	else if (numbots > wantedbots)
	{
		clear_bots(wantedbots);
	}

	K_AssignFoes();

	// We should have enough bots now :)

#ifdef HAVE_DISCORDRPC
	// Player count change was possible, so update presence
	DRPC_UpdatePresence();
#endif
}

/*--------------------------------------------------
	boolean K_PlayerUsesBotMovement(const player_t *player)

		See header file for description.
--------------------------------------------------*/
boolean K_PlayerUsesBotMovement(const player_t *player)
{
	if (K_PodiumSequence() == true)
		return true;

	// Lua can't override the podium sequence result, but it can
	// override the following results:
	{
		UINT8 shouldOverride = LUA_HookPlayerForceResults(const_cast<player_t*>(player),
			HOOK(PlayerUsesBotMovement));
		if (shouldOverride == 1)
			return true;
		if (shouldOverride == 2)
			return false;
	}


	if (player->exiting)
		return true;

	if (player->bot)
		return true;

#ifdef DEVELOP
	if (cv_takeover.value)
		return true;
#endif

	return false;
}

/*--------------------------------------------------
	boolean K_BotCanTakeCut(player_t *player)

		See header file for description.
--------------------------------------------------*/
boolean K_BotCanTakeCut(const player_t *player)
{
	if (
#if 1
		K_TripwirePassConditions(player) != TRIPWIRE_NONE
#else
		K_ApplyOffroad(player) == false
#endif
		|| player->itemtype == KITEM_SNEAKER
		|| player->itemtype == KITEM_ROCKETSNEAKER
		|| player->itemtype == KITEM_INVINCIBILITY
		)
	{
		return true;
	}

	return false;
}

/*--------------------------------------------------
	static fixed_t K_BotSpeedScaled(const player_t *player, fixed_t speed)

		What the bot "thinks" their speed is, for predictions.
		Mainly to make bots brake earlier when on friction sectors.

	Input Arguments:-
		player - The bot player to calculate speed for.
		speed - Raw speed value.

	Return:-
		The bot's speed value for calculations.
--------------------------------------------------*/
static fixed_t K_BotSpeedScaled(const player_t *player, fixed_t speed)
{
	fixed_t result = speed;

	if (P_IsObjectOnGround(player->mo) == false)
	{
		// You have no air control, so don't predict too far ahead.
		return 0;
	}

	if (player->mo->movefactor != FRACUNIT)
	{
		fixed_t moveFactor = player->mo->movefactor;

		if (moveFactor == 0)
		{
			moveFactor = 1;
		}

		// Reverse against friction. Allows for bots to
		// acknowledge they'll be moving faster on ice,
		// and to steer harder / brake earlier.
		moveFactor = FixedDiv(FRACUNIT, moveFactor);

		// The full value is way too strong, reduce it.
		moveFactor -= (moveFactor - FRACUNIT)*3/4;

		result = FixedMul(result, moveFactor);
	}

	if (player->mo->standingslope != nullptr)
	{
		const pslope_t *slope = player->mo->standingslope;

		if (!(slope->flags & SL_NOPHYSICS) && abs(slope->zdelta) >= FRACUNIT/21)
		{
			fixed_t slopeMul = FRACUNIT;
			angle_t angle = K_MomentumAngle(player->mo) - slope->xydirection;

			if (P_MobjFlip(player->mo) * slope->zdelta < 0)
				angle ^= ANGLE_180;

			// Going uphill: 0
			// Going downhill: FRACUNIT*2
			slopeMul = FRACUNIT + FINECOSINE(angle >> ANGLETOFINESHIFT);

			// Range: 0.5 to 1.5
			result = FixedMul(result, (FRACUNIT>>1) + (slopeMul >> 1));
		}
	}

	return result;
}

/*--------------------------------------------------
	botcontroller_t *K_GetBotController(const mobj_t *mobj)

		See header file for description.
--------------------------------------------------*/
botcontroller_t *K_GetBotController(const mobj_t *mobj)
{
	botcontroller_t *ret = nullptr;

	if (P_MobjWasRemoved(mobj) == true)
	{
		return nullptr;
	}

	if (mobj->subsector == nullptr || mobj->subsector->sector == nullptr)
	{
		return nullptr;
	}

	ret = &mobj->subsector->sector->botController;

	ffloor_t *rover = nullptr;
	for (rover = mobj->subsector->sector->ffloors; rover; rover = rover->next)
	{
		if ((rover->fofflags & FOF_EXISTS) == 0)
		{
			continue;
		}

		fixed_t topheight = P_GetFOFTopZ(mobj, mobj->subsector->sector, rover, mobj->x, mobj->y, nullptr);
		fixed_t bottomheight = P_GetFOFBottomZ(mobj, mobj->subsector->sector, rover, mobj->x, mobj->y, nullptr);

		if (mobj->z > topheight || mobj->z + mobj->height < bottomheight)
		{
			continue;
		}

		botcontroller_t *roverController = &rover->master->frontsector->botController;
		if (roverController->trick != 0 || roverController->flags != 0)
		{
			ret = roverController;
		}
	}

	return ret;
}

/*--------------------------------------------------
	fixed_t K_BotMapModifier(void)

		See header file for description.
--------------------------------------------------*/
fixed_t K_BotMapModifier(void)
{
	// fuck it we ball
	return 5*FRACUNIT/10;

	constexpr INT32 complexity_scale = 10000;
	fixed_t modifier_max = (10 * FRACUNIT / 10) - FRACUNIT;
	fixed_t modifier_min = (5 * FRACUNIT / 10) - FRACUNIT;

	const fixed_t complexity_value = std::clamp<fixed_t>(
		FixedDiv(K_GetTrackComplexity(), complexity_scale),
		modifier_min,
		modifier_max
	);

	return FRACUNIT + complexity_value;
}

/*--------------------------------------------------
	static UINT32 K_BotRubberbandDistance(const player_t *player)

		Calculates the distance away from 1st place that the
		bot should rubberband to.

	Input Arguments:-
		player - Player to compare.

	Return:-
		Distance to add, as an integer.
--------------------------------------------------*/
static UINT32 K_BotRubberbandDistance(const player_t *player)
{
	UINT32 spacing = FixedDiv(640 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed)) / FRACUNIT;
	const UINT8 portpriority = player - players;
	UINT8 pos = 1;
	UINT8 i;

	if (player->botvars.rival || cv_levelskull.value)
	{
		// The rival should always try to be the front runner for the race.
		return 0;
	}

	/*
	if (player->botvars.foe)
		spacing /= 2;
	*/

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (i == portpriority)
		{
			continue;
		}

		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		if (!players[i].bot)
		{
			continue;
		}

		if (G_SameTeam(player, &players[i]) == true)
		{
			// Don't consider friendlies with your rubberbanding.
			continue;
		}

		if (player->botvars.foe && !players[i].botvars.foe)
		{
			continue;
		}

		// First check difficulty levels, then score, then settle it with port priority!
		if (player->botvars.difficulty < players[i].botvars.difficulty)
		{
			pos += 3;
		}
		else if (player->score < players[i].score)
		{
			pos += 2;
		}
		else if (i < portpriority)
		{
			pos += 1;
		}
	}

	return (pos * spacing);
}

/*--------------------------------------------------
	fixed_t K_BotRubberband(const player_t *player)

		See header file for description.
--------------------------------------------------*/
fixed_t K_BotRubberband(const player_t *player)
{
	if (player->exiting)
	{
		// You're done, we don't need to rubberband anymore.
		return FRACUNIT;
	}

	const botcontroller_t *botController = K_GetBotController(player->mo);
	if (botController != nullptr && (botController->flags & TMBOT_NORUBBERBAND) == TMBOT_NORUBBERBAND) // Disable rubberbanding
	{
		return FRACUNIT;
	}

	fixed_t expreduce = 0;

	// Allow the status quo to assert itself a bit. Bots get most of their speed from their
	// mechanics adjustments, not from items, so kill some bot speed if they've got bad EXP.
	if (player->gradingfactor < FRACUNIT && !(player->botvars.rival) && player->botvars.difficulty > 1)
	{
		UINT8 levelreduce = std::min<UINT8>(3, player->botvars.difficulty/4); // How much to drop the "effective level" of bots that are consistently behind
		expreduce = Easing_Linear((K_EffectiveGradingFactor(player) - MINGRADINGFACTOR) * 2, levelreduce*FRACUNIT, 0);
		if (player->botvars.foe)
			expreduce /= 2;
	}

	fixed_t difficultyEase = (((player->botvars.difficulty - 1) * FRACUNIT) - expreduce) / (MAXBOTDIFFICULTY - 1);

	if (difficultyEase < 0)
		difficultyEase = 0;

	if (cv_levelskull.value)
		difficultyEase = FRACUNIT;

	// Lv.   1: x0.75 avg
	// Lv. MAX: x1.05 avg
	const fixed_t rubberBase = Easing_OutSine(
		difficultyEase,
		FRACUNIT * 75 / 100,
		FRACUNIT * 105 / 100
	);

	// +/- x0.35
	const fixed_t rubberStretchiness = FixedMul(
		FixedDiv(
			35 * FRACUNIT / 100,
			K_GetKartGameSpeedScalar(gamespeed)
		),
		K_BotMapModifier()
	);

	// Lv.   1: x0.4 min
	// Lv. MAX: x0.85 min
	constexpr fixed_t rubberSlowMin = FRACUNIT / 2;
	const fixed_t rubberSlow = std::max<fixed_t>( rubberBase - rubberStretchiness, rubberSlowMin );

	// Lv.   1: x0.9 max
	// Lv. MAX: x1.35 max
	constexpr fixed_t rubberFastMax = FRACUNIT * 3 / 2;
	const fixed_t rubberFast = std::min<fixed_t>( rubberBase + rubberStretchiness, rubberFastMax );

	fixed_t rubberband = FRACUNIT >> 1;
	player_t *firstplace = nullptr;
	size_t i = SIZE_MAX;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		// Don't rubberband to ourselves...
		if (player == &players[i])
		{
			continue;
		}

		// Don't rubberband to friendlies...
		if (G_SameTeam(player, &players[i]) == true)
		{
			continue;
		}

#if 0
		// Only rubberband up to players.
		if (players[i].bot)
		{
			continue;
		}
#endif

		if (firstplace == nullptr || players[i].distancetofinish < firstplace->distancetofinish)
		{
			firstplace = &players[i];
		}
	}

	if (firstplace != nullptr)
	{
		const UINT32 spacing = FixedDiv(10240 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed)) / FRACUNIT;
		const UINT32 wanteddist = firstplace->distancetofinish + K_BotRubberbandDistance(player);
		const INT32 distdiff = player->distancetofinish - wanteddist;

		rubberband = FixedDiv(distdiff + spacing, spacing * 2);

		if (player->boostpower < FRACUNIT)
		{
			// Do not let bots cheese offroad as much.
			rubberband = FixedMul(rubberband, player->boostpower);
		}

		if (P_MobjWasRemoved(player->mo) == false && player->mo->movefactor < FRACUNIT)
		{
			// Do not let bots speed up on ice too much.
			rubberband = FixedMul(rubberband, player->mo->movefactor);
		}

		if (rubberband > FRACUNIT)
		{
			rubberband = FRACUNIT;
		}
		else if (rubberband < 0)
		{
			rubberband = 0;
		}
	}

	UINT32 scaled_dist = player->distancetofinish;
	if (mapobjectscale != FRACUNIT)
	{
		// Bring back to normal scale.
		scaled_dist = FixedDiv(scaled_dist, mapobjectscale);
	}

	UINT32 END_DIST = 2048 * 14;

	if (K_EffectiveGradingFactor(player) <= FRACUNIT)
	{
		END_DIST = Easing_Linear((K_EffectiveGradingFactor(player) - MINGRADINGFACTOR) * 2, END_DIST * 2, END_DIST);
	}

	if (scaled_dist < END_DIST)
	{
		// At the end of tracks, start slowing down.
		rubberband = FixedMul(rubberband, FixedDiv(scaled_dist, END_DIST));
	}

	return Easing_Linear(rubberband, rubberSlow, rubberFast);
}

/*--------------------------------------------------
	fixed_t K_UpdateRubberband(player_t *player)

		See header file for description.
--------------------------------------------------*/
fixed_t K_UpdateRubberband(player_t *player)
{
	fixed_t dest = K_BotRubberband(player);

	fixed_t deflect = player->botvars.recentDeflection;
	if (deflect > BOTMAXDEFLECTION)
		deflect = BOTMAXDEFLECTION;

	dest = FixedMul(dest, Easing_Linear(
		FixedDiv(deflect, BOTMAXDEFLECTION),
		BOTSTRAIGHTSPEED,
		BOTTURNSPEED
	));

	fixed_t ret = player->botvars.rubberband;

	UINT8 ease_soften = (ret > dest) ? 3 : 8;

	if (player->botvars.bumpslow && dest > ret)
		ease_soften = 80;

	// Ease into the new value.
	ret += (dest - player->botvars.rubberband) / ease_soften;

	return ret;
}

/*--------------------------------------------------
	fixed_t K_DistanceOfLineFromPoint(fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y, fixed_t cx, fixed_t cy)

		See header file for description.
--------------------------------------------------*/
fixed_t K_DistanceOfLineFromPoint(fixed_t v1x, fixed_t v1y, fixed_t v2x, fixed_t v2y, fixed_t px, fixed_t py)
{
	// Copy+paste from P_ClosestPointOnLine :pensive:
	fixed_t startx = v1x;
	fixed_t starty = v1y;
	fixed_t dx = v2x - v1x;
	fixed_t dy = v2y - v1y;

	fixed_t cx, cy;
	fixed_t vx, vy;
	fixed_t magnitude;
	fixed_t t;

	cx = px - startx;
	cy = py - starty;

	vx = dx;
	vy = dy;

	magnitude = R_PointToDist2(v2x, v2y, startx, starty);
	vx = FixedDiv(vx, magnitude);
	vy = FixedDiv(vy, magnitude);

	t = (FixedMul(vx, cx) + FixedMul(vy, cy));

	vx = FixedMul(vx, t);
	vy = FixedMul(vy, t);

	return R_PointToDist2(px, py, startx + vx, starty + vy);
}

/*--------------------------------------------------
	static void K_GetBotWaypointRadius(waypoint_t *waypoint, fixed_t *smallestRadius, fixed_t *smallestScaled)

		Calculates a new waypoint radius size to use, making it
		thinner depending on how harsh the turn is.

	Input Arguments:-
		waypoint - Waypoint to retrieve the radius of.

	Return:-
		N/A
--------------------------------------------------*/
static void K_GetBotWaypointRadius(waypoint_t *const waypoint, fixed_t *smallestRadius, fixed_t *smallestScaled)
{
	static const fixed_t maxReduce = FRACUNIT/32;
	static const angle_t maxDelta = ANGLE_22h;

	fixed_t radius = waypoint->mobj->radius;
	fixed_t reduce = FRACUNIT;
	angle_t delta = 0;

	size_t i, j;

	for (i = 0; i < waypoint->numnextwaypoints; i++)
	{
		const waypoint_t *next = waypoint->nextwaypoints[i];
		const angle_t nextAngle = R_PointToAngle2(
			waypoint->mobj->x, waypoint->mobj->y,
			next->mobj->x, next->mobj->y
		);

		for (j = 0; j < waypoint->numprevwaypoints; j++)
		{
			const waypoint_t *prev = waypoint->prevwaypoints[j];
			const angle_t prevAngle = R_PointToAngle2(
				prev->mobj->x, prev->mobj->y,
				waypoint->mobj->x, waypoint->mobj->y
			);

			delta = std::max<angle_t>(delta, AngleDelta(nextAngle, prevAngle));
		}
	}

	if (delta > maxDelta)
	{
		delta = maxDelta;
	}

	reduce = FixedDiv(delta, maxDelta);
	reduce = FRACUNIT + FixedMul(reduce, maxReduce - FRACUNIT);

	*smallestRadius = std::min<fixed_t>(*smallestRadius, radius);
	*smallestScaled = std::min<fixed_t>(*smallestScaled, FixedMul(radius, reduce));
}

static fixed_t K_ScaleWPDistWithSlope(fixed_t disttonext, angle_t angletonext, const pslope_t *slope, SINT8 flip)
{
	if (slope == nullptr)
	{
		return disttonext;
	}

	if ((slope->flags & SL_NOPHYSICS) == 0 && abs(slope->zdelta) >= FRACUNIT/21)
	{
		// Displace the prediction to go with the slope physics.
		fixed_t slopeMul = FRACUNIT;
		angle_t angle = angletonext - slope->xydirection;

		if (flip * slope->zdelta < 0)
		{
			angle ^= ANGLE_180;
		}

		// Going uphill: 0
		// Going downhill: FRACUNIT*2
		slopeMul = FRACUNIT + FINECOSINE(angle >> ANGLETOFINESHIFT);

		// Range: 0.25 to 1.75
		return FixedMul(disttonext, (FRACUNIT >> 2) + ((slopeMul * 3) >> 2));
	}

	return disttonext;
}

/*--------------------------------------------------
	static botprediction_t *K_CreateBotPrediction(const player_t *player)

		Calculates a point further along the track to attempt to drive towards.

	Input Arguments:-
		player - Player to compare.

	Return:-
		Bot prediction struct.
--------------------------------------------------*/
static botprediction_t *K_CreateBotPrediction(const player_t *player)
{
	ZoneScoped;

	const precise_t time = I_GetPreciseTime();

	// Stair janking makes it harder to steer, so attempt to steer harder.
	const UINT8 jankDiv = (player->stairjank > 0) ? 4 : 1;

	const INT16 handling = K_GetKartTurnValue(player, KART_FULLTURN) / jankDiv; // Reduce prediction based on how fast you can turn

	const tic_t futuresight = (TICRATE * KART_FULLTURN) / std::max<INT16>(1, handling); // How far ahead into the future to try and predict
	const fixed_t speed = K_BotSpeedScaled(player, P_AproxDistance(player->mo->momx, player->mo->momy));

	const INT32 startDist = 0; //(DEFAULT_WAYPOINT_RADIUS * mapobjectscale) / FRACUNIT;
	const INT32 maxDist = (DEFAULT_WAYPOINT_RADIUS * 3 * mapobjectscale) / FRACUNIT; // This function gets very laggy when it goes far distances, and going too far isn't very helpful anyway.
	const INT32 distance = std::min<INT32>(((speed / FRACUNIT) * static_cast<INT32>(futuresight)) + startDist, maxDist);

	// Halves radius when encountering a wall on your way to your destination.
	fixed_t radReduce = FRACUNIT;

	fixed_t radius = INT32_MAX;
	fixed_t radiusScaled = INT32_MAX;

	INT32 distanceleft = distance;
	angle_t angletonext = ANGLE_MAX;
	INT32 disttonext = INT32_MAX;
	INT32 distscaled = INT32_MAX;
	pslope_t *nextslope = player->mo->standingslope;

	waypoint_t *wp = player->nextwaypoint;
	mobj_t *prevwpmobj = player->mo;

	const boolean useshortcuts = K_BotCanTakeCut(player);
	const boolean huntbackwards = false;
	boolean pathfindsuccess = false;
	path_t pathtofinish = {0};

	botprediction_t *predict = nullptr;
	size_t i;

	if (wp == nullptr || P_MobjWasRemoved(wp->mobj) == true)
	{
		// Can't do any of this if we don't have a waypoint.
		return nullptr;
	}

	predict = static_cast<botprediction_t *>(Z_Calloc(sizeof(botprediction_t), PU_LEVEL, nullptr));

	// Init defaults in case of pathfind failure
	angletonext = R_PointToAngle2(prevwpmobj->x, prevwpmobj->y, wp->mobj->x, wp->mobj->y);
	disttonext = P_AproxDistance(prevwpmobj->x - wp->mobj->x, prevwpmobj->y - wp->mobj->y);
	nextslope = wp->mobj->standingslope;
	distscaled = K_ScaleWPDistWithSlope(disttonext, angletonext, nextslope, P_MobjFlip(wp->mobj)) / FRACUNIT;

	pathfindsuccess = K_PathfindThruCircuit(
		wp, (unsigned)distanceleft,
		&pathtofinish,
		useshortcuts, huntbackwards
	);

	// Go through the waypoints until we've traveled the distance we wanted to predict ahead!
	if (pathfindsuccess == true)
	{
		for (i = 0; i < pathtofinish.numnodes; i++)
		{
			wp = (waypoint_t *)pathtofinish.array[i].nodedata;

			if (i == 0)
			{
				prevwpmobj = player->mo;
			}
			else
			{
				prevwpmobj = ((waypoint_t *)pathtofinish.array[ i - 1 ].nodedata)->mobj;
			}

			angletonext = R_PointToAngle2(prevwpmobj->x, prevwpmobj->y, wp->mobj->x, wp->mobj->y);
			disttonext = P_AproxDistance(prevwpmobj->x - wp->mobj->x, prevwpmobj->y - wp->mobj->y);
			nextslope = wp->mobj->standingslope;
			distscaled = K_ScaleWPDistWithSlope(disttonext, angletonext, nextslope, P_MobjFlip(wp->mobj)) / FRACUNIT;

			if (P_TraceBotTraversal(player->mo, wp->mobj) == false)
			{
				// If we can't get a direct path to this waypoint, reduce our prediction drastically.
				distscaled *= 4;
				radReduce = FRACUNIT >> 1;
			}

			K_GetBotWaypointRadius(wp, &radius, &radiusScaled);
			distanceleft -= distscaled;

			if (distanceleft <= 0)
			{
				// We're done!!
				break;
			}
		}

		Z_Free(pathtofinish.array);
	}

	// Set our predicted point's coordinates,
	// and use the smallest radius of all of the waypoints in the chain!
	predict->x = wp->mobj->x;
	predict->y = wp->mobj->y;

	predict->baseRadius = radius;
	predict->radius = FixedMul(radiusScaled, radReduce);

	// Set the prediction coordinates between the 2 waypoints if there's still distance left.
	if (distanceleft > 0)
	{
		// Scaled with the leftover anglemul!
		predict->x += P_ReturnThrustX(nullptr, angletonext, std::min<fixed_t>(disttonext, distanceleft) * FRACUNIT);
		predict->y += P_ReturnThrustY(nullptr, angletonext, std::min<fixed_t>(disttonext, distanceleft) * FRACUNIT);
	}

	ps_bots[player - players].prediction += I_GetPreciseTime() - time;
	return predict;
}

/*--------------------------------------------------
	static UINT8 K_TrySpindash(const player_t *player, ticcmd_t *cmd)

		Determines conditions where the bot should attempt to spindash.

	Input Arguments:-
		player - Bot player to check.

	Return:-
		0 to make the bot drive normally, 1 to e-brake, 2 to e-brake & charge spindash.
		(TODO: make this an enum)
--------------------------------------------------*/
static UINT8 K_TrySpindash(const player_t *player, ticcmd_t *cmd)
{
	ZoneScoped;

	const tic_t difficultyModifier = (TICRATE/6);

	const fixed_t oldSpeed = R_PointToDist2(0, 0, player->rmomx, player->rmomy);
	const fixed_t baseAccel = K_GetNewSpeed(player) - oldSpeed;
	const fixed_t speedDiff = player->speed - player->lastspeed;

	const INT32 angleDiff = AngleDelta(player->mo->angle, K_MomentumAngleReal(player->mo));

	if (player->spindashboost || player->tiregrease // You just released a spindash, you don't need to try again yet, jeez.
		|| P_IsObjectOnGround(player->mo) == false) // Not in a state where we want 'em to spindash.
	{
		return 0;
	}

	// Try "start boosts" first
	if (leveltime == starttime)
	{
		// Forces them to release, even if they haven't fully charged.
		// Don't want them to keep charging if they didn't have time to.
		return 0;
	}

	if (leveltime < starttime)
	{
		INT32 boosthold = starttime - K_GetSpindashChargeTime(player);

		boosthold -= (DIFFICULTBOT - std::min<UINT8>(DIFFICULTBOT, player->botvars.difficulty)) * difficultyModifier;

		if (leveltime >= (unsigned)boosthold)
		{
			// Start charging...
			return 2;
		}
		else
		{
			// Just hold your ground and e-brake.
			return 1;
		}
	}

	if (player->botvars.spindashconfirm >= BOTSPINDASHCONFIRM)
	{
		INT32 chargingPoint = (K_GetSpindashChargeTime(player) + difficultyModifier);

		// Release quicker the higher the difficulty is.
		// Sounds counter-productive, but that's actually the best strategy after the race has started.
		chargingPoint -= std::min<UINT8>(DIFFICULTBOT, player->botvars.difficulty) * difficultyModifier;

		if (player->spindash > chargingPoint)
		{
			// Time to release.
			return 0;
		}

		return 2;
	}
	else
	{
		// Logic for normal racing.
		boolean anyCondition = false;
		boolean uphill = false;

#define AddForCondition(x) \
	if (x) \
	{ \
		anyCondition = true;\
		if (player->botvars.spindashconfirm < BOTSPINDASHCONFIRM) \
		{ \
			cmd->bot.spindashconfirm++; \
		} \
	}

		if (K_SlopeResistance(player) == false && player->mo->standingslope != nullptr)
		{
			const pslope_t *slope = player->mo->standingslope;

			if ((slope->flags & SL_NOPHYSICS) == 0 && abs(slope->zdelta) >= FRACUNIT/21)
			{
				const fixed_t speedPercent = FixedDiv(player->speed, 20 * player->mo->scale);
				fixed_t slopeDot = 0;
				angle_t angle = K_MomentumAngle(player->mo) - slope->xydirection;

				if (P_MobjFlip(player->mo) * slope->zdelta < 0)
				{
					angle ^= ANGLE_180;
				}

				slopeDot = FINECOSINE(angle >> ANGLETOFINESHIFT);
				uphill = ((slopeDot + (speedPercent / 2)) < -FRACUNIT/2);
			}
		}

		constexpr fixed_t minimum_offroad = (3 << FRACBITS) >> 1; // Do not spindash in weak offroad
		AddForCondition(K_ApplyOffroad(player) == true && player->offroad > minimum_offroad); // Slowed by offroad
		AddForCondition(speedDiff < (baseAccel >> 3)); // Accelerating slower than expected
		AddForCondition(angleDiff > ANG60); // Being pushed backwards
		AddForCondition(uphill == true); // Going up a steep slope without speed

		if (player->cmomx || player->cmomy)
		{
			angle_t cAngle = R_PointToDist2(0, 0, player->cmomx, player->cmomy);
			angle_t cDelta = AngleDelta(player->mo->angle, cAngle);

			AddForCondition(cDelta > ANGLE_90); // Conveyor going against you
		}

		if (anyCondition == false)
		{
			if (player->botvars.spindashconfirm > 0)
			{
				cmd->bot.spindashconfirm--;
			}
		}
	}

	// We're doing just fine, we don't need to spindash, thanks.
	return 0;
}

/*--------------------------------------------------
	static boolean K_TryRingShooter(const player_t *player, const botcontroller_t *botController)

		Determines conditions where the bot should attempt to respawn.

	Input Arguments:-
		player - Bot player to check.
		botController - Bot controller struct, if it exists.

	Return:-
		true if we want to hold the respawn button, otherwise false.
--------------------------------------------------*/
static boolean K_TryRingShooter(const player_t *player, const botcontroller_t *botController)
{
	ZoneScoped;

	if (player->respawn.state != RESPAWNST_NONE)
	{
		// We're already respawning!
		return false;
	}

	if (player->exiting)
	{
		// Where are you trying to go?
		return false;
	}

	if ((gametyperules & GTR_CIRCUIT) == 0 || (leveltime <= starttime))
	{
		// Only do this during a Race that has started.
		return false;
	}

	if (botController != nullptr && (botController->flags & TMBOT_NOCONTROL) == TMBOT_NOCONTROL)
	{
		// Bot controls are disabled, so WANT to sit still.
		return false;
	}

	return true;
}

/*--------------------------------------------------
	static void K_DrawPredictionDebug(botprediction_t *predict, const player_t *player)

		Draws objects to show where the viewpoint bot is trying to go.

	Input Arguments:-
		predict - The prediction to visualize.
		player - The bot player this prediction is for.

	Return:-
		None
--------------------------------------------------*/
static void K_DrawPredictionDebug(botprediction_t *predict, const player_t *player)
{
	mobj_t *debugMobj = nullptr;
	angle_t sideAngle = ANGLE_MAX;
	UINT8 i = UINT8_MAX;

	I_Assert(predict != nullptr);
	I_Assert(player != nullptr);
	I_Assert(player->mo != nullptr && P_MobjWasRemoved(player->mo) == false);

	sideAngle = player->mo->angle + ANGLE_90;

	debugMobj = P_SpawnMobj(predict->x, predict->y, player->mo->z, MT_SPARK);
	P_SetMobjState(debugMobj, S_THOK);

	debugMobj->frame &= ~FF_TRANSMASK;
	debugMobj->frame |= FF_TRANS20|FF_FULLBRIGHT;

	debugMobj->color = SKINCOLOR_ORANGE;
	P_SetScale(debugMobj, debugMobj->destscale * 2);

	debugMobj->tics = 2;

	for (i = 0; i < 2; i++)
	{
		mobj_t *radiusMobj = nullptr;
		fixed_t radiusX = predict->x, radiusY = predict->y;

		if (i & 1)
		{
			radiusX -= FixedMul(predict->radius, FINECOSINE(sideAngle >> ANGLETOFINESHIFT));
			radiusY -= FixedMul(predict->radius, FINESINE(sideAngle >> ANGLETOFINESHIFT));
		}
		else
		{
			radiusX += FixedMul(predict->radius, FINECOSINE(sideAngle >> ANGLETOFINESHIFT));
			radiusY += FixedMul(predict->radius, FINESINE(sideAngle >> ANGLETOFINESHIFT));
		}

		radiusMobj = P_SpawnMobj(radiusX, radiusY, player->mo->z, MT_SPARK);
		P_SetMobjState(radiusMobj, S_THOK);

		radiusMobj->frame &= ~FF_TRANSMASK;
		radiusMobj->frame |= FF_TRANS20|FF_FULLBRIGHT;

		radiusMobj->color = SKINCOLOR_YELLOW;
		P_SetScale(debugMobj, debugMobj->destscale / 2);

		radiusMobj->tics = 2;
	}
}

/*--------------------------------------------------
	static void K_BotTrick(const player_t *player, ticcmd_t *cmd, const botcontroller_t *botController)

		Determines inputs for trick panels.

	Input Arguments:-
		player - Player to generate the ticcmd for.
		cmd - The player's ticcmd to modify.
		botController - Bot controller struct.

	Return:-
		None
--------------------------------------------------*/
static void K_BotTrick(const player_t *player, ticcmd_t *cmd, const botcontroller_t *botController)
{
	// Trick panel state -- do nothing until a controller line is found, in which case do a trick.
	if (botController == nullptr)
	{
		return;
	}

	if (player->trickpanel == TRICKSTATE_READY)
	{
		switch (botController->trick)
		{
			case TMBOTTR_LEFT:
				cmd->turning = KART_FULLTURN;
				break;
			case TMBOTTR_RIGHT:
				cmd->turning = -KART_FULLTURN;
				break;
			case TMBOTTR_UP:
				cmd->throwdir = KART_FULLTURN;
				break;
			case TMBOTTR_DOWN:
				cmd->throwdir = -KART_FULLTURN;
				break;
		}
	}
}

/*--------------------------------------------------
	static angle_t K_BotSmoothLanding(const player_t *player, angle_t destangle)

		Calculates a new destination angle while in the air,
		to be able to successfully smooth land.

	Input Arguments:-
		player - Bot player to check.
		destangle - Previous destination angle.

	Return:-
		New destination angle.
--------------------------------------------------*/
static angle_t K_BotSmoothLanding(const player_t *player, angle_t destangle)
{
	ZoneScoped;

	angle_t newAngle = destangle;
	boolean air = !P_IsObjectOnGround(player->mo);
	angle_t steepVal = air ? STUMBLE_STEEP_VAL_AIR : STUMBLE_STEEP_VAL;
	angle_t slopeSteep = std::max<angle_t>(AngleDelta(player->mo->pitch, 0), AngleDelta(player->mo->roll, 0));

	if (slopeSteep > steepVal)
	{
		fixed_t pitchMul = -FINESINE(destangle >> ANGLETOFINESHIFT);
		fixed_t rollMul = FINECOSINE(destangle >> ANGLETOFINESHIFT);
		angle_t testAngles[2];
		angle_t testDeltas[2];
		UINT8 i;

		testAngles[0] = R_PointToAngle2(0, 0, rollMul, pitchMul);
		testAngles[1] = R_PointToAngle2(0, 0, -rollMul, -pitchMul);

		for (i = 0; i < 2; i++)
		{
			testDeltas[i] = AngleDelta(testAngles[i], destangle);
		}

		if (testDeltas[1] < testDeltas[0])
		{
			return testAngles[1];
		}
		else
		{
			return testAngles[0];
		}
	}

	return newAngle;
}

/*--------------------------------------------------
	static INT32 K_HandleBotTrack(const player_t *player, ticcmd_t *cmd, botprediction_t *predict)

		Determines inputs for standard track driving.

	Input Arguments:-
		player - Player to generate the ticcmd for.
		cmd - The player's ticcmd to modify.
		predict - Pointer to the bot's prediction.

	Return:-
		New value for turn amount.
--------------------------------------------------*/
static INT32 K_HandleBotTrack(const player_t *player, ticcmd_t *cmd, botprediction_t *predict, angle_t destangle)
{
	ZoneScoped;

	// Handle steering towards waypoints!
	INT32 turnamt = 0;
	SINT8 turnsign = 0;
	angle_t moveangle;
	INT32 anglediff;

	I_Assert(predict != nullptr);

	destangle = K_BotSmoothLanding(player, destangle);
	moveangle = player->mo->angle + K_GetUnderwaterTurnAdjust(player);
	anglediff = AngleDeltaSigned(moveangle, destangle);

	// predictionerror
	cmd->angle = std::min(destangle - moveangle, moveangle - destangle) >> TICCMD_REDUCE;

	if (anglediff < 0)
	{
		turnsign = 1;
	}
	else
	{
		turnsign = -1;
	}

	anglediff = abs(anglediff);
	turnamt = KART_FULLTURN * turnsign;

	if (anglediff > ANGLE_67h)
	{
		// Wrong way!
		cmd->forwardmove = -MAXPLMOVE;
		cmd->buttons |= BT_BRAKE;
	}
	else
	{
		const fixed_t playerwidth = (player->mo->radius * 2);
		fixed_t realrad = predict->radius*3/4; // Remove a "safe" distance away from the edges of the road
		fixed_t rad = realrad;
		fixed_t dirdist = K_DistanceOfLineFromPoint(
			player->mo->x, player->mo->y,
			player->mo->x + FINECOSINE(moveangle >> ANGLETOFINESHIFT), player->mo->y + FINESINE(moveangle >> ANGLETOFINESHIFT),
			predict->x, predict->y
		);

		if (realrad < playerwidth)
		{
			realrad = playerwidth;
		}

		// Become more precise based on how hard you need to turn
		// This makes predictions into turns a little nicer
		// Facing 90 degrees away from the predicted point gives you 0 radius
		rad = FixedMul(rad,
			FixedDiv(std::max<angle_t>(0, ANGLE_90 - anglediff), ANGLE_90)
		);

		// Become more precise the slower you're moving
		// Also helps with turns
		// Full speed uses full radius
		rad = FixedMul(rad,
			FixedDiv(K_BotSpeedScaled(player, player->speed), K_GetKartSpeed(player, false, false))
		);

		// Cap the radius to reasonable bounds
		if (rad > realrad)
		{
			rad = realrad;
		}
		else if (rad < playerwidth)
		{
			rad = playerwidth;
		}

		// Full speed ahead!
		cmd->buttons |= BT_ACCELERATE;
		cmd->forwardmove = MAXPLMOVE;

		if (dirdist <= rad)
		{
			// Going the right way, don't turn at all.
			turnamt = 0;
		}
	}

	return turnamt;
}

/*--------------------------------------------------
	static INT32 K_HandleBotReverse(const player_t *player, ticcmd_t *cmd, botprediction_t *predict)

		Determines inputs for reversing.

	Input Arguments:-
		player - Player to generate the ticcmd for.
		cmd - The player's ticcmd to modify.
		predict - Pointer to the bot's prediction.

	Return:-
		New value for turn amount.
--------------------------------------------------*/
static INT32 K_HandleBotReverse(const player_t *player, ticcmd_t *cmd, botprediction_t *predict, angle_t destangle)
{
	ZoneScoped;

	// Handle steering towards waypoints!
	INT32 turnamt = 0;
	SINT8 turnsign = 0;
	angle_t moveangle, angle;
	INT16 anglediff, momdiff;

	if (predict != nullptr)
	{
		// TODO: Should we reverse through bot controllers?
		return K_HandleBotTrack(player, cmd, predict, destangle);
	}

	if (player->nextwaypoint == nullptr
		|| player->nextwaypoint->mobj == nullptr
		|| P_MobjWasRemoved(player->nextwaypoint->mobj))
	{
		// No data available...
		return 0;
	}

	if ((player->nextwaypoint->prevwaypoints != nullptr)
		&& (player->nextwaypoint->numprevwaypoints > 0U))
	{
		size_t i;
		for (i = 0U; i < player->nextwaypoint->numprevwaypoints; i++)
		{
			if (!K_GetWaypointIsEnabled(player->nextwaypoint->prevwaypoints[i]))
			{
				continue;
			}

			destangle = R_PointToAngle2(
				player->nextwaypoint->prevwaypoints[i]->mobj->x, player->nextwaypoint->prevwaypoints[i]->mobj->y,
				player->nextwaypoint->mobj->x, player->nextwaypoint->mobj->y
			);

			break;
		}
	}

	destangle = K_BotSmoothLanding(player, destangle);

	// Calculate turn direction first.
	moveangle = player->mo->angle + K_GetUnderwaterTurnAdjust(player);
	angle = (moveangle - destangle);

	if (angle < ANGLE_180)
	{
		turnsign = -1; // Turn right
		anglediff = AngleFixed(angle)>>FRACBITS;
	}
	else
	{
		turnsign = 1; // Turn left
		anglediff = 360-(AngleFixed(angle)>>FRACBITS);
	}

	anglediff = abs(anglediff);
	turnamt = KART_FULLTURN * turnsign;

	// Now calculate momentum
	momdiff = 180;
	if (player->speed > player->mo->scale)
	{
		momdiff = 0;
		moveangle = K_MomentumAngle(player->mo);
		angle = (moveangle - destangle);

		if (angle < ANGLE_180)
		{
			momdiff = AngleFixed(angle)>>FRACBITS;
		}
		else
		{
			momdiff = 360-(AngleFixed(angle)>>FRACBITS);
		}

		momdiff = abs(momdiff);
	}

	if (anglediff > 90 || momdiff < 90)
	{
		// We're not facing the track,
		// or we're going too fast.
		// Let's E-Brake.
		cmd->forwardmove = 0;
		cmd->buttons |= BT_ACCELERATE|BT_BRAKE;
	}
	else
	{
		fixed_t slopeMul = FRACUNIT;

		if (player->mo->standingslope != nullptr)
		{
			const pslope_t *slope = player->mo->standingslope;

			if (!(slope->flags & SL_NOPHYSICS) && abs(slope->zdelta) >= FRACUNIT/21)
			{
				angle_t sangle = player->mo->angle - slope->xydirection;

				if (P_MobjFlip(player->mo) * slope->zdelta < 0)
					sangle ^= ANGLE_180;

				slopeMul = FRACUNIT - FINECOSINE(sangle >> ANGLETOFINESHIFT);
			}
		}

#define STEEP_SLOPE (FRACUNIT*11/10)
		if (slopeMul > STEEP_SLOPE)
		{
			// Slope is too steep to reverse -- EBrake.
			cmd->forwardmove = 0;
			cmd->buttons |= BT_ACCELERATE|BT_BRAKE;
		}
		else
		{
			cmd->forwardmove = -MAXPLMOVE;
			cmd->buttons |= BT_BRAKE; //|BT_LOOKBACK
		}
#undef STEEP_SLOPE

		if (anglediff < 10)
		{
			turnamt = 0;
		}
	}

	return turnamt;
}

/*--------------------------------------------------
	static void K_BotPodiumTurning(const player_t *player, ticcmd_t *cmd)

		Calculates bot turning for the podium cutscene.
--------------------------------------------------*/
static void K_BotPodiumTurning(const player_t *player, ticcmd_t *cmd)
{
	const angle_t destAngle = R_PointToAngle2(
		player->mo->x, player->mo->y,
		player->currentwaypoint->mobj->x, player->currentwaypoint->mobj->y
	);
	const INT32 delta = AngleDeltaSigned(destAngle, player->mo->angle);
	const INT16 handling = K_GetKartTurnValue(player, KART_FULLTURN);
	fixed_t mul = FixedDiv(delta, (angle_t)(handling << TICCMD_REDUCE));

	if (mul > FRACUNIT)
	{
		mul = FRACUNIT;
	}

	if (mul < -FRACUNIT)
	{
		mul = -FRACUNIT;
	}

	cmd->turning = FixedMul(mul, KART_FULLTURN);
}

/*--------------------------------------------------
	static void K_BuildBotPodiumTiccmd(const player_t *player, ticcmd_t *cmd)

		Calculates all bot movement for the podium cutscene.
--------------------------------------------------*/
static void K_BuildBotPodiumTiccmd(const player_t *player, ticcmd_t *cmd)
{
	if (player->currentwaypoint == nullptr)
	{
		// We've reached the end of our path.
		// Simply stop moving.
		return;
	}

	if (K_GetWaypointIsSpawnpoint(player->currentwaypoint) == false)
	{
		// Hacky flag reuse: slow down before reaching your podium stand.
		cmd->forwardmove = MAXPLMOVE * 3 / 4;
	}
	else
	{
		cmd->forwardmove = MAXPLMOVE;
	}

	cmd->buttons |= BT_ACCELERATE;

	K_BotPodiumTurning(player, cmd);
}

/*--------------------------------------------------
	static void K_BuildBotTiccmdNormal(const player_t *player, ticcmd_t *cmd)

		Build ticcmd for bots with a style of BOT_STYLE_NORMAL
--------------------------------------------------*/
static void K_BuildBotTiccmdNormal(player_t *player, ticcmd_t *cmd)
{
	precise_t t = 0;

	botprediction_t *predict = nullptr;
	auto predict_finally = srb2::finally([&predict]() { Z_Free(predict); });

	boolean trySpindash = true;
	angle_t destangle = 0;
	UINT8 spindash = 0;
	INT32 turnamt = 0;

	cmd->angle = 0; // For bots, this is used to transmit predictionerror to gamelogic.
	// Will be overwritten by K_HandleBotTrack if we have a destination.

	if (!(gametyperules & GTR_BOTS) // No bot behaviors
		|| K_GetNumWaypoints() == 0 // No waypoints
		|| leveltime <= introtime // During intro camera
		|| player->playerstate == PST_DEAD // Dead, respawning.
		|| player->mo->scale <= 1) // Post-finish "death" animation
	{
		// No need to do anything else.
		return;
	}

	if (player->exiting && player->nextwaypoint == K_GetFinishLineWaypoint() && ((mapheaderinfo[gamemap - 1]->levelflags & LF_SECTIONRACE) == LF_SECTIONRACE))
	{
		// Sprint map finish, don't give Sal's children migraines trying to pathfind out
		return;
	}

	// Defanging bots for testing.
	#ifdef DEVELOP
		if (!cv_botcontrol.value)
			return;
	#endif

	// Actual gameplay behaviors below this block!
	const botcontroller_t *botController = K_GetBotController(player->mo);
	if (player->trickpanel != TRICKSTATE_NONE)
	{
		K_BotTrick(player, cmd, botController);

		// Don't do anything else.
		return;
	}

	if (botController != nullptr && (botController->flags & TMBOT_NOCONTROL) == TMBOT_NOCONTROL)
	{
		// Disable bot controls entirely.
		return;
	}

	if (K_TryRingShooter(player, botController) == true && player->botvars.respawnconfirm >= BOTRESPAWNCONFIRM)
	{
		// We want to respawn. Simply hold Y and stop here!
		cmd->buttons |= BT_RESPAWNMASK;
		return;
	}

	destangle = player->mo->angle;

	boolean forcedDir = false;
	if (botController != nullptr && (botController->flags & TMBOT_FORCEDIR) == TMBOT_FORCEDIR)
	{
		const fixed_t dist = DEFAULT_WAYPOINT_RADIUS * player->mo->scale;

		// Overwritten prediction
		predict = static_cast<botprediction_t *>(Z_Calloc(sizeof(botprediction_t), PU_STATIC, nullptr));

		predict->x = player->mo->x + FixedMul(dist, FINECOSINE(botController->forceAngle >> ANGLETOFINESHIFT));
		predict->y = player->mo->y + FixedMul(dist, FINESINE(botController->forceAngle >> ANGLETOFINESHIFT));
		predict->radius = (DEFAULT_WAYPOINT_RADIUS / 4) * mapobjectscale;

		forcedDir = true;
	}

	if (P_IsObjectOnGround(player->mo) == false)
	{
		if (player->fastfall == 0 && player->respawn.state == RESPAWNST_NONE)
		{
			if (botController != nullptr && (botController->flags & TMBOT_FASTFALL) == TMBOT_FASTFALL)
			{
				// Fast fall!
				cmd->buttons |= BT_EBRAKEMASK;
				return;
			}
		}

		//return; // Don't allow bots to turn in the air.
	}

	if (forcedDir == true)
	{
		destangle = R_PointToAngle2(player->mo->x, player->mo->y, predict->x, predict->y);
		turnamt = K_HandleBotTrack(player, cmd, predict, destangle);
		trySpindash = false;
	}
	else if (leveltime <= starttime && finishBeamLine != nullptr)
	{
		// Handle POSITION!!
		const fixed_t distBase = 480*mapobjectscale;
		const fixed_t distAdjust = 128*mapobjectscale;

		const fixed_t closeDist = distBase + (distAdjust * (9 - player->kartweight));
		const fixed_t farDist = closeDist + (distAdjust * 2);

		const tic_t futureSight = (TICRATE >> 1);

		fixed_t distToFinish = K_DistanceOfLineFromPoint(
			finishBeamLine->v1->x, finishBeamLine->v1->y,
			finishBeamLine->v2->x, finishBeamLine->v2->y,
			player->mo->x, player->mo->y
		) - (K_BotSpeedScaled(player, player->speed) * futureSight);

		// Don't run the spindash code at all until we're in the right place
		trySpindash = false;

		if (distToFinish < closeDist)
		{
			// We're too close, we need to start backing up.
			turnamt = K_HandleBotReverse(player, cmd, predict, destangle);
		}
		else if (distToFinish < farDist)
		{
			INT32 bullyTurn = INT32_MAX;

			// We're in about the right place, let's do whatever we want to.

			if (player->kartspeed >= 5)
			{
				// Faster characters want to spindash.
				// Slower characters will use their momentum.
				trySpindash = true;
			}

			// Look for characters to bully.
			bullyTurn = K_PositionBully(player);
			if (bullyTurn == INT32_MAX)
			{
				// No one to bully, just go for a spindash as anyone.
				if (predict == nullptr)
				{
					// Create a prediction.
					predict = K_CreateBotPrediction(player);
				}

				if (predict != nullptr)
				{
					K_NudgePredictionTowardsObjects(predict, player);
					destangle = R_PointToAngle2(player->mo->x, player->mo->y, predict->x, predict->y);
					turnamt = K_HandleBotTrack(player, cmd, predict, destangle);
				}
				cmd->buttons &= ~(BT_ACCELERATE|BT_BRAKE);
				cmd->forwardmove = 0;
				trySpindash = true;
			}
			else
			{
				turnamt = bullyTurn;

				// If already spindashing, wait until we get a relatively OK charge first.
				if (player->spindash == 0 || player->spindash > TICRATE)
				{
					trySpindash = false;
					cmd->buttons |= BT_ACCELERATE;
					cmd->forwardmove = MAXPLMOVE;
				}
			}
		}
		else
		{
			// Too far away, we need to just drive up.
			if (predict == nullptr)
			{
				// Create a prediction.
				predict = K_CreateBotPrediction(player);
			}

			if (predict != nullptr)
			{
				K_NudgePredictionTowardsObjects(predict, player);
				destangle = R_PointToAngle2(player->mo->x, player->mo->y, predict->x, predict->y);
				turnamt = K_HandleBotTrack(player, cmd, predict, destangle);
			}
		}
	}
	else
	{
		// Handle steering towards waypoints!
		if (predict == nullptr)
		{
			// Create a prediction.
			predict = K_CreateBotPrediction(player);
		}

		if (predict != nullptr)
		{
			K_NudgePredictionTowardsObjects(predict, player);
			destangle = R_PointToAngle2(player->mo->x, player->mo->y, predict->x, predict->y);
			turnamt = K_HandleBotTrack(player, cmd, predict, destangle);
		}
	}

	if (trySpindash == true)
	{
		// Spindashing
		spindash = K_TrySpindash(player, cmd);

		if (spindash > 0)
		{
			cmd->buttons |= BT_EBRAKEMASK;
			cmd->forwardmove = 0;

			if (spindash == 2 && player->speed < 6*mapobjectscale)
			{
				cmd->buttons |= BT_DRIFT;
			}
		}
	}

	if (spindash == 0 && player->exiting == 0)
	{
		// Don't pointlessly try to use rings/sneakers while charging a spindash.
		// TODO: Allowing projectile items like orbinaut while e-braking would be nice, maybe just pass in the spindash variable?
		t = I_GetPreciseTime();
		K_BotItemUsage(player, cmd, turnamt);
		ps_bots[player - players].item = I_GetPreciseTime() - t;
	}

	// Update turning quicker if we're moving at high speeds.
	UINT8 turndelta = (player->speed > (7 * K_GetKartSpeed(player, false, false) / 4)) ? 2 : 1;

	if (turnamt != 0)
	{
		if (turnamt > KART_FULLTURN)
		{
			turnamt = KART_FULLTURN;
		}
		else if (turnamt < -KART_FULLTURN)
		{
			turnamt = -KART_FULLTURN;
		}

		if (turnamt > 0)
		{
			// Count up
			if (player->botvars.turnconfirm < BOTTURNCONFIRM)
			{
				cmd->bot.turnconfirm += turndelta;
			}
		}
		else if (turnamt < 0)
		{
			// Count down
			if (player->botvars.turnconfirm > -BOTTURNCONFIRM)
			{
				cmd->bot.turnconfirm -= turndelta;
			}
		}
		else
		{
			// Back to neutral
			if (player->botvars.turnconfirm < 0)
			{
				cmd->bot.turnconfirm++;
			}
			else if (player->botvars.turnconfirm > 0)
			{
				cmd->bot.turnconfirm--;
			}
		}

		if (abs(player->botvars.turnconfirm) >= BOTTURNCONFIRM)
		{
			// You're commiting to your turn, you're allowed!
			cmd->turning = turnamt;
		}
	}

	// Free the prediction we made earlier
	if (predict != nullptr)
	{
		if (cv_kartdebugbots.value != 0 && player - players == displayplayers[0] && !(paused || P_AutoPause()))
		{
			K_DrawPredictionDebug(predict, player);
		}
	}
}

/*--------------------------------------------------
	void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd)

		See header file for description.
--------------------------------------------------*/
void K_BuildBotTiccmd(
	player_t *player, // annoyingly NOT const because of LUA_HookTiccmd... grumble grumble
	ticcmd_t *cmd)
{
	ZoneScoped;

	// Remove any existing controls
	memset(cmd, 0, sizeof(ticcmd_t));

	if (player->mo == nullptr
		|| player->spectator == true
		|| G_GamestateUsesLevel() == false)
	{
		// Not in the level.
		return;
	}

	// Complete override of all ticcmd functionality.
	// May add more hooks to individual pieces of bot ticcmd,
	// but this should always be here so anyone can roll
	// their own :)
	if (LUA_HookTiccmd(player, cmd, HOOK(BotTiccmd)) == true)
	{
		cmd->flags |= TICCMD_BOT;
		return;
	}

	cmd->flags |= TICCMD_BOT;

	if (K_PodiumSequence() == true)
	{
		K_BuildBotPodiumTiccmd(player, cmd);
		return;
	}

	switch (player->botvars.style)
	{
		case BOT_STYLE_STAY:
		{
			// Hey, this one's pretty easy :P
			break;
		}
		default:
		{
			K_BuildBotTiccmdNormal(player, cmd);
			break;
		}
	}
}

/*--------------------------------------------------
	void K_UpdateBotGameplayVars(player_t *player);

		See header file for description.
--------------------------------------------------*/
void K_UpdateBotGameplayVars(player_t *player)
{
	if (gamestate != GS_LEVEL || !player->mo)
	{
		// Not in the level.
		return;
	}

	if (cv_levelskull.value)
		player->botvars.difficulty = MAXBOTDIFFICULTY;

	if (K_InRaceDuel())
		player->botvars.rival = true;
	else if (grandprixinfo.gp != true)
		player->botvars.rival = false;

	player->botvars.rubberband = K_UpdateRubberband(player);

	player->botvars.turnconfirm += player->cmd.bot.turnconfirm;

	if (player->spindashboost || player->tiregrease // You just released a spindash, you don't need to try again yet, jeez.
		|| P_IsObjectOnGround(player->mo) == false) // Not in a state where we want 'em to spindash.
	{
		player->botvars.spindashconfirm = 0;
	}
	else
	{
		if (player->cmd.bot.spindashconfirm < 0 && abs(player->cmd.bot.spindashconfirm) > player->botvars.spindashconfirm)
		{
			player->botvars.spindashconfirm = 0;
		}
		else
		{
			player->botvars.spindashconfirm += player->cmd.bot.spindashconfirm;
		}
	}

	angle_t mangle = K_MomentumAngleEx(player->mo, 5*mapobjectscale); // magic threshold
	angle_t langle = player->botvars.lastAngle;
	angle_t dangle = 0;
	if (mangle >= langle)
		dangle = mangle - langle;
	else
		dangle = langle - mangle;
	// Writing this made me move my tongue around in my mouth

	UINT32 smo = BOTANGLESAMPLES - 1;

	player->botvars.recentDeflection = (smo * player->botvars.recentDeflection / BOTANGLESAMPLES) + (dangle / BOTANGLESAMPLES);

	player->botvars.lastAngle = mangle;

	const botcontroller_t *botController = K_GetBotController(player->mo);
	if (K_TryRingShooter(player, botController) == true)
	{
		// Our anti-grief system is already a perfect system
		// for determining if we're not making progress, so
		// lets reuse it for bot respawning!
		P_IncrementGriefValue(player, &player->botvars.respawnconfirm, BOTRESPAWNCONFIRM);
	}

	K_UpdateBotGameplayVarsItemUsage(player);
}

boolean K_BotUnderstandsItem(kartitems_t item)
{
	if (item == KITEM_BALLHOG)
		return false; // Sorry. MRs welcome!
	return true;
}
