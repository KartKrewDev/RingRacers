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
/// \file  r_bsp.c
/// \brief BSP traversal, handling of LineSegs for rendering

#include <algorithm>
#include <vector>

#include <tracy/tracy/Tracy.hpp>

#include "command.h"
#include "doomdef.h"
#include "g_game.h"
#include "r_local.h"
#include "r_state.h"
#include "r_portal.h" // Add seg portals
#include "r_fps.h"

#include "r_splats.h"
#include "p_local.h" // camera
#include "p_slopes.h"
#include "z_zone.h" // Check R_Prep3DFloors
#include "taglist.h"

#include "k_terrain.h"

extern "C" consvar_t cv_debugfinishline, cv_debugrender_freezebsp;

seg_t *curline;
side_t *sidedef;
line_t *linedef;
sector_t *frontsector;
sector_t *backsector;

// very ugly realloc() of drawsegs at run-time, I upped it to 512
// instead of 256.. and someone managed to send me a level with
// 896 drawsegs! So too bad here's a limit removal a-la-Boom
drawseg_t *curdrawsegs = NULL; /**< This is used to handle multiple lists for masked drawsegs. */
drawseg_t *drawsegs = NULL;
drawseg_t *ds_p = NULL;

// indicates doors closed wrt automap bugfix:
INT32 doorclosed;

// A wall was drawn covering the whole screen, which means we
// can block off the BSP across that seg.
boolean g_walloffscreen;

static std::vector<std::vector<INT32>> node_cache;
static std::vector<INT32>* current_node_cache;

boolean R_NoEncore(sector_t *sector, levelflat_t *flat, boolean ceiling)
{
	const boolean invertEncore = (sector->flags & MSF_INVERTENCORE);
	const terrain_t *terrain = (flat != NULL ? flat->terrain : NULL);

	if ((terrain == NULL) || (terrain->flags & TRF_REMAP))
	{
		return invertEncore;
	}

	if (invertEncore)
	{
		return false;
	}

	if (ceiling)
	{
		return ((boolean)(sector->flags & MSF_FLIPSPECIAL_CEILING));
	}

	return ((boolean)(sector->flags & MSF_FLIPSPECIAL_FLOOR));
}

boolean R_IsRipplePlane(sector_t *sector, ffloor_t *rover, int ceiling)
{
	return rover ? (rover->fofflags & FOF_RIPPLE) :
		(sector->flags & (MSF_RIPPLE_FLOOR << ceiling));
}

//
// R_ClearDrawSegs
//
void R_ClearDrawSegs(void)
{
	ds_p = drawsegs;
}

// Fix from boom.
#define MAXSEGS (MAXVIDWIDTH/2+1)

// newend is one past the last valid seg
static cliprange_t *newend;
static cliprange_t solidsegs[MAXSEGS];

namespace
{

enum class ClipType
{
	// Does handle solid walls,
	//  e.g. single sided LineDefs (middle texture)
	//  that entirely block the view.
	kSolid,
	kSolidDontRender,

	// Clips the given range of columns, but does not include it in the clip list.
	// Does handle windows, e.g. LineDefs with upper and lower texture.
	kPass,
};

void R_CrunchWallSegment(cliprange_t *start, cliprange_t *next)
{
	// Remove start+1 to next from the clip list, because start now covers their area.

	if (next == start)
		return; // Post just extended past the bottom of one post.

	while (next++ != newend)
		*++start = *next; // Remove a post.

	newend = start + 1;

	// NO MORE CRASHING!
	if (newend - solidsegs > MAXSEGS)
		I_Error("R_CrunchWallSegment: Solid Segs overflow!\n");
}

template <ClipType Type>
void R_ClipWallSegment(INT32 first, INT32 last)
{
	cliprange_t *next;
	cliprange_t *start;

	// Find the first range that touches the range (adjacent pixels are touching).
	start = solidsegs;
	while (start->last < first - 1)
		start++;

	if (first < start->first)
	{
		if (last < start->first - 1)
		{
			// Post is entirely visible (above start), so insert a new clippost.
			if constexpr (Type != ClipType::kSolidDontRender)
			{
				R_StoreWallRange(first, last);
			}

			if constexpr (Type != ClipType::kPass)
			{
				next = newend;
				newend++;
				// NO MORE CRASHING!
				if (newend - solidsegs > MAXSEGS)
					I_Error("R_ClipSolidWallSegment: Solid Segs overflow!\n");

				while (next != start)
				{
					*next = *(next-1);
					next--;
				}
				next->first = first;
				next->last = last;
			}

			return;
		}

		// There is a fragment above *start.
		if constexpr (Type != ClipType::kSolidDontRender)
		{
			R_StoreWallRange(first, start->first - 1);
		}

		if constexpr (Type != ClipType::kPass)
		{
			// Now adjust the clip size.
			start->first = first;
		}
	}

	// Bottom contained in start?
	if (last <= start->last)
		return;

	next = start;
	while (last >= (next+1)->first - 1)
	{
		// There is a fragment between two posts.
		if constexpr (Type != ClipType::kSolidDontRender)
		{
			R_StoreWallRange(next->last + 1, (next+1)->first - 1);
		}

		next++;

		if (last <= next->last)
		{
			if constexpr (Type != ClipType::kPass)
			{
				// Bottom is contained in next.
				// Adjust the clip size.
				start->last = next->last;
				R_CrunchWallSegment(start, next);
			}

			return;
		}
	}

	// There is a fragment after *next.
	if constexpr (Type != ClipType::kSolidDontRender)
	{
		R_StoreWallRange(next->last + 1, last);
	}

	if constexpr (Type != ClipType::kPass)
	{
		// Adjust the clip size.
		start->last = last;
		R_CrunchWallSegment(start, next);
	}
}

}; // namespace

//
// R_ClearClipSegs
//
void R_ClearClipSegs(void)
{
	solidsegs[0].first = -0x7fffffff;
	solidsegs[0].last = -1;
	solidsegs[1].first = viewwidth;
	solidsegs[1].last = 0x7fffffff;
	newend = solidsegs + 2;
}
void R_PortalClearClipSegs(INT32 start, INT32 end)
{
	solidsegs[0].first = -0x7fffffff;
	solidsegs[0].last = start-1;
	solidsegs[1].first = end;
	solidsegs[1].last = 0x7fffffff;
	newend = solidsegs + 2;
}

//
// If player's view height is underneath fake floor, lower the
// drawn ceiling to be just under the floor height, and replace
// the drawn floor and ceiling textures, and light level, with
// the control sector's.
//
// Similar for ceiling, only reflected.
//
sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec, INT32 *floorlightlevel,
	INT32 *ceilinglightlevel, boolean back)
{
	if (floorlightlevel)
		*floorlightlevel = sec->floorlightsec == -1 ?
			(sec->floorlightabsolute ? sec->floorlightlevel : std::max(0, std::min(255, sec->lightlevel + sec->floorlightlevel))) : sectors[sec->floorlightsec].lightlevel;

	if (ceilinglightlevel)
		*ceilinglightlevel = sec->ceilinglightsec == -1 ?
			(sec->ceilinglightabsolute ? sec->ceilinglightlevel : std::max(0, std::min(255, sec->lightlevel + sec->ceilinglightlevel))) : sectors[sec->ceilinglightsec].lightlevel;

	// if (sec->midmap != -1)
	//	mapnum = sec->midmap;
	// In original colormap code, this block did not run if sec->midmap was set
	if (!sec->extra_colormap && sec->heightsec != -1)
	{
		const sector_t *s = &sectors[sec->heightsec];
		mobj_t *viewmobj = viewplayer->mo;
		INT32 heightsec;
		boolean underwater;
		UINT8 i = R_GetViewNumber();

		if (camera[i].chase)
			heightsec = R_PointInSubsector(camera[i].x, camera[i].y)->sector->heightsec;
		else if (i > r_splitscreen && viewmobj)
			heightsec = R_PointInSubsector(viewmobj->x, viewmobj->y)->sector->heightsec;
		else
			return sec;

		underwater = (heightsec != -1 && viewz <= sectors[heightsec].floorheight);

		// Replace sector being drawn, with a copy to be hacked
		*tempsec = *sec;

		// Replace floor and ceiling height with other sector's heights.
		tempsec->floorheight = s->floorheight;
		tempsec->ceilingheight = s->ceilingheight;

		if ((underwater && (tempsec->  floorheight = sec->floorheight,
			tempsec->ceilingheight = s->floorheight - 1, !back)) || viewz <= s->floorheight)
		{ // head-below-floor hack
			tempsec->floorpic = s->floorpic;
			tempsec->floor_xoffs = s->floor_xoffs;
			tempsec->floor_yoffs = s->floor_yoffs;
			tempsec->floorpic_angle = s->floorpic_angle;

			if (underwater)
			{
				if (s->ceilingpic == skyflatnum)
				{
					tempsec->floorheight = tempsec->ceilingheight+1;
					tempsec->ceilingpic = tempsec->floorpic;
					tempsec->ceiling_xoffs = tempsec->floor_xoffs;
					tempsec->ceiling_yoffs = tempsec->floor_yoffs;
					tempsec->ceilingpic_angle = tempsec->floorpic_angle;
				}
				else
				{
					tempsec->ceilingpic = s->ceilingpic;
					tempsec->ceiling_xoffs = s->ceiling_xoffs;
					tempsec->ceiling_yoffs = s->ceiling_yoffs;
					tempsec->ceilingpic_angle = s->ceilingpic_angle;
				}
			}

			tempsec->lightlevel = s->lightlevel;

			if (floorlightlevel)
				*floorlightlevel = s->floorlightsec == -1 ? (s->floorlightabsolute ? s->floorlightlevel : std::max(0, std::min(255, s->lightlevel + s->floorlightlevel)))
					: sectors[s->floorlightsec].lightlevel;

			if (ceilinglightlevel)
				*ceilinglightlevel = s->ceilinglightsec == -1 ? (s->ceilinglightabsolute ? s->ceilinglightlevel : std::max(0, std::min(255, s->lightlevel + s->ceilinglightlevel)))
					: sectors[s->ceilinglightsec].lightlevel;
		}
		else if (heightsec != -1 && viewz >= sectors[heightsec].ceilingheight
			&& sec->ceilingheight > s->ceilingheight)
		{ // Above-ceiling hack
			tempsec->ceilingheight = s->ceilingheight;
			tempsec->floorheight = s->ceilingheight + 1;

			tempsec->floorpic = tempsec->ceilingpic = s->ceilingpic;
			tempsec->floor_xoffs = tempsec->ceiling_xoffs = s->ceiling_xoffs;
			tempsec->floor_yoffs = tempsec->ceiling_yoffs = s->ceiling_yoffs;
			tempsec->floorpic_angle = tempsec->ceilingpic_angle = s->ceilingpic_angle;

			if (s->floorpic == skyflatnum) // SKYFIX?
			{
				tempsec->ceilingheight = tempsec->floorheight-1;
				tempsec->floorpic = tempsec->ceilingpic;
				tempsec->floor_xoffs = tempsec->ceiling_xoffs;
				tempsec->floor_yoffs = tempsec->ceiling_yoffs;
				tempsec->floorpic_angle = tempsec->ceilingpic_angle;
			}
			else
			{
				tempsec->ceilingheight = sec->ceilingheight;
				tempsec->floorpic = s->floorpic;
				tempsec->floor_xoffs = s->floor_xoffs;
				tempsec->floor_yoffs = s->floor_yoffs;
				tempsec->floorpic_angle = s->floorpic_angle;
			}

			tempsec->lightlevel = s->lightlevel;

			if (floorlightlevel)
				*floorlightlevel = s->floorlightsec == -1 ? (s->floorlightabsolute ? s->floorlightlevel : std::max(0, std::min(255, s->lightlevel + s->floorlightlevel)))
					: sectors[s->floorlightsec].lightlevel;

			if (ceilinglightlevel)
				*ceilinglightlevel = s->ceilinglightsec == -1 ? (s->ceilinglightabsolute ? s->ceilinglightlevel : std::max(0, std::min(255, s->lightlevel + s->ceilinglightlevel)))
					: sectors[s->ceilinglightsec].lightlevel;
		}
		sec = tempsec;
	}

	return sec;
}

boolean R_IsEmptyLine(seg_t *line, sector_t *front, sector_t *back)
{
	return (
		!R_IsDebugLine(line) &&
		!line->polyseg &&
		back->ceilingpic == front->ceilingpic
		&& back->floorpic == front->floorpic
		&& back->f_slope == front->f_slope
		&& back->c_slope == front->c_slope
		&& back->lightlevel == front->lightlevel
		&& !line->sidedef->midtexture
		// Check offsets too!
		&& back->floor_xoffs == front->floor_xoffs
		&& back->floor_yoffs == front->floor_yoffs
		&& back->floorpic_angle == front->floorpic_angle
		&& back->ceiling_xoffs == front->ceiling_xoffs
		&& back->ceiling_yoffs == front->ceiling_yoffs
		&& back->ceilingpic_angle == front->ceilingpic_angle
		// Consider altered lighting.
		&& back->floorlightlevel == front->floorlightlevel
		&& back->floorlightabsolute == front->floorlightabsolute
		&& back->ceilinglightlevel == front->ceilinglightlevel
		&& back->ceilinglightabsolute == front->ceilinglightabsolute
		&& back->floorlightsec == front->floorlightsec
		&& back->ceilinglightsec == front->ceilinglightsec
		// Consider colormaps
		&& back->extra_colormap == front->extra_colormap
		&& ((!front->ffloors && !back->ffloors)
		|| Tag_Compare(&front->tags, &back->tags))
		&& (!cv_debugfinishline.value || back->damagetype == front->damagetype));
}

boolean R_IsDebugLine(seg_t *line)
{
	if (cv_debugfinishline.value)
	{
		switch (line->linedef->special)
		{
			case 2001: // Ring Racers: Finish Line
			case 2003: // Ring Racers: Respawn Line
			case 2005: // Ring Racers: Dismount flying object Line
				return true;
		}
	}

	return false;
}

boolean R_ShouldFlipTripWire(const line_t *ld)
{
	// Flip tripwire textures when they are unpegged
	// so the energy flows downward instead of upward, matching collision behavior
	return (ld->tripwire && !(ld->flags & ML_MIDPEG));
}

//
// R_AddLine
// Clips the given segment and adds any visible pieces to the line list.
//
static void R_AddLine(seg_t *line)
{
	INT32 x1, x2;
	angle_t angle1, angle2, span, tspan;
	static sector_t tempsec;
	boolean bothceilingssky = false, bothfloorssky = false;

	g_portal = NULL;

	if (line->polyseg && !(line->polyseg->flags & POF_RENDERSIDES))
		return;

	// big room fix
	angle1 = R_PointToAngleEx(viewx, viewy, line->v1->x, line->v1->y);
	angle2 = R_PointToAngleEx(viewx, viewy, line->v2->x, line->v2->y);
	curline = line;

	// Clip to view edges.
	span = angle1 - angle2;

	// Back side? i.e. backface culling?
	if (span >= ANGLE_180)
		return;

	// Global angle needed by segcalc.
	rw_angle1 = angle1;
	angle1 -= viewangle;
	angle2 -= viewangle;

	tspan = angle1 + clipangle[viewssnum];
	if (tspan > doubleclipangle[viewssnum])
	{
		tspan -= doubleclipangle[viewssnum];

		// Totally off the left edge?
		if (tspan >= span)
			return;

		angle1 = clipangle[viewssnum];
	}
	tspan = clipangle[viewssnum] - angle2;
	if (tspan > doubleclipangle[viewssnum])
	{
		tspan -= doubleclipangle[viewssnum];

		// Totally off the left edge?
		if (tspan >= span)
			return;

		angle2 = -(signed)clipangle[viewssnum];
	}

	// The seg is in the view range, but not necessarily visible.
	angle1 = (angle1+ANGLE_90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANGLE_90)>>ANGLETOFINESHIFT;
	x1 = viewangletox[viewssnum][angle1];
	x2 = viewangletox[viewssnum][angle2];

	// Does not cross a pixel?
	if (x1 >= x2)       // killough 1/31/98 -- change == to >= for robustness
		return;

	backsector = line->backsector;

	// Portal line
	if (line->linedef->special == 40 && line->side == 0)
	{
		// Render portal if recursiveness limit hasn't been reached.
		// Otherwise, render the wall normally.
		if (portalrender < cv_maxportals.value)
		{
			size_t p;
			mtag_t tag = Tag_FGet(&line->linedef->tags);
			INT32 li1 = line->linedef-lines;
			INT32 li2;

			for (p = 0; (li2 = Tag_Iterate_Lines(tag, p)) >= 0; p++)
			{
				// Skip invalid lines.
				if ((tag != Tag_FGet(&lines[li2].tags)) || (lines[li1].special != lines[li2].special) || (li1 == li2))
					continue;

				Portal_Add2Lines(li1, li2, x1, x2);
				goto clipsolid;
			}
		}
	}

	// Single sided line?
	if (!backsector)
		goto clipsolid;

	// Finish line debug: make solid walls pitch black. This
	// contrasts areas that are impossible to traverse next to
	// finish lines.
	if (cv_debugfinishline.value && (line->linedef->flags & (ML_IMPASSABLE|ML_BLOCKPLAYERS)))
	{
		goto clipsolid;
	}

	backsector = R_FakeFlat(backsector, &tempsec, NULL, NULL, true);

	doorclosed = 0;

	if (backsector->ceilingpic == skyflatnum && frontsector->ceilingpic == skyflatnum)
		bothceilingssky = true;
	if (backsector->floorpic == skyflatnum && frontsector->floorpic == skyflatnum)
		bothfloorssky = true;

	if (bothceilingssky && bothfloorssky) // everything's sky? let's save us a bit of time then
	{
		if (!R_IsDebugLine(line) &&
			!line->polyseg &&
			!line->sidedef->midtexture
			&& ((!frontsector->ffloors && !backsector->ffloors)
				|| Tag_Compare(&frontsector->tags, &backsector->tags)))
			return; // line is empty, don't even bother

		goto clippass; // treat like wide open window instead
	}

	// Closed door.
	if (frontsector->f_slope || frontsector->c_slope || backsector->f_slope || backsector->c_slope)
	{
		fixed_t frontf1,frontf2, frontc1, frontc2; // front floor/ceiling ends
		fixed_t backf1, backf2, backc1, backc2; // back floor ceiling ends
#define SLOPEPARAMS(slope, end1, end2, normalheight) \
		end1 = P_GetZAt(slope, line->v1->x, line->v1->y, normalheight); \
		end2 = P_GetZAt(slope, line->v2->x, line->v2->y, normalheight);

		SLOPEPARAMS(frontsector->f_slope, frontf1, frontf2, frontsector->  floorheight)
		SLOPEPARAMS(frontsector->c_slope, frontc1, frontc2, frontsector->ceilingheight)
		SLOPEPARAMS( backsector->f_slope,  backf1,  backf2,  backsector->  floorheight)
		SLOPEPARAMS( backsector->c_slope,  backc1,  backc2,  backsector->ceilingheight)
#undef SLOPEPARAMS
		// if both ceilings are skies, consider it always "open"
		// same for floors
		if (!bothceilingssky && !bothfloorssky)
		{
			doorclosed = (backc1 <= frontf1 && backc2 <= frontf2)
			|| (backf1 >= frontc1 && backf2 >= frontc2)
			// Check for automap fix. Store in doorclosed for r_segs.c
			|| (backc1 <= backf1 && backc2 <= backf2
				&& ((backc1 >= frontc1 && backc2 >= frontc2) || curline->sidedef->toptexture)
				&& ((backf1 <= frontf1 && backf2 >= frontf2) || curline->sidedef->bottomtexture));

			if (doorclosed)
				goto clipsolid;
		}

		// Window.
		if (!bothceilingssky) // ceilings are always the "same" when sky
			if (backc1 != frontc1 || backc2 != frontc2)
				goto clippass;
		if (!bothfloorssky)	// floors are always the "same" when sky
			if (backf1 != frontf1 || backf2 != frontf2)
				goto clippass;
	}
	else
	{
		// if both ceilings are skies, consider it always "open"
		// same for floors
		if (!bothceilingssky && !bothfloorssky)
		{
			doorclosed = backsector->ceilingheight <= frontsector->floorheight
			|| backsector->floorheight >= frontsector->ceilingheight
			// Check for automap fix. Store in doorclosed for r_segs.c
			//
			// This is used to fix the automap bug which
			// showed lines behind closed doors simply because the door had a dropoff.
			//
			// It assumes that Doom has already ruled out a door being closed because
			// of front-back closure (e.g. front floor is taller than back ceiling).
			//
			// if door is closed because back is shut:
			|| (backsector->ceilingheight <= backsector->floorheight
				// preserve a kind of transparent door/lift special effect:
				&& (backsector->ceilingheight >= frontsector->ceilingheight || curline->sidedef->toptexture)
				&& (backsector->floorheight <= frontsector->floorheight || curline->sidedef->bottomtexture));

			if (doorclosed)
				goto clipsolid;
		}

		// Window.
		if (!bothceilingssky) // ceilings are always the "same" when sky
			if (backsector->ceilingheight != frontsector->ceilingheight)
				goto clippass;
		if (!bothfloorssky)	// floors are always the "same" when sky
			if (backsector->floorheight != frontsector->floorheight)
				goto clippass;
	}

	// Reject empty lines used for triggers and special events.
	// Identical floor and ceiling on both sides, identical light levels on both sides,
	// and no middle texture.

	if (R_IsEmptyLine(line, frontsector, backsector))
		return;

clippass:
	g_walloffscreen = false;
	R_ClipWallSegment<ClipType::kPass>(x1, x2 - 1);

	if (g_walloffscreen)
	{
		R_ClipWallSegment<ClipType::kSolidDontRender>(x1, x2 -1);
	}
	return;

clipsolid:
	g_walloffscreen = false;
	R_ClipWallSegment<ClipType::kSolid>(x1, x2 - 1);
}

//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
//
//   | 0 | 1 | 2
// --+---+---+---
// 0 | 0 | 1 | 2
// 1 | 4 | 5 | 6
// 2 | 8 | 9 | A
INT32 checkcoord[12][4] =
{
	{3, 0, 2, 1},
	{3, 0, 2, 0},
	{3, 1, 2, 0},
	{0}, // UNUSED
	{2, 0, 2, 1},
	{0}, // UNUSED
	{3, 1, 3, 0},
	{0}, // UNUSED
	{2, 0, 3, 1},
	{2, 1, 3, 1},
	{2, 1, 3, 0}
};

static boolean R_CheckBBox(const fixed_t *bspcoord)
{
	angle_t angle1, angle2;
	INT32 sx1, sx2, boxpos;
	const INT32* check;
	cliprange_t *start;

	// Find the corners of the box that define the edges from current viewpoint.
	if ((boxpos = (viewx <= bspcoord[BOXLEFT] ? 0 : viewx < bspcoord[BOXRIGHT] ? 1 : 2) + (viewy >= bspcoord[BOXTOP] ? 0 : viewy > bspcoord[BOXBOTTOM] ? 4 : 8)) == 5)
		return true;

	check = checkcoord[boxpos];

	// big room fix
	angle1 = R_PointToAngleEx(viewx, viewy, bspcoord[check[0]], bspcoord[check[1]]) - viewangle;
	angle2 = R_PointToAngleEx(viewx, viewy, bspcoord[check[2]], bspcoord[check[3]]) - viewangle;

	if ((signed)angle1 < (signed)angle2)
	{
		if ((angle1 >= ANGLE_180) && (angle1 < ANGLE_270))
			angle1 = ANGLE_180-1;
		else
			angle2 = ANGLE_180;
	}

	if ((signed)angle2 >= (signed)clipangle[viewssnum]) return false;
	if ((signed)angle1 <= -(signed)clipangle[viewssnum]) return false;
	if ((signed)angle1 >= (signed)clipangle[viewssnum]) angle1 = clipangle[viewssnum];
	if ((signed)angle2 <= -(signed)clipangle[viewssnum]) angle2 = 0-clipangle[viewssnum];

	// Find the first clippost that touches the source post (adjacent pixels are touching).
	angle1 = (angle1+ANGLE_90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANGLE_90)>>ANGLETOFINESHIFT;
	sx1 = viewangletox[viewssnum][angle1];
	sx2 = viewangletox[viewssnum][angle2];

	// Does not cross a pixel.
	if (sx1 >= sx2) return false;

	start = solidsegs;
	while (start->last < sx2)
		start++;

	if (sx1 >= start->first && sx2 <= start->last)
		return false; // The clippost contains the new span.

	return true;
}

size_t numpolys;        // number of polyobjects in current subsector
size_t num_po_ptrs;     // number of polyobject pointers allocated
polyobj_t **po_ptrs; // temp ptr array to sort polyobject pointers

//
// R_PolyobjCompare
//
// Callback for qsort that compares the z distance of two polyobjects.
// Returns the difference such that the closer polyobject will be
// sorted first.
//
static int R_PolyobjCompare(const void *p1, const void *p2)
{
	const polyobj_t *po1 = *(const polyobj_t * const *)p1;
	const polyobj_t *po2 = *(const polyobj_t * const *)p2;

	return po1->zdist - po2->zdist;
}

//
// R_SortPolyObjects
//
// haleyjd 03/03/06: Here's the REAL meat of Eternity's polyobject system.
// Hexen just figured this was impossible, but as mentioned in polyobj.c,
// it is perfectly doable within the confines of the BSP tree. Polyobjects
// must be sorted to draw in DOOM's front-to-back order within individual
// subsectors. This is a modified version of R_SortVisSprites.
//
void R_SortPolyObjects(subsector_t *sub)
{
	if (numpolys)
	{
		polyobj_t *po;
		INT32 i = 0;

		// allocate twice the number needed to minimize allocations
		if (num_po_ptrs < numpolys*2)
		{
			// use free instead realloc since faster (thanks Lee ^_^)
			free(po_ptrs);
			po_ptrs = static_cast<polyobj_t**>(malloc((num_po_ptrs = numpolys*2) * sizeof(*po_ptrs)));
		}

		po = sub->polyList;

		while (po)
		{
			po->zdist = R_PointToDist2(viewx, viewy,
				po->centerPt.x, po->centerPt.y);
			po_ptrs[i++] = po;
			po = (polyobj_t *)(po->link.next);
		}

		// the polyobjects are NOT in any particular order, so use qsort
		// 03/10/06: only bother if there are actually polys to sort
		if (numpolys >= 2)
		{
			qsort(po_ptrs, numpolys, sizeof(polyobj_t *),
				R_PolyobjCompare);
		}
	}
}

//
// R_PolysegCompare
//
// Callback for qsort to sort the segs of a polyobject. Returns such that the
// closer one is sorted first. I sure hope this doesn't break anything. -Red
//
static int R_PolysegCompare(const void *p1, const void *p2)
{
	const seg_t *seg1 = *(const seg_t * const *)p1;
	const seg_t *seg2 = *(const seg_t * const *)p2;
	fixed_t dist1v1, dist1v2, dist2v1, dist2v2;

	// TODO might be a better way to get distance?
#define pdist(x, y) (FixedMul(R_PointToDist(x, y), FINECOSINE((R_PointToAngle(x, y)-viewangle)>>ANGLETOFINESHIFT))+0xFFFFFFF)
#define vxdist(v) pdist(v->x, v->y)

	dist1v1 = vxdist(seg1->v1);
	dist1v2 = vxdist(seg1->v2);
	dist2v1 = vxdist(seg2->v1);
	dist2v2 = vxdist(seg2->v2);

	if (std::min(dist1v1, dist1v2) != std::min(dist2v1, dist2v2))
		return std::min(dist1v1, dist1v2) - std::min(dist2v1, dist2v2);

	{ // That didn't work, so now let's try this.......
		fixed_t delta1, delta2, x1, y1, x2, y2;
		vertex_t *near1, *near2, *far1, *far2; // wherever you are~

		delta1 = R_PointToDist2(seg1->v1->x, seg1->v1->y, seg1->v2->x, seg1->v2->y);
		delta2 = R_PointToDist2(seg2->v1->x, seg2->v1->y, seg2->v2->x, seg2->v2->y);

		delta1 = FixedDiv(128<<FRACBITS, delta1);
		delta2 = FixedDiv(128<<FRACBITS, delta2);

		if (dist1v1 < dist1v2)
		{
			near1 = seg1->v1;
			far1 = seg1->v2;
		}
		else
		{
			near1 = seg1->v2;
			far1 = seg1->v1;
		}

		if (dist2v1 < dist2v2)
		{
			near2 = seg2->v1;
			far2 = seg2->v2;
		}
		else
		{
			near2 = seg2->v2;
			far2 = seg2->v1;
		}

		x1 = near1->x + FixedMul(far1->x-near1->x, delta1);
		y1 = near1->y + FixedMul(far1->y-near1->y, delta1);

		x2 = near2->x + FixedMul(far2->x-near2->x, delta2);
		y2 = near2->y + FixedMul(far2->y-near2->y, delta2);

		return pdist(x1, y1)-pdist(x2, y2);
	}
#undef vxdist
#undef pdist
}

//
// R_AddPolyObjects
//
// haleyjd 02/19/06
// Adds all segs in all polyobjects in the given subsector.
//
static void R_AddPolyObjects(subsector_t *sub)
{
	polyobj_t *po = sub->polyList;
	size_t i, j;

	numpolys = 0;

	// count polyobjects
	while (po)
	{
		++numpolys;
		po = (polyobj_t *)(po->link.next);
	}

	// for render stats
	ps_numpolyobjects += numpolys;

	// sort polyobjects
	R_SortPolyObjects(sub);

	// render polyobjects
	for (i = 0; i < numpolys; ++i)
	{
		qsort(po_ptrs[i]->segs, po_ptrs[i]->segCount, sizeof(seg_t *), R_PolysegCompare);
		for (j = 0; j < po_ptrs[i]->segCount; ++j)
			R_AddLine(po_ptrs[i]->segs[j]);
	}
}

//
// R_Subsector
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//

drawseg_t *firstseg;

static void R_Subsector(size_t num)
{
	INT32 count, floorlightlevel, ceilinglightlevel, light;
	seg_t *line;
	subsector_t *sub;
	static sector_t tempsec; // Deep water hack
	extracolormap_t *floorcolormap;
	extracolormap_t *ceilingcolormap;
	fixed_t floorcenterz, ceilingcenterz;
	ffloor_t *rover;

	ZoneScoped;

	// subsectors added at run-time
	if (num >= numsubsectors)
	{
		CONS_Debug(DBG_RENDER, "R_Subsector: ss %s with numss = %s\n", sizeu1(num), sizeu2(numsubsectors));
		return;
	}

	sub = &subsectors[num];
	frontsector = sub->sector;
	count = sub->numlines;
	line = &segs[sub->firstline];

	// Deep water/fake ceiling effect.
	frontsector = R_FakeFlat(frontsector, &tempsec, &floorlightlevel, &ceilinglightlevel, false);

	floorcolormap = ceilingcolormap = frontsector->extra_colormap;

	floorcenterz   = P_GetSectorFloorZAt  (frontsector, frontsector->soundorg.x, frontsector->soundorg.y);
	ceilingcenterz = P_GetSectorCeilingZAt(frontsector, frontsector->soundorg.x, frontsector->soundorg.y);

	// Check and prep all 3D floors. Set the sector floor/ceiling light levels and colormaps.
	if (frontsector->ffloors)
	{
		boolean anyMoved = frontsector->moved;

		if (anyMoved == false)
		{
			for (rover = frontsector->ffloors; rover; rover = rover->next)
			{
				sector_t *controlSec = &sectors[rover->secnum];

				if (controlSec->moved == true)
				{
					anyMoved = true;
					break;
				}
			}
		}

		if (anyMoved == true)
		{
			frontsector->numlights = sub->sector->numlights = 0;
			R_Prep3DFloors(frontsector);
			sub->sector->lightlist = frontsector->lightlist;
			sub->sector->numlights = frontsector->numlights;
			sub->sector->moved = frontsector->moved = false;
		}

		light = R_GetPlaneLight(frontsector, floorcenterz, false);
		if (frontsector->floorlightsec == -1 && !frontsector->floorlightabsolute)
			floorlightlevel = std::max(0, std::min(255, *frontsector->lightlist[light].lightlevel + frontsector->floorlightlevel));
		floorcolormap = *frontsector->lightlist[light].extra_colormap;
		light = R_GetPlaneLight(frontsector, ceilingcenterz, false);
		if (frontsector->ceilinglightsec == -1 && !frontsector->ceilinglightabsolute)
			ceilinglightlevel = std::max(0, std::min(255, *frontsector->lightlist[light].lightlevel + frontsector->ceilinglightlevel));
		ceilingcolormap = *frontsector->lightlist[light].extra_colormap;
	}

	auto sector_damage = [](sector_t* s) { return static_cast<sectordamage_t>(s->damagetype); };

	auto floor_damage = [&](sector_t* s) { return s->flags & MSF_FLIPSPECIAL_FLOOR ? sector_damage(s) : SD_NONE; };
	auto ceiling_damage = [&](sector_t* s) { return s->flags & MSF_FLIPSPECIAL_CEILING ? sector_damage(s) : SD_NONE; };

	sub->sector->extra_colormap = frontsector->extra_colormap;

	if (P_GetSectorFloorZAt(frontsector, viewx, viewy) < viewz
		|| frontsector->floorpic == skyflatnum
		|| (frontsector->heightsec != -1 && sectors[frontsector->heightsec].ceilingpic == skyflatnum))
	{
		floorplane = R_FindPlane(
			frontsector->floorheight, frontsector->floorpic, floorlightlevel,
			frontsector->floor_xoffs, frontsector->floor_yoffs, frontsector->floorpic_angle,
			floorcolormap, NULL, NULL, frontsector->f_slope,
			R_NoEncore(frontsector, &levelflats[frontsector->floorpic], false),
			R_IsRipplePlane(frontsector, NULL, false),
			false, frontsector, floor_damage(frontsector)
		);
	}
	else
		floorplane = NULL;

	if (P_GetSectorCeilingZAt(frontsector, viewx, viewy) > viewz
		|| frontsector->ceilingpic == skyflatnum
		|| (frontsector->heightsec != -1 && sectors[frontsector->heightsec].floorpic == skyflatnum))
	{
		ceilingplane = R_FindPlane(
			frontsector->ceilingheight, frontsector->ceilingpic, ceilinglightlevel,
			frontsector->ceiling_xoffs, frontsector->ceiling_yoffs, frontsector->ceilingpic_angle,
			ceilingcolormap, NULL, NULL, frontsector->c_slope,
			R_NoEncore(frontsector, &levelflats[frontsector->ceilingpic], true),
			R_IsRipplePlane(frontsector, NULL, true),
			true, frontsector, ceiling_damage(frontsector)
		);
	}
	else
		ceilingplane = NULL;

	numffloors = 0;
	ffloor[numffloors].slope = NULL;
	ffloor[numffloors].plane = NULL;
	ffloor[numffloors].polyobj = NULL;
	if (frontsector->ffloors)
	{
		fixed_t heightcheck, planecenterz;

		auto fof_damage = [&](auto& f)
		{
			sector_t* s = rover->master->frontsector;
			return rover->fofflags & FOF_BLOCKPLAYER ? f(s) : sector_damage(s);
		};

		auto fof_top_damage = [&] { return fof_damage(floor_damage); };
		auto fof_bottom_damage = [&] { return fof_damage(ceiling_damage); };

		for (rover = frontsector->ffloors; rover && numffloors < MAXFFLOORS; rover = rover->next)
		{
			bool visible = rover->fofflags & FOF_RENDERPLANES;

			if (!(rover->fofflags & FOF_EXISTS) || (!cv_debugfinishline.value && !visible))
				continue;

			if (frontsector->cullheight)
			{
				if (R_DoCulling(frontsector->cullheight, viewsector->cullheight, viewz, *rover->bottomheight, *rover->topheight))
				{
					rover->norender = leveltime;
					continue;
				}
			}

			ffloor[numffloors].plane = NULL;
			ffloor[numffloors].polyobj = NULL;

			heightcheck = P_GetFFloorBottomZAt(rover, viewx, viewy);

			planecenterz = P_GetFFloorBottomZAt(rover, frontsector->soundorg.x, frontsector->soundorg.y);
			if (planecenterz <= ceilingcenterz
				&& planecenterz >= floorcenterz
				&& ((viewz < heightcheck && (rover->fofflags & FOF_BOTHPLANES || !(rover->fofflags & FOF_INVERTPLANES)))
				|| (viewz > heightcheck && (rover->fofflags & FOF_BOTHPLANES || rover->fofflags & FOF_INVERTPLANES))))
			{
				sectordamage_t damage = fof_bottom_damage();

				if (!damage && !visible)
				{
					rover->norender = leveltime; // Tell R_StoreWallRange to skip this
					continue;
				}

				light = R_GetPlaneLight(frontsector, planecenterz,
					viewz < heightcheck);

				ffloor[numffloors].plane = R_FindPlane(
					*rover->bottomheight, *rover->bottompic,
					*frontsector->lightlist[light].lightlevel, *rover->bottomxoffs,
					*rover->bottomyoffs, *rover->bottomangle, *frontsector->lightlist[light].extra_colormap, rover, NULL, *rover->b_slope,
					R_NoEncore(rover->master->frontsector, &levelflats[*rover->bottompic], true),
					R_IsRipplePlane(rover->master->frontsector, rover, true),
					true, frontsector, damage
				);

				ffloor[numffloors].slope = *rover->b_slope;

				// Tell the renderer this sector has slopes in it.
				if (ffloor[numffloors].slope)
					frontsector->hasslope = true;

				ffloor[numffloors].height = heightcheck;
				ffloor[numffloors].ffloor = rover;
				numffloors++;
			}
			if (numffloors >= MAXFFLOORS)
				break;
			ffloor[numffloors].plane = NULL;
			ffloor[numffloors].polyobj = NULL;

			heightcheck = P_GetFFloorTopZAt(rover, viewx, viewy);

			planecenterz = P_GetFFloorTopZAt(rover, frontsector->soundorg.x, frontsector->soundorg.y);
			if (planecenterz >= floorcenterz
				&& planecenterz <= ceilingcenterz
				&& ((viewz > heightcheck && (rover->fofflags & FOF_BOTHPLANES || !(rover->fofflags & FOF_INVERTPLANES)))
				|| (viewz < heightcheck && (rover->fofflags & FOF_BOTHPLANES || rover->fofflags & FOF_INVERTPLANES))))
			{
				sectordamage_t damage = fof_top_damage();

				if (!damage && !visible)
				{
					rover->norender = leveltime; // Tell R_StoreWallRange to skip this
					continue;
				}

				light = R_GetPlaneLight(frontsector, planecenterz, viewz < heightcheck);

				ffloor[numffloors].plane = R_FindPlane(
					*rover->topheight, *rover->toppic,
					*frontsector->lightlist[light].lightlevel, *rover->topxoffs, *rover->topyoffs, *rover->topangle,
					*frontsector->lightlist[light].extra_colormap, rover, NULL, *rover->t_slope,
					R_NoEncore(rover->master->frontsector, &levelflats[*rover->toppic], false),
					R_IsRipplePlane(rover->master->frontsector, rover, false),
					false, frontsector, damage
				);

				ffloor[numffloors].slope = *rover->t_slope;

				// Tell the renderer this sector has slopes in it.
				if (ffloor[numffloors].slope)
					frontsector->hasslope = true;

				ffloor[numffloors].height = heightcheck;
				ffloor[numffloors].ffloor = rover;
				numffloors++;
			}
		}
	}

	// Polyobjects have planes, too!
	if (sub->polyList)
	{
		polyobj_t *po = sub->polyList;
		sector_t *polysec;

		auto poly_damage = [&](auto& f) { return polysec->flags & POF_SOLID ? f(polysec) : sector_damage(polysec); };
		auto poly_top_damage = [&] { return poly_damage(floor_damage); };
		auto poly_bottom_damage = [&] { return poly_damage(ceiling_damage); };

		while (po)
		{
			if (numffloors >= MAXFFLOORS)
				break;

			if (!(po->flags & POF_RENDERPLANES)) // Don't draw planes
			{
				po = (polyobj_t *)(po->link.next);
				continue;
			}

			polysec = po->lines[0]->backsector;
			ffloor[numffloors].plane = NULL;

			if (polysec->floorheight <= ceilingcenterz
				&& polysec->floorheight >= floorcenterz
				&& (viewz < polysec->floorheight))
			{
				light = R_GetPlaneLight(frontsector, polysec->floorheight, viewz < polysec->floorheight);

				ffloor[numffloors].plane = R_FindPlane(
					polysec->floorheight, polysec->floorpic,
					(light == -1 ? frontsector->lightlevel : *frontsector->lightlist[light].lightlevel), polysec->floor_xoffs, polysec->floor_yoffs,
					polysec->floorpic_angle-po->angle,
					(light == -1 ? frontsector->extra_colormap : *frontsector->lightlist[light].extra_colormap), NULL, po,
					NULL, // will ffloors be slopable eventually?
					R_NoEncore(polysec, &levelflats[polysec->floorpic], false),
					false, /* TODO: wet polyobjects? */
					true, frontsector, poly_bottom_damage()
				);

				ffloor[numffloors].height = polysec->floorheight;
				ffloor[numffloors].polyobj = po;
				ffloor[numffloors].slope = NULL;
				//ffloor[numffloors].ffloor = rover;
				po->visplane = ffloor[numffloors].plane;
				numffloors++;
			}

			if (numffloors >= MAXFFLOORS)
				break;

			ffloor[numffloors].plane = NULL;

			if (polysec->ceilingheight >= floorcenterz
				&& polysec->ceilingheight <= ceilingcenterz
				&& (viewz > polysec->ceilingheight))
			{
				light = R_GetPlaneLight(frontsector, polysec->floorheight, viewz < polysec->floorheight);

				ffloor[numffloors].plane = R_FindPlane(
					polysec->ceilingheight, polysec->ceilingpic,
					(light == -1 ? frontsector->lightlevel : *frontsector->lightlist[light].lightlevel), polysec->ceiling_xoffs, polysec->ceiling_yoffs, polysec->ceilingpic_angle-po->angle,
					(light == -1 ? frontsector->extra_colormap : *frontsector->lightlist[light].extra_colormap), NULL, po,
					NULL, // will ffloors be slopable eventually?
					R_NoEncore(polysec, &levelflats[polysec->ceilingpic], true),
					false, /* TODO: wet polyobjects? */
					false, frontsector, poly_top_damage()
				);

				ffloor[numffloors].polyobj = po;
				ffloor[numffloors].height = polysec->ceilingheight;
				ffloor[numffloors].slope = NULL;
				//ffloor[numffloors].ffloor = rover;
				po->visplane = ffloor[numffloors].plane;
				numffloors++;
			}

			po = (polyobj_t *)(po->link.next);
		}
	}

   // killough 9/18/98: Fix underwater slowdown, by passing real sector
   // instead of fake one. Improve sprite lighting by basing sprite
   // lightlevels on floor & ceiling lightlevels in the surrounding area.
   //
   // 10/98 killough:
   //
   // NOTE: TeamTNT fixed this bug incorrectly, messing up sprite lighting!!!
   // That is part of the 242 effect!!!  If you simply pass sub->sector to
   // the old code you will not get correct lighting for underwater sprites!!!
   // Either you must pass the fake sector and handle validcount here, on the
   // real sector, or you must account for the lighting in some other way,
   // like passing it as an argument.
	R_AddSprites(sub->sector, (floorlightlevel+ceilinglightlevel)/2);

	firstseg = NULL;

	// haleyjd 02/19/06: draw polyobjects before static lines
	if (sub->polyList)
		R_AddPolyObjects(sub);

	while (count--)
	{
//		CONS_Debug(DBG_GAMELOGIC, "Adding normal line %d...(%d)\n", line->linedef-lines, leveltime);
		if (!line->glseg && !line->polyseg) // ignore segs that belong to polyobjects
			R_AddLine(line);
		line++;
		curline = NULL; /* cph 2001/11/18 - must clear curline now we're done with it, so stuff doesn't try using it for other things */
	}
}

//
// R_Prep3DFloors
//
// This function creates the lightlists that the given sector uses to light
// floors/ceilings/walls according to the 3D floors.
void R_Prep3DFloors(sector_t *sector)
{
	ffloor_t *rover;
	ffloor_t *best;
	fixed_t bestheight, maxheight;
	INT32 count, i;
	sector_t *sec;
	pslope_t *bestslope = NULL;
	fixed_t heighttest; // I think it's better to check the Z height at the sector's center
	                    // than assume unsloped heights are accurate indicators of order in sloped sectors. -Red

	count = 1;
	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if ((rover->fofflags & FOF_EXISTS) && (!(rover->fofflags & FOF_NOSHADE)
			|| (rover->fofflags & FOF_CUTLEVEL) || (rover->fofflags & FOF_CUTSPRITES)))
		{
			count++;
			if (rover->fofflags & FOF_DOUBLESHADOW)
				count++;
		}
	}

	if (count != sector->numlights)
	{
		Z_Free(sector->lightlist);
		sector->lightlist = static_cast<lightlist_t*>(Z_Calloc(sizeof (*sector->lightlist) * count, PU_LEVEL, NULL));
		sector->numlights = count;
	}
	else
		memset(sector->lightlist, 0, sizeof (lightlist_t) * count);

	heighttest = P_GetSectorCeilingZAt(sector, sector->soundorg.x, sector->soundorg.y);

	sector->lightlist[0].height = heighttest + 1;
	sector->lightlist[0].slope = sector->c_slope;
	sector->lightlist[0].lightlevel = &sector->lightlevel;
	sector->lightlist[0].caster = NULL;
	sector->lightlist[0].extra_colormap = &sector->extra_colormap;
	sector->lightlist[0].flags = 0;

	maxheight = INT32_MAX;
	for (i = 1; i < count; i++)
	{
		bestheight = INT32_MAX * -1;
		best = NULL;
		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			rover->lastlight = 0;
			if (!(rover->fofflags & FOF_EXISTS) || (rover->fofflags & FOF_NOSHADE
				&& !(rover->fofflags & FOF_CUTLEVEL) && !(rover->fofflags & FOF_CUTSPRITES)))
			continue;

			heighttest = P_GetFFloorTopZAt(rover, sector->soundorg.x, sector->soundorg.y);

			if (heighttest > bestheight && heighttest < maxheight)
			{
				best = rover;
				bestheight = heighttest;
				bestslope = *rover->t_slope;
				continue;
			}
			if (rover->fofflags & FOF_DOUBLESHADOW) {
				heighttest = P_GetFFloorBottomZAt(rover, sector->soundorg.x, sector->soundorg.y);

				if (heighttest > bestheight
					&& heighttest < maxheight)
				{
					best = rover;
					bestheight = heighttest;
					bestslope = *rover->b_slope;
					continue;
				}
			}
		}
		if (!best)
		{
			sector->numlights = i;
			return;
		}

		sector->lightlist[i].height = maxheight = bestheight;
		sector->lightlist[i].caster = best;
		sector->lightlist[i].flags = best->fofflags;
		sector->lightlist[i].slope = bestslope;
		sec = &sectors[best->secnum];

		if (best->fofflags & FOF_NOSHADE)
		{
			sector->lightlist[i].lightlevel = sector->lightlist[i-1].lightlevel;
			sector->lightlist[i].extra_colormap = sector->lightlist[i-1].extra_colormap;
		}
		else if (best->fofflags & FOF_COLORMAPONLY)
		{
			sector->lightlist[i].lightlevel = sector->lightlist[i-1].lightlevel;
			sector->lightlist[i].extra_colormap = &sec->extra_colormap;
		}
		else
		{
			sector->lightlist[i].lightlevel = best->toplightlevel;
			sector->lightlist[i].extra_colormap = &sec->extra_colormap;
		}

		if (best->fofflags & FOF_DOUBLESHADOW)
		{
			heighttest = P_GetFFloorBottomZAt(best, sector->soundorg.x, sector->soundorg.y);
			if (bestheight == heighttest) ///TODO: do this in a more efficient way -Red
			{
				sector->lightlist[i].lightlevel = sector->lightlist[best->lastlight].lightlevel;
				sector->lightlist[i].extra_colormap =
					sector->lightlist[best->lastlight].extra_colormap;
			}
			else
				best->lastlight = i - 1;
		}
	}
}

INT32 R_GetPlaneLight(sector_t *sector, fixed_t planeheight, boolean underside)
{
	INT32 i;

	if (!underside)
	{
		for (i = 1; i < sector->numlights; i++)
			if (sector->lightlist[i].height <= planeheight)
				return i - 1;

		return sector->numlights - 1;
	}

	for (i = 1; i < sector->numlights; i++)
		if (sector->lightlist[i].height < planeheight)
			return i - 1;

	return sector->numlights - 1;
}

//
// RenderBSPNode
// Renders all subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.
//
// killough 5/2/98: reformatted, removed tail recursion

void R_RenderBSPNode(INT32 bspnum)
{
	node_t *bsp;
	INT32 side;

	ZoneScoped;

	ps_numbspcalls++;

	while (!(bspnum & NF_SUBSECTOR))  // Found a subsector?
	{
		bsp = &nodes[bspnum];

		// Decide which side the view point is on.
		side = R_PointOnSide(viewx, viewy, bsp);
		// Recursively divide front space.
		R_RenderBSPNode(bsp->children[side]);

		// Possibly divide back space.

		if (!R_CheckBBox(bsp->bbox[side^1]))
			return;

		bspnum = bsp->children[side^1];
	}

	// PORTAL CULLING
	if (portalcullsector) {
		sector_t *sect = subsectors[bspnum & ~NF_SUBSECTOR].sector;
		if (sect != portalcullsector)
			return;
		portalcullsector = NULL;
	}

	bspnum = (bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR);
	R_Subsector(bspnum);

	if (current_node_cache)
	{
		current_node_cache->push_back(bspnum);
	}
}

static bool render_cache(size_t cachenum)
{
	if (node_cache.empty() && !cv_debugrender_freezebsp.value)
	{
		current_node_cache = nullptr;
		return false;
	}

	if (!cv_debugrender_freezebsp.value)
	{
		// free cache
		node_cache = {};
		current_node_cache = nullptr;
		return false;
	}

	if (node_cache.size() <= cachenum)
	{
		node_cache.resize(cachenum + 1);
		current_node_cache = &node_cache[cachenum];
		return false;
	}

	for (INT32 bspnum : node_cache[cachenum])
		R_Subsector(bspnum);

	return true;
}

void R_RenderFirstBSPNode(size_t cachenum)
{
	if (!render_cache(cachenum))
		R_RenderBSPNode((INT32)numnodes - 1);
}
