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
/// \file  m_misc.cpp
/// \brief Commonly used routines
///        Default config file, PCX screenshots, file i/o

#ifdef __GNUC__

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
// Ignore "argument might be clobbered by longjmp" warning in GCC
// (if libpng is compiled with setjmp error handling)
#pragma GCC diagnostic ignored "-Wclobbered"
#endif

#include <unistd.h>
#endif


#include <algorithm>
#include <filesystem>
#include <errno.h>

// Extended map support.
#include <ctype.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#include <immintrin.h>
#define NEED_INTEL_DENORMAL_BIT 1
#endif

#include "doomdef.h"
#include "g_game.h"
#include "m_misc.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "v_video.h"
#include "z_zone.h"
#include "g_input.h"
#include "i_time.h"
#include "i_video.h"
#include "d_main.h"
#include "m_argv.h"
#include "i_system.h"
#include "command.h" // cv_execversion

#include "m_anigif.h"
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
#include "m_avrecorder.h"
#include "m_avrecorder.hpp"
#endif

// So that the screenshot menu auto-updates...
#include "k_menu.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#ifdef HAVE_SDL
#include "sdl/hwsym_sdl.h"
#ifdef __linux__
#ifndef _LARGEFILE64_SOURCE
typedef off_t off64_t;
#endif
#endif
#endif

#if defined(__MINGW32__) && ((__GNUC__ > 7) || (__GNUC__ == 6 && __GNUC_MINOR__ >= 3)) && (__GNUC__ < 8)
#define PRIdS "u"
#elif defined (_WIN32)
// pedantic: %I is nonstandard, is it ok to assume
// unsigned int?
//#define PRIdS "Iu"
#ifdef _WIN64
#define PRIdS "lu"
#else
#define PRIdS "u"
#endif
#else
#define PRIdS "zu"
#endif

#ifdef HAVE_PNG

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

 #include "zlib.h"
 #include "png.h"
 #if (PNG_LIBPNG_VER_MAJOR > 1) || (PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR >= 4)
  #define NO_PNG_DEBUG // 1.4.0 move png_debug to pngpriv.h
 #endif
 #ifdef PNG_WRITE_SUPPORTED
  #define USE_PNG // Only actually use PNG if write is supported.
  #if defined (PNG_WRITE_APNG_SUPPORTED) //|| !defined(PNG_STATIC)
    #include "apng.h"
    #define USE_APNG
  #endif
  // See hardware/hw_draw.c for a similar check to this one.
 #endif
#endif

extern "C" CV_PossibleValue_t lossless_recorder_cons_t[];
CV_PossibleValue_t lossless_recorder_cons_t[] = {{MM_GIF, "GIF"}, {MM_APNG, "aPNG"}, {MM_SCREENSHOT, "Screenshots"}, {0, NULL}};

extern "C" CV_PossibleValue_t zlib_mem_level_t[];
CV_PossibleValue_t zlib_mem_level_t[] = {
	{1, "(Min Memory) 1"},
	{2, "2"}, {3, "3"}, {4, "4"}, {5, "5"}, {6, "6"}, {7, "7"},
	{8, "(Optimal) 8"}, //libpng Default
	{9, "(Max Memory) 9"}, {0, NULL}};

extern "C" CV_PossibleValue_t zlib_level_t[];
CV_PossibleValue_t zlib_level_t[] = {
	{0, "No Compression"},  //Z_NO_COMPRESSION
	{1, "(Fastest) 1"}, //Z_BEST_SPEED
	{2, "2"}, {3, "3"}, {4, "4"}, {5, "5"},
	{6, "(Optimal) 6"}, //Zlib Default
	{7, "7"}, {8, "8"},
	{9, "(Maximum) 9"}, //Z_BEST_COMPRESSION
	{0, NULL}};

extern "C" CV_PossibleValue_t zlib_strategy_t[];
CV_PossibleValue_t zlib_strategy_t[] = {
	{0, "Normal"}, //Z_DEFAULT_STRATEGY
	{1, "Filtered"}, //Z_FILTERED
	{2, "Huffman Only"}, //Z_HUFFMAN_ONLY
	{3, "RLE"}, //Z_RLE
	{4, "Fixed"}, //Z_FIXED
	{0, NULL}};

extern "C" CV_PossibleValue_t zlib_window_bits_t[];
CV_PossibleValue_t zlib_window_bits_t[] = {
#ifdef WBITS_8_OK
	{8, "256"},
#endif
	{9, "512"}, {10, "1k"}, {11, "2k"}, {12, "4k"}, {13, "8k"},
	{14, "16k"}, {15, "32k"},
	{0, NULL}};

#ifdef USE_APNG
static boolean apng_downscale = false; // So nobody can do something dumb like changing cvars mid output
#endif

boolean takescreenshot = false; // Take a screenshot this tic

moviemode_t moviemode = MM_OFF;

g_takemapthumbnail_t g_takemapthumbnail = TMT_NO;

char joinedIPlist[NUMLOGIP][2][MAX_LOGIP];
char joinedIP[MAX_LOGIP];

// This initializes the above array to have NULL evrywhere it should.
void M_InitJoinedIPArray(void)
{
	UINT8 i;
	for (i=0; i < NUMLOGIP; i++)
	{
		joinedIPlist[i][0][0] = joinedIPlist[i][1][0] = '\0';
	}
}

// This adds an entry to the above array
void M_AddToJoinedIPs(char *address, char *servname)
{
	UINT8 i = 0;

	// Check for dupes...
	for (i = 0; i < NUMLOGIP-1; i++) // intentionally not < NUMLOGIP
	{
		// Check the addresses
		if (strcmp(joinedIPlist[i][0], address) == 0)
		{
			break;
		}
	}

	CONS_Printf("Adding %s (%s) to list of manually joined IPs\n", servname, address);

	// Start by moving every IP up 1 slot (dropping the last IP in the table)
	for (; i; i--)
	{
		strlcpy(joinedIPlist[i][0], joinedIPlist[i-1][0], MAX_LOGIP);
		strlcpy(joinedIPlist[i][1], joinedIPlist[i-1][1], MAX_LOGIP);
	}

	// and add the new IP at the start of the table!
	strlcpy(joinedIPlist[0][0], address, MAX_LOGIP);
	strlcpy(joinedIPlist[0][1], servname, MAX_LOGIP);
}

// ==========================================================================
//                         FILE INPUT / OUTPUT
// ==========================================================================

// some libcs has no access function, make our own
#if 0
int access(const char *path, int amode)
{
	int accesshandle = -1;
	FILE *handle = NULL;
	if (amode == 6) // W_OK|R_OK
		handle = fopen(path, "r+");
	else if (amode == 4) // R_OK
		handle = fopen(path, "r");
	else if (amode == 2) // W_OK
		handle = fopen(path, "a+");
	else if (amode == 0) //F_OK
		handle = fopen(path, "rb");
	if (handle)
	{
		accesshandle = 0;
		fclose(handle);
	}
	return accesshandle;
}
#endif


//
// FIL_WriteFile
//
#ifndef O_BINARY
#define O_BINARY 0
#endif

/** Writes out a file.
  *
  * \param name   Name of the file to write.
  * \param source Memory location to write from.
  * \param length How many bytes to write.
  * \return True on success, false on failure.
  */
boolean FIL_WriteFile(char const *name, const void *source, size_t length)
{
	FILE *handle = NULL;
	size_t count;

	//if (FIL_WriteFileOK(name))
		handle = fopen(name, "w+b");

	if (!handle)
		return false;

	count = fwrite(source, 1, length, handle);
	fclose(handle);

	if (count < length)
		return false;

	return true;
}

/** Reads in a file, appending a zero byte at the end.
  *
  * \param name   Filename to read.
  * \param buffer Pointer to a pointer, which will be set to the location of a
  *               newly allocated buffer holding the file's contents.
  * \return Number of bytes read, not counting the zero byte added to the end,
  *         or 0 on error.
  */
size_t FIL_ReadFileTag(char const *name, UINT8 **buffer, INT32 tag)
{
	FILE *handle = NULL;
	size_t count, length;
	UINT8 *buf;

	if (FIL_ReadFileOK(name))
		handle = fopen(name, "rb");

	if (!handle)
		return 0;

	fseek(handle,0,SEEK_END);
	length = ftell(handle);
	fseek(handle,0,SEEK_SET);

	buf = static_cast<UINT8*>(Z_Malloc(length + 1, tag, NULL));
	count = fread(buf, 1, length, handle);
	fclose(handle);

	if (count < length)
	{
		Z_Free(buf);
		return 0;
	}

	// append 0 byte for script text files
	buf[length] = 0;

	*buffer = buf;
	return length;
}

/** Makes a copy of a text file with all newlines converted into LF newlines.
  *
  * \param textfilename The name of the source file
  * \param binfilename The name of the destination file
  */
boolean FIL_ConvertTextFileToBinary(const char *textfilename, const char *binfilename)
{
	FILE *textfile;
	FILE *binfile;
	UINT8 buffer[1024];
	size_t count;
	boolean success;

	textfile = fopen(textfilename, "r");
	if (!textfile)
		return false;

	binfile = fopen(binfilename, "wb");
	if (!binfile)
	{
		fclose(textfile);
		return false;
	}

	do
	{
		count = fread(buffer, 1, sizeof(buffer), textfile);
		fwrite(buffer, 1, count, binfile);
	} while (count);

	success = !(ferror(textfile) || ferror(binfile));

	fclose(textfile);
	fclose(binfile);

	return success;
}

boolean FIL_RenameFile(char const *old_name, char const *new_name)
{
	int result = rename(old_name, new_name);
	return (result == 0);
}

/** Check if the filename exists
  *
  * \param name   Filename to check.
  * \return true if file exists, false if it doesn't.
  */
boolean FIL_FileExists(char const *name)
{
	return access(name,0)+1; //F_OK
}


/** Check if the filename OK to write
  *
  * \param name   Filename to check.
  * \return true if file write-able, false if it doesn't.
  */
boolean FIL_WriteFileOK(char const *name)
{
	return access(name,2)+1; //W_OK
}


/** Check if the filename OK to read
  *
  * \param name   Filename to check.
  * \return true if file read-able, false if it doesn't.
  */
boolean FIL_ReadFileOK(char const *name)
{
	return access(name,4)+1; //R_OK
}

/** Check if the filename OK to read/write
  *
  * \param name   Filename to check.
  * \return true if file (read/write)-able, false if it doesn't.
  */
boolean FIL_FileOK(char const *name)
{
	return access(name,6)+1; //R_OK|W_OK
}


/** Checks if a pathname has a file extension and adds the extension provided
  * if not.
  *
  * \param path      Pathname to check.
  * \param extension Extension to add if no extension is there.
  */
void FIL_DefaultExtension(char *path, const char *extension)
{
	char *src;

	// search for '.' from end to begin, add .EXT only when not found
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return; // it has an extension
		src--;
	}

	strcat(path, extension);
}

void FIL_ForceExtension(char *path, const char *extension)
{
	char *src;

	// search for '.' from end to begin, add .EXT only when not found
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
		{
			*src = '\0';
			break; // it has an extension
		}
		src--;
	}

	strcat(path, extension);
}

/** Checks if a filename extension is found.
  * Lump names do not contain dots.
  *
  * \param in String to check.
  * \return True if an extension is found, otherwise false.
  */
boolean FIL_CheckExtension(const char *in)
{
	while (*in++)
		if (*in == '.')
			return true;

	return false;
}

// LAST IPs JOINED LOG FILE!
// ...It won't be as overly engineered as the config file because let's be real there's 0 need to...

// Save the file:
void M_SaveJoinedIPs(void)
{
	FILE *f = NULL;
	UINT8 i;
	const char *filepath = va("%s" PATHSEP "%s", srb2home, IPLOGFILE);

	if (!*joinedIPlist[0][0])
		return;	// Don't bother, there's nothing to save.

	f = fopen(filepath, "w");

	if (!f)
	{
		CONS_Alert(CONS_WARNING, "Could not save recent IP list into %s\n", IPLOGFILE);
		return;
	}

	for (i = 0; i < NUMLOGIP; i++)
	{
		if (*joinedIPlist[i][0])
		{
			fprintf(f, "%s%s%s\n", joinedIPlist[i][0], IPLOGFILESEP, joinedIPlist[i][1]);
		}
	}

	fclose(f);
}


// Load the file:
void M_LoadJoinedIPs(void)
{
	FILE *f = NULL;
	UINT8 i = 0;
	char *filepath;
	char *s;
	char buffer[2*(MAX_LOGIP+1)];

	filepath = va("%s" PATHSEP "%s", srb2home, IPLOGFILE);
	f = fopen(filepath, "r");

	if (f == NULL)
		return;	// File doesn't exist? sure, just do nothing then.

	for (i = 0; fgets(buffer, (int)sizeof(buffer), f); i++)	// Don't let us write more than we can chew!
	{
		if (i >= NUMLOGIP)
			break;

		if (!*buffer || *buffer == '\n')
			break;

		s = strtok(buffer, IPLOGFILESEP);	// We got the address
		strlcpy(joinedIPlist[i][0], s, MAX_LOGIP);

		s = strtok(NULL, IPLOGFILESEP);	// Let's get rid of this awful \n while we're here!

		if (s)
		{
			UINT16 j = 1;
			//strcpy(joinedIPlist[i][1], s); -- get rid of \n too...
			char *c = joinedIPlist[i][1];
			while (*s && *s != '\n' && j < MAX_LOGIP)
			{
				*c = *s;
				s++;
				c++;
				j++;
			}
			*c = '\0';
		}
	}
	fclose(f);	// We're done here
}


// ==========================================================================
//                        CONFIGURATION FILE
// ==========================================================================

//
// DEFAULTS
//

char configfile[MAX_WADPATH];

// ==========================================================================
//                          CONFIGURATION
// ==========================================================================
static boolean gameconfig_loaded = false; // true once config.cfg loaded AND executed

/** Saves a player's config, possibly to a particular file.
  *
  * \sa Command_LoadConfig_f
  */
void Command_SaveConfig_f(void)
{
	char tmpstr[MAX_WADPATH];

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("saveconfig <filename[.cfg]> [-silent] : save config to a file\n"));
		return;
	}
	strcpy(tmpstr, COM_Argv(1));
	FIL_ForceExtension(tmpstr, ".cfg");

	M_SaveConfig(tmpstr);
	if (stricmp(COM_Argv(2), "-silent"))
		CONS_Printf(M_GetText("config saved as %s\n"), configfile);
}

/** Loads a game config, possibly from a particular file.
  *
  * \sa Command_SaveConfig_f, Command_ChangeConfig_f
  */
void Command_LoadConfig_f(void)
{
	UINT8 i;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("loadconfig <filename[.cfg]> : load config from a file\n"));
		return;
	}

	strcpy(configfile, COM_Argv(1));
	FIL_ForceExtension(configfile, ".cfg");

	// load default control
	G_ClearAllControlKeys();

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		G_ApplyControlScheme(i, gamecontroldefault);
	}

	// temporarily reset execversion to default
	CV_ToggleExecVersion(true);
	COM_BufInsertText(va("%s \"%s\"\n", cv_execversion.name, cv_execversion.defaultvalue));
	CV_InitFilterVar();

	// exec the config
	COM_BufInsertText(va("exec \"%s\" -immediate\n", configfile));

	// don't filter anymore vars and don't let this convsvar be changed
	COM_BufInsertText(va("%s \"%d\"\n", cv_execversion.name, EXECVERSION));
	CV_ToggleExecVersion(false);
}

/** Saves the current configuration and loads another.
  *
  * \sa Command_LoadConfig_f, Command_SaveConfig_f
  */
void Command_ChangeConfig_f(void)
{
	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("changeconfig <filename[.cfg]> : save current config and load another\n"));
		return;
	}

	COM_BufAddText(va("saveconfig \"%s\"\n", configfile));
	COM_BufAddText(va("loadconfig \"%s\"\n", COM_Argv(1)));
}

extern "C" struct CVarList* cvlist_execversion;

/** Loads the default config file.
  *
  * \sa Command_LoadConfig_f
  */
void M_FirstLoadConfig(void)
{
	UINT8 i;

	// configfile is initialised by d_main when searching for the wad?

	// check for a custom config file
	if (M_CheckParm("-config") && M_IsNextParm())
	{
		strcpy(configfile, M_GetNextParm());
		CONS_Printf(M_GetText("config file: %s\n"), configfile);
	}

	// load default control
	G_DefineDefaultControls();

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		G_ApplyControlScheme(i, gamecontroldefault);
	}

	// register execversion here before we load any configs
	{
		CV_RegisterList(cvlist_execversion);
	}

	// temporarily reset execversion to default
	// we shouldn't need to do this, but JUST in case...
	CV_ToggleExecVersion(true);
	COM_BufInsertText(va("%s \"%s\"\n", cv_execversion.name, cv_execversion.defaultvalue));
	CV_InitFilterVar();

	// load config, make sure those commands doesnt require the screen...
	COM_BufInsertText(va("exec \"%s\" -immediate\n", configfile));
	// no COM_BufExecute() needed; that does it right away

	// don't filter anymore vars and don't let this convsvar be changed
	COM_BufInsertText(va("%s \"%d\"\n", cv_execversion.name, EXECVERSION));
	CV_ToggleExecVersion(false);

	// make sure I_Quit() will write back the correct config
	// (do not write back the config if it crash before)
	gameconfig_loaded = true;

	// reset to default player stuff
#if 0
	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		COM_BufAddText (va("%s \"%s\"\n",cv_skin[i].name,cv_skin[i].defaultvalue));
		COM_BufAddText (va("%s \"%s\"\n",cv_playercolor[i].name,cv_playercolor[i].defaultvalue));
	}
#endif
}

/** Saves the game configuration.
  *
  * \sa Command_SaveConfig_f
  */
void M_SaveConfig(const char *filename)
{
	FILE *f;
	char tmppath[2048];

	// make sure not to write back the config until it's been correctly loaded
	if (!gameconfig_loaded)
		return;

	// can change the file name
	if (filename)
	{
		if (!strstr(filename, ".cfg"))
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Config filename must be .cfg\n"));
			return;
		}

		// append srb2home to beginning of filename
		// but check if srb2home isn't already there, first
		if (!strstr(filename, srb2home))
		{
			sprintf(tmppath, "%s" PATHSEP "%s.tmp", srb2home, filename);
		}
		else
		{
			sprintf(tmppath, "%s", filename);
		}

		f = fopen(tmppath, "w");
		// change it only if valid
		if (f)
		{
			strcpy(configfile, tmppath);
		}
		else
		{
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't save game config file %s\n"), tmppath);
			return;
		}
	}
	else
	{
		if (!strstr(configfile, ".cfg"))
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Config filename must be .cfg\n"));
			return;
		}

		sprintf(tmppath, "%s.tmp", configfile);

		f = fopen(tmppath, "w");
		if (!f)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't save game config file %s\n"), configfile);
			return;
		}
	}

	// header message
	fprintf(f, "// Dr. Robotnik's Ring Racers configuration file.\n");

	// print execversion FIRST, because subsequent consvars need to be filtered
	// always print current EXECVERSION
	fprintf(f, "%s \"%d\"\n", cv_execversion.name, EXECVERSION);

	// FIXME: save key aliases if ever implemented..

	CV_SaveVariables(f);

	if (!dedicated)
	{
		G_SaveKeySetting(f, gamecontrol[0], gamecontrol[1], gamecontrol[2], gamecontrol[3]);
	}

	fclose(f);

	{
		// Atomically replace the old config once the new one has been written.

		namespace fs = std::filesystem;

		fs::path tmp{tmppath};
		fs::path real{configfile};

		try
		{
			fs::rename(tmp, real);
		}
		catch (const fs::filesystem_error& ex)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Failed to move temp config file to real destination\n"));
		}
	}
}

// ==========================================================================
//                              SCREENSHOTS
// ==========================================================================
static UINT8 screenshot_palette[768];
static void M_CreateScreenShotPalette(void)
{
	size_t i, j;
	for (i = 0, j = 0; i < 768; i += 3, j++)
	{
		RGBA_t locpal = ((cv_screenshot_colorprofile.value)
		? pLocalPalette[(std::max<INT32>(st_palette,0)*256)+j]
		: pMasterPalette[(std::max<INT32>(st_palette,0)*256)+j]);
		screenshot_palette[i] = locpal.s.red;
		screenshot_palette[i+1] = locpal.s.green;
		screenshot_palette[i+2] = locpal.s.blue;
	}
}

#if NUMSCREENS > 2
static const char *Newsnapshotfile(const char *pathname, const char *ext)
{
	static char freename[20] = "ringracersXXXX.exte";
	int i = 5000; // start in the middle: num screenshots divided by 2
	int add = i; // how much to add or subtract if wrong; gets divided by 2 each time
	int result; // -1 = guess too high, 0 = correct, 1 = guess too low

	// find a file name to save it to
	I_Assert(strlen(ext) < (sizeof freename) - 15);
	strcpy(freename+15,ext);

	for (;;)
	{
		freename[10] = (char)('0' + (char)(i/1000));
		freename[11] = (char)('0' + (char)((i/100)%10));
		freename[12] = (char)('0' + (char)((i/10)%10));
		freename[13] = (char)('0' + (char)(i%10));

		if (FIL_WriteFileOK(va(pandf,pathname,freename))) // access succeeds
			result = 1; // too low
		else // access fails: equal or too high
		{
			if (!i)
				break; // not too high, so it must be equal! YAY!

			freename[10] = (char)('0' + (char)((i-1)/1000));
			freename[11] = (char)('0' + (char)(((i-1)/100)%10));
			freename[12] = (char)('0' + (char)(((i-1)/10)%10));
			freename[13] = (char)('0' + (char)((i-1)%10));
			if (!FIL_WriteFileOK(va(pandf,pathname,freename))) // access fails
				result = -1; // too high
			else
				break; // not too high, so equal, YAY!
		}

		add /= 2;

		if (!add) // don't get stuck at 5 due to truncation!
			add = 1;

		i += add * result;

		if (i < 0 || i > 9999)
			return NULL;
	}

	freename[10] = (char)('0' + (char)(i/1000));
	freename[11] = (char)('0' + (char)((i/100)%10));
	freename[12] = (char)('0' + (char)((i/10)%10));
	freename[13] = (char)('0' + (char)(i%10));

	return freename;
}
#endif

#ifdef HAVE_PNG
FUNCNORETURN static void PNG_error(png_structp PNG, png_const_charp pngtext)
{
	//CONS_Debug(DBG_RENDER, "libpng error at %p: %s", PNG, pngtext);
	I_Error("libpng error at %p: %s", (void*)PNG, pngtext);
}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng warning at %p: %s", (void*)PNG, pngtext);
}

static void M_PNGhdr(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_uint_32 width, PNG_CONST png_uint_32 height, PNG_CONST png_byte *palette)
{
	const png_byte png_interlace = PNG_INTERLACE_NONE; //PNG_INTERLACE_ADAM7
	if (palette)
	{
		png_colorp png_PLTE = static_cast<png_colorp>(png_malloc(png_ptr, sizeof(png_color)*256)); //palette
		png_uint_16 i;

		const png_byte *pal = palette;

		for (i = 0; i < 256; i++)
		{
			png_PLTE[i].red   = *pal; pal++;
			png_PLTE[i].green = *pal; pal++;
			png_PLTE[i].blue  = *pal; pal++;
		}

		png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_PALETTE,
		 png_interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info_before_PLTE(png_ptr, png_info_ptr);
		png_set_PLTE(png_ptr, png_info_ptr, png_PLTE, 256);
		png_free(png_ptr, (png_voidp)png_PLTE); // safe in libpng-1.2.1+
		png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);
		png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
	}
	else
	{
		png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
		 png_interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info_before_PLTE(png_ptr, png_info_ptr);
		png_set_compression_strategy(png_ptr, Z_FILTERED);
	}
}

static void M_PNGText(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_byte movie)
{
#ifdef PNG_TEXT_SUPPORTED
#define SRB2PNGTXT 11 //PNG_KEYWORD_MAX_LENGTH(79) is the max
	png_text png_infotext[SRB2PNGTXT];
	char keytxt[SRB2PNGTXT][12] = {
	"Title", "Description", "Playername", "Mapnum", "Mapname",
	"Location", "Interface", "Render Mode", "Revision", "Build Date", "Build Time"};
	char titletxt[] = "Dr. Robotnik's Ring Racers " VERSIONSTRING;
	png_charp playertxt =  cv_playername[0].zstring;
	char desctxt[] = "Ring Racers Screenshot";
	char Movietxt[] = "Ring Racers Movie";
	size_t i;
	char interfacetxt[] =
#ifdef HAVE_SDL
	 "SDL";
#else
	 "Unknown";
#endif
	char rendermodetxt[9];
	char maptext[MAXMAPLUMPNAME];
	char lvlttltext[48];
	char locationtxt[40];
	char ctrevision[40];
	char ctdate[40];
	char cttime[40];

	switch (rendermode)
	{
		case render_soft:
			strcpy(rendermodetxt, "Software");
			break;
		case render_opengl:
			strcpy(rendermodetxt, "OpenGL");
			break;
		default: // Just in case
			strcpy(rendermodetxt, "None");
			break;
	}

	if (gamestate == GS_LEVEL)
	{
		const char* mapname = G_BuildMapName(gamemap);
		if (mapname)
			snprintf(maptext, sizeof(maptext), "%s", mapname);
		else
			snprintf(maptext, sizeof(maptext), "Unknown");
	}
	else
		snprintf(maptext, sizeof(maptext), "Unknown");

	if (gamestate == GS_LEVEL && mapheaderinfo[gamemap-1]->lvlttl[0] != '\0')
		snprintf(lvlttltext, 48, "%s%s%s",
			mapheaderinfo[gamemap-1]->lvlttl,
			(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE) ? "" :
			(mapheaderinfo[gamemap-1]->zonttl[0] != '\0') ? va(" %s",mapheaderinfo[gamemap-1]->zonttl) : " Zone",
			(mapheaderinfo[gamemap-1]->actnum > 0) ? va(" %d",mapheaderinfo[gamemap-1]->actnum) : "");
	else
		snprintf(lvlttltext, 48, "Unknown");

	if (gamestate == GS_LEVEL && players[g_localplayers[0]].mo)
		snprintf(locationtxt, 40, "X:%d Y:%d Z:%d A:%d",
			players[g_localplayers[0]].mo->x>>FRACBITS,
			players[g_localplayers[0]].mo->y>>FRACBITS,
			players[g_localplayers[0]].mo->z>>FRACBITS,
			FixedInt(AngleFixed(players[g_localplayers[0]].mo->angle)));
	else
		snprintf(locationtxt, 40, "Unknown");

	memset(png_infotext,0x00,sizeof (png_infotext));

	for (i = 0; i < SRB2PNGTXT; i++)
		png_infotext[i].key  = keytxt[i];

	png_infotext[0].text = titletxt;
	if (movie)
		png_infotext[1].text = Movietxt;
	else
		png_infotext[1].text = desctxt;
	png_infotext[2].text = playertxt;
	png_infotext[3].text = maptext;
	png_infotext[4].text = lvlttltext;
	png_infotext[5].text = locationtxt;
	png_infotext[6].text = interfacetxt;
	png_infotext[7].text = rendermodetxt;
	png_infotext[8].text = strncpy(ctrevision, comprevision, sizeof(ctrevision)-1);
	png_infotext[9].text = strncpy(ctdate, compdate, sizeof(ctdate)-1);
	png_infotext[10].text = strncpy(cttime, comptime, sizeof(cttime)-1);

	png_set_text(png_ptr, png_info_ptr, png_infotext, SRB2PNGTXT);
#undef SRB2PNGTXT
#endif
}

static inline void M_PNGImage(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_uint_32 height, png_bytep png_buf)
{
	png_uint_32 pitch = png_get_rowbytes(png_ptr, static_cast<const png_info*>(png_info_ptr));
	png_bytepp row_pointers = static_cast<png_bytepp>(png_malloc(png_ptr, height* sizeof (png_bytep)));
	png_uint_32 y;
	for (y = 0; y < height; y++)
	{
		row_pointers[y] = png_buf;
		png_buf += pitch;
	}
	png_write_image(png_ptr, row_pointers);
	png_free(png_ptr, (png_voidp)row_pointers);
}

#ifdef USE_APNG
static png_structp apng_ptr = NULL;
static png_infop   apng_info_ptr = NULL;
static apng_infop  apng_ainfo_ptr = NULL;
static png_FILE_p  apng_FILE = NULL;
static png_uint_32 apng_frames = 0;
#ifdef PNG_STATIC // Win32 build have static libpng
#define aPNG_set_acTL png_set_acTL
#define aPNG_write_frame_head png_write_frame_head
#define aPNG_write_frame_tail png_write_frame_tail
#else // outside libpng may not have apng support

#ifndef PNG_WRITE_APNG_SUPPORTED // libpng header may not have apng patch

#ifndef PNG_INFO_acTL
#define PNG_INFO_acTL 0x10000L
#endif
#ifndef PNG_INFO_fcTL
#define PNG_INFO_fcTL 0x20000L
#endif
#ifndef PNG_FIRST_FRAME_HIDDEN
#define PNG_FIRST_FRAME_HIDDEN       0x0001
#endif
#ifndef PNG_DISPOSE_OP_NONE
#define PNG_DISPOSE_OP_NONE        0x00
#endif
#ifndef PNG_DISPOSE_OP_BACKGROUND
#define PNG_DISPOSE_OP_BACKGROUND  0x01
#endif
#ifndef PNG_DISPOSE_OP_PREVIOUS
#define PNG_DISPOSE_OP_PREVIOUS    0x02
#endif
#ifndef PNG_BLEND_OP_SOURCE
#define PNG_BLEND_OP_SOURCE        0x00
#endif
#ifndef PNG_BLEND_OP_OVER
#define PNG_BLEND_OP_OVER          0x01
#endif
#ifndef PNG_HAVE_acTL
#define PNG_HAVE_acTL             0x4000
#endif
#ifndef PNG_HAVE_fcTL
#define PNG_HAVE_fcTL             0x8000L
#endif

#endif
typedef png_uint_32 (*P_png_set_acTL) (png_structp png_ptr,
   png_infop info_ptr, png_uint_32 num_frames, png_uint_32 num_plays);
typedef void (*P_png_write_frame_head) (png_structp png_ptr,
   png_infop info_ptr, png_bytepp row_pointers,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den, png_byte dispose_op,
   png_byte blend_op);

typedef void (*P_png_write_frame_tail) (png_structp png_ptr,
   png_infop info_ptr);
static P_png_set_acTL aPNG_set_acTL = NULL;
static P_png_write_frame_head aPNG_write_frame_head = NULL;
static P_png_write_frame_tail aPNG_write_frame_tail = NULL;
#endif

static inline boolean M_PNGLib(void)
{
#ifdef PNG_STATIC // Win32 build have static libpng
	return true;
#else
	static void *pnglib = NULL;
	if (aPNG_set_acTL && aPNG_write_frame_head && aPNG_write_frame_tail)
		return true;
	if (pnglib)
		return false;
#ifdef _WIN32
	pnglib = GetModuleHandleA("libpng.dll");
	if (!pnglib)
		pnglib = GetModuleHandleA("libpng12.dll");
	if (!pnglib)
		pnglib = GetModuleHandleA("libpng13.dll");
#elif defined (HAVE_SDL)
#ifdef __APPLE__
	pnglib = hwOpen("libpng.dylib");
#else
	pnglib = hwOpen("libpng.so");
#endif
#endif
	if (!pnglib)
		return false;
#ifdef HAVE_SDL
	aPNG_set_acTL = (P_png_set_acTL) hwSym("png_set_acTL", pnglib);
	aPNG_write_frame_head = (P_png_write_frame_head) hwSym("png_write_frame_head", pnglib);
	aPNG_write_frame_tail = (P_png_write_frame_tail) hwSym("png_write_frame_tail", pnglib);
#endif
#ifdef _WIN32
	aPNG_set_acTL = (P_png_set_acTL) GetProcAddress("png_set_acTL", pnglib);
	aPNG_write_frame_head = (P_png_write_frame_head) GetProcAddress("png_write_frame_head", pnglib);
	aPNG_write_frame_tail = (P_png_write_frame_tail) GetProcAddress("png_write_frame_tail", pnglib);
#endif
	return (aPNG_set_acTL && aPNG_write_frame_head && aPNG_write_frame_tail);
#endif
}

static void M_PNGFrame(png_structp png_ptr, png_infop png_info_ptr, png_bytep png_buf)
{
	png_uint_16 downscale = apng_downscale ? vid.dupx : 1;

	png_uint_32 pitch = png_get_rowbytes(png_ptr, png_info_ptr);
	PNG_CONST png_uint_32 width = vid.width / downscale;
	PNG_CONST png_uint_32 height = vid.height / downscale;
	png_bytepp row_pointers = (png_bytepp) png_malloc(png_ptr, height * sizeof (png_bytep));
	png_uint_32 x, y;
	png_uint_16 framedelay = (png_uint_16)cv_apng_delay.value;

	apng_frames++;

	for (y = 0; y < height; y++)
	{
		row_pointers[y] = (png_bytep) malloc(pitch * sizeof(png_byte));
		for (x = 0; x < width; x++)
			row_pointers[y][x] = png_buf[x * downscale];
		png_buf += pitch * (downscale * downscale);
	}
		//for (x = 0; x < width; x++)
		//{
		//	printf("%d", x);
		//	row_pointers[y][x] = 0;
		//}
	/*	row_pointers[y] = calloc(1, sizeof(png_bytep));
		png_buf += pitch * 2;
	}*/

#ifndef PNG_STATIC
	if (aPNG_write_frame_head)
#endif
		aPNG_write_frame_head(apng_ptr, apng_info_ptr, row_pointers,
			width,     /* width */
			height,    /* height */
			0,         /* x offset */
			0,         /* y offset */
			framedelay, TICRATE,/* delay numerator and denominator */
			PNG_DISPOSE_OP_BACKGROUND, /* dispose */
			PNG_BLEND_OP_SOURCE        /* blend */
		                     );

	png_write_image(png_ptr, row_pointers);

#ifndef PNG_STATIC
	if (aPNG_write_frame_tail)
#endif
		aPNG_write_frame_tail(apng_ptr, apng_info_ptr);

	png_free(png_ptr, (png_voidp)row_pointers);
}

static void M_PNGfix_acTL(png_structp png_ptr, png_infop png_info_ptr,
		apng_infop png_ainfo_ptr)
{
	apng_set_acTL(png_ptr, png_info_ptr, png_ainfo_ptr, apng_frames, 0);

#ifndef NO_PNG_DEBUG
	png_debug(1, "in png_write_acTL\n");
#endif
}

static boolean M_SetupaPNG(png_const_charp filename, png_bytep pal)
{
	png_uint_16 downscale;

	apng_downscale = (!!cv_apng_downscale.value);

	downscale = apng_downscale ? vid.dupx : 1;

	apng_FILE = fopen(filename,"wb+"); // + mode for reading
	if (!apng_FILE)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on opening %s for write\n", filename);
		return false;
	}

	apng_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	 PNG_error, PNG_warn);
	if (!apng_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on initialize libpng\n");
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	apng_info_ptr = png_create_info_struct(apng_ptr);
	if (!apng_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on allocate for libpng\n");
		png_destroy_write_struct(&apng_ptr,  NULL);
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	apng_ainfo_ptr = apng_create_info_struct(apng_ptr);
	if (!apng_ainfo_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_StartMovie: Error on allocate for apng\n");
		png_destroy_write_struct(&apng_ptr, &apng_info_ptr);
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	png_init_io(apng_ptr, apng_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(apng_ptr, MAXVIDWIDTH, MAXVIDHEIGHT);
#endif

	//png_set_filter(apng_ptr, 0, PNG_ALL_FILTERS);

	png_set_compression_level(apng_ptr, cv_zlib_levela.value);
	png_set_compression_mem_level(apng_ptr, cv_zlib_memorya.value);
	png_set_compression_strategy(apng_ptr, cv_zlib_strategya.value);
	png_set_compression_window_bits(apng_ptr, cv_zlib_window_bitsa.value);

	M_PNGhdr(apng_ptr, apng_info_ptr, vid.width / downscale, vid.height / downscale, pal);

	M_PNGText(apng_ptr, apng_info_ptr, true);

	apng_set_set_acTL_fn(apng_ptr, apng_ainfo_ptr, aPNG_set_acTL);

	apng_set_acTL(apng_ptr, apng_info_ptr, apng_ainfo_ptr, PNG_UINT_31_MAX, 0);

	apng_write_info(apng_ptr, apng_info_ptr, apng_ainfo_ptr);

	apng_frames = 0;

	return true;
}
#endif
#endif

// ==========================================================================
//                             MOVIE MODE
// ==========================================================================
#if NUMSCREENS > 2
static inline moviemode_t M_StartMovieAPNG(const char *pathname)
{
#ifdef USE_APNG
	UINT8 *palette = NULL;
	const char *freename = NULL;
	boolean ret = false;

	if (!M_PNGLib())
	{
		CONS_Alert(CONS_ERROR, "Couldn't create aPNG: libpng not found\n");
		return MM_OFF;
	}

	if (!(freename = Newsnapshotfile(pathname,"png")))
	{
		CONS_Alert(CONS_ERROR, "Couldn't create aPNG: no slots open in %s\n", pathname);
		return MM_OFF;
	}

	if (rendermode == render_soft)
	{
		M_CreateScreenShotPalette();
		palette = screenshot_palette;
	}

	ret = M_SetupaPNG(va(pandf,pathname,freename), palette);

	if (!ret)
	{
		CONS_Alert(CONS_ERROR, "Couldn't create aPNG: error creating %s in %s\n", freename, pathname);
		return MM_OFF;
	}
	return MM_APNG;
#else
	// no APNG support exists
	(void)pathname;
	CONS_Alert(CONS_ERROR, "Couldn't create aPNG: this build lacks aPNG support\n");
	return MM_OFF;
#endif
}

static inline moviemode_t M_StartMovieGIF(const char *pathname)
{
#ifdef HAVE_ANIGIF
	const char *freename;

	if (!(freename = Newsnapshotfile(pathname,"gif")))
	{
		CONS_Alert(CONS_ERROR, "Couldn't create GIF: no slots open in %s\n", pathname);
		return MM_OFF;
	}

	if (!GIF_open(va(pandf,pathname,freename)))
	{
		CONS_Alert(CONS_ERROR, "Couldn't create GIF: error creating %s in %s\n", freename, pathname);
		return MM_OFF;
	}
	return MM_GIF;
#else
	// no GIF support exists
	(void)pathname;
	CONS_Alert(CONS_ERROR, "Couldn't create GIF: this build lacks GIF support\n");
	return MM_OFF;
#endif
}
#endif

static inline moviemode_t M_StartMovieAVRecorder(const char *pathname)
{
#ifndef SRB2_CONFIG_ENABLE_WEBM_MOVIES
	return MM_OFF;
#else
	const char *ext = M_AVRecorder_GetFileExtension();
	const char *freename;

	if (!(freename = Newsnapshotfile(pathname, ext)))
	{
		CONS_Alert(CONS_ERROR, "Couldn't create %s file: no slots open in %s\n", ext, pathname);
		return MM_OFF;
	}

	if (!M_AVRecorder_Open(va(pandf,pathname,freename)))
	{
		return MM_OFF;
	}

	return MM_AVRECORDER;
#endif
}

void M_StartMovie(moviemode_t mode)
{
#if NUMSCREENS > 2
	const char *folder;
	char pathname[MAX_WADPATH];

	if (moviemode)
		return;

	switch (mode)
	{
	case MM_GIF:
		folder = "gifs";
		break;

	case MM_AVRECORDER:
		folder = "movies";
		break;

	default:
		folder = "slideshows";
	}

	sprintf(pathname, "%s" PATHSEP "media" PATHSEP "%s" PATHSEP, srb2home, folder);
	M_MkdirEach(pathname, M_PathParts(pathname) - 2, 0755);

	if (rendermode == render_none)
		I_Error("Can't make a movie without a render system\n");

	switch (mode)
	{
		case MM_GIF:
			moviemode = M_StartMovieGIF(pathname);
			break;
		case MM_APNG:
			moviemode = M_StartMovieAPNG(pathname);
			break;
		case MM_SCREENSHOT:
			moviemode = MM_SCREENSHOT;
			break;
		case MM_AVRECORDER:
			moviemode = M_StartMovieAVRecorder(pathname);
			break;
		default: //???
			return;
	}

	if (moviemode == MM_APNG)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "aPNG");
	else if (moviemode == MM_GIF)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "GIF");
	else if (moviemode == MM_SCREENSHOT)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "screenshots");
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
	else if (moviemode == MM_AVRECORDER)
	{
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), M_AVRecorder_GetCurrentFormat());
		M_AVRecorder_PrintCurrentConfiguration();
	}
#endif

	//g_singletics = (moviemode != MM_OFF);
#endif
}

static void M_SaveFrame_AVRecorder(uint32_t width, uint32_t height, tcb::span<const std::byte> data);

void M_LegacySaveFrame(void)
{
#if NUMSCREENS > 2
	// TODO: until HWR2 replaces legacy OpenGL renderer, this
	//       function still needs to called for OpenGL.
#ifdef HWRENDER
	if (rendermode != render_opengl)
#endif
	{
		return;
	}

	// paranoia: should be unnecessary without singletics
	static tic_t oldtic = 0;

#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
	if (moviemode == MM_AVRECORDER)
	{
		if (M_AVRecorder_IsExpired())
		{
			M_StopMovie();
			return;
		}
	}
#endif

	// skip interpolated frames for other modes
	if (oldtic == I_GetTime())
		return;
	else
		oldtic = I_GetTime();

	switch (moviemode)
	{
		case MM_SCREENSHOT:
			takescreenshot = true;
			return;
		case MM_GIF:
			GIF_frame();
			return;
		case MM_APNG:
#ifdef USE_APNG
			{
				UINT8 *linear = NULL;
				if (!apng_FILE) // should not happen!!
				{
					moviemode = MM_OFF;
					return;
				}

				if (rendermode == render_soft)
				{
					// munge planar buffer to linear
					linear = screens[2];
					I_ReadScreen(linear);
				}
#ifdef HWRENDER
				else
					linear = HWR_GetScreenshot();
#endif
				M_PNGFrame(apng_ptr, apng_info_ptr, (png_bytep)linear);
#ifdef HWRENDER
				if (rendermode == render_opengl && linear)
					free(linear);
#endif

				if (apng_frames == PNG_UINT_31_MAX)
				{
					CONS_Alert(CONS_NOTICE, M_GetText("Max movie size reached\n"));
					M_StopMovie();
				}
			}
#else
			moviemode = MM_OFF;
#endif
			return;
		case MM_AVRECORDER:
#if defined(SRB2_CONFIG_ENABLE_WEBM_MOVIES) && defined(HWRENDER)
			{
				UINT8 *linear = HWR_GetScreenshot();
				M_SaveFrame_AVRecorder(vid.width, vid.height, tcb::as_bytes(tcb::span(linear, 3 * vid.width * vid.height)));
				free(linear);
			}
#endif
			return;
		default:
			return;
	}
#endif
}

static void M_SaveFrame_GIF(uint32_t width, uint32_t height, tcb::span<const std::byte> data)
{
	if (moviemode != MM_GIF)
	{
		return;
	}

	static tic_t oldtic = 0;

	// limit the recording to TICRATE
	if (oldtic == I_GetTime())
	{
		return;
	}

	oldtic = I_GetTime();

	GIF_frame_rgb24(width, height, reinterpret_cast<const uint8_t*>(data.data()));
}

static void M_SaveFrame_AVRecorder(uint32_t width, uint32_t height, tcb::span<const std::byte> data)
{
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
	if (M_AVRecorder_IsExpired())
	{
		M_StopMovie();
		return;
	}

	auto frame = g_av_recorder->new_staging_video_frame(width, height);
	if (!frame)
	{
		// Not time to submit a frame!
		return;
	}

	auto data_begin = reinterpret_cast<const uint8_t*>(data.data());
	auto data_end = reinterpret_cast<const uint8_t*>(data.data() + data.size_bytes());
	std::copy(data_begin, data_end, frame->screen.begin());
	g_av_recorder->push_staging_video_frame(std::move(frame));
#endif
}

void M_SaveFrame(uint32_t width, uint32_t height, tcb::span<const std::byte> data)
{
	switch (moviemode)
	{
	case MM_GIF:
		M_SaveFrame_GIF(width, height, data);
		break;
	case MM_AVRECORDER:
		M_SaveFrame_AVRecorder(width, height, data);
		break;
	default:
		break;
	}
}

void M_StopMovie(void)
{
#if NUMSCREENS > 2
	switch (moviemode)
	{
		case MM_GIF:
			if (!GIF_close())
				return;
			break;
		case MM_APNG:
#ifdef USE_APNG
			if (!apng_FILE)
				return;

			if (apng_frames)
			{
				M_PNGfix_acTL(apng_ptr, apng_info_ptr, apng_ainfo_ptr);
				apng_write_end(apng_ptr, apng_info_ptr, apng_ainfo_ptr);
			}

			png_destroy_write_struct(&apng_ptr, &apng_info_ptr);

			fclose(apng_FILE);
			apng_FILE = NULL;
			CONS_Printf("aPNG closed; wrote %u frames\n", (UINT32)apng_frames);
			apng_frames = 0;
			break;
#else
			return;
#endif
		case MM_SCREENSHOT:
			break;
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
		case MM_AVRECORDER:
			M_AVRecorder_Close();
			break;
#endif
		default:
			return;
	}
	moviemode = MM_OFF;
	CONS_Printf(M_GetText("Movie mode disabled.\n"));
#endif
}

// ==========================================================================
//                            SCREEN SHOTS
// ==========================================================================
#ifdef USE_PNG
/** Writes a PNG file to disk.
  *
  * \param filename Filename to write to.
  * \param data     The image data.
  * \param width    Width of the picture.
  * \param height   Height of the picture.
  * \param palette  Palette of image data.
  *  \note if palette is NULL, BGR888 format
  */
boolean M_SavePNG(const char *filename, const void *data, int width, int height, const UINT8 *palette)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif
	png_FILE_p png_FILE;

	png_FILE = fopen(filename,"wb");
	if (!png_FILE)
	{
		CONS_Debug(DBG_RENDER, "M_SavePNG: Error on opening %s for write\n", filename);
		return false;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_SavePNG: Error on initialize libpng\n");
		fclose(png_FILE);
		remove(filename);
		return false;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "M_SavePNG: Error on allocate for libpng\n");
		png_destroy_write_struct(&png_ptr,  NULL);
		fclose(png_FILE);
		remove(filename);
		return false;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		//CONS_Debug(DBG_RENDER, "libpng write error on %s\n", filename);
		png_destroy_write_struct(&png_ptr, &png_info_ptr);
		fclose(png_FILE);
		remove(filename);
		return false;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr),jmpbuf, sizeof (jmp_buf));
#endif
	png_init_io(png_ptr, png_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, MAXVIDWIDTH, MAXVIDHEIGHT);
#endif

	//png_set_filter(png_ptr, 0, PNG_ALL_FILTERS);

	png_set_compression_level(png_ptr, cv_zlib_level.value);
	png_set_compression_mem_level(png_ptr, cv_zlib_memory.value);
	png_set_compression_strategy(png_ptr, cv_zlib_strategy.value);
	png_set_compression_window_bits(png_ptr, cv_zlib_window_bits.value);

	M_PNGhdr(png_ptr, png_info_ptr, width, height, palette);

	M_PNGText(png_ptr, png_info_ptr, false);

	png_write_info(png_ptr, png_info_ptr);

	M_PNGImage(png_ptr, png_info_ptr, height, (png_bytep)data);

	png_write_end(png_ptr, png_info_ptr);
	png_destroy_write_struct(&png_ptr, &png_info_ptr);

	fclose(png_FILE);
	return true;
}
#else
/** PCX file structure.
  */
typedef struct
{
	UINT8 manufacturer;
	UINT8 version;
	UINT8 encoding;
	UINT8 bits_per_pixel;

	UINT16 xmin, ymin;
	UINT16 xmax, ymax;
	UINT16 hres, vres;
	UINT8  palette[48];

	UINT8 reserved;
	UINT8 color_planes;
	UINT16 bytes_per_line;
	UINT16 palette_type;

	char filler[58];
	UINT8 data; ///< Unbounded; used for all picture data.
} pcx_t;

/** Writes a PCX file to disk.
  *
  * \param filename Filename to write to.
  * \param data     The image data.
  * \param width    Width of the picture.
  * \param height   Height of the picture.
  * \param palette  Palette of image data
  */
#if NUMSCREENS > 2
static boolean WritePCXfile(const char *filename, const UINT8 *data, int width, int height, const UINT8 *pal)
{
	int i;
	size_t length;
	pcx_t *pcx;
	UINT8 *pack;

	pcx = Z_Malloc(width*height*2 + 1000, PU_STATIC, NULL);

	pcx->manufacturer = 0x0a; // PCX id
	pcx->version = 5; // 256 color
	pcx->encoding = 1; // uncompressed
	pcx->bits_per_pixel = 8; // 256 color
	pcx->xmin = pcx->ymin = 0;
	pcx->xmax = SHORT(width - 1);
	pcx->ymax = SHORT(height - 1);
	pcx->hres = SHORT(width);
	pcx->vres = SHORT(height);
	memset(pcx->palette, 0, sizeof (pcx->palette));
	pcx->reserved = 0;
	pcx->color_planes = 1; // chunky image
	pcx->bytes_per_line = SHORT(width);
	pcx->palette_type = SHORT(1); // not a grey scale
	memset(pcx->filler, 0, sizeof (pcx->filler));

	// pack the image
	pack = &pcx->data;

	for (i = 0; i < width*height; i++)
	{
		if ((*data & 0xc0) != 0xc0)
			*pack++ = *data++;
		else
		{
			*pack++ = 0xc1;
			*pack++ = *data++;
		}
	}

	// write the palette
	*pack++ = 0x0c; // palette ID byte

	// write color table
	{
		for (i = 0; i < 256; i++)
		{
			*pack++ = *pal; pal++;
			*pack++ = *pal; pal++;
			*pack++ = *pal; pal++;
		}
	}

	// write output file
	length = pack - (UINT8 *)pcx;
	i = FIL_WriteFile(filename, pcx, length);

	Z_Free(pcx);
	return i;
}
#endif
#endif

void M_ScreenShot(void)
{
	takescreenshot = true;
}

void M_DoLegacyGLScreenShot(void)
{
	const std::byte* fake_data = nullptr;
	M_DoScreenShot(vid.width, vid.height, tcb::span(fake_data, vid.width * vid.height));
}

/** Takes a screenshot.
  * The screenshot is saved as "srb2xxxx.png" where xxxx is the lowest
  * four-digit number for which a file does not already exist.
  *
  * \sa HWR_ScreenShot
  */
void M_DoScreenShot(UINT32 width, UINT32 height, tcb::span<const std::byte> data)
{
#if NUMSCREENS > 2
	const char *freename = NULL;
	char pathname[MAX_WADPATH];
	boolean ret = false;

	// Don't take multiple screenshots, obviously
	takescreenshot = false;

	// how does one take a screenshot without a render system?
	if (rendermode == render_none)
		return;

	strcpy(pathname, srb2home);
	strcat(pathname, PATHSEP "media" PATHSEP "screenshots" PATHSEP);
	M_MkdirEach(pathname, M_PathParts(pathname) - 2, 0755);

#ifdef USE_PNG
	freename = Newsnapshotfile(pathname,"png");
#else
	if (rendermode == render_soft)
		freename = Newsnapshotfile(pathname,"pcx");
	else if (rendermode == render_opengl)
		freename = Newsnapshotfile(pathname,"tga");
#endif

	if (!freename)
		goto failure;

	// save the pcx file
#ifdef HWRENDER
	if (rendermode == render_opengl)
		ret = HWR_Screenshot(va(pandf,pathname,freename));
	else
#endif
	{
		const void* pixel_data = static_cast<const void*>(data.data());
#ifdef USE_PNG
		ret = M_SavePNG(va(pandf,pathname,freename), pixel_data, width, height, NULL);
#else
		ret = WritePCXfile(va(pandf,pathname,freename), linear, vid.width, vid.height, screenshot_palette);
#endif
	}

failure:
	if (ret)
	{
		if (moviemode != MM_SCREENSHOT)
			CONS_Printf(M_GetText("Screen shot %s saved in %s\n"), freename, pathname);
	}
	else
	{
		if (freename)
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't create screen shot %s in %s\n"), freename, pathname);
		else
			CONS_Alert(CONS_ERROR, M_GetText("Couldn't create screen shot in %s (all 10000 slots used!)\n"), pathname);

		if (moviemode == MM_SCREENSHOT)
			M_StopMovie();
	}
#endif
}

void M_SaveMapThumbnail(UINT32 width, UINT32 height, tcb::span<const std::byte> data)
{
#ifdef USE_PNG
#if NUMSCREENS > 2

	char *filepath;
	switch (g_takemapthumbnail)
	{
		case TMT_PICTURE:
		default:
		{
			filepath = va("%s" PATHSEP "PICTURE_%s.png", srb2home, G_BuildMapName(gamemap));
			break;
		}
		case TMT_RICHPRES:
		{
			filepath = va("%s" PATHSEP "map_%s.png", srb2home, G_BuildMapName(gamemap));
			break;
		}
	}

	// save the file
	const void* pixel_data = static_cast<const void*>(data.data());
	boolean ret = M_SavePNG(filepath, pixel_data, width, height, NULL);

	if (ret)
	{
		CONS_Printf(M_GetText("Created thumbnail at \"%s\"\n"), filepath);
	}
	else
	{
		CONS_Alert(CONS_ERROR, M_GetText("Couldn't create %s\n"), filepath);
	}

	g_takemapthumbnail = TMT_NO;

#endif // #ifdef USE_PNG
#endif // #if NUMSCREENS > 2
}

void M_ScreenshotTicker(void)
{
	const UINT8 pid = 0; // TODO: should splitscreen players be allowed to use this too?

	if (M_MenuButtonPressed(pid, MBT_SCREENSHOT))
	{
		M_ScreenShot();
	}
	else if (M_MenuButtonPressed(pid, MBT_STARTMOVIE))
	{
		if (moviemode)
		{
			M_StopMovie();
		}
		else
		{
			M_StartMovie(MM_AVRECORDER);
		}
	}
	else if (M_MenuButtonPressed(pid, MBT_STARTLOSSLESS))
	{
		if (moviemode)
		{
			M_StopMovie();
		}
		else
		{
			M_StartMovie(static_cast<moviemode_t>(cv_lossless_recorder.value));
		}
	}
}

void M_MinimapGenerate(void)
{
#ifdef USE_PNG
	char *filepath;
	boolean ret = false;
	minigen_t *minigen = NULL;
	size_t option_scale;
	INT32 mul = 1;

	if (gamestate != GS_LEVEL)
	{
		CONS_Alert(CONS_ERROR, "You must be in a level to generate a preliminary minimap!\n");
		return;
	}

	if (automapactive)
	{
		CONS_Alert(CONS_ERROR, "The automap is active! Please deactivate it and try again.\n");
		return;
	}

	option_scale = COM_CheckPartialParm("-m");

	if (option_scale)
	{
		if (COM_Argc() < option_scale + 2)/* no argument after? */
		{
			CONS_Alert(CONS_ERROR,
					"No multiplier follows parameter '%s'.\n",
					COM_Argv(option_scale));
			return;
		}

		mul = atoi(COM_Argv(option_scale + 1));

		if (mul < 1 || mul > 10)
		{
			CONS_Alert(CONS_ERROR,
					"Multiplier %d must be within range 1-10.\n",
					mul);
			return;
		}

		filepath = va("%s" PATHSEP "%s-MINIMAP-%d.png", srb2home, G_BuildMapName(gamemap), mul);
	}
	else
	{
		filepath = va("%s" PATHSEP "%s-MINIMAP.png", srb2home, G_BuildMapName(gamemap));
	}

	minigen = AM_MinimapGenerate(mul);

	if (minigen == NULL || minigen->buf == NULL)
		goto failure;

	M_CreateScreenShotPalette();
	ret = M_SavePNG(filepath, minigen->buf, minigen->w, minigen->h, screenshot_palette);

failure:
	if (minigen->buf != NULL)
		free(minigen->buf);

	if (ret)
	{
		CONS_Printf(M_GetText("%s saved.\nRemember that this is not a complete minimap,\nand must be edited before putting in-game.\n"), filepath);
		if (mul != 1)
		{
			CONS_Printf("You should divide its size by %d!\n", mul);
		}
	}
	else
	{
		CONS_Alert(CONS_ERROR, M_GetText("Couldn't create %s\n"), filepath);
	}
#endif //#ifdef USE_PNG
}

// ==========================================================================
//                       TRANSLATION FUNCTIONS
// ==========================================================================

// M_StartupLocale.
// Sets up gettext to translate SRB2's strings.
#ifdef GETTEXT
	#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
		#define GETTEXTDOMAIN1 "/usr/share/locale"
		#define GETTEXTDOMAIN2 "/usr/local/share/locale"
	#elif defined (_WIN32)
		#define GETTEXTDOMAIN1 "."
	#endif
#endif // GETTEXT

void M_StartupLocale(void)
{
#ifdef GETTEXT
	char *textdomhandle = NULL;
#endif //GETTEXT

	CONS_Printf("M_StartupLocale...\n");

	setlocale(LC_ALL, "");

	// Do not set numeric locale as that affects atof
	setlocale(LC_NUMERIC, "C");

#ifdef GETTEXT
	// FIXME: global name define anywhere?
#ifdef GETTEXTDOMAIN1
	textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN1);
#endif
#ifdef GETTEXTDOMAIN2
	if (!textdomhandle)
		textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN2);
#endif
#ifdef GETTEXTDOMAIN3
	if (!textdomhandle)
		textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN3);
#endif
#ifdef GETTEXTDOMAIN4
	if (!textdomhandle)
		textdomhandle = bindtextdomain("srb2", GETTEXTDOMAIN4);
#endif
	if (textdomhandle)
		textdomain("srb2");
	else
		CONS_Printf("Could not find locale text domain!\n");
#endif //GETTEXT
}

// ==========================================================================
//                        MISC STRING FUNCTIONS
// ==========================================================================

/** Returns a temporary string made out of varargs.
  * For use with CONS_Printf().
  *
  * \param format Format string.
  * \return Pointer to a static buffer of 1024 characters, containing the
  *         resulting string.
  */
char *va(const char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

/** Creates a string in the first argument that is the second argument followed
  * by the third argument followed by the first argument.
  * Useful for making filenames with full path. s1 = s2+s3+s1
  *
  * \param s1 First string, suffix, and destination.
  * \param s2 Second string. Ends up first in the result.
  * \param s3 Third string. Ends up second in the result.
  */
void strcatbf(char *s1, const char *s2, const char *s3)
{
	char tmp[1024];

	strcpy(tmp, s1);
	strcpy(s1, s2);
	strcat(s1, s3);
	strcat(s1, tmp);
}

/** Converts an ASCII Hex string into an integer. Thanks, Borland!
  * <Inuyasha> I don't know if this belongs here specifically, but it sure
  *            doesn't belong in p_spec.c, that's for sure
  *
  * \param hexStg Hexadecimal string.
  * \return an Integer based off the contents of the string.
  */
INT32 axtoi(const char *hexStg)
{
	INT32 n = 0;
	INT32 m = 0;
	INT32 count;
	INT32 intValue = 0;
	INT32 digit[8];
	while (n < 8)
	{
		if (hexStg[n] == '\0')
			break;
		if (hexStg[n] >= '0' && hexStg[n] <= '9') // 0-9
			digit[n] = (hexStg[n] & 0x0f);
		else if (hexStg[n] >= 'a' && hexStg[n] <= 'f') // a-f
			digit[n] = (hexStg[n] & 0x0f) + 9;
		else if (hexStg[n] >= 'A' && hexStg[n] <= 'F') // A-F
			digit[n] = (hexStg[n] & 0x0f) + 9;
		else
			break;
		n++;
	}
	count = n;
	m = n - 1;
	n = 0;
	while (n < count)
	{
		intValue = intValue | (digit[n] << (m << 2));
		m--;
		n++;
	}
	return intValue;
}

// Token parser variables

static UINT32 oldendPos = 0; // old value of endPos, used by M_UnGetToken
static UINT32 endPos = 0; // now external to M_GetToken, but still static

/** Token parser for TEXTURES, ANIMDEFS, and potentially other lumps later down the line.
  * Was originally R_GetTexturesToken when I was coding up the TEXTURES parser, until I realized I needed it for ANIMDEFS too.
  * Parses up to the next whitespace character or comma. When finding the start of the next token, whitespace is skipped.
  * Commas are not; if a comma is encountered, then THAT'S returned as the token.
  * -Shadow Hog
  *
  * \param inputString The string to be parsed. If NULL is supplied instead of a string, it will continue parsing the last supplied one.
  * The pointer to the last string supplied is stored as a static variable, so be careful not to free it while this function is still using it!
  * \return A pointer to a string, containing the fetched token. This is in freshly allocated memory, so be sure to Z_Free() it as appropriate.
*/
char *M_GetToken(const char *inputString)
{
	static const char *stringToUse = NULL; // Populated if inputString != NULL; used otherwise
	static UINT32 startPos = 0;
//	static UINT32 endPos = 0;
	static UINT32 stringLength = 0;
	static UINT8 inComment = 0; // 0 = not in comment, 1 = // Single-line, 2 = /* Multi-line */
	char *texturesToken = NULL;
	UINT32 texturesTokenLength = 0;

	if (inputString != NULL)
	{
		stringToUse = inputString;
		startPos = 0;
		oldendPos = endPos = 0;
		stringLength = strlen(inputString);
	}
	else
	{
		startPos = oldendPos = endPos;
	}
	if (stringToUse == NULL)
		return NULL;

	// Try to detect comments now, in case we're pointing right at one
	if (startPos < stringLength - 1
		&& inComment == 0)
	{
		if (stringToUse[startPos] == '/'
			&& stringToUse[startPos+1] == '/')
		{
			//Single-line comment start
			inComment = 1;
		}
		else if (stringToUse[startPos] == '/'
			&& stringToUse[startPos+1] == '*')
		{
			//Multi-line comment start
			inComment = 2;
		}
	}

	// Find the first non-whitespace char, or else the end of the string trying
	while (startPos < stringLength
			&& (stringToUse[startPos] == ' '
			|| stringToUse[startPos] == '\t'
			|| stringToUse[startPos] == '\r'
			|| stringToUse[startPos] == '\n'
			|| stringToUse[startPos] == '\0'
			|| stringToUse[startPos] == '=' || stringToUse[startPos] == ';' // UDMF TEXTMAP.
			|| inComment != 0))
	{
		// Try to detect comment endings now
		if (inComment == 1
			&& stringToUse[startPos] == '\n')
		{
			// End of line for a single-line comment
			inComment = 0;
		}
		else if (inComment == 2
			&& startPos < stringLength - 1
			&& stringToUse[startPos] == '*'
			&& stringToUse[startPos+1] == '/')
		{
			// End of multi-line comment
			inComment = 0;
			startPos++; // Make damn well sure we're out of the comment ending at the end of it all
		}

		startPos++;

		// Try to detect comment starts now
		if (startPos < stringLength - 1
			&& inComment == 0)
		{
			if (stringToUse[startPos] == '/'
				&& stringToUse[startPos+1] == '/')
			{
				//Single-line comment start
				inComment = 1;
			}
			else if (stringToUse[startPos] == '/'
				&& stringToUse[startPos+1] == '*')
			{
				//Multi-line comment start
				inComment = 2;
			}
		}
	}

	// If the end of the string is reached, no token is to be read
	if (startPos >= stringLength) {
		endPos = stringLength;
		return NULL;
	}
	// Else, if it's one of these three symbols, capture only this one character
	else if (stringToUse[startPos] == ','
			|| stringToUse[startPos] == '{'
			|| stringToUse[startPos] == '}')
	{
		endPos = startPos + 1;
		texturesToken = (char *)Z_Malloc(2*sizeof(char),PU_STATIC,NULL);
		texturesToken[0] = stringToUse[startPos];
		texturesToken[1] = '\0';
		return texturesToken;
	}
	// Return entire string within quotes, except without the quotes.
	else if (stringToUse[startPos] == '"')
	{
		endPos = ++startPos;
		while (endPos < stringLength && stringToUse[endPos] != '"')
			endPos++;

		texturesTokenLength = endPos++ - startPos;
		// Assign the memory. Don't forget an extra byte for the end of the string!
		texturesToken = (char *)Z_Malloc((texturesTokenLength+1)*sizeof(char),PU_STATIC,NULL);
		// Copy the string.
		M_Memcpy(texturesToken, stringToUse+startPos, (size_t)texturesTokenLength);
		// Make the final character NUL.
		texturesToken[texturesTokenLength] = '\0';

		return texturesToken;
	}

	// Now find the end of the token. This includes several additional characters that are okay to capture as one character, but not trailing at the end of another token.
	endPos = startPos + 1;
	while (endPos < stringLength
			&& (stringToUse[endPos] != ' '
			&& stringToUse[endPos] != '\t'
			&& stringToUse[endPos] != '\r'
			&& stringToUse[endPos] != '\n'
			&& stringToUse[endPos] != ','
			&& stringToUse[endPos] != '{'
			&& stringToUse[endPos] != '}'
			&& stringToUse[endPos] != '=' && stringToUse[endPos] != ';' // UDMF TEXTMAP.
			&& inComment == 0))
	{
		endPos++;
		// Try to detect comment starts now; if it's in a comment, we don't want it in this token
		if (endPos < stringLength - 1
			&& inComment == 0)
		{
			if (stringToUse[endPos] == '/'
				&& stringToUse[endPos+1] == '/')
			{
				//Single-line comment start
				inComment = 1;
			}
			else if (stringToUse[endPos] == '/'
				&& stringToUse[endPos+1] == '*')
			{
				//Multi-line comment start
				inComment = 2;
			}
		}
	}
	texturesTokenLength = endPos - startPos;

	// Assign the memory. Don't forget an extra byte for the end of the string!
	texturesToken = (char *)Z_Malloc((texturesTokenLength+1)*sizeof(char),PU_STATIC,NULL);
	// Copy the string.
	M_Memcpy(texturesToken, stringToUse+startPos, (size_t)texturesTokenLength);
	// Make the final character NUL.
	texturesToken[texturesTokenLength] = '\0';
	return texturesToken;
}

/** Undoes the last M_GetToken call
  * The current position along the string being parsed is reset to the last saved position.
  * This exists mostly because of R_ParseTexture/R_ParsePatch honestly, but could be useful elsewhere?
  * -Monster Iestyn (22/10/16)
 */
void M_UnGetToken(void)
{
	endPos = oldendPos;
}

/** Returns the current token's position.
 */
UINT32 M_GetTokenPos(void)
{
	return endPos;
}

#define NUMTOKENS 2
static const char *tokenizerInput = NULL;
static UINT32 tokenCapacity[NUMTOKENS] = {0};
static char *tokenizerToken[NUMTOKENS] = {NULL};
static UINT32 tokenizerStartPos = 0;
static UINT32 tokenizerEndPos = 0;
static UINT32 tokenizerInputLength = 0;
static UINT8 tokenizerInComment = 0; // 0 = not in comment, 1 = // Single-line, 2 = /* Multi-line */
static boolean tokenizerIsString = false; // did we strip quotes from this token?

void M_TokenizerOpen(const char *inputString, size_t inputLength)
{
	size_t i;

	tokenizerInput = inputString;
	for (i = 0; i < NUMTOKENS; i++)
	{
		tokenCapacity[i] = 1024;
		tokenizerToken[i] = (char*)Z_Malloc(tokenCapacity[i] * sizeof(char), PU_STATIC, NULL);
	}
	tokenizerInputLength = inputLength;
}

void M_TokenizerClose(void)
{
	size_t i;

	tokenizerInput = NULL;
	for (i = 0; i < NUMTOKENS; i++)
		Z_Free(tokenizerToken[i]);
	tokenizerStartPos = 0;
	tokenizerEndPos = 0;
	tokenizerInComment = 0;
	tokenizerIsString = false;
}

static void M_DetectComment(UINT32 *pos)
{
	if (tokenizerInComment)
		return;

	if (*pos >= tokenizerInputLength - 1)
		return;

	if (tokenizerInput[*pos] != '/')
		return;

	//Single-line comment start
	if (tokenizerInput[*pos + 1] == '/')
		tokenizerInComment = 1;
	//Multi-line comment start
	else if (tokenizerInput[*pos + 1] == '*')
		tokenizerInComment = 2;
}

static void M_ReadTokenString(UINT32 i)
{
	UINT32 tokenLength = tokenizerEndPos - tokenizerStartPos;
	if (tokenLength + 1 > tokenCapacity[i])
	{
		tokenCapacity[i] = tokenLength + 1;
		// Assign the memory. Don't forget an extra byte for the end of the string!
		tokenizerToken[i] = (char *)Z_Malloc(tokenCapacity[i] * sizeof(char), PU_STATIC, NULL);
	}

	// Copy the string.
	M_Memcpy(tokenizerToken[i], tokenizerInput + tokenizerStartPos, (size_t)tokenLength);

	// Make the final character NUL.
	tokenizerToken[i][tokenLength] = '\0';
}

const char *M_TokenizerRead(UINT32 i)
{
	if (!tokenizerInput)
		return NULL;

	tokenizerStartPos = tokenizerEndPos;

	// Reset string flag
	tokenizerIsString = false;

	// Try to detect comments now, in case we're pointing right at one
	M_DetectComment(&tokenizerStartPos);

	// Find the first non-whitespace char, or else the end of the string trying
	while ((tokenizerInput[tokenizerStartPos] == ' '
			|| tokenizerInput[tokenizerStartPos] == '\t'
			|| tokenizerInput[tokenizerStartPos] == '\r'
			|| tokenizerInput[tokenizerStartPos] == '\n'
			|| tokenizerInput[tokenizerStartPos] == '\0'
			|| tokenizerInput[tokenizerStartPos] == '=' || tokenizerInput[tokenizerStartPos] == ';' // UDMF TEXTMAP.
			|| tokenizerInComment != 0)
			&& tokenizerStartPos < tokenizerInputLength)
	{
		// Try to detect comment endings now
		if (tokenizerInComment == 1	&& tokenizerInput[tokenizerStartPos] == '\n')
			tokenizerInComment = 0; // End of line for a single-line comment
		else if (tokenizerInComment == 2
			&& tokenizerStartPos < tokenizerInputLength - 1
			&& tokenizerInput[tokenizerStartPos] == '*'
			&& tokenizerInput[tokenizerStartPos+1] == '/')
		{
			// End of multi-line comment
			tokenizerInComment = 0;
			tokenizerStartPos++; // Make damn well sure we're out of the comment ending at the end of it all
		}

		tokenizerStartPos++;
		M_DetectComment(&tokenizerStartPos);
	}

	// If the end of the string is reached, no token is to be read
	if (tokenizerStartPos == tokenizerInputLength) {
		tokenizerEndPos = tokenizerInputLength;
		return NULL;
	}
	// Else, if it's one of these three symbols, capture only this one character
	else if (tokenizerInput[tokenizerStartPos] == ','
			|| tokenizerInput[tokenizerStartPos] == '{'
			|| tokenizerInput[tokenizerStartPos] == '}')
	{
		tokenizerEndPos = tokenizerStartPos + 1;
		tokenizerToken[i][0] = tokenizerInput[tokenizerStartPos];
		tokenizerToken[i][1] = '\0';
		return tokenizerToken[i];
	}
	// Return entire string within quotes, except without the quotes.
	else if (tokenizerInput[tokenizerStartPos] == '"')
	{
		tokenizerEndPos = ++tokenizerStartPos;
		while (tokenizerInput[tokenizerEndPos] != '"' && tokenizerEndPos < tokenizerInputLength)
			tokenizerEndPos++;

		M_ReadTokenString(i);
		tokenizerEndPos++;

		// Tell us the the token was a string.
		tokenizerIsString = true;

		return tokenizerToken[i];
	}

	// Now find the end of the token. This includes several additional characters that are okay to capture as one character, but not trailing at the end of another token.
	tokenizerEndPos = tokenizerStartPos + 1;
	while ((tokenizerInput[tokenizerEndPos] != ' '
			&& tokenizerInput[tokenizerEndPos] != '\t'
			&& tokenizerInput[tokenizerEndPos] != '\r'
			&& tokenizerInput[tokenizerEndPos] != '\n'
			&& tokenizerInput[tokenizerEndPos] != ','
			&& tokenizerInput[tokenizerEndPos] != '{'
			&& tokenizerInput[tokenizerEndPos] != '}'
			&& tokenizerInput[tokenizerEndPos] != '=' && tokenizerInput[tokenizerEndPos] != ';' // UDMF TEXTMAP.
			&& tokenizerInComment == 0)
			&& tokenizerEndPos < tokenizerInputLength)
	{
		tokenizerEndPos++;
		// Try to detect comment starts now; if it's in a comment, we don't want it in this token
		M_DetectComment(&tokenizerEndPos);
	}

	M_ReadTokenString(i);
	return tokenizerToken[i];
}

UINT32 M_TokenizerGetEndPos(void)
{
	return tokenizerEndPos;
}

void M_TokenizerSetEndPos(UINT32 newPos)
{
	tokenizerEndPos = newPos;
}

boolean M_TokenizerJustReadString(void)
{
	return tokenizerIsString;
}

/** Count bits in a number.
  */
UINT8 M_CountBits(UINT32 num, UINT8 size)
{
	UINT8 i, sum = 0;

	for (i = 0; i < size; ++i)
		if (num & (1 << i))
			++sum;
	return sum;
}

const char *GetRevisionString(void)
{
	static char rev[9] = {0};
	if (rev[0])
		return rev;

	if (comprevision[0] == 'r')
		strncpy(rev, comprevision, 7);
	else
		snprintf(rev, 7, "r%s", comprevision);
	rev[7] = '\0';

	return rev;
}

// Vector/matrix math
TVector *VectorMatrixMultiply(TVector v, TMatrix m)
{
	static TVector ret;

	ret[0] = FixedMul(v[0],m[0][0]) + FixedMul(v[1],m[1][0]) + FixedMul(v[2],m[2][0]) + FixedMul(v[3],m[3][0]);
	ret[1] = FixedMul(v[0],m[0][1]) + FixedMul(v[1],m[1][1]) + FixedMul(v[2],m[2][1]) + FixedMul(v[3],m[3][1]);
	ret[2] = FixedMul(v[0],m[0][2]) + FixedMul(v[1],m[1][2]) + FixedMul(v[2],m[2][2]) + FixedMul(v[3],m[3][2]);
	ret[3] = FixedMul(v[0],m[0][3]) + FixedMul(v[1],m[1][3]) + FixedMul(v[2],m[2][3]) + FixedMul(v[3],m[3][3]);

	return &ret;
}

TMatrix *RotateXMatrix(angle_t rad)
{
	static TMatrix ret;
	const angle_t fa = rad>>ANGLETOFINESHIFT;
	const fixed_t cosrad = FINECOSINE(fa), sinrad = FINESINE(fa);

	ret[0][0] = FRACUNIT; ret[0][1] =       0; ret[0][2] = 0;        ret[0][3] = 0;
	ret[1][0] =        0; ret[1][1] =  cosrad; ret[1][2] = sinrad;   ret[1][3] = 0;
	ret[2][0] =        0; ret[2][1] = -sinrad; ret[2][2] = cosrad;   ret[2][3] = 0;
	ret[3][0] =        0; ret[3][1] =       0; ret[3][2] = 0;        ret[3][3] = FRACUNIT;

	return &ret;
}

#if 0
TMatrix *RotateYMatrix(angle_t rad)
{
	static TMatrix ret;
	const angle_t fa = rad>>ANGLETOFINESHIFT;
	const fixed_t cosrad = FINECOSINE(fa), sinrad = FINESINE(fa);

	ret[0][0] = cosrad;   ret[0][1] =        0; ret[0][2] = -sinrad;   ret[0][3] = 0;
	ret[1][0] = 0;        ret[1][1] = FRACUNIT; ret[1][2] = 0;         ret[1][3] = 0;
	ret[2][0] = sinrad;   ret[2][1] =        0; ret[2][2] = cosrad;    ret[2][3] = 0;
	ret[3][0] = 0;        ret[3][1] =        0; ret[3][2] = 0;         ret[3][3] = FRACUNIT;

	return &ret;
}
#endif

TMatrix *RotateZMatrix(angle_t rad)
{
	static TMatrix ret;
	const angle_t fa = rad>>ANGLETOFINESHIFT;
	const fixed_t cosrad = FINECOSINE(fa), sinrad = FINESINE(fa);

	ret[0][0] = cosrad;    ret[0][1] = sinrad;   ret[0][2] =        0; ret[0][3] = 0;
	ret[1][0] = -sinrad;   ret[1][1] = cosrad;   ret[1][2] =        0; ret[1][3] = 0;
	ret[2][0] = 0;         ret[2][1] = 0;        ret[2][2] = FRACUNIT; ret[2][3] = 0;
	ret[3][0] = 0;         ret[3][1] = 0;        ret[3][2] =        0; ret[3][3] = FRACUNIT;

	return &ret;
}

/** Set of functions to take in a size_t as an argument,
  * put the argument in a character buffer, and return the
  * pointer to that buffer.
  * This is to eliminate usage of PRIdS, so gettext can work
  * with *all* of SRB2's strings.
  */
char *sizeu1(size_t num)
{
	static char sizeu1_buf[28];
	sprintf(sizeu1_buf, "%" PRIdS, num);
	return sizeu1_buf;
}

char *sizeu2(size_t num)
{
	static char sizeu2_buf[28];
	sprintf(sizeu2_buf, "%" PRIdS, num);
	return sizeu2_buf;
}

char *sizeu3(size_t num)
{
	static char sizeu3_buf[28];
	sprintf(sizeu3_buf, "%" PRIdS, num);
	return sizeu3_buf;
}

char *sizeu4(size_t num)
{
	static char sizeu4_buf[28];
	sprintf(sizeu4_buf, "%" PRIdS, num);
	return sizeu4_buf;
}

char *sizeu5(size_t num)
{
	static char sizeu5_buf[28];
	sprintf(sizeu5_buf, "%" PRIdS, num);
	return sizeu5_buf;
}

/** Return the appropriate message for a file error or end of file.
*/
const char *M_FileError(FILE *fp)
{
	if (ferror(fp))
		return strerror(errno);
	else
		return "end-of-file";
}

/** Return the number of parts of this path.
*/
int M_PathParts(const char *p)
{
	int parts = 0;

	if (p == NULL)
		return 0;

#ifdef _WIN32
	if (!strncmp(&p[1], ":\\", 2))
		p += 3;
#endif

	while (*(p += strspn(p, PATHSEP)))
	{
		parts++;
		p += strcspn(p, PATHSEP);
	}

	return parts;
}

/** Check whether a path is an absolute path.
*/
boolean M_IsPathAbsolute(const char *path)
{
#ifdef _WIN32
	return ( strncmp(&path[1], ":\\", 2) == 0 );
#else
	return ( path[0] == '/' );
#endif
}

/** I_mkdir for each part of the path.
*/
void M_MkdirEachUntil(const char *cpath, int start, int end, int mode)
{
	char path[256];
	char *p;
	int n;
	int c;

	if (end > 0 && end <= start)
		return;

	strlcpy(path, cpath, sizeof path);

#ifdef _WIN32
	if (!strncmp(&path[1], ":\\", 2))
		p = &path[3];
	else
#endif
		p = path;

	while (end != 0 && *(p += strspn(p, PATHSEP)))
	{
		n = strcspn(p, PATHSEP);

		if (start > 0)
			start--;
		else
		{
			c = p[n];
			p[n] = '\0';

			I_mkdir(path, mode);

			p[n] = c;
		}

		p += n;

		if (end > 0)
			end--;
	}
}

void M_MkdirEach(const char *path, int start, int mode)
{
	M_MkdirEachUntil(path, start, -1, mode);
}

int M_JumpWord(const char *line)
{
	int c;

	c = line[0];

	if (isspace(c))
		return strspn(line, " ");
	else if (ispunct(c))
		return strspn(line, PUNCTUATION);
	else
	{
		if (isspace(line[1]))
			return 1 + strspn(&line[1], " ");
		else
			return strcspn(line, " " PUNCTUATION);
	}
}

int M_JumpWordReverse(const char *line, int offset)
{
	int (*is)(int);
	int c;
	c = line[--offset];
	if (isspace(c))
		is = isspace;
	else if (ispunct(c))
		is = ispunct;
	else
		is = isalnum;
	c = (*is)(line[offset]);
	while (offset > 0 &&
			(*is)(line[offset - 1]) == c)
		offset--;
	return offset;
}

const char * M_Ftrim (double f)
{
	static char dig[9];/* "0." + 6 digits (6 is printf's default) */
	int i;
	/* I know I said it's the default, but just in case... */
	sprintf(dig, "%.6f", fabs(modf(f, &f)));
	/* trim trailing zeroes */
	for (i = strlen(dig)-1; dig[i] == '0'; --i)
		;
	if (dig[i] == '.')/* :NOTHING: */
		return "";
	else
	{
		dig[i + 1] = '\0';
		return &dig[1];/* skip the 0 */
	}
}

/** Enable floating point denormal-to-zero section, if necessary */
floatdenormalstate_t M_EnterFloatDenormalToZero(void)
{
#ifdef NEED_INTEL_DENORMAL_BIT
	floatdenormalstate_t state = 0;
	state |= _MM_GET_FLUSH_ZERO_MODE() == _MM_FLUSH_ZERO_ON ? 1 : 0;
	state |= _MM_GET_DENORMALS_ZERO_MODE() == _MM_DENORMALS_ZERO_ON ? 2 : 0;

	if ((state & 1) == 0)
	{
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	}
	if ((state & 2) == 0)
	{
		_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	}
	return state;
#else
	return 0;
#endif
}

/** Exit floating point denormal-to-zero section, if necessary, restoring previous state */
void M_ExitFloatDenormalToZero(floatdenormalstate_t previous)
{
#ifdef NEED_INTEL_DENORMAL_BIT
	if ((previous & 1) == 0)
	{
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
	}
	if ((previous & 2) == 0)
	{
		_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_OFF);
	}
	return;
#else
	(void)previous;
	return;
#endif
}
