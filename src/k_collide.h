// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef K_COLLIDE_H
#define K_COLLIDE_H

#include "doomtype.h"
#include "p_mobj.h"

#ifdef __cplusplus
extern "C" {
#endif

angle_t K_GetCollideAngle(mobj_t *t1, mobj_t *t2);

dboolean K_BananaBallhogCollide(mobj_t *t1, mobj_t *t2);
dboolean K_EggItemCollide(mobj_t *t1, mobj_t *t2);

void K_DoMineSearch(mobj_t *actor, fixed_t size);
tic_t K_MineExplodeAttack(mobj_t *actor, fixed_t size, dboolean spin);
dboolean K_MineCollide(mobj_t *t1, mobj_t *t2);

dboolean K_LandMineCollide(mobj_t *t1, mobj_t *t2);

dboolean K_DropTargetCollide(mobj_t *t1, mobj_t *t2);

void K_LightningShieldAttack(mobj_t *actor, fixed_t size);

dboolean K_BubbleShieldCanReflect(mobj_t *t1, mobj_t *t2);
dboolean K_BubbleShieldReflect(mobj_t *t1, mobj_t *t2);
dboolean K_BubbleShieldCollide(mobj_t *t1, mobj_t *t2);

dboolean K_InstaWhipCollide(mobj_t *shield, mobj_t *victim);

dboolean K_KitchenSinkCollide(mobj_t *t1, mobj_t *t2);

dboolean K_FallingRockCollide(mobj_t *t1, mobj_t *t2);

dboolean K_PvPTouchDamage(mobj_t *t1, mobj_t *t2);

void K_PuntHazard(mobj_t *t1, mobj_t *t2);
dboolean K_PuntCollide(mobj_t *t1, mobj_t *t2);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
