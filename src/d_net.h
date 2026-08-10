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

#ifndef D_NET_H
#define D_NET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "doomdef.h"

// Max computers in a game
// 127 is probably as high as this can go, because
// int8_t is used for nodes sometimes >:(
#define MAXNETNODES 127
#define BROADCASTADDR MAXNETNODES
#define NETSPLITSCREEN // Kart's splitscreen netgame feature

#define STATLENGTH (TICRATE*2)

// stat of net
extern int32_t ticruned, ticmiss;
extern int32_t getbps, sendbps;
extern float lostpercent, duppercent, gamelostpercent;
extern int32_t packetheaderlength;
dboolean Net_GetNetStat(void);
extern int32_t getbytes;
extern int64_t sendbytes; // Realtime updated

#define PACKETMEASUREWINDOW (TICRATE*2)
extern dboolean packetloss[MAXPLAYERS][PACKETMEASUREWINDOW];

extern int8_t nodetoplayer[MAXNETNODES];
extern int8_t nodetoplayer2[MAXNETNODES]; // Say the numplayer for this node if any (splitscreen)
extern int8_t nodetoplayer3[MAXNETNODES]; // Say the numplayer for this node if any (splitscreen == 2)
extern int8_t nodetoplayer4[MAXNETNODES]; // Say the numplayer for this node if any (splitscreen == 3)
extern uint8_t playerpernode[MAXNETNODES]; // Used specially for splitscreen
extern dboolean nodeingame[MAXNETNODES]; // Set false as nodes leave game
extern dboolean nodeneedsauth[MAXNETNODES];

extern dboolean serverrunning;

int32_t Net_GetFreeAcks(dboolean urgent);
void Net_AckTicker(void);

// If reliable return true if packet sent, 0 else
dboolean HSendPacket(int32_t node, dboolean reliable, uint8_t acknum,
	size_t packetlength);
dboolean HGetPacket(void);
void D_SetDoomcom(void);
dboolean D_CheckNetGame(void);
void D_CloseConnection(void);
void Net_UnAcknowledgePacket(int32_t node);
void Net_CloseConnection(int32_t node);
void Net_ConnectionTimeout(int32_t node);
void Net_AbortPacketType(uint8_t packettype);
void Net_SendAcks(int32_t node);
void Net_WaitAllAckReceived(uint32_t timeout);

dboolean IsPacketSigned(int packettype);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
