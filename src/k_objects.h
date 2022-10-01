/* object-specific code */
#ifndef k_objects_H
#define k_objects_H

/* Hyudoro */
void Obj_HyudoroDeploy(mobj_t *master);
void Obj_HyudoroThink(mobj_t *actor);
void Obj_HyudoroCenterThink(mobj_t *actor);
void Obj_HyudoroCollide(mobj_t *special, mobj_t *toucher);

/* Garden Top */
void Obj_GardenTopDeploy(mobj_t *rider);
mobj_t *Obj_GardenTopThrow(player_t *player);
mobj_t *Obj_GardenTopDestroy(player_t *player);
void Obj_GardenTopThink(mobj_t *top);
void Obj_GardenTopSparkThink(mobj_t *spark);
boolean Obj_GardenTopPlayerIsGrinding(player_t *player);

/* Shrink */
void Obj_PohbeeThinker(mobj_t *pohbee);
void Obj_PohbeeRemoved(mobj_t *pohbee);
void Obj_ShrinkGunRemoved(mobj_t *gun);
boolean Obj_ShrinkLaserCollide(mobj_t *gun, mobj_t *victim);
void Obj_CreateShrinkPohbees(player_t *owner);

/* Item Debris */
void Obj_SpawnItemDebrisEffects(mobj_t *collectible, mobj_t *collector);
void Obj_ItemDebrisThink(mobj_t *debris);
fixed_t Obj_ItemDebrisBounce(mobj_t *debris, fixed_t momz);

/* SPB */
void Obj_SPBThink(mobj_t *spb);
void Obj_SPBExplode(mobj_t *spb);
void Obj_SPBTouch(mobj_t *spb, mobj_t *toucher);

/* SPB Juicebox Rings */
void Obj_MantaRingThink(mobj_t *manta);
mobj_t *Obj_MantaRingCreate(mobj_t *spb, mobj_t *owner, mobj_t *chase);

/* Orbinaut */
void Obj_OrbinautThink(mobj_t *th);
boolean Obj_OrbinautJawzCollide(mobj_t *t1, mobj_t *t2);
void Obj_OrbinautThrown(mobj_t *th, fixed_t finalSpeed, SINT8 dir);
void Obj_OrbinautJawzMoveHeld(player_t *player);

/* Jawz */
void Obj_JawzThink(mobj_t *th);
void Obj_JawzThrown(mobj_t *th, fixed_t finalSpeed, SINT8 dir);

#endif/*k_objects_H*/
