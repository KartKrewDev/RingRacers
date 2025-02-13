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
/// \file  dehacked.h
/// \brief Dehacked files.

#ifndef __DEHACKED_H__
#define __DEHACKED_H__

#include "m_fixed.h" // for get_number

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	UNDO_NONE    = 0x00,
	UNDO_NEWLINE = 0x01,
	UNDO_SPACE   = 0x02,
	UNDO_CUTLINE = 0x04,
	UNDO_HEADER  = 0x07,
	UNDO_ENDTEXT = 0x08,
	UNDO_TODO = 0,
	UNDO_DONE = 0,
} undotype_f;

void DEH_LoadDehackedLump(lumpnum_t lumpnum);
void DEH_LoadDehackedLumpPwad(UINT16 wad, UINT16 lump, boolean mainfile);

// SRB2Kart
extern int freeslotusage[2][2];
void DEH_UpdateMaxFreeslots(void);

void DEH_Check(void);

fixed_t get_number(const char *word);
FUNCPRINTF void deh_warning(const char *first, ...);
void deh_strlcpy(char *dst, const char *src, size_t size, const char *warntext);

extern boolean deh_loaded;

extern boolean gamedataadded;
extern boolean titlechanged;
extern boolean introchanged;

#define MAXRECURSION 30
extern const char *superactions[MAXRECURSION];
extern UINT8 superstack;

// If the dehacked patch does not match this version, we throw a warning
#define PATCHVERSION 2

#define MAXLINELEN 1024

// the code was first write for a file
// converted to use memory with this functions
struct MYFILE
{
	char *data;
	char *curpos;
	size_t size;
	UINT16 wad;
};
#define myfeof(a) (a->data + a->size <= a->curpos)
char *myfgets(char *buf, size_t bufsize, MYFILE *f);
char *myhashfgets(char *buf, size_t bufsize, MYFILE *f);


#ifdef __cplusplus
} // extern "C"
#endif

#endif
