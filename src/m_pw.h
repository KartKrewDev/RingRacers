// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef m_pw_H
#define m_pw_H

#include "doomtype.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	M_PW_INVALID,
	M_PW_EXTRAS,
	M_PW_CHALLENGES,
}
try_password_e;

void M_PasswordInit(void);
try_password_e M_TryPassword(const char *password, boolean challenges);
boolean M_TryExactPassword(const char *password, const char *encodedhash);

#ifdef __cplusplus
} // extern "C"
#endif

#endif/*m_pw_H*/
