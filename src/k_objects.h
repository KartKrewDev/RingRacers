/* object-specific code */
#ifndef k_objects_H
#define k_objects_H

/* Hyudoro */
void Obj_HyudoroDeploy(mobj_t *master);
void Obj_HyudoroThink(mobj_t *actor);
void Obj_HyudoroCenterThink(mobj_t *actor);
void Obj_HyudoroCollide(mobj_t *special, mobj_t *toucher);

#endif/*k_objects_H*/
