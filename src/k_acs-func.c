// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_acs-func.c
/// \brief ACS CallFunc definitions

#include "k_acs.h"

#include "doomtype.h"
#include "doomdef.h"
#include "d_think.h"
#include "p_mobj.h"
#include "p_tick.h"
#include "w_wad.h"

#include "CAPI/BinaryIO.h"
#include "CAPI/Environment.h"
#include "CAPI/Module.h"
#include "CAPI/PrintBuf.h"
#include "CAPI/Scope.h"
#include "CAPI/String.h"
#include "CAPI/Thread.h"

bool ACS_CF_EndPrint(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	ACSVM_PrintBuf *buf = NULL;

	(void)argV;
	(void)argC;

	buf = ACSVM_Thread_GetPrintBuf(thread);
	CONS_Printf("%s\n", ACSVM_PrintBuf_GetData(buf));
	ACSVM_PrintBuf_Drop(buf);

	return false;
}

bool ACS_CF_Timer(ACSVM_Thread *thread, ACSVM_Word const *argV, ACSVM_Word argC)
{
	(void)argV;
	(void)argC;

	ACSVM_Thread_DataStk_Push(thread, leveltime);
	return false;
}
