// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
// \brief Power Level system

#include "k_pwrlv.h"
#include "d_netcmd.h"
#include "g_game.h"
#include "s_sound.h"
#include "m_random.h"
#include "m_cond.h" // M_UpdateUnlockablesAndExtraEmblems
#include "p_tick.h" // leveltime
#include "k_grandprix.h"
#include "k_profiles.h"
#include "k_serverstats.h"
#include "k_kart.h" // K_InRaceDuel

// Client-sided calculations done for Power Levels.
// This is done so that clients will never be able to hack someone else's score over the server.
UINT16 clientpowerlevels[MAXPLAYERS][PWRLV_NUMTYPES];

// Total calculated power add during the match,
// totalled at the end of the round.
INT16 clientPowerAdd[MAXPLAYERS];

// Players who spectated mid-race
UINT8 spectateGriefed = 0;

// Game setting scrambles based on server Power Level
SINT8 speedscramble = -1;
SINT8 encorescramble = -1;

SINT8 K_UsingPowerLevels(void)
{
	if (!cv_kartusepwrlv.value)
	{
		// Explicitly forbidden.
		return PWRLV_DISABLED;
	}

	if (!(netgame || (demo.playback && demo.netgame)))
	{
		// Servers only.
		return PWRLV_DISABLED;
	}

	if (roundqueue.size > 0 && roundqueue.position > 0)
	{
		// When explicit progression is in place, we're going by different rules.
		return PWRLV_DISABLED;
	}

	if (gametype == GT_RACE)
	{
		// Race PWR.
		return PWRLV_RACE;
	}

	if (gametype == GT_BATTLE)
	{
		// Battle PWR.
		return PWRLV_BATTLE;
	}

	// We do not support PWR for custom gametypes at this moment in time.
	return PWRLV_DISABLED;
}

void K_ClearClientPowerLevels(void)
{
	memset(clientpowerlevels, 0, sizeof clientpowerlevels);
	memset(clientPowerAdd, 0, sizeof clientPowerAdd);
}

// "Tyron sneaks mahjong ratings into Ring Racers"
// A system that allows player ratings to inflate over time, then settle based on their winrate.
static fixed_t K_CalculatePowerLevelInc(UINT16 you, UINT16 them, boolean won)
{
	// == ★ TUNING ZONE ★ ==

	fixed_t BASE_CHANGE = 20*FRACUNIT; // The base amount that ratings should change per comparison. Higher = more volatile

	INT16 STABLE_RATE = 4000; // The fulcrum point between positive-sum and even rankings.
	INT16 CEILING_RATE = 7000; // The fulcrum point between even and negative-sum rankings.

	// % modifiers to gains and losses. Positive numbers mean you gain more when gaining and drain more when draining.
	// Negative numbers mean changes are less volatile; this makes gains less powerful and drains less punishing.
	// "Strong" players are above STABLE_RATE. "Weak" players are below STABLE_RATE.
	fixed_t STRONG_GAIN_PER_K = 	-20*FRACUNIT/100; // How much to modify gains per 1000 points above stable.
	fixed_t STRONG_DRAIN_PER_K = 	20*FRACUNIT/100; // How much to modify losses per 1000 points above stable.
	fixed_t WEAK_GAIN_PER_K = 		20*FRACUNIT/100; // How much to modify gains per 1000 points BELOW stable.
	fixed_t WEAK_DRAIN_PER_K = 		-20*FRACUNIT/100; // How much to modify losses per 1000 points BELOW stable.

	fixed_t GAP_INFLUENCE_PER_K = 	20*FRACUNIT/100; // How much to modify changes per 1000 point rating gap between participants.
	// This affects gains for the weaker player and drains for the stronger player, to reward upsets / reassert the order.

	// == Derived helper vars ==

	INT16 STABLE_DELTA = 0;

	// Positive deltas if your rating is deflationary, negative if you're inflationary.
	if (you < STABLE_RATE)
		STABLE_DELTA = you - STABLE_RATE;
	else if (you > CEILING_RATE)
		STABLE_DELTA = you - CEILING_RATE;

	INT16 ABS_STABLE_DELTA = abs(STABLE_DELTA);

	INT16 RATING_GAP = you - them;
	INT16 ABS_RATING_GAP = abs(RATING_GAP);

	fixed_t GAP_INFLUENCE = GAP_INFLUENCE_PER_K / 1000 * ABS_RATING_GAP;

	// == Working vars ==

	fixed_t change = won ? BASE_CHANGE : -1 * BASE_CHANGE; // The rating change to eventually apply.
	fixed_t gainMod = FRACUNIT; // Multiplier to winnings (change > 0).
	fixed_t drainMod = FRACUNIT; // Multiplier to lost rating (change < 0).
	fixed_t gainPerK = 0;
	fixed_t lossPerK = 0;

	// == ★ FIXED POINT MATH ZONE ★ ==

	if (STABLE_DELTA > 0)
	{
		// Use strong-player modifiers.
		gainPerK = STRONG_GAIN_PER_K;
		lossPerK = STRONG_DRAIN_PER_K;
	}
	else if (STABLE_DELTA < 0)
	{
		// Use weak-player modifiers.
		gainPerK = WEAK_GAIN_PER_K;
		lossPerK = WEAK_DRAIN_PER_K;
	}

	// Apply rating-based modifiers: divide by 1000 so we're in the "right units"
	// to tune with "per 1000" numbers up top.
	gainMod += gainPerK / 1000 * ABS_STABLE_DELTA;
	drainMod += lossPerK / 1000 * ABS_STABLE_DELTA;

	// EXTRA: Weak players gain more versus strong players and vice versa.
	if (RATING_GAP > 0)
	{
		// You're strong. You lose more when losing.
		drainMod += GAP_INFLUENCE;
	}
	else if (RATING_GAP < 0)
	{
		// You're the underdog. You win more when winning.
		gainMod += GAP_INFLUENCE;
	}

	// Keep negative values etc from causing havoc.
	gainMod = clamp(gainMod, FRACUNIT/10, FRACUNIT*10);
	drainMod = clamp(drainMod, FRACUNIT/10, FRACUNIT*10);

	// Winning? Apply gain mod. Losing? Apply drain mod.
	if (change > 0)
	{
		change = FixedMul(change, gainMod);
	}
	else if (change < 0)
	{
		change = FixedMul(change, drainMod);
	}

	CONS_Debug(DBG_PWRLV, "R1=%d R2=%d W=%d - G=%d D=%d - C=%d\n", you, them, won, gainMod, drainMod, change/FRACUNIT);

	return change;
}

INT16 K_PowerLevelPlacementScore(player_t *player)
{
	if ((player->pflags & PF_NOCONTEST) || (player->spectator))
	{
		return 0;
	}

	if (gametyperules & GTR_CIRCUIT)
	{
		return MAXPLAYERS - player->position;
	}
	else
	{
		return player->roundscore;
	}
}

INT16 K_CalculatePowerLevelAvg(void)
{
	INT32 avg = 0;
	UINT8 div = 0;
	SINT8 t = PWRLV_DISABLED;
	UINT8 i;

	if (!netgame || !cv_kartusepwrlv.value)
	{
		CONS_Debug(DBG_PWRLV, "Not in a netgame, or not using power levels -- no average.\n");
		return 0; // No average.
	}

	if ((gametyperules & GTR_CIRCUIT))
		t = PWRLV_RACE;
	else if ((gametyperules & GTR_BUMPERS))
		t = PWRLV_BATTLE;

	if (t == PWRLV_DISABLED)
	{
		CONS_Debug(DBG_PWRLV, "Could not set a power level type -- no average.\n");
		return 0; // Hmm?!
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator
			|| clientpowerlevels[i][t] == 0) // splitscreen player
			continue;

		avg += clientpowerlevels[i][t];
		div++;
	}

	if (!div)
	{
		CONS_Debug(DBG_PWRLV, "Found no players -- no average.\n");
		return 0; // No average.
	}

	avg /= div;

	return (INT16)avg;
}

void K_UpdatePowerLevels(player_t *player, UINT8 gradingpoint, boolean forfeit)
{
	const UINT8 playerNum = player - players;
	const boolean exitBonus = ((gradingpoint >= K_GetNumGradingPoints()) || (player->pflags & PF_NOCONTEST));

	SINT8 powerType = K_UsingPowerLevels();

	INT16 yourScore = 0;
	UINT16 yourPower = 0;

	UINT8 i;

	// Compare every single player against each other for power level increases.
	// Every player you won against gives you more points, and vice versa.
	// The amount of points won per match-up depends on the difference between the loser's power and the winner's power.
	// See K_CalculatePowerLevelInc for more info.

	if (powerType == PWRLV_DISABLED)
	{
		return;
	}

	// Spectators don't have immunity if they're forfeiting: we're TRYING to punish them!
	if (!playeringame[playerNum] || (player->spectator && !forfeit))
	{
		return;
	}

	// Probably being called from some stray codepath or a double exit.
	// We have already finished calculating PWR, don't touch anything!
	if (player->finalized)
	{
		return;
	}

	CONS_Debug(DBG_PWRLV, "\n========\n");
	CONS_Debug(DBG_PWRLV, "* Power Level change for player %s (CHECKPOINT %d) *\n", player_names[playerNum], gradingpoint);
	CONS_Debug(DBG_PWRLV, "========\n");

	yourPower = clientpowerlevels[playerNum][powerType];

	if (K_InRaceDuel())
		yourPower += clientPowerAdd[playerNum];

	if (yourPower == 0)
	{
		// Guests don't record power level changes.
		return;
	}

	CONS_Debug(DBG_PWRLV, "%s's PWR.LV: %d\n", player_names[playerNum], yourPower);

	yourScore = K_PowerLevelPlacementScore(player);
	CONS_Debug(DBG_PWRLV, "%s's gametype score: %d\n", player_names[playerNum], yourScore);

	CONS_Debug(DBG_PWRLV, "========\n");

	boolean dueling = K_InRaceDuel();
	for (i = 0; i < MAXPLAYERS; i++)
	{
		UINT16 theirScore = 0;
		INT16 theirPower = 0;

		fixed_t ourinc = 0; // Total pt increment
		fixed_t theirinc = 0;

		boolean won = false;

		if (i == playerNum) // Same person
		{
			continue;
		}

		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		if (G_SameTeam(player, &players[i]) == true)
		{
			// You don't win/lose against your teammates.
			continue;
		}

		CONS_Debug(DBG_PWRLV, "%s VS %s:\n", player_names[playerNum], player_names[i]);

		theirPower = clientpowerlevels[i][powerType];
		if (K_InRaceDuel())
			theirPower += clientPowerAdd[i];

		if (theirPower == 0)
		{
			// No power level (splitscreen guests, bots)
			continue;
		}

		CONS_Debug(DBG_PWRLV, "%s's PWR.LV: %d\n", player_names[i], theirPower);

		if (forfeit == true)
		{
			ourinc = K_CalculatePowerLevelInc(yourPower, theirPower, false);
			theirinc = K_CalculatePowerLevelInc(theirPower, yourPower, true);
			CONS_Debug(DBG_PWRLV, "FORFEIT! increment is %d\n", ourinc);
		}
		else
		{
			theirScore = K_PowerLevelPlacementScore(&players[i]);
			CONS_Debug(DBG_PWRLV, "%s's gametype score: %d\n", player_names[i], theirScore);

			if (yourScore == theirScore && forfeit == false) // Tie -- neither get any points for this match up.
			{
				CONS_Debug(DBG_PWRLV, "TIE, no change.\n");
				continue;
			}

			won = (yourScore > theirScore);

			if (won == true && forfeit == false) // This player won!
			{
				ourinc = K_CalculatePowerLevelInc(yourPower, theirPower, true);
				theirinc = K_CalculatePowerLevelInc(theirPower, yourPower, false);
				CONS_Debug(DBG_PWRLV, "WON! increment is %d\n", ourinc);
			}
			else // This player lost...
			{
				ourinc = K_CalculatePowerLevelInc(yourPower, theirPower, false);
				theirinc = K_CalculatePowerLevelInc(theirPower, yourPower, true);
				CONS_Debug(DBG_PWRLV, "LOST... increment is %d\n", ourinc);
			}
		}

		if (dueling)
		{
			fixed_t prevInc = ourinc;

			// INT32 winnerscore = (yourScore > theirScore) ? player->duelscore : players[i].duelscore;
			INT16 multiplier = 2;
			ourinc *= multiplier;
			theirinc *= multiplier;

			// CONS_Printf("%s PWR UPDATE: %d\n", player_names[player - players], inc);

			CONS_Debug(DBG_PWRLV, "DUELING: Boosted (%d * %d = %d)\n", prevInc/FRACUNIT, multiplier, ourinc/FRACUNIT);
		}
		else
		{
			fixed_t prevInc = ourinc;

			INT16 dvs = max(K_GetNumGradingPoints(), 1);
			ourinc = FixedDiv(ourinc, dvs*FRACUNIT);
			theirinc = FixedDiv(theirinc, dvs*FRACUNIT);

			if (exitBonus)
			{
				ourinc = FixedMul(ourinc, FRACUNIT + K_FinalCheckpointPower());
				theirinc = FixedMul(theirinc, FRACUNIT + K_FinalCheckpointPower());
				CONS_Debug(DBG_PWRLV, "Final check bonus (%d / %d * %d = %d)\n", prevInc/FRACUNIT, dvs, K_FinalCheckpointPower(), ourinc/FRACUNIT);
			}
			else
			{
				CONS_Debug(DBG_PWRLV, "Reduced (%d / %d = %d) because it's not the end of the race\n", prevInc/FRACUNIT, dvs, ourinc/FRACUNIT);
			}
		}

		CONS_Debug(DBG_PWRLV, "========\n");

		CONS_Debug(DBG_PWRLV, "Total Result:\n");
		CONS_Debug(DBG_PWRLV, "Our increment: %d\n", ourinc / FRACUNIT);
		CONS_Debug(DBG_PWRLV, "Their increment: %d\n", theirinc / FRACUNIT);

		CONS_Debug(DBG_PWRLV, "%s current: %d\n", player_names[playerNum], clientPowerAdd[playerNum]);
		clientPowerAdd[playerNum] += ourinc/FRACUNIT;
		CONS_Debug(DBG_PWRLV, "%s final: %d\n", player_names[playerNum], clientPowerAdd[playerNum]);

		CONS_Debug(DBG_PWRLV, "%s current: %d\n", player_names[i], clientPowerAdd[i]);
		clientPowerAdd[i] += theirinc/FRACUNIT;
		CONS_Debug(DBG_PWRLV, "%s final: %d\n", player_names[i], clientPowerAdd[i]);

		CONS_Debug(DBG_PWRLV, "========\n");
	}
}

void K_UpdatePowerLevelsFinalize(player_t *player, boolean onForfeit)
{
	if (player->finalized)
		return;

	// Finalize power level increments for any checkpoints not yet calculated.
	// For spectate / quit / NO CONTEST
	INT16 checksleft = 0;
	UINT8 i;

	// No remaining laps in Duel.
	if (K_InRaceDuel())
		return;

	checksleft = K_GetNumGradingPoints() - player->gradingpointnum;

	if (checksleft <= 0)
	{
		if (!(gametyperules & GTR_CHECKPOINTS)) // We should probably do at least _one_ PWR update.
			K_UpdatePowerLevels(player, player->gradingpointnum, onForfeit);
		// We've done every checkpoint already.
		return;
	}

	for (i = 1; i <= checksleft; i++)
	{
		K_UpdatePowerLevels(player, player->gradingpointnum + i, onForfeit);
	}

	player->finalized = true;
}

INT16 K_FinalPowerIncrement(player_t *player, INT16 yourPower, INT16 baseInc)
{
	INT16 inc = baseInc;
	UINT8 numPlayers = 0;
	UINT8 i;

	if (yourPower == 0)
	{
		// Guests don't record power level changes.
		return 0;
	}

	SINT8 powerType = K_UsingPowerLevels();
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		INT16 theirPower = clientpowerlevels[i][powerType];
		if (theirPower == 0)
		{
			// Don't count guests or bots.
			continue;
		}

		numPlayers++;
	}

	if (inc <= 0)
	{
		if (player->position == 1 && numPlayers > 1 && !(K_InRaceDuel()))
		{
			// Won the whole match?
			// Get at least one point.
			inc = 1;
		}
#if 0
		else
		{
			// You trade points in 1v1s,
			// but is more lenient in bigger lobbies.
			inc /= max(1, numPlayers-1);

			if (inc == 0)
			{
				if (baseInc > 0)
				{
					inc = 1;
				}
				else if (baseInc < 0)
				{
					inc = -1;
				}
			}
		}
#endif
	}

	if (yourPower + inc > PWRLVRECORD_MAX)
	{
		inc -= ((yourPower + inc) - PWRLVRECORD_MAX);
	}

	if (yourPower + inc < PWRLVRECORD_MIN)
	{
		inc -= ((yourPower + inc) - PWRLVRECORD_MIN);
	}

	return inc;
}

void K_CashInPowerLevels(void)
{
	SINT8 powerType = K_UsingPowerLevels();
	UINT8 i;

	CONS_Debug(DBG_PWRLV, "\n========\n");
	CONS_Debug(DBG_PWRLV, "Cashing in power level changes...\n");
	CONS_Debug(DBG_PWRLV, "========\n");

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == true && powerType != PWRLV_DISABLED)
		{
			INT16 inc = K_FinalPowerIncrement(&players[i], clientpowerlevels[i][powerType], clientPowerAdd[i]);

			clientpowerlevels[i][powerType] += inc;

			CONS_Debug(DBG_PWRLV, "%s: %d -> %d (%d)\n", player_names[i], clientpowerlevels[i][powerType] - inc, clientpowerlevels[i][powerType], inc);
		}

		clientPowerAdd[i] = 0;
	}

	SV_UpdateStats();

	CONS_Debug(DBG_PWRLV, "========\n");
}

void K_SetPowerLevelScrambles(SINT8 powertype)
{
	switch (powertype)
	{
		case PWRLV_RACE:
			if (cv_kartspeed.value == -1 || cv_kartencore.value == -1)
			{
				UINT8 speed = KARTSPEED_EASY;
				boolean encore = false;
				INT16 avg = 0, min = 0;
				UINT8 i, t = 1;

				avg = K_CalculatePowerLevelAvg();

				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (!playeringame[i] || players[i].spectator
						|| clientpowerlevels[i][t] == 0) // splitscreen player
						continue;

					if (min == 0 || clientpowerlevels[i][0] < min)
						min = clientpowerlevels[i][0];
				}

				CONS_Debug(DBG_GAMELOGIC, "Min: %d, Avg: %d\n", min, avg);

				if (avg == 0 || min == 0)
				{
					CONS_Debug(DBG_GAMELOGIC, "No average/minimum, no scramblin'.\n");
					speedscramble = encorescramble = -1;
					return;
				}

					if (avg >= 9500) // 3am 1v1-ers
						t = 6;

					else if (avg >= 9000) // Unemployed
						t = 5;

					else if (avg >= 7000) // Sweaty strangers
						t = 4;

					else if (avg >= 6500) // Experienced, lets see something interesting
						t = 3;

					else if (avg >= 4000) // Getting into it, likely experienced but just building power
						t = 2;

					else if (avg < 3300 || (avg <= 4000 && min < 2000)) // Casual group, mandatory first impressions; or if mostly new & 1 guy is really coping
						t = 0;

					else if (avg >= 3300) // Transition point
						t = 1;


				CONS_Debug(DBG_GAMELOGIC, "Table position: %d\n", t);

				switch (t)
				{
					case 6:
						speed = KARTSPEED_HARD;
						encore = true;
						break;
					case 5:
						speed = P_RandomChance(PR_RULESCRAMBLE, (7<<FRACBITS)/10) ? KARTSPEED_HARD : KARTSPEED_NORMAL;
						encore = P_RandomChance(PR_RULESCRAMBLE, FRACUNIT>>1);
						break;
					case 4:
						speed = P_RandomChance(PR_RULESCRAMBLE, (3<<FRACBITS)/10) ? KARTSPEED_HARD : KARTSPEED_NORMAL;
						encore = P_RandomChance(PR_RULESCRAMBLE, FRACUNIT>>2);
						break;
					case 3:
						speed = KARTSPEED_NORMAL;
						encore = P_RandomChance(PR_RULESCRAMBLE, FRACUNIT>>3);
						break;
					case 2:
						speed = P_RandomChance(PR_RULESCRAMBLE, (3<<FRACBITS)/10) ? KARTSPEED_NORMAL : KARTSPEED_EASY;
						encore = P_RandomChance(PR_RULESCRAMBLE, FRACUNIT>>5);
						break;
					case 1: default:
						speed = KARTSPEED_EASY;
						encore = false;
						break;
					case 0:
						speed = KARTSPEED_EASY;
						encore = false;
						break;
				}

				CONS_Debug(DBG_GAMELOGIC, "Rolled speed: %d\n", speed);
				CONS_Debug(DBG_GAMELOGIC, "Rolled encore: %s\n", (encore ? "true" : "false"));

				if (cv_kartspeed.value == KARTSPEED_AUTO)
					speedscramble = speed;
				else
					speedscramble = -1;

				if (cv_debugencorevote.value)
					encorescramble = 1;
				else if (cv_kartencore.value == -1)
					encorescramble = (encore ? 1 : 0);
				else
					encorescramble = -1;
			}
			break;
		default:
			break;
	}
}

void K_PlayerForfeit(UINT8 playerNum, boolean pointLoss)
{
	UINT8 p = 0;

	SINT8 powerType = PWRLV_DISABLED;
	UINT16 yourPower = 0;
	INT16 inc = 0;

	UINT8 i;

	// power level & spectating is netgames only
	if (!netgame)
	{
		return;
	}

	// Hey, I just got here!
	if (players[playerNum].jointime <= 1)
	{
		return;
	}

	// 20 sec into a match counts as a forfeit -- automatic loss against every other player in the match.
	if (gamestate != GS_LEVEL || leveltime <= starttime+(20*TICRATE))
	{
		return;
	}

	spectateGriefed++;

	// This server isn't using power levels, so don't mess with them.
	if (!cv_kartusepwrlv.value)
	{
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if ((playeringame[i] && !players[i].spectator)
			|| (i == playerNum))
		{
			p++;
		}
	}

	if (p < 2) // no players
	{
		return;
	}

	powerType = K_UsingPowerLevels();

	if (powerType == PWRLV_DISABLED) // No power type?!
	{
		return;
	}

	yourPower = clientpowerlevels[playerNum][powerType];
	if (yourPower == 0) // splitscreen guests don't record power level changes
	{
		return;
	}

	K_UpdatePowerLevelsFinalize(&players[playerNum], true);
	inc = K_FinalPowerIncrement(&players[playerNum], yourPower, clientPowerAdd[playerNum]);

	if (inc == 0)
	{
		// No change
		return;
	}

	if (pointLoss)
	{
		clientpowerlevels[playerNum][powerType] += inc;
		clientPowerAdd[playerNum] = 0;
		SV_UpdateStats();
	}
}
