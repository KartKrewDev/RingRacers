// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman.
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>

#include "../cxxutil.hpp"
#include "objects.hpp"

#include "../m_easing.h"
#include "../m_random.h"
#include "../r_skins.h"
#include "../tables.h"

using namespace srb2::objects;

namespace
{

Vec2<Fixed> angle_vector(angle_t x)
{
	return Vec2<Fixed> {FCOS(x), FSIN(x)};
}

template <typename F>
void radial_generic(int ofs, int spokes, F&& f)
{
	int ang = 360 / spokes;
	for (int i = 0; i < spokes; ++i)
	{
		f((ofs + (ang * i)) % 360);
	}
}

angle_t degr_to_angle(int degr)
{
	return FixedAngle(degr * FRACUNIT);
}

struct Particle : Mobj
{
	void extravalue1() = delete;
	UINT8 bounces() const { return mobj_t::extravalue1; }
	void bounces(UINT8 n) { mobj_t::extravalue1 = n; }

	void extravalue2() = delete;
	UINT8 counter() const { return mobj_t::extravalue2; }
	void counter(UINT8 n) { mobj_t::extravalue2 = n; }

	bool is_shrapnel() const { return sprite == SPR_KRBM; }

	static void spew(Mobj* source,int pskin)
	{
		auto generic = [&](spritenum_t sprite, int pskinn, statenum_t spr2state, int degr, Fixed scale, int momx, const Vec2<int>& momz)
		{
			Particle* x = source->spawn_from<Particle>({}, MT_KART_PARTICLE);
			if (x)
			{
				if(pskinn >= 0 && pskinn < numskins
					&& spr2state > S_NULL && spr2state < NUMSTATES &&
					states[spr2state].frame >= 0 && states[spr2state].frame < NUMPLAYERSPRITES * 2 && //'NUMPLAYERSPRITES * 2' being the length of the 'skin_t.sprites' array member
					skins[pskinn]->sprites[states[spr2state].frame].numframes > 0)
				{		

					x->skin = (void*)(skins[pskinn]);
					x->state(spr2state);
					//frame will be set by state()
				}
				else{
					//state will be set by mapthing definition
					x->sprite = sprite;
					x->frame = 0;
				}

				x->frame |=FF_SEMIBRIGHT;

				x->color = source->color;
				x->lightlevel = 112;
				x->scale(scale * x->scale());

				x->instathrust(source->angle + degr_to_angle(degr), momx * mapobjectscale);
				x->momz = P_RandomRange(PR_ITEM_DEBRIS, momz.x, momz.y) * mapobjectscale * 2;

				x->angle = P_Random(PR_ITEM_DEBRIS);
				x->rollangle = P_Random(PR_ITEM_DEBRIS);

				x->renderflags |= RF_DONTDRAW;
			}
			return x;
		};

		auto part = [&](spritenum_t sprite, int pskinn, statenum_t spr2state,  int degr, Fixed scale)
		{
			return generic(sprite, pskinn, spr2state, degr, scale, 2, {8, 16});
		};

		auto radial = [&](spritenum_t sprite, int pskinn, statenum_t spr2state, int ofs, int spokes, Fixed scale)
		{
			radial_generic(ofs, spokes, [&](int ang) { part(sprite, pskinn, spr2state, ang, scale); });
		};

		constexpr Fixed kSmall = 3*FRACUNIT/2;
		constexpr Fixed kMedium = 7*FRACUNIT/4;
		constexpr Fixed kLarge = 2*FRACUNIT;

		part(SPR_DIEE, pskin, S_KART_LEFTOVER_PARTICLE_CUSTOM_E, 0, kLarge); // steering wheel
		part(SPR_DIEK, pskin, S_KART_LEFTOVER_PARTICLE_CUSTOM_K, 180 + 45, kLarge); // engine

		part(SPR_DIEG, pskin, S_KART_LEFTOVER_PARTICLE_CUSTOM_G, 90, kLarge); // left pedal base
		part(SPR_DIED, pskin, S_KART_LEFTOVER_PARTICLE_CUSTOM_D, -90, kLarge); // right pedal base

		radial(SPR_DIEI, pskin, S_KART_LEFTOVER_PARTICLE_CUSTOM_I, 90, 2, kLarge); // wheel axle bars
		radial(SPR_DIEC, pskin, S_KART_LEFTOVER_PARTICLE_CUSTOM_C, 90, 2, kLarge); // pedal tips
		radial(SPR_DIEA, pskin, S_KART_LEFTOVER_PARTICLE_CUSTOM_A, 45, 4, kMedium); // tires
		radial(SPR_DIEH, pskin, S_KART_LEFTOVER_PARTICLE_CUSTOM_H, 45, 4, kMedium); // struts / springs
		radial(SPR_DIEB, pskin, S_KART_LEFTOVER_PARTICLE_CUSTOM_B, 360/12, 6, kSmall); // pipeframe bars
		radial(SPR_DIEJ, pskin, S_KART_LEFTOVER_PARTICLE_CUSTOM_J, 360/16, 8, kSmall); // screws

		radial_generic(0, 6, [&](int degr) { generic(SPR_KRBM, -1, S_NULL, degr, kSmall, 8, {22, 28}); }); // shrapnel

		// explosion
		radial_generic(
			45, 4,
			[&](int degr)
			{
				if (Mobj* x = source->spawn_from<Mobj>({0, 0, source->height}, MT_KART_PARTICLE))
				{
					x->flags |= MF_NOGRAVITY | MF_NOCLIPHEIGHT;
					x->height = 0;
					x->scale(2 * x->scale());
					x->angle = degr_to_angle(degr);
					x->state(S_KART_XPL01);
					x->renderflags |= RF_REDUCEVFX;
				}
			}
		);
	}

	void think()
	{
		if (!fuse && !momz)
		{
			// Getting stuck underneath a crusher... force
			// a landing so the fuse activates.
			on_land();
		}

		if (state()->num() == S_BRAKEDRIFT)
		{
			renderflags ^= RF_DONTDRAW;
			return;
		}

		// explosion
		if (sprite == SPR_DIEN)
		{
			counter(counter() + 1);
			if (counter() > 6)
			{
				renderflags ^= RF_DONTDRAW;
			}
			return;
		}

		constexpr tic_t kReappear = 16;
		if (counter() < kReappear && !is_shrapnel())
		{
			counter(counter() + 1);
			if (counter() == kReappear)
			{
				renderflags &= ~RF_DONTDRAW;
			}
		}

		angle += ANGLE_11hh;
		rollangle += ANGLE_11hh;

		if (is_shrapnel() && leveltime % 2 == 0)
		{
			if (Mobj* x = spawn_from<Mobj>({}, MT_BOOMEXPLODE))
			{
				x->color = SKINCOLOR_RUBY;
				x->scale_between(x->scale() / 2, x->scale() * 8, x->scale() / 16);
				x->state(S_SLOWBOOM2);
			}
		}

		spritescale({FRACUNIT, FRACUNIT}); // unsquish
	}

	void on_land()
	{
		if (!fuse)
		{
			fuse = (is_shrapnel() ? 70 : 90);
		}

		auto squash = [&](int tics)
		{
			hitlag(tics);
			spritescale({2*FRACUNIT, FRACUNIT/2}); // squish
		};

		switch (sprite)
		{
		case SPR_DIEB: // bar
			squash(2);
			break;

		case SPR_DIEH: // struts
			squash(4);
			break;

		case SPR_DIEI: // screws
			squash(1);
			break;

		case SPR_DIEK: // engine
			squash(5);
			break;

		default:
			break;
		}

		if (!is_shrapnel() && fuse > 7 && (bounces() & 1)) // 7 = 0.2/(1/35)
		{
			// note: determinate random argument eval order
			int32_t rand_volume = P_RandomRange(PR_ITEM_DEBRIS, 20, 40);
			int32_t rand_sound = P_RandomRange(PR_ITEM_DEBRIS, sfx_die01, sfx_die03);
			voice(
				static_cast<sfxenum_t>(rand_sound),
				rand_volume * 255 / 100
			);
		}

		bounces(bounces() + 1);
	}
};

struct Kart : Mobj
{
	static constexpr tic_t kVibrateTimer = 70;
	static constexpr UINT32 kNoClipFlags = MF_NOCLIP | MF_NOCLIPTHING;

	static tic_t burn_duration() { return (gametyperules & GTR_CLOSERPLAYERS ? 10 : 20) * TICRATE; }

	void extravalue1() = delete;
	UINT8 weight() const { return mobj_t::extravalue1; }
	void weight(UINT8 n) { mobj_t::extravalue1 = n; }

	void extravalue2() = delete;
	tic_t timer() const { return mobj_t::extravalue2; }
	void timer(tic_t n) { mobj_t::extravalue2 = n; }

	void threshold() = delete;
	tic_t cooldown() const { return mobj_t::threshold; }
	void cooldown(tic_t n) { mobj_t::threshold = n; }

	void movecount() = delete;
	tic_t burning() const { return mobj_t::movecount; }
	void burning(tic_t n) { mobj_t::movecount = n; }

	void target() = delete;
	Mobj* player() const { return Mobj::target(); }
	void player(Mobj* n) { Mobj::target(n); }

	static void spawn(Mobj* target)
	{
		SRB2_ASSERT(target->player != nullptr);

		Kart* kart = target->spawn_from<Kart>({}, MT_KART_LEFTOVER);
		if (!kart)
			return;

		kart->angle = target->angle;
		kart->color = target->color;
		P_SetObjectMomZ(kart, 20*FRACUNIT, false);
		kart->weight(target->player->kartweight);
		kart->flags |= kNoClipFlags;

		if (target->player->pflags & PF_NOCONTEST)
			target->tracer(kart);

		kart->state(S_INVISIBLE);
		kart->timer(kVibrateTimer);
		kart->exact_hitlag(15, true);
		kart->player(target);

		Obj_SpawnCustomBrolyKi(target, kart->hitlag() - 2, 32 * mapobjectscale, 0);

		target->exact_hitlag(kart->hitlag() + 1, true);
		target->frame |= FF_SEMIBRIGHT;
		target->lightlevel = 128;
	}

	void think()
	{
		if (burning() > 0)
		{
			burning(burning() - 1);
			fire();
		}

		if (cooldown() > 0)
		{
			cooldown(cooldown() - 1);
		}

		if (timer() > 0)
		{
			timer(timer() - 1);
			animate();
		}
	}

	bool destroy()
	{
		if (cooldown())
		{
			// no-op P_DamageMobj
			return true;
		}


		if (health <= 1)
		{
			return false;
		}

		Mobj* p = player();
		bool pValid = Mobj::valid(p) && p->player;
		int pSkin = pValid ? p->player->skin : -1; //rip lyman lineface :-1
		bool hasCustomHusk = pSkin >=0 && pSkin < numskins && skins[pSkin]->sprites[SPR2_DKRF].numframes;

		if(hasCustomHusk)
		{
			skin = (void*)(skins[pSkin]);
		}

		Particle::spew(this,pSkin);
		scale(3*scale()/2);

		if(hasCustomHusk){
			flags |= MF_NOSQUISH; //K_Squish() automates spritexscale/spriteyscale & this flag prevents that at the cost of no squish visual when the kart husk hits the ground
			fixed_t huskScale = FixedDiv(mapobjectscale, scale());
			spritexscale(FixedMul(spritexscale(), huskScale));
			spriteyscale(FixedMul(spriteyscale(), huskScale));
		}

		health = 1;
		state(!hasCustomHusk ? S_KART_LEFTOVER_NOTIRES : S_KART_LEFTOVER_PARTICLE_CUSTOM_F);
		cooldown(20);
		burning(burn_duration());

		if (!cv_reducevfx.value)
		{
			voice(sfx_die00);
		}

		if(pValid)
		{
			if((skins[p->player->skin]->flags & SF_BADNIK))
			{
				P_SpawnBadnikExplosion(p);
				p->spritescale({2*FRACUNIT, 2*FRACUNIT});
				p->flags |= MF_NOSQUISH;
			}

			p->state(S_KART_DEAD);
		}

		return true;
	}

private:
	void fire()
	{
		auto spread = [&](const Vec2<Fixed>& range, const Vec2<Fixed>& zrange)
		{
			angle_t ang = P_Random(PR_ITEM_DEBRIS);
			Fixed r = P_RandomRange(PR_ITEM_DEBRIS, range.x, range.y) * mapobjectscale * 4;
			Fixed z = P_RandomRange(PR_ITEM_DEBRIS, zrange.x, zrange.y) * mapobjectscale * 4;
			return spawn_from<Mobj>({angle_vector(ang) * r, z}, MT_THOK);
		};

		auto vfx = [&](fixed_t f)
		{
			if (Mobj* x = spread({16, 32}, {0, 0}))
			{
				x->state(S_KART_FIRE);
				x->lightlevel = 176;
				x->renderflags |= RF_ABSOLUTELIGHTLEVEL | RF_SEMIBRIGHT | RF_REDUCEVFX;
			}

			if (f < 3*FRACUNIT/4)
			{
				auto smoke = [&]
				{
					if (Mobj* x = spread({3, 6}, {0, 8}))
					{
						Fixed from = x->scale() / 3;
						Fixed to = 5 * x->scale() / 4;
						x->scale_between(from, to, (to - from) / 35);
						x->state(S_KART_SMOKE);
						x->lightlevel = -112;
						x->momz = 16 * mapobjectscale;
					}
				};

				smoke();
				smoke();
			}
		};

		UINT32 rf = RF_SEMIBRIGHT;

		if (burning() && P_IsObjectOnGround(this))
		{
			fixed_t f = burning() * FRACUNIT / burn_duration();

			if ((leveltime % std::max<fixed_t>(1, Easing_OutCubic(f, 8, 1))) == 0)
			{
				vfx(f);
			}

			if (f < 3*FRACUNIT/4)
			{
				auto spark = [&](int degr)
				{
					angle_t ang = angle + degr_to_angle(degr);
					if (Particle* x = spawn_from<Particle>({angle_vector(ang) * Fixed {radius}, 0}, MT_KART_PARTICLE))
					{
						x->state(S_BRAKEDRIFT);
						x->fuse = 12;
						x->color = SKINCOLOR_PASTEL;
						x->angle = ang - ANGLE_90;
						x->scale(2 * x->scale() / 5);
						x->flags |= MF_NOGRAVITY | MF_NOCLIPHEIGHT;
						x->renderflags |= RF_ADD;
					}
				};

				if (leveltime % 16 == 0)
				{
					radial_generic(45, 4, spark);
				}
			}

			if (leveltime & 1)
			{
				rf = RF_FULLBRIGHT;
			}

			voice_loop(sfx_kc51);
		}

		renderflags = (renderflags & ~RF_BRIGHTMASK) | rf;
	}

	void animate()
	{
		Mobj* p = player();

		if (!Mobj::valid(p))
		{
			return;
		}

		if (timer())
		{
			// Vibration on the death sprite eases downward
			p->exact_hitlag(Easing_InCubic(timer() * FRACUNIT / kVibrateTimer, 2, 90), true);
		}
		else
		{
			flags &= ~kNoClipFlags;
			P_PlayDeathSound(p);
		}

		// First tick after hitlag: destroyed kart appears! State will change away from S_INVISIBLE inside destroy() where S_INVISIBLE was set in static spawn()
		if (state()->num() == S_INVISIBLE)
		{
			destroy();
		}
	}

	static void P_SpawnBadnikExplosion(mobj_t *target)
	{
		UINT8 count = 24;
		angle_t ang = 0;
		angle_t step = ANGLE_MAX / count;
		fixed_t spd = 8 * mapobjectscale;
		for (UINT8 i = 0; i < count; ++i)
		{
			fixed_t rand_x;
			fixed_t rand_y;
			fixed_t rand_z;

			// note: determinate random argument eval order
			rand_z = P_RandomRange(PR_EXPLOSION, -48, 48);
			rand_y = P_RandomRange(PR_EXPLOSION, -48, 48);
			rand_x = P_RandomRange(PR_EXPLOSION, -48, 48);
			mobj_t *x = P_SpawnMobjFromMobjUnscaled(
				target,
				rand_x * target->scale,
				rand_y * target->scale,
				rand_z * target->scale,
				MT_THOK
			);
			x->hitlag = 0;
			P_InstaScale(x, 3 * x->scale / 2);
			P_InstaThrust(x, ang, spd);
			x->momz = P_RandomRange(PR_EXPLOSION, -4, 4) * mapobjectscale;
			P_SetMobjStateNF(x, S_BADNIK_EXPLOSION1);
			ang += step;
		}
		// burst effects (copied from MT_ITEMCAPSULE)
		ang = FixedAngle(360*P_RandomFixed(PR_ITEM_DEBRIS));
		for (UINT8 i = 0; i < 2; i++)
		{
			mobj_t *blast = P_SpawnMobjFromMobj(target, 0, 0, target->info->height >> 1, MT_BATTLEBUMPER_BLAST);
			blast->hitlag = 0;
			blast->angle = ang + i*ANGLE_90;
			P_SetScale(blast, 2*blast->scale/3);
			blast->destscale = 6*blast->scale;
			blast->scalespeed = (blast->destscale - blast->scale) / 30;
			P_SetMobjStateNF(blast, static_cast<statenum_t>(S_BADNIK_EXPLOSION_SHOCKWAVE1 + i));
		}
	}
};

}; // namespace

void Obj_SpawnDestroyedKart(mobj_t *player)
{
	Kart::spawn(static_cast<Mobj*>(player));
}

void Obj_DestroyedKartThink(mobj_t *kart)
{
	static_cast<Kart*>(kart)->think();
}

boolean Obj_DestroyKart(mobj_t *kart)
{
	return static_cast<Kart*>(kart)->destroy();
}

void Obj_DestroyedKartParticleThink(mobj_t *part)
{
	static_cast<Particle*>(part)->think();
}

void Obj_DestroyedKartParticleLanding(mobj_t *part)
{
	static_cast<Particle*>(part)->on_land();
}
