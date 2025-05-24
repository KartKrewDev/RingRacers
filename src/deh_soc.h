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
/// \file  deh_soc.h
/// \brief Load SOC file and change tables and text

#ifndef __DEH_SOC_H__
#define __DEH_SOC_H__

#include "doomdef.h"
#include "g_game.h"
#include "sounds.h"
#include "info.h"
#include "d_think.h"
#include "m_argv.h"
#include "z_zone.h"
#include "w_wad.h"
#include "k_menu.h"
#include "m_misc.h"
#include "f_finale.h"
#include "st_stuff.h"
#include "i_system.h"
#include "p_setup.h"
#include "r_data.h"
#include "r_textures.h"
#include "r_draw.h"
#include "r_picformats.h"
#include "r_things.h" // R_Char2Frame
#include "r_sky.h"
#include "fastcmp.h"
#include "lua_script.h" // Reluctantly included for LUA_EvalMath
#include "d_clisrv.h"

#ifdef HWRENDER
#include "hardware/hw_light.h"
#endif

#include "info.h"
#include "dehacked.h"
#include "doomdef.h" // HWRENDER

#ifdef __cplusplus
extern "C" {
#endif

// Crazy word-reading stuff
/// \todo Put these in a seperate file or something.
mobjtype_t get_mobjtype(const char *word);
statenum_t get_state(const char *word);
spritenum_t get_sprite(const char *word);
playersprite_t get_sprite2(const char *word);
sfxenum_t get_sfx(const char *word);
//INT16 get_gametype(const char *word);
//powertype_t get_power(const char *word);
skincolornum_t get_skincolor(const char *word);

void readwipes(MYFILE *f);
void readmaincfg(MYFILE *f, boolean mainfile);
void readconditionset(MYFILE *f, UINT16 setnum);
void readunlockable(MYFILE *f, INT32 num);
void reademblemdata(MYFILE *f, INT32 num);
void readsound(MYFILE *f, INT32 num);
void readframe(MYFILE *f, INT32 num);
void readhuditem(MYFILE *f, INT32 num);
void readmenu(MYFILE *f, INT32 num);
void readtextprompt(MYFILE *f, INT32 num);
void readcutscene(MYFILE *f, INT32 num);
void readlevelheader(MYFILE *f, char * name);
void readgametype(MYFILE *f, char *gtname);
void readsprite2(MYFILE *f, INT32 num);
#ifdef HWRENDER
void readlight(MYFILE *f, INT32 num);
#endif
void readskincolor(MYFILE *f, INT32 num);
void readthing(MYFILE *f, INT32 num);
void readfreeslots(MYFILE *f);
void clear_emblems(void);
void clear_unlockables(void);
void clear_levels(void);
void clear_conditionsets(void);

void readcupheader(MYFILE *f, cupheader_t *cup);
void readfollower(MYFILE *f);
void readfollowercategory(MYFILE *f);
preciptype_t get_precip(const char *word);
void readweather(MYFILE *f, INT32 num);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
