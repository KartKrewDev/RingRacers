// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
<<<<<<< HEAD
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2019 by Sonic Team Junior.
=======
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
>>>>>>> srb2/next
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
<<<<<<< HEAD
/// \file
/// \brief imports/exports for the GPU hardware low-level interface API
=======
/// \file hw_drv.h
/// \brief imports/exports for the 3D hardware low-level interface API
>>>>>>> srb2/next

#ifndef __HWR_DRV_H__
#define __HWR_DRV_H__

// this must be here 19991024 by Kin
#include "../screen.h"
#include "hw_data.h"
#include "hw_defs.h"
#include "hw_md2.h"

#include "hw_dll.h"

// ==========================================================================
//                                                       STANDARD DLL EXPORTS
// ==========================================================================

EXPORT boolean HWRAPI(Init) (void);
#ifndef HAVE_SDL
EXPORT void HWRAPI(Shutdown) (void);
#endif
#ifdef _WINDOWS
EXPORT void HWRAPI(GetModeList) (vmode_t **pvidmodes, INT32 *numvidmodes);
#endif
EXPORT void HWRAPI(SetPalette) (RGBA_t *ppal);
EXPORT void HWRAPI(FinishUpdate) (INT32 waitvbl);
EXPORT void HWRAPI(Draw2DLine) (F2DCoord *v1, F2DCoord *v2, RGBA_t Color);
EXPORT void HWRAPI(DrawPolygon) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags);
EXPORT void HWRAPI(DrawIndexedTriangles) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, UINT32 *IndexArray);
EXPORT void HWRAPI(RenderSkyDome) (gl_sky_t *sky);
EXPORT void HWRAPI(SetBlend) (FBITFIELD PolyFlags);
EXPORT void HWRAPI(ClearBuffer) (FBOOLEAN ColorMask, FBOOLEAN DepthMask, FRGBAFloat *ClearColor);
EXPORT void HWRAPI(SetTexture) (FTextureInfo *TexInfo);
EXPORT void HWRAPI(UpdateTexture) (FTextureInfo *TexInfo);
EXPORT void HWRAPI(ReadRect) (INT32 x, INT32 y, INT32 width, INT32 height, INT32 dst_stride, UINT16 *dst_data);
EXPORT void HWRAPI(GClipRect) (INT32 minx, INT32 miny, INT32 maxx, INT32 maxy, float nearclip);
EXPORT void HWRAPI(ClearMipMapCache) (void);

//Hurdler: added for backward compatibility
EXPORT void HWRAPI(SetSpecialState) (hwdspecialstate_t IdState, INT32 Value);

//Hurdler: added for new development
<<<<<<< HEAD
EXPORT void HWRAPI(DrawModel) (model_t *model, INT32 frameIndex, INT32 duration, INT32 tics, INT32 nextFrameIndex, FTransform *pos, float scale, UINT8 flipped, FSurfaceInfo *Surface);
=======
EXPORT void HWRAPI(DrawModel) (model_t *model, INT32 frameIndex, INT32 duration, INT32 tics, INT32 nextFrameIndex, FTransform *pos, float scale, UINT8 flipped, UINT8 hflipped, FSurfaceInfo *Surface);
>>>>>>> srb2/next
EXPORT void HWRAPI(CreateModelVBOs) (model_t *model);
EXPORT void HWRAPI(SetTransform) (FTransform *stransform);
EXPORT INT32 HWRAPI(GetTextureUsed) (void);
<<<<<<< HEAD

EXPORT void HWRAPI(RenderSkyDome) (INT32 tex, INT32 texture_width, INT32 texture_height, FTransform transform);
=======
>>>>>>> srb2/next

EXPORT void HWRAPI(FlushScreenTextures) (void);
EXPORT void HWRAPI(StartScreenWipe) (void);
EXPORT void HWRAPI(EndScreenWipe) (void);
EXPORT void HWRAPI(DoScreenWipe) (void);
EXPORT void HWRAPI(DrawIntermissionBG) (void);
EXPORT void HWRAPI(MakeScreenTexture) (void);
EXPORT void HWRAPI(MakeScreenFinalTexture) (void);
EXPORT void HWRAPI(DrawScreenFinalTexture) (int width, int height);

#define SCREENVERTS 10
EXPORT void HWRAPI(PostImgRedraw) (float points[SCREENVERTS][SCREENVERTS][2]);

// jimita
EXPORT boolean HWRAPI(LoadShaders) (void);
EXPORT void HWRAPI(KillShaders) (void);
EXPORT void HWRAPI(SetShader) (int shader);
EXPORT void HWRAPI(UnSetShader) (void);

<<<<<<< HEAD
EXPORT void HWRAPI(LoadCustomShader) (int number, char *shader, size_t size, boolean fragment);
EXPORT void HWRAPI(InitCustomShaders) (void);

EXPORT void HWRAPI(StartBatching) (void);
EXPORT void HWRAPI(RenderBatches) (int *sNumPolys, int *sNumVerts, int *sNumCalls, int *sNumShaders, int *sNumTextures, int *sNumPolyFlags, int *sNumColors);
=======
EXPORT void HWRAPI(SetShaderInfo) (hwdshaderinfo_t info, INT32 value);
EXPORT void HWRAPI(LoadCustomShader) (int number, char *shader, size_t size, boolean fragment);
EXPORT boolean HWRAPI(InitCustomShaders) (void);
>>>>>>> srb2/next

// ==========================================================================
//                                      HWR DRIVER OBJECT, FOR CLIENT PROGRAM
// ==========================================================================

#if !defined (_CREATE_DLL_)

struct hwdriver_s
{
	Init                pfnInit;
	SetPalette          pfnSetPalette;
	FinishUpdate        pfnFinishUpdate;
	Draw2DLine          pfnDraw2DLine;
	DrawPolygon         pfnDrawPolygon;
	DrawIndexedTriangles    pfnDrawIndexedTriangles;
	RenderSkyDome       pfnRenderSkyDome;
	SetBlend            pfnSetBlend;
	ClearBuffer         pfnClearBuffer;
	SetTexture          pfnSetTexture;
	UpdateTexture       pfnUpdateTexture;
	ReadRect            pfnReadRect;
	GClipRect           pfnGClipRect;
	ClearMipMapCache    pfnClearMipMapCache;
	SetSpecialState     pfnSetSpecialState;//Hurdler: added for backward compatibility
	DrawModel           pfnDrawModel;
	CreateModelVBOs     pfnCreateModelVBOs;
	SetTransform        pfnSetTransform;
	GetTextureUsed      pfnGetTextureUsed;
#ifdef _WINDOWS
	GetModeList         pfnGetModeList;
#endif
#ifndef HAVE_SDL
	Shutdown            pfnShutdown;
#endif
	PostImgRedraw       pfnPostImgRedraw;
	FlushScreenTextures pfnFlushScreenTextures;
	StartScreenWipe     pfnStartScreenWipe;
	EndScreenWipe       pfnEndScreenWipe;
	DoScreenWipe        pfnDoScreenWipe;
	DrawIntermissionBG  pfnDrawIntermissionBG;
	MakeScreenTexture   pfnMakeScreenTexture;
	MakeScreenFinalTexture  pfnMakeScreenFinalTexture;
	DrawScreenFinalTexture  pfnDrawScreenFinalTexture;

<<<<<<< HEAD
	RenderSkyDome pfnRenderSkyDome;

	LoadShaders pfnLoadShaders;
	KillShaders pfnKillShaders;
	SetShader pfnSetShader;
	UnSetShader pfnUnSetShader;

	LoadCustomShader pfnLoadCustomShader;
	InitCustomShaders pfnInitCustomShaders;

	StartBatching pfnStartBatching;
	RenderBatches pfnRenderBatches;
=======
	LoadShaders         pfnLoadShaders;
	KillShaders         pfnKillShaders;
	SetShader           pfnSetShader;
	UnSetShader         pfnUnSetShader;

	SetShaderInfo       pfnSetShaderInfo;
	LoadCustomShader    pfnLoadCustomShader;
	InitCustomShaders   pfnInitCustomShaders;
>>>>>>> srb2/next
};

extern struct hwdriver_s hwdriver;

#define HWD hwdriver

#endif //not defined _CREATE_DLL_

#endif //__HWR_DRV_H__