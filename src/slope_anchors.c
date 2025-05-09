// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \brief Charyb's vertex slope anchors.
///        This file is self contained to avoid a Big Large merge conflict.

#include "taglist.h"

/*
FIXME
FIXME
FIXME
FIXME	WHEN 2.2 MERGE IS OVER, REFACTOR A LOT OF THE CODE IN P_SLOPES.C AND
FIXME	MAKE THIS NOT REDUNDANT.
FIXME
FIXME
FIXME
*/

struct anchor_list
{
	mapthing_t     ** anchors;
	const vertex_t ** points;
	fixed_t         * closeness;
	size_t            count;
};

struct anchor_list   floor_anchors;
struct anchor_list ceiling_anchors;

static void * new_list (size_t n) {
	return Z_Malloc(n, PU_LEVEL, NULL);
}

static void make_new_anchor_list (struct anchor_list * list) {
	list->anchors   = new_list(list->count * sizeof *list->anchors);
	list->points    = new_list(list->count * sizeof *list->points);
	list->closeness = new_list(list->count * sizeof *list->closeness);
}

static void allocate_anchors (void) {
	size_t i;

	floor_anchors.count = 0;
	ceiling_anchors.count = 0;

	for (i = 0; i < nummapthings; ++i)
	{
		switch (mapthings[i].type)
		{
			case FLOOR_SLOPE_THING:
				floor_anchors.count++;
				break;

			case CEILING_SLOPE_THING:
				ceiling_anchors.count++;
				break;
		}
	}

	make_new_anchor_list(&floor_anchors);
	make_new_anchor_list(&ceiling_anchors);
}

static void
compare_vertex_distance
(
		const vertex_t ** nearest,
		fixed_t         * nearest_distance,
		const fixed_t     origin_x,
		const fixed_t     origin_y,
		const vertex_t  * v
){
	const fixed_t distance = abs(P_AproxDistance
			(
				origin_x - v->x,
				origin_y - v->y
			));

	if (distance < (*nearest_distance))
	{
		(*nearest) = v;
		(*nearest_distance) = distance;
	}
}

static const vertex_t *
nearest_point
(
		fixed_t           * closeness,
		mapthing_t        * a,
		const sector_t    * sector
){
	const fixed_t x = a->x << FRACBITS;
	const fixed_t y = a->y << FRACBITS;

	const vertex_t * v = NULL;/* shut compiler up, should never be NULL */

	size_t i;

	(*closeness) = INT32_MAX;

	for (i = 0; i < sector->linecount; ++i)
	{
		compare_vertex_distance(&v, closeness, x, y, sector->lines[i]->v1);
		compare_vertex_distance(&v, closeness, x, y, sector->lines[i]->v2);
	}

	return v;
}

static INT16
anchor_height
(
		const mapthing_t * a,
		const sector_t   * s
){
	const fixed_t x = a->x << FRACBITS;
	const fixed_t y = a->y << FRACBITS;

	if (a->options & MTF_OBJECTFLIP)
	{
		return ( P_GetSectorCeilingZAt(s, x, y) >> FRACBITS ) - a->z;
	}
	else
	{
		return ( P_GetSectorFloorZAt(s, x, y) >> FRACBITS ) + a->z;
	}
}

static void
set_anchor
(
		struct anchor_list * list,
		mapthing_t         * a
){
	const subsector_t * sub = R_PointInSubsector
		(
				a->x << FRACBITS,
				a->y << FRACBITS
		);

	const vertex_t * v;

	fixed_t closeness;

	v = nearest_point(&closeness, a, sub->sector);

	a->x = ( v->x >> FRACBITS );
	a->y = ( v->y >> FRACBITS );

	a->z = anchor_height(a, sub->sector);

	list->anchors  [list->count] = a;
	list->points   [list->count] = v;
	list->closeness[list->count] = closeness;

	list->count++;
}

static void build_anchors (void) {
	size_t i;

	floor_anchors.count = 0;
	ceiling_anchors.count = 0;

	for (i = 0; i < nummapthings; ++i)
	{
		switch (mapthings[i].type)
		{
			case FLOOR_SLOPE_THING:
				set_anchor(&floor_anchors, &mapthings[i]);
				break;

			case CEILING_SLOPE_THING:
				set_anchor(&ceiling_anchors, &mapthings[i]);
				break;
		}
	}
}

static void
get_anchor
(
		mapthing_t               **      anchors,
		fixed_t                     distances[3],
		const struct anchor_list  * list,
		const INT32                 group,
		const vertex_t            * v
){
	size_t i;

	int k;

	for (i = 0; i < list->count; ++i)
	{
		if (list->points[i] == v && list->anchors[i]->thing_args[0] == group)
		{
			for (k = 0; k < 3; ++k)
			{
				if (list->closeness[i] < distances[k])
				{
					if (k == 0)
					{
						distances[2] = distances[1];
						distances[1] = distances[0];

						anchors  [2] = anchors  [1];
						anchors  [1] = anchors  [0];
					}
					else if (k == 1)
					{
						distances[2] = distances[1];
						anchors  [2] = anchors  [1];
					}

					distances[k] = list->closeness[i];

					anchors[k] = list->anchors[i];

					break;
				}
				else if (list->anchors[i] == anchors[k])
				{
					break;
				}
			}
		}
	}
}

static void
get_sector_anchors
(
		mapthing_t               **      anchors,
		fixed_t                     distances[3],
		const struct anchor_list  * list,
		const INT32                 group,
		const sector_t            * sector
){
	size_t i;

	for (i = 0; i < sector->linecount; ++i)
	{
		get_anchor(anchors, distances, list, group, sector->lines[i]->v1);
		get_anchor(anchors, distances, list, group, sector->lines[i]->v2);
	}
}

static mapthing_t **
find_closest_anchors
(
		const sector_t           * sector,
		const struct anchor_list * list,
		const INT32                group
){
	fixed_t distances[3] = { INT32_MAX, INT32_MAX, INT32_MAX };

	mapthing_t ** anchors;

	int last = 0;

	size_t i;

	if (list->count < 3)
	{
		I_Error("At least three slope anchors are required to make a slope.");
	}

	anchors = Z_Malloc(3 * sizeof *anchors, PU_LEVEL, NULL);

	if (sector->numattached > 0)
	{
		for (i = 0; i < sector->numattached; ++i)
		{
			get_sector_anchors
				(anchors, distances, list, group, &sectors[sector->attached[i]]);
		}
	}
	else
	{
		get_sector_anchors(anchors, distances, list, group, sector);
	}

	if (distances[2] < INT32_MAX)
	{
		return anchors;
	}

	if (distances[1] < INT32_MAX)
		last = 2;
	else if (distances[0] < INT32_MAX)
		last = 1;
	else
		last = 0;

	if (sector->numattached > 0)
	{
		CONS_Printf("\nSearched for anchors in sectors...\n\n");

		for (i = 0; i < sector->numattached; ++i)
		{
			CONS_Printf("#%s\n", sizeu1 (sector->attached[i]));
		}

		I_Error(
				"(Control Sector #%s)"
				" Slope requires anchors (with group ID %d)"
				" near 3 of its target sectors' vertices (%d found)"

				"\n\nCheck the log to see which sectors were searched.",

				sizeu1 (sector - sectors),
				group,
				last
		);
	}
	else
	{
		I_Error(
				"(Sector #%s)"
				" Slope requires anchors (with group ID %d)"
				" near 3 of its vertices (%d found)",

				sizeu1 (sector - sectors),
				group,
				last
		);
	}
}

static pslope_t *
new_vertex_slope
(
		mapthing_t  ** anchors,
		const INT16    flags
){
	pslope_t * slope = Z_Calloc(sizeof (pslope_t), PU_LEVEL, NULL);
	const vector3_t anchorVertices[3] = {
		{anchors[0]->x << FRACBITS, anchors[0]->y << FRACBITS, anchors[0]->z << FRACBITS},
		{anchors[1]->x << FRACBITS, anchors[1]->y << FRACBITS, anchors[1]->z << FRACBITS},
		{anchors[2]->x << FRACBITS, anchors[2]->y << FRACBITS, anchors[2]->z << FRACBITS}
	};

	if (flags & TMSAF_NOPHYSICS)
	{
		slope->flags |= SL_NOPHYSICS;
	}

	if (flags & TMSAF_DYNAMIC)
	{
		slope->flags |= SL_DYNAMIC;
	}

	P_ReconfigureViaVertexes(slope, anchorVertices[0], anchorVertices[1], anchorVertices[2]);
	//slope->refpos = 5;

	// Add to the slope list
	slope->next = slopelist;
	slopelist = slope;

	slopecount++;
	slope->id = slopecount;

	return slope;
}

static mapthing_t **
flip_slope
(
		mapthing_t     ** origin,
		const sector_t  * sector
){
	mapthing_t  * copy    = Z_Malloc(3 * sizeof (mapthing_t),   PU_LEVEL, NULL);
	mapthing_t ** anchors = Z_Malloc(3 * sizeof (mapthing_t *), PU_LEVEL, NULL);

	size_t i;

	for (i = 0; i < 3; ++i)
	{
		memcpy(&copy[i], origin[i], sizeof copy[i]);

		copy[i].options ^= MTF_OBJECTFLIP;
		copy[i].z = anchor_height(&copy[i], sector);

		anchors[i] = &copy[i];
	}

	return anchors;
}

static void
slope_sector
(
		pslope_t                ** slope,
		pslope_t                ** alt,
		sector_t                 * sector,
		const INT16                flags,
		const struct anchor_list * list,
		const INT32                group
){
	mapthing_t ** anchors = find_closest_anchors(sector, list, group);

	if (anchors != NULL)
	{
		(*slope) = new_vertex_slope(anchors, flags);

		/* invert slope to opposite side */
		if (flags & TMSAF_MIRROR)
		{
			(*alt) = new_vertex_slope(flip_slope(anchors, sector), flags);
		}

		sector->hasslope = true;
	}
}

static void
make_anchored_slope
(
		const line_t * line,
		const int      plane
){
	INT16 flags = line->args[1];

	const int side = ( flags & TMSAF_BACKSIDE ) != 0;

	sector_t   *  s;

	INT32 group = line->args[2];

	if (side == 0 || (line->flags & ML_TWOSIDED))
	{
		s = sides[line->sidenum[side]].sector;

		if (plane == (TMSA_FLOOR|TMSA_CEILING))
		{
			flags &= ~TMSAF_MIRROR;
		}

		if (plane & TMSA_FLOOR)
		{
			slope_sector
				(&s->f_slope, &s->c_slope, s, flags, &floor_anchors, group);
		}

		if (plane & TMSA_CEILING)
		{
			slope_sector
				(&s->c_slope, &s->f_slope, s, flags, &ceiling_anchors, group);
		}
	}
}

static void
make_anchored_slope_from_sector
(
		sector_t * s,
		const int      plane
){
	INT16 flags = s->args[1];
	INT32 group = s->args[2];

	if (plane == (TMSA_FLOOR|TMSA_CEILING))
	{
		flags &= ~TMSAF_MIRROR;
	}

	if (plane & TMSA_FLOOR)
	{
		slope_sector
			(&s->f_slope, &s->c_slope, s, flags, &floor_anchors, group);
	}

	if (plane & TMSA_CEILING)
	{
		slope_sector
			(&s->c_slope, &s->f_slope, s, flags, &ceiling_anchors, group);
	}
}

static void P_BuildSlopeAnchorList (void) {
	allocate_anchors();
	build_anchors();
}

static void P_SetupAnchoredSlopes (void) {
	size_t i;

	for (i = 0; i < numlines; ++i)
	{
		if (lines[i].special == LT_SLOPE_ANCHORS)
		{
			int plane = (lines[i].args[0] & (TMSA_FLOOR|TMSA_CEILING));

			if (plane == 0)
			{
				CONS_Alert(CONS_WARNING, "Slope anchor linedef %s has no planes set.\n", sizeu1(i));
				continue;
			}

			make_anchored_slope(&lines[i], plane);
		}
	}

	for (i = 0; i < numsectors; ++i)
	{
		if (sectors[i].action == LT_SLOPE_ANCHORS)
		{
			int plane = (sectors[i].args[0] & (TMSA_FLOOR|TMSA_CEILING));

			if (plane == 0)
			{
				CONS_Alert(CONS_WARNING, "Slope anchor sector %s has no planes set.\n", sizeu1(i));
				continue;
			}

			make_anchored_slope_from_sector(&sectors[i], plane);
		}
	}
}
