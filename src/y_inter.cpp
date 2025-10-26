// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  y_inter.cpp
/// \brief Tally screens, or "Intermissions" as they were formally called in Doom

#include <algorithm>

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_net.h"
#include "i_video.h"
#include "p_tick.h"
#include "r_defs.h"
#include "r_skins.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "y_inter.h"
#include "z_zone.h"
#include "k_menu.h"
#include "m_misc.h"
#include "i_system.h"
#include "p_setup.h"

#include "r_local.h"
#include "r_fps.h"
#include "p_local.h"

#include "m_cond.h" // condition sets
#include "lua_hook.h" // IntermissionThinker hook

#include "lua_hud.h"
#include "lua_hudlib_drawlist.h"

#include "m_random.h" // M_RandomKey
#include "g_input.h" // G_PlayerInputDown
#include "k_hud.h" // K_DrawMapThumbnail
#include "k_battle.h"
#include "k_boss.h"
#include "k_kart.h"
#include "k_pwrlv.h"
#include "k_grandprix.h"
#include "k_serverstats.h" // SV_BumpMatchStats
#include "m_easing.h"
#include "music.h"

#include "v_draw.hpp"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

typedef struct
{
	char patch[9];
	 INT32 points;
	UINT8 display;
} y_bonus_t;

static y_data_t data;

// graphics
static patch_t *bgpatch = NULL;     // INTERSCR
static patch_t *widebgpatch = NULL;
static patch_t *bgtile = NULL;      // SPECTILE/SRB2BACK
static patch_t *interpic = NULL;    // custom picture defined in map header

#define INFINITE_TIMER (INT16_MAX) // just some arbitrarily large value that won't easily overflow
static INT32 timer;
static INT32 powertype = PWRLV_DISABLED;

static INT32 intertic;
static INT32 endtic = -1;
static INT32 sorttic = -1;

static fixed_t mqscroll = 0;
static fixed_t chkscroll = 0;
static fixed_t ttlscroll = 0;

intertype_t intertype = int_none;

static huddrawlist_h luahuddrawlist_intermission;

static boolean Y_CanSkipIntermission(void)
{
	if (!netgame)
	{
		return true;
	}

	return false;
}

boolean Y_IntermissionPlayerLock(void)
{
	return (gamestate == GS_INTERMISSION && data.rankingsmode == false);
}

static void Y_UnloadData(void);

//
// SRB2Kart - Y_CalculateMatchData and ancillary functions
//
static void Y_CompareTime(INT32 i)
{
	UINT32 val = ((players[i].pflags & PF_NOCONTEST || players[i].realtime == UINT32_MAX)
		? (UINT32_MAX-1) : players[i].realtime);

	if (!(val < data.val[data.numplayers]))
		return;

	data.val[data.numplayers] = val;
	data.num[data.numplayers] = i;
}

static void Y_CompareScore(INT32 i)
{
	UINT32 val = ((players[i].pflags & PF_NOCONTEST)
			? (UINT32_MAX-1) : players[i].roundscore);

	if (!(data.val[data.numplayers] == UINT32_MAX
	|| (!(players[i].pflags & PF_NOCONTEST) && val > data.val[data.numplayers])))
		return;

	data.val[data.numplayers] = val;
	data.num[data.numplayers] = i;
}

static void Y_CompareRank(INT32 i)
{
	INT16 increase = ((data.increase[i] == INT16_MIN) ? 0 : data.increase[i]);
	UINT32 score = players[i].score;

	if (powertype != PWRLV_DISABLED)
	{
		score = clientpowerlevels[i][powertype];
	}

	if (!(data.val[data.numplayers] == UINT32_MAX || (score - increase) > data.val[data.numplayers]))
		return;

	data.val[data.numplayers] = (score - increase);
	data.num[data.numplayers] = i;
}

static void Y_CalculateMatchData(UINT8 rankingsmode, void (*comparison)(INT32))
{
	INT32 i, j;
	boolean completed[MAXPLAYERS];
	INT32 numplayersingame = 0;
	boolean getmainplayer = false;
	UINT32 topscore = 0, btopemeralds = 0;

	// Initialize variables
	if (rankingsmode > 1)
		;
	else if ((data.rankingsmode = (boolean)rankingsmode))
	{
		sprintf(data.headerstring, "Total Rankings");
		data.gotthrough = false;
	}
	else
	{
		getmainplayer = true;

		data.encore = encoremode;

		memset(data.jitter, 0, sizeof (data.jitter));
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		data.val[i] = UINT32_MAX;
		data.grade[i] = GRADE_INVALID;

		if (!playeringame[i] || players[i].spectator)
		{
			data.increase[i] = INT16_MIN;
			continue;
		}
		
		// for getting the proper maximum value for score-to-EXP conversion
		if (gametype == GT_BATTLE)
		{
			if ((&players[i])->roundscore > topscore)
			{
				topscore = (&players[i])->roundscore;
			}
			if (K_NumEmeralds(&players[i]) > btopemeralds)
			{
				btopemeralds = K_NumEmeralds(&players[i]); // necessary so non-emerald wins can still get max EXP if no one else is holding more emeralds
			}
		}
		
		if (K_InRaceDuel() == true)
		{
			if (((UINT32)(&players[i])->duelscore) > topscore)
			{
				topscore = (&players[i])->duelscore;
			}
		}

		if (!rankingsmode)
			data.increase[i] = INT16_MIN;

		numplayersingame++;
	}

	memset(completed, 0, sizeof (completed));
	data.numplayers = 0;
	data.showroundnum = false;

	data.isduel = (numplayersingame <= 2);

	srb2::StandingsJson standings {};
	bool savestandings = (!rankingsmode && demo.recording);

	// Team stratification (this code only barely supports more than 2 teams)
	data.winningteam = TEAM_UNASSIGNED;
	data.halfway = UINT8_MAX;

	UINT8 countteam[TEAM__MAX];
	UINT8 smallestteam = UINT8_MAX;
	memset(countteam, 0, sizeof(countteam));

	if (rankingsmode == 0 && G_GametypeHasTeams())
	{
		for (i = data.winningteam+1; i < TEAM__MAX; i++)
		{
			countteam[i] = G_CountTeam(i);

			if (g_teamscores[data.winningteam] < g_teamscores[i])
			{
				data.winningteam = i;
			}

			if (smallestteam > countteam[i])
			{
				smallestteam = countteam[i];
			}
		}

		if (countteam[data.winningteam])
		{
			data.halfway = countteam[data.winningteam] - 1;
		}
	}

	for (j = 0; j < numplayersingame; j++)
	{
		i = 0;

		if (data.winningteam != TEAM_UNASSIGNED)
		{
			for (; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator || completed[i])
					continue;

				if (players[i].team != data.winningteam)
					continue;

				comparison(i);
			}

			if (data.val[data.numplayers] == UINT32_MAX)
			{
				// Only run the un-teamed loop if everybody
				// on the winning team was previously placed
				i = 0;
			}
		}

		for (; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator || completed[i])
				continue;

			comparison(i);
		}

		i = data.num[data.numplayers];

		completed[i] = true;
		data.grade[i] = K_PlayerTallyActive(&players[i]) ? players[i].tally.rank : GRADE_INVALID;

		if (data.numplayers && (data.val[data.numplayers] == data.val[data.numplayers-1]))
		{
			data.pos[data.numplayers] = data.pos[data.numplayers-1];
		}
		else
		{
			data.pos[data.numplayers] = data.numplayers+1;
		}

#define strtime data.strval[data.numplayers]

		strtime[0] = '\0';

		if (!rankingsmode)
		{
			// Online rank is handled further below in this file.
			if (powertype == PWRLV_DISABLED)
			{
				UINT8 pointgetters = numplayersingame + spectateGriefed;
				UINT32 scoreconversion = 0;
				UINT32 pscore = 0;

				// accept players that nocontest, but not bots
				if (data.pos[data.numplayers] <= pointgetters &&
					!((players[i].pflags & PF_NOCONTEST) && players[i].bot))
				{
					if (gametype == GT_BATTLE)
					{
						pscore = (&players[i])->roundscore + K_NumEmeralds(&players[i]);
						scoreconversion = FixedRescale(pscore, 0, topscore + btopemeralds, Easing_Linear, EXP_MIN, EXP_MAX);
						data.increase[i] = K_CalculateGPRankPoints(scoreconversion, data.pos[data.numplayers], pointgetters);
					}
					else
					{
						// For Duel scoring, convert duelscore into EXP.
						if (K_InRaceDuel())
						{
							pscore = (&players[i])->duelscore;
							scoreconversion = FixedRescale(pscore, 0, topscore, Easing_Linear, EXP_MIN, EXP_MAX);
							data.increase[i] = K_CalculateGPRankPoints(scoreconversion, data.pos[data.numplayers], pointgetters);
						}
						else
						{
							data.increase[i] = K_CalculateGPRankPoints((&players[i])->exp, data.pos[data.numplayers], pointgetters);
						}
					}

					if (data.winningteam != TEAM_UNASSIGNED)
					{
						if (smallestteam != 0
						&& players[i].team != data.winningteam)
						{
							data.increase[i] /= 2;
						}
					}
				}

				if (data.increase[i] > 0)
				{
					players[i].score += data.increase[i];
				}
			}

			if (savestandings)
			{
				srb2::StandingJson standing {};
				standing.ranking = data.pos[data.numplayers];
				standing.name = srb2::String(player_names[i]);
				standing.demoskin = players[i].skin;
				standing.skincolor = srb2::String(skincolors[players[i].skincolor].name);
				standing.timeorscore = data.val[data.numplayers];
				standings.standings.emplace_back(std::move(standing));
			}

			if (data.val[data.numplayers] == (UINT32_MAX-1))
				STRBUFCPY(strtime, "RETIRED.");
			else
			{
				if (intertype == int_time)
				{
					snprintf(strtime, sizeof strtime, "%i'%02i\"%02i", G_TicsToMinutes(data.val[data.numplayers], true),
					G_TicsToSeconds(data.val[data.numplayers]), G_TicsToCentiseconds(data.val[data.numplayers]));
				}
				else
				{
					snprintf(strtime, sizeof strtime, "%d", data.val[data.numplayers]);
				}
			}
		}
		else
		{
			if (powertype != PWRLV_DISABLED && !clientpowerlevels[i][powertype])
			{
				// No power level (guests)
				STRBUFCPY(strtime, "----");
			}
			else
			{
				snprintf(strtime, sizeof strtime, "%d", data.val[data.numplayers]);
			}
		}

		strtime[sizeof strtime - 1] = '\0';

#undef strtime

		data.numplayers++;
	}

	if (data.numplayers <= 2
		|| data.halfway == UINT8_MAX
		|| data.halfway >= 8
		|| (data.numplayers - data.halfway) >= 8)
	{
		data.halfway = (data.numplayers-1)/2;
	}

	if (savestandings)
	{
		srb2::write_current_demo_end_marker();
		srb2::write_current_demo_standings(standings);
	}

	if (getmainplayer == true)
	{
		// Okay, player scores have been set now - we can calculate GP-relevant material.
		{
			if (grandprixinfo.gp == true)
			{
				K_UpdateGPRank(&grandprixinfo.rank);
			}

			// See also G_GetNextMap, M_DrawPause
			data.showrank = false;
			if (grandprixinfo.gp == true
				&& netgame == false // TODO netgame Special Mode support
				&& grandprixinfo.gamespeed >= KARTSPEED_NORMAL
				&& roundqueue.size > 1
				&& roundqueue.entries[roundqueue.size - 1].rankrestricted == true
			)
			{
				if (roundqueue.position == roundqueue.size-1)
				{
					// On A rank pace? Then you get a chance for S rank!
					fixed_t rankforline = K_CalculateGPPercent(&grandprixinfo.rank);
					fixed_t required = K_SealedStarEntryRequirement(&grandprixinfo.rank);

					data.showrank = (rankforline >= required);

					data.linemeter =
						(std::min(rankforline, required)
							* (2 * TICRATE)
						) / required;

					// A little extra time to take it all in
					timer += TICRATE;
				}

				if (gamedata->everseenspecial == true
					|| roundqueue.position == roundqueue.size)
				{
					// Additional cases in which it should always be shown.
					data.showrank = true;
				}
			}
		}

		i = MAXPLAYERS;

		for (j = 0; j < data.numplayers; j++)
		{
			i = data.num[j];

			if (i >= MAXPLAYERS
				|| playeringame[i] == false
				|| players[i].spectator == true)
			{
				continue;
			}

			if (demo.playback)
			{
				if (!P_IsDisplayPlayer(&players[i]))
				{
					continue;
				}

				break;
			}

			if (!P_IsPartyPlayer(&players[i]))
			{
				continue;
			}

			break;
		}

		data.headerstring[0] = '\0';
		data.gotthrough = false;
		data.mainplayer = MAXPLAYERS;

		if (j < data.numplayers)
		{
			data.mainplayer = i;

			if (data.winningteam != TEAM_UNASSIGNED
			&& players[i].team != TEAM_UNASSIGNED)
			{
				data.gotthrough = true;

				snprintf(data.headerstring,
					sizeof data.headerstring,
					"%s TEAM",
					g_teaminfo[players[i].team].name);

				data.showroundnum = true;
			}
			else if (!(players[i].pflags & PF_NOCONTEST))
			{
				data.gotthrough = true;

				if (players[i].skin < numskins)
				{
					snprintf(data.headerstring,
						sizeof data.headerstring,
						"%s",
						R_CanShowSkinInDemo(players[i].skin) ? skins[players[i].skin]->realname : "???");
				}

				data.showroundnum = true;
			}
			else
			{
				snprintf(data.headerstring,
					sizeof data.headerstring,
					"NO CONTEST...");
			}
		}
		else
		{
			if (roundqueue.position > 0 && roundqueue.position <= roundqueue.size
				&& (grandprixinfo.gp == false || grandprixinfo.eventmode == GPEVENT_NONE))
			{
				snprintf(data.headerstring,
					sizeof data.headerstring,
					"ROUND");

				data.showroundnum = true;
			}
			else if (K_CheckBossIntro() == true && bossinfo.enemyname)
			{
				snprintf(data.headerstring,
					sizeof data.headerstring,
					"%s",
					bossinfo.enemyname);
			}
			else if (battleprisons == true)
			{
				snprintf(data.headerstring,
					sizeof data.headerstring,
					"PRISON BREAK");
			}
			else
			{
				snprintf(data.headerstring,
					sizeof data.headerstring,
					"%s STAGE",
					gametypes[gametype]->name);
			}
		}

		data.headerstring[sizeof data.headerstring - 1] = '\0';
	}
}

typedef enum
{
	BPP_AHEAD,
	BPP_DONE,
	BPP_MAIN,
	BPP_SHADOW = BPP_MAIN,
	BPP_MAX
} bottomprogressionpatch_t;

//
// Y_PlayerStandingsDrawer
//
// Handles drawing the center-of-screen player standings.
//
void Y_PlayerStandingsDrawer(y_data_t *standings, INT32 xoffset)
{
	if (standings->numplayers == 0)
	{
		return;
	}

	UINT8 i;

	SINT8 yspacing = 14;
	INT32 heightcount = (standings->numplayers - 1);

	INT32 x, y;
	INT32 x2, returny, inwardshim = 0;

	boolean verticalresults = (standings->numplayers < 4 && (standings->numplayers == 1 || standings->isduel == false));
	boolean datarightofcolumn = false;
	boolean drawping = (netgame && gamestate == GS_LEVEL);

	INT32 hilicol = highlightflags;

	patch_t *resbar = static_cast<patch_t*>(W_CachePatchName("R_RESBAR", PU_PATCH)); // Results bars for players
	patch_t *cpu = static_cast<patch_t*>(W_CachePatchName("K_CPU", PU_PATCH));

	if (drawping || standings->rankingsmode != 0)
	{
		inwardshim = 8;
	}

	if (verticalresults)
	{
		x = (BASEVIDWIDTH/2) - 61;
	}
	else
	{
		x = 29;
		inwardshim /= 2;
		heightcount /= 2;
	}

	x += xoffset + inwardshim;
	x2 = x;

	if (drawping)
	{
		x2 -= 9;
	}

	UINT8 halfway = standings->halfway;

	if (halfway > 4)
	{
		yspacing--;
	}
	else if (halfway <= 2)
	{
		yspacing++;
		if (verticalresults)
		{
			yspacing++;
		}
	}

	y = 106 - (heightcount * yspacing)/2;

	if (standings->isduel)
	{
		y += 38;
	}
	else if (y < 70)
	{
		// One sanity check.
		y = 70;
	}

	returny = y;

	boolean (*_isHighlightedPlayer)(const player_t *) =
		(demo.playback
			? P_IsDisplayPlayer
			: P_IsPartyPlayer
		);

	boolean doreverse = (
		standings->isduel && standings->numplayers == 2
		&& standings->num[0] > standings->num[1]
	);

	i = 0;

	if (doreverse)
	{
		i = standings->numplayers-1;
		halfway++;
	}

	do // don't use "continue" in this loop just for sanity's sake
	{
		const UINT8 pnum = standings->num[i];

		if (pnum == MAXPLAYERS)
			;
		else if (!playeringame[pnum] || players[pnum].spectator == true)
			standings->num[i] = MAXPLAYERS; // this should be the only field setting in this function
		else
		{
			UINT8 *charcolormap = NULL;
			if (!R_CanShowSkinInDemo(players[pnum].skin))
			{
				charcolormap = R_GetTranslationColormap(TC_BLINK, static_cast<skincolornum_t>(players[pnum].skincolor), GTC_CACHE);
			}
			else
			{
				charcolormap = R_GetTranslationColormap(players[pnum].skin, static_cast<skincolornum_t>(players[pnum].skincolor), GTC_CACHE);
			}

			if (standings->isduel)
			{
				INT32 duelx = x + 22 + (datarightofcolumn ? inwardshim : -inwardshim);
				INT32 duely = y - 80;

				V_DrawScaledPatch(duelx, duely, 0, static_cast<patch_t*>(W_CachePatchName("DUELGRPH", PU_CACHE)));
				V_DrawScaledPatch(duelx + 8, duely + 9, V_TRANSLUCENT, static_cast<patch_t*>(W_CachePatchName("PREVBACK", PU_CACHE)));

				UINT8 spr2 = SPR2_STIN;
				if (standings->pos[i] == 2)
				{
					spr2 = (datarightofcolumn ? SPR2_STGR : SPR2_STGL);
				}

				M_DrawCharacterSprite(
					duelx + 40, duely + 78,
					players[pnum].skin,
					spr2,
					(datarightofcolumn ? 1 : 7),
					0,
					0,
					charcolormap
				);

				duelx += 8;
				duely += 5;

				UINT8 j;
				for (j = 0; j <= splitscreen; j++)
				{
					if (pnum == g_localplayers[j])
						break;
				}

				INT32 letterpos = duelx + (datarightofcolumn ? 44 : 0);

				if (j > splitscreen || demo.playback)
				{
					// TODO: EGGA isn't strictly correct for demo playback since they're not really network players, but it's better than displaying local profile.
					V_DrawScaledPatch(letterpos, duely, 0, static_cast<patch_t*>(W_CachePatchName(va("CHAR%s", (players[pnum].bot ? "CPU" : "EGGA")), PU_CACHE)));
				}
				else
				{
					duelx += (datarightofcolumn ? -1 : 11);

					UINT8 profilen = cv_lastprofile[j].value;

					V_DrawScaledPatch(duelx, duely, 0, static_cast<patch_t*>(W_CachePatchName("FILEBACK", PU_CACHE)));

					if (datarightofcolumn && j == 0)
						letterpos++; // A is one pixel thinner

					V_DrawScaledPatch(letterpos, duely, 0, static_cast<patch_t*>(W_CachePatchName(va("CHARSEL%c", 'A' + j), PU_CACHE)));

					profile_t *pr = PR_GetProfile(profilen);

					V_DrawCenteredFileString(duelx+26, duely, 0, pr ? pr->profilename : "PLAYER");
				}
			}

			// Apply the jitter offset (later reversed)
			if (standings->jitter[pnum] > 0)
				y--;

			V_DrawMappedPatch(x, y, 0, resbar, NULL);

			if (gametype != GT_TUTORIAL)
			{
				V_DrawRightAlignedThinString(x+13, y-2, 0, va("%d", standings->pos[i]));
			}

			//if (players[pnum].skincolor != SKINCOLOR_NONE)
			{
				if ((players[pnum].pflags & PF_NOCONTEST) && players[pnum].bot)
				{
					// RETIRED !!
					V_DrawMappedPatch(
						x+14, y-5,
						0,
						static_cast<patch_t*>(W_CachePatchName("MINIDEAD", PU_CACHE)),
						R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(players[pnum].skincolor), GTC_CACHE)
					);
				}
				else
				{
					charcolormap = R_GetTranslationColormap(players[pnum].skin, static_cast<skincolornum_t>(players[pnum].skincolor), GTC_CACHE);
					V_DrawMappedPatch(x+14, y-5, 0,
						R_CanShowSkinInDemo(players[pnum].skin) ?
						faceprefix[players[pnum].skin][FACE_MINIMAP] : kp_unknownminimap,
						charcolormap);
				}
			}

/*			y2 = y;

			if ((netgame || (demo.playback && demo.netgame)) && playerconsole[pnum] == 0 && server_lagless && !players[pnum].bot)
			{
				static UINT8 alagles_timer = 0;
				patch_t *alagles;

				y2 = ( y - 4 );

				V_DrawScaledPatch(x + 36, y2, 0, W_CachePatchName(va("BLAGLES%d", (intertic / 3) % 6), PU_CACHE));
				// every 70 tics
				if (( leveltime % 70 ) == 0)
				{
					alagles_timer = 9;
				}
				if (alagles_timer > 0)
				{
					alagles = W_CachePatchName(va("ALAGLES%d", alagles_timer), PU_CACHE);
					V_DrawScaledPatch(x + 36, y2, 0, alagles);
					if (( leveltime % 2 ) == 0)
						alagles_timer--;
				}
				else
				{
					alagles = W_CachePatchName("ALAGLES0", PU_CACHE);
					V_DrawScaledPatch(x + 36, y2, 0, alagles);
				}

				y2 += SHORT (alagles->height) + 1;
			}*/

			V_DrawThinString(
				x+27, y-2,
				(
					_isHighlightedPlayer(&players[pnum])
						? hilicol
						: 0
				),
				player_names[pnum]
			);

			if (netgame && cv_voice_allowservervoice.value)
			{
				patch_t *voxpat;
				int voxxoffs = 0;
				int voxyoffs = 0;
				if (players[pnum].pflags2 & (PF2_SELFDEAFEN | PF2_SERVERDEAFEN))
				{
					voxpat = (patch_t*) W_CachePatchName("VOXCRD", PU_HUDGFX);
					voxxoffs = 1;
					voxyoffs = -5;
				}
				else if (players[pnum].pflags2 & (PF2_SELFMUTE | PF2_SERVERMUTE | PF2_SERVERTEMPMUTE))
				{
					voxpat = (patch_t*) W_CachePatchName("VOXCRM", PU_HUDGFX);
					voxxoffs = 1;
					voxyoffs = -6;
				}
				else if (S_IsPlayerVoiceActive(pnum))
				{
					voxpat = (patch_t*) W_CachePatchName("VOXCRA", PU_HUDGFX);
					voxyoffs = -4;
				}
				else
				{
					voxpat = NULL;
				}

				if (voxpat)
				{
					int namewidth = V_ThinStringWidth(player_names[pnum], 0);
					V_DrawFixedPatch((x + 27 + namewidth + voxxoffs) * FRACUNIT, (y + voxyoffs) * FRACUNIT, FRACUNIT, 0, voxpat, NULL);
				}
			}

			V_DrawRightAlignedThinString(
				x+118, y-2,
				0,
				standings->strval[i]
			);

			if (drawping)
			{
				if (players[pnum].bot)
				{
					V_DrawScaledPatch(
						x2-2 + (datarightofcolumn ? 2 : -2), y-2,
						0,
						cpu
					);
				}
				else
				{
					HU_drawPing(
						(x2 - 2) * FRACUNIT, (y-2) * FRACUNIT,
						playerpingtable[pnum],
						playerdelaytable[pnum],
						playerpacketlosstable[pnum],
						0,
						(datarightofcolumn ? 1 : -1)
					);
				}
			}
			else if (gamestate == GS_LEVEL)
				;
			else if (standings->rankingsmode != 0)
			{
				char *increasenum = NULL;

				if (standings->increase[pnum] != INT16_MIN)
				{
					increasenum = va(
						"(%d)",
						standings->increase[pnum]
					);
				}

				if (increasenum)
				{
					if (datarightofcolumn)
					{
						V_DrawThinString(
							x2, y-2,
							0,
							increasenum
						);
					}
					else
					{
						V_DrawRightAlignedThinString(
							x2, y-2,
							0,
							increasenum
						);
					}
				}
			}
			else if (standings->grade[pnum] != GRADE_INVALID)
			{
				patch_t *gradePtc = static_cast<patch_t*>(W_CachePatchName(va("R_INRNK%c", K_GetGradeChar(static_cast<gp_rank_e>(standings->grade[pnum]))), PU_PATCH));
				patch_t *gradeBG = NULL;

				UINT16 gradeColor = SKINCOLOR_NONE;
				UINT8 *gradeClm = NULL;

				gradeColor = K_GetGradeColor(static_cast<gp_rank_e>(standings->grade[pnum]));
				if (gradeColor != SKINCOLOR_NONE)
				{
					gradeClm = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(gradeColor), GTC_CACHE);
				}

				if (datarightofcolumn)
				{
					gradeBG = static_cast<patch_t*>(W_CachePatchName("R_INRNKR", PU_PATCH));
					V_DrawMappedPatch(x + 118, y, 0, gradeBG, gradeClm);
					V_DrawMappedPatch(x + 118 + 4, y - 1, 0, gradePtc, gradeClm);
				}
				else
				{
					gradeBG = static_cast<patch_t*>(W_CachePatchName("R_INRNKL", PU_PATCH));
					V_DrawMappedPatch(x - 12, y, 0, gradeBG, gradeClm);
					V_DrawMappedPatch(x - 12 + 3, y - 1, 0, gradePtc, gradeClm);
				}
			}

			// Reverse the jitter offset
			if (standings->jitter[pnum] > 0)
				y++;
		}

		y += yspacing;

		if (verticalresults == false && i == halfway)
		{
			x = 169 + xoffset - inwardshim;
			y = returny;

			datarightofcolumn = true;
			x2 = x + 118 + 5;
		}

		if (!doreverse)
		{
			if (++i < standings->numplayers)
				continue;
			break;
		}

		if (i == 0)
			break;
		i--;
	}
	while (true);

	if (standings->rankingsmode)
	{
		if (standings->numplayers < 2)
			;
		else if (standings->isduel)
		{
			x = BASEVIDWIDTH / 2 + xoffset;
			y = roundqueue.size ? (BASEVIDHEIGHT/2) : (BASEVIDHEIGHT - 19);
			Y_DrawRankMode(x, y, true);
		}
		else
		{
			Y_DrawRankMode(x + 122, returny - yspacing + 7, false);
		}
	}
}

//
// Y_RoundQueueDrawer
//
// Handles drawing the bottom-of-screen progression.
// Currently requires intermission y_data for animation only.
//
void Y_RoundQueueDrawer(y_data_t *standings, INT32 offset, boolean doanimations, boolean widescreen, boolean adminmode)
{
	if (roundqueue.size == 0)
	{
		if (!adminmode
		|| menuqueue.size == 0)
		{
			return;
		}
	}

	// The following is functionally a hack.
	// Due to how interpolation works, it's functionally one frame behind.
	// So we offset certain interpolated timers by this to make our lives easier!
	// This permits cues handled in the ticker and visuals to match up,
	// like the player pin reaching the Sealed Star the frame of the fade.
	// We also do this rather than doing extrapoleration because that would
	// still put 35fps in the future. ~toast 100523
	SINT8 interpoffs = (R_UsingFrameInterpolation() ? 1 : 0);

	UINT8 i;

	UINT8 *greymap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_GREY, GTC_CACHE);

	INT32 baseflags = 0;
	INT32 bufferspace = 0;

	if (widescreen)
	{
		baseflags |= V_SNAPTOBOTTOM;
		bufferspace = ((vid.width/vid.dupx) - BASEVIDWIDTH) / 2;
	}

	// Background pieces
	patch_t *queuebg_flat = static_cast<patch_t*>(W_CachePatchName("R_RMBG1", PU_PATCH));
	patch_t *queuebg_upwa = static_cast<patch_t*>(W_CachePatchName("R_RMBG2", PU_PATCH));
	patch_t *queuebg_down = static_cast<patch_t*>(W_CachePatchName("R_RMBG3", PU_PATCH));
	patch_t *queuebg_prize = static_cast<patch_t*>(W_CachePatchName("R_RMBG4", PU_PATCH));

	// Progression lines
	patch_t *line_upwa[BPP_MAX];
	patch_t *line_down[BPP_MAX];
	patch_t *line_flat[BPP_MAX];

	line_upwa[BPP_AHEAD] = static_cast<patch_t*>(W_CachePatchName("R_RRMLN1", PU_PATCH));
	line_upwa[BPP_DONE] = static_cast<patch_t*>(W_CachePatchName("R_RRMLN3", PU_PATCH));
	line_upwa[BPP_SHADOW] = static_cast<patch_t*>(W_CachePatchName("R_RRMLS1", PU_PATCH));

	line_down[BPP_AHEAD] = static_cast<patch_t*>(W_CachePatchName("R_RRMLN2", PU_PATCH));
	line_down[BPP_DONE] = static_cast<patch_t*>(W_CachePatchName("R_RRMLN4", PU_PATCH));
	line_down[BPP_SHADOW] = static_cast<patch_t*>(W_CachePatchName("R_RRMLS2", PU_PATCH));

	line_flat[BPP_AHEAD] = static_cast<patch_t*>(W_CachePatchName("R_RRMLN5", PU_PATCH));
	line_flat[BPP_DONE] = static_cast<patch_t*>(W_CachePatchName("R_RRMLN6", PU_PATCH));
	line_flat[BPP_SHADOW] = static_cast<patch_t*>(W_CachePatchName("R_RRMLS3", PU_PATCH));

	// Progress markers
	patch_t *level_dot[BPP_MAIN];
	patch_t *bonus_dot[BPP_MAIN];
	patch_t *capsu_dot[BPP_MAIN];
	patch_t *prize_dot[BPP_MAIN];

	level_dot[BPP_AHEAD] = static_cast<patch_t*>(W_CachePatchName("R_RRMRK2", PU_PATCH));
	level_dot[BPP_DONE] = static_cast<patch_t*>(W_CachePatchName("R_RRMRK1", PU_PATCH));

	bonus_dot[BPP_AHEAD] = static_cast<patch_t*>(W_CachePatchName("R_RRMRK7", PU_PATCH));
	bonus_dot[BPP_DONE] = static_cast<patch_t*>(W_CachePatchName("R_RRMRK8", PU_PATCH));

	capsu_dot[BPP_AHEAD] = static_cast<patch_t*>(W_CachePatchName("R_RRMRK3", PU_PATCH));
	capsu_dot[BPP_DONE] = static_cast<patch_t*>(W_CachePatchName("R_RRMRK5", PU_PATCH));

	prize_dot[BPP_AHEAD] = static_cast<patch_t*>(W_CachePatchName("R_RRMRK4", PU_PATCH));
	prize_dot[BPP_DONE] = static_cast<patch_t*>(W_CachePatchName("R_RRMRK6", PU_PATCH));

	patch_t *rpmark[2];
	rpmark[0] = static_cast<patch_t*>(W_CachePatchName("R_RPMARK", PU_PATCH));
	rpmark[1] = static_cast<patch_t*>(W_CachePatchName("R_R2MARK", PU_PATCH));

	UINT8 *colormap = NULL, *oppositemap = NULL;
	fixed_t playerx = 0, playery = 0;
	UINT16 pskin = MAXSKINS;
	UINT16 pcolor = SKINCOLOR_WHITE;

	if (standings->mainplayer == MAXPLAYERS)
	{
		;
	}
	else if (playeringame[standings->mainplayer] == false)
	{
		standings->mainplayer = MAXPLAYERS;
	}
	else if (players[standings->mainplayer].spectator == false
		&& players[standings->mainplayer].skin < numskins
		&& players[standings->mainplayer].skincolor != SKINCOLOR_NONE
		&& players[standings->mainplayer].skincolor < numskincolors
	)
	{
		pskin = players[standings->mainplayer].skin;
		pcolor = players[standings->mainplayer].skincolor;
	}

	colormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(pcolor), GTC_CACHE);
	oppositemap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(skincolors[pcolor].invcolor), GTC_CACHE);

	UINT8 workingqueuesize = roundqueue.size;
	boolean upwa = false;

	if (roundqueue.size > 1
		&& roundqueue.entries[roundqueue.size - 1].rankrestricted == true
	)
	{
		if (roundqueue.size & 1)
		{
			upwa = true;
		}

		if (!adminmode)
		{
			workingqueuesize--;
		}
	}

	INT32 widthofroundqueue, totalsteps;

	INT32 menusendoffset = 0;
	if (menuqueue.sending)
	{
		if (menuqueue.sending > menuqueue.size)
		{
			menusendoffset = menuqueue.size;
		}
		else
		{
			menusendoffset = menuqueue.sending-1;
		}
	}

	if (adminmode)
	{
		totalsteps = std::min(workingqueuesize + (menuqueue.size - menusendoffset), ROUNDQUEUE_MAX);
	}
	else
	{
		totalsteps = workingqueuesize;
	}

	widthofroundqueue = (totalsteps - 1) * 24;

	INT32 x = (BASEVIDWIDTH - widthofroundqueue) / 2;
	INT32 y, basey = 167 + offset;

	INT32 spacetospecial = 0;

	// The following block handles horizontal easing of the
	// progression bar on the last non-rankrestricted round.
	if (!adminmode && standings->showrank == true)
	{
		fixed_t percentslide = 0;
		SINT8 deferxoffs = 0;

		const INT32 desiredx2 = (290 + bufferspace);
		spacetospecial = std::max<INT32>(desiredx2 - widthofroundqueue - (24 - bufferspace), 16);

		if (roundqueue.position == roundqueue.size)
		{
			percentslide = FRACUNIT;
		}
		else if (doanimations
			&& roundqueue.position == roundqueue.size-1
			&& timer - interpoffs <= 3*TICRATE)
		{
			const INT32 through = (3*TICRATE) - (timer - interpoffs - 1);
			const INT32 slidetime = (TICRATE/2);

			if (through >= slidetime)
			{
				percentslide = FRACUNIT;
			}
			else
			{
					percentslide = R_InterpolateFixed(
						(through - 1) * FRACUNIT,
						(through * FRACUNIT)
					) / slidetime;
			}
		}

		if (percentslide != 0)
		{
			const INT32 differencetocover = (x + widthofroundqueue + spacetospecial - desiredx2);

			if (percentslide == FRACUNIT)
			{
				x -= (differencetocover + deferxoffs);
			}
			else
			{
				x -= Easing_OutCubic(
					percentslide,
					0,
					differencetocover * FRACUNIT
				) / FRACUNIT;
			}
		}
	}

	// Fill in background to left edge of screen
	fixed_t xiter = x;

	if (upwa == true)
	{
		xiter -= 24;
		V_DrawMappedPatch(xiter, basey, baseflags, queuebg_upwa, greymap);
	}

	// Draw to left side of screen
	while (xiter > -bufferspace)
	{
		xiter -= 24;
		V_DrawMappedPatch(xiter, basey, baseflags, queuebg_flat, greymap);
	}

	// Draw to right side of screen
	xiter = x + widthofroundqueue;
	while (xiter < BASEVIDWIDTH + bufferspace)
	{
		xiter += 24;
		V_DrawMappedPatch(xiter, basey, baseflags, queuebg_flat, greymap);
	}

	// Actually queued maps
	for (i = 0; i < workingqueuesize; i++)
	{
		// Draw the background, and grab the appropriate line, to the right of the dot
		patch_t **choose_line = NULL;

		upwa ^= true;
		if (upwa == false)
		{
			y = basey + 4;

			V_DrawMappedPatch(x, basey, baseflags, queuebg_down, greymap);

			if (i+1 != workingqueuesize) // no more line?
			{
				choose_line = line_down;
			}
		}
		else
		{
			y = basey + 12;

			if (i+1 != workingqueuesize) // no more line?
			{
				V_DrawMappedPatch(x, basey, baseflags, queuebg_upwa, greymap);

				choose_line = line_upwa;
			}
			else
			{
				V_DrawMappedPatch(
					x,
					basey,
					baseflags,
					((workingqueuesize == totalsteps) ? queuebg_flat : queuebg_upwa),
					greymap
				);
			}
		}

		if (roundqueue.position == i+1)
		{
			playerx = (x * FRACUNIT);
			playery = (y * FRACUNIT);

			// If there's standard progression ahead of us, visibly move along it.
			if (
				doanimations
				&& choose_line != NULL
				&& timer - interpoffs <= 2*TICRATE
			)
			{
				// 8 tics is chosen because it plays nice
				// with both the x and y distance to cover.
				fixed_t through = (2*TICRATE) - (timer - interpoffs - 1);;

				if (through > 8)
				{
					if (through == 9 + interpoffs)
					{
						// Impactful landing
						playery += FRACUNIT;
					}

					through = 8 * FRACUNIT;
				}
				else
				{
					through = R_InterpolateFixed(
						(through - 1) * FRACUNIT,
						(through * FRACUNIT)
					);
				}

				// 24 pixels when all is said and done
				if (!nextmapoverride)
					playerx += through * 3;

				if (upwa == false)
				{
					playery += through;
				}
				else
				{
					playery -= through;
				}

				if (through > 0 && through < 8 * FRACUNIT)
				{
					// Hoparabola and a skip.
					const fixed_t jumpfactor = through - (4 * FRACUNIT);
					// jumpfactor squared goes through 36 -> 0 -> 36.
					// 12 pixels is an arbitrary jump height, but we match it to invert the parabola.
					playery -= ((12 * FRACUNIT)
							- (FixedMul(jumpfactor, jumpfactor) / 3)
					);
				}
			}
			// End of the moving along
		}

		if (choose_line != NULL)
		{
			// Draw the line to the right of the dot

			V_DrawMappedPatch(
				x - 1, basey + 11,
				baseflags,
				choose_line[BPP_SHADOW],
				NULL
			);

			boolean lineisfull = false, recttoclear = false;

			if (roundqueue.position > i+1)
			{
				lineisfull = true;
			}
			else if (
				doanimations == true
				&& roundqueue.position == i+1
				&& timer - interpoffs <= 2*TICRATE
			)
			{
				// 8 tics is chosen because it plays nice
				// with both the x and y distance to cover.
				const INT32 through = (2*TICRATE) - (timer - interpoffs - 1);

				if (through == 0)
				{
					; // no change...
				}
				else if (through > 8)
				{
					lineisfull = true;
				}
				else
				{
					V_DrawMappedPatch(
						x - 1, basey + 12,
						baseflags,
						choose_line[BPP_DONE],
						colormap
					);

					V_SetClipRect(
						playerx + FRACUNIT,
						0,
						(BASEVIDWIDTH + bufferspace) << FRACBITS,
						BASEVIDHEIGHT << FRACBITS,
						baseflags
					);

					recttoclear = true;
				}
			}

			V_DrawMappedPatch(
				x - 1, basey + 12,
				baseflags,
				choose_line[lineisfull ? BPP_DONE : BPP_AHEAD],
				lineisfull ? colormap : NULL
			);

			if (recttoclear == true)
			{
				V_ClearClipRect();
			}
		}
		else
		{
			// Handle special entry on the end
			// (has to be drawn before the semifinal dot due to overlap)
			if (!adminmode && standings->showrank == true)
			{
				const fixed_t x2 = x + spacetospecial;

				if (roundqueue.position == roundqueue.size)
				{
					playerx = (x2 * FRACUNIT);
					playery = (y * FRACUNIT);
				}
				else if (
					doanimations == true
					&& roundqueue.position == workingqueuesize
					&& timer - interpoffs <= 2*TICRATE
				)
				{
					const INT32 through = ((2*TICRATE) - (timer - interpoffs - 1));
					fixed_t linefill;

					if (through > standings->linemeter)
					{
						linefill = standings->linemeter * FRACUNIT;

						// Small judder if there's enough time for it
						if (timer <= 2)
						{
							;
						}
						else if (through == (standings->linemeter + 1 + interpoffs))
						{
							playerx += FRACUNIT;
						}
						else if (through == (standings->linemeter + 2 + interpoffs))
						{
							playerx -= FRACUNIT;
						}
					}
					else
					{
						linefill = R_InterpolateFixed(
							(through - 1) * FRACUNIT,
							(through * FRACUNIT)
						);
					}

					const fixed_t percent = FixedDiv(
							linefill,
							(2*TICRATE) * FRACUNIT
						);

					playerx +=
						FixedMul(
							(x2 - x) * FRACUNIT,
							percent
						);
				}

				// Special background bump
				V_DrawMappedPatch(x2 - 13, basey, baseflags, queuebg_prize, greymap);

				// Draw the final line
				const fixed_t barstart = x + 6;
				const fixed_t barend = x2 - 6;

				if (barend - 2 >= barstart)
				{
					boolean lineisfull = false, recttoclear = false;

					xiter = barstart;

					if (playerx >= (barend + 1) * FRACUNIT)
					{
						lineisfull = true;
					}
					else if (playerx <= (barstart - 1) * FRACUNIT)
					{
						;
					}
					else
					{
						const fixed_t fillend = std::min((playerx / FRACUNIT) + 2, barend);

						while (xiter < fillend)
						{
							V_DrawMappedPatch(
								xiter - 1, basey + 10,
								baseflags,
								line_flat[BPP_SHADOW],
								NULL
							);

							V_DrawMappedPatch(
								xiter - 1, basey + 12,
								baseflags,
								line_flat[BPP_DONE],
								colormap
							);

							xiter += 2;
						}

						// Undo the last step so we can draw the unfilled area of the patch.
						xiter -= 2;

						V_SetClipRect(
							playerx,
							0,
							(BASEVIDWIDTH + bufferspace) << FRACBITS,
							BASEVIDHEIGHT << FRACBITS,
							baseflags
						);

						recttoclear = true;
					}

					while (xiter < barend)
					{
						V_DrawMappedPatch(
							xiter - 1, basey + 10,
							baseflags,
							line_flat[BPP_SHADOW],
							NULL
						);

						V_DrawMappedPatch(
							xiter - 1, basey + 12,
							baseflags,
							line_flat[lineisfull ? BPP_DONE : BPP_AHEAD],
							lineisfull ? colormap : NULL
						);

						xiter += 2;
					}

					if (recttoclear == true)
					{
						V_ClearClipRect();
					}
				}

				// Draw the final dot
				V_DrawMappedPatch(
					x2 - 8, y,
					baseflags,
					prize_dot[roundqueue.position == roundqueue.size ? BPP_DONE : BPP_AHEAD],
					roundqueue.position == roundqueue.size ? oppositemap : colormap
				);
			}
			// End of the special entry handling
		}

		// Now draw the dot
		patch_t **chose_dot = NULL;

		if (roundqueue.entries[i].rankrestricted == true)
		{
			// This shouldn't show up in regular play, but don't hide it entirely.
			chose_dot = prize_dot;
		}
		else if (
			roundqueue.entries[i].overridden == true
			|| (grandprixinfo.gp == true
				&& roundqueue.entries[i].gametype != GT_RACE) // roundqueue.entries[0].gametype
		)
		{
			if ((gametypes[roundqueue.entries[i].gametype]->rules & GTR_PRISONS) == GTR_PRISONS)
			{
				chose_dot = capsu_dot;
			}
			else
			{
				chose_dot = bonus_dot;
			}
		}
		else
		{
			chose_dot = level_dot;
		}

		if (chose_dot)
		{
			V_DrawMappedPatch(
				x - 8, y,
				baseflags,
				chose_dot[roundqueue.position >= i+1 ? BPP_DONE : BPP_AHEAD],
				roundqueue.position == i+1 ? oppositemap : colormap
			);
		}

		x += 24;
	}

	totalsteps -= i;

	// Maps in the progress of being queued on the menu
	if (adminmode && totalsteps)
	{
		for (i = menusendoffset; i < (totalsteps + menusendoffset); i++)
		{
			upwa ^= true;
			if (upwa == false)
			{
				y = basey + 4;

				V_DrawMappedPatch(x, basey, baseflags, queuebg_down, greymap);
			}
			else
			{
				y = basey + 12;

				if (i+1 != menuqueue.size) // no more line?
				{
					V_DrawMappedPatch(x, basey, baseflags, queuebg_upwa, greymap);
				}
				else
				{
					V_DrawMappedPatch(x, basey, baseflags, queuebg_flat, greymap);
				}
			}

			V_DrawMappedPatch(
				x - 8, y,
				baseflags,
				level_dot[BPP_AHEAD],
				NULL
			);

			V_DrawMappedPatch(
				x - 10, y - 14,
				baseflags,
				rpmark[0],
				NULL
			);

			K_DrawMapAsFace(
				x - 9, y - 13,
				(baseflags|((menuqueue.entries[i].encore) ? V_FLIP : 0)),
				menuqueue.entries[i].mapnum,
				NULL, FRACUNIT, 1
			);

			x += 24;
		}
	}

	// Draw the player position through the round queue!
	if (playery != 0)
	{
		// Change alignment
		playerx -= (10 * FRACUNIT);
		playery -= (14 * FRACUNIT);

		if (pskin < numskins)
		{
			// Draw outline for rank icon
			V_DrawFixedPatch(
				playerx, playery,
				FRACUNIT,
				baseflags,
				rpmark[0],
				NULL
			);

			// Draw the player's rank icon
			V_DrawFixedPatch(
				playerx + FRACUNIT, playery + FRACUNIT,
				FRACUNIT,
				baseflags,
				faceprefix[pskin][FACE_RANK],
				R_GetTranslationColormap(pskin, static_cast<skincolornum_t>(pcolor), GTC_CACHE)
			);
		}
		else
		{
			// Draw mini arrow
			V_DrawFixedPatch(
				playerx, playery,
				FRACUNIT,
				baseflags,
				rpmark[1],
				NULL
			);
		}
	}
}

#define INTERBUTTONSLIDEIN (TICRATE/2)

//
// Y_DrawIntermissionButton
//
// It's a button that slides at the given time
//
void Y_DrawIntermissionButton(INT32 startslide, INT32 through, boolean widescreen)
{
	INT32 percentslide = 0;
	const INT32 slidetime = (TICRATE/4);
	boolean pressed = false;

	if (startslide >= 0)
	{
		through = startslide;
	}
	else
	{
		through -= ((TICRATE/2) + 1);
		pressed = (!menuactive && M_MenuConfirmHeld(0));
	}

	if (through >= 0)
	{
		if (through >= slidetime)
		{
			percentslide = FRACUNIT;
		}
		else
		{
			percentslide = R_InterpolateFixed(
				(through - 1) * FRACUNIT,
				(through * FRACUNIT)
			) / slidetime;
		}
	}

	if (percentslide < FRACUNIT)
	{
		INT32 offset = 0;

		if (percentslide)
		{
			offset = Easing_InCubic(
				percentslide,
				0,
				16 * FRACUNIT
			);
		}

		using srb2::Draw;

		Draw::TextElement text = Draw::TextElement().parse(pressed ? "<a_pressed>" : "<a>");
		Draw draw = Draw(FixedToFloat(2*FRACUNIT - offset), FixedToFloat((BASEVIDHEIGHT - 16)*FRACUNIT)).flags(widescreen ? (V_SNAPTOLEFT|V_SNAPTOBOTTOM) : 0);
		draw.text(text.string());

		/*
		K_drawButton(
			2*FRACUNIT - offset,
			(BASEVIDHEIGHT - 16)*FRACUNIT,
			(widescreen
				? (V_SNAPTOLEFT|V_SNAPTOBOTTOM)
				: 0
			),
			kp_button_a[1],
			pressed
		);
		*/
	}
}

//
// Y_DrawRankMode
//
// Draws EXP or MOBIUMS label depending on context.
// x and y designate the coordinates of the most bottom-right pixel to draw from (because it is the left extent and patch heights that vary),
// or the bottom-center if center is true.
//
void Y_DrawRankMode(INT32 x, INT32 y, boolean center)
{
	boolean	useMobiums = (powertype != PWRLV_DISABLED);
	INT32	textWidth, middleLeftEdge, middleRightEdge, middleWidth;

	char	text[8];
	char	iconPatchName[8];
	UINT8	iconWidth; // the graphic paddings are inconsistent...
	UINT8	*iconColormap;
	UINT8	*stickerColormap;

	patch_t	*iconPatch;
	patch_t	*stickerTail = static_cast<patch_t*>(W_CachePatchName("INT_STK1", PU_CACHE));
	patch_t	*stickerMiddle = static_cast<patch_t*>(W_CachePatchName("INT_STK2", PU_CACHE));
	patch_t	*stickerHead = center ? stickerTail : static_cast<patch_t*>(W_CachePatchName("INT_STK3", PU_CACHE));
	UINT32	stickerHeadFlags = 0;
	UINT8	stickerHeadOffset = 0;

	if (useMobiums)
	{
		snprintf(text, sizeof text, "MOBIUMS");
		snprintf(iconPatchName, sizeof iconPatchName, "K_STMOB");
		iconWidth = 22;
		iconColormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(SKINCOLOR_NONE), GTC_CACHE);
		stickerColormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(SKINCOLOR_TEA), GTC_CACHE);
	}
	else
	{
		snprintf(text, sizeof text, "EXP");
		snprintf(iconPatchName, sizeof iconPatchName, "K_STEXP");
		iconWidth = 16;
		iconColormap = R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(SKINCOLOR_MUSTARD), GTC_CACHE);
		stickerColormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(SKINCOLOR_MUSTARD), GTC_CACHE);
	}

	iconPatch = static_cast<patch_t*>(W_CachePatchName(iconPatchName, PU_CACHE));
	textWidth = (INT32)V_ThinStringWidth(text, 0);
	middleLeftEdge = x - iconWidth - textWidth - 8;
	middleRightEdge = x - stickerHead->width;
	middleWidth = middleRightEdge - middleLeftEdge;

	if (center)
	{
		// flip the right-hand sticker tail and keep it left-aligned
		stickerHeadFlags |= V_FLIP;
		stickerHeadOffset += stickerHead->width;

		// sliiightly extend the right side of the sticker
		middleWidth += 2;
		middleRightEdge += 2;

		// shift all components to the right so that our x coordinates are center-aligned
		#define CENTER_SHIFT (stickerHead->width + middleWidth / 2)
		x += CENTER_SHIFT;
		middleLeftEdge += CENTER_SHIFT;
		middleRightEdge += CENTER_SHIFT;
		#undef CENTER_SHIFT
	}

	// draw sticker
	V_DrawMappedPatch(middleRightEdge + stickerHeadOffset, y - stickerHead->height, stickerHeadFlags, stickerHead, stickerColormap);
	V_DrawStretchyFixedPatch(
		middleLeftEdge << FRACBITS,
		(y - stickerMiddle->height) << FRACBITS,
		(middleWidth << FRACBITS) / stickerMiddle->width + 1,
		FRACUNIT,
		0, stickerMiddle, stickerColormap
	);
	V_DrawMappedPatch(middleLeftEdge - stickerTail->width, y - stickerTail->height, 0, stickerTail, stickerColormap);

	// draw icon and text
	V_DrawMappedPatch(x - iconPatch->width - 6, y - iconPatch->height + 4, 0, iconPatch, iconColormap);
	V_DrawThinString(middleLeftEdge - 1, y - 9, 0, text);
}

void Y_DrawIntermissionHeader(fixed_t x, fixed_t y, boolean gotthrough, const char *headerstring, boolean showroundnum, boolean small)
{
	const INT32 v_width = (small ? BASEVIDWIDTH/2 : BASEVIDWIDTH);
	const fixed_t frac = (small ? FRACUNIT/2 : FRACUNIT);
	const INT32 small_flag = (small ? V_SPLITSCREEN : 0);

	if (small && r_splitscreen > 1)
	{
		V_SetClipRect(
			0,
			0,
			v_width << FRACBITS,
			BASEVIDHEIGHT << FRACBITS,
			V_SPLITSCREEN
		);
	}

	// Header bar
	patch_t *rtpbr = static_cast<patch_t*>(W_CachePatchName((small ? "R_RTPB4" : "R_RTPBR"), PU_PATCH));
	V_DrawFixedPatch((20 * frac) + x, (24 * frac) + y, FRACUNIT, small_flag, rtpbr, NULL);

	fixed_t headerx, headery, headerwidth = 0;

	if (gotthrough)
	{
		headerx = (51 * frac);
		headery = (7 * frac);
	}
	else
	{
		headerwidth = V_TitleCardStringWidth(headerstring, small);

		headerx = (v_width - headerwidth) * (FRACUNIT / 2);
		headery = 17 * frac;
	}

	// Draw round numbers
	if (showroundnum == true)
	{
		patch_t *roundpatch = ST_getRoundPicture(small);

		if (roundpatch)
		{
			fixed_t roundx = (v_width * 3 * FRACUNIT) / 4;

			if (headerwidth != 0)
			{
				const fixed_t roundoffset = (8 * frac) + (roundpatch->width * FRACUNIT);

				roundx = headerx + roundoffset;
				headerx -= roundoffset/2;
			}

			V_DrawFixedPatch(x + roundx, (39 * frac) + y, FRACUNIT, small_flag, roundpatch, NULL);
		}
	}

	V_DrawTitleCardStringFixed(x + headerx, y + headery, FRACUNIT, headerstring, small_flag, false, 0, 0, small);

	if (gotthrough)
	{
		// GOT THROUGH ROUND
		patch_t *gthro = static_cast<patch_t*>(W_CachePatchName((small ? "R_GTHR4" : "R_GTHRO"), PU_PATCH));
		V_DrawFixedPatch((50 * frac) + x, (42 * frac) + y, FRACUNIT, small_flag, gthro, NULL);
	}

	V_ClearClipRect();
}

static void Y_DrawMapTitleString(fixed_t x, const char *name)
{
	V_DrawStringScaled(
		x - ttlscroll,
		(BASEVIDHEIGHT - 73) * FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		V_SUBTRACT | V_60TRANS,
		NULL,
		LSHI_FONT,
		name
	);
}

static fixed_t Y_DrawMapTitle(void)
{
	const char *name = bossinfo.valid && bossinfo.enemyname ?
		bossinfo.enemyname : mapheaderinfo[prevmap]->menuttl;
	char *buf = NULL;

	if (!name[0])
	{
		buf = G_BuildMapTitle(prevmap + 1);
		name = buf;
	}

	fixed_t w = V_StringScaledWidth(
		FRACUNIT,
		FRACUNIT,
		FRACUNIT,
		0,
		LSHI_FONT,
		name
	) + (16 * FRACUNIT);

	fixed_t x = BASEVIDWIDTH * FRACUNIT;

	while (x > -w)
	{
		Y_DrawMapTitleString(x, name);
		x -= w;
	}

	Z_Free(buf);

	return w;
}

//
// Y_IntermissionDrawer
//
// Called by D_Display. Nothing is modified here; all it does is draw. (SRB2Kart: er, about that...)
// Neat concept, huh?
//
void Y_IntermissionDrawer(void)
{
	// INFO SEGMENT
	// Numbers are V_DrawRightAlignedThinString as flags
	// resbar 1 (48,82)  5 (176, 82)
	// 2 (48, 96)

	//player icon 1 (55,79) 2 (55,93) 5 (183,79)

	if (intertype == int_none || rendermode == render_none)
		return;

	fixed_t x;

	// Checker scroll
	patch_t *rbgchk = static_cast<patch_t*>(W_CachePatchName("R_RBGCHK", PU_PATCH));

	// Scrolling marquee
	patch_t *rrmq = static_cast<patch_t*>(W_CachePatchName("R_RRMQ", PU_PATCH));

	fixed_t mqloop = SHORT(rrmq->width)*FRACUNIT;
	fixed_t chkloop = SHORT(rbgchk->width)*FRACUNIT;

	UINT8 *bgcolor = R_GetTranslationColormap(TC_INTERMISSION, static_cast<skincolornum_t>(0), GTC_CACHE);

	// Draw the background
	K_DrawMapThumbnail(0, 0, BASEVIDWIDTH<<FRACBITS, (data.encore ? V_FLIP : 0), prevmap, bgcolor);

	for (x = -mqscroll; x < (BASEVIDWIDTH * FRACUNIT); x += mqloop)
	{
		V_DrawFixedPatch(x, 154<<FRACBITS, FRACUNIT, V_SUBTRACT, rrmq, NULL);
	}

	V_DrawFixedPatch(chkscroll, 0, FRACUNIT, V_SUBTRACT, rbgchk, NULL);
	V_DrawFixedPatch(chkscroll - chkloop, 0, FRACUNIT, V_SUBTRACT, rbgchk, NULL);

	fixed_t ttlloop = Y_DrawMapTitle();

	// Animate scrolling elements if relevant
	if (!paused && !P_AutoPause())
	{
		mqscroll += renderdeltatics;
		if (mqscroll > mqloop)
			mqscroll %= mqloop;

		chkscroll += renderdeltatics;
		if (chkscroll > chkloop)
			chkscroll %= chkloop;

		ttlscroll += renderdeltatics * 2;
		if (ttlscroll > ttlloop)
			ttlscroll %= ttlloop;
	}

	if (renderisnewtic)
	{
		LUA_HUD_ClearDrawList(luahuddrawlist_intermission);
		LUA_HookHUD(luahuddrawlist_intermission, HUD_HOOK(intermission));
	}
	LUA_HUD_DrawList(luahuddrawlist_intermission);

	if (!LUA_HudEnabled(hud_intermissiontally))
		goto skiptallydrawer;

	x = 0;
	if (sorttic != -1 && intertic > sorttic)
	{
		const INT32 count = (intertic - sorttic);

		if (count < 8)
			x = -((count * BASEVIDWIDTH) / 8);
		else if (count == 8)
			goto skiptallydrawer;
		else if (count < 16)
			x = (((16 - count) * BASEVIDWIDTH) / 8);
	}

	// Draw the header bar
	Y_DrawIntermissionHeader(x << FRACBITS, 0, data.gotthrough, data.headerstring, data.showroundnum, false);

	// Returns early if there's no players to draw
	Y_PlayerStandingsDrawer(&data, x);

	if (sorttic == -1 || ((intertic - sorttic) < 8))
		K_drawKartTeamScores(true, x);

	// Draw bottom (and top) pieces
skiptallydrawer:
	if (!LUA_HudEnabled(hud_intermissionmessages))
		goto finalcounter;

	// Returns early if there's no roundqueue entries to draw
	Y_RoundQueueDrawer(&data, 0, true, false, false);

	if (netgame)
	{
		if (speedscramble != -1 && speedscramble != gamespeed)
		{
			V_DrawCenteredThinString(BASEVIDWIDTH/2, 154, highlightflags,
				va(M_GetText("Next race will be %s Speed!"), kartspeed_cons_t[1+speedscramble].strvalue));
		}
	}

finalcounter:
	if ((modeattacking == ATTACKING_NONE) && demo.recording)
		ST_DrawSaveReplayHint(0);

	if (Y_CanSkipIntermission())
	{
		const tic_t end = roundqueue.size != 0 ? 3*TICRATE : TICRATE;
		Y_DrawIntermissionButton(INTERBUTTONSLIDEIN - intertic, end - timer, false);
	}
	else
	{
		const INT32 tickDown = (timer + 1)/TICRATE;

		// See also k_vote.c
		V__DrawOneScaleString(
			2*FRACUNIT,
			(BASEVIDHEIGHT - (2+8))*FRACUNIT,
			FRACUNIT,
			0, NULL,
			OPPRF_FONT,
			va("%d", tickDown)
		);
	}

	M_DrawMenuForeground();
}

//
// Y_Ticker
//
// Manages fake score tally for single player end of act, and decides when intermission is over.
//
void Y_Ticker(void)
{
	if (intertype == int_none)
		return;

	if (demo.recording)
		G_CheckDemoTitleEntry();

	// Check for pause or menu up in single player
	if (paused || P_AutoPause())
		return;

	LUA_HOOK(IntermissionThinker);

	if (Y_CanSkipIntermission())
	{
		if (intertic < INTERBUTTONSLIDEIN)
		{
			intertic++;
			return;
		}

		boolean preventintertic = (intertic == INTERBUTTONSLIDEIN);

		if (!menuactive && M_MenuConfirmPressed(0))
		{
			// If there is a roundqueue, make time for it.
			// Else, end instantly on button press.
			// Actually, give it a slight delay, so the "kaching" sound isn't cut off.
			const tic_t end = roundqueue.size != 0 ? 3*TICRATE : TICRATE;

			if (intertic == INTERBUTTONSLIDEIN) // card flip hasn't started
			{
				if (sorttic != -1)
				{
					intertic = sorttic;
				}
				else
				{
					timer = end;
				}

				preventintertic = false;
			}
			else if (timer >= INFINITE_TIMER && intertic >= sorttic + 16) // card done flipping
			{
				const INT32 kaching = sorttic + 16 + (2*TICRATE);

				if (intertic < kaching)
				{
					intertic = kaching; // kaching immediately
				}

				timer = end;
			}
		}

		if (preventintertic)
		{
			return;
		}
	}

	intertic++;

	// Team scramble code for team match and CTF.
	// Don't do this if we're going to automatically scramble teams next round.
	/*
	if (G_GametypeHasTeams() && cv_teamscramble.value && !cv_scrambleonchange.value && server)
	{
		// If we run out of time in intermission, the beauty is that
		// the P_Ticker() team scramble code will pick it up.
		if ((intertic % (TICRATE/7)) == 0)
			P_DoTeamscrambling();
	}
	*/

	if ((timer < INFINITE_TIMER && --timer <= 0)
		|| (intertic == endtic))
	{
		Y_EndIntermission();
		G_AfterIntermission();
		return;
	}

	// Animation sounds for roundqueue, see Y_RoundQueueDrawer
	if (roundqueue.size > 1
		&& roundqueue.position != 0
		&& (timer - 1) <= 2*TICRATE)
	{
		const INT32 through = ((2*TICRATE) - (timer - 1));

		UINT8 workingqueuesize = roundqueue.size - 1;

		if (data.showrank == true
			&& roundqueue.position == workingqueuesize)
		{
			// Handle special entry on the end
			if (through == data.linemeter && timer > 2)
			{
				S_StopSoundByID(NULL, sfx_gpmetr);
				S_StartSound(NULL, sfx_kc50);
			}
			else if (through == 0)
			{
				S_StartSound(NULL, sfx_gpmetr);
			}
		}
		else
		{
			if (data.showrank == false
				&& roundqueue.entries[workingqueuesize].rankrestricted == true)
			{
				workingqueuesize--;
			}

			if (through == 9
				&& roundqueue.position <= workingqueuesize)
			{
				// Impactful landing
				S_StartSound(NULL, sfx_kc50);
			}
		}
	}

	if (intertic < TICRATE || endtic != -1)
	{
		return;
	}

	if (data.rankingsmode && intertic & 1)
	{
		memset(data.jitter, 0, sizeof (data.jitter));
		return;
	}

	if (intertype == int_time || intertype == int_score)
	{
		{
			if (!data.rankingsmode && sorttic != -1 && (intertic >= sorttic + 8))
			{
				Y_MidIntermission();
				Y_CalculateMatchData(1, Y_CompareRank);
			}

			if (data.rankingsmode && intertic > sorttic+16+(2*TICRATE))
			{
				INT32 q=0,r=0;
				boolean kaching = true;

				for (q = 0; q < data.numplayers; q++)
				{
					if (data.num[q] == MAXPLAYERS
						|| !data.increase[data.num[q]]
						|| data.increase[data.num[q]] == INT16_MIN)
					{
						continue;
					}

					r++;
					data.jitter[data.num[q]] = 1;

					// Player can skip the tally, kaching!
					if (Y_CanSkipIntermission() && timer < INFINITE_TIMER)
					{
						data.increase[data.num[q]] = 0;
					}

					if (powertype != PWRLV_DISABLED)
					{
						// Power Levels
						if (abs(data.increase[data.num[q]]) < 10)
						{
							// Not a lot of point increase left, just set to 0 instantly
							data.increase[data.num[q]] = 0;
						}
						else
						{
							SINT8 remove = 0; // default (should not happen)

							if (data.increase[data.num[q]] < 0)
								remove = -10;
							else if (data.increase[data.num[q]] > 0)
								remove = 10;

							// Remove 10 points at a time
							data.increase[data.num[q]] -= remove;

							// Still not zero, no kaching yet
							if (data.increase[data.num[q]] != 0)
								kaching = false;
						}
					}
					else
					{
						// Basic bitch points
						if (data.increase[data.num[q]])
						{
							data.increase[data.num[q]] = std::max(data.increase[data.num[q]] - 3, 0);

							if (data.increase[data.num[q]] != 0)
								kaching = false;
						}
					}
				}

				if (r)
				{
					S_StartSound(NULL, (kaching ? sfx_chchng : sfx_ptally));
					Y_CalculateMatchData(2, Y_CompareRank);
				}
				/*else -- This is how to define an endtic, but we currently use timer for both SP and MP.
					endtic = intertic + 3*TICRATE;*/
			}
		}
	}
}

boolean Y_ShouldDoIntermission(void)
{
	// no intermission for GP events
	if ((grandprixinfo.gp == true && grandprixinfo.eventmode != GPEVENT_NONE)
	// or for failing in time attack mode
	|| (modeattacking && (players[consoleplayer].pflags & PF_NOCONTEST))
	// or for explicit requested skip (outside of modeattacking)
	|| (modeattacking == ATTACKING_NONE && skipstats != 0)
	// or tutorial skip material
	|| (nextmapoverride == NEXTMAP_TUTORIALCHALLENGE+1 || tutorialchallenge != TUTORIALSKIP_NONE)
	// or title screen attract demos
	|| (demo.playback && demo.attract == DEMO_ATTRACT_TITLE))
	{
		return false;
	}
	return true;
}

//
// Y_GetIntermissionType
//
// Returns the intermission type from the current gametype.
//
intertype_t Y_GetIntermissionType(void)
{
	intertype_t ret = static_cast<intertype_t>(gametypes[gametype]->intermission);

	if (ret == int_scoreortimeattack)
	{
		UINT8 i = 0, nump = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
			{
				continue;
			}

			nump++;
		}

		ret = (nump < 2 ? int_time : int_score);
	}

	return ret;
}

//
// Y_DetermineIntermissionType
//
// Determines the intermission type from the current gametype.
//
void Y_DetermineIntermissionType(void)
{
	// no intermission for GP events
	if (!Y_ShouldDoIntermission())
	{
		intertype = int_none;
		return;
	}

	intertype = Y_GetIntermissionType();
}

static UINT8 Y_PlayersBestPossiblePosition(player_t *const player)
{
	UINT8 bestPossiblePosition = MAXPLAYERS + 1;
	UINT8 i = UINT8_MAX;

	if ((player->pflags & PF_NOCONTEST) == 0)
	{
		if (player->exiting)
		{
			// They are finished, so their position is set in stone.
			bestPossiblePosition = player->position;
		}
		else
		{
			// If they're NOT finished, then check what their points could be
			// if they finished in the first available position.
			bestPossiblePosition = 1;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				player_t *const other = &players[i];

				if (!playeringame[i] || other->spectator)
				{
					continue;
				}

				if (other == player)
				{
					continue;
				}

				if (other->exiting)
				{
					bestPossiblePosition = std::max<UINT8>(bestPossiblePosition, other->position + 1);
				}
			}
		}
	}

	return bestPossiblePosition;
}

static UINT32 Y_EstimatePodiumScore(player_t *const player, UINT8 numPlaying)
{
	UINT8 pos = Y_PlayersBestPossiblePosition(player);
	UINT32 ourScore = player->score;

	ourScore += K_CalculateGPRankPoints(player->exp, pos, numPlaying);

	return ourScore;
}

static boolean Y_GuaranteedGPFirstPlace(void)
{
	player_t *bestInParty = nullptr;
	UINT32 bestPartyScore = 0;

	UINT8 numPlaying = spectateGriefed;

	UINT8 i = UINT8_MAX;

	// Quick first loop to count players.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *const comparePlayer = &players[i];

		if (!playeringame[i] || comparePlayer->spectator)
		{
			continue;
		}

		numPlaying++;
	}

	// Iterate our party, estimate the best possible exiting score out of all of them.
	for (i = 0; i <= r_splitscreen; i++)
	{
		player_t *const comparePlayer = &players[displayplayers[i]];

		if (comparePlayer->spectator)
		{
			continue;
		}

		if (!comparePlayer->exiting)
		{
			continue;
		}

		UINT32 newScore = Y_EstimatePodiumScore(comparePlayer, numPlaying);
		if (bestInParty == nullptr || newScore > bestPartyScore)
		{
			bestInParty = comparePlayer;
			bestPartyScore = newScore;
		}
	}

	if (bestInParty == nullptr)
	{
		// No partied players are actually available,
		// so always use the regular intermission music.
		return false;
	}

	// Iterate through all players not belonging to our party.
	// Estimate the possible scores that they could get.
	// Play special music only if none of these scores beat ours!
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *comparePlayer = &players[i];

		if (!playeringame[i] || comparePlayer->spectator)
		{
			continue;
		}

		if (P_IsPartyPlayer(comparePlayer))
		{
			continue;
		}

		if (Y_EstimatePodiumScore(comparePlayer, numPlaying) >= bestPartyScore)
		{
			// NO, there is a chance that we will NOT finish first!
			// You may still be able to finish first, but it is NOT guaranteed.
			return false;
		}
	}

	// There is an overwhelmingly good chance
	// that we are finishing in first place.
	return true;
}

void Y_PlayIntermissionMusic(void)
{
	if (modeattacking != ATTACKING_NONE)
	{
		Music_Remap("intermission", "timent");
	}
	else if (grandprixinfo.gp == true
		&& grandprixinfo.cup != nullptr
		&& roundqueue.size > 0
		&& roundqueue.roundnum >= grandprixinfo.cup->numlevels)
	{
		if (Y_GuaranteedGPFirstPlace())
		{
			Music_Remap("intermission", "gprnds");
		}
		else
		{
			Music_Remap("intermission", "gprnd5");
		}
	}
	else
	{
		Music_Remap("intermission", "racent");
	}

	if (!Music_Playing("intermission"))
		Music_Play("intermission");
}

extern "C" boolean blockreset;

//
// Y_StartIntermission
//
// Called by G_DoCompleted. Sets up data for intermission drawer/ticker.
//
void Y_StartIntermission(void)
{
	UINT8 i = 0, nump = 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;
		nump++;
	}

	intertic = -1;

#ifdef PARANOIA
	if (endtic != -1)
		I_Error("endtic is dirty");
#endif

	blockreset = false;

	// set player Power Level type
	powertype = K_UsingPowerLevels();

	// determine the tic the intermission ends
	// Technically cv_inttime is saved to demos... but this permits having extremely long timers for post-netgame chatting without stranding you on the intermission in netreplays.
	if (!K_CanChangeRules(false))
	{
		timer = 10*TICRATE;
	}
	else
	{
		timer = cv_inttime.value*TICRATE;
	}

	// determine the tic everybody's scores/PWR starts getting sorted
	sorttic = -1;
	if (!timer)
		;
	else if (
		( // Match Race or Time Attack
			netgame == false
			&& grandprixinfo.gp == false
		)
		&& (
			modeattacking != ATTACKING_NONE // Definitely never another map
			|| ( // Any level sequence?
				roundqueue.size == 0 // No maps queued, points aren't relevant
				|| roundqueue.position == 0 // OR points from this round will be discarded
			)
		)
	)
	{
		// No PWR/global score, skip it
		// (the above is influenced by G_GetNextMap)
		timer /= 2;
	}
	else
	{
		// Minimum two seconds for match results, then two second slideover approx halfway through
		sorttic = std::max<INT32>((timer/2) - 2*TICRATE, 2*TICRATE);
	}

	// TODO: code's a mess, I'm just making it extra clear
	// that this piece of code is supposed to take priority
	// over the above. :)
	if (Y_CanSkipIntermission())
	{
		timer = INFINITE_TIMER; // doesn't count down

		if (sorttic != -1)
		{
			// Will start immediately, but must be triggered.
			// Needs to be TICRATE to bypass a condition in Y_Ticker.
			sorttic = TICRATE;
		}
	}

	// We couldn't display the intermission even if we wanted to.
	// But we still need to give the players their score bonuses, dummy.
	//if (dedicated) return;

	// This should always exist, but just in case...
	if (prevmap >= nummapheaders || !mapheaderinfo[prevmap])
		I_Error("Y_StartIntermission: Internal map ID %d not found (nummapheaders = %d)", prevmap, nummapheaders);

	switch (intertype)
	{
		case int_score:
		{
			// Calculate who won
			Y_CalculateMatchData(0, Y_CompareScore);
			break;
		}
		case int_time:
		{
			// Calculate who won
			Y_CalculateMatchData(0, Y_CompareTime);
			break;
		}

		case int_none:
		default:
			break;
	}

	if (powertype != PWRLV_DISABLED)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			// Kind of a hack to do this here,
			// but couldn't think of a better way.
			data.increase[i] = K_FinalPowerIncrement(
				&players[i],
				clientpowerlevels[i][powertype],
				clientPowerAdd[i]
			);
		}

		K_CashInPowerLevels();
	}

	SV_BumpMatchStats();

	if (!timer)
	{
		Y_EndIntermission();
		return;
	}

	if (staffsync)
	{
		Y_EndIntermission();
		return;
	}

	G_SetGamestate(GS_INTERMISSION);

	if (demo.playback)
	{
		// Replay menu is inacessible here.
		// Press A to continue!
		M_ClearMenus(true);
	}

	if (musiccountdown == 0)
	{
		Y_PlayIntermissionMusic();
	}

	S_ShowMusicCredit(); // Always call

	LUA_HUD_DestroyDrawList(luahuddrawlist_intermission);
	luahuddrawlist_intermission = LUA_HUD_CreateDrawList();

	if (roundqueue.size > 0 && roundqueue.position == roundqueue.size)
	{
		Automate_Run(AEV_QUEUEEND);
	}

	Automate_Run(AEV_INTERMISSIONSTART);
	bgpatch = static_cast<patch_t*>(W_CachePatchName("MENUBG", PU_STATIC));
	widebgpatch = static_cast<patch_t*>(W_CachePatchName("WEIRDRES", PU_STATIC));
}

// ======

//
// Y_MidIntermission
//
void Y_MidIntermission(void)
{
	// Replacing bots that fail out of play
	K_RetireBots();

	// If tournament play is not in action...
	if (roundqueue.position == 0)
	{
		// Unset player teams in anticipation of P_ShuffleTeams

		UINT8 i;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			players[i].team = TEAM_UNASSIGNED;
		}
	}
}

//
// Y_EndIntermission
//
void Y_EndIntermission(void)
{
	if (!data.rankingsmode)
	{
		Y_MidIntermission();
	}

	Y_UnloadData();

	endtic = -1;
	sorttic = -1;
	intertype = int_none;
}

#define UNLOAD(x) if (x) {Patch_Free(x);} x = NULL;
#define CLEANUP(x) x = NULL;

//
// Y_UnloadData
//
static void Y_UnloadData(void)
{
	// In hardware mode, don't Z_ChangeTag a pointer returned by W_CachePatchName().
	// It doesn't work and is unnecessary.
	if (rendermode == render_opengl)
		return;

	// unload the background patches
	UNLOAD(bgpatch);
	UNLOAD(widebgpatch);
	UNLOAD(bgtile);
	UNLOAD(interpic);
}
