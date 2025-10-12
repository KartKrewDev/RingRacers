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
/// \brief EXP VFX code.

#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../k_kart.h"
#include "../k_powerup.h"
#include "../m_random.h"
#include "../r_main.h"
#include "../m_easing.h"
#include "../s_sound.h"
#include "../sounds.h"

#define EXP_ARCTIME (8)
#define EXP_ORBIT (100)

static void ghostme(mobj_t *exp, player_t *player)
{
	if (exp->cusval%2)
		return;

	mobj_t *ghost = P_SpawnGhostMobj(exp);
	ghost->colorized = true;
	ghost->color = player->skincolor;
	ghost->renderflags |= RF_ADD;
	ghost->fuse = 2;
}

void Obj_ExpThink (mobj_t *exp)
{
	if (P_MobjWasRemoved(exp->target)
		|| exp->target->health == 0
		|| exp->target->destscale <= 1 // sealed star fall out
		|| !exp->target->player)
	{
		P_RemoveMobj(exp);
	}
	else
	{
		mobj_t *mo = exp->target;
		player_t *player = mo->player;

		fixed_t dist, fakez;
		angle_t hang, vang;

		dist = P_AproxDistance(P_AproxDistance(exp->x - mo->x, exp->y - mo->y), exp->z - mo->z);

		exp->renderflags |= RF_DONTDRAW;
		exp->renderflags &= ~K_GetPlayerDontDrawFlag(player);

		// K_MatchGenericExtraFlags(exp, mo);

		exp->cusval++;

		// bullshit copypaste orbit behavior
		if (exp->threshold)
		{
			fixed_t orbit = (4*mo->scale) * (16 - exp->extravalue1);

			P_SetScale(exp, (exp->destscale = mapobjectscale - ((mapobjectscale/28) * exp->extravalue1)));
			exp->z = exp->target->z;
			P_MoveOrigin(exp,
				mo->x + FixedMul(orbit, FINECOSINE(exp->angle >> ANGLETOFINESHIFT)),
				mo->y + FixedMul(orbit, FINESINE(exp->angle >> ANGLETOFINESHIFT)),
				exp->z + mo->scale * 24 * P_MobjFlip(exp));

			exp->momx = 0;
			exp->momy = 0;
			exp->momz = 0;

			ghostme(exp, player);

			exp->angle += ANG30;
			exp->extravalue1++;

			if (exp->extravalue1 >= 16)
			{
				if(P_IsDisplayPlayer(player)) // As you know Kris, I am FUCKING your EXP.
				{
				S_StopSoundByID(exp->target, sfx_exp);
				S_StartSound(exp->target, sfx_exp);
				}
				P_RemoveMobj(exp);
			}


			return;
		}

		exp->angle += ANGLE_45/2;

		UINT8 damper = 3;

		fixed_t vert = dist/3;
		fixed_t speed = 60*exp->scale;

		if (exp->extravalue2) // Mode: going down, aim at the player and speed up / dampen stray movement
		{
			if (exp->extravalue1)
				exp->extravalue1--;

			exp->extravalue2++;

			speed += exp->extravalue2 * exp->scale/2;

			fakez = mo->z + (vert * exp->extravalue1 / EXP_ARCTIME);
			damper = 1;
		}
		else // Mode: going up, aim above the player
		{
			exp->extravalue1++;
			if (exp->extravalue1 >= EXP_ARCTIME)
				exp->extravalue2 = 1;

			fakez = mo->z + vert;
		}

		if (mo->flags & MFE_VERTICALFLIP)
			fakez -= mo->height/2;
		else
			fakez += mo->height/2;

		hang = R_PointToAngle2(exp->x, exp->y, mo->x, mo->y);
		vang = R_PointToAngle2(exp->z, 0, fakez, dist);

		exp->momx -= exp->momx>>(damper), exp->momy -= exp->momy>>(damper), exp->momz -= exp->momz>>(damper);
		exp->momx += FixedMul(FINESINE(vang>>ANGLETOFINESHIFT), FixedMul(FINECOSINE(hang>>ANGLETOFINESHIFT), speed));
		exp->momy += FixedMul(FINESINE(vang>>ANGLETOFINESHIFT), FixedMul(FINESINE(hang>>ANGLETOFINESHIFT), speed));
		exp->momz += FixedMul(FINECOSINE(vang>>ANGLETOFINESHIFT), speed);

		ghostme(exp, player);

		if (dist < (EXP_ORBIT * exp->scale) && exp->extravalue2)
		{
			exp->threshold = TICRATE;
			exp->extravalue1 = 0;
			exp->extravalue2 = 0;
		}
	}
}
