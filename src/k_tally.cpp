// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) by Sally "TehRealSalt" Cochenour
// Copyright (C) by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_tally.cpp
/// \brief End of level tally screen animations

#include <vector>

#include "k_tally.h"

#include "k_kart.h"
#include "k_rank.h"
#include "k_grandprix.h"
#include "k_battle.h"
#include "k_boss.h"
#include "k_specialstage.h"
#include "k_hud.h"
#include "doomstat.h"
#include "g_game.h"
#include "p_local.h"
#include "r_main.h"
#include "r_skins.h"
#include "v_video.h"
#include "v_draw.hpp"
#include "z_zone.h"
#include "y_inter.h"
#include "m_easing.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "r_fps.h"
#include "g_party.h"

boolean level_tally_t::UseBonuses(void)
{
	if ((gametyperules & GTR_SPECIALSTART) || (grandprixinfo.gp == true && grandprixinfo.eventmode == GPEVENT_SPECIAL))
	{
		// Special Stage -- the only bonus for these is completing it or not.
		return false;
	}

	// No bonuses / ranking in FREE PLAY or Time Attack
	return (grandprixinfo.gp == true || K_TimeAttackRules() == false);
}

void level_tally_t::DetermineBonuses(void)
{
	std::vector<tally_bonus_e> temp_bonuses;

	// Figure out a set of two bonuses to use
	// for this gametype's ranking, depending
	// on what rules are activated and how important
	// they are.

	// The end of the vector is prioritized
	// more than the beginning of the vector,
	// so this is basically ranked choice
	// starting from least important to
	// most important.

	if (UseBonuses() == true)
	{
		if (grandprixinfo.gp == true && (gametypes[gt]->rules & GTR_SPHERES) == 0)
		{
			// Ring is a throw-away bonus, just meant to
			// encourage people to earn more lives,
			// so only have it in GP and with lowest priority.
			temp_bonuses.push_back(TALLY_BONUS_RING);
		}

		if (totalPrisons == 0) // These are only for regular Battle, not Prison Break
		{
			if ((gametypes[gt]->rules & GTR_POWERSTONES) == GTR_POWERSTONES)
			{
				// Give a consolation bonus for people who
				// almost won by getting all Chaos Emeralds.
				temp_bonuses.push_back(TALLY_BONUS_POWERSTONES);
			}

			if ((gametypes[gt]->rules & GTR_POINTLIMIT) == GTR_POINTLIMIT)
			{
				// Give a consolation bonus for getting
				// close to the point limit.
				temp_bonuses.push_back(TALLY_BONUS_SCORE);
			}
		}

		if (totalLaps > 0)
		{
			// Give circuit gamemodes a consolation bonus
			// for getting good placements on each lap.
			temp_bonuses.push_back(TALLY_BONUS_LAP);
		}

		if (totalPrisons > 0)
		{
			// If prisons exist, then this means that it is
			// the entire point of the mode, so make it
			// the most important.
			temp_bonuses.push_back(TALLY_BONUS_PRISON);
		}
	}

	// Take the two most important, and put them in our actual list.
	for (int i = 0; i < TALLY_WINDOW_SIZE; i++)
	{
		if (temp_bonuses.empty() == true)
		{
			bonuses[i] = TALLY_BONUS_NA; // No bonus to add...
		}
		else
		{
			bonuses[i] = temp_bonuses.back();
			temp_bonuses.pop_back();

#if 0
			switch (bonuses[i])
			{
				default:
					displayBonus[i] = 0;
					break;
			}
#else
			displayBonus[i] = 0;
#endif
		}
	}
}

void level_tally_t::DetermineStatistics(void)
{
	std::vector<tally_stat_e> temp_stats;

	// Same thing as DetermineBonuses, but for the
	// for-fun stats that don't do anything.

	if (grandprixinfo.gp == true)
	{
		// Show the player's total rings.
		// This one is special and also
		// shows lives gained.
		temp_stats.push_back(TALLY_STAT_TOTALRINGS);
	}

	// Maybe there will be a situation in the
	// future where we DON'T want to show time?
	temp_stats.push_back(TALLY_STAT_TIME);

	// Take the two most important, and put them in our actual list.
	for (int i = 0; i < TALLY_WINDOW_SIZE; i++)
	{
		if (temp_stats.empty() == true)
		{
			stats[i] = TALLY_STAT_NA; // No stat to add...
		}
		else
		{
			stats[i] = temp_stats.back();
			temp_stats.pop_back();

			switch (stats[i])
			{
				case TALLY_STAT_TOTALRINGS:
					displayStat[i] = ringPool;
					break;
				default:
					displayStat[i] = 0;
					break;
			}
		}
	}
}

INT32 level_tally_t::CalculateGrade(void)
{
	static const fixed_t gradePercents[GRADE_A] = {
		 7*FRACUNIT/20,		// D: 35% or higher
		10*FRACUNIT/20,		// C: 50% or higher
		14*FRACUNIT/20,		// B: 70% or higher
		17*FRACUNIT/20		// A: 85% or higher
	};
	INT32 retGrade = GRADE_E; // gp_rank_e

	INT32 bonusWeights[TALLY_WINDOW_SIZE];
	for (int i = 0; i < TALLY_WINDOW_SIZE; i++)
	{
		switch (bonuses[i])
		{
			case TALLY_BONUS_RING:
			{
				bonusWeights[i] = 20;
				break;
			}
			case TALLY_BONUS_SCORE:
			{
				bonusWeights[i] = ((pointLimit != 0) ? 100 : 0);
				break;
			}
			case TALLY_BONUS_LAP:
			case TALLY_BONUS_PRISON:
			case TALLY_BONUS_POWERSTONES:
			{
				bonusWeights[i] = 150;
				break;
			}
			default:
			{
				bonusWeights[i] = 0;
				break;
			}
		}
	}

	const INT32 positionWeight = (position > 0 && numPlayers > 2) ? 50 : 0;
	const INT32 total = positionWeight + bonusWeights[0] + bonusWeights[1];

	INT32 ours = 0;
	fixed_t percent = 0;

	if (position > 0 && numPlayers > 2)
	{
		const INT32 sc = (position - 1);
		const INT32 loser = ((numPlayers + 1) / 2); // number of winner positions
		ours += ((loser - sc) * positionWeight) / loser;
	}

	for (int i = 0; i < TALLY_WINDOW_SIZE; i++)
	{
		switch (bonuses[i])
		{
			case TALLY_BONUS_RING:
			{
				ours += (rings * bonusWeights[i]) / 20;
				break;
			}
			case TALLY_BONUS_LAP:
			{
				// Use a special curve for this.
				// The difference between 0 and 1 lap points is an important difference in skill,
				// while the difference between 5 and 6 is not very notable.
				const fixed_t frac = (laps * FRACUNIT) / std::max(1, static_cast<int>(totalLaps));
				ours += Easing_OutSine(frac, 0, bonusWeights[i]);
				break;
			}
			case TALLY_BONUS_PRISON:
			{
				ours += (prisons * bonusWeights[i]) / std::max(1, static_cast<int>(totalPrisons));
				break;
			}
			case TALLY_BONUS_SCORE:
			{
				if (pointLimit != 0)
				{
					ours += (points * bonusWeights[i]) / std::max(1, static_cast<int>(abs(pointLimit)));
				}
				break;
			}
			case TALLY_BONUS_POWERSTONES:
			{
				ours += (powerStones * bonusWeights[i]) / 7;
				break;
			}
			default:
			{
				break;
			}
		}
	}

	percent = FixedDiv(ours, total);

	for (retGrade = GRADE_E; retGrade < GRADE_A; retGrade++)
	{
		if (percent < gradePercents[retGrade])
		{
			break;
		}
	}

	return retGrade;
}

void level_tally_t::Init(player_t *player)
{
	if (active == true)
	{
		return;
	}

	active = true;
	owner = player;
	gt = gametype;

	const boolean game_over = (
		G_GametypeUsesLives()
		? ((player->pflags & PF_LOSTLIFE) == PF_LOSTLIFE)
		: (tutorialchallenge == TUTORIALSKIP_INPROGRESS && K_IsPlayerLosing(player))
	);

	time = std::min(static_cast<INT32>(player->realtime), (100 * 60 * TICRATE) - 1);
	ringPool = player->totalring;
	livesAdded = 0;

	position = numPlayers = 0;
	rings = 0;
	laps = totalLaps = 0;
	points = pointLimit = 0;
	powerStones = 0;

	rank = GRADE_INVALID;

	if (player->spectator == false && player->bot == false && game_over == false)
	{
		if (K_TimeAttackRules() == false && K_Cooperative() == false)
		{
			position = (player->pflags & PF_NOCONTEST) ? (MAXPLAYERS+1) : player->position;

			for (int i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] == true && players[i].spectator == false)
				{
					numPlayers++;
				}
			}
		}

		if ((gametypes[gt]->rules & GTR_SPHERES) == 0)
		{
			if (player->hudrings > 0) // Don't count negative rings
			{
				rings = player->hudrings;
			}
		}

		if ((gametypes[gt]->rules & GTR_CIRCUIT) == GTR_CIRCUIT)
		{
			laps = player->lapPoints;
			totalLaps = numlaps;

			if (inDuel == false)
			{
				totalLaps *= 2;
			}
		}

		if (battleprisons)
		{
			prisons = numtargets;
			totalPrisons = maptargets;
		}

		if ((gametypes[gt]->rules & GTR_POINTLIMIT) == GTR_POINTLIMIT)
		{
			points = player->roundscore;
			pointLimit = g_pointlimit;

			if (pointLimit == 0)
			{
				// Get max from players in server.
				// This is set as a negative number
				// to communicate to not show it as x/y
				// when drawing it on the HUD.
				for (int i = 0; i < MAXPLAYERS; i++)
				{
					if (playeringame[i] == true && players[i].spectator == false)
					{
						pointLimit = std::min(pointLimit, static_cast<int>(-players[i].roundscore));
					}
				}
			}
		}

		if ((gametypes[gt]->rules & GTR_POWERSTONES) == GTR_POWERSTONES)
		{
			powerStones = K_NumEmeralds(player);
		}

		DetermineStatistics();
		DetermineBonuses();

		rank = CalculateGrade();
	}

	header[0] = '\0';
	gotThru = showRoundNum = false;

	if (player->spectator == false)
	{
		if (game_over == true)
		{
			if (tutorialchallenge == TUTORIALSKIP_INPROGRESS)
			{
				snprintf(
					header, sizeof header,
					"NICE TRY"
				);
			}
			else if (G_GametypeUsesLives() && player->lives <= 0)
			{
				snprintf(
					header, sizeof header,
					"GAME OVER"
				);
			}
			else
			{
				snprintf(
					header, sizeof header,
					"TRY AGAIN"
				);
			}
		}
		else if ((player->pflags & PF_NOCONTEST) == 0)
		{
			gotThru = true;

			if (player->skin < numskins)
			{
				snprintf(
					header, sizeof header,
					"%s", skins[player->skin].realname
				);
			}

			showRoundNum = true;
		}
		else
		{
			snprintf(
				header, sizeof header,
				"NO CONTEST..."
			);
		}
	}
	else
	{
		if (roundqueue.position > 0 && roundqueue.position <= roundqueue.size
			&& (grandprixinfo.gp == false || grandprixinfo.eventmode == GPEVENT_NONE))
		{
			snprintf(
				header, sizeof header,
				"ROUND"
			);

			showRoundNum = true;
		}
		else if (K_CheckBossIntro() == true && bossinfo.enemyname)
		{
			snprintf(
				header, sizeof header,
				"%s", bossinfo.enemyname
			);
		}
		else if (battleprisons == true)
		{
			snprintf(
				header, sizeof header,
				"PRISON BREAK"
			);
		}
		else
		{
			snprintf(
				header, sizeof header,
				"%s STAGE",
				gametypes[gametype]->name
			);
		}
	}

	header[sizeof header - 1] = '\0';

	// Only show grade if there were any bonuses
	if (rank != GRADE_INVALID)
	{
		showGrade = (position > 0);
		if (showGrade == false)
		{
			for (int i = 0;	i < TALLY_WINDOW_SIZE; i++)
			{
				if (bonuses[i] != TALLY_BONUS_NA)
				{
					showGrade = true;
					break;
				}
			}
		}
	}

	if (showGrade == false)
	{
		rank = GRADE_INVALID;
	}

	lineCount = 0;
	for (int i = 0; i < TALLY_WINDOW_SIZE; i++)
	{
		if (stats[i] != TALLY_STAT_NA)
		{
			lineCount++;
		}
		if (bonuses[i] != TALLY_BONUS_NA)
		{
			lineCount++;
		}
	}

	gradeVoice = sfx_None;
	if (showGrade == true)
	{
		// It'd be neat to add all of the grade sounds,
		// but not this close to release
		if (rank < GRADE_C)
		{
			gradeVoice = ((skin_t *)player->mo->skin)->soundsid[S_sfx[sfx_klose].skinsound];
		}
		else
		{
			gradeVoice = ((skin_t *)player->mo->skin)->soundsid[S_sfx[sfx_kwin].skinsound];
		}
	}

	delay = K_TallyDelay(); // sync up with musiccountdown

	if (game_over == true)
	{
		// set up game over instead
		state = TALLY_ST_GAMEOVER_SLIDEIN;
		showGrade = false;
	}
	else
	{
		state = TALLY_ST_GOTTHRU_SLIDEIN;
	}

	hudSlide = 0;

	transition = 0;
	transitionTime = TICRATE/2;

	done = (player->spectator == true || player->bot == true);

	if (specialstageinfo.valid == true && (player->pflags & PF_NOCONTEST) == PF_NOCONTEST)
	{
		// No tally when losing special stages
		state = TALLY_ST_IGNORE;
		delay = 0;
	}

	if (UINT8 pnum = player - players; G_IsPartyLocal(pnum))
	{
		UINT8 view = G_PartyPosition(pnum);
		// Battle: if this player's viewpoint has changed
		// since being eliminated, set it back so they see
		// their own Tally and not someone else's.
		if (displayplayers[view] != pnum)
		{
			displayplayers[view] = pnum;
			G_FixCamera(1 + view);
		}
	}
}

void level_tally_t::NewLine(void)
{
	state = TALLY_ST_TEXT_APPEAR;
	lines++;
	//S_StartSound(NULL, sfx_mbs5b);
	//transition = 0;
	//transitionTime = TICRATE/5;
	delay = TICRATE/5;
}

boolean level_tally_t::IncrementLine(void)
{
	UINT8 count = lines;

	INT32 *value = nullptr;
	INT32 dest = 0;

	INT32 amount = 1;
	INT32 freq = 2;

	boolean lives_check = false;

	for (int i = 0; i < TALLY_WINDOW_SIZE; i++)
	{
		if (count == 0)
		{
			break;
		}

		if (stats[i] == TALLY_STAT_NA)
		{
			break;
		}

		value = &displayStat[i];
		lives_check = (
			stats[i] == TALLY_STAT_TOTALRINGS // Rings also shows the Lives.
			&& livesAdded < owner->xtralife // Don't check if we've maxxed out!
		);

		switch (stats[i])
		{
			case TALLY_STAT_TIME:
				dest = time;
				amount = 111;
				freq = 0;
				break;
			case TALLY_STAT_TOTALRINGS:
				dest = ringPool + rings;
				amount = 1;
				freq = 1;
				break;
			default:
				dest = 0;
				amount = 1;
				freq = 2;
				break;
		}

		count--;
	}

	for (int i = 0; i < TALLY_WINDOW_SIZE; i++)
	{
		if (count == 0)
		{
			break;
		}

		if (bonuses[i] == TALLY_BONUS_NA)
		{
			break;
		}

		value = &displayBonus[i];
		lives_check = false;

		switch (bonuses[i])
		{
			case TALLY_BONUS_RING:
				dest = rings;
				amount = 1;
				freq = 1;
				break;
			case TALLY_BONUS_LAP:
				dest = laps;
				amount = 1;
				freq = 4;
				break;
			case TALLY_BONUS_PRISON:
				dest = prisons;
				amount = 1;
				freq = 4;
				break;
			case TALLY_BONUS_SCORE:
				dest = points;
				amount = 11;
				freq = 2;
				break;
			case TALLY_BONUS_POWERSTONES:
				dest = powerStones;
				amount = 1;
				freq = 4;
				break;
			default:
				dest = 0;
				amount = 1;
				freq = 2;
				break;
		}

		count--;
	}

	if (count > 0)
	{
		// No more lines to update.
		return true;
	}

	const boolean playSounds = P_IsDisplayPlayer(owner);

	if (*value == dest)
	{
		// We've reached our destination
		return true;
	}

	const INT32 prevVal = *value;

	if (playSounds == true && tickSound == 0)
	{
		S_StopSoundByNum(sfx_mbs5f);
		S_StartSound(NULL, sfx_mbs5f);
		tickSound = 3;
	}

	if (*value > dest)
	{
		*value -= amount;

		if (*value < dest)
		{
			*value = dest;
		}
	}
	else
	{
		*value += amount;

		if (*value > dest)
		{
			*value = dest;
		}
	}

	if (lives_check == true)
	{
		const UINT8 lifethreshold = 20;
		const UINT8 oldExtra = prevVal / lifethreshold;
		const UINT8 extra = *value / lifethreshold;

		// Handle extra life sound & blinking
		if (extra > oldExtra)
		{
			livesAdded++;
			xtraBlink = TICRATE;

			if (playSounds == true)
			{
				S_StopSoundByNum(sfx_cdfm73);
				S_StartSound(NULL, sfx_cdfm73);
			}
		}
	}

	delay = freq;
	return false;
}

void level_tally_t::Tick(void)
{
	if (hudSlide < TICRATE/4)
	{
		hudSlide++;
	}

	if (tickSound > 0)
	{
		tickSound--;
	}

	if (xtraBlink > 0)
	{
		xtraBlink--;
	}

	if (delay > 0)
	{
		delay--;
		return;
	}

	if (transition < FRACUNIT)
	{
		if (transitionTime <= 0)
		{
			transition = FRACUNIT;
			return;
		}

		transition += FRACUNIT / transitionTime;
		if (transition > FRACUNIT)
		{
			transition = FRACUNIT;
		}
		return;
	}

	const boolean playSounds = P_IsDisplayPlayer(owner);

	switch (state)
	{
		case TALLY_ST_GOTTHRU_SLIDEIN:
		{
			state = TALLY_ST_GOTTHRU_SLIDEUP;
			transition = 0;
			transitionTime = TICRATE/2;
			delay = TICRATE/2;
			break;
		}
		case TALLY_ST_GOTTHRU_SLIDEUP:
		{
			state = TALLY_ST_BOXES_SLIDEIN;
			transition = 0;
			transitionTime = TICRATE/2;
			delay = TICRATE/5;
			break;
		}
		case TALLY_ST_BOXES_SLIDEIN:
		{
			NewLine();
			break;
		}
		case TALLY_ST_TEXT_APPEAR:
		{
			if (IncrementLine() == true)
			{
				if (grandprixinfo.gp == true // In GP
					&& lines >= lineCount // Finished the bonuses
					&& livesAdded < owner->xtralife // Didn't max out by other causes
				)
				{
					// This is only true if Rings alone aren't responsible for our added lives.
					// Generally for Prison Break, but could be earned in custom contexts too.
					livesAdded = owner->xtralife;
					xtraBlink = TICRATE;

					if (playSounds == true)
					{
						S_StopSoundByNum(sfx_cdfm73);
						S_StartSound(NULL, sfx_cdfm73);
					}
				}

				if (playSounds == true)
				{
					S_StopSoundByNum(sfx_mbs5b);
					S_StartSound(NULL, (lines >= lineCount) ? sfx_mbs70 : sfx_mbs5b);
				}

				state = TALLY_ST_TEXT_PAUSE;
				delay = TICRATE/2;
			}
			break;
		}
		case TALLY_ST_TEXT_PAUSE:
		{
			if (lines < lineCount)
			{
				NewLine();
			}
			else
			{
				if (showGrade == true)
				{
					state = TALLY_ST_GRADE_APPEAR;
					transition = 0;
					transitionTime = TICRATE/7;
					delay = TICRATE/2;

					// for UCRP_FINISHGRADE
					owner->roundconditions.checkthisframe = true;
				}
				else
				{
					state = TALLY_ST_DONE;
					delay = 5*TICRATE;
				}
			}
			break;
		}
		case TALLY_ST_GRADE_APPEAR:
		{
			if (playSounds)
			{
				S_StartSound(NULL, sfx_rank);
			}
			state = TALLY_ST_GRADE_VOICE;
			delay = TICRATE/2;
			break;
		}
		case TALLY_ST_GRADE_VOICE:
		{
			if (playSounds && cv_kartvoices.value)
			{
				S_StartSound(NULL, gradeVoice);
			}
			state = TALLY_ST_DONE;
			delay = 5*TICRATE;
			break;
		}
		case TALLY_ST_GAMEOVER_SLIDEIN:
		{
			state = TALLY_ST_GAMEOVER_LIVES;
			delay = TICRATE;
			break;
		}
		case TALLY_ST_GAMEOVER_LIVES:
		{
			state = TALLY_ST_GAMEOVER_DONE;
			delay = 4*TICRATE;
			break;
		}
		case TALLY_ST_DONE:
		case TALLY_ST_GAMEOVER_DONE:
		case TALLY_ST_IGNORE:
		{
			done = true;
			break;
		}
		
		default:
		{
			// error occured, silently fix
			state = TALLY_ST_DONE;
			break;
		}
	}
}

void level_tally_t::Draw(void)
{
	if (state == TALLY_ST_IGNORE)
	{
		return;
	}

	const float transition_f = FixedToFloat(transition);
	const float transition_i = 1.0 - transition_f;

	const float frac = (r_splitscreen ? 0.5 : 1.0);

	INT32 v_width = BASEVIDWIDTH;
	INT32 v_height = BASEVIDHEIGHT;
	if (r_splitscreen > 0)
	{
		v_height /= 2;
	}
	if (r_splitscreen > 1)
	{
		v_width /= 2;
	}

	SINT8 h_transition_sign = 1;
	if (r_splitscreen > 1)
	{
		if (!(R_GetViewNumber() & 1))
		{
			h_transition_sign = -h_transition_sign;
		}
	}
	else if (r_splitscreen > 0)
	{
		if (R_GetViewNumber() == 1)
		{
			h_transition_sign = -h_transition_sign;
		}
	}

	srb2::Draw drawer = (srb2::Draw(0, 0))
		.flags(V_SPLITSCREEN)
		.clipx(0, v_width)
		.clipy(0, v_height);


	INT32 fade = 5;
	if (state == TALLY_ST_GOTTHRU_SLIDEIN
		|| state == TALLY_ST_GAMEOVER_SLIDEIN)
	{
		fade = (5 * transition_f);
	}

	V_DrawFadeFill(
		(vid.width / 2) * (r_splitscreen > 1 && R_GetViewNumber() & 1),
		(vid.height / 2) * (R_GetViewNumber() > (r_splitscreen > 1)),
		vid.width / (r_splitscreen > 1 ? 2 : 1),
		vid.height / (r_splitscreen ? 2 : 1),
		V_NOSCALESTART,
		31, fade
	);

	const INT32 header_width = (r_splitscreen ? (BASEVIDWIDTH * 0.5) : BASEVIDWIDTH);
	const INT32 header_x = (v_width - header_width) * 0.5;

	const INT32 header_height = 36 * frac;
	const INT32 header_centered = (v_height * 0.5) - header_height;

	switch (state)
	{
		case TALLY_ST_GAMEOVER_SLIDEIN:
		case TALLY_ST_GAMEOVER_LIVES:
		case TALLY_ST_GAMEOVER_DONE:
		case TALLY_ST_GOTTHRU_SLIDEIN:
			Y_DrawIntermissionHeader(
				(header_x * FRACUNIT) + (v_width * transition_i * FRACUNIT * h_transition_sign),
				header_centered * FRACUNIT,
				gotThru, header, showRoundNum, (r_splitscreen > 0)
			);
			break;

		case TALLY_ST_GOTTHRU_SLIDEUP:
			Y_DrawIntermissionHeader(
				header_x * FRACUNIT,
				header_centered * transition_i * FRACUNIT,
				gotThru, header, showRoundNum, (r_splitscreen > 0)
			);
			break;

		default:
			Y_DrawIntermissionHeader(
				header_x * FRACUNIT,
				0,
				gotThru, header, showRoundNum, (r_splitscreen > 0)
			);
			break;
	}

	if (state == TALLY_ST_GAMEOVER_SLIDEIN
		|| state == TALLY_ST_GAMEOVER_LIVES
		|| state == TALLY_ST_GAMEOVER_DONE)
	{
		if (G_GametypeUsesLives() && owner->lives > 0)
		{
			srb2::Draw lives_drawer = drawer
				.xy(
					(v_width * 0.5) + (v_width * transition_i * h_transition_sign) - 8.0,
					header_centered + (header_height * 1.5) + 4.0
				);

			const skincolornum_t color = static_cast<skincolornum_t>(owner->skincolor);
			lives_drawer
				.colormap(owner->skin, color)
				.patch(faceprefix[owner->skin][FACE_RANK]);

			UINT8 lives_num = owner->lives;
			if (state == TALLY_ST_GAMEOVER_SLIDEIN)
			{
				lives_num++;
			}
			else if (state == TALLY_ST_GAMEOVER_LIVES)
			{
				lives_num++;

				if (((delay / 5) & 1) == 0)
				{
					lives_num = 0;
				}
			}

			if (lives_num > 0)
			{
				lives_drawer
					.xy(17.0, 4.0)
					.patch( va("OPPRNK%02d", lives_num) );
			}
		}

		return;
	}

	if (state != TALLY_ST_GOTTHRU_SLIDEIN
		&& state != TALLY_ST_GOTTHRU_SLIDEUP)
	{
		UINT8 numBoxes = 0;
		boolean drawStats = false;
		if (stats[0] != TALLY_STAT_NA)
		{
			numBoxes++;
			drawStats = true;
		}
		if (bonuses[0] != TALLY_BONUS_NA)
		{
			numBoxes++;
		}

		patch_t *box_fg = static_cast<patch_t*>( W_CachePatchName(va("RNKBLK%sA", (r_splitscreen ? "4" : "1")), PU_CACHE) );
		patch_t *box_bg = static_cast<patch_t*>( W_CachePatchName(va("RNKBLK%sB", (r_splitscreen ? "4" : "1")), PU_CACHE) );

		patch_t *sticker = static_cast<patch_t*>( W_CachePatchName((r_splitscreen ? "K_SPDMBG" : "K_STTIME"), PU_CACHE) );
		const float sticker_offset = (r_splitscreen ? 0.0 : 3.0);

		patch_t *egg_sticker = static_cast<patch_t*>( W_CachePatchName("EGGSTKR", PU_CACHE) );

		srb2::Draw drawer_box = drawer.xy(
			(v_width * 0.5) - (box_bg->width * 0.5),
			((v_height * 0.5) + (8.0 * frac)) - ((box_bg->height * 0.5) * numBoxes)
		);

		UINT8 displayLines = lines;

		for (int b = 0;	b < numBoxes; b++)
		{
			srb2::Draw drawer_box_offset = drawer_box;

			switch (state)
			{
				case TALLY_ST_BOXES_SLIDEIN:
					drawer_box_offset = drawer_box_offset.x( ((b & 1) ? 1 : -1) * transition_i * v_width * h_transition_sign );
					break;
				default:
					break;
			}

			drawer_box_offset
				.colormap(SKINCOLOR_BLUE)
				.flags(V_ADD)
				.patch(box_bg);

			drawer_box_offset
				.patch(box_fg);

			srb2::Draw drawer_text = drawer_box_offset
				.xy(11.0 * frac, 6.0 * frac)
				.font(r_splitscreen ? srb2::Draw::Font::kPing : srb2::Draw::Font::kTimer);

			UINT8 boxLines = 0;
			if (drawStats == true)
			{
				for (int i = 0; i < TALLY_WINDOW_SIZE; i++)
				{
					if (stats[i] != TALLY_STAT_NA)
					{
						boxLines++;
					}
				}
			}
			else
			{
				for (int i = 0; i < TALLY_WINDOW_SIZE; i++)
				{
					if (bonuses[i] != TALLY_BONUS_NA)
					{
						boxLines++;
					}
				}
			}

			drawer_text = drawer_text.y(17.0 * frac * (TALLY_WINDOW_SIZE - boxLines) * 0.5);

			for (int i = 0; i < TALLY_WINDOW_SIZE; i++)
			{
				if (displayLines == 0)
				{
					break;
				}

				const char *bonus_code = "XX";
				if (drawStats == true)
				{
					if (stats[i] == TALLY_STAT_NA)
					{
						continue;
					}

					switch (stats[i])
					{
						case TALLY_STAT_TIME:
							bonus_code = "TM";
							break;
						case TALLY_STAT_TOTALRINGS:
							bonus_code = "TR";
							break;
						default:
							break;
					}
				}
				else
				{
					if (bonuses[i] == TALLY_BONUS_NA)
					{
						continue;
					}

					switch (bonuses[i])
					{
						case TALLY_BONUS_RING:
							bonus_code = "RB";
							break;
						case TALLY_BONUS_LAP:
							bonus_code = "LA";
							break;
						case TALLY_BONUS_PRISON:
							bonus_code = "PR";
							break;
						case TALLY_BONUS_SCORE:
							bonus_code = "ST";
							break;
						case TALLY_BONUS_POWERSTONES:
							bonus_code = "EM";
							break;
						default:
							break;
					}
				}

				if (r_splitscreen == 0)
				{
					drawer_text
						.xy(100.0 * frac, -2.0 * frac)
						.patch(egg_sticker);
				}

				drawer_text
					.y(-1.0 * frac)
					.patch(va("BNS%sP_%s", (r_splitscreen ? "4" : "1"), bonus_code));

				drawer_text
					.xy((197.0 * frac) - (sticker->width * 0.5), -sticker_offset)
					.patch(sticker);

				if (drawStats == true)
				{
					switch (stats[i])
					{
						case TALLY_STAT_TIME:
						{
							INT32 work_minutes = displayStat[i] / (60 * TICRATE);
							INT32 work_seconds = displayStat[i] / TICRATE % 60;
							INT32 work_tics = G_TicsToCentiseconds(displayStat[i]);

							drawer_text
								.x(197.0 * frac)
								.align(srb2::Draw::Align::kCenter)
								.text(va(
									"%d%d'%d%d\"%d%d",
									work_minutes / 10,
									work_minutes % 10,
									work_seconds / 10,
									work_seconds % 10,
									work_tics / 10,
									work_tics % 10
								));
							break;
						}
						case TALLY_STAT_TOTALRINGS:
						{
							drawer_text
								.x(184.0 * frac)
								.align(srb2::Draw::Align::kCenter)
								.text(va("%d", displayStat[i]));

							srb2::Draw lives_drawer = drawer_text
								.xy(221.0 * frac, -1.0 * frac);

							const skincolornum_t color = static_cast<skincolornum_t>(owner->skincolor);
							lives_drawer
								.x(r_splitscreen ? -7.0 : 0.0)
								.colormap(owner->skin, color)
								.patch(faceprefix[owner->skin][r_splitscreen ? FACE_MINIMAP : FACE_RANK]);

							UINT8 lives_num = std::min(owner->lives + livesAdded, 10);
							if (xtraBlink > 0 && (xtraBlink & 1) == 0 && livesAdded > 0)
							{
								lives_num = 0;
							}

							if (lives_num > 0)
							{
								if (r_splitscreen)
								{
									lives_drawer
										.xy(6.0, 2.0)
										.align(srb2::Draw::Align::kLeft)
										.text(va("%d", lives_num));
								}
								else
								{
									lives_drawer
										.xy(17.0, 4.0)
										.patch( va("OPPRNK%02d", lives_num) );
								}
							}

							break;
						}
						default:
						{
							drawer_text
								.x(197.0 * frac)
								.align(srb2::Draw::Align::kCenter)
								.text(va("%d", displayStat[i]));
							break;
						}
					}
				}
				else
				{
					switch (bonuses[i])
					{
						case TALLY_BONUS_RING:
						{
							drawer_text
								.x(197.0 * frac)
								.align(srb2::Draw::Align::kCenter)
								.text(va("%d / 20", displayBonus[i]));
							break;
						}
						case TALLY_BONUS_LAP:
						{
							drawer_text
								.x(197.0 * frac)
								.align(srb2::Draw::Align::kCenter)
								.text(va("%d / %d", displayBonus[i], totalLaps));
							break;
						}
						case TALLY_BONUS_PRISON:
						{
							drawer_text
								.x(197.0 * frac)
								.align(srb2::Draw::Align::kCenter)
								.text(va("%d / %d", displayBonus[i], totalPrisons));
							break;
						}
						case TALLY_BONUS_SCORE:
						{
							if (pointLimit > 0)
							{
								drawer_text
									.x(197.0 * frac)
									.align(srb2::Draw::Align::kCenter)
									.text(va("%d / %d", displayBonus[i], pointLimit));
							}
							else
							{
								drawer_text
									.x(197.0 * frac)
									.align(srb2::Draw::Align::kCenter)
									.text(va("%d", displayBonus[i]));
							}
							break;
						}
						case TALLY_BONUS_POWERSTONES:
						{
							drawer_text
								.x(197.0 * frac)
								.align(srb2::Draw::Align::kCenter)
								.text(va("%d / 7", displayBonus[i]));
							break;
						}
						default:
						{
							drawer_text
								.x(197.0 * frac)
								.align(srb2::Draw::Align::kCenter)
								.text(va("%d", displayBonus[i]));
							break;
						}
					}
				}

				drawer_text = drawer_text.y(17.0 * frac);
				displayLines--;
			}
			drawStats = false;

			drawer_box = drawer_box.y(box_bg->height);
		}
	}

	if (showGrade == false)
	{
		return;
	}

	if ((state == TALLY_ST_GRADE_APPEAR && delay == 0)
		|| state == TALLY_ST_GRADE_VOICE
		|| state == TALLY_ST_DONE)
	{
		char grade_letter = K_GetGradeChar( static_cast<gp_rank_e>(rank) );

		patch_t *grade_img = static_cast<patch_t*>( W_CachePatchName(va("R_FINR%c%c", (r_splitscreen ? 'S' : 'N'), grade_letter), PU_CACHE) );
		srb2::Draw grade_drawer = drawer
			.xy(v_width * 0.5, v_height - (2.0 * frac) - (grade_img->height * 0.5))
			.colormap( static_cast<skincolornum_t>(K_GetGradeColor( static_cast<gp_rank_e>(rank) )) );

		float sc = 1.0;
		if (state == TALLY_ST_GRADE_APPEAR)
		{
			sc += transition_i * 8.0;
		}

		grade_drawer
			.xy(-grade_img->width * 0.5 * sc, -grade_img->height * 0.5 * sc)
			.scale(sc)
			.patch(grade_img);
	}
}

void K_InitPlayerTally(player_t *player)
{
	player->tally.Init(player);
}

void K_TickPlayerTally(player_t *player)
{
	player->tally.Tick();
}

void K_DrawPlayerTally(void)
{
	stplyr->tally.Draw();
}

boolean K_PlayerTallyActive(player_t *player)
{
	return player->tally.active; //(player->exiting || (player->pflags & PF_NOCONTEST));
}

tic_t K_TallyDelay(void)
{
	return ((gametyperules & GTR_BUMPERS) ? 4 : 3) * TICRATE;
}
