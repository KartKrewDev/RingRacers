// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Vivian "toastergrl" Grannell.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_skins.h
/// \brief Skins stuff

#ifndef __R_SKINS__
#define __R_SKINS__

#include "info.h"
#include "sounds.h"
#include "d_player.h" // skinflags
#include "r_patch.h"
#include "r_picformats.h" // spriteinfo_t
#include "r_defs.h" // spritedef_t

#ifdef __cplusplus
extern "C" {
#endif

/// Defaults
#define SKINRIVALS 3
// should be all lowercase!! S_SKIN processing does a strlwr
#define DEFAULTSKIN "eggman"
#define DEFAULTSKIN2 "tails" // secondary player
#define DEFAULTSKIN3 "sonic" // third player
#define DEFAULTSKIN4 "knuckles" // fourth player

/// The skin_t struct
struct skin_t
{
	char name[SKINNAMESIZE+1]; // name of skin
	UINT16 skinnum;
	UINT32 namehash; // quickncasehash(->name, SKINNAMESIZE)
	UINT16 wadnum;
	skinflags_t flags;

	char realname[SKINNAMESIZE+1]; // Display name for level completion.

	// SRB2kart
	UINT8 kartspeed;
	UINT8 kartweight;
	//

	INT32 followitem;

	// Definable color translation table
	UINT8 starttranscolor;
	UINT16 prefcolor;
	UINT16 supercolor;
	UINT16 prefoppositecolor; // if 0 use tables instead

	fixed_t highresscale; // scale of highres, default is 0.5

	skinrecord_t records;

	char rivals[SKINRIVALS][SKINNAMESIZE+1]; // Your top 3 rivals for GP mode. Uses names so that you can reference skins that aren't added

	// specific sounds per skin
	sfxenum_t soundsid[NUMSKINSOUNDS]; // sound # in S_sfx table

	// contains super versions too
	spritedef_t sprites[NUMPLAYERSPRITES*2];
	spriteinfo_t sprinfo[NUMPLAYERSPRITES*2];
};

enum facepatches {
	FACE_RANK = 0,
	FACE_WANTED,
	FACE_MINIMAP,
	NUMFACES
};

typedef enum {
	ENGINECLASS_A,
	ENGINECLASS_B,
	ENGINECLASS_C,

	ENGINECLASS_D,
	ENGINECLASS_E,
	ENGINECLASS_F,

	ENGINECLASS_G,
	ENGINECLASS_H,
	ENGINECLASS_I,

	ENGINECLASS_J,
	ENGINECLASS_R = 17,
} engineclass_t;

engineclass_t R_GetEngineClass(SINT8 speed, SINT8 weight, skinflags_t flags);

/// Externs
extern INT32 numskins;
extern skin_t **skins;

extern CV_PossibleValue_t Forceskin_cons_t[];

/// Function prototypes

// Loading
void R_InitSkins(void);
void R_AddSkins(UINT16 wadnum, boolean mainfile);
void R_PatchSkins(UINT16 wadnum, boolean mainfile);

// Access
INT32 R_SkinAvailable(const char *name);
INT32 R_SkinAvailableEx(const char *name, boolean demoskins);
boolean R_SkinUsable(INT32 playernum, INT32 skinnum, boolean demoskins);
UINT8 *R_GetSkinAvailabilities(boolean demolock, INT32 botforcecharacter);
boolean R_CanShowSkinInDemo(INT32 skinnum);

// Setting
void SetPlayerSkin(INT32 playernum,const char *skinname);
void SetPlayerSkinByNum(INT32 playernum,INT32 skinnum); // Tails 03-16-2002

// Set backup
INT32 GetSkinNumClosestToStats(UINT8 kartspeed, UINT8 kartweight, UINT32 flags, boolean unlock);
UINT16 R_BotDefaultSkin(void);

// Heavy Magician
void SetFakePlayerSkin(player_t* player, INT32 skinnum);
void SetRandomFakePlayerSkin(player_t* player, boolean fast, boolean instant);
void ClearFakePlayerSkin(player_t* player);

// Visual flair
UINT32 R_GetLocalRandomSkin(void);

// Sprite2
UINT8 P_GetSkinSprite2(skin_t *skin, UINT8 spr2, player_t *player);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__R_SKINS__
