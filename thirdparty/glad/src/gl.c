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
    context->BlendFunc = (PFNGLBLENDFUNCPROC) load(userptr, "glBlendFunc");
    context->Clear = (PFNGLCLEARPROC) load(userptr, "glClear");
    context->ClearColor = (PFNGLCLEARCOLORPROC) load(userptr, "glClearColor");
    context->ClearDepth = (PFNGLCLEARDEPTHPROC) load(userptr, "glClearDepth");
    context->ClearStencil = (PFNGLCLEARSTENCILPROC) load(userptr, "glClearStencil");
    context->ColorMask = (PFNGLCOLORMASKPROC) load(userptr, "glColorMask");
    context->CullFace = (PFNGLCULLFACEPROC) load(userptr, "glCullFace");
    context->DepthFunc = (PFNGLDEPTHFUNCPROC) load(userptr, "glDepthFunc");
    context->DepthMask = (PFNGLDEPTHMASKPROC) load(userptr, "glDepthMask");
    context->DepthRange = (PFNGLDEPTHRANGEPROC) load(userptr, "glDepthRange");
    context->Disable = (PFNGLDISABLEPROC) load(userptr, "glDisable");
    context->DrawBuffer = (PFNGLDRAWBUFFERPROC) load(userptr, "glDrawBuffer");
    context->Enable = (PFNGLENABLEPROC) load(userptr, "glEnable");
    context->Finish = (PFNGLFINISHPROC) load(userptr, "glFinish");
    context->Flush = (PFNGLFLUSHPROC) load(userptr, "glFlush");
    context->FrontFace = (PFNGLFRONTFACEPROC) load(userptr, "glFrontFace");
    context->GetBooleanv = (PFNGLGETBOOLEANVPROC) load(userptr, "glGetBooleanv");
    context->GetDoublev = (PFNGLGETDOUBLEVPROC) load(userptr, "glGetDoublev");
    context->GetError = (PFNGLGETERRORPROC) load(userptr, "glGetError");
    context->GetFloatv = (PFNGLGETFLOATVPROC) load(userptr, "glGetFloatv");
    context->GetIntegerv = (PFNGLGETINTEGERVPROC) load(userptr, "glGetIntegerv");
    context->GetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    context->GetTexImage = (PFNGLGETTEXIMAGEPROC) load(userptr, "glGetTexImage");
    context->GetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC) load(userptr, "glGetTexLevelParameterfv");
    context->GetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC) load(userptr, "glGetTexLevelParameteriv");
    context->GetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC) load(userptr, "glGetTexParameterfv");
    context->GetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC) load(userptr, "glGetTexParameteriv");
    context->Hint = (PFNGLHINTPROC) load(userptr, "glHint");
    context->IsEnabled = (PFNGLISENABLEDPROC) load(userptr, "glIsEnabled");
    context->LineWidth = (PFNGLLINEWIDTHPROC) load(userptr, "glLineWidth");
    context->LogicOp = (PFNGLLOGICOPPROC) load(userptr, "glLogicOp");
    context->PixelStoref = (PFNGLPIXELSTOREFPROC) load(userptr, "glPixelStoref");
    context->PixelStorei = (PFNGLPIXELSTOREIPROC) load(userptr, "glPixelStorei");
    context->PointSize = (PFNGLPOINTSIZEPROC) load(userptr, "glPointSize");
    context->PolygonMode = (PFNGLPOLYGONMODEPROC) load(userptr, "glPolygonMode");
    context->ReadBuffer = (PFNGLREADBUFFERPROC) load(userptr, "glReadBuffer");
    context->ReadPixels = (PFNGLREADPIXELSPROC) load(userptr, "glReadPixels");
    context->Scissor = (PFNGLSCISSORPROC) load(userptr, "glScissor");
    context->StencilFunc = (PFNGLSTENCILFUNCPROC) load(userptr, "glStencilFunc");
    context->StencilMask = (PFNGLSTENCILMASKPROC) load(userptr, "glStencilMask");
    context->StencilOp = (PFNGLSTENCILOPPROC) load(userptr, "glStencilOp");
    context->TexImage1D = (PFNGLTEXIMAGE1DPROC) load(userptr, "glTexImage1D");
    context->TexImage2D = (PFNGLTEXIMAGE2DPROC) load(userptr, "glTexImage2D");
    context->TexParameterf = (PFNGLTEXPARAMETERFPROC) load(userptr, "glTexParameterf");
    context->TexParameterfv = (PFNGLTEXPARAMETERFVPROC) load(userptr, "glTexParameterfv");
    context->TexParameteri = (PFNGLTEXPARAMETERIPROC) load(userptr, "glTexParameteri");
    context->TexParameteriv = (PFNGLTEXPARAMETERIVPROC) load(userptr, "glTexParameteriv");
    context->Viewport = (PFNGLVIEWPORTPROC) load(userptr, "glViewport");
}
static void glad_gl_load_GL_VERSION_1_1(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_1_1) return;
    context->BindTexture = (PFNGLBINDTEXTUREPROC) load(userptr, "glBindTexture");
    context->CopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC) load(userptr, "glCopyTexImage1D");
    context->CopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC) load(userptr, "glCopyTexImage2D");
    context->CopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC) load(userptr, "glCopyTexSubImage1D");
    context->CopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC) load(userptr, "glCopyTexSubImage2D");
    context->DeleteTextures = (PFNGLDELETETEXTURESPROC) load(userptr, "glDeleteTextures");
    context->DrawArrays = (PFNGLDRAWARRAYSPROC) load(userptr, "glDrawArrays");
    context->DrawElements = (PFNGLDRAWELEMENTSPROC) load(userptr, "glDrawElements");
    context->GenTextures = (PFNGLGENTEXTURESPROC) load(userptr, "glGenTextures");
    context->IsTexture = (PFNGLISTEXTUREPROC) load(userptr, "glIsTexture");
    context->PolygonOffset = (PFNGLPOLYGONOFFSETPROC) load(userptr, "glPolygonOffset");
    context->TexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC) load(userptr, "glTexSubImage1D");
    context->TexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC) load(userptr, "glTexSubImage2D");
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
    context->CompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC) load(userptr, "glCompressedTexImage1D");
    context->CompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) load(userptr, "glCompressedTexImage2D");
    context->CompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC) load(userptr, "glCompressedTexImage3D");
    context->CompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC) load(userptr, "glCompressedTexSubImage1D");
    context->CompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) load(userptr, "glCompressedTexSubImage2D");
    context->CompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC) load(userptr, "glCompressedTexSubImage3D");
    context->GetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC) load(userptr, "glGetCompressedTexImage");
    context->SampleCoverage = (PFNGLSAMPLECOVERAGEPROC) load(userptr, "glSampleCoverage");
}
static void glad_gl_load_GL_VERSION_1_4(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_1_4) return;
    context->BlendColor = (PFNGLBLENDCOLORPROC) load(userptr, "glBlendColor");
    context->BlendEquation = (PFNGLBLENDEQUATIONPROC) load(userptr, "glBlendEquation");
    context->BlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC) load(userptr, "glBlendFuncSeparate");
    context->MultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC) load(userptr, "glMultiDrawArrays");
    context->MultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC) load(userptr, "glMultiDrawElements");
    context->PointParameterf = (PFNGLPOINTPARAMETERFPROC) load(userptr, "glPointParameterf");
    context->PointParameterfv = (PFNGLPOINTPARAMETERFVPROC) load(userptr, "glPointParameterfv");
    context->PointParameteri = (PFNGLPOINTPARAMETERIPROC) load(userptr, "glPointParameteri");
    context->PointParameteriv = (PFNGLPOINTPARAMETERIVPROC) load(userptr, "glPointParameteriv");
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
static void glad_gl_load_GL_VERSION_2_1(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_2_1) return;
    context->UniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC) load(userptr, "glUniformMatrix2x3fv");
    context->UniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC) load(userptr, "glUniformMatrix2x4fv");
    context->UniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC) load(userptr, "glUniformMatrix3x2fv");
    context->UniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC) load(userptr, "glUniformMatrix3x4fv");
    context->UniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC) load(userptr, "glUniformMatrix4x2fv");
    context->UniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC) load(userptr, "glUniformMatrix4x3fv");
}
static void glad_gl_load_GL_VERSION_3_0(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_3_0) return;
    context->BeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC) load(userptr, "glBeginConditionalRender");
    context->BeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC) load(userptr, "glBeginTransformFeedback");
    context->BindBufferBase = (PFNGLBINDBUFFERBASEPROC) load(userptr, "glBindBufferBase");
    context->BindBufferRange = (PFNGLBINDBUFFERRANGEPROC) load(userptr, "glBindBufferRange");
    context->BindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC) load(userptr, "glBindFragDataLocation");
    context->BindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) load(userptr, "glBindFramebuffer");
    context->BindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) load(userptr, "glBindRenderbuffer");
    context->BindVertexArray = (PFNGLBINDVERTEXARRAYPROC) load(userptr, "glBindVertexArray");
    context->BlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC) load(userptr, "glBlitFramebuffer");
    context->CheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) load(userptr, "glCheckFramebufferStatus");
    context->ClampColor = (PFNGLCLAMPCOLORPROC) load(userptr, "glClampColor");
    context->ClearBufferfi = (PFNGLCLEARBUFFERFIPROC) load(userptr, "glClearBufferfi");
    context->ClearBufferfv = (PFNGLCLEARBUFFERFVPROC) load(userptr, "glClearBufferfv");
    context->ClearBufferiv = (PFNGLCLEARBUFFERIVPROC) load(userptr, "glClearBufferiv");
    context->ClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC) load(userptr, "glClearBufferuiv");
    context->ColorMaski = (PFNGLCOLORMASKIPROC) load(userptr, "glColorMaski");
    context->DeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) load(userptr, "glDeleteFramebuffers");
    context->DeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) load(userptr, "glDeleteRenderbuffers");
    context->DeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) load(userptr, "glDeleteVertexArrays");
    context->Disablei = (PFNGLDISABLEIPROC) load(userptr, "glDisablei");
    context->Enablei = (PFNGLENABLEIPROC) load(userptr, "glEnablei");
    context->EndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC) load(userptr, "glEndConditionalRender");
    context->EndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC) load(userptr, "glEndTransformFeedback");
    context->FlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC) load(userptr, "glFlushMappedBufferRange");
    context->FramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) load(userptr, "glFramebufferRenderbuffer");
    context->FramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC) load(userptr, "glFramebufferTexture1D");
    context->FramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) load(userptr, "glFramebufferTexture2D");
    context->FramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC) load(userptr, "glFramebufferTexture3D");
    context->FramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC) load(userptr, "glFramebufferTextureLayer");
    context->GenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) load(userptr, "glGenFramebuffers");
    context->GenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) load(userptr, "glGenRenderbuffers");
    context->GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) load(userptr, "glGenVertexArrays");
    context->GenerateMipmap = (PFNGLGENERATEMIPMAPPROC) load(userptr, "glGenerateMipmap");
    context->GetBooleani_v = (PFNGLGETBOOLEANI_VPROC) load(userptr, "glGetBooleani_v");
    context->GetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC) load(userptr, "glGetFragDataLocation");
    context->GetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) load(userptr, "glGetFramebufferAttachmentParameteriv");
    context->GetIntegeri_v = (PFNGLGETINTEGERI_VPROC) load(userptr, "glGetIntegeri_v");
    context->GetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) load(userptr, "glGetRenderbufferParameteriv");
    context->GetStringi = (PFNGLGETSTRINGIPROC) load(userptr, "glGetStringi");
    context->GetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC) load(userptr, "glGetTexParameterIiv");
    context->GetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC) load(userptr, "glGetTexParameterIuiv");
    context->GetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC) load(userptr, "glGetTransformFeedbackVarying");
    context->GetUniformuiv = (PFNGLGETUNIFORMUIVPROC) load(userptr, "glGetUniformuiv");
    context->GetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC) load(userptr, "glGetVertexAttribIiv");
    context->GetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC) load(userptr, "glGetVertexAttribIuiv");
    context->IsEnabledi = (PFNGLISENABLEDIPROC) load(userptr, "glIsEnabledi");
    context->IsFramebuffer = (PFNGLISFRAMEBUFFERPROC) load(userptr, "glIsFramebuffer");
    context->IsRenderbuffer = (PFNGLISRENDERBUFFERPROC) load(userptr, "glIsRenderbuffer");
    context->IsVertexArray = (PFNGLISVERTEXARRAYPROC) load(userptr, "glIsVertexArray");
    context->MapBufferRange = (PFNGLMAPBUFFERRANGEPROC) load(userptr, "glMapBufferRange");
    context->RenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) load(userptr, "glRenderbufferStorage");
    context->RenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) load(userptr, "glRenderbufferStorageMultisample");
    context->TexParameterIiv = (PFNGLTEXPARAMETERIIVPROC) load(userptr, "glTexParameterIiv");
    context->TexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC) load(userptr, "glTexParameterIuiv");
    context->TransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC) load(userptr, "glTransformFeedbackVaryings");
    context->Uniform1ui = (PFNGLUNIFORM1UIPROC) load(userptr, "glUniform1ui");
    context->Uniform1uiv = (PFNGLUNIFORM1UIVPROC) load(userptr, "glUniform1uiv");
    context->Uniform2ui = (PFNGLUNIFORM2UIPROC) load(userptr, "glUniform2ui");
    context->Uniform2uiv = (PFNGLUNIFORM2UIVPROC) load(userptr, "glUniform2uiv");
    context->Uniform3ui = (PFNGLUNIFORM3UIPROC) load(userptr, "glUniform3ui");
    context->Uniform3uiv = (PFNGLUNIFORM3UIVPROC) load(userptr, "glUniform3uiv");
    context->Uniform4ui = (PFNGLUNIFORM4UIPROC) load(userptr, "glUniform4ui");
    context->Uniform4uiv = (PFNGLUNIFORM4UIVPROC) load(userptr, "glUniform4uiv");
    context->VertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC) load(userptr, "glVertexAttribI1i");
    context->VertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC) load(userptr, "glVertexAttribI1iv");
    context->VertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC) load(userptr, "glVertexAttribI1ui");
    context->VertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC) load(userptr, "glVertexAttribI1uiv");
    context->VertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC) load(userptr, "glVertexAttribI2i");
    context->VertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC) load(userptr, "glVertexAttribI2iv");
    context->VertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC) load(userptr, "glVertexAttribI2ui");
    context->VertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC) load(userptr, "glVertexAttribI2uiv");
    context->VertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC) load(userptr, "glVertexAttribI3i");
    context->VertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC) load(userptr, "glVertexAttribI3iv");
    context->VertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC) load(userptr, "glVertexAttribI3ui");
    context->VertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC) load(userptr, "glVertexAttribI3uiv");
    context->VertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC) load(userptr, "glVertexAttribI4bv");
    context->VertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC) load(userptr, "glVertexAttribI4i");
    context->VertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC) load(userptr, "glVertexAttribI4iv");
    context->VertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC) load(userptr, "glVertexAttribI4sv");
    context->VertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC) load(userptr, "glVertexAttribI4ubv");
    context->VertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC) load(userptr, "glVertexAttribI4ui");
    context->VertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC) load(userptr, "glVertexAttribI4uiv");
    context->VertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC) load(userptr, "glVertexAttribI4usv");
    context->VertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC) load(userptr, "glVertexAttribIPointer");
}
static void glad_gl_load_GL_VERSION_3_1(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_3_1) return;
    context->BindBufferBase = (PFNGLBINDBUFFERBASEPROC) load(userptr, "glBindBufferBase");
    context->BindBufferRange = (PFNGLBINDBUFFERRANGEPROC) load(userptr, "glBindBufferRange");
    context->CopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC) load(userptr, "glCopyBufferSubData");
    context->DrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC) load(userptr, "glDrawArraysInstanced");
    context->DrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC) load(userptr, "glDrawElementsInstanced");
    context->GetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC) load(userptr, "glGetActiveUniformBlockName");
    context->GetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC) load(userptr, "glGetActiveUniformBlockiv");
    context->GetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC) load(userptr, "glGetActiveUniformName");
    context->GetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC) load(userptr, "glGetActiveUniformsiv");
    context->GetIntegeri_v = (PFNGLGETINTEGERI_VPROC) load(userptr, "glGetIntegeri_v");
    context->GetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC) load(userptr, "glGetUniformBlockIndex");
    context->GetUniformIndices = (PFNGLGETUNIFORMINDICESPROC) load(userptr, "glGetUniformIndices");
    context->PrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC) load(userptr, "glPrimitiveRestartIndex");
    context->TexBuffer = (PFNGLTEXBUFFERPROC) load(userptr, "glTexBuffer");
    context->UniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC) load(userptr, "glUniformBlockBinding");
}
static void glad_gl_load_GL_VERSION_3_2(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->VERSION_3_2) return;
    context->ClientWaitSync = (PFNGLCLIENTWAITSYNCPROC) load(userptr, "glClientWaitSync");
    context->DeleteSync = (PFNGLDELETESYNCPROC) load(userptr, "glDeleteSync");
    context->DrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC) load(userptr, "glDrawElementsBaseVertex");
    context->DrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC) load(userptr, "glDrawElementsInstancedBaseVertex");
    context->DrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC) load(userptr, "glDrawRangeElementsBaseVertex");
    context->FenceSync = (PFNGLFENCESYNCPROC) load(userptr, "glFenceSync");
    context->FramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC) load(userptr, "glFramebufferTexture");
    context->GetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC) load(userptr, "glGetBufferParameteri64v");
    context->GetInteger64i_v = (PFNGLGETINTEGER64I_VPROC) load(userptr, "glGetInteger64i_v");
    context->GetInteger64v = (PFNGLGETINTEGER64VPROC) load(userptr, "glGetInteger64v");
    context->GetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC) load(userptr, "glGetMultisamplefv");
    context->GetSynciv = (PFNGLGETSYNCIVPROC) load(userptr, "glGetSynciv");
    context->IsSync = (PFNGLISSYNCPROC) load(userptr, "glIsSync");
    context->MultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC) load(userptr, "glMultiDrawElementsBaseVertex");
    context->ProvokingVertex = (PFNGLPROVOKINGVERTEXPROC) load(userptr, "glProvokingVertex");
    context->SampleMaski = (PFNGLSAMPLEMASKIPROC) load(userptr, "glSampleMaski");
    context->TexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC) load(userptr, "glTexImage2DMultisample");
    context->TexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC) load(userptr, "glTexImage3DMultisample");
    context->WaitSync = (PFNGLWAITSYNCPROC) load(userptr, "glWaitSync");
}
static void glad_gl_load_GL_APPLE_flush_buffer_range(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->APPLE_flush_buffer_range) return;
    context->BufferParameteriAPPLE = (PFNGLBUFFERPARAMETERIAPPLEPROC) load(userptr, "glBufferParameteriAPPLE");
    context->FlushMappedBufferRangeAPPLE = (PFNGLFLUSHMAPPEDBUFFERRANGEAPPLEPROC) load(userptr, "glFlushMappedBufferRangeAPPLE");
}
static void glad_gl_load_GL_APPLE_vertex_array_object(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->APPLE_vertex_array_object) return;
    context->BindVertexArrayAPPLE = (PFNGLBINDVERTEXARRAYAPPLEPROC) load(userptr, "glBindVertexArrayAPPLE");
    context->DeleteVertexArraysAPPLE = (PFNGLDELETEVERTEXARRAYSAPPLEPROC) load(userptr, "glDeleteVertexArraysAPPLE");
    context->GenVertexArraysAPPLE = (PFNGLGENVERTEXARRAYSAPPLEPROC) load(userptr, "glGenVertexArraysAPPLE");
    context->IsVertexArrayAPPLE = (PFNGLISVERTEXARRAYAPPLEPROC) load(userptr, "glIsVertexArrayAPPLE");
}
static void glad_gl_load_GL_ARB_color_buffer_float(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_color_buffer_float) return;
    context->ClampColorARB = (PFNGLCLAMPCOLORARBPROC) load(userptr, "glClampColorARB");
}
static void glad_gl_load_GL_ARB_copy_buffer(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_copy_buffer) return;
    context->CopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC) load(userptr, "glCopyBufferSubData");
}
static void glad_gl_load_GL_ARB_draw_buffers(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_draw_buffers) return;
    context->DrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC) load(userptr, "glDrawBuffersARB");
}
static void glad_gl_load_GL_ARB_draw_elements_base_vertex(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_draw_elements_base_vertex) return;
    context->DrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC) load(userptr, "glDrawElementsBaseVertex");
    context->DrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC) load(userptr, "glDrawElementsInstancedBaseVertex");
    context->DrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC) load(userptr, "glDrawRangeElementsBaseVertex");
    context->MultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC) load(userptr, "glMultiDrawElementsBaseVertex");
}
static void glad_gl_load_GL_ARB_draw_instanced(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_draw_instanced) return;
    context->DrawArraysInstancedARB = (PFNGLDRAWARRAYSINSTANCEDARBPROC) load(userptr, "glDrawArraysInstancedARB");
    context->DrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC) load(userptr, "glDrawElementsInstancedARB");
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
static void glad_gl_load_GL_ARB_map_buffer_range(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_map_buffer_range) return;
    context->FlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC) load(userptr, "glFlushMappedBufferRange");
    context->MapBufferRange = (PFNGLMAPBUFFERRANGEPROC) load(userptr, "glMapBufferRange");
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
static void glad_gl_load_GL_ARB_provoking_vertex(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_provoking_vertex) return;
    context->ProvokingVertex = (PFNGLPROVOKINGVERTEXPROC) load(userptr, "glProvokingVertex");
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
static void glad_gl_load_GL_ARB_sync(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_sync) return;
    context->ClientWaitSync = (PFNGLCLIENTWAITSYNCPROC) load(userptr, "glClientWaitSync");
    context->DeleteSync = (PFNGLDELETESYNCPROC) load(userptr, "glDeleteSync");
    context->FenceSync = (PFNGLFENCESYNCPROC) load(userptr, "glFenceSync");
    context->GetInteger64v = (PFNGLGETINTEGER64VPROC) load(userptr, "glGetInteger64v");
    context->GetSynciv = (PFNGLGETSYNCIVPROC) load(userptr, "glGetSynciv");
    context->IsSync = (PFNGLISSYNCPROC) load(userptr, "glIsSync");
    context->WaitSync = (PFNGLWAITSYNCPROC) load(userptr, "glWaitSync");
}
static void glad_gl_load_GL_ARB_texture_buffer_object(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_texture_buffer_object) return;
    context->TexBufferARB = (PFNGLTEXBUFFERARBPROC) load(userptr, "glTexBufferARB");
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
static void glad_gl_load_GL_ARB_texture_multisample(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_texture_multisample) return;
    context->GetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC) load(userptr, "glGetMultisamplefv");
    context->SampleMaski = (PFNGLSAMPLEMASKIPROC) load(userptr, "glSampleMaski");
    context->TexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC) load(userptr, "glTexImage2DMultisample");
    context->TexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC) load(userptr, "glTexImage3DMultisample");
}
static void glad_gl_load_GL_ARB_uniform_buffer_object(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_uniform_buffer_object) return;
    context->BindBufferBase = (PFNGLBINDBUFFERBASEPROC) load(userptr, "glBindBufferBase");
    context->BindBufferRange = (PFNGLBINDBUFFERRANGEPROC) load(userptr, "glBindBufferRange");
    context->GetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC) load(userptr, "glGetActiveUniformBlockName");
    context->GetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC) load(userptr, "glGetActiveUniformBlockiv");
    context->GetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC) load(userptr, "glGetActiveUniformName");
    context->GetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC) load(userptr, "glGetActiveUniformsiv");
    context->GetIntegeri_v = (PFNGLGETINTEGERI_VPROC) load(userptr, "glGetIntegeri_v");
    context->GetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC) load(userptr, "glGetUniformBlockIndex");
    context->GetUniformIndices = (PFNGLGETUNIFORMINDICESPROC) load(userptr, "glGetUniformIndices");
    context->UniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC) load(userptr, "glUniformBlockBinding");
}
static void glad_gl_load_GL_ARB_vertex_array_object(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->ARB_vertex_array_object) return;
    context->BindVertexArray = (PFNGLBINDVERTEXARRAYPROC) load(userptr, "glBindVertexArray");
    context->DeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) load(userptr, "glDeleteVertexArrays");
    context->GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) load(userptr, "glGenVertexArrays");
    context->IsVertexArray = (PFNGLISVERTEXARRAYPROC) load(userptr, "glIsVertexArray");
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
static void glad_gl_load_GL_EXT_direct_state_access(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_direct_state_access) return;
    context->BindMultiTextureEXT = (PFNGLBINDMULTITEXTUREEXTPROC) load(userptr, "glBindMultiTextureEXT");
    context->CheckNamedFramebufferStatusEXT = (PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC) load(userptr, "glCheckNamedFramebufferStatusEXT");
    context->ClearNamedBufferDataEXT = (PFNGLCLEARNAMEDBUFFERDATAEXTPROC) load(userptr, "glClearNamedBufferDataEXT");
    context->ClearNamedBufferSubDataEXT = (PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC) load(userptr, "glClearNamedBufferSubDataEXT");
    context->ClientAttribDefaultEXT = (PFNGLCLIENTATTRIBDEFAULTEXTPROC) load(userptr, "glClientAttribDefaultEXT");
    context->CompressedMultiTexImage1DEXT = (PFNGLCOMPRESSEDMULTITEXIMAGE1DEXTPROC) load(userptr, "glCompressedMultiTexImage1DEXT");
    context->CompressedMultiTexImage2DEXT = (PFNGLCOMPRESSEDMULTITEXIMAGE2DEXTPROC) load(userptr, "glCompressedMultiTexImage2DEXT");
    context->CompressedMultiTexImage3DEXT = (PFNGLCOMPRESSEDMULTITEXIMAGE3DEXTPROC) load(userptr, "glCompressedMultiTexImage3DEXT");
    context->CompressedMultiTexSubImage1DEXT = (PFNGLCOMPRESSEDMULTITEXSUBIMAGE1DEXTPROC) load(userptr, "glCompressedMultiTexSubImage1DEXT");
    context->CompressedMultiTexSubImage2DEXT = (PFNGLCOMPRESSEDMULTITEXSUBIMAGE2DEXTPROC) load(userptr, "glCompressedMultiTexSubImage2DEXT");
    context->CompressedMultiTexSubImage3DEXT = (PFNGLCOMPRESSEDMULTITEXSUBIMAGE3DEXTPROC) load(userptr, "glCompressedMultiTexSubImage3DEXT");
    context->CompressedTextureImage1DEXT = (PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC) load(userptr, "glCompressedTextureImage1DEXT");
    context->CompressedTextureImage2DEXT = (PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC) load(userptr, "glCompressedTextureImage2DEXT");
    context->CompressedTextureImage3DEXT = (PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC) load(userptr, "glCompressedTextureImage3DEXT");
    context->CompressedTextureSubImage1DEXT = (PFNGLCOMPRESSEDTEXTURESUBIMAGE1DEXTPROC) load(userptr, "glCompressedTextureSubImage1DEXT");
    context->CompressedTextureSubImage2DEXT = (PFNGLCOMPRESSEDTEXTURESUBIMAGE2DEXTPROC) load(userptr, "glCompressedTextureSubImage2DEXT");
    context->CompressedTextureSubImage3DEXT = (PFNGLCOMPRESSEDTEXTURESUBIMAGE3DEXTPROC) load(userptr, "glCompressedTextureSubImage3DEXT");
    context->CopyMultiTexImage1DEXT = (PFNGLCOPYMULTITEXIMAGE1DEXTPROC) load(userptr, "glCopyMultiTexImage1DEXT");
    context->CopyMultiTexImage2DEXT = (PFNGLCOPYMULTITEXIMAGE2DEXTPROC) load(userptr, "glCopyMultiTexImage2DEXT");
    context->CopyMultiTexSubImage1DEXT = (PFNGLCOPYMULTITEXSUBIMAGE1DEXTPROC) load(userptr, "glCopyMultiTexSubImage1DEXT");
    context->CopyMultiTexSubImage2DEXT = (PFNGLCOPYMULTITEXSUBIMAGE2DEXTPROC) load(userptr, "glCopyMultiTexSubImage2DEXT");
    context->CopyMultiTexSubImage3DEXT = (PFNGLCOPYMULTITEXSUBIMAGE3DEXTPROC) load(userptr, "glCopyMultiTexSubImage3DEXT");
    context->CopyTextureImage1DEXT = (PFNGLCOPYTEXTUREIMAGE1DEXTPROC) load(userptr, "glCopyTextureImage1DEXT");
    context->CopyTextureImage2DEXT = (PFNGLCOPYTEXTUREIMAGE2DEXTPROC) load(userptr, "glCopyTextureImage2DEXT");
    context->CopyTextureSubImage1DEXT = (PFNGLCOPYTEXTURESUBIMAGE1DEXTPROC) load(userptr, "glCopyTextureSubImage1DEXT");
    context->CopyTextureSubImage2DEXT = (PFNGLCOPYTEXTURESUBIMAGE2DEXTPROC) load(userptr, "glCopyTextureSubImage2DEXT");
    context->CopyTextureSubImage3DEXT = (PFNGLCOPYTEXTURESUBIMAGE3DEXTPROC) load(userptr, "glCopyTextureSubImage3DEXT");
    context->DisableClientStateIndexedEXT = (PFNGLDISABLECLIENTSTATEINDEXEDEXTPROC) load(userptr, "glDisableClientStateIndexedEXT");
    context->DisableClientStateiEXT = (PFNGLDISABLECLIENTSTATEIEXTPROC) load(userptr, "glDisableClientStateiEXT");
    context->DisableIndexedEXT = (PFNGLDISABLEINDEXEDEXTPROC) load(userptr, "glDisableIndexedEXT");
    context->DisableVertexArrayAttribEXT = (PFNGLDISABLEVERTEXARRAYATTRIBEXTPROC) load(userptr, "glDisableVertexArrayAttribEXT");
    context->DisableVertexArrayEXT = (PFNGLDISABLEVERTEXARRAYEXTPROC) load(userptr, "glDisableVertexArrayEXT");
    context->EnableClientStateIndexedEXT = (PFNGLENABLECLIENTSTATEINDEXEDEXTPROC) load(userptr, "glEnableClientStateIndexedEXT");
    context->EnableClientStateiEXT = (PFNGLENABLECLIENTSTATEIEXTPROC) load(userptr, "glEnableClientStateiEXT");
    context->EnableIndexedEXT = (PFNGLENABLEINDEXEDEXTPROC) load(userptr, "glEnableIndexedEXT");
    context->EnableVertexArrayAttribEXT = (PFNGLENABLEVERTEXARRAYATTRIBEXTPROC) load(userptr, "glEnableVertexArrayAttribEXT");
    context->EnableVertexArrayEXT = (PFNGLENABLEVERTEXARRAYEXTPROC) load(userptr, "glEnableVertexArrayEXT");
    context->FlushMappedNamedBufferRangeEXT = (PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEEXTPROC) load(userptr, "glFlushMappedNamedBufferRangeEXT");
    context->FramebufferDrawBufferEXT = (PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC) load(userptr, "glFramebufferDrawBufferEXT");
    context->FramebufferDrawBuffersEXT = (PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC) load(userptr, "glFramebufferDrawBuffersEXT");
    context->FramebufferReadBufferEXT = (PFNGLFRAMEBUFFERREADBUFFEREXTPROC) load(userptr, "glFramebufferReadBufferEXT");
    context->GenerateMultiTexMipmapEXT = (PFNGLGENERATEMULTITEXMIPMAPEXTPROC) load(userptr, "glGenerateMultiTexMipmapEXT");
    context->GenerateTextureMipmapEXT = (PFNGLGENERATETEXTUREMIPMAPEXTPROC) load(userptr, "glGenerateTextureMipmapEXT");
    context->GetBooleanIndexedvEXT = (PFNGLGETBOOLEANINDEXEDVEXTPROC) load(userptr, "glGetBooleanIndexedvEXT");
    context->GetCompressedMultiTexImageEXT = (PFNGLGETCOMPRESSEDMULTITEXIMAGEEXTPROC) load(userptr, "glGetCompressedMultiTexImageEXT");
    context->GetCompressedTextureImageEXT = (PFNGLGETCOMPRESSEDTEXTUREIMAGEEXTPROC) load(userptr, "glGetCompressedTextureImageEXT");
    context->GetDoubleIndexedvEXT = (PFNGLGETDOUBLEINDEXEDVEXTPROC) load(userptr, "glGetDoubleIndexedvEXT");
    context->GetDoublei_vEXT = (PFNGLGETDOUBLEI_VEXTPROC) load(userptr, "glGetDoublei_vEXT");
    context->GetFloatIndexedvEXT = (PFNGLGETFLOATINDEXEDVEXTPROC) load(userptr, "glGetFloatIndexedvEXT");
    context->GetFloati_vEXT = (PFNGLGETFLOATI_VEXTPROC) load(userptr, "glGetFloati_vEXT");
    context->GetFramebufferParameterivEXT = (PFNGLGETFRAMEBUFFERPARAMETERIVEXTPROC) load(userptr, "glGetFramebufferParameterivEXT");
    context->GetIntegerIndexedvEXT = (PFNGLGETINTEGERINDEXEDVEXTPROC) load(userptr, "glGetIntegerIndexedvEXT");
    context->GetMultiTexEnvfvEXT = (PFNGLGETMULTITEXENVFVEXTPROC) load(userptr, "glGetMultiTexEnvfvEXT");
    context->GetMultiTexEnvivEXT = (PFNGLGETMULTITEXENVIVEXTPROC) load(userptr, "glGetMultiTexEnvivEXT");
    context->GetMultiTexGendvEXT = (PFNGLGETMULTITEXGENDVEXTPROC) load(userptr, "glGetMultiTexGendvEXT");
    context->GetMultiTexGenfvEXT = (PFNGLGETMULTITEXGENFVEXTPROC) load(userptr, "glGetMultiTexGenfvEXT");
    context->GetMultiTexGenivEXT = (PFNGLGETMULTITEXGENIVEXTPROC) load(userptr, "glGetMultiTexGenivEXT");
    context->GetMultiTexImageEXT = (PFNGLGETMULTITEXIMAGEEXTPROC) load(userptr, "glGetMultiTexImageEXT");
    context->GetMultiTexLevelParameterfvEXT = (PFNGLGETMULTITEXLEVELPARAMETERFVEXTPROC) load(userptr, "glGetMultiTexLevelParameterfvEXT");
    context->GetMultiTexLevelParameterivEXT = (PFNGLGETMULTITEXLEVELPARAMETERIVEXTPROC) load(userptr, "glGetMultiTexLevelParameterivEXT");
    context->GetMultiTexParameterIivEXT = (PFNGLGETMULTITEXPARAMETERIIVEXTPROC) load(userptr, "glGetMultiTexParameterIivEXT");
    context->GetMultiTexParameterIuivEXT = (PFNGLGETMULTITEXPARAMETERIUIVEXTPROC) load(userptr, "glGetMultiTexParameterIuivEXT");
    context->GetMultiTexParameterfvEXT = (PFNGLGETMULTITEXPARAMETERFVEXTPROC) load(userptr, "glGetMultiTexParameterfvEXT");
    context->GetMultiTexParameterivEXT = (PFNGLGETMULTITEXPARAMETERIVEXTPROC) load(userptr, "glGetMultiTexParameterivEXT");
    context->GetNamedBufferParameterivEXT = (PFNGLGETNAMEDBUFFERPARAMETERIVEXTPROC) load(userptr, "glGetNamedBufferParameterivEXT");
    context->GetNamedBufferPointervEXT = (PFNGLGETNAMEDBUFFERPOINTERVEXTPROC) load(userptr, "glGetNamedBufferPointervEXT");
    context->GetNamedBufferSubDataEXT = (PFNGLGETNAMEDBUFFERSUBDATAEXTPROC) load(userptr, "glGetNamedBufferSubDataEXT");
    context->GetNamedFramebufferAttachmentParameterivEXT = (PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) load(userptr, "glGetNamedFramebufferAttachmentParameterivEXT");
    context->GetNamedFramebufferParameterivEXT = (PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC) load(userptr, "glGetNamedFramebufferParameterivEXT");
    context->GetNamedProgramLocalParameterIivEXT = (PFNGLGETNAMEDPROGRAMLOCALPARAMETERIIVEXTPROC) load(userptr, "glGetNamedProgramLocalParameterIivEXT");
    context->GetNamedProgramLocalParameterIuivEXT = (PFNGLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXTPROC) load(userptr, "glGetNamedProgramLocalParameterIuivEXT");
    context->GetNamedProgramLocalParameterdvEXT = (PFNGLGETNAMEDPROGRAMLOCALPARAMETERDVEXTPROC) load(userptr, "glGetNamedProgramLocalParameterdvEXT");
    context->GetNamedProgramLocalParameterfvEXT = (PFNGLGETNAMEDPROGRAMLOCALPARAMETERFVEXTPROC) load(userptr, "glGetNamedProgramLocalParameterfvEXT");
    context->GetNamedProgramStringEXT = (PFNGLGETNAMEDPROGRAMSTRINGEXTPROC) load(userptr, "glGetNamedProgramStringEXT");
    context->GetNamedProgramivEXT = (PFNGLGETNAMEDPROGRAMIVEXTPROC) load(userptr, "glGetNamedProgramivEXT");
    context->GetNamedRenderbufferParameterivEXT = (PFNGLGETNAMEDRENDERBUFFERPARAMETERIVEXTPROC) load(userptr, "glGetNamedRenderbufferParameterivEXT");
    context->GetPointerIndexedvEXT = (PFNGLGETPOINTERINDEXEDVEXTPROC) load(userptr, "glGetPointerIndexedvEXT");
    context->GetPointeri_vEXT = (PFNGLGETPOINTERI_VEXTPROC) load(userptr, "glGetPointeri_vEXT");
    context->GetTextureImageEXT = (PFNGLGETTEXTUREIMAGEEXTPROC) load(userptr, "glGetTextureImageEXT");
    context->GetTextureLevelParameterfvEXT = (PFNGLGETTEXTURELEVELPARAMETERFVEXTPROC) load(userptr, "glGetTextureLevelParameterfvEXT");
    context->GetTextureLevelParameterivEXT = (PFNGLGETTEXTURELEVELPARAMETERIVEXTPROC) load(userptr, "glGetTextureLevelParameterivEXT");
    context->GetTextureParameterIivEXT = (PFNGLGETTEXTUREPARAMETERIIVEXTPROC) load(userptr, "glGetTextureParameterIivEXT");
    context->GetTextureParameterIuivEXT = (PFNGLGETTEXTUREPARAMETERIUIVEXTPROC) load(userptr, "glGetTextureParameterIuivEXT");
    context->GetTextureParameterfvEXT = (PFNGLGETTEXTUREPARAMETERFVEXTPROC) load(userptr, "glGetTextureParameterfvEXT");
    context->GetTextureParameterivEXT = (PFNGLGETTEXTUREPARAMETERIVEXTPROC) load(userptr, "glGetTextureParameterivEXT");
    context->GetVertexArrayIntegeri_vEXT = (PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC) load(userptr, "glGetVertexArrayIntegeri_vEXT");
    context->GetVertexArrayIntegervEXT = (PFNGLGETVERTEXARRAYINTEGERVEXTPROC) load(userptr, "glGetVertexArrayIntegervEXT");
    context->GetVertexArrayPointeri_vEXT = (PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC) load(userptr, "glGetVertexArrayPointeri_vEXT");
    context->GetVertexArrayPointervEXT = (PFNGLGETVERTEXARRAYPOINTERVEXTPROC) load(userptr, "glGetVertexArrayPointervEXT");
    context->IsEnabledIndexedEXT = (PFNGLISENABLEDINDEXEDEXTPROC) load(userptr, "glIsEnabledIndexedEXT");
    context->MapNamedBufferEXT = (PFNGLMAPNAMEDBUFFEREXTPROC) load(userptr, "glMapNamedBufferEXT");
    context->MapNamedBufferRangeEXT = (PFNGLMAPNAMEDBUFFERRANGEEXTPROC) load(userptr, "glMapNamedBufferRangeEXT");
    context->MatrixFrustumEXT = (PFNGLMATRIXFRUSTUMEXTPROC) load(userptr, "glMatrixFrustumEXT");
    context->MatrixLoadIdentityEXT = (PFNGLMATRIXLOADIDENTITYEXTPROC) load(userptr, "glMatrixLoadIdentityEXT");
    context->MatrixLoadTransposedEXT = (PFNGLMATRIXLOADTRANSPOSEDEXTPROC) load(userptr, "glMatrixLoadTransposedEXT");
    context->MatrixLoadTransposefEXT = (PFNGLMATRIXLOADTRANSPOSEFEXTPROC) load(userptr, "glMatrixLoadTransposefEXT");
    context->MatrixLoaddEXT = (PFNGLMATRIXLOADDEXTPROC) load(userptr, "glMatrixLoaddEXT");
    context->MatrixLoadfEXT = (PFNGLMATRIXLOADFEXTPROC) load(userptr, "glMatrixLoadfEXT");
    context->MatrixMultTransposedEXT = (PFNGLMATRIXMULTTRANSPOSEDEXTPROC) load(userptr, "glMatrixMultTransposedEXT");
    context->MatrixMultTransposefEXT = (PFNGLMATRIXMULTTRANSPOSEFEXTPROC) load(userptr, "glMatrixMultTransposefEXT");
    context->MatrixMultdEXT = (PFNGLMATRIXMULTDEXTPROC) load(userptr, "glMatrixMultdEXT");
    context->MatrixMultfEXT = (PFNGLMATRIXMULTFEXTPROC) load(userptr, "glMatrixMultfEXT");
    context->MatrixOrthoEXT = (PFNGLMATRIXORTHOEXTPROC) load(userptr, "glMatrixOrthoEXT");
    context->MatrixPopEXT = (PFNGLMATRIXPOPEXTPROC) load(userptr, "glMatrixPopEXT");
    context->MatrixPushEXT = (PFNGLMATRIXPUSHEXTPROC) load(userptr, "glMatrixPushEXT");
    context->MatrixRotatedEXT = (PFNGLMATRIXROTATEDEXTPROC) load(userptr, "glMatrixRotatedEXT");
    context->MatrixRotatefEXT = (PFNGLMATRIXROTATEFEXTPROC) load(userptr, "glMatrixRotatefEXT");
    context->MatrixScaledEXT = (PFNGLMATRIXSCALEDEXTPROC) load(userptr, "glMatrixScaledEXT");
    context->MatrixScalefEXT = (PFNGLMATRIXSCALEFEXTPROC) load(userptr, "glMatrixScalefEXT");
    context->MatrixTranslatedEXT = (PFNGLMATRIXTRANSLATEDEXTPROC) load(userptr, "glMatrixTranslatedEXT");
    context->MatrixTranslatefEXT = (PFNGLMATRIXTRANSLATEFEXTPROC) load(userptr, "glMatrixTranslatefEXT");
    context->MultiTexBufferEXT = (PFNGLMULTITEXBUFFEREXTPROC) load(userptr, "glMultiTexBufferEXT");
    context->MultiTexCoordPointerEXT = (PFNGLMULTITEXCOORDPOINTEREXTPROC) load(userptr, "glMultiTexCoordPointerEXT");
    context->MultiTexEnvfEXT = (PFNGLMULTITEXENVFEXTPROC) load(userptr, "glMultiTexEnvfEXT");
    context->MultiTexEnvfvEXT = (PFNGLMULTITEXENVFVEXTPROC) load(userptr, "glMultiTexEnvfvEXT");
    context->MultiTexEnviEXT = (PFNGLMULTITEXENVIEXTPROC) load(userptr, "glMultiTexEnviEXT");
    context->MultiTexEnvivEXT = (PFNGLMULTITEXENVIVEXTPROC) load(userptr, "glMultiTexEnvivEXT");
    context->MultiTexGendEXT = (PFNGLMULTITEXGENDEXTPROC) load(userptr, "glMultiTexGendEXT");
    context->MultiTexGendvEXT = (PFNGLMULTITEXGENDVEXTPROC) load(userptr, "glMultiTexGendvEXT");
    context->MultiTexGenfEXT = (PFNGLMULTITEXGENFEXTPROC) load(userptr, "glMultiTexGenfEXT");
    context->MultiTexGenfvEXT = (PFNGLMULTITEXGENFVEXTPROC) load(userptr, "glMultiTexGenfvEXT");
    context->MultiTexGeniEXT = (PFNGLMULTITEXGENIEXTPROC) load(userptr, "glMultiTexGeniEXT");
    context->MultiTexGenivEXT = (PFNGLMULTITEXGENIVEXTPROC) load(userptr, "glMultiTexGenivEXT");
    context->MultiTexImage1DEXT = (PFNGLMULTITEXIMAGE1DEXTPROC) load(userptr, "glMultiTexImage1DEXT");
    context->MultiTexImage2DEXT = (PFNGLMULTITEXIMAGE2DEXTPROC) load(userptr, "glMultiTexImage2DEXT");
    context->MultiTexImage3DEXT = (PFNGLMULTITEXIMAGE3DEXTPROC) load(userptr, "glMultiTexImage3DEXT");
    context->MultiTexParameterIivEXT = (PFNGLMULTITEXPARAMETERIIVEXTPROC) load(userptr, "glMultiTexParameterIivEXT");
    context->MultiTexParameterIuivEXT = (PFNGLMULTITEXPARAMETERIUIVEXTPROC) load(userptr, "glMultiTexParameterIuivEXT");
    context->MultiTexParameterfEXT = (PFNGLMULTITEXPARAMETERFEXTPROC) load(userptr, "glMultiTexParameterfEXT");
    context->MultiTexParameterfvEXT = (PFNGLMULTITEXPARAMETERFVEXTPROC) load(userptr, "glMultiTexParameterfvEXT");
    context->MultiTexParameteriEXT = (PFNGLMULTITEXPARAMETERIEXTPROC) load(userptr, "glMultiTexParameteriEXT");
    context->MultiTexParameterivEXT = (PFNGLMULTITEXPARAMETERIVEXTPROC) load(userptr, "glMultiTexParameterivEXT");
    context->MultiTexRenderbufferEXT = (PFNGLMULTITEXRENDERBUFFEREXTPROC) load(userptr, "glMultiTexRenderbufferEXT");
    context->MultiTexSubImage1DEXT = (PFNGLMULTITEXSUBIMAGE1DEXTPROC) load(userptr, "glMultiTexSubImage1DEXT");
    context->MultiTexSubImage2DEXT = (PFNGLMULTITEXSUBIMAGE2DEXTPROC) load(userptr, "glMultiTexSubImage2DEXT");
    context->MultiTexSubImage3DEXT = (PFNGLMULTITEXSUBIMAGE3DEXTPROC) load(userptr, "glMultiTexSubImage3DEXT");
    context->NamedBufferDataEXT = (PFNGLNAMEDBUFFERDATAEXTPROC) load(userptr, "glNamedBufferDataEXT");
    context->NamedBufferStorageEXT = (PFNGLNAMEDBUFFERSTORAGEEXTPROC) load(userptr, "glNamedBufferStorageEXT");
    context->NamedBufferSubDataEXT = (PFNGLNAMEDBUFFERSUBDATAEXTPROC) load(userptr, "glNamedBufferSubDataEXT");
    context->NamedCopyBufferSubDataEXT = (PFNGLNAMEDCOPYBUFFERSUBDATAEXTPROC) load(userptr, "glNamedCopyBufferSubDataEXT");
    context->NamedFramebufferParameteriEXT = (PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC) load(userptr, "glNamedFramebufferParameteriEXT");
    context->NamedFramebufferRenderbufferEXT = (PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC) load(userptr, "glNamedFramebufferRenderbufferEXT");
    context->NamedFramebufferTexture1DEXT = (PFNGLNAMEDFRAMEBUFFERTEXTURE1DEXTPROC) load(userptr, "glNamedFramebufferTexture1DEXT");
    context->NamedFramebufferTexture2DEXT = (PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC) load(userptr, "glNamedFramebufferTexture2DEXT");
    context->NamedFramebufferTexture3DEXT = (PFNGLNAMEDFRAMEBUFFERTEXTURE3DEXTPROC) load(userptr, "glNamedFramebufferTexture3DEXT");
    context->NamedFramebufferTextureEXT = (PFNGLNAMEDFRAMEBUFFERTEXTUREEXTPROC) load(userptr, "glNamedFramebufferTextureEXT");
    context->NamedFramebufferTextureFaceEXT = (PFNGLNAMEDFRAMEBUFFERTEXTUREFACEEXTPROC) load(userptr, "glNamedFramebufferTextureFaceEXT");
    context->NamedFramebufferTextureLayerEXT = (PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC) load(userptr, "glNamedFramebufferTextureLayerEXT");
    context->NamedProgramLocalParameter4dEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETER4DEXTPROC) load(userptr, "glNamedProgramLocalParameter4dEXT");
    context->NamedProgramLocalParameter4dvEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETER4DVEXTPROC) load(userptr, "glNamedProgramLocalParameter4dvEXT");
    context->NamedProgramLocalParameter4fEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETER4FEXTPROC) load(userptr, "glNamedProgramLocalParameter4fEXT");
    context->NamedProgramLocalParameter4fvEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETER4FVEXTPROC) load(userptr, "glNamedProgramLocalParameter4fvEXT");
    context->NamedProgramLocalParameterI4iEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERI4IEXTPROC) load(userptr, "glNamedProgramLocalParameterI4iEXT");
    context->NamedProgramLocalParameterI4ivEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERI4IVEXTPROC) load(userptr, "glNamedProgramLocalParameterI4ivEXT");
    context->NamedProgramLocalParameterI4uiEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIEXTPROC) load(userptr, "glNamedProgramLocalParameterI4uiEXT");
    context->NamedProgramLocalParameterI4uivEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIVEXTPROC) load(userptr, "glNamedProgramLocalParameterI4uivEXT");
    context->NamedProgramLocalParameters4fvEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERS4FVEXTPROC) load(userptr, "glNamedProgramLocalParameters4fvEXT");
    context->NamedProgramLocalParametersI4ivEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERSI4IVEXTPROC) load(userptr, "glNamedProgramLocalParametersI4ivEXT");
    context->NamedProgramLocalParametersI4uivEXT = (PFNGLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXTPROC) load(userptr, "glNamedProgramLocalParametersI4uivEXT");
    context->NamedProgramStringEXT = (PFNGLNAMEDPROGRAMSTRINGEXTPROC) load(userptr, "glNamedProgramStringEXT");
    context->NamedRenderbufferStorageEXT = (PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC) load(userptr, "glNamedRenderbufferStorageEXT");
    context->NamedRenderbufferStorageMultisampleCoverageEXT = (PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXTPROC) load(userptr, "glNamedRenderbufferStorageMultisampleCoverageEXT");
    context->NamedRenderbufferStorageMultisampleEXT = (PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) load(userptr, "glNamedRenderbufferStorageMultisampleEXT");
    context->ProgramUniform1dEXT = (PFNGLPROGRAMUNIFORM1DEXTPROC) load(userptr, "glProgramUniform1dEXT");
    context->ProgramUniform1dvEXT = (PFNGLPROGRAMUNIFORM1DVEXTPROC) load(userptr, "glProgramUniform1dvEXT");
    context->ProgramUniform1fEXT = (PFNGLPROGRAMUNIFORM1FEXTPROC) load(userptr, "glProgramUniform1fEXT");
    context->ProgramUniform1fvEXT = (PFNGLPROGRAMUNIFORM1FVEXTPROC) load(userptr, "glProgramUniform1fvEXT");
    context->ProgramUniform1iEXT = (PFNGLPROGRAMUNIFORM1IEXTPROC) load(userptr, "glProgramUniform1iEXT");
    context->ProgramUniform1ivEXT = (PFNGLPROGRAMUNIFORM1IVEXTPROC) load(userptr, "glProgramUniform1ivEXT");
    context->ProgramUniform1uiEXT = (PFNGLPROGRAMUNIFORM1UIEXTPROC) load(userptr, "glProgramUniform1uiEXT");
    context->ProgramUniform1uivEXT = (PFNGLPROGRAMUNIFORM1UIVEXTPROC) load(userptr, "glProgramUniform1uivEXT");
    context->ProgramUniform2dEXT = (PFNGLPROGRAMUNIFORM2DEXTPROC) load(userptr, "glProgramUniform2dEXT");
    context->ProgramUniform2dvEXT = (PFNGLPROGRAMUNIFORM2DVEXTPROC) load(userptr, "glProgramUniform2dvEXT");
    context->ProgramUniform2fEXT = (PFNGLPROGRAMUNIFORM2FEXTPROC) load(userptr, "glProgramUniform2fEXT");
    context->ProgramUniform2fvEXT = (PFNGLPROGRAMUNIFORM2FVEXTPROC) load(userptr, "glProgramUniform2fvEXT");
    context->ProgramUniform2iEXT = (PFNGLPROGRAMUNIFORM2IEXTPROC) load(userptr, "glProgramUniform2iEXT");
    context->ProgramUniform2ivEXT = (PFNGLPROGRAMUNIFORM2IVEXTPROC) load(userptr, "glProgramUniform2ivEXT");
    context->ProgramUniform2uiEXT = (PFNGLPROGRAMUNIFORM2UIEXTPROC) load(userptr, "glProgramUniform2uiEXT");
    context->ProgramUniform2uivEXT = (PFNGLPROGRAMUNIFORM2UIVEXTPROC) load(userptr, "glProgramUniform2uivEXT");
    context->ProgramUniform3dEXT = (PFNGLPROGRAMUNIFORM3DEXTPROC) load(userptr, "glProgramUniform3dEXT");
    context->ProgramUniform3dvEXT = (PFNGLPROGRAMUNIFORM3DVEXTPROC) load(userptr, "glProgramUniform3dvEXT");
    context->ProgramUniform3fEXT = (PFNGLPROGRAMUNIFORM3FEXTPROC) load(userptr, "glProgramUniform3fEXT");
    context->ProgramUniform3fvEXT = (PFNGLPROGRAMUNIFORM3FVEXTPROC) load(userptr, "glProgramUniform3fvEXT");
    context->ProgramUniform3iEXT = (PFNGLPROGRAMUNIFORM3IEXTPROC) load(userptr, "glProgramUniform3iEXT");
    context->ProgramUniform3ivEXT = (PFNGLPROGRAMUNIFORM3IVEXTPROC) load(userptr, "glProgramUniform3ivEXT");
    context->ProgramUniform3uiEXT = (PFNGLPROGRAMUNIFORM3UIEXTPROC) load(userptr, "glProgramUniform3uiEXT");
    context->ProgramUniform3uivEXT = (PFNGLPROGRAMUNIFORM3UIVEXTPROC) load(userptr, "glProgramUniform3uivEXT");
    context->ProgramUniform4dEXT = (PFNGLPROGRAMUNIFORM4DEXTPROC) load(userptr, "glProgramUniform4dEXT");
    context->ProgramUniform4dvEXT = (PFNGLPROGRAMUNIFORM4DVEXTPROC) load(userptr, "glProgramUniform4dvEXT");
    context->ProgramUniform4fEXT = (PFNGLPROGRAMUNIFORM4FEXTPROC) load(userptr, "glProgramUniform4fEXT");
    context->ProgramUniform4fvEXT = (PFNGLPROGRAMUNIFORM4FVEXTPROC) load(userptr, "glProgramUniform4fvEXT");
    context->ProgramUniform4iEXT = (PFNGLPROGRAMUNIFORM4IEXTPROC) load(userptr, "glProgramUniform4iEXT");
    context->ProgramUniform4ivEXT = (PFNGLPROGRAMUNIFORM4IVEXTPROC) load(userptr, "glProgramUniform4ivEXT");
    context->ProgramUniform4uiEXT = (PFNGLPROGRAMUNIFORM4UIEXTPROC) load(userptr, "glProgramUniform4uiEXT");
    context->ProgramUniform4uivEXT = (PFNGLPROGRAMUNIFORM4UIVEXTPROC) load(userptr, "glProgramUniform4uivEXT");
    context->ProgramUniformMatrix2dvEXT = (PFNGLPROGRAMUNIFORMMATRIX2DVEXTPROC) load(userptr, "glProgramUniformMatrix2dvEXT");
    context->ProgramUniformMatrix2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC) load(userptr, "glProgramUniformMatrix2fvEXT");
    context->ProgramUniformMatrix2x3dvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X3DVEXTPROC) load(userptr, "glProgramUniformMatrix2x3dvEXT");
    context->ProgramUniformMatrix2x3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC) load(userptr, "glProgramUniformMatrix2x3fvEXT");
    context->ProgramUniformMatrix2x4dvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X4DVEXTPROC) load(userptr, "glProgramUniformMatrix2x4dvEXT");
    context->ProgramUniformMatrix2x4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC) load(userptr, "glProgramUniformMatrix2x4fvEXT");
    context->ProgramUniformMatrix3dvEXT = (PFNGLPROGRAMUNIFORMMATRIX3DVEXTPROC) load(userptr, "glProgramUniformMatrix3dvEXT");
    context->ProgramUniformMatrix3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC) load(userptr, "glProgramUniformMatrix3fvEXT");
    context->ProgramUniformMatrix3x2dvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X2DVEXTPROC) load(userptr, "glProgramUniformMatrix3x2dvEXT");
    context->ProgramUniformMatrix3x2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC) load(userptr, "glProgramUniformMatrix3x2fvEXT");
    context->ProgramUniformMatrix3x4dvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X4DVEXTPROC) load(userptr, "glProgramUniformMatrix3x4dvEXT");
    context->ProgramUniformMatrix3x4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC) load(userptr, "glProgramUniformMatrix3x4fvEXT");
    context->ProgramUniformMatrix4dvEXT = (PFNGLPROGRAMUNIFORMMATRIX4DVEXTPROC) load(userptr, "glProgramUniformMatrix4dvEXT");
    context->ProgramUniformMatrix4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC) load(userptr, "glProgramUniformMatrix4fvEXT");
    context->ProgramUniformMatrix4x2dvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X2DVEXTPROC) load(userptr, "glProgramUniformMatrix4x2dvEXT");
    context->ProgramUniformMatrix4x2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC) load(userptr, "glProgramUniformMatrix4x2fvEXT");
    context->ProgramUniformMatrix4x3dvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X3DVEXTPROC) load(userptr, "glProgramUniformMatrix4x3dvEXT");
    context->ProgramUniformMatrix4x3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC) load(userptr, "glProgramUniformMatrix4x3fvEXT");
    context->PushClientAttribDefaultEXT = (PFNGLPUSHCLIENTATTRIBDEFAULTEXTPROC) load(userptr, "glPushClientAttribDefaultEXT");
    context->TextureBufferEXT = (PFNGLTEXTUREBUFFEREXTPROC) load(userptr, "glTextureBufferEXT");
    context->TextureBufferRangeEXT = (PFNGLTEXTUREBUFFERRANGEEXTPROC) load(userptr, "glTextureBufferRangeEXT");
    context->TextureImage1DEXT = (PFNGLTEXTUREIMAGE1DEXTPROC) load(userptr, "glTextureImage1DEXT");
    context->TextureImage2DEXT = (PFNGLTEXTUREIMAGE2DEXTPROC) load(userptr, "glTextureImage2DEXT");
    context->TextureImage3DEXT = (PFNGLTEXTUREIMAGE3DEXTPROC) load(userptr, "glTextureImage3DEXT");
    context->TexturePageCommitmentEXT = (PFNGLTEXTUREPAGECOMMITMENTEXTPROC) load(userptr, "glTexturePageCommitmentEXT");
    context->TextureParameterIivEXT = (PFNGLTEXTUREPARAMETERIIVEXTPROC) load(userptr, "glTextureParameterIivEXT");
    context->TextureParameterIuivEXT = (PFNGLTEXTUREPARAMETERIUIVEXTPROC) load(userptr, "glTextureParameterIuivEXT");
    context->TextureParameterfEXT = (PFNGLTEXTUREPARAMETERFEXTPROC) load(userptr, "glTextureParameterfEXT");
    context->TextureParameterfvEXT = (PFNGLTEXTUREPARAMETERFVEXTPROC) load(userptr, "glTextureParameterfvEXT");
    context->TextureParameteriEXT = (PFNGLTEXTUREPARAMETERIEXTPROC) load(userptr, "glTextureParameteriEXT");
    context->TextureParameterivEXT = (PFNGLTEXTUREPARAMETERIVEXTPROC) load(userptr, "glTextureParameterivEXT");
    context->TextureRenderbufferEXT = (PFNGLTEXTURERENDERBUFFEREXTPROC) load(userptr, "glTextureRenderbufferEXT");
    context->TextureStorage1DEXT = (PFNGLTEXTURESTORAGE1DEXTPROC) load(userptr, "glTextureStorage1DEXT");
    context->TextureStorage2DEXT = (PFNGLTEXTURESTORAGE2DEXTPROC) load(userptr, "glTextureStorage2DEXT");
    context->TextureStorage2DMultisampleEXT = (PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC) load(userptr, "glTextureStorage2DMultisampleEXT");
    context->TextureStorage3DEXT = (PFNGLTEXTURESTORAGE3DEXTPROC) load(userptr, "glTextureStorage3DEXT");
    context->TextureStorage3DMultisampleEXT = (PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC) load(userptr, "glTextureStorage3DMultisampleEXT");
    context->TextureSubImage1DEXT = (PFNGLTEXTURESUBIMAGE1DEXTPROC) load(userptr, "glTextureSubImage1DEXT");
    context->TextureSubImage2DEXT = (PFNGLTEXTURESUBIMAGE2DEXTPROC) load(userptr, "glTextureSubImage2DEXT");
    context->TextureSubImage3DEXT = (PFNGLTEXTURESUBIMAGE3DEXTPROC) load(userptr, "glTextureSubImage3DEXT");
    context->UnmapNamedBufferEXT = (PFNGLUNMAPNAMEDBUFFEREXTPROC) load(userptr, "glUnmapNamedBufferEXT");
    context->VertexArrayBindVertexBufferEXT = (PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC) load(userptr, "glVertexArrayBindVertexBufferEXT");
    context->VertexArrayColorOffsetEXT = (PFNGLVERTEXARRAYCOLOROFFSETEXTPROC) load(userptr, "glVertexArrayColorOffsetEXT");
    context->VertexArrayEdgeFlagOffsetEXT = (PFNGLVERTEXARRAYEDGEFLAGOFFSETEXTPROC) load(userptr, "glVertexArrayEdgeFlagOffsetEXT");
    context->VertexArrayFogCoordOffsetEXT = (PFNGLVERTEXARRAYFOGCOORDOFFSETEXTPROC) load(userptr, "glVertexArrayFogCoordOffsetEXT");
    context->VertexArrayIndexOffsetEXT = (PFNGLVERTEXARRAYINDEXOFFSETEXTPROC) load(userptr, "glVertexArrayIndexOffsetEXT");
    context->VertexArrayMultiTexCoordOffsetEXT = (PFNGLVERTEXARRAYMULTITEXCOORDOFFSETEXTPROC) load(userptr, "glVertexArrayMultiTexCoordOffsetEXT");
    context->VertexArrayNormalOffsetEXT = (PFNGLVERTEXARRAYNORMALOFFSETEXTPROC) load(userptr, "glVertexArrayNormalOffsetEXT");
    context->VertexArraySecondaryColorOffsetEXT = (PFNGLVERTEXARRAYSECONDARYCOLOROFFSETEXTPROC) load(userptr, "glVertexArraySecondaryColorOffsetEXT");
    context->VertexArrayTexCoordOffsetEXT = (PFNGLVERTEXARRAYTEXCOORDOFFSETEXTPROC) load(userptr, "glVertexArrayTexCoordOffsetEXT");
    context->VertexArrayVertexAttribBindingEXT = (PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC) load(userptr, "glVertexArrayVertexAttribBindingEXT");
    context->VertexArrayVertexAttribDivisorEXT = (PFNGLVERTEXARRAYVERTEXATTRIBDIVISOREXTPROC) load(userptr, "glVertexArrayVertexAttribDivisorEXT");
    context->VertexArrayVertexAttribFormatEXT = (PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC) load(userptr, "glVertexArrayVertexAttribFormatEXT");
    context->VertexArrayVertexAttribIFormatEXT = (PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC) load(userptr, "glVertexArrayVertexAttribIFormatEXT");
    context->VertexArrayVertexAttribIOffsetEXT = (PFNGLVERTEXARRAYVERTEXATTRIBIOFFSETEXTPROC) load(userptr, "glVertexArrayVertexAttribIOffsetEXT");
    context->VertexArrayVertexAttribLFormatEXT = (PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC) load(userptr, "glVertexArrayVertexAttribLFormatEXT");
    context->VertexArrayVertexAttribLOffsetEXT = (PFNGLVERTEXARRAYVERTEXATTRIBLOFFSETEXTPROC) load(userptr, "glVertexArrayVertexAttribLOffsetEXT");
    context->VertexArrayVertexAttribOffsetEXT = (PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC) load(userptr, "glVertexArrayVertexAttribOffsetEXT");
    context->VertexArrayVertexBindingDivisorEXT = (PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC) load(userptr, "glVertexArrayVertexBindingDivisorEXT");
    context->VertexArrayVertexOffsetEXT = (PFNGLVERTEXARRAYVERTEXOFFSETEXTPROC) load(userptr, "glVertexArrayVertexOffsetEXT");
}
static void glad_gl_load_GL_EXT_draw_buffers2(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_draw_buffers2) return;
    context->ColorMaskIndexedEXT = (PFNGLCOLORMASKINDEXEDEXTPROC) load(userptr, "glColorMaskIndexedEXT");
    context->DisableIndexedEXT = (PFNGLDISABLEINDEXEDEXTPROC) load(userptr, "glDisableIndexedEXT");
    context->EnableIndexedEXT = (PFNGLENABLEINDEXEDEXTPROC) load(userptr, "glEnableIndexedEXT");
    context->GetBooleanIndexedvEXT = (PFNGLGETBOOLEANINDEXEDVEXTPROC) load(userptr, "glGetBooleanIndexedvEXT");
    context->GetIntegerIndexedvEXT = (PFNGLGETINTEGERINDEXEDVEXTPROC) load(userptr, "glGetIntegerIndexedvEXT");
    context->IsEnabledIndexedEXT = (PFNGLISENABLEDINDEXEDEXTPROC) load(userptr, "glIsEnabledIndexedEXT");
}
static void glad_gl_load_GL_EXT_draw_instanced(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_draw_instanced) return;
    context->DrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC) load(userptr, "glDrawArraysInstancedEXT");
    context->DrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC) load(userptr, "glDrawElementsInstancedEXT");
}
static void glad_gl_load_GL_EXT_draw_range_elements(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_draw_range_elements) return;
    context->DrawRangeElementsEXT = (PFNGLDRAWRANGEELEMENTSEXTPROC) load(userptr, "glDrawRangeElementsEXT");
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
static void glad_gl_load_GL_EXT_gpu_shader4(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_gpu_shader4) return;
    context->BindFragDataLocationEXT = (PFNGLBINDFRAGDATALOCATIONEXTPROC) load(userptr, "glBindFragDataLocationEXT");
    context->GetFragDataLocationEXT = (PFNGLGETFRAGDATALOCATIONEXTPROC) load(userptr, "glGetFragDataLocationEXT");
    context->GetUniformuivEXT = (PFNGLGETUNIFORMUIVEXTPROC) load(userptr, "glGetUniformuivEXT");
    context->GetVertexAttribIivEXT = (PFNGLGETVERTEXATTRIBIIVEXTPROC) load(userptr, "glGetVertexAttribIivEXT");
    context->GetVertexAttribIuivEXT = (PFNGLGETVERTEXATTRIBIUIVEXTPROC) load(userptr, "glGetVertexAttribIuivEXT");
    context->Uniform1uiEXT = (PFNGLUNIFORM1UIEXTPROC) load(userptr, "glUniform1uiEXT");
    context->Uniform1uivEXT = (PFNGLUNIFORM1UIVEXTPROC) load(userptr, "glUniform1uivEXT");
    context->Uniform2uiEXT = (PFNGLUNIFORM2UIEXTPROC) load(userptr, "glUniform2uiEXT");
    context->Uniform2uivEXT = (PFNGLUNIFORM2UIVEXTPROC) load(userptr, "glUniform2uivEXT");
    context->Uniform3uiEXT = (PFNGLUNIFORM3UIEXTPROC) load(userptr, "glUniform3uiEXT");
    context->Uniform3uivEXT = (PFNGLUNIFORM3UIVEXTPROC) load(userptr, "glUniform3uivEXT");
    context->Uniform4uiEXT = (PFNGLUNIFORM4UIEXTPROC) load(userptr, "glUniform4uiEXT");
    context->Uniform4uivEXT = (PFNGLUNIFORM4UIVEXTPROC) load(userptr, "glUniform4uivEXT");
    context->VertexAttribI1iEXT = (PFNGLVERTEXATTRIBI1IEXTPROC) load(userptr, "glVertexAttribI1iEXT");
    context->VertexAttribI1ivEXT = (PFNGLVERTEXATTRIBI1IVEXTPROC) load(userptr, "glVertexAttribI1ivEXT");
    context->VertexAttribI1uiEXT = (PFNGLVERTEXATTRIBI1UIEXTPROC) load(userptr, "glVertexAttribI1uiEXT");
    context->VertexAttribI1uivEXT = (PFNGLVERTEXATTRIBI1UIVEXTPROC) load(userptr, "glVertexAttribI1uivEXT");
    context->VertexAttribI2iEXT = (PFNGLVERTEXATTRIBI2IEXTPROC) load(userptr, "glVertexAttribI2iEXT");
    context->VertexAttribI2ivEXT = (PFNGLVERTEXATTRIBI2IVEXTPROC) load(userptr, "glVertexAttribI2ivEXT");
    context->VertexAttribI2uiEXT = (PFNGLVERTEXATTRIBI2UIEXTPROC) load(userptr, "glVertexAttribI2uiEXT");
    context->VertexAttribI2uivEXT = (PFNGLVERTEXATTRIBI2UIVEXTPROC) load(userptr, "glVertexAttribI2uivEXT");
    context->VertexAttribI3iEXT = (PFNGLVERTEXATTRIBI3IEXTPROC) load(userptr, "glVertexAttribI3iEXT");
    context->VertexAttribI3ivEXT = (PFNGLVERTEXATTRIBI3IVEXTPROC) load(userptr, "glVertexAttribI3ivEXT");
    context->VertexAttribI3uiEXT = (PFNGLVERTEXATTRIBI3UIEXTPROC) load(userptr, "glVertexAttribI3uiEXT");
    context->VertexAttribI3uivEXT = (PFNGLVERTEXATTRIBI3UIVEXTPROC) load(userptr, "glVertexAttribI3uivEXT");
    context->VertexAttribI4bvEXT = (PFNGLVERTEXATTRIBI4BVEXTPROC) load(userptr, "glVertexAttribI4bvEXT");
    context->VertexAttribI4iEXT = (PFNGLVERTEXATTRIBI4IEXTPROC) load(userptr, "glVertexAttribI4iEXT");
    context->VertexAttribI4ivEXT = (PFNGLVERTEXATTRIBI4IVEXTPROC) load(userptr, "glVertexAttribI4ivEXT");
    context->VertexAttribI4svEXT = (PFNGLVERTEXATTRIBI4SVEXTPROC) load(userptr, "glVertexAttribI4svEXT");
    context->VertexAttribI4ubvEXT = (PFNGLVERTEXATTRIBI4UBVEXTPROC) load(userptr, "glVertexAttribI4ubvEXT");
    context->VertexAttribI4uiEXT = (PFNGLVERTEXATTRIBI4UIEXTPROC) load(userptr, "glVertexAttribI4uiEXT");
    context->VertexAttribI4uivEXT = (PFNGLVERTEXATTRIBI4UIVEXTPROC) load(userptr, "glVertexAttribI4uivEXT");
    context->VertexAttribI4usvEXT = (PFNGLVERTEXATTRIBI4USVEXTPROC) load(userptr, "glVertexAttribI4usvEXT");
    context->VertexAttribIPointerEXT = (PFNGLVERTEXATTRIBIPOINTEREXTPROC) load(userptr, "glVertexAttribIPointerEXT");
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
static void glad_gl_load_GL_EXT_provoking_vertex(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_provoking_vertex) return;
    context->ProvokingVertexEXT = (PFNGLPROVOKINGVERTEXEXTPROC) load(userptr, "glProvokingVertexEXT");
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
static void glad_gl_load_GL_EXT_texture_buffer_object(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_texture_buffer_object) return;
    context->TexBufferEXT = (PFNGLTEXBUFFEREXTPROC) load(userptr, "glTexBufferEXT");
}
static void glad_gl_load_GL_EXT_texture_integer(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_texture_integer) return;
    context->ClearColorIiEXT = (PFNGLCLEARCOLORIIEXTPROC) load(userptr, "glClearColorIiEXT");
    context->ClearColorIuiEXT = (PFNGLCLEARCOLORIUIEXTPROC) load(userptr, "glClearColorIuiEXT");
    context->GetTexParameterIivEXT = (PFNGLGETTEXPARAMETERIIVEXTPROC) load(userptr, "glGetTexParameterIivEXT");
    context->GetTexParameterIuivEXT = (PFNGLGETTEXPARAMETERIUIVEXTPROC) load(userptr, "glGetTexParameterIuivEXT");
    context->TexParameterIivEXT = (PFNGLTEXPARAMETERIIVEXTPROC) load(userptr, "glTexParameterIivEXT");
    context->TexParameterIuivEXT = (PFNGLTEXPARAMETERIUIVEXTPROC) load(userptr, "glTexParameterIuivEXT");
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
static void glad_gl_load_GL_EXT_transform_feedback(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->EXT_transform_feedback) return;
    context->BeginTransformFeedbackEXT = (PFNGLBEGINTRANSFORMFEEDBACKEXTPROC) load(userptr, "glBeginTransformFeedbackEXT");
    context->BindBufferBaseEXT = (PFNGLBINDBUFFERBASEEXTPROC) load(userptr, "glBindBufferBaseEXT");
    context->BindBufferOffsetEXT = (PFNGLBINDBUFFEROFFSETEXTPROC) load(userptr, "glBindBufferOffsetEXT");
    context->BindBufferRangeEXT = (PFNGLBINDBUFFERRANGEEXTPROC) load(userptr, "glBindBufferRangeEXT");
    context->EndTransformFeedbackEXT = (PFNGLENDTRANSFORMFEEDBACKEXTPROC) load(userptr, "glEndTransformFeedbackEXT");
    context->GetTransformFeedbackVaryingEXT = (PFNGLGETTRANSFORMFEEDBACKVARYINGEXTPROC) load(userptr, "glGetTransformFeedbackVaryingEXT");
    context->TransformFeedbackVaryingsEXT = (PFNGLTRANSFORMFEEDBACKVARYINGSEXTPROC) load(userptr, "glTransformFeedbackVaryingsEXT");
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
static void glad_gl_load_GL_NVX_conditional_render(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->NVX_conditional_render) return;
    context->BeginConditionalRenderNVX = (PFNGLBEGINCONDITIONALRENDERNVXPROC) load(userptr, "glBeginConditionalRenderNVX");
    context->EndConditionalRenderNVX = (PFNGLENDCONDITIONALRENDERNVXPROC) load(userptr, "glEndConditionalRenderNVX");
}
static void glad_gl_load_GL_NV_conditional_render(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->NV_conditional_render) return;
    context->BeginConditionalRenderNV = (PFNGLBEGINCONDITIONALRENDERNVPROC) load(userptr, "glBeginConditionalRenderNV");
    context->EndConditionalRenderNV = (PFNGLENDCONDITIONALRENDERNVPROC) load(userptr, "glEndConditionalRenderNV");
}
static void glad_gl_load_GL_NV_explicit_multisample(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->NV_explicit_multisample) return;
    context->GetMultisamplefvNV = (PFNGLGETMULTISAMPLEFVNVPROC) load(userptr, "glGetMultisamplefvNV");
    context->SampleMaskIndexedNV = (PFNGLSAMPLEMASKINDEXEDNVPROC) load(userptr, "glSampleMaskIndexedNV");
    context->TexRenderbufferNV = (PFNGLTEXRENDERBUFFERNVPROC) load(userptr, "glTexRenderbufferNV");
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
static void glad_gl_load_GL_NV_transform_feedback(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->NV_transform_feedback) return;
    context->ActiveVaryingNV = (PFNGLACTIVEVARYINGNVPROC) load(userptr, "glActiveVaryingNV");
    context->BeginTransformFeedbackNV = (PFNGLBEGINTRANSFORMFEEDBACKNVPROC) load(userptr, "glBeginTransformFeedbackNV");
    context->BindBufferBaseNV = (PFNGLBINDBUFFERBASENVPROC) load(userptr, "glBindBufferBaseNV");
    context->BindBufferOffsetNV = (PFNGLBINDBUFFEROFFSETNVPROC) load(userptr, "glBindBufferOffsetNV");
    context->BindBufferRangeNV = (PFNGLBINDBUFFERRANGENVPROC) load(userptr, "glBindBufferRangeNV");
    context->EndTransformFeedbackNV = (PFNGLENDTRANSFORMFEEDBACKNVPROC) load(userptr, "glEndTransformFeedbackNV");
    context->GetActiveVaryingNV = (PFNGLGETACTIVEVARYINGNVPROC) load(userptr, "glGetActiveVaryingNV");
    context->GetTransformFeedbackVaryingNV = (PFNGLGETTRANSFORMFEEDBACKVARYINGNVPROC) load(userptr, "glGetTransformFeedbackVaryingNV");
    context->GetVaryingLocationNV = (PFNGLGETVARYINGLOCATIONNVPROC) load(userptr, "glGetVaryingLocationNV");
    context->TransformFeedbackAttribsNV = (PFNGLTRANSFORMFEEDBACKATTRIBSNVPROC) load(userptr, "glTransformFeedbackAttribsNV");
    context->TransformFeedbackStreamAttribsNV = (PFNGLTRANSFORMFEEDBACKSTREAMATTRIBSNVPROC) load(userptr, "glTransformFeedbackStreamAttribsNV");
    context->TransformFeedbackVaryingsNV = (PFNGLTRANSFORMFEEDBACKVARYINGSNVPROC) load(userptr, "glTransformFeedbackVaryingsNV");
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
static void glad_gl_load_GL_NV_vertex_program4(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->NV_vertex_program4) return;
    context->GetVertexAttribIivEXT = (PFNGLGETVERTEXATTRIBIIVEXTPROC) load(userptr, "glGetVertexAttribIivEXT");
    context->GetVertexAttribIuivEXT = (PFNGLGETVERTEXATTRIBIUIVEXTPROC) load(userptr, "glGetVertexAttribIuivEXT");
    context->VertexAttribI1iEXT = (PFNGLVERTEXATTRIBI1IEXTPROC) load(userptr, "glVertexAttribI1iEXT");
    context->VertexAttribI1ivEXT = (PFNGLVERTEXATTRIBI1IVEXTPROC) load(userptr, "glVertexAttribI1ivEXT");
    context->VertexAttribI1uiEXT = (PFNGLVERTEXATTRIBI1UIEXTPROC) load(userptr, "glVertexAttribI1uiEXT");
    context->VertexAttribI1uivEXT = (PFNGLVERTEXATTRIBI1UIVEXTPROC) load(userptr, "glVertexAttribI1uivEXT");
    context->VertexAttribI2iEXT = (PFNGLVERTEXATTRIBI2IEXTPROC) load(userptr, "glVertexAttribI2iEXT");
    context->VertexAttribI2ivEXT = (PFNGLVERTEXATTRIBI2IVEXTPROC) load(userptr, "glVertexAttribI2ivEXT");
    context->VertexAttribI2uiEXT = (PFNGLVERTEXATTRIBI2UIEXTPROC) load(userptr, "glVertexAttribI2uiEXT");
    context->VertexAttribI2uivEXT = (PFNGLVERTEXATTRIBI2UIVEXTPROC) load(userptr, "glVertexAttribI2uivEXT");
    context->VertexAttribI3iEXT = (PFNGLVERTEXATTRIBI3IEXTPROC) load(userptr, "glVertexAttribI3iEXT");
    context->VertexAttribI3ivEXT = (PFNGLVERTEXATTRIBI3IVEXTPROC) load(userptr, "glVertexAttribI3ivEXT");
    context->VertexAttribI3uiEXT = (PFNGLVERTEXATTRIBI3UIEXTPROC) load(userptr, "glVertexAttribI3uiEXT");
    context->VertexAttribI3uivEXT = (PFNGLVERTEXATTRIBI3UIVEXTPROC) load(userptr, "glVertexAttribI3uivEXT");
    context->VertexAttribI4bvEXT = (PFNGLVERTEXATTRIBI4BVEXTPROC) load(userptr, "glVertexAttribI4bvEXT");
    context->VertexAttribI4iEXT = (PFNGLVERTEXATTRIBI4IEXTPROC) load(userptr, "glVertexAttribI4iEXT");
    context->VertexAttribI4ivEXT = (PFNGLVERTEXATTRIBI4IVEXTPROC) load(userptr, "glVertexAttribI4ivEXT");
    context->VertexAttribI4svEXT = (PFNGLVERTEXATTRIBI4SVEXTPROC) load(userptr, "glVertexAttribI4svEXT");
    context->VertexAttribI4ubvEXT = (PFNGLVERTEXATTRIBI4UBVEXTPROC) load(userptr, "glVertexAttribI4ubvEXT");
    context->VertexAttribI4uiEXT = (PFNGLVERTEXATTRIBI4UIEXTPROC) load(userptr, "glVertexAttribI4uiEXT");
    context->VertexAttribI4uivEXT = (PFNGLVERTEXATTRIBI4UIVEXTPROC) load(userptr, "glVertexAttribI4uivEXT");
    context->VertexAttribI4usvEXT = (PFNGLVERTEXATTRIBI4USVEXTPROC) load(userptr, "glVertexAttribI4usvEXT");
    context->VertexAttribIPointerEXT = (PFNGLVERTEXATTRIBIPOINTEREXTPROC) load(userptr, "glVertexAttribIPointerEXT");
}
static void glad_gl_load_GL_SGIS_point_parameters(GladGLContext *context, GLADuserptrloadfunc load, void* userptr) {
    if(!context->SGIS_point_parameters) return;
    context->PointParameterfSGIS = (PFNGLPOINTPARAMETERFSGISPROC) load(userptr, "glPointParameterfSGIS");
    context->PointParameterfvSGIS = (PFNGLPOINTPARAMETERFVSGISPROC) load(userptr, "glPointParameterfvSGIS");
}


static void glad_gl_resolve_aliases(GladGLContext *context) {
    if (context->ActiveTexture == NULL && context->ActiveTextureARB != NULL) context->ActiveTexture = (PFNGLACTIVETEXTUREPROC)context->ActiveTextureARB;
    if (context->ActiveTextureARB == NULL && context->ActiveTexture != NULL) context->ActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)context->ActiveTexture;
    if (context->AttachObjectARB == NULL && context->AttachShader != NULL) context->AttachObjectARB = (PFNGLATTACHOBJECTARBPROC)context->AttachShader;
    if (context->AttachShader == NULL && context->AttachObjectARB != NULL) context->AttachShader = (PFNGLATTACHSHADERPROC)context->AttachObjectARB;
    if (context->BeginConditionalRender == NULL && context->BeginConditionalRenderNV != NULL) context->BeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC)context->BeginConditionalRenderNV;
    if (context->BeginConditionalRenderNV == NULL && context->BeginConditionalRender != NULL) context->BeginConditionalRenderNV = (PFNGLBEGINCONDITIONALRENDERNVPROC)context->BeginConditionalRender;
    if (context->BeginQuery == NULL && context->BeginQueryARB != NULL) context->BeginQuery = (PFNGLBEGINQUERYPROC)context->BeginQueryARB;
    if (context->BeginQueryARB == NULL && context->BeginQuery != NULL) context->BeginQueryARB = (PFNGLBEGINQUERYARBPROC)context->BeginQuery;
    if (context->BeginTransformFeedback == NULL && context->BeginTransformFeedbackEXT != NULL) context->BeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)context->BeginTransformFeedbackEXT;
    if (context->BeginTransformFeedback == NULL && context->BeginTransformFeedbackNV != NULL) context->BeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)context->BeginTransformFeedbackNV;
    if (context->BeginTransformFeedbackEXT == NULL && context->BeginTransformFeedback != NULL) context->BeginTransformFeedbackEXT = (PFNGLBEGINTRANSFORMFEEDBACKEXTPROC)context->BeginTransformFeedback;
    if (context->BeginTransformFeedbackEXT == NULL && context->BeginTransformFeedbackNV != NULL) context->BeginTransformFeedbackEXT = (PFNGLBEGINTRANSFORMFEEDBACKEXTPROC)context->BeginTransformFeedbackNV;
    if (context->BeginTransformFeedbackNV == NULL && context->BeginTransformFeedback != NULL) context->BeginTransformFeedbackNV = (PFNGLBEGINTRANSFORMFEEDBACKNVPROC)context->BeginTransformFeedback;
    if (context->BeginTransformFeedbackNV == NULL && context->BeginTransformFeedbackEXT != NULL) context->BeginTransformFeedbackNV = (PFNGLBEGINTRANSFORMFEEDBACKNVPROC)context->BeginTransformFeedbackEXT;
    if (context->BindAttribLocation == NULL && context->BindAttribLocationARB != NULL) context->BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)context->BindAttribLocationARB;
    if (context->BindAttribLocationARB == NULL && context->BindAttribLocation != NULL) context->BindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC)context->BindAttribLocation;
    if (context->BindBuffer == NULL && context->BindBufferARB != NULL) context->BindBuffer = (PFNGLBINDBUFFERPROC)context->BindBufferARB;
    if (context->BindBufferARB == NULL && context->BindBuffer != NULL) context->BindBufferARB = (PFNGLBINDBUFFERARBPROC)context->BindBuffer;
    if (context->BindBufferBase == NULL && context->BindBufferBaseEXT != NULL) context->BindBufferBase = (PFNGLBINDBUFFERBASEPROC)context->BindBufferBaseEXT;
    if (context->BindBufferBase == NULL && context->BindBufferBaseNV != NULL) context->BindBufferBase = (PFNGLBINDBUFFERBASEPROC)context->BindBufferBaseNV;
    if (context->BindBufferBaseEXT == NULL && context->BindBufferBase != NULL) context->BindBufferBaseEXT = (PFNGLBINDBUFFERBASEEXTPROC)context->BindBufferBase;
    if (context->BindBufferBaseEXT == NULL && context->BindBufferBaseNV != NULL) context->BindBufferBaseEXT = (PFNGLBINDBUFFERBASEEXTPROC)context->BindBufferBaseNV;
    if (context->BindBufferBaseNV == NULL && context->BindBufferBase != NULL) context->BindBufferBaseNV = (PFNGLBINDBUFFERBASENVPROC)context->BindBufferBase;
    if (context->BindBufferBaseNV == NULL && context->BindBufferBaseEXT != NULL) context->BindBufferBaseNV = (PFNGLBINDBUFFERBASENVPROC)context->BindBufferBaseEXT;
    if (context->BindBufferOffsetEXT == NULL && context->BindBufferOffsetNV != NULL) context->BindBufferOffsetEXT = (PFNGLBINDBUFFEROFFSETEXTPROC)context->BindBufferOffsetNV;
    if (context->BindBufferOffsetNV == NULL && context->BindBufferOffsetEXT != NULL) context->BindBufferOffsetNV = (PFNGLBINDBUFFEROFFSETNVPROC)context->BindBufferOffsetEXT;
    if (context->BindBufferRange == NULL && context->BindBufferRangeEXT != NULL) context->BindBufferRange = (PFNGLBINDBUFFERRANGEPROC)context->BindBufferRangeEXT;
    if (context->BindBufferRange == NULL && context->BindBufferRangeNV != NULL) context->BindBufferRange = (PFNGLBINDBUFFERRANGEPROC)context->BindBufferRangeNV;
    if (context->BindBufferRangeEXT == NULL && context->BindBufferRange != NULL) context->BindBufferRangeEXT = (PFNGLBINDBUFFERRANGEEXTPROC)context->BindBufferRange;
    if (context->BindBufferRangeEXT == NULL && context->BindBufferRangeNV != NULL) context->BindBufferRangeEXT = (PFNGLBINDBUFFERRANGEEXTPROC)context->BindBufferRangeNV;
    if (context->BindBufferRangeNV == NULL && context->BindBufferRange != NULL) context->BindBufferRangeNV = (PFNGLBINDBUFFERRANGENVPROC)context->BindBufferRange;
    if (context->BindBufferRangeNV == NULL && context->BindBufferRangeEXT != NULL) context->BindBufferRangeNV = (PFNGLBINDBUFFERRANGENVPROC)context->BindBufferRangeEXT;
    if (context->BindFragDataLocation == NULL && context->BindFragDataLocationEXT != NULL) context->BindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)context->BindFragDataLocationEXT;
    if (context->BindFragDataLocationEXT == NULL && context->BindFragDataLocation != NULL) context->BindFragDataLocationEXT = (PFNGLBINDFRAGDATALOCATIONEXTPROC)context->BindFragDataLocation;
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
    if (context->ClampColor == NULL && context->ClampColorARB != NULL) context->ClampColor = (PFNGLCLAMPCOLORPROC)context->ClampColorARB;
    if (context->ClampColorARB == NULL && context->ClampColor != NULL) context->ClampColorARB = (PFNGLCLAMPCOLORARBPROC)context->ClampColor;
    if (context->ColorMaski == NULL && context->ColorMaskIndexedEXT != NULL) context->ColorMaski = (PFNGLCOLORMASKIPROC)context->ColorMaskIndexedEXT;
    if (context->ColorMaskIndexedEXT == NULL && context->ColorMaski != NULL) context->ColorMaskIndexedEXT = (PFNGLCOLORMASKINDEXEDEXTPROC)context->ColorMaski;
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
    if (context->DeleteVertexArrays == NULL && context->DeleteVertexArraysAPPLE != NULL) context->DeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)context->DeleteVertexArraysAPPLE;
    if (context->DeleteVertexArraysAPPLE == NULL && context->DeleteVertexArrays != NULL) context->DeleteVertexArraysAPPLE = (PFNGLDELETEVERTEXARRAYSAPPLEPROC)context->DeleteVertexArrays;
    if (context->DetachObjectARB == NULL && context->DetachShader != NULL) context->DetachObjectARB = (PFNGLDETACHOBJECTARBPROC)context->DetachShader;
    if (context->DetachShader == NULL && context->DetachObjectARB != NULL) context->DetachShader = (PFNGLDETACHSHADERPROC)context->DetachObjectARB;
    if (context->Disablei == NULL && context->DisableIndexedEXT != NULL) context->Disablei = (PFNGLDISABLEIPROC)context->DisableIndexedEXT;
    if (context->DisableIndexedEXT == NULL && context->Disablei != NULL) context->DisableIndexedEXT = (PFNGLDISABLEINDEXEDEXTPROC)context->Disablei;
    if (context->DisableVertexAttribArray == NULL && context->DisableVertexAttribArrayARB != NULL) context->DisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)context->DisableVertexAttribArrayARB;
    if (context->DisableVertexAttribArrayARB == NULL && context->DisableVertexAttribArray != NULL) context->DisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)context->DisableVertexAttribArray;
    if (context->DrawArrays == NULL && context->DrawArraysEXT != NULL) context->DrawArrays = (PFNGLDRAWARRAYSPROC)context->DrawArraysEXT;
    if (context->DrawArraysEXT == NULL && context->DrawArrays != NULL) context->DrawArraysEXT = (PFNGLDRAWARRAYSEXTPROC)context->DrawArrays;
    if (context->DrawArraysInstanced == NULL && context->DrawArraysInstancedARB != NULL) context->DrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)context->DrawArraysInstancedARB;
    if (context->DrawArraysInstanced == NULL && context->DrawArraysInstancedEXT != NULL) context->DrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)context->DrawArraysInstancedEXT;
    if (context->DrawArraysInstancedARB == NULL && context->DrawArraysInstanced != NULL) context->DrawArraysInstancedARB = (PFNGLDRAWARRAYSINSTANCEDARBPROC)context->DrawArraysInstanced;
    if (context->DrawArraysInstancedARB == NULL && context->DrawArraysInstancedEXT != NULL) context->DrawArraysInstancedARB = (PFNGLDRAWARRAYSINSTANCEDARBPROC)context->DrawArraysInstancedEXT;
    if (context->DrawArraysInstancedEXT == NULL && context->DrawArraysInstanced != NULL) context->DrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)context->DrawArraysInstanced;
    if (context->DrawArraysInstancedEXT == NULL && context->DrawArraysInstancedARB != NULL) context->DrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC)context->DrawArraysInstancedARB;
    if (context->DrawBuffers == NULL && context->DrawBuffersARB != NULL) context->DrawBuffers = (PFNGLDRAWBUFFERSPROC)context->DrawBuffersARB;
    if (context->DrawBuffers == NULL && context->DrawBuffersATI != NULL) context->DrawBuffers = (PFNGLDRAWBUFFERSPROC)context->DrawBuffersATI;
    if (context->DrawBuffersARB == NULL && context->DrawBuffers != NULL) context->DrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC)context->DrawBuffers;
    if (context->DrawBuffersARB == NULL && context->DrawBuffersATI != NULL) context->DrawBuffersARB = (PFNGLDRAWBUFFERSARBPROC)context->DrawBuffersATI;
    if (context->DrawBuffersATI == NULL && context->DrawBuffers != NULL) context->DrawBuffersATI = (PFNGLDRAWBUFFERSATIPROC)context->DrawBuffers;
    if (context->DrawBuffersATI == NULL && context->DrawBuffersARB != NULL) context->DrawBuffersATI = (PFNGLDRAWBUFFERSATIPROC)context->DrawBuffersARB;
    if (context->DrawElementsInstanced == NULL && context->DrawElementsInstancedARB != NULL) context->DrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)context->DrawElementsInstancedARB;
    if (context->DrawElementsInstanced == NULL && context->DrawElementsInstancedEXT != NULL) context->DrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)context->DrawElementsInstancedEXT;
    if (context->DrawElementsInstancedARB == NULL && context->DrawElementsInstanced != NULL) context->DrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC)context->DrawElementsInstanced;
    if (context->DrawElementsInstancedARB == NULL && context->DrawElementsInstancedEXT != NULL) context->DrawElementsInstancedARB = (PFNGLDRAWELEMENTSINSTANCEDARBPROC)context->DrawElementsInstancedEXT;
    if (context->DrawElementsInstancedEXT == NULL && context->DrawElementsInstanced != NULL) context->DrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)context->DrawElementsInstanced;
    if (context->DrawElementsInstancedEXT == NULL && context->DrawElementsInstancedARB != NULL) context->DrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXTPROC)context->DrawElementsInstancedARB;
    if (context->DrawRangeElements == NULL && context->DrawRangeElementsEXT != NULL) context->DrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)context->DrawRangeElementsEXT;
    if (context->DrawRangeElementsEXT == NULL && context->DrawRangeElements != NULL) context->DrawRangeElementsEXT = (PFNGLDRAWRANGEELEMENTSEXTPROC)context->DrawRangeElements;
    if (context->Enablei == NULL && context->EnableIndexedEXT != NULL) context->Enablei = (PFNGLENABLEIPROC)context->EnableIndexedEXT;
    if (context->EnableIndexedEXT == NULL && context->Enablei != NULL) context->EnableIndexedEXT = (PFNGLENABLEINDEXEDEXTPROC)context->Enablei;
    if (context->EnableVertexAttribArray == NULL && context->EnableVertexAttribArrayARB != NULL) context->EnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)context->EnableVertexAttribArrayARB;
    if (context->EnableVertexAttribArrayARB == NULL && context->EnableVertexAttribArray != NULL) context->EnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)context->EnableVertexAttribArray;
    if (context->EndConditionalRender == NULL && context->EndConditionalRenderNV != NULL) context->EndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)context->EndConditionalRenderNV;
    if (context->EndConditionalRender == NULL && context->EndConditionalRenderNVX != NULL) context->EndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)context->EndConditionalRenderNVX;
    if (context->EndConditionalRenderNV == NULL && context->EndConditionalRender != NULL) context->EndConditionalRenderNV = (PFNGLENDCONDITIONALRENDERNVPROC)context->EndConditionalRender;
    if (context->EndConditionalRenderNV == NULL && context->EndConditionalRenderNVX != NULL) context->EndConditionalRenderNV = (PFNGLENDCONDITIONALRENDERNVPROC)context->EndConditionalRenderNVX;
    if (context->EndConditionalRenderNVX == NULL && context->EndConditionalRender != NULL) context->EndConditionalRenderNVX = (PFNGLENDCONDITIONALRENDERNVXPROC)context->EndConditionalRender;
    if (context->EndConditionalRenderNVX == NULL && context->EndConditionalRenderNV != NULL) context->EndConditionalRenderNVX = (PFNGLENDCONDITIONALRENDERNVXPROC)context->EndConditionalRenderNV;
    if (context->EndQuery == NULL && context->EndQueryARB != NULL) context->EndQuery = (PFNGLENDQUERYPROC)context->EndQueryARB;
    if (context->EndQueryARB == NULL && context->EndQuery != NULL) context->EndQueryARB = (PFNGLENDQUERYARBPROC)context->EndQuery;
    if (context->EndTransformFeedback == NULL && context->EndTransformFeedbackEXT != NULL) context->EndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)context->EndTransformFeedbackEXT;
    if (context->EndTransformFeedback == NULL && context->EndTransformFeedbackNV != NULL) context->EndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)context->EndTransformFeedbackNV;
    if (context->EndTransformFeedbackEXT == NULL && context->EndTransformFeedback != NULL) context->EndTransformFeedbackEXT = (PFNGLENDTRANSFORMFEEDBACKEXTPROC)context->EndTransformFeedback;
    if (context->EndTransformFeedbackEXT == NULL && context->EndTransformFeedbackNV != NULL) context->EndTransformFeedbackEXT = (PFNGLENDTRANSFORMFEEDBACKEXTPROC)context->EndTransformFeedbackNV;
    if (context->EndTransformFeedbackNV == NULL && context->EndTransformFeedback != NULL) context->EndTransformFeedbackNV = (PFNGLENDTRANSFORMFEEDBACKNVPROC)context->EndTransformFeedback;
    if (context->EndTransformFeedbackNV == NULL && context->EndTransformFeedbackEXT != NULL) context->EndTransformFeedbackNV = (PFNGLENDTRANSFORMFEEDBACKNVPROC)context->EndTransformFeedbackEXT;
    if (context->FlushMappedBufferRange == NULL && context->FlushMappedBufferRangeAPPLE != NULL) context->FlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)context->FlushMappedBufferRangeAPPLE;
    if (context->FlushMappedBufferRangeAPPLE == NULL && context->FlushMappedBufferRange != NULL) context->FlushMappedBufferRangeAPPLE = (PFNGLFLUSHMAPPEDBUFFERRANGEAPPLEPROC)context->FlushMappedBufferRange;
    if (context->FramebufferRenderbuffer == NULL && context->FramebufferRenderbufferEXT != NULL) context->FramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)context->FramebufferRenderbufferEXT;
    if (context->FramebufferRenderbufferEXT == NULL && context->FramebufferRenderbuffer != NULL) context->FramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)context->FramebufferRenderbuffer;
    if (context->FramebufferTexture == NULL && context->FramebufferTextureARB != NULL) context->FramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)context->FramebufferTextureARB;
    if (context->FramebufferTexture == NULL && context->FramebufferTextureEXT != NULL) context->FramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)context->FramebufferTextureEXT;
    if (context->FramebufferTexture1D == NULL && context->FramebufferTexture1DEXT != NULL) context->FramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)context->FramebufferTexture1DEXT;
    if (context->FramebufferTexture1DEXT == NULL && context->FramebufferTexture1D != NULL) context->FramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)context->FramebufferTexture1D;
    if (context->FramebufferTexture2D == NULL && context->FramebufferTexture2DEXT != NULL) context->FramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)context->FramebufferTexture2DEXT;
    if (context->FramebufferTexture2DEXT == NULL && context->FramebufferTexture2D != NULL) context->FramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)context->FramebufferTexture2D;
    if (context->FramebufferTexture3D == NULL && context->FramebufferTexture3DEXT != NULL) context->FramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)context->FramebufferTexture3DEXT;
    if (context->FramebufferTexture3DEXT == NULL && context->FramebufferTexture3D != NULL) context->FramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)context->FramebufferTexture3D;
    if (context->FramebufferTextureARB == NULL && context->FramebufferTexture != NULL) context->FramebufferTextureARB = (PFNGLFRAMEBUFFERTEXTUREARBPROC)context->FramebufferTexture;
    if (context->FramebufferTextureARB == NULL && context->FramebufferTextureEXT != NULL) context->FramebufferTextureARB = (PFNGLFRAMEBUFFERTEXTUREARBPROC)context->FramebufferTextureEXT;
    if (context->FramebufferTextureEXT == NULL && context->FramebufferTexture != NULL) context->FramebufferTextureEXT = (PFNGLFRAMEBUFFERTEXTUREEXTPROC)context->FramebufferTexture;
    if (context->FramebufferTextureEXT == NULL && context->FramebufferTextureARB != NULL) context->FramebufferTextureEXT = (PFNGLFRAMEBUFFERTEXTUREEXTPROC)context->FramebufferTextureARB;
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
    if (context->GenVertexArrays == NULL && context->GenVertexArraysAPPLE != NULL) context->GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)context->GenVertexArraysAPPLE;
    if (context->GenVertexArraysAPPLE == NULL && context->GenVertexArrays != NULL) context->GenVertexArraysAPPLE = (PFNGLGENVERTEXARRAYSAPPLEPROC)context->GenVertexArrays;
    if (context->GetActiveAttrib == NULL && context->GetActiveAttribARB != NULL) context->GetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)context->GetActiveAttribARB;
    if (context->GetActiveAttribARB == NULL && context->GetActiveAttrib != NULL) context->GetActiveAttribARB = (PFNGLGETACTIVEATTRIBARBPROC)context->GetActiveAttrib;
    if (context->GetActiveUniform == NULL && context->GetActiveUniformARB != NULL) context->GetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)context->GetActiveUniformARB;
    if (context->GetActiveUniformARB == NULL && context->GetActiveUniform != NULL) context->GetActiveUniformARB = (PFNGLGETACTIVEUNIFORMARBPROC)context->GetActiveUniform;
    if (context->GetAttribLocation == NULL && context->GetAttribLocationARB != NULL) context->GetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)context->GetAttribLocationARB;
    if (context->GetAttribLocationARB == NULL && context->GetAttribLocation != NULL) context->GetAttribLocationARB = (PFNGLGETATTRIBLOCATIONARBPROC)context->GetAttribLocation;
    if (context->GetBooleani_v == NULL && context->GetBooleanIndexedvEXT != NULL) context->GetBooleani_v = (PFNGLGETBOOLEANI_VPROC)context->GetBooleanIndexedvEXT;
    if (context->GetBooleanIndexedvEXT == NULL && context->GetBooleani_v != NULL) context->GetBooleanIndexedvEXT = (PFNGLGETBOOLEANINDEXEDVEXTPROC)context->GetBooleani_v;
    if (context->GetBufferParameteriv == NULL && context->GetBufferParameterivARB != NULL) context->GetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)context->GetBufferParameterivARB;
    if (context->GetBufferParameterivARB == NULL && context->GetBufferParameteriv != NULL) context->GetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)context->GetBufferParameteriv;
    if (context->GetBufferPointerv == NULL && context->GetBufferPointervARB != NULL) context->GetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)context->GetBufferPointervARB;
    if (context->GetBufferPointervARB == NULL && context->GetBufferPointerv != NULL) context->GetBufferPointervARB = (PFNGLGETBUFFERPOINTERVARBPROC)context->GetBufferPointerv;
    if (context->GetBufferSubData == NULL && context->GetBufferSubDataARB != NULL) context->GetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)context->GetBufferSubDataARB;
    if (context->GetBufferSubDataARB == NULL && context->GetBufferSubData != NULL) context->GetBufferSubDataARB = (PFNGLGETBUFFERSUBDATAARBPROC)context->GetBufferSubData;
    if (context->GetCompressedTexImage == NULL && context->GetCompressedTexImageARB != NULL) context->GetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)context->GetCompressedTexImageARB;
    if (context->GetCompressedTexImageARB == NULL && context->GetCompressedTexImage != NULL) context->GetCompressedTexImageARB = (PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)context->GetCompressedTexImage;
    if (context->GetFragDataLocation == NULL && context->GetFragDataLocationEXT != NULL) context->GetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)context->GetFragDataLocationEXT;
    if (context->GetFragDataLocationEXT == NULL && context->GetFragDataLocation != NULL) context->GetFragDataLocationEXT = (PFNGLGETFRAGDATALOCATIONEXTPROC)context->GetFragDataLocation;
    if (context->GetFramebufferAttachmentParameteriv == NULL && context->GetFramebufferAttachmentParameterivEXT != NULL) context->GetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)context->GetFramebufferAttachmentParameterivEXT;
    if (context->GetFramebufferAttachmentParameterivEXT == NULL && context->GetFramebufferAttachmentParameteriv != NULL) context->GetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)context->GetFramebufferAttachmentParameteriv;
    if (context->GetIntegeri_v == NULL && context->GetIntegerIndexedvEXT != NULL) context->GetIntegeri_v = (PFNGLGETINTEGERI_VPROC)context->GetIntegerIndexedvEXT;
    if (context->GetIntegerIndexedvEXT == NULL && context->GetIntegeri_v != NULL) context->GetIntegerIndexedvEXT = (PFNGLGETINTEGERINDEXEDVEXTPROC)context->GetIntegeri_v;
    if (context->GetMultisamplefv == NULL && context->GetMultisamplefvNV != NULL) context->GetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)context->GetMultisamplefvNV;
    if (context->GetMultisamplefvNV == NULL && context->GetMultisamplefv != NULL) context->GetMultisamplefvNV = (PFNGLGETMULTISAMPLEFVNVPROC)context->GetMultisamplefv;
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
    if (context->GetTexParameterIiv == NULL && context->GetTexParameterIivEXT != NULL) context->GetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)context->GetTexParameterIivEXT;
    if (context->GetTexParameterIivEXT == NULL && context->GetTexParameterIiv != NULL) context->GetTexParameterIivEXT = (PFNGLGETTEXPARAMETERIIVEXTPROC)context->GetTexParameterIiv;
    if (context->GetTexParameterIuiv == NULL && context->GetTexParameterIuivEXT != NULL) context->GetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)context->GetTexParameterIuivEXT;
    if (context->GetTexParameterIuivEXT == NULL && context->GetTexParameterIuiv != NULL) context->GetTexParameterIuivEXT = (PFNGLGETTEXPARAMETERIUIVEXTPROC)context->GetTexParameterIuiv;
    if (context->GetTransformFeedbackVarying == NULL && context->GetTransformFeedbackVaryingEXT != NULL) context->GetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)context->GetTransformFeedbackVaryingEXT;
    if (context->GetTransformFeedbackVaryingEXT == NULL && context->GetTransformFeedbackVarying != NULL) context->GetTransformFeedbackVaryingEXT = (PFNGLGETTRANSFORMFEEDBACKVARYINGEXTPROC)context->GetTransformFeedbackVarying;
    if (context->GetUniformfv == NULL && context->GetUniformfvARB != NULL) context->GetUniformfv = (PFNGLGETUNIFORMFVPROC)context->GetUniformfvARB;
    if (context->GetUniformfvARB == NULL && context->GetUniformfv != NULL) context->GetUniformfvARB = (PFNGLGETUNIFORMFVARBPROC)context->GetUniformfv;
    if (context->GetUniformiv == NULL && context->GetUniformivARB != NULL) context->GetUniformiv = (PFNGLGETUNIFORMIVPROC)context->GetUniformivARB;
    if (context->GetUniformivARB == NULL && context->GetUniformiv != NULL) context->GetUniformivARB = (PFNGLGETUNIFORMIVARBPROC)context->GetUniformiv;
    if (context->GetUniformLocation == NULL && context->GetUniformLocationARB != NULL) context->GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)context->GetUniformLocationARB;
    if (context->GetUniformLocationARB == NULL && context->GetUniformLocation != NULL) context->GetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC)context->GetUniformLocation;
    if (context->GetUniformuiv == NULL && context->GetUniformuivEXT != NULL) context->GetUniformuiv = (PFNGLGETUNIFORMUIVPROC)context->GetUniformuivEXT;
    if (context->GetUniformuivEXT == NULL && context->GetUniformuiv != NULL) context->GetUniformuivEXT = (PFNGLGETUNIFORMUIVEXTPROC)context->GetUniformuiv;
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
    if (context->GetVertexAttribIiv == NULL && context->GetVertexAttribIivEXT != NULL) context->GetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)context->GetVertexAttribIivEXT;
    if (context->GetVertexAttribIivEXT == NULL && context->GetVertexAttribIiv != NULL) context->GetVertexAttribIivEXT = (PFNGLGETVERTEXATTRIBIIVEXTPROC)context->GetVertexAttribIiv;
    if (context->GetVertexAttribIuiv == NULL && context->GetVertexAttribIuivEXT != NULL) context->GetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)context->GetVertexAttribIuivEXT;
    if (context->GetVertexAttribIuivEXT == NULL && context->GetVertexAttribIuiv != NULL) context->GetVertexAttribIuivEXT = (PFNGLGETVERTEXATTRIBIUIVEXTPROC)context->GetVertexAttribIuiv;
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
    if (context->IsEnabledi == NULL && context->IsEnabledIndexedEXT != NULL) context->IsEnabledi = (PFNGLISENABLEDIPROC)context->IsEnabledIndexedEXT;
    if (context->IsEnabledIndexedEXT == NULL && context->IsEnabledi != NULL) context->IsEnabledIndexedEXT = (PFNGLISENABLEDINDEXEDEXTPROC)context->IsEnabledi;
    if (context->IsFramebuffer == NULL && context->IsFramebufferEXT != NULL) context->IsFramebuffer = (PFNGLISFRAMEBUFFERPROC)context->IsFramebufferEXT;
    if (context->IsFramebufferEXT == NULL && context->IsFramebuffer != NULL) context->IsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)context->IsFramebuffer;
    if (context->IsProgramARB == NULL && context->IsProgramNV != NULL) context->IsProgramARB = (PFNGLISPROGRAMARBPROC)context->IsProgramNV;
    if (context->IsProgramNV == NULL && context->IsProgramARB != NULL) context->IsProgramNV = (PFNGLISPROGRAMNVPROC)context->IsProgramARB;
    if (context->IsQuery == NULL && context->IsQueryARB != NULL) context->IsQuery = (PFNGLISQUERYPROC)context->IsQueryARB;
    if (context->IsQueryARB == NULL && context->IsQuery != NULL) context->IsQueryARB = (PFNGLISQUERYARBPROC)context->IsQuery;
    if (context->IsRenderbuffer == NULL && context->IsRenderbufferEXT != NULL) context->IsRenderbuffer = (PFNGLISRENDERBUFFERPROC)context->IsRenderbufferEXT;
    if (context->IsRenderbufferEXT == NULL && context->IsRenderbuffer != NULL) context->IsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)context->IsRenderbuffer;
    if (context->IsVertexArray == NULL && context->IsVertexArrayAPPLE != NULL) context->IsVertexArray = (PFNGLISVERTEXARRAYPROC)context->IsVertexArrayAPPLE;
    if (context->IsVertexArrayAPPLE == NULL && context->IsVertexArray != NULL) context->IsVertexArrayAPPLE = (PFNGLISVERTEXARRAYAPPLEPROC)context->IsVertexArray;
    if (context->LinkProgram == NULL && context->LinkProgramARB != NULL) context->LinkProgram = (PFNGLLINKPROGRAMPROC)context->LinkProgramARB;
    if (context->LinkProgramARB == NULL && context->LinkProgram != NULL) context->LinkProgramARB = (PFNGLLINKPROGRAMARBPROC)context->LinkProgram;
    if (context->MapBuffer == NULL && context->MapBufferARB != NULL) context->MapBuffer = (PFNGLMAPBUFFERPROC)context->MapBufferARB;
    if (context->MapBufferARB == NULL && context->MapBuffer != NULL) context->MapBufferARB = (PFNGLMAPBUFFERARBPROC)context->MapBuffer;
    if (context->MultiDrawArrays == NULL && context->MultiDrawArraysEXT != NULL) context->MultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)context->MultiDrawArraysEXT;
    if (context->MultiDrawArraysEXT == NULL && context->MultiDrawArrays != NULL) context->MultiDrawArraysEXT = (PFNGLMULTIDRAWARRAYSEXTPROC)context->MultiDrawArrays;
    if (context->MultiDrawElements == NULL && context->MultiDrawElementsEXT != NULL) context->MultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)context->MultiDrawElementsEXT;
    if (context->MultiDrawElementsEXT == NULL && context->MultiDrawElements != NULL) context->MultiDrawElementsEXT = (PFNGLMULTIDRAWELEMENTSEXTPROC)context->MultiDrawElements;
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
    if (context->ProvokingVertex == NULL && context->ProvokingVertexEXT != NULL) context->ProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)context->ProvokingVertexEXT;
    if (context->ProvokingVertexEXT == NULL && context->ProvokingVertex != NULL) context->ProvokingVertexEXT = (PFNGLPROVOKINGVERTEXEXTPROC)context->ProvokingVertex;
    if (context->RenderbufferStorage == NULL && context->RenderbufferStorageEXT != NULL) context->RenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)context->RenderbufferStorageEXT;
    if (context->RenderbufferStorageEXT == NULL && context->RenderbufferStorage != NULL) context->RenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)context->RenderbufferStorage;
    if (context->RenderbufferStorageMultisample == NULL && context->RenderbufferStorageMultisampleEXT != NULL) context->RenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)context->RenderbufferStorageMultisampleEXT;
    if (context->RenderbufferStorageMultisampleEXT == NULL && context->RenderbufferStorageMultisample != NULL) context->RenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)context->RenderbufferStorageMultisample;
    if (context->SampleCoverage == NULL && context->SampleCoverageARB != NULL) context->SampleCoverage = (PFNGLSAMPLECOVERAGEPROC)context->SampleCoverageARB;
    if (context->SampleCoverageARB == NULL && context->SampleCoverage != NULL) context->SampleCoverageARB = (PFNGLSAMPLECOVERAGEARBPROC)context->SampleCoverage;
    if (context->ShaderSource == NULL && context->ShaderSourceARB != NULL) context->ShaderSource = (PFNGLSHADERSOURCEPROC)context->ShaderSourceARB;
    if (context->ShaderSourceARB == NULL && context->ShaderSource != NULL) context->ShaderSourceARB = (PFNGLSHADERSOURCEARBPROC)context->ShaderSource;
    if (context->StencilOpSeparate == NULL && context->StencilOpSeparateATI != NULL) context->StencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)context->StencilOpSeparateATI;
    if (context->StencilOpSeparateATI == NULL && context->StencilOpSeparate != NULL) context->StencilOpSeparateATI = (PFNGLSTENCILOPSEPARATEATIPROC)context->StencilOpSeparate;
    if (context->TexBuffer == NULL && context->TexBufferARB != NULL) context->TexBuffer = (PFNGLTEXBUFFERPROC)context->TexBufferARB;
    if (context->TexBuffer == NULL && context->TexBufferEXT != NULL) context->TexBuffer = (PFNGLTEXBUFFERPROC)context->TexBufferEXT;
    if (context->TexBufferARB == NULL && context->TexBuffer != NULL) context->TexBufferARB = (PFNGLTEXBUFFERARBPROC)context->TexBuffer;
    if (context->TexBufferARB == NULL && context->TexBufferEXT != NULL) context->TexBufferARB = (PFNGLTEXBUFFERARBPROC)context->TexBufferEXT;
    if (context->TexBufferEXT == NULL && context->TexBuffer != NULL) context->TexBufferEXT = (PFNGLTEXBUFFEREXTPROC)context->TexBuffer;
    if (context->TexBufferEXT == NULL && context->TexBufferARB != NULL) context->TexBufferEXT = (PFNGLTEXBUFFEREXTPROC)context->TexBufferARB;
    if (context->TexImage3D == NULL && context->TexImage3DEXT != NULL) context->TexImage3D = (PFNGLTEXIMAGE3DPROC)context->TexImage3DEXT;
    if (context->TexImage3DEXT == NULL && context->TexImage3D != NULL) context->TexImage3DEXT = (PFNGLTEXIMAGE3DEXTPROC)context->TexImage3D;
    if (context->TexParameterIiv == NULL && context->TexParameterIivEXT != NULL) context->TexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)context->TexParameterIivEXT;
    if (context->TexParameterIivEXT == NULL && context->TexParameterIiv != NULL) context->TexParameterIivEXT = (PFNGLTEXPARAMETERIIVEXTPROC)context->TexParameterIiv;
    if (context->TexParameterIuiv == NULL && context->TexParameterIuivEXT != NULL) context->TexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)context->TexParameterIuivEXT;
    if (context->TexParameterIuivEXT == NULL && context->TexParameterIuiv != NULL) context->TexParameterIuivEXT = (PFNGLTEXPARAMETERIUIVEXTPROC)context->TexParameterIuiv;
    if (context->TexSubImage1D == NULL && context->TexSubImage1DEXT != NULL) context->TexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC)context->TexSubImage1DEXT;
    if (context->TexSubImage1DEXT == NULL && context->TexSubImage1D != NULL) context->TexSubImage1DEXT = (PFNGLTEXSUBIMAGE1DEXTPROC)context->TexSubImage1D;
    if (context->TexSubImage2D == NULL && context->TexSubImage2DEXT != NULL) context->TexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)context->TexSubImage2DEXT;
    if (context->TexSubImage2DEXT == NULL && context->TexSubImage2D != NULL) context->TexSubImage2DEXT = (PFNGLTEXSUBIMAGE2DEXTPROC)context->TexSubImage2D;
    if (context->TexSubImage3D == NULL && context->TexSubImage3DEXT != NULL) context->TexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)context->TexSubImage3DEXT;
    if (context->TexSubImage3DEXT == NULL && context->TexSubImage3D != NULL) context->TexSubImage3DEXT = (PFNGLTEXSUBIMAGE3DEXTPROC)context->TexSubImage3D;
    if (context->TransformFeedbackVaryings == NULL && context->TransformFeedbackVaryingsEXT != NULL) context->TransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)context->TransformFeedbackVaryingsEXT;
    if (context->TransformFeedbackVaryingsEXT == NULL && context->TransformFeedbackVaryings != NULL) context->TransformFeedbackVaryingsEXT = (PFNGLTRANSFORMFEEDBACKVARYINGSEXTPROC)context->TransformFeedbackVaryings;
    if (context->Uniform1f == NULL && context->Uniform1fARB != NULL) context->Uniform1f = (PFNGLUNIFORM1FPROC)context->Uniform1fARB;
    if (context->Uniform1fARB == NULL && context->Uniform1f != NULL) context->Uniform1fARB = (PFNGLUNIFORM1FARBPROC)context->Uniform1f;
    if (context->Uniform1fv == NULL && context->Uniform1fvARB != NULL) context->Uniform1fv = (PFNGLUNIFORM1FVPROC)context->Uniform1fvARB;
    if (context->Uniform1fvARB == NULL && context->Uniform1fv != NULL) context->Uniform1fvARB = (PFNGLUNIFORM1FVARBPROC)context->Uniform1fv;
    if (context->Uniform1i == NULL && context->Uniform1iARB != NULL) context->Uniform1i = (PFNGLUNIFORM1IPROC)context->Uniform1iARB;
    if (context->Uniform1iARB == NULL && context->Uniform1i != NULL) context->Uniform1iARB = (PFNGLUNIFORM1IARBPROC)context->Uniform1i;
    if (context->Uniform1iv == NULL && context->Uniform1ivARB != NULL) context->Uniform1iv = (PFNGLUNIFORM1IVPROC)context->Uniform1ivARB;
    if (context->Uniform1ivARB == NULL && context->Uniform1iv != NULL) context->Uniform1ivARB = (PFNGLUNIFORM1IVARBPROC)context->Uniform1iv;
    if (context->Uniform1ui == NULL && context->Uniform1uiEXT != NULL) context->Uniform1ui = (PFNGLUNIFORM1UIPROC)context->Uniform1uiEXT;
    if (context->Uniform1uiEXT == NULL && context->Uniform1ui != NULL) context->Uniform1uiEXT = (PFNGLUNIFORM1UIEXTPROC)context->Uniform1ui;
    if (context->Uniform1uiv == NULL && context->Uniform1uivEXT != NULL) context->Uniform1uiv = (PFNGLUNIFORM1UIVPROC)context->Uniform1uivEXT;
    if (context->Uniform1uivEXT == NULL && context->Uniform1uiv != NULL) context->Uniform1uivEXT = (PFNGLUNIFORM1UIVEXTPROC)context->Uniform1uiv;
    if (context->Uniform2f == NULL && context->Uniform2fARB != NULL) context->Uniform2f = (PFNGLUNIFORM2FPROC)context->Uniform2fARB;
    if (context->Uniform2fARB == NULL && context->Uniform2f != NULL) context->Uniform2fARB = (PFNGLUNIFORM2FARBPROC)context->Uniform2f;
    if (context->Uniform2fv == NULL && context->Uniform2fvARB != NULL) context->Uniform2fv = (PFNGLUNIFORM2FVPROC)context->Uniform2fvARB;
    if (context->Uniform2fvARB == NULL && context->Uniform2fv != NULL) context->Uniform2fvARB = (PFNGLUNIFORM2FVARBPROC)context->Uniform2fv;
    if (context->Uniform2i == NULL && context->Uniform2iARB != NULL) context->Uniform2i = (PFNGLUNIFORM2IPROC)context->Uniform2iARB;
    if (context->Uniform2iARB == NULL && context->Uniform2i != NULL) context->Uniform2iARB = (PFNGLUNIFORM2IARBPROC)context->Uniform2i;
    if (context->Uniform2iv == NULL && context->Uniform2ivARB != NULL) context->Uniform2iv = (PFNGLUNIFORM2IVPROC)context->Uniform2ivARB;
    if (context->Uniform2ivARB == NULL && context->Uniform2iv != NULL) context->Uniform2ivARB = (PFNGLUNIFORM2IVARBPROC)context->Uniform2iv;
    if (context->Uniform2ui == NULL && context->Uniform2uiEXT != NULL) context->Uniform2ui = (PFNGLUNIFORM2UIPROC)context->Uniform2uiEXT;
    if (context->Uniform2uiEXT == NULL && context->Uniform2ui != NULL) context->Uniform2uiEXT = (PFNGLUNIFORM2UIEXTPROC)context->Uniform2ui;
    if (context->Uniform2uiv == NULL && context->Uniform2uivEXT != NULL) context->Uniform2uiv = (PFNGLUNIFORM2UIVPROC)context->Uniform2uivEXT;
    if (context->Uniform2uivEXT == NULL && context->Uniform2uiv != NULL) context->Uniform2uivEXT = (PFNGLUNIFORM2UIVEXTPROC)context->Uniform2uiv;
    if (context->Uniform3f == NULL && context->Uniform3fARB != NULL) context->Uniform3f = (PFNGLUNIFORM3FPROC)context->Uniform3fARB;
    if (context->Uniform3fARB == NULL && context->Uniform3f != NULL) context->Uniform3fARB = (PFNGLUNIFORM3FARBPROC)context->Uniform3f;
    if (context->Uniform3fv == NULL && context->Uniform3fvARB != NULL) context->Uniform3fv = (PFNGLUNIFORM3FVPROC)context->Uniform3fvARB;
    if (context->Uniform3fvARB == NULL && context->Uniform3fv != NULL) context->Uniform3fvARB = (PFNGLUNIFORM3FVARBPROC)context->Uniform3fv;
    if (context->Uniform3i == NULL && context->Uniform3iARB != NULL) context->Uniform3i = (PFNGLUNIFORM3IPROC)context->Uniform3iARB;
    if (context->Uniform3iARB == NULL && context->Uniform3i != NULL) context->Uniform3iARB = (PFNGLUNIFORM3IARBPROC)context->Uniform3i;
    if (context->Uniform3iv == NULL && context->Uniform3ivARB != NULL) context->Uniform3iv = (PFNGLUNIFORM3IVPROC)context->Uniform3ivARB;
    if (context->Uniform3ivARB == NULL && context->Uniform3iv != NULL) context->Uniform3ivARB = (PFNGLUNIFORM3IVARBPROC)context->Uniform3iv;
    if (context->Uniform3ui == NULL && context->Uniform3uiEXT != NULL) context->Uniform3ui = (PFNGLUNIFORM3UIPROC)context->Uniform3uiEXT;
    if (context->Uniform3uiEXT == NULL && context->Uniform3ui != NULL) context->Uniform3uiEXT = (PFNGLUNIFORM3UIEXTPROC)context->Uniform3ui;
    if (context->Uniform3uiv == NULL && context->Uniform3uivEXT != NULL) context->Uniform3uiv = (PFNGLUNIFORM3UIVPROC)context->Uniform3uivEXT;
    if (context->Uniform3uivEXT == NULL && context->Uniform3uiv != NULL) context->Uniform3uivEXT = (PFNGLUNIFORM3UIVEXTPROC)context->Uniform3uiv;
    if (context->Uniform4f == NULL && context->Uniform4fARB != NULL) context->Uniform4f = (PFNGLUNIFORM4FPROC)context->Uniform4fARB;
    if (context->Uniform4fARB == NULL && context->Uniform4f != NULL) context->Uniform4fARB = (PFNGLUNIFORM4FARBPROC)context->Uniform4f;
    if (context->Uniform4fv == NULL && context->Uniform4fvARB != NULL) context->Uniform4fv = (PFNGLUNIFORM4FVPROC)context->Uniform4fvARB;
    if (context->Uniform4fvARB == NULL && context->Uniform4fv != NULL) context->Uniform4fvARB = (PFNGLUNIFORM4FVARBPROC)context->Uniform4fv;
    if (context->Uniform4i == NULL && context->Uniform4iARB != NULL) context->Uniform4i = (PFNGLUNIFORM4IPROC)context->Uniform4iARB;
    if (context->Uniform4iARB == NULL && context->Uniform4i != NULL) context->Uniform4iARB = (PFNGLUNIFORM4IARBPROC)context->Uniform4i;
    if (context->Uniform4iv == NULL && context->Uniform4ivARB != NULL) context->Uniform4iv = (PFNGLUNIFORM4IVPROC)context->Uniform4ivARB;
    if (context->Uniform4ivARB == NULL && context->Uniform4iv != NULL) context->Uniform4ivARB = (PFNGLUNIFORM4IVARBPROC)context->Uniform4iv;
    if (context->Uniform4ui == NULL && context->Uniform4uiEXT != NULL) context->Uniform4ui = (PFNGLUNIFORM4UIPROC)context->Uniform4uiEXT;
    if (context->Uniform4uiEXT == NULL && context->Uniform4ui != NULL) context->Uniform4uiEXT = (PFNGLUNIFORM4UIEXTPROC)context->Uniform4ui;
    if (context->Uniform4uiv == NULL && context->Uniform4uivEXT != NULL) context->Uniform4uiv = (PFNGLUNIFORM4UIVPROC)context->Uniform4uivEXT;
    if (context->Uniform4uivEXT == NULL && context->Uniform4uiv != NULL) context->Uniform4uivEXT = (PFNGLUNIFORM4UIVEXTPROC)context->Uniform4uiv;
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
    if (context->VertexAttribI1i == NULL && context->VertexAttribI1iEXT != NULL) context->VertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)context->VertexAttribI1iEXT;
    if (context->VertexAttribI1iEXT == NULL && context->VertexAttribI1i != NULL) context->VertexAttribI1iEXT = (PFNGLVERTEXATTRIBI1IEXTPROC)context->VertexAttribI1i;
    if (context->VertexAttribI1iv == NULL && context->VertexAttribI1ivEXT != NULL) context->VertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)context->VertexAttribI1ivEXT;
    if (context->VertexAttribI1ivEXT == NULL && context->VertexAttribI1iv != NULL) context->VertexAttribI1ivEXT = (PFNGLVERTEXATTRIBI1IVEXTPROC)context->VertexAttribI1iv;
    if (context->VertexAttribI1ui == NULL && context->VertexAttribI1uiEXT != NULL) context->VertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)context->VertexAttribI1uiEXT;
    if (context->VertexAttribI1uiEXT == NULL && context->VertexAttribI1ui != NULL) context->VertexAttribI1uiEXT = (PFNGLVERTEXATTRIBI1UIEXTPROC)context->VertexAttribI1ui;
    if (context->VertexAttribI1uiv == NULL && context->VertexAttribI1uivEXT != NULL) context->VertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC)context->VertexAttribI1uivEXT;
    if (context->VertexAttribI1uivEXT == NULL && context->VertexAttribI1uiv != NULL) context->VertexAttribI1uivEXT = (PFNGLVERTEXATTRIBI1UIVEXTPROC)context->VertexAttribI1uiv;
    if (context->VertexAttribI2i == NULL && context->VertexAttribI2iEXT != NULL) context->VertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)context->VertexAttribI2iEXT;
    if (context->VertexAttribI2iEXT == NULL && context->VertexAttribI2i != NULL) context->VertexAttribI2iEXT = (PFNGLVERTEXATTRIBI2IEXTPROC)context->VertexAttribI2i;
    if (context->VertexAttribI2iv == NULL && context->VertexAttribI2ivEXT != NULL) context->VertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)context->VertexAttribI2ivEXT;
    if (context->VertexAttribI2ivEXT == NULL && context->VertexAttribI2iv != NULL) context->VertexAttribI2ivEXT = (PFNGLVERTEXATTRIBI2IVEXTPROC)context->VertexAttribI2iv;
    if (context->VertexAttribI2ui == NULL && context->VertexAttribI2uiEXT != NULL) context->VertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)context->VertexAttribI2uiEXT;
    if (context->VertexAttribI2uiEXT == NULL && context->VertexAttribI2ui != NULL) context->VertexAttribI2uiEXT = (PFNGLVERTEXATTRIBI2UIEXTPROC)context->VertexAttribI2ui;
    if (context->VertexAttribI2uiv == NULL && context->VertexAttribI2uivEXT != NULL) context->VertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC)context->VertexAttribI2uivEXT;
    if (context->VertexAttribI2uivEXT == NULL && context->VertexAttribI2uiv != NULL) context->VertexAttribI2uivEXT = (PFNGLVERTEXATTRIBI2UIVEXTPROC)context->VertexAttribI2uiv;
    if (context->VertexAttribI3i == NULL && context->VertexAttribI3iEXT != NULL) context->VertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)context->VertexAttribI3iEXT;
    if (context->VertexAttribI3iEXT == NULL && context->VertexAttribI3i != NULL) context->VertexAttribI3iEXT = (PFNGLVERTEXATTRIBI3IEXTPROC)context->VertexAttribI3i;
    if (context->VertexAttribI3iv == NULL && context->VertexAttribI3ivEXT != NULL) context->VertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)context->VertexAttribI3ivEXT;
    if (context->VertexAttribI3ivEXT == NULL && context->VertexAttribI3iv != NULL) context->VertexAttribI3ivEXT = (PFNGLVERTEXATTRIBI3IVEXTPROC)context->VertexAttribI3iv;
    if (context->VertexAttribI3ui == NULL && context->VertexAttribI3uiEXT != NULL) context->VertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)context->VertexAttribI3uiEXT;
    if (context->VertexAttribI3uiEXT == NULL && context->VertexAttribI3ui != NULL) context->VertexAttribI3uiEXT = (PFNGLVERTEXATTRIBI3UIEXTPROC)context->VertexAttribI3ui;
    if (context->VertexAttribI3uiv == NULL && context->VertexAttribI3uivEXT != NULL) context->VertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC)context->VertexAttribI3uivEXT;
    if (context->VertexAttribI3uivEXT == NULL && context->VertexAttribI3uiv != NULL) context->VertexAttribI3uivEXT = (PFNGLVERTEXATTRIBI3UIVEXTPROC)context->VertexAttribI3uiv;
    if (context->VertexAttribI4bv == NULL && context->VertexAttribI4bvEXT != NULL) context->VertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)context->VertexAttribI4bvEXT;
    if (context->VertexAttribI4bvEXT == NULL && context->VertexAttribI4bv != NULL) context->VertexAttribI4bvEXT = (PFNGLVERTEXATTRIBI4BVEXTPROC)context->VertexAttribI4bv;
    if (context->VertexAttribI4i == NULL && context->VertexAttribI4iEXT != NULL) context->VertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)context->VertexAttribI4iEXT;
    if (context->VertexAttribI4iEXT == NULL && context->VertexAttribI4i != NULL) context->VertexAttribI4iEXT = (PFNGLVERTEXATTRIBI4IEXTPROC)context->VertexAttribI4i;
    if (context->VertexAttribI4iv == NULL && context->VertexAttribI4ivEXT != NULL) context->VertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)context->VertexAttribI4ivEXT;
    if (context->VertexAttribI4ivEXT == NULL && context->VertexAttribI4iv != NULL) context->VertexAttribI4ivEXT = (PFNGLVERTEXATTRIBI4IVEXTPROC)context->VertexAttribI4iv;
    if (context->VertexAttribI4sv == NULL && context->VertexAttribI4svEXT != NULL) context->VertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)context->VertexAttribI4svEXT;
    if (context->VertexAttribI4svEXT == NULL && context->VertexAttribI4sv != NULL) context->VertexAttribI4svEXT = (PFNGLVERTEXATTRIBI4SVEXTPROC)context->VertexAttribI4sv;
    if (context->VertexAttribI4ubv == NULL && context->VertexAttribI4ubvEXT != NULL) context->VertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)context->VertexAttribI4ubvEXT;
    if (context->VertexAttribI4ubvEXT == NULL && context->VertexAttribI4ubv != NULL) context->VertexAttribI4ubvEXT = (PFNGLVERTEXATTRIBI4UBVEXTPROC)context->VertexAttribI4ubv;
    if (context->VertexAttribI4ui == NULL && context->VertexAttribI4uiEXT != NULL) context->VertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)context->VertexAttribI4uiEXT;
    if (context->VertexAttribI4uiEXT == NULL && context->VertexAttribI4ui != NULL) context->VertexAttribI4uiEXT = (PFNGLVERTEXATTRIBI4UIEXTPROC)context->VertexAttribI4ui;
    if (context->VertexAttribI4uiv == NULL && context->VertexAttribI4uivEXT != NULL) context->VertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)context->VertexAttribI4uivEXT;
    if (context->VertexAttribI4uivEXT == NULL && context->VertexAttribI4uiv != NULL) context->VertexAttribI4uivEXT = (PFNGLVERTEXATTRIBI4UIVEXTPROC)context->VertexAttribI4uiv;
    if (context->VertexAttribI4usv == NULL && context->VertexAttribI4usvEXT != NULL) context->VertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)context->VertexAttribI4usvEXT;
    if (context->VertexAttribI4usvEXT == NULL && context->VertexAttribI4usv != NULL) context->VertexAttribI4usvEXT = (PFNGLVERTEXATTRIBI4USVEXTPROC)context->VertexAttribI4usv;
    if (context->VertexAttribIPointer == NULL && context->VertexAttribIPointerEXT != NULL) context->VertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)context->VertexAttribIPointerEXT;
    if (context->VertexAttribIPointerEXT == NULL && context->VertexAttribIPointer != NULL) context->VertexAttribIPointerEXT = (PFNGLVERTEXATTRIBIPOINTEREXTPROC)context->VertexAttribIPointer;
    if (context->VertexAttribPointer == NULL && context->VertexAttribPointerARB != NULL) context->VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)context->VertexAttribPointerARB;
    if (context->VertexAttribPointerARB == NULL && context->VertexAttribPointer != NULL) context->VertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)context->VertexAttribPointer;
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

    context->APPLE_flush_buffer_range = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_APPLE_flush_buffer_range");
    context->APPLE_vertex_array_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_APPLE_vertex_array_object");
    context->ARB_color_buffer_float = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_color_buffer_float");
    context->ARB_copy_buffer = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_copy_buffer");
    context->ARB_draw_buffers = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_draw_buffers");
    context->ARB_draw_elements_base_vertex = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_draw_elements_base_vertex");
    context->ARB_draw_instanced = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_draw_instanced");
    context->ARB_framebuffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_framebuffer_object");
    context->ARB_geometry_shader4 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_geometry_shader4");
    context->ARB_imaging = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_imaging");
    context->ARB_map_buffer_range = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_map_buffer_range");
    context->ARB_multisample = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_multisample");
    context->ARB_multitexture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_multitexture");
    context->ARB_occlusion_query = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_occlusion_query");
    context->ARB_point_parameters = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_point_parameters");
    context->ARB_provoking_vertex = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_provoking_vertex");
    context->ARB_shader_objects = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_shader_objects");
    context->ARB_sync = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_sync");
    context->ARB_texture_buffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_texture_buffer_object");
    context->ARB_texture_compression = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_texture_compression");
    context->ARB_texture_multisample = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_texture_multisample");
    context->ARB_uniform_buffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_uniform_buffer_object");
    context->ARB_vertex_array_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_vertex_array_object");
    context->ARB_vertex_buffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_vertex_buffer_object");
    context->ARB_vertex_program = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_vertex_program");
    context->ARB_vertex_shader = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ARB_vertex_shader");
    context->ATI_draw_buffers = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ATI_draw_buffers");
    context->ATI_separate_stencil = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_ATI_separate_stencil");
    context->EXT_blend_color = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_blend_color");
    context->EXT_blend_equation_separate = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_blend_equation_separate");
    context->EXT_blend_func_separate = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_blend_func_separate");
    context->EXT_blend_minmax = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_blend_minmax");
    context->EXT_copy_texture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_copy_texture");
    context->EXT_direct_state_access = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_direct_state_access");
    context->EXT_draw_buffers2 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_draw_buffers2");
    context->EXT_draw_instanced = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_draw_instanced");
    context->EXT_draw_range_elements = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_draw_range_elements");
    context->EXT_framebuffer_blit = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_framebuffer_blit");
    context->EXT_framebuffer_multisample = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_framebuffer_multisample");
    context->EXT_framebuffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_framebuffer_object");
    context->EXT_gpu_shader4 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_gpu_shader4");
    context->EXT_multi_draw_arrays = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_multi_draw_arrays");
    context->EXT_point_parameters = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_point_parameters");
    context->EXT_provoking_vertex = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_provoking_vertex");
    context->EXT_subtexture = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_subtexture");
    context->EXT_texture3D = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture3D");
    context->EXT_texture_array = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_array");
    context->EXT_texture_buffer_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_buffer_object");
    context->EXT_texture_integer = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_integer");
    context->EXT_texture_object = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_object");
    context->EXT_transform_feedback = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_transform_feedback");
    context->EXT_vertex_array = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_vertex_array");
    context->INGR_blend_func_separate = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_INGR_blend_func_separate");
    context->NVX_conditional_render = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NVX_conditional_render");
    context->NV_conditional_render = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_conditional_render");
    context->NV_explicit_multisample = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_explicit_multisample");
    context->NV_geometry_program4 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_geometry_program4");
    context->NV_point_sprite = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_point_sprite");
    context->NV_transform_feedback = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_transform_feedback");
    context->NV_vertex_program = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_vertex_program");
    context->NV_vertex_program4 = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_NV_vertex_program4");
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
    context->VERSION_2_1 = (major == 2 && minor >= 1) || major > 2;
    context->VERSION_3_0 = (major == 3 && minor >= 0) || major > 3;
    context->VERSION_3_1 = (major == 3 && minor >= 1) || major > 3;
    context->VERSION_3_2 = (major == 3 && minor >= 2) || major > 3;

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
    glad_gl_load_GL_VERSION_2_1(context, load, userptr);
    glad_gl_load_GL_VERSION_3_0(context, load, userptr);
    glad_gl_load_GL_VERSION_3_1(context, load, userptr);
    glad_gl_load_GL_VERSION_3_2(context, load, userptr);

    if (!glad_gl_find_extensions_gl(context, version)) return 0;
    glad_gl_load_GL_APPLE_flush_buffer_range(context, load, userptr);
    glad_gl_load_GL_APPLE_vertex_array_object(context, load, userptr);
    glad_gl_load_GL_ARB_color_buffer_float(context, load, userptr);
    glad_gl_load_GL_ARB_copy_buffer(context, load, userptr);
    glad_gl_load_GL_ARB_draw_buffers(context, load, userptr);
    glad_gl_load_GL_ARB_draw_elements_base_vertex(context, load, userptr);
    glad_gl_load_GL_ARB_draw_instanced(context, load, userptr);
    glad_gl_load_GL_ARB_framebuffer_object(context, load, userptr);
    glad_gl_load_GL_ARB_geometry_shader4(context, load, userptr);
    glad_gl_load_GL_ARB_imaging(context, load, userptr);
    glad_gl_load_GL_ARB_map_buffer_range(context, load, userptr);
    glad_gl_load_GL_ARB_multisample(context, load, userptr);
    glad_gl_load_GL_ARB_multitexture(context, load, userptr);
    glad_gl_load_GL_ARB_occlusion_query(context, load, userptr);
    glad_gl_load_GL_ARB_point_parameters(context, load, userptr);
    glad_gl_load_GL_ARB_provoking_vertex(context, load, userptr);
    glad_gl_load_GL_ARB_shader_objects(context, load, userptr);
    glad_gl_load_GL_ARB_sync(context, load, userptr);
    glad_gl_load_GL_ARB_texture_buffer_object(context, load, userptr);
    glad_gl_load_GL_ARB_texture_compression(context, load, userptr);
    glad_gl_load_GL_ARB_texture_multisample(context, load, userptr);
    glad_gl_load_GL_ARB_uniform_buffer_object(context, load, userptr);
    glad_gl_load_GL_ARB_vertex_array_object(context, load, userptr);
    glad_gl_load_GL_ARB_vertex_buffer_object(context, load, userptr);
    glad_gl_load_GL_ARB_vertex_program(context, load, userptr);
    glad_gl_load_GL_ARB_vertex_shader(context, load, userptr);
    glad_gl_load_GL_ATI_draw_buffers(context, load, userptr);
    glad_gl_load_GL_ATI_separate_stencil(context, load, userptr);
    glad_gl_load_GL_EXT_blend_color(context, load, userptr);
    glad_gl_load_GL_EXT_blend_equation_separate(context, load, userptr);
    glad_gl_load_GL_EXT_blend_func_separate(context, load, userptr);
    glad_gl_load_GL_EXT_blend_minmax(context, load, userptr);
    glad_gl_load_GL_EXT_copy_texture(context, load, userptr);
    glad_gl_load_GL_EXT_direct_state_access(context, load, userptr);
    glad_gl_load_GL_EXT_draw_buffers2(context, load, userptr);
    glad_gl_load_GL_EXT_draw_instanced(context, load, userptr);
    glad_gl_load_GL_EXT_draw_range_elements(context, load, userptr);
    glad_gl_load_GL_EXT_framebuffer_blit(context, load, userptr);
    glad_gl_load_GL_EXT_framebuffer_multisample(context, load, userptr);
    glad_gl_load_GL_EXT_framebuffer_object(context, load, userptr);
    glad_gl_load_GL_EXT_gpu_shader4(context, load, userptr);
    glad_gl_load_GL_EXT_multi_draw_arrays(context, load, userptr);
    glad_gl_load_GL_EXT_point_parameters(context, load, userptr);
    glad_gl_load_GL_EXT_provoking_vertex(context, load, userptr);
    glad_gl_load_GL_EXT_subtexture(context, load, userptr);
    glad_gl_load_GL_EXT_texture3D(context, load, userptr);
    glad_gl_load_GL_EXT_texture_array(context, load, userptr);
    glad_gl_load_GL_EXT_texture_buffer_object(context, load, userptr);
    glad_gl_load_GL_EXT_texture_integer(context, load, userptr);
    glad_gl_load_GL_EXT_texture_object(context, load, userptr);
    glad_gl_load_GL_EXT_transform_feedback(context, load, userptr);
    glad_gl_load_GL_EXT_vertex_array(context, load, userptr);
    glad_gl_load_GL_INGR_blend_func_separate(context, load, userptr);
    glad_gl_load_GL_NVX_conditional_render(context, load, userptr);
    glad_gl_load_GL_NV_conditional_render(context, load, userptr);
    glad_gl_load_GL_NV_explicit_multisample(context, load, userptr);
    glad_gl_load_GL_NV_geometry_program4(context, load, userptr);
    glad_gl_load_GL_NV_point_sprite(context, load, userptr);
    glad_gl_load_GL_NV_transform_feedback(context, load, userptr);
    glad_gl_load_GL_NV_vertex_program(context, load, userptr);
    glad_gl_load_GL_NV_vertex_program4(context, load, userptr);
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
