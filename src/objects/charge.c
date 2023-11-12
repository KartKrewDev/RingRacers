#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../k_kart.h"
#include "../k_powerup.h"

void Obj_ChargeAuraThink (mobj_t *aura)
{
    if (P_MobjWasRemoved(aura->target) || !aura->target->player || !aura->target->player->trickcharge)
    {
        P_RemoveMobj(aura);
    }
    else
    {
        mobj_t *mo = aura->target;
        player_t *player = mo->player;

        // Follow player
        aura->flags &= ~(MF_NOCLIPTHING);
		P_MoveOrigin(aura, mo->x, mo->y, mo->z + mo->height/2);
		aura->flags |= MF_NOCLIPTHING;
        aura->color = mo->color;

        fixed_t baseScale = 12*mo->scale/10;
        P_SetScale(aura, baseScale);

        // Twirl
        aura->angle = aura->angle - ANG1*(player->trickcharge/TICRATE + 4);
        // Visuals
        aura->renderflags |= RF_PAPERSPRITE|RF_ADD;

    }
}