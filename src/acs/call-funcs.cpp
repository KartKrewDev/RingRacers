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
/// \file  call-funcs.cpp
/// \brief Action Code Script: CallFunc instructions

#include <algorithm>
#include <cctype>

#include "acsvm.hpp"

#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"

#include "../d_think.h"
#include "../p_mobj.h"
#include "../p_tick.h"
#include "../w_wad.h"
#include "../m_random.h"
#include "../g_game.h"
#include "../d_player.h"
#include "../r_defs.h"
#include "../r_state.h"
#include "../p_polyobj.h"
#include "../taglist.h"
#include "../p_local.h"
#include "../deh_tables.h"
#include "../fastcmp.h"
#include "../hu_stuff.h"
#include "../s_sound.h"
#include "../r_textures.h"
#include "../m_cond.h"
#include "../r_skins.h"
#include "../k_battle.h"
#include "../k_podium.h"
#include "../z_zone.h"

#include "call-funcs.hpp"

#include "environment.hpp"
#include "thread.hpp"
#include "../cxxutil.hpp"

using namespace srb2::acs;

/*--------------------------------------------------
	static bool ACS_GetMobjTypeFromString(const char *word, mobjtype_t *type)

		Helper function for CallFunc_ThingCount. Gets
		an object type from a string.

	Input Arguments:-
		word: The mobj class string.
		type: Variable to store the result in.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_GetMobjTypeFromString(const char *word, mobjtype_t *type)
{
	if (fastncmp("MT_", word, 3))
	{
		// take off the MT_
		word += 3;
	}

	for (int i = 0; i < NUMMOBJFREESLOTS; i++)
	{
		if (!FREE_MOBJS[i])
		{
			break;
		}

		if (fastcmp(word, FREE_MOBJS[i]))
		{
			*type = static_cast<mobjtype_t>(static_cast<int>(MT_FIRSTFREESLOT) + i);
			return true;
		}
	}

	for (int i = 0; i < MT_FIRSTFREESLOT; i++)
	{
		if (fastcmp(word, MOBJTYPE_LIST[i] + 3))
		{
			*type = static_cast<mobjtype_t>(i);
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_GetSFXFromString(const char *word, sfxenum_t *type)

		Helper function for sound playing functions.
		Gets a SFX id from a string.

	Input Arguments:-
		word: The sound effect string.
		type: Variable to store the result in.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static bool ACS_GetSFXFromString(const char *word, sfxenum_t *type)
{
	if (fastncmp("SFX_", word, 4))
	{
		// take off the SFX_
		word += 4;
	}
	else if (fastncmp("DS", word, 2))
	{
		// take off the DS
		word += 2;
	}

	for (int i = 0; i < NUMSFX; i++)
	{
		if (S_sfx[i].name && fasticmp(word, S_sfx[i].name))
		{
			*type = static_cast<sfxenum_t>(i);
			return true;
		}
	}

	return false;
}

/*--------------------------------------------------
	static bool ACS_CountThing(mobj_t *mobj, mobjtype_t type)

		Helper function for CallFunc_ThingCount.
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
	static bool ACS_ActivatorIsLocal(ACSVM::Thread *thread)

		Helper function for many print functions.
		Returns whenever or not the activator of the
		thread is a display player or not.

	Input Arguments:-
		thread: The thread we're exeucting on.

	Return:-
		true if it's for a display player,
		otherwise false.
--------------------------------------------------*/
static bool ACS_ActivatorIsLocal(ACSVM::Thread *thread)
{
	auto info = &static_cast<Thread *>(thread)->info;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		return P_IsDisplayPlayer(info->mo->player);
	}

	return false;
}

/*--------------------------------------------------
	static UINT32 ACS_SectorThingCounter(sector_t *sec, bool (*filter)(mobj_t *))

		Helper function for CallFunc_CountEnemies
		and CallFunc_CountPushables. Counts a number
		of things in the specified sector.

	Input Arguments:-
		sec: The sector to search in.
		filter: Filter function, total count is increased when
			this function returns true.

	Return:-
		Numbers of things matching the filter found.
--------------------------------------------------*/
static UINT32 ACS_SectorThingCounter(sector_t *sec, bool (*filter)(mobj_t *))
{
	msecnode_t *node = sec->touching_thinglist; // things touching this sector
	UINT32 count = 0;

	while (node)
	{
		mobj_t *mo = node->m_thing;

		if (mo->z > sec->ceilingheight
			|| mo->z + mo->height < sec->floorheight)
		{
			continue;
		}

		if (filter(mo) == true)
		{
			count++;
		}

		node = node->m_thinglist_next;
	}

	return count;
}

/*--------------------------------------------------
	static UINT32 ACS_SectorTagThingCounter(mtag_t tag, bool (*filter)(mobj_t *))

		Helper function for CallFunc_CountEnemies
		and CallFunc_CountPushables. Counts a number
		of things in the tagged sectors.

	Input Arguments:-
		tag: The sector tag to search in.
		filter: Filter function, total count is increased when
			this function returns true.

	Return:-
		Numbers of things matching the filter found.
--------------------------------------------------*/
static UINT32 ACS_SectorTagThingCounter(mtag_t tag, bool (*filter)(mobj_t *))
{
	INT32 secnum = -1;
	UINT32 count = 0;
	size_t i;

	TAG_ITER_SECTORS(tag, secnum)
	{
		sector_t *sec = &sectors[secnum];
		boolean FOFsector = false;

		// Check the lines of this sector, to see if it is a FOF control sector.
		for (i = 0; i < sec->linecount; i++)
		{
			INT32 targetsecnum = -1;

			if (sec->lines[i]->special < 100 || sec->lines[i]->special >= 300)
			{
				continue;
			}

			FOFsector = true;

			TAG_ITER_SECTORS(sec->lines[i]->args[0], targetsecnum)
			{
				sector_t *targetsec = &sectors[targetsecnum];
				count += ACS_SectorThingCounter(targetsec, filter);
			}
		}

		if (FOFsector == false)
		{
			count += ACS_SectorThingCounter(sec, filter);
		}
	}

	return count;
}

/*--------------------------------------------------
	bool CallFunc_Random(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		ACS wrapper for P_RandomRange.
--------------------------------------------------*/
bool CallFunc_Random(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	INT32 low = 0;
	INT32 high = 0;

	(void)argC;

	low = argV[0];
	high = argV[1];

	thread->dataStk.push(P_RandomRange(PR_ACS, low, high));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_ThingCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Counts the number of things of a particular
		type and tid. Both fields are optional;
		no type means indescriminate against type,
		no tid means search thru all thinkers.
--------------------------------------------------*/
bool CallFunc_ThingCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = NULL;
	ACSVM::String *str = NULL;
	const char *className = NULL;
	size_t classLen = 0;

	mobjtype_t type = MT_NULL;
	mtag_t tid = 0;

	size_t count = 0;

	(void)argC;

	map = thread->scopeMap;
	str = map->getString(argV[0]);

	className = str->str;
	classLen = str->len;

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
		mobj_t *mobj = nullptr;

		while ((mobj = P_FindMobjFromTID(tid, mobj, nullptr)) != nullptr)
		{
			if (ACS_CountThing(mobj, type) == true)
			{
				++count;
			}
		}
	}
	else
	{
		// Search thinkers instead of tag lists.
		thinker_t *th = nullptr;
		mobj_t *mobj = nullptr;

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

	thread->dataStk.push(count);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_TagWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pauses the thread until the tagged
		sector stops moving.
--------------------------------------------------*/
bool CallFunc_TagWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	thread->state = {
		ACSVM::ThreadState::WaitTag,
		argV[0],
		ACS_TAGTYPE_SECTOR
	};

	return true; // Execution interrupted
}

/*--------------------------------------------------
	bool CallFunc_PolyWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pauses the thread until the tagged
		polyobject stops moving.
--------------------------------------------------*/
bool CallFunc_PolyWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	thread->state = {
		ACSVM::ThreadState::WaitTag,
		argV[0],
		ACS_TAGTYPE_POLYOBJ
	};

	return true; // Execution interrupted
}

/*--------------------------------------------------
	bool CallFunc_CameraWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pauses the thread until the tagged
		camera is done moving.
--------------------------------------------------*/
bool CallFunc_CameraWait(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argC;

	thread->state = {
		ACSVM::ThreadState::WaitTag,
		argV[0],
		ACS_TAGTYPE_CAMERA
	};

	return true; // Execution interrupted
}

/*--------------------------------------------------
	bool CallFunc_ChangeFloor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Changes a floor texture.
--------------------------------------------------*/
bool CallFunc_ChangeFloor(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = nullptr;
	ACSVM::String *str = nullptr;
	const char *texName = nullptr;

	INT32 secnum = -1;
	mtag_t tag = 0;

	(void)argC;

	tag = argV[0];

	map = thread->scopeMap;
	str = map->getString(argV[1]);
	texName = str->str;

	TAG_ITER_SECTORS(tag, secnum)
	{
		sector_t *sec = &sectors[secnum];
		sec->floorpic = P_AddLevelFlatRuntime(texName);
	}

	return false;
}

/*--------------------------------------------------
	bool CallFunc_ChangeCeiling(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Changes a ceiling texture.
--------------------------------------------------*/
bool CallFunc_ChangeCeiling(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = NULL;
	ACSVM::String *str = NULL;
	const char *texName = NULL;

	INT32 secnum = -1;
	mtag_t tag = 0;

	(void)argC;

	tag = argV[0];

	map = thread->scopeMap;
	str = map->getString(argV[1]);
	texName = str->str;

	TAG_ITER_SECTORS(tag, secnum)
	{
		sector_t *sec = &sectors[secnum];
		sec->ceilingpic = P_AddLevelFlatRuntime(texName);
	}

	return false;
}

/*--------------------------------------------------
	bool CallFunc_LineSide(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pushes which side of the linedef was
		activated.
--------------------------------------------------*/
bool CallFunc_LineSide(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argV;
	(void)argC;

	thread->dataStk.push(info->side);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_ClearLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		If there is an activating linedef, set its
		special to 0.
--------------------------------------------------*/
bool CallFunc_ClearLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argV;
	(void)argC;

	if (info->line != NULL)
	{
		// One time only.
		info->line->special = 0;
	}

	return false;
}

/*--------------------------------------------------
	bool CallFunc_EndPrint(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		One of the ACS wrappers for CEcho. This
		version only prints if the activator is a
		display player.
--------------------------------------------------*/
bool CallFunc_EndPrint(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	if (ACS_ActivatorIsLocal(thread) == true)
		HU_DoTitlecardCEcho(thread->printBuf.data());

	thread->printBuf.drop();
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pushes the number of players to ACS.
--------------------------------------------------*/
bool CallFunc_PlayerCount(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
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

	thread->dataStk.push(numPlayers);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_GameType(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pushes the current gametype to ACS.
--------------------------------------------------*/
bool CallFunc_GameType(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(gametype);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_GameSpeed(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pushes the current game speed to ACS.
--------------------------------------------------*/
bool CallFunc_GameSpeed(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(gamespeed);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_Timer(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Pushes leveltime to ACS.
--------------------------------------------------*/
bool CallFunc_Timer(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(leveltime);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SectorSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Plays a point sound effect from a sector.
--------------------------------------------------*/
bool CallFunc_SectorSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	ACSVM::MapScope *map = nullptr;
	ACSVM::String *str = nullptr;

	const char *sfxName = nullptr;
	size_t sfxLen = 0;

	sfxenum_t sfxId = sfx_None;
	INT32 vol = 0;
	mobj_t *origin = nullptr;

	(void)argC;

	map = thread->scopeMap;
	str = map->getString(argV[0]);

	sfxName = str->str;
	sfxLen = str->len;

	if (sfxLen > 0)
	{
		bool success = ACS_GetSFXFromString(sfxName, &sfxId);

		if (success == false)
		{
			// Exit early.

			CONS_Alert(CONS_WARNING,
				"Couldn't find sfx named \"%s\" for SectorSound.\n",
				sfxName
			);

			return false;
		}
	}

	vol = argV[1];

	if (info->sector != nullptr)
	{
		// New to Ring Racers: Use activating sector directly.
		origin = static_cast<mobj_t *>(static_cast<void *>(&info->sector->soundorg));
	}
	else if (info->line != nullptr)
	{
		// Original Hexen behavior: Use line's frontsector.
		origin = static_cast<mobj_t *>(static_cast<void *>(&info->line->frontsector->soundorg));
	}

	S_StartSoundAtVolume(origin, sfxId, vol);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_AmbientSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Plays a sound effect globally.
--------------------------------------------------*/
bool CallFunc_AmbientSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = nullptr;
	ACSVM::String *str = nullptr;

	const char *sfxName = nullptr;
	size_t sfxLen = 0;

	sfxenum_t sfxId = sfx_None;
	INT32 vol = 0;

	(void)argC;

	map = thread->scopeMap;
	str = map->getString(argV[0]);

	sfxName = str->str;
	sfxLen = str->len;

	if (sfxLen > 0)
	{
		bool success = ACS_GetSFXFromString(sfxName, &sfxId);

		if (success == false)
		{
			// Exit early.

			CONS_Alert(CONS_WARNING,
				"Couldn't find sfx named \"%s\" for AmbientSound.\n",
				sfxName
			);

			return false;
		}
	}

	vol = argV[1];

	S_StartSoundAtVolume(NULL, sfxId, vol);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetLineTexture(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Plays a sound effect globally.
--------------------------------------------------*/
enum
{
	SLT_POS_TOP,
	SLT_POS_MIDDLE,
	SLT_POS_BOTTOM
};

bool CallFunc_SetLineTexture(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	mtag_t tag = 0;
	UINT8 sideId = 0;
	UINT8 texPos = 0;

	ACSVM::MapScope *map = NULL;
	ACSVM::String *str = NULL;
	const char *texName = NULL;
	INT32 texId = LUMPERROR;

	INT32 lineId = -1;

	(void)argC;

	tag = argV[0];
	sideId = (argV[1] & 1);
	texPos = argV[2];

	map = thread->scopeMap;
	str = map->getString(argV[3]);
	texName = str->str;

	texId = R_TextureNumForName(texName);

	TAG_ITER_LINES(tag, lineId)
	{
		line_t *line = &lines[lineId];
		side_t *side = &sides[line->sidenum[sideId]];

		switch (texPos)
		{
			case SLT_POS_MIDDLE:
			{
				side->midtexture = texId;
				break;
			}
			case SLT_POS_BOTTOM:
			{
				side->bottomtexture = texId;
				break;
			}
			case SLT_POS_TOP:
			default:
			{
				side->toptexture = texId;
				break;
			}
		}
	}

	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Changes a linedef's special and arguments.
--------------------------------------------------*/
bool CallFunc_SetLineSpecial(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	mtag_t tag = 0;
	INT32 spec = 0;
	size_t numArgs = 0;

	INT32 lineId = -1;

	tag = argV[0];
	spec = argV[1];

	numArgs = std::min(std::max((signed)(argC - 2), 0), NUMLINEARGS);

	TAG_ITER_LINES(tag, lineId)
	{
		line_t *line = &lines[lineId];
		size_t i;

		if (info->line != nullptr && line == info->line)
		{
			continue;
		}

		line->special = spec;

		for (i = 0; i < numArgs; i++)
		{
			line->args[i] = argV[i + 2];
		}
	}

	return false;
}

/*--------------------------------------------------
	bool CallFunc_ThingSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Plays a sound effect for a tagged object.
--------------------------------------------------*/
bool CallFunc_ThingSound(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;

	ACSVM::MapScope *map = nullptr;
	ACSVM::String *str = nullptr;

	const char *sfxName = nullptr;
	size_t sfxLen = 0;

	mtag_t tag = 0;
	sfxenum_t sfxId = sfx_None;
	INT32 vol = 0;

	mobj_t *mobj = nullptr;

	(void)argC;

	tag = argV[0];

	map = thread->scopeMap;
	str = map->getString(argV[1]);

	sfxName = str->str;
	sfxLen = str->len;

	if (sfxLen > 0)
	{
		bool success = ACS_GetSFXFromString(sfxName, &sfxId);

		if (success == false)
		{
			// Exit early.

			CONS_Alert(CONS_WARNING,
				"Couldn't find sfx named \"%s\" for AmbientSound.\n",
				sfxName
			);

			return false;
		}
	}

	vol = argV[2];

	while ((mobj = P_FindMobjFromTID(tag, mobj, info->mo)) != nullptr)
	{
		S_StartSoundAtVolume(mobj, sfxId, vol);
	}

	return false;
}


/*--------------------------------------------------
	bool CallFunc_EndPrintBold(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		One of the ACS wrappers for CEcho. This
		version prints for all players.
--------------------------------------------------*/
bool CallFunc_EndPrintBold(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	HU_DoTitlecardCEcho(thread->printBuf.data());

	thread->printBuf.drop();
	return false;
}
/*--------------------------------------------------
	bool CallFunc_PlayerTeam(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's team ID.
--------------------------------------------------*/
bool CallFunc_PlayerTeam(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	UINT8 teamID = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		teamID = info->mo->player->ctfteam;
	}

	thread->dataStk.push(teamID);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerRings(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's ring count.
--------------------------------------------------*/
bool CallFunc_PlayerRings(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	SINT8 rings = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		rings = info->mo->player->rings;
	}

	thread->dataStk.push(rings);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerScore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's ring count.
--------------------------------------------------*/
bool CallFunc_PlayerScore(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	UINT32 score = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		score = info->mo->player->roundscore;
	}

	thread->dataStk.push(score);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_EndLog(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		One of the ACS wrappers for CONS_Printf.
		This version only prints if the activator
		is a display player.
--------------------------------------------------*/
bool CallFunc_EndLog(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	CONS_Printf("%s\n", thread->printBuf.data());
	thread->printBuf.drop();
	return false;
}

/*--------------------------------------------------
	bool CallFunc_strcmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		ACS wrapper for strcmp.
--------------------------------------------------*/
static int ACS_strcmp(ACSVM::String *a, ACSVM::String *b)
{
	for (char const *sA = a->str, *sB = b->str; ; ++sA, ++sB)
	{
		char cA = *sA, cB = *sB;

		if (cA != cB)
		{
			return (cA < cB) ? -1 : 1;
		}

		if (!cA)
		{
			return 0;
		}
	}
}

bool CallFunc_strcmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = NULL;

	ACSVM::String *strA = nullptr;
	ACSVM::String *strB = nullptr;

	(void)argC;

	map = thread->scopeMap;

	strA = map->getString(argV[0]);
	strB = map->getString(argV[1]);

	thread->dataStk.push(ACS_strcmp(strA, strB));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_strcasecmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		ACS wrapper for strcasecmp / stricmp.
--------------------------------------------------*/
static int ACS_strcasecmp(ACSVM::String *a, ACSVM::String *b)
{
	for (char const *sA = a->str, *sB = b->str; ; ++sA, ++sB)
	{
		char cA = std::tolower(*sA), cB = std::tolower(*sB);

		if (cA != cB)
		{
			return (cA < cB) ? -1 : 1;
		}

		if (!cA)
		{
			return 0;
		}
	}
}

bool CallFunc_strcasecmp(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	ACSVM::MapScope *map = NULL;

	ACSVM::String *strA = nullptr;
	ACSVM::String *strB = nullptr;

	(void)argC;

	map = thread->scopeMap;

	strA = map->getString(argV[0]);
	strB = map->getString(argV[1]);

	thread->dataStk.push(ACS_strcasecmp(strA, strB));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_CountEnemies(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the number of enemies in the tagged sectors.
--------------------------------------------------*/
bool ACS_EnemyFilter(mobj_t *mo)
{
	return ((mo->flags & (MF_ENEMY|MF_BOSS)) && mo->health > 0);
}

bool CallFunc_CountEnemies(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	mtag_t tag = 0;
	UINT32 count = 0;

	(void)argC;

	tag = argV[0];
	count = ACS_SectorTagThingCounter(tag, ACS_EnemyFilter);

	thread->dataStk.push(count);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_CountPushables(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the number of pushables in the tagged sectors.
--------------------------------------------------*/
bool ACS_PushableFilter(mobj_t *mo)
{
	return ((mo->flags & MF_PUSHABLE)
		|| ((mo->info->flags & MF_PUSHABLE) && mo->fuse));
}

bool CallFunc_CountPushables(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	mtag_t tag = 0;
	UINT32 count = 0;

	(void)argC;

	tag = argV[0];
	count = ACS_SectorTagThingCounter(tag, ACS_PushableFilter);

	thread->dataStk.push(count);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_HaveUnlockableTrigger(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if an unlockable trigger has been gotten.
--------------------------------------------------*/
bool CallFunc_HaveUnlockableTrigger(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	UINT8 id = 0;
	bool unlocked = false;
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argC;

	id = argV[0];

	if (id < 0 || id > 31) // limited by 32 bit variable
	{
		CONS_Printf("Bad unlockable trigger ID %d\n", id);
	}
	else if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		unlocked = (info->mo->player->roundconditions.unlocktriggers & (1 << id));
	}

	thread->dataStk.push(unlocked);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_HaveUnlockable(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if an unlockable has been gotten.
--------------------------------------------------*/
bool CallFunc_HaveUnlockable(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	UINT8 id = 0;
	bool unlocked = false;

	(void)argC;

	id = argV[0];

	if (id < 0 || id >= MAXUNLOCKABLES)
	{
		CONS_Printf("Bad unlockable ID %d\n", id);
	}
	else
	{
		unlocked = M_CheckNetUnlockByID(id);
	}

	thread->dataStk.push(unlocked);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerSkin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's skin name.
--------------------------------------------------*/
bool CallFunc_PlayerSkin(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	Environment *env = &ACSEnv;
	auto info = &static_cast<Thread *>(thread)->info;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		UINT8 skin = info->mo->player->skin;
		thread->dataStk.push(~env->getString( skins[skin].name )->idx);
		return false;
	}

	thread->dataStk.push(0);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_GetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating object's current dye.
--------------------------------------------------*/
bool CallFunc_GetObjectDye(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	Environment *env = &ACSEnv;
	auto info = &static_cast<Thread *>(thread)->info;
	UINT16 dye = SKINCOLOR_NONE;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false))
	{
		dye = (info->mo->player != NULL) ? info->mo->player->dye : info->mo->color;
	}

	thread->dataStk.push(~env->getString( skincolors[dye].name )->idx);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's number of Chaos Emeralds.
--------------------------------------------------*/
bool CallFunc_PlayerEmeralds(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	UINT8 count = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		count = K_NumEmeralds(info->mo->player);
	}

	thread->dataStk.push(count);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PlayerLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the activating player's current lap.
--------------------------------------------------*/
bool CallFunc_PlayerLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	auto info = &static_cast<Thread *>(thread)->info;
	UINT8 laps = 0;

	(void)argV;
	(void)argC;

	if ((info != NULL)
		&& (info->mo != NULL && P_MobjWasRemoved(info->mo) == false)
		&& (info->mo->player != NULL))
	{
		laps = info->mo->player->laps;
	}

	thread->dataStk.push(laps);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_LowestLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the lowest lap of all of the players in-game.
--------------------------------------------------*/
bool CallFunc_LowestLap(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(P_FindLowestLap());
	return false;
}

/*--------------------------------------------------
	bool CallFunc_EncoreMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if the map is in Encore Mode.
--------------------------------------------------*/
bool CallFunc_EncoreMode(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(encoremode);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_BreakTheCapsules(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if the map is in Break the Capsules.
--------------------------------------------------*/
bool CallFunc_BreakTheCapsules(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push(battleprisons);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_TimeAttack(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns if the map is a Time Attack session.
--------------------------------------------------*/
bool CallFunc_TimeAttack(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	thread->dataStk.push((modeattacking != ATTACKING_NONE));
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PodiumPosition(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Returns the best position of all non-CPU players.
--------------------------------------------------*/
bool CallFunc_PodiumPosition(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	UINT8 ret = MAXPLAYERS;
	INT32 i;

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

		if (player->bot == true)
		{
			continue;
		}

		ret = std::min(ret, player->position);
	}

	thread->dataStk.push(ret);
	return false;
}

/*--------------------------------------------------
	bool CallFunc_PodiumFinish(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Ends the podium sequence. Doesn't do anything
		outside of podium maps.
--------------------------------------------------*/
bool CallFunc_PodiumFinish(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	(void)argV;
	(void)argC;

	K_FinishCeremony();
	return false;
}

/*--------------------------------------------------
	bool CallFunc_SetLineRenderStyle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Changes a linedef's blend mode and alpha.
--------------------------------------------------*/
bool CallFunc_SetLineRenderStyle(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	mtag_t tag = 0;
	patchalphastyle_t blend = AST_COPY;
	fixed_t alpha = FRACUNIT;

	INT32 lineId = -1;

	tag = argV[0];

	switch (argV[1])
	{
		case TMB_TRANSLUCENT:
		default:
			blend = AST_COPY;
			break;
		case TMB_ADD:
			blend = AST_ADD;
			break;
		case TMB_SUBTRACT:
			blend = AST_SUBTRACT;
			break;
		case TMB_REVERSESUBTRACT:
			blend = AST_REVERSESUBTRACT;
			break;
		case TMB_MODULATE:
			blend = AST_MODULATE;
			break;
	}

	alpha = argV[2];
	alpha = std::clamp(alpha, 0, FRACUNIT);

	TAG_ITER_LINES(tag, lineId)
	{
		line_t *line = &lines[lineId];

		line->blendmode = blend;
		line->alpha = alpha;
	}

	return false;
}

/*--------------------------------------------------
	bool CallFunc_Get/SetSectorProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)

		Generic sector property management.
--------------------------------------------------*/
enum
{
	SECTOR_PROP_FLOORHEIGHT,
	SECTOR_PROP_CEILINGHEIGHT,
	SECTOR_PROP_FLOORPIC,
	SECTOR_PROP_CEILINGPIC,
	SECTOR_PROP_LIGHTLEVEL,
	SECTOR_PROP_FLOORLIGHTLEVEL,
	SECTOR_PROP_CEILINGLIGHTLEVEL,
	SECTOR_PROP_FLOORLIGHTABSOLUTE,
	SECTOR_PROP_CEILINGLIGHTABSOLUTE,
	SECTOR_PROP_SPECIAL,
	SECTOR_PROP_FLAGS,
	SECTOR_PROP_SPECIALFLAGS,
	SECTOR_PROP_GRAVITY,
	SECTOR_PROP_ACTIVATION,
	SECTOR_PROP_ACTION,
	SECTOR_PROP_ARG0,
	SECTOR_PROP_ARG1,
	SECTOR_PROP_ARG2,
	SECTOR_PROP_ARG3,
	SECTOR_PROP_ARG4,
	SECTOR_PROP_ARG5,
	SECTOR_PROP_ARG6,
	SECTOR_PROP_ARG7,
	SECTOR_PROP_ARG8,
	SECTOR_PROP_ARG9,
	SECTOR_PROP_ARG0STR,
	SECTOR_PROP_ARG1STR,
	SECTOR_PROP__MAX
};

bool CallFunc_GetSectorProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	Environment *env = &ACSEnv;

	mtag_t tag = 0;
	INT32 sectorID = -1;
	sector_t *sector = NULL;

	INT32 property = SECTOR_PROP__MAX;
	INT32 value = 0;

	tag = argV[0];
	if ((sectorID = Tag_Iterate_Sectors(tag, 0)) != -1)
	{
		sector = &sectors[ sectorID ];
	}

	property = argV[1];

	if (sector != NULL)
	{

#define PROP_INT(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( sector->y ); \
		break; \
	}

#define PROP_STR(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( sector->y )->idx ); \
		break; \
	}

#define PROP_FLAT(x, y) \
	case x: \
	{ \
		value = static_cast<INT32>( ~env->getString( levelflats[ sector->y ].name )->idx ); \
		break; \
	}

		switch (property)
		{
			PROP_INT(SECTOR_PROP_FLOORHEIGHT, floorheight)
			PROP_INT(SECTOR_PROP_CEILINGHEIGHT, ceilingheight)
			PROP_FLAT(SECTOR_PROP_FLOORPIC, floorpic)
			PROP_FLAT(SECTOR_PROP_CEILINGPIC, ceilingpic)
			PROP_INT(SECTOR_PROP_LIGHTLEVEL, lightlevel)
			PROP_INT(SECTOR_PROP_FLOORLIGHTLEVEL, floorlightlevel)
			PROP_INT(SECTOR_PROP_CEILINGLIGHTLEVEL, ceilinglightlevel)
			PROP_INT(SECTOR_PROP_FLOORLIGHTABSOLUTE, floorlightabsolute)
			PROP_INT(SECTOR_PROP_CEILINGLIGHTABSOLUTE, ceilinglightabsolute)
			PROP_INT(SECTOR_PROP_SPECIAL, special)
			PROP_INT(SECTOR_PROP_FLAGS, flags)
			PROP_INT(SECTOR_PROP_SPECIALFLAGS, specialflags)
			PROP_INT(SECTOR_PROP_GRAVITY, gravity)
			PROP_INT(SECTOR_PROP_ACTIVATION, activation)
			PROP_INT(SECTOR_PROP_ACTION, action)
			PROP_INT(SECTOR_PROP_ARG0, args[0])
			PROP_INT(SECTOR_PROP_ARG1, args[1])
			PROP_INT(SECTOR_PROP_ARG2, args[2])
			PROP_INT(SECTOR_PROP_ARG3, args[3])
			PROP_INT(SECTOR_PROP_ARG4, args[4])
			PROP_INT(SECTOR_PROP_ARG5, args[5])
			PROP_INT(SECTOR_PROP_ARG6, args[6])
			PROP_INT(SECTOR_PROP_ARG7, args[7])
			PROP_INT(SECTOR_PROP_ARG8, args[8])
			PROP_INT(SECTOR_PROP_ARG9, args[9])
			PROP_STR(SECTOR_PROP_ARG0STR, stringargs[0])
			PROP_STR(SECTOR_PROP_ARG1STR, stringargs[1])
			default:
			{
				CONS_Alert(CONS_WARNING, "GetSectorProperty type %d out of range (expected 0 - %d).\n", property, SECTOR_PROP__MAX-1);
				break;
			}
		}

#undef PROP_FLAT
#undef PROP_STR
#undef PROP_INT

	}

	thread->dataStk.push(value);
	return false;
}

bool CallFunc_SetSectorProperty(ACSVM::Thread *thread, const ACSVM::Word *argV, ACSVM::Word argC)
{
	//Environment *env = &ACSEnv;

	mtag_t tag = 0;
	INT32 sectorID = -1;
	sector_t *sector = NULL;

	INT32 property = SECTOR_PROP__MAX;
	INT32 value = 0;

	tag = argV[0];
	property = argV[1];
	value = argV[2];

	TAG_ITER_SECTORS(tag, sectorID)
	{
		sector = &sectors[sectorID];

#define PROP_READONLY(x, y) \
	case x: \
	{ \
		CONS_Alert(CONS_WARNING, "SetSectorProperty type '%s' cannot be written to.\n", "y"); \
		break; \
	}

#define PROP_INT(x, y) \
	case x: \
	{ \
		sector->y = static_cast< decltype(sector->y) >(value); \
		break; \
	}

#define PROP_STR(x, y) \
	case x: \
	{ \
		ACSVM::String *str = thread->scopeMap->getString( value ); \
		if (str->len == 0) \
		{ \
			Z_Free(sector->y); \
			sector->y = NULL; \
		} \
		else \
		{ \
			sector->y = static_cast<char *>(Z_Realloc(sector->y, str->len + 1, PU_LEVEL, NULL)); \
			M_Memcpy(sector->y, str->str, str->len + 1); \
			sector->y[str->len] = '\0'; \
		} \
		break; \
	}

#define PROP_FLAT(x, y) \
	case x: \
	{ \
		sector->y = P_AddLevelFlatRuntime( thread->scopeMap->getString( value )->str ); \
		break; \
	}

		switch (property)
		{
			PROP_INT(SECTOR_PROP_FLOORHEIGHT, floorheight)
			PROP_INT(SECTOR_PROP_CEILINGHEIGHT, ceilingheight)
			PROP_FLAT(SECTOR_PROP_FLOORPIC, floorpic)
			PROP_FLAT(SECTOR_PROP_CEILINGPIC, ceilingpic)
			PROP_INT(SECTOR_PROP_LIGHTLEVEL, lightlevel)
			PROP_INT(SECTOR_PROP_FLOORLIGHTLEVEL, floorlightlevel)
			PROP_INT(SECTOR_PROP_CEILINGLIGHTLEVEL, ceilinglightlevel)
			PROP_INT(SECTOR_PROP_FLOORLIGHTABSOLUTE, floorlightabsolute)
			PROP_INT(SECTOR_PROP_CEILINGLIGHTABSOLUTE, ceilinglightabsolute)
			PROP_INT(SECTOR_PROP_SPECIAL, special)
			PROP_INT(SECTOR_PROP_FLAGS, flags)
			PROP_INT(SECTOR_PROP_SPECIALFLAGS, specialflags)
			PROP_INT(SECTOR_PROP_GRAVITY, gravity)
			PROP_INT(SECTOR_PROP_ACTIVATION, activation)
			PROP_INT(SECTOR_PROP_ACTION, action)
			PROP_INT(SECTOR_PROP_ARG0, args[0])
			PROP_INT(SECTOR_PROP_ARG1, args[1])
			PROP_INT(SECTOR_PROP_ARG2, args[2])
			PROP_INT(SECTOR_PROP_ARG3, args[3])
			PROP_INT(SECTOR_PROP_ARG4, args[4])
			PROP_INT(SECTOR_PROP_ARG5, args[5])
			PROP_INT(SECTOR_PROP_ARG6, args[6])
			PROP_INT(SECTOR_PROP_ARG7, args[7])
			PROP_INT(SECTOR_PROP_ARG8, args[8])
			PROP_INT(SECTOR_PROP_ARG9, args[9])
			PROP_STR(SECTOR_PROP_ARG0STR, stringargs[0])
			PROP_STR(SECTOR_PROP_ARG1STR, stringargs[1])
			default:
			{
				CONS_Alert(CONS_WARNING, "SetSectorProperty type %d out of range (expected 0 - %d).\n", property, SECTOR_PROP__MAX-1);
				break;
			}
		}

#undef PROP_FLAT
#undef PROP_STR
#undef PROP_INT
#undef PROP_READONLY

	}

	return false;
}
