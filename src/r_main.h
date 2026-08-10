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
/// \file  r_main.h
/// \brief Rendering variables, consvars, defines

#ifndef R_MAIN_H
#define R_MAIN_H

#include "d_player.h"
#include "r_data.h"
#include "r_textures.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// POV related.
//
extern fixed_t viewcos, viewsin;
extern int32_t viewheight;
extern int32_t centerx, centery;

extern fixed_t centerxfrac;
extern fixed_t centeryfrac;
extern fixed_t projection[MAXSPLITSCREENPLAYERS];
extern fixed_t projectiony[MAXSPLITSCREENPLAYERS];
extern fixed_t fovtan[MAXSPLITSCREENPLAYERS];
extern fixed_t g_fovcache[MAXSPLITSCREENPLAYERS];

extern size_t validcount, linecount, loopcount, framecount;

// The fraction of a tic being drawn (for interpolation between two tics)
extern fixed_t rendertimefrac;
// Same as rendertimefrac but not suspended when the game is paused
extern fixed_t rendertimefrac_unpaused;
// Evaluated delta tics for this frame (how many tics since the last frame)
extern fixed_t renderdeltatics;
// The current render is a new logical tic
extern dboolean renderisnewtic;

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.
// Now with 32 levels.
// LIGHTLEVELS is now defined in r_state.h
#define LIGHTSEGSHIFT 3

#define MAXLIGHTSCALE 48
#define LIGHTSCALESHIFT 12
#define MAXLIGHTZ 128
#define LIGHTZSHIFT 20

#define LIGHTRESOLUTIONFIX (640*fovtan[viewssnum]/vid.width)

extern lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern lighttable_t *scalelightfixed[MAXLIGHTSCALE];
extern lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS 32

// Utility functions.
int32_t R_PointOnSide(fixed_t x, fixed_t y, node_t *node);
int32_t R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line);
angle_t R_PointToAngle(fixed_t x, fixed_t y);
angle_t R_PointToAnglePlayer(player_t *player, fixed_t x, fixed_t y);
angle_t R_PointToAngle64(int64_t x, int64_t y);
angle_t R_PointToAngle2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1);
angle_t R_PointToAngleEx(int32_t x2, int32_t y2, int32_t x1, int32_t y1);
fixed_t R_PointToDist(fixed_t x, fixed_t y);
fixed_t R_PointToDist2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1);

fixed_t R_ScaleFromGlobalAngle(angle_t visangle);
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);
subsector_t *R_PointInSubsectorOrNull(fixed_t x, fixed_t y);

dboolean R_DoCulling(line_t *cullheight, line_t *viewcullheight, fixed_t vz, fixed_t bottomh, fixed_t toph);

void R_GetRenderBlockMapDimensions(fixed_t drawdist, int32_t *xl, int32_t *xh, int32_t *yl, int32_t *yh);

// Render stats

extern precise_t ps_prevframetime;// time when previous frame was rendered
extern precise_t ps_rendercalltime;
extern precise_t ps_uitime;
extern precise_t ps_swaptime;

extern precise_t ps_bsptime;

extern precise_t ps_sw_spritecliptime;
extern precise_t ps_sw_portaltime;
extern precise_t ps_sw_planetime;
extern precise_t ps_sw_maskedtime;

extern int ps_numbspcalls;
extern int ps_numsprites;
extern int ps_numdrawnodes;
extern int ps_numpolyobjects;

struct RenderStats
{
	size_t visplanes;
	size_t drawsegs;
	size_t skybox_portals;
};

extern struct RenderStats g_renderstats;

//
// REFRESH - the actual rendering functions.
//

extern consvar_t cv_showhud;
extern consvar_t cv_homremoval;
extern consvar_t cv_chasecam[MAXSPLITSCREENPLAYERS];

extern consvar_t cv_shadow;
extern consvar_t cv_ffloorclip;
extern consvar_t cv_drawdist, cv_drawdist_precip;
extern consvar_t cv_fov[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_skybox;
extern consvar_t cv_drawpickups;
extern consvar_t cv_debugfinishline;
extern consvar_t cv_drawinput;
extern consvar_t cv_drawtimer;
extern consvar_t cv_debugfonts;
extern consvar_t cv_vorpal;

// debugging

typedef enum {
	SW_HI_PLANES,
	SW_HI_FOFPLANES,
	SW_HI_FOFSIDES,
	SW_HI_MIDTEXTURES,
	SW_HI_WALLS,
	SW_HI_THINGS,
	SW_HI_SKY,

	NUM_SW_HI
} debugrender_highlight_t;

extern uint32_t debugrender_highlight;

void R_CheckDebugHighlight(debugrender_highlight_t type);
int32_t R_AdjustLightLevel(int32_t light);
uint8_t R_DebugLineColor(const line_t *ld);

void Command_Debugrender_highlight(void);

extern consvar_t
	cv_debugrender_contrast,
	cv_debugrender_spriteclip,
	cv_debugrender_portal;

// Called by startup code.
void R_Init(void);

void R_CheckViewMorph(int split);
void R_ApplyViewMorph(int split);
angle_t R_ViewRollAngle(const player_t *player, uint8_t viewnum);

// just sets setsizeneeded true
extern dboolean setsizeneeded;
void R_SetViewSize(void);

// do it (sometimes explicitly called)
void R_ExecuteSetViewSize(void);

fixed_t R_FOV(int split);
void R_CheckFOV(void);

dboolean R_ShowHUD(void);

void R_SetupFrame(int split);
void R_SkyboxFrame(int split);

dboolean R_ViewpointHasChasecam(player_t *player);
dboolean R_IsViewpointThirdPerson(player_t *player, dboolean skybox);

// Called by D_Display.
void R_RenderPlayerView(void);

// add commands related to engine, at game startup
void R_RegisterEngineStuff(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
