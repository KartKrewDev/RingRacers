/**
 * SPDX-License-Identifier: (WTFPL OR CC0-1.0) AND Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/gl.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */

#ifdef __cplusplus
extern "C" {
#endif








static void glad_gl_load_GL_VERSION_1_0(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_1_0) return;
    context->Accum = (PFNGLACCUMPROC) load(userptr, "glAccum");
    context->AlphaFunc = (PFNGLALPHAFUNCPROC) load(userptr, "glAlphaFunc");
    context->Begin = (PFNGLBEGINPROC) load(userptr, "glBegin");
    context->Bitmap = (PFNGLBITMAPPROC) load(userptr, "glBitmap");
    context->BlendFunc = (PFNGLBLENDFUNCPROC) load(userptr, "glBlendFunc");
    context->CallList = (PFNGLCALLLISTPROC) load(userptr, "glCallList");
    context->CallLists = (PFNGLCALLLISTSPROC) load(userptr, "glCallLists");
    context->Clear = (PFNGLCLEARPROC) load(userptr, "glClear");
    context->ClearAccum = (PFNGLCLEARACCUMPROC) load(userptr, "glClearAccum");
    context->ClearColor = (PFNGLCLEARCOLORPROC) load(userptr, "glClearColor");
    context->ClearDepth = (PFNGLCLEARDEPTHPROC) load(userptr, "glClearDepth");
    context->ClearIndex = (PFNGLCLEARINDEXPROC) load(userptr, "glClearIndex");
    context->ClearStencil = (PFNGLCLEARSTENCILPROC) load(userptr, "glClearStencil");
    context->ClipPlane = (PFNGLCLIPPLANEPROC) load(userptr, "glClipPlane");
    context->Color3b = (PFNGLCOLOR3BPROC) load(userptr, "glColor3b");
    context->Color3bv = (PFNGLCOLOR3BVPROC) load(userptr, "glColor3bv");
    context->Color3d = (PFNGLCOLOR3DPROC) load(userptr, "glColor3d");
    context->Color3dv = (PFNGLCOLOR3DVPROC) load(userptr, "glColor3dv");
    context->Color3f = (PFNGLCOLOR3FPROC) load(userptr, "glColor3f");
    context->Color3fv = (PFNGLCOLOR3FVPROC) load(userptr, "glColor3fv");
    context->Color3i = (PFNGLCOLOR3IPROC) load(userptr, "glColor3i");
    context->Color3iv = (PFNGLCOLOR3IVPROC) load(userptr, "glColor3iv");
    context->Color3s = (PFNGLCOLOR3SPROC) load(userptr, "glColor3s");
    context->Color3sv = (PFNGLCOLOR3SVPROC) load(userptr, "glColor3sv");
    context->Color3ub = (PFNGLCOLOR3UBPROC) load(userptr, "glColor3ub");
    context->Color3ubv = (PFNGLCOLOR3UBVPROC) load(userptr, "glColor3ubv");
    context->Color3ui = (PFNGLCOLOR3UIPROC) load(userptr, "glColor3ui");
    context->Color3uiv = (PFNGLCOLOR3UIVPROC) load(userptr, "glColor3uiv");
    context->Color3us = (PFNGLCOLOR3USPROC) load(userptr, "glColor3us");
    context->Color3usv = (PFNGLCOLOR3USVPROC) load(userptr, "glColor3usv");
    context->Color4b = (PFNGLCOLOR4BPROC) load(userptr, "glColor4b");
    context->Color4bv = (PFNGLCOLOR4BVPROC) load(userptr, "glColor4bv");
    context->Color4d = (PFNGLCOLOR4DPROC) load(userptr, "glColor4d");
    context->Color4dv = (PFNGLCOLOR4DVPROC) load(userptr, "glColor4dv");
    context->Color4f = (PFNGLCOLOR4FPROC) load(userptr, "glColor4f");
    context->Color4fv = (PFNGLCOLOR4FVPROC) load(userptr, "glColor4fv");
    context->Color4i = (PFNGLCOLOR4IPROC) load(userptr, "glColor4i");
    context->Color4iv = (PFNGLCOLOR4IVPROC) load(userptr, "glColor4iv");
    context->Color4s = (PFNGLCOLOR4SPROC) load(userptr, "glColor4s");
    context->Color4sv = (PFNGLCOLOR4SVPROC) load(userptr, "glColor4sv");
    context->Color4ub = (PFNGLCOLOR4UBPROC) load(userptr, "glColor4ub");
    context->Color4ubv = (PFNGLCOLOR4UBVPROC) load(userptr, "glColor4ubv");
    context->Color4ui = (PFNGLCOLOR4UIPROC) load(userptr, "glColor4ui");
    context->Color4uiv = (PFNGLCOLOR4UIVPROC) load(userptr, "glColor4uiv");
    context->Color4us = (PFNGLCOLOR4USPROC) load(userptr, "glColor4us");
    context->Color4usv = (PFNGLCOLOR4USVPROC) load(userptr, "glColor4usv");
    context->ColorMask = (PFNGLCOLORMASKPROC) load(userptr, "glColorMask");
    context->ColorMaterial = (PFNGLCOLORMATERIALPROC) load(userptr, "glColorMaterial");
    context->CopyPixels = (PFNGLCOPYPIXELSPROC) load(userptr, "glCopyPixels");
    context->CullFace = (PFNGLCULLFACEPROC) load(userptr, "glCullFace");
    context->DeleteLists = (PFNGLDELETELISTSPROC) load(userptr, "glDeleteLists");
    context->DepthFunc = (PFNGLDEPTHFUNCPROC) load(userptr, "glDepthFunc");
    context->DepthMask = (PFNGLDEPTHMASKPROC) load(userptr, "glDepthMask");
    context->DepthRange = (PFNGLDEPTHRANGEPROC) load(userptr, "glDepthRange");
    context->Disable = (PFNGLDISABLEPROC) load(userptr, "glDisable");
    context->DrawBuffer = (PFNGLDRAWBUFFERPROC) load(userptr, "glDrawBuffer");
    context->DrawPixels = (PFNGLDRAWPIXELSPROC) load(userptr, "glDrawPixels");
    context->EdgeFlag = (PFNGLEDGEFLAGPROC) load(userptr, "glEdgeFlag");
    context->EdgeFlagv = (PFNGLEDGEFLAGVPROC) load(userptr, "glEdgeFlagv");
    context->Enable = (PFNGLENABLEPROC) load(userptr, "glEnable");
    context->End = (PFNGLENDPROC) load(userptr, "glEnd");
    context->EndList = (PFNGLENDLISTPROC) load(userptr, "glEndList");
    context->EvalCoord1d = (PFNGLEVALCOORD1DPROC) load(userptr, "glEvalCoord1d");
    context->EvalCoord1dv = (PFNGLEVALCOORD1DVPROC) load(userptr, "glEvalCoord1dv");
    context->EvalCoord1f = (PFNGLEVALCOORD1FPROC) load(userptr, "glEvalCoord1f");
    context->EvalCoord1fv = (PFNGLEVALCOORD1FVPROC) load(userptr, "glEvalCoord1fv");
    context->EvalCoord2d = (PFNGLEVALCOORD2DPROC) load(userptr, "glEvalCoord2d");
    context->EvalCoord2dv = (PFNGLEVALCOORD2DVPROC) load(userptr, "glEvalCoord2dv");
    context->EvalCoord2f = (PFNGLEVALCOORD2FPROC) load(userptr, "glEvalCoord2f");
    context->EvalCoord2fv = (PFNGLEVALCOORD2FVPROC) load(userptr, "glEvalCoord2fv");
    context->EvalMesh1 = (PFNGLEVALMESH1PROC) load(userptr, "glEvalMesh1");
    context->EvalMesh2 = (PFNGLEVALMESH2PROC) load(userptr, "glEvalMesh2");
    context->EvalPoint1 = (PFNGLEVALPOINT1PROC) load(userptr, "glEvalPoint1");
    context->EvalPoint2 = (PFNGLEVALPOINT2PROC) load(userptr, "glEvalPoint2");
    context->FeedbackBuffer = (PFNGLFEEDBACKBUFFERPROC) load(userptr, "glFeedbackBuffer");
    context->Finish = (PFNGLFINISHPROC) load(userptr, "glFinish");
    context->Flush = (PFNGLFLUSHPROC) load(userptr, "glFlush");
    context->Fogf = (PFNGLFOGFPROC) load(userptr, "glFogf");
    context->Fogfv = (PFNGLFOGFVPROC) load(userptr, "glFogfv");
    context->Fogi = (PFNGLFOGIPROC) load(userptr, "glFogi");
    context->Fogiv = (PFNGLFOGIVPROC) load(userptr, "glFogiv");
    context->FrontFace = (PFNGLFRONTFACEPROC) load(userptr, "glFrontFace");
    context->Frustum = (PFNGLFRUSTUMPROC) load(userptr, "glFrustum");
    context->GenLists = (PFNGLGENLISTSPROC) load(userptr, "glGenLists");
    context->GetBooleanv = (PFNGLGETBOOLEANVPROC) load(userptr, "glGetBooleanv");
    context->GetClipPlane = (PFNGLGETCLIPPLANEPROC) load(userptr, "glGetClipPlane");
    context->GetDoublev = (PFNGLGETDOUBLEVPROC) load(userptr, "glGetDoublev");
    context->GetError = (PFNGLGETERRORPROC) load(userptr, "glGetError");
    context->GetFloatv = (PFNGLGETFLOATVPROC) load(userptr, "glGetFloatv");
    context->GetIntegerv = (PFNGLGETINTEGERVPROC) load(userptr, "glGetIntegerv");
    context->GetLightfv = (PFNGLGETLIGHTFVPROC) load(userptr, "glGetLightfv");
    context->GetLightiv = (PFNGLGETLIGHTIVPROC) load(userptr, "glGetLightiv");
    context->GetMapdv = (PFNGLGETMAPDVPROC) load(userptr, "glGetMapdv");
    context->GetMapfv = (PFNGLGETMAPFVPROC) load(userptr, "glGetMapfv");
    context->GetMapiv = (PFNGLGETMAPIVPROC) load(userptr, "glGetMapiv");
    context->GetMaterialfv = (PFNGLGETMATERIALFVPROC) load(userptr, "glGetMaterialfv");
    context->GetMaterialiv = (PFNGLGETMATERIALIVPROC) load(userptr, "glGetMaterialiv");
    context->GetPixelMapfv = (PFNGLGETPIXELMAPFVPROC) load(userptr, "glGetPixelMapfv");
    context->GetPixelMapuiv = (PFNGLGETPIXELMAPUIVPROC) load(userptr, "glGetPixelMapuiv");
    context->GetPixelMapusv = (PFNGLGETPIXELMAPUSVPROC) load(userptr, "glGetPixelMapusv");
    context->GetPolygonStipple = (PFNGLGETPOLYGONSTIPPLEPROC) load(userptr, "glGetPolygonStipple");
    context->GetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    context->GetTexEnvfv = (PFNGLGETTEXENVFVPROC) load(userptr, "glGetTexEnvfv");
    context->GetTexEnviv = (PFNGLGETTEXENVIVPROC) load(userptr, "glGetTexEnviv");
    context->GetTexGendv = (PFNGLGETTEXGENDVPROC) load(userptr, "glGetTexGendv");
    context->GetTexGenfv = (PFNGLGETTEXGENFVPROC) load(userptr, "glGetTexGenfv");
    context->GetTexGeniv = (PFNGLGETTEXGENIVPROC) load(userptr, "glGetTexGeniv");
    context->GetTexImage = (PFNGLGETTEXIMAGEPROC) load(userptr, "glGetTexImage");
    context->GetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC) load(userptr, "glGetTexLevelParameterfv");
    context->GetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC) load(userptr, "glGetTexLevelParameteriv");
    context->GetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC) load(userptr, "glGetTexParameterfv");
    context->GetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC) load(userptr, "glGetTexParameteriv");
    context->Hint = (PFNGLHINTPROC) load(userptr, "glHint");
    context->IndexMask = (PFNGLINDEXMASKPROC) load(userptr, "glIndexMask");
    context->Indexd = (PFNGLINDEXDPROC) load(userptr, "glIndexd");
    context->Indexdv = (PFNGLINDEXDVPROC) load(userptr, "glIndexdv");
    context->Indexf = (PFNGLINDEXFPROC) load(userptr, "glIndexf");
    context->Indexfv = (PFNGLINDEXFVPROC) load(userptr, "glIndexfv");
    context->Indexi = (PFNGLINDEXIPROC) load(userptr, "glIndexi");
    context->Indexiv = (PFNGLINDEXIVPROC) load(userptr, "glIndexiv");
    context->Indexs = (PFNGLINDEXSPROC) load(userptr, "glIndexs");
    context->Indexsv = (PFNGLINDEXSVPROC) load(userptr, "glIndexsv");
    context->InitNames = (PFNGLINITNAMESPROC) load(userptr, "glInitNames");
    context->IsEnabled = (PFNGLISENABLEDPROC) load(userptr, "glIsEnabled");
    context->IsList = (PFNGLISLISTPROC) load(userptr, "glIsList");
    context->LightModelf = (PFNGLLIGHTMODELFPROC) load(userptr, "glLightModelf");
    context->LightModelfv = (PFNGLLIGHTMODELFVPROC) load(userptr, "glLightModelfv");
    context->LightModeli = (PFNGLLIGHTMODELIPROC) load(userptr, "glLightModeli");
    context->LightModeliv = (PFNGLLIGHTMODELIVPROC) load(userptr, "glLightModeliv");
    context->Lightf = (PFNGLLIGHTFPROC) load(userptr, "glLightf");
    context->Lightfv = (PFNGLLIGHTFVPROC) load(userptr, "glLightfv");
    context->Lighti = (PFNGLLIGHTIPROC) load(userptr, "glLighti");
    context->Lightiv = (PFNGLLIGHTIVPROC) load(userptr, "glLightiv");
    context->LineStipple = (PFNGLLINESTIPPLEPROC) load(userptr, "glLineStipple");
    context->LineWidth = (PFNGLLINEWIDTHPROC) load(userptr, "glLineWidth");
    context->ListBase = (PFNGLLISTBASEPROC) load(userptr, "glListBase");
    context->LoadIdentity = (PFNGLLOADIDENTITYPROC) load(userptr, "glLoadIdentity");
    context->LoadMatrixd = (PFNGLLOADMATRIXDPROC) load(userptr, "glLoadMatrixd");
    context->LoadMatrixf = (PFNGLLOADMATRIXFPROC) load(userptr, "glLoadMatrixf");
    context->LoadName = (PFNGLLOADNAMEPROC) load(userptr, "glLoadName");
    context->LogicOp = (PFNGLLOGICOPPROC) load(userptr, "glLogicOp");
    context->Map1d = (PFNGLMAP1DPROC) load(userptr, "glMap1d");
    context->Map1f = (PFNGLMAP1FPROC) load(userptr, "glMap1f");
    context->Map2d = (PFNGLMAP2DPROC) load(userptr, "glMap2d");
    context->Map2f = (PFNGLMAP2FPROC) load(userptr, "glMap2f");
    context->MapGrid1d = (PFNGLMAPGRID1DPROC) load(userptr, "glMapGrid1d");
    context->MapGrid1f = (PFNGLMAPGRID1FPROC) load(userptr, "glMapGrid1f");
    context->MapGrid2d = (PFNGLMAPGRID2DPROC) load(userptr, "glMapGrid2d");
    context->MapGrid2f = (PFNGLMAPGRID2FPROC) load(userptr, "glMapGrid2f");
    context->Materialf = (PFNGLMATERIALFPROC) load(userptr, "glMaterialf");
    context->Materialfv = (PFNGLMATERIALFVPROC) load(userptr, "glMaterialfv");
    context->Materiali = (PFNGLMATERIALIPROC) load(userptr, "glMateriali");
    context->Materialiv = (PFNGLMATERIALIVPROC) load(userptr, "glMaterialiv");
    context->MatrixMode = (PFNGLMATRIXMODEPROC) load(userptr, "glMatrixMode");
    context->MultMatrixd = (PFNGLMULTMATRIXDPROC) load(userptr, "glMultMatrixd");
    context->MultMatrixf = (PFNGLMULTMATRIXFPROC) load(userptr, "glMultMatrixf");
    context->NewList = (PFNGLNEWLISTPROC) load(userptr, "glNewList");
    context->Normal3b = (PFNGLNORMAL3BPROC) load(userptr, "glNormal3b");
    context->Normal3bv = (PFNGLNORMAL3BVPROC) load(userptr, "glNormal3bv");
    context->Normal3d = (PFNGLNORMAL3DPROC) load(userptr, "glNormal3d");
    context->Normal3dv = (PFNGLNORMAL3DVPROC) load(userptr, "glNormal3dv");
    context->Normal3f = (PFNGLNORMAL3FPROC) load(userptr, "glNormal3f");
    context->Normal3fv = (PFNGLNORMAL3FVPROC) load(userptr, "glNormal3fv");
    context->Normal3i = (PFNGLNORMAL3IPROC) load(userptr, "glNormal3i");
    context->Normal3iv = (PFNGLNORMAL3IVPROC) load(userptr, "glNormal3iv");
    context->Normal3s = (PFNGLNORMAL3SPROC) load(userptr, "glNormal3s");
    context->Normal3sv = (PFNGLNORMAL3SVPROC) load(userptr, "glNormal3sv");
    context->Ortho = (PFNGLORTHOPROC) load(userptr, "glOrtho");
    context->PassThrough = (PFNGLPASSTHROUGHPROC) load(userptr, "glPassThrough");
    context->PixelMapfv = (PFNGLPIXELMAPFVPROC) load(userptr, "glPixelMapfv");
    context->PixelMapuiv = (PFNGLPIXELMAPUIVPROC) load(userptr, "glPixelMapuiv");
    context->PixelMapusv = (PFNGLPIXELMAPUSVPROC) load(userptr, "glPixelMapusv");
    context->PixelStoref = (PFNGLPIXELSTOREFPROC) load(userptr, "glPixelStoref");
    context->PixelStorei = (PFNGLPIXELSTOREIPROC) load(userptr, "glPixelStorei");
    context->PixelTransferf = (PFNGLPIXELTRANSFERFPROC) load(userptr, "glPixelTransferf");
    context->PixelTransferi = (PFNGLPIXELTRANSFERIPROC) load(userptr, "glPixelTransferi");
    context->PixelZoom = (PFNGLPIXELZOOMPROC) load(userptr, "glPixelZoom");
    context->PointSize = (PFNGLPOINTSIZEPROC) load(userptr, "glPointSize");
    context->PolygonMode = (PFNGLPOLYGONMODEPROC) load(userptr, "glPolygonMode");
    context->PolygonStipple = (PFNGLPOLYGONSTIPPLEPROC) load(userptr, "glPolygonStipple");
    context->PopAttrib = (PFNGLPOPATTRIBPROC) load(userptr, "glPopAttrib");
    context->PopMatrix = (PFNGLPOPMATRIXPROC) load(userptr, "glPopMatrix");
    context->PopName = (PFNGLPOPNAMEPROC) load(userptr, "glPopName");
    context->PushAttrib = (PFNGLPUSHATTRIBPROC) load(userptr, "glPushAttrib");
    context->PushMatrix = (PFNGLPUSHMATRIXPROC) load(userptr, "glPushMatrix");
    context->PushName = (PFNGLPUSHNAMEPROC) load(userptr, "glPushName");
    context->RasterPos2d = (PFNGLRASTERPOS2DPROC) load(userptr, "glRasterPos2d");
    context->RasterPos2dv = (PFNGLRASTERPOS2DVPROC) load(userptr, "glRasterPos2dv");
    context->RasterPos2f = (PFNGLRASTERPOS2FPROC) load(userptr, "glRasterPos2f");
    context->RasterPos2fv = (PFNGLRASTERPOS2FVPROC) load(userptr, "glRasterPos2fv");
    context->RasterPos2i = (PFNGLRASTERPOS2IPROC) load(userptr, "glRasterPos2i");
    context->RasterPos2iv = (PFNGLRASTERPOS2IVPROC) load(userptr, "glRasterPos2iv");
    context->RasterPos2s = (PFNGLRASTERPOS2SPROC) load(userptr, "glRasterPos2s");
    context->RasterPos2sv = (PFNGLRASTERPOS2SVPROC) load(userptr, "glRasterPos2sv");
    context->RasterPos3d = (PFNGLRASTERPOS3DPROC) load(userptr, "glRasterPos3d");
    context->RasterPos3dv = (PFNGLRASTERPOS3DVPROC) load(userptr, "glRasterPos3dv");
    context->RasterPos3f = (PFNGLRASTERPOS3FPROC) load(userptr, "glRasterPos3f");
    context->RasterPos3fv = (PFNGLRASTERPOS3FVPROC) load(userptr, "glRasterPos3fv");
    context->RasterPos3i = (PFNGLRASTERPOS3IPROC) load(userptr, "glRasterPos3i");
    context->RasterPos3iv = (PFNGLRASTERPOS3IVPROC) load(userptr, "glRasterPos3iv");
    context->RasterPos3s = (PFNGLRASTERPOS3SPROC) load(userptr, "glRasterPos3s");
    context->RasterPos3sv = (PFNGLRASTERPOS3SVPROC) load(userptr, "glRasterPos3sv");
    context->RasterPos4d = (PFNGLRASTERPOS4DPROC) load(userptr, "glRasterPos4d");
    context->RasterPos4dv = (PFNGLRASTERPOS4DVPROC) load(userptr, "glRasterPos4dv");
    context->RasterPos4f = (PFNGLRASTERPOS4FPROC) load(userptr, "glRasterPos4f");
    context->RasterPos4fv = (PFNGLRASTERPOS4FVPROC) load(userptr, "glRasterPos4fv");
    context->RasterPos4i = (PFNGLRASTERPOS4IPROC) load(userptr, "glRasterPos4i");
    context->RasterPos4iv = (PFNGLRASTERPOS4IVPROC) load(userptr, "glRasterPos4iv");
    context->RasterPos4s = (PFNGLRASTERPOS4SPROC) load(userptr, "glRasterPos4s");
    context->RasterPos4sv = (PFNGLRASTERPOS4SVPROC) load(userptr, "glRasterPos4sv");
    context->ReadBuffer = (PFNGLREADBUFFERPROC) load(userptr, "glReadBuffer");
    context->ReadPixels = (PFNGLREADPIXELSPROC) load(userptr, "glReadPixels");
    context->Rectd = (PFNGLRECTDPROC) load(userptr, "glRectd");
    context->Rectdv = (PFNGLRECTDVPROC) load(userptr, "glRectdv");
    context->Rectf = (PFNGLRECTFPROC) load(userptr, "glRectf");
    context->Rectfv = (PFNGLRECTFVPROC) load(userptr, "glRectfv");
    context->Recti = (PFNGLRECTIPROC) load(userptr, "glRecti");
    context->Rectiv = (PFNGLRECTIVPROC) load(userptr, "glRectiv");
    context->Rects = (PFNGLRECTSPROC) load(userptr, "glRects");
    context->Rectsv = (PFNGLRECTSVPROC) load(userptr, "glRectsv");
    context->RenderMode = (PFNGLRENDERMODEPROC) load(userptr, "glRenderMode");
    context->Rotated = (PFNGLROTATEDPROC) load(userptr, "glRotated");
    context->Rotatef = (PFNGLROTATEFPROC) load(userptr, "glRotatef");
    context->Scaled = (PFNGLSCALEDPROC) load(userptr, "glScaled");
    context->Scalef = (PFNGLSCALEFPROC) load(userptr, "glScalef");
    context->Scissor = (PFNGLSCISSORPROC) load(userptr, "glScissor");
    context->SelectBuffer = (PFNGLSELECTBUFFERPROC) load(userptr, "glSelectBuffer");
    context->ShadeModel = (PFNGLSHADEMODELPROC) load(userptr, "glShadeModel");
    context->StencilFunc = (PFNGLSTENCILFUNCPROC) load(userptr, "glStencilFunc");
    context->StencilMask = (PFNGLSTENCILMASKPROC) load(userptr, "glStencilMask");
    context->StencilOp = (PFNGLSTENCILOPPROC) load(userptr, "glStencilOp");
    context->TexCoord1d = (PFNGLTEXCOORD1DPROC) load(userptr, "glTexCoord1d");
    context->TexCoord1dv = (PFNGLTEXCOORD1DVPROC) load(userptr, "glTexCoord1dv");
    context->TexCoord1f = (PFNGLTEXCOORD1FPROC) load(userptr, "glTexCoord1f");
    context->TexCoord1fv = (PFNGLTEXCOORD1FVPROC) load(userptr, "glTexCoord1fv");
    context->TexCoord1i = (PFNGLTEXCOORD1IPROC) load(userptr, "glTexCoord1i");
    context->TexCoord1iv = (PFNGLTEXCOORD1IVPROC) load(userptr, "glTexCoord1iv");
    context->TexCoord1s = (PFNGLTEXCOORD1SPROC) load(userptr, "glTexCoord1s");
    context->TexCoord1sv = (PFNGLTEXCOORD1SVPROC) load(userptr, "glTexCoord1sv");
    context->TexCoord2d = (PFNGLTEXCOORD2DPROC) load(userptr, "glTexCoord2d");
    context->TexCoord2dv = (PFNGLTEXCOORD2DVPROC) load(userptr, "glTexCoord2dv");
    context->TexCoord2f = (PFNGLTEXCOORD2FPROC) load(userptr, "glTexCoord2f");
    context->TexCoord2fv = (PFNGLTEXCOORD2FVPROC) load(userptr, "glTexCoord2fv");
    context->TexCoord2i = (PFNGLTEXCOORD2IPROC) load(userptr, "glTexCoord2i");
    context->TexCoord2iv = (PFNGLTEXCOORD2IVPROC) load(userptr, "glTexCoord2iv");
    context->TexCoord2s = (PFNGLTEXCOORD2SPROC) load(userptr, "glTexCoord2s");
    context->TexCoord2sv = (PFNGLTEXCOORD2SVPROC) load(userptr, "glTexCoord2sv");
    context->TexCoord3d = (PFNGLTEXCOORD3DPROC) load(userptr, "glTexCoord3d");
    context->TexCoord3dv = (PFNGLTEXCOORD3DVPROC) load(userptr, "glTexCoord3dv");
    context->TexCoord3f = (PFNGLTEXCOORD3FPROC) load(userptr, "glTexCoord3f");
    context->TexCoord3fv = (PFNGLTEXCOORD3FVPROC) load(userptr, "glTexCoord3fv");
    context->TexCoord3i = (PFNGLTEXCOORD3IPROC) load(userptr, "glTexCoord3i");
    context->TexCoord3iv = (PFNGLTEXCOORD3IVPROC) load(userptr, "glTexCoord3iv");
    context->TexCoord3s = (PFNGLTEXCOORD3SPROC) load(userptr, "glTexCoord3s");
    context->TexCoord3sv = (PFNGLTEXCOORD3SVPROC) load(userptr, "glTexCoord3sv");
    context->TexCoord4d = (PFNGLTEXCOORD4DPROC) load(userptr, "glTexCoord4d");
    context->TexCoord4dv = (PFNGLTEXCOORD4DVPROC) load(userptr, "glTexCoord4dv");
    context->TexCoord4f = (PFNGLTEXCOORD4FPROC) load(userptr, "glTexCoord4f");
    context->TexCoord4fv = (PFNGLTEXCOORD4FVPROC) load(userptr, "glTexCoord4fv");
    context->TexCoord4i = (PFNGLTEXCOORD4IPROC) load(userptr, "glTexCoord4i");
    context->TexCoord4iv = (PFNGLTEXCOORD4IVPROC) load(userptr, "glTexCoord4iv");
    context->TexCoord4s = (PFNGLTEXCOORD4SPROC) load(userptr, "glTexCoord4s");
    context->TexCoord4sv = (PFNGLTEXCOORD4SVPROC) load(userptr, "glTexCoord4sv");
    context->TexEnvf = (PFNGLTEXENVFPROC) load(userptr, "glTexEnvf");
    context->TexEnvfv = (PFNGLTEXENVFVPROC) load(userptr, "glTexEnvfv");
    context->TexEnvi = (PFNGLTEXENVIPROC) load(userptr, "glTexEnvi");
    context->TexEnviv = (PFNGLTEXENVIVPROC) load(userptr, "glTexEnviv");
    context->TexGend = (PFNGLTEXGENDPROC) load(userptr, "glTexGend");
    context->TexGendv = (PFNGLTEXGENDVPROC) load(userptr, "glTexGendv");
    context->TexGenf = (PFNGLTEXGENFPROC) load(userptr, "glTexGenf");
    context->TexGenfv = (PFNGLTEXGENFVPROC) load(userptr, "glTexGenfv");
    context->TexGeni = (PFNGLTEXGENIPROC) load(userptr, "glTexGeni");
    context->TexGeniv = (PFNGLTEXGENIVPROC) load(userptr, "glTexGeniv");
    context->TexImage1D = (PFNGLTEXIMAGE1DPROC) load(userptr, "glTexImage1D");
    context->TexImage2D = (PFNGLTEXIMAGE2DPROC) load(userptr, "glTexImage2D");
    context->TexParameterf = (PFNGLTEXPARAMETERFPROC) load(userptr, "glTexParameterf");
    context->TexParameterfv = (PFNGLTEXPARAMETERFVPROC) load(userptr, "glTexParameterfv");
    context->TexParameteri = (PFNGLTEXPARAMETERIPROC) load(userptr, "glTexParameteri");
    context->TexParameteriv = (PFNGLTEXPARAMETERIVPROC) load(userptr, "glTexParameteriv");
    context->Translated = (PFNGLTRANSLATEDPROC) load(userptr, "glTranslated");
    context->Translatef = (PFNGLTRANSLATEFPROC) load(userptr, "glTranslatef");
    context->Vertex2d = (PFNGLVERTEX2DPROC) load(userptr, "glVertex2d");
    context->Vertex2dv = (PFNGLVERTEX2DVPROC) load(userptr, "glVertex2dv");
    context->Vertex2f = (PFNGLVERTEX2FPROC) load(userptr, "glVertex2f");
    context->Vertex2fv = (PFNGLVERTEX2FVPROC) load(userptr, "glVertex2fv");
    context->Vertex2i = (PFNGLVERTEX2IPROC) load(userptr, "glVertex2i");
    context->Vertex2iv = (PFNGLVERTEX2IVPROC) load(userptr, "glVertex2iv");
    context->Vertex2s = (PFNGLVERTEX2SPROC) load(userptr, "glVertex2s");
    context->Vertex2sv = (PFNGLVERTEX2SVPROC) load(userptr, "glVertex2sv");
    context->Vertex3d = (PFNGLVERTEX3DPROC) load(userptr, "glVertex3d");
    context->Vertex3dv = (PFNGLVERTEX3DVPROC) load(userptr, "glVertex3dv");
    context->Vertex3f = (PFNGLVERTEX3FPROC) load(userptr, "glVertex3f");
    context->Vertex3fv = (PFNGLVERTEX3FVPROC) load(userptr, "glVertex3fv");
    context->Vertex3i = (PFNGLVERTEX3IPROC) load(userptr, "glVertex3i");
    context->Vertex3iv = (PFNGLVERTEX3IVPROC) load(userptr, "glVertex3iv");
    context->Vertex3s = (PFNGLVERTEX3SPROC) load(userptr, "glVertex3s");
    context->Vertex3sv = (PFNGLVERTEX3SVPROC) load(userptr, "glVertex3sv");
    context->Vertex4d = (PFNGLVERTEX4DPROC) load(userptr, "glVertex4d");
    context->Vertex4dv = (PFNGLVERTEX4DVPROC) load(userptr, "glVertex4dv");
    context->Vertex4f = (PFNGLVERTEX4FPROC) load(userptr, "glVertex4f");
    context->Vertex4fv = (PFNGLVERTEX4FVPROC) load(userptr, "glVertex4fv");
    context->Vertex4i = (PFNGLVERTEX4IPROC) load(userptr, "glVertex4i");
    context->Vertex4iv = (PFNGLVERTEX4IVPROC) load(userptr, "glVertex4iv");
    context->Vertex4s = (PFNGLVERTEX4SPROC) load(userptr, "glVertex4s");
    context->Vertex4sv = (PFNGLVERTEX4SVPROC) load(userptr, "glVertex4sv");
    context->Viewport = (PFNGLVIEWPORTPROC) load(userptr, "glViewport");
}
static void glad_gl_load_GL_VERSION_1_1(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_1_1) return;
    context->AreTexturesResident = (PFNGLARETEXTURESRESIDENTPROC) load(userptr, "glAreTexturesResident");
    context->ArrayElement = (PFNGLARRAYELEMENTPROC) load(userptr, "glArrayElement");
    context->BindTexture = (PFNGLBINDTEXTUREPROC) load(userptr, "glBindTexture");
    context->ColorPointer = (PFNGLCOLORPOINTERPROC) load(userptr, "glColorPointer");
    context->CopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC) load(userptr, "glCopyTexImage1D");
    context->CopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC) load(userptr, "glCopyTexImage2D");
    context->CopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC) load(userptr, "glCopyTexSubImage1D");
    context->CopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC) load(userptr, "glCopyTexSubImage2D");
    context->DeleteTextures = (PFNGLDELETETEXTURESPROC) load(userptr, "glDeleteTextures");
    context->DisableClientState = (PFNGLDISABLECLIENTSTATEPROC) load(userptr, "glDisableClientState");
    context->DrawArrays = (PFNGLDRAWARRAYSPROC) load(userptr, "glDrawArrays");
    context->DrawElements = (PFNGLDRAWELEMENTSPROC) load(userptr, "glDrawElements");
    context->EdgeFlagPointer = (PFNGLEDGEFLAGPOINTERPROC) load(userptr, "glEdgeFlagPointer");
    context->EnableClientState = (PFNGLENABLECLIENTSTATEPROC) load(userptr, "glEnableClientState");
    context->GenTextures = (PFNGLGENTEXTURESPROC) load(userptr, "glGenTextures");
    context->GetPointerv = (PFNGLGETPOINTERVPROC) load(userptr, "glGetPointerv");
    context->IndexPointer = (PFNGLINDEXPOINTERPROC) load(userptr, "glIndexPointer");
    context->Indexub = (PFNGLINDEXUBPROC) load(userptr, "glIndexub");
    context->Indexubv = (PFNGLINDEXUBVPROC) load(userptr, "glIndexubv");
    context->InterleavedArrays = (PFNGLINTERLEAVEDARRAYSPROC) load(userptr, "glInterleavedArrays");
    context->IsTexture = (PFNGLISTEXTUREPROC) load(userptr, "glIsTexture");
    context->NormalPointer = (PFNGLNORMALPOINTERPROC) load(userptr, "glNormalPointer");
    context->PolygonOffset = (PFNGLPOLYGONOFFSETPROC) load(userptr, "glPolygonOffset");
    context->PopClientAttrib = (PFNGLPOPCLIENTATTRIBPROC) load(userptr, "glPopClientAttrib");
    context->PrioritizeTextures = (PFNGLPRIORITIZETEXTURESPROC) load(userptr, "glPrioritizeTextures");
    context->PushClientAttrib = (PFNGLPUSHCLIENTATTRIBPROC) load(userptr, "glPushClientAttrib");
    context->TexCoordPointer = (PFNGLTEXCOORDPOINTERPROC) load(userptr, "glTexCoordPointer");
    context->TexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC) load(userptr, "glTexSubImage1D");
    context->TexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC) load(userptr, "glTexSubImage2D");
    context->VertexPointer = (PFNGLVERTEXPOINTERPROC) load(userptr, "glVertexPointer");
}
static void glad_gl_load_GL_VERSION_1_2(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_1_2) return;
    context->CopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC) load(userptr, "glCopyTexSubImage3D");
    context->DrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC) load(userptr, "glDrawRangeElements");
    context->TexImage3D = (PFNGLTEXIMAGE3DPROC) load(userptr, "glTexImage3D");
    context->TexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC) load(userptr, "glTexSubImage3D");
}
static void glad_gl_load_GL_VERSION_1_3(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_1_3) return;
    context->ActiveTexture = (PFNGLACTIVETEXTUREPROC) load(userptr, "glActiveTexture");
    context->ClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC) load(userptr, "glClientActiveTexture");
    context->CompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC) load(userptr, "glCompressedTexImage1D");
    context->CompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) load(userptr, "glCompressedTexImage2D");
    context->CompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC) load(userptr, "glCompressedTexImage3D");
    context->CompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC) load(userptr, "glCompressedTexSubImage1D");
    context->CompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) load(userptr, "glCompressedTexSubImage2D");
    context->CompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC) load(userptr, "glCompressedTexSubImage3D");
    context->GetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC) load(userptr, "glGetCompressedTexImage");
    context->LoadTransposeMatrixd = (PFNGLLOADTRANSPOSEMATRIXDPROC) load(userptr, "glLoadTransposeMatrixd");
    context->LoadTransposeMatrixf = (PFNGLLOADTRANSPOSEMATRIXFPROC) load(userptr, "glLoadTransposeMatrixf");
    context->MultTransposeMatrixd = (PFNGLMULTTRANSPOSEMATRIXDPROC) load(userptr, "glMultTransposeMatrixd");
    context->MultTransposeMatrixf = (PFNGLMULTTRANSPOSEMATRIXFPROC) load(userptr, "glMultTransposeMatrixf");
    context->MultiTexCoord1d = (PFNGLMULTITEXCOORD1DPROC) load(userptr, "glMultiTexCoord1d");
    context->MultiTexCoord1dv = (PFNGLMULTITEXCOORD1DVPROC) load(userptr, "glMultiTexCoord1dv");
    context->MultiTexCoord1f = (PFNGLMULTITEXCOORD1FPROC) load(userptr, "glMultiTexCoord1f");
    context->MultiTexCoord1fv = (PFNGLMULTITEXCOORD1FVPROC) load(userptr, "glMultiTexCoord1fv");
    context->MultiTexCoord1i = (PFNGLMULTITEXCOORD1IPROC) load(userptr, "glMultiTexCoord1i");
    context->MultiTexCoord1iv = (PFNGLMULTITEXCOORD1IVPROC) load(userptr, "glMultiTexCoord1iv");
    context->MultiTexCoord1s = (PFNGLMULTITEXCOORD1SPROC) load(userptr, "glMultiTexCoord1s");
    context->MultiTexCoord1sv = (PFNGLMULTITEXCOORD1SVPROC) load(userptr, "glMultiTexCoord1sv");
    context->MultiTexCoord2d = (PFNGLMULTITEXCOORD2DPROC) load(userptr, "glMultiTexCoord2d");
    context->MultiTexCoord2dv = (PFNGLMULTITEXCOORD2DVPROC) load(userptr, "glMultiTexCoord2dv");
    context->MultiTexCoord2f = (PFNGLMULTITEXCOORD2FPROC) load(userptr, "glMultiTexCoord2f");
    context->MultiTexCoord2fv = (PFNGLMULTITEXCOORD2FVPROC) load(userptr, "glMultiTexCoord2fv");
    context->MultiTexCoord2i = (PFNGLMULTITEXCOORD2IPROC) load(userptr, "glMultiTexCoord2i");
    context->MultiTexCoord2iv = (PFNGLMULTITEXCOORD2IVPROC) load(userptr, "glMultiTexCoord2iv");
    context->MultiTexCoord2s = (PFNGLMULTITEXCOORD2SPROC) load(userptr, "glMultiTexCoord2s");
    context->MultiTexCoord2sv = (PFNGLMULTITEXCOORD2SVPROC) load(userptr, "glMultiTexCoord2sv");
    context->MultiTexCoord3d = (PFNGLMULTITEXCOORD3DPROC) load(userptr, "glMultiTexCoord3d");
    context->MultiTexCoord3dv = (PFNGLMULTITEXCOORD3DVPROC) load(userptr, "glMultiTexCoord3dv");
    context->MultiTexCoord3f = (PFNGLMULTITEXCOORD3FPROC) load(userptr, "glMultiTexCoord3f");
    context->MultiTexCoord3fv = (PFNGLMULTITEXCOORD3FVPROC) load(userptr, "glMultiTexCoord3fv");
    context->MultiTexCoord3i = (PFNGLMULTITEXCOORD3IPROC) load(userptr, "glMultiTexCoord3i");
    context->MultiTexCoord3iv = (PFNGLMULTITEXCOORD3IVPROC) load(userptr, "glMultiTexCoord3iv");
    context->MultiTexCoord3s = (PFNGLMULTITEXCOORD3SPROC) load(userptr, "glMultiTexCoord3s");
    context->MultiTexCoord3sv = (PFNGLMULTITEXCOORD3SVPROC) load(userptr, "glMultiTexCoord3sv");
    context->MultiTexCoord4d = (PFNGLMULTITEXCOORD4DPROC) load(userptr, "glMultiTexCoord4d");
    context->MultiTexCoord4dv = (PFNGLMULTITEXCOORD4DVPROC) load(userptr, "glMultiTexCoord4dv");
    context->MultiTexCoord4f = (PFNGLMULTITEXCOORD4FPROC) load(userptr, "glMultiTexCoord4f");
    context->MultiTexCoord4fv = (PFNGLMULTITEXCOORD4FVPROC) load(userptr, "glMultiTexCoord4fv");
    context->MultiTexCoord4i = (PFNGLMULTITEXCOORD4IPROC) load(userptr, "glMultiTexCoord4i");
    context->MultiTexCoord4iv = (PFNGLMULTITEXCOORD4IVPROC) load(userptr, "glMultiTexCoord4iv");
    context->MultiTexCoord4s = (PFNGLMULTITEXCOORD4SPROC) load(userptr, "glMultiTexCoord4s");
    context->MultiTexCoord4sv = (PFNGLMULTITEXCOORD4SVPROC) load(userptr, "glMultiTexCoord4sv");
    context->SampleCoverage = (PFNGLSAMPLECOVERAGEPROC) load(userptr, "glSampleCoverage");
}
static void glad_gl_load_GL_VERSION_1_4(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_1_4) return;
    context->BlendColor = (PFNGLBLENDCOLORPROC) load(userptr, "glBlendColor");
    context->BlendEquation = (PFNGLBLENDEQUATIONPROC) load(userptr, "glBlendEquation");
    context->BlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC) load(userptr, "glBlendFuncSeparate");
    context->FogCoordPointer = (PFNGLFOGCOORDPOINTERPROC) load(userptr, "glFogCoordPointer");
    context->FogCoordd = (PFNGLFOGCOORDDPROC) load(userptr, "glFogCoordd");
    context->FogCoorddv = (PFNGLFOGCOORDDVPROC) load(userptr, "glFogCoorddv");
    context->FogCoordf = (PFNGLFOGCOORDFPROC) load(userptr, "glFogCoordf");
    context->FogCoordfv = (PFNGLFOGCOORDFVPROC) load(userptr, "glFogCoordfv");
    context->MultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC) load(userptr, "glMultiDrawArrays");
    context->MultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC) load(userptr, "glMultiDrawElements");
    context->PointParameterf = (PFNGLPOINTPARAMETERFPROC) load(userptr, "glPointParameterf");
    context->PointParameterfv = (PFNGLPOINTPARAMETERFVPROC) load(userptr, "glPointParameterfv");
    context->PointParameteri = (PFNGLPOINTPARAMETERIPROC) load(userptr, "glPointParameteri");
    context->PointParameteriv = (PFNGLPOINTPARAMETERIVPROC) load(userptr, "glPointParameteriv");
    context->SecondaryColor3b = (PFNGLSECONDARYCOLOR3BPROC) load(userptr, "glSecondaryColor3b");
    context->SecondaryColor3bv = (PFNGLSECONDARYCOLOR3BVPROC) load(userptr, "glSecondaryColor3bv");
    context->SecondaryColor3d = (PFNGLSECONDARYCOLOR3DPROC) load(userptr, "glSecondaryColor3d");
    context->SecondaryColor3dv = (PFNGLSECONDARYCOLOR3DVPROC) load(userptr, "glSecondaryColor3dv");
    context->SecondaryColor3f = (PFNGLSECONDARYCOLOR3FPROC) load(userptr, "glSecondaryColor3f");
    context->SecondaryColor3fv = (PFNGLSECONDARYCOLOR3FVPROC) load(userptr, "glSecondaryColor3fv");
    context->SecondaryColor3i = (PFNGLSECONDARYCOLOR3IPROC) load(userptr, "glSecondaryColor3i");
    context->SecondaryColor3iv = (PFNGLSECONDARYCOLOR3IVPROC) load(userptr, "glSecondaryColor3iv");
    context->SecondaryColor3s = (PFNGLSECONDARYCOLOR3SPROC) load(userptr, "glSecondaryColor3s");
    context->SecondaryColor3sv = (PFNGLSECONDARYCOLOR3SVPROC) load(userptr, "glSecondaryColor3sv");
    context->SecondaryColor3ub = (PFNGLSECONDARYCOLOR3UBPROC) load(userptr, "glSecondaryColor3ub");
    context->SecondaryColor3ubv = (PFNGLSECONDARYCOLOR3UBVPROC) load(userptr, "glSecondaryColor3ubv");
    context->SecondaryColor3ui = (PFNGLSECONDARYCOLOR3UIPROC) load(userptr, "glSecondaryColor3ui");
    context->SecondaryColor3uiv = (PFNGLSECONDARYCOLOR3UIVPROC) load(userptr, "glSecondaryColor3uiv");
    context->SecondaryColor3us = (PFNGLSECONDARYCOLOR3USPROC) load(userptr, "glSecondaryColor3us");
    context->SecondaryColor3usv = (PFNGLSECONDARYCOLOR3USVPROC) load(userptr, "glSecondaryColor3usv");
    context->SecondaryColorPointer = (PFNGLSECONDARYCOLORPOINTERPROC) load(userptr, "glSecondaryColorPointer");
    context->WindowPos2d = (PFNGLWINDOWPOS2DPROC) load(userptr, "glWindowPos2d");
    context->WindowPos2dv = (PFNGLWINDOWPOS2DVPROC) load(userptr, "glWindowPos2dv");
    context->WindowPos2f = (PFNGLWINDOWPOS2FPROC) load(userptr, "glWindowPos2f");
    context->WindowPos2fv = (PFNGLWINDOWPOS2FVPROC) load(userptr, "glWindowPos2fv");
    context->WindowPos2i = (PFNGLWINDOWPOS2IPROC) load(userptr, "glWindowPos2i");
    context->WindowPos2iv = (PFNGLWINDOWPOS2IVPROC) load(userptr, "glWindowPos2iv");
    context->WindowPos2s = (PFNGLWINDOWPOS2SPROC) load(userptr, "glWindowPos2s");
    context->WindowPos2sv = (PFNGLWINDOWPOS2SVPROC) load(userptr, "glWindowPos2sv");
    context->WindowPos3d = (PFNGLWINDOWPOS3DPROC) load(userptr, "glWindowPos3d");
    context->WindowPos3dv = (PFNGLWINDOWPOS3DVPROC) load(userptr, "glWindowPos3dv");
    context->WindowPos3f = (PFNGLWINDOWPOS3FPROC) load(userptr, "glWindowPos3f");
    context->WindowPos3fv = (PFNGLWINDOWPOS3FVPROC) load(userptr, "glWindowPos3fv");
    context->WindowPos3i = (PFNGLWINDOWPOS3IPROC) load(userptr, "glWindowPos3i");
    context->WindowPos3iv = (PFNGLWINDOWPOS3IVPROC) load(userptr, "glWindowPos3iv");
    context->WindowPos3s = (PFNGLWINDOWPOS3SPROC) load(userptr, "glWindowPos3s");
    context->WindowPos3sv = (PFNGLWINDOWPOS3SVPROC) load(userptr, "glWindowPos3sv");
}
static void glad_gl_load_GL_VERSION_1_5(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_1_5) return;
    context->BeginQuery = (PFNGLBEGINQUERYPROC) load(userptr, "glBeginQuery");
    context->BindBuffer = (PFNGLBINDBUFFERPROC) load(userptr, "glBindBuffer");
    context->BufferData = (PFNGLBUFFERDATAPROC) load(userptr, "glBufferData");
    context->BufferSubData = (PFNGLBUFFERSUBDATAPROC) load(userptr, "glBufferSubData");
    context->DeleteBuffers = (PFNGLDELETEBUFFERSPROC) load(userptr, "glDeleteBuffers");
    context->DeleteQueries = (PFNGLDELETEQUERIESPROC) load(userptr, "glDeleteQueries");
    context->EndQuery = (PFNGLENDQUERYPROC) load(userptr, "glEndQuery");
    context->GenBuffers = (PFNGLGENBUFFERSPROC) load(userptr, "glGenBuffers");
    context->GenQueries = (PFNGLGENQUERIESPROC) load(userptr, "glGenQueries");
    context->GetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC) load(userptr, "glGetBufferParameteriv");
    context->GetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC) load(userptr, "glGetBufferPointerv");
    context->GetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC) load(userptr, "glGetBufferSubData");
    context->GetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC) load(userptr, "glGetQueryObjectiv");
    context->GetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC) load(userptr, "glGetQueryObjectuiv");
    context->GetQueryiv = (PFNGLGETQUERYIVPROC) load(userptr, "glGetQueryiv");
    context->IsBuffer = (PFNGLISBUFFERPROC) load(userptr, "glIsBuffer");
    context->IsQuery = (PFNGLISQUERYPROC) load(userptr, "glIsQuery");
    context->MapBuffer = (PFNGLMAPBUFFERPROC) load(userptr, "glMapBuffer");
    context->UnmapBuffer = (PFNGLUNMAPBUFFERPROC) load(userptr, "glUnmapBuffer");
}
static void glad_gl_load_GL_VERSION_2_0(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_2_0) return;
    context->AttachShader = (PFNGLATTACHSHADERPROC) load(userptr, "glAttachShader");
    context->BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC) load(userptr, "glBindAttribLocation");
    context->BlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC) load(userptr, "glBlendEquationSeparate");
    context->CompileShader = (PFNGLCOMPILESHADERPROC) load(userptr, "glCompileShader");
    context->CreateProgram = (PFNGLCREATEPROGRAMPROC) load(userptr, "glCreateProgram");
    context->CreateShader = (PFNGLCREATESHADERPROC) load(userptr, "glCreateShader");
    context->DeleteProgram = (PFNGLDELETEPROGRAMPROC) load(userptr, "glDeleteProgram");
    context->DeleteShader = (PFNGLDELETESHADERPROC) load(userptr, "glDeleteShader");
    context->DetachShader = (PFNGLDETACHSHADERPROC) load(userptr, "glDetachShader");
    context->DisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) load(userptr, "glDisableVertexAttribArray");
    context->DrawBuffers = (PFNGLDRAWBUFFERSPROC) load(userptr, "glDrawBuffers");
    context->EnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) load(userptr, "glEnableVertexAttribArray");
    context->GetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC) load(userptr, "glGetActiveAttrib");
    context->GetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC) load(userptr, "glGetActiveUniform");
    context->GetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC) load(userptr, "glGetAttachedShaders");
    context->GetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC) load(userptr, "glGetAttribLocation");
    context->GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) load(userptr, "glGetProgramInfoLog");
    context->GetProgramiv = (PFNGLGETPROGRAMIVPROC) load(userptr, "glGetProgramiv");
    context->GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) load(userptr, "glGetShaderInfoLog");
    context->GetShaderSource = (PFNGLGETSHADERSOURCEPROC) load(userptr, "glGetShaderSource");
    context->GetShaderiv = (PFNGLGETSHADERIVPROC) load(userptr, "glGetShaderiv");
    context->GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) load(userptr, "glGetUniformLocation");
    context->GetUniformfv = (PFNGLGETUNIFORMFVPROC) load(userptr, "glGetUniformfv");
    context->GetUniformiv = (PFNGLGETUNIFORMIVPROC) load(userptr, "glGetUniformiv");
    context->GetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC) load(userptr, "glGetVertexAttribPointerv");
    context->GetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC) load(userptr, "glGetVertexAttribdv");
    context->GetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC) load(userptr, "glGetVertexAttribfv");
    context->GetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC) load(userptr, "glGetVertexAttribiv");
    context->IsProgram = (PFNGLISPROGRAMPROC) load(userptr, "glIsProgram");
    context->IsShader = (PFNGLISSHADERPROC) load(userptr, "glIsShader");
    context->LinkProgram = (PFNGLLINKPROGRAMPROC) load(userptr, "glLinkProgram");
    context->ShaderSource = (PFNGLSHADERSOURCEPROC) load(userptr, "glShaderSource");
    context->StencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC) load(userptr, "glStencilFuncSeparate");
    context->StencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC) load(userptr, "glStencilMaskSeparate");
    context->StencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC) load(userptr, "glStencilOpSeparate");
    context->Uniform1f = (PFNGLUNIFORM1FPROC) load(userptr, "glUniform1f");
    context->Uniform1fv = (PFNGLUNIFORM1FVPROC) load(userptr, "glUniform1fv");
    context->Uniform1i = (PFNGLUNIFORM1IPROC) load(userptr, "glUniform1i");
    context->Uniform1iv = (PFNGLUNIFORM1IVPROC) load(userptr, "glUniform1iv");
    context->Uniform2f = (PFNGLUNIFORM2FPROC) load(userptr, "glUniform2f");
    context->Uniform2fv = (PFNGLUNIFORM2FVPROC) load(userptr, "glUniform2fv");
    context->Uniform2i = (PFNGLUNIFORM2IPROC) load(userptr, "glUniform2i");
    context->Uniform2iv = (PFNGLUNIFORM2IVPROC) load(userptr, "glUniform2iv");
    context->Uniform3f = (PFNGLUNIFORM3FPROC) load(userptr, "glUniform3f");
    context->Uniform3fv = (PFNGLUNIFORM3FVPROC) load(userptr, "glUniform3fv");
    context->Uniform3i = (PFNGLUNIFORM3IPROC) load(userptr, "glUniform3i");
    context->Uniform3iv = (PFNGLUNIFORM3IVPROC) load(userptr, "glUniform3iv");
    context->Uniform4f = (PFNGLUNIFORM4FPROC) load(userptr, "glUniform4f");
    context->Uniform4fv = (PFNGLUNIFORM4FVPROC) load(userptr, "glUniform4fv");
    context->Uniform4i = (PFNGLUNIFORM4IPROC) load(userptr, "glUniform4i");
    context->Uniform4iv = (PFNGLUNIFORM4IVPROC) load(userptr, "glUniform4iv");
    context->UniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC) load(userptr, "glUniformMatrix2fv");
    context->UniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC) load(userptr, "glUniformMatrix3fv");
    context->UniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC) load(userptr, "glUniformMatrix4fv");
    context->UseProgram = (PFNGLUSEPROGRAMPROC) load(userptr, "glUseProgram");
    context->ValidateProgram = (PFNGLVALIDATEPROGRAMPROC) load(userptr, "glValidateProgram");
    context->VertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC) load(userptr, "glVertexAttrib1d");
    context->VertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC) load(userptr, "glVertexAttrib1dv");
    context->VertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC) load(userptr, "glVertexAttrib1f");
    context->VertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC) load(userptr, "glVertexAttrib1fv");
    context->VertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC) load(userptr, "glVertexAttrib1s");
    context->VertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC) load(userptr, "glVertexAttrib1sv");
    context->VertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC) load(userptr, "glVertexAttrib2d");
    context->VertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC) load(userptr, "glVertexAttrib2dv");
    context->VertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC) load(userptr, "glVertexAttrib2f");
    context->VertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC) load(userptr, "glVertexAttrib2fv");
    context->VertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC) load(userptr, "glVertexAttrib2s");
    context->VertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC) load(userptr, "glVertexAttrib2sv");
    context->VertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC) load(userptr, "glVertexAttrib3d");
    context->VertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC) load(userptr, "glVertexAttrib3dv");
    context->VertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC) load(userptr, "glVertexAttrib3f");
    context->VertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC) load(userptr, "glVertexAttrib3fv");
    context->VertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC) load(userptr, "glVertexAttrib3s");
    context->VertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC) load(userptr, "glVertexAttrib3sv");
    context->VertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC) load(userptr, "glVertexAttrib4Nbv");
    context->VertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC) load(userptr, "glVertexAttrib4Niv");
    context->VertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC) load(userptr, "glVertexAttrib4Nsv");
    context->VertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC) load(userptr, "glVertexAttrib4Nub");
    context->VertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC) load(userptr, "glVertexAttrib4Nubv");
    context->VertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC) load(userptr, "glVertexAttrib4Nuiv");
    context->VertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC) load(userptr, "glVertexAttrib4Nusv");
    context->VertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC) load(userptr, "glVertexAttrib4bv");
    context->VertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC) load(userptr, "glVertexAttrib4d");
    context->VertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC) load(userptr, "glVertexAttrib4dv");
    context->VertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC) load(userptr, "glVertexAttrib4f");
    context->VertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC) load(userptr, "glVertexAttrib4fv");
    context->VertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC) load(userptr, "glVertexAttrib4iv");
    context->VertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC) load(userptr, "glVertexAttrib4s");
    context->VertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC) load(userptr, "glVertexAttrib4sv");
    context->VertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC) load(userptr, "glVertexAttrib4ubv");
    context->VertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC) load(userptr, "glVertexAttrib4uiv");
    context->VertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC) load(userptr, "glVertexAttrib4usv");
    context->VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) load(userptr, "glVertexAttribPointer");
}
static void glad_gl_load_GL_ARB_draw_buffers(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_draw_buffers) return;
    context->DrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC) load(userptr, "glDrawBuffersARB");
}
static void glad_gl_load_GL_ARB_framebuffer_object(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_framebuffer_object) return;
    context->BindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) load(userptr, "glBindFramebuffer");
    context->BindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) load(userptr, "glBindRenderbuffer");
    context->BlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC) load(userptr, "glBlitFramebuffer");
    context->CheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) load(userptr, "glCheckFramebufferStatus");
    context->DeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) load(userptr, "glDeleteFramebuffers");
    context->DeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) load(userptr, "glDeleteRenderbuffers");
    context->FramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) load(userptr, "glFramebufferRenderbuffer");
    context->FramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC) load(userptr, "glFramebufferTexture1D");
    context->FramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) load(userptr, "glFramebufferTexture2D");
    context->FramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC) load(userptr, "glFramebufferTexture3D");
    context->FramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC) load(userptr, "glFramebufferTextureLayer");
    context->GenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) load(userptr, "glGenFramebuffers");
    context->GenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) load(userptr, "glGenRenderbuffers");
    context->GenerateMipmap = (PFNGLGENERATEMIPMAPPROC) load(userptr, "glGenerateMipmap");
    context->GetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) load(userptr, "glGetFramebufferAttachmentParameteriv");
    context->GetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) load(userptr, "glGetRenderbufferParameteriv");
    context->IsFramebuffer = (PFNGLISFRAMEBUFFERPROC) load(userptr, "glIsFramebuffer");
    context->IsRenderbuffer = (PFNGLISRENDERBUFFERPROC) load(userptr, "glIsRenderbuffer");
    context->RenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) load(userptr, "glRenderbufferStorage");
    context->RenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) load(userptr, "glRenderbufferStorageMultisample");
}
static void glad_gl_load_GL_ARB_geometry_shader4(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_geometry_shader4) return;
    context->FramebufferTextureARB = (PFNGLFRAMEBUFFERTEXTUREARBPROC) load(userptr, "glFramebufferTextureARB");
    context->FramebufferTextureFaceARB = (PFNGLFRAMEBUFFERTEXTUREFACEARBPROC) load(userptr, "glFramebufferTextureFaceARB");
    context->FramebufferTextureLayerARB = (PFNGLFRAMEBUFFERTEXTURELAYERARBPROC) load(userptr, "glFramebufferTextureLayerARB");
    context->ProgramParameteriARB = (PFNGLPROGRAMPARAMETERIARBPROC) load(userptr, "glProgramParameteriARB");
}
static void glad_gl_load_GL_ARB_imaging(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_imaging) return;
    context->BlendColor = (PFNGLBLENDCOLORPROC) load(userptr, "glBlendColor");
    context->BlendEquation = (PFNGLBLENDEQUATIONPROC) load(userptr, "glBlendEquation");
}
static void glad_gl_load_GL_ARB_multisample(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_multisample) return;
    context->SampleCoverageARB = (PFNGLSAMPLECOVERAGEARBPROC) load(userptr, "glSampleCoverageARB");
}
static void glad_gl_load_GL_ARB_multitexture(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_multitexture) return;
    context->ActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC) load(userptr, "glActiveTextureARB");
    context->ClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC) load(userptr, "glClientActiveTextureARB");
    context->MultiTexCoord1dARB = (PFNGLMULTITEXCOORD1DARBPROC) load(userptr, "glMultiTexCoord1dARB");
    context->MultiTexCoord1dvARB = (PFNGLMULTITEXCOORD1DVARBPROC) load(userptr, "glMultiTexCoord1dvARB");
    context->MultiTexCoord1fARB = (PFNGLMULTITEXCOORD1FARBPROC) load(userptr, "glMultiTexCoord1fARB");
    context->MultiTexCoord1fvARB = (PFNGLMULTITEXCOORD1FVARBPROC) load(userptr, "glMultiTexCoord1fvARB");
    context->MultiTexCoord1iARB = (PFNGLMULTITEXCOORD1IARBPROC) load(userptr, "glMultiTexCoord1iARB");
    context->MultiTexCoord1ivARB = (PFNGLMULTITEXCOORD1IVARBPROC) load(userptr, "glMultiTexCoord1ivARB");
    context->MultiTexCoord1sARB = (PFNGLMULTITEXCOORD1SARBPROC) load(userptr, "glMultiTexCoord1sARB");
    context->MultiTexCoord1svARB = (PFNGLMULTITEXCOORD1SVARBPROC) load(userptr, "glMultiTexCoord1svARB");
    context->MultiTexCoord2dARB = (PFNGLMULTITEXCOORD2DARBPROC) load(userptr, "glMultiTexCoord2dARB");
    context->MultiTexCoord2dvARB = (PFNGLMULTITEXCOORD2DVARBPROC) load(userptr, "glMultiTexCoord2dvARB");
    context->MultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC) load(userptr, "glMultiTexCoord2fARB");
    context->MultiTexCoord2fvARB = (PFNGLMULTITEXCOORD2FVARBPROC) load(userptr, "glMultiTexCoord2fvARB");
    context->MultiTexCoord2iARB = (PFNGLMULTITEXCOORD2IARBPROC) load(userptr, "glMultiTexCoord2iARB");
    context->MultiTexCoord2ivARB = (PFNGLMULTITEXCOORD2IVARBPROC) load(userptr, "glMultiTexCoord2ivARB");
    context->MultiTexCoord2sARB = (PFNGLMULTITEXCOORD2SARBPROC) load(userptr, "glMultiTexCoord2sARB");
    context->MultiTexCoord2svARB = (PFNGLMULTITEXCOORD2SVARBPROC) load(userptr, "glMultiTexCoord2svARB");
    context->MultiTexCoord3dARB = (PFNGLMULTITEXCOORD3DARBPROC) load(userptr, "glMultiTexCoord3dARB");
    context->MultiTexCoord3dvARB = (PFNGLMULTITEXCOORD3DVARBPROC) load(userptr, "glMultiTexCoord3dvARB");
    context->MultiTexCoord3fARB = (PFNGLMULTITEXCOORD3FARBPROC) load(userptr, "glMultiTexCoord3fARB");
    context->MultiTexCoord3fvARB = (PFNGLMULTITEXCOORD3FVARBPROC) load(userptr, "glMultiTexCoord3fvARB");
    context->MultiTexCoord3iARB = (PFNGLMULTITEXCOORD3IARBPROC) load(userptr, "glMultiTexCoord3iARB");
    context->MultiTexCoord3ivARB = (PFNGLMULTITEXCOORD3IVARBPROC) load(userptr, "glMultiTexCoord3ivARB");
    context->MultiTexCoord3sARB = (PFNGLMULTITEXCOORD3SARBPROC) load(userptr, "glMultiTexCoord3sARB");
    context->MultiTexCoord3svARB = (PFNGLMULTITEXCOORD3SVARBPROC) load(userptr, "glMultiTexCoord3svARB");
    context->MultiTexCoord4dARB = (PFNGLMULTITEXCOORD4DARBPROC) load(userptr, "glMultiTexCoord4dARB");
    context->MultiTexCoord4dvARB = (PFNGLMULTITEXCOORD4DVARBPROC) load(userptr, "glMultiTexCoord4dvARB");
    context->MultiTexCoord4fARB = (PFNGLMULTITEXCOORD4FARBPROC) load(userptr, "glMultiTexCoord4fARB");
    context->MultiTexCoord4fvARB = (PFNGLMULTITEXCOORD4FVARBPROC) load(userptr, "glMultiTexCoord4fvARB");
    context->MultiTexCoord4iARB = (PFNGLMULTITEXCOORD4IARBPROC) load(userptr, "glMultiTexCoord4iARB");
    context->MultiTexCoord4ivARB = (PFNGLMULTITEXCOORD4IVARBPROC) load(userptr, "glMultiTexCoord4ivARB");
    context->MultiTexCoord4sARB = (PFNGLMULTITEXCOORD4SARBPROC) load(userptr, "glMultiTexCoord4sARB");
    context->MultiTexCoord4svARB = (PFNGLMULTITEXCOORD4SVARBPROC) load(userptr, "glMultiTexCoord4svARB");
}
static void glad_gl_load_GL_ARB_occlusion_query(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_occlusion_query) return;
    context->BeginQueryARB = (PFNGLBEGINQUERYARBPROC) load(userptr, "glBeginQueryARB");
    context->DeleteQueriesARB = (PFNGLDELETEQUERIESARBPROC) load(userptr, "glDeleteQueriesARB");
    context->EndQueryARB = (PFNGLENDQUERYARBPROC) load(userptr, "glEndQueryARB");
    context->GenQueriesARB = (PFNGLGENQUERIESARBPROC) load(userptr, "glGenQueriesARB");
    context->GetQueryObjectivARB = (PFNGLGETQUERYOBJECTIVARBPROC) load(userptr, "glGetQueryObjectivARB");
    context->GetQueryObjectuivARB = (PFNGLGETQUERYOBJECTUIVARBPROC) load(userptr, "glGetQueryObjectuivARB");
    context->GetQueryivARB = (PFNGLGETQUERYIVARBPROC) load(userptr, "glGetQueryivARB");
    context->IsQueryARB = (PFNGLISQUERYARBPROC) load(userptr, "glIsQueryARB");
}
static void glad_gl_load_GL_ARB_point_parameters(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_point_parameters) return;
    context->PointParameterfARB = (PFNGLPOINTPARAMETERFARBPROC) load(userptr, "glPointParameterfARB");
    context->PointParameterfvARB = (PFNGLPOINTPARAMETERFVARBPROC) load(userptr, "glPointParameterfvARB");
}
static void glad_gl_load_GL_ARB_shader_objects(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_shader_objects) return;
    context->AttachObjectARB = (PFNGLATTACHOBJECTARBPROC) load(userptr, "glAttachObjectARB");
    context->CompileShaderARB = (PFNGLCOMPILESHADERARBPROC) load(userptr, "glCompileShaderARB");
    context->CreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) load(userptr, "glCreateProgramObjectARB");
    context->CreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) load(userptr, "glCreateShaderObjectARB");
    context->DeleteObjectARB = (PFNGLDELETEOBJECTARBPROC) load(userptr, "glDeleteObjectARB");
    context->DetachObjectARB = (PFNGLDETACHOBJECTARBPROC) load(userptr, "glDetachObjectARB");
    context->GetActiveUniformARB = (PFNGLGETACTIVEUNIFORMARBPROC) load(userptr, "glGetActiveUniformARB");
    context->GetAttachedObjectsARB = (PFNGLGETATTACHEDOBJECTSARBPROC) load(userptr, "glGetAttachedObjectsARB");
    context->GetHandleARB = (PFNGLGETHANDLEARBPROC) load(userptr, "glGetHandleARB");
    context->GetInfoLogARB = (PFNGLGETINFOLOGARBPROC) load(userptr, "glGetInfoLogARB");
    context->GetObjectParameterfvARB = (PFNGLGETOBJECTPARAMETERFVARBPROC) load(userptr, "glGetObjectParameterfvARB");
    context->GetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) load(userptr, "glGetObjectParameterivARB");
    context->GetShaderSourceARB = (PFNGLGETSHADERSOURCEARBPROC) load(userptr, "glGetShaderSourceARB");
    context->GetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC) load(userptr, "glGetUniformLocationARB");
    context->GetUniformfvARB = (PFNGLGETUNIFORMFVARBPROC) load(userptr, "glGetUniformfvARB");
    context->GetUniformivARB = (PFNGLGETUNIFORMIVARBPROC) load(userptr, "glGetUniformivARB");
    context->LinkProgramARB = (PFNGLLINKPROGRAMARBPROC) load(userptr, "glLinkProgramARB");
    context->ShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) load(userptr, "glShaderSourceARB");
    context->Uniform1fARB = (PFNGLUNIFORM1FARBPROC) load(userptr, "glUniform1fARB");
    context->Uniform1fvARB = (PFNGLUNIFORM1FVARBPROC) load(userptr, "glUniform1fvARB");
    context->Uniform1iARB = (PFNGLUNIFORM1IARBPROC) load(userptr, "glUniform1iARB");
    context->Uniform1ivARB = (PFNGLUNIFORM1IVARBPROC) load(userptr, "glUniform1ivARB");
    context->Uniform2fARB = (PFNGLUNIFORM2FARBPROC) load(userptr, "glUniform2fARB");
    context->Uniform2fvARB = (PFNGLUNIFORM2FVARBPROC) load(userptr, "glUniform2fvARB");
    context->Uniform2iARB = (PFNGLUNIFORM2IARBPROC) load(userptr, "glUniform2iARB");
    context->Uniform2ivARB = (PFNGLUNIFORM2IVARBPROC) load(userptr, "glUniform2ivARB");
    context->Uniform3fARB = (PFNGLUNIFORM3FARBPROC) load(userptr, "glUniform3fARB");
    context->Uniform3fvARB = (PFNGLUNIFORM3FVARBPROC) load(userptr, "glUniform3fvARB");
    context->Uniform3iARB = (PFNGLUNIFORM3IARBPROC) load(userptr, "glUniform3iARB");
    context->Uniform3ivARB = (PFNGLUNIFORM3IVARBPROC) load(userptr, "glUniform3ivARB");
    context->Uniform4fARB = (PFNGLUNIFORM4FARBPROC) load(userptr, "glUniform4fARB");
    context->Uniform4fvARB = (PFNGLUNIFORM4FVARBPROC) load(userptr, "glUniform4fvARB");
    context->Uniform4iARB = (PFNGLUNIFORM4IARBPROC) load(userptr, "glUniform4iARB");
    context->Uniform4ivARB = (PFNGLUNIFORM4IVARBPROC) load(userptr, "glUniform4ivARB");
    context->UniformMatrix2fvARB = (PFNGLUNIFORMMATRIX2FVARBPROC) load(userptr, "glUniformMatrix2fvARB");
    context->UniformMatrix3fvARB = (PFNGLUNIFORMMATRIX3FVARBPROC) load(userptr, "glUniformMatrix3fvARB");
    context->UniformMatrix4fvARB = (PFNGLUNIFORMMATRIX4FVARBPROC) load(userptr, "glUniformMatrix4fvARB");
    context->UseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) load(userptr, "glUseProgramObjectARB");
    context->ValidateProgramARB = (PFNGLVALIDATEPROGRAMARBPROC) load(userptr, "glValidateProgramARB");
}
static void glad_gl_load_GL_ARB_texture_compression(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_texture_compression) return;
    context->CompressedTexImage1DARB = (PFNGLCOMPRESSEDTEXIMAGE1DARBPROC) load(userptr, "glCompressedTexImage1DARB");
    context->CompressedTexImage2DARB = (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC) load(userptr, "glCompressedTexImage2DARB");
    context->CompressedTexImage3DARB = (PFNGLCOMPRESSEDTEXIMAGE3DARBPROC) load(userptr, "glCompressedTexImage3DARB");
    context->CompressedTexSubImage1DARB = (PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC) load(userptr, "glCompressedTexSubImage1DARB");
    context->CompressedTexSubImage2DARB = (PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC) load(userptr, "glCompressedTexSubImage2DARB");
    context->CompressedTexSubImage3DARB = (PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC) load(userptr, "glCompressedTexSubImage3DARB");
    context->GetCompressedTexImageARB = (PFNGLGETCOMPRESSEDTEXIMAGEARBPROC) load(userptr, "glGetCompressedTexImageARB");
}
static void glad_gl_load_GL_ARB_transpose_matrix(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_transpose_matrix) return;
    context->LoadTransposeMatrixdARB = (PFNGLLOADTRANSPOSEMATRIXDARBPROC) load(userptr, "glLoadTransposeMatrixdARB");
    context->LoadTransposeMatrixfARB = (PFNGLLOADTRANSPOSEMATRIXFARBPROC) load(userptr, "glLoadTransposeMatrixfARB");
    context->MultTransposeMatrixdARB = (PFNGLMULTTRANSPOSEMATRIXDARBPROC) load(userptr, "glMultTransposeMatrixdARB");
    context->MultTransposeMatrixfARB = (PFNGLMULTTRANSPOSEMATRIXFARBPROC) load(userptr, "glMultTransposeMatrixfARB");
}
static void glad_gl_load_GL_ARB_vertex_buffer_object(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_vertex_buffer_object) return;
    context->BindBufferARB = (PFNGLBINDBUFFERARBPROC) load(userptr, "glBindBufferARB");
    context->BufferDataARB = (PFNGLBUFFERDATAARBPROC) load(userptr, "glBufferDataARB");
    context->BufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC) load(userptr, "glBufferSubDataARB");
    context->DeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC) load(userptr, "glDeleteBuffersARB");
    context->GenBuffersARB = (PFNGLGENBUFFERSARBPROC) load(userptr, "glGenBuffersARB");
    context->GetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC) load(userptr, "glGetBufferParameterivARB");
    context->GetBufferPointervARB = (PFNGLGETBUFFERPOINTERVARBPROC) load(userptr, "glGetBufferPointervARB");
    context->GetBufferSubDataARB = (PFNGLGETBUFFERSUBDATAARBPROC) load(userptr, "glGetBufferSubDataARB");
    context->IsBufferARB = (PFNGLISBUFFERARBPROC) load(userptr, "glIsBufferARB");
    context->MapBufferARB = (PFNGLMAPBUFFERARBPROC) load(userptr, "glMapBufferARB");
    context->UnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC) load(userptr, "glUnmapBufferARB");
}
static void glad_gl_load_GL_ARB_vertex_program(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_vertex_program) return;
    context->BindProgramARB = (PFNGLBINDPROGRAMARBPROC) load(userptr, "glBindProgramARB");
    context->DeleteProgramsARB = (PFNGLDELETEPROGRAMSARBPROC) load(userptr, "glDeleteProgramsARB");
    context->DisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) load(userptr, "glDisableVertexAttribArrayARB");
    context->EnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC) load(userptr, "glEnableVertexAttribArrayARB");
    context->GenProgramsARB = (PFNGLGENPROGRAMSARBPROC) load(userptr, "glGenProgramsARB");
    context->GetProgramEnvParameterdvARB = (PFNGLGETPROGRAMENVPARAMETERDVARBPROC) load(userptr, "glGetProgramEnvParameterdvARB");
    context->GetProgramEnvParameterfvARB = (PFNGLGETPROGRAMENVPARAMETERFVARBPROC) load(userptr, "glGetProgramEnvParameterfvARB");
    context->GetProgramLocalParameterdvARB = (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) load(userptr, "glGetProgramLocalParameterdvARB");
    context->GetProgramLocalParameterfvARB = (PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) load(userptr, "glGetProgramLocalParameterfvARB");
    context->GetProgramStringARB = (PFNGLGETPROGRAMSTRINGARBPROC) load(userptr, "glGetProgramStringARB");
    context->GetProgramivARB = (PFNGLGETPROGRAMIVARBPROC) load(userptr, "glGetProgramivARB");
    context->GetVertexAttribPointervARB = (PFNGLGETVERTEXATTRIBPOINTERVARBPROC) load(userptr, "glGetVertexAttribPointervARB");
    context->GetVertexAttribdvARB = (PFNGLGETVERTEXATTRIBDVARBPROC) load(userptr, "glGetVertexAttribdvARB");
    context->GetVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) load(userptr, "glGetVertexAttribfvARB");
    context->GetVertexAttribivARB = (PFNGLGETVERTEXATTRIBIVARBPROC) load(userptr, "glGetVertexAttribivARB");
    context->IsProgramARB = (PFNGLISPROGRAMARBPROC) load(userptr, "glIsProgramARB");
    context->ProgramEnvParameter4dARB = (PFNGLPROGRAMENVPARAMETER4DARBPROC) load(userptr, "glProgramEnvParameter4dARB");
    context->ProgramEnvParameter4dvARB = (PFNGLPROGRAMENVPARAMETER4DVARBPROC) load(userptr, "glProgramEnvParameter4dvARB");
    context->ProgramEnvParameter4fARB = (PFNGLPROGRAMENVPARAMETER4FARBPROC) load(userptr, "glProgramEnvParameter4fARB");
    context->ProgramEnvParameter4fvARB = (PFNGLPROGRAMENVPARAMETER4FVARBPROC) load(userptr, "glProgramEnvParameter4fvARB");
    context->ProgramLocalParameter4dARB = (PFNGLPROGRAMLOCALPARAMETER4DARBPROC) load(userptr, "glProgramLocalParameter4dARB");
    context->ProgramLocalParameter4dvARB = (PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) load(userptr, "glProgramLocalParameter4dvARB");
    context->ProgramLocalParameter4fARB = (PFNGLPROGRAMLOCALPARAMETER4FARBPROC) load(userptr, "glProgramLocalParameter4fARB");
    context->ProgramLocalParameter4fvARB = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) load(userptr, "glProgramLocalParameter4fvARB");
    context->ProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC) load(userptr, "glProgramStringARB");
    context->VertexAttrib1dARB = (PFNGLVERTEXATTRIB1DARBPROC) load(userptr, "glVertexAttrib1dARB");
    context->VertexAttrib1dvARB = (PFNGLVERTEXATTRIB1DVARBPROC) load(userptr, "glVertexAttrib1dvARB");
    context->VertexAttrib1fARB = (PFNGLVERTEXATTRIB1FARBPROC) load(userptr, "glVertexAttrib1fARB");
    context->VertexAttrib1fvARB = (PFNGLVERTEXATTRIB1FVARBPROC) load(userptr, "glVertexAttrib1fvARB");
    context->VertexAttrib1sARB = (PFNGLVERTEXATTRIB1SARBPROC) load(userptr, "glVertexAttrib1sARB");
    context->VertexAttrib1svARB = (PFNGLVERTEXATTRIB1SVARBPROC) load(userptr, "glVertexAttrib1svARB");
    context->VertexAttrib2dARB = (PFNGLVERTEXATTRIB2DARBPROC) load(userptr, "glVertexAttrib2dARB");
    context->VertexAttrib2dvARB = (PFNGLVERTEXATTRIB2DVARBPROC) load(userptr, "glVertexAttrib2dvARB");
    context->VertexAttrib2fARB = (PFNGLVERTEXATTRIB2FARBPROC) load(userptr, "glVertexAttrib2fARB");
    context->VertexAttrib2fvARB = (PFNGLVERTEXATTRIB2FVARBPROC) load(userptr, "glVertexAttrib2fvARB");
    context->VertexAttrib2sARB = (PFNGLVERTEXATTRIB2SARBPROC) load(userptr, "glVertexAttrib2sARB");
    context->VertexAttrib2svARB = (PFNGLVERTEXATTRIB2SVARBPROC) load(userptr, "glVertexAttrib2svARB");
    context->VertexAttrib3dARB = (PFNGLVERTEXATTRIB3DARBPROC) load(userptr, "glVertexAttrib3dARB");
    context->VertexAttrib3dvARB = (PFNGLVERTEXATTRIB3DVARBPROC) load(userptr, "glVertexAttrib3dvARB");
    context->VertexAttrib3fARB = (PFNGLVERTEXATTRIB3FARBPROC) load(userptr, "glVertexAttrib3fARB");
    context->VertexAttrib3fvARB = (PFNGLVERTEXATTRIB3FVARBPROC) load(userptr, "glVertexAttrib3fvARB");
    context->VertexAttrib3sARB = (PFNGLVERTEXATTRIB3SARBPROC) load(userptr, "glVertexAttrib3sARB");
    context->VertexAttrib3svARB = (PFNGLVERTEXATTRIB3SVARBPROC) load(userptr, "glVertexAttrib3svARB");
    context->VertexAttrib4NbvARB = (PFNGLVERTEXATTRIB4NBVARBPROC) load(userptr, "glVertexAttrib4NbvARB");
    context->VertexAttrib4NivARB = (PFNGLVERTEXATTRIB4NIVARBPROC) load(userptr, "glVertexAttrib4NivARB");
    context->VertexAttrib4NsvARB = (PFNGLVERTEXATTRIB4NSVARBPROC) load(userptr, "glVertexAttrib4NsvARB");
    context->VertexAttrib4NubARB = (PFNGLVERTEXATTRIB4NUBARBPROC) load(userptr, "glVertexAttrib4NubARB");
    context->VertexAttrib4NubvARB = (PFNGLVERTEXATTRIB4NUBVARBPROC) load(userptr, "glVertexAttrib4NubvARB");
    context->VertexAttrib4NuivARB = (PFNGLVERTEXATTRIB4NUIVARBPROC) load(userptr, "glVertexAttrib4NuivARB");
    context->VertexAttrib4NusvARB = (PFNGLVERTEXATTRIB4NUSVARBPROC) load(userptr, "glVertexAttrib4NusvARB");
    context->VertexAttrib4bvARB = (PFNGLVERTEXATTRIB4BVARBPROC) load(userptr, "glVertexAttrib4bvARB");
    context->VertexAttrib4dARB = (PFNGLVERTEXATTRIB4DARBPROC) load(userptr, "glVertexAttrib4dARB");
    context->VertexAttrib4dvARB = (PFNGLVERTEXATTRIB4DVARBPROC) load(userptr, "glVertexAttrib4dvARB");
    context->VertexAttrib4fARB = (PFNGLVERTEXATTRIB4FARBPROC) load(userptr, "glVertexAttrib4fARB");
    context->VertexAttrib4fvARB = (PFNGLVERTEXATTRIB4FVARBPROC) load(userptr, "glVertexAttrib4fvARB");
    context->VertexAttrib4ivARB = (PFNGLVERTEXATTRIB4IVARBPROC) load(userptr, "glVertexAttrib4ivARB");
    context->VertexAttrib4sARB = (PFNGLVERTEXATTRIB4SARBPROC) load(userptr, "glVertexAttrib4sARB");
    context->VertexAttrib4svARB = (PFNGLVERTEXATTRIB4SVARBPROC) load(userptr, "glVertexAttrib4svARB");
    context->VertexAttrib4ubvARB = (PFNGLVERTEXATTRIB4UBVARBPROC) load(userptr, "glVertexAttrib4ubvARB");
    context->VertexAttrib4uivARB = (PFNGLVERTEXATTRIB4UIVARBPROC) load(userptr, "glVertexAttrib4uivARB");
    context->VertexAttrib4usvARB = (PFNGLVERTEXATTRIB4USVARBPROC) load(userptr, "glVertexAttrib4usvARB");
    context->VertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC) load(userptr, "glVertexAttribPointerARB");
}
static void glad_gl_load_GL_ARB_vertex_shader(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_vertex_shader) return;
    context->BindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC) load(userptr, "glBindAttribLocationARB");
    context->DisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) load(userptr, "glDisableVertexAttribArrayARB");
    context->EnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC) load(userptr, "glEnableVertexAttribArrayARB");
    context->GetActiveAttribARB = (PFNGLGETACTIVEATTRIBARBPROC) load(userptr, "glGetActiveAttribARB");
    context->GetAttribLocationARB = (PFNGLGETATTRIBLOCATIONARBPROC) load(userptr, "glGetAttribLocationARB");
    context->GetVertexAttribPointervARB = (PFNGLGETVERTEXATTRIBPOINTERVARBPROC) load(userptr, "glGetVertexAttribPointervARB");
    context->GetVertexAttribdvARB = (PFNGLGETVERTEXATTRIBDVARBPROC) load(userptr, "glGetVertexAttribdvARB");
    context->GetVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) load(userptr, "glGetVertexAttribfvARB");
    context->GetVertexAttribivARB = (PFNGLGETVERTEXATTRIBIVARBPROC) load(userptr, "glGetVertexAttribivARB");
    context->VertexAttrib1dARB = (PFNGLVERTEXATTRIB1DARBPROC) load(userptr, "glVertexAttrib1dARB");
    context->VertexAttrib1dvARB = (PFNGLVERTEXATTRIB1DVARBPROC) load(userptr, "glVertexAttrib1dvARB");
    context->VertexAttrib1fARB = (PFNGLVERTEXATTRIB1FARBPROC) load(userptr, "glVertexAttrib1fARB");
    context->VertexAttrib1fvARB = (PFNGLVERTEXATTRIB1FVARBPROC) load(userptr, "glVertexAttrib1fvARB");
    context->VertexAttrib1sARB = (PFNGLVERTEXATTRIB1SARBPROC) load(userptr, "glVertexAttrib1sARB");
    context->VertexAttrib1svARB = (PFNGLVERTEXATTRIB1SVARBPROC) load(userptr, "glVertexAttrib1svARB");
    context->VertexAttrib2dARB = (PFNGLVERTEXATTRIB2DARBPROC) load(userptr, "glVertexAttrib2dARB");
    context->VertexAttrib2dvARB = (PFNGLVERTEXATTRIB2DVARBPROC) load(userptr, "glVertexAttrib2dvARB");
    context->VertexAttrib2fARB = (PFNGLVERTEXATTRIB2FARBPROC) load(userptr, "glVertexAttrib2fARB");
    context->VertexAttrib2fvARB = (PFNGLVERTEXATTRIB2FVARBPROC) load(userptr, "glVertexAttrib2fvARB");
    context->VertexAttrib2sARB = (PFNGLVERTEXATTRIB2SARBPROC) load(userptr, "glVertexAttrib2sARB");
    context->VertexAttrib2svARB = (PFNGLVERTEXATTRIB2SVARBPROC) load(userptr, "glVertexAttrib2svARB");
    context->VertexAttrib3dARB = (PFNGLVERTEXATTRIB3DARBPROC) load(userptr, "glVertexAttrib3dARB");
    context->VertexAttrib3dvARB = (PFNGLVERTEXATTRIB3DVARBPROC) load(userptr, "glVertexAttrib3dvARB");
    context->VertexAttrib3fARB = (PFNGLVERTEXATTRIB3FARBPROC) load(userptr, "glVertexAttrib3fARB");
    context->VertexAttrib3fvARB = (PFNGLVERTEXATTRIB3FVARBPROC) load(userptr, "glVertexAttrib3fvARB");
    context->VertexAttrib3sARB = (PFNGLVERTEXATTRIB3SARBPROC) load(userptr, "glVertexAttrib3sARB");
    context->VertexAttrib3svARB = (PFNGLVERTEXATTRIB3SVARBPROC) load(userptr, "glVertexAttrib3svARB");
    context->VertexAttrib4NbvARB = (PFNGLVERTEXATTRIB4NBVARBPROC) load(userptr, "glVertexAttrib4NbvARB");
    context->VertexAttrib4NivARB = (PFNGLVERTEXATTRIB4NIVARBPROC) load(userptr, "glVertexAttrib4NivARB");
    context->VertexAttrib4NsvARB = (PFNGLVERTEXATTRIB4NSVARBPROC) load(userptr, "glVertexAttrib4NsvARB");
    context->VertexAttrib4NubARB = (PFNGLVERTEXATTRIB4NUBARBPROC) load(userptr, "glVertexAttrib4NubARB");
    context->VertexAttrib4NubvARB = (PFNGLVERTEXATTRIB4NUBVARBPROC) load(userptr, "glVertexAttrib4NubvARB");
    context->VertexAttrib4NuivARB = (PFNGLVERTEXATTRIB4NUIVARBPROC) load(userptr, "glVertexAttrib4NuivARB");
    context->VertexAttrib4NusvARB = (PFNGLVERTEXATTRIB4NUSVARBPROC) load(userptr, "glVertexAttrib4NusvARB");
    context->VertexAttrib4bvARB = (PFNGLVERTEXATTRIB4BVARBPROC) load(userptr, "glVertexAttrib4bvARB");
    context->VertexAttrib4dARB = (PFNGLVERTEXATTRIB4DARBPROC) load(userptr, "glVertexAttrib4dARB");
    context->VertexAttrib4dvARB = (PFNGLVERTEXATTRIB4DVARBPROC) load(userptr, "glVertexAttrib4dvARB");
    context->VertexAttrib4fARB = (PFNGLVERTEXATTRIB4FARBPROC) load(userptr, "glVertexAttrib4fARB");
    context->VertexAttrib4fvARB = (PFNGLVERTEXATTRIB4FVARBPROC) load(userptr, "glVertexAttrib4fvARB");
    context->VertexAttrib4ivARB = (PFNGLVERTEXATTRIB4IVARBPROC) load(userptr, "glVertexAttrib4ivARB");
    context->VertexAttrib4sARB = (PFNGLVERTEXATTRIB4SARBPROC) load(userptr, "glVertexAttrib4sARB");
    context->VertexAttrib4svARB = (PFNGLVERTEXATTRIB4SVARBPROC) load(userptr, "glVertexAttrib4svARB");
    context->VertexAttrib4ubvARB = (PFNGLVERTEXATTRIB4UBVARBPROC) load(userptr, "glVertexAttrib4ubvARB");
    context->VertexAttrib4uivARB = (PFNGLVERTEXATTRIB4UIVARBPROC) load(userptr, "glVertexAttrib4uivARB");
    context->VertexAttrib4usvARB = (PFNGLVERTEXATTRIB4USVARBPROC) load(userptr, "glVertexAttrib4usvARB");
    context->VertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC) load(userptr, "glVertexAttribPointerARB");
}
static void glad_gl_load_GL_ARB_window_pos(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_window_pos) return;
    context->WindowPos2dARB = (PFNGLWINDOWPOS2DARBPROC) load(userptr, "glWindowPos2dARB");
    context->WindowPos2dvARB = (PFNGLWINDOWPOS2DVARBPROC) load(userptr, "glWindowPos2dvARB");
    context->WindowPos2fARB = (PFNGLWINDOWPOS2FARBPROC) load(userptr, "glWindowPos2fARB");
    context->WindowPos2fvARB = (PFNGLWINDOWPOS2FVARBPROC) load(userptr, "glWindowPos2fvARB");
    context->WindowPos2iARB = (PFNGLWINDOWPOS2IARBPROC) load(userptr, "glWindowPos2iARB");
    context->WindowPos2ivARB = (PFNGLWINDOWPOS2IVARBPROC) load(userptr, "glWindowPos2ivARB");
    context->WindowPos2sARB = (PFNGLWINDOWPOS2SARBPROC) load(userptr, "glWindowPos2sARB");
    context->WindowPos2svARB = (PFNGLWINDOWPOS2SVARBPROC) load(userptr, "glWindowPos2svARB");
    context->WindowPos3dARB = (PFNGLWINDOWPOS3DARBPROC) load(userptr, "glWindowPos3dARB");
    context->WindowPos3dvARB = (PFNGLWINDOWPOS3DVARBPROC) load(userptr, "glWindowPos3dvARB");
    context->WindowPos3fARB = (PFNGLWINDOWPOS3FARBPROC) load(userptr, "glWindowPos3fARB");
    context->WindowPos3fvARB = (PFNGLWINDOWPOS3FVARBPROC) load(userptr, "glWindowPos3fvARB");
    context->WindowPos3iARB = (PFNGLWINDOWPOS3IARBPROC) load(userptr, "glWindowPos3iARB");
    context->WindowPos3ivARB = (PFNGLWINDOWPOS3IVARBPROC) load(userptr, "glWindowPos3ivARB");
    context->WindowPos3sARB = (PFNGLWINDOWPOS3SARBPROC) load(userptr, "glWindowPos3sARB");
    context->WindowPos3svARB = (PFNGLWINDOWPOS3SVARBPROC) load(userptr, "glWindowPos3svARB");
}
static void glad_gl_load_GL_ATI_draw_buffers(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ATI_draw_buffers) return;
    context->DrawBuffersATI = (PFNGLDRAWBUFFERSATIPROC) load(userptr, "glDrawBuffersATI");
}
static void glad_gl_load_GL_ATI_separate_stencil(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ATI_separate_stencil) return;
    context->StencilFuncSeparateATI = (PFNGLSTENCILFUNCSEPARATEATIPROC) load(userptr, "glStencilFuncSeparateATI");
    context->StencilOpSeparateATI = (PFNGLSTENCILOPSEPARATEATIPROC) load(userptr, "glStencilOpSeparateATI");
}
static void glad_gl_load_GL_EXT_blend_color(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_blend_color) return;
    context->BlendColorEXT = (PFNGLBLENDCOLOREXTPROC) load(userptr, "glBlendColorEXT");
}
static void glad_gl_load_GL_EXT_blend_equation_separate(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_blend_equation_separate) return;
    context->BlendEquationSeparateEXT = (PFNGLBLENDEQUATIONSEPARATEEXTPROC) load(userptr, "glBlendEquationSeparateEXT");
}
static void glad_gl_load_GL_EXT_blend_func_separate(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_blend_func_separate) return;
    context->BlendFuncSeparateEXT = (PFNGLBLENDFUNCSEPARATEEXTPROC) load(userptr, "glBlendFuncSeparateEXT");
}
static void glad_gl_load_GL_EXT_blend_minmax(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_blend_minmax) return;
    context->BlendEquationEXT = (PFNGLBLENDEQUATIONEXTPROC) load(userptr, "glBlendEquationEXT");
}
static void glad_gl_load_GL_EXT_copy_texture(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_copy_texture) return;
    context->CopyTexImage1DEXT = (PFNGLCOPYTEXIMAGE1DEXTPROC) load(userptr, "glCopyTexImage1DEXT");
    context->CopyTexImage2DEXT = (PFNGLCOPYTEXIMAGE2DEXTPROC) load(userptr, "glCopyTexImage2DEXT");
    context->CopyTexSubImage1DEXT = (PFNGLCOPYTEXSUBIMAGE1DEXTPROC) load(userptr, "glCopyTexSubImage1DEXT");
    context->CopyTexSubImage2DEXT = (PFNGLCOPYTEXSUBIMAGE2DEXTPROC) load(userptr, "glCopyTexSubImage2DEXT");
    context->CopyTexSubImage3DEXT = (PFNGLCOPYTEXSUBIMAGE3DEXTPROC) load(userptr, "glCopyTexSubImage3DEXT");
}
static void glad_gl_load_GL_EXT_draw_range_elements(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_draw_range_elements) return;
    context->DrawRangeElementsEXT = (PFNGLDRAWRANGEELEMENTSEXTPROC) load(userptr, "glDrawRangeElementsEXT");
}
static void glad_gl_load_GL_EXT_fog_coord(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_fog_coord) return;
    context->FogCoordPointerEXT = (PFNGLFOGCOORDPOINTEREXTPROC) load(userptr, "glFogCoordPointerEXT");
    context->FogCoorddEXT = (PFNGLFOGCOORDDEXTPROC) load(userptr, "glFogCoorddEXT");
    context->FogCoorddvEXT = (PFNGLFOGCOORDDVEXTPROC) load(userptr, "glFogCoorddvEXT");
    context->FogCoordfEXT = (PFNGLFOGCOORDFEXTPROC) load(userptr, "glFogCoordfEXT");
    context->FogCoordfvEXT = (PFNGLFOGCOORDFVEXTPROC) load(userptr, "glFogCoordfvEXT");
}
static void glad_gl_load_GL_EXT_framebuffer_blit(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_framebuffer_blit) return;
    context->BlitFramebufferEXT = (PFNGLBLITFRAMEBUFFEREXTPROC) load(userptr, "glBlitFramebufferEXT");
}
static void glad_gl_load_GL_EXT_framebuffer_multisample(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_framebuffer_multisample) return;
    context->RenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) load(userptr, "glRenderbufferStorageMultisampleEXT");
}
static void glad_gl_load_GL_EXT_framebuffer_object(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_framebuffer_object) return;
    context->BindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC) load(userptr, "glBindFramebufferEXT");
    context->BindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC) load(userptr, "glBindRenderbufferEXT");
    context->CheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) load(userptr, "glCheckFramebufferStatusEXT");
    context->DeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC) load(userptr, "glDeleteFramebuffersEXT");
    context->DeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC) load(userptr, "glDeleteRenderbuffersEXT");
    context->FramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) load(userptr, "glFramebufferRenderbufferEXT");
    context->FramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) load(userptr, "glFramebufferTexture1DEXT");
    context->FramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) load(userptr, "glFramebufferTexture2DEXT");
    context->FramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) load(userptr, "glFramebufferTexture3DEXT");
    context->GenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC) load(userptr, "glGenFramebuffersEXT");
    context->GenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC) load(userptr, "glGenRenderbuffersEXT");
    context->GenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC) load(userptr, "glGenerateMipmapEXT");
    context->GetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) load(userptr, "glGetFramebufferAttachmentParameterivEXT");
    context->GetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) load(userptr, "glGetRenderbufferParameterivEXT");
    context->IsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC) load(userptr, "glIsFramebufferEXT");
    context->IsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC) load(userptr, "glIsRenderbufferEXT");
    context->RenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC) load(userptr, "glRenderbufferStorageEXT");
}
static void glad_gl_load_GL_EXT_multi_draw_arrays(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_multi_draw_arrays) return;
    context->MultiDrawArraysEXT = (PFNGLMULTIDRAWARRAYSEXTPROC) load(userptr, "glMultiDrawArraysEXT");
    context->MultiDrawElementsEXT = (PFNGLMULTIDRAWELEMENTSEXTPROC) load(userptr, "glMultiDrawElementsEXT");
}
static void glad_gl_load_GL_EXT_point_parameters(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_point_parameters) return;
    context->PointParameterfEXT = (PFNGLPOINTPARAMETERFEXTPROC) load(userptr, "glPointParameterfEXT");
    context->PointParameterfvEXT = (PFNGLPOINTPARAMETERFVEXTPROC) load(userptr, "glPointParameterfvEXT");
}
static void glad_gl_load_GL_EXT_secondary_color(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_secondary_color) return;
    context->SecondaryColor3bEXT = (PFNGLSECONDARYCOLOR3BEXTPROC) load(userptr, "glSecondaryColor3bEXT");
    context->SecondaryColor3bvEXT = (PFNGLSECONDARYCOLOR3BVEXTPROC) load(userptr, "glSecondaryColor3bvEXT");
    context->SecondaryColor3dEXT = (PFNGLSECONDARYCOLOR3DEXTPROC) load(userptr, "glSecondaryColor3dEXT");
    context->SecondaryColor3dvEXT = (PFNGLSECONDARYCOLOR3DVEXTPROC) load(userptr, "glSecondaryColor3dvEXT");
    context->SecondaryColor3fEXT = (PFNGLSECONDARYCOLOR3FEXTPROC) load(userptr, "glSecondaryColor3fEXT");
    context->SecondaryColor3fvEXT = (PFNGLSECONDARYCOLOR3FVEXTPROC) load(userptr, "glSecondaryColor3fvEXT");
    context->SecondaryColor3iEXT = (PFNGLSECONDARYCOLOR3IEXTPROC) load(userptr, "glSecondaryColor3iEXT");
    context->SecondaryColor3ivEXT = (PFNGLSECONDARYCOLOR3IVEXTPROC) load(userptr, "glSecondaryColor3ivEXT");
    context->SecondaryColor3sEXT = (PFNGLSECONDARYCOLOR3SEXTPROC) load(userptr, "glSecondaryColor3sEXT");
    context->SecondaryColor3svEXT = (PFNGLSECONDARYCOLOR3SVEXTPROC) load(userptr, "glSecondaryColor3svEXT");
    context->SecondaryColor3ubEXT = (PFNGLSECONDARYCOLOR3UBEXTPROC) load(userptr, "glSecondaryColor3ubEXT");
    context->SecondaryColor3ubvEXT = (PFNGLSECONDARYCOLOR3UBVEXTPROC) load(userptr, "glSecondaryColor3ubvEXT");
    context->SecondaryColor3uiEXT = (PFNGLSECONDARYCOLOR3UIEXTPROC) load(userptr, "glSecondaryColor3uiEXT");
    context->SecondaryColor3uivEXT = (PFNGLSECONDARYCOLOR3UIVEXTPROC) load(userptr, "glSecondaryColor3uivEXT");
    context->SecondaryColor3usEXT = (PFNGLSECONDARYCOLOR3USEXTPROC) load(userptr, "glSecondaryColor3usEXT");
    context->SecondaryColor3usvEXT = (PFNGLSECONDARYCOLOR3USVEXTPROC) load(userptr, "glSecondaryColor3usvEXT");
    context->SecondaryColorPointerEXT = (PFNGLSECONDARYCOLORPOINTEREXTPROC) load(userptr, "glSecondaryColorPointerEXT");
}
static void glad_gl_load_GL_EXT_subtexture(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_subtexture) return;
    context->TexSubImage1DEXT = (PFNGLTEXSUBIMAGE1DEXTPROC) load(userptr, "glTexSubImage1DEXT");
    context->TexSubImage2DEXT = (PFNGLTEXSUBIMAGE2DEXTPROC) load(userptr, "glTexSubImage2DEXT");
}
static void glad_gl_load_GL_EXT_texture3D(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_texture3D) return;
    context->TexImage3DEXT = (PFNGLTEXIMAGE3DEXTPROC) load(userptr, "glTexImage3DEXT");
    context->TexSubImage3DEXT = (PFNGLTEXSUBIMAGE3DEXTPROC) load(userptr, "glTexSubImage3DEXT");
}
static void glad_gl_load_GL_EXT_texture_array(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_texture_array) return;
    context->FramebufferTextureLayerEXT = (PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC) load(userptr, "glFramebufferTextureLayerEXT");
}
static void glad_gl_load_GL_EXT_texture_object(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_texture_object) return;
    context->AreTexturesResidentEXT = (PFNGLARETEXTURESRESIDENTEXTPROC) load(userptr, "glAreTexturesResidentEXT");
    context->BindTextureEXT = (PFNGLBINDTEXTUREEXTPROC) load(userptr, "glBindTextureEXT");
    context->DeleteTexturesEXT = (PFNGLDELETETEXTURESEXTPROC) load(userptr, "glDeleteTexturesEXT");
    context->GenTexturesEXT = (PFNGLGENTEXTURESEXTPROC) load(userptr, "glGenTexturesEXT");
    context->IsTextureEXT = (PFNGLISTEXTUREEXTPROC) load(userptr, "glIsTextureEXT");
    context->PrioritizeTexturesEXT = (PFNGLPRIORITIZETEXTURESEXTPROC) load(userptr, "glPrioritizeTexturesEXT");
}
static void glad_gl_load_GL_EXT_vertex_array(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_vertex_array) return;
    context->ArrayElementEXT = (PFNGLARRAYELEMENTEXTPROC) load(userptr, "glArrayElementEXT");
    context->ColorPointerEXT = (PFNGLCOLORPOINTEREXTPROC) load(userptr, "glColorPointerEXT");
    context->DrawArraysEXT = (PFNGLDRAWARRAYSEXTPROC) load(userptr, "glDrawArraysEXT");
    context->EdgeFlagPointerEXT = (PFNGLEDGEFLAGPOINTEREXTPROC) load(userptr, "glEdgeFlagPointerEXT");
    context->GetPointervEXT = (PFNGLGETPOINTERVEXTPROC) load(userptr, "glGetPointervEXT");
    context->IndexPointerEXT = (PFNGLINDEXPOINTEREXTPROC) load(userptr, "glIndexPointerEXT");
    context->NormalPointerEXT = (PFNGLNORMALPOINTEREXTPROC) load(userptr, "glNormalPointerEXT");
    context->TexCoordPointerEXT = (PFNGLTEXCOORDPOINTEREXTPROC) load(userptr, "glTexCoordPointerEXT");
    context->VertexPointerEXT = (PFNGLVERTEXPOINTEREXTPROC) load(userptr, "glVertexPointerEXT");
}
static void glad_gl_load_GL_INGR_blend_func_separate(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->INGR_blend_func_separate) return;
    context->BlendFuncSeparateINGR = (PFNGLBLENDFUNCSEPARATEINGRPROC) load(userptr, "glBlendFuncSeparateINGR");
}
static void glad_gl_load_GL_KHR_debug(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->KHR_debug) return;
    context->DebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC) load(userptr, "glDebugMessageCallback");
    context->DebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC) load(userptr, "glDebugMessageControl");
    context->DebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC) load(userptr, "glDebugMessageInsert");
    context->GetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC) load(userptr, "glGetDebugMessageLog");
    context->GetObjectLabel = (PFNGLGETOBJECTLABELPROC) load(userptr, "glGetObjectLabel");
    context->GetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC) load(userptr, "glGetObjectPtrLabel");
    context->GetPointerv = (PFNGLGETPOINTERVPROC) load(userptr, "glGetPointerv");
    context->ObjectLabel = (PFNGLOBJECTLABELPROC) load(userptr, "glObjectLabel");
    context->ObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC) load(userptr, "glObjectPtrLabel");
    context->PopDebugGroup = (PFNGLPOPDEBUGGROUPPROC) load(userptr, "glPopDebugGroup");
    context->PushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC) load(userptr, "glPushDebugGroup");
}
static void glad_gl_load_GL_MESA_window_pos(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->MESA_window_pos) return;
    context->WindowPos2dMESA = (PFNGLWINDOWPOS2DMESAPROC) load(userptr, "glWindowPos2dMESA");
    context->WindowPos2dvMESA = (PFNGLWINDOWPOS2DVMESAPROC) load(userptr, "glWindowPos2dvMESA");
    context->WindowPos2fMESA = (PFNGLWINDOWPOS2FMESAPROC) load(userptr, "glWindowPos2fMESA");
    context->WindowPos2fvMESA = (PFNGLWINDOWPOS2FVMESAPROC) load(userptr, "glWindowPos2fvMESA");
    context->WindowPos2iMESA = (PFNGLWINDOWPOS2IMESAPROC) load(userptr, "glWindowPos2iMESA");
    context->WindowPos2ivMESA = (PFNGLWINDOWPOS2IVMESAPROC) load(userptr, "glWindowPos2ivMESA");
    context->WindowPos2sMESA = (PFNGLWINDOWPOS2SMESAPROC) load(userptr, "glWindowPos2sMESA");
    context->WindowPos2svMESA = (PFNGLWINDOWPOS2SVMESAPROC) load(userptr, "glWindowPos2svMESA");
    context->WindowPos3dMESA = (PFNGLWINDOWPOS3DMESAPROC) load(userptr, "glWindowPos3dMESA");
    context->WindowPos3dvMESA = (PFNGLWINDOWPOS3DVMESAPROC) load(userptr, "glWindowPos3dvMESA");
    context->WindowPos3fMESA = (PFNGLWINDOWPOS3FMESAPROC) load(userptr, "glWindowPos3fMESA");
    context->WindowPos3fvMESA = (PFNGLWINDOWPOS3FVMESAPROC) load(userptr, "glWindowPos3fvMESA");
    context->WindowPos3iMESA = (PFNGLWINDOWPOS3IMESAPROC) load(userptr, "glWindowPos3iMESA");
    context->WindowPos3ivMESA = (PFNGLWINDOWPOS3IVMESAPROC) load(userptr, "glWindowPos3ivMESA");
    context->WindowPos3sMESA = (PFNGLWINDOWPOS3SMESAPROC) load(userptr, "glWindowPos3sMESA");
    context->WindowPos3svMESA = (PFNGLWINDOWPOS3SVMESAPROC) load(userptr, "glWindowPos3svMESA");
    context->WindowPos4dMESA = (PFNGLWINDOWPOS4DMESAPROC) load(userptr, "glWindowPos4dMESA");
    context->WindowPos4dvMESA = (PFNGLWINDOWPOS4DVMESAPROC) load(userptr, "glWindowPos4dvMESA");
    context->WindowPos4fMESA = (PFNGLWINDOWPOS4FMESAPROC) load(userptr, "glWindowPos4fMESA");
    context->WindowPos4fvMESA = (PFNGLWINDOWPOS4FVMESAPROC) load(userptr, "glWindowPos4fvMESA");
    context->WindowPos4iMESA = (PFNGLWINDOWPOS4IMESAPROC) load(userptr, "glWindowPos4iMESA");
    context->WindowPos4ivMESA = (PFNGLWINDOWPOS4IVMESAPROC) load(userptr, "glWindowPos4ivMESA");
    context->WindowPos4sMESA = (PFNGLWINDOWPOS4SMESAPROC) load(userptr, "glWindowPos4sMESA");
    context->WindowPos4svMESA = (PFNGLWINDOWPOS4SVMESAPROC) load(userptr, "glWindowPos4svMESA");
}
static void glad_gl_load_GL_NV_geometry_program4(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->NV_geometry_program4) return;
    context->FramebufferTextureEXT = (PFNGLFRAMEBUFFERTEXTUREEXTPROC) load(userptr, "glFramebufferTextureEXT");
    context->FramebufferTextureFaceEXT = (PFNGLFRAMEBUFFERTEXTUREFACEEXTPROC) load(userptr, "glFramebufferTextureFaceEXT");
    context->FramebufferTextureLayerEXT = (PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC) load(userptr, "glFramebufferTextureLayerEXT");
    context->ProgramVertexLimitNV = (PFNGLPROGRAMVERTEXLIMITNVPROC) load(userptr, "glProgramVertexLimitNV");
}
static void glad_gl_load_GL_NV_point_sprite(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->NV_point_sprite) return;
    context->PointParameteriNV = (PFNGLPOINTPARAMETERINVPROC) load(userptr, "glPointParameteriNV");
    context->PointParameterivNV = (PFNGLPOINTPARAMETERIVNVPROC) load(userptr, "glPointParameterivNV");
}
static void glad_gl_load_GL_NV_vertex_program(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->NV_vertex_program) return;
    context->AreProgramsResidentNV = (PFNGLAREPROGRAMSRESIDENTNVPROC) load(userptr, "glAreProgramsResidentNV");
    context->BindProgramNV = (PFNGLBINDPROGRAMNVPROC) load(userptr, "glBindProgramNV");
    context->DeleteProgramsNV = (PFNGLDELETEPROGRAMSNVPROC) load(userptr, "glDeleteProgramsNV");
    context->ExecuteProgramNV = (PFNGLEXECUTEPROGRAMNVPROC) load(userptr, "glExecuteProgramNV");
    context->GenProgramsNV = (PFNGLGENPROGRAMSNVPROC) load(userptr, "glGenProgramsNV");
    context->GetProgramParameterdvNV = (PFNGLGETPROGRAMPARAMETERDVNVPROC) load(userptr, "glGetProgramParameterdvNV");
    context->GetProgramParameterfvNV = (PFNGLGETPROGRAMPARAMETERFVNVPROC) load(userptr, "glGetProgramParameterfvNV");
    context->GetProgramStringNV = (PFNGLGETPROGRAMSTRINGNVPROC) load(userptr, "glGetProgramStringNV");
    context->GetProgramivNV = (PFNGLGETPROGRAMIVNVPROC) load(userptr, "glGetProgramivNV");
    context->GetTrackMatrixivNV = (PFNGLGETTRACKMATRIXIVNVPROC) load(userptr, "glGetTrackMatrixivNV");
    context->GetVertexAttribPointervNV = (PFNGLGETVERTEXATTRIBPOINTERVNVPROC) load(userptr, "glGetVertexAttribPointervNV");
    context->GetVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC) load(userptr, "glGetVertexAttribdvNV");
    context->GetVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) load(userptr, "glGetVertexAttribfvNV");
    context->GetVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC) load(userptr, "glGetVertexAttribivNV");
    context->IsProgramNV = (PFNGLISPROGRAMNVPROC) load(userptr, "glIsProgramNV");
    context->LoadProgramNV = (PFNGLLOADPROGRAMNVPROC) load(userptr, "glLoadProgramNV");
    context->ProgramParameter4dNV = (PFNGLPROGRAMPARAMETER4DNVPROC) load(userptr, "glProgramParameter4dNV");
    context->ProgramParameter4dvNV = (PFNGLPROGRAMPARAMETER4DVNVPROC) load(userptr, "glProgramParameter4dvNV");
    context->ProgramParameter4fNV = (PFNGLPROGRAMPARAMETER4FNVPROC) load(userptr, "glProgramParameter4fNV");
    context->ProgramParameter4fvNV = (PFNGLPROGRAMPARAMETER4FVNVPROC) load(userptr, "glProgramParameter4fvNV");
    context->ProgramParameters4dvNV = (PFNGLPROGRAMPARAMETERS4DVNVPROC) load(userptr, "glProgramParameters4dvNV");
    context->ProgramParameters4fvNV = (PFNGLPROGRAMPARAMETERS4FVNVPROC) load(userptr, "glProgramParameters4fvNV");
    context->RequestResidentProgramsNV = (PFNGLREQUESTRESIDENTPROGRAMSNVPROC) load(userptr, "glRequestResidentProgramsNV");
    context->TrackMatrixNV = (PFNGLTRACKMATRIXNVPROC) load(userptr, "glTrackMatrixNV");
    context->VertexAttrib1dNV = (PFNGLVERTEXATTRIB1DNVPROC) load(userptr, "glVertexAttrib1dNV");
    context->VertexAttrib1dvNV = (PFNGLVERTEXATTRIB1DVNVPROC) load(userptr, "glVertexAttrib1dvNV");
    context->VertexAttrib1fNV = (PFNGLVERTEXATTRIB1FNVPROC) load(userptr, "glVertexAttrib1fNV");
    context->VertexAttrib1fvNV = (PFNGLVERTEXATTRIB1FVNVPROC) load(userptr, "glVertexAttrib1fvNV");
    context->VertexAttrib1sNV = (PFNGLVERTEXATTRIB1SNVPROC) load(userptr, "glVertexAttrib1sNV");
    context->VertexAttrib1svNV = (PFNGLVERTEXATTRIB1SVNVPROC) load(userptr, "glVertexAttrib1svNV");
    context->VertexAttrib2dNV = (PFNGLVERTEXATTRIB2DNVPROC) load(userptr, "glVertexAttrib2dNV");
    context->VertexAttrib2dvNV = (PFNGLVERTEXATTRIB2DVNVPROC) load(userptr, "glVertexAttrib2dvNV");
    context->VertexAttrib2fNV = (PFNGLVERTEXATTRIB2FNVPROC) load(userptr, "glVertexAttrib2fNV");
    context->VertexAttrib2fvNV = (PFNGLVERTEXATTRIB2FVNVPROC) load(userptr, "glVertexAttrib2fvNV");
    context->VertexAttrib2sNV = (PFNGLVERTEXATTRIB2SNVPROC) load(userptr, "glVertexAttrib2sNV");
    context->VertexAttrib2svNV = (PFNGLVERTEXATTRIB2SVNVPROC) load(userptr, "glVertexAttrib2svNV");
    context->VertexAttrib3dNV = (PFNGLVERTEXATTRIB3DNVPROC) load(userptr, "glVertexAttrib3dNV");
    context->VertexAttrib3dvNV = (PFNGLVERTEXATTRIB3DVNVPROC) load(userptr, "glVertexAttrib3dvNV");
    context->VertexAttrib3fNV = (PFNGLVERTEXATTRIB3FNVPROC) load(userptr, "glVertexAttrib3fNV");
    context->VertexAttrib3fvNV = (PFNGLVERTEXATTRIB3FVNVPROC) load(userptr, "glVertexAttrib3fvNV");
    context->VertexAttrib3sNV = (PFNGLVERTEXATTRIB3SNVPROC) load(userptr, "glVertexAttrib3sNV");
    context->VertexAttrib3svNV = (PFNGLVERTEXATTRIB3SVNVPROC) load(userptr, "glVertexAttrib3svNV");
    context->VertexAttrib4dNV = (PFNGLVERTEXATTRIB4DNVPROC) load(userptr, "glVertexAttrib4dNV");
    context->VertexAttrib4dvNV = (PFNGLVERTEXATTRIB4DVNVPROC) load(userptr, "glVertexAttrib4dvNV");
    context->VertexAttrib4fNV = (PFNGLVERTEXATTRIB4FNVPROC) load(userptr, "glVertexAttrib4fNV");
    context->VertexAttrib4fvNV = (PFNGLVERTEXATTRIB4FVNVPROC) load(userptr, "glVertexAttrib4fvNV");
    context->VertexAttrib4sNV = (PFNGLVERTEXATTRIB4SNVPROC) load(userptr, "glVertexAttrib4sNV");
    context->VertexAttrib4svNV = (PFNGLVERTEXATTRIB4SVNVPROC) load(userptr, "glVertexAttrib4svNV");
    context->VertexAttrib4ubNV = (PFNGLVERTEXATTRIB4UBNVPROC) load(userptr, "glVertexAttrib4ubNV");
    context->VertexAttrib4ubvNV = (PFNGLVERTEXATTRIB4UBVNVPROC) load(userptr, "glVertexAttrib4ubvNV");
    context->VertexAttribPointerNV = (PFNGLVERTEXATTRIBPOINTERNVPROC) load(userptr, "glVertexAttribPointerNV");
    context->VertexAttribs1dvNV = (PFNGLVERTEXATTRIBS1DVNVPROC) load(userptr, "glVertexAttribs1dvNV");
    context->VertexAttribs1fvNV = (PFNGLVERTEXATTRIBS1FVNVPROC) load(userptr, "glVertexAttribs1fvNV");
    context->VertexAttribs1svNV = (PFNGLVERTEXATTRIBS1SVNVPROC) load(userptr, "glVertexAttribs1svNV");
    context->VertexAttribs2dvNV = (PFNGLVERTEXATTRIBS2DVNVPROC) load(userptr, "glVertexAttribs2dvNV");
    context->VertexAttribs2fvNV = (PFNGLVERTEXATTRIBS2FVNVPROC) load(userptr, "glVertexAttribs2fvNV");
    context->VertexAttribs2svNV = (PFNGLVERTEXATTRIBS2SVNVPROC) load(userptr, "glVertexAttribs2svNV");
    context->VertexAttribs3dvNV = (PFNGLVERTEXATTRIBS3DVNVPROC) load(userptr, "glVertexAttribs3dvNV");
    context->VertexAttribs3fvNV = (PFNGLVERTEXATTRIBS3FVNVPROC) load(userptr, "glVertexAttribs3fvNV");
    context->VertexAttribs3svNV = (PFNGLVERTEXATTRIBS3SVNVPROC) load(userptr, "glVertexAttribs3svNV");
    context->VertexAttribs4dvNV = (PFNGLVERTEXATTRIBS4DVNVPROC) load(userptr, "glVertexAttribs4dvNV");
    context->VertexAttribs4fvNV = (PFNGLVERTEXATTRIBS4FVNVPROC) load(userptr, "glVertexAttribs4fvNV");
    context->VertexAttribs4svNV = (PFNGLVERTEXATTRIBS4SVNVPROC) load(userptr, "glVertexAttribs4svNV");
    context->VertexAttribs4ubvNV = (PFNGLVERTEXATTRIBS4UBVNVPROC) load(userptr, "glVertexAttribs4ubvNV");
}
static void glad_gl_load_GL_SGIS_point_parameters(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->SGIS_point_parameters) return;
    context->PointParameterfSGIS = (PFNGLPOINTPARAMETERFSGISPROC) load(userptr, "glPointParameterfSGIS");
    context->PointParameterfvSGIS = (PFNGLPOINTPARAMETERFVSGISPROC) load(userptr, "glPointParameterfvSGIS");
}


static void glad_gl_resolve_aliases(GladGLContext *context) {
    if (context->ActiveTexture == NULL && context->ActiveTextureARB != NULL) context->ActiveTexture = (PFNGLACTIVETEXTUREPROC)context->ActiveTextureARB;
    if (context->ActiveTextureARB == NULL && context->ActiveTexture != NULL) context->ActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)context->ActiveTexture;
    if (context->ArrayElement == NULL && context->ArrayElementEXT != NULL) context->ArrayElement = (PFNGLARRAYELEMENTPROC)context->ArrayElementEXT;
    if (context->ArrayElementEXT == NULL && context->ArrayElement != NULL) context->ArrayElementEXT = (PFNGLARRAYELEMENTEXTPROC)context->ArrayElement;
    if (context->AttachObjectARB == NULL && context->AttachShader != NULL) context->AttachObjectARB = (PFNGLATTACHOBJECTARBPROC)context->AttachShader;
    if (context->AttachShader == NULL && context->AttachObjectARB != NULL) context->AttachShader = (PFNGLATTACHSHADERPROC)context->AttachObjectARB;
    if (context->BeginQuery == NULL && context->BeginQueryARB != NULL) context->BeginQuery = (PFNGLBEGINQUERYPROC)context->BeginQueryARB;
    if (context->BeginQueryARB == NULL && context->BeginQuery != NULL) context->BeginQueryARB = (PFNGLBEGINQUERYARBPROC)context->BeginQuery;
    if (context->BindAttribLocation == NULL && context->BindAttribLocationARB != NULL) context->BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)context->BindAttribLocationARB;
    if (context->BindAttribLocationARB == NULL && context->BindAttribLocation != NULL) context->BindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC)context->BindAttribLocation;
    if (context->BindBuffer == NULL && context->BindBufferARB != NULL) context->BindBuffer = (PFNGLBINDBUFFERPROC)context->BindBufferARB;
    if (context->BindBufferARB == NULL && context->BindBuffer != NULL) context->BindBufferARB = (PFNGLBINDBUFFERARBPROC)context->BindBuffer;
    if (context->BindProgramARB == NULL && context->BindProgramNV != NULL) context->BindProgramARB = (PFNGLBINDPROGRAMARBPROC)context->BindProgramNV;
    if (context->BindProgramNV == NULL && context->BindProgramARB != NULL) context->BindProgramNV = (PFNGLBINDPROGRAMNVPROC)context->BindProgramARB;
    if (context->BindTexture == NULL && context->BindTextureEXT != NULL) context->BindTexture = (PFNGLBINDTEXTUREPROC)context->BindTextureEXT;
    if (context->BindTextureEXT == NULL && context->BindTexture != NULL) context->BindTextureEXT = (PFNGLBINDTEXTUREEXTPROC)context->BindTexture;
    if (context->BlendColor == NULL && context->BlendColorEXT != NULL) context->BlendColor = (PFNGLBLENDCOLORPROC)context->BlendColorEXT;
    if (context->BlendColorEXT == NULL && context->BlendColor != NULL) context->BlendColorEXT = (PFNGLBLENDCOLOREXTPROC)context->BlendColor;
    if (context->BlendEquation == NULL && context->BlendEquationEXT != NULL) context->BlendEquation = (PFNGLBLENDEQUATIONPROC)context->BlendEquationEXT;
    if (context->BlendEquationEXT == NULL && context->BlendEquation != NULL) context->BlendEquationEXT = (PFNGLBLENDEQUATIONEXTPROC)context->BlendEquation;
    if (context->BlendEquationSeparate == NULL && context->BlendEquationSeparateEXT != NULL) context->BlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)context->BlendEquationSeparateEXT;
    if (context->BlendEquationSeparateEXT == NULL && context->BlendEquationSeparate != NULL) context->BlendEquationSeparateEXT = (PFNGLBLENDEQUATIONSEPARATEEXTPROC)context->BlendEquationSeparate;
    if (context->BlendFuncSeparate == NULL && context->BlendFuncSeparateEXT != NULL) context->BlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)context->BlendFuncSeparateEXT;
    if (context->BlendFuncSeparate == NULL && context->BlendFuncSeparateINGR != NULL) context->BlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)context->BlendFuncSeparateINGR;
    if (context->BlendFuncSeparateEXT == NULL && context->BlendFuncSeparate != NULL) context->BlendFuncSeparateEXT = (PFNGLBLENDFUNCSEPARATEEXTPROC)context->BlendFuncSeparate;
    if (context->BlendFuncSeparateEXT == NULL && context->BlendFuncSeparateINGR != NULL) context->BlendFuncSeparateEXT = (PFNGLBLENDFUNCSEPARATEEXTPROC)context->BlendFuncSeparateINGR;
    if (context->BlendFuncSeparateINGR == NULL && context->BlendFuncSeparate != NULL) context->BlendFuncSeparateINGR = (PFNGLBLENDFUNCSEPARATEINGRPROC)context->BlendFuncSeparate;
    if (context->BlendFuncSeparateINGR == NULL && context->BlendFuncSeparateEXT != NULL) context->BlendFuncSeparateINGR = (PFNGLBLENDFUNCSEPARATEINGRPROC)context->BlendFuncSeparateEXT;
    if (context->BlitFramebuffer == NULL && context->BlitFramebufferEXT != NULL) context->BlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)context->BlitFramebufferEXT;
    if (context->BlitFramebufferEXT == NULL && context->BlitFramebuffer != NULL) context->BlitFramebufferEXT = (PFNGLBLITFRAMEBUFFEREXTPROC)context->BlitFramebuffer;
    if (context->BufferData == NULL && context->BufferDataARB != NULL) context->BufferData = (PFNGLBUFFERDATAPROC)context->BufferDataARB;
    if (context->BufferDataARB == NULL && context->BufferData != NULL) context->BufferDataARB = (PFNGLBUFFERDATAARBPROC)context->BufferData;
    if (context->BufferSubData == NULL && context->BufferSubDataARB != NULL) context->BufferSubData = (PFNGLBUFFERSUBDATAPROC)context->BufferSubDataARB;
    if (context->BufferSubDataARB == NULL && context->BufferSubData != NULL) context->BufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)context->BufferSubData;
    if (context->CheckFramebufferStatus == NULL && context->CheckFramebufferStatusEXT != NULL) context->CheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)context->CheckFramebufferStatusEXT;
    if (context->CheckFramebufferStatusEXT == NULL && context->CheckFramebufferStatus != NULL) context->CheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)context->CheckFramebufferStatus;
    if (context->ClientActiveTexture == NULL && context->ClientActiveTextureARB != NULL) context->ClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)context->ClientActiveTextureARB;
    if (context->ClientActiveTextureARB == NULL && context->ClientActiveTexture != NULL) context->ClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC)context->ClientActiveTexture;
    if (context->CompileShader == NULL && context->CompileShaderARB != NULL) context->CompileShader = (PFNGLCOMPILESHADERPROC)context->CompileShaderARB;
    if (context->CompileShaderARB == NULL && context->CompileShader != NULL) context->CompileShaderARB = (PFNGLCOMPILESHADERARBPROC)context->CompileShader;
    if (context->CompressedTexImage1D == NULL && context->CompressedTexImage1DARB != NULL) context->CompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)context->CompressedTexImage1DARB;
    if (context->CompressedTexImage1DARB == NULL && context->CompressedTexImage1D != NULL) context->CompressedTexImage1DARB = (PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)context->CompressedTexImage1D;
    if (context->CompressedTexImage2D == NULL && context->CompressedTexImage2DARB != NULL) context->CompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)context->CompressedTexImage2DARB;
    if (context->CompressedTexImage2DARB == NULL && context->CompressedTexImage2D != NULL) context->CompressedTexImage2DARB = (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)context->CompressedTexImage2D;
    if (context->CompressedTexImage3D == NULL && context->CompressedTexImage3DARB != NULL) context->CompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)context->CompressedTexImage3DARB;
    if (context->CompressedTexImage3DARB == NULL && context->CompressedTexImage3D != NULL) context->CompressedTexImage3DARB = (PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)context->CompressedTexImage3D;
    if (context->CompressedTexSubImage1D == NULL && context->CompressedTexSubImage1DARB != NULL) context->CompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)context->CompressedTexSubImage1DARB;
    if (context->CompressedTexSubImage1DARB == NULL && context->CompressedTexSubImage1D != NULL) context->CompressedTexSubImage1DARB = (PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)context->CompressedTexSubImage1D;
    if (context->CompressedTexSubImage2D == NULL && context->CompressedTexSubImage2DARB != NULL) context->CompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)context->CompressedTexSubImage2DARB;
    if (context->CompressedTexSubImage2DARB == NULL && context->CompressedTexSubImage2D != NULL) context->CompressedTexSubImage2DARB = (PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)context->CompressedTexSubImage2D;
    if (context->CompressedTexSubImage3D == NULL && context->CompressedTexSubImage3DARB != NULL) context->CompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)context->CompressedTexSubImage3DARB;
    if (context->CompressedTexSubImage3DARB == NULL && context->CompressedTexSubImage3D != NULL) context->CompressedTexSubImage3DARB = (PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)context->CompressedTexSubImage3D;
    if (context->CopyTexImage1D == NULL && context->CopyTexImage1DEXT != NULL) context->CopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC)context->CopyTexImage1DEXT;
    if (context->CopyTexImage1DEXT == NULL && context->CopyTexImage1D != NULL) context->CopyTexImage1DEXT = (PFNGLCOPYTEXIMAGE1DEXTPROC)context->CopyTexImage1D;
    if (context->CopyTexImage2D == NULL && context->CopyTexImage2DEXT != NULL) context->CopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC)context->CopyTexImage2DEXT;
    if (context->CopyTexImage2DEXT == NULL && context->CopyTexImage2D != NULL) context->CopyTexImage2DEXT = (PFNGLCOPYTEXIMAGE2DEXTPROC)context->CopyTexImage2D;
    if (context->CopyTexSubImage1D == NULL && context->CopyTexSubImage1DEXT != NULL) context->CopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC)context->CopyTexSubImage1DEXT;
    if (context->CopyTexSubImage1DEXT == NULL && context->CopyTexSubImage1D != NULL) context->CopyTexSubImage1DEXT = (PFNGLCOPYTEXSUBIMAGE1DEXTPROC)context->CopyTexSubImage1D;
    if (context->CopyTexSubImage2D == NULL && context->CopyTexSubImage2DEXT != NULL) context->CopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC)context->CopyTexSubImage2DEXT;
    if (context->CopyTexSubImage2DEXT == NULL && context->CopyTexSubImage2D != NULL) context->CopyTexSubImage2DEXT = (PFNGLCOPYTEXSUBIMAGE2DEXTPROC)context->CopyTexSubImage2D;
    if (context->CopyTexSubImage3D == NULL && context->CopyTexSubImage3DEXT != NULL) context->CopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)context->CopyTexSubImage3DEXT;
    if (context->CopyTexSubImage3DEXT == NULL && context->CopyTexSubImage3D != NULL) context->CopyTexSubImage3DEXT = (PFNGLCOPYTEXSUBIMAGE3DEXTPROC)context->CopyTexSubImage3D;
    if (context->CreateProgram == NULL && context->CreateProgramObjectARB != NULL) context->CreateProgram = (PFNGLCREATEPROGRAMPROC)context->CreateProgramObjectARB;
    if (context->CreateProgramObjectARB == NULL && context->CreateProgram != NULL) context->CreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC)context->CreateProgram;
    if (context->CreateShader == NULL && context->CreateShaderObjectARB != NULL) context->CreateShader = (PFNGLCREATESHADERPROC)context->CreateShaderObjectARB;
    if (context->CreateShaderObjectARB == NULL && context->CreateShader != NULL) context->CreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC)context->CreateShader;
    if (context->DeleteBuffers == NULL && context->DeleteBuffersARB != NULL) context->DeleteBuffers = (PFNGLDELETEBUFFERSPROC)context->DeleteBuffersARB;
    if (context->DeleteBuffersARB == NULL && context->DeleteBuffers != NULL) context->DeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)context->DeleteBuffers;
    if (context->DeleteFramebuffers == NULL && context->DeleteFramebuffersEXT != NULL) context->DeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)context->DeleteFramebuffersEXT;
    if (context->DeleteFramebuffersEXT == NULL && context->DeleteFramebuffers != NULL) context->DeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)context->DeleteFramebuffers;
    if (context->DeleteProgramsARB == NULL && context->DeleteProgramsNV != NULL) context->DeleteProgramsARB = (PFNGLDELETEPROGRAMSARBPROC)context->DeleteProgramsNV;
    if (context->DeleteProgramsNV == NULL && context->DeleteProgramsARB != NULL) context->DeleteProgramsNV = (PFNGLDELETEPROGRAMSNVPROC)context->DeleteProgramsARB;
    if (context->DeleteQueries == NULL && context->DeleteQueriesARB != NULL) context->DeleteQueries = (PFNGLDELETEQUERIESPROC)context->DeleteQueriesARB;
    if (context->DeleteQueriesARB == NULL && context->DeleteQueries != NULL) context->DeleteQueriesARB = (PFNGLDELETEQUERIESARBPROC)context->DeleteQueries;
    if (context->DeleteRenderbuffers == NULL && context->DeleteRenderbuffersEXT != NULL) context->DeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)context->DeleteRenderbuffersEXT;
    if (context->DeleteRenderbuffersEXT == NULL && context->DeleteRenderbuffers != NULL) context->DeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)context->DeleteRenderbuffers;
    if (context->DetachObjectARB == NULL && context->DetachShader != NULL) context->DetachObjectARB = (PFNGLDETACHOBJECTARBPROC)context->DetachShader;
    if (context->DetachShader == NULL && context->DetachObjectARB != NULL) context->DetachShader = (PFNGLDETACHSHADERPROC)context->DetachObjectARB;
    if (context->DisableVertexAttribArray == NULL && context->DisableVertexAttribArrayARB != NULL) context->DisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)context->DisableVertexAttribArrayARB;
    if (context->DisableVertexAttribArrayARB == NULL && context->DisableVertexAttribArray != NULL) context->DisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)context->DisableVertexAttribArray;
    if (context->DrawArrays == NULL && context->DrawArraysEXT != NULL) context->DrawArrays = (PFNGLDRAWARRAYSPROC)context->DrawArraysEXT;
    if (context->DrawArraysEXT == NULL && context->DrawArrays != NULL) context->DrawArraysEXT = (PFNGLDRAWARRAYSEXTPROC)context->DrawArrays;
    if (context->DrawBuffers == NULL && context->DrawBuffersARB != NULL) context->DrawBuffers = (PFNGLDRAWBUFFERSPROC)context->DrawBuffersARB;
    if (context->DrawBuffers == NULL && context->DrawBuffersATI != NULL) context->DrawBuffers = (PFNGLDRAWBUFFERSPROC)context->DrawBuffersATI;
    if (context->DrawBuffersARB == NULL && context->DrawBuffers != NULL) context->DrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC)context->DrawBuffers;
    if (context->DrawBuffersARB == NULL && context->DrawBuffersATI != NULL) context->DrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC)context->DrawBuffersATI;
    if (context->DrawBuffersATI == NULL && context->DrawBuffers != NULL) context->DrawBuffersATI = (PFNGLDRAWBUFFERSATIPROC)context->DrawBuffers;
    if (context->DrawBuffersATI == NULL && context->DrawBuffersARB != NULL) context->DrawBuffersATI = (PFNGLDRAWBUFFERSATIPROC)context->DrawBuffersARB;
    if (context->DrawRangeElements == NULL && context->DrawRangeElementsEXT != NULL) context->DrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)context->DrawRangeElementsEXT;
    if (context->DrawRangeElementsEXT == NULL && context->DrawRangeElements != NULL) context->DrawRangeElementsEXT = (PFNGLDRAWRANGEELEMENTSEXTPROC)context->DrawRangeElements;
    if (context->EnableVertexAttribArray == NULL && context->EnableVertexAttribArrayARB != NULL) context->EnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)context->EnableVertexAttribArrayARB;
    if (context->EnableVertexAttribArrayARB == NULL && context->EnableVertexAttribArray != NULL) context->EnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)context->EnableVertexAttribArray;
    if (context->EndQuery == NULL && context->EndQueryARB != NULL) context->EndQuery = (PFNGLENDQUERYPROC)context->EndQueryARB;
    if (context->EndQueryARB == NULL && context->EndQuery != NULL) context->EndQueryARB = (PFNGLENDQUERYARBPROC)context->EndQuery;
    if (context->FogCoordd == NULL && context->FogCoorddEXT != NULL) context->FogCoordd = (PFNGLFOGCOORDDPROC)context->FogCoorddEXT;
    if (context->FogCoorddEXT == NULL && context->FogCoordd != NULL) context->FogCoorddEXT = (PFNGLFOGCOORDDEXTPROC)context->FogCoordd;
    if (context->FogCoorddv == NULL && context->FogCoorddvEXT != NULL) context->FogCoorddv = (PFNGLFOGCOORDDVPROC)context->FogCoorddvEXT;
    if (context->FogCoorddvEXT == NULL && context->FogCoorddv != NULL) context->FogCoorddvEXT = (PFNGLFOGCOORDDVEXTPROC)context->FogCoorddv;
    if (context->FogCoordf == NULL && context->FogCoordfEXT != NULL) context->FogCoordf = (PFNGLFOGCOORDFPROC)context->FogCoordfEXT;
    if (context->FogCoordfEXT == NULL && context->FogCoordf != NULL) context->FogCoordfEXT = (PFNGLFOGCOORDFEXTPROC)context->FogCoordf;
    if (context->FogCoordfv == NULL && context->FogCoordfvEXT != NULL) context->FogCoordfv = (PFNGLFOGCOORDFVPROC)context->FogCoordfvEXT;
    if (context->FogCoordfvEXT == NULL && context->FogCoordfv != NULL) context->FogCoordfvEXT = (PFNGLFOGCOORDFVEXTPROC)context->FogCoordfv;
    if (context->FogCoordPointer == NULL && context->FogCoordPointerEXT != NULL) context->FogCoordPointer = (PFNGLFOGCOORDPOINTERPROC)context->FogCoordPointerEXT;
    if (context->FogCoordPointerEXT == NULL && context->FogCoordPointer != NULL) context->FogCoordPointerEXT = (PFNGLFOGCOORDPOINTEREXTPROC)context->FogCoordPointer;
    if (context->FramebufferRenderbuffer == NULL && context->FramebufferRenderbufferEXT != NULL) context->FramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)context->FramebufferRenderbufferEXT;
    if (context->FramebufferRenderbufferEXT == NULL && context->FramebufferRenderbuffer != NULL) context->FramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)context->FramebufferRenderbuffer;
    if (context->FramebufferTexture1D == NULL && context->FramebufferTexture1DEXT != NULL) context->FramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)context->FramebufferTexture1DEXT;
    if (context->FramebufferTexture1DEXT == NULL && context->FramebufferTexture1D != NULL) context->FramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)context->FramebufferTexture1D;
    if (context->FramebufferTexture2D == NULL && context->FramebufferTexture2DEXT != NULL) context->FramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)context->FramebufferTexture2DEXT;
    if (context->FramebufferTexture2DEXT == NULL && context->FramebufferTexture2D != NULL) context->FramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)context->FramebufferTexture2D;
    if (context->FramebufferTexture3D == NULL && context->FramebufferTexture3DEXT != NULL) context->FramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)context->FramebufferTexture3DEXT;
    if (context->FramebufferTexture3DEXT == NULL && context->FramebufferTexture3D != NULL) context->FramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)context->FramebufferTexture3D;
    if (context->FramebufferTextureFaceARB == NULL && context->FramebufferTextureFaceEXT != NULL) context->FramebufferTextureFaceARB = (PFNGLFRAMEBUFFERTEXTUREFACEARBPROC)context->FramebufferTextureFaceEXT;
    if (context->FramebufferTextureFaceEXT == NULL && context->FramebufferTextureFaceARB != NULL) context->FramebufferTextureFaceEXT = (PFNGLFRAMEBUFFERTEXTUREFACEEXTPROC)context->FramebufferTextureFaceARB;
    if (context->FramebufferTextureLayer == NULL && context->FramebufferTextureLayerARB != NULL) context->FramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)context->FramebufferTextureLayerARB;
    if (context->FramebufferTextureLayer == NULL && context->FramebufferTextureLayerEXT != NULL) context->FramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)context->FramebufferTextureLayerEXT;
    if (context->FramebufferTextureLayerARB == NULL && context->FramebufferTextureLayer != NULL) context->FramebufferTextureLayerARB = (PFNGLFRAMEBUFFERTEXTURELAYERARBPROC)context->FramebufferTextureLayer;
    if (context->FramebufferTextureLayerARB == NULL && context->FramebufferTextureLayerEXT != NULL) context->FramebufferTextureLayerARB = (PFNGLFRAMEBUFFERTEXTURELAYERARBPROC)context->FramebufferTextureLayerEXT;
    if (context->FramebufferTextureLayerEXT == NULL && context->FramebufferTextureLayer != NULL) context->FramebufferTextureLayerEXT = (PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC)context->FramebufferTextureLayer;
    if (context->FramebufferTextureLayerEXT == NULL && context->FramebufferTextureLayerARB != NULL) context->FramebufferTextureLayerEXT = (PFNGLFRAMEBUFFERTEXTURELAYEREXTPROC)context->FramebufferTextureLayerARB;
    if (context->GenBuffers == NULL && context->GenBuffersARB != NULL) context->GenBuffers = (PFNGLGENBUFFERSPROC)context->GenBuffersARB;
    if (context->GenBuffersARB == NULL && context->GenBuffers != NULL) context->GenBuffersARB = (PFNGLGENBUFFERSARBPROC)context->GenBuffers;
    if (context->GenerateMipmap == NULL && context->GenerateMipmapEXT != NULL) context->GenerateMipmap = (PFNGLGENERATEMIPMAPPROC)context->GenerateMipmapEXT;
    if (context->GenerateMipmapEXT == NULL && context->GenerateMipmap != NULL) context->GenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)context->GenerateMipmap;
    if (context->GenFramebuffers == NULL && context->GenFramebuffersEXT != NULL) context->GenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)context->GenFramebuffersEXT;
    if (context->GenFramebuffersEXT == NULL && context->GenFramebuffers != NULL) context->GenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)context->GenFramebuffers;
    if (context->GenProgramsARB == NULL && context->GenProgramsNV != NULL) context->GenProgramsARB = (PFNGLGENPROGRAMSARBPROC)context->GenProgramsNV;
    if (context->GenProgramsNV == NULL && context->GenProgramsARB != NULL) context->GenProgramsNV = (PFNGLGENPROGRAMSNVPROC)context->GenProgramsARB;
    if (context->GenQueries == NULL && context->GenQueriesARB != NULL) context->GenQueries = (PFNGLGENQUERIESPROC)context->GenQueriesARB;
    if (context->GenQueriesARB == NULL && context->GenQueries != NULL) context->GenQueriesARB = (PFNGLGENQUERIESARBPROC)context->GenQueries;
    if (context->GenRenderbuffers == NULL && context->GenRenderbuffersEXT != NULL) context->GenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)context->GenRenderbuffersEXT;
    if (context->GenRenderbuffersEXT == NULL && context->GenRenderbuffers != NULL) context->GenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)context->GenRenderbuffers;
    if (context->GetActiveAttrib == NULL && context->GetActiveAttribARB != NULL) context->GetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)context->GetActiveAttribARB;
    if (context->GetActiveAttribARB == NULL && context->GetActiveAttrib != NULL) context->GetActiveAttribARB = (PFNGLGETACTIVEATTRIBARBPROC)context->GetActiveAttrib;
    if (context->GetActiveUniform == NULL && context->GetActiveUniformARB != NULL) context->GetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)context->GetActiveUniformARB;
    if (context->GetActiveUniformARB == NULL && context->GetActiveUniform != NULL) context->GetActiveUniformARB = (PFNGLGETACTIVEUNIFORMARBPROC)context->GetActiveUniform;
    if (context->GetAttribLocation == NULL && context->GetAttribLocationARB != NULL) context->GetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)context->GetAttribLocationARB;
    if (context->GetAttribLocationARB == NULL && context->GetAttribLocation != NULL) context->GetAttribLocationARB = (PFNGLGETATTRIBLOCATIONARBPROC)context->GetAttribLocation;
    if (context->GetBufferParameteriv == NULL && context->GetBufferParameterivARB != NULL) context->GetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)context->GetBufferParameterivARB;
    if (context->GetBufferParameterivARB == NULL && context->GetBufferParameteriv != NULL) context->GetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)context->GetBufferParameteriv;
    if (context->GetBufferPointerv == NULL && context->GetBufferPointervARB != NULL) context->GetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)context->GetBufferPointervARB;
    if (context->GetBufferPointervARB == NULL && context->GetBufferPointerv != NULL) context->GetBufferPointervARB = (PFNGLGETBUFFERPOINTERVARBPROC)context->GetBufferPointerv;
    if (context->GetBufferSubData == NULL && context->GetBufferSubDataARB != NULL) context->GetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)context->GetBufferSubDataARB;
    if (context->GetBufferSubDataARB == NULL && context->GetBufferSubData != NULL) context->GetBufferSubDataARB = (PFNGLGETBUFFERSUBDATAARBPROC)context->GetBufferSubData;
    if (context->GetCompressedTexImage == NULL && context->GetCompressedTexImageARB != NULL) context->GetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)context->GetCompressedTexImageARB;
    if (context->GetCompressedTexImageARB == NULL && context->GetCompressedTexImage != NULL) context->GetCompressedTexImageARB = (PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)context->GetCompressedTexImage;
    if (context->GetFramebufferAttachmentParameteriv == NULL && context->GetFramebufferAttachmentParameterivEXT != NULL) context->GetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)context->GetFramebufferAttachmentParameterivEXT;
    if (context->GetFramebufferAttachmentParameterivEXT == NULL && context->GetFramebufferAttachmentParameteriv != NULL) context->GetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)context->GetFramebufferAttachmentParameteriv;
    if (context->GetPointerv == NULL && context->GetPointervEXT != NULL) context->GetPointerv = (PFNGLGETPOINTERVPROC)context->GetPointervEXT;
    if (context->GetPointervEXT == NULL && context->GetPointerv != NULL) context->GetPointervEXT = (PFNGLGETPOINTERVEXTPROC)context->GetPointerv;
    if (context->GetQueryiv == NULL && context->GetQueryivARB != NULL) context->GetQueryiv = (PFNGLGETQUERYIVPROC)context->GetQueryivARB;
    if (context->GetQueryivARB == NULL && context->GetQueryiv != NULL) context->GetQueryivARB = (PFNGLGETQUERYIVARBPROC)context->GetQueryiv;
    if (context->GetQueryObjectiv == NULL && context->GetQueryObjectivARB != NULL) context->GetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)context->GetQueryObjectivARB;
    if (context->GetQueryObjectivARB == NULL && context->GetQueryObjectiv != NULL) context->GetQueryObjectivARB = (PFNGLGETQUERYOBJECTIVARBPROC)context->GetQueryObjectiv;
    if (context->GetQueryObjectuiv == NULL && context->GetQueryObjectuivARB != NULL) context->GetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)context->GetQueryObjectuivARB;
    if (context->GetQueryObjectuivARB == NULL && context->GetQueryObjectuiv != NULL) context->GetQueryObjectuivARB = (PFNGLGETQUERYOBJECTUIVARBPROC)context->GetQueryObjectuiv;
    if (context->GetRenderbufferParameteriv == NULL && context->GetRenderbufferParameterivEXT != NULL) context->GetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)context->GetRenderbufferParameterivEXT;
    if (context->GetRenderbufferParameterivEXT == NULL && context->GetRenderbufferParameteriv != NULL) context->GetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)context->GetRenderbufferParameteriv;
    if (context->GetShaderSource == NULL && context->GetShaderSourceARB != NULL) context->GetShaderSource = (PFNGLGETSHADERSOURCEPROC)context->GetShaderSourceARB;
    if (context->GetShaderSourceARB == NULL && context->GetShaderSource != NULL) context->GetShaderSourceARB = (PFNGLGETSHADERSOURCEARBPROC)context->GetShaderSource;
    if (context->GetUniformfv == NULL && context->GetUniformfvARB != NULL) context->GetUniformfv = (PFNGLGETUNIFORMFVPROC)context->GetUniformfvARB;
    if (context->GetUniformfvARB == NULL && context->GetUniformfv != NULL) context->GetUniformfvARB = (PFNGLGETUNIFORMFVARBPROC)context->GetUniformfv;
    if (context->GetUniformiv == NULL && context->GetUniformivARB != NULL) context->GetUniformiv = (PFNGLGETUNIFORMIVPROC)context->GetUniformivARB;
    if (context->GetUniformivARB == NULL && context->GetUniformiv != NULL) context->GetUniformivARB = (PFNGLGETUNIFORMIVARBPROC)context->GetUniformiv;
    if (context->GetUniformLocation == NULL && context->GetUniformLocationARB != NULL) context->GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)context->GetUniformLocationARB;
    if (context->GetUniformLocationARB == NULL && context->GetUniformLocation != NULL) context->GetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)context->GetUniformLocation;
    if (context->GetVertexAttribdv == NULL && context->GetVertexAttribdvARB != NULL) context->GetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)context->GetVertexAttribdvARB;
    if (context->GetVertexAttribdv == NULL && context->GetVertexAttribdvNV != NULL) context->GetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)context->GetVertexAttribdvNV;
    if (context->GetVertexAttribdvARB == NULL && context->GetVertexAttribdv != NULL) context->GetVertexAttribdvARB = (PFNGLGETVERTEXATTRIBDVARBPROC)context->GetVertexAttribdv;
    if (context->GetVertexAttribdvARB == NULL && context->GetVertexAttribdvNV != NULL) context->GetVertexAttribdvARB = (PFNGLGETVERTEXATTRIBDVARBPROC)context->GetVertexAttribdvNV;
    if (context->GetVertexAttribdvNV == NULL && context->GetVertexAttribdv != NULL) context->GetVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC)context->GetVertexAttribdv;
    if (context->GetVertexAttribdvNV == NULL && context->GetVertexAttribdvARB != NULL) context->GetVertexAttribdvNV = (PFNGLGETVERTEXATTRIBDVNVPROC)context->GetVertexAttribdvARB;
    if (context->GetVertexAttribfv == NULL && context->GetVertexAttribfvARB != NULL) context->GetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)context->GetVertexAttribfvARB;
    if (context->GetVertexAttribfv == NULL && context->GetVertexAttribfvNV != NULL) context->GetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)context->GetVertexAttribfvNV;
    if (context->GetVertexAttribfvARB == NULL && context->GetVertexAttribfv != NULL) context->GetVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC)context->GetVertexAttribfv;
    if (context->GetVertexAttribfvARB == NULL && context->GetVertexAttribfvNV != NULL) context->GetVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC)context->GetVertexAttribfvNV;
    if (context->GetVertexAttribfvNV == NULL && context->GetVertexAttribfv != NULL) context->GetVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC)context->GetVertexAttribfv;
    if (context->GetVertexAttribfvNV == NULL && context->GetVertexAttribfvARB != NULL) context->GetVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC)context->GetVertexAttribfvARB;
    if (context->GetVertexAttribiv == NULL && context->GetVertexAttribivARB != NULL) context->GetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)context->GetVertexAttribivARB;
    if (context->GetVertexAttribiv == NULL && context->GetVertexAttribivNV != NULL) context->GetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)context->GetVertexAttribivNV;
    if (context->GetVertexAttribivARB == NULL && context->GetVertexAttribiv != NULL) context->GetVertexAttribivARB = (PFNGLGETVERTEXATTRIBIVARBPROC)context->GetVertexAttribiv;
    if (context->GetVertexAttribivARB == NULL && context->GetVertexAttribivNV != NULL) context->GetVertexAttribivARB = (PFNGLGETVERTEXATTRIBIVARBPROC)context->GetVertexAttribivNV;
    if (context->GetVertexAttribivNV == NULL && context->GetVertexAttribiv != NULL) context->GetVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC)context->GetVertexAttribiv;
    if (context->GetVertexAttribivNV == NULL && context->GetVertexAttribivARB != NULL) context->GetVertexAttribivNV = (PFNGLGETVERTEXATTRIBIVNVPROC)context->GetVertexAttribivARB;
    if (context->GetVertexAttribPointerv == NULL && context->GetVertexAttribPointervARB != NULL) context->GetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)context->GetVertexAttribPointervARB;
    if (context->GetVertexAttribPointerv == NULL && context->GetVertexAttribPointervNV != NULL) context->GetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)context->GetVertexAttribPointervNV;
    if (context->GetVertexAttribPointervARB == NULL && context->GetVertexAttribPointerv != NULL) context->GetVertexAttribPointervARB = (PFNGLGETVERTEXATTRIBPOINTERVARBPROC)context->GetVertexAttribPointerv;
    if (context->GetVertexAttribPointervARB == NULL && context->GetVertexAttribPointervNV != NULL) context->GetVertexAttribPointervARB = (PFNGLGETVERTEXATTRIBPOINTERVARBPROC)context->GetVertexAttribPointervNV;
    if (context->GetVertexAttribPointervNV == NULL && context->GetVertexAttribPointerv != NULL) context->GetVertexAttribPointervNV = (PFNGLGETVERTEXATTRIBPOINTERVNVPROC)context->GetVertexAttribPointerv;
    if (context->GetVertexAttribPointervNV == NULL && context->GetVertexAttribPointervARB != NULL) context->GetVertexAttribPointervNV = (PFNGLGETVERTEXATTRIBPOINTERVNVPROC)context->GetVertexAttribPointervARB;
    if (context->IsBuffer == NULL && context->IsBufferARB != NULL) context->IsBuffer = (PFNGLISBUFFERPROC)context->IsBufferARB;
    if (context->IsBufferARB == NULL && context->IsBuffer != NULL) context->IsBufferARB = (PFNGLISBUFFERARBPROC)context->IsBuffer;
    if (context->IsFramebuffer == NULL && context->IsFramebufferEXT != NULL) context->IsFramebuffer = (PFNGLISFRAMEBUFFERPROC)context->IsFramebufferEXT;
    if (context->IsFramebufferEXT == NULL && context->IsFramebuffer != NULL) context->IsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)context->IsFramebuffer;
    if (context->IsProgramARB == NULL && context->IsProgramNV != NULL) context->IsProgramARB = (PFNGLISPROGRAMARBPROC)context->IsProgramNV;
    if (context->IsProgramNV == NULL && context->IsProgramARB != NULL) context->IsProgramNV = (PFNGLISPROGRAMNVPROC)context->IsProgramARB;
    if (context->IsQuery == NULL && context->IsQueryARB != NULL) context->IsQuery = (PFNGLISQUERYPROC)context->IsQueryARB;
    if (context->IsQueryARB == NULL && context->IsQuery != NULL) context->IsQueryARB = (PFNGLISQUERYARBPROC)context->IsQuery;
    if (context->IsRenderbuffer == NULL && context->IsRenderbufferEXT != NULL) context->IsRenderbuffer = (PFNGLISRENDERBUFFERPROC)context->IsRenderbufferEXT;
    if (context->IsRenderbufferEXT == NULL && context->IsRenderbuffer != NULL) context->IsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)context->IsRenderbuffer;
    if (context->LinkProgram == NULL && context->LinkProgramARB != NULL) context->LinkProgram = (PFNGLLINKPROGRAMPROC)context->LinkProgramARB;
    if (context->LinkProgramARB == NULL && context->LinkProgram != NULL) context->LinkProgramARB = (PFNGLLINKPROGRAMARBPROC)context->LinkProgram;
    if (context->LoadTransposeMatrixd == NULL && context->LoadTransposeMatrixdARB != NULL) context->LoadTransposeMatrixd = (PFNGLLOADTRANSPOSEMATRIXDPROC)context->LoadTransposeMatrixdARB;
    if (context->LoadTransposeMatrixdARB == NULL && context->LoadTransposeMatrixd != NULL) context->LoadTransposeMatrixdARB = (PFNGLLOADTRANSPOSEMATRIXDARBPROC)context->LoadTransposeMatrixd;
    if (context->LoadTransposeMatrixf == NULL && context->LoadTransposeMatrixfARB != NULL) context->LoadTransposeMatrixf = (PFNGLLOADTRANSPOSEMATRIXFPROC)context->LoadTransposeMatrixfARB;
    if (context->LoadTransposeMatrixfARB == NULL && context->LoadTransposeMatrixf != NULL) context->LoadTransposeMatrixfARB = (PFNGLLOADTRANSPOSEMATRIXFARBPROC)context->LoadTransposeMatrixf;
    if (context->MapBuffer == NULL && context->MapBufferARB != NULL) context->MapBuffer = (PFNGLMAPBUFFERPROC)context->MapBufferARB;
    if (context->MapBufferARB == NULL && context->MapBuffer != NULL) context->MapBufferARB = (PFNGLMAPBUFFERARBPROC)context->MapBuffer;
    if (context->MultiDrawArrays == NULL && context->MultiDrawArraysEXT != NULL) context->MultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)context->MultiDrawArraysEXT;
    if (context->MultiDrawArraysEXT == NULL && context->MultiDrawArrays != NULL) context->MultiDrawArraysEXT = (PFNGLMULTIDRAWARRAYSEXTPROC)context->MultiDrawArrays;
    if (context->MultiDrawElements == NULL && context->MultiDrawElementsEXT != NULL) context->MultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)context->MultiDrawElementsEXT;
    if (context->MultiDrawElementsEXT == NULL && context->MultiDrawElements != NULL) context->MultiDrawElementsEXT = (PFNGLMULTIDRAWELEMENTSEXTPROC)context->MultiDrawElements;
    if (context->MultiTexCoord1d == NULL && context->MultiTexCoord1dARB != NULL) context->MultiTexCoord1d = (PFNGLMULTITEXCOORD1DPROC)context->MultiTexCoord1dARB;
    if (context->MultiTexCoord1dARB == NULL && context->MultiTexCoord1d != NULL) context->MultiTexCoord1dARB = (PFNGLMULTITEXCOORD1DARBPROC)context->MultiTexCoord1d;
    if (context->MultiTexCoord1dv == NULL && context->MultiTexCoord1dvARB != NULL) context->MultiTexCoord1dv = (PFNGLMULTITEXCOORD1DVPROC)context->MultiTexCoord1dvARB;
    if (context->MultiTexCoord1dvARB == NULL && context->MultiTexCoord1dv != NULL) context->MultiTexCoord1dvARB = (PFNGLMULTITEXCOORD1DVARBPROC)context->MultiTexCoord1dv;
    if (context->MultiTexCoord1f == NULL && context->MultiTexCoord1fARB != NULL) context->MultiTexCoord1f = (PFNGLMULTITEXCOORD1FPROC)context->MultiTexCoord1fARB;
    if (context->MultiTexCoord1fARB == NULL && context->MultiTexCoord1f != NULL) context->MultiTexCoord1fARB = (PFNGLMULTITEXCOORD1FARBPROC)context->MultiTexCoord1f;
    if (context->MultiTexCoord1fv == NULL && context->MultiTexCoord1fvARB != NULL) context->MultiTexCoord1fv = (PFNGLMULTITEXCOORD1FVPROC)context->MultiTexCoord1fvARB;
    if (context->MultiTexCoord1fvARB == NULL && context->MultiTexCoord1fv != NULL) context->MultiTexCoord1fvARB = (PFNGLMULTITEXCOORD1FVARBPROC)context->MultiTexCoord1fv;
    if (context->MultiTexCoord1i == NULL && context->MultiTexCoord1iARB != NULL) context->MultiTexCoord1i = (PFNGLMULTITEXCOORD1IPROC)context->MultiTexCoord1iARB;
    if (context->MultiTexCoord1iARB == NULL && context->MultiTexCoord1i != NULL) context->MultiTexCoord1iARB = (PFNGLMULTITEXCOORD1IARBPROC)context->MultiTexCoord1i;
    if (context->MultiTexCoord1iv == NULL && context->MultiTexCoord1ivARB != NULL) context->MultiTexCoord1iv = (PFNGLMULTITEXCOORD1IVPROC)context->MultiTexCoord1ivARB;
    if (context->MultiTexCoord1ivARB == NULL && context->MultiTexCoord1iv != NULL) context->MultiTexCoord1ivARB = (PFNGLMULTITEXCOORD1IVARBPROC)context->MultiTexCoord1iv;
    if (context->MultiTexCoord1s == NULL && context->MultiTexCoord1sARB != NULL) context->MultiTexCoord1s = (PFNGLMULTITEXCOORD1SPROC)context->MultiTexCoord1sARB;
    if (context->MultiTexCoord1sARB == NULL && context->MultiTexCoord1s != NULL) context->MultiTexCoord1sARB = (PFNGLMULTITEXCOORD1SARBPROC)context->MultiTexCoord1s;
    if (context->MultiTexCoord1sv == NULL && context->MultiTexCoord1svARB != NULL) context->MultiTexCoord1sv = (PFNGLMULTITEXCOORD1SVPROC)context->MultiTexCoord1svARB;
    if (context->MultiTexCoord1svARB == NULL && context->MultiTexCoord1sv != NULL) context->MultiTexCoord1svARB = (PFNGLMULTITEXCOORD1SVARBPROC)context->MultiTexCoord1sv;
    if (context->MultiTexCoord2d == NULL && context->MultiTexCoord2dARB != NULL) context->MultiTexCoord2d = (PFNGLMULTITEXCOORD2DPROC)context->MultiTexCoord2dARB;
    if (context->MultiTexCoord2dARB == NULL && context->MultiTexCoord2d != NULL) context->MultiTexCoord2dARB = (PFNGLMULTITEXCOORD2DARBPROC)context->MultiTexCoord2d;
    if (context->MultiTexCoord2dv == NULL && context->MultiTexCoord2dvARB != NULL) context->MultiTexCoord2dv = (PFNGLMULTITEXCOORD2DVPROC)context->MultiTexCoord2dvARB;
    if (context->MultiTexCoord2dvARB == NULL && context->MultiTexCoord2dv != NULL) context->MultiTexCoord2dvARB = (PFNGLMULTITEXCOORD2DVARBPROC)context->MultiTexCoord2dv;
    if (context->MultiTexCoord2f == NULL && context->MultiTexCoord2fARB != NULL) context->MultiTexCoord2f = (PFNGLMULTITEXCOORD2FPROC)context->MultiTexCoord2fARB;
    if (context->MultiTexCoord2fARB == NULL && context->MultiTexCoord2f != NULL) context->MultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)context->MultiTexCoord2f;
    if (context->MultiTexCoord2fv == NULL && context->MultiTexCoord2fvARB != NULL) context->MultiTexCoord2fv = (PFNGLMULTITEXCOORD2FVPROC)context->MultiTexCoord2fvARB;
    if (context->MultiTexCoord2fvARB == NULL && context->MultiTexCoord2fv != NULL) context->MultiTexCoord2fvARB = (PFNGLMULTITEXCOORD2FVARBPROC)context->MultiTexCoord2fv;
    if (context->MultiTexCoord2i == NULL && context->MultiTexCoord2iARB != NULL) context->MultiTexCoord2i = (PFNGLMULTITEXCOORD2IPROC)context->MultiTexCoord2iARB;
    if (context->MultiTexCoord2iARB == NULL && context->MultiTexCoord2i != NULL) context->MultiTexCoord2iARB = (PFNGLMULTITEXCOORD2IARBPROC)context->MultiTexCoord2i;
    if (context->MultiTexCoord2iv == NULL && context->MultiTexCoord2ivARB != NULL) context->MultiTexCoord2iv = (PFNGLMULTITEXCOORD2IVPROC)context->MultiTexCoord2ivARB;
    if (context->MultiTexCoord2ivARB == NULL && context->MultiTexCoord2iv != NULL) context->MultiTexCoord2ivARB = (PFNGLMULTITEXCOORD2IVARBPROC)context->MultiTexCoord2iv;
    if (context->MultiTexCoord2s == NULL && context->MultiTexCoord2sARB != NULL) context->MultiTexCoord2s = (PFNGLMULTITEXCOORD2SPROC)context->MultiTexCoord2sARB;
    if (context->MultiTexCoord2sARB == NULL && context->MultiTexCoord2s != NULL) context->MultiTexCoord2sARB = (PFNGLMULTITEXCOORD2SARBPROC)context->MultiTexCoord2s;
    if (context->MultiTexCoord2sv == NULL && context->MultiTexCoord2svARB != NULL) context->MultiTexCoord2sv = (PFNGLMULTITEXCOORD2SVPROC)context->MultiTexCoord2svARB;
    if (context->MultiTexCoord2svARB == NULL && context->MultiTexCoord2sv != NULL) context->MultiTexCoord2svARB = (PFNGLMULTITEXCOORD2SVARBPROC)context->MultiTexCoord2sv;
    if (context->MultiTexCoord3d == NULL && context->MultiTexCoord3dARB != NULL) context->MultiTexCoord3d = (PFNGLMULTITEXCOORD3DPROC)context->MultiTexCoord3dARB;
    if (context->MultiTexCoord3dARB == NULL && context->MultiTexCoord3d != NULL) context->MultiTexCoord3dARB = (PFNGLMULTITEXCOORD3DARBPROC)context->MultiTexCoord3d;
    if (context->MultiTexCoord3dv == NULL && context->MultiTexCoord3dvARB != NULL) context->MultiTexCoord3dv = (PFNGLMULTITEXCOORD3DVPROC)context->MultiTexCoord3dvARB;
    if (context->MultiTexCoord3dvARB == NULL && context->MultiTexCoord3dv != NULL) context->MultiTexCoord3dvARB = (PFNGLMULTITEXCOORD3DVARBPROC)context->MultiTexCoord3dv;
    if (context->MultiTexCoord3f == NULL && context->MultiTexCoord3fARB != NULL) context->MultiTexCoord3f = (PFNGLMULTITEXCOORD3FPROC)context->MultiTexCoord3fARB;
    if (context->MultiTexCoord3fARB == NULL && context->MultiTexCoord3f != NULL) context->MultiTexCoord3fARB = (PFNGLMULTITEXCOORD3FARBPROC)context->MultiTexCoord3f;
    if (context->MultiTexCoord3fv == NULL && context->MultiTexCoord3fvARB != NULL) context->MultiTexCoord3fv = (PFNGLMULTITEXCOORD3FVPROC)context->MultiTexCoord3fvARB;
    if (context->MultiTexCoord3fvARB == NULL && context->MultiTexCoord3fv != NULL) context->MultiTexCoord3fvARB = (PFNGLMULTITEXCOORD3FVARBPROC)context->MultiTexCoord3fv;
    if (context->MultiTexCoord3i == NULL && context->MultiTexCoord3iARB != NULL) context->MultiTexCoord3i = (PFNGLMULTITEXCOORD3IPROC)context->MultiTexCoord3iARB;
    if (context->MultiTexCoord3iARB == NULL && context->MultiTexCoord3i != NULL) context->MultiTexCoord3iARB = (PFNGLMULTITEXCOORD3IARBPROC)context->MultiTexCoord3i;
    if (context->MultiTexCoord3iv == NULL && context->MultiTexCoord3ivARB != NULL) context->MultiTexCoord3iv = (PFNGLMULTITEXCOORD3IVPROC)context->MultiTexCoord3ivARB;
    if (context->MultiTexCoord3ivARB == NULL && context->MultiTexCoord3iv != NULL) context->MultiTexCoord3ivARB = (PFNGLMULTITEXCOORD3IVARBPROC)context->MultiTexCoord3iv;
    if (context->MultiTexCoord3s == NULL && context->MultiTexCoord3sARB != NULL) context->MultiTexCoord3s = (PFNGLMULTITEXCOORD3SPROC)context->MultiTexCoord3sARB;
    if (context->MultiTexCoord3sARB == NULL && context->MultiTexCoord3s != NULL) context->MultiTexCoord3sARB = (PFNGLMULTITEXCOORD3SARBPROC)context->MultiTexCoord3s;
    if (context->MultiTexCoord3sv == NULL && context->MultiTexCoord3svARB != NULL) context->MultiTexCoord3sv = (PFNGLMULTITEXCOORD3SVPROC)context->MultiTexCoord3svARB;
    if (context->MultiTexCoord3svARB == NULL && context->MultiTexCoord3sv != NULL) context->MultiTexCoord3svARB = (PFNGLMULTITEXCOORD3SVARBPROC)context->MultiTexCoord3sv;
    if (context->MultiTexCoord4d == NULL && context->MultiTexCoord4dARB != NULL) context->MultiTexCoord4d = (PFNGLMULTITEXCOORD4DPROC)context->MultiTexCoord4dARB;
    if (context->MultiTexCoord4dARB == NULL && context->MultiTexCoord4d != NULL) context->MultiTexCoord4dARB = (PFNGLMULTITEXCOORD4DARBPROC)context->MultiTexCoord4d;
    if (context->MultiTexCoord4dv == NULL && context->MultiTexCoord4dvARB != NULL) context->MultiTexCoord4dv = (PFNGLMULTITEXCOORD4DVPROC)context->MultiTexCoord4dvARB;
    if (context->MultiTexCoord4dvARB == NULL && context->MultiTexCoord4dv != NULL) context->MultiTexCoord4dvARB = (PFNGLMULTITEXCOORD4DVARBPROC)context->MultiTexCoord4dv;
    if (context->MultiTexCoord4f == NULL && context->MultiTexCoord4fARB != NULL) context->MultiTexCoord4f = (PFNGLMULTITEXCOORD4FPROC)context->MultiTexCoord4fARB;
    if (context->MultiTexCoord4fARB == NULL && context->MultiTexCoord4f != NULL) context->MultiTexCoord4fARB = (PFNGLMULTITEXCOORD4FARBPROC)context->MultiTexCoord4f;
    if (context->MultiTexCoord4fv == NULL && context->MultiTexCoord4fvARB != NULL) context->MultiTexCoord4fv = (PFNGLMULTITEXCOORD4FVPROC)context->MultiTexCoord4fvARB;
    if (context->MultiTexCoord4fvARB == NULL && context->MultiTexCoord4fv != NULL) context->MultiTexCoord4fvARB = (PFNGLMULTITEXCOORD4FVARBPROC)context->MultiTexCoord4fv;
    if (context->MultiTexCoord4i == NULL && context->MultiTexCoord4iARB != NULL) context->MultiTexCoord4i = (PFNGLMULTITEXCOORD4IPROC)context->MultiTexCoord4iARB;
    if (context->MultiTexCoord4iARB == NULL && context->MultiTexCoord4i != NULL) context->MultiTexCoord4iARB = (PFNGLMULTITEXCOORD4IARBPROC)context->MultiTexCoord4i;
    if (context->MultiTexCoord4iv == NULL && context->MultiTexCoord4ivARB != NULL) context->MultiTexCoord4iv = (PFNGLMULTITEXCOORD4IVPROC)context->MultiTexCoord4ivARB;
    if (context->MultiTexCoord4ivARB == NULL && context->MultiTexCoord4iv != NULL) context->MultiTexCoord4ivARB = (PFNGLMULTITEXCOORD4IVARBPROC)context->MultiTexCoord4iv;
    if (context->MultiTexCoord4s == NULL && context->MultiTexCoord4sARB != NULL) context->MultiTexCoord4s = (PFNGLMULTITEXCOORD4SPROC)context->MultiTexCoord4sARB;
    if (context->MultiTexCoord4sARB == NULL && context->MultiTexCoord4s != NULL) context->MultiTexCoord4sARB = (PFNGLMULTITEXCOORD4SARBPROC)context->MultiTexCoord4s;
    if (context->MultiTexCoord4sv == NULL && context->MultiTexCoord4svARB != NULL) context->MultiTexCoord4sv = (PFNGLMULTITEXCOORD4SVPROC)context->MultiTexCoord4svARB;
    if (context->MultiTexCoord4svARB == NULL && context->MultiTexCoord4sv != NULL) context->MultiTexCoord4svARB = (PFNGLMULTITEXCOORD4SVARBPROC)context->MultiTexCoord4sv;
    if (context->MultTransposeMatrixd == NULL && context->MultTransposeMatrixdARB != NULL) context->MultTransposeMatrixd = (PFNGLMULTTRANSPOSEMATRIXDPROC)context->MultTransposeMatrixdARB;
    if (context->MultTransposeMatrixdARB == NULL && context->MultTransposeMatrixd != NULL) context->MultTransposeMatrixdARB = (PFNGLMULTTRANSPOSEMATRIXDARBPROC)context->MultTransposeMatrixd;
    if (context->MultTransposeMatrixf == NULL && context->MultTransposeMatrixfARB != NULL) context->MultTransposeMatrixf = (PFNGLMULTTRANSPOSEMATRIXFPROC)context->MultTransposeMatrixfARB;
    if (context->MultTransposeMatrixfARB == NULL && context->MultTransposeMatrixf != NULL) context->MultTransposeMatrixfARB = (PFNGLMULTTRANSPOSEMATRIXFARBPROC)context->MultTransposeMatrixf;
    if (context->PointParameterf == NULL && context->PointParameterfARB != NULL) context->PointParameterf = (PFNGLPOINTPARAMETERFPROC)context->PointParameterfARB;
    if (context->PointParameterf == NULL && context->PointParameterfEXT != NULL) context->PointParameterf = (PFNGLPOINTPARAMETERFPROC)context->PointParameterfEXT;
    if (context->PointParameterf == NULL && context->PointParameterfSGIS != NULL) context->PointParameterf = (PFNGLPOINTPARAMETERFPROC)context->PointParameterfSGIS;
    if (context->PointParameterfARB == NULL && context->PointParameterf != NULL) context->PointParameterfARB = (PFNGLPOINTPARAMETERFARBPROC)context->PointParameterf;
    if (context->PointParameterfARB == NULL && context->PointParameterfEXT != NULL) context->PointParameterfARB = (PFNGLPOINTPARAMETERFARBPROC)context->PointParameterfEXT;
    if (context->PointParameterfARB == NULL && context->PointParameterfSGIS != NULL) context->PointParameterfARB = (PFNGLPOINTPARAMETERFARBPROC)context->PointParameterfSGIS;
    if (context->PointParameterfEXT == NULL && context->PointParameterf != NULL) context->PointParameterfEXT = (PFNGLPOINTPARAMETERFEXTPROC)context->PointParameterf;
    if (context->PointParameterfEXT == NULL && context->PointParameterfARB != NULL) context->PointParameterfEXT = (PFNGLPOINTPARAMETERFEXTPROC)context->PointParameterfARB;
    if (context->PointParameterfEXT == NULL && context->PointParameterfSGIS != NULL) context->PointParameterfEXT = (PFNGLPOINTPARAMETERFEXTPROC)context->PointParameterfSGIS;
    if (context->PointParameterfSGIS == NULL && context->PointParameterf != NULL) context->PointParameterfSGIS = (PFNGLPOINTPARAMETERFSGISPROC)context->PointParameterf;
    if (context->PointParameterfSGIS == NULL && context->PointParameterfARB != NULL) context->PointParameterfSGIS = (PFNGLPOINTPARAMETERFSGISPROC)context->PointParameterfARB;
    if (context->PointParameterfSGIS == NULL && context->PointParameterfEXT != NULL) context->PointParameterfSGIS = (PFNGLPOINTPARAMETERFSGISPROC)context->PointParameterfEXT;
    if (context->PointParameterfv == NULL && context->PointParameterfvARB != NULL) context->PointParameterfv = (PFNGLPOINTPARAMETERFVPROC)context->PointParameterfvARB;
    if (context->PointParameterfv == NULL && context->PointParameterfvEXT != NULL) context->PointParameterfv = (PFNGLPOINTPARAMETERFVPROC)context->PointParameterfvEXT;
    if (context->PointParameterfv == NULL && context->PointParameterfvSGIS != NULL) context->PointParameterfv = (PFNGLPOINTPARAMETERFVPROC)context->PointParameterfvSGIS;
    if (context->PointParameterfvARB == NULL && context->PointParameterfv != NULL) context->PointParameterfvARB = (PFNGLPOINTPARAMETERFVARBPROC)context->PointParameterfv;
    if (context->PointParameterfvARB == NULL && context->PointParameterfvEXT != NULL) context->PointParameterfvARB = (PFNGLPOINTPARAMETERFVARBPROC)context->PointParameterfvEXT;
    if (context->PointParameterfvARB == NULL && context->PointParameterfvSGIS != NULL) context->PointParameterfvARB = (PFNGLPOINTPARAMETERFVARBPROC)context->PointParameterfvSGIS;
    if (context->PointParameterfvEXT == NULL && context->PointParameterfv != NULL) context->PointParameterfvEXT = (PFNGLPOINTPARAMETERFVEXTPROC)context->PointParameterfv;
    if (context->PointParameterfvEXT == NULL && context->PointParameterfvARB != NULL) context->PointParameterfvEXT = (PFNGLPOINTPARAMETERFVEXTPROC)context->PointParameterfvARB;
    if (context->PointParameterfvEXT == NULL && context->PointParameterfvSGIS != NULL) context->PointParameterfvEXT = (PFNGLPOINTPARAMETERFVEXTPROC)context->PointParameterfvSGIS;
    if (context->PointParameterfvSGIS == NULL && context->PointParameterfv != NULL) context->PointParameterfvSGIS = (PFNGLPOINTPARAMETERFVSGISPROC)context->PointParameterfv;
    if (context->PointParameterfvSGIS == NULL && context->PointParameterfvARB != NULL) context->PointParameterfvSGIS = (PFNGLPOINTPARAMETERFVSGISPROC)context->PointParameterfvARB;
    if (context->PointParameterfvSGIS == NULL && context->PointParameterfvEXT != NULL) context->PointParameterfvSGIS = (PFNGLPOINTPARAMETERFVSGISPROC)context->PointParameterfvEXT;
    if (context->PointParameteri == NULL && context->PointParameteriNV != NULL) context->PointParameteri = (PFNGLPOINTPARAMETERIPROC)context->PointParameteriNV;
    if (context->PointParameteriNV == NULL && context->PointParameteri != NULL) context->PointParameteriNV = (PFNGLPOINTPARAMETERINVPROC)context->PointParameteri;
    if (context->PointParameteriv == NULL && context->PointParameterivNV != NULL) context->PointParameteriv = (PFNGLPOINTPARAMETERIVPROC)context->PointParameterivNV;
    if (context->PointParameterivNV == NULL && context->PointParameteriv != NULL) context->PointParameterivNV = (PFNGLPOINTPARAMETERIVNVPROC)context->PointParameteriv;
    if (context->PrioritizeTextures == NULL && context->PrioritizeTexturesEXT != NULL) context->PrioritizeTextures = (PFNGLPRIORITIZETEXTURESPROC)context->PrioritizeTexturesEXT;
    if (context->PrioritizeTexturesEXT == NULL && context->PrioritizeTextures != NULL) context->PrioritizeTexturesEXT = (PFNGLPRIORITIZETEXTURESEXTPROC)context->PrioritizeTextures;
    if (context->RenderbufferStorage == NULL && context->RenderbufferStorageEXT != NULL) context->RenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)context->RenderbufferStorageEXT;
    if (context->RenderbufferStorageEXT == NULL && context->RenderbufferStorage != NULL) context->RenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)context->RenderbufferStorage;
    if (context->RenderbufferStorageMultisample == NULL && context->RenderbufferStorageMultisampleEXT != NULL) context->RenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)context->RenderbufferStorageMultisampleEXT;
    if (context->RenderbufferStorageMultisampleEXT == NULL && context->RenderbufferStorageMultisample != NULL) context->RenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)context->RenderbufferStorageMultisample;
    if (context->SampleCoverage == NULL && context->SampleCoverageARB != NULL) context->SampleCoverage = (PFNGLSAMPLECOVERAGEPROC)context->SampleCoverageARB;
    if (context->SampleCoverageARB == NULL && context->SampleCoverage != NULL) context->SampleCoverageARB = (PFNGLSAMPLECOVERAGEARBPROC)context->SampleCoverage;
    if (context->SecondaryColor3b == NULL && context->SecondaryColor3bEXT != NULL) context->SecondaryColor3b = (PFNGLSECONDARYCOLOR3BPROC)context->SecondaryColor3bEXT;
    if (context->SecondaryColor3bEXT == NULL && context->SecondaryColor3b != NULL) context->SecondaryColor3bEXT = (PFNGLSECONDARYCOLOR3BEXTPROC)context->SecondaryColor3b;
    if (context->SecondaryColor3bv == NULL && context->SecondaryColor3bvEXT != NULL) context->SecondaryColor3bv = (PFNGLSECONDARYCOLOR3BVPROC)context->SecondaryColor3bvEXT;
    if (context->SecondaryColor3bvEXT == NULL && context->SecondaryColor3bv != NULL) context->SecondaryColor3bvEXT = (PFNGLSECONDARYCOLOR3BVEXTPROC)context->SecondaryColor3bv;
    if (context->SecondaryColor3d == NULL && context->SecondaryColor3dEXT != NULL) context->SecondaryColor3d = (PFNGLSECONDARYCOLOR3DPROC)context->SecondaryColor3dEXT;
    if (context->SecondaryColor3dEXT == NULL && context->SecondaryColor3d != NULL) context->SecondaryColor3dEXT = (PFNGLSECONDARYCOLOR3DEXTPROC)context->SecondaryColor3d;
    if (context->SecondaryColor3dv == NULL && context->SecondaryColor3dvEXT != NULL) context->SecondaryColor3dv = (PFNGLSECONDARYCOLOR3DVPROC)context->SecondaryColor3dvEXT;
    if (context->SecondaryColor3dvEXT == NULL && context->SecondaryColor3dv != NULL) context->SecondaryColor3dvEXT = (PFNGLSECONDARYCOLOR3DVEXTPROC)context->SecondaryColor3dv;
    if (context->SecondaryColor3f == NULL && context->SecondaryColor3fEXT != NULL) context->SecondaryColor3f = (PFNGLSECONDARYCOLOR3FPROC)context->SecondaryColor3fEXT;
    if (context->SecondaryColor3fEXT == NULL && context->SecondaryColor3f != NULL) context->SecondaryColor3fEXT = (PFNGLSECONDARYCOLOR3FEXTPROC)context->SecondaryColor3f;
    if (context->SecondaryColor3fv == NULL && context->SecondaryColor3fvEXT != NULL) context->SecondaryColor3fv = (PFNGLSECONDARYCOLOR3FVPROC)context->SecondaryColor3fvEXT;
    if (context->SecondaryColor3fvEXT == NULL && context->SecondaryColor3fv != NULL) context->SecondaryColor3fvEXT = (PFNGLSECONDARYCOLOR3FVEXTPROC)context->SecondaryColor3fv;
    if (context->SecondaryColor3i == NULL && context->SecondaryColor3iEXT != NULL) context->SecondaryColor3i = (PFNGLSECONDARYCOLOR3IPROC)context->SecondaryColor3iEXT;
    if (context->SecondaryColor3iEXT == NULL && context->SecondaryColor3i != NULL) context->SecondaryColor3iEXT = (PFNGLSECONDARYCOLOR3IEXTPROC)context->SecondaryColor3i;
    if (context->SecondaryColor3iv == NULL && context->SecondaryColor3ivEXT != NULL) context->SecondaryColor3iv = (PFNGLSECONDARYCOLOR3IVPROC)context->SecondaryColor3ivEXT;
    if (context->SecondaryColor3ivEXT == NULL && context->SecondaryColor3iv != NULL) context->SecondaryColor3ivEXT = (PFNGLSECONDARYCOLOR3IVEXTPROC)context->SecondaryColor3iv;
    if (context->SecondaryColor3s == NULL && context->SecondaryColor3sEXT != NULL) context->SecondaryColor3s = (PFNGLSECONDARYCOLOR3SPROC)context->SecondaryColor3sEXT;
    if (context->SecondaryColor3sEXT == NULL && context->SecondaryColor3s != NULL) context->SecondaryColor3sEXT = (PFNGLSECONDARYCOLOR3SEXTPROC)context->SecondaryColor3s;
    if (context->SecondaryColor3sv == NULL && context->SecondaryColor3svEXT != NULL) context->SecondaryColor3sv = (PFNGLSECONDARYCOLOR3SVPROC)context->SecondaryColor3svEXT;
    if (context->SecondaryColor3svEXT == NULL && context->SecondaryColor3sv != NULL) context->SecondaryColor3svEXT = (PFNGLSECONDARYCOLOR3SVEXTPROC)context->SecondaryColor3sv;
    if (context->SecondaryColor3ub == NULL && context->SecondaryColor3ubEXT != NULL) context->SecondaryColor3ub = (PFNGLSECONDARYCOLOR3UBPROC)context->SecondaryColor3ubEXT;
    if (context->SecondaryColor3ubEXT == NULL && context->SecondaryColor3ub != NULL) context->SecondaryColor3ubEXT = (PFNGLSECONDARYCOLOR3UBEXTPROC)context->SecondaryColor3ub;
    if (context->SecondaryColor3ubv == NULL && context->SecondaryColor3ubvEXT != NULL) context->SecondaryColor3ubv = (PFNGLSECONDARYCOLOR3UBVPROC)context->SecondaryColor3ubvEXT;
    if (context->SecondaryColor3ubvEXT == NULL && context->SecondaryColor3ubv != NULL) context->SecondaryColor3ubvEXT = (PFNGLSECONDARYCOLOR3UBVEXTPROC)context->SecondaryColor3ubv;
    if (context->SecondaryColor3ui == NULL && context->SecondaryColor3uiEXT != NULL) context->SecondaryColor3ui = (PFNGLSECONDARYCOLOR3UIPROC)context->SecondaryColor3uiEXT;
    if (context->SecondaryColor3uiEXT == NULL && context->SecondaryColor3ui != NULL) context->SecondaryColor3uiEXT = (PFNGLSECONDARYCOLOR3UIEXTPROC)context->SecondaryColor3ui;
    if (context->SecondaryColor3uiv == NULL && context->SecondaryColor3uivEXT != NULL) context->SecondaryColor3uiv = (PFNGLSECONDARYCOLOR3UIVPROC)context->SecondaryColor3uivEXT;
    if (context->SecondaryColor3uivEXT == NULL && context->SecondaryColor3uiv != NULL) context->SecondaryColor3uivEXT = (PFNGLSECONDARYCOLOR3UIVEXTPROC)context->SecondaryColor3uiv;
    if (context->SecondaryColor3us == NULL && context->SecondaryColor3usEXT != NULL) context->SecondaryColor3us = (PFNGLSECONDARYCOLOR3USPROC)context->SecondaryColor3usEXT;
    if (context->SecondaryColor3usEXT == NULL && context->SecondaryColor3us != NULL) context->SecondaryColor3usEXT = (PFNGLSECONDARYCOLOR3USEXTPROC)context->SecondaryColor3us;
    if (context->SecondaryColor3usv == NULL && context->SecondaryColor3usvEXT != NULL) context->SecondaryColor3usv = (PFNGLSECONDARYCOLOR3USVPROC)context->SecondaryColor3usvEXT;
    if (context->SecondaryColor3usvEXT == NULL && context->SecondaryColor3usv != NULL) context->SecondaryColor3usvEXT = (PFNGLSECONDARYCOLOR3USVEXTPROC)context->SecondaryColor3usv;
    if (context->SecondaryColorPointer == NULL && context->SecondaryColorPointerEXT != NULL) context->SecondaryColorPointer = (PFNGLSECONDARYCOLORPOINTERPROC)context->SecondaryColorPointerEXT;
    if (context->SecondaryColorPointerEXT == NULL && context->SecondaryColorPointer != NULL) context->SecondaryColorPointerEXT = (PFNGLSECONDARYCOLORPOINTEREXTPROC)context->SecondaryColorPointer;
    if (context->ShaderSource == NULL && context->ShaderSourceARB != NULL) context->ShaderSource = (PFNGLSHADERSOURCEPROC)context->ShaderSourceARB;
    if (context->ShaderSourceARB == NULL && context->ShaderSource != NULL) context->ShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)context->ShaderSource;
    if (context->StencilOpSeparate == NULL && context->StencilOpSeparateATI != NULL) context->StencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)context->StencilOpSeparateATI;
    if (context->StencilOpSeparateATI == NULL && context->StencilOpSeparate != NULL) context->StencilOpSeparateATI = (PFNGLSTENCILOPSEPARATEATIPROC)context->StencilOpSeparate;
    if (context->TexImage3D == NULL && context->TexImage3DEXT != NULL) context->TexImage3D = (PFNGLTEXIMAGE3DPROC)context->TexImage3DEXT;
    if (context->TexImage3DEXT == NULL && context->TexImage3D != NULL) context->TexImage3DEXT = (PFNGLTEXIMAGE3DEXTPROC)context->TexImage3D;
    if (context->TexSubImage1D == NULL && context->TexSubImage1DEXT != NULL) context->TexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC)context->TexSubImage1DEXT;
    if (context->TexSubImage1DEXT == NULL && context->TexSubImage1D != NULL) context->TexSubImage1DEXT = (PFNGLTEXSUBIMAGE1DEXTPROC)context->TexSubImage1D;
    if (context->TexSubImage2D == NULL && context->TexSubImage2DEXT != NULL) context->TexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)context->TexSubImage2DEXT;
    if (context->TexSubImage2DEXT == NULL && context->TexSubImage2D != NULL) context->TexSubImage2DEXT = (PFNGLTEXSUBIMAGE2DEXTPROC)context->TexSubImage2D;
    if (context->TexSubImage3D == NULL && context->TexSubImage3DEXT != NULL) context->TexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)context->TexSubImage3DEXT;
    if (context->TexSubImage3DEXT == NULL && context->TexSubImage3D != NULL) context->TexSubImage3DEXT = (PFNGLTEXSUBIMAGE3DEXTPROC)context->TexSubImage3D;
    if (context->Uniform1f == NULL && context->Uniform1fARB != NULL) context->Uniform1f = (PFNGLUNIFORM1FPROC)context->Uniform1fARB;
    if (context->Uniform1fARB == NULL && context->Uniform1f != NULL) context->Uniform1fARB = (PFNGLUNIFORM1FARBPROC)context->Uniform1f;
    if (context->Uniform1fv == NULL && context->Uniform1fvARB != NULL) context->Uniform1fv = (PFNGLUNIFORM1FVPROC)context->Uniform1fvARB;
    if (context->Uniform1fvARB == NULL && context->Uniform1fv != NULL) context->Uniform1fvARB = (PFNGLUNIFORM1FVARBPROC)context->Uniform1fv;
    if (context->Uniform1i == NULL && context->Uniform1iARB != NULL) context->Uniform1i = (PFNGLUNIFORM1IPROC)context->Uniform1iARB;
    if (context->Uniform1iARB == NULL && context->Uniform1i != NULL) context->Uniform1iARB = (PFNGLUNIFORM1IARBPROC)context->Uniform1i;
    if (context->Uniform1iv == NULL && context->Uniform1ivARB != NULL) context->Uniform1iv = (PFNGLUNIFORM1IVPROC)context->Uniform1ivARB;
    if (context->Uniform1ivARB == NULL && context->Uniform1iv != NULL) context->Uniform1ivARB = (PFNGLUNIFORM1IVARBPROC)context->Uniform1iv;
    if (context->Uniform2f == NULL && context->Uniform2fARB != NULL) context->Uniform2f = (PFNGLUNIFORM2FPROC)context->Uniform2fARB;
    if (context->Uniform2fARB == NULL && context->Uniform2f != NULL) context->Uniform2fARB = (PFNGLUNIFORM2FARBPROC)context->Uniform2f;
    if (context->Uniform2fv == NULL && context->Uniform2fvARB != NULL) context->Uniform2fv = (PFNGLUNIFORM2FVPROC)context->Uniform2fvARB;
    if (context->Uniform2fvARB == NULL && context->Uniform2fv != NULL) context->Uniform2fvARB = (PFNGLUNIFORM2FVARBPROC)context->Uniform2fv;
    if (context->Uniform2i == NULL && context->Uniform2iARB != NULL) context->Uniform2i = (PFNGLUNIFORM2IPROC)context->Uniform2iARB;
    if (context->Uniform2iARB == NULL && context->Uniform2i != NULL) context->Uniform2iARB = (PFNGLUNIFORM2IARBPROC)context->Uniform2i;
    if (context->Uniform2iv == NULL && context->Uniform2ivARB != NULL) context->Uniform2iv = (PFNGLUNIFORM2IVPROC)context->Uniform2ivARB;
    if (context->Uniform2ivARB == NULL && context->Uniform2iv != NULL) context->Uniform2ivARB = (PFNGLUNIFORM2IVARBPROC)context->Uniform2iv;
    if (context->Uniform3f == NULL && context->Uniform3fARB != NULL) context->Uniform3f = (PFNGLUNIFORM3FPROC)context->Uniform3fARB;
    if (context->Uniform3fARB == NULL && context->Uniform3f != NULL) context->Uniform3fARB = (PFNGLUNIFORM3FARBPROC)context->Uniform3f;
    if (context->Uniform3fv == NULL && context->Uniform3fvARB != NULL) context->Uniform3fv = (PFNGLUNIFORM3FVPROC)context->Uniform3fvARB;
    if (context->Uniform3fvARB == NULL && context->Uniform3fv != NULL) context->Uniform3fvARB = (PFNGLUNIFORM3FVARBPROC)context->Uniform3fv;
    if (context->Uniform3i == NULL && context->Uniform3iARB != NULL) context->Uniform3i = (PFNGLUNIFORM3IPROC)context->Uniform3iARB;
    if (context->Uniform3iARB == NULL && context->Uniform3i != NULL) context->Uniform3iARB = (PFNGLUNIFORM3IARBPROC)context->Uniform3i;
    if (context->Uniform3iv == NULL && context->Uniform3ivARB != NULL) context->Uniform3iv = (PFNGLUNIFORM3IVPROC)context->Uniform3ivARB;
    if (context->Uniform3ivARB == NULL && context->Uniform3iv != NULL) context->Uniform3ivARB = (PFNGLUNIFORM3IVARBPROC)context->Uniform3iv;
    if (context->Uniform4f == NULL && context->Uniform4fARB != NULL) context->Uniform4f = (PFNGLUNIFORM4FPROC)context->Uniform4fARB;
    if (context->Uniform4fARB == NULL && context->Uniform4f != NULL) context->Uniform4fARB = (PFNGLUNIFORM4FARBPROC)context->Uniform4f;
    if (context->Uniform4fv == NULL && context->Uniform4fvARB != NULL) context->Uniform4fv = (PFNGLUNIFORM4FVPROC)context->Uniform4fvARB;
    if (context->Uniform4fvARB == NULL && context->Uniform4fv != NULL) context->Uniform4fvARB = (PFNGLUNIFORM4FVARBPROC)context->Uniform4fv;
    if (context->Uniform4i == NULL && context->Uniform4iARB != NULL) context->Uniform4i = (PFNGLUNIFORM4IPROC)context->Uniform4iARB;
    if (context->Uniform4iARB == NULL && context->Uniform4i != NULL) context->Uniform4iARB = (PFNGLUNIFORM4IARBPROC)context->Uniform4i;
    if (context->Uniform4iv == NULL && context->Uniform4ivARB != NULL) context->Uniform4iv = (PFNGLUNIFORM4IVPROC)context->Uniform4ivARB;
    if (context->Uniform4ivARB == NULL && context->Uniform4iv != NULL) context->Uniform4ivARB = (PFNGLUNIFORM4IVARBPROC)context->Uniform4iv;
    if (context->UniformMatrix2fv == NULL && context->UniformMatrix2fvARB != NULL) context->UniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)context->UniformMatrix2fvARB;
    if (context->UniformMatrix2fvARB == NULL && context->UniformMatrix2fv != NULL) context->UniformMatrix2fvARB = (PFNGLUNIFORMMATRIX2FVARBPROC)context->UniformMatrix2fv;
    if (context->UniformMatrix3fv == NULL && context->UniformMatrix3fvARB != NULL) context->UniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)context->UniformMatrix3fvARB;
    if (context->UniformMatrix3fvARB == NULL && context->UniformMatrix3fv != NULL) context->UniformMatrix3fvARB = (PFNGLUNIFORMMATRIX3FVARBPROC)context->UniformMatrix3fv;
    if (context->UniformMatrix4fv == NULL && context->UniformMatrix4fvARB != NULL) context->UniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)context->UniformMatrix4fvARB;
    if (context->UniformMatrix4fvARB == NULL && context->UniformMatrix4fv != NULL) context->UniformMatrix4fvARB = (PFNGLUNIFORMMATRIX4FVARBPROC)context->UniformMatrix4fv;
    if (context->UnmapBuffer == NULL && context->UnmapBufferARB != NULL) context->UnmapBuffer = (PFNGLUNMAPBUFFERPROC)context->UnmapBufferARB;
    if (context->UnmapBufferARB == NULL && context->UnmapBuffer != NULL) context->UnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)context->UnmapBuffer;
    if (context->UseProgram == NULL && context->UseProgramObjectARB != NULL) context->UseProgram = (PFNGLUSEPROGRAMPROC)context->UseProgramObjectARB;
    if (context->UseProgramObjectARB == NULL && context->UseProgram != NULL) context->UseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC)context->UseProgram;
    if (context->ValidateProgram == NULL && context->ValidateProgramARB != NULL) context->ValidateProgram = (PFNGLVALIDATEPROGRAMPROC)context->ValidateProgramARB;
    if (context->ValidateProgramARB == NULL && context->ValidateProgram != NULL) context->ValidateProgramARB = (PFNGLVALIDATEPROGRAMARBPROC)context->ValidateProgram;
    if (context->VertexAttrib1d == NULL && context->VertexAttrib1dARB != NULL) context->VertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)context->VertexAttrib1dARB;
    if (context->VertexAttrib1d == NULL && context->VertexAttrib1dNV != NULL) context->VertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)context->VertexAttrib1dNV;
    if (context->VertexAttrib1dARB == NULL && context->VertexAttrib1d != NULL) context->VertexAttrib1dARB = (PFNGLVERTEXATTRIB1DARBPROC)context->VertexAttrib1d;
    if (context->VertexAttrib1dARB == NULL && context->VertexAttrib1dNV != NULL) context->VertexAttrib1dARB = (PFNGLVERTEXATTRIB1DARBPROC)context->VertexAttrib1dNV;
    if (context->VertexAttrib1dNV == NULL && context->VertexAttrib1d != NULL) context->VertexAttrib1dNV = (PFNGLVERTEXATTRIB1DNVPROC)context->VertexAttrib1d;
    if (context->VertexAttrib1dNV == NULL && context->VertexAttrib1dARB != NULL) context->VertexAttrib1dNV = (PFNGLVERTEXATTRIB1DNVPROC)context->VertexAttrib1dARB;
    if (context->VertexAttrib1dv == NULL && context->VertexAttrib1dvARB != NULL) context->VertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)context->VertexAttrib1dvARB;
    if (context->VertexAttrib1dv == NULL && context->VertexAttrib1dvNV != NULL) context->VertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)context->VertexAttrib1dvNV;
    if (context->VertexAttrib1dvARB == NULL && context->VertexAttrib1dv != NULL) context->VertexAttrib1dvARB = (PFNGLVERTEXATTRIB1DVARBPROC)context->VertexAttrib1dv;
    if (context->VertexAttrib1dvARB == NULL && context->VertexAttrib1dvNV != NULL) context->VertexAttrib1dvARB = (PFNGLVERTEXATTRIB1DVARBPROC)context->VertexAttrib1dvNV;
    if (context->VertexAttrib1dvNV == NULL && context->VertexAttrib1dv != NULL) context->VertexAttrib1dvNV = (PFNGLVERTEXATTRIB1DVNVPROC)context->VertexAttrib1dv;
    if (context->VertexAttrib1dvNV == NULL && context->VertexAttrib1dvARB != NULL) context->VertexAttrib1dvNV = (PFNGLVERTEXATTRIB1DVNVPROC)context->VertexAttrib1dvARB;
    if (context->VertexAttrib1f == NULL && context->VertexAttrib1fARB != NULL) context->VertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)context->VertexAttrib1fARB;
    if (context->VertexAttrib1f == NULL && context->VertexAttrib1fNV != NULL) context->VertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)context->VertexAttrib1fNV;
    if (context->VertexAttrib1fARB == NULL && context->VertexAttrib1f != NULL) context->VertexAttrib1fARB = (PFNGLVERTEXATTRIB1FARBPROC)context->VertexAttrib1f;
    if (context->VertexAttrib1fARB == NULL && context->VertexAttrib1fNV != NULL) context->VertexAttrib1fARB = (PFNGLVERTEXATTRIB1FARBPROC)context->VertexAttrib1fNV;
    if (context->VertexAttrib1fNV == NULL && context->VertexAttrib1f != NULL) context->VertexAttrib1fNV = (PFNGLVERTEXATTRIB1FNVPROC)context->VertexAttrib1f;
    if (context->VertexAttrib1fNV == NULL && context->VertexAttrib1fARB != NULL) context->VertexAttrib1fNV = (PFNGLVERTEXATTRIB1FNVPROC)context->VertexAttrib1fARB;
    if (context->VertexAttrib1fv == NULL && context->VertexAttrib1fvARB != NULL) context->VertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)context->VertexAttrib1fvARB;
    if (context->VertexAttrib1fv == NULL && context->VertexAttrib1fvNV != NULL) context->VertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)context->VertexAttrib1fvNV;
    if (context->VertexAttrib1fvARB == NULL && context->VertexAttrib1fv != NULL) context->VertexAttrib1fvARB = (PFNGLVERTEXATTRIB1FVARBPROC)context->VertexAttrib1fv;
    if (context->VertexAttrib1fvARB == NULL && context->VertexAttrib1fvNV != NULL) context->VertexAttrib1fvARB = (PFNGLVERTEXATTRIB1FVARBPROC)context->VertexAttrib1fvNV;
    if (context->VertexAttrib1fvNV == NULL && context->VertexAttrib1fv != NULL) context->VertexAttrib1fvNV = (PFNGLVERTEXATTRIB1FVNVPROC)context->VertexAttrib1fv;
    if (context->VertexAttrib1fvNV == NULL && context->VertexAttrib1fvARB != NULL) context->VertexAttrib1fvNV = (PFNGLVERTEXATTRIB1FVNVPROC)context->VertexAttrib1fvARB;
    if (context->VertexAttrib1s == NULL && context->VertexAttrib1sARB != NULL) context->VertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)context->VertexAttrib1sARB;
    if (context->VertexAttrib1s == NULL && context->VertexAttrib1sNV != NULL) context->VertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)context->VertexAttrib1sNV;
    if (context->VertexAttrib1sARB == NULL && context->VertexAttrib1s != NULL) context->VertexAttrib1sARB = (PFNGLVERTEXATTRIB1SARBPROC)context->VertexAttrib1s;
    if (context->VertexAttrib1sARB == NULL && context->VertexAttrib1sNV != NULL) context->VertexAttrib1sARB = (PFNGLVERTEXATTRIB1SARBPROC)context->VertexAttrib1sNV;
    if (context->VertexAttrib1sNV == NULL && context->VertexAttrib1s != NULL) context->VertexAttrib1sNV = (PFNGLVERTEXATTRIB1SNVPROC)context->VertexAttrib1s;
    if (context->VertexAttrib1sNV == NULL && context->VertexAttrib1sARB != NULL) context->VertexAttrib1sNV = (PFNGLVERTEXATTRIB1SNVPROC)context->VertexAttrib1sARB;
    if (context->VertexAttrib1sv == NULL && context->VertexAttrib1svARB != NULL) context->VertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)context->VertexAttrib1svARB;
    if (context->VertexAttrib1sv == NULL && context->VertexAttrib1svNV != NULL) context->VertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)context->VertexAttrib1svNV;
    if (context->VertexAttrib1svARB == NULL && context->VertexAttrib1sv != NULL) context->VertexAttrib1svARB = (PFNGLVERTEXATTRIB1SVARBPROC)context->VertexAttrib1sv;
    if (context->VertexAttrib1svARB == NULL && context->VertexAttrib1svNV != NULL) context->VertexAttrib1svARB = (PFNGLVERTEXATTRIB1SVARBPROC)context->VertexAttrib1svNV;
    if (context->VertexAttrib1svNV == NULL && context->VertexAttrib1sv != NULL) context->VertexAttrib1svNV = (PFNGLVERTEXATTRIB1SVNVPROC)context->VertexAttrib1sv;
    if (context->VertexAttrib1svNV == NULL && context->VertexAttrib1svARB != NULL) context->VertexAttrib1svNV = (PFNGLVERTEXATTRIB1SVNVPROC)context->VertexAttrib1svARB;
    if (context->VertexAttrib2d == NULL && context->VertexAttrib2dARB != NULL) context->VertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)context->VertexAttrib2dARB;
    if (context->VertexAttrib2d == NULL && context->VertexAttrib2dNV != NULL) context->VertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)context->VertexAttrib2dNV;
    if (context->VertexAttrib2dARB == NULL && context->VertexAttrib2d != NULL) context->VertexAttrib2dARB = (PFNGLVERTEXATTRIB2DARBPROC)context->VertexAttrib2d;
    if (context->VertexAttrib2dARB == NULL && context->VertexAttrib2dNV != NULL) context->VertexAttrib2dARB = (PFNGLVERTEXATTRIB2DARBPROC)context->VertexAttrib2dNV;
    if (context->VertexAttrib2dNV == NULL && context->VertexAttrib2d != NULL) context->VertexAttrib2dNV = (PFNGLVERTEXATTRIB2DNVPROC)context->VertexAttrib2d;
    if (context->VertexAttrib2dNV == NULL && context->VertexAttrib2dARB != NULL) context->VertexAttrib2dNV = (PFNGLVERTEXATTRIB2DNVPROC)context->VertexAttrib2dARB;
    if (context->VertexAttrib2dv == NULL && context->VertexAttrib2dvARB != NULL) context->VertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)context->VertexAttrib2dvARB;
    if (context->VertexAttrib2dv == NULL && context->VertexAttrib2dvNV != NULL) context->VertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)context->VertexAttrib2dvNV;
    if (context->VertexAttrib2dvARB == NULL && context->VertexAttrib2dv != NULL) context->VertexAttrib2dvARB = (PFNGLVERTEXATTRIB2DVARBPROC)context->VertexAttrib2dv;
    if (context->VertexAttrib2dvARB == NULL && context->VertexAttrib2dvNV != NULL) context->VertexAttrib2dvARB = (PFNGLVERTEXATTRIB2DVARBPROC)context->VertexAttrib2dvNV;
    if (context->VertexAttrib2dvNV == NULL && context->VertexAttrib2dv != NULL) context->VertexAttrib2dvNV = (PFNGLVERTEXATTRIB2DVNVPROC)context->VertexAttrib2dv;
    if (context->VertexAttrib2dvNV == NULL && context->VertexAttrib2dvARB != NULL) context->VertexAttrib2dvNV = (PFNGLVERTEXATTRIB2DVNVPROC)context->VertexAttrib2dvARB;
    if (context->VertexAttrib2f == NULL && context->VertexAttrib2fARB != NULL) context->VertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)context->VertexAttrib2fARB;
    if (context->VertexAttrib2f == NULL && context->VertexAttrib2fNV != NULL) context->VertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)context->VertexAttrib2fNV;
    if (context->VertexAttrib2fARB == NULL && context->VertexAttrib2f != NULL) context->VertexAttrib2fARB = (PFNGLVERTEXATTRIB2FARBPROC)context->VertexAttrib2f;
    if (context->VertexAttrib2fARB == NULL && context->VertexAttrib2fNV != NULL) context->VertexAttrib2fARB = (PFNGLVERTEXATTRIB2FARBPROC)context->VertexAttrib2fNV;
    if (context->VertexAttrib2fNV == NULL && context->VertexAttrib2f != NULL) context->VertexAttrib2fNV = (PFNGLVERTEXATTRIB2FNVPROC)context->VertexAttrib2f;
    if (context->VertexAttrib2fNV == NULL && context->VertexAttrib2fARB != NULL) context->VertexAttrib2fNV = (PFNGLVERTEXATTRIB2FNVPROC)context->VertexAttrib2fARB;
    if (context->VertexAttrib2fv == NULL && context->VertexAttrib2fvARB != NULL) context->VertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)context->VertexAttrib2fvARB;
    if (context->VertexAttrib2fv == NULL && context->VertexAttrib2fvNV != NULL) context->VertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)context->VertexAttrib2fvNV;
    if (context->VertexAttrib2fvARB == NULL && context->VertexAttrib2fv != NULL) context->VertexAttrib2fvARB = (PFNGLVERTEXATTRIB2FVARBPROC)context->VertexAttrib2fv;
    if (context->VertexAttrib2fvARB == NULL && context->VertexAttrib2fvNV != NULL) context->VertexAttrib2fvARB = (PFNGLVERTEXATTRIB2FVARBPROC)context->VertexAttrib2fvNV;
    if (context->VertexAttrib2fvNV == NULL && context->VertexAttrib2fv != NULL) context->VertexAttrib2fvNV = (PFNGLVERTEXATTRIB2FVNVPROC)context->VertexAttrib2fv;
    if (context->VertexAttrib2fvNV == NULL && context->VertexAttrib2fvARB != NULL) context->VertexAttrib2fvNV = (PFNGLVERTEXATTRIB2FVNVPROC)context->VertexAttrib2fvARB;
    if (context->VertexAttrib2s == NULL && context->VertexAttrib2sARB != NULL) context->VertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)context->VertexAttrib2sARB;
    if (context->VertexAttrib2s == NULL && context->VertexAttrib2sNV != NULL) context->VertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)context->VertexAttrib2sNV;
    if (context->VertexAttrib2sARB == NULL && context->VertexAttrib2s != NULL) context->VertexAttrib2sARB = (PFNGLVERTEXATTRIB2SARBPROC)context->VertexAttrib2s;
    if (context->VertexAttrib2sARB == NULL && context->VertexAttrib2sNV != NULL) context->VertexAttrib2sARB = (PFNGLVERTEXATTRIB2SARBPROC)context->VertexAttrib2sNV;
    if (context->VertexAttrib2sNV == NULL && context->VertexAttrib2s != NULL) context->VertexAttrib2sNV = (PFNGLVERTEXATTRIB2SNVPROC)context->VertexAttrib2s;
    if (context->VertexAttrib2sNV == NULL && context->VertexAttrib2sARB != NULL) context->VertexAttrib2sNV = (PFNGLVERTEXATTRIB2SNVPROC)context->VertexAttrib2sARB;
    if (context->VertexAttrib2sv == NULL && context->VertexAttrib2svARB != NULL) context->VertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)context->VertexAttrib2svARB;
    if (context->VertexAttrib2sv == NULL && context->VertexAttrib2svNV != NULL) context->VertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)context->VertexAttrib2svNV;
    if (context->VertexAttrib2svARB == NULL && context->VertexAttrib2sv != NULL) context->VertexAttrib2svARB = (PFNGLVERTEXATTRIB2SVARBPROC)context->VertexAttrib2sv;
    if (context->VertexAttrib2svARB == NULL && context->VertexAttrib2svNV != NULL) context->VertexAttrib2svARB = (PFNGLVERTEXATTRIB2SVARBPROC)context->VertexAttrib2svNV;
    if (context->VertexAttrib2svNV == NULL && context->VertexAttrib2sv != NULL) context->VertexAttrib2svNV = (PFNGLVERTEXATTRIB2SVNVPROC)context->VertexAttrib2sv;
    if (context->VertexAttrib2svNV == NULL && context->VertexAttrib2svARB != NULL) context->VertexAttrib2svNV = (PFNGLVERTEXATTRIB2SVNVPROC)context->VertexAttrib2svARB;
    if (context->VertexAttrib3d == NULL && context->VertexAttrib3dARB != NULL) context->VertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)context->VertexAttrib3dARB;
    if (context->VertexAttrib3d == NULL && context->VertexAttrib3dNV != NULL) context->VertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)context->VertexAttrib3dNV;
    if (context->VertexAttrib3dARB == NULL && context->VertexAttrib3d != NULL) context->VertexAttrib3dARB = (PFNGLVERTEXATTRIB3DARBPROC)context->VertexAttrib3d;
    if (context->VertexAttrib3dARB == NULL && context->VertexAttrib3dNV != NULL) context->VertexAttrib3dARB = (PFNGLVERTEXATTRIB3DARBPROC)context->VertexAttrib3dNV;
    if (context->VertexAttrib3dNV == NULL && context->VertexAttrib3d != NULL) context->VertexAttrib3dNV = (PFNGLVERTEXATTRIB3DNVPROC)context->VertexAttrib3d;
    if (context->VertexAttrib3dNV == NULL && context->VertexAttrib3dARB != NULL) context->VertexAttrib3dNV = (PFNGLVERTEXATTRIB3DNVPROC)context->VertexAttrib3dARB;
    if (context->VertexAttrib3dv == NULL && context->VertexAttrib3dvARB != NULL) context->VertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)context->VertexAttrib3dvARB;
    if (context->VertexAttrib3dv == NULL && context->VertexAttrib3dvNV != NULL) context->VertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)context->VertexAttrib3dvNV;
    if (context->VertexAttrib3dvARB == NULL && context->VertexAttrib3dv != NULL) context->VertexAttrib3dvARB = (PFNGLVERTEXATTRIB3DVARBPROC)context->VertexAttrib3dv;
    if (context->VertexAttrib3dvARB == NULL && context->VertexAttrib3dvNV != NULL) context->VertexAttrib3dvARB = (PFNGLVERTEXATTRIB3DVARBPROC)context->VertexAttrib3dvNV;
    if (context->VertexAttrib3dvNV == NULL && context->VertexAttrib3dv != NULL) context->VertexAttrib3dvNV = (PFNGLVERTEXATTRIB3DVNVPROC)context->VertexAttrib3dv;
    if (context->VertexAttrib3dvNV == NULL && context->VertexAttrib3dvARB != NULL) context->VertexAttrib3dvNV = (PFNGLVERTEXATTRIB3DVNVPROC)context->VertexAttrib3dvARB;
    if (context->VertexAttrib3f == NULL && context->VertexAttrib3fARB != NULL) context->VertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)context->VertexAttrib3fARB;
    if (context->VertexAttrib3f == NULL && context->VertexAttrib3fNV != NULL) context->VertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)context->VertexAttrib3fNV;
    if (context->VertexAttrib3fARB == NULL && context->VertexAttrib3f != NULL) context->VertexAttrib3fARB = (PFNGLVERTEXATTRIB3FARBPROC)context->VertexAttrib3f;
    if (context->VertexAttrib3fARB == NULL && context->VertexAttrib3fNV != NULL) context->VertexAttrib3fARB = (PFNGLVERTEXATTRIB3FARBPROC)context->VertexAttrib3fNV;
    if (context->VertexAttrib3fNV == NULL && context->VertexAttrib3f != NULL) context->VertexAttrib3fNV = (PFNGLVERTEXATTRIB3FNVPROC)context->VertexAttrib3f;
    if (context->VertexAttrib3fNV == NULL && context->VertexAttrib3fARB != NULL) context->VertexAttrib3fNV = (PFNGLVERTEXATTRIB3FNVPROC)context->VertexAttrib3fARB;
    if (context->VertexAttrib3fv == NULL && context->VertexAttrib3fvARB != NULL) context->VertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)context->VertexAttrib3fvARB;
    if (context->VertexAttrib3fv == NULL && context->VertexAttrib3fvNV != NULL) context->VertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)context->VertexAttrib3fvNV;
    if (context->VertexAttrib3fvARB == NULL && context->VertexAttrib3fv != NULL) context->VertexAttrib3fvARB = (PFNGLVERTEXATTRIB3FVARBPROC)context->VertexAttrib3fv;
    if (context->VertexAttrib3fvARB == NULL && context->VertexAttrib3fvNV != NULL) context->VertexAttrib3fvARB = (PFNGLVERTEXATTRIB3FVARBPROC)context->VertexAttrib3fvNV;
    if (context->VertexAttrib3fvNV == NULL && context->VertexAttrib3fv != NULL) context->VertexAttrib3fvNV = (PFNGLVERTEXATTRIB3FVNVPROC)context->VertexAttrib3fv;
    if (context->VertexAttrib3fvNV == NULL && context->VertexAttrib3fvARB != NULL) context->VertexAttrib3fvNV = (PFNGLVERTEXATTRIB3FVNVPROC)context->VertexAttrib3fvARB;
    if (context->VertexAttrib3s == NULL && context->VertexAttrib3sARB != NULL) context->VertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)context->VertexAttrib3sARB;
    if (context->VertexAttrib3s == NULL && context->VertexAttrib3sNV != NULL) context->VertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)context->VertexAttrib3sNV;
    if (context->VertexAttrib3sARB == NULL && context->VertexAttrib3s != NULL) context->VertexAttrib3sARB = (PFNGLVERTEXATTRIB3SARBPROC)context->VertexAttrib3s;
    if (context->VertexAttrib3sARB == NULL && context->VertexAttrib3sNV != NULL) context->VertexAttrib3sARB = (PFNGLVERTEXATTRIB3SARBPROC)context->VertexAttrib3sNV;
    if (context->VertexAttrib3sNV == NULL && context->VertexAttrib3s != NULL) context->VertexAttrib3sNV = (PFNGLVERTEXATTRIB3SNVPROC)context->VertexAttrib3s;
    if (context->VertexAttrib3sNV == NULL && context->VertexAttrib3sARB != NULL) context->VertexAttrib3sNV = (PFNGLVERTEXATTRIB3SNVPROC)context->VertexAttrib3sARB;
    if (context->VertexAttrib3sv == NULL && context->VertexAttrib3svARB != NULL) context->VertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)context->VertexAttrib3svARB;
    if (context->VertexAttrib3sv == NULL && context->VertexAttrib3svNV != NULL) context->VertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)context->VertexAttrib3svNV;
    if (context->VertexAttrib3svARB == NULL && context->VertexAttrib3sv != NULL) context->VertexAttrib3svARB = (PFNGLVERTEXATTRIB3SVARBPROC)context->VertexAttrib3sv;
    if (context->VertexAttrib3svARB == NULL && context->VertexAttrib3svNV != NULL) context->VertexAttrib3svARB = (PFNGLVERTEXATTRIB3SVARBPROC)context->VertexAttrib3svNV;
    if (context->VertexAttrib3svNV == NULL && context->VertexAttrib3sv != NULL) context->VertexAttrib3svNV = (PFNGLVERTEXATTRIB3SVNVPROC)context->VertexAttrib3sv;
    if (context->VertexAttrib3svNV == NULL && context->VertexAttrib3svARB != NULL) context->VertexAttrib3svNV = (PFNGLVERTEXATTRIB3SVNVPROC)context->VertexAttrib3svARB;
    if (context->VertexAttrib4bv == NULL && context->VertexAttrib4bvARB != NULL) context->VertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)context->VertexAttrib4bvARB;
    if (context->VertexAttrib4bvARB == NULL && context->VertexAttrib4bv != NULL) context->VertexAttrib4bvARB = (PFNGLVERTEXATTRIB4BVARBPROC)context->VertexAttrib4bv;
    if (context->VertexAttrib4d == NULL && context->VertexAttrib4dARB != NULL) context->VertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)context->VertexAttrib4dARB;
    if (context->VertexAttrib4d == NULL && context->VertexAttrib4dNV != NULL) context->VertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)context->VertexAttrib4dNV;
    if (context->VertexAttrib4dARB == NULL && context->VertexAttrib4d != NULL) context->VertexAttrib4dARB = (PFNGLVERTEXATTRIB4DARBPROC)context->VertexAttrib4d;
    if (context->VertexAttrib4dARB == NULL && context->VertexAttrib4dNV != NULL) context->VertexAttrib4dARB = (PFNGLVERTEXATTRIB4DARBPROC)context->VertexAttrib4dNV;
    if (context->VertexAttrib4dNV == NULL && context->VertexAttrib4d != NULL) context->VertexAttrib4dNV = (PFNGLVERTEXATTRIB4DNVPROC)context->VertexAttrib4d;
    if (context->VertexAttrib4dNV == NULL && context->VertexAttrib4dARB != NULL) context->VertexAttrib4dNV = (PFNGLVERTEXATTRIB4DNVPROC)context->VertexAttrib4dARB;
    if (context->VertexAttrib4dv == NULL && context->VertexAttrib4dvARB != NULL) context->VertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)context->VertexAttrib4dvARB;
    if (context->VertexAttrib4dv == NULL && context->VertexAttrib4dvNV != NULL) context->VertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)context->VertexAttrib4dvNV;
    if (context->VertexAttrib4dvARB == NULL && context->VertexAttrib4dv != NULL) context->VertexAttrib4dvARB = (PFNGLVERTEXATTRIB4DVARBPROC)context->VertexAttrib4dv;
    if (context->VertexAttrib4dvARB == NULL && context->VertexAttrib4dvNV != NULL) context->VertexAttrib4dvARB = (PFNGLVERTEXATTRIB4DVARBPROC)context->VertexAttrib4dvNV;
    if (context->VertexAttrib4dvNV == NULL && context->VertexAttrib4dv != NULL) context->VertexAttrib4dvNV = (PFNGLVERTEXATTRIB4DVNVPROC)context->VertexAttrib4dv;
    if (context->VertexAttrib4dvNV == NULL && context->VertexAttrib4dvARB != NULL) context->VertexAttrib4dvNV = (PFNGLVERTEXATTRIB4DVNVPROC)context->VertexAttrib4dvARB;
    if (context->VertexAttrib4f == NULL && context->VertexAttrib4fARB != NULL) context->VertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)context->VertexAttrib4fARB;
    if (context->VertexAttrib4f == NULL && context->VertexAttrib4fNV != NULL) context->VertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)context->VertexAttrib4fNV;
    if (context->VertexAttrib4fARB == NULL && context->VertexAttrib4f != NULL) context->VertexAttrib4fARB = (PFNGLVERTEXATTRIB4FARBPROC)context->VertexAttrib4f;
    if (context->VertexAttrib4fARB == NULL && context->VertexAttrib4fNV != NULL) context->VertexAttrib4fARB = (PFNGLVERTEXATTRIB4FARBPROC)context->VertexAttrib4fNV;
    if (context->VertexAttrib4fNV == NULL && context->VertexAttrib4f != NULL) context->VertexAttrib4fNV = (PFNGLVERTEXATTRIB4FNVPROC)context->VertexAttrib4f;
    if (context->VertexAttrib4fNV == NULL && context->VertexAttrib4fARB != NULL) context->VertexAttrib4fNV = (PFNGLVERTEXATTRIB4FNVPROC)context->VertexAttrib4fARB;
    if (context->VertexAttrib4fv == NULL && context->VertexAttrib4fvARB != NULL) context->VertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)context->VertexAttrib4fvARB;
    if (context->VertexAttrib4fv == NULL && context->VertexAttrib4fvNV != NULL) context->VertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)context->VertexAttrib4fvNV;
    if (context->VertexAttrib4fvARB == NULL && context->VertexAttrib4fv != NULL) context->VertexAttrib4fvARB = (PFNGLVERTEXATTRIB4FVARBPROC)context->VertexAttrib4fv;
    if (context->VertexAttrib4fvARB == NULL && context->VertexAttrib4fvNV != NULL) context->VertexAttrib4fvARB = (PFNGLVERTEXATTRIB4FVARBPROC)context->VertexAttrib4fvNV;
    if (context->VertexAttrib4fvNV == NULL && context->VertexAttrib4fv != NULL) context->VertexAttrib4fvNV = (PFNGLVERTEXATTRIB4FVNVPROC)context->VertexAttrib4fv;
    if (context->VertexAttrib4fvNV == NULL && context->VertexAttrib4fvARB != NULL) context->VertexAttrib4fvNV = (PFNGLVERTEXATTRIB4FVNVPROC)context->VertexAttrib4fvARB;
    if (context->VertexAttrib4iv == NULL && context->VertexAttrib4ivARB != NULL) context->VertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)context->VertexAttrib4ivARB;
    if (context->VertexAttrib4ivARB == NULL && context->VertexAttrib4iv != NULL) context->VertexAttrib4ivARB = (PFNGLVERTEXATTRIB4IVARBPROC)context->VertexAttrib4iv;
    if (context->VertexAttrib4Nbv == NULL && context->VertexAttrib4NbvARB != NULL) context->VertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)context->VertexAttrib4NbvARB;
    if (context->VertexAttrib4NbvARB == NULL && context->VertexAttrib4Nbv != NULL) context->VertexAttrib4NbvARB = (PFNGLVERTEXATTRIB4NBVARBPROC)context->VertexAttrib4Nbv;
    if (context->VertexAttrib4Niv == NULL && context->VertexAttrib4NivARB != NULL) context->VertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)context->VertexAttrib4NivARB;
    if (context->VertexAttrib4NivARB == NULL && context->VertexAttrib4Niv != NULL) context->VertexAttrib4NivARB = (PFNGLVERTEXATTRIB4NIVARBPROC)context->VertexAttrib4Niv;
    if (context->VertexAttrib4Nsv == NULL && context->VertexAttrib4NsvARB != NULL) context->VertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)context->VertexAttrib4NsvARB;
    if (context->VertexAttrib4NsvARB == NULL && context->VertexAttrib4Nsv != NULL) context->VertexAttrib4NsvARB = (PFNGLVERTEXATTRIB4NSVARBPROC)context->VertexAttrib4Nsv;
    if (context->VertexAttrib4Nub == NULL && context->VertexAttrib4NubARB != NULL) context->VertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)context->VertexAttrib4NubARB;
    if (context->VertexAttrib4Nub == NULL && context->VertexAttrib4ubNV != NULL) context->VertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)context->VertexAttrib4ubNV;
    if (context->VertexAttrib4NubARB == NULL && context->VertexAttrib4Nub != NULL) context->VertexAttrib4NubARB = (PFNGLVERTEXATTRIB4NUBARBPROC)context->VertexAttrib4Nub;
    if (context->VertexAttrib4NubARB == NULL && context->VertexAttrib4ubNV != NULL) context->VertexAttrib4NubARB = (PFNGLVERTEXATTRIB4NUBARBPROC)context->VertexAttrib4ubNV;
    if (context->VertexAttrib4Nubv == NULL && context->VertexAttrib4NubvARB != NULL) context->VertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)context->VertexAttrib4NubvARB;
    if (context->VertexAttrib4Nubv == NULL && context->VertexAttrib4ubvNV != NULL) context->VertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)context->VertexAttrib4ubvNV;
    if (context->VertexAttrib4NubvARB == NULL && context->VertexAttrib4Nubv != NULL) context->VertexAttrib4NubvARB = (PFNGLVERTEXATTRIB4NUBVARBPROC)context->VertexAttrib4Nubv;
    if (context->VertexAttrib4NubvARB == NULL && context->VertexAttrib4ubvNV != NULL) context->VertexAttrib4NubvARB = (PFNGLVERTEXATTRIB4NUBVARBPROC)context->VertexAttrib4ubvNV;
    if (context->VertexAttrib4Nuiv == NULL && context->VertexAttrib4NuivARB != NULL) context->VertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)context->VertexAttrib4NuivARB;
    if (context->VertexAttrib4NuivARB == NULL && context->VertexAttrib4Nuiv != NULL) context->VertexAttrib4NuivARB = (PFNGLVERTEXATTRIB4NUIVARBPROC)context->VertexAttrib4Nuiv;
    if (context->VertexAttrib4Nusv == NULL && context->VertexAttrib4NusvARB != NULL) context->VertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)context->VertexAttrib4NusvARB;
    if (context->VertexAttrib4NusvARB == NULL && context->VertexAttrib4Nusv != NULL) context->VertexAttrib4NusvARB = (PFNGLVERTEXATTRIB4NUSVARBPROC)context->VertexAttrib4Nusv;
    if (context->VertexAttrib4s == NULL && context->VertexAttrib4sARB != NULL) context->VertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)context->VertexAttrib4sARB;
    if (context->VertexAttrib4s == NULL && context->VertexAttrib4sNV != NULL) context->VertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)context->VertexAttrib4sNV;
    if (context->VertexAttrib4sARB == NULL && context->VertexAttrib4s != NULL) context->VertexAttrib4sARB = (PFNGLVERTEXATTRIB4SARBPROC)context->VertexAttrib4s;
    if (context->VertexAttrib4sARB == NULL && context->VertexAttrib4sNV != NULL) context->VertexAttrib4sARB = (PFNGLVERTEXATTRIB4SARBPROC)context->VertexAttrib4sNV;
    if (context->VertexAttrib4sNV == NULL && context->VertexAttrib4s != NULL) context->VertexAttrib4sNV = (PFNGLVERTEXATTRIB4SNVPROC)context->VertexAttrib4s;
    if (context->VertexAttrib4sNV == NULL && context->VertexAttrib4sARB != NULL) context->VertexAttrib4sNV = (PFNGLVERTEXATTRIB4SNVPROC)context->VertexAttrib4sARB;
    if (context->VertexAttrib4sv == NULL && context->VertexAttrib4svARB != NULL) context->VertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)context->VertexAttrib4svARB;
    if (context->VertexAttrib4sv == NULL && context->VertexAttrib4svNV != NULL) context->VertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)context->VertexAttrib4svNV;
    if (context->VertexAttrib4svARB == NULL && context->VertexAttrib4sv != NULL) context->VertexAttrib4svARB = (PFNGLVERTEXATTRIB4SVARBPROC)context->VertexAttrib4sv;
    if (context->VertexAttrib4svARB == NULL && context->VertexAttrib4svNV != NULL) context->VertexAttrib4svARB = (PFNGLVERTEXATTRIB4SVARBPROC)context->VertexAttrib4svNV;
    if (context->VertexAttrib4svNV == NULL && context->VertexAttrib4sv != NULL) context->VertexAttrib4svNV = (PFNGLVERTEXATTRIB4SVNVPROC)context->VertexAttrib4sv;
    if (context->VertexAttrib4svNV == NULL && context->VertexAttrib4svARB != NULL) context->VertexAttrib4svNV = (PFNGLVERTEXATTRIB4SVNVPROC)context->VertexAttrib4svARB;
    if (context->VertexAttrib4ubNV == NULL && context->VertexAttrib4Nub != NULL) context->VertexAttrib4ubNV = (PFNGLVERTEXATTRIB4UBNVPROC)context->VertexAttrib4Nub;
    if (context->VertexAttrib4ubNV == NULL && context->VertexAttrib4NubARB != NULL) context->VertexAttrib4ubNV = (PFNGLVERTEXATTRIB4UBNVPROC)context->VertexAttrib4NubARB;
    if (context->VertexAttrib4ubv == NULL && context->VertexAttrib4ubvARB != NULL) context->VertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)context->VertexAttrib4ubvARB;
    if (context->VertexAttrib4ubvARB == NULL && context->VertexAttrib4ubv != NULL) context->VertexAttrib4ubvARB = (PFNGLVERTEXATTRIB4UBVARBPROC)context->VertexAttrib4ubv;
    if (context->VertexAttrib4ubvNV == NULL && context->VertexAttrib4Nubv != NULL) context->VertexAttrib4ubvNV = (PFNGLVERTEXATTRIB4UBVNVPROC)context->VertexAttrib4Nubv;
    if (context->VertexAttrib4ubvNV == NULL && context->VertexAttrib4NubvARB != NULL) context->VertexAttrib4ubvNV = (PFNGLVERTEXATTRIB4UBVNVPROC)context->VertexAttrib4NubvARB;
    if (context->VertexAttrib4uiv == NULL && context->VertexAttrib4uivARB != NULL) context->VertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)context->VertexAttrib4uivARB;
    if (context->VertexAttrib4uivARB == NULL && context->VertexAttrib4uiv != NULL) context->VertexAttrib4uivARB = (PFNGLVERTEXATTRIB4UIVARBPROC)context->VertexAttrib4uiv;
    if (context->VertexAttrib4usv == NULL && context->VertexAttrib4usvARB != NULL) context->VertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)context->VertexAttrib4usvARB;
    if (context->VertexAttrib4usvARB == NULL && context->VertexAttrib4usv != NULL) context->VertexAttrib4usvARB = (PFNGLVERTEXATTRIB4USVARBPROC)context->VertexAttrib4usv;
    if (context->VertexAttribPointer == NULL && context->VertexAttribPointerARB != NULL) context->VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)context->VertexAttribPointerARB;
    if (context->VertexAttribPointerARB == NULL && context->VertexAttribPointer != NULL) context->VertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)context->VertexAttribPointer;
    if (context->WindowPos2d == NULL && context->WindowPos2dARB != NULL) context->WindowPos2d = (PFNGLWINDOWPOS2DPROC)context->WindowPos2dARB;
    if (context->WindowPos2d == NULL && context->WindowPos2dMESA != NULL) context->WindowPos2d = (PFNGLWINDOWPOS2DPROC)context->WindowPos2dMESA;
    if (context->WindowPos2dARB == NULL && context->WindowPos2d != NULL) context->WindowPos2dARB = (PFNGLWINDOWPOS2DARBPROC)context->WindowPos2d;
    if (context->WindowPos2dARB == NULL && context->WindowPos2dMESA != NULL) context->WindowPos2dARB = (PFNGLWINDOWPOS2DARBPROC)context->WindowPos2dMESA;
    if (context->WindowPos2dMESA == NULL && context->WindowPos2d != NULL) context->WindowPos2dMESA = (PFNGLWINDOWPOS2DMESAPROC)context->WindowPos2d;
    if (context->WindowPos2dMESA == NULL && context->WindowPos2dARB != NULL) context->WindowPos2dMESA = (PFNGLWINDOWPOS2DMESAPROC)context->WindowPos2dARB;
    if (context->WindowPos2dv == NULL && context->WindowPos2dvARB != NULL) context->WindowPos2dv = (PFNGLWINDOWPOS2DVPROC)context->WindowPos2dvARB;
    if (context->WindowPos2dv == NULL && context->WindowPos2dvMESA != NULL) context->WindowPos2dv = (PFNGLWINDOWPOS2DVPROC)context->WindowPos2dvMESA;
    if (context->WindowPos2dvARB == NULL && context->WindowPos2dv != NULL) context->WindowPos2dvARB = (PFNGLWINDOWPOS2DVARBPROC)context->WindowPos2dv;
    if (context->WindowPos2dvARB == NULL && context->WindowPos2dvMESA != NULL) context->WindowPos2dvARB = (PFNGLWINDOWPOS2DVARBPROC)context->WindowPos2dvMESA;
    if (context->WindowPos2dvMESA == NULL && context->WindowPos2dv != NULL) context->WindowPos2dvMESA = (PFNGLWINDOWPOS2DVMESAPROC)context->WindowPos2dv;
    if (context->WindowPos2dvMESA == NULL && context->WindowPos2dvARB != NULL) context->WindowPos2dvMESA = (PFNGLWINDOWPOS2DVMESAPROC)context->WindowPos2dvARB;
    if (context->WindowPos2f == NULL && context->WindowPos2fARB != NULL) context->WindowPos2f = (PFNGLWINDOWPOS2FPROC)context->WindowPos2fARB;
    if (context->WindowPos2f == NULL && context->WindowPos2fMESA != NULL) context->WindowPos2f = (PFNGLWINDOWPOS2FPROC)context->WindowPos2fMESA;
    if (context->WindowPos2fARB == NULL && context->WindowPos2f != NULL) context->WindowPos2fARB = (PFNGLWINDOWPOS2FARBPROC)context->WindowPos2f;
    if (context->WindowPos2fARB == NULL && context->WindowPos2fMESA != NULL) context->WindowPos2fARB = (PFNGLWINDOWPOS2FARBPROC)context->WindowPos2fMESA;
    if (context->WindowPos2fMESA == NULL && context->WindowPos2f != NULL) context->WindowPos2fMESA = (PFNGLWINDOWPOS2FMESAPROC)context->WindowPos2f;
    if (context->WindowPos2fMESA == NULL && context->WindowPos2fARB != NULL) context->WindowPos2fMESA = (PFNGLWINDOWPOS2FMESAPROC)context->WindowPos2fARB;
    if (context->WindowPos2fv == NULL && context->WindowPos2fvARB != NULL) context->WindowPos2fv = (PFNGLWINDOWPOS2FVPROC)context->WindowPos2fvARB;
    if (context->WindowPos2fv == NULL && context->WindowPos2fvMESA != NULL) context->WindowPos2fv = (PFNGLWINDOWPOS2FVPROC)context->WindowPos2fvMESA;
    if (context->WindowPos2fvARB == NULL && context->WindowPos2fv != NULL) context->WindowPos2fvARB = (PFNGLWINDOWPOS2FVARBPROC)context->WindowPos2fv;
    if (context->WindowPos2fvARB == NULL && context->WindowPos2fvMESA != NULL) context->WindowPos2fvARB = (PFNGLWINDOWPOS2FVARBPROC)context->WindowPos2fvMESA;
    if (context->WindowPos2fvMESA == NULL && context->WindowPos2fv != NULL) context->WindowPos2fvMESA = (PFNGLWINDOWPOS2FVMESAPROC)context->WindowPos2fv;
    if (context->WindowPos2fvMESA == NULL && context->WindowPos2fvARB != NULL) context->WindowPos2fvMESA = (PFNGLWINDOWPOS2FVMESAPROC)context->WindowPos2fvARB;
    if (context->WindowPos2i == NULL && context->WindowPos2iARB != NULL) context->WindowPos2i = (PFNGLWINDOWPOS2IPROC)context->WindowPos2iARB;
    if (context->WindowPos2i == NULL && context->WindowPos2iMESA != NULL) context->WindowPos2i = (PFNGLWINDOWPOS2IPROC)context->WindowPos2iMESA;
    if (context->WindowPos2iARB == NULL && context->WindowPos2i != NULL) context->WindowPos2iARB = (PFNGLWINDOWPOS2IARBPROC)context->WindowPos2i;
    if (context->WindowPos2iARB == NULL && context->WindowPos2iMESA != NULL) context->WindowPos2iARB = (PFNGLWINDOWPOS2IARBPROC)context->WindowPos2iMESA;
    if (context->WindowPos2iMESA == NULL && context->WindowPos2i != NULL) context->WindowPos2iMESA = (PFNGLWINDOWPOS2IMESAPROC)context->WindowPos2i;
    if (context->WindowPos2iMESA == NULL && context->WindowPos2iARB != NULL) context->WindowPos2iMESA = (PFNGLWINDOWPOS2IMESAPROC)context->WindowPos2iARB;
    if (context->WindowPos2iv == NULL && context->WindowPos2ivARB != NULL) context->WindowPos2iv = (PFNGLWINDOWPOS2IVPROC)context->WindowPos2ivARB;
    if (context->WindowPos2iv == NULL && context->WindowPos2ivMESA != NULL) context->WindowPos2iv = (PFNGLWINDOWPOS2IVPROC)context->WindowPos2ivMESA;
    if (context->WindowPos2ivARB == NULL && context->WindowPos2iv != NULL) context->WindowPos2ivARB = (PFNGLWINDOWPOS2IVARBPROC)context->WindowPos2iv;
    if (context->WindowPos2ivARB == NULL && context->WindowPos2ivMESA != NULL) context->WindowPos2ivARB = (PFNGLWINDOWPOS2IVARBPROC)context->WindowPos2ivMESA;
    if (context->WindowPos2ivMESA == NULL && context->WindowPos2iv != NULL) context->WindowPos2ivMESA = (PFNGLWINDOWPOS2IVMESAPROC)context->WindowPos2iv;
    if (context->WindowPos2ivMESA == NULL && context->WindowPos2ivARB != NULL) context->WindowPos2ivMESA = (PFNGLWINDOWPOS2IVMESAPROC)context->WindowPos2ivARB;
    if (context->WindowPos2s == NULL && context->WindowPos2sARB != NULL) context->WindowPos2s = (PFNGLWINDOWPOS2SPROC)context->WindowPos2sARB;
    if (context->WindowPos2s == NULL && context->WindowPos2sMESA != NULL) context->WindowPos2s = (PFNGLWINDOWPOS2SPROC)context->WindowPos2sMESA;
    if (context->WindowPos2sARB == NULL && context->WindowPos2s != NULL) context->WindowPos2sARB = (PFNGLWINDOWPOS2SARBPROC)context->WindowPos2s;
    if (context->WindowPos2sARB == NULL && context->WindowPos2sMESA != NULL) context->WindowPos2sARB = (PFNGLWINDOWPOS2SARBPROC)context->WindowPos2sMESA;
    if (context->WindowPos2sMESA == NULL && context->WindowPos2s != NULL) context->WindowPos2sMESA = (PFNGLWINDOWPOS2SMESAPROC)context->WindowPos2s;
    if (context->WindowPos2sMESA == NULL && context->WindowPos2sARB != NULL) context->WindowPos2sMESA = (PFNGLWINDOWPOS2SMESAPROC)context->WindowPos2sARB;
    if (context->WindowPos2sv == NULL && context->WindowPos2svARB != NULL) context->WindowPos2sv = (PFNGLWINDOWPOS2SVPROC)context->WindowPos2svARB;
    if (context->WindowPos2sv == NULL && context->WindowPos2svMESA != NULL) context->WindowPos2sv = (PFNGLWINDOWPOS2SVPROC)context->WindowPos2svMESA;
    if (context->WindowPos2svARB == NULL && context->WindowPos2sv != NULL) context->WindowPos2svARB = (PFNGLWINDOWPOS2SVARBPROC)context->WindowPos2sv;
    if (context->WindowPos2svARB == NULL && context->WindowPos2svMESA != NULL) context->WindowPos2svARB = (PFNGLWINDOWPOS2SVARBPROC)context->WindowPos2svMESA;
    if (context->WindowPos2svMESA == NULL && context->WindowPos2sv != NULL) context->WindowPos2svMESA = (PFNGLWINDOWPOS2SVMESAPROC)context->WindowPos2sv;
    if (context->WindowPos2svMESA == NULL && context->WindowPos2svARB != NULL) context->WindowPos2svMESA = (PFNGLWINDOWPOS2SVMESAPROC)context->WindowPos2svARB;
    if (context->WindowPos3d == NULL && context->WindowPos3dARB != NULL) context->WindowPos3d = (PFNGLWINDOWPOS3DPROC)context->WindowPos3dARB;
    if (context->WindowPos3d == NULL && context->WindowPos3dMESA != NULL) context->WindowPos3d = (PFNGLWINDOWPOS3DPROC)context->WindowPos3dMESA;
    if (context->WindowPos3dARB == NULL && context->WindowPos3d != NULL) context->WindowPos3dARB = (PFNGLWINDOWPOS3DARBPROC)context->WindowPos3d;
    if (context->WindowPos3dARB == NULL && context->WindowPos3dMESA != NULL) context->WindowPos3dARB = (PFNGLWINDOWPOS3DARBPROC)context->WindowPos3dMESA;
    if (context->WindowPos3dMESA == NULL && context->WindowPos3d != NULL) context->WindowPos3dMESA = (PFNGLWINDOWPOS3DMESAPROC)context->WindowPos3d;
    if (context->WindowPos3dMESA == NULL && context->WindowPos3dARB != NULL) context->WindowPos3dMESA = (PFNGLWINDOWPOS3DMESAPROC)context->WindowPos3dARB;
    if (context->WindowPos3dv == NULL && context->WindowPos3dvARB != NULL) context->WindowPos3dv = (PFNGLWINDOWPOS3DVPROC)context->WindowPos3dvARB;
    if (context->WindowPos3dv == NULL && context->WindowPos3dvMESA != NULL) context->WindowPos3dv = (PFNGLWINDOWPOS3DVPROC)context->WindowPos3dvMESA;
    if (context->WindowPos3dvARB == NULL && context->WindowPos3dv != NULL) context->WindowPos3dvARB = (PFNGLWINDOWPOS3DVARBPROC)context->WindowPos3dv;
    if (context->WindowPos3dvARB == NULL && context->WindowPos3dvMESA != NULL) context->WindowPos3dvARB = (PFNGLWINDOWPOS3DVARBPROC)context->WindowPos3dvMESA;
    if (context->WindowPos3dvMESA == NULL && context->WindowPos3dv != NULL) context->WindowPos3dvMESA = (PFNGLWINDOWPOS3DVMESAPROC)context->WindowPos3dv;
    if (context->WindowPos3dvMESA == NULL && context->WindowPos3dvARB != NULL) context->WindowPos3dvMESA = (PFNGLWINDOWPOS3DVMESAPROC)context->WindowPos3dvARB;
    if (context->WindowPos3f == NULL && context->WindowPos3fARB != NULL) context->WindowPos3f = (PFNGLWINDOWPOS3FPROC)context->WindowPos3fARB;
    if (context->WindowPos3f == NULL && context->WindowPos3fMESA != NULL) context->WindowPos3f = (PFNGLWINDOWPOS3FPROC)context->WindowPos3fMESA;
    if (context->WindowPos3fARB == NULL && context->WindowPos3f != NULL) context->WindowPos3fARB = (PFNGLWINDOWPOS3FARBPROC)context->WindowPos3f;
    if (context->WindowPos3fARB == NULL && context->WindowPos3fMESA != NULL) context->WindowPos3fARB = (PFNGLWINDOWPOS3FARBPROC)context->WindowPos3fMESA;
    if (context->WindowPos3fMESA == NULL && context->WindowPos3f != NULL) context->WindowPos3fMESA = (PFNGLWINDOWPOS3FMESAPROC)context->WindowPos3f;
    if (context->WindowPos3fMESA == NULL && context->WindowPos3fARB != NULL) context->WindowPos3fMESA = (PFNGLWINDOWPOS3FMESAPROC)context->WindowPos3fARB;
    if (context->WindowPos3fv == NULL && context->WindowPos3fvARB != NULL) context->WindowPos3fv = (PFNGLWINDOWPOS3FVPROC)context->WindowPos3fvARB;
    if (context->WindowPos3fv == NULL && context->WindowPos3fvMESA != NULL) context->WindowPos3fv = (PFNGLWINDOWPOS3FVPROC)context->WindowPos3fvMESA;
    if (context->WindowPos3fvARB == NULL && context->WindowPos3fv != NULL) context->WindowPos3fvARB = (PFNGLWINDOWPOS3FVARBPROC)context->WindowPos3fv;
    if (context->WindowPos3fvARB == NULL && context->WindowPos3fvMESA != NULL) context->WindowPos3fvARB = (PFNGLWINDOWPOS3FVARBPROC)context->WindowPos3fvMESA;
    if (context->WindowPos3fvMESA == NULL && context->WindowPos3fv != NULL) context->WindowPos3fvMESA = (PFNGLWINDOWPOS3FVMESAPROC)context->WindowPos3fv;
    if (context->WindowPos3fvMESA == NULL && context->WindowPos3fvARB != NULL) context->WindowPos3fvMESA = (PFNGLWINDOWPOS3FVMESAPROC)context->WindowPos3fvARB;
    if (context->WindowPos3i == NULL && context->WindowPos3iARB != NULL) context->WindowPos3i = (PFNGLWINDOWPOS3IPROC)context->WindowPos3iARB;
    if (context->WindowPos3i == NULL && context->WindowPos3iMESA != NULL) context->WindowPos3i = (PFNGLWINDOWPOS3IPROC)context->WindowPos3iMESA;
    if (context->WindowPos3iARB == NULL && context->WindowPos3i != NULL) context->WindowPos3iARB = (PFNGLWINDOWPOS3IARBPROC)context->WindowPos3i;
    if (context->WindowPos3iARB == NULL && context->WindowPos3iMESA != NULL) context->WindowPos3iARB = (PFNGLWINDOWPOS3IARBPROC)context->WindowPos3iMESA;
    if (context->WindowPos3iMESA == NULL && context->WindowPos3i != NULL) context->WindowPos3iMESA = (PFNGLWINDOWPOS3IMESAPROC)context->WindowPos3i;
    if (context->WindowPos3iMESA == NULL && context->WindowPos3iARB != NULL) context->WindowPos3iMESA = (PFNGLWINDOWPOS3IMESAPROC)context->WindowPos3iARB;
    if (context->WindowPos3iv == NULL && context->WindowPos3ivARB != NULL) context->WindowPos3iv = (PFNGLWINDOWPOS3IVPROC)context->WindowPos3ivARB;
    if (context->WindowPos3iv == NULL && context->WindowPos3ivMESA != NULL) context->WindowPos3iv = (PFNGLWINDOWPOS3IVPROC)context->WindowPos3ivMESA;
    if (context->WindowPos3ivARB == NULL && context->WindowPos3iv != NULL) context->WindowPos3ivARB = (PFNGLWINDOWPOS3IVARBPROC)context->WindowPos3iv;
    if (context->WindowPos3ivARB == NULL && context->WindowPos3ivMESA != NULL) context->WindowPos3ivARB = (PFNGLWINDOWPOS3IVARBPROC)context->WindowPos3ivMESA;
    if (context->WindowPos3ivMESA == NULL && context->WindowPos3iv != NULL) context->WindowPos3ivMESA = (PFNGLWINDOWPOS3IVMESAPROC)context->WindowPos3iv;
    if (context->WindowPos3ivMESA == NULL && context->WindowPos3ivARB != NULL) context->WindowPos3ivMESA = (PFNGLWINDOWPOS3IVMESAPROC)context->WindowPos3ivARB;
    if (context->WindowPos3s == NULL && context->WindowPos3sARB != NULL) context->WindowPos3s = (PFNGLWINDOWPOS3SPROC)context->WindowPos3sARB;
    if (context->WindowPos3s == NULL && context->WindowPos3sMESA != NULL) context->WindowPos3s = (PFNGLWINDOWPOS3SPROC)context->WindowPos3sMESA;
    if (context->WindowPos3sARB == NULL && context->WindowPos3s != NULL) context->WindowPos3sARB = (PFNGLWINDOWPOS3SARBPROC)context->WindowPos3s;
    if (context->WindowPos3sARB == NULL && context->WindowPos3sMESA != NULL) context->WindowPos3sARB = (PFNGLWINDOWPOS3SARBPROC)context->WindowPos3sMESA;
    if (context->WindowPos3sMESA == NULL && context->WindowPos3s != NULL) context->WindowPos3sMESA = (PFNGLWINDOWPOS3SMESAPROC)context->WindowPos3s;
    if (context->WindowPos3sMESA == NULL && context->WindowPos3sARB != NULL) context->WindowPos3sMESA = (PFNGLWINDOWPOS3SMESAPROC)context->WindowPos3sARB;
    if (context->WindowPos3sv == NULL && context->WindowPos3svARB != NULL) context->WindowPos3sv = (PFNGLWINDOWPOS3SVPROC)context->WindowPos3svARB;
    if (context->WindowPos3sv == NULL && context->WindowPos3svMESA != NULL) context->WindowPos3sv = (PFNGLWINDOWPOS3SVPROC)context->WindowPos3svMESA;
    if (context->WindowPos3svARB == NULL && context->WindowPos3sv != NULL) context->WindowPos3svARB = (PFNGLWINDOWPOS3SVARBPROC)context->WindowPos3sv;
    if (context->WindowPos3svARB == NULL && context->WindowPos3svMESA != NULL) context->WindowPos3svARB = (PFNGLWINDOWPOS3SVARBPROC)context->WindowPos3svMESA;
    if (context->WindowPos3svMESA == NULL && context->WindowPos3sv != NULL) context->WindowPos3svMESA = (PFNGLWINDOWPOS3SVMESAPROC)context->WindowPos3sv;
    if (context->WindowPos3svMESA == NULL && context->WindowPos3svARB != NULL) context->WindowPos3svMESA = (PFNGLWINDOWPOS3SVMESAPROC)context->WindowPos3svARB;
}

#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_3_0)
#define GLAD_GL_IS_SOME_NEW_VERSION 1
#else
#define GLAD_GL_IS_SOME_NEW_VERSION 0
#endif

static int glad_gl_get_extensions(GladGLContext *context, int version, const char **out_exts, unsigned int *out_num_exts_i, char ***out_exts_i) {
#if GLAD_GL_IS_SOME_NEW_VERSION
    if(GLAD_VERSION_MAJOR(version) < 3) {
#else
    GLAD_UNUSED(version);
    GLAD_UNUSED(out_num_exts_i);
    GLAD_UNUSED(out_exts_i);
#endif
        if (context->GetString == NULL) {
            return 0;
        }
        *out_exts = (const char *)context->GetString(GL_EXTENSIONS);
#if GLAD_GL_IS_SOME_NEW_VERSION
    } else {
        unsigned int index = 0;
        unsigned int num_exts_i = 0;
        char **exts_i = NULL;
        if (context->GetStringi == NULL || context->GetIntegerv == NULL) {
            return 0;
        }
        context->GetIntegerv(GL_NUM_EXTENSIONS, (int*) &num_exts_i);
        if (num_exts_i > 0) {
            exts_i = (char **) malloc(num_exts_i * (sizeof *exts_i));
        }
        if (exts_i == NULL) {
            return 0;
        }
        for(index = 0; index < num_exts_i; index++) {
            const char *gl_str_tmp = (const char*) context->GetStringi(GL_EXTENSIONS, index);
            size_t len = strlen(gl_str_tmp) + 1;

            char *local_str = (char*) malloc(len * sizeof(char));
            if(local_str != NULL) {
                memcpy(local_str, gl_str_tmp, len * sizeof(char));
            }

            exts_i[index] = local_str;
        }

        *out_num_exts_i = num_exts_i;
        *out_exts_i = exts_i;
    }
#endif
    return 1;
}
static void glad_gl_free_extensions(char **exts_i, unsigned int num_exts_i) {
    if (exts_i != NULL) {
        unsigned int index;
        for(index = 0; index < num_exts_i; index++) {
            free((void *) (exts_i[index]));
        }
        free((void *)exts_i);
        exts_i = NULL;
    }
}
static int glad_gl_has_extension(int version, const char *exts, unsigned int num_exts_i, char **exts_i, const char *ext) {
    if(GLAD_VERSION_MAJOR(version) < 3 || !GLAD_GL_IS_SOME_NEW_VERSION) {
        const char *extensions;
        const char *loc;
        const char *terminator;
        extensions = exts;
        if(extensions == NULL || ext == NULL) {
            return 0;
        }
        while(1) {
            loc = strstr(extensions, ext);
            if(loc == NULL) {
                return 0;
            }
            terminator = loc + strlen(ext);
            if((loc == extensions || *(loc - 1) == ' ') &&
                (*terminator == ' ' || *terminator == '\0')) {
                return 1;
            }
            extensions = terminator;
        }
    } else {
        unsigned int index;
        for(index = 0; index < num_exts_i; index++) {
            const char *e = exts_i[index];
            if(strcmp(e, ext) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

static GLADapiproc glad_gl_get_proc_from_userptr(void *userptr, const char* name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

static int glad_gl_find_extensions_gl(GladGLContext *context, int version) {
    const char *exts = NULL;
    unsigned int num_exts_i = 0;
    char **exts_i = NULL;
    if (!glad_gl_get_extensions(context, version, &exts, &num_exts_i, &exts_i)) return 0;

    context->ARB_draw_buffers = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_draw_buffers");
    context->ARB_framebuffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_framebuffer_object");
    context->ARB_geometry_shader4 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_geometry_shader4");
    context->ARB_imaging = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_imaging");
    context->ARB_multisample = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_multisample");
    context->ARB_multitexture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_multitexture");
    context->ARB_occlusion_query = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_occlusion_query");
    context->ARB_point_parameters = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_point_parameters");
    context->ARB_shader_objects = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_shader_objects");
    context->ARB_texture_compression = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_texture_compression");
    context->ARB_transpose_matrix = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_transpose_matrix");
    context->ARB_vertex_buffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_vertex_buffer_object");
    context->ARB_vertex_program = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_vertex_program");
    context->ARB_vertex_shader = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_vertex_shader");
    context->ARB_window_pos = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_window_pos");
    context->ATI_draw_buffers = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ATI_draw_buffers");
    context->ATI_separate_stencil = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ATI_separate_stencil");
    context->EXT_blend_color = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_blend_color");
    context->EXT_blend_equation_separate = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_blend_equation_separate");
    context->EXT_blend_func_separate = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_blend_func_separate");
    context->EXT_blend_minmax = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_blend_minmax");
    context->EXT_copy_texture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_copy_texture");
    context->EXT_draw_range_elements = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_draw_range_elements");
    context->EXT_fog_coord = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_fog_coord");
    context->EXT_framebuffer_blit = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_framebuffer_blit");
    context->EXT_framebuffer_multisample = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_framebuffer_multisample");
    context->EXT_framebuffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_framebuffer_object");
    context->EXT_multi_draw_arrays = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_multi_draw_arrays");
    context->EXT_point_parameters = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_point_parameters");
    context->EXT_secondary_color = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_secondary_color");
    context->EXT_subtexture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_subtexture");
    context->EXT_texture3D = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture3D");
    context->EXT_texture_array = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_array");
    context->EXT_texture_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_object");
    context->EXT_vertex_array = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_vertex_array");
    context->INGR_blend_func_separate = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_INGR_blend_func_separate");
    context->KHR_debug = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_KHR_debug");
    context->MESA_window_pos = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_MESA_window_pos");
    context->NV_geometry_program4 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_geometry_program4");
    context->NV_point_sprite = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_point_sprite");
    context->NV_vertex_program = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_vertex_program");
    context->SGIS_point_parameters = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_SGIS_point_parameters");

    glad_gl_free_extensions(exts_i, num_exts_i);

    return 1;
}

static int glad_gl_find_core_gl(GladGLContext *context) {
    int i;
    const char* version;
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        "OpenGL SC ",
        NULL
    };
    int major = 0;
    int minor = 0;
    version = (const char*) context->GetString(GL_VERSION);
    if (!version) return 0;
    for (i = 0;  prefixes[i];  i++) {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0) {
            version += length;
            break;
        }
    }

    GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);

    context->VERSION_1_0 = (major == 1 && minor >= 0) || major > 1;
    context->VERSION_1_1 = (major == 1 && minor >= 1) || major > 1;
    context->VERSION_1_2 = (major == 1 && minor >= 2) || major > 1;
    context->VERSION_1_3 = (major == 1 && minor >= 3) || major > 1;
    context->VERSION_1_4 = (major == 1 && minor >= 4) || major > 1;
    context->VERSION_1_5 = (major == 1 && minor >= 5) || major > 1;
    context->VERSION_2_0 = (major == 2 && minor >= 0) || major > 2;

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoadGLContextUserPtr(GladGLContext *context, GLADuserptrloadfunc load, void *userptr) {
    int version;

    context->GetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    if(context->GetString == NULL) return 0;
    if(context->GetString(GL_VERSION) == NULL) return 0;
    version = glad_gl_find_core_gl(context);

    glad_gl_load_GL_VERSION_1_0(context, load, userptr);
    glad_gl_load_GL_VERSION_1_1(context, load, userptr);
    glad_gl_load_GL_VERSION_1_2(context, load, userptr);
    glad_gl_load_GL_VERSION_1_3(context, load, userptr);
    glad_gl_load_GL_VERSION_1_4(context, load, userptr);
    glad_gl_load_GL_VERSION_1_5(context, load, userptr);
    glad_gl_load_GL_VERSION_2_0(context, load, userptr);

    if (!glad_gl_find_extensions_gl(context, version)) return 0;
    glad_gl_load_GL_ARB_draw_buffers(context, load, userptr);
    glad_gl_load_GL_ARB_framebuffer_object(context, load, userptr);
    glad_gl_load_GL_ARB_geometry_shader4(context, load, userptr);
    glad_gl_load_GL_ARB_imaging(context, load, userptr);
    glad_gl_load_GL_ARB_multisample(context, load, userptr);
    glad_gl_load_GL_ARB_multitexture(context, load, userptr);
    glad_gl_load_GL_ARB_occlusion_query(context, load, userptr);
    glad_gl_load_GL_ARB_point_parameters(context, load, userptr);
    glad_gl_load_GL_ARB_shader_objects(context, load, userptr);
    glad_gl_load_GL_ARB_texture_compression(context, load, userptr);
    glad_gl_load_GL_ARB_transpose_matrix(context, load, userptr);
    glad_gl_load_GL_ARB_vertex_buffer_object(context, load, userptr);
    glad_gl_load_GL_ARB_vertex_program(context, load, userptr);
    glad_gl_load_GL_ARB_vertex_shader(context, load, userptr);
    glad_gl_load_GL_ARB_window_pos(context, load, userptr);
    glad_gl_load_GL_ATI_draw_buffers(context, load, userptr);
    glad_gl_load_GL_ATI_separate_stencil(context, load, userptr);
    glad_gl_load_GL_EXT_blend_color(context, load, userptr);
    glad_gl_load_GL_EXT_blend_equation_separate(context, load, userptr);
    glad_gl_load_GL_EXT_blend_func_separate(context, load, userptr);
    glad_gl_load_GL_EXT_blend_minmax(context, load, userptr);
    glad_gl_load_GL_EXT_copy_texture(context, load, userptr);
    glad_gl_load_GL_EXT_draw_range_elements(context, load, userptr);
    glad_gl_load_GL_EXT_fog_coord(context, load, userptr);
    glad_gl_load_GL_EXT_framebuffer_blit(context, load, userptr);
    glad_gl_load_GL_EXT_framebuffer_multisample(context, load, userptr);
    glad_gl_load_GL_EXT_framebuffer_object(context, load, userptr);
    glad_gl_load_GL_EXT_multi_draw_arrays(context, load, userptr);
    glad_gl_load_GL_EXT_point_parameters(context, load, userptr);
    glad_gl_load_GL_EXT_secondary_color(context, load, userptr);
    glad_gl_load_GL_EXT_subtexture(context, load, userptr);
    glad_gl_load_GL_EXT_texture3D(context, load, userptr);
    glad_gl_load_GL_EXT_texture_array(context, load, userptr);
    glad_gl_load_GL_EXT_texture_object(context, load, userptr);
    glad_gl_load_GL_EXT_vertex_array(context, load, userptr);
    glad_gl_load_GL_INGR_blend_func_separate(context, load, userptr);
    glad_gl_load_GL_KHR_debug(context, load, userptr);
    glad_gl_load_GL_MESA_window_pos(context, load, userptr);
    glad_gl_load_GL_NV_geometry_program4(context, load, userptr);
    glad_gl_load_GL_NV_point_sprite(context, load, userptr);
    glad_gl_load_GL_NV_vertex_program(context, load, userptr);
    glad_gl_load_GL_SGIS_point_parameters(context, load, userptr);


    glad_gl_resolve_aliases(context);

    return version;
}


int gladLoadGLContext(GladGLContext *context, GLADloadfunc load) {
    return gladLoadGLContextUserPtr(context, glad_gl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}



 


#ifdef __cplusplus
}
#endif
