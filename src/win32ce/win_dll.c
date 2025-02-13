// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief load and initialise the 3D driver DLL

#include "../doomdef.h"

#ifdef HW3SOUND
#include "../hardware/hw3dsdrv.h"      // get the 3D sound driver DLL export prototypes
#endif

#include "win_dll.h"
#include "win_main.h"       // I_GetLastErrorMsgBox()

#if defined(HW3SOUND)
typedef struct loadfunc_s {
	LPCSTR fnName;
	LPVOID fnPointer;
} loadfunc_t;

// --------------------------------------------------------------------------
// Load a DLL, returns the HMODULE handle or NULL
// --------------------------------------------------------------------------
static inline HMODULE LoadDLL (LPCSTR dllName, loadfunc_t *funcTable)
{
	LPVOID      funcPtr;
	loadfunc_t *loadfunc;
	HMODULE     hModule;

	if ((hModule = LoadLibraryA(dllName)) != NULL)
	{
		// get function pointers for all functions we use
		for (loadfunc = funcTable; loadfunc->fnName != NULL; loadfunc++)
		{
			funcPtr = GetProcAddress(hModule, loadfunc->fnName);
			if (!funcPtr) {
				//I_GetLastErrorMsgBox ();
				MessageBoxA(NULL, va("The '%s' haven't the good specification (function %s missing)\n\n"
				           "You must use dll from the same zip of this exe\n", dllName, loadfunc->fnName),
				           "Error", MB_OK|MB_ICONINFORMATION);
				return FALSE;
			}
			// store function address
			*((LPVOID*)loadfunc->fnPointer) = funcPtr;
		}
	}
	else
	{
		MessageBoxA(NULL, va("LoadLibrary() FAILED : couldn't load '%s'\r\n", dllName), "Warning", MB_OK|MB_ICONINFORMATION);
		//I_GetLastErrorMsgBox ();
	}

	return hModule;
}


// --------------------------------------------------------------------------
// Unload the DLL
// --------------------------------------------------------------------------
static inline VOID UnloadDLL (HMODULE* pModule)
{
	if (FreeLibrary(*pModule))
		*pModule = NULL;
	else
		I_GetLastErrorMsgBox ();
}
#endif

#ifdef HW3SOUND
static HMODULE hwsModule = NULL;

static loadfunc_t hwsFuncTable[] = {
	{"_Startup@8",              &hw3ds_driver.pfnStartup},
	{"_Shutdown@0",             &hw3ds_driver.pfnShutdown},
	{"_AddSfx@4",               &hw3ds_driver.pfnAddSfx},
	{"_AddSource@8",            &hw3ds_driver.pfnAddSource},
	{"_StartSource@4",          &hw3ds_driver.pfnStartSource},
	{"_StopSource@4",           &hw3ds_driver.pfnStopSource},
	{"_GetHW3DSVersion@0",      &hw3ds_driver.pfnGetHW3DSVersion},
	{"_BeginFrameUpdate@0",     &hw3ds_driver.pfnBeginFrameUpdate},
	{"_EndFrameUpdate@0",       &hw3ds_driver.pfnEndFrameUpdate},
	{"_IsPlaying@4",            &hw3ds_driver.pfnIsPlaying},
	{"_UpdateListener@8",       &hw3ds_driver.pfnUpdateListener},
	{"_UpdateSourceParms@12",   &hw3ds_driver.pfnUpdateSourceParms},
	{"_SetCone@8",              &hw3ds_driver.pfnSetCone},
	{"_SetGlobalSfxVolume@4",   &hw3ds_driver.pfnSetGlobalSfxVolume},
	{"_Update3DSource@8",       &hw3ds_driver.pfnUpdate3DSource},
	{"_ReloadSource@8",         &hw3ds_driver.pfnReloadSource},
	{"_KillSource@4",           &hw3ds_driver.pfnKillSource},
	{"_KillSfx@4",              &hw3ds_driver.pfnKillSfx},
	{"_GetHW3DSTitle@8",        &hw3ds_driver.pfnGetHW3DSTitle},
	{NULL, NULL}
};

BOOL Init3DSDriver(LPCSTR dllName)
{
	hwsModule = LoadDLL(dllName, hwsFuncTable);
	return (hwsModule != NULL);
}

VOID Shutdown3DSDriver (VOID)
{
	UnloadDLL(&hwsModule);
}
#endif
