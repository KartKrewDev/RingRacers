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

#include <fmt/format.h>

#include "../mobj_list.hpp"

#include "../core/hash_map.hpp"
#include "../core/vector.hpp"
#include "../doomdef.h"
#include "../doomtype.h"
#include "../info.h"
#include "../g_game.h"
#include "../k_color.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_bbox.h"
#include "../m_fixed.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../p_maputl.h"
#include "../p_mobj.h"
#include "../p_setup.h"
#include "../p_tick.h"
#include "../r_defs.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../sounds.h"
#include "../tables.h"

using std::min;
using std::max;
using std::clamp;

extern mobj_t* svg_checkpoints;

#define checkpoint_id(o) ((o)->thing_args[0])
#define checkpoint_linetag(o) ((o)->thing_args[1])
#define checkpoint_other(o) ((o)->target)
#define checkpoint_orb(o) ((o)->tracer)
#define checkpoint_arm(o) ((o)->hnext)
#define checkpoint_next(o) ((o)->hprev)
#define checkpoint_var(o) ((o)->movedir)
#define checkpoint_speed(o) ((o)->movecount)
#define checkpoint_speed_multiplier(o) ((o)->movefactor)
#define checkpoint_reverse(o) ((o)->reactiontime)

namespace
{

struct LineOnDemand : line_t
{
private:
	vertex_t v1_data_;

public:
	LineOnDemand(const line_t* line) {}

	LineOnDemand(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2) :
		line_t {},
		v1_data_{ x1, y1 }
	{
		v1 = &v1_data_;
		dx = x2 - x1;
		dy = y2 - y1;
		bbox[0] = max(y1, y2);
		bbox[1] = min(y1, y2);
		bbox[2] = min(x1, x2);
		bbox[3] = max(x1, x2);
	};

	LineOnDemand(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, fixed_t r) : LineOnDemand(x1, y1, x2, y2)
	{
		bbox[BOXTOP] += r;
		bbox[BOXBOTTOM] -= r;
		bbox[BOXLEFT] -= r;
		bbox[BOXRIGHT] += r;
	}

	bool overlaps(const LineOnDemand& other) const
	{
		return bbox[BOXTOP] >= other.bbox[BOXBOTTOM] && bbox[BOXBOTTOM] <= other.bbox[BOXTOP] &&
			bbox[BOXLEFT] <= other.bbox[BOXRIGHT] && bbox[BOXRIGHT] >= other.bbox[BOXLEFT];
	}

	bool overlaps(const line_t& other) const
	{
		return bbox[BOXTOP] >= other.bbox[BOXBOTTOM] && bbox[BOXBOTTOM] <= other.bbox[BOXTOP] &&
			bbox[BOXLEFT] <= other.bbox[BOXRIGHT] && bbox[BOXRIGHT] >= other.bbox[BOXLEFT];
	}
};

struct Checkpoint : mobj_t
{
	static constexpr int kArmLength = 59;
	static constexpr int kOrbRadius = 21;
	static constexpr int kCookieRadius = 19;
	static constexpr int kOrbitDistance = kCookieRadius + kArmLength + kOrbRadius;
	static constexpr fixed_t kBaseSpeed = FRACUNIT/35;
	static constexpr fixed_t kMinSpeedMultiplier = FRACUNIT/2;
	static constexpr fixed_t kMaxSpeedMultiplier = 3*FRACUNIT/2;
	static constexpr fixed_t kSpeedMultiplierRange = kMaxSpeedMultiplier - kMinSpeedMultiplier;
	static constexpr fixed_t kMinPivotDelay = FRACUNIT/2;
	static constexpr int kSparkleOffset = 10;
	static constexpr int kSparkleZ = 34;
	static constexpr int kSparkleRadius = 12;
	static constexpr int kSparkleAroundRadius = 128;
	static constexpr int kSparkleAroundCircumfrence = kSparkleAroundRadius * M_TAU_FIXED;
	static constexpr int kSparkleAroundCount = kSparkleAroundCircumfrence / kSparkleRadius / FRACUNIT;

	struct Orb : mobj_t
	{
		void afterimages()
		{
			mobj_t* ghost = P_SpawnGhostMobj(this);

			// Flickers every frame
			ghost->extravalue1 = 1;
			ghost->extravalue2 = 2;

			ghost->tics = 8;
		}
	};

	struct Arm : mobj_t {};

	INT32 id() const { return checkpoint_id(this); }
	INT32 linetag() const { return checkpoint_linetag(this); }

	Checkpoint* other() const { return static_cast<Checkpoint*>(checkpoint_other(this)); }
	void other(Checkpoint* n) { P_SetTarget(&checkpoint_other(this), n); }

	Orb* orb() const { return static_cast<Orb*>(checkpoint_orb(this)); }
	void orb(Orb* n) { P_SetTarget(&checkpoint_orb(this), n); }

	Arm* arm() const { return static_cast<Arm*>(checkpoint_arm(this)); }
	void arm(Arm* n) { P_SetTarget(&checkpoint_arm(this), n); }

	Checkpoint* next() const { return static_cast<Checkpoint*>(checkpoint_next(this)); }
	void next(Checkpoint* n) { P_SetTarget(&checkpoint_next(this), n); }

	fixed_t var() const { return checkpoint_var(this); }
	void var(fixed_t n) { checkpoint_var(this) = n; }

	fixed_t speed() const { return checkpoint_speed(this); }
	void speed(fixed_t n) { checkpoint_speed(this) = n; }

	fixed_t speed_multiplier() const { return checkpoint_speed_multiplier(this); }
	void speed_multiplier(fixed_t n) { checkpoint_speed_multiplier(this) = n; }

	bool reverse() const { return checkpoint_reverse(this); }
	void reverse(bool n) { checkpoint_reverse(this) = n; }

	// Valid to use as an alpha.
	bool valid() const
	{
		auto f = [](const mobj_t* th) { return !P_MobjWasRemoved(th); };
		return f(this) && f(other()) && f(orb()) && f(arm());
	}

	bool activated() const { return var(); }

	// Line between A and B things.
	LineOnDemand crossing_line() const { return LineOnDemand(x, y, other()->x, other()->y, radius); }

	// Middle between A and B.
	vector3_t center_position() const
	{
		return {x + ((other()->x - x) / 2), y + ((other()->y - y) / 2), z + ((other()->z - z) / 2)};
	}

	void gingerbread()
	{
		P_InstaScale(this, 3 * scale / 2);

		orb(new_piece<Orb>(S_CHECKPOINT_ORB_DEAD));
		orb()->whiteshadow = true;

		arm(new_piece<Arm>(S_CHECKPOINT_ARM));

		deactivate();
	}

	// will not work properly after a player enters intoa  new lap
	INT32 players_passed()
	{
		INT32 pcount = 0;
		for (INT32 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].spectator && players[i].checkpointId >= id())
				pcount++;
		}
		return pcount;
	}

	boolean top_half_has_passed()
	{
		INT32 winningpos = 1;

		INT32 nump = D_NumPlayersInRace();
		winningpos = nump / 2;
		winningpos += nump % 2;

		return players_passed() >= winningpos;
	}

	void animate()
	{
		orient();
		pull();

		if (speed())
		{
			var(var() + speed());

			if (!clip_var())
			{
				speed(speed() - FixedDiv(speed() / 50, max<fixed_t>(speed_multiplier(), 1)));
			}
		}

		if (!top_half_has_passed())
		{
			sparkle_between(0);
		}
	}

	void twirl(angle_t dir, fixed_t multiplier)
	{
		var(0);
		speed_multiplier(clamp(multiplier, kMinSpeedMultiplier, kMaxSpeedMultiplier));
		speed(FixedDiv(kBaseSpeed, speed_multiplier()));
		reverse(AngleDeltaSigned(angle_to_other(), dir) > 0);

		sparkle_between(FixedMul(80 * mapobjectscale, multiplier));
	}

	void untwirl()
	{
		speed_multiplier(kMinSpeedMultiplier);
		speed(FixedDiv(-(kBaseSpeed), speed_multiplier()));
	}

	void activate()
	{
		var(FRACUNIT - 1);
		speed(0);
		P_SetMobjState(orb(), S_CHECKPOINT_ORB_LIVE);
		orb()->shadowscale = 0;
	}

	void deactivate()
	{
		var(0);
		speed(0);
		P_SetMobjState(orb(), S_CHECKPOINT_ORB_DEAD);
		orb()->shadowscale = FRACUNIT/2;
	}

	void sparkle_around_center()
	{
		const vector3_t pos = center_position();

		fixed_t mom = 5 * scale;

		for (angle_t a = 0;;)
		{
			spawn_sparkle({pos.x + FixedMul(mom, FCOS(a)), pos.y + FixedMul(mom, FSIN(a)), pos.z}, mom, 20 * scale, a);

			angle_t turn = a + (ANGLE_MAX / kSparkleAroundCount);

			if (turn < a) // overflowed a full 360 degrees
			{
				break;
			}

			a = turn;
		}
	}

private:
	static angle_t to_angle(fixed_t f) { return FixedAngle((f & FRACMASK) * 360); }

	template <typename T>
	T* new_piece(statenum_t state)
	{
		mobj_t* x = P_SpawnMobjFromMobj(this, 0, 0, 0, MT_THOK);

		P_SetMobjState(x, state);

		return static_cast<T*>(x);
	}

	angle_t angle_to_other() const { return R_PointToAngle2(x, y, other()->x, other()->y); }
	angle_t facing_angle() const { return angle_to_other() + ANGLE_90; }

	angle_t pivot() const
	{
		fixed_t pos = FixedMul(
			FixedDiv(speed_multiplier() - kMinSpeedMultiplier, kSpeedMultiplierRange),
			kMinPivotDelay
		);

		return to_angle(FixedDiv(max(var(), pos) - pos, FRACUNIT - pos)) / 4;
	}

	void orient()
	{
		angle_t facing = facing_angle();

		if (speed() >= 0)
		{
			fixed_t range = FRACUNIT + FixedRound((speed_multiplier() - kMinSpeedMultiplier) * 6);

			angle = facing + (to_angle(FixedMul(var(), range)) * (reverse() ? -1 : 1));
		}

		arm()->angle = angle - ANGLE_90;
		arm()->rollangle = -(ANGLE_90) + pivot();

		if (arm()->eflags & MFE_VERTICALFLIP)
		{
			arm()->rollangle = InvAngle(arm()->rollangle);
		}
	}

	void pull()
	{
		fixed_t r = kOrbitDistance * scale;
		fixed_t xy = FixedMul(r, FCOS(pivot()));

		P_MoveOrigin(
			orb(),
			x + FixedMul(xy, FCOS(arm()->angle)),
			y + FixedMul(xy, FSIN(arm()->angle)),
			P_GetMobjHead(this) + (FixedMul(r, FSIN(pivot())) * P_MobjFlip(this))
		);

		if (orb()->eflags & MFE_VERTICALFLIP)
		{
			orb()->z -= orb()->height;
		}

		P_MoveOrigin(arm(), orb()->x, orb()->y, orb()->z);

		if (arm()->eflags & MFE_VERTICALFLIP)
		{
			arm()->z += orb()->height - arm()->height;
		}

		if (speed())
		{
			orb()->afterimages();
		}
	}

	void spawn_sparkle(const vector3_t& pos, fixed_t xy_momentum, fixed_t z_momentum, angle_t dir, skincolornum_t color = SKINCOLOR_ULTRAMARINE)
	{
		auto rng = [=](int units) { return P_RandomRange(PR_DECORATION, -(units) * scale, +(units) * scale); };

		// note: determinate random argument eval order
		fixed_t rand_z = rng(24);
		fixed_t rand_y = rng(12);
		fixed_t rand_x = rng(12);
		// From K_DrawDraftCombiring
		mobj_t* p = P_SpawnMobjFromMobjUnscaled(
			this,
			(pos.x - x) + rand_x,
			(pos.y - y) + rand_y,
			(pos.z - z) + rand_z,
			MT_SIGNSPARKLE
		);

		P_SetMobjState(p, static_cast<statenum_t>(S_CHECKPOINT_SPARK1 + (leveltime % 11)));

		p->colorized = true;

		if (xy_momentum)
		{
			P_Thrust(p, dir, xy_momentum);
			p->momz = P_RandomKey(PR_DECORATION, max<fixed_t>(z_momentum, 1));
			p->destscale = 0;
			p->scalespeed = p->scale / 35;
			p->color = color;
			p->fuse = 0;

			// Something lags at the start of the level. The
			// timing is inconsistent, so this value is
			// vibes-based.
			constexpr int kIntroDelay = 8;

			if (leveltime < kIntroDelay)
			{
				p->hitlag = kIntroDelay;
			}
		}
		else
		{
			p->color = color;
			p->fuse = 2;
		}
	}

	void sparkle_between(fixed_t momentum)
	{
		angle_t a = angle_to_other();

		if (a < ANGLE_180)
		{
			// Let's only do it for one of the two.
			return;
		}

		angle_t dir = a - (reverse() ? ANGLE_90 : -(ANGLE_90));

		fixed_t r = kSparkleRadius * scale;
		fixed_t ofs = (kSparkleOffset * scale) + r;
		fixed_t between = R_PointToDist2(x, y, other()->x, other()->y);

		angle_t pitch = R_PointToAngle2(0, z, between, other()->z);
		vector3_t step = {
			FixedMul(FCOS(a), FCOS(pitch)),
			FixedMul(FSIN(a), FCOS(pitch)),
			FSIN(pitch)
		};

		between = R_PointToDist2(0, z, between, other()->z);

		for (; ofs < between; ofs += 2 * r)
		{
			spawn_sparkle(
				{x + FixedMul(ofs, step.x), y + FixedMul(ofs, step.y), z + (kSparkleZ * scale) + FixedMul(ofs, step.z)},
				momentum,
				momentum / 2,
				dir,
				activated() ? SKINCOLOR_GREEN : SKINCOLOR_ULTRAMARINE
			);
		}
	}

	bool clip_var()
	{
		if (speed() > 0)
		{
			if (var() >= FRACUNIT)
			{
				activate();
				return true;
			}
		}
		else
		{
			if (var() < 0)
			{
				deactivate();
				return true;
			}
		}

		return false;
	}
};

struct CheckpointManager
{
	auto begin() { return list_.begin(); }
	auto end() { return list_.end(); }

	auto find_checkpoint(INT32 id)
	{
		auto it = std::find_if(begin(), end(), [id](Checkpoint* chk) { return chk->id() == id; });
		return it != end() ? *it : nullptr;
	}

	void remove_checkpoint(Checkpoint* end) { list_.erase(end); }

	void link_checkpoint(Checkpoint* chk)
	{
		auto id = chk->id();
		if (chk->spawnpoint && id == 0)
			{
				auto msg = fmt::format(
					"Checkpoint thing (index #{}, thing type {}) has an invalid ID! ID must not be 0.\n",
					chk->spawnpoint - mapthings,
					chk->spawnpoint->type
				);
				CONS_Alert(CONS_WARNING, "%s", msg.c_str());
				return;
			}

		if (auto other = find_checkpoint(id))
		{
			if (chk->spawnpoint && other->spawnpoint && chk->spawnpoint->angle != other->spawnpoint->angle)
			{
				auto msg = fmt::format(
					"Checkpoints things with ID {} (index #{} and #{}, thing type {}) do not have matching angles.\n",
					chk->id(),
					chk->spawnpoint - mapthings,
					other->spawnpoint - mapthings,
					chk->spawnpoint->type
				);
				CONS_Alert(CONS_WARNING, "%s", msg.c_str());
				return;
			}
			other->other(chk);
			chk->other(other);
		}
		else // Checkpoint isn't in the list, find any associated tagged lines and make the pair
		{
			if (chk->linetag())
			{
				auto lines = tagged_lines(chk->linetag());
				if (lines.empty() && gametype != GT_TUTORIAL)
				{
					CONS_Alert(CONS_WARNING, "Checkpoint thing %s, has linetag %d, but no lines found. Please ensure all checkpoints have associated lines.\n", sizeu1(chk->spawnpoint - mapthings), chk->linetag());
				}
				else
				{
					lines_.try_emplace(chk->linetag(), lines);
				}
			}
			else
			{
				if (gametype != GT_TUTORIAL)
				{
					CONS_Alert(CONS_WARNING, "Checkpoint thing %s, has no linetag. Please ensure all checkpoint things have a linetag.\n", sizeu1(chk->spawnpoint - mapthings));
				}
			}
			list_.push_front(chk);
			count_ += 1; // Mobjlist can't have a count on it, so we keep it here
		}

		chk->gingerbread();
	}

	void clear()
	{
		lines_.clear();
		list_.clear();
		count_ = 0;
	}

	auto count() { return count_; }

	const srb2::Vector<line_t*>* lines_for(const Checkpoint* chk) const
	{
		auto it = lines_.find(chk->linetag());
		return it != lines_.end() ? &it->second : nullptr;
	}

private:
	INT32 count_;
	srb2::MobjList<Checkpoint, svg_checkpoints> list_;
	srb2::HashMap<INT32, srb2::Vector<line_t*>> lines_;

	static srb2::Vector<line_t*> tagged_lines(INT32 tag)
	{
		srb2::Vector<line_t*> checklines;
		INT32 li;
		TAG_ITER_LINES(tag, li)
		{
			line_t* line = lines + li;
			checklines.push_back(line);
		}
		return checklines;
	}
};

CheckpointManager g_checkpoints;

}; // namespace

void Obj_LinkCheckpoint(mobj_t* end)
{
	g_checkpoints.link_checkpoint(static_cast<Checkpoint*>(end));
}

void Obj_UnlinkCheckpoint(mobj_t* end)
{
	auto chk = static_cast<Checkpoint*>(end);
	g_checkpoints.remove_checkpoint(chk);
	P_RemoveMobj(chk->orb());
	P_RemoveMobj(chk->arm());
}

void Obj_CheckpointThink(mobj_t* end)
{
	auto chk = static_cast<Checkpoint*>(end);

	if (!chk->valid())
	{
		return;
	}

	chk->animate();
}

void Obj_CrossCheckpoints(player_t* player, fixed_t old_x, fixed_t old_y)
{
	if (player->exiting
	|| (
		(gametyperules & GTR_CIRCUIT)
		&& (player->laps == 0)
	))
	{
		return;
	}

	LineOnDemand ray(old_x, old_y, player->mo->x, player->mo->y, player->mo->radius);

	auto it = std::find_if(
		g_checkpoints.begin(),
		g_checkpoints.end(),
		[&](Checkpoint* chk)
		{
			if (!chk->valid())
			{
				return false;
			}

			const srb2::Vector<line_t*>* lines = g_checkpoints.lines_for(chk);
			INT32 side;
			INT32 oldside;

			if (!lines || lines->empty())
			{
				LineOnDemand dyngate = chk->crossing_line();
				if (!ray.overlaps(dyngate))
					return false;

				side = P_PointOnLineSide(player->mo->x, player->mo->y, &dyngate);
				oldside = P_PointOnLineSide(old_x, old_y, &dyngate);
			}
			else
			{
				auto it = std::find_if(
					lines->begin(),
					lines->end(),
					[&](const line_t* line)
					{
						return ray.overlaps(*line);
					}
				);

				if (it == lines->end())
				{
					return false;
				}

				line_t* line = *it;
				side = P_PointOnLineSide(player->mo->x, player->mo->y, line);
				oldside = P_PointOnLineSide(old_x, old_y, line);
			}

			// Check if the bounding boxes of the two lines
			// overlap. This relies on the player movement not
			// being so large that it creates an oversized box,
			// but thankfully that doesn't seem to happen, under
			// normal circumstances.

			if (side == oldside)
			{
				// Did not cross.
				return false;

			}

			return true;
		}
	);

	if (it == g_checkpoints.end())
	{
		return;
	}

	Checkpoint* chk = *it;

	if (player->checkpointId == chk->id())
	{
		return;
	}

	if (gametyperules & GTR_CHECKPOINTS)
	{
		for (Checkpoint* chk : g_checkpoints)
		{
			if (chk->valid())
			{
				chk->untwirl();
				chk->other()->untwirl();
			}
		}
	}

	if (player->position <= 1)
	{
		angle_t direction = R_PointToAngle2(old_x, old_y, player->mo->x, player->mo->y);
		fixed_t speed_multiplier = FixedDiv(player->speed, K_GetKartSpeed(player, false, false));
		chk->twirl(direction, speed_multiplier);
		chk->other()->twirl(direction, speed_multiplier);
	}

	S_StartSound(player->mo, sfx_s3k63);

	player->checkpointId = chk->id();

	UINT16 oldexp = player->exp;

	K_CheckpointCrossAward(player);

	if (player->exp > oldexp)
	{
		UINT16 expdiff = (player->exp - oldexp);
		K_SpawnEXP(player, expdiff, chk);
		K_SpawnEXP(player, expdiff, chk->other());
	}

	K_UpdatePowerLevels(player, player->gradingpointnum, false);
}

mobj_t* Obj_FindCheckpoint(INT32 id)
{
	return g_checkpoints.find_checkpoint(id);
}

boolean Obj_GetCheckpointRespawnPosition(const mobj_t* mobj, vector3_t* return_pos)
{
	auto chk = static_cast<const Checkpoint*>(mobj);

	if (!chk->valid())
	{
		return false;
	}

	*return_pos = chk->center_position();

	return true;
}

angle_t Obj_GetCheckpointRespawnAngle(const mobj_t* mobj)
{
	auto chk = static_cast<const Checkpoint*>(mobj);

	return chk->spawnpoint ? FixedAngle(chk->spawnpoint->angle * FRACUNIT): 0u;
}

void Obj_ActivateCheckpointInstantly(mobj_t* mobj)
{
	auto chk = static_cast<Checkpoint*>(mobj);

	if (chk->valid())
	{
		chk->sparkle_around_center(); // only do it for one
		chk->activate();
		chk->other()->activate();
	}
}

// Returns a count of checkpoint gates, not objects
UINT32 Obj_GetCheckpointCount()
{
	return g_checkpoints.count();
}

void Obj_ClearCheckpoints()
{
	g_checkpoints.clear();
}

void Obj_DeactivateCheckpoints()
{
	for (Checkpoint* chk : g_checkpoints)
	{
		if (chk->valid())
		{
			chk->untwirl();
			chk->other()->untwirl();
		}
	}
}
