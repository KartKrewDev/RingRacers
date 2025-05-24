// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze, Andrey Budko (prboom)
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_fps.c
/// \brief Uncapped framerate stuff.

#include "r_fps.h"

#include <vector>

#include "p_mobj.h"
#include "r_main.h"
#include "g_game.h"
#include "i_video.h"
#include "r_plane.h"
#include "r_state.h"
#include "z_zone.h"
#include "i_time.h"

UINT32 R_GetFramerateCap(void)
{
	if (rendermode == render_none)
	{
		// If we're not rendering (dedicated server),
		// we shouldn't be using any interpolation.
		return TICRATE;
	}

	if (cv_fpscap.value == 0)
	{
		// 0: Match refresh rate
		return I_GetRefreshRate();
	}

	if (cv_fpscap.value < 0)
	{
		// -1: Unlimited
		return 0;
	}

	return cv_fpscap.value;
}

boolean R_UsingFrameInterpolation(void)
{
	return (R_GetFramerateCap() != TICRATE || I_GetTimeScale() < FRACUNIT);
}

static viewvars_t pview_old[MAXSPLITSCREENPLAYERS];
static viewvars_t pview_new[MAXSPLITSCREENPLAYERS];
static viewvars_t skyview_old[MAXSPLITSCREENPLAYERS];
static viewvars_t skyview_new[MAXSPLITSCREENPLAYERS];

static viewvars_t *oldview = &pview_old[0];
static tic_t last_view_update;
static int oldview_invalid[MAXSPLITSCREENPLAYERS] = {0, 0, 0, 0};
viewvars_t *newview = &pview_new[0];


enum viewcontext_e viewcontext = VIEWCONTEXT_PLAYER1;

static std::vector<levelinterpolator_t> levelinterpolators;

static fixed_t R_LerpFixed(fixed_t from, fixed_t to, fixed_t frac)
{
	return from + FixedMul(frac, to - from);
}

static angle_t R_LerpAngle(angle_t from, angle_t to, fixed_t frac)
{
	return from + FixedMul(frac, to - from);
}

static vector2_t *R_LerpVector2(const vector2_t *from, const vector2_t *to, fixed_t frac, vector2_t *out)
{
	FV2_SubEx(to, from, out);
	FV2_MulEx(out, frac, out);
	FV2_AddEx(from, out, out);
	return out;
}

static vector3_t *R_LerpVector3(const vector3_t *from, const vector3_t *to, fixed_t frac, vector3_t *out)
{
	FV3_SubEx(to, from, out);
	FV3_MulEx(out, frac, out);
	FV3_AddEx(from, out, out);
	return out;
}

// recalc necessary stuff for mouseaiming
// slopes are already calculated for the full possible view (which is 4*viewheight).
// 18/08/18: (No it's actually 16*viewheight, thanks Jimita for finding this out)
static void R_SetupFreelook(player_t *player, boolean skybox)
{
	G_FinalClipAimingPitch((INT32 *)&aimingangle, player, skybox);

	centeryfrac = (viewheight/2)<<FRACBITS;

	if (rendermode == render_soft)
		centeryfrac += FixedMul(AIMINGTODY(aimingangle), FixedDiv(viewwidth<<FRACBITS, BASEVIDWIDTH<<FRACBITS));

	centery = FixedInt(FixedRound(centeryfrac));

	if (rendermode == render_soft)
		yslope = &yslopetab[viewssnum][viewheight*8 - centery];
}

#undef AIMINGTODY

void R_InterpolateViewRollAngle(fixed_t frac)
{
	viewroll = R_LerpAngle(oldview->roll, newview->roll, frac);
}

void R_InterpolateView(fixed_t frac)
{
	viewvars_t* prevview = oldview;
	UINT8 i;

	if (FIXED_TO_FLOAT(frac) < 0)
		frac = 0;
	if (frac > FRACUNIT)
		frac = FRACUNIT;

	i = R_GetViewNumber();

	if (oldview_invalid[i] != 0)
	{
		// interpolate from newview to newview
		prevview = newview;
	}

	viewx = R_LerpFixed(prevview->x, newview->x, frac);
	viewy = R_LerpFixed(prevview->y, newview->y, frac);
	viewz = R_LerpFixed(prevview->z, newview->z, frac);

	viewangle = R_LerpAngle(prevview->angle, newview->angle, frac);
	aimingangle = R_LerpAngle(prevview->aim, newview->aim, frac);
	viewroll = R_LerpAngle(prevview->roll, newview->roll, frac);

	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	viewplayer = newview->player;
	viewsector = R_PointInSubsector(viewx, viewy)->sector;

	R_SetupFreelook(newview->player, newview->sky);
}

void R_UpdateViewInterpolation(void)
{
	UINT8 i;

	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	{
		pview_old[i] = pview_new[i];
		skyview_old[i] = skyview_new[i];
		if (oldview_invalid[i] > 0) oldview_invalid[i]--;
	}

	last_view_update = I_GetTime();
}

void R_ResetViewInterpolation(UINT8 p)
{
	// Wait an extra tic if the interpolation state hasn't
	// updated yet.
	int t = last_view_update == I_GetTime() ? 1 : 2;

	if (p == 0)
	{
		UINT8 i;
		for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
		{
			oldview_invalid[i] = t;
		}
	}
	else
	{
		oldview_invalid[p - 1] = t;
	}
}

void R_RelativeTeleportViewInterpolation(UINT8 p, fixed_t xdiff, fixed_t ydiff, fixed_t zdiff, angle_t angdiff)
{
	pview_old[p].x += xdiff;
	pview_old[p].y += ydiff;
	pview_old[p].z += zdiff;
	pview_old[p].angle += angdiff;
}

void R_SetViewContext(enum viewcontext_e _viewcontext)
{
	UINT8 i = 0;

	I_Assert(_viewcontext >= VIEWCONTEXT_PLAYER1
			&& _viewcontext <= VIEWCONTEXT_SKY4);
	viewcontext = _viewcontext;

	switch (viewcontext)
	{
		case VIEWCONTEXT_PLAYER1:
		case VIEWCONTEXT_PLAYER2:
		case VIEWCONTEXT_PLAYER3:
		case VIEWCONTEXT_PLAYER4:
			i = viewcontext - VIEWCONTEXT_PLAYER1;
			oldview = &pview_old[i];
			newview = &pview_new[i];
			break;
		case VIEWCONTEXT_SKY1:
		case VIEWCONTEXT_SKY2:
		case VIEWCONTEXT_SKY3:
		case VIEWCONTEXT_SKY4:
			i = viewcontext - VIEWCONTEXT_SKY1;
			oldview = &skyview_old[i];
			newview = &skyview_new[i];
			break;
		default:
			I_Error("viewcontext value is invalid: we should never get here without an assert!!");
			break;
	}
}

fixed_t R_InterpolateFixed(fixed_t from, fixed_t to)
{
	if (!R_UsingFrameInterpolation())
	{
		return to;
	}

	return (R_LerpFixed(from, to, rendertimefrac));
}

angle_t R_InterpolateAngle(angle_t from, angle_t to)
{
	if (!R_UsingFrameInterpolation())
	{
		return to;
	}

	return (R_LerpAngle(from, to, rendertimefrac));
}

void R_InterpolateMobjState(mobj_t *mobj, fixed_t frac, interpmobjstate_t *out)
{
	if (frac == FRACUNIT)
	{
		out->x = mobj->x;
		out->y = mobj->y;
		out->z = mobj->z;
		out->scale = mobj->scale;
		out->subsector = mobj->subsector;
		out->angle = mobj->player ? mobj->player->drawangle : mobj->angle;
		out->spritexscale = mobj->spritexscale;
		out->spriteyscale = mobj->spriteyscale;
		out->spritexoffset = mobj->spritexoffset;
		out->spriteyoffset = mobj->spriteyoffset;
		return;
	}

	out->x = R_LerpFixed(mobj->old_x, mobj->x, frac);
	out->y = R_LerpFixed(mobj->old_y, mobj->y, frac);
	out->z = R_LerpFixed(mobj->old_z, mobj->z, frac);
	out->scale = mobj->resetinterp ? mobj->scale : R_LerpFixed(mobj->old_scale, mobj->scale, frac);
	out->spritexscale = mobj->resetinterp ? mobj->spritexscale : R_LerpFixed(mobj->old_spritexscale, mobj->spritexscale, frac);
	out->spriteyscale = mobj->resetinterp ? mobj->spriteyscale : R_LerpFixed(mobj->old_spriteyscale, mobj->spriteyscale, frac);

	// Sprite offsets are not interpolated until we have a way to interpolate them explicitly in Lua.
	// It seems existing mods visually break more often than not if it is interpolated.
	out->spritexoffset = mobj->spritexoffset;
	out->spriteyoffset = mobj->spriteyoffset;

	out->subsector = R_PointInSubsector(out->x, out->y);

	if (mobj->player)
	{
		out->angle = mobj->resetinterp ? mobj->player->drawangle : R_LerpAngle(mobj->player->old_drawangle, mobj->player->drawangle, frac);
	}
	else
	{
		out->angle = mobj->resetinterp ? mobj->angle : R_LerpAngle(mobj->old_angle, mobj->angle, frac);
	}
}

void R_InterpolatePrecipMobjState(precipmobj_t *mobj, fixed_t frac, interpmobjstate_t *out)
{
	if (frac == FRACUNIT)
	{
		out->x = mobj->x;
		out->y = mobj->y;
		out->z = mobj->z;
		out->scale = mapobjectscale;
		out->subsector = mobj->subsector;
		out->angle = mobj->angle;
		out->spritexscale = mobj->spritexscale;
		out->spriteyscale = mobj->spriteyscale;
		out->spritexoffset = mobj->spritexoffset;
		out->spriteyoffset = mobj->spriteyoffset;
		return;
	}

	out->x = R_LerpFixed(mobj->old_x, mobj->x, frac);
	out->y = R_LerpFixed(mobj->old_y, mobj->y, frac);
	out->z = R_LerpFixed(mobj->old_z, mobj->z, frac);
	out->scale = mapobjectscale;
	out->spritexscale = R_LerpFixed(mobj->old_spritexscale, mobj->spritexscale, frac);
	out->spriteyscale = R_LerpFixed(mobj->old_spriteyscale, mobj->spriteyscale, frac);
	out->spritexoffset = R_LerpFixed(mobj->old_spritexoffset, mobj->spritexoffset, frac);
	out->spriteyoffset = R_LerpFixed(mobj->old_spriteyoffset, mobj->spriteyoffset, frac);

	out->subsector = R_PointInSubsector(out->x, out->y);

	out->angle = R_LerpAngle(mobj->old_angle, mobj->angle, frac);
}

static levelinterpolator_t *CreateInterpolator(levelinterpolator_type_e type, thinker_t *thinker)
{
	auto* ret = &levelinterpolators.emplace_back(levelinterpolator_t { type, thinker });
	return ret;
}

void R_CreateInterpolator_SectorPlane(thinker_t *thinker, sector_t *sector, boolean ceiling)
{
	levelinterpolator_t *interp = CreateInterpolator(LVLINTERP_SectorPlane, thinker);
	interp->sectorplane.sector = sector;
	interp->sectorplane.ceiling = ceiling;
	if (ceiling)
	{
		interp->sectorplane.oldheight = interp->sectorplane.bakheight = sector->ceilingheight;
	}
	else
	{
		interp->sectorplane.oldheight = interp->sectorplane.bakheight = sector->floorheight;
	}
}

void R_CreateInterpolator_SectorScroll(thinker_t *thinker, sector_t *sector, boolean ceiling)
{
	levelinterpolator_t *interp = CreateInterpolator(LVLINTERP_SectorScroll, thinker);
	interp->sectorscroll.sector = sector;
	interp->sectorscroll.ceiling = ceiling;
	if (ceiling)
	{
		interp->sectorscroll.oldxoffs = interp->sectorscroll.bakxoffs = sector->ceiling_xoffs;
		interp->sectorscroll.oldyoffs = interp->sectorscroll.bakyoffs = sector->ceiling_yoffs;
	}
	else
	{
		interp->sectorscroll.oldxoffs = interp->sectorscroll.bakxoffs = sector->floor_xoffs;
		interp->sectorscroll.oldyoffs = interp->sectorscroll.bakyoffs = sector->floor_yoffs;
	}
}

void R_CreateInterpolator_SideScroll(thinker_t *thinker, side_t *side)
{
	levelinterpolator_t *interp = CreateInterpolator(LVLINTERP_SideScroll, thinker);
	interp->sidescroll.side = side;
	interp->sidescroll.oldtextureoffset = interp->sidescroll.baktextureoffset = side->textureoffset;
	interp->sidescroll.oldrowoffset = interp->sidescroll.bakrowoffset = side->rowoffset;
}

void R_CreateInterpolator_Polyobj(thinker_t *thinker, polyobj_t *polyobj)
{
	levelinterpolator_t *interp = CreateInterpolator(LVLINTERP_Polyobj, thinker);
	interp->polyobj.polyobj = polyobj;
	interp->polyobj.vertices_size = polyobj->numVertices;

	interp->polyobj.oldvertices = static_cast<fixed_t*>(Z_Calloc(sizeof(fixed_t) * 2 * polyobj->numVertices, PU_LEVEL, NULL));
	interp->polyobj.bakvertices = static_cast<fixed_t*>(Z_Calloc(sizeof(fixed_t) * 2 * polyobj->numVertices, PU_LEVEL, NULL));
	for (size_t i = 0; i < polyobj->numVertices; i++)
	{
		interp->polyobj.oldvertices[i * 2    ] = interp->polyobj.bakvertices[i * 2    ] = polyobj->vertices[i]->x;
		interp->polyobj.oldvertices[i * 2 + 1] = interp->polyobj.bakvertices[i * 2 + 1] = polyobj->vertices[i]->y;
	}

	interp->polyobj.oldcx = interp->polyobj.bakcx = polyobj->centerPt.x;
	interp->polyobj.oldcy = interp->polyobj.bakcy = polyobj->centerPt.y;
}

void R_CreateInterpolator_DynSlope(thinker_t *thinker, pslope_t *slope)
{
	levelinterpolator_t *interp = CreateInterpolator(LVLINTERP_DynSlope, thinker);
	interp->dynslope.slope = slope;

	FV3_Copy(&interp->dynslope.oldo, &slope->o);
	FV3_Copy(&interp->dynslope.bako, &slope->o);

	FV2_Copy(&interp->dynslope.oldd, &slope->d);
	FV2_Copy(&interp->dynslope.bakd, &slope->d);

	interp->dynslope.oldzdelta = interp->dynslope.bakzdelta = slope->zdelta;
}

void R_InitializeLevelInterpolators(void)
{
	levelinterpolators.clear();
}

static void UpdateLevelInterpolatorState(levelinterpolator_t *interp)
{
	size_t i;

	switch (interp->type)
	{
	case LVLINTERP_SectorPlane:
		interp->sectorplane.oldheight = interp->sectorplane.bakheight;
		interp->sectorplane.bakheight = interp->sectorplane.ceiling ? interp->sectorplane.sector->ceilingheight : interp->sectorplane.sector->floorheight;
		break;
	case LVLINTERP_SectorScroll:
		interp->sectorscroll.oldxoffs = interp->sectorscroll.bakxoffs;
		interp->sectorscroll.bakxoffs = interp->sectorscroll.ceiling ? interp->sectorscroll.sector->ceiling_xoffs : interp->sectorscroll.sector->floor_xoffs;
		interp->sectorscroll.oldyoffs = interp->sectorscroll.bakyoffs;
		interp->sectorscroll.bakyoffs = interp->sectorscroll.ceiling ? interp->sectorscroll.sector->ceiling_yoffs : interp->sectorscroll.sector->floor_yoffs;
		break;
	case LVLINTERP_SideScroll:
		interp->sidescroll.oldtextureoffset = interp->sidescroll.baktextureoffset;
		interp->sidescroll.baktextureoffset = interp->sidescroll.side->textureoffset;
		interp->sidescroll.oldrowoffset = interp->sidescroll.bakrowoffset;
		interp->sidescroll.bakrowoffset = interp->sidescroll.side->rowoffset;
		break;
	case LVLINTERP_Polyobj:
		for (i = 0; i < interp->polyobj.vertices_size; i++)
		{
			interp->polyobj.oldvertices[i * 2    ] = interp->polyobj.bakvertices[i * 2    ];
			interp->polyobj.oldvertices[i * 2 + 1] = interp->polyobj.bakvertices[i * 2 + 1];
			interp->polyobj.bakvertices[i * 2    ] = interp->polyobj.polyobj->vertices[i]->x;
			interp->polyobj.bakvertices[i * 2 + 1] = interp->polyobj.polyobj->vertices[i]->y;
		}
		interp->polyobj.oldcx = interp->polyobj.bakcx;
		interp->polyobj.oldcy = interp->polyobj.bakcy;
		interp->polyobj.bakcx = interp->polyobj.polyobj->centerPt.x;
		interp->polyobj.bakcy = interp->polyobj.polyobj->centerPt.y;
		break;
	case LVLINTERP_DynSlope:
		FV3_Copy(&interp->dynslope.oldo, &interp->dynslope.bako);
		FV2_Copy(&interp->dynslope.oldd, &interp->dynslope.bakd);
		interp->dynslope.oldzdelta = interp->dynslope.bakzdelta;

		FV3_Copy(&interp->dynslope.bako, &interp->dynslope.slope->o);
		FV2_Copy(&interp->dynslope.bakd, &interp->dynslope.slope->d);
		interp->dynslope.bakzdelta = interp->dynslope.slope->zdelta;
		break;
	}
}

void R_UpdateLevelInterpolators(void)
{
	for (levelinterpolator_t& interp : levelinterpolators)
	{
		UpdateLevelInterpolatorState(&interp);
	}
}

void R_ClearLevelInterpolatorState(thinker_t *thinker)
{
	for (levelinterpolator_t& interp : levelinterpolators)
	{
		if (interp.thinker == thinker)
		{
			// Do it twice to make the old state match the new
			UpdateLevelInterpolatorState(&interp);
			UpdateLevelInterpolatorState(&interp);
		}
	}
}

void R_ApplyLevelInterpolators(fixed_t frac)
{
	size_t ii;

	for (levelinterpolator_t& i : levelinterpolators)
	{
		levelinterpolator_t* interp = &i;

		switch (interp->type)
		{
		case LVLINTERP_SectorPlane:
			if (interp->sectorplane.ceiling)
			{
				interp->sectorplane.sector->ceilingheight = R_LerpFixed(interp->sectorplane.oldheight, interp->sectorplane.bakheight, frac);
			}
			else
			{
				interp->sectorplane.sector->floorheight = R_LerpFixed(interp->sectorplane.oldheight, interp->sectorplane.bakheight, frac);
			}
			interp->sectorplane.sector->moved = true;
			break;
		case LVLINTERP_SectorScroll:
			if (interp->sectorscroll.ceiling)
			{
				interp->sectorscroll.sector->ceiling_xoffs = R_LerpFixed(interp->sectorscroll.oldxoffs, interp->sectorscroll.bakxoffs, frac);
				interp->sectorscroll.sector->ceiling_yoffs = R_LerpFixed(interp->sectorscroll.oldyoffs, interp->sectorscroll.bakyoffs, frac);
			}
			else
			{
				interp->sectorscroll.sector->floor_xoffs = R_LerpFixed(interp->sectorscroll.oldxoffs, interp->sectorscroll.bakxoffs, frac);
				interp->sectorscroll.sector->floor_yoffs = R_LerpFixed(interp->sectorscroll.oldyoffs, interp->sectorscroll.bakyoffs, frac);
			}
			break;
		case LVLINTERP_SideScroll:
			interp->sidescroll.side->textureoffset = R_LerpFixed(interp->sidescroll.oldtextureoffset, interp->sidescroll.baktextureoffset, frac);
			interp->sidescroll.side->rowoffset = R_LerpFixed(interp->sidescroll.oldrowoffset, interp->sidescroll.bakrowoffset, frac);
			break;
		case LVLINTERP_Polyobj:
			for (ii = 0; ii < interp->polyobj.vertices_size; ii++)
			{
				interp->polyobj.polyobj->vertices[ii]->x = R_LerpFixed(interp->polyobj.oldvertices[ii * 2    ], interp->polyobj.bakvertices[ii * 2    ], frac);
				interp->polyobj.polyobj->vertices[ii]->y = R_LerpFixed(interp->polyobj.oldvertices[ii * 2 + 1], interp->polyobj.bakvertices[ii * 2 + 1], frac);
			}
			interp->polyobj.polyobj->centerPt.x = R_LerpFixed(interp->polyobj.oldcx, interp->polyobj.bakcx, frac);
			interp->polyobj.polyobj->centerPt.y = R_LerpFixed(interp->polyobj.oldcy, interp->polyobj.bakcy, frac);
			break;
		case LVLINTERP_DynSlope:
			R_LerpVector3(&interp->dynslope.oldo, &interp->dynslope.bako, frac, &interp->dynslope.slope->o);
			R_LerpVector2(&interp->dynslope.oldd, &interp->dynslope.bakd, frac, &interp->dynslope.slope->d);
			interp->dynslope.slope->zdelta = R_LerpFixed(interp->dynslope.oldzdelta, interp->dynslope.bakzdelta, frac);
			break;
		}
	}
}

void R_RestoreLevelInterpolators(void)
{
	size_t ii;

	for (levelinterpolator_t& i : levelinterpolators)
	{
		levelinterpolator_t* interp = &i;

		switch (interp->type)
		{
		case LVLINTERP_SectorPlane:
			if (interp->sectorplane.ceiling)
			{
				interp->sectorplane.sector->ceilingheight = interp->sectorplane.bakheight;
			}
			else
			{
				interp->sectorplane.sector->floorheight = interp->sectorplane.bakheight;
			}
			interp->sectorplane.sector->moved = true;
			break;
		case LVLINTERP_SectorScroll:
			if (interp->sectorscroll.ceiling)
			{
				interp->sectorscroll.sector->ceiling_xoffs = interp->sectorscroll.bakxoffs;
				interp->sectorscroll.sector->ceiling_yoffs = interp->sectorscroll.bakyoffs;
			}
			else
			{
				interp->sectorscroll.sector->floor_xoffs = interp->sectorscroll.bakxoffs;
				interp->sectorscroll.sector->floor_yoffs = interp->sectorscroll.bakyoffs;
			}
			break;
		case LVLINTERP_SideScroll:
			interp->sidescroll.side->textureoffset = interp->sidescroll.baktextureoffset;
			interp->sidescroll.side->rowoffset = interp->sidescroll.bakrowoffset;
			break;
		case LVLINTERP_Polyobj:
			for (ii = 0; ii < interp->polyobj.vertices_size; ii++)
			{
				interp->polyobj.polyobj->vertices[ii]->x = interp->polyobj.bakvertices[ii * 2    ];
				interp->polyobj.polyobj->vertices[ii]->y = interp->polyobj.bakvertices[ii * 2 + 1];
			}
			interp->polyobj.polyobj->centerPt.x = interp->polyobj.bakcx;
			interp->polyobj.polyobj->centerPt.y = interp->polyobj.bakcy;
			break;
		case LVLINTERP_DynSlope:
			FV3_Copy(&interp->dynslope.slope->o, &interp->dynslope.bako);
			FV2_Copy(&interp->dynslope.slope->d, &interp->dynslope.bakd);
			interp->dynslope.slope->zdelta = interp->dynslope.bakzdelta;
			break;
		}
	}
}

void R_DestroyLevelInterpolators(thinker_t *thinker)
{
	size_t i;

	for (i = 0; i < levelinterpolators.size(); i++)
	{
		levelinterpolator_t* interp = &levelinterpolators[i];

		if (interp->thinker == thinker)
		{
			// Swap the tail of the level interpolators to this spot
			levelinterpolators[i] = *levelinterpolators.rbegin();

			levelinterpolators.pop_back();
		}
	}
}

static std::vector<mobj_t*> interpolated_mobjs;

// NOTE: This will NOT check that the mobj has already been added, for perf
// reasons.
void R_AddMobjInterpolator(mobj_t *mobj)
{
	interpolated_mobjs.push_back(mobj);

	R_ResetMobjInterpolationState(mobj);
	mobj->resetinterp = true;
}

void R_RemoveMobjInterpolator(mobj_t *mobj)
{
	for (size_t i = 0; i < interpolated_mobjs.size(); i++)
	{
		if (interpolated_mobjs[i] == mobj)
		{
			interpolated_mobjs[i] = *interpolated_mobjs.rbegin();
			interpolated_mobjs.pop_back();
			return;
		}
	}
}

void R_InitMobjInterpolators(void)
{
	interpolated_mobjs.clear();
}

void R_UpdateMobjInterpolators(void)
{
	for (mobj_t* mobj : interpolated_mobjs)
	{
		if (!P_MobjWasRemoved(mobj))
			R_ResetMobjInterpolationState(mobj);
	}
}

//
// P_ResetMobjInterpolationState
//
// Reset the rendering interpolation state of the mobj.
//
void R_ResetMobjInterpolationState(mobj_t *mobj)
{
	mobj->old_x2 = mobj->old_x;
	mobj->old_y2 = mobj->old_y;
	mobj->old_z2 = mobj->old_z;
	mobj->old_angle2 = mobj->old_angle;
	mobj->old_pitch2 = mobj->old_pitch;
	mobj->old_roll2 = mobj->old_roll;
	mobj->old_scale2 = mobj->old_scale;
	mobj->old_x = mobj->x;
	mobj->old_y = mobj->y;
	mobj->old_z = mobj->z;
	mobj->old_angle = mobj->angle;
	mobj->old_pitch = mobj->pitch;
	mobj->old_roll = mobj->roll;
	mobj->old_scale = mobj->scale;
	mobj->old_spritexscale = mobj->spritexscale;
	mobj->old_spriteyscale = mobj->spriteyscale;
	mobj->old_spritexoffset = mobj->spritexoffset;
	mobj->old_spriteyoffset = mobj->spriteyoffset;

	if (mobj->player)
	{
		mobj->player->old_drawangle2 = mobj->player->old_drawangle;
		mobj->player->old_drawangle = mobj->player->drawangle;
	}

	mobj->resetinterp = false;
}

//
// P_ResetPrecipitationMobjInterpolationState
//
// Reset the rendering interpolation state of the precipmobj.
//
void R_ResetPrecipitationMobjInterpolationState(precipmobj_t *mobj)
{
	mobj->old_x2 = mobj->old_x;
	mobj->old_y2 = mobj->old_y;
	mobj->old_z2 = mobj->old_z;
	mobj->old_angle2 = mobj->old_angle;
	mobj->old_pitch2 = mobj->old_pitch;
	mobj->old_roll2 = mobj->old_roll;
	mobj->old_x = mobj->x;
	mobj->old_y = mobj->y;
	mobj->old_z = mobj->z;
	mobj->old_angle = mobj->angle;
	mobj->old_spritexscale = mobj->spritexscale;
	mobj->old_spriteyscale = mobj->spriteyscale;
	mobj->old_spritexoffset = mobj->spritexoffset;
	mobj->old_spriteyoffset = mobj->spriteyoffset;
}
