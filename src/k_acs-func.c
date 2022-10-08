// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2016 by James Haley, David Hill, et al. (Team Eternity)
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_acs-func.c
/// \brief ACS CallFunc definitions

#include "k_acs.h"

#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
#include "d_think.h"
#include "p_mobj.h"
#include "p_tick.h"
#include "w_wad.h"
#include "m_random.h"
#include "g_game.h"
#include "d_player.h"
#include "r_defs.h"
#include "r_state.h"
#include "p_polyobj.h"
#include "taglist.h"
#include "p_local.h"
#include "deh_tables.h"
#include "fastcmp.h"
#include "hu_stuff.h"

/*--------------------------------------------------
	static bool ACS_GetMobjTypeFromString(const char *word, mobjtype_t *type)

		Helper function for ACS_CF_ThingCount. Gets
		an object type from a string.

	Input Arguments:-
		word: The mobj class string.
		type: Variable to store the result in.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_GetMobjTypeFromString(const char *word, mobjtype_t *type)
{
	mobjtype_t i;

	if (fastncmp("MT_", word, 3))
	{
		// take off the MT_
		word += 3;
	}

	for (i = 0; i < NUMMOBJFREESLOTS; i++)
	{
		if (!FREE_MOBJS[i])
		{
			break;
		}

		if (fastcmp(word, FREE_MOBJS[i]))
		{
			*type = MT_FIRSTFREESLOT+i;
			return true;
		}
	}

	for (i = 0; i < MT_FIRSTFREESLOT; i++)
	{
		if (fastcmp(word, MOBJTYPE_LIST[i]+3))
		{
			*type = i;
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_CountThing(mobj_t *mobj, mobjtype_t type)

		Helper function for ACS_CF_ThingCount.
		Returns whenever or not to add this thing
		to the thing count.

	Input Arguments:-
		mobj: The mobj we want to count.
		type: Type exclusion.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_CountThing(mobj_t *mobj, mobjtype_t type)
{
	if (type == MT_NULL || mobj->type == type)
	{
		// Don't count dead monsters
		if (mobj->info->spawnhealth > 0 && mobj->health <= 0)
		{
			// Note: Hexen checks for COUNTKILL.
			// SRB2 does not have an equivalent, so I'm checking
			// spawnhealth. Feel free to replace this condition
			// with literally anything else.
			return false;
		}

		// Count this object.
		return true;
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_ActivatorIsLocal(ACSVM_Thread *thread)

		Helper function for many print functions.
		Returns whenever or not the activator of the
		thread is a display player or not.

	Input Arguments:-
		thread: The thread we're exeucting on.

	Return:-
		true if it's for a display player,
		otherwise false.
--------------------------------------------------*/
static bool ACS_ActivatorIsLocal(ACSVM_Thread *thread)
{
	acs_threadinfo_t *info = (acs_threadinfo_t *)ACSVM_Thread_GetInfo(thread);

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		return P_IsDisplayPlayer(info->mo->player);
	}

	return false;
}

/*--------------------------------------------------
	bool ACS_CF_Random(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		ACS wrapper for P_RandomRange.
--------------------------------------------------*/
bool ACS_CF_Random(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	INT32 low = 0;
	INT32 high = 0;

	(void)argC;

	low = argV[0];
	high = argV[1];

	ACSVM_Thread_DataStk_Push(thread, P_RandomRange(PR_ACS, low, high));
	return false;
}

/*--------------------------------------------------
	bool ACS_CF_ThingCount(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		Counts the number of things of a particular
		type and tid. Both fields are optional;
		no type means indescriminate against type,
		no tid means search thru all thinkers.
--------------------------------------------------*/
bool ACS_CF_ThingCount(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	ACSVM_MapScope *map = NULL;
	ACSVM_String *str = NULL;
	const char *className = NULL;
	size_t classLen = 0;

	mobjtype_t type = MT_NULL;
	mtag_t tid = 0;

	size_t count = 0;

	(void)argC;

	map = ACSVM_Thread_GetScopeMap(thread);
	str = ACSVM_MapScope_GetString(map, argV[0]);

	className = ACSVM_String_GetStr(str);
	classLen = ACSVM_String_GetLen(str);

	if (classLen > 0)
	{
		bool success = ACS_GetMobjTypeFromString(className, &type);

		if (success == false)
		{
			// Exit early.

			CONS_Alert(CONS_WARNING,
				"Couldn't find object type \"%s\" for ThingCount.\n",
				className
			);

			return false;
		}
	}

	tid = argV[1];

	if (tid != 0)
	{
		// TODO: Even in SRB2 next's UDMF, tag lists
		// still aren't attached to mobj_t, only
		// mapthing_t. Fix this.
	}
	else
	{
		// Search thinkers instead of tag lists.
		thinker_t *th = NULL;
		mobj_t *mobj = NULL;

		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			{
				continue;
			}

			mobj = (mobj_t *)th;

			if (ACS_CountThing(mobj, type) == true)
			{
				++count;
			}
		}
	}

	ACSVM_Thread_DataStk_Push(thread, count);
	return false;
}

/*--------------------------------------------------
	bool ACS_CF_TagWait(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		Pauses the thread until the tagged
		sector stops moving.
--------------------------------------------------*/
bool ACS_CF_TagWait(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	ACSVM_ThreadState st = {0};

	(void)argC;

	st.state = ACSVM_ThreadState_WaitTag;
	st.data = argV[0];
	st.type = ACS_TAGTYPE_SECTOR;

	ACSVM_Thread_SetState(thread, st);
	return true; // Execution interrupted
}

/*--------------------------------------------------
	bool ACS_CF_PolyWait(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		Pauses the thread until the tagged
		polyobject stops moving.
--------------------------------------------------*/
bool ACS_CF_PolyWait(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	ACSVM_ThreadState st = {0};

	(void)argC;

	st.state = ACSVM_ThreadState_WaitTag;
	st.data = argV[0];
	st.type = ACS_TAGTYPE_POLYOBJ;

	ACSVM_Thread_SetState(thread, st);
	return true; // Execution interrupted
}

/*--------------------------------------------------
	bool ACS_CF_ChangeFloor(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		Changes a floor texture.
--------------------------------------------------*/
bool ACS_CF_ChangeFloor(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	ACSVM_MapScope *map = NULL;
	ACSVM_String *str = NULL;
	const char *texName = NULL;

	INT32 secnum = -1;
	mtag_t tag = 0;

	(void)argC;

	tag = argV[0];

	map = ACSVM_Thread_GetScopeMap(thread);
	str = ACSVM_MapScope_GetString(map, argV[1]);
	texName = ACSVM_String_GetStr(str);

	TAG_ITER_SECTORS(tag, secnum)
	{
		sector_t *sec = &sectors[secnum];
		sec->floorpic = P_AddLevelFlatRuntime(texName);
	}

	return false;
}

/*--------------------------------------------------
	bool ACS_CF_ChangeCeiling(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		Changes a ceiling texture.
--------------------------------------------------*/
bool ACS_CF_ChangeCeiling(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	ACSVM_MapScope *map = NULL;
	ACSVM_String *str = NULL;
	const char *texName = NULL;

	INT32 secnum = -1;
	mtag_t tag = 0;

	(void)argC;

	tag = argV[0];

	map = ACSVM_Thread_GetScopeMap(thread);
	str = ACSVM_MapScope_GetString(map, argV[1]);
	texName = ACSVM_String_GetStr(str);

	TAG_ITER_SECTORS(tag, secnum)
	{
		sector_t *sec = &sectors[secnum];
		sec->ceilingpic = P_AddLevelFlatRuntime(texName);
	}

	return false;
}

/*--------------------------------------------------
	bool ACS_CF_LineSide(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		Pushes which side of the linedef was
		activated.
--------------------------------------------------*/
bool ACS_CF_LineSide(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	acs_threadinfo_t *info = (acs_threadinfo_t *)ACSVM_Thread_GetInfo(thread);

	(void)argV;
	(void)argC;

	ACSVM_Thread_DataStk_Push(thread, info->side);
	return false;
}

/*--------------------------------------------------
	bool ACS_CF_EndPrint(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		One of the ACS wrappers for CEcho. This
		version only prints if the activator is a
		display player.
--------------------------------------------------*/
bool ACS_CF_EndPrint(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	ACSVM_PrintBuf *buf = NULL;

	(void)argV;
	(void)argC;

	buf = ACSVM_Thread_GetPrintBuf(thread);

	if (ACS_ActivatorIsLocal(thread) == true)
	{
		HU_SetCEchoDuration(5);
		HU_DoCEcho(ACSVM_PrintBuf_GetData(buf));
	}

	ACSVM_PrintBuf_Drop(buf);

	return false;
}

/*--------------------------------------------------
	bool ACS_CF_PlayerCount(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		Pushes the number of players to ACS.
--------------------------------------------------*/
bool ACS_CF_PlayerCount(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	UINT8 numPlayers = 0;
	UINT8 i;

	(void)argV;
	(void)argC;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];

		if (player->spectator == true)
		{
			continue;
		}

		numPlayers++;
	}

	ACSVM_Thread_DataStk_Push(thread, numPlayers);
	return false;
}

/*--------------------------------------------------
	bool ACS_CF_GameType(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		Pushes the current gametype to ACS.
--------------------------------------------------*/
bool ACS_CF_GameType(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	(void)argV;
	(void)argC;

	ACSVM_Thread_DataStk_Push(thread, gametype);
	return false;
}

/*--------------------------------------------------
	bool ACS_CF_GameSpeed(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		Pushes the current game speed to ACS.
--------------------------------------------------*/
bool ACS_CF_GameSpeed(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	(void)argV;
	(void)argC;

	ACSVM_Thread_DataStk_Push(thread, gamespeed);
	return false;
}

/*--------------------------------------------------
	bool ACS_CF_Timer(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		Pushes leveltime to ACS.
--------------------------------------------------*/
bool ACS_CF_Timer(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	(void)argV;
	(void)argC;

	ACSVM_Thread_DataStk_Push(thread, leveltime);
	return false;
}

/*--------------------------------------------------
	bool ACS_CF_EndPrintBold(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		One of the ACS wrappers for CEcho. This
		version prints for all players.
--------------------------------------------------*/
bool ACS_CF_EndPrintBold(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	ACSVM_PrintBuf *buf = NULL;

	(void)argV;
	(void)argC;

	buf = ACSVM_Thread_GetPrintBuf(thread);

	HU_SetCEchoDuration(5);
	HU_DoCEcho(ACSVM_PrintBuf_GetData(buf));

	ACSVM_PrintBuf_Drop(buf);

	return false;
}

/*--------------------------------------------------
	bool ACS_CF_EndLog(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)

		One of the ACS wrappers for CONS_Printf.
		This version only prints if the activator
		is a display player.
--------------------------------------------------*/
bool ACS_CF_EndLog(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	ACSVM_PrintBuf *buf = NULL;

	(void)argV;
	(void)argC;

	buf = ACSVM_Thread_GetPrintBuf(thread);

	if (ACS_ActivatorIsLocal(thread) == true)
	{
		CONS_Printf("%s\n", ACSVM_PrintBuf_GetData(buf));
	}

	ACSVM_PrintBuf_Drop(buf);

	return false;
}
