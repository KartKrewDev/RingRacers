// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __K_COLLIDE__
#define __K_COLLIDE__

#include "doomtype.h"
#include "p_mobj.h"

#ifdef __cplusplus
extern "C" {
#endif

angle_t K_GetCollideAngle(mobj_t *t1, mobj_t *t2);

boolean K_BananaBallhogCollide(mobj_t *t1, mobj_t *t2);
boolean K_EggItemCollide(mobj_t *t1, mobj_t *t2);

void K_DoMineSearch(mobj_t *actor, fixed_t size);
tic_t K_MineExplodeAttack(mobj_t *actor, fixed_t size, boolean spin);
boolean K_MineCollide(mobj_t *t1, mobj_t *t2);

boolean K_LandMineCollide(mobj_t *t1, mobj_t *t2);

boolean K_DropTargetCollide(mobj_t *t1, mobj_t *t2);

void K_LightningShieldAttack(mobj_t *actor, fixed_t size);

boolean K_BubbleShieldCanReflect(mobj_t *t1, mobj_t *t2);
boolean K_BubbleShieldReflect(mobj_t *t1, mobj_t *t2);
boolean K_BubbleShieldCollide(mobj_t *t1, mobj_t *t2);

boolean K_InstaWhipCollide(mobj_t *shield, mobj_t *victim);

boolean K_KitchenSinkCollide(mobj_t *t1, mobj_t *t2);

boolean K_FallingRockCollide(mobj_t *t1, mobj_t *t2);

boolean K_PvPTouchDamage(mobj_t *t1, mobj_t *t2);

void K_PuntHazard(mobj_t *t1, mobj_t *t2);
boolean K_PuntCollide(mobj_t *t1, mobj_t *t2);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
