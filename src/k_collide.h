#ifndef __K_COLLIDE__
#define __K_COLLIDE__

#include "doomtype.h"
#include "p_mobj.h"

angle_t K_GetCollideAngle(mobj_t *t1, mobj_t *t2);

boolean K_BananaBallhogCollide(mobj_t *t1, mobj_t *t2);
boolean K_EggItemCollide(mobj_t *t1, mobj_t *t2);

void K_DoMineSearch(mobj_t *actor, fixed_t size);
tic_t K_MineExplodeAttack(mobj_t *actor, fixed_t size, boolean spin);
boolean K_MineCollide(mobj_t *t1, mobj_t *t2);

boolean K_LandMineCollide(mobj_t *t1, mobj_t *t2);

boolean K_DropTargetCollide(mobj_t *t1, mobj_t *t2);

void K_LightningShieldAttack(mobj_t *actor, fixed_t size);
boolean K_BubbleShieldCollide(mobj_t *t1, mobj_t *t2);

boolean K_KitchenSinkCollide(mobj_t *t1, mobj_t *t2);

boolean K_FallingRockCollide(mobj_t *t1, mobj_t *t2);
boolean K_SMKIceBlockCollide(mobj_t *t1, mobj_t *t2);

boolean K_PvPTouchDamage(mobj_t *t1, mobj_t *t2);

#endif
