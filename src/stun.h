// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  stun.h
/// \brief RFC 5389 client implementation to fetch external IP address.

#ifndef KART_STUN_H
#define KART_STUN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*stun_callback_t)(UINT32 address);

void    csprng (void * const buffer, const size_t size);

void    STUN_bind (stun_callback_t);
boolean STUN_got_response (const char * const buffer, const size_t size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif/*KART_STUN_H*/
