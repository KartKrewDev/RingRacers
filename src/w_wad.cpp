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
/// \file  w_wad.c
/// \brief Handles WAD file header, directory, lump I/O

#ifdef HAVE_ZLIB
#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

#include <zlib.h>
#endif

#ifdef __GNUC__
#include <unistd.h>
#endif

#define ZWAD

#ifdef ZWAD
#include <errno.h>
#include "lzf.h"
#endif

#include <algorithm>
#include <cstddef>

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"

#include "w_wad.h"
#include "z_zone.h"
#include "fastcmp.h"

#include "g_game.h" // G_LoadGameData
#include "m_cond.h" // gamedata itself
#include "filesrch.h"

#include "i_video.h" // rendermode
#include "d_netfil.h"
#include "deh_soc.h"
#include "d_clisrv.h"
#include "r_defs.h"
#include "r_data.h"
#include "r_textures.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "i_time.h"
#include "i_system.h"
#include "md5.h"
#include "lua_script.h"
#include "g_game.h" // G_SetGameModified
#include "d_main.h"

#include "k_terrain.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#include "hardware/hw_glob.h"
#endif

#ifdef _DEBUG
#include "console.h"
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif


typedef struct
{
	const char *name;
	size_t len;
} lumpchecklist_t;

// Must be a power of two
#define LUMPNUMCACHESIZE 64
#define LUMPNUMCACHENAME 32

typedef struct lumpnum_cache_s
{
	char lumpname[LUMPNUMCACHENAME];
	UINT32 lumphash;
	lumpnum_t lumpnum;
} lumpnum_cache_t;

static lumpnum_cache_t lumpnumcache[LUMPNUMCACHESIZE];
static UINT16 lumpnumcacheindex = 0;

//===========================================================================
//                                                                    GLOBALS
//===========================================================================
UINT16 numwadfiles = 0; // number of active wadfiles
wadfile_t *wadfiles[MAX_WADFILES]; // 0 to numwadfiles-1 are valid

static FILE *g_shaderspk3file;
static UINT16 g_shaderspk3numlumps;
static lumpinfo_t *g_shaderspk3lumps;

#ifndef NOMD5
static void PrintMD5String(const UINT8 *md5, char *buf);
#endif

// W_Shutdown
// Closes all of the WAD files before quitting
// If not done on a Mac then open wad files
// can prevent removable media they are on from
// being ejected
void W_Shutdown(void)
{
	while (numwadfiles--)
	{
		wadfile_t *wad = wadfiles[numwadfiles];

		fclose(wad->handle);
		Z_Free(wad->filename);
		while (wad->numlumps--)
		{
			Z_Free(wad->lumpinfo[wad->numlumps].longname);
			if (wad->lumpinfo[wad->numlumps].fullname != wad->lumpinfo[wad->numlumps].longname)
			{
				Z_Free(wad->lumpinfo[wad->numlumps].fullname);
			}
		}

		Z_Free(wad->lumpinfo);
		Z_Free(wad);
	}

	// Cleanup the separate shader lookup
	if (g_shaderspk3file)
	{
		while (g_shaderspk3numlumps--)
		{
			lumpinfo_t *lump = &g_shaderspk3lumps[g_shaderspk3numlumps];
			Z_Free(lump->longname);
			if (lump->fullname != lump->longname)
			{
				Z_Free(lump->fullname);
			}
		}
		Z_Free(g_shaderspk3lumps);
		g_shaderspk3lumps = NULL;
		fclose(g_shaderspk3file);
		g_shaderspk3file = NULL;
	}
}

//===========================================================================
//                                                        LUMP BASED ROUTINES
//===========================================================================

// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.

static char filenamebuf[MAX_WADPATH];

// W_OpenWadFile
// Helper function for opening the WAD file.
// Returns the FILE * handle for the file, or NULL if not found or could not be opened
// If "useerrors" is true then print errors in the console, else just don't bother
// "filename" may be modified to have the correct path the actual file is located in, if necessary
FILE *W_OpenWadFile(const char **filename, const char *priorityfolder, boolean useerrors)
{
	FILE *handle;

	// Officially, strncpy should not have overlapping buffers, since W_VerifyNMUSlumps is called after this, and it
	// changes filename to point at filenamebuf, it would technically be doing that. I doubt any issue will occur since
	// they point to the same location, but it's better to be safe and this is a simple change.
	if (filenamebuf != *filename)
	{
		strncpy(filenamebuf, *filename, MAX_WADPATH);
		filenamebuf[MAX_WADPATH - 1] = '\0';
		*filename = filenamebuf;
	}

	// open wad file
	if ((handle = fopen(*filename, "rb")) == NULL)
	{
		// If we failed to load the file with the path as specified by
		// the user, strip the directories and search for the file.
		nameonly(filenamebuf);

		// If findfile finds the file, the full path will be returned
		// in filenamebuf == *filename.
		if (findfile(filenamebuf, priorityfolder, NULL, true))
		{
			if ((handle = fopen(*filename, "rb")) == NULL)
			{
				if (useerrors)
					CONS_Alert(CONS_ERROR, M_GetText("Can't open %s\n"), *filename);
				return NULL;
			}
		}
		else
		{
			if (useerrors)
				CONS_Alert(CONS_ERROR, M_GetText("File %s not found.\n"), *filename);
			return NULL;
		}
	}
	return handle;
}

// Look for all DEHACKED and Lua scripts inside a PK3 archive.
static inline void W_LoadDehackedLumpsPK3(UINT16 wadnum, boolean mainfile)
{
	UINT16 posStart, posEnd;

	posStart = W_CheckNumForFullNamePK3("Init.lua", wadnum, 0);
	if (posStart != INT16_MAX)
	{
		LUA_LoadLump(wadnum, posStart, true);
	}
	else
	{
		posStart = W_CheckNumForFolderStartPK3("Lua/", wadnum, 0);
		if (posStart != INT16_MAX)
		{
			posEnd = W_CheckNumForFolderEndPK3("Lua/", wadnum, posStart);
			for (; posStart < posEnd; posStart++)
				LUA_LoadLump(wadnum, posStart, true);
		}
	}

	posStart = W_CheckNumForFolderStartPK3("SOC/", wadnum, 0);
	if (posStart != INT16_MAX)
	{
		posEnd = W_CheckNumForFolderEndPK3("SOC/", wadnum, posStart);

		for(; posStart < posEnd; posStart++)
		{
			lumpinfo_t *lump_p = &wadfiles[wadnum]->lumpinfo[posStart];
			size_t length = strlen(wadfiles[wadnum]->filename) + 1 + strlen(lump_p->fullname); // length of file name, '|', and lump name
			char *name = static_cast<char*>(malloc(length + 1));
			sprintf(name, "%s|%s", wadfiles[wadnum]->filename, lump_p->fullname);
			name[length] = '\0';
			CONS_Printf(M_GetText("Loading SOC from %s\n"), name);
			DEH_LoadDehackedLumpPwad(wadnum, posStart, mainfile);
			free(name);
		}
	}
}

// search for all DEHACKED lump in all wads and load it
static inline void W_LoadDehackedLumps(UINT16 wadnum, boolean mainfile)
{
	UINT16 lump;

	// Find Lua scripts before SOCs to allow new A_Actions in SOC editing.
	{
		lumpinfo_t *lump_p = wadfiles[wadnum]->lumpinfo;
		for (lump = 0; lump < wadfiles[wadnum]->numlumps; lump++, lump_p++)
			if (memcmp(lump_p->name,"LUA_",4)==0)
				LUA_LoadLump(wadnum, lump, true);
	}

	{
		lumpinfo_t *lump_p = wadfiles[wadnum]->lumpinfo;
		for (lump = 0; lump < wadfiles[wadnum]->numlumps; lump++, lump_p++)
			if (memcmp(lump_p->name,"SOC_",4)==0) // Check for generic SOC lump
			{	// shameless copy+paste of code from LUA_LoadLump
				size_t length = strlen(wadfiles[wadnum]->filename) + 1 + strlen(lump_p->fullname); // length of file name, '|', and lump name
				char *name = static_cast<char*>(malloc(length + 1));
				sprintf(name, "%s|%s", wadfiles[wadnum]->filename, lump_p->fullname);
				name[length] = '\0';

				CONS_Printf(M_GetText("Loading SOC from %s\n"), name);
				DEH_LoadDehackedLumpPwad(wadnum, lump, mainfile);
				free(name);
			}
			else if (memcmp(lump_p->name,"MAINCFG",8)==0) // Check for MAINCFG
			{
				CONS_Printf(M_GetText("Loading main config from %s\n"), wadfiles[wadnum]->filename);
				DEH_LoadDehackedLumpPwad(wadnum, lump, mainfile);
			}
			else if (memcmp(lump_p->name,"OBJCTCFG",8)==0) // Check for OBJCTCFG
			{
				CONS_Printf(M_GetText("Loading object config from %s\n"), wadfiles[wadnum]->filename);
				DEH_LoadDehackedLumpPwad(wadnum, lump, mainfile);
			}
	}
}

/** Compute MD5 message digest for bytes read from STREAM of this filname.
  *
  * The resulting message digest number will be written into the 16 bytes
  * beginning at RESBLOCK.
  *
  * \param filename path of file
  * \param resblock resulting MD5 checksum
  * \return 0 if MD5 checksum was made, and is at resblock, 1 if error was found
  */
static inline INT32 W_MakeFileMD5(const char *filename, void *resblock)
{
#ifdef NOMD5
	(void)filename;
	memset(resblock, 0x00, 16);
#else
	FILE *fhandle;

	if ((fhandle = fopen(filename, "rb")) != NULL)
	{
		tic_t t = I_GetTime();
		CONS_Debug(DBG_SETUP, "Making MD5 for %s\n",filename);
		if (md5_stream(fhandle, resblock) == 1)
		{
			fclose(fhandle);
			return 1;
		}
		CONS_Debug(DBG_SETUP, "MD5 calc for %s took %f seconds\n",
			filename, (float)(I_GetTime() - t)/NEWTICRATE);
		fclose(fhandle);
		return 0;
	}
#endif
	return 1;
}

// Invalidates the cache of lump numbers. Call this whenever a wad is added.
static void W_InvalidateLumpnumCache(void)
{
	memset(lumpnumcache, 0, sizeof (lumpnumcache));
}

/** Detect a file type.
 * \todo Actually detect the wad/pkzip headers and whatnot, instead of just checking the extensions.
 */
static restype_t ResourceFileDetect (const char* filename)
{
	if (!stricmp(&filename[strlen(filename) - 4], ".pk3"))
		return RET_PK3;
	if (!stricmp(&filename[strlen(filename) - 4], ".soc"))
		return RET_SOC;
	if (!stricmp(&filename[strlen(filename) - 4], ".lua"))
		return RET_LUA;

	return RET_WAD;
}

/** Create a 1-lump lumpinfo_t for standalone files.
 */
static lumpinfo_t* ResGetLumpsStandalone (FILE* handle, UINT16* numlumps, const char* lumpname)
{
	lumpinfo_t* lumpinfo = static_cast<lumpinfo_t*>(Z_Calloc(sizeof (*lumpinfo), PU_STATIC, NULL));
	lumpinfo->position = 0;
	fseek(handle, 0, SEEK_END);
	lumpinfo->size = ftell(handle);
	fseek(handle, 0, SEEK_SET);
	strcpy(lumpinfo->name, lumpname);
	lumpinfo->hash = quickncasehash(lumpname, 8);

	// Allocate the lump's long name.
	lumpinfo->longname = static_cast<char*>(Z_Malloc(9 * sizeof(char), PU_STATIC, NULL));
	strcpy(lumpinfo->longname, lumpname);
	lumpinfo->longname[8] = '\0';

	// Allocate the lump's full name.
	lumpinfo->fullname = static_cast<char*>(Z_Malloc(9 * sizeof(char), PU_STATIC, NULL));
	strcpy(lumpinfo->fullname, lumpname);
	lumpinfo->fullname[8] = '\0';

	*numlumps = 1;
	return lumpinfo;
}

/** Create a lumpinfo_t array for a WAD file.
 */
static lumpinfo_t* ResGetLumpsWad (FILE* handle, UINT16* nlmp, const char* filename)
{
	UINT16 numlumps = *nlmp;
	lumpinfo_t* lumpinfo;
	size_t i;
	INT32 compressed = 0;

	wadinfo_t header;
	lumpinfo_t *lump_p;
	filelump_t *fileinfo;
	void *fileinfov;

	// read the header
	if (fread(&header, 1, sizeof header, handle) < sizeof header)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Can't read wad header because %s\n"), M_FileError(handle));
		return NULL;
	}

	if (memcmp(header.identification, "ZWAD", 4) == 0)
		compressed = 1;
	else if (memcmp(header.identification, "IWAD", 4) != 0
		&& memcmp(header.identification, "PWAD", 4) != 0
		&& memcmp(header.identification, "SDLL", 4) != 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Invalid WAD header\n"));
		return NULL;
	}

	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);

	// read wad file directory
	i = header.numlumps * sizeof (*fileinfo);
	fileinfov = fileinfo = static_cast<filelump_t*>(malloc(i));
	if (fseek(handle, header.infotableofs, SEEK_SET) == -1
		|| fread(fileinfo, 1, i, handle) < i)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Corrupt wadfile directory (%s)\n"), M_FileError(handle));
		free(fileinfov);
		return NULL;
	}

	numlumps = header.numlumps;

	// fill in lumpinfo for this wad
	lump_p = lumpinfo = static_cast<lumpinfo_t*>(Z_Malloc(numlumps * sizeof (*lumpinfo), PU_STATIC, NULL));
	for (i = 0; i < numlumps; i++, lump_p++, fileinfo++)
	{
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = lump_p->disksize = LONG(fileinfo->size);
		if (compressed) // wad is compressed, lump might be
		{
			UINT32 realsize = 0;
			if (fseek(handle, lump_p->position, SEEK_SET)
				== -1 || fread(&realsize, 1, sizeof realsize,
				handle) < sizeof realsize)
			{
				I_Error("corrupt compressed file: %s; maybe %s", /// \todo Avoid the bailout?
					filename, M_FileError(handle));
			}
			realsize = LONG(realsize);
			if (realsize != 0)
			{
				lump_p->size = realsize;
				lump_p->compression = CM_LZF;
			}
			else
			{
				lump_p->size -= 4;
				lump_p->compression = CM_NOCOMPRESSION;
			}

			lump_p->position += 4;
			lump_p->disksize -= 4;
		}
		else
			lump_p->compression = CM_NOCOMPRESSION;

		memset(lump_p->name, 0x00, 9);
		strncpy(lump_p->name, fileinfo->name, 8);

		if (WADNAMECHECK(fileinfo->name))
		{
			size_t namelen;
			const char *trimname, *dotpos;

			trimname = strrchr(filename, PATHSEP[0]);
#if defined (_WIN32)
			// For Zone Builder support, work around temporary filenames.
			// They're annoyingly randomised, BUT they follow \Temp\8\8.3...
			// AND they're always guaranteed to follow the map file, which
			// should already have a WADNAME in it for us to piggyback off.
			// EXAMPLE: // \Temp\gj3l7w7n\4f926789.wad

			if (trimname != 0
				&& wadnamelump != LUMPERROR
				&& strlen(trimname+1) == 8+1+3)
			{
				const char *temp = trimname-1;
				while (temp >= filename+5 && *temp != PATHSEP[0])
					temp--;

				if (((trimname-1) - temp) == 8
					&& temp >= filename+5
					&& !strncmp(temp-5, PATHSEP"Temp", 5))
				{
					filename = wadfiles[
						((wadnamelump & ~UINT16_MAX) >> 16)
						]->filename;
					trimname = strrchr(filename, PATHSEP[0]);
				}
			}
#endif
			// Strip away file address
			if (trimname != 0)
				trimname++;
			else
				trimname = filename; // Care taken for root files.

			// First stop, not last, to permit RR_GREENHILLS.beta3.wad
			if ((dotpos = strchr(trimname, '.')) != 0)
				namelen = (dotpos + 1 - trimname);
			else
				namelen = strlen(trimname);

			// Allocate the lump's long and full name (save on memory).
			lump_p->longname = lump_p->fullname = static_cast<char*>(Z_Calloc(namelen * sizeof(char), PU_STATIC, NULL));
			strncpy(lump_p->longname, trimname, namelen);
			lump_p->longname[namelen-1] = '\0';

			CONS_Debug(DBG_SETUP, "WADNAME handling:\n -- path %s\n -- interpreted lumpname %s\n", filename, lump_p->longname);

			// Grab the hash from the first part
			lump_p->hash = quickncasehash(lump_p->longname, 8);

			wadnamelump = i | (numwadfiles << 16);
		}
		else
		{
			// Set up true hash
			lump_p->hash = quickncasehash(lump_p->name, 8);

			// Allocate the lump's long and full name (save on memory).
			lump_p->longname = lump_p->fullname = static_cast<char*>(Z_Malloc(9 * sizeof(char), PU_STATIC, NULL));
			strncpy(lump_p->longname, fileinfo->name, 8);
			lump_p->longname[8] = '\0';
		}
	}
	free(fileinfov);
	*nlmp = numlumps;
	return lumpinfo;
}

/** Optimized pattern search in a file.
 */
static boolean ResFindSignature (FILE* handle, char endPat[], UINT32 startpos)
{
	//the Wii U has rather slow filesystem access, and fgetc is *unbearable*
	//so I reimplemented this function to buffer 128k chunks
	char *s;
	int c;

	fseek(handle, 0, SEEK_END);
	size_t len = ftell(handle);

	fseek(handle, startpos, SEEK_SET);
	size_t remaining = len - startpos;
	size_t chunkpos = startpos;

	s = endPat;

	//128k buffers
	size_t buffer_size = std::min(128 * 1024 * sizeof(char), remaining);
	char* buffer = static_cast<char*>(malloc(buffer_size));

	size_t bytes_read = 0;
	while ((bytes_read = fread(buffer, 1, buffer_size, handle)) > 0) {
		for (size_t i = 0; i < bytes_read; i++) {
			c = (int)buffer[i];

			if (*s != c && s > endPat) // No match?
				s = endPat; // We "reset" the counter by sending the s pointer back to the start of the array.
			if (*s == c)
			{
				s++;
				if (*s == 0x00) // The array pointer has reached the key char which marks the end. It means we have matched the signature.
				{
					//the original function would leave the FILE* seeked to the end of the match
					size_t foundpos = chunkpos + i + 1;
					fseek(handle, foundpos, SEEK_SET);

					free(buffer);
					return true;
				}
			}
		}
		chunkpos += bytes_read;
	}

	free(buffer);
	return false;
}

#if defined(_MSC_VER)
#pragma pack(1)
#endif
typedef struct zend_s
{
	char signature[4];
	UINT16 diskpos;
	UINT16 cdirdisk;
	UINT16 diskentries;
	UINT16 entries;
	UINT32 cdirsize;
	UINT32 cdiroffset;
	UINT16 commentlen;
} ATTRPACK zend_t;

typedef struct zentry_s
{
	char signature[4];
	UINT16 version;
	UINT16 versionneeded;
	UINT16 flags;
	UINT16 compression;
	UINT16 modtime;
	UINT16 moddate;
	UINT32 CRC32;
	UINT32 compsize;
	UINT32 size;
	UINT16 namelen;
	UINT16 xtralen;
	UINT16 commlen;
	UINT16 diskstart;
	UINT16 attrint;
	UINT32 attrext;
	UINT32 offset;
} ATTRPACK zentry_t;

typedef struct zlentry_s
{
	char signature[4];
	UINT16 versionneeded;
	UINT16 flags;
	UINT16 compression;
	UINT16 modtime;
	UINT16 moddate;
	UINT32 CRC32;
	UINT32 compsize;
	UINT32 size;
	UINT16 namelen;
	UINT16 xtralen;
} ATTRPACK zlentry_t;
#if defined(_MSC_VER)
#pragma pack()
#endif

/** Create a lumpinfo_t array for a PKZip file.
 */
static lumpinfo_t* ResGetLumpsZip (FILE* handle, UINT16* nlmp)
{
    zend_t zend;
    zlentry_t zlentry;

	UINT16 numlumps = *nlmp;
	lumpinfo_t* lumpinfo;
	lumpinfo_t *lump_p;
	size_t i;

	char pat_central[] = {0x50, 0x4b, 0x01, 0x02, 0x00};
	char pat_end[] = {0x50, 0x4b, 0x05, 0x06, 0x00};

	// Look for central directory end signature near end of file.
	// Contains entry number (number of lumps), and central directory start offset.
	fseek(handle, 0, SEEK_END);
	if (!ResFindSignature(handle, pat_end, std::max(0l, ftell(handle) - (22 + 65536))))
	{
		CONS_Alert(CONS_ERROR, "Missing central directory\n");
		return NULL;
	}

	fseek(handle, -4, SEEK_CUR);
	if (fread(&zend, 1, sizeof zend, handle) < sizeof zend)
	{
		CONS_Alert(CONS_ERROR, "Corrupt central directory (%s)\n", M_FileError(handle));
		return NULL;
	}
	numlumps = SHORT(zend.entries);

	lump_p = lumpinfo = static_cast<lumpinfo_t*>(Z_Malloc(numlumps * sizeof (*lumpinfo), PU_STATIC, NULL));

	fseek(handle, LONG(zend.cdiroffset), SEEK_SET);

	char *cdir = static_cast<char*>(Z_MallocAlign(LONG(zend.cdirsize), PU_STATIC, &cdir, 7));
	auto cdir_finally = srb2::finally([cdir] { Z_Free(cdir); });

	if (fread(cdir, 1, LONG(zend.cdirsize), handle) < static_cast<UINT32>(LONG(zend.cdirsize)))
	{
		CONS_Alert(CONS_ERROR, "Failed to read central directory (%s)\n", M_FileError(handle));
		Z_Free(lumpinfo);
		return NULL;
	}

	size_t offset = 0;

	for (i = 0; i < numlumps; i++, lump_p++)
	{
		zentry_t *zentry = reinterpret_cast<zentry_t*>(cdir + offset);
		char* fullname;
		char* trimname;
		char* dotpos;

		if (memcmp(zentry->signature, pat_central, 4))
		{
			CONS_Alert(CONS_ERROR, "Central directory is corrupt\n");
			Z_Free(lumpinfo);
			return NULL;
		}

		lump_p->position = LONG(zentry->offset); // NOT ACCURATE YET: we still need to read the local entry to find our true position
		lump_p->disksize = LONG(zentry->compsize);
		lump_p->size = LONG(zentry->size);

		fullname = static_cast<char*>(malloc(SHORT(zentry->namelen) + 1));
		strlcpy(fullname, (char*)(zentry + 1), SHORT(zentry->namelen) + 1);

		// Strip away file address and extension for the 8char name.
		if ((trimname = strrchr(fullname, '/')) != 0)
			trimname++;
		else
			trimname = fullname; // Care taken for root files.

		if ((dotpos = strrchr(trimname, '.')) == 0)
			dotpos = fullname + strlen(fullname); // Watch for files without extension.

		memset(lump_p->name, '\0', 9); // Making sure they're initialized to 0. Is it necessary?
		strncpy(lump_p->name, trimname, std::min(static_cast<std::ptrdiff_t>(8), dotpos - trimname));
		lump_p->hash = quickncasehash(lump_p->name, 8);

		lump_p->longname = static_cast<char*>(Z_Calloc(dotpos - trimname + 1, PU_STATIC, NULL));
		strlcpy(lump_p->longname, trimname, dotpos - trimname + 1);

		lump_p->fullname = static_cast<char*>(Z_Calloc(SHORT(zentry->namelen) + 1, PU_STATIC, NULL));
		strncpy(lump_p->fullname, fullname, SHORT(zentry->namelen));

		switch(SHORT(zentry->compression))
		{
		case 0:
			lump_p->compression = CM_NOCOMPRESSION;
			break;
#ifdef HAVE_ZLIB
		case 8:
			lump_p->compression = CM_DEFLATE;
			break;
#endif
		case 14:
			lump_p->compression = CM_LZF;
			break;
		default:
			CONS_Alert(CONS_WARNING, "%s: Unsupported compression method\n", fullname);
			lump_p->compression = CM_UNSUPPORTED;
			break;
		}

		free(fullname);

		// skip and ignore comments/extra fields
		offset += sizeof *zentry + SHORT(zentry->namelen) + SHORT(zentry->xtralen) + SHORT(zentry->commlen);
	}

	// Adjust lump position values properly
	for (i = 0, lump_p = lumpinfo; i < numlumps; i++, lump_p++)
	{
		// skip and ignore comments/extra fields
		if ((fseek(handle, lump_p->position, SEEK_SET) != 0) || (fread(&zlentry, 1, sizeof(zlentry_t), handle) < sizeof(zlentry_t)))
		{
			CONS_Alert(CONS_ERROR, "Local headers for lump %s are corrupt\n", lump_p->fullname);
			Z_Free(lumpinfo);
			return NULL;
		}

		lump_p->position += sizeof(zlentry_t) + SHORT(zlentry.namelen) + SHORT(zlentry.xtralen);
	}

	*nlmp = numlumps;
	return lumpinfo;
}

static UINT16 W_InitFileError (const char *filename, boolean exitworthy)
{
	if (exitworthy)
	{
#ifdef _DEBUG
		CONS_Error(va("%s was not found or not valid.\nCheck the log for more details.\n", filename));
#else
		I_Error("%s was not found or not valid.\nCheck the log for more details.\n", filename);
#endif
	}
	else
		CONS_Printf(M_GetText("Errors occurred while loading %s; not added.\n"), filename);
	return INT16_MAX;
}

//  Allocate a wadfile, setup the lumpinfo (directory) and
//  lumpcache, add the wadfile to the current active wadfiles
//
//  now returns index into wadfiles[], you can get wadfile_t *
//  with:
//       wadfiles[<return value>]
//
//  return -1 in case of problem
//
// Can now load dehacked files (.soc)
//
UINT16 W_InitFile(const char *filename, boolean mainfile, boolean startup, const char *md5expected)
{
	FILE *handle;
	lumpinfo_t *lumpinfo = NULL;
	wadfile_t *wadfile;
	restype_t type;
	UINT16 numlumps = 0;
#ifndef NOMD5
	size_t i;
#endif
	UINT8 md5sum[16];
	int important;

	if (!(refreshdirmenu & REFRESHDIR_ADDFILE))
		refreshdirmenu = REFRESHDIR_NORMAL|REFRESHDIR_ADDFILE; // clean out cons_alerts that happened earlier

	if (refreshdirname)
		Z_Free(refreshdirname);
	if (dirmenu)
	{
		refreshdirname = Z_StrDup(filename);
		nameonly(refreshdirname);
	}
	else
		refreshdirname = NULL;

	CONS_Printf("Loading %s\n", filename);
	//
	// check if limit of active wadfiles
	//
	if (numwadfiles >= MAX_WADFILES)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Maximum wad files reached\n"));
		refreshdirmenu |= REFRESHDIR_MAX;
		return W_InitFileError(filename, startup);
	}

	// open wad file
	if ((handle = W_OpenWadFile(&filename, (mainfile ? NULL : "addons"), true)) == NULL)
		return W_InitFileError(filename, startup);

	important = W_VerifyNMUSlumps(filename, handle, startup);

	if (important == -1)
	{
		fclose(handle);
		return INT16_MAX;
	}

	important = !important;

#ifndef NOMD5
	//
	// w-waiiiit!
	// Let's not add a wad file if the MD5 matches
	// an MD5 of an already added WAD file!
	//
	W_MakeFileMD5(filename, md5sum);

	if (md5expected)
	{
		// moved Graue's <graue@oceanbase.org> W_VerifyFileMD5 inline here
		UINT8 realmd5[MD5_LEN];
		INT32 ix;

		I_Assert(strlen(md5expected) == 2*MD5_LEN);

		// Convert an md5 string like "7d355827fa8f981482246d6c95f9bd48"
		// into a real md5.
		for (ix = 0; ix < 2*MD5_LEN; ix++)
		{
			INT32 n, c = md5expected[ix];
			if (isdigit(c))
				n = c - '0';
			else
			{
				I_Assert(isxdigit(c));
				if (isupper(c)) n = c - 'A' + 10;
				else n = c - 'a' + 10;
			}
			if (ix & 1) realmd5[ix>>1] = (UINT8)(realmd5[ix>>1]+n);
			else realmd5[ix>>1] = (UINT8)(n<<4);
		}

		if (memcmp(realmd5, md5sum, 16) != 0)
		{
			char actualmd5text[2*MD5_LEN+1];
			PrintMD5String(md5sum, actualmd5text);
#ifdef DEVELOP
			CONS_Printf("File %s does not match expected md5\n", filename);
#else
			if (startup)
			{
				I_Error(M_GetText("File is old, is corrupt or has been modified: %s (found md5: %s, wanted: %s)\n"), filename, actualmd5text, md5expected);
			}
			else
			{
				CONS_Alert(CONS_ERROR, M_GetText("Did not load file %s because it did not match expected md5sum %s\n"), filename, md5expected);
				if (handle)
					fclose(handle);
				return W_InitFileError(filename, false);
			}
#endif
		}
	}

	for (i = 0; i < numwadfiles; i++)
	{
		if (!memcmp(wadfiles[i]->md5sum, md5sum, 16))
		{
			CONS_Alert(CONS_ERROR, M_GetText("%s is already loaded\n"), filename);
			if (handle)
				fclose(handle);
			return W_InitFileError(filename, false);
		}
	}
#endif

	// Do this immediately before anything of consequence that invalidates gamedata can happen.
	if ((mainfile == false) && (gamedata != NULL) && (gamedata->everloadedaddon == false))
	{
		gamedata->everloadedaddon = true;
		M_UpdateUnlockablesAndExtraEmblems(true, true);
		G_SaveGameData();
	}

	switch(type = ResourceFileDetect(filename))
	{
	case RET_SOC:
		lumpinfo = ResGetLumpsStandalone(handle, &numlumps, "SOC_INIT");
		break;
	case RET_LUA:
		lumpinfo = ResGetLumpsStandalone(handle, &numlumps, "LUA_INIT");
		break;
	case RET_PK3:
		lumpinfo = ResGetLumpsZip(handle, &numlumps);
		break;
	case RET_WAD:
		lumpinfo = ResGetLumpsWad(handle, &numlumps, filename);
		break;
	default:
		CONS_Alert(CONS_ERROR, "Unsupported file format\n");
	}

	if (lumpinfo == NULL)
	{
		fclose(handle);
		return W_InitFileError(filename, startup);
	}

	if (important && !mainfile)
	{
		G_SetGameModified(true, false);
	}

	//
	// link wad file to search files
	//
	wadfile = static_cast<wadfile_t*>(Z_Malloc(sizeof (*wadfile), PU_STATIC, NULL));
	wadfile->filename = Z_StrDup(filename);
	wadfile->type = type;
	wadfile->handle = handle;
	wadfile->numlumps = (UINT16)numlumps;
	wadfile->lumpinfo = lumpinfo;
	wadfile->important = important;
	fseek(handle, 0, SEEK_END);
	wadfile->filesize = (unsigned)ftell(handle);
	wadfile->type = type;

	// already generated, just copy it over
	M_Memcpy(&wadfile->md5sum, &md5sum, 16);

	//
	// set up caching
	//
	Z_Calloc(numlumps * sizeof (*wadfile->lumpcache), PU_STATIC, &wadfile->lumpcache);
	Z_Calloc(numlumps * sizeof (*wadfile->patchcache), PU_STATIC, &wadfile->patchcache);

	//
	// add the wadfile
	//
	CONS_Printf(M_GetText("Added file %s (%u lumps)\n"), filename, numlumps);
	wadfiles[numwadfiles] = wadfile;
	numwadfiles++; // must come BEFORE W_LoadDehackedLumps, so any addfile called by COM_BufInsertText called by Lua doesn't overwrite what we just loaded

	//
	// fill out metadata
	//
	if (mainfile == false) // main files do not need a MODINFO
	{
		wadfile->metadata = new mod_metadata_t(numwadfiles - 1);

		CONS_Printf(
			"== %s (version %s) - by %s ==\n"
			"%s\n"
			"More @ %s\n"
			"[ DESIGNED FOR RING RACERS %d.%d ]\n",
			wadfile->metadata->name().c_str(),
			wadfile->metadata->version().c_str(),
			wadfile->metadata->author().c_str(),
			wadfile->metadata->description().c_str(),
			wadfile->metadata->info_url().c_str(),
			wadfile->metadata->game_version(),
			wadfile->metadata->game_subversion()
		);
	}

#ifdef HWRENDER
	// Read shaders from file
	if (rendermode == render_opengl && (vid.glstate == VID_GL_LIBRARY_LOADED))
	{
		HWR_LoadCustomShadersFromFile(numwadfiles - 1, (type == RET_PK3));
		HWR_CompileShaders();
	}
#endif // HWRENDER

	// TODO: HACK ALERT - Load Lua & SOC stuff right here. I feel like this should be out of this place, but... Let's stick with this for now.
	switch (wadfile->type)
	{
	case RET_WAD:
		W_LoadDehackedLumps(numwadfiles - 1, mainfile);
		break;
	case RET_PK3:
		W_LoadDehackedLumpsPK3(numwadfiles - 1, mainfile);
		break;
	case RET_SOC:
		CONS_Printf(M_GetText("Loading SOC from %s\n"), wadfile->filename);
		DEH_LoadDehackedLumpPwad(numwadfiles - 1, 0, mainfile);
		break;
	case RET_LUA:
		LUA_LoadLump(numwadfiles - 1, 0, true);
		break;
	default:
		break;
	}

	K_InitTerrain(numwadfiles - 1);

	if (refreshdirmenu & REFRESHDIR_GAMEDATA)
		G_LoadGameData();
	DEH_UpdateMaxFreeslots();

	W_InvalidateLumpnumCache();
	return wadfile->numlumps;
}

/** Tries to load a series of files.
  * All files are wads unless they have an extension of ".soc" or ".lua".
  *
  * Each file is optional, but at least one file must be found or an error will
  * result. Lump names can appear multiple times. The name searcher looks
  * backwards, so a later file overrides all earlier ones.
  */
INT32 W_InitMultipleFiles(const initmultiplefilesentry_t *entries, INT32 count, boolean addons)
{
	INT32 i;
	INT32 rc = 1;
	INT32 overallrc = 1;

	// will be realloced as lumps are added
	for (i = 0; i < count; ++i)
	{
		const initmultiplefilesentry_t *entry = &entries[i];

		// Previously, W_VerifyNMUSlumps was called to mark game modified
		// for addons... but W_InitFile already does exactly that!

		//CONS_Debug(DBG_SETUP, "Loading %s\n", *filenames);
		rc = W_InitFile(entry->filename, !addons, true, entry->md5sum);
		if (rc == INT16_MAX)
			CONS_Printf(M_GetText("Errors occurred while loading %s; not added.\n"), entry->filename);
		overallrc &= (rc != INT16_MAX) ? 1 : 0;
	}

	if (!numwadfiles)
		I_Error("W_InitMultipleFiles: no files found");

	return overallrc;
}

/** Make sure a lump number is valid.
  * Compiles away to nothing if PARANOIA is not defined.
  */
static boolean TestValidLump(UINT16 wad, UINT16 lump)
{
	I_Assert(wad < MAX_WADFILES);
	if (!wadfiles[wad]) // make sure the wad file exists
		return false;

	I_Assert(lump < wadfiles[wad]->numlumps);
	if (lump >= wadfiles[wad]->numlumps) // make sure the lump exists
		return false;

	return true;
}


const char *W_CheckNameForNumPwad(UINT16 wad, UINT16 lump)
{
	if (lump >= wadfiles[wad]->numlumps || !TestValidLump(wad, 0))
		return NULL;

	return wadfiles[wad]->lumpinfo[lump].name;
}

const char *W_CheckNameForNum(lumpnum_t lumpnum)
{
	return W_CheckNameForNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum));
}

const char *W_CheckLongNameForNumPwad(UINT16 wad, UINT16 lump)
{
	if (lump >= wadfiles[wad]->numlumps || !TestValidLump(wad, 0))
		return NULL;

	return wadfiles[wad]->lumpinfo[lump].longname;
}

const char *W_CheckLongNameForNum(lumpnum_t lumpnum)
{
	return W_CheckLongNameForNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum));
}

//
// wadid is a wad number
// (Used for sprites loading)
//
// 'startlump' is the lump number to start the search
//
UINT16 W_FindNextEmptyInPwad(UINT16 wad, UINT16 startlump)
{
	UINT16 i;

	if (!TestValidLump(wad,0))
		return INT16_MAX;

	//
	// scan forward
	// start at 'startlump', useful parameter when there are multiple
	//                       resources with the same name
	//
	if (startlump < wadfiles[wad]->numlumps)
	{
		lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
		for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
			if (!lump_p->size)
				return i;
	}

	// not found.
	return INT16_MAX;
}

// Get a map marker for WADs, and a standalone WAD file lump inside PK3s. Takes uppercase names only
UINT16 W_CheckNumForMapPwad(const char *name, UINT32 hash, UINT16 wad, UINT16 startlump)
{
	UINT16 i, end;

	if (wadfiles[wad]->type == RET_WAD)
	{
		for (i = startlump; i < wadfiles[wad]->numlumps; i++)
		{
			// Not the hash?
			if ((wadfiles[wad]->lumpinfo + i)->hash != hash)
				continue;

			// Not the name? (always use longname, even in wads, to accomodate WADNAME)
			if (strcasecmp(name, (wadfiles[wad]->lumpinfo + i)->longname))
				continue;

			// Not a header?
			if (W_LumpLength(i | (wad << 16)) > 0)
				continue;

			return i;
		}
	}
	else if (wadfiles[wad]->type == RET_PK3)
	{
		i = W_CheckNumForFolderStartPK3("maps/", wad, startlump);

		if (i != INT16_MAX)
		{
			end = W_CheckNumForFolderEndPK3("maps/", wad, i);

			// Now look for the specified map.
			for (; i < end; i++)
			{
				// Not the hash?
				if ((wadfiles[wad]->lumpinfo + i)->hash != hash)
					continue;

				// Not the name?
				if (strcasecmp(name, (wadfiles[wad]->lumpinfo + i)->longname))
					continue;

				// Not a .wad?
				if (!W_IsLumpWad(i | (wad << 16)))
					continue;

				return i;
			}
		}
	}

	return INT16_MAX;
}

//
// Same as the original, but checks in one pwad only.
// wadid is a wad number
// (Used for sprites loading)
//
// 'startlump' is the lump number to start the search
//
UINT16 W_CheckNumForNamePwad(const char *name, UINT16 wad, UINT16 startlump)
{
	UINT16 i;
	UINT32 hash = quickncasehash(name, 8);

	if (!TestValidLump(wad,0))
		return INT16_MAX;

	//
	// scan forward
	// start at 'startlump', useful parameter when there are multiple
	//                       resources with the same name
	//
	if (startlump < wadfiles[wad]->numlumps)
	{
		lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
		for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
		{
			if (lump_p->hash != hash)
				continue;
			if (strncasecmp(lump_p->name, name, 8))
				continue;
			return i;
		}
	}

	// not found.
	return INT16_MAX;
}

//
// Like W_CheckNumForNamePwad, but can find entries with long names
//
// Should be the only version, but that's not possible until we fix
// all the instances of non null-terminated strings in the codebase...
//
UINT16 W_CheckNumForLongNamePwad(const char *name, UINT16 wad, UINT16 startlump)
{
	UINT16 i;
	UINT32 hash = quickncasehash(name, 8); // Not a mistake, legacy system for short lumpnames

	if (!TestValidLump(wad,0))
		return INT16_MAX;

	//
	// scan forward
	// start at 'startlump', useful parameter when there are multiple
	//                       resources with the same name
	//
	if (startlump < wadfiles[wad]->numlumps)
	{
		lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
		for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
		{
			if (lump_p->hash != hash)
				continue;
			if (strcasecmp(lump_p->longname, name))
				continue;
			return i;
		}
	}

	// not found.
	return INT16_MAX;
}

UINT16
W_CheckNumForMarkerStartPwad (const char *name, UINT16 wad, UINT16 startlump)
{
	UINT16 marker;
	marker = W_CheckNumForNamePwad(name, wad, startlump);
	if (marker != INT16_MAX)
		marker++; // Do not count the first marker
	return marker;
}

// Look for the first lump from a folder.
UINT16 W_CheckNumForFolderStartPK3(const char *name, UINT16 wad, UINT16 startlump)
{
	size_t name_length;
	INT32 i;
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
	name_length = strlen(name);
	for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (strnicmp(name, lump_p->fullname, name_length) == 0)
		{
			/* SLADE is special and puts a single directory entry. Skip that. */
			if (strlen(lump_p->fullname) == name_length)
				i++;
			break;
		}
	}
	return i;
}

// In a PK3 type of resource file, it looks for the next lumpinfo entry that doesn't share the specified pathfile.
// Useful for finding folder ends.
// Returns the position of the lumpinfo entry.
UINT16 W_CheckNumForFolderEndPK3(const char *name, UINT16 wad, UINT16 startlump)
{
	INT32 i;
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
	for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (strnicmp(name, lump_p->fullname, strlen(name)))
			break;
	}
	return i;
}

// In a PK3 type of resource file, it looks for an entry with the specified full name.
// Returns lump position in PK3's lumpinfo, or INT16_MAX if not found.
UINT16 W_CheckNumForFullNamePK3(const char *name, UINT16 wad, UINT16 startlump)
{
	INT32 i;
	lumpinfo_t *lump_p = wadfiles[wad]->lumpinfo + startlump;
	for (i = startlump; i < wadfiles[wad]->numlumps; i++, lump_p++)
	{
		if (!strnicmp(name, lump_p->fullname, strlen(name)))
		{
			return i;
		}
	}
	// Not found at all?
	return INT16_MAX;
}

//
// W_CheckNumForName
// Returns LUMPERROR if name not found.
//
lumpnum_t W_CheckNumForName(const char *name)
{
	lumpnum_t check = INT16_MAX;
	UINT32 hash = name ? quickncasehash(name, 8) : 0;
	INT32 i;

	if (name == NULL)
		return LUMPERROR;

	if (!*name) // some doofus gave us an empty string?
		return LUMPERROR;

	// Check the lumpnumcache first. Loop backwards so that we check
	// most recent entries first
	for (i = lumpnumcacheindex + LUMPNUMCACHESIZE; i > lumpnumcacheindex; i--)
	{
		if (!lumpnumcache[i & (LUMPNUMCACHESIZE - 1)].lumpname[8]
			&& lumpnumcache[i & (LUMPNUMCACHESIZE - 1)].lumphash == hash
			&& strncasecmp(lumpnumcache[i & (LUMPNUMCACHESIZE - 1)].lumpname, name, 8) == 0)
		{
			lumpnumcacheindex = i & (LUMPNUMCACHESIZE - 1);
			return lumpnumcache[lumpnumcacheindex].lumpnum;
		}
	}

	// scan wad files backwards so patch lump files take precedence
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		check = W_CheckNumForNamePwad(name,(UINT16)i,0);
		if (check != INT16_MAX)
			break; //found it
	}

	if (check == INT16_MAX)
	{
		return LUMPERROR;
	}
	else
	{
		// Update the cache.
		lumpnumcacheindex = (lumpnumcacheindex + 1) & (LUMPNUMCACHESIZE - 1);
		memset(lumpnumcache[lumpnumcacheindex].lumpname, '\0', LUMPNUMCACHENAME);
		strncpy(lumpnumcache[lumpnumcacheindex].lumpname, name, 8);
		lumpnumcache[lumpnumcacheindex].lumpnum = (i << 16) | check;
		lumpnumcache[lumpnumcacheindex].lumphash = hash;

		return lumpnumcache[lumpnumcacheindex].lumpnum;
	}
}

//
// Like W_CheckNumForName, but can find entries with long names
//
// Should be the only version, but that's not possible until we fix
// all the instances of non null-terminated strings in the codebase...
//
lumpnum_t W_CheckNumForLongName(const char *name)
{
	lumpnum_t check = INT16_MAX;
	UINT32 hash = name ? quickncasehash(name, LUMPNUMCACHENAME) : 0;
	INT32 i;

	if (name == NULL)
		return LUMPERROR;

	if (!*name) // some doofus gave us an empty string?
		return LUMPERROR;

	// Check the lumpnumcache first. Loop backwards so that we check
	// most recent entries first
	for (i = lumpnumcacheindex + LUMPNUMCACHESIZE; i > lumpnumcacheindex; i--)
	{
		if (lumpnumcache[i & (LUMPNUMCACHESIZE - 1)].lumphash == hash
			&& strcasecmp(lumpnumcache[i & (LUMPNUMCACHESIZE - 1)].lumpname, name) == 0)
		{
			lumpnumcacheindex = i & (LUMPNUMCACHESIZE - 1);
			return lumpnumcache[lumpnumcacheindex].lumpnum;
		}
	}

	// scan wad files backwards so patch lump files take precedence
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		check = W_CheckNumForLongNamePwad(name,(UINT16)i,0);
		if (check != INT16_MAX)
			break; //found it
	}

	if (check == INT16_MAX)
	{
		return LUMPERROR;
	}
	else
	{
		if (strlen(name) < LUMPNUMCACHENAME)
		{
			// Update the cache.
			lumpnumcacheindex = (lumpnumcacheindex + 1) & (LUMPNUMCACHESIZE - 1);
			memset(lumpnumcache[lumpnumcacheindex].lumpname, '\0', LUMPNUMCACHENAME);
			strlcpy(lumpnumcache[lumpnumcacheindex].lumpname, name, LUMPNUMCACHENAME);
			lumpnumcache[lumpnumcacheindex].lumpnum = (i << 16) | check;
			lumpnumcache[lumpnumcacheindex].lumphash = hash;
		}

		return (i << 16) | check;
	}
}

// Look for valid map data through all added files in descendant order.
// Get a map marker for WADs, and a standalone WAD file lump inside PK3s.
lumpnum_t W_CheckNumForMap(const char *name, boolean checktofirst)
{
	lumpnum_t check = INT16_MAX;
	UINT32 uhash, hash = quickncasehash(name, LUMPNUMCACHENAME);
	INT32 i;
	UINT16 firstfile = (checktofirst || (partadd_earliestfile == UINT16_MAX)) ? 0 : partadd_earliestfile;

	// Check the lumpnumcache first. Loop backwards so that we check
	// most recent entries first
	for (i = lumpnumcacheindex + LUMPNUMCACHESIZE; i > lumpnumcacheindex; i--)
	{
		if (lumpnumcache[i & (LUMPNUMCACHESIZE - 1)].lumphash == hash
			&& strcasecmp(lumpnumcache[i & (LUMPNUMCACHESIZE - 1)].lumpname, name) == 0)
		{
			lumpnumcacheindex = i & (LUMPNUMCACHESIZE - 1);
			return lumpnumcache[lumpnumcacheindex].lumpnum;
		}
	}

	uhash = quickncasehash(name, 8); // Not a mistake, legacy system for short lumpnames

	for (i = numwadfiles - 1; i >= firstfile; i--)
	{
		check = W_CheckNumForMapPwad(name, uhash, (UINT16)i, 0);

		if (check != INT16_MAX)
			break; // found it
	}

	if (check == INT16_MAX)
	{
		return LUMPERROR;
	}
	else
	{
		if (strlen(name) < LUMPNUMCACHENAME)
		{
			// Update the cache.
			lumpnumcacheindex = (lumpnumcacheindex + 1) & (LUMPNUMCACHESIZE - 1);
			memset(lumpnumcache[lumpnumcacheindex].lumpname, '\0', LUMPNUMCACHENAME);
			strlcpy(lumpnumcache[lumpnumcacheindex].lumpname, name, LUMPNUMCACHENAME);
			lumpnumcache[lumpnumcacheindex].lumpnum = (i << 16) | check;
			lumpnumcache[lumpnumcacheindex].lumphash = hash;
		}

		return (i << 16) | check;
	}
}

//
// W_GetNumForName
//
// Calls W_CheckNumForName, but bombs out if not found.
//
lumpnum_t W_GetNumForName(const char *name)
{
	lumpnum_t i;

	i = W_CheckNumForName(name);

	if (i == LUMPERROR)
		I_Error("W_GetNumForName: %s not found!\n", name);

	return i;
}

//
// Like W_GetNumForName, but can find entries with long names
//
// Should be the only version, but that's not possible until we fix
// all the instances of non null-terminated strings in the codebase...
//
lumpnum_t W_GetNumForLongName(const char *name)
{
	lumpnum_t i;

	i = W_CheckNumForLongName(name);

	if (i == LUMPERROR)
		I_Error("W_GetNumForLongName: %s not found!\n", name);

	return i;
}

//
// W_CheckNumForNameInBlock
// Checks only in blocks from blockstart lump to blockend lump
//
lumpnum_t W_CheckNumForNameInBlock(const char *name, const char *blockstart, const char *blockend)
{
	INT32 i;
	lumpnum_t bsid, beid;
	lumpnum_t check = INT16_MAX;

	// scan wad files backwards so patch lump files take precedence
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		if (wadfiles[i]->type == RET_WAD)
		{
			bsid = W_CheckNumForNamePwad(blockstart, (UINT16)i, 0);
			if (bsid == INT16_MAX)
				continue; // Start block doesn't exist?
			beid = W_CheckNumForNamePwad(blockend, (UINT16)i, 0);
			if (beid == INT16_MAX)
				continue; // End block doesn't exist?

			check = W_CheckNumForNamePwad(name, (UINT16)i, bsid);
			if (check < beid)
				return (i<<16)+check; // found it, in our constraints
		}
	}
	return LUMPERROR;
}

//
// W_CheckNumForNameInFolder
// Checks only in PK3s in the specified folder
//
lumpnum_t W_CheckNumForNameInFolder(const char *lump, const char *folder)
{
	INT32 i;
	lumpnum_t fsid, feid;
	lumpnum_t check = INT16_MAX;

	// scan wad files backwards so patch lump files take precedence
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		if (wadfiles[i]->type == RET_PK3)
		{
			fsid = W_CheckNumForFolderStartPK3(folder, (UINT16)i, 0);
			if (fsid == INT16_MAX)
			{
				continue; // Start doesn't exist?
			}

			feid = W_CheckNumForFolderEndPK3(folder, (UINT16)i, fsid);
			if (feid == INT16_MAX)
			{
				continue; // End doesn't exist?
			}

			check = W_CheckNumForLongNamePwad(lump, (UINT16)i, fsid);
			if (check < feid)
			{
				return (i<<16) | check; // found it, in our constraints
			}
		}
	}

	return LUMPERROR;
}

// Used by Lua. Case sensitive lump checking, quickly...
#include "fastcmp.h"
UINT8 W_LumpExists(const char *name)
{
	INT32 i,j;
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		lumpinfo_t *lump_p = wadfiles[i]->lumpinfo;
		for (j = 0; j < wadfiles[i]->numlumps; ++j, ++lump_p)
		{
			if (!fastcmp(lump_p->longname, name))
				continue;
			return true;
		}
	}
	return false;
}

// Thanks to the introduction of "client side WAD files",
// a notion which is insanity in any other branch of DOOM,
// any direct wadnum ID is not a guaranteed index (and
// lumpnum_t, which has it in their upper bits, suffer too)
// We can do an O(n) conversion back and forth, which is
// better than nothing, but still kind of annoying to do.
// It was either this or killing musicwads lmao ~toast 180925

lumpnum_t W_LumpIntoNetSave(lumpnum_t lump)
{
	UINT32 wad = (lump >> 16);
	if (lump == LUMPERROR // Bad already
	|| wad < mainwads) // Same between client/server
	{
		// Give what we get.
		return lump;
	}

	if (wad >= numwadfiles // Outside of range
	|| !wadfiles[wad]->important) // Can't convert local lumpnum
	{
		// No good return result!
		return LUMPERROR;
	}

	// Count previous local files the client might not have.
	UINT32 i = (mainwads + musicwads), localoffset = 0;
	for (; i < wad; i++)
	{
		if (wadfiles[i]->important)
			continue;

		localoffset++;
	}

	if (!localoffset)
	{
		// No local files, return unchanged.
		return lump;
	}

	if (localoffset <= wad)
	{
		// Success, return with the conversion.
		return ((wad - localoffset) << 16) | (lump & UINT16_MAX);
	}

	// Death!!
	return LUMPERROR;
}

lumpnum_t W_LumpFromNetSave(lumpnum_t lump)
{
	UINT32 netwad = (lump >> 16);
	if (lump == LUMPERROR // Bad already
	|| netwad < mainwads) // Same between client/server
	{
		// Give what we get.
		return lump;
	}

	// Count previous local files the server would ignore.
	UINT32 i = (mainwads + musicwads), localoffset = 0;
	for (; (i - localoffset) <= netwad && i < numwadfiles; i++)
	{
		if (wadfiles[i]->important)
			continue;

		localoffset++;
	}

	if (!localoffset)
	{
		// No local files, return unchanged.
		return lump;
	}

	if (netwad + localoffset < numwadfiles)
	{
		// Success, return with the conversion.
		return ((netwad + localoffset) << 16) | (lump & UINT16_MAX);
	}

	// Death!!
	return LUMPERROR;
}

size_t W_LumpLengthPwad(UINT16 wad, UINT16 lump)
{
	if (!TestValidLump(wad, lump))
		return 0;
	return wadfiles[wad]->lumpinfo[lump].size;
}

/** Returns the buffer size needed to load the given lump.
  *
  * \param lump Lump number to look at.
  * \return Buffer size needed, in bytes.
  */
size_t W_LumpLength(lumpnum_t lumpnum)
{
	return W_LumpLengthPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum));
}

//
// W_IsLumpWad
// Is the lump a WAD? (presumably in a PK3)
//
boolean W_IsLumpWad(lumpnum_t lumpnum)
{
	if (wadfiles[WADFILENUM(lumpnum)]->type == RET_PK3)
	{
		const char *lumpfullName = (wadfiles[WADFILENUM(lumpnum)]->lumpinfo + LUMPNUM(lumpnum))->fullname;

		if (strlen(lumpfullName) < 4)
			return false; // can't possibly be a WAD can it?
		return !strnicmp(lumpfullName + strlen(lumpfullName) - 4, ".wad", 4);
	}

	return false; // WADs should never be inside non-PK3s as far as SRB2 is concerned
}

//
// W_IsLumpFolder
// Is the lump a folder? (in a PK3 obviously)
//
boolean W_IsLumpFolder(UINT16 wad, UINT16 lump)
{
	if (wadfiles[wad]->type == RET_PK3)
	{
		const char *name = wadfiles[wad]->lumpinfo[lump].fullname;

		return (name[strlen(name)-1] == '/'); // folders end in '/'
	}

	return false; // non-PK3s don't have folders
}

#ifdef HAVE_ZLIB
/* report a zlib or i/o error */
void zerr(int ret)
{
    CONS_Printf("zpipe: ");
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            CONS_Printf("error reading stdin\n");
        if (ferror(stdout))
            CONS_Printf("error writing stdout\n");
        break;
    case Z_STREAM_ERROR:
        CONS_Printf("invalid compression level\n");
        break;
    case Z_DATA_ERROR:
        CONS_Printf("invalid or incomplete deflate data\n");
        break;
    case Z_MEM_ERROR:
        CONS_Printf("out of memory\n");
        break;
    case Z_VERSION_ERROR:
        CONS_Printf("zlib version mismatch!\n");
    }
}
#endif

/** Reads bytes from the head of a lump.
  * Note: If the lump is compressed, the whole thing has to be read anyway.
  *
  * \param wad Wad number to read from.
  * \param lump Lump number to read from.
  * \param dest Buffer in memory to serve as destination.
  * \param size Number of bytes to read.
  * \param offest Number of bytes to offset.
  * \return Number of bytes read (should equal size).
  * \sa W_ReadLump, W_RawReadLumpHeader
  */
size_t W_ReadLumpHeaderPwad(UINT16 wad, UINT16 lump, void *dest, size_t size, size_t offset)
{
	size_t lumpsize;
	lumpinfo_t *l;
	FILE *handle;

	if (!TestValidLump(wad,lump))
		return 0;

	lumpsize = wadfiles[wad]->lumpinfo[lump].size;
	// empty resource (usually markers like S_START, F_END ..)
	if (!lumpsize || lumpsize<offset)
		return 0;

	// zero size means read all the lump
	if (!size || size+offset > lumpsize)
		size = lumpsize - offset;

	// Let's get the raw lump data.
	// We setup the desired file handle to read the lump data.
	l = wadfiles[wad]->lumpinfo + lump;
	handle = wadfiles[wad]->handle;
	fseek(handle, (long)(l->position + offset), SEEK_SET);

	// But let's not copy it yet. We support different compression formats on lumps, so we need to take that into account.
	switch(wadfiles[wad]->lumpinfo[lump].compression)
	{
	case CM_NOCOMPRESSION:		// If it's uncompressed, we directly write the data into our destination, and return the bytes read.
#ifdef NO_PNG_LUMPS
		{
			size_t bytesread = fread(dest, 1, size, handle);
			if (Picture_IsLumpPNG((UINT8 *)dest, bytesread))
				Picture_ThrowPNGError(l->fullname, wadfiles[wad]->filename);
			return bytesread;
		}
#else
		return fread(dest, 1, size, handle);
#endif
	case CM_LZF:		// Is it LZF compressed? Used by ZWADs.
		{
#ifdef ZWAD
			char *rawData; // The lump's raw data.
			char *decData; // Lump's decompressed real data.
			size_t retval; // Helper var, lzf_decompress returns 0 when an error occurs.

			rawData = static_cast<char*>(Z_Malloc(l->disksize, PU_STATIC, NULL));
			decData = static_cast<char*>(Z_Malloc(l->size, PU_STATIC, NULL));

			if (fread(rawData, 1, l->disksize, handle) < l->disksize)
				I_Error("wad %d, lump %d: cannot read compressed data", wad, lump);
			retval = lzf_decompress(rawData, l->disksize, decData, l->size);
#ifndef AVOID_ERRNO
			if (retval == 0) // If this was returned, check if errno was set
			{
				// errno is a global var set by the lzf functions when something goes wrong.
				if (errno == E2BIG)
					I_Error("wad %d, lump %d: compressed data too big (bigger than %s)", wad, lump, sizeu1(l->size));
				else if (errno == EINVAL)
					I_Error("wad %d, lump %d: invalid compressed data", wad, lump);
			}
			// Otherwise, fall back on below error (if zero was actually the correct size then ???)
#endif
			if (retval != l->size)
			{
				I_Error("wad %d, lump %d: decompressed to wrong number of bytes (expected %s, got %s)", wad, lump, sizeu1(l->size), sizeu2(retval));
			}

			if (!decData) // Did we get no data at all?
				return 0;
			M_Memcpy(dest, decData + offset, size);
			Z_Free(rawData);
			Z_Free(decData);
#ifdef NO_PNG_LUMPS
			if (Picture_IsLumpPNG((UINT8 *)dest, size))
				Picture_ThrowPNGError(l->fullname, wadfiles[wad]->filename);
#endif
			return size;
#else
			//I_Error("ZWAD files not supported on this platform.");
			return 0;
#endif

		}
#ifdef HAVE_ZLIB
	case CM_DEFLATE: // Is it compressed via DEFLATE? Very common in ZIPs/PK3s, also what most doom-related editors support.
		{
			UINT8 *rawData; // The lump's raw data.
			UINT8 *decData; // Lump's decompressed real data.

			int zErr; // Helper var.
			z_stream strm;
			unsigned long rawSize = l->disksize;
			unsigned long decSize = size;

			rawData = static_cast<UINT8*>(Z_Malloc(rawSize, PU_STATIC, NULL));
			decData = static_cast<UINT8*>(dest);

			if (fread(rawData, 1, rawSize, handle) < rawSize)
				I_Error("wad %d, lump %d: cannot read compressed data", wad, lump);

			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;

			strm.total_in = strm.avail_in = rawSize;
			strm.total_out = strm.avail_out = decSize;

			strm.next_in = rawData;
			strm.next_out = decData;

			zErr = inflateInit2(&strm, -15);
			if (zErr == Z_OK)
			{
				zErr = inflate(&strm, Z_SYNC_FLUSH);
				if (zErr != Z_OK && zErr != Z_STREAM_END)
				{
					size = 0;
					zerr(zErr);
				}
				(void)inflateEnd(&strm);
			}
			else
			{
				size = 0;
				zerr(zErr);
			}

			Z_Free(rawData);

#ifdef NO_PNG_LUMPS
			if (Picture_IsLumpPNG((UINT8 *)dest, size))
				Picture_ThrowPNGError(l->fullname, wadfiles[wad]->filename);
#endif
			return size;
		}
#endif
	default:
		I_Error("wad %d, lump %d: unsupported compression type!", wad, lump);
	}
	return 0;
}

size_t W_ReadLumpHeader(lumpnum_t lumpnum, void *dest, size_t size, size_t offset)
{
	return W_ReadLumpHeaderPwad(WADFILENUM(lumpnum), LUMPNUM(lumpnum), dest, size, offset);
}

/** Reads a lump into memory.
  *
  * \param lump Lump number to read from.
  * \param dest Buffer in memory to serve as destination. Size must be >=
  *             W_LumpLength().
  * \sa W_ReadLumpHeader
  */
void W_ReadLump(lumpnum_t lumpnum, void *dest)
{
	W_ReadLumpHeaderPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),dest,0,0);
}

void W_ReadLumpPwad(UINT16 wad, UINT16 lump, void *dest)
{
	W_ReadLumpHeaderPwad(wad, lump, dest, 0, 0);
}

// ==========================================================================
// W_CacheLumpNum
// ==========================================================================
void *W_CacheLumpNumPwad(UINT16 wad, UINT16 lump, INT32 tag)
{
	lumpcache_t *lumpcache;

	if (!TestValidLump(wad,lump))
		return NULL;

	lumpcache = wadfiles[wad]->lumpcache;
	if (!lumpcache[lump])
	{
		void *ptr = Z_Malloc(W_LumpLengthPwad(wad, lump), tag, &lumpcache[lump]);
		W_ReadLumpHeaderPwad(wad, lump, ptr, 0, 0);  // read the lump in full
	}
	else
		Z_ChangeTag(lumpcache[lump], tag);

	return lumpcache[lump];
}

void *W_CacheLumpNum(lumpnum_t lumpnum, INT32 tag)
{
	return W_CacheLumpNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),tag);
}

//
// W_CacheLumpNumForce
//
// Forces the lump to be loaded, even if it already is!
//
void *W_CacheLumpNumForce(lumpnum_t lumpnum, INT32 tag)
{
	UINT16 wad, lump;
	void *ptr;

	wad = WADFILENUM(lumpnum);
	lump = LUMPNUM(lumpnum);

	if (!TestValidLump(wad,lump))
		return NULL;

	ptr = Z_Malloc(W_LumpLengthPwad(wad, lump), tag, NULL);
	W_ReadLumpHeaderPwad(wad, lump, ptr, 0, 0);  // read the lump in full

	return ptr;
}

//
// W_IsLumpCached
//
// If a lump is already cached return true, otherwise
// return false.
//
// no outside code uses the PWAD form, for now
static inline boolean W_IsLumpCachedPWAD(UINT16 wad, UINT16 lump, void *ptr)
{
	void *lcache;

	if (!TestValidLump(wad, lump))
		return false;

	lcache = wadfiles[wad]->lumpcache[lump];

	if (ptr)
	{
		if (ptr == lcache)
			return true;
	}
	else if (lcache)
		return true;

	return false;
}

boolean W_IsLumpCached(lumpnum_t lumpnum, void *ptr)
{
	return W_IsLumpCachedPWAD(WADFILENUM(lumpnum),LUMPNUM(lumpnum), ptr);
}

//
// W_IsPatchCached
//
// If a patch is already cached return true, otherwise
// return false.
//
// no outside code uses the PWAD form, for now
static inline boolean W_IsPatchCachedPWAD(UINT16 wad, UINT16 lump, void *ptr)
{
	void *lcache;

	if (!TestValidLump(wad, lump))
		return false;

	lcache = wadfiles[wad]->patchcache[lump];

	if (ptr)
	{
		if (ptr == lcache)
			return true;
	}
	else if (lcache)
		return true;

	return false;
}

boolean W_IsPatchCached(lumpnum_t lumpnum, void *ptr)
{
	return W_IsPatchCachedPWAD(WADFILENUM(lumpnum),LUMPNUM(lumpnum), ptr);
}

// ==========================================================================
// W_CacheLumpName
// ==========================================================================
void *W_CacheLumpName(const char *name, INT32 tag)
{
	return W_CacheLumpNum(W_GetNumForName(name), tag);
}

// ==========================================================================
//                                         CACHING OF GRAPHIC PATCH RESOURCES
// ==========================================================================

// Graphic 'patches' are loaded, and if necessary, converted into the format
// the most useful for the current rendermode. For software renderer, the
// graphic patches are kept as is. For the hardware renderer, graphic patches
// are 'unpacked', and are kept into the cache in that unpacked format, and
// the heap memory cache then acts as a 'level 2' cache just after the
// graphics card memory.

//
// Cache a patch into heap memory, convert the patch format as necessary
//

static void *MakePatch(void *lumpdata, size_t size, INT32 tag, void *cache)
{
	void *ptr, *dest;
	size_t len = size;

	ptr = lumpdata;

#ifndef NO_PNG_LUMPS
	if (Picture_IsLumpPNG((UINT8 *)lumpdata, len))
	{
		ptr = Picture_PNGConvert((UINT8 *)lumpdata, PICFMT_DOOMPATCH, NULL, NULL, NULL, NULL, len, &len, PICFLAGS_NONE);
	}
#endif

	dest = Z_Calloc(sizeof(patch_t), tag, cache);

	Patch_Create(static_cast<softwarepatch_t*>(ptr), len, dest);

	{
		patch_t* patch = (patch_t*) ptr;

		if (patch->width > 2048 || patch->height > 2048)
		{
			// This is INTENTIONAL. Even if software can handle it, very old GL hardware will not.
			// For the sake of a compatibility baseline, we will not allow anything larger than this.
			I_Error("Patch size cannot be greater than 2048x2048!");
		}
	}

	return dest;
}

void *W_CacheSoftwarePatchNumPwad(UINT16 wad, UINT16 lump, INT32 tag)
{
	lumpcache_t *lumpcache = NULL;

	if (!TestValidLump(wad, lump))
		return NULL;

	lumpcache = wadfiles[wad]->patchcache;

	if (!lumpcache[lump])
	{
		size_t len = W_LumpLengthPwad(wad, lump);
		void *lumpdata = Z_Malloc(len, PU_STATIC, NULL);

		// read the lump in full
		W_ReadLumpHeaderPwad(wad, lump, lumpdata, 0, 0);

		MakePatch(lumpdata, len, tag, &lumpcache[lump]);
		Z_Free(lumpdata);
	}
	else
		Z_ChangeTag(lumpcache[lump], tag);

	return lumpcache[lump];
}

void *W_CacheSoftwarePatchNum(lumpnum_t lumpnum, INT32 tag)
{
	return W_CacheSoftwarePatchNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),tag);
}

void *W_CachePatchNumPwad(UINT16 wad, UINT16 lump, INT32 tag)
{
	patch_t *patch;

	if (!TestValidLump(wad, lump))
		return NULL;

	patch = static_cast<patch_t*>(W_CacheSoftwarePatchNumPwad(wad, lump, tag));

#ifdef HWRENDER
	// Software-only compile cache the data without conversion
	if (rendermode != render_opengl)
#endif
		return (void *)patch;

#ifdef HWRENDER
	Patch_CreateGL(patch);
	return (void *)patch;
#endif
}

void *W_CachePatchNum(lumpnum_t lumpnum, INT32 tag)
{
	return W_CachePatchNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),tag);
}

void W_UnlockCachedPatch(void *patch)
{
	if (!patch)
		return;

	// The hardware code does its own memory management, as its patches
	// have different lifetimes from software's.
#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_UnlockCachedPatch((GLPatch_t *)((patch_t *)patch)->hardware);
	else
#endif
		Z_Unlock(patch);
}

void *W_CachePatchName(const char *name, INT32 tag)
{
	lumpnum_t num;

	num = W_CheckNumForName(name);

	if (num == LUMPERROR)
		return missingpat;
	return W_CachePatchNum(num, tag);
}

void *W_CachePatchLongName(const char *name, INT32 tag)
{
	lumpnum_t num;

	num = W_CheckNumForLongName(name);

	if (num == LUMPERROR)
		return missingpat;
	return W_CachePatchNum(num, tag);
}

#ifndef NOMD5

/**
  * Prints an MD5 string into a human-readable textual format.
  *
  * \param md5 The md5 in binary form -- MD5_LEN (16) bytes.
  * \param buf Where to print the textual form. Needs 2*MD5_LEN+1 (33) bytes.
  * \author Graue <graue@oceanbase.org>
  */
#define MD5_FORMAT \
	"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
static void PrintMD5String(const UINT8 *md5, char *buf)
{
	snprintf(buf, 2*MD5_LEN+1, MD5_FORMAT,
		md5[0], md5[1], md5[2], md5[3],
		md5[4], md5[5], md5[6], md5[7],
		md5[8], md5[9], md5[10], md5[11],
		md5[12], md5[13], md5[14], md5[15]);
}
#endif

// Verify versions for different archive
// formats. checklist assumed to be valid.

static int
W_VerifyName (const char *name, lumpchecklist_t *checklist, boolean status)
{
	size_t j;
	for (j = 0; checklist[j].len && checklist[j].name; ++j)
	{
		if (( strncasecmp(name, checklist[j].name,
						checklist[j].len) != false ) == status)
		{
			return true;
		}
	}
	return false;
}

static int
W_VerifyWAD (FILE *fp, lumpchecklist_t *checklist, boolean status)
{
	size_t i;

	// assume wad file
	wadinfo_t header;
	filelump_t lumpinfo;

	// read the header
	if (fread(&header, 1, sizeof header, fp) == sizeof header
			&& header.numlumps < INT16_MAX
			&& strncmp(header.identification, "ZWAD", 4)
			&& strncmp(header.identification, "IWAD", 4)
			&& strncmp(header.identification, "PWAD", 4)
			&& strncmp(header.identification, "SDLL", 4))
	{
		return true;
	}

	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);

	// let seek to the lumpinfo list
	if (fseek(fp, header.infotableofs, SEEK_SET) == -1)
		return true;

	for (i = 0; i < header.numlumps; i++)
	{
		// fill in lumpinfo for this wad file directory
		if (fread(&lumpinfo, sizeof (lumpinfo), 1 , fp) != 1)
			return true;

		lumpinfo.filepos = LONG(lumpinfo.filepos);
		lumpinfo.size = LONG(lumpinfo.size);

		if (lumpinfo.size == 0)
			continue;

		if (! W_VerifyName(lumpinfo.name, checklist, status))
			return false;
	}

	return true;
}

// List of blacklisted folders to use when checking the PK3
static lumpchecklist_t folderblacklist[] =
{
	{"Lua/", 4},
	{"SOC/", 4},
	{"Sprites/",  8},
	{"Textures/", 9},
	{"Patches/", 8},
	{"Flats/", 6},
	{"Fades/", 6},
	{NULL, 0},
};

static int
W_VerifyPK3 (FILE *fp, lumpchecklist_t *checklist, boolean status)
{
	int verified = true;

    zend_t zend;
    zlentry_t zlentry;

	long file_size;/* size of zip file */
	long data_size;/* size of data inside zip file */

	UINT16 numlumps;
	size_t i;

	char pat_central[] = {0x50, 0x4b, 0x01, 0x02, 0x00};
	char pat_end[] = {0x50, 0x4b, 0x05, 0x06, 0x00};

	char lumpname[9];

	// Haha the ResGetLumpsZip function doesn't
	// check for file errors, so neither will I.

	// Central directory bullshit

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);

	if (!ResFindSignature(fp, pat_end, std::max(0l, ftell(fp) - (22 + 65536))))
		return true;

	fseek(fp, -4, SEEK_CUR);
	if (fread(&zend, 1, sizeof zend, fp) < sizeof zend)
		return true;

	data_size = sizeof zend;

	numlumps = SHORT(zend.entries);

	fseek(fp, LONG(zend.cdiroffset), SEEK_SET);

	char *cdir = static_cast<char*>(malloc(LONG(zend.cdirsize)));
	auto cdir_finally = srb2::finally([cdir] { free(cdir); });

	if (fread(cdir, 1, LONG(zend.cdirsize), fp) < static_cast<UINT32>(LONG(zend.cdirsize)))
		return true;

	size_t offset = 0;

	for (i = 0; i < numlumps; i++)
	{
		zentry_t *zentry = reinterpret_cast<zentry_t*>(cdir + offset);
		char* fullname;
		char* trimname;
		char* dotpos;

		if (memcmp(zentry->signature, pat_central, 4) != 0)
			return true;

		if (verified == true)
		{
			fullname = static_cast<char*>(malloc(SHORT(zentry->namelen) + 1));
			strlcpy(fullname, (char*)(zentry + 1), SHORT(zentry->namelen) + 1);

			// Strip away file address and extension for the 8char name.
			if ((trimname = strrchr(fullname, '/')) != 0)
				trimname++;
			else
				trimname = fullname; // Care taken for root files.

			if (*trimname) // Ignore directories, well kinda
			{
				if ((dotpos = strrchr(trimname, '.')) == 0)
					dotpos = fullname + strlen(fullname); // Watch for files without extension.

				memset(lumpname, '\0', 9); // Making sure they're initialized to 0. Is it necessary?
				strncpy(lumpname, trimname, std::min(static_cast<std::ptrdiff_t>(8), dotpos - trimname));

				if (! W_VerifyName(lumpname, checklist, status))
					verified = false;

				// Check for directories next, if it's blacklisted it will return false
				else if (W_VerifyName(fullname, folderblacklist, status))
					verified = false;
			}

			free(fullname);
		}

		offset += sizeof *zentry + SHORT(zentry->namelen) + SHORT(zentry->xtralen) + SHORT(zentry->commlen);

		data_size +=
			sizeof *zentry + SHORT(zentry->namelen) + SHORT(zentry->xtralen) + SHORT(zentry->commlen);

		if (fseek(fp, LONG(zentry->offset), SEEK_SET) != 0)
			return true;

		if (fread(&zlentry, 1, sizeof(zlentry_t), fp) < sizeof (zlentry_t))
			return true;

		data_size +=
			sizeof zlentry + SHORT(zlentry.namelen) + SHORT(zlentry.xtralen) + LONG(zlentry.compsize);
	}

	if (data_size < file_size)
	{
		const char * error = "ZIP file has holes (%ld extra bytes)\n";
		CONS_Alert(CONS_ERROR, error, (file_size - data_size));
		return -1;
	}
	else if (data_size > file_size)
	{
		const char * error = "Reported size of ZIP file contents exceeds file size (%ld extra bytes)\n";
		CONS_Alert(CONS_ERROR, error, (data_size - file_size));
		return -1;
	}
	else
	{
		return verified;
	}
}


/** Checks a wad for lumps other than music and sound.
  * Used during game load to verify music.dta is a good file and during a
  * netgame join (on the server side) to see if a wad is important enough to
  * be sent.
  *
  * \param filename Filename of the wad to check.
  * \param exit_on_error Whether to exit upon file error.
  * \return 1 if file contains only music/sound lumps, 0 if it contains other
  *         stuff (maps, sprites, dehacked lumps, and so on). -1 if there no
  *         file exists with that filename
  * \author Alam Arias
  */
int W_VerifyNMUSlumps(const char *filename, FILE *handle, boolean exit_on_error)
{
	lumpchecklist_t NMUSlist[] =
	{
		{"O_", 2}, // Digital music
		{"DS", 2}, // Sound effects

		{"ENDOOM", 6}, // ENDOOM text lump
		{"PLAYPAL", 7}, // Palette
		{"COLORMAP", 8}, // Colormap
		{"PAL", 3}, // Palette changes
		{"CLM", 3}, // Colormap changes
		{"TRANS", 5}, // Translucency map

		{"CONSBACK", 8}, // Console Background graphic

		{"SAVE", 4}, // Save Select graphics here and below
		{"BLACXLVL", 8},
		{"GAMEDONE", 8},
		{"CONT", 4}, // Continue icons on saves (probably not used anymore)
		{"STNONEX", 7}, // "X" graphic
		{"ULTIMATE", 8}, // Ultimate no-save

		{"CRFNT", 5}, // Sonic 1 font changes
		{"NTFNT", 5}, // Character Select font changes
		{"NTFNO", 5}, // Character Select font (outline)
		{"LTFNT", 5}, // Level title font changes
		{"TTL", 3}, // Act number changes
		{"STCFN", 5}, // Console font changes
		{"TNYFN", 5}, // Tiny console font changes

		{"STLIVE", 6}, // Life graphics, background and the "X" that shows under skin's HUDNAME
		{"CROSHAI", 7}, // First person crosshairs
		{"INTERSC", 7}, // Default intermission backgrounds (co-op)
		{"STT", 3}, // Acceptable HUD changes (Score Time Rings)
		{"YB_", 3}, // Intermission graphics, goes with the above
		{"M_", 2}, // As does menu stuff
		{"MKFNT", 5}, // Kart font changes
		{"K_", 2}, // Kart graphic changes
		{"MUSICDEF", 8}, // Kart song definitions
		{"RVFXANIM", 8}, // Photosensitivity texture animation changes

		{"TLG_", 4}, // Generic button legends

		{"MODINFO", 7}, // Addon metadata
		{"MODICON", 7}, // Addon icon

#ifdef HWRENDER
		{"SHADERS", 7},
		{"SH_", 3},
#endif
		{NULL, 0},
	};

	int status = 0;

	if (stricmp(&filename[strlen(filename) - 4], ".pk3") == 0)
	{
		status = W_VerifyPK3(handle, NMUSlist, false);
	}
	else
	{
		// detect wad file by the absence of the other supported extensions
		if (stricmp(&filename[strlen(filename) - 4], ".soc")
		&& stricmp(&filename[strlen(filename) - 4], ".lua"))
		{
			status = W_VerifyWAD(handle, NMUSlist, false);
		}
	}

	// repair file handle so we don't have to open a new one
	fseek(handle, 0, SEEK_SET);

	if (status == -1)
		W_InitFileError(filename, exit_on_error);

	return status;
}

void W_InitShaderLookup(const char *filename)
{
	I_Assert(g_shaderspk3file == NULL);

	FILE* handle;
	char filename_buf[2048];

	g_shaderspk3file = NULL;
	g_shaderspk3lumps = NULL;
	g_shaderspk3numlumps = 0;

	strncpy(filename_buf, filename, 2048);
	filename_buf[2048 - 1] = '\0';

	if ((handle = fopen(filename_buf, "rb")) == NULL)
	{
		nameonly(filename_buf);

		if (!findfile(filename_buf, "data", NULL, true))
			return;

		if ((handle = fopen(filename_buf, "rb")) == NULL)
			return;
	}

	// It is acceptable to fail opening the pk3 lookup.
	// The shader pk3 lookup is only needed to build a lookup directory of the zip
	// for later. We always check for the flat file shader anyway.

	UINT16 numlumps;
	lumpinfo_t *shader_lumps = ResGetLumpsZip(handle, &numlumps);
	if (shader_lumps == NULL)
	{
		return;
	}
	g_shaderspk3file = handle;
	g_shaderspk3lumps = shader_lumps;
	g_shaderspk3numlumps = numlumps;
}

static boolean ReadShaderFlatFile(const char *filename, size_t *size, void *dest)
{
	FILE* flat_handle = NULL;
	char filename_buf[2048];
	char filename_only_buf[512];

	strncpy(filename_buf, filename, 2048);
	filename_buf[2048 - 1] = '\0';

	if ((flat_handle = fopen(filename_buf, "rb")) == NULL)
	{
		nameonly(filename_buf);
		strncpy(filename_only_buf, filename_buf, 512);
		filename_only_buf[512 - 1] = '\0';
		sprintf(filename_buf, "%s/shaders/%s", srb2path, filename_only_buf);
		if ((flat_handle = fopen(filename_buf, "rb")) == NULL)
		{
			return false;
		}
	}

	// idk, pray it's not >2gb. ansi c made mistakes
	fseek(flat_handle, 0, SEEK_END);
	*size = ftell(flat_handle);
	fseek(flat_handle, 0, SEEK_SET);
	if (dest)
	{
		fread(dest, *size, 1, flat_handle);
	}

	fclose(flat_handle);
	return true;
}

boolean W_ReadShader(const char *filename, size_t *size, void *dest)
{
	I_Assert(filename != NULL);
	I_Assert(size != NULL);

	if (ReadShaderFlatFile(filename, size, dest))
	{
		return true;
	}

	UINT32 hash = quickncasehash(filename, 512);

	lumpinfo_t* lump = NULL;
	for (int i = 0 ; i < g_shaderspk3numlumps; ++i)
	{
		lump = &g_shaderspk3lumps[i];
		UINT32 lumpnamehash = quickncasehash(lump->fullname, 512);
		if (lumpnamehash == hash)
		{
			break;
		}
		lump = NULL;
	}

	if (lump == NULL)
	{
		return false;
	}

	size_t sizelocal = lump->size;
	if (dest == NULL)
	{
		*size = sizelocal;
		return true;
	}

	if (fseek(g_shaderspk3file, lump->position, SEEK_SET) != 0)
		I_Error("Failed to seek shaders pk3 to offset of file: %s", strerror(errno));

	switch (lump->compression)
	{
	case CM_NOCOMPRESSION:
		if (fread(dest, sizelocal, 1, g_shaderspk3file) != 0)
			I_Error("Failed to read file in shaders pk3: %s", strerror(errno));
		break;
#ifdef HAVE_ZLIB
	case CM_DEFLATE:
	{
		UINT8 *rawData; // The lump's raw data.
		UINT8 *decData; // Lump's decompressed real data.

		int zErr; // Helper var.
		z_stream strm;
		unsigned long rawSize = lump->disksize;
		unsigned long decSize = (unsigned long)*size;

		rawData = static_cast<UINT8*>(Z_Malloc(rawSize, PU_STATIC, NULL));
		decData = static_cast<UINT8*>(dest);

		if (fread(rawData, 1, rawSize, g_shaderspk3file) < rawSize)
			I_Error("Failed to read compressed file in shaders pk3: %s", strerror(errno));

		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;

		strm.total_in = strm.avail_in = rawSize;
		strm.total_out = strm.avail_out = decSize;

		strm.next_in = rawData;
		strm.next_out = decData;

		zErr = inflateInit2(&strm, -15);
		if (zErr == Z_OK)
		{
			zErr = inflate(&strm, Z_SYNC_FLUSH);
			if (zErr != Z_OK && zErr != Z_STREAM_END)
			{
				zerr(zErr);
			}
			(void)inflateEnd(&strm);
		}
		else
		{
			size = 0;
			zerr(zErr);
		}

		Z_Free(rawData);
	}
		break;
#endif
	default:
		return false;
	}

	*size = sizelocal;
	return true;
}

/** \brief Generates a virtual resource used for level data loading.
 *
 * \param lumpnum_t reference
 * \return Virtual resource
 *
 */
virtres_t* vres_GetMap(lumpnum_t lumpnum)
{
	UINT32 i;
	virtres_t* vres = NULL;
	virtlump_t* vlumps = NULL;
	size_t numlumps = 0;

	if (lastloadedmaplumpnum == lumpnum && curmapvirt != NULL)
	{
		// Avoid duplicating all our hard work.
		return curmapvirt;
	}

	if (W_IsLumpWad(lumpnum))
	{
		UINT32 realentry;
		size_t *vsizecache;

		// Remember that we're assuming that the WAD will have a specific set of lumps in a specific order.
		UINT8 *wadData = static_cast<UINT8*>(W_CacheLumpNum(lumpnum, PU_LEVEL));
		filelump_t *fileinfo = (filelump_t *)(wadData + LONG(((wadinfo_t *)wadData)->infotableofs));

		i = LONG(((wadinfo_t *)wadData)->numlumps);
		vsizecache = static_cast<size_t*>(Z_Malloc(sizeof(size_t)*i, PU_LEVEL, NULL));

		for (realentry = 0; realentry < i; realentry++)
		{
			vsizecache[realentry] = (size_t)(LONG(((filelump_t *)(fileinfo + realentry))->size));

			if (!vsizecache[realentry])
				continue;

			numlumps++;
		}

		vlumps = static_cast<virtlump_t*>(Z_Malloc(sizeof(virtlump_t)*numlumps, PU_LEVEL, NULL));

		// Build the lumps, skipping over empty entries.
		for (i = 0, realentry = 0; i < numlumps; realentry++)
		{
			if (vsizecache[realentry] == 0)
				continue;
			vlumps[i].size = vsizecache[realentry];
			// Play it safe with the name in this case.
			memcpy(vlumps[i].name, (fileinfo + realentry)->name, 8);
			vlumps[i].name[8] = '\0';
			vlumps[i].data = static_cast<UINT8*>(
				Z_Malloc(vlumps[i].size, PU_LEVEL, NULL) // This is memory inefficient, sorry about that.
			);
			memcpy(vlumps[i].data, wadData + LONG((fileinfo + realentry)->filepos), vlumps[i].size);
			i++;
		}

		Z_Free(vsizecache);
		Z_Free(wadData);
	}
	else
	{
		// Count number of lumps until the end of resource OR up until next 0-length lump.
		lumpnum_t lumppos = lumpnum + 1;
		for (i = LUMPNUM(lumppos); i < wadfiles[WADFILENUM(lumpnum)]->numlumps; i++, lumppos++, numlumps++)
		{
			if (W_LumpLength(lumppos) > 0)
				continue;

			break;
		}
		numlumps++;

		vlumps = static_cast<virtlump_t*>(Z_Malloc(sizeof(virtlump_t)*numlumps, PU_LEVEL, NULL));
		for (i = 0; i < numlumps; i++, lumpnum++)
		{
			vlumps[i].size = W_LumpLength(lumpnum);
			memcpy(vlumps[i].name, W_CheckNameForNum(lumpnum), 8);
			vlumps[i].name[8] = '\0';
			vlumps[i].data = static_cast<UINT8*>(W_CacheLumpNum(lumpnum, PU_LEVEL));
		}
	}
	vres = static_cast<virtres_t*>(Z_Malloc(sizeof(virtres_t), PU_LEVEL, NULL));
	vres->vlumps = vlumps;
	vres->numlumps = numlumps;

	return vres;
}

/** \brief Frees zone memory for a given virtual resource.
 *
 * \param Virtual resource
 */
void vres_Free(virtres_t* vres)
{
	if (vres == curmapvirt)
	{
		// No-sell multiple references.
		return;
	}

	while (vres->numlumps--)
	{
		if (vres->vlumps[vres->numlumps].data)
		{
			Z_Free(vres->vlumps[vres->numlumps].data);
		}
	}
	Z_Free(vres->vlumps);
	Z_Free(vres);
}

/** (Debug) Prints lumps from a virtual resource into console.
 */
/*
static void vres_Diag(const virtres_t* vres)
{
	UINT32 i;
	for (i = 0; i < vres->numlumps; i++)
		CONS_Printf("%s\n", vres->vlumps[i].name);
}
*/

/** \brief Finds a lump in a given virtual resource.
 *
 * \param Virtual resource
 * \param Lump name to look for
 * \return Virtual lump if found, NULL otherwise
 *
 */
virtlump_t* vres_Find(const virtres_t* vres, const char* name)
{
	UINT32 i;
	for (i = 0; i < vres->numlumps; i++)
		if (fastcmp(name, vres->vlumps[i].name))
			return &vres->vlumps[i];
	return NULL;
}

/** \brief Gets patch from given virtual lump
 *
 * \param Virtual lump
 * \return Patch data
 *
 */
void *vres_GetPatch(virtlump_t *vlump, INT32 tag)
{
	patch_t *patch;

	if (!vlump)
		return NULL;

	patch = static_cast<patch_t*>(MakePatch(vlump->data, vlump->size, tag, NULL));

#ifdef HWRENDER
	// Software-only compile cache the data without conversion
	if (rendermode == render_soft || rendermode == render_none)
#endif
		return (void *)patch;

#ifdef HWRENDER
	Patch_CreateGL(patch);
	return (void *)patch;
#endif
}
