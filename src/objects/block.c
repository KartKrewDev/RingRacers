// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez.
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  block.c
/// \brief Guard object code.

#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../k_kart.h"
#include "../k_powerup.h"

void Obj_BlockRingThink (mobj_t *ring)
{
    if (P_MobjWasRemoved(ring->target) || !ring->target->player)
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
        ring->color = mo->player->skincolor;

        fixed_t baseScale = mo->scale / 2;
        baseScale += (mo->scale / 30) * player->spheres;

        P_SetScale(ring, baseScale);

        // Twirl
        ring->angle = ring->target->angle + (ANG15/2 * leveltime);
        // Visuals
        ring->renderflags |= RF_ADD|RF_PAPERSPRITE;

        if (leveltime%2)
            ring->renderflags &= ~RF_DONTDRAW;
        else
            ring->renderflags |= RF_DONTDRAW;

        if (K_PowerUpRemaining(player, POWERUP_BARRIER) || !K_PlayerGuard(player))
            ring->renderflags |= RF_DONTDRAW;
    }
}

void Obj_BlockBodyThink (mobj_t *body)
{
    if (P_MobjWasRemoved(body->target) || !body->target->player)
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

        if (K_PowerUpRemaining(player, POWERUP_BARRIER) || !K_PlayerGuard(player))
            body->renderflags |= RF_DONTDRAW;
    }
}

void Obj_GuardBreakThink (mobj_t *fx)
{
    if (leveltime%2)
        fx->renderflags &= ~RF_DONTDRAW;
    else
        fx->renderflags |= RF_DONTDRAW;
}
