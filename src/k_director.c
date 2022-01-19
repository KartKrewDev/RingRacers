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

#define SWITCHTIME  TICRATE*5
#define DEBOUNCETIME   TICRATE*2
#define BOREDOMTIME 3*TICRATE/2
#define TRANSFERTIME    TICRATE
#define BREAKAWAYDIST   4000
#define CONFUSINGDIST   250

INT32 FindPlayerByPlace(INT32 place);
void K_UpdateDirectorPositions(void);
boolean K_CanSwitchDirector(void);
fixed_t K_GetFinishGap(INT32 leader, INT32 follower);
void K_DirectorSwitch(INT32 player);
void K_DirectorFollowAttack(player_t *player, mobj_t *inflictor, mobj_t *source);
void K_UpdateDirector(void);

INT32 cooldown = 0;
INT32 bored = 0;
INT32 positions[MAXPLAYERS] = {0};
INT32 confidence[MAXPLAYERS] = {0};

INT32 freeze = 0;
INT32 attacker = 0;

INT32 FindPlayerByPlace(INT32 place)
{
	INT32 playernum;
	for (playernum = 0; playernum < MAXPLAYERS; ++playernum)
		if (playeringame[playernum])
	{
		if (players[playernum].position == place)
		{
			return playernum;
		}
	}
	return -1;
}

void K_UpdateDirectorPositions(void) {
    INT32 playernum;
	for (playernum = 0; playernum < MAXPLAYERS; ++playernum)
		if (playeringame[playernum])
	{
		if (players[playernum].position == positions[playernum]) {
            confidence[playernum]++;
        }
        else {
			confidence[playernum] = 0;
            positions[playernum] = players[playernum].position;
		}
	} else {
        positions[playernum] = 0;
        confidence[playernum] = 0;
    }
}

boolean K_CanSwitchDirector(void) {
    INT32 *displayplayerp = &displayplayers[0];
    if (players[*displayplayerp].trickpanel > 0) {
        // CONS_Printf("NO SWITCH, panel");
        return false;
    }
    return cooldown >= SWITCHTIME;
}

fixed_t K_GetFinishGap(INT32 leader, INT32 follower) {
    fixed_t dista = players[follower].distancetofinish;
    fixed_t distb = players[leader].distancetofinish;
    if (players[follower].position < players[leader].position) {
        return distb-dista;
    } else {
        return dista-distb;
    }
}

void K_DirectorSwitch(INT32 player) {
    if (!K_CanSwitchDirector())
        return;
    // CONS_Printf("SWITCHING: %s\n", player_names[player]);
    G_ResetView(1, player, true);
    cooldown = 0;
}

void K_DirectorFollowAttack(player_t *player, mobj_t *inflictor, mobj_t *source) {
    if (!P_IsDisplayPlayer(player))
        return;
    if (inflictor && inflictor->player) {
        // CONS_Printf("INFLICTOR SET\n");
        attacker = inflictor->player-players;
        freeze = TRANSFERTIME;
        cooldown = SWITCHTIME;
    }
    if (source && source->player) {
        // CONS_Printf("SOURCE SET\n");
        attacker = source->player-players;
        freeze = TRANSFERTIME;
        cooldown = SWITCHTIME;
    }
    // CONS_Printf("FOLLOW ATTACK\n");
    // CONS_Printf(M_GetText("%s attacked\n"), player_names[attacker]);
}

void K_UpdateDirector(void) {
    INT32 *displayplayerp = &displayplayers[0];
    INT32 targetposition;

    K_UpdateDirectorPositions();
    cooldown++;

    if (freeze >= 1) {
        // CONS_Printf("FROZEN\n");
        if (freeze == 1) {
            K_DirectorSwitch(attacker);
            // CONS_Printf("ATTACKER SWITCH\n");
        }
        freeze--;
        return;
    }

    for(targetposition = 2; targetposition < MAXPLAYERS; targetposition++) {
        INT32 leader = FindPlayerByPlace(targetposition - 1);
        INT32 follower = FindPlayerByPlace(targetposition);
        fixed_t gap = K_GetFinishGap(leader, follower);

        /*
        CONS_Printf("Eval %d GP %d CD %d FC %d DC %d\n", 
           targetposition, gap, cooldown, confidence[follower], confidence[*displayplayerp]);
        */

        if (gap > BREAKAWAYDIST) {
            bored++;
            if (bored > BOREDOMTIME) {
                // CONS_Printf("BREAKAWAY, falling back to %d\n", targetposition);
                continue;
            }
        } else if (bored > 0) {
            bored--;
        }

        /*
        if (gap < CONFUSINGDIST && *displayplayerp == leader) {
            CONS_Printf("No switch: too close\n");
            break;
        }
        */

        if (*displayplayerp != follower && confidence[follower] > DEBOUNCETIME) {
            K_DirectorSwitch(FindPlayerByPlace(targetposition));
        }
        if (positions[*displayplayerp] != targetposition && confidence[*displayplayerp] > DEBOUNCETIME) {
            K_DirectorSwitch(FindPlayerByPlace(targetposition));
        }

        break;
    }
}