// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez.
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  bail.c
/// \brief Charge VFX code.

#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../info.h"
#include "../k_kart.h"
#include "../p_local.h"

void Obj_BailChargeThink (mobj_t *aura)
{
    if (P_MobjWasRemoved(aura->target)
		|| aura->target->health == 0
		|| aura->target->destscale <= 1 // sealed star fall out
		|| !aura->target->player
        || !aura->target->player->bailcharge)
    {
        P_RemoveMobj(aura);
    }
    else
    {
        mobj_t *mo = aura->target;
        player_t *player = mo->player;

        if (P_PlayerInPain(player) && aura->state != &states[S_TECHCHARGE])
        {
            P_SetMobjState(aura, S_TECHCHARGE);
            player->bailcharge = 1;
        }

        // Follow player
        aura->flags &= ~(MF_NOCLIPTHING);
		P_MoveOrigin(aura, mo->x, mo->y, mo->z + mo->height/2);
		aura->flags |= MF_NOCLIPTHING;
        // aura->color = mo->color;

        // aura->renderflags &= ~RF_DONTDRAW;

        fixed_t baseScale = 12*mo->scale/10;

        P_SetScale(aura, baseScale);
    }
}
