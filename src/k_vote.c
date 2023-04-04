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

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

static INT32 timer;

#define UNLOAD(x) if (x) {Patch_Free(x);} x = NULL;
#define CLEANUP(x) x = NULL;

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

#define PLANET_FRAMES (9)
#define TEXT_LEVEL_SCROLL (2*FRACUNIT)
#define TEXT_DERR_SCROLL (2*FRACUNIT)
static patch_t *bg_planet[PLANET_FRAMES] = { NULL };
static patch_t *bg_checker = NULL;
static patch_t *bg_levelText = NULL;
static patch_t *bg_derrText = NULL;

static void Y_UnloadVoteData(void);

//
// Y_VoteDrawer
//
// Draws the voting screen!
//
static void Y_DrawVoteBackground(void)
{
	static fixed_t bgTimer = 0;

	static fixed_t derrPos = 0;
	const fixed_t derrLoop = bg_derrText->width * FRACUNIT;

	static fixed_t levelPos = 0;
	const fixed_t levelLoop = bg_levelText->height * FRACUNIT;

	const UINT8 planetFrame = (bgTimer / FRACUNIT) % PLANET_FRAMES;

	V_DrawFixedPatch(
		0, 0,
		FRACUNIT, 0,
		bg_planet[planetFrame], NULL
	);
	V_DrawFixedPatch(
		(BASEVIDWIDTH - bg_checker->width) * FRACUNIT, 0,
		FRACUNIT, V_ADD,
		bg_checker, NULL
	);
	V_DrawFixedPatch(
		(BASEVIDWIDTH - bg_checker->width) * FRACUNIT, 0,
		FRACUNIT, V_ADD,
		bg_checker, NULL
	);

	levelPos += FixedMul(TEXT_DERR_SCROLL, renderdeltatics);
	while (levelPos > levelLoop)
	{
		levelPos -= levelLoop;
	}

	V_DrawFixedPatch(
		((BASEVIDWIDTH - bg_levelText->width) * FRACUNIT) - levelPos,
		-levelPos,
		FRACUNIT, V_ADD,
		bg_levelText, NULL
	);
	V_DrawFixedPatch(
		((BASEVIDWIDTH - bg_levelText->width) * FRACUNIT) - levelPos + levelLoop,
		-levelPos + levelLoop,
		FRACUNIT, V_ADD,
		bg_levelText, NULL
	);

	derrPos += FixedMul(TEXT_DERR_SCROLL, renderdeltatics);
	while (derrPos > derrLoop)
	{
		derrPos -= derrLoop;
	}

	V_DrawFixedPatch(
		-derrPos,
		(BASEVIDHEIGHT - bg_derrText->height) * FRACUNIT,
		FRACUNIT, V_SUBTRACT,
		bg_derrText, NULL
	);
	V_DrawFixedPatch(
		-derrPos + derrLoop,
		(BASEVIDHEIGHT - bg_derrText->height) * FRACUNIT,
		FRACUNIT, V_SUBTRACT,
		bg_derrText, NULL
	);

	bgTimer += renderdeltatics;
}

void Y_VoteDrawer(void)
{
	INT32 i, x, y = 0, height = 0;
	UINT8 selected[4];
	fixed_t rubyheight = 0;

	// If we early return, skip drawing the 3D scene (software buffer) so it doesn't clobber the frame for the wipe
	g_wipeskiprender = true;

	if (rendermode == render_none)
		return;

	if (votetic >= voteendtic && voteendtic != -1)
		return;

	if (!voteclient.loaded)
		return;

	g_wipeskiprender = false;

	{
		static angle_t rubyfloattime = 0;
		rubyheight = FINESINE(rubyfloattime>>ANGLETOFINESHIFT);
		rubyfloattime += FixedMul(ANGLE_MAX/NEWTICRATE, renderdeltatics);
	}

	Y_DrawVoteBackground();

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
		UINT8 j, color;

		if (selected[i])
		{
			const char *str;
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

			if (i == 3)
			{
				str = "RANDOM";
				K_DrawLikeMapThumbnail(
					(BASEVIDWIDTH-100)<<FRACBITS, (y)<<FRACBITS,
					80<<FRACBITS,
					V_SNAPTORIGHT|(levelinfo[i].encore ? V_FLIP : 0),
					randomlvl,
					NULL
				);
			}
			else
			{
				str = levelinfo[i].str;
				K_DrawMapThumbnail(
					(BASEVIDWIDTH-100)<<FRACBITS, (y)<<FRACBITS,
					80<<FRACBITS,
					V_SNAPTORIGHT|(levelinfo[i].encore ? V_FLIP : 0),
					votelevels[i][0],
					NULL
				);
			}

			if (levelinfo[i].encore)
			{
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
			if (i == 3)
			{
				K_DrawLikeMapThumbnail(
					(BASEVIDWIDTH-60)<<FRACBITS, (y)<<FRACBITS,
					40<<FRACBITS,
					V_SNAPTORIGHT|(levelinfo[i].encore ? V_FLIP : 0),
					randomlvl,
					NULL
				);
			}
			else
			{
				K_DrawMapThumbnail(
					(BASEVIDWIDTH-60)<<FRACBITS, (y)<<FRACBITS,
					40<<FRACBITS,
					V_SNAPTORIGHT|(levelinfo[i].encore ? V_FLIP : 0),
					votelevels[i][0],
					NULL
				);
			}

			if (levelinfo[i].encore)
			{
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
			if (!timer && i == voteclient.ranim)
			{
				V_DrawScaledPatch(x-18, y+9, V_SNAPTOLEFT, cursor);
				if (voteendtic != -1 && !(votetic % 4))
					V_DrawFill(x-1, y-1, 42, 27, 0|V_SNAPTOLEFT);
				else
					V_DrawFill(x-1, y-1, 42, 27, levelinfo[votes[i]].gtc|V_SNAPTOLEFT);
			}

			if (votes[i] >= 3 && (i != pickedvote || voteendtic == -1))
			{
				K_DrawLikeMapThumbnail(
					(x)<<FRACBITS, (y)<<FRACBITS,
					40<<FRACBITS,
					V_SNAPTOLEFT|(levelinfo[votes[i]].encore ? V_FLIP : 0),
					randomlvl,
					NULL
				);
			}
			else
			{
				K_DrawMapThumbnail(
					(x)<<FRACBITS, (y)<<FRACBITS,
					40<<FRACBITS,
					V_SNAPTOLEFT|(levelinfo[votes[i]].encore ? V_FLIP : 0),
					votelevels[votes[i]][0],
					NULL);
			}

			if (levelinfo[votes[i]].encore)
			{
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
		INT32 tickdown = (timer+1)/TICRATE;
		V_DrawCenteredString(BASEVIDWIDTH/2, 188, V_YELLOWMAP,
			va("Vote ends in %d", tickdown));
	}

	M_DrawMenuForeground();
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
		G_AfterIntermission();
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
	{
		S_ChangeMusicInternal("vote", true);
		S_ShowMusicCredit();
	}

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
				G_AfterIntermission();
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
	//boolean battlemode = ((votelevels[0][1] & ~VOTEMODIFIER_ENCORE) == GT_BATTLE); // todo gametyperules

	votetic = -1;

#ifdef PARANOIA
	if (voteendtic != -1)
		I_Error("voteendtic is dirty");
#endif

	cursor = W_CachePatchName("M_CURSOR", PU_STATIC);
	cursor1 = W_CachePatchName("P1CURSOR", PU_STATIC);
	cursor2 = W_CachePatchName("P2CURSOR", PU_STATIC);
	cursor3 = W_CachePatchName("P3CURSOR", PU_STATIC);
	cursor4 = W_CachePatchName("P4CURSOR", PU_STATIC);
	randomlvl = W_CachePatchName("RANDOMLV", PU_STATIC);
	rubyicon = W_CachePatchName("RUBYICON", PU_STATIC);

	for (i = 0; i < PLANET_FRAMES; i++)
	{
		bg_planet[i] = W_CachePatchName(va("VT_BG_%d", i + 1), PU_STATIC);
	}

	bg_checker = W_CachePatchName("VT_RACE", PU_STATIC);
	bg_levelText = W_CachePatchName("VT_WELC", PU_STATIC);
	bg_derrText = W_CachePatchName("VT_DERR", PU_STATIC);

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
		levelinfo[i].gtc = 73; // yellowmap[0] -- TODO rewrite vote screen
		if (i == 2 && votelevels[i][1] != votelevels[0][1])
			levelinfo[i].gts = gametypes[votelevels[i][1]]->name;
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
	INT32 i;

	voteclient.loaded = false;

	if (rendermode != render_soft)
		return;

	UNLOAD(cursor);
	UNLOAD(cursor1);
	UNLOAD(cursor2);
	UNLOAD(cursor3);
	UNLOAD(cursor4);
	UNLOAD(randomlvl);
	UNLOAD(rubyicon);

	for (i = 0; i < PLANET_FRAMES; i++)
	{
		UNLOAD(bg_planet[i]);
	}

	UNLOAD(bg_checker);
	UNLOAD(bg_levelText);
	UNLOAD(bg_derrText);
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
		G_AfterIntermission();
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
			G_AfterIntermission();
			return;
		}
		else
			S_ChangeMusicInternal("voteea", true);
	}

	deferredlevel = level;
	pickedvote = pick;
	timer = 0;
}
