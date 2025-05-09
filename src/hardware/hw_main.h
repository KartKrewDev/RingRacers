// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_main.h
/// \brief 3D render mode functions

#ifndef __HWR_MAIN_H__
#define __HWR_MAIN_H__

#include "hw_data.h"
#include "hw_defs.h"

#include "../am_map.h"
#include "../d_player.h"
#include "../r_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Startup & Shutdown the hardware mode renderer
void HWR_Startup(void);
void HWR_Switch(void);
void HWR_Shutdown(void);

void HWR_drawAMline(const fline_t *fl, INT32 color);
void HWR_FadeScreenMenuBack(UINT16 color, UINT8 strength);
void HWR_DrawConsoleBack(UINT32 color, INT32 height);
void HWR_EncoreInvertScreen(void);
void HWR_DrawCustomFadeScreen(UINT8 color, UINT8 strength);
void HWR_DrawTutorialBack(UINT32 color, INT32 boxheight);
void HWR_RenderSkyboxView(player_t *player);
void HWR_RenderPlayerView(void);
void HWR_ClearSkyDome(void);
void HWR_BuildSkyDome(void);
void HWR_DrawViewBorder(INT32 clearlines);
void HWR_DrawFlatFill(INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatlumpnum);
void HWR_InitTextureMapping(void);
void HWR_SetViewSize(void);
void HWR_DrawPatch(patch_t *gpatch, INT32 x, INT32 y, INT32 option);
void HWR_DrawStretchyFixedPatch(patch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 option, const UINT8 *colormap);
void HWR_DrawCroppedPatch(patch_t *gpatch, fixed_t x, fixed_t y, fixed_t scale, INT32 option, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h);
void HWR_MakePatch(const patch_t *patch, GLPatch_t *grPatch, GLMipmap_t *grMipmap, boolean makebitmap);
void HWR_CreatePlanePolygons(INT32 bspnum);
void HWR_CreateStaticLightmaps(INT32 bspnum);
void HWR_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color);
void HWR_DrawFadeFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color, UINT16 actualcolor, UINT8 strength);
void HWR_DrawConsoleFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color, UINT32 actualcolor);	// Lat: separate flags from color since color needs to be an uint to work right.
void HWR_DrawDiag(INT32 x, INT32 y, INT32 wh, INT32 color);
void HWR_DrawPic(INT32 x,INT32 y,lumpnum_t lumpnum);

UINT8 *HWR_GetScreenshot(void);
boolean HWR_Screenshot(const char *pathname);

void HWR_AddCommands(void);
void HWR_AddSessionCommands(void);
void transform(float *cx, float *cy, float *cz);
INT32 HWR_GetTextureUsed(void);
void HWR_DoPostProcessor(player_t *player);
void HWR_StartScreenWipe(void);
void HWR_EndScreenWipe(void);
void HWR_DrawIntermissionBG(void);
void HWR_DoWipe(UINT8 wipenum, UINT8 scrnnum);
void HWR_DoTintedWipe(UINT8 wipenum, UINT8 scrnnum);
void HWR_MakeScreenFinalTexture(void);
void HWR_DrawScreenFinalTexture(int width, int height);

// This stuff is put here so models can use them
boolean HWR_OverrideObjectLightLevel(mobj_t *thing, INT32 *lightlevel);
void HWR_Lighting(FSurfaceInfo *Surface, INT32 light_level, extracolormap_t *colormap, const boolean directional);
UINT8 HWR_FogBlockAlpha(INT32 light, extracolormap_t *colormap); // Let's see if this can work

UINT8 HWR_GetTranstableAlpha(INT32 transtablenum);
FBITFIELD HWR_GetBlendModeFlag(INT32 ast);
FBITFIELD HWR_SurfaceBlend(INT32 style, INT32 transtablenum, FSurfaceInfo *pSurf);
FBITFIELD HWR_TranstableToAlpha(INT32 transtablenum, FSurfaceInfo *pSurf);

boolean HWR_CompileShaders(void);

void HWR_LoadAllCustomShaders(void);
void HWR_LoadCustomShadersFromFile(UINT16 wadnum, boolean PK3);
const char *HWR_GetShaderName(INT32 shader);

extern customshaderxlat_t shaderxlat[];

extern CV_PossibleValue_t glanisotropicmode_cons_t[];

#ifdef ALAM_LIGHTING
extern consvar_t cv_gldynamiclighting;
extern consvar_t cv_glstaticlighting;
extern consvar_t cv_glcoronas;
extern consvar_t cv_glcoronasize;
#endif

extern consvar_t cv_glshaders, cv_glallowshaders;
extern consvar_t cv_glmodels;

// SRB2Kart: We don't like these options.
// Interpolation should be up to who animated the model.
// Lighting makes the modeler's intended texturing look funky.
//#define BAD_MODEL_OPTIONS

#ifdef BAD_MODEL_OPTIONS
extern consvar_t cv_glmodelinterpolation;
extern consvar_t cv_glmodellighting;
#endif

extern consvar_t cv_glfiltermode;
extern consvar_t cv_glanisotropicmode;
extern consvar_t cv_fovchange;
extern consvar_t cv_glsolvetjoin;
extern consvar_t cv_glshearing;
extern consvar_t cv_glspritebillboarding;
extern consvar_t cv_glskydome;

extern consvar_t cv_glbatching;

extern float gl_viewwidth, gl_viewheight, gl_baseviewwindowy;

extern float gl_viewwindowx, gl_basewindowcentery;

// BP: big hack for a test in lighting ref : 1249753487AB
extern fixed_t *hwbbox;
extern FTransform atransform;
extern float gl_viewsin, gl_viewcos;


// Render stats
extern precise_t ps_hw_skyboxtime;
extern precise_t ps_hw_nodesorttime;
extern precise_t ps_hw_nodedrawtime;
extern precise_t ps_hw_spritesorttime;
extern precise_t ps_hw_spritedrawtime;

// Render stats for batching
extern int ps_hw_numpolys;
extern int ps_hw_numverts;
extern int ps_hw_numcalls;
extern int ps_hw_numshaders;
extern int ps_hw_numtextures;
extern int ps_hw_numpolyflags;
extern int ps_hw_numcolors;
extern precise_t ps_hw_batchsorttime;
extern precise_t ps_hw_batchdrawtime;

extern boolean gl_init;
extern boolean gl_maploaded;
extern boolean gl_maptexturesloaded;
extern boolean gl_sessioncommandsadded;
extern boolean gl_shadersavailable;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
