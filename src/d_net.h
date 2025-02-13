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
/// \file  d_net.h
/// \brief part of layer 4 (transport) (tp4) of the osi model
///        assure the reception of packet and proceed a checksums
///
///        There is a data struct that stores network communication related
///        stuff, and one that defines the actual packets to be transmitted

#ifndef __D_NET__
#define __D_NET__

#ifdef __cplusplus
extern "C" {
#endif

#include "doomdef.h"

// Max computers in a game
// 127 is probably as high as this can go, because
// SINT8 is used for nodes sometimes >:(
#define MAXNETNODES 127
#define BROADCASTADDR MAXNETNODES
#define NETSPLITSCREEN // Kart's splitscreen netgame feature

#define STATLENGTH (TICRATE*2)

// stat of net
extern INT32 ticruned, ticmiss;
extern INT32 getbps, sendbps;
extern float lostpercent, duppercent, gamelostpercent;
extern INT32 packetheaderlength;
boolean Net_GetNetStat(void);
extern INT32 getbytes;
extern INT64 sendbytes; // Realtime updated

#define PACKETMEASUREWINDOW (TICRATE*2)
extern boolean packetloss[MAXPLAYERS][PACKETMEASUREWINDOW];

extern SINT8 nodetoplayer[MAXNETNODES];
extern SINT8 nodetoplayer2[MAXNETNODES]; // Say the numplayer for this node if any (splitscreen)
extern SINT8 nodetoplayer3[MAXNETNODES]; // Say the numplayer for this node if any (splitscreen == 2)
extern SINT8 nodetoplayer4[MAXNETNODES]; // Say the numplayer for this node if any (splitscreen == 3)
extern UINT8 playerpernode[MAXNETNODES]; // Used specially for splitscreen
extern boolean nodeingame[MAXNETNODES]; // Set false as nodes leave game
extern boolean nodeneedsauth[MAXNETNODES];

extern boolean serverrunning;

INT32 Net_GetFreeAcks(boolean urgent);
void Net_AckTicker(void);

// If reliable return true if packet sent, 0 else
boolean HSendPacket(INT32 node, boolean reliable, UINT8 acknum,
	size_t packetlength);
boolean HGetPacket(void);
void D_SetDoomcom(void);
boolean D_CheckNetGame(void);
void D_CloseConnection(void);
void Net_UnAcknowledgePacket(INT32 node);
void Net_CloseConnection(INT32 node);
void Net_ConnectionTimeout(INT32 node);
void Net_AbortPacketType(UINT8 packettype);
void Net_SendAcks(INT32 node);
void Net_WaitAllAckReceived(UINT32 timeout);

boolean IsPacketSigned(int packettype);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
