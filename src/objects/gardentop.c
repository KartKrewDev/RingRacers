// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_local.h"
#include "../s_sound.h"

// TODO: separate from this file
static fixed_t K_FlipZOffset(mobj_t *us, mobj_t *them)
{
	fixed_t z = 0;

	if (them->eflags & MFE_VERTICALFLIP)
		z += them->height;

	if (us->eflags & MFE_VERTICALFLIP)
		z -= us->height;

	return z;
}

#define SPARKCOLOR SKINCOLOR_ROBIN

enum {
	TOP_ANCHORED,
	TOP_LOOSE,
};

#define topsfx_floating sfx_s3k7d
#define topsfx_grinding sfx_s3k79
#define topsfx_lift sfx_s3ka0

#define rider_top(o) ((o)->hnext)

/* All Top states */
#define top_mode(o) ((o)->extravalue1)
#define top_float(o) ((o)->lastlook)
#define top_sound(o) ((o)->extravalue2)
#define top_soundtic(o) ((o)->movecount)
#define top_helpme(o) ((o)->cusval)
#define top_lifetime(o) ((o)->cvmem)

/* TOP_ANCHORED */
#define top_rider(o) ((o)->tracer)

/* TOP_LOOSE */
#define top_waveangle(o) ((o)->movedir)
/* wavepause will take mobjinfo reactiontime automatically */
#define top_wavepause(o) ((o)->reactiontime)

#define spark_top(o) ((o)->target)
#define spark_angle(o) ((o)->movedir)

enum {
	ARROW_OVERHEAD,
	ARROW_IN_FRONT,
};

#define arrow_top(o) ((o)->target)

static inline player_t *
get_rider_player (mobj_t *rider)
{
	return rider ? rider->player : NULL;
}

static inline player_t *
get_top_rider_player (mobj_t *top)
{
	return get_rider_player(top_rider(top));
}

static inline boolean
is_top_grind_input (mobj_t *top)
{
	player_t *player = get_top_rider_player(top);

	return player && K_IsHoldingDownTop(player);
}

static inline boolean
is_top_grinding (mobj_t *top)
{
	if (top_float(top) > 0)
		return false;

	if (!P_IsObjectOnGround(top))
		return false;

	return true;
}

static inline fixed_t
grind_spark_base_scale (player_t *player)
{
	return FRACUNIT/2 +
		player->topdriftheld * FRACUNIT
		/ GARDENTOP_MAXGRINDTIME;
}

static inline INT32
get_player_steer_tilt
(		player_t * player,
		INT32 stages)
{
	return player->steering
		* stages

		// 1 degree for a full turn
		/ KART_FULLTURN
		* ANG1

		// stages is for fractions of a full turn, divide to
		// get a fraction of a degree
		/ stages

		// angle is inverted in reverse gravity
		* P_MobjFlip(player->mo);
}

static inline fixed_t
goofy_shake (fixed_t n)
{
	return P_RandomRange(PR_DECORATION, -1, 1) * n;
}

static inline void
init_top
(		mobj_t * top,
		INT32 mode)
{
	top_mode(top) = mode;
	top_float(top) = 0;
	top_sound(top) = sfx_None;
	top_waveangle(top) = 0;
	top_helpme(top) = (mode == TOP_ANCHORED) ? 1 : 0;
	top_lifetime(top) = 0;
}

static void
spawn_spark
(		mobj_t * top,
		angle_t angle)
{
	mobj_t *spark = P_SpawnMobjFromMobj(
			top, 0, 0, 0, MT_GARDENTOPSPARK);

	P_SetTarget(&spark_top(spark), top);

	spark_angle(spark) = angle;

	spark->color = SPARKCOLOR;
	spark->spriteyscale = 3*FRACUNIT/4;
}

static void
spawn_spark_circle
(		mobj_t * top,
		UINT8 n)
{
	const angle_t a = ANGLE_MAX / n;

	UINT8 i;

	for (i = 0; i < n; ++i)
	{
		spawn_spark(top, i * a);
	}
}

static void
spawn_grind_spark (mobj_t *top)
{
	mobj_t *rider = top_rider(top);
	mobj_t *spark;

	player_t *player = NULL;

	fixed_t x = 0;
	fixed_t y = 0;

	angle_t angle = top->angle;

	if (rider)
	{
		const fixed_t speed = -20  * top->scale;

		angle = K_MomentumAngle(rider);

		x = P_ReturnThrustX(rider, angle, speed);
		y = P_ReturnThrustY(rider, angle, speed);

		player = get_rider_player(rider);
	}

	top_helpme(top) = 0;

	spark = P_SpawnMobjFromMobj(
			top, x, y, 0, MT_DRIFTSPARK);

	spark->momx = x;
	spark->momy = y;

	P_SetMobjState(spark, S_DRIFTSPARK_A1);

	spark->angle = angle;
	spark->color = SPARKCOLOR;

	if (player)
	{
		spark->destscale = FixedMul(spark->destscale,
				grind_spark_base_scale(player));

		P_SetScale(spark, spark->destscale);
	}
}

static void
spawn_arrow (mobj_t *top)
{
	mobj_t *arrow = P_SpawnMobjFromMobj(
			top, 0, 0, 0, MT_GARDENTOPARROW);

	P_SetTarget(&arrow_top(arrow), top);

	P_SetScale(arrow,
			(arrow->destscale = 3 * arrow->scale / 4));
}

static void
loop_sfx
(		mobj_t * top,
		sfxenum_t sfx)
{
	switch (sfx)
	{
		case topsfx_floating:
			if (S_SoundPlaying(top, sfx))
			{
				return;
			}
			break;

		case topsfx_grinding:
			if ((sfxenum_t)top_sound(top) != sfx)
			{
				top_soundtic(top) = leveltime;
			}

			/* FIXME: could this sound just be looped
			   normally? :face_holding_back_tears: */
			if ((leveltime - top_soundtic(top)) % 28 > 0)
			{
				return;
			}
			break;

		default:
			break;
	}

	S_StartSound(top, sfx);
}

static void
modulate (mobj_t *top)
{
	const fixed_t max_hover = top->height / 4;
	const fixed_t hover_step = max_hover / 4;

	sfxenum_t ambience = sfx_None;

	if (is_top_grind_input(top))
	{
		if (top_float(top) == max_hover)
		{
			P_SetMobjState(top, S_GARDENTOP_SINKING1);
		}

		if (top_float(top) > 0)
		{
			top_float(top) = max(0,
					top_float(top) - hover_step);
		}
		else if (P_IsObjectOnGround(top))
		{
			spawn_grind_spark(top);
			ambience = topsfx_grinding;
		}
	}
	else
	{
		if (top_float(top) == 0)
		{
			P_SetMobjState(top, S_GARDENTOP_FLOATING);

			S_StopSoundByID(top, topsfx_grinding);
			S_StartSound(top, topsfx_lift);
		}

		if (top_float(top) < max_hover)
		{
			top_float(top) = min(max_hover,
					top_float(top) + hover_step);
		}
		else
		{
			ambience = topsfx_floating;
		}
	}

	top->sprzoff = top_float(top) * P_MobjFlip(top);

	if (ambience)
	{
		loop_sfx(top, ambience);
	}

	top_sound(top) = ambience;
}

static void
tilt (mobj_t *top)
{
	player_t *player = get_top_rider_player(top);

	INT32 tilt = top->rollangle;

	if (is_top_grind_input(top))
	{
		const angle_t tiltmax = ANGLE_22h;

		tilt += get_player_steer_tilt(player, 4);

		if (abs(tilt) > tiltmax)
		{
			tilt = intsign(tilt) * tiltmax;
		}
	}
	else
	{
		const angle_t decay = ANG1 * 2;

		if (abs(tilt) > decay)
		{
			tilt -= intsign(tilt) * decay;
		}
		else
		{
			tilt = 0;
		}
	}

	top->rollangle = tilt;

	/* Vibrate left and right if you're about to lose it. */
	if (player && player->topinfirst)
	{
		top->spritexoffset = P_LerpFlip(32*FRACUNIT, 1);
	}
	else
	{
		top->spritexoffset = 0;
	}

	/* Go ABSOLUTELY NUTS if the player is tumbling... */
	if (player && player->tumbleBounces > 0)
	{
		const fixed_t yofs = 48 * FRACUNIT;
		const fixed_t ofs3d = 24 * top->scale;

		/* spriteyoffset scales, e.g. with K_Squish */
		top->spriteyoffset = FixedDiv(
				goofy_shake(yofs), top->spriteyscale);

		top->sprxoff = goofy_shake(ofs3d);
		top->spryoff = goofy_shake(ofs3d);
	}
	else
	{
		top->spriteyoffset = 0;
		top->sprxoff = 0;
		top->spryoff = 0;
	}
}

static void
anchor
(		mobj_t * us,
		mobj_t * them,
		angle_t angle,
		fixed_t radius)
{
	const fixed_t x = P_ReturnThrustX(us, angle, radius);
	const fixed_t y = P_ReturnThrustY(us, angle, radius);

	K_MatchFlipFlags(us, them);

	P_MoveOrigin(us, them->x + x, them->y + y,
			them->z + K_FlipZOffset(us, them));

	us->angle = angle;
}

static void
anchor_top (mobj_t *top)
{
	mobj_t *rider = top_rider(top);
	player_t *player = get_rider_player(rider);

	if (player && player->curshield != KSHIELD_TOP)
	{
		P_RemoveMobj(top);
		return;
	}

	/* Rider lost track of this object. */
	if (rider_top(rider) != top)
	{
		P_RemoveMobj(top);
		return;
	}

	tilt(top);

	anchor(top, rider, rider->angle, 0);

	K_MatchGenericExtraFlagsNoZAdjust(top, rider);

	/* Copying the Z momentum lets the Top squash and stretch
	   as it falls with the player. Don't copy the X/Y
	   momentum because then it would always get slightly
	   ahead of the player. */
	top->momx = 0;
	top->momy = 0;
	top->momz = rider->momz;

	top_lifetime(top)++;

	/* The Z momentum can put the Top slightly ahead of the
	   player in that axis too. It looks cool if the Top
	   falls below you but not if it bounces up. */
	if (top->momz * P_MobjFlip(top) > 0)
	{
		top->momz = 0;
	}

	/* match rider's slope tilt */
	top->pitch = rider->pitch;
	top->roll = rider->roll;
}

static void
loose_think (mobj_t *top)
{
	const fixed_t thrustamount = top->movefactor / 2;
	const angle_t momangle = K_MomentumAngle(top);

	angle_t ang = top->angle;

	mobj_t *ghost = P_SpawnGhostMobj(top);
	ghost->colorized = true; // already has color!

	if (AngleDelta(ang, momangle) > ANGLE_90)
	{
		top->angle = momangle;
	}

	if (top_wavepause(top))
	{
		top_wavepause(top)--;
	}
	else
	{
		/* oscillate between +90 and -90 degrees */
		ang += AbsAngle(top_waveangle(top)) - ANGLE_90;
	}

	P_InstaThrust(top, top->angle, thrustamount);
	P_Thrust(top, ang, thrustamount);

	//top_waveangle(top) = (angle_t)top_waveangle(top) + ANG10;
	top_waveangle(top) += ANG10;

	/* intangibility grace period */
	if (top->threshold > 0)
	{
		top->threshold--;
	}
}

static void
anchor_spark (mobj_t *spark)
{
	mobj_t *top = spark_top(spark);
	mobj_t *rider = top_rider(top);
	player_t *player = get_rider_player(rider);

	anchor(spark, top,
			(top->angle + spark_angle(spark)), spark->scale);

	if (player)
	{
		const fixed_t topspeed =
			K_GetKartSpeed(player, false, false);

		const fixed_t speed = FixedHypot(
				rider->momx, rider->momy);

		P_SetScale(spark, FixedMul(top->scale, FRACUNIT/2 +
					FixedDiv(speed / 2, topspeed)));
	}
}

static void
anchor_arrow_overhead (mobj_t *arrow)
{
	mobj_t *top = arrow_top(arrow);
	mobj_t *rider = top_rider(top);

	const fixed_t height =
		top->height + rider->height + (3 * arrow->height / 4);

	arrow->sprzoff = top->sprzoff +
		(height * P_MobjFlip(arrow));

	anchor(arrow, top, rider->angle + ANGLE_180, 0);
}

void
Obj_GardenTopDeploy (mobj_t *rider)
{
	player_t *player = rider->player;

	mobj_t *top = P_SpawnMobjFromMobj(
			rider, 0, 0, 0, MT_GARDENTOP);

	init_top(top, TOP_ANCHORED);

	top->flags |= MF_NOCLIPHEIGHT;

	/* only the player's shadow needs to be rendered */
	top->shadowscale = 0;

	P_SetTarget(&top_rider(top), rider);
	P_SetTarget(&rider_top(rider), top);

	if (player)
	{
		player->curshield = KSHIELD_TOP;
		rider->radius = K_DefaultPlayerRadius(player);

		/* Doing this here to set itemscale.
		   And unset right afterward so the item box doesn't flicker! */
		K_SetItemOut(player);
		P_InstaScale(top, K_ItemScaleForPlayer(player));
		K_UnsetItemOut(player);
	}

	spawn_spark_circle(top, 6);

	spawn_arrow(top);
}

mobj_t *
Obj_GardenTopThrow (player_t *player)
{
	mobj_t *top = K_GetGardenTop(player);

	if (top)
	{
		const fixed_t oldfloat = top_float(top);
		const fixed_t height = top->height;

		/* Sucks that another one needs to be spawned but
		   this way, the throwing function can be used. */
		top = K_ThrowKartItem(
				player, true, MT_GARDENTOP, 1, 0, 0);

		K_UpdateHnextList(player, true);

		init_top(top, TOP_LOOSE);

		top_float(top) = oldfloat;
		top_waveangle(top) = 0;

		/* prevents it from hitting us on its way out */
		top->threshold = 20;

		/* ensure it's tangible */
		top->flags &= ~(MF_NOCLIPTHING);

		/* Put player PHYSICALLY on top. While riding the
		   Top, player collision was used and the player
		   technically remained on the ground. Now they
		   should fall off. */
		P_SetOrigin(player->mo, player->mo->x, player->mo->y,
				player->mo->z + height * P_MobjFlip(player->mo));
	}

	if (player->itemamount > 0)
		K_AdjustPlayerItemAmount(player, -1);

	if (player->itemamount <= 0)
		player->itemtype = KITEM_NONE;

	player->curshield = KSHIELD_NONE;

	player->mo->radius = K_DefaultPlayerRadius(player);

	return top;
}

mobj_t *
Obj_GardenTopDestroy (player_t *player)
{
	mobj_t *top = Obj_GardenTopThrow(player);

	if (top)
	{
		/* kill kill kill die die die */
		P_KillMobj(top, NULL, NULL, DMG_NORMAL);
	}

	return top;
}

void
Obj_GardenTopThink (mobj_t *top)
{
	modulate(top);

	switch (top_mode(top))
	{
		case TOP_ANCHORED:
			if (top_rider(top))
			{
				anchor_top(top);
			}
			break;

		case TOP_LOOSE:
			loose_think(top);
			break;
	}
}

void
Obj_GardenTopSparkThink (mobj_t *spark)
{
	mobj_t *top = spark_top(spark);

	if (!top)
	{
		P_RemoveMobj(spark);
		return;
	}

	anchor_spark(spark);

	if (is_top_grinding(top))
	{
		spark->renderflags ^= RF_DONTDRAW;
	}
	else
	{
		spark->renderflags |= RF_DONTDRAW;
	}
}

void
Obj_GardenTopArrowThink (mobj_t *arrow)
{
	mobj_t *top = arrow_top(arrow);
	mobj_t *rider = top ? top_rider(top) : NULL;

	if (!rider)
	{
		P_RemoveMobj(arrow);
		return;
	}

	anchor_arrow_overhead(arrow);

	if (rider->player)
	{
		// Don't show for other players
		arrow->renderflags =
			(arrow->renderflags & ~(RF_DONTDRAW)) |
			(RF_DONTDRAW & ~(K_GetPlayerDontDrawFlag(rider->player)));
	}
}

boolean
Obj_GardenTopPlayerIsGrinding (const player_t *player)
{
	mobj_t *top = K_GetGardenTop(player);

	return top ? is_top_grinding(top) : false;
}

boolean
Obj_GardenTopPlayerNeedsHelp (const mobj_t *top)
{
	if (top && top_mode(top) != TOP_ANCHORED)
		return false;
	return top ? (top_helpme(top) || top_lifetime(top) < 3*TICRATE) : false;
}
