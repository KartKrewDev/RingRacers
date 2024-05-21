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
/// \file  d_clisrv.h
/// \brief high level networking stuff

#ifndef __D_CLISRV__
#define __D_CLISRV__

#include "d_ticcmd.h"
#include "d_net.h"
#include "d_netcmd.h"
#include "d_net.h"
#include "tables.h"
#include "d_player.h"
#include "mserv.h"

#include "k_pwrlv.h" // PWRLV_NUMTYPES
#include "p_saveg.h" // NETSAVEGAMESIZE

#ifdef __cplusplus
extern "C" {
#endif

/*
The 'packet version' is used to distinguish packet formats.
This version is independent of VERSION and SUBVERSION. Different
applications may follow different packet versions.
*/
#define PACKETVERSION 0

// Network play related stuff.
// There is a data struct that stores network
//  communication related stuff, and another
//  one that defines the actual packets to
//  be transmitted.

#define HU_MAXMSGLEN 223

#define MAXSERVERNAME 32
#define MAXSERVERCONTACT 1024

// Networking and tick handling related.
#define BACKUPTICS 512 // more than enough for most timeouts....
#define CLIENTBACKUPTICS 32
#define MAXTEXTCMD 512

// No. of tics your controls can be delayed by.

// TODO: Instead of storing a ton of extra cmds for gentlemens' delay,
// keep them in a linked-list, with timestamps to discard everything that's older than already sent.
// That will support any amount of lag, and be less wasteful for clients who don't use it.
// This just works as a quick implementation.
#define MAXGENTLEMENDELAY TICRATE

//
// Packet structure
//
typedef enum
{
	PT_NOTHING,       // To send a nop through the network. ^_~
	PT_SERVERCFG,     // Server config used in start game
	                  // (must stay 1 for backwards compatibility).
	                  // This is a positive response to a CLIENTJOIN request.
	PT_CLIENTCMD,     // Ticcmd of the client.
	PT_CLIENTMIS,     // Same as above with but saying resend from.
	PT_CLIENT2CMD,    // 2 cmds in the packet for splitscreen.
	PT_CLIENT2MIS,    // Same as above with but saying resend from
	PT_NODEKEEPALIVE, // Same but without ticcmd and consistancy
	PT_NODEKEEPALIVEMIS,
	PT_SERVERTICS,    // All cmds for the tic.
	PT_SERVERREFUSE,  // Server refuses joiner (reason inside).
	PT_SERVERSHUTDOWN,
	PT_CLIENTQUIT,    // Client closes the connection.

	PT_ASKINFO,       // Anyone can ask info of the server.
	PT_SERVERINFO,    // Send game & server info (gamespy).
	PT_PLAYERINFO,    // Send information for players in game (gamespy).
	PT_REQUESTFILE,   // Client requests a file transfer
	PT_ASKINFOVIAMS,  // Packet from the MS requesting info be sent to new client.
	                  // If this ID changes, update masterserver definition.

	PT_WILLRESENDGAMESTATE, // Hey Client, I am about to resend you the gamestate!
	PT_CANRECEIVEGAMESTATE, // Okay Server, I'm ready to receive it, you can go ahead.
	PT_RECEIVEDGAMESTATE,   // Thank you Server, I am ready to play again!

	PT_SENDINGLUAFILE, // Server telling a client Lua needs to open a file
	PT_ASKLUAFILE,     // Client telling the server they don't have the file
	PT_HASLUAFILE,     // Client telling the server they have the file

	// Add non-PT_CANFAIL packet types here to avoid breaking MS compatibility.

	// Kart-specific packets
	PT_CLIENT3CMD,    // 3P
	PT_CLIENT3MIS,
	PT_CLIENT4CMD,    // 4P
	PT_CLIENT4MIS,
	PT_BASICKEEPALIVE,// Keep the network alive during wipes, as tics aren't advanced and NetUpdate isn't called

	PT_CANFAIL,       // This is kind of a priority. Anything bigger than CANFAIL
	                  // allows HSendPacket(*, true, *, *) to return false.
	                  // In addition, this packet can't occupy all the available slots.

	PT_FILEFRAGMENT = PT_CANFAIL, // A part of a file.
	PT_FILEACK,
	PT_FILERECEIVED,

	PT_TEXTCMD,       // Extra text commands from the client.
	PT_TEXTCMD2,      // Splitscreen text commands.
	PT_TEXTCMD3,      // 3P
	PT_TEXTCMD4,      // 4P
	PT_CLIENTJOIN,    // Client wants to join; used in start game.
	PT_NODETIMEOUT,   // Packet sent to self if the connection times out.

	PT_TELLFILESNEEDED, // Client, to server: "what other files do I need starting from this number?"
	PT_MOREFILESNEEDED, // Server, to client: "you need these (+ more on top of those)"

	PT_LOGIN,         // Login attempt from the client.

	PT_PING,          // Packet sent to tell clients the other client's latency to server.

	PT_CLIENTKEY,		// "Here's my public key"
	PT_SERVERCHALLENGE,		// "Prove it"

	PT_CHALLENGEALL,	// Prove to the other clients you are who you say you are, sign this random bullshit!
	PT_RESPONSEALL,		// OK, here is my signature on that random bullshit
	PT_RESULTSALL,		// Here's what everyone responded to PT_CHALLENGEALL with, if this is wrong or you don't receive it disconnect

	PT_SAY,				// "Hey server, please send this chat message to everyone via XD_SAY"

	NUMPACKETTYPE
} packettype_t;

typedef enum
{
	SIGN_OK,
	SIGN_BADTIME, // Timestamp differs by too much, suspect reuse of an old challenge.
	SIGN_BADIP // Asked to sign the wrong IP by an external host, suspect reuse of another server's challenge.
} shouldsign_t;

#ifdef PACKETDROP
void Command_Drop(void);
void Command_Droprate(void);
#endif
void Command_Numnodes(void);

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// Client to server packet
struct clientcmd_pak
{
	UINT8 client_tic;
	UINT8 resendfrom;
	INT16 consistancy;
	ticcmd_t cmd;
} ATTRPACK;

// Splitscreen packet
// WARNING: must have the same format of clientcmd_pak, for more easy use
struct client2cmd_pak
{
	UINT8 client_tic;
	UINT8 resendfrom;
	INT16 consistancy;
	ticcmd_t cmd, cmd2;
} ATTRPACK;

// 3P Splitscreen packet
// WARNING: must have the same format of clientcmd_pak, for more easy use
struct client3cmd_pak
{
	UINT8 client_tic;
	UINT8 resendfrom;
	INT16 consistancy;
	ticcmd_t cmd, cmd2, cmd3;
} ATTRPACK;

// 4P Splitscreen packet
// WARNING: must have the same format of clientcmd_pak, for more easy use
struct client4cmd_pak
{
	UINT8 client_tic;
	UINT8 resendfrom;
	INT16 consistancy;
	ticcmd_t cmd, cmd2, cmd3, cmd4;
} ATTRPACK;

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif

// Server to client packet
// this packet is too large
struct servertics_pak
{
	UINT8 starttic;
	UINT8 numtics;
	UINT8 numslots; // "Slots filled": Highest player number in use plus one.
	ticcmd_t cmds[45]; // Normally [BACKUPTIC][MAXPLAYERS] but too large
} ATTRPACK;

struct serverconfig_pak
{
	UINT8 version; // Different versions don't work
	UINT8 subversion; // Contains build version

	// Server launch stuffs
	UINT8 serverplayer;
	UINT8 totalslotnum; // "Slots": highest player number in use plus one.

	tic_t gametic;
	UINT8 clientnode;
	UINT8 gamestate;

	UINT8 gametype;
	UINT8 modifiedgame;

	char server_context[8]; // Unique context id, generated at server startup.

	// Discord info (always defined for net compatibility)
	UINT8 maxplayer;
	boolean allownewplayer;
	boolean discordinvites;

	char server_name[MAXSERVERNAME];
	char server_contact[MAXSERVERCONTACT];
} ATTRPACK;

struct filetx_pak
{
	UINT8 fileid;
	UINT32 filesize;
	UINT8 iteration;
	UINT32 position;
	UINT16 size;
	UINT8 data[]; // Size is variable using hardware_MAXPACKETLENGTH
} ATTRPACK;

struct fileacksegment_t
{
	UINT32 start;
	UINT32 acks;
} ATTRPACK;

struct fileack_pak
{
	UINT8 fileid;
	UINT8 iteration;
	UINT8 numsegments;
	fileacksegment_t segments[];
} ATTRPACK;

#ifdef _MSC_VER
#pragma warning(default : 4200)
#endif

#define MAXAPPLICATION 16

struct clientconfig_pak
{
	UINT8 _255;/* see serverinfo_pak */
	UINT8 packetversion;
	char application[MAXAPPLICATION];
	UINT8 version; // Different versions don't work
	UINT8 subversion; // Contains build version
	UINT8 localplayers;	// number of splitscreen players
	UINT8 mode;
	char names[MAXSPLITSCREENPLAYERS][MAXPLAYERNAME];
	UINT8 availabilities[MAXAVAILABILITY];
	uint8_t challengeResponse[MAXSPLITSCREENPLAYERS][SIGNATURELENGTH];
} ATTRPACK;

#define SV_SPEEDMASK 0x03		// used to send kartspeed
#define SV_DEDICATED 0x40		// server is dedicated
#define SV_LOTSOFADDONS 0x20	// flag used to ask for full file list in d_netfil

#define MAXFILENEEDED 915
#define MAX_MIRROR_LENGTH 256
// This packet is too large
struct serverinfo_pak
{
	/*
	In the old packet, 'version' is the first field. Now that field is set
	to 255 always, so older versions won't be confused with the new
	versions or vice-versa.
	*/
	UINT8 _255;
	UINT8 packetversion;
	char  application[MAXAPPLICATION];
	UINT8 version;
	UINT8 subversion;
	UINT8 commit[GIT_SHA_ABBREV];
	UINT8 numberofplayer;
	UINT8 maxplayer;
	UINT8 refusereason; // 0: joinable, 1: joins disabled, 2: full
	char gametypename[24];
	UINT8 modifiedgame;
	UINT8 cheatsenabled;
	UINT8 kartvars; // Previously isdedicated, now appropriated for our own nefarious purposes
	UINT8 fileneedednum;
	tic_t time;
	tic_t leveltime;
	char servername[MAXSERVERNAME];
	char maptitle[33];
	unsigned char mapmd5[16];
	UINT8 actnum;
	UINT8 iszone;
	char httpsource[MAX_MIRROR_LENGTH]; // HTTP URL to download from, always defined for compatibility
	INT16 avgpwrlv; // Kart avg power level
	UINT8 fileneeded[MAXFILENEEDED]; // is filled with writexxx (byteptr.h)
} ATTRPACK;

struct serverrefuse_pak
{
	char reason[255];
} ATTRPACK;

struct askinfo_pak
{
	UINT8 version;
	tic_t time; // used for ping evaluation
} ATTRPACK;

struct msaskinfo_pak
{
	char clientaddr[22];
	tic_t time; // used for ping evaluation
} ATTRPACK;

// Shorter player information for external use.
struct plrinfo
{
	UINT8 num;
	char name[MAXPLAYERNAME+1];
	UINT8 address[4]; // sending another string would run us up against MAXPACKETLENGTH
	UINT8 team;
	UINT8 skin;
	UINT8 data; // Color is first four bits, hasflag, isit and issuper have one bit each, the last is unused.
	UINT32 score;
	UINT16 timeinserver; // In seconds.
} ATTRPACK;

// Shortest player information for join during intermission.
struct plrconfig
{
	char name[MAXPLAYERNAME+1];
	UINT8 skin;
	UINT16 color;
	UINT32 pflags;
	UINT32 score;
	UINT8 ctfteam;
} ATTRPACK;

struct filesneededconfig_pak
{
	INT32 first;
	UINT8 num;
	UINT8 more;
	UINT8 files[MAXFILENEEDED]; // is filled with writexxx (byteptr.h)
} ATTRPACK;

struct clientkey_pak
{
	uint8_t key[MAXSPLITSCREENPLAYERS][PUBKEYLENGTH];
} ATTRPACK;

struct serverchallenge_pak
{
	uint8_t secret[CHALLENGELENGTH];
} ATTRPACK;

struct challengeall_pak
{
	uint8_t secret[CHALLENGELENGTH];
} ATTRPACK;

struct responseall_pak
{
	uint8_t signature[MAXSPLITSCREENPLAYERS][SIGNATURELENGTH];
} ATTRPACK;

struct resultsall_pak
{
	uint8_t signature[MAXPLAYERS][SIGNATURELENGTH];
} ATTRPACK;

struct say_pak
{
	char message[HU_MAXMSGLEN];
	UINT8 target;
	UINT8 flags;
	UINT8 source;
} ATTRPACK;

struct netinfo_pak
{
	UINT32 pingtable[MAXPLAYERS+1];
	UINT32 packetloss[MAXPLAYERS+1];
	UINT32 delay[MAXPLAYERS+1];
} ATTRPACK;

//
// Network packet data
//
struct doomdata_t
{
	UINT32 checksum;
	UINT8 ack; // If not zero the node asks for acknowledgement, the receiver must resend the ack
	UINT8 ackreturn; // The return of the ack number

	UINT8 packettype;
#ifdef SIGNGAMETRAFFIC
	uint8_t signature[MAXSPLITSCREENPLAYERS][SIGNATURELENGTH];
#endif
	UINT8 reserved; // Padding
	union
	{
		clientcmd_pak clientpak;            //         147 bytes
		client2cmd_pak client2pak;          //         206 bytes
		client3cmd_pak client3pak;          //         264 bytes(?)
		client4cmd_pak client4pak;          //         324 bytes(?)
		servertics_pak serverpak;           //      132495 bytes (more around 360, no?)
		serverconfig_pak servercfg;         //         773 bytes
		UINT8 textcmd[MAXTEXTCMD+2];        //       66049 bytes (wut??? 64k??? More like 258 bytes...)
		char filetxpak[sizeof (filetx_pak)];//         139 bytes
		char fileack[sizeof (fileack_pak)];
		UINT8 filereceived;
		clientconfig_pak clientcfg;         //         136 bytes
		UINT8 md5sum[16];
		serverinfo_pak serverinfo;          //        1024 bytes
		serverrefuse_pak serverrefuse;      //       65025 bytes (somehow I feel like those values are garbage...)
		askinfo_pak askinfo;                //          61 bytes
		msaskinfo_pak msaskinfo;            //          22 bytes
		plrinfo playerinfo[MSCOMPAT_MAXPLAYERS];//         576 bytes(?)
		plrconfig playerconfig[MAXPLAYERS]; // (up to) 528 bytes(?)
		INT32 filesneedednum;               //           4 bytes
		filesneededconfig_pak filesneededcfg; //       ??? bytes
		netinfo_pak netinfo;					// Don't believe their lies
		clientkey_pak clientkey;				// 32 bytes
		serverchallenge_pak serverchallenge;	// 256 bytes
		challengeall_pak challengeall;			// 256 bytes
		responseall_pak responseall;			// 256 bytes
		resultsall_pak resultsall;				// 1024 bytes. Also, you really shouldn't trust anything here.
		say_pak say;							// I don't care anymore.
	} u; // This is needed to pack diff packet types data together
} ATTRPACK;

#if defined(_MSC_VER)
#pragma pack()
#endif

#define MAXSERVERLIST (MAXNETNODES-1)
#define GTCALC_RACE 0
#define GTCALC_BATTLE 1
#define GTCALC_CUSTOM 2
struct serverelem_t
{
	SINT8 node;
	serverinfo_pak info;
	UINT8 cachedgtcalc;
};

extern serverelem_t serverlist[MAXSERVERLIST];
extern UINT32 serverlistcount, serverlistultimatecount;
extern INT32 mapchangepending;

// Points inside doomcom
extern doomdata_t *netbuffer;
extern consvar_t cv_stunserver;
extern consvar_t cv_httpsource;
extern consvar_t cv_kicktime;

extern consvar_t cv_showjoinaddress;
extern consvar_t cv_playbackspeed;

#define BASEPACKETSIZE      offsetof(doomdata_t, u)
#define FILETXHEADER        offsetof(filetx_pak, data)
#define BASESERVERTICSSIZE  offsetof(doomdata_t, u.serverpak.cmds[0])

typedef enum
{
	KICK_MSG_PLAYER_QUIT = 0,	// Player intentionally left
	KICK_MSG_KICKED,			// Server kick message w/ no reason
	KICK_MSG_CUSTOM_KICK,		// Server kick message w/ reason
	KICK_MSG_VOTE_KICK,			// Vote kick message
	KICK_MSG_BANNED,			// Ban message w/ no reason
	KICK_MSG_CUSTOM_BAN,		// Ban message w/ custom reason
	KICK_MSG_TIMEOUT,			// Player's connection timed out
	KICK_MSG_PING_HIGH,			// Player hit the ping limit
	KICK_MSG_GRIEF,				// Player was detected by antigrief
	KICK_MSG_CON_FAIL,			// Player failed to resync game state
	KICK_MSG_SIGFAIL,			// Player failed signature check
	KICK_MSG__MAX				// Number of unique messages
} kickmsg_t;

typedef enum
{
	KR_KICK          = 1, //Kicked by server
	KR_PINGLIMIT     = 2, //Broke Ping Limit
	KR_SYNCH         = 3, //Synch Failure
	KR_TIMEOUT       = 4, //Connection Timeout
	KR_BAN           = 5, //Banned by server
	KR_LEAVE         = 6, //Quit the game
} kickreason_t;

/* the max number of name changes in some time period */
#define MAXNAMECHANGES (5)
#define NAMECHANGERATE (60*TICRATE)

extern boolean server;
extern boolean serverrunning;
#define client (!server)
extern boolean dedicated; // For dedicated server
extern UINT16 software_MAXPACKETLENGTH;
extern boolean acceptnewnode;
extern SINT8 servernode;
extern char connectedservername[MAXSERVERNAME];
extern char connectedservercontact[MAXSERVERCONTACT];
extern UINT32 ourIP;
extern uint8_t lastReceivedKey[MAXNETNODES][MAXSPLITSCREENPLAYERS][PUBKEYLENGTH];
extern uint8_t lastSentChallenge[MAXNETNODES][CHALLENGELENGTH];
extern uint8_t lastChallengeAll[CHALLENGELENGTH];
extern uint8_t lastReceivedSignature[MAXPLAYERS][SIGNATURELENGTH];
extern uint8_t knownWhenChallenged[MAXPLAYERS][PUBKEYLENGTH];
extern boolean expectChallenge;

// We give clients a chance to verify each other once per race.
// When is that challenge sent, and when should clients bail if they don't receive the responses?
#define CHALLENGEALL_START (TICRATE*5) // Server sends challenges here.
#define CHALLENGEALL_KICKUNRESPONSIVE (TICRATE*10) // Server kicks players that haven't submitted signatures here.
#define CHALLENGEALL_SENDRESULTS (TICRATE*15) // Server waits for kicks to process until here. (Failing players shouldn't be in-game when results are received, or clients get spooked.)
#define CHALLENGEALL_CLIENTCUTOFF (TICRATE*20) // If challenge process hasn't completed by now, clients who were in-game for CHALLENGEALL_START should leave.

void Command_Ping_f(void);
extern tic_t connectiontimeout;
extern tic_t jointimeout;
extern UINT16 pingmeasurecount;
extern UINT32 realpingtable[MAXPLAYERS];
extern UINT32 playerpingtable[MAXPLAYERS];
extern UINT32 playerpacketlosstable[MAXPLAYERS];
extern UINT32 playerdelaytable[MAXPLAYERS];
extern tic_t servermaxping;

extern boolean server_lagless;
extern consvar_t cv_mindelay;

extern consvar_t cv_netticbuffer, cv_allownewplayer, cv_maxconnections, cv_joindelay;
extern consvar_t cv_pingtimeout, cv_resynchattempts, cv_blamecfail;
extern consvar_t cv_maxsend, cv_noticedownload, cv_downloadspeed;

#ifdef VANILLAJOINNEXTROUND
extern consvar_t cv_joinnextround;
#endif

extern consvar_t cv_discordinvites;

extern consvar_t cv_allowguests;

extern consvar_t cv_gamestochat;

#ifdef DEVELOP
	extern consvar_t cv_badjoin;
	extern consvar_t cv_badtraffic;
	extern consvar_t cv_badresponse;
	extern consvar_t cv_noresponse;
	extern consvar_t cv_nochallenge;
	extern consvar_t cv_badresults;
	extern consvar_t cv_noresults;
	extern consvar_t cv_badtime;
	extern consvar_t cv_badip;
#endif

// Used in d_net, the only dependence
tic_t ExpandTics(INT32 low, tic_t basetic);
void D_ClientServerInit(void);

void GenerateChallenge(uint8_t *buf);
shouldsign_t ShouldSignChallenge(uint8_t *message);

// Initialise the other field
void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(const UINT8 **p, INT32 playernum));
void SendNetXCmdForPlayer(UINT8 playerid, netxcmd_t id, const void *param, size_t nparam);
#define SendNetXCmd(id, param, nparam) SendNetXCmdForPlayer(0, id, param, nparam) // Shortcut for P1
void SendKick(UINT8 playernum, UINT8 msg);

// Create any new ticcmds and broadcast to other players.
void NetKeepAlive(void);
void NetUpdate(void);

void SV_StartSinglePlayerServer(INT32 dogametype, boolean donetgame);
boolean SV_SpawnServer(void);
void SV_StopServer(void);
void SV_ResetServer(void);

/*--------------------------------------------------
	boolean K_AddBotFromServer(UINT8 skin, UINT8 difficulty, botStyle_e style, UINT8 *newplayernum);

		Adds a new bot, using a server-sided packet sent to all clients.
		Using regular K_AddBot wherever possible is better, but this is kept
		as a back-up measure if this is the only option.

	Input Arguments:-
		skin - Skin number that the bot will use.
		difficulty - Difficulty level this bot will use.
		style - Bot style to spawn this bot with, see botStyle_e.
		newplayernum - Pointer to the last valid player slot number.
			Is a pointer so that this function can be called multiple times to add more than one bot.

	Return:-
		true if a bot can be added via a packet later, otherwise false.
--------------------------------------------------*/

boolean K_AddBotFromServer(UINT8 skin, UINT8 difficulty, botStyle_e style, UINT8 *p);

void CL_AddSplitscreenPlayer(void);
void CL_RemoveSplitscreenPlayer(UINT8 p);
void CL_Reset(void);
void CL_ClearPlayer(INT32 playernum);
void CL_RemovePlayer(INT32 playernum, kickreason_t reason);
void CL_QueryServerList(msg_server_t *list);
void CL_UpdateServerList(void);
void CL_TimeoutServerList(void);
// Is there a game running
boolean Playing(void);

// Advance client-to-client pubkey verification flow
void UpdateChallenges(void);

// Broadcasts special packets to other players
//  to notify of game exit
void D_QuitNetGame(void);

//? How many ticks to run?
boolean TryRunTics(tic_t realtic);

// extra data for lmps
// these functions scare me. they contain magic.
/*boolean AddLmpExtradata(UINT8 **demo_p, INT32 playernum);
void ReadLmpExtraData(UINT8 **demo_pointer, INT32 playernum);*/

// translate a playername in a player number return -1 if not found and
// print a error message in the console
SINT8 nametonum(const char *name);

extern char motd[254], server_context[8];
extern UINT8 playernode[MAXPLAYERS];
/* consoleplayer of this player (splitscreen) */
extern UINT8 playerconsole[MAXPLAYERS];

INT32 D_NumPlayers(void);
boolean D_IsPlayerHumanAndGaming(INT32 player_number);

void D_ResetTiccmds(void);
void D_ResetTiccmdAngle(UINT8 ss, angle_t angle);
ticcmd_t *D_LocalTiccmd(UINT8 ss);

tic_t GetLag(INT32 node);
UINT8 GetFreeXCmdSize(UINT8 playerid);

void D_MD5PasswordPass(const UINT8 *buffer, size_t len, const char *salt, void *dest);

extern UINT8 hu_redownloadinggamestate;

extern UINT8 adminpassmd5[16];
extern boolean adminpasswordset;

extern boolean hu_stopped;

//
// SRB2Kart
//

struct rewind_t {
	UINT8 savebuffer[NETSAVEGAMESIZE];
	tic_t leveltime;
	size_t demopos;

	ticcmd_t oldcmd[MAXPLAYERS];
	mobj_t oldghost[MAXPLAYERS];

	rewind_t *next;
};

void CL_ClearRewinds(void);
rewind_t *CL_SaveRewindPoint(size_t demopos);
rewind_t *CL_RewindToTime(tic_t time);

void HandleSigfail(const char *string);

void DoSayPacket(SINT8 target, UINT8 flags, UINT8 source, char *message);
void DoSayPacketFromCommand(SINT8 target, size_t usedargs, UINT8 flags);
void SendServerNotice(SINT8 target, char *message);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
