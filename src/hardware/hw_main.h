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

#ifndef HWR_MAIN_H
#define HWR_MAIN_H

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

void HWR_drawAMline(const fline_t *fl, int32_t color);
void HWR_FadeScreenMenuBack(uint16_t color, uint8_t strength);
void HWR_DrawConsoleBack(uint32_t color, int32_t height);
void HWR_EncoreInvertScreen(void);
void HWR_DrawCustomFadeScreen(uint8_t color, uint8_t strength);
void HWR_DrawTutorialBack(uint32_t color, int32_t boxheight);
void HWR_RenderSkyboxView(player_t *player);
void HWR_RenderPlayerView(void);
void HWR_ClearSkyDome(void);
void HWR_BuildSkyDome(void);
void HWR_DrawViewBorder(int32_t clearlines);
void HWR_DrawFlatFill(int32_t x, int32_t y, int32_t w, int32_t h, lumpnum_t flatlumpnum);
void HWR_InitTextureMapping(void);
void HWR_SetViewSize(void);
void HWR_DrawPatch(patch_t *gpatch, int32_t x, int32_t y, int32_t option);
void HWR_DrawStretchyFixedPatch(patch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, int32_t option, const uint8_t *colormap);
void HWR_DrawCroppedPatch(patch_t *gpatch, fixed_t x, fixed_t y, fixed_t scale, int32_t option, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h);
void HWR_MakePatch(const patch_t *patch, GLPatch_t *grPatch, GLMipmap_t *grMipmap, dboolean makebitmap);
void HWR_CreatePlanePolygons(int32_t bspnum);
void HWR_CreateStaticLightmaps(int32_t bspnum);
void HWR_DrawFill(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
void HWR_DrawFadeFill(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color, uint16_t actualcolor, uint8_t strength);
void HWR_DrawConsoleFill(int32_t x, int32_t y, int32_t w, int32_t h, int32_t color, uint32_t actualcolor);	// Lat: separate flags from color since color needs to be an uint to work right.
void HWR_DrawDiag(int32_t x, int32_t y, int32_t wh, int32_t color);
void HWR_DrawPic(int32_t x,int32_t y,lumpnum_t lumpnum);

uint8_t *HWR_GetScreenshot(void);
dboolean HWR_Screenshot(const char *pathname);

void HWR_AddCommands(void);
void HWR_AddSessionCommands(void);
void transform(float *cx, float *cy, float *cz);
int32_t HWR_GetTextureUsed(void);
void HWR_DoPostProcessor(player_t *player);
void HWR_StartScreenWipe(void);
void HWR_EndScreenWipe(void);
void HWR_DrawIntermissionBG(void);
void HWR_DoWipe(uint8_t wipenum, uint8_t scrnnum);
void HWR_DoTintedWipe(uint8_t wipenum, uint8_t scrnnum);
void HWR_MakeScreenFinalTexture(void);
void HWR_DrawScreenFinalTexture(int width, int height);

// This stuff is put here so models can use them
dboolean HWR_OverrideObjectLightLevel(mobj_t *thing, int32_t *lightlevel);
void HWR_Lighting(FSurfaceInfo *Surface, int32_t light_level, extracolormap_t *colormap, const dboolean directional);
uint8_t HWR_FogBlockAlpha(int32_t light, extracolormap_t *colormap); // Let's see if this can work

uint8_t HWR_GetTranstableAlpha(int32_t transtablenum);
FBITFIELD HWR_GetBlendModeFlag(int32_t ast);
FBITFIELD HWR_SurfaceBlend(int32_t style, int32_t transtablenum, FSurfaceInfo *pSurf);
FBITFIELD HWR_TranstableToAlpha(int32_t transtablenum, FSurfaceInfo *pSurf);

dboolean HWR_CompileShaders(void);

void HWR_LoadAllCustomShaders(void);
void HWR_LoadCustomShadersFromFile(uint16_t wadnum, dboolean PK3);
const char *HWR_GetShaderName(int32_t shader);

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

extern dboolean gl_init;
extern dboolean gl_maploaded;
extern dboolean gl_maptexturesloaded;
extern dboolean gl_sessioncommandsadded;
extern dboolean gl_shadersavailable;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
