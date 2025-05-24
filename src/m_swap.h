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
/// \file  m_swap.h
/// \brief Endianess handling, swapping 16bit and 32bit

#ifndef __M_SWAP__
#define __M_SWAP__

#include "endian.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SWAP_SHORT(x) ((INT16)(\
(((UINT16)(x) & (UINT16)0x00ffU) << 8) \
| \
(((UINT16)(x) & (UINT16)0xff00U) >> 8))) \

#define SWAP_LONG(x) ((INT32)(\
(((UINT32)(x) & (UINT32)0x000000ffUL) << 24) \
| \
(((UINT32)(x) & (UINT32)0x0000ff00UL) <<  8) \
| \
(((UINT32)(x) & (UINT32)0x00ff0000UL) >>  8) \
| \
(((UINT32)(x) & (UINT32)0xff000000UL) >> 24)))

#define SWAP_LONGLONG(x) ((INT64)(\
(((UINT64)(x) & (UINT64)0x00000000000000ffULL) << 56) \
| \
(((UINT64)(x) & (UINT64)0x000000000000ff00ULL) << 40) \
| \
(((UINT64)(x) & (UINT64)0x0000000000ff0000ULL) << 24) \
| \
(((UINT64)(x) & (UINT64)0x00000000ff000000ULL) <<  8) \
| \
(((UINT64)(x) & (UINT64)0x000000ff00000000ULL) >>  8) \
| \
(((UINT64)(x) & (UINT64)0x0000ff0000000000ULL) >> 24) \
| \
(((UINT64)(x) & (UINT64)0x00ff000000000000ULL) >> 40) \
| \
(((UINT64)(x) & (UINT64)0xff00000000000000ULL) >> 56)))

// Endianess handling.
// WAD files are stored little endian.
#ifdef SRB2_BIG_ENDIAN
#define SHORT SWAP_SHORT
#define LONG SWAP_LONG
#define LONGLON SWAP_LONGLONG
#define MSBF_SHORT(x) ((INT16)(x))
#define MSBF_LONG(x) ((INT32)(x))
#define MSBF_LONGLONG(x) ((INT64)(x))
#else
#define SHORT(x) ((INT16)(x))
#define LONG(x)	((INT32)(x))
#define LONGLONG(x) ((INT64)(x))
#define MSBF_SHORT SWAP_SHORT
#define MSBF_LONG SWAP_LONG
#define MSBF_LONGLONG SWAP_LONGLONG
#endif

// Big to little endian
#ifdef SRB2_LITTLE_ENDIAN
	#define BIGENDIAN_LONG(x) ((INT32)(((x)>>24)&0xff)|(((x)<<8)&0xff0000)|(((x)>>8)&0xff00)|(((x)<<24)&0xff000000))
	#define BIGENDIAN_SHORT(x) ((INT16)(((x)>>8)|((x)<<8)))
#else
	#define BIGENDIAN_LONG(x) ((INT32)(x))
	#define BIGENDIAN_SHORT(x) ((INT16)(x))
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
