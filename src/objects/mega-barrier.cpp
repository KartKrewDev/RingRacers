// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <cstddef>

#include "../doomdef.h"
#include "../d_player.h"
#include "../g_game.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../k_powerup.h"
#include "../m_fixed.h"
#include "../p_local.h"
#include "../p_mobj.h"
#include "../r_defs.h"

#define barrier_player(o) ((o)->extravalue1)

namespace
{

struct Barrier;

// TODO: header
struct Mobj : mobj_t
{
	struct PosArg
	{
		fixed_t x, y, z;

		PosArg(fixed_t x_, fixed_t y_, fixed_t z_) : x(x_), y(y_), z(z_) {}
		PosArg(const mobj_t* mobj) : x(mobj->x), y(mobj->y), z(mobj->z) {}
	};

	static bool valid(const Mobj* mobj) { return !P_MobjWasRemoved(mobj); }

	PosArg center() const { return {x, y, z + (height / 2)}; }

	template <typename T>
	T* spawn_offset(mobjtype_t type) { return static_cast<T*>(P_SpawnMobjFromMobj(this, 0, 0, 0, type)); }

	void state(statenum_t state) { P_SetMobjState(this, state); }
	statenum_t statenum() const { return static_cast<statenum_t>(mobj_t::state - states); }

	fixed_t scale() const { return mobj_t::scale; }

	void scale(fixed_t n)
	{
		mobj_t::scale = n;
		mobj_t::destscale = n;
	}

	void move_origin(const PosArg& p) { P_MoveOrigin(this, p.x, p.y, p.z); }

	void remove() { P_RemoveMobj(this); }
};

struct Player : player_t
{
	struct Powerups : powerupvars_t
	{
		Barrier* barrier() const { return reinterpret_cast<Barrier*>(powerupvars_t::barrier); }
		void barrier(Barrier* n) { P_SetTarget(&this->powerupvars_t::barrier, reinterpret_cast<mobj_t*>(n)); }
	};

	static bool valid(std::size_t i) { return i < MAXPLAYERS && playeringame[i]; }
	static Player* at(std::size_t i) { return static_cast<Player*>(&players[i]); }

	std::size_t num() const { return this - Player::at(0); }
	Mobj* mobj() const { return static_cast<Mobj*>(mo); }

	Powerups& powerups() { return static_cast<Powerups&>(player_t::powerup); }
	const Powerups& powerups() const { return static_cast<const Powerups&>(player_t::powerup); }
};

struct Barrier : Mobj
{
	static constexpr angle_t kSpinSpeed = ANGLE_22h;
	static constexpr angle_t kSpinGap = 20*ANG1;

	static Barrier* spawn(Player* player, statenum_t state, int idx)
	{
		Barrier* child = player->mobj()->spawn_offset<Barrier>(MT_MEGABARRIER);

		child->angle = player->mobj()->angle + (idx * kSpinGap);
		child->player(player);
		child->renderflags |= RF_DONTDRAW;
		child->state(state);

		return child;
	}

	static void spawn_chain(Player* player)
	{
		player->powerups().barrier(spawn(player, S_MEGABARRIER1, 0));
		spawn(player, S_MEGABARRIER2, 1);
		spawn(player, S_MEGABARRIER2, 2);
		spawn(player, S_MEGABARRIER2, 3);
		spawn(player, S_MEGABARRIER2, 4);
		spawn(player, S_MEGABARRIER3, 5);
	}

	int playernum() const { return barrier_player(this); }
	Player* player() const { return Player::at(playernum()); }
	void player(player_t* n) { barrier_player(this) = n - players; }

	bool valid() const { return Mobj::valid(this) && Player::valid(playernum()) && Mobj::valid(player()->mobj()); }

	bool think()
	{
		if (!valid() || !K_PowerUpRemaining(player(), POWERUP_BARRIER))
		{
			remove();
			return false;
		}

		Mobj* source = player()->mobj();
		color = source->color;
		scale(8 * source->scale() / 9);
		move_origin(source->center());
		angle += kSpinSpeed;
		eflags = (eflags & ~MFE_VERTICALFLIP) | (source->eflags & MFE_VERTICALFLIP);

        if (K_PlayerGuard(player()))
		{
			renderflags &= ~RF_DONTDRAW;
			renderflags ^= RF_ADD | RF_TRANS90;
		}
		else
		{
			renderflags |= RF_DONTDRAW;
		}

		return true;
	}
};

}; // namespace

void Obj_SpawnMegaBarrier(player_t* p)
{
	Player* player = static_cast<Player*>(p);

	if (!Mobj::valid(player->powerups().barrier()))
	{
		Barrier::spawn_chain(player);
	}
}

boolean Obj_MegaBarrierThink(mobj_t* mobj)
{
	return static_cast<Barrier*>(mobj)->think();
}
