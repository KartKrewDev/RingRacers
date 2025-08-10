// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef k_endcam_h
#define k_endcam_h

#include "typedef.h"

#include "doomtype.h"
#include "m_fixed.h"
#include "tables.h"

#ifdef __cplusplus
extern "C" {
#endif

struct endcam_t
{
	//
	// Configurable properties
	//

	vector3_t origin; // center point
	vector2_t startRadius; // X = horizontal, Y = vertical
	vector2_t endRadius;

	tic_t swirlDuration;
	fixed_t startAngle; // 180*FRACUNIT, NOT ANGLE_180
	fixed_t endAngle;

	// 1) Camera pans vertically to keep this object centered
	// 2) After swirling ends, pan horizontally too
	mobj_t *panMobj;
	tic_t panDuration; // dropoff after swirling ends
	fixed_t panSpeed; // 0-FRACUNIT

	/// ...

	// You should not set these yourself.
	// Use K_CommitEndCamera.
	boolean active;
	tic_t begin; // leveltime
};

extern endcam_t g_endcam;

// Sets endcam_t.active and endcam_t.begin.
//
//   VERY IMPORTANT:
//
// Set the OTHER fields in endcam_t BEFORE calling this
// function, so the camera can cut away cleanly.
void K_CommitEndCamera(void);

// Automatically set up a cool camera in one-shot.
void K_StartRoundWinCamera(mobj_t *origin, angle_t focusAngle, fixed_t finalRadius, tic_t panDuration, fixed_t panSpeed, tic_t swirlDuration);

// Stop the end camera
void K_StopRoundWinCamera(void);

/// ...

// Low-level functions
void K_MoveEndCamera(camera_t *thiscam);
void K_EndCameraGC(void);
boolean K_EndCameraIsFreezing(void);
void K_SaveEndCamera(savebuffer_t *save);
void K_LoadEndCamera(savebuffer_t *save);

#ifdef __cplusplus
} // extern "C"
#endif

#endif/*k_endcam_h*/
