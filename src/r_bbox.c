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
/// \file  r_bbox.c
/// \brief Boundary box (cube) renderer

#include "doomdef.h"
#include "command.h"
#include "r_local.h"
#include "screen.h" // cv_renderhitbox
#include "v_video.h" // V_DrawFill

enum {
	RENDERHITBOX_OFF,
	RENDERHITBOX_TANGIBLE,
	RENDERHITBOX_ALL,
	RENDERHITBOX_INTANGIBLE,
	RENDERHITBOX_RINGS,
};

CV_PossibleValue_t renderhitbox_cons_t[] = {
	{RENDERHITBOX_OFF, "Off"},
	{RENDERHITBOX_TANGIBLE, "Tangible"},
	{RENDERHITBOX_ALL, "All"},
	{RENDERHITBOX_INTANGIBLE, "Intangible"},
	{RENDERHITBOX_RINGS, "Rings"},
	{0}};

struct bbox_col {
	INT32 x;
	INT32 y;
	INT32 h;
};

struct bbox_config {
	fixed_t height;
	fixed_t tz;
	struct bbox_col col[4];
	UINT8 color;
};

static inline void
raster_bbox_seg
(		INT32 x,
		fixed_t y,
		fixed_t h,
		UINT8 pixel)
{
	y /= FRACUNIT;

	h = y + (FixedCeil(abs(h)) / FRACUNIT);

	if (y < 0)
		y = 0;

	if (h >= viewheight)
		h = viewheight;

	while (y < h)
	{
		topleft[x + y * vid.width] = pixel;
		y++;
	}
}

static void
draw_bbox_col
(		struct bbox_config * bb,
		int p,
		fixed_t tx,
		fixed_t ty)
{
	struct bbox_col *col = &bb->col[p];

	fixed_t xscale, yscale;

	if (ty < FRACUNIT) // projection breaks down here
		ty = FRACUNIT;

	xscale = FixedDiv(projection[viewssnum], ty);
	yscale = FixedDiv(projectiony[viewssnum], ty);

	col->x = (centerxfrac + FixedMul(tx, xscale)) / FRACUNIT;
	col->y = (centeryfrac - FixedMul(bb->tz, yscale));
	col->h = FixedMul(bb->height, yscale);

	if (col->x >= 0 && col->x < viewwidth)
	{
		raster_bbox_seg(col->x, col->y, col->h, bb->color);
	}
}

static void
draw_bbox_row
(		struct bbox_config * bb,
		int p1,
		int p2)
{
	struct bbox_col
		*a = &bb->col[p1],
		*b = &bb->col[p2];

	INT32 x1, x2; // left, right
	INT32 dx; // width

	fixed_t y1, y2; // top, bottom
	fixed_t s1, s2; // top and bottom increment

	if (a->x > b->x)
	{
		struct bbox_col *c = a;
		a = b;
		b = c;
	}

	x1 = a->x;
	x2 = b->x;

	if (x1 == x2 || x1 >= viewwidth || x2 < 0)
		return;

	dx = x2 - x1;

	y1 = a->y;
	y2 = b->y;
	s1 = (y2 - y1) / dx;

	y2 = y1 + a->h;
	s2 = ((b->y + b->h) - y2) / dx;

	// FixedCeil needs a minimum!!! :D :D

	if (s1 == 0)
		s1 = 1;

	if (s2 == 0)
		s2 = 1;

	if (x1 < 0)
	{
		y1 -= x1 * s1;
		y2 -= x1 * s2;
		x1 = 0;
	}

	if (x2 >= viewwidth)
		x2 = viewwidth - 1;

	while (x1 < x2)
	{
		raster_bbox_seg(x1, y1, s1, bb->color);
		raster_bbox_seg(x1, y2, s2, bb->color);

		y1 += s1;
		y2 += s2;

		x1++;
	}
}

UINT8 R_GetBoundingBoxColor(mobj_t *thing)
{
	UINT32 flags = thing->flags;

	if (thing->player)
		return 255; // 0FF

	if (flags & (MF_NOCLIPTHING))
		return 7; // BFBFBF

	if (flags & (MF_SPECIAL))
		return 73; // FF0

	if (flags & (MF_BOSS|MF_MISSILE|MF_ENEMY|MF_PAIN))
		return 35; // F00

	if (flags & (MF_NOCLIP))
		return 152; // 00F

	return 0; // FFF
}

void R_DrawThingBoundingBox(vissprite_t *vis)
{
	// radius offsets
	fixed_t rs = vis->scale;
	fixed_t rc = vis->xscale;

	// translated coordinates
	fixed_t tx = vis->gx;
	fixed_t ty = vis->gy;

	struct bbox_config bb = {
		.height = vis->thingheight,
		.tz = vis->texturemid,
		.color = R_GetBoundingBoxColor(vis->mobj),
	};

	if (vis->mobjflags & MF_PAPERCOLLISION)
	{
		// 0--1

		draw_bbox_col(&bb, 0, tx - rc, ty - rs); // left
		draw_bbox_col(&bb, 1, tx + rc, ty + rs); // right
		draw_bbox_row(&bb, 0, 1); // connect both
	}
	else
	{
		// 1--3
		// |  |
		// 0--2

		// left

		draw_bbox_col(&bb, 0, tx, ty); // bottom
		draw_bbox_col(&bb, 1, tx - rc, ty + rs); // top

		// right

		tx += rs;
		ty += rc;

		draw_bbox_col(&bb, 2, tx, ty); // bottom
		draw_bbox_col(&bb, 3, tx - rc, ty + rs); // top

		// connect all four columns

		draw_bbox_row(&bb, 0, 1);
		draw_bbox_row(&bb, 1, 3);
		draw_bbox_row(&bb, 3, 2);
		draw_bbox_row(&bb, 2, 0);
	}
}

static boolean is_tangible (mobj_t *thing)
{
	// These objects can never touch another
	if (thing->flags & (MF_NOCLIPTHING))
	{
		return false;
	}

	// These objects probably do nothing! :D
	if ((thing->flags & (MF_SPECIAL|MF_SOLID|MF_SHOOTABLE
					|MF_PUSHABLE|MF_BOSS|MF_MISSILE|MF_SPRING
					|MF_ELEMENTAL|MF_ENEMY|MF_PAIN|MF_DONTPUNT
					|MF_PICKUPFROMBELOW)) == 0U)
	{
		return false;
	}

	return true;
}

boolean R_ThingBoundingBoxVisible(mobj_t *thing)
{
	INT32 cvmode = cv_renderhitbox.value;

	if (thing->type == MT_WAYPOINT)
	{
		// Waypoints debugger serves this purpose
		return false;
	}

	if (thing == r_viewmobj)
	{
		// Rendering bbox right on top causes anomalies
		return false;
	}

	switch (cvmode)
	{
		case RENDERHITBOX_OFF:
			return false;

		case RENDERHITBOX_ALL:
			return true;

		case RENDERHITBOX_INTANGIBLE:
			return !is_tangible(thing);

		case RENDERHITBOX_TANGIBLE:
			// Exclude rings from here, lots of them!
			if (thing->type == MT_RING)
			{
				return false;
			}

			return is_tangible(thing);

		case RENDERHITBOX_RINGS:
			return (thing->type == MT_RING);

		default:
			return false;
	}
}
