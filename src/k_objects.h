/* object-specific code */
#ifndef k_objects_H
#define k_objects_H

/* Hyudoro */
void Obj_HyudoroDeploy(mobj_t *master);
void Obj_HyudoroThink(mobj_t *actor);
void Obj_HyudoroCenterThink(mobj_t *actor);
void Obj_HyudoroCollide(mobj_t *special, mobj_t *toucher);

/* Shrink */
void Obj_PohbeeThinker(mobj_t *pohbee);
void Obj_PohbeeRemoved(mobj_t *pohbee);
void Obj_ShrinkGunRemoved(mobj_t *gun);
boolean Obj_ShrinkLaserCollide(mobj_t *gun, mobj_t *victim);
void Obj_CreateShrinkPohbees(player_t *owner);

/* SPB */
void Obj_SPBThink(mobj_t *spb);

#endif/*k_objects_H*/
