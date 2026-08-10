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

#ifndef M_SWAP_H
#define M_SWAP_H

#include "endian.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SWAP_SHORT(x) ((int16_t)(\
(((uint16_t)(x) & (uint16_t)0x00ffU) << 8) \
| \
(((uint16_t)(x) & (uint16_t)0xff00U) >> 8))) \

#define SWAP_LONG(x) ((int32_t)(\
(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) \
| \
(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) \
| \
(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) \
| \
(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24)))

#define SWAP_LONGLONG(x) ((int64_t)(\
(((uint64_t)(x) & (uint64_t)0x00000000000000ffULL) << 56) \
| \
(((uint64_t)(x) & (uint64_t)0x000000000000ff00ULL) << 40) \
| \
(((uint64_t)(x) & (uint64_t)0x0000000000ff0000ULL) << 24) \
| \
(((uint64_t)(x) & (uint64_t)0x00000000ff000000ULL) <<  8) \
| \
(((uint64_t)(x) & (uint64_t)0x000000ff00000000ULL) >>  8) \
| \
(((uint64_t)(x) & (uint64_t)0x0000ff0000000000ULL) >> 24) \
| \
(((uint64_t)(x) & (uint64_t)0x00ff000000000000ULL) >> 40) \
| \
(((uint64_t)(x) & (uint64_t)0xff00000000000000ULL) >> 56)))

// Endianess handling.
// WAD files are stored little endian.
#ifdef SRB2_BIG_ENDIAN
#define LSBF_SHORT SWAP_SHORT
#define LSBF_LONG SWAP_LONG
#define LSBF_LONGLONG SWAP_LONGLONG
#define MSBF_SHORT(x) ((int16_t)(x))
#define MSBF_LONG(x) ((int32_t)(x))
#define MSBF_LONGLONG(x) ((int64_t)(x))
#else
#define LSBF_SHORT(x) ((int16_t)(x))
#define LSBF_LONG(x)	((int32_t)(x))
#define LSBF_LONGLONG(x) ((int64_t)(x))
#define MSBF_SHORT SWAP_SHORT
#define MSBF_LONG SWAP_LONG
#define MSBF_LONGLONG SWAP_LONGLONG
#endif

// Big to little endian
#ifdef SRB2_LITTLE_ENDIAN
	#define BIGENDIAN_LONG(x) ((int32_t)(((x)>>24)&0xff)|(((x)<<8)&0xff0000)|(((x)>>8)&0xff00)|(((x)<<24)&0xff000000))
	#define BIGENDIAN_SHORT(x) ((int16_t)(((x)>>8)|((x)<<8)))
#else
	#define BIGENDIAN_LONG(x) ((int32_t)(x))
	#define BIGENDIAN_SHORT(x) ((int16_t)(x))
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
