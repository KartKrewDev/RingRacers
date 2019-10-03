/// \file  k_pwrlv.c
/// \brief SRB2Kart Power Levels

#include "k_pwrlv.h"
#include "d_netcmd.h"
#include "g_game.h"
#include "s_sound.h"
#include "m_random.h"
#include "m_cond.h" // M_UpdateUnlockablesAndExtraEmblems
#include "p_tick.h" // leveltime

// Online rankings for the main gametypes.
// This array is saved to the gamedata.
UINT16 vspowerlevel[PWRLV_NUMTYPES];

// Client-sided calculations done for Power Levels.
// This is done so that clients will never be able to hack someone else's score over the server.
UINT16 clientpowerlevels[MAXPLAYERS][PWRLV_NUMTYPES];

// Which players spec-scummed, and their power level before scumming.
// On race finish, everyone is considered to have "won" against these people.
INT16 nospectategrief[MAXPLAYERS]; 

// Game setting scrambles based on server Power Level
SINT8 speedscramble = -1;
SINT8 encorescramble = -1;

void K_ClearClientPowerLevels(void)
{
	UINT8 i, j;
	for (i = 0; i < MAXPLAYERS; i++)
		for (j = 0; j < PWRLV_NUMTYPES; j++)
			clientpowerlevels[i][j] = 0;
}

// Adapted from this: http://wiki.tockdom.com/wiki/Player_Rating
INT16 K_CalculatePowerLevelInc(INT16 diff)
{
	INT16 control[10] = {0,0,0,1,8,50,125,125,125,125};
	fixed_t increment = 0;
	fixed_t x;
	UINT8 j;

#define MAXDIFF (PWRLVRECORD_MAX - 1)
	if (diff > MAXDIFF)
		diff = MAXDIFF;
	if (diff < -MAXDIFF)
		diff = -MAXDIFF;
#undef MAXDIFF

	x = ((diff-2)<<FRACBITS) / PWRLVRECORD_DEF;

	for (j = 3; j < 10; j++) // Just skipping to 3 since 0 thru 2 will always just add 0...
	{
		fixed_t f = abs(x - ((j-4)<<FRACBITS));
		fixed_t add;

		if (f >= (2<<FRACBITS))
		{
			continue; //add = 0;
		}
		else if (f >= (1<<FRACBITS))
		{
			fixed_t f2 = (2<<FRACBITS) - f;
			add = FixedMul(FixedMul(f2, f2), f2) / 6;
		}
		else
		{
			add = ((3*FixedMul(FixedMul(f, f), f)) - (6*FixedMul(f, f)) + (4<<FRACBITS)) / 6;
		}

		increment += (add * control[j]);
	}

	return (INT16)(increment >> FRACBITS);
}

INT16 K_CalculatePowerLevelAvg(void)
{
	fixed_t avg = 0;
	UINT8 div = 0;
	SINT8 t = PWRLV_DISABLED;
	UINT8 i;

	if (!netgame || !cv_kartusepwrlv.value)
		return 0; // No average.

	if (G_RaceGametype())
		t = PWRLV_RACE;
	else if (G_BattleGametype())
		t = PWRLV_BATTLE;

	if (t == PWRLV_DISABLED)
		return 0; // Hmm?!

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator
			|| clientpowerlevels[i][t] == 0) // splitscreen player
			continue;

		avg += clientpowerlevels[i][t];
		div++;
	}

	if (!div)
		return 0; // No average.

	avg /= div;

	return (INT16)(avg >> FRACBITS);
}

// -- K_UpdatePowerLevels could not be moved here due to usage of y_data, unfortunately. --

void K_SetPowerLevelScrambles(SINT8 powertype)
{
	switch (powertype)
	{
		case PWRLV_RACE:
			if (cv_kartspeed.value == -1 || cv_kartencore.value == -1)
			{
				boolean hardmode = false;
				boolean encore = false;
				INT16 avg = 0, min = 0;
				UINT8 i, t = 0;

				avg = K_CalculatePowerLevelAvg();

				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (min == 0 || clientpowerlevels[i][0] < min)
						min = clientpowerlevels[i][0];
				}

				if (min >= 6000)
				{
					if (avg >= 8000)
						t = 4;
					else
						t = 3;
				}
				else if (min >= 4000)
				{
					if (avg >= 5000)
						t = 3;
					else
						t = 2;
				}
				else if (min >= 2500)
				{
					if (avg >= 3000)
						t = 2;
					else
						t = 1;
				}
				else if (min >= 500)
				{
					if (avg >= 2000)
						t = 1;
					else
						t = 0;
				}
				else
					t = 0;

				switch (t)
				{
					case 4:
						hardmode = encore = true;
						break;
					case 3:
						hardmode = true;
						encore = M_RandomChance(FRACUNIT>>1);
						break;
					case 2:
						hardmode = M_RandomChance((7<<FRACBITS)/10);
						encore = M_RandomChance(FRACUNIT>>2);
						break;
					case 1:
						hardmode = M_RandomChance((3<<FRACBITS)/10);
						encore = false;
						break;
					case 0:
					default:
						hardmode = encore = false;
						break;
				}

				if (cv_kartspeed.value == -1)
					speedscramble = (hardmode ? 2 : 1);
				else
					speedscramble = -1;

				if (cv_kartencore.value == -1)
					encorescramble = (encore ? 1 : 0);
				else
					encorescramble = -1;
			}
			break;
		default:
			break;
	}
}

void K_PlayerForfeit(UINT8 playernum, boolean pointloss)
{
	UINT8 p = 0;
	INT32 powertype = PWRLV_DISABLED;
	UINT16 yourpower = PWRLVRECORD_DEF;
	UINT16 theirpower = PWRLVRECORD_DEF;
	INT16 diff = 0; // Loser PWR.LV - Winner PWR.LV
	INT16 inc = 0;
	UINT8 i;

	// power level & spectating is netgames only
	if (!netgame)
		return;

	// This server isn't using power levels anyway!
	if (!cv_kartusepwrlv.value)
		return;

	// Hey, I just got here!
	if (players[playernum].jointime <= 1)
		return;

	// 20 sec into the match counts as a forfeit -- automatic loss against every other player in the match.
	if (gamestate != GS_LEVEL || leveltime <= starttime+(20*TICRATE))
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
			p++;
	}

	if (p < 2) // no players
		return;

	if (G_RaceGametype())
		powertype = PWRLV_RACE;
	else if (G_BattleGametype())
		powertype = PWRLV_BATTLE;

	if (powertype == PWRLV_DISABLED) // No power type?!
		return;

	if (clientpowerlevels[playernum][powertype] == 0) // splitscreen guests don't record power level changes
		return;
	yourpower = clientpowerlevels[playernum][powertype];

	// Set up the point compensation.
	nospectategrief[playernum] = yourpower;

	if (!pointloss) // This is set for stuff like sync-outs, which shouldn't be so harsh on the victim!
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (i == playernum)
			continue;

		theirpower = PWRLVRECORD_DEF;
		if (clientpowerlevels[i][powertype] != 0) // No power level acts as 5000 (used for splitscreen guests)
			theirpower = clientpowerlevels[i][powertype];

		diff = yourpower - theirpower;
		inc -= K_CalculatePowerLevelInc(diff);
	}

	if (inc == 0) // No change.
		return;

	if (yourpower + inc > PWRLVRECORD_MAX) // I mean... we're subtracting... but y'know how it is :V
		inc -= ((yourpower + inc) - PWRLVRECORD_MAX);
	if (yourpower + inc < PWRLVRECORD_MIN)
		inc -= ((yourpower + inc) - PWRLVRECORD_MIN);

	clientpowerlevels[playernum][powertype] += inc;

	if (playernum == consoleplayer)
	{
		vspowerlevel[powertype] = clientpowerlevels[playernum][powertype];
		if (M_UpdateUnlockablesAndExtraEmblems(true))
			S_StartSound(NULL, sfx_ncitem);
		G_SaveGameData(true); // save your punishment!
	}
}
