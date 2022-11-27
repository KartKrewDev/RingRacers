// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
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

/// Defaults
#define SKINRIVALS 3
// should be all lowercase!! S_SKIN processing does a strlwr
#define DEFAULTSKIN "eggman"
#define DEFAULTSKIN2 "tails" // secondary player
#define DEFAULTSKIN3 "sonic" // third player
#define DEFAULTSKIN4 "knuckles" // fourth player

/// The skin_t struct
typedef struct
{
	char name[SKINNAMESIZE+1]; // INT16 descriptive name of the skin
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

	char rivals[SKINRIVALS][SKINNAMESIZE+1]; // Your top 3 rivals for GP mode. Uses names so that you can reference skins that aren't added

	// specific sounds per skin
	sfxenum_t soundsid[NUMSKINSOUNDS]; // sound # in S_sfx table

	// contains super versions too
	spritedef_t sprites[NUMPLAYERSPRITES*2];
	spriteinfo_t sprinfo[NUMPLAYERSPRITES*2];
} skin_t;

enum facepatches {
	FACE_RANK = 0,
	FACE_WANTED,
	FACE_MINIMAP,
	NUMFACES
};

/// Externs
extern INT32 numskins;
extern skin_t skins[MAXSKINS];

extern CV_PossibleValue_t Forceskin_cons_t[];

/// Function prototypes
void R_InitSkins(void);

void SetPlayerSkin(INT32 playernum,const char *skinname);
void SetPlayerSkinByNum(INT32 playernum,INT32 skinnum); // Tails 03-16-2002
void SetFakePlayerSkin(player_t* player, INT32 skinnum);
void SetRandomFakePlayerSkin(player_t* player, boolean fast);
void ClearFakePlayerSkin(player_t* player);
boolean R_SkinUsable(INT32 playernum, INT32 skinnum, boolean demoskins);

UINT8 *R_GetSkinAvailabilities(boolean demolock);
INT32 R_SkinAvailable(const char *name);

void R_PatchSkins(UINT16 wadnum, boolean mainfile);
void R_AddSkins(UINT16 wadnum, boolean mainfile);

UINT8 P_GetSkinSprite2(skin_t *skin, UINT8 spr2, player_t *player);

#endif //__R_SKINS__
