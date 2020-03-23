#ifndef __K_COLLIDE__
#define __K_COLLIDE__

#include "doomtype.h"
#include "p_mobj.h"

boolean K_OrbinautJawzCollide(mobj_t *t1, mobj_t *t2);
boolean K_BananaBallhogCollide(mobj_t *t1, mobj_t *t2);
boolean K_EggItemCollide(mobj_t *t1, mobj_t *t2);
boolean K_MineCollide(mobj_t *t1, mobj_t *t2);
boolean K_MineExplosionCollide(mobj_t *t1, mobj_t *t2);
boolean K_KitchenSinkCollide(mobj_t *t1, mobj_t *t2);
boolean K_FallingRockCollide(mobj_t *t1, mobj_t *t2);
boolean K_SMKIceBlockCollide(mobj_t *t1, mobj_t *t2);

#endif
