// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
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
#include "g_demo.h"
#include "r_main.h"

namespace
{

std::vector<line_t*> g_lines;

};

void P_TestLine(line_t* ld)
{
	g_lines.emplace_back(ld);
}

extern "C" consvar_t cv_showgremlins;

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

	if (!collision)
	{
		line_t *line = nullptr;

		if (!g_lines.empty() && !G_CompatLevel(0x000E))
		{
			// FIXME: This condition is a failsafe!
			// SlopeAABBvsLine::vs_slope can sometimes report
			// no collision despite P_CheckPosition saying otherwise.
			// (Generally related to infinitesimal numbers or overflows.)
			// When it reports no collision, that means no line and
			// no normals to base the collision off of.
			// Here we provide the last line checked and normals based on
			// the line and the player's momentum angle.
			// But the proper fix would be to make vs_slope work!!

			line = g_lines.back();

			angle_t lineangle = line->angle;
			angle_t mobjangle = R_PointToAngle2(ax, ay, bx, by);
			angle_t diff = lineangle - mobjangle;

			lineangle += diff > ANGLE_180 ? -ANGLE_90 : ANGLE_90;

			return_normal->x = FINECOSINE((lineangle >> ANGLETOFINESHIFT) & FINEMASK);
			return_normal->y = FINESINE((lineangle >> ANGLETOFINESHIFT) & FINEMASK);

			// Moving the gremlin debug here so that it
			// still fires even when the workaround is used.
			if (cv_showgremlins.value)
			{
				mobj_t *mo = g_tm.thing;

				if (mo)
				{
					mobj_t *x = P_SpawnMobj(mo->x, mo->y, mo->z, MT_THOK);
					x->frame = FF_FULLBRIGHT | FF_ADD;
					x->renderflags = RF_ALWAYSONTOP;
					x->color = SKINCOLOR_RED;

					CONS_Printf(
						"GREMLIN: leveltime=%u x=%f y=%f z=%f momx=%f momy=%f momz=%f\n",
						leveltime,
						FixedToFloat(mo->x),
						FixedToFloat(mo->y),
						FixedToFloat(mo->z),
						FixedToFloat(mo->momx),
						FixedToFloat(mo->momy),
						FixedToFloat(mo->momz)
					);
				}
			}
		}

		g_lines.clear();
		return line;
	}

	g_lines.clear();
	return_normal->x = Fixed {collision->normal.x};
	return_normal->y = Fixed {collision->normal.y};

	return collision->ld;
}

void P_ClearTestLines(void)
{
	g_lines.clear();
}
