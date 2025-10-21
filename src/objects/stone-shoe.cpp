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
#include <cstdlib>

#include "objects.hpp"

#include "../g_game.h"
#include "../k_kart.h"
#include "../m_easing.h"
#include "../m_fixed.h"
#include "../p_spec.h"
#include "../r_main.h"
#include "../tables.h"

using namespace srb2::objects;

namespace
{

struct Player : Mobj
{
	bool valid() const { return player != nullptr; }
};

struct Shoe;

struct Chain : Mobj
{
	void hnext() = delete;
	Chain* next() const { return Mobj::hnext<Chain>(); }
	void next(Chain* n) { Mobj::hnext(n); }

	void target() = delete;
	Shoe* shoe() const { return Mobj::target<Shoe>(); }
	void shoe(Shoe* n) { Mobj::target(n); }

	bool valid() const;

	bool try_damage(Player* pmo);

	bool tick()
	{
		if (!valid())
		{
			remove();
			return false;
		}

		return true;
	}
};

struct Shoe : Mobj
{
	void target() = delete;
	Player* follow() const { return Mobj::target<Player>(); }
	void follow(Player* n) { Mobj::target(n); }

	void movedir() = delete;
	INT32 dir() const { return Mobj::movedir; }
	void dir(INT32 n) { Mobj::movedir = n; }

	void hnext() = delete;
	Chain* chain() const { return Mobj::hnext<Chain>(); }
	void chain(Chain* n) { Mobj::hnext(n); }

	void extravalue1() = delete;
	INT32 chainLength() const { return mobj_t::extravalue1; }
	void chainLength(INT32 n) { mobj_t::extravalue1 = n; }

	void extravalue2() = delete;
	INT32 owner() const { return mobj_t::extravalue2; }
	void owner(INT32 n) { mobj_t::extravalue2 = n; }

	void threshold() = delete;
	bool bouncing() const { return mobj_t::threshold; }
	void bouncing(bool n) { mobj_t::threshold = n; }

	bool valid() const { return Mobj::valid(follow()) && follow()->valid() && Mobj::valid(chain()); }

	Fixed minDist() const { return 200 * mapobjectscale; }
	Fixed maxDist() const { return 500 * mapobjectscale; }

	angle_t followAngle() const { return R_PointToAngle2(x, y, follow()->x, follow()->y); }
	Fixed followDistance() const { return FixedHypot(x - follow()->x, y - follow()->y); }

	static Vec2<Fixed> followVector(angle_t a) { return Vec2<Fixed> {FCOS(a), FSIN(a)}; }
	Vec2<Fixed> followVector() const { return followVector(followAngle()); }

	player_t* ownerPlayer() const
	{
		if (owner() < 0 || owner() >= MAXPLAYERS)
			return nullptr;
		return &players[owner()];
	}

	static Shoe* spawn
	(	INT32 owner,
		Player* victim)
	{
		Vec2<Fixed> P = followVector(victim->angle) * Fixed {-40 * mapobjectscale};
		Shoe* shoe = victim->spawn_from<Shoe>({P, 0}, MT_STONESHOE);

		shoe->follow(victim);
		shoe->owner(owner);
		shoe->dir(0);
		shoe->fuse = 15 * TICRATE;

		INT32 numLinks = 5;
		Chain* link = nullptr;

		for (INT32 i = 0; i < numLinks; ++i)
		{
			Chain* node = shoe->spawn_from<Chain>(MT_STONESHOE_CHAIN);
			node->next(link);
			node->shoe(shoe);
			link = node;
		}

		shoe->chain(link);
		shoe->chainLength(numLinks);

		shoe->voice(sfx_s3k5d);

		return shoe;
	}

	bool tick()
	{
		if (!valid())
		{
			remove();
			return false;
		}

		move();
		move_chain();

		return true;
	}

	bool try_damage
	(	Player* pmo,
		Mobj* inflictor)
	{
		if (pmo == follow())
			return false;

		if (!valid())
			return false;

		mobj_t* source = nullptr;

		if (ownerPlayer())
			source = ownerPlayer()->mo;

		bool hit = false;

		if (bouncing())
			hit = P_DamageMobj(pmo, inflictor, source, 1, DMG_TUMBLE);
		else if (FixedHypot(momx, momy) > 16 * mapobjectscale)
			hit = P_DamageMobj(pmo, inflictor, source, 1, DMG_NORMAL);

		if (hit && ownerPlayer() && follow()->player && pmo->player)
		{
			// Give Amps to both the originator of the Shoe and the person dragging it.
			K_SpawnAmps(ownerPlayer(), K_PvPAmpReward(10, ownerPlayer(), pmo->player), pmo);
			K_SpawnAmps(follow()->player, K_PvPAmpReward(10, follow()->player, pmo->player), pmo);
		}

		return true;
	}

private:
	void animate()
	{
		INT32 speed = 20; // TODO
		tic_t t = leveltime / speed;
		INT32 th = ANGLE_180 / speed * 2;
		UINT32 ff = 0;

		if (t % 8 > 3)
		{
			ff = FF_VERTICALFLIP;
			angle = ANGLE_180 + dir();
		}
		else
			angle = dir();

		frame = (t % 4) | ff;
		dir(dir() + th);
		rollangle -= th;

		old_angle = angle;
	}

	void move()
	{
		Fixed dist = followDistance();
		angle_t a = followAngle();
		bool close = true;

		Fixed dz = z - follow()->z;

		if (dz < -maxDist())
			z = follow()->z - maxDist();
		else if (dz > maxDist())
			z = follow()->z + maxDist();

		if (dist > minDist())
		{
			if (dist > maxDist())
			{
				move_origin({
					follow()->x - maxDist() * Fixed {FCOS(a)},
					follow()->y - maxDist() * Fixed {FSIN(a)},
					z,
				});

				close = false;

				if (P_IsObjectOnGround(this))
				{
					momz = flip(32 * mapobjectscale);
					bouncing(true);
					voice(sfx_s3k5f);
					P_StartQuakeFromMobj(5, 40 * scale(), 512 * scale(), this);
				}
			}

			thrust(a, 10 * mapobjectscale);

			Fixed maxSpeed = 32 * mapobjectscale;
			Fixed speed = FixedHypot(momx, momy);

			if (speed > maxSpeed)
				instathrust(a, maxSpeed);

			if (P_IsObjectOnGround(this) && leveltime % 5 == 0)
				voice(sfx_s3k6f);
		}
		else
		{
			if (!P_IsObjectOnGround(this))
				bouncing(false);
		}

		if (close)
			friction -= 200;
		else
			friction += 500;

		if (dist > maxDist())
			animate();
		else
		{
			frame = 1;
			rollangle = 0;
			angle = a;
		}

		follow()->player->stonedrag = dist > minDist();

		sprzoff(30 * scale());

		K_MatchFlipFlags(this, follow());
	}

	void move_chain()
	{
		const Fixed shoeSpriteRadius = 48 * scale();
		const Fixed chainSpriteRadius = 26 * chain()->scale();

		Fixed fd = std::max<Fixed>(followDistance() - shoeSpriteRadius - follow()->radius - chainSpriteRadius * 2, 0);
		Fixed fdz = follow()->z - z;
		Fixed nd = fd / std::max(chainLength(), 1);
		Fixed ndz = fdz / std::max(chainLength(), 1);

		Vec2<Fixed> v = followVector();
		Vec2<Fixed> p = pos2d() + v * nd / 2 + v * Fixed {shoeSpriteRadius + chainSpriteRadius};
		Fixed pz = z + ndz / 2;

		Chain* node = chain();

		while (Mobj::valid(node))
		{
			node->move_origin({p, pz});
			K_MatchFlipFlags(node, this);
			node->sprzoff(sprzoff());

			// Let chain flicker like shoe does
			node->renderflags = renderflags;

			p += v * nd;
			pz += ndz;

			node = node->next();
		}
	}
};

bool Chain::valid() const
{
	return Mobj::valid(shoe());
}

bool Chain::try_damage(Player* pmo)
{
	if (!Mobj::valid(shoe()))
		return false;

	return shoe()->try_damage(pmo, this);
}

}; // namespace

mobj_t *Obj_SpawnStoneShoe(INT32 owner, mobj_t *victim)
{
	return Shoe::spawn(owner, static_cast<Player*>(victim));
}

boolean Obj_TickStoneShoe(mobj_t *shoe)
{
	return static_cast<Shoe*>(shoe)->tick();
}

boolean Obj_TickStoneShoeChain(mobj_t *chain)
{
	return static_cast<Chain*>(chain)->tick();
}

player_t *Obj_StoneShoeOwnerPlayer(mobj_t *shoe)
{
	return static_cast<Shoe*>(shoe)->ownerPlayer();
}

void Obj_CollideStoneShoe(mobj_t *mover, mobj_t *mobj)
{
	switch (mobj->type)
	{
		case MT_STONESHOE: {
			Shoe* shoe = static_cast<Shoe*>(mobj);

			if (!shoe->try_damage(static_cast<Player*>(mover), shoe))
				if (shoe->follow() != mover)
					K_KartSolidBounce(mover, shoe);
			break;
		}

		case MT_STONESHOE_CHAIN:
			static_cast<Chain*>(mobj)->try_damage(static_cast<Player*>(mover));
			break;

		default:
			break;
	}
}
