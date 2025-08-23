// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef mobj_hpp
#define mobj_hpp

#include <optional>

#include "math/fixed.hpp"
#include "math/line_segment.hpp"
#include "math/vec.hpp"

#include "doomtype.h"
#include "k_hitlag.h"
#include "k_kart.h"
#include "info.h"
#include "p_local.h"
#include "p_mobj.h"
#include "s_sound.h"
#include "sounds.h"
#include "typedef.h"

namespace srb2
{

struct Mobj : mobj_t
{
	using fixed = math::Fixed;
	using line_segment = math::LineSegment<fixed>;
	using vec2 = math::Vec2<fixed>;

	// TODO: Vec3 would be nice
	struct PosArg
	{
		fixed x, y, z;

		PosArg() : PosArg(0, 0, 0) {}
		PosArg(fixed x_, fixed y_, fixed z_) : x(x_), y(y_), z(z_) {}

		template <typename T>
		PosArg(math::Vec2<T> p, fixed z) : PosArg(p.x, p.y, z) {}

		PosArg(const mobj_t* mobj) : PosArg(mobj->x, mobj->y, mobj->z) {}
	};

	// ManagedPtr(mobj_t::target); wrapper around a reference
	// counted mobj pointer. Assigning the wrapper updates
	// the original pointer and performs reference counting.
	struct ManagedPtr
	{
		ManagedPtr(mobj_t*& ref) : ref_(ref) {}
		ManagedPtr& operator=(mobj_t* ptr)
		{
			P_SetTarget(&ref_, ptr);
			return *this;
		}
		operator mobj_t*() const { return ref_; }

	private:
		mobj_t*& ref_;
	};


	//
	// Validity checks
	//

	static bool valid(const Mobj* mobj) { return !P_MobjWasRemoved(mobj); } // safe for nullptr
	bool valid() const { return Mobj::valid(this); }

	void remove() { P_RemoveMobj(this); }


	//
	// Spawning
	//

	// Mobj_t::spawn; spawn Mobj at position. Mobj inherits map defaults (scale, gravity).
	template <typename T>
	static T* spawn(const PosArg& p, mobjtype_t type) { return static_cast<T*>(P_SpawnMobj(p.x, p.y, p.z, type)); }

	// this->spawn_from; spawn Mobj relative to parent (this->pos + p). Mobj inherits parent scale, gravity.
	template <typename T>
	T* spawn_from(const PosArg& p, mobjtype_t type)
	{
		return static_cast<T*>(P_SpawnMobjFromMobjUnscaled(this, p.x, p.y, p.z, type));
	}

	template <typename T>
	T* spawn_from(mobjtype_t type) { return spawn_from<T>({}, type); }

	// TODO: ghosts have unique properties, add Ghost class
	Mobj* spawn_ghost() { return static_cast<Mobj*>(P_SpawnGhostMobj(this)); }


	//
	// Position
	//

	PosArg center() const { return {x, y, z + (height / 2)}; }
	PosArg pos() const { return {x, y, z}; }
	vec2 pos2d() const { return {x, y}; }
	fixed top() const { return z + height; }

	bool is_flipped() const { return eflags & MFE_VERTICALFLIP; }
	fixed flip(fixed x) const { return x * P_MobjFlip(this); }

	// Collision helper
	bool z_overlaps(const Mobj* b) const { return z < b->top() && b->z < top(); }

	void move_origin(const PosArg& p) { P_MoveOrigin(this, p.x, p.y, p.z); }
	void set_origin(const PosArg& p) { P_SetOrigin(this, p.x, p.y, p.z); }
	void instathrust(angle_t angle, fixed speed) { P_InstaThrust(this, angle, speed); }
	void thrust(angle_t angle, fixed speed) { P_Thrust(this, angle, speed); }

	static void bounce(Mobj* t1, Mobj* t2) { K_KartBouncing(t1, t2); }
	void solid_bounce(Mobj* solid) { K_KartSolidBounce(this, solid); }

	// A = bottom left corner
	// this->aabb; the standard bounding box. This is inapproporiate for paper collision!
	line_segment aabb() const { return {{x - radius, y - radius}, {x + radius, y + radius}}; }


	//
	// Mobj pointers
	//

#define MOBJ_PTR_METHOD(member) \
	template <typename T = Mobj>\
	T* member() const { return static_cast<T*>(mobj_t::member); }\
	template <typename T>\
	void member(T* n) { ManagedPtr(this->mobj_t::member) = n; }

	MOBJ_PTR_METHOD(hnext)
	MOBJ_PTR_METHOD(hprev)
	MOBJ_PTR_METHOD(itnext)
	MOBJ_PTR_METHOD(target)
	MOBJ_PTR_METHOD(tracer)
	MOBJ_PTR_METHOD(punt_ref)
	MOBJ_PTR_METHOD(owner)

#undef MOBJ_PTR_METHOD


	//
	// State
	//

	struct State : state_t
	{
		statenum_t num() const { return static_cast<statenum_t>(static_cast<const state_t*>(this) - states); }
	};

	void state(statenum_t state) { (player ? P_SetPlayerMobjState : P_SetMobjState)(this, state); }
	const State* state() const { return static_cast<const State*>(mobj_t::state); }


	//
	// Scale
	//

	fixed scale() const { return mobj_t::scale; }

	void scale(fixed n)
	{
		P_SetScale(this, n);
		mobj_t::destscale = n;
	}

	void scale_to(fixed stop, std::optional<fixed> speed = {})
	{
		mobj_t::destscale = stop;

		if (speed)
		{
			mobj_t::scalespeed = *speed;
		}
	}

	void scale_between(fixed start, fixed stop, std::optional<fixed> speed = {})
	{
		P_SetScale(this, start);
		scale_to(stop, speed);
	}


	//
	// Sprite offsets
	//

#define FIXED_METHOD(member) \
	fixed member() const { return mobj_t::member; } \
	void member(fixed n) { mobj_t::member = n; }

	FIXED_METHOD(spritexscale)
	FIXED_METHOD(spriteyscale)
	FIXED_METHOD(spritexoffset)
	FIXED_METHOD(spriteyoffset)
	FIXED_METHOD(sprxoff)
	FIXED_METHOD(spryoff)
	FIXED_METHOD(sprzoff)

	FIXED_METHOD(bakexoff)
	FIXED_METHOD(bakeyoff)
	FIXED_METHOD(bakezoff)
	FIXED_METHOD(bakexpiv)
	FIXED_METHOD(bakeypiv)
	FIXED_METHOD(bakezpiv)

	vec2 spritescale() const { return {spritexscale(), spriteyscale()}; }
	void spritescale(const vec2& v)
	{
		spritexscale(v.x);
		spriteyscale(v.y);
	}

	vec2 spriteoffset() const { return {spritexoffset(), spriteyoffset()}; }
	void spriteoffset(const vec2& v)
	{
		spritexoffset(v.x);
		spriteyoffset(v.y);
	}

	vec2 sproff2d() const { return {sprxoff(), spryoff()}; }
	void sproff2d(const vec2& v)
	{
		sprxoff(v.x);
		spryoff(v.y);
	}

	void linkdraw(bool n) { flags2 = n ? flags2 | MF2_LINKDRAW : flags2 & ~MF2_LINKDRAW; }

	// WARNING: sets tracer!
	void linkdraw(Mobj* parent)
	{
		tracer(parent);
		linkdraw(true);
	}

	void linkdraw(Mobj* parent, INT32 offset)
	{
		linkdraw(parent);
		dispoffset = offset;
	}

	// TODO: Vec3


	//
	// Sound
	//

	bool voice_playing(sfxenum_t sfx) const { return S_SoundPlaying(this, sfx); }

	void voice(sfxenum_t sfx, int volume = 255) const { S_StartSoundAtVolume(this, sfx, volume); }
	void voice_reduced(sfxenum_t sfx, const player_t* player, int volume = 255) const
	{
		S_ReducedVFXSoundAtVolume(this, sfx, volume, player);
	}

	void voice_loop(sfxenum_t sfx, int volume = 255) const
	{
		if (!voice_playing(sfx))
		{
			voice(sfx, volume);
		}
	}


	//
	// Hitlag
	//

	INT32 hitlag() const { return mobj_t::hitlag; }
	void hitlag(INT32 tics, bool damage = false) { K_AddHitLag(this, tics, damage); }
	void hitlag(Mobj* inflictor, Mobj* source, INT32 tics, bool damage)
	{
		K_SetHitLagForObjects(this, inflictor, source, tics, damage);
	}
	void exact_hitlag(INT32 tics, bool damage)
	{
		mobj_t::hitlag = tics;
		mobj_t::eflags = (mobj_t::eflags & ~MFE_DAMAGEHITLAG) | (MFE_DAMAGEHITLAG * damage);
	}
};

}; // namespace srb2

#endif/*mobj_hpp*/
