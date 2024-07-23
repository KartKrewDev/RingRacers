// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by AJ "Tyron" Martinez.
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  amps.c
/// \brief Amps VFX code.

#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../k_kart.h"
#include "../k_powerup.h"
#include "../m_random.h"
#include "../r_main.h"
#include "../m_easing.h"

#define AMP_ARCTIME (8)

void Obj_AmpsThink (mobj_t *amps)
{
    if (P_MobjWasRemoved(amps->target)
		|| amps->target->health == 0
		|| amps->target->destscale <= 1 // sealed star fall out
		|| !amps->target->player)
    {
        P_RemoveMobj(amps);
    }
    else
    {
        mobj_t *mo = amps->target;
        player_t *player = mo->player;

        fixed_t dist, fakez;
        angle_t hang, vang;

        UINT8 damper = 3;

        dist = P_AproxDistance(P_AproxDistance(amps->x - mo->x, amps->y - mo->y), amps->z - mo->z);

        fixed_t vert = dist/3;
        fixed_t speed = 45*amps->scale;

        if (amps->extravalue2)
        {
            if (amps->extravalue1)
                amps->extravalue1--;

            amps->extravalue2++;

            speed += amps->extravalue1 * amps->scale/2;

            fakez = mo->z + (vert * amps->extravalue1 / AMP_ARCTIME);
            damper = 1;
        }
        else
        {
            amps->extravalue1++;
            if (amps->extravalue1 >= AMP_ARCTIME)
                amps->extravalue2 = 1;

            fakez = mo->z + vert;
        }

        if (mo->flags & MFE_VERTICALFLIP)
            fakez -= mo->height/2;
        else
            fakez += mo->height/2;

        hang = R_PointToAngle2(amps->x, amps->y, mo->x, mo->y);
        vang = R_PointToAngle2(amps->z, 0, fakez, dist);

        amps->momx -= amps->momx>>(damper), amps->momy -= amps->momy>>(damper), amps->momz -= amps->momz>>(damper);
        amps->momx += FixedMul(FINESINE(vang>>ANGLETOFINESHIFT), FixedMul(FINECOSINE(hang>>ANGLETOFINESHIFT), speed));
        amps->momy += FixedMul(FINESINE(vang>>ANGLETOFINESHIFT), FixedMul(FINESINE(hang>>ANGLETOFINESHIFT), speed));
        amps->momz += FixedMul(FINECOSINE(vang>>ANGLETOFINESHIFT), speed);

        if (dist < (120 * amps->scale) && amps->extravalue2 && !player->ampsounds)
        {   
            K_AwardPlayerAmps(player, 2);
            P_RemoveMobj(amps);
        }
    }
}