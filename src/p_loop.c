// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_loop.c
/// \brief Sonic loop physics

#include "doomdef.h"
#include "d_player.h"
#include "k_kart.h"
#include "p_local.h"
#include "p_setup.h"
#include "p_slopes.h"
#include "r_main.h"

static inline angle_t
get_pitch (fixed_t revolution)
{
	return FixedAngle((revolution & FRACMASK) * 360);
}

static inline fixed_t
normal_revolution (const sonicloopvars_t *s)
{
	return FixedDiv(
			s->revolution - s->min_revolution,
			s->max_revolution - s->min_revolution);
}

void P_HaltPlayerOrbit(player_t *player)
{
	// see P_PlayerOrbit
	player->mo->flags &= ~(MF_NOCLIPHEIGHT);

	player->loop.radius = 0;
	player->loop.camera.exit_tic = leveltime;
}

void P_ExitPlayerOrbit(player_t *player)
{
	sonicloopvars_t *s = &player->loop;

	angle_t pitch = get_pitch(s->revolution);
	angle_t yaw = s->yaw;

	fixed_t co, si;
	fixed_t speed;

	if (s->radius < 0)
	{
		pitch += ANGLE_180;
	}

	co = FCOS(pitch);
	si = FSIN(pitch);

	speed = FixedMul(co, player->speed);

	P_InstaThrust(player->mo, yaw, speed);

	player->mo->momz = FixedMul(si, player->speed);

	if (speed < 0)
	{
		yaw += ANGLE_180;
	}

	// excludes only extremely vertical angles
	if (abs(co) * 4 > abs(si))
	{
		P_SetPlayerAngle(player, yaw);
	}

	if (s->flip)
	{
		player->mo->eflags ^= MFE_VERTICALFLIP;
		player->mo->flags2 ^= MF2_OBJECTFLIP;

		P_SetPitchRoll(player->mo,
				pitch + ANGLE_180, s->yaw);
	}

	// tiregrease gives less friction, extends momentum
	K_SetTireGrease(player, 3*TICRATE);

	P_HaltPlayerOrbit(player);
}

boolean P_PlayerOrbit(player_t *player)
{
	sonicloopvars_t *s = &player->loop;

	angle_t pitch;
	angle_t pitch_normal;

	fixed_t r, xy, z;
	fixed_t xs, ys;

	fixed_t step, left;

	fixed_t grav;

	if (s->radius == 0)
	{
		return false;
	}

	grav = abs(P_GetMobjGravity(player->mo));

	// Lose speed on the way up. revolution = 0.5 always
	// points straight up.
	if (abs(s->revolution & FRACMASK) < FRACUNIT/2)
	{
		player->speed -= grav;
	}
	else
	{
		player->speed += 4 * grav;
	}

	pitch = get_pitch(s->revolution);
	pitch_normal = get_pitch(normal_revolution(s) / 2);

	r = abs(s->radius) -
		FixedMul(player->mo->radius, abs(FSIN(pitch)));

	xy = FixedMul(r, FSIN(pitch));

	z = FixedMul(abs(s->radius), -(FCOS(pitch))) -
		FixedMul(player->mo->height, FSIN(pitch / 2));

	// XY shift is transformed on wave scale; less movement
	// at start and end of rotation, more halfway.
	xs = FixedMul(s->shift.x, FCOS(pitch_normal));
	ys = FixedMul(s->shift.y, FSIN(pitch_normal - ANGLE_90));

	// Interpolate 0-1 over entire rotation.
	xs += FixedMul(s->origin_shift.x, FCOS(pitch_normal));
	ys += FixedMul(s->origin_shift.y, FSIN(pitch_normal - ANGLE_90));

	xs += FixedMul(xy, FCOS(s->yaw));
	ys += FixedMul(xy, FSIN(s->yaw));

	P_MoveOrigin(player->mo,
			s->origin.x + xs,
			s->origin.y + ys,
			s->origin.z + z);

	// Match rollangle to revolution
	P_SetPitchRoll(player->mo,
			s->radius < 0 ? (ANGLE_180 + pitch) : pitch,
			s->yaw);

	// circumfrence = (2r)PI
	step = FixedDiv(player->speed,
			FixedMul(s->radius, M_TAU_FIXED));

	left = (s->max_revolution - s->revolution);

	if (left == 0)
	{
		P_ExitPlayerOrbit(player);

		return false;
	}

	if (abs(left) < abs(step))
	{
		step = left;
	}

	// If player slows down by too much, throw them
	// out of the loop and reset them.
	// (markedfordeath will kill the player on their
	// first ground contact!)
	if (player->speed < player->mo->scale)
	{
		P_HaltPlayerOrbit(player);
		player->markedfordeath = true;
		K_PlayPainSound(player->mo, NULL);
		K_StumblePlayer(player);

		return false;
	}

	s->revolution += step;

	// We need to not clip the ground. It sucks but setting
	// this flag is the only way to do that.
	player->mo->flags |= (MF_NOCLIPHEIGHT);

	return true;
}
