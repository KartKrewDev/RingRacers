#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"

void Obj_BlockRingThink (mobj_t *ring)
{
    if (P_MobjWasRemoved(ring->target) || !ring->target->player || !ring->target->player->ebrakefor)
    {
        P_RemoveMobj(ring);
    }
    else
    {
        mobj_t *mo = ring->target;
        player_t *player = mo->player;

        // Follow player
        ring->flags &= ~(MF_NOCLIPTHING);
		P_MoveOrigin(ring, mo->x, mo->y, mo->z + mo->height/2);
		ring->flags |= MF_NOCLIPTHING;
        ring->color = mo->color;

        fixed_t baseScale = mo->scale / 2;
        baseScale += (mo->scale / 30) * player->spheres;
        P_SetScale(ring, baseScale);

        // Twirl
        ring->angle = ring->target->angle + (ANG15 * leveltime);
        // Visuals
        ring->renderflags |= RF_ADD|RF_PAPERSPRITE;

        if (leveltime%2)
            ring->renderflags &= ~RF_DONTDRAW;
        else
            ring->renderflags |= RF_DONTDRAW;

        if (player->spheres == 0)
            ring->renderflags |= RF_DONTDRAW;
    }
}

void Obj_BlockBodyThink (mobj_t *body)
{
    if (P_MobjWasRemoved(body->target) || !body->target->player || !body->target->player->ebrakefor)
    {
        P_RemoveMobj(body);
    }
    else
    {
        mobj_t *mo = body->target;
        player_t *player = mo->player;

        // Follow player
        body->flags &= ~(MF_NOCLIPTHING);

        fixed_t baseScale = mo->scale / 2;
        baseScale += (mo->scale / 30) * player->spheres;
        P_SetScale(body, baseScale);

		P_MoveOrigin(body, mo->x, mo->y, mo->z + mo->height/2);
		body->flags |= MF_NOCLIPTHING;
        body->color = mo->color;

        // Visuals
        body->renderflags |= RF_ADD;

        if (leveltime%2 == 0)
            body->renderflags &= ~RF_DONTDRAW;
        else
            body->renderflags |= RF_DONTDRAW;

        if (player->spheres == 0)
            body->renderflags |= RF_DONTDRAW;
    }
}