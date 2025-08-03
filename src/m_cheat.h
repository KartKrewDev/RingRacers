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
/// \file  m_cheat.h
/// \brief Cheat code checking

#ifndef __M_CHEAT__
#define __M_CHEAT__

#include "d_event.h"
#include "d_player.h"
#include "p_mobj.h"
#include "command.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CHEAT_NOCLIP,
	CHEAT_GOD,
	CHEAT_SAVECHECKPOINT,
	CHEAT_RINGS,
	CHEAT_LIVES,
	CHEAT_SCALE,
	CHEAT_FLIP,
	CHEAT_HURT,
	CHEAT_RELATIVE_TELEPORT,
	CHEAT_TELEPORT,
	CHEAT_DEVMODE,
	CHEAT_GIVEITEM,
	CHEAT_SCORE,
	CHEAT_ANGLE,
	CHEAT_RESPAWNAT,
	CHEAT_GIVEPOWERUP,
	CHEAT_SPHERES,
	CHEAT_FREEZE,
	CHEAT_AMPS,

	NUMBER_OF_CHEATS
} cheat_t;

//
// ObjectPlace
//
void Command_ObjectPlace_f(void);
//void Command_Writethings_f(void);

extern consvar_t cv_opflags, cv_ophoopflags, cv_mapthingnum, cv_speed;
//extern consvar_t cv_snapto, cv_grid;

extern boolean objectplacing;
extern mobjtype_t op_currentthing;
extern UINT16 op_currentdoomednum;
extern UINT32 op_displayflags;

boolean OP_FreezeObjectplace(void);
void OP_ResetObjectplace(void);
void OP_ObjectplaceMovement(player_t *player);

//
// Other cheats
//
void Command_CheatNoClip_f(void);
void Command_CheatGod_f(void);
void Command_CheatFreeze_f(void);
void Command_Savecheckpoint_f(void);
void Command_Setrings_f(void);
void Command_Setspheres_f(void);
void Command_Setamps_f(void);
void Command_Setlives_f(void);
void Command_Setroundscore_f(void);
void Command_Devmode_f(void);
void Command_Scale_f(void);
void Command_Gravflip_f(void);
void Command_Hurtme_f(void);

void Command_Stumble_f(void);
void Command_Whumble_f(void);
void Command_Tumble_f(void);
void Command_Explode_f(void);
void Command_Spinout_f(void);
void Command_Wipeout_f(void);
void Command_Sting_f(void);
void Command_Kill_f(void);

void Command_Teleport_f(void);
void Command_RTeleport_f(void);
void Command_Skynum_f(void);
void Command_Weather_f(void);
void Command_Grayscale_f(void);
void Command_Goto_f(void);
void Command_Angle_f(void);
void Command_RespawnAt_f(void);
void Command_GotoSkybox_f(void);
#ifdef DEVELOP
void Command_Crypt_f(void);
#endif
#ifdef _DEBUG
void Command_CauseCfail_f(void);
#endif
#ifdef LUA_ALLOW_BYTECODE
void Command_Dumplua_f(void);
#endif

extern consvar_t cv_devmode_screen;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
