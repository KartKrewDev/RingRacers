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
/// \brief Tool for dynamic referencing of hardware rendering functions
///
///	Declaration and definition of the HW rendering
///	functions do have the same name. Originally, the
///	implementation was stored in a separate library.
///	For SDL, we need some function to return the addresses,
///	otherwise we have a conflict with the compiler.

#include "hwsym_sdl.h"
#include "../doomdef.h"

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL

#include "SDL.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#if defined (_XBOX) || defined (_arch_dreamcast) || defined(GP2X)
#define NOLOADSO
#endif

#if SDL_VERSION_ATLEAST(1,2,6) && !defined (NOLOADSO)
#include "SDL_loadso.h" // 1.2.6+
#elif !defined (NOLOADSO)
#define NOLOADSO
#endif

#define  _CREATE_DLL_  // necessary for Unix AND Windows

#ifdef HW3SOUND
#include "../hardware/hw3dsdrv.h"
#endif

#define GETFUNC(func) \
	else if (0 == strcmp(#func, funcName)) \
		funcPointer = &func \
//
//
/**	\brief	The *hwSym function

	Stupid function to return function addresses

	\param	funcName	the name of the function
	\param	handle	an object to look in(NULL for self)

	\return	void
*/
//
void *hwSym(const char *funcName,void *handle)
{
	void *funcPointer = NULL;
	if (0 == strcmp("FinishUpdate", funcName))
		return funcPointer; //&FinishUpdate;
#ifdef STATIC3DS
	GETFUNC(Startup);
	GETFUNC(AddSfx);
	GETFUNC(AddSource);
	GETFUNC(StartSource);
	GETFUNC(StopSource);
	GETFUNC(GetHW3DSVersion);
	GETFUNC(BeginFrameUpdate);
	GETFUNC(EndFrameUpdate);
	GETFUNC(IsPlaying);
	GETFUNC(UpdateListener);
	GETFUNC(UpdateSourceParms);
	GETFUNC(SetGlobalSfxVolume);
	GETFUNC(SetCone);
	GETFUNC(Update3DSource);
	GETFUNC(ReloadSource);
	GETFUNC(KillSource);
	GETFUNC(Shutdown);
	GETFUNC(GetHW3DSTitle);
#endif
#ifdef NOLOADSO
	else
		funcPointer = handle;
#else
	else if (handle)
		funcPointer = SDL_LoadFunction(handle,funcName);
#endif
	if (!funcPointer)
		I_OutputMsg("hwSym for %s: %s\n", funcName, SDL_GetError());
	return funcPointer;
}

/**	\brief	The *hwOpen function

	\param	hwfile	Open a handle to the SO

	\return	Handle to SO


*/

void *hwOpen(const char *hwfile)
{
#ifdef NOLOADSO
	(void)hwfile;
	return NULL;
#else
	void *tempso = NULL;
	tempso = SDL_LoadObject(hwfile);
	if (!tempso) I_OutputMsg("hwOpen of %s: %s\n", hwfile, SDL_GetError());
	return tempso;
#endif
}

/**	\brief	The hwClose function

	\param	handle	Close the handle of the SO

	\return	void


*/

void hwClose(void *handle)
{
#ifdef NOLOADSO
	(void)handle;
#else
	SDL_UnloadObject(handle);
#endif
}
#endif
