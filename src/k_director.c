// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
/// \file  k_director.c
/// \brief SRB2kart automatic spectator camera.

#include "k_kart.h"
#include "k_respawn.h"
#include "doomdef.h"
#include "g_game.h"
#include "v_video.h"
#include "k_director.h"
#include "d_netcmd.h"
#include "p_local.h"
#include "g_party.h"

#define SWITCHTIME TICRATE * 5		// cooldown between unforced switches
#define BOREDOMTIME 3 * TICRATE / 2 // how long until players considered far apart?
#define TRANSFERTIME TICRATE		// how long to delay reaction shots?
#define BREAKAWAYDIST 4000			// how *far* until players considered far apart?
#define WALKBACKDIST 600			// how close should a trailing player be before we switch?
#define PINCHDIST 30000				// how close should the leader be to be considered "end of race"?

struct directorinfo directorinfo;

void K_InitDirector(void)
{
	INT32 playernum;

	directorinfo.active = false;
	directorinfo.cooldown = SWITCHTIME;
	directorinfo.freeze = 0;
	directorinfo.attacker = 0;
	directorinfo.maxdist = 0;

	for (playernum = 0; playernum < MAXPLAYERS; playernum++)
	{
		directorinfo.sortedplayers[playernum] = -1;
		directorinfo.gap[playernum] = INT32_MAX;
		directorinfo.boredom[playernum] = 0;
	}
}

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

static void K_UpdateDirectorPositions(void)
{
	INT32 playernum;
	INT32 position;
	player_t* target;

	memset(directorinfo.sortedplayers, -1, sizeof(directorinfo.sortedplayers));

	for (playernum = 0; playernum < MAXPLAYERS; playernum++)
	{
		target = &players[playernum];

		if (playeringame[playernum] && !target->spectator && target->position > 0)
		{
			directorinfo.sortedplayers[target->position - 1] = playernum;
		}
	}

	for (position = 0; position < MAXPLAYERS - 1; position++)
	{
		directorinfo.gap[position] = INT32_MAX;

		if (directorinfo.sortedplayers[position] == -1 || directorinfo.sortedplayers[position + 1] == -1)
		{
			continue;
		}

		directorinfo.gap[position] = P_ScaleFromMap(K_GetFinishGap(directorinfo.sortedplayers[position], directorinfo.sortedplayers[position + 1]), FRACUNIT);

		if (directorinfo.gap[position] >= BREAKAWAYDIST)
		{
			directorinfo.boredom[position] = min(BOREDOMTIME * 2, directorinfo.boredom[position] + 1);
		}
		else if (directorinfo.boredom[position] > 0)
		{
			directorinfo.boredom[position]--;
		}
	}

	directorinfo.maxdist = P_ScaleFromMap(players[directorinfo.sortedplayers[0]].distancetofinish, FRACUNIT);
}

static boolean K_CanSwitchDirector(void)
{
	INT32 *displayplayerp = &displayplayers[0];

	if (players[*displayplayerp].trickpanel > 0)
	{
		return false;
	}

	if (directorinfo.cooldown > 0)
	{
		return false;
	}

	return true;
}

static void K_DirectorSwitch(INT32 player, boolean force)
{
	if (!directorinfo.active)
	{
		return;
	}

	if (P_IsDisplayPlayer(&players[player]))
	{
		return;
	}

	if (players[player].exiting)
	{
		return;
	}

	if (!force && !K_CanSwitchDirector())
	{
		return;
	}

	G_ResetView(1, player, true);
	directorinfo.cooldown = SWITCHTIME;
}

static void K_DirectorForceSwitch(INT32 player, INT32 time)
{
	if (players[player].exiting)
	{
		return;
	}

	directorinfo.attacker = player;
	directorinfo.freeze = time;
}

void K_DirectorFollowAttack(player_t *player, mobj_t *inflictor, mobj_t *source)
{
	if (!P_IsDisplayPlayer(player))
	{
		return;
	}

	if (inflictor && inflictor->player)
	{
		K_DirectorForceSwitch(inflictor->player - players, TRANSFERTIME);
	}
	else if (source && source->player)
	{
		K_DirectorForceSwitch(source->player - players, TRANSFERTIME);
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

	V_DrawThinString(10, 0, V_70TRANS, va("PLACE"));
	V_DrawThinString(40, 0, V_70TRANS, va("CONF?"));
	V_DrawThinString(80, 0, V_70TRANS, va("GAP"));
	V_DrawThinString(120, 0, V_70TRANS, va("BORED"));
	V_DrawThinString(150, 0, V_70TRANS, va("COOLDOWN: %d", directorinfo.cooldown));
	V_DrawThinString(230, 0, V_70TRANS, va("MAXDIST: %d", directorinfo.maxdist));

	for (position = 0; position < MAXPLAYERS - 1; position++)
	{
		ytxt = 10 * (position + 1);
		leader = directorinfo.sortedplayers[position];
		follower = directorinfo.sortedplayers[position + 1];

		if (leader == -1 || follower == -1)
			break;

		V_DrawThinString(10, ytxt, V_70TRANS, va("%d", position));
		V_DrawThinString(20, ytxt, V_70TRANS, va("%d", position + 1));

		if (players[leader].positiondelay)
		{
			V_DrawThinString(40, ytxt, V_70TRANS, va("NG"));
		}

		V_DrawThinString(80, ytxt, V_70TRANS, va("%d", directorinfo.gap[position]));

		if (directorinfo.boredom[position] >= BOREDOMTIME)
		{
			V_DrawThinString(120, ytxt, V_70TRANS, va("BORED"));
		}
		else
		{
			V_DrawThinString(120, ytxt, V_70TRANS, va("%d", directorinfo.boredom[position]));
		}

		V_DrawThinString(150, ytxt, V_70TRANS, va("%s", player_names[leader]));
		V_DrawThinString(230, ytxt, V_70TRANS, va("%s", player_names[follower]));
	}
}

void K_UpdateDirector(void)
{
	INT32 *displayplayerp = &displayplayers[0];
	INT32 targetposition;

	K_UpdateDirectorPositions();

	if (directorinfo.cooldown > 0) {
		directorinfo.cooldown--;
	}

	// handle pending forced switches
	if (directorinfo.freeze > 0)
	{
		if (!(--directorinfo.freeze))
			K_DirectorSwitch(directorinfo.attacker, true);

		return;
	}

	// if there's only one player left in the list, just switch to that player
	if (directorinfo.sortedplayers[0] != -1 && directorinfo.sortedplayers[1] == -1)
	{
		K_DirectorSwitch(directorinfo.sortedplayers[0], false);
		return;
	}

	// aaight, time to walk through the standings to find the first interesting pair
	// NB: targetposition/sortedplayers is 0-indexed, aiming at the "back half" of a given pair by default.
	// we adjust for this when comparing to player->position or when looking at the leading player, Don't Freak Out
	for (targetposition = 1; targetposition < MAXPLAYERS; targetposition++)
	{
		INT32 target;

		// you are out of players, try again
		if (directorinfo.sortedplayers[targetposition] == -1)
		{
			break;
		}

		// pair too far apart? try the next one
		if (directorinfo.boredom[targetposition - 1] >= BOREDOMTIME)
		{
			continue;
		}

		// pair finished? try the next one
		if (players[directorinfo.sortedplayers[targetposition]].exiting)
		{
			continue;
		}

		// don't risk switching away from forward pairs at race end, might miss something!
		if (directorinfo.maxdist > PINCHDIST)
		{
			// if the "next" player is close enough, they should be able to see everyone fine!
			// walk back through the standings to find a vantage that gets everyone in frame.
			// (also creates a pretty cool effect w/ overtakes at speed)
			while (targetposition < MAXPLAYERS && directorinfo.gap[targetposition] < WALKBACKDIST)
			{
				targetposition++;
			}
		}

		target = directorinfo.sortedplayers[targetposition];

		// stop here since we're already viewing this player
		if (*displayplayerp == target)
		{
			break;
		}

		// if this is a splitscreen player, try next pair
		if (P_IsDisplayPlayer(&players[target]))
		{
			continue;
		}

		// if we're certain the back half of the pair is actually in this position, try to switch
		if (!players[target].positiondelay)
		{
			K_DirectorSwitch(target, false);
		}

		// even if we're not certain, if we're certain we're watching the WRONG player, try to switch
		if (players[*displayplayerp].position != targetposition+1 && !players[*displayplayerp].positiondelay)
		{
			K_DirectorSwitch(target, false);
		}

		break;
	}
}

void K_ToggleDirector(boolean active)
{
	if (directorinfo.active != active)
	{
		directorinfo.cooldown = 0; // switch immediately
	}

	directorinfo.active = active;
}

boolean K_DirectorIsAvailable(UINT8 viewnum)
{
	return viewnum <= r_splitscreen && viewnum < G_PartySize(consoleplayer) &&
		displayplayers[viewnum] != G_PartyMember(consoleplayer, viewnum);
}
