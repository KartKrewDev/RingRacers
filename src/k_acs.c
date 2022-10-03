// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_acs.c
/// \brief Action Code Script implementation using ACSVM

#include "k_acs.h"

#include "doomtype.h"
#include "doomdef.h"

#include "CAPI/BinaryIO.h"
#include "CAPI/Environment.h"
#include "CAPI/Module.h"
#include "CAPI/PrintBuf.h"
#include "CAPI/Scope.h"
#include "CAPI/String.h"
#include "CAPI/Thread.h"

static tic_t ExecTime = 0;

boolean K_ACS_EndPrint(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	(void)thread;
	(void)argV;
	(void)argC;

/*
	ACSVM_PrintBuf *buf = ACSVM_Thread_GetPrintBuf(thread);

	CONS_Printf("%s\n", ACSVM_PrintBuf_GetData(buf));
	ACSVM_PrintBuf_Drop(buf);
*/

	return false;
}

boolean K_ACS_Timer(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	(void)argV;
	(void)argC;

	ACSVM_Thread_DataStk_Push(thread, ExecTime);
	return false;
}
