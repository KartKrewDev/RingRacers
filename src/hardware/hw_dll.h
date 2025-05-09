// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_dll.h
/// \brief Win32 DLL and Shared Objects API definitions

#ifndef __HWR_DLL_H__
#define __HWR_DLL_H__

#ifdef __cplusplus
extern "C" {
#endif

// Function declaration for exports from the DLL :
// EXPORT <return-type> HWRAPI(<function-name>) (<arguments>);
// If _CREATE_DLL_ is defined the above declaration translates to :
// __declspec(dllexport) <return-type> WINAPI <function-name> (<arguments>);
// If _CREATE_DLL_ is NOT DEFINED the above declaration translates to :
// __declspec(dllexport) <return->type> (WINAPI *<function-name>) (<arguments>);

#ifdef _CREATE_DLL_
 #ifdef __cplusplus
  #define EXPORT  extern "C"
 #else
  #define EXPORT
 #endif
 #ifdef _WIN32
  #define HWRAPI(fn)  WINAPI fn
 #else
  #define HWRAPI(fn)  fn
 #endif
#else // _CREATE_DLL_
 #define EXPORT      typedef
 #ifdef _WIN32
  #define HWRAPI(fn)  (WINAPI *fn)
 #else
  #define HWRAPI(fn)  (*fn)
 #endif
#endif

// ==========================================================================
//                                                                      MATHS
// ==========================================================================

// Constants
#define DEGREE (0.017453292519943295769236907684883l) // 2*PI/360

void GL_DBG_Printf(const char *format, ...) /*FUNCPRINTF*/;

#ifdef __CYGWIN__
void _init() __attribute__((constructor));
void _fini() __attribute__((destructor));
#else
void _init();
void _fini();
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
