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

#ifndef R_SKINS_H
#define R_SKINS_H

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
	uint16_t skinnum;
	uint32_t namehash; // quickncasehash(->name, SKINNAMESIZE)
	uint16_t wadnum;
	skinflags_t flags;

	char realname[SKINNAMESIZE+1]; // Display name for level completion.

	// SRB2kart
	uint8_t kartspeed;
	uint8_t kartweight;
	//

	int32_t followitem;

	// Definable color translation table
	uint8_t starttranscolor;
	uint16_t prefcolor;
	uint16_t supercolor;
	uint16_t prefoppositecolor; // if 0 use tables instead

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

engineclass_t R_GetEngineClass(int8_t speed, int8_t weight, skinflags_t flags);

/// Externs
extern int32_t numskins;
extern skin_t **skins;

extern CV_PossibleValue_t Forceskin_cons_t[];

/// Function prototypes

// Loading
void R_InitSkins(void);
void R_AddSkins(uint16_t wadnum, dboolean mainfile);
void R_PatchSkins(uint16_t wadnum, dboolean mainfile);

// Access
int32_t R_SkinAvailable(const char *name);
int32_t R_SkinAvailableEx(const char *name, dboolean demoskins);
dboolean R_SkinUsable(int32_t playernum, int32_t skinnum, dboolean demoskins);
uint8_t *R_GetSkinAvailabilities(dboolean demolock, int32_t botforcecharacter);
dboolean R_CanShowSkinInDemo(int32_t skinnum);

// Setting
void SetPlayerSkin(int32_t playernum,const char *skinname);
void SetPlayerSkinByNum(int32_t playernum,int32_t skinnum); // Tails 03-16-2002

// Set backup
int32_t GetSkinNumClosestToStats(uint8_t kartspeed, uint8_t kartweight, uint32_t flags, dboolean unlock);
uint16_t R_BotDefaultSkin(void);

// Heavy Magician
void SetFakePlayerSkin(player_t* player, int32_t skinnum);
void SetRandomFakePlayerSkin(player_t* player, dboolean fast, dboolean instant);
void ClearFakePlayerSkin(player_t* player);

// Visual flair
uint32_t R_GetLocalRandomSkin(void);

// Sprite2
uint8_t P_GetSkinSprite2(skin_t *skin, uint8_t spr2, player_t *player);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //R_SKINS_H
