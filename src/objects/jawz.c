// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  jawz.c
/// \brief Jawz item code.

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../g_game.h"
#include "../z_zone.h"
#include "../k_waypoint.h"
#include "../k_respawn.h"
#include "../k_collide.h"
#include "../k_specialstage.h"

#define MAX_JAWZ_TURN (ANGLE_90 / 15) // We can turn a maximum of 6 degrees per frame at regular max speed

#define jawz_speed(o) ((o)->movefactor)
#define jawz_selfdelay(o) ((o)->threshold)
#define jawz_dropped(o) ((o)->flags2 & MF2_AMBUSH)
#define jawz_droptime(o) ((o)->movecount)

#define jawz_retcolor(o) ((o)->extravalue2)
#define jawz_stillturn(o) ((o)->cusval)

#define jawz_owner(o) ((o)->target)
#define jawz_chase(o) ((o)->tracer)

static void JawzChase(mobj_t *th, boolean grounded)
{
	fixed_t thrustamount = 0;
	fixed_t frictionsafety = (th->friction == 0) ? 1 : th->friction;
	fixed_t topspeed = jawz_speed(th);

	if (jawz_chase(th) != NULL && P_MobjWasRemoved(jawz_chase(th)) == false)
	{
		if (jawz_chase(th)->health > 0)
		{
			const angle_t targetangle = R_PointToAngle2(
				th->x, th->y,
				jawz_chase(th)->x, jawz_chase(th)->y
			);
			angle_t angledelta = th->angle - targetangle;
			mobj_t *ret = NULL;

			if (gametyperules & GTR_CIRCUIT)
			{
				const fixed_t distbarrier = FixedMul(
					512 * mapobjectscale,
					FRACUNIT + ((gamespeed-1) * (FRACUNIT/4))
				);

				const fixed_t distaway = P_AproxDistance(
					jawz_chase(th)->x - th->x,
					jawz_chase(th)->y - th->y
				);

				if (distaway < distbarrier)
				{
					if (jawz_chase(th)->player != NULL)
					{
						fixed_t speeddifference = abs(
							topspeed - min(
								jawz_chase(th)->player->speed,
								K_GetKartSpeed(jawz_chase(th)->player, false, false)
							)
						);

						topspeed = topspeed - FixedMul(speeddifference, FRACUNIT - FixedDiv(distaway, distbarrier));
					}
				}
			}

			if (angledelta != 0)
			{
				angle_t turnSpeed = MAX_JAWZ_TURN;
				boolean turnclockwise = true;

				// MAX_JAWZ_TURN gets stronger the slower the top speed of jawz
				if (topspeed < jawz_speed(th))
				{
					if (topspeed == 0)
					{
						turnSpeed = ANGLE_180;
					}
					else
					{
						fixed_t anglemultiplier = FixedDiv(jawz_speed(th), topspeed);
						turnSpeed += FixedAngle(FixedMul(AngleFixed(turnSpeed), anglemultiplier));
					}
				}

				if (angledelta > ANGLE_180)
				{
					angledelta = InvAngle(angledelta);
					turnclockwise = false;
				}

				if (angledelta > turnSpeed)
				{
					angledelta = turnSpeed;
				}

				if (turnclockwise == true)
				{
					th->angle -= angledelta;
				}
				else
				{
					th->angle += angledelta;
				}
			}

			ret = P_SpawnMobjFromMobj(jawz_chase(th), 0, 0, 0, MT_PLAYERRETICULE);
			ret->old_x = jawz_chase(th)->old_x;
			ret->old_y = jawz_chase(th)->old_y;
			ret->old_z = jawz_chase(th)->old_z;
			P_SetTarget(&ret->target, jawz_chase(th));
			ret->frame |= ((leveltime % 10) / 2) + 5;
			ret->color = jawz_retcolor(th);
			ret->renderflags = (ret->renderflags & ~RF_DONTDRAW) | (th->renderflags & RF_DONTDRAW);
			ret->hitlag = 0; // spawns every tic, so don't inherit player hitlag
		}
		else
		{
			P_SetTarget(&jawz_chase(th), NULL);
		}
	}

	if (jawz_chase(th) == NULL || P_MobjWasRemoved(jawz_chase(th)) == true)
	{
		mobj_t *newChase = NULL;
		player_t *owner = NULL;

		th->angle = K_MomentumAngle(th);

		if ((jawz_owner(th) != NULL && P_MobjWasRemoved(jawz_owner(th)) == false)
			&& (jawz_owner(th)->player != NULL))
		{
			owner = jawz_owner(th)->player;
		}

		newChase = K_FindJawzTarget(th, owner, ANGLE_90);
		if (newChase != NULL)
		{
			P_SetTarget(&jawz_chase(th), newChase);
		}
	}

	if (jawz_stillturn(th) > 0)
	{
		// When beginning to chase your own owner,
		// we should turn but not thrust quite yet.
		return;
	}

	if (grounded == true)
	{
		const fixed_t currentspeed = R_PointToDist2(0, 0, th->momx, th->momy);

		if (currentspeed >= topspeed)
		{
			// Thrust as if you were at top speed, slow down naturally
			thrustamount = FixedDiv(topspeed, frictionsafety) - topspeed;
		}
		else
		{
			const fixed_t beatfriction = FixedDiv(currentspeed, frictionsafety) - currentspeed;
			// Thrust to immediately get to top speed
			thrustamount = beatfriction + FixedDiv(topspeed - currentspeed, frictionsafety);
		}

		P_Thrust(th, th->angle, thrustamount);
	}
}

static boolean JawzSteersBetter(void)
{
	return !!!(gametyperules & GTR_CIRCUIT);
}

void Obj_JawzThink(mobj_t *th)
{
	mobj_t *ghost = P_SpawnGhostMobj(th);
	boolean grounded = P_IsObjectOnGround(th);

	if (th->fuse > 0 && th->fuse <= TICRATE)
	{
		th->renderflags ^= RF_DONTDRAW;
	}

	if (jawz_dropped(th))
	{
		if (grounded && (th->flags & MF_NOCLIPTHING))
		{
			th->momx = 1;
			th->momy = 0;
			S_StartSound(th, th->info->deathsound);
			th->flags &= ~MF_NOCLIPTHING;
		}

		return;
	}

	if (jawz_owner(th) != NULL && P_MobjWasRemoved(jawz_owner(th)) == false
		&& jawz_owner(th)->player != NULL)
	{
		ghost->color = jawz_owner(th)->player->skincolor;
		ghost->colorized = true;
	}

	if (JawzSteersBetter() == true && !jawz_stillturn(th) && (th->momx != 0 && th->momy != 0))
	{
		th->friction = max(0, 3 * th->friction / 4);
	}

	JawzChase(th, grounded);
	K_DriftDustHandling(th);

	/* todo: UDMFify
	if (P_MobjTouchingSectorSpecialFlag(th, ?))
	{
		K_DoPogoSpring(th, 0, 1);
	}
	*/

	if (jawz_selfdelay(th) > 0)
	{
		jawz_selfdelay(th)--;
	}

	if (jawz_stillturn(th) > 0)
	{
		jawz_stillturn(th)--;
	}

	if (leveltime % TICRATE == 0)
	{
		S_StartSound(th, th->info->activesound);
	}
}

void Obj_JawzThrown(mobj_t *th, fixed_t finalSpeed, fixed_t dir)
{
	INT32 lastTarg = -1;
	player_t *owner = NULL;

	if (jawz_owner(th) != NULL && P_MobjWasRemoved(jawz_owner(th)) == false
		&& jawz_owner(th)->player != NULL)
	{
		lastTarg = jawz_owner(th)->player->lastjawztarget;
		jawz_retcolor(th) = jawz_owner(th)->player->skincolor;
		owner = jawz_owner(th)->player;
	}
	else
	{
		jawz_retcolor(th) = SKINCOLOR_KETCHUP;
	}

	if (dir < 0)
	{
		// Thrown backwards, init self-chase
		P_SetTarget(&jawz_chase(th), jawz_owner(th));

		// Stop it here.
		th->momx = 0;
		th->momy = 0;

		// Return at absolutely 120% of the owner's speed if it's any less than that.
		fixed_t min_backthrowspeed = 6*(K_GetKartSpeed(owner, false, false))/5;
		if (owner->speed >= min_backthrowspeed)
		{
			finalSpeed = 6*(owner->speed)/5;
		}
		else
		{
			finalSpeed = min_backthrowspeed;
		}

		// Set a fuse.
		th->fuse = RR_PROJECTILE_FUSE;

		// Stay still while you turn towards the player
		jawz_stillturn(th) = ANGLE_180 / MAX_JAWZ_TURN;
	}
	else
	{
		if ((lastTarg >= 0 && lastTarg < MAXPLAYERS)
			&& playeringame[lastTarg] == true)
		{
			player_t *tryPlayer = &players[lastTarg];

			if (tryPlayer->spectator == false)
			{
				P_SetTarget(&jawz_chase(th), tryPlayer->mo);
			}
		}

		// Sealed Star: target the UFO immediately. I don't
		// wanna fuck with the lastjawztarget stuff, so just
		// do this if a target wasn't set.
		if (jawz_chase(th) == NULL || P_MobjWasRemoved(jawz_chase(th)) == true)
		{
			P_SetTarget(&jawz_chase(th), K_FindJawzTarget(th, owner, ANGLE_90));
		}
	}

	S_StartSound(th, th->info->activesound);
	jawz_speed(th) = finalSpeed;
}
