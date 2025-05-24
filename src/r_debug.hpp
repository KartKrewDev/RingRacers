// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef r_debug_hpp
#define r_debug_hpp

#include "doomtype.h"

namespace srb2::r_debug
{

void add_texture_to_frame_list(INT32 texnum);
void clear_frame_list();
void draw_frame_list();

}; // namespace srb2::r_debug

#endif/*r_debug_hpp*/
