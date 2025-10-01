// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_misc.h
/// \brief Commonly used routines
///        Default config file, PCX screenshots, file i/o

#ifndef __M_MISC__
#define __M_MISC__

#include "doomtype.h"
#include "tables.h"

#include "d_event.h" // Screenshot responder
#include "command.h"
#include "w_wad.h"

#ifdef __cplusplus

#include <cstddef>

#include <tcb/span.hpp>

void M_DoScreenShot(uint32_t width, uint32_t height, tcb::span<const std::byte> data);
void M_SaveFrame(uint32_t width, uint32_t height, tcb::span<const std::byte> data);

void M_SaveMapThumbnail(uint32_t width, uint32_t height, tcb::span<const std::byte> data);

extern "C" {
#endif

typedef enum {
	MM_OFF = 0,
	MM_APNG,
	MM_GIF,
	MM_SCREENSHOT,
	MM_AVRECORDER,
} moviemode_t;
extern moviemode_t moviemode;

typedef enum {
	TMT_NO = 0,
	TMT_PICTURE,
	TMT_RICHPRES,
} g_takemapthumbnail_t;
extern g_takemapthumbnail_t g_takemapthumbnail;

extern consvar_t cv_screenshot_colorprofile;
extern consvar_t cv_lossless_recorder;
extern consvar_t cv_zlib_memory, cv_zlib_level, cv_zlib_strategy, cv_zlib_window_bits;
extern consvar_t cv_zlib_memorya, cv_zlib_levela, cv_zlib_strategya, cv_zlib_window_bitsa;
extern consvar_t cv_apng_delay, cv_apng_downscale;

void M_StartMovie(moviemode_t mode);
void M_LegacySaveFrame(void);
void M_StopMovie(void);

// the file where game vars and settings are saved
#define CONFIGFILENAME "ringconfig.cfg"

// The file where we'll save the last IPs we joined
#define IPLOGFILE "ringsavedips.txt"
#define IPLOGFILESEP ";"
#define NUMLOGIP 3
#define MAX_LOGIP 255

// Array where we'll store addresses to display for last servers joined
// {address, servame}
extern char joinedIPlist[NUMLOGIP][2][MAX_LOGIP];

// Keep the address we're joining in mind until we've finished joining.
// Since we don't wanna add an IP address we aren't even sure worked out.
extern char joinedIP[MAX_LOGIP];

void M_InitJoinedIPArray(void);
void M_AddToJoinedIPs(char *address, char *servname);
void M_SaveJoinedIPs(void);
void M_LoadJoinedIPs(void);

boolean FIL_WriteFile(char const *name, const void *source, size_t length);
size_t FIL_ReadFileTag(char const *name, UINT8 **buffer, INT32 tag);
#define FIL_ReadFile(n, b) FIL_ReadFileTag(n, b, PU_STATIC)

boolean FIL_ConvertTextFileToBinary(const char *textfilename, const char *binfilename);

boolean FIL_RenameFile(const char *old_name, const char *new_name);

boolean FIL_FileExists(const char *name);
boolean FIL_WriteFileOK(char const *name);
boolean FIL_ReadFileOK(char const *name);
boolean FIL_FileOK(char const *name);

void FIL_DefaultExtension (char *path, const char *extension);
void FIL_ForceExtension(char *path, const char *extension);
boolean FIL_CheckExtension(const char *in);

#ifdef HAVE_PNG
boolean M_SavePNG(const char *filename, const void *data, int width, int height, const UINT8 *palette);
#endif

extern boolean takescreenshot;
void M_ScreenShot(void);
#ifdef HWRENDER
void M_DoLegacyGLScreenShot(void);
#endif
void M_ScreenshotTicker(void);

void M_MinimapGenerate(void);

void Command_SaveConfig_f(void);
void Command_LoadConfig_f(void);
void Command_ChangeConfig_f(void);

void M_FirstLoadConfig(void);
// save game config: cvars, aliases..
void M_SaveConfig(const char *filename);

INT32 axtoi(const char *hexStg);

const char *GetRevisionString(void);

// Vector/matrix math
typedef fixed_t TVector[4];
typedef fixed_t TMatrix[4][4];

TVector *VectorMatrixMultiply(TVector v, TMatrix m);
TMatrix *RotateXMatrix(angle_t rad);
#if 0
TMatrix *RotateYMatrix(angle_t rad);
#endif
TMatrix *RotateZMatrix(angle_t rad);

// s1 = s2+s3+s1 (1024 lenghtmax)
void strcatbf(char *s1, const char *s2, const char *s3);

const char *M_FileError(FILE *handle);

int     M_PathParts      (const char *path);
boolean M_IsPathAbsolute (const char *path);
void    M_MkdirEach      (const char *path, int start, int mode);
void    M_MkdirEachUntil (const char *path, int start, int end, int mode);

/* Return offset to the first word in a string. */
/* E.g. cursor += M_JumpWord(line + cursor); */
int M_JumpWord (const char *s);

/* Return index of the last word behind offset bytes in a string. */
/* E.g. cursor = M_JumpWordReverse(line, cursor); */
int M_JumpWordReverse (const char *line, int offset);

/*
Return dot and then the fractional part of a float, without
trailing zeros, or "" if the fractional part is zero.
*/
const char * M_Ftrim (double);

// counting bits, for weapon ammo code, usually
FUNCMATH UINT8 M_CountBits(UINT32 num, UINT8 size);

extern char configfile[MAX_WADPATH];

typedef INT32 floatdenormalstate_t;

/** Enable floating point denormal-to-zero section, if necessary */
floatdenormalstate_t M_EnterFloatDenormalToZero(void);
/** Exit floating point denormal-to-zero section, if necessary, restoring previous state */
void M_ExitFloatDenormalToZero(floatdenormalstate_t previous);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
