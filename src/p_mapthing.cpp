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
/// \file  p_mapthing.cpp
/// \brief Mapthing spawning assistance

#include <algorithm>
#include <iterator>
#include <set>

#include "core/static_vec.hpp"

#include "doomstat.h" // mapobjectscale
#include "info.h"
#include "m_fixed.h"
#include "p_local.h" // ONFLOORZ
#include "p_mobj.h"
#include "p_slopes.h"
#include "r_defs.h"
#include "r_main.h"

fixed_t P_GetMobjSpawnHeight(
	const mobjtype_t mobjtype,
	const fixed_t x,
	const fixed_t y,
	const fixed_t dz,
	const fixed_t offset,
	const size_t layer,
	const boolean flip,
	const fixed_t scale
)
{
	const fixed_t finalScale = FixedMul(scale, mapobjectscale);

	const fixed_t finalZOffset = flip
		? -(dz) - FixedMul(finalScale, offset + mobjinfo[mobjtype].height)
		: +(dz) + FixedMul(finalScale, offset);

	const sector_t* sector = R_PointInSubsector(x, y)->sector;

	// Axis objects snap to the floor.
	if (mobjtype == MT_AXIS || mobjtype == MT_AXISTRANSFER || mobjtype == MT_AXISTRANSFERLINE)
		return ONFLOORZ;

	if (layer != 0)
	{
		// multiset is a container that automatically sorts
		// each insertion. It only contains unique values, so
		// two FOFs at the exact same height only count as one
		// layer.
		std::multiset<fixed_t> heights;

		auto get_height = flip ? P_GetFFloorBottomZAt : P_GetFFloorTopZAt;

		for (ffloor_t* rover = sector->ffloors; rover; rover = rover->next)
		{
			heights.insert(get_height(rover, x, y));
		}

		if (!heights.empty())
		{
			auto get_layer = [layer, &heights](auto it)
			{
				std::advance(it, std::min(layer, heights.size()) - 1);
				return *it;
			};

			if (flip)
			{
				return get_layer(heights.rbegin()) + finalZOffset;
			}
			else
			{
				return get_layer(heights.begin()) + finalZOffset;
			}
		}
	}

	// Establish height.
	if (flip)
		return P_GetSectorCeilingZAt(sector, x, y) + finalZOffset;
	else
		return P_GetSectorFloorZAt(sector, x, y) + finalZOffset;
}
