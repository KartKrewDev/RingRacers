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

// Wait for any player to vote before starting the timer.
// Disabled because a player can join and be sent to
// the waiting screen, and if there's only 1 player in
// the vote screen they can idle for as long as they want,
// effectively locking the server up.
// This can be re-enabled if you want to send the vote
// screen in gamestate. (I don't feel like it.)
//#define VOTE_TIME_WAIT_FOR_VOTE

#define UNLOAD(x) if (x) {Patch_Free(x);} x = NULL;
#define CLEANUP(x) x = NULL;

#define PLANET_FRAMES (9)
#define TEXT_LEVEL_SCROLL (2*FRACUNIT)
#define TEXT_DERR_SCROLL (2*FRACUNIT)

#define ARM_FRAMES (4)
#define BULB_FRAMES (4)

#define CATCHER_SPEED (8*FRACUNIT)
#define CATCHER_Y_OFFSET (48*FRACUNIT)
#define CATCHER_OFFSCREEN (-CATCHER_Y_OFFSET * 2)

#define SELECTION_WIDTH (72*FRACUNIT)
#define SELECTION_HEIGHT ((SELECTION_WIDTH * BASEVIDHEIGHT) / BASEVIDWIDTH)
#define SELECTION_X (10*FRACUNIT + (SELECTION_WIDTH >> 1))
#define SELECTION_Y (144*FRACUNIT + (SELECTION_HEIGHT >> 1))
#define SELECTION_SPACE (4*FRACUNIT)
#define SELECTION_SPACING_W (SELECTION_WIDTH + SELECTION_SPACE)
#define SELECTION_SPACING_H (SELECTION_HEIGHT + SELECTION_SPACE)
#define SELECTION_HOP (10*FRACUNIT)

// Catcher data
enum
{
	CATCHER_NA = 0,

	CATCHER_FG_LOWER,
	CATCHER_FG_GRAB,
	CATCHER_FG_STRUGGLE,
	CATCHER_FG_POPUP,
	CATCHER_FG_RISE,

	CATCHER_BG_LOWER,
	CATCHER_BG_RELEASE,
	CATCHER_BG_RISE,
};

typedef struct
{
	fixed_t x, y;
	fixed_t destX, destY;

	UINT8 spr;
	boolean small;

	UINT8 action;
	tic_t delay;

	SINT8 level;
	UINT8 player;
} y_vote_catcher;

// Clientside & splitscreen player info.
typedef struct
{
	y_vote_catcher catcher;
	SINT8 selection;
	UINT8 delay;
} y_vote_player;

// Vote "pile" data. Objects for each vote scattered about.
typedef struct
{
	fixed_t x, y;
	fixed_t destX, destY;
	y_vote_catcher catcher;
} y_vote_pile;

// Voting roulette variables.
typedef struct
{
	y_vote_pile pile[MAXPLAYERS];
	UINT8 anim;
	UINT8 tics;
	UINT32 offset;
	UINT32 endOffset;
	UINT8 syncTime;
} y_vote_roulette;

// General vote variables
typedef struct
{
	INT32 timer;
	INT32 tic, endtic;
	boolean notYetPicked;
	boolean loaded;
	SINT8 deferredLevel;
	y_vote_player players[MAXSPLITSCREENPLAYERS];
	y_vote_roulette roulette;
} y_vote_data;

// Voting level drawing
typedef struct
{
	char str[62];
	boolean encore;
	fixed_t hop;
} y_vote_draw_level;

// General vote drawing
typedef struct
{
	patch_t *ruby_icon;
	fixed_t ruby_height;

	patch_t *bg_planet[PLANET_FRAMES];
	patch_t *bg_checker;
	patch_t *bg_levelText;
	patch_t *bg_derrText;

	patch_t *catcher_ufo;
	patch_t *catcher_arms[ARM_FRAMES];
	patch_t *catcher_pole;
	patch_t *catcher_bulb[BULB_FRAMES];

	fixed_t selectTransition;
	y_vote_draw_level levels[VOTE_NUM_LEVELS];
} y_vote_draw;

static y_vote_data vote = {0};
static y_vote_draw vote_draw = {0};

boolean Y_PlayerIDCanVote(const UINT8 playerId)
{
	player_t *player = NULL;

	if (playerId >= MAXPLAYERS || playeringame[playerId] == false)
	{
		return false;
	}

	player = &players[playerId];
	if (player->spectator == true || player->bot == true)
	{
		return false;
	}

	return true;
}

void Y_SetPlayersVote(const UINT8 playerId, SINT8 newVote)
{
	if (gamestate != GS_VOTING)
	{
		return;
	}

	if (newVote < 0 || newVote >= VOTE_NUM_LEVELS)
	{
		newVote = VOTE_NOT_PICKED;
	}

	g_votes[playerId] = newVote;

#ifdef VOTE_TIME_WAIT_FOR_VOTE
	if (vote.timer == -1)
	{
		// Someone has voted, so start the timer now.
		vote.timer = cv_votetime.value * TICRATE;
	}
#endif
}

static void Y_DrawVoteThumbnail(fixed_t x, fixed_t y, fixed_t width, INT32 flags, SINT8 v, boolean dim)
{
	const fixed_t height = (width * BASEVIDHEIGHT) / BASEVIDWIDTH;
	INT32 fx, fy, fw, fh;
	INT32 dupx, dupy;

	if (v < 0 || v >= VOTE_NUM_LEVELS)
	{
		return;
	}

	x -= width / 2;
	y -= height / 2;

	dupx = vid.dupx;
	dupy = vid.dupy;

	if (flags & V_SCALEPATCHMASK)
	{
		switch ((flags & V_SCALEPATCHMASK) >> V_SCALEPATCHSHIFT)
		{
			case 1: // V_NOSCALEPATCH
				dupx = dupy = 1;
				break;
			case 2: // V_SMALLSCALEPATCH
				dupx = vid.smalldupx;
				dupy = vid.smalldupy;
				break;
			case 3: // V_MEDSCALEPATCH
				dupx = vid.meddupx;
				dupy = vid.meddupy;
				break;
			default:
				break;
		}
	}

	// only use one dup, to avoid stretching (har har)
	dupx = dupy = (dupx < dupy ? dupx : dupy);

	fx = FixedMul(x, dupx << FRACBITS) >> FRACBITS;
	fy = FixedMul(y, dupy << FRACBITS) >> FRACBITS;
	fw = FixedMul(width - 1, dupx << FRACBITS) >> FRACBITS; // Why does only this need -1 to match up? IDFK
	fh = FixedMul(height, dupy << FRACBITS) >> FRACBITS;

	V_AdjustXYWithSnap(&fx, &fy, flags, dupx, dupy);

	V_DrawFill(
		fx - dupx, fy - dupy,
		fw + (dupx << 1), fh + (dupy << 1),
		0|flags|V_NOSCALESTART
	);

	K_DrawMapThumbnail(
		x, y,
		width, flags,
		g_voteLevels[v][0],
		NULL
	);

	if (dim == true)
	{
		V_DrawFadeFill(
			fx, fy,
			fw, fh,
			flags|V_NOSCALESTART,
			31, 5
		);
	}
}

static void Y_DrawCatcher(y_vote_catcher *catcher)
{
#define NUM_UFO_COLORS (8)
	static const skincolornum_t ufoColors[NUM_UFO_COLORS] = {
		SKINCOLOR_EMERALD,
		SKINCOLOR_SWAMP,
		SKINCOLOR_TAFFY,
		SKINCOLOR_ROSE,
		SKINCOLOR_CYAN,
		SKINCOLOR_NAVY,
		SKINCOLOR_GOLD,
		SKINCOLOR_BRONZE,
	};

#define NUM_BULB_COLORS (2)
	static const skincolornum_t bulbColors[NUM_BULB_COLORS] = {
		SKINCOLOR_JAWZ,
		SKINCOLOR_LILAC,
	};

	static fixed_t anim = 0;
	tic_t colorTic = 0;

	fixed_t baseX = INT32_MAX;
	fixed_t x = INT32_MAX;
	fixed_t y = INT32_MAX;

	UINT8 *craneColor = NULL;
	UINT8 *bulbColor = NULL;

	if (catcher->action == CATCHER_NA)
	{
		// Don't display in the empty state
		return;
	}

	anim += renderdeltatics;
	colorTic = (anim / 3) / FRACUNIT;

	baseX = catcher->x;

	if (catcher->action == CATCHER_FG_STRUGGLE)
	{
		if ((anim / FRACUNIT) & 1)
		{
			baseX += FRACUNIT;
		}
		else
		{
			baseX -= FRACUNIT;
		}
	}

	x = baseX - (vote_draw.catcher_ufo->width * FRACUNIT / 2);
	y = catcher->y - (vote_draw.catcher_ufo->height * FRACUNIT) + CATCHER_Y_OFFSET;

	craneColor = R_GetTranslationColormap(TC_DEFAULT, ufoColors[colorTic % NUM_UFO_COLORS], GTC_MENUCACHE);
	bulbColor = R_GetTranslationColormap(TC_DEFAULT, bulbColors[(colorTic / BULB_FRAMES) % NUM_BULB_COLORS], GTC_MENUCACHE);

	if (catcher->level != VOTE_NOT_PICKED)
	{
		Y_DrawVoteThumbnail(
			baseX, catcher->y,
			SELECTION_WIDTH, 0,
			catcher->level, false
		);
	}

	V_DrawFixedPatch(
		x, y,
		FRACUNIT, 0,
		vote_draw.catcher_arms[catcher->spr % ARM_FRAMES],
		craneColor
	);

	V_DrawFixedPatch(
		x, y,
		FRACUNIT, 0,
		vote_draw.catcher_bulb[colorTic % BULB_FRAMES],
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
#undef NUM_UFO_COLORS
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
		FRACUNIT, V_ADD|V_TRANSLUCENT,
		vote_draw.bg_checker, NULL
	);
	V_DrawFixedPatch(
		(BASEVIDWIDTH - vote_draw.bg_checker->width) * FRACUNIT, 0,
		FRACUNIT, V_ADD|V_TRANSLUCENT,
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

static void Y_DrawVoteSelection(fixed_t offset)
{
	fixed_t x = SELECTION_X;
	fixed_t y = SELECTION_Y + FixedMul(offset, SELECTION_HEIGHT * 2);
	INT32 i;

	//
	// Draw map icons
	//
	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		boolean selected = false;
		INT32 flags = 0;
		fixed_t destHop = 0;
		INT32 j;

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
			destHop = SELECTION_HOP;
		}

		if (vote_draw.levels[i].encore == true)
		{
			flags |= V_FLIP;
		}

		vote_draw.levels[i].hop += FixedMul(
			(destHop - vote_draw.levels[i].hop) / 2,
			renderdeltatics
		);

		Y_DrawVoteThumbnail(
			x, y - vote_draw.levels[i].hop,
			SELECTION_WIDTH, flags,
			i, (selected == false)
		);

		if (vote_draw.levels[i].encore == true)
		{
			V_DrawFixedPatch(
				x - (vote_draw.ruby_icon->width * (FRACUNIT >> 1)),
				y - (vote_draw.ruby_icon->height * (FRACUNIT >> 1)) - (vote_draw.ruby_height << 1),
				FRACUNIT, (flags & ~V_FLIP),
				vote_draw.ruby_icon,
				NULL
			);
		}

		x += SELECTION_SPACING_W;
	}

	//
	// Draw our catchers
	//
	for (i = 0; i <= splitscreen; i++)
	{
		Y_DrawCatcher(&vote.players[i].catcher);
	}
}

static void Y_DrawVotePile(void)
{
	// TODO
}

void Y_VoteDrawer(void)
{
	static angle_t rubyFloatTime = 0;

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

	vote_draw.ruby_height = FINESINE(rubyFloatTime >> ANGLETOFINESHIFT);
	rubyFloatTime += FixedMul(ANGLE_MAX / NEWTICRATE, renderdeltatics);

	vote_draw.selectTransition += FixedMul(
		(((g_pickedVote != VOTE_NOT_PICKED) ? FRACUNIT : 0) - vote_draw.selectTransition) / 2,
		renderdeltatics
	);

	Y_DrawVoteBackground();
	Y_DrawVotePile();
	Y_DrawVoteSelection(vote_draw.selectTransition);

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

static void Y_PlayerSendVote(const UINT8 localPlayer)
{
	y_vote_player *const player = &vote.players[localPlayer];
	y_vote_catcher *const catcher = &player->catcher;

	catcher->action = CATCHER_FG_LOWER;

	catcher->x = catcher->destX = SELECTION_X + (SELECTION_SPACING_W * player->selection);
	catcher->y = CATCHER_OFFSCREEN;
	catcher->destY = SELECTION_Y - SELECTION_HOP;
	catcher->spr = 0;
	catcher->level = VOTE_NOT_PICKED;

	S_StartSound(NULL, sfx_kc37);
}

static void Y_TickPlayerCatcher(const UINT8 localPlayer)
{
	y_vote_player *const player = &vote.players[localPlayer];
	y_vote_catcher *const catcher = &player->catcher;

	fixed_t spd = CATCHER_SPEED;
	fixed_t xDelta = catcher->destX - catcher->x;

	if (xDelta != 0)
	{
		// Move X position first
		if (abs(xDelta) <= spd)
		{
			catcher->x = catcher->destX;
		}
		else
		{
			if (xDelta < 0)
			{
				catcher->x -= spd;
			}
			else
			{
				catcher->x += spd;
			}
		}
	}
	else
	{
		// Then start moving Y position
		fixed_t yDelta = catcher->destY - catcher->y;

		if (abs(yDelta) <= spd)
		{
			catcher->y = catcher->destY;
		}
		else
		{
			if (yDelta < 0)
			{
				catcher->y -= spd;
			}
			else
			{
				catcher->y += spd;
			}
		}
	}

	if (catcher->delay > 0)
	{
		catcher->delay--;
		return;
	}

	switch (catcher->action)
	{
		case CATCHER_FG_LOWER:
		{
			if (catcher->x == catcher->destX && catcher->y == catcher->destY)
			{
				catcher->action++;

				S_StopSoundByNum(sfx_kc37);
				S_StartSound(NULL, sfx_kc68);
			}
			break;
		}

		case CATCHER_FG_GRAB:
		{
			catcher->spr++;

			if (catcher->spr >= ARM_FRAMES-1)
			{
				catcher->action = CATCHER_FG_STRUGGLE;
				catcher->level = vote.players[localPlayer].selection;
				catcher->delay = 20;
			}
			break;
		}

		case CATCHER_FG_STRUGGLE:
		{
			catcher->action = CATCHER_FG_POPUP;
			catcher->destY -= SELECTION_HOP * 3;
			catcher->delay = 15;
			break;
		}

		case CATCHER_FG_POPUP:
		{
			catcher->action = CATCHER_FG_RISE;
			catcher->destY = CATCHER_OFFSCREEN;
			S_StartSound(NULL, sfx_kc37);
			break;
		}

		case CATCHER_FG_RISE:
		{
			if (catcher->x == catcher->destX && catcher->y == catcher->destY)
			{
				D_ModifyClientVote(localPlayer, vote.players[localPlayer].selection);
				catcher->action = CATCHER_NA;
				S_StopSoundByNum(sfx_kc37);
			}
			break;
		}

		default:
		{
			catcher->action = CATCHER_NA;
			break;
		}
	}
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
		if (Y_PlayerIDCanVote(i) == false)
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

	if (vote.timer > 0)
	{
		vote.timer--;
	}

	for (i = 0; i <= splitscreen; i++)
	{
		Y_TickPlayerCatcher(i);
	}

	if (g_pickedVote != VOTE_NOT_PICKED)
	{
		vote.timer = 0;
		vote.roulette.syncTime++;

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

			if (vote.roulette.tics > 0)
			{
				vote.roulette.tics--;
			}
			else
			{
				vote.roulette.offset++;
				vote.roulette.tics = min(6, 9 * vote.roulette.offset / 40);
				S_StartSound(NULL, sfx_kc39);
			}

			if (vote.roulette.endOffset == 0 || vote.roulette.offset < vote.roulette.endOffset)
			{
				vote.roulette.anim = tempvotes[((g_pickedVote + vote.roulette.offset) % numvotes)];
			}

			if (vote.roulette.offset > 40)
			{
				if (vote.roulette.endOffset == 0)
				{
					if (vote.roulette.syncTime % 51 == 0) // Song is 1.45 seconds long (sorry @ whoever wants to replace it in a music wad :V)
					{
						for (i = 5; i >= 3; i--) // Find a suitable place to stop
						{
							if (tempvotes[((g_pickedVote + vote.roulette.offset + i) % numvotes)] == g_pickedVote)
							{
								vote.roulette.endOffset = vote.roulette.offset + i;

								if (M_RandomChance(FRACUNIT/32)) // Let it cheat occasionally~
								{
									vote.roulette.endOffset++;
								}

								S_ChangeMusicInternal("voteeb", false);
								break;
							}
						}
					}
				}
				else if (vote.roulette.offset >= vote.roulette.endOffset)
				{
					vote.endtic = vote.tic + (3*TICRATE);
					Y_VoteStops(g_pickedVote, vote.deferredLevel);
				}
			}
		}
		else
		{
			vote.roulette.anim = g_pickedVote;
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
		if (vote.timer != 0)
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
					&& menuactive == false
					&& vote.players[i].delay == 0
					&& g_pickedVote == VOTE_NOT_PICKED && g_votes[p] == VOTE_NOT_PICKED
					&& vote.players[i].catcher.action == CATCHER_NA)
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
						Y_PlayerSendVote(i);
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

#ifdef VOTE_TIME_WAIT_FOR_VOTE
	vote.timer = -1; // Timer is not set until the first vote is added
#else
	vote.timer = cv_votetime.value * TICRATE;
#endif

	g_pickedVote = VOTE_NOT_PICKED;
	vote.notYetPicked = true;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		vote.players[i].selection = 0;
		vote.players[i].delay = 0;
	}

	vote.roulette.anim = 0;
	vote.roulette.tics = 0;
	vote.roulette.offset = 0;
	vote.roulette.endOffset = 0;
	vote.roulette.syncTime = 0;

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

	vote_draw.selectTransition = FRACUNIT;

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

		vote.roulette.syncTime = 0;

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
			/*
			case VOTE_END_QUICK:
			{
				// Only one unique vote, so just end it immediately.
				vote.endtic = vote.tic + (5*TICRATE);
				S_ChangeMusicInternal("voteeb", false);
				Y_VoteStops(pick, level);
				break;
			}
			*/
			default:
			{
				S_ChangeMusicInternal("voteea", true);
				break;
			}
		}
	}

	vote.deferredLevel = level;
	g_pickedVote = pick;
	vote.timer = -1;
}
