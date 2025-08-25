// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  discord.h
/// \brief Discord Rich Presence handling

#ifdef HAVE_DISCORDRPC

#include <time.h>

#include "i_system.h"
#include "d_clisrv.h"
#include "d_netcmd.h"
#include "i_net.h"
#include "g_game.h"
#include "p_tick.h"
#include "k_menu.h" // gametype_cons_t
#include "r_things.h" // skins
#include "mserv.h" // cv_advertise
#include "s_sound.h"
#include "z_zone.h"
#include "byteptr.h"
#include "stun.h"
#include "i_tcp.h" // current_port
#include "k_grandprix.h"
#include "k_battle.h"
#include "m_cond.h" // M_GameTrulyStarted

#include "discord.h"
#include "doomdef.h"

// Feel free to provide your own, if you care enough to create another Discord app for this :P
#define DISCORD_APPID "977470696852684833"

// An undocumented feature of Discord RPC is the ability to host images on your own URL.
// We use this to avoid the asset count restrictions on Discord apps.
#define IMAGE_REPO "https://www.kartkrew.org/theme/images/drpc-rr/"
#define IMAGE_EXT ".png"

#ifdef DEVELOP
#define DISCORD_SECRETIVE
#endif

// length of IP strings
#define IP_SIZE 21

struct discordInfo_s discordInfo;

discordRequest_t *discordRequestList = NULL;

size_t g_discord_skins = 0;

static char self_ip[IP_SIZE];

/*--------------------------------------------------
	const char *DRPC_HideUsername(const char *input)

		See header file for description.
--------------------------------------------------*/
const char *DRPC_HideUsername(const char *input)
{
	static char buffer[5];
	int i;

	buffer[0] = input[0];

	for (i = 1; i < 4; ++i)
	{
		buffer[i] = '.';
	}

	buffer[4] = '\0';
	return buffer;
}

/*--------------------------------------------------
	static char *DRPC_XORIPString(const char *input)

		Simple XOR encryption/decryption. Not complex or
		very secretive because we aren't sending anything
		that isn't easily accessible via our Master Server anyway.
--------------------------------------------------*/
static char *DRPC_XORIPString(const char *input)
{
	const UINT8 xor[IP_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};
	char *output = malloc(sizeof(char) * (IP_SIZE+1));
	UINT8 i;

	for (i = 0; i < IP_SIZE; i++)
	{
		char xorinput;

		if (!input[i])
			break;

		xorinput = input[i] ^ xor[i];

		if (xorinput < 32 || xorinput > 126)
		{
			xorinput = input[i];
		}

		output[i] = xorinput;
	}

	output[i] = '\0';

	return output;
}

/*--------------------------------------------------
	static void DRPC_HandleReady(const DiscordUser *user)

		Callback function, ran when the game connects to Discord.

	Input Arguments:-
		user - Struct containing Discord user info.

	Return:-
		None
--------------------------------------------------*/
static void DRPC_HandleReady(const DiscordUser *user)
{
	if (cv_discordstreamer.value)
	{
		CONS_Printf("Discord: connected to %s\n", DRPC_HideUsername(user->username));
	}
	else
	{
		CONS_Printf("Discord: connected to %s (%s)\n", user->username, user->userId);
	}
}

/*--------------------------------------------------
	static void DRPC_HandleDisconnect(int err, const char *msg)

		Callback function, ran when disconnecting from Discord.

	Input Arguments:-
		err - Error type
		msg - Error message

	Return:-
		None
--------------------------------------------------*/
static void DRPC_HandleDisconnect(int err, const char *msg)
{
	CONS_Printf("Discord: disconnected (%d: %s)\n", err, msg);
}

/*--------------------------------------------------
	static void DRPC_HandleError(int err, const char *msg)

		Callback function, ran when Discord outputs an error.

	Input Arguments:-
		err - Error type
		msg - Error message

	Return:-
		None
--------------------------------------------------*/
static void DRPC_HandleError(int err, const char *msg)
{
	CONS_Alert(CONS_WARNING, "Discord error (%d: %s)\n", err, msg);
}

/*--------------------------------------------------
	static void DRPC_HandleJoin(const char *secret)

		Callback function, ran when Discord wants to
		connect a player to the game via a channel invite
		or a join request.

	Input Arguments:-
		secret - Value that links you to the server.

	Return:-
		None
--------------------------------------------------*/
static void DRPC_HandleJoin(const char *secret)
{
	char *ip = DRPC_XORIPString(secret);
	CONS_Printf("Connecting to %s via Discord\n", ip);
	M_ClearMenus(true); //Don't have menus open during connection screen
	if (demo.playback && demo.attract)
		G_CheckDemoStatus(); //Stop the title demo, so that the connect command doesn't error if a demo is playing
	COM_BufAddText(va("connect \"%s\"\n", ip));
	free(ip);
}

/*--------------------------------------------------
	static boolean DRPC_InvitesAreAllowed(void)

		Determines whenever or not invites or
		ask to join requests are allowed.

	Input Arguments:-
		None

	Return:-
		true if invites are allowed, false otherwise.
--------------------------------------------------*/
static boolean DRPC_InvitesAreAllowed(void)
{
	if (!Playing())
	{
		// We're not playing, so we should not be getting invites.
		return false;
	}

	if (cv_discordasks.value == 0)
	{
		// Client has the CVar set to off, so never allow invites from this client.
		return false;
	}

	if (discordInfo.joinsAllowed == true)
	{
		if (discordInfo.everyoneCanInvite == true)
		{
			// Everyone's allowed!
			return true;
		}
		else if (consoleplayer == serverplayer || IsPlayerAdmin(consoleplayer))
		{
			// Only admins are allowed!
			return true;
		}
	}

	// Did not pass any of the checks
	return false;
}

/*--------------------------------------------------
	static void DRPC_HandleJoinRequest(const DiscordUser *requestUser)

		Callback function, ran when Discord wants to
		ask the player if another Discord user can join
		or not.

	Input Arguments:-
		requestUser - DiscordUser struct for the user trying to connect.

	Return:-
		None
--------------------------------------------------*/
static void DRPC_HandleJoinRequest(const DiscordUser *requestUser)
{
	discordRequest_t *append = discordRequestList;
	discordRequest_t *newRequest;

	if (DRPC_InvitesAreAllowed() == false)
	{
		// Something weird happened if this occurred...
		Discord_Respond(requestUser->userId, DISCORD_REPLY_IGNORE);
		return;
	}

	newRequest = Z_Calloc(sizeof(discordRequest_t), PU_STATIC, NULL);

	newRequest->username = Z_Calloc(344, PU_STATIC, NULL);
	snprintf(newRequest->username, 344, "%s", requestUser->username);

#if 0
	newRequest->discriminator = Z_Calloc(8, PU_STATIC, NULL);
	snprintf(newRequest->discriminator, 8, "%s", requestUser->discriminator);
#endif

	newRequest->userID = Z_Calloc(32, PU_STATIC, NULL);
	snprintf(newRequest->userID, 32, "%s", requestUser->userId);

	if (append != NULL)
	{
		discordRequest_t *prev = NULL;

		while (append != NULL)
		{
			// CHECK FOR DUPES!! Ignore any that already exist from the same user.
			if (!strcmp(newRequest->userID, append->userID))
			{
				Discord_Respond(newRequest->userID, DISCORD_REPLY_IGNORE);
				DRPC_RemoveRequest(newRequest);
				return;
			}

			prev = append;
			append = append->next;
		}

		newRequest->prev = prev;
		prev->next = newRequest;
	}
	else
	{
		discordRequestList = newRequest;
		//M_RefreshPauseMenu();
	}

	// Made it to the end, request was valid, so play the request sound :)
	S_StartSound(NULL, sfx_requst);
}

/*--------------------------------------------------
	void DRPC_RemoveRequest(discordRequest_t *removeRequest)

		See header file for description.
--------------------------------------------------*/
void DRPC_RemoveRequest(discordRequest_t *removeRequest)
{
	if (removeRequest->prev != NULL)
	{
		removeRequest->prev->next = removeRequest->next;
	}

	if (removeRequest->next != NULL)
	{
		removeRequest->next->prev = removeRequest->prev;

		if (removeRequest == discordRequestList)
		{
			discordRequestList = removeRequest->next;
		}
	}
	else
	{
		if (removeRequest == discordRequestList)
		{
			discordRequestList = NULL;
		}
	}

	Z_Free(removeRequest->username);
#if 0
	Z_Free(removeRequest->discriminator);
#endif
	Z_Free(removeRequest->userID);
	Z_Free(removeRequest);
}

/*--------------------------------------------------
	void DRPC_Init(void)

		See header file for description.
--------------------------------------------------*/
void DRPC_Init(void)
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));

	handlers.ready = DRPC_HandleReady;
	handlers.disconnected = DRPC_HandleDisconnect;
	handlers.errored = DRPC_HandleError;
	handlers.joinGame = DRPC_HandleJoin;
	handlers.joinRequest = DRPC_HandleJoinRequest;

	Discord_Initialize(DISCORD_APPID, &handlers, 1, NULL);
	I_AddExitFunc(Discord_Shutdown);
	DRPC_UpdatePresence();
}

/*--------------------------------------------------
	static void DRPC_GotServerIP(UINT32 address)

		Callback triggered by successful STUN response.

	Input Arguments:-
		address - IPv4 address of this machine, in network byte order.

	Return:-
		None
--------------------------------------------------*/
static void DRPC_GotServerIP(UINT32 address)
{
	const unsigned char * p = (const unsigned char *)&address;
	sprintf(self_ip, "%u.%u.%u.%u:%u", p[0], p[1], p[2], p[3], current_port);
	DRPC_UpdatePresence();
}

/*--------------------------------------------------
	static const char *DRPC_GetServerIP(void)

		Retrieves the IP address of the server that you're
		connected to. Will attempt to use curl for getting your
		own IP address, if it's not yours.
--------------------------------------------------*/
static const char *DRPC_GetServerIP(void)
{
	const char *address;

	// If you're connected
	if (I_GetNodeAddress && (address = I_GetNodeAddress(servernode)) != NULL)
	{
		if (strcmp(address, "self"))
		{
			// We're not the server, so we could successfully get the IP!
			// No need to do anything else :)
			return address;
		}
	}

	if (self_ip[0])
	{
		return self_ip;
	}
	else
	{
		// There happens to be a good way to get it after all! :D
		STUN_bind(DRPC_GotServerIP);
		return NULL;
	}
}

/*--------------------------------------------------
	void DRPC_EmptyRequests(void)

		Empties the request list. Any existing requests
		will get an ignore reply.
--------------------------------------------------*/
static void DRPC_EmptyRequests(void)
{
	while (discordRequestList != NULL)
	{
		Discord_Respond(discordRequestList->userID, DISCORD_REPLY_IGNORE);
		DRPC_RemoveRequest(discordRequestList);
	}
}

#ifndef DISCORD_SECRETIVE
/*--------------------------------------------------
	static boolean DRPC_DisplayGonerSetup(void)

		Returns true if we're in the initial
		tutorial game state.
--------------------------------------------------*/
static boolean DRPC_DisplayGonerSetup(void)
{
	if (M_GameTrulyStarted())
	{
		// We're past all that tutorial stuff.
		return false;
	}

	if (Playing())
	{
		// Need to check a bunch of stuff manually,
		// since with command line and/or console you
		// can play a bit of the game without fully
		// fully starting the game.

		if (netgame)
		{
			// We smuggled into a netgame early,
			// show the netgame's info.
			return false;
		}

		if (tutorialchallenge == TUTORIALSKIP_INPROGRESS)
		{
			// Attempting the Dirty Bubble Challenge
			return true;
		}

		// If it's not GT_TUTORIAL, it's directly
		// command line into a specific map.
		return (gametype == GT_TUTORIAL);
	}

	// If we're in a menu, and the game hasn't started,
	// then we're definitely in goner setup.
	return true;
}
#endif

enum {
	DISCORD_GS_UNKNOWN,
	DISCORD_GS_CUSTOM,
	DISCORD_GS_RACE,
	DISCORD_GS_BATTLE,
	DISCORD_GS_TUTORIAL,
	DISCORD_GS_TIMEATTACK,
	DISCORD_GS_GRANDPRIX,
	DISCORD_GS_VOTING,
	DISCORD_GS_MENU,
	DISCORD_GS_REPLAY,
	DISCORD_GS_TITLE,
	DISCORD_GS_CREDITS,
	DISCORD_GS_GONER
};

/*--------------------------------------------------
	void DRPC_UpdatePresence(void)

		See header file for description.
--------------------------------------------------*/
void DRPC_UpdatePresence(void)
{
	boolean joinSecretSet = false;
	char *clientJoinSecret = NULL;

	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));

	if (dedicated)
	{
		return;
	}

	if (!cv_discordrp.value)
	{
		// User doesn't want to show their game information, so update with empty presence.
		// This just shows that they're playing SRB2Kart. (If that's too much, then they should disable game activity :V)
		DRPC_EmptyRequests();
		Discord_UpdatePresence(&discordPresence);
		return;
	}

#ifdef DISCORD_SECRETIVE
	// This way, we can use the invite feature in-dev, but not have snoopers seeing any potential secrets! :P
	discordPresence.largeImageKey = IMAGE_REPO "misc_develop" IMAGE_EXT;
	discordPresence.largeImageText = "No peeking!";
	discordPresence.state = "Development EXE";

	if (netgame)
	{
		if (DRPC_InvitesAreAllowed() == true)
		{
			const char *join;

			// Grab the host's IP for joining.
			if ((join = DRPC_GetServerIP()) != NULL)
			{
				clientJoinSecret = DRPC_XORIPString(join);
				discordPresence.joinSecret = clientJoinSecret;
				joinSecretSet = true;
			}
			else
			{
				return;
			}
		}

		discordPresence.partyId = server_context; // Thanks, whoever gave us Mumble support, for implementing the EXACT thing Discord wanted for this field!
		discordPresence.partySize = D_NumPlayers(); // Players in server
		discordPresence.partyMax = discordInfo.maxPlayers; // Max players
	}
	else
	{
		// Reset discord info if you're not in a place that uses it!
		// Important for if you join a server that compiled without HAVE_DISCORDRPC,
		// so that you don't ever end up using bad information from another server.
		memset(&discordInfo, 0, sizeof(discordInfo));
	}

#else

	char detailstr[128];
	char localstr[128];

	char charimg[128];
	char charname[128];

	char largeimg[128];
	char largename[128];

	UINT8 gs = DISCORD_GS_UNKNOWN;
	if (DRPC_DisplayGonerSetup())
	{
		gs = DISCORD_GS_GONER;
	}
	else if (demo.playback)
	{
		switch (demo.attract)
		{
			case DEMO_ATTRACT_TITLE:
			{
				gs = DISCORD_GS_TITLE;
				break;
			}
			case DEMO_ATTRACT_CREDITS:
			{
				gs = DISCORD_GS_CREDITS;
				break;
			}
			default:
			{
				gs = DISCORD_GS_REPLAY;
				break;
			}
		}
	}
	else
	{
		switch (gamestate)
		{
			case GS_LEVEL:
			case GS_INTERMISSION:
			{
				if (grandprixinfo.gp == true)
				{
					gs = DISCORD_GS_GRANDPRIX;
				}
				else if (modeattacking)
				{
					gs = DISCORD_GS_TIMEATTACK;
				}
				else if (gametype >= GT_FIRSTFREESLOT)
				{
					gs = DISCORD_GS_CUSTOM;
				}
				else
				{
					switch (gametype)
					{
						case GT_RACE:
						{
							gs = DISCORD_GS_RACE;
							break;
						}
						case GT_BATTLE:
						{
							gs = DISCORD_GS_BATTLE;
							break;
						}
						case GT_TUTORIAL:
						{
							gs = DISCORD_GS_TUTORIAL;
							break;
						}
						case GT_SPECIAL:
						case GT_VERSUS:
						{
							// When/if these are accessible outside of
							// Grand Prix or Time Attack, then these
							// should get their own images.
							// But right now, you're just using command line.
							// Just patch over it for now.
							gs = DISCORD_GS_GRANDPRIX;
							break;
						}
						default:
						{
							break; // leave as UNKNOWN...
						}
					}
				}
				break;
			}
			case GS_CEREMONY:
			{
				gs = DISCORD_GS_GRANDPRIX;
				break;
			}
			case GS_VOTING:
			{
				gs = DISCORD_GS_VOTING;
				break;
			}
			case GS_TITLESCREEN:
			case GS_INTRO:
			{
				gs = DISCORD_GS_TITLE;
				break;
			}
			case GS_CREDITS:
			case GS_EVALUATION:
			{
				gs = DISCORD_GS_CREDITS;
				break;
			}
			case GS_MENU:
			{
				if (menuactive && currentMenu == &EXTRAS_EggTVDef)
				{
					gs = DISCORD_GS_REPLAY;
					break;
				}
			}
			/* FALLTHRU */
			default:
			{
				gs = DISCORD_GS_MENU;
				break;
			}
		}
	}

	// Server info
	if (gs == DISCORD_GS_GONER)
	{
		if (Playing())
		{
			discordPresence.state = "TRAINING DATA";
		}
		else if (gamedata->gonerlevel >= GDGONER_OUTRO)
		{
			discordPresence.state = "EVALUATION";
		}
		else
		{
			discordPresence.state = "MISSING DATA";
		}
	}
	else if (netgame)
	{
		if (DRPC_InvitesAreAllowed() == true)
		{
			const char *join;

			// Grab the host's IP for joining.
			if ((join = DRPC_GetServerIP()) != NULL)
			{
				discordPresence.joinSecret = DRPC_XORIPString(join);
				joinSecretSet = true;
			}
			else
			{
				return;
			}
		}

		if (cv_advertise.value)
		{
			discordPresence.state = "Public";
		}
		else
		{
			discordPresence.state = "Private";
		}

		discordPresence.partyId = server_context; // Thanks, whoever gave us Mumble support, for implementing the EXACT thing Discord wanted for this field!
		discordPresence.partySize = D_NumPlayers(); // Players in server
		discordPresence.partyMax = discordInfo.maxPlayers; // Max players
	}
	else
	{
		// Reset discord info if you're not in a place that uses it!
		// Important for if you join a server that compiled without HAVE_DISCORDRPC,
		// so that you don't ever end up using bad information from another server.
		memset(&discordInfo, 0, sizeof(discordInfo));

		if (Playing())
		{
			snprintf(localstr, 128, "Local (%dP)", splitscreen + 1);
			discordPresence.state = localstr;
		}
		else
		{
			switch (gs)
			{
				case DISCORD_GS_REPLAY:
				{
					discordPresence.state = "Watching Replays";
					break;
				}
				case DISCORD_GS_TITLE:
				{
					discordPresence.state = "Title Screen";
					break;
				}
				case DISCORD_GS_CREDITS:
				{
					discordPresence.state = "Watching Credits";
					break;
				}
				default:
				{
					discordPresence.state = "Menu";
					break;
				}
			}
		}
	}

	if (gs == DISCORD_GS_GONER)
	{
		// Gametype info
		discordPresence.details = "Setup";

		discordPresence.largeImageKey = IMAGE_REPO "gs_goner" IMAGE_EXT;
		discordPresence.largeImageText = "NO SIGNAL";
	}
	else
	{
		// Gametype info
		if ((gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_VOTING || gamestate == GS_CEREMONY) && Playing())
		{
			if (grandprixinfo.gp)
			{
				char roundstr[32];

				if (gamestate == GS_CEREMONY)
				{
					snprintf(roundstr, 32, " | Ceremony");
				}
				else
				{
					switch (grandprixinfo.eventmode)
					{
						case GPEVENT_BONUS:
						{
							snprintf(roundstr, 32, " | Bonus");
							break;
						}
						case GPEVENT_SPECIAL:
						{
							snprintf(roundstr, 32, " | Special");
							break;
						}
						case GPEVENT_NONE:
						{
							if (roundqueue.position > 0 && roundqueue.position <= roundqueue.size)
							{
								snprintf(roundstr, 32, " | Round %d", roundqueue.position);
							}
							break;
						}
					}
				}

				snprintf(detailstr, 128, "Grand Prix%s | %s",
					roundstr,
					grandprixinfo.masterbots ? "Master" : gpdifficulty_cons_t[grandprixinfo.gamespeed].strvalue
				);
				discordPresence.details = detailstr;
			}
			else if (battleprisons == true)
			{
				discordPresence.details = "Prison Break";
			}
			else if (modeattacking)
			{
				if (modeattacking & ATTACKING_SPB)
				{
					discordPresence.details = "SPB Attack";
				}
				else
				{
					discordPresence.details = "Time Attack";
				}
			}
			else
			{
				snprintf(detailstr, 128, "%s%s%s",
					gametypes[gametype]->name,
					(gametypes[gametype]->speed == KARTSPEED_AUTO) ? va(" | %s", kartspeed_cons_t[gamespeed + 1].strvalue) : "",
					(encoremode == true) ? " | Encore" : ""
				);
				discordPresence.details = detailstr;
			}
		}

		if (gamestate == GS_LEVEL && Playing())
		{
			const time_t currentTime = time(NULL);
			const time_t mapTimeStart = currentTime - ((leveltime + starttime) / TICRATE);

			discordPresence.startTimestamp = mapTimeStart;

			if (timelimitintics > 0)
			{
				const time_t mapTimeEnd = mapTimeStart + ((timelimitintics + starttime + 1) / TICRATE);
				discordPresence.endTimestamp = mapTimeEnd;
			}
		}

		// Gametype image
		// Use these when there's no map image available!
		switch (gs)
		{
			case DISCORD_GS_CUSTOM:
			{
				discordPresence.largeImageKey = IMAGE_REPO "custom_gs" IMAGE_EXT;
				snprintf(largename, 128, "%s", gametypes[gametype]->name);
				discordPresence.largeImageText = largename;
				break;
			}
			case DISCORD_GS_RACE:
			{
				discordPresence.largeImageKey = IMAGE_REPO "gs_race" IMAGE_EXT;
				discordPresence.largeImageText = "Race";
				break;
			}
			case DISCORD_GS_BATTLE:
			{
				discordPresence.largeImageKey = IMAGE_REPO "gs_battle" IMAGE_EXT;
				discordPresence.largeImageText = "Battle";
				break;
			}
			case DISCORD_GS_TUTORIAL:
			{
				discordPresence.largeImageKey = IMAGE_REPO "gs_tutorial" IMAGE_EXT;
				discordPresence.largeImageText = "Tutorial";
				break;
			}
			case DISCORD_GS_TIMEATTACK:
			{
				discordPresence.largeImageKey = IMAGE_REPO "gs_timeattack" IMAGE_EXT;
				discordPresence.largeImageText = "Time Attack";
				break;
			}
			case DISCORD_GS_GRANDPRIX:
			{
				discordPresence.largeImageKey = IMAGE_REPO "gs_grandprix" IMAGE_EXT;
				discordPresence.largeImageText = "Grand Prix";
				break;
			}
			case DISCORD_GS_VOTING:
			{
				discordPresence.largeImageKey = IMAGE_REPO "gs_voting" IMAGE_EXT;
				discordPresence.largeImageText = "Voting";
				break;
			}
			case DISCORD_GS_MENU:
			{
				discordPresence.largeImageKey = IMAGE_REPO "gs_menu" IMAGE_EXT;
				discordPresence.largeImageText = "Menu";
				break;
			}
			case DISCORD_GS_REPLAY:
			{
				discordPresence.largeImageKey = IMAGE_REPO "gs_replay" IMAGE_EXT;
				discordPresence.largeImageText = "Watching Replays";
				break;
			}
			case DISCORD_GS_TITLE:
			{
				discordPresence.largeImageKey = IMAGE_REPO "gs_title" IMAGE_EXT;
				discordPresence.largeImageText = "Title Screen";
				break;
			}
			case DISCORD_GS_CREDITS:
			{
				discordPresence.largeImageKey = IMAGE_REPO "gs_credits" IMAGE_EXT;
				discordPresence.largeImageText = "Credits";
				break;
			}
			default:
			{
				discordPresence.largeImageKey = IMAGE_REPO "misc_develop" IMAGE_EXT;
				discordPresence.largeImageText = "Invalid DRPC state?";
				break;
			}
		}

		if ((gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_CEREMONY) && !demo.attract)
		{
			// Try a map image, if it exists!
			if (gamemap-1 < basenummapheaders)
			{
				snprintf(largeimg, 128, "%smap_%s%s", IMAGE_REPO, G_BuildMapName(gamemap), IMAGE_EXT);
				discordPresence.largeImageKey = largeimg; // Map image
			}

			// Map name on tool tip
			char *title = G_BuildMapTitle(gamemap);
			snprintf(largename, 128, "Map: %s", title);
			discordPresence.largeImageText = largename;
			Z_Free(title);
		}

		// Character info
		if (Playing() && playeringame[consoleplayer] && !players[consoleplayer].spectator)
		{
			// Character image
			if ((unsigned)players[consoleplayer].skin < g_discord_skins) // Supported skins
			{
				snprintf(charimg, 128, "%schar_%s%s", IMAGE_REPO, skins[ players[consoleplayer].skin ]->name, IMAGE_EXT);
				discordPresence.smallImageKey = charimg;
			}
			else
			{
				// Use the custom character icon!
				discordPresence.smallImageKey = IMAGE_REPO "custom_char" IMAGE_EXT;
			}

			snprintf(charname, 128, "Character: %s", skins[players[consoleplayer].skin]->realname);
			discordPresence.smallImageText = charname; // Character name
		}
	}
#endif // DISCORD_SECRETIVE

	if (joinSecretSet == false)
	{
		// Not able to join? Flush the request list, if it exists.
		DRPC_EmptyRequests();
	}

	Discord_UpdatePresence(&discordPresence);
	free(clientJoinSecret);
}

#endif // HAVE_DISCORDRPC
