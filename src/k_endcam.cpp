// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>

#include "archive_wrapper.hpp"

#include "byteptr.h"
#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"
#include "g_game.h"
#include "k_battle.h"
#include "k_endcam.h"
#include "m_easing.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_saveg.h"
#include "p_tick.h"
#include "r_fps.h"
#include "r_main.h"

endcam_t g_endcam;

namespace
{

fixed_t interval(tic_t t, tic_t d)
{
	return (std::min(t, d) * FRACUNIT) / std::max<tic_t>(d, 1u);
}

fixed_t interval(tic_t t, tic_t s, tic_t d)
{
	return interval(std::max(t, s) - s, d);
}

INT32 lerp(fixed_t f, INT32 a, INT32 b)
{
	return a + FixedMul(f, b - a);
}

struct Camera : camera_t
{
	void pos(const vector3_t& p)
	{
		x = p.x;
		y = p.y;
		z = p.z;
		subsector = R_PointInSubsector(x, y);
	}
};

struct EndCam : endcam_t
{
	tic_t Time() const { return leveltime - begin; }
	bool Freezing() const { return active && Time() <= swirlDuration; }

	void GC()
	{
		if (P_MobjWasRemoved(panMobj))
		{
			P_SetTarget(&panMobj, nullptr);
		}
	}

	void Start()
	{
		active = true;
		begin = leveltime;

		// Reset all viewpoints
		for (int i = 0; i < MAXSPLITSCREENPLAYERS; ++i)
		{
			Move(static_cast<Camera&>(camera[i]));
		}

		R_ResetViewInterpolation(0);
	}

	void Move(Camera& cam)
	{
		tic_t t = Time();
		fixed_t pan = Easing_OutQuint(interval(t, swirlDuration, panDuration), FRACUNIT, 0);

		auto aim = [&](vector3_t& p, vector3_t& q)
		{
			cam.aiming = lerp(pan, cam.aiming, R_PointToAngle2(0, p.z, FixedHypot(q.x - p.x, q.y - p.y), q.z));
			cam.pos(p);
		};

		if (t <= swirlDuration)
		{
			fixed_t swirl = interval(t, swirlDuration);

			angle_t ang = FixedAngle(swirl < FRACUNIT/2 ?
				Easing_InOutQuint(swirl, startAngle, endAngle) :
				Easing_InOutQuad(swirl, startAngle, endAngle));

			fixed_t hDist = Easing_OutQuad(swirl, startRadius.x, endRadius.x);

			vector3_t p = {
				origin.x - FixedMul(FCOS(ang), hDist),
				origin.y - FixedMul(FSIN(ang), hDist),
				origin.z + Easing_OutQuad(swirl, startRadius.y, endRadius.y)
			};

			aim(p, origin);
			cam.angle = ang;
		}
		else if (!P_MobjWasRemoved(panMobj))
		{
			vector3_t q = {panMobj->x, panMobj->y, P_GetMobjHead(panMobj)};
			vector3_t p = {cam.x, cam.y, cam.z};
			Follow(FixedMul(pan, panSpeed), p, q); // modifies p

			aim(p, q);
			cam.angle = lerp(
				Easing_Linear(pan, FRACUNIT/4, FRACUNIT),
				cam.angle,
				R_PointToAngle2(p.x, p.y, q.x, q.y)
			);
		}

	}

	void Stop()
	{
		active = false;
	}

	template <typename T>
	void Archive(T&& ar)
	{
		static_assert(srb2::is_archive_wrapper_v<T>);
#define X(T, var) SRB2_ARCHIVE_WRAPPER_CALL(ar, T, var)
		X(vector3_t, origin);
		X(vector2_t, startRadius);
		X(vector2_t, endRadius);
		X(tic_t, swirlDuration);
		X(fixed_t, startAngle);
		X(fixed_t, endAngle);
		// panMobj is handled in p_saveg.c
		X(tic_t, panDuration);
		X(fixed_t, panSpeed);
		X(bool, active);
		X(tic_t, begin);
#undef X
	}

private:
	void Follow(fixed_t f, vector3_t& p, vector3_t q) const
	{
		FV3_Sub(&q, &p);
		q.x = FixedMul(q.x, f);
		q.y = FixedMul(q.y, f);
		q.z = FixedMul(q.z, f);

		FV3_Add(&p, &q);
	}
};

EndCam& endcam_cast()
{
	return static_cast<EndCam&>(g_endcam);
}

}; // namespace

void K_CommitEndCamera(void)
{
	// Level will be frozen, so make sure the lasers are
	// spawned before that happens.
	K_SpawnOvertimeBarrier();

	endcam_cast().Start();
}

void K_MoveEndCamera(camera_t *thiscam)
{
	endcam_cast().Move(static_cast<Camera&>(*thiscam));
}

void K_EndCameraGC(void)
{
	endcam_cast().GC();
}

boolean K_EndCameraIsFreezing(void)
{
	return endcam_cast().Freezing();
}

void K_SaveEndCamera(savebuffer_t *save)
{
	endcam_cast().Archive(srb2::ArchiveWrapper(save));
}

void K_LoadEndCamera(savebuffer_t *save)
{
	endcam_cast().Archive(srb2::UnArchiveWrapper(save));
}

void K_StartRoundWinCamera(mobj_t *origin, angle_t focusAngle, fixed_t finalRadius, tic_t panDuration, fixed_t panSpeed, tic_t swirlDuration)
{
	const fixed_t angF = AngleFixed(focusAngle);

	g_endcam.origin = {origin->x, origin->y, P_GetMobjHead(origin)};
	g_endcam.startRadius = {2400*mapobjectscale, 800*mapobjectscale};
	g_endcam.endRadius = {finalRadius, finalRadius / 2};

	g_endcam.swirlDuration = swirlDuration;
	g_endcam.startAngle = angF + (90*FRACUNIT);
	g_endcam.endAngle = angF + (720*FRACUNIT);

	P_SetTarget(&g_endcam.panMobj, origin);
	g_endcam.panDuration = panDuration;
	g_endcam.panSpeed = panSpeed;

	K_CommitEndCamera();

	g_darkness.start = leveltime;
	g_darkness.end = leveltime + g_endcam.swirlDuration + DARKNESS_FADE_TIME;
}

void K_StopRoundWinCamera(void)
{
	endcam_cast().Stop();
}
