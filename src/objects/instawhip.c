#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"

void Obj_InstaWhipThink (mobj_t *whip)
{
    if (P_MobjWasRemoved(whip->target))
    {
        P_RemoveMobj(whip);
    }
    else
    {
        mobj_t *mo = whip->target;
        player_t *player = mo->player;

        // Follow player
        whip->flags &= ~(MF_NOCLIPTHING);
        P_SetScale(whip, whip->target->scale);
		P_MoveOrigin(whip, mo->x, mo->y, mo->z + mo->height/2);
		whip->flags |= MF_NOCLIPTHING;

        // Twirl
        whip->angle = whip->target->angle + (ANG30 * 2 * whip->fuse);
        whip->target->player->drawangle = whip->angle;
        if (player->follower)
            player->follower->angle = whip->angle;
        player->pflags |= PF_GAINAX;
        player->glanceDir = -2;

        // Visuals
        whip->renderflags |= RF_NOSPLATBILLBOARD|RF_FULLBRIGHT;

        if (whip->renderflags & RF_DONTDRAW)
            whip->renderflags &= ~RF_DONTDRAW;
        else
            whip->renderflags |= RF_DONTDRAW;

        if (whip->extravalue2) // Whip has no hitbox but removing it is a pain in the ass
            whip->renderflags |= RF_DONTDRAW;
    }
}
