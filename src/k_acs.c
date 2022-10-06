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
/// \file  k_acs.c
/// \brief Action Code Script implementation using ACSVM

#include "k_acs.h"

#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
#include "z_zone.h"
#include "w_wad.h"
#include "i_system.h"
#include "r_defs.h"
#include "r_state.h"
#include "p_polyobj.h"
#include "taglist.h"

#include "CAPI/BinaryIO.h"
#include "CAPI/Environment.h"
#include "CAPI/Module.h"
#include "CAPI/PrintBuf.h"
#include "CAPI/Scope.h"
#include "CAPI/String.h"
#include "CAPI/Thread.h"

static ACSVM_Environment *ACSenv = NULL;

/*--------------------------------------------------
	ACSVM_Environment *ACS_GetEnvironment(void)

		See header file for description.
--------------------------------------------------*/
ACSVM_Environment *ACS_GetEnvironment(void)
{
	return ACSenv;
}

/*--------------------------------------------------
	static void ACS_EnvBadAlloc(ACSVM_Environment *env, char const *what)

		ACSVM Environment hook. Runs in case of a memory
		allocation failure occuring. Environment state
		afterwards is unusable; the only thing safe to do
		is using ACSVM_FreeEnvironment.

	Input Arguments:-
		env - The ACS environment data.
		what - Error string.

	Return:-
		N/A
--------------------------------------------------*/
static void ACS_EnvBadAlloc(ACSVM_Environment *env, char const *what)
{
	(void)env;

	CONS_Alert(CONS_ERROR, "Error allocating memory for ACS (%s)\n", what);

	if (env == ACSenv)
	{
		// Restart the main environment.
		ACS_Shutdown();
		I_RemoveExitFunc(ACS_Shutdown); // Since ACS_Init will add it again.
		ACS_Init();
	}
}

/*--------------------------------------------------
	static void ACS_EnvReadError(ACSVM_Environment *env, char const *what)

		ACSVM Environment hook. Runs when an ACS module
		fails to read. Environment state should be safe
		afterwards.

	Input Arguments:-
		env - The ACS environment data.
		what - Error string.

	Return:-
		N/A
--------------------------------------------------*/
static void ACS_EnvReadError(ACSVM_Environment *env, char const *what)
{
	(void)env;
	CONS_Alert(CONS_WARNING, "Error reading ACS module (%s)\n", what);
}

/*--------------------------------------------------
	static void ACS_EnvSerialError(ACSVM_Environment *env, char const *what)

		ACSVM Environment hook. Runs when the ACS state
		fails to save or load. Environment state is
		safe in that it shouldn't be causing crashes,
		but it is indeterminate.

	Input Arguments:-
		env - The ACS environment data.
		what - Error string.

	Return:-
		N/A
--------------------------------------------------*/
static void ACS_EnvSerialError(ACSVM_Environment *env, char const *what)
{
	(void)env;
	CONS_Alert(CONS_WARNING, "Error serializing ACS state (%s)\n", what);
}

/*--------------------------------------------------
	static void ACS_EnvThreadKilled(ACSVM_Environment *env, char const *what)

		ACSVM Environment hook. Runs when the thread
		has been killed for whatever reason, so that
		the console can warn the user about it.

	Input Arguments:-
		env - The ACS environment the thread is from.
		thread - The thread that was stopped.
		reason - The reason the thread was stopped. See ACSVM_KillType.
		data - Bytecode data at time of stopping.

	Return:-
		N/A
--------------------------------------------------*/
static void ACS_EnvThreadKilled(ACSVM_Environment const *env, ACSVM_Thread *thread, ACSVM_Word reason, ACSVM_Word data)
{
	static const char *strings[] = {
		"Just for fun", // ACSVM_KillType_None
		"Out of bounds", // ACSVM_KillType_OutOfBounds
		"Unknown code", // ACSVM_KillType_UnknownCode
		"Unknown function", // ACSVM_KillType_UnknownFunc
		"Reached recursion limit" // ACSVM_KillType_BranchLimit
	};

	(void)env;
	(void)thread;
	(void)data;

	CONS_Alert(CONS_ERROR, "ACS thread killed (%s)\n", strings[reason]);
}

/*--------------------------------------------------
	static void ACS_AddCodeDataCallFunc(
		ACSVM_Environment *env, ACSVM_Word code,
		char const *args, ACSVM_Word stack, ACSVM_Word func)

		Shortcut function to simplify adding
		CallFuncs. These are for code data ones;
		accessible in all ACS formats.

	Input Arguments:-
		env - The ACS environment data to add this to.
		code - The byte code for this function.
		args - A string of arguments to use. The
			letters represent different operations
			to preform on the stack.
		argc - Number of arguments to pull from
			the stack, if not using args.
		func - The function to add.

	Return:-
		N/A
--------------------------------------------------*/
static inline void ACS_AddCodeDataCallFunc(ACSVM_Environment *env, ACSVM_Word code, char const *args, ACSVM_Word argc, ACSVM_CallFunc func)
{
	ACSVM_Environment_AddCodeDataACS0(
		env,
		code,
		args,
		((argc > 0) ? ACSVM_Code_CallFunc_Lit : ACSVM_Code_CallFunc),
		argc,
		ACSVM_Environment_AddCallFunc(env, func)
	);
}

/*--------------------------------------------------
	static void ACS_EnvConstruct(ACSVM_Environment *env)

		ACSVM Environment hook. Runs when the
		environment is initally created.

	Input Arguments:-
		env - The ACS environment data to construct.

	Return:-
		N/A
--------------------------------------------------*/
static void ACS_EnvConstruct(ACSVM_Environment *env)
{
	ACSVM_GlobalScope *global = ACSVM_Environment_GetGlobalScope(env, 0);

	// Activate global scope immediately, since we don't want it off.
	// Not that we're adding any modules to it, though. :p
	ACSVM_GlobalScope_SetActive(global, true);

	// Add the data & function pointers
	// See also:
	// - https://doomwiki.org/wiki/ACS0_instruction_set
	// - https://github.com/DavidPH/ACSVM/blob/master/ACSVM/CodeData.hpp
	// - https://github.com/DavidPH/ACSVM/blob/master/ACSVM/CodeList.hpp

	//  0 to 56: Implemented by ACSVM
	ACS_AddCodeDataCallFunc(env,   57, "",        2, ACS_CF_Random);
	ACS_AddCodeDataCallFunc(env,   58, "WW",      0, ACS_CF_Random);

	ACS_AddCodeDataCallFunc(env,   61, "",        1, ACS_CF_TagWait);
	ACS_AddCodeDataCallFunc(env,   62, "W",       0, ACS_CF_TagWait);
	ACS_AddCodeDataCallFunc(env,   63, "",        1, ACS_CF_PolyWait);
	ACS_AddCodeDataCallFunc(env,   64, "W",       0, ACS_CF_PolyWait);
	// 69 to 79: Implemented by ACSVM

	// 81 to 82: Implemented by ACSVM

	// 84 to 85: Implemented by ACSVM
	ACS_AddCodeDataCallFunc(env,   86, "",        0, ACS_CF_EndPrint);
	// 87 to 89: Implemented by ACSVM
	ACS_AddCodeDataCallFunc(env,   90, "",        0, ACS_CF_PlayerCount);
	ACS_AddCodeDataCallFunc(env,   91, "",        0, ACS_CF_GameType);
	ACS_AddCodeDataCallFunc(env,   92, "",        0, ACS_CF_GameSpeed);
	ACS_AddCodeDataCallFunc(env,   93, "",        0, ACS_CF_Timer);
	// 136 to 137: Implemented by ACSVM

	// 157: Implemented by ACSVM

	// 167 to 173: Implemented by ACSVM
	ACS_AddCodeDataCallFunc(env,  174, "BB",      0, ACS_CF_Random);
	// 175 to 179: Implemented by ACSVM

	// 181 to 189: Implemented by ACSVM

	// 203 to 217: Implemented by ACSVM

	// 225 to 243: Implemented by ACSVM

	// 253: Implemented by ACSVM

	// 256 to 257: Implemented by ACSVM

	// 263: Implemented by ACSVM
	ACS_AddCodeDataCallFunc(env,  270, "",        0, ACS_CF_EndPrint);
	// 273 to 275: Implemented by ACSVM

	// 291 to 325: Implemented by ACSVM

	// 330: Implemented by ACSVM

	// 349 to 361: Implemented by ACSVM

	// 363 to 380: Implemented by ACSVM
}

/*--------------------------------------------------
	static void ACS_EnvLoadModule(ACSVM_Environment *env, ACSVM_Module *module)

		ACSVM Environment hook. Runs when a ACS
		module is being loaded.

	Input Arguments:-
		env - The ACS environment data.
		module - The ACS module being loaded.

	Return:-
		true when successful, otherwise false.
		Returning false will also call the
		ACS_EnvReadError hook.
--------------------------------------------------*/
static bool ACS_EnvLoadModule(ACSVM_Environment *env, ACSVM_Module *module)
{
	ACSVM_ModuleName name = ACSVM_Module_GetName(module);
	const char *str = ACSVM_String_GetStr(name.s);

	size_t lumpLen = 0;

	ACSVM_Byte *data = NULL;
	size_t size = 0;

	bool ret = false;

	(void)env;

	if (name.i == (size_t)LUMPERROR)
	{
		// No lump given for module.
		CONS_Alert(CONS_ERROR, "Bad lump for ACS module \"%s\"\n", str);
		return false;
	}

	lumpLen = W_LumpLength(name.i);

	if (W_IsLumpWad(name.i) == true || lumpLen == 0)
	{
		// The lump given is a virtual resource.
		// Try to grab it from there.
		virtres_t *vRes = vres_GetMap(name.i);
		virtlump_t *vLump = vres_Find(vRes, "BEHAVIOR");

		CONS_Printf("Attempting to load ACS module from map's virtual resource...\n");

		if (vLump != NULL)
		{
			data = Z_Calloc(vLump->size, PU_STATIC, NULL);
			memcpy(data, vLump->data, vLump->size);
			size = vLump->size;
			CONS_Printf("Successfully found BEHAVIOR lump.\n");
		}
		else
		{
			CONS_Printf("No BEHAVIOR lump found.\n");
		}
	}
	else
	{
		// It's a real lump.
		data = Z_Calloc(lumpLen, PU_STATIC, NULL);
		W_ReadLump(name.i, data);
		size = lumpLen;
		CONS_Printf("Loading ACS module directly from lump.\n");
	}

	if (data != NULL && size > 0)
	{
		CONS_Printf("Reading bytecode of ACS module...\n");
		ret = ACSVM_Module_ReadBytecode(module, data, size);
	}
	else
	{
		// Unlike Hexen, BEHAVIOR is not required.
		// Simply ignore in this instance.
		CONS_Printf("No data received, ignoring...\n");
		ret = true;
	}

	Z_Free(data);
	return ret;
}

/*--------------------------------------------------
	static bool ACS_EnvCheckTag(ACSVM_Environment const *env, ACSVM_Word type, ACSVM_Word tag)

		ACSVM Environment hook. Ran to determine
		whenever or not a thread should still be
		waiting on a tag movement.
		See: TagWait, PolyWait.

	Input Arguments:-
		env - The ACS environment data.
		type - The kind of level data we're waiting on. See also: acs_tagType_e.
		tag - The tag of said level data.

	Return:-
		true when the tag is done moving and
		execution can continue, or false to keep
		the thread paused.
--------------------------------------------------*/
static bool ACS_EnvCheckTag(ACSVM_Environment const *env, ACSVM_Word type, ACSVM_Word tag)
{
	(void)env;

	switch (type)
	{
		case ACS_TAGTYPE_SECTOR:
		{
			INT32 secnum = -1;

			TAG_ITER_SECTORS(tag, secnum)
			{
				sector_t *sec = &sectors[secnum];

				if (sec->floordata != NULL || sec->ceilingdata != NULL)
				{
					return false;
				}
			}

			return true;
		}

		case ACS_TAGTYPE_POLYOBJ:
		{
			const polyobj_t *po = Polyobj_GetForNum(tag);
			return (po == NULL || po->thinker == NULL);
		}
	}

	return true;
}

/*--------------------------------------------------
	void ACS_Init(void)

		See header file for description.
--------------------------------------------------*/
void ACS_Init(void)
{
	// Initialize ACS on engine start-up.
	ACSVM_EnvironmentFuncs funcs = {0};

	funcs.bad_alloc = ACS_EnvBadAlloc;
	funcs.readError = ACS_EnvReadError;
	funcs.serialError = ACS_EnvSerialError;
	funcs.printKill = ACS_EnvThreadKilled;
	funcs.ctor = ACS_EnvConstruct;
	funcs.loadModule = ACS_EnvLoadModule;
	funcs.checkTag = ACS_EnvCheckTag;

	ACSenv = ACSVM_AllocEnvironment(&funcs, NULL);

	I_AddExitFunc(ACS_Shutdown);
}

/*--------------------------------------------------
	void ACS_Shutdown(void)

		See header file for description.
--------------------------------------------------*/
void ACS_Shutdown(void)
{
	// Delete ACS environment.
	ACSVM_FreeEnvironment(ACSenv);
	ACSenv = NULL;
}

/*--------------------------------------------------
	static void ACS_ResetHub(ACSVM_GlobalScope *global)

		Shortcut function to quickly free the
		only hub scope Ring Racers uses.

	Input Arguments:-
		global - The global scope to free the hub from.

	Return:-
		N/A
--------------------------------------------------*/
static void ACS_ResetHub(ACSVM_GlobalScope *global)
{
	ACSVM_HubScope *hub = ACSVM_GlobalScope_GetHubScope(global, 0);
	ACSVM_GlobalScope_FreeHubScope(global, hub);
}

/*--------------------------------------------------
	static void ACS_ResetMap(ACSVM_HubScope *hub)

		Shortcut function to quickly free the
		only map scope Ring Racers uses.

	Input Arguments:-
		hub - The hub scope to free the map from.

	Return:-
		N/A
--------------------------------------------------*/
static void ACS_ResetMap(ACSVM_HubScope *hub)
{
	ACSVM_MapScope *map = ACSVM_HubScope_GetMapScope(hub, 0);
	ACSVM_HubScope_FreeMapScope(hub, map);
}

/*--------------------------------------------------
	void ACS_LoadLevelScripts(size_t mapID)

		See header file for description.
--------------------------------------------------*/
void ACS_LoadLevelScripts(size_t mapID)
{
	ACSVM_Environment *env = ACSenv;
	ACSVM_StringTable *strTab = ACSVM_Environment_GetStringTable(env);

	ACSVM_GlobalScope *global = NULL;
	ACSVM_HubScope *hub = NULL;
	ACSVM_MapScope *map = NULL;

	ACSVM_Module **modules = NULL;
	size_t modules_len = 0;
	size_t modules_size = 4;

	global = ACSVM_Environment_GetGlobalScope(ACSenv, 0);

	// Just some notes on how Hexen's scopes work, if anyone
	// intends to implement proper hub logic:

	// The integer is an ID for which hub / map it is,
	// and instead sets active according to which ones
	// should run, since you can go between them.

	// But I didn't intend on implementing these features,
	// since hubs aren't planned for Ring Racers (although
	// they might be useful for SRB2), and I intentionally
	// avoided implementing global ACS (since Lua would be
	// a better language to do that kind of code).

	// Since we literally only are using map scope, we can
	// just free everything between every level. But if
	// hubs are to be implemented, this logic would need
	// to be far more sophisticated.

	// Reset hub scope, even if we are not using it.
	ACS_ResetHub(global);
	hub = ACSVM_GlobalScope_GetHubScope(global, 0);
	ACSVM_HubScope_SetActive(hub, true);

	// Start up new map scope.
	ACS_ResetMap(hub);
	map = ACSVM_HubScope_GetMapScope(hub, 0);
	ACSVM_MapScope_SetActive(map, true);

	// Allocate module list.
	modules = Z_Calloc(modules_size * sizeof(ACSVM_Module *), PU_STATIC, NULL);

	// Insert BEHAVIOR lump into the list.
	{
		char const *str = mapheaderinfo[mapID]->lumpname;
		size_t len = strlen(str);
		size_t hash = ACSVM_StrHash(str, len);

		ACSVM_ModuleName name = {0};

		name.s = ACSVM_StringTable_GetStringByData(strTab, str, len, hash);
		name.i = mapheaderinfo[mapID]->lumpnum;

		if (modules_len >= modules_size)
		{
			modules_size *= 2;
			modules = Z_Realloc(modules, modules_size * sizeof(ACSVM_Module *), PU_STATIC, &modules);
		}

		modules[modules_len] = ACSVM_Environment_GetModule(env, name);
		modules_len++;
	}

	if (modules_len > 0)
	{
		// Register the modules with map scope.
		ACSVM_MapScope_AddModules(map, modules, modules_len);
	}

	// Start OPEN scripts.
	ACSVM_MapScope_ScriptStartType(map, 1, NULL, 0, NULL, NULL);
}

/*--------------------------------------------------
	void ACS_Tick(void)

		See header file for description.
--------------------------------------------------*/
void ACS_Tick(void)
{
	ACSVM_Environment *env = ACSenv;

	if (ACSVM_Environment_HasActiveThread(env) == false)
	{
		return;
	}

	ACSVM_Environment_Exec(env);
}
