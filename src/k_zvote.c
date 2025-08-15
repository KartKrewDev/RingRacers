// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_zvote.c
/// \brief Player callable mid-game vote

#include "k_zvote.h"

#include "doomdef.h"
#include "command.h"
#include "g_game.h"
#include "g_input.h"
#include "d_clisrv.h"
#include "p_local.h"
#include "hu_stuff.h"
#include "v_video.h"
#include "k_hud.h"
#include "r_draw.h"
#include "r_fps.h"
#include "byteptr.h"
#include "s_sound.h"

extern consvar_t cv_zvote_quorum;
extern consvar_t cv_zvote_spectators;
extern consvar_t cv_zvote_length;
extern consvar_t cv_zvote_delay;

midVote_t g_midVote = {0};

typedef void (*K_ZVoteFinishCallback)(void);

typedef struct
{
	const char *name;
	const char *label;
	consvar_t cv_allowed;
	K_ZVoteFinishCallback callback;
} midVoteTypeDef_t;

/*--------------------------------------------------
	static void K_MidVoteKick(void)

		MVT_KICK's success function.
--------------------------------------------------*/
static void K_MidVoteKick(void)
{
	if (g_midVote.victim == NULL)
	{
		return;
	}

	SendKick(g_midVote.victim - players, KICK_MSG_VOTE_KICK);
}

/*--------------------------------------------------
	static void K_MidVoteMute(void)

		MVT_MUTE's success function.
--------------------------------------------------*/
static void K_MidVoteMute(void)
{
	UINT8 buf[2];

	if (g_midVote.victim == NULL)
	{
		return;
	}

	buf[0] = g_midVote.victim - players;
	buf[1] = 1;
	SendNetXCmd(XD_SERVERMUTEPLAYER, &buf, 2);
}

/*--------------------------------------------------
	static void K_MidVoteRockTheVote(void)

		MVT_RTV's success function.
--------------------------------------------------*/
static void K_MidVoteRockTheVote(void)
{
	if (G_GamestateUsesExitLevel() == false)
	{
		return;
	}

	SendNetXCmd(XD_EXITLEVEL, NULL, 0);
}

/*--------------------------------------------------
	static void K_MidVoteRunItBack(void)

		MVT_RUNITBACK's success function.
--------------------------------------------------*/
static void K_MidVoteRunItBack(void)
{
	boolean newencore = false;

	if (cv_kartencore.value != 0)
	{
		newencore = (cv_kartencore.value == 1) || encoremode;
	}

	D_MapChange(gamemap, gametype, newencore, false, 0, false, false);
}

static midVoteTypeDef_t g_midVoteTypeDefs[MVT__MAX] =
{
	{ // MVT_KICK
		"KICK",
		"Kick Player?",
		CVAR_INIT ("zvote_kick_allowed", "Yes", CV_SAVE|CV_NETVAR, CV_YesNo, NULL),
		K_MidVoteKick
	},

	{ // MVT_MUTE
		"MUTE",
		"Mute Player?",
		CVAR_INIT ("zvote_mute_allowed", "Yes", CV_SAVE|CV_NETVAR, CV_YesNo, NULL),
		K_MidVoteMute
	},

	{ // MVT_RTV
		"RTV",
		"Skip Level?",
		CVAR_INIT ("zvote_rtv_allowed", "Yes", CV_SAVE|CV_NETVAR, CV_YesNo, NULL),
		K_MidVoteRockTheVote
	},

	{ // MVT_RUNITBACK
		"RUNITBACK",
		"Redo Level?",
		CVAR_INIT ("zvote_runitback_allowed", "Yes", CV_SAVE|CV_NETVAR, CV_YesNo, NULL),
		K_MidVoteRunItBack
	},
};

/*--------------------------------------------------
	boolean K_MidVoteTypeUsesVictim(midVoteType_e voteType)

		See header file for description.
--------------------------------------------------*/
boolean K_MidVoteTypeUsesVictim(midVoteType_e voteType)
{
	switch (voteType)
	{
		case MVT_KICK:
		{
			return true;
		}
		case MVT_MUTE:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}

/*--------------------------------------------------
	static void Command_CallVote(void)

		Callback function for the "zvote_call" console command.
--------------------------------------------------*/
static void Command_CallVote(void)
{
	size_t numArgs = 0;

	const char *voteTypeStr = NULL;
	midVoteType_e voteType = MVT__MAX;

	const char *voteVariableStr = NULL;
	INT32 voteVariable = 0;

	INT32 i = INT32_MAX;

	if (netgame == false)
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	numArgs = COM_Argc();
	if (numArgs < 2)
	{
		CONS_Printf("%s <type> [variable]: calls a vote\n", COM_Argv(0));
		return;
	}

	voteTypeStr = COM_Argv(1);

	for (voteType = 0; voteType < MVT__MAX; voteType++)
	{
		if (strcasecmp(voteTypeStr, g_midVoteTypeDefs[voteType].name) == 0)
		{
			break;
		}
	}

	if (voteType == MVT__MAX)
	{
		CONS_Printf("Unknown vote type \"%s\".\n", voteTypeStr);
		return;
	}

	if (numArgs > 2)
	{
		voteVariableStr = COM_Argv(2);
		voteVariable = atoi(voteVariableStr);

		if (K_MidVoteTypeUsesVictim(voteType) == true)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (strcasecmp(player_names[i], voteVariableStr) == 0)
				{
					voteVariable = i;
					break;
				}
			}
		}
	}

	K_SendCallMidVote(voteType, voteVariable);
}

/*--------------------------------------------------
	void K_SendCallMidVote(midVoteType_e voteType, INT32 voteVariable)

		See header file for description.
--------------------------------------------------*/
void K_SendCallMidVote(midVoteType_e voteType, INT32 voteVariable)
{
	player_t *victim = NULL;

	if (K_MidVoteTypeUsesVictim(voteType) == true)
	{
		if (voteVariable >= 0 && voteVariable < MAXPLAYERS)
		{
			victim = &players[voteVariable];
		}
	}

	if (K_AllowNewMidVote(&players[consoleplayer], voteType, voteVariable, victim) == false)
	{
		// Invalid vote inputs.
		return;
	}

	UINT8 buf[MAXTEXTCMD];
	UINT8 *buf_p = buf;

	WRITEUINT8(buf_p, voteType);
	WRITEINT32(buf_p, voteVariable);

	SendNetXCmd(XD_CALLZVOTE, buf, buf_p - buf);
}

/*--------------------------------------------------
	static void Got_CallZVote(const UINT8 **cp, INT32 playernum)

		Callback function for XD_CALLZVOTE NetXCmd.
		Attempts to start a new vote using K_InitNewMidVote.

	Input Arguments:-
		cp - Pointer to readable byte stream.
		playernum - The player this packet came from.

	Return:-
		N/A
--------------------------------------------------*/
static void Got_CallZVote(const UINT8 **cp, INT32 playernum)
{
	midVoteType_e type = MVT__MAX;
	INT32 variable = 0;
	player_t *victim = NULL;

	type = READUINT8(*cp);
	variable = READINT32(*cp);

	if (K_MidVoteTypeUsesVictim(type) == true)
	{
		if (variable >= 0 && variable < MAXPLAYERS)
		{
			victim = &players[variable];
		}
	}

	K_InitNewMidVote(&players[playernum], type, variable, victim);
}

/*--------------------------------------------------
	static void K_PlayerSendMidVote(const UINT8 id)

		Sends a local player's confirmed vote to
		the server.

	Input Arguments:-
		id - Local splitscreen player ID.

	Return:-
		N/A
--------------------------------------------------*/
static void K_PlayerSendMidVote(const UINT8 id)
{
	if (id >= MAXSPLITSCREENPLAYERS)
	{
		return;
	}

	SendNetXCmdForPlayer(id, XD_SETZVOTE, NULL, 0);
}

/*--------------------------------------------------
	static void Got_SetZVote(const UINT8 **cp, INT32 playernum)

		Callback function for XD_SETZVOTE NetXCmd.
		Updates the vote table.

	Input Arguments:-
		cp - Pointer to readable byte stream.
		playernum - The player this packet came from.

	Return:-
		N/A
--------------------------------------------------*/
static void Got_SetZVote(const UINT8 **cp, INT32 playernum)
{
	(void)cp;

	if (g_midVote.active == false)
	{
		return;
	}

	S_StartSound(NULL, sfx_gshad);

	g_midVote.votes[playernum] = true;
}

/*--------------------------------------------------
	void K_RegisterMidVoteCVars(void)

		See header file for description.
--------------------------------------------------*/
void K_RegisterMidVoteCVars(void)
{
	INT32 i = INT32_MAX;

	for (i = 0; i < MVT__MAX; i++)
	{
		CV_RegisterVar(&g_midVoteTypeDefs[i].cv_allowed);
	}

	COM_AddCommand("zvote_call", Command_CallVote);

	RegisterNetXCmd(XD_CALLZVOTE, Got_CallZVote);
	RegisterNetXCmd(XD_SETZVOTE, Got_SetZVote);
}

/*--------------------------------------------------
	void K_ResetMidVote(void)

		See header file for description.
--------------------------------------------------*/
void K_ResetMidVote(void)
{
	memset(&g_midVote, 0, sizeof(g_midVote));
}

/*--------------------------------------------------
	boolean K_AnyMidVotesAllowed(void)

		See header file for description.
--------------------------------------------------*/
boolean K_AnyMidVotesAllowed(void)
{
	INT32 i = INT32_MAX;

	for (i = 0; i < MVT__MAX; i++)
	{
		if (g_midVoteTypeDefs[i].cv_allowed.value != 0)
		{
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	midVoteType_e K_GetNextCallableMidVote(INT32 seed, boolean backwards)

		See header file for description.
--------------------------------------------------*/

midVoteType_e K_GetNextAllowedMidVote(midVoteType_e seed, boolean backwards)
{
	if (seed >= MVT__MAX)
		seed = 0;

	midVoteType_e i = seed;

	if (backwards)
	{
		do
		{
			if (i <= 0)
				i = MVT__MAX;
			i--;

			if (g_midVoteTypeDefs[i].cv_allowed.value != 0)
				return i;

		}
		while (i != seed);
	}
	else
	{
		do
		{
			i++;
			if (i >= MVT__MAX)
				i = 0;

			if (g_midVoteTypeDefs[i].cv_allowed.value != 0)
				return i;

		}
		while (i != seed);
	}

	return MVT__MAX;
}

/*--------------------------------------------------
	boolean K_PlayerIDAllowedInMidVote(const UINT8 id)

		See header file for description.
--------------------------------------------------*/
boolean K_PlayerIDAllowedInMidVote(const UINT8 id)
{
	const player_t *player = &players[id];

	if (playeringame[id] == false)
	{
		// Needs to be present to vote.
		return false;
	}

	if (player->bot == true)
	{
		// Bots don't vote on these issues.
		return false;
	}

	if (cv_zvote_spectators.value == 0 && player->spectator == true)
	{
		// Spectators don't vote on these issues, unless the server allows it.
		return false;
	}

	return true;
}

/*--------------------------------------------------
	UINT8 K_RequiredMidVotes(void)

		See header file for description.
--------------------------------------------------*/
UINT8 K_RequiredMidVotes(void)
{
	UINT8 allowedCount = 0;
	INT32 i = INT32_MAX;

	if (g_midVote.active == false)
	{
		// No vote is currently running.
		return 0;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (K_PlayerIDAllowedInMidVote(i) == true)
		{
			allowedCount++;
		}
	}

	if (allowedCount > 1)
	{
		return max(
			2, // require at least 2 votes, regardless of how low the quorum is
			(FixedMul(
				(allowedCount << FRACBITS),
				cv_zvote_quorum.value
			) + (FRACUNIT >> 1)) >> FRACBITS // Round up to bias towards more votes being required in small games
		);
	}
	else
	{
		// 1P session, just require the one vote.
		return 1;
	}
}

/*--------------------------------------------------
	boolean K_PlayerIDMidVoted(const UINT8 id)

		See header file for description.
--------------------------------------------------*/
boolean K_PlayerIDMidVoted(const UINT8 id)
{
	const player_t *player = &players[id];

	if (K_PlayerIDAllowedInMidVote(id) == false)
	{
		// This person isn't allowed to participate in votes.
		return false;
	}

	if (player == g_midVote.caller)
	{
		// The person who called the vote always votes for it.
		return true;
	}
	else if (player == g_midVote.victim)
	{
		// The person being voted off never votes for it.
		return false;
	}

	return g_midVote.votes[id];
}

/*--------------------------------------------------
	UINT8 K_CountMidVotes(void)

		See header file for description.
--------------------------------------------------*/
UINT8 K_CountMidVotes(void)
{
	UINT8 voteCount = 0;
	INT32 i = INT32_MAX;

	if (g_midVote.active == false)
	{
		// No vote is currently running.
		return 0;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (K_PlayerIDMidVoted(i) == true)
		{
			voteCount++;
		}
	}

	return voteCount;
}

/*--------------------------------------------------
	boolean K_MinimalCheckNewMidVote(midVoteType_e type)

		See header file for description.
--------------------------------------------------*/
boolean K_MinimalCheckNewMidVote(midVoteType_e type)
{
	if (g_midVote.active == true)
	{
		// Don't allow another vote if one is already running.
		return false;
	}

	if (g_midVote.delay > 0)
	{
		// Don't allow another vote if one has recently just ran.
		return false;
	}

	if (type < 0 || type >= MVT__MAX)
	{
		// Invalid range.
		return false;
	}

	if (g_midVoteTypeDefs[type].cv_allowed.value == 0)
	{
		// These types of votes aren't allowed on this server.
		return false;
	}

	if (K_PlayerIDAllowedInMidVote(consoleplayer) == false)
	{
		// Invalid calling player.
		return false;
	}

	return true;
}

/*--------------------------------------------------
	boolean K_AllowNewMidVote(player_t *caller, midVoteType_e type, INT32 variable, player_t *victim)

		See header file for description.
--------------------------------------------------*/
boolean K_AllowNewMidVote(player_t *caller, midVoteType_e type, INT32 variable, player_t *victim)
{
	(void)variable;

	if (g_midVote.active == true)
	{
		// Don't allow another vote if one is already running.
		if (P_IsMachineLocalPlayer(caller) == true)
		{
			CONS_Alert(CONS_ERROR, "A vote is already in progress.\n");
		}

		return false;
	}

	if (g_midVote.delay > 0)
	{
		// Don't allow another vote if one has recently just ran.
		if (P_IsMachineLocalPlayer(caller) == true)
		{
			CONS_Alert(CONS_ERROR, "Another vote was called too recently.\n");
		}

		return false;
	}

	if (type < 0 || type >= MVT__MAX)
	{
		// Invalid range.
		if (P_IsMachineLocalPlayer(caller) == true)
		{
			CONS_Alert(CONS_ERROR, "Invalid vote type.\n");
		}

		return false;
	}

	if (g_midVoteTypeDefs[type].cv_allowed.value == 0)
	{
		// These types of votes aren't allowed on this server.
		if (P_IsMachineLocalPlayer(caller) == true)
		{
			CONS_Alert(CONS_ERROR, "Vote type is not allowed in this server.\n");
		}

		return false;
	}

	if (caller == NULL || K_PlayerIDAllowedInMidVote(caller - players) == false)
	{
		// Invalid calling player.
		if (caller != NULL && P_IsMachineLocalPlayer(caller) == true)
		{
			CONS_Alert(CONS_ERROR, "Invalid calling player.\n");
		}

		return false;
	}

	if (K_MidVoteTypeUsesVictim(type) == true)
	{
		if (victim == NULL)
		{
			// Invalid victim.
			if (P_IsMachineLocalPlayer(caller) == true)
			{
				CONS_Alert(CONS_ERROR, "Can't kick this player; it's invalid.\n");
			}

			return false;
		}

		if (caller == victim)
		{
			if (P_IsMachineLocalPlayer(caller) == true)
			{
				CONS_Alert(CONS_ERROR, "Can't kick yourself.\n");
			}

			return false;
		}

		if ((victim - players) == serverplayer
#ifndef DEVELOP
			|| IsPlayerAdmin((victim - players)) == true
#endif
			)
		{
			// Victim is the server or an admin.
			if (P_IsMachineLocalPlayer(caller) == true)
			{
				CONS_Alert(CONS_ERROR, "Can't kick this player; they are an administrator.\n");
			}

			return false;
		}
	}

	return true;
}

/*--------------------------------------------------
	void K_InitNewMidVote(player_t *caller, midVoteType_e type, INT32 variable, player_t *victim)

		See header file for description.
--------------------------------------------------*/
void K_InitNewMidVote(player_t *caller, midVoteType_e type, INT32 variable, player_t *victim)
{
	INT32 i = INT32_MAX;

	if (K_AllowNewMidVote(caller, type, variable, victim) == false)
	{
		// Invalid vote inputs.
		return;
	}

	K_ResetMidVote();

	g_midVote.active = true;
	g_midVote.caller = caller;

	g_midVote.type = type;
	g_midVote.variable = variable;
	g_midVote.victim = victim;

	if (server || IsPlayerAdmin(consoleplayer))
	{
		if (victim)
			HU_AddChatText(va("%s called a vote to %s %s\n", player_names[caller-players], g_midVoteTypeDefs[type].name, player_names[victim-players]), true);
		else
			HU_AddChatText(va("%s called a vote to %s\n", player_names[caller-players], g_midVoteTypeDefs[type].name), true);
	}

	S_StartSound(NULL, sfx_cdfm67);

	g_midVote.votes[caller - players] = true;

	for (i = 0; i <= splitscreen; i++)
	{
		if (caller == &players[g_localplayers[i]])
		{
			// The person who voted should already be confirmed.
			g_midVote.gui[i].slide = ZVOTE_GUI_SLIDE;
			g_midVote.gui[i].confirm = ZVOTE_GUI_CONFIRM;
			g_midVote.gui[i].unpress = true;
		}
	}
}

/*--------------------------------------------------
	void K_MidVoteFinalize(fixed_t delayMul)

		See header file for description.
--------------------------------------------------*/
void K_MidVoteFinalize(fixed_t delayMul)
{
	K_ResetMidVote();
	g_midVote.delay = FixedMul(cv_zvote_delay.value * TICRATE, delayMul);
}

/*--------------------------------------------------
	void K_MidVoteSuccess(void)

		See header file for description.
--------------------------------------------------*/
void K_MidVoteSuccess(void)
{
	if (
		server == true
		&& demo.playback == false
		&& g_midVoteTypeDefs[ g_midVote.type ].callback != NULL
	)
	{
		g_midVoteTypeDefs[ g_midVote.type ].callback();
	}

	K_MidVoteFinalize(FRACUNIT); // Vote succeeded, so the delay is normal.
}

/*--------------------------------------------------
	void K_MidVoteFailure(void)

		See header file for description.
--------------------------------------------------*/
void K_MidVoteFailure(void)
{
	K_MidVoteFinalize(2*FRACUNIT); // Vote failed, so the delay is longer.
}

/*--------------------------------------------------
	static void K_HandleMidVoteInput(void)

		See header file for description.
--------------------------------------------------*/
static void K_HandleMidVoteInput(void)
{
	INT32 i = INT32_MAX;

	for (i = 0; i <= splitscreen; i++)
	{
		//player_t *const player = &players[ g_localplayers[i] ];
		midVoteGUI_t *const gui = &g_midVote.gui[i];
		boolean pressed = false;

		if (menuactive == false)
		{
			pressed = G_PlayerInputDown(i, gc_z, 0);
		}

		// Between states, require us to unpress Z.
		if (pressed == true)
		{
			if (gui->unpress == true)
			{
				pressed = false;
			}
		}
		else
		{
			gui->unpress = false;
		}

		if (gui->slide < ZVOTE_GUI_SLIDE)
		{
			if (gui->slide > 0 || pressed == true)
			{
				gui->slide++;
				gui->unpress = true;
			}
		}
		else if (gui->confirm < ZVOTE_GUI_CONFIRM)
		{
			if (K_PlayerIDAllowedInMidVote(g_localplayers[i]) == false)
			{
				gui->confirm = 0;
				continue;
			}

			if (pressed == true)
			{
				gui->confirm++;

				if (gui->confirm == ZVOTE_GUI_CONFIRM)
				{
					K_PlayerSendMidVote(i);
					gui->unpress = true;
				}
			}
			else
			{
				gui->confirm = 0;
			}
		}
	}
}

#define ZVOTE_PATCH_EXC_START (4)
#define ZVOTE_PATCH_EXC_LOOP (3)
#define ZVOTE_PATCH_BAR_SEGS (12)

/*--------------------------------------------------
	void K_TickMidVote(void)

		See header file for description.
--------------------------------------------------*/
void K_TickMidVote(void)
{
	UINT8 numVotes = 0;
	UINT8 requiredVotes = 0;

	if (g_midVote.active == false)
	{
		// No vote is currently running.
		if (g_midVote.delay > 0)
		{
			// Decrement timer for allowing the next vote.
			g_midVote.delay--;
		}

		return;
	}

	if (g_midVote.end > 0)
	{
		g_midVote.end++;

		if (g_midVote.end > ZVOTE_GUI_SUCCESS)
		{
			if (g_midVote.endVotes >= g_midVote.endRequired)
			{
				K_MidVoteSuccess();
			}
			else
			{
				K_MidVoteFailure();
			}
		}

		return;
	}

	numVotes = K_CountMidVotes();
	requiredVotes = K_RequiredMidVotes();

	if (numVotes >= requiredVotes
		|| g_midVote.time > (unsigned)(cv_zvote_length.value * TICRATE))
	{
		// Vote finished.
		// Start the ending animation.
		S_StartSound(NULL, sfx_kc48);
		g_midVote.end++;
		g_midVote.endVotes = numVotes;
		g_midVote.endRequired = requiredVotes;
		return;
	}

	K_HandleMidVoteInput();
	g_midVote.time++;

	// Go go gadget duplicated code. Sorry, this blows ass and makes no sense.
	// I hope we never change this timing again, but if we do, check the drawer as well.
	const tic_t spd = 2;
	const tic_t anim = (g_midVote.time - ZVOTE_GUI_SLIDE) / spd;
	const UINT8 frame = anim % (ZVOTE_PATCH_EXC_LOOP + ZVOTE_GUI_SLIDE);

	if (frame == 0 && g_midVote.time % spd == 0 && g_midVote.gui[R_GetViewNumber()].slide == 0)
		S_StartSound(NULL, sfx_s3kd2s);
}

/*--------------------------------------------------
	void K_CacheMidVotePatches(void)

		See header file for description.
--------------------------------------------------*/

static patch_t *g_exclamationSlide = NULL;
static patch_t *g_exclamationStart[ZVOTE_PATCH_EXC_LOOP] = {NULL};
static patch_t *g_exclamation = NULL;
static patch_t *g_exclamationLoop[ZVOTE_PATCH_EXC_LOOP] = {NULL};

static patch_t *g_zBar[2] = {NULL};
static patch_t *g_zBarEnds[2][2][2] = {{{NULL}}};

void K_UpdateMidVotePatches(void)
{
	HU_UpdatePatch(&g_exclamationSlide, "TLSBA0");

	HU_UpdatePatch(&g_exclamationStart[0], "TLSBB0");
	HU_UpdatePatch(&g_exclamationStart[1], "TLSBC0");
	HU_UpdatePatch(&g_exclamationStart[2], "TLSBD0");

	HU_UpdatePatch(&g_exclamation, "TLSBE0");

	HU_UpdatePatch(&g_exclamationLoop[0], "TLSBF0");
	HU_UpdatePatch(&g_exclamationLoop[1], "TLSBG0");
	HU_UpdatePatch(&g_exclamationLoop[2], "TLSBD0");

	HU_UpdatePatch(&g_zBar[0], "TLBWE0");

	HU_UpdatePatch(&g_zBarEnds[0][0][0], "TLBWC0");
	HU_UpdatePatch(&g_zBarEnds[0][0][1], "TLBWD0");

	HU_UpdatePatch(&g_zBarEnds[0][1][0], "TLBWA0");
	HU_UpdatePatch(&g_zBarEnds[0][1][1], "TLBWB0");

	HU_UpdatePatch(&g_zBar[1], "TLBXE0");

	HU_UpdatePatch(&g_zBarEnds[1][0][0], "TLBXC0");
	HU_UpdatePatch(&g_zBarEnds[1][0][1], "TLBXD0");

	HU_UpdatePatch(&g_zBarEnds[1][1][0], "TLBXA0");
	HU_UpdatePatch(&g_zBarEnds[1][1][1], "TLBXB0");
}

/*--------------------------------------------------
	const char *K_GetMidVoteLabel(midVoteType_e i)

		See header file for description.
--------------------------------------------------*/

const char *K_GetMidVoteLabel(midVoteType_e i)
{
	if (
		i < 0
		|| i >= MVT__MAX
		|| g_midVoteTypeDefs[i].label == NULL)
	{
		return "N/A";
	}

	return g_midVoteTypeDefs[i].label;
}

/*--------------------------------------------------
	static void K_DrawMidVoteBar(fixed_t x, fixed_t y, INT32 flags, fixed_t fill, skincolornum_t color, boolean flipped)

		Draws a bar

	Input Arguments:-
		voteType - The vote type to check.

	Return:-
		true if it uses a victim, otherwise false.
--------------------------------------------------*/
static void K_DrawMidVoteBar(fixed_t x, fixed_t y, INT32 flags, fixed_t fill, skincolornum_t color, boolean flipped)
{
	const SINT8 sign = (flipped == true) ? -1 : 1;
	patch_t *bar = g_zBar[0];
	UINT8 *clm = NULL;
	INT32 i = INT32_MAX;

	if (color > SKINCOLOR_NONE)
	{
		clm = R_GetTranslationColormap(TC_BLINK, color, GTC_CACHE);
	}

	for (i = 0; i < ZVOTE_PATCH_BAR_SEGS; i++)
	{
		bar = g_zBar[0];

		if (i == 0)
		{
			bar = g_zBarEnds[0][flipped][0];
		}
		else if (i == ZVOTE_PATCH_BAR_SEGS - 1)
		{
			bar = g_zBarEnds[0][flipped][1];
		}

		if (fill < FRACUNIT)
		{
			V_DrawFixedPatch(
				x, y,
				FRACUNIT, flags,
				bar, NULL
			);
		}

		x += bar->width * FRACUNIT * sign;
	}

	if (fill > 0)
	{
		const INT32 fillSegs = FixedMul(fill, ZVOTE_PATCH_BAR_SEGS);

		for (i = 0; i < fillSegs; i++)
		{
			x -= bar->width * FRACUNIT * sign;

			bar = g_zBar[1];

			if (i == fillSegs - 1)
			{
				bar = g_zBarEnds[1][flipped][0];
			}
			else if (i == 0)
			{
				bar = g_zBarEnds[1][flipped][1];
			}

			V_DrawFixedPatch(
				x, y,
				FRACUNIT, flags,
				bar, clm
			);
		}
	}
}

/*--------------------------------------------------
	void K_DrawMidVote(void)

		See header file for description.
--------------------------------------------------*/
void K_DrawMidVote(void)
{
	const INT32 id = R_GetViewNumber();
	midVoteGUI_t *gui = NULL;
	boolean pressed = false;
	fixed_t x = INT32_MAX, y = INT32_MAX;

	pressed = G_PlayerInputDown(id, gc_z, 0);
	gui = &g_midVote.gui[id];

	if (gui->slide == 0)
	{
		// Draw the exclamation indicator.
		patch_t *exc = g_exclamation;

		x = 295 * FRACUNIT;
		y = 127 * FRACUNIT;

		if (g_midVote.time < ZVOTE_GUI_SLIDE)
		{
			x += ((ZVOTE_GUI_SLIDE - g_midVote.time) * (ZVOTE_GUI_SLIDE - g_midVote.time)) << (FRACBITS - 1);
			exc = g_exclamationSlide;
		}
		else
		{
			const tic_t spd = 2;
			const tic_t anim = (g_midVote.time - ZVOTE_GUI_SLIDE) / spd;
			const UINT8 frame = anim % (ZVOTE_PATCH_EXC_LOOP + ZVOTE_GUI_SLIDE);

			if (frame < ZVOTE_PATCH_EXC_LOOP)
			{
				if (anim > ZVOTE_GUI_SLIDE)
				{
					exc = g_exclamationLoop[frame];
				}
				else
				{
					exc = g_exclamationStart[frame];
				}
			}
		}

		V_DrawFixedPatch(
			x, y, FRACUNIT,
			V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN,
			exc, NULL
		);
		K_DrawGameControl(
			x/FRACUNIT - 4, y/FRACUNIT + exc->height - 8,
			id, pressed ? "<z_pressed>" : "<z>",
			0, MENU_FONT, V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN
		);
		/*
		K_drawButton(
			x - (4 * FRACUNIT),
			y + ((exc->height - kp_button_z[1][0]->height) * FRACUNIT),
			V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN,
			kp_button_z[1], pressed
		);
		*/
	}
	else
	{
		// Draw the actual vote status
		const fixed_t barHalf = (g_zBar[0]->width * FRACUNIT * (ZVOTE_PATCH_BAR_SEGS - 1)) >> 1;
		const boolean blink = (gametic & 1);
		boolean drawButton = blink;
		boolean drawVotes = blink;

		fixed_t strWidth = 0;

		fixed_t fill = FRACUNIT;
		skincolornum_t fillColor = SKINCOLOR_NONE;

		x = (BASEVIDWIDTH * FRACUNIT) - barHalf;
		y = 144 * FRACUNIT;

		if (gui->slide < ZVOTE_GUI_SLIDE)
		{
			x += ((ZVOTE_GUI_SLIDE - gui->slide) * (ZVOTE_GUI_SLIDE - gui->slide)) << (FRACBITS - 1);
		}

		// Hold bar
		if (g_midVote.end > 0)
		{
			if (g_midVote.endVotes >= g_midVote.endRequired)
			{
				fillColor = SKINCOLOR_GREEN;
			}
			else
			{
				fillColor = SKINCOLOR_RED;
			}
		}
		else
		{
			if (gui->confirm < ZVOTE_GUI_CONFIRM)
			{
				fill = FixedDiv(gui->confirm, ZVOTE_GUI_CONFIRM);
				fillColor = SKINCOLOR_WHITE;
			}
		}
		K_DrawMidVoteBar(
			x - barHalf, y,
			V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN,
			fill, fillColor,
			(id & 1)
		);

		const char *label = K_GetMidVoteLabel(g_midVote.type);

		// Vote main label
		strWidth = V__OneScaleStringWidth(
			FRACUNIT,
			V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN,
			KART_FONT, label
		);

		V__DrawOneScaleString(
			x - (strWidth >> 1),
			y - (18 * FRACUNIT),
			FRACUNIT,
			V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN, NULL,
			KART_FONT, label
		);

		// Vote extra text
		switch (g_midVote.type)
		{
			case MVT_KICK:
			case MVT_MUTE:
			{
				// Draw victim name
				if (g_midVote.victim != NULL)
				{
					strWidth = V__OneScaleStringWidth(
						FRACUNIT,
						V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN,
						TINY_FONT, player_names[g_midVote.victim - players]
					);

					V__DrawOneScaleString(
						x - (strWidth >> 1),
						y + (18 * FRACUNIT),
						FRACUNIT,
						V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN, NULL,
						TINY_FONT, player_names[g_midVote.victim - players]
					);
				}
				break;
			}
			default:
			{
				break;
			}
		}

		// Button
		if (g_midVote.end == 0)
		{
			drawButton = true;
		}

		if (K_PlayerIDAllowedInMidVote(g_localplayers[id]) == false)
		{
			// Player isn't allowed to vote, so don't show it.
			drawButton = false;
		}

		if (drawButton == true)
		{
			K_DrawGameControl(
				x/FRACUNIT-20, y/FRACUNIT + 2, id,
				pressed ? "<z_pressed>" : "<z>",
				0, MENU_FONT, V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN
			);
			/*
			K_drawButton(
				x - (20 * FRACUNIT),
				y - (2 * FRACUNIT),
				V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN,
				kp_button_z[0], pressed
			);
			*/
		}

		// Vote count
		if (g_midVote.end == 0)
		{
			if (gui->confirm == ZVOTE_GUI_CONFIRM || pressed == false)
			{
				drawVotes = true;
			}
		}

		if (drawVotes == true)
		{
			const fixed_t voteY = y + (2 * FRACUNIT);
			fixed_t voteX = x + (8 * FRACUNIT);
			fixed_t voteWidth = 0;
			UINT8 votes = 0;
			UINT8 require = 0;

			if (g_midVote.end > 0)
			{
				votes = g_midVote.endVotes;
				require = g_midVote.endRequired;
			}
			else
			{
				votes = K_CountMidVotes();
				require = K_RequiredMidVotes();
			}

			voteWidth = V__OneScaleStringWidth(
				FRACUNIT,
				V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN,
				OPPRF_FONT, va("%d/%d", votes, require)
			);

			V__DrawOneScaleString(
				voteX - (voteWidth >> 1),
				voteY,
				FRACUNIT,
				V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_SPLITSCREEN, NULL,
				OPPRF_FONT, va("%d/%d", votes, require)
			);
		}
	}
}
