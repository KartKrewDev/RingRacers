// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

/*
 * Compiles date and time.
 *
 * Kalaron: Can't this be somewhere else instead of in an extra c file?
 * Alam: Code::Block, XCode and the Makefile touch this file to update
 *  the timestamp
 *
 */

#if (defined(CMAKECONFIG))
#include "config.h"
const char *compbranch = SRB2_COMP_BRANCH;
const char *comprevision = SRB2_COMP_REVISION;
const char *compnote = SRB2_COMP_NOTE;
const char *comptype = CMAKE_BUILD_TYPE;
const int compoptimized = SRB2_COMP_OPTIMIZED;

#elif (defined(COMPVERSION))
#include "comptime.h"

#else
const char *compbranch = "Unknown";
const char *comprevision = "illegal";

#endif

const int compuncommitted =
#if (defined(COMPVERSION_UNCOMMITTED))
1;
#else
0;
#endif

const char *compdate = __DATE__;
const char *comptime = __TIME__;
