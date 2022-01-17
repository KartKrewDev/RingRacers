// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2000 by Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze, Andrey Budko (prboom)
// Copyright (C) 1999-2019 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_fps.h
/// \brief Uncapped framerate stuff.

#include "r_fps.h"

#include "r_main.h"
#include "g_game.h"
#include "i_video.h"
#include "r_plane.h"
#include "p_spec.h"
#include "r_state.h"
#ifdef HWRENDER
#include "hardware/hw_main.h" // for cv_glshearing
#endif

static viewvars_t pview_old[MAXSPLITSCREENPLAYERS];
static viewvars_t pview_new[MAXSPLITSCREENPLAYERS];
static viewvars_t skyview_old[MAXSPLITSCREENPLAYERS];
static viewvars_t skyview_new[MAXSPLITSCREENPLAYERS];

static viewvars_t *oldview = &pview_old[0];
viewvars_t *newview = &pview_new[0];

enum viewcontext_e viewcontext = VIEWCONTEXT_PLAYER1;

static fixed_t R_LerpFixed(fixed_t from, fixed_t to, fixed_t frac)
{
	return FixedMul(frac, to - from);
}

static angle_t R_LerpAngle(angle_t from, angle_t to, fixed_t frac)
{
	return FixedMul(frac, to - from);
}

// recalc necessary stuff for mouseaiming
// slopes are already calculated for the full possible view (which is 4*viewheight).
// 18/08/18: (No it's actually 16*viewheight, thanks Jimita for finding this out)
static void R_SetupFreelook(player_t *player, boolean skybox)
{
#ifndef HWRENDER
	(void)player;
	(void)skybox;
#endif

	// clip it in the case we are looking a hardware 90 degrees full aiming
	// (lmps, network and use F12...)
	if (rendermode == render_soft
#ifdef HWRENDER
		|| (rendermode == render_opengl
			&& (cv_glshearing.value == 1
			|| (cv_glshearing.value == 2 && R_IsViewpointThirdPerson(player, skybox))))
#endif
		)
	{
		G_SoftwareClipAimingPitch((INT32 *)&aimingangle);
	}

	centeryfrac = (viewheight/2)<<FRACBITS;

	if (rendermode == render_soft)
		centeryfrac += FixedMul(AIMINGTODY(aimingangle), FixedDiv(viewwidth<<FRACBITS, BASEVIDWIDTH<<FRACBITS));

	centery = FixedInt(FixedRound(centeryfrac));

	if (rendermode == render_soft)
		yslope = &yslopetab[viewssnum][viewheight*8 - centery];
}

#undef AIMINGTODY

void R_InterpolateView(fixed_t frac)
{
	if (frac < 0)
		frac = 0;
	if (frac > FRACUNIT)
		frac = FRACUNIT;

	viewx = oldview->x + R_LerpFixed(oldview->x, newview->x, frac);
	viewy = oldview->y + R_LerpFixed(oldview->y, newview->y, frac);
	viewz = oldview->z + R_LerpFixed(oldview->z, newview->z, frac);

	viewangle = oldview->angle + R_LerpAngle(oldview->angle, newview->angle, frac);
	aimingangle = oldview->aim + R_LerpAngle(oldview->aim, newview->aim, frac);
	viewroll = oldview->roll + R_LerpAngle(oldview->roll, newview->roll, frac);

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
	}
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
	if (cv_frameinterpolation.value == 0)
	{
		return to;
	}

	return (from + R_LerpFixed(from, to, rendertimefrac));
}

angle_t R_InterpolateAngle(angle_t from, angle_t to)
{
	if (cv_frameinterpolation.value == 0)
	{
		return to;
	}

	return (from + R_LerpAngle(from, to, rendertimefrac));
}
