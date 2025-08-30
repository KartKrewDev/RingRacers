// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by AJ "Tyron" Martinez.
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  charge.c
/// \brief Charge VFX code.

#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../k_kart.h"
#include "../k_powerup.h"
#include "../m_random.h"

#define CHARGEAURA_BURSTTIME (9)
#define CHARGEAURA_SPARKRADIUS (40)

// xval1: destruction timer
// xval2: master (spawns other visuals)
// cvmem: spawn time (used to offset flash)
void Obj_ChargeAuraThink (mobj_t *aura)
{
    if (P_MobjWasRemoved(aura->target)
		|| aura->target->health == 0
		|| aura->target->destscale <= 1 // sealed star fall out
		|| !aura->target->player
		|| (aura->extravalue1 >= CHARGEAURA_BURSTTIME))
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

        aura->renderflags &= ~RF_DONTDRAW;

        fixed_t baseScale = 12*mo->scale/10;

        if (aura->extravalue1 || !player->trickcharge)
        {
            aura->extravalue1++;
            baseScale += (mo->scale / 3) * aura->extravalue1;
            aura->renderflags &= ~RF_TRANSMASK;
            aura->renderflags |= (aura->extravalue1)<<RF_TRANSSHIFT;
            if (aura->extravalue1 % 2)
                aura->renderflags |= RF_DONTDRAW;
        }

        P_SetScale(aura, baseScale);

        // Twirl
        aura->angle = aura->angle - ANG1*(player->trickcharge/TICRATE + 4);
        // Visuals
        aura->renderflags |= RF_PAPERSPRITE|RF_ADD;

        // fuck
        boolean forceinvisible = !!!((leveltime - aura->cvmem) % 4);
        if (aura->extravalue1 || !(player->driftcharge > K_GetKartDriftSparkValueForStage(player, 3)))
            forceinvisible = false;

        if (forceinvisible)
            aura->renderflags |= RF_DONTDRAW;

        if (aura->extravalue2)
        {
            if (player->driftcharge)
            {
				fixed_t rand_x;
				fixed_t rand_y;
				fixed_t rand_z;

				// note: determinate random argument eval order
				rand_z = P_RandomRange(PR_DECORATION, -1*CHARGEAURA_SPARKRADIUS, CHARGEAURA_SPARKRADIUS);
				rand_y = P_RandomRange(PR_DECORATION, -1*CHARGEAURA_SPARKRADIUS, CHARGEAURA_SPARKRADIUS);
				rand_x = P_RandomRange(PR_DECORATION, -1*CHARGEAURA_SPARKRADIUS, CHARGEAURA_SPARKRADIUS);
                mobj_t *spark = P_SpawnMobjFromMobj(aura,
                    FRACUNIT*rand_x,
                    FRACUNIT*rand_y,
                    FRACUNIT*rand_z,
                    MT_CHARGESPARK);
                spark->frame = P_RandomRange(PR_DECORATION, 1, 5);
                spark->renderflags |= RF_FULLBRIGHT|RF_ADD;
                P_SetTarget(&spark->target, aura);
                P_SetScale(spark, 15*aura->scale/10);
            }

            if (forceinvisible)
            {
                mobj_t *flicker = P_SpawnMobjFromMobj(aura, 0, 0, 0, MT_CHARGEFLICKER);
                P_SetTarget(&flicker->target, aura);
                P_SetScale(flicker, aura->scale);
            }
        }
    }
}

void Obj_ChargeFallThink (mobj_t *charge)
{
    if (P_MobjWasRemoved(charge->target) || !charge->target->player)
    {
        P_RemoveMobj(charge);
    }
    else
    {
        mobj_t *mo = charge->target;

        // Follow player
        charge->flags &= ~(MF_NOCLIPTHING);
		P_MoveOrigin(charge, mo->x, mo->y, mo->z);
		charge->flags |= MF_NOCLIPTHING;
        charge->color = mo->color;
        charge->angle = mo->angle + ANGLE_45 + (ANGLE_90 * charge->extravalue1);

        if (!P_IsObjectOnGround(mo))
            charge->renderflags |= RF_DONTDRAW;
        else
            charge->renderflags &= ~RF_DONTDRAW;

        fixed_t baseScale = 12*mo->scale/10;
        P_SetScale(charge, baseScale);

        charge->renderflags &= ~RF_TRANSMASK;
        if (charge->tics < 10)
            charge->renderflags |= (9 - charge->tics)<<RF_TRANSSHIFT;

        // Visuals
        charge->renderflags |= RF_PAPERSPRITE|RF_ADD;
    }
}

// xval1: lifetime (used to offset from tracked player)
void Obj_ChargeReleaseThink (mobj_t *release)
{
    release->renderflags &= ~RF_TRANSMASK;
    if (release->tics < 36)
        release->renderflags |= (9 - release->tics/4)<<RF_TRANSSHIFT;
    release->rollangle += ANG15/2;

    if (P_MobjWasRemoved(release->target) || !release->target->player)
        return;

    release->extravalue1++;

    fixed_t off = 8 * release->extravalue1 * release->target->scale;
    angle_t ang = K_MomentumAngle(release->target) + ANGLE_180;
    fixed_t xoff = FixedMul(off, FINECOSINE(ang >> ANGLETOFINESHIFT));
    fixed_t yoff = FixedMul(off, FINESINE(ang >> ANGLETOFINESHIFT));

    P_MoveOrigin(release, release->target->x + xoff, release->target->y + yoff, release->target->z + release->target->height/2);
}

void Obj_ChargeExtraThink (mobj_t *extra)
{
    extra->renderflags &= ~RF_TRANSMASK;
    if (extra->tics < 18)
        extra->renderflags |= (9 - extra->tics/2)<<RF_TRANSSHIFT;
    extra->rollangle += ANG30;
}
