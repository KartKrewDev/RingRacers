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

#ifndef P_SETUP_H
#define P_SETUP_H

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
extern int32_t numdmstarts, numcoopstarts, numteamstarts[TEAM__MAX], numfaultstarts;

extern dboolean levelloading;
extern dboolean g_reloadinggamestate;
extern uint8_t levelfadecol;

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

	uint8_t  type;
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
			int32_t             num;
			int32_t         lastnum; // texture number of the flat
			// for flat animation
			int32_t         basenum;
		}
		texture;
	}
	u;

	uint16_t width, height;

	terrain_t *terrain;

	// for flat animation
	int32_t animseq; // start pos. in the anim sequence
	int32_t numpics;
	int32_t speed;

	// for textures
	uint8_t *picture;
#ifdef HWRENDER
	void *mipmap;
	void *mippic;
#endif
};

extern size_t numlevelflats;
extern levelflat_t *levelflats;
int32_t P_AddLevelFlat(const char *flatname, levelflat_t *levelflat);
int32_t P_AddLevelFlatRuntime(const char *flatname);
int32_t P_CheckLevelFlat(const char *flatname);

extern size_t nummapthings;
extern mapthing_t *mapthings;

void P_SetupLevelSky(const char *skytexname, dboolean global);
void P_RespawnThings(void);
void P_FreeLevelState(void);
void P_ResetLevelMusic(void);
dboolean P_UseContinuousLevelMusic(void);
void P_LoadLevelMusic(void);
dboolean P_LoadLevel(dboolean fromnetsave, dboolean reloadinggamestate);
void P_PostLoadLevel(void);
#ifdef HWRENDER
void HWR_LoadLevel(void);
#endif
dboolean P_AddWadFile(const char *wadfilename);

#define MAPRET_ADDED (1)
#define MAPRET_CURRENTREPLACED (1<<1)
uint8_t P_InitMapData(void);
extern lumpnum_t wadnamelump;
extern int16_t wadnamemap;
#define WADNAMECHECK(name) (!strncmp(name, "WADNAME", 7))

// WARNING: The following functions should be grouped as follows:
// any amount of PartialAdds followed by MultiSetups until returned true,
// as soon as possible.
uint16_t P_PartialAddWadFile(const char *wadfilename);
// Run a single stage of multisetup, or all of them if fullsetup set.
//   fullsetup true: run everything
//   otherwise multiple stages
// returns true if setup finished on this call, false otherwise (always true on fullsetup)
// throws I_Error if called without any partial adds started as a safeguard
dboolean P_MultiSetupWadFiles(dboolean fullsetup);
// Get the current setup stage.
//   if negative, no PartialAdds done since last MultiSetup
//   if 0, partial adds done but MultiSetup not called yet
//   if positive, setup's partway done
int8_t P_PartialAddGetStage(void);
extern uint16_t partadd_earliestfile;

void P_ReduceVFXTextureReload(void);

dboolean P_RunSOC(const char *socfilename);
void P_LoadSoundsRange(uint16_t wadnum, uint16_t first, uint16_t num);
void P_LoadMusicsRange(uint16_t wadnum, uint16_t first, uint16_t num);
//void P_WriteThings(void);
void P_UpdateSegLightOffset(seg_t *li);
dboolean P_ApplyLightOffset(uint8_t baselightnum, const sector_t *sector);
dboolean P_ApplyLightOffsetFine(uint8_t baselightlevel, const sector_t *sector);
dboolean P_SectorUsesDirectionalLighting(const sector_t *sector);
size_t P_PrecacheLevelFlats(void);
void P_AllocMapHeader(int16_t i);

void P_SetDefaultHeaderFollowers(uint16_t i);
void P_DeleteHeaderFollowers(uint16_t i);

// Needed for NiGHTS
void P_ReloadRings(void);

void Command_dumprrautomedaltimes(void);
void Command_Platinums(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
