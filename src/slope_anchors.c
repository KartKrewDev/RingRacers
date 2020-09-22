// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by James R.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \brief Charyb's vertex slope anchors.
///        This file is self contained to avoid a Big Large merge conflict.

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
	size_t            count;
};

struct anchor_list   floor_anchors;
struct anchor_list ceiling_anchors;

static void * new_list (size_t n) {
	return Z_Malloc(n, PU_LEVEL, NULL);
}

static void make_new_anchor_list (struct anchor_list * list) {
	list->anchors = new_list(list->count * sizeof *list->anchors);
	list->points  = new_list(list->count * sizeof *list->points);
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
		mapthing_t        * a,
		const sector_t    * sector
){
	const fixed_t x = a->x << FRACBITS;
	const fixed_t y = a->y << FRACBITS;

	const vertex_t * nearest = NULL;/* shut compiler up, should never be NULL */
	fixed_t          nearest_distance = INT32_MAX;

	size_t i;

	for (i = 0; i < sector->linecount; ++i)
	{
		compare_vertex_distance(&nearest, &nearest_distance, x, y, sector->lines[i]->v1);
		compare_vertex_distance(&nearest, &nearest_distance, x, y, sector->lines[i]->v2);
	}

	return nearest;
}

static INT16
anchor_height
(
		const mapthing_t * a,
		const sector_t   * s
){
	if (a->extrainfo)
	{
		return a->options;
	}
	else
	{
		INT16 z = ( a->options >> ZSHIFT );

		if (a->options & MTF_OBJECTFLIP)
		{
			return ( s->ceilingheight >> FRACBITS ) - z;
		}
		else
		{
			return ( s->floorheight >> FRACBITS ) + z;
		}
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

	a->z = anchor_height(a, sub->sector);

	v = nearest_point(a, sub->sector);

	a->x = ( v->x >> FRACBITS );
	a->y = ( v->y >> FRACBITS );

	list->anchors[list->count] = a;
	list->points [list->count] = v;

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

static int
get_anchor
(
		const vertex_t            * points[3],
		mapthing_t               **      anchors,
		int                       * last_anchor,
		const struct anchor_list  * list,
		const vertex_t            * v
){
	size_t i;

	if (
			v != points[0] &&
			v != points[1] &&
			v != points[2]
	){
		for (i = 0; i < list->count; ++i)
		{
			if (list->points[i] == v)
			{
				points [*last_anchor] = v;
				anchors[*last_anchor] = list->anchors[i];

				if (++(*last_anchor) == 3)
				{
					return 1;
				}
				else
				{
					return 0;
				}
			}
		}
	}

	return 0;
}

static mapthing_t **
find_closest_anchors
(
		const sector_t           * sector,
		const struct anchor_list * list
){
	const vertex_t * points[3] = {0};

	mapthing_t ** anchors;

	size_t i;

	int last = 0;

	if (list->count < 3)
	{
		I_Error("At least three slope anchors are required to make a slope.");
	}

	anchors = Z_Malloc(3 * sizeof *anchors, PU_LEVEL, NULL);

	for (i = 0; i < sector->linecount; ++i)
	{
		if (get_anchor(points, anchors, &last, list, sector->lines[i]->v1) != 0)
		{
			return anchors;
		}

		if (get_anchor(points, anchors, &last, list, sector->lines[i]->v2) != 0)
		{
			return anchors;
		}
	}

	I_Error(
			"(Sector #%s)"
			" Slope requires anchors near 3 of its vertices (%d found)",

			sizeu1 (sector - sectors),
			last
	);
}

static pslope_t *
new_vertex_slope
(
		mapthing_t  ** anchors,
		const INT16    flags
){
	pslope_t * slope = Z_Calloc(sizeof (pslope_t), PU_LEVEL, NULL);

	slope->flags = SL_VERTEXSLOPE;

	if (flags & ML_NOSONIC)
	{
		slope->flags |= SL_NOPHYSICS;
	}

	if (( flags & ML_NOTAILS ) == 0)
	{
		slope->flags |= SL_NODYNAMIC;
	}

	slope->vertices = anchors;

	P_ReconfigureVertexSlope(slope);
	slope->refpos = 5;

	// Add to the slope list
	slope->next = slopelist;
	slopelist = slope;

	slopecount++;
	slope->id = slopecount;

	return slope;
}

static void
make_anchored_slope
(
		const line_t * line,
		const int      plane
){
	enum
	{
		FLOOR   = 0x1,
		CEILING = 0x2,
	};

	const INT16 flags = line->flags;

	const int side = ( flags & ML_NOCLIMB ) != 0;

	sector_t   *  sector;
	mapthing_t ** anchors;

	if (side == 0 || flags & ML_TWOSIDED)
	{
		sector = sides[line->sidenum[side]].sector;

		if (plane & FLOOR)
		{
			anchors = find_closest_anchors(sector, &floor_anchors);
			sector->f_slope = new_vertex_slope(anchors, flags);
		}

		if (plane & CEILING)
		{
			anchors = find_closest_anchors(sector, &ceiling_anchors);
			sector->c_slope = new_vertex_slope(anchors, flags);
		}

		sector->hasslope = true;
	}
}

static void P_BuildSlopeAnchorList (void) {
	allocate_anchors();
	build_anchors();
}

static void P_SetupAnchoredSlopes (void) {
	enum
	{
		FLOOR   = 0x1,
		CEILING = 0x2,
	};

	size_t i;

	for (i = 0; i < numlines; ++i)
	{
		if (lines[i].special == LT_SLOPE_ANCHORS_FLOOR)
		{
			make_anchored_slope(&lines[i], FLOOR);
		}
		else if (lines[i].special == LT_SLOPE_ANCHORS_CEILING)
		{
			make_anchored_slope(&lines[i], CEILING);
		}
		else if (lines[i].special == LT_SLOPE_ANCHORS)
		{
			make_anchored_slope(&lines[i], FLOOR|CEILING);
		}
	}
}
