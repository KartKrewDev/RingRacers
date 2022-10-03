// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_acs.h
/// \brief Action Code Script implementation using ACSVM

#ifndef __K_ACS__
#define __K_ACS__

#include "doomtype.h"
#include "doomdef.h"

#include "CAPI/BinaryIO.h"
#include "CAPI/Environment.h"
#include "CAPI/Module.h"
#include "CAPI/PrintBuf.h"
#include "CAPI/Scope.h"
#include "CAPI/String.h"
#include "CAPI/Thread.h"

boolean K_ACS_EndPrint(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);
boolean K_ACS_Timer(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC);

#endif // __K_ACS__
