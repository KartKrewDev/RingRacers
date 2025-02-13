// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  mserv.c
/// \brief Commands used to communicate with the master server

#if !defined (UNDER_CE)
#include <time.h>
#endif

#include "doomstat.h"
#include "doomdef.h"
#include "console.h" // con_startup
#include "command.h"
#include "i_threads.h"
#include "mserv.h"
#include "z_zone.h"

// SRB2Kart
#include "k_menu.h"

#ifdef HAVE_DISCORDRPC
#include "discord.h"
#endif

#ifdef MASTERSERVER

static int     MSId;
static int     MSRegisteredId = -1;

static boolean MSRegistered;
static boolean MSInProgress;
static boolean MSUpdateAgain;

static time_t  MSLastPing;

static char *MSRules;

#ifdef HAVE_THREADS
static I_mutex MSMutex;
static I_cond  MSCond;

#  define Lock_state()   I_lock_mutex  (&MSMutex)
#  define Unlock_state() I_unlock_mutex (MSMutex)
#else/*HAVE_THREADS*/
#  define Lock_state()
#  define Unlock_state()
#endif/*HAVE_THREADS*/

static void Command_Listserv_f(void);

#endif/*MASTERSERVER*/

void Update_parameters (void);

#if defined (MASTERSERVER) && defined (HAVE_THREADS)
int           ms_QueryId;
I_mutex       ms_QueryId_mutex;

msg_server_t *ms_ServerList;
I_mutex       ms_ServerList_mutex;
#endif

UINT16 current_port = 0;

/** Adds variables and commands relating to the master server.
  *
  * \sa cv_masterserver, cv_servername,
  *     Command_Listserv_f
  */
void AddMServCommands(void)
{
#ifdef MASTERSERVER
	COM_AddCommand("listserv", Command_Listserv_f);
	COM_AddCommand("masterserver_update", Update_parameters); // allows people to updates manually in case you were delisted by accident
#endif
}

#ifdef MASTERSERVER

static void WarnGUI (void)
{
#ifdef HAVE_THREADS
	I_lock_mutex(&k_menu_mutex);
#endif
	M_StartMessage("Online Play", M_GetText("There was a problem connecting to\nthe Master Server\n\nCheck the console for details.\n"), NULL, MM_NOTHING, NULL, NULL);
#ifdef HAVE_THREADS
	I_unlock_mutex(k_menu_mutex);
#endif
}

#define NUM_LIST_SERVER MAXSERVERLIST
msg_server_t *GetShortServersList(int id)
{
	msg_server_t *server_list;

	// +1 for easy test
	server_list = malloc(( NUM_LIST_SERVER + 1 ) * sizeof *server_list);

	if (HMS_fetch_servers(server_list, id))
		return server_list;
	else
	{
		free(server_list);
		WarnGUI();
		return NULL;
	}
}

#ifdef UPDATE_ALERT
char *GetMODVersion(int id)
{
	char *buffer;
	int c;

	(void)id;

	buffer = malloc(16);

	c = HMS_compare_mod_version(buffer, 16);

#ifdef HAVE_THREADS
	I_lock_mutex(&ms_QueryId_mutex);
	{
		if (id != ms_QueryId)
			c = -1;
	}
	I_unlock_mutex(ms_QueryId_mutex);
#endif

	if (c > 0)
		return buffer;
	else
	{
		free(buffer);

		if (! c)
			WarnGUI();

		return NULL;
	}
}
#endif

/** Gets a list of game servers. Called from console.
  */
static void Command_Listserv_f(void)
{
	CONS_Printf(M_GetText("Retrieving server list...\n"));

	{
		HMS_list_servers();
	}
}

static boolean firstmsrules = false;

static void
Get_masterserver_rules (boolean checkfirst)
{
	char rules[256];

	if (checkfirst)
	{
		boolean MSRulesExist;

		Lock_state();
		MSRulesExist = (MSRules != NULL);
		Unlock_state();

		if (MSRulesExist)
			return;
	}

	if (HMS_fetch_rules(rules, sizeof rules))
	{
		Lock_state();
		Z_Free(MSRules);
		MSRules = Z_StrDup(rules);

		if (MSRegistered == true)
		{
			CONS_Printf("\n");
			CONS_Alert(CONS_NOTICE, "%s\n", rules);
		}

		firstmsrules = true;

		Unlock_state();
	}
}

static void
Finish_registration (void)
{
	int registered;

	CONS_Printf("Registering this server on the master server...\n");

	registered = HMS_register();

	Lock_state();
	{
		MSRegistered = registered;
		MSRegisteredId = MSId;

		time(&MSLastPing);
	}
	Unlock_state();

	char *rules = GetMasterServerRules();
	if (rules == NULL)
	{
		Get_masterserver_rules(true);
	}
	else
	{
		CONS_Printf("\n");
		CONS_Alert(CONS_NOTICE, "%s\n", rules);
	}

	if (registered)
		CONS_Printf("Master server registration successful.\n");
}

static void
Finish_update (void)
{
	int registered;
	int done;

	Lock_state();
	{
		registered = MSRegistered;
		MSUpdateAgain = false;/* this will happen anyway */
	}
	Unlock_state();

	if (registered)
	{
		if (HMS_update())
		{
			Lock_state();
			{
				time(&MSLastPing);
				MSRegistered = true;
			}
			Unlock_state();

			CONS_Printf("Updated master server listing.\n");
		}
		else
			Finish_registration();
	}
	else
		Finish_registration();

	Lock_state();
	{
		done = ! MSUpdateAgain;

		if (done)
			MSInProgress = false;
	}
	Unlock_state();

	if (! done)
		Finish_update();
}

static void
Finish_unlist (void)
{
	int registered;

	Lock_state();
	{
		registered = MSRegistered;

		if (MSId == MSRegisteredId)
			MSId++;
	}
	Unlock_state();

	if (registered)
	{
		CONS_Printf("Removing this server from the master server...\n");

		if (HMS_unlist())
			CONS_Printf("Server deregistration request successfully sent.\n");

		Lock_state();
		{
			MSRegistered = false;
		}
		Unlock_state();

#ifdef HAVE_THREADS
		I_wake_all_cond(&MSCond);
#endif
	}
}

static void
Finish_masterserver_change (char *api)
{
	HMS_set_api(api);

	if (!con_startup)
		Get_masterserver_rules(false);
}

#ifdef HAVE_THREADS
static int *
Server_id (void)
{
	int *id;
	id = malloc(sizeof *id);
	Lock_state();
	{
		*id = MSId;
	}
	Unlock_state();
	return id;
}

static int *
New_server_id (void)
{
	int *id;
	id = malloc(sizeof *id);
	Lock_state();
	{
		*id = ++MSId;
		I_wake_all_cond(&MSCond);
	}
	Unlock_state();
	return id;
}

static void
Register_server_thread (int *id)
{
	int same;

	Lock_state();
	{
		/* wait for previous unlist to finish */
		while (*id == MSId && MSRegistered)
			I_hold_cond(&MSCond, MSMutex);

		same = ( *id == MSId );/* it could have been a while */
	}
	Unlock_state();

	if (same)/* it could have been a while */
		Finish_registration();

	free(id);
}

static void
Update_server_thread (int *id)
{
	int same;

	Lock_state();
	{
		same = ( *id == MSRegisteredId );
	}
	Unlock_state();

	if (same)
		Finish_update();

	free(id);
}

static void
Unlist_server_thread (int *id)
{
	int same;

	Lock_state();
	{
		same = ( *id == MSRegisteredId );
	}
	Unlock_state();

	if (same)
		Finish_unlist();

	free(id);
}

static void
Change_masterserver_thread (char *api)
{
	Lock_state();
	{
		while (MSRegistered)
			I_hold_cond(&MSCond, MSMutex);
	}
	Unlock_state();

	Finish_masterserver_change(api);
}

static void
Get_masterserver_rules_thread (void)
{
	// THIS FUNC has its own lock check in it
	Get_masterserver_rules(true);
}
#endif/*HAVE_THREADS*/

void RegisterServer(void)
{
#ifdef MASTERSERVER
#ifdef HAVE_THREADS
	I_spawn_thread(
			"register-server",
			(I_thread_fn)Register_server_thread,
			New_server_id()
	);
#else
	Finish_registration();
#endif
#endif/*MASTERSERVER*/
}

static void UpdateServer(void)
{
#ifdef HAVE_THREADS
	I_spawn_thread(
			"update-server",
			(I_thread_fn)Update_server_thread,
			Server_id()
	);
#else
	Finish_update();
#endif
}

void UnregisterServer(void)
{
#ifdef MASTERSERVER
#ifdef HAVE_THREADS
	I_spawn_thread(
			"unlist-server",
			(I_thread_fn)Unlist_server_thread,
			Server_id()
	);
#else
	Finish_unlist();
#endif
#endif/*MASTERSERVER*/
}

char *GetMasterServerRules(void)
{
	char *rules;

	Lock_state();
	rules = MSRules ? Z_StrDup(MSRules) : NULL;
	Unlock_state();

	return rules;
}

static boolean
Online (void)
{
	return ( serverrunning && netgame && cv_advertise.value );
}

static inline void SendPingToMasterServer(void)
{
	int ready;
	time_t now;

	if (Online())
	{
		time(&now);

		Lock_state();
		{
			ready = (
					MSRegisteredId == MSId &&
					! MSInProgress &&
					now >= ( MSLastPing + 60 * cv_masterserver_update_rate.value )
			);

			if (ready)
				MSInProgress = true;
		}
		Unlock_state();

		if (ready)
			UpdateServer();
	}
}

void MasterClient_Ticker(void)
{
#ifdef MASTERSERVER
	SendPingToMasterServer();
#endif
}

static void
Set_api (const char *api)
{
#ifdef HAVE_THREADS
	I_spawn_thread(
			"change-masterserver",
			(I_thread_fn)Change_masterserver_thread,
			strdup(api)
	);
#else
	Finish_masterserver_change(strdup(api));
#endif
}

void
Get_rules (void)
{
#ifdef HAVE_THREADS
	I_spawn_thread(
			"get-masterserver-rules",
			(I_thread_fn)Get_masterserver_rules_thread,
			NULL
	);
#else
	Get_masterserver_rules(true);
#endif
}

#endif/*MASTERSERVER*/

void
Update_parameters (void)
{
#ifdef MASTERSERVER
	int registered;
	int delayed;

	if (Online())
	{
		Lock_state();
		{
			delayed = MSInProgress;

			if (delayed)/* do another update after the current one */
				MSUpdateAgain = true;
			else
				registered = MSRegistered;
		}
		Unlock_state();

		if (! delayed && registered)
			UpdateServer();
	}
#endif/*MASTERSERVER*/
}

void MasterServer_OnChange(void);
void MasterServer_OnChange(void)
{
#ifdef MASTERSERVER
	UnregisterServer();

	Set_api(cv_masterserver.string);

	if (Online())
		RegisterServer();
#endif/*MASTERSERVER*/
}

void Advertise_OnChange(void);
void Advertise_OnChange(void)
{
	int different;

	if (cv_advertise.value)
	{
		if (serverrunning && netgame)
		{
			Lock_state();
			{
				different = ( MSId != MSRegisteredId );
			}
			Unlock_state();

			if (different)
			{
				RegisterServer();
			}
		}
	}
	else
	{
		UnregisterServer();
	}

#ifdef HAVE_DISCORDRPC
	DRPC_UpdatePresence();
#endif

	M_PopupMasterServerRules();
}
