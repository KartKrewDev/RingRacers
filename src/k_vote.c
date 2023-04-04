// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) by Sally "TehRealSalt" Cochenour
// Copyright (C) by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_vote.c
/// \brief Voting screen

#include "k_vote.h"

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
#include "k_hud.h" // K_DrawMapThumbnail
#include "k_battle.h"
#include "k_boss.h"
#include "k_pwrlv.h"
#include "k_grandprix.h"
#include "k_color.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#define UNLOAD(x) if (x) {Patch_Free(x);} x = NULL;
#define CLEANUP(x) x = NULL;

#define PLANET_FRAMES (9)
#define TEXT_LEVEL_SCROLL (2*FRACUNIT)
#define TEXT_DERR_SCROLL (2*FRACUNIT)

#define ARM_FRAMES (4)
#define BULB_FRAMES (4)

// Catcher data
typedef struct
{
	fixed_t x, y;
	UINT8 spr;
	boolean small;
} y_vote_catcher;

// Clientside & splitscreen player info.
typedef struct
{
	y_vote_catcher catcher;
	SINT8 selection;
	UINT8 delay;
} y_vote_player;

typedef struct
{
	INT32 timer;
	INT32 tic, endtic;
	boolean notYetPicked;
	boolean loaded;

	y_vote_player players[MAXSPLITSCREENPLAYERS];
	UINT8 ranim;
	UINT8 rtics;
	UINT8 roffset;
	UINT8 rsynctime;
	UINT8 rendoff;

	SINT8 deferredLevel;
} y_vote_data;

typedef struct
{
	char str[62];
	boolean encore;
	fixed_t hop;
} y_vote_draw_level;

typedef struct
{
	patch_t *ruby_icon;
	patch_t *bg_planet[PLANET_FRAMES];
	patch_t *bg_checker;
	patch_t *bg_levelText;
	patch_t *bg_derrText;
	patch_t *catcher_ufo;
	patch_t *catcher_arms[ARM_FRAMES];
	patch_t *catcher_pole;
	patch_t *catcher_bulb[BULB_FRAMES];
	y_vote_draw_level levels[VOTE_NUM_LEVELS];
} y_vote_draw;

static y_vote_data vote = {0};
static y_vote_draw vote_draw = {0};

static boolean Y_PlayerIDCanVote(const UINT8 id)
{
	if (id >= MAXPLAYERS)
	{
		return false;
	}

	if (playeringame[id] == false || players[id].spectator == true)
	{
		return false;
	}

	if (players[id].bot == true)
	{
		return false;
	}

	return true;
}

static void Y_DrawCatcher(y_vote_catcher *catcher)
{
#define NUM_BULB_COLORS (2)
	static const skincolornum_t bulbColors[NUM_BULB_COLORS] = {
		SKINCOLOR_JAWZ,
		SKINCOLOR_LILAC,
	};

	const tic_t anim = gametic / 3; // Using gametic for this is probably a bit goofy

	fixed_t x = catcher->x - (vote_draw.catcher_ufo->width * FRACUNIT / 2);
	fixed_t y = catcher->y - (vote_draw.catcher_ufo->height * FRACUNIT * 7 / 8);

	UINT8 *craneColor = NULL;
	UINT8 *bulbColor = NULL;

	craneColor = R_GetTranslationColormap(TC_DEFAULT, K_RainbowColor(anim), GTC_MENUCACHE);
	bulbColor = R_GetTranslationColormap(TC_DEFAULT, bulbColors[(anim / BULB_FRAMES) % NUM_BULB_COLORS], GTC_MENUCACHE);

	V_DrawFixedPatch(
		x, y,
		FRACUNIT, 0,
		vote_draw.catcher_arms[catcher->spr % ARM_FRAMES],
		craneColor
	);

	V_DrawFixedPatch(
		x, y,
		FRACUNIT, 0,
		vote_draw.catcher_bulb[anim % BULB_FRAMES],
		bulbColor
	);

	V_DrawFixedPatch(
		x, y,
		FRACUNIT, 0,
		vote_draw.catcher_ufo,
		craneColor
	);

	V_DrawFixedPatch(
		x, y,
		FRACUNIT, 0,
		vote_draw.catcher_pole,
		NULL
	);
#undef NUM_BULB_COLORS
}

//
// Y_VoteDrawer
//
// Draws the voting screen!
//
static void Y_DrawVoteBackground(void)
{
	static fixed_t bgTimer = 0;

	static fixed_t derrPos = 0;
	const fixed_t derrLoop = vote_draw.bg_derrText->width * FRACUNIT;

	static fixed_t levelPos = 0;
	const fixed_t levelLoop = vote_draw.bg_levelText->height * FRACUNIT;

	const UINT8 planetFrame = (bgTimer / FRACUNIT) % PLANET_FRAMES;

	V_DrawFixedPatch(
		0, 0,
		FRACUNIT, 0,
		vote_draw.bg_planet[planetFrame], NULL
	);
	V_DrawFixedPatch(
		(BASEVIDWIDTH - vote_draw.bg_checker->width) * FRACUNIT, 0,
		FRACUNIT, V_ADD,
		vote_draw.bg_checker, NULL
	);
	V_DrawFixedPatch(
		(BASEVIDWIDTH - vote_draw.bg_checker->width) * FRACUNIT, 0,
		FRACUNIT, V_ADD,
		vote_draw.bg_checker, NULL
	);

	levelPos += FixedMul(TEXT_DERR_SCROLL, renderdeltatics);
	while (levelPos > levelLoop)
	{
		levelPos -= levelLoop;
	}

	V_DrawFixedPatch(
		((BASEVIDWIDTH - vote_draw.bg_levelText->width) * FRACUNIT) - levelPos,
		-levelPos,
		FRACUNIT, V_ADD,
		vote_draw.bg_levelText, NULL
	);
	V_DrawFixedPatch(
		((BASEVIDWIDTH - vote_draw.bg_levelText->width) * FRACUNIT) - levelPos + levelLoop,
		-levelPos + levelLoop,
		FRACUNIT, V_ADD,
		vote_draw.bg_levelText, NULL
	);

	derrPos += FixedMul(TEXT_DERR_SCROLL, renderdeltatics);
	while (derrPos > derrLoop)
	{
		derrPos -= derrLoop;
	}

	V_DrawFixedPatch(
		-derrPos,
		(BASEVIDHEIGHT - vote_draw.bg_derrText->height) * FRACUNIT,
		FRACUNIT, V_SUBTRACT,
		vote_draw.bg_derrText, NULL
	);
	V_DrawFixedPatch(
		-derrPos + derrLoop,
		(BASEVIDHEIGHT - vote_draw.bg_derrText->height) * FRACUNIT,
		FRACUNIT, V_SUBTRACT,
		vote_draw.bg_derrText, NULL
	);

	bgTimer += renderdeltatics;
}

void Y_VoteDrawer(void)
{
	fixed_t x, y;
	fixed_t rubyHeight = 0;
	INT32 i, j;

	// If we early return, skip drawing the 3D scene (software buffer) so it doesn't clobber the frame for the wipe
	g_wipeskiprender = true;

	if (rendermode == render_none)
	{
		return;
	}

	if (vote.tic >= vote.endtic && vote.endtic != -1)
	{
		return;
	}

	if (vote.loaded == false)
	{
		return;
	}

	g_wipeskiprender = false;

	{
		static angle_t rubyFloatTime = 0;
		rubyHeight = FINESINE(rubyFloatTime >> ANGLETOFINESHIFT);
		rubyFloatTime += FixedMul(ANGLE_MAX / NEWTICRATE, renderdeltatics);
	}

	Y_DrawVoteBackground();

	x = 10 * FRACUNIT;
	y = 144 * FRACUNIT;

	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		boolean selected = false;
		INT32 flags = 0;
		fixed_t destHop = 0;

		for (j = 0; j <= splitscreen; j++) // another loop for drawing the selection backgrounds in the right order, grumble grumble..
		{
			const UINT8 p = g_localplayers[j];

			if (vote.players[j].selection != i)
			{
				continue;
			}

			if (g_votes[p] != VOTE_NOT_PICKED || Y_PlayerIDCanVote(p) == false)
			{
				continue;
			}

			selected = true;
			break;
		}

		if (selected == true)
		{
			destHop = 10*FRACUNIT;
		}
		else
		{
			flags |= V_TRANSLUCENT;
		}

		if (vote_draw.levels[i].encore == true)
		{
			flags |= V_FLIP;
		}

		if (vote_draw.levels[i].hop < destHop)
		{
			vote_draw.levels[i].hop += FixedMul(
				(destHop - vote_draw.levels[i].hop) / 2,
				renderdeltatics
			);
		}
		else
		{
			vote_draw.levels[i].hop = destHop;
		}

		K_DrawMapThumbnail(
			x, y - vote_draw.levels[i].hop,
			72 << FRACBITS, flags,
			g_voteLevels[i][0],
			NULL
		);

		if (vote_draw.levels[i].encore == true)
		{
			V_DrawFixedPatch(
				x + (40 << FRACBITS),
				((y + 25) << FRACBITS) - (rubyHeight << 1),
				FRACUNIT, (flags & ~V_FLIP),
				vote_draw.ruby_icon, 
				NULL
			);
		}

		x += (75 << FRACBITS);
	}

	// TODO: draw + update catchers
	Y_DrawCatcher(&vote.players[0].catcher);

	/*
	x = 20;
	y = 10;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (dedicated && i == 0) // While leaving blank spots for non-existent players is largely intentional, the first spot *always* being blank looks a tad silly :V
			continue;

		if ((playeringame[i] && !players[i].spectator) && g_votes[i] != VOTE_NOT_PICKED)
		{
			if (!timer && i == voteclient.ranim)
			{
				V_DrawScaledPatch(x-18, y+9, V_SNAPTOLEFT, cursor);
				if (voteendtic != -1 && !(votetic % 4))
					V_DrawFill(x-1, y-1, 42, 27, 0|V_SNAPTOLEFT);
				else
					V_DrawFill(x-1, y-1, 42, 27, levelinfo[g_votes[i]].gtc|V_SNAPTOLEFT);
			}

			K_DrawMapThumbnail(
				(x)<<FRACBITS, (y)<<FRACBITS,
				40<<FRACBITS,
				V_SNAPTOLEFT|(levelinfo[g_votes[i]].encore ? V_FLIP : 0),
				g_voteLevels[g_votes[i]][0],
				NULL
			);

			if (levelinfo[g_votes[i]].encore)
			{
				V_DrawFixedPatch((x+20)<<FRACBITS, (y<<FRACBITS) + (25<<(FRACBITS-1)) - rubyheight, FRACUNIT/2, V_SNAPTOLEFT, rubyicon, NULL);
			}

			if (levelinfo[g_votes[i]].gts)
			{
				V_DrawDiag(x, y, 8, V_SNAPTOLEFT|31);
				V_DrawDiag(x, y, 6, V_SNAPTOLEFT|levelinfo[g_votes[i]].gtc);
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
	*/

	if (vote.timer > 0)
	{
		const INT32 tickDown = (vote.timer + 1) / TICRATE;

		V_DrawCenteredString(
			BASEVIDWIDTH/2, 188,
			V_YELLOWMAP,
			va("Vote ends in %d", tickDown)
		);
	}

	M_DrawMenuForeground();
}

//
// Y_VoteStop
//
// Vote screen's selection stops moving
//
static void Y_VoteStops(SINT8 pick, SINT8 level)
{
	nextmap = g_voteLevels[level][0];

	if (netgame && P_IsLocalPlayer(&players[pick]))
	{
		S_StartSound(NULL, sfx_yeeeah); // yeeeah!
	}
	else
	{
		S_StartSound(NULL, sfx_kc48); // just a cool sound
	}

	deferencoremode = (g_voteLevels[level][1] & VOTE_MOD_ENCORE);
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

	if (paused || P_AutoPause() || vote.loaded == false)
	{
		return;
	}

	LUA_HOOK(VoteThinker);

	vote.tic++;

	if (vote.tic == vote.endtic)
	{
		Y_EndVote();
		G_AfterIntermission();
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++) // Correct votes as early as possible, before they're processed by the game at all
	{
		if (playeringame[i] == false || players[i].spectator == true)
		{
			g_votes[i] = VOTE_NOT_PICKED; // Spectators are the lower class, and have effectively no voice in the government. Democracy sucks.
		}
		else if (g_pickedVote != VOTE_NOT_PICKED && g_votes[i] == VOTE_NOT_PICKED)
		{
			g_votes[i] = 3; // Slow people get random values -- TODO: random vote doesn't exist anymore
		}
	}

	if (server && g_pickedVote != VOTE_NOT_PICKED && g_votes[g_pickedVote] == VOTE_NOT_PICKED) // Uh oh! The person who got picked left! Recalculate, quick!
	{
		D_PickVote();
	}

	if (vote.tic == 0)
	{
		S_ChangeMusicInternal("vote", true);
		S_ShowMusicCredit();
	}

	if (vote.timer)
	{
		vote.timer--;
	}

	if (g_pickedVote != VOTE_NOT_PICKED)
	{
		vote.timer = 0;
		vote.rsynctime++;

		if (vote.endtic == -1)
		{
			UINT8 tempvotes[MAXPLAYERS];
			UINT8 numvotes = 0;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (g_votes[i] == VOTE_NOT_PICKED)
				{
					continue;
				}

				tempvotes[numvotes] = i;
				numvotes++;
			}

			if (numvotes < 1) // Whoops! Get outta here.
			{
				Y_EndVote();
				G_AfterIntermission();
				return;
			}

			vote.rtics--;

			if (vote.rtics <= 0)
			{
				vote.roffset++;
				vote.rtics = min(20, (3*vote.roffset/4)+5);
				S_StartSound(NULL, sfx_kc39);
			}

			if (vote.rendoff == 0 || vote.roffset < vote.rendoff)
			{
				vote.ranim = tempvotes[((g_pickedVote + vote.roffset) % numvotes)];
			}

			if (vote.roffset >= 20)
			{
				if (vote.rendoff == 0)
				{
					if (vote.rsynctime % 51 == 0) // Song is 1.45 seconds long (sorry @ whoever wants to replace it in a music wad :V)
					{
						for (i = 5; i >= 3; i--) // Find a suitable place to stop
						{
							if (tempvotes[((g_pickedVote + vote.roffset + i) % numvotes)] == g_pickedVote)
							{
								vote.rendoff = vote.roffset + i;

								if (M_RandomChance(FRACUNIT/32)) // Let it cheat occasionally~
								{
									vote.rendoff++;
								}

								S_ChangeMusicInternal("voteeb", false);
								break;
							}
						}
					}
				}
				else if (vote.roffset >= vote.rendoff)
				{
					vote.endtic = vote.tic + (3*TICRATE);
					Y_VoteStops(g_pickedVote, vote.deferredLevel);
				}
			}
		}
		else
		{
			vote.ranim = g_pickedVote;
		}
	}
	else if (vote.notYetPicked)
	{
		if (vote.tic < 3*(NEWTICRATE/7)) // give it some time before letting you control it :V
		{
			return;
		}

		/*
		The vote ended, but it will take at least a tic for that to reach us from
		the server. Don't let me change the vote now, it won't matter anyway!
		*/
		if (vote.timer)
		{
			for (i = 0; i <= splitscreen; i++)
			{
				const UINT8 p = g_localplayers[i];
				boolean moved = false;

				if (vote.players[i].delay)
				{
					vote.players[i].delay--;
				}

				if (Y_PlayerIDCanVote(p) == true
					&& vote.players[i].delay == 0
					&& g_pickedVote == VOTE_NOT_PICKED && g_votes[p] == VOTE_NOT_PICKED
					&& menuactive == false)
				{
					if (G_PlayerInputDown(i, gc_left, 0))
					{
						vote.players[i].selection--;
						moved = true;
					}

					if (G_PlayerInputDown(i, gc_right, 0))
					{
						vote.players[i].selection++;
						moved = true;
					}

					if (vote.players[i].selection < 0)
					{
						vote.players[i].selection = VOTE_NUM_LEVELS - 1;
					}

					if (vote.players[i].selection >= VOTE_NUM_LEVELS)
					{
						vote.players[i].selection = 0;
					}

					if (G_PlayerInputDown(i, gc_a, 0) && moved == false)
					{
						D_ModifyClientVote(consoleplayer, vote.players[i].selection, i);
						moved = true;
					}
				}

				if (moved)
				{
					S_StartSound(NULL, sfx_kc4a);
					vote.players[i].delay = NEWTICRATE/7;
				}
			}
		}

		if (server)
		{
			everyone_voted = true;/* the default condition */

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (Y_PlayerIDCanVote(i) == false)
				{
					continue;
				}

				if (g_votes[i] == VOTE_NOT_PICKED)
				{
					if (vote.timer == 0)
					{
						g_votes[i] = 3; // RANDOMIZE LATER
					}
					else
					{
						everyone_voted = false;
					}
				}
			}

			if (everyone_voted == true)
			{
				vote.timer = 0;

				if (vote.endtic == -1)
				{
					vote.notYetPicked = false; /* don't pick vote twice */
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

	vote.tic = vote.endtic = -1;

	vote_draw.ruby_icon = W_CachePatchName("RUBYICON", PU_STATIC);

	for (i = 0; i < PLANET_FRAMES; i++)
	{
		vote_draw.bg_planet[i] = W_CachePatchName(va("VT_BG_%d", i + 1), PU_STATIC);
	}

	vote_draw.bg_checker = W_CachePatchName("VT_RACE", PU_STATIC);
	vote_draw.bg_levelText = W_CachePatchName("VT_WELC", PU_STATIC);
	vote_draw.bg_derrText = W_CachePatchName("VT_DERR", PU_STATIC);

	vote_draw.catcher_ufo = W_CachePatchName("VT_UFO1", PU_STATIC);
	for (i = 0; i < ARM_FRAMES; i++)
	{
		vote_draw.catcher_arms[i] = W_CachePatchName(va("VT_ARMS%d", i + 1), PU_STATIC);
	}
	vote_draw.catcher_pole = W_CachePatchName("VT_POLE", PU_STATIC);
	for (i = 0; i < BULB_FRAMES; i++)
	{
		vote_draw.catcher_bulb[i] = W_CachePatchName(va("VT_BULB%d", i + 1), PU_STATIC);
	}

	vote.timer = cv_votetime.value * TICRATE;

	g_pickedVote = VOTE_NOT_PICKED;
	vote.notYetPicked = true;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		vote.players[i].selection = 0;
		vote.players[i].delay = 0;
		vote.players[i].catcher.x = 10*FRACUNIT + (72*FRACUNIT/2);
		vote.players[i].catcher.y = 144*FRACUNIT + (48*FRACUNIT/2);
	}

	vote.ranim = 0;
	vote.rtics = 1;
	vote.roffset = 0;
	vote.rsynctime = 0;
	vote.rendoff = 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		g_votes[i] = VOTE_NOT_PICKED;
	}

	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		// set up the encore
		vote_draw.levels[i].encore = (g_voteLevels[i][1] & VOTE_MOD_ENCORE);

		// set up the levelstring
		if (mapheaderinfo[g_voteLevels[i][0]]->levelflags & LF_NOZONE || !mapheaderinfo[g_voteLevels[i][0]]->zonttl[0])
		{
			if (mapheaderinfo[g_voteLevels[i][0]]->actnum > 0)
				snprintf(vote_draw.levels[i].str,
					sizeof vote_draw.levels[i].str,
					"%s %d",
					mapheaderinfo[g_voteLevels[i][0]]->lvlttl, mapheaderinfo[g_voteLevels[i][0]]->actnum);
			else
				snprintf(vote_draw.levels[i].str,
					sizeof vote_draw.levels[i].str,
					"%s",
					mapheaderinfo[g_voteLevels[i][0]]->lvlttl);
		}
		else
		{
			if (mapheaderinfo[g_voteLevels[i][0]]->actnum > 0)
				snprintf(vote_draw.levels[i].str,
					sizeof vote_draw.levels[i].str,
					"%s %s %d",
					mapheaderinfo[g_voteLevels[i][0]]->lvlttl, mapheaderinfo[g_voteLevels[i][0]]->zonttl, mapheaderinfo[g_voteLevels[i][0]]->actnum);
			else
				snprintf(vote_draw.levels[i].str,
					sizeof vote_draw.levels[i].str,
					"%s %s",
					mapheaderinfo[g_voteLevels[i][0]]->lvlttl, mapheaderinfo[g_voteLevels[i][0]]->zonttl);
		}

		vote_draw.levels[i].str[sizeof vote_draw.levels[i].str - 1] = '\0';
	}

	vote.loaded = true;
	Automate_Run(AEV_VOTESTART);
}

//
// Y_UnloadVoteData
//
static void Y_UnloadVoteData(void)
{
	INT32 i;

	vote.loaded = false;

	if (rendermode != render_soft)
	{
		return;
	}

	UNLOAD(vote_draw.ruby_icon);

	for (i = 0; i < PLANET_FRAMES; i++)
	{
		UNLOAD(vote_draw.bg_planet[i]);
	}
	UNLOAD(vote_draw.bg_checker);
	UNLOAD(vote_draw.bg_levelText);
	UNLOAD(vote_draw.bg_derrText);

	UNLOAD(vote_draw.catcher_ufo);
	for (i = 0; i < ARM_FRAMES; i++)
	{
		UNLOAD(vote_draw.catcher_arms[i]);
	}
	UNLOAD(vote_draw.catcher_pole);
	for (i = 0; i < BULB_FRAMES; i++)
	{
		UNLOAD(vote_draw.catcher_bulb[i]);
	}
}

//
// Y_EndVote
//
void Y_EndVote(void)
{
	Y_UnloadVoteData();
	vote.endtic = -1;
}

//
// Y_SetupVoteFinish
//

enum
{
	VOTE_END_IMMEDIATE = 0,
	VOTE_END_QUICK,
	VOTE_END_NORMAL,
};

void Y_SetupVoteFinish(SINT8 pick, SINT8 level)
{
	if (vote.loaded == false)
	{
		return;
	}

	if (pick == VOTE_NOT_PICKED) // No other votes? We gotta get out of here, then!
	{
		Y_EndVote();
		G_AfterIntermission();
		return;
	}

	if (g_pickedVote == VOTE_NOT_PICKED)
	{
		INT32 i;
		SINT8 votecompare = VOTE_NOT_PICKED;
		INT32 endtype = VOTE_END_IMMEDIATE;

		vote.rsynctime = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (Y_PlayerIDCanVote(i) == true && g_votes[i] == VOTE_NOT_PICKED)
			{
				g_votes[i] = 3; // RANDOMIZE
			}

			if (g_votes[i] == VOTE_NOT_PICKED || endtype > VOTE_END_QUICK) // Don't need to go on
			{
				continue;
			}

			if (endtype == VOTE_END_NORMAL)
			{
				continue;
			}

			if (votecompare == VOTE_NOT_PICKED)
			{
				votecompare = g_votes[i];
				endtype = VOTE_END_QUICK;
			}
			else if (g_votes[i] != votecompare)
			{
				endtype = VOTE_END_NORMAL;
			}
		}

		switch (endtype)
		{
			case VOTE_END_IMMEDIATE:
			{
				// Might as well put it here, too, just in case.
				Y_EndVote();
				G_AfterIntermission();
				return;
			}
			case VOTE_END_QUICK:
			{
				// Only one unique vote, so just end it immediately.
				vote.endtic = vote.tic + (5*TICRATE);
				S_ChangeMusicInternal("voteeb", false);
				Y_VoteStops(pick, level);
				break;
			}
			default:
			{
				S_ChangeMusicInternal("voteea", true);
				break;
			}
		}
	}

	vote.deferredLevel = level;
	g_pickedVote = pick;
	vote.timer = 0;
}
