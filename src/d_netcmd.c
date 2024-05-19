// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netcmd.c
/// \brief host/client network commands
///        commands are executed through the command buffer
///	       like console commands, other miscellaneous commands (at the end)

#include "doomdef.h"

#include "console.h"
#include "command.h"
#include "i_time.h"
#include "i_system.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "g_input.h"
#include "k_menu.h"
#include "r_local.h"
#include "r_skins.h"
#include "p_local.h"
#include "p_setup.h"
#include "s_sound.h"
#include "i_sound.h"
#include "m_misc.h"
#include "am_map.h"
#include "byteptr.h"
#include "d_netfil.h"
#include "p_spec.h"
#include "m_cheat.h"
#include "d_clisrv.h"
#include "d_net.h"
#include "v_video.h"
#include "d_main.h"
#include "m_random.h"
#include "f_finale.h"
#include "filesrch.h"
#include "mserv.h"
#include "z_zone.h"
#include "lua_script.h"
#include "lua_hook.h"
#include "m_cond.h"
#include "m_anigif.h"
#include "md5.h"

// SRB2kart
#include "k_kart.h"
#include "k_battle.h"
#include "k_pwrlv.h"
#include "y_inter.h"
#include "k_color.h"
#include "k_respawn.h"
#include "k_grandprix.h"
#include "k_follower.h"
#include "doomstat.h"
#include "deh_tables.h"
#include "m_perfstats.h"
#include "k_specialstage.h"
#include "k_race.h"
#include "g_party.h"
#include "k_vote.h"
#include "k_zvote.h"
#include "k_bot.h"
#include "k_powerup.h"
#include "k_roulette.h"
#include "k_bans.h"
#include "k_director.h"
#include "k_credits.h"

#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
#include "m_avrecorder.h"
#endif

#ifdef HAVE_DISCORDRPC
#include "discord.h"
#endif

// ------
// protos
// ------

static void Got_NameAndColor(const UINT8 **cp, INT32 playernum);
static void Got_WeaponPref(const UINT8 **cp, INT32 playernum);
static void Got_PartyInvite(const UINT8 **cp, INT32 playernum);
static void Got_AcceptPartyInvite(const UINT8 **cp, INT32 playernum);
static void Got_CancelPartyInvite(const UINT8 **cp, INT32 playernum);
static void Got_LeaveParty(const UINT8 **cp, INT32 playernum);
static void Got_Mapcmd(const UINT8 **cp, INT32 playernum);
static void Got_ExitLevelcmd(const UINT8 **cp, INT32 playernum);
static void Got_SetupVotecmd(const UINT8 **cp, INT32 playernum);
static void Got_ModifyVotecmd(const UINT8 **cp, INT32 playernum);
static void Got_PickVotecmd(const UINT8 **cp, INT32 playernum);
static void Got_RequestAddfilecmd(const UINT8 **cp, INT32 playernum);
static void Got_Addfilecmd(const UINT8 **cp, INT32 playernum);
static void Got_Pause(const UINT8 **cp, INT32 playernum);
static void Got_RandomSeed(const UINT8 **cp, INT32 playernum);
static void Got_RunSOCcmd(const UINT8 **cp, INT32 playernum);
static void Got_Teamchange(const UINT8 **cp, INT32 playernum);
static void Got_Clearscores(const UINT8 **cp, INT32 playernum);
static void Got_DiscordInfo(const UINT8 **cp, INT32 playernum);
static void Got_ScheduleTaskcmd(const UINT8 **cp, INT32 playernum);
static void Got_ScheduleClearcmd(const UINT8 **cp, INT32 playernum);
static void Got_Automatecmd(const UINT8 **cp, INT32 playernum);
static void Got_RequestMapQueuecmd(const UINT8 **cp, INT32 playernum);
static void Got_MapQueuecmd(const UINT8 **cp, INT32 playernum);
static void Got_Cheat(const UINT8 **cp, INT32 playernum);

static void Command_Playdemo_f(void);
static void Command_Timedemo_f(void);
static void Command_Stopdemo_f(void);
static void Command_StartMovie_f(void);
static void Command_StartLossless_f(void);
static void Command_StopMovie_f(void);
static void Command_Map_f(void);
static void Command_RandomMap(void);
static void Command_RestartLevel(void);
static void Command_QueueMap_f(void);
static void Command_ResetCamera_f(void);

static void Command_View_f (void);
static void Command_SetViews_f(void);

static void Command_Invite_f(void);
static void Command_CancelInvite_f(void);
static void Command_AcceptInvite_f(void);
static void Command_RejectInvite_f(void);
static void Command_LeaveParty_f(void);

static void Command_Addfile(void);
static void Command_ListWADS_f(void);
static void Command_ListDoomednums_f(void);
static void Command_cxdiag_f(void);
static void Command_ListUnusedSprites_f(void);
static void Command_RunSOC(void);
static void Command_Pause(void);

static void Command_Version_f(void);
#ifdef UPDATE_ALERT
static void Command_ModDetails_f(void);
#endif
static void Command_ShowGametype_f(void);
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void);
static void Command_Playintro_f(void);

static void Command_Displayplayer_f(void);

static void Command_ExitLevel_f(void);
static void Command_Showmap_f(void);
static void Command_Mapmd5_f(void);

static void Command_Teamchange_f(void);
static void Command_Teamchange2_f(void);
static void Command_Teamchange3_f(void);
static void Command_Teamchange4_f(void);

static void Command_ServerTeamChange_f(void);

static void Command_Clearscores_f(void);

// Remote Administration
static void Command_Changepassword_f(void);
static void Command_Login_f(void);
static void Got_Verification(const UINT8 **cp, INT32 playernum);
static void Got_Removal(const UINT8 **cp, INT32 playernum);
static void Command_Verify_f(void);
static void Command_RemoveAdmin_f(void);
static void Command_MotD_f(void);
static void Got_MotD_f(const UINT8 **cp, INT32 playernum);

static void Command_ShowScores_f(void);
static void Command_ShowTime_f(void);

static void Command_Isgamemodified_f(void);
#ifdef _DEBUG
static void Command_Togglemodified_f(void);
static void Command_Archivetest_f(void);
#endif

static void Command_KartGiveItem_f(void);

static void Command_Schedule_Add(void);
static void Command_Schedule_Clear(void);
static void Command_Schedule_List(void);

static void Command_Automate_Set(void);

static void Command_Eval(void);

static void Command_WriteTextmap(void);

#ifdef DEVELOP
static void Command_FastForward(void);
#endif

// =========================================================================
//                           CLIENT VARIABLES
// =========================================================================

UINT16 lastgoodcolor[MAXSPLITSCREENPLAYERS] = {SKINCOLOR_NONE, SKINCOLOR_NONE, SKINCOLOR_NONE, SKINCOLOR_NONE};

CV_PossibleValue_t kartdebugitem_cons_t[] =
{
#define FOREACH( name, n ) { n, #name }
	KART_ITEM_ITERATOR,
#undef  FOREACH
	{POWERUP_SMONITOR, "SMonitor"},
	{POWERUP_BARRIER, "Barrier"},
	{POWERUP_BUMPER, "Bumper"},
	{POWERUP_BADGE, "Badge"},
	{POWERUP_SUPERFLICKY, "SuperFlicky"},
	{POWERUP_POINTS, "Points"},
	{0}
};

CV_PossibleValue_t capsuletest_cons_t[] = {
	{CV_CAPSULETEST_OFF, "Off"},
	{CV_CAPSULETEST_MULTIPLAYER, "Multiplayer"},
	{CV_CAPSULETEST_TIMEATTACK, "TimeAttack"},
	{0, NULL}};

void CapsuleTest_OnChange(void);
void CapsuleTest_OnChange(void)
{
	if (gamestate == GS_LEVEL)
		CONS_Printf("Level must be restarted for capsuletest to have effect.\n");
}

CV_PossibleValue_t pointlimit_cons_t[] = {{1, "MIN"}, {MAXSCORE, "MAX"}, {0, "None"}, {-1, "Auto"}, {0, NULL}};
CV_PossibleValue_t numlaps_cons_t[] = {{0, "MIN"}, {MAX_LAPS, "MAX"}, {-1, "Map default"}, {0, NULL}};

CV_PossibleValue_t perfstats_cons_t[] = {
	{PS_OFF, "Off"},
	{PS_RENDER, "Rendering"},
	{PS_LOGIC, "Logic"},
	{PS_BOT, "Bots"},
	{PS_THINKFRAME, "ThinkFrame"},
	{0, NULL}
};

char timedemo_name[256];
boolean timedemo_csv;
char timedemo_csv_id[256];
boolean timedemo_quit;

INT16 gametype = GT_RACE;
INT16 g_lastgametype = GT_RACE;
INT16 numgametypes = GT_FIRSTFREESLOT;

boolean forceresetplayers = false;
boolean deferencoremode = false;
boolean forcespecialstage = false;

UINT8 splitscreen = 0;
INT32 adminplayers[MAXPLAYERS];

// Scheduled commands.
scheduleTask_t **schedule = NULL;
size_t schedule_size = 0;
size_t schedule_len = 0;

// Automation commands
char *automate_commands[AEV__MAX];

const char *automate_names[AEV__MAX] =
{
	"RoundStart", // AEV_ROUNDSTART
	"IntermissionStart", // AEV_INTERMISSIONSTART
	"VoteStart", // AEV_VOTESTART
	"QueueStart", // AEV_QUEUESTART
	"QueueEnd", // AEV_QUEUEEND
};

UINT32 livestudioaudience_timer = 90;

/// \warning Keep this up-to-date if you add/remove/rename net text commands
const char *netxcmdnames[MAXNETXCMD - 1] =
{
	"NAMEANDCOLOR", // XD_NAMEANDCOLOR
	"WEAPONPREF", // XD_WEAPONPREF
	"KICK", // XD_KICK
	"NETVAR", // XD_NETVAR
	"SAY", // XD_SAY
	"MAP", // XD_MAP
	"EXITLEVEL", // XD_EXITLEVEL
	"ADDFILE", // XD_ADDFILE
	"PAUSE", // XD_PAUSE
	"ADDPLAYER", // XD_ADDPLAYER
	"TEAMCHANGE", // XD_TEAMCHANGE
	"CLEARSCORES", // XD_CLEARSCORES
	"VERIFIED", // XD_VERIFIED
	"RANDOMSEED", // XD_RANDOMSEED
	"RUNSOC", // XD_RUNSOC
	"REQADDFILE", // XD_REQADDFILE
	"SETMOTD", // XD_SETMOTD
	"DEMOTED", // XD_DEMOTED
	"LUACMD", // XD_LUACMD
	"LUAVAR", // XD_LUAVAR
	"LUAFILE", // XD_LUAFILE

	// SRB2Kart
	"SETUPVOTE", // XD_SETUPVOTE
	"MODIFYVOTE", // XD_MODIFYVOTE
	"PICKVOTE", // XD_PICKVOTE
	"REMOVEPLAYER", // XD_REMOVEPLAYER
	"PARTYINVITE", // XD_PARTYINVITE
	"ACCEPTPARTYINVITE", // XD_ACCEPTPARTYINVITE
	"LEAVEPARTY", // XD_LEAVEPARTY
	"CANCELPARTYINVITE", // XD_CANCELPARTYINVITE
	"CHEAT", // XD_CHEAT
	"ADDBOT", // XD_ADDBOT
	"DISCORD", // XD_DISCORD
	"PLAYSOUND", // XD_PLAYSOUND
	"SCHEDULETASK", // XD_SCHEDULETASK
	"SCHEDULECLEAR", // XD_SCHEDULECLEAR
	"AUTOMATE", // XD_AUTOMATE
	"REQMAPQUEUE", // XD_REQMAPQUEUE
	"MAPQUEUE", // XD_MAPQUEUE
	"CALLZVOTE", // XD_CALLZVOTE
	"SETZVOTE", // XD_SETZVOTE
};

// =========================================================================
//                           SERVER STARTUP
// =========================================================================

/** Registers server commands and variables.
  * Anything required by a dedicated server should probably go here.
  *
  * \sa D_RegisterClientCommands
  */
void D_RegisterServerCommands(void)
{
	INT32 i;

	Forceskin_cons_t[0].value = -1;
	Forceskin_cons_t[0].strvalue = "Off";

	// Set the values to 0/NULL, it will be overwritten later when a skin is assigned to the slot.
	for (i = 1; i < MAXSKINS; i++)
	{
		Forceskin_cons_t[i].value = 0;
		Forceskin_cons_t[i].strvalue = NULL;
	}

	RegisterNetXCmd(XD_NAMEANDCOLOR, Got_NameAndColor);
	RegisterNetXCmd(XD_WEAPONPREF, Got_WeaponPref);
	RegisterNetXCmd(XD_PARTYINVITE, Got_PartyInvite);
	RegisterNetXCmd(XD_ACCEPTPARTYINVITE, Got_AcceptPartyInvite);
	RegisterNetXCmd(XD_CANCELPARTYINVITE, Got_CancelPartyInvite);
	RegisterNetXCmd(XD_LEAVEPARTY, Got_LeaveParty);
	RegisterNetXCmd(XD_MAP, Got_Mapcmd);
	RegisterNetXCmd(XD_EXITLEVEL, Got_ExitLevelcmd);
	RegisterNetXCmd(XD_ADDFILE, Got_Addfilecmd);
	RegisterNetXCmd(XD_REQADDFILE, Got_RequestAddfilecmd);
	RegisterNetXCmd(XD_PAUSE, Got_Pause);
	RegisterNetXCmd(XD_RUNSOC, Got_RunSOCcmd);
	RegisterNetXCmd(XD_LUACMD, Got_Luacmd);
	RegisterNetXCmd(XD_LUAFILE, Got_LuaFile);

	RegisterNetXCmd(XD_SETUPVOTE, Got_SetupVotecmd);
	RegisterNetXCmd(XD_MODIFYVOTE, Got_ModifyVotecmd);
	RegisterNetXCmd(XD_PICKVOTE, Got_PickVotecmd);

	RegisterNetXCmd(XD_SCHEDULETASK, Got_ScheduleTaskcmd);
	RegisterNetXCmd(XD_SCHEDULECLEAR, Got_ScheduleClearcmd);
	RegisterNetXCmd(XD_AUTOMATE, Got_Automatecmd);
	RegisterNetXCmd(XD_REQMAPQUEUE, Got_RequestMapQueuecmd);
	RegisterNetXCmd(XD_MAPQUEUE, Got_MapQueuecmd);

	RegisterNetXCmd(XD_CHEAT, Got_Cheat);

	// Remote Administration
	COM_AddCommand("password", Command_Changepassword_f);
	COM_AddCommand("login", Command_Login_f); // useful in dedicated to kick off remote admin
	COM_AddCommand("promote", Command_Verify_f);
	RegisterNetXCmd(XD_VERIFIED, Got_Verification);
	COM_AddCommand("demote", Command_RemoveAdmin_f);
	RegisterNetXCmd(XD_DEMOTED, Got_Removal);

	COM_AddCommand("motd", Command_MotD_f);
	RegisterNetXCmd(XD_SETMOTD, Got_MotD_f); // For remote admin

	RegisterNetXCmd(XD_TEAMCHANGE, Got_Teamchange);
	COM_AddCommand("serverchangeteam", Command_ServerTeamChange_f);

	RegisterNetXCmd(XD_CLEARSCORES, Got_Clearscores);
	COM_AddDebugCommand("clearscores", Command_Clearscores_f);
	COM_AddCommand("map", Command_Map_f);
	COM_AddDebugCommand("randommap", Command_RandomMap);
	COM_AddCommand("restartlevel", Command_RestartLevel);
	COM_AddCommand("queuemap", Command_QueueMap_f);

	COM_AddCommand("exitgame", Command_ExitGame_f);
	COM_AddCommand("retry", Command_Retry_f);
	COM_AddCommand("exitlevel", Command_ExitLevel_f);
	COM_AddDebugCommand("showmap", Command_Showmap_f);
	COM_AddCommand("mapmd5", Command_Mapmd5_f);

	COM_AddCommand("addfile", Command_Addfile);
	COM_AddDebugCommand("listwad", Command_ListWADS_f);
	COM_AddDebugCommand("listmapthings", Command_ListDoomednums_f);
	COM_AddDebugCommand("cxdiag", Command_cxdiag_f);
	COM_AddCommand("listunusedsprites", Command_ListUnusedSprites_f);

	COM_AddCommand("runsoc", Command_RunSOC);
	COM_AddCommand("pause", Command_Pause);

	COM_AddDebugCommand("gametype", Command_ShowGametype_f);
	COM_AddDebugCommand("version", Command_Version_f);
#ifdef UPDATE_ALERT
	COM_AddCommand("mod_details", Command_ModDetails_f);
#endif
	COM_AddCommand("quit", Command_Quit_f);

	COM_AddCommand("saveconfig", Command_SaveConfig_f);
	COM_AddCommand("loadconfig", Command_LoadConfig_f);
	COM_AddCommand("changeconfig", Command_ChangeConfig_f);
	COM_AddDebugCommand("isgamemodified", Command_Isgamemodified_f); // test
	COM_AddDebugCommand("showscores", Command_ShowScores_f);
	COM_AddDebugCommand("showtime", Command_ShowTime_f);
#ifdef _DEBUG
	COM_AddDebugCommand("togglemodified", Command_Togglemodified_f);
	COM_AddDebugCommand("archivetest", Command_Archivetest_f);
#endif

	COM_AddDebugCommand("downloads", Command_Downloads_f);

	COM_AddDebugCommand("give", Command_KartGiveItem_f);
	COM_AddDebugCommand("give2", Command_KartGiveItem_f);
	COM_AddDebugCommand("give3", Command_KartGiveItem_f);
	COM_AddDebugCommand("give4", Command_KartGiveItem_f);

	COM_AddCommand("schedule_add", Command_Schedule_Add);
	COM_AddCommand("schedule_clear", Command_Schedule_Clear);
	COM_AddCommand("schedule_list", Command_Schedule_List);

	COM_AddCommand("automate_set", Command_Automate_Set);

	COM_AddDebugCommand("eval", Command_Eval);

	COM_AddCommand("writetextmap", Command_WriteTextmap);

	// for master server connection
	AddMServCommands();

	RegisterNetXCmd(XD_RANDOMSEED, Got_RandomSeed);

	// d_clisrv

	COM_AddDebugCommand("ping", Command_Ping_f);

	RegisterNetXCmd(XD_DISCORD, Got_DiscordInfo);

	COM_AddDebugCommand("numthinkers", Command_Numthinkers_f);
	COM_AddDebugCommand("countmobjs", Command_CountMobjs_f);

#ifdef _DEBUG
	COM_AddDebugCommand("causecfail", Command_CauseCfail_f);
#endif
#ifdef LUA_ALLOW_BYTECODE
	COM_AddCommand("dumplua", Command_Dumplua_f);
#endif

#ifdef DEVELOP
	COM_AddDebugCommand("fastforward", Command_FastForward);
	COM_AddDebugCommand("crypt", Command_Crypt_f);
#endif

	K_RegisterMidVoteCVars();

	{
		extern struct CVarList *cvlist_server;
		CV_RegisterList(cvlist_server);
	}
}

// =========================================================================
//                           CLIENT STARTUP
// =========================================================================

/** Registers client commands and variables.
  * Nothing needed for a dedicated server should be registered here.
  *
  * \sa D_RegisterServerCommands
  */
void D_RegisterClientCommands(void)
{
	INT32 i;

	// Set default player names
	// Monster Iestyn (12/08/19): not sure where else I could have actually put this, but oh well
	for (i = 0; i < MAXPLAYERS; i++)
		sprintf(player_names[i], "Player %c", 'A' + i); // SRB2Kart: Letters like Sonic 3!

	if (dedicated)
		return;

	COM_AddCommand("changeteam", Command_Teamchange_f);
	COM_AddCommand("changeteam2", Command_Teamchange2_f);
	COM_AddCommand("changeteam3", Command_Teamchange3_f);
	COM_AddCommand("changeteam4", Command_Teamchange4_f);

	COM_AddCommand("invite", Command_Invite_f);
	COM_AddCommand("cancelinvite", Command_CancelInvite_f);
	COM_AddCommand("acceptinvite", Command_AcceptInvite_f);
	COM_AddCommand("rejectinvite", Command_RejectInvite_f);
	COM_AddCommand("leaveparty", Command_LeaveParty_f);

	COM_AddCommand("playdemo", Command_Playdemo_f);
	COM_AddCommand("timedemo", Command_Timedemo_f);
	COM_AddCommand("stopdemo", Command_Stopdemo_f);
	COM_AddCommand("playintro", Command_Playintro_f);

	COM_AddDebugCommand("resetcamera", Command_ResetCamera_f);

	COM_AddDebugCommand("view", Command_View_f);
	COM_AddCommand("view2", Command_View_f);
	COM_AddCommand("view3", Command_View_f);
	COM_AddCommand("view4", Command_View_f);

	COM_AddCommand("setviews", Command_SetViews_f);

	COM_AddCommand("setcontrol", Command_Setcontrol_f);
	COM_AddCommand("setcontrol2", Command_Setcontrol2_f);
	COM_AddCommand("setcontrol3", Command_Setcontrol3_f);
	COM_AddCommand("setcontrol4", Command_Setcontrol4_f);

	COM_AddCommand("screenshot", M_ScreenShot);
	COM_AddCommand("startmovie", Command_StartMovie_f);
	COM_AddCommand("startlossless", Command_StartLossless_f);
	COM_AddCommand("stopmovie", Command_StopMovie_f);
	COM_AddDebugCommand("minigen", M_MinimapGenerate);

#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
	M_AVRecorder_AddCommands();
#endif

	COM_AddDebugCommand("displayplayer", Command_Displayplayer_f);

	// k_menu.c
	//CV_RegisterVar(&cv_compactscoreboard);

	// ingame object placing
	COM_AddDebugCommand("objectplace", Command_ObjectPlace_f);
	//COM_AddCommand("writethings", Command_Writethings_f);
//	CV_RegisterVar(&cv_grid);
//	CV_RegisterVar(&cv_snapto);

	// add cheats
	COM_AddDebugCommand("noclip", Command_CheatNoClip_f);
	COM_AddDebugCommand("god", Command_CheatGod_f);
	COM_AddDebugCommand("freeze", Command_CheatFreeze_f);
	COM_AddDebugCommand("setrings", Command_Setrings_f);
	COM_AddDebugCommand("setspheres", Command_Setspheres_f);
	COM_AddDebugCommand("setlives", Command_Setlives_f);
	COM_AddDebugCommand("setscore", Command_Setscore_f);
	COM_AddDebugCommand("devmode", Command_Devmode_f);
	COM_AddDebugCommand("savecheckpoint", Command_Savecheckpoint_f);
	COM_AddDebugCommand("scale", Command_Scale_f);
	COM_AddDebugCommand("gravflip", Command_Gravflip_f);
	COM_AddDebugCommand("hurtme", Command_Hurtme_f);
	COM_AddDebugCommand("teleport", Command_Teleport_f);
	COM_AddDebugCommand("rteleport", Command_RTeleport_f);
	COM_AddDebugCommand("skynum", Command_Skynum_f);
	COM_AddDebugCommand("weather", Command_Weather_f);
	COM_AddDebugCommand("grayscale", Command_Grayscale_f);
	COM_AddDebugCommand("goto", Command_Goto_f);
	COM_AddDebugCommand("angle", Command_Angle_f);
	COM_AddDebugCommand("respawnat", Command_RespawnAt_f);
	COM_AddDebugCommand("goto_skybox", Command_GotoSkybox_f);

	{
		extern struct CVarList *cvlist_player;
		CV_RegisterList(cvlist_player);
	}
}

/** Checks if a name (as received from another player) is okay.
  * A name is okay if it is no fewer than 1 and no more than ::MAXPLAYERNAME
  * chars long (not including NUL), it does not begin or end with a space,
  * it does not contain non-printing characters (according to isprint(), which
  * allows space), it does not start with a digit, and no other player is
  * currently using it.
  * \param name      Name to check.
  * \param playernum Player who wants the name, so we can check if they already
  *                  have it, and let them keep it if so.
  * \sa CleanupPlayerName, SetPlayerName, Got_NameAndColor
  * \author Graue <graue@oceanbase.org>
  */

static boolean AllowedPlayerNameChar(char ch)
{
	if (!isprint(ch) || ch == ';' || ch == '"' || (UINT8)(ch) >= 0x80)
		return false;

	return true;
}

boolean EnsurePlayerNameIsGood(char *name, INT32 playernum)
{
	size_t ix, len = strlen(name);

	if (len == 0 || len > MAXPLAYERNAME)
		return false; // Empty or too long.
	if (name[0] == ' ')
		return false; // Starts with a space.
	if (isdigit(name[0]))
		return false; // Starts with a digit.
	if (name[0] == '@' || name[0] == '~')
		return false; // Starts with an admin symbol.

	// Clean up trailing whitespace.
	while (len && name[len-1] == ' ')
	{
		name[len-1] = '\0';
		len--;
	}
	if (len == 0)
		return false;

	// Check if it contains a non-printing character.
	// Note: ANSI C isprint() considers space a printing character.
	// Also don't allow semicolons, since they are used as
	// console command separators.

	// Also, anything over 0x80 is disallowed too, since compilers love to
	// differ on whether they're printable characters or not.
	for (ix = 0; ix < len; ix++)
		if (!AllowedPlayerNameChar(name[ix]))
			return false;

	// Check if a player is currently using the name, case-insensitively.
	for (ix = 0; ix < MAXPLAYERS; ix++)
	{
		if (ix != (size_t)playernum && playeringame[ix]
			&& strcasecmp(name, player_names[ix]) == 0)
		{
			// We shouldn't kick people out just because
			// they joined the game with the same name
			// as someone else -- modify the name instead.

			// Recursion!
			// Slowly strip characters off the end of the
			// name until we no longer have a duplicate.
			if (len > 1)
			{
				name[len-1] = '\0';
				if (!EnsurePlayerNameIsGood (name, playernum))
					return false;
			}
			else if (len == 1) // Agh!
			{
				// Last ditch effort...
				sprintf(name, "%d", 'A' + M_RandomKey(26));
				if (!EnsurePlayerNameIsGood (name, playernum))
					return false;
			}
			else
				return false;
		}
	}

	return true;
}

/** Cleans up a local player's name before sending a name change.
  * Spaces at the beginning or end of the name are removed. Then if the new
  * name is identical to another player's name, ignoring case, the name change
  * is canceled, and the name in cv_playername[n].value
  * is restored to what it was before.
  *
  * We assume that if playernum is in ::g_localplayers
  * (unless clientjoin is true, a necessary evil)
  * the console variable ::cv_playername[n] is
  * already set to newname. However, the player name table is assumed to
  * contain the old name.
  *
  * \param playernum Player number who has requested a name change.
  *                  Should be in ::g_localplayers.
  * \param newname   New name for that player; should already be in
  *                  ::cv_playername if player is a local player.
  * \sa cv_playername, SendNameAndColor, SetPlayerName
  * \author Graue <graue@oceanbase.org>
  */
void CleanupPlayerName(INT32 playernum, const char *newname)
{
	char *buf;
	char *p;
	char *tmpname = NULL;
	INT32 i;
	boolean namefailed = true;
	boolean clientjoin = !!(playernum >= MAXPLAYERS);

	if (clientjoin)
		playernum -= MAXPLAYERS;

	buf = Z_StrDup(newname);

	do
	{
		p = buf;

		while (*p == ' ')
			p++; // remove leading spaces

		if (strlen(p) == 0)
			break; // empty names not allowed

		if (isdigit(*p))
			break; // names starting with digits not allowed

		if (*p == '@' || *p == '~')
			break; // names that start with @ or ~ (admin symbols) not allowed

		tmpname = p;

		do
		{
			if (!AllowedPlayerNameChar(*p))
				break;
		}
		while (*++p) ;

		if (*p)/* bad char found */
			break;

		// Remove trailing spaces.
		p = &tmpname[strlen(tmpname)-1]; // last character
		while (*p == ' ' && p >= tmpname)
		{
			*p = '\0';
			p--;
		}

		if (strlen(tmpname) == 0)
			break; // another case of an empty name

		// Truncate name if it's too long (max MAXPLAYERNAME chars
		// excluding NUL).
		if (strlen(tmpname) > MAXPLAYERNAME)
			tmpname[MAXPLAYERNAME] = '\0';

		// Remove trailing spaces again.
		p = &tmpname[strlen(tmpname)-1]; // last character
		while (*p == ' ' && p >= tmpname)
		{
			*p = '\0';
			p--;
		}

		// no stealing another player's name
		if (!clientjoin)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (i != playernum && playeringame[i]
					&& strcasecmp(tmpname, player_names[i]) == 0)
				{
					break;
				}
			}

			if (i < MAXPLAYERS)
				break;
		}

		// name is okay then
		namefailed = false;
	} while (0);

	if (namefailed)
		tmpname = player_names[playernum];

	// set consvars whether namefailed or not, because even if it succeeded,
	// spaces may have been removed
	if (clientjoin)
		CV_StealthSet(&cv_playername[playernum], tmpname);
	else
	{
		for (i = 0; i <= splitscreen; i++)
		{
			if (playernum == g_localplayers[i])
			{
				CV_StealthSet(&cv_playername[i], tmpname);
				break;
			}
		}

		if (i > splitscreen)
		{
			I_Assert(((void)"CleanupPlayerName used on non-local player", 0));
		}
	}

	Z_Free(buf);
}

/** Sets a player's name, if it is good.
  * If the name is not good (indicating a modified or buggy client), it is not
  * set, and if we are the server in a netgame, the player responsible is
  * kicked with a consistency failure.
  *
  * This function prints a message indicating the name change, unless the game
  * is currently showing the intro, e.g. when processing autoexec.cfg.
  *
  * \param playernum Player number who has requested a name change.
  * \param newname   New name for that player. Should be good, but won't
  *                  necessarily be if the client is maliciously modified or
  *                  buggy.
  * \sa CleanupPlayerName, EnsurePlayerNameIsGood
  * \author Graue <graue@oceanbase.org>
  */
static void SetPlayerName(INT32 playernum, char *newname)
{
	if (EnsurePlayerNameIsGood(newname, playernum))
	{
		if (strcasecmp(newname, player_names[playernum]) != 0)
		{
			if (netgame)
				HU_AddChatText(va("\x82*%s renamed to %s", player_names[playernum], newname), false);

			player_name_changes[playernum]++;

			strcpy(player_names[playernum], newname);
			demo_extradata[playernum] |= DXD_NAME;
		}
	}
	else
	{
		CONS_Printf(M_GetText("Player %d sent a bad name change\n"), playernum+1);
		if (server && netgame)
			SendKick(playernum, KICK_MSG_CON_FAIL);
	}
}

boolean CanChangeSkin(INT32 playernum)
{
	(void)playernum;

	// Of course we can change if we're not playing
	if (!Playing() || !addedtogame)
		return true;

	// Force skin in effect.
	if (cv_forceskin.value != -1 && K_CanChangeRules(true))
		return false;

	// ... there used to be a lot more here, but it's now handled in Got_NameAndColor.

	return true;
}

static void ForceAllSkins(INT32 forcedskin)
{
	INT32 i;

	if (demo.playback)
		return; // DXD_SKIN should handle all changes for us

	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i])
			continue;

		SetPlayerSkinByNum(i, forcedskin);
	}

	if (dedicated)
		return;

	// If it's me (or my brother (or my sister (or my trusty pet dog))), set appropriate skin value in cv_skin
	for (i = 0; i <= splitscreen; i++)
	{
		if (!playeringame[g_localplayers[i]])
			continue;

		CV_StealthSet(&cv_skin[i], skins[forcedskin].name);
	}
}

static const char *
VaguePartyDescription (int playernum, int size, int default_color)
{
	static char party_description
		[1 + MAXPLAYERNAME + 1 + sizeof " and x others"];
	const char *name;
	name = player_names[playernum];
	/*
	less than check for the dumb compiler because I KNOW it'll
	complain about "writing x bytes into an area of y bytes"!!!
	*/
	if (size > 1 && size <= MAXSPLITSCREENPLAYERS)
	{
		sprintf(party_description,
				"\x83%s%c and %d other%s",
				name,
				default_color,
				( size - 1 ),
				( (size > 2) ? "s" : "" )
		);
	}
	else
	{
		sprintf(party_description,
				"\x83%s%c",
				name,
				default_color
		);
	}
	return party_description;
}

static INT32 snacpending[MAXSPLITSCREENPLAYERS] = {0,0,0,0};
static INT32 chmappending = 0;

// name, color, or skin has changed
//
static void SendNameAndColor(const UINT8 n)
{
	const INT32 playernum = g_localplayers[n];
	player_t *player = NULL;

	if (splitscreen < n)
	{
		return; // can happen if skin4/color4/name4 changed
	}

	if (playernum == -1)
	{
		return;
	}

	player = &players[playernum];

	UINT16 sendColor = cv_playercolor[n].value;
	UINT16 sendFollowerColor = cv_followercolor[n].value;

	// don't allow inaccessible colors
	if (sendColor != SKINCOLOR_NONE && K_ColorUsable(sendColor, false, true) == false)
	{
		if (player->skincolor && K_ColorUsable(player->skincolor, false, true) == true)
		{
			// Use our previous color
			CV_StealthSetValue(&cv_playercolor[n], player->skincolor);
		}
		else
		{
			// Use our default color
			CV_StealthSetValue(&cv_playercolor[n], SKINCOLOR_NONE);
		}

		lastgoodcolor[playernum] = sendColor = cv_playercolor[n].value;
	}

	// ditto for follower colour:
	if (sendFollowerColor != SKINCOLOR_NONE && K_ColorUsable(sendFollowerColor, true, true) == false)
	{
		CV_StealthSet(&cv_followercolor[n], "Default"); // set it to "Default". I don't care about your stupidity!
		sendFollowerColor = cv_followercolor[n].value;
	}

	// We'll handle it later if we're not playing.
	if (!Playing())
	{
		return;
	}

	// Don't change skin if the server doesn't want you to.
	if (!CanChangeSkin(playernum))
	{
		CV_StealthSet(&cv_skin[n], skins[player->skin].name);
	}

	// check if player has the skin loaded (cv_skin may have
	// the name of a skin that was available in the previous game)
	cv_skin[n].value = R_SkinAvailableEx(cv_skin[n].string, false);
	if ((cv_skin[n].value < 0) || !R_SkinUsable(playernum, cv_skin[n].value, false))
	{
		CV_StealthSet(&cv_skin[n], DEFAULTSKIN);
		cv_skin[n].value = 0;
	}

	cv_follower[n].value = K_FollowerAvailable(cv_follower[n].string);
	if (cv_follower[n].value < 0 || !K_FollowerUsable(cv_follower[n].value))
	{
		CV_StealthSet(&cv_follower[n], "None");
		cv_follower[n].value = -1;
	}

	if (sendColor == SKINCOLOR_NONE)
	{
		sendColor = skins[cv_skin[n].value].prefcolor;
	}

	if (sendFollowerColor == SKINCOLOR_NONE)
	{
		if (cv_follower[n].value >= 0)
		{
			sendFollowerColor = followers[cv_follower[n].value].defaultcolor;
		}
		else
		{
			sendFollowerColor = SKINCOLOR_RED; // picked by dice roll, guaranteed to be random
		}
	}

	// Don't send if everything was identical.
	if (!strcmp(cv_playername[n].string, player_names[playernum])
		&& sendColor == player->skincolor
		&& !stricmp(cv_skin[n].string, skins[player->skin].name)
		&& !stricmp(cv_follower[n].string,
			(player->followerskin < 0 ? "None" : followers[player->followerskin].name))
		&& sendFollowerColor == player->followercolor)
	{
		return;
	}

	snacpending[n]++;

	// Don't change name if muted
	if (player_name_changes[playernum] >= MAXNAMECHANGES)
	{
		CV_StealthSet(&cv_playername[n], player_names[playernum]);
		HU_AddChatText("\x85* You must wait to change your name again.", false);
	}
	else if (cv_mute.value && !(server || IsPlayerAdmin(playernum)))
	{
		CV_StealthSet(&cv_playername[n], player_names[playernum]);
	}
	else // Cleanup name if changing it
	{
		CleanupPlayerName(playernum, cv_playername[n].zstring);
	}

	char buf[MAXPLAYERNAME+12];
	char *p = buf;

	// Finally write out the complete packet and send it off.
	WRITESTRINGN(p, cv_playername[n].zstring, MAXPLAYERNAME);
	WRITEUINT16(p, sendColor);
	WRITEUINT8(p, (UINT8)cv_skin[n].value);
	WRITEINT16(p, (INT16)cv_follower[n].value);
	//CONS_Printf("Sending follower id %d\n", (INT16)cv_follower[n].value);
	WRITEUINT16(p, sendFollowerColor);

	SendNetXCmdForPlayer(n, XD_NAMEANDCOLOR, buf, p - buf);
}

static void FinalisePlaystateChange(INT32 playernum)
{
	demo_extradata[playernum] |= DXD_PLAYSTATE;

	// Clear player score and rings if a spectator.
	if (players[playernum].spectator)
	{
		// To attempt to discourage rage-spectators, we delay any rejoining.
		// If you're engaging in a DUEL and quit early, in addition to the
		// indignity of losing your PWR, you get a special extra-long delay.
		if (
			netgame
			&& players[playernum].jointime > 1
			&& players[playernum].spectatorReentry == 0
		)
		{
			UINT8 pcount = 0;

			if (cv_duelspectatorreentry.value > cv_spectatorreentry.value)
			{
				UINT8 i;

				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (!playeringame[i] || players[i].spectator)
						continue;
					if (++pcount < 2)
						continue;
					break;
				}
			}

			players[playernum].spectatorReentry =
				(pcount == 1)
					? (cv_duelspectatorreentry.value * TICRATE)
					: (cv_spectatorreentry.value * TICRATE);

			//CONS_Printf("player %u got re-entry of %u\n", playernum, players[playernum].spectatorReentry);
		}

		if (gametyperules & GTR_POINTLIMIT) // SRB2kart
		{
			players[playernum].roundscore = 0;
			K_CalculateBattleWanted();
		}

		K_PlayerForfeit(playernum, true);

		if (players[playernum].mo)
			players[playernum].mo->health = 1;

		K_StripItems(&players[playernum]);
	}

	// Reset away view
	G_ResetViews();

	K_CheckBumpers(); // SRB2Kart
	P_CheckRacers(); // also SRB2Kart
}

static void Got_NameAndColor(const UINT8 **cp, INT32 playernum)
{
	player_t *p = &players[playernum];
	char name[MAXPLAYERNAME+1];
	UINT16 color, followercolor;
	UINT8 skin;
	INT16 follower;
	SINT8 localplayer = -1;
	UINT8 i;

#ifdef PARANOIA
	if (playernum < 0 || playernum > MAXPLAYERS)
		I_Error("There is no player %d!", playernum);
#endif

	for (i = 0; i <= splitscreen; i++)
	{
		if (playernum == g_localplayers[i])
		{
			snacpending[i]--;

#ifdef PARANOIA
			if (snacpending[i] < 0)
			{
				S_StartSound(NULL, sfx_monch);
				I_Sleep(1000); // let the monch play out for 1 second
				I_Error("snacpending[%d] negative!", i);
			}
#endif

			localplayer = i;
			break;
		}
	}

	READSTRINGN(*cp, name, MAXPLAYERNAME);
	color = READUINT16(*cp);
	skin = READUINT8(*cp);
	follower = READINT16(*cp);
	followercolor = READUINT16(*cp);

	// set name
	if (player_name_changes[playernum] < MAXNAMECHANGES)
	{
		if (strcasecmp(player_names[playernum], name) != 0)
			SetPlayerName(playernum, name);
	}

	// set color
	p->skincolor = color % numskincolors;
	if (p->mo)
		p->mo->color = (UINT16)p->skincolor;
	demo_extradata[playernum] |= DXD_COLOR;

	// normal player colors
	if (server && !P_IsMachineLocalPlayer(p))
	{
		boolean kick = false;

		// don't allow inaccessible colors
		if (K_ColorUsable(p->skincolor, false, false) == false)
		{
			kick = true;
		}

		if (kick)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal color change received from %s, color: %d)\n"), player_names[playernum], p->skincolor);
			SendKick(playernum, KICK_MSG_CON_FAIL);
			return;
		}
	}

	// set skin
	if (cv_forceskin.value >= 0 && K_CanChangeRules(true)) // Server wants everyone to use the same player
	{
		const INT32 forcedskin = cv_forceskin.value;
		SetPlayerSkinByNum(playernum, forcedskin);

		if (localplayer != -1)
			CV_StealthSet(&cv_skin[localplayer], skins[forcedskin].name);
	}
	else
	{
		UINT8 oldskin = players[playernum].skin;

		SetPlayerSkinByNum(playernum, skin);

		// The following is a miniature subset of Got_Teamchange.
		if ((gamestate == GS_LEVEL) // In a level?
			&& (players[playernum].jointime > 1) // permit on join
			&& (leveltime > introtime) // permit during intro turnaround
			&& (players[playernum].skin != oldskin)) // a skin change actually happened?
		{
			players[playernum].roundconditions.switched_skin = true;

			if (
				cv_restrictskinchange.value // Skin changes are restricted?
				&& G_GametypeHasSpectators() // not a spectator...
				&& players[playernum].spectator == false // ...but could be?
			)
			{
				for (i = 0; i < MAXPLAYERS; ++i)
				{
					if (i == playernum)
						continue;
					if (!playeringame[i])
						continue;
					if (players[i].spectator)
						continue;
					break;
				}

				if (i != MAXPLAYERS // Someone on your server who isn't you?
					&& LUA_HookTeamSwitch(&players[playernum], 0, false, false, false)) // fiiiine, lua can except it
				{
					P_DamageMobj(players[playernum].mo, NULL, NULL, 1, DMG_SPECTATOR);

					if (players[i].spectator)
					{
						HU_AddChatText(va("\x82*%s became a spectator.", player_names[playernum]), false);

						FinalisePlaystateChange(playernum);
					}
				}
			}
		}
	}

	// set follower colour:
	// Don't bother doing garbage and kicking if we receive None,
	// this is both silly and a waste of time,
	// this will be handled properly in K_HandleFollower.
	p->followercolor = followercolor;

	// set follower
	K_SetFollowerByNum(playernum, follower);
}

enum {
	WP_KICKSTARTACCEL = 1<<0,
	WP_SHRINKME = 1<<1,
	WP_AUTOROULETTE = 1<<2,
	WP_ANALOGSTICK = 1<<3,
	WP_AUTORING = 1<<4,
};

void WeaponPref_Send(UINT8 ssplayer)
{
	UINT8 prefs = 0;

	if (cv_kickstartaccel[ssplayer].value)
		prefs |= WP_KICKSTARTACCEL;

	if (cv_autoroulette[ssplayer].value)
		prefs |= WP_AUTOROULETTE;

	if (cv_shrinkme[ssplayer].value)
		prefs |= WP_SHRINKME;

	if (gamecontrolflags[ssplayer] & GCF_ANALOGSTICK)
		prefs |= WP_ANALOGSTICK;

	if (cv_autoring[ssplayer].value)
		prefs |= WP_AUTORING;

	UINT8 buf[2];
	buf[0] = prefs;
	buf[1] = cv_mindelay.value;

	SendNetXCmdForPlayer(ssplayer, XD_WEAPONPREF, buf, sizeof buf);
}

void WeaponPref_Save(UINT8 **cp, INT32 playernum)
{
	player_t *player = &players[playernum];

	UINT8 prefs = 0;

	if (player->pflags & PF_KICKSTARTACCEL)
		prefs |= WP_KICKSTARTACCEL;

	if (player->pflags & PF_AUTOROULETTE)
		prefs |= WP_AUTOROULETTE;

	if (player->pflags & PF_SHRINKME)
		prefs |= WP_SHRINKME;

	if (player->pflags & PF_ANALOGSTICK)
		prefs |= WP_ANALOGSTICK;

	if (player->pflags & PF_AUTORING)
		prefs |= WP_AUTORING;

	WRITEUINT8(*cp, prefs);
}

size_t WeaponPref_Parse(const UINT8 *bufstart, INT32 playernum)
{
	const UINT8 *p = bufstart;
	player_t *player = &players[playernum];

	UINT8 prefs = READUINT8(p);

	player->pflags &= ~(PF_KICKSTARTACCEL|PF_SHRINKME|PF_AUTOROULETTE|PF_AUTORING);

	if (prefs & WP_KICKSTARTACCEL)
		player->pflags |= PF_KICKSTARTACCEL;

	if (prefs & WP_AUTOROULETTE)
		player->pflags |= PF_AUTOROULETTE;

	if (prefs & WP_SHRINKME)
		player->pflags |= PF_SHRINKME;

	if (prefs & WP_ANALOGSTICK)
		player->pflags |= PF_ANALOGSTICK;
	else
		player->pflags &= ~PF_ANALOGSTICK;

	if (prefs & WP_AUTORING)
		player->pflags |= PF_AUTORING;

	if (leveltime < 2)
	{
		// BAD HACK: No other place I tried to slot this in
		// made it work for the host when they initally host,
		// so this will have to do.
		K_UpdateShrinkCheat(player);
	}

	return p - bufstart;
}

static void Got_WeaponPref(const UINT8 **cp,INT32 playernum)
{
	*cp += WeaponPref_Parse(*cp, playernum);

	UINT8 mindelay = READUINT8(*cp);
	if (server)
	{
		for (UINT8 i = 0; i < G_LocalSplitscreenPartySize(playernum); ++i)
			playerdelaytable[G_LocalSplitscreenPartyMember(playernum, i)] = mindelay;
	}

	// SEE ALSO g_demo.c
	demo_extradata[playernum] |= DXD_WEAPONPREF;
}

static void Got_PartyInvite(const UINT8 **cp,INT32 playernum)
{
	UINT8 invitee;

	boolean kick = false;

	invitee = READUINT8 (*cp);

	if (
			invitee < MAXPLAYERS &&
			playeringame[invitee] &&
			playerconsole[playernum] == playernum/* only consoleplayer may! */
	){
		invitee = playerconsole[invitee];
		/* you cannot invite yourself or your computer */
		if (invitee == playernum)
			kick = true;
	}
	else
		kick = true;

	if (kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal splitscreen invitation received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (splitscreen_invitations[invitee] < 0)
	{
		splitscreen_invitations[invitee] = playernum;

		if (invitee == consoleplayer)/* hey that's me! */
		{
			HU_AddChatText(va(
						"\x82*You have been invited to join %s.",
						VaguePartyDescription(
							playernum, G_PartySize(playernum), '\x82')
			), true);
		}
	}
}

static void Got_AcceptPartyInvite(const UINT8 **cp,INT32 playernum)
{
	int invitation;

	(void)cp;

	if (playerconsole[playernum] != playernum)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal accept splitscreen invite received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	invitation = splitscreen_invitations[playernum];

	if (invitation >= 0)
	{
		if (G_IsPartyLocal(invitation))
		{
			HU_AddChatText(va(
						"\x82*%s joined your party!",
						VaguePartyDescription(
							playernum, G_LocalSplitscreenPartySize(playernum), '\x82')
			), true);
		}
		else if (playernum == consoleplayer)
		{
			HU_AddChatText(va(
						"\x82*You joined %s's party!",
						VaguePartyDescription(
							invitation, G_PartySize(invitation), '\x82')
			), true);
		}

		G_JoinParty(invitation, playernum);

		splitscreen_invitations[playernum] = -1;
	}
}

static void Got_CancelPartyInvite(const UINT8 **cp,INT32 playernum)
{
	UINT8 invitee;

	invitee = READUINT8 (*cp);

	if (
			invitee < MAXPLAYERS &&
			playeringame[invitee]
	){
		invitee = playerconsole[invitee];

		if (splitscreen_invitations[invitee] == playerconsole[playernum])
		{
			splitscreen_invitations[invitee] = -1;

			if (consoleplayer == invitee)
			{
				HU_AddChatText("\x85*Your invitation has been rescinded.", true);
			}
		}
	}
	else
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal cancel splitscreen invite received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
	}
}

static void Got_LeaveParty(const UINT8 **cp,INT32 playernum)
{
	(void)cp;

	if (playerconsole[playernum] != playernum)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal accept splitscreen invite received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (consoleplayer == splitscreen_invitations[playernum])
	{
		HU_AddChatText(va(
					"\x85*\x83%s\x85 rejected your invitation.",
					player_names[playernum]
		), true);
	}

	splitscreen_invitations[playernum] = -1;

	if (G_IsPartyLocal(playernum) && playernum != consoleplayer)
	{
		HU_AddChatText(va(
					"\x85*%s left your party.",
					VaguePartyDescription(
						playernum, G_LocalSplitscreenPartySize(playernum), '\x85')
		), true);
	}

	G_LeaveParty(playernum);
}

void D_SendPlayerConfig(UINT8 n)
{
	SendNameAndColor(n);
	WeaponPref_Send(n);
}

static camera_t *LocalMutableCamera(INT32 playernum)
{
	if (demo.playback)
	{
		// TODO: could have splitscreen support
		if (!camera[0].freecam)
		{
			return NULL;
		}

		return &camera[0];
	}

	if (G_IsPartyLocal(playernum))
	{
		UINT8 viewnum = G_PartyPosition(playernum);
		camera_t *cam = &camera[viewnum];

		if (cam->freecam || (players[playernum].spectator && !K_DirectorIsAvailable(viewnum)))
		{
			return cam;
		}
	}

	return NULL;
}

void D_Cheat(INT32 playernum, INT32 cheat, ...)
{
	va_list ap;

	UINT8 buf[64];
	UINT8 *p = buf;

	camera_t *cam;
	if ((cam = LocalMutableCamera(playernum)))
	{
		switch (cheat)
		{
			case CHEAT_RELATIVE_TELEPORT:
				va_start(ap, cheat);
				cam->x += va_arg(ap, fixed_t);
				cam->y += va_arg(ap, fixed_t);
				cam->z += va_arg(ap, fixed_t);
				va_end(ap);
				return;

			case CHEAT_TELEPORT:
				va_start(ap, cheat);
				cam->x = va_arg(ap, fixed_t);
				cam->y = va_arg(ap, fixed_t);
				cam->z = va_arg(ap, fixed_t);
				va_end(ap);
				return;

			case CHEAT_ANGLE:
				va_start(ap, cheat);
				cam->angle = va_arg(ap, angle_t);
				va_end(ap);
				return;

			default:
				break;
		}
	}

	if (!CV_CheatsEnabled())
	{
		CONS_Printf("This cannot be used without cheats enabled.\n");
		return;
	}

	if (demo.playback && cheat == CHEAT_DEVMODE)
	{
		// There is no networking in demos, but devmode is
		// too useful to be inaccessible!
		// TODO: maybe allow everything, even though it would
		// desync replays? May be useful for debugging.
		va_start(ap, cheat);
		cht_debug = va_arg(ap, UINT32);
		va_end(ap);
		return;
	}

	// sanity check
	if (demo.playback)
	{
		return;
	}

	WRITEUINT8(p, playernum);
	WRITEUINT8(p, cheat);

	va_start(ap, cheat);
#define COPY(writemacro, type) writemacro (p, va_arg(ap, type))

	switch (cheat)
	{
		case CHEAT_SAVECHECKPOINT:
			COPY(WRITEFIXED, fixed_t); // x
			COPY(WRITEFIXED, fixed_t); // y
			COPY(WRITEFIXED, fixed_t); // z
			break;

		case CHEAT_RINGS:
		case CHEAT_LIVES:
			// If you're confused why 'int' instead of
			// 'SINT8', search online: 'default argument promotions'
			COPY(WRITESINT8, int);
			break;

		case CHEAT_SCALE:
			COPY(WRITEFIXED, fixed_t);
			break;

		case CHEAT_HURT:
			COPY(WRITEINT32, INT32);
			break;

		case CHEAT_RELATIVE_TELEPORT:
		case CHEAT_TELEPORT:
			COPY(WRITEFIXED, fixed_t);
			COPY(WRITEFIXED, fixed_t);
			COPY(WRITEFIXED, fixed_t);
			break;

		case CHEAT_DEVMODE:
			COPY(WRITEUINT32, UINT32);
			break;

		case CHEAT_GIVEITEM:
			COPY(WRITESINT8, int);
			COPY(WRITEUINT8, unsigned int);
			break;

		case CHEAT_GIVEPOWERUP:
			COPY(WRITEUINT8, unsigned int);
			COPY(WRITEUINT16, unsigned int);
			break;

		case CHEAT_SCORE:
			COPY(WRITEUINT32, UINT32);
			break;

		case CHEAT_ANGLE:
			COPY(WRITEANGLE, angle_t);
			break;

		case CHEAT_RESPAWNAT:
			COPY(WRITEINT32, INT32);
			break;

		case CHEAT_SPHERES:
			COPY(WRITEINT16, int);
			break;
	}

#undef COPY
	va_end(ap);

	SendNetXCmd(XD_CHEAT, buf, p - buf);
}

// Only works for displayplayer, sorry!
static void Command_ResetCamera_f(void)
{
	P_ResetCamera(&players[g_localplayers[0]], &camera[0]);
}

/* Consider replacing nametonum with this */
static INT32 LookupPlayer(const char *s)
{
	INT32 playernum;

	if (*s == '0')/* clever way to bypass atoi */
		return 0;

	if (( playernum = atoi(s) ))
	{
		playernum = max(min(playernum, MAXPLAYERS-1), 0);/* not out of range */
		return playernum;
	}

	for (playernum = 0; playernum < MAXPLAYERS; ++playernum)
	{
		/* Match name case-insensitively: fully, or partially the start. */
		if (playeringame[playernum])
			if (strnicmp(player_names[playernum], s, strlen(s)) == 0)
		{
			return playernum;
		}
	}
	return -1;
}

static INT32 FindPlayerByPlace(INT32 place)
{
	INT32 playernum;
	for (playernum = 0; playernum < MAXPLAYERS; ++playernum)
		if (playeringame[playernum])
	{
		if (players[playernum].position == place)
		{
			return playernum;
		}
	}
	return -1;
}

//
// GetViewablePlayerPlaceRange
// Return in first and last, that player available to view, sorted by placement
// in the race.
//
static void GetViewablePlayerPlaceRange(INT32 *first, INT32 *last)
{
	INT32 i;
	INT32 place;

	(*first) = MAXPLAYERS;
	(*last) = 0;

	for (i = 0; i < MAXPLAYERS; ++i)
		if (G_CouldView(i))
	{
		place = players[i].position;
		if (place < (*first))
			(*first) = place;
		if (place > (*last))
			(*last) = place;
	}
}

static int GetCommandViewNumber(void)
{
	char c = COM_Argv(0)[strlen(COM_Argv(0))-1];/* may be digit */

	switch (c)
	{
		default:
			return 0;

		case '2':
		case '3':
		case '4':
			return c - '1';
	}
}

#define PRINTVIEWPOINT( pre,suf ) \
	CONS_Printf(pre"viewing \x84(%d) \x83%s\x80"suf".\n",\
			(*displayplayerp), player_names[(*displayplayerp)]);
static void Command_View_f(void)
{
	INT32 *displayplayerp;
	INT32 olddisplayplayer;
	int viewnum = 1 + GetCommandViewNumber();
	const char *playerparam;
	INT32 placenum;
	INT32 playernum;
	INT32 firstplace, lastplace;

	if (viewnum > 1 && !( multiplayer && demo.playback ))
	{
		CONS_Alert(CONS_NOTICE,
				"You must be viewing a multiplayer replay to use this.\n");
		return;
	}

	if (camera[viewnum-1].freecam)
		return;

	displayplayerp = &displayplayers[viewnum-1];

	if (COM_Argc() > 1)/* switch to player */
	{
		playerparam = COM_Argv(1);
		if (playerparam[0] == '#')/* search by placement */
		{
			placenum = atoi(&playerparam[1]);
			playernum = FindPlayerByPlace(placenum);
			if (playernum == -1 || !G_CouldView(playernum))
			{
				GetViewablePlayerPlaceRange(&firstplace, &lastplace);
				if (playernum == -1)
				{
					CONS_Alert(CONS_WARNING, "There is no player in that place! ");
				}
				else
				{
					CONS_Alert(CONS_WARNING,
							"That player cannot be viewed currently! "
							"The first player that you can view is \x82#%d\x80; ",
							firstplace);
				}
				CONS_Printf("Last place is \x82#%d\x80.\n", lastplace);
				return;
			}
		}
		else
		{
			if (( playernum = LookupPlayer(COM_Argv(1)) ) == -1)
			{
				CONS_Alert(CONS_WARNING, "There is no player by that name!\n");
				return;
			}
			if (!playeringame[playernum])
			{
				CONS_Alert(CONS_WARNING, "There is no player using that slot!\n");
				return;
			}
		}

		olddisplayplayer = (*displayplayerp);
		G_ResetView(viewnum, playernum, false);

		/* The player we wanted was corrected to who it already was. */
		if ((*displayplayerp) == olddisplayplayer)
			return;

		if ((*displayplayerp) != playernum)/* differ parameter */
		{
			/* skipped some */
			CONS_Alert(CONS_NOTICE, "That player cannot be viewed currently.\n");
			PRINTVIEWPOINT ("Now "," instead")
		}
		else
			PRINTVIEWPOINT ("Now ",)
	}
	else/* print current view */
	{
		if (r_splitscreen < viewnum-1)/* We can't see those guys! */
			return;
		PRINTVIEWPOINT ("Currently ",)
	}
}
#undef PRINTVIEWPOINT

static void Command_SetViews_f(void)
{
	UINT8 splits;
	UINT8 newsplits;

	if (COM_Argc() != 2)
	{
		CONS_Printf("setviews <views>: set the number of split screens\n");
		return;
	}

	splits = r_splitscreen+1;

	newsplits = atoi(COM_Argv(1));
	newsplits = min(max(newsplits, 1), 4);

	if (newsplits > splits && demo.playback && multiplayer)
	{
		G_AdjustView(newsplits, 0, true);
	}
	else
	{
		// Even if the splits go beyond the real number of
		// splitscreen players, displayplayers was filled
		// with duplicates of P1 (see Got_AddPlayer).
		if (demo.playback)
		{
			G_SyncDemoParty(consoleplayer, newsplits-1);
		}
		else
		{
			r_splitscreen = newsplits-1;
			R_ExecuteSetViewSize();
		}

		// If promoting (outside of replays), make sure the
		// camera is in the correct position.
		UINT8 i;
		for (i = splits + 1; i <= newsplits; ++i)
		{
			G_FixCamera(i);
		}
	}
}

static void
Command_Invite_f (void)
{
	UINT8 buffer[1];

	int invitee;

	if (COM_Argc() != 2)
	{
		CONS_Printf("invite <player>: Invite a player to your party.\n");
		return;
	}

	if (G_PartySize(consoleplayer) >= MAXSPLITSCREENPLAYERS)
	{
		CONS_Alert(CONS_WARNING, "Your party is full!\n");
		return;
	}

	invitee = LookupPlayer(COM_Argv(1));

	if (invitee == -1)
	{
		CONS_Alert(CONS_WARNING, "There is no player by that name!\n");
		return;
	}
	if (!playeringame[invitee])
	{
		CONS_Alert(CONS_WARNING, "There is no player using that slot!\n");
		return;
	}

	if (G_IsPartyLocal(invitee))
	{
		CONS_Alert(CONS_WARNING, "That player is already a member of your party.\n");
		return;
	}

	if (splitscreen_invitations[invitee] >= 0)
	{
		CONS_Alert(CONS_WARNING,
				"That player has already been invited to join another party.\n");
		return;
	}

	if ((G_PartySize(consoleplayer) + G_LocalSplitscreenPartySize(invitee)) > MAXSPLITSCREENPLAYERS)
	{
		CONS_Alert(CONS_WARNING,
				"That player joined with too many "
				"splitscreen players for your party.\n");
		return;
	}

	CONS_Printf(
			"Inviting %s...\n",
			VaguePartyDescription(
				invitee, G_LocalSplitscreenPartySize(invitee), '\x80')
	);

	buffer[0] = invitee;

	SendNetXCmd(XD_PARTYINVITE, buffer, sizeof buffer);
}

static void
Command_CancelInvite_f (void)
{
	UINT8 buffer[1];

	int invitee;

	if (COM_Argc() != 2)
	{
		CONS_Printf("cancelinvite <player>: Rescind a party invitation.\n");
		return;
	}

	invitee = LookupPlayer(COM_Argv(1));

	if (invitee == -1)
	{
		CONS_Alert(CONS_WARNING, "There is no player by that name!\n");
		return;
	}
	if (!playeringame[invitee])
	{
		CONS_Alert(CONS_WARNING, "There is no player using that slot!\n");
		return;
	}

	if (splitscreen_invitations[invitee] != consoleplayer)
	{
		CONS_Alert(CONS_WARNING,
				"You have not invited this player!\n");
		return;
	}

	CONS_Printf(
			"Rescinding invite to %s...\n",
			VaguePartyDescription(
				invitee, G_LocalSplitscreenPartySize(invitee), '\x80')
	);

	buffer[0] = invitee;

	SendNetXCmd(XD_CANCELPARTYINVITE, buffer, sizeof buffer);
}

static boolean
CheckPartyInvite (void)
{
	if (splitscreen_invitations[consoleplayer] < 0)
	{
		CONS_Alert(CONS_WARNING, "There is no open party invitation.\n");
		return false;
	}
	return true;
}

static void
Command_AcceptInvite_f (void)
{
	if (CheckPartyInvite())
		SendNetXCmd(XD_ACCEPTPARTYINVITE, NULL, 0);
}

static void
Command_RejectInvite_f (void)
{
	if (CheckPartyInvite())
	{
		CONS_Printf("\x85Rejecting invite...\n");

		SendNetXCmd(XD_LEAVEPARTY, NULL, 0);
	}
}

static void
Command_LeaveParty_f (void)
{
	if (G_PartySize(consoleplayer) > G_LocalSplitscreenPartySize(consoleplayer))
	{
		CONS_Printf("\x85Leaving party...\n");

		SendNetXCmd(XD_LEAVEPARTY, NULL, 0);
	}
}

// ========================================================================

// play a demo, add .lmp for external demos
// eg: playdemo demo1 plays the internal game demo
//
// UINT8 *demofile; // demo file buffer
static void Command_Playdemo_f(void)
{
	const char *arg1 = NULL;
	menudemo_t menudemo = {0};

	if (COM_Argc() < 2)
	{
		CONS_Printf("playdemo <demoname> [-addfiles / -force]:\n");
		CONS_Printf(M_GetText(
					"Play back a demo file. The full path from your Kart directory must be given.\n\n"

					"* With \"-addfiles\", any required files are added from a list contained within the demo file.\n"
					"* With \"-force\", the demo is played even if the necessary files have not been added.\n"));
		return;
	}

	if (netgame)
	{
		CONS_Printf(M_GetText("You can't play a demo while in a netgame.\n"));
		return;
	}

	arg1 = COM_Argv(1);

	// Internal if no extension, external if one exists
	if (FIL_CheckExtension(arg1))
	{
		// External demos must be checked first
		sprintf(menudemo.filepath, "%s" PATHSEP "%s", srb2home, arg1);
		G_LoadDemoInfo(&menudemo, /*allownonmultiplayer*/ true);

		if (menudemo.type != MD_LOADED)
		{
			// Do nothing because the demo can't be played
			CONS_Alert(CONS_ERROR, "Unable to playdemo %s", menudemo.filepath);
			return;
		}
	}
	else
	{
		strlcpy(menudemo.filepath, arg1, sizeof(menudemo.filepath));
	}

	// disconnect from server here?
	if (demo.playback)
		G_StopDemo();

	CONS_Printf(M_GetText("Playing back demo '%s'.\n"), menudemo.filepath);

	demo.loadfiles = strcmp(COM_Argv(2), "-addfiles") == 0;
	demo.ignorefiles = strcmp(COM_Argv(2), "-force") == 0;

	G_DoPlayDemo(menudemo.filepath);
}

static void Command_Timedemo_f(void)
{
	size_t i = 0;
	const char *arg1 = NULL;
	menudemo_t menudemo = {0};

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("timedemo <demoname> [-csv [<trialid>]] [-quit]: time a demo\n"));
		return;
	}

	if (netgame)
	{
		CONS_Printf(M_GetText("You can't play a demo while in a netgame.\n"));
		return;
	}

	arg1 = COM_Argv(1);

	// Internal if no extension, external if one exists
	if (FIL_CheckExtension(arg1))
	{
		// External demos must be checked first
		sprintf(menudemo.filepath, "%s" PATHSEP "%s", srb2home, arg1);
		G_LoadDemoInfo(&menudemo, /*allownonmultiplayer*/ true);

		if (menudemo.type != MD_LOADED)
		{
			// Do nothing because the demo can't be played
			CONS_Alert(CONS_ERROR, "Unable to timedemo %s", menudemo.filepath);
			return;
		}

		strlcpy(timedemo_name, menudemo.filepath, sizeof(timedemo_name));
	}
	else
	{
		strlcpy(timedemo_name, arg1, sizeof(timedemo_name));
	}

	// disconnect from server here?
	if (demo.playback)
		G_StopDemo();

	// print timedemo results as CSV?
	i = COM_CheckParm("-csv");
	timedemo_csv = (i > 0);
	if (COM_CheckParm("-quit") != i + 1)
		strcpy(timedemo_csv_id, COM_Argv(i + 1)); // user-defined string to identify row
	else
		timedemo_csv_id[0] = 0;

	// exit after the timedemo?
	timedemo_quit = (COM_CheckParm("-quit") > 0);

	CONS_Printf(M_GetText("Timing demo '%s'.\n"), timedemo_name);

	G_TimeDemo(timedemo_name);
}

// stop current demo
static void Command_Stopdemo_f(void)
{
	G_CheckDemoStatus();
	CONS_Printf(M_GetText("Stopped demo.\n"));
}

static void Command_StartMovie_f(void)
{
	M_StartMovie(MM_AVRECORDER);
}

static void Command_StartLossless_f(void)
{
	M_StartMovie(cv_lossless_recorder.value);
}

static void Command_StopMovie_f(void)
{
	M_StopMovie();
}

INT32 mapchangepending = 0;

/** Runs a map change.
  * The supplied data are assumed to be good. If provided by a user, they will
  * have already been checked in Command_Map_f().
  *
  * Do \b NOT call this function directly from a menu! M_Responder() is called
  * from within the event processing loop, and this function calls
  * SV_SpawnServer(), which calls CL_ConnectToServer(), which gives you "Press
  * ESC to abort", which calls I_GetKey(), which adds an event. In other words,
  * 63 old events will get reexecuted, with ridiculous results. Just don't do
  * it (without setting delay to 1, which is the current solution).
  *
  * \param mapnum             Map number to change to.
  * \param gametype           Gametype to switch to.
  * \param pencoremode        Is this 'Encore Mode'?
  * \param presetplayers      1 to reset player scores and lives and such, 0 not to.
  * \param delay              Determines how the function will be executed: 0 to do
  *                           it all right now (must not be done from a menu), 1 to
  *                           do step one and prepare step two, 2 to do step two.
  * \param skipprecutscene    To skip the precutscence or not?
  * \param pforcespecialstage For certain contexts, forces a special stage.
  * \sa D_GameTypeChanged, Command_Map_f
  * \author Graue <graue@oceanbase.org>
  */
void D_MapChange(UINT16 mapnum, INT32 newgametype, boolean pencoremode, boolean presetplayers, INT32 delay, boolean skipprecutscene, boolean pforcespecialstage)
{
	static char buf[1+1+1+1+1+2+4];
	static char *buf_p = buf;
	// The supplied data are assumed to be good.
	I_Assert(delay >= 0 && delay <= 2);

	CONS_Debug(DBG_GAMELOGIC, "Map change: mapnum=%d gametype=%d pencoremode=%d presetplayers=%d delay=%d skipprecutscene=%d pforcespecialstage = %d\n",
	           mapnum, newgametype, pencoremode, presetplayers, delay, skipprecutscene, pforcespecialstage);

	if (delay != 2)
	{
		UINT8 flags = 0;
		//I_Assert(W_CheckNumForName(G_BuildMapName(mapnum)) != LUMPERROR);
		buf_p = buf;
		if (pencoremode)
			flags |= 1;
		if (presetplayers)
			flags |= 1<<1;
		if (skipprecutscene)
			flags |= 1<<2;
		if (pforcespecialstage)
			flags |= 1<<3;
		if (roundqueue.netcommunicate)
			flags |= 1<<4;
		WRITEUINT8(buf_p, flags);

		if (roundqueue.netcommunicate)
		{
			// roundqueue state
			WRITEUINT8(buf_p, roundqueue.position);
			WRITEUINT8(buf_p, roundqueue.size);
			WRITEUINT8(buf_p, roundqueue.roundnum);
			roundqueue.netcommunicate = false;
		}

		// new gametype value
		WRITEUINT16(buf_p, newgametype);

		WRITEUINT16(buf_p, mapnum);
	}

	if (delay == 1)
		mapchangepending = 1;
	else
	{
		mapchangepending = 0;
		// spawn the server if needed
		// reset players if there is a new one
		if (!IsPlayerAdmin(consoleplayer))
		{
			if (SV_SpawnServer())
				buf[0] &= ~(1<<1);
			if (!Playing()) // you failed to start a server somehow, so cancel the map change
				return;
		}

		chmappending++;

		if (netgame)
			WRITEUINT32(buf_p, M_RandomizedSeed()); // random seed
		SendNetXCmd(XD_MAP, buf, buf_p - buf);
	}
}

void D_SetupVote(INT16 newgametype)
{
	const UINT32 rules = gametypes[newgametype]->rules;

	UINT8 buf[(VOTE_NUM_LEVELS * 2) + 4];
	UINT8 *p = buf;

	INT32 i;

	UINT16 votebuffer[VOTE_NUM_LEVELS + 1];
	memset(votebuffer, UINT16_MAX, sizeof(votebuffer));

	WRITEINT16(p, newgametype);
	WRITEUINT8(p, ((cv_kartencore.value == 1) && (rules & GTR_ENCORE)));
	WRITEUINT8(p, G_SometimesGetDifferentEncore());

	UINT8 numPlayers = 0;

	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			continue;
		}

		extern consvar_t cv_forcebots; // debug

		if (!(rules & GTR_BOTS) && players[i].bot && !cv_forcebots.value)
		{
			// Gametype doesn't support bots
			continue;
		}

		numPlayers++;
	}

	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		UINT16 m = G_RandMapPerPlayerCount(
			G_TOLFlag(newgametype),
			prevmap, false,
			(i < VOTE_NUM_LEVELS-1),
			votebuffer,
			numPlayers
		);
		votebuffer[i] = m;
		WRITEUINT16(p, m);
	}

	SendNetXCmd(XD_SETUPVOTE, buf, p - buf);
}

void D_ModifyClientVote(UINT8 player, SINT8 voted)
{
	char buf[2];
	char *p = buf;
	UINT8 sendPlayer = consoleplayer;

	if (player == UINT8_MAX)
	{
		// Special game vote (map anger, duel)
		if (!server)
		{
			return;
		}
	}

	if (player == UINT8_MAX)
	{
		// special vote
		WRITEUINT8(p, UINT8_MAX);
	}
	else
	{
		INT32 i = 0;
		WRITEUINT8(p, player);

		for (i = 0; i <= splitscreen; i++)
		{
			if (g_localplayers[i] == player)
			{
				sendPlayer = i;
			}
		}
	}

	WRITESINT8(p, voted);

	SendNetXCmdForPlayer(sendPlayer, XD_MODIFYVOTE, buf, p - buf);
}

void D_PickVote(void)
{
	char buf[2];
	char* p = buf;
	SINT8 temppicks[VOTE_TOTAL];
	SINT8 templevels[VOTE_TOTAL];
	SINT8 votecompare = VOTE_NOT_PICKED;
	UINT8 numvotes = 0, key = 0;
	INT32 i;

	for (i = 0; i < VOTE_TOTAL; i++)
	{
		if (Y_PlayerIDCanVote(i) == false)
		{
			continue;
		}

		if (g_votes[i] != VOTE_NOT_PICKED)
		{
			temppicks[numvotes] = i;
			templevels[numvotes] = g_votes[i];
			numvotes++;

			if (votecompare == VOTE_NOT_PICKED)
			{
				votecompare = g_votes[i];
			}
		}
	}

	if (numvotes > 0)
	{
		key = M_RandomKey(numvotes);
		WRITESINT8(p, temppicks[key]);
		WRITESINT8(p, templevels[key]);
	}
	else
	{
		WRITESINT8(p, VOTE_NOT_PICKED);
		WRITESINT8(p, 0);
	}

	SendNetXCmd(XD_PICKVOTE, &buf, 2);
}

static char *
ConcatCommandArgv (int start, int end)
{
	char *final;

	size_t size;

	int i;
	char *p;

	size = 0;

	for (i = start; i < end; ++i)
	{
		/*
		one space after each argument, but terminating
		character on final argument
		*/
		size += strlen(COM_Argv(i)) + 1;
	}

	final = ZZ_Alloc(size);
	p = final;

	--end;/* handle the final argument separately */
	for (i = start; i < end; ++i)
	{
		p += sprintf(p, "%s ", COM_Argv(i));
	}
	/* at this point "end" is actually the last argument's position */
	strcpy(p, COM_Argv(end));

	return final;
}

//
// Warp to map code.
// Called either from map <mapname> console command, or idclev cheat.
//
// Largely rewritten by James.
//

static INT32 GetGametypeParm(size_t option_gametype)
{
	const char *gametypename;
	INT32 newgametype;

	if (COM_Argc() < option_gametype + 2)/* no argument after? */
	{
		CONS_Alert(CONS_ERROR,
				"No gametype name follows parameter '%s'.\n",
				COM_Argv(option_gametype));
		return -1;
	}

	// new gametype value
	// use current one by default
	gametypename = COM_Argv(option_gametype + 1);

	newgametype = G_GetGametypeByName(gametypename);

	if (newgametype == -1) // reached end of the list with no match
	{
		/* Did they give us a gametype number? That's okay too! */
		if (isdigit(gametypename[0]))
		{
			INT16 d = atoi(gametypename);
			if (d >= 0 && d < numgametypes)
				newgametype = d;
			else
			{
				CONS_Alert(CONS_ERROR,
						"Gametype number %d is out of range. Use a number between"
						" 0 and %d inclusive. ...Or just use the name. :v\n",
						d,
						numgametypes-1);
				return -1;
			}
		}
		else
		{
			CONS_Alert(CONS_ERROR,
					"'%s' is not a valid gametype.\n",
					gametypename);
			return -1;
		}
	}

	if (Playing() && netgame && (gametypes[newgametype]->rules & GTR_FORBIDMP))
	{
		CONS_Alert(CONS_ERROR,
				"'%s' is not a net-compatible gametype.\n",
				gametypename);
		return -1;
	}

	return newgametype;
}

static void Command_Map_f(void)
{
	size_t first_option;
	size_t option_force;
	size_t option_gametype;
	size_t option_encore;
	size_t option_skill;
	size_t option_server;
	size_t option_match;
	boolean newresetplayers;
	boolean newforcespecialstage;

	boolean usingcheats;
	boolean ischeating;

	INT32 newmapnum;

	char   *    mapname;
	char   *realmapname = NULL;

	INT32 newgametype = gametype;
	boolean newencoremode = (cv_kartencore.value == 1);

	if (Playing())
	{
		if (client && !IsPlayerAdmin(consoleplayer))
		{
			CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
			return;
		}
	}
	else if (netgame)
	{
		CONS_Printf(M_GetText("You cannot start a session while joining a server.\n"));
		return;
	}

	option_force    =   COM_CheckPartialParm("-f");
	option_gametype =   COM_CheckPartialParm("-g");
	option_encore   =   COM_CheckPartialParm("-e");
	option_skill    =   COM_CheckParm("-skill");
	option_server   =   COM_CheckParm("-server");
	option_match    =   COM_CheckParm("-match");
	newresetplayers = ! COM_CheckParm("-noresetplayers");
	newforcespecialstage = COM_CheckParm("-forcespecialstage");

	usingcheats = CV_CheatsEnabled();
	ischeating = (!(netgame || multiplayer)) || (!newresetplayers);

	if (!( first_option = COM_FirstOption() ))
		first_option = COM_Argc();

	if (first_option < 2)
	{
		/* I'm going over the fucking lines and I DON'T CAREEEEE */
		CONS_Printf("map <name / number> [-gametype <type>] [-force]:\n");
		CONS_Printf(M_GetText(
					"Warp to a map, by its name, two character code, with optional \"MAP\" prefix, or by its number (though why would you).\n"
					"All parameters are case-insensitive and may be abbreviated.\n"));
		return;
	}

	mapname = ConcatCommandArgv(1, first_option);

	newmapnum = G_FindMapByNameOrCode(mapname, &realmapname);

	if (newmapnum == 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Could not find any map described as '%s'.\n"), mapname);
		Z_Free(mapname);
		return;
	}

	if (/*newmapnum != 1 &&*/ M_MapLocked(newmapnum))
	{
		ischeating = true;
	}

	if (ischeating && !usingcheats)
	{
		CONS_Printf(M_GetText("Cheats must be enabled.\n"));
		Z_Free(realmapname);
		Z_Free(mapname);
		return;
	}

	// new gametype value
	// use current one by default
	if (option_gametype)
	{
		newgametype = GetGametypeParm(option_gametype);
		if (newgametype == -1)
		{
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}
	}
	else if (!Playing() || (netgame == false && grandprixinfo.gp == true))
	{
		newresetplayers = true;
		if (mapheaderinfo[newmapnum-1])
		{
			// Let's just guess so we don't have to specify the gametype EVERY time...
			newgametype = G_GuessGametypeByTOL(mapheaderinfo[newmapnum-1]->typeoflevel);

			if (!option_force && newgametype == -1)
			{
				CONS_Alert(CONS_WARNING, M_GetText("%s (%s) doesn't support any known gametype!\n"), realmapname, G_BuildMapName(newmapnum));
				Z_Free(realmapname);
				Z_Free(mapname);
				return;
			}

			if (newgametype == -1)
				newgametype = GT_RACE; // sensible default
		}
	}

	// new encoremode value
	if (option_encore)
	{
		newencoremode = !newencoremode;

		if (!M_SecretUnlocked(SECRET_ENCORE, false) && newencoremode == true && !usingcheats)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("You haven't unlocked Encore Mode yet!\n"));
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}
	}

	if (!option_force && newgametype == gametype && Playing()) // SRB2Kart
		newresetplayers = false; // if not forcing and gametypes is the same

	// don't use a gametype the map doesn't support
	if (cht_debug || option_force || cv_skipmapcheck.value)
	{
		// The player wants us to trek on anyway.  Do so.
	}
	else
	{
		// G_TOLFlag handles both multiplayer gametype and ignores it for !multiplayer
		if (!(
					mapheaderinfo[newmapnum-1] &&
					mapheaderinfo[newmapnum-1]->typeoflevel & G_TOLFlag(newgametype)
		))
		{
			CONS_Alert(CONS_WARNING, M_GetText("%s (%s) doesn't support %s mode!\n(Use -force to override)\n"), realmapname, G_BuildMapName(newmapnum), gametypes[newgametype]->name);
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}
	}

	{
		if ((option_match && option_server)
		|| (option_match && option_skill)
		|| (option_server && option_skill))
		{
			CONS_Alert(CONS_WARNING, M_GetText("These options can't be combined.\nSelect only one out of -server, -match, or -skill.\n"));
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}

		if (!Playing())
		{
			UINT8 ssplayers = cv_splitplayers.value-1;
			boolean newnetgame = (option_server != 0);

			multiplayer = true;
			netgame = false;

			if (cv_maxconnections.value < ssplayers+1)
				CV_SetValue(&cv_maxconnections, ssplayers+1);

			SV_StartSinglePlayerServer(newgametype, newnetgame);

			if (splitscreen != ssplayers)
			{
				splitscreen = ssplayers;
				SplitScreen_OnChange();
			}

			if (!newnetgame && (newgametype != GT_TUTORIAL) && option_match == 0)
			{
				grandprixinfo.gp = true;
				grandprixinfo.initalize = true;
				grandprixinfo.cup = NULL;

				grandprixinfo.gamespeed = (cv_kartspeed.value == KARTSPEED_AUTO ? KARTSPEED_NORMAL : cv_kartspeed.value);
				grandprixinfo.masterbots = false;
			}

			if (newnetgame)
			{
				restoreMenu = &PLAY_MP_OptSelectDef;
			}
			else
			{
				restoreMenu = NULL;
			}

			M_ClearMenus(true);
		}
		else if (
			((grandprixinfo.gp == true ? option_match : option_skill) != 0) // Can't swap between.
			|| (!netgame && (option_server != 0)) // Can't promote to server.
		)
		{
			CONS_Alert(CONS_WARNING, M_GetText("You are already playing a game.\nReturn to the menu to use this option.\n"));
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}

		if (grandprixinfo.gp)
		{
			grandprixinfo.wonround = false;

			if (option_skill)
			{
				const char *skillname = COM_Argv(option_skill + 1);
				INT32 newskill = -1;
				INT32 j;

				for (j = 0; gpdifficulty_cons_t[j].strvalue; j++)
				{
					if (!strcasecmp(gpdifficulty_cons_t[j].strvalue, skillname))
					{
						newskill = (INT16)gpdifficulty_cons_t[j].value;
						break;
					}
				}

				if (!gpdifficulty_cons_t[j].strvalue) // reached end of the list with no match
				{
					INT32 num = atoi(COM_Argv(option_skill + 1)); // assume they gave us a skill number, which is okay too
					if (num >= KARTSPEED_EASY && num <= KARTGP_MASTER)
						newskill = (INT16)num;
				}

				if (newskill != -1)
				{
					if (newskill == KARTGP_MASTER)
					{
						grandprixinfo.gamespeed = KARTSPEED_HARD;
						grandprixinfo.masterbots = true;
					}
					else
					{
						grandprixinfo.gamespeed = newskill;
						grandprixinfo.masterbots = false;
					}
				}
			}
		}
	}

	D_MapChange(newmapnum, newgametype, newencoremode, newresetplayers, 0, false, newforcespecialstage);

	Z_Free(realmapname);
	Z_Free(mapname);
}

/** Receives a map command and changes the map.
  *
  * \param cp        Data buffer.
  * \param playernum Player number responsible for the message. Should be
  *                  ::serverplayer or ::adminplayer.
  * \sa D_MapChange
  */
static void Got_Mapcmd(const UINT8 **cp, INT32 playernum)
{
	UINT8 flags;
	INT32 presetplayer = 1;
	UINT8 skipprecutscene, pforcespecialstage;
	boolean pencoremode, hasroundqueuedata;
	UINT16 mapnumber, lastgametype;

	forceresetplayers = deferencoremode = false;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal map change received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	flags = READUINT8(*cp);

	pencoremode = ((flags & 1) != 0);

	presetplayer = ((flags & (1<<1)) != 0);

	skipprecutscene = ((flags & (1<<2)) != 0);

	pforcespecialstage = ((flags & (1<<3)) != 0);

	hasroundqueuedata = ((flags & (1<<4)) != 0);

	if (hasroundqueuedata)
	{
		UINT8 position = READUINT8(*cp);
		UINT8 size = READUINT8(*cp);
		UINT8 roundnum = READUINT8(*cp);

		if (playernum != serverplayer // Clients, even admin clients, don't have full roundqueue data
			|| position > size // Sanity check A (intentionally not a >= comparison)
			|| size > ROUNDQUEUE_MAX) // Sanity Check B (ditto)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal round-queue data received from %s\n"), player_names[playernum]);
			if (server && playernum != serverplayer)
				SendKick(playernum, KICK_MSG_CON_FAIL);
			return;
		}

		roundqueue.position = position;
		if (size < roundqueue.size)
		{
			// Bafooliganism afoot - set to zero if the size is zero! ~toast 150523
			roundqueue.size = size;
		}
		else while (roundqueue.size < size)
		{
			// We wipe rather than provide full data to prevent bloating the packet,
			// and because only this data is necessary for sync. ~toast 100423
			memset(&roundqueue.entries[roundqueue.size], 0, sizeof(roundentry_t));
			roundqueue.size++;
		}
		roundqueue.roundnum = roundnum; // no sanity checking required, server is authoriative
	}

	// No more kicks below this line, we can now start modifying state beyond this function.
	if (chmappending)
		chmappending--;

	lastgametype = gametype;
	gametype = READUINT16(*cp);
	G_SetGametype(gametype); // I fear putting that macro as an argument

	if (gametype < 0 || gametype >= numgametypes)
		gametype = lastgametype;
	else if (gametype != lastgametype)
		D_GameTypeChanged(lastgametype); // emulate consvar_t behavior for gametype

	if (hasroundqueuedata && roundqueue.position > 0 && roundqueue.size > 0)
	{
		// ...we can evaluate CURRENT specifics for roundqueue data, though.
		roundqueue.entries[roundqueue.position-1].gametype = gametype;
		roundqueue.entries[roundqueue.position-1].encore = pencoremode;
	}

	if (!(gametyperules & GTR_ENCORE))
		pencoremode = false;

	mapnumber = READUINT16(*cp);

	// Handle some Grand Prix state.
	if (grandprixinfo.gp)
	{
		boolean caughtretry = (gametype == lastgametype
				&& mapnumber == gamemap);
		if (pforcespecialstage // Forced.
			|| (caughtretry && grandprixinfo.eventmode == GPEVENT_SPECIAL) // Catch retries of forced.
			|| (roundqueue.size == 0 && (gametyperules & (GTR_BOSS|GTR_CATCHER)))) // Force convention for the (queue)map command.
		{
			grandprixinfo.eventmode = GPEVENT_SPECIAL;

			if (pforcespecialstage == true && gamedata->everseenspecial == false)
			{
				gamedata->everseenspecial = true;
				// No need to do anything else here -- P_LoadLevel will get this for us!
				//M_UpdateUnlockablesAndExtraEmblems(true, true);
				//gamedata->deferredsave = true;
			}
		}
		else if (gametype != GT_RACE)
		{
			grandprixinfo.eventmode = GPEVENT_BONUS;
		}
		else
		{
			grandprixinfo.eventmode = GPEVENT_NONE;
		}
	}

	if (netgame)
		P_ClearRandom(READUINT32(*cp));

	if (!skipprecutscene)
	{
		DEBFILE(va("Warping to %s [resetplayer=%d lastgametype=%d gametype=%d cpnd=%d]\n",
			G_BuildMapName(mapnumber), presetplayer, lastgametype, gametype, chmappending));
		CON_LogMessage(M_GetText("Speeding off to level...\n"));
	}

	if (demo.playback && !demo.timing)
		precache = false;

	demo.willsave = (cv_recordmultiplayerdemos.value == 2);
	demo.savebutton = 0;

	G_InitNew(pencoremode, mapnumber, presetplayer, skipprecutscene);
	if (demo.playback && !demo.timing)
		precache = true;
	if (demo.timing)
		G_DoneLevelLoad();

#ifdef HAVE_DISCORDRPC
	DRPC_UpdatePresence();
#endif
}

static void Command_RandomMap(void)
{
	INT32 oldmapnum;
	INT32 newmapnum;
	INT32 newgametype = (Playing() ? gametype : menugametype);
	boolean newencore = false;
	boolean newresetplayers;
	size_t option_gametype;

	if (client && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	if ((option_gametype = COM_CheckPartialParm("-g")))
	{
		newgametype = GetGametypeParm(option_gametype);
		if (newgametype == -1)
			return;
	}

	// TODO: Handle singleplayer conditions.
	// The existing ones are way too annoyingly complicated and "anti-cheat" for my tastes.

	if (Playing())
	{
		if (cv_kartencore.value == 1 && (gametypes[newgametype]->rules & GTR_ENCORE))
		{
			newencore = true;
		}
		newresetplayers = false;

		if (gamestate == GS_LEVEL)
		{
			oldmapnum = gamemap-1;
		}
		else
		{
			oldmapnum = prevmap;
		}
	}
	else
	{
		newresetplayers = true;
		oldmapnum = -1;
	}

	newmapnum = G_RandMap(G_TOLFlag(newgametype), oldmapnum, false, false, NULL) + 1;
	D_MapChange(newmapnum, newgametype, newencore, newresetplayers, 0, false, false);
}

static void Command_RestartLevel(void)
{
	boolean newencore = false;

	if (client && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	if (!Playing())
	{
		CONS_Printf(M_GetText("You must be in a game to use this.\n"));
		return;
	}

	if (K_CanChangeRules(false) == false && CV_CheatsEnabled() == false)
	{
		CONS_Printf(M_GetText("Cheats must be enabled.\n"));
		return;
	}

	if (cv_kartencore.value != 0)
	{
		newencore = (cv_kartencore.value == 1) || encoremode;
	}

	D_MapChange(gamemap, g_lastgametype, newencore, false, 0, false, false);
}

static void Handle_MapQueueSend(UINT16 newmapnum, UINT16 newgametype, boolean newencoremode)
{
	static char buf[1+2+2];
	static char *buf_p = buf;

	UINT8 flags = 0;
	boolean doclear = (newgametype == ROUNDQUEUE_CLEAR);

	CONS_Debug(DBG_GAMELOGIC, "Map queue: mapnum=%d newgametype=%d newencoremode=%d\n",
	           newmapnum, newgametype, newencoremode);

	buf_p = buf;

	if (newencoremode)
		flags |= 1;

	WRITEUINT8(buf_p, flags);
	WRITEUINT16(buf_p, newgametype);

	if (client)
	{
		WRITEUINT16(buf_p, newmapnum);
		SendNetXCmd(XD_REQMAPQUEUE, buf, buf_p - buf);
		return;
	}

	WRITEUINT8(buf_p, roundqueue.size);

	if (doclear == true)
	{
		memset(&roundqueue, 0, sizeof(struct roundqueue));
	}
	else
	{
		G_MapIntoRoundQueue(newmapnum, newgametype, newencoremode, false);
	}

	SendNetXCmd(XD_MAPQUEUE, buf, buf_p - buf);
}

static void Command_QueueMap_f(void)
{
	size_t first_option;
	size_t option_force;
	size_t option_gametype;
	size_t option_encore;
	size_t option_clear;

	boolean usingcheats;
	boolean ischeating;

	INT32 newmapnum;

	char   *    mapname;
	char   *realmapname = NULL;

	INT32 newgametype = gametype;
	boolean newencoremode = (cv_kartencore.value == 1);

	if (!Playing())
	{
		CONS_Printf(M_GetText("Levels can only be queued in-game.\n"));
		return;
	}

	if (client && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	usingcheats = CV_CheatsEnabled();
	ischeating = (!(netgame || multiplayer) || !K_CanChangeRules(false));

	option_clear = COM_CheckParm("-clear");

	if (option_clear)
	{
		if (ischeating && !usingcheats)
		{
			CONS_Printf(M_GetText("Cheats must be enabled.\n"));
			return;
		}

		if (roundqueue.size == 0)
		{
			CONS_Printf(M_GetText("Round queue is already empty!\n"));
			return;
		}

		Handle_MapQueueSend(0, ROUNDQUEUE_CLEAR, false);
		return;
	}

	if (roundqueue.size >= ROUNDQUEUE_MAX)
	{
		CONS_Printf(M_GetText("Round queue is currently full.\n"));
		return;
	}

	option_force    =   COM_CheckPartialParm("-f");
	option_gametype =   COM_CheckPartialParm("-g");
	option_encore   =   COM_CheckPartialParm("-e");

	if (!( first_option = COM_FirstOption() ))
		first_option = COM_Argc();

	if (first_option < 2)
	{
		/* I'm going over the fucking lines and I DON'T CAREEEEE */
		CONS_Printf("queuemap <name / number> [-gametype <type>] [-force] / [-clear]:\n");
		CONS_Printf(M_GetText(
					"Queue up a map by its name, or by its number (though why would you).\n"
					"All parameters are case-insensitive and may be abbreviated.\n"));
		return;
	}

	mapname = ConcatCommandArgv(1, first_option);

	newmapnum = G_FindMapByNameOrCode(mapname, &realmapname);

	if (newmapnum == 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Could not find any map described as '%s'.\n"), mapname);
		Z_Free(mapname);
		return;
	}

	if (/*newmapnum != 1 &&*/ M_MapLocked(newmapnum))
	{
		ischeating = true;
	}

	if (ischeating && !usingcheats)
	{
		CONS_Printf(M_GetText("Cheats must be enabled.\n"));
		Z_Free(realmapname);
		Z_Free(mapname);
		return;
	}

	// new gametype value
	// use current one by default
	if (option_gametype)
	{
		newgametype = GetGametypeParm(option_gametype);
		if (newgametype == -1)
		{
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}
	}

	// new encoremode value
	if (option_encore)
	{
		newencoremode = !newencoremode;

		if (!M_SecretUnlocked(SECRET_ENCORE, false) && newencoremode == true && !usingcheats)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("You haven't unlocked Encore Mode yet!\n"));
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}
	}

	// don't use a gametype the map doesn't support
	if (cht_debug || option_force || cv_skipmapcheck.value)
	{
		// The player wants us to trek on anyway.  Do so.
	}
	else
	{
		// G_TOLFlag handles both multiplayer gametype and ignores it for !multiplayer
		if (!(
					mapheaderinfo[newmapnum-1] &&
					mapheaderinfo[newmapnum-1]->typeoflevel & G_TOLFlag(newgametype)
		))
		{
			CONS_Alert(CONS_WARNING, M_GetText("%s (%s) doesn't support %s mode!\n(Use -force to override)\n"), realmapname, G_BuildMapName(newmapnum), gametypes[newgametype]->name);
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}
	}

	Handle_MapQueueSend(newmapnum-1, newgametype, newencoremode);

	Z_Free(realmapname);
	Z_Free(mapname);
}

static void Got_RequestMapQueuecmd(const UINT8 **cp, INT32 playernum)
{
	UINT8 flags;
	boolean setencore;
	UINT16 mapnumber, setgametype;
	boolean doclear = false;

	flags = READUINT8(*cp);

	setencore = ((flags & 1) != 0);

	setgametype = READUINT16(*cp);

	mapnumber = READUINT16(*cp);

	if (!IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal request map queue command received from %s\n"), player_names[playernum]);
		if (server && playernum != serverplayer)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	doclear = (setgametype == ROUNDQUEUE_CLEAR);

	if (doclear == true)
	{
		if (roundqueue.size == 0)
		{
			CONS_Alert(CONS_ERROR, "queuemap: Queue is already empty!\n");
			return;
		}
	}
	else if (roundqueue.size >= ROUNDQUEUE_MAX)
	{
		CONS_Alert(CONS_ERROR, "queuemap: Unable to add map beyond %u\n", roundqueue.size);
		return;
	}

	if (client)
		return;

	Handle_MapQueueSend(mapnumber, setgametype, setencore);
}

static void Got_MapQueuecmd(const UINT8 **cp, INT32 playernum)
{
	UINT8 flags, queueposition, i;
	boolean setencore;
	UINT16 setgametype;
	boolean doclear = false;

	flags = READUINT8(*cp);

	setencore = ((flags & 1) != 0);

	setgametype = READUINT16(*cp);

	queueposition = READUINT8(*cp);

	if (playernum != serverplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal map queue command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	doclear = (setgametype == ROUNDQUEUE_CLEAR);

	if (doclear == false && queueposition >= ROUNDQUEUE_MAX)
	{
		CONS_Alert(CONS_ERROR, "queuemap: Unable to add map beyond %u\n", roundqueue.size);
		return;
	}

	if (!server)
	{
		if (doclear == true)
		{
			memset(&roundqueue, 0, sizeof(struct roundqueue));
		}
		else
		{
			while (roundqueue.size <= queueposition)
			{
				memset(&roundqueue.entries[roundqueue.size], 0, sizeof(roundentry_t));
				roundqueue.size++;
			}

			G_MapSlipIntoRoundQueue(queueposition, 0, setgametype, setencore, false);
		}

		for (i = 0; i <= splitscreen; i++)
		{
			if (!IsPlayerAdmin(g_localplayers[i]))
				continue;
			break;
		}

		if (i > splitscreen)
			return;
	}

	if (doclear)
		CONS_Printf("queuemap: The round queue was cleared.\n");
	else
		CONS_Printf("queuemap: A map was added to the round queue (pos. %u)\n", queueposition+1);
}

static void Command_Pause(void)
{
	UINT8 buf[2];
	UINT8 *cp = buf;

	if (COM_Argc() > 1)
		WRITEUINT8(cp, (char)(atoi(COM_Argv(1)) != 0));
	else
		WRITEUINT8(cp, (char)(!paused));

	if (dedicated)
		WRITEUINT8(cp, 1);
	else
		WRITEUINT8(cp, 0);

	if (cv_pause.value || server || (IsPlayerAdmin(consoleplayer)))
	{
		if (!paused && (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_VOTING || gamestate == GS_WAITINGPLAYERS)))
		{
			CONS_Printf(M_GetText("You can't pause here.\n"));
			return;
		}
		// TODO: this would make a great debug feature for release
#ifndef DEVELOP
		else if (modeattacking)	// in time attack, pausing restarts the map
		{
			//M_ModeAttackRetry(0);	// directly call from m_menu;
			return;
		}
#endif

		SendNetXCmd(XD_PAUSE, &buf, 2);
	}
	else
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
}

static void Got_Pause(const UINT8 **cp, INT32 playernum)
{
	UINT8 dedicatedpause = false;
	const char *playername;

	if (netgame && !cv_pause.value && playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal pause command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	// TODO: this would make a great debug feature for release
#ifndef DEVELOP
	if (modeattacking && !demo.playback)
		return;
#endif

	paused = READUINT8(*cp);
	dedicatedpause = READUINT8(*cp);

	if (!demo.playback)
	{
		if (netgame)
		{
			if (dedicatedpause)
				playername = "SERVER";
			else
				playername = player_names[playernum];

			if (paused)
				CONS_Printf(M_GetText("Game paused by %s\n"), playername);
			else
				CONS_Printf(M_GetText("Game unpaused by %s\n"), playername);
		}

		if (paused)
		{
			if (!menuactive || netgame)
				S_PauseAudio();
		}
		else
			S_ResumeAudio();
	}

	I_UpdateMouseGrab();
	G_ResetAllDeviceRumbles();
}

/** Deals with an ::XD_RANDOMSEED message in a netgame.
  * These messages set the position of the random number LUT and are crucial to
  * correct synchronization.
  *
  * Such a message should only ever come from the ::serverplayer. If it comes
  * from any other player, it is ignored.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer.
  * \author Graue <graue@oceanbase.org>
  */
static void Got_RandomSeed(const UINT8 **cp, INT32 playernum)
{
	UINT32 seed;

	seed = READUINT32(*cp);

	if (playernum != serverplayer) // it's not from the server, wtf?
		return;

	// Sal: this seems unused, so this is probably fine?
	// If it needs specific RNG classes, then it should be easy to add.
	P_ClearRandom(seed);
}

/** Clears all players' scores in a netgame.
  * Only the server or a remote admin can use this command, for obvious reasons.
  *
  * \sa XD_CLEARSCORES, Got_Clearscores
  * \author SSNTails <http://www.ssntails.org>
  */
static void Command_Clearscores_f(void)
{
	if (!(server || (IsPlayerAdmin(consoleplayer))))
		return;

	SendNetXCmd(XD_CLEARSCORES, NULL, 1);
}

/** Handles an ::XD_CLEARSCORES message, which resets all players' scores in a
  * netgame to zero.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer
  *                  or ::adminplayer.
  * \sa XD_CLEARSCORES, Command_Clearscores_f
  * \author SSNTails <http://www.ssntails.org>
  */
static void Got_Clearscores(const UINT8 **cp, INT32 playernum)
{
	INT32 i;

	(void)cp;
	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal clear scores command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
		players[i].score = 0;

	CONS_Printf(M_GetText("Scores have been reset by the server.\n"));
}

// Team changing functions
static void HandleTeamChangeCommand(UINT8 localplayer)
{
	const char *commandname = NULL;
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	switch (localplayer)
	{
		case 0:
			commandname = "changeteam";
			break;
		default:
			commandname = va("changeteam%d", localplayer+1);
			break;
	}

	//      0         1
	// changeteam  <color>

	if (COM_Argc() <= 1)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("%s <team>: switch to a new team (%s)\n"), commandname, "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("%s <team>: switch to a new team (%s)\n"), commandname, "spectator or playing");
		else
			CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (!strcasecmp(COM_Argv(1), "red") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(1), "blue") || !strcasecmp(COM_Argv(1), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else if (!strcasecmp(COM_Argv(1), "playing") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 3;
		else
			error = true;
	}
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (error)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("%s <team>: switch to a new team (%s)\n"), commandname, "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("%s <team>: switch to a new team (%s)\n"), commandname, "spectator or playing");
		return;
	}

	if (players[g_localplayers[localplayer]].spectator)
		error = !(NetPacket.packet.newteam || (players[g_localplayers[localplayer]].pflags & PF_WANTSTOJOIN)); // :lancer:
	else if (G_GametypeHasTeams())
		error = (NetPacket.packet.newteam == players[g_localplayers[localplayer]].ctfteam);
	else if (G_GametypeHasSpectators() && !players[g_localplayers[localplayer]].spectator)
		error = (NetPacket.packet.newteam == 3);
#ifdef PARANOIA
	else
		I_Error("Invalid gametype after initial checks!");
#endif

	if (error)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You're already on that team!\n"));
		return;
	}

	if (!cv_allowteamchange.value && NetPacket.packet.newteam) // allow swapping to spectator even in locked teams.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("The server is not allowing team changes at the moment.\n"));
		return;
	}

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmdForPlayer(localplayer, XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

static void Command_Teamchange_f(void)
{
	HandleTeamChangeCommand(0);
}

static void Command_Teamchange2_f(void)
{
	HandleTeamChangeCommand(1);
}

static void Command_Teamchange3_f(void)
{
	HandleTeamChangeCommand(2);
}

static void Command_Teamchange4_f(void)
{
	HandleTeamChangeCommand(3);
}

static void Command_ServerTeamChange_f(void)
{
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	if (!(server || (IsPlayerAdmin(consoleplayer))))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	//        0              1         2
	// serverchangeteam <playernum>  <team>

	if (COM_Argc() < 3)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "spectator or playing");
		else
			CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (!strcasecmp(COM_Argv(2), "red") || !strcasecmp(COM_Argv(2), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(2), "blue") || !strcasecmp(COM_Argv(2), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(2), "spectator") || !strcasecmp(COM_Argv(2), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!strcasecmp(COM_Argv(2), "spectator") || !strcasecmp(COM_Argv(2), "0"))
			NetPacket.packet.newteam = 0;
		else if (!strcasecmp(COM_Argv(2), "playing") || !strcasecmp(COM_Argv(2), "1"))
			NetPacket.packet.newteam = 3;
		else
			error = true;
	}
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (error)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "spectator or playing");
		return;
	}

	NetPacket.packet.playernum = atoi(COM_Argv(1));

	if (!playeringame[NetPacket.packet.playernum])
	{
		CONS_Alert(CONS_NOTICE, M_GetText("There is no player %d!\n"), NetPacket.packet.playernum);
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam == players[NetPacket.packet.playernum].ctfteam ||
			(players[NetPacket.packet.playernum].spectator && !NetPacket.packet.newteam))
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if ((players[NetPacket.packet.playernum].spectator && !NetPacket.packet.newteam) ||
			(!players[NetPacket.packet.playernum].spectator && NetPacket.packet.newteam == 3))
			error = true;
	}
#ifdef PARANOIA
	else
		I_Error("Invalid gametype after initial checks!");
#endif

	if (error)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("That player is already on that team!\n"));
		return;
	}

	NetPacket.packet.verification = true; // This signals that it's a server change

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

void P_SetPlayerSpectator(INT32 playernum)
{
	//Make sure you're in the right gametype.
	if (!G_GametypeHasTeams() && !G_GametypeHasSpectators())
		return;

	// Don't duplicate efforts.
	if (players[playernum].spectator)
		return;

	players[playernum].spectator = true;
	players[playernum].pflags &= ~PF_WANTSTOJOIN;

	players[playernum].playerstate = PST_REBORN;
}

//todo: This and the other teamchange functions are getting too long and messy. Needs cleaning.
static void Got_Teamchange(const UINT8 **cp, INT32 playernum)
{
	changeteam_union NetPacket;
	boolean error = false, wasspectator = false;
	NetPacket.value.l = NetPacket.value.b = READINT16(*cp);

	if (!G_GametypeHasTeams() && !G_GametypeHasSpectators()) //Make sure you're in the right gametype.
	{
		// this should never happen unless the client is hacked/buggy
		CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
	}

	if (NetPacket.packet.verification) // Special marker that the server sent the request
	{
		if (playernum != serverplayer && (!IsPlayerAdmin(playernum)))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
			if (server)
				SendKick(playernum, KICK_MSG_CON_FAIL);
			return;
		}
		playernum = NetPacket.packet.playernum;
	}

	// Prevent multiple changes in one go.
	if (players[playernum].spectator && !(players[playernum].pflags & PF_WANTSTOJOIN) && !NetPacket.packet.newteam)
		return;
	else if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam && (NetPacket.packet.newteam == (unsigned)players[playernum].ctfteam))
			return;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!players[playernum].spectator && NetPacket.packet.newteam == 3)
			return;
	}
	else
	{
		if (playernum != serverplayer && (!IsPlayerAdmin(playernum)))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
			if (server)
				SendKick(playernum, KICK_MSG_CON_FAIL);
		}
		return;
	}

	// Don't switch team, just go away, please, go awaayyyy, aaauuauugghhhghgh
	if (!LUA_HookTeamSwitch(&players[playernum], NetPacket.packet.newteam, players[playernum].spectator, NetPacket.packet.autobalance, NetPacket.packet.scrambled))
		return;

	//Make sure that the right team number is sent. Keep in mind that normal clients cannot change to certain teams in certain gametypes.
#ifdef PARANOIA
	if (!G_GametypeHasTeams() && !G_GametypeHasSpectators())
		I_Error("Invalid gametype after initial checks!");
#endif

	if (!cv_allowteamchange.value)
	{
		if (!NetPacket.packet.verification && NetPacket.packet.newteam)
			error = true; //Only admin can change status, unless changing to spectator.
	}

	if (server && ((NetPacket.packet.newteam < 0 || NetPacket.packet.newteam > 3) || error))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
		SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	//Safety first!
	// (not respawning spectators here...)
	wasspectator = (players[playernum].spectator == true);

	if (!wasspectator)
	{
		if (gamestate == GS_LEVEL && players[playernum].mo)
		{
			// The following will call P_SetPlayerSpectator if successful
			P_DamageMobj(players[playernum].mo, NULL, NULL, 1, DMG_SPECTATOR);
		}

		//...but because the above could return early under some contexts, we try again here
		P_SetPlayerSpectator(playernum);
	}

	//Now that we've done our error checking and killed the player
	//if necessary, put the player on the correct team/status.

	// This serves us in both teamchange contexts.
	if (NetPacket.packet.newteam != 0)
	{
		players[playernum].pflags |= PF_WANTSTOJOIN;
	}
	else
	{
		players[playernum].pflags &= ~PF_WANTSTOJOIN;
	}

	if (G_GametypeHasTeams())
	{
		// This one is, of course, specific.
		players[playernum].ctfteam = NetPacket.packet.newteam;
	}

	if (NetPacket.packet.autobalance)
	{
		if (NetPacket.packet.newteam == 1)
			CONS_Printf(M_GetText("%s was autobalanced to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
		else if (NetPacket.packet.newteam == 2)
			CONS_Printf(M_GetText("%s was autobalanced to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.scrambled)
	{
		if (NetPacket.packet.newteam == 1)
			CONS_Printf(M_GetText("%s was scrambled to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
		else if (NetPacket.packet.newteam == 2)
			CONS_Printf(M_GetText("%s was scrambled to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 1)
	{
		CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 2)
	{
		CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 0 && !wasspectator)
		HU_AddChatText(va("\x82*%s became a spectator.", player_names[playernum]), false); // "entered the game" text was moved to P_SpectatorJoinGame

	if (gamestate != GS_LEVEL || wasspectator == true)
		return;

	FinalisePlaystateChange(playernum);
}

//
// Attempts to make password system a little sane without
// rewriting the entire goddamn XD_file system
//
#define BASESALT "basepasswordstorage"

void D_SetPassword(const char *pw)
{
	D_MD5PasswordPass((const UINT8 *)pw, strlen(pw), BASESALT, &adminpassmd5);
	adminpasswordset = true;
}

// Remote Administration
static void Command_Changepassword_f(void)
{
#ifdef NOMD5
	// If we have no MD5 support then completely disable XD_LOGIN responses for security.
	CONS_Alert(CONS_NOTICE, "Remote administration commands are not supported in this build.\n");
#else
	if (client) // cannot change remotely
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("password <password>: change remote admin password\n"));
		return;
	}

	D_SetPassword(COM_Argv(1));
	CONS_Printf(M_GetText("Password set.\n"));
#endif
}

static void Command_Login_f(void)
{
#ifdef NOMD5
	// If we have no MD5 support then completely disable XD_LOGIN responses for security.
	CONS_Alert(CONS_NOTICE, "Remote administration commands are not supported in this build.\n");
#else
	const char *pw;

	if (!netgame)
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	// If the server uses login, it will effectively just remove admin privileges
	// from whoever has them. This is good.
	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("login <password>: Administrator login\n"));
		return;
	}

	pw = COM_Argv(1);

	// Do the base pass to get what the server has (or should?)
	D_MD5PasswordPass((const UINT8 *)pw, strlen(pw), BASESALT, &netbuffer->u.md5sum);

	// Do the final pass to get the comparison the server will come up with
	D_MD5PasswordPass(netbuffer->u.md5sum, 16, va("PNUM%02d", consoleplayer), &netbuffer->u.md5sum);

	CONS_Printf(M_GetText("Sending login... (Notice only given if password is correct.)\n"));

	netbuffer->packettype = PT_LOGIN;
	HSendPacket(servernode, true, 0, 16);
#endif
}

boolean IsPlayerAdmin(INT32 playernum)
{
#if 0 // defined(DEVELOP)
	return playernum != serverplayer;
#else
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
		if (playernum == adminplayers[i])
			return true;

	return false;
#endif
}

void SetAdminPlayer(INT32 playernum)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playernum == adminplayers[i])
			return; // Player is already admin

		if (adminplayers[i] == -1)
		{
			adminplayers[i] = playernum; // Set the player to a free spot
			break; // End the loop now. If it keeps going, the same player might get assigned to two slots.
		}
	}
}

void ClearAdminPlayers(void)
{
	memset(adminplayers, -1, sizeof(adminplayers));
}

void RemoveAdminPlayer(INT32 playernum)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
		if (playernum == adminplayers[i])
			adminplayers[i] = -1;
}

static void Command_Verify_f(void)
{
	if (client)
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (!netgame)
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("promote <playernum>: give admin privileges to a player\n"));
		return;
	}

	INT32 playernum = atoi(COM_Argv(1));

	if (playernum >= 0 && playernum < MAXPLAYERS && playeringame[playernum])
	{
		UINT8 buf[1] = {playernum};
		SendNetXCmd(XD_VERIFIED, buf, 1);
	}
}

static void Got_Verification(const UINT8 **cp, INT32 playernum)
{
	INT16 num = READUINT8(*cp);

	if (playernum != serverplayer) // it's not from the server (hacker or bug)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal verification received from %s (serverplayer is %s)\n"), player_names[playernum], player_names[serverplayer]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	SetAdminPlayer(num);

	if (num != consoleplayer)
		return;

	CONS_Printf(M_GetText("You are now a server administrator.\n"));
}

static void Command_RemoveAdmin_f(void)
{
	if (client)
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("demote <playernum>: remove admin privileges from a player\n"));
		return;
	}

	INT32 playernum = atoi(COM_Argv(1));

	if (playernum >= 0 && playernum < MAXPLAYERS && playeringame[playernum])
	{
		UINT8 buf[1] = {playernum};
		SendNetXCmd(XD_DEMOTED, buf, 1);
	}
}

static void Got_Removal(const UINT8 **cp, INT32 playernum)
{
	UINT8 num = READUINT8(*cp);

	if (playernum != serverplayer) // it's not from the server (hacker or bug)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal demotion received from %s (serverplayer is %s)\n"), player_names[playernum], player_names[serverplayer]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	RemoveAdminPlayer(num);

	if (num != consoleplayer)
		return;

	CONS_Printf(M_GetText("You are no longer a server administrator.\n"));
}

void Schedule_Run(void)
{
	size_t i;

	if (schedule_len == 0)
	{
		// No scheduled tasks to run.
		return;
	}

	if (!cv_schedule.value)
	{
		// We don't WANT to run tasks.
		return;
	}

	if (K_CanChangeRules(false) == false)
	{
		// Don't engage in automation while in a restricted context.
		return;
	}

	for (i = 0; i < schedule_len; i++)
	{
		scheduleTask_t *task = schedule[i];

		if (task == NULL)
		{
			// Shouldn't happen.
			break;
		}

		if (task->timer > 0)
		{
			task->timer--;
		}

		if (task->timer == 0)
		{
			// Reset timer
			task->timer = task->basetime;

			// Run command for server
			if (server)
			{
				CONS_Printf(
					"%d seconds elapsed, running \"" "\x82" "%s" "\x80" "\".\n",
					task->basetime,
					task->command
				);

				COM_BufAddText(task->command);
				COM_BufAddText("\n");
			}
		}
	}
}

void Schedule_Insert(scheduleTask_t *addTask)
{
	if (schedule_len >= schedule_size)
	{
		if (schedule_size == 0)
		{
			schedule_size = 8;
		}
		else
		{
			schedule_size *= 2;
		}

		schedule = Z_ReallocAlign(
			(void*) schedule,
			sizeof(scheduleTask_t*) * schedule_size,
			PU_STATIC,
			NULL,
			sizeof(scheduleTask_t*) * 8
		);
	}

	schedule[schedule_len] = addTask;
	schedule_len++;
}

void Schedule_Add(INT16 basetime, INT16 timeleft, const char *command)
{
	scheduleTask_t *task = (scheduleTask_t*) Z_CallocAlign(
		sizeof(scheduleTask_t),
		PU_STATIC,
		NULL,
		sizeof(scheduleTask_t) * 8
	);

	task->basetime = basetime;
	task->timer = timeleft;
	task->command = Z_StrDup(command);

	Schedule_Insert(task);
}

void Schedule_Clear(void)
{
	size_t i;

	for (i = 0; i < schedule_len; i++)
	{
		scheduleTask_t *task = schedule[i];

		if (task->command)
			Z_Free(task->command);
	}

	schedule_len = 0;
	schedule_size = 0;
	schedule = NULL;
}

void Automate_Set(automateEvents_t type, const char *command)
{
	if (!server)
	{
		// Since there's no list command or anything for this,
		// we don't need this code to run for anyone but the server.
		return;
	}

	if (automate_commands[type] != NULL)
	{
		// Free the old command.
		Z_Free(automate_commands[type]);
	}

	if (command == NULL || strlen(command) == 0)
	{
		// Remove the command.
		automate_commands[type] = NULL;
	}
	else
	{
		// New command.
		automate_commands[type] = Z_StrDup(command);
	}
}

void Automate_Run(automateEvents_t type)
{
	extern consvar_t cv_automate;

	if (!server)
	{
		// Only the server should be doing this.
		return;
	}

	if (K_CanChangeRules(false) == false)
	{
		// Don't engage in automation while in a restricted context.
		return;
	}

#ifdef PARANOIA
	if (type >= AEV__MAX)
	{
		// Shouldn't happen.
		I_Error("Attempted to run invalid automation type.");
		return;
	}
#endif

	if (!cv_automate.value)
	{
		// We don't want to run automation.
		return;
	}

	if (automate_commands[type] == NULL)
	{
		// No command to run.
		return;
	}

	CONS_Printf(
		"Running %s automate command \"" "\x82" "%s" "\x80" "\"...\n",
		automate_names[type],
		automate_commands[type]
	);

	COM_BufAddText(automate_commands[type]);
	COM_BufAddText("\n");
}

void Automate_Clear(void)
{
	size_t i;

	for (i = 0; i < AEV__MAX; i++)
	{
		Automate_Set(i, NULL);
	}
}

void LiveStudioAudience(void)
{
	if (livestudioaudience_timer == 0)
	{
		S_StartSound(NULL, sfx_mbv91);
		livestudioaudience_timer = 90;
	}
	else
	{
		livestudioaudience_timer--;
	}
}

static void Command_MotD_f(void)
{
	size_t i, j;
	char *mymotd;

	if ((j = COM_Argc()) < 2)
	{
		CONS_Printf(M_GetText("motd <message>: Set a message that clients see upon join.\n"));
		return;
	}

	if (!(server || (IsPlayerAdmin(consoleplayer))))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	mymotd = Z_Malloc(sizeof(motd), PU_STATIC, NULL);

	strlcpy(mymotd, COM_Argv(1), sizeof motd);
	for (i = 2; i < j; i++)
	{
		strlcat(mymotd, " ", sizeof motd);
		strlcat(mymotd, COM_Argv(i), sizeof motd);
	}

	// Disallow non-printing characters and semicolons.
	for (i = 0; mymotd[i] != '\0'; i++)
		if (!isprint(mymotd[i]) || mymotd[i] == ';')
		{
			Z_Free(mymotd);
			return;
		}

	if ((netgame || multiplayer) && client)
		SendNetXCmd(XD_SETMOTD, mymotd, i); // send the actual size of the motd string, not the full buffer's size
	else
	{
		strcpy(motd, mymotd);
		CONS_Printf(M_GetText("Message of the day set.\n"));
	}

	Z_Free(mymotd);
}

static void Got_MotD_f(const UINT8 **cp, INT32 playernum)
{
	char *mymotd = Z_Malloc(sizeof(motd), PU_STATIC, NULL);
	INT32 i;
	boolean kick = false;

	READSTRINGN(*cp, mymotd, sizeof(motd));

	// Disallow non-printing characters and semicolons.
	for (i = 0; mymotd[i] != '\0'; i++)
		if (!isprint(mymotd[i]) || mymotd[i] == ';')
			kick = true;

	if ((playernum != serverplayer && !IsPlayerAdmin(playernum)) || kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal motd change received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		Z_Free(mymotd);
		return;
	}

	strcpy(motd, mymotd);

	CONS_Printf(M_GetText("Message of the day set.\n"));

	Z_Free(mymotd);
}

static void Command_RunSOC(void)
{
	const char *fn;
	char buf[255];
	size_t length = 0;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("runsoc <socfile.soc> or <lumpname>: run a soc\n"));
		return;
	}
	else
		fn = COM_Argv(1);

	if (netgame && !(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	if (!(netgame || multiplayer))
	{
		if (!P_RunSOC(fn))
			CONS_Printf(M_GetText("Could not find SOC.\n"));
		else
			G_SetGameModified(multiplayer, false);
		return;
	}

	nameonly(strcpy(buf, fn));
	length = strlen(buf)+1;

	SendNetXCmd(XD_RUNSOC, buf, length);
}

static void Got_RunSOCcmd(const UINT8 **cp, INT32 playernum)
{
	char filename[256];
	filestatus_t ncs = FS_NOTCHECKED;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal runsoc command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	READSTRINGN(*cp, filename, 255);

	// Maybe add md5 support?
	if (strstr(filename, ".soc") != NULL)
	{
		ncs = findfile(filename,NULL,true);

		if (ncs != FS_FOUND)
		{
			Command_ExitGame_f();
			if (ncs == FS_NOTFOUND)
			{
				CONS_Printf(M_GetText("The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server.\n"), filename);
				M_StartMessage("Server Connection Failure", va("The server added a file\n(%s)\nthat you do not have.\n",filename), NULL, MM_NOTHING, NULL, "Return to Menu");
			}
			else
			{
				CONS_Printf(M_GetText("Unknown error finding soc file (%s) the server added.\n"), filename);
				M_StartMessage("Server Connection Failure", va("Unknown error trying to load a file\nthat the server added\n(%s).\n",filename), NULL, MM_NOTHING, NULL, "Return to Menu");
			}
			return;
		}
	}

	P_RunSOC(filename);
	G_SetGameModified(true, false);
}

/** Adds a pwad at runtime.
  * Searches for sounds, maps, music, new images.
  */
static void Command_Addfile(void)
{
#ifndef TESTERS
	size_t argc = COM_Argc(); // amount of arguments total
	size_t curarg; // current argument index

	if (argc < 2)
	{
		CONS_Printf(M_GetText("addfile <filename.pk3/wad/lua/soc> [filename2...] [...]: Load add-ons\n"));
		return;
	}

	const char **addedfiles = Z_Calloc(sizeof(const char*) * argc, PU_STATIC, NULL);
	size_t numfilesadded = 0; // the amount of filenames processed

	// start at one to skip command name
	for (curarg = 1; curarg < argc; curarg++)
	{
		const char *fn, *p;
		char buf[256];
		char *buf_p = buf;
		INT32 i;
		size_t ii;
		int musiconly; // W_VerifyNMUSlumps isn't boolean
		boolean fileadded = false;

		fn = COM_Argv(curarg);

		// For the amount of filenames previously processed...
		for (ii = 0; ii < numfilesadded; ii++)
		{
			// If this is one of them, don't try to add it.
			if (!strcmp(fn, addedfiles[ii]))
			{
				fileadded = true;
				break;
			}
		}

		// If we've added this one, skip to the next one.
		if (fileadded)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Already processed %s, skipping\n"), fn);
			continue;
		}

		// Disallow non-printing characters and semicolons.
		for (i = 0; fn[i] != '\0'; i++)
			if (!isprint(fn[i]) || fn[i] == ';')
			{
				Z_Free(addedfiles);
				return;
			}

		musiconly = W_VerifyNMUSlumps(fn, false);

		if (musiconly == -1)
		{
			addedfiles[numfilesadded++] = fn;
			continue;
		}

		if (!musiconly)
		{
			// ... But only so long as they contain nothing more then music and sprites.
			if (netgame && !(server || IsPlayerAdmin(consoleplayer)))
			{
				CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
				continue;
			}
			G_SetGameModified(multiplayer, false);
		}

		// Add file on your client directly if it is trivial, or you aren't in a netgame.
		if (!(netgame || multiplayer) || musiconly)
		{
			P_AddWadFile(fn);
			addedfiles[numfilesadded++] = fn;
			continue;
		}

		p = fn+strlen(fn);
		while(--p >= fn)
			if (*p == '\\' || *p == '/' || *p == ':')
				break;
		++p;

		// check total packet size and no of files currently loaded
		// See W_LoadWadFile in w_wad.c
		if (numwadfiles >= MAX_WADFILES)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Too many files loaded to add %s\n"), fn);
			Z_Free(addedfiles);
			return;
		}

		WRITESTRINGN(buf_p,p,240);

		// calculate and check md5
		{
			UINT8 md5sum[16];
#ifdef NOMD5
			memset(md5sum,0,16);
#else
			FILE *fhandle;
			boolean valid = true;

			if ((fhandle = W_OpenWadFile(&fn, true)) != NULL)
			{
				tic_t t = I_GetTime();
				CONS_Debug(DBG_SETUP, "Making MD5 for %s\n",fn);
				md5_stream(fhandle, md5sum);
				CONS_Debug(DBG_SETUP, "MD5 calc for %s took %f second\n", fn, (float)(I_GetTime() - t)/TICRATE);
				fclose(fhandle);
			}
			else // file not found
				continue;

			for (i = 0; i < numwadfiles; i++)
			{
				if (!memcmp(wadfiles[i]->md5sum, md5sum, 16))
				{
					CONS_Alert(CONS_ERROR, M_GetText("%s is already loaded\n"), fn);
					valid = false;
					break;
				}
			}

			if (valid == false)
			{
				continue;
			}
#endif
			WRITEMEM(buf_p, md5sum, 16);
		}

		addedfiles[numfilesadded++] = fn;

		if (IsPlayerAdmin(consoleplayer) && (!server)) // Request to add file
			SendNetXCmd(XD_REQADDFILE, buf, buf_p - buf);
		else
			SendNetXCmd(XD_ADDFILE, buf, buf_p - buf);
	}

	Z_Free(addedfiles);
#endif/*TESTERS*/
}

static void Got_RequestAddfilecmd(const UINT8 **cp, INT32 playernum)
{
	char filename[241];
	filestatus_t ncs = FS_NOTCHECKED;
	UINT8 md5sum[16];
	boolean kick = false;
	boolean toomany = false;
	INT32 i,j;

	READSTRINGN(*cp, filename, 240);
	READMEM(*cp, md5sum, 16);

	// Only the server processes this message.
	if (client)
		return;

	// Disallow non-printing characters and semicolons.
	for (i = 0; filename[i] != '\0'; i++)
		if (!isprint(filename[i]) || filename[i] == ';')
			kick = true;

	if ((playernum != serverplayer && !IsPlayerAdmin(playernum)) || kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfile command received from %s\n"), player_names[playernum]);
		SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	// See W_LoadWadFile in w_wad.c
	if (numwadfiles >= MAX_WADFILES)
		toomany = true;
	else
		ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND || toomany)
	{
		char message[275];

		if (toomany)
			sprintf(message, M_GetText("Too many files loaded to add %s\n"), filename);
		else if (ncs == FS_NOTFOUND)
			sprintf(message, M_GetText("The server doesn't have %s\n"), filename);
		else if (ncs == FS_MD5SUMBAD)
			sprintf(message, M_GetText("Checksum mismatch on %s\n"), filename);
		else
			sprintf(message, M_GetText("Unknown error finding wad file (%s)\n"), filename);

		CONS_Printf("%s",message);

		for (j = 0; j < MAXPLAYERS; j++)
			if (adminplayers[j])
				COM_BufAddText(va("sayto %d %s", adminplayers[j], message));

		return;
	}

	COM_BufAddText(va("addfile %s\n", filename));
}

static void Got_Addfilecmd(const UINT8 **cp, INT32 playernum)
{
	char filename[241];
	filestatus_t ncs = FS_NOTCHECKED;
	UINT8 md5sum[16];

	READSTRINGN(*cp, filename, 240);
	READMEM(*cp, md5sum, 16);

	if (playernum != serverplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfile command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND || !P_AddWadFile(filename))
	{
		Command_ExitGame_f();
		if (ncs == FS_FOUND)
		{
			CONS_Printf(M_GetText("The server tried to add %s,\nbut you have too many files added.\nRestart the game to clear loaded files\nand play on this server."), filename);
			M_StartMessage("Server Connection Failure", va("The server added a file \n(%s)\nbut you have too many files added.\nRestart the game to clear loaded files.\n",filename), NULL, MM_NOTHING, NULL, "Return to Menu");
		}
		else if (ncs == FS_NOTFOUND)
		{
			CONS_Printf(M_GetText("The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server."), filename);
			M_StartMessage("Server Connection Failure", va("The server added a file \n(%s)\nthat you do not have.\n",filename), NULL, MM_NOTHING, NULL, "Return to Menu");
		}
		else if (ncs == FS_MD5SUMBAD)
		{
			CONS_Printf(M_GetText("Checksum mismatch while loading %s.\nMake sure you have the copy of\nthis file that the server has.\n"), filename);
			M_StartMessage("Server Connection Failure", va("Checksum mismatch while loading \n%s.\nThe server seems to have a\ndifferent version of this file.\n",filename), NULL, MM_NOTHING, NULL, "Return to Menu");
		}
		else
		{
			CONS_Printf(M_GetText("Unknown error finding wad file (%s) the server added.\n"), filename);
			M_StartMessage("Server Connection Failure", va("Unknown error trying to load a file\nthat the server added \n(%s).\n",filename), NULL, MM_NOTHING, NULL, "Return to Menu");
		}
		return;
	}

	G_SetGameModified(true, false);
}

static void Command_ListWADS_f(void)
{
	INT32 i = numwadfiles;
	char *tempname;
	CONS_Printf(M_GetText("There are %d wads loaded:\n"),numwadfiles);
	for (i--; i >= 0; i--)
	{
		nameonly(tempname = va("%s", wadfiles[i]->filename));
		if (!i)
			CONS_Printf("\x82 IWAD\x80: %s\n", tempname);
		else if (i <= mainwads)
			CONS_Printf("\x82 * %.2d\x80: %s\n", i, tempname);
		else if (!wadfiles[i]->important)
			CONS_Printf("\x86 %c %.2d: %s\n", ((i <= mainwads + musicwads) ? '*' : ' '), i, tempname);
		else
			CONS_Printf("   %.2d: %s\n", i, tempname);
	}
}

#define MAXDOOMEDNUM 4095

static void Command_ListDoomednums_f(void)
{
	INT16 i, j, focusstart = 0, focusend = 0;
	INT32 argc = COM_Argc(), argstart = 0;
	INT16 table[MAXDOOMEDNUM];
	boolean nodoubles = false;
	UINT8 doubles[(MAXDOOMEDNUM+8/8)];

	if (argc > 1)
	{
		nodoubles = (strcmp(COM_Argv(1), "-nodoubles") == 0);
		if (nodoubles)
		{
			argc--;
			argstart++;
		}
	}

	switch (argc)
	{
		case 1:
			focusend = MAXDOOMEDNUM;
			break;
		case 3:
			focusend = atoi(COM_Argv(argstart+2));
			if (focusend < 1 || focusend > MAXDOOMEDNUM)
			{
				CONS_Printf("arg 2: doomednum \x82""%d \x85out of range (1-4095)\n", focusend);
				return;
			}
			//FALLTHRU
		case 2:
			focusstart = atoi(COM_Argv(argstart+1));
			if (focusstart < 1 || focusstart > MAXDOOMEDNUM)
			{
				CONS_Printf("arg 1: doomednum \x82""%d \x85out of range (1-4095)\n", focusstart);
				return;
			}
			if (!focusend)
				focusend = focusstart;
			else if (focusend < focusstart) // silently and helpfully swap.
			{
				j = focusstart;
				focusstart = focusend;
				focusend = j;
			}
			break;
		default:
			CONS_Printf("listmapthings: \x86too many arguments!\n");
			return;
	}

	// see P_SpawnNonMobjMapThing
	memset(table, 0, sizeof(table));
	memset(doubles, 0, sizeof(doubles));
	for (i = 1; i <= MAXPLAYERS; i++)
		table[i-1] = MT_PLAYER; // playerstarts
	table[33-1] = table[34-1] = table[35-1] = MT_PLAYER; // battle/team starts
	table[750-1] = table[777-1] = table[778-1] = MT_UNKNOWN; // slopes
	for (i = 600; i <= 609; i++)
		table[i-1] = MT_RING; // placement patterns
	table[1705-1] = table[1713-1] = MT_HOOP; // types of hoop

	CONS_Printf("\x82""Checking for double defines...\n");
	for (i = 1; i < MT_FIRSTFREESLOT+NUMMOBJFREESLOTS; i++)
	{
		j = mobjinfo[i].doomednum;
		if (j < (focusstart ? focusstart : 1) || j > focusend)
			continue;
		if (table[--j])
		{
			doubles[j/8] |= 1<<(j&7);
			CONS_Printf("	doomednum \x82""%d""\x80 is \x85""double-defined\x80 by ", j+1);
			if (i < MT_FIRSTFREESLOT)
			{
				CONS_Printf("\x87""hardcode %s <-- MAJOR ERROR\n", MOBJTYPE_LIST[i]);
				continue;
			}
			CONS_Printf("\x81""freeslot MT_""%s\n", FREE_MOBJS[i-MT_FIRSTFREESLOT]);
			continue;
		}
		table[j] = i;
	}
	CONS_Printf("\x82Printing doomednum usage...\n");
	if (!focusstart)
	{
		i = 35; // skip MT_PLAYER spam
		if (!nodoubles)
			CONS_Printf("	doomednums \x82""1-35""\x80 are used by ""\x87""hardcode MT_PLAYER\n");
	}
	else
		i = focusstart-1;

	for (; i < focusend; i++)
	{
		if (nodoubles && !(doubles[i/8] & 1<<(i&7)))
			continue;
		if (!table[i])
		{
			if (focusstart)
			{
				CONS_Printf("	doomednum \x82""%d""\x80 is \x83""free!", i+1);
				if (i < 99) // above the humble crawla? how dare you
					CONS_Printf(" (Don't freeslot this low...)");
				CONS_Printf("\n");
			}
			continue;
		}
		CONS_Printf("	doomednum \x82""%d""\x80 is used by ", i+1);
		if (table[i] < MT_FIRSTFREESLOT)
		{
			CONS_Printf("\x87""hardcode %s\n", MOBJTYPE_LIST[table[i]]);
			continue;
		}
		CONS_Printf("\x81""freeslot MT_""%s\n", FREE_MOBJS[table[i]-MT_FIRSTFREESLOT]);
	}
}

#undef MAXDOOMEDNUM

static void Command_cxdiag_f(void)
{
	UINT16 i, j, errors = 0;

	CONS_Printf("\x82""Welcome to the Challenge eXception Diagnostic.\n");

	{
		conditionset_t *c;
		condition_t *cn;

		CONS_Printf("\x82""Evaluating ConditionSets...\n");

		for (i = 0; i < MAXCONDITIONSETS; i++)
		{
			c = &conditionSets[i];
			if (!c->numconditions)
				continue;

			UINT32 lastID = 0;
			boolean validSoFar = true;
			boolean requiresPlaying = false;
			boolean lastRequiresPlaying = false;
			boolean lastrequiredplayingvalid = false;
			boolean immediatelyprefix = false;
			INT32 relevantlevelgt = -1;
			UINT8 lastj = j = 0;
			while (true) //for (j = 0; j < c->numconditions; ++j)
			{
				if (j >= c->numconditions)
				{
					if (j == lastj)
						break;
					UINT8 swap = j;
					j = lastj;
					lastj = swap;

					lastrequiredplayingvalid = false;
					validSoFar = true;
				}

				cn = &c->condition[j];

				if (lastID)
				{
					//if (lastID != cn->id && validSoFar)
						//CONS_Printf("\x87""Condition%d good\n", lastID);
					if (lastID != cn->id)
					{
						lastrequiredplayingvalid = false;
						validSoFar = true;
						if (j != lastj)
						{
							UINT8 swap = j;
							j = lastj;
							lastj = swap;
							continue;
						}
						relevantlevelgt = -1;
					}
					else if (!validSoFar)
					{
						j++;
						continue;
					}
				}

				const boolean firstpass = (lastj <= j);

				if (cn->type == UC_DESCRIPTIONOVERRIDE)
				{
					if (firstpass)
						;
					else if (!cn->stringvar)
					{
						CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) - Description override has no description!?\n", i+1, j+1, cn->id);
						errors++;
					}
					else if (cn->stringvar[0] != tolower(cn->stringvar[0]))
					{
						CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) - Description override begins with capital letter, which isn't necessary and can sometimes look weird in generated descriptions\n", i+1, j+1, cn->id);
						errors++;
					}
					lastID = cn->id;
					j++;
					continue;
				}

				if (cn->type == UC_AND || cn->type == UC_THEN || cn->type == UC_COMMA)
				{
					if (firstpass)
						;
					else if (immediatelyprefix || lastID != cn->id)
					{
						CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) - Conjunction immediately follows %s - this just looks plain weird!\n", i+1, j+1, cn->id, immediatelyprefix ? "Prefix type" : "start");
						errors++;
					}
					lastID = cn->id;
					j++;
					continue;
				}

				lastID = cn->id;

				lastRequiresPlaying = requiresPlaying;
				requiresPlaying = (cn->type >= UCRP_REQUIRESPLAYING);

				if (!firstpass && lastrequiredplayingvalid)
				{
					if (lastRequiresPlaying != requiresPlaying)
					{
						CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) combines Playing condition and Statistics condition - will never be achieved\n", i+1, j+1, lastID);
						validSoFar = false;
						errors++;
					}
				}
				lastrequiredplayingvalid = true;

				immediatelyprefix = (cn->type >= UCRP_PREFIX_GRANDPRIX && cn->type <= UCRP_PREFIX_ISMAP);

				if (cn->type == UCRP_PREFIX_ISMAP || cn->type == UCRP_ISMAP)
				{
					if (firstpass && relevantlevelgt != -1)
					{
						CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) has multiple courses specified\n", i+1, j+1, lastID);
						validSoFar = false;
						errors++;
					}
					if (cn->requirement == 0)
						relevantlevelgt = 0;
					else if (cn->requirement > 0 && cn->requirement < basenummapheaders)
						relevantlevelgt = G_GuessGametypeByTOL(mapheaderinfo[cn->requirement]->typeoflevel);
					else
						relevantlevelgt = -1;
				}
				else if (firstpass || relevantlevelgt == -1)
					;
				else if (cn->type >= UCRP_PODIUMCUP && cn->type <= UCRP_PODIUMNOCONTINUES)
				{
					CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) is Podium state when specific course in Cup already requested\n", i+1, j+1, lastID);
					validSoFar = false;
					errors++;
				}
				else if (cn->type == UCRP_FINISHALLPRISONS || cn->type == UCRP_PREFIX_PRISONBREAK)
				{
					if (!(gametypes[relevantlevelgt]->rules & GTR_PRISONS))
					{
						CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) is Prison Break-based, but with %s course\n", i+1, j+1, lastID, gametypes[relevantlevelgt]->name);
						validSoFar = false;
						errors++;
					}
				}
				else if (cn->type == UCRP_SMASHUFO || cn->type == UCRP_PREFIX_SEALEDSTAR)
				{
					if (!(gametypes[relevantlevelgt]->rules & GTR_CATCHER))
					{
						CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) is Sealed Star-based, but with %s course\n", i+1, j+1, lastID, gametypes[relevantlevelgt]->name);
						validSoFar = false;
						errors++;
					}
				}
				else if (cn->type == UCRP_RINGS || cn->type == UCRP_RINGSEXACT || cn->type == UCRP_RINGDEBT)
				{
					if ((gametypes[relevantlevelgt]->rules & GTR_SPHERES))
					{
						CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) is Rings-based, but with %s course\n", i+1, j+1, lastID, gametypes[relevantlevelgt]->name);
						validSoFar = false;
						errors++;
					}
				}
				else if (cn->type == UCRP_GROWCONSECUTIVEBEAMS || cn->type == UCRP_FAULTED || cn->type == UCRP_FINISHPERFECT)
				{
					if (!(gametypes[relevantlevelgt]->rules & GTR_CIRCUIT))
					{
						CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) is circuit-based, but with %s course\n", i+1, j+1, lastID, gametypes[relevantlevelgt]->name);
						validSoFar = false;
						errors++;
					}
				}
				else if (cn->type == UCRP_FINISHTIMELEFT)
				{
					if (!(gametypes[relevantlevelgt]->rules & GTR_TIMELIMIT))
					{
						CONS_Printf("\x87""	ConditionSet %u entry %u (Condition%u) is timelimit-based, but with %s course\n", i+1, j+1, lastID, gametypes[relevantlevelgt]->name);
						validSoFar = false;
						errors++;
					}
				}


				j++;
			}
		}
	}

	{
		unlockable_t *un;

		CONS_Printf("\x82""Evaluating Challenges...\n");

		for (i = 0; i < MAXUNLOCKABLES; i++)
		{
			un = &unlockables[i];
			j = un->conditionset;
			if (!j)
				continue;

			if (!conditionSets[j-1].numconditions)
			{
				CONS_Printf("\x87""	Unlockable %u has ConditionSet %u, which has no Conditions successfully set - will never be unlocked?\n", i+1, j);
				errors++;
			}
		}
	}

	{
		CONS_Printf("\x82""Evaluating Time Medals...\n");

		for (i = 0; i < numemblems; i++)
		{
			if (emblemlocations[i].type != ET_TIME)
				continue;

			INT32 checkLevel = M_EmblemMapNum(&emblemlocations[i]);

			if (checkLevel >= nummapheaders || !mapheaderinfo[checkLevel])
				continue;

			if (emblemlocations[i].tag > 0)
			{
				if (emblemlocations[i].tag > mapheaderinfo[checkLevel]->ghostCount)
				{
					CONS_Printf("\x87""	Time Medal %u (level %s) has tag %d, which is greater than the number of associated Staff Ghosts (%u)\n", i+1, mapheaderinfo[checkLevel]->lumpname, emblemlocations[i].tag, mapheaderinfo[checkLevel]->ghostCount);
					errors++;
				}
			}
			else switch (emblemlocations[i].tag)
			{
				case 0:
				{
					if (emblemlocations[i].var < TICRATE)
					{
						CONS_Printf("\x87""	Time Medal %u (level %s) is set to %d (less than a second??)\n", i+1, mapheaderinfo[checkLevel]->lumpname, emblemlocations[i].var);
						errors++;
					}
					break;
				}

				case AUTOMEDAL_PLATINUM:
				{
					if (mapheaderinfo[checkLevel]->ghostCount == 0)
					{
						CONS_Printf("\x87""	Time Medal %u (level %s) is AUTOMEDAL_PLATINUM, but there are no associated Staff Ghosts\n", i+1, mapheaderinfo[checkLevel]->lumpname);
						errors++;
					}
					break;
				}

				case AUTOMEDAL_GOLD:
				case AUTOMEDAL_SILVER:
				case AUTOMEDAL_BRONZE:
				{
					if (mapheaderinfo[checkLevel]->ghostCount < 2)
					{
						CONS_Printf("\x87""	Time Medal %u (level %s) is an Auto Medal, but there are %u associated Staff Ghosts, which is less than the 2 recommended minimum\n", i+1, mapheaderinfo[checkLevel]->lumpname, mapheaderinfo[checkLevel]->ghostCount);
						errors++;
					}
					break;
				}

				default:
				{
					CONS_Printf("\x87""	Time Medal %u (level %s) has invalid tag (%d)\n", i+1, mapheaderinfo[checkLevel]->lumpname, emblemlocations[i].tag);
					errors++;

					break;
				}
			}
		}
	}

	if (errors)
		CONS_Printf("\x85""%u errors detected.\n", errors);
	else
		CONS_Printf("\x83""No errors detected! Good job\n");
}

void Command_ListUnusedSprites_f(void)
{
	size_t i, j;

	CONS_Printf("\x82Printing sprite non-usage...\n");

	for (i = 0; i < NUMSPRITES; i++)
	{
		if (sprites[i].numframes)
		{
			// We're only showing unused sprites...
			continue;
		}

		if (i < SPR_FIRSTFREESLOT)
		{
			CONS_Printf("	\x87""hardcode SPR_""%.4s\n", sprnames[i]);
			continue;
		}

		if (used_spr[(i-SPR_FIRSTFREESLOT)/8] == 0xFF)
		{
			for (j = 0; j < 8; j++)
			{
				CONS_Printf("	\x81""freeslot SPR_""%.4s\n", sprnames[i+j]);
			}

			i += j;
		}

		if (used_spr[(i-SPR_FIRSTFREESLOT)/8] & (1<<(i%8)))
		{
			CONS_Printf("	\x81""freeslot SPR_""%.4s\n", sprnames[i]);
			continue;
		}

		break;
	}
}

// =========================================================================
//                            MISC. COMMANDS
// =========================================================================

/** Prints program version.
  */
static void Command_Version_f(void)
{
#ifdef DEVELOP
	CONS_Printf("Ring Racers %s %s %s (%s %s)\n", D_GetFancyBranchName(), comprevision, compnote, compdate, comptime);
#else
	CONS_Printf("Ring Racers %s (%s %s %s %s) ", VERSIONSTRING, compdate, comptime, comprevision, D_GetFancyBranchName());
#endif

	// Base library
#if defined( HAVE_SDL)
	CONS_Printf("SDL ");
#endif

	// OS
	// Would be nice to use SDL_GetPlatform for this
#if defined (_WIN32) || defined (_WIN64)
	CONS_Printf("Windows ");
#elif defined(__linux__)
	CONS_Printf("Linux ");
#elif defined(MACOSX)
	CONS_Printf("macOS ");
#elif defined(UNIXCOMMON)
	CONS_Printf("Unix (Common) ");
#else
	CONS_Printf("Other OS ");
#endif

	// Bitness
	if (sizeof(void*) == 4)
		CONS_Printf("32-bit ");
	else if (sizeof(void*) == 8)
		CONS_Printf("64-bit ");
	else // 16-bit? 128-bit?
		CONS_Printf("Bits Unknown ");

	CONS_Printf("%s ", comptype);

	// No ASM?
#ifdef NOASM
	CONS_Printf("\x85" "NOASM " "\x80");
#endif

	// DEVELOP build
#if defined(TESTERS)
	CONS_Printf("\x88" "TESTERS " "\x80");
#elif defined(DEVELOP)
	CONS_Printf("\x87" "DEVELOP " "\x80");
#endif

	if (compuncommitted)
		CONS_Printf("\x85" "! UNCOMMITTED CHANGES ! " "\x80");

	CONS_Printf("\n");
}

#ifdef UPDATE_ALERT
static void Command_ModDetails_f(void)
{
	CONS_Printf(M_GetText("Mod App Name: %s\nMod Version: %d\nCode Base:%d\n"), SRB2APPLICATION, MODVERSION, CODEBASE);
}
#endif

// Returns current gametype being used.
//
static void Command_ShowGametype_f(void)
{
	const char *gametypestr = NULL;

	// get name string for current gametype
	if (gametype >= 0 && gametype < numgametypes)
		gametypestr = gametypes[gametype]->name;

	if (gametypestr)
		CONS_Printf(M_GetText("Current gametype is %s\n"), gametypestr);
	else // string for current gametype was not found above (should never happen)
		CONS_Printf(M_GetText("Unknown gametype set (%d)\n"), gametype);
}

/** Plays the intro.
  */
static void Command_Playintro_f(void)
{
	if (netgame)
		return;

	F_StartIntro();
}

/** Quits the game immediately.
  */
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void)
{
	LUA_HookBool(true, HOOK(GameQuit));
	I_Quit();
}

void ItemFinder_OnChange(void)
{
	if (!cv_itemfinder.value)
		return; // it's fine.

	if (!M_SecretUnlocked(SECRET_ITEMFINDER, true))
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSetValue(&cv_itemfinder, 0);
		return;
	}
	else if (netgame || multiplayer)
	{
		CONS_Printf(M_GetText("This only works in single player.\n"));
		CV_StealthSetValue(&cv_itemfinder, 0);
		return;
	}
}

/** Deals with a pointlimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the pointlimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * We don't check immediately for the pointlimit having been reached,
  * because you would get "caught" when turning it up in the menu.
  * \sa cv_pointlimit, TimeLimit_OnChange
  * \author Graue <graue@oceanbase.org>
  */
void PointLimit_OnChange(void);
void PointLimit_OnChange(void)
{
	if (K_CanChangeRules(false) == false)
	{
		return;
	}

	if (cv_pointlimit.value == -1)
	{
		CONS_Printf(M_GetText("Point limit will be left up to the gametype.\n"));
	}

	if (gamestate == GS_LEVEL && leveltime < starttime)
	{
		switch (cv_pointlimit.value)
		{
			case -1:
				break;

			case 0:
				CONS_Printf(M_GetText("Point limit has been disabled.\n"));
				break;

			default:
				CONS_Printf(M_GetText("Point limit has been set to %d.\n"), cv_pointlimit.value);
		}

		g_pointlimit = K_PointLimitForGametype();
	}
	else
	{
		switch (cv_pointlimit.value)
		{
			case -1:
				break;

			case 0:
				CONS_Printf(M_GetText("Point limit will be disabled next round.\n"));
				break;

			default:
				CONS_Printf(M_GetText("Point limit will be %d next round.\n"), cv_pointlimit.value);
		}
	}
}

void NetTimeout_OnChange(void);
void NetTimeout_OnChange(void)
{
	connectiontimeout = (tic_t)cv_nettimeout.value;
}

void JoinTimeout_OnChange(void);
void JoinTimeout_OnChange(void)
{
	jointimeout = (tic_t)cv_jointimeout.value;
}

void Lagless_OnChange (void);
void Lagless_OnChange (void)
{
	/* don't back out of dishonesty, or go lagless after playing honestly */
	if (cv_lagless.value && gamestate == GS_LEVEL)
		server_lagless = true;
}

UINT32 timelimitintics = 0;
UINT32 extratimeintics = 0;
UINT32 secretextratime = 0;

UINT32 g_pointlimit = 0;

/** Deals with a timelimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the timelimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * \sa cv_timelimit, PointLimit_OnChange
  */
void TimeLimit_OnChange(void);
void TimeLimit_OnChange(void)
{
	if (K_CanChangeRules(false) == false)
	{
		return;
	}

	if (cv_timelimit.value == -1)
	{
		CONS_Printf(M_GetText("Time limit will be left up to the gametype.\n"));
	}

	if (gamestate == GS_LEVEL && leveltime < starttime)
	{
		switch (cv_timelimit.value)
		{
			case -1:
				break;

			case 0:
				CONS_Printf(M_GetText("Time limit has been disabled.\n"));
				break;

			default:
				CONS_Printf(M_GetText("Time limit has been set to %d second%s.\n"), cv_timelimit.value,cv_timelimit.value == 1 ? "" : "s");
		}

		timelimitintics = K_TimeLimitForGametype();
		extratimeintics = secretextratime = 0;

#ifdef HAVE_DISCORDRPC
		DRPC_UpdatePresence();
#endif
	}
	else
	{
		switch (cv_timelimit.value)
		{
			case -1:
				break;

			case 0:
				CONS_Printf(M_GetText("Time limit will be disabled next round.\n"));
				break;

			default:
				CONS_Printf(M_GetText("Time limit will be %d second%s next round.\n"), cv_timelimit.value,cv_timelimit.value == 1 ? "" : "s");
		}
	}
}

/** Adjusts certain settings to match a changed gametype.
  *
  * \param lastgametype The gametype we were playing before now.
  * \sa D_MapChange
  * \author Graue <graue@oceanbase.org>
  * \todo Get rid of the hardcoded stuff, ugly stuff, etc.
  */
void D_GameTypeChanged(INT32 lastgametype)
{
	if (netgame)
	{
		const char *oldgt = NULL, *newgt = NULL;

		if (lastgametype >= 0 && lastgametype < numgametypes)
			oldgt = gametypes[lastgametype]->name;
		if (gametype >= 0 && gametype < numgametypes)
			newgt = gametypes[gametype]->name;

		if (oldgt && newgt && (lastgametype != gametype))
			CONS_Printf(M_GetText("Gametype was changed from %s to %s\n"), oldgt, newgt);
	}

	// don't retain teams in other modes or between changes from ctf to team match.
	// also, stop any and all forms of team scrambling that might otherwise take place.
	if (G_GametypeHasTeams())
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				players[i].ctfteam = 0;

		if (server || (IsPlayerAdmin(consoleplayer)))
		{
			CV_StealthSetValue(&cv_teamscramble, 0);
			teamscramble = 0;
		}
	}
}

void Gravity_OnChange(void);
void Gravity_OnChange(void)
{
	if (netgame)
	{
		// TODO: multiplayer support
		return;
	}

	gravity = cv_gravity.value;
}

void SoundTest_OnChange(void);
void SoundTest_OnChange(void)
{
	INT32 sfxfreeint = (INT32)sfxfree;
	if (cv_soundtest.value < 0)
	{
		CV_SetValue(&cv_soundtest, sfxfreeint-1);
		return;
	}

	if (cv_soundtest.value >= sfxfreeint)
	{
		CV_SetValue(&cv_soundtest, 0);
		return;
	}

	S_StopSounds();
	S_StartSound(NULL, cv_soundtest.value);
}

void AutoBalance_OnChange(void);
void AutoBalance_OnChange(void)
{
	autobalance = (INT16)cv_autobalance.value;
}

void TeamScramble_OnChange(void);
void TeamScramble_OnChange(void)
{
	INT16 i = 0, j = 0, playercount = 0;
	boolean repick = true;
	INT32 blue = 0, red = 0;
	INT32 maxcomposition = 0;
	INT16 newteam = 0;
	INT32 retries = 0;
	boolean success = false;

	// Don't trigger outside level or intermission!
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_VOTING))
		return;

	if (!cv_teamscramble.value)
		teamscramble = 0;

	if (!G_GametypeHasTeams() && (server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		CV_StealthSetValue(&cv_teamscramble, 0);
		return;
	}

	// If a team scramble is already in progress, do not allow another one to be started!
	if (teamscramble)
		return;

retryscramble:

	// Clear related global variables. These will get used again in p_tick.c/y_inter.c as the teams are scrambled.
	memset(&scrambleplayers, 0, sizeof(scrambleplayers));
	memset(&scrambleteams, 0, sizeof(scrambleplayers));
	scrambletotal = scramblecount = 0;
	blue = red = maxcomposition = newteam = playercount = 0;
	repick = true;

	// Put each player's node in the array.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
		{
			scrambleplayers[playercount] = i;
			playercount++;
		}
	}

	if (playercount < 2)
	{
		CV_StealthSetValue(&cv_teamscramble, 0);
		return; // Don't scramble one or zero players.
	}

	// Randomly place players on teams.
	if (cv_teamscramble.value == 1)
	{
		maxcomposition = playercount / 2;

		// Now randomly assign players to teams.
		// If the teams get out of hand, assign the rest to the other team.
		for (i = 0; i < playercount; i++)
		{
			if (repick)
				newteam = (INT16)((M_RandomByte() % 2) + 1);

			// One team has the most players they can get, assign the rest to the other team.
			if (red == maxcomposition || blue == maxcomposition)
			{
				if (red == maxcomposition)
					newteam = 2;
				else //if (blue == maxcomposition)
					newteam = 1;

				repick = false;
			}

			scrambleteams[i] = newteam;

			if (newteam == 1)
				red++;
			else
				blue++;
		}
	}
	else if (cv_teamscramble.value == 2) // Same as before, except split teams based on current score.
	{
		// Now, sort the array based on points scored.
		for (i = 1; i < playercount; i++)
		{
			for (j = i; j < playercount; j++)
			{
				INT16 tempplayer = 0;

				if ((players[scrambleplayers[i-1]].score > players[scrambleplayers[j]].score))
				{
					tempplayer = scrambleplayers[i-1];
					scrambleplayers[i-1] = scrambleplayers[j];
					scrambleplayers[j] = tempplayer;
				}
			}
		}

		// Now assign players to teams based on score. Scramble in pairs.
		// If there is an odd number, one team will end up with the unlucky slob who has no points. =(
		for (i = 0; i < playercount; i++)
		{
			if (repick)
			{
				newteam = (INT16)((M_RandomByte() % 2) + 1);
				repick = false;
			}
			// (i != 2) means it does ABBABABA, instead of ABABABAB.
			// Team A gets 1st, 4th, 6th, 8th.
			// Team B gets 2nd, 3rd, 5th, 7th.
			// So 1st on one team, 2nd/3rd on the other, then alternates afterwards.
			// Sounds strange on paper, but works really well in practice!
			else if (i != 2)
			{
				// We will only randomly pick the team for the first guy.
				// Otherwise, just alternate back and forth, distributing players.
				newteam = 3 - newteam;
			}

			scrambleteams[i] = newteam;
		}
	}

	// Check to see if our random selection actually
	// changed anybody. If not, we run through and try again.
	for (i = 0; i < playercount; i++)
	{
		if (players[scrambleplayers[i]].ctfteam != scrambleteams[i])
			success = true;
	}

	if (!success && retries < 5)
	{
		retries++;
		goto retryscramble; //try again
	}

	// Display a witty message, but only during scrambles specifically triggered by an admin.
	if (cv_teamscramble.value)
	{
		scrambletotal = playercount;
		teamscramble = (INT16)cv_teamscramble.value;

		if (!(gamestate == GS_INTERMISSION && cv_scrambleonchange.value))
			CONS_Printf(M_GetText("Teams will be scrambled next round.\n"));
	}
}

static void Command_Showmap_f(void)
{
	if (gamestate == GS_LEVEL)
	{
		if (mapheaderinfo[gamemap-1]->zonttl[0] && !(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
		{
			if (mapheaderinfo[gamemap-1]->actnum > 0)
				CONS_Printf("%s (%d): %s %s %d\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl, mapheaderinfo[gamemap-1]->zonttl, mapheaderinfo[gamemap-1]->actnum);
			else
				CONS_Printf("%s (%d): %s %s\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl, mapheaderinfo[gamemap-1]->zonttl);
		}
		else
		{
			if (mapheaderinfo[gamemap-1]->actnum > 0)
				CONS_Printf("%s (%d): %s %d\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl, mapheaderinfo[gamemap-1]->actnum);
			else
				CONS_Printf("%s (%d): %s\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl);
		}
	}
	else
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
}

static void Command_Mapmd5_f(void)
{
	if (gamestate == GS_LEVEL)
	{
		INT32 i;
		char md5tmp[33];
		for (i = 0; i < 16; ++i)
			sprintf(&md5tmp[i*2], "%02x", mapmd5[i]);
		CONS_Printf("%s: %s\n", G_BuildMapName(gamemap), md5tmp);
	}
	else
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
}

boolean G_GamestateUsesExitLevel(void)
{
	if (demo.playback)
		return false;

	switch (gamestate)
	{
		case GS_LEVEL:
		case GS_CREDITS:
			return true;

		default:
			return false;
	}
}

static void Command_ExitLevel_f(void)
{
	if (!(server || (IsPlayerAdmin(consoleplayer))))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
	}
	else if (K_CanChangeRules(false) == false && CV_CheatsEnabled() == false)
	{
		CONS_Printf(M_GetText("This cannot be used without cheats enabled.\n"));
	}
	else if (G_GamestateUsesExitLevel() == false)
	{
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
	}
	else
	{
		SendNetXCmd(XD_EXITLEVEL, NULL, 0);
	}
}

static void Got_ExitLevelcmd(const UINT8 **cp, INT32 playernum)
{
	(void)cp;

	// Ignore duplicate XD_EXITLEVEL commands.
	if (gameaction == ga_completed)
		return;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal exitlevel command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (G_GamestateUsesExitLevel() == false)
		return;

	G_FinishExitLevel();
}

static void Got_SetupVotecmd(const UINT8 **cp, INT32 playernum)
{
	INT16 newGametype = 0;
	boolean baseEncore = false;
	boolean optionalEncore = false;
	INT16 tempVoteLevels[VOTE_NUM_LEVELS][2];
	INT32 i;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum)) // admin shouldn't be able to set up vote...
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal vote setup received from %s\n"), player_names[playernum]);
		if (server)
		{
			SendKick(playernum, KICK_MSG_CON_FAIL);
		}
		return;
	}

	newGametype = READINT16(*cp);
	baseEncore = (boolean)READUINT8(*cp);
	optionalEncore = (boolean)READUINT8(*cp);

	if (!(gametyperules & GTR_ENCORE))
	{
		// Strip illegal Encore flags.
		baseEncore = optionalEncore = false;
	}

	if (newGametype < 0 || newGametype >= numgametypes)
	{
		if (server)
		{
			I_Error("Got_SetupVotecmd: Gametype %d out of range (numgametypes = %d)", newGametype, numgametypes);
		}

		CONS_Alert(CONS_WARNING, M_GetText("Vote setup with bad gametype %d received from %s\n"), newGametype, player_names[playernum]);
		return;
	}

	for (i = 0; i < VOTE_NUM_LEVELS; i++)
	{
		tempVoteLevels[i][0] = (UINT16)READUINT16(*cp);
		tempVoteLevels[i][1] = (baseEncore == true) ? VOTE_MOD_ENCORE : 0;

		if (tempVoteLevels[i][0] < nummapheaders && mapheaderinfo[tempVoteLevels[i][0]])
		{
			continue;
		}

		if (server)
		{
			I_Error("Got_SetupVotecmd: Internal map ID %d not found (nummapheaders = %d)", tempVoteLevels[i][0], nummapheaders);
		}

		CONS_Alert(CONS_WARNING, M_GetText("Vote setup with bad map ID %d received from %s\n"), tempVoteLevels[i][0], player_names[playernum]);
		return;
	}

	{
		INT16 oldGametype = gametype;
		G_SetGametype(newGametype);
		D_GameTypeChanged(oldGametype);
	}

	if (optionalEncore == true)
	{
		tempVoteLevels[VOTE_NUM_LEVELS - 1][1] ^= VOTE_MOD_ENCORE;
	}

	memcpy(g_voteLevels, tempVoteLevels, sizeof(g_voteLevels));

	G_SetGamestate(GS_VOTING);
	Y_StartVote();
}

static void Got_ModifyVotecmd(const UINT8 **cp, INT32 playernum)
{
	UINT8 targetID = READUINT8(*cp);
	SINT8 vote = READSINT8(*cp);

	if (targetID == UINT8_MAX)
	{
		if (playernum != serverplayer) // server-only special vote
		{
			goto fail;
		}

		targetID = VOTE_SPECIAL;
	}
	else if (playeringame[targetID] == true && players[targetID].bot == true)
	{
		if (targetID >= MAXPLAYERS
			|| playernum != serverplayer)
		{
			goto fail;
		}
	}
	else
	{
		if (targetID >= MAXPLAYERS
			|| playernode[targetID] != playernode[playernum])
		{
			goto fail;
		}
	}

	Y_SetPlayersVote(targetID, vote);
	return;

fail:
	CONS_Alert(CONS_WARNING,
		M_GetText ("Illegal modify vote command received from %s\n"),
		player_names[playernum]
	);

	if (server)
	{
		SendKick(playernum, KICK_MSG_CON_FAIL);
	}
}

static void Got_PickVotecmd(const UINT8 **cp, INT32 playernum)
{
	SINT8 pick = READSINT8(*cp);
	SINT8 level = READSINT8(*cp);

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal vote setup received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	Y_SetupVoteFinish(pick, level);
}

static void Got_ScheduleTaskcmd(const UINT8 **cp, INT32 playernum)
{
	char command[MAXTEXTCMD];
	INT16 seconds;

	seconds = READINT16(*cp);
	READSTRING(*cp, command);

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING,
				M_GetText ("Illegal schedule task received from %s\n"),
				player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	Schedule_Add(seconds, seconds, (const char *)command);

	if (server || consoleplayer == playernum)
	{
		CONS_Printf(
			"OK! Running \"" "\x82" "%s" "\x80" "\" every " "\x82" "%d" "\x80" " seconds.\n",
			command,
			seconds
		);
	}
}

static void Got_ScheduleClearcmd(const UINT8 **cp, INT32 playernum)
{
	(void)cp;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING,
				M_GetText ("Illegal schedule clear received from %s\n"),
				player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	Schedule_Clear();

	if (server || consoleplayer == playernum)
	{
		CONS_Printf("All scheduled tasks have been cleared.\n");
	}
}

static void Got_Automatecmd(const UINT8 **cp, INT32 playernum)
{
	UINT8 eventID;
	char command[MAXTEXTCMD];

	eventID = READUINT8(*cp);
	READSTRING(*cp, command);

	if (
		(playernum != serverplayer && !IsPlayerAdmin(playernum))
		|| (eventID >= AEV__MAX)
	)
	{
		CONS_Alert(CONS_WARNING,
				M_GetText ("Illegal automate received from %s\n"),
				player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	Automate_Set(eventID, command);

	if (server || consoleplayer == playernum)
	{
		if (command[0] == '\0')
		{
			CONS_Printf(
				"Removed the %s automate command.\n",
				automate_names[eventID]
			);
		}
		else
		{
			CONS_Printf(
				"Set the %s automate command to \"" "\x82" "%s" "\x80" "\".\n",
				automate_names[eventID],
				command
			);
		}
	}
}

static void Got_Cheat(const UINT8 **cp, INT32 playernum)
{
	UINT8 targetPlayer = READUINT8(*cp);
	cheat_t cheat = READUINT8(*cp);

	player_t *player;

	if (cheat >= NUMBER_OF_CHEATS || !CV_CheatsEnabled() || targetPlayer >= MAXPLAYERS ||
			playernode[targetPlayer] != playernode[playernum])
	{
		CONS_Alert(CONS_WARNING,
				M_GetText ("Illegal cheat command received from %s\n"),
				player_names[playernum]);
		return;
	}

	player = &players[targetPlayer];

	switch (cheat)
	{
		case CHEAT_NOCLIP: {
			const char *status = "on";

			if (!P_MobjWasRemoved(player->mo))
			{
				UINT32 noclipFlags = MF_NOCLIP;

				if (player->spectator)
				{
					noclipFlags |= MF_NOCLIPHEIGHT;
				}

				if (player->mo->flags & MF_NOCLIP)
				{
					player->mo->flags &= ~(noclipFlags);
					status = "off";
				}
				else
				{
					player->mo->flags |= noclipFlags;
				}
			}

			CV_CheaterWarning(targetPlayer, va("noclip %s", status));
			break;
		}

		case CHEAT_GOD: {
			const char *status = (player->pflags & PF_GODMODE) ? "off" : "on";

			player->pflags ^= PF_GODMODE;

			CV_CheaterWarning(targetPlayer, va("GOD MODE %s", status));
			break;
		}

		case CHEAT_SAVECHECKPOINT: {
			fixed_t x = READFIXED(*cp);
			fixed_t y = READFIXED(*cp);
			fixed_t z = READFIXED(*cp);

			player->respawn.pointx = x;
			player->respawn.pointy = y;
			player->respawn.pointz = z;
			player->respawn.manual = true;

			CV_CheaterWarning(targetPlayer, va("temporary checkpoint created at %d, %d, %d",
						x / FRACUNIT, y / FRACUNIT, z / FRACUNIT));
			break;
		}

		case CHEAT_RINGS: {
			SINT8 rings = READSINT8(*cp);

			// P_GivePlayerRings does value clamping
			player->rings = 0;
			P_GivePlayerRings(player, rings);

			CV_CheaterWarning(targetPlayer, va("rings = %d", rings));
			break;
		}

		case CHEAT_LIVES: {
			SINT8 lives = READSINT8(*cp);

			// P_GivePlayerLives does value clamping
			player->lives = 0;
			P_GivePlayerLives(player, lives);

			CV_CheaterWarning(targetPlayer, va("lives = %d", lives));
			break;
		}

		case CHEAT_SCALE: {
			const fixed_t smin = FRACUNIT/100;
			const fixed_t smax = 100*FRACUNIT;

			fixed_t s = READFIXED(*cp);
			float f;

			s = min(max(smin, s), smax);
			f = FIXED_TO_FLOAT(s);

			if (!P_MobjWasRemoved(player->mo))
			{
				player->mo->destscale = s;
			}

			CV_CheaterWarning(targetPlayer, va("scale = %d%s", (int)f, M_Ftrim(FIXED_TO_FLOAT(s))));
			break;
		}

		case CHEAT_FLIP: {
			if (!P_MobjWasRemoved(player->mo))
			{
				player->mo->flags2 ^= MF2_OBJECTFLIP;
			}

			CV_CheaterWarning(targetPlayer, "invert gravity");
			break;
		}

		case CHEAT_HURT: {
			INT32 damage = READINT32(*cp);

			if (!P_MobjWasRemoved(player->mo))
			{
				P_DamageMobj(player->mo, NULL, NULL, damage, DMG_NORMAL);
			}

			CV_CheaterWarning(targetPlayer, va("%d damage to me", damage));
			break;
		}

		case CHEAT_RELATIVE_TELEPORT:
		case CHEAT_TELEPORT: {
			fixed_t x = READFIXED(*cp);
			fixed_t y = READFIXED(*cp);
			fixed_t z = READFIXED(*cp);

			float f[3] = {
				FIXED_TO_FLOAT(x),
				FIXED_TO_FLOAT(y),
				FIXED_TO_FLOAT(z),
			};
			char t[3][9];

			if (!P_MobjWasRemoved(player->mo))
			{
				P_MapStart();
				if (cheat == CHEAT_RELATIVE_TELEPORT)
				{
					P_SetOrigin(player->mo,
							player->mo->x + x,
							player->mo->y + y,
							player->mo->z + z);
				}
				else
				{
					P_SetOrigin(player->mo, x, y, z);
				}
				P_MapEnd();

				player->pflags |= PF_TRUSTWAYPOINTS;
				player->bigwaypointgap = 0;

				S_StartSound(player->mo, sfx_mixup);
			}

			strlcpy(t[0], M_Ftrim(f[0]), sizeof t[0]);
			strlcpy(t[1], M_Ftrim(f[1]), sizeof t[1]);
			strlcpy(t[2], M_Ftrim(f[2]), sizeof t[2]);

			CV_CheaterWarning(targetPlayer, va("%s %d%s, %d%s, %d%s",
						cheat == CHEAT_RELATIVE_TELEPORT
						? "relative teleport by"
						: "teleport to",
						(int)f[0], t[0], (int)f[1], t[1], (int)f[2], t[2]));
			break;
		}

		case CHEAT_DEVMODE: {
			UINT32 flags = READUINT32(*cp);
			cht_debug = flags;
			CV_CheaterWarning(targetPlayer, va("devmode %x", flags));
			break;
		}

		case CHEAT_GIVEITEM: {
			SINT8 item = READSINT8(*cp);
			UINT8 amt = READUINT8(*cp);

			item = max(item, KITEM_SAD);
			item = min(item, NUMKARTITEMS - 1);

			K_StripItems(player);

			// Cancel roulette if rolling
			K_StopRoulette(&player->itemRoulette);

			player->itemtype = item;
			player->itemamount = amt;

			if (amt == 0)
			{
				CV_CheaterWarning(playernum, "delete my items");
			}
			else
			{
				// FIXME: we should have actual KITEM_ name array
				const char *itemname = cv_kartdebugitem.PossibleValue[1 + item].strvalue;

				CV_CheaterWarning(playernum, va("give item %s x%d", itemname, amt));
			}
			break;
		}

		case CHEAT_GIVEPOWERUP: {
			UINT8 powerup = READUINT8(*cp);
			UINT16 time = READUINT16(*cp);

			powerup = min(powerup, LASTPOWERUP);

			// FIXME: we should have actual KITEM_ name array
			const char *powerupname = cv_kartdebugitem.PossibleValue[
				1 + NUMKARTITEMS + (powerup - FIRSTPOWERUP)].strvalue;

			K_GivePowerUp(player, powerup, time);

			CV_CheaterWarning(playernum, va("give powerup %s %d tics", powerupname, time));
			break;
		}

		case CHEAT_SCORE: {
			UINT32 score = READUINT32(*cp);

			player->roundscore = score;

			CV_CheaterWarning(targetPlayer, va("score = %u", score));
			break;
		}

		case CHEAT_ANGLE: {
			angle_t angle = READANGLE(*cp);
			float anglef = FIXED_TO_FLOAT(AngleFixed(angle));

			P_SetPlayerAngle(player, angle);

			CV_CheaterWarning(targetPlayer, va("angle = %d%s", (int)anglef, M_Ftrim(anglef)));
			break;
		}

		case CHEAT_RESPAWNAT: {
			INT32 id = READINT32(*cp);
			waypoint_t *finish = K_GetFinishLineWaypoint();
			waypoint_t *waypoint = K_GetWaypointFromID(id);
			path_t path = {0};
			boolean retryBackwards = false;
			const UINT32 baseDist = FixedMul(RESPAWN_DIST, mapobjectscale);

			CV_CheaterWarning(targetPlayer, va("respawnat %d", id));

			if (waypoint == NULL)
			{
				CONS_Alert(CONS_WARNING, "respawnat: no waypoint with that ID\n");
				break;
			}

			// First, just try to go forward normally
			if (K_PathfindToWaypoint(player->respawn.wp, waypoint, &path, false, false))
			{
				// If the path forward is too short, extend it by moving the origin behind
				if (path.totaldist < baseDist)
				{
					retryBackwards = true;
				}
				else
				{
					size_t i;

					for (i = 0; i < path.numnodes; ++i)
					{
						// If we had to cross the finish line, this waypoint is behind us
						if (path.array[i].nodedata == finish)
						{
							retryBackwards = true;
							break;
						}
					}
				}

				Z_Free(path.array);
			}
			else
			{
				retryBackwards = true;
			}

			if (retryBackwards)
			{
				memset(&path, 0, sizeof path);
				if (!K_PathfindThruCircuit(waypoint, baseDist, &path, false, true))
				{
					CONS_Alert(CONS_WARNING, "respawnat: no path to waypoint\n");
					break;
				}

				// Update origin since lightsnake must go forwards
				player->respawn.wp = path.array[path.numnodes - 1].nodedata;

				Z_Free(path.array);
			}

			player->respawn.state = RESPAWNST_NONE;
			K_DoIngameRespawn(player);
			player->respawn.distanceleft = retryBackwards ? baseDist : path.totaldist;
			break;
		}

		case CHEAT_SPHERES: {
			INT16 spheres = READINT16(*cp);

			// P_GivePlayerSpheres does value clamping
			player->spheres = 0;
			P_GivePlayerSpheres(player, spheres);

			CV_CheaterWarning(targetPlayer, va("spheres = %d", spheres));
			break;
		}

		case CHEAT_FREEZE: {
			const char *status = P_FreezeCheat() ? "off" : "on";
			P_SetFreezeCheat( !P_FreezeCheat() );
			CV_CheaterWarning(targetPlayer, va("freeze %s", status));
			break;
		}

		case NUMBER_OF_CHEATS:
			break;
	}
}

static const char *displayplayer_compose_col(int playernum)
{
	return va("\x84(%d) \x83%s\x80", playernum, player_names[playernum]);
}

static int displayplayer_col_len(const char *text)
{
	int n = strlen(text);
	int k = n;
	int i;
	for (i = 0; i < n; ++i)
	{
		if (!isprint(text[i]))
			k--;
	}
	return k;
}

static void displayplayer_calc_col(int *col, const char *text)
{
	if (text && text[0] != ' ')
	{
		int n = displayplayer_col_len(text);
		if (*col < n)
			*col = n;
	}
}

static void displayplayer_print_col(int *col, const char *text)
{
	if (text)
	{
		if (*col)
		{
			int n = *col - displayplayer_col_len(text);
			CONS_Printf("%s%*s ", text, n, "");
		}
	}
	else
		CONS_Printf("\n");
}

static void displayplayer_iter_table(int table[5], void(*col_cb)(int*,const char*))
{
	int i;

	col_cb(&table[0], "");
	for (i = 0; i < 4; ++i)
		col_cb(&table[1 + i], va(" %d", i));
	col_cb(NULL, NULL);

	col_cb(&table[0], "g_local");
	for (i = 0; i <= splitscreen; ++i)
		col_cb(&table[1 + i], displayplayer_compose_col(g_localplayers[i]));
	col_cb(NULL, NULL);

	col_cb(&table[0], "display");
	for (i = 0; i <= r_splitscreen; ++i)
		col_cb(&table[1 + i], displayplayer_compose_col(displayplayers[i]));
	col_cb(NULL, NULL);

	col_cb(&table[0], "local party");
	for (i = 0; i < G_LocalSplitscreenPartySize(consoleplayer); ++i)
		col_cb(&table[1 + i], displayplayer_compose_col(G_LocalSplitscreenPartyMember(consoleplayer, i)));
	col_cb(NULL, NULL);

	col_cb(&table[0], "final party");
	for (i = 0; i < G_PartySize(consoleplayer); ++i)
		col_cb(&table[1 + i], displayplayer_compose_col(G_PartyMember(consoleplayer, i)));
	col_cb(NULL, NULL);
}

/** Prints the number of displayplayers[0].
  */
static void Command_Displayplayer_f(void)
{
	int table[5] = {0};
	displayplayer_iter_table(table, displayplayer_calc_col);
	displayplayer_iter_table(table, displayplayer_print_col);
}

/** Quits a game and returns to the title screen.
  *
  */
void Command_ExitGame_f(void)
{
	INT32 i;

	LUA_HookBool(false, HOOK(GameQuit));

	D_QuitNetGame();
	CL_Reset();
	CV_ClearChangedFlags();

	for (i = 0; i < MAXPLAYERS; i++)
		CL_ClearPlayer(i);

	splitscreen = 0;
	SplitScreen_OnChange();

	cht_debug = 0;
	memset(&luabanks, 0, sizeof(luabanks));

	if (dirmenu)
		closefilemenu(true);

	if (!modeattacking)
	{
		// YES, this is where demo.attract gets cleared!
		if (demo.attract == DEMO_ATTRACT_CREDITS)
		{
			F_DeferContinueCredits(); // <-- clears demo.attract
		}
		else if (restoreMenu == NULL) // this is true for attract demos too!
		{
			D_StartTitle(); // <-- clears demo.attract
		}
		else
		{
			D_ClearState();
			M_StartControlPanel();
			demo.attract = DEMO_ATTRACT_OFF; // shouldn't ever happen, but let's keep the code symmetrical
		}
	}
}

void Command_Retry_f(void)
{
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
	{
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
	}
	else if (grandprixinfo.gp == false)
	{
		CONS_Printf(M_GetText("This only works in singleplayer games.\n"));
	}
	else if (grandprixinfo.eventmode == GPEVENT_BONUS)
	{
		CONS_Printf(M_GetText("You can't retry right now!\n"));
	}
	else
	{
		M_ClearMenus(true);
		G_SetRetryFlag();
	}
}

/** Reports to the console whether or not the game has been modified.
  *
  * \todo Make it obvious, so a console command won't be necessary.
  * \sa modifiedgame
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Isgamemodified_f(void)
{
	if (savemoddata)
		CONS_Printf("The game has been modified with an addon using its own save data.\n");
	else if (modifiedgame)
		CONS_Printf("The game has been modified, but is still using Ring Racers save data.\n");
	else
		CONS_Printf("The game has not been modified.\n");
}

#ifdef _DEBUG
static void Command_Togglemodified_f(void)
{
	modifiedgame = !modifiedgame;
}

static void Command_Archivetest_f(void)
{
	savebuffer_t save = {0};
	UINT32 i, wrote;
	thinker_t *th;
	if (gamestate != GS_LEVEL)
	{
		CONS_Printf("This command only works in-game, you dummy.\n");
		return;
	}

	// assign mobjnum
	i = 1;
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		if (th->function.acp1 != (actionf_p1)P_RemoveThinkerDelayed)
			((mobj_t *)th)->mobjnum = i++;

	// allocate buffer
	if (P_SaveBufferAlloc(&save, 1024) == false)
	{
		CONS_Printf("Unable to allocate buffer.\n");
		return;
	}

	// test archive
	CONS_Printf("LUA_Archive...\n");
	LUA_Archive(&save, true);
	WRITEUINT8(save.p, 0x7F);
	wrote = (UINT32)(save.p - save.buffer);

	// clear Lua state, so we can really see what happens!
	CONS_Printf("Clearing state!\n");
	LUA_ClearExtVars();

	// test unarchive
	save.p = save.buffer;
	CONS_Printf("LUA_UnArchive...\n");
	LUA_UnArchive(&save, true);
	i = READUINT8(save.p);
	if (i != 0x7F || wrote != (UINT32)(save.p - save.buffer))
	{
		CONS_Printf("Savegame corrupted. (write %u, read %u)\n", wrote, (UINT32)(save.p - save.buffer));
	}

	// free buffer
	P_SaveBufferFree(&save);
	CONS_Printf("Done. No crash.\n");
}
#endif

/** Give yourself an, optional quantity or one of, an item.
*/
static void Command_KartGiveItem_f(void)
{
	UINT8 localplayer = g_localplayers[GetCommandViewNumber()];

	int           ac;
	const char *name;
	INT32       item;

	const char * str;

	int i;

	if (CV_CheatsEnabled())
	{
		ac = COM_Argc();
		if (ac < 2)
		{
			CONS_Printf(
"give <item> [amount]: Give yourself an item\n"
			);
		}
		else
		{
			item = NUMKARTITEMS;

			name = COM_Argv(1);

			if (isdigit(*name) || *name == '-')
			{
				item = atoi(name);
			}
			else
			{
				/* first check exact match */
				if (!CV_CompleteValue(&cv_kartdebugitem, &name, &item))
				{
					CONS_Printf("\x83" "Autocomplete:\n");

					/* then do very loose partial matching */
					for (i = 0; ( str = kartdebugitem_cons_t[i].strvalue ); ++i)
					{
						if (strcasestr(str, name) != NULL)
						{
							CONS_Printf("\x83\t%s\n", str);
							item = kartdebugitem_cons_t[i].value;
						}
					}
				}
			}

			if (item >= FIRSTPOWERUP)
			{
				INT32 amt;

				if (ac > 2)
					amt = atoi(COM_Argv(2));
				else
					amt = BATTLE_POWERUP_TIME;

				D_Cheat(localplayer, CHEAT_GIVEPOWERUP, item, amt);
			}
			else if (item < NUMKARTITEMS)
			{
				INT32 amt;

				if (ac > 2)
					amt = atoi(COM_Argv(2));
				else
					amt = (item != KITEM_NONE);/* default to one quantity, or zero, if KITEM_NONE */

				D_Cheat(localplayer, CHEAT_GIVEITEM, item, amt);
			}
			else
			{
				CONS_Alert(CONS_WARNING,
						"No item matches '%s'\n",
						name);
			}
		}
	}
	else
	{
		CONS_Printf("This cannot be used without cheats enabled.\n");
	}
}

static void Command_Schedule_Add(void)
{
	UINT8 buf[MAXTEXTCMD];
	UINT8 *buf_p = buf;

	size_t ac;
	INT16 seconds;
	const char *command;

	if (!(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Printf("Only the server or a remote admin can use this.\n");
		return;
	}

	ac = COM_Argc();
	if (ac < 3)
	{
		CONS_Printf("schedule <seconds> <...>: runs the specified commands on a recurring timer\n");
		return;
	}

	seconds = atoi(COM_Argv(1));

	if (seconds <= 0)
	{
		CONS_Printf("Timer must be at least 1 second.\n");
		return;
	}

	command = COM_Argv(2);

	WRITEINT16(buf_p, seconds);
	WRITESTRING(buf_p, command);

	SendNetXCmd(XD_SCHEDULETASK, buf, buf_p - buf);
}

static void Command_Schedule_Clear(void)
{
	if (!(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Printf("Only the server or a remote admin can use this.\n");
		return;
	}

	SendNetXCmd(XD_SCHEDULECLEAR, NULL, 0);
}

static void Command_Schedule_List(void)
{
	size_t i;

	if (!(server || IsPlayerAdmin(consoleplayer)))
	{
		// I set it up in a way that this information could be available
		// to everyone, but HOSTMOD has it server/admin-only too, so eh?
		CONS_Printf("Only the server or a remote admin can use this.\n");
		return;
	}

	if (schedule_len == 0)
	{
		CONS_Printf("No tasks are scheduled.\n");
		return;
	}

	for (i = 0; i < schedule_len; i++)
	{
		scheduleTask_t *task = schedule[i];

		CONS_Printf(
			"In " "\x82" "%d" "\x80" " second%s: " "\x82" "%s" "\x80" "\n",
			task->timer,
			(task->timer > 1) ? "s" : "",
			task->command
		);
	}
}

static void Command_Automate_Set(void)
{
	UINT8 buf[MAXTEXTCMD];
	UINT8 *buf_p = buf;

	size_t ac;

	const char *event;
	size_t eventID;

	const char *command;

	if (!(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Printf("Only the server or a remote admin can use this.\n");
		return;
	}

	ac = COM_Argc();
	if (ac < 3)
	{
		CONS_Printf("automate_set <event> <command>: sets the command to run each time a event triggers\n");
		return;
	}

	event = COM_Argv(1);

	for (eventID = 0; eventID < AEV__MAX; eventID++)
	{
		if (strcasecmp(event, automate_names[eventID]) == 0)
		{
			break;
		}
	}

	if (eventID == AEV__MAX)
	{
		CONS_Printf("Unknown event type \"%s\".\n", event);
		return;
	}

	command = COM_Argv(2);

	WRITEUINT8(buf_p, eventID);
	WRITESTRING(buf_p, command);

	SendNetXCmd(XD_AUTOMATE, buf, buf_p - buf);
}

static void Command_Eval(void)
{
	const char *args = COM_Args();

	if (args)
	{
		const fixed_t n = LUA_EvalMath(args);

		CONS_Printf("%f (%d)\n", FixedToFloat(n), n);
	}
}

static void Command_WriteTextmap(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(
"writetextmap <map> [map2...]: Update a map to the latest UDMF version.\n"
"- Use the full map name, e.g. RR_TestRun.\n"
"- You can give this command UP TO %d map names and it will convert all of them.\n"
"- This command generates TEXTMAP files.\n"
"- The location of the generated TEXTMAPs will appear in the console.\n",
ROUNDQUEUE_MAX
		);
		return;
	}

	if (Playing())
	{
		CONS_Alert(CONS_ERROR, "This command cannot be used in-game. Return to the titlescreen first!\n");
		return;
	}

	if (COM_Argc() - 1 > ROUNDQUEUE_MAX)
	{
		CONS_Alert(CONS_ERROR, "Cannot convert more than %d maps. Try again.\n", ROUNDQUEUE_MAX);
		return;
	}

	// Start up a "minor" grand prix session
	memset(&grandprixinfo, 0, sizeof(struct grandprixinfo));
	memset(&roundqueue, 0, sizeof(struct roundqueue));

	grandprixinfo.gamespeed = KARTSPEED_NORMAL;
	grandprixinfo.masterbots = false;

	grandprixinfo.gp = true;
	grandprixinfo.cup = NULL;
	grandprixinfo.wonround = false;

	grandprixinfo.initalize = true;

	roundqueue.position = 1;
	roundqueue.roundnum = 1;
	roundqueue.writetextmap = true;

	size_t i;

	for (i = 1; i < COM_Argc(); ++i)
	{
		INT32 map = G_MapNumber(COM_Argv(i));

		if (map < 0 || map >= nummapheaders)
		{
			CONS_Alert(CONS_WARNING, "%s: Map doesn't exist. Not doing anything.\n", COM_Argv(i));

			// clear round queue (to be safe)
			memset(&roundqueue, 0, sizeof(struct roundqueue));
			return;
		}

		INT32 gt = G_GuessGametypeByTOL(mapheaderinfo[map]->typeoflevel);

		G_MapIntoRoundQueue(map, gt != -1 ? gt : GT_RACE, false, false);
	}

	D_MapChange(1 + roundqueue.entries[0].mapnum, roundqueue.entries[0].gametype, false, true, 1, false, false);

	CON_ToggleOff();
}

#ifdef DEVELOP
static void Command_FastForward(void)
{
	boolean r_flag = false;
	boolean t_flag = false;

	size_t num_time_args = 0;
	const char *time_arg = NULL;

	for (size_t i = 1; i < COM_Argc(); ++i)
	{
		const char *arg = COM_Argv(i);
		if (arg[0] == '-')
		{
			while (*++arg)
			{
				switch (*arg)
				{
					case 'r':
						r_flag = true;
						break;
					case 't':
						t_flag = true;
						break;
				}
			}
		}
		else
		{
			time_arg = arg;
			num_time_args++;
		}
	}

	if (num_time_args != 1)
	{
		CONS_Printf(
			"fastforward [-r] <[mm:]ss>: fast-forward the map time in seconds\n"
			"fastforward -t [-r] <tics>: fast-forward the map time in tics\n"
			"* If the map time has already passed, do nothing.\n"
			"* With -r, fast-forward relative to the current time instead of to an exact map time.\n"
		);
		return;
	}

	char *p;
	tic_t t = strtol(time_arg, &p, 10);

	if (!t_flag)
	{
		t *= TICRATE;

		if (*p == ':')
		{
			t *= 60;
			t += strtol(&p[1], &p, 10) * TICRATE;
		}
	}

	if (*p)
	{
		CONS_Printf("fastforward: time value is malformed '%s'\n", time_arg);
		return;
	}

	if (!r_flag)
	{
		if (leveltime > t)
		{
			CONS_Printf("fastforward: leveltime has already passed\n");
			return;
		}

		t -= leveltime;
	}

	g_fast_forward = t;
}
#endif

/** Makes a change to ::cv_forceskin take effect immediately.
  *
  * \sa Command_SetForcedSkin_f, cv_forceskin, forcedskin
  * \author Graue <graue@oceanbase.org>
  */
void ForceSkin_OnChange(void);
void ForceSkin_OnChange(void)
{
	// NOT in SP, silly!
	if (!Playing() || !K_CanChangeRules(true))
		return;

	if (cv_forceskin.value < 0)
		CONS_Printf("The server has lifted the forced character restrictions.\n");
	else
	{
		CONS_Printf("The server is restricting all players to \"%s\".\n",cv_forceskin.string);
		ForceAllSkins(cv_forceskin.value);
	}
}

//Allows the player's name to be changed if cv_mute is off.
static void Name_OnChange(const UINT8 p)
{
	if (cv_mute.value && !(server || IsPlayerAdmin(g_localplayers[p])))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername[p], player_names[g_localplayers[p]]);
		return;
	}

	SendNameAndColor(p);
}

void Name1_OnChange(void);
void Name1_OnChange(void)
{
	Name_OnChange(0);
}

void Name2_OnChange(void);
void Name2_OnChange(void)
{
	Name_OnChange(1);
}

void Name3_OnChange(void);
void Name3_OnChange(void)
{
	Name_OnChange(2);
}

void Name4_OnChange(void);
void Name4_OnChange(void)
{
	Name_OnChange(3);
}

// sends the follower change for players
static void FollowerAny_OnChange(UINT8 pnum)
{
	if (!Playing())
		return; // don't send anything there.

	SendNameAndColor(pnum);
	G_SetPlayerGamepadIndicatorToPlayerColor(pnum);
}

// sends the follower change for players
void Follower_OnChange(void);
void Follower_OnChange(void)
{
	FollowerAny_OnChange(0);
}

// About the same as Color_OnChange but for followers.
void Followercolor_OnChange(void);
void Followercolor_OnChange(void)
{
	FollowerAny_OnChange(0);
}

// repeat for the 3 other players

void Follower2_OnChange(void);
void Follower2_OnChange(void)
{
	FollowerAny_OnChange(1);
}

void Followercolor2_OnChange(void);
void Followercolor2_OnChange(void)
{
	FollowerAny_OnChange(1);
}

void Follower3_OnChange(void);
void Follower3_OnChange(void)
{
	FollowerAny_OnChange(2);
}

void Followercolor3_OnChange(void);
void Followercolor3_OnChange(void)
{
	FollowerAny_OnChange(2);
}

void Follower4_OnChange(void);
void Follower4_OnChange(void)
{
	FollowerAny_OnChange(3);
}

void Followercolor4_OnChange(void);
void Followercolor4_OnChange(void)
{
	FollowerAny_OnChange(3);
}

/** Sends a skin change for the console player, unless that player is moving. Also forces them to spectate if the change is done during gameplay
  * \sa cv_skin, Skin2_OnChange, Color_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin_OnChange(const UINT8 p)
{
	if (!Playing() || splitscreen < p)
	{
		// do whatever you want
		return;
	}

	if (!CV_CheatsEnabled() && !(netgame || K_CanChangeRules(false))
		&& (gamestate != GS_WAITINGPLAYERS)) // allows command line -warp x +skin y
	{
		CV_StealthSet(&cv_skin[p], skins[players[g_localplayers[p]].skin].name);
		return;
	}

	if (CanChangeSkin(g_localplayers[p]))
	{
		SendNameAndColor(p);
	}
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		CV_StealthSet(&cv_skin[p], skins[players[g_localplayers[p]].skin].name);
	}
}

void Skin1_OnChange(void);
void Skin1_OnChange(void)
{
	Skin_OnChange(0);
}

/** Sends a skin change for the secondary splitscreen player, unless that
  * player is moving. Forces spectate the player if the change is done during gameplay.
  * \sa cv_skin2, Skin_OnChange, Color2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
void Skin2_OnChange(void);
void Skin2_OnChange(void)
{
	Skin_OnChange(1);
}

void Skin3_OnChange(void);
void Skin3_OnChange(void)
{
	Skin_OnChange(2);
}

void Skin4_OnChange(void);
void Skin4_OnChange(void)
{
	Skin_OnChange(3);
}

/** Sends a color change for the console player, unless that player is moving.
  * \sa cv_playercolor, Color2_OnChange, Skin_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color_OnChange(const UINT8 p)
{
	I_Assert(p < MAXSPLITSCREENPLAYERS);

	UINT16 color = cv_playercolor[p].value;
	boolean colorisgood = (color == SKINCOLOR_NONE || K_ColorUsable(color, false, true) == true);

	if (Playing() && p <= splitscreen)
	{
		if (P_PlayerMoving(g_localplayers[p]) == true)
		{
			colorisgood = false;
		}
		else if (colorisgood == true)
		{
			// Color change menu scrolling fix is no longer necessary
			SendNameAndColor(p);
		}
	}

	if (colorisgood == false)
	{
		CV_StealthSetValue(&cv_playercolor[p], lastgoodcolor[p]);
		return;
	}

	lastgoodcolor[p] = color;
	G_SetPlayerGamepadIndicatorToPlayerColor(p);
}

void Color1_OnChange(void);
void Color1_OnChange(void)
{
	Color_OnChange(0);
}

/** Sends a color change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_playercolor2, Color_OnChange, Skin2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
void Color2_OnChange(void);
void Color2_OnChange(void)
{
	Color_OnChange(1);
}

void Color3_OnChange(void);
void Color3_OnChange(void)
{
	Color_OnChange(2);
}

void Color4_OnChange(void);
void Color4_OnChange(void)
{
	Color_OnChange(3);
}

/** Displays the result of the chat being muted or unmuted.
  * The server or remote admin should already know and be able to talk
  * regardless, so this is only displayed to clients.
  *
  * \sa cv_mute
  * \author Graue <graue@oceanbase.org>
  */
void Mute_OnChange(void);
void Mute_OnChange(void)
{
	/*if (server || (IsPlayerAdmin(consoleplayer)))
		return;*/
	// Kinda dumb IMO, you should be able to see confirmation for having muted the chat as the host or admin.

	if (leveltime <= 1)
		return;	// avoid having this notification put in our console / log when we boot the server.

	if (cv_mute.value)
		HU_AddChatText(M_GetText("\x82*Chat has been muted."), false);
	else
		HU_AddChatText(M_GetText("\x82*Chat is no longer muted."), false);
}

/** Hack to clear all changed flags after game start.
  * A lot of code (written by dummies, obviously) uses COM_BufAddText() to run
  * commands and change consvars, especially on game start. This is problematic
  * because CV_ClearChangedFlags() needs to get called on game start \b after
  * all those commands are run.
  *
  * Here's how it's done: the last thing in COM_BufAddText() is "dummyconsvar
  * 1", so we end up here, where dummyconsvar is reset to 0 and all the changed
  * flags are set to 0.
  *
  * \todo Fix the aforementioned code and make this hack unnecessary.
  * \sa cv_dummyconsvar
  * \author Graue <graue@oceanbase.org>
  */
void DummyConsvar_OnChange(void);
void DummyConsvar_OnChange(void)
{
	extern consvar_t cv_dummyconsvar;

	if (cv_dummyconsvar.value == 1)
	{
		CV_SetValue(&cv_dummyconsvar, 0);
		CV_ClearChangedFlags();
	}
}

static void Command_ShowScores_f(void)
{
	UINT8 i;

	if (!(netgame || multiplayer))
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
			// FIXME: %lu? what's wrong with %u? ~Callum (produces warnings...)
			CONS_Printf(M_GetText("%s's score is %u\n"), player_names[i], players[i].score);
	}
	CONS_Printf(M_GetText("The pointlimit is %d\n"), g_pointlimit);

}

static void Command_ShowTime_f(void)
{
	if (!(netgame || multiplayer))
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	CONS_Printf(M_GetText("The current time is %f.\nThe timelimit is %f\n"), (double)leveltime/TICRATE, (double)timelimitintics/TICRATE);
}

// SRB2Kart: On change messages
void NumLaps_OnChange(void);
void NumLaps_OnChange(void)
{
	if (gamestate == GS_LEVEL)
	{
		numlaps = K_RaceLapCount(gamemap - 1);

		if (cv_numlaps.value == -1)
		{
			CONS_Printf(M_GetText("Number of laps have been set to %d (map default).\n"), numlaps);
		}
		else
		{
			CONS_Printf(M_GetText("Number of laps have been set to %d.\n"), numlaps);
		}
	}
	else if (Playing())
	{
		if (cv_numlaps.value == -1)
		{
			CONS_Printf(M_GetText("Number of laps will be the map default next round.\n"));
		}
		else
		{
			CONS_Printf(M_GetText("Number of laps will be set to %d next round.\n"), cv_numlaps.value);
		}
	}
}

void KartFrantic_OnChange(void);
void KartFrantic_OnChange(void)
{
	if (K_CanChangeRules(false) == false)
	{
		return;
	}

	if (gamestate == GS_LEVEL && leveltime < starttime)
	{
		CONS_Printf(M_GetText("Frantic items has been set to %s.\n"), cv_kartfrantic.value ? M_GetText("on") : M_GetText("off"));
		franticitems = (boolean)cv_kartfrantic.value;
	}
	else
	{
		CONS_Printf(M_GetText("Frantic items will be turned %s next round.\n"), cv_kartfrantic.value ? M_GetText("on") : M_GetText("off"));
	}
}

void KartSpeed_OnChange(void);
void KartSpeed_OnChange(void)
{
	if (K_CanChangeRules(false) == false)
	{
		return;
	}

	if (gamestate == GS_LEVEL && leveltime < starttime && cv_kartspeed.value != KARTSPEED_AUTO)
	{
		CONS_Printf(M_GetText("Game speed has been changed to \"%s\".\n"), cv_kartspeed.string);
		gamespeed = (UINT8)cv_kartspeed.value;
	}
	else
	{
		CONS_Printf(M_GetText("Game speed will be changed to \"%s\" next round.\n"), cv_kartspeed.string);
	}
}

void KartEncore_OnChange(void);
void KartEncore_OnChange(void)
{
	if (K_CanChangeRules(false) == false)
	{
		return;
	}

	CONS_Printf(M_GetText("Encore Mode will be set to %s next round.\n"), cv_kartencore.string);
}

void KartEliminateLast_OnChange(void);
void KartEliminateLast_OnChange(void)
{
	P_CheckRacers();
}

void Schedule_OnChange(void);
void Schedule_OnChange(void)
{
	size_t i;

	if (cv_schedule.value)
	{
		return;
	}

	if (schedule_len == 0)
	{
		return;
	}

	// Reset timers when turning off.
	for (i = 0; i < schedule_len; i++)
	{
		scheduleTask_t *task = schedule[i];
		task->timer = task->basetime;
	}
}

void LiveStudioAudience_OnChange(void);
void LiveStudioAudience_OnChange(void)
{
	livestudioaudience_timer = 90;
}

void Got_DiscordInfo(const UINT8 **p, INT32 playernum)
{
	if (playernum != serverplayer /*&& !IsPlayerAdmin(playernum)*/)
	{
		// protect against hacked/buggy client
		CONS_Alert(CONS_WARNING, M_GetText("Illegal Discord info command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	// Don't do anything with the information if we don't have Discord RP support
#ifdef HAVE_DISCORDRPC
	discordInfo.maxPlayers = READUINT8(*p);
	discordInfo.joinsAllowed = (boolean)READUINT8(*p);
	discordInfo.everyoneCanInvite = (boolean)READUINT8(*p);

	DRPC_UpdatePresence();
#else
	(*p) += 3;
#endif
}
