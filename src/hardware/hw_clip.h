/*
 *  hw_clip.h
 *  SRB2CB
 *
 *  PrBoom's OpenGL clipping
 *
 *
 */

#ifndef __HARDWARE_HW_CLIP_H__
#define __HARDWARE_HW_CLIP_H__

// OpenGL BSP clipping
#include "../doomdef.h"
#include "../tables.h"
#include "../doomtype.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define HAVE_SPHEREFRUSTRUM // enable if you want gld_SphereInFrustum and related code

boolean gld_clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle);
void gld_clipper_SafeAddClipRange(angle_t startangle, angle_t endangle);
void gld_clipper_Clear(void);
angle_t gld_FrustumAngle(angle_t tiltangle);
#ifdef HAVE_SPHEREFRUSTRUM
void gld_FrustrumSetup(void);
boolean gld_SphereInFrustum(float x, float y, float z, float radius);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __HARDWARE_HW_CLIP_H__
