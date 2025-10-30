// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2016 by Kay "Kaito" Sinclaire.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  menus/play-online-server-browser.c
/// \brief MULTIPLAYER ROOM FETCH / REFRESH THREADS

#include "../k_menu.h"
#include "../v_video.h"
#include "../s_sound.h"

//#define SERVERLISTDEBUG

#ifdef SERVERLISTDEBUG
#include "../m_random.h"

void M_ServerListFillDebug(void);
#endif

// Quit
static boolean M_ServerBrowserQuit(void)
{
	serverlistultimatecount = 0;
	serverlistmode = false;
	return true;
}

menuitem_t PLAY_MP_ServerBrowser[] =
{

	{IT_STRING | IT_CVAR, "SORT BY", NULL,	// tooltip MUST be null.
		NULL, {.cvar = &cv_serversort}, 0, 0},

	{IT_STRING | IT_CALL, "REFRESH", NULL,
		NULL, {.routine = &M_RefreshServers}, 0, 0},

	{IT_NOTHING, NULL, NULL, NULL, {NULL}, 0, 0},
};

menu_t PLAY_MP_ServerBrowserDef = {
	sizeof (PLAY_MP_ServerBrowser) / sizeof (menuitem_t),
	&PLAY_MP_RoomSelectDef,
	0,
	PLAY_MP_ServerBrowser,
	32, 36,
	0, 0,
	0,
	"NETMD2",
	0, 0,
	M_DrawMPServerBrowser,
	NULL,
	M_MPServerBrowserTick,
	NULL,
	M_ServerBrowserQuit,
	M_ServerBrowserInputs
};

// for server fetch threads...
M_waiting_mode_t m_waiting_mode = M_NOT_WAITING;

void
M_SetWaitingMode (int mode)
{
#ifdef HAVE_THREADS
	I_lock_mutex(&k_menu_mutex);
#endif
	{
		m_waiting_mode = mode;
	}
#ifdef HAVE_THREADS
	I_unlock_mutex(k_menu_mutex);
#endif
}

int
M_GetWaitingMode (void)
{
	int mode;

#ifdef HAVE_THREADS
	I_lock_mutex(&k_menu_mutex);
#endif
	{
		mode = m_waiting_mode;
	}
#ifdef HAVE_THREADS
	I_unlock_mutex(k_menu_mutex);
#endif

	return mode;
}

#ifdef MASTERSERVER
#ifdef HAVE_THREADS
void
Spawn_masterserver_thread (const char *name, void (*thread)(int*))
{
	int *id = malloc(sizeof *id);

	I_lock_mutex(&ms_QueryId_mutex);
	{
		*id = ms_QueryId;
	}
	I_unlock_mutex(ms_QueryId_mutex);

	I_spawn_thread(name, (I_thread_fn)thread, id);
}

int
Same_instance (int id)
{
	int okay;

	I_lock_mutex(&ms_QueryId_mutex);
	{
		okay = ( id == ms_QueryId );
	}
	I_unlock_mutex(ms_QueryId_mutex);

	return okay;
}
#endif/*HAVE_THREADS*/

void
Fetch_servers_thread (int *id)
{
	msg_server_t * server_list;

	(void)id;

	M_SetWaitingMode(M_WAITING_SERVERS);

#ifdef HAVE_THREADS
	server_list = GetShortServersList(*id);
#else
	server_list = GetShortServersList(0);
#endif

	if (server_list)
	{
#ifdef HAVE_THREADS
		if (Same_instance(*id))
#endif
		{
			M_SetWaitingMode(M_NOT_WAITING);

#ifdef HAVE_THREADS
			I_lock_mutex(&ms_ServerList_mutex);
			{
				ms_ServerList = server_list;
			}
			I_unlock_mutex(ms_ServerList_mutex);
#else
			CL_QueryServerList(server_list);
			free(server_list);
#endif
		}
#ifdef HAVE_THREADS
		else
		{
			free(server_list);
		}
#endif
	}

#ifdef HAVE_THREADS
	free(id);
#endif
}
#endif/*MASTERSERVER*/

// updates serverlist
void M_RefreshServers(INT32 choice)
{
	(void)choice;

	CL_UpdateServerList();

#ifdef SERVERLISTDEBUG
	M_ServerListFillDebug();
#else /*SERVERLISTDEBUG*/
#ifdef MASTERSERVER
#ifdef HAVE_THREADS
	Spawn_masterserver_thread("fetch-servers", Fetch_servers_thread);
#else/*HAVE_THREADS*/
	Fetch_servers_thread(NULL);
#endif/*HAVE_THREADS*/
#endif/*MASTERSERVER*/
#endif /*SERVERLISTDEBUG*/
}

#ifdef UPDATE_ALERT
static void M_CheckMODVersion(int id)
{
	char updatestring[500];
	const char *updatecheck = GetMODVersion(id);
	if(updatecheck)
	{
		sprintf(updatestring, UPDATE_ALERT_STRING, VERSIONSTRING, updatecheck);
#ifdef HAVE_THREADS
		I_lock_mutex(&k_menu_mutex);
#endif
		M_StartMessage("Game Update", updatestring, NULL, MM_NOTHING, NULL, "Return to Menu");
#ifdef HAVE_THREADS
		I_unlock_mutex(k_menu_mutex);
#endif
	}
}
#endif/*UPDATE_ALERT*/

#if defined (UPDATE_ALERT) && defined (HAVE_THREADS)
static void
Check_new_version_thread (int *id)
{
	M_SetWaitingMode(M_WAITING_VERSION);

	M_CheckMODVersion(*id);

	if (Same_instance(*id))
	{
		Fetch_servers_thread(id);
	}
	else
	{
		free(id);
	}
}
#endif/*defined (UPDATE_ALERT) && defined (HAVE_THREADS)*/


// Initializes serverlist when entering the menu...
void M_ServersMenu(INT32 choice)
{
	(void)choice;
	// modified game check: no longer handled
	// we don't request a restart unless the filelist differs

	serverlistmode = true;

	CL_UpdateServerList();

	mpmenu.ticker = 0;
	mpmenu.servernum = 0;
	mpmenu.scrolln = 0;
	mpmenu.slide = 0;

	PLAY_MP_ServerBrowserDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PLAY_MP_ServerBrowserDef, false);
	itemOn = 0;

#ifdef SERVERLISTDEBUG
	M_ServerListFillDebug();
#else /*SERVERLISTDEBUG*/

#if defined (MASTERSERVER) && defined (HAVE_THREADS)
	I_lock_mutex(&ms_QueryId_mutex);
	{
		ms_QueryId++;
	}
	I_unlock_mutex(ms_QueryId_mutex);

	I_lock_mutex(&ms_ServerList_mutex);
	{
		if (ms_ServerList)
		{
			free(ms_ServerList);
			ms_ServerList = NULL;
		}
	}
	I_unlock_mutex(ms_ServerList_mutex);

#ifdef UPDATE_ALERT
	Spawn_masterserver_thread("check-new-version", Check_new_version_thread);
#else/*UPDATE_ALERT*/
	Spawn_masterserver_thread("fetch-servers", Fetch_servers_thread);
#endif/*UPDATE_ALERT*/
#else/*defined (MASTERSERVER) && defined (HAVE_THREADS)*/
#ifdef UPDATE_ALERT
	M_CheckMODVersion(0);
#endif/*UPDATE_ALERT*/
	M_RefreshServers(0);
#endif/*defined (MASTERSERVER) && defined (HAVE_THREADS)*/

#endif /*SERVERLISTDEBUG*/
}

#ifdef SERVERLISTDEBUG

// Fill serverlist with a bunch of garbage to make our life easier in debugging
void M_ServerListFillDebug(void)
{
	UINT8 i = 0;

	serverlistcount = 40;
	memset(serverlist, 0, sizeof(serverlist));	// zero out the array for convenience...

	for (i = 0; i < serverlistcount; i++)
	{
		// We don't really care about the server node for this, let's just fill in the info so that we have a visual...
		serverlist[i].info.maxplayer = M_RandomRange(8, 16);
		UINT8 val = i % 16;
		serverlist[i].info.numberofplayer = min(val, serverlist[i].info.maxplayer);

		serverlist[i].info.avgpwrlv = M_RandomRange(500, 1500);
		serverlist[i].info.time = M_RandomRange(1, 8);	// ping

		strcpy(serverlist[i].info.servername, va("Serv %d", i+1));

		const char *tempgtname;
		if (i < 10 && mpmenu.room == 1)
		{
			tempgtname = va("%c%c%c%c",
				'A' + M_RandomKey(26),
				'a' + M_RandomKey(26),
				'a' + M_RandomKey(26),
				'a' + M_RandomKey(26)
			);
		}
		else
			tempgtname = (i & 1) ? "Race" : "Battle";

		strcpy(serverlist[i].info.gametypename, tempgtname);

		const INT32 gtidentifier = G_GetGametypeByName(tempgtname);
		UINT8 gtcalc = GTCALC_RACE;
		if (gtidentifier != GT_RACE)
		{
			gtcalc = (gtidentifier == GT_BATTLE) ? GTCALC_BATTLE : GTCALC_CUSTOM;
		}
		serverlist[i].cachedgtcalc = gtcalc;

		serverlist[i].info.kartvars = M_RandomRange(0, 3) & SV_SPEEDMASK;

		serverlist[i].info.modifiedgame = M_RandomRange(0, 1);

		CONS_Printf("Serv %d | %d...\n", i, serverlist[i].info.modifiedgame);
	}

	M_SortServerList();
}

#endif // SERVERLISTDEBUG

// Ascending order, not descending.
// The casts are safe as long as the caller doesn't do anything stupid.
#define SERVER_LIST_ENTRY_COMPARATOR(key) \
static int ServerListEntryComparator_##key(const void *entry1, const void *entry2) \
{ \
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2; \
	if (sa->info.key != sb->info.key) \
		return sa->info.key - sb->info.key; \
	return sa->info.time - sb->info.time; \
}

// This does descending instead of ascending.
#define SERVER_LIST_ENTRY_COMPARATOR_REVERSE(key) \
static int ServerListEntryComparator_##key##_reverse(const void *entry1, const void *entry2) \
{ \
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2; \
	if (sb->info.key != sa->info.key) \
		return sb->info.key - sa->info.key; \
	return sa->info.time - sb->info.time; \
}

//SERVER_LIST_ENTRY_COMPARATOR(time) -- done seperately due to the usual tiebreaker being time
SERVER_LIST_ENTRY_COMPARATOR(numberofplayer)
SERVER_LIST_ENTRY_COMPARATOR_REVERSE(numberofplayer)
SERVER_LIST_ENTRY_COMPARATOR_REVERSE(maxplayer)
SERVER_LIST_ENTRY_COMPARATOR(avgpwrlv)

static int ServerListEntryComparator_time(const void *entry1, const void *entry2)
{
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2;
	if (sa->info.time != sb->info.time)
		return sa->info.time - sb->info.time;
	return strcmp(sa->info.servername, sb->info.servername);
}

static int ServerListEntryComparator_gametypename(const void *entry1, const void *entry2)
{
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2;
	int c;
	if (( c = strcasecmp(sa->info.gametypename, sb->info.gametypename) ))
		return c;
	return sa->info.time - sb->info.time;
}

static int ServerListEntryComparator_recommended(const void *entry1, const void *entry2)
{
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2;

	INT32 saseedval = sa->info.numberofplayer;
	INT32 sbseedval = sb->info.numberofplayer;

	// Tyron wrote the following on 25072022:
	// "sort should be two parts
	// top part of the list is "all non-empty servers sorted by reverse playercount", with servers above a certain reported ping marked as bad connection or whatever
	// bottom part of the list is all empty servers sorted by ping"
	// toast is implementing on 27082023, over a year later, because
	// "fixing server join flow" is saner to do near the end

	{
		const UINT8 SERVER_EMPTY = 1;

		// The intent with this nudge is to show you
		// good games you could get a memorable Duel in,
		// with the possibility to really katamari into
		// something more substantial.
		// By comparison, empty games are not nearly as
		// fun to get going, so let's lower their SEO.
		if (!saseedval)
			saseedval = MAXPLAYERS + SERVER_EMPTY;
		if (!sbseedval)
			sbseedval = MAXPLAYERS + SERVER_EMPTY;
	}

	if (saseedval != sbseedval)
		return saseedval - sbseedval;

	return sa->info.time - sb->info.time;
}

void M_SortServerList(void)
{
	switch(cv_serversort.value)
	{
	case -1:
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_recommended);
		break;
	case 0:		// Ping.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_time);
		break;
	case 1:		// AVG. Power Level
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_avgpwrlv);
		break;
	case 2:		// Most players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_numberofplayer_reverse);
		break;
	case 3:		// Least players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_numberofplayer);
		break;
	case 4:		// Max players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_maxplayer_reverse);
		break;
	case 5:		// Gametype.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_gametypename);
		break;
	}
}


// Server browser inputs & ticker
void M_MPServerBrowserTick(void)
{
	mpmenu.ticker++;
	mpmenu.slide /= 2;

#if defined (MASTERSERVER) && defined (HAVE_THREADS)
	I_lock_mutex(&ms_ServerList_mutex);
	{
		if (ms_ServerList)
		{
			CL_QueryServerList(ms_ServerList);
			free(ms_ServerList);
			ms_ServerList = NULL;
		}
	}
	I_unlock_mutex(ms_ServerList_mutex);
#endif

	CL_TimeoutServerList();
}

static void M_ServerBrowserConfirm(INT32 choice)
{
	if (choice != MA_YES)
		return;

	COM_BufAddText(va("connect node %d\n", serverlist[mpmenu.servernum].node));

	M_PleaseWait();
}

// Input handler for server browser.
boolean M_ServerBrowserInputs(INT32 ch)
{
	UINT8 pid = 0;
	INT16 maxscroll = serverlistcount - (SERVERSPERPAGE/2) - 2; // Why? Because
	if (maxscroll < 0)
		maxscroll = 0;

	const INT16 serverbrowserOn = (currentMenu->numitems - 1);

	(void) ch;

	if (!itemOn && menucmd[pid].dpad_ud < 0)
	{
		if (serverlistcount)
		{
			// Return the MS listing to the bottom.
			INT32 prevscroll = mpmenu.scrolln;

			mpmenu.servernum = serverlistcount-1;
			mpmenu.scrolln = maxscroll;
			mpmenu.slide = SERVERSPACE * (prevscroll - (INT32)mpmenu.scrolln);
		}
		else
		{
			M_PrevOpt(); // Double apply
		}
	}
	else if (itemOn == (serverbrowserOn - 1) && menucmd[pid].dpad_ud > 0 && !serverlistcount)
	{
		M_NextOpt(); // Double apply
	}
	else if (itemOn == serverbrowserOn)	// server browser itself...
	{
#ifndef SERVERLISTDEBUG
		if (M_MenuConfirmPressed(pid))
		{
			M_SetMenuDelay(pid);

			if (serverlist[mpmenu.servernum].info.numberofplayer >= 8)
			{
				S_StartSound(NULL, sfx_s3k96);
				M_StartMessage("Some advice...",
					"Ring Racers was designed for \x82""8 or fewer players""\x80"".\n""\x80""Racing may be ""\x82""more frustrating""\x80"" in large games.\n""\x86""(Some servers allow more spectators than players.)",
				M_ServerBrowserConfirm, MM_YESNO, "Bring on the imbalance...!", "Never mind!");
			}
			else
			{
				M_ServerBrowserConfirm(MA_YES);
			}

			return true;
		}
#endif

		if (menucmd[pid].dpad_ud > 0)	// down
		{
			if ((UINT32)(mpmenu.servernum+1) < serverlistcount)
			{
				// Listing scroll down
				mpmenu.servernum++;
				if (mpmenu.scrolln < maxscroll && mpmenu.servernum > SERVERSPERPAGE/2)
				{
					mpmenu.scrolln++;
					mpmenu.slide += SERVERSPACE;
				}

				S_StartSound(NULL, sfx_s3k5b);
				M_SetMenuDelay(pid);

				return true;
			}

			// Return the MS listing to the top.
			INT32 prevscroll = mpmenu.scrolln;

			mpmenu.servernum = 0;
			mpmenu.scrolln = 0;
			mpmenu.slide = SERVERSPACE * (prevscroll - (INT32)mpmenu.scrolln);
		}
		else if (menucmd[pid].dpad_ud < 0)
		{
			if (mpmenu.servernum)
			{
				// Listing scroll up
				if (mpmenu.servernum <= (INT16)maxscroll && mpmenu.scrolln)
				{
					mpmenu.scrolln--;
					mpmenu.slide -= SERVERSPACE;
				}
				mpmenu.servernum--;

				S_StartSound(NULL, sfx_s3k5b);
				M_SetMenuDelay(pid);

				return true;
			}
		}
	}
	return false;	// use normal behaviour.
}
