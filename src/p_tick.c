// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_tick.c
/// \brief Archiving: SaveGame I/O, Thinker, Ticker

#include "doomstat.h"
#include "d_main.h"
#include "g_game.h"
#include "g_input.h"
#include "p_local.h"
#include "z_zone.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "p_polyobj.h"
#include "m_random.h"
#include "m_cond.h" // gamedata->playtime
#include "lua_script.h"
#include "lua_hook.h"
#include "m_perfstats.h"
#include "i_system.h" // I_GetPreciseTime
#include "i_video.h" // rendermode
#include "r_main.h"
#include "r_fps.h"
#include "d_clisrv.h" // UpdateChallenges
#include "p_link.h"

// Object place
#include "m_cheat.h"

// SRB2Kart
#include "k_kart.h"
#include "k_race.h"
#include "k_battle.h"
#include "k_boss.h"
#include "k_waypoint.h"
#include "k_director.h"
#include "k_specialstage.h"
#include "acs/interface.h"
#include "k_objects.h"
#include "music.h"
#include "k_dialogue.h"
#include "m_easing.h"
#include "k_hud.h" // messagetimer
#include "k_endcam.h"

#include "lua_profile.h"

#ifdef PARANOIA
#include "deh_tables.h" // MOBJTYPE_LIST
#endif

tic_t leveltime;
boolean thinkersCompleted;

UINT32 thinker_era = 0;

static boolean g_freezeCheat;
static boolean g_freezeLevel;

boolean P_LevelIsFrozen(void)
{
	return (g_freezeLevel || g_freezeCheat || K_EndCameraIsFreezing());
}

boolean P_FreezeCheat(void)
{
	return (g_freezeLevel || g_freezeCheat || K_EndCameraIsFreezing());
}

void P_SetFreezeCheat(boolean value)
{
	g_freezeCheat = value;
}

void P_SetFreezeLevel(boolean value)
{
	g_freezeLevel = value;
}

boolean P_MobjIsFrozen(mobj_t *mobj)
{
	if (g_freezeCheat == true)
	{
		// freeze cheat
		switch (mobj->type)
		{
			case MT_PLAYER:
			{
				break;
			}
			default:
			{
				return true;
			}
		}
	}

	if (g_freezeLevel == true || K_EndCameraIsFreezing())
	{
		// level totally frozen
		return true;
	}

	if ((mobj->eflags & MFE_PAUSED) == MFE_PAUSED)
	{
		// hitlag
		return true;
	}

	// manual
	return mobj->frozen;
}

INT32 P_AltFlip(INT32 n, tic_t tics)
{
	return leveltime % (2 * tics) < tics ? n : -(n);
}

// Please read p_tick.h
INT32 P_LerpFlip(INT32 n, tic_t tics)
{
	const tic_t w = 2 * tics;

	return P_AltFlip(((leveltime % w) - tics) * n, w);
}

//
// THINKERS
// All thinkers should be allocated by Z_Calloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//

// The entries will behave like both the head and tail of the lists.
thinker_t thlist[NUM_THINKERLISTS];

void Command_Numthinkers_f(void)
{
	INT32 num;
	INT32 count = 0;
	actionf_p1 action;
	thinker_t *think;
	thinklistnum_t start = 0;
	thinklistnum_t end = NUM_THINKERLISTS - 1;
	thinklistnum_t i;

	if (G_GamestateUsesLevel() == false)
	{
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
		return;
	}

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("numthinkers <#>: Count number of thinkers\n"));
		CONS_Printf(
			"\t1: P_MobjThinker\n"
			"\t2: P_NullPrecipThinker\n"
			"\t3: T_Friction\n"
			"\t4: T_Pusher\n"
			"\t5: P_RemoveThinkerDelayed\n");
		return;
	}

	num = atoi(COM_Argv(1));

	switch (num)
	{
		case 1:
			start = end = THINK_MOBJ;
			action = (actionf_p1)P_MobjThinker;
			CONS_Printf(M_GetText("Number of %s: "), "P_MobjThinker");
			break;
		case 2:
			start = end = THINK_PRECIP;
			action = (actionf_p1)P_NullPrecipThinker;
			CONS_Printf(M_GetText("Number of %s: "), "P_NullPrecipThinker");
			break;
		case 3:
			start = end = THINK_MAIN;
			action = (actionf_p1)T_Friction;
			CONS_Printf(M_GetText("Number of %s: "), "T_Friction");
			break;
		case 4:
			start = end = THINK_MAIN;
			action = (actionf_p1)T_Pusher;
			CONS_Printf(M_GetText("Number of %s: "), "T_Pusher");
			break;
		case 5:
			action = (actionf_p1)P_RemoveThinkerDelayed;
			CONS_Printf(M_GetText("Number of %s: "), "P_RemoveThinkerDelayed");
			break;
		default:
			CONS_Printf(M_GetText("That is not a valid number.\n"));
			return;
	}

	for (i = start; i <= end; i++)
	{
		for (think = thlist[i].next; think != &thlist[i]; think = think->next)
		{
			if (think->function.acp1 != action)
				continue;

			count++;
		}
	}

	CONS_Printf("%d\n", count);
}

void Command_CountMobjs_f(void)
{
	thinker_t *th;
	mobjtype_t i;
	INT32 count;

	if (G_GamestateUsesLevel() == false)
	{
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
		return;
	}

	if (COM_Argc() >= 2)
	{
		size_t j;
		for (j = 1; j < COM_Argc(); j++)
		{
			i = atoi(COM_Argv(j));
			if (i >= NUMMOBJTYPES)
			{
				CONS_Printf(M_GetText("Object number %d out of range (max %d).\n"), i, NUMMOBJTYPES-1);
				continue;
			}

			count = 0;

			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				if (((mobj_t *)th)->type == i)
					count++;
			}

			CONS_Printf(M_GetText("There are %d objects of type %d currently in the level.\n"), count, i);
		}
		return;
	}

	CONS_Printf(M_GetText("Count of active objects in level:\n"));

	for (i = 0; i < NUMMOBJTYPES; i++)
	{
		count = 0;

		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			if (((mobj_t *)th)->type == i)
				count++;
		}

		if (count > 0) // Don't bother displaying if there are none of this type!
			CONS_Printf(" * %d: %d\n", i, count);
	}
}

//
// P_InitThinkers
//
void P_InitThinkers(void)
{
	UINT8 i;

	P_InvalidateThinkersWithoutInit();

	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		thlist[i].prev = thlist[i].next = &thlist[i];
	}

	iquehead = iquetail = 0;

	waypointcap = NULL;
	trackercap = NULL;

	titlemapcam.mobj = NULL;

	for (i = 0; i <= 15; i++)
	{
		skyboxcenterpnts[i] = skyboxviewpnts[i] = NULL;
	}

	P_InitMobjPointers();
}

//
// P_InvalidateThinkersWithoutInit
//

void P_InvalidateThinkersWithoutInit(void)
{
	thinker_era++;
}

// Adds a new thinker at the end of the list.
void P_AddThinker(const thinklistnum_t n, thinker_t *thinker)
{
#ifdef PARANOIA
	I_Assert(n < NUM_THINKERLISTS);
#endif

	thlist[n].prev->next = thinker;
	thinker->next = &thlist[n];
	thinker->prev = thlist[n].prev;
	thlist[n].prev = thinker;

	thinker->references = 0;    // killough 11/98: init reference counter to 0
	thinker->cachable = n == THINK_MOBJ;

#ifdef PARANOIA
	thinker->debug_mobjtype = MT_NULL;
#endif
}

#ifdef PARANOIA
static const char *MobjTypeName(const mobj_t *mobj)
{
	actionf_p1 p1 = mobj->thinker.function.acp1;

	if (p1 == (actionf_p1)P_MobjThinker)
	{
		return MOBJTYPE_LIST[mobj->type];
	}
	else if (p1 == (actionf_p1)P_RemoveThinkerDelayed)
	{
		if (mobj->thinker.debug_mobjtype != MT_NULL)
		{
			return MOBJTYPE_LIST[mobj->thinker.debug_mobjtype];
		}
	}

	return "<Not a mobj>";
}

static const char *MobjThinkerName(const mobj_t *mobj)
{
	actionf_p1 p1 = mobj->thinker.function.acp1;

	if (p1 == (actionf_p1)P_MobjThinker)
	{
		return "P_MobjThinker";
	}
	else if (p1 == (actionf_p1)P_RemoveThinkerDelayed)
	{
		return "P_RemoveThinkerDelayed";
	}

	return "<Unknown Thinker>";
}
#endif

//
// killough 11/98:
//
// Make currentthinker external, so that P_RemoveThinkerDelayed
// can adjust currentthinker when thinkers self-remove.

static thinker_t *currentthinker;

//
// P_RemoveThinkerDelayed()
//
// Called automatically as part of the thinker loop in P_RunThinkers(),
// on nodes which are pending deletion.
//
// If this thinker has no more pointers referencing it indirectly,
// remove it, and set currentthinker to one node preceeding it, so
// that the next step in P_RunThinkers() will get its successor.
//
void P_RemoveThinkerDelayed(thinker_t *thinker)
{
	if (thinker->references != 0)
	{
#ifdef PARANOIA
		// Removed mobjs can be the target of another mobj. In
		// that case, the other mobj will manage its reference
		// to the removed mobj in P_MobjThinker. However, if
		// the removed mobj is removed after the other object
		// thinks, the reference management is delayed by one
		// tic (or two?)
		const UINT8 delay = 2;

		if (thinker->debug_time > leveltime)
		{
			thinker->debug_time = leveltime + delay + 1; // do not print errors again
		}
		else if ((thinker->debug_time + delay) <= leveltime)
		{
			CONS_Printf(
					"PARANOIA/P_RemoveThinkerDelayed: %p %s references=%d\n",
					(void*)thinker,
					MobjTypeName((mobj_t*)thinker),
					thinker->references
			);

			thinker->debug_time = leveltime + delay + 1; // do not print this error again
		}
#endif
		return;
	}

	R_DestroyLevelInterpolators(thinker);

	/* Note that currentthinker is guaranteed to point to us,
	* and since we're freeing our memory, we had better change that. So
	* point it to thinker->prev, so the iterator will correctly move on to
	* thinker->prev->next = thinker->next */
	currentthinker = thinker->prev;

	/* Remove from main thinker list */
	P_UnlinkThinker(thinker);
}

//
// P_UnlinkThinker()
//
// Actually removes thinker from the list and frees its memory.
//
void P_UnlinkThinker(thinker_t *thinker)
{
	thinker_t *next = thinker->next;

	I_Assert(thinker->references == 0);

	(next->prev = thinker->prev)->next = next;
	if (thinker->cachable)
	{
		// put cachable thinkers in the mobj cache, so we can avoid allocations
		((mobj_t *)thinker)->hnext = mobjcache;
		mobjcache = (mobj_t *)thinker;
	}
	else
	{
		Z_Free(thinker);
	}
}

//
// P_RemoveThinker
//
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
// killough 4/25/98:
//
// Instead of marking the function with -1 value cast to a function pointer,
// set the function to P_RemoveThinkerDelayed(), so that later, it will be
// removed automatically as part of the thinker process.
//
void P_RemoveThinker(thinker_t *thinker)
{
	LUA_InvalidateUserdata(thinker);
	thinker->function.acp1 = (actionf_p1)P_RemoveThinkerDelayed;
}

/*
 * P_SetTarget
 *
 * This function is used to keep track of pointer references to mobj thinkers.
 * In Doom, objects such as lost souls could sometimes be removed despite
 * their still being referenced. In Boom, 'target' mobj fields were tested
 * during each gametic, and any objects pointed to by them would be prevented
 * from being removed. But this was incomplete, and was slow (every mobj was
 * checked during every gametic). Now, we keep a count of the number of
 * references, and delay removal until the count is 0.
 */

mobj_t *P_SetTarget2(mobj_t **mop, mobj_t *targ
#ifdef PARANOIA
		, const char *source_file, int source_line
#endif
)
{
	if (*mop) // If there was a target already, decrease its refcount
	{
		(*mop)->thinker.references--;

#ifdef PARANOIA
		if ((*mop)->thinker.references < 0)
		{
			CONS_Printf(
					"PARANOIA/P_SetTarget: %p %s %s references=%d, references go negative! (%s:%d)\n",
					(void*)*mop,
					MobjTypeName(*mop),
					MobjThinkerName(*mop),
					(*mop)->thinker.references,
					source_file,
					source_line
			);
		}

		(*mop)->thinker.debug_time = leveltime;
#endif
	}

	if (targ != NULL) // Set new target and if non-NULL, increase its counter
	{
		targ->thinker.references++;

#ifdef PARANOIA
		targ->thinker.debug_time = leveltime;
#endif
	}

	*mop = targ;

	return targ;
}

//
// P_RunThinkers
//
// killough 4/25/98:
//
// Fix deallocator to stop using "next" pointer after node has been freed
// (a Doom bug).
//
// Process each thinker. For thinkers which are marked deleted, we must
// load the "next" pointer prior to freeing the node. In Doom, the "next"
// pointer was loaded AFTER the thinker was freed, which could have caused
// crashes.
//
// But if we are not deleting the thinker, we should reload the "next"
// pointer after calling the function, in case additional thinkers are
// added at the end of the list.
//
// killough 11/98:
//
// Rewritten to delete nodes implicitly, by making currentthinker
// external and using P_RemoveThinkerDelayed() implicitly.
//
static void P_RunThinkers(void)
{
	size_t i;

	for (i = 0; i < NUM_ACTIVETHINKERLISTS; i++)
	{
		ps_thlist_times[i] = I_GetPreciseTime();
		for (currentthinker = thlist[i].next; currentthinker != &thlist[i]; currentthinker = currentthinker->next)
		{
#ifdef PARANOIA
			I_Assert(currentthinker->function.acp1 != NULL);
#endif
			currentthinker->function.acp1(currentthinker);
		}
		ps_thlist_times[i] = I_GetPreciseTime() - ps_thlist_times[i];
	}

	if (gametyperules & GTR_CIRCUIT)
		K_RunFinishLineBeam();

	if (gametyperules & GTR_PAPERITEMS)
		K_RunPaperItemSpawners();

	if ((gametyperules & GTR_OVERTIME) && battleovertime.enabled)
		K_RunBattleOvertime();

	ps_acs_time = I_GetPreciseTime();
	ACS_Tick();
	ps_acs_time = I_GetPreciseTime() - ps_acs_time;
}

//
// P_DoAutobalanceTeams()
//
// Determine if the teams are unbalanced, and if so, move a player to the other team.
//
static void P_DoAutobalanceTeams(void)
{
	changeteam_union NetPacket;
	UINT16 usvalue;
	INT32 i=0;
	INT32 red=0, blue=0;
	INT32 redarray[MAXPLAYERS], bluearray[MAXPLAYERS];
	//INT32 redflagcarrier = 0, blueflagcarrier = 0;
	INT32 totalred = 0, totalblue = 0;

	NetPacket.value.l = NetPacket.value.b = 0;
	memset(redarray, 0, sizeof(redarray));
	memset(bluearray, 0, sizeof(bluearray));

	// Only do it if we have enough room in the net buffer to send it.
	// Otherwise, come back next time and try again.
	if (sizeof(usvalue) > GetFreeXCmdSize(0))
		return;

	//We have to store the players in an array with the rest of their team.
	//We can then pick a random player to be forced to change teams.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && players[i].ctfteam)
		{
			if (players[i].ctfteam == 1)
			{
				//if (!players[i].gotflag)
				{
					redarray[red] = i; //store the player's node.
					red++;
				}
				/*else
					redflagcarrier++;*/
			}
			else
			{
				//if (!players[i].gotflag)
				{
					bluearray[blue] = i; //store the player's node.
					blue++;
				}
				/*else
					blueflagcarrier++;*/
			}
		}
	}

	totalred = red;// + redflagcarrier;
	totalblue = blue;// + blueflagcarrier;

	if ((abs(totalred - totalblue) > max(1, (totalred + totalblue) / 8)))
	{
		if (totalred > totalblue)
		{
			i = M_RandomKey(red);
			NetPacket.packet.newteam = 2;
			NetPacket.packet.playernum = redarray[i];
			NetPacket.packet.verification = true;
			NetPacket.packet.autobalance = true;

			usvalue  = SHORT(NetPacket.value.l|NetPacket.value.b);
			SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
		}
		else //if (totalblue > totalred)
		{
			i = M_RandomKey(blue);
			NetPacket.packet.newteam = 1;
			NetPacket.packet.playernum = bluearray[i];
			NetPacket.packet.verification = true;
			NetPacket.packet.autobalance = true;

			usvalue  = SHORT(NetPacket.value.l|NetPacket.value.b);
			SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
		}
	}
}

//
// P_DoTeamscrambling()
//
// If a team scramble has been started, scramble one person from the
// pre-made scramble array. Said array is created in TeamScramble_OnChange()
//
void P_DoTeamscrambling(void)
{
	changeteam_union NetPacket;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	// Only do it if we have enough room in the net buffer to send it.
	// Otherwise, come back next time and try again.
	if (sizeof(usvalue) > GetFreeXCmdSize(0))
		return;

	if (scramblecount < scrambletotal)
	{
		if (players[scrambleplayers[scramblecount]].ctfteam != scrambleteams[scramblecount])
		{
			NetPacket.packet.newteam = scrambleteams[scramblecount];
			NetPacket.packet.playernum = scrambleplayers[scramblecount];
			NetPacket.packet.verification = true;
			NetPacket.packet.scrambled = true;

			usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
			SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
		}

		scramblecount++; //Increment, and get to the next player when we come back here next time.
	}
	else
		CV_SetValue(&cv_teamscramble, 0);
}

static inline void P_DoTeamStuff(void)
{
	// Automatic team balance for CTF and team match
	if (leveltime % (TICRATE * 5) == 0) //only check once per five seconds for the sake of CPU conservation.
	{
		// Do not attempt to autobalance and scramble teams at the same time.
		// Only the server should execute this. No verified admins, please.
		if ((cv_autobalance.value && !cv_teamscramble.value) && cv_allowteamchange.value && server)
			P_DoAutobalanceTeams();
	}

	// Team scramble code for team match and CTF.
	if ((leveltime % (TICRATE/7)) == 0)
	{
		// If we run out of time in the level, the beauty is that
		// the Y_Ticker() team scramble code will pick it up.
		if (cv_teamscramble.value && server)
			P_DoTeamscrambling();
	}
}

static inline void P_DeviceRumbleTick(void)
{
	UINT8 i;

	for (i = 0; i <= splitscreen; i++)
	{
		player_t *player = &players[g_localplayers[i]];
		UINT16 low = 0;
		UINT16 high = 0;

		if (player->mo != NULL && !player->exiting)
		{
			if ((player->mo->eflags & MFE_DAMAGEHITLAG) && player->mo->hitlag)
			{
				low = high = 65536 / 2;
			}
			else if (player->sneakertimer > (sneakertime-(TICRATE/2)))
			{
				low = high = 65536 / (3+player->numsneakers);
			}
			else if (((player->boostpower < FRACUNIT) || (player->stairjank > 8))
				&& P_IsObjectOnGround(player->mo) && player->speed != 0)
			{
				low = high = 65536 / 32;
			}
		}

		G_PlayerDeviceRumble(i, low, high);
	}
}

void P_RunChaseCameras(void)
{
	UINT8 i;

	for (i = 0; i <= r_splitscreen; i++)
	{
		if (camera[i].chase)
		{
			player_t *p = &players[displayplayers[i]];
			camera_t *cam = &camera[i];

			if (p->mo && p->cmd.throwdir != 0)
			{
				if (p->speed < 6 * p->mo->scale && abs(cam->dpad_y_held) < 2*TICRATE)
					cam->dpad_y_held += intsign(p->cmd.throwdir);
			}
			else
				cam->dpad_y_held = 0;

			P_MoveChaseCamera(p, cam, false);
		}
	}

	K_EndCameraGC();
}

static fixed_t P_GetDarkness(tic_t start, tic_t end)
{
	if (leveltime <= end)
	{
		tic_t fade = end - DARKNESS_FADE_TIME;
		tic_t t;

		if (leveltime < fade) // dark or darkening
		{
			t = leveltime - start;
			t = min(t, DARKNESS_FADE_TIME);
		}
		else // lightening
		{
			t = end - leveltime;
		}

		return Easing_Linear((t * FRACUNIT) / DARKNESS_FADE_TIME, 0,
			(gametyperules & GTR_BUMPERS) ? FRACUNIT/3 : FRACUNIT/7);
	}

	return 0;
}

static void P_TickDarkness(void)
{
	const fixed_t globalValue = P_GetDarkness(g_darkness.start, g_darkness.end);
	UINT8 i;

	for (i = 0; i <= r_splitscreen; ++i)
	{
		const player_t *p = &players[displayplayers[i]];
		fixed_t value = P_GetDarkness(p->darkness_start, p->darkness_end);

		g_darkness.value[i] = value ? value : globalValue;
	}
}

static void P_TickMusicFade(void)
{
	if (leveltime >= g_musicfade.start && leveltime <= g_musicfade.end)
	{
		INT32 half = (g_musicfade.end - g_musicfade.start) / 2;
		INT32 fade = max(1, g_musicfade.fade);
		INT32 mid = half - fade;
		INT32 t = abs((INT32)leveltime - (INT32)(g_musicfade.start + half));
		Music_LevelVolume((max(t, mid) - mid) * 100 / fade);
	}
	else if (!g_musicfade.ticked)
		Music_LevelVolume(100);

	g_musicfade.ticked = true;
}

static void P_StartLevelMusic(void)
{
	if (!Music_Playing("level_nosync"))
	{
		// Do not stop level_nosync
		Music_Play(P_UseContinuousLevelMusic() ? "level_nosync" : "level");
	}
}

//
// P_Ticker
//
void P_Ticker(boolean run)
{
	quake_t *quake = NULL;
	INT32 i;

	thinkersCompleted = false;

	// Increment jointime and quittime even if paused
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			players[i].jointime++;
		}
	}

	if (objectplacing)
	{
		if (OP_FreezeObjectplace())
		{
			P_MapStart();
			R_UpdateMobjInterpolators();
			OP_ObjectplaceMovement(&players[0]);
			P_MoveChaseCamera(&players[0], &camera[0], false);
			R_UpdateViewInterpolation();
			P_MapEnd();
			return;
		}
	}

	// Check for pause or menu up in single player
	if (paused || P_AutoPause())
	{
		if (demo.rewinding && leveltime > 0)
		{
			leveltime = (leveltime-1) & ~3;
			if (timeinmap > 0)
				timeinmap = (timeinmap-1) & ~3;
			G_PreviewRewind(leveltime);
		}
		else
			P_RunChaseCameras();	// special case: allow freecam to MOVE during pause!
		return;
	}

	for (i = 0; i <= r_splitscreen; i++)
		postimgtype[i] = postimg_none;

	P_MapStart();

	if (run)
	{
		R_UpdateMobjInterpolators();

		if (demo.recording)
		{
			G_WriteDemoExtraData();
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i])
					G_WriteDemoTiccmd(&players[i].cmd, i);
		}
		if (demo.playback && !demo.waitingfortally)
		{
			G_ReadDemoExtraData();
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i])
					G_ReadDemoTiccmd(&players[i].cmd, i);
		}

		LUA_ResetTicTimers();

		ps_lua_mobjhooks = 0;
		ps_checkposition_calls = 0;

		LUA_HOOK(PreThinkFrame);

		ps_playerthink_time = I_GetPreciseTime();

		K_UpdateAllPlayerPositions();

		// OK! Now that we got all of that sorted, players can think!
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!(playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo)))
				continue;
			P_PlayerThink(&players[i]);
			K_KartPlayerHUDUpdate(&players[i]);
		}

		ps_playerthink_time = I_GetPreciseTime() - ps_playerthink_time;

		if (gamedata && gamestate == GS_LEVEL && !demo.playback)
		{
			mapheader_t *mapheader;

			mapheader = mapheaderinfo[gamemap - 1];

			// Keep track of how long they've been playing!
			gamedata->totalplaytime++;

			// Map playtime
			if (mapheader)
			{
				mapheader->records.timeplayed++;
			}

			// Netgame total time
			if (netgame)
			{
				gamedata->totalnetgametime++;

				if (mapheader)
				{
					mapheader->records.netgametimeplayed++;
				}
			}

			// Per-skin total playtime for all machine-local players
			for (i = 0; i < MAXPLAYERS; i++)
			{
				skin_t *playerskin;

				if (!P_IsMachineLocalPlayer(&players[i]))
				{
					continue;
				}

				if (!(playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo)))
				{
					continue;
				}

				if (players[i].skin >= numskins)
				{
					continue;
				}

				playerskin = &skins[players[i].skin];

				playerskin->records.timeplayed++;
			}

			if (gametype != GT_TUTORIAL)
			{
				INT32 mode = M_GameDataGameType(gametype, battleprisons);

				// Gamedata mode playtime
				if (mode >= 0 && mode < GDGT_MAX)
				{
					gamedata->modeplaytime[mode]++;
					if (mapheader)
					{
						mapheader->records.modetimeplayed[mode]++;
					}
				}

				// Attacking mode playtime
				if (modeattacking != ATTACKING_NONE)
				{
					if (encoremode) // ((modeattacking & ATTACKING_SPB) != 0)
					{
						gamedata->spbattackingtotaltime++;
						if (mapheader)
						{
							mapheader->records.spbattacktimeplayed++;
						}
					}
					//else
					{
						gamedata->timeattackingtotaltime++;
						if (mapheader)
						{
							mapheader->records.timeattacktimeplayed++;
						}
					}
				}

				// Per-skin mode playtime
				for (i = 0; i < MAXPLAYERS; i++)
				{
					skin_t *playerskin;

					if (!P_IsMachineLocalPlayer(&players[i]))
					{
						continue;
					}

					if (!(playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo)))
					{
						continue;
					}

					if (players[i].skin >= numskins)
					{
						continue;
					}

					playerskin = &skins[players[i].skin];
					playerskin->records.modetimeplayed[mode]++;
				}
			}

			// TODO would this be laggy with more conditions in play...
			if (
				(leveltime > introtime
					&& M_UpdateUnlockablesAndExtraEmblems(true, false))
				|| gamedata->deferredsave
			)
			{
				G_SaveGameData();
			}
		}
	}

	if (run)
	{
		ps_thinkertime = I_GetPreciseTime();
		P_RunThinkers();
		ps_thinkertime = I_GetPreciseTime() - ps_thinkertime;
		thinkersCompleted = true;

		// Run any "after all the other thinkers" stuff
		{
			player_t *finishingPlayers[MAXPLAYERS];
			UINT8 numingame = 0, numFinishingPlayers = 0;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo))
				{
					P_PlayerAfterThink(&players[i]);

					if (players[i].spectator == true)
						continue;

					numingame++;

					// Check for the number of ties for first place after every player has thunk run for this tic
					if (players[i].exiting == 1 && players[i].position == 1 &&
							(players[i].pflags & (PF_HITFINISHLINE|PF_NOCONTEST)) == PF_HITFINISHLINE)
					{
						finishingPlayers[numFinishingPlayers++] = &players[i];
					}
				}
			}

			if ((netgame) // Antigrief is supposed to apply?
				&& !(K_Cooperative() || timelimitintics > 0 || g_pointlimit > 0) // There are rules that will punish a griefing player
				&& (gametyperules & GTR_CIRCUIT) && (leveltime > starttime) && K_GetNumWaypoints()) // The following only detects race griefing
			{
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo))
					{
						if (players[i].spectator == true)
							continue;

						if (players[i].exiting || (players[i].pflags & PF_NOCONTEST))
							continue;

						if (players[i].bot == true)
							continue;

						P_CheckRaceGriefing(&players[i], (numingame > 1));
					}
				}
			}

			// Apply rumble to local players
			if (!demo.playback)
			{
				P_DeviceRumbleTick();
			}

			if (numFinishingPlayers > 1)
			{
				for (i = 0; i < numFinishingPlayers; i++)
				{
					P_SetupSignExit(finishingPlayers[i], true);
				}
			}
			else if (numFinishingPlayers == 1)
			{
				P_SetupSignExit(finishingPlayers[0], false);
			}
		}

		if (musiccountdown > 0)
		{
			// Music is controlled by completion sequence
		}
		else if (K_CheckBossIntro() == true)
		{
			// Bosses have a punchy start, so no position.
			if (leveltime == 1)
			{
				P_StartLevelMusic();
			}
		}
		else if (leveltime < starttime + TICRATE)
		{
			if (leveltime == (starttime + (TICRATE/2)))
			{
				// Plays the music after the starting countdown.
				P_StartLevelMusic();
			}
			else if (starttime != introtime)
			{
				// Start countdown/music handling
				if (leveltime == starttime-(3*TICRATE))
				{
					S_StartSound(NULL, sfx_s3ka7); // 3,
				}
				else if ((leveltime == starttime-(2*TICRATE))
					|| (leveltime == starttime-TICRATE))
				{
					S_StartSound(NULL, sfx_s3ka7); // 2, 1,
				}
				else if (leveltime == starttime)
				{
					S_StartSound(NULL, sfx_s3kad); // GO!
				}

				// POSITION!! music
				if (modeattacking == ATTACKING_NONE || !P_UseContinuousLevelMusic())
				{
					P_StartPositionMusic(true); // exact times only
				}
			}
		}

		if (modeattacking != ATTACKING_NONE && P_UseContinuousLevelMusic())
		{
			if (leveltime == 4)
			{
				P_StartLevelMusic();
			}
		}

		ps_lua_thinkframe_time = I_GetPreciseTime();
		LUA_HookThinkFrame();
		ps_lua_thinkframe_time = I_GetPreciseTime() - ps_lua_thinkframe_time;
	}

	if (run)
		UpdateChallenges();

	// Run shield positioning
	P_RunOverlays();

	P_UpdateSpecials();
	P_RespawnSpecials();

	// Lightning, rain sounds, etc.
	P_PrecipitationEffects();

	if (run)
	{
		leveltime++;

		if (starttime > introtime && leveltime == starttime)
		{
			ACS_RunPositionScript();
		}

		if ((gametyperules & GTR_OVERTIME) && !battleprisons &&
			timelimitintics > 0 && leveltime == (timelimitintics + starttime + 1))
		{
			ACS_RunOvertimeScript();
		}
	}

	if (g_fast_forward == 0)
	{
		timeinmap++;
	}

	if (G_GametypeHasTeams())
		P_DoTeamStuff();

	if (run)
	{
		if (racecountdown > 1)
			racecountdown--;

		P_TickDarkness();
		P_TickMusicFade();

		if (exitcountdown >= 1)
		{
			boolean run_exit_countdown = true;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] == false)
				{
					continue;
				}

				player_t *player = &players[i];
				if (player->spectator == false && K_PlayerTallyActive(player) == true && player->tally.done == false)
				{
					run_exit_countdown = false;
					break;
				}
			}

			if (run_exit_countdown == true)
			{
				if (exitcountdown > 1)
				{
					exitcountdown--;
				}

				if (exitcountdown == 1)
				{
					if (demo.playback)
						G_FinishExitLevel();
					else if (server)
						SendNetXCmd(XD_EXITLEVEL, NULL, 0);
				}
			}
		}

		K_RunItemCooldowns();

		K_BossInfoTicker();

		K_TickSpecialStage();

		if ((gametyperules & GTR_POINTLIMIT))
		{
			if (wantedcalcdelay && --wantedcalcdelay <= 0)
				K_CalculateBattleWanted();
		}

		if (bombflashtimer)
			bombflashtimer--;	// Bomb seizure prevention

		// Tick quake effects
		quake = g_quakes;
		while (quake != NULL)
		{
			if (quake->time <= 0)
			{
				// Time out, remove this effect
				quake_t *remove = quake;
				quake = quake->next;
				P_FreeQuake(remove);
				continue;
			}

			quake->time--;

			if (quake->epicenter != NULL && quake->mobj != NULL && P_MobjWasRemoved(quake->mobj) == false)
			{
				quake->epicenter->x = quake->mobj->x;
				quake->epicenter->y = quake->mobj->y;
				quake->epicenter->z = quake->mobj->z + (quake->mobj->height / 2);
			}

			quake = quake->next;
		}

		if (demo.recording)
		{
			G_WriteAllGhostTics();

			if (cv_recordmultiplayerdemos.value && demo.savebutton && demo.savebutton + 3*TICRATE < leveltime)
				G_CheckDemoTitleEntry();
		}
		else if (demo.playback && !demo.waitingfortally) // Use Ghost data for consistency checks.
		{
			G_ConsAllGhostTics();
		}

		if (modeattacking)
		{
			if (!demo.waitingfortally)
				G_GhostTicker();
			if (!demo.playback)
				G_TickTimeStickerMedals();
		}

		if (mapreset > 1
			&& --mapreset <= 1
			&& server) // Remember: server uses it for mapchange, but EVERYONE ticks down for the animation
				D_MapChange(gamemap, gametype, encoremode, true, 0, false, false);

		if (cv_kartdebugwaypoints.value != 0)
		{
			K_DebugWaypointsVisualise();
		}
	}

	if (gamestate == GS_LEVEL)
	{
		// Move the camera during levels.
		K_UpdateDirector();
		P_RunChaseCameras();
	}

	if (run && !levelloading && leveltime)
	{
		K_TickDialogue();
		K_TickMessages();
	}

	if (run)
	{
		LUA_HOOK(PostThinkFrame);

		R_UpdateLevelInterpolators();

		// Hack: ensure newview is assigned every tic.
		// Ensures view interpolation is T-1 to T in poor network conditions
		// We need a better way to assign view state decoupled from game logic
		if (rendermode != render_none)
		{
			for (i = 0; i <= r_splitscreen; i++)
			{
				player_t *player = &players[displayplayers[i]];
				if (!player->mo)
					continue;
				const boolean skybox = (player->skybox.viewpoint && cv_skybox.value); // True if there's a skybox object and skyboxes are on
				if (skybox)
				{
					R_SkyboxFrame(i);
				}
				R_SetupFrame(i);
			}
		}
	}

	P_MapEnd();

	if (demo.playback)
		G_StoreRewindInfo();

	for (i = 0; i < MAXPLAYERS; i++)
	{
		G_CopyTiccmd(&players[i].oldcmd, &players[i].cmd, 1);
	}

	if (D_IsDeferredStartTitle())
	{
		D_StartTitle();
	}
//	Z_CheckMemCleanup();
}
