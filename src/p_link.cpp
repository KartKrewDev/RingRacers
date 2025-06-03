// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <functional>
#include <initializer_list>

#include "p_link.h"
#include "p_mobj.h"


#define LINK_PACK \
	svg_battleUfoSpawners,\
    svg_checkpoints,\
    svg_rocks



using link = mobj_t*;
using each_ref = std::initializer_list<std::reference_wrapper<mobj_t*>>;

link LINK_PACK;

void P_InitMobjPointers(void)
{
	for (mobj_t*& head : each_ref {LINK_PACK})
	{
		head = nullptr;
	}
}

void P_SaveMobjPointers(void(*fn)(mobj_t*))
{
	for (mobj_t* head : {LINK_PACK})
	{
		fn(head);
	}
}

void P_LoadMobjPointers(void(*fn)(mobj_t**))
{
	for (mobj_t*& head : each_ref {LINK_PACK})
	{
		fn(&head);
	}
}
