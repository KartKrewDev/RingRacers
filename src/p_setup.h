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
/// \file  p_setup.h
/// \brief Setup a game, startup stuff

#ifndef __P_SETUP__
#define __P_SETUP__

#include "doomdata.h"
#include "doomstat.h"
#include "r_defs.h"
#include "k_terrain.h"

#ifdef __cplusplus
extern "C" {
#endif

// map md5, sent to players via PT_SERVERINFO
extern unsigned char mapmd5[16];

// Player spawn spots for deathmatch.
#define MAX_DM_STARTS 64
extern mapthing_t *deathmatchstarts[MAX_DM_STARTS];
extern INT32 numdmstarts, numcoopstarts, numteamstarts[TEAM__MAX], numfaultstarts;

extern boolean levelloading;
extern boolean g_reloadinggamestate;
extern UINT8 levelfadecol;

extern tic_t oldbest;

extern lumpnum_t lastloadedmaplumpnum; // for comparative savegame
extern virtres_t *curmapvirt;

/* for levelflat type */
enum
{
	LEVELFLAT_NONE,/* HOM time my friend */
	LEVELFLAT_FLAT,
	LEVELFLAT_PATCH,
	LEVELFLAT_PNG,
	LEVELFLAT_TEXTURE,
};

//
// MAP used flats lookup table
//
struct levelflat_t
{
	char name[9]; // resource name from wad

	UINT8  type;
	union
	{
		struct
		{
			lumpnum_t     lumpnum; // lump number of the flat
			// for flat animation
			lumpnum_t baselumpnum;
		}
		flat;
		struct
		{
			INT32             num;
			INT32         lastnum; // texture number of the flat
			// for flat animation
			INT32         basenum;
		}
		texture;
	}
	u;

	UINT16 width, height;

	terrain_t *terrain;

	// for flat animation
	INT32 animseq; // start pos. in the anim sequence
	INT32 numpics;
	INT32 speed;

	// for textures
	UINT8 *picture;
#ifdef HWRENDER
	void *mipmap;
	void *mippic;
#endif
};

extern size_t numlevelflats;
extern levelflat_t *levelflats;
INT32 P_AddLevelFlat(const char *flatname, levelflat_t *levelflat);
INT32 P_AddLevelFlatRuntime(const char *flatname);
INT32 P_CheckLevelFlat(const char *flatname);

extern size_t nummapthings;
extern mapthing_t *mapthings;

void P_SetupLevelSky(const char *skytexname, boolean global);
void P_RespawnThings(void);
void P_FreeLevelState(void);
void P_ResetLevelMusic(void);
boolean P_UseContinuousLevelMusic(void);
void P_LoadLevelMusic(void);
boolean P_LoadLevel(boolean fromnetsave, boolean reloadinggamestate);
void P_PostLoadLevel(void);
#ifdef HWRENDER
void HWR_LoadLevel(void);
#endif
boolean P_AddWadFile(const char *wadfilename);

#define MAPRET_ADDED (1)
#define MAPRET_CURRENTREPLACED (1<<1)
UINT8 P_InitMapData(void);
extern lumpnum_t wadnamelump;
extern INT16 wadnamemap;
#define WADNAMECHECK(name) (!strncmp(name, "WADNAME", 7))

// WARNING: The following functions should be grouped as follows:
// any amount of PartialAdds followed by MultiSetups until returned true,
// as soon as possible.
UINT16 P_PartialAddWadFile(const char *wadfilename);
// Run a single stage of multisetup, or all of them if fullsetup set.
//   fullsetup true: run everything
//   otherwise multiple stages
// returns true if setup finished on this call, false otherwise (always true on fullsetup)
// throws I_Error if called without any partial adds started as a safeguard
boolean P_MultiSetupWadFiles(boolean fullsetup);
// Get the current setup stage.
//   if negative, no PartialAdds done since last MultiSetup
//   if 0, partial adds done but MultiSetup not called yet
//   if positive, setup's partway done
SINT8 P_PartialAddGetStage(void);
extern UINT16 partadd_earliestfile;

void P_ReduceVFXTextureReload(void);

boolean P_RunSOC(const char *socfilename);
void P_LoadSoundsRange(UINT16 wadnum, UINT16 first, UINT16 num);
void P_LoadMusicsRange(UINT16 wadnum, UINT16 first, UINT16 num);
//void P_WriteThings(void);
void P_UpdateSegLightOffset(seg_t *li);
boolean P_ApplyLightOffset(UINT8 baselightnum, const sector_t *sector);
boolean P_ApplyLightOffsetFine(UINT8 baselightlevel, const sector_t *sector);
boolean P_SectorUsesDirectionalLighting(const sector_t *sector);
size_t P_PrecacheLevelFlats(void);
void P_AllocMapHeader(INT16 i);

void P_SetDefaultHeaderFollowers(UINT16 i);
void P_DeleteHeaderFollowers(UINT16 i);

// Needed for NiGHTS
void P_ReloadRings(void);

void Command_dumprrautomedaltimes(void);
void Command_Platinums(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
