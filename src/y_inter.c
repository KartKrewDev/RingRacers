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
#include "m_menu.h"
#include "m_misc.h"
#include "i_system.h"
#include "p_setup.h"

#include "r_local.h"
#include "p_local.h"

#include "m_cond.h" // condition sets
#include "lua_hook.h" // IntermissionThinker hook

#include "lua_hud.h"

#include "m_random.h" // M_RandomKey
#include "g_input.h" // PLAYER1INPUTDOWN
#include "k_color.h" // colortranslations
#include "k_battle.h"
#include "k_pwrlv.h"
#include "console.h" // cons_menuhighlight
#include "lua_hook.h" // IntermissionThinker hook

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
#endif

typedef struct
{
	char patch[9];
	 INT32 points;
	UINT8 display;
} y_bonus_t;

typedef union
{
	/*struct
	{
		char passed1[21]; // KNUCKLES GOT    / CRAWLA HONCHO
		char passed2[16]; // THROUGH THE ACT / PASSED THE ACT
		INT32 passedx1;
		INT32 passedx2;

		y_bonus_t bonuses[4];
		patch_t *bonuspatches[4];

		SINT8 gotperfbonus; // Used for visitation flags.

		UINT32 score, total; // fake score, total
		UINT32 tics; // time

		UINT8 actnum; // act number being displayed
		patch_t *ptotal; // TOTAL
		UINT8 gotlife; // Number of extra lives obtained
<<<<<<< HEAD
	} coop;*/

	struct
	{
		UINT8 *color[MAXPLAYERS]; // Winner's color #
=======
	} coop;

	struct
	{
		char passed1[29];             // KNUCKLES GOT     / CRAWLA HONCHO
		char passed2[17];             // A CHAOS EMERALD? / GOT THEM ALL!
		char passed3[15];             //                   CAN NOW BECOME
		char passed4[SKINNAMESIZE+7]; //                   SUPER CRAWLA HONCHO
		INT32 passedx1;
		INT32 passedx2;
		INT32 passedx3;
		INT32 passedx4;

		y_bonus_t bonuses[2];
		patch_t *bonuspatches[2];

		patch_t *pscore; // SCORE
		UINT32 score; // fake score

		// Continues
		UINT8 continues;
		patch_t *pcontinues;
		INT32 *playerchar; // Continue HUD
		UINT16 *playercolor;

		UINT8 gotlife; // Number of extra lives obtained
	} spec;

	struct
	{
		UINT32 scores[MAXPLAYERS]; // Winner's score
		UINT16 *color[MAXPLAYERS]; // Winner's color #
		boolean spectator[MAXPLAYERS]; // Spectator list
>>>>>>> srb2/next
		INT32 *character[MAXPLAYERS]; // Winner's character #
		INT32 num[MAXPLAYERS]; // Winner's player #
		char *name[MAXPLAYERS]; // Winner's name
		INT32 numplayers; // Number of players being displayed
		char levelstring[64]; // holds levelnames up to 64 characters
		// SRB2kart
		INT16 increase[MAXPLAYERS]; // how much did the score increase by?
		UINT8 jitter[MAXPLAYERS]; // wiggle
		UINT32 val[MAXPLAYERS]; // Gametype-specific value
		UINT8 pos[MAXPLAYERS]; // player positions. used for ties
		boolean rankingsmode; // rankings mode
		boolean encore; // encore mode
	} match;
<<<<<<< HEAD
=======

	struct
	{
		UINT16 *color[MAXPLAYERS]; // Winner's color #
		INT32 *character[MAXPLAYERS]; // Winner's character #
		INT32 num[MAXPLAYERS]; // Winner's player #
		char name[MAXPLAYERS][9]; // Winner's name
		UINT32 times[MAXPLAYERS];
		UINT32 rings[MAXPLAYERS];
		UINT32 maxrings[MAXPLAYERS];
		UINT32 monitors[MAXPLAYERS];
		UINT32 scores[MAXPLAYERS];
		UINT32 points[MAXPLAYERS];
		INT32 numplayers; // Number of players being displayed
		char levelstring[40]; // holds levelnames up to 32 characters
	} competition;

>>>>>>> srb2/next
} y_data;

static y_data data;

// graphics
static patch_t *bgpatch = NULL;     // INTERSCR
static patch_t *widebgpatch = NULL; // INTERSCW
static patch_t *bgtile = NULL;      // SPECTILE/SRB2BACK
static patch_t *interpic = NULL;    // custom picture defined in map header
static boolean usetile;
static INT32 timer;

typedef struct
{
	INT32 source_width, source_height;
	INT32 source_bpp, source_rowbytes;
	UINT8 *source_picture;
	INT32 target_width, target_height;
	INT32 target_bpp, target_rowbytes;
	UINT8 *target_picture;
} y_buffer_t;

boolean usebuffer = false;
static boolean useinterpic;
<<<<<<< HEAD
static INT32 timer;
static INT32 powertype = PWRLV_DISABLED;
=======
static boolean safetorender = true;
static y_buffer_t *y_buffer;
>>>>>>> srb2/next

static INT32 intertic;
static INT32 tallydonetic = -1;
static INT32 endtic = -1;
static INT32 sorttic = -1;

intertype_t intertype = int_none;
intertype_t intermissiontypes[NUMGAMETYPES];

<<<<<<< HEAD
static void Y_FollowIntermission(void);
=======
static void Y_RescaleScreenBuffer(void);
static void Y_AwardCoopBonuses(void);
static void Y_AwardSpecialStageBonus(void);
static void Y_CalculateCompetitionWinners(void);
static void Y_CalculateTimeRaceWinners(void);
static void Y_CalculateMatchWinners(void);
>>>>>>> srb2/next
static void Y_UnloadData(void);
static void Y_CleanupData(void);

<<<<<<< HEAD
// SRB2Kart: voting stuff
// Level images
typedef struct
{
	char str[62];
	UINT8 gtc;
	const char *gts;
	patch_t *pic;
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
	UINT32 val = ((players[i].pflags & PF_TIMEOVER || players[i].realtime == UINT32_MAX)
		? (UINT32_MAX-1) : players[i].realtime);

	if (!(val < data.match.val[data.match.numplayers]))
		return;

	data.match.val[data.match.numplayers] = val;
	data.match.num[data.match.numplayers] = i;
}

static void Y_CompareScore(INT32 i)
{
	UINT32 val = ((players[i].pflags & PF_TIMEOVER)
			? (UINT32_MAX-1) : players[i].marescore);

	if (!(data.match.val[data.match.numplayers] == UINT32_MAX
	|| (!(players[i].pflags & PF_TIMEOVER) && val > data.match.val[data.match.numplayers])))
		return;

	data.match.val[data.match.numplayers] = val;
	data.match.num[data.match.numplayers] = i;
}

static void Y_CompareRank(INT32 i)
{
	INT16 increase = ((data.match.increase[i] == INT16_MIN) ? 0 : data.match.increase[i]);
	UINT32 score = (powertype != -1 ? clientpowerlevels[i][powertype] : players[i].score);

	if (!(data.match.val[data.match.numplayers] == UINT32_MAX || (score - increase) > data.match.val[data.match.numplayers]))
		return;

	data.match.val[data.match.numplayers] = (score - increase);
	data.match.num[data.match.numplayers] = i;
}

static void Y_CalculateMatchData(UINT8 rankingsmode, void (*comparison)(INT32))
{
	INT32 i, j;
	boolean completed[MAXPLAYERS];
	INT32 numplayersingame = 0, numgriefers = 0;

	// Initialize variables
	if (rankingsmode > 1)
		;
	else if ((data.match.rankingsmode = (boolean)rankingsmode))
	{
		sprintf(data.match.levelstring, "* Total Rankings *");
		data.match.encore = false;
	}
	else
	{
		// set up the levelstring
		if (mapheaderinfo[prevmap]->levelflags & LF_NOZONE)
		{
			if (mapheaderinfo[prevmap]->actnum[0])
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"* %s %s *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"* %s *",
					mapheaderinfo[prevmap]->lvlttl);
		}
		else
		{
			const char *zonttl = (mapheaderinfo[prevmap]->zonttl[0] ? mapheaderinfo[prevmap]->zonttl : "ZONE");
			if (mapheaderinfo[prevmap]->actnum[0])
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"* %s %s %s *",
					mapheaderinfo[prevmap]->lvlttl, zonttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.match.levelstring,
					sizeof data.match.levelstring,
					"* %s %s *",
					mapheaderinfo[prevmap]->lvlttl, zonttl);
		}

		data.match.levelstring[sizeof data.match.levelstring - 1] = '\0';

		data.match.encore = encoremode;

		memset(data.match.jitter, 0, sizeof (data.match.jitter));
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		data.match.val[i] = UINT32_MAX;

		if (nospectategrief[i] != -1)
			numgriefers++;

		if (!playeringame[i] || players[i].spectator)
		{
			data.match.increase[i] = INT16_MIN;
			continue;
		}

		if (!rankingsmode)
			data.match.increase[i] = INT16_MIN;

		numplayersingame++;
	}

	memset(data.match.color, 0, sizeof (data.match.color));
	memset(data.match.character, 0, sizeof (data.match.character));
	memset(completed, 0, sizeof (completed));
	data.match.numplayers = 0;

	for (j = 0; j < numplayersingame; j++)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator || completed[i])
				continue;

			comparison(i);
		}

		i = data.match.num[data.match.numplayers];

		completed[i] = true;

		data.match.color[data.match.numplayers] = &players[i].skincolor;
		data.match.character[data.match.numplayers] = &players[i].skin;
		data.match.name[data.match.numplayers] = player_names[i];

		if (data.match.numplayers && (data.match.val[data.match.numplayers] == data.match.val[data.match.numplayers-1]))
			data.match.pos[data.match.numplayers] = data.match.pos[data.match.numplayers-1];
		else
			data.match.pos[data.match.numplayers] = data.match.numplayers+1;

		if ((!rankingsmode && powertype == -1) // Single player rankings (grand prix). Online rank is handled below.
			&& !(players[i].pflags & PF_TIMEOVER) && (data.match.pos[data.match.numplayers] < (numplayersingame + numgriefers)))
		{
			data.match.increase[i] = (numplayersingame + numgriefers) - data.match.pos[data.match.numplayers];
			players[i].score += data.match.increase[i];
		}

		if (demo.recording && !rankingsmode)
			G_WriteStanding(
				data.match.pos[data.match.numplayers],
				data.match.name[data.match.numplayers],
				*data.match.character[data.match.numplayers],
				*data.match.color[data.match.numplayers],
				data.match.val[data.match.numplayers]
			);

		data.match.numplayers++;
	}
=======
// Stuff copy+pasted from st_stuff.c
#define ST_DrawNumFromHud(h,n)        V_DrawTallNum(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f, n)
#define ST_DrawPadNumFromHud(h,n,q)   V_DrawPaddedTallNum(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f, n, q)
#define ST_DrawPatchFromHud(h,p)      V_DrawScaledPatch(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f, p)

static void Y_IntermissionTokenDrawer(void)
{
	INT32 y, offs, lowy, calc;
	UINT32 tokencount;
	INT16 temp;
	UINT8 em;

	offs = 0;
	lowy = BASEVIDHEIGHT - 32 - 8;
	temp = SHORT(tokenicon->height)/2;

	em = 0;
	while (emeralds & (1 << em))
		if (++em == 7)
			return;

	if (tallydonetic != -1)
	{
		offs = (intertic - tallydonetic)*2;
		if (offs > 10)
			offs = 8;
	}

	V_DrawSmallScaledPatch(32, lowy-1, 0, emeraldpics[2][em]); // coinbox

	y = (lowy + offs + 1) - (temp + (token + 1)*8);

	for (tokencount = token; tokencount; tokencount--)
	{
		if (y >= -temp)
			V_DrawSmallScaledPatch(32, y, 0, tokenicon);
		y += 8;
	}

	y += (offs*(temp - 1)/8);
	calc = (lowy - y)*2;

	if (calc > 0)
		V_DrawCroppedPatch(32<<FRACBITS, y<<FRACBITS, FRACUNIT/2, 0, tokenicon, 0, 0, SHORT(tokenicon->width), calc);
}

//
// Y_ConsiderScreenBuffer
//
// Can we copy the current screen
// to a buffer?
//
void Y_ConsiderScreenBuffer(void)
{
	if (gameaction != ga_completed)
		return;

	if (y_buffer == NULL)
		y_buffer = Z_Calloc(sizeof(y_buffer_t), PU_STATIC, NULL);
	else
		return;

	y_buffer->source_width = vid.width;
	y_buffer->source_height = vid.height;
	y_buffer->source_bpp = vid.bpp;
	y_buffer->source_rowbytes = vid.rowbytes;
	y_buffer->source_picture = ZZ_Alloc(y_buffer->source_width*vid.bpp * y_buffer->source_height);
	VID_BlitLinearScreen(screens[1], y_buffer->source_picture, vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);

	// Make the rescaled screen buffer
	Y_RescaleScreenBuffer();
}

//
// Y_RescaleScreenBuffer
//
// Write the rescaled source picture,
// to the destination picture that
// has the current screen's resolutions.
//
static void Y_RescaleScreenBuffer(void)
{
	INT32 sx, sy; // source
	INT32 dx, dy; // dest
	fixed_t scalefac, yscalefac;
	fixed_t rowfrac, colfrac;
	UINT8 *dest;

	// Who knows?
	if (y_buffer == NULL)
		return;

	if (y_buffer->target_picture)
		Z_Free(y_buffer->target_picture);

	y_buffer->target_width = vid.width;
	y_buffer->target_height = vid.height;
	y_buffer->target_rowbytes = vid.rowbytes;
	y_buffer->target_bpp = vid.bpp;
	y_buffer->target_picture = ZZ_Alloc(y_buffer->target_width*vid.bpp * y_buffer->target_height);
	dest = y_buffer->target_picture;

	scalefac = FixedDiv(y_buffer->target_width*FRACUNIT, y_buffer->source_width*FRACUNIT);
	yscalefac = FixedDiv(y_buffer->target_height*FRACUNIT, y_buffer->source_height*FRACUNIT);

	rowfrac = FixedDiv(FRACUNIT, yscalefac);
	colfrac = FixedDiv(FRACUNIT, scalefac);

	for (sy = 0, dy = 0; sy < (y_buffer->source_height << FRACBITS) && dy < y_buffer->target_height; sy += rowfrac, dy++)
		for (sx = 0, dx = 0; sx < (y_buffer->source_width << FRACBITS) && dx < y_buffer->target_width; sx += colfrac, dx += y_buffer->target_bpp)
			dest[(dy * y_buffer->target_rowbytes) + dx] = y_buffer->source_picture[((sy>>FRACBITS) * y_buffer->source_width) + (sx>>FRACBITS)];
}

//
// Y_CleanupScreenBuffer
//
// Free all related memory.
//
void Y_CleanupScreenBuffer(void)
{
	// Who knows?
	if (y_buffer == NULL)
		return;

	if (y_buffer->target_picture)
		Z_Free(y_buffer->target_picture);

	if (y_buffer->source_picture)
		Z_Free(y_buffer->source_picture);

	Z_Free(y_buffer);
	y_buffer = NULL;
>>>>>>> srb2/next
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

	if (rendermode == render_none)
		return;

	if (intertype == int_none)
	{
		LUAh_IntermissionHUD();
		return;
	}

	if (!usebuffer)
	// Lactozilla: Renderer switching
	if (needpatchrecache)
	{
		Y_CleanupData();
		safetorender = false;
	}

	if (!usebuffer || !safetorender)
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	if (!safetorender)
		goto dontdrawbg;

	if (useinterpic)
		V_DrawScaledPatch(0, 0, 0, interpic);
	else if (!usetile)
	{
		if (rendermode == render_soft && usebuffer)
		{
			// no y_buffer
			if (y_buffer == NULL)
				VID_BlitLinearScreen(screens[1], screens[0], vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);
			else
			{
				// Maybe the resolution changed?
				if ((y_buffer->target_width != vid.width) || (y_buffer->target_height != vid.height))
					Y_RescaleScreenBuffer();

				// Blit the already-scaled screen buffer to the current screen
				VID_BlitLinearScreen(y_buffer->target_picture, screens[0], vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);
			}
		}
#ifdef HWRENDER
		else if (rendermode != render_soft && usebuffer)
			HWR_DrawIntermissionBG();
#endif
		else
		{
			if (widebgpatch && rendermode == render_soft && vid.width / vid.dupx == 400)
				V_DrawScaledPatch(0, 0, V_SNAPTOLEFT, widebgpatch);
			else
				V_DrawScaledPatch(0, 0, 0, bgpatch);
		}
	}
	else
		V_DrawPatchFill(bgtile);

<<<<<<< HEAD
	if (usebuffer) // Fade everything out
		V_DrawFadeScreen(0xFF00, 22);

	if (!r_splitscreen)
		whiteplayer = demo.playback ? displayplayers[0] : consoleplayer;

	if (cons_menuhighlight.value)
		hilicol = cons_menuhighlight.value;
	else if (modeattacking)
		hilicol = V_ORANGEMAP;
	else
		hilicol = ((intertype == int_race) ? V_SKYMAP : V_REDMAP);

	if (sorttic != -1 && intertic > sorttic && !demo.playback)
	{
		INT32 count = (intertic - sorttic);

		if (count < 8)
			x -= ((count * vid.width) / (8 * vid.dupx));
		else if (count == 8)
			goto dotimer;
		else if (count < 16)
			x += (((16 - count) * vid.width) / (8 * vid.dupx));
	}

	// SRB2kart 290117 - compeltely replaced this block.
	/*if (intertype == int_timeattack)
	{
		// draw time
		ST_DrawPatchFromHud(HUD_TIME, sbotime);
		if (cv_timetic.value)
			ST_DrawNumFromHud(HUD_SECONDS, data.coop.tics);
		else
=======
	LUAh_IntermissionHUD();
	if (!LUA_HudEnabled(hud_intermissiontally))
		goto skiptallydrawer;

dontdrawbg:
	if (intertype == int_coop)
	{
		INT32 bonusy;

		if (gottoken) // first to be behind everything else
			Y_IntermissionTokenDrawer();

		if (!splitscreen)
>>>>>>> srb2/next
		{
			// draw score
			ST_DrawPatchFromHud(HUD_SCORE, sboscore);
			ST_DrawNumFromHud(HUD_SCORENUM, data.coop.score);

			// draw time
			ST_DrawPatchFromHud(HUD_TIME, sbotime);
			if (cv_timetic.value == 3)
				ST_DrawNumFromHud(HUD_SECONDS, data.coop.tics);
			else
			{
				INT32 seconds, minutes, tictrn;

				seconds = G_TicsToSeconds(data.coop.tics);
				minutes = G_TicsToMinutes(data.coop.tics, true);
				tictrn  = G_TicsToCentiseconds(data.coop.tics);

				ST_DrawNumFromHud(HUD_MINUTES, minutes); // Minutes
				ST_DrawPatchFromHud(HUD_TIMECOLON, sbocolon); // Colon
				ST_DrawPadNumFromHud(HUD_SECONDS, seconds, 2); // Seconds

<<<<<<< HEAD
			// SRB2kart - pulled from old coop block, just in case we need it
			// we should show centiseconds on the intermission screen too, if the conditions are right.
			if (modeattacking || cv_timetic.value == 2)
			{
				ST_DrawPatchFromHud(HUD_TIMETICCOLON, sboperiod); // Period
				ST_DrawPadNumFromHud(HUD_TICS, tictrn, 2); // Tics
=======
				if (cv_timetic.value == 1 || cv_timetic.value == 2 || modeattacking) // there's not enough room for tics in splitscreen, don't even bother trying!
				{
					ST_DrawPatchFromHud(HUD_TIMETICCOLON, sboperiod); // Period
					ST_DrawPadNumFromHud(HUD_TICS, tictrn, 2); // Tics
				}
>>>>>>> srb2/next
			}

			ST_DrawPatchFromHud(HUD_TIMETICCOLON, sboperiod); // Period
			ST_DrawPadNumFromHud(HUD_TICS, tictrn, 2); // Tics
		}

		// draw the "got through act" lines and act number
		V_DrawLevelTitle(data.coop.passedx1, 49, 0, data.coop.passed1);
		{
			INT32 h = V_LevelNameHeight(data.coop.passed2);
			V_DrawLevelTitle(data.coop.passedx2, 49+h+2, 0, data.coop.passed2);

<<<<<<< HEAD
		if (strlen(mapheaderinfo[prevmap]->actnum) > 0)
			V_DrawScaledPatch(244, 57, 0, data.coop.ttlnum);

		//if (gottimebonus && endtic != -1)
		//	V_DrawCenteredString(BASEVIDWIDTH/2, 172, V_YELLOWMAP, "TIME BONUS UNLOCKED!");
=======
			if (data.coop.actnum)
				V_DrawLevelActNum(244, 42+h, 0, data.coop.actnum);
		}

		bonusy = 150;
		// Total
		if (safetorender)
		{
			V_DrawScaledPatch(152, bonusy, 0, data.coop.ptotal);
			V_DrawTallNum(BASEVIDWIDTH - 68, bonusy + 1, 0, data.coop.total);
		}
		bonusy -= (3*SHORT(tallnum[0]->height)/2) + 1;

		// Draw bonuses
		for (i = 3; i >= 0; --i)
		{
			if (data.coop.bonuses[i].display && safetorender)
			{
				V_DrawScaledPatch(152, bonusy, 0, data.coop.bonuspatches[i]);
				V_DrawTallNum(BASEVIDWIDTH - 68, bonusy + 1, 0, data.coop.bonuses[i].points);
			}
			bonusy -= (3*SHORT(tallnum[0]->height)/2) + 1;
		}
>>>>>>> srb2/next
	}
	else*/ if (intertype == int_race || intertype == int_match)
	{
<<<<<<< HEAD
#define NUMFORNEWCOLUMN 8
		INT32 y = 41, gutter = ((data.match.numplayers > NUMFORNEWCOLUMN) ? 0 : (BASEVIDWIDTH/2));
		INT32 dupadjust = (vid.width/vid.dupx), duptweak = (dupadjust - BASEVIDWIDTH)/2;
		const char *timeheader;
		int y2;

		if (data.match.rankingsmode)
			timeheader = (powertype != -1 ? "PWR.LV" : "RANK");
		else
			timeheader = ((intertype == int_race || (intertype == int_match && battlecapsules)) ? "TIME" : "SCORE");
=======
		static tic_t animatetic = 0;
		INT32 ttheight = 16;
		INT32 xoffset1 = 0; // Line 1 x offset
		INT32 xoffset2 = 0; // Line 2 x offset
		INT32 xoffset3 = 0; // Line 3 x offset
		INT32 xoffset4 = 0; // Line 4 x offset
		INT32 xoffset5 = 0; // Line 5 x offset
		INT32 xoffset6 = 0; // Line 6 x offset
		UINT8 drawsection = 0;

		if (gottoken) // first to be behind everything else
			Y_IntermissionTokenDrawer();

		// draw the header
		if (intertic <= 2*TICRATE)
			animatetic = 0;
		else if (!animatetic && data.spec.bonuses[0].points == 0 && data.spec.bonuses[1].points == 0 && data.spec.passed3[0] != '\0')
			animatetic = intertic + TICRATE;

		if (animatetic && (tic_t)intertic >= animatetic)
		{
			const INT32 scradjust = (vid.width/vid.dupx)>>3; // 40 for BASEVIDWIDTH
			INT32 animatetimer = (intertic - animatetic);
			if (animatetimer <= 16)
			{
				xoffset1 = -(animatetimer      * scradjust);
				xoffset2 = -((animatetimer- 2) * scradjust);
				xoffset3 = -((animatetimer- 4) * scradjust);
				xoffset4 = -((animatetimer- 6) * scradjust);
				xoffset5 = -((animatetimer- 8) * scradjust);
				xoffset6 = -((animatetimer-10) * scradjust);
				if (xoffset2 > 0) xoffset2 = 0;
				if (xoffset3 > 0) xoffset3 = 0;
				if (xoffset4 > 0) xoffset4 = 0;
				if (xoffset5 > 0) xoffset5 = 0;
				if (xoffset6 > 0) xoffset6 = 0;
			}
			else if (animatetimer < 34)
			{
				drawsection = 1;
				xoffset1 = (24-animatetimer) * scradjust;
				xoffset2 = (26-animatetimer) * scradjust;
				xoffset3 = (28-animatetimer) * scradjust;
				xoffset4 = (30-animatetimer) * scradjust;
				xoffset5 = (32-animatetimer) * scradjust;
				xoffset6 = (34-animatetimer) * scradjust;
				if (xoffset1 < 0) xoffset1 = 0;
				if (xoffset2 < 0) xoffset2 = 0;
				if (xoffset3 < 0) xoffset3 = 0;
				if (xoffset4 < 0) xoffset4 = 0;
				if (xoffset5 < 0) xoffset5 = 0;
			}
			else
			{
				drawsection = 1;
				if (animatetimer == 32)
					S_StartSound(NULL, sfx_s3k68);
			}
		}

		if (drawsection == 1)
		{
			const char *ringtext = "\x82" "50 rings, no shield";
			const char *tut1text = "\x82" "press " "\x80" "spin";
			const char *tut2text = "\x82" "mid-" "\x80" "jump";
			ttheight = 8;
			V_DrawLevelTitle(data.spec.passedx1 + xoffset1, ttheight, 0, data.spec.passed1);
			ttheight += V_LevelNameHeight(data.spec.passed3) + 2;
			V_DrawLevelTitle(data.spec.passedx3 + xoffset2, ttheight, 0, data.spec.passed3);
			ttheight += V_LevelNameHeight(data.spec.passed4) + 2;
			V_DrawLevelTitle(data.spec.passedx4 + xoffset3, ttheight, 0, data.spec.passed4);

			ttheight = 108;
			V_DrawLevelTitle(BASEVIDWIDTH/2 + xoffset4 - (V_LevelNameWidth(ringtext)/2), ttheight, 0, ringtext);
			ttheight += V_LevelNameHeight(tut1text) + 2;
			V_DrawLevelTitle(BASEVIDWIDTH/2 + xoffset5 - (V_LevelNameWidth(tut1text)/2), ttheight, 0, tut1text);
			ttheight += V_LevelNameHeight(tut2text) + 2;
			V_DrawLevelTitle(BASEVIDWIDTH/2 + xoffset6 - (V_LevelNameWidth(tut2text)/2), ttheight, 0, tut2text);
		}
		else
		{
			INT32 yoffset = 0;
			if (data.spec.passed1[0] != '\0')
			{
				ttheight = 24;
				V_DrawLevelTitle(data.spec.passedx1 + xoffset1, ttheight, 0, data.spec.passed1);
				ttheight += V_LevelNameHeight(data.spec.passed2) + 2;
				V_DrawLevelTitle(data.spec.passedx2 + xoffset2, ttheight, 0, data.spec.passed2);
			}
			else
			{
				ttheight = 24 + (V_LevelNameHeight(data.spec.passed2)/2) + 2;
				V_DrawLevelTitle(data.spec.passedx2 + xoffset1, ttheight, 0, data.spec.passed2);
			}

			V_DrawScaledPatch(152 + xoffset3, 108, 0, data.spec.bonuspatches[0]);
			V_DrawTallNum(BASEVIDWIDTH + xoffset3 - 68, 109, 0, data.spec.bonuses[0].points);
			if (data.spec.bonuses[1].display)
			{
				V_DrawScaledPatch(152 + xoffset4, 124, 0, data.spec.bonuspatches[1]);
				V_DrawTallNum(BASEVIDWIDTH + xoffset4 - 68, 125, 0, data.spec.bonuses[1].points);
				yoffset = 16;
				// hack; pass the buck along...
				xoffset4 = xoffset5;
				xoffset5 = xoffset6;
			}
			V_DrawScaledPatch(152 + xoffset4, 124+yoffset, 0, data.spec.pscore);
			V_DrawTallNum(BASEVIDWIDTH + xoffset4 - 68, 125+yoffset, 0, data.spec.score);

			// Draw continues!
			if (continuesInSession /* && (data.spec.continues & 0x80) */) // Always draw when continues are a thing
			{
				UINT8 continues = data.spec.continues & 0x7F;

				V_DrawScaledPatch(152 + xoffset5, 150+yoffset, 0, data.spec.pcontinues);
				if (continues > 5)
				{
					INT32 leftx = (continues >= 10) ? 216 : 224;
					V_DrawContinueIcon(leftx + xoffset5, 162+yoffset, 0, *data.spec.playerchar, *data.spec.playercolor);
					V_DrawScaledPatch(leftx + xoffset5 + 12, 160+yoffset, 0, stlivex);
					if (!((data.spec.continues & 0x80) && !(endtic < 0 || intertic%20 < 10)))
						V_DrawRightAlignedString(252 + xoffset5, 158+yoffset, 0,
							va("%d",(((data.spec.continues & 0x80) && (endtic < 0)) ? continues-1 : continues)));
				}
				else
				{
					for (i = 0; i < continues; ++i)
					{
						if ((data.spec.continues & 0x80) && i == continues-1 && (endtic < 0 || intertic%20 < 10))
							break;
						V_DrawContinueIcon(246 + xoffset5 - (i*20), 162+yoffset, 0, *data.spec.playerchar, *data.spec.playercolor);
					}
				}
			}
		}

		// draw the emeralds
		//if (intertic & 1)
		{
			boolean drawthistic = !(ALL7EMERALDS(emeralds) && (intertic & 1));
			INT32 emeraldx = 152 - 3*28;
			INT32 em = P_GetNextEmerald();

			if (em == 7)
			{
				if (!stagefailed)
				{
					fixed_t adjust = 2*(FINESINE(FixedAngle((intertic + 1)<<(FRACBITS-4)) & FINEMASK));
					V_DrawFixedPatch(152<<FRACBITS, (74<<FRACBITS) - adjust, FRACUNIT, 0, emeraldpics[0][em], NULL);
				}
			}
			else if (em < 7)
			{
				static UINT8 emeraldbounces = 0;
				static INT32 emeraldmomy = 20;
				static INT32 emeraldy = -40;

				if (drawthistic)
					for (i = 0; i < 7; ++i)
					{
						if ((i != em) && (emeralds & (1 << i)))
							V_DrawScaledPatch(emeraldx, 74, 0, emeraldpics[0][i]);
						emeraldx += 28;
					}

				emeraldx = 152 + (em-3)*28;

				if (intertic <= 1)
				{
					emeraldbounces = 0;
					emeraldmomy = 20;
					emeraldy = -40;
				}
				else
				{
					if (!stagefailed)
					{
						if (emeraldbounces < 3)
						{
							emeraldy += (++emeraldmomy);
							if (emeraldy > 74)
							{
								S_StartSound(NULL, sfx_tink); // tink
								emeraldbounces++;
								emeraldmomy = -(emeraldmomy/2);
								emeraldy = 74;
							}
						}
					}
					else
					{
						if (emeraldy < (vid.height/vid.dupy)+16)
						{
							emeraldy += (++emeraldmomy);
							emeraldx += intertic - 6;
						}
						if (emeraldbounces < 1 && emeraldy > 74)
						{
							S_StartSound(NULL, sfx_shldls); // nope
							emeraldbounces++;
							emeraldmomy = -(emeraldmomy/2);
							emeraldy = 74;
						}
					}
					if (drawthistic)
						V_DrawScaledPatch(emeraldx, emeraldy, 0, emeraldpics[0][em]);
				}
			}
		}
	}
	else if (intertype == int_match || intertype == int_race)
	{
		INT32 j = 0;
		INT32 x = 4;
		INT32 y = 48;
		char name[MAXPLAYERNAME+1];
		char strtime[10];

		// draw the header
		if (safetorender)
			V_DrawScaledPatch(112, 2, 0, data.match.result);
>>>>>>> srb2/next

		// draw the level name
		V_DrawCenteredString(-4 + x + BASEVIDWIDTH/2, 12, 0, data.match.levelstring);
		V_DrawFill((x-3) - duptweak, 34, dupadjust-2, 1, 0);

		if (data.match.encore)
			V_DrawCenteredString(-4 + x + BASEVIDWIDTH/2, 12-8, hilicol, "ENCORE MODE");

		if (data.match.numplayers > NUMFORNEWCOLUMN)
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

		for (i = 0; i < data.match.numplayers; i++)
		{
			boolean dojitter = data.match.jitter[data.match.num[i]];
			data.match.jitter[data.match.num[i]] = 0;

			if (data.match.num[i] != MAXPLAYERS && playeringame[data.match.num[i]] && !players[data.match.num[i]].spectator)
			{
				char strtime[MAXPLAYERNAME+1];

				if (dojitter)
					y--;

				V_DrawCenteredString(x+6, y, 0, va("%d", data.match.pos[i]));

				if (data.match.color[i])
				{
					UINT8 *colormap = R_GetTranslationColormap(*data.match.character[i], *data.match.color[i], GTC_CACHE);
					V_DrawMappedPatch(x+16, y-4, 0, facerankprefix[*data.match.character[i]], colormap);
				}

				if (data.match.num[i] == whiteplayer)
				{
					UINT8 cursorframe = (intertic / 4) % 8;
					V_DrawScaledPatch(x+16, y-4, 0, W_CachePatchName(va("K_CHILI%d", cursorframe+1), PU_CACHE));
				}

				STRBUFCPY(strtime, data.match.name[i]);

				y2 = y;

				if (netgame && playerconsole[data.match.num[i]] == 0 && server_lagless && !players[data.match.num[i]].bot)
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

				if (data.match.numplayers > NUMFORNEWCOLUMN)
					V_DrawThinString(x+36, y2-1, ((data.match.num[i] == whiteplayer) ? hilicol : 0)|V_ALLOWLOWERCASE|V_6WIDTHSPACE, strtime);
				else
					V_DrawString(x+36, y2, ((data.match.num[i] == whiteplayer) ? hilicol : 0)|V_ALLOWLOWERCASE, strtime);

				if (data.match.rankingsmode)
				{
					if (powertype != -1 && !clientpowerlevels[data.match.num[i]][powertype]) // No power level (splitscreen guests)
						STRBUFCPY(strtime, "----");
					else
					{
<<<<<<< HEAD
						if (data.match.increase[data.match.num[i]] != INT16_MIN)
						{
							snprintf(strtime, sizeof strtime, "(%d)", data.match.increase[data.match.num[i]]);

							if (data.match.numplayers > NUMFORNEWCOLUMN)
								V_DrawRightAlignedThinString(x+133+gutter, y-1, V_6WIDTHSPACE, strtime);
							else
								V_DrawRightAlignedString(x+118+gutter, y, 0, strtime);
						}
=======
						if (players[data.match.num[i]].pflags & PF_GAMETYPEOVER)
							snprintf(strtime, sizeof strtime, "DNF");
						else
							snprintf(strtime, sizeof strtime,
								"%i:%02i.%02i",
								G_TicsToMinutes(data.match.scores[i], true),
								G_TicsToSeconds(data.match.scores[i]), G_TicsToCentiseconds(data.match.scores[i]));
>>>>>>> srb2/next

						snprintf(strtime, sizeof strtime, "%d", data.match.val[i]);
					}

					if (data.match.numplayers > NUMFORNEWCOLUMN)
						V_DrawRightAlignedThinString(x+152+gutter, y-1, V_6WIDTHSPACE, strtime);
					else
						V_DrawRightAlignedString(x+152+gutter, y, 0, strtime);
				}
				else
				{
					if (data.match.val[i] == (UINT32_MAX-1))
						V_DrawRightAlignedThinString(x+152+gutter, y-1, (data.match.numplayers > NUMFORNEWCOLUMN ? V_6WIDTHSPACE : 0), "NO CONTEST.");
					else
					{
<<<<<<< HEAD
						if (intertype == int_race || (intertype == int_match && battlecapsules))
						{
							snprintf(strtime, sizeof strtime, "%i'%02i\"%02i", G_TicsToMinutes(data.match.val[i], true),
							G_TicsToSeconds(data.match.val[i]), G_TicsToCentiseconds(data.match.val[i]));
							strtime[sizeof strtime - 1] = '\0';

							if (data.match.numplayers > NUMFORNEWCOLUMN)
								V_DrawRightAlignedThinString(x+152+gutter, y-1, V_6WIDTHSPACE, strtime);
							else
								V_DrawRightAlignedString(x+152+gutter, y, 0, strtime);
						}
=======
						if (players[data.match.num[i]].pflags & PF_GAMETYPEOVER)
							snprintf(strtime, sizeof strtime, "DNF");
>>>>>>> srb2/next
						else
						{
							if (data.match.numplayers > NUMFORNEWCOLUMN)
								V_DrawRightAlignedThinString(x+152+gutter, y-1, V_6WIDTHSPACE, va("%i", data.match.val[i]));
							else
								V_DrawRightAlignedString(x+152+gutter, y, 0, va("%i", data.match.val[i]));
						}
					}
				}

				if (dojitter)
					y++;
			}
			else
				data.match.num[i] = MAXPLAYERS; // this should be the only field setting in this function

			y += 18;

			if (i == NUMFORNEWCOLUMN-1)
			{
				y = 41;
				x += BASEVIDWIDTH/2;
			}
#undef NUMFORNEWCOLUMN
		}
	}

<<<<<<< HEAD
dotimer:
	if (timer)
=======
		V_DrawString(x+36, 32, V_YELLOWMAP, "NAME");
		V_DrawString(x+(BASEVIDWIDTH/2)+36, 32, V_YELLOWMAP, "NAME");

		V_DrawRightAlignedString(x+152, 32, V_YELLOWMAP, "SCORE");
		V_DrawRightAlignedString(x+(BASEVIDWIDTH/2)+152, 32, V_YELLOWMAP, "SCORE");

		for (i = 0; i < data.match.numplayers; i++)
		{
			if (playeringame[data.match.num[i]] && !(data.match.spectator[i]))
			{
				UINT8 *colormap = R_GetTranslationColormap(*data.match.character[i], *data.match.color[i], GTC_CACHE);

				if (*data.match.color[i] == SKINCOLOR_RED) //red
				{
					if (redplayers++ > 9)
						continue;
					x = 4 + (BASEVIDWIDTH/2);
					y = (redplayers * 16) + 32;
					V_DrawCenteredString(x+6, y, 0, va("%d", redplayers));
				}
				else if (*data.match.color[i] == SKINCOLOR_BLUE) //blue
				{
					if (blueplayers++ > 9)
						continue;
					x = 4;
					y = (blueplayers * 16) + 32;
					V_DrawCenteredString(x+6, y, 0, va("%d", blueplayers));
				}
				else
					continue;

				// Draw the back sprite, it looks ugly if we don't
				V_DrawSmallScaledPatch(x+16, y-4, 0, livesback);

				//color is ALWAYS going to be 6/7 here, no need to check if it's nonzero.
				V_DrawSmallMappedPatch(x+16, y-4, 0,faceprefix[*data.match.character[i]], colormap);

				strlcpy(name, data.match.name[i], 9);

				V_DrawString(x+36, y, V_ALLOWLOWERCASE, name);

				V_DrawRightAlignedString(x+152, y, 0, va("%u", data.match.scores[i]));
			}
		}
	}
	else if (intertype == int_comp)
>>>>>>> srb2/next
	{
		char *string;
		INT32 tickdown = (timer+1)/TICRATE;

		if (multiplayer && demo.playback)
			string = va("Replay ends in %d", tickdown);
		else
			string = va("%s starts in %d", cv_advancemap.string, tickdown);

		V_DrawCenteredString(BASEVIDWIDTH/2, 188, hilicol,
			string);
	}

	if ((demo.recording || demo.savemode == DSM_SAVED) && !demo.playback)
		switch (demo.savemode)
		{
		case DSM_NOTSAVING:
			V_DrawRightAlignedThinString(BASEVIDWIDTH - 2, 2, V_SNAPTOTOP|V_SNAPTORIGHT|V_ALLOWLOWERCASE|hilicol, "Look Backward: Save replay");
			break;

<<<<<<< HEAD
		case DSM_SAVED:
			V_DrawRightAlignedThinString(BASEVIDWIDTH - 2, 2, V_SNAPTOTOP|V_SNAPTORIGHT|V_ALLOWLOWERCASE|hilicol, "Replay saved!");
			break;
=======
				if (players[data.competition.num[i]].pflags & PF_GAMETYPEOVER)
					snprintf(sstrtime, sizeof sstrtime, "Time Over");
				else if (players[data.competition.num[i]].lives <= 0)
					snprintf(sstrtime, sizeof sstrtime, "Game Over");
				else
					snprintf(sstrtime, sizeof sstrtime, "%i:%02i.%02i", G_TicsToMinutes(ptime, true),
							G_TicsToSeconds(ptime), G_TicsToCentiseconds(ptime));

				sstrtime[sizeof sstrtime - 1] = '\0';
				// Time
				V_DrawRightAlignedThinString(x+160, y, ((data.competition.times[i] & 0x80000000) ? V_YELLOWMAP : 0), sstrtime);
				// Rings
				V_DrawRightAlignedThinString(x+188, y, V_MONOSPACE|((data.competition.rings[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pring));
				// Total rings
				V_DrawRightAlignedThinString(x+216, y, V_MONOSPACE|((data.competition.maxrings[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pmaxring));
				// Monitors
				V_DrawRightAlignedThinString(x+244, y, V_MONOSPACE|((data.competition.monitors[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pmonitor));
				// Score
				V_DrawRightAlignedThinString(x+288, y, V_MONOSPACE|((data.competition.scores[i] & 0x80000000) ? V_YELLOWMAP : 0), va("%u", pscore));
				// Final Points
				V_DrawRightAlignedString(x+312, y, V_YELLOWMAP, va("%d", data.competition.points[i]));
			}
>>>>>>> srb2/next

		case DSM_TITLEENTRY:
			ST_DrawDemoTitleEntry();
			break;

		default: // Don't render any text here
			break;
		}

<<<<<<< HEAD
	//if ((intertic/TICRATE) & 1) // Make it obvious that scrambling is happening next round. (OR NOT, I GUESS)
	//{
		/*if (cv_scrambleonchange.value && cv_teamscramble.value)
			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, hilicol, M_GetText("Teams will be scrambled next round!"));*/
		if (speedscramble != -1 && speedscramble != gamespeed)
			V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-24, hilicol|V_ALLOWLOWERCASE|V_SNAPTOBOTTOM,
				va(M_GetText("Next race will be %s Speed!"), kartspeed_cons_t[1+speedscramble].strvalue));
	//}
=======
skiptallydrawer:
	if (!LUA_HudEnabled(hud_intermissionmessages))
		return;

	if (timer)
		V_DrawCenteredString(BASEVIDWIDTH/2, 188, V_YELLOWMAP,
			va("start in %d seconds", timer/TICRATE));

	// Make it obvious that scrambling is happening next round.
	if (cv_scrambleonchange.value && cv_teamscramble.value && (intertic/TICRATE % 2 == 0))
		V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, V_YELLOWMAP, M_GetText("Teams will be scrambled next round!"));
>>>>>>> srb2/next
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
		if (demo.savemode == DSM_NOTSAVING && InputDown(gc_lookback, 1))
			demo.savemode = DSM_TITLEENTRY;

		if (demo.savemode == DSM_WILLSAVE || demo.savemode == DSM_WILLAUTOSAVE)
			G_SaveDemo();
	}

	// Check for pause or menu up in single player
	if (paused || P_AutoPause())
		return;

<<<<<<< HEAD
#ifdef HAVE_BLUA
	LUAh_IntermissionThinker();
#endif
=======
	LUAh_IntermissionThinker();
>>>>>>> srb2/next

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

	if (intertype == int_race || intertype == int_match)
	{
		if (netgame || multiplayer)
		{
<<<<<<< HEAD
			if (sorttic == -1)
				sorttic = intertic + max((cv_inttime.value/2)-2, 2)*TICRATE; // 8 second pause after match results
			else if (!(multiplayer && demo.playback)) // Don't advance to rankings in replays
			{
				if (!data.match.rankingsmode && (intertic >= sorttic + 8))
					Y_CalculateMatchData(1, Y_CompareRank);
=======
			if (mapheaderinfo[gamemap-1]->musinterfadeout
#ifdef _WIN32
				// can't fade midi due to win32 volume hack
				&& S_MusicType() != MU_MID
#endif
			)
				S_FadeOutStopMusic(mapheaderinfo[gamemap-1]->musinterfadeout);
			else if (mapheaderinfo[gamemap-1]->musintername[0] && S_MusicExists(mapheaderinfo[gamemap-1]->musintername, !midi_disabled, !digital_disabled))
				S_ChangeMusicInternal(mapheaderinfo[gamemap-1]->musintername, false); // don't loop it
			else
				S_ChangeMusicInternal("_clear", false); // don't loop it
			tallydonetic = -1;
		}
>>>>>>> srb2/next

				if (data.match.rankingsmode && intertic > sorttic+16+(2*TICRATE))
				{
					INT32 q=0,r=0;
					boolean kaching = true;

<<<<<<< HEAD
					for (q = 0; q < data.match.numplayers; q++)
					{
						if (data.match.num[q] == MAXPLAYERS
						|| !data.match.increase[data.match.num[q]]
						|| data.match.increase[data.match.num[q]] == INT16_MIN)
							continue;

						r++;
						data.match.jitter[data.match.num[q]] = 1;

						if (powertype != -1)
						{
							// Power Levels
							if (abs(data.match.increase[data.match.num[q]]) < 10)
							{
								// Not a lot of point increase left, just set to 0 instantly
								data.match.increase[data.match.num[q]] = 0;
							}
							else
							{
								SINT8 remove = 0; // default (should not happen)

								if (data.match.increase[data.match.num[q]] < 0)
									remove = -10;
								else if (data.match.increase[data.match.num[q]] > 0)
									remove = 10;

								// Remove 10 points at a time
								data.match.increase[data.match.num[q]] -= remove;

								// Still not zero, no kaching yet
								if (data.match.increase[data.match.num[q]] != 0)
									kaching = false;
							}
						}
						else
						{
							// Basic bitch points
							if (data.match.increase[data.match.num[q]])
							{
								if (--data.match.increase[data.match.num[q]])
									kaching = false;
							}
						}
					}
=======
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && (players[i].cmd.buttons & BT_USE))
				skip = true;

		// bonuses count down by 222 each tic
		for (i = 0; i < 4; ++i)
		{
			if (!data.coop.bonuses[i].points)
				continue;

			data.coop.bonuses[i].points -= 222;
			data.coop.total += 222;
			data.coop.score += 222;
			if (data.coop.bonuses[i].points < 0 || skip == true) // too far?
			{
				data.coop.score += data.coop.bonuses[i].points;
				data.coop.total += data.coop.bonuses[i].points;
				data.coop.bonuses[i].points = 0;
			}
			if (data.coop.score > MAXSCORE)
				data.coop.score = MAXSCORE;
			if (data.coop.bonuses[i].points > 0)
				anybonuses = true;
		}

		if (!anybonuses)
		{
			tallydonetic = intertic;
			endtic = intertic + 3*TICRATE; // 3 second pause after end of tally
			S_StartSound(NULL, (gottoken ? sfx_token : sfx_chchng)); // cha-ching!

			// Update when done with tally
			if ((!modifiedgame || savemoddata) && !(netgame || multiplayer) && !demoplayback)
			{
				if (M_UpdateUnlockablesAndExtraEmblems())
					S_StartSound(NULL, sfx_s3k68);

				G_SaveGameData();
			}
		}
		else if (!(intertic & 1))
			S_StartSound(NULL, sfx_ptally); // tally sound effect

		if (data.coop.gotlife > 0 && (skip == true || data.coop.score % 50000 < oldscore % 50000)) // just passed a 50000 point mark
		{
			// lives are already added since tally is fake, but play the music
			P_PlayLivesJingle(NULL);
			--data.coop.gotlife;
		}
	}
	else if (intertype == int_spec) // coop or single player, special stage
	{
		INT32 i;
		UINT32 oldscore = data.spec.score;
		boolean skip = false, super = false, anybonuses = false;

		if (!intertic) // first time only
		{
			if (mapheaderinfo[gamemap-1]->musinterfadeout
#ifdef _WIN32
				// can't fade midi due to win32 volume hack
				&& S_MusicType() != MU_MID
#endif
			)
				S_FadeOutStopMusic(mapheaderinfo[gamemap-1]->musinterfadeout);
			else if (mapheaderinfo[gamemap-1]->musintername[0] && S_MusicExists(mapheaderinfo[gamemap-1]->musintername, !midi_disabled, !digital_disabled))
				S_ChangeMusicInternal(mapheaderinfo[gamemap-1]->musintername, false); // don't loop it
			else
				S_ChangeMusicInternal("_clear", false); // don't loop it
			tallydonetic = -1;
		}

		if (intertic < 2*TICRATE) // TWO second pause before tally begins, thank you mazmazz
			return;

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
			{
				if (players[i].cmd.buttons & BT_USE)
					skip = true;
				if (players[i].charflags & SF_SUPER)
					super = true;
			}

		if (tallydonetic != -1 && ((data.spec.continues & 0x80) || (super && ALL7EMERALDS(emeralds))))
		{
			if ((intertic - tallydonetic) > (3*TICRATE)/2)
			{
				endtic = intertic + 4*TICRATE; // 4 second pause after end of tally
				if (data.spec.continues & 0x80)
					S_StartSound(NULL, sfx_s3kac); // bingly-bingly-bing!

			}
			return;
		}

		// bonuses count down by 222 each tic
		for (i = 0; i < 2; ++i)
		{
			if (!data.spec.bonuses[i].points)
				continue;

			data.spec.bonuses[i].points -= 222;
			data.spec.score += 222;
			if (data.spec.bonuses[i].points < 0 || skip == true) // too far?
			{
				data.spec.score += data.spec.bonuses[i].points;
				data.spec.bonuses[i].points = 0;
			}
			if (data.spec.score > MAXSCORE)
				data.spec.score = MAXSCORE;
			if (data.spec.bonuses[i].points > 0)
				anybonuses = true;
		}

		if (!anybonuses)
		{
			tallydonetic = intertic;
			if (!((data.spec.continues & 0x80) || (super && ALL7EMERALDS(emeralds)))) // don't set endtic yet!
				endtic = intertic + 4*TICRATE; // 4 second pause after end of tally

			S_StartSound(NULL, (gottoken ? sfx_token : sfx_chchng)); // cha-ching!

			// Update when done with tally
			if ((!modifiedgame || savemoddata) && !(netgame || multiplayer) && !demoplayback)
			{
				if (M_UpdateUnlockablesAndExtraEmblems())
					S_StartSound(NULL, sfx_s3k68);
>>>>>>> srb2/next

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
<<<<<<< HEAD
		else
			endtic = intertic + 8*TICRATE; // 8 second pause after end of tally
=======
		else if (!(intertic & 1))
			S_StartSound(NULL, sfx_ptally); // tally sound effect

		if (data.spec.gotlife > 0 && (skip == true || data.spec.score % 50000 < oldscore % 50000)) // just passed a 50000 point mark
		{
			// lives are already added since tally is fake, but play the music
			P_PlayLivesJingle(NULL);
			--data.spec.gotlife;
		}
	}
	else if (intertype == int_match || intertype == int_ctf || intertype == int_teammatch) // match
	{
		if (!intertic) // first time only
			S_ChangeMusicInternal("_inter", true); // loop it

		// If a player has left or joined, recalculate scores.
		if (data.match.numplayers != D_NumPlayers())
			Y_CalculateMatchWinners();
	}
	else if (intertype == int_race || intertype == int_comp) // race
	{
		if (!intertic) // first time only
			S_ChangeMusicInternal("_inter", true); // loop it

		// Don't bother recalcing for race. It doesn't make as much sense.
>>>>>>> srb2/next
	}
}

//
<<<<<<< HEAD
// Y_UpdateRecordReplays
//
// Update replay files/data, etc. for Record Attack
// See G_SetNightsRecords for NiGHTS Attack.
//
static void Y_UpdateRecordReplays(void)
{
	const size_t glen = strlen(srb2home)+1+strlen("media")+strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
	char *gpath;
	char lastdemo[256], bestdemo[256];
	UINT8 earnedEmblems;

	// Record new best time
	if (!mainrecords[gamemap-1])
		G_AllocMainRecordData(gamemap-1);

	if (players[consoleplayer].pflags & PF_TIMEOVER)
	{
		players[consoleplayer].realtime = UINT32_MAX;
	}

	if (((mainrecords[gamemap-1]->time == 0) || (players[consoleplayer].realtime < mainrecords[gamemap-1]->time))
		&& (players[consoleplayer].realtime < UINT32_MAX)) // DNF
	{
		mainrecords[gamemap-1]->time = players[consoleplayer].realtime;
	}

	if (modeattacking == ATTACKING_RECORD)
	{
		if ((mainrecords[gamemap-1]->lap == 0) || (bestlap < mainrecords[gamemap-1]->lap))
			mainrecords[gamemap-1]->lap = bestlap;
	}
	else
	{
		mainrecords[gamemap-1]->lap = 0;
	}

	// Save demo!
	bestdemo[255] = '\0';
	lastdemo[255] = '\0';
	G_SetDemoTime(players[consoleplayer].realtime, bestlap);
	G_CheckDemoStatus();

	gpath = va("%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s",
			srb2home, timeattackfolder);
	M_MkdirEach(gpath, M_PathParts(gpath) - 3, 0755);

	if ((gpath = malloc(glen)) == NULL)
		I_Error("Out of memory for replay filepath\n");

	sprintf(gpath,"%s"PATHSEP"media"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(gamemap));
	snprintf(lastdemo, 255, "%s-%s-last.lmp", gpath, cv_chooseskin.string);

	if (FIL_FileExists(lastdemo))
	{
		UINT8 *buf;
		size_t len = FIL_ReadFile(lastdemo, &buf);

		snprintf(bestdemo, 255, "%s-%s-time-best.lmp", gpath, cv_chooseskin.string);
		if (!FIL_FileExists(bestdemo) || G_CmpDemoTime(bestdemo, lastdemo) & 1)
		{ // Better time, save this demo.
			if (FIL_FileExists(bestdemo))
				remove(bestdemo);
			FIL_WriteFile(bestdemo, buf, len);
			CONS_Printf("\x83%s\x80 %s '%s'\n", M_GetText("NEW RECORD TIME!"), M_GetText("Saved replay as"), bestdemo);
		}

		if (modeattacking == ATTACKING_RECORD)
		{
			snprintf(bestdemo, 255, "%s-%s-lap-best.lmp", gpath, cv_chooseskin.string);
			if (!FIL_FileExists(bestdemo) || G_CmpDemoTime(bestdemo, lastdemo) & (1<<1))
			{ // Better lap time, save this demo.
				if (FIL_FileExists(bestdemo))
					remove(bestdemo);
				FIL_WriteFile(bestdemo, buf, len);
				CONS_Printf("\x83%s\x80 %s '%s'\n", M_GetText("NEW RECORD LAP!"), M_GetText("Saved replay as"), bestdemo);
			}
		}

		//CONS_Printf("%s '%s'\n", M_GetText("Saved replay as"), lastdemo);

		Z_Free(buf);
	}
	free(gpath);

	// Check emblems when level data is updated
	if ((earnedEmblems = M_CheckLevelEmblems()))
		CONS_Printf(M_GetText("\x82" "Earned %hu medal%s for Record Attack records.\n"), (UINT16)earnedEmblems, earnedEmblems > 1 ? "s" : "");

	if (M_UpdateUnlockablesAndExtraEmblems(false))
		S_StartSound(NULL, sfx_ncitem);

	// SRB2Kart - save here so you NEVER lose your earned times/medals.
	G_SaveGameData(false);

	// Update timeattack menu's replay availability.
	CV_AddValue(&cv_nextmap, 1);
	CV_AddValue(&cv_nextmap, -1);
}

static void K_UpdatePowerLevels(void)
=======
// Y_StartIntermission
//
// Called by G_DoCompleted. Sets up data for intermission drawer/ticker.
//
void Y_StartIntermission(void)
>>>>>>> srb2/next
{
	INT32 i, j;
	INT32 numplayersingame = 0, numgriefers = 0;
	INT16 increment[MAXPLAYERS];

	// Compare every single player against each other for power level increases.
	// Every player you won against gives you more points, and vice versa.
	// The amount of points won per match-up depends on the difference between the loser's power and the winner's power.
	// See K_CalculatePowerLevelInc for more info.

<<<<<<< HEAD
	for (i = 0; i < MAXPLAYERS; i++)
=======
	safetorender = true;

	if (!multiplayer)
>>>>>>> srb2/next
	{
		increment[i] = 0;

<<<<<<< HEAD
		if (nospectategrief[i] != -1)
			numgriefers++;
=======
		intertype = (G_IsSpecialStage(gamemap)) ? int_spec : int_coop;
	}
	else
	{
		if (cv_inttime.value == 0 && gametype == GT_COOP)
			timer = 0;
		else
		{
			timer = cv_inttime.value*TICRATE;
>>>>>>> srb2/next

		if (!playeringame[i] || players[i].spectator)
			continue;

<<<<<<< HEAD
		numplayersingame++;
=======
		if (intermissiontypes[gametype] != int_none)
			intertype = intermissiontypes[gametype];
		else if (gametype == GT_COOP)
			intertype = (G_IsSpecialStage(gamemap)) ? int_spec : int_coop;
		else if (gametype == GT_TEAMMATCH)
			intertype = int_teammatch;
		else if (gametype == GT_MATCH
		 || gametype == GT_TAG
		 || gametype == GT_HIDEANDSEEK)
			intertype = int_match;
		else if (gametype == GT_RACE)
			intertype = int_race;
		else if (gametype == GT_COMPETITION)
			intertype = int_comp;
		else if (gametype == GT_CTF)
			intertype = int_ctf;
>>>>>>> srb2/next
	}

	for (i = 0; i < numplayersingame; i++)
	{
		UINT16 yourpower = PWRLVRECORD_DEF;
		UINT16 theirpower = PWRLVRECORD_DEF;
		INT16 diff = 0; // Loser PWR.LV - Winner PWR.LV
		INT16 inc = 0; // Total pt increment
		UINT8 ipnum = data.match.num[i];
		UINT8 jpnum;

		CONS_Debug(DBG_GAMELOGIC, "Power Level Gain for player %d:\n", ipnum);

<<<<<<< HEAD
		if (clientpowerlevels[ipnum][powertype] == 0) // splitscreen guests don't record power level changes
			continue;
		yourpower = clientpowerlevels[ipnum][powertype];

		CONS_Debug(DBG_GAMELOGIC, "Player %d's PWR.LV: %d\n", ipnum, yourpower);

		for (j = 0; j < numplayersingame; j++)
=======
	switch (intertype)
	{
		case int_coop: // coop or single player, normal level
>>>>>>> srb2/next
		{
			boolean won = false;

			jpnum = data.match.num[j];

			if (i == j || ipnum == jpnum) // Same person
				continue;

			CONS_Debug(DBG_GAMELOGIC, "Player %d VS Player %d:\n", ipnum, jpnum);

<<<<<<< HEAD
			if (data.match.val[i] == data.match.val[j]) // Tie -- neither get any points for this match up.
			{
				CONS_Debug(DBG_GAMELOGIC, "TIE, no change.\n");
				continue;
			}

			if (clientpowerlevels[jpnum][powertype] == 0) // No power level (splitscreen guests, bots)
				continue;

			theirpower = clientpowerlevels[jpnum][powertype];

			CONS_Debug(DBG_GAMELOGIC, "Player %d's PWR.LV: %d\n", jpnum, theirpower);

			if (G_RaceGametype())
			{
				if (data.match.val[i] < data.match.val[j])
					won = true;
=======
			for (i = 0; i < 4; ++i)
				data.coop.bonuspatches[i] = W_CachePatchName(data.coop.bonuses[i].patch, PU_PATCH);
			data.coop.ptotal = W_CachePatchName("YB_TOTAL", PU_PATCH);

			// get act number
			data.coop.actnum = mapheaderinfo[gamemap-1]->actnum;

			// get background patches
			widebgpatch = W_CachePatchName("INTERSCW", PU_PATCH);
			bgpatch = W_CachePatchName("INTERSCR", PU_PATCH);

			// grab an interscreen if appropriate
			if (mapheaderinfo[gamemap-1]->interscreen[0] != '#')
			{
				interpic = W_CachePatchName(mapheaderinfo[gamemap-1]->interscreen, PU_PATCH);
				useinterpic = true;
				usebuffer = false;
			}
			else
			{
				useinterpic = false;
#ifdef HWRENDER
				if (rendermode == render_opengl)
					usebuffer = true; // This needs to be here for OpenGL, otherwise usebuffer is never set to true for it, and thus there's no screenshot in the intermission
#endif
			}
			usetile = false;

			// set up the "got through act" message according to skin name
			// too long so just show "YOU GOT THROUGH THE ACT"
			if (strlen(skins[players[consoleplayer].skin].realname) > 13)
			{
				strcpy(data.coop.passed1, "you got");
				strcpy(data.coop.passed2, (mapheaderinfo[gamemap-1]->actnum) ? "through act" : "through the act");
			}
			// long enough that "X GOT" won't fit so use "X PASSED THE ACT"
			else if (strlen(skins[players[consoleplayer].skin].realname) > 8)
			{
				strcpy(data.coop.passed1, skins[players[consoleplayer].skin].realname);
				strcpy(data.coop.passed2, (mapheaderinfo[gamemap-1]->actnum) ? "passed act" : "passed the act");
>>>>>>> srb2/next
			}
			else
			{
<<<<<<< HEAD
				if (data.match.val[i] > data.match.val[j])
					won = true;
=======
				snprintf(data.coop.passed1, sizeof data.coop.passed1, "%s got",
					skins[players[consoleplayer].skin].realname);
				strcpy(data.coop.passed2, (mapheaderinfo[gamemap-1]->actnum) ? "through act" : "through the act");
>>>>>>> srb2/next
			}

			if (won) // This player won!
			{
				diff = theirpower - yourpower;
				inc += K_CalculatePowerLevelInc(diff);
				CONS_Debug(DBG_GAMELOGIC, "WON! Diff is %d, total increment is %d\n", diff, inc);
			}
			else // This player lost...
			{
				diff = yourpower - theirpower;
				inc -= K_CalculatePowerLevelInc(diff);
				CONS_Debug(DBG_GAMELOGIC, "LOST... Diff is %d, total increment is %d\n", diff, inc);
			}
		}

<<<<<<< HEAD
		if (numgriefers != 0) // Automatic win against quitters.
		{
			for (jpnum = 0; jpnum < MAXPLAYERS; jpnum++)
			{
				if (nospectategrief[jpnum] == -1) // Empty slot
					continue;

				if (ipnum == jpnum) // Same person
					continue;

				CONS_Debug(DBG_GAMELOGIC, "Player %d VS Player %d (griefer):\n", ipnum, jpnum);

				if (nospectategrief[jpnum] == 0) // No power level (splitscreen guests, bots)
					continue;

				theirpower = nospectategrief[jpnum];

				CONS_Debug(DBG_GAMELOGIC, "Player %d's PWR.LV: %d\n", jpnum, theirpower);

				diff = theirpower - yourpower;
				inc += K_CalculatePowerLevelInc(diff);
				CONS_Debug(DBG_GAMELOGIC, "AUTO-WON! Diff is %d, total increment is %d\n", diff, inc);
=======
		case int_spec: // coop or single player, special stage
		{
			// give out ring bonuses
			Y_AwardSpecialStageBonus();

			for (i = 0; i < 2; ++i)
				data.spec.bonuspatches[i] = W_CachePatchName(data.spec.bonuses[i].patch, PU_PATCH);

			data.spec.pscore = W_CachePatchName("YB_SCORE", PU_PATCH);
			data.spec.pcontinues = W_CachePatchName("YB_CONTI", PU_PATCH);

			// get background tile
			bgtile = W_CachePatchName("SPECTILE", PU_PATCH);

			// grab an interscreen if appropriate
			if (mapheaderinfo[gamemap-1]->interscreen[0] != '#')
			{
				interpic = W_CachePatchName(mapheaderinfo[gamemap-1]->interscreen, PU_PATCH);
				useinterpic = true;
>>>>>>> srb2/next
			}
		}

		if (inc == 0)
		{
			data.match.increase[ipnum] = INT16_MIN;
			CONS_Debug(DBG_GAMELOGIC, "Total Result: No increment, no change.\n");
			continue;
		}

<<<<<<< HEAD
		if (yourpower + inc > PWRLVRECORD_MAX)
			inc -= ((yourpower + inc) - PWRLVRECORD_MAX);
		if (yourpower + inc < PWRLVRECORD_MIN)
			inc -= ((yourpower + inc) - PWRLVRECORD_MIN);
=======
			// get special stage specific patches
/*			if (!stagefailed && ALL7EMERALDS(emeralds))
			{
				data.spec.cemerald = W_CachePatchName("GOTEMALL", PU_PATCH);
				data.spec.headx = 70;
				data.spec.nowsuper = players[consoleplayer].skin
					? NULL : W_CachePatchName("NOWSUPER", PU_PATCH);
			}
			else
			{
				data.spec.cemerald = W_CachePatchName("CEMERALD", PU_PATCH);
				data.spec.headx = 48;
				data.spec.nowsuper = NULL;
			} */
>>>>>>> srb2/next

		CONS_Debug(DBG_GAMELOGIC, "Total Result: Increment of %d.\n", inc);
		increment[ipnum] = inc;
	}

<<<<<<< HEAD
	CONS_Debug(DBG_GAMELOGIC, "Setting final power levels...\n");
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (increment[i] == 0)
			continue;

		data.match.increase[i] = increment[i];
		clientpowerlevels[i][powertype] += data.match.increase[i];
=======
			// set up the "got through act" message according to skin name
			if (stagefailed)
			{
				strcpy(data.spec.passed2, "Special Stage");
				data.spec.passed1[0] = '\0';
			}
			else if (ALL7EMERALDS(emeralds))
			{
				snprintf(data.spec.passed1,
					sizeof data.spec.passed1, "%s",
					skins[players[consoleplayer].skin].realname);
				data.spec.passed1[sizeof data.spec.passed1 - 1] = '\0';
				strcpy(data.spec.passed2, "got them all!");

				if (players[consoleplayer].charflags & SF_SUPER)
				{
					strcpy(data.spec.passed3, "can now become");
					snprintf(data.spec.passed4,
						sizeof data.spec.passed4, "Super %s",
						skins[players[consoleplayer].skin].realname);
					data.spec.passed4[sizeof data.spec.passed4 - 1] = '\0';
				}
			}
			else
			{
				if (strlen(skins[players[consoleplayer].skin].realname) <= SKINNAMESIZE-5)
				{
					snprintf(data.spec.passed1,
						sizeof data.spec.passed1, "%s got",
						skins[players[consoleplayer].skin].realname);
					data.spec.passed1[sizeof data.spec.passed1 - 1] = '\0';
				}
				else
					strcpy(data.spec.passed1, "You got");
				strcpy(data.spec.passed2, "a Chaos Emerald");
				if (P_GetNextEmerald() > 6)
				{
					data.spec.passed2[15] = '?';
					data.spec.passed2[16] = '\0';
				}
			}
			data.spec.passedx1 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed1))/2;
			data.spec.passedx2 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed2))/2;
			data.spec.passedx3 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed3))/2;
			data.spec.passedx4 = (BASEVIDWIDTH - V_LevelNameWidth(data.spec.passed4))/2;
			break;
		}
>>>>>>> srb2/next

		if (i == consoleplayer)
		{
			CONS_Debug(DBG_GAMELOGIC, "Player %d is you! Saving...\n", i);
			vspowerlevel[powertype] = clientpowerlevels[i][powertype];
			if (M_UpdateUnlockablesAndExtraEmblems(true))
				S_StartSound(NULL, sfx_ncitem);
			G_SaveGameData(true);
		}
	}
}

//
// Y_StartIntermission
//
// Called by G_DoCompleted. Sets up data for intermission drawer/ticker.
//
void Y_StartIntermission(void)
{
	intertic = -1;

#ifdef PARANOIA
	if (endtic != -1)
		I_Error("endtic is dirty");
#endif

	// set player Power Level type
	powertype = PWRLV_DISABLED;

<<<<<<< HEAD
	if (netgame && cv_kartusepwrlv.value)
	{
		if (G_RaceGametype())
			powertype = PWRLV_RACE;
		else if (G_BattleGametype())
			powertype = PWRLV_BATTLE;
	}

	if (!multiplayer)
	{
		timer = 0;
=======
			// get RESULT header
			data.match.result =
				W_CachePatchName("RESULT", PU_PATCH);

			bgtile = W_CachePatchName("SRB2BACK", PU_PATCH);
			usetile = true;
			useinterpic = false;
			break;
		}
>>>>>>> srb2/next

		if (!majormods && !multiplayer && !demo.playback) // move this once we have a proper time attack screen
		{
			// Update visitation flags
			mapvisited[gamemap-1] |= MV_BEATEN;
			if (ALL7EMERALDS(emeralds))
				mapvisited[gamemap-1] |= MV_ALLEMERALDS;
			/*if (ultimatemode)
				mapvisited[gamemap-1] |= MV_ULTIMATE;
			if (data.coop.gotperfbonus)
				mapvisited[gamemap-1] |= MV_PERFECT;*/

			if (modeattacking)
				Y_UpdateRecordReplays();
		}
	}
	else
	{
		if (cv_inttime.value == 0 && gametype == GT_COOP)
			timer = 0;
		else if (demo.playback) // Override inttime (which is pulled from the replay anyway
			timer = 10*TICRATE;
		else
		{
			timer = cv_inttime.value*TICRATE;

			if (!timer)
				timer = 1;
		}
	}

	if (gametype == GT_MATCH)
		intertype = int_match;
	else //if (gametype == GT_RACE)
		intertype = int_race;

<<<<<<< HEAD
	// We couldn't display the intermission even if we wanted to.
	// But we still need to give the players their score bonuses, dummy.
	//if (dedicated) return;

	// This should always exist, but just in case...
	if(!mapheaderinfo[prevmap])
		P_AllocMapHeader(prevmap);
=======
			// get RESULT header
			data.match.result = W_CachePatchName("RESULT", PU_PATCH);

			bgtile = W_CachePatchName("SRB2BACK", PU_PATCH);
			usetile = true;
			useinterpic = false;
			break;
		}
>>>>>>> srb2/next

	switch (intertype)
	{
		case int_match:
		{
			// Calculate who won
			if (battlecapsules)
			{
				Y_CalculateMatchData(0, Y_CompareTime);
			}
			else
			{
				Y_CalculateMatchData(0, Y_CompareScore);
			}

<<<<<<< HEAD
			if (cv_inttime.value > 0)
				S_ChangeMusicInternal("racent", true); // loop it

			break;
		}
		case int_race: // (time-only race)
		{
			// Calculate who won
			Y_CalculateMatchData(0, Y_CompareTime);
=======
			bgtile = W_CachePatchName("SRB2BACK", PU_PATCH);
			usetile = true;
			useinterpic = false;
			break;
		}

		case int_comp: // classic (full race)
		{
			// find out who won
			Y_CalculateCompetitionWinners();

			// set up the levelstring
			if (mapheaderinfo[prevmap]->actnum)
				snprintf(data.competition.levelstring,
					sizeof data.competition.levelstring,
					"%.32s * %d *",
					mapheaderinfo[prevmap]->lvlttl, mapheaderinfo[prevmap]->actnum);
			else
				snprintf(data.competition.levelstring,
					sizeof data.competition.levelstring,
					"* %.32s *",
					mapheaderinfo[prevmap]->lvlttl);

			data.competition.levelstring[sizeof data.competition.levelstring - 1] = '\0';

			// get background tile
			bgtile = W_CachePatchName("SRB2BACK", PU_PATCH);
			usetile = true;
			useinterpic = false;
>>>>>>> srb2/next
			break;
		}

		case int_none:
		default:
			break;
	}

	if (powertype != PWRLV_DISABLED)
		K_UpdatePowerLevels();

	//if (intertype == int_race || intertype == int_match)
	{
		//bgtile = W_CachePatchName("SRB2BACK", PU_STATIC);
		usetile = useinterpic = false;
		usebuffer = true;
	}
}

// ======

//
// Y_EndIntermission
//
void Y_EndIntermission(void)
{
	Y_UnloadData();

	endtic = -1;
	sorttic = -1;
	intertype = int_none;
	usebuffer = false;
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

#define UNLOAD(x) Z_ChangeTag(x, PU_CACHE); x = NULL

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

	/*switch (intertype)
	{
		case int_coop:
			// unload the coop and single player patches
			UNLOAD(data.coop.ttlnum);
			UNLOAD(data.coop.bonuspatches[3]);
			UNLOAD(data.coop.bonuspatches[2]);
			UNLOAD(data.coop.bonuspatches[1]);
			UNLOAD(data.coop.bonuspatches[0]);
			UNLOAD(data.coop.ptotal);
			break;
		case int_spec:
			// unload the special stage patches
			//UNLOAD(data.spec.cemerald);
			//UNLOAD(data.spec.nowsuper);
			UNLOAD(data.spec.bonuspatch);
			UNLOAD(data.spec.pscore);
			UNLOAD(data.spec.pcontinues);
			break;
		case int_match:
		case int_race:
		default:
			//without this default,
			//int_none, int_tag, int_chaos, and int_classicrace
			//are not handled
			break;
	}*/
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
		angle_t rubyfloattime = (ANGLE_MAX/NEWTICRATE)*(votetic % NEWTICRATE);
		rubyheight = FINESINE(rubyfloattime>>ANGLETOFINESHIFT);
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

<<<<<<< HEAD
		if (i == 3)
		{
			str = "RANDOM";
			pic = randomlvl;
		}
		else
		{
			str = levelinfo[i].str;
			pic = levelinfo[i].pic;
		}
=======
		if ((players[i].pflags & PF_GAMETYPEOVER) || players[i].lives <= 0)
			players[i].rings = 0;

		times[i]    = players[i].realtime;
		rings[i]    = (UINT32)max(players[i].rings, 0);
		maxrings[i] = (UINT32)players[i].totalring;
		monitors[i] = (UINT32)players[i].numboxes;
		scores[i]   = (UINT32)min(players[i].score, MAXSCORE);
>>>>>>> srb2/next

		if (selected[i])
		{
			UINT8 sizeadd = selected[i];

			for (j = 0; j <= splitscreen; j++) // another loop for drawing the selection backgrounds in the right order, grumble grumble..
			{
				INT32 handy = y;
				UINT8 p;
				UINT8 *colormap;
				patch_t *thiscurs;

<<<<<<< HEAD
				if (voteclient.playerinfo[j].selection != i)
					continue;
=======
			if (max(players[i].rings, 0) >= max(players[j].rings, 0))
				points[i]++;
			else
				bestat[1] = false;
>>>>>>> srb2/next

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

					color = colortranslations[players[p].skincolor][7];
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
				pic = randomlvl;
			else
				pic = levelinfo[votes[i]].pic;

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
				V_DrawMappedPatch(x+24, y+9, V_SNAPTOLEFT, facerankprefix[players[i].skin], colormap);
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

<<<<<<< HEAD
	if (timer)
	{
		INT32 hilicol, tickdown = (timer+1)/TICRATE;
		if (cons_menuhighlight.value)
			hilicol = cons_menuhighlight.value;
		else if (gametype == GT_RACE)
			hilicol = V_SKYMAP;
		else //if (gametype == GT_MATCH)
			hilicol = V_REDMAP;
		V_DrawCenteredString(BASEVIDWIDTH/2, 188, hilicol,
			va("Vote ends in %d", tickdown));
	}
}

//
// Y_VoteStop
=======
//
// Y_SetTimeBonus
//
static void Y_SetTimeBonus(player_t *player, y_bonus_t *bstruct)
{
	INT32 secs, bonus;

	strncpy(bstruct->patch, "YB_TIME", sizeof(bstruct->patch));
	bstruct->display = true;

	// calculate time bonus
	secs = player->realtime / TICRATE;
	if      (secs <  30) /*   :30 */ bonus = 50000;
	else if (secs <  60) /*  1:00 */ bonus = 10000;
	else if (secs <  90) /*  1:30 */ bonus = 5000;
	else if (secs < 120) /*  2:00 */ bonus = 4000;
	else if (secs < 180) /*  3:00 */ bonus = 3000;
	else if (secs < 240) /*  4:00 */ bonus = 2000;
	else if (secs < 300) /*  5:00 */ bonus = 1000;
	else if (secs < 360) /*  6:00 */ bonus = 500;
	else if (secs < 420) /*  7:00 */ bonus = 400;
	else if (secs < 480) /*  8:00 */ bonus = 300;
	else if (secs < 540) /*  9:00 */ bonus = 200;
	else if (secs < 600) /* 10:00 */ bonus = 100;
	else  /* TIME TAKEN: TOO LONG */ bonus = 0;
	bstruct->points = bonus;
}

//
// Y_SetRingBonus
//
static void Y_SetRingBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_RING", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = max(0, (player->rings) * 100);
}

//
// Y_SetNightsBonus
//
static void Y_SetNightsBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_NIGHT", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = player->totalmarescore;
}

//
// Y_SetLapBonus
//
static void Y_SetLapBonus(player_t *player, y_bonus_t *bstruct)
{
	strncpy(bstruct->patch, "YB_LAP", sizeof(bstruct->patch));
	bstruct->display = true;
	bstruct->points = max(0, player->totalmarebonuslap * 1000);
}

>>>>>>> srb2/next
//
// Vote screen's selection stops moving
//
SINT8 deferredlevel = 0;
static void Y_VoteStops(SINT8 pick, SINT8 level)
{
	nextmap = votelevels[level][0];

	if (level == 4)
		S_StartSound(NULL, sfx_noooo2); // gasp
	else if (mapheaderinfo[nextmap] && (mapheaderinfo[nextmap]->menuflags & LF2_HIDEINMENU))
		S_StartSound(NULL, sfx_noooo1); // this is bad
	else if (netgame && P_IsLocalPlayer(&players[pick]))
		S_StartSound(NULL, sfx_yeeeah); // yeeeah!
	else
		S_StartSound(NULL, sfx_kc48); // just a cool sound

	if (gametype != votelevels[level][1])
	{
		INT16 lastgametype = gametype;
		gametype = votelevels[level][1];
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

#ifdef HAVE_BLUA
	LUAh_VoteThinker();
#endif

	votetic++;

<<<<<<< HEAD
	if (votetic == voteendtic)
=======
	if (intertype != int_coop || data.coop.gotperfbonus == -1)
>>>>>>> srb2/next
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
<<<<<<< HEAD
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
=======
			if (!playeringame[i]) continue;
			sharedringtotal += players[i].rings;
		}
		if (!sharedringtotal || nummaprings == -1 || sharedringtotal < nummaprings)
			bstruct->display = false;
		else
		{
			bstruct->display = true;
			bstruct->points = 50000;
		}
	}
	if (intertype != int_coop)
		return;

	data.coop.gotperfbonus = (bstruct->display ? 1 : 0);
}

static void Y_SetSpecialRingBonus(player_t *player, y_bonus_t *bstruct)
{
	INT32 i, sharedringtotal = 0;

	(void)player;
	strncpy(bstruct->patch, "YB_RING", sizeof(bstruct->patch));
	bstruct->display = true;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i]) continue;
		sharedringtotal += players[i].rings;
	}
	bstruct->points = max(0, (sharedringtotal) * 100);
}

// This list can be extended in the future with SOC/Lua, perhaps.
typedef void (*bonus_f)(player_t *, y_bonus_t *);
bonus_f bonuses_list[6][4] = {
	{
		Y_SetNullBonus,
		Y_SetNullBonus,
		Y_SetNullBonus,
		Y_SetNullBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetTimeBonus,
		Y_SetRingBonus,
		Y_SetPerfectBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetGuardBonus,
		Y_SetRingBonus,
		Y_SetNullBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetGuardBonus,
		Y_SetRingBonus,
		Y_SetPerfectBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetNightsBonus,
		Y_SetLapBonus,
		Y_SetNullBonus,
	},
	{
		Y_SetNullBonus,
		Y_SetLinkBonus,
		Y_SetLapBonus,
		Y_SetNullBonus,
	},
};
>>>>>>> srb2/next

				if (voteclient.playerinfo[i].delay)
					voteclient.playerinfo[i].delay--;

				if ((playeringame[p] && !players[p].spectator)
						&& !voteclient.playerinfo[i].delay
						&& pickedvote == -1 && votes[p] == -1)
				{
					if (InputDown(gc_aimforward, i+1) || JoyAxis(AXISAIM, i+1) < 0)
					{
						voteclient.playerinfo[i].selection--;
						pressed = true;
					}

					if ((InputDown(gc_aimbackward, i+1) || JoyAxis(AXISAIM, i+1) > 0) && !pressed)
					{
						voteclient.playerinfo[i].selection++;
						pressed = true;
					}

					if (voteclient.playerinfo[i].selection < 0)
						voteclient.playerinfo[i].selection = 3;
					if (voteclient.playerinfo[i].selection > 3)
						voteclient.playerinfo[i].selection = 0;

					if ((InputDown(gc_accelerate, i+1) || JoyAxis(AXISMOVE, i+1) > 0) && !pressed)
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
<<<<<<< HEAD
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
=======
			(bonuses_list[bonusnum][j])(&players[i], &localbonuses[j]);
			players[i].score += localbonuses[j].points;
			if (players[i].score > MAXSCORE)
				players[i].score = MAXSCORE;
		}

		ptlives = min(
			(INT32)((!ultimatemode && !modeattacking && players[i].lives != INFLIVES) ? max((INT32)((players[i].score/50000) - (oldscore/50000)), (INT32)0) : 0),
			(INT32)(mapheaderinfo[prevmap]->maxbonuslives < 0 ? INT32_MAX : mapheaderinfo[prevmap]->maxbonuslives));
		if (ptlives)
			P_GivePlayerLives(&players[i], ptlives);

		if (i == consoleplayer)
		{
			data.coop.gotlife = (((netgame || multiplayer) && G_GametypeUsesCoopLives() && cv_cooplives.value == 0) ? 0 : ptlives);
			M_Memcpy(&data.coop.bonuses, &localbonuses, sizeof(data.coop.bonuses));
>>>>>>> srb2/next
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
<<<<<<< HEAD
	INT32 i = 0;

	votetic = -1;
=======
	INT32 i, oldscore, ptlives;
	y_bonus_t localbonuses[2];

	data.spec.score = players[consoleplayer].score;
	memset(data.spec.bonuses, 0, sizeof(data.spec.bonuses));
	memset(data.spec.bonuspatches, 0, sizeof(data.spec.bonuspatches));
>>>>>>> srb2/next

#ifdef PARANOIA
	if (voteendtic != -1)
		I_Error("voteendtic is dirty");
#endif

	widebgpatch = W_CachePatchName(((gametype == GT_MATCH) ? "BATTLSCW" : "INTERSCW"), PU_STATIC);
	bgpatch = W_CachePatchName(((gametype == GT_MATCH) ? "BATTLSCR" : "INTERSCR"), PU_STATIC);
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

<<<<<<< HEAD
	voteclient.ranim = 0;
	voteclient.rtics = 1;
	voteclient.roffset = 0;
	voteclient.rsynctime = 0;
	voteclient.rendoff = 0;

	for (i = 0; i < MAXPLAYERS; i++)
		votes[i] = -1;

	for (i = 0; i < 5; i++)
	{
		lumpnum_t lumpnum;

		// set up the encore
		levelinfo[i].encore = (votelevels[i][1] & 0x80);
		votelevels[i][1] &= ~0x80;

		// set up the str
		if (i == 4)
			levelinfo[i].str[0] = '\0';
		else
		{
			// set up the levelstring
			if (mapheaderinfo[votelevels[i][0]]->levelflags & LF_NOZONE || !mapheaderinfo[votelevels[i][0]]->zonttl[0])
			{
				if (mapheaderinfo[votelevels[i][0]]->actnum[0])
					snprintf(levelinfo[i].str,
						sizeof levelinfo[i].str,
						"%s %s",
						mapheaderinfo[votelevels[i][0]]->lvlttl, mapheaderinfo[votelevels[i][0]]->actnum);
				else
					snprintf(levelinfo[i].str,
						sizeof levelinfo[i].str,
						"%s",
						mapheaderinfo[votelevels[i][0]]->lvlttl);
			}
			else
			{
				if (mapheaderinfo[votelevels[i][0]]->actnum[0])
					snprintf(levelinfo[i].str,
						sizeof levelinfo[i].str,
						"%s %s %s",
						mapheaderinfo[votelevels[i][0]]->lvlttl, mapheaderinfo[votelevels[i][0]]->zonttl, mapheaderinfo[votelevels[i][0]]->actnum);
				else
					snprintf(levelinfo[i].str,
						sizeof levelinfo[i].str,
						"%s %s",
						mapheaderinfo[votelevels[i][0]]->lvlttl, mapheaderinfo[votelevels[i][0]]->zonttl);
			}
=======
		if (!playeringame[i] || players[i].lives < 1) // not active or game over
		{
			Y_SetNullBonus(&players[i], &localbonuses[0]);
			Y_SetNullBonus(&players[i], &localbonuses[1]);
		}
		else if (maptol & TOL_NIGHTS) // NiGHTS bonus score instead of Rings
		{
			Y_SetNightsBonus(&players[i], &localbonuses[0]);
			Y_SetNullBonus(&players[i], &localbonuses[1]);
		}
		else
		{
			Y_SetSpecialRingBonus(&players[i], &localbonuses[0]);
			Y_SetPerfectBonus(&players[i], &localbonuses[1]);
		}
		players[i].score += localbonuses[0].points;
		players[i].score += localbonuses[1].points;
		if (players[i].score > MAXSCORE)
			players[i].score = MAXSCORE;

		// grant extra lives right away since tally is faked
		ptlives = min(
			(INT32)((!ultimatemode && !modeattacking && players[i].lives != INFLIVES) ? max((INT32)((players[i].score/50000) - (oldscore/50000)), (INT32)0) : 0),
			(INT32)(mapheaderinfo[prevmap]->maxbonuslives < 0 ? INT32_MAX : mapheaderinfo[prevmap]->maxbonuslives));
		P_GivePlayerLives(&players[i], ptlives);

		if (i == consoleplayer)
		{
			data.spec.gotlife = (((netgame || multiplayer) && G_GametypeUsesCoopLives() && cv_cooplives.value == 0) ? 0 : ptlives);
			M_Memcpy(&data.spec.bonuses, &localbonuses, sizeof(data.spec.bonuses));
>>>>>>> srb2/next

			levelinfo[i].str[sizeof levelinfo[i].str - 1] = '\0';
		}

		// set up the gtc and gts
		levelinfo[i].gtc = G_GetGametypeColor(votelevels[i][1]);
		if (i == 2 && votelevels[i][1] != votelevels[0][1])
			levelinfo[i].gts = gametype_cons_t[votelevels[i][1]].strvalue;
		else
			levelinfo[i].gts = NULL;

		// set up the pic
		lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(votelevels[i][0]+1)));
		if (lumpnum != LUMPERROR)
			levelinfo[i].pic = W_CachePatchName(va("%sP", G_BuildMapName(votelevels[i][0]+1)), PU_STATIC);
		else
			levelinfo[i].pic = W_CachePatchName("BLANKLVL", PU_STATIC);
	}

	voteclient.loaded = true;
}

//
// Y_EndVote
//
void Y_EndVote(void)
{
	Y_UnloadVoteData();
	voteendtic = -1;
}

<<<<<<< HEAD
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

	UNLOAD(levelinfo[4].pic);
	UNLOAD(levelinfo[3].pic);
	UNLOAD(levelinfo[2].pic);
	UNLOAD(levelinfo[1].pic);
	UNLOAD(levelinfo[0].pic);
}
=======
#define UNLOAD(x) if (x) {Z_ChangeTag(x, PU_CACHE);} x = NULL;
#define CLEANUP(x) x = NULL;
>>>>>>> srb2/next

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
<<<<<<< HEAD
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

			if (level == 4)
			{
				votes[i] = 4;
				continue;
			}

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

		if (level == 4 || endtype == 1) // Only one unique vote, so just end it immediately.
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
=======
		case int_coop:
			// unload the coop and single player patches
			UNLOAD(data.coop.bonuspatches[3]);
			UNLOAD(data.coop.bonuspatches[2]);
			UNLOAD(data.coop.bonuspatches[1]);
			UNLOAD(data.coop.bonuspatches[0]);
			UNLOAD(data.coop.ptotal);
			break;
		case int_spec:
			// unload the special stage patches
			//UNLOAD(data.spec.cemerald);
			//UNLOAD(data.spec.nowsuper);
			UNLOAD(data.spec.bonuspatches[1]);
			UNLOAD(data.spec.bonuspatches[0]);
			UNLOAD(data.spec.pscore);
			UNLOAD(data.spec.pcontinues);
			break;
		case int_match:
		case int_race:
			UNLOAD(data.match.result);
			break;
		case int_ctf:
			UNLOAD(data.match.blueflag);
			UNLOAD(data.match.redflag);
			break;
		default:
			//without this default,
			//int_none, int_tag, int_chaos, and int_comp
			//are not handled
			break;
>>>>>>> srb2/next
	}
}

static void Y_CleanupData(void)
{
	// unload the background patches
	CLEANUP(bgpatch);
	CLEANUP(widebgpatch);
	CLEANUP(bgtile);
	CLEANUP(interpic);

<<<<<<< HEAD
	deferredlevel = level;
	pickedvote = pick;
	timer = 0;
=======
	switch (intertype)
	{
		case int_coop:
			// unload the coop and single player patches
			CLEANUP(data.coop.bonuspatches[3]);
			CLEANUP(data.coop.bonuspatches[2]);
			CLEANUP(data.coop.bonuspatches[1]);
			CLEANUP(data.coop.bonuspatches[0]);
			CLEANUP(data.coop.ptotal);
			break;
		case int_spec:
			// unload the special stage patches
			//CLEANUP(data.spec.cemerald);
			//CLEANUP(data.spec.nowsuper);
			CLEANUP(data.spec.bonuspatches[1]);
			CLEANUP(data.spec.bonuspatches[0]);
			CLEANUP(data.spec.pscore);
			CLEANUP(data.spec.pcontinues);
			break;
		case int_match:
		case int_race:
			CLEANUP(data.match.result);
			break;
		case int_ctf:
			CLEANUP(data.match.blueflag);
			CLEANUP(data.match.redflag);
			break;
		default:
			//without this default,
			//int_none, int_tag, int_chaos, and int_classicrace
			//are not handled
			break;
	}
>>>>>>> srb2/next
}
