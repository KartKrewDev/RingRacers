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
/// \file
/// \brief Main program, simply calls D_SRB2Main and D_SRB2Loop, the high level loop.

#include "../doomdef.h"
#include "../m_argv.h"
#include "../d_main.h"
#include "../m_misc.h"/* path shit */
#include "../i_system.h"
#include "../core/string.h"

#include <exception>

#include <tracy/tracy/Tracy.hpp>

#if defined (__GNUC__) || defined (__unix__)
#include <unistd.h>
#endif

#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#include <errno.h>
#endif

#include "time.h" // For log timestamps

#ifdef HAVE_TTF
#include "SDL.h"
#include "i_ttf.h"
#endif

#if defined (_WIN32) && !defined (main)
//#define SDLMAIN
#endif

#ifdef SDLMAIN
#include "SDL_main.h"
#elif defined(FORCESDLMAIN)
extern int SDL_main(int argc, char *argv[]);
#endif

#ifdef LOGMESSAGES
FILE *logstream = NULL;
char logfilename[1024];
#endif

#ifndef DOXYGEN
#ifndef O_TEXT
#define O_TEXT 0
#endif

#ifndef O_SEQUENTIAL
#define O_SEQUENTIAL 0
#endif
#endif

#ifdef LOGMESSAGES
static void InitLogging(void)
{
	const char *logdir = NULL;
	time_t my_time;
	struct tm * timeinfo;
	const char *format;
	const char *reldir;
	int left;
	boolean fileabs;
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
	const char *link;
#endif

	logdir = D_Home();

	my_time = time(NULL);
	timeinfo = localtime(&my_time);

	if (M_CheckParm("-logfile") && M_IsNextParm())
	{
		format = M_GetNextParm();
		fileabs = M_IsPathAbsolute(format);
	}
	else
	{
		format = "log-%Y-%m-%d_%H-%M-%S.txt";
		fileabs = false;
	}

	if (fileabs)
	{
		strftime(logfilename, sizeof logfilename, format, timeinfo);
	}
	else
	{
		if (M_CheckParm("-logdir") && M_IsNextParm())
			reldir = M_GetNextParm();
		else
			reldir = "logs";

		if (M_IsPathAbsolute(reldir))
		{
			left = snprintf(logfilename, sizeof logfilename,
					"%s" PATHSEP, reldir);
		}
		else
#ifdef DEFAULTDIR
		if (logdir)
		{
			left = snprintf(logfilename, sizeof logfilename,
					"%s" PATHSEP DEFAULTDIR PATHSEP "%s" PATHSEP, logdir, reldir);
		}
		else
#endif/*DEFAULTDIR*/
		{
			left = snprintf(logfilename, sizeof logfilename,
					"." PATHSEP "%s" PATHSEP, reldir);
		}

		strftime(&logfilename[left], sizeof logfilename - left,
				format, timeinfo);
	}

	M_MkdirEachUntil(logfilename,
			M_PathParts(logdir) - 1,
			M_PathParts(logfilename) - 1, 0755);

#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
	logstream = fopen(logfilename, "w");
#ifdef DEFAULTDIR
	if (logdir)
		link = va("%s/" DEFAULTDIR "/latest-log.txt", logdir);
	else
#endif/*DEFAULTDIR*/
		link = "latest-log.txt";
	unlink(link);
	if (symlink(logfilename, link) == -1)
	{
		I_OutputMsg("Error symlinking latest-log.txt: %s\n", strerror(errno));
	}
#else/*defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)*/
	logstream = fopen("latest-log.txt", "wt+");
#endif/*defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)*/
}
#endif

#if defined(_WIN32) && !defined(_MSC_VER)
static void init_exchndl()
{
	HMODULE exchndl_module = LoadLibraryA("exchndl.dll");

	if (exchndl_module == NULL)
	{
		I_Error("exchndl.dll or mgwhelp.dll is missing, RPT files cannot be generated.\n"
			"If you NEED to run without them, use -noexchndl.");
	}

	using PFN_ExcHndlInit = void(*)(void);
	PFN_ExcHndlInit pfnExcHndlInit = reinterpret_cast<PFN_ExcHndlInit>(
		GetProcAddress(exchndl_module, "ExcHndlInit"));
	if (pfnExcHndlInit != NULL)
		(pfnExcHndlInit)();
}
#endif

#ifdef _WIN32
static void
ChDirToExe (void)
{
	CHAR path[MAX_PATH];
	if (GetModuleFileNameA(NULL, path, MAX_PATH) > 0)
	{
		strrchr(path, '\\')[0] = '\0';
		SetCurrentDirectoryA(path);
	}
}
#endif

static void walk_exception_stack(srb2::String& accum, bool nested) {
	if (nested)
		accum.append("\n  Caused by: Unknown exception");
	else
		accum.append("Uncaught exception: Unknown exception");
}

static void walk_exception_stack(srb2::String& accum, const std::exception& ex, bool nested) {
	if (nested)
		accum.append("\n  Caused by: ");
	else
		accum.append("Uncaught exception: ");

	accum.append("(");
	accum.append(typeid(ex).name());
	accum.append(") ");
	accum.append(ex.what());

	try {
		std::rethrow_if_nested(ex);
	} catch (const std::exception& ex) {
		walk_exception_stack(accum, ex, true);
	} catch (...) {
		walk_exception_stack(accum, true);
	}
}


/**	\brief	The main function

	\param	argc	number of arg
	\param	*argv	string table

	\return	int
*/
#if defined (__GNUC__) && (__GNUC__ >= 4)
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif

#ifdef FORCESDLMAIN
int SDL_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	myargc = argc;
	myargv = argv; /// \todo pull out path to exe from this string

	tracy::SetThreadName("Main");

#ifdef HAVE_TTF
#ifdef _WIN32
	I_StartupTTF(FONTPOINTSIZE, SDL_INIT_VIDEO|SDL_INIT_AUDIO, SDL_SWSURFACE);
#else
	I_StartupTTF(FONTPOINTSIZE, SDL_INIT_VIDEO, SDL_SWSURFACE);
#endif
#endif

#ifdef _WIN32
	ChDirToExe();
#endif

#ifdef LOGMESSAGES
	if (!M_CheckParm("-nolog"))
		InitLogging();
#endif/*LOGMESSAGES*/

	//I_OutputMsg("I_StartupSystem() ...\n");
	I_StartupSystem();

#if defined (_WIN32) && !defined(_MSC_VER)
	if (!M_CheckParm("-noexchndl"))
	{
		init_exchndl();
	}
#endif

	try {

	// startup SRB2
	CONS_Printf("Setting up Dr. Robotnik's Ring Racers...\n");
	D_SRB2Main();
#ifdef LOGMESSAGES
	if (!M_CheckParm("-nolog"))
		CONS_Printf("Logfile: %s\n", logfilename);
#endif
	CONS_Printf("Entering main game loop...\n");
	// never return
	D_SRB2Loop();

	} catch (const std::exception& ex) {
		srb2::String exception;
		walk_exception_stack(exception, ex, false);
		I_Error("%s", exception.c_str());
	} catch (...) {
		srb2::String exception;
		walk_exception_stack(exception, false);
		I_Error("%s", exception.c_str());
	}

#ifdef BUGTRAP
	// This is safe even if BT didn't start.
	ShutdownBugTrap();
#endif

	// return to OS
	return 0;
}

#ifdef _MSC_VER
int WINAPI WinMain(HINSTANCE pInstance, HINSTANCE pPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	return main(__argc, __argv);
}
#endif

void* operator new(size_t count)
{
	auto p = malloc(count);
	TracyAlloc(p, count);
	return p;
}

void operator delete(void* ptr) noexcept
{
	TracyFree(ptr);
	free(ptr);
}
