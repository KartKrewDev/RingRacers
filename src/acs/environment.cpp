// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2016 by James Haley, David Hill, et al. (Team Eternity)
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  environment.cpp
/// \brief Action Code Script: Environment definition

#include <algorithm>
#include <vector>

#include "acsvm.hpp"

#include "../doomtype.h"
#include "../doomdef.h"
#include "../doomstat.h"

#include "../r_defs.h"
#include "../r_state.h"
#include "../g_game.h"
#include "../p_spec.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../p_local.h"
#include "../k_dialogue.hpp"

#include "environment.hpp"
#include "thread.hpp"
#include "call-funcs.hpp"
#include "../cxxutil.hpp"

using namespace srb2::acs;

Environment ACSEnv;

Environment::Environment()
{
	ACSVM::GlobalScope *global = getGlobalScope(0);

	// Activate global scope immediately, since we don't want it off.
	// Not that we're adding any modules to it, though. :p
	global->active = true;

	// Set a branch limit (same as ZDoom's instruction limit)
	branchLimit = 2000000;

	// Add the data & function pointers.

	// Starting with raw ACS0 codes. I'm using this classic-style
	// format here to have a blueprint for what needs implementing,
	// but it'd also be fine to move these to new style.

	// See also:
	// - https://doomwiki.org/wiki/ACS0_instruction_set
	// - https://github.com/DavidPH/ACSVM/blob/master/ACSVM/CodeData.hpp
	// - https://github.com/DavidPH/ACSVM/blob/master/ACSVM/CodeList.hpp

	//  0 to 56: Implemented by ACSVM
	addCodeDataACS0( 57, {"",        2, addCallFunc(CallFunc_Random)});
	addCodeDataACS0( 58, {"WW",      0, addCallFunc(CallFunc_Random)});
	addCodeDataACS0( 59, {"",        2, addCallFunc(CallFunc_ThingCount)});
	addCodeDataACS0( 60, {"WW",      0, addCallFunc(CallFunc_ThingCount)});
	addCodeDataACS0( 61, {"",        1, addCallFunc(CallFunc_TagWait)});
	addCodeDataACS0( 62, {"W",       0, addCallFunc(CallFunc_TagWait)});
	addCodeDataACS0( 63, {"",        1, addCallFunc(CallFunc_PolyWait)});
	addCodeDataACS0( 64, {"W",       0, addCallFunc(CallFunc_PolyWait)});
	addCodeDataACS0( 65, {"",        2, addCallFunc(CallFunc_ChangeFloor)});
	addCodeDataACS0( 66, {"WWS",     0, addCallFunc(CallFunc_ChangeFloor)});
	addCodeDataACS0( 67, {"",        2, addCallFunc(CallFunc_ChangeCeiling)});
	addCodeDataACS0( 68, {"WWS",     0, addCallFunc(CallFunc_ChangeCeiling)});
	// 69 to 79: Implemented by ACSVM
	addCodeDataACS0( 80, {"",        0, addCallFunc(CallFunc_LineSide)});
	// 81 to 82: Implemented by ACSVM
	addCodeDataACS0( 83, {"",        0, addCallFunc(CallFunc_ClearLineSpecial)});
	// 84 to 85: Implemented by ACSVM
	addCodeDataACS0( 86, {"",        0, addCallFunc(CallFunc_EndPrint)});
	// 87 to 89: Implemented by ACSVM
	addCodeDataACS0( 90, {"",        0, addCallFunc(CallFunc_PlayerCount)});
	addCodeDataACS0( 91, {"",        0, addCallFunc(CallFunc_GameType)});
	addCodeDataACS0( 92, {"",        0, addCallFunc(CallFunc_GameSpeed)});
	addCodeDataACS0( 93, {"",        0, addCallFunc(CallFunc_Timer)});
	addCodeDataACS0( 94, {"",        2, addCallFunc(CallFunc_SectorSound)});
	addCodeDataACS0( 95, {"",        2, addCallFunc(CallFunc_AmbientSound)});

	addCodeDataACS0( 97, {"",        4, addCallFunc(CallFunc_SetLineTexture)});
	addCodeDataACS0( 98, {"",        2, addCallFunc(CallFunc_SetLineBlocking)});
	addCodeDataACS0( 99, {"",        7, addCallFunc(CallFunc_SetLineSpecial)});
	addCodeDataACS0(100, {"",        3, addCallFunc(CallFunc_ThingSound)});
	addCodeDataACS0(101, {"",        0, addCallFunc(CallFunc_EndPrintBold)});

	addCodeDataACS0(118, {"",        0, addCallFunc(CallFunc_IsNetworkGame)});
	addCodeDataACS0(119, {"",        0, addCallFunc(CallFunc_PlayerTeam)});
	addCodeDataACS0(120, {"",        0, addCallFunc(CallFunc_PlayerRings)});

	addCodeDataACS0(122, {"",        0, addCallFunc(CallFunc_PlayerScore)});

	// 136 to 137: Implemented by ACSVM

	// 157: Implemented by ACSVM

	// 167 to 173: Implemented by ACSVM
	addCodeDataACS0(174, {"BB",      0, addCallFunc(CallFunc_Random)});
	// 175 to 179: Implemented by ACSVM

	// 181 to 189: Implemented by ACSVM

	// 203 to 217: Implemented by ACSVM

	// 225 to 243: Implemented by ACSVM

	addCodeDataACS0(247, {"",        0, addCallFunc(CallFunc_PlayerNumber)});
	addCodeDataACS0(248, {"",        0, addCallFunc(CallFunc_ActivatorTID)});

	// 253: Implemented by ACSVM

	// 256 to 257: Implemented by ACSVM

	// 263: Implemented by ACSVM
	addCodeDataACS0(270, {"",        0, addCallFunc(CallFunc_EndLog)});
	// 273 to 275: Implemented by ACSVM

	// 291 to 325: Implemented by ACSVM

	// 330: Implemented by ACSVM

	// 349 to 361: Implemented by ACSVM

	// 363 to 380: Implemented by ACSVM

	// Now for new style functions.
	// This style is preferred for added functions
	// that aren't mimicing one from Hexen's or ZDoom's
	// ACS implementations.
	addFuncDataACS0(   1, addCallFunc(CallFunc_GetLineProperty));
	addFuncDataACS0(   2, addCallFunc(CallFunc_SetLineProperty));
	addFuncDataACS0(   3, addCallFunc(CallFunc_GetLineUserProperty));
	addFuncDataACS0(   4, addCallFunc(CallFunc_GetSectorProperty));
	addFuncDataACS0(   5, addCallFunc(CallFunc_SetSectorProperty));
	addFuncDataACS0(   6, addCallFunc(CallFunc_GetSectorUserProperty));
	addFuncDataACS0(   7, addCallFunc(CallFunc_GetSideProperty));
	addFuncDataACS0(   8, addCallFunc(CallFunc_SetSideProperty));
	addFuncDataACS0(   9, addCallFunc(CallFunc_GetSideUserProperty));
	addFuncDataACS0(  10, addCallFunc(CallFunc_GetThingProperty));
	addFuncDataACS0(  11, addCallFunc(CallFunc_SetThingProperty));
	addFuncDataACS0(  12, addCallFunc(CallFunc_GetThingUserProperty));
	//addFuncDataACS0(  13, addCallFunc(CallFunc_GetPlayerProperty));
	//addFuncDataACS0(  14, addCallFunc(CallFunc_SetPlayerProperty));
	//addFuncDataACS0(  15, addCallFunc(CallFunc_GetPolyobjProperty));
	//addFuncDataACS0(  16, addCallFunc(CallFunc_SetPolyobjProperty));

	addFuncDataACS0( 100, addCallFunc(CallFunc_strcmp));
	addFuncDataACS0( 101, addCallFunc(CallFunc_strcasecmp));

	addFuncDataACS0( 300, addCallFunc(CallFunc_CountEnemies));
	addFuncDataACS0( 301, addCallFunc(CallFunc_CountPushables));
	addFuncDataACS0( 302, addCallFunc(CallFunc_HaveUnlockableTrigger));
	addFuncDataACS0( 303, addCallFunc(CallFunc_HaveUnlockable));
	addFuncDataACS0( 304, addCallFunc(CallFunc_PlayerSkin));
	addFuncDataACS0( 305, addCallFunc(CallFunc_GetObjectDye));
	addFuncDataACS0( 306, addCallFunc(CallFunc_PlayerEmeralds));
	addFuncDataACS0( 307, addCallFunc(CallFunc_PlayerLap));
	addFuncDataACS0( 308, addCallFunc(CallFunc_LowestLap));
	addFuncDataACS0( 309, addCallFunc(CallFunc_EncoreMode));
	addFuncDataACS0( 310, addCallFunc(CallFunc_PrisonBreak));
	addFuncDataACS0( 311, addCallFunc(CallFunc_TimeAttack));
	addFuncDataACS0( 312, addCallFunc(CallFunc_ThingCount));
	addFuncDataACS0( 313, addCallFunc(CallFunc_GrandPrix));
	addFuncDataACS0( 314, addCallFunc(CallFunc_GetGrabbedSprayCan));
	addFuncDataACS0( 315, addCallFunc(CallFunc_PlayerBot));
	addFuncDataACS0( 316, addCallFunc(CallFunc_PositionStart));
	addFuncDataACS0( 317, addCallFunc(CallFunc_FreePlay));
	addFuncDataACS0( 318, addCallFunc(CallFunc_CheckTutorialChallenge));
	addFuncDataACS0( 319, addCallFunc(CallFunc_PlayerLosing));
	addFuncDataACS0( 320, addCallFunc(CallFunc_PlayerExiting));

	addFuncDataACS0( 500, addCallFunc(CallFunc_CameraWait));
	addFuncDataACS0( 501, addCallFunc(CallFunc_PodiumPosition));
	addFuncDataACS0( 502, addCallFunc(CallFunc_PodiumFinish));
	addFuncDataACS0( 503, addCallFunc(CallFunc_SetLineRenderStyle));
	addFuncDataACS0( 504, addCallFunc(CallFunc_MapWarp));
	addFuncDataACS0( 505, addCallFunc(CallFunc_AddBot));
	addFuncDataACS0( 506, addCallFunc(CallFunc_StopLevelExit));
	addFuncDataACS0( 507, addCallFunc(CallFunc_ExitLevel));
	addFuncDataACS0( 508, addCallFunc(CallFunc_MusicPlay));
	addFuncDataACS0( 509, addCallFunc(CallFunc_MusicStopAll));
	addFuncDataACS0( 510, addCallFunc(CallFunc_MusicRemap));
	addFuncDataACS0( 511, addCallFunc(CallFunc_Freeze));
	addFuncDataACS0( 512, addCallFunc(CallFunc_MusicDim));

	addFuncDataACS0( 600, addCallFunc(CallFunc_DialogueSetSpeaker));
	addFuncDataACS0( 601, addCallFunc(CallFunc_DialogueSetCustomSpeaker));
	addFuncDataACS0( 602, addCallFunc(CallFunc_DialogueNewText));
	addFuncDataACS0( 603, addCallFunc(CallFunc_DialogueWaitDismiss));
	addFuncDataACS0( 604, addCallFunc(CallFunc_DialogueWaitText));
	addFuncDataACS0( 605, addCallFunc(CallFunc_DialogueAutoDismiss));

	addFuncDataACS0( 700, addCallFunc(CallFunc_AddMessage));
	addFuncDataACS0( 701, addCallFunc(CallFunc_AddMessageForPlayer));
	addFuncDataACS0( 702, addCallFunc(CallFunc_ClearPersistentMessages));
	addFuncDataACS0( 703, addCallFunc(CallFunc_ClearPersistentMessageForPlayer));
}

ACSVM::Thread *Environment::allocThread()
{
	return new Thread(this);
}

ACSVM::ModuleName Environment::getModuleName(char const *str, size_t len)
{
	ACSVM::String *name = getString(str, len);
	lumpnum_t lump = W_CheckNumForNameInFolder(str, "ACS/");

	return { name, nullptr, static_cast<size_t>(lump) };
}

void Environment::loadModule(ACSVM::Module *module)
{
	ACSVM::ModuleName *const name = &module->name;

	size_t lumpLen = 0;
	std::vector<ACSVM::Byte> data;

	if (name->i == (size_t)LUMPERROR)
	{
		// No lump given for module.
		CONS_Alert(CONS_WARNING, "Could not find ACS module \"%s\"; scripts will not function properly!\n", name->s->str);
		return; //throw ACSVM::ReadError("file open failure");
	}

	lumpLen = W_LumpLength(name->i);

	if (W_IsLumpWad(name->i) == true || lumpLen == 0)
	{
		CONS_Debug(DBG_SETUP, "Attempting to load ACS module from the BEHAVIOR lump of map '%s'...\n", name->s->str);

		// The lump given is a virtual resource.
		// Try to grab a BEHAVIOR lump from inside of it.
		virtres_t *vRes = vres_GetMap(name->i);
		auto _ = srb2::finally([vRes]() { vres_Free(vRes); });

		virtlump_t *vLump = vres_Find(vRes, "BEHAVIOR");
		if (vLump != nullptr && vLump->size > 0)
		{
			data.insert(data.begin(), vLump->data, vLump->data + vLump->size);
			CONS_Debug(DBG_SETUP, "Successfully found BEHAVIOR lump.\n");
		}
		else
		{
			CONS_Debug(DBG_SETUP, "No BEHAVIOR lump found.\n");
		}
	}
	else
	{
		CONS_Debug(DBG_SETUP, "Loading ACS module directly from lump '%s'...\n", name->s->str);

		// It's a real lump.
		ACSVM::Byte *lump = static_cast<ACSVM::Byte *>(Z_Calloc(lumpLen, PU_STATIC, nullptr));
		auto _ = srb2::finally([lump]() { Z_Free(lump); });

		W_ReadLump(name->i, lump);
		data.insert(data.begin(), lump, lump + lumpLen);
	}

	if (data.empty() == false)
	{
		try
		{
			module->readBytecode(data.data(), data.size());
		}
		catch (const ACSVM::ReadError &e)
		{
			CONS_Alert(CONS_ERROR, "Failed to load ACS module '%s': %s\n", name->s->str, e.what());
			throw ACSVM::ReadError("failed import");
		}
	}
	else
	{
		// Unlike Hexen, a BEHAVIOR lump is not required.
		// Simply ignore in this instance.
		CONS_Debug(DBG_SETUP, "ACS module has no data, ignoring...\n");
	}
}

bool Environment::checkTag(ACSVM::Word type, ACSVM::Word tag)
{
	switch (type)
	{
		case ACS_TAGTYPE_SECTOR:
		{
			INT32 secnum = -1;

			TAG_ITER_SECTORS(tag, secnum)
			{
				sector_t *sec = &sectors[secnum];

				if (sec->floordata != nullptr || sec->ceilingdata != nullptr)
				{
					return false;
				}
			}

			return true;
		}

		case ACS_TAGTYPE_POLYOBJ:
		{
			const polyobj_t *po = Polyobj_GetForNum(tag);
			return (po == nullptr || po->thinker == nullptr);
		}

		case ACS_TAGTYPE_CAMERA:
		{
			const mobj_t *camera = P_FindObjectTypeFromTag(MT_ALTVIEWMAN, tag);
			if (camera == nullptr)
			{
				return true;
			}

			return (camera->tracer == nullptr || P_MobjWasRemoved(camera->tracer) == true);
		}

		case ACS_TAGTYPE_DIALOGUE:
		{
			// TODO when we move away from g_dialogue
			//  See also call-funcs.cpp Dialogue_ValidCheck
			if (netgame || !g_dialogue.EraIsValid(tag)) // cheeky reuse
			{
				return true;
			}

			if (g_dialogue.Dismissable())
			{
				// wait for dismissal
				return (!g_dialogue.Active());
			}
			else
			{
				// wait for text to finish
				return (g_dialogue.TextDone());
			}
		}
	}

	return true;
}

ACSVM::Word Environment::callSpecImpl
	(
		ACSVM::Thread *thread, ACSVM::Word spec,
		const ACSVM::Word *argV, ACSVM::Word argC
	)
{
	auto info = &static_cast<Thread *>(thread)->info;
	ACSVM::MapScope *const map = thread->scopeMap;

	INT32 args[NUM_SCRIPT_ARGS] = {0};

	char *stringargs[NUM_SCRIPT_STRINGARGS] = {0};
	auto _ = srb2::finally(
		[stringargs]()
		{
			for (int i = 0; i < NUM_SCRIPT_STRINGARGS; i++)
			{
				Z_Free(stringargs[i]);
			}
		}
	);

	activator_t *activator = static_cast<activator_t *>(Z_Calloc(sizeof(activator_t), PU_LEVEL, nullptr));
	auto __ = srb2::finally(
		[info, activator]()
		{
			if (info->thread_era == thinker_era)
			{
				P_SetTarget(&activator->mo, NULL);
				Z_Free(activator);
			}
		}
	);

	int i = 0;

	for (i = 0; i < std::min((signed)(argC), NUM_SCRIPT_STRINGARGS); i++)
	{
		ACSVM::String *strPtr = map->getString(argV[i]);

		stringargs[i] = static_cast<char *>(Z_Malloc(strPtr->len + 1, PU_STATIC, nullptr));
		M_Memcpy(stringargs[i], strPtr->str, strPtr->len + 1);
	}

	for (i = 0; i < std::min((signed)(argC), NUM_SCRIPT_ARGS); i++)
	{
		args[i] = argV[i];
	}

	P_SetTarget(&activator->mo, info->mo);
	activator->line = info->line;
	activator->side = info->side;
	activator->sector = info->sector;
	activator->po = info->po;
	activator->fromLineSpecial = false;

	P_ProcessSpecial(activator, spec, args, stringargs);
	return 1;
}

void Environment::printKill(ACSVM::Thread *thread, ACSVM::Word type, ACSVM::Word data)
{
	std::string scriptName;

	ACSVM::String *scriptNamePtr = (thread->script != nullptr) ? (thread->script->name.s) : nullptr;
	if (scriptNamePtr && scriptNamePtr->len)
		scriptName = std::string(scriptNamePtr->str);
	else
		scriptName = std::to_string((int)thread->script->name.i);

	ACSVM::KillType killType = static_cast<ACSVM::KillType>(type);

	if (killType == ACSVM::KillType::BranchLimit)
	{
		CONS_Alert(CONS_ERROR, "Terminated runaway script %s\n", scriptName.c_str());
		return;
	}
	else if (killType == ACSVM::KillType::UnknownCode)
	{
		CONS_Alert(CONS_ERROR, "ACSVM ERROR: Unknown opcode %d in script %s\n", data, scriptName.c_str());
	}
	else if (killType == ACSVM::KillType::UnknownFunc)
	{
		CONS_Alert(CONS_ERROR, "ACSVM ERROR: Unknown function %d in script %s\n", data, scriptName.c_str());
	}
	else if (killType == ACSVM::KillType::OutOfBounds)
	{
		CONS_Alert(CONS_ERROR, "ACSVM ERROR: Jumped to out of bounds location %s in script %s\n",
			sizeu1(thread->codePtr - thread->module->codeV.data() - 1), scriptName.c_str());
	}
	else
	{
		CONS_Alert(CONS_ERROR, "ACSVM ERROR: Kill %u:%d at %s in script %s\n",
			type, data, sizeu1(thread->codePtr - thread->module->codeV.data() - 1), scriptName.c_str());
	}

	CONS_Printf("Script terminated.\n");
}
