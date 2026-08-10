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
/// \file  w_wad.h
/// \brief WAD I/O functions, wad resource definitions (some)

#ifndef W_WAD_H
#define W_WAD_H

#ifdef HWRENDER
#include "hardware/hw_data.h"
#endif

#include "k_modinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

// a raw entry of the wad directory
// NOTE: This sits here and not in w_wad.c because p_setup.c makes use of it to load map WADs inside PK3s.
#if defined(_MSC_VER)
#pragma pack(1)
#endif
struct filelump_t
{
	uint32_t filepos; // file offset of the resource
	uint32_t size; // size of the resource
	char name[8]; // name of the resource
} ATTRPACK;
#if defined(_MSC_VER)
#pragma pack()
#endif


// ==============================================================
//               WAD FILE STRUCTURE DEFINITIONS
// ==============================================================

// header of a wad file
struct wadinfo_t
{
	char identification[4]; // should be "IWAD" or "PWAD"
	uint32_t numlumps; // how many resources
	uint32_t infotableofs; // the 'directory' of resources
};

// Available compression methods for lumps.
typedef enum
{
	CM_NOCOMPRESSION,
#ifdef HAVE_ZLIB
	CM_DEFLATE,
#endif
	CM_LZF,
	CM_UNSUPPORTED
} compmethod;

//  a memory entry of the wad directory
struct lumpinfo_t
{
	unsigned long position; // filelump_t filepos
	unsigned long disksize; // filelump_t size
	char name[9];           // filelump_t name[] e.g. "LongEntr"
	uint32_t hash;
	char *longname;         //                   e.g. "LongEntryName"
	char *fullname;         //                   e.g. "Folder/Subfolder/LongEntryName.extension"
	size_t size;            // real (uncompressed) size
	compmethod compression; // lump compression method
};

// =========================================================================
//                         'VIRTUAL' RESOURCES
// =========================================================================

struct virtlump_t {
	char name[9];
	uint8_t* data;
	size_t size;
};

struct virtres_t {
	size_t numlumps;
	virtlump_t* vlumps;
};

virtres_t* vres_GetMap(lumpnum_t);
void vres_Free(virtres_t*);
virtlump_t* vres_Find(const virtres_t*, const char*);
void* vres_GetPatch(virtlump_t *vlump, int32_t tag);

// =========================================================================
//                         DYNAMIC WAD LOADING
// =========================================================================

#define MAX_WADPATH 512
#define MAX_WADFILES 255 // maximum of wad files used at the same time
// Replay code relies on it being an uint8_t. There are no SINT8s handling WAD indices, though.
// Can be set all the way up to 255 but not 256,
// because an uint8_t will never be >= 256, probably breaking some conditionals.

#define lumpcache_t void *

// Resource type of the WAD. Yeah, I know this sounds dumb, but I'll leave it like this until I clean up the code further.
typedef enum restype
{
	RET_WAD,
	RET_SOC,
	RET_LUA,
	RET_PK3,
	RET_UNKNOWN,
} restype_t;

struct wadfile_t
{
	char *filename;
	restype_t type;
	lumpinfo_t *lumpinfo;
	lumpcache_t *lumpcache;
	lumpcache_t *patchcache;
	uint16_t numlumps; // this wad's number of resources
	FILE *handle;
	uint32_t filesize; // for network
	uint8_t md5sum[16];

	dboolean important; // also network - !W_VerifyNMUSlumps

	mod_metadata_t *metadata;
};

#define WADFILENUM(lumpnum) (uint16_t)((lumpnum)>>16) // wad flumpnum>>16) // wad file number in upper word
#define LUMPNUM(lumpnum) (uint16_t)((lumpnum)&0xFFFF) // lump number for this pwad

extern uint16_t numwadfiles;
extern wadfile_t *wadfiles[MAX_WADFILES];

// =========================================================================

void W_Shutdown(void);

// Opens a WAD file. Returns the FILE * handle for the file, or NULL if not found or could not be opened
FILE *W_OpenWadFile(const char **filename, const char *priorityfolder, dboolean useerrors);
// Load and add a wadfile to the active wad files, returns numbers of lumps, INT16_MAX on error
uint16_t W_InitFile(const char *filename, dboolean mainfile, dboolean startup, const char *md5expected);

typedef struct initmultiplefilesentry_t
{
	const char *filename;
	const char *md5sum;
} initmultiplefilesentry_t;
// W_InitMultipleFiles returns 1 if all is okay, 0 otherwise,
// so that it stops with a message if a file was not found, but not if all is okay.
// W_InitMultipleFiles exits if a file was not found, but not if all is okay.
int32_t W_InitMultipleFiles(const initmultiplefilesentry_t *entries, int32_t count, dboolean addons);

const char *W_CheckNameForNumPwad(uint16_t wad, uint16_t lump);
const char *W_CheckNameForNum(lumpnum_t lumpnum);
const char *W_CheckLongNameForNumPwad(uint16_t wad, uint16_t lump);
const char *W_CheckLongNameForNum(lumpnum_t lumpnum);

uint16_t W_FindNextEmptyInPwad(uint16_t wad, uint16_t startlump); // checks only in one pwad

uint16_t W_CheckNumForMapPwad(const char *name, uint32_t hash, uint16_t wad, uint16_t startlump);
uint16_t W_CheckNumForNamePwad(const char *name, uint16_t wad, uint16_t startlump); // checks only in one pwad
uint16_t W_CheckNumForLongNamePwad(const char *name, uint16_t wad, uint16_t startlump);

/* Find the first lump after F_START for instance. */
uint16_t W_CheckNumForMarkerStartPwad(const char *name, uint16_t wad, uint16_t startlump);

uint16_t W_CheckNumForFullNamePK3(const char *name, uint16_t wad, uint16_t startlump);
uint16_t W_CheckNumForFolderStartPK3(const char *name, uint16_t wad, uint16_t startlump);
uint16_t W_CheckNumForFolderEndPK3(const char *name, uint16_t wad, uint16_t startlump);

lumpnum_t W_CheckNumForMap(const char *name, dboolean checktofirst);
lumpnum_t W_CheckNumForName(const char *name);
lumpnum_t W_CheckNumForLongName(const char *name);
lumpnum_t W_GetNumForName(const char *name); // like W_CheckNumForName but I_Error on LUMPERROR
lumpnum_t W_GetNumForLongName(const char *name);
lumpnum_t W_CheckNumForNameInBlock(const char *name, const char *blockstart, const char *blockend);
lumpnum_t W_CheckNumForNameInFolder(const char *lump, const char *folder);
uint8_t W_LumpExists(const char *name); // Lua uses this.

lumpnum_t W_LumpIntoNetSave(lumpnum_t lump);
lumpnum_t W_LumpFromNetSave(lumpnum_t lump);

size_t W_LumpLengthPwad(uint16_t wad, uint16_t lump);
size_t W_LumpLength(lumpnum_t lumpnum);

dboolean W_IsLumpWad(lumpnum_t lumpnum); // for loading maps from WADs in PK3s
dboolean W_IsLumpFolder(uint16_t wad, uint16_t lump); // for detecting folder "lumps"

#ifdef HAVE_ZLIB
void zerr(int ret); // zlib error checking
#endif

size_t W_ReadLumpHeaderPwad(uint16_t wad, uint16_t lump, void *dest, size_t size, size_t offset);
size_t W_ReadLumpHeader(lumpnum_t lump, void *dest, size_t size, size_t offest); // read all or a part of a lump
void W_ReadLumpPwad(uint16_t wad, uint16_t lump, void *dest);
void W_ReadLump(lumpnum_t lump, void *dest);

void *W_CacheLumpNumPwad(uint16_t wad, uint16_t lump, int32_t tag);
void *W_CacheLumpNum(lumpnum_t lump, int32_t tag);
void *W_CacheLumpNumForce(lumpnum_t lumpnum, int32_t tag);

dboolean W_IsLumpCached(lumpnum_t lump, void *ptr);
dboolean W_IsPatchCached(lumpnum_t lump, void *ptr);

void *W_CacheLumpName(const char *name, int32_t tag);
void *W_CachePatchName(const char *name, int32_t tag);
void *W_CachePatchLongName(const char *name, int32_t tag);

// Returns either a Software patch, or an OpenGL patch.
// Performs any necessary conversions from PNG images.
void *W_CachePatchNumPwad(uint16_t wad, uint16_t lump, int32_t tag);
void *W_CachePatchNum(lumpnum_t lumpnum, int32_t tag);

// Returns a Software patch.
// Performs any necessary conversions from PNG images.
void *W_CacheSoftwarePatchNumPwad(uint16_t wad, uint16_t lump, int32_t tag);
void *W_CacheSoftwarePatchNum(lumpnum_t lumpnum, int32_t tag);

void W_UnlockCachedPatch(void *patch);

int W_VerifyNMUSlumps(const char *filename, FILE *handle, dboolean exit_on_error);

/// Initialize non-legacy GL shader lookup, which lives outside the lump management system.
void W_InitShaderLookup(const char *filename);
dboolean W_ReadShader(const char *filename, size_t *size, void *dest);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // W_WAD_H
