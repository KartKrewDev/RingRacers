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

#define SWITCHTIME TICRATE*5 // cooldown between unforced switches
#define BOREDOMTIME 3*TICRATE/2 // how long until players considered far apart?
#define TRANSFERTIME TICRATE // how long to delay reaction shots?
#define BREAKAWAYDIST 4000 // how *far* until players considered far apart?
#define WALKBACKDIST 600 // how close should a trailing player be before we switch?
#define PINCHDIST 30000 // how close should the leader be to be considered "end of race"?

void K_UpdateDirectorPositions(void);
boolean K_CanSwitchDirector(void);
fixed_t K_GetFinishGap(INT32 leader, INT32 follower);
void K_DirectorSwitch(INT32 player, boolean force);
void K_DirectorFollowAttack(player_t *player, mobj_t *inflictor, mobj_t *source);
void K_UpdateDirector(void);
void K_DirectorForceSwitch(INT32 player, INT32 time);

INT32 cooldown = 0; // how long has it been since we last switched?
INT32 freeze = 0; // when nonzero, fixed switch pending, freeze logic!
INT32 attacker = 0; // who to switch to when freeze delay elapses
INT32 maxdist = 0; // how far is the closest player from finishing?

INT32 sortedplayers[MAXPLAYERS] = {0}; // position-1 goes in, player index comes out. 
INT32 gap[MAXPLAYERS] = {0}; // gap between a given position and their closest pursuer
INT32 boredom[MAXPLAYERS] = {0}; // how long has a given position had no credible attackers?

fixed_t K_GetFinishGap(INT32 leader, INT32 follower) {
    fixed_t dista = players[follower].distancetofinish;
    fixed_t distb = players[leader].distancetofinish;
    if (players[follower].position < players[leader].position) {
        return distb-dista;
    } else {
        return dista-distb;
    }
}

void K_UpdateDirectorPositions(void) {
    INT32 playernum;

    memset(sortedplayers, -1, sizeof(sortedplayers));
	for (playernum = 0; playernum < MAXPLAYERS; ++playernum) {
		if (playeringame[playernum])
            sortedplayers[players[playernum].position-1] = playernum;
    }

    memset(gap, INT32_MAX, sizeof(gap));
    for (playernum = 0; playernum < MAXPLAYERS-1; ++playernum) {
        if (sortedplayers[playernum] == -1 || sortedplayers[playernum+1] == -1)
            break;
        gap[playernum] = P_ScaleFromMap(K_GetFinishGap(sortedplayers[playernum], sortedplayers[playernum+1]), FRACUNIT);
        if (gap[playernum] >= BREAKAWAYDIST)
            boredom[playernum] = min(BOREDOMTIME*2, boredom[playernum]+1);
        else if (boredom[playernum] > 0)
            boredom[playernum]--;
    }

    maxdist = P_ScaleFromMap(players[sortedplayers[0]].distancetofinish, FRACUNIT);
}

boolean K_CanSwitchDirector(void) {
    INT32 *displayplayerp = &displayplayers[0];
    if (players[*displayplayerp].trickpanel > 0)
        return false;
    if (cooldown < SWITCHTIME)
        return false;
    return true;
}

void K_DirectorSwitch(INT32 player, boolean force) {
    if (P_IsDisplayPlayer(&players[player]))
        return;
    if (players[player].exiting)
        return;
    if (!force && !K_CanSwitchDirector())
        return;
    G_ResetView(1, player, true);
    cooldown = 0;
}

void K_DirectorForceSwitch(INT32 player, INT32 time) {
    if (players[player].exiting)
        return;
    attacker = player;
    freeze = time;
}

void K_DirectorFollowAttack(player_t *player, mobj_t *inflictor, mobj_t *source) {
    if (!P_IsDisplayPlayer(player))
        return;
    if (inflictor && inflictor->player)
        K_DirectorForceSwitch(inflictor->player-players, TRANSFERTIME);
    if (source && source->player)
        K_DirectorForceSwitch(source->player-players, TRANSFERTIME);
}

void K_DrawDirectorDebugger(void) {
    INT32 playernum;
    INT32 leader;
    INT32 follower;
    INT32 ytxt;

    if (!cv_kartdebugdirector.value)
        return;

    V_DrawThinString(10, 0, V_70TRANS, va("PLACE"));
    V_DrawThinString(40, 0, V_70TRANS, va("CONF?"));
    V_DrawThinString(80, 0, V_70TRANS, va("GAP"));
    V_DrawThinString(120, 0, V_70TRANS, va("BORED"));
    V_DrawThinString(150, 0, V_70TRANS, va("COOLDOWN: %d", cooldown));
    V_DrawThinString(230, 0, V_70TRANS, va("MAXDIST: %d", maxdist));

    for (playernum = 0; playernum < MAXPLAYERS-1; ++playernum) {
        ytxt = 10*playernum+10;
        leader = sortedplayers[playernum];
        follower = sortedplayers[playernum+1];
        if (leader == -1 || follower == -1)
            break;

        V_DrawThinString(10, ytxt, V_70TRANS, va("%d", playernum));
        V_DrawThinString(20, ytxt, V_70TRANS, va("%d", playernum+1));
        if (players[leader].positiondelay)
            V_DrawThinString(40, ytxt, V_70TRANS, va("NG"));
        V_DrawThinString(80, ytxt, V_70TRANS, va("%d", gap[playernum]));
        if (boredom[playernum] >= BOREDOMTIME)
            V_DrawThinString(120, ytxt, V_70TRANS, va("BORED"));
        else
            V_DrawThinString(120, ytxt, V_70TRANS, va("%d", boredom[playernum]));
        V_DrawThinString(150, ytxt, V_70TRANS, va("%s", player_names[leader]));
        V_DrawThinString(230, ytxt, V_70TRANS, va("%s", player_names[follower]));
    }
}

void K_UpdateDirector(void) {
    INT32 *displayplayerp = &displayplayers[0];
    INT32 targetposition;

    if (!cv_director.value)
        return;

    K_UpdateDirectorPositions();
    cooldown++;

    // handle pending forced switches
    if (freeze > 0) {
        if (freeze == 1)
            K_DirectorSwitch(attacker, true);
        freeze--;
        return;
    }

    // aaight, time to walk through the standings to find the first interesting pair
    for(targetposition = 0; targetposition < MAXPLAYERS-1; targetposition++) {
        INT32 target;

        // you are out of players, try again
        if (sortedplayers[targetposition+1] == -1)
            break;

        // pair too far apart? try the next one
        if (boredom[targetposition] >= BOREDOMTIME)
            continue;

        // pair finished? try the next one
        if (players[sortedplayers[targetposition+1]].exiting)
            continue;

        // don't risk switching away from forward pairs at race end, might miss something!
        if (maxdist > PINCHDIST) {
            // if the "next" player is close enough, they should be able to see everyone fine!
            // walk back through the standings to find a vantage that gets everyone in frame.
            // (also creates a pretty cool effect w/ overtakes at speed)
            while (targetposition < MAXPLAYERS && gap[targetposition+1] < WALKBACKDIST) {
                targetposition++;
            }
        }

        target = sortedplayers[targetposition+1];

        // if we're certain the back half of the pair is actually in this position, try to switch
        if (*displayplayerp != target && !players[target].positiondelay)
            K_DirectorSwitch(target, false);
        // even if we're not certain, if we're certain we're watching the WRONG player, try to switch
        if (players[*displayplayerp].position != targetposition && !players[target].positiondelay)
            K_DirectorSwitch(target, false);

        break;
    }
}