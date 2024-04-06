// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by James Robert Roman
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <optional>
#include <vector>

#include "math/fixed.hpp"
#include "p_sweep.hpp"

#include "p_local.h"

namespace
{

std::vector<line_t*> g_lines;

};

void P_TestLine(line_t* ld)
{
	g_lines.emplace_back(ld);
}

line_t* P_SweepTestLines(fixed_t ax, fixed_t ay, fixed_t bx, fixed_t by, fixed_t r, vector2_t* return_normal)
{
	using namespace srb2::math;
	using namespace srb2::sweep;

	struct Collision
	{
		unit z;
		vec2 normal;
		line_t* ld;

		bool operator<(const Collision& b) const { return z < b.z; }
	};

	std::optional<Collision> collision;

	LineSegment<Fixed> l{{ax, ay}, {bx, by}};
	AABBvsLine sweep{r, l};

	for (line_t* ld : g_lines)
	{
		LineSegment<Fixed> ls{{ld->v1->x, ld->v1->y}, {ld->v2->x, ld->v2->y}};
		Result rs = sweep(ls);
		if (rs.hit)
		{
			if (!collision || rs.hit->z < collision->z)
			{
				collision = {rs.hit->z, rs.hit->n, ld};
			}
		}
	}

	g_lines.clear();

	if (!collision)
	{
		return nullptr;
	}

	return_normal->x = Fixed {collision->normal.x};
	return_normal->y = Fixed {collision->normal.y};

	return collision->ld;
}

void P_ClearTestLines(void)
{
	g_lines.clear();
}
