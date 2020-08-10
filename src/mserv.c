// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
// Copyright (C)      2020 by James R.
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
#include "command.h"
#include "i_threads.h"
#include "mserv.h"
#include "m_menu.h"
#include "z_zone.h"

static int     MSId;
static int     MSRegisteredId = -1;

static boolean MSRegistered;
static boolean MSInProgress;
static boolean MSUpdateAgain;

static time_t  MSLastPing;

#ifdef HAVE_THREADS
static I_mutex MSMutex;
static I_cond  MSCond;

#  define Lock_state()   I_lock_mutex  (&MSMutex)
#  define Unlock_state() I_unlock_mutex (MSMutex)
#else/*HAVE_THREADS*/
#  define Lock_state()
#  define Unlock_state()
#endif/*HAVE_THREADS*/

static void Update_parameters (void);

#ifndef NONET
static void Command_Listserv_f(void);
#endif
static void MasterServer_OnChange(void);

<<<<<<< HEAD
#define DEF_PORT "28900"
consvar_t cv_masterserver = {"masterserver", "ms.srb2.org:"DEF_PORT, CV_SAVE, NULL, MasterServer_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_servername = {"servername", "SRB2Kart server", CV_SAVE|CV_CALL|CV_NOINIT, NULL, ServerName_OnChange, 0, NULL, NULL, 0, 0, NULL};
=======
static CV_PossibleValue_t masterserver_update_rate_cons_t[] = {
	{2,  "MIN"},
	{60, "MAX"},
	{0}
};
>>>>>>> srb2/next

consvar_t cv_masterserver = {"masterserver", "https://mb.srb2.org/MS/0", CV_SAVE|CV_CALL, NULL, MasterServer_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_servername = {"servername", "SRB2Kart server", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Update_parameters, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_masterserver_update_rate = {"masterserver_update_rate", "15", CV_SAVE|CV_CALL|CV_NOINIT, masterserver_update_rate_cons_t, Update_parameters, 0, NULL, NULL, 0, 0, NULL};

INT16 ms_RoomId = -1;

#ifdef HAVE_THREADS
int           ms_QueryId;
I_mutex       ms_QueryId_mutex;

msg_server_t *ms_ServerList;
I_mutex       ms_ServerList_mutex;
#endif

UINT16 current_port = 0;

// Room list is an external variable now.
// Avoiding having to get info ten thousand times...
msg_rooms_t room_list[NUM_LIST_ROOMS+1]; // +1 for easy test

/** Adds variables and commands relating to the master server.
  *
  * \sa cv_masterserver, cv_servername,
  *     Command_Listserv_f
  */
void AddMServCommands(void)
{
#ifndef NONET
	CV_RegisterVar(&cv_masterserver);
	CV_RegisterVar(&cv_masterserver_update_rate);
	CV_RegisterVar(&cv_masterserver_timeout);
	CV_RegisterVar(&cv_masterserver_debug);
	CV_RegisterVar(&cv_masterserver_token);
	CV_RegisterVar(&cv_servername);
	COM_AddCommand("listserv", Command_Listserv_f);
#endif
}

static void WarnGUI (void)
{
#ifdef HAVE_THREADS
	I_lock_mutex(&m_menu_mutex);
#endif
	M_StartMessage(M_GetText("There was a problem connecting to\nthe Master Server\n\nCheck the console for details.\n"), NULL, MM_NOTHING);
#ifdef HAVE_THREADS
	I_unlock_mutex(m_menu_mutex);
#endif
}

#define NUM_LIST_SERVER MAXSERVERLIST
msg_server_t *GetShortServersList(INT32 room, int id)
{
	msg_server_t *server_list;

	// +1 for easy test
	server_list = malloc(( NUM_LIST_SERVER + 1 ) * sizeof *server_list);

	if (HMS_fetch_servers(server_list, room, id))
		return server_list;
	else
	{
		free(server_list);
		WarnGUI();
		return NULL;
	}
}

INT32 GetRoomsList(boolean hosting, int id)
{
	if (HMS_fetch_rooms( ! hosting, id))
		return 1;
	else
	{
		WarnGUI();
		return -1;
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

// Console only version of the above (used before game init)
void GetMODVersion_Console(void)
{
	char buffer[16];

	if (HMS_compare_mod_version(buffer, sizeof buffer) > 0)
		I_Error(UPDATE_ALERT_STRING_CONSOLE, VERSIONSTRING, buffer);
}
#endif

#ifndef NONET
/** Gets a list of game servers. Called from console.
  */
static void Command_Listserv_f(void)
{
	CONS_Printf(M_GetText("Retrieving server list...\n"));

	{
		HMS_list_servers();
	}
}
#endif

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

	Lock_state();
	{
		if (MSId == MSRegisteredId)
			MSId++;
	}
	Unlock_state();
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
<<<<<<< HEAD
#ifdef NONET
	(void)firstadd;
#else
	static INT32 retry = 0;
	int i, res;
	socklen_t j;
	msg_t msg;
	msg_server_t *info = (msg_server_t *)msg.buffer;
	INT32 room = -1;
	fd_set tset;
	time_t timestamp = time(NULL);
	UINT32 signature, tmp;
	const char *insname;

	M_Memcpy(&tset, &wset, sizeof (tset));
	res = select(255, NULL, &tset, NULL, &select_timeout);
	if (res != ERRSOCKET && !res)
	{
		if (retry++ > 30) // an about 30 second timeout
		{
			retry = 0;
			CONS_Alert(CONS_ERROR, M_GetText("Master Server timed out\n"));
			MSLastPing = timestamp;
			return ConnectionFailed();
		}
		return MS_CONNECT_ERROR;
	}
	retry = 0;
	/*
	Somehow we can still select our old socket despite it being closed(?).
	Atleast, that's what I THINK is happening. Anyway, we have to check that we
	haven't open a socket, and actually open it!
	*/
	/*if (res == ERRSOCKET)*//* wtf? no! */
	if (socket_fd == (SOCKET_TYPE)ERRSOCKET)
	{
		if (MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Master Server socket error #%u: %s\n"), errno, strerror(errno));
			MSLastPing = timestamp;
			return ConnectionFailed();
		}
	}

	// so, the socket is writable, but what does that mean, that the connection is
	// ok, or bad... let see that!
	j = (socklen_t)sizeof (i);
	getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, (char *)&i, &j);
	/*
	This is also wrong. If getsockopt fails, i doesn't have to be set. Plus, if
	it is set (which it appearantly is on linux), we check errno anyway. And in
	the case that i is returned as normal, we don't even report the correct
	value! So we accomplish NOTHING, except returning due to dumb luck.
	If you care, fix this--I don't. -James (R.)
	*/
	if (i) // it was bad
=======
	int same;

	Lock_state();
>>>>>>> srb2/next
	{
		/* wait for previous unlist to finish */
		while (*id == MSId && MSRegistered)
			I_hold_cond(&MSCond, MSMutex);

<<<<<<< HEAD
#ifdef PARANOIA
	if (ms_RoomId <= 0)
		I_Error("Attmepted to host in room \"All\"!\n");
#endif
	room = ms_RoomId;

	for(signature = 0, insname = cv_servername.string; *insname; signature += *insname++);
	tmp = (UINT32)(signature * (size_t)&MSLastPing);
	signature *= tmp;
	signature &= 0xAAAAAAAA;
	M_Memcpy(&info->header.signature, &signature, sizeof (UINT32));

	strcpy(info->ip, "");
	strcpy(info->port, int2str(current_port));
	strcpy(info->name, cv_servername.string);
	M_Memcpy(&info->room, & room, sizeof (INT32));
#if VERSION > 0 || SUBVERSION > 0
	sprintf(info->version, "%d.%d", VERSION, SUBVERSION);
#else // Trunk build, send revision info
	strcpy(info->version, GetRevisionString());
#endif
	strcpy(registered_server.name, cv_servername.string);

	if(firstadd)
		msg.type = ADD_SERVER_MSG;
	else
		msg.type = PING_SERVER_MSG;

	msg.length = (UINT32)sizeof (msg_server_t);
	msg.room = 0;
	if (MS_Write(&msg) < 0)
	{
		MSLastPing = timestamp;
		return ConnectionFailed();
=======
		same = ( *id == MSId );/* it could have been a while */
>>>>>>> srb2/next
	}
	Unlock_state();

	if (same)/* it could have been a while */
		Finish_registration();

	free(id);
}

static void
Update_server_thread (int *id)
{
<<<<<<< HEAD
	msg_t msg;
	msg_server_t *info = (msg_server_t *)msg.buffer;

	strcpy(info->header.buffer, "");
	strcpy(info->ip, "");
	strcpy(info->port, int2str(current_port));
	strcpy(info->name, registered_server.name);
	sprintf(info->version, "%d.%d", VERSION, SUBVERSION);

	msg.type = REMOVE_SERVER_MSG;
	msg.length = (UINT32)sizeof (msg_server_t);
	msg.room = 0;
	if (MS_Write(&msg) < 0)
		return MS_WRITE_ERROR;

	return MS_NO_ERROR;
}
=======
	int same;
>>>>>>> srb2/next

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

	HMS_set_api(api);
}
#endif/*HAVE_THREADS*/

void RegisterServer(void)
{
#ifdef HAVE_THREADS
	I_spawn_thread(
			"register-server",
			(I_thread_fn)Register_server_thread,
			New_server_id()
	);
#else
	Finish_registration();
#endif
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
#ifdef HAVE_THREADS
	I_spawn_thread(
			"unlist-server",
			(I_thread_fn)Unlist_server_thread,
			Server_id()
	);
#else
	Finish_unlist();
#endif
}

static boolean
Online (void)
{
	return ( serverrunning && ms_RoomId > 0 );
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

static void
Update_parameters (void)
{
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
}

void MasterClient_Ticker(void)
{
	SendPingToMasterServer();
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
	HMS_set_api(strdup(api));
#endif
}

static void MasterServer_OnChange(void)
{
	UnregisterServer();

	/*
	TODO: remove this for v2, it's just a hack
	for those coming in with an old config.
	*/
	if (
			! cv_masterserver.changed &&
			strcmp(cv_masterserver.string, "ms.srb2.org:28900") == 0
	){
		CV_StealthSet(&cv_masterserver, cv_masterserver.defaultvalue);
	}

	Set_api(cv_masterserver.string);

	if (Online())
		RegisterServer();
}
