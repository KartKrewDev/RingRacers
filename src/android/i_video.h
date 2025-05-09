// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef _SRB2_ANDROID_VIDEO_
#define _SRB2_ANDROID_VIDEO_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

UINT8 *android_surface;

JNIEnv* jni_env;
jobject androidVideo;
jmethodID videoFrameCB;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
