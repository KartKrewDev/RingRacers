// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez.
// Copyright (C) 2025 by Kart Krew
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

static UINT8 interlacedfuckingthing[] = {6,1,7,2,8,3,9,2,1,1,1,2,1,3,1,2,10,1,11,2,4,3,5,2,1,1,1,2,1,3,1,2};

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

        if (amps->extravalue2) // Mode: going down, aim at the player and speed up / dampen stray movement
        {
            if (amps->extravalue1)
                amps->extravalue1--;

            amps->extravalue2++;

            speed += amps->extravalue2 * amps->scale/2;

            fakez = mo->z + (vert * amps->extravalue1 / AMP_ARCTIME);
            damper = 1;
        }
        else // Mode: going up, aim above the player
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

        if (dist < (120 * amps->scale) && amps->extravalue2 && !player->amppickup)
        {
            K_AwardPlayerAmps(player, 2);
            P_RemoveMobj(amps);
        }
    }
}

void Obj_AmpRingThink (mobj_t *amps)
{
    if (P_MobjWasRemoved(amps->target) || !amps->target->player)
    {
        P_RemoveMobj(amps);
    }
    else
    {
        mobj_t *mo = amps->target;
        player_t *player = mo->player;

        amps->flags &= ~(MF_NOCLIPTHING);
		P_MoveOrigin(amps, mo->x, mo->y, mo->z + mo->height/2);
		amps->flags |= MF_NOCLIPTHING;

        amps->color = mo->player->skincolor;
        amps->sprite = SPR_AMPB;
        amps->frame = interlacedfuckingthing[(leveltime/1)%32]-1;
        amps->colorized = true;

        if (amps->scale != 3*mo->scale/2)
            P_InstaScale(amps, 3*mo->scale/2);

        amps->angle = amps->target->angle + (ANG15/2 * leveltime);
        amps->renderflags |= RF_ADD|RF_PAPERSPRITE|RF_FULLBRIGHT;

        if (player->overshield)
            amps->renderflags &= ~RF_DONTDRAW;
        else
            amps->renderflags |= RF_DONTDRAW;

        if (player->overshield < 35 && player->overshield % 2)
            amps->renderflags |= RF_DONTDRAW;
    }
}

void Obj_AmpBodyThink (mobj_t *amps)
{
    if (P_MobjWasRemoved(amps->target) || !amps->target->player)
    {
        P_RemoveMobj(amps);
    }
    else
    {
        mobj_t *mo = amps->target;
        player_t *player = mo->player;

        amps->flags &= ~(MF_NOCLIPTHING);
		P_MoveOrigin(amps, mo->x, mo->y, mo->z + mo->height/2);
		amps->flags |= MF_NOCLIPTHING;

        amps->color = mo->player->skincolor;
        amps->frame = leveltime%14;
        amps->colorized = true;

        if (amps->scale != 5*mo->scale/4)
            P_InstaScale(amps, 5*mo->scale/4);

        amps->angle = amps->target->angle;
        amps->renderflags |= RF_ADD|RF_FULLBRIGHT;

        if (player->overdrive || player->overdriveready)
            amps->renderflags &= ~RF_DONTDRAW;
        else
            amps->renderflags |= RF_DONTDRAW;

        if ((player->overdrive < 35 && player->overdrive % 2) || (player->overdriveready && leveltime % 2))
            amps->renderflags |= RF_DONTDRAW;
    }
}

void Obj_AmpAuraThink (mobj_t *amps)
{
    if (P_MobjWasRemoved(amps->target) || !amps->target->player)
    {
        P_RemoveMobj(amps);
    }
    else
    {
        mobj_t *mo = amps->target;
        player_t *player = mo->player;

        amps->flags &= ~(MF_NOCLIPTHING);
		P_MoveOrigin(amps, mo->x, mo->y, mo->z + mo->height/2);
		amps->flags |= MF_NOCLIPTHING;

        amps->color = mo->player->skincolor;
        amps->frame = leveltime%2;
        amps->colorized = true;

        if (amps->scale != 5*mo->scale/4)
            P_InstaScale(amps, 5*mo->scale/4);

        amps->angle = amps->target->angle;
        amps->renderflags |= RF_ADD|RF_FULLBRIGHT;

        amps->renderflags &= ~RF_TRANSMASK;
        amps->renderflags |= (max(3, NUMTRANSMAPS - player->overdrive/TICRATE) << RF_TRANSSHIFT);

        if (!player->overdrive)
            amps->renderflags |= RF_DONTDRAW;
        else
            amps->renderflags &= ~RF_DONTDRAW;
    }
}

void Obj_AmpBurstThink (mobj_t *amps)
{
    amps->renderflags &= ~RF_TRANSMASK;
    amps->renderflags |= (NUMTRANSMAPS - amps->fuse) << RF_TRANSSHIFT;
}
