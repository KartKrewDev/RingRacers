// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2004 by Stephen McGranahan.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_slopes.c
/// \brief ZDoom + Eternity Engine Slopes, ported and enhanced by Kalaron

#include "d_think.h"
#include "doomdef.h"
#include "g_demo.h"
#include "r_defs.h"
#include "r_state.h"
#include "m_bbox.h"
#include "z_zone.h"
#include "p_local.h"
#include "p_spec.h"
#include "p_slopes.h"
#include "p_setup.h"
#include "r_main.h"
#include "p_maputl.h"
#include "w_wad.h"
#include "r_fps.h"
#include "k_kart.h" // K_PlayerEBrake
#include "m_easing.h"

pslope_t *slopelist = NULL;
UINT16 slopecount = 0;

static void P_BuildSlopeAnchorList (void);
static void P_SetupAnchoredSlopes  (void);

// Calculate light
void P_UpdateSlopeLightOffset(pslope_t *slope)
{
	const UINT8 contrast = maplighting.contrast;

	fixed_t contrastFixed = ((fixed_t)contrast) * FRACUNIT;
	fixed_t zMul = FRACUNIT;
	fixed_t light = FRACUNIT;
	fixed_t extralight = 0;

	if (slope->normal.z == 0)
	{
		slope->lightOffset = slope->hwLightOffset = 0;
		return;
	}

	if (maplighting.directional == true)
	{
		fixed_t nX = -slope->normal.x;
		fixed_t nY = -slope->normal.y;
		fixed_t nLen = FixedHypot(nX, nY);

		if (nLen == 0)
		{
			slope->lightOffset = slope->hwLightOffset = 0;
			return;
		}

		nX = FixedDiv(nX, nLen);
		nY = FixedDiv(nY, nLen);

		/*
		if (slope is ceiling)
		{
			// There is no good way to calculate this condition here.
			// We reverse it in R_FindPlane now.
			nX = -nX;
			nY = -nY;
		}
		*/

		light = FixedMul(nX, FINECOSINE(maplighting.angle >> ANGLETOFINESHIFT))
			+ FixedMul(nY, FINESINE(maplighting.angle >> ANGLETOFINESHIFT));
		light = (light + FRACUNIT) / 2;
	}
	else
	{
		light = FixedDiv(R_PointToAngle2(0, 0, abs(slope->normal.y), abs(slope->normal.x)), ANGLE_90);
	}

	zMul = min(FRACUNIT, abs(slope->zdelta)*3/2); // *3/2, to make 60 degree slopes match walls.
	contrastFixed = FixedMul(contrastFixed, zMul);

	extralight = -contrastFixed + FixedMul(light, contrastFixed * 2);

	// Between -2 and 2 for software, -16 and 16 for hardware
	slope->lightOffset = FixedFloor((extralight / 8) + (FRACUNIT / 2)) / FRACUNIT;
	slope->hwLightOffset = FixedFloor(extralight + (FRACUNIT / 2)) / FRACUNIT;
}

// Calculate line normal
void P_CalculateSlopeNormal(pslope_t *slope) {
	slope->normal.z = FINECOSINE(slope->zangle>>ANGLETOFINESHIFT);
	slope->normal.x = FixedMul(FINESINE(slope->zangle>>ANGLETOFINESHIFT), slope->d.x);
	slope->normal.y = FixedMul(FINESINE(slope->zangle>>ANGLETOFINESHIFT), slope->d.y);
	P_UpdateSlopeLightOffset(slope);
}

// Calculate slope's high & low z
static void P_CalculateLineSlopeHighLow(pslope_t *slope, line_t *line, boolean ceiling, boolean back)
{
	// To find the real highz/lowz of a slope, you need to check all the vertexes
	// in the slope's sector with P_GetZAt to get the REAL lowz & highz
	// Although these slopes are set by floorheights the ANGLE is what a slope is,
	// so technically any slope can extend on forever (they are just bound by sectors)
	// *You can use sourceline as a reference to see if two slopes really are the same

	size_t l;

	sector_t *checksector = back ? line->backsector : line->frontsector;

	// Default points for high and low
	fixed_t highest = INT32_MIN;
	fixed_t lowest = INT32_MAX;

	// Now check to see what the REAL high and low points of the slope inside the sector
	// TODO: Is this really needed outside of FOFs? -Red

	for (l = 0; l < checksector->linecount; l++)
	{
		pslope_t *checkslope = ceiling ? checksector->c_slope : checksector->f_slope;
		fixed_t height = P_GetSlopeZAt(checkslope, checksector->lines[l]->v1->x, checksector->lines[l]->v1->y);

		if (height > highest)
			highest = height;

		if (height < lowest)
			lowest = height;
	}

	// Sets extra clipping data for the slope
	slope->highz = highest;
	slope->lowz = lowest;
}

/// Setup slope via 3 vertexes.
void P_ReconfigureViaVertexes (pslope_t *slope, const vector3_t v1, const vector3_t v2, const vector3_t v3)
{
	vector3_t vec1, vec2;

	memset(slope->constants, 0, sizeof(slope->constants));

	// Set origin.
	FV3_Copy(&slope->o, &v1);

	// Get slope's normal.
	FV3_SubEx(&v2, &v1, &vec1);
	FV3_SubEx(&v3, &v1, &vec2);

	// Set some defaults for a non-sloped "slope"
	if (vec1.z == 0 && vec2.z == 0)
	{
		slope->zangle = slope->xydirection = 0;
		slope->zdelta = slope->d.x = slope->d.y = 0;
		slope->normal.x = slope->normal.y = 0;
		slope->normal.z = FRACUNIT;
	}
	else
	{
		/// \note Using fixed point for vectorial products easily leads to overflows so we work around by downscaling them.
		fixed_t m = max(
			max(max(abs(vec1.x), abs(vec1.y)), abs(vec1.z)),
			max(max(abs(vec2.x), abs(vec2.y)), abs(vec2.z))
		) >> 5; // shifting right by 5 is good enough.

		FV3_Cross(
				FV3_Divide(&vec1, m),
				FV3_Divide(&vec2, m),
				&slope->normal
				);

		// NOTE: FV3_Magnitude() doesn't work properly in some cases, and chaining FixedHypot() seems to give worse results.
		m = R_PointToDist2(0, 0, R_PointToDist2(0, 0, slope->normal.x, slope->normal.y), slope->normal.z);

		// Invert normal if it's facing down.
		if (slope->normal.z < 0)
			m = -m;

		FV3_Divide(&slope->normal, m);

		// Get direction vector
		m = FixedHypot(slope->normal.x, slope->normal.y);
		slope->d.x = -FixedDiv(slope->normal.x, m);
		slope->d.y = -FixedDiv(slope->normal.y, m);

		// Z delta
		slope->zdelta = FixedDiv(m, slope->normal.z);

		// Get angles
		slope->xydirection = R_PointToAngle2(0, 0, slope->d.x, slope->d.y)+ANGLE_180;
		slope->zangle = R_PointToAngle2(0, 0, FRACUNIT, -slope->zdelta);
	}

	P_UpdateSlopeLightOffset(slope);
}

/// Setup slope via constants.
static void ReconfigureViaConstants (pslope_t *slope, const fixed_t a, const fixed_t b, const fixed_t c, const fixed_t d)
{
	fixed_t m;
	fixed_t o = 0;
	vector3_t *normal = &slope->normal;

	slope->constants[0] = a;
	slope->constants[1] = b;
	slope->constants[2] = c;
	slope->constants[3] = d;

	if (c)
		o = abs(c) <= FRACUNIT ? -FixedMul(d, FixedDiv(FRACUNIT, c)) : -FixedDiv(d, c);

	// Set origin.
	FV3_Load(&slope->o, 0, 0, o);

	// Get slope's normal.
	FV3_Load(normal, a, b, c);
	FV3_Normalize(normal);

	// Invert normal if it's facing down.
	if (normal->z < 0)
		FV3_Negate(normal);

	// Get direction vector
	m = FixedHypot(normal->x, normal->y);
	slope->d.x = -FixedDiv(normal->x, m);
	slope->d.y = -FixedDiv(normal->y, m);

	// Z delta
	slope->zdelta = FixedDiv(m, normal->z);

	// Get angles
	slope->xydirection = R_PointToAngle2(0, 0, slope->d.x, slope->d.y)+ANGLE_180;
	slope->zangle = R_PointToAngle2(0, 0, FRACUNIT, -slope->zdelta);

	P_UpdateSlopeLightOffset(slope);
}

/// Recalculate dynamic slopes.
void T_DynamicSlopeLine (dynlineplanethink_t* th)
{
	pslope_t* slope = th->slope;
	line_t* srcline = th->sourceline;

	fixed_t zdelta;

	switch(th->type) {
	case DP_FRONTFLOOR:
		zdelta = srcline->backsector->floorheight - srcline->frontsector->floorheight;
		slope->o.z = srcline->frontsector->floorheight;
		break;

	case DP_FRONTCEIL:
		zdelta = srcline->backsector->ceilingheight - srcline->frontsector->ceilingheight;
		slope->o.z = srcline->frontsector->ceilingheight;
		break;

	case DP_BACKFLOOR:
		zdelta = srcline->frontsector->floorheight - srcline->backsector->floorheight;
		slope->o.z = srcline->backsector->floorheight;
		break;

	case DP_BACKCEIL:
		zdelta = srcline->frontsector->ceilingheight - srcline->backsector->ceilingheight;
		slope->o.z = srcline->backsector->ceilingheight;
		break;

	default:
		return;
	}

	if (slope->zdelta != FixedDiv(zdelta, th->extent)) {
		slope->zdelta = FixedDiv(zdelta, th->extent);
		slope->zangle = R_PointToAngle2(0, 0, th->extent, -zdelta);
		P_CalculateSlopeNormal(slope);
	}
}

/// Mapthing-defined
void T_DynamicSlopeVert (dynvertexplanethink_t* th)
{
	size_t i;

	for (i = 0; i < 3; i++)
	{
		if (th->relative & (1 << i))
			th->vex[i].z = th->origvecheights[i] + (th->secs[i]->floorheight - th->origsecheights[i]);
		else
			th->vex[i].z = th->secs[i]->floorheight;
	}

	P_ReconfigureViaVertexes(th->slope, th->vex[0], th->vex[1], th->vex[2]);
}

static inline void P_AddDynLineSlopeThinker (pslope_t* slope, dynplanetype_t type, line_t* sourceline, fixed_t extent)
{
	dynlineplanethink_t* th = Z_LevelPoolCalloc(sizeof (*th));
	th->thinker.alloctype = TAT_LEVELPOOL;
	th->thinker.size = sizeof(*th);
	th->thinker.function.acp1 = (actionf_p1)T_DynamicSlopeLine;
	th->slope = slope;
	th->type = type;
	th->sourceline = sourceline;
	th->extent = extent;
	// Handle old demos as well.
	P_AddThinker(G_CompatLevel(0x000E) ? THINK_DYNSLOPEDEMO : THINK_DYNSLOPE, &th->thinker);

	// interpolation
	R_CreateInterpolator_DynSlope(&th->thinker, slope);
}

static inline void P_AddDynVertexSlopeThinker (pslope_t* slope, const INT16 tags[3], const vector3_t vx[3])
{
	dynvertexplanethink_t* th = Z_LevelPoolCalloc(sizeof (*th));
	th->thinker.alloctype = TAT_LEVELPOOL;
	th->thinker.size = sizeof(*th);
	size_t i;
	INT32 l;
	th->thinker.function.acp1 = (actionf_p1)T_DynamicSlopeVert;
	th->slope = slope;

	for (i = 0; i < 3; i++) {
		l = Tag_FindLineSpecial(799, tags[i]);
		if (l == -1)
		{
			Z_Free(th);
			return;
		}
		th->secs[i] = lines[l].frontsector;
		th->vex[i] = vx[i];
		th->origsecheights[i] = lines[l].frontsector->floorheight;
		th->origvecheights[i] = vx[i].z;
		if (lines[l].args[0])
			th->relative |= 1<<i;
	}
	// Handle old demos as well.
	P_AddThinker(G_CompatLevel(0x000E) ? THINK_DYNSLOPEDEMO : THINK_DYNSLOPE, &th->thinker);
}

/// Create a new slope and add it to the slope list.
static inline pslope_t* Slope_Add (const UINT8 flags)
{
	pslope_t *ret = Z_Calloc(sizeof(pslope_t), PU_LEVEL, NULL);
	ret->flags = flags;

	ret->next = slopelist;
	slopelist = ret;

	slopecount++;
	ret->id = slopecount;

	return ret;
}

/// Alocates and fill the contents of a slope structure.
static pslope_t *MakeViaVectors(const vector3_t *o, const vector2_t *d,
                             const fixed_t zdelta, UINT8 flags)
{
	pslope_t *ret = Slope_Add(flags);

	FV3_Copy(&ret->o, o);
	FV2_Copy(&ret->d, d);

	ret->zdelta = zdelta;

	ret->flags = flags;

	return ret;
}

/// Get furthest perpendicular distance from all vertexes in a sector for a given line.
static fixed_t GetExtent(sector_t *sector, line_t *line)
{
	// ZDoom code reference: v3float_t = vertex_t
	fixed_t fardist = -FRACUNIT;
	size_t i;

	// Find furthest vertex from the reference line. It, along with the two ends
	// of the line, will define the plane.
	for(i = 0; i < sector->linecount; i++)
	{
		line_t *li = sector->lines[i];
		vertex_t tempv;
		fixed_t dist;

		// Don't compare to the slope line.
		if(li == line)
			continue;

		P_ClosestPointOnLine(li->v1->x, li->v1->y, line, &tempv);
		dist = R_PointToDist2(tempv.x, tempv.y, li->v1->x, li->v1->y);
		if(dist > fardist)
			fardist = dist;

		// Okay, maybe do it for v2 as well?
		P_ClosestPointOnLine(li->v2->x, li->v2->y, line, &tempv);
		dist = R_PointToDist2(tempv.x, tempv.y, li->v2->x, li->v2->y);
		if(dist > fardist)
			fardist = dist;
	}

	return fardist;
}

static boolean P_CopySlope(pslope_t** toslope, pslope_t* fromslope)
{
	if (*toslope || !fromslope)
		return true;

	*toslope = fromslope;
	return true;
}

static void P_UpdateHasSlope(sector_t *sec)
{
	size_t i;

	sec->hasslope = true;

	// if this is an FOF control sector, make sure any target sectors also are marked as having slopes
	if (sec->numattached)
		for (i = 0; i < sec->numattached; i++)
			sectors[sec->attached[i]].hasslope = true;
}

/// Creates one or more slopes based on the given line type and front/back sectors.
static void line_SpawnViaLine(const int linenum, const boolean spawnthinker)
{
	// With dynamic slopes, it's fine to just leave this function as normal,
	// because checking to see if a slope had changed will waste more memory than
	// if the slope was just updated when called
	line_t *line = lines + linenum;
	pslope_t *fslope = NULL, *cslope = NULL;
	vector3_t origin, point;
	vector2_t direction;
	fixed_t nx, ny, dz, extent;

	boolean frontfloor = line->args[0] == TMS_FRONT;
	boolean backfloor = line->args[0] == TMS_BACK;
	boolean frontceil = line->args[1] == TMS_FRONT;
	boolean backceil = line->args[1] == TMS_BACK;
	UINT8 flags = 0; // Slope flags
	if (line->args[2] & TMSL_NOPHYSICS)
		flags |= SL_NOPHYSICS;
	if (line->args[2] & TMSL_DYNAMIC)
		flags |= SL_DYNAMIC;

	if(!frontfloor && !backfloor && !frontceil && !backceil)
	{
		CONS_Printf("line_SpawnViaLine: Slope special with nothing to do.\n");
		return;
	}

	if(!line->frontsector || !line->backsector)
	{
		CONS_Debug(DBG_SETUP, "line_SpawnViaLine: Slope special used on a line without two sides. (line number %i)\n", linenum);
		return;
	}

	{
		fixed_t len = R_PointToDist2(0, 0, line->dx, line->dy);
		nx = FixedDiv(line->dy, len);
		ny = -FixedDiv(line->dx, len);
	}

	// Set origin to line's center.
	origin.x = line->v1->x + (line->v2->x - line->v1->x)/2;
	origin.y = line->v1->y + (line->v2->y - line->v1->y)/2;

	// For FOF slopes, make a special function to copy to the xy origin & direction relative to the position of the FOF on the map!
	if(frontfloor || frontceil)
	{
		line->frontsector->hasslope = true; // Tell the software renderer that we're sloped

		origin.z = line->backsector->floorheight;
		direction.x = nx;
		direction.y = ny;

		extent = GetExtent(line->frontsector, line);

		if(extent < 0)
		{
			CONS_Printf("line_SpawnViaLine failed to get frontsector extent on line number %i\n", linenum);
			return;
		}

		// reposition the origin according to the extent
		point.x = origin.x + FixedMul(direction.x, extent);
		point.y = origin.y + FixedMul(direction.y, extent);
		direction.x = -direction.x;
		direction.y = -direction.y;

		// TODO: We take origin and point 's xy values and translate them to the center of an FOF!

		if(frontfloor)
		{
			point.z = line->frontsector->floorheight; // Startz
			dz = FixedDiv(origin.z - point.z, extent); // Destinationz

			// In P_SpawnSlopeLine the origin is the centerpoint of the sourcelinedef

			fslope = line->frontsector->f_slope =
			MakeViaVectors(&point, &direction, dz, flags);

			// Now remember that f_slope IS a vector
			// fslope->o = origin      3D point 1 of the vector
			// fslope->d = destination 3D point 2 of the vector
			// fslope->normal is a 3D line perpendicular to the 3D vector

			P_CalculateLineSlopeHighLow(fslope, line, false, false);

			fslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			fslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(fslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynLineSlopeThinker(fslope, DP_FRONTFLOOR, line, extent);
		}
		if(frontceil)
		{
			origin.z = line->backsector->ceilingheight;
			point.z = line->frontsector->ceilingheight;
			dz = FixedDiv(origin.z - point.z, extent);

			cslope = line->frontsector->c_slope =
			MakeViaVectors(&point, &direction, dz, flags);

			P_CalculateLineSlopeHighLow(cslope, line, true, false);

			cslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			cslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(cslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynLineSlopeThinker(cslope, DP_FRONTCEIL, line, extent);
		}
	}
	if(backfloor || backceil)
	{
		line->backsector->hasslope = true; // Tell the software renderer that we're sloped

		origin.z = line->frontsector->floorheight;
		// Backsector
		direction.x = -nx;
		direction.y = -ny;

		extent = GetExtent(line->backsector, line);

		if(extent < 0)
		{
			CONS_Printf("line_SpawnViaLine failed to get backsector extent on line number %i\n", linenum);
			return;
		}

		// reposition the origin according to the extent
		point.x = origin.x + FixedMul(direction.x, extent);
		point.y = origin.y + FixedMul(direction.y, extent);
		direction.x = -direction.x;
		direction.y = -direction.y;

		if(backfloor)
		{
			point.z = line->backsector->floorheight;
			dz = FixedDiv(origin.z - point.z, extent);

			fslope = line->backsector->f_slope =
			MakeViaVectors(&point, &direction, dz, flags);

			P_CalculateLineSlopeHighLow(fslope, line, false, true);

			fslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			fslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(fslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynLineSlopeThinker(fslope, DP_BACKFLOOR, line, extent);
		}
		if(backceil)
		{
			origin.z = line->frontsector->ceilingheight;
			point.z = line->backsector->ceilingheight;
			dz = FixedDiv(origin.z - point.z, extent);

			cslope = line->backsector->c_slope =
			MakeViaVectors(&point, &direction, dz, flags);

			P_CalculateLineSlopeHighLow(cslope, line, true, true);

			cslope->zangle = R_PointToAngle2(0, origin.z, extent, point.z);
			cslope->xydirection = R_PointToAngle2(origin.x, origin.y, point.x, point.y);

			P_CalculateSlopeNormal(cslope);

			if (spawnthinker && (flags & SL_DYNAMIC))
				P_AddDynLineSlopeThinker(cslope, DP_BACKCEIL, line, extent);
		}
	}

	if (line->args[2] & TMSL_COPY)
	{
		if (frontfloor)
			P_CopySlope(&line->backsector->f_slope, line->frontsector->f_slope);
		if (backfloor)
			P_CopySlope(&line->frontsector->f_slope, line->backsector->f_slope);
		if (frontceil)
			P_CopySlope(&line->backsector->c_slope, line->frontsector->c_slope);
		if (backceil)
			P_CopySlope(&line->frontsector->c_slope, line->backsector->c_slope);

		if (backfloor || backceil)
			P_UpdateHasSlope(line->frontsector);
		if (frontfloor || frontceil)
			P_UpdateHasSlope(line->backsector);
	}
}

/// Creates a new slope from three mapthings with the specified IDs
static pslope_t *MakeViaMapthings(INT16 tag1, INT16 tag2, INT16 tag3, UINT8 flags, const boolean spawnthinker)
{
	size_t i;
	mapthing_t* mt = mapthings;
	mapthing_t* vertices[3] = {0};
	INT16 tags[3] = {tag1, tag2, tag3};

	vector3_t vx[3];
	pslope_t* ret = Slope_Add(flags);

	// And... look for the vertices in question.
	for (i = 0; i < nummapthings; i++, mt++) {
		if (mt->type != 750) // Haha, I'm hijacking the old Chaos Spawn thingtype for something!
			continue;

		if (!vertices[0] && mt->tid == tag1)
			vertices[0] = mt;
		else if (!vertices[1] && mt->tid == tag2)
			vertices[1] = mt;
		else if (!vertices[2] && mt->tid == tag3)
			vertices[2] = mt;
	}

	// Now set heights for each vertex, because they haven't been set yet
	for (i = 0; i < 3; i++) {
		mt = vertices[i];
		if (!mt) // If a vertex wasn't found, it's game over. There's nothing you can do to recover (except maybe try and kill the slope instead - TODO?)
			I_Error("MakeViaMapthings: Slope vertex %s (for linedef tag %d) not found!", sizeu1(i), tag1);
		vx[i].x = mt->x << FRACBITS;
		vx[i].y = mt->y << FRACBITS;
		vx[i].z = mt->z << FRACBITS;
		if (!mt->thing_args[0])
			vx[i].z += R_PointInSubsector(vx[i].x, vx[i].y)->sector->floorheight;
	}

	P_ReconfigureViaVertexes(ret, vx[0], vx[1], vx[2]);

	if (spawnthinker && (flags & SL_DYNAMIC))
		P_AddDynVertexSlopeThinker(ret, tags, vx);

	return ret;
}

/// Create vertex based slopes using tagged mapthings.
static void line_SpawnViaMapthingVertexes(const int linenum, const boolean spawnthinker)
{
	line_t *line = lines + linenum;
	side_t *side;
	pslope_t **slopetoset;
	UINT16 tag1 = line->args[1];
	UINT16 tag2 = line->args[2];
	UINT16 tag3 = line->args[3];
	UINT8 flags = 0; // Slope flags

	if (line->args[4] & TMSL_NOPHYSICS)
		flags |= SL_NOPHYSICS;
	if (line->args[4] & TMSL_DYNAMIC)
		flags |= SL_DYNAMIC;

	switch(line->args[0])
	{
	case TMSP_FRONTFLOOR:
		slopetoset = &line->frontsector->f_slope;
		side = &sides[line->sidenum[0]];
		break;
	case TMSP_FRONTCEILING:
		slopetoset = &line->frontsector->c_slope;
		side = &sides[line->sidenum[0]];
		break;
	case TMSP_BACKFLOOR:
		slopetoset = &line->backsector->f_slope;
		side = &sides[line->sidenum[1]];
		break;
	case TMSP_BACKCEILING:
		slopetoset = &line->backsector->c_slope;
		side = &sides[line->sidenum[1]];
	default:
		return;
	}

	*slopetoset = MakeViaMapthings(tag1, tag2, tag3, flags, spawnthinker);

	side->sector->hasslope = true;
}

/// Spawn textmap vertex slopes.
static void SpawnVertexSlopes(void)
{
	line_t *l1, *l2;
	sector_t* sc;
	vertex_t *v1, *v2, *v3;
	size_t i;
	for (i = 0, sc = sectors; i < numsectors; i++, sc++)
	{
		// The vertex slopes only work for 3-vertex sectors (and thus 3-sided sectors).
		if (sc->linecount != 3)
			continue;

		l1 = sc->lines[0];
		l2 = sc->lines[1];

		// Determine the vertexes.
		v1 = l1->v1;
		v2 = l1->v2;
		if ((l2->v1 != v1) && (l2->v1 != v2))
			v3 = l2->v1;
		else
			v3 = l2->v2;

		if (v1->floorzset || v2->floorzset || v3->floorzset)
		{
			vector3_t vtx[3] = {
				{v1->x, v1->y, v1->floorzset ? v1->floorz : sc->floorheight},
				{v2->x, v2->y, v2->floorzset ? v2->floorz : sc->floorheight},
				{v3->x, v3->y, v3->floorzset ? v3->floorz : sc->floorheight}};
			pslope_t *slop = Slope_Add(0);
			sc->f_slope = slop;
			sc->hasslope = true;
			P_ReconfigureViaVertexes(slop, vtx[0], vtx[1], vtx[2]);
		}

		if (v1->ceilingzset || v2->ceilingzset || v3->ceilingzset)
		{
			vector3_t vtx[3] = {
				{v1->x, v1->y, v1->ceilingzset ? v1->ceilingz : sc->ceilingheight},
				{v2->x, v2->y, v2->ceilingzset ? v2->ceilingz : sc->ceilingheight},
				{v3->x, v3->y, v3->ceilingzset ? v3->ceilingz : sc->ceilingheight}};
			pslope_t *slop = Slope_Add(0);
			sc->c_slope = slop;
			sc->hasslope = true;
			P_ReconfigureViaVertexes(slop, vtx[0], vtx[1], vtx[2]);
		}
	}
}

static boolean P_SetSlopeFromTag(sector_t *sec, INT32 tag, boolean ceiling)
{
	INT32 i;
	pslope_t **secslope = ceiling ? &sec->c_slope : &sec->f_slope;

	if (!tag || *secslope)
		return false;
	TAG_ITER_SECTORS(tag, i)
	{
		pslope_t *srcslope = ceiling ? sectors[i].c_slope : sectors[i].f_slope;
		if (srcslope)
		{
			*secslope = srcslope;
			return true;
		}
	}
	return false;
}

//
// P_CopySectorSlope
//
// Searches through tagged sectors and copies
//
void P_CopySectorSlope(line_t *line)
{
	sector_t *fsec = line->frontsector;
	sector_t *bsec = line->backsector;
	boolean setfront = false;
	boolean setback = false;

	setfront |= P_SetSlopeFromTag(fsec, line->args[0], false);
	setfront |= P_SetSlopeFromTag(fsec, line->args[1], true);
	if (bsec)
	{
		setback |= P_SetSlopeFromTag(bsec, line->args[2], false);
		setback |= P_SetSlopeFromTag(bsec, line->args[3], true);

		if (line->args[4] & TMSC_FRONTTOBACKFLOOR)
			setback |= P_CopySlope(&bsec->f_slope, fsec->f_slope);
		if (line->args[4] & TMSC_BACKTOFRONTFLOOR)
			setfront |= P_CopySlope(&fsec->f_slope, bsec->f_slope);
		if (line->args[4] & TMSC_FRONTTOBACKCEILING)
			setback |= P_CopySlope(&bsec->c_slope, fsec->c_slope);
		if (line->args[4] & TMSC_BACKTOFRONTCEILING)
			setfront |= P_CopySlope(&fsec->c_slope, bsec->c_slope);
	}

	if (setfront)
		P_UpdateHasSlope(fsec);
	if (setback)
		P_UpdateHasSlope(bsec);

	line->special = 0; // Linedef was use to set slopes, it finished its job, so now make it a normal linedef
}

//
// P_SlopeById
//
// Looks in the slope list for a slope with a specified ID. Mostly useful for netgame sync
//
pslope_t *P_SlopeById(UINT16 id)
{
	pslope_t *ret;
	for (ret = slopelist; ret && ret->id != id; ret = ret->next);
	return ret;
}

/// Creates a new slope from equation constants.
pslope_t *MakeViaEquationConstants(const fixed_t a, const fixed_t b, const fixed_t c, const fixed_t d)
{
	pslope_t* ret = Slope_Add(0);

	ReconfigureViaConstants(ret, a, b, c, d);

	return ret;
}

/// Initializes and reads the slopes from the map data.
void P_SpawnSlopes(const boolean fromsave) {
	size_t i;

	/// Generates vertex slopes.
	SpawnVertexSlopes();

	/// Generates line special-defined slopes.
	for (i = 0; i < numlines; i++)
	{
		switch (lines[i].special)
		{
			case 700:
				line_SpawnViaLine(i, !fromsave);
				break;

			case 704:
				line_SpawnViaMapthingVertexes(i, !fromsave);
				break;

			default:
				break;
		}
	}

	// jart

	/// Build list of slope anchors--faster searching.
	P_BuildSlopeAnchorList();

	/// Setup anchor based slopes.
	P_SetupAnchoredSlopes();

	// end of jart

	/// Copies slopes from tagged sectors via line specials.
	/// \note Doesn't actually copy, but instead they share the same pointers.
	for (i = 0; i < numlines; i++)
		switch (lines[i].special)
		{
			case 720:
				P_CopySectorSlope(&lines[i]);
			default:
				break;
		}
}

/// Initializes slopes.
void P_InitSlopes(void)
{
	slopelist = NULL;
	slopecount = 0;
}

// ============================================================================
//
// Various utilities related to slopes
//

// Returns the height of the sloped plane at (x, y) as a fixed_t
fixed_t P_GetSlopeZAt(const pslope_t *slope, fixed_t x, fixed_t y)
{
	fixed_t dist = FixedMul(x - slope->o.x, slope->d.x) +
	               FixedMul(y - slope->o.y, slope->d.y);

	return slope->o.z + FixedMul(dist, slope->zdelta);
}

// Like P_GetSlopeZAt but falls back to z if slope is NULL
fixed_t P_GetZAt(const pslope_t *slope, fixed_t x, fixed_t y, fixed_t z)
{
	return slope ? P_GetSlopeZAt(slope, x, y) : z;
}

// Returns the height of the sector floor at (x, y)
fixed_t P_GetSectorFloorZAt(const sector_t *sector, fixed_t x, fixed_t y)
{
	return sector->f_slope ? P_GetSlopeZAt(sector->f_slope, x, y) : sector->floorheight;
}

// Returns the height of the sector ceiling at (x, y)
fixed_t P_GetSectorCeilingZAt(const sector_t *sector, fixed_t x, fixed_t y)
{
	return sector->c_slope ? P_GetSlopeZAt(sector->c_slope, x, y) : sector->ceilingheight;
}

// Returns the height of the FOF top at (x, y)
fixed_t P_GetFFloorTopZAt(const ffloor_t *ffloor, fixed_t x, fixed_t y)
{
	return *ffloor->t_slope ? P_GetSlopeZAt(*ffloor->t_slope, x, y) : *ffloor->topheight;
}

// Returns the height of the FOF bottom  at (x, y)
fixed_t P_GetFFloorBottomZAt(const ffloor_t *ffloor, fixed_t x, fixed_t y)
{
	return *ffloor->b_slope ? P_GetSlopeZAt(*ffloor->b_slope, x, y) : *ffloor->bottomheight;
}

// Returns the height of the light list at (x, y)
fixed_t P_GetLightZAt(const lightlist_t *light, fixed_t x, fixed_t y)
{
	return light->slope ? P_GetSlopeZAt(light->slope, x, y) : light->height;
}

// Returns true if we should run slope physics code on an object.
boolean P_CanApplySlopePhysics(mobj_t *mo, pslope_t *slope)
{
	if (slope == NULL || mo == NULL || P_MobjWasRemoved(mo) == true)
	{
		// Invalid input.
		return false;
	}

	if (slope->flags & SL_NOPHYSICS)
	{
		// Physics are turned off.
		return false;
	}

	if (slope->normal.x == 0 && slope->normal.y == 0)
	{
		// Flat slope? No such thing, man. No such thing.
		return false;
	}

	if (mo->player != NULL)
	{
		if (K_PlayerEBrake(mo->player) == true)
		{
			// Spindash negates slopes.
			return false;
		}
	}

	// We can do slope physics.
	return true;
}

// Returns true if we should run slope launch code on an object.
boolean P_CanApplySlopeLaunch(mobj_t *mo, pslope_t *slope)
{
	if (slope == NULL || mo == NULL || P_MobjWasRemoved(mo) == true)
	{
		// Invalid input.
		return false;
	}

	// No physics slopes are fine to launch off of.

	if (slope->normal.x == 0 && slope->normal.y == 0)
	{
		// Flat slope? No such thing, man. No such thing.
		return false;
	}

	if (mo->eflags & MFE_DONTSLOPELAUNCH)
	{
		CONS_Printf("MFE_DONTSLOPELAUNCH\n");
		mo->eflags &= ~MFE_DONTSLOPELAUNCH; // You get one cancelled launch
		// Don't launch off of slopes.
		return false;
	}

	// We can do slope launching.
	return true;
}

//
// P_QuantizeMomentumToSlope
//
// When given a vector, rotates it and aligns it to a slope
void P_QuantizeMomentumToSlope(vector3_t *momentum, pslope_t *slope)
{
	vector3_t axis; // Fuck you, C90.

	axis.x = -slope->d.y;
	axis.y = slope->d.x;
	axis.z = 0;

	FV3_Rotate(momentum, &axis, slope->zangle >> ANGLETOFINESHIFT);
}

//
// P_ReverseQuantizeMomentumToSlope
//
// When given a vector, rotates and aligns it to a flat surface (from being relative to a given slope)
void P_ReverseQuantizeMomentumToSlope(vector3_t *momentum, pslope_t *slope)
{
	slope->zangle = InvAngle(slope->zangle);
	P_QuantizeMomentumToSlope(momentum, slope);
	slope->zangle = InvAngle(slope->zangle);
}

//
// P_SlopeLaunch
//
// Handles slope ejection for objects
void P_SlopeLaunch(mobj_t *mo)
{
	if (P_CanApplySlopeLaunch(mo, mo->standingslope) == true) // If there's physics, time for launching.
	{
		vector3_t slopemom;
		slopemom.x = mo->momx;
		slopemom.y = mo->momy;
		slopemom.z = mo->momz;
		P_QuantizeMomentumToSlope(&slopemom, mo->standingslope);

		mo->momx = slopemom.x;
		mo->momy = slopemom.y;
		mo->momz = slopemom.z;

		mo->eflags |= MFE_SLOPELAUNCHED;
	}

	//CONS_Printf("Launched off of slope.\n");
	mo->standingslope = NULL;
	mo->terrain = NULL;

	if (mo->player)
	{
		mo->player->stairjank = 0; // fuck you
	}
}

//
// P_GetWallTransferMomZ
//
// It would be nice to have a single function that does everything necessary for slope-to-wall transfer.
// However, it needs to be seperated out in P_XYMovement to take into account momentum before and after hitting the wall.
// This just performs the necessary calculations for getting the base vertical momentum; the horizontal is already reasonably calculated by P_SlideMove.
fixed_t P_GetWallTransferMomZ(mobj_t *mo, pslope_t *slope)
{
	vector3_t slopemom, axis;
	angle_t ang;

	if (P_CanApplySlopeLaunch(mo, mo->standingslope) == false)
	{
		return false;
	}

	// If there's physics, time for launching.
	// Doesn't kill the vertical momentum as much as P_SlopeLaunch does.
	ang = slope->zangle + ANG15*((slope->zangle > 0) ? 1 : -1);
	if (ang > ANGLE_90 && ang < ANGLE_180)
	{
		// hard cap of directly upwards
		ang = ((slope->zangle > 0) ? ANGLE_90 : InvAngle(ANGLE_90));
	}

	slopemom.x = mo->momx;
	slopemom.y = mo->momy;
	slopemom.z = mo->momz;

	axis.x = -slope->d.y;
	axis.y = slope->d.x;
	axis.z = 0;

	FV3_Rotate(&slopemom, &axis, ang >> ANGLETOFINESHIFT);

	return slopemom.z;
}

// Function to help handle landing on slopes
void P_HandleSlopeLanding(mobj_t *thing, pslope_t *slope)
{
	vector3_t mom; // Ditto.

	if (P_CanApplySlopePhysics(thing, slope) == false) // No physics, no need to make anything complicated.
	{
		if (P_MobjFlip(thing)*(thing->momz) < 0) // falling, land on slope
		{
			thing->standingslope = slope;
			P_SetPitchRollFromSlope(thing, slope);
			thing->momz = -P_MobjFlip(thing);
		}

		return;
	}

	mom.x = thing->momx;
	mom.y = thing->momy;
	mom.z = thing->momz*2;

	P_ReverseQuantizeMomentumToSlope(&mom, slope);

	if (P_MobjFlip(thing)*mom.z < 0)
	{
		// falling, land on slope
		thing->momx = mom.x;
		thing->momy = mom.y;
		thing->standingslope = slope;
		P_SetPitchRollFromSlope(thing, slope);
		thing->momz = -P_MobjFlip(thing);
	}
}

// https://yourlogicalfallacyis.com/slippery-slope
// Handles sliding down slopes, like if they were made of butter :)
void P_ButteredSlope(mobj_t *mo)
{
	const fixed_t gameSpeed = K_GetKartGameSpeedScalar(gamespeed);
	fixed_t thrust = 0;

	if (mo->flags & (MF_NOCLIPHEIGHT|MF_NOGRAVITY))
	{
		return; // don't slide down slopes if you can't touch them or you're not affected by gravity
	}

	if (P_CanApplySlopePhysics(mo, mo->standingslope) == false)
	{
		return; // No physics, no butter.
	}

	if (mo->player != NULL)
	{
		if (abs(mo->standingslope->zdelta) < FRACUNIT/21)
		{
			// Don't slide on non-steep slopes.
			// Changed in Ring Racers to only not apply physics on very slight slopes.
			// (I think about 4 degree angles.)
			return;
		}

		if (abs(mo->standingslope->zdelta) < FRACUNIT/2
			&& !(mo->player->rmomx || mo->player->rmomy))
		{
			// Allow the player to stand still on slopes below a certain steepness.
			// 45 degree angle steep, to be exact.
			return;
		}
	}

	thrust = FINESINE(mo->standingslope->zangle>>ANGLETOFINESHIFT) * 5 / 4 * (mo->eflags & MFE_VERTICALFLIP ? 1 : -1);

	if (mo->momx || mo->momy)
	{
		fixed_t mult = FRACUNIT;
		angle_t angle = R_PointToAngle2(0, 0, mo->momx, mo->momy) - mo->standingslope->xydirection;

		if (P_MobjFlip(mo) * mo->standingslope->zdelta < 0)
		{
			angle ^= ANGLE_180;
		}

		// Make uphill easier to climb, and downhill even faster.
		mult = FINECOSINE(angle >> ANGLETOFINESHIFT);

		// Make relative to game speed
		mult = FixedMul(mult, gameSpeed);

		// Easy / Battle: SUPER NERF slope climbs, so that they're usually possible without resources.
		// (New players suck at budgeting, and may not remember they have spindash / rings at all!)
		// Special exception for Tutorial because we're trying to teach slope mechanics there.
		if (K_GetKartGameSpeedScalar(gamespeed) < FRACUNIT && gametype != GT_TUTORIAL)
		{
			// Same as above, but use facing angle:
			angle_t easyangle = mo->angle - mo->standingslope->xydirection;
			if (P_MobjFlip(mo) * mo->standingslope->zdelta < 0)
			{
				easyangle ^= ANGLE_180;
			}
			fixed_t mult2 = FINECOSINE(easyangle >> ANGLETOFINESHIFT);
			mult2 = FixedMul(mult2, gameSpeed);

			// Prefer the modifier that helps us go where we're going.
			if (mult2 < mult)
				mult = mult2;

			// And if we're staring down a slope, believe in ourself really hard and climb it anyway.
			if (mult < 0)
				mult = 8 * mult / 5;
		}

		mult = FRACUNIT + (FRACUNIT + mult)*4/3;
		thrust = FixedMul(thrust, mult);
	}

	// Let's get the gravity strength for the object...
	thrust = FixedMul(thrust, abs(P_GetMobjGravity(mo)));

	fixed_t basefriction = ORIG_FRICTION;
	if (mo->player)
		basefriction = K_PlayerBaseFriction(mo->player, ORIG_FRICTION);

	if (mo->friction != basefriction && basefriction != 0)
	{
		// ... and its friction against the ground for good measure.
		// (divided by original friction to keep behaviour for normal slopes the same)
		thrust = FixedMul(thrust, FixedDiv(mo->friction, basefriction));

		// Sal: Also consider movefactor of players.
		// We want ice to make slopes *really* funnel you in a specific direction.
		fixed_t move_factor = P_MoveFactorFromFriction(mo->friction);

		if (mo->player != NULL)
		{
			if (mo->player->icecube.frozen == true)
			{
				// Undo this change with ice cubes, because it is insanity.
				move_factor = FRACUNIT;
			}
			else if (mo->player->tiregrease > 0)
			{
				// Undo this change with tire grease, so that
				// springs and spindash can still overpower slopes.
				fixed_t grease_frac = clamp((FRACUNIT * mo->player->tiregrease) / greasetics, 0, FRACUNIT);
				move_factor = Easing_Linear(grease_frac, move_factor, FRACUNIT);
			}
		}

		thrust = FixedMul(thrust, FixedDiv(FRACUNIT, move_factor));
	}

	P_Thrust(mo, mo->standingslope->xydirection, thrust);
}

// jart
#include "slope_anchors.c"
