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
/// \file  p_setup.cpp
/// \brief Do all the WAD I/O, get map description, set up initial state and misc. LUTs

#include <algorithm>

#include <fmt/format.h>

#include "cxxutil.hpp"

#include "doomdef.h"
#include "d_main.h"
#include "byteptr.h"
#include "g_game.h"

#include "p_local.h"
#include "p_setup.h"
#include "p_spec.h"
#include "p_saveg.h"

#include "i_time.h"
#include "i_video.h" // for I_FinishUpdate()..
#include "r_sky.h"
#include "i_system.h"

#include "r_data.h"
#include "r_things.h" // for R_AddSpriteDefs
#include "r_textures.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "r_sky.h"
#include "r_draw.h"
#include "r_fps.h" // R_ResetViewInterpolation in level load

#include "s_sound.h"
#include "i_sound.h" // I_FreeSfx
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_splats.h"

#include "hu_stuff.h"
#include "console.h"

#include "m_misc.h"
#include "m_fixed.h"
#include "m_random.h"

#include "dehacked.h" // for map headers
#include "r_main.h"
#include "m_cond.h" // for emblems

#include "m_argv.h"

#include "p_polyobj.h"

#include "v_video.h"

#include "filesrch.h" // refreshdirmenu

#include "lua_hud.h" // level title

#include "f_finale.h" // wipes

#include "md5.h" // map MD5

// for MapLoad hook
#include "lua_script.h"
#include "lua_hook.h"

#ifdef _WIN32
#include <malloc.h>
#include <math.h>
#endif
#ifdef HWRENDER
#include "hardware/hw_main.h"
#include "hardware/hw_light.h"
#include "hardware/hw_model.h"
#endif

#include "p_slopes.h"

#include "fastcmp.h" // textmap parsing
#include "taglist.h"

// SRB2Kart
#include "core/string.h"
#include "core/vector.hpp"
#include "k_kart.h"
#include "k_race.h"
#include "k_battle.h" // K_BattleInit
#include "k_pwrlv.h"
#include "k_waypoint.h"
#include "k_bot.h"
#include "k_grandprix.h"
#include "k_boss.h"
#include "k_terrain.h" // TRF_TRIPWIRE
#include "k_brightmap.h"
#include "k_director.h" // K_InitDirector
#include "k_specialstage.h"
#include "acs/interface.h"
#include "doomstat.h" // MAXMUSNAMES
#include "k_podium.h"
#include "k_rank.h"
#include "k_mapuser.h"
#include "music.h"
#include "k_dialogue.h"
#include "k_hud.h" // K_ClearPersistentMessages
#include "k_endcam.h"
#include "k_credits.h"
#include "k_objects.h"
#include "p_deepcopy.h"
#include "k_color.h" // K_ColorUsable

// Replay names have time
#if !defined (UNDER_CE)
#include <time.h>
#endif

#include <tracy/tracy/TracyC.h>

extern "C" consvar_t cv_continuousmusic;
boolean g_reloadinggamestate = false;

//
// Map MD5, calculated on level load.
// Sent to clients in PT_SERVERINFO.
//
unsigned char mapmd5[16];

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//

boolean udmf;
INT32 udmf_version;
size_t numvertexes, numsegs, numsectors, numsubsectors, numnodes, numlines, numsides, nummapthings;
size_t num_orig_vertexes;
vertex_t *vertexes;
seg_t *segs;
sector_t *sectors;
subsector_t *subsectors;
node_t *nodes;
line_t *lines;
side_t *sides;
mapthing_t *mapthings;
sector_t *spawnsectors;
line_t *spawnlines;
side_t *spawnsides;
INT32 numcheatchecks;
UINT16 bossdisabled;
boolean stoppedclock;
boolean levelloading;
UINT8 levelfadecol;

tic_t oldbest;
// I cannot fucking believe this is needed, but gamedata is updated at exactly
// the wrong time to check your record in the tally screen.

virtres_t *curmapvirt;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
INT32 bmapwidth, bmapheight; // size in mapblocks

INT32 *blockmap; // INT32 for large maps
// offsets in blockmap are from here
INT32 *blockmaplump; // Big blockmap

// origin of block map
fixed_t bmaporgx, bmaporgy;
// for thing chains
mobj_t **blocklinks;
precipmobj_t **precipblocklinks;

// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed LineOf Sight calculation.
// Without special effect, this could be used as a PVS lookup as well.
//
UINT8 *rejectmatrix;

// Maintain single and multi player starting spots.
INT32 numdmstarts, numcoopstarts, numteamstarts[TEAM__MAX];
INT32 numfaultstarts;

mapthing_t *deathmatchstarts[MAX_DM_STARTS];
mapthing_t *playerstarts[MAXPLAYERS];
mapthing_t *teamstarts[TEAM__MAX][MAXPLAYERS];
mapthing_t *faultstart;

// Global state for PartialAddWadFile/MultiSetupWadFiles
// Might be replacable with parameters, but non-trivial when the functions are called on separate tics
static SINT8 partadd_stage = -1;
static boolean partadd_important = false;
UINT16 partadd_earliestfile = UINT16_MAX;


// Maintain *ZOOM TUBE* waypoints
// Renamed because SRB2Kart owns real waypoints.
mobj_t *tubewaypoints[NUMTUBEWAYPOINTSEQUENCES][TUBEWAYPOINTSEQUENCESIZE];
UINT16 numtubewaypoints[NUMTUBEWAYPOINTSEQUENCES];

void P_AddTubeWaypoint(UINT8 sequence, UINT8 id, mobj_t *waypoint)
{
	P_SetTarget(&tubewaypoints[sequence][id], waypoint);
	if (id >= numtubewaypoints[sequence])
		numtubewaypoints[sequence] = id + 1;
}

static void P_ResetTubeWaypoints(void)
{
	UINT16 sequence, id;
	for (sequence = 0; sequence < NUMTUBEWAYPOINTSEQUENCES; sequence++)
	{
		for (id = 0; id < numtubewaypoints[sequence]; id++)
			tubewaypoints[sequence][id] = NULL;

		numtubewaypoints[sequence] = 0;
	}
}

mobj_t *P_GetFirstTubeWaypoint(UINT8 sequence)
{
	return tubewaypoints[sequence][0];
}

mobj_t *P_GetLastTubeWaypoint(UINT8 sequence)
{
	return tubewaypoints[sequence][numtubewaypoints[sequence] - 1];
}

mobj_t *P_GetPreviousTubeWaypoint(mobj_t *current, boolean wrap)
{
	UINT8 sequence = current->threshold;
	UINT8 id = current->health;

	if (id == 0)
	{
		if (!wrap)
			return NULL;

		id = numtubewaypoints[sequence] - 1;
	}
	else
		id--;

	return tubewaypoints[sequence][id];
}

mobj_t *P_GetNextTubeWaypoint(mobj_t *current, boolean wrap)
{
	UINT8 sequence = current->threshold;
	UINT8 id = current->health;

	if (id == numtubewaypoints[sequence] - 1)
	{
		if (!wrap)
			return NULL;

		id = 0;
	}
	else
		id++;

	return tubewaypoints[sequence][id];
}

mobj_t *P_GetClosestTubeWaypoint(UINT8 sequence, mobj_t *mo)
{
	UINT8 wp;
	mobj_t *mo2, *result = NULL;
	fixed_t bestdist = 0;
	fixed_t curdist;

	for (wp = 0; wp < numtubewaypoints[sequence]; wp++)
	{
		mo2 = tubewaypoints[sequence][wp];

		if (!mo2)
			continue;

		curdist = P_AproxDistance(P_AproxDistance(mo->x - mo2->x, mo->y - mo2->y), mo->z - mo2->z);

		if (result && curdist > bestdist)
			continue;

		result = mo2;
		bestdist = curdist;
	}

	return result;
}

// Return true if all waypoints are in the same location
boolean P_IsDegeneratedTubeWaypointSequence(UINT8 sequence)
{
	mobj_t *first, *waypoint;
	UINT8 wp;

	if (numtubewaypoints[sequence] <= 1)
		return true;

	first = tubewaypoints[sequence][0];

	for (wp = 1; wp < numtubewaypoints[sequence]; wp++)
	{
		waypoint = tubewaypoints[sequence][wp];

		if (!waypoint)
			continue;

		if (waypoint->x != first->x)
			return false;

		if (waypoint->y != first->y)
			return false;

		if (waypoint->z != first->z)
			return false;
	}

	return true;
}


/** Logs an error about a map being corrupt, then terminate.
  * This allows reporting highly technical errors for usefulness, without
  * confusing a novice map designer who simply needs to run ZenNode.
  *
  * If logging is disabled in this compile, or the log file is not opened, the
  * full technical details are printed in the I_Error() message.
  *
  * \param msg The message to log. This message can safely result from a call
  *            to va(), since that function is not used here.
  * \todo Fix the I_Error() message. On some implementations the logfile may
  *       not be called log.txt.
  * \sa CON_LogMessage, I_Error
  */
FUNCNORETURN static ATTRNORETURN void CorruptMapError(const char *msg)
{
	// don't use va() because the calling function probably uses it
	char mapname[MAXMAPLUMPNAME];

	if (gamemap > 0 && gamemap <= nummapheaders && mapheaderinfo[gamemap-1])
	{
		sprintf(mapname, "%s", mapheaderinfo[gamemap-1]->lumpname);
	}
	else
	{
		sprintf(mapname, "ID %d", gamemap-1);
	}

	CON_LogMessage("Map ");
	CON_LogMessage(mapname);
	CON_LogMessage(" is corrupt: ");
	CON_LogMessage(msg);
	CON_LogMessage("\n");
	I_Error("Invalid or corrupt map.\nLook in log file or text console for technical details.");
}

/** Sets a header's followers to the default list
  *
  * \param i The header to set followers for
  */
void P_SetDefaultHeaderFollowers(UINT16 i)
{
	static INT16 defaultfollowers[MAXHEADERFOLLOWERS];
	static UINT8 validdefaultfollowers = 0;

	if (validdefaultfollowers == 0)
	{
		const char *defaultfollowernames[] =
		{
			"Flicky",
			"Chao",
			"Motobuddy",
			NULL
		};

		for (validdefaultfollowers = 0; defaultfollowernames[validdefaultfollowers]; validdefaultfollowers++)
		{
			defaultfollowers[validdefaultfollowers] = K_FollowerAvailable(defaultfollowernames[validdefaultfollowers]);
		}

		I_Assert(validdefaultfollowers != 0);
	}

	mapheaderinfo[i]->followers = static_cast<INT16*>(Z_Realloc(mapheaderinfo[i]->followers, sizeof(INT16) * validdefaultfollowers, PU_STATIC, NULL));

	for (mapheaderinfo[i]->numFollowers = 0; mapheaderinfo[i]->numFollowers < validdefaultfollowers; mapheaderinfo[i]->numFollowers++)
	{
		mapheaderinfo[i]->followers[mapheaderinfo[i]->numFollowers] = defaultfollowers[mapheaderinfo[i]->numFollowers];
	}
}

/** Clears a header's followers
  *
  * \param i The header to clear followers for
  */
void P_DeleteHeaderFollowers(UINT16 i)
{
	if (mapheaderinfo[i]->followers)
		Z_Free(mapheaderinfo[i]->followers);
	mapheaderinfo[i]->followers = NULL;
	mapheaderinfo[i]->numFollowers = 0;
}

#define NUMLAPS_DEFAULT 3

static void P_ClearMapHeaderLighting(mapheader_lighting_t *lighting)
{
	lighting->light_contrast = 16;
	lighting->sprite_backlight = 0;
	lighting->use_light_angle = false;
	lighting->light_angle = 0;
}

/** Clears the data from a single map header.
  *
  * \param i Map number to clear header for.
  * \sa P_ClearMapHeaderInfo
  */
static void P_ClearSingleMapHeaderInfo(INT16 num)
{
	int i;

	mapheaderinfo[num]->lvlttl[0] = '\0';
	mapheaderinfo[num]->menuttl[0] = '\0';
	mapheaderinfo[num]->zonttl[0] = '\0';
	mapheaderinfo[num]->actnum = 0;
	mapheaderinfo[num]->typeoflevel = 0;
	mapheaderinfo[num]->gravity = DEFAULT_GRAVITY;
	mapheaderinfo[num]->keywords[0] = '\0';
	mapheaderinfo[num]->relevantskin[0] = '\0';

	mapheaderinfo[num]->musname[0][0] = 0;
	mapheaderinfo[num]->musname_size = 0;
	mapheaderinfo[num]->encoremusname[0][0] = 0;
	mapheaderinfo[num]->encoremusname_size = 0;

	for (i = 0; i < MAXMUSNAMES-1; i++)
	{
		mapheaderinfo[num]->cache_muslock[i] = MAXUNLOCKABLES;
	}

	mapheaderinfo[num]->positionmus[0] = '\0';
	mapheaderinfo[num]->associatedmus[0][0] = 0;
	mapheaderinfo[num]->associatedmus_size = 0;

	mapheaderinfo[num]->mustrack = 0;
	mapheaderinfo[num]->muspos = 0;

	mapheaderinfo[num]->weather = PRECIP_NONE;
	snprintf(mapheaderinfo[num]->skytexture, 5, "SKY1");
	mapheaderinfo[num]->skytexture[4] = 0;
	mapheaderinfo[num]->skybox_scalex = 16;
	mapheaderinfo[num]->skybox_scaley = 16;
	mapheaderinfo[num]->skybox_scalez = 16;
	mapheaderinfo[num]->darkness = FRACUNIT;
	mapheaderinfo[num]->runsoc[0] = '#';
	mapheaderinfo[num]->scriptname[0] = '#';
	mapheaderinfo[num]->precutscenenum = 0;
	mapheaderinfo[num]->cutscenenum = 0;
	mapheaderinfo[num]->palette = UINT16_MAX;
	mapheaderinfo[num]->encorepal = UINT16_MAX;
	mapheaderinfo[num]->numlaps = NUMLAPS_DEFAULT;
	mapheaderinfo[num]->lapspersection = 1;
	mapheaderinfo[num]->levelselect = 0;
	mapheaderinfo[num]->levelflags = 0;
	mapheaderinfo[num]->menuflags = 0;
	mapheaderinfo[num]->playerLimit = MAXPLAYERS;
	mapheaderinfo[num]->mobj_scale = FRACUNIT;
	mapheaderinfo[num]->default_waypoint_radius = 0;
	P_ClearMapHeaderLighting(&mapheaderinfo[num]->lighting);
	P_ClearMapHeaderLighting(&mapheaderinfo[num]->lighting_encore);
	mapheaderinfo[num]->use_encore_lighting = false;
#if 1 // equivalent to "Followers = DEFAULT"
	P_SetDefaultHeaderFollowers(num);
#else
	P_DeleteHeaderFollowers(num);
#endif

	memset(&mapheaderinfo[num]->records, 0, sizeof(recorddata_t));
	mapheaderinfo[num]->records.spraycan = MCAN_INVALID;

	mapheaderinfo[num]->justPlayed = 0;
	mapheaderinfo[num]->anger = 0;

	mapheaderinfo[num]->destroyforchallenge_size = 0;

	mapheaderinfo[num]->cache_maplock = MAXUNLOCKABLES;

	mapheaderinfo[num]->customopts = NULL;
	mapheaderinfo[num]->numCustomOptions = 0;

	if (mapheaderinfo[num]->ghostBrief != NULL)
	{
		for (i = 0; i < mapheaderinfo[num]->ghostCount; i++)
		{
			Z_Free(mapheaderinfo[num]->ghostBrief[i]);
		}
		Z_Free(mapheaderinfo[num]->ghostBrief);
	}
	mapheaderinfo[num]->ghostBrief = NULL;
	mapheaderinfo[num]->ghostCount = 0;
	mapheaderinfo[num]->ghostBriefSize = 0;
	mapheaderinfo[num]->automedaltime[0] = 1;
	mapheaderinfo[num]->automedaltime[1] = 2;
	mapheaderinfo[num]->automedaltime[2] = 3;
	mapheaderinfo[num]->automedaltime[3] = 4;
	mapheaderinfo[num]->cameraHeight = INT32_MIN;
}

/** Allocates a new map-header structure.
  *
  * \param i Index of header to allocate.
  */
void P_AllocMapHeader(INT16 i)
{
	if (i > nummapheaders)
		I_Error("P_AllocMapHeader: Called on %d, should be %d", i, nummapheaders);

	if (i >= NEXTMAP_SPECIAL)
	{
		I_Error("P_AllocMapHeader: Too many maps!");
	}

	if (i >= mapallocsize)
	{
		if (!mapallocsize)
		{
			mapallocsize = 16;
		}
		else
		{
			mapallocsize *= 2;
		}

		mapheaderinfo = static_cast<mapheader_t**>(Z_ReallocAlign(
			(void*) mapheaderinfo,
			sizeof(mapheader_t*) * mapallocsize,
			PU_STATIC,
			NULL,
			sizeof(mapheader_t*) * 8
		));

		if (!mapheaderinfo)
			I_Error("P_AllocMapHeader: Not enough memory to realloc mapheaderinfo (size %d)", mapallocsize);
	}

	if (!mapheaderinfo[i])
	{
		mapheaderinfo[i] = static_cast<mapheader_t*>(Z_Malloc(sizeof(mapheader_t), PU_STATIC, NULL));
		if (!mapheaderinfo[i])
			I_Error("P_AllocMapHeader: Not enough memory to allocate new mapheader (ID %d)", i);

		mapheaderinfo[i]->lumpnum = LUMPERROR;
		mapheaderinfo[i]->lumpname = NULL;
		mapheaderinfo[i]->thumbnailPic = NULL;
		mapheaderinfo[i]->minimapPic = NULL;
		mapheaderinfo[i]->ghostCount = 0;
		mapheaderinfo[i]->ghostBriefSize = 0;
		mapheaderinfo[i]->ghostBrief = NULL;
		mapheaderinfo[i]->cup = NULL;
		mapheaderinfo[i]->followers = NULL;
		nummapheaders++;
	}
	P_ClearSingleMapHeaderInfo(i);
}

//
// levelflats
//
#define MAXLEVELFLATS 256

size_t numlevelflats;
levelflat_t *levelflats;
levelflat_t *foundflats;

//SoM: Other files want this info.
size_t P_PrecacheLevelFlats(void)
{
	lumpnum_t lump;
	size_t i;

	//SoM: 4/18/2000: New flat code to make use of levelflats.
	flatmemory = 0;
	for (i = 0; i < numlevelflats; i++)
	{
		if (levelflats[i].type == LEVELFLAT_FLAT)
		{
			lump = levelflats[i].u.flat.lumpnum;
			if (devparm)
				flatmemory += W_LumpLength(lump);
			R_GetFlat(lump);
		}
	}
	return flatmemory;
}

/*
levelflat refers to an array of level flats,
or NULL if we want to allocate it now.
*/
static INT32
Ploadflat (levelflat_t *levelflat, const char *flatname, boolean resize)
{
	int       texturenum;
	size_t i;

	// Scan through the already found flats, return if it matches.
	for (i = 0; i < numlevelflats; i++)
	{
		if (strnicmp(levelflat[i].name, flatname, 8) == 0)
			return i;
	}

	if (resize)
	{
		// allocate new flat memory
		levelflats = static_cast<levelflat_t*>(Z_Realloc(levelflats, (numlevelflats + 1) * sizeof(*levelflats), PU_LEVEL, NULL));
		levelflat  = levelflats + numlevelflats;
	}
	else
	{
		if (numlevelflats >= MAXLEVELFLATS)
			I_Error("Too many flats in level\n");

		levelflat += numlevelflats;
	}

	// Store the name.
	strlcpy(levelflat->name, flatname, sizeof (levelflat->name));
	strupr(levelflat->name);

	if (( texturenum = R_CheckTextureNumForName(levelflat->name) ) == -1)
	{
		// check for missing texture
		if (( texturenum = R_CheckTextureNumForName(MISSING_TEXTURE) ) != -1)
			goto texturefound;

		// nevermind
		levelflat->type = LEVELFLAT_NONE;
	}
	else
	{
texturefound:
		levelflat->type = LEVELFLAT_TEXTURE;
		levelflat->u.texture.    num = texturenum;
		levelflat->u.texture.lastnum = texturenum;
		/* start out unanimated */
		levelflat->u.texture.basenum = -1;
	}

	levelflat->terrain =
		K_GetTerrainForTextureName(levelflat->name);

	CONS_Debug(DBG_SETUP, "flat #%03d: %s\n", atoi(sizeu1(numlevelflats)), levelflat->name);

	return ( numlevelflats++ );
}

// Auxiliary function. Find a flat in the active wad files,
// allocate an id for it, and set the levelflat (to speedup search)
INT32 P_AddLevelFlat(const char *flatname, levelflat_t *levelflat)
{
	return Ploadflat(levelflat, flatname, false);
}

// help function for Lua and $$$.sav reading
// same as P_AddLevelFlat, except this is not setup so we must realloc levelflats to fit in the new flat
// no longer a static func in lua_maplib.c because p_saveg.c also needs it
//
INT32 P_AddLevelFlatRuntime(const char *flatname)
{
	return Ploadflat(levelflats, flatname, true);
}

// help function for $$$.sav checking
// this simply returns the flat # for the name given
//
INT32 P_CheckLevelFlat(const char *flatname)
{
	size_t i;
	levelflat_t *levelflat = levelflats;

	//
	//  scan through the already found flats
	//
	for (i = 0; i < numlevelflats; i++, levelflat++)
		if (strnicmp(levelflat->name,flatname,8)==0)
			break;

	if (i == numlevelflats)
		return 0; // ??? flat was not found, this should not happen!

	// level flat id
	return (INT32)i;
}

//
// P_ReloadRings
// Used by NiGHTS, clears all ring/sphere/hoop/etc items and respawns them
//
void P_ReloadRings(void)
{
	mobj_t *mo;
	thinker_t *th;
	size_t i, numHoops = 0;
	// Okay, if you have more than 4000 hoops in your map,
	// you're insane.
	mapthing_t *hoopsToRespawn[4096];
	mapthing_t *mt = mapthings;

	// scan the thinkers to find rings/spheres/hoops to unset
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)th;

		if (mo->type == MT_HOOPCENTER)
		{
			// Hoops give me a headache
			if (mo->threshold == 4242) // Dead hoop
			{
				hoopsToRespawn[numHoops++] = mo->spawnpoint;
				P_RemoveMobj(mo);
			}
			continue;
		}
		if (mo->type != MT_RING)
			continue;

		// Don't auto-disintegrate things being pulled to us
		if (mo->flags2 & MF2_NIGHTSPULL)
			continue;

		P_RemoveMobj(mo);
	}

	// Reiterate through mapthings
	for (i = 0; i < nummapthings; i++, mt++)
	{
		// Notice an omission? We handle hoops differently.
		if (mt->type == mobjinfo[MT_RING].doomednum)
		{
			mt->mobj = NULL;
			P_SpawnMapThing(mt);
		}
		else if (mt->type >= 600 && mt->type <= 611) // Item patterns
		{
			mt->mobj = NULL;
			P_SpawnItemPattern(mt);
		}
	}
	for (i = 0; i < numHoops; i++)
	{
		P_SpawnHoop(hoopsToRespawn[i]);
	}
}

static int cmp_loopends(const void *a, const void *b)
{
	const mapthing_t
		*mt1 = *(const mapthing_t*const*)a,
		*mt2 = *(const mapthing_t*const*)b;

	// weighted sorting; tag takes precedence over type
	const int maincomp = intsign(mt1->tid - mt2->tid) * 2 +
		intsign(mt1->thing_args[0] - mt2->thing_args[0]);

	// JugadorXEI (04/20/25): If a qsort comparison ends up with an equal result,
	// it results in UNSPECIFIED BEHAVIOR, so assuming the previous two comparisons
	// are equal, let's make it consistent with Linux behaviour (ascending order).
	return maincomp != 0 ? maincomp : intsign((mt1 - mapthings) - (mt2 - mapthings));
}

static void P_SpawnMapThings(boolean spawnemblems)
{
	size_t i;
	mapthing_t *mt;

	mapthing_t **loopends;
	size_t num_loopends = 0;

	// Spawn axis points first so they are at the front of the list for fast searching.
	for (i = 0, mt = mapthings; i < nummapthings; i++, mt++)
	{
		switch (mt->type)
		{
			case 1700: // MT_AXIS
			case 1701: // MT_AXISTRANSFER
			case 1702: // MT_AXISTRANSFERLINE
			case 2021: // MT_LOOPCENTERPOINT
				mt->mobj = NULL;
				P_SpawnMapThing(mt);
				break;
			case 2020: // MT_LOOPENDPOINT
				num_loopends++;
				break;
			default:
				break;
		}
	}

	Z_Malloc(num_loopends * sizeof *loopends, PU_STATIC,
			&loopends);
	num_loopends = 0;

	for (i = 0, mt = mapthings; i < nummapthings; i++, mt++)
	{
		switch (mt->type)
		{
			case 1700: // MT_AXIS
			case 1701: // MT_AXISTRANSFER
			case 1702: // MT_AXISTRANSFERLINE
			case 2021: // MT_LOOPCENTERPOINT
				continue; // These were already spawned
		}

		if (mt->type == mobjinfo[MT_ITEMCAPSULE].doomednum)
		{
			continue; // These will spawn later (in k_battle.c K_BattleInit)
		}

		if (mt->type == mobjinfo[MT_BATTLECAPSULE].doomednum && gametype != GT_TUTORIAL)
		{
			continue; // These will spawn later (in k_battle.c K_BattleInit), unless we're in a tutorial
		}

		if (!spawnemblems && mt->type == mobjinfo[MT_EMBLEM].doomednum)
			continue;

		mt->mobj = NULL;

		if (mt->type == mobjinfo[MT_LOOPENDPOINT].doomednum)
		{
			loopends[num_loopends] = mt;
			num_loopends++;
			continue;
		}

		if (mt->type >= 600 && mt->type <= 611) // item patterns
			P_SpawnItemPattern(mt);
		else if (mt->type == 1713) // hoops
			P_SpawnHoop(mt);
		else // Everything else
			P_SpawnMapThing(mt);
	}

	qsort(loopends, num_loopends, sizeof *loopends,
			cmp_loopends);

	for (i = 1; i < num_loopends; ++i)
	{
		mapthing_t
			*mt1 = loopends[i - 1],
			*mt2 = loopends[i];

		if (mt1->tid == mt2->tid &&
				mt1->thing_args[0] == mt2->thing_args[0])
		{
			P_SpawnItemLine(mt1, mt2);
			i++;
		}
	}

	Z_Free(loopends);

	if (spawnemblems
		&& gametype != GT_TUTORIAL
		&& !modeattacking
		&& !(tutorialchallenge == TUTORIALSKIP_INPROGRESS && gamedata->gotspraycans == 0))
	{
		const UINT8 recommendedcans =
#ifdef DEVELOP
			!(mapheaderinfo[gamemap-1]->typeoflevel & TOL_RACE) ? 0 :
#endif
			1;

		if (nummapspraycans > recommendedcans)
			CONS_Alert(CONS_ERROR, "SPRAY CANS: Map has too many Spray Cans (%d)!", nummapspraycans);
#ifdef DEVELOP
		else if (nummapspraycans != recommendedcans)
			CONS_Alert(CONS_ERROR, "SPRAY CANS: Krew-made Race maps need a Spray Can placed!");
#endif
	}
}

// Experimental groovy write function!
/*
void P_WriteThings(void)
{
	const char * filename;
	size_t i, length;
	mapthing_t *mt;
	savebuffer_t save = {0};
	INT16 temp;

	if (P_SaveBufferAlloc(&save, nummapthings * sizeof (mapthing_t)) == false)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for thing writing!\n"));
		return;
	}

	save.end = save.buffer + save.size;

	mt = mapthings;
	for (i = 0; i < nummapthings; i++, mt++)
	{
		WRITEINT16(save.p, mt->x);
		WRITEINT16(save.p, mt->y);

		WRITEINT16(save.p, mt->angle);

		temp = (INT16)(mt->type + ((INT16)mt->extrainfo << 12));
		WRITEINT16(save.p, temp);
		WRITEUINT16(save.p, mt->options);
	}

	length = save.p - save.buffer;

	filename = va("newthings-%s.lmp", G_BuildMapName(gamemap));

	FIL_WriteFile(filename, save.buffer, length);
	P_SaveBufferFree(&save);

	CONS_Printf(M_GetText("%s saved.\n"), filename);
}
*/

//
// MAP LOADING FUNCTIONS
//

static void P_LoadVertices(UINT8 *data)
{
	mapvertex_t *mv = (mapvertex_t *)data;
	vertex_t *v = vertexes;
	size_t i;

	// Copy and convert vertex coordinates, internal representation as fixed.
	for (i = 0; i < numvertexes; i++, v++, mv++)
	{
		v->x = SHORT(mv->x)<<FRACBITS;
		v->y = SHORT(mv->y)<<FRACBITS;
		v->floorzset = v->ceilingzset = false;
		v->floorz = v->ceilingz = 0;
	}
}

static void P_InitializeSector(sector_t *ss)
{
	memset(&ss->soundorg, 0, sizeof(ss->soundorg));

	ss->validcount = 0;

	ss->thinglist = NULL;

	ss->floordata = NULL;
	ss->ceilingdata = NULL;
	ss->lightingdata = NULL;
	ss->fadecolormapdata = NULL;

	ss->heightsec = -1;
	ss->camsec = -1;

	ss->floorlightsec = ss->ceilinglightsec = -1;
	ss->crumblestate = CRUMBLE_NONE;

	ss->touching_thinglist = NULL;

	ss->linecount = 0;
	ss->lines = NULL;

	ss->ffloors = NULL;
	ss->attached = NULL;
	ss->attachedsolid = NULL;
	ss->numattached = 0;
	ss->maxattached = 1;
	ss->lightlist = NULL;
	ss->numlights = 0;
	ss->moved = true;

	ss->extra_colormap = NULL;

	ss->gravityptr = NULL;

	ss->cullheight = NULL;

	ss->floorspeed = ss->ceilspeed = 0;

	ss->touching_preciplist = NULL;

	ss->f_slope = NULL;
	ss->c_slope = NULL;
	ss->hasslope = false;

	ss->spawn_lightlevel = ss->lightlevel;

	ss->spawn_extra_colormap = NULL;

	memset(&ss->botController, 0, sizeof(ss->botController));
}

static void P_LoadSectors(UINT8 *data)
{
	mapsector_t *ms = (mapsector_t *)data;
	sector_t *ss = sectors;
	size_t i;

	// For each counted sector, copy the sector raw data from our cache pointer ms, to the global table pointer ss.
	for (i = 0; i < numsectors; i++, ss++, ms++)
	{
		ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;

		ss->floorpic = P_AddLevelFlat(ms->floorpic, foundflats);
		ss->ceilingpic = P_AddLevelFlat(ms->ceilingpic, foundflats);

		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		Tag_FSet(&ss->tags, SHORT(ms->tag));

		ss->floor_xoffs = ss->floor_yoffs = 0;
		ss->ceiling_xoffs = ss->ceiling_yoffs = 0;

		ss->floorpic_angle = ss->ceilingpic_angle = 0;

		ss->floorlightlevel = ss->ceilinglightlevel = 0;
		ss->floorlightabsolute = ss->ceilinglightabsolute = false;

		ss->colormap_protected = false;

		ss->gravity = FRACUNIT;

		ss->flags = MSF_FLIPSPECIAL_FLOOR;
		ss->specialflags = static_cast<sectorspecialflags_t>(0);
		ss->damagetype = SD_NONE;
		ss->triggertag = 0;
		ss->triggerer = TO_PLAYER;

		ss->friction = ORIG_FRICTION;

		ss->action = 0;
		memset(ss->args, 0, NUM_SCRIPT_ARGS*sizeof(*ss->args));
		memset(ss->stringargs, 0x00, NUM_SCRIPT_STRINGARGS*sizeof(*ss->stringargs));
		ss->activation = static_cast<sectoractionflags_t>(0);

		P_InitializeSector(ss);
	}
}

static void P_InitializeLinedef(line_t *ld)
{
	vertex_t *v1 = ld->v1;
	vertex_t *v2 = ld->v2;
	UINT8 j;

	ld->dx = v2->x - v1->x;
	ld->dy = v2->y - v1->y;

	ld->angle = R_PointToAngle2(0, 0, ld->dx, ld->dy);

	ld->bbox[BOXLEFT] = std::min(v1->x, v2->x);
	ld->bbox[BOXRIGHT] = std::max(v1->x, v2->x);
	ld->bbox[BOXBOTTOM] = std::min(v1->y, v2->y);
	ld->bbox[BOXTOP] = std::max(v1->y, v2->y);

	if (!ld->dx)
		ld->slopetype = ST_VERTICAL;
	else if (!ld->dy)
		ld->slopetype = ST_HORIZONTAL;
	else if ((ld->dy > 0) == (ld->dx > 0))
		ld->slopetype = ST_POSITIVE;
	else
		ld->slopetype = ST_NEGATIVE;

	ld->frontsector = ld->backsector = NULL;

	ld->validcount = 0;
	ld->polyobj = NULL;

	ld->tripwire = false;

	ld->callcount = 0;

	// cph 2006/09/30 - fix sidedef errors right away.
	// cph 2002/07/20 - these errors are fatal if not fixed, so apply them
	for (j = 0; j < 2; j++)
		if (ld->sidenum[j] != 0xffff && ld->sidenum[j] >= (UINT16)numsides)
		{
			ld->sidenum[j] = 0xffff;
			CONS_Debug(DBG_SETUP, "P_InitializeLinedef: Linedef %s has out-of-range sidedef number\n", sizeu1((size_t)(ld - lines)));
		}

	// killough 11/98: fix common wad errors (missing sidedefs):
	if (ld->sidenum[0] == 0xffff)
	{
		ld->sidenum[0] = 0;  // Substitute dummy sidedef for missing right side
		// cph - print a warning about the bug
		CONS_Debug(DBG_SETUP, "P_InitializeLinedef: Linedef %s missing first sidedef\n", sizeu1((size_t)(ld - lines)));
	}

	if ((ld->sidenum[1] == 0xffff) && (ld->flags & ML_TWOSIDED))
	{
		ld->flags &= ~ML_TWOSIDED;  // Clear 2s flag for missing left side
		// cph - print a warning about the bug
		CONS_Debug(DBG_SETUP, "P_InitializeLinedef: Linedef %s has two-sided flag set, but no second sidedef\n", sizeu1((size_t)(ld - lines)));
	}

	if (ld->sidenum[0] != 0xffff)
	{
		sides[ld->sidenum[0]].special = ld->special;
		sides[ld->sidenum[0]].line = ld;
	}
	if (ld->sidenum[1] != 0xffff)
	{
		sides[ld->sidenum[1]].special = ld->special;
		sides[ld->sidenum[1]].line = ld;
	}
}

static void P_SetLinedefV1(size_t i, UINT16 vertex_num)
{
	if (vertex_num >= numvertexes)
	{
		CONS_Debug(DBG_SETUP, "P_SetLinedefV1: linedef %s has out-of-range v1 num %u\n", sizeu1(i), vertex_num);
		vertex_num = 0;
	}
	lines[i].v1 = &vertexes[vertex_num];
}

static void P_SetLinedefV2(size_t i, UINT16 vertex_num)
{
	if (vertex_num >= numvertexes)
	{
		CONS_Debug(DBG_SETUP, "P_SetLinedefV2: linedef %s has out-of-range v2 num %u\n", sizeu1(i), vertex_num);
		vertex_num = 0;
	}
	lines[i].v2 = &vertexes[vertex_num];
}

static void P_LoadLinedefs(UINT8 *data)
{
	maplinedef_t *mld = (maplinedef_t *)data;
	line_t *ld = lines;
	size_t i;

	for (i = 0; i < numlines; i++, mld++, ld++)
	{
		ld->flags = (UINT32)(SHORT(mld->flags));
		ld->special = SHORT(mld->special);
		Tag_FSet(&ld->tags, SHORT(mld->tag));
		memset(ld->args, 0, NUM_SCRIPT_ARGS*sizeof(*ld->args));
		memset(ld->stringargs, 0x00, NUM_SCRIPT_STRINGARGS*sizeof(*ld->stringargs));
		ld->alpha = FRACUNIT;
		ld->executordelay = 0;
		ld->activation = 0;
		P_SetLinedefV1(i, SHORT(mld->v1));
		P_SetLinedefV2(i, SHORT(mld->v2));

		ld->sidenum[0] = SHORT(mld->sidenum[0]);
		ld->sidenum[1] = SHORT(mld->sidenum[1]);

		P_InitializeLinedef(ld);
	}
}

static void P_SetSidedefSector(size_t i, UINT16 sector_num)
{
	// cph 2006/09/30 - catch out-of-range sector numbers; use sector 0 instead
	if (sector_num >= numsectors)
	{
		CONS_Debug(DBG_SETUP, "P_SetSidedefSector: sidedef %s has out-of-range sector num %u\n", sizeu1(i), sector_num);
		sector_num = 0;
	}
	sides[i].sector = &sectors[sector_num];
}

static void P_InitializeSidedef(side_t *sd)
{
	if (!sd->line)
	{
		CONS_Debug(DBG_SETUP, "P_LoadSidedefs: Sidedef %s is not used by any linedef\n", sizeu1((size_t)(sd - sides)));
		sd->line = &lines[0];
		sd->special = sd->line->special;
	}

	sd->colormap_data = NULL;
}

/* -- Reference implementation
static void P_WriteConstant(INT32 constant, char **target)
{
	char buffer[12];
	size_t len;

	sprintf(buffer, "%d", constant);
	len = strlen(buffer) + 1;
	*target = Z_Malloc(len, PU_LEVEL, NULL);
	M_Memcpy(*target, buffer, len);
} */

static void P_WriteDuplicateText(const char *text, char **target)
{
	if (text == NULL || text[0] == '\0')
		return;

	size_t len = strlen(text) + 1;
	*target = static_cast<char*>(Z_Malloc(len, PU_LEVEL, NULL));
	M_Memcpy(*target, text, len);
}

static void P_WriteSkincolor(INT32 constant, char **target)
{
	if (constant <= SKINCOLOR_NONE
	|| constant >= (INT32)numskincolors)
		return;

	P_WriteDuplicateText(
		va("SKINCOLOR_%s", skincolors[constant].name),
		target
	);
}

static void P_WriteSfx(INT32 constant, char **target)
{
	if (constant <= sfx_None
	|| constant >= (INT32)sfxfree)
		return;

	P_WriteDuplicateText(
		va("sfx_%s", S_sfx[constant].name),
		target
	);
}

static void P_LoadSidedefs(UINT8 *data)
{
	mapsidedef_t *msd = (mapsidedef_t*)data;
	side_t *sd = sides;
	size_t i;

	for (i = 0; i < numsides; i++, sd++, msd++)
	{
		INT16 textureoffset = SHORT(msd->textureoffset);
		boolean isfrontside;

		P_InitializeSidedef(sd);

		isfrontside = sd->line->sidenum[0] == i;

		// Repeat count for midtexture
		if (((sd->line->flags & (ML_TWOSIDED|ML_WRAPMIDTEX)) == (ML_TWOSIDED|ML_WRAPMIDTEX))
			&& !(sd->special >= 300 && sd->special < 500)) // exempt linedef exec specials
		{
			sd->repeatcnt = (INT16)(((UINT16)textureoffset) >> 12);
			sd->textureoffset = (((UINT16)textureoffset) & 2047) << FRACBITS;
		}
		else
		{
			sd->repeatcnt = 0;
			sd->textureoffset = textureoffset << FRACBITS;
		}
		sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;

		P_SetSidedefSector(i, SHORT(msd->sector));

		// Special info stored in texture fields!
		switch (sd->special)
		{
			case 606: //SoM: 4/4/2000: Just colormap transfer
			case 447: // Change colormap of tagged sectors! -- Monster Iestyn 14/06/18
			case 455: // Fade colormaps! mazmazz 9/12/2018 (:flag_us:)
				// SoM: R_CreateColormap will only create a colormap in software mode...
				// Perhaps we should just call it instead of doing the calculations here.
				sd->colormap_data = R_CreateColormapFromLinedef(msd->toptexture, msd->midtexture, msd->bottomtexture);
				sd->toptexture = sd->midtexture = sd->bottomtexture = 0;
				break;

			case 414: // Play SFX
			{
				sd->toptexture = sd->midtexture = sd->bottomtexture = 0;

				if (!isfrontside)
					break;

				if (msd->toptexture[0] != '-' || msd->toptexture[1] != '\0')
				{
					char process[8 + 1];
					M_Memcpy(process, msd->toptexture, 8);
					process[8] = '\0';

					P_WriteDuplicateText(process, &sd->line->stringargs[0]);
				}
				break;
			}

			case 9: // Mace parameters
			case 14: // Bustable block parameters
			case 15: // Fan particle spawner parameters
			{
				if (msd->toptexture[7] == '\0' && strcasecmp(msd->toptexture, "MT_NULL") == 0)
				{
					// Don't bulk the conversion with irrelevant types
					break;
				}
			}
			// FALLTHRU
			case 331: // Trigger linedef executor: Skin - Continuous
			case 332: // Trigger linedef executor: Skin - Each time
			case 333: // Trigger linedef executor: Skin - Once
			case 334: // Trigger linedef executor: Object dye - Continuous
			case 335: // Trigger linedef executor: Object dye - Each time
			case 336: // Trigger linedef executor: Object dye - Once
			case 425: // Calls P_SetMobjState on calling mobj
			case 442: // Calls P_SetMobjState on mobjs of a given type in the tagged sectors
			case 443: // Calls a named Lua function
			case 461: // Spawns an object on the map based on texture offsets
			case 463: // Colorizes an object
			case 475: // ACS_Execute
			case 476: // ACS_ExecuteAlways
			case 477: // ACS_Suspend
			case 478: // ACS_Terminate
			{
				char process[8*3+1];
				memset(process,0,8*3+1);
				sd->toptexture = sd->midtexture = sd->bottomtexture = 0;
				if (msd->toptexture[0] == '-' && msd->toptexture[1] == '\0')
					break;
				else
					M_Memcpy(process,msd->toptexture,8);
				if (msd->midtexture[0] != '-' || msd->midtexture[1] != '\0')
					M_Memcpy(process+strlen(process), msd->midtexture, 8);
				if (msd->bottomtexture[0] != '-' || msd->bottomtexture[1] != '\0')
					M_Memcpy(process+strlen(process), msd->bottomtexture, 8);

				P_WriteDuplicateText(
					process,
					&sd->line->stringargs[(isfrontside) ? 0 : 1]
				);
				break;
			}

			case 259: // Custom FOF
				if (!isfrontside)
				{
					if ((msd->toptexture[0] >= '0' && msd->toptexture[0] <= '9')
						|| (msd->toptexture[0] >= 'A' && msd->toptexture[0] <= 'F'))
						sd->toptexture = axtoi(msd->toptexture);
					else
						I_Error("Custom FOF (line id %s) needs a value in the linedef's back side upper texture field.", sizeu1(sd->line - lines));

					sd->midtexture = R_TextureNumForName(msd->midtexture);
					sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
					break;
				}
				// FALLTHRU
			default: // normal cases
				if (msd->toptexture[0] == '#')
				{
					char *col = msd->toptexture;
					sd->toptexture =
						((col[1]-'0')*100 + (col[2]-'0')*10 + col[3]-'0')+1;
					if (col[4]) // extra num for blendmode
						sd->toptexture += (col[4]-'0')*1000;
					sd->bottomtexture = sd->toptexture;
					sd->midtexture = R_TextureNumForName(msd->midtexture);
				}
				else
				{
					sd->midtexture = R_TextureNumForName(msd->midtexture);
					sd->toptexture = R_TextureNumForName(msd->toptexture);
					sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
				}
				break;
		}
	}
}

static void P_LoadThings(UINT8 *data)
{
	mapthing_t *mt;
	size_t i;

	for (i = 0, mt = mapthings; i < nummapthings; i++, mt++)
	{
		mt->x = READINT16(data);
		mt->y = READINT16(data);

		mt->angle = READINT16(data);
		mt->type = READUINT16(data);
		mt->options = READUINT16(data);
		mt->extrainfo = (UINT8)(mt->type >> 12);
		mt->tid = 0;
		mt->scale = FRACUNIT;
		mt->spritexscale = mt->spriteyscale = FRACUNIT;
		memset(mt->thing_args, 0, NUM_MAPTHING_ARGS*sizeof(*mt->thing_args));
		memset(mt->thing_stringargs, 0x00, NUM_MAPTHING_STRINGARGS*sizeof(*mt->thing_stringargs));
		mt->special = 0;
		memset(mt->script_args, 0, NUM_SCRIPT_ARGS*sizeof(*mt->script_args));
		memset(mt->script_stringargs, 0x00, NUM_SCRIPT_STRINGARGS*sizeof(*mt->script_stringargs));
		mt->pitch = mt->roll = 0;
		mt->layer = 0;

		mt->type &= 4095;

		if (mt->type == 1705 || (mt->type == 750 && mt->extrainfo))
			mt->z = mt->options; // NiGHTS Hoops use the full flags bits to set the height.
		else
			mt->z = mt->options >> ZSHIFT;

		mt->adjusted_z = INT32_MAX;

		mt->mobj = NULL;
	}
}

// Stores positions for relevant map data spread through a TEXTMAP.
UINT32 mapthingsPos[UINT16_MAX];
UINT32 linesPos[UINT16_MAX];
UINT32 sidesPos[UINT16_MAX];
UINT32 vertexesPos[UINT16_MAX];
UINT32 sectorsPos[UINT16_MAX];

// Determine total amount of map data in TEXTMAP.
static boolean TextmapCount(size_t size)
{
	TracyCZone(__zone, true);

	const char *tkn = M_TokenizerRead(0);
	UINT8 brackets = 0;

	nummapthings = 0;
	numlines = 0;
	numsides = 0;
	numvertexes = 0;
	numsectors = 0;

	// Look for namespace at the beginning.
	if (!fastcmp(tkn, "namespace"))
	{
		CONS_Alert(CONS_ERROR, "No namespace at beginning of lump!\n");
		TracyCZoneEnd(__zone);
		return false;
	}

	// Check if namespace is valid.
	tkn = M_TokenizerRead(0);
	if (!fastcmp(tkn, "ringracers"))
		CONS_Alert(CONS_WARNING, "Invalid namespace '%s', only 'ringracers' is supported. This map may have issues loading.\n", tkn);

	while ((tkn = M_TokenizerRead(0)) && M_TokenizerGetEndPos() < size)
	{
		// Avoid anything inside bracketed stuff, only look for external keywords.
		if (brackets)
		{
			if (fastcmp(tkn, "}"))
				brackets--;
		}
		else if (fastcmp(tkn, "{"))
			brackets++;
		// Check for valid fields.
		else if (fastcmp(tkn, "thing"))
			mapthingsPos[nummapthings++] = M_TokenizerGetEndPos();
		else if (fastcmp(tkn, "linedef"))
			linesPos[numlines++] = M_TokenizerGetEndPos();
		else if (fastcmp(tkn, "sidedef"))
			sidesPos[numsides++] = M_TokenizerGetEndPos();
		else if (fastcmp(tkn, "vertex"))
			vertexesPos[numvertexes++] = M_TokenizerGetEndPos();
		else if (fastcmp(tkn, "sector"))
			sectorsPos[numsectors++] = M_TokenizerGetEndPos();
		else if (fastcmp(tkn, "version"))
		{
			tkn = M_TokenizerRead(0);
			udmf_version = atoi(tkn);
			if (udmf_version > UDMF_CURRENT_VERSION)
				CONS_Alert(CONS_WARNING, "Map is intended for future UDMF version '%d', current supported version is '%d'. This map may have issues loading.\n", udmf_version, UDMF_CURRENT_VERSION);
		}
		else
			CONS_Alert(CONS_NOTICE, "Unknown field '%s'.\n", tkn);
	}

	if (brackets)
	{
		CONS_Alert(CONS_ERROR, "Unclosed brackets detected in textmap lump.\n");
		TracyCZoneEnd(__zone);
		return false;
	}

	TracyCZoneEnd(__zone);
	return true;
}

enum
{
	PROP_NUM_TYPE_NA,
	PROP_NUM_TYPE_INT,
	PROP_NUM_TYPE_FLOAT
};

static void ParseUserProperty(mapUserProperties_t *user, const char *param, const char *val)
{
	if (fastncmp(param, "user_", 5) && strlen(param) > 5)
	{
		const boolean valIsString = M_TokenizerJustReadString();
		const char *key = param + 5;
		const size_t valLen = strlen(val);
		UINT8 numberType = PROP_NUM_TYPE_INT;
		size_t i = 0;

		if (valIsString == true)
		{
			// Value is a string. Upload directly!
			K_UserPropertyPush(user, key, USER_PROP_STR, &val);
			return;
		}

		for (i = 0; i < valLen; i++)
		{
			if (val[i] == '.')
			{
				numberType = PROP_NUM_TYPE_FLOAT;
			}
			else if (val[i] < '0' || val[i] > '9')
			{
				numberType = PROP_NUM_TYPE_NA;
				break;
			}
		}

		switch (numberType)
		{
			case PROP_NUM_TYPE_INT:
			{
				// Value is an integer.
				INT32 vInt = atol(val);
				K_UserPropertyPush(user, key, USER_PROP_INT, &vInt);
				break;
			}
			case PROP_NUM_TYPE_FLOAT:
			{
				// Value is a float. Convert to fixed.
				fixed_t vFixed = FLOAT_TO_FIXED(atof(val));
				K_UserPropertyPush(user, key, USER_PROP_FIXED, &vFixed);
				break;
			}
			case PROP_NUM_TYPE_NA:
			default:
			{
				// Value is some other kind of type.
				// Currently we just support bool.

				boolean vBool = fastcmp("true", val);
				if (vBool == true || fastcmp("false", val))
				{
					// Value is a boolean.
					K_UserPropertyPush(user, key, USER_PROP_BOOL, &vBool);
				}
				else
				{
					// Value is invalid.
					CONS_Alert(CONS_WARNING, "Could not interpret user property \"%s\" value (%s)\n", param, val);
				}
				break;
			}
		}
	}
}

static void ParseTextmapVertexParameter(UINT32 i, const char *param, const char *val)
{
	if (fastcmp(param, "x"))
		vertexes[i].x = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "y"))
		vertexes[i].y = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "zfloor"))
	{
		vertexes[i].floorz = FLOAT_TO_FIXED(atof(val));
		vertexes[i].floorzset = true;
	}
	else if (fastcmp(param, "zceiling"))
	{
		vertexes[i].ceilingz = FLOAT_TO_FIXED(atof(val));
		vertexes[i].ceilingzset = true;
	}
}

typedef struct textmap_colormap_s {
	boolean used;
	INT32 lightcolor;
	UINT8 lightalpha;
	INT32 fadecolor;
	UINT8 fadealpha;
	UINT8 fadestart;
	UINT8 fadeend;
	UINT8 flags;
} textmap_colormap_t;

textmap_colormap_t textmap_colormap = { false, 0, 25, 0, 25, 0, 31, 0 };

typedef enum
{
    PD_A = 1,
    PD_B = 1<<1,
    PD_C = 1<<2,
    PD_D = 1<<3,
} planedef_t;

typedef struct textmap_plane_s {
    UINT8 defined;
    fixed_t a, b, c, d;
} textmap_plane_t;

textmap_plane_t textmap_planefloor = {0, 0, 0, 0, 0};
textmap_plane_t textmap_planeceiling = {0, 0, 0, 0, 0};

static void ParseTextmapSectorParameter(UINT32 i, const char *param, const char *val)
{
	if (fastcmp(param, "heightfloor"))
		sectors[i].floorheight = atol(val) << FRACBITS;
	else if (fastcmp(param, "heightceiling"))
		sectors[i].ceilingheight = atol(val) << FRACBITS;
	if (fastcmp(param, "texturefloor"))
		sectors[i].floorpic = P_AddLevelFlat(val, foundflats);
	else if (fastcmp(param, "textureceiling"))
		sectors[i].ceilingpic = P_AddLevelFlat(val, foundflats);
	else if (fastcmp(param, "lightlevel"))
		sectors[i].lightlevel = atol(val);
	else if (fastcmp(param, "lightfloor"))
		sectors[i].floorlightlevel = atol(val);
	else if (fastcmp(param, "lightfloorabsolute") && fastcmp("true", val))
		sectors[i].floorlightabsolute = true;
	else if (fastcmp(param, "lightceiling"))
		sectors[i].ceilinglightlevel = atol(val);
	else if (fastcmp(param, "lightceilingabsolute") && fastcmp("true", val))
		sectors[i].ceilinglightabsolute = true;
	else if (fastcmp(param, "id"))
		Tag_FSet(&sectors[i].tags, atol(val));
	else if (fastcmp(param, "moreids"))
	{
		const char* id = val;
		while (id)
		{
			Tag_Add(&sectors[i].tags, atol(id));
			if ((id = strchr(id, ' ')))
				id++;
		}
	}
	else if (fastcmp(param, "xpanningfloor"))
		sectors[i].floor_xoffs = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "ypanningfloor"))
		sectors[i].floor_yoffs = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "xpanningceiling"))
		sectors[i].ceiling_xoffs = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "ypanningceiling"))
		sectors[i].ceiling_yoffs = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "rotationfloor"))
		sectors[i].floorpic_angle = FixedAngle(FLOAT_TO_FIXED(atof(val)));
	else if (fastcmp(param, "rotationceiling"))
		sectors[i].ceilingpic_angle = FixedAngle(FLOAT_TO_FIXED(atof(val)));
	else if (fastcmp(param, "floorplane_a"))
	{
		textmap_planefloor.defined |= PD_A;
		textmap_planefloor.a = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "floorplane_b"))
	{
		textmap_planefloor.defined |= PD_B;
		textmap_planefloor.b = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "floorplane_c"))
	{
		textmap_planefloor.defined |= PD_C;
		textmap_planefloor.c = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "floorplane_d"))
	{
		textmap_planefloor.defined |= PD_D;
		textmap_planefloor.d = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "ceilingplane_a"))
	{
		textmap_planeceiling.defined |= PD_A;
		textmap_planeceiling.a = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "ceilingplane_b"))
	{
		textmap_planeceiling.defined |= PD_B;
		textmap_planeceiling.b = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "ceilingplane_c"))
	{
		textmap_planeceiling.defined |= PD_C;
		textmap_planeceiling.c = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "ceilingplane_d"))
	{
		textmap_planeceiling.defined |= PD_D;
		textmap_planeceiling.d = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "lightcolor"))
	{
		textmap_colormap.used = true;
		textmap_colormap.lightcolor = atol(val);
	}
	else if (fastcmp(param, "lightalpha"))
	{
		textmap_colormap.used = true;
		textmap_colormap.lightalpha = atol(val);
	}
	else if (fastcmp(param, "fadecolor"))
	{
		textmap_colormap.used = true;
		textmap_colormap.fadecolor = atol(val);
	}
	else if (fastcmp(param, "fadealpha"))
	{
		textmap_colormap.used = true;
		textmap_colormap.fadealpha = atol(val);
	}
	else if (fastcmp(param, "fadestart"))
	{
		textmap_colormap.used = true;
		textmap_colormap.fadestart = atol(val);
	}
	else if (fastcmp(param, "fadeend"))
	{
		textmap_colormap.used = true;
		textmap_colormap.fadeend = atol(val);
	}
	else if (fastcmp(param, "colormapfog") && fastcmp("true", val))
	{
		textmap_colormap.used = true;
		textmap_colormap.flags |= CMF_FOG;
	}
	else if (fastcmp(param, "colormapfadesprites") && fastcmp("true", val))
	{
		textmap_colormap.used = true;
		textmap_colormap.flags |= CMF_FADEFULLBRIGHTSPRITES;
	}
	else if (fastcmp(param, "colormapprotected") && fastcmp("true", val))
		sectors[i].colormap_protected = true;
	else if (fastcmp(param, "flipspecial_nofloor") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags & ~MSF_FLIPSPECIAL_FLOOR);
	else if (fastcmp(param, "flipspecial_ceiling") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_FLIPSPECIAL_CEILING);
	else if (fastcmp(param, "triggerspecial_touch") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_TRIGGERSPECIAL_TOUCH);
	else if (fastcmp(param, "triggerspecial_headbump") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_TRIGGERSPECIAL_HEADBUMP);
	else if (fastcmp(param, "invertprecip") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_INVERTPRECIP);
	else if (fastcmp(param, "gravityflip") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_GRAVITYFLIP);
	else if (fastcmp(param, "heatwave") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_HEATWAVE);
	else if (fastcmp(param, "noclipcamera") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_NOCLIPCAMERA);
	else if (fastcmp(param, "ripple_floor") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_RIPPLE_FLOOR);
	else if (fastcmp(param, "ripple_ceiling") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_RIPPLE_CEILING);
	else if (fastcmp(param, "invertencore") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_INVERTENCORE);
	else if (fastcmp(param, "flatlighting") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_FLATLIGHTING);
	else if (fastcmp(param, "forcedirectionallighting") && fastcmp("true", val))
		sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_DIRECTIONLIGHTING);
	else if (fastcmp(param, "nostepup") && fastcmp("true", val))
		sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_NOSTEPUP);
	else if (fastcmp(param, "doublestepup") && fastcmp("true", val))
		sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_DOUBLESTEPUP);
	else if (fastcmp(param, "nostepdown") && fastcmp("true", val))
		sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_NOSTEPDOWN);
	else if ((fastcmp(param, "cheatcheckactivator") || fastcmp(param, "starpostactivator")) && fastcmp("true", val))
		sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_CHEATCHECKACTIVATOR);
	else if (fastcmp(param, "exit") && fastcmp("true", val))
		sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_EXIT);
	else if (fastcmp(param, "deleteitems") && fastcmp("true", val))
		sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_DELETEITEMS);
	else if (fastcmp(param, "fan") && fastcmp("true", val))
		sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_FAN);
	else if (fastcmp(param, "zoomtubestart") && fastcmp("true", val))
		sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_ZOOMTUBESTART);
	else if (fastcmp(param, "zoomtubeend") && fastcmp("true", val))
		sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_ZOOMTUBEEND);
	else if (fastcmp(param, "friction"))
		sectors[i].friction = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "gravity"))
		sectors[i].gravity = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "damagetype"))
	{
		if (fastcmp(val, "Generic"))
			sectors[i].damagetype = SD_GENERIC;
		if (fastcmp(val, "Lava"))
			sectors[i].damagetype = SD_LAVA;
		if (fastcmp(val, "DeathPit"))
			sectors[i].damagetype = SD_DEATHPIT;
		if (fastcmp(val, "Instakill"))
			sectors[i].damagetype = SD_INSTAKILL;
		if (fastcmp(val, "Stumble"))
			sectors[i].damagetype = SD_STUMBLE;
	}
	else if (fastcmp(param, "action"))
		sectors[i].action = atol(val);
	else if (fastncmp(param, "stringarg", 9) && strlen(param) > 9)
	{
		size_t argnum = atol(param + 9);
		if (argnum >= NUM_SCRIPT_STRINGARGS)
			return;
		sectors[i].stringargs[argnum] = static_cast<char*>(Z_Malloc(strlen(val) + 1, PU_LEVEL, NULL));
		M_Memcpy(sectors[i].stringargs[argnum], val, strlen(val) + 1);
	}
	else if (fastncmp(param, "arg", 3) && strlen(param) > 3)
	{
		size_t argnum = atol(param + 3);
		if (argnum >= NUM_SCRIPT_ARGS)
			return;
		sectors[i].args[argnum] = atol(val);
	}
	else if (fastcmp(param, "repeatspecial") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | ((sectors[i].activation & ~SECSPAC_TRIGGERMASK) | SECSPAC_REPEATSPECIAL));
	else if (fastcmp(param, "continuousspecial") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | ((sectors[i].activation & ~SECSPAC_TRIGGERMASK) | SECSPAC_CONTINUOUSSPECIAL));
	else if (fastcmp(param, "playerenter") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | SECSPAC_ENTER);
	else if (fastcmp(param, "playerfloor") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | SECSPAC_FLOOR);
	else if (fastcmp(param, "playerceiling") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | SECSPAC_CEILING);
	else if (fastcmp(param, "monsterenter") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | SECSPAC_ENTERMONSTER);
	else if (fastcmp(param, "monsterfloor") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | SECSPAC_FLOORMONSTER);
	else if (fastcmp(param, "monsterceiling") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | SECSPAC_CEILINGMONSTER);
	else if (fastcmp(param, "missileenter") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | SECSPAC_ENTERMISSILE);
	else if (fastcmp(param, "missilefloor") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | SECSPAC_FLOORMISSILE);
	else if (fastcmp(param, "missileceiling") && fastcmp("true", val))
		sectors[i].activation = static_cast<sectoractionflags_t>(sectors[i].activation | SECSPAC_CEILINGMISSILE);
	else
		ParseUserProperty(&sectors[i].user, param, val);
}

static void ParseTextmapSidedefParameter(UINT32 i, const char *param, const char *val)
{
	if (fastcmp(param, "offsetx"))
		sides[i].textureoffset = atol(val)<<FRACBITS;
	else if (fastcmp(param, "offsety"))
		sides[i].rowoffset = atol(val)<<FRACBITS;
	else if (fastcmp(param, "texturetop"))
		sides[i].toptexture = R_TextureNumForName(val);
	else if (fastcmp(param, "texturebottom"))
		sides[i].bottomtexture = R_TextureNumForName(val);
	else if (fastcmp(param, "texturemiddle"))
		sides[i].midtexture = R_TextureNumForName(val);
	else if (fastcmp(param, "sector"))
		P_SetSidedefSector(i, atol(val));
	else if (fastcmp(param, "repeatcnt"))
		sides[i].repeatcnt = atol(val);
	else
		ParseUserProperty(&sides[i].user, param, val);
}

static void ParseTextmapLinedefParameter(UINT32 i, const char *param, const char *val)
{
	if (fastcmp(param, "id"))
		Tag_FSet(&lines[i].tags, atol(val));
	else if (fastcmp(param, "moreids"))
	{
		const char* id = val;
		while (id)
		{
			Tag_Add(&lines[i].tags, atol(id));
			if ((id = strchr(id, ' ')))
				id++;
		}
	}
	else if (fastcmp(param, "special"))
		lines[i].special = atol(val);
	else if (fastcmp(param, "v1"))
		P_SetLinedefV1(i, atol(val));
	else if (fastcmp(param, "v2"))
		P_SetLinedefV2(i, atol(val));
	else if (fastncmp(param, "stringarg", 9) && strlen(param) > 9)
	{
		size_t argnum = atol(param + 9);
		if (argnum >= NUM_SCRIPT_STRINGARGS)
			return;
		lines[i].stringargs[argnum] = static_cast<char*>(Z_Malloc(strlen(val) + 1, PU_LEVEL, NULL));
		M_Memcpy(lines[i].stringargs[argnum], val, strlen(val) + 1);
	}
	else if (fastncmp(param, "arg", 3) && strlen(param) > 3)
	{
		size_t argnum = atol(param + 3);
		if (argnum >= NUM_SCRIPT_ARGS)
			return;
		lines[i].args[argnum] = atol(val);
	}
	else if (fastcmp(param, "sidefront"))
		lines[i].sidenum[0] = atol(val);
	else if (fastcmp(param, "sideback"))
		lines[i].sidenum[1] = atol(val);
	else if (fastcmp(param, "alpha"))
		lines[i].alpha = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "blendmode") || fastcmp(param, "renderstyle"))
	{
		if (fastcmp(val, "translucent"))
			lines[i].blendmode = AST_COPY;
		else if (fastcmp(val, "add"))
			lines[i].blendmode = AST_ADD;
		else if (fastcmp(val, "subtract"))
			lines[i].blendmode = AST_SUBTRACT;
		else if (fastcmp(val, "reversesubtract"))
			lines[i].blendmode = AST_REVERSESUBTRACT;
		else if (fastcmp(val, "modulate"))
			lines[i].blendmode = AST_MODULATE;
		if (fastcmp(val, "fog"))
			lines[i].blendmode = AST_FOG;
	}

	// Flags
	else if (fastcmp(param, "blocking") && fastcmp("true", val))
		lines[i].flags |= ML_IMPASSABLE;
	else if (fastcmp(param, "blockplayers") && fastcmp("true", val))
		lines[i].flags |= ML_BLOCKPLAYERS;
	else if (fastcmp(param, "twosided") && fastcmp("true", val))
		lines[i].flags |= ML_TWOSIDED;
	else if (fastcmp(param, "dontpegtop") && fastcmp("true", val))
		lines[i].flags |= ML_DONTPEGTOP;
	else if (fastcmp(param, "dontpegbottom") && fastcmp("true", val))
		lines[i].flags |= ML_DONTPEGBOTTOM;
	else if (fastcmp(param, "skewtd") && fastcmp("true", val))
		lines[i].flags |= ML_SKEWTD;
	else if (fastcmp(param, "noclimb") && fastcmp("true", val))
		lines[i].flags |= ML_NOCLIMB;
	else if (fastcmp(param, "noskew") && fastcmp("true", val))
		lines[i].flags |= ML_NOSKEW;
	else if (fastcmp(param, "midpeg") && fastcmp("true", val))
		lines[i].flags |= ML_MIDPEG;
	else if (fastcmp(param, "midsolid") && fastcmp("true", val))
		lines[i].flags |= ML_MIDSOLID;
	else if (fastcmp(param, "wrapmidtex") && fastcmp("true", val))
		lines[i].flags |= ML_WRAPMIDTEX;
	else if (fastcmp(param, "blockmonsters") && fastcmp("true", val))
		lines[i].flags |= ML_BLOCKMONSTERS;
	else if (fastcmp(param, "nonet") && fastcmp("true", val))
		lines[i].flags |= ML_NONET;
	else if (fastcmp(param, "netonly") && fastcmp("true", val))
		lines[i].flags |= ML_NETONLY;
	else if (fastcmp(param, "notbouncy") && fastcmp("true", val))
		lines[i].flags |= ML_NOTBOUNCY;
	else if (fastcmp(param, "transfer") && fastcmp("true", val))
		lines[i].flags |= ML_TFERLINE;
	else if (fastcmp(param, "midtexinviswall") && fastcmp("true", val))
		lines[i].flags |= ML_MIDTEXINVISWALL;
	// Activation flags
	else if (fastcmp(param, "repeatspecial") && fastcmp("true", val))
		lines[i].activation |= SPAC_REPEATSPECIAL;
	else if (fastcmp(param, "playercross") && fastcmp("true", val))
		lines[i].activation |= SPAC_CROSS;
	else if (fastcmp(param, "monstercross") && fastcmp("true", val))
		lines[i].activation |= SPAC_CROSSMONSTER;
	else if (fastcmp(param, "missilecross") && fastcmp("true", val))
		lines[i].activation |= SPAC_CROSSMISSILE;
	else if (fastcmp(param, "playerpush") && fastcmp("true", val))
		lines[i].activation |= SPAC_PUSH;
	else if (fastcmp(param, "monsterpush") && fastcmp("true", val))
		lines[i].activation |= SPAC_PUSHMONSTER;
	else if (fastcmp(param, "impact") && fastcmp("true", val))
		lines[i].activation |= SPAC_IMPACT;
	else
		ParseUserProperty(&lines[i].user, param, val);
}

static void ParseTextmapThingParameter(UINT32 i, const char *param, const char *val)
{
	if (fastcmp(param, "id"))
		mapthings[i].tid = atol(val);
	else if (fastcmp(param, "x"))
		mapthings[i].x = atol(val);
	else if (fastcmp(param, "y"))
		mapthings[i].y = atol(val);
	else if (fastcmp(param, "height"))
		mapthings[i].z = atol(val);
	else if (fastcmp(param, "angle"))
		mapthings[i].angle = atol(val);
	else if (fastcmp(param, "pitch"))
		mapthings[i].pitch = atol(val);
	else if (fastcmp(param, "roll"))
		mapthings[i].roll = atol(val);
	else if (fastcmp(param, "type"))
		mapthings[i].type = atol(val);
	else if (fastcmp(param, "scale"))
	{
		if (udmf_version < 1)
		{
			mapthings[i].scale = FLOAT_TO_FIXED(atof(val));
		}
		else
		{
			mapthings[i].spritexscale = mapthings[i].spriteyscale = FLOAT_TO_FIXED(atof(val));
		}
	}
	else if (fastcmp(param, "scalex"))
	{
		if (udmf_version < 1)
		{
			mapthings[i].scale = FLOAT_TO_FIXED(atof(val));
		}
		else
		{
			mapthings[i].spritexscale = FLOAT_TO_FIXED(atof(val));
		}
	}
	else if (fastcmp(param, "scaley"))
	{
		if (udmf_version < 1)
		{
			mapthings[i].scale = FLOAT_TO_FIXED(atof(val));
		}
		else
		{
			mapthings[i].spriteyscale = FLOAT_TO_FIXED(atof(val));
		}
	}
	else if (fastcmp(param, "mobjscale"))
		mapthings[i].scale = FLOAT_TO_FIXED(atof(val));
	// Flags
	else if (fastcmp(param, "flip") && fastcmp("true", val))
		mapthings[i].options |= MTF_OBJECTFLIP;

	else if (fastcmp(param, "special"))
		mapthings[i].special = atol(val);
	else if (fastcmp(param, "foflayer"))
		mapthings[i].layer = atol(val);
	else if (fastncmp(param, "stringarg", 9) && strlen(param) > 9)
	{
		if (udmf_version < 1)
		{
			size_t argnum = atol(param + 9);
			if (argnum >= NUM_MAPTHING_STRINGARGS)
				return;
			size_t len = strlen(val);
			mapthings[i].thing_stringargs[argnum] = static_cast<char*>(Z_Malloc(len + 1, PU_LEVEL, NULL));
			M_Memcpy(mapthings[i].thing_stringargs[argnum], val, len);
			mapthings[i].thing_stringargs[argnum][len] = '\0';
		}
		else
		{
			size_t argnum = atol(param + 9);
			if (argnum >= NUM_SCRIPT_STRINGARGS)
				return;
			size_t len = strlen(val);
			mapthings[i].script_stringargs[argnum] = static_cast<char*>(Z_Malloc(len + 1, PU_LEVEL, NULL));
			M_Memcpy(mapthings[i].script_stringargs[argnum], val, len);
			mapthings[i].script_stringargs[argnum][len] = '\0';
		}
	}
	else if (fastncmp(param, "arg", 3) && strlen(param) > 3)
	{
		if (udmf_version < 1)
		{
			size_t argnum = atol(param + 3);
			if (argnum >= NUM_MAPTHING_ARGS)
				return;
			mapthings[i].thing_args[argnum] = atol(val);
		}
		else
		{
			size_t argnum = atol(param + 3);
			if (argnum >= NUM_SCRIPT_ARGS)
				return;
			mapthings[i].script_args[argnum] = atol(val);
		}
	}
	else if (fastncmp(param, "thingstringarg", 14) && strlen(param) > 14)
	{
		size_t argnum = atol(param + 14);
		if (argnum >= NUM_MAPTHING_STRINGARGS)
			return;
		size_t len = strlen(val);
		mapthings[i].thing_stringargs[argnum] = static_cast<char*>(Z_Malloc(len + 1, PU_LEVEL, NULL));
		M_Memcpy(mapthings[i].thing_stringargs[argnum], val, len);
		mapthings[i].thing_stringargs[argnum][len] = '\0';
	}
	else if (fastncmp(param, "thingarg", 8) && strlen(param) > 8)
	{
		size_t argnum = atol(param + 8);
		if (argnum >= NUM_MAPTHING_ARGS)
			return;
		mapthings[i].thing_args[argnum] = atol(val);
	}
	else
		ParseUserProperty(&mapthings[i].user, param, val);
}

/** From a given position table, run a specified parser function through a {}-encapsuled text.
  *
  * \param Position of the data to parse, in the textmap.
  * \param Structure number (mapthings, sectors, ...).
  * \param Parser function pointer.
  */
static void TextmapParse(UINT32 dataPos, size_t num, void (*parser)(UINT32, const char *, const char *))
{
	const char *param, *val;

	M_TokenizerSetEndPos(dataPos);
	param = M_TokenizerRead(0);
	if (!fastcmp(param, "{"))
	{
		CONS_Alert(CONS_WARNING, "Invalid UDMF data capsule!\n");
		return;
	}

	while (true)
	{
		param = M_TokenizerRead(0);
		if (fastcmp(param, "}"))
			break;
		val = M_TokenizerRead(1);
		parser(num, param, val);
	}
}

/** Provides a fix to the flat alignment coordinate transform from standard Textmaps.
 */
static void TextmapFixFlatOffsets(sector_t *sec)
{
	if (sec->floorpic_angle)
	{
		fixed_t pc = FINECOSINE(sec->floorpic_angle>>ANGLETOFINESHIFT);
		fixed_t ps = FINESINE  (sec->floorpic_angle>>ANGLETOFINESHIFT);
		fixed_t xoffs = sec->floor_xoffs;
		fixed_t yoffs = sec->floor_yoffs;
		sec->floor_xoffs = (FixedMul(xoffs, pc) % MAXFLATSIZE) - (FixedMul(yoffs, ps) % MAXFLATSIZE);
		sec->floor_yoffs = (FixedMul(xoffs, ps) % MAXFLATSIZE) + (FixedMul(yoffs, pc) % MAXFLATSIZE);
	}

	if (sec->ceilingpic_angle)
	{
		fixed_t pc = FINECOSINE(sec->ceilingpic_angle>>ANGLETOFINESHIFT);
		fixed_t ps = FINESINE  (sec->ceilingpic_angle>>ANGLETOFINESHIFT);
		fixed_t xoffs = sec->ceiling_xoffs;
		fixed_t yoffs = sec->ceiling_yoffs;
		sec->ceiling_xoffs = (FixedMul(xoffs, pc) % MAXFLATSIZE) - (FixedMul(yoffs, ps) % MAXFLATSIZE);
		sec->ceiling_yoffs = (FixedMul(xoffs, ps) % MAXFLATSIZE) + (FixedMul(yoffs, pc) % MAXFLATSIZE);
	}
}

static void TextmapUnfixFlatOffsets(sector_t *sec)
{
	if (sec->floorpic_angle)
	{
		fixed_t pc = FINECOSINE(sec->floorpic_angle>>ANGLETOFINESHIFT);
		fixed_t ps = -FINESINE (sec->floorpic_angle>>ANGLETOFINESHIFT);
		fixed_t xoffs = sec->floor_xoffs;
		fixed_t yoffs = sec->floor_yoffs;
		sec->floor_xoffs = (FixedMul(xoffs, pc) % MAXFLATSIZE) - (FixedMul(yoffs, ps) % MAXFLATSIZE);
		sec->floor_yoffs = (FixedMul(xoffs, ps) % MAXFLATSIZE) + (FixedMul(yoffs, pc) % MAXFLATSIZE);
	}

	if (sec->ceilingpic_angle)
	{
		fixed_t pc = FINECOSINE(sec->ceilingpic_angle>>ANGLETOFINESHIFT);
		fixed_t ps = -FINESINE (sec->ceilingpic_angle>>ANGLETOFINESHIFT);
		fixed_t xoffs = sec->ceiling_xoffs;
		fixed_t yoffs = sec->ceiling_yoffs;
		sec->ceiling_xoffs = (FixedMul(xoffs, pc) % MAXFLATSIZE) - (FixedMul(yoffs, ps) % MAXFLATSIZE);
		sec->ceiling_yoffs = (FixedMul(xoffs, ps) % MAXFLATSIZE) + (FixedMul(yoffs, pc) % MAXFLATSIZE);
	}
}

static INT32 P_ColorToRGBA(INT32 color, UINT8 alpha)
{
	UINT8 r = (color >> 16) & 0xFF;
	UINT8 g = (color >> 8) & 0xFF;
	UINT8 b = color & 0xFF;
	return R_PutRgbaRGBA(r, g, b, alpha);
}

static INT32 P_RGBAToColor(INT32 rgba)
{
	UINT8 r = R_GetRgbaR(rgba);
	UINT8 g = R_GetRgbaG(rgba);
	UINT8 b = R_GetRgbaB(rgba);
	return (r << 16) | (g << 8) | b;
}

static void TextmapWriteSlopeConstants(FILE *f, sector_t *sec)
{
	if (sec->f_slope != NULL)
	{
		const pslope_t *slope = sec->f_slope;

		fprintf(f, "floorplane_a = %f;\n", FIXED_TO_FLOAT(slope->constants[0]));
		fprintf(f, "floorplane_b = %f;\n", FIXED_TO_FLOAT(slope->constants[1]));
		fprintf(f, "floorplane_c = %f;\n", FIXED_TO_FLOAT(slope->constants[2]));
		fprintf(f, "floorplane_d = %f;\n", FIXED_TO_FLOAT(slope->constants[3]));
	}

	if (sec->c_slope != NULL)
	{
		const pslope_t *slope = sec->c_slope;

		fprintf(f, "ceilingplane_a = %f;\n", FIXED_TO_FLOAT(slope->constants[0]));
		fprintf(f, "ceilingplane_b = %f;\n", FIXED_TO_FLOAT(slope->constants[1]));
		fprintf(f, "ceilingplane_c = %f;\n", FIXED_TO_FLOAT(slope->constants[2]));
		fprintf(f, "ceilingplane_d = %f;\n", FIXED_TO_FLOAT(slope->constants[3]));
	}
}

typedef struct
{
	mapthing_t *teleport;
	mapthing_t *altview;
	mapthing_t *angleanchor;
} sectorspecialthings_t;

static boolean P_CanWriteTextmap(void)
{
	return roundqueue.writetextmap == true && roundqueue.size > 0;
}

static FILE *P_OpenTextmap(const char *mode, const char *error)
{
	FILE *f;
	char *filepath = va("%s" PATHSEP "TEXTMAP.%s.txt", srb2home, mapheaderinfo[gamemap-1]->lumpname);

	f = fopen(filepath, mode);
	if (!f)
	{
		CONS_Alert(CONS_ERROR, M_GetText("%s %s\n"), error, filepath);
	}

	return f;
}

static void P_WriteTextmapThing(FILE *f, mapthing_t *wmapthings, size_t i, size_t k)
{
	size_t j;
	fprintf(f, "thing // %s\n", sizeu1(k));
	fprintf(f, "{\n");
	if (wmapthings[i].tid != 0)
		fprintf(f, "id = %d;\n", wmapthings[i].tid);
	fprintf(f, "x = %d;\n", wmapthings[i].x);
	fprintf(f, "y = %d;\n", wmapthings[i].y);
	if (wmapthings[i].z != 0)
		fprintf(f, "height = %d;\n", wmapthings[i].z);
	fprintf(f, "angle = %d;\n", wmapthings[i].angle);
	if (wmapthings[i].pitch != 0)
		fprintf(f, "pitch = %d;\n", wmapthings[i].pitch);
	if (wmapthings[i].roll != 0)
		fprintf(f, "roll = %d;\n", wmapthings[i].roll);
	if (wmapthings[i].type != 0)
		fprintf(f, "type = %d;\n", wmapthings[i].type);
	if (wmapthings[i].spritexscale != FRACUNIT)
		fprintf(f, "scalex = %f;\n", FIXED_TO_FLOAT(wmapthings[i].spritexscale));
	if (wmapthings[i].spriteyscale != FRACUNIT)
		fprintf(f, "scaley = %f;\n", FIXED_TO_FLOAT(wmapthings[i].spriteyscale));
	if (wmapthings[i].scale != FRACUNIT)
		fprintf(f, "mobjscale = %f;\n", FIXED_TO_FLOAT(wmapthings[i].scale));
	if (wmapthings[i].options & MTF_OBJECTFLIP)
		fprintf(f, "flip = true;\n");
	if (wmapthings[i].special != 0)
		fprintf(f, "special = %d;\n", wmapthings[i].special);
	for (j = 0; j < NUM_SCRIPT_ARGS; j++)
		if (wmapthings[i].script_args[j] != 0)
			fprintf(f, "arg%s = %d;\n", sizeu1(j), wmapthings[i].script_args[j]);
	for (j = 0; j < NUM_SCRIPT_STRINGARGS; j++)
		if (mapthings[i].script_stringargs[j])
			fprintf(f, "stringarg%s = \"%s\";\n", sizeu1(j), mapthings[i].script_stringargs[j]);
	for (j = 0; j < NUM_MAPTHING_ARGS; j++)
		if (wmapthings[i].thing_args[j] != 0)
			fprintf(f, "thingarg%s = %d;\n", sizeu1(j), wmapthings[i].thing_args[j]);
	for (j = 0; j < NUM_MAPTHING_STRINGARGS; j++)
		if (mapthings[i].thing_stringargs[j])
			fprintf(f, "thingstringarg%s = \"%s\";\n", sizeu1(j), mapthings[i].thing_stringargs[j]);
	if (wmapthings[i].user.length > 0)
	{
		for (j = 0; j < wmapthings[i].user.length; j++)
		{
			mapUserProperty_t *const prop = &wmapthings[i].user.properties[j];
			switch (prop->type)
			{
				case USER_PROP_BOOL:
					fprintf(f, "user_%s = %s;\n", prop->key, (prop->valueBool == true) ? "true" : "false");
					break;
				case USER_PROP_INT:
					fprintf(f, "user_%s = %d;\n", prop->key, prop->valueInt);
					break;
				case USER_PROP_FIXED:
					fprintf(f, "user_%s = %f;\n", prop->key, FIXED_TO_FLOAT(prop->valueFixed));
					break;
				case USER_PROP_STR:
					fprintf(f, "user_%s = \"%s\";\n", prop->key, prop->valueStr);
					break;
			}
		}
	}
	fprintf(f, "}\n");
	fprintf(f, "\n");
}

static void P_WriteTextmap(void)
{
	size_t i, j, k;
	FILE *f;
	mtag_t firsttag;
	mapthing_t *wmapthings;
	vertex_t *wvertexes;
	sector_t *wsectors;
	line_t *wlines;
	side_t *wsides;
	mtag_t freetag;
	sectorspecialthings_t *specialthings;
	boolean *wusedvertexes;

	f = P_OpenTextmap("w", "Couldn't save map file");
	if (!f)
	{
		return;
	}

	wmapthings = static_cast<mapthing_t*>(Z_Calloc(nummapthings * sizeof(*mapthings), PU_LEVEL, NULL));
	wvertexes = static_cast<vertex_t*>(Z_Calloc(num_orig_vertexes * sizeof(*vertexes), PU_LEVEL, NULL));
	wsectors = static_cast<sector_t*>(Z_Calloc(numsectors * sizeof(*sectors), PU_LEVEL, NULL));
	wlines = static_cast<line_t*>(Z_Calloc(numlines * sizeof(*lines), PU_LEVEL, NULL));
	wsides = static_cast<side_t*>(Z_Calloc(numsides * sizeof(*sides), PU_LEVEL, NULL));
	specialthings = static_cast<sectorspecialthings_t*>(Z_Calloc(numsectors * sizeof(*sectors), PU_LEVEL, NULL));
	wusedvertexes = static_cast<boolean*>(Z_Calloc(num_orig_vertexes * sizeof(boolean), PU_LEVEL, NULL));

	memcpy(wmapthings, mapthings, nummapthings * sizeof(*mapthings));
	memcpy(wvertexes, vertexes, num_orig_vertexes * sizeof(*vertexes));
	memcpy(wsectors, sectors, numsectors * sizeof(*sectors));
	memcpy(wlines, lines, numlines * sizeof(*lines));
	memcpy(wsides, sides, numsides * sizeof(*sides));

	for (i = 0; i < nummapthings; i++)
	{
		if (mapthings[i].user.length)
		{
			wmapthings[i].user.properties = static_cast<mapUserProperty_t*>(memcpy(
				Z_Malloc(mapthings[i].user.length * sizeof(mapUserProperty_t), PU_LEVEL, NULL),
				mapthings[i].user.properties,
				mapthings[i].user.length * sizeof(mapUserProperty_t)
			));
		}
	}

	for (i = 0; i < numsectors; i++)
	{
		if (sectors[i].tags.count)
			wsectors[i].tags.tags = static_cast<mtag_t*>(memcpy(Z_Malloc(sectors[i].tags.count*sizeof(mtag_t), PU_LEVEL, NULL), sectors[i].tags.tags, sectors[i].tags.count*sizeof(mtag_t)));

		if (sectors[i].user.length)
		{
			wsectors[i].user.properties = static_cast<mapUserProperty_t*>(memcpy(
				Z_Malloc(sectors[i].user.length * sizeof(mapUserProperty_t), PU_LEVEL, NULL),
				sectors[i].user.properties,
				sectors[i].user.length * sizeof(mapUserProperty_t)
			));
		}
	}

	for (i = 0; i < numlines; i++)
	{
		size_t v;

		if (lines[i].tags.count)
			wlines[i].tags.tags = static_cast<mtag_t*>(memcpy(Z_Malloc(lines[i].tags.count * sizeof(mtag_t), PU_LEVEL, NULL), lines[i].tags.tags, lines[i].tags.count * sizeof(mtag_t)));

		if (lines[i].user.length)
		{
			wlines[i].user.properties = static_cast<mapUserProperty_t*>(memcpy(
				Z_Malloc(lines[i].user.length * sizeof(mapUserProperty_t), PU_LEVEL, NULL),
				lines[i].user.properties,
				lines[i].user.length * sizeof(mapUserProperty_t)
			));
		}

		v = lines[i].v1 - vertexes;
		wusedvertexes[v] = true;

		v = lines[i].v2 - vertexes;
		wusedvertexes[v] = true;
	}

	for (i = 0; i < numsides; i++)
	{
		if (sides[i].user.length)
		{
			wsides[i].user.properties = static_cast<mapUserProperty_t*>(memcpy(
				Z_Malloc(sides[i].user.length * sizeof(mapUserProperty_t), PU_LEVEL, NULL),
				sides[i].user.properties,
				sides[i].user.length * sizeof(mapUserProperty_t)
			));
		}
	}

	if (!udmf)
	{
		freetag = Tag_NextUnused(0);

		for (i = 0; i < nummapthings; i++)
		{
			subsector_t *ss;
			INT32 s;

			if (wmapthings[i].type != 751 && wmapthings[i].type != 752 && wmapthings[i].type != 758)
				continue;

			ss = R_PointInSubsector(wmapthings[i].x << FRACBITS, wmapthings[i].y << FRACBITS);

			if (!ss)
				continue;

			s = ss->sector - sectors;

			switch (wmapthings[i].type)
			{
				case 751:
					if (!specialthings[s].teleport)
						specialthings[s].teleport = &wmapthings[i];
					break;
				case 752:
					if (!specialthings[s].altview)
						specialthings[s].altview = &wmapthings[i];
					break;
				case 758:
					if (!specialthings[s].angleanchor)
						specialthings[s].angleanchor = &wmapthings[i];
					break;
				default:
					break;
			}
		}

		for (i = 0; i < numlines; i++)
		{
			INT32 s;

			switch (wlines[i].special)
			{
				case 1:
					TAG_ITER_SECTORS(Tag_FGet(&wlines[i].tags), s)
					{
						CONS_Alert(CONS_WARNING, M_GetText("Linedef %s applies custom gravity to sector %d. Changes to this gravity at runtime will not be reflected in the converted map. Use linedef type 469 for this.\n"), sizeu1(i), s);
						wsectors[s].gravity = FixedDiv(lines[i].frontsector->floorheight >> FRACBITS, 1000);
					}
					break;
				case 2:
					CONS_Alert(CONS_WARNING, M_GetText("Custom exit linedef %s detected. Changes to the next map at runtime will not be reflected in the converted map. Use linedef type 468 for this.\n"), sizeu1(i));
					wlines[i].args[0] = lines[i].frontsector->floorheight >> FRACBITS;
					wlines[i].args[2] = lines[i].frontsector->ceilingheight >> FRACBITS;
					break;
				case 5:
				case 50:
				case 51:
					CONS_Alert(CONS_WARNING, M_GetText("Linedef %s has type %d, which is not supported in UDMF.\n"), sizeu1(i), wlines[i].special);
					break;
				case 61:
					if (wlines[i].flags & ML_MIDSOLID)
						continue;
					if (!wlines[i].args[1])
						continue;
					CONS_Alert(CONS_WARNING, M_GetText("Linedef %s with crusher type 61 rises twice as fast on spawn. This behavior is not supported in UDMF.\n"), sizeu1(i));
					break;
				case 76:
					if (freetag == (mtag_t)MAXTAGS)
					{
						CONS_Alert(CONS_WARNING, M_GetText("No unused tag found. Linedef %s with type 76 cannot be converted.\n"), sizeu1(i));
						break;
					}
					TAG_ITER_SECTORS(wlines[i].args[0], s)
						for (j = 0; (unsigned)j < wsectors[s].linecount; j++)
						{
							line_t *line = wsectors[s].lines[j] - lines + wlines;
							if (line->special < 100 || line->special >= 300)
								continue;
							Tag_Add(&line->tags, freetag);
						}
					wlines[i].args[0] = freetag;
					freetag = Tag_NextUnused(freetag);
					break;
				case 259:
					if (wlines[i].args[3] & FOF_QUICKSAND)
						CONS_Alert(CONS_WARNING, M_GetText("Quicksand properties of custom FOF on linedef %s cannot be converted. Use linedef type 75 instead.\n"), sizeu1(i));
					if (wlines[i].args[3] & FOF_BUSTUP)
						CONS_Alert(CONS_WARNING, M_GetText("Bustable properties of custom FOF on linedef %s cannot be converted. Use linedef type 74 instead.\n"), sizeu1(i));
					break;
				case 412:
					if ((s = Tag_Iterate_Sectors(wlines[i].args[0], 0)) < 0)
						break;
					if (!specialthings[s].teleport)
						break;
					if (freetag == (mtag_t)MAXTAGS)
					{
						CONS_Alert(CONS_WARNING, M_GetText("No unused tag found. Linedef %s with type 412 cannot be converted.\n"), sizeu1(i));
						break;
					}
					specialthings[s].teleport->tid = freetag;
					wlines[i].args[0] = freetag;
					freetag = Tag_NextUnused(freetag);
					break;
				case 422:
					if ((s = Tag_Iterate_Sectors(wlines[i].args[0], 0)) < 0)
						break;
					if (!specialthings[s].altview)
						break;
					if (freetag == (mtag_t)MAXTAGS)
					{
						CONS_Alert(CONS_WARNING, M_GetText("No unused tag found. Linedef %s with type 422 cannot be converted.\n"), sizeu1(i));
						break;
					}
					specialthings[s].altview->tid = freetag;
					wlines[i].args[0] = freetag;
					specialthings[s].altview->pitch = wlines[i].args[2];
					freetag = Tag_NextUnused(freetag);
					break;
				case 447:
					CONS_Alert(CONS_WARNING, M_GetText("Linedef %s has change colormap action, which cannot be converted automatically. Tag arg0 to a sector with the desired colormap.\n"), sizeu1(i));
					if (wlines[i].flags & ML_TFERLINE)
						CONS_Alert(CONS_WARNING, M_GetText("Linedef %s mixes front and back colormaps, which is not supported in UDMF. Copy one colormap to the target sector first, then mix in the second one.\n"), sizeu1(i));
					break;
				case 455:
					CONS_Alert(CONS_WARNING, M_GetText("Linedef %s has fade colormap action, which cannot be converted automatically. Tag arg0 to a sector with the desired colormap.\n"), sizeu1(i));
					if (wlines[i].flags & ML_TFERLINE)
						CONS_Alert(CONS_WARNING, M_GetText("Linedef %s specifies starting colormap for the fade, which is not supported in UDMF. Change the colormap with linedef type 447 instead.\n"), sizeu1(i));
					break;
				case 457:
					if ((s = Tag_Iterate_Sectors(wlines[i].args[0], 0)) < 0)
						break;
					if (!specialthings[s].angleanchor)
						break;
					if (freetag == (mtag_t)MAXTAGS)
					{
						CONS_Alert(CONS_WARNING, M_GetText("No unused tag found. Linedef %s with type 457 cannot be converted.\n"), sizeu1(i));
						break;
					}
					specialthings[s].angleanchor->tid = freetag;
					wlines[i].args[0] = freetag;
					freetag = Tag_NextUnused(freetag);
					break;
				case 606:
					if (wlines[i].args[0] == MTAG_GLOBAL)
					{
						sector_t *sec = wlines[i].frontsector - sectors + wsectors;
						sec->extra_colormap = wsides[wlines[i].sidenum[0]].colormap_data;
					}
					else
					{
						TAG_ITER_SECTORS(wlines[i].args[0], s)
						{
							if (wsectors[s].colormap_protected)
								continue;

							wsectors[s].extra_colormap = wsides[wlines[i].sidenum[0]].colormap_data;
							if (freetag == (mtag_t)MAXTAGS)
							{
								CONS_Alert(CONS_WARNING, M_GetText("No unused tag found. Linedef %s with type 606 cannot be converted.\n"), sizeu1(i));
								break;
							}
							Tag_Add(&wsectors[s].tags, freetag);
							wlines[i].args[1] = freetag;
							freetag = Tag_NextUnused(freetag);
							break;
						}
					}
					break;
				default:
					break;
			}

			if (wlines[i].special >= 300 && wlines[i].special < 400)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Linedef %s is a linedef executor, which is not supported in UDMF. Use ACS instead.\n"), sizeu1(i));
				wlines[i].special = 0;
			}

			if (wlines[i].executordelay != 0)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Linedef %s has an linedef executor delay, which is not supported in UDMF. Use ACS instead.\n"), sizeu1(i));
			}
		}

		for (i = 0; i < numsectors; i++)
		{
			if (Tag_Find(&wsectors[i].tags, LE_CAPSULE0))
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s has reserved tag %d, which is not supported in UDMF. Use arg3 of the boss mapthing instead.\n"), sizeu1(i), LE_CAPSULE0);
			if (Tag_Find(&wsectors[i].tags, LE_CAPSULE1))
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s has reserved tag %d, which is not supported in UDMF. Use arg3 of the boss mapthing instead.\n"), sizeu1(i), LE_CAPSULE1);
			if (Tag_Find(&wsectors[i].tags, LE_CAPSULE2))
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s has reserved tag %d, which is not supported in UDMF. Use arg3 of the boss mapthing instead.\n"), sizeu1(i), LE_CAPSULE2);

			switch (GETSECSPECIAL(wsectors[i].special, 1))
			{
				case 9:
				case 10:
					CONS_Alert(CONS_WARNING, M_GetText("Sector %s has ring drainer effect, which is not supported in UDMF. Use action 460 instead.\n"), sizeu1(i));
					break;
				default:
					break;
			}

			switch (GETSECSPECIAL(wsectors[i].special, 2))
			{
				case 6:
					CONS_Alert(CONS_WARNING, M_GetText("Sector %s has emerald check trigger type, which is not supported in UDMF. Use ACS instead.\n"), sizeu1(i));
					break;
				case 7:
					CONS_Alert(CONS_WARNING, M_GetText("Sector %s has NiGHTS mare trigger type, which is not supported in UDMF. Use ACS instead.\n"), sizeu1(i));
					break;
				case 9:
					CONS_Alert(CONS_WARNING, M_GetText("Sector %s has Egg Capsule type, which is not supported in UDMF. Use action 464 instead.\n"), sizeu1(i));
					break;
				default:
					break;
			}

			if (wsectors[i].triggertag)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s uses a linedef executor trigger tag, which is not supported in UDMF. Use ACS instead.\n"), sizeu1(i));
			}
			if (wsectors[i].triggerer)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s uses a linedef executor trigger effect, which is not supported in UDMF. Use ACS instead.\n"), sizeu1(i));
			}
			if ((wsectors[i].flags & (MSF_TRIGGERLINE_PLANE|MSF_TRIGGERLINE_MOBJ)) != 0)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s uses a linedef executor trigger flag, which is not supported in UDMF. Use ACS instead.\n"), sizeu1(i));
			}
		}
	}

	fprintf(f, "namespace = \"ringracers\";\n");
	fprintf(f, "version = %d;\n", UDMF_CURRENT_VERSION);
	for (i = k = 0; i < nummapthings; i++)
	{
		if (!udmf)
		{
			if (wmapthings[i].type == mobjinfo[MT_WAYPOINT].doomednum
				|| wmapthings[i].type == mobjinfo[MT_WAYPOINT_ANCHOR].doomednum
				|| wmapthings[i].type == mobjinfo[MT_WAYPOINT_RISER].doomednum)
			{
				// Skip waypoints. Because the multi-thing setup was merged into a
				// single thing type in UDMF, these must be converted later.
				continue;
			}
		}

		P_WriteTextmapThing(f, wmapthings, i, k);

		k++;
	}

	j = 0;
	for (i = 0; i < num_orig_vertexes; i++)
	{
		if (wusedvertexes[i] == false)
		{
			continue;
		}

		fprintf(f, "vertex // %s\n", sizeu1(j));
		fprintf(f, "{\n");
		fprintf(f, "x = %f;\n", FIXED_TO_FLOAT(wvertexes[j].x));
		fprintf(f, "y = %f;\n", FIXED_TO_FLOAT(wvertexes[j].y));
		if (wvertexes[j].floorzset)
			fprintf(f, "zfloor = %f;\n", FIXED_TO_FLOAT(wvertexes[j].floorz));
		if (wvertexes[j].ceilingzset)
			fprintf(f, "zceiling = %f;\n", FIXED_TO_FLOAT(wvertexes[j].ceilingz));
		fprintf(f, "}\n");
		fprintf(f, "\n");

		j++;
	}

	for (i = 0; i < numlines; i++)
	{
		fprintf(f, "linedef // %s\n", sizeu1(i));
		fprintf(f, "{\n");
		fprintf(f, "v1 = %s;\n", sizeu1(wlines[i].v1 - vertexes));
		fprintf(f, "v2 = %s;\n", sizeu1(wlines[i].v2 - vertexes));
		fprintf(f, "sidefront = %d;\n", wlines[i].sidenum[0]);
		if (wlines[i].sidenum[1] != 0xffff)
			fprintf(f, "sideback = %d;\n", wlines[i].sidenum[1]);
		firsttag = Tag_FGet(&wlines[i].tags);
		if (firsttag != 0)
			fprintf(f, "id = %d;\n", firsttag);
		if (wlines[i].tags.count > 1)
		{
			fprintf(f, "moreids = \"");
			for (j = 1; j < wlines[i].tags.count; j++)
			{
				if (j > 1)
					fprintf(f, " ");
				fprintf(f, "%d", wlines[i].tags.tags[j]);
			}
			fprintf(f, "\";\n");
		}
		if (wlines[i].special != 0)
			fprintf(f, "special = %d;\n", wlines[i].special);
		for (j = 0; j < NUM_SCRIPT_ARGS; j++)
			if (wlines[i].args[j] != 0)
				fprintf(f, "arg%s = %d;\n", sizeu1(j), wlines[i].args[j]);
		for (j = 0; j < NUM_SCRIPT_STRINGARGS; j++)
			if (lines[i].stringargs[j])
				fprintf(f, "stringarg%s = \"%s\";\n", sizeu1(j), lines[i].stringargs[j]);
		if (wlines[i].alpha != FRACUNIT)
			fprintf(f, "alpha = %f;\n", FIXED_TO_FLOAT(wlines[i].alpha));
		if (wlines[i].blendmode != AST_COPY)
		{
			switch (wlines[i].blendmode)
			{
				case AST_ADD:
					fprintf(f, "renderstyle = \"add\";\n");
					break;
				case AST_SUBTRACT:
					fprintf(f, "renderstyle = \"subtract\";\n");
					break;
				case AST_REVERSESUBTRACT:
					fprintf(f, "renderstyle = \"reversesubtract\";\n");
					break;
				case AST_MODULATE:
					fprintf(f, "renderstyle = \"modulate\";\n");
					break;
				case AST_FOG:
					fprintf(f, "renderstyle = \"fog\";\n");
					break;
				default:
					break;
			}
		}
		if (wlines[i].flags & ML_IMPASSABLE)
			fprintf(f, "blocking = true;\n");
		if (wlines[i].flags & ML_BLOCKPLAYERS)
			fprintf(f, "blockplayers = true;\n");
		if (wlines[i].flags & ML_BLOCKMONSTERS)
			fprintf(f, "blockmonsters = true;\n");
		if (wlines[i].flags & ML_TWOSIDED)
			fprintf(f, "twosided = true;\n");
		if (wlines[i].flags & ML_DONTPEGTOP)
			fprintf(f, "dontpegtop = true;\n");
		if (wlines[i].flags & ML_DONTPEGBOTTOM)
			fprintf(f, "dontpegbottom = true;\n");
		if (wlines[i].flags & ML_SKEWTD)
			fprintf(f, "skewtd = true;\n");
		if (wlines[i].flags & ML_NOCLIMB)
			fprintf(f, "noclimb = true;\n");
		if (wlines[i].flags & ML_NOSKEW)
			fprintf(f, "noskew = true;\n");
		if (wlines[i].flags & ML_MIDPEG)
			fprintf(f, "midpeg = true;\n");
		if (wlines[i].flags & ML_MIDSOLID)
			fprintf(f, "midsolid = true;\n");
		if (wlines[i].flags & ML_WRAPMIDTEX)
			fprintf(f, "wrapmidtex = true;\n");
		if (wlines[i].flags & ML_NONET)
			fprintf(f, "nonet = true;\n");
		if (wlines[i].flags & ML_NETONLY)
			fprintf(f, "netonly = true;\n");
		if (wlines[i].flags & ML_NOTBOUNCY)
			fprintf(f, "notbouncy = true;\n");
		if (wlines[i].flags & ML_TFERLINE)
			fprintf(f, "transfer = true;\n");
		if (wlines[i].flags & ML_MIDTEXINVISWALL)
			fprintf(f, "midtexinviswall = true;\n");
		if (wlines[i].activation & SPAC_REPEATSPECIAL)
			fprintf(f, "repeatspecial = true;\n");
		if (wlines[i].activation & SPAC_CROSS)
			fprintf(f, "playercross = true;\n");
		if (wlines[i].activation & SPAC_CROSSMONSTER)
			fprintf(f, "monstercross = true;\n");
		if (wlines[i].activation & SPAC_CROSSMISSILE)
			fprintf(f, "missilecross = true;\n");
		if (wlines[i].activation & SPAC_PUSH)
			fprintf(f, "playerpush = true;\n");
		if (wlines[i].activation & SPAC_PUSHMONSTER)
			fprintf(f, "monsterpush = true;\n");
		if (wlines[i].activation & SPAC_IMPACT)
			fprintf(f, "impact = true;\n");
		if (wlines[i].user.length > 0)
		{
			for (j = 0; j < wlines[i].user.length; j++)
			{
				mapUserProperty_t *const prop = &wlines[i].user.properties[j];
				switch (prop->type)
				{
					case USER_PROP_BOOL:
						fprintf(f, "user_%s = %s;\n", prop->key, (prop->valueBool == true) ? "true" : "false");
						break;
					case USER_PROP_INT:
						fprintf(f, "user_%s = %d;\n", prop->key, prop->valueInt);
						break;
					case USER_PROP_FIXED:
						fprintf(f, "user_%s = %f;\n", prop->key, FIXED_TO_FLOAT(prop->valueFixed));
						break;
					case USER_PROP_STR:
						fprintf(f, "user_%s = \"%s\";\n", prop->key, prop->valueStr);
						break;
				}
			}
		}
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	for (i = 0; i < numsides; i++)
	{
		fprintf(f, "sidedef // %s\n", sizeu1(i));
		fprintf(f, "{\n");
		fprintf(f, "sector = %s;\n", sizeu1(wsides[i].sector - sectors));
		if (wsides[i].textureoffset != 0)
			fprintf(f, "offsetx = %d;\n", wsides[i].textureoffset >> FRACBITS);
		if (wsides[i].rowoffset != 0)
			fprintf(f, "offsety = %d;\n", wsides[i].rowoffset >> FRACBITS);
		if (wsides[i].toptexture > 0 && wsides[i].toptexture < numtextures)
			fprintf(f, "texturetop = \"%.*s\";\n", 8, textures[wsides[i].toptexture]->name);
		if (wsides[i].bottomtexture > 0 && wsides[i].bottomtexture < numtextures)
			fprintf(f, "texturebottom = \"%.*s\";\n", 8, textures[wsides[i].bottomtexture]->name);
		if (wsides[i].midtexture > 0 && wsides[i].midtexture < numtextures)
			fprintf(f, "texturemiddle = \"%.*s\";\n", 8, textures[wsides[i].midtexture]->name);
		if (wsides[i].repeatcnt != 0)
			fprintf(f, "repeatcnt = %d;\n", wsides[i].repeatcnt);
		if (wsides[i].user.length > 0)
		{
			for (j = 0; j < wsides[i].user.length; j++)
			{
				mapUserProperty_t *const prop = &wsides[i].user.properties[j];
				switch (prop->type)
				{
					case USER_PROP_BOOL:
						fprintf(f, "user_%s = %s;\n", prop->key, (prop->valueBool == true) ? "true" : "false");
						break;
					case USER_PROP_INT:
						fprintf(f, "user_%s = %d;\n", prop->key, prop->valueInt);
						break;
					case USER_PROP_FIXED:
						fprintf(f, "user_%s = %f;\n", prop->key, FIXED_TO_FLOAT(prop->valueFixed));
						break;
					case USER_PROP_STR:
						fprintf(f, "user_%s = \"%s\";\n", prop->key, prop->valueStr);
						break;
				}
			}
		}
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	for (i = 0; i < numsectors; i++)
	{
		fprintf(f, "sector // %s\n", sizeu1(i));
		fprintf(f, "{\n");
		fprintf(f, "heightfloor = %d;\n", wsectors[i].floorheight >> FRACBITS);
		fprintf(f, "heightceiling = %d;\n", wsectors[i].ceilingheight >> FRACBITS);
		if (wsectors[i].floorpic != -1)
			fprintf(f, "texturefloor = \"%s\";\n", levelflats[wsectors[i].floorpic].name);
		if (wsectors[i].ceilingpic != -1)
			fprintf(f, "textureceiling = \"%s\";\n", levelflats[wsectors[i].ceilingpic].name);
		fprintf(f, "lightlevel = %d;\n", wsectors[i].lightlevel);
		if (wsectors[i].floorlightlevel != 0)
			fprintf(f, "lightfloor = %d;\n", wsectors[i].floorlightlevel);
		if (wsectors[i].floorlightabsolute)
			fprintf(f, "lightfloorabsolute = true;\n");
		if (wsectors[i].ceilinglightlevel != 0)
			fprintf(f, "lightceiling = %d;\n", wsectors[i].ceilinglightlevel);
		if (wsectors[i].ceilinglightabsolute)
			fprintf(f, "lightceilingabsolute = true;\n");
		firsttag = Tag_FGet(&wsectors[i].tags);
		if (firsttag != 0)
			fprintf(f, "id = %d;\n", firsttag);
		if (wsectors[i].tags.count > 1)
		{
			fprintf(f, "moreids = \"");
			for (j = 1; j < wsectors[i].tags.count; j++)
			{
				if (j > 1)
					fprintf(f, " ");
				fprintf(f, "%d", wsectors[i].tags.tags[j]);
			}
			fprintf(f, "\";\n");
		}
		sector_t tempsec = wsectors[i];
		TextmapUnfixFlatOffsets(&tempsec);
		if (tempsec.floor_xoffs != 0)
			fprintf(f, "xpanningfloor = %f;\n", FIXED_TO_FLOAT(tempsec.floor_xoffs));
		if (tempsec.floor_yoffs != 0)
			fprintf(f, "ypanningfloor = %f;\n", FIXED_TO_FLOAT(tempsec.floor_yoffs));
		if (tempsec.ceiling_xoffs != 0)
			fprintf(f, "xpanningceiling = %f;\n", FIXED_TO_FLOAT(tempsec.ceiling_xoffs));
		if (tempsec.ceiling_yoffs != 0)
			fprintf(f, "ypanningceiling = %f;\n", FIXED_TO_FLOAT(tempsec.ceiling_yoffs));
		if (wsectors[i].floorpic_angle != 0)
			fprintf(f, "rotationfloor = %f;\n", FIXED_TO_FLOAT(AngleFixed(wsectors[i].floorpic_angle)));
		if (wsectors[i].ceilingpic_angle != 0)
			fprintf(f, "rotationceiling = %f;\n", FIXED_TO_FLOAT(AngleFixed(wsectors[i].ceilingpic_angle)));
		if (wsectors[i].extra_colormap)
		{
			INT32 lightcolor = P_RGBAToColor(wsectors[i].extra_colormap->rgba);
			UINT8 lightalpha = R_GetRgbaA(wsectors[i].extra_colormap->rgba);
			INT32 fadecolor = P_RGBAToColor(wsectors[i].extra_colormap->fadergba);
			UINT8 fadealpha = R_GetRgbaA(wsectors[i].extra_colormap->fadergba);

			if (lightcolor != 0)
				fprintf(f, "lightcolor = %d;\n", lightcolor);
			if (lightalpha != 25)
				fprintf(f, "lightalpha = %d;\n", lightalpha);
			if (fadecolor != 0)
				fprintf(f, "fadecolor = %d;\n", fadecolor);
			if (fadealpha != 25)
				fprintf(f, "fadealpha = %d;\n", fadealpha);
			if (wsectors[i].extra_colormap->fadestart != 0)
				fprintf(f, "fadestart = %d;\n", wsectors[i].extra_colormap->fadestart);
			if (wsectors[i].extra_colormap->fadeend != 31)
				fprintf(f, "fadeend = %d;\n", wsectors[i].extra_colormap->fadeend);
			if (wsectors[i].extra_colormap->flags & CMF_FOG)
				fprintf(f, "colormapfog = true;\n");
			if (wsectors[i].extra_colormap->flags & CMF_FADEFULLBRIGHTSPRITES)
				fprintf(f, "colormapfadesprites = true;\n");
		}
		if (wsectors[i].colormap_protected)
			fprintf(f, "colormapprotected = true;\n");
		if (!(wsectors[i].flags & MSF_FLIPSPECIAL_FLOOR))
			fprintf(f, "flipspecial_nofloor = true;\n");
		if (wsectors[i].flags & MSF_FLIPSPECIAL_CEILING)
			fprintf(f, "flipspecial_ceiling = true;\n");
		if (wsectors[i].flags & MSF_TRIGGERSPECIAL_TOUCH)
			fprintf(f, "triggerspecial_touch = true;\n");
		if (wsectors[i].flags & MSF_TRIGGERSPECIAL_HEADBUMP)
			fprintf(f, "triggerspecial_headbump = true;\n");
		if (wsectors[i].flags & MSF_INVERTPRECIP)
			fprintf(f, "invertprecip = true;\n");
		if (wsectors[i].flags & MSF_GRAVITYFLIP)
			fprintf(f, "gravityflip = true;\n");
		if (wsectors[i].flags & MSF_HEATWAVE)
			fprintf(f, "heatwave = true;\n");
		if (wsectors[i].flags & MSF_NOCLIPCAMERA)
			fprintf(f, "noclipcamera = true;\n");
		if (wsectors[i].flags & MSF_RIPPLE_FLOOR)
			fprintf(f, "ripple_floor = true;\n");
		if (wsectors[i].flags & MSF_RIPPLE_CEILING)
			fprintf(f, "ripple_ceiling = true;\n");
		if (wsectors[i].flags & MSF_INVERTENCORE)
			fprintf(f, "invertencore = true;\n");
		if (wsectors[i].specialflags & SSF_NOSTEPUP)
			fprintf(f, "nostepup = true;\n");
		if (wsectors[i].specialflags & SSF_DOUBLESTEPUP)
			fprintf(f, "doublestepup = true;\n");
		if (wsectors[i].specialflags & SSF_NOSTEPDOWN)
			fprintf(f, "nostepdown = true;\n");
		if (wsectors[i].specialflags & SSF_CHEATCHECKACTIVATOR)
			fprintf(f, "cheatcheckactivator = true;\n");
		if (wsectors[i].specialflags & SSF_EXIT)
			fprintf(f, "exit = true;\n");
		if (wsectors[i].specialflags & SSF_DELETEITEMS)
			fprintf(f, "deleteitems = true;\n");
		if (wsectors[i].specialflags & SSF_FAN)
			fprintf(f, "fan = true;\n");
		if (wsectors[i].specialflags & SSF_ZOOMTUBESTART)
			fprintf(f, "zoomtubestart = true;\n");
		if (wsectors[i].specialflags & SSF_ZOOMTUBEEND)
			fprintf(f, "zoomtubeend = true;\n");
		if (wsectors[i].friction != ORIG_FRICTION)
			fprintf(f, "friction = %f;\n", FIXED_TO_FLOAT(wsectors[i].friction));
		if (wsectors[i].gravity != FRACUNIT)
			fprintf(f, "gravity = %f;\n", FIXED_TO_FLOAT(wsectors[i].gravity));
		if (wsectors[i].damagetype != SD_NONE)
		{
			switch (wsectors[i].damagetype)
			{
				case SD_GENERIC:
					fprintf(f, "damagetype = \"Generic\";\n");
					break;
				case SD_LAVA:
					fprintf(f, "damagetype = \"Lava\";\n");
					break;
				case SD_DEATHPIT:
					fprintf(f, "damagetype = \"DeathPit\";\n");
					break;
				case SD_INSTAKILL:
					fprintf(f, "damagetype = \"Instakill\";\n");
					break;
				case SD_STUMBLE:
					fprintf(f, "damagetype = \"Stumble\";\n");
					break;
				default:
					break;
			}
		}
		TextmapWriteSlopeConstants(f, &wsectors[i]);
		if (wsectors[i].action != 0)
			fprintf(f, "action = %d;\n", wsectors[i].action);
		for (j = 0; j < NUM_SCRIPT_ARGS; j++)
			if (wsectors[i].args[j] != 0)
				fprintf(f, "arg%s = %d;\n", sizeu1(j), wsectors[i].args[j]);
		for (j = 0; j < NUM_SCRIPT_STRINGARGS; j++)
			if (wsectors[i].stringargs[j])
				fprintf(f, "stringarg%s = \"%s\";\n", sizeu1(j), wsectors[i].stringargs[j]);
		switch (wsectors[i].activation & SECSPAC_TRIGGERMASK)
		{
			case SECSPAC_REPEATSPECIAL:
			{
				fprintf(f, "repeatspecial = true;\n");
				break;
			}
			case SECSPAC_CONTINUOUSSPECIAL:
			{
				fprintf(f, "continuousspecial = true;\n");
				break;
			}
		}
		if (wsectors[i].activation & SECSPAC_ENTER)
			fprintf(f, "playerenter = true;\n");
		if (wsectors[i].activation & SECSPAC_FLOOR)
			fprintf(f, "playerfloor = true;\n");
		if (wsectors[i].activation & SECSPAC_CEILING)
			fprintf(f, "playerceiling = true;\n");
		if (wsectors[i].activation & SECSPAC_ENTERMONSTER)
			fprintf(f, "monsterenter = true;\n");
		if (wsectors[i].activation & SECSPAC_FLOORMONSTER)
			fprintf(f, "monsterfloor = true;\n");
		if (wsectors[i].activation & SECSPAC_CEILINGMONSTER)
			fprintf(f, "monsterceiling = true;\n");
		if (wsectors[i].activation & SECSPAC_ENTERMISSILE)
			fprintf(f, "missileenter = true;\n");
		if (wsectors[i].activation & SECSPAC_FLOORMISSILE)
			fprintf(f, "missilefloor = true;\n");
		if (wsectors[i].activation & SECSPAC_CEILINGMISSILE)
			fprintf(f, "missileceiling = true;\n");
		if (wsectors[i].user.length > 0)
		{
			for (j = 0; j < wsectors[i].user.length; j++)
			{
				mapUserProperty_t *const prop = &wsectors[i].user.properties[j];
				switch (prop->type)
				{
					case USER_PROP_BOOL:
						fprintf(f, "user_%s = %s;\n", prop->key, (prop->valueBool == true) ? "true" : "false");
						break;
					case USER_PROP_INT:
						fprintf(f, "user_%s = %d;\n", prop->key, prop->valueInt);
						break;
					case USER_PROP_FIXED:
						fprintf(f, "user_%s = %f;\n", prop->key, FIXED_TO_FLOAT(prop->valueFixed));
						break;
					case USER_PROP_STR:
						fprintf(f, "user_%s = \"%s\";\n", prop->key, prop->valueStr);
						break;
				}
			}
		}
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	fclose(f);

	for (i = 0; i < nummapthings; i++)
	{
		if (wmapthings[i].user.length)
			Z_Free(wmapthings[i].user.properties);
	}

	for (i = 0; i < numsectors; i++)
	{
		if (wsectors[i].tags.count)
			Z_Free(wsectors[i].tags.tags);

		if (wsectors[i].user.length)
			Z_Free(wsectors[i].user.properties);
	}

	for (i = 0; i < numlines; i++)
	{
		if (wlines[i].tags.count)
			Z_Free(wlines[i].tags.tags);

		if (wlines[i].user.length)
			Z_Free(wlines[i].user.properties);
	}

	for (i = 0; i < numsides; i++)
	{
		if (wsides[i].user.length)
			Z_Free(wsides[i].user.properties);
	}

	Z_Free(wmapthings);
	Z_Free(wvertexes);
	Z_Free(wsectors);
	Z_Free(wlines);
	Z_Free(wsides);
	Z_Free(specialthings);
	Z_Free(wusedvertexes);
}

static void P_WriteTextmapWaypoints(void)
{
	FILE *f;
	mobj_t *waypointmobj;

	// Append to output from P_WriteTextmap prior
	f = P_OpenTextmap("a", "Couldn't save map file (waypoints)");
	if (!f)
	{
		return;
	}

	for (
			waypointmobj = waypointcap;
			waypointmobj;
			waypointmobj = waypointmobj->tracer
	){
		P_WriteTextmapThing(f, waypointmobj->spawnpoint, 0, waypointmobj->spawnpoint - mapthings);
	}

	fclose(f);
}

/** Loads the textmap data, after obtaining the elements count and allocating their respective space.
  */
static void P_LoadTextmap(void)
{
	TracyCZone(__zone, true);

	UINT32 i;

	vertex_t   *vt;
	sector_t   *sc;
	line_t     *ld;
	side_t     *sd;
	mapthing_t *mt;

	/// Given the UDMF specs, some fields are given a default value.
	/// If an element's field has a default value set, it is omitted
	/// from the textmap, and therefore we have to account for it by
	/// preemptively setting that value beforehand.

	for (i = 0, vt = vertexes; i < numvertexes; i++, vt++)
	{
		// Defaults.
		vt->x = vt->y = INT32_MAX;
		vt->floorzset = vt->ceilingzset = false;
		vt->floorz = vt->ceilingz = 0;

		TextmapParse(vertexesPos[i], i, ParseTextmapVertexParameter);

		if (vt->x == INT32_MAX)
			I_Error("P_LoadTextmap: vertex %s has no x value set!\n", sizeu1(i));
		if (vt->y == INT32_MAX)
			I_Error("P_LoadTextmap: vertex %s has no y value set!\n", sizeu1(i));
	}

	for (i = 0, sc = sectors; i < numsectors; i++, sc++)
	{
		// Defaults.
		sc->floorheight = 0;
		sc->ceilingheight = 0;

		sc->floorpic = 0;
		sc->ceilingpic = 0;

		sc->lightlevel = 255;

		sc->special = 0;
		Tag_FSet(&sc->tags, 0);

		sc->floor_xoffs = sc->floor_yoffs = 0;
		sc->ceiling_xoffs = sc->ceiling_yoffs = 0;

		sc->floorpic_angle = sc->ceilingpic_angle = 0;

		sc->floorlightlevel = sc->ceilinglightlevel = 0;
		sc->floorlightabsolute = sc->ceilinglightabsolute = false;

		sc->colormap_protected = false;

		sc->gravity = FRACUNIT;

		sc->flags = MSF_FLIPSPECIAL_FLOOR;
		sc->specialflags = static_cast<sectorspecialflags_t>(0);
		sc->damagetype = SD_NONE;
		sc->triggertag = 0;
		sc->triggerer = TO_PLAYER;

		sc->friction = ORIG_FRICTION;

		sc->action = 0;
		memset(sc->args, 0, NUM_SCRIPT_ARGS*sizeof(*sc->args));
		memset(sc->stringargs, 0x00, NUM_SCRIPT_STRINGARGS*sizeof(*sc->stringargs));
		sc->activation = static_cast<sectoractionflags_t>(0);

		K_UserPropertiesClear(&sc->user);

		textmap_colormap.used = false;
		textmap_colormap.lightcolor = 0;
		textmap_colormap.lightalpha = 25;
		textmap_colormap.fadecolor = 0;
		textmap_colormap.fadealpha = 25;
		textmap_colormap.fadestart = 0;
		textmap_colormap.fadeend = 31;
		textmap_colormap.flags = 0;

		textmap_planefloor.defined = 0;
		textmap_planeceiling.defined = 0;

		TextmapParse(sectorsPos[i], i, ParseTextmapSectorParameter);

		P_InitializeSector(sc);
		if (textmap_colormap.used)
		{
			INT32 rgba = P_ColorToRGBA(textmap_colormap.lightcolor, textmap_colormap.lightalpha);
			INT32 fadergba = P_ColorToRGBA(textmap_colormap.fadecolor, textmap_colormap.fadealpha);
			sc->extra_colormap = sc->spawn_extra_colormap = R_CreateColormap(rgba, fadergba, textmap_colormap.fadestart, textmap_colormap.fadeend, textmap_colormap.flags);
		}

		if (textmap_planefloor.defined == (PD_A|PD_B|PD_C|PD_D))
		{
			sc->f_slope = MakeViaEquationConstants(textmap_planefloor.a, textmap_planefloor.b, textmap_planefloor.c, textmap_planefloor.d);
			sc->hasslope = true;
		}

		if (textmap_planeceiling.defined == (PD_A|PD_B|PD_C|PD_D))
		{
			sc->c_slope = MakeViaEquationConstants(textmap_planeceiling.a, textmap_planeceiling.b, textmap_planeceiling.c, textmap_planeceiling.d);
			sc->hasslope = true;
		}

		TextmapFixFlatOffsets(sc);
	}

	for (i = 0, ld = lines; i < numlines; i++, ld++)
	{
		// Defaults.
		ld->v1 = ld->v2 = NULL;
		ld->flags = 0;
		ld->special = 0;
		Tag_FSet(&ld->tags, 0);

		memset(ld->args, 0, NUM_SCRIPT_ARGS*sizeof(*ld->args));
		memset(ld->stringargs, 0x00, NUM_SCRIPT_STRINGARGS*sizeof(*ld->stringargs));
		ld->alpha = FRACUNIT;
		ld->executordelay = 0;
		ld->sidenum[0] = 0xffff;
		ld->sidenum[1] = 0xffff;

		ld->activation = 0;
		K_UserPropertiesClear(&ld->user);

		TextmapParse(linesPos[i], i, ParseTextmapLinedefParameter);

		if (!ld->v1)
			I_Error("P_LoadTextmap: linedef %s has no v1 value set!\n", sizeu1(i));
		if (!ld->v2)
			I_Error("P_LoadTextmap: linedef %s has no v2 value set!\n", sizeu1(i));
		if (ld->sidenum[0] == 0xffff)
			I_Error("P_LoadTextmap: linedef %s has no sidefront value set!\n", sizeu1(i));

		P_InitializeLinedef(ld);
	}

	for (i = 0, sd = sides; i < numsides; i++, sd++)
	{
		// Defaults.
		sd->textureoffset = 0;
		sd->rowoffset = 0;
		sd->toptexture = R_TextureNumForName("-");
		sd->midtexture = R_TextureNumForName("-");
		sd->bottomtexture = R_TextureNumForName("-");
		sd->sector = NULL;
		sd->repeatcnt = 0;

		K_UserPropertiesClear(&sd->user);

		TextmapParse(sidesPos[i], i, ParseTextmapSidedefParameter);

		if (!sd->sector)
			I_Error("P_LoadTextmap: sidedef %s has no sector value set!\n", sizeu1(i));

		P_InitializeSidedef(sd);
	}

	for (i = 0, mt = mapthings; i < nummapthings; i++, mt++)
	{
		// Defaults.
		mt->x = mt->y = 0;
		mt->angle = mt->pitch = mt->roll = 0;
		mt->type = 0;
		mt->options = 0;
		mt->z = 0;
		mt->extrainfo = 0;
		mt->tid = 0;
		mt->scale = FRACUNIT;
		mt->spritexscale = mt->spriteyscale = FRACUNIT;
		memset(mt->thing_args, 0, NUM_MAPTHING_ARGS*sizeof(*mt->thing_args));
		memset(mt->thing_stringargs, 0x00, NUM_MAPTHING_STRINGARGS*sizeof(*mt->thing_stringargs));
		mt->special = 0;
		memset(mt->script_args, 0, NUM_SCRIPT_ARGS*sizeof(*mt->script_args));
		memset(mt->script_stringargs, 0x00, NUM_SCRIPT_STRINGARGS*sizeof(*mt->script_stringargs));
		mt->layer = 0;
		mt->adjusted_z = INT32_MAX;
		mt->mobj = NULL;

		K_UserPropertiesClear(&mt->user);

		TextmapParse(mapthingsPos[i], i, ParseTextmapThingParameter);
	}

	TracyCZoneEnd(__zone);
}

static fixed_t
P_MirrorTextureOffset
(		fixed_t offset,
		fixed_t source_width,
		fixed_t actual_width)
{
	/*
	Adjusting the horizontal alignment is a bit ASS...
	Textures on the opposite side of the line will begin
	drawing from the opposite end.

	Start with the texture width and subtract the seg
	length to account for cropping/wrapping. Subtract the
	offset to mirror the alignment.
	*/
	return source_width - actual_width - offset;
}

static boolean P_CheckLineSideTripWire(line_t *ld, int p)
{
	INT32 n;

	side_t *sda;
	side_t *sdb;

	terrain_t *terrain;

	boolean tripwire;

	n = ld->sidenum[p];

	if (n == 0xffff)
		return false;

	sda = &sides[n];

	terrain = K_GetTerrainForTextureNum(sda->midtexture);
	tripwire = terrain && (terrain->flags & TRF_TRIPWIRE);

	// If we are texture TRIPWIRE and have the ML_MIDTEXINVISWALL, the replace texture with TRIPWLOW
	if (tripwire && (ld->flags & ML_MIDTEXINVISWALL)) // if we do backwards compat, update this to also swap for older custom maps without the flag
	{
		if (sda->midtexture == R_TextureNumForName("TRIPWIRE"))
		{
			sda->midtexture = R_TextureNumForName("TRIPWLOW");
		}
		else if (sda->midtexture == R_TextureNumForName("2RIPWIRE"))
		{
			sda->midtexture = R_TextureNumForName("2RIPWLOW");
		}
		else if (sda->midtexture == R_TextureNumForName("4RIPWIRE"))
		{
			sda->midtexture = R_TextureNumForName("4RIPWLOW");
		}
	}

	if (tripwire)
	{
		// copy midtexture to other side
		n = ld->sidenum[!p];

		if (n != 0xffff)
		{
			fixed_t linelength = FixedHypot(ld->dx, ld->dy);
			texture_t *tex = textures[sda->midtexture];

			sdb = &sides[n];

			sdb->midtexture = sda->midtexture;
			sdb->rowoffset = sda->rowoffset;

			// mirror texture alignment
			sdb->textureoffset = P_MirrorTextureOffset(
					sda->textureoffset, tex->width * FRACUNIT,
					linelength);
		}
	}

	return tripwire;
}

static void P_ProcessLinedefsAfterSidedefs(void)
{
	size_t i = numlines;
	line_t *ld = lines;
	const boolean subtractTripwire = ((mapheaderinfo[gamemap - 1]->levelflags & LF_SUBTRACTNUM) == LF_SUBTRACTNUM);

	for (; i--; ld++)
	{
		ld->frontsector = sides[ld->sidenum[0]].sector; //e6y: Can't be -1 here
		ld->backsector = ld->sidenum[1] != 0xffff ? sides[ld->sidenum[1]].sector : 0;

		// Check for tripwire, if either side matches then
		// copy that (mid)texture to the other side.
		ld->tripwire =
			P_CheckLineSideTripWire(ld, 0) ||
			P_CheckLineSideTripWire(ld, 1);

		if (ld->tripwire)
		{
			ld->blendmode = (subtractTripwire ? AST_COPY : AST_ADD);
			ld->alpha = FRACUNIT;
		}

		if (udmf)
			continue;

		switch (ld->special)
		{
		// Compile linedef text from both sidedefs for appropriate specials.
		case 331: // Trigger linedef executor: Skin - Continuous
		case 332: // Trigger linedef executor: Skin - Each time
		case 333: // Trigger linedef executor: Skin - Once
		case 443: // Calls a named Lua function
			if (ld->stringargs[0] && ld->stringargs[1])
			{
				size_t len[2];
				len[0] = strlen(ld->stringargs[0]);
				len[1] = strlen(ld->stringargs[1]);

				if (len[1])
				{
					ld->stringargs[0] = static_cast<char*>(Z_Realloc(ld->stringargs[0], len[0] + len[1] + 1, PU_LEVEL, NULL));
					M_Memcpy(ld->stringargs[0] + len[0] + 1, ld->stringargs[1], len[1] + 1);
				}

				Z_Free(ld->stringargs[1]);
				ld->stringargs[1] = NULL;
			}
			break;
		case 447: // Change colormap
		case 455: // Fade colormap
			if (ld->flags & ML_DONTPEGBOTTOM) // alternate alpha (by texture offsets)
			{
				extracolormap_t *exc = R_CopyColormap(sides[ld->sidenum[0]].colormap_data, false);
				INT16 alpha = std::max<fixed_t>(std::min<fixed_t>(sides[ld->sidenum[0]].textureoffset >> FRACBITS, 25), -25);
				INT16 fadealpha = std::max<fixed_t>(std::min<fixed_t>(sides[ld->sidenum[0]].rowoffset >> FRACBITS, 25), -25);

				// If alpha is negative, set "subtract alpha" flag and store absolute value
				if (alpha < 0)
				{
					alpha *= -1;
					ld->args[2] |= TMCF_SUBLIGHTA;
				}
				if (fadealpha < 0)
				{
					fadealpha *= -1;
					ld->args[2] |= TMCF_SUBFADEA;
				}

				exc->rgba = R_GetRgbaRGB(exc->rgba) + R_PutRgbaA(alpha);
				exc->fadergba = R_GetRgbaRGB(exc->fadergba) + R_PutRgbaA(fadealpha);

				if (!(sides[ld->sidenum[0]].colormap_data = R_GetColormapFromList(exc)))
				{
					exc->colormap = R_CreateLightTable(exc);
					R_AddColormapToList(exc);
					sides[ld->sidenum[0]].colormap_data = exc;
				}
				else
					Z_Free(exc);
			}
			break;
		}
	}
}

static boolean P_LoadMapData(const virtres_t *virt)
{
	TracyCZone(__zone, true);

	virtlump_t *virtvertexes = NULL, *virtsectors = NULL, *virtsidedefs = NULL, *virtlinedefs = NULL, *virtthings = NULL;

	// Count map data.
	if (udmf) // Count how many entries for each type we got in textmap.
	{
		virtlump_t *textmap = vres_Find(virt, "TEXTMAP");
		M_TokenizerOpen((char *)textmap->data, textmap->size);
		if (!TextmapCount(textmap->size))
		{
			M_TokenizerClose();
			TracyCZoneEnd(__zone);
			return false;
		}
	}
	else
	{
		virtthings   = vres_Find(virt, "THINGS");
		virtvertexes = vres_Find(virt, "VERTEXES");
		virtsectors  = vres_Find(virt, "SECTORS");
		virtsidedefs = vres_Find(virt, "SIDEDEFS");
		virtlinedefs = vres_Find(virt, "LINEDEFS");

		if (!virtthings)
			I_Error("THINGS lump not found");
		if (!virtvertexes)
			I_Error("VERTEXES lump not found");
		if (!virtsectors)
			I_Error("SECTORS lump not found");
		if (!virtsidedefs)
			I_Error("SIDEDEFS lump not found");
		if (!virtlinedefs)
			I_Error("LINEDEFS lump not found");

		// Traditional doom map format just assumes the number of elements from the lump sizes.
		numvertexes  = virtvertexes->size / sizeof (mapvertex_t);
		numsectors   = virtsectors->size  / sizeof (mapsector_t);
		numsides     = virtsidedefs->size / sizeof (mapsidedef_t);
		numlines     = virtlinedefs->size / sizeof (maplinedef_t);
		nummapthings = virtthings->size   / (5 * sizeof (INT16));
	}

	if (numvertexes <= 0)
		I_Error("Level has no vertices");
	if (numsectors <= 0)
		I_Error("Level has no sectors");
	if (numsides <= 0)
		I_Error("Level has no sidedefs");
	if (numlines <= 0)
		I_Error("Level has no linedefs");

	// Copy original vertex count before BSP modifications,
	// as it can alter how -writetextmap works.
	num_orig_vertexes = numvertexes;

	vertexes  = static_cast<vertex_t*>(Z_Calloc(numvertexes * sizeof (*vertexes), PU_LEVEL, NULL));
	sectors   = static_cast<sector_t*>(Z_Calloc(numsectors * sizeof (*sectors), PU_LEVEL, NULL));
	sides     = static_cast<side_t*>(Z_Calloc(numsides * sizeof (*sides), PU_LEVEL, NULL));
	lines     = static_cast<line_t*>(Z_Calloc(numlines * sizeof (*lines), PU_LEVEL, NULL));
	mapthings = static_cast<mapthing_t*>(Z_Calloc(nummapthings * sizeof (*mapthings), PU_LEVEL, NULL));

	// Allocate a big chunk of memory as big as our MAXLEVELFLATS limit.
	//Fab : FIXME: allocate for whatever number of flats - 512 different flats per level should be plenty
	foundflats = static_cast<levelflat_t*>(calloc(MAXLEVELFLATS, sizeof (*foundflats)));
	if (foundflats == NULL)
		I_Error("Ran out of memory while loading sectors\n");

	numlevelflats = 0;

	// Load map data.
	if (udmf)
	{
		P_LoadTextmap();
		M_TokenizerClose();
	}
	else
	{
		P_LoadVertices(virtvertexes->data);
		P_LoadSectors(virtsectors->data);
		P_LoadLinedefs(virtlinedefs->data);
		P_LoadSidedefs(virtsidedefs->data);
		P_LoadThings(virtthings->data);
	}

	P_ProcessLinedefsAfterSidedefs();

	R_ClearTextureNumCache(true);

	// set the sky flat num
	skyflatnum = P_AddLevelFlat(SKYFLATNAME, foundflats);

	// copy table for global usage
	levelflats = static_cast<levelflat_t*>(M_Memcpy(Z_Calloc(numlevelflats * sizeof (*levelflats), PU_LEVEL, NULL), foundflats, numlevelflats * sizeof (levelflat_t)));
	free(foundflats);

	// search for animated flats and set up
	P_SetupLevelFlatAnims();

	TracyCZoneEnd(__zone);
	return true;
}

static void P_InitializeSubsector(subsector_t *ss)
{
	ss->sector = NULL;
	ss->validcount = 0;
}

static inline void P_LoadSubsectors(UINT8 *data)
{
	mapsubsector_t *ms = (mapsubsector_t*)data;
	subsector_t *ss = subsectors;
	size_t i;

	for (i = 0; i < numsubsectors; i++, ss++, ms++)
	{
		ss->numlines = SHORT(ms->numsegs);
		ss->firstline = SHORT(ms->firstseg);
		P_InitializeSubsector(ss);
	}
}

static void P_LoadNodes(UINT8 *data)
{
	UINT8 j, k;
	mapnode_t *mn = (mapnode_t*)data;
	node_t *no = nodes;
	size_t i;

	for (i = 0; i < numnodes; i++, no++, mn++)
	{
		no->x = SHORT(mn->x)<<FRACBITS;
		no->y = SHORT(mn->y)<<FRACBITS;
		no->dx = SHORT(mn->dx)<<FRACBITS;
		no->dy = SHORT(mn->dy)<<FRACBITS;
		for (j = 0; j < 2; j++)
		{
			no->children[j] = SHORT(mn->children[j]);
			for (k = 0; k < 4; k++)
				no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
		}
	}
}

/** Computes the length of a seg in fracunits.
  *
  * \param seg Seg to compute length for.
  * \return Length in fracunits.
  */
static fixed_t P_SegLength(seg_t *seg)
{
	INT64 dx = (seg->v2->x - seg->v1->x)>>1;
	INT64 dy = (seg->v2->y - seg->v1->y)>>1;
	return FixedHypot(dx, dy)<<1;
}

#ifdef HWRENDER
/** Computes the length of a seg as a float.
  * This is needed for OpenGL.
  *
  * \param seg Seg to compute length for.
  * \return Length as a float.
  */
static inline float P_SegLengthFloat(seg_t *seg)
{
	float dx, dy;

	// make a vector (start at origin)
	dx = FIXED_TO_FLOAT(seg->v2->x - seg->v1->x);
	dy = FIXED_TO_FLOAT(seg->v2->y - seg->v1->y);

	return (float)hypot(dx, dy);
}
#endif

/** Updates the light offset
  *
  * \param li Seg to update the light offsets of
  */
void P_UpdateSegLightOffset(seg_t *li)
{
	const UINT8 contrast = maplighting.contrast;
	const fixed_t contrastFixed = ((fixed_t)contrast) * FRACUNIT;
	fixed_t light = FRACUNIT;
	fixed_t extralight = 0;

	if (maplighting.directional == true)
	{
		angle_t liAngle = R_PointToAngle2(0, 0, (li->v1->x - li->v2->x), (li->v1->y - li->v2->y)) - ANGLE_90;

		light = FixedMul(FINECOSINE(liAngle >> ANGLETOFINESHIFT), FINECOSINE(maplighting.angle >> ANGLETOFINESHIFT))
			+ FixedMul(FINESINE(liAngle >> ANGLETOFINESHIFT), FINESINE(maplighting.angle >> ANGLETOFINESHIFT));
		light = (light + FRACUNIT) / 2;
	}
	else
	{
		light = FixedDiv(R_PointToAngle2(0, 0, abs(li->v1->x - li->v2->x), abs(li->v1->y - li->v2->y)), ANGLE_90);
	}

	extralight = -contrastFixed + FixedMul(light, contrastFixed * 2);

	// Between -2 and 2 for software, -16 and 16 for hardware
	li->lightOffset = FixedFloor((extralight / 8) + (FRACUNIT / 2)) / FRACUNIT;
	li->hwLightOffset = FixedFloor(extralight + (FRACUNIT / 2)) / FRACUNIT;
}

boolean P_SectorUsesDirectionalLighting(const sector_t *sector)
{
	if (sector != NULL)
	{
		// explicitly turned on
		if (sector->flags & MSF_DIRECTIONLIGHTING)
		{
			return true;
		}

		// explicitly turned off
		if (sector->flags & MSF_FLATLIGHTING)
		{
			return false;
		}

		// automatically turned on
		if (sector->ceilingpic == skyflatnum)
		{
			// sky is visible
			return true;
		}
	}

	// default is off, for indoors
	return false;
}

boolean P_ApplyLightOffset(UINT8 baselightnum, const sector_t *sector)
{
	if (!P_SectorUsesDirectionalLighting(sector))
	{
		return false;
	}

	// Don't apply light offsets at full bright or full dark.
	// Is in steps of light num .
	return (baselightnum < LIGHTLEVELS-1 && baselightnum > 0);
}

boolean P_ApplyLightOffsetFine(UINT8 baselightlevel, const sector_t *sector)
{
	if (!P_SectorUsesDirectionalLighting(sector))
	{
		return false;
	}

	// Don't apply light offsets at full bright or full dark.
	// Uses exact light levels for more smoothness.
	return (baselightlevel < 255 && baselightlevel > 0);
}

static void P_InitializeSeg(seg_t *seg)
{
	if (seg->linedef)
	{
		UINT16 side = seg->linedef->sidenum[seg->side];

		if (side == 0xffff)
			I_Error("P_InitializeSeg: Seg %s refers to side %d of linedef %s, which doesn't exist!\n", sizeu1((size_t)(seg - segs)), seg->side, sizeu1((size_t)(seg->linedef - lines)));

		seg->sidedef = &sides[side];

		seg->frontsector = seg->sidedef->sector;
		seg->backsector = (seg->linedef->flags & ML_TWOSIDED) ? sides[seg->linedef->sidenum[seg->side ^ 1]].sector : NULL;
	}

#ifdef HWRENDER
	seg->pv1 = seg->pv2 = NULL;

	//Hurdler: 04/12/2000: for now, only used in hardware mode
	seg->lightmaps = NULL; // list of static lightmap for this seg
#endif

	seg->numlights = 0;
	seg->rlights = NULL;
	seg->polyseg = NULL;
	seg->dontrenderme = false;

	P_UpdateSegLightOffset(seg);
}

static void P_LoadSegs(UINT8 *data)
{
	mapseg_t *ms = (mapseg_t*)data;
	seg_t *seg = segs;
	size_t i;

	for (i = 0; i < numsegs; i++, seg++, ms++)
	{
		seg->v1 = &vertexes[SHORT(ms->v1)];
		seg->v2 = &vertexes[SHORT(ms->v2)];

		seg->side = SHORT(ms->side);

		seg->offset = (SHORT(ms->offset)) << FRACBITS;

		seg->angle = (SHORT(ms->angle)) << FRACBITS;

		seg->linedef = &lines[SHORT(ms->linedef)];

		seg->length = P_SegLength(seg);
		seg->flength = P_SegLengthFloat(seg);

		seg->glseg = false;
		P_InitializeSeg(seg);
	}
}

typedef enum {
	NT_DOOM,
	NT_XNOD,
	NT_ZNOD,
	NT_XGLN,
	NT_ZGLN,
	NT_XGL2,
	NT_ZGL2,
	NT_XGL3,
	NT_ZGL3,
	NT_UNSUPPORTED,
	NUMNODETYPES
} nodetype_t;

// Find out the BSP format.
static nodetype_t P_GetNodetype(const virtres_t *virt, UINT8 **nodedata)
{
	boolean supported[NUMNODETYPES] = {0};
	nodetype_t nodetype = NT_UNSUPPORTED;
	char signature[4 + 1];

	*nodedata = NULL;

	if (udmf)
	{
		virtlump_t *virtznodes = vres_Find(virt, "ZNODES");

		if (virtznodes && virtznodes->size)
		{
			*nodedata = virtznodes->data;
			supported[NT_XGLN] = supported[NT_XGL3] = true;
		}
	}
	else
	{
		virtlump_t *virtsegs = vres_Find(virt, "SEGS");
		virtlump_t *virtssectors;

		if (virtsegs && virtsegs->size)
		{
			virtlump_t *virtnodes = vres_Find(virt, "NODES");
			if (virtnodes && virtnodes->size)
			{
				*nodedata = virtnodes->data;
				return NT_DOOM; // Traditional map format BSP tree.
			}
		}
		else
		{
			virtssectors = vres_Find(virt, "SSECTORS");

			if (virtssectors && virtssectors->size)
			{ // Possibly GL nodes: NODES ignored, SSECTORS takes precedence as nodes lump, (It is confusing yeah) and has a signature.
				*nodedata = virtssectors->data;
				supported[NT_XGLN] = supported[NT_ZGLN] = supported[NT_XGL3] = true;
			}
			else
			{ // Possibly ZDoom extended nodes: SSECTORS is empty, NODES has a signature.
				virtlump_t *virtnodes = vres_Find(virt, "NODES");
				if (virtnodes && virtnodes->size)
				{
					*nodedata = virtnodes->data;
					supported[NT_XNOD] = supported[NT_ZNOD] = true;
				}
			}
		}
	}

	if (*nodedata == NULL)
	{
		I_Error("Level has no nodes (does your map have at least 2 sectors?)");
	}

	M_Memcpy(signature, *nodedata, 4);
	signature[4] = '\0';
	(*nodedata) += 4;

	if (!strcmp(signature, "XNOD"))
		nodetype = NT_XNOD;
	else if (!strcmp(signature, "ZNOD"))
		nodetype = NT_ZNOD;
	else if (!strcmp(signature, "XGLN"))
		nodetype = NT_XGLN;
	else if (!strcmp(signature, "ZGLN"))
		nodetype = NT_ZGLN;
	else if (!strcmp(signature, "XGL3"))
		nodetype = NT_XGL3;

	return supported[nodetype] ? nodetype : NT_UNSUPPORTED;
}

// Extended node formats feature additional vertices; useful for OpenGL, but totally useless in gamelogic.
static boolean P_LoadExtraVertices(UINT8 **data)
{
	UINT32 origvrtx = READUINT32((*data));
	UINT32 xtrvrtx = READUINT32((*data));
	line_t* ld = lines;
	vertex_t *oldpos = vertexes;
	size_t i;

	if (numvertexes != origvrtx) // If native vertex count doesn't match node original vertex count, bail out (broken data?).
	{
		CONS_Alert(CONS_WARNING, "Vertex count in map data and nodes differ!\n");
		return false;
	}

	if (!xtrvrtx)
		return true;

	// If extra vertexes were generated, reallocate the vertex array and fix the pointers.
	numvertexes += xtrvrtx;
	vertexes = static_cast<vertex_t*>(Z_Realloc(vertexes, numvertexes*sizeof(*vertexes), PU_LEVEL, NULL));

	for (i = 0, ld = lines; i < numlines; i++, ld++)
	{
		ld->v1 = &vertexes[ld->v1 - oldpos];
		ld->v2 = &vertexes[ld->v2 - oldpos];
	}

	// Read extra vertex data.
	for (i = origvrtx; i < numvertexes; i++)
	{
		vertexes[i].x = READFIXED((*data));
		vertexes[i].y = READFIXED((*data));
	}

	return true;
}

static boolean P_LoadExtendedSubsectorsAndSegs(UINT8 **data, nodetype_t nodetype)
{
	size_t i, k;
	INT16 m;
	seg_t *seg;

	// Subsectors
	numsubsectors = READUINT32((*data));
	subsectors = static_cast<subsector_t*>(Z_Calloc(numsubsectors*sizeof(*subsectors), PU_LEVEL, NULL));

	for (i = 0; i < numsubsectors; i++)
		subsectors[i].numlines = READUINT32((*data));

	// Segs
	numsegs = READUINT32((*data));
	segs = static_cast<seg_t*>(Z_Calloc(numsegs*sizeof(*segs), PU_LEVEL, NULL));

	for (i = 0, k = 0; i < numsubsectors; i++)
	{
		subsectors[i].firstline = k;
		P_InitializeSubsector(&subsectors[i]);

		switch (nodetype)
		{
		case NT_XGLN:
		case NT_XGL3:
			for (m = 0; m < subsectors[i].numlines; m++, k++)
			{
				UINT32 vertexnum = READUINT32((*data));
				UINT16 linenum;

				if (vertexnum >= numvertexes)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Seg %s in subsector %d has invalid vertex %d!\n", sizeu1(k), m, vertexnum);

				segs[k - 1 + ((m == 0) ? subsectors[i].numlines : 0)].v2 = segs[k].v1 = &vertexes[vertexnum];

				*data += sizeof (UINT32); // partner, can be ignored by software renderer

				linenum = (nodetype == NT_XGL3) ? READUINT32((*data)) : READUINT16((*data));
				if (linenum != 0xFFFF && linenum >= numlines)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Seg %s in subsector %s has invalid linedef %d!\n", sizeu1(k), sizeu2(i), linenum);
				segs[k].glseg = (linenum == 0xFFFF);
				segs[k].linedef = (linenum == 0xFFFF) ? NULL : &lines[linenum];
				segs[k].side = READUINT8((*data));
			}
			while (segs[subsectors[i].firstline].glseg)
			{
				subsectors[i].firstline++;
				if (subsectors[i].firstline == k)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Subsector %s does not have any valid segs!", sizeu1(i));
			}
			break;

		case NT_XNOD:
			for (m = 0; m < subsectors[i].numlines; m++, k++)
			{
				UINT32 v1num = READUINT32((*data));
				UINT32 v2num = READUINT32((*data));
				UINT16 linenum = READUINT16((*data));

				if (v1num >= numvertexes)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Seg %s in subsector %d has invalid v1 %d!\n", sizeu1(k), m, v1num);
				if (v2num >= numvertexes)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Seg %s in subsector %d has invalid v2 %d!\n", sizeu1(k), m, v2num);
				if (linenum >= numlines)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Seg %s in subsector %d has invalid linedef %d!\n", sizeu1(k), m, linenum);

				segs[k].v1 = &vertexes[v1num];
				segs[k].v2 = &vertexes[v2num];
				segs[k].linedef = &lines[linenum];
				segs[k].side = READUINT8((*data));
				segs[k].glseg = false;
			}
			break;

		default:
			return false;
		}
	}

	for (i = 0, seg = segs; i < numsegs; i++, seg++)
	{
		vertex_t *v1 = seg->v1;
		vertex_t *v2 = seg->v2;
		P_InitializeSeg(seg);
		seg->angle = R_PointToAngle2(v1->x, v1->y, v2->x, v2->y);
		if (seg->linedef)
		{
			vertex_t *v = (seg->side == 1) ? seg->linedef->v2 : seg->linedef->v1;
			segs[i].offset = FixedHypot(v1->x - v->x, v1->y - v->y);
		}
		seg->length = P_SegLength(seg);
		seg->flength = P_SegLengthFloat(seg);
	}

	return true;
}

// Auxiliary function: Shrink node ID from 32-bit to 16-bit.
static UINT16 ShrinkNodeID(UINT32 x) {
	UINT16 mask = (x >> 16) & 0xC000;
	UINT16 result = x;
	return result | mask;
}

static void P_LoadExtendedNodes(UINT8 **data, nodetype_t nodetype)
{
	node_t *mn;
	size_t i, j, k;
	boolean xgl3 = (nodetype == NT_XGL3);

	numnodes = READINT32((*data));
	nodes = static_cast<node_t*>(Z_Calloc(numnodes*sizeof(*nodes), PU_LEVEL, NULL));

	for (i = 0, mn = nodes; i < numnodes; i++, mn++)
	{
		// Splitter
		mn->x = xgl3 ? READINT32((*data)) : (READINT16((*data)) << FRACBITS);
		mn->y = xgl3 ? READINT32((*data)) : (READINT16((*data)) << FRACBITS);
		mn->dx = xgl3 ? READINT32((*data)) : (READINT16((*data)) << FRACBITS);
		mn->dy = xgl3 ? READINT32((*data)) : (READINT16((*data)) << FRACBITS);

		// Bounding boxes
		for (j = 0; j < 2; j++)
			for (k = 0; k < 4; k++)
				mn->bbox[j][k] = READINT16((*data)) << FRACBITS;

		//Children
		mn->children[0] = ShrinkNodeID(READUINT32((*data))); /// \todo Use UINT32 for node children in a future, instead?
		mn->children[1] = ShrinkNodeID(READUINT32((*data)));
	}
}

static void P_LoadMapBSP(const virtres_t *virt)
{
	UINT8 *nodedata = NULL;
	nodetype_t nodetype = P_GetNodetype(virt, &nodedata);

	switch (nodetype)
	{
	case NT_DOOM:
	{
		virtlump_t *virtssectors = vres_Find(virt, "SSECTORS");
		virtlump_t* virtnodes = vres_Find(virt, "NODES");
		virtlump_t *virtsegs = vres_Find(virt, "SEGS");

		numsubsectors = virtssectors->size / sizeof(mapsubsector_t);
		numnodes      = virtnodes->size    / sizeof(mapnode_t);
		numsegs       = virtsegs->size     / sizeof(mapseg_t);

		if (numsubsectors <= 0)
			I_Error("Level has no subsectors (did you forget to run it through a nodesbuilder?)");
		if (numnodes <= 0)
			I_Error("Level has no nodes (does your map have at least 2 sectors?)");
		if (numsegs <= 0)
			I_Error("Level has no segs");

		subsectors = static_cast<subsector_t*>(Z_Calloc(numsubsectors * sizeof(*subsectors), PU_LEVEL, NULL));
		nodes      = static_cast<node_t*>(Z_Calloc(numnodes * sizeof(*nodes), PU_LEVEL, NULL));
		segs       = static_cast<seg_t*>(Z_Calloc(numsegs * sizeof(*segs), PU_LEVEL, NULL));

		P_LoadSubsectors(virtssectors->data);
		P_LoadNodes(virtnodes->data);
		P_LoadSegs(virtsegs->data);
		break;
	}
	case NT_XNOD:
	case NT_XGLN:
	case NT_XGL3:
		if (!P_LoadExtraVertices(&nodedata))
			return;
		if (!P_LoadExtendedSubsectorsAndSegs(&nodedata, nodetype))
			return;
		P_LoadExtendedNodes(&nodedata, nodetype);
		break;
	default:
		CONS_Alert(CONS_WARNING, "Unsupported BSP format detected.\n");
		return;
	}
	return;
}

// Split from P_LoadBlockMap for convenience
// -- Monster Iestyn 08/01/18
static void P_ReadBlockMapLump(INT16 *wadblockmaplump, size_t count)
{
	size_t i;
	blockmaplump = static_cast<INT32*>(Z_Calloc(sizeof (*blockmaplump) * count, PU_LEVEL, NULL));

	// killough 3/1/98: Expand wad blockmap into larger internal one,
	// by treating all offsets except -1 as unsigned and zero-extending
	// them. This potentially doubles the size of blockmaps allowed,
	// because Doom originally considered the offsets as always signed.

	blockmaplump[0] = SHORT(wadblockmaplump[0]);
	blockmaplump[1] = SHORT(wadblockmaplump[1]);
	blockmaplump[2] = (INT32)(SHORT(wadblockmaplump[2])) & 0xffff;
	blockmaplump[3] = (INT32)(SHORT(wadblockmaplump[3])) & 0xffff;

	for (i = 4; i < count; i++)
	{
		INT16 t = SHORT(wadblockmaplump[i]);          // killough 3/1/98
		blockmaplump[i] = t == -1 ? (INT32)-1 : (INT32) t & 0xffff;
	}
}

// This needs to be a separate function
// because making both the WAD and PK3 loading code use
// the same functions is trickier than it looks for blockmap
// -- Monster Iestyn 09/01/18
static boolean P_LoadBlockMap(UINT8 *data, size_t count)
{
	if (!count || count >= 0x20000)
		return false;

	//CONS_Printf("Reading blockmap lump for pk3...\n");

	// no need to malloc anything, assume the data is uncompressed for now
	count /= 2;
	P_ReadBlockMapLump((INT16 *)data, count);

	bmaporgx = blockmaplump[0]<<FRACBITS;
	bmaporgy = blockmaplump[1]<<FRACBITS;
	bmapwidth = blockmaplump[2];
	bmapheight = blockmaplump[3];

	// clear out mobj chains
	count = sizeof (*blocklinks)* bmapwidth*bmapheight;
	blocklinks = static_cast<mobj_t**>(Z_Calloc(count, PU_LEVEL, NULL));
	blockmap = blockmaplump+4;

	// haleyjd 2/22/06: setup polyobject blockmap
	count = sizeof(*polyblocklinks) * bmapwidth * bmapheight;
	polyblocklinks = static_cast<polymaplink_t**>(Z_Calloc(count, PU_LEVEL, NULL));

	count = sizeof (*precipblocklinks)* bmapwidth*bmapheight;
	precipblocklinks = static_cast<precipmobj_t**>(Z_Calloc(count, PU_LEVEL, NULL));

	return true;
}

static boolean LineInBlock(fixed_t cx1, fixed_t cy1, fixed_t cx2, fixed_t cy2, fixed_t bx1, fixed_t by1)
{
	fixed_t bbox[4];
	line_t testline;
	vertex_t vtest;

	bbox[BOXRIGHT] = bx1 + MAPBLOCKUNITS;
	bbox[BOXTOP] = by1 + MAPBLOCKUNITS;
	bbox[BOXLEFT] = bx1;
	bbox[BOXBOTTOM] = by1;

	// Trivial rejection
	if (cx1 < bbox[BOXLEFT] && cx2 < bbox[BOXLEFT])
		return false;

	if (cx1 > bbox[BOXRIGHT] && cx2 > bbox[BOXRIGHT])
		return false;

	if (cy1 < bbox[BOXBOTTOM] && cy2 < bbox[BOXBOTTOM])
		return false;

	if (cy1 > bbox[BOXTOP] && cy2 > bbox[BOXTOP])
		return false;

	// Rats, guess we gotta check
	// if the line intersects
	// any sides of the block.
	cx1 <<= FRACBITS;
	cy1 <<= FRACBITS;
	cx2 <<= FRACBITS;
	cy2 <<= FRACBITS;
	bbox[BOXTOP] <<= FRACBITS;
	bbox[BOXBOTTOM] <<= FRACBITS;
	bbox[BOXLEFT] <<= FRACBITS;
	bbox[BOXRIGHT] <<= FRACBITS;

	testline.v1 = &vtest;

	testline.v1->x = cx1;
	testline.v1->y = cy1;
	testline.dx = cx2 - cx1;
	testline.dy = cy2 - cy1;

	if ((testline.dx > 0) ^ (testline.dy > 0))
		testline.slopetype = ST_NEGATIVE;
	else
		testline.slopetype = ST_POSITIVE;

	return P_BoxOnLineSide(bbox, &testline) == -1;
}

//
// killough 10/98:
//
// Rewritten to use faster algorithm.
//
// SSN Edit: Killough's wasn't accurate enough, sometimes excluding
// blocks that the line did in fact exist in, so now we use
// a fail-safe approach that puts a 'box' around each line.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
static void P_CreateBlockMap(void)
{
	size_t i;
	fixed_t minx = INT32_MAX, miny = INT32_MAX, maxx = INT32_MIN, maxy = INT32_MIN;
	// First find limits of map

	for (i = 0; i < numvertexes; i++)
	{
		if (vertexes[i].x>>FRACBITS < minx)
			minx = vertexes[i].x>>FRACBITS;
		else if (vertexes[i].x>>FRACBITS > maxx)
			maxx = vertexes[i].x>>FRACBITS;
		if (vertexes[i].y>>FRACBITS < miny)
			miny = vertexes[i].y>>FRACBITS;
		else if (vertexes[i].y>>FRACBITS > maxy)
			maxy = vertexes[i].y>>FRACBITS;
	}

	// Save blockmap parameters
	bmaporgx = minx << FRACBITS;
	bmaporgy = miny << FRACBITS;
	bmapwidth = ((maxx-minx) >> MAPBTOFRAC) + 1;
	bmapheight = ((maxy-miny) >> MAPBTOFRAC)+ 1;

	// Compute blockmap, which is stored as a 2d array of variable-sized lists.
	//
	// Pseudocode:
	//
	// For each linedef:
	//
	//   Map the starting and ending vertices to blocks.
	//
	//   Starting in the starting vertex's block, do:
	//
	//     Add linedef to current block's list, dynamically resizing it.
	//
	//     If current block is the same as the ending vertex's block, exit loop.
	//
	//     Move to an adjacent block by moving towards the ending block in
	//     either the x or y direction, to the block which contains the linedef.

	{
		typedef struct
		{
			INT32 n, nalloc;
			INT32 *list;
		} bmap_t; // blocklist structure

		size_t tot = bmapwidth * bmapheight; // size of blockmap
		bmap_t *bmap = static_cast<bmap_t*>(calloc(tot, sizeof (*bmap))); // array of blocklists
		boolean straight;

		if (bmap == NULL) I_Error("%s: Out of memory making blockmap", "P_CreateBlockMap");

		for (i = 0; i < numlines; i++)
		{
			// starting coordinates
			INT32 x = (lines[i].v1->x>>FRACBITS) - minx;
			INT32 y = (lines[i].v1->y>>FRACBITS) - miny;
			INT32 bxstart, bxend, bystart, byend, v2x, v2y, curblockx, curblocky;

			v2x = lines[i].v2->x>>FRACBITS;
			v2y = lines[i].v2->y>>FRACBITS;

			// Draw a "box" around the line.
			bxstart = (x >> MAPBTOFRAC);
			bystart = (y >> MAPBTOFRAC);

			v2x -= minx;
			v2y -= miny;

			bxend = ((v2x) >> MAPBTOFRAC);
			byend = ((v2y) >> MAPBTOFRAC);

			if (bxend < bxstart)
			{
				INT32 temp = bxstart;
				bxstart = bxend;
				bxend = temp;
			}

			if (byend < bystart)
			{
				INT32 temp = bystart;
				bystart = byend;
				byend = temp;
			}

			// Catch straight lines
			// This fixes the error where straight lines
			// directly on a blockmap boundary would not
			// be included in the proper blocks.
			if (lines[i].v1->y == lines[i].v2->y)
			{
				straight = true;
				bystart--;
				byend++;
			}
			else if (lines[i].v1->x == lines[i].v2->x)
			{
				straight = true;
				bxstart--;
				bxend++;
			}
			else
				straight = false;

			// Now we simply iterate block-by-block until we reach the end block.
			for (curblockx = bxstart; curblockx <= bxend; curblockx++)
			for (curblocky = bystart; curblocky <= byend; curblocky++)
			{
				size_t b = curblocky * bmapwidth + curblockx;

				if (b >= tot)
					continue;

				if (!straight && !(LineInBlock((fixed_t)x, (fixed_t)y, (fixed_t)v2x, (fixed_t)v2y, (fixed_t)(curblockx << MAPBTOFRAC), (fixed_t)(curblocky << MAPBTOFRAC))))
					continue;

				// Increase size of allocated list if necessary
				if (bmap[b].n >= bmap[b].nalloc)
				{
					// Graue 02-29-2004: make code more readable, don't realloc a null pointer
					// (because it crashes for me, and because the comp.lang.c FAQ says so)
					if (bmap[b].nalloc == 0)
						bmap[b].nalloc = 8;
					else
						bmap[b].nalloc *= 2;
					bmap[b].list = static_cast<INT32*>(Z_Realloc(bmap[b].list, bmap[b].nalloc * sizeof (*bmap->list), PU_CACHE, &bmap[b].list));
					if (!bmap[b].list)
						I_Error("Out of Memory in P_CreateBlockMap");
				}

				// Add linedef to end of list
				bmap[b].list[bmap[b].n++] = (INT32)i;
			}
		}

		// Compute the total size of the blockmap.
		//
		// Compression of empty blocks is performed by reserving two offset words
		// at tot and tot+1.
		//
		// 4 words, unused if this routine is called, are reserved at the start.
		{
			size_t count = tot + 6; // we need at least 1 word per block, plus reserved's

			for (i = 0; i < tot; i++)
				if (bmap[i].n)
					count += bmap[i].n + 2; // 1 header word + 1 trailer word + blocklist

			// Allocate blockmap lump with computed count
			blockmaplump = static_cast<INT32*>(Z_Calloc(sizeof (*blockmaplump) * count, PU_LEVEL, NULL));
		}

		// Now compress the blockmap.
		{
			size_t ndx = tot += 4; // Advance index to start of linedef lists
			bmap_t *bp = bmap; // Start of uncompressed blockmap

			blockmaplump[ndx++] = 0; // Store an empty blockmap list at start
			blockmaplump[ndx++] = -1; // (Used for compression)

			for (i = 4; i < tot; i++, bp++)
				if (bp->n) // Non-empty blocklist
				{
					blockmaplump[blockmaplump[i] = (INT32)(ndx++)] = 0; // Store index & header
					do
						blockmaplump[ndx++] = bp->list[--bp->n]; // Copy linedef list
					while (bp->n);
					blockmaplump[ndx++] = -1; // Store trailer
					Z_Free(bp->list); // Free linedef list
				}
				else // Empty blocklist: point to reserved empty blocklist
					blockmaplump[i] = (INT32)tot;

			free(bmap); // Free uncompressed blockmap
		}
	}
	{
		size_t count = sizeof (*blocklinks) * bmapwidth * bmapheight;
		// clear out mobj chains (copied from from P_LoadBlockMap)
		blocklinks = static_cast<mobj_t**>(Z_Calloc(count, PU_LEVEL, NULL));
		blockmap = blockmaplump + 4;

		// haleyjd 2/22/06: setup polyobject blockmap
		count = sizeof(*polyblocklinks) * bmapwidth * bmapheight;
		polyblocklinks = static_cast<polymaplink_t**>(Z_Calloc(count, PU_LEVEL, NULL));

		count = sizeof (*precipblocklinks)* bmapwidth*bmapheight;
		precipblocklinks = static_cast<precipmobj_t**>(Z_Calloc(count, PU_LEVEL, NULL));
	}
}

// PK3 version
// -- Monster Iestyn 09/01/18
static void P_LoadReject(UINT8 *data, size_t count)
{
	if (!count) // zero length, someone probably used ZDBSP
	{
		rejectmatrix = NULL;
		CONS_Debug(DBG_SETUP, "P_LoadReject: REJECT lump has size 0, will not be loaded\n");
	}
	else
	{
		rejectmatrix = static_cast<UINT8*>(Z_Malloc(count, PU_LEVEL, NULL)); // allocate memory for the reject matrix
		M_Memcpy(rejectmatrix, data, count); // copy the data into it
	}
}

static void P_LoadMapLUT(const virtres_t *virt)
{
	virtlump_t* virtblockmap = vres_Find(virt, "BLOCKMAP");
	virtlump_t* virtreject   = vres_Find(virt, "REJECT");

	// Lookup tables
	if (virtreject)
		P_LoadReject(virtreject->data, virtreject->size);
	else
		rejectmatrix = NULL;

	if (!(virtblockmap && P_LoadBlockMap(virtblockmap->data, virtblockmap->size)))
		P_CreateBlockMap();
}

//
// P_LinkMapData
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
static void P_LinkMapData(void)
{
	size_t i, j;
	line_t *li;
	sector_t *sector;
	subsector_t *ss = subsectors;
	size_t sidei;
	seg_t *seg;
	fixed_t bbox[4];

	// look up sector number for each subsector
	for (i = 0; i < numsubsectors; i++, ss++)
	{
		if (ss->firstline >= numsegs)
			CorruptMapError(va("P_LinkMapData: ss->firstline invalid "
				"(subsector %s, firstline refers to %d of %s)", sizeu1(i), ss->firstline,
				sizeu2(numsegs)));
		seg = &segs[ss->firstline];
		sidei = (size_t)(seg->sidedef - sides);
		if (!seg->sidedef)
			CorruptMapError(va("P_LinkMapData: seg->sidedef is NULL "
				"(subsector %s, firstline is %d)", sizeu1(i), ss->firstline));
		if (seg->sidedef - sides < 0 || seg->sidedef - sides > (UINT16)numsides)
			CorruptMapError(va("P_LinkMapData: seg->sidedef refers to sidedef %s of %s "
				"(subsector %s, firstline is %d)", sizeu1(sidei), sizeu2(numsides),
				sizeu3(i), ss->firstline));
		if (!seg->sidedef->sector)
			CorruptMapError(va("P_LinkMapData: seg->sidedef->sector is NULL "
				"(subsector %s, firstline is %d, sidedef is %s)", sizeu1(i), ss->firstline,
				sizeu1(sidei)));
		ss->sector = seg->sidedef->sector;
	}

	// count number of lines in each sector
	for (i = 0, li = lines; i < numlines; i++, li++)
	{
		li->frontsector->linecount++;

		if (li->backsector && li->backsector != li->frontsector)
			li->backsector->linecount++;
	}

	// allocate linebuffers for each sector
	for (i = 0, sector = sectors; i < numsectors; i++, sector++)
	{
		if (sector->linecount == 0) // no lines found?
		{
			sector->lines = NULL;
			CONS_Debug(DBG_SETUP, "P_LinkMapData: sector %s has no lines\n", sizeu1(i));
		}
		else
		{
			sector->lines = static_cast<line_t**>(Z_Calloc(sector->linecount * sizeof(line_t*), PU_LEVEL, NULL));

			// zero the count, since we'll later use this to track how many we've recorded
			sector->linecount = 0;
		}
	}

	// iterate through lines, assigning them to sectors' linebuffers,
	// and recalculate the counts in the process
	for (i = 0, li = lines; i < numlines; i++, li++)
	{
		li->frontsector->lines[li->frontsector->linecount++] = li;

		if (li->backsector && li->backsector != li->frontsector)
			li->backsector->lines[li->backsector->linecount++] = li;
	}

	// set soundorg's position for each sector
	for (i = 0, sector = sectors; i < numsectors; i++, sector++)
	{
		M_ClearBox(bbox);

		if (sector->linecount != 0)
		{
			for (j = 0; j < sector->linecount; j++)
			{
				li = sector->lines[j];
				M_AddToBox(bbox, li->v1->x, li->v1->y);
				M_AddToBox(bbox, li->v2->x, li->v2->y);
			}
		}

		// set the degenmobj_t to the middle of the bounding box
		sector->soundorg.x = (((bbox[BOXRIGHT]>>FRACBITS) + (bbox[BOXLEFT]>>FRACBITS))/2)<<FRACBITS;
		sector->soundorg.y = (((bbox[BOXTOP]>>FRACBITS) + (bbox[BOXBOTTOM]>>FRACBITS))/2)<<FRACBITS;
		sector->soundorg.z = sector->floorheight; // default to sector's floor height
	}
}

// For maps in binary format, add multi-tags from linedef specials. This must be done
// before any linedef specials have been processed.
static void P_AddBinaryMapTagsFromLine(sector_t *sector, line_t *line)
{
	Tag_Add(&sector->tags, Tag_FGet(&line->tags));
	if (line->flags & ML_BLOCKMONSTERS) {
		if (sides[line->sidenum[0]].textureoffset)
			Tag_Add(&sector->tags, (INT32)sides[line->sidenum[0]].textureoffset / FRACUNIT);
		if (sides[line->sidenum[0]].rowoffset)
			Tag_Add(&sector->tags, (INT32)sides[line->sidenum[0]].rowoffset / FRACUNIT);
	}
	if (line->flags & ML_TFERLINE) {
		if (sides[line->sidenum[1]].textureoffset)
			Tag_Add(&sector->tags, (INT32)sides[line->sidenum[1]].textureoffset / FRACUNIT);
		if (sides[line->sidenum[1]].rowoffset)
			Tag_Add(&sector->tags, (INT32)sides[line->sidenum[1]].rowoffset / FRACUNIT);
	}
}

static void P_AddBinaryMapTags(void)
{
	size_t i;

	for (i = 0; i < numlines; i++) {
		// 97: Apply Tag to Front Sector
		// 98: Apply Tag to Back Sector
		// 99: Apply Tag to Front and Back Sectors
		if (lines[i].special == 97 || lines[i].special == 99)
			P_AddBinaryMapTagsFromLine(lines[i].frontsector, &lines[i]);
		if (lines[i].special == 98 || lines[i].special == 99)
			P_AddBinaryMapTagsFromLine(lines[i].backsector, &lines[i]);
	}

	// Run this loop after the 97-99 loop to ensure that 96 can search through all of the
	// 97-99-applied tags.
	for (i = 0; i < numlines; i++) {
		size_t j;
		mtag_t tag, target_tag;
		mtag_t offset_tags[4];

		// 96: Apply Tag to Tagged Sectors
		if (lines[i].special != 96)
			continue;

		tag = Tag_FGet(&lines[i].frontsector->tags);
		target_tag = Tag_FGet(&lines[i].tags);
		memset(offset_tags, 0, sizeof(mtag_t)*4);
		if (lines[i].flags & ML_BLOCKMONSTERS) {
			offset_tags[0] = (INT32)sides[lines[i].sidenum[0]].textureoffset / FRACUNIT;
			offset_tags[1] = (INT32)sides[lines[i].sidenum[0]].rowoffset / FRACUNIT;
		}
		if (lines[i].flags & ML_TFERLINE) {
			offset_tags[2] = (INT32)sides[lines[i].sidenum[1]].textureoffset / FRACUNIT;
			offset_tags[3] = (INT32)sides[lines[i].sidenum[1]].rowoffset / FRACUNIT;
		}

		for (j = 0; j < numsectors; j++) {
			boolean matches_target_tag = target_tag && Tag_Find(&sectors[j].tags, target_tag);
			size_t k;
			for (k = 0; k < 4; k++) {
				if (lines[i].flags & ML_WRAPMIDTEX) {
					if (matches_target_tag || (offset_tags[k] && Tag_Find(&sectors[j].tags, offset_tags[k]))) {
						Tag_Add(&sectors[j].tags, tag);
						break;
					}
				} else if (matches_target_tag) {
					if (k == 0)
						Tag_Add(&sectors[j].tags, tag);
					if (offset_tags[k])
						Tag_Add(&sectors[j].tags, offset_tags[k]);
				}
			}
		}
	}

	for (i = 0; i < nummapthings; i++)
	{
		switch (mapthings[i].type)
		{
		case 291:
		case 322:
		case 750:
		case 760:
		case 761:
		case 762:
			mapthings[i].tid = mapthings[i].angle;
			break;
		case 290:
		case 292:
		case 294:
		case 780:
		case 2020: // MT_LOOPENDPOINT
		case 2021: // MT_LOOPCENTERPOINT
			mapthings[i].tid = mapthings[i].extrainfo;
			break;
		default:
			break;
		}
	}
}

static line_t *P_FindPointPushLine(taglist_t *list)
{
	INT32 i, l;

	for (i = 0; i < list->count; i++)
	{
		mtag_t tag = list->tags[i];
		TAG_ITER_LINES(tag, l)
		{
			if (Tag_FGet(&lines[l].tags) != tag)
				continue;

			if (lines[l].special != 547)
				continue;

			return &lines[l];
		}
	}

	return NULL;
}

static void P_SetBinaryFOFAlpha(line_t *line)
{
	if (sides[line->sidenum[0]].toptexture > 0)
	{
		line->args[1] = sides[line->sidenum[0]].toptexture;

		if (line->args[1] == 901) // additive special
		{
			line->args[1] = 0xff;
			line->args[2] = TMB_ADD;
		}
		else if (line->args[1] == 902) // subtractive special
		{
			line->args[1] = 0xff;
			line->args[2] = TMB_SUBTRACT;
		}
		else if (line->args[1] >= 1001) // fourth digit
		{
			line->args[1] %= 1000;
			line->args[2] = (sides[line->sidenum[0]].toptexture/1000); // TMB_TRANSLUCNET, TMB_ADD, etc
		}
	}
	else
	{
		line->args[1] = 128;
		line->args[2] = TMB_TRANSLUCENT;
	}
}

static INT32 P_GetFOFFlags(INT32 oldflags)
{
	INT32 result = 0;
	if (oldflags & FF_OLD_EXISTS)
		result |= FOF_EXISTS;
	if (oldflags & FF_OLD_BLOCKPLAYER)
		result |= FOF_BLOCKPLAYER;
	if (oldflags & FF_OLD_BLOCKOTHERS)
		result |= FOF_BLOCKOTHERS;
	if (oldflags & FF_OLD_RENDERSIDES)
		result |= FOF_RENDERSIDES;
	if (oldflags & FF_OLD_RENDERPLANES)
		result |= FOF_RENDERPLANES;
	if (oldflags & FF_OLD_SWIMMABLE)
		result |= FOF_SWIMMABLE;
	if (oldflags & FF_OLD_NOSHADE)
		result |= FOF_NOSHADE;
	if (oldflags & FF_OLD_CUTSOLIDS)
		result |= FOF_CUTSOLIDS;
	if (oldflags & FF_OLD_CUTEXTRA)
		result |= FOF_CUTEXTRA;
	if (oldflags & FF_OLD_CUTSPRITES)
		result |= FOF_CUTSPRITES;
	if (oldflags & FF_OLD_BOTHPLANES)
		result |= FOF_BOTHPLANES;
	if (oldflags & FF_OLD_EXTRA)
		result |= FOF_EXTRA;
	if (oldflags & FF_OLD_TRANSLUCENT)
		result |= FOF_TRANSLUCENT;
	if (oldflags & FF_OLD_FOG)
		result |= FOF_FOG;
	if (oldflags & FF_OLD_INVERTPLANES)
		result |= FOF_INVERTPLANES;
	if (oldflags & FF_OLD_ALLSIDES)
		result |= FOF_ALLSIDES;
	if (oldflags & FF_OLD_INVERTSIDES)
		result |= FOF_INVERTSIDES;
	if (oldflags & FF_OLD_DOUBLESHADOW)
		result |= FOF_DOUBLESHADOW;
	if (oldflags & FF_OLD_FLOATBOB)
		result |= FOF_FLOATBOB;
	if (oldflags & FF_OLD_NORETURN)
		result |= FOF_NORETURN;
	if (oldflags & FF_OLD_CRUMBLE)
		result |= FOF_CRUMBLE;
	if (oldflags & FF_OLD_GOOWATER)
		result |= FOF_GOOWATER;
	if (oldflags & FF_OLD_MARIO)
		result |= FOF_MARIO;
	if (oldflags & FF_OLD_BUSTUP)
		result |= FOF_BUSTUP;
	if (oldflags & FF_OLD_QUICKSAND)
		result |= FOF_QUICKSAND;
	if (oldflags & FF_OLD_PLATFORM)
		result |= FOF_PLATFORM;
	if (oldflags & FF_OLD_REVERSEPLATFORM)
		result |= FOF_REVERSEPLATFORM;
	if (oldflags & FF_OLD_RIPPLE)
		result |= FOF_RIPPLE;
	if (oldflags & FF_OLD_COLORMAPONLY)
		result |= FOF_COLORMAPONLY;
	return result;
}

static INT32 P_GetFOFBusttype(INT32 oldflags)
{
	if (oldflags & FF_OLD_SHATTER)
		return TMFB_TOUCH;
	if (oldflags & FF_OLD_SPINBUST)
		return TMFB_SPIN;
	if (oldflags & FF_OLD_STRONGBUST)
		return TMFB_STRONG;
	return TMFB_REGULAR;
}

static void P_ConvertBinaryLinedefTypes(void)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		mtag_t tag = Tag_FGet(&lines[i].tags);

		switch (lines[i].special)
		{
		case 2: //Custom exit
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] |= TMEF_SKIPTALLY;
			if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[1] |= TMEF_EMERALDCHECK;
			break;
		case 3: //Zoom tube parameters
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_MIDSOLID);
			break;
		case 4: //Speed pad parameters
			CONS_Alert(CONS_WARNING, "Speed Pad linedef is deprecated. Use the TERRAIN effect!\n");
			break;
		case 7: //Sector flat alignment
			lines[i].args[0] = tag;
			if ((lines[i].flags & (ML_NETONLY|ML_NONET)) == (ML_NETONLY|ML_NONET))
			{
				CONS_Alert(CONS_WARNING, M_GetText("Flat alignment linedef (tag %d) doesn't have anything to do.\nConsider changing the linedef's flag configuration or removing it entirely.\n"), tag);
				lines[i].special = 0;
			}
			else if (lines[i].flags & ML_NETONLY)
				lines[i].args[1] = TMP_CEILING;
			else if (lines[i].flags & ML_NONET)
				lines[i].args[1] = TMP_FLOOR;
			else
				lines[i].args[1] = TMP_BOTH;
			lines[i].flags &= ~(ML_NETONLY|ML_NONET);

			if (lines[i].flags & ML_BLOCKMONSTERS) // Set offset through x and y texture offsets
			{
				angle_t flatangle = InvAngle(R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y));
				fixed_t xoffs = sides[lines[i].sidenum[0]].textureoffset;
				fixed_t yoffs = sides[lines[i].sidenum[0]].rowoffset;

				//If no tag is given, apply to front sector
				if (lines[i].args[0] == 0)
					P_ApplyFlatAlignment(lines[i].frontsector, flatangle, xoffs, yoffs, lines[i].args[1] != TMP_CEILING, lines[i].args[1] != TMP_FLOOR);
				else
				{
					INT32 s;
					TAG_ITER_SECTORS(lines[i].args[0], s)
						P_ApplyFlatAlignment(sectors + s, flatangle, xoffs, yoffs, lines[i].args[1] != TMP_CEILING, lines[i].args[1] != TMP_FLOOR);
				}
				lines[i].special = 0;
			}
			break;
		case 8: //Special sector properties
		{
			INT32 s;

			lines[i].args[0] = tag;
			TAG_ITER_SECTORS(tag, s)
			{
				if (lines[i].flags & ML_NOCLIMB)
				{
					sectors[s].flags = static_cast<sectorflags_t>(sectors[s].flags & ~MSF_FLIPSPECIAL_FLOOR);
					sectors[s].flags = static_cast<sectorflags_t>(sectors[s].flags | MSF_FLIPSPECIAL_CEILING);
				}
				else if (lines[i].flags & ML_MIDSOLID)
					sectors[s].flags = static_cast<sectorflags_t>(sectors[s].flags | MSF_FLIPSPECIAL_BOTH);

				if (lines[i].flags & ML_MIDPEG)
					sectors[s].flags = static_cast<sectorflags_t>(sectors[s].flags | MSF_TRIGGERSPECIAL_TOUCH);
				if (lines[i].flags & ML_NOSKEW)
					sectors[s].flags = static_cast<sectorflags_t>(sectors[s].flags | MSF_TRIGGERSPECIAL_HEADBUMP);

				if (lines[i].flags & ML_SKEWTD)
					sectors[s].flags = static_cast<sectorflags_t>(sectors[s].flags | MSF_INVERTPRECIP);

				if (lines[i].flags & ML_DONTPEGTOP)
					sectors[s].flags = static_cast<sectorflags_t>(sectors[s].flags | MSF_RIPPLE_FLOOR);
				if (lines[i].flags & ML_DONTPEGBOTTOM)
					sectors[s].flags = static_cast<sectorflags_t>(sectors[s].flags | MSF_RIPPLE_CEILING);
			}

			if (GETSECSPECIAL(lines[i].frontsector->special, 4) != 12)
				lines[i].special = 0;

			break;
		}
		case 10: //Culling plane
			lines[i].args[0] = tag;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 11: //Rope hang parameters
			lines[i].args[0] = (lines[i].flags & ML_NOCLIMB) ? 0 : sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_SKEWTD);
			break;
		case 13: //Heat wave effect
		{
			INT32 s;

			TAG_ITER_SECTORS(tag, s)
				sectors[s].flags = static_cast<sectorflags_t>(sectors[s].flags | MSF_HEATWAVE);

			break;
		}
		case 14: //Bustable block parameters
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_SKEWTD);
			break;
		case 16: //Minecart parameters
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			break;
		case 20: //PolyObject first line
		{
			INT32 check = -1;
			INT32 paramline = -1;

			TAG_ITER_LINES(tag, check)
			{
				if (lines[check].special == 22)
				{
					paramline = check;
					break;
				}
			}

			//PolyObject ID
			lines[i].args[0] = tag;

			//Default: Invisible planes
			lines[i].args[3] |= TMPF_INVISIBLEPLANES;

			//Linedef executor tag
			lines[i].args[4] = 32000 + lines[i].args[0];

			if (paramline == -1)
				break; // no extra settings to apply, let's leave it

			//Parent ID
			lines[i].args[1] = lines[paramline].frontsector->special;
			//Translucency
			lines[i].args[2] = (lines[paramline].flags & ML_DONTPEGTOP)
						? (sides[lines[paramline].sidenum[0]].textureoffset >> FRACBITS)
						: ((lines[paramline].frontsector->floorheight >> FRACBITS) / 100);

			//Flags
			if (lines[paramline].flags & ML_SKEWTD)
				lines[i].args[3] |= TMPF_NOINSIDES;
			if (lines[paramline].flags & ML_NOSKEW)
				lines[i].args[3] |= TMPF_INTANGIBLE;
			if (lines[paramline].flags & ML_MIDPEG)
				lines[i].args[3] |= TMPF_PUSHABLESTOP;
			if (lines[paramline].flags & ML_MIDSOLID)
				lines[i].args[3] &= ~TMPF_INVISIBLEPLANES;
			/*if (lines[paramline].flags & ML_WRAPMIDTEX)
				lines[i].args[3] |= TMPF_DONTCLIPPLANES;*/
			if (lines[paramline].flags & ML_BLOCKMONSTERS)
				lines[i].args[3] |= TMPF_SPLAT;
			if (lines[paramline].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMPF_EXECUTOR;

			break;
		}
		case 30: //Polyobject - waving flag
			lines[i].args[0] = tag;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[2] = AngleFixed(lines[i].angle) >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			break;
		case 31: //Polyobject - displacement by front sector
			lines[i].args[0] = tag;
			lines[i].args[1] = 0;
			lines[i].args[2] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
			lines[i].args[3] = R_PointToAngle2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
			break;
		case 32: //Polyobject - angular displacement by front sector
			lines[i].args[0] = tag;
			lines[i].args[1] = 0;
			lines[i].args[2] = sides[lines[i].sidenum[0]].textureoffset ? sides[lines[i].sidenum[0]].textureoffset >> FRACBITS : 128;
			lines[i].args[3] = sides[lines[i].sidenum[0]].rowoffset ? sides[lines[i].sidenum[0]].rowoffset >> FRACBITS : 90;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[4] |= TMPR_DONTROTATEOTHERS;
			else if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[4] |= TMPR_ROTATEPLAYERS;
			break;
		case 50: //Instantly lower floor on level load
		case 51: //Instantly raise ceiling on level load
			lines[i].args[0] = tag;
			break;
		case 52: //Continuously falling sector
			lines[i].args[0] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 53: //Continuous floor/ceiling mover
		case 54: //Continuous floor mover
		case 55: //Continuous ceiling mover
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 53) ? TMP_BOTH : lines[i].special - 54;
			lines[i].args[2] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[3] = lines[i].args[2];
			lines[i].args[4] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[5] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].special = 53;
			break;
		case 56: //Continuous two-speed floor/ceiling mover
		case 57: //Continuous two-speed floor mover
		case 58: //Continuous two-speed ceiling mover
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 56) ? TMP_BOTH : lines[i].special - 57;
			lines[i].args[2] = abs(lines[i].dx) >> FRACBITS;
			lines[i].args[3] = abs(lines[i].dy) >> FRACBITS;
			lines[i].args[4] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[5] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].special = 56;
			break;
		case 59: //Activate moving platform
		case 60: //Activate moving platform (adjustable speed)
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 60) ? P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS : 8;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[4] = (lines[i].flags & ML_NOCLIMB) ? 1 : 0;
			lines[i].special = 60;
			break;
		case 61: //Crusher (Ceiling to floor)
		case 62: //Crusher (Floor to ceiling)
			lines[i].args[0] = tag;
			lines[i].args[1] = lines[i].special - 61;
			if (lines[i].flags & ML_MIDSOLID)
			{
				lines[i].args[2] = abs(lines[i].dx) >> FRACBITS;
				lines[i].args[3] = lines[i].args[2];
			}
			else
			{
				lines[i].args[2] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> (FRACBITS + 1);
				lines[i].args[3] = lines[i].args[2] / 4;
			}
			lines[i].special = 61;
			break;
		case 63: //Fake floor/ceiling planes
			lines[i].args[0] = tag;
			break;
		case 64: //Appearing/disappearing FOF
			lines[i].args[0] = (lines[i].flags & ML_BLOCKPLAYERS) ? 0 : tag;
			lines[i].args[1] = (lines[i].flags & ML_BLOCKPLAYERS) ? tag : Tag_FGet(&lines[i].frontsector->tags);
			lines[i].args[2] = lines[i].dx >> FRACBITS;
			lines[i].args[3] = lines[i].dy >> FRACBITS;
			lines[i].args[4] = lines[i].frontsector->floorheight >> FRACBITS;
			lines[i].args[5] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 66: //Move floor by displacement
		case 67: //Move ceiling by displacement
		case 68: //Move floor and ceiling by displacement
			lines[i].args[0] = tag;
			lines[i].args[1] = lines[i].special - 66;
			lines[i].args[2] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] *= -1;
			lines[i].special = 66;
			break;
		case 76: //Make FOF bouncy
			lines[i].args[0] = tag;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			break;
		case 80: //Raise tagged things by type to this FOF
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = tag;
			break;
		case 81: //Block enemies
			lines[i].flags |= ML_BLOCKMONSTERS;
			lines[i].special = 0;
			break;
		case 100: //FOF: solid, opaque, shadowcasting
		case 101: //FOF: solid, opaque, non-shadowcasting
		case 102: //FOF: solid, translucent
		case 103: //FOF: solid, sides only
		case 104: //FOF: solid, no sides
		case 105: //FOF: solid, invisible
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special == 102)
			{
				if (lines[i].flags & ML_NOCLIMB)
					lines[i].args[3] |= TMFA_INSIDES;
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= TMFA_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			//Appearance
			if (lines[i].special == 105)
				lines[i].args[3] |= TMFA_NOPLANES|TMFA_NOSIDES;
			else if (lines[i].special == 104)
				lines[i].args[3] |= TMFA_NOSIDES;
			else if (lines[i].special == 103)
				lines[i].args[3] |= TMFA_NOPLANES;
			if (lines[i].special != 100 && (lines[i].special != 104 || !(lines[i].flags & ML_NOCLIMB)))
				lines[i].args[3] |= TMFA_NOSHADE;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[3] |= TMFA_SPLAT;

			//Tangibility
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] |= TMFT_DONTBLOCKOTHERS;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[4] |= TMFT_DONTBLOCKPLAYER;

			lines[i].special = 100;
			break;
		case 120: //FOF: water, opaque
		case 121: //FOF: water, translucent
		case 122: //FOF: water, opaque, no sides
		case 123: //FOF: water, translucent, no sides
		case 124: //FOF: goo water, translucent
		case 125: //FOF: goo water, translucent, no sides
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special == 120 || lines[i].special == 122)
				lines[i].args[1] = 255;
			else
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= TMFW_SPLAT;
			}

			//No sides?
			if (lines[i].special == 122 || lines[i].special == 123 || lines[i].special == 125)
				lines[i].args[3] |= TMFW_NOSIDES;

			//Flags
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMFW_DOUBLESHADOW;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[3] |= TMFW_COLORMAPONLY;
			if (!(lines[i].flags & ML_WRAPMIDTEX))
				lines[i].args[3] |= TMFW_NORIPPLE;

			//Goo?
			if (lines[i].special >= 124)
				lines[i].args[3] |= TMFW_GOOWATER;

			//Splat rendering?
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[3] |= TMFW_SPLAT;

			lines[i].special = 120;
			break;
		case 140: //FOF: intangible from bottom, opaque
		case 141: //FOF: intangible from bottom, translucent
		case 142: //FOF: intangible from bottom, translucent, no sides
		case 143: //FOF: intangible from top, opaque
		case 144: //FOF: intangible from top, translucent
		case 145: //FOF: intangible from top, translucent, no sides
		case 146: //FOF: only tangible from sides
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special == 141 || lines[i].special == 142 || lines[i].special == 144 || lines[i].special == 145)
			{
				if (lines[i].flags & ML_NOCLIMB)
					lines[i].args[3] |= TMFA_INSIDES;
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= TMFA_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			//Appearance
			if (lines[i].special == 142 || lines[i].special == 145)
				lines[i].args[3] |= TMFA_NOSIDES;
			else if (lines[i].special == 146)
				lines[i].args[3] |= TMFA_NOPLANES;
			if (lines[i].special != 146 && (lines[i].flags & ML_NOCLIMB))
				lines[i].args[3] |= TMFA_NOSHADE;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[3] |= TMFA_SPLAT;

			//Tangibility
			if (lines[i].special <= 142)
				lines[i].args[4] |= TMFT_INTANGIBLEBOTTOM;
			else if (lines[i].special <= 145)
				lines[i].args[4] |= TMFT_INTANGIBLETOP;
			else
				lines[i].args[4] |= TMFT_INTANGIBLEBOTTOM|TMFT_INTANGIBLETOP;

			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] |= TMFT_DONTBLOCKOTHERS;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[4] |= TMFT_DONTBLOCKPLAYER;

			lines[i].special = 100;
			break;
		case 150: //FOF: Air bobbing
		case 151: //FOF: Air bobbing (adjustable)
		case 152: //FOF: Reverse air bobbing (adjustable)
		case 153: //FOF: Dynamically sinking platform
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 150) ? 16 : (P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS);

			//Flags
			if (lines[i].special == 152)
				lines[i].args[2] |= TMFB_REVERSE;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] |= TMFB_SPINDASH;
			if (lines[i].special == 153)
				lines[i].args[2] |= TMFB_DYNAMIC;

			lines[i].special = 150;
			break;
		case 160: //FOF: Water bobbing
			lines[i].args[0] = tag;
			break;
		case 170: //FOF: Crumbling, respawn
		case 171: //FOF: Crumbling, no respawn
		case 172: //FOF: Crumbling, respawn, intangible from bottom
		case 173: //FOF: Crumbling, no respawn, intangible from bottom
		case 174: //FOF: Crumbling, respawn, intangible from bottom, translucent
		case 175: //FOF: Crumbling, no respawn, intangible from bottom, translucent
		case 176: //FOF: Crumbling, respawn, floating, bobbing
		case 177: //FOF: Crumbling, no respawn, floating, bobbing
		case 178: //FOF: Crumbling, respawn, floating
		case 179: //FOF: Crumbling, no respawn, floating
		case 180: //FOF: Crumbling, respawn, air bobbing
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special >= 174 && lines[i].special <= 175)
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[4] |= TMFC_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			if (lines[i].special >= 172 && lines[i].special <= 175)
			{
				lines[i].args[3] |= TMFT_INTANGIBLEBOTTOM;
				if (lines[i].flags & ML_NOCLIMB)
					lines[i].args[4] |= TMFC_NOSHADE;
			}

			if (lines[i].special % 2 == 1)
				lines[i].args[4] |= TMFC_NORETURN;
			if (lines[i].special == 176 || lines[i].special == 177 || lines[i].special == 180)
				lines[i].args[4] |= TMFC_AIRBOB;
			if (lines[i].special >= 176 && lines[i].special <= 179)
				lines[i].args[4] |= TMFC_FLOATBOB;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[4] |= TMFC_SPLAT;

			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[3] |= TMFT_DONTBLOCKOTHERS;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[3] |= TMFT_DONTBLOCKPLAYER;

			lines[i].special = 170;
			break;
		case 190: // FOF: Rising, solid, opaque, shadowcasting
		case 191: // FOF: Rising, solid, opaque, non-shadowcasting
		case 192: // FOF: Rising, solid, translucent
		case 193: // FOF: Rising, solid, invisible
		case 194: // FOF: Rising, intangible from bottom, opaque
		case 195: // FOF: Rising, intangible from bottom, translucent
			lines[i].args[0] = tag;

			//Translucency
			if (lines[i].special == 192 || lines[i].special == 195)
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= TMFA_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			//Appearance
			if (lines[i].special == 193)
				lines[i].args[3] |= TMFA_NOPLANES|TMFA_NOSIDES;
			if (lines[i].special >= 194)
				lines[i].args[3] |= TMFA_INSIDES;
			if (lines[i].special != 190 && (lines[i].special <= 193 || lines[i].flags & ML_NOCLIMB))
				lines[i].args[3] |= TMFA_NOSHADE;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[3] |= TMFA_SPLAT;

			//Tangibility
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] |= TMFT_DONTBLOCKOTHERS;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[4] |= TMFT_DONTBLOCKPLAYER;
			if (lines[i].special >= 194)
				lines[i].args[4] |= TMFT_INTANGIBLEBOTTOM;

			//Speed
			lines[i].args[5] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;

			//Flags
			if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[6] |= TMFR_REVERSE;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[6] |= TMFR_SPINDASH;

			lines[i].special = 190;
			break;
		case 200: //FOF: Light block
		case 201: //FOF: Half light block
			lines[i].args[0] = tag;
			if (lines[i].special == 201)
				lines[i].args[1] = 1;
			lines[i].special = 200;
			break;
		case 202: //FOF: Fog block
		case 223: //FOF: Intangible, invisible
			lines[i].args[0] = tag;
			break;
		case 220: //FOF: Intangible, opaque
		case 221: //FOF: Intangible, translucent
		case 222: //FOF: Intangible, sides only
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special == 221)
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= TMFA_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			//Appearance
			if (lines[i].special == 222)
				lines[i].args[3] |= TMFA_NOPLANES;
			if (lines[i].special == 221)
				lines[i].args[3] |= TMFA_INSIDES;
			if (lines[i].special != 220 && !(lines[i].flags & ML_NOCLIMB))
				lines[i].args[3] |= TMFA_NOSHADE;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[3] |= TMFA_SPLAT;

			lines[i].special = 220;
			break;
		case 250: //FOF: Mario block
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] |= TMFM_BRICK;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[1] |= TMFM_INVISIBLE;
			break;
		case 251: //FOF: Thwomp block
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_WRAPMIDTEX) //Custom speeds
			{
				lines[i].args[1] = lines[i].dy >> FRACBITS;
				lines[i].args[2] = lines[i].dx >> FRACBITS;
			}
			else
			{
				lines[i].args[1] = 80;
				lines[i].args[2] = 16;
			}
			if (lines[i].flags & ML_MIDSOLID)
				P_WriteSfx(sides[lines[i].sidenum[0]].textureoffset >> FRACBITS, &lines[i].stringargs[0]);
			if (lines[i].flags & ML_SKEWTD) // Kart Z delay. Yes, it used the same field as the above.
				lines[i].args[3] = (unsigned)(sides[lines[i].sidenum[0]].textureoffset >> FRACBITS);
			break;
		case 252: //FOF: Shatter block
		case 253: //FOF: Shatter block, translucent
		case 254: //FOF: Bustable block
		case 255: //FOF: Spin-bustable block
		case 256: //FOF: Spin-bustable block, translucent
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special == 253 || lines[i].special == 256)
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[4] |= TMFB_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			//Bustable type
			if (lines[i].special <= 253)
				lines[i].args[3] = TMFB_TOUCH;
			else if (lines[i].special >= 255)
				lines[i].args[3] = TMFB_SPIN;
			else if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] = TMFB_STRONG;
			else
				lines[i].args[3] = TMFB_REGULAR;

			//Flags
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[4] |= TMFB_PUSHABLES;
			if (lines[i].flags & ML_WRAPMIDTEX)
			{
				lines[i].args[4] |= TMFB_EXECUTOR;
				lines[i].args[5] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			}
			if (lines[i].special == 252 && lines[i].flags & ML_NOCLIMB)
				lines[i].args[4] |= TMFB_ONLYBOTTOM;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[4] |= TMFB_SPLAT;

			lines[i].special = 254;
			break;
		case 257: //FOF: Quicksand
			lines[i].args[0] = tag;
			if (!(lines[i].flags & ML_WRAPMIDTEX))
				lines[i].args[1] = 1; //No ripple effect
			lines[i].args[2] = lines[i].dx >> FRACBITS; //Sinking speed
			lines[i].args[3] = lines[i].dy >> FRACBITS; //Friction
			break;
		case 258: //FOF: Laser
			lines[i].args[0] = tag;

			//Alpha
			P_SetBinaryFOFAlpha(&lines[i]);

			//Flags
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[3] |= TMFL_NOBOSSES;
			//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
			if (lines[i].flags & ML_BLOCKMONSTERS || lines[i].args[1] == 256)
				lines[i].args[3] |= TMFL_SPLAT;

			break;
		case 259: //Custom FOF
			if (lines[i].sidenum[1] == 0xffff)
				I_Error("Custom FOF (tag %d) found without a linedef back side!", tag);

			lines[i].args[0] = tag;
			lines[i].args[3] = P_GetFOFFlags(sides[lines[i].sidenum[1]].toptexture);
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[3] |= FOF_SPLAT;
			lines[i].args[4] = P_GetFOFBusttype(sides[lines[i].sidenum[1]].toptexture);
			if (sides[lines[i].sidenum[1]].toptexture & FF_OLD_SHATTERBOTTOM)
				lines[i].args[4] |= TMFB_ONLYBOTTOM;
			if (lines[i].args[3] & FOF_TRANSLUCENT)
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= FOF_SPLAT;
			}
			else
				lines[i].args[1] = 255;
			break;
		case 300: //Trigger linedef executor - Continuous
		case 301: //Trigger linedef executor - Each time
		case 302: //Trigger linedef executor - Once
			if (lines[i].special == 302)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 301)
				lines[i].args[0] = (lines[i].flags & ML_NOTBOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].special = 300;
			break;
		case 303: //Ring count - Continuous
		case 304: //Ring count - Once
			lines[i].args[0] = (lines[i].special == 304) ? TMT_ONCE : TMT_CONTINUOUS;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] = TMC_LTE;
			else if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[2] = TMC_GTE;
			else
				lines[i].args[2] = TMC_EQUAL;
			lines[i].args[3] = !!(lines[i].flags & ML_MIDSOLID);
			lines[i].special = 303;
			break;
		case 305: //Character ability - Continuous
		case 306: //Character ability - Each time
		case 307: //Character ability - Once
			if (lines[i].special == 307)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 306)
				lines[i].args[0] = (lines[i].flags & ML_NOTBOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = (P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS) / 10;
			lines[i].special = 305;
			break;
		case 308: //Race only - once
			lines[i].args[0] = TMT_ONCE;
			lines[i].args[1] = GTR_CIRCUIT;
			lines[i].args[2] = TMF_HASANY;
			break;
		case 309: //CTF red team - continuous
		case 310: //CTF red team - each time
		case 311: //CTF blue team - continuous
		case 312: //CTF blue team - each time
			if (lines[i].special % 2 == 0)
				lines[i].args[0] = (lines[i].flags & ML_NOTBOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = (lines[i].special > 310) ? TMT_BLUE : TMT_ORANGE;
			lines[i].special = 309;
			break;
		case 313: //No more enemies - once
			lines[i].args[0] = tag;
			break;
		case 314: //Number of pushables - Continuous
		case 315: //Number of pushables - Once
			lines[i].args[0] = (lines[i].special == 315) ? TMT_ONCE : TMT_CONTINUOUS;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] = TMC_GTE;
			else if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[2] = TMC_LTE;
			else
				lines[i].args[2] = TMC_EQUAL;
			lines[i].special = 314;
			break;
		case 317: //Condition set trigger - Continuous
		case 318: //Condition set trigger - Once
			lines[i].args[0] = (lines[i].special == 318) ? TMT_ONCE : TMT_CONTINUOUS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].special = 317;
			break;
		case 319: //Unlockable trigger - Continuous
		case 320: //Unlockable trigger - Once
			lines[i].args[0] = (lines[i].special == 320) ? TMT_ONCE : TMT_CONTINUOUS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].special = 319;
			break;
		case 321: //Trigger after X calls - Continuous
		case 322: //Trigger after X calls - Each time
			if (lines[i].special % 2 == 0)
				lines[i].args[0] = (lines[i].flags & ML_NOTBOUNCY) ? TMXT_EACHTIMEENTERANDEXIT : TMXT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMXT_CONTINUOUS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
			{
				lines[i].args[2] = 1;
				lines[i].args[3] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			}
			else
				lines[i].args[2] = lines[i].args[3] = 0;
			lines[i].special = 321;
			break;
		case 323: //NiGHTSerize - Each time
		case 324: //NiGHTSerize - Once
		case 325: //DeNiGHTSerize - Each time
		case 326: //DeNiGHTSerize - Once
		case 327: //NiGHTS lap - Each time
		case 328: //NiGHTS lap - Once
		case 329: //Ideya capture touch - Each time
		case 330: //Ideya capture touch - Once
			lines[i].args[0] = (lines[i].special + 1) % 2;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] = TMC_LTE;
			else if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[3] = TMC_GTE;
			else
				lines[i].args[3] = TMC_EQUAL;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] = TMC_LTE;
			else if (lines[i].flags & ML_NOSKEW)
				lines[i].args[4] = TMC_GTE;
			else
				lines[i].args[4] = TMC_EQUAL;
			if (lines[i].flags & ML_DONTPEGBOTTOM)
				lines[i].args[5] = TMNP_SLOWEST;
			else if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[5] = TMNP_TRIGGERER;
			else
				lines[i].args[5] = TMNP_FASTEST;
			if (lines[i].special % 2 == 0)
				lines[i].special--;
			if (lines[i].special == 323)
			{
				if (lines[i].flags & ML_TFERLINE)
					lines[i].args[6] = TMN_FROMNONIGHTS;
				else if (lines[i].flags & ML_DONTPEGTOP)
					lines[i].args[6] = TMN_FROMNIGHTS;
				else
					lines[i].args[6] = TMN_ALWAYS;

				if (lines[i].flags & ML_MIDPEG)
					lines[i].args[7] |= TMN_BONUSLAPS;
				if (lines[i].flags & ML_NOTBOUNCY)
					lines[i].args[7] |= TMN_LEVELCOMPLETION;
			}
			else if (lines[i].special == 325)
			{
				if (lines[i].flags & ML_TFERLINE)
					lines[i].args[6] = TMD_NOBODYNIGHTS;
				else if (lines[i].flags & ML_DONTPEGTOP)
					lines[i].args[6] = TMD_SOMEBODYNIGHTS;
				else
					lines[i].args[6] = TMD_ALWAYS;

				lines[i].args[7] = !!(lines[i].flags & ML_MIDPEG);
			}
			else if (lines[i].special == 327)
				lines[i].args[6] = !!(lines[i].flags & ML_MIDPEG);
			else
			{
				if (lines[i].flags & ML_DONTPEGTOP)
					lines[i].args[6] = TMS_ALWAYS;
				else if (lines[i].flags & ML_NOTBOUNCY)
					lines[i].args[6] = TMS_IFNOTENOUGH;
				else
					lines[i].args[6] = TMS_IFENOUGH;

				if (lines[i].flags & ML_MIDPEG)
					lines[i].args[7] |= TMI_BONUSLAPS;
				if (lines[i].flags & ML_TFERLINE)
					lines[i].args[7] |= TMI_ENTER;
			}
			break;
		case 331: // Player skin - continuous
		case 332: // Player skin - each time
		case 333: // Player skin - once
			if (lines[i].special == 303)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 302)
				lines[i].args[0] = (lines[i].flags & ML_NOTBOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			lines[i].special = 331;
			break;
		case 334: // Object dye - continuous
		case 335: // Object dye - each time
		case 336: // Object dye - once
			if (lines[i].special == 336)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 335)
				lines[i].args[0] = (lines[i].flags & ML_NOTBOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			lines[i].special = 334;
			break;
		case 337: //Emerald check - continuous
		case 338: //Emerald check - each time
		case 339: //Emerald check - once
			if (lines[i].special == 339)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 338)
				lines[i].args[0] = (lines[i].flags & ML_NOTBOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = EMERALD_ALLCHAOS;
			lines[i].args[2] = TMF_HASALL;
			lines[i].special = 337;
			break;
		case 340: //NiGHTS mare - continuous
		case 341: //NiGHTS mare - each time
		case 342: //NiGHTS mare - once
			if (lines[i].special == 342)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 341)
				lines[i].args[0] = (lines[i].flags & ML_NOTBOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] = TMC_LTE;
			else if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[2] = TMC_GTE;
			else
				lines[i].args[2] = TMC_EQUAL;
			lines[i].special = 340;
			break;
		case 400: //Copy tagged sector's floor height/texture
		case 401: //Copy tagged sector's ceiling height/texture
			lines[i].args[0] = 0;
			lines[i].args[1] = tag;
			lines[i].args[2] = lines[i].special - 400;
			lines[i].args[3] = !(lines[i].flags & ML_NOCLIMB);
			lines[i].special = 400;
			break;
		case 402: //Copy light level
			lines[i].args[0] = 0;
			lines[i].args[1] = tag;
			lines[i].args[2] = 0;
			break;
		case 403: //Copy-move tagged sector's floor height/texture
		case 404: //Copy-move tagged sector's ceiling height/texture
			lines[i].args[0] = 0;
			lines[i].args[1] = tag;
			lines[i].args[2] = (lines[i].special == 403) ? TMP_FLOOR : TMP_CEILING;
			lines[i].args[3] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[4] = (lines[i].flags & ML_BLOCKPLAYERS) ? sides[lines[i].sidenum[0]].textureoffset >> FRACBITS : 0;
			lines[i].args[5] = !!(lines[i].flags & ML_NOCLIMB);
			lines[i].special = 403;
			break;
		case 405: //Move floor according to front texture offsets
		case 407: //Move ceiling according to front texture offsets
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 405) ? TMP_FLOOR : TMP_CEILING;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[4] = !!(lines[i].flags & ML_NOCLIMB);
			lines[i].special = 405;
			break;
		case 408: //Copy flats
			lines[i].args[0] = 0;
			lines[i].args[1] = tag;
			if ((lines[i].flags & (ML_NOCLIMB|ML_MIDSOLID)) == (ML_NOCLIMB|ML_MIDSOLID))
			{
				CONS_Alert(CONS_WARNING, M_GetText("Copy flats linedef (tag %d) doesn't have anything to do.\nConsider changing the linedef's flag configuration or removing it entirely.\n"), tag);
				lines[i].special = 0;
			}
			else if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] = TMP_CEILING;
			else if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[2] = TMP_FLOOR;
			else
				lines[i].args[2] = TMP_BOTH;
			break;
		case 409: //Change tagged sector's tag
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] = TMT_ADD;
			else if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[2] = TMT_REMOVE;
			else
				lines[i].args[2] = TMT_REPLACEFIRST;
			break;
		case 410: //Change front sector's tag
			lines[i].args[0] = 0;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] = TMT_ADD;
			else if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[2] = TMT_REMOVE;
			else
				lines[i].args[2] = TMT_REPLACEFIRST;
			break;
		case 411: //Stop plane movement
			lines[i].args[0] = tag;
			break;
		case 412: //Teleporter
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[1] |= TMT_SILENT;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] |= TMT_KEEPANGLE;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[1] |= TMT_KEEPMOMENTUM;
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[1] |= TMT_RELATIVE;
			lines[i].args[2] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[4] = lines[i].frontsector->ceilingheight >> FRACBITS;
			break;
		case 414: //Play sound effect
			lines[i].args[3] = tag;
			if (tag != 0)
			{
				if (lines[i].flags & ML_WRAPMIDTEX)
				{
					lines[i].args[1] = TMSS_TAGGEDSECTOR;
					lines[i].args[2] = TMSL_EVERYONE;
				}
				else
				{
					lines[i].args[1] = TMSS_NOWHERE;
					lines[i].args[2] = TMSL_TAGGEDSECTOR;
				}
			}
			else
			{
				if (lines[i].flags & ML_NOCLIMB)
				{
					lines[i].args[1] = TMSS_NOWHERE;
					lines[i].args[2] = TMSL_TRIGGERER;
				}
				else if (lines[i].flags & ML_MIDSOLID)
				{
					lines[i].args[1] = TMSS_NOWHERE;
					lines[i].args[2] = TMSL_EVERYONE;
				}
				else if (lines[i].flags & ML_BLOCKPLAYERS)
				{
					lines[i].args[1] = TMSS_TRIGGERSECTOR;
					lines[i].args[2] = TMSL_EVERYONE;
				}
				else
				{
					lines[i].args[1] = TMSS_TRIGGERMOBJ;
					lines[i].args[2] = TMSL_EVERYONE;
				}
			}
			break;
		case 416: //Start adjustable flickering light
		case 417: //Start adjustable pulsating light
		case 602: //Adjustable pulsating light
		case 603: //Adjustable flickering light
			lines[i].args[0] = tag;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[2] = lines[i].frontsector->lightlevel;
			if ((lines[i].flags & ML_NOCLIMB) && lines[i].backsector)
				lines[i].args[4] = lines[i].backsector->lightlevel;
			else
				lines[i].args[3] = 1;
			break;
		case 418: //Start adjustable blinking light (unsynchronized)
		case 419: //Start adjustable blinking light (synchronized)
		case 604: //Adjustable blinking light (unsynchronized)
		case 605: //Adjustable blinking light (synchronized)
			lines[i].args[0] = tag;
			lines[i].args[1] = abs(lines[i].dx) >> FRACBITS;
			lines[i].args[2] = abs(lines[i].dy) >> FRACBITS;
			lines[i].args[3] = lines[i].frontsector->lightlevel;
			if ((lines[i].flags & ML_NOCLIMB) && lines[i].backsector)
				lines[i].args[5] = lines[i].backsector->lightlevel;
			else
				lines[i].args[4] |= TMB_USETARGET;
			if (lines[i].special % 2 == 1)
			{
				lines[i].args[4] |= TMB_SYNC;
				lines[i].special--;
			}
			break;
		case 420: //Fade light level
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_DONTPEGBOTTOM)
			{
				lines[i].args[1] = std::max<fixed_t>(sides[lines[i].sidenum[0]].textureoffset >> FRACBITS, 0);
				// failsafe: if user specifies Back Y Offset and NOT Front Y Offset, use the Back Offset
				// to be consistent with other light and fade specials
				lines[i].args[2] = ((lines[i].sidenum[1] != 0xFFFF && !(sides[lines[i].sidenum[0]].rowoffset >> FRACBITS)) ?
					std::max<fixed_t>(std::min<fixed_t>(sides[lines[i].sidenum[1]].rowoffset >> FRACBITS, 255), 0)
					: std::max<fixed_t>(std::min<fixed_t>(sides[lines[i].sidenum[0]].rowoffset >> FRACBITS, 255), 0));
			}
			else
			{
				lines[i].args[1] = lines[i].frontsector->lightlevel;
				lines[i].args[2] = abs(P_AproxDistance(lines[i].dx, lines[i].dy)) >> FRACBITS;
			}
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[3] |= TMF_TICBASED;
			if (lines[i].flags & ML_WRAPMIDTEX)
				lines[i].args[3] |= TMF_OVERRIDE;
			break;
		case 421: //Stop lighting effect
			lines[i].args[0] = tag;
			break;
		case 422: //Switch to cut-away view
			lines[i].args[0] = tag;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[2] = (lines[i].flags & ML_NOCLIMB) ? sides[lines[i].sidenum[0]].textureoffset >> FRACBITS : 0;
			break;
		case 423: //Change sky
		case 424: //Change weather
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 426: //Stop object
			lines[i].args[0] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 427: //Award score
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			break;
		case 428: //Start platform movement
			lines[i].args[0] = tag;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[4] = (lines[i].flags & ML_NOCLIMB) ? 1 : 0;
			break;
		case 429: //Crush ceiling once
		case 430: //Crush floor once
		case 431: //Crush floor and ceiling once
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 429) ? TMP_CEILING : ((lines[i].special == 430) ? TMP_FLOOR : TMP_BOTH);
			if (lines[i].special == 430 || lines[i].flags & ML_MIDSOLID)
			{
				lines[i].args[2] = abs(lines[i].dx) >> FRACBITS;
				lines[i].args[3] = lines[i].args[2];
			}
			else
			{
				lines[i].args[2] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> (FRACBITS + 1);
				lines[i].args[3] = lines[i].args[2] / 4;
			}
			lines[i].special = 429;
			break;
		case 432: //Enable/disable 2D mode
			lines[i].args[0] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 433: //Enable/disable gravity flip
			lines[i].args[0] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 435: //Change plane scroller direction
			lines[i].args[0] = tag;
			lines[i].args[1] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
			lines[i].args[2] = AngleFixed(R_PointToAngle2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y)) >> FRACBITS;
			break;
		case 436: //Shatter FOF
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			break;
		case 437: //Disable player control
			lines[i].args[0] = ((sides[lines[i].sidenum[0]].textureoffset >> FRACBITS) != 0);
			break;
		case 438: //Change object size
			lines[i].args[0] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			break;
		case 439: //Change tagged linedef's textures
			lines[i].args[0] = 0;
			lines[i].args[1] = tag;
			lines[i].args[2] = TMSD_FRONTBACK;
			lines[i].args[3] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 441: //Condition set trigger
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			break;
		case 442: //Change object type state
			lines[i].args[2] = tag;
			lines[i].args[3] = (lines[i].sidenum[1] == 0xffff) ? 1 : 0;
			break;
		case 444: //Earthquake
			lines[i].args[0] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			break;
		case 445: //Make FOF disappear/reappear
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 446: //Make FOF crumble
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] |= TMFR_NORETURN;
			if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[2] |= TMFR_CHECKFLAG;
			break;
		case 447: //Change colormap
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[2] |= TMCF_RELATIVE;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[2] |= TMCF_SUBLIGHTR|TMCF_SUBFADER;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] |= TMCF_SUBLIGHTG|TMCF_SUBFADEG;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[2] |= TMCF_SUBLIGHTB|TMCF_SUBFADEB;
			break;
		case 448: //Change skybox
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if ((lines[i].flags & (ML_MIDSOLID|ML_BLOCKPLAYERS)) == ML_MIDSOLID) // Solid Midtexture is on but Block Enemies is off?
			{
				CONS_Alert(CONS_WARNING,
					M_GetText("Skybox switch linedef (tag %d) doesn't have anything to do.\nConsider changing the linedef's flag configuration or removing it entirely.\n"),
					tag);
				lines[i].special = 0;
				break;
			}
			else if ((lines[i].flags & (ML_MIDSOLID|ML_BLOCKPLAYERS)) == (ML_MIDSOLID|ML_BLOCKPLAYERS))
				lines[i].args[2] = TMS_CENTERPOINT;
			else if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[2] = TMS_BOTH;
			else
				lines[i].args[2] = TMS_VIEWPOINT;
			lines[i].args[3] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 449: //Enable bosses with parameters
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 450: //Execute linedef executor (specific tag)
			lines[i].args[0] = tag;
			break;
		case 451: //Execute linedef executor (random tag in range)
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			break;
		case 452: //Set FOF translucency
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = lines[i].sidenum[1] != 0xffff ? (sides[lines[i].sidenum[1]].textureoffset >> FRACBITS) : (P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS);
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[3] |= TMST_RELATIVE;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMST_DONTDOTRANSLUCENT;
			break;
		case 453: //Fade FOF
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = lines[i].sidenum[1] != 0xffff ? (sides[lines[i].sidenum[1]].textureoffset >> FRACBITS) : (lines[i].dx >> FRACBITS);
			lines[i].args[3] = lines[i].sidenum[1] != 0xffff ? (sides[lines[i].sidenum[1]].rowoffset >> FRACBITS) : (abs(lines[i].dy) >> FRACBITS);
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[4] |= TMFT_RELATIVE;
			if (lines[i].flags & ML_WRAPMIDTEX)
				lines[i].args[4] |= TMFT_OVERRIDE;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[4] |= TMFT_TICBASED;
			if (lines[i].flags & ML_NOTBOUNCY)
				lines[i].args[4] |= TMFT_IGNORECOLLISION;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] |= TMFT_GHOSTFADE;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[4] |= TMFT_DONTDOTRANSLUCENT;
			if (lines[i].flags & ML_BLOCKPLAYERS)
				lines[i].args[4] |= TMFT_DONTDOEXISTS;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[4] |= (TMFT_DONTDOLIGHTING|TMFT_DONTDOCOLORMAP);
			if (lines[i].flags & ML_TFERLINE)
				lines[i].args[4] |= TMFT_USEEXACTALPHA;
			break;
		case 454: //Stop fading FOF
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_BLOCKPLAYERS);
			break;
		case 455: //Fade colormap
		{
			INT32 speed = (INT32)((((lines[i].flags & ML_DONTPEGBOTTOM) || !sides[lines[i].sidenum[0]].rowoffset) && lines[i].sidenum[1] != 0xFFFF) ?
				abs(sides[lines[i].sidenum[1]].rowoffset >> FRACBITS)
				: abs(sides[lines[i].sidenum[0]].rowoffset >> FRACBITS));

			lines[i].args[0] = tag;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[2] = speed;
			else
				lines[i].args[2] = (256 + speed - 1)/speed;
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[3] |= TMCF_RELATIVE;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[3] |= TMCF_SUBLIGHTR|TMCF_SUBFADER;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMCF_SUBLIGHTG|TMCF_SUBFADEG;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[3] |= TMCF_SUBLIGHTB|TMCF_SUBFADEB;
			if (lines[i].flags & ML_NOTBOUNCY)
				lines[i].args[3] |= TMCF_FROMBLACK;
			if (lines[i].flags & ML_WRAPMIDTEX)
				lines[i].args[3] |= TMCF_OVERRIDE;
			break;
		}
		case 456: //Stop fading colormap
			lines[i].args[0] = tag;
			break;
		case 457: //Track object's angle
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[3] = (lines[i].sidenum[1] != 0xffff) ? sides[lines[i].sidenum[1]].textureoffset >> FRACBITS : 0;
			lines[i].args[4] = !!(lines[i].flags & ML_NOSKEW);
			break;
		case 460: //Award rings
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 461: //Spawn object
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[3] = lines[i].frontsector->floorheight >> FRACBITS;
			lines[i].args[4] = (lines[i].flags & ML_SKEWTD) ? AngleFixed(R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y)) >> FRACBITS : 0;
			if (lines[i].flags & ML_NOCLIMB)
			{
				if (lines[i].sidenum[1] != 0xffff) // Make sure the linedef has a back side
				{
					lines[i].args[5] = 1;
					lines[i].args[6] = sides[lines[i].sidenum[1]].textureoffset >> FRACBITS;
					lines[i].args[7] = sides[lines[i].sidenum[1]].rowoffset >> FRACBITS;
					lines[i].args[8] = lines[i].frontsector->ceilingheight >> FRACBITS;
				}
				else
				{
					CONS_Alert(CONS_WARNING, "Linedef Type %d - Spawn Object: Linedef is set for random range but has no back side.\n", lines[i].special);
					lines[i].args[5] = 0;
				}
			}
			else
				lines[i].args[5] = 0;
			break;
		case 464: //Trigger egg capsule
			lines[i].args[0] = tag;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 466: //Set level failure state
			lines[i].args[0] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 467: //Set light level
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = TML_SECTOR;
			lines[i].args[3] = !!(lines[i].flags & ML_MIDPEG);
			break;
		case 480: //Polyobject - door slide
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = AngleFixed(R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y)) >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].sidenum[1] != 0xffff)
				lines[i].args[4] = sides[lines[i].sidenum[1]].textureoffset >> FRACBITS;
			break;
		case 481: //Polyobject - door swing
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].sidenum[1] != 0xffff)
				lines[i].args[3] = sides[lines[i].sidenum[1]].textureoffset >> FRACBITS;
			break;
		case 482: //Polyobject - move
		case 483: //Polyobject - move, override
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = AngleFixed(R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y)) >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[4] = lines[i].special == 483;
			lines[i].special = 482;
			break;
		case 484: //Polyobject - rotate right
		case 485: //Polyobject - rotate right, override
		case 486: //Polyobject - rotate left
		case 487: //Polyobject - rotate left, override
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].args[2] == 360)
				lines[i].args[3] |= TMPR_CONTINUOUS;
			else if (lines[i].args[2] == 0)
				lines[i].args[2] = 360;
			if (lines[i].special < 486)
				lines[i].args[2] *= -1;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMPR_DONTROTATEOTHERS;
			else if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[3] |= TMPR_ROTATEPLAYERS;
			if (lines[i].special % 2 == 1)
				lines[i].args[3] |= TMPR_OVERRIDE;
			lines[i].special = 484;
			break;
		case 488: //Polyobject - move by waypoints
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[3] = PWR_WRAP;
			else if (lines[i].flags & ML_NOSKEW)
				lines[i].args[3] = PWR_COMEBACK;
			else
				lines[i].args[3] = PWR_STOP;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] |= PWF_REVERSE;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[4] |= PWF_LOOP;
			break;
		case 489: //Polyobject - turn invisible, intangible
		case 490: //Polyobject - turn visible, tangible
			lines[i].args[0] = tag;
			lines[i].args[1] = 491 - lines[i].special;
			if (!(lines[i].flags & ML_NOCLIMB))
				lines[i].args[2] = lines[i].args[1];
			lines[i].special = 489;
			break;
		case 491: //Polyobject - set translucency
			lines[i].args[0] = tag;
			// If Front X Offset is specified, use that. Else, use floorheight.
			lines[i].args[1] = (sides[lines[i].sidenum[0]].textureoffset ? sides[lines[i].sidenum[0]].textureoffset : lines[i].frontsector->floorheight) >> FRACBITS;
			// If DONTPEGBOTTOM, specify raw translucency value. Else, take it out of 1000.
			if (!(lines[i].flags & ML_DONTPEGBOTTOM))
				lines[i].args[1] /= 100;
			lines[i].args[2] = !!(lines[i].flags & ML_MIDPEG);
			break;
		case 492: //Polyobject - fade translucency
			lines[i].args[0] = tag;
			// If Front X Offset is specified, use that. Else, use floorheight.
			lines[i].args[1] = (sides[lines[i].sidenum[0]].textureoffset ? sides[lines[i].sidenum[0]].textureoffset : lines[i].frontsector->floorheight) >> FRACBITS;
			// If DONTPEGBOTTOM, specify raw translucency value. Else, take it out of 1000.
			if (!(lines[i].flags & ML_DONTPEGBOTTOM))
				lines[i].args[1] /= 100;
			// allow Back Y Offset to be consistent with other fade specials
			lines[i].args[2] = (lines[i].sidenum[1] != 0xffff && !sides[lines[i].sidenum[0]].rowoffset) ?
				abs(sides[lines[i].sidenum[1]].rowoffset >> FRACBITS)
				: abs(sides[lines[i].sidenum[0]].rowoffset >> FRACBITS);
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[3] |= TMPF_RELATIVE;
			if (lines[i].flags & ML_WRAPMIDTEX)
				lines[i].args[3] |= TMPF_OVERRIDE;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[3] |= TMPF_TICBASED;
			if (lines[i].flags & ML_NOTBOUNCY)
				lines[i].args[3] |= TMPF_IGNORECOLLISION;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[3] |= TMPF_GHOSTFADE;
			break;
		case 499: //Ring Racers - Toggle waypoints
			lines[i].args[0] = tag;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 500: //Scroll front wall left
		case 501: //Scroll front wall right
			lines[i].args[0] = 0;
			lines[i].args[1] = ((lines[i].special == 500) ? -1 : 1) * (1 << SCROLL_SHIFT);
			lines[i].args[2] = 0;
			lines[i].special = 500;
			break;
		case 502: //Scroll tagged wall
		case 503: //Scroll tagged wall (accelerative)
		case 504: //Scroll tagged wall (displacement)
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_MIDPEG)
			{
				if (lines[i].sidenum[1] == 0xffff)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line special %d (line #%s) missing back side!\n", lines[i].special, sizeu1(i));
					lines[i].special = 0;
					break;
				}
				lines[i].args[1] = 1;
			}
			else
				lines[i].args[1] = 0;
			if (lines[i].flags & ML_NOSKEW)
			{
				lines[i].args[2] = sides[lines[i].sidenum[0]].textureoffset >> (FRACBITS - SCROLL_SHIFT);
				lines[i].args[3] = sides[lines[i].sidenum[0]].rowoffset >> (FRACBITS - SCROLL_SHIFT);
			}
			else
			{
				lines[i].args[2] = lines[i].dx >> FRACBITS;
				lines[i].args[3] = lines[i].dy >> FRACBITS;
			}
			lines[i].args[4] = lines[i].special - 502;
			lines[i].special = 502;
			break;
		case 505: //Scroll front wall by front side offsets
		case 506: //Scroll front wall by back side offsets
		case 507: //Scroll back wall by front side offsets
		case 508: //Scroll back wall by back side offsets
			lines[i].args[0] = lines[i].special >= 507;
			if (lines[i].special % 2 == 0)
			{
				if (lines[i].sidenum[1] == 0xffff)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line special %d (line #%s) missing back side!\n", lines[i].special, sizeu1(i));
					lines[i].special = 0;
					break;
				}
				lines[i].args[1] = sides[lines[i].sidenum[1]].textureoffset >> FRACBITS;
				lines[i].args[2] = sides[lines[i].sidenum[1]].rowoffset >> FRACBITS;
			}
			else
			{
				lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
				lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			}
			lines[i].special = 500;
			break;
		case 510: //Scroll floor texture
		case 511: //Scroll floor texture (accelerative)
		case 512: //Scroll floor texture (displacement)
		case 513: //Scroll ceiling texture
		case 514: //Scroll ceiling texture (accelerative)
		case 515: //Scroll ceiling texture (displacement)
		case 520: //Carry objects on floor
		case 521: //Carry objects on floor (accelerative)
		case 522: //Carry objects on floor (displacement)
		case 523: //Carry objects on ceiling
		case 524: //Carry objects on ceiling (accelerative)
		case 525: //Carry objects on ceiling (displacement)
		case 530: //Scroll floor texture and carry objects
		case 531: //Scroll floor texture and carry objects (accelerative)
		case 532: //Scroll floor texture and carry objects (displacement)
		case 533: //Scroll ceiling texture and carry objects
		case 534: //Scroll ceiling texture and carry objects (accelerative)
		case 535: //Scroll ceiling texture and carry objects (displacement)
			lines[i].args[0] = tag;
			lines[i].args[1] = ((lines[i].special % 10) < 3) ? TMP_FLOOR : TMP_CEILING;
			lines[i].args[2] = ((lines[i].special - 510)/10 + 1) % 3;
			lines[i].args[3] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
			lines[i].args[4] = (lines[i].special % 10) % 3;
			if (lines[i].args[2] != TMS_SCROLLONLY && !(lines[i].flags & ML_NOCLIMB))
				lines[i].args[4] |= TMST_NONEXCLUSIVE;
			lines[i].special = 510;
			break;
		case 540: //Floor friction
		{
			INT32 s;
			fixed_t strength; // friction value of sector
			fixed_t friction; // friction value to be applied during movement

			strength = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (strength > 0) // sludge
				strength = strength*2; // otherwise, the maximum sludginess value is +967...

			// The following might seem odd. At the time of movement,
			// the move distance is multiplied by 'friction/0x10000', so a
			// higher friction value actually means 'less friction'.
			friction = ORIG_FRICTION - (0x1EB8*strength)/0x80; // ORIG_FRICTION is 0xE800

			TAG_ITER_SECTORS(tag, s)
				sectors[s].friction = friction;
			break;
		}
		case 541: //Wind
		case 542: //Upwards wind
		case 543: //Downwards wind
		case 544: //Current
		case 545: //Upwards current
		case 546: //Downwards current
			lines[i].args[0] = tag;
			switch ((lines[i].special - 541) % 3)
			{
				case 0:
					lines[i].args[1] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
					break;
				case 1:
					lines[i].args[2] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
					break;
				case 2:
					lines[i].args[2] = -R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
					break;
			}
			lines[i].args[3] = (lines[i].special >= 544) ? p_current : p_wind;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[4] |= TMPF_SLIDE;
			if (!(lines[i].flags & ML_NOCLIMB))
				lines[i].args[4] |= TMPF_NONEXCLUSIVE;
			lines[i].special = 541;
			break;
		case 600: //Floor lighting
		case 601: //Ceiling lighting
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 601) ? TMP_CEILING : TMP_FLOOR;
			lines[i].special = 600;
			break;
		case 606: //Colormap
			lines[i].args[0] = tag;
			break;
		case 700: //Slope front sector floor
		case 701: //Slope front sector ceiling
		case 702: //Slope front sector floor and ceiling
		case 703: //Slope front sector floor and back sector ceiling
		case 710: //Slope back sector floor
		case 711: //Slope back sector ceiling
		case 712: //Slope back sector floor and ceiling
		case 713: //Slope back sector floor and front sector ceiling
		{
			boolean frontfloor = (lines[i].special == 700 || lines[i].special == 702 || lines[i].special == 703);
			boolean backfloor = (lines[i].special == 710 || lines[i].special == 712 || lines[i].special == 713);
			boolean frontceil = (lines[i].special == 701 || lines[i].special == 702 || lines[i].special == 713);
			boolean backceil = (lines[i].special == 711 || lines[i].special == 712 || lines[i].special == 703);

			lines[i].args[0] = backfloor ? TMS_BACK : (frontfloor ? TMS_FRONT : TMS_NONE);
			lines[i].args[1] = backceil ? TMS_BACK : (frontceil ? TMS_FRONT : TMS_NONE);

			if (lines[i].flags & ML_NETONLY)
				lines[i].args[2] |= TMSL_NOPHYSICS;
			if (lines[i].flags & ML_NONET)
				lines[i].args[2] |= TMSL_DYNAMIC;
			if (lines[i].flags & ML_TFERLINE)
				lines[i].args[2] |= TMSL_COPY;

			lines[i].special = 700;
			break;
		}
		case 704: //Slope front sector floor by 3 tagged vertices
		case 705: //Slope front sector ceiling by 3 tagged vertices
		case 714: //Slope back sector floor by 3 tagged vertices
		case 715: //Slope back sector ceiling  by 3 tagged vertices
		{
			if (lines[i].special == 704)
				lines[i].args[0] = TMSP_FRONTFLOOR;
			else if (lines[i].special == 705)
				lines[i].args[0] = TMSP_FRONTCEILING;
			else if (lines[i].special == 714)
				lines[i].args[0] = TMSP_BACKFLOOR;
			else if (lines[i].special == 715)
				lines[i].args[0] = TMSP_BACKCEILING;

			lines[i].args[1] = tag;

			if (lines[i].flags & ML_BLOCKMONSTERS)
			{
				UINT8 side = lines[i].special >= 714;

				if (side == 1 && lines[i].sidenum[1] == 0xffff)
					CONS_Debug(DBG_GAMELOGIC, "P_ConvertBinaryMap: Line special %d (line #%s) missing 2nd side!\n", lines[i].special, sizeu1(i));
				else
				{
					lines[i].args[2] = sides[lines[i].sidenum[side]].textureoffset >> FRACBITS;
					lines[i].args[3] = sides[lines[i].sidenum[side]].rowoffset >> FRACBITS;
				}
			}
			else
			{
				lines[i].args[2] = lines[i].args[1];
				lines[i].args[3] = lines[i].args[1];
			}

			if (lines[i].flags & ML_NETONLY)
				lines[i].args[4] |= TMSL_NOPHYSICS;
			if (lines[i].flags & ML_NONET)
				lines[i].args[4] |= TMSL_DYNAMIC;

			lines[i].special = 704;
			break;
		}
		case 720: //Copy front side floor slope
		case 721: //Copy front side ceiling slope
		case 722: //Copy front side floor and ceiling slope
			if (lines[i].special != 721)
				lines[i].args[0] = tag;
			if (lines[i].special != 720)
				lines[i].args[1] = tag;
			lines[i].special = 720;
			break;
		case 723: //Copy back side floor slope
		case 724: //Copy back side ceiling slope
		case 725: //Copy back side floor and ceiling slope
			if (lines[i].special != 724)
				lines[i].args[2] = tag;
			if (lines[i].special != 723)
				lines[i].args[3] = tag;
			lines[i].special = 720;
			break;
		case 730: //Copy front side floor slope to back side
		case 731: //Copy front side ceiling slope to back side
		case 732: //Copy front side floor and ceiling slope to back side
			if (lines[i].special != 731)
				lines[i].args[4] |= TMSC_FRONTTOBACKFLOOR;
			if (lines[i].special != 730)
				lines[i].args[4] |= TMSC_FRONTTOBACKCEILING;
			lines[i].special = 720;
			break;
		case 733: //Copy back side floor slope to front side
		case 734: //Copy back side ceiling slope to front side
		case 735: //Copy back side floor and ceiling slope to front side
			if (lines[i].special != 734)
				lines[i].args[4] |= TMSC_BACKTOFRONTFLOOR;
			if (lines[i].special != 733)
				lines[i].args[4] |= TMSC_BACKTOFRONTCEILING;
			lines[i].special = 720;
			break;
		case 799: //Set dynamic slope vertex to front sector height
			lines[i].args[0] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case LT_SLOPE_ANCHORS_OLD_FLOOR: //Slope front sector floor by 3 tagged vertices
		case LT_SLOPE_ANCHORS_OLD_CEILING: //Slope front sector ceiling by 3 tagged vertices
		case LT_SLOPE_ANCHORS_OLD: //Slope back sector floor by 3 tagged vertices
		{
			if (lines[i].special == LT_SLOPE_ANCHORS_OLD_FLOOR)
				lines[i].args[0] = TMSA_FLOOR;
			else if (lines[i].special == LT_SLOPE_ANCHORS_OLD_CEILING)
				lines[i].args[0] = TMSA_CEILING;
			else if (lines[i].special == LT_SLOPE_ANCHORS_OLD)
				lines[i].args[0] = (TMSA_FLOOR|TMSA_CEILING);

			if (lines[i].flags & ML_NETONLY)
				lines[i].args[1] |= TMSAF_NOPHYSICS;
			if (lines[i].flags & ML_NONET)
				lines[i].args[1] |= TMSAF_DYNAMIC;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] |= TMSAF_BACKSIDE;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[1] |= TMSAF_MIRROR;

			lines[i].args[2] = tag;

			lines[i].special = LT_SLOPE_ANCHORS;
			break;
		}
		case 909: //Fog wall
			lines[i].blendmode = AST_FOG;
			break;
		case 2001: //Finish line
			lines[i].activation |= (SPAC_CROSS|SPAC_REPEATSPECIAL);
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[0] |= TMCFF_FLIP;
			break;
		case 2003: //Respawn line
			lines[i].activation |= (SPAC_CROSS|SPAC_REPEATSPECIAL);
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[0] |= TMCRF_FRONTONLY;
			break;
		case 2004: //Bot controller
			lines[i].args[0] = sides[lines[i].sidenum[0]].rowoffset / FRACUNIT;

			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] |= TMBOT_NORUBBERBAND;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[1] |= TMBOT_NOCONTROL;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[1] |= TMBOT_FORCEDIR;

			lines[i].args[2] = sides[lines[i].sidenum[0]].textureoffset / FRACUNIT;
			break;
		default:
			break;
		}

		// Set alpha for translucent walls
		if (lines[i].special >= 900 && lines[i].special < 909)
			lines[i].alpha = ((909 - lines[i].special) << FRACBITS)/10;

		// Set alpha for additive/subtractive/reverse subtractive walls
		if (lines[i].special >= 910 && lines[i].special <= 939)
			lines[i].alpha = ((10 - lines[i].special % 10) << FRACBITS)/10;

		if (lines[i].special >= 910 && lines[i].special <= 919) // additive
			lines[i].blendmode = AST_ADD;

		if (lines[i].special >= 920 && lines[i].special <= 929) // subtractive
			lines[i].blendmode = AST_SUBTRACT;

		if (lines[i].special >= 930 && lines[i].special <= 939) // reverse subtractive
			lines[i].blendmode = AST_REVERSESUBTRACT;

		if (lines[i].special == 940) // modulate
			lines[i].blendmode = AST_MODULATE;

		//Linedef executor delay
		if (lines[i].special >= 400 && lines[i].special < 500)
		{
			//Dummy value to indicate that this executor is delayed.
			//The real value is taken from the back sector at runtime.
			if (lines[i].flags & ML_DONTPEGTOP)
				lines[i].executordelay = 1;
		}
	}
}

static void P_ConvertBinarySectorTypes(void)
{
	size_t i;

	for (i = 0; i < numsectors; i++)
	{
		mtag_t tag = Tag_FGet(&sectors[i].tags);

		switch(GETSECSPECIAL(sectors[i].special, 1))
		{
			case 1: //Damage
				sectors[i].damagetype = SD_GENERIC;
				break;
			case 2: //Offroad (Weak)
				CONS_Alert(CONS_WARNING, "Offroad specials will be deprecated soon. Use the TERRAIN effect!\n");
				sectors[i].offroad = FRACUNIT;
				break;
			case 3: //Offroad
				CONS_Alert(CONS_WARNING, "Offroad specials will be deprecated soon. Use the TERRAIN effect!\n");
				sectors[i].offroad = 2*FRACUNIT;
				break;
			case 4: //Offroad (Strong)
				CONS_Alert(CONS_WARNING, "Offroad specials will be deprecated soon. Use the TERRAIN effect!\n");
				sectors[i].offroad = 3*FRACUNIT;
				break;
			case 5: //Spikes
				sectors[i].damagetype = SD_GENERIC;
				break;
			case 6: //Death pit (camera tilt)
			case 7: //Death pit (no camera tilt)
				sectors[i].damagetype = SD_DEATHPIT;
				break;
			case 8: //Instakill
				sectors[i].damagetype = SD_INSTAKILL;
				break;
			//case 9: -- Ring Drainer (Floor Touch)
			//case 10: -- Ring Drainer (No Floor Touch)
			case 11: // Stumble
				sectors[i].damagetype = SD_STUMBLE;
				break;
			case 12: //Wall sector
				sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_NOSTEPUP);
				break;
			case 13: //Ramp sector
				sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_DOUBLESTEPUP);
				break;
			case 14: //Non-ramp sector
				sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_NOSTEPDOWN);
				break;
			default:
				break;
		}

		switch(GETSECSPECIAL(sectors[i].special, 2))
		{
			case 1: //Trigger linedef executor (pushable objects)
				sectors[i].triggertag = tag;
				sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_TRIGGERLINE_PLANE);
				sectors[i].triggerer = TO_MOBJ;
				break;
			case 2: //Trigger linedef executor (Anywhere in sector, all players)
				sectors[i].triggertag = tag;
				sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags & ~MSF_TRIGGERLINE_PLANE);
				sectors[i].triggerer = TO_ALLPLAYERS;
				break;
			case 3: //Trigger linedef executor (Floor touch, all players)
				sectors[i].triggertag = tag;
				sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_TRIGGERLINE_PLANE);
				sectors[i].triggerer = TO_ALLPLAYERS;
				break;
			case 4: //Trigger linedef executor (Anywhere in sector)
				sectors[i].triggertag = tag;
				sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags & ~MSF_TRIGGERLINE_PLANE);
				sectors[i].triggerer = TO_PLAYER;
				break;
			case 5: //Trigger linedef executor (Floor touch)
				sectors[i].triggertag = tag;
				sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_TRIGGERLINE_PLANE);
				sectors[i].triggerer = TO_PLAYER;
				break;
			case 8: //Check for linedef executor on FOFs
				sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_TRIGGERLINE_MOBJ);
				break;
			case 15: //Invert Encore
				sectors[i].flags = static_cast<sectorflags_t>(sectors[i].flags | MSF_INVERTENCORE);
				break;
			default:
				break;
		}

		switch(GETSECSPECIAL(sectors[i].special, 3))
		{
			case 1: //Trick panel
			case 3:
				CONS_Alert(CONS_WARNING, "Trick Panel special is deprecated. Use the TERRAIN effect!\n");
				break;
			case 5: //Speed pad
				CONS_Alert(CONS_WARNING, "Speed Pad special is deprecated. Use the TERRAIN effect!\n");
				break;
			default:
				break;
		}

		switch(GETSECSPECIAL(sectors[i].special, 4))
		{
			case 1: //Cheat Check activator
				sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_CHEATCHECKACTIVATOR);
				break;
			case 2: //Exit
				sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_EXIT);
				break;
			case 5: //Fan sector
				sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_FAN);
				break;
			case 6: //Sneaker panel
				CONS_Alert(CONS_WARNING, "Sneaker Panel special is deprecated. Use the TERRAIN effect!\n");
				break;
			case 7: //Destroy items
				sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_DELETEITEMS);
				break;
			case 8: //Zoom tube start
				sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_ZOOMTUBESTART);
				break;
			case 9: //Zoom tube end
				sectors[i].specialflags = static_cast<sectorspecialflags_t>(sectors[i].specialflags | SSF_ZOOMTUBEEND);
				break;
			default:
				break;
		}
	}
}

static void P_ConvertBinaryThingTypes(void)
{
	size_t i;
	mobjtype_t mobjtypeofthing[4096] {};
	mobjtype_t mobjtype;

	for (i = 0; i < NUMMOBJTYPES; i++)
	{
		if (mobjinfo[i].doomednum < 0 || mobjinfo[i].doomednum >= 4096)
			continue;

		mobjtypeofthing[mobjinfo[i].doomednum] = (mobjtype_t)i;
	}

	for (i = 0; i < nummapthings; i++)
	{
		mobjtype = mobjtypeofthing[mapthings[i].type];
		if (mobjtype)
		{
			if (mobjinfo[mobjtype].flags & MF_BOSS)
			{
				INT32 paramoffset = mapthings[i].extrainfo*LE_PARAMWIDTH;
				mapthings[i].thing_args[0] = mapthings[i].extrainfo;
				mapthings[i].thing_args[1] = !!(mapthings[i].options & MTF_OBJECTSPECIAL);
				mapthings[i].thing_args[2] = LE_BOSSDEAD + paramoffset;
				mapthings[i].thing_args[3] = LE_ALLBOSSESDEAD + paramoffset;
				mapthings[i].thing_args[4] = LE_PINCHPHASE + paramoffset;
			}
			if (mobjinfo[mobjtype].flags & MF_PUSHABLE)
			{
				if ((mapthings[i].options & (MTF_OBJECTSPECIAL|MTF_AMBUSH)) == (MTF_OBJECTSPECIAL|MTF_AMBUSH))
					mapthings[i].thing_args[0] = TMP_CLASSIC;
				else if (mapthings[i].options & MTF_OBJECTSPECIAL)
					mapthings[i].thing_args[0] = TMP_SLIDE;
				else if (mapthings[i].options & MTF_AMBUSH)
					mapthings[i].thing_args[0] = TMP_IMMOVABLE;
				else
					mapthings[i].thing_args[0] = TMP_NORMAL;
			}
			if (K_IsDuelItem(mobjtype) == true)
			{
				mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_EXTRA);
			}
		}

		if (mapthings[i].type >= 1 && mapthings[i].type <= 35) //Player starts
		{
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			continue;
		}
		else if (mapthings[i].type >= 2200 && mapthings[i].type <= 2217) //Flickies
		{
			mapthings[i].thing_args[0] = mapthings[i].angle;
			if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].thing_args[1] |= TMFF_AIMLESS;
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[1] |= TMFF_STATIONARY;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].thing_args[1] |= TMFF_HOP;
			if (mapthings[i].type == 2207)
				mapthings[i].thing_args[2] = mapthings[i].extrainfo;
			continue;
		}

		switch (mapthings[i].type)
		{
		case 102: //SDURF
		case 1805: //Puma
			mapthings[i].thing_args[0] = mapthings[i].angle;
			break;
		case 110: //THZ Turret
			mapthings[i].thing_args[0] = LE_TURRET;
			break;
		case 111: //Pop-up Turret
			mapthings[i].thing_args[0] = mapthings[i].angle;
			break;
		case 103: //Buzz (Gold)
		case 104: //Buzz (Red)
		case 105: //Jetty-syn Bomber
		case 106: //Jetty-syn Gunner
		case 117: //Robo-Hood
		case 126: //Crushstacean
		case 128: //Bumblebore
		case 132: //Cacolantern
		case 138: //Banpyura
		case 1602: //Pian
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 119: //Egg Guard
			if ((mapthings[i].options & (MTF_EXTRA|MTF_OBJECTSPECIAL)) == MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[0] = TMGD_LEFT;
			else if ((mapthings[i].options & (MTF_EXTRA|MTF_OBJECTSPECIAL)) == MTF_EXTRA)
				mapthings[i].thing_args[0] = TMGD_RIGHT;
			else
				mapthings[i].thing_args[0] = TMGD_BACK;
			mapthings[i].thing_args[1] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 127: //Hive Elemental
			mapthings[i].thing_args[0] = mapthings[i].extrainfo;
			break;
		case 135: //Pterabyte Spawner
			mapthings[i].thing_args[0] = mapthings[i].extrainfo + 1;
			break;
		case 136: //Pyre Fly
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 201: //Egg Slimer
			mapthings[i].thing_args[5] = !(mapthings[i].options & MTF_AMBUSH);
			break;
		case 203: //Egg Colosseum
			mapthings[i].thing_args[5] = LE_BOSS4DROP + mapthings[i].extrainfo * LE_PARAMWIDTH;
			break;
		case 204: //Fang
			mapthings[i].thing_args[4] = LE_BOSS4DROP + mapthings[i].extrainfo*LE_PARAMWIDTH;
			if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].thing_args[5] |= TMF_GRAYSCALE;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].thing_args[5] |= TMF_SKIPINTRO;
			break;
		case 206: //Brak Eggman (Old)
			mapthings[i].thing_args[5] = LE_BRAKPLATFORM + mapthings[i].extrainfo*LE_PARAMWIDTH;
			break;
		case 207: //Metal Sonic (Race)
		case 2104: //Amy Cameo
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_EXTRA);
			break;
		case 208: //Metal Sonic (Battle)
			mapthings[i].thing_args[5] = !!(mapthings[i].options & MTF_EXTRA);
			break;
		case 209: //Brak Eggman
			mapthings[i].thing_args[5] = LE_BRAKVILEATACK + mapthings[i].extrainfo*LE_PARAMWIDTH;
			if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].thing_args[6] |= TMB_NODEATHFLING;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].thing_args[6] |= TMB_BARRIER;
			break;
		case 292: //Boss waypoint
			mapthings[i].thing_args[0] = mapthings[i].angle;
			mapthings[i].thing_args[1] = mapthings[i].options & 7;
			break;
		case 294: //Fang waypoint
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 300: //Ring
		case 301: //Bounce ring
		case 302: //Rail ring
		case 303: //Infinity ring
		case 304: //Automatic ring
		case 305: //Explosion ring
		case 306: //Scatter ring
		case 307: //Grenade ring
		case 308: //Red team ring
		case 309: //Blue team ring
		case 312: //Emerald token
		case 320: //Emerald hunt location
		case 321: //Match chaos emerald spawn
		case 322: //Emblem
		case 330: //Bounce ring panel
		case 331: //Rail ring panel
		case 332: //Automatic ring panel
		case 333: //Explosion ring panel
		case 334: //Scatter ring panel
		case 335: //Grenade ring panel
		case 520: //Bomb sphere
		case 521: //Spikeball
		case 1706: //Blue sphere
		case 1800: //Coin
			mapthings[i].thing_args[0] = !(mapthings[i].options & MTF_AMBUSH);
			break;
		case 409: //Extra life monitor
			mapthings[i].thing_args[2] = !(mapthings[i].options & (MTF_AMBUSH|MTF_OBJECTSPECIAL));
			break;
		case 500: //Air bubble patch
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 502: //Cheat Check
			if (mapthings[i].extrainfo)
				// Allow thing Parameter to define star post num too!
				// For cheatchecks above param 15 (the 16th), add 360 to the angle like before and start parameter from 1 (NOT 0)!
				// So the 16th cheatcheck is angle=0 param=15, the 17th would be angle=360 param=1.
				// This seems more intuitive for mappers to use, since most SP maps won't have over 16 consecutive star posts.
				mapthings[i].thing_args[0] = mapthings[i].extrainfo + (mapthings[i].angle/360) * 15;
			else
				// Old behavior if Parameter is 0; add 360 to the angle for each consecutive star post.
				mapthings[i].thing_args[0] = (mapthings[i].angle/360);
			mapthings[i].thing_args[1] = !!(mapthings[i].options & MTF_OBJECTSPECIAL);
			break;
		case 522: //Wall spike
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
			{
				mapthings[i].thing_args[0] = mobjinfo[MT_WALLSPIKE].speed + mapthings[i].angle/360;
				mapthings[i].thing_args[1] = (16 - mapthings[i].extrainfo) * mapthings[i].thing_args[0]/16;
				if (mapthings[i].options & MTF_EXTRA)
					mapthings[i].thing_args[2] |= TMSF_RETRACTED;
			}
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].thing_args[2] |= TMSF_INTANGIBLE;
			break;
		case 523: //Spike
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
			{
				mapthings[i].thing_args[0] = mobjinfo[MT_SPIKE].speed + mapthings[i].angle;
				mapthings[i].thing_args[1] = (16 - mapthings[i].extrainfo) * mapthings[i].thing_args[0]/16;
				if (mapthings[i].options & MTF_EXTRA)
					mapthings[i].thing_args[2] |= TMSF_RETRACTED;
			}
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].thing_args[2] |= TMSF_INTANGIBLE;
			break;
		case 540: //Fan
			mapthings[i].thing_args[0] = mapthings[i].angle;
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[1] |= TMF_INVISIBLE;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].thing_args[1] |= TMF_NODISTANCECHECK;
			break;
		case 541: //Gas jet
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			mapthings[i].thing_args[1] = !!(mapthings[i].options & MTF_OBJECTSPECIAL);
			break;
		case 543: //Balloon
			if (mapthings[i].angle > 0)
			{
				P_WriteSkincolor(((mapthings[i].angle - 1) % (numskincolors - 1)) + 1, &mapthings[i].thing_stringargs[0]);
			}
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 555: //Diagonal yellow spring
		case 556: //Diagonal red spring
		case 557: //Diagonal blue spring
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[0] |= TMDS_NOGRAVITY;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].thing_args[0] |= TMDS_ROTATEEXTRA;
			break;
		case 558: //Horizontal yellow spring
		case 559: //Horizontal red spring
		case 560: //Horizontal blue spring
			mapthings[i].thing_args[0] = !(mapthings[i].options & MTF_AMBUSH);
			break;
		case 700: //Water ambience A
		case 701: //Water ambience B
		case 702: //Water ambience C
		case 703: //Water ambience D
		case 704: //Water ambience E
		case 705: //Water ambience F
		case 706: //Water ambience G
		case 707: //Water ambience H
			mapthings[i].thing_args[0] = 35;
			P_WriteSfx(sfx_amwtr1 + mapthings[i].type - 700, &mapthings[i].thing_stringargs[0]);
			mapthings[i].type = 700;
			break;
		case 708: //Disco ambience
			mapthings[i].thing_args[0] = 512;
			P_WriteSfx(sfx_ambint, &mapthings[i].thing_stringargs[0]);
			mapthings[i].type = 700;
			break;
		case 709: //Volcano ambience
			mapthings[i].thing_args[0] = 220;
			P_WriteSfx(sfx_ambin2, &mapthings[i].thing_stringargs[0]);
			mapthings[i].type = 700;
			break;
		case 710: //Machine ambience
			mapthings[i].thing_args[0] = 24;
			P_WriteSfx(sfx_ambmac, &mapthings[i].thing_stringargs[0]);
			mapthings[i].type = 700;
			break;
		case 750: //Slope vertex
			mapthings[i].thing_args[0] = mapthings[i].extrainfo;
			break;
		case 753: //Zoom tube waypoint
			mapthings[i].thing_args[0] = mapthings[i].angle >> 8;
			mapthings[i].thing_args[1] = mapthings[i].angle & 255;
			break;
		case 754: //Push point
		case 755: //Pull point
		{
			subsector_t *ss = R_PointInSubsector(mapthings[i].x << FRACBITS, mapthings[i].y << FRACBITS);
			sector_t *s;
			line_t *line;

			if (!ss)
			{
				CONS_Debug(DBG_GAMELOGIC, "Push/pull point: Placed outside of map bounds!\n");
				break;
			}

			s = ss->sector;
			line = P_FindPointPushLine(&s->tags);

			if (!line)
			{
				CONS_Debug(DBG_GAMELOGIC, "Push/pull point: Unable to find line of type 547 tagged to sector %s!\n", sizeu1((size_t)(s - sectors)));
				break;
			}

			mapthings[i].thing_args[0] = mapthings[i].angle;
			mapthings[i].thing_args[1] = P_AproxDistance(line->dx >> FRACBITS, line->dy >> FRACBITS);
			if (mapthings[i].type == 755)
				mapthings[i].thing_args[1] *= -1;
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[2] |= TMPP_NOZFADE;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].thing_args[2] |= TMPP_PUSHZ;
			if (!(line->flags & ML_NOCLIMB))
				mapthings[i].thing_args[2] |= TMPP_NONEXCLUSIVE;
			mapthings[i].type = 754;
			break;
		}
		case 756: //Blast linedef executor
			mapthings[i].thing_args[0] = mapthings[i].angle;
			break;
		case 757: //Fan particle generator
		{
			INT32 j = Tag_FindLineSpecial(15, mapthings[i].angle);

			if (j == -1)
			{
				CONS_Debug(DBG_GAMELOGIC, "Particle generator (mapthing #%s) needs to be tagged to a #15 parameter line (trying to find tag %d).\n", sizeu1(i), mapthings[i].angle);
				break;
			}
			mapthings[i].thing_args[0] = mapthings[i].z;
			mapthings[i].thing_args[1] = R_PointToDist2(lines[j].v1->x, lines[j].v1->y, lines[j].v2->x, lines[j].v2->y) >> FRACBITS;
			mapthings[i].thing_args[2] = sides[lines[j].sidenum[0]].textureoffset >> FRACBITS;
			mapthings[i].thing_args[3] = sides[lines[j].sidenum[0]].rowoffset >> FRACBITS;
			mapthings[i].thing_args[4] = lines[j].backsector ? sides[lines[j].sidenum[1]].textureoffset >> FRACBITS : 0;
			mapthings[i].thing_args[6] = mapthings[i].angle;
			P_WriteDuplicateText(lines[j].stringargs[0], &mapthings[i].thing_stringargs[0]);
			break;
		}
		case 762: //PolyObject spawn point (crush)
		{
			INT32 check = -1;
			INT32 firstline = -1;

			TAG_ITER_LINES(mapthings[i].tid, check)
			{
				if (lines[check].special == 20)
				{
					firstline = check;
					break;
				}
			}

			if (firstline != -1)
				lines[firstline].args[3] |= TMPF_CRUSH;

			mapthings[i].type = 761;
			break;
		}
		case 780: //Skybox
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_OBJECTSPECIAL);
			break;
		case 799: //Tutorial plant
			mapthings[i].thing_args[0] = mapthings[i].extrainfo;
			break;
		case 1002: //Dripping water
			mapthings[i].thing_args[0] = mapthings[i].angle;
			break;
		case 1007: //Kelp
		case 1008: //Stalagmite (DSZ1)
		case 1011: //Stalagmite (DSZ2)
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_OBJECTSPECIAL);
			break;
		case 1102: //Eggman Statue
			mapthings[i].thing_args[1] = !!(mapthings[i].options & MTF_EXTRA);
			break;
		case 1104: //Mace spawnpoint
		case 1105: //Chain with maces spawnpoint
		case 1106: //Chained spring spawnpoint
		case 1107: //Chain spawnpoint
		case 1109: //Firebar spawnpoint
		case 1110: //Custom mace spawnpoint
		{
			mtag_t tag = (mtag_t)mapthings[i].angle;
			INT32 j = Tag_FindLineSpecial(9, tag);

			if (j == -1)
			{
				CONS_Debug(DBG_GAMELOGIC, "Chain/mace setup: Unable to find parameter line 9 (tag %d)!\n", tag);
				break;
			}

			mapthings[i].angle = lines[j].frontsector->ceilingheight >> FRACBITS;
			mapthings[i].pitch = lines[j].frontsector->floorheight >> FRACBITS;
			mapthings[i].thing_args[0] = lines[j].dx >> FRACBITS;
			mapthings[i].thing_args[1] = mapthings[i].extrainfo;
			mapthings[i].thing_args[3] = lines[j].dy >> FRACBITS;
			mapthings[i].thing_args[4] = sides[lines[j].sidenum[0]].textureoffset >> FRACBITS;
			mapthings[i].thing_args[7] = -sides[lines[j].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[j].backsector)
			{
				mapthings[i].roll = lines[j].backsector->ceilingheight >> FRACBITS;
				mapthings[i].thing_args[2] = sides[lines[j].sidenum[1]].rowoffset >> FRACBITS;
				mapthings[i].thing_args[5] = lines[j].backsector->floorheight >> FRACBITS;
				mapthings[i].thing_args[6] = sides[lines[j].sidenum[1]].textureoffset >> FRACBITS;
			}
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].thing_args[8] |= TMM_DOUBLESIZE;
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[8] |= TMM_SILENT;
			if (lines[j].flags & ML_NOCLIMB)
				mapthings[i].thing_args[8] |= TMM_ALLOWYAWCONTROL;
			if (lines[j].flags & ML_SKEWTD)
				mapthings[i].thing_args[8] |= TMM_SWING;
			if (lines[j].flags & ML_NOSKEW)
				mapthings[i].thing_args[8] |= TMM_MACELINKS;
			if (lines[j].flags & ML_MIDPEG)
				mapthings[i].thing_args[8] |= TMM_CENTERLINK;
			if (lines[j].flags & ML_MIDSOLID)
				mapthings[i].thing_args[8] |= TMM_CLIP;
			if (lines[j].flags & ML_WRAPMIDTEX)
				mapthings[i].thing_args[8] |= TMM_ALWAYSTHINK;
			if (mapthings[i].type == 1110)
			{
				P_WriteDuplicateText(lines[j].stringargs[0], &mapthings[i].thing_stringargs[0]);
				P_WriteDuplicateText(lines[j].stringargs[1], &mapthings[i].thing_stringargs[1]);
			}
			break;
		}
		case 1101: //Torch
		case 1119: //Candle
		case 1120: //Candle pricket
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_EXTRA);
			break;
		case 1121: //Flame holder
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[0] |= TMFH_NOFLAME;
			if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].thing_args[0] |= TMFH_CORONA;
			break;
		case 1127: //Spectator EggRobo
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].thing_args[0] = TMED_LEFT;
			else if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[0] = TMED_RIGHT;
			else
				mapthings[i].thing_args[0] = TMED_NONE;
			break;
		case 1200: //Tumbleweed (Big)
		case 1201: //Tumbleweed (Small)
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1202: //Rock spawner
		{
			mtag_t tag = (mtag_t)mapthings[i].angle;
			INT32 j = Tag_FindLineSpecial(12, tag);

			if (j == -1)
			{
				CONS_Debug(DBG_GAMELOGIC, "Rock spawner: Unable to find parameter line 12 (tag %d)!\n", tag);
				break;
			}
			mapthings[i].angle = AngleFixed(R_PointToAngle2(lines[j].v2->x, lines[j].v2->y, lines[j].v1->x, lines[j].v1->y)) >> FRACBITS;
			mapthings[i].thing_args[0] = P_AproxDistance(lines[j].dx, lines[j].dy) >> FRACBITS;
			mapthings[i].thing_args[1] = sides[lines[j].sidenum[0]].textureoffset >> FRACBITS;
			mapthings[i].thing_args[2] = !!(lines[j].flags & ML_NOCLIMB);
			INT32 id = (sides[lines[j].sidenum[0]].rowoffset >> FRACBITS);
			// Rather than introduce deh_tables.h as a dependency for literally one
			// conversion, we just... recreate the string expected to be produced.
			if (id > 0 && id < 16)
				P_WriteDuplicateText(va("MT_ROCKCRUMBLE%d", id+1), &mapthings[i].thing_stringargs[0]);
			break;
		}
		case 1221: //Minecart saloon door
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1229: //Minecart switch point
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1300: //Flame jet (horizontal)
		case 1301: //Flame jet (vertical)
			mapthings[i].thing_args[0] = (mapthings[i].angle >> 13)*TICRATE/2;
			mapthings[i].thing_args[1] = ((mapthings[i].angle >> 10) & 7)*TICRATE/2;
			mapthings[i].thing_args[2] = 80 - 5*mapthings[i].extrainfo;
			mapthings[i].thing_args[3] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1304: //Lavafall
			mapthings[i].thing_args[0] = mapthings[i].angle;
			mapthings[i].thing_args[1] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1305: //Rollout Rock
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1488: // Follower Audience (unfortunately numbered)
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[2] |= TMAUDIM_FLOAT;
			if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].thing_args[2] |= TMAUDIM_BORED;

			mapthings[i].thing_args[3] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1500: //Glaregoyle
		case 1501: //Glaregoyle (Up)
		case 1502: //Glaregoyle (Down)
		case 1503: //Glaregoyle (Long)
			if (mapthings[i].angle >= 360)
				mapthings[i].thing_args[1] = 7*(mapthings[i].angle/360) + 1;
			break;
		case 1700: //Axis
			mapthings[i].thing_args[2] = mapthings[i].angle & 16383;
			mapthings[i].thing_args[3] = !!(mapthings[i].angle & 16384);
			/* FALLTHRU */
		case 1701: //Axis transfer
		case 1702: //Axis transfer line
			mapthings[i].thing_args[0] = mapthings[i].extrainfo;
			mapthings[i].thing_args[1] = mapthings[i].options;
			break;
		case 1703: //Ideya drone
			mapthings[i].thing_args[0] = mapthings[i].angle & 0xFFF;
			mapthings[i].thing_args[1] = mapthings[i].extrainfo*32;
			mapthings[i].thing_args[2] = ((mapthings[i].angle & 0xF000) >> 12)*32;
			if ((mapthings[i].options & (MTF_OBJECTSPECIAL|MTF_EXTRA)) == (MTF_OBJECTSPECIAL|MTF_EXTRA))
				mapthings[i].thing_args[3] = TMDA_BOTTOM;
			else if ((mapthings[i].options & (MTF_OBJECTSPECIAL|MTF_EXTRA)) == MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[3] = TMDA_TOP;
			else if ((mapthings[i].options & (MTF_OBJECTSPECIAL|MTF_EXTRA)) == MTF_EXTRA)
				mapthings[i].thing_args[3] = TMDA_MIDDLE;
			else
				mapthings[i].thing_args[3] = TMDA_BOTTOMOFFSET;
			mapthings[i].thing_args[4] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1704: //NiGHTS bumper
			mapthings[i].pitch = 30 * (((mapthings[i].options & 15) + 9) % 12);
			mapthings[i].options &= ~0xF;
			break;
		case 1705: //Hoop
		case 1713: //Hoop (Customizable)
		{
			UINT16 oldangle = mapthings[i].angle;
			mapthings[i].angle = ((oldangle >> 8)*360)/256;
			mapthings[i].pitch = ((oldangle & 255)*360)/256;
			mapthings[i].thing_args[0] = (mapthings[i].type == 1705) ? 96 : (mapthings[i].options & 0xF)*16 + 32;
			mapthings[i].options &= ~0xF;
			mapthings[i].type = 1713;
			break;
		}
		case 1710: //Ideya capture
			mapthings[i].thing_args[0] = mapthings[i].extrainfo;
			mapthings[i].thing_args[1] = mapthings[i].angle;
			break;
		case 1714: //Ideya anchor point
			mapthings[i].thing_args[0] = mapthings[i].extrainfo;
			break;
		case 1806: //King Bowser
			mapthings[i].thing_args[0] = LE_KOOPA;
			break;
		case 1807: //Axe
			mapthings[i].thing_args[0] = LE_AXE;
			break;
		case 2000: //Smashing spikeball
			mapthings[i].thing_args[0] = mapthings[i].angle;
			break;
		case 2006: //Jack-o'-lantern 1
		case 2007: //Jack-o'-lantern 2
		case 2008: //Jack-o'-lantern 3
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_EXTRA);
			break;
		case 2001: // MT_WAYPOINT
		{
			INT32 firstline = Tag_FindLineSpecial(2000, (INT16)mapthings[i].angle);

			mapthings[i].tid = mapthings[i].angle;

			mapthings[i].thing_args[0] = mapthings[i].z;
			mapthings[i].z = 0;

			if (firstline != -1)
			{
				fixed_t lineradius = sides[lines[firstline].sidenum[0]].textureoffset;
				fixed_t linez = sides[lines[firstline].sidenum[0]].rowoffset;

				if (lineradius > 0)
					mapthings[i].thing_args[1] = lineradius / FRACUNIT;

				mapthings[i].z = linez / FRACUNIT;
			}

			if (mapthings[i].extrainfo == 1)
			{
				mapthings[i].thing_args[2] |= TMWPF_FINISHLINE;
			}

			if (mapthings[i].options & MTF_EXTRA)
			{
				mapthings[i].thing_args[2] |= TMWPF_DISABLED;
			}

			if (mapthings[i].options & MTF_OBJECTSPECIAL)
			{
				mapthings[i].thing_args[2] |= TMWPF_SHORTCUT;
			}

			if (mapthings[i].options & MTF_AMBUSH)
			{
				mapthings[i].thing_args[2] |= TMWPF_NORESPAWN;
			}

			break;
		}
		case 2004: // MT_BOTHINT
			mapthings[i].thing_args[0] = mapthings[i].angle;
			mapthings[i].thing_args[1] = mapthings[i].extrainfo;
			mapthings[i].thing_args[2] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 2010: // MT_ITEMCAPSULE
			mapthings[i].thing_args[0] = mapthings[i].angle;
			mapthings[i].thing_args[1] = mapthings[i].extrainfo;

			if (mapthings[i].options & MTF_OBJECTSPECIAL)
			{
				// Special = +16 items (+80 for rings)
				mapthings[i].thing_args[1] += 16;
			}

			if (mapthings[i].options & MTF_EXTRA)
			{
				// was advertised as an "invert time attack" flag, actually was an "all gamemodes" flag
				mapthings[i].thing_args[3] = TMICM_MULTIPLAYER|TMICM_TIMEATTACK;
			}
			else
			{
				mapthings[i].thing_args[3] = TMICM_DEFAULT;
			}

			if (mapthings[i].options & MTF_AMBUSH)
			{
				mapthings[i].thing_args[2] |= TMICF_INVERTSIZE;
			}
			break;
		case 2020: // MT_LOOPENDPOINT
		{
			mapthings[i].thing_args[0] =
				mapthings[i].options & MTF_AMBUSH ?
				TMLOOP_BETA : TMLOOP_ALPHA;
			break;
		}
		case 2021: // MT_LOOPCENTERPOINT
			mapthings[i].thing_args[0] = (mapthings[i].options & MTF_AMBUSH) == MTF_AMBUSH;
			mapthings[i].thing_args[1] = mapthings[i].angle;
			break;
		case 2050: // MT_DUELBOMB
			mapthings[i].thing_args[1] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1950: // MT_AAZTREE_HELPER
			mapthings[i].thing_args[0] = mapthings[i].extrainfo;
			mapthings[i].thing_args[1] = mapthings[i].angle;
			break;
		case 2333: // MT_BATTLECAPSULE
			mapthings[i].thing_args[0] = mapthings[i].extrainfo;
			mapthings[i].thing_args[1] = mapthings[i].angle;

			if (mapthings[i].options & MTF_OBJECTSPECIAL)
			{
				mapthings[i].thing_args[2] |= TMBCF_REVERSE;
			}

			if (mapthings[i].options & MTF_AMBUSH)
			{
				mapthings[i].thing_args[2] |= TMBCF_BACKANDFORTH;
			}
			break;
		case 3122: // MT_MAYONAKAARROW
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[0] = TMMA_WARN;
			else if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].thing_args[0] = TMMA_FLIP;
			break;
		case 2018: // MT_PETSMOKER
			mapthings[i].thing_args[0] = !!(mapthings[i].options & MTF_OBJECTSPECIAL);
			break;
		case 3786: // MT_BATTLEUFO_SPAWNER
			mapthings[i].thing_args[0] = mapthings[i].angle;
			break;
		case 3400: // MT_WATERPALACETURBINE
		{
			mtag_t tag = (mtag_t)mapthings[i].angle;
			INT32 j = Tag_FindLineSpecial(2009, tag);

			if (j == -1)
			{
				CONS_Debug(DBG_GAMELOGIC, "Water Palace Turbine setup: Unable to find parameter line 2009 (tag %d)!\n", tag);
				break;
			}

			if (!lines[j].backsector)
			{
				CONS_Debug(DBG_GAMELOGIC, "Water Palace Turbine setup: No backside for parameter line 2009 (tag %d)!\n", tag);
				break;
			}

			mapthings[i].angle = sides[lines[j].sidenum[0]].rowoffset >> FRACBITS;

			if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].thing_args[0] = 1;
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].thing_args[1] = 1;

			mapthings[i].thing_args[2] = lines[j].frontsector->floorheight >> FRACBITS;
			mapthings[i].thing_args[3] = lines[j].frontsector->ceilingheight >> FRACBITS;

			mapthings[i].thing_args[4] = lines[j].backsector->floorheight >> FRACBITS;

			mapthings[i].thing_args[5] = sides[lines[j].sidenum[0]].textureoffset >> FRACBITS;
			if (mapthings[i].thing_args[5] < 0)
				mapthings[i].thing_args[5] = -mapthings[i].thing_args[5];

			mapthings[i].thing_args[6] = sides[lines[j].sidenum[1]].rowoffset >> FRACBITS;
			if (mapthings[i].thing_args[6] < 0)
				mapthings[i].thing_args[6] = -mapthings[i].thing_args[6];

			mapthings[i].thing_args[7] = sides[lines[j].sidenum[1]].textureoffset >> FRACBITS;
			if (mapthings[i].thing_args[7] < 0)
				mapthings[i].thing_args[7] = -mapthings[i].thing_args[7];

			if (lines[j].flags & ML_SKEWTD)
				mapthings[i].thing_args[8] = R_PointToDist2(lines[j].v2->x, lines[j].v2->y, lines[j].v1->x, lines[j].v1->y) >> FRACBITS;

			if (lines[j].flags & ML_NOSKEW)
				mapthings[i].thing_args[9] = 1;

			break;
		}
		case 3441: // MT_DASHRING
		case 3442: // MT_RAINBOWDASHRING
			mapthings[i].thing_args[0] = mapthings[i].options & 13;
			mapthings[i].thing_args[1] = mapthings[i].extrainfo;
			break;
		case FLOOR_SLOPE_THING:
		case CEILING_SLOPE_THING:
			mapthings[i].thing_args[0] = mapthings[i].extrainfo;
			break;
		case 4094: // MT_ARKARROW
			mapthings[i].thing_args[0] = mapthings[i].extrainfo;
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
			{
				// Special = add 16 to the symbol type
				mapthings[i].thing_args[0] += 16;
			}
			if (mapthings[i].options & MTF_AMBUSH)
			{
				// Ambush = add 32 to the symbol type
				mapthings[i].thing_args[0] += 32;
			}
			break;
		default:
			break;
		}
	}
}

static void P_ConvertBinaryLinedefFlags(void)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (!!(lines[i].flags & ML_DONTPEGBOTTOM) ^ !!(lines[i].flags & ML_MIDPEG))
			lines[i].flags |= ML_MIDPEG;
		else
			lines[i].flags &= ~ML_MIDPEG;

		if (lines[i].special >= 100 && lines[i].special < 300)
		{
			if (lines[i].flags & ML_DONTPEGTOP)
				lines[i].flags |= ML_SKEWTD;
			else
				lines[i].flags &= ~ML_SKEWTD;

			if ((lines[i].flags & ML_TFERLINE) && lines[i].frontsector)
			{
				size_t j;

				for (j = 0; j < lines[i].frontsector->linecount; j++)
				{
					if (lines[i].frontsector->lines[j]->flags & ML_DONTPEGTOP)
						lines[i].frontsector->lines[j]->flags |= ML_SKEWTD;
					else
						lines[i].frontsector->lines[j]->flags &= ~ML_SKEWTD;
				}
			}
		}

	}
}

//For maps in binary format, converts setup of specials to UDMF format.
static void P_ConvertBinaryMap(void)
{
	P_ConvertBinaryLinedefTypes();
	P_ConvertBinarySectorTypes();
	P_ConvertBinaryThingTypes();
	P_ConvertBinaryLinedefFlags();
}

/** Compute MD5 message digest for bytes read from memory source
  *
  * The resulting message digest number will be written into the 16 bytes
  * beginning at RESBLOCK.
  *
  * \param filename path of file
  * \param resblock resulting MD5 checksum
  * \return 0 if MD5 checksum was made, and is at resblock, 1 if error was found
  */
static INT32 P_MakeBufferMD5(const char *buffer, size_t len, void *resblock)
{
#ifdef NOMD5
	(void)buffer;
	(void)len;
	memset(resblock, 0x00, 16);
	return 1;
#else
	tic_t t = I_GetTime();
	CONS_Debug(DBG_SETUP, "Making MD5\n");
	if (md5_buffer(buffer, len, resblock) == NULL)
		return 1;
	CONS_Debug(DBG_SETUP, "MD5 calc took %f seconds\n", (float)(I_GetTime() - t)/NEWTICRATE);
	return 0;
#endif
}

static void P_MakeMapMD5(virtres_t *virt, void *dest)
{
	unsigned char resmd5[16];

	if (udmf)
	{
		virtlump_t *textmap = vres_Find(virt, "TEXTMAP");
		P_MakeBufferMD5((char*)textmap->data, textmap->size, resmd5);
	}
	else
	{
		unsigned char linemd5[16];
		unsigned char sectormd5[16];
		unsigned char thingmd5[16];
		unsigned char sidedefmd5[16];
		UINT8 i;

		// Create a hash for the current map
		// get the actual lumps!
		virtlump_t* virtlines   = vres_Find(virt, "LINEDEFS");
		virtlump_t* virtsectors = vres_Find(virt, "SECTORS");
		virtlump_t* virtmthings = vres_Find(virt, "THINGS");
		virtlump_t* virtsides   = vres_Find(virt, "SIDEDEFS");

		P_MakeBufferMD5((char*)virtlines->data,   virtlines->size, linemd5);
		P_MakeBufferMD5((char*)virtsectors->data, virtsectors->size,  sectormd5);
		P_MakeBufferMD5((char*)virtmthings->data, virtmthings->size,   thingmd5);
		P_MakeBufferMD5((char*)virtsides->data,   virtsides->size, sidedefmd5);

		for (i = 0; i < 16; i++)
			resmd5[i] = (linemd5[i] + sectormd5[i] + thingmd5[i] + sidedefmd5[i]) & 0xFF;
	}

	M_Memcpy(dest, &resmd5, 16);
}

static boolean P_LoadMapFromFile(void)
{
	TracyCZone(__zone, true);

	virtlump_t *textmap = vres_Find(curmapvirt, "TEXTMAP");

	udmf = textmap != NULL;
	udmf_version = 0;

	if (!P_LoadMapData(curmapvirt))
	{
		TracyCZoneEnd(__zone);
		return false;
	}

	P_LoadMapBSP(curmapvirt);
	P_LoadMapLUT(curmapvirt);

	P_LinkMapData();

	if (!udmf)
		P_AddBinaryMapTags();

	Taglist_InitGlobalTables();

	if (!udmf)
		P_ConvertBinaryMap();

	if (P_CanWriteTextmap())
		P_WriteTextmap();

	// Copy relevant map data for NetArchive purposes.
	P_DeepCopySectors(&spawnsectors, &sectors, numsectors);
	P_DeepCopyLines(&spawnlines, &lines, numlines);
	P_DeepCopySides(&spawnsides, &sides, numsides);

	P_MakeMapMD5(curmapvirt, &mapmd5);

	TracyCZoneEnd(__zone);
	return true;
}

//
// LEVEL INITIALIZATION FUNCTIONS
//

/** Sets up a sky texture to use for the level.
  * The sky texture is used instead of F_SKY1.
  */
void P_SetupLevelSky(const char *skytexname, boolean global)
{
	char tex[9];
	if (!skytexname || !skytexname[0])
		return;

	strncpy(tex, skytexname, 9);
	tex[8] = 0;

	skytexture = R_TextureNumForName(tex);
	strncpy(levelskytexture, tex, 9);

	// Global change
	if (global)
		strncpy(globallevelskytexture, tex, 9);

	// Don't go beyond for dedicated servers
	if (dedicated)
		return;

	// scale up the old skies, if needed
	R_SetupSkyDraw();
}

static const char *maplumpname;
lumpnum_t lastloadedmaplumpnum; // for comparative savegame

extern "C" boolean blockreset;

//
// P_LevelInitStuff
//
// Some player initialization for map start.
//
static void P_InitLevelSettings(void)
{
	INT32 i;
	UINT8 p = 0;

	leveltime = 0;
	modulothing = 0;
	blockreset = 0;

	P_SetFreezeLevel(false);
	P_SetFreezeCheat(false);

	K_TimerReset();

	nummaprings = 0;
	nummapboxes = numgotboxes = 0;
	maptargets = numtargets = 0;
	battleprisons = false;

	nummapspraycans = 0;
	numchallengedestructibles = 0;

	Obj_AncientGearLevelInit();

	// circuit, race and competition stuff
	numcheatchecks = 0;

	if (!g_reloadinggamestate)
		timeinmap = 0;

	// special stage
	stagefailed = true; // assume failed unless proven otherwise - P_GiveEmerald or emerald touchspecial
	// Reset temporary record data
	//memset(&ntemprecords, 0, sizeof(nightsdata_t));

	// earthquake camera
	g_quakes = NULL;

	// song credit init
	if (!Music_Playing("credits"))
	{
		S_StopMusicCredit();
		cursongcredit.trans = NUMTRANSMAPS;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
			p++;

		if (grandprixinfo.gp == false)
			players[i].lives = 3;

		G_PlayerReborn(i, true);
		K_UpdateShrinkCheat(&players[i]);
	}

	racecountdown = exitcountdown = musiccountdown = exitfadestarted = 0;

	g_exit.losing = false;
	g_exit.retry = false;

	// Gamespeed and frantic items
	const boolean multi_speed = (gametypes[gametype]->speed == KARTSPEED_AUTO);
	gamespeed = multi_speed ? KARTSPEED_EASY : gametypes[gametype]->speed;
	franticitems = false;
	g_teamplay = false;
	g_duelpermitted = false;

	if (K_PodiumSequence() == true)
	{
		// Okay, now that everything preceding is handled, set the position.
		for (i = 0; i < MAXPLAYERS; i++)
		{
			players[i].position = K_GetPodiumPosition(&players[i]);
		}

		// We don't touch the gamespeed, though!
	}
	else if (tutorialchallenge == TUTORIALSKIP_INPROGRESS)
	{
		gamespeed = KARTSPEED_NORMAL;
	}
	else if (grandprixinfo.gp == true)
	{
		if (multi_speed)
		{
			gamespeed = grandprixinfo.gamespeed;
		}
	}
	else if (modeattacking != ATTACKING_NONE)
	{
		if (multi_speed)
		{
			if ((gametyperules & GTR_CATCHER) && encoremode == false)
			{
				gamespeed = KARTSPEED_NORMAL;
			}
			else
			{
				gamespeed = KARTSPEED_HARD;
			}
		}
	}
	else
	{
		if (multi_speed)
		{
			if (cv_kartspeed.value == KARTSPEED_AUTO)
				gamespeed = ((speedscramble == -1) ? KARTSPEED_EASY : (UINT8)speedscramble);
			else
				gamespeed = (UINT8)cv_kartspeed.value;
		}
		franticitems = (boolean)cv_kartfrantic.value;
		g_teamplay = (boolean)cv_teamplay.value; // we will overwrite this later if there is not enough players
		g_duelpermitted = (boolean)cv_duel.value; // Ignored if too many players, see K_InRaceDuel

	}

	memset(&battleovertime, 0, sizeof(struct battleovertime));
	speedscramble = encorescramble = -1;

	K_ResetSpecialStage();
	K_ResetBossInfo();

	memset(&g_endcam, 0, sizeof g_endcam);
}

#if 0
// Respawns all the mapthings and mobjs in the map from the already loaded map data.
void P_RespawnThings(void)
{
	// Search through all the thinkers.
	thinker_t *think;
	INT32 i, viewid = -1, centerid = -1; // for skyboxes

	// check if these are any of the normal viewpoint/centerpoint mobjs in the level or not
	if (skyboxmo[0] || skyboxmo[1])
		for (i = 0; i < 16; i++)
		{
			if (skyboxmo[0] && skyboxmo[0] == skyboxviewpnts[i])
				viewid = i; // save id just in case
			if (skyboxmo[1] && skyboxmo[1] == skyboxcenterpnts[i])
				centerid = i; // save id just in case
		}

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;
		P_RemoveMobj((mobj_t *)think);
	}

	P_InitLevelSettings();

	memset(localaiming, 0, sizeof(localaiming));

	P_SpawnMapThings(true);

	// restore skybox viewpoint/centerpoint if necessary, set them to defaults if we can't do that
	skyboxmo[0] = skyboxviewpnts[(viewid >= 0) ? viewid : 0];
	skyboxmo[1] = skyboxcenterpnts[(centerid >= 0) ? centerid : 0];
}
#endif

static void P_ResetSpawnpoints(void)
{
	UINT8 i, j;

	// reset the player starts
	for (i = 0; i < MAXPLAYERS; i++)
	{
		playerstarts[i] = NULL;

		if (playeringame[i])
		{
			players[i].skybox.viewpoint = NULL;
			players[i].skybox.centerpoint = NULL;
		}
	}

	numfaultstarts = 0;
	faultstart = NULL;

	numdmstarts = 0;
	for (i = 0; i < MAX_DM_STARTS; i++)
		deathmatchstarts[i] = NULL;

	for (i = 0; i < TEAM__MAX; i++)
	{
		numteamstarts[i] = 0;

		for (j = 0; j < MAXPLAYERS; j++)
		{
			teamstarts[i][j] = NULL;
		}
	}

	for (i = 0; i < 16; i++)
		skyboxviewpnts[i] = skyboxcenterpnts[i] = NULL;
}

static void P_TryAddExternalGhost(const char *defdemoname)
{
	if (FIL_FileExists(defdemoname))
	{
		savebuffer_t buf = {0};

		if (P_SaveBufferFromFile(&buf, defdemoname))
		{
			Z_ChangeTag(buf.buffer, PU_LEVEL);
			G_AddGhost(&buf, defdemoname);
		}
		else
		{
			CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), defdemoname);
		}
	}
}

static void P_LoadRecordGhosts(void)
{
	// see also /menus/play-local-race-time-attack.c's M_PrepareTimeAttack
	char *gpath;
	const char *modeprefix = M_GetRecordMode();
	INT32 i;

	gpath = Z_StrDup(va("%s" PATHSEP "media" PATHSEP "replay" PATHSEP "%s" PATHSEP "%s", srb2home, timeattackfolder, G_BuildMapName(gamemap)));

	enum
	{
		kTime	= 1 << 0,
		kLap	= 1 << 1,
		kLast	= 1 << 2,
	};

	auto map_ghosts = [](int value)
	{
		auto map = [](const consvar_t& cvar, int value, UINT8 bit) { return cvar.value == value ? bit : 0; };

		return
			// Best Time ghost
			((modeattacking & ATTACKING_TIME) ? map(cv_ghost_besttime, value, kTime) : 0) |

			// Best Lap ghost
			((modeattacking & ATTACKING_LAP) ? map(cv_ghost_bestlap, value, kLap) : 0) |

			// Best Lap ghost
			map(cv_ghost_last, value, kLast);
	};

	auto add_ghosts = [gpath](const srb2::String& base, UINT8 bits)
	{
		auto load = [base](const char* suffix) { P_TryAddExternalGhost(fmt::format("{}{}.lmp", base, suffix).c_str()); };

		if (bits & kTime)
			load("time-best");

		if (bits & kLap)
			load("lap-best");

		if (bits & kLast)
			load("last");
	};

	UINT8 allGhosts = map_ghosts(2);
	UINT8 sameGhosts = map_ghosts(1);

	if (allGhosts)
	{
		for (i = 0; i < numskins; ++i)
			add_ghosts(fmt::format("{}-{}-{}", gpath, skins[i]->name, modeprefix), allGhosts);
	}

	if (sameGhosts)
	{
		INT32 skin = R_SkinAvailableEx(cv_skin[0].string, false);
		if (skin < 0 || !R_SkinUsable(-1, skin, false))
			skin = 0; // use default skin
		add_ghosts(fmt::format("{}-{}-{}", gpath, skins[skin]->name, modeprefix), sameGhosts);
	}

	// Guest ghost
	if (cv_ghost_guest.value)
		P_TryAddExternalGhost(va("%s-%sguest.lmp", gpath, modeprefix));

	// Staff Attack ghosts
	if (cv_ghost_staff.value && !modeprefix[0])
	{
		for (i = mapheaderinfo[gamemap-1]->ghostCount; i > 0; i--)
		{
			savebuffer_t buf = {0};

			staffbrief_t* ghostbrief = mapheaderinfo[gamemap-1]->ghostBrief[i - 1];
			const char* lumpname = W_CheckLongNameForNumPwad(ghostbrief->wad, ghostbrief->lump);
			size_t lumplength = W_LumpLengthPwad(ghostbrief->wad, ghostbrief->lump);
			if (lumplength == 0)
			{
				if (lumpname)
				{
					CONS_Alert(CONS_ERROR, M_GetText("Failed to read staff ghost lump '%s'.\n"), lumpname);
				}
				else
				{
					CONS_Alert(CONS_ERROR, M_GetText("Failed to read staff ghost lump for map '%s'.\n"), mapheaderinfo[gamemap-1]->lumpname);
				}

				continue;
			}

			P_SaveBufferZAlloc(&buf, lumplength, PU_LEVEL, NULL);
			W_ReadLumpPwad(ghostbrief->wad, ghostbrief->lump, buf.buffer);
			G_AddGhost(&buf, (char*)lumpname);
		}
	}

	Z_Free(gpath);
}

static void P_SetupCamera(UINT8 pnum, camera_t *cam)
{
	if (players[pnum].mo && (server || addedtogame))
	{
		cam->x = players[pnum].mo->x;
		cam->y = players[pnum].mo->y;
		cam->z = players[pnum].mo->z;
		cam->angle = players[pnum].mo->angle;
		cam->subsector = R_PointInSubsector(cam->x, cam->y); // make sure camera has a subsector set -- Monster Iestyn (12/11/18)
	}
	else
	{
		mapthing_t *thing;

		if (gametyperules & GTR_BATTLESTARTS)
			thing = deathmatchstarts[0];
		else
			thing = playerstarts[0];

		if (thing)
		{
			cam->x = thing->x;
			cam->y = thing->y;
			cam->z = thing->z;
			cam->angle = FixedAngle((fixed_t)thing->angle << FRACBITS);
			cam->subsector = R_PointInSubsector(cam->x, cam->y); // make sure camera has a subsector set -- Monster Iestyn (12/11/18)
		}
	}

	cam->chase = false; // tell camera to reset its position next tic
}

static void P_InitCamera(void)
{
	if (!dedicated)
	{
		UINT8 i;

		for (i = 0; i <= r_splitscreen; i++)
		{
			//displayplayers[i] = g_localplayers[i]; // Start with your OWN view, please!
			P_SetupCamera(displayplayers[i], &camera[i]);
		}
	}
}

static void P_ShuffleTeams(void)
{
	size_t i;

	if (G_GametypeHasTeams() == false)
	{
		// Teams are not enabled, force to TEAM_UNASSIGNED

		for (i = 0; i < MAXPLAYERS; i++)
		{
			players[i].team = TEAM_UNASSIGNED;
		}

		return;
	}

	// The following will sort TEAM_UNASSIGNED players at random.
	// In addition, you should know all players will have their team
	// unset every round unless certain conditions are met.
	// See Y_MidIntermission, G_InitNew (where resetplayer == true)

	CONS_Debug(DBG_TEAMS, "Shuffling player teams...\n");

	srb2::Vector<UINT8> player_shuffle;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] == false || players[i].spectator == true)
		{
			continue;
		}
		player_shuffle.push_back(i);
	}

	size_t n = player_shuffle.size();
	if (inDuel == true || n <= 2) // cv_teamplay_min.value
	{
		CONS_Debug(DBG_TEAMS, "Not enough players to support teams; forcing teamplay preference off.\n");

		// Not enough players for teams.
		// Turn off the preference for this match.
		g_teamplay = false;

		// But we may still be in a forced
		// teams gametype, so only return false
		// if our preference means anything.
		if (G_GametypeHasTeams() == false)
		{
			return;
		}
	}

	if (n > 1)
	{
		for (i = n - 1; i > 0; i--)
		{
			size_t j = P_RandomKey(PR_TEAMS, i + 1);

			size_t temp = player_shuffle[i];
			player_shuffle[i] = player_shuffle[j];
			player_shuffle[j] = temp;
		}
	}

	for (i = 0; i < n; i++)
	{
		G_AutoAssignTeam(&players[ player_shuffle[i] ]);
	}
}

static void P_InitPlayers(void)
{
	INT32 i, skin = -1, follower = -1, col = SKINCOLOR_NONE;

	// Make sure objectplace is OFF when you first start the level!
	OP_ResetObjectplace();

	// Update skins / colors between levels.
	G_UpdateAllPlayerPreferences();

	// Are we forcing a character?
	if (gametype == GT_TUTORIAL)
	{
		// Get skin from name.
		if (mapheaderinfo[gamemap-1] && mapheaderinfo[gamemap-1]->relevantskin[0])
		{
			if (strcmp(mapheaderinfo[gamemap-1]->relevantskin, "_PROFILE") == 0)
			{
				profile_t *p = PR_GetProfile(cv_ttlprofilen.value);
				if (p && !netgame)
				{
					skin = R_SkinAvailable(p->skinname);

					if (!R_SkinUsable(g_localplayers[0], skin, false))
					{
						skin = GetSkinNumClosestToStats(skins[skin]->kartspeed, skins[skin]->kartweight, skins[skin]->flags, false);
					}

					if (K_ColorUsable(static_cast<skincolornum_t>(p->color), false, true) == true)
					{
						col = p->color;
					}
				}
			}
			else
			{
				skin = R_SkinAvailable(mapheaderinfo[gamemap-1]->relevantskin);
			}
		}
		else
		{
			skin = R_SkinAvailable(DEFAULTSKIN);
		}

		// Handle invalid case.
		if (skin == -1)
		{
			skin = 0;
		}

		if (netgame || horngoner)
			; // shouldn't happen but at least attempt to sync if it does
		else for (i = 0; i < numfollowers; i++)
		{
			if (followers[i].hornsound != sfx_melody)
				continue;

			if (K_FollowerUsable(i))
				follower = i;
			break;
		}
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		players[i].mo = NULL;

		// If we're forcing a character, do it now.
		if (skin != -1)
		{
			SetPlayerSkinByNum(i, skin);
			players[i].skincolor = (col != SKINCOLOR_NONE) ? col : skins[skin]->prefcolor;

			players[i].followerskin = follower;
			if (follower != -1)
				players[i].followercolor = followers[follower].defaultcolor;
		}

		G_SpawnPlayer(i);

		players[i].xtralife = 0; // extra lives do not ever carry over from the previous round

		if (P_MobjWasRemoved(players[i].mo) == false && !players[i].spectator)
		{
			// Rooooooolllling staaaaaaart
			if ((gametyperules & (GTR_ROLLINGSTART|GTR_CIRCUIT)) == (GTR_ROLLINGSTART|GTR_CIRCUIT))
			{
				P_InstaThrust(players[i].mo, players[i].mo->angle, K_GetKartSpeed(&players[i], false, false));
			}
		}
	}

	K_UpdateAllPlayerPositions();
}

static void P_InitGametype(void)
{
	size_t i;

	spectateGriefed = 0;
	K_CashInPowerLevels(); // Pushes power level changes even if intermission was skipped

	if (modeattacking && !demo.playback)
		P_LoadRecordGhosts();

	numlaps = K_RaceLapCount(gamemap - 1);

	wantedcalcdelay = wantedfrequency*2;

	for (i = 0; i < NUMKARTITEMS-1; i++)
		itemCooldowns[i] = 0;

	mapreset = 0;

	thwompsactive = false;
	lastLowestLap = 0;
	spbplace = -1;

	G_ClearRetryFlag();

	// Start recording replay in multiplayer with a temp filename
	//@TODO I'd like to fix dedis crashing when recording replays for the future too...
	if (gamestate == GS_LEVEL && !demo.playback && multiplayer && !dedicated)
	{
		char buf[MAX_WADPATH];
		char ver[128];
		int parts;

#ifdef DEVELOP
		if (strcmp(compbranch, ""))
			sprintf(ver, "%s-%s", compbranch, comprevision);
		else
			strcpy(ver, comprevision);
#else
		strcpy(ver, VERSIONSTRING);
#endif
		// Replace path separators with hyphens
		{
			char *p = ver;
			while ((p = strpbrk(p, "/\\")))
			{
				*p = '-';
				p++;
			}
		}
		sprintf(buf, "%s" PATHSEP "media" PATHSEP "replay" PATHSEP "online" PATHSEP "%s" PATHSEP "%d-%s",
				srb2home, ver, (int) (time(NULL)), G_BuildMapName(gamemap));

		parts = M_PathParts(buf);
		M_MkdirEachUntil(buf, parts - 5, parts - 1, 0755);

		G_RecordDemo(buf);
	}

	if (gamestate != GS_TITLESCREEN && M_GameTrulyStarted())
	{
		// Started a game? Move on to the next jam when you go back to the title screen
		// this permits all but titlescreen, instead of only GS_LEVEL
		// because that one's way too easy to activate again and again
		CV_SetValue(&cv_menujam_update, 1);
		gamedata->musicstate = GDMUSIC_NONE;
	}
}

struct minimapinfo minimapinfo;

static void P_InitMinimapInfo(void)
{
	size_t i, count;
	fixed_t a;
	fixed_t b;

	node_t *bsp = &nodes[numnodes-1];

	minimapinfo.minimap_pic = static_cast<patch_t*>(mapheaderinfo[gamemap-1]->minimapPic);

	minimapinfo.min_x = minimapinfo.max_x = minimapinfo.min_y = minimapinfo.max_y = INT32_MAX;
	count = 0;
	for (i = 0; i < nummapthings; i++)
	{
		if (mapthings[i].type != mobjinfo[MT_MINIMAPBOUND].doomednum)
			continue;
		count++;

		if (mapthings[i].x < minimapinfo.min_x)
		{
			minimapinfo.max_x = minimapinfo.min_x;
			minimapinfo.min_x = mapthings[i].x;
		}
		else
		{
			minimapinfo.max_x = mapthings[i].x;
		}

		if (mapthings[i].y < minimapinfo.min_y)
		{
			minimapinfo.max_y = minimapinfo.min_y;
			minimapinfo.min_y = mapthings[i].y;
		}
		else
		{
			minimapinfo.max_y = mapthings[i].y;
		}
	}

	if (count == 0)
	{
		minimapinfo.min_x = bsp->bbox[0][BOXLEFT];
		minimapinfo.max_x = bsp->bbox[0][BOXRIGHT];
		minimapinfo.min_y = bsp->bbox[0][BOXBOTTOM];
		minimapinfo.max_y = bsp->bbox[0][BOXTOP];

		if (bsp->bbox[1][BOXLEFT] < minimapinfo.min_x)
			minimapinfo.min_x = bsp->bbox[1][BOXLEFT];
		if (bsp->bbox[1][BOXRIGHT] > minimapinfo.max_x)
			minimapinfo.max_x = bsp->bbox[1][BOXRIGHT];
		if (bsp->bbox[1][BOXBOTTOM] < minimapinfo.min_y)
			minimapinfo.min_y = bsp->bbox[1][BOXBOTTOM];
		if (bsp->bbox[1][BOXTOP] > minimapinfo.max_y)
			minimapinfo.max_y = bsp->bbox[1][BOXTOP];

		// You might be wondering why these are being bitshift here
		// it's because mapwidth and height would otherwise overflow for maps larger than half the size possible...
		// map boundaries and sizes will ALWAYS be whole numbers thankfully
		// later calculations take into consideration that these are actually not in terms of FRACUNIT though
		minimapinfo.min_x >>= FRACBITS;
		minimapinfo.max_x >>= FRACBITS;
		minimapinfo.min_y >>= FRACBITS;
		minimapinfo.max_y >>= FRACBITS;
	}
	else if (count != 2)
	{
		I_Error("P_InitMinimapInfo: Too %s minimap helper objects! (found %s of mapthingnum %d, should have 2)",
			(count < 2 ? "few" : "many"), sizeu1(count), mobjinfo[MT_MINIMAPBOUND].doomednum);
	}
	minimapinfo.map_w = minimapinfo.max_x - minimapinfo.min_x;
	minimapinfo.map_h = minimapinfo.max_y - minimapinfo.min_y;

	minimapinfo.minimap_w = minimapinfo.minimap_h = 100;

	a = FixedDiv(minimapinfo.minimap_w<<FRACBITS, minimapinfo.map_w<<4);
	b = FixedDiv(minimapinfo.minimap_h<<FRACBITS, minimapinfo.map_h<<4);

	if (a < b)
	{
		minimapinfo.minimap_h = FixedMul(a, minimapinfo.map_h)>>(FRACBITS-4);
		minimapinfo.zoom = a;
	}
	else
	{
		if (a != b)
		{
			minimapinfo.minimap_w = FixedMul(b, minimapinfo.map_w)>>(FRACBITS-4);
		}
		minimapinfo.zoom = b;
	}

	minimapinfo.zoom >>= (FRACBITS-4);
	minimapinfo.zoom -= (minimapinfo.zoom/20);

	// These should always be small enough to be bitshift back right now
	minimapinfo.offs_x = FixedMul((minimapinfo.min_x + minimapinfo.map_w/2) << FRACBITS, minimapinfo.zoom);
	minimapinfo.offs_y = FixedMul((minimapinfo.min_y + minimapinfo.map_h/2) << FRACBITS, minimapinfo.zoom);
}

void P_ResetLevelMusic(void)
{
	UINT8 idx = 0;

	mapheader_t* mapheader = mapheaderinfo[gamemap - 1];
	UINT8 truesize = (encoremode && mapheader->encoremusname_size)
		? mapheader->encoremusname_size
		: mapheader->musname_size;

	// To keep RNG in sync, we will always pull from RNG, even if unused
	UINT32 random = P_Random(PR_MUSICSELECT);

	if (demo.playback)
	{
		// mapmusrng has already been set by the demo; just make sure it's valid
		if (mapmusrng >= truesize)
		{
			mapmusrng = 0;
		}
		return;
	}

	if (truesize > 1)
	{
		UINT8 tempmapmus[MAXMUSNAMES], tempmapmus_size = 1, i;

		tempmapmus[0] = 0;

		for (i = 1; i < truesize; i++)
		{
			if (mapheader->cache_muslock[i-1] < MAXUNLOCKABLES
			&& !M_CheckNetUnlockByID(mapheader->cache_muslock[i-1]))
				continue;

			tempmapmus[tempmapmus_size++] = i;
		}

		if (tempmapmus_size > 1)
		{
			if (g_reloadingMap == (modeattacking == ATTACKING_NONE))
			{
				// If restarting the map, simply cycle
				// through available alt music.
				idx = (mapmusrng + 1) % tempmapmus_size;
			}
			else if (modeattacking)
			{
				// Short circuit the cycle.
				idx = mapmusrng % tempmapmus_size;
			}
			else
			{
				idx = random % tempmapmus_size;
			}
			idx = tempmapmus[idx];
		}
	}

	mapmusrng = idx;
}

boolean P_UseContinuousLevelMusic(void)
{
	if (gametyperules & GTR_BOSS)
		return false;

	if (modeattacking != ATTACKING_NONE)
		return cv_continuousmusic.value;

	return (gametyperules & GTR_NOPOSITION) != 0;
}

void P_LoadLevelMusic(void)
{
	mapheader_t* mapheader = mapheaderinfo[gamemap-1];
	const char *music = mapheader->musname[0];

	if (encoremode && mapheader->encoremusname_size
	&& mapmusrng < mapheader->encoremusname_size)
	{
		music = mapheader->encoremusname[mapmusrng];
	}
	else if (mapmusrng < mapheader->musname_size)
	{
		music = mapheader->musname[mapmusrng];
	}

	if (P_UseContinuousLevelMusic())
	{
		if (!stricmp(Music_Song("level_nosync"), music))
		{
			//  Do not reset music if it is the same
			Music_BatchExempt("level_nosync");
		}
		Music_StopAll();
		Music_Remap("level_nosync", music);
	}
	else
	{
		Music_StopAll();
		Music_Remap("level", music);

		tic_t level_music_start = starttime + (TICRATE/2);
		Music_Seek("level", (std::max(leveltime, level_music_start) - level_music_start) * 1000UL / TICRATE);
	}

	Music_ResetLevelVolume();
}

void P_FreeLevelState(void)
{
	if (numsectors)
	{
		F_EndTextPrompt(false, true);
		K_UnsetDialogue();

		ACS_InvalidateMapScope();
		LUA_InvalidateLevel();

		Obj_ClearCheckpoints();

		sector_t *ss;
		for (ss = sectors; sectors+numsectors != ss; ss++)
		{
			Z_Free(ss->attached);
			Z_Free(ss->attachedsolid);
		}

		// This is the simplest guard against double frees.
		// No valid map has zero sectors. Or, come to think
		// of it, less than two in general! ~toast 310525
		numsectors = 0;
	}

	// Clear pointers that would be left dangling by the purge
	R_FlushTranslationColormapCache();

#ifdef HWRENDER
	// Free GPU textures before freeing patches.
	if (rendermode == render_opengl && (vid.glstate == VID_GL_LIBRARY_LOADED))
		HWR_ClearAllTextures();
#endif

	if (rendermode == render_soft)
	{
		// Queued draws might reference patches or colormaps about to be freed.
		// Flush 2D to make sure no read-after-free occurs.
		srb2::rhi::Rhi* rhi = srb2::sys::get_rhi(srb2::sys::g_current_rhi);
		srb2::sys::main_hardware_state()->twodee_renderer->flush(*rhi, srb2::g_2d);
	}

	G_FreeGhosts(); // ghosts are allocated with PU_LEVEL
	Patch_FreeTag(PU_PATCH_LOWPRIORITY);
	Patch_FreeTag(PU_PATCH_ROTATED);
	Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

	R_InitMobjInterpolators();
	R_InitializeLevelInterpolators();
}

/** Loads a level from a lump or external wad.
  *
  * \param fromnetsave If true, skip some stuff because we're loading a netgame snapshot.
  * \todo Clean up, refactor, split up; get rid of the bloat.
  */
boolean P_LoadLevel(boolean fromnetsave, boolean reloadinggamestate)
{
	TracyCZone(__zone, true);

	// use gamemap to get map number.
	// 99% of the things already did, so.
	// Map header should always be in place at this point
	INT32 i;
	virtlump_t *encoreLump = NULL;

	boolean fade_shortcircuit = false;

	levelloading = true;
	g_reloadinggamestate = reloadinggamestate;

	// This is needed. Don't touch.
	maptol = mapheaderinfo[gamemap-1]->typeoflevel;

	CON_Drawer(); // let the user know what we are going to do
	I_FinishUpdate(); // page flip or blit buffer

	// Reset the palette
	if (rendermode != render_none)
		V_SetPaletteLump("PLAYPAL");

	// Initialize sector node list.
	P_Initsecnode();

	// Clear CECHO messages
	HU_ClearCEcho();
	HU_ClearTitlecardCEcho();

	if (mapheaderinfo[gamemap-1]->runsoc[0] != '#')
		P_RunSOC(mapheaderinfo[gamemap-1]->runsoc);

	P_InitLevelSettings();

	if (demo.attract != DEMO_ATTRACT_TITLE && gamestate != GS_TITLESCREEN)
	{
		// Stop titlescreen music from overriding level music.
		// Except on the title screen, where an attract demo or title map may be used.
		Music_Stop("title");
	}

	if (demo.attract != DEMO_ATTRACT_CREDITS)
	{
		Music_Stop("credits");
	}

	for (i = 0; i <= r_splitscreen; i++)
		postimgtype[i] = postimg_none;

	// Initial height of PointOfView
	// will be set by player think.
	players[consoleplayer].viewz = 1;

	// (This define might be useful for other areas of code? Not sure)
	tic_t locstarttime, endtime, nowtime;

#define WAIT(timetowait) \
	locstarttime = nowtime = lastwipetic; \
	endtime = locstarttime + timetowait; \
	while (nowtime < endtime) \
	{ \
		while (!((nowtime = I_GetTime()) - lastwipetic)) \
		{ \
			I_Sleep(cv_sleep.value); \
			I_UpdateTime(); \
		} \
		lastwipetic = nowtime; \
		if (moviemode && rendermode == render_opengl) \
			M_LegacySaveFrame(); \
		else if (moviemode && rendermode == render_soft) \
			I_CaptureVideoFrame(); \
		NetKeepAlive(); \
	} \

	// Cancel all d_main.c fadeouts (keep fade in though).
	if (reloadinggamestate)
		wipegamestate = gamestate; // Don't fade if reloading the gamestate
	// Encore mode fade to pink to white
	// This is handled BEFORE sounds are stopped.
	else if (encoremode && !prevencoremode && !demo.simplerewind)
	{
		if (rendermode != render_none)
		{
			Music_StopAll(); // er, about that...

			// Fade to an inverted screen, with a circle fade...
			F_WipeStartScreen();

			V_EncoreInvertScreen();
			F_WipeEndScreen();

			S_StartSound(NULL, sfx_ruby1);
			F_RunWipe(wipe_encore_toinvert, wipedefs[wipe_encore_toinvert], false, NULL, false, false);

			// Hold on invert for extra effect.

			WAIT((3*TICRATE)/2);
			S_StartSound(NULL, sfx_ruby2);

			// Then fade to a white screen
			F_WipeStartScreen();

			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);
			F_WipeEndScreen();

			F_RunWipe(wipe_encore_towhite, wipedefs[wipe_encore_towhite], false, "FADEMAP1", false, true); // wiggle the screen during this!

			// THEN fade to a black screen.
			F_WipeStartScreen();

			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
			F_WipeEndScreen();

			F_RunWipe(wipe_level_toblack, wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

			// Wait a bit longer.
			WAIT((3*TICRATE)/4);
		}
		else
		{
			// dedicated servers can call this now, to wait the appropriate amount of time for clients to wipe
			F_RunWipe(wipe_encore_towhite, wipedefs[wipe_encore_towhite], false, "FADEMAP1", false, true);

			WAIT((3*TICRATE)/2);

			F_RunWipe(wipe_level_toblack, wipedefs[wipe_level_toblack], false, "FADEMAP0", false, false);

			WAIT((3*TICRATE)/4);
		}
	}
#undef WAIT

	if (demo.attract
	|| (G_IsModeAttackRetrying() && !demo.playback && (gametyperules & GTR_BOSS) == 0))
	{
		fade_shortcircuit = true;
	}

	// Make sure all sounds are stopped before Z_FreeTags.
	S_StopSounds();
	S_ClearSfx();

	// Reset the palette now all fades have been done
	if (rendermode != render_none)
		V_ReloadPalette(); // Set the level palette

	// Music set-up
	if (demo.attract || demo.simplerewind)
	{
		// Leave the music alone! We're already playing what we want!
		// Pull from RNG even though music will never change
		// To silence playback has desynced warning
		P_Random(PR_MUSICSELECT);
	}
	else if (!reloadinggamestate)
	{
		if (K_PodiumSequence())
		{
			// mapmusrng is set by local player position in K_ResetCeremony
			P_LoadLevelMusic();
		}
		else if (gamestate == GS_LEVEL)
		{
			if (fade_shortcircuit)
			{
				pausedelay = -3; // preticker plus one
			}

			// We should be fine starting music here.
			// Don't do this during titlemap, because the menu code handles music by itself.
			// Netsave loading does this itself, because leveltime isn't set yet!
			if (!fromnetsave)
			{
				P_ResetLevelMusic();
				P_LoadLevelMusic();
			}
		}
	}

	// Let's fade to black or white here
	// But only if we didn't do the encore startup wipe
	if (!reloadinggamestate && !demo.simplerewind)
	{
		int wipetype = wipe_level_toblack;
		sfxenum_t fadesound = sfx_None;

		// Default
		levelfadecol = 31;

		if (gamestate == GS_TITLESCREEN)
		{
			;
		}
		else if (K_PodiumHasEmerald())
		{
			// Special Stage out
			fadesound = sfx_s3k6a;
			levelfadecol = 0;
			wipetype = wipe_encore_towhite;
		}
		else if (gametyperules & GTR_SPECIALSTART)
		{
			// Special Stage in
			fadesound = sfx_s3kaf;
			levelfadecol = 0;
			wipetype = wipe_encore_towhite;
		}
		else if (skipstats == 1 && (gametyperules & GTR_BOSS) == 0)
		{
			// MapWarp
			fadesound = sfx_s3k73;
			levelfadecol = 0;
			wipetype = wipe_encore_towhite;
		}
		else if (encoremode)
		{
			// Encore
			levelfadecol = 0;
			wipetype = wipe_encore_towhite;
		}

		if (g_attractnowipe)
		{
			// Attract demos do a custom fade on exit, so
			// don't run a wipe here.
			g_attractnowipe = false;
		}
		else
		{
			if (!fade_shortcircuit)
				S_StartSound(NULL, fadesound);

			if (rendermode != render_none)
			{
				F_WipeStartScreen();

				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, levelfadecol);
				F_WipeEndScreen();
			}

			F_RunWipe(wipetype, wipedefs[wipetype], false, ((levelfadecol == 0) ? "FADEMAP1" : "FADEMAP0"), false, false);

			// Hold respawn to keep waiting until you're ready
			if (G_IsModeAttackRetrying() && !demo.playback)
			{
				nowtime = lastwipetic;
				while (G_PlayerInputDown(0, gc_bail, splitscreen + 1) == true)
				{
					while (!((nowtime = I_GetTime()) - lastwipetic))
					{
						I_Sleep(cv_sleep.value);
						I_UpdateTime();
					} \

					I_OsPolling();
					G_ResetAllDeviceResponding();

					for (; eventtail != eventhead; eventtail = (eventtail+1) & (MAXEVENTS-1))
					{
						HandleGamepadDeviceEvents(&events[eventtail]);
						G_MapEventsToControls(&events[eventtail]);
					}

					lastwipetic = nowtime;
					if (moviemode && rendermode == render_opengl)
						M_LegacySaveFrame();
					else if (moviemode && rendermode == render_soft)
						I_CaptureVideoFrame();
					NetKeepAlive();
				}

				//wipestyleflags |= (WSF_FADEOUT|WSF_TOWHITE);
			}
		}
	}

	P_FreeLevelState();

	R_InitializeLevelInterpolators();

	P_InitThinkers();
	P_InitTIDHash();
	R_InitMobjInterpolators();
	P_InitCachedActions();

	K_ClearPersistentMessages();

	// internal game map
	maplumpname = mapheaderinfo[gamemap-1]->lumpname;
	lastloadedmaplumpnum = mapheaderinfo[gamemap-1]->lumpnum;
	if (lastloadedmaplumpnum == LUMPERROR)
		I_Error("Map %s not found.\n", maplumpname);

	curmapvirt = vres_GetMap(lastloadedmaplumpnum);

	if (mapheaderinfo[gamemap-1])
	{
		if (encoremode
#ifdef DEVELOP
				&& cv_kartencoremap.value
#endif
				)
		{
			encoreLump = vres_Find(curmapvirt, "ENCORE");
		}
		else
		{
			encoreLump = vres_Find(curmapvirt, "TWEAKMAP");
		}
	}

	if (encoreLump)
	{
		if (encoreLump->size != 256)
		{
			I_Error("%s: %s lump is not 256 bytes (actual size %s bytes)\n"
					"Make sure the lump is in DOOM Flat or SRB2 Encore format and 16x16",
					maplumpname, encoreLump->name, sizeu1(encoreLump->size));
		}

		R_ReInitColormaps(mapheaderinfo[gamemap-1]->palette, encoreLump->data, encoreLump->size);
	}
	else
	{
		R_ReInitColormaps(mapheaderinfo[gamemap-1]->palette, NULL, 0);
	}
	CON_SetupBackColormap();

	// SRB2 determines the sky texture to be used depending on the map header.
	P_SetupLevelSky(mapheaderinfo[gamemap-1]->skytexture, true);

	P_ResetSpawnpoints();

	P_ResetTubeWaypoints();

	P_MapStart(); // tm.thing can be used starting from this point

	// init anything that P_SpawnSlopes/P_LoadThings needs to know
	P_InitSpecials();

	P_InitSlopes(); //Initialize slopes before the map loads.

	if (!P_LoadMapFromFile())
	{
		TracyCZoneEnd(__zone);
		return false;
	}

	// set up world state
	// jart: needs to be done here so anchored slopes know the attached list
	P_SpawnSpecials(fromnetsave);

	P_SpawnSlopes(fromnetsave);

	P_SpawnMapThings(!fromnetsave);

	P_InitMinimapInfo();

	for (numcoopstarts = 0; numcoopstarts < MAXPLAYERS; numcoopstarts++)
		if (!playerstarts[numcoopstarts])
			break;

	P_SpawnSpecialsThatRequireObjects(fromnetsave);

	if (!udmf)
	{
		// Backwards compatibility for non-UDMF maps
		K_AdjustWaypointsParameters();

		if (P_CanWriteTextmap())
			P_WriteTextmapWaypoints();
	}

	if (!fromnetsave) //  ugly hack for P_NetUnArchiveMisc (and P_LoadNetGame)
		P_SpawnPrecipitation();

	// The waypoint data that's in PU_LEVEL needs to be reset back to 0/NULL now since PU_LEVEL was cleared
	K_ClearWaypoints();
	K_ClearFinishBeamLine();

	// Load the waypoints please!
	if (gametyperules & GTR_CIRCUIT && gamestate != GS_TITLESCREEN)
	{
		if (K_SetupWaypointList() == false)
		{
			CONS_Alert(CONS_ERROR, "Waypoints were not able to be setup! Player positions will not work correctly.\n");
		}

		if (K_GenerateFinishBeamLine() == false)
		{
			CONS_Alert(CONS_ERROR, "No valid finish line beam setup could be found.\n");
		}
	}

#ifdef HWRENDER // not win32 only 19990829 by Kin
	gl_maploaded = false;

	// Lactozilla: Free extrasubsectors regardless of renderer.
	HWR_FreeExtraSubsectors();

	// Create plane polygons.
	if (rendermode == render_opengl)
		HWR_LoadLevel();
#endif

	// oh god I hope this helps
	// (addendum: apparently it does!
	//  none of this needs to be done because it's not the beginning of the map when
	//  a netgame save is being loaded, and could actively be harmful by messing with
	//  the client's view of the data.)
	if (!fromnetsave)
	{
		P_InitGametype();

		// Initialize ACS scripts
		ACS_LoadLevelScripts(gamemap-1);
	}

	// Now safe to free.
	// We do the following silly
	// construction because vres_Free
	// no-sells deletions of pointers
	// that are == curmapvirt.
	{
		virtres_t *temp = curmapvirt;
		curmapvirt = NULL;
		vres_Free(temp);
	}

	if (!reloadinggamestate)
	{
		P_InitCamera();
		memset(localaiming, 0, sizeof(localaiming));
		K_InitDirector();
	}

	// Remove the loading shit from the screen
	if (rendermode != render_none && !titlemapinaction && !reloadinggamestate)
		F_WipeColorFill(levelfadecol);

	if (precache || dedicated)
		R_PrecacheLevel();

	if (!demo.playback)
	{
		mapheaderinfo[gamemap-1]->records.mapvisited |= MV_VISITED;

		M_UpdateUnlockablesAndExtraEmblems(true, true);
		G_SaveGameData();
	}

	oldbest = G_GetBestTime(gamemap - 1);

	P_MapEnd(); // tm.thing is no longer needed from this point onwards

	if (!udmf && !P_CanWriteTextmap())
	{
		// *Playing* binary maps is disabled; the support is kept in the code for binary map conversions only.
		// This is to make sure people use UDMF and, indirectly, guarantee that they will ship their PWAD
		// maps with extended GL nodes, as per UDMF requirement. We can retrofit a modern hardware renderer
		// into the game in the future without disrupting existing PWADs, which does not suffer from the issues
		// of the current GL renderer.
		I_Error("Playing binary maps is disabled; please convert to UDMF TEXTMAP and rebuild nodes.");
	}

	if (!(netgame || demo.playback))
	{
		// There is literally no reason to preserve ticcmds from a previous map
		// offline, even and especially if mindelay is involved.
		// Stop
		D_ResetTiccmds();
		memset(netcmds, 0, sizeof(netcmds)); // This is somehow actually necessary.
	}

	if (!fromnetsave)
	{
		INT32 buf = gametic % BACKUPTICS;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				G_CopyTiccmd(&players[i].cmd, &netcmds[buf][i], 1);
			}
		}

		for (i = 0; i <= r_splitscreen; i++)
		{
			postimgtype[i] = postimg_none;
		}

		if (marathonmode & MA_INGAME)
		{
			marathonmode = static_cast<marathonmode_t>(marathonmode | MA_INIT);
		}
	}
	else
	{
		// Don't run P_PostLoadLevel when loading netgames.
		levelloading = false;
	}

	if (rendermode != render_none && reloadinggamestate == false)
	{
		R_ResetViewInterpolation(0);
		R_UpdateMobjInterpolators();

		// Title card!
		G_StartTitleCard();

		// Can the title card actually run, though?
		if (WipeStageTitle && !fade_shortcircuit && fromnetsave == false)
		{
			G_PreLevelTitleCard();
		}

		// Apply FOV override.
		R_CheckFOV();
	}

	TracyCZoneEnd(__zone);
	return true;
}

void P_PostLoadLevel(void)
{
	TracyCZone(__zone, true);
	INT32 i;

	P_MapStart();

	if (G_GametypeHasSpectators())
	{
		K_CheckSpectateStatus(false);
	}

	if (demo.playback)
		;
	else if (grandprixinfo.gp == true)
	{
		if (savedata.lives > 0)
		{
			K_LoadGrandPrixSaveGame();
			savedata.lives = 0;
		}
		else if (grandprixinfo.initalize == true)
		{
			K_InitGrandPrixRank(&grandprixinfo.rank);
			K_InitGrandPrixBots();
			grandprixinfo.initalize = false;
		}
		else
		{
			K_UpdateGrandPrixBots();
			grandprixinfo.wonround = false;
		}
	}
	else
	{
		// We're in a Match Race, use simplistic randomized bots.
		K_UpdateMatchRaceBots();
	}

	if (roundqueue.snapshotmaps == true)
	{
		// Force spectator for snapshotmaps command
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
			{
				continue;
			}

			players[i].spectator = true;
		}
	}

	P_ShuffleTeams();

	K_TimerInit();

	P_InitPlayers();

	if (demo.recording) // Okay, level loaded, character spawned and skinned,
		G_BeginRecording(); // I AM NOW READY TO RECORD.
	demo.deferstart = true;

	nextmapoverride = 0;
	skipstats = 0;

	P_RunCachedActions();

	G_HandleSaveLevel(false);

	if (marathonmode & MA_INGAME)
	{
		marathonmode = static_cast<marathonmode_t>(marathonmode & ~MA_INIT);
	}

	Music_TuneReset(); // Placed before ACS scripts to allow remaps to occur on level start.

	ACS_RunLevelStartScripts();
	LUA_HookInt(gamemap, HOOK(MapLoad));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		if (players[i].spectator)
			continue;
		ACS_RunPlayerEnterScript(&players[i]);
	}

	P_MapEnd();

	// We're now done loading the level.
	levelloading = false;

	TracyCZoneEnd(__zone);
}

//
// P_RunSOC
//
// Runs a SOC file or a lump, depending on if ".SOC" exists in the filename
//
boolean P_RunSOC(const char *socfilename)
{
	lumpnum_t lump;

	if (strstr(socfilename, ".soc") != NULL)
		return P_AddWadFile(socfilename);

	lump = W_CheckNumForName(socfilename);
	if (lump == LUMPERROR)
		return false;

	CONS_Printf(M_GetText("Loading SOC lump: %s\n"), socfilename);
	DEH_LoadDehackedLump(lump);

	return true;
}

// Auxiliary function for PK3 loading - looks for sound replacements.
// NOTE: it does not really add any new sound entry or anything.
void P_LoadSoundsRange(UINT16 wadnum, UINT16 first, UINT16 num)
{
	size_t j;
	lumpinfo_t *lumpinfo = wadfiles[wadnum]->lumpinfo + first;
	for (; num > 0; num--, lumpinfo++)
	{
		// Let's check whether it's replacing an existing sound or it's a brand new one.
		for (j = 1; j < NUMSFX; j++)
		{
			if (S_sfx[j].name && !strnicmp(S_sfx[j].name, lumpinfo->name + 2, 6))
			{
				// the sound will be reloaded when needed,
				// since sfx->data will be NULL
				CONS_Debug(DBG_SETUP, "Sound %.8s replaced\n", lumpinfo->name);

				I_FreeSfx(&S_sfx[j]);
				break; // there shouldn't be two sounds with the same name, so stop looking
			}
		}
	}
}

// Auxiliary function for PK3 loading - looks for music and music replacements.
// NOTE: does nothing but print debug messages. The code is handled somewhere else.
void P_LoadMusicsRange(UINT16 wadnum, UINT16 first, UINT16 num)
{
	lumpinfo_t *lumpinfo = wadfiles[wadnum]->lumpinfo + first;
	char *name;
	for (; num > 0; num--, lumpinfo++)
	{
		name = lumpinfo->name;
		if (name[0] == 'O' && name[1] == '_')
		{
			CONS_Debug(DBG_SETUP, "Music %.8s replaced\n", name);
		}
		else if (name[0] == 'D' && name[1] == '_')
		{
			CONS_Debug(DBG_SETUP, "Music %.8s replaced\n", name);
		}
	}
	return;
}

// Auxiliary function - input a folder name and gives us the resource markers positions.
static lumpinfo_t* FindFolder(const char *folName, UINT16 *start, UINT16 *end, lumpinfo_t *lumpinfo, UINT16 *pnumlumps, size_t *pi)
{
	UINT16 numlumps = *pnumlumps;
	size_t i = *pi;
	if (!stricmp(lumpinfo->fullname, folName))
	{
		lumpinfo++;
		*start = ++i;
		for (; i < numlumps; i++, lumpinfo++)
			if (strnicmp(lumpinfo->fullname, folName, strlen(folName)))
				break;
		lumpinfo--;
		*end = i-- - *start;
		*pi = i;
		*pnumlumps = numlumps;
		return lumpinfo;
	}
	return lumpinfo;
}

static tic_t round_to_next_second(tic_t time)
{
	return static_cast<tic_t>(std::ceil(time / static_cast<float>(TICRATE)) * TICRATE);
}

static void P_DeriveAutoMedalTimes(mapheader_t& map)
{
	// Gather staff ghost times
	srb2::Vector<tic_t> stafftimes;
	for (int i = 0; i < map.ghostCount; i++)
	{
		tic_t time = map.ghostBrief[i]->time;
		if (time <= 0)
		{
			continue;
		}

		stafftimes.push_back(map.ghostBrief[i]->time);
	}

	if (stafftimes.empty())
	{
		// Use fallback times
		map.automedaltime[0] = 1;
		map.automedaltime[1] = 2;
		map.automedaltime[2] = 3;
		map.automedaltime[3] = 4;
		return;
	}

	std::sort(stafftimes.begin(), stafftimes.end());

	// Auto Platinum is the best staff ghost time
	// Auto Gold is the median staff ghost time or 10% longer than Platinum, whichever is higher
	// Silver and Bronze are 10% longer and 10% longer successively
	// Gold, Silver and Bronze are rounded up to the next second

	tic_t best = stafftimes.at(0);
	tic_t gold = std::max(
		round_to_next_second(stafftimes.at(stafftimes.size() / 2)),
		round_to_next_second(static_cast<tic_t>(std::ceil(best * 1.1f)))
	);
	if (gold == best)
	{
		gold += 1;
	}
	tic_t silver = round_to_next_second(static_cast<tic_t>(std::ceil(gold * 1.1f)));
	if (silver == gold)
	{
		silver += 1;
	}
	tic_t bronze = round_to_next_second(static_cast<tic_t>(std::ceil(silver * 1.1f)));
	if (bronze == silver)
	{
		bronze += 1;
	}

	map.automedaltime[0] = best;
	map.automedaltime[1] = gold;
	map.automedaltime[2] = silver;
	map.automedaltime[3] = bronze;
}

lumpnum_t wadnamelump = LUMPERROR;
INT16 wadnamemap = 0; // gamemap based

// Initialising map data (and catching replacements)...
UINT8 P_InitMapData(void)
{
	UINT8 ret = 0;
	INT32 i, j;
	lumpnum_t maplump;
	virtres_t *virtmap;
	virtlump_t *minimap, *thumbnailPic;
	char *name;

	for (i = 0; i < nummapheaders; ++i)
	{
		name = mapheaderinfo[i]->lumpname;
		maplump = W_CheckNumForMap(name, (mapheaderinfo[i]->lumpnum == LUMPERROR));

		// Always check for cup cache reassociations.
		// (The core assumption is that cups < headers.)
		if (maplump != LUMPERROR || mapheaderinfo[i]->lumpnum != LUMPERROR)
		{
			cupheader_t *cup = kartcupheaders;

			while (cup)
			{
				for (j = 0; j < CUPCACHE_MAX; j++)
				{
					// No level in this slot?
					if (!cup->levellist[j])
						continue;

					// Already discovered?
					if (cup->cachedlevels[j] != NEXTMAP_INVALID)
						continue;

					// Not your name?
					if (strcasecmp(cup->levellist[j], name) != 0)
						continue;

					// Have a map recognise the first cup it's a part of.
					if (!mapheaderinfo[i]->cup)
						mapheaderinfo[i]->cup = cup;

					cup->cachedlevels[j] = i;
				}
				cup = cup->next;
			}
		}

		// Doesn't exist in this set of files?
		if (maplump == LUMPERROR)
		{
#ifndef DEVELOP
			if (!basenummapheaders)
			{
				I_Error("P_InitMapData: Base map %s has a header but no level\n", name);
			}
#endif
			continue;
		}

		// No change?
		if (mapheaderinfo[i]->lumpnum == maplump)
			continue;

		// Okay, it does...
		{
			ret |= MAPRET_ADDED;

			if (basenummapheaders)
			{
				CONS_Printf("%s\n", name);

				if (mapheaderinfo[i]->lumpnum != LUMPERROR)
				{
					G_SetGameModified(multiplayer, true); // oops, double-defined - no record attack privileges for you

					//If you replaced the map you're on, end the level when done.
					if (i == gamemap - 1)
						ret |= MAPRET_CURRENTREPLACED;
				}
			}

			mapheaderinfo[i]->lumpnum = maplump;

			if (maplump == wadnamelump)
				wadnamemap = i+1;

			// Get map thumbnail and minimap
			virtmap = vres_GetMap(mapheaderinfo[i]->lumpnum);
			thumbnailPic = vres_Find(virtmap, "PICTURE");
			minimap = vres_Find(virtmap, "MINIMAP");

			// Clear out existing graphics...
			if (mapheaderinfo[i]->thumbnailPic)
			{
				Patch_Free(static_cast<patch_t*>(mapheaderinfo[i]->thumbnailPic));
				mapheaderinfo[i]->thumbnailPic = NULL;
			}

			if (mapheaderinfo[i]->minimapPic)
			{
				Patch_Free(static_cast<patch_t*>(mapheaderinfo[i]->minimapPic));
				mapheaderinfo[i]->minimapPic = NULL;
			}

			// Now apply the new ones!
			if (thumbnailPic != NULL)
			{
				mapheaderinfo[i]->thumbnailPic = vres_GetPatch(thumbnailPic, PU_STATIC);
			}

			if (minimap != NULL)
			{
				mapheaderinfo[i]->minimapPic = vres_GetPatch(minimap, PU_STATIC);
			}

			// Staff ghosts.
			// The trouble with staff ghosts is that they're too large to cache.
			// So we store extra information about them, and load later.
			while (mapheaderinfo[i]->ghostCount > 0)
			{
				mapheaderinfo[i]->ghostCount--;

				Z_Free(mapheaderinfo[i]->ghostBrief[mapheaderinfo[i]->ghostCount]);
				mapheaderinfo[i]->ghostBrief[mapheaderinfo[i]->ghostCount] = NULL;
			}

			for (INT32 wadindex = 0; wadindex < numwadfiles; wadindex++)
			{
				if (wadfiles[wadindex]->type != RET_PK3)
				{
					continue;
				}
				srb2::String ghostdirname = srb2::format("staffghosts/{}/", mapheaderinfo[i]->lumpname);

				UINT16 lumpstart = W_CheckNumForFolderStartPK3(ghostdirname.c_str(), wadindex, 0);
				if (lumpstart == INT16_MAX)
				{
					continue;
				}
				UINT16 lumpend = W_CheckNumForFolderEndPK3(ghostdirname.c_str(), wadindex, lumpstart);
				if (lumpend == INT16_MAX)
				{
					continue;
				}

				for (UINT16 lumpnum = lumpstart; lumpnum < lumpend; lumpnum++)
				{
					if (W_IsLumpFolder(wadindex, lumpnum))
					{
						continue;
					}

					size_t lumplength = W_LumpLengthPwad(wadindex, lumpnum);
					UINT8* ghostdata = static_cast<UINT8*>(Z_Malloc(lumplength, PU_STATIC, nullptr));
					auto ghostdata_finalizer = srb2::finally([=]() { Z_Free(ghostdata); });

					W_ReadLumpPwad(wadindex, lumpnum, ghostdata);
					staffbrief_t* briefghost = G_GetStaffGhostBrief(ghostdata);
					if (briefghost == nullptr)
					{
						continue;
					}
					briefghost->wad = wadindex;
					briefghost->lump = lumpnum;

					// Resize ghostBrief if needed
					if (mapheaderinfo[i]->ghostBriefSize < static_cast<UINT32>(mapheaderinfo[i]->ghostCount + 1))
					{
						UINT32 newsize = mapheaderinfo[i]->ghostBriefSize + 4;
						mapheaderinfo[i]->ghostBrief = static_cast<staffbrief_t**>(Z_Realloc(mapheaderinfo[i]->ghostBrief, sizeof(staffbrief_t*) * newsize, PU_STATIC, NULL));
						mapheaderinfo[i]->ghostBriefSize = newsize;
					}
					mapheaderinfo[i]->ghostBrief[mapheaderinfo[i]->ghostCount] = briefghost;
					mapheaderinfo[i]->ghostCount++;
				}

				// Sort shortest to longest for Time Attack menu
				std::sort(
					mapheaderinfo[i]->ghostBrief,
					mapheaderinfo[i]->ghostBrief + mapheaderinfo[i]->ghostCount,
					[](staffbrief_t* a, staffbrief_t* b) { return a->time < b->time; }
				);
			}

			P_DeriveAutoMedalTimes(*mapheaderinfo[i]);

			vres_Free(virtmap);
		}
	}

	return ret;
}

void Command_dumprrautomedaltimes(void)
{
	FILE* out;
	char outname[512];
	memset(outname, 0, sizeof(outname));
	sprintf(outname, "%s/rrautomedaltimes.csv", srb2home);
	out = fopen(outname, "wb");
	if (out == NULL)
	{
		CONS_Alert(CONS_ERROR, "Failed to dump rrautomedaltimes.csv\n");
		return;
	}
	fprintf(out, "Name,Bronze,Silver,Gold,Platinum\n");
	for (int i = 0; i < nummapheaders; ++i)
	{
		fprintf(out, "%s,", mapheaderinfo[i]->lvlttl);
		fprintf(
			out,
			"%d'%d\"%02d,",
			G_TicsToMinutes(mapheaderinfo[i]->automedaltime[3], true),
			G_TicsToSeconds(mapheaderinfo[i]->automedaltime[3]),
			G_TicsToCentiseconds(mapheaderinfo[i]->automedaltime[3])
		);
		fprintf(
			out,
			"%d'%d\"%02d,",
			G_TicsToMinutes(mapheaderinfo[i]->automedaltime[2], true),
			G_TicsToSeconds(mapheaderinfo[i]->automedaltime[2]),
			G_TicsToCentiseconds(mapheaderinfo[i]->automedaltime[2])
		);
		fprintf(
			out,
			"%d'%d\"%02d,",
			G_TicsToMinutes(mapheaderinfo[i]->automedaltime[1], true),
			G_TicsToSeconds(mapheaderinfo[i]->automedaltime[1]),
			G_TicsToCentiseconds(mapheaderinfo[i]->automedaltime[1])
		);
		fprintf(
			out,
			"%d'%d\"%02d\n",
			G_TicsToMinutes(mapheaderinfo[i]->automedaltime[0], true),
			G_TicsToSeconds(mapheaderinfo[i]->automedaltime[0]),
			G_TicsToCentiseconds(mapheaderinfo[i]->automedaltime[0])
		);
	}
	fclose(out);
	CONS_Printf("Medal times written to %s\n", outname);
}

void Command_Platinums(void)
{
	srb2::Vector<std::string> platinums;

	for (INT32 j = 0; j < nummapheaders; j++)
	{
		mapheader_t *map = mapheaderinfo[j];

		if (map == NULL || map->ghostCount < 1)
			continue;

		srb2::Vector<std::pair<tic_t, std::string>> stafftimes;
		for (int i = 0; i < map->ghostCount; i++)
		{
			tic_t time = map->ghostBrief[i]->time;
			if (time <= 0)
			{
				continue;
			}

			stafftimes.push_back(std::make_pair(map->ghostBrief[i]->time, map->ghostBrief[i]->name));
		}

		if (stafftimes.empty())
		{
			continue;
		}

		std::sort(stafftimes.begin(), stafftimes.end(), [](auto &left, auto &right) {
			return left.first < right.first;
		});

		CONS_Printf("%s: ", map->lumpname);

		tic_t platinumtime = UINT32_MAX;

		std::unordered_map<std::string, int> names;

		for (auto &stafftime : stafftimes)
		{
			if (stafftime == stafftimes[0])
			{
				CONS_Printf("%s (%02d:%02d:%02d)", stafftime.second.c_str(),
					G_TicsToMinutes(stafftime.first, true), G_TicsToSeconds(stafftime.first), G_TicsToCentiseconds(stafftime.first));
				platinumtime = stafftime.first;
			}
			else
			{
				CONS_Printf(", %s (+%d:%02d:%02d)", stafftime.second.c_str(),
					G_TicsToMinutes(stafftime.first - platinumtime, true),
					G_TicsToSeconds(stafftime.first - platinumtime),
					G_TicsToCentiseconds(stafftime.first - platinumtime));
			}

			names[stafftime.second]++;
			if (names[stafftime.second] > 1)
			{
				CONS_Printf(" (DUPLICATE)");
			}
		}

		CONS_Printf("\n");

		platinums.push_back(stafftimes[0].second);
	}

	std::unordered_map<std::string, int> frequency;

    for (const auto& platinum : platinums)
	{
        frequency[platinum]++;
    }

    for (const auto& pair : frequency)
	{
		CONS_Printf("%s: %d - ", pair.first.c_str(), pair.second);

		for (INT32 j = 0; j < nummapheaders; j++)
		{
			mapheader_t *map = mapheaderinfo[j];

			if (map == NULL || map->ghostCount < 1)
				continue;

			// Gather staff ghost times
			srb2::Vector<tic_t> stafftimes;
			for (int i = 0; i < map->ghostCount; i++)
			{
				tic_t time = map->ghostBrief[i]->time;
				if (time <= 0)
				{
					continue;
				}

				stafftimes.push_back(map->ghostBrief[i]->time);
			}

			if (stafftimes.empty())
			{
				continue;
			}

			std::sort(stafftimes.begin(), stafftimes.end());

			for (int i = 0; i < map->ghostCount; i++)
			{
				if (strcmp(pair.first.c_str(), map->ghostBrief[i]->name))
					break;

				tic_t time = map->ghostBrief[i]->time;
				if (time == stafftimes.at(0))
				{
					CONS_Printf("%s, ", map->lumpname);
					break;
				}
			}
		}

		CONS_Printf("\n");
    }
}

//
// Add a wadfile to the active wad files,
// replace sounds, musics, patches, textures, sprites and maps
//
boolean P_AddWadFile(const char *wadfilename)
{
	UINT16 wadnum;

	if ((wadnum = P_PartialAddWadFile(wadfilename)) == UINT16_MAX)
		return false;

	if (P_PartialAddGetStage() >= 0)
		P_MultiSetupWadFiles(true);

	return true;
}

//
// Add a WAD file and do the per-WAD setup stages.
// Call P_MultiSetupWadFiles as soon as possible after any number of these.
//
UINT16 P_PartialAddWadFile(const char *wadfilename)
{
	size_t i, j, sreplaces = 0, mreplaces = 0, digmreplaces = 0;
	UINT16 numlumps, wadnum;
	char *name;
	lumpinfo_t *lumpinfo;

	// Vars to help us with the position start and amount of each resource type.
	// Useful for PK3s since they use folders.
	// WADs use markers for some resources, but others such as sounds are checked lump-by-lump anyway.
//	UINT16 luaPos, luaNum = 0;
//	UINT16 socPos, socNum = 0;
	UINT16 sfxPos = 0, sfxNum = 0;
	UINT16 musPos = 0, musNum = 0;
//	UINT16 sprPos, sprNum = 0;
	UINT16 texPos = 0, texNum = 0;
//	UINT16 patPos, patNum = 0;
//	UINT16 flaPos, flaNum = 0;
//	UINT16 mapPos, mapNum = 0;

	// Init file.
	if ((numlumps = W_InitFile(wadfilename, false, false, nullptr)) == INT16_MAX)
	{
		refreshdirmenu |= REFRESHDIR_NOTLOADED;
		return false;
	}

	wadnum = (UINT16)(numwadfiles-1);

	// Init partadd.
	if (wadfiles[wadnum]->important)
	{
		partadd_important = true;
	}
	if (partadd_stage != 0)
	{
		partadd_earliestfile = wadnum;
	}
	partadd_stage = 0;

	switch(wadfiles[wadnum]->type)
	{
	case RET_PK3:
		// Look for the lumps that act as resource delimitation markers.
		lumpinfo = wadfiles[wadnum]->lumpinfo;
		for (i = 0; i < numlumps; i++, lumpinfo++)
		{
//			lumpinfo = FindFolder("Lua/",      &luaPos, &luaNum, lumpinfo, &numlumps, &i);
//			lumpinfo = FindFolder("SOC/",      &socPos, &socNum, lumpinfo, &numlumps, &i);
			lumpinfo = FindFolder("Sounds/",   &sfxPos, &sfxNum, lumpinfo, &numlumps, &i);
			lumpinfo = FindFolder("Music/",    &musPos, &musNum, lumpinfo, &numlumps, &i);
//			lumpinfo = FindFolder("Sprites/",  &sprPos, &sprNum, lumpinfo, &numlumps, &i);
			lumpinfo = FindFolder("Textures/", &texPos, &texNum, lumpinfo, &numlumps, &i);
//			lumpinfo = FindFolder("Patches/",  &patPos, &patNum, lumpinfo, &numlumps, &i);
//			lumpinfo = FindFolder("Flats/",    &flaPos, &flaNum, lumpinfo, &numlumps, &i);
//			lumpinfo = FindFolder("Maps/",     &mapPos, &mapNum, lumpinfo, &numlumps, &i);
		}

		// Update the detected resources.
		// Note: ALWAYS load Lua scripts first, SOCs right after, and the remaining resources afterwards.
//		if (luaNum) // Lua scripts.
//			P_LoadLuaScrRange(wadnum, luaPos, luaNum);
//		if (socNum) // SOCs.
//			P_LoadDehackRange(wadnum, socPos, socNum);
		if (sfxNum) // Sounds. TODO: Function currently only updates already existing sounds, the rest is handled somewhere else.
			P_LoadSoundsRange(wadnum, sfxPos, sfxNum);
		if (musNum) // Music. TODO: Useless function right now.
			P_LoadMusicsRange(wadnum, musPos, musNum);
//		if (sprNum) // Sprites.
//			R_LoadSpritsRange(wadnum, sprPos, sprNum);
//		if (texNum) // Textures. TODO: R_LoadTextures() does the folder positioning once again. New function maybe?
//			R_LoadTextures();
		break;
	default:
		lumpinfo = wadfiles[wadnum]->lumpinfo;
		for (i = 0; i < numlumps; i++, lumpinfo++)
		{
			name = lumpinfo->name;
			if (name[0] == 'D')
			{
				if (name[1] == 'S')
				{
					for (j = 1; j < NUMSFX; j++)
					{
						if (S_sfx[j].name && !strnicmp(S_sfx[j].name, name + 2, 6))
						{
							// the sound will be reloaded when needed,
							// since sfx->data will be NULL
							CONS_Debug(DBG_SETUP, "Sound %.8s replaced\n", name);

							I_FreeSfx(&S_sfx[j]);

							sreplaces++;
							break; // there shouldn't be two sounds with the same name, so stop looking
						}
					}
				}
				else if (name[1] == '_')
				{
					CONS_Debug(DBG_SETUP, "Music %.8s ignored\n", name);
					mreplaces++;
				}
			}
			else if (name[0] == 'O' && name[1] == '_')
			{
				CONS_Debug(DBG_SETUP, "Music %.8s replaced\n", name);
				digmreplaces++;
			}
		}
		break;
	}
	if (!devparm && sreplaces)
		CONS_Printf(M_GetText("%s sounds replaced\n"), sizeu1(sreplaces));
	if (!devparm && mreplaces)
		CONS_Printf(M_GetText("%s midi musics ignored\n"), sizeu1(mreplaces));
	if (!devparm && digmreplaces)
		CONS_Printf(M_GetText("%s digital musics replaced\n"), sizeu1(digmreplaces));

#ifdef HWRENDER
	// Free GPU textures before freeing patches.
	if (rendermode == render_opengl && (vid.glstate == VID_GL_LIBRARY_LOADED))
		HWR_ClearAllTextures();
#endif

	//
	// search for sprite replacements
	//
	Patch_FreeTag(PU_SPRITE);
	Patch_FreeTag(PU_PATCH_ROTATED);
	R_AddSpriteDefs(wadnum);

	// Reload it all anyway, just in case they
	// added some textures but didn't insert a
	// TEXTURES/etc. list.
	R_LoadTexturesPwad(wadnum); // numtexture changes

	// Reload BRIGHT
	K_InitBrightmapsPwad(wadnum);

	//
	// look for skins
	//
	R_AddSkins(wadnum, false); // faB: wadfile index in wadfiles[]
	R_PatchSkins(wadnum, false); // toast: PATCH PATCH

	//
	// edit music defs
	//
	S_LoadMusicDefs(wadnum);

	//
	// extra sprite/skin data
	//
	R_LoadSpriteInfoLumps(wadnum, numlumps);

	// For anything that has to be done over every wadfile at once, see P_MultiSetupWadFiles.

	refreshdirmenu &= ~REFRESHDIR_GAMEDATA; // Under usual circumstances we'd wait for REFRESHDIR_ flags to disappear the next frame, but this one's a bit too dangerous for that...

	return true;
}

// Only exists to make sure there's no way to overwrite partadd_stage externally
// unless you really push yourself.
SINT8 P_PartialAddGetStage(void)
{
	return partadd_stage;
}

//
// Set up a series of partially added WAD files.
// Setup functions that iterate over every loaded WAD go here.
// If fullsetup false, only do one stage per call.
//
boolean P_MultiSetupWadFiles(boolean fullsetup)
{
	if (partadd_stage < 0)
		I_Error(M_GetText("P_MultiSetupWadFiles: Post-load addon setup attempted without loading any addons first"));

	if (partadd_stage == 0)
	{
		// Flush and reload HUD graphics
		//ST_UnloadGraphics();
		HU_LoadGraphics();
		ST_LoadGraphics();
		ST_ReloadSkinFaceGraphics();

		if (!partadd_important)
			partadd_stage = -1; // everything done
		else if (fullsetup)
			partadd_stage++;
	}

	if (partadd_stage == 1)
	{
		// Prevent savefile cheating
		if (cursaveslot >= 0)
			cursaveslot = 0;

		// Reload ANIMATED / ANIMDEFS
		P_InitPicAnims();

		// reload status bar (warning should have valid player!)
		if (gamestate == GS_LEVEL)
			ST_Start();

#ifdef HWRENDER
		HWR_ReloadModels();
#endif

		if (fullsetup)
			partadd_stage++;
	}

	if (partadd_stage == 2)
	{
		UINT8 mapsadded = P_InitMapData();

		if (!mapsadded)
			CONS_Printf(M_GetText("No maps added\n"));

		if ((mapsadded & MAPRET_CURRENTREPLACED)
			&& (gamestate == GS_LEVEL)
			&& (netgame || multiplayer))
		{
			CONS_Printf(M_GetText("Current map %d replaced by added file, ending the level to ensure consistency.\n"), gamemap);
			if (server)
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
		}

		//if (fullsetup)
			//partadd_stage++;
		partadd_stage = -1;
	}

	I_Assert(!fullsetup || partadd_stage < 0);

	if (partadd_stage < 0)
	{
		// possible future work: only do this if musicdefs/map headers changed
		S_PopulateSoundTestSequence();

		partadd_important = false;
		partadd_earliestfile = UINT16_MAX;
		return true;
	}

	partadd_stage++;
	return false;
}

//
// Let's see if this works
//
void P_ReduceVFXTextureReload(void)
{
	P_InitPicAnims();

	if (!G_GamestateUsesLevel())
		return;

	P_SetupLevelFlatAnims();
}

// Let's see if *this* works
extern "C" void ReduceVFX_OnChange(void);
void ReduceVFX_OnChange(void)
{
	if (con_startup_loadprogress < LOADED_CONFIG)
		return;
	P_ReduceVFXTextureReload();
}

