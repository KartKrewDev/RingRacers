// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
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

#include "CAPI/BinaryIO.h"
#include "CAPI/Environment.h"
#include "CAPI/Module.h"
#include "CAPI/PrintBuf.h"
#include "CAPI/Scope.h"
#include "CAPI/String.h"
#include "CAPI/Thread.h"

static ACSVM_Environment *ACSenv = NULL;

ACSVM_Environment *ACS_GetEnvironment(void)
{
	return ACSenv;
}

ACSVM_GlobalScope *ACS_GetGlobal(void)
{
	return ACSVM_Environment_GetGlobalScope(ACSenv, 0);
}

ACSVM_HubScope *ACS_GetHub(void)
{
	ACSVM_GlobalScope *global = ACS_GetGlobal();

	if (global == NULL)
	{
		return NULL;
	}

	return ACSVM_GlobalScope_GetHubScope(global, 0);
}

ACSVM_MapScope *ACS_GetMap(void)
{
	ACSVM_HubScope *hub = ACS_GetHub();

	if (hub == NULL)
	{
		return NULL;
	}

	return ACSVM_HubScope_GetMapScope(hub, 0);
}

static void ACS_EnvBadAlloc(ACSVM_Environment *env, char const *what)
{
	(void)env;
	I_Error("Error allocating memory for ACS (%s)\n", what);
}

static void ACS_EnvReadError(ACSVM_Environment *env, char const *what)
{
	(void)env;
	I_Error("Error reading ACS module (%s)\n", what);
}

static void ACS_EnvConstruct(ACSVM_Environment *env)
{
	ACSVM_GlobalScope *global = ACSVM_Environment_GetGlobalScope(env, 0);

	// Activate global scope immediately,
	// since we don't want it off.
	ACSVM_GlobalScope_SetActive(global, true);

	// Add the data & function pointers
	// See also:
	// https://doomwiki.org/wiki/ACS0_instruction_set
	// 
	//  0 to 56: Implemented by ACSVM

	// 69 to 79: Implemented by ACSVM

	// 81 to 82: Implemented by ACSVM

	// 84 to 85: Implemented by ACSVM
	ACSVM_Environment_AddCodeDataACS0(env,  86, "", ACSVM_Code_CallFunc, 0, ACSVM_Environment_AddCallFunc(env, ACS_CF_EndPrint));
	// 87 to 89: Implemented by ACSVM
	ACSVM_Environment_AddCodeDataACS0(env,  93, "", ACSVM_Code_CallFunc, 0, ACSVM_Environment_AddCallFunc(env, ACS_CF_Timer));
	// 136 to 137: Implemented by ACSVM

	// 157: Implemented by ACSVM

	// 167 to 173: Implemented by ACSVM

	// 175 to 179: Implemented by ACSVM

	// 181 to 189: Implemented by ACSVM

	// 203 to 217: Implemented by ACSVM

	// 225 to 243: Implemented by ACSVM

	// 253: Implemented by ACSVM

	// 256 to 257: Implemented by ACSVM

	// 263: Implemented by ACSVM
	ACSVM_Environment_AddCodeDataACS0(env, 270, "", ACSVM_Code_CallFunc, 0, ACSVM_Environment_AddCallFunc(env, ACS_CF_EndPrint));
	// 273 to 275: Implemented by ACSVM

	// 291 to 325: Implemented by ACSVM

	// 330: Implemented by ACSVM

	// 349 to 361: Implemented by ACSVM

	// 363 to 380: Implemented by ACSVM
}

static bool ACS_EnvLoadModule(ACSVM_Environment *env, ACSVM_Module *module)
{
	ACSVM_ModuleName name = {0};
	const char *str = NULL;

	size_t lumpLen = 0;

	ACSVM_Byte *data = NULL;
	size_t size = 0;

	bool ret = false;

	(void)env;

	ACSVM_Module_GetName(module, &name);
	str = ACSVM_String_GetStr(name.s);

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

void ACS_Init(void)
{
	// Initialize ACS on engine start-up.
	ACSVM_EnvironmentFuncs funcs = {0};

	funcs.bad_alloc = ACS_EnvBadAlloc;
	funcs.readError = ACS_EnvReadError;
	funcs.ctor = ACS_EnvConstruct;
	funcs.loadModule = ACS_EnvLoadModule;

	ACSenv = ACSVM_AllocEnvironment(&funcs, NULL);

	I_AddExitFunc(ACS_Shutdown);
}

void ACS_Shutdown(void)
{
	// Delete ACS environment.
	ACSVM_FreeEnvironment(ACSenv);
	ACSenv = NULL;
}

void ACS_LoadLevelScripts(size_t mapID)
{
	ACSVM_Environment *env = ACSenv;
	ACSVM_StringTable *strTab = ACSVM_Environment_GetStringTable(env);

	ACSVM_HubScope *hub = NULL;
	ACSVM_MapScope *map = NULL;

	ACSVM_Module **modules = NULL;
	size_t modules_len = 0;
	size_t modules_size = 4;

	// No hub support. Simply always reset it.
	hub = ACS_GetHub();
	ACSVM_HubScope_SetActive(hub, true);

	// Start up map scope.
	map = ACSVM_HubScope_GetMapScope(hub, 0);
	ACSVM_MapScope_SetActive(map, true);

	// Allocate module list.
	modules = Z_Calloc(modules_size * sizeof(ACSVM_Module *), PU_STATIC, NULL);

	// Insert BEHAVIOR lump.
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

void ACS_Tick(void)
{
	ACSVM_Environment *env = ACSenv;

	if (ACSVM_Environment_HasActiveThread(env) == false)
	{
		return;
	}

	ACSVM_Environment_Exec(env);
}
