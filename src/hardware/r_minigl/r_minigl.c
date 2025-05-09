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
/// \file
/// \brief MiniGL API for Doom Legacy


// tell r_opengl.cpp to compile for MiniGL Drivers
#define MINI_GL_COMPATIBILITY

// tell r_opengl.cpp to compile for ATI Rage Pro OpenGL driver
//#define ATI_RAGE_PRO_COMPATIBILITY

#define DRIVER_STRING "HWRAPI Init(): Ring Racers MiniGL renderer"

// Include this at end
#include "../r_opengl/r_opengl.c"
#include "../r_opengl/ogl_win.c"

// That's all ;-)
// Just, be sure to do the right changes in r_opengl.cpp
