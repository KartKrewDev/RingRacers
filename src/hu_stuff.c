// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  hu_stuff.c
/// \brief Heads up display

#include "doomdef.h"
#include "byteptr.h"
#include "hu_stuff.h"
#include "font.h"

#include "k_menu.h" // highlightflags
#include "m_cond.h" // emblems
#include "m_misc.h" // word jumping

#include "d_clisrv.h"

#include "g_game.h"
#include "g_input.h"

#include "i_video.h"
#include "i_system.h"

#include "st_stuff.h" // ST_HEIGHT
#include "r_local.h"

#include "keys.h"
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

#include "console.h"
#include "am_map.h"
#include "d_main.h"

#include "p_local.h" // camera[]
#include "p_tick.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "lua_hud.h"
#include "lua_hudlib_drawlist.h"
#include "lua_hook.h"

// SRB2Kart
#include "s_sound.h" // song credits
#include "k_kart.h"
#include "k_battle.h"
#include "k_grandprix.h"
#include "k_color.h"
#include "k_hud.h"
#include "r_fps.h"
#include "d_clisrv.h"
#include "y_inter.h" // Y_PlayerStandingsDrawer
#include "g_party.h"

// coords are scaled
#define HU_INPUTX 0
#define HU_INPUTY 0

//-------------------------------------------
//              heads up font
//-------------------------------------------

// ping font
// Note: I'd like to adress that at this point we might *REALLY* want to work towards a common drawString function that can take any font we want because this is really turning into a MESS. :V -Lat'
patch_t *pinggfx[5];	// small ping graphic
patch_t *mping[5]; // smaller ping graphic
patch_t *pingmeasure[2]; // ping measurement graphic
patch_t *pinglocal[2]; // mindelay indecator

patch_t *framecounter;
patch_t *frameslash;	// framerate stuff. Used in screen.c

static player_t *plr;
boolean hu_keystrokes; // :)
boolean chat_on; // entering a chat message?
static char w_chat[HU_MAXMSGLEN + 1];
static size_t c_input = 0; // let's try to make the chat input less shitty.
static boolean headsupactive = false;
boolean hu_showscores; // draw rankings
static char hu_tick;

//-------------------------------------------
//              misc vars
//-------------------------------------------

patch_t *missingpat;
patch_t *blanklvl, *nolvl;
patch_t *unvisitedlvl[4];

// song credits
static patch_t *songcreditbg;

// -------
// protos.
// -------
static void HU_DrawRankings(void);

//======================================================================
//                 KEYBOARD LAYOUTS FOR ENTERING TEXT
//======================================================================

char *shiftxform;

char english_shiftxform[] =
{
	0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31,
	' ', '!', '"', '#', '$', '%', '&',
	'"', // shift-'
	'(', ')', '*', '+',
	'<', // shift-,
	'_', // shift--
	'>', // shift-.
	'?', // shift-/
	')', // shift-0
	'!', // shift-1
	'@', // shift-2
	'#', // shift-3
	'$', // shift-4
	'%', // shift-5
	'^', // shift-6
	'&', // shift-7
	'*', // shift-8
	'(', // shift-9
	':',
	':', // shift-;
	'<',
	'+', // shift-=
	'>', '?', '@',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', // shift-[
	'|', // shift-backslash - OH MY GOD DOES WATCOM SUCK
	'}', // shift-]
	'"', '_',
	'~', // shift-`
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', '|', '}', '~', 127
};

static char cechotext[1024];
static tic_t cechotimer = 0;
static tic_t cechoduration = 5*TICRATE;
static INT32 cechoflags = 0;

struct tcecho_state
{
	char text[1024];	// buffer for the titlecard text
	tic_t start;		// gametic that the message started
	tic_t duration;		// Set automatically
};

#define NUM_TCECHO_STATES (1 + MAXSPLITSCREENPLAYERS)
static struct tcecho_state g_tcecho[NUM_TCECHO_STATES];

static tic_t resynch_ticker = 0;

static huddrawlist_h luahuddrawlist_scores;

//======================================================================
//                          HEADS UP INIT
//======================================================================

// just after
static void Command_Say_f(void);
static void Command_Sayto_f(void);
static void Command_Sayteam_f(void);
static void Command_CSay_f(void);
static void Command_Shout(void);
static void Got_Saycmd(const UINT8 **p, INT32 playernum);

void HU_LoadGraphics(void)
{
	INT32 i;

	if (dedicated)
		return;

	Font_Load();

	HU_UpdatePatch(&blanklvl, "BLANKLVL");
	HU_UpdatePatch(&nolvl, "M_NOLVL");

	for (i = 0; i < 4; i++)
	{
		HU_UpdatePatch(&unvisitedlvl[i], "PREVST0%d", i+1);
	}

	HU_UpdatePatch(&songcreditbg, "K_SONGCR");

	// cache ping gfx:
	for (i = 0; i < 5; i++)
	{
		HU_UpdatePatch(&pinggfx[i], "PINGGFX%d", i+1);
		HU_UpdatePatch(&mping[i], "MPING%d", i+1);
	}

	HU_UpdatePatch(&pingmeasure[0], "PINGD");
	HU_UpdatePatch(&pingmeasure[1], "PINGMS");

	HU_UpdatePatch(&pinglocal[0], "PINGGFXL");
	HU_UpdatePatch(&pinglocal[1], "MPINGL");

	// fps stuff
	HU_UpdatePatch(&framecounter, "FRAMER");
	HU_UpdatePatch(&frameslash, "FRAMESL");
}

// Initialise Heads up
// once at game startup.
//
void HU_Init(void)
{
	font_t font;

	COM_AddCommand("say", Command_Say_f);
	COM_AddCommand("sayto", Command_Sayto_f);
	COM_AddCommand("sayteam", Command_Sayteam_f);
	COM_AddCommand("csay", Command_CSay_f);
	COM_AddCommand("shout", Command_Shout);
	RegisterNetXCmd(XD_SAY, Got_Saycmd);

	// only allocate if not present, to save us a lot of headache
	if (missingpat == NULL)
	{
		lumpnum_t missingnum = W_GetNumForName("MISSING");
		if (missingnum == LUMPERROR)
			I_Error("HU_LoadGraphics: \"MISSING\" patch not present in resource files.");

		missingpat = W_CachePatchNum(missingnum, PU_STATIC);
	}

	// set shift translation table
	shiftxform = english_shiftxform;

	/*
	Setup fonts
	*/

	if (!dedicated)
	{
#define  DIM( s, n ) ( font.start = s, font.size = n )
#define ADIM( name )        DIM (name ## _FONTSTART, name ## _FONTSIZE)
#define   PR( s )           strcpy(font.prefix, s)
#define  DIG( n )           ( font.digits = n )
#define  REG                Font_DumbRegister(&font)

		DIG  (3);

		ADIM (HU);
		PR   ("STCFN");
		REG;

		PR   ("MNUFN");
		REG;

		PR   ("TNYFN");
		REG;

		PR   ("MDFN");
		REG;

		PR   ("FILEF");
		REG;

		ADIM (LT);
		PR   ("LTFNT");
		REG;

		ADIM (CRED);
		PR   ("CRFNT");
		REG;

		DIG  (3);

		ADIM (LT);

		PR   ("GTOL");
		REG;

		PR   ("GTFN");
		REG;

		PR   ("4GTOL");
		REG;

		PR   ("4GTFN");
		REG;

		DIG  (1);

		DIM  (0, 10);

		PR   ("STTNUM");
		REG;

		PR   ("NGTNUM");
		REG;

		PR   ("PINGN");
		REG;

		PR   ("PRFN");
		REG;

		DIM  ('0', 10);
		DIG  (2);

		PR   ("ROLNUM");
		REG;

		PR   ("RO4NUM");
		REG;

		DIG  (3);

		ADIM (KART);
		PR   ("MKFNT");
		REG;

		ADIM (NUM);
		PR   ("TMFNT");
		REG;

		PR   ("TMFNS");
		REG;

		ADIM (LT);
		PR   ("GAMEM");
		REG;

		ADIM (LT);
		PR   ("THIFN");
		REG;

		PR   ("TLWFN");
		REG;

		ADIM (NUM);
		PR   ("OPPRF");
		REG;

		ADIM (NUM);
		PR   ("PINGF");
		REG;

#undef  REG
#undef  DIG
#undef  PR
#undef  ADMIN
#undef  DIM
	}

	HU_LoadGraphics();

	luahuddrawlist_scores = LUA_HUD_CreateDrawList();
}

patch_t *HU_UpdateOrBlankPatch(patch_t **user, boolean required, const char *format, ...)
{
	va_list ap;
	char buffer[9];

	lumpnum_t lump = INT16_MAX;
	patch_t *patch;

	va_start (ap, format);
	vsnprintf(buffer, sizeof buffer, format, ap);
	va_end   (ap);

	if (user && partadd_earliestfile != UINT16_MAX)
	{
		UINT16 fileref = numwadfiles;
		lump = INT16_MAX;

		while ((lump == INT16_MAX) && ((--fileref) >= partadd_earliestfile))
		{
			lump = W_CheckNumForNamePwad(buffer, fileref, 0);
		}

		/* no update in this wad */
		if (fileref < partadd_earliestfile)
			return *user;

		lump |= (fileref << 16);
	}
	else
	{
		lump = W_CheckNumForName(buffer);

		if (lump == LUMPERROR)
		{
			if (required == true)
				*user = missingpat;

			return *user;
		}
	}

	patch = W_CachePatchNum(lump, PU_HUDGFX);

	if (user)
	{
		if (*user)
			Patch_Free(*user);

		*user = patch;
	}

	return patch;
}

static inline void HU_Stop(void)
{
	headsupactive = false;
}

//
// Reset Heads up when consoleplayer spawns
//
void HU_Start(void)
{
	if (headsupactive)
		HU_Stop();

	plr = &players[consoleplayer];

	headsupactive = true;
}

//======================================================================
//                            EXECUTION
//======================================================================


// EVERY CHANGE IN THIS SCRIPT IS LOL XD! BY VINCYTM

static UINT32 chat_nummsg_log = 0;
static UINT32 chat_nummsg_min = 0;
static UINT32 chat_scroll = 0;
static tic_t chat_scrolltime = 0;

static UINT32 chat_maxscroll = 0; // how far can we scroll?

//static chatmsg_t chat_mini[CHAT_BUFSIZE]; // Display the last few messages sent.
//static chatmsg_t chat_log[CHAT_BUFSIZE]; // Keep every message sent to us in memory so we can scroll n shit, it's cool.

static char chat_log[CHAT_BUFSIZE][255]; // hold the last 48 or so messages in that log.
static char chat_mini[8][255]; // display up to 8 messages that will fade away / get overwritten
static tic_t chat_timers[8];

static boolean chat_scrollmedown = false; // force instant scroll down on the chat log. Happens when you open it / send a message.

// remove text from minichat table

static INT16 addy = 0; // use this to make the messages scroll smoothly when one fades away

static void HU_removeChatText_Mini(void)
{
	// MPC: Don't create new arrays, just iterate through an existing one
	size_t i;
	for(i=0;i<chat_nummsg_min-1;i++) {
		strcpy(chat_mini[i], chat_mini[i+1]);
		chat_timers[i] = chat_timers[i+1];
	}
	chat_nummsg_min--; // lost 1 msg.

	// use addy and make shit slide smoothly af.
	addy += (vid.width < 640) ? 8 : 6;

}

// same but w the log. TODO: optimize this and maybe merge in a single func? im bad at C.
static void HU_removeChatText_Log(void)
{
	// MPC: Don't create new arrays, just iterate through an existing one
	size_t i;
	for(i=0;i<chat_nummsg_log-1;i++) {
		strcpy(chat_log[i], chat_log[i+1]);
	}
	chat_nummsg_log--; // lost 1 msg.
}

void HU_AddChatText(const char *text, boolean playsound)
{
	if (playsound && cv_consolechat.value != 2)	// Don't play the sound if we're using hidden chat.
		S_StartSound(NULL, sfx_radio);
	// reguardless of our preferences, put all of this in the chat buffer in case we decide to change from oldchat mid-game.

	if (chat_nummsg_log >= CHAT_BUFSIZE) // too many messages!
		HU_removeChatText_Log();

	strcpy(chat_log[chat_nummsg_log], text);
	chat_nummsg_log++;

	if (chat_nummsg_min >= 8)
		HU_removeChatText_Mini();

	strcpy(chat_mini[chat_nummsg_min], text);
	chat_timers[chat_nummsg_min] = TICRATE*cv_chattime.value;
	chat_nummsg_min++;

	if (OLDCHAT) // if we're using oldchat, print directly in console
		CONS_Printf("%s\n", text);
	else			// if we aren't, still save the message to log.txt
		CON_LogMessage(va("%s\n", text));
}

/** Runs a say command, sending an ::XD_SAY message.
  * A say command consists of a signed 8-bit integer for the target, an
  * unsigned 8-bit flag variable, and then the message itself.
  *
  * The target is 0 to say to everyone, 1 to 32 to say to that player, or -1
  * to -32 to say to everyone on that player's team. Note: This means you
  * have to add 1 to the player number, since they are 0 to 31 internally.
  *
  * The flag HU_SHOUT will be set if it is the dedicated server speaking.
  *
  * This function obtains the message using COM_Argc() and COM_Argv().
  *
  * \param target    Target to send message to.
  * \param usedargs  Number of arguments to ignore.
  * \param flags     Set HU_CSAY for server/admin to CECHO everyone.
  * \sa Command_Say_f, Command_Sayteam_f, Command_Sayto_f, Got_Saycmd
  * \author Graue <graue@oceanbase.org>
  */

void DoSayCommand(char *message, SINT8 target, UINT8 flags, UINT8 source)
{
	char buf[3 + HU_MAXMSGLEN];
	char *p = buf;

	// Enforce shout for the dedicated server.
	if (dedicated && source == serverplayer && !(flags & HU_CSAY))
	{
		flags |= HU_SHOUT;
	}

	WRITESINT8(p, target);
	WRITEUINT8(p, flags);
	WRITEUINT8(p, source);
	WRITESTRINGN(p, message, HU_MAXMSGLEN);

	SendNetXCmd(XD_SAY, buf, p - buf);
}

/** Send a message to everyone.
  * \sa DoSayPacket, Command_Sayteam_f, Command_Sayto_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Say_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("say <message>: send a message\n"));
		return;
	}

	// Autoshout is handled by HU_queueChatChar.
	// If you're using the say command, you can use the shout command, lol.
	DoSayPacketFromCommand(0, 1, 0);
}

/** Send a message to a particular person.
  * \sa DoSayPacket, Command_Sayteam_f, Command_Say_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Sayto_f(void)
{
	INT32 target;

	if (COM_Argc() < 3)
	{
		CONS_Printf(M_GetText("sayto <playername|playernum> <message>: send a message to a player\n"));
		return;
	}

	target = nametonum(COM_Argv(1));
	if (target == -1)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("No player with that name!\n"));
		return;
	}
	target++; // Internally we use 0 to 31, but say command uses 1 to 32.

	DoSayPacketFromCommand((SINT8)target, 2, 0);
}

/** Send a message to members of the player's team.
  * \sa DoSayPacket, Command_Say_f, Command_Sayto_f
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Sayteam_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("sayteam <message>: send a message to your team\n"));
		return;
	}

	if (dedicated)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Dedicated servers can't send team messages. Use \"say\".\n"));
		return;
	}

	if (G_GametypeHasTeams())	// revert to normal say if we don't have teams in this gametype.
		DoSayPacketFromCommand(-1, 1, 0);
	else
		DoSayPacketFromCommand(0, 1, 0);
}

/** Send a message to everyone, to be displayed by CECHO. Only
  * permitted to servers and admins.
  */
static void Command_CSay_f(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("csay <message>: send a message to be shown in the middle of the screen\n"));
		return;
	}

	if (!server && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Only servers and admins can use csay.\n"));
		return;
	}

	DoSayPacketFromCommand(0, 1, HU_CSAY);
}

static void Command_Shout(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("shout <message>: send a message with special alert sound, name, and color\n"));
		return;
	}

	if (!server && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Only servers and admins can use shout.\n"));
		return;
	}

	DoSayPacketFromCommand(0, 1, HU_SHOUT);
}

/** Receives a message, processing an ::XD_SAY command.
  * \sa DoSayPacket
  * \author Graue <graue@oceanbase.org>
  */
static void Got_Saycmd(const UINT8 **p, INT32 playernum)
{
	SINT8 target;
	UINT8 flags;
	const char *dispname;
	char buf[HU_MAXMSGLEN + 1];
	char *msg;
	boolean action = false;
	char *ptr;

	CONS_Debug(DBG_NETPLAY,"Received SAY cmd from Player %d (%s)\n", playernum+1, player_names[playernum]);

	// Only server can ever legitimately send this
	if (playernum != serverplayer)
		return;

	target = READSINT8(*p);
	flags = READUINT8(*p);
	playernum = READUINT8(*p);
	msg = buf;
	READSTRINGN(*p, msg, HU_MAXMSGLEN);

	//check for invalid characters (0x80 or above)
	{
		size_t i;
		const size_t j = strlen(msg);
		for (i = 0; i < j; i++)
		{
			if (msg[i] & 0x80)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Illegal say command received from %s containing invalid characters\n"), player_names[playernum]);
				if (server)
					SendKick(playernum, KICK_MSG_CON_FAIL);
				return;
			}
		}
	}

	// If it's a CSAY, just CECHO and be done with it.
	if (flags & HU_CSAY)
	{
		HU_SetCEchoDuration(5);
		I_OutputMsg("Server message: ");
		HU_DoCEcho(msg);
		return;
	}

	// Handle "/me" actions, but only in messages to everyone.
	if (target == 0 && strlen(msg) > 4 && strnicmp(msg, "/me ", 4) == 0)
	{
		msg += 4;
		action = true;
	}

	if (flags & HU_SHOUT)
		dispname = cv_shoutname.zstring;
	else
		dispname = player_names[playernum];

	// Clean up message a bit
	// If you use a \r character, you can remove your name
	// from before the text and then pretend to be someone else!
	ptr = msg;
	while (*ptr != '\0')
	{
		if (*ptr == '\r')
			*ptr = ' ';

		ptr++;
	}

	// Show messages sent by you, to you, to your team, or to everyone:
	if (playernum == consoleplayer // By you
	|| (target == -1 && ST_SameTeam(&players[consoleplayer], &players[playernum])) // To your team
	|| target == 0 // To everyone
	|| consoleplayer == target-1) // To you
	{
		const char *prefix = "", *cstart = "", *cend = "", *adminchar = "\x82~\x83", *remotechar = "\x82@\x83", *fmt2, *textcolor = "\x80";
		char *tempchar = NULL;
		char color_prefix[2];

		if (flags & HU_SHOUT)
		{
			if (cv_shoutcolor.value == -1)
			{
				UINT16 chatcolor = skincolors[players[playernum].skincolor].chatcolor;

				if (chatcolor > V_TANMAP)
				{
					sprintf(color_prefix, "%c", '\x80');
				}
				else
				{
					sprintf(color_prefix, "%c", '\x80' + (chatcolor >> V_CHARCOLORSHIFT));
				}
			}
			else
			{
				sprintf(color_prefix, "%c", '\x80' + cv_shoutcolor.value);
			}

			// Colorize full text
			cstart = textcolor = color_prefix;
		}
		else if (players[playernum].spectator)
		{
			// grey text
			cstart = textcolor = "\x86";
		}
		else if (target == -1) // say team
		{
			if (players[playernum].ctfteam == 1)
			{
				// red text
				cstart = textcolor = "\x85";
			}
			else
			{
				// blue text
				cstart = textcolor = "\x84";
			}
		}
		else
		{
			UINT16 chatcolor = skincolors[players[playernum].skincolor].chatcolor;

			if (chatcolor > V_TANMAP)
			{
				sprintf(color_prefix, "%c", '\x80');
			}
			else
			{
				sprintf(color_prefix, "%c", '\x80' + (chatcolor >> V_CHARCOLORSHIFT));
			}

			cstart = color_prefix;
		}
		prefix = cstart;

		// Give admins and remote admins their symbols.
		if (playernum == serverplayer)
			tempchar = (char *)Z_Calloc(strlen(cstart) + strlen(adminchar) + 1, PU_STATIC, NULL);
		else if (IsPlayerAdmin(playernum))
			tempchar = (char *)Z_Calloc(strlen(cstart) + strlen(remotechar) + 1, PU_STATIC, NULL);

		if (tempchar)
		{
			if (playernum == serverplayer)
				strcat(tempchar, adminchar);
			else
				strcat(tempchar, remotechar);
			strcat(tempchar, cstart);
			cstart = tempchar;
		}

		// Choose the proper format string for display.
		// Each format includes four strings: color start, display
		// name, color end, and the message itself.
		// '\4' makes the message yellow and beeps; '\3' just beeps.
		if (action)
			fmt2 = "* %s%s%s%s \x82%s%s";
		else if (target-1 == consoleplayer) // To you
		{
			prefix = "\x82[PM]";
			cstart = "\x82";
			textcolor = "\x82";
			fmt2 = "%s<%s%s>%s\x80 %s%s";

			if (flags & HU_PRIVNOTICE)
			{
				dispname = "SERVER";
				prefix = "\x82";
				fmt2 = "%s[%s%s]%s %s%s";
			}
		}
		else if (target > 0) // By you, to another player
		{
			// Use target's name.
			dispname = player_names[target-1];
			prefix = "\x82[TO]";
			cstart = "\x82";
			fmt2 = "%s<%s%s>%s\x80 %s%s";

			if (flags & HU_PRIVNOTICE)
			{
				if (tempchar)
					Z_Free(tempchar);
				return; // I pretend I do not see it
			}
		}
		else // To everyone or sayteam, it doesn't change anything.
			fmt2 = "%s<%s%s%s>\x80 %s%s";
		/*else // To your team
		{
			if (players[playernum].ctfteam == 1) // red
				prefix = "\x85[TEAM]";
			else if (players[playernum].ctfteam == 2) // blue
				prefix = "\x84[TEAM]";
			else
				prefix = "\x83"; // makes sure this doesn't implode if you sayteam on non-team gamemodes

			fmt2 = "%s<%s%s>\x80%s %s%s";
		}*/

		HU_AddChatText(va(fmt2, prefix, cstart, dispname, cend, textcolor, msg), (cv_chatnotifications.value) && !(flags & HU_SHOUT)); // add to chat

		if ((cv_chatnotifications.value) && (flags & HU_SHOUT))
			S_StartSound(NULL, sfx_sysmsg);

		if (tempchar)
			Z_Free(tempchar);
	}
#ifdef _DEBUG
	// I just want to point out while I'm here that because the data is still
	// sent to all players, techincally anyone can see your chat if they really
	// wanted to, even if you used sayto or sayteam.
	// You should never send any sensitive info through sayto for that reason.
	else
		CONS_Printf("Dropped chat: %d %d %s\n", playernum, target, msg);
#endif
}

void HU_TickSongCredits(void)
{
	if (cursongcredit.text == NULL) // No def
	{
		cursongcredit.x = cursongcredit.old_x = 0;
		cursongcredit.anim = 0;
		cursongcredit.trans = NUMTRANSMAPS;
		return;
	}

	cursongcredit.old_x = cursongcredit.x;

	if (cursongcredit.anim > 0)
	{
		INT32 len = V_ThinStringWidth(cursongcredit.text, 0);
		fixed_t destx = (len+7) * FRACUNIT;

		if (cursongcredit.trans > 0)
		{
			cursongcredit.trans--;
		}

		if (cursongcredit.x < destx)
		{
			cursongcredit.x += (destx - cursongcredit.x) / 2;
		}

		if (cursongcredit.x > destx)
		{
			cursongcredit.x = destx;
		}

		cursongcredit.anim--;
	}
	else
	{
		if (cursongcredit.trans < NUMTRANSMAPS)
		{
			cursongcredit.trans++;
		}

		if (cursongcredit.x > 0)
		{
			cursongcredit.x /= 2;
		}

		if (cursongcredit.x < 0)
		{
			cursongcredit.x = 0;
		}
	}
}

void HU_Ticker(void)
{
	static boolean hu_holdscores = false;

	if (dedicated)
		return;

	hu_tick++;
	hu_tick &= 7; // currently only to blink chat input cursor

	// Rankings
	if (G_PlayerInputDown(0, gc_rankings, 0))
	{
		if (!hu_holdscores)
		{
			hu_showscores ^= true;
		}
		hu_holdscores = true;
	}
	else
	{
		hu_holdscores = false;
	}

	hu_keystrokes = false;

	if (chat_on)
	{
		// count down the scroll timer.
		if (chat_scrolltime > 0)
			chat_scrolltime--;
	}

	if (netgame)
	{
		size_t i = 0;

		// handle chat timers
		for (i=0; (i<chat_nummsg_min); i++)
		{
			if (chat_timers[i] > 0)
				chat_timers[i]--;
			else
				HU_removeChatText_Mini();
		}
	}

	if (cechotimer)
		cechotimer--;

	if (gamestate != GS_LEVEL)
	{
		return;
	}

	resynch_ticker++;
}

static boolean teamtalk = false;
static boolean justscrolleddown;
static boolean justscrolledup;
static INT16 typelines = 1; // number of drawfill lines we need when drawing the chat. it's some weird hack and might be one frame off but I'm lazy to make another loop.
// It's up here since it has to be reset when we open the chat.

static boolean HU_chatboxContainsOnlySpaces(void)
{
	size_t i;

	for (i = 0; w_chat[i]; i++)
		if (w_chat[i] != ' ')
			return false;

	return true;
}

static void HU_sendChatMessage(void)
{
	char buf[2 + HU_MAXMSGLEN + 1];
	char *msg = &buf[2];
	size_t ci;
	INT32 target = 0;

	// if our message was nothing but spaces, don't send it.
	if (HU_chatboxContainsOnlySpaces())
		return;

	// copy printable characters and terminating '\0' only.
	for (ci = 2; w_chat[ci-2]; ci++)
	{
		char c = w_chat[ci-2];
		if (c >= ' ' && !(c & 0x80))
			buf[ci] = c;
	};
	buf[ci] = '\0';

	memset(w_chat, '\0', sizeof(w_chat));
	c_input = 0;

	if (strlen(msg) > 4 && strnicmp(msg, "/pm", 3) == 0) // used /pm
	{
		INT32 spc = 1; // used if playernum[1] is a space.
		char playernum[3];
		const char *newmsg;

		strncpy(playernum, msg+3, 3);
		// check for undesirable characters in our "number"
		if (!(isdigit(playernum[0]) && isdigit(playernum[1])))
		{
			// check if playernum[1] is a space
			if (playernum[1] == ' ')
				spc = 0;
				// let it slide
			else
			{
				HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<player num> \'.", false);
				return;
			}
		}
		// I'm very bad at C, I swear I am, additional checks eww!
		if (spc != 0 && msg[5] != ' ')
		{
			HU_AddChatText("\x82NOTICE: \x80Invalid command format. Correct format is \'/pm<player num> \'.", false);
			return;
		}

		target = atoi(playernum); // turn that into a number

		// check for target player, if it doesn't exist then we can't send the message!
		if (target < MAXPLAYERS && playeringame[target]) // player exists
			target++; // even though playernums are from 0 to 31, target is 1 to 32, so up that by 1 to have it work!
		else
		{
			HU_AddChatText(va("\x82NOTICE: \x80Player %d does not exist.", target), false); // same
			return;
		}

		// we need to get rid of the /pm<player num>
		newmsg = msg+5+spc;
		strlcpy(msg, newmsg, HU_MAXMSGLEN + 1);
	}
	if (ci > 2) // don't send target+flags+empty message.
	{
		if (teamtalk)
			buf[0] = -1; // target
		else
			buf[0] = target;

		buf[1] = ((server || IsPlayerAdmin(consoleplayer)) && cv_autoshout.value) ? HU_SHOUT : 0; // flags

		DoSayPacket(target, buf[1], consoleplayer, msg);
	}
}

void HU_clearChatChars(void)
{
	memset(w_chat, '\0', sizeof(w_chat));
	chat_on = false;
	c_input = 0;

	I_UpdateMouseGrab();
}

//
// Returns true if key eaten
//
boolean HU_Responder(event_t *ev)
{
	if (ev->type != ev_keydown)
		return false;

	// only KeyDown events now...

	// Shoot, to prevent P1 chatting from ruining the game for everyone else, it's either:
	// A. completely disallow opening chat entirely in online splitscreen
	// or B. iterate through all controls to make sure it's bound to player 1 before eating
	// You can see which one I chose.
	// (Unless if you're sharing a keyboard, since you probably establish when you start chatting that you have dibs on it...)
	// (Ahhh, the good ol days when I was a kid who couldn't afford an extra USB controller...)

	if (ev->data1 >= NUMKEYS)
	{
		INT32 i, j;
		for (i = 0; i < num_gamecontrols; i++)
		{
			for (j = 0; j < MAXINPUTMAPPING; j++)
			{
				if (gamecontrol[0][i][j] == ev->data1)
					break;
			}

			if (j < MAXINPUTMAPPING)
			{
				break;
			}
		}

		if (i == num_gamecontrols && j == MAXINPUTMAPPING)
			return false;
	}

	if (!chat_on)
	{
		// enter chat mode
		if ((ev->data1 == gamecontrol[0][gc_talk][0] || ev->data1 == gamecontrol[0][gc_talk][1]
			|| ev->data1 == gamecontrol[0][gc_talk][2] || ev->data1 == gamecontrol[0][gc_talk][3])
			&& netgame && !OLD_MUTE) // check for old chat mute, still let the players open the chat incase they want to scroll otherwise.
		{
			chat_on = true;
			w_chat[0] = 0;
			teamtalk = false;
			chat_scrollmedown = true;
			typelines = 1;
			return true;
		}
		if ((ev->data1 == gamecontrol[0][gc_teamtalk][0] || ev->data1 == gamecontrol[0][gc_teamtalk][1]
			|| ev->data1 == gamecontrol[0][gc_teamtalk][2] || ev->data1 == gamecontrol[0][gc_teamtalk][3])
			&& netgame && !OLD_MUTE)
		{
			chat_on = true;
			w_chat[0] = 0;
			teamtalk = G_GametypeHasTeams();	// Don't teamtalk if we don't have teams.
			chat_scrollmedown = true;
			typelines = 1;
			return true;
		}
	}
	else // if chat_on
	{
		INT32 c = (INT32)ev->data1;

		// Ignore modifier keys
		// Note that we do this here so users can still set
		// their chat keys to one of these, if they so desire.
		if (ev->data1 == KEY_LSHIFT || ev->data1 == KEY_RSHIFT
		 || ev->data1 == KEY_LCTRL || ev->data1 == KEY_RCTRL
		 || ev->data1 == KEY_LALT || ev->data1 == KEY_RALT)
			return true;

		// Ignore non-keyboard keys, except when the talk key is bound
		if (ev->data1 >= NUMKEYS
		/*&& (ev->data1 != gamecontrol[0][gc_talkkey][0]
		&& ev->data1 != gamecontrol[0][gc_talkkey][1])*/)
			return false;

		c = CON_ShiftChar(c);

		// pasting. pasting is cool. chat is a bit limited, though :(
		if ((c == 'v' || c == 'V') && ctrldown)
		{
			const char *paste;
			size_t chatlen;
			size_t pastelen;

			if (CHAT_MUTE)
				return true;

			paste = I_ClipboardPaste();
			if (paste == NULL)
				return true;

			chatlen = strlen(w_chat);
			pastelen = strlen(paste);
			if (chatlen+pastelen > HU_MAXMSGLEN)
				return true; // we can't paste this!!

			memmove(&w_chat[c_input + pastelen], &w_chat[c_input], (chatlen - c_input) + 1); // +1 for '\0'
			memcpy(&w_chat[c_input], paste, pastelen); // copy all of that.
			c_input += pastelen;
			return true;
		}
		else if (c == KEY_ENTER)
		{
			if (!CHAT_MUTE)
				HU_sendChatMessage();

			chat_on = false;
			c_input = 0; // reset input cursor
			chat_scrollmedown = true; // you hit enter, so you might wanna autoscroll to see what you just sent. :)
			I_UpdateMouseGrab();
		}
		else if (c == KEY_ESCAPE
			/*|| ((c == gamecontrol[0][gc_talkkey][0] || c == gamecontrol[0][gc_talkkey][1]
			|| c == gamecontrol[0][gc_teamkey][0] || c == gamecontrol[0][gc_teamkey][1])
			&& c >= NUMKEYS)*/) // If it's not a keyboard key, then the chat button is used as a toggle.
		{
			chat_on = false;
			c_input = 0; // reset input cursor
			I_UpdateMouseGrab();
		}
		else if ((c == KEY_UPARROW || c == KEY_MOUSEWHEELUP) && chat_scroll > 0 && !OLDCHAT) // CHAT SCROLLING YAYS!
		{
			chat_scroll--;
			justscrolledup = true;
			chat_scrolltime = 4;
		}
		else if ((c == KEY_DOWNARROW || c == KEY_MOUSEWHEELDOWN) && chat_scroll < chat_maxscroll && chat_maxscroll > 0 && !OLDCHAT)
		{
			chat_scroll++;
			justscrolleddown = true;
			chat_scrolltime = 4;
		}
		else if (c == KEY_LEFTARROW && c_input != 0 && !OLDCHAT) // i said go back
		{
			if (ctrldown)
				c_input = M_JumpWordReverse(w_chat, c_input);
			else
				c_input--;
		}
		else if (c == KEY_RIGHTARROW && c_input < strlen(w_chat) && !OLDCHAT) // don't need to check for admin or w/e here since the chat won't ever contain anything if it's muted.
		{
			if (ctrldown)
				c_input += M_JumpWord(&w_chat[c_input]);
			else
				c_input++;
		}
		else if ((c >= HU_FONTSTART && c <= HU_FONTEND && fontv[HU_FONT].font[c-HU_FONTSTART])
			|| c == ' ') // Allow spaces, of course
		{
			if (CHAT_MUTE || strlen(w_chat) >= HU_MAXMSGLEN)
				return true;

			memmove(&w_chat[c_input + 1], &w_chat[c_input], strlen(w_chat) - c_input + 1);
			w_chat[c_input] = c;
			c_input++;
		}
		else if (c == KEY_BACKSPACE)
		{
			if (CHAT_MUTE || c_input <= 0)
				return true;

			memmove(&w_chat[c_input - 1], &w_chat[c_input], strlen(w_chat) - c_input + 1);
			c_input--;
		}
		else if (c == KEY_DEL)
		{
			if (CHAT_MUTE || c_input >= strlen(w_chat))
				return true;

			memmove(&w_chat[c_input], &w_chat[c_input + 1], strlen(w_chat) - c_input);
		}

		return true;
	}

	return false;
}

//======================================================================
//                         HEADS UP DRAWING
//======================================================================

// Precompile a wordwrapped string to any given width.
// Now a wrapper for the chat drawer.
static char *CHAT_WordWrap(INT32 w, fixed_t scale, INT32 option, const char *string)
{
	return V_ScaledWordWrap(
		w << FRACBITS,
		scale, FRACUNIT, FRACUNIT,
		option,
		HU_FONT,
		string
	);
}


// 30/7/18: chaty is now the distance at which the lowest point of the chat will be drawn if that makes any sense.

INT16 chatx = 13, chaty = 169; // let's use this as our coordinates

// chat stuff by VincyTM LOL XD!

// HU_DrawMiniChat

static void HU_drawMiniChat(void)
{
	INT32 x = chatx+2;
	const INT32 charheight = (vid.width < 640) ? 12 : 6;
	INT32 boxw = cv_chatwidth.value;
	size_t i = chat_nummsg_min;

	INT32 msglines = 0;
	// process all messages once without rendering anything or doing anything fancy so that we know how many lines each message has...
	INT32 y;

	if (!chat_nummsg_min)
		return; // needless to say it's useless to do anything if we don't have anything to draw.

	if (r_splitscreen > 1)
		boxw = max(64, boxw/2);

	const fixed_t scale = (vid.width < 640) ? FRACUNIT : FRACUNIT/2;

	for (; i > 0; i--)
	{
		char *msg = CHAT_WordWrap(boxw-4, scale, V_SNAPTOBOTTOM|V_SNAPTOLEFT, chat_mini[i-1]);
		size_t j = 0;
		INT32 linescount = 1;

		for (; msg[j]; j++) // iterate through msg
		{
			if (msg[j] != '\n') // get back down.
				continue;

			linescount++;
		}

		msglines += linescount;

		if (msg)
			Z_Free(msg);
	}

	y = chaty - charheight*(msglines+1);

#ifdef NETSPLITSCREEN
	if (r_splitscreen)
	{
		y -= BASEVIDHEIGHT/2;
		if (r_splitscreen > 1)
			y += 16;
	}
	else
#endif
	{
		y -= (cv_kartspeedometer.value ? 16 : 0);
	}

	i = 0;

	for (; i<=(chat_nummsg_min-1); i++) // iterate through our hot messages
	{
		INT32 timer = ((cv_chattime.value*TICRATE)-chat_timers[i]) - cv_chattime.value*TICRATE+9; // see below...
		INT32 transflag = (timer >= 0 && timer <= 9) ? (timer*V_10TRANS) : 0; // you can make bad jokes out of this one.
		size_t j = 0;
		char *msg = CHAT_WordWrap(boxw-4, scale, V_SNAPTOBOTTOM|V_SNAPTOLEFT, chat_mini[i]); // get the current message, and word wrap it.

		INT32 linescount = 1;

		for (; msg[j]; j++) // iterate through msg
		{
			if (msg[j] != '\n') // get back down.
				continue;

			linescount++;
		}

		if (cv_chatbacktint.value) // on request of wolfy
		{
			INT32 width = V_StringWidth(msg, 0);
			if (vid.width >= 640)
				width /= 2;

			V_DrawFillConsoleMap(
				x-2, y,
				width+4,
				charheight * linescount,
				159|V_SNAPTOBOTTOM|V_SNAPTOLEFT
			);
		}

		V_DrawStringScaled(
			x << FRACBITS,
			y << FRACBITS,
			scale, FRACUNIT, FRACUNIT,
			V_SNAPTOBOTTOM|V_SNAPTOLEFT|transflag,
			NULL,
			HU_FONT,
			msg
		);

		y += charheight * linescount;

		if (msg)
			Z_Free(msg);
	}

	// decrement addy and make that shit smooth:
	addy /= 2;

}

// HU_DrawChatLog

static void HU_drawChatLog(INT32 offset)
{
	const INT32 charheight = (vid.width < 640) ? 12 : 6;
	INT32 boxw = cv_chatwidth.value, boxh = cv_chatheight.value;
	INT32 x = chatx+2, y;
	UINT32 i = 0;
	INT32 chat_topy, chat_bottomy;
	INT32 highlight = V_YELLOWMAP;
	boolean atbottom = false;

	// make sure that our scroll position isn't "illegal";
	if (chat_scroll > chat_maxscroll)
		chat_scroll = chat_maxscroll;

#ifdef NETSPLITSCREEN
	if (r_splitscreen)
	{
		boxh = max(6, boxh/2);
		if (r_splitscreen > 1)
			boxw = max(64, boxw/2);
	}
#endif

	y = chaty - offset*charheight - (chat_scroll*charheight) - boxh*charheight - 12;

#ifdef NETSPLITSCREEN
	if (r_splitscreen)
	{
		y -= BASEVIDHEIGHT/2;

		if (r_splitscreen > 1)
			y += 16;
	}
	else
#endif
	{
		y -= (cv_kartspeedometer.value ? 16 : 0);
	}

	chat_topy = y + chat_scroll*charheight;
	chat_bottomy = boxh*charheight + 2;

	V_DrawFillConsoleMap(chatx, chat_topy, boxw, chat_bottomy, 159|V_SNAPTOBOTTOM|V_SNAPTOLEFT); // log box

	const fixed_t scale = (vid.width < 640) ? FRACUNIT : FRACUNIT/2;

	V_SetClipRect(
		(chatx) << FRACBITS, (chat_topy) << FRACBITS,
		(boxw) << FRACBITS, (chat_bottomy) <<FRACBITS,
		V_SNAPTOBOTTOM|V_SNAPTOLEFT
	);

	chat_bottomy += chat_topy - 2;

	INT32 dy = 0;

	for (i=0; i<chat_nummsg_log; i++) // iterate through our chatlog
	{
		INT32 j = 0, startj = 0;
		char *msg = CHAT_WordWrap(boxw-4, scale, V_SNAPTOBOTTOM|V_SNAPTOLEFT, chat_log[i]); // get the current message, and word wrap it.

		INT32 linescount = 1;

		for (; msg[j]; j++) // iterate through msg
		{
			if (msg[j] != '\n') // get back down.
				continue;

			if (y + dy >= chat_bottomy)
				;
			else if (y + dy + 2 + charheight < chat_topy)
			{
				dy += charheight;

				if (y + dy + 2 + charheight >= chat_topy)
				{
					startj = j;
				}

				continue;
			}

			linescount++;
		}

		if (y + dy < chat_bottomy)
		{
			V_DrawStringScaled(
				(x + 2) << FRACBITS,
				(y + dy + 2) << FRACBITS,
				scale, FRACUNIT, FRACUNIT,
				V_SNAPTOBOTTOM|V_SNAPTOLEFT,
				NULL,
				HU_FONT,
				msg+startj
			);
		}

		dy += charheight * linescount;

		if (msg)
			Z_Free(msg);
	}

	V_ClearClipRect();

	if (((chat_scroll >= chat_maxscroll) || (chat_scrollmedown)) && !(justscrolleddown || justscrolledup || chat_scrolltime)) // was already at the bottom of the page before new maxscroll calculation and was NOT scrolling.
	{
		atbottom = true; // we should scroll
	}
	chat_scrollmedown = false;

	// getmaxscroll through a lazy hack. We do all these loops,
	// so let's not do more loops that are gonna lag the game more. :P
	chat_maxscroll = max(dy / charheight - cv_chatheight.value, 0);

	// if we're not bound by the time, autoscroll for next frame:
	if (atbottom)
		chat_scroll = chat_maxscroll;

	// draw arrows to indicate that we can (or not) scroll.
	// account for Y = -1 offset in tinyfont
	if (chat_scroll > 0)
		V_DrawCharacter(chatx-9, ((justscrolledup) ? (chat_topy-1) : (chat_topy)), V_SNAPTOBOTTOM | V_SNAPTOLEFT | highlight | '\x1A', false); // up arrow
	if (chat_scroll < chat_maxscroll)
		V_DrawCharacter(chatx-9, chat_bottomy-((justscrolleddown) ? 5 : 6), V_SNAPTOBOTTOM | V_SNAPTOLEFT | highlight | '\x1B', false); // down arrow

	justscrolleddown = false;
	justscrolledup = false;
}

//
// HU_DrawChat
//
// Draw chat input
//

static void HU_DrawChat(void)
{
	const INT32 charheight = (vid.width < 640) ? 12 : 6;
	INT32 boxw = cv_chatwidth.value;
	INT32 y = chaty;
	UINT32 i = 0;
	char cflag = '\x80', tflag = '\x80';
	const char *ntalk = "Say: ", *ttalk = "Team: ";
	const char *talk = ntalk;
	const char *mute = "\x86""Chat has been muted.";

#ifdef NETSPLITSCREEN
	if (r_splitscreen)
	{
		y -= BASEVIDHEIGHT/2;
		if (r_splitscreen > 1)
		{
			y += 16;
			boxw = max(64, boxw/2);
		}
	}
	else
#endif
	{
		y -= (cv_kartspeedometer.value ? 16 : 0);
	}

	if (teamtalk)
	{
		talk = ttalk;
#if 0
		if (players[consoleplayer].ctfteam == 1)
			t = '\0x85';  // Red
		else if (players[consoleplayer].ctfteam == 2)
			t = '\0x84'; // Blue
#endif
	}

	typelines = 1;

	const fixed_t scale = (vid.width < 640) ? FRACUNIT : FRACUNIT/2;

	char *msg = NULL;

	if (CHAT_MUTE)
	{
		talk = mute;
		//cflag = '\x86'; // set text in gray if chat is muted.
	}
	else
	{
		msg = CHAT_WordWrap(
			boxw-4,
			scale,
			V_SNAPTOBOTTOM|V_SNAPTOLEFT,
			va("%c%s %c%s%c%c", cflag, talk, tflag, w_chat, '\x80', '_')
		);

		for (; msg[i]; i++) // iterate through msg
		{
			if (msg[i] != '\n') // get back down.
				continue;

			typelines++;
		}

		// This is removed after the fact to not have the newline handling flicker.
		if (i != 0 && hu_tick >= 4)
		{
			msg[i-1] = '\0';
		}
	}

	y -= typelines * charheight;

	V_DrawFillConsoleMap(chatx, y-1, boxw, (typelines*charheight), 159 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);

	V_DrawStringScaled(
		(chatx + 2) << FRACBITS,
		y << FRACBITS,
		scale, FRACUNIT, FRACUNIT,
		V_SNAPTOBOTTOM|V_SNAPTOLEFT,
		NULL,
		HU_FONT,
		msg ? msg : talk
	);

	if (msg)
		Z_Free(msg);

	// handle /pm list. It's messy, horrible and I don't care.
	if (!CHAT_MUTE && !teamtalk && vid.width >= 640 && strnicmp(w_chat, "/pm", 3) == 0) // 320x200 unsupported kthxbai
	{
		INT32 count = 0;
		INT32 p_dispy = chaty - charheight -1;

		if (r_splitscreen)
		{
			p_dispy -= BASEVIDHEIGHT/2;
			if (r_splitscreen > 1)
				p_dispy += 16;
		}
		else
		{
			p_dispy -= (cv_kartspeedometer.value ? 16 : 0);
		}

		i = 0;
		for(i=0; (i<MAXPLAYERS); i++)
		{
			// filter: (code needs optimization pls help I'm bad with C)
			if (w_chat[3])
			{
				char playernum[3];
				UINT32 n;
				// right, that's half important: (w_chat[4] may be a space since /pm0 msg is perfectly acceptable!)
				if ( ( ((w_chat[3] != 0) && ((w_chat[3] < '0') || (w_chat[3] > '9'))) || ((w_chat[4] != 0) && (((w_chat[4] < '0') || (w_chat[4] > '9'))))) && (w_chat[4] != ' '))
					break;

				strncpy(playernum, w_chat+3, 3);
				n = atoi(playernum); // turn that into a number
				// special cases:

				if ((n == 0) && !(w_chat[4] == '0'))
				{
					if (!(i<10))
						continue;
				}
				else if ((n == 1) && !(w_chat[3] == '0'))
				{
					if (!((i == 1) || ((i >= 10) && (i <= 19))))
						continue;
				}
				else if ((n == 2) && !(w_chat[3] == '0'))
				{
					if (!((i == 2) || ((i >= 20) && (i <= 29))))
						continue;
				}
				else if ((n == 3) && !(w_chat[3] == '0'))
				{
					if (!((i == 3) || ((i >= 30) && (i <= 31))))
						continue;
				}
				else	// general case.
				{
					if (i != n)
						continue;
				}
			}

			if (playeringame[i])
			{
				char name[MAXPLAYERNAME+1];
				strlcpy(name, player_names[i], 7); // shorten name to 7 characters.
				V_DrawFillConsoleMap(chatx+ boxw + 2, p_dispy- (6*count), 48, 6, 159 | V_SNAPTOBOTTOM | V_SNAPTOLEFT); // fill it like the chat so the text doesn't become hard to read because of the hud.
				V_DrawSmallString(chatx+ boxw + 4, p_dispy- (6*count), V_SNAPTOBOTTOM|V_SNAPTOLEFT, va("\x82%d\x80 - %s", i, name));
				count++;
			}
		}
		if (count == 0) // no results.
		{
			V_DrawFillConsoleMap(chatx+boxw+2, p_dispy- (6*count), 48, 6, 159 | V_SNAPTOBOTTOM | V_SNAPTOLEFT); // fill it like the chat so the text doesn't become hard to read because of the hud.
			V_DrawSmallString(chatx+boxw+4, p_dispy- (6*count), V_SNAPTOBOTTOM|V_SNAPTOLEFT, "NO RESULT.");
		}
	}

	HU_drawChatLog(typelines-1); // typelines is the # of lines we're typing. If there's more than 1 then the log should scroll up to give us more space.
}


// For anyone who, for some godforsaken reason, likes oldchat.


static void HU_DrawChat_Old(void)
{
	INT32 t = 0, c = 0, y = HU_INPUTY;
	size_t i = 0;
	const char *ntalk = "Say: ", *ttalk = "Say-Team: ";
	const char *talk = ntalk;
	INT32 charwidth = 8 * con_scalefactor; //(fontv[HU_FONT].font['A'-HU_FONTSTART]->width) * con_scalefactor;
	INT32 charheight = 8 * con_scalefactor; //(fontv[HU_FONT].font['A'-HU_FONTSTART]->height) * con_scalefactor;
	if (teamtalk)
	{
		talk = ttalk;
#if 0
		if (players[consoleplayer].ctfteam == 1)
			t = 0x500;  // Red
		else if (players[consoleplayer].ctfteam == 2)
			t = 0x400; // Blue
#endif
	}

	while (talk[i])
	{
		if (talk[i] < HU_FONTSTART)
		{
			++i;
			//charwidth = 4 * con_scalefactor;
		}
		else
		{
			//charwidth = (fontv[HU_FONT].font[talk[i]-HU_FONTSTART]->width) * con_scalefactor;
			V_DrawCharacter(HU_INPUTX + c, y, talk[i++] | cv_constextsize.value | V_NOSCALESTART, true);
		}
		c += charwidth;
	}

	if ((strlen(w_chat) == 0 || c_input == 0) && hu_tick < 4)
		V_DrawCharacter(HU_INPUTX+c, y+2*con_scalefactor, '_' |cv_constextsize.value | V_NOSCALESTART|t, true);

	i = 0;
	while (w_chat[i])
	{

		if (c_input == (i+1) && hu_tick < 4)
		{
			INT32 cursorx = (HU_INPUTX+c+charwidth < vid.width) ? (HU_INPUTX + c + charwidth) : (HU_INPUTX); // we may have to go down.
			INT32 cursory = (cursorx != HU_INPUTX) ? (y) : (y+charheight);
			V_DrawCharacter(cursorx, cursory+2*con_scalefactor, '_' |cv_constextsize.value | V_NOSCALESTART|t, true);
		}

		//Hurdler: isn't it better like that?
		if (w_chat[i] < HU_FONTSTART)
		{
			++i;
			//charwidth = 4 * con_scalefactor;
		}
		else
		{
			//charwidth = (fontv[HU_FONT].font[w_chat[i]-HU_FONTSTART]->width) * con_scalefactor;
			V_DrawCharacter(HU_INPUTX + c, y, w_chat[i++] | cv_constextsize.value | V_NOSCALESTART | t, true);
		}

		c += charwidth;
		if (c >= vid.width)
		{
			c = 0;
			y += charheight;
		}
	}

	if (hu_tick < 4)
		V_DrawCharacter(HU_INPUTX + c, y, '_' | cv_constextsize.value |V_NOSCALESTART|t, true);
}

static void HU_DrawCEcho(void)
{
	INT32 i = 0;
	INT32 y = (BASEVIDHEIGHT/2)-4;
	INT32 pnumlines = 0;

	UINT32 realflags = cechoflags|V_SPLITSCREEN; // requested as part of splitscreen's stuff

	char *line;
	char *echoptr;
	char temp[1024];

	for (i = 0; cechotext[i] != '\0'; ++i)
		if (cechotext[i] == '\\')
			pnumlines++;

	y -= (pnumlines-1)*6;

	// Prevent crashing because I'm sick of this
	if (y < 0)
	{
		CONS_Alert(CONS_WARNING, "CEcho contained too many lines, not displaying\n");
		cechotimer = 0;
		return;
	}

	strcpy(temp, cechotext);
	echoptr = &temp[0];

	while (*echoptr != '\0')
	{
		line = strchr(echoptr, '\\');

		if (line == NULL)
			break;

		*line = '\0';

		V_DrawCenteredString(BASEVIDWIDTH/2, y, realflags, echoptr);
		y += 12;

		echoptr = line;
		echoptr++;
	}
}

static tic_t HU_TitlecardCEchoElapsed(const struct tcecho_state *state)
{
	return max(gametic, state->start) - state->start;
}

static void HU_DrawTitlecardCEcho(size_t num)
{
	const struct tcecho_state *state = &g_tcecho[num];

	tic_t elapsed = HU_TitlecardCEchoElapsed(state);
	UINT8 viewnum = max(1, num) - 1;
	boolean p4 = (num != 0 && r_splitscreen);

	// If the splitscreens were somehow decreased in the
	// middle of drawing this, don't draw it.
	if (viewnum > r_splitscreen)
	{
		return;
	}

	if (elapsed < state->duration)
	{
		INT32 i = 0;
		INT32 x = BASEVIDWIDTH/2;
		INT32 y = BASEVIDHEIGHT/2;
		INT32 pnumlines = 0;
		INT32 timeroffset = 0;
		INT32 fadeout = state->duration * 2 / 3;

		char *line;
		char *echoptr;
		char temp[1024];

		for (i = 0; state->text[i] != '\0'; ++i)
			if (state->text[i] == '\\')
				pnumlines++;

		if (p4)
		{
			if (r_splitscreen == 1) // 2P
			{
				y -= (1 - (viewnum * 2)) * (y / 2);
			}
			else // 3P / 4P
			{
				x -= (1 - ((viewnum % 2) * 2)) * (x / 2);
				y -= (1 - ((viewnum / 2) * 2)) * (y / 2);
			}

			y -= 11 + ((pnumlines-1) * 9);
		}
		else
		{
			y -= 18 + ((pnumlines-1) * 16);
		}

		// Prevent crashing because I'm sick of this
		if (y < 0)
		{
			CONS_Alert(CONS_WARNING, "CEcho contained too many lines, not displaying\n");
			cechotimer = 0;
			return;
		}

		strcpy(temp, state->text);
		echoptr = &temp[0];

		while (*echoptr != '\0')
		{
			INT32 ofs;
			INT32 timer = (INT32)(elapsed - timeroffset);
			
			if (timer <= 0)
				return;	// we don't care.
			
			line = strchr(echoptr, '\\');
			
			if (line == NULL)
				break;

			*line = '\0';
			
			ofs = V_CenteredTitleCardStringOffset(echoptr, p4);
			V_DrawTitleCardString(x - ofs, y, echoptr, 0, false, timer, fadeout, p4);

			y += p4 ? 18 : 32;
			
			// offset the timer for the next line.
			timeroffset += strlen(echoptr);
			
			// set the ptr to the \0 we made and advance it because we don't want an empty string.
			echoptr = line;
			echoptr++;
		}
	}
}

//
// demo info stuff
//
UINT32 hu_demotime;
UINT32 hu_demolap;


//
// Song credits
//
void HU_DrawSongCredits(void)
{
	if (!cursongcredit.text || cursongcredit.trans >= NUMTRANSMAPS) // No def
	{
		return;
	}

	fixed_t x = R_InterpolateFixed(cursongcredit.old_x, cursongcredit.x);
	fixed_t y;

	if (gamestate == GS_INTERMISSION)
	{
		y = (BASEVIDHEIGHT - 13) * FRACUNIT;
	}
	else if (gamestate == GS_MENU)
	{
		y = 30 * FRACUNIT;
	}
	else
	{
		y = (r_splitscreen ? (BASEVIDHEIGHT/2)-4 : 40) * FRACUNIT;
	}

	INT32 bgt = (NUMTRANSMAPS/2) + (cursongcredit.trans / 2);

	if (bgt < NUMTRANSMAPS)
	{
		V_DrawFixedPatch(x, y - (2 * FRACUNIT),
			FRACUNIT, V_SNAPTOLEFT|(bgt<<V_ALPHASHIFT),
			songcreditbg, NULL);
	}

	V_DrawRightAlignedThinStringAtFixed(x, y,
		V_SNAPTOLEFT|(cursongcredit.trans<<V_ALPHASHIFT),
		cursongcredit.text);
}


// Heads up displays drawer, call each frame
//
void HU_Drawer(void)
{
	// Closed chat
	if (!chat_on)
	{
		typelines = 1;
		chat_scrolltime = 0;

		if (!OLDCHAT && cv_consolechat.value < 2 && netgame) // Don't display minimized chat if you set the mode to Window (Hidden)
			HU_drawMiniChat(); // draw messages in a cool fashion.
	}

	if (gamestate != GS_LEVEL)
		goto drawontop;

	// draw multiplayer rankings
	if (hu_showscores)
	{
		if (netgame || multiplayer)
		{
			if (LUA_HudEnabled(hud_rankings))
				HU_DrawRankings();
			if (renderisnewtic)
			{
				LUA_HUD_ClearDrawList(luahuddrawlist_scores);
				LUA_HookHUD(luahuddrawlist_scores, HUD_HOOK(scores));
			}
			LUA_HUD_DrawList(luahuddrawlist_scores);
		}
	}

	// draw desynch text
	if (hu_redownloadinggamestate)
	{
		char resynch_text[14];
		UINT32 i;

		// Animate the dots
		strcpy(resynch_text, "Resynching");
		for (i = 0; i < (resynch_ticker / 16) % 4; i++)
			strcat(resynch_text, ".");

		V_DrawCenteredString(BASEVIDWIDTH/2, 180, V_YELLOWMAP, resynch_text);
	}

drawontop:
	// Opened chat
	if (chat_on)
	{
		if (!OLDCHAT)
			HU_DrawChat();
		else
			HU_DrawChat_Old();
	}

	if (cechotimer)
		HU_DrawCEcho();

	const struct tcecho_state *firststate = &g_tcecho[0];

	// Server messages overwrite player-specific messages
	if (HU_TitlecardCEchoElapsed(firststate) < firststate->duration)
	{
		HU_DrawTitlecardCEcho(0);
	}
	else
	{
		size_t i;

		for (i = 1; i < NUM_TCECHO_STATES; ++i)
		{
			HU_DrawTitlecardCEcho(i);
		}
	}
}

//======================================================================
//                 HUD MESSAGES CLEARING FROM SCREEN
//======================================================================

// Clear old messages from the borders around the view window
// (only for reduced view, refresh the borders when needed)
//
// startline: y coord to start clear,
// clearlines: how many lines to clear.
//
static INT32 oldclearlines;

void HU_Erase(void)
{
	INT32 topline, bottomline;
	INT32 y, yoffset;

#ifdef HWRENDER
	// clear hud msgs on double buffer (OpenGL mode)
	boolean secondframe;
	static INT32 secondframelines;
#endif

	if (con_clearlines == oldclearlines && !con_hudupdate && !chat_on)
		return;

#ifdef HWRENDER
	// clear the other frame in double-buffer modes
	secondframe = (con_clearlines != oldclearlines);
	if (secondframe)
		secondframelines = oldclearlines;
#endif

	// clear the message lines that go away, so use _oldclearlines_
	bottomline = oldclearlines;
	oldclearlines = con_clearlines;
	if (chat_on && OLDCHAT)
		if (bottomline < 8)
			bottomline = 8; // only do it for consolechat. consolechat is gay.

	if (automapactive || viewwindowx == 0) // hud msgs don't need to be cleared
		return;

	// software mode copies view border pattern & beveled edges from the backbuffer
	if (rendermode == render_soft)
	{
		topline = 0;
		for (y = topline, yoffset = y*vid.width; y < bottomline; y++, yoffset += vid.width)
		{
			if (y < viewwindowy || y >= viewwindowy + viewheight)
				R_VideoErase(yoffset, vid.width); // erase entire line
			else
			{
				R_VideoErase(yoffset, viewwindowx); // erase left border
				// erase right border
				R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
			}
		}
		con_hudupdate = false; // if it was set..
	}
#ifdef HWRENDER
	else if (rendermode != render_none)
	{
		// refresh just what is needed from the view borders
		HWR_DrawViewBorder(secondframelines);
		con_hudupdate = secondframe;
	}
#endif
}

//======================================================================
//                   IN-LEVEL MULTIPLAYER RANKINGS
//======================================================================

static int
Ping_gfx_num (int lag)
{
	if (lag <= 2)
		return 0;
	else if (lag <= 4)
		return 1;
	else if (lag <= 7)
		return 2;
	else if (lag <= 10)
		return 3;
	else
		return 4;
}

static int
Ping_gfx_color (int lag)
{
	if (lag <= 2)
		return SKINCOLOR_JAWZ;
	else if (lag <= 4)
		return SKINCOLOR_MINT;
	else if (lag <= 7)
		return SKINCOLOR_GOLD;
	else if (lag <= 10)
		return SKINCOLOR_RASPBERRY;
	else
		return SKINCOLOR_MAGENTA;
}

static const UINT8 *
Ping_gfx_colormap (UINT32 ping, UINT32 lag)
{
	const UINT8 *colormap = R_GetTranslationColormap(TC_RAINBOW, Ping_gfx_color(lag), GTC_CACHE);

	if (servermaxping && ping > servermaxping && hu_tick < 4)
	{
		// flash ping red if too high
		colormap = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_WHITE, GTC_CACHE);
	}

	return colormap;
}

static UINT32
Ping_conversion (UINT32 lag)
{
	if (cv_pingmeasurement.value)
	{
		lag = (INT32)(lag * (1000.00f / TICRATE));
	}

	return lag;
}

static int
PL_gfx_color (int pl)
{
	if (pl <= 2)
		return 72;
	else if (pl <= 4)
		return 54;
	else if (pl <= 6)
		return 35;
	else
		return 181;
}

//
// HU_drawPing
//
void HU_drawPing(fixed_t x, fixed_t y, UINT32 ping, UINT32 mindelay, UINT32 pl, INT32 flags, SINT8 toside)
{
	const UINT8 *colormap = NULL;
	INT32 measureid = cv_pingmeasurement.value ? 1 : 0;
	patch_t *gfx; // gfx to draw
	fixed_t x2, y2;
	UINT32 lag = max(ping, mindelay);

	x2 = x;
	y2 = y + FRACUNIT;

	if (toside == 0)
	{
		if (measureid == 0)
		{
			x2 += (10 * FRACUNIT);
		}

		y2 += (8 * FRACUNIT);
	}
	else if (toside > 0)
	{
		// V_DrawPingNum
		const fixed_t w = (fontv[PINGNUM_FONT].font[0]->width) * FRACUNIT - FRACUNIT;
		x2 += (16 * FRACUNIT) + (int)(log(Ping_conversion(lag)) / log(10)) * w;

		if (measureid == 0)
		{
			x2 += (4 * FRACUNIT);
		}
	}
	else if (toside < 0)
	{
		if (measureid == 1)
		{
			x2 -= (pingmeasure[measureid]->width * FRACUNIT);
		}
	}

	if (ping <= mindelay)
	{
		gfx = pinglocal[0];
	}
	else
	{
		gfx = pinggfx[Ping_gfx_num(ping)];
	}

	if (pl)
	{
		V_DrawFill(
			-gfx->leftoffset + x/FRACUNIT + 2 - 1,
			-gfx->topoffset + y/FRACUNIT - 1,
			gfx->width + 2,
			gfx->height + 2,
			PL_gfx_color(pl) | flags
		);
	}

	V_DrawFixedPatch(
		x + (2 * FRACUNIT),
		y,
		FRACUNIT, flags,
		gfx,
		NULL
	);

	if (measureid == 1)
	{
		V_DrawFixedPatch(
			x2,
			y2,
			FRACUNIT, flags,
			pingmeasure[measureid],
			NULL
		);
	}

	colormap = Ping_gfx_colormap(ping, lag);

	x2 = V_DrawPingNum(
		x2,
		y2,
		flags, Ping_conversion(lag),
		colormap
	);

	if (measureid == 0)
	{
		V_DrawFixedPatch(
			x2 + ((1 - pingmeasure[measureid]->width) * FRACUNIT),
			y2,
			FRACUNIT, flags,
			pingmeasure[measureid],
			NULL
		);
	}
}

void
HU_drawMiniPing (INT32 x, INT32 y, UINT32 ping, UINT32 mindelay, INT32 flags)
{
	patch_t *patch;
	INT32 w = BASEVIDWIDTH;

	if (r_splitscreen > 1)
	{
		w /= 2;
	}

	if (ping <= mindelay)
		patch = pinglocal[1]; // stone shoe
	else
		patch = mping[Ping_gfx_num(ping)];

	if (( flags & V_SNAPTORIGHT ))
		x += ( w - SHORT (patch->width) );

	V_DrawScaledPatch(x, y, flags, patch);
}

//
// HU_DrawSpectatorTicker
//
static inline void HU_DrawSpectatorTicker(void)
{
	INT32 i;
	INT32 length = 0, height = 174;
	INT32 totallength = 0, templength = -8;
	INT32 dupadjust = (vid.width/vid.dupx), duptweak = (dupadjust - BASEVIDWIDTH)/2;

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && players[i].spectator)
			totallength += (signed)strlen(player_names[i]) * 8 + 16;

	length -= (leveltime % (totallength + dupadjust+8));
	length += dupadjust;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && players[i].spectator)
		{
			char *pos;
			char initial[MAXPLAYERNAME+1];
			char current[MAXPLAYERNAME+1];
			INT32 len;

			len = ((signed)strlen(player_names[i]) * 8 + 16);

			strcpy(initial, player_names[i]);
			pos = initial;

			if (length >= -len)
			{
				if (length < -8)
				{
					UINT8 eatenchars = (UINT8)(abs(length) / 8);

					if (eatenchars <= strlen(initial))
					{
						// Eat one letter off the left side,
						// then compensate the drawing position.
						pos += eatenchars;
						strcpy(current, pos);
						templength = ((length + 8) % 8);
					}
					else
					{
						strcpy(current, " ");
						templength = length;
					}
				}
				else
				{
					strcpy(current, initial);
					templength = length;
				}

				V_DrawString(templength - duptweak, height, V_TRANSLUCENT, current);
			}

			if ((length += len) >= dupadjust+8)
				break;
		}
	}
}

//
// HU_DrawRankings
//
static void HU_DrawRankings(void)
{
	INT32 i, j, hilicol = highlightflags;
	boolean timedone = false, pointsdone = false;

	if (!automapactive)
		V_DrawFadeScreen(0xFF00, 16); // A little more readable, and prevents cheating the fades under other circumstances.

	// draw the current gametype in the lower right
	if (grandprixinfo.gp == true)
		V_DrawString(4, 188, hilicol|V_SNAPTOBOTTOM|V_SNAPTOLEFT, "Grand Prix");
	else if (battleprisons)
		V_DrawString(4, 188, hilicol|V_SNAPTOBOTTOM|V_SNAPTOLEFT, "Prisons");
	else if (gametype >= 0 && gametype < numgametypes)
		V_DrawString(4, 188, hilicol|V_SNAPTOBOTTOM|V_SNAPTOLEFT, gametypes[gametype]->name);

	// Left hand side
	const boolean roundqueueinaction = (roundqueue.position > 0 && roundqueue.position <= roundqueue.size);

	if (roundqueueinaction
		&& roundqueue.entries[roundqueue.position-1].overridden == true)
	{
		V_DrawCenteredString(64, 8, 0, "ROUND");
		V_DrawCenteredString(64, 16, hilicol, "???");
	}
	else if (grandprixinfo.gp == true && grandprixinfo.eventmode != GPEVENT_NONE)
	{
		const char *roundstr = NULL;
		V_DrawCenteredString(64, 8, 0, "ROUND");
		switch (grandprixinfo.eventmode)
		{
			case GPEVENT_SPECIAL:
				roundstr = "SPECIAL";
				break;
			default:
				roundstr = "BONUS";
				break;
		}
		V_DrawCenteredString(64, 16, hilicol, roundstr);
	}
	else if (roundqueueinaction)
	{
		V_DrawCenteredString(64, 8, 0, "ROUND");
		V_DrawCenteredString(64, 16, hilicol, va("%d", roundqueue.roundnum));
	}
	else if ((gametyperules & GTR_TIMELIMIT) && timelimitintics > 0)
	{
		UINT32 timeval = (timelimitintics + starttime + 1 - leveltime);
		if (timeval > timelimitintics+1)
			timeval = timelimitintics+1;
		timeval /= TICRATE;

		if (leveltime <= (timelimitintics + starttime))
		{
			V_DrawCenteredString(64, 8, 0, "TIME LEFT");
			V_DrawCenteredString(64, 16, hilicol, va("%u", timeval));
		}

		// overtime
		if (!players[consoleplayer].exiting && (leveltime > (timelimitintics + starttime + TICRATE/2)) && cv_overtime.value)
		{
			V_DrawCenteredString(64, 8, 0, "TIME LEFT");
			V_DrawCenteredString(64, 16, hilicol, "OVERTIME");
		}

		timedone = true;
	}
	else if ((gametyperules & GTR_POINTLIMIT) && g_pointlimit > 0)
	{
		V_DrawCenteredString(64, 8, 0, "POINT LIMIT");
		V_DrawCenteredString(64, 16, hilicol, va("%d", g_pointlimit));
		pointsdone = true;
	}
	else if (gametyperules & GTR_CIRCUIT)
	{
		V_DrawCenteredString(64, 8, 0, "LAPS");
		V_DrawCenteredString(64, 16, hilicol, va("%d", numlaps));
	}

	// Right hand side
	if (battleprisons == true)
	{
		if (numtargets < maptargets)
		{
			V_DrawCenteredString(256, 8, 0, "PRISONS");
			V_DrawCenteredString(256, 16, hilicol, va("%d", maptargets - numtargets));
		}
	}
	else if (!timedone && (gametyperules & GTR_TIMELIMIT) && timelimitintics > 0)
	{
		UINT32 timeval = (timelimitintics + starttime + 1 - leveltime);
		if (timeval > timelimitintics+1)
			timeval = timelimitintics+1;
		timeval /= TICRATE;

		if (leveltime <= (timelimitintics + starttime))
		{
			V_DrawCenteredString(256, 8, 0, "TIME LEFT");
			V_DrawCenteredString(256, 16, hilicol, va("%u", timeval));
		}

		// overtime
		if (!players[consoleplayer].exiting && (leveltime > (timelimitintics + starttime + TICRATE/2)) && cv_overtime.value)
		{
			V_DrawCenteredString(256, 8, 0, "TIME LEFT");
			V_DrawCenteredString(256, 16, hilicol, "OVERTIME");
		}
	}
	else if (!pointsdone && (gametyperules & GTR_POINTLIMIT) && g_pointlimit > 0)
	{
		V_DrawCenteredString(256, 8, 0, "POINT LIMIT");
		V_DrawCenteredString(256, 16, hilicol, va("%d", g_pointlimit));
	}
	else if (gametypes[gametype]->speed == KARTSPEED_AUTO)
	{
		V_DrawCenteredString(256, 8, 0, "GAME SPEED");
		V_DrawCenteredString(256, 16, hilicol, (cv_4thgear.value) ? va("4th Gear") : kartspeed_cons_t[1+gamespeed].strvalue);
	}

	boolean completed[MAXPLAYERS];
	y_data_t standings;

	memset(completed, 0, sizeof (completed));
	memset(&standings, 0, sizeof (standings));

	UINT8 numplayersingame = 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator || !players[i].mo)
		{
			completed[i] = true;
			continue;
		}

		numplayersingame++;
	}

	for (j = 0; j < numplayersingame; j++)
	{
		UINT8 lowestposition = MAXPLAYERS+1;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (completed[i])
				continue;

			if (players[i].position >= lowestposition)
				continue;

			standings.num[standings.numplayers] = i;
			lowestposition = players[i].position;
		}

		i = standings.num[standings.numplayers];

		completed[i] = true;

		standings.character[standings.numplayers] = players[i].skin;
		standings.color[standings.numplayers] = players[i].skincolor;
		standings.pos[standings.numplayers] = players[i].position;

#define strtime standings.strval[standings.numplayers]

		standings.val[standings.numplayers] = 0;
		strtime[0] = '\0';

		if (players[i].pflags & PF_NOCONTEST)
		{
			standings.val[standings.numplayers] = (UINT32_MAX-1);
			STRBUFCPY(strtime, "RETIRED.");
		}
		else if ((gametyperules & GTR_CIRCUIT))
		{
			standings.val[standings.numplayers] = players[i].laps;

			if (players[i].exiting)
			{
				snprintf(strtime, sizeof strtime, "%i'%02i\"%02i", G_TicsToMinutes(players[i].realtime, true),
				G_TicsToSeconds(players[i].realtime), G_TicsToCentiseconds(players[i].realtime));
			}
			else if (numlaps > 1)
			{
				snprintf(strtime, sizeof strtime, "Lap %d", players[i].laps);
			}
		}
		else if ((gametyperules & GTR_POINTLIMIT))
		{
			standings.val[standings.numplayers] = players[i].roundscore;
			snprintf(strtime, sizeof strtime, "%d", standings.val[standings.numplayers]);
		}

#undef strtime

		standings.numplayers++;
	}

	// Returns early if there's no players to draw
	Y_PlayerStandingsDrawer(&standings, 0);

	// draw spectators in a ticker across the bottom
	if (netgame && G_GametypeHasSpectators())
		HU_DrawSpectatorTicker();
}

// Interface to CECHO settings for the outside world, avoiding the
// expense (and security problems) of going via the console buffer.
void HU_ClearCEcho(void)
{
	cechotimer = 0;
}

void HU_SetCEchoDuration(INT32 seconds)
{
	cechoduration = seconds * TICRATE;
}

void HU_SetCEchoFlags(INT32 flags)
{
	// Don't allow cechoflags to contain any bits in V_PARAMMASK
	cechoflags = (flags & ~V_PARAMMASK);
}

void HU_DoCEcho(const char *msg)
{
	I_OutputMsg("%s\n", msg); // print to log

	strncpy(cechotext, msg, sizeof(cechotext));
	strncat(cechotext, "\\", sizeof(cechotext) - strlen(cechotext) - 1);
	cechotext[sizeof(cechotext) - 1] = '\0';
	cechotimer = cechoduration;
}

// Simply set the timer to 0 to clear it.
// No need to bother clearing the buffer or anything.
void HU_ClearTitlecardCEcho(void)
{
	size_t i;

	for (i = 0; i < NUM_TCECHO_STATES; ++i)
	{
		g_tcecho[i].duration = 0;
	}
}

// Similar but for titlecard CEcho and also way less convoluted because I have no clue whatever the fuck they were trying above.
void HU_DoTitlecardCEchoForDuration(player_t *player, const char *msg, boolean interrupt, tic_t duration)
{
	if (player && !P_IsDisplayPlayer(player))
	{
		return;
	}

	struct tcecho_state *state = &g_tcecho[0];

	if (player)
	{
		state = &g_tcecho[1 + G_PartyPosition(player - players)];
	}

	// If this message should not interrupt an existing
	// message. Check if another message is already running.
	if (!interrupt && HU_TitlecardCEchoElapsed(state) < state->duration)
	{
		return;
	}

	I_OutputMsg("%s\n", msg);	// print to log

	strncpy(state->text, msg, sizeof(state->text));
	strncat(state->text, "\\", sizeof(state->text) - strlen(state->text) - 1);
	state->text[sizeof(state->text) - 1] = '\0';
	state->start = gametic;
	state->duration = duration ? duration : TICRATE*6 + strlen(state->text);
}

void HU_DoTitlecardCEcho(player_t *player, const char *msg, boolean interrupt)
{
	HU_DoTitlecardCEchoForDuration(player, msg, interrupt, 0u);
}
