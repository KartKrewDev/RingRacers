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
/// \file  i_tcp.h
/// \brief TCP driver, sockets.

#ifndef __I_TCP__
#define __I_TCP__

#ifdef __cplusplus
extern "C" {
#endif

#include "d_net.h"

extern UINT16 current_port;

/**	\brief	The I_InitTcpNetwork function

	\return	true if going or in a netgame, else it's false
*/
boolean I_InitTcpNetwork(void);

/**	\brief	The I_InitTcpNetwork function

	\return	true if TCP/IP stack was initilized, else it's false
*/
boolean I_InitTcpDriver(void);
void I_ShutdownTcpDriver(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
