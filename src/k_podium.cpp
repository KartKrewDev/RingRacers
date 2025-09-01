// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_podium.c
/// \brief Grand Prix podium cutscene

#include "k_podium.h"

#include "core/string.h"
#include "doomdef.h"
#include "d_main.h"
#include "d_netcmd.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "r_local.h"
#include "s_sound.h"
#include "i_time.h"
#include "i_video.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_threads.h"
#include "dehacked.h"
#include "g_input.h"
#include "console.h"
#include "m_random.h"
#include "m_misc.h" // moviemode functionality
#include "y_inter.h"
#include "m_cond.h"
#include "p_local.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "st_stuff.h" // hud hiding
#include "fastcmp.h"

#include "lua_hud.h"
#include "lua_hook.h"

#include "k_menu.h"
#include "k_grandprix.h"
#include "k_rank.h"

#include "v_draw.hpp"

#include "k_hud.h"

typedef enum
{
	PODIUM_ST_CONGRATS_SLIDEIN,
	PODIUM_ST_CONGRATS_SLIDEUP,
	PODIUM_ST_DATA_SLIDEIN,
	PODIUM_ST_DATA_PAUSE,
	PODIUM_ST_LEVEL_APPEAR,
	PODIUM_ST_LEVEL_PAUSE,
	PODIUM_ST_TOTALS_SLIDEIN,
	PODIUM_ST_TOTALS_PAUSE,
	PODIUM_ST_GRADE_APPEAR,
	PODIUM_ST_GRADE_VOICE,
	PODIUM_ST_DONE,
	PODIUM_ST_EXIT,
} podium_state_e;

static struct podiumData_s
{
	boolean ranking;
	gpRank_t rank;
	gp_rank_e grade;

	podium_state_e state;
	INT32 delay;
	INT32 transition, transitionTime;

	UINT8 displayLevels;
	sfxenum_t gradeVoice;

	cupheader_t *cup;
	UINT8 emeraldnum;

	boolean fastForward;

	char header[64];
	char difficulty[64];

	void Init(void);
	void NextLevel(void);
	void Tick(void);
	void Draw(void);
} g_podiumData;

void podiumData_s::Init(void)
{
	fastForward = false;

	if (grandprixinfo.cup != nullptr)
	{
		rank = grandprixinfo.rank;
		cup = grandprixinfo.cup;
		emeraldnum = cup->emeraldnum;
	}
	else
	{
		// construct fake rank for testing podium
		// directly from the editor

		size_t cupID = M_RandomRange(0, numkartcupheaders-1);
		cup = kartcupheaders;
		for (size_t i = 0; i < cupID; i++)
		{
			if (cup == nullptr)
			{
				break;
			}

			cup = cup->next;
		}
		emeraldnum = 0;

		memset(&rank, 0, sizeof(gpRank_t));
		rank.skin = players[consoleplayer].skin;

		rank.numPlayers = std::clamp<UINT8>(M_RandomRange(0, MAXSPLITSCREENPLAYERS + 1), 1, MAXSPLITSCREENPLAYERS);
		rank.totalPlayers = K_GetGPPlayerCount(rank.numPlayers);

		rank.position = M_RandomRange(1, 4);

		rank.continuesUsed = M_RandomRange(0, 3);

		// Fake totals
		rank.numLevels = 8;

		constexpr INT32 numRaces = 5;
		for (INT32 i = 0; i < rank.numPlayers; i++)
		{
			rank.totalPoints += numRaces * K_CalculateGPRankPoints(EXP_MAX, i+1, rank.totalPlayers);
		}
		rank.totalRings = numRaces * rank.numPlayers * 20;

		// Randomized winnings
		INT32 rgs = 0;
		INT32 exp = 0;
		INT32 texp = 0;
		INT32 prs = 0;
		INT32 tprs = 0;

		rank.winPoints = M_RandomRange(0, rank.totalPoints);

		for (INT32 i = 0; i < rank.numLevels; i++)
		{
			gpRank_level_t *const lvl = &rank.levels[i];
			UINT8 specialWinner = 0;
			UINT16 pprs = 0;
			UINT16 pexp = 0;

			lvl->id = M_RandomRange(4, nummapheaders);

			lvl->event = GPEVENT_NONE;
			switch (i)
			{
				case 2:
				case 5:
				{
					lvl->event = GPEVENT_BONUS;
					lvl->totalPrisons = M_RandomRange(1, 10);
					tprs += lvl->totalPrisons;
					break;
				}
				case 7:
				{
					lvl->event = GPEVENT_SPECIAL;
					specialWinner = M_RandomRange(0, rank.numPlayers);
					break;
				}
				default:
				{
					lvl->totalExp = EXP_TARGET;
					texp += lvl->totalExp * rank.numPlayers;
					break;
				}
			}

			lvl->time = M_RandomRange(50*TICRATE, 210*TICRATE);

			lvl->continues = 0;
			if (!M_RandomRange(0, 2))
				lvl->continues = M_RandomRange(1, 3);

			for (INT32 j = 0; j < rank.numPlayers; j++)
			{
				gpRank_level_perplayer_t *const dta = &lvl->perPlayer[j];

				dta->position = M_RandomRange(1, rank.totalPlayers);

				if (lvl->event == GPEVENT_NONE)
				{
					dta->rings = M_RandomRange(0, 20);
					rgs += dta->rings;

					dta->exp = M_RandomRange(EXP_MIN, EXP_MAX);
					pexp += dta->exp;
				}

				if (lvl->event == GPEVENT_BONUS)
				{
					dta->prisons = M_RandomRange(0, lvl->totalPrisons);
					pprs = std::max(pprs, dta->prisons);
				}

				if (lvl->event == GPEVENT_SPECIAL)
				{
					dta->gotSpecialPrize = (j+1 == specialWinner);
					dta->grade = GRADE_E;
					if (dta->gotSpecialPrize)
					{
						rank.specialWon = true;
					}
				}
				else
				{
					dta->grade = static_cast<gp_rank_e>(M_RandomRange(static_cast<INT32>(GRADE_E), static_cast<INT32>(GRADE_A)));
				}
			}

			exp += pexp;
			prs += pprs;
		}

		rank.rings = rgs;
		rank.exp = exp;
		rank.totalExp = texp;
		rank.prisons = prs;
		rank.totalPrisons = tprs;
	}

	grade = K_CalculateGPGrade(&rank);

	delay = TICRATE/2;

	transition = 0;
	transitionTime = TICRATE/2;

	header[0] = '\0';

	if (rank.position > RANK_NEUTRAL_POSITION)
	{
		snprintf(
			header, sizeof header,
			"NO GOOD..."
		);
	}
	else
	{
		snprintf(
			header, sizeof header,
			"CONGRATULATIONS"
		);
	}

	switch(grandprixinfo.gamespeed)
	{
		case KARTSPEED_EASY:
			snprintf(difficulty, sizeof difficulty, "Relaxed");
			break;
		case KARTSPEED_NORMAL:
			snprintf(difficulty, sizeof difficulty, "Intense");
			break;
		case KARTSPEED_HARD:
			snprintf(difficulty, sizeof difficulty, "Vicious");
			break;
		default:
			snprintf(difficulty, sizeof difficulty, "?");
	}

	if (grandprixinfo.masterbots)
		snprintf(difficulty, sizeof difficulty, "Master");

	if (cv_4thgear.value || cv_levelskull.value)
		snprintf(difficulty, sizeof difficulty, "Extra");

	header[sizeof header - 1] = '\0';

	displayLevels = 0;

	gradeVoice = sfx_None;

	// It'd be neat to add all of the grade sounds,
	// but not this close to release
	if (rank.position > RANK_NEUTRAL_POSITION || grade < GRADE_C)
	{
		gradeVoice = skins[rank.skin]->soundsid[S_sfx[sfx_klose].skinsound];
	}
	else
	{
		gradeVoice = skins[rank.skin]->soundsid[S_sfx[sfx_kwin].skinsound];
	}
}

void podiumData_s::NextLevel(void)
{
	state = PODIUM_ST_LEVEL_APPEAR;
	displayLevels++;
	delay = TICRATE/7;
}

void podiumData_s::Tick(void)
{
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

	switch (state)
	{
		case PODIUM_ST_CONGRATS_SLIDEIN:
		{
			state = PODIUM_ST_CONGRATS_SLIDEUP;
			transition = 0;
			transitionTime = TICRATE/2;
			delay = TICRATE/2;
			break;
		}
		case PODIUM_ST_CONGRATS_SLIDEUP:
		{
			state = PODIUM_ST_DATA_SLIDEIN;
			transition = 0;
			transitionTime = TICRATE/2;
			delay = TICRATE/5;
			break;
		}
		case PODIUM_ST_DATA_SLIDEIN:
		{
			state = PODIUM_ST_DATA_PAUSE;
			delay = TICRATE/5;
			break;
		}
		case PODIUM_ST_DATA_PAUSE:
		{
			NextLevel();
			break;
		}
		case PODIUM_ST_LEVEL_APPEAR:
		{
			S_StopSoundByNum(sfx_mbs5b);
			S_StartSound(nullptr, (displayLevels >= rank.numLevels) ? sfx_mbs70 : sfx_mbs5b);

			state = PODIUM_ST_LEVEL_PAUSE;
			delay = TICRATE/2;
			break;
		}
		case PODIUM_ST_LEVEL_PAUSE:
		{
			if (displayLevels < rank.numLevels)
			{
				NextLevel();
			}
			else
			{
				state = PODIUM_ST_TOTALS_SLIDEIN;
				transition = 0;
				transitionTime = TICRATE/2;
				delay = TICRATE/5;
			}
			break;
		}
		case PODIUM_ST_TOTALS_SLIDEIN:
		{
			state = PODIUM_ST_TOTALS_PAUSE;
			delay = TICRATE/5;
			break;
		}
		case PODIUM_ST_TOTALS_PAUSE:
		{
			state = PODIUM_ST_GRADE_APPEAR;
			transition = 0;
			transitionTime = TICRATE/7;
			delay = TICRATE/2;
			break;
		}
		case PODIUM_ST_GRADE_APPEAR:
		{
			S_StartSound(nullptr, sfx_rank);
			if (K_CalculateGPGrade(&rank) >= GRADE_S)
				S_StartSoundAtVolume(nullptr, sfx_srank, 200);
			state = PODIUM_ST_GRADE_VOICE;
			delay = TICRATE/2;
			break;
		}
		case PODIUM_ST_GRADE_VOICE:
		{
			if (cv_kartvoices.value)
			{
				S_StartSound(nullptr, gradeVoice);
			}
			state = PODIUM_ST_DONE;
			delay = 5*TICRATE;
			break;
		}
		case PODIUM_ST_DONE:
		{
			if (menuactive == false && M_MenuConfirmPressed(0) == true)
			{
				state = PODIUM_ST_EXIT;
				delay = 2*TICRATE;
			}
			break;
		}
		case PODIUM_ST_EXIT:
		{
			if (grandprixinfo.gp == true
				&& grandprixinfo.cup != nullptr
				&& grandprixinfo.cup->playcredits == true)
			{
				nextmap = NEXTMAP_CREDITS;
			}
			else
			{
				nextmap = NEXTMAP_TITLE;
			}

			G_EndGame();
			return;
		}
	}
}

void podiumData_s::Draw(void)
{
	INT32 i;

	const float transition_f = FixedToFloat(transition);
	const float transition_i = 1.0 - transition_f;

	srb2::Draw drawer = srb2::Draw(0, 0);

	INT32 fade = 5;
	if (state == PODIUM_ST_CONGRATS_SLIDEIN)
	{
		fade = (5 * transition_f);
	}

	V_DrawFadeFill(
		0, 0,
		vid.width, vid.height,
		V_NOSCALESTART,
		31, fade
	);

	constexpr INT32 header_height = 36;
	constexpr INT32 header_offset = -16;
	constexpr INT32 header_centered = (BASEVIDHEIGHT * 0.5) - header_height - header_offset;

	switch (state)
	{
		case PODIUM_ST_CONGRATS_SLIDEIN:
			Y_DrawIntermissionHeader(
				(BASEVIDWIDTH * transition_i * FRACUNIT),
				(header_centered + header_offset) * FRACUNIT,
				false, header, false, false
			);
			break;

		case PODIUM_ST_CONGRATS_SLIDEUP:
			Y_DrawIntermissionHeader(
				0,
				((header_centered * transition_i) + header_offset) * FRACUNIT,
				false, header, false, false
			);
			break;

		default:
			Y_DrawIntermissionHeader(
				0,
				header_offset * FRACUNIT,
				false, header, false, false
			);
			break;
	}

	const boolean singlePlayer = (rank.numPlayers == 1);
	player_t *bestHuman = &players[consoleplayer];

	if (singlePlayer == false)
	{
		UINT8 bestPos = UINT8_MAX;

		for (i = 0; i < rank.numPlayers; i++)
		{
			// BLEH BLEH, skincolor isn't saved to GP results, so I can't use the same values that get set. ANNOYING.
			if (players[i].position < bestPos)
			{
				bestHuman = &players[i];
				bestPos = players[i].position;
			}
		}
	}

	srb2::Draw drawer_winner = drawer.xy(16, 16);
	if (state >= PODIUM_ST_DATA_SLIDEIN)
	{
		if (state == PODIUM_ST_DATA_SLIDEIN)
		{
			drawer_winner = drawer_winner.x( transition_i * -BASEVIDWIDTH );
		}

		drawer_winner
			.colormap(bestHuman->skin, static_cast<skincolornum_t>(bestHuman->skincolor))
			.patch(faceprefix[bestHuman->skin][FACE_WANTED]);

		drawer_winner
			.xy(16, 28)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kMenu)
			.text(difficulty);

		drawer_winner
			.xy(44, 31)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kZVote)
			.text(va("%c%d", (rank.scorePosition > 0 ? '+' : ' '), rank.scorePosition));

		// drawer_winner
		// 	.xy(64, 19)
		// 	.patch("K_POINT4");

		// drawer_winner
		// 	.xy(88, 21)
		// 	.align(srb2::Draw::Align::kLeft)
		// 	.font(srb2::Draw::Font::kPing)
		// 	.colormap(TC_RAINBOW, SKINCOLOR_GOLD)
		// 	.text(va("%d", rank.winPoints));

		// drawer_winner
		// 	.xy(75, 31)
		// 	.align(srb2::Draw::Align::kCenter)
		// 	.font(srb2::Draw::Font::kZVote)
		// 	.text(va("%c%d", (rank.scoreGPPoints > 0 ? '+' : ' '), rank.scoreGPPoints));


		srb2::Draw drawer_trophy = drawer.xy(272, 10);
		if (state == PODIUM_ST_DATA_SLIDEIN)
		{
			drawer_trophy = drawer_trophy.x( transition_i * BASEVIDWIDTH );
		}

		if (cup != nullptr)
		{
			M_DrawCup(
				cup, drawer_trophy.x() * FRACUNIT, drawer_trophy.y() * FRACUNIT,
				0, true,
				(rank.position >= 1 && rank.position <= 3) ? rank.position : 0
			);
		}
	}

	if (state >= PODIUM_ST_LEVEL_APPEAR)
	{
		srb2::Draw drawer_line = drawer_winner.xy(80, 28);

		for (i = 0; i <= displayLevels; i++)
		{
			srb2::Draw drawer_perplayer = drawer_line;
			gpRank_level_t *lvl = nullptr;

			if (i > 0)
			{
				drawer_line
					.xy(-88, 6)
					.width(304)
					.height(2)
					.fill(31);

				lvl = &rank.levels[i - 1];

				if (lvl->id > 0)
				{
					char *title = G_BuildMapTitle(lvl->id);
					if (title)
					{
						drawer_perplayer
							.align(srb2::Draw::Align::kRight)
							.font(srb2::Draw::Font::kThin)
							.text(title);

						Z_Free(title);
					}
				}
			}

			INT32 p;
			for (p = 0; p < rank.numPlayers; p++)
			{
				player_t *const player = &players[displayplayers[p]];

				if (lvl == nullptr)
				{
					if (singlePlayer == false)
					{
						drawer_perplayer
							.xy(12, -2)
							.colormap(player->skin, static_cast<skincolornum_t>(player->skincolor))
							.patch(faceprefix[player->skin][FACE_MINIMAP]);

						drawer_perplayer
							.xy(26, 0)
							.font(srb2::Draw::Font::kConsole)
							.text(va("%c", ('A' + p)));
					}
				}
				else
				{
					gpRank_level_perplayer_t *const dta = &lvl->perPlayer[p];
					srb2::Draw drawer_rank = drawer_perplayer.xy(2, 0);

					if (lvl->event != GPEVENT_SPECIAL && dta->grade != GRADE_INVALID)
					{
							drawer_rank
								.xy(0, -1).flags(lvl->continues ? V_TRANSLUCENT : 0)
								.colormap( static_cast<skincolornum_t>(K_GetGradeColor(dta->grade)) )
								.patch(va("R_CUPRN%c", K_GetGradeChar(dta->grade)));
					}

					if (lvl->continues)
						drawer_rank.xy(7, 1).align(srb2::Draw::Align::kCenter).font(srb2::Draw::Font::kPing).colorize(SKINCOLOR_RED).text(va("-%d", lvl->continues));

					// Do not draw any stats for GAME OVERed player
					if (dta->grade != GRADE_INVALID || lvl->event == GPEVENT_SPECIAL)
					{
						srb2::Draw drawer_gametype = drawer_rank.xy(18, 0);

						switch (lvl->event)
						{
							case GPEVENT_BONUS:
							{
								drawer_gametype
									.xy(0, 1)
									.patch("K_CAPICO");

								drawer_gametype
									.xy(22, 1)
									.align(srb2::Draw::Align::kCenter)
									.font(srb2::Draw::Font::kPing)
									.text(va("%d/%d", dta->prisons, lvl->totalPrisons));
								break;
							}
							case GPEVENT_SPECIAL:
							{
								srb2::Draw drawer_emerald = drawer_gametype;
								UINT8 emeraldNum = g_podiumData.emeraldnum;

								boolean useWhiteFrame = ((leveltime & 1) || !dta->gotSpecialPrize);
								patch_t *emeraldPatch = nullptr;
								skincolornum_t emeraldColor = SKINCOLOR_NONE;

								if (emeraldNum == 0)
								{
									// Prize -- todo, currently using fake Emerald
									emeraldColor = SKINCOLOR_GOLD;
								}
								else
								{
									emeraldColor = static_cast<skincolornum_t>( SKINCOLOR_CHAOSEMERALD1 + ((emeraldNum - 1) % 7) );
								}

								{
									srb2::String emeraldName;
									if (emeraldNum > 7)
									{
										emeraldName = (useWhiteFrame ? "K_SUPER2" : "K_SUPER1");
									}
									else
									{
										emeraldName = (useWhiteFrame ? "K_EMERC" : "K_EMERW");
									}

									emeraldPatch = static_cast<patch_t*>( W_CachePatchName(emeraldName.c_str(), PU_CACHE) );
								}

								if (dta->gotSpecialPrize)
								{
									if (emeraldColor != SKINCOLOR_NONE)
									{
										drawer_emerald = drawer_emerald.colormap( emeraldColor );
									}
								}
								else
								{
									drawer_emerald = drawer_emerald.colormap( TC_BLINK, SKINCOLOR_BLACK );
								}

								drawer_emerald
									.xy(6 - (emeraldPatch->width * 0.5), 0)
									.patch(emeraldPatch);
								break;
							}
							default:
							{

								drawer_gametype
									.xy(0, 1)
									.colorize(static_cast<skincolornum_t>(SKINCOLOR_MUSTARD))
									.patch("K_SPTEXP");

								// Colorize the crystal, just like we do for hud
								skincolornum_t overlaycolor = SKINCOLOR_MUSTARD;
								fixed_t stablerateinverse = FRACUNIT - EXP_STABLERATE;
								INT16 exp_range = EXP_MAX-EXP_MIN;
								INT16 exp_offset = dta->exp-EXP_MIN;
								fixed_t factor = (exp_offset*FRACUNIT) / exp_range; // 0.0 to 1.0 in fixed
								// amount of blue is how much factor is above EXP_STABLERATE, and amount of red is how much factor is below
								// assume that EXP_STABLERATE is within 0.0 to 1.0 in fixed
								if (factor <= stablerateinverse)
								{
									overlaycolor = SKINCOLOR_RUBY;
									factor = FixedDiv(factor, stablerateinverse);
								}
								else
								{
									overlaycolor = SKINCOLOR_ULTRAMARINE;
									fixed_t bluemaxoffset = EXP_STABLERATE;
									factor = factor - stablerateinverse;
									factor = FRACUNIT - FixedDiv(factor, bluemaxoffset);
								}

								auto transflag = K_GetTransFlagFromFixed(factor, false);
								drawer_gametype
									.xy(0, 1)
									.colorize(static_cast<skincolornum_t>(overlaycolor))
									.flags(transflag)
									.patch("K_SPTEXP");

								drawer_gametype
									.xy(23, 1)
									.align(srb2::Draw::Align::kCenter)
									.font(srb2::Draw::Font::kPing)
									.text(va("%d", dta->exp));
								break;
							}
						}

						if (singlePlayer)
						{
							srb2::Draw drawer_rings = drawer_gametype.xy(36, 0);

							if (lvl->event == GPEVENT_NONE)
							{
								drawer_rings
									.xy(0, -1)
									.patch("K_SRING1");

								drawer_rings
									.xy(22, 1)
									.colormap(TC_RAINBOW, SKINCOLOR_YELLOW)
									.align(srb2::Draw::Align::kCenter)
									.font(srb2::Draw::Font::kPing)
									.text(va("%d", dta->rings));
							}

							srb2::Draw drawer_timer = drawer_rings.xy(36, 0);

							drawer_timer
								.xy(0, 0)
								.patch("K_STTIMS");

							drawer_timer
								.xy(32, 1)
								.align(srb2::Draw::Align::kCenter)
								.font(srb2::Draw::Font::kPing)
								.text(lvl->time == UINT32_MAX ?
									"--'--\"--" : va(
									"%i'%02i\"%02i",
									G_TicsToMinutes(lvl->time, true),
									G_TicsToSeconds(lvl->time),
									G_TicsToCentiseconds(lvl->time)
								));
						}
					}
				}

				drawer_perplayer = drawer_perplayer.x(56);
			}

			drawer_line = drawer_line.y(12);
		}
	}

	if (state >= PODIUM_ST_TOTALS_SLIDEIN)
	{
		srb2::Draw drawer_totals = drawer
			.xy(BASEVIDWIDTH * 0.5, BASEVIDHEIGHT - 48.0);

		srb2::Draw drawer_totals_left = drawer_totals
			.x(-144.0);

		srb2::Draw drawer_totals_right = drawer_totals
			.x(72.0);

		if (state == PODIUM_ST_TOTALS_SLIDEIN)
		{
			drawer_totals_left = drawer_totals_left.x( transition_i * -BASEVIDWIDTH );
			drawer_totals_right = drawer_totals_right.x( transition_i * BASEVIDWIDTH );
		}

		drawer_totals_left
			.xy(8.0, 8.0)
			.patch("R_RTPBR");

		skincolornum_t continuesColor = SKINCOLOR_NONE;
		if (rank.continuesUsed == 0)
		{
			continuesColor = SKINCOLOR_GOLD;
		}
		else if (rank.scoreContinues < 0)
		{
			continuesColor = SKINCOLOR_RED;
		}

		drawer_totals_left
			.y(24.0)
			.patch("RANKCONT");

		drawer_totals_left
			.xy(44.0, 24.0)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kTimer)
			.colormap( TC_RAINBOW, continuesColor )
			.text(va("%d", rank.continuesUsed));

		drawer_totals_left
			.xy(44.0, 38.0)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kZVote)
			.colormap( TC_RAINBOW, continuesColor )
			.text(va("%c%d", (rank.scoreContinues >= 0 ? '+' : ' '), rank.scoreContinues));

		drawer_totals_left
			.patch("RANKRING");

		drawer_totals_left
			.xy(44.0, 0.0)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kThinTimer)
			.text(va("%d / %d", rank.rings, rank.totalRings));

		drawer_totals_left
			.xy(44.0, 14.0)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kZVote)
			.text(va("%c%d", (rank.scoreRings > 0 ? '+' : ' '), rank.scoreRings));

		drawer_totals_right
			.xy(16.0, 49.0)
			.patch("CAPS_ZB");

		drawer_totals_right
			.xy(50.0, 24.0)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kThinTimer)
			.text(va("%d / %d", rank.prisons, rank.totalPrisons));

		drawer_totals_right
			.xy(50.0, 38.0)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kZVote)
			.text(va("%c%d", (rank.scorePrisons > 0 ? '+' : ' '), rank.scorePrisons));

		drawer_totals_right
			.colorize(static_cast<skincolornum_t>(SKINCOLOR_MUSTARD))
			.patch("K_STEXP");

		// Colorize the crystal for the totals, just like we do for in race hud
		fixed_t extraexpfactor = (EXP_MAX*FRACUNIT) / EXP_TARGET;
		INT16 totalExpMax = FixedMul(rank.totalExp*FRACUNIT, extraexpfactor) / FRACUNIT; // im just going to calculate it from target lol
		INT16 totalExpMin = rank.numPlayers*EXP_MIN;
		skincolornum_t overlaycolor = SKINCOLOR_MUSTARD;
		fixed_t stablerateinverse = FRACUNIT - EXP_STABLERATE;
		INT16 exp_range = totalExpMax-totalExpMin;
		INT16 exp_offset = rank.exp-totalExpMin;
		fixed_t factor = (exp_offset*FRACUNIT) / exp_range; // 0.0 to 1.0 in fixed
		// amount of blue is how much factor is above EXP_STABLERATE, and amount of red is how much factor is below
		// assume that EXP_STABLERATE is within 0.0 to 1.0 in fixed
		if (factor <= stablerateinverse)
		{
			overlaycolor = SKINCOLOR_RUBY;
			factor = FixedDiv(factor, stablerateinverse);
		}
		else
		{
			overlaycolor = SKINCOLOR_ULTRAMARINE;
			fixed_t bluemaxoffset = EXP_STABLERATE;
			factor = factor - stablerateinverse;
			factor = FRACUNIT - FixedDiv(factor, bluemaxoffset);
		}

		auto transflag = K_GetTransFlagFromFixed(factor, false);
		drawer_totals_right
			.colorize(static_cast<skincolornum_t>(overlaycolor))
			.flags(transflag)
			.patch("K_STEXP");

		drawer_totals_right
			.xy(50.0, 0.0)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kThinTimer)
			.text(va("%d / %d", rank.exp, rank.totalExp));

		drawer_totals_right
			.xy(50.0, 14.0)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kZVote)
			.text(va("%c%d", (rank.scoreExp > 0 ? '+' : ' '), rank.scoreExp));
	}

	if ((state == PODIUM_ST_GRADE_APPEAR && delay == 0)
		|| state >= PODIUM_ST_GRADE_VOICE)
	{
		char grade_letter = K_GetGradeChar( static_cast<gp_rank_e>(grade) );

		patch_t *grade_img = static_cast<patch_t*>( W_CachePatchName(va("R_FINRN%c", grade_letter), PU_CACHE) );
		srb2::Draw grade_drawer = drawer
			.xy(BASEVIDWIDTH * 0.5, BASEVIDHEIGHT - 2.0 - (grade_img->height * 0.5))
			.colormap( static_cast<skincolornum_t>(K_GetGradeColor( static_cast<gp_rank_e>(grade) )) );

		if (rank.specialWon == true)
		{
			UINT8 emeraldNum = g_podiumData.emeraldnum;

			const boolean emeraldBlink = (leveltime & 1);
			patch_t *emeraldOverlay = nullptr;
			patch_t *emeraldUnderlay = nullptr;
			skincolornum_t emeraldColor = SKINCOLOR_NONE;

			if (emeraldNum == 0)
			{
				// Prize -- todo, currently using fake Emerald
				emeraldColor = SKINCOLOR_GOLD;
			}
			else
			{
				emeraldColor = static_cast<skincolornum_t>( SKINCOLOR_CHAOSEMERALD1 + ((emeraldNum - 1) % 7) );
			}

			{
				if (emeraldNum > 7)
				{
					emeraldOverlay = static_cast<patch_t*>( W_CachePatchName("SEMRA0", PU_CACHE) );
					emeraldUnderlay = static_cast<patch_t*>( W_CachePatchName("SEMRB0", PU_CACHE) );
				}
				else
				{
					emeraldOverlay = static_cast<patch_t*>( W_CachePatchName("EMRCA0", PU_CACHE) );
					emeraldUnderlay = static_cast<patch_t*>( W_CachePatchName("EMRCB0", PU_CACHE) );
				}
			}

			srb2::Draw emerald_drawer = grade_drawer
				.xy(-48, 20)
				.colormap(emeraldColor)
				.scale(0.5);

			if (emeraldBlink)
			{
				emerald_drawer
					.flags(V_ADD)
					.patch(emeraldOverlay);

				if (emeraldUnderlay != nullptr)
				{
					emerald_drawer
						.patch(emeraldUnderlay);
				}
			}
			else
			{
				emerald_drawer
					.patch(emeraldOverlay);
			}
		}

		grade_drawer
			.xy(48, -2)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kMenu)
			.text("TOTAL");

		grade_drawer
			.xy(48, 8)
			.align(srb2::Draw::Align::kCenter)
			.font(srb2::Draw::Font::kThinTimer)
			.text(va("%d", rank.scoreTotal));

		float sc = 1.0;
		if (state == PODIUM_ST_GRADE_APPEAR)
		{
			sc += transition_i * 8.0;
		}

		grade_drawer
			.xy(-grade_img->width * 0.5 * sc, -grade_img->height * 0.5 * sc)
			.scale(sc)
			.patch(grade_img);
	}

	if (state >= PODIUM_ST_DATA_SLIDEIN)
	{
		K_DrawKartPositionNumXY(
			rank.position, 1,
			(drawer_winner.x() + 36) * FRACUNIT, (drawer_winner.y() + 2) * FRACUNIT,
			FRACUNIT, drawer_winner.flags(),
			leveltime,
			((mapheaderinfo[gamemap - 1]->levelflags & LF_SUBTRACTNUM) == LF_SUBTRACTNUM),
			true,
			true,
			(rank.position > 3)
		);

		if (state == PODIUM_ST_DONE)
		{
			Y_DrawIntermissionButton(delay, 0, true);
		}
		else if (state == PODIUM_ST_EXIT)
		{
			Y_DrawIntermissionButton(-1, (2*TICRATE) - delay, true);
		}
	}
}

/*--------------------------------------------------
	boolean K_PodiumSequence(void)

		See header file for description.
--------------------------------------------------*/
boolean K_PodiumSequence(void)
{
	return (gamestate == GS_CEREMONY);
}

/*--------------------------------------------------
	boolean K_PodiumRanking(void)

		See header file for description.
--------------------------------------------------*/
boolean K_PodiumRanking(void)
{
	return (gamestate == GS_CEREMONY && g_podiumData.ranking == true);
}

/*--------------------------------------------------
	boolean K_PodiumGrade(void)

		See header file for description.
--------------------------------------------------*/
gp_rank_e K_PodiumGrade(void)
{
	if (K_PodiumSequence() == false)
	{
		return GRADE_E;
	}

	return g_podiumData.grade;
}

/*--------------------------------------------------
	boolean K_PodiumHasEmerald(void)

		See header file for description.
--------------------------------------------------*/
boolean K_PodiumHasEmerald(void)
{
	if (K_PodiumSequence() == false)
	{
		return false;
	}

	return g_podiumData.rank.specialWon;
}

/*--------------------------------------------------
	UINT8 K_GetPodiumPosition(player_t *player)

		See header file for description.
--------------------------------------------------*/
UINT8 K_GetPodiumPosition(player_t *player)
{
	UINT8 position = 1;
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *other = NULL;
		if (playeringame[i] == false)
		{
			continue;
		}

		other = &players[i];
		if (other->bot == false && other->spectator == true)
		{
			continue;
		}

		if (other->score > player->score)
		{
			// Final score is the important part.
			position++;
		}
		else if (other->score == player->score)
		{
			if (other->bot == false && player->bot == true)
			{
				// Bots are never as important as players.
				position++;
			}
			else if (i < player - players)
			{
				// Port priority is the final tie breaker.
				position++;
			}
		}
	}

	return position;
}

/*--------------------------------------------------
	static void K_SetPodiumWaypoint(player_t *const player, waypoint_t *const waypoint)

		Changes the player's current and next waypoints, for
		use during the podium sequence.

	Input Arguments:-
		player - The player to update the waypoints of.
		waypoint - The new current waypoint.

	Return:-
		None
--------------------------------------------------*/
static void K_SetPodiumWaypoint(player_t *const player, waypoint_t *const waypoint)
{
	// Set the new waypoint.
	player->currentwaypoint = waypoint;

	if ((waypoint == NULL)
		|| (waypoint->nextwaypoints == NULL)
		|| (waypoint->numnextwaypoints == 0U))
	{
		// No waypoint, or no next waypoint.
		player->nextwaypoint = NULL;
		return;
	}

	// Simply use the first available next waypoint.
	// No need for split paths in these cutscenes.
	player->nextwaypoint = waypoint->nextwaypoints[0];
}

/*--------------------------------------------------
	void K_InitializePodiumWaypoint(player_t *const player)

		See header file for description.
--------------------------------------------------*/
void K_InitializePodiumWaypoint(player_t *const player)
{
	if ((player != NULL) && (player->mo != NULL))
	{
		if (player->position == 0)
		{
			// Just in case a netgame scenario with a late joiner ocurrs.
			player->position = K_GetPodiumPosition(player);
		}

		if (player->position > 0 && player->position <= MAXPLAYERS)
		{
			// Initialize our first waypoint to the one that
			// matches our position.
			K_SetPodiumWaypoint(player, K_GetWaypointFromID(player->position));
		}
		else
		{
			// None does, so remove it if we happen to have one.
			K_SetPodiumWaypoint(player, NULL);
		}
	}
}

/*--------------------------------------------------
	void K_UpdatePodiumWaypoints(player_t *const player)

		See header file for description.
--------------------------------------------------*/
void K_UpdatePodiumWaypoints(player_t *const player)
{
	if ((player != NULL) && (player->mo != NULL))
	{
		if (player->currentwaypoint != NULL)
		{
			const fixed_t xydist = P_AproxDistance(
				player->mo->x - player->currentwaypoint->mobj->x,
				player->mo->y - player->currentwaypoint->mobj->y
			);
			const fixed_t xyzdist = P_AproxDistance(
				xydist,
				player->mo->z - player->currentwaypoint->mobj->z
			);
			//const fixed_t speed = P_AproxDistance(player->mo->momx, player->mo->momy);

			if (xyzdist <= player->mo->radius + player->currentwaypoint->mobj->radius)
			{
				// Reached waypoint, go to the next waypoint.
				K_SetPodiumWaypoint(player, player->nextwaypoint);
			}
		}
	}
}

/*--------------------------------------------------
	boolean K_StartCeremony(void)

		See header file for description.
--------------------------------------------------*/
boolean K_StartCeremony(void)
{
	if (grandprixinfo.gp == false)
	{
		return false;
	}

	INT32 i;
	INT32 podiumMapNum = NEXTMAP_INVALID;

	if (grandprixinfo.cup != NULL
	&& grandprixinfo.cup->cachedlevels[CUPCACHE_PODIUM] != NEXTMAP_INVALID)
	{
		podiumMapNum = grandprixinfo.cup->cachedlevels[CUPCACHE_PODIUM];
	}
	else if (podiummap)
	{
		podiumMapNum = G_MapNumber(podiummap);
	}

	if (podiumMapNum < nummapheaders
		&& mapheaderinfo[podiumMapNum]
		&& mapheaderinfo[podiumMapNum]->lumpnum != LUMPERROR)
	{
		gamemap = podiumMapNum+1;
		g_reloadingMap = false;

		encoremode = grandprixinfo.encore;

		if (savedata.lives > 0)
		{
			K_LoadGrandPrixSaveGame();
			savedata.lives = 0;
		}

		// Make sure all of the GAME OVER'd players can spawn
		// and be present for the podium
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				if (players[i].lives < 1)
					players[i].lives = 1;

				if (players[i].bot)
					players[i].spectator = false;
			}
		}

		G_SetGametype(GT_RACE);
		G_DoLoadLevelEx(false, GS_CEREMONY);
		wipegamestate = GS_CEREMONY; // I don't know what else to do here

		r_splitscreen = 0; // Only one screen for the ceremony
		R_ExecuteSetViewSize();
		return true;
	}

	return false;
}

/*--------------------------------------------------
	void K_FinishCeremony(void)

		See header file for description.
--------------------------------------------------*/
void K_FinishCeremony(void)
{
	if (K_PodiumSequence() == false)
	{
		return;
	}

	g_podiumData.ranking = true;
	g_fast_forward = 0;
}

/*--------------------------------------------------
	void K_ResetCeremony(void)

		See header file for description.
--------------------------------------------------*/
void K_ResetCeremony(void)
{
	SINT8 i;

	memset(&g_podiumData, 0, sizeof(struct podiumData_s));

	if (K_PodiumSequence() == false)
	{
		return;
	}

	// Establish rank and grade for this play session.
	g_podiumData.Init();

	// Set up music for podium.
	{
		if (g_podiumData.rank.position == 1)
		{
			mapmusrng = 2;
		}
		else if (g_podiumData.rank.position <= RANK_NEUTRAL_POSITION)
		{
			mapmusrng = 1;
		}
		else
		{
			mapmusrng = 0;
		}

		UINT8 limit = (encoremode && mapheaderinfo[gamemap-1]->encoremusname_size)
			? mapheaderinfo[gamemap-1]->encoremusname_size
			: mapheaderinfo[gamemap-1]->musname_size;

		if (limit < 1)
			limit = 1;

		while (mapmusrng >= limit)
		{
			mapmusrng--;
		}
	}

	if (!grandprixinfo.cup)
	{
		return;
	}

	cupheader_t *emeraldcup = NULL;

	if (gamedata->sealedswaps[GDMAX_SEALEDSWAPS-1] != NULL // all found
	|| grandprixinfo.cup->id >= basenumkartcupheaders // custom content
	|| M_SecretUnlocked(SECRET_SPECIALATTACK, false)) // true order
	{
		// Standard order.
		emeraldcup = grandprixinfo.cup;
	}
	else
	{
		// Determine order from sealedswaps.
		for (i = 0; i < GDMAX_SEALEDSWAPS; i++)
		{
			if (gamedata->sealedswaps[i] == NULL)
			{
				if (g_podiumData.rank.specialWon == true)
				{
					// First visit! Mark it off.
					gamedata->sealedswaps[i] = grandprixinfo.cup;
				}

				break;
			}

			if (gamedata->sealedswaps[i] != grandprixinfo.cup)
				continue;

			// Repeat visit, grab the same ID.
			break;
		}

		// If there's pending stars, apply them to the new cup order.
		if (i < GDMAX_SEALEDSWAPS)
		{
			emeraldcup = kartcupheaders;
			while (emeraldcup)
			{
				if (emeraldcup->id >= basenumkartcupheaders)
				{
					emeraldcup = NULL;
					break;
				}

				if (emeraldcup->emeraldnum == i+1)
					break;

				emeraldcup = emeraldcup->next;
			}

			g_podiumData.emeraldnum = i+1;
		}
	}

	// Write grade, position, and emerald-having-ness for later sessions!
	i = (grandprixinfo.masterbots) ? KARTGP_MASTER : grandprixinfo.gamespeed;

	// All results populate downwards in difficulty. This prevents someone
	// who's just won on Normal from feeling obligated to complete Easy too.
	for (; i >= 0; i--)
	{
		boolean anymerit = false;

		if ((grandprixinfo.cup->windata[i].best_placement == 0) // First run
			|| (g_podiumData.rank.position <= grandprixinfo.cup->windata[i].best_placement)) // Later, better run
		{
			grandprixinfo.cup->windata[i].best_placement = g_podiumData.rank.position;

			// The following will not occur in unmodified builds, but pre-emptively sanitise gamedata if someone just changes MAXPLAYERS and calls it a day
			if (grandprixinfo.cup->windata[i].best_placement > 0x0F)
			{
				grandprixinfo.cup->windata[i].best_placement = 0x0F;
			}

			anymerit = true;
		}

		if (g_podiumData.grade >= grandprixinfo.cup->windata[i].best_grade)
		{
			grandprixinfo.cup->windata[i].best_grade = g_podiumData.grade;
			anymerit = true;
		}

		if (g_podiumData.rank.specialWon == true && emeraldcup)
		{
			emeraldcup->windata[i].got_emerald = true;
			anymerit = true;
		}

		if (anymerit == true)
		{
			grandprixinfo.cup->windata[i].best_skin.id = g_podiumData.rank.skin;
			grandprixinfo.cup->windata[i].best_skin.unloaded = NULL;
		}
	}

	// Update visitation.
	prevmap = gamemap-1;
	G_UpdateVisited();

	// will subsequently save in P_LoadLevel
}

/*--------------------------------------------------
	void K_CeremonyTicker(boolean run)

		See header file for description.
--------------------------------------------------*/
void K_CeremonyTicker(boolean run)
{
	// don't trigger if doing anything besides idling
	if (gameaction != ga_nothing || gamestate != GS_CEREMONY)
	{
		return;
	}

	P_TickAltView(&titlemapcam);

	if (titlemapcam.mobj != NULL)
	{
		camera[0].x = titlemapcam.mobj->x;
		camera[0].y = titlemapcam.mobj->y;
		camera[0].z = titlemapcam.mobj->z;
		camera[0].angle = titlemapcam.mobj->angle;
		camera[0].aiming = titlemapcam.mobj->pitch;
		camera[0].subsector = titlemapcam.mobj->subsector;
	}

	if (g_podiumData.ranking == false)
	{
		if (run == true)
		{
			if (g_podiumData.fastForward == true)
			{
				if (g_fast_forward == 0)
				{
					// Possibly an infinite loop, finalize even if we're still in the middle of the cutscene.
					K_FinishCeremony();
				}
			}
			else
			{
				if (menuactive == false && M_MenuConfirmPressed(0) == true)
				{
					if (!netgame)
					{
						constexpr tic_t kSkipToTime = 60 * TICRATE;
						if (kSkipToTime > leveltime)
						{
							g_fast_forward = kSkipToTime - leveltime;
						}
					}

					g_podiumData.fastForward = true;
				}
			}
		}

		return;
	}

	if (run == true)
	{
		g_podiumData.Tick();
	}
}

/*--------------------------------------------------
	void K_CeremonyDrawer(void)

		See header file for description.
--------------------------------------------------*/
void K_CeremonyDrawer(void)
{
	if (g_podiumData.ranking == false)
	{
		// not ready to draw.
		return;
	}

	g_podiumData.Draw();
}
