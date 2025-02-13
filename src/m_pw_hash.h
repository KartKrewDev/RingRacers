// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef m_pw_hash_H
#define m_pw_hash_H

#include "doomtype.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_PW_HASH_SIZE (64)
#define M_PW_SALT_SIZE (16)
#define M_PW_BUF_SIZE M_PW_HASH_SIZE

UINT8 *M_HashPassword(UINT8 hash[M_PW_HASH_SIZE], const char *key, const UINT8 salt[M_PW_SALT_SIZE]);

#ifdef __cplusplus
} // extern "C"
#endif

#endif/*m_pw_hash_H*/
