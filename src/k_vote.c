// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
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
#include "music.h"

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

#define SELECTOR_FRAMES (2)

#define CATCHER_SPEED (8*FRACUNIT)
#define CATCHER_Y_OFFSET (48*FRACUNIT)
#define CATCHER_OFFSCREEN (-CATCHER_Y_OFFSET * 2)

#define CATCHER_Y_OFFSET_SMALL (CATCHER_Y_OFFSET / 2)

#define SELECTION_WIDTH (72*FRACUNIT)
#define SELECTION_HEIGHT ((SELECTION_WIDTH * BASEVIDHEIGHT) / BASEVIDWIDTH)
#define SELECTION_X (10*FRACUNIT + (SELECTION_WIDTH >> 1))
#define SELECTION_Y (144*FRACUNIT + (SELECTION_HEIGHT >> 1))
#define SELECTION_SPACE (4*FRACUNIT)
#define SELECTION_SPACING_W (SELECTION_WIDTH + SELECTION_SPACE)
#define SELECTION_SPACING_H (SELECTION_HEIGHT + SELECTION_SPACE)
#define SELECTION_HOP (10*FRACUNIT)

#define SELECTOR_SPACE (8*FRACUNIT)
#define SELECTOR_Y ((SELECTION_HEIGHT / 2) + SELECTOR_SPACE)
#define SELECTOR_HEIGHT ((30*FRACUNIT) + SELECTOR_SPACE)

#define PILE_WIDTH (46*FRACUNIT)
#define PILE_HEIGHT ((PILE_WIDTH * BASEVIDHEIGHT) / BASEVIDWIDTH)
#define PILE_SPACE (4*FRACUNIT)
#define PILE_SPACING_W (PILE_WIDTH + PILE_SPACE)
#define PILE_SPACING_H (PILE_HEIGHT + PILE_SPACE)

#define LOTS_OF_VOTES_X (120*FRACUNIT)
#define LOTS_OF_VOTES_Y (80*FRACUNIT)

// Give time for the animations to finish before finalizing the vote stages.
#define SELECT_DELAY_TIME (TICRATE*4)
#define PICK_DELAY_TIME (TICRATE/2)
#define STRIKE_DELAY_TIME (TICRATE/3)

#define MAP_ANGER_MAX (2)

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

	fixed_t anim; // UI scope variable
} y_vote_catcher;

// Clientside & splitscreen player info.
typedef struct
{
	y_vote_catcher catcher;
	SINT8 selection;
	UINT8 delay;
	boolean sentTimeOutVote;
	fixed_t x, destX;
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
	y_vote_pile pile[VOTE_TOTAL];
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
	INT32 selectFinalize, pickFinalize, strikeFinalize;
	boolean notYetPicked;
	boolean loaded;
	SINT8 deferredLevel;
	y_vote_player players[MAXSPLITSCREENPLAYERS];
	y_vote_roulette roulette;

	// If both of these players are valid,
	// and they're the only players in the server,
	// then we want stage striking!
	player_t *strike_loser;
	player_t *strike_winner;
	boolean strike_turn;
	boolean strike_time_out;
	boolean stage_striking;
} y_vote_data;

// Voting level drawing
typedef struct
{
	char str[128];
	size_t str_len;
	boolean encore;
	fixed_t hop;
} y_vote_draw_level;

// Voting selector drawing
typedef struct
{
	fixed_t x;
	fixed_t destX;
} y_vote_draw_selector;

// General vote drawing
typedef struct
{
	patch_t *ruby_icon;
	fixed_t ruby_height;

	patch_t *strike_icon;

	patch_t *bg_planet[PLANET_FRAMES];
	patch_t *bg_checker;
	patch_t *bg_levelText;
	patch_t *bg_derrText;

	patch_t *catcher_ufo[2];
	patch_t *catcher_arms[2][ARM_FRAMES];
	patch_t *catcher_pole[2];
	patch_t *catcher_bulb[2][BULB_FRAMES];

	fixed_t selectTransition;
	y_vote_draw_level levels[VOTE_NUM_LEVELS];

	patch_t *selector_arrow;
	patch_t *selector_letter[MAXSPLITSCREENPLAYERS][2];
	y_vote_draw_selector selectors[MAXSPLITSCREENPLAYERS];
} y_vote_draw;

static y_vote_data vote = {0};
static y_vote_draw vote_draw = {0};

static void Y_SetVoteTimer(void)
{
	vote.timer = cv_votetime.value * TICRATE;

	if (vote.stage_striking == true)
	{
		vote.timer /= 2;
	}
}

static UINT8 Y_CountStriked(void)
{
	INT32 i;

	UINT8 num_striked = 0;
	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		if (g_votes_striked[i] == true)
		{
			num_striked++;
		}
	}

	return num_striked;
}

static boolean Y_VoteIDIsSpecial(const UINT8 playerId)
{
	switch (playerId)
	{
		case VOTE_SPECIAL:
		case VOTE_TIMEOUT_LOSER:
		case VOTE_TIMEOUT_WINNER:
		{
			// Special vote spot, always allow
			return true;
		}
		default:
		{
			return false;
		}
	}
}

boolean Y_PlayerIDCanVote(const UINT8 playerId)
{
	if (Y_VoteIDIsSpecial(playerId) == true)
	{
		return true;
	}

	if (playerId >= MAXPLAYERS || playeringame[playerId] == false)
	{
		return false;
	}

	const player_t *player = &players[playerId];
	if (player->spectator == true)
	{
		return false;
	}

	if (player->bot == true && cv_botscanvote.value == 0)
	{
		// Bots may only vote if the server allows it
		return false;
	}

	return true;
}

static boolean Y_IsPlayersTurn(const UINT8 playerId)
{
	if (Y_VoteIDIsSpecial(playerId) == true)
	{
		return true;
	}

	if (vote.stage_striking == false)
	{
		// Not stage striking -- we can always vote.
		return true;
	}

	// Is it our turn to strike a stage?
	const player_t *player = &players[playerId];
	if (vote.strike_turn == true)
	{
		return (player == vote.strike_winner);
	}
	else
	{
		return (player == vote.strike_loser);
	}
}

static boolean Y_PlayerIDCanVoteRightNow(const UINT8 playerId)
{
	if (Y_IsPlayersTurn(playerId) == false)
	{
		return false;
	}

	return Y_PlayerIDCanVote(playerId);
}

static boolean Y_PlayerCanSelect(const UINT8 localId)
{
	if (localId > splitscreen)
	{
		return false;
	}

	const UINT8 p = g_localplayers[localId];

	if (g_pickedVote != VOTE_NOT_PICKED)
	{
		return false;
	}

	if (g_votes[p] != VOTE_NOT_PICKED)
	{
		return false;
	}

	if (vote.players[localId].catcher.action != CATCHER_NA
		|| vote.players[localId].catcher.delay > 0)
	{
		return false;
	}

	return Y_PlayerIDCanVoteRightNow(p);
}

static void Y_SortPile(void)
{
	UINT8 numVotes = 0;
	UINT8 votesLeft = 0;
	INT32 i;

	for (i = 0; i < VOTE_TOTAL; i++)
	{
		if (g_votes[i] == VOTE_NOT_PICKED)
		{
			continue;
		}

		numVotes++;
	}

	if (numVotes == 0)
	{
		return;
	}

	votesLeft = numVotes;

	for (i = 0; i < VOTE_TOTAL; i++)
	{
		y_vote_pile *const pile = &vote.roulette.pile[i];

		if (g_votes[i] == VOTE_NOT_PICKED)
		{
			continue;
		}

		// Just center it for now.
		pile->destX = BASEVIDWIDTH << FRACBITS >> 1;
		pile->destY = BASEVIDHEIGHT << FRACBITS >> 1;

		if (numVotes <= 1)
		{
			; // NOP
		}
		else if (numVotes == 2)
		{
			// Offset just a bit from the center.
			if (votesLeft <= 1)
			{
				pile->destX += PILE_SPACING_W >> 1;
			}
			else
			{
				pile->destX -= PILE_SPACING_W >> 1;
			}
		}
		else if (numVotes <= 12)
		{
			const boolean odd = ((numVotes % 2) != 0);
			UINT8 rowSize = (numVotes + 1) / 2;
			INT32 xOffset = 0;

			if (votesLeft > rowSize)
			{
				SINT8 topRowIndex = (rowSize - ((votesLeft - 1) % rowSize)) - 1;

				if (odd == true)
				{
					rowSize--;
					topRowIndex--;
				}

				xOffset = -(rowSize - 1) + (topRowIndex << 1);

				pile->destY -= PILE_SPACING_H >> 1;
			}
			else
			{
				const SINT8 botRowIndex = votesLeft - 1;
				xOffset = -(rowSize - 1) + (botRowIndex << 1);

				pile->destY += PILE_SPACING_H >> 1;
			}

			pile->destX += (PILE_SPACING_W >> 1) * xOffset;
		}
		else
		{
			angle_t a = ANGLE_90 + (ANGLE_MAX / numVotes) * (votesLeft - 1);
			pile->destX += FixedMul(LOTS_OF_VOTES_X, FINECOSINE(a >> ANGLETOFINESHIFT));
			pile->destY += FixedMul(LOTS_OF_VOTES_Y,  -FINESINE(a >> ANGLETOFINESHIFT));
		}

		votesLeft--;

		if (votesLeft == 0)
		{
			break;
		}
	}
}

void Y_SetPlayersVote(const UINT8 inputPlayerId, SINT8 newVote)
{
	INT32 i;

	if (gamestate != GS_VOTING)
	{
		return;
	}

	UINT8 playerId = inputPlayerId;

	// Manually overwrite these players for timed out votes.
	// Loser/winner is encoded in the vote ID to prevent race
	// race conditions with real votes causing problems.
	if (inputPlayerId == VOTE_TIMEOUT_LOSER)
	{
		playerId = (vote.strike_loser - players);
	}
	else if (inputPlayerId == VOTE_TIMEOUT_WINNER)
	{
		playerId = (vote.strike_winner - players);
	}

	if (newVote < 0 || newVote >= VOTE_NUM_LEVELS)
	{
		newVote = VOTE_NOT_PICKED;
	}

	if (playerId < MAXPLAYERS)
	{
		if (Y_PlayerIDCanVoteRightNow(playerId) == false)
		{
			// Not your turn, dude!
			return;
		}

		if (vote.stage_striking == true)
		{
			if (newVote != VOTE_NOT_PICKED
				&& g_votes_striked[newVote] == false
				&& Y_CountStriked() < VOTE_NUM_LEVELS-1)
			{
				// Strike a stage, instead of voting.
				g_votes_striked[newVote] = true;

				// Change turn.
				vote.strike_turn = !vote.strike_turn;

				// Reset variables.
				Y_SetVoteTimer();
				for (i = 0; i <= splitscreen; i++)
				{
					vote.players[i].sentTimeOutVote = false;
					vote.players[i].delay = NEWTICRATE/7;
				}
				vote.strike_time_out = false;

				// TODO: striking animation
			}

			return;
		}
	}

	y_vote_pile *const pile = &vote.roulette.pile[playerId];
	y_vote_catcher *const catcher = &pile->catcher;

	g_votes[playerId] = newVote;

	Y_SortPile();

	pile->x = pile->destX;
	pile->y = pile->destY;

	catcher->action = CATCHER_BG_LOWER;

	catcher->x = catcher->destX = pile->x;
	catcher->y = CATCHER_OFFSCREEN;
	catcher->destY = pile->y;
	catcher->spr = ARM_FRAMES-1;
	catcher->level = g_votes[playerId];
	catcher->player = playerId;

#ifdef VOTE_TIME_WAIT_FOR_VOTE
	if (vote.timer == -1)
	{
		// Someone has voted, so start the timer now.
		Y_SetVoteTimer();
	}
#endif
}

static void Y_DrawVoteThumbnail(fixed_t center_x, fixed_t center_y, fixed_t width, INT32 flags, SINT8 v, boolean dim, SINT8 playerID, boolean from_selection)
{
	const boolean encore = vote_draw.levels[v].encore;
	const fixed_t height = (width * BASEVIDHEIGHT) / BASEVIDWIDTH;
	const fixed_t x = center_x - (width >> 1);
	const fixed_t y = center_y - (height >> 1);
	INT32 fx, fy, fw, fh;
	INT32 dupx, dupy;

	if (v < 0 || v >= VOTE_NUM_LEVELS)
	{
		return;
	}

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

	boolean striked = false;
	if (from_selection == true)
	{
		striked = g_votes_striked[v];
	}

	V_DrawFill(
		fx - dupx, fy - dupy,
		fw + (dupx << 1), fh + (dupy << 1),
		0|flags|V_NOSCALESTART
	);

	if (striked == true)
	{
		const fixed_t strikeScale = width / 32;
		V_DrawFixedPatch(
			center_x - (strikeScale * 25 / 2), center_y - (strikeScale * 22 / 2),
			strikeScale, flags,
			vote_draw.strike_icon,
			NULL
		);
	}
	else
	{
		K_DrawMapThumbnail(
			x, y,
			width, flags | ((encore == true) ? V_FLIP : 0),
			g_voteLevels[v][0],
			(dim == true ? R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_GREY, GTC_MENUCACHE) : NULL)
		);

		if (encore == true)
		{
			const fixed_t rubyScale = width / 72;
			V_DrawFixedPatch(
				center_x, center_y - FixedMul(vote_draw.ruby_height << 1, rubyScale),
				rubyScale, flags,
				vote_draw.ruby_icon,
				NULL
			);
		}

		if (vote.stage_striking == true
			&& from_selection == true
			&& dim == false)
		{
			if (Y_CountStriked() < VOTE_NUM_LEVELS-1)
			{
				const fixed_t strikeScale = width / 32;
				V_DrawFixedPatch(
					center_x - (strikeScale * 25 / 2), center_y - (strikeScale * 22 / 2),
					strikeScale, flags /*| V_TRANSLUCENT*/,
					vote_draw.strike_icon,
					NULL
				);
			}
		}
	}

	if (dim == true)
	{
		V_DrawFadeFill(
			fx, fy,
			fw, fh,
			flags|V_NOSCALESTART,
			31, 5
		);
	}

	if (playerID >= 0)
	{
		const INT32 whiteSq = 16 * dupx;

		if (playerID < MAXPLAYERS) // Player vote
		{
			UINT8 *playerMap = R_GetTranslationColormap(players[playerID].skin, players[playerID].skincolor, GTC_CACHE);
			patch_t *playerPatch = faceprefix[players[playerID].skin][FACE_RANK];

			V_DrawFixedPatch(
				(fx + fw - whiteSq + dupx) * FRACUNIT,
				(fy + fh - whiteSq + dupy) * FRACUNIT,
				FRACUNIT, flags|V_NOSCALESTART,
				playerPatch, playerMap
			);
		}
		else if (vote.stage_striking == false) // Angry map
		{
			K_DrawMapAsFace(
				fx + fw - whiteSq + dupx,
				fy + fh - whiteSq + dupy,
				flags | V_NOSCALESTART | ((encore == true) ? V_FLIP : 0),
				g_voteLevels[v][0],
				NULL, FRACUNIT, 1
			);
		}
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

	const UINT8 sizeOffset = (catcher->small == true) ? 1 : 0;
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

	catcher->anim += renderdeltatics;
	colorTic = (catcher->anim / 3) / FRACUNIT;

	baseX = catcher->x;

	if (catcher->action == CATCHER_FG_STRUGGLE)
	{
		if ((catcher->anim / FRACUNIT) & 1)
		{
			baseX += FRACUNIT;
		}
		else
		{
			baseX -= FRACUNIT;
		}
	}

	x = baseX - (vote_draw.catcher_ufo[sizeOffset]->width * FRACUNIT / 2);
	y = catcher->y - (vote_draw.catcher_ufo[sizeOffset]->height * FRACUNIT) + ((catcher->small == true) ? CATCHER_Y_OFFSET_SMALL : CATCHER_Y_OFFSET);

	craneColor = R_GetTranslationColormap(TC_DEFAULT, ufoColors[colorTic % NUM_UFO_COLORS], GTC_MENUCACHE);
	bulbColor = R_GetTranslationColormap(TC_DEFAULT, bulbColors[(colorTic / BULB_FRAMES) % NUM_BULB_COLORS], GTC_MENUCACHE);

	if (catcher->level != VOTE_NOT_PICKED)
	{
		Y_DrawVoteThumbnail(
			baseX, catcher->y,
			((catcher->small == true) ? PILE_WIDTH : SELECTION_WIDTH), 0,
			catcher->level, false,
			catcher->player, false
		);
	}

	V_DrawFixedPatch(
		x, y,
		FRACUNIT, 0,
		vote_draw.catcher_arms[sizeOffset][catcher->spr % ARM_FRAMES],
		craneColor
	);

	V_DrawFixedPatch(
		x, y,
		FRACUNIT, 0,
		vote_draw.catcher_bulb[sizeOffset][colorTic % BULB_FRAMES],
		bulbColor
	);

	V_DrawFixedPatch(
		x, y,
		FRACUNIT, 0,
		vote_draw.catcher_ufo[sizeOffset],
		craneColor
	);

	V_DrawFixedPatch(
		x, y,
		FRACUNIT, 0,
		vote_draw.catcher_pole[sizeOffset],
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
	static UINT32 bgTimer = 0;

	static fixed_t derrPos = 0;
	const fixed_t derrLoop = vote_draw.bg_derrText->width * FRACUNIT;

	static fixed_t levelPos = 0;
	const fixed_t levelLoop = vote_draw.bg_levelText->height * FRACUNIT;

	if (cv_reducevfx.value)
	{
		bgTimer = 0;
	}

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

	if (!cv_reducevfx.value)
	{
		bgTimer += renderdeltatics;
	}
}

static void Y_DrawVoteSelector(const fixed_t y, const fixed_t time, const UINT8 localPlayer)
{
	const fixed_t destX = SELECTION_X + (vote.players[localPlayer].selection * SELECTION_SPACING_W);

	vote_draw.selectors[localPlayer].x += FixedMul(
		(destX - vote_draw.selectors[localPlayer].x) * 3 / 4,
		renderdeltatics
	);

	if (Y_PlayerCanSelect(localPlayer) == false)
	{
		return;
	}

	static const UINT8 freq = 7;
	UINT8 *colormap = NULL;

	if (splitscreen > 0)
	{
		const UINT8 blink = ((time / freq / FRACUNIT) & 1);

		colormap = R_GetTranslationColormap(TC_RAINBOW, players[ g_localplayers[localPlayer] ].skincolor, GTC_CACHE);

		V_DrawFixedPatch(
			vote_draw.selectors[localPlayer].x, y - SELECTOR_Y - (9*FRACUNIT),
			FRACUNIT, 0,
			vote_draw.selector_letter[localPlayer][blink],
			colormap
		);
	}

	fixed_t bob = FixedMul((time / freq * 2) + (FRACUNIT / 2), ANGLE_90);
	if (localPlayer & 1)
	{
		bob = FCOS(bob);
	}
	else
	{
		bob = FSIN(bob);
	}

	V_DrawFixedPatch(
		vote_draw.selectors[localPlayer].x, y - SELECTOR_Y + bob,
		FRACUNIT, 0,
		vote_draw.selector_arrow,
		colormap
	);
}

static void Y_DrawVoteSelection(fixed_t offset)
{
	static fixed_t animTimer = 0;
	animTimer += renderdeltatics;

	const size_t charAnim = animTimer / FRACUNIT / 4;

	fixed_t x = SELECTION_X;
	fixed_t y = SELECTION_Y + FixedMul(offset, (SELECTION_HEIGHT + SELECTOR_HEIGHT) * 2);
	INT32 i;

	//
	// Draw map icons
	//
	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		boolean selected = false;
		fixed_t destHop = 0;
		INT32 j;

		for (j = 0; j <= splitscreen; j++) // another loop for drawing the selection backgrounds in the right order, grumble grumble..
		{
			const UINT8 p = g_localplayers[j];

			if (vote.players[j].selection != i)
			{
				continue;
			}

			if (g_votes[p] != VOTE_NOT_PICKED || Y_PlayerIDCanVoteRightNow(p) == false)
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

		vote_draw.levels[i].hop += FixedMul(
			(destHop - vote_draw.levels[i].hop) / 2,
			renderdeltatics
		);

		if (vote_draw.levels[i].hop > FRACUNIT >> 2)
		{
			const fixed_t height = (SELECTION_WIDTH * BASEVIDHEIGHT) / BASEVIDWIDTH;
			const fixed_t tx = x - (SELECTION_WIDTH >> 1);
			const fixed_t ty = y + (height >> 1);

			INT32 fx, fy, fw, fh;
			INT32 dupx, dupy;

			dupx = vid.dupx;
			dupy = vid.dupy;

			// only use one dup, to avoid stretching (har har)
			dupx = dupy = (dupx < dupy ? dupx : dupy);

			fx = FixedMul(tx, dupx << FRACBITS) >> FRACBITS;
			fy = FixedMul(ty, dupy << FRACBITS) >> FRACBITS;
			fw = FixedMul(SELECTION_WIDTH - 1, dupx << FRACBITS) >> FRACBITS; // Why does only this need -1 to match up? IDFK
			fh = FixedMul(SELECTION_HOP, dupy << FRACBITS) >> FRACBITS;

			V_AdjustXYWithSnap(&fx, &fy, 0, dupx, dupy);

			V_DrawFill(
				fx - dupx, fy - fh + dupy,
				fw + (dupx << 1), fh,
				31|V_NOSCALESTART
			);

			size_t ci;
			for (ci = 0; ci < 12; ci++)
			{
				const size_t c = (ci + charAnim) % vote_draw.levels[i].str_len;

				V_DrawCharacterScaled(
					(fx + (6 * dupx * ci)) << FRACBITS,
					(fy - fh + dupy) << FRACBITS,
					FRACUNIT,
					V_ORANGEMAP | V_FORCEUPPERCASE | V_NOSCALESTART,
					MED_FONT,
					vote_draw.levels[i].str[c],
					NULL
				);
			}
		}

		Y_DrawVoteThumbnail(
			x, y - vote_draw.levels[i].hop,
			SELECTION_WIDTH, 0,
			i, (selected == false),
			-1, true
		);

		x += SELECTION_SPACING_W;
	}

	if (vote.stage_striking == true && Y_CountStriked() < VOTE_NUM_LEVELS-1)
	{
		UINT8 current_strike_player = (
			(vote.strike_turn == true)
				? (vote.strike_winner - players)
				: (vote.strike_loser - players)
		);

		for (i = 0; i <= splitscreen; i++)
		{
			if (g_localplayers[i] == current_strike_player)
			{
				break;
			}
		}

		if (i > splitscreen)
		{
			const char *wait_str = va("Waiting for %s...", player_names[current_strike_player]);

			V_DrawThinString(
				BASEVIDWIDTH / 2 - (V_ThinStringWidth(wait_str, 0) / 2),
				180,
				0,
				wait_str
			);
		}
	}

	//
	// Draw our catchers
	//
	for (i = 0; i <= splitscreen; i++)
	{
		Y_DrawCatcher(&vote.players[i].catcher);
	}

	if (offset != FRACUNIT)
	{
		//
		// Draw splitscreen selectors
		//

		//if (splitscreen > 0)
		{
			const UINT8 priority = vote.tic % (splitscreen + 1);

			for (i = 0; i <= splitscreen; i++)
			{
				if (i == priority)
				{
					continue;
				}

				Y_DrawVoteSelector(y, animTimer, i);
			}

			Y_DrawVoteSelector(y, animTimer, priority);
		}
	}
}

static void Y_DrawVotePile(void)
{
	INT32 i;

	for (i = 0; i < VOTE_TOTAL; i++)
	{
		y_vote_pile *const pile = &vote.roulette.pile[i];
		y_vote_catcher *const catcher = &pile->catcher;

		if (catcher->level != VOTE_NOT_PICKED)
		{
			continue;
		}

		if (g_votes[i] == VOTE_NOT_PICKED)
		{
			continue;
		}

		Y_DrawVoteThumbnail(
			pile->x, pile->y,
			PILE_WIDTH, 0,
			g_votes[i],
			(i != vote.roulette.anim || g_pickedVote == VOTE_NOT_PICKED),
			i, false
		);
	}

	for (i = 0; i < VOTE_TOTAL; i++)
	{
		Y_DrawCatcher(&vote.roulette.pile[i].catcher);
	}
}

void Y_VoteDrawer(void)
{
	static angle_t rubyFloatTime = 0;

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

	vote_draw.ruby_height = FINESINE(rubyFloatTime >> ANGLETOFINESHIFT);
	rubyFloatTime += FixedMul(ANGLE_MAX / NEWTICRATE, renderdeltatics);

	if (vote.loaded == true)
	{
		boolean slideOut = true;
		INT32 i;

		for (i = 0; i <= splitscreen; i++)
		{
			if (g_votes[ g_localplayers[i] ] == VOTE_NOT_PICKED)
			{
				slideOut = false;
				break;
			}
		}

		vote_draw.selectTransition += FixedMul(
			((slideOut ? FRACUNIT : 0) - vote_draw.selectTransition) / 2,
			renderdeltatics
		);
	}

	Y_DrawVoteBackground();
	Y_DrawVotePile();
	Y_DrawVoteSelection(vote_draw.selectTransition);

	if (vote.timer > 0)
	{
		const INT32 tickDown = (vote.timer + 1) / TICRATE;

		// See also y_inter.c
		V__DrawOneScaleString(
			2*FRACUNIT,
			(BASEVIDHEIGHT - (2+8))*FRACUNIT,
			FRACUNIT,
			0, NULL,
			OPPRF_FONT,
			va("%d", tickDown)
		);
	}

	// TODO better voice chat speaking indicator integration
	{
		char speakingstring[2048];
		memset(speakingstring, 0, sizeof(speakingstring));

		for (int i = 0; i < MAXPLAYERS; i++)
		{
			if (netgame && cv_voice_allowservervoice.value && S_IsPlayerVoiceActive(i))
			{
				strcat(speakingstring, player_names[i]);
				strcat(speakingstring, " ");
			}
		}

		V_DrawThinString(0, 0, 0, speakingstring);
	}

	M_DrawMenuForeground();
}

//
// Y_VoteStop
//
// Vote screen's selection stops moving
//
static void Y_FinalizeVote(const SINT8 level)
{
	nextmap = g_voteLevels[level][0];
	deferencoremode = ((g_voteLevels[level][1] & VOTE_MOD_ENCORE) ==  VOTE_MOD_ENCORE);
}

static void Y_VoteStops(SINT8 pick, SINT8 level)
{
	Y_FinalizeVote(level);

	if (netgame && pick < MAXPLAYERS && P_IsPartyPlayer(&players[pick]))
	{
		S_StartSound(NULL, sfx_yeeeah); // yeeeah!
	}
	else
	{
		S_StartSound(NULL, sfx_kc48); // just a cool sound
	}
}

static void Y_PlayerSendStrike(const UINT8 localPlayer)
{
	y_vote_player *const player = &vote.players[localPlayer];
	y_vote_catcher *const catcher = &player->catcher;

	if (g_votes_striked[player->selection] == true)
	{
		// TODO: "Can't select" animation
		return;
	}

	D_ModifyClientVote(g_localplayers[localPlayer], player->selection);
	catcher->action = CATCHER_NA;
	catcher->delay = 5;
}

static void Y_PlayerSendVote(const UINT8 localPlayer)
{
	if (vote.stage_striking == true)
	{
		Y_PlayerSendStrike(localPlayer);
		return;
	}

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

static boolean Y_TickGenericCatcher(y_vote_catcher *const catcher)
{
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
		return false;
	}

	return true;
}

static void Y_TickPlayerCatcher(const UINT8 localPlayer)
{
	y_vote_player *const player = &vote.players[localPlayer];
	y_vote_catcher *const catcher = &player->catcher;

	if (Y_TickGenericCatcher(catcher) == false)
	{
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
				if (vote.stage_striking == false)
				{
					D_ModifyClientVote(g_localplayers[localPlayer], vote.players[localPlayer].selection);
				}

				catcher->action = CATCHER_NA;
				catcher->delay = 5;
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

static void Y_TickPileCatcher(const UINT8 playerId)
{
	y_vote_pile *const pile = &vote.roulette.pile[playerId];
	y_vote_catcher *const catcher = &pile->catcher;

	if (Y_TickGenericCatcher(catcher) == false)
	{
		return;
	}

	switch (catcher->action)
	{
		case CATCHER_BG_LOWER:
		{
			if (catcher->x == catcher->destX && catcher->y == catcher->destY)
			{
				catcher->level = VOTE_NOT_PICKED;
				catcher->action++;
			}
			break;
		}

		case CATCHER_BG_RELEASE:
		{
			catcher->spr--;

			if (catcher->spr == 0)
			{
				catcher->destY = CATCHER_OFFSCREEN;
				catcher->action = CATCHER_BG_RISE;
			}
			break;
		}

		case CATCHER_BG_RISE:
		{
			if (catcher->x == catcher->destX && catcher->y == catcher->destY)
			{
				catcher->action = CATCHER_NA;
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

static void Y_TickPlayerPile(const UINT8 playerId)
{
	y_vote_pile *const pile = &vote.roulette.pile[playerId];
	y_vote_catcher *const catcher = &pile->catcher;

	fixed_t movedX = 0;
	fixed_t movedY = 0;

	if (g_votes[playerId] == VOTE_NOT_PICKED)
	{
		catcher->action = CATCHER_NA;
		return;
	}

	movedX = (pile->destX - pile->x) / 2;
	movedY = (pile->destY - pile->y) / 2;

	if (movedX != 0 || movedY != 0)
	{
		pile->x += movedX;
		pile->y += movedY;

		catcher->x += movedX;
		catcher->y += movedY;

		catcher->destX += movedX;
		catcher->destY += movedY;
	}

	Y_TickPileCatcher(playerId);
}

static void Y_TickVoteRoulette(void)
{
	INT32 i;

	vote.timer = 0;
	vote.roulette.syncTime++;

	if (vote.endtic == -1)
	{
		UINT8 tempvotes[VOTE_TOTAL];
		UINT8 numvotes = 0;

		for (i = 0; i < VOTE_TOTAL; i++)
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
			vote.roulette.tics = min(5, 7 * vote.roulette.offset / 40);
			S_StartSound(NULL, sfx_kc39);
		}

		if (vote.roulette.endOffset == 0 || vote.roulette.offset < vote.roulette.endOffset)
		{
			vote.roulette.anim = tempvotes[((g_pickedVote + vote.roulette.offset) % numvotes)];
		}

		if (vote.roulette.offset > 20)
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

							if (M_RandomChance(FRACUNIT/4)) // Let it cheat occasionally~
							{
								vote.roulette.endOffset++;
							}

							Music_Play("vote_end");
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

static SINT8 Y_TryMapAngerVote(void)
{
	SINT8 angryMaps[VOTE_NUM_LEVELS] = { -1 };
	size_t angryMapsCount = 0;

	boolean mapVoted[VOTE_NUM_LEVELS] = { false };
	INT32 pick = 0;

	INT32 numPlayers = 0;
	INT32 i = 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (Y_PlayerIDCanVote(i) == false)
		{
			continue;
		}

		numPlayers++;

		if (g_votes[i] != VOTE_NOT_PICKED)
		{
			mapVoted[ g_votes[i] ] = true;
		}
	}

	if (numPlayers < 3)
	{
		// Don't handle map anger if there's not enough players.
		return VOTE_NOT_PICKED;
	}

	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		const INT16 mapID = g_voteLevels[i][0];

		if (mapVoted[i] == true)
		{
			// Someone voted for us, no need to be angry anymore :)
			mapheaderinfo[ mapID ]->anger = 0;
		}
		else
		{
			// Increment map anger for maps that weren't picked by a single soul.
			mapheaderinfo[ mapID ]->anger++;

			if (mapheaderinfo[ mapID ]->anger > MAP_ANGER_MAX)
			{
				// If they are angry enough, then it can vote for itself!
				angryMaps[ angryMapsCount ] = i;
				angryMapsCount++;
			}
		}
	}

	if (angryMapsCount == 0)
	{
		return VOTE_NOT_PICKED;
	}

	// Set the special vote to a random angry map.
	pick = M_RandomKey(angryMapsCount);
	return angryMaps[pick];
}

static void Y_ExitStageStrike(void)
{
	INT32 i;

	vote.stage_striking = false;

	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		g_votes_striked[i] = false;
	}

	vote.strike_loser = NULL;
	vote.strike_winner = NULL;
	vote.strike_turn = false;
	vote.strike_time_out = false;

	Y_SetVoteTimer();
}

static boolean Y_CheckStageStrikeStatus(void)
{
	INT32 i;
	UINT8 num_voters = 0;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (Y_PlayerIDCanVote(i) == false)
		{
			continue;
		}

		num_voters++;
		if (num_voters > 2)
		{
			break;
		}
	}

	if (num_voters != 2)
	{
		// Someone joined or left. Stage striking is broken!
		return false;
	}

	if (vote.strike_loser == NULL || Y_PlayerIDCanVote(vote.strike_loser - players) == false)
	{
		// Loser is invalidated!
		return false;
	}

	if (vote.strike_winner == NULL || Y_PlayerIDCanVote(vote.strike_winner - players) == false)
	{
		// Winner is invalidated!
		return false;
	}

	// Looks good, we can tick stage striking.
	return true;
}

static void Y_TickVoteStageStrike(void)
{
	INT32 i;

	if (Y_CheckStageStrikeStatus() == false)
	{
		Y_ExitStageStrike();
		return;
	}

	SINT8 the_only_level = VOTE_NOT_PICKED;
	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		if (g_votes_striked[i] == true)
		{
			continue;
		}

		if (the_only_level != VOTE_NOT_PICKED)
		{
			// More than 1 valid level.
			// Unset and stop iterating.
			the_only_level = VOTE_NOT_PICKED;
			break;
		}

		the_only_level = i;
	}

	if (the_only_level != VOTE_NOT_PICKED)
	{
		vote.timer = 0;
		vote.strikeFinalize = STRIKE_DELAY_TIME;

		if (vote.selectFinalize < SELECT_DELAY_TIME)
		{
			if (vote.selectFinalize == 0)
			{
				for (i = 0; i <= splitscreen; i++)
				{
					UINT8 p = g_localplayers[i];

					if (p != (vote.strike_loser - players)
						&& p != (vote.strike_winner - players))
					{
						continue;
					}

					y_vote_player *const player = &vote.players[i];
					y_vote_catcher *const catcher = &player->catcher;

					player->selection = the_only_level;
					catcher->action = CATCHER_FG_LOWER;

					catcher->x = catcher->destX = SELECTION_X + (SELECTION_SPACING_W * player->selection);
					catcher->y = CATCHER_OFFSCREEN;
					catcher->destY = SELECTION_Y - SELECTION_HOP;
					catcher->spr = 0;
					catcher->level = VOTE_NOT_PICKED;

					S_StartSound(NULL, sfx_kc37);
				}
			}

			vote.selectFinalize++;
		}

		if (vote.selectFinalize >= SELECT_DELAY_TIME)
		{
			if (vote.pickFinalize < PICK_DELAY_TIME)
			{
				vote.pickFinalize++;
			}
			else if (vote.endtic == -1)
			{
				vote.notYetPicked = false; /* don't pick vote twice */

				if (server)
				{
					D_PickVote( the_only_level );
				}
			}
		}
	}
	else
	{
		if (vote.timer == 0)
		{
			if (vote.strikeFinalize < STRIKE_DELAY_TIME)
			{
				vote.strikeFinalize++;
			}
		}
		else
		{
			vote.strikeFinalize = 0;
		}

		if (vote.strikeFinalize >= STRIKE_DELAY_TIME)
		{
			// We didn't get their timeout strike net command.
			// Maybe they hacked their exe, or connection was
			// interrupted, or some other issue.

			// Let's just strike a random stage for them.
			if (server && vote.strike_time_out == false)
			{
				INT32 rng = M_RandomKey(VOTE_NUM_LEVELS);
				for (i = 0; i < VOTE_NUM_LEVELS; i++)
				{
					if (g_votes_striked[i] == false)
					{
						break;
					}

					rng++;
					if (rng >= VOTE_NUM_LEVELS)
					{
						rng = 0;
					}
				}

				D_ModifyClientVote((vote.strike_turn == true) ? VOTE_TIMEOUT_WINNER : VOTE_TIMEOUT_LOSER, rng);
			}

			vote.strike_time_out = true;
		}
		else if (vote.timer > 0)
		{
			vote.timer--;
			vote.selectFinalize = 0;
			vote.pickFinalize = 0;
		}
	}
}

static void Y_TickVoteSelection(void)
{
	boolean everyone_voted = true;/* the default condition */
	INT32 i, j;

	if (vote.tic < 3*(NEWTICRATE/7)) // give it some time before letting you control it :V
	{
		return;
	}

	/*
	The vote ended, but it will take at least a tic for that to reach us from
	the server. Don't let me change the vote now, it won't matter anyway!
	*/
	for (i = 0; i <= splitscreen; i++)
	{
		boolean moved = false;

		if (Y_PlayerCanSelect(i) == true)
		{
			if (vote.players[i].delay > 0)
			{
				vote.players[i].delay--;
			}
			else if (vote.timer == 0)
			{
				// Time's up, send our vote ASAP.
				if (vote.players[i].sentTimeOutVote == false)
				{
					// Move off of striked stages for the timeout vote.
					for (j = 0; j < VOTE_NUM_LEVELS; j++)
					{
						if (g_votes_striked[vote.players[i].selection] == false)
						{
							break;
						}

						vote.players[i].selection++;
						if (vote.players[i].selection >= VOTE_NUM_LEVELS)
						{
							vote.players[i].selection = 0;
						}
					}

					Y_PlayerSendVote(i);
					vote.players[i].sentTimeOutVote = true;
					vote.players[i].delay = NEWTICRATE/7;
				}
			}
			else if (menuactive == false && chat_on == false)
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
		}

		if (moved == true)
		{
			S_StartSound(NULL, sfx_kc4a);
			vote.players[i].delay = NEWTICRATE/7;
		}
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (Y_PlayerIDCanVote(i) == false)
		{
			continue;
		}

		if (server && players[i].bot == true && Y_PlayerIDCanVoteRightNow(i) == true && g_votes[i] == VOTE_NOT_PICKED)
		{
			if (( M_RandomFixed() % 100 ) == 0)
			{
				// bots vote randomly
				INT32 rng = M_RandomKey(VOTE_NUM_LEVELS);
				for (j = 0; j < VOTE_NUM_LEVELS; j++)
				{
					if (g_votes_striked[j] == false)
					{
						break;
					}

					rng++;
					if (rng >= VOTE_NUM_LEVELS)
					{
						rng = 0;
					}
				}

				D_ModifyClientVote(i, rng);
			}
		}

		if (g_votes[i] == VOTE_NOT_PICKED)
		{
			everyone_voted = false;
		}
	}

	if (vote.stage_striking == true)
	{
		// Use the same selection logic, otherwise use separate ending logic.
		Y_TickVoteStageStrike();
		return;
	}

	if (everyone_voted == true)
	{
		vote.timer = 0;
		vote.selectFinalize = SELECT_DELAY_TIME;
	}

	if (vote.timer == 0)
	{
		if (vote.selectFinalize < SELECT_DELAY_TIME)
		{
			vote.selectFinalize++;
		}
	}
	else
	{
		vote.selectFinalize = 0;
	}

	if (vote.selectFinalize >= SELECT_DELAY_TIME)
	{
		if (vote.pickFinalize < PICK_DELAY_TIME)
		{
			vote.pickFinalize++;
		}
		else if (vote.endtic == -1)
		{
			vote.notYetPicked = false; /* don't pick vote twice */

			if (server)
			{
				D_PickVote( Y_TryMapAngerVote() );
			}
		}
	}
	else if (vote.timer > 0)
	{
		vote.timer--;
		vote.pickFinalize = 0;
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

	// Correct invalid votes as early as possible,
	// before they're processed by the rest of the ticker
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (Y_PlayerIDCanVote(i) == false)
		{
			// Spectators are the lower class, and have
			// effectively no voice in the government. Democracy sucks.
			g_votes[i] = VOTE_NOT_PICKED;
		}
	}

	if (server && g_pickedVote != VOTE_NOT_PICKED && g_votes[g_pickedVote] == VOTE_NOT_PICKED) // Uh oh! The person who got picked left! Recalculate, quick!
	{
		D_PickVote( VOTE_NOT_PICKED );
	}

	if (vote.tic == 0)
	{
		Music_Play("vote");
	}

	if (g_pickedVote != VOTE_NOT_PICKED)
	{
		Y_TickVoteRoulette();
	}
	else if (vote.notYetPicked == true)
	{
		Y_TickVoteSelection();
	}

	for (i = 0; i <= splitscreen; i++)
	{
		Y_TickPlayerCatcher(i);
	}

	Y_SortPile();

	for (i = 0; i < VOTE_TOTAL; i++)
	{
		Y_TickPlayerPile(i);
	}
}

//
// Y_StartVote
//
// MK online style voting screen, appears after intermission
//
static void Y_InitVoteDrawing(void)
{
	INT32 i = 0, j = 0;

	vote_draw.ruby_icon = W_CachePatchName("RUBYICON", PU_STATIC);
	vote_draw.strike_icon = W_CachePatchName("VT_LSTRK", PU_STATIC);

	for (i = 0; i < PLANET_FRAMES; i++)
	{
		vote_draw.bg_planet[i] = W_CachePatchName(va("VT_BG_%d", i + 1), PU_STATIC);
	}

	vote_draw.bg_checker = W_CachePatchName("VT_RACE", PU_STATIC);
	vote_draw.bg_levelText = W_CachePatchName("VT_WELC", PU_STATIC);
	vote_draw.bg_derrText = W_CachePatchName("VT_DERR", PU_STATIC);

	vote_draw.catcher_ufo[0] = W_CachePatchName("VT_UFO1", PU_STATIC);
	vote_draw.catcher_ufo[1] = W_CachePatchName("VS_UFO1", PU_STATIC);
	for (i = 0; i < ARM_FRAMES; i++)
	{
		vote_draw.catcher_arms[0][i] = W_CachePatchName(va("VT_ARMS%d", i + 1), PU_STATIC);
		vote_draw.catcher_arms[1][i] = W_CachePatchName(va("VS_ARMS%d", i + 1), PU_STATIC);
	}
	vote_draw.catcher_pole[0] = W_CachePatchName("VT_POLE", PU_STATIC);
	vote_draw.catcher_pole[1] = W_CachePatchName("VS_POLE", PU_STATIC);
	for (i = 0; i < BULB_FRAMES; i++)
	{
		vote_draw.catcher_bulb[0][i] = W_CachePatchName(va("VT_BULB%d", i + 1), PU_STATIC);
		vote_draw.catcher_bulb[1][i] = W_CachePatchName(va("VS_BULB%d", i + 1), PU_STATIC);
	}

	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		const mapheader_t *header = mapheaderinfo[g_voteLevels[i][0]];

		// set up the encore
		vote_draw.levels[i].encore = (g_voteLevels[i][1] & VOTE_MOD_ENCORE);

		// set up the level title string
		memset(vote_draw.levels[i].str, 0, sizeof(vote_draw.levels[i].str));
		vote_draw.levels[i].str_len = 0;

		vote_draw.levels[i].str_len += snprintf(
			vote_draw.levels[i].str + vote_draw.levels[i].str_len,
			sizeof(vote_draw.levels[i].str) - vote_draw.levels[i].str_len,
			"%s",
			header->lvlttl
		);

		if (header->zonttl[0])
		{
			vote_draw.levels[i].str_len += snprintf(
				vote_draw.levels[i].str + vote_draw.levels[i].str_len,
				sizeof(vote_draw.levels[i].str) - vote_draw.levels[i].str_len,
				" %s",
				header->zonttl
			);
		}

		if (header->actnum > 0)
		{
			vote_draw.levels[i].str_len += snprintf(
				vote_draw.levels[i].str + vote_draw.levels[i].str_len,
				sizeof(vote_draw.levels[i].str) - vote_draw.levels[i].str_len,
				" %d",
				header->actnum
			);
		}

		vote_draw.levels[i].str_len += snprintf(
			vote_draw.levels[i].str + vote_draw.levels[i].str_len,
			sizeof(vote_draw.levels[i].str) - vote_draw.levels[i].str_len,
			"    "
		);
	}

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		y_vote_player *const player = &vote.players[i];

		vote_draw.selectors[i].x = vote_draw.selectors[i].destX = SELECTION_X + (player->selection * SELECTION_SPACING_W);

		for (j = 0; j < SELECTOR_FRAMES; j++)
		{
			vote_draw.selector_letter[i][j] = W_CachePatchName(va("VSSPTR%c%d", 'A' + i, j + 1), PU_STATIC);
		}
	}

	vote_draw.selector_arrow = W_CachePatchName("VSSPTR1", PU_STATIC);

	vote_draw.selectTransition = FRACUNIT;
}

static boolean Y_DetermineStageStrike(void)
{
	player_t *a = NULL;
	player_t *b = NULL;

	UINT8 num_voters = 0;

	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (Y_PlayerIDCanVote(i) == false)
		{
			continue;
		}

		num_voters++;

		// Just set the pointers for now, sort them later.
		if (a == NULL)
		{
			a = &players[i];
		}
		else if (b == NULL)
		{
			b = &players[i];
		}
		else
		{
			// Too many players
			return false;
		}
	}

	if (num_voters != 2 || a == NULL || b == NULL)
	{
		// Requires exactly 2 of them.
		return false;
	}

	UINT32 score_a = 0;
	UINT32 score_b = 0;

	intertype_t scoring_type = Y_GetIntermissionType();
	switch (scoring_type)
	{
		case int_time:
		{
			score_a = UINT32_MAX - a->realtime;
			score_b = UINT32_MAX - b->realtime;
			break;
		}
		case int_score:
		{
			score_a = a->score;
			score_b = b->score;
			break;
		}
		default:
		{
			// Invalid, exit now.
			return false;
		}
	}

	if (a->pflags & PF_NOCONTEST)
	{
		score_a = 0;
	}

	if (b->pflags & PF_NOCONTEST)
	{
		score_b = 0;
	}

	if (score_a == score_b)
	{
		// TODO: should be a coin flip, but how
		// should the RNG for this be handled?
		score_a++;
	}

	if (score_a > score_b)
	{
		vote.strike_loser = b;
		vote.strike_winner = a;
	}
	else
	{
		vote.strike_loser = a;
		vote.strike_winner = b;
	}

	vote.stage_striking = true;
	return true;
}

void Y_StartVote(void)
{
	INT32 i = 0;

	memset(&vote, 0, sizeof(vote));
	memset(&vote_draw, 0, sizeof(vote_draw));

	// Restarting vote from the menu: stop any long sounds
	// that were playing (kc37).
	S_StopSounds();

	vote.tic = vote.endtic = -1;

	g_pickedVote = VOTE_NOT_PICKED;
	vote.notYetPicked = true;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		y_vote_player *const player = &vote.players[i];
		y_vote_catcher *const catcher = &player->catcher;

		player->selection = (i % VOTE_NUM_LEVELS);

		catcher->action = CATCHER_NA;
		catcher->small = false;
		catcher->player = -1;
	}

	for (i = 0; i < VOTE_TOTAL; i++)
	{
		y_vote_pile *const pile = &vote.roulette.pile[i];
		y_vote_catcher *const catcher = &pile->catcher;

		g_votes[i] = VOTE_NOT_PICKED;

		catcher->action = CATCHER_NA;
		catcher->small = true;
		catcher->player = i;
	}

	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		g_votes_striked[i] = false;
	}

	Y_DetermineStageStrike();

#ifdef VOTE_TIME_WAIT_FOR_VOTE
	vote.timer = -1; // Timer is not set until the first vote is added
#else
	Y_SetVoteTimer();
#endif

	Y_InitVoteDrawing();

	vote.loaded = true;
	Automate_Run(AEV_VOTESTART);
}

//
// Y_UnloadVoteData
//
static void Y_UnloadVoteData(void)
{
	INT32 i, j;

	vote.loaded = false;

	if (rendermode == render_opengl)
	{
		return;
	}

	UNLOAD(vote_draw.ruby_icon);
	UNLOAD(vote_draw.strike_icon);

	for (i = 0; i < PLANET_FRAMES; i++)
	{
		UNLOAD(vote_draw.bg_planet[i]);
	}
	UNLOAD(vote_draw.bg_checker);
	UNLOAD(vote_draw.bg_levelText);
	UNLOAD(vote_draw.bg_derrText);

	for (j = 0; j < 2; j++)
	{
		UNLOAD(vote_draw.catcher_ufo[j]);
		for (i = 0; i < ARM_FRAMES; i++)
		{
			UNLOAD(vote_draw.catcher_arms[j][i]);
		}
		UNLOAD(vote_draw.catcher_pole[j]);
		for (i = 0; i < BULB_FRAMES; i++)
		{
			UNLOAD(vote_draw.catcher_bulb[j][i]);
		}
	}

	for (j = 0; j < MAXSPLITSCREENPLAYERS; j++)
	{
		for (i = 0; i < SELECTOR_FRAMES; i++)
		{
			UNLOAD(vote_draw.selector_letter[j][i]);
		}
	}

	UNLOAD(vote_draw.selector_arrow);
}

//
// Y_EndVote
//
void Y_EndVote(void)
{
	if (nextmap >= NEXTMAP_SPECIAL)
	{
		// Don't leave nextmap unset if the vote is ended through
		// weird means! (such as a dedicated server becoming empty)
		// If nextmap was left at NEXTMAP_VOTING, we'd crash!
		Y_FinalizeVote(0);
	}

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

void Y_SetupVoteFinish(SINT8 pick, SINT8 level, SINT8 anger)
{
	if (vote.loaded == false)
	{
		return;
	}

	if (anger != VOTE_NOT_PICKED)
	{
		Y_SetPlayersVote(VOTE_SPECIAL, anger);
	}

	if (pick == VOTE_NOT_PICKED || level == VOTE_NOT_PICKED) // No other votes? We gotta get out of here, then!
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

		for (i = 0; i < VOTE_TOTAL; i++)
		{
			if (g_votes[i] == VOTE_NOT_PICKED)
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
				break;
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
				Music_Play("vote_end");
				Y_VoteStops(pick, level);
				break;
			}
			default:
			{
				Music_Play("vote_suspense");
				break;
			}
		}
	}

	vote.deferredLevel = level;
	g_pickedVote = pick;
	vote.timer = -1;
	vote.selectFinalize = SELECT_DELAY_TIME;
	vote.pickFinalize = PICK_DELAY_TIME;
	vote.strikeFinalize = STRIKE_DELAY_TIME;
}

//
// Y_VoteContext
//

enum
{
	VOTE_CTX_NORMAL = 0,
	VOTE_CTX_DUEL,
};

UINT8 Y_VoteContext(void)
{
	if (vote.stage_striking == true)
	{
		return VOTE_CTX_DUEL;
	}

	return VOTE_CTX_NORMAL;
}
