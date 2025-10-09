// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_grandprix.cpp
/// \brief Grand Prix mode game logic & bot behaviors

#include "k_grandprix.h"

#include <algorithm>
#include <vector>

#include "k_specialstage.h"
#include "doomdef.h"
#include "d_player.h"
#include "g_game.h"
#include "k_bot.h"
#include "k_kart.h"
#include "k_podium.h"
#include "m_random.h"
#include "p_local.h"
#include "r_things.h"
#include "lua_hook.h"

struct grandprixinfo grandprixinfo;

/*--------------------------------------------------
	UINT8 K_BotStartingDifficulty(SINT8 value)

		See header file for description.
--------------------------------------------------*/
UINT8 K_BotStartingDifficulty(SINT8 value)
{
	// startingdifficulty: Easy = 3, Normal = 6, Hard = 9
	SINT8 difficulty = (value + 1) * 3;

	if (difficulty > MAXBOTDIFFICULTY)
	{
		difficulty = MAXBOTDIFFICULTY;
	}
	else if (difficulty < 1)
	{
		difficulty = 1;
	}

	return difficulty;
}

/*--------------------------------------------------
	INT16 K_CalculateGPRankPoints(UINT16 diplayexp, UINT8 position, UINT8 numplayers)

		See header file for description.
--------------------------------------------------*/
INT16 K_CalculateGPRankPoints(UINT16 exp, UINT8 position, UINT8 numplayers)
{
	INT16 points;

	if (position > numplayers || position == 0)
	{
		// Invalid position, no points
		return 0;
	}

	points = exp;

	// somehow underflowed?
	if (points < 0)
	{
		points = 0;
	}

	LUA_HookGPRankPoints(position, numplayers, &points);

	return points;
}

/*--------------------------------------------------
	UINT8 K_GetGPPlayerCount(UINT8 humans)

		See header file for description.
--------------------------------------------------*/
UINT8 K_GetGPPlayerCount(UINT8 humans)
{
	// 1P -> 8 total
	// 2P -> 8 total
	// 3P -> 12 total
	// 4P -> 16 total
	return std::clamp<UINT8>(humans * 4, 8, MAXPLAYERS);
}

// Kind of hate unsigned types
static UINT8 K_GetOffsetStartingDifficulty(const UINT8 startingdifficulty, UINT8 offset)
{
	if (offset >= startingdifficulty)
		return 1;
	return startingdifficulty - offset;
}

/*--------------------------------------------------
	static INT16 K_RivalScore(player_t *bot)

		Creates a "rival score" for a bot, used to determine which bot is the
		most deserving of the rival status.

	Input Arguments:-
		bot - Player to check.

	Return:-
		"Rival score" value.
--------------------------------------------------*/
static INT16 K_RivalScore(player_t *bot)
{
	const UINT16 difficulty = bot->botvars.difficulty;
	const UINT16 score = bot->score;
	SINT8 roundnum = 1, roundsleft = 1;
	UINT16 lowestscore = UINT16_MAX;
	UINT8 lowestdifficulty = MAXBOTDIFFICULTY;
	UINT8 i;

	if (grandprixinfo.cup != NULL && roundqueue.size > 0)
	{
		roundnum = roundqueue.roundnum;
		roundsleft = grandprixinfo.cup->numlevels - roundnum;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		if (players[i].score < lowestscore)
		{
			lowestscore = players[i].score;
		}

		if (players[i].bot == true && players[i].botvars.difficulty < lowestdifficulty)
		{
			lowestdifficulty = players[i].botvars.difficulty;
		}
	}

	// In the early game, difficulty is more important.
	// This will try to influence the higher difficulty bots to get rival more often & get even more points.
	// However, when we're running low on matches left, we need to focus more on raw score!

	return ((difficulty - lowestdifficulty) * roundsleft) + ((score - lowestscore) * roundnum);
}

static boolean CompareRivals(player_t *a, player_t *b)
{
	if (a == NULL)
		return false;
	if (b == NULL)
		return true;

	if (K_RivalScore(a) != K_RivalScore(b))
	{
		// Rival Score is HIGH when bots are strong. Sort them first!
		return (K_RivalScore(a) > K_RivalScore(b));
	}

	// Fuck it
	return a > b;
}

void K_AssignFoes(void)
{
	std::vector<player_t *> bots;
	boolean addedplayer = false;

	for (UINT8 i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == false)
			continue;

		player_t *player = &players[i];

		if (!player->spectator && player->bot)
		{
			addedplayer = true;
			bots.push_back(player);
			player->botvars.foe = false;
		}
	}

	// Why the fuck is this blowing up sometimes
	if (!addedplayer)
		return;

	std::stable_sort(bots.begin(), bots.end(), CompareRivals);

	UINT8 i = 0;
	for (auto &bot : bots)
	{
		if (bot != NULL)
			bot->botvars.foe = true;

		i++;
		if (i > 2)
			break;
	}
}

/*--------------------------------------------------
	void K_InitGrandPrixBots(void)

		See header file for description.
--------------------------------------------------*/
void K_InitGrandPrixBots(void)
{
	const UINT16 defaultbotskin = R_BotDefaultSkin();

	const UINT8 startingdifficulty = K_BotStartingDifficulty(grandprixinfo.gamespeed);
	UINT8 difficultylevels[MAXPLAYERS];

	UINT8 playercount = 8;
	UINT8 wantedbots = 0;

	UINT8 numplayers = 0;
	UINT8 competitors[MAXSPLITSCREENPLAYERS];

	UINT16 usableskins, skincount = (demo.playback ? demo.numskins : numskins);;
	UINT16 grabskins[MAXSKINS+1];

	UINT16 botskinlist[MAXPLAYERS];
	UINT8 botskinlistpos = 0;

	UINT8 newplayernum = 0;
	UINT16 i, j;

	memset(competitors, MAXPLAYERS, sizeof (competitors));
	memset(botskinlist, defaultbotskin, sizeof (botskinlist));

	// Init usable bot skins list
	for (usableskins = 0; usableskins < skincount; usableskins++)
	{
		grabskins[usableskins] = usableskins;
	}
	grabskins[usableskins] = MAXSKINS;

#if MAXPLAYERS != 16
	I_Error("GP bot difficulty levels need rebalanced for the new player count!\n");
#endif

	if (grandprixinfo.masterbots)
	{
		// Everyone is max difficulty!!
		memset(difficultylevels, MAXBOTDIFFICULTY, sizeof (difficultylevels));
	}
	else
	{
		// init difficulty levels list
		difficultylevels[ 0] = startingdifficulty;
		difficultylevels[ 1] = K_GetOffsetStartingDifficulty(startingdifficulty, 1);
		difficultylevels[ 2] = K_GetOffsetStartingDifficulty(startingdifficulty, 2);
		difficultylevels[ 3] = K_GetOffsetStartingDifficulty(startingdifficulty, 3);
		difficultylevels[ 4] = K_GetOffsetStartingDifficulty(startingdifficulty, 3);
		difficultylevels[ 5] = K_GetOffsetStartingDifficulty(startingdifficulty, 4);
		difficultylevels[ 6] = K_GetOffsetStartingDifficulty(startingdifficulty, 4);
		difficultylevels[ 7] = K_GetOffsetStartingDifficulty(startingdifficulty, 4);
		difficultylevels[ 8] = K_GetOffsetStartingDifficulty(startingdifficulty, 5);
		difficultylevels[ 9] = K_GetOffsetStartingDifficulty(startingdifficulty, 5);
		difficultylevels[10] = K_GetOffsetStartingDifficulty(startingdifficulty, 5);
		difficultylevels[11] = K_GetOffsetStartingDifficulty(startingdifficulty, 6);
		difficultylevels[12] = K_GetOffsetStartingDifficulty(startingdifficulty, 6);
		difficultylevels[13] = K_GetOffsetStartingDifficulty(startingdifficulty, 7);
		difficultylevels[14] = K_GetOffsetStartingDifficulty(startingdifficulty, 7);
		difficultylevels[15] = K_GetOffsetStartingDifficulty(startingdifficulty, 8);
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			if (players[i].bot == true)
			{
				// Remove existing bots.
				CL_RemovePlayer(i, KR_LEAVE);
				continue;
			}

			if (numplayers < MAXSPLITSCREENPLAYERS && !players[i].spectator)
			{
				competitors[numplayers] = i;
				grabskins[players[i].skin] = MAXSKINS;
				numplayers++;
			}
			else
			{
				P_SetPlayerSpectator(i); // force spectate for all other players, if they happen to exist?
			}
		}
	}

	playercount = K_GetGPPlayerCount(numplayers);
	wantedbots = playercount - numplayers;

	// Create rival list
	if (numplayers > 0)
	{
		for (i = 0; i < SKINRIVALS; i++)
		{
			for (j = 0; j < numplayers; j++)
			{
				player_t *p = &players[competitors[j]];
				const char *rivalname = skins[p->skin]->rivals[i];
				INT32 rivalnum = R_SkinAvailable(rivalname);

				// Intentionally referenced before (currently dummied out) unlock check. Such a tease!
				if (rivalnum != -1 && grabskins[(UINT16)rivalnum] != MAXSKINS)
				{
					botskinlist[botskinlistpos++] = (UINT16)rivalnum;
					grabskins[(UINT16)rivalnum] = MAXSKINS;
				}
			}
		}
	}

	// Rearrange usable bot skins list to prevent gaps for randomised selection
	for (i = 0; i < usableskins; i++)
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

	// Pad the remaining list with random skins if we need to
	if (botskinlistpos < wantedbots)
	{
		while (botskinlistpos < wantedbots)
		{
			UINT16 skinnum = defaultbotskin;

			if (usableskins > 0)
			{
				UINT16 index = P_RandomKey(PR_BOTS, usableskins);
				skinnum = grabskins[index];
				grabskins[index] = grabskins[--usableskins];
			}

			botskinlist[botskinlistpos++] = skinnum;
		}
	}

	for (i = 0; i < wantedbots; i++)
	{
		if (!K_AddBot(botskinlist[i], difficultylevels[i], BOT_STYLE_NORMAL, &newplayernum))
		{
			break;
		}
		if (i <= 2)
			players[newplayernum-1].botvars.foe = true;
	}
}

/*--------------------------------------------------
	void K_LoadGrandPrixSaveGame(void)

		See header file for description.
---------------------------------------------------*/

void K_LoadGrandPrixSaveGame(void)
{
	if (splitscreen)
	{
		// You're not doing splitscreen runs at GDQ.
		// We are literally 14 days from code freeze
		// and I am not accomodating weird setup this
		// second in my last minute QoL feature.
		// I will *actually* fight you
		return;
	}

	players[consoleplayer].lives = savedata.lives;
	players[consoleplayer].score = savedata.score;
	players[consoleplayer].totalring = savedata.totalring;

	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (savedata.bots[i].valid == false)
			continue;

		K_SetBot(i, savedata.bots[i].skin, savedata.bots[i].difficulty, BOT_STYLE_NORMAL);

		players[i].botvars.rival = savedata.bots[i].rival;
		players[i].botvars.foe = savedata.bots[i].foe;
		players[i].score = savedata.bots[i].score;

		players[i].spectator = K_BotDefaultSpectator();
	}
}

/*--------------------------------------------------
	void K_UpdateGrandPrixBots(void)

		See header file for description.
--------------------------------------------------*/
void K_UpdateGrandPrixBots(void)
{
	player_t *oldrival = NULL;
	player_t *newrival = NULL;
	UINT16 newrivalscore = 0;
	UINT8 i;

	if (K_PodiumSequence() == true)
	{
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || !players[i].bot)
		{
			continue;
		}

		players[i].spectator = K_BotDefaultSpectator();
	}

	if (grandprixinfo.wonround == false)
	{
		return;
	}

	K_AssignFoes();

	// Find the rival.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator || !players[i].bot)
		{
			continue;
		}

		if (players[i].botvars.diffincrease)
		{
			// CONS_Printf("in %d inc %d", players[i].botvars.difficulty, players[i].botvars.diffincrease);
			if (players[i].botvars.diffincrease < 0)
				players[i].botvars.difficulty = std::max(1, players[i].botvars.difficulty + players[i].botvars.diffincrease);
			else
				players[i].botvars.difficulty += players[i].botvars.diffincrease;

			if (players[i].botvars.difficulty > MAXBOTDIFFICULTY)
			{
				players[i].botvars.difficulty = MAXBOTDIFFICULTY;
			}

			players[i].botvars.diffincrease = 0;
			// CONS_Printf("out %d\n", players[i].botvars.difficulty);
		}

		if (players[i].botvars.rival)
		{
			if (oldrival == NULL)
			{
				// Record the old rival to compare with our calculated new rival
				oldrival = &players[i];
			}
			else
			{
				// Somehow 2 rivals were set?!
				// Let's quietly fix our mess...
				players[i].botvars.rival = false;
			}
		}
	}

	// Find the bot with the best average of score & difficulty.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		UINT16 ns = 0;

		if (!playeringame[i] || players[i].spectator || !players[i].bot)
		{
			continue;
		}

		ns = K_RivalScore(&players[i]);

		if (ns > newrivalscore)
		{
			newrival = &players[i];
			newrivalscore = ns;
		}
	}

	// Even if there's a new rival, we want to make sure that they're a better fit than the current one.
	if (oldrival != newrival)
	{
		if (oldrival != NULL)
		{
			UINT16 os = K_RivalScore(oldrival);

			if (newrivalscore < os + 5)
			{
				// This rival's only *slightly* better, no need to fire the old one.
				// Our current rival's just fine, thank you very much.
				return;
			}

			// Hand over your badge.
			oldrival->botvars.rival = false;
		}

		// Set our new rival!
		newrival->botvars.rival = true;
	}
}

/*--------------------------------------------------
	static UINT8 K_BotExpectedStanding(player_t *bot)

		Predicts what placement a bot was expected to be in.
		Used for determining if a bot's difficulty should raise.

	Input Arguments:-
		bot - Player to check.

	Return:-
		Position number the bot was expected to be in.
--------------------------------------------------*/
static UINT8 K_BotExpectedStanding(player_t *bot)
{
	UINT8 pos = 1;
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (i == (bot - players))
		{
			continue;
		}

		if (playeringame[i] && !players[i].spectator)
		{
			if (players[i].bot)
			{
				if (players[i].botvars.difficulty > bot->botvars.difficulty)
				{
					pos++;
				}
				else if (players[i].score > bot->score)
				{
					pos++;
				}
			}
			else
			{
				// Human player, always increment
				pos++;
			}
		}
	}

	return pos;
}

/*--------------------------------------------------
	void K_IncreaseBotDifficulty(player_t *bot)

		See header file for description.
--------------------------------------------------*/
void K_IncreaseBotDifficulty(player_t *bot)
{
	UINT8 playerCount = 0;
	UINT8 wonCount = 0;

	UINT8 humanThatBeatUs = 0;
	INT16 beatenDelta = 0;

	UINT8 winnerHuman = UINT8_MAX;
	INT16 winnerDelta = 0;

	UINT8 statusQuo = 1;
	INT16 disruptDelta = 0;

	INT16 increase = 1;
	size_t i = SIZE_MAX;

	bot->botvars.diffincrease = 0;

	if (grandprixinfo.masterbots == true)
	{
		// Master bot difficulty is not dynamic.
		return;
	}

	// Increment bot difficulty based on
	// how much they were beaten by a player!

	// Find the worst-placing player that still beat us.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *other = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		other = &players[i];
		if (other->spectator == true)
		{
			continue;
		}

		playerCount++;
		if (other->bot == true)
		{
			continue;
		}

		if (other->position <= bot->position && other->position > humanThatBeatUs)
		{
			humanThatBeatUs = other->position;
		}

		if (other->position < winnerHuman)
		{
			winnerHuman = other->position;
		}
	}

	wonCount = playerCount / 2;
	if (playerCount & 1)
	{
		// Round up
		wonCount++;
	}

	statusQuo = K_BotExpectedStanding(bot);

	// How many levels they gain depends on how hard they beat us,
	// and how much the status quo was disturbed.
	beatenDelta = bot->position - humanThatBeatUs;
	winnerDelta = wonCount - winnerHuman;
	disruptDelta = abs(statusQuo - bot->position);

	increase = (beatenDelta + winnerDelta + disruptDelta - 2) / 3;

	SINT8 rankNudge = 1; // Generally, we want bots to rank up at least once, but...

	// If humans are struggling, we want to back off, or even derank if it's dire.
	// Average human ranks to determine general bot "rank inertia".
	SINT8 totalRank = 0;
	SINT8 humanPlayers = 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *human = NULL;

		if (playeringame[i] == false)
			continue;

		human = &players[i];

		if (human->spectator == true)
			continue;

		if (human->bot == true)
			continue;

		humanPlayers++;

		totalRank += human->tally.rank;
	}

	if (humanPlayers)
	{
		SINT8 averageRank = totalRank / humanPlayers;

		// RELAXED MODE:
		// Continues don't drop bot difficulty, because we always advance.
		// Bots will still level up from standard advancement; we need a
		// much steeper rank nudge to keep difficulty at the right level.
		if (grandprixinfo.gamespeed == KARTSPEED_EASY)
		{
			switch (averageRank)
			{
				case GRADE_E:
					rankNudge = -4;
					break;
				case GRADE_D:
					rankNudge = -2;
					break;
				case GRADE_C:
					rankNudge = -1;
					break;
				case GRADE_B:
					rankNudge = 0;
					break;
				case GRADE_A:
					rankNudge = 1;
					break;
			}
		}
		else
		{
			switch (averageRank)
			{
				case GRADE_E:
				case GRADE_D:
				case GRADE_C:
					rankNudge = -1;
					break;
				case GRADE_B:
					rankNudge = 0;
					break;
				case GRADE_A:
					rankNudge = 1;
					break;
			}
		}
	}

	increase += rankNudge;

	if (increase <= 0)
	{
		// TYRON: We want to allow SMALL bot rank downs if a player gets rolled but still squeaks by.
		// (Think, like, dire E rank in 4th.)
		// This is a deviation from SalCode™ and should be reexamined if bots get drowsy.
	}

	bot->botvars.diffincrease = increase;
}

static boolean CompareJoiners(player_t *a, player_t *b)
{
	if (a->spectatorReentry != b->spectatorReentry)
	{
		// Push low re-entry cooldown to the back.
		return (a->spectatorReentry > b->spectatorReentry);
	}

	if (a->spectatewait != b->spectatewait)
	{
		// Push high waiting time to the back.
		return (a->spectatewait < b->spectatewait);
	}

	// Fuck it
	return a > b;
}

static boolean CompareReplacements(player_t *a, player_t *b)
{
	if ((a->pflags & PF_NOCONTEST) != (b->pflags & PF_NOCONTEST))
	{
		// Push NO CONTEST to the back.
		return ((a->pflags & PF_NOCONTEST) == 0);
	}

	if (a->position != b->position)
	{
		// Push bad position to the back.
		return (a->position < b->position);
	}

	// Fuck it
	return a > b;
}

/*--------------------------------------------------
	void K_RetireBots(void)

		See header file for description.
--------------------------------------------------*/
void K_RetireBots(void)
{
	UINT16 i;

	if (grandprixinfo.gp == true
		&& grandprixinfo.eventmode != GPEVENT_NONE)
	{
		// Bots can't do anything here, replacement means they never get their spotlight.
		return;
	}

	if (roundqueue.size > 0)
	{
		// Get the last non-special entry
		i = roundqueue.size;
		while (--i > roundqueue.position)
		{
			if (!roundqueue.entries[i].rankrestricted)
				break;
		}

		if (roundqueue.position >= i)
		{
			// Out of non-special entries, no replacement.
			return;
		}
	}

	// Duel Shuffle:
	// In games with limited player count, shuffle the NO CONTEST
	// player with a spectator that wants to play. Intended for 1v1
	// servers, but really this can help any server with a lower
	// player count than connection count.
	std::vector<player_t *> joining;
	std::vector<player_t *> humans;
	std::vector<player_t *> bots;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == false)
		{
			continue;
		}

		player_t *player = &players[i];

		if (player->spectator == true)
		{
			if (player->bot == false && (player->pflags & PF_WANTSTOJOIN) == PF_WANTSTOJOIN)
			{
				joining.push_back(player);
			}
		}
		else
		{
			if (player->bot == true)
			{
				bots.push_back(player);
			}
			else
			{
				humans.push_back(player);
			}
		}
	}

	const size_t max_lobby_size = static_cast<size_t>(cv_maxplayers.value);

	size_t num_joining = joining.size();
	size_t num_playing = humans.size() + bots.size();

	std::stable_sort(joining.begin(), joining.end(), CompareJoiners);
	std::stable_sort(humans.begin(), humans.end(), CompareReplacements);
	std::stable_sort(bots.begin(), bots.end(), CompareReplacements);

	// If a player spectated mid-race or mid-duel, they will be placed in-game by K_CheckSpectateStatus,
	// and their position will be set to 0. Since we're only replacing one player as of now, there's no need
	// to do anything; a player has already been replaced.
	bool player_already_joined = (!humans.empty() && humans[0]->position == 0);

	if (G_GametypeHasSpectators() == true && grandprixinfo.gp == false && !player_already_joined && cv_shuffleloser.value != 0)
	{
		// While joiners and players still exist, insert joiners.

		//UINT8 replacements = max_lobby_size / 2; // Only replace bottom half
		UINT8 replacements = 1; // Only replace a single player.

		while (replacements > 0)
		{
			if (joining.size() == 0 || (humans.size() + bots.size()) == 0)
			{
				// No one to replace or to join.
				break;
			}

			if (num_playing + num_joining <= max_lobby_size)
			{
				// We can fit everyone, so we don't need to do manual replacement.
				break;
			}

			player_t *joiner = joining.back();
			joining.pop_back();

			player_t *replace = nullptr;
			if (bots.size() > 0)
			{
				replace = bots.back();

				// A bot is taking up this slot? Remove it entirely.
				CL_RemovePlayer(replace - players, KR_LEAVE);

				bots.pop_back();
			}
			else
			{
				replace = humans.back();

				// A human is taking up this slot? Spectate them.
				replace->spectator = true;
				replace->pflags |= PF_WANTSTOJOIN; // We were are spectator against our will, we want to play ASAP.

				humans.pop_back();
			}

			// Add our waiting-to-play spectator to the game.
			P_SpectatorJoinGame(joiner);

			replacements--;
			num_joining--;
		}
	}

	// No need to run any of this code if no bots afoot
	if (bots.size() == 0)
	{
		return;
	}

	// Remove all still-in-contestors
	bots.erase(std::remove_if(bots.begin(), bots.end(),
		[](player_t *bot) {
			return (
				bot->spectator
				|| !(bot->pflags & PF_NOCONTEST)
			);
		}
	), bots.end());

	// No need to run any of this code if no *NO-CONTESTORS* afoot
	if (bots.size() == 0)
	{
		return;
	}

	// Okay, now this is essentially the original contents of K_RetireBots with cpp swag

	const UINT16 defaultbotskin = R_BotDefaultSkin();
	SINT8 newDifficulty;

	UINT16 usableskins, skincount = (demo.playback ? demo.numskins : numskins);
	UINT16 grabskins[MAXSKINS+1];

	// Handle adjusting difficulty for new bots
	{
		if (grandprixinfo.gp) // Sure, let's let this happen all the time :)
		{
			if (grandprixinfo.masterbots == true)
			{
				newDifficulty = MAXBOTDIFFICULTY;
			}
			else
			{
				const UINT8 startingdifficulty = K_BotStartingDifficulty(grandprixinfo.gamespeed);
				newDifficulty = startingdifficulty - 4;
				if (roundqueue.size > 0)
				{
					newDifficulty += roundqueue.roundnum;
				}
			}
		}
		else
		{
			newDifficulty = cv_kartbot.value;
		}

		if (newDifficulty > MAXBOTDIFFICULTY)
		{
			newDifficulty = MAXBOTDIFFICULTY;
		}
		else if (newDifficulty < 1)
		{
			newDifficulty = 1;
		}
	}

	// HACK!!!!! that survived allll the way since two days to end of cleanup period :)
	// we do this so that any bot that's been removed doesn't count for K_SetNameForBot conflicts
	for (player_t *bot : bots)
	{
		player_names[(bot-players)][0] = '0';
	}

	// Init usable bot skins list
	for (usableskins = 0; usableskins < skincount; usableskins++)
	{
		grabskins[usableskins] = usableskins;
	}
	grabskins[usableskins] = MAXSKINS;

	// Exclude player skins
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		if (players[i].spectator)
			continue;

		grabskins[players[i].skin] = MAXSKINS;
	}

	// Rearrange usable bot skins list to prevent gaps for randomised selection
	for (i = 0; i < usableskins; i++)
	{
		if (!(grabskins[i] == MAXSKINS || !R_SkinUsable(-1, grabskins[i], true)))
			continue;
		while (usableskins > i && (grabskins[usableskins] == MAXSKINS || !R_SkinUsable(-1, grabskins[usableskins], true)))
			usableskins--;
		grabskins[i] = grabskins[usableskins];
		grabskins[usableskins] = MAXSKINS;
	}

	// Replace nocontested bots.
	for (player_t *bot : bots)
	{
		UINT16 skinnum = defaultbotskin;

		if (usableskins > 0)
		{
			UINT16 index = P_RandomKey(PR_BOTS, usableskins);
			skinnum = grabskins[index];
			grabskins[index] = grabskins[--usableskins];
		}

		memcpy(&bot->availabilities, R_GetSkinAvailabilities(false, skinnum), MAXAVAILABILITY*sizeof(UINT8));

		bot->botvars.difficulty = newDifficulty;
		bot->botvars.diffincrease = 0;

		K_SetNameForBot(bot - players, skins[skinnum]->realname);

		bot->prefskin = skinnum;
		bot->prefcolor = skins[skinnum]->prefcolor;
		bot->preffollower = -1;
		bot->preffollowercolor = SKINCOLOR_NONE;
		G_UpdatePlayerPreferences(bot);

		bot->score = 0;
		LUA_HookPlayer(bot, HOOK(BotJoin));
		bot->pflags &= ~PF_NOCONTEST;
	}
}

/*--------------------------------------------------
	void K_FakeBotResults(player_t *bot)

		See header file for description.
--------------------------------------------------*/
void K_FakeBotResults(player_t *bot)
{
	const UINT32 distfactor = FixedMul(32 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed)) / FRACUNIT;
	UINT32 worstdist = 0;
	tic_t besttime = UINT32_MAX;
	UINT8 numplayers = 0;
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
		{
			numplayers++;

			if (players[i].exiting && players[i].realtime < besttime)
			{
				besttime = players[i].realtime;
			}

			if (players[i].distancetofinish > worstdist)
			{
				worstdist = players[i].distancetofinish;
			}
		}
	}

	if (besttime == UINT32_MAX) // No one finished, so you don't finish either.
	{
		// We don't apply PF_NOCONTEST in the exitlevel case - that's done for all players in G_DoCompleted.
		return;
	}

	if (bot->distancetofinish >= worstdist) // Last place, you aren't going to finish.
	{
		// This was a successful murder!
		bot->pflags |= PF_NOCONTEST;
		return;
	}

	// hey, you "won"
	bot->exiting = 1;
	bot->realtime += (bot->distancetofinish / distfactor);
	bot->distancetofinish = 0;
	K_IncreaseBotDifficulty(bot);
}

/*--------------------------------------------------
	void K_PlayerLoseLife(player_t *player)

		See header file for description.
--------------------------------------------------*/
void K_PlayerLoseLife(player_t *player)
{
	if (!G_GametypeUsesLives() || !G_GametypeAllowsRetrying())
	{
		return;
	}

	if (player->spectator || player->bot || player->lives <= 0 || (player->pflags & PF_LOSTLIFE))
	{
		return;
	}

	player->lives--;
	player->pflags |= PF_LOSTLIFE;
}

/*--------------------------------------------------
	boolean K_CanChangeRules(boolean allowdemos)

		See header file for description.
--------------------------------------------------*/
boolean K_CanChangeRules(boolean allowdemos)
{
	if (grandprixinfo.gp == true)
	{
		// Don't cheat the rules of the GP!
		return false;
	}

	if (marathonmode)
	{
		// Don't cheat the endurance challenge!
		return false;
	}

	if (modeattacking != ATTACKING_NONE)
	{
		// Don't cheat the rules of Time Trials!
		return false;
	}

	if (gametype == GT_TUTORIAL || tutorialchallenge == TUTORIALSKIP_INPROGRESS)
	{
		// Tutorials are locked down.
		return false;
	}

	if (!allowdemos && demo.playback)
	{
		// We've already got our important settings!
		return false;
	}

	return true;
}

extern "C" consvar_t cv_forcebots;

/*--------------------------------------------------
	boolean K_BotDefaultSpectator(player_t *player);

		See header file for description.
--------------------------------------------------*/
boolean K_BotDefaultSpectator(void)
{
	if (cv_forcebots.value)
	{
		return false;
	}

	if (!(gametyperules & GTR_BOTS))
	{
		// This gametype does not support bots.
		return true;
	}

	if (grandprixinfo.eventmode != GPEVENT_NONE)
	{
		// This is a special round of GP, so bots must spectate.
		return true;
	}

	return false;
}
