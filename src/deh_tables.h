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
/// \file  deh_tables.h
/// \brief Define DeHackEd tables.

#ifndef __DEH_TABLES_H__
#define __DEH_TABLES_H__

#include "doomdef.h" // Constants
#include "d_think.h" // actionf_t
#include "info.h" // Mobj, state, sprite, etc constants
#include "lua_script.h"

#ifdef __cplusplus
extern "C" {
#endif

// Free slot names
// The crazy word-reading stuff uses these.
extern char *FREE_STATES[NUMSTATEFREESLOTS];
extern char *FREE_MOBJS[NUMMOBJFREESLOTS];
extern char *FREE_SKINCOLORS[NUMCOLORFREESLOTS];
extern UINT8 used_spr[(NUMSPRITEFREESLOTS / 8) + 1]; // Bitwise flag for sprite freeslot in use! I would use ceil() here if I could, but it only saves 1 byte of memory anyway.

#define initfreeslots() {\
	memset(FREE_STATES,0,sizeof(char *) * NUMSTATEFREESLOTS);\
	memset(FREE_MOBJS,0,sizeof(char *) * NUMMOBJFREESLOTS);\
	memset(FREE_SKINCOLORS,0,sizeof(char *) * NUMCOLORFREESLOTS);\
	memset(used_spr,0,sizeof(UINT8) * ((NUMSPRITEFREESLOTS / 8) + 1));\
}

struct flickytypes_s {
	const char *name;
	const mobjtype_t type;
};

/** Action pointer for reading actions from Dehacked lumps.
  */
struct actionpointer_t
{
	actionf_t action; ///< Function pointer corresponding to the actual action.
	const char *name; ///< Name of the action in ALL CAPS.
};

struct int_const_s {
	const char *n;
	// has to be able to hold both fixed_t and angle_t, so drastic measure!!
	lua_Integer v;
};

extern struct flickytypes_s FLICKYTYPES[];
extern actionpointer_t actionpointers[]; // Array mapping action names to action functions.
extern const char *const STATE_LIST[];
extern const char *const MOBJTYPE_LIST[];
extern const char *const MOBJFLAG_LIST[];
extern const char *const MOBJFLAG2_LIST[]; // \tMF2_(\S+).*// (.+) --> \t"\1", // \2
extern const char *const MOBJEFLAG_LIST[];
extern const char *const MAPTHINGFLAG_LIST[4];
extern const char *const PLAYERFLAG_LIST[];
extern const char *const GAMETYPERULE_LIST[];
extern const char *const ML_LIST[]; // Linedef flags
extern const char *const MSF_LIST[]; // Sector flags
extern const char *const SSF_LIST[]; // Sector special flags
extern const char *const SD_LIST[]; // Sector damagetype
extern const char *const TO_LIST[]; // Sector triggerer
extern const char *COLOR_ENUMS[];
extern const char *const POWERS_LIST[];
extern const char *const KARTHUD_LIST[];
extern const char *const HUDITEMS_LIST[];

extern struct int_const_s const INT_CONST[];

// Moved to this file because it can't work compile-time otherwise
void DEH_TableCheck(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
