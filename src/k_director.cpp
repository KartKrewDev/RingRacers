// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez.
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_director.c
/// \brief SRB2kart automatic spectator camera.

#include <algorithm>
#include <array>

#include "cxxutil.hpp"

#include "k_kart.h"
#include "k_respawn.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "v_video.h"
#include "k_director.h"
#include "d_netcmd.h"
#include "p_local.h"
#include "g_party.h"
#include "command.h"

extern "C" consvar_t cv_devmode_screen;

static bool race_rules()
{
	return gametyperules & GTR_CIRCUIT;
}

#define SWITCHTIME TICRATE * 5			// cooldown between unforced switches
#define BOREDOMTIME 3 * TICRATE / 2 	// how long until players considered far apart?
#define TRANSFERTIME TICRATE			// how long to delay reaction shots?
#define BREAKAWAYDIST 4000				// how *far* until players considered far apart?
#define WALKBACKDIST 600				// how close should a trailing player be before we switch?
#define PINCHDIST 30000					// how close should the leader be to be considered "end of race"?

static fixed_t K_GetFinishGap(INT32 leader, INT32 follower)
{
	fixed_t dista = players[follower].distancetofinish;
	fixed_t distb = players[leader].distancetofinish;

	if (players[follower].position < players[leader].position)
	{
		return distb - dista;
	}
	else
	{
		return dista - distb;
	}
}

struct DirectorInfo
{
    UINT8 viewnum;						// which screen does this director apply to?
    boolean active = false;				// is view point switching enabled?
    tic_t cooldown = SWITCHTIME;		// how long has it been since we last switched?
    tic_t freeze = 0;					// when nonzero, fixed switch pending, freeze logic!
    INT32 attacker = 0;					// who to switch to when freeze delay elapses
    INT32 maxdist = 0;					// how far is the closest player from finishing?

    struct PlayerStat
	{
		INT32 sorted = -1;				// position-1 goes in, player index comes out.
		INT32 gap = INT32_MAX;			// gap between a given position and their closest pursuer
		INT32 boredom = 0;				// how long has a given position had no credible attackers?
	}
	playerstat[MAXPLAYERS];

	DirectorInfo(UINT8 viewnum_) : viewnum(viewnum_) {}

	INT32 viewplayernum() const { return displayplayers[viewnum]; }
	player_t* viewplayer() const { return &players[viewplayernum()]; }

	void update()
	{
		INT32 targetposition;

		update_positions();

		if (cooldown > 0) {
			cooldown--;
		}

		// handle pending forced switches
		if (freeze > 0)
		{
			if (!(--freeze))
				change(attacker, true);

			return;
		}

		// if there's only one player left in the list, just switch to that player
		if (playerstat[0].sorted != -1 && (playerstat[1].sorted == -1 ||
				// TODO: Battle; I just threw this together quick. Focus on leader.
				!race_rules()))
		{
			change(playerstat[0].sorted, false);
			return;
		}

		// aaight, time to walk through the standings to find the first interesting pair
		// NB: targetposition/PlayerStat::sorted is 0-indexed, aiming at the "back half" of a given pair by default.
		// we adjust for this when comparing to player->position or when looking at the leading player, Don't Freak Out
		for (targetposition = 1; targetposition < MAXPLAYERS; targetposition++)
		{
			INT32 target;

			// you are out of players, try again
			if (playerstat[targetposition].sorted == -1)
			{
				break;
			}

			// pair too far apart? try the next one
			if (playerstat[targetposition - 1].boredom >= BOREDOMTIME)
			{
				continue;
			}

			// pair finished? try the next one
			if (players[playerstat[targetposition].sorted].exiting ||
				// Battle: player was killed by Overtime Barrier
				(players[playerstat[targetposition].sorted].pflags & PF_ELIMINATED))
			{
				continue;
			}

			// don't risk switching away from forward pairs at race end, might miss something!
			if (maxdist > PINCHDIST)
			{
				// if the "next" player is close enough, they should be able to see everyone fine!
				// walk back through the standings to find a vantage that gets everyone in frame.
				// (also creates a pretty cool effect w/ overtakes at speed)
				while (targetposition < MAXPLAYERS && playerstat[targetposition].gap < WALKBACKDIST)
				{
					targetposition++;
				}
			}

			target = playerstat[targetposition].sorted;

			// stop here since we're already viewing this player
			if (viewplayernum() == target)
			{
				break;
			}

			// if we're certain the back half of the pair is actually in this position, try to switch
			if (!players[target].positiondelay)
			{
				change(target, false);
			}

			// even if we're not certain, if we're certain we're watching the WRONG player, try to switch
			if (viewplayer()->position != targetposition+1 && !viewplayer()->positiondelay)
			{
				change(target, false);
			}

			break;
		}
	}

	void force_switch(INT32 player, INT32 time)
	{
		if (players[player].exiting)
		{
			return;
		}

		attacker = player;
		freeze = time;
	}

private:
	void update_positions()
	{
		INT32 playernum;
		INT32 position;
		player_t* target;

		for (PlayerStat& stat : playerstat)
		{
			stat.sorted = -1;
		}

		for (playernum = 0; playernum < MAXPLAYERS; playernum++)
		{
			target = &players[playernum];

			if (playeringame[playernum] && !target->spectator && target->position > 0)
			{
				playerstat[target->position - 1].sorted = playernum;
			}
		}

		for (position = 0; position < MAXPLAYERS - 1; position++)
		{
			playerstat[position].gap = INT32_MAX;

			if (playerstat[position].sorted == -1 || playerstat[position + 1].sorted == -1)
			{
				continue;
			}

			playerstat[position].gap = P_ScaleFromMap(
				K_GetFinishGap(playerstat[position].sorted, playerstat[position + 1].sorted),
				FRACUNIT
			);

			if (playerstat[position].gap >= BREAKAWAYDIST)
			{
				playerstat[position].boredom = std::min<INT32>(BOREDOMTIME * 2, playerstat[position].boredom + 1);
			}
			else if (playerstat[position].boredom > 0)
			{
				playerstat[position].boredom--;
			}
		}

		if (playerstat[0].sorted == -1)
		{
			maxdist = -1;
			return;
		}

		maxdist = P_ScaleFromMap(players[playerstat[0].sorted].distancetofinish, FRACUNIT);
	}

	bool can_change() const
	{
		if (viewplayer()->trickpanel != TRICKSTATE_NONE)
		{
			return false;
		}

		if (cooldown > 0)
		{
			return false;
		}

		return true;
	}

	void change(INT32 player, boolean force)
	{
		if (!active)
		{
			return;
		}

		if (players[player].exiting)
		{
			return;
		}

		if (!force && !can_change())
		{
			return;
		}

		G_ResetView(1 + viewnum, player, true);
		cooldown = SWITCHTIME;
	}
};

struct DirectorInfoManager
{
	DirectorInfo& operator [](UINT8 viewnum)
	{
		SRB2_ASSERT(viewnum < MAXSPLITSCREENPLAYERS);
		return info_[viewnum];
	}

	auto begin() { return info_.begin(); }
	auto end() { return std::next(begin(), r_splitscreen + 1); }

private:
	static_assert(MAXSPLITSCREENPLAYERS == 4);
	std::array<DirectorInfo, MAXSPLITSCREENPLAYERS> info_{0, 1, 2, 3};
}
g_directorinfo;

void K_InitDirector(void)
{
	g_directorinfo = {};
}

void K_DirectorFollowAttack(player_t *player, mobj_t *inflictor, mobj_t *source)
{
	for (DirectorInfo& directorinfo : g_directorinfo)
	{
		if (directorinfo.viewplayer() != player)
		{
			continue;
		}

		if (inflictor && inflictor->player)
		{
			directorinfo.force_switch(inflictor->player - players, TRANSFERTIME);
		}
		else if (source && source->player)
		{
			directorinfo.force_switch(source->player - players, TRANSFERTIME);
		}
	}
}

void K_DrawDirectorDebugger(void)
{
	INT32 position;
	INT32 leader;
	INT32 follower;
	INT32 ytxt;

	if (!cv_kartdebugdirector.value)
	{
		return;
	}

	DirectorInfo& directorinfo = g_directorinfo[cv_devmode_screen.value];

	V_DrawThinString(10, 0, V_70TRANS, va("PLACE"));
	V_DrawThinString(40, 0, V_70TRANS, va("CONF?"));
	V_DrawThinString(80, 0, V_70TRANS, va("GAP"));
	V_DrawThinString(120, 0, V_70TRANS, va("BORED"));
	V_DrawThinString(150, 0, V_70TRANS, va("COOLDOWN: %d", directorinfo.cooldown));
	V_DrawThinString(230, 0, V_70TRANS, va("MAXDIST: %d", directorinfo.maxdist));

	for (position = 0; position < MAXPLAYERS - 1; position++)
	{
		ytxt = 10 * (position + 1);
		leader = directorinfo.playerstat[position].sorted;
		follower = directorinfo.playerstat[position + 1].sorted;

		if (leader == -1 || follower == -1)
			break;

		V_DrawThinString(10, ytxt, V_70TRANS, va("%d", position));
		V_DrawThinString(20, ytxt, V_70TRANS, va("%d", position + 1));

		if (players[leader].positiondelay)
		{
			V_DrawThinString(40, ytxt, V_70TRANS, va("NG"));
		}

		V_DrawThinString(80, ytxt, V_70TRANS, va("%d", directorinfo.playerstat[position].gap));

		if (directorinfo.playerstat[position].boredom >= BOREDOMTIME)
		{
			V_DrawThinString(120, ytxt, V_70TRANS, va("BORED"));
		}
		else
		{
			V_DrawThinString(120, ytxt, V_70TRANS, va("%d", directorinfo.playerstat[position].boredom));
		}

		V_DrawThinString(150, ytxt, V_70TRANS, va("%s", player_names[leader]));
		V_DrawThinString(230, ytxt, V_70TRANS, va("%s", player_names[follower]));
	}
}

void K_UpdateDirector(void)
{
	for (DirectorInfo& directorinfo : g_directorinfo)
	{
		directorinfo.update();
	}
}

void K_ToggleDirector(UINT8 viewnum, boolean active)
{
	DirectorInfo& directorinfo = g_directorinfo[viewnum];

	if (directorinfo.active != active)
	{
		directorinfo.cooldown = 0; // switch immediately
	}

	directorinfo.active = active;
}

boolean K_DirectorIsEnabled(UINT8 viewnum)
{
	return g_directorinfo[viewnum].active;
}

boolean K_DirectorIsAvailable(UINT8 viewnum)
{
	return viewnum <= r_splitscreen && viewnum < G_PartySize(consoleplayer) &&
		displayplayers[viewnum] != G_PartyMember(consoleplayer, viewnum);
}
