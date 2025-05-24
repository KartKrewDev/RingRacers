// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_patch.h
/// \brief Patch generation.

#ifndef __R_PATCH__
#define __R_PATCH__

#include "r_defs.h"
#include "r_picformats.h"
#include "doomdef.h"
#include "d_player.h"

#ifdef __cplusplus
extern "C" {
#endif

// Patch functions
patch_t *Patch_Create(softwarepatch_t *source, size_t srcsize, void *dest);
void Patch_Free(patch_t *patch);
boolean Patch_WasFreedThisFrame(void);
void Patch_ResetFreedThisFrame(void);

#define Patch_FreeTag(tagnum) Patch_FreeTags(tagnum, tagnum)
void Patch_FreeTags(INT32 lowtag, INT32 hightag);

void Patch_GenerateFlat(patch_t *patch, pictureflags_t flags);

#ifdef HWRENDER
void *Patch_AllocateHardwarePatch(patch_t *patch);
void *Patch_CreateGL(patch_t *patch);
#endif

#ifdef ROTSPRITE
void Patch_Rotate(patch_t *patch, INT32 angle, INT32 xpivot, INT32 ypivot, boolean flip);
patch_t *Patch_GetRotated(patch_t *patch, INT32 angle, boolean flip);
patch_t *Patch_GetRotatedSprite(
	spriteframe_t *sprite,
	size_t frame, size_t spriteangle,
	boolean flip, boolean adjustfeet,
	void *info, INT32 rotationangle);

INT32 R_GetRollAngle(angle_t rollangle);
angle_t R_GetPitchRollAngle(mobj_t *mobj, player_t *viewPlayer);
angle_t R_ModelRotationAngle(mobj_t *mobj, player_t *viewPlayer);
angle_t R_SpriteRotationAngle(mobj_t *mobj, player_t *viewPlayer);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __R_PATCH__
