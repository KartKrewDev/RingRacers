// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2004-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  y_inter.c
/// \brief Tally screens, or "Intermissions" as they were formally called in Doom

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
#include "p_local.h"

#include "m_cond.h" // condition sets
#include "lua_hook.h" // IntermissionThinker hook

#include "lua_hud.h"
#include "lua_hudlib_drawlist.h"

#include "m_random.h" // M_RandomKey
#include "g_input.h" // G_PlayerInputDown
#include "k_battle.h"
#include "k_boss.h"
#include "k_pwrlv.h"
#include "k_grandprix.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

typedef struct
{
	char patch[9];
	 INT32 points;
	UINT8 display;
} y_bonus_t;

typedef struct
{
	INT32 *character[MAXPLAYERS]; // Winner's character #
	UINT16 *color[MAXPLAYERS]; // Winner's color #
	SINT8 num[MAXPLAYERS]; // Winner's player #
	char *name[MAXPLAYERS]; // Winner's name

	UINT8 numplayers; // Number of players being displayed

	char levelstring[64]; // holds levelnames up to 64 characters

	// SRB2kart
	INT16 increase[MAXPLAYERS]; // how much did the score increase by?
	UINT8 jitter[MAXPLAYERS]; // wiggle

	UINT32 val[MAXPLAYERS]; // Gametype-specific value
	UINT8 pos[MAXPLAYERS]; // player positions. used for ties

	boolean rankingsmode; // rankings mode
	boolean encore; // encore mode
} y_data;

static y_data data;

// graphics
static patch_t *bgpatch = NULL;     // INTERSCR
static patch_t *widebgpatch = NULL;
static patch_t *bgtile = NULL;      // SPECTILE/SRB2BACK
static patch_t *interpic = NULL;    // custom picture defined in map header
static INT32 timer;

static INT32 timer;
static INT32 powertype = PWRLV_DISABLED;

static INT32 intertic;
static INT32 endtic = -1;
static INT32 sorttic = -1;

intertype_t intertype = int_none;
intertype_t intermissiontypes[NUMGAMETYPES];

static huddrawlist_h luahuddrawlist_intermission;

static void Y_FollowIntermission(void);
static void Y_UnloadData(void);

// SRB2Kart: voting stuff
// Level images
typedef struct
{
	char str[62];
	UINT8 gtc;
	const char *gts;
	boolean encore;
} y_votelvlinfo;

// Clientside & splitscreen player info.
typedef struct
{
	SINT8 selection;
	UINT8 delay;
} y_voteplayer;

typedef struct
{
	y_voteplayer playerinfo[4];
	UINT8 ranim;
	UINT8 rtics;
	UINT8 roffset;
	UINT8 rsynctime;
	UINT8 rendoff;
	boolean loaded;
} y_voteclient;

static y_votelvlinfo levelinfo[5];
static y_voteclient voteclient;
static INT32 votetic;
static INT32 voteendtic = -1;
static boolean votenotyetpicked;
static patch_t *cursor = NULL;
static patch_t *cursor1 = NULL;
static patch_t *cursor2 = NULL;
static patch_t *cursor3 = NULL;
static patch_t *cursor4 = NULL;
static patch_t *randomlvl = NULL;
static patch_t *rubyicon = NULL;

static void Y_UnloadVoteData(void);

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

	// Initialize variables
	if (rankingsmode > 1)
		;
	else if ((data.rankingsmode = (boolean)rankingsmode))
	{
		sprintf(data.levelstring, "* Total Rankings *");
		data.encore = false;
	}
	else
	{
		// set up the levelstring
		if (bossinfo.boss == true && bossinfo.enemyname)
		{
			snprintf(data.levelstring,
				sizeof data.levelstring,
				"* %s *",
				bossinfo.enemyname);
		}
		else if (mapheaderinfo[prevmap]->levelflags & LF_NOZONE)
		{
			if (mapheaderinfo[prevmap]->actnum > 0)
				snprintf(data.levelstring,
					sizeof data.levelstring,
					"* %s %d *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.levelstring,
					sizeof data.levelstring,
					"* %s *",
					mapheaderinfo[prevmap]->lvlttl);
		}
		else
		{
			const char *zonttl = (mapheaderinfo[prevmap]->zonttl[0] ? mapheaderinfo[prevmap]->zonttl : "ZONE");
			if (mapheaderinfo[prevmap]->actnum > 0)
				snprintf(data.levelstring,
					sizeof data.levelstring,
					"* %s %s %d *",
					mapheaderinfo[prevmap]->lvlttl, zonttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.levelstring,
					sizeof data.levelstring,
					"* %s %s *",
					mapheaderinfo[prevmap]->lvlttl, zonttl);
		}

		data.levelstring[sizeof data.levelstring - 1] = '\0';

		data.encore = encoremode;

		memset(data.jitter, 0, sizeof (data.jitter));
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		data.val[i] = UINT32_MAX;

		if (!playeringame[i] || players[i].spectator)
		{
			data.increase[i] = INT16_MIN;
			continue;
		}

		if (!rankingsmode)
			data.increase[i] = INT16_MIN;

		numplayersingame++;
	}

	memset(data.color, 0, sizeof (data.color));
	memset(data.character, 0, sizeof (data.character));
	memset(completed, 0, sizeof (completed));
	data.numplayers = 0;

	for (j = 0; j < numplayersingame; j++)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator || completed[i])
				continue;

			comparison(i);
		}

		i = data.num[data.numplayers];

		completed[i] = true;

		data.color[data.numplayers] = &players[i].skincolor;
		data.character[data.numplayers] = &players[i].skin;
		data.name[data.numplayers] = player_names[i];

		if (data.numplayers && (data.val[data.numplayers] == data.val[data.numplayers-1]))
		{
			data.pos[data.numplayers] = data.pos[data.numplayers-1];
		}
		else
		{
			data.pos[data.numplayers] = data.numplayers+1;
		}

		if (!rankingsmode)
		{
			if ((powertype == PWRLV_DISABLED)
				&& !(players[i].pflags & PF_NOCONTEST)
				&& (data.pos[data.numplayers] < (numplayersingame + spectateGriefed)))
			{
				// Online rank is handled further below in this file.
				data.increase[i] = K_CalculateGPRankPoints(data.pos[data.numplayers], numplayersingame + spectateGriefed);
				players[i].score += data.increase[i];
			}

			if (demo.recording)
			{
				G_WriteStanding(
					data.pos[data.numplayers],
					data.name[data.numplayers],
					*data.character[data.numplayers],
					*data.color[data.numplayers],
					data.val[data.numplayers]
				);
			}
		}

		data.numplayers++;
	}
}

//
// Y_IntermissionDrawer
//
// Called by D_Display. Nothing is modified here; all it does is draw. (SRB2Kart: er, about that...)
// Neat concept, huh?
//
void Y_IntermissionDrawer(void)
{
	INT32 i, whiteplayer = MAXPLAYERS, x = 4, hilicol = V_YELLOWMAP; // fallback

	if (intertype == int_none || rendermode == render_none)
		return;

	// the merge was kind of a mess, how does this work -- toast 171021
	{
		M_DrawMenuBackground();
	}

	if (renderisnewtic)
	{
		LUA_HUD_ClearDrawList(luahuddrawlist_intermission);
		LUA_HookHUD(luahuddrawlist_intermission, HUD_HOOK(intermission));
	}
	LUA_HUD_DrawList(luahuddrawlist_intermission);

	if (!LUA_HudEnabled(hud_intermissiontally))
		goto skiptallydrawer;

	if (!r_splitscreen)
		whiteplayer = demo.playback ? displayplayers[0] : consoleplayer;

	if (modeattacking)
		hilicol = V_ORANGEMAP;
	else
		hilicol = ((intertype == int_race) ? V_SKYMAP : V_REDMAP);

	if (sorttic != -1 && intertic > sorttic)
	{
		INT32 count = (intertic - sorttic);

		if (count < 8)
			x -= ((count * vid.width) / (8 * vid.dupx));
		else if (count == 8)
			goto skiptallydrawer;
		else if (count < 16)
			x += (((16 - count) * vid.width) / (8 * vid.dupx));
	}

	if (intertype == int_race || intertype == int_battle || intertype == int_battletime)
	{
#define NUMFORNEWCOLUMN 8
		INT32 y = 41, gutter = ((data.numplayers > NUMFORNEWCOLUMN) ? 0 : (BASEVIDWIDTH/2));
		INT32 dupadjust = (vid.width/vid.dupx), duptweak = (dupadjust - BASEVIDWIDTH)/2;
		const char *timeheader;
		int y2;

		if (data.rankingsmode)
		{
			if (powertype == PWRLV_DISABLED)
			{
				timeheader = "RANK";
			}
			else
			{
				timeheader = "PWR.LV";
			}
		}
		else
		{
			switch (intertype)
			{
				case int_battle:
					timeheader = "SCORE";
					break;
				default:
					timeheader = "TIME";
					break;
			}
		}

		// draw the level name
		V_DrawCenteredString(-4 + x + BASEVIDWIDTH/2, 12, 0, data.levelstring);
		V_DrawFill((x-3) - duptweak, 34, dupadjust-2, 1, 0);

		if (data.encore)
			V_DrawCenteredString(-4 + x + BASEVIDWIDTH/2, 12-8, hilicol, "ENCORE MODE");

		if (data.numplayers > NUMFORNEWCOLUMN)
		{
			V_DrawFill(x+156, 24, 1, 158, 0);
			V_DrawFill((x-3) - duptweak, 182, dupadjust-2, 1, 0);

			V_DrawCenteredString(x+6+(BASEVIDWIDTH/2), 24, hilicol, "#");
			V_DrawString(x+36+(BASEVIDWIDTH/2), 24, hilicol, "NAME");

			V_DrawRightAlignedString(x+152, 24, hilicol, timeheader);
		}

		V_DrawCenteredString(x+6, 24, hilicol, "#");
		V_DrawString(x+36, 24, hilicol, "NAME");

		V_DrawRightAlignedString(x+(BASEVIDWIDTH/2)+152, 24, hilicol, timeheader);

		for (i = 0; i < data.numplayers; i++)
		{
			boolean dojitter = data.jitter[data.num[i]];
			data.jitter[data.num[i]] = 0;

			if (data.num[i] != MAXPLAYERS && playeringame[data.num[i]] && !players[data.num[i]].spectator)
			{
				char strtime[MAXPLAYERNAME+1];

				if (dojitter)
					y--;

				V_DrawCenteredString(x+6, y, 0, va("%d", data.pos[i]));

				if (data.color[i])
				{
					UINT8 *colormap = R_GetTranslationColormap(*data.character[i], *data.color[i], GTC_CACHE);
					V_DrawMappedPatch(x+16, y-4, 0, faceprefix[*data.character[i]][FACE_RANK], colormap);
				}

				if (data.num[i] == whiteplayer)
				{
					UINT8 cursorframe = (intertic / 4) % 8;
					V_DrawScaledPatch(x+16, y-4, 0, W_CachePatchName(va("K_CHILI%d", cursorframe+1), PU_CACHE));
				}

				if ((players[data.num[i]].pflags & PF_NOCONTEST) && players[data.num[i]].bot)
				{
					// RETIRED!!
					V_DrawScaledPatch(x+12, y-7, 0, W_CachePatchName("K_NOBLNS", PU_CACHE));
				}

				STRBUFCPY(strtime, data.name[i]);

				y2 = y;

				if ((netgame || (demo.playback && demo.netgame)) && playerconsole[data.num[i]] == 0 && server_lagless && !players[data.num[i]].bot)
				{
					static int alagles_timer = 0;
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
				}

				if (data.numplayers > NUMFORNEWCOLUMN)
					V_DrawThinString(x+36, y2-1, ((data.num[i] == whiteplayer) ? hilicol : 0)|V_ALLOWLOWERCASE|V_6WIDTHSPACE, strtime);
				else
					V_DrawString(x+36, y2, ((data.num[i] == whiteplayer) ? hilicol : 0)|V_ALLOWLOWERCASE, strtime);

				if (data.rankingsmode)
				{
					if (powertype != PWRLV_DISABLED && !clientpowerlevels[data.num[i]][powertype])
					{
						// No power level (guests)
						STRBUFCPY(strtime, "----");
					}
					else
					{
						if (data.increase[data.num[i]] != INT16_MIN)
						{
							snprintf(strtime, sizeof strtime, "(%d)", data.increase[data.num[i]]);

							if (data.numplayers > NUMFORNEWCOLUMN)
								V_DrawRightAlignedThinString(x+133+gutter, y-1, V_6WIDTHSPACE, strtime);
							else
								V_DrawRightAlignedString(x+118+gutter, y, 0, strtime);
						}

						snprintf(strtime, sizeof strtime, "%d", data.val[i]);
					}

					if (data.numplayers > NUMFORNEWCOLUMN)
						V_DrawRightAlignedThinString(x+152+gutter, y-1, V_6WIDTHSPACE, strtime);
					else
						V_DrawRightAlignedString(x+152+gutter, y, 0, strtime);
				}
				else
				{
					if (data.val[i] == (UINT32_MAX-1))
						V_DrawRightAlignedThinString(x+152+gutter, y-1, (data.numplayers > NUMFORNEWCOLUMN ? V_6WIDTHSPACE : 0), "NO CONTEST.");
					else
					{
						if (intertype == int_race || intertype == int_battletime)
						{
							snprintf(strtime, sizeof strtime, "%i'%02i\"%02i", G_TicsToMinutes(data.val[i], true),
							G_TicsToSeconds(data.val[i]), G_TicsToCentiseconds(data.val[i]));
							strtime[sizeof strtime - 1] = '\0';

							if (data.numplayers > NUMFORNEWCOLUMN)
								V_DrawRightAlignedThinString(x+152+gutter, y-1, V_6WIDTHSPACE, strtime);
							else
								V_DrawRightAlignedString(x+152+gutter, y, 0, strtime);
						}
						else
						{
							if (data.numplayers > NUMFORNEWCOLUMN)
								V_DrawRightAlignedThinString(x+152+gutter, y-1, V_6WIDTHSPACE, va("%i", data.val[i]));
							else
								V_DrawRightAlignedString(x+152+gutter, y, 0, va("%i", data.val[i]));
						}
					}
				}

				if (dojitter)
					y++;
			}
			else
				data.num[i] = MAXPLAYERS; // this should be the only field setting in this function

			y += 18;

			if (i == NUMFORNEWCOLUMN-1)
			{
				y = 41;
				x += BASEVIDWIDTH/2;
			}
#undef NUMFORNEWCOLUMN
		}
	}

skiptallydrawer:
	if (!LUA_HudEnabled(hud_intermissionmessages))
		return;

	if (timer && grandprixinfo.gp == false && bossinfo.boss == false && !modeattacking)
	{
		char *string;
		INT32 tickdown = (timer+1)/TICRATE;

		if (multiplayer && demo.playback)
			string = va("Replay ends in %d", tickdown);
		else
			string = va("%s starts in %d", cv_advancemap.string, tickdown);

		V_DrawCenteredString(BASEVIDWIDTH/2, 188, hilicol, string);

		if ((demo.recording || demo.savemode == DSM_SAVED) && !demo.playback)
		{
			switch (demo.savemode)
			{
				case DSM_NOTSAVING:
					V_DrawRightAlignedThinString(BASEVIDWIDTH - 2, 2, V_SNAPTOTOP|V_SNAPTORIGHT|V_ALLOWLOWERCASE|hilicol, "(B) or (X): Save replay");
					break;

				case DSM_SAVED:
					V_DrawRightAlignedThinString(BASEVIDWIDTH - 2, 2, V_SNAPTOTOP|V_SNAPTORIGHT|V_ALLOWLOWERCASE|hilicol, "Replay saved!");
					break;

				case DSM_TITLEENTRY:
					ST_DrawDemoTitleEntry();
					break;

				default: // Don't render any text here
					break;
			}
		}

		//if ((intertic/TICRATE) & 1) // Make it obvious that scrambling is happening next round. (OR NOT, I GUESS)
		//{
			if (speedscramble != -1 && speedscramble != gamespeed)
			{
				V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-24, hilicol|V_ALLOWLOWERCASE|V_SNAPTOBOTTOM,
					va(M_GetText("Next race will be %s Speed!"), kartspeed_cons_t[1+speedscramble].strvalue));
			}
		//}
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
	{
		if (demo.savemode == DSM_NOTSAVING)
			G_CheckDemoTitleEntry();

		if (demo.savemode == DSM_WILLSAVE || demo.savemode == DSM_WILLAUTOSAVE)
			G_SaveDemo();
	}

	// Check for pause or menu up in single player
	if (paused || P_AutoPause())
		return;

	LUA_HOOK(IntermissionThinker);

	intertic++;

	// Team scramble code for team match and CTF.
	// Don't do this if we're going to automatically scramble teams next round.
	/*if (G_GametypeHasTeams() && cv_teamscramble.value && !cv_scrambleonchange.value && server)
	{
		// If we run out of time in intermission, the beauty is that
		// the P_Ticker() team scramble code will pick it up.
		if ((intertic % (TICRATE/7)) == 0)
			P_DoTeamscrambling();
	}*/

	// multiplayer uses timer (based on cv_inttime)
	if (timer)
	{
		if (!--timer)
		{
			Y_EndIntermission();
			G_AfterIntermission();
			return;
		}
	}
	// single player is hardcoded to go away after awhile
	else if (intertic == endtic)
	{
		Y_EndIntermission();
		G_AfterIntermission();
		return;
	}

	if (intertic < TICRATE || intertic & 1 || endtic != -1)
		return;

	if (intertype == int_race || intertype == int_battle || intertype == int_battletime)
	{
		//if (!(multiplayer && demo.playback)) // Don't advance to rankings in replays
		{
			if (!data.rankingsmode && sorttic != -1 && (intertic >= sorttic + 8))
			{
				K_RetireBots();
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
							if (--data.increase[data.num[q]])
								kaching = false;
						}
					}
				}

				if (r)
				{
					S_StartSound(NULL, (kaching ? sfx_chchng : sfx_ptally));
					Y_CalculateMatchData(2, Y_CompareRank);
				}
				else
					endtic = intertic + 3*TICRATE; // 3 second pause after end of tally
			}
		}
	}
}

//
// Y_DetermineIntermissionType
//
// Determines the intermission type from the current gametype.
//
void Y_DetermineIntermissionType(void)
{
	// set to int_none initially
	intertype = int_none;

	if (intermissiontypes[gametype] != int_none)
		intertype = intermissiontypes[gametype];
	else if (gametype == GT_RACE)
		intertype = int_race;
	else if (gametype == GT_BATTLE)
	{
		UINT8 i = 0, nump = 0;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;
			nump++;
		}
		intertype = (nump < 2 ? int_battletime : int_battle);
	}
}

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

	// set player Power Level type
	powertype = K_UsingPowerLevels();

	// determine the tic the intermission ends
	if (!multiplayer || demo.playback)
	{
		timer = ((nump >= 2) ? 10 : 5)*TICRATE;
	}
	else
	{
		timer = cv_inttime.value*TICRATE;

		if (!timer)
			timer = 1; // prevent a weird bug
	}

	// determine the tic everybody's scores/PWR starts getting sorted
	sorttic = -1;
	if (multiplayer || nump >= 2)
	{
		sorttic = max((timer/2) - 2*TICRATE, 2*TICRATE); // 8 second pause after match results
	}

	if (intermissiontypes[gametype] != int_none)
		intertype = intermissiontypes[gametype];

	// We couldn't display the intermission even if we wanted to.
	// But we still need to give the players their score bonuses, dummy.
	//if (dedicated) return;

	// This should always exist, but just in case...
	if (prevmap >= nummapheaders || !mapheaderinfo[prevmap])
		I_Error("Y_StartIntermission: Internal map ID %d not found (nummapheaders = %d)", prevmap, nummapheaders);

	switch (intertype)
	{
		case int_battle:
		case int_battletime:
		{
			if (cv_inttime.value > 0)
				S_ChangeMusicInternal("racent", true); // loop it

			// Calculate who won
			if (intertype == int_battle)
			{
				Y_CalculateMatchData(0, Y_CompareScore);
				break;
			}
		}
		// FALLTHRU
		case int_race:
		{
			// Calculate who won
			Y_CalculateMatchData(0, Y_CompareTime);
			break;
		}

		case int_none:
		default:
			break;
	}

	LUA_HUD_DestroyDrawList(luahuddrawlist_intermission);
	luahuddrawlist_intermission = LUA_HUD_CreateDrawList();

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

	Automate_Run(AEV_INTERMISSIONSTART);
	bgpatch = W_CachePatchName("MENUBG", PU_STATIC);
	widebgpatch = W_CachePatchName("WEIRDRES", PU_STATIC);

	M_UpdateMenuBGImage(true);
}

// ======

//
// Y_EndIntermission
//
void Y_EndIntermission(void)
{
	K_RetireBots();
	Y_UnloadData();

	endtic = -1;
	sorttic = -1;
	intertype = int_none;
}

//
// Y_FollowIntermission
//
static void Y_FollowIntermission(void)
{
	// This handles whether to play a post-level cutscene, end the game,
	// or simply go to the next level.
	// No need to duplicate the code here!
	G_AfterIntermission();
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
	if (rendermode != render_soft)
		return;

	// unload the background patches
	UNLOAD(bgpatch);
	UNLOAD(widebgpatch);
	UNLOAD(bgtile);
	UNLOAD(interpic);
}

// SRB2Kart: Voting!

//
// Y_VoteDrawer
//
// Draws the voting screen!
//
void Y_VoteDrawer(void)
{
	INT32 i, x, y = 0, height = 0;
	UINT8 selected[4];
	fixed_t rubyheight = 0;

	if (rendermode == render_none)
		return;

	if (votetic >= voteendtic && voteendtic != -1)
		return;

	if (!voteclient.loaded)
		return;

	{
		static angle_t rubyfloattime = 0;
		rubyheight = FINESINE(rubyfloattime>>ANGLETOFINESHIFT);
		rubyfloattime += FixedMul(ANGLE_MAX/NEWTICRATE, renderdeltatics);
	}

	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	if (widebgpatch && rendermode == render_soft && vid.width / vid.dupx > 320)
		V_DrawScaledPatch(((vid.width/2) / vid.dupx) - (SHORT(widebgpatch->width)/2),
							(vid.height / vid.dupy) - SHORT(widebgpatch->height),
							V_SNAPTOTOP|V_SNAPTOLEFT, widebgpatch);
	else
		V_DrawScaledPatch(((vid.width/2) / vid.dupx) - (SHORT(bgpatch->width)/2), // Keep the width/height adjustments, for screens that are less wide than 320(?)
							(vid.height / vid.dupy) - SHORT(bgpatch->height),
							V_SNAPTOTOP|V_SNAPTOLEFT, bgpatch);

	for (i = 0; i < 4; i++) // First, we need to figure out the height of this thing...
	{
		UINT8 j;
		selected[i] = 0; // Initialize

		for (j = 0; j <= splitscreen; j++)
		{
			if (voteclient.playerinfo[j].selection == i)
				selected[i]++;
		}

		if (selected[i])
			height += 50;
		else
			height += 25;

		if (i < 3)
			height += 5-splitscreen;
	}

	y = (200-height)/2;
	for (i = 0; i < 4; i++)
	{
		const char *str;
		patch_t *pic;
		UINT8 j, color;

		if (i == 3)
		{
			str = "RANDOM";
			pic = randomlvl;
		}
		else
		{
			str = levelinfo[i].str;

			pic = NULL;

			if (mapheaderinfo[votelevels[i][0]])
			{
				pic = mapheaderinfo[votelevels[i][0]]->thumbnailPic;
			}

			if (!pic)
			{
				pic = blanklvl;
			}
		}

		if (selected[i])
		{
			UINT8 sizeadd = selected[i];

			for (j = 0; j <= splitscreen; j++) // another loop for drawing the selection backgrounds in the right order, grumble grumble..
			{
				INT32 handy = y;
				UINT8 p;
				UINT8 *colormap;
				patch_t *thiscurs;

				if (voteclient.playerinfo[j].selection != i)
					continue;

				if (!splitscreen)
				{
					thiscurs = cursor;
					p = consoleplayer;
					color = levelinfo[i].gtc;
					colormap = NULL;
				}
				else
				{
					switch (j)
					{
						case 1:
							thiscurs = cursor2;
							p = g_localplayers[1];
							break;
						case 2:
							thiscurs = cursor3;
							p = g_localplayers[2];
							break;
						case 3:
							thiscurs = cursor4;
							p = g_localplayers[3];
							break;
						default:
							thiscurs = cursor1;
							p = g_localplayers[0];
							break;
					}

					color = skincolors[players[p].skincolor].ramp[7];
					colormap = R_GetTranslationColormap(TC_DEFAULT, players[p].skincolor, GTC_CACHE);
				}

				if (votes[p] != -1 || players[p].spectator)
					continue;

				handy += 6*(3-splitscreen) + (13*j);
				V_DrawMappedPatch(BASEVIDWIDTH-124, handy, V_SNAPTORIGHT, thiscurs, colormap);

				if (votetic % 10 < 4)
					V_DrawFill(BASEVIDWIDTH-100-sizeadd, y-sizeadd, 80+(sizeadd*2), 50+(sizeadd*2), 0|V_SNAPTORIGHT);
				else
					V_DrawFill(BASEVIDWIDTH-100-sizeadd, y-sizeadd, 80+(sizeadd*2), 50+(sizeadd*2), color|V_SNAPTORIGHT);

				sizeadd--;
			}

			if (!levelinfo[i].encore)
				V_DrawSmallScaledPatch(BASEVIDWIDTH-100, y, V_SNAPTORIGHT, pic);
			else
			{
				V_DrawFixedPatch((BASEVIDWIDTH-20)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, V_FLIP|V_SNAPTORIGHT, pic, 0);
				V_DrawFixedPatch((BASEVIDWIDTH-60)<<FRACBITS, ((y+25)<<FRACBITS) - (rubyheight<<1), FRACUNIT, V_SNAPTORIGHT, rubyicon, NULL);
			}

			V_DrawRightAlignedThinString(BASEVIDWIDTH-21, 40+y, V_SNAPTORIGHT|V_6WIDTHSPACE, str);

			if (levelinfo[i].gts)
			{
				INT32 w = V_ThinStringWidth(levelinfo[i].gts, V_SNAPTORIGHT)+1;
				V_DrawFill(BASEVIDWIDTH-100, y+10, w+1, 2, V_SNAPTORIGHT|31);
				V_DrawFill(BASEVIDWIDTH-100, y, w, 11, V_SNAPTORIGHT|levelinfo[i].gtc);
				V_DrawDiag(BASEVIDWIDTH-100+w+1, y, 12, V_SNAPTORIGHT|31);
				V_DrawDiag(BASEVIDWIDTH-100+w, y, 11, V_SNAPTORIGHT|levelinfo[i].gtc);
				V_DrawThinString(BASEVIDWIDTH-99, y+1, V_SNAPTORIGHT, levelinfo[i].gts);
			}

			y += 50;
		}
		else
		{
			if (!levelinfo[i].encore)
				V_DrawTinyScaledPatch(BASEVIDWIDTH-60, y, V_SNAPTORIGHT, pic);
			else
			{
				V_DrawFixedPatch((BASEVIDWIDTH-20)<<FRACBITS, y<<FRACBITS, FRACUNIT/4, V_FLIP|V_SNAPTORIGHT, pic, 0);
				V_DrawFixedPatch((BASEVIDWIDTH-40)<<FRACBITS, (y<<FRACBITS) + (25<<(FRACBITS-1)) - rubyheight, FRACUNIT/2, V_SNAPTORIGHT, rubyicon, NULL);
			}

			if (levelinfo[i].gts)
			{
				V_DrawDiag(BASEVIDWIDTH-60, y, 8, V_SNAPTORIGHT|31);
				V_DrawDiag(BASEVIDWIDTH-60, y, 6, V_SNAPTORIGHT|levelinfo[i].gtc);
			}
			y += 25;
		}

		y += 5-splitscreen;
	}

	x = 20;
	y = 10;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (dedicated && i == 0) // While leaving blank spots for non-existent players is largely intentional, the first spot *always* being blank looks a tad silly :V
			continue;

		if ((playeringame[i] && !players[i].spectator) && votes[i] != -1)
		{
			patch_t *pic;

			if (votes[i] >= 3 && (i != pickedvote || voteendtic == -1))
			{
				pic = randomlvl;
			}
			else
			{
				pic = NULL;

				if (mapheaderinfo[votelevels[votes[i]][0]])
				{
					pic = mapheaderinfo[votelevels[votes[i]][0]]->thumbnailPic;
				}

				if (!pic)
				{
					pic = blanklvl;
				}
			}

			if (!timer && i == voteclient.ranim)
			{
				V_DrawScaledPatch(x-18, y+9, V_SNAPTOLEFT, cursor);
				if (voteendtic != -1 && !(votetic % 4))
					V_DrawFill(x-1, y-1, 42, 27, 0|V_SNAPTOLEFT);
				else
					V_DrawFill(x-1, y-1, 42, 27, levelinfo[votes[i]].gtc|V_SNAPTOLEFT);
			}

			if (!levelinfo[votes[i]].encore)
				V_DrawTinyScaledPatch(x, y, V_SNAPTOLEFT, pic);
			else
			{
				V_DrawFixedPatch((x+40)<<FRACBITS, (y)<<FRACBITS, FRACUNIT/4, V_SNAPTOLEFT|V_FLIP, pic, 0);
				V_DrawFixedPatch((x+20)<<FRACBITS, (y<<FRACBITS) + (25<<(FRACBITS-1)) - rubyheight, FRACUNIT/2, V_SNAPTOLEFT, rubyicon, NULL);
			}

			if (levelinfo[votes[i]].gts)
			{
				V_DrawDiag(x, y, 8, V_SNAPTOLEFT|31);
				V_DrawDiag(x, y, 6, V_SNAPTOLEFT|levelinfo[votes[i]].gtc);
			}

			if (players[i].skincolor)
			{
				UINT8 *colormap = R_GetTranslationColormap(players[i].skin, players[i].skincolor, GTC_CACHE);
				V_DrawMappedPatch(x+24, y+9, V_SNAPTOLEFT, faceprefix[players[i].skin][FACE_RANK], colormap);
			}

			if (!splitscreen && i == consoleplayer)
			{
				UINT8 cursorframe = (votetic / 4) % 8;
				V_DrawScaledPatch(x+24, y+9, V_SNAPTOLEFT, W_CachePatchName(va("K_CHILI%d", cursorframe+1), PU_CACHE));
			}
		}

		y += 30;

		if (y > BASEVIDHEIGHT-40)
		{
			x += 60;
			y = 10;
		}
	}

	if (timer)
	{
		INT32 hilicol, tickdown = (timer+1)/TICRATE;
		if (gametype == GT_RACE)
			hilicol = V_SKYMAP;
		else //if (gametype == GT_BATTLE)
			hilicol = V_REDMAP;
		V_DrawCenteredString(BASEVIDWIDTH/2, 188, hilicol,
			va("Vote ends in %d", tickdown));
	}
}

//
// Y_VoteStop
//
// Vote screen's selection stops moving
//
SINT8 deferredlevel = 0;
static void Y_VoteStops(SINT8 pick, SINT8 level)
{
	nextmap = votelevels[level][0];

	//if (level == 4)
	//	S_StartSound(NULL, sfx_noooo2); // gasp
	if (mapheaderinfo[nextmap] && (mapheaderinfo[nextmap]->menuflags & LF2_HIDEINMENU))
		S_StartSound(NULL, sfx_noooo1); // this is bad
	else if (netgame && P_IsLocalPlayer(&players[pick]))
		S_StartSound(NULL, sfx_yeeeah); // yeeeah!
	else
		S_StartSound(NULL, sfx_kc48); // just a cool sound

	if (gametype != votelevels[level][1])
	{
		INT16 lastgametype = gametype;
		G_SetGametype(votelevels[level][1]);
		D_GameTypeChanged(lastgametype);
		forceresetplayers = true;
	}

	deferencoremode = (levelinfo[level].encore);
}

//
// Y_VoteTicker
//
// Vote screen thinking :eggthinking:
//
void Y_VoteTicker(void)
{
	INT32 i;
	boolean everyone_voted;

	if (paused || P_AutoPause() || !voteclient.loaded)
		return;

	LUA_HOOK(VoteThinker);

	votetic++;

	if (votetic == voteendtic)
	{
		Y_EndVote();
		Y_FollowIntermission();
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++) // Correct votes as early as possible, before they're processed by the game at all
	{
		if (!playeringame[i] || players[i].spectator)
			votes[i] = -1; // Spectators are the lower class, and have effectively no voice in the government. Democracy sucks.
		else if (pickedvote != -1 && votes[i] == -1)
			votes[i] = 3; // Slow people get random
	}

	if (server && pickedvote != -1 && votes[pickedvote] == -1) // Uh oh! The person who got picked left! Recalculate, quick!
		D_PickVote();

	if (!votetic)
		S_ChangeMusicInternal("vote", true);

	if (timer)
		timer--;

	if (pickedvote != -1)
	{
		timer = 0;
		voteclient.rsynctime++;

		if (voteendtic == -1)
		{
			UINT8 tempvotes[MAXPLAYERS];
			UINT8 numvotes = 0;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (votes[i] == -1)
					continue;
				tempvotes[numvotes] = i;
				numvotes++;
			}

			if (numvotes < 1) // Whoops! Get outta here.
			{
				Y_EndVote();
				Y_FollowIntermission();
				return;
			}

			voteclient.rtics--;

			if (voteclient.rtics <= 0)
			{
				voteclient.roffset++;
				voteclient.rtics = min(20, (3*voteclient.roffset/4)+5);
				S_StartSound(NULL, sfx_kc39);
			}

			if (voteclient.rendoff == 0 || voteclient.roffset < voteclient.rendoff)
				voteclient.ranim = tempvotes[((pickedvote + voteclient.roffset) % numvotes)];

			if (voteclient.roffset >= 20)
			{
				if (voteclient.rendoff == 0)
				{
					if (voteclient.rsynctime % 51 == 0) // Song is 1.45 seconds long (sorry @ whoever wants to replace it in a music wad :V)
					{
						for (i = 5; i >= 3; i--) // Find a suitable place to stop
						{
							if (tempvotes[((pickedvote + voteclient.roffset + i) % numvotes)] == pickedvote)
							{
								voteclient.rendoff = voteclient.roffset+i;
								if (M_RandomChance(FRACUNIT/32)) // Let it cheat occasionally~
									voteclient.rendoff++;
								S_ChangeMusicInternal("voteeb", false);
								break;
							}
						}
					}
				}
				else if (voteclient.roffset >= voteclient.rendoff)
				{
					voteendtic = votetic + (3*TICRATE);
					Y_VoteStops(pickedvote, deferredlevel);
				}
			}
		}
		else
			voteclient.ranim = pickedvote;
	}
	else if (votenotyetpicked)
	{
		if (votetic < 3*(NEWTICRATE/7)) // give it some time before letting you control it :V
			return;

		/*
		The vote ended, but it will take at least a tic for that to reach us from
		the server. Don't let me change the vote now, it won't matter anyway!
		*/
		if (timer)
		{
			for (i = 0; i <= splitscreen; i++)
			{
				UINT8 p;
				boolean pressed = false;

				switch (i)
				{
					case 1:
						p = g_localplayers[1];
						break;
					case 2:
						p = g_localplayers[2];
						break;
					case 3:
						p = g_localplayers[3];
						break;
					default:
						p = consoleplayer;
						break;
				}

				if (voteclient.playerinfo[i].delay)
					voteclient.playerinfo[i].delay--;

				if ((playeringame[p] && !players[p].spectator)
						&& !voteclient.playerinfo[i].delay
						&& pickedvote == -1 && votes[p] == -1 && menuactive == false)
				{
					if (G_PlayerInputDown(i, gc_up, 0))
					{
						voteclient.playerinfo[i].selection--;
						pressed = true;
					}

					if (G_PlayerInputDown(i, gc_down, 0) && pressed == false)
					{
						voteclient.playerinfo[i].selection++;
						pressed = true;
					}

					if (voteclient.playerinfo[i].selection < 0)
						voteclient.playerinfo[i].selection = 3;
					if (voteclient.playerinfo[i].selection > 3)
						voteclient.playerinfo[i].selection = 0;

					if (G_PlayerInputDown(i, gc_a, 0) && pressed == false)
					{
						D_ModifyClientVote(consoleplayer, voteclient.playerinfo[i].selection, i);
						pressed = true;
					}
				}

				if (pressed)
				{
					S_StartSound(NULL, sfx_kc4a);
					voteclient.playerinfo[i].delay = NEWTICRATE/7;
				}
			}
		}

		if (server)
		{
			everyone_voted = true;/* the default condition */

			if (timer == 0)
			{
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if ((playeringame[i] && !players[i].spectator) && votes[i] == -1)
						votes[i] = 3;
				}
			}
			else
			{
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if ((playeringame[i] && !players[i].spectator) && votes[i] == -1)
					{
						if (players[i].bot)
						{
							if (( M_RandomFixed() % 100 ) == 0)
								D_ModifyClientVote(i, M_RandomKey(4), 0);
						}

						if (votes[i] == -1)
							everyone_voted = false;
					}
				}
			}

			if (everyone_voted)
			{
				timer = 0;
				if (voteendtic == -1)
				{
					votenotyetpicked = false;/* don't pick vote twice */
					D_PickVote();
				}
			}
		}
	}
}

//
// Y_StartVote
//
// MK online style voting screen, appears after intermission
//
void Y_StartVote(void)
{
	INT32 i = 0;
	boolean battlemode = ((votelevels[0][1] & ~VOTEMODIFIER_ENCORE) == GT_BATTLE); // todo gametyperules

	votetic = -1;

#ifdef PARANOIA
	if (voteendtic != -1)
		I_Error("voteendtic is dirty");
#endif

	widebgpatch = W_CachePatchName((battlemode ? "BATTLSCW" : "INTERSCW"), PU_STATIC);
	bgpatch = W_CachePatchName((battlemode ? "BATTLSCR" : "INTERSCR"), PU_STATIC);
	cursor = W_CachePatchName("M_CURSOR", PU_STATIC);
	cursor1 = W_CachePatchName("P1CURSOR", PU_STATIC);
	cursor2 = W_CachePatchName("P2CURSOR", PU_STATIC);
	cursor3 = W_CachePatchName("P3CURSOR", PU_STATIC);
	cursor4 = W_CachePatchName("P4CURSOR", PU_STATIC);
	randomlvl = W_CachePatchName("RANDOMLV", PU_STATIC);
	rubyicon = W_CachePatchName("RUBYICON", PU_STATIC);

	timer = cv_votetime.value*TICRATE;
	pickedvote = -1;

	votenotyetpicked = true;

	for (i = 0; i < 3; i++)
	{
		voteclient.playerinfo[i].selection = 0;
		voteclient.playerinfo[i].delay = 0;
	}

	voteclient.ranim = 0;
	voteclient.rtics = 1;
	voteclient.roffset = 0;
	voteclient.rsynctime = 0;
	voteclient.rendoff = 0;

	for (i = 0; i < MAXPLAYERS; i++)
		votes[i] = -1;

	for (i = 0; i < 4; i++)
	{
		// set up the encore
		levelinfo[i].encore = (votelevels[i][1] & VOTEMODIFIER_ENCORE);
		votelevels[i][1] &= ~VOTEMODIFIER_ENCORE;

		// set up the levelstring
		if (mapheaderinfo[votelevels[i][0]]->levelflags & LF_NOZONE || !mapheaderinfo[votelevels[i][0]]->zonttl[0])
		{
			if (mapheaderinfo[votelevels[i][0]]->actnum > 0)
				snprintf(levelinfo[i].str,
					sizeof levelinfo[i].str,
					"%s %d",
					mapheaderinfo[votelevels[i][0]]->lvlttl, mapheaderinfo[votelevels[i][0]]->actnum);
			else
				snprintf(levelinfo[i].str,
					sizeof levelinfo[i].str,
					"%s",
					mapheaderinfo[votelevels[i][0]]->lvlttl);
		}
		else
		{
			if (mapheaderinfo[votelevels[i][0]]->actnum > 0)
				snprintf(levelinfo[i].str,
					sizeof levelinfo[i].str,
					"%s %s %d",
					mapheaderinfo[votelevels[i][0]]->lvlttl, mapheaderinfo[votelevels[i][0]]->zonttl, mapheaderinfo[votelevels[i][0]]->actnum);
			else
				snprintf(levelinfo[i].str,
					sizeof levelinfo[i].str,
					"%s %s",
					mapheaderinfo[votelevels[i][0]]->lvlttl, mapheaderinfo[votelevels[i][0]]->zonttl);
		}

		levelinfo[i].str[sizeof levelinfo[i].str - 1] = '\0';

		// set up the gtc and gts
		levelinfo[i].gtc = G_GetGametypeColor(votelevels[i][1]);
		if (i == 2 && votelevels[i][1] != votelevels[0][1])
			levelinfo[i].gts = Gametype_Names[votelevels[i][1]];
		else
			levelinfo[i].gts = NULL;
	}

	voteclient.loaded = true;
	Automate_Run(AEV_VOTESTART);
}

//
// Y_EndVote
//
void Y_EndVote(void)
{
	Y_UnloadVoteData();
	voteendtic = -1;
}

//
// Y_UnloadVoteData
//
static void Y_UnloadVoteData(void)
{
	voteclient.loaded = false;

	if (rendermode != render_soft)
		return;

	UNLOAD(widebgpatch);
	UNLOAD(bgpatch);
	UNLOAD(cursor);
	UNLOAD(cursor1);
	UNLOAD(cursor2);
	UNLOAD(cursor3);
	UNLOAD(cursor4);
	UNLOAD(randomlvl);
	UNLOAD(rubyicon);
}

//
// Y_SetupVoteFinish
//
void Y_SetupVoteFinish(SINT8 pick, SINT8 level)
{
	if (!voteclient.loaded)
		return;

	if (pick == -1) // No other votes? We gotta get out of here, then!
	{
		Y_EndVote();
		Y_FollowIntermission();
		return;
	}

	if (pickedvote == -1)
	{
		INT32 i;
		SINT8 votecompare = -1;
		INT32 endtype = 0;

		voteclient.rsynctime = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if ((playeringame[i] && !players[i].spectator) && votes[i] == -1)
				votes[i] = 3;

			if (votes[i] == -1 || endtype > 1) // Don't need to go on
				continue;

			if (endtype == 2)
				continue;

			if (votecompare == -1)
			{
				votecompare = votes[i];
				endtype = 1;
			}
			else if (votes[i] != votecompare)
				endtype = 2;
		}

		if (endtype == 1) // Only one unique vote, so just end it immediately.
		{
			voteendtic = votetic + (5*TICRATE);
			S_ChangeMusicInternal("voteeb", false);
			Y_VoteStops(pick, level);
		}
		else if (endtype == 0) // Might as well put this here, too.
		{
			Y_EndVote();
			Y_FollowIntermission();
			return;
		}
		else
			S_ChangeMusicInternal("voteea", true);
	}

	deferredlevel = level;
	pickedvote = pick;
	timer = 0;
}
