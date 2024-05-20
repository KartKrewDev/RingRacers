// Emacs style mode select   -*- C++ -*-
//
// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
//
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// Changes by Graue <graue@oceanbase.org> are in the public domain.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief SRB2 system stuff for SDL

#include <thread>

#include <signal.h>

#ifdef _WIN32
#define RPC_NO_WINDOWS_H
#include <windows.h>
#include "../doomtype.h"
typedef BOOL (WINAPI *p_GetDiskFreeSpaceExA)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
typedef BOOL (WINAPI *p_IsProcessorFeaturePresent) (DWORD);
typedef DWORD (WINAPI *p_timeGetTime) (void);
typedef UINT (WINAPI *p_timeEndPeriod) (UINT);
typedef HANDLE (WINAPI *p_OpenFileMappingA) (DWORD, BOOL, LPCSTR);
typedef LPVOID (WINAPI *p_MapViewOfFile) (HANDLE, DWORD, DWORD, DWORD, SIZE_T);
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#elif defined (_MSC_VER)
#include <direct.h>
#endif
#if defined (__unix__) || defined (UNIXCOMMON)
#include <fcntl.h>
#endif

#include <stdio.h>
#ifdef _WIN32
#include <conio.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED
#include "SDL.h"

#ifdef HAVE_TTF
#include "i_ttf.h"
#endif

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#include "SDL_cpuinfo.h"
#define HAVE_SDLCPUINFO

#if defined (__unix__) || defined(__APPLE__) || (defined (UNIXCOMMON) && !defined (__HAIKU__))
#if defined (__linux__)
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
/*For meminfo*/
#include <sys/types.h>
#ifdef FREEBSD
#include <kvm.h>
#endif
#include <nlist.h>
#include <sys/vmmeter.h>
#endif
#endif

#if defined (__linux__) || (defined (UNIXCOMMON) && !defined (__HAIKU__))
#ifndef NOTERMIOS
#include <termios.h>
#include <sys/ioctl.h> // ioctl
#define HAVE_TERMIOS
#endif
#endif

#if (defined (__unix__) && !defined (_MSDOS)) || (defined (UNIXCOMMON) && !defined(__APPLE__))
#include <errno.h>
#include <sys/wait.h>
#define NEWSIGNALHANDLER
#endif

#ifndef NOMUMBLE
#ifdef __linux__ // need -lrt
#include <sys/mman.h>
#ifdef MAP_FAILED
#define HAVE_SHM
#endif
#include <wchar.h>
#endif

#ifdef _WIN32
#define HAVE_MUMBLE
#define WINMUMBLE
#elif defined (HAVE_SHM)
#define HAVE_MUMBLE
#endif
#endif // NOMUMBLE

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef __APPLE__
#include "macosx/mac_resources.h"
#endif

#ifndef errno
#include <errno.h>
#endif

#if (defined(__linux__) && defined(__USE_GNU)) || (defined (__unix__) || defined (UNIXCOMMON)) && !defined(__linux__) || defined(__APPLE__)
#include <execinfo.h>
#include <time.h>
#define UNIXBACKTRACE
#endif

// Locations for searching for bios.pk3
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#define DEFAULTWADLOCATION1 "/usr/local/share/games/RingRacers"
#define DEFAULTWADLOCATION2 "/usr/local/games/RingRacers"
#define DEFAULTWADLOCATION3 "/usr/share/games/RingRacers"
#define DEFAULTWADLOCATION4 "/usr/games/RingRacers"
#define DEFAULTSEARCHPATH1 "/usr/local/games"
#define DEFAULTSEARCHPATH2 "/usr/games"
#define DEFAULTSEARCHPATH3 "/usr/local"
#endif

/**	\brief WAD file to look for
*/
#define WADKEYWORD "bios.pk3"
/**	\brief holds wad path
*/
static char returnWadPath[256];

//Alam_GBC: SDL

#include "../doomdef.h"
#include "../m_misc.h"
#include "../i_time.h"
#include "../i_video.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../i_threads.h"
#include "../screen.h" //vid.WndParent
#include "../d_net.h"
#include "../g_game.h"
#include "../filesrch.h"
#include "../s_sound.h"
#include "../core/thread_pool.h"
#include "endtxt.h"
#include "sdlmain.h"

#include "../i_joy.h"

#include "../m_argv.h"

// SRB2Kart
#include "../k_pwrlv.h"
#include "../r_main.h" // Frame interpolation/uncapped
#include "../r_fps.h"

#include "../k_menu.h"

#ifdef MAC_ALERT
#include "macosx/mac_alert.h"
#endif

#include "../d_main.h"

#if !defined(NOMUMBLE) && defined(HAVE_MUMBLE)
// Mumble context string
#include "../d_clisrv.h"
#include "../byteptr.h"
#endif

static std::thread::id g_main_thread_id;

/**	\brief SDL info about joysticks
*/
SDLJoyInfo_t JoyInfo[MAXSPLITSCREENPLAYERS];

SDL_bool consolevent = SDL_FALSE;
SDL_bool framebuffer = SDL_FALSE;

UINT8 keyboard_started = false;
boolean g_in_exiting_signal_handler = false;

#ifdef UNIXBACKTRACE
#define STDERR_WRITE(string) if (fd != -1) I_OutputMsg("%s", string)
#define CRASHLOG_WRITE(string) if (fd != -1) write(fd, string, strlen(string))
#define CRASHLOG_STDERR_WRITE(string) \
	if (fd != -1)\
		write(fd, string, strlen(string));\
	I_OutputMsg("%s", string)

static void write_backtrace(INT32 signal)
{
	int fd = -1;
	size_t size;
	time_t rawtime;
	struct tm timeinfo;

	enum { BT_SIZE = 1024, STR_SIZE = 32 };
	void *array[BT_SIZE];
	char timestr[STR_SIZE];

	const char *error = "An error occurred within Dr. Robotnik's Ring Racers! Send this stack trace to someone who can help!\n";
	const char *error2 = "(Or find crash-log.txt in your Ring Racers directory.)\n"; // Shown only to stderr.

	fd = open(va("%s" PATHSEP "%s", srb2home, "crash-log.txt"), O_CREAT|O_APPEND|O_RDWR, S_IRUSR|S_IWUSR);

	if (fd == -1)
		I_OutputMsg("\nWARNING: Couldn't open crash log for writing! Make sure your permissions are correct. Please save the below report!\n");

	// Get the current time as a string.
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	strftime(timestr, STR_SIZE, "%a, %d %b %Y %T %z", &timeinfo);

	CRASHLOG_WRITE("------------------------\n"); // Nice looking seperator

	CRASHLOG_STDERR_WRITE("\n"); // Newline to look nice for both outputs.
	CRASHLOG_STDERR_WRITE(error); // "Oops, SRB2 crashed" message
	STDERR_WRITE(error2); // Tell the user where the crash log is.

	// Tell the log when we crashed.
	CRASHLOG_WRITE("Time of crash: ");
	CRASHLOG_WRITE(timestr);
	CRASHLOG_WRITE("\n");

	// Give the crash log the cause and a nice 'Backtrace:' thing
	// The signal is given to the user when the parent process sees we crashed.
	CRASHLOG_WRITE("Cause: ");
	CRASHLOG_WRITE(strsignal(signal));
	CRASHLOG_WRITE("\n"); // Newline for the signal name

	CRASHLOG_STDERR_WRITE("\nBacktrace:\n");

	// Flood the output and log with the backtrace
	size = backtrace(array, BT_SIZE);
	backtrace_symbols_fd(array, size, fd);
	backtrace_symbols_fd(array, size, STDERR_FILENO);

	CRASHLOG_WRITE("\n"); // Write another newline to the log so it looks nice :)

	close(fd);
}
#undef STDERR_WRITE
#undef CRASHLOG_WRITE
#undef CRASHLOG_STDERR_WRITE
#endif // UNIXBACKTRACE

static void I_ShowErrorMessageBox(const char *messagefordevelopers, boolean dumpmade)
{
	static char finalmessage[2048];
	size_t firstimpressionsline = 3; // "Dr Robotnik's Ring Racers" has encountered...

	if (M_CheckParm("-dedicated"))
		return;

	snprintf(
		finalmessage,
		sizeof(finalmessage),
			"Hee Ho!\n"
			"\n"
			"\"Dr. Robotnik's Ring Racers\" has encountered an unrecoverable error and needs to close.\n"
			"This is (usually) not your fault, but we encourage you to report it in the community. This should be done alongside your "
			"%s"
			"log file (%s).\n"
			"\n"
			"The following information is for a programmer (please be nice to them!) but\n"
			"may also be useful for server hosts and add-on creators.\n"
			"\n"
			"%s",
		dumpmade ?
#if defined (UNIXBACKTRACE)
			"crash-log.txt"
#elif defined (_WIN32)
			".rpt crash dump"
#endif
			" (very important!) and " : "",
#ifdef LOGMESSAGES
		logfilename[0] ? logfilename :
#endif
		"uh oh, one wasn't made!?",
		messagefordevelopers);

	// Rudementary word wrapping.
	// Simple and effective. Does not handle nonuniform letter sizes, etc. but who cares.
	// We can't use V_ScaledWordWrap, which this shares DNA with, because no guarantee
	// string character graphics exist as reference in the error handler...
	{
		size_t max = 0, maxatstart = 0, start = 0, width = 0, i;

		for (i = 0; finalmessage[i]; i++)
		{
			if (finalmessage[i] == ' ')
			{
				start = i;
				max += 4;
				maxatstart = max;
			}
			else if (finalmessage[i] == '\n')
			{
				if (firstimpressionsline > 0)
				{
					firstimpressionsline--;
					if (firstimpressionsline == 0)
					{
						width = max;
					}
				}
				start = 0;
				max = 0;
				maxatstart = 0;
				continue;
			}
			else
				max += 8;

			// Start trying to wrap if presumed length exceeds the space we want.
			if (width > 0 && max >= width && start > 0)
			{
				finalmessage[start] = '\n';
				max -= maxatstart;
				start = 0;
			}
		}
	}

	// Implement message box with SDL_ShowSimpleMessageBox,
	// which should fail gracefully if it can't put a message box up
	// on the target system
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
		"Dr. Robotnik's Ring Racers " VERSIONSTRING " Error",
		finalmessage, NULL);

	// Note that SDL_ShowSimpleMessageBox does *not* require SDL to be
	// initialized at the time, so calling it after SDL_Quit() is
	// perfectly okay! In addition, we do this on purpose so the
	// fullscreen window is closed before displaying the error message
	// in case the fullscreen window blocks it for some absurd reason.
}

static void I_ReportSignal(int num, int coredumped)
{
	//static char msg[] = "oh no! back to reality!\r\n";
	const char *      sigmsg;
	char msg[128];

	switch (num)
	{
//	case SIGINT:
//		sigmsg = "SIGINT - interrupted";
//		break;
	case SIGILL:
		sigmsg = "SIGILL - illegal instruction - invalid function image";
		break;
	case SIGFPE:
		sigmsg = "SIGFPE - mathematical exception";
		break;
	case SIGSEGV:
		sigmsg = "SIGSEGV - segment violation";
		break;
//	case SIGTERM:
//		sigmsg = "SIGTERM - Software termination signal from kill";
//		break;
//	case SIGBREAK:
//		sigmsg = "SIGBREAK - Ctrl-Break sequence";
//		break;
	case SIGABRT:
		sigmsg = "SIGABRT - abnormal termination triggered by abort call";
		break;
	default:
		sprintf(msg,"signal number %d", num);
		if (coredumped)
			sigmsg = 0;
		else
			sigmsg = msg;
	}

	if (coredumped)
	{
		if (sigmsg)
			strcpy(msg, sigmsg);
		strcat(msg, " (core dumped)");

		sigmsg = msg;
	}

	I_OutputMsg("\nProcess killed by signal: %s\n\n", sigmsg);

	I_ShowErrorMessageBox(sigmsg,
#if defined (UNIXBACKTRACE)
		true
#elif defined (_WIN32)
		!M_CheckParm("-noexchndl")
#else
		false
#endif
	);
}

#ifndef NEWSIGNALHANDLER
static ATTRNORETURN void signal_handler(INT32 num)
{
	g_in_exiting_signal_handler = true;

	if (g_main_thread_id != std::this_thread::get_id())
	{
		// Do not attempt any sort of recovery if this signal triggers off the main thread
		signal(num, SIG_DFL);
		raise(num);
		exit(-2);
	}

	D_QuitNetGame(); // Fix server freezes
	CL_AbortDownloadResume();
	G_DirtyGameData();
#ifdef UNIXBACKTRACE
	write_backtrace(num);
#endif
	I_ReportSignal(num, 0);
	signal(num, SIG_DFL);               //default signal action
	raise(num);
}
#endif

FUNCNORETURN static ATTRNORETURN void quit_handler(int num)
{
	signal(num, SIG_DFL); //default signal action
	raise(num);
	I_Quit();
}

#ifdef HAVE_TERMIOS
// TERMIOS console code from Quake3: thank you!
SDL_bool stdin_active = SDL_TRUE;

typedef struct
{
	size_t cursor;
	char buffer[256];
} feild_t;

feild_t tty_con;

// when printing general stuff to stdout stderr (Sys_Printf)
//   we need to disable the tty console stuff
// this increments so we can recursively disable
static INT32 ttycon_hide = 0;
// some key codes that the terminal may be using
// TTimo NOTE: I'm not sure how relevant this is
static INT32 tty_erase;
static INT32 tty_eof;

static struct termios tty_tc;

// =============================================================
// tty console routines
// NOTE: if the user is editing a line when something gets printed to the early console then it won't look good
//   so we provide tty_Clear and tty_Show to be called before and after a stdout or stderr output
// =============================================================

// flush stdin, I suspect some terminals are sending a LOT of garbage
// FIXME TTimo relevant?
#if 0
static inline void tty_FlushIn(void)
{
	char key;
	while (read(STDIN_FILENO, &key, 1)!=-1);
}
#endif

// do a backspace
// TTimo NOTE: it seems on some terminals just sending '\b' is not enough
//   so for now, in any case we send "\b \b" .. yeah well ..
//   (there may be a way to find out if '\b' alone would work though)
static void tty_Back(void)
{
	char key;
	ssize_t d;
	key = '\b';
	d = write(STDOUT_FILENO, &key, 1);
	key = ' ';
	d = write(STDOUT_FILENO, &key, 1);
	key = '\b';
	d = write(STDOUT_FILENO, &key, 1);
	(void)d;
}

static void tty_Clear(void)
{
	size_t i;
	if (tty_con.cursor>0)
	{
		for (i=0; i<tty_con.cursor; i++)
		{
			tty_Back();
		}
	}

}

// clear the display of the line currently edited
// bring cursor back to beginning of line
static inline void tty_Hide(void)
{
	//I_Assert(consolevent);
	if (ttycon_hide)
	{
		ttycon_hide++;
		return;
	}
	tty_Clear();
	ttycon_hide++;
}

// show the current line
// FIXME TTimo need to position the cursor if needed??
static inline void tty_Show(void)
{
	size_t i;
	ssize_t d;
	//I_Assert(consolevent);
	I_Assert(ttycon_hide>0);
	ttycon_hide--;
	if (ttycon_hide == 0 && tty_con.cursor)
	{
		for (i=0; i<tty_con.cursor; i++)
		{
			d = write(STDOUT_FILENO, tty_con.buffer+i, 1);
		}
	}
	(void)d;
}

// never exit without calling this, or your terminal will be left in a pretty bad state
static void I_ShutdownConsole(void)
{
	if (consolevent)
	{
		I_OutputMsg("Shutdown tty console\n");
		consolevent = SDL_FALSE;
		tcsetattr (STDIN_FILENO, TCSADRAIN, &tty_tc);
	}
}

static void I_StartupConsole(void)
{
	struct termios tc;

	// TTimo
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=390 (404)
	// then SIGTTIN or SIGTOU is emitted, if not catched, turns into a SIGSTP
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	consolevent = static_cast<SDL_bool>(!M_CheckParm("-noconsole"));
	framebuffer = static_cast<SDL_bool>(M_CheckParm("-framebuffer"));

	if (framebuffer)
		consolevent = SDL_FALSE;

	if (!consolevent) return;

	if (isatty(STDIN_FILENO)!=1)
	{
		I_OutputMsg("stdin is not a tty, tty console mode failed\n");
		consolevent = SDL_FALSE;
		return;
	}
	memset(&tty_con, 0x00, sizeof(tty_con));
	tcgetattr (0, &tty_tc);
	tty_erase = tty_tc.c_cc[VERASE];
	tty_eof = tty_tc.c_cc[VEOF];
	tc = tty_tc;
	/*
	 ECHO: don't echo input characters
	 ICANON: enable canonical mode.  This  enables  the  special
	  characters  EOF,  EOL,  EOL2, ERASE, KILL, REPRINT,
	  STATUS, and WERASE, and buffers by lines.
	 ISIG: when any of the characters  INTR,  QUIT,  SUSP,  or
	  DSUSP are received, generate the corresponding signal
	*/
	tc.c_lflag &= ~(ECHO | ICANON);
	/*
	 ISTRIP strip off bit 8
	 INPCK enable input parity checking
	 */
	tc.c_iflag &= ~(ISTRIP | INPCK);
	tc.c_cc[VMIN] = 0; //1?
	tc.c_cc[VTIME] = 0;
	tcsetattr (0, TCSADRAIN, &tc);
}

void I_GetConsoleEvents(void)
{
	// we use this when sending back commands
	event_t ev = {};
	char key = 0;
	ssize_t d;

	if (!consolevent)
		return;

	ev.type = ev_console;
	if (read(STDIN_FILENO, &key, 1) == -1 || !key)
		return;

	// we have something
	// backspace?
	// NOTE TTimo testing a lot of values .. seems it's the only way to get it to work everywhere
	if ((key == tty_erase) || (key == 127) || (key == 8))
	{
		if (tty_con.cursor > 0)
		{
			tty_con.cursor--;
			tty_con.buffer[tty_con.cursor] = '\0';
			tty_Back();
		}
		ev.data1 = KEY_BACKSPACE;
	}
	else if (key < ' ') // check if this is a control char
	{
		if (key == '\n')
		{
			tty_Clear();
			tty_con.cursor = 0;
			ev.data1 = KEY_ENTER;
		}
		else return;
	}
	else
	{
		// push regular character
		ev.data1 = tty_con.buffer[tty_con.cursor] = key;
		tty_con.cursor++;
		// print the current line (this is differential)
		d = write(STDOUT_FILENO, &key, 1);
	}
	if (ev.data1) D_PostEvent(&ev);
	//tty_FlushIn();
	(void)d;
}

#elif defined (_WIN32)
static BOOL I_ReadyConsole(HANDLE ci)
{
	DWORD gotinput;
	if (ci == INVALID_HANDLE_VALUE) return FALSE;
	if (WaitForSingleObject(ci,0) != WAIT_OBJECT_0) return FALSE;
	if (GetFileType(ci) != FILE_TYPE_CHAR) return FALSE;
	if (!GetConsoleMode(ci, &gotinput)) return FALSE;
	return (GetNumberOfConsoleInputEvents(ci, &gotinput) && gotinput);
}

static boolean entering_con_command = false;

static void Impl_HandleKeyboardConsoleEvent(KEY_EVENT_RECORD evt, HANDLE co)
{
	event_t event;
	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	DWORD t;

	memset(&event,0x00,sizeof (event));

	if (evt.bKeyDown)
	{
		event.type = ev_console;
		entering_con_command = true;
		switch (evt.wVirtualKeyCode)
		{
			case VK_ESCAPE:
			case VK_TAB:
				event.data1 = KEY_NULL;
				break;
			case VK_RETURN:
				entering_con_command = false;
				/* FALLTHRU */
			default:
				//event.data1 = MapVirtualKey(evt.wVirtualKeyCode,2); // convert in to char
				event.data1 = evt.uChar.AsciiChar;
		}
		if (co != INVALID_HANDLE_VALUE && GetFileType(co) == FILE_TYPE_CHAR && GetConsoleMode(co, &t))
		{
			if (event.data1 && event.data1 != KEY_LSHIFT && event.data1 != KEY_RSHIFT)
			{
#ifdef _UNICODE
				WriteConsole(co, &evt.uChar.UnicodeChar, 1, &t, NULL);
#else
				WriteConsole(co, &evt.uChar.AsciiChar, 1 , &t, NULL);
#endif
			}
			if (evt.wVirtualKeyCode == VK_BACK
				&& GetConsoleScreenBufferInfo(co,&CSBI))
			{
				WriteConsoleOutputCharacterA(co, " ",1, CSBI.dwCursorPosition, &t);
			}
		}
	}
	if (event.data1) D_PostEvent(&event);
}

void I_GetConsoleEvents(void)
{
	HANDLE ci = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
	INPUT_RECORD input;
	DWORD t;

	while (I_ReadyConsole(ci) && ReadConsoleInput(ci, &input, 1, &t) && t)
	{
		switch (input.EventType)
		{
			case KEY_EVENT:
				Impl_HandleKeyboardConsoleEvent(input.Event.KeyEvent, co);
				break;
			case MOUSE_EVENT:
			case WINDOW_BUFFER_SIZE_EVENT:
			case MENU_EVENT:
			case FOCUS_EVENT:
				break;
		}
	}
}

static void I_StartupConsole(void)
{
	HANDLE ci, co;
	const INT32 ded = M_CheckParm("-dedicated");
	BOOL gotConsole = FALSE;
	if (M_CheckParm("-console") || ded)
		gotConsole = AllocConsole();
#ifdef _DEBUG
	else if (M_CheckParm("-noconsole") && !ded)
#else
	else if (!M_CheckParm("-console") && !ded)
#endif
	{
		FreeConsole();
		gotConsole = FALSE;
	}

	if (gotConsole)
	{
		SetConsoleTitleA("Dr. Robotnik's Ring Racers Console");
		consolevent = SDL_TRUE;
	}

	//Let get the real console HANDLE, because Mingw's Bash is bad!
	ci = CreateFile(TEXT("CONIN$") ,               GENERIC_READ, FILE_SHARE_READ,  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	co = CreateFile(TEXT("CONOUT$"), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ci != INVALID_HANDLE_VALUE)
	{
		const DWORD CM = ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT;
		SetStdHandle(STD_INPUT_HANDLE, ci);
		if (GetFileType(ci) == FILE_TYPE_CHAR)
			SetConsoleMode(ci, CM); //default mode but no ENABLE_MOUSE_INPUT
	}
	if (co != INVALID_HANDLE_VALUE)
	{
		SetStdHandle(STD_OUTPUT_HANDLE, co);
		SetStdHandle(STD_ERROR_HANDLE, co);
	}
}
static inline void I_ShutdownConsole(void){}
#else
void I_GetConsoleEvents(void){}
static inline void I_StartupConsole(void)
{
#ifdef _DEBUG
	consolevent = M_CheckParm("-noconsole") > 0 ? SDL_FALSE : SDL_TRUE;
#else
	consolevent = M_CheckParm("-console") > 0 ? SDL_TRUE : SDL_FALSE;
#endif

	framebuffer = M_CheckParm("-framebuffer") > 0 ? SDL_TRUE : SDL_FALSE;

	if (framebuffer)
		consolevent = SDL_FALSE;
}
static inline void I_ShutdownConsole(void){}
#endif

//
// StartupKeyboard
//
static void I_RegisterSignals (void)
{
	g_main_thread_id = std::this_thread::get_id();

#ifdef SIGINT
	signal(SIGINT , quit_handler);
#endif
#ifdef SIGBREAK
	signal(SIGBREAK , quit_handler);
#endif
#ifdef SIGTERM
	signal(SIGTERM , quit_handler);
#endif

	// If these defines don't exist,
	// then compilation would have failed above us...
#ifndef NEWSIGNALHANDLER
	signal(SIGILL , signal_handler);
	signal(SIGSEGV , signal_handler);
	signal(SIGABRT , signal_handler);
	signal(SIGFPE , signal_handler);
#endif
}

#ifdef NEWSIGNALHANDLER
static void signal_handler_child(INT32 num)
{
	G_DirtyGameData();

#ifdef UNIXBACKTRACE
	write_backtrace(num);
#endif

	signal(num, SIG_DFL);               //default signal action
	raise(num);
}

static void I_RegisterChildSignals(void)
{
	// If these defines don't exist,
	// then compilation would have failed above us...
	signal(SIGILL , signal_handler_child);
	signal(SIGSEGV , signal_handler_child);
	signal(SIGABRT , signal_handler_child);
	signal(SIGFPE , signal_handler_child);
}
#endif

//
//I_OutputMsg
//
void I_OutputMsg(const char *fmt, ...)
{
	size_t len;
	char txt[8192];
	va_list  argptr;

	va_start(argptr,fmt);
	vsprintf(txt, fmt, argptr);
	va_end(argptr);

#ifdef HAVE_TTF
	if (TTF_WasInit()) I_TTFDrawText(currentfont, solid, DEFAULTFONTFGR, DEFAULTFONTFGG, DEFAULTFONTFGB,  DEFAULTFONTFGA,
	DEFAULTFONTBGR, DEFAULTFONTBGG, DEFAULTFONTBGB, DEFAULTFONTBGA, txt);
#endif

#if defined (_WIN32) && defined (_MSC_VER)
	OutputDebugStringA(txt);
#endif

	len = strlen(txt);

#ifdef LOGMESSAGES
	if (logstream)
	{
		size_t d = fwrite(txt, len, 1, logstream);
		fflush(logstream);
		(void)d;
	}
#endif

#if defined (_WIN32)
#ifdef DEBUGFILE
	if (debugfile != stderr)
#endif
	{
		HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD bytesWritten;

		if (co == INVALID_HANDLE_VALUE)
			return;

		if (GetFileType(co) == FILE_TYPE_CHAR && GetConsoleMode(co, &bytesWritten))
		{
			static COORD coordNextWrite = {0,0};
			LPVOID oldLines = NULL;
			INT oldLength;
			CONSOLE_SCREEN_BUFFER_INFO csbi;

			// Save the lines that we're going to obliterate.
			GetConsoleScreenBufferInfo(co, &csbi);
			oldLength = csbi.dwSize.X * (csbi.dwCursorPosition.Y - coordNextWrite.Y) + csbi.dwCursorPosition.X - coordNextWrite.X;

			if (oldLength > 0)
			{
				LPVOID blank = malloc(oldLength);
				if (!blank) return;
				memset(blank, ' ', oldLength); // Blank out.
				oldLines = malloc(oldLength*sizeof(TCHAR));
				if (!oldLines)
				{
					free(blank);
					return;
				}

				ReadConsoleOutputCharacter(co, (LPSTR)oldLines, oldLength, coordNextWrite, &bytesWritten);

				// Move to where we what to print - which is where we would've been,
				// had console input not been in the way,
				SetConsoleCursorPosition(co, coordNextWrite);

				WriteConsoleA(co, blank, oldLength, &bytesWritten, NULL);
				free(blank);

				// And back to where we want to print again.
				SetConsoleCursorPosition(co, coordNextWrite);
			}

			// Actually write the string now!
			WriteConsoleA(co, txt, (DWORD)len, &bytesWritten, NULL);

			// Next time, output where we left off.
			GetConsoleScreenBufferInfo(co, &csbi);
			coordNextWrite = csbi.dwCursorPosition;

			// Restore what was overwritten.
			if (oldLines && entering_con_command)
				WriteConsole(co, oldLines, oldLength, &bytesWritten, NULL);
			if (oldLines) free(oldLines);
		}
		else // Redirected to a file.
			WriteFile(co, txt, (DWORD)len, &bytesWritten, NULL);
	}
#else
#ifdef HAVE_TERMIOS
	if (consolevent)
	{
		tty_Hide();
	}
#endif

	if (!framebuffer)
		fprintf(stderr, "%s", txt);
#ifdef HAVE_TERMIOS
	if (consolevent)
	{
		tty_Show();
	}
#endif

	// 2004-03-03 AJR Since not all messages end in newline, some were getting displayed late.
	if (!framebuffer)
		fflush(stderr);

#endif
}

//
// I_GetKey
//
INT32 I_GetKey (void)
{
	// Warning: I_GetKey empties the event queue till next keypress
	event_t *ev;
	INT32 rc = 0;

	G_ResetAllDeviceResponding();

	// return the first keypress from the event queue
	for (; eventtail != eventhead; eventtail = (eventtail+1)&(MAXEVENTS-1))
	{
		ev = &events[eventtail];

		HandleGamepadDeviceEvents(ev);

		if (ev->type == ev_keydown || ev->type == ev_console)
		{
			rc = ev->data1;
			continue;
		}
	}

	return rc;
}

void
I_CursedWindowMovement (int xd, int yd)
{
	SDL_SetWindowPosition(window, window_x + xd, window_y + yd);
}

boolean I_HasOpenURL()
{
	#if (SDL_VERSION_ATLEAST(2, 0, 14))
		return true;
	#else
		return false;
	#endif
}

void I_OpenURL(const char *data)
{
	#if (SDL_VERSION_ATLEAST(2, 0, 14))
		SDL_OpenURL(data);
	#else
		(void)data;
		return;
	#endif
}

//
// I_JoyScale
//
void I_JoyScale(void)
{
	Joystick[0].bGamepadStyle = cv_joyscale[0].value==0;
	JoyInfo[0].scale = Joystick[0].bGamepadStyle?1:cv_joyscale[0].value;
}

void I_JoyScale2(void)
{
	Joystick[1].bGamepadStyle = cv_joyscale[1].value==0;
	JoyInfo[1].scale = Joystick[1].bGamepadStyle?1:cv_joyscale[1].value;
}

void I_JoyScale3(void)
{
	Joystick[2].bGamepadStyle = cv_joyscale[2].value==0;
	JoyInfo[2].scale = Joystick[2].bGamepadStyle?1:cv_joyscale[2].value;
}

void I_JoyScale4(void)
{
	Joystick[3].bGamepadStyle = cv_joyscale[3].value==0;
	JoyInfo[3].scale = Joystick[3].bGamepadStyle?1:cv_joyscale[1].value;
}

void I_SetGamepadPlayerIndex(INT32 device_id, INT32 player)
{
#if !(SDL_VERSION_ATLEAST(2,0,12))
	(void)device_id;
	(void)player;
#else
	I_Assert(device_id > 0); // Gamepad devices are always ID 1 or higher
	I_Assert(player >= 0 && player < MAXSPLITSCREENPLAYERS);

	SDL_GameController *controller = SDL_GameControllerFromInstanceID(device_id - 1);
	if (controller == NULL)
	{
		return;
	}

	SDL_GameControllerSetPlayerIndex(controller, player);
#endif
}

void I_SetGamepadIndicatorColor(INT32 device_id, UINT8 red, UINT8 green, UINT8 blue)
{
#if !(SDL_VERSION_ATLEAST(2,0,14))
	(void)device_id;
	(void)red;
	(void)green;
	(void)blue;
#else
	I_Assert(device_id > 0); // Gamepad devices are always ID 1 or higher

	SDL_GameController *controller = SDL_GameControllerFromInstanceID(device_id - 1);
	if (controller == NULL)
	{
		return;
	}

	SDL_GameControllerSetLED(controller, red, green, blue);
#endif
}

void I_GetGamepadGuid(INT32 device_id, char *out, int out_len)
{
	SDL_GameController *controller;
	SDL_Joystick *joystick;
	SDL_JoystickGUID guid;

	I_Assert(device_id > 0);
	I_Assert(out != NULL);
	I_Assert(out_len > 0);

	if (out_len < 33)
	{
		out[0] = 0;
		return;
	}

	controller = SDL_GameControllerFromInstanceID(device_id - 1);
	if (controller == NULL)
	{
		out[0] = 0;
		return;
	}
	joystick = SDL_GameControllerGetJoystick(controller);
	if (joystick == NULL)
	{
		out[0] = 0;
		return;
	}

	guid = SDL_JoystickGetGUID(joystick);
	SDL_JoystickGetGUIDString(guid, out, out_len);
}

void I_GetGamepadName(INT32 device_id, char *out, int out_len)
{
	SDL_GameController *controller;
	const char *name;
	int name_len;

	I_Assert(device_id > 0);
	I_Assert(out != NULL);
	I_Assert(out_len > 0);

	controller = SDL_GameControllerFromInstanceID(device_id - 1);
	if (controller == NULL)
	{
		out[0] = 0;
		return;
	}

	name = SDL_GameControllerName(controller);
	name_len = strlen(name) + 1;
	memcpy(out, name, out_len < name_len ? out_len : name_len);
	out[out_len - 1] = 0;
}

void I_GamepadRumble(INT32 device_id, UINT16 low_strength, UINT16 high_strength)
{
#if !(SDL_VERSION_ATLEAST(2,0,9))
	(void)device_id;
	(void)low_strength;
	(void)high_strength;
#else
	I_Assert(device_id > 0); // Gamepad devices are always ID 1 or higher

	SDL_GameController *controller = SDL_GameControllerFromInstanceID(device_id - 1);
	if (controller == NULL)
	{
		return;
	}

	SDL_GameControllerRumble(controller, low_strength, high_strength, 0);
#endif
}

void I_GamepadRumbleTriggers(INT32 device_id, UINT16 left_strength, UINT16 right_strength)
{
#if !(SDL_VERSION_ATLEAST(2,0,14))
	(void)device_id;
	(void)left_strength;
	(void)right_strength;
#else
	I_Assert(device_id > 0); // Gamepad devices are always ID 1 or higher

	SDL_GameController *controller = SDL_GameControllerFromInstanceID(device_id - 1);
	if (controller == NULL)
	{
		return;
	}

	SDL_GameControllerRumbleTriggers(controller, left_strength, right_strength, 0);
#endif
}

//
// I_StartupInput
//
void I_StartupInput(void)
{
	if (M_CheckParm("-nojoy"))
		return;

	{
		char dbpath[1024];
		sprintf(dbpath, "%s" PATHSEP "gamecontrollerdb.txt", srb2path);
		SDL_GameControllerAddMappingsFromFile(dbpath);
	}

	{
		char dbpath[1024];
		sprintf(dbpath, "%s" PATHSEP "gamecontrollerdb_user.txt", srb2home);
		SDL_GameControllerAddMappingsFromFile(dbpath);
	}

	if (SDL_WasInit(SDL_INIT_GAMECONTROLLER))
	{
		return;
	}

	if (M_CheckParm("-noxinput"))
		SDL_SetHintWithPriority("SDL_XINPUT_ENABLED", "0", SDL_HINT_OVERRIDE);

	if (M_CheckParm("-nohidapi"))
		SDL_SetHintWithPriority("SDL_JOYSTICK_HIDAPI", "0", SDL_HINT_OVERRIDE);

	CONS_Printf("I_StartupInput()...\n");

	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == -1)
	{
		CONS_Printf(M_GetText("Couldn't initialize game controllers: %s\n"), SDL_GetError());
		return;
	}

	// Upon initialization, the gamecontroller subsystem will automatically dispatch controller device added events
	// for controllers connected before initialization.
}

static void I_ShutdownInput(void)
{
	// The game code is now responsible for resetting its internal state based on ev_gamepad_device_removed events.
	// In practice, Input should never be shutdown and restarted during runtime.

	if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == SDL_INIT_GAMECONTROLLER)
	{
		CONS_Printf("Shutting down gamecontroller system\n");
		SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
		I_OutputMsg("I_Joystick: SDL's Game Controller system has been shutdown\n");
	}

	if (SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
	{
		CONS_Printf("Shutting down joy system\n");
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		I_OutputMsg("I_Joystick: SDL's Joystick system has been shutdown\n");
	}
}

INT32 I_NumJoys(void)
{
	INT32 numjoy = 0;
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
		numjoy = SDL_NumJoysticks();
	return numjoy;
}

static char joyname[255]; // joystick name is straight from the driver

const char *I_GetJoyName(INT32 joyindex)
{
	const char *tempname = NULL;
	SDL_Joystick* joystick;
	joyname[0] = 0;
	joyindex--; //SDL's Joystick System starts at 0, not 1

	if (SDL_WasInit(SDL_INIT_JOYSTICK) != SDL_INIT_JOYSTICK)
	{
		return joyname;
	}

	// joyindex corresponds to the open joystick *instance* ID, not the joystick number
	joystick = SDL_JoystickFromInstanceID(joyindex);
	if (joystick == NULL)
	{
		return joyname;
	}

	tempname = SDL_JoystickNameForIndex(joyindex);
	if (tempname)
	{
		strncpy(joyname, tempname, 254);
		joyname[254] = 0;
	}

	return joyname;
}

#ifndef NOMUMBLE
#ifdef HAVE_MUMBLE
// Best Mumble positional audio settings:
// Minimum distance 3.0 m
// Bloom 175%
// Maximum distance 80.0 m
// Minimum volume 50%
#define DEG2RAD (0.017453292519943295769236907684883l) // TAU/360 or PI/180
#define MUMBLEUNIT (64.0f) // FRACUNITS in a Meter

static struct mumble_s {
#ifdef WINMUMBLE
	UINT32 uiVersion;
	DWORD uiTick;
#else
	Uint32 uiVersion;
	Uint32 uiTick;
#endif
	float fAvatarPosition[3];
	float fAvatarFront[3];
	float fAvatarTop[3]; // defaults to Y-is-up (only used for leaning)
	wchar_t name[256]; // game name
	float fCameraPosition[3];
	float fCameraFront[3];
	float fCameraTop[3]; // defaults to Y-is-up (only used for leaning)
	wchar_t identity[256]; // player id
#ifdef WINMUMBLE
	UINT32 context_len;
#else
	Uint32 context_len;
#endif
	unsigned char context[256]; // server/team
	wchar_t description[2048]; // game description
} *mumble = NULL;
#endif // HAVE_MUMBLE

static void I_SetupMumble(void)
{
#ifdef WINMUMBLE
	HANDLE hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MumbleLink");
	if (!hMap)
		return;

	mumble = static_cast<mumble_s*>(MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*mumble)));
	if (!mumble)
		CloseHandle(hMap);
#elif defined (HAVE_SHM)
	int shmfd;
	char memname[256];

	snprintf(memname, 256, "/MumbleLink.%d", getuid());
	shmfd = shm_open(memname, O_RDWR, S_IRUSR | S_IWUSR);

	if(shmfd < 0)
		return;

	mumble = mmap(NULL, sizeof(*mumble), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (mumble == MAP_FAILED)
		mumble = NULL;
#endif
}

void I_UpdateMumble(const mobj_t *mobj, const listener_t listener)
{
#ifdef HAVE_MUMBLE
	double angle;
	fixed_t anglef;

	if (!mumble)
		return;

	if(mumble->uiVersion != 2) {
		wcsncpy(mumble->name, L"Dr. Robotnik's Ring Racers "VERSIONSTRINGW, 256);
		wcsncpy(mumble->description, L"Dr. Robotnik's Ring Racers with integrated Mumble Link support.", 2048);
		mumble->uiVersion = 2;
	}
	mumble->uiTick++;

	if (!netgame || gamestate != GS_LEVEL) { // Zero out, but never delink.
		mumble->fAvatarPosition[0] = mumble->fAvatarPosition[1] = mumble->fAvatarPosition[2] = 0.0f;
		mumble->fAvatarFront[0] = 1.0f;
		mumble->fAvatarFront[1] = mumble->fAvatarFront[2] = 0.0f;
		mumble->fCameraPosition[0] = mumble->fCameraPosition[1] = mumble->fCameraPosition[2] = 0.0f;
		mumble->fCameraFront[0] = 1.0f;
		mumble->fCameraFront[1] = mumble->fCameraFront[2] = 0.0f;
		return;
	}

	{
		UINT8 *p = mumble->context;
		WRITEMEM(p, server_context, 8);
		WRITEINT16(p, gamemap);
		mumble->context_len = (UINT32)(p - mumble->context);
	}

	if (mobj) {
		mumble->fAvatarPosition[0] = FIXED_TO_FLOAT(mobj->x) / MUMBLEUNIT;
		mumble->fAvatarPosition[1] = FIXED_TO_FLOAT(mobj->z) / MUMBLEUNIT;
		mumble->fAvatarPosition[2] = FIXED_TO_FLOAT(mobj->y) / MUMBLEUNIT;

		anglef = AngleFixed(mobj->angle);
		angle = FIXED_TO_FLOAT(anglef)*DEG2RAD;
		mumble->fAvatarFront[0] = (float)cos(angle);
		mumble->fAvatarFront[1] = 0.0f;
		mumble->fAvatarFront[2] = (float)sin(angle);
	} else {
		mumble->fAvatarPosition[0] = mumble->fAvatarPosition[1] = mumble->fAvatarPosition[2] = 0.0f;
		mumble->fAvatarFront[0] = 1.0f;
		mumble->fAvatarFront[1] = mumble->fAvatarFront[2] = 0.0f;
	}

	mumble->fCameraPosition[0] = FIXED_TO_FLOAT(listener.x) / MUMBLEUNIT;
	mumble->fCameraPosition[1] = FIXED_TO_FLOAT(listener.z) / MUMBLEUNIT;
	mumble->fCameraPosition[2] = FIXED_TO_FLOAT(listener.y) / MUMBLEUNIT;

	anglef = AngleFixed(listener.angle);
	angle = FIXED_TO_FLOAT(anglef)*DEG2RAD;
	mumble->fCameraFront[0] = (float)cos(angle);
	mumble->fCameraFront[1] = 0.0f;
	mumble->fCameraFront[2] = (float)sin(angle);
#else
	(void)mobj;
	(void)listener;
#endif // HAVE_MUMBLE
}
#undef WINMUMBLE
#endif // NOMUMBLE

//
// I_Tactile
//
void I_Tactile(FFType pFFType, const JoyFF_t *FFEffect)
{
	// UNUSED.
	(void)pFFType;
	(void)FFEffect;
}

void I_Tactile2(FFType pFFType, const JoyFF_t *FFEffect)
{
	// UNUSED.
	(void)pFFType;
	(void)FFEffect;
}

void I_Tactile3(FFType pFFType, const JoyFF_t *FFEffect)
{
	// UNUSED.
	(void)pFFType;
	(void)FFEffect;
}

void I_Tactile4(FFType pFFType, const JoyFF_t *FFEffect)
{
	// UNUSED.
	(void)pFFType;
	(void)FFEffect;
}

//
// I_GetTime
// returns time in 1/TICRATE second tics
//

static Uint64 timer_frequency;

precise_t I_GetPreciseTime(void)
{
	return SDL_GetPerformanceCounter();
}

UINT64 I_GetPrecisePrecision(void)
{
	return SDL_GetPerformanceFrequency();
}

static UINT32 frame_rate;

static double frame_frequency;
static UINT64 frame_epoch;
static double elapsed_frames;

static void I_InitFrameTime(const UINT64 now, const UINT32 cap)
{
	frame_rate = cap;
	frame_epoch = now;

	//elapsed_frames = 0.0;

	if (frame_rate == 0)
	{
		// Shouldn't be used, but just in case...?
		frame_frequency = 1.0;
		return;
	}

	frame_frequency = timer_frequency / (double)frame_rate;
}

double I_GetFrameTime(void)
{
	const UINT64 now = SDL_GetPerformanceCounter();
	const UINT32 cap = R_GetFramerateCap();

	if (cap != frame_rate)
	{
		// Maybe do this in a OnChange function for cv_fpscap?
		I_InitFrameTime(now, cap);
	}

	if (frame_rate == 0)
	{
		// Always advance a frame.
		elapsed_frames += 1.0;
	}
	else
	{
		elapsed_frames += (now - frame_epoch) / frame_frequency;
	}

	frame_epoch = now; // moving epoch
	return elapsed_frames;
}

//
// I_StartupTimer
//
void I_StartupTimer(void)
{
	timer_frequency = SDL_GetPerformanceFrequency();

	I_InitFrameTime(0, R_GetFramerateCap());
	elapsed_frames  = 0.0;
}

void I_Sleep(UINT32 ms)
{
	SDL_Delay(ms);
}

#ifdef NEWSIGNALHANDLER
static void newsignalhandler_Warn(const char *pr)
{
	char text[128];

	snprintf(text, sizeof text,
			"Error while setting up signal reporting: %s: %s",
			pr,
			strerror(errno)
	);

	I_OutputMsg("%s\n", text);

	if (!M_CheckParm("-dedicated"))
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"Startup error",
			text, NULL);

	I_ShutdownConsole();
	exit(-1);
}

static void I_Fork(void)
{
	int child;
	int status;
	int signum;
	int c;

	child = fork();

	switch (child)
	{
		case -1:
			newsignalhandler_Warn("fork()");
			break;
		case 0:
			I_RegisterChildSignals();
			break;
		default:
			if (logstream)
				fclose(logstream);/* the child has this */

			c = wait(&status);

#ifdef LOGMESSAGES
			/* By the way, exit closes files. */
			logstream = fopen(logfilename, "at");
#else
			logstream = 0;
#endif

			if (c == -1)
			{
				kill(child, SIGKILL);
				newsignalhandler_Warn("wait()");
			}
			else
			{
				if (WIFSIGNALED (status))
				{
					signum = WTERMSIG (status);
#ifdef WCOREDUMP
					I_ReportSignal(signum, WCOREDUMP (status));
#else
					I_ReportSignal(signum, 0);
#endif
					status = 128 + signum;
				}
				else if (WIFEXITED (status))
				{
					status = WEXITSTATUS (status);
				}

				I_ShutdownConsole();
				exit(status);
			}
	}
}
#endif/*NEWSIGNALHANDLER*/

INT32 I_StartupSystem(void)
{
	SDL_version SDLcompiled;
	SDL_version SDLlinked;
	SDL_VERSION(&SDLcompiled)
	SDL_GetVersion(&SDLlinked);
	I_StartupConsole();
#ifdef NEWSIGNALHANDLER
	// This is useful when debugging. It lets GDB attach to
	// the correct process easily.
	if (!M_CheckParm("-nofork"))
		I_Fork();
#endif
#ifdef HAVE_THREADS
	I_start_threads();
	I_AddExitFunc(I_stop_threads);
	I_ThreadPoolInit();
	I_AddExitFunc(I_ThreadPoolShutdown);
#endif
	I_RegisterSignals();
	I_OutputMsg("Compiled for SDL version: %d.%d.%d\n",
	 SDLcompiled.major, SDLcompiled.minor, SDLcompiled.patch);
	I_OutputMsg("Linked with SDL version: %d.%d.%d\n",
	 SDLlinked.major, SDLlinked.minor, SDLlinked.patch);
	if (SDL_Init(0) < 0)
		I_Error("Dr. Robotnik's Ring Racers: SDL System Error: %s", SDL_GetError()); //Alam: Oh no....
#ifndef NOMUMBLE
	I_SetupMumble();
#endif
	return 0;
}

//
// I_Quit
//
void I_Quit(void)
{
	static SDL_bool quiting = SDL_FALSE;

	/* prevent recursive I_Quit() */
	if (quiting) goto death;
	SDLforceUngrabMouse();
	quiting = SDL_FALSE;
	M_SaveConfig(NULL); //save game config, cvars..
	M_SaveJoinedIPs();

	// Make sure you lose points for ALT-F4
	if (Playing())
		K_PlayerForfeit(consoleplayer, true);

	G_SaveGameData(); // Tails 12-08-2002
	//added:16-02-98: when recording a demo, should exit using 'q' key,
	//        but sometimes we forget and use 'F10'.. so save here too.

	if (demo.recording)
		G_CheckDemoStatus();

#ifdef DEVELOP
	// Join up with thread if waiting
	R_PrintTextureDuplicates();
#endif

	D_QuitNetGame();
	CL_AbortDownloadResume();
	I_ShutdownMusic();
	I_ShutdownSound();
	// use this for 1.28 19990220 by Kin
	I_ShutdownGraphics();
	I_ShutdownInput();
	I_ShutdownSystem();
	SDL_Quit();
	/* if option -noendtxt is set, don't print the text */
	if (!M_CheckParm("-noendtxt") && W_CheckNumForName("ENDOOM") != LUMPERROR)
	{
		printf("\r");
		ShowEndTxt();
	}
	if (myargmalloc)
		free(myargv); // Deallocate allocated memory
death:
	W_Shutdown();
	exit(0);
}

void I_WaitVBL(INT32 count)
{
	count = 1;
	SDL_Delay(count);
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

//
// I_Error
//
/**	\brief phuck recursive errors
*/
static INT32 errorcount = 0;

/**	\brief recursive error detecting
*/
static boolean shutdowning = false;

void I_Error(const char *error, ...)
{
	va_list argptr;
	char buffer[8192];

	if (std::this_thread::get_id() != g_main_thread_id)
	{
		// Do not attempt a graceful shutdown. Errors off the main thread are unresolvable.
		exit(-2);
	}

	// recursive error detecting
	if (shutdowning)
	{
		errorcount++;
		if (errorcount == 1)
			SDLforceUngrabMouse();
		// try to shutdown each subsystem separately
		if (errorcount == 2)
			I_ShutdownMusic();
		if (errorcount == 3)
			I_ShutdownSound();
		if (errorcount == 4)
			I_ShutdownGraphics();
		if (errorcount == 5)
			I_ShutdownInput();
		if (errorcount == 6)
			I_ShutdownSystem();
		if (errorcount == 7)
			SDL_Quit();
		if (errorcount == 8)
			G_DirtyGameData();
		if (errorcount > 20)
		{
			va_start(argptr, error);
			vsprintf(buffer, error, argptr);
			va_end(argptr);
			// Implement message box with SDL_ShowSimpleMessageBox,
			// which should fail gracefully if it can't put a message box up
			// on the target system
			if (!M_CheckParm("-dedicated"))
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
					"Dr. Robotnik's Ring Racers " VERSIONSTRING " Recursive Error",
					buffer, NULL);

			W_Shutdown();
			exit(-1); // recursive errors detected
		}
	}
	else
	{
		// This makes crashes funnier by stimulating the funnicampus of the brain
		S_StopSounds();
		S_StartSound(NULL, sfx_etexpl);
	}

	shutdowning = true;

	// Display error message in the console before we start shutting it down
	va_start(argptr, error);
	vsprintf(buffer, error, argptr);
	va_end(argptr);
	I_OutputMsg("\nI_Error(): %s\n", buffer);
	// ---

	// FUCK OFF, stop allocating memory to write entire gamedata & configs
	// when the program needs to shut down ASAP and we already save
	// these all the time! Just set the dirty bit and GET OUT!
	G_DirtyGameData();

	// Shutdown. Here might be other errors.

	/* Prevent segmentation fault if testers go to Record Attack... */
#ifndef TESTERS
	if (demo.recording)
		G_CheckDemoStatus();
#endif

	D_QuitNetGame();
	CL_AbortDownloadResume();

	I_ShutdownMusic();
	I_ShutdownGraphics();
	I_ShutdownInput();

	I_ShowErrorMessageBox(buffer, false);

	// We wait until now to do this so the funny sound can be heard
	I_ShutdownSound();
	// use this for 1.28 19990220 by Kin
	I_ShutdownSystem();
	SDL_Quit();

	W_Shutdown();

#if defined (PARANOIA) || defined (DEVELOP)
	*(volatile INT32 *)0 = 4; //Alam: Debug!
#endif

	exit(-1);
}

/**	\brief quit function table
*/
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS]; /* initialized to all bits 0 */

//
//  Adds a function to the list that need to be called by I_SystemShutdown().
//
void I_AddExitFunc(void (*func)())
{
	INT32 c;

	for (c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if (!quit_funcs[c])
		{
			quit_funcs[c] = func;
			break;
		}
	}
}


//
//  Removes a function from the list that need to be called by
//   I_SystemShutdown().
//
void I_RemoveExitFunc(void (*func)())
{
	INT32 c;

	for (c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if (quit_funcs[c] == func)
		{
			while (c < MAX_QUIT_FUNCS-1)
			{
				quit_funcs[c] = quit_funcs[c+1];
				c++;
			}
			quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
			break;
		}
	}
}

#if !(defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON))
static void Shittycopyerror(const char *name)
{
	I_OutputMsg(
			"Error copying log file: %s: %s\n",
			name,
			strerror(errno)
	);
}

static void Shittylogcopy(void)
{
	char buf[8192];
	FILE *fp;
	size_t r;
	if (fseek(logstream, 0, SEEK_SET) == -1)
	{
		Shittycopyerror("fseek");
	}
	else if (( fp = fopen(logfilename, "wt") ))
	{
		while (( r = fread(buf, 1, sizeof buf, logstream) ))
		{
			if (fwrite(buf, 1, r, fp) < r)
			{
				Shittycopyerror("fwrite");
				break;
			}
		}
		if (ferror(logstream))
		{
			Shittycopyerror("fread");
		}
		fclose(fp);
	}
	else
	{
		Shittycopyerror(logfilename);
	}
}
#endif/*!(defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON))*/

//
//  Closes down everything. This includes restoring the initial
//  palette and video mode, and removing whatever mouse, keyboard, and
//  timer routines have been installed.
//
//  NOTE: Shutdown user funcs are effectively called in reverse order.
//
void I_ShutdownSystem(void)
{
	INT32 c;

#ifdef NEWSIGNALHANDLER
	if (M_CheckParm("-nofork"))
#endif
		I_ShutdownConsole();

	for (c = MAX_QUIT_FUNCS-1; c >= 0; c--)
		if (quit_funcs[c])
			(*quit_funcs[c])();
#ifdef LOGMESSAGES
	if (logstream)
	{
		I_OutputMsg("I_ShutdownSystem(): end of logstream.\n");
#if !(defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON))
		Shittylogcopy();
#endif
		fclose(logstream);
		logstream = NULL;
	}
#endif

}

void I_GetDiskFreeSpace(INT64 *freespace)
{
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#if defined (SOLARIS) || defined (__HAIKU__)
	*freespace = INT32_MAX;
	return;
#else // Both Linux and BSD have this, apparently.
	struct statfs stfs;
	if (statfs(srb2home, &stfs) == -1)
	{
		*freespace = INT32_MAX;
		return;
	}
	*freespace = stfs.f_bavail * stfs.f_bsize;
#endif
#elif defined (_WIN32)
	static p_GetDiskFreeSpaceExA pfnGetDiskFreeSpaceEx = NULL;
	static boolean testwin95 = false;
	ULARGE_INTEGER usedbytes, lfreespace;

	if (!testwin95)
	{
		*(void**)&pfnGetDiskFreeSpaceEx = FUNCPTRCAST(GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetDiskFreeSpaceExA"));
		testwin95 = true;
	}
	if (pfnGetDiskFreeSpaceEx)
	{
		if (pfnGetDiskFreeSpaceEx(srb2home, &lfreespace, &usedbytes, NULL))
			*freespace = lfreespace.QuadPart;
		else
			*freespace = INT32_MAX;
	}
	else
	{
		DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
		GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector,
						 &NumberOfFreeClusters, &TotalNumberOfClusters);
		*freespace = BytesPerSector*SectorsPerCluster*NumberOfFreeClusters;
	}
#else // Dummy for platform independent; 1GB should be enough
	*freespace = 1024*1024*1024;
#endif
}

char *I_GetUserName(void)
{
	static char username[MAXPLAYERNAME+1];
	char *p;
#ifdef _WIN32
	DWORD i = MAXPLAYERNAME;

	if (!GetUserNameA(username, &i))
#endif
	{
		p = I_GetEnv("USER");
		if (!p)
		{
			p = I_GetEnv("user");
			if (!p)
			{
				p = I_GetEnv("USERNAME");
				if (!p)
				{
					p = I_GetEnv("username");
					if (!p)
					{
						return NULL;
					}
				}
			}
		}
		strncpy(username, p, MAXPLAYERNAME);
	}


	if (strcmp(username, "") != 0)
		return username;
	return NULL; // dummy for platform independent version
}

INT32 I_mkdir(const char *dirname, INT32 unixright)
{
//[segabor]
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON) || defined (__CYGWIN__)
	return mkdir(dirname, unixright);
#elif defined (_WIN32)
	UNREFERENCED_PARAMETER(unixright); /// \todo should implement ntright under nt...
	return CreateDirectoryA(dirname, NULL);
#else
	(void)dirname;
	(void)unixright;
	return false;
#endif
}

char *I_GetEnv(const char *name)
{
#ifdef NEED_SDL_GETENV
	return SDL_getenv(name);
#else
	return getenv(name);
#endif
}

INT32 I_PutEnv(char *variable)
{
#ifdef NEED_SDL_GETENV
	return SDL_putenv(variable);
#else
	return putenv(variable);
#endif
}

INT32 I_ClipboardCopy(const char *data, size_t size)
{
	char storage[256];
	if (size > 255)
		size = 255;
	memcpy(storage, data, size);
	storage[size] = 0;

	if (SDL_SetClipboardText(storage))
		return 0;
	return -1;
}

const char *I_ClipboardPaste(void)
{
	static char clipboard_modified[256];
	char *clipboard_contents, *i = clipboard_modified;

	if (!SDL_HasClipboardText())
		return NULL;

	clipboard_contents = SDL_GetClipboardText();
	strlcpy(clipboard_modified, clipboard_contents, 256);
	SDL_free(clipboard_contents);

	while (*i)
	{
		if (*i == '\n' || *i == '\r')
		{ // End on newline
			*i = 0;
			break;
		}
		else if (*i == '\t')
			*i = ' '; // Tabs become spaces
		else if (*i < 32 || (unsigned)*i > 127)
			*i = '?'; // Nonprintable chars become question marks
		++i;
	}
	return (const char *)&clipboard_modified;
}

/**	\brief	The isWadPathOk function

	\param	path	string path to check

	\return if true, wad file found


*/
static boolean isWadPathOk(const char *path)
{
	char *wad3path = static_cast<char*>(malloc(256));

	if (!wad3path)
		return false;

	sprintf(wad3path, pandf, path, WADKEYWORD);

	if (FIL_ReadFileOK(wad3path))
	{
		free(wad3path);
		return true;
	}

	free(wad3path);
	return false;
}

static void pathonly(char *s)
{
	size_t j;

	for (j = strlen(s); j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
		{
			if (s[j] == ':') s[j+1] = 0;
			else s[j] = 0;
			return;
		}
}

/**	\brief	search for bios.pk3 in the given path

	\param	searchDir	starting path

	\return	WAD path if not NULL


*/
static const char *searchWad(const char *searchDir)
{
	static char tempsw[256] = "";
	filestatus_t fstemp;

	strcpy(tempsw, WADKEYWORD);
	fstemp = filesearch(tempsw,searchDir,NULL,true,20);
	if (fstemp == FS_FOUND)
	{
		pathonly(tempsw);
		return tempsw;
	}

	return NULL;
}

/**	\brief go through all possible paths and look for bios.pk3

  \return path to bios.pk3 if any

*/
static const char *locateWad(void)
{
	const char *envstr;
	const char *WadPath;

	I_OutputMsg("RINGRACERSWADDIR");
	// does RINGRACERSWADDIR exist?
	if (((envstr = I_GetEnv("RINGRACERSWADDIR")) != NULL) && isWadPathOk(envstr))
		return envstr;

#ifndef NOCWD
	I_OutputMsg(",.");
	// examine current dir
	strcpy(returnWadPath, ".");
	if (isWadPathOk(returnWadPath))
		return NULL;
#endif


#ifdef DEFAULTDIR
	I_OutputMsg(",HOME/" DEFAULTDIR);
	// examine user jart directory
	if ((envstr = I_GetEnv("HOME")) != NULL)
	{
		sprintf(returnWadPath, "%s" PATHSEP DEFAULTDIR, envstr);
		if (isWadPathOk(returnWadPath))
			return returnWadPath;
	}
#endif

#ifdef __APPLE__
	OSX_GetResourcesPath(returnWadPath);
	I_OutputMsg(",%s", returnWadPath);
	if (isWadPathOk(returnWadPath))
	{
		return returnWadPath;
	}
#endif

	// examine default dirs
#ifdef DEFAULTWADLOCATION1
	I_OutputMsg("," DEFAULTWADLOCATION1);
	strcpy(returnWadPath, DEFAULTWADLOCATION1);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION2
	I_OutputMsg("," DEFAULTWADLOCATION2);
	strcpy(returnWadPath, DEFAULTWADLOCATION2);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION3
	I_OutputMsg("," DEFAULTWADLOCATION3);
	strcpy(returnWadPath, DEFAULTWADLOCATION3);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION4
	I_OutputMsg("," DEFAULTWADLOCATION4);
	strcpy(returnWadPath, DEFAULTWADLOCATION4);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION5
	I_OutputMsg("," DEFAULTWADLOCATION5);
	strcpy(returnWadPath, DEFAULTWADLOCATION5);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION6
	I_OutputMsg("," DEFAULTWADLOCATION6);
	strcpy(returnWadPath, DEFAULTWADLOCATION6);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION7
	I_OutputMsg("," DEFAULTWADLOCATION7);
	strcpy(returnWadPath, DEFAULTWADLOCATION7);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifndef NOHOME
	// find in $HOME
	I_OutputMsg(",HOME");
	if ((envstr = I_GetEnv("HOME")) != NULL)
	{
		WadPath = searchWad(envstr);
		if (WadPath)
			return WadPath;
	}
#endif
#ifdef DEFAULTSEARCHPATH1
	// find in /usr/local
	I_OutputMsg(", in:" DEFAULTSEARCHPATH1);
	WadPath = searchWad(DEFAULTSEARCHPATH1);
	if (WadPath)
		return WadPath;
#endif
#ifdef DEFAULTSEARCHPATH2
	// find in /usr/games
	I_OutputMsg(", in:" DEFAULTSEARCHPATH2);
	WadPath = searchWad(DEFAULTSEARCHPATH2);
	if (WadPath)
		return WadPath;
#endif
#ifdef DEFAULTSEARCHPATH3
	// find in ???
	I_OutputMsg(", in:" DEFAULTSEARCHPATH3);
	WadPath = searchWad(DEFAULTSEARCHPATH3);
	if (WadPath)
		return WadPath;
#endif
	// if nothing was found
	return NULL;
}

const char *I_LocateWad(void)
{
	const char *waddir;

	I_OutputMsg("Looking for WADs in: ");
	waddir = locateWad();
	I_OutputMsg("\n");

	if (waddir)
	{
		// change to the directory where we found bios.pk3
#if defined (_WIN32)
		SetCurrentDirectoryA(waddir);
#else
		if (chdir(waddir) == -1)
			I_OutputMsg("Couldn't change working directory\n");
#endif
	}
	return waddir;
}

#ifdef __linux__
#define MEMINFO_FILE "/proc/meminfo"
#define MEMTOTAL "MemTotal:"
#define MEMAVAILABLE "MemAvailable:"
#define MEMFREE "MemFree:"
#define CACHED "Cached:"
#define BUFFERS "Buffers:"
#define SHMEM "Shmem:"

/* Parse the contents of /proc/meminfo (in buf), return value of "name"
 * (example: MemTotal) */
static long get_entry(const char* name, const char* buf)
{
	long val;
	char* hit = strstr(const_cast<char*>(buf), name);
	if (hit == NULL) {
		return -1;
	}

	errno = 0;
	val = strtol(hit + strlen(name), NULL, 10);
	if (errno != 0) {
		CONS_Alert(CONS_ERROR, M_GetText("get_entry: strtol() failed: %s\n"), strerror(errno));
		return -1;
	}
	return val;
}
#endif

// quick fix for compil
UINT32 I_GetFreeMem(UINT32 *total)
{
#ifdef FREEBSD
	struct vmmeter sum;
	kvm_t *kd;
	struct nlist namelist[] =
	{
#define X_SUM   0
		{"_cnt"},
		{NULL}
	};
	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open")) == NULL)
	{
		if (total)
			*total = 0L;
		return 0;
	}
	if (kvm_nlist(kd, namelist) != 0)
	{
		kvm_close (kd);
		if (total)
			*total = 0L;
		return 0;
	}
	if (kvm_read(kd, namelist[X_SUM].n_value, &sum,
		sizeof (sum)) != sizeof (sum))
	{
		kvm_close(kd);
		if (total)
			*total = 0L;
		return 0;
	}
	kvm_close(kd);

	if (total)
		*total = sum.v_page_count * sum.v_page_size;
	return sum.v_free_count * sum.v_page_size;
#elif defined (SOLARIS)
	/* Just guess */
	if (total)
		*total = 32 << 20;
	return 32 << 20;
#elif defined (_WIN32)
	MEMORYSTATUS info;

	info.dwLength = sizeof (MEMORYSTATUS);
	GlobalMemoryStatus( &info );
	if (total)
		*total = (UINT32)info.dwTotalPhys;
	return (UINT32)info.dwAvailPhys;
#elif defined (__linux__)
	/* Linux */
	char buf[1024];
	char *memTag;
	UINT32 freeKBytes;
	UINT32 totalKBytes;
	INT32 n;
	INT32 meminfo_fd = -1;
	long Cached;
	long MemFree;
	long Buffers;
	long Shmem;
	long MemAvailable = -1;

	meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
	n = read(meminfo_fd, buf, 1023);
	close(meminfo_fd);

	if (n < 0)
	{
		// Error
		if (total)
			*total = 0L;
		return 0;
	}

	buf[n] = '\0';
	if ((memTag = strstr(buf, MEMTOTAL)) == NULL)
	{
		// Error
		if (total)
			*total = 0L;
		return 0;
	}

	memTag += sizeof (MEMTOTAL);
	totalKBytes = atoi(memTag);

	if ((memTag = strstr(buf, MEMAVAILABLE)) == NULL)
	{
		Cached = get_entry(CACHED, buf);
		MemFree = get_entry(MEMFREE, buf);
		Buffers = get_entry(BUFFERS, buf);
		Shmem = get_entry(SHMEM, buf);
		MemAvailable = Cached + MemFree + Buffers - Shmem;

		if (MemAvailable == -1)
		{
			// Error
			if (total)
				*total = 0L;
			return 0;
		}
		freeKBytes = MemAvailable;
	}
	else
	{
		memTag += sizeof (MEMAVAILABLE);
		freeKBytes = atoi(memTag);
	}

	if (total)
		*total = totalKBytes << 10;
	return freeKBytes << 10;
#else
	// Guess 48 MB.
	if (total)
		*total = 48<<20;
	return 48<<20;
#endif
}

// note CPUAFFINITY code used to reside here
void I_RegisterSysCommands(void) {}

#endif // HAVE_SDL
