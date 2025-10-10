// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  command.c
/// \brief Parse and execute commands from console input/scripts and remote server
///
///        Handles console variables, which is a simplified version
///        of commands, each consvar can have a function called when
///        it is modified.. thus it acts nearly as commands.
///
///        code shamelessly inspired by the QuakeC sources, thanks Id :)

#include "doomdef.h"
#include "doomstat.h"
#include "command.h"
#include "console.h"
#include "z_zone.h"
#include "k_menu.h"
#include "m_misc.h"
#include "m_fixed.h"
#include "m_argv.h"
#include "byteptr.h"
#include "p_saveg.h"
#include "g_game.h" // for player_names
#include "m_cond.h" // for encore mode
#include "d_netcmd.h"
#include "hu_stuff.h"
#include "p_setup.h"
#include "lua_script.h"
#include "d_netfil.h" // findfile
#include "r_data.h" // Color_cons_t
#include "r_skins.h"
#include "m_random.h"
#include "p_local.h" // P_ResetPlayerCheats
#include "k_color.h"

//========
// protos.
//========
static boolean COM_Exists(const char *com_name);
static void COM_ExecuteString(char *com_text);

static void COM_Alias_f(void);
static void COM_Echo_f(void);
static void COM_CEcho_f(void);
static void COM_CEchoFlags_f(void);
static void COM_CEchoDuration_f(void);
static void COM_Exec_f(void);
static void COM_Wait_f(void);
static void COM_Help_f(void);
static void COM_Toggle_f(void);
static void COM_Add_f(void);
static void COM_Choose_f(void);
static void COM_ChooseWeighted_f(void);
static void COM_Reset_f(void);

static boolean CV_FilterVarByVersion(consvar_t *v, const char *valstr);

static boolean CV_Command(void);
consvar_t *CV_FindVar(const char *name);
static const char *CV_StringValue(const char *var_name);

consvar_t *consvar_vars; // list of registered console variables
static UINT16     consvar_number_of_netids = 0;

static char com_token[1024];
static char *COM_Parse(char *data);

static char * COM_Purge (char *text, int *lenp);

CV_PossibleValue_t CV_OnOff[] = {{0, "Off"}, {1, "On"}, {0, NULL}};
CV_PossibleValue_t CV_YesNo[] = {{0, "No"}, {1, "Yes"}, {0, NULL}};
CV_PossibleValue_t CV_Unsigned[] = {{0, "MIN"}, {999999999, "MAX"}, {0, NULL}};
CV_PossibleValue_t CV_Natural[] = {{1, "MIN"}, {999999999, "MAX"}, {0, NULL}};
CV_PossibleValue_t CV_TrueFalse[] = {{0, "False"}, {1, "True"}, {0, NULL}};

// SRB2kart
CV_PossibleValue_t kartspeed_cons_t[] = {
	{KARTSPEED_AUTO, "Auto Gear"},
	{KARTSPEED_EASY, "Gear 1"},
	{KARTSPEED_NORMAL, "Gear 2"},
	{KARTSPEED_HARD, "Gear 3"},
	{0, NULL}
};
CV_PossibleValue_t dummykartspeed_cons_t[] = {
	{KARTSPEED_EASY, "Gear 1"},
	{KARTSPEED_NORMAL, "Gear 2"},
	{KARTSPEED_HARD, "Gear 3"},
	{0, NULL}
};
CV_PossibleValue_t gpdifficulty_cons_t[] = {
	{KARTSPEED_EASY, "Relaxed"},
	{KARTSPEED_NORMAL, "Intense"},
	{KARTSPEED_HARD, "Vicious"},
	{KARTGP_MASTER, "Master"},
	{0, NULL}
};
CV_PossibleValue_t descriptiveinput_cons_t[] = {
	{0, "\"Emulator\""},
	{1, "Modern"},
	{2, "Modern Flip"},
	{3, "6Bt. (Auto)"},
	{4, "6Bt. (A)"},
	{5, "6Bt. (B)"},
	{6, "6Bt. (C)"},
	{7, "6Bt. (D)"},
	{8, "6Bt. (E)"},
	{9, "6Bt. (F)"},
	{10, "6Bt. (G)"},
	{0, NULL}
};

// Filter consvars by EXECVERSION
// First implementation is 2 (1.0.2), so earlier configs default at 1 (1.0.0)
// Also set CV_HIDDEN during runtime, after config is loaded

static boolean execversion_enabled = false;

// for default joyaxis detection
#if 0
static boolean joyaxis_default[4] = {false,false,false,false};
static INT32 joyaxis_count[4] = {0,0,0,0};
#endif

#define COM_BUF_SIZE (32<<10) // command buffer size
#define MAX_ALIAS_RECURSION 100 // max recursion allowed for aliases

static INT32 com_wait; // one command per frame (for cmd sequences)

// command aliases
//
typedef struct cmdalias_s
{
	struct cmdalias_s *next;
	char *name;
	char *value; // the command string to replace the alias
} cmdalias_t;

static cmdalias_t *com_alias; // aliases list

// =========================================================================
//                            COMMAND BUFFER
// =========================================================================

static vsbuf_t com_text; // variable sized buffer

/** Purges control characters out of some text.
  *
  * \param s The text.
  * \param np Optionally a pointer to fill with the new string length.
  * \return The text.
  * \sa COM_ExecuteString
  */
static char *
COM_Purge (char *s, int *np)
{
	char *t;
	char *p;
	int n;
	n = strlen(s);
	t = s + n + 1;
	p = s;
	while (( p = strchr(p, '\033') ))
	{
		memmove(p, &p[1], t - p - 1);
		n--;
	}
	if (np)
		(*np) = n;
	return s;
}

/** Adds text into the command buffer for later execution.
  *
  * \param ptext The text to add.
  * \sa COM_BufInsertTextEx
  */
void COM_BufAddTextEx(const char *ptext, int flags)
{
	int l;
	char *text;

	text = COM_Purge(Z_StrDup(ptext), &l);

	if (com_text.cursize + 2 + l >= com_text.maxsize)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Command buffer full!\n"));
		return;
	}

	VS_WriteEx(&com_text, text, l, flags);

	Z_Free(text);
}

/** Adds command text and executes it immediately.
  *
  * \param ptext The text to execute. A newline is automatically added.
  * \sa COM_BufAddTextEx
  */
void COM_BufInsertTextEx(const char *ptext, int flags)
{
	const INT32 old_wait = com_wait;

	char *temp = NULL;
	size_t templen;

	// copy off any commands still remaining in the exec buffer
	templen = com_text.cursize;
	if (templen)
	{
		temp = M_Memcpy(ZZ_Alloc(templen), com_text.data, templen);
		VS_Clear(&com_text);
	}

	com_wait = 0;

	// add the entire text of the file (or alias)
	COM_BufAddTextEx(ptext, flags);
	COM_BufExecute(); // do it right away

	com_wait += old_wait;

	// add the copied off data
	if (templen)
	{
		VS_Write(&com_text, temp, templen);
		Z_Free(temp);
	}
}

/** Progress the wait timer and flush waiting console commands when ready.
  */
void
COM_BufTicker(void)
{
	if (com_wait)
	{
		com_wait--;
		return;
	}

	COM_BufExecute();
}

/** Flushes (executes) console commands in the buffer.
  */
void COM_BufExecute(void)
{
	size_t i;
	char *ptext;
	char line[1024] = "";
	INT32 quotes;

	while (com_text.cursize)
	{
		// find a '\n' or; line break
		ptext = (char *)com_text.data;

		quotes = 0;
		for (i = 0; i < com_text.cursize; i++)
		{
			if (ptext[i] == '\"' && !quotes && i > 0 && ptext[i-1] != ' ') // Malformed command
				break;
			if (ptext[i] == '\"')
				quotes++;
			if (!(quotes & 1) && ptext[i] == ';')
				break; // don't break if inside a quoted string
			if (ptext[i] == '\n' || ptext[i] == '\r')
				break;
		}

		M_Memcpy(line, ptext, i);
		line[i] = 0;

		// flush the command text from the command buffer, _BEFORE_
		// executing, to avoid that 'recursive' aliases overflow the
		// command text buffer, in that case, new commands are inserted
		// at the beginning, in place of the actual, so it doesn't
		// overflow
		if (i == com_text.cursize)
			// the last command was just flushed
			com_text.cursize = 0;
		else
		{
			i++;
			com_text.cursize -= i;
			//memcpy(ptext, ptext+i, com_text.cursize); // Use memmove if the memory areas do overlap.
			memmove(ptext, ptext+i, com_text.cursize);
		}

		// execute the command line
		COM_ExecuteString(line);

		// delay following commands if a wait was encountered
		if (com_wait)
		{
			com_wait--;
			break;
		}
	}
}

/** Executes a string immediately.  Used for skirting around WAIT commands.
  */
void COM_ImmedExecute(const char *ptext)
{
	size_t i = 0, j = 0;
	char line[1024] = "";
	INT32 quotes;

	while (i < strlen(ptext))
	{

		quotes = 0;
		for (j = 0; i < strlen(ptext); i++,j++)
		{
			if (ptext[i] == '\"' && !quotes && i > 0 && ptext[i-1] != ' ') // Malformed command
				return;
			if (ptext[i] == '\"')
				quotes++;
			// don't break if inside a quoted string
			if ((!(quotes & 1) && ptext[i] == ';') || ptext[i] == '\n' || ptext[i] == '\r')
				break;
		}

		memcpy(line, ptext+(i-j), j);
		line[j] = 0;

		// execute the command line
		COM_ExecuteString(line);

		i++; // move to next character
	}
}

// =========================================================================
//                            COMMAND EXECUTION
// =========================================================================

xcommand_t *com_commands = NULL; // current commands

#define MAX_ARGS 80
static size_t com_argc;
static char *com_argv[MAX_ARGS];
static const char *com_null_string = "";
static char *com_args = NULL; // current command args or NULL
static int com_flags;

static void Got_NetVar(const UINT8 **p, INT32 playernum);

/** Initializes command buffer and adds basic commands.
  */
void COM_Init(void)
{
	// allocate command buffer
	VS_Alloc(&com_text, COM_BUF_SIZE);

	// cheats is a special cvar, so register it ASAP
	{
		extern struct CVarList *cvlist_command;
		CV_RegisterList(cvlist_command);
	}

	// add standard commands
	COM_AddCommand("alias", COM_Alias_f);
	COM_AddCommand("echo", COM_Echo_f);
	COM_AddCommand("cecho", COM_CEcho_f);
	COM_AddCommand("cechoflags", COM_CEchoFlags_f);
	COM_AddCommand("cechoduration", COM_CEchoDuration_f);
	COM_AddCommand("exec", COM_Exec_f);
	COM_AddCommand("wait", COM_Wait_f);
	COM_AddDebugCommand("help", COM_Help_f);
	COM_AddCommand("toggle", COM_Toggle_f);
	COM_AddCommand("add", COM_Add_f);
	COM_AddCommand("choose", COM_Choose_f);
	COM_AddCommand("chooseweighted", COM_ChooseWeighted_f);
	COM_AddCommand("reset", COM_Reset_f);
	RegisterNetXCmd(XD_NETVAR, Got_NetVar);
}

/** Gets a console command argument count.
  *
  * \return Number of arguments for the last command.
  * \sa COM_Argv
  */
size_t COM_Argc(void)
{
	return com_argc;
}

/** Gets a console command argument.
  *
  * \param arg Index of the argument (0 to COM_Argc() - 1).
  * \return String pointer to the indicated argument.
  * \sa COM_Argc, COM_Args
  */
const char *COM_Argv(size_t arg)
{
	if (arg >= com_argc || (signed)arg < 0)
		return com_null_string;
	return com_argv[arg];
}

/** Gets all console command arguments.
  *
  * \return String pointer to all arguments for the last command.
  * \sa COM_Argv
  */
char *COM_Args(void)
{
	return com_args;
}

/** Checks if a parameter was passed to a console command.
  *
  * \param check The parameter to look for, e.g. "-noerror".
  * \return The index of the argument containing the parameter,
  *         or 0 if the parameter was not found.
  */
size_t COM_CheckParm(const char *check)
{
	size_t i;

	for (i = 1; i < com_argc; i++)
		if (!strcasecmp(check, com_argv[i]))
			return i;
	return 0;
}

/** \brief COM_CheckParm, but checks only the start of each argument.
  *        E.g. checking for "-no" would match "-noerror" too.
  */
size_t COM_CheckPartialParm(const char *check)
{
	int  len;
	size_t i;

	len = strlen(check);

	for (i = 1; i < com_argc; i++)
	{
		if (strncasecmp(check, com_argv[i], len) == 0)
			return i;
	}
	return 0;
}

/** Find the first argument that starts with a hyphen (-).
  * \return The index of the argument, or 0
  *         if there are no such arguments.
  */
size_t COM_FirstOption(void)
{
	size_t i;

	for (i = 1; i < com_argc; i++)
	{
		if (com_argv[i][0] == '-')/* options start with a hyphen */
			return i;
	}
	return 0;
}

/** Parses a string into command-line tokens.
  *
  * \param ptext A null-terminated string. Does not need to be
  *             newline-terminated.
  */
static void COM_TokenizeString(char *ptext)
{
	size_t i;

	// Clear the args from the last string.
	for (i = 0; i < com_argc; i++)
		Z_Free(com_argv[i]);

	com_argc = 0;
	Z_Free(com_args);
	com_args = NULL;
	com_flags = 0;

	while (com_argc < MAX_ARGS)
	{
		// Skip whitespace up to a newline.
		while (*ptext != '\0' && *ptext <= ' ' && *ptext != '\n')
		{
			if (ptext[0] == '\033')
			{
				com_flags = (unsigned)ptext[1];
				ptext += 2;
			}
			else
				ptext++;
		}

		// A newline means end of command in buffer,
		// thus end of this command's args too.
		if (*ptext == '\n' || *ptext == '\0')
			break;

		if (com_argc == 1)
			com_args = COM_Purge(Z_StrDup(ptext), NULL);

		ptext = COM_Parse(ptext);
		if (ptext == NULL)
			break;

		com_argv[com_argc] = Z_StrDup(com_token);
		com_argc++;
	}
}

/** Adds a console command.
  *
  * \param name Name of the command.
  * \param func Function called when the command is run.
  */
xcommand_t *COM_AddCommand(const char *name, com_func_t func)
{
	xcommand_t *cmd;

	// fail if the command is a variable name
	if (CV_StringValue(name)[0] != '\0')
	{
		I_Error("%s is a variable name\n", name);
		return NULL;
	}

	// fail if the command already exists
	for (cmd = com_commands; cmd; cmd = cmd->next)
	{
		if (!stricmp(name, cmd->name)) //case insensitive now that we have lower and uppercase!
		{
			// don't I_Error for Lua commands
			// Lua commands can replace game commands, and they have priority.
			// BUT, if for some reason we screwed up and made two console commands with the same name,
			// it's good to have this here so we find out.
			if (cmd->function != COM_Lua_f)
				I_Error("Command %s already exists\n", name);

			return NULL;
		}
	}

	cmd = ZZ_Alloc(sizeof *cmd);
	cmd->name = name;
	cmd->function = func;
	cmd->debug = false;
	cmd->next = com_commands;
	com_commands = cmd;

	return cmd;
}

void COM_AddDebugCommand(const char *name, com_func_t func)
{
	xcommand_t *cmd = COM_AddCommand(name, func);

	cmd->debug = true;
}

/** Adds a console command for Lua.
  * No I_Errors allowed; return a negative code instead.
  *
  * \param name Name of the command.
  */
int COM_AddLuaCommand(const char *name)
{
	xcommand_t *cmd;

	// fail if the command is a variable name
	if (CV_StringValue(name)[0] != '\0')
		return -1;

	// command already exists
	for (cmd = com_commands; cmd; cmd = cmd->next)
	{
		if (!stricmp(name, cmd->name)) //case insensitive now that we have lower and uppercase!
		{
			// replace the built in command.
			cmd->function = COM_Lua_f;
			return 1;
		}
	}

	// Add a new command.
	cmd = ZZ_Alloc(sizeof *cmd);
	cmd->name = name;
	cmd->function = COM_Lua_f;
	cmd->debug = false;
	cmd->next = com_commands;
	com_commands = cmd;
	return 0;
}

/** Tests if a command exists.
  *
  * \param com_name Name to test for.
  * \return True if a command by the given name exists.
  */
static boolean COM_Exists(const char *com_name)
{
	xcommand_t *cmd;

	for (cmd = com_commands; cmd; cmd = cmd->next)
		if (!stricmp(com_name, cmd->name))
			return true;

	return false;
}

/** Does command completion for the console.
  *
  * \param partial The partial name of the command (potentially).
  * \param skips   Number of commands to skip.
  * \return The complete command name, or NULL.
  * \sa CV_CompleteAlias, CV_CompleteVar
  */
const char *COM_CompleteCommand(const char *partial, INT32 skips)
{
	xcommand_t *cmd;
	size_t len;

	len = strlen(partial);

	if (!len)
		return NULL;

	// check functions
	for (cmd = com_commands; cmd; cmd = cmd->next)
		if (!strncmp(partial, cmd->name, len))
			if (!skips--)
				return cmd->name;

	return NULL;
}

/** Completes the name of an alias.
  *
  * \param partial The partial name of the alias (potentially).
  * \param skips   Number of aliases to skip.
  * \return The complete alias name, or NULL.
  * \sa CV_CompleteCommand, CV_CompleteVar
  */
const char *COM_CompleteAlias(const char *partial, INT32 skips)
{
	cmdalias_t *a;
	size_t len;

	len = strlen(partial);

	if (!len)
		return NULL;

	// check functions
	for (a = com_alias; a; a = a->next)
		if (!strncmp(partial, a->name, len))
			if (!skips--)
				return a->name;

	return NULL;
}

/** Parses a single line of text into arguments and tries to execute it.
  * The text can come from the command buffer, a remote client, or stdin.
  *
  * \param ptext A single line of text.
  */
static void COM_ExecuteString(char *ptext)
{
	xcommand_t *cmd;
	cmdalias_t *a;
	static INT32 recursion = 0; // detects recursion and stops it if it goes too far

	COM_TokenizeString(ptext);

	// execute the command line
	if (COM_Argc() == 0)
		return; // no tokens

	// check functions
	for (cmd = com_commands; cmd; cmd = cmd->next)
	{
		if (!stricmp(com_argv[0], cmd->name)) //case insensitive now that we have lower and uppercase!
		{
			cmd->function();
			return;
		}
	}

	// check aliases
	for (a = com_alias; a; a = a->next)
	{
		if (!stricmp(com_argv[0], a->name))
		{
			if (recursion > MAX_ALIAS_RECURSION)
				CONS_Alert(CONS_WARNING, M_GetText("Alias recursion cycle detected!\n"));
			else
			{
				char buf[1024];
				char *write = buf, *read = a->value, *seek = read;

				while ((seek = strchr(seek, '$')) != NULL)
				{
					memcpy(write, read, seek-read);
					write += seek-read;

					seek++;

					if (*seek >= '1' && *seek <= '9')
					{
						if (com_argc > (size_t)(*seek - '0'))
						{
							memcpy(write, com_argv[*seek - '0'], strlen(com_argv[*seek - '0']));
							write += strlen(com_argv[*seek - '0']);
						}
						seek++;
					}
					else
					{
						*write = '$';
						write++;
					}

					read = seek;
				}
				WRITESTRING(write, read);

				// Monster Iestyn: keep track of how many levels of recursion we're in
				recursion++;
				COM_BufInsertText(buf);
				recursion--;
			}
			return;
		}
	}

	// check cvars
	// Hurdler: added at Ebola's request ;)
	// (don't flood the console in software mode with bad gl_xxx command)
	if (!CV_Command() && (con_destlines || dedicated))
		CONS_Printf(M_GetText("Unknown command '%s'\n"), COM_Argv(0));
}

// =========================================================================
//                            SCRIPT COMMANDS
// =========================================================================

/** Creates a command name that replaces another command.
  */
static void COM_Alias_f(void)
{
	cmdalias_t *a;

	if (COM_Argc() < 3)
	{
		CONS_Printf(M_GetText("alias <name> <command>: create a shortcut command that executes other command(s)\n"));
		return;
	}

	a = ZZ_Alloc(sizeof *a);
	a->next = com_alias;
	com_alias = a;

	a->name = Z_StrDup(COM_Argv(1));
	// Just use arg 2 if it's the only other argument, in case the alias is wrapped in quotes (backward compat, or multiple commands in one string).
	// Otherwise pull the whole string and seek to the end of the alias name. The strctr is in case the alias is quoted.
	a->value = Z_StrDup(COM_Argc() == 3 ? COM_Argv(2) : (strchr(COM_Args() + strlen(a->name), ' ') + 1));
}

/** Prints a line of text to the console.
  */
static void COM_Echo_f(void)
{
	size_t i;

	for (i = 1; i < COM_Argc(); i++)
		CONS_Printf("%s ", COM_Argv(i));
	CONS_Printf("\n");
}

/** Displays text on the center of the screen for a short time.
  */
static void COM_CEcho_f(void)
{
	size_t i;
	char cechotext[1024] = "";

	strncpy(cechotext, COM_Argv(1), sizeof(cechotext)-1);

	for (i = 2; i < COM_Argc(); i++)
	{
		strncat(cechotext, " ", sizeof(cechotext)-1);
		strncat(cechotext, COM_Argv(i), sizeof(cechotext)-1);
	}

	cechotext[sizeof(cechotext) - 1] = '\0';

	HU_DoTitlecardCEcho(NULL, cechotext, true);
}

/** Sets drawing flags for the CECHO command.
  */
static void COM_CEchoFlags_f(void)
{
	if (COM_Argc() > 1)
	{
		const char *arg = COM_Argv(1);

		if (arg[0] && arg[0] == '0' &&
			arg[1] && arg[1] == 'x') // Use hexadecimal!
			HU_SetCEchoFlags(axtoi(arg+2));
		else
			HU_SetCEchoFlags(atoi(arg));
	}
	else
		CONS_Printf(M_GetText("cechoflags <flags>: set CEcho flags, prepend with 0x to use hexadecimal\n"));
}

/** Sets the duration for CECHO commands to stay on the screen
  */
static void COM_CEchoDuration_f(void)
{
	if (COM_Argc() > 1)
		HU_SetCEchoDuration(atoi(COM_Argv(1)));
}

/** Executes a script file.
  */
static void COM_Exec_f(void)
{
	UINT8 *buf = NULL;
	char filename[256];

	if (COM_Argc() < 2 || COM_Argc() > 3)
	{
		CONS_Printf(M_GetText("exec <filename>: run a script file\n"));
		return;
	}

	// load file
	// Try with Argv passed verbatim first, for back compat
	FIL_ReadFile(COM_Argv(1), &buf);

	if (!buf)
	{
		// Now try by searching the file path
		// filename is modified with the full found path
		strcpy(filename, COM_Argv(1));
		if (findfile(filename, NULL, NULL, true) != FS_NOTFOUND)
			FIL_ReadFile(filename, &buf);

		if (!buf)
		{
			if (!COM_CheckParm("-noerror"))
				CONS_Printf(M_GetText("couldn't execute file %s\n"), COM_Argv(1));
			return;
		}
	}

	if (!COM_CheckParm("-silent"))
		CONS_Printf(M_GetText("executing %s\n"), COM_Argv(1));

	if (COM_CheckParm("-immediate"))
	{
		// immediately parses and executes all lines
		// sidesteps wait from all sources, even self
		COM_ImmedExecute((char *)buf);
	}
	else
	{
		// insert text file into the command buffer
		// delays execution if interpreting wait cmd
		COM_BufAddTextEx((char *)buf, com_flags);
		COM_BufAddTextEx("\n", com_flags);
	}

	// free buffer
	Z_Free(buf);
}

/** Delays execution of the rest of the commands until the next frame.
  * Allows sequences of commands like "jump; fire; backward".
  */
static void COM_Wait_f(void)
{
	if (COM_Argc() > 1)
		com_wait = atoi(COM_Argv(1));
	else
		com_wait = 1; // 1 frame
}

/** Prints help on variables and commands.
  */
static void COM_Help_f(void)
{
	xcommand_t *cmd;
	consvar_t *cvar;
	INT32 i = 0;

	if (COM_Argc() > 1)
	{
		const char *help = COM_Argv(1);
		cvar = CV_FindVar(help);
		if (cvar)
		{
			boolean floatmode = false;
			const char *cvalue = NULL;
			CONS_Printf("\x82""Variable %s:\n", cvar->name);
			CONS_Printf(M_GetText("  flags :"));
			if (cvar->flags & CV_SAVE)
				CONS_Printf("AUTOSAVE ");
			if (cvar->flags & CV_FLOAT)
			{
				CONS_Printf("FLOAT ");
				floatmode = true;
			}
			if (cvar->flags & CV_NETVAR)
				CONS_Printf("NETVAR ");
			if (cvar->flags & CV_CALL)
				CONS_Printf("ACTION ");
			if (cvar->flags & CV_CHEAT)
				CONS_Printf("CHEAT ");
			CONS_Printf("\n");
			if (cvar->PossibleValue)
			{
				CONS_Printf(" Possible values:\n");
				if (cvar->PossibleValue == CV_YesNo)
					CONS_Printf("  Yes or No (On or Off, True or False, 1 or 0)\n");
				else if (cvar->PossibleValue == CV_OnOff)
					CONS_Printf("  On or Off (Yes or No, True or False, 1 or 0)\n");
				else if (cvar->PossibleValue == CV_TrueFalse)
					CONS_Printf("  True or False (On or Off, Yes or No, 1 or 0)\n");
				else if (cvar->PossibleValue == Color_cons_t || cvar->PossibleValue == Followercolor_cons_t)
				{
					boolean follower = (cvar->PossibleValue == Followercolor_cons_t);
					for (i = SKINCOLOR_NONE; i < numskincolors; ++i)
					{
						if (K_ColorUsable(i, follower, true) == true)
						{
							CONS_Printf("  %-3d : %s\n", i, skincolors[i].name);
							if (i == cvar->value)
								cvalue = skincolors[i].name;
						}
					}

					if (follower)
					{
						CONS_Printf("  %-3d : %s\n", FOLLOWERCOLOR_MATCH, "Match");
						if (FOLLOWERCOLOR_MATCH == cvar->value)
							cvalue = "Match";
						CONS_Printf("  %-3d : %s\n", FOLLOWERCOLOR_MATCH, "Opposite");
						if (FOLLOWERCOLOR_OPPOSITE == cvar->value)
							cvalue = "Opposite";
					}
				}
				else
				{
#define MINVAL 0
#define MAXVAL 1
					if (!stricmp(cvar->PossibleValue[MINVAL].strvalue, "MIN"))
					{
						if (floatmode)
						{
							float fu = FIXED_TO_FLOAT(cvar->PossibleValue[MINVAL].value);
							float ck = FIXED_TO_FLOAT(cvar->PossibleValue[MAXVAL].value);
							CONS_Printf("  range from %ld%s to %ld%s\n",
									(long)fu, M_Ftrim(fu),
									(long)ck, M_Ftrim(ck));
						}
						else
							CONS_Printf("  range from %d to %d\n", cvar->PossibleValue[MINVAL].value,
								cvar->PossibleValue[MAXVAL].value);
						i = MAXVAL+1;
					}
#undef MINVAL
#undef MAXVAL

					//CONS_Printf(M_GetText("  possible value : %s\n"), cvar->name);
					while (cvar->PossibleValue[i].strvalue)
					{
						if (floatmode)
							CONS_Printf("  %-2f : %s\n", FIXED_TO_FLOAT(cvar->PossibleValue[i].value),
								cvar->PossibleValue[i].strvalue);
						else
							CONS_Printf("  %-2d : %s\n", cvar->PossibleValue[i].value,
								cvar->PossibleValue[i].strvalue);
						if (cvar->PossibleValue[i].value == cvar->value)
							cvalue = cvar->PossibleValue[i].strvalue;
						i++;
					}
				}
			}

			if (cvalue)
				CONS_Printf(" Current value: %s\n", cvalue);
			else if (cvar->string)
				CONS_Printf(" Current value: %s\n", cvar->string);
			else
				CONS_Printf(" Current value: %d\n", cvar->value);

			if (cvar->revert.v.string != NULL && strcmp(cvar->revert.v.string, cvar->string) != 0)
				CONS_Printf(" Value before netgame: %s\n", cvar->revert.v.string);
		}
		else
		{
			for (cmd = com_commands; cmd; cmd = cmd->next)
			{
				if (strcmp(cmd->name, help))
					continue;

				CONS_Printf("\x82""Command %s:\n", cmd->name);
				CONS_Printf("  help is not available for commands");
				CONS_Printf("\x82""\nCheck wiki.srb2.org for more or try typing <name> without arguments\n");
				return;
			}

			CONS_Printf("No exact match, searching...\n");

			// variables
			CONS_Printf("\x82""Variables:\n");
			for (cvar = consvar_vars; cvar; cvar = cvar->next)
			{
				if ((cvar->flags & CV_NOSHOWHELP) || (!strstr(cvar->name, help)))
					continue;
				CONS_Printf("%s ", cvar->name);
				i++;
			}

			// commands
			CONS_Printf("\x82""\nCommands:\n");
			for (cmd = com_commands; cmd; cmd = cmd->next)
			{
				if (!strstr(cmd->name, help))
					continue;
				CONS_Printf("%s ",cmd->name);
				i++;
			}

			CONS_Printf("\x82""\nCheck wiki.srb2.org for more or type help <command or variable>\n");

			CONS_Debug(DBG_GAMELOGIC, "\x87Total : %d\n", i);
		}
		return;
	}

	{
		// variables
		CONS_Printf("\x82""Variables:\n");
		for (cvar = consvar_vars; cvar; cvar = cvar->next)
		{
			if (cvar->flags & CV_NOSHOWHELP)
				continue;
			CONS_Printf("%s ", cvar->name);
			i++;
		}

		// commands
		CONS_Printf("\x82""\nCommands:\n");
		for (cmd = com_commands; cmd; cmd = cmd->next)
		{
			CONS_Printf("%s ",cmd->name);
			i++;
		}

		CONS_Printf("\x82""\nCheck wiki.srb2.org for more or type help <command or variable>\n");

		CONS_Debug(DBG_GAMELOGIC, "\x82Total : %d\n", i);
	}
}

/** Toggles a console variable. Useful for on/off values.
  *
  * This works on on/off, yes/no values by default. Given
  * a list of values, cycles between them.
  */
static void COM_Toggle_f(void)
{
	consvar_t *cvar;

	if (COM_Argc() == 1 || COM_Argc() == 3)
	{
		CONS_Printf(M_GetText("Toggle <cvar_name>: Toggle the value of a cvar\n"));
		CONS_Printf("Toggle <cvar_name> <value1> <value2>...: Cycle along a set of values\n");
		return;
	}
	cvar = CV_FindVar(COM_Argv(1));
	if (!cvar)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("%s is not a cvar\n"), COM_Argv(1));
		return;
	}

	if (COM_Argc() == 2)
	{
		if (!(cvar->PossibleValue == CV_YesNo || cvar->PossibleValue == CV_OnOff))
		{
			CONS_Alert(CONS_NOTICE, M_GetText("%s is not a boolean value\n"), COM_Argv(1));
			return;
		}
	}

	// netcvar don't change imediately
	cvar->flags |= CV_SHOWMODIFONETIME;

	if (COM_Argc() == 2)
	{
		CV_AddValue(cvar, +1);
	}
	else
	{
		size_t i;

		for (i = 2; i < COM_Argc() - 1; ++i)
		{
			const char *str = COM_Argv(i);
			INT32 val = 0;

			if (!cvar->PossibleValue ||
					CV_CompleteValue(cvar, &str, &val))
			{
				if (str ? !stricmp(cvar->string, str)
						: cvar->value == val)
				{
					CV_Set(cvar, COM_Argv(i + 1));
					return;
				}
			}
		}

		CV_Set(cvar, COM_Argv(2));
	}
}

/** Command variant of CV_AddValue
  */
static void COM_Add_f(void)
{
	consvar_t *cvar;

	if (COM_Argc() != 3)
	{
		CONS_Printf(M_GetText("Add <cvar_name> <value>: Add to the value of a cvar. Negative values work too!\n"));
		return;
	}
	cvar = CV_FindVar(COM_Argv(1));
	if (!cvar)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("%s is not a cvar\n"), COM_Argv(1));
		return;
	}

	if (( cvar->flags & CV_FLOAT ))
	{
		float n =FIXED_TO_FLOAT (cvar->value) + atof(COM_Argv(2));
		CV_Set(cvar, va("%ld%s", (long)n, M_Ftrim(n)));
	}
	else
		CV_AddValue(cvar, atoi(COM_Argv(2)));
}

static void COM_Choose_f(void)
{
	size_t na = COM_Argc();

	if (na < 2)
	{
		CONS_Printf(M_GetText("choose <option1> [<option2>] [<option3>] [...]: Picks a command at random\n"));
		return;
	}

	COM_BufAddText(COM_Argv(M_RandomKey(na - 1) + 1));
	COM_BufAddText("\n");
}

static void COM_ChooseWeighted_f(void)
{
	size_t na = COM_Argc();
	size_t i, cmd;
	const char *commands[40];
	INT32 weights[40];
	INT32 totalWeight = 0;
	INT32 roll;

	if (na < 3)
	{
		CONS_Printf(M_GetText("chooseweighted <option1> <weight1> [<option2> <weight2>] [<option3> <weight3>] [...]: Picks a command with weighted randomization\n"));
		return;
	}

	memset(weights, 0, sizeof(weights));

	i = 1;
	cmd = 0;
	while (i < na)
	{
		commands[cmd] = COM_Argv(i);

		i++;
		if (i >= na)
		{
			break;
		}

		weights[cmd] = atoi(COM_Argv(i));
		totalWeight += weights[cmd];

		i++;
		cmd++;
	}

	if (cmd == 0 || totalWeight <= 0)
	{
		return;
	}

	roll = M_RandomRange(1, totalWeight);

	for (i = 0; i < cmd; i++)
	{
		if (roll <= weights[i])
		{
			if (commands[i] == NULL)
			{
				break;
			}

			COM_BufAddText(commands[i]);
			COM_BufAddText("\n");
			break;
		}

		roll -= weights[i];
	}
}

static void COM_Reset_f(void)
{
	size_t i;

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("reset <cvar1> [cvar2] [...]: Resets a cvar to its default value\n"));
		return;
	}

	for (i = 1; i < COM_Argc(); ++i)
	{
		consvar_t *cvar = CV_FindVar(COM_Argv(i));

		if (!cvar)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("%s is not a cvar\n"), COM_Argv(i));
			continue;
		}

		CV_Set(cvar, cvar->defaultvalue);

		// Sometimes a cvar cannot be changed, e.g. CV_NETVAR without admin privilege.
		if (!stricmp(cvar->string, cvar->defaultvalue))
		{
			CONS_Printf("%s = %s\n", cvar->name, cvar->string);
		}
	}
}

// =========================================================================
//                      VARIABLE SIZE BUFFERS
// =========================================================================

/** Initializes a variable size buffer.
  *
  * \param buf      Buffer to initialize.
  * \param initsize Initial size for the buffer.
  */
void VS_Alloc(vsbuf_t *buf, size_t initsize)
{
#define VSBUFMINSIZE 256
	if (initsize < VSBUFMINSIZE)
		initsize = VSBUFMINSIZE;
	buf->data = Z_Malloc(initsize, PU_STATIC, NULL);
	buf->maxsize = initsize;
	buf->cursize = 0;
#undef VSBUFMINSIZE
}

/** Frees a variable size buffer.
  *
  * \param buf Buffer to free.
  */
void VS_Free(vsbuf_t *buf)
{
	buf->cursize = 0;
}

/** Clears a variable size buffer.
  *
  * \param buf Buffer to clear.
  */
void VS_Clear(vsbuf_t *buf)
{
	buf->cursize = 0;
}

/** Makes sure a variable size buffer has enough space for data of a
  * certain length.
  *
  * \param buf    The buffer. It is enlarged if necessary.
  * \param length The length of data we need to add.
  * \return Pointer to where the new data can go.
  */
void *VS_GetSpace(vsbuf_t *buf, size_t length)
{
	void *data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			I_Error("overflow 111");

		if (length > buf->maxsize)
			I_Error("overflow l%s 112", sizeu1(length));

		buf->overflowed = true;
		CONS_Printf("VS buffer overflow");
		VS_Clear(buf);
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

/** Copies data to the end of a variable size buffer.
  *
  * \param buf    The buffer.
  * \param data   The data to copy.
  * \param length The length of the data.
  * \sa VS_Print
  */
void VS_Write(vsbuf_t *buf, const void *data, size_t length)
{
	M_Memcpy(VS_GetSpace(buf, length), data, length);
}

void VS_WriteEx(vsbuf_t *buf, const void *data, size_t length, int flags)
{
	char *p;
	p = VS_GetSpace(buf, 2 + length);
	p[0] = '\033';
	p[1] = flags;
	M_Memcpy(&p[2], data, length);
}

/** Prints text in a variable buffer. Like VS_Write() plus a
  * trailing NUL.
  *
  * \param buf  The buffer.
  * \param data The NUL-terminated string.
  * \sa VS_Write
  */
void VS_Print(vsbuf_t *buf, const char *data)
{
	size_t len;

	len = strlen(data) + 1;

	if (buf->data[buf->cursize-1])
		M_Memcpy((UINT8 *)VS_GetSpace(buf, len), data, len); // no trailing 0
	else
		M_Memcpy((UINT8 *)VS_GetSpace(buf, len-1) - 1, data, len); // write over trailing 0
}

// =========================================================================
//
//                           CONSOLE VARIABLES
//
//   console variables are a simple way of changing variables of the game
//   through the console or code, at run time.
//
//   console vars acts like simplified commands, because a function can be
//   attached to them, and called whenever a console var is modified
//
// =========================================================================

static const char *cv_null_string = "";

/** Searches if a variable has been registered.
  *
  * \param name Variable to search for.
  * \return Pointer to the variable if found, or NULL.
  * \sa CV_FindVar
  */
static consvar_t *CV_FindVarInternal(const char *name)
{
	consvar_t *cvar;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if (!stricmp(name,cvar->name))
			return cvar;

	return NULL;
}

/** Searches if a variable has been registered and is visible to the console.
  *
  * \param name Variable to search for.
  * \return Pointer to the variable if found, or NULL.
  * \sa CV_FindVarInternal
  */
consvar_t *CV_FindVar(const char *name)
{
	consvar_t *cvar = CV_FindVarInternal(name);

	if (cvar && (cvar->flags & CV_NOSHOWHELP))
		return NULL;

	return cvar;
}

/** Finds a net variable based on its identifier number.
  *
  * \param netid The variable's identifier number.
  * \return A pointer to the variable itself if found, or NULL.
  */
static consvar_t *CV_FindNetVar(UINT16 netid)
{
	consvar_t *cvar;

	if (netid > consvar_number_of_netids)
		return NULL;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if (cvar->netid == netid)
			return cvar;

	if (netid == 44542) // ouch this hack
		return &cv_karteliminatelast;

	return NULL;
}

static void Setvalue(consvar_t *var, const char *valstr, boolean stealth);

/** Registers a variable for later use from the console.
  *
  * \param variable The variable to register.
  */
void CV_RegisterVar(consvar_t *variable)
{
	// first check to see if it has already been defined
	if (CV_FindVarInternal(variable->name))
	{
		CONS_Printf(M_GetText("Variable %s is already defined\n"), variable->name);
		return;
	}

	// check for overlap with a command
	if (COM_Exists(variable->name))
	{
		CONS_Printf(M_GetText("%s is a command name\n"), variable->name);
		return;
	}

	// check net variables
	if (variable->flags & CV_NETVAR)
	{
		/* in case of overflow... */
		if (consvar_number_of_netids == UINT16_MAX)
			I_Error("Way too many netvars");

		variable->netid = ++consvar_number_of_netids;
	}

	// link the variable in
	if (!(variable->flags & CV_HIDDEN))
	{
		variable->next = consvar_vars;
		consvar_vars = variable;
	}
	variable->string = variable->zstring = NULL;
	memset(&variable->revert, 0, sizeof variable->revert);
	variable->changed = 0; // new variable has not been modified by the user

#ifdef PARANOIA
	if ((variable->flags & CV_NOINIT) && !(variable->flags & CV_CALL))
		I_Error("variable %s has CV_NOINIT without CV_CALL\n", variable->name);
	if ((variable->flags & CV_CALL) && !variable->func)
		I_Error("variable %s has CV_CALL without a function\n", variable->name);
#endif

	if (variable->flags & CV_NOINIT)
		variable->flags &= ~CV_CALL;

	Setvalue(variable, variable->defaultvalue, false);

	if (variable->flags & CV_NOINIT)
		variable->flags |= CV_CALL;

	// the SetValue will set this bit
	variable->flags &= ~CV_MODIFIED;
}

/** Finds the string value of a console variable.
  *
  * \param var_name The variable's name.
  * \return The string value or "" if the variable is not found.
  */
static const char *CV_StringValue(const char *var_name)
{
	consvar_t *var;

	var = CV_FindVar(var_name);
	if (!var)
		return cv_null_string;
	return var->string;
}

/** Completes the name of a console variable.
  *
  * \param partial The partial name of the variable (potentially).
  * \param skips   Number of variables to skip.
  * \return The complete variable name, or NULL.
  * \sa COM_CompleteCommand, CV_CompleteAlias
  */
const char *CV_CompleteVar(char *partial, INT32 skips)
{
	consvar_t *cvar;
	size_t len;

	len = strlen(partial);

	if (!len)
		return NULL;

	// check variables
	for (cvar = consvar_vars; cvar; cvar = cvar->next)
	{
		if (cvar->flags & CV_NOSHOWHELP)
			continue;
		if (strncmp(partial, cvar->name, len))
			continue;
		if (skips--)
			continue;
		return cvar->name;
	}

	return NULL;
}

boolean CV_CompleteValue(consvar_t *var, const char **valstrp, INT32 *intval)
{
	const char *valstr = *valstrp;

	INT32 overrideval = 0;

	INT32 v;

	if (var == &cv_forceskin)
	{
		v = R_SkinAvailable(valstr);

		if (!R_SkinUsable(-1, v, false))
			v = -1;

		goto finish;
	}

	if (var->PossibleValue)
	{
		INT32 i;

		if (var->flags & CV_FLOAT)
		{
			double d = atof(valstr);
			if (fpclassify(d) == FP_ZERO && valstr[0] != '0')
				v = INT32_MIN;
			else
				v = (INT32)(d * FRACUNIT);
		}
		else
		{
			v = atoi(valstr);
			if (!v && valstr[0] != '0')
				v = INT32_MIN; // Invalid integer trigger
		}

		if (var->PossibleValue[0].strvalue && !stricmp(var->PossibleValue[0].strvalue, "MIN")) // bounded cvar
		{
#define MINVAL 0
#define MAXVAL 1
#ifdef PARANOIA
			if (!var->PossibleValue[MAXVAL].strvalue)
				I_Error("Bounded cvar \"%s\" without maximum!\n", var->name);
#endif

			// search for other
			for (i = MAXVAL+1; var->PossibleValue[i].strvalue; i++)
				if (v == var->PossibleValue[i].value || !stricmp(var->PossibleValue[i].strvalue, valstr))
					goto found;

			if ((v != INT32_MIN && v < var->PossibleValue[MINVAL].value) || !stricmp(valstr, "MIN"))
			{
				i = MINVAL;
				goto found;
			}
			else if ((v != INT32_MIN && v > var->PossibleValue[MAXVAL].value) || !stricmp(valstr, "MAX"))
			{
				i = MAXVAL;
				goto found;
			}
			if (v == INT32_MIN)
				goto badinput;
			valstr = NULL; // not a preset value
#undef MINVAL
#undef MAXVAL
		}
		else
		{
			// check first strings
			for (i = 0; var->PossibleValue[i].strvalue; i++)
				if (!stricmp(var->PossibleValue[i].strvalue, valstr))
					goto found;
			if (v != INT32_MIN)
			{
				// check INT32 now
				for (i = 0; var->PossibleValue[i].strvalue; i++)
					if (v == var->PossibleValue[i].value)
						goto found;
			}
			// Not found ... but wait, there's hope!
			if (var->PossibleValue == CV_OnOff || var->PossibleValue == CV_YesNo || var->PossibleValue == CV_TrueFalse)
			{
				overrideval = -1;
				if (!stricmp(valstr, "on") || !stricmp(valstr, "yes") || !stricmp(valstr, "true"))
					overrideval = 1;
				else if (!stricmp(valstr, "off") || !stricmp(valstr, "no") || !stricmp(valstr, "false"))
					overrideval = 0;

				if (overrideval != -1)
				{
					for (i = 0; var->PossibleValue[i].strvalue; i++)
						if (overrideval == var->PossibleValue[i].value)
							goto found;
				}
			}

			// ...or not.
			goto badinput;
found:
			v = var->PossibleValue[i].value;
			valstr = var->PossibleValue[i].strvalue;
		}

finish:
		if (intval)
			*intval = v;

		*valstrp = valstr;

		return true;
	}

// landing point for possiblevalue failures
badinput:
	return false;
}

/** Sets a value to a variable with less checking. Only for internal use.
  *
  * \param var    Variable to set.
  * \param valstr String value for the variable.
  */
static void Setvalue(consvar_t *var, const char *valstr, boolean stealth)
{
	boolean override = false;
	INT32 overrideval = 0;
	const char *overridestr = valstr;

	if ((var->flags & CV_CHEAT) && CV_CheatsEnabled() == false)
	{
		// Enforce to default value without cheats.
		if (stricmp(var->defaultvalue, valstr) != 0)
		{
			// Warn the user about this.
			CONS_Printf("This cannot be used without cheats enabled.\n");
		}

		valstr = overridestr = var->defaultvalue;
	}

	if (CV_CompleteValue(var, &overridestr, &overrideval))
	{
		if (overridestr)
		{
			valstr = overridestr;
			override = true;
		}
	}
	else if (var->PossibleValue)
		goto badinput;

	if (client && execversion_enabled)
	{
		if (var->revert.allocated)
		{
			Z_Free(var->revert.v.string);

			// Z_StrDup creates a new zone memory block, so we can keep the allocated flag on
			if (override)
				var->revert.allocated = false; // the below value is not allocated in zone memory, don't try to free it!
		}

		if (override)
			var->revert.v.const_munge = valstr;
		else
			var->revert.v.string = Z_StrDup(valstr);

		return;
	}

	// free the old value string
	Z_Free(var->zstring);

	if (override)
	{
		var->zstring = NULL;
		var->value = overrideval;
		var->string = valstr;
	}
	else
	{
		var->string = var->zstring = Z_StrDup(valstr);

		if (var->flags & CV_FLOAT)
		{
			double d = atof(var->string);
			var->value = (INT32)(d * FRACUNIT);
		}
		else
			var->value = atoi(var->string);
	}

	if (var->flags & CV_SHOWMODIFONETIME || var->flags & CV_SHOWMODIF)
	{
		CONS_Printf(M_GetText("%s set to %s\n"), var->name, var->string);
		var->flags &= ~CV_SHOWMODIFONETIME;
	}
	else // display message in debug file only
	{
		DEBFILE(va("%s set to %s\n", var->name, var->string));
	}

	var->flags |= CV_MODIFIED;

	// raise 'on change' code
	LUA_CVarChanged(var); // let consolelib know what cvar this is.

	if ((var->flags & CV_CALL) && !stealth)
		var->func();

	return;

// landing point for possiblevalue failures
badinput:

	CONS_Printf(M_GetText("\"%s\" is not a possible value for \"%s\"\n"), valstr, var->name);

	// default value not valid... ?!
	if (var->defaultvalue == valstr)
		I_Error("Variable %s default value \"%s\" is not a possible value\n", var->name, var->defaultvalue);
}

//
// Use XD_NETVAR argument:
//      2 byte for variable identification
//      then the value of the variable followed with a 0 byte (like str)
//

static boolean serverloading = false;

static consvar_t *
ReadNetVar (const UINT8 **p, const char **return_value, boolean *return_stealth)
{
	UINT16  netid;
	const char   *val;
	boolean stealth;

	consvar_t *cvar;

	netid   = READUINT16 (*p);
	val     = (const char *)*p;
	SKIPSTRING (*p);
	stealth = READUINT8  (*p);

	cvar = CV_FindNetVar(netid);

	if (cvar)
	{
		(*return_value)   = val;
		(*return_stealth) = stealth;

		DEBFILE(va("Netvar received: %s [netid=%d] value %s\n", cvar->name, netid, val));
	}
	else
		CONS_Alert(CONS_WARNING, "Netvar not found with netid %hu\n", netid);

	return cvar;
}

static consvar_t *
ReadDemoVar (const UINT8 **p, const char **return_value, boolean *return_stealth)
{
	const char   *name;
	const char   *val;
	boolean stealth;

	consvar_t *cvar;

	name    = (const char *)*p;
	SKIPSTRING (*p);
	val     = (const char *)*p;
	SKIPSTRING (*p);
	stealth = READUINT8  (*p);

	cvar = CV_FindVarInternal(name);

	if (cvar)
	{
		(*return_value)   = val;
		(*return_stealth) = stealth;
	}
	else
		CONS_Alert(CONS_WARNING, "Netvar not found with name %s\n", name);

	return cvar;
}

static void Got_NetVar(const UINT8 **p, INT32 playernum)
{
	consvar_t *cvar;
	const char *svalue;
	boolean stealth;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum) && !serverloading)
	{
		// not from server or remote admin, must be hacked/buggy client
		CONS_Alert(CONS_WARNING, M_GetText("Illegal netvar command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	cvar = ReadNetVar(p, &svalue, &stealth);

	if (cvar)
	{
		Setvalue(cvar, svalue, stealth);

		if ((cvar->flags & CV_CHEAT) && stricmp(cvar->string, cvar->defaultvalue) != 0) // use cvar->string to compare what it is now
		{
			CV_CheaterWarning(playernum, va("%s %s", cvar->name, svalue)); // but use svalue to show what they inputted
		}
	}
}

void CV_SaveVars(UINT8 **p, boolean in_demo)
{
	consvar_t *cvar;
	UINT8 *count_p = *p;
	UINT16 count = 0;

	// send only changed cvars ...
	// the client will reset all netvars to default before loading
	WRITEUINT16(*p, 0x0000);
	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if ((cvar->flags & CV_NETVAR) && !CV_IsSetToDefault(cvar))
		{
			if (in_demo)
				WRITESTRING(*p, cvar->name);
			else
				WRITEUINT16(*p, cvar->netid);

			WRITESTRING(*p, cvar->string);

			WRITEUINT8(*p, false);
			++count;
		}
	WRITEUINT16(count_p, count);
}

static size_t CV_LoadVars(const UINT8 *bufstart,
		consvar_t *(*got)(const UINT8 **p, const char **ret_value, boolean *ret_stealth))
{
	const UINT8 *p = bufstart;
	const boolean store = (client || demo.playback);

	consvar_t *cvar;
	UINT16 count;

	const char *val;
	boolean stealth;

	// prevent "invalid command received"
	serverloading = true;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
	{
		if (cvar->flags & CV_NETVAR)
		{
			if (store && cvar->revert.v.string == NULL)
			{
				cvar->revert.v.const_munge = cvar->string;
				cvar->revert.allocated = ( cvar->zstring != NULL );
				cvar->zstring = NULL;/* don't free this */
			}

			Setvalue(cvar, cvar->defaultvalue, true);
		}
	}

	count = READUINT16(p);
	while (count--)
	{
		cvar = (*got)(&p, &val, &stealth);

		if (cvar)
			Setvalue(cvar, val, stealth);
	}

	serverloading = false;

	return p - bufstart;
}

void CV_RevertNetVars(void)
{
	consvar_t * cvar;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
	{
		if (cvar->revert.v.string != NULL)
		{
			Setvalue(
				cvar,
				cvar->revert.v.string,
#ifdef DEVELOP
				(cvar == &cv_cheats)
#else
				false
#endif
			);

			if (cvar->revert.allocated)
			{
				Z_Free(cvar->revert.v.string);
				cvar->revert.allocated = false; // no value being held now
			}

			cvar->revert.v.string = NULL;
		}
	}
}

size_t CV_LoadNetVars(const UINT8 *p)
{
	return CV_LoadVars(p, ReadNetVar);
}

size_t CV_LoadDemoVars(const UINT8 *p)
{
	return CV_LoadVars(p, ReadDemoVar);
}

static void CV_SetCVar(consvar_t *var, const char *value, boolean stealth);

void CV_CheatsChanged(void)
{
	if (CV_CheatsEnabled())
	{
		G_SetUsedCheats();
	}
	else
	{
		consvar_t *cvar;

		// Set everything back to default.
		for (cvar = consvar_vars; cvar; cvar = cvar->next)
			if (cvar->flags & CV_CHEAT)
				CV_SetCVar(cvar, cvar->defaultvalue, false);

		// Reset any other cheat command effects here, as well.
		cht_debug = 0;

		P_ResetPlayerCheats();
	}
}

// Returns true if the variable's current value is its default value
boolean CV_IsSetToDefault(consvar_t *v)
{
	return (!(strcmp(v->defaultvalue, v->string)));
}

// If any cheats CVars are not at their default settings, return true.
// Else return false.
boolean CV_CheatsEnabled(void)
{
	return (boolean)cv_cheats.value;
}

// Consistent print about cheaters in multiplayer.
void CV_CheaterWarning(UINT8 playerID, const char *command)
{
	if (netgame)
	{
		CONS_Printf("\x85" "%s cheats:" "\x80" " %s\n", player_names[playerID], command);
	}
}

/** Sets a value to a variable, performing some checks and calling the
  * callback function if there is one.
  * Does as if "<varname> <value>" is entered at the console.
  *
  * \param var   The variable.
  * \param value The string value.
  * \sa CV_StealthSet, CV_SetValue
  */
static void CV_SetCVar(consvar_t *var, const char *value, boolean stealth)
{
#ifdef PARANOIA
	if (!var)
		I_Error("CV_Set: no variable\n");
	if (!var->string)
		I_Error("CV_Set: %s no string set!\n", var->name);
#endif
	if (!var || !var->string || !value || !stricmp(var->string, value))
		return; // no changes

	if (var->flags & CV_NETVAR)
	{
		// send the value of the variable
		UINT8 buf[128];
		UINT8 *p = buf;

		// Loading from a config in a netgame? Set revert value.
		if (client && execversion_enabled)
		{
			Setvalue(var, value, true);
			return;
		}

		if (!(server || (addedtogame && IsPlayerAdmin(consoleplayer))))
		{
			CONS_Printf(M_GetText("Only the server or admin can change: %s %s\n"), var->name, var->string);
			return;
		}

		if (var == &cv_kartencore && !M_SecretUnlocked(SECRET_ENCORE, false))
		{
			CONS_Printf(M_GetText("You haven't unlocked Encore Mode yet!\n"));
			return;
		}

		if (var == &cv_kartspeed && !M_SecretUnlocked(SECRET_HARDSPEED, false))
		{
			if (!stricmp(value, "Gear 3") || atoi(value) >= KARTSPEED_HARD)
			{
				CONS_Printf(M_GetText("You haven't unlocked this yet!\n"));
				return;
			}
		}

		if (var == &cv_forceskin)
		{
			INT32 skin = R_SkinAvailable(value);
			if ((stricmp(value, "None")) && ((skin == -1) || !R_SkinUsable(-1, skin, false)))
			{
				CONS_Printf("Please provide a valid skin name (\"None\" disables).\n");
				return;
			}
		}

		// Only add to netcmd buffer if in a netgame, otherwise, just change it.
		if (netgame)
		{
			WRITEUINT16(p, var->netid);
			WRITESTRING(p, value);
			WRITEUINT8(p, stealth);

			SendNetXCmd(XD_NETVAR, buf, p-buf);
		}
		else
			Setvalue(var, value, stealth);
	}
	else
		if ((var->flags & CV_NOTINNET) && netgame)
		{
			CONS_Printf(M_GetText("This variable can't be changed while in netgame: %s %s\n"), var->name, var->string);
			return;
		}
		else
			Setvalue(var, value, stealth);
}

/** Sets a value to a variable without calling its callback function.
  *
  * \param var   The variable.
  * \param value The string value.
  * \sa CV_Set, CV_StealthSetValue
  */
void CV_StealthSet(consvar_t *var, const char *value)
{
	CV_SetCVar(var, value, true);
}

/** Sets a numeric value to a variable, sometimes calling its callback
  * function.
  *
  * \param var   The variable.
  * \param value The numeric value, converted to a string before setting.
  * \param stealth Do we call the callback function or not?
  */
static void CV_SetValueMaybeStealth(consvar_t *var, INT32 value, boolean stealth)
{
	char val[SKINNAMESIZE+1];

	if (var == &cv_forceskin) // Special handling.
	{
		const char *tmpskin = NULL;
		if ((value < 0) || (value >= numskins))
			tmpskin = "None";
		else
			tmpskin = skins[value]->name;
		strlcpy(val, tmpskin, SKINNAMESIZE+1);
	}
	else
		sprintf(val, "%d", value);

	CV_SetCVar(var, val, stealth);
}

/** Sets a numeric value to a variable without calling its callback
  * function.
  *
  * \param var   The variable.
  * \param value The numeric value, converted to a string before setting.
  * \sa CV_SetValue, CV_StealthSet
  */
void CV_StealthSetValue(consvar_t *var, INT32 value)
{
	CV_SetValueMaybeStealth(var, value, true);
}

// New wrapper for what used to be CV_Set()
void CV_Set(consvar_t *var, const char *value)
{
	CV_SetCVar(var, value, false);
}

/** Sets a numeric value to a variable, performing some checks and
  * calling the callback function if there is one.
  *
  * \param var   The variable.
  * \param value The numeric value, converted to a string before setting.
  * \sa CV_Set, CV_StealthSetValue
  */
void CV_SetValue(consvar_t *var, INT32 value)
{
	CV_SetValueMaybeStealth(var, value, false);
}

/** Adds a value to a console variable.
  * Used to increment and decrement variables from the menu.
  * Contains special cases to handle pointlimit in some multiplayer modes,
  * map number for game hosting, etc.
  *
  * \param var       The variable to add to.
  * \param increment The change in the variable; can be negative for a
  *                  decrement.
  * \sa CV_SetValue
  */
void CV_AddValue(consvar_t *var, INT32 increment)
{
	INT32 newvalue, max;

	if (!increment)
		return;

	if (var == &cv_forceskin) // Special handling.
	{
		INT32 oldvalue = var->value;
		newvalue = oldvalue;
		do
		{
			newvalue += increment;
			if (newvalue < -1)
				newvalue = (numskins - 1);
			else if (newvalue >= numskins)
				newvalue = -1;
		} while ((oldvalue != newvalue)
				&& !(R_SkinUsable(-1, newvalue, false)));
	}
	else
		newvalue = var->value + increment;

	if (var->PossibleValue)
	{
#define MINVAL 0
#define MAXVAL 1
		if (var->PossibleValue[MINVAL].strvalue && !strcmp(var->PossibleValue[MINVAL].strvalue, "MIN"))
		{
#ifdef PARANOIA
			if (!var->PossibleValue[MAXVAL].strvalue)
				I_Error("Bounded cvar \"%s\" without maximum!\n", var->name);
#endif

			if (newvalue < var->PossibleValue[MINVAL].value || newvalue > var->PossibleValue[MAXVAL].value)
			{
				INT32 currentindice = -1, newindice;
				for (max = MAXVAL+1; var->PossibleValue[max].strvalue; max++)
				{
					if (var->PossibleValue[max].value == newvalue)
					{
						increment = 0;
						currentindice = max;
						break; // The value we definitely want, stop here.
					}
					else if (var->PossibleValue[max].value == var->value)
						currentindice = max; // The value we maybe want.
				}

				if (increment)
				{
					increment = (increment > 0) ? 1 : -1;
					if (currentindice == -1 && max != MAXVAL+1)
						newindice = ((increment > 0) ? MAXVAL : max) + increment;
					else
						newindice = currentindice + increment;

					if (newindice >= max || newindice <= MAXVAL)
					{
						newvalue = var->PossibleValue[((increment > 0) ? MINVAL : MAXVAL)].value;
						CV_SetValue(var, newvalue);
					}
					else
						CV_Set(var, var->PossibleValue[newindice].strvalue);
				}
				else
					CV_Set(var, var->PossibleValue[currentindice].strvalue);
			}
			else
				CV_SetValue(var, newvalue);
		}
#undef MINVAL
#undef MAXVAL
		else
		{
			INT32 currentindice = -1, newindice;

			// this code do not support more than same value for differant PossibleValue
			for (max = 0; var->PossibleValue[max].strvalue; max++)
				if (var->PossibleValue[max].value == var->value)
					currentindice = max;

			// The following options will NOT handle netsyncing.
			if (var == &cv_chooseskin)
			{
				// Special case for the chooseskin variable, used only directly from the menu
				newvalue = var->value - 1;
				do
				{
					if (increment > 0) // Going up!
					{
						newvalue++;
						if (newvalue == MAXSKINS)
							newvalue = 0;
					}
					else if (increment < 0) // Going down!
					{
						newvalue--;
						if (newvalue == -1)
							newvalue = MAXSKINS-1;
					}
				} while (var->PossibleValue[newvalue].strvalue == NULL);

				var->value = newvalue + 1;
				var->string = var->PossibleValue[newvalue].strvalue;
				var->func();
				return;
			}
			else if (var == &cv_playercolor[0] || var == &cv_playercolor[1] || var == &cv_playercolor[2] || var == &cv_playercolor[3])
			{
				// Special case for the playercolor variable, used only directly from the menu
				if (increment > 0) // Going up!
				{
					newvalue = var->value + 1;
					if (newvalue > numskincolors-1)
						newvalue = 1;
					var->value = newvalue;
					var->string = var->PossibleValue[var->value].strvalue;
					var->func();
					return;
				}
				else if (increment < 0) // Going down!
				{
					newvalue = var->value - 1;
					if (newvalue < 1)
						newvalue = numskincolors-1;
					var->value = newvalue;
					var->string = var->PossibleValue[var->value].strvalue;
					var->func();
					return;
				}
			}
			else if (var->PossibleValue == kartspeed_cons_t
				|| var->PossibleValue == dummykartspeed_cons_t
				|| var->PossibleValue == gpdifficulty_cons_t)
			{
				if (!M_SecretUnlocked(SECRET_HARDSPEED, false))
				{
					max = KARTSPEED_NORMAL+1;
					if (var->PossibleValue == kartspeed_cons_t)
						max++; // Accommodate KARTSPEED_AUTO
				}
				else if (var->PossibleValue == gpdifficulty_cons_t
					&& !M_SecretUnlocked(SECRET_MASTERMODE, false))
				{
					max = KARTSPEED_HARD+1;
				}
			}
#ifdef PARANOIA
			if (currentindice == -1)
				I_Error("CV_AddValue: current value %d not found in possible value\n",
					var->value);
#endif

			newindice = (currentindice + increment + max) % max;
			CV_Set(var, var->PossibleValue[newindice].strvalue);
		}
	}
	else
		CV_SetValue(var, newvalue);

	var->changed = 1; // user has changed it now
}

void CV_InitFilterVar(void)
{
#if 0
	UINT8 i;
	for (i = 0; i < 4; i++)
	{
		joyaxis_default[i] = true;
		joyaxis_count[i] = 0;
	}
#endif
}

void CV_ToggleExecVersion(boolean enable)
{
	execversion_enabled = enable;
}

void CV_EnforceExecVersion(void);
void CV_EnforceExecVersion(void)
{
	if (!execversion_enabled)
		CV_StealthSetValue(&cv_execversion, EXECVERSION);
}

static boolean CV_FilterVarByVersion(consvar_t *v, const char *valstr)
{
	(void)valstr;

	// True means allow the CV change, False means block it

	// We only care about CV_SAVE because this filters the user's config files
	// We do this same check in CV_Command
	if (!(v->flags & CV_SAVE))
		return true;

#if 0 // We don't have changed saved cvars yet
	if (GETMAJOREXECVERSION(cv_execversion.value) < 2)
	{
		if (!stricmp(v->name, "example"))
			return false;
	}
#endif

	return true;
}

/** Displays or changes a variable from the console.
  * Since the user is presumed to have been directly responsible
  * for this change, the variable is marked as changed this game.
  *
  * \return False if passed command was not recognized as a console
  *         variable, otherwise true.
  * \sa CV_ClearChangedFlags
  */
static boolean CV_Command(void)
{
	consvar_t *v;

	// check variables
	v = CV_FindVar(COM_Argv(0));
	if (!v)
		return false;

	if (( com_flags & COM_SAFE ) && ( v->flags & CV_NOLUA ))
		return false;

	// perform a variable print or set
	if (COM_Argc() == 1)
	{
		CONS_Printf(M_GetText("\"%s\" is \"%s\" default is \"%s\"\n"), v->name, v->string, v->defaultvalue);
		return true;
	}

	if (!(v->flags & CV_SAVE) || CV_FilterVarByVersion(v, COM_Argv(1)))
	{
		CV_Set(v, COM_Argv(1));
		v->changed = 1; // now it's been changed by (presumably) the user
	}
	return true;
}

/** Marks all variables as unchanged, indicating they've not been changed
  * by the user this game.
  *
  * \sa CV_Command
  * \author Graue <graue@oceanbase.org>
  */
void CV_ClearChangedFlags(void)
{
	consvar_t *cvar;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		cvar->changed = 0;
}

/** Saves console variables to a file if they have the ::CV_SAVE
  * flag set.
  *
  * \param f File to save to.
  */
void CV_SaveVariables(FILE *f)
{
	consvar_t *cvar;

	for (cvar = consvar_vars; cvar; cvar = cvar->next)
		if (cvar->flags & CV_SAVE)
		{
			char stringtowrite[MAXTEXTCMD+1];

			const char * string;

			if (cvar->revert.v.string != NULL)
			{
				string = cvar->revert.v.string;
			}
			else
			{
				string = cvar->string;
			}

			// Silly hack for Min/Max vars
#define MINVAL 0
#define MAXVAL 1
			if (
					cvar->PossibleValue != NULL &&
					cvar->PossibleValue[0].strvalue &&
					stricmp(cvar->PossibleValue[0].strvalue, "MIN") == 0
			){ // bounded cvar
				int which = stricmp(string, "MAX") == 0;

				if (which || stricmp(string, "MIN") == 0)
				{
					INT32 value = cvar->PossibleValue[which].value;

					if (cvar->flags & CV_FLOAT)
						sprintf(stringtowrite, "%f", FIXED_TO_FLOAT(value));
					else
						sprintf(stringtowrite, "%d", value);

					string = stringtowrite;
				}
			}
#undef MINVAL
#undef MAXVAL

			fprintf(f, "%s \"%s\"\n", cvar->name, string);
		}
}

//============================================================================
//                            SCRIPT PARSE
//============================================================================

/** Parses a token out of a string. Handles script files too.
  *
  * \param data String to parse.
  * \return The data pointer after the token. NULL if no token found.
  */
static char *COM_Parse(char *data)
{
	char c;
	size_t len = 0;

	com_token[0] = 0;

	if (data == NULL)
		return NULL;

	// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == '\0')
			return NULL; // end of file;
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		for (;;)
		{
			c = *data++;
			if (c == '\"' || c == '\0')
			{
				com_token[len] = 0;
				return data;
			}
			if (c == '\033')
				data++;
			else
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (c == '\033')
		{
			do
			{
				data += 2;
				c = *data;
			}
			while (c == '\033') ;
		}
		else
		{
			com_token[len] = c;
			data++;
			len++;
			c = *data;
		}
	} while (c > 32);

	com_token[len] = 0;
	return data;
}
