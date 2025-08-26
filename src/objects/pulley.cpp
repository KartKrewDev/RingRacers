// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by "Lat'"
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <cmath>

#include "objects.hpp"

#include "../d_player.h"

using namespace srb2::objects;

namespace
{

struct Pulley;

struct Hook : Mobj
{
	void target() = delete;
	Mobj* player() const { return Mobj::target(); }
	void player(Mobj* n) { Mobj::target(n); }

	void tracer() = delete;
	Pulley* pulley() const;
	void pulley(Pulley* n);

	void touch(Mobj* toucher);
};

struct Pulley : Mobj
{
	// hook states to keep the code clean :)
	enum class Mode : INT32
	{
		kNull = 0, // not set
		kIdle = 1, // wait for player
		kHook = 2, // player hooked, short delay before we start pulling
		kPull = 3, // pulling the player upwards
		kDown = 4, // player has been flung, go back down
	};

	static constexpr tic_t kPullupDelay = TICRATE/4;
	static Fixed max_pullup_speed() { return 32*mapobjectscale; }

	// how far down do we extend the hook from our current
	// position?
	void cvmem() = delete;
	Fixed height() const { return mobj_t::cvmem; }
	void height(Fixed n) { mobj_t::cvmem = n; }

	// just makes it easier
	Fixed bottom() const { return z - height(); }

	void target() = delete;
	Hook* hook() const { return Mobj::target<Hook>(); }
	void hook(Hook* n) { Mobj::target(n); }

	void extravalue1() = delete;
	Mode mode() const { return static_cast<Mode>(mobj_t::extravalue1); }
	void mode(Mode n) { mobj_t::extravalue1 = static_cast<INT32>(n); }

	void thing_args() = delete;
	bool trick_bit() const { return mobj_t::thing_args[0] & 1; }

	void extravalue2() = delete;
	tic_t ticker() const { return mobj_t::extravalue2; }
	void ticker(tic_t n) { mobj_t::extravalue2 = n; }

	void tracer() = delete;
	Mobj* rope() const { return Mobj::tracer(); }
	void rope(Mobj* n) { Mobj::tracer(n); }

	void init()
	{
		if (Mobj::valid(hook()))
			hook()->remove();

		if (Mobj::valid(rope()))
			rope()->remove();

		if (!spawnpoint)
			return; // what the fuck

		height(spawnpoint->angle * FRACUNIT);

		// spawn the hook:
		if (Hook* h = spawn<Hook>({x, y, bottom()}, MT_PULLUPHOOK))
		{
			h->sprite = SPR_HCHK;
			h->frame = 0;
			h->color = SKINCOLOR_RED;

			hook(h); // don't lose track of that.
			h->pulley(this); // point to daddy

			// set idle state
			mode(Mode::kIdle);
		}

		if (Mobj* h = spawn<Mobj>({x, y, bottom()}, MT_THOK))
		{
			// jartha note: this visual has been completely replaced vs the old lua version
			h->sprite = SPR_HCCH;
			h->frame = 0;
			h->tics = -1;
			rope(h);
			animate();
			h->mobj_t::old_spriteyscale = h->mobj_t::spriteyscale;
		}
	}

	bool think()
	{
		if (mode() == Mode::kNull)
		{
			init();
			return true;
		}

		if (!Mobj::valid(hook()))
		{
			mode(Mode::kNull); // wtf! force respawn hook
			return false;
		}

		// handle functionality:
		auto mode_handler = [&]
		{
			switch (mode())
			{
			case Mode::kHook:
				return think_hook();
			case Mode::kPull:
				return think_pull();
			case Mode::kDown:
				return think_down();
			default:
				return true;
			}
		};
		if (!mode_handler())
			return false;

		// handle the hook visuals here
		animate();
		return true;
	}

private:
	bool think_player()
	{
		// Hook the player and ensure they remain in place!
		Mobj* pmo = hook()->player();
		if (!Mobj::valid(pmo) || !pmo->player)
		{
			mode(Mode::kNull); // reset hook
			return false;
		}

		pmo->flags |= MF_NOGRAVITY;
		pmo->move_origin(hook());
		// pmo->angle = angle;
		return true;
	};

	bool think_hook()
	{
		if (!think_player())
			return false;

		// wait .5 second before pulling
		ticker(ticker() + 1);
		if (ticker() > kPullupDelay)
		{
			mode(Mode::kPull);
			ticker(0); // (don't forget to reset that...)
			hook()->momz = flip(mapobjectscale/4);
		}
		return true;
	}

	bool think_pull()
	{
		if (!think_player())
			return false;

		hook()->momz = hook()->momz * 14 / 10;
		if (std::abs(hook()->momz) > max_pullup_speed())
			hook()->momz = flip(max_pullup_speed());

		// reaching the top
		if (hook()->z > z)
			apex();
		return true;
	}

	void apex()
	{
		mode(Mode::kDown);

		Mobj* pmo = hook()->player();
		P_ResetPlayer(pmo->player);

		// special flag sets trick panel state
		if (trick_bit()) // tyron 2023-10-30 spooky no look UDMF fix
		{
			K_DoPogoSpring(pmo, 32*FRACUNIT, 0);
			pmo->player->trickpanel = TRICKSTATE_READY;
			// jartha note: trickdelay does not exist, maybe it got replaced at some point?
			//pmo->player->trickdelay = 8;
		}

		pmo->momz = hook()->momz;
		pmo->player->pullup = false;
		pmo->flags &= ~MF_NOGRAVITY;

		hook()->momz = 0; // stop!
		hook()->player(nullptr); // this looks stupid, but anyway this makes the hook forget about the player
	}

	bool think_down()
	{
		// go back down slowly.
		hook()->momz = -24 * mapobjectscale;
		if (hook()->z < bottom())
		{
			// jartha note: lua discrepancy: setting z in lua does P_CheckPosition. Is it fine to skip that?
			hook()->z = bottom();
			hook()->momz = 0;

			mode(Mode::kIdle); // aaand we're ready again.
		}
		return true;
	}

	void animate()
	{
		if (!Mobj::valid(rope()) || !Mobj::valid(hook()))
			return;

		rope()->z = hook()->top();
		rope()->spriteyscale(Fixed {std::max<fixed_t>(0, z - hook()->top())} / std::max<Fixed>(1, 32 * rope()->scale()));
	}
};

Pulley* Hook::pulley() const
{
	return Mobj::tracer<Pulley>();
}

void Hook::pulley(Pulley* n)
{
	Mobj::tracer(n);
}

void Hook::touch(Mobj* toucher)
{
	if (Mobj::valid(player()))
		return; // nope
	if (!Mobj::valid(pulley()))
		return; // wtf
	if (pulley()->mode() != Pulley::Mode::kIdle)
		return; // hook is busy
	if (toucher->player->pullup)
		return; // Already hooked!

	player(toucher);
	pulley()->mode(Pulley::Mode::kHook);

	P_ResetPlayer(toucher->player); // stop everything we're doing
	toucher->player->pullup = true;
	pulley()->angle = toucher->angle;
}

}; // namespace

void Obj_PulleyThink(mobj_t *root)
{
	static_cast<Pulley*>(root)->think();
}

void Obj_PulleyHookTouch(mobj_t *special, mobj_t *toucher)
{
	static_cast<Hook*>(special)->touch(static_cast<Mobj*>(toucher));
}
