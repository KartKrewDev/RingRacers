// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef MUSIC_DETAIL_HPP
#define MUSIC_DETAIL_HPP

#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"

namespace srb2::music::detail
{

inline constexpr tic_t msec_to_tics(int msec)
{
	return msec * (TICRATE / 1000.f);
}

inline constexpr int tics_to_msec(tic_t tics)
{
	return tics * (1000.f / TICRATE);
}

inline tic_t tic_time()
{
	// Use gametic so it is synced with game logic.
	return gametic;
}

}; // namespace srb2::music::detail

#endif // MUSIC_DETAIL_HPP
