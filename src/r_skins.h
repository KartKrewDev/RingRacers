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
#include "r_patch.h" // spriteinfo_t
#include "r_defs.h" // spritedef_t

/// Defaults
#define SKINNAMESIZE 16
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
	char facerank[9], facewant[9], facemmap[9]; // Arbitrarily named patch lumps

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
boolean R_SkinUsable(INT32 playernum, INT32 skinnum);
UINT32 R_GetSkinAvailabilities(void);
INT32 R_SkinAvailable(const char *name);
void R_PatchSkins(UINT16 wadnum);
void R_AddSkins(UINT16 wadnum);

UINT8 P_GetSkinSprite2(skin_t *skin, UINT8 spr2, player_t *player);

// SRB2Kart Followers

//
// We'll define these here because they're really just a mobj that'll follow some rules behind a player
//
typedef struct follower_s
{
	char skinname[SKINNAMESIZE+1];	// Skin Name. This is what to refer to when asking the commands anything.
	char name[SKINNAMESIZE+1];		// Name. This is used for the menus. We'll just follow the same rules as skins for this.

	UINT16 defaultcolor;	// default color for menus.

	fixed_t scale;			// Scale relative to the player's.
	fixed_t bubblescale;	// Bubble scale relative to the player scale. If not set, no bubble will spawn (default)

	// some position shenanigans:
	INT32 atangle;			// angle the object will be at around the player. The object itself will always face the same direction as the player.
	INT32 dist;				// distance relative to the player. (In a circle)
	INT32 height;			// height of the follower, this is mostly important for Z flipping.
	INT32 zoffs;			// Z offset relative to the player's height. Cannot be negative.

	// movement options

	UINT32 horzlag;			// Lag for X/Y displacement. Default is 2. Must be > 0 because we divide by this number.
	UINT32 vertlag;			// not Vert from Neptunia lagging, this is for Z displacement lag Default is 6. Must be > 0 because we divide by this number.
	INT32 bobamp;			// Bob amplitude. Default is 4.
	INT32 bobspeed;			// Arbitrary modifier for bobbing speed, default is TICRATE*2 (70).

	// from there on out, everything is STATES to allow customization
	// these are only set once when the action is performed and are then free to animate however they want.

	INT32 idlestate;		// state when the player is at a standstill
	INT32 followstate;		// state when the player is moving
	INT32 hurtstate;		// state when the player is being hurt
	INT32 winstate;			// state when the player has won
	INT32 losestate;		// state when the player has lost
	INT32 hitconfirmstate;	// state for hit confirm
	UINT32 hitconfirmtime;	// time to keep the above playing for
} follower_t;

extern INT32 numfollowers;
extern follower_t followers[MAXSKINS]; // again, use the same rules as skins, no reason not to.

INT32 R_FollowerAvailable(const char *name);
boolean SetPlayerFollower(INT32 playernum,const char *skinname);
void SetFollower(INT32 playernum,INT32 skinnum);

#endif //__R_SKINS__
