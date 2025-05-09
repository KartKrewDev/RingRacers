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
/// \brief exception handler

#ifndef __WIN32_WIN_DBG_H__
#define __WIN32_WIN_DBG_H__

//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUGTRAP

BOOL InitBugTrap(void);
void ShutdownBugTrap(void);
BOOL IsBugTrapLoaded(void);

#endif

// called in the exception filter of the __try block, writes all useful debugging information
// to a file, using only win32 functions in case the C runtime is in a bad state.
LONG WINAPI RecordExceptionInfo(PEXCEPTION_POINTERS data/*, LPCSTR Message, LPSTR lpCmdLine*/);

extern LPTOP_LEVEL_EXCEPTION_FILTER prevExceptionFilter;

#ifdef __MINGW32__

#include <excpt.h>

#ifndef TRYLEVEL_NONE

#ifndef __MINGW64__
#define NO_SEH_MINGW //Alam:?
#endif
#ifndef GetExceptionInformation
void *__cdecl _exception_info(void);
#define GetExceptionInformation (struct _EXCEPTION_POINTERS *)_exception_info
#endif //GetExceptionInformation

//Alam_GBC: use __try1(seh)
#ifndef __try
#define __try
#endif //__try

//#undef NO_SEH_MINGW //Alam: win_dbg's code not working with MINGW
//Alam_GBC: use __except1
#ifndef __except
#define __except(x) if (0)
#endif //__except

#endif // !__TRYLEVEL_NONE

#endif // __MINGW32__

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __WIN32_WIN_DBG_H__
