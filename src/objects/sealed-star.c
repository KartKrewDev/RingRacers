// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \brief Sealed Star objects

#include <string.h>

#include "../info.h"
#include "../doomdef.h"
#include "../g_game.h"
#include "../p_local.h"
#include "../m_fixed.h"
#include "../m_random.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../r_main.h"
#include "../s_sound.h"

static const INT32 kMinTranslucency = 0;
static const INT32 kMaxTranslucency = 10;

void Obj_SSCandleMobjThink(mobj_t* mo)
{
	mo->extravalue1 = (mo->extravalue1 + 3) % 360;
	mo->z += FSIN(mo->extravalue1 * ANG1) / 4;
	if (mo->tracer)
	{
		mo->tracer->z = mo->z + (256*FRACUNIT);
	}
}

void Obj_SSHologramRotatorMapThingSpawn(mobj_t* mo, mapthing_t* mt)
{
	INT32 numStates = 0;
	statenum_t mystates[32] = {0};
	char stringarg[256];
	char *token;
	int i;
	INT32 speed = 8;
	INT32 radius = 256;
	INT32 amplitude = mt->thing_args[2];
	INT32 frequency = mt->thing_args[3];
	angle_t angDiff;
	mobj_t* part;
	fixed_t circumference;

	// This is probably dangerously unsafe but what code written in C isn't
	// Christ, parsing strings in C is heinous
	// strtok is a nightmare so let's try to avoid that

	if (mt->thing_stringargs[0] == NULL)
	{
		CONS_Alert(CONS_WARNING, "MT_HOLOGRAM_ROTATOR %d: stringarg 0 is NULL\n", mt->tid);
		return;
	}

	memset(stringarg, 0, sizeof(stringarg));
	strlcpy(stringarg, mt->thing_stringargs[0], sizeof(stringarg));
	for (i = 0; i < (int)(sizeof(stringarg)); i++)
	{
		if (stringarg[i] == ' ' || stringarg[i] == ',')
		{
			stringarg[i] = 0;
		}
	}
	stringarg[sizeof(stringarg) - 1] = 0;

	for (token = stringarg; token < stringarg + sizeof(stringarg) && numStates < 32; )
	{
		char *next = token + strlen(token) + 1;

		if (stricmp(token, "bird") == 0)
		{
			mystates[numStates] = S_HOLOGRAM_BIRD;
			numStates += 1;
		}
		else if (stricmp(token, "fish") == 0)
		{
			mystates[numStates] = S_HOLOGRAM_FISH;
			numStates += 1;
		}
		else if (stricmp(token, "crab") == 0)
		{
			mystates[numStates] = S_HOLOGRAM_CRAB;
			numStates += 1;
		}
		else if (stricmp(token, "squid") == 0)
		{
			mystates[numStates] = S_HOLOGRAM_SQUID;
			numStates += 1;
		}

		token = next;
	}

	if (numStates == 0)
	{
		CONS_Alert(CONS_WARNING, "MT_HOLOGRAM_ROTATOR %d: stringarg 0 consists exclusively of unrecognised creatures\n", mt->tid);
		return;
	}

	if (mt->thing_args[0])
	{
		speed = mt->thing_args[0];
	}
	if (mt->thing_args[1])
	{
		radius = mt->thing_args[1];
	}

	// adjust for quarter-scale default
	radius *= 4;
	amplitude *= 2;

	angDiff = FixedAngle(360 * FRACUNIT/numStates);
	part = mo;
	circumference = 2 * M_PI_FIXED * radius;

	mo->cusval = abs(speed * mo->scale);
	mo->movedir = FixedAngle(FixedDiv(360 * speed * FRACUNIT, circumference));
	mo->movefactor = radius * mo->scale;

	if (amplitude > 0 && frequency > 0)
	{
		mo->extravalue1 = amplitude * mo->scale;
		mo->extravalue2 = frequency * ANG1 / 2;
	}
	else
	{
		mo->extravalue1 = 0;
		mo->extravalue2 = 0;
	}

	for (i = 0; i < numStates; i++)
	{
		statenum_t thisstate = mystates[i];
		P_SetTarget(&part->hnext, P_SpawnMobjFromMobj(mo, 0, 0, 0, MT_SS_HOLOGRAM));
		P_SetTarget(&part->hnext->hprev, part);
		part = part->hnext;
		P_SetTarget(&part->target, mo);
		P_SetMobjState(part, thisstate);
		part->angle = angDiff * i;
		if (speed < 0)
		{
			part->renderflags ^= RF_HORIZONTALFLIP;
		}
	}
}

void Obj_SSHologramRotatorMobjThink(mobj_t* mo)
{
	mobj_t* part = mo;

	while (part->hnext)
	{
		fixed_t oldZ, z;

		part = part->hnext;
		oldZ = part->z;

		if (mo->extravalue1)
		{
			z = mo->z + mo->extravalue1 + FixedMul(mo->extravalue1, FSIN(mo->extravalue2 * (part->extravalue2 + leveltime)));
		}
		else
		{
			z = oldZ;
		}

		part->angle += mo->movedir;
		P_MoveOrigin(
			part,
			mo->x + FixedMul(mo->movefactor, FCOS(part->angle - ANGLE_90)),
			mo->y + FixedMul(mo->movefactor, FSIN(part->angle - ANGLE_90)),
			z
		);

		part->rollangle = R_PointToAngle2(0, 0, mo->cusval, z - oldZ);
	}
}

void Obj_SSHologramMobjSpawn(mobj_t* mo)
{
	mo->fuse = mo->reactiontime;
	mo->threshold = -1 + P_RandomKey(PR_UNDEFINED, 2) * 2;
	mo->extravalue2 = P_RandomFixed(PR_UNDEFINED);
}

void Obj_SSHologramMapThingSpawn(mobj_t* mo, mapthing_t* mt)
{
	char* stringarg0 = mt->thing_stringargs[0];

	if (stringarg0 == NULL)
	{
		P_SetMobjState(mo, S_HOLOGRAM_CRAB);
	}
	else if (stricmp(stringarg0, "bird"))
	{
		P_SetMobjState(mo, S_HOLOGRAM_BIRD);
	}
	else if (stricmp(stringarg0, "crab") == 0)
	{
		P_SetMobjState(mo, S_HOLOGRAM_CRAB);
	}
	else if (stricmp(stringarg0, "fish") == 0)
	{
		P_SetMobjState(mo, S_HOLOGRAM_FISH);
	}
	else if (stricmp(stringarg0, "squid") == 0)
	{
		P_SetMobjState(mo, S_HOLOGRAM_SQUID);
	}

	if (mt->thing_args[0])
	{
		mo->renderflags ^= RF_HORIZONTALFLIP;
	}
}

void Obj_SSHologramMobjFuse(mobj_t* mo)
{
	INT32 trans = (mo->frame & FF_TRANSMASK) >> FF_TRANSSHIFT;

	if (trans >= kMaxTranslucency)
	{
		mo->threshold = -1;
	}
	else if (trans <= kMinTranslucency)
	{
		mo->threshold = 1;
	}

	trans += mo->threshold;
	mo->frame = (mo->frame & ~FF_TRANSMASK) | (trans << FF_TRANSSHIFT);

	if (trans >= kMaxTranslucency)
	{
		mo->fuse = 24;
	}
	else
	{
		mo->fuse = mo->reactiontime;
	}
}

static struct {
	INT32 t;
	INT32 x;
	INT32 y;
	INT32 z;
	INT32 w;
	INT32 xx;
	INT32 yy;
	INT32 zz;
	INT32 ww;
} srng_state = {0, 5197528, 3154710, 9406548, 1028369, 0, 0, 0, 0};

static void set_srng_seed(INT32 seed)
{
	srng_state.t = seed;
	srng_state.xx = srng_state.x;
	srng_state.yy = srng_state.y;
	srng_state.zz = srng_state.z;
	srng_state.ww = srng_state.w;
}

static INT32 srng_random(void)
{
	srng_state.xx = srng_state.yy;
	srng_state.yy = srng_state.zz;
	srng_state.zz = srng_state.ww;
	srng_state.ww = srng_state.ww ^ (srng_state.ww >> 19) ^ srng_state.t ^ (srng_state.t >> 8);
	srng_state.t = srng_state.xx ^ (srng_state.xx << 11);
	return srng_state.ww;
}

static INT32 srng_RandomRange(INT32 a, INT32 b)
{
	return a + abs(srng_random() % (b - a + 1));
}

static fixed_t srng_RandomFixed(void)
{
	return (fixed_t)(srng_RandomRange(0, FRACUNIT));
}

static boolean srng_RandomChance(INT32 chance)
{
	return srng_RandomFixed() < chance;
}

// spinning-related values
#define COIN_FRAMES (4) // number of frames the coin has
#define MIN_SPIN_TICS (3) // fastest speed with which a coin can animate
#define MAX_SPIN_TICS (10) // slowest speed with which a coin can animate

// spawning-related values
#define SPAWNS_PER_MAPTHINGS (10) // number of coins to spawn per spawner
#define COIN_SCALE (2*FRACUNIT) // upscales coins by this factor
#define SPAWN_RANGE (256*FRACUNIT*4) // cubed range over which coins should spawn
#define MIN_SPACING (64*FRACUNIT*4) // minimum spherical space between each coin
// IMPORTANT not to make the spacing too large relative to the spawn range, or spawn attempts could potentially escalate and impact load times or even cause a softlock

// eidolon hardcoder note: this should probably have been implemented
// with a statistical distribution instead of a linear search,
// but this was hardcoded to copy the exact behavior of the Lua

static boolean cloud_CheckDistribution(mobj_t **coins, INT32 coin_count, fixed_t x, fixed_t y, fixed_t z, fixed_t scale)
{
	fixed_t spacing = FixedMul(MIN_SPACING, scale);
	int i;
	x = FixedMul(x, scale);
	y = FixedMul(y, scale);
	z = FixedMul(z, scale);

	for (i = 0; i < coin_count; i++)
	{
		mobj_t *coin = coins[i];
		if (FixedHypot(FixedHypot(coin->x - x, coin->y - y), coin->z - z) < spacing)
		{
			return false;
		}
	}

	return true;
}

static void CoinCloudSpawn(mobj_t *mo, INT32 seed, mobjtype_t ty)
{
	fixed_t x, y, z;
	mobj_t *coins[SPAWNS_PER_MAPTHINGS];
	mobj_t *coin;
	int i;

	set_srng_seed(seed);
	for (i = 0; i < SPAWNS_PER_MAPTHINGS; i++)
	{
		// as mentioned... this is a really goofy way to implement a statistical distribution
		do {
			x = FixedMul(SPAWN_RANGE, srng_RandomFixed()) - (SPAWN_RANGE >> 1);
			y = FixedMul(SPAWN_RANGE, srng_RandomFixed()) - (SPAWN_RANGE >> 1);
			z = FixedMul(SPAWN_RANGE, srng_RandomFixed());
		} while (!cloud_CheckDistribution(coins, i, x, y, z, mo->scale));

		coin = P_SpawnMobjFromMobj(mo, x, y, z, ty);
		coin->scale = coin->destscale = FixedMul(coin->scale, COIN_SCALE);

		// initial angle & frame
		coin->angle = FixedAngle(360 * srng_RandomFixed());
		coin->frame = (coin->frame & ~FF_FRAMEMASK) | srng_RandomRange(0, COIN_FRAMES - 1);

		// random animation speed
		coin->extravalue1 = srng_RandomRange(MIN_SPIN_TICS, MAX_SPIN_TICS);
		coin->extravalue2 = coin->extravalue1;

		coins[i] = coin;
	}
}

void Obj_SSCoinCloudMapThingSpawn(mobj_t* mo, mapthing_t* mt)
{
	CoinCloudSpawn(mo, mt->thing_args[0], MT_SS_COIN);
}

void Obj_SSCoinMobjThink(mobj_t* mo)
{
	mo->extravalue2 -= 1;
	if (mo->extravalue2 <= 0)
	{
		mo->extravalue2 = mo->extravalue1;
		mo->frame = (mo->frame & ~FF_FRAMEMASK) | ((mo->frame + 1) % COIN_FRAMES);
	}
}

// spinning-related values
#define GOBLET_FRAMES (8) // number of frames the goblet has
#define MIN_VERTICAL_SPIN_TICS (3) // fastest speed with which a goblet can animate (frame)
#define MAX_VERTICAL_SPIN_TICS (10) // slowest speed with which a goblet can animate (frame)
#define MIN_HORIZONTAL_SPIN_SPEED (ANG1 * 1) // slowest speed with which a goblet can rotate (angle)
#define MAX_HORIZONTAL_SPIN_SPEED (ANG1 * 3) // fastest speed with which a goblet can rotate (angle)

// spawning-related values
#define GOBLET_SPAWNS_PER_MAPTHING (5) // number of goblets to spawn per spawner
#define GOBLET_SCALE (2*FRACUNIT) // upscales goblets by this factor
#define GOBLET_SPAWN_RANGE (256*FRACUNIT*4) // cubed range over which goblets should spawn
#define GOBLET_MIN_SPACING (64*FRACUNIT*4) // minimum spherical space between each goblet
// IMPORTANT not to make the spacing too large relative to the spawn range, or spawn attempts could potentially escalate and impact load times or even cause a softlock

#define GOBLET_SEED_OFFSET (1267491679)				// I'm setting a garbage seed offset here so that seed 0 clouds are markedly different to their coin counterparts

static void GobletCloudSpawn(mobj_t *mo, INT32 seed, mobjtype_t ty)
{
	fixed_t x, y, z;
	mobj_t *coins[GOBLET_SPAWNS_PER_MAPTHING];
	mobj_t *coin;
	int i;

	set_srng_seed(seed + GOBLET_SEED_OFFSET);
	for (i = 0; i < GOBLET_SPAWNS_PER_MAPTHING; i++)
	{
		// as mentioned... this is a really goofy way to implement a statistical distribution
		do {
			x = FixedMul(GOBLET_SPAWN_RANGE, srng_RandomFixed()) - (GOBLET_SPAWN_RANGE >> 1);
			y = FixedMul(GOBLET_SPAWN_RANGE, srng_RandomFixed()) - (GOBLET_SPAWN_RANGE >> 1);
			z = FixedMul(GOBLET_SPAWN_RANGE, srng_RandomFixed());
		} while (!cloud_CheckDistribution(coins, i, x, y, z, mo->scale));

		coin = P_SpawnMobjFromMobj(mo, x, y, z, ty);
		coin->scale = coin->destscale = FixedMul(coin->scale, GOBLET_SCALE);

		// initial angle & frame
		coin->angle = FixedAngle(360 * srng_RandomFixed());
		coin->frame = (coin->frame & ~FF_FRAMEMASK) | srng_RandomRange(0, GOBLET_FRAMES - 1);

		// random animation speed
		coin->extravalue1 = srng_RandomRange(MIN_VERTICAL_SPIN_TICS, MAX_VERTICAL_SPIN_TICS);
		coin->extravalue2 = coin->extravalue1;

		// random angle speed
		coin->movedir = MIN_HORIZONTAL_SPIN_SPEED + FixedMul(srng_RandomFixed(), MAX_HORIZONTAL_SPIN_SPEED - MIN_HORIZONTAL_SPIN_SPEED);
		if (srng_RandomChance(FRACUNIT/2))
		{
			coin->movedir = -coin->movedir;
		}

		coins[i] = coin;
	}
}

void Obj_SSGobletCloudMapThingSpawn(mobj_t* mo, mapthing_t* mt)
{
	GobletCloudSpawn(mo, mt->thing_args[0], MT_SS_GOBLET);
}

void Obj_SSGobletMobjThink(mobj_t* mo)
{
	mo->angle += mo->movedir;
	mo->extravalue2 -= 1;
	if (mo->extravalue2 <= 0)
	{
		mo->extravalue2 = mo->extravalue1;
		mo->frame = (mo->frame & ~FF_FRAMEMASK) | ((mo->frame + 1) % GOBLET_FRAMES);
	}
}

#define LAMP_SCALE (4 * FRACUNIT)
#define BULB_HORIZONTAL_OFFSET (124 * FRACUNIT)
#define BULB_VERTICAL_OFFSET (243 * FRACUNIT)

void Obj_SSLampMapThingSpawn(mobj_t* mo, mapthing_t* mt)
{
	int i;
	angle_t angle = FixedAngle(mt->angle * FRACUNIT);
	fixed_t bulbX = FixedMul(BULB_HORIZONTAL_OFFSET, FCOS(angle));
	fixed_t bulbY = FixedMul(BULB_HORIZONTAL_OFFSET, FSIN(angle));
	mobj_t *core, *part;

	mo->scale = mo->destscale = FixedMul(mo->scale, LAMP_SCALE);

	// INVISIBLE (BUT RENDERERED) CORE
	// we need this because linkdrawing to a papersprite results in all linked mobjs becoming invisible when the papersprite is viewed parallel to its angle
	core = P_SpawnMobjFromMobj(mo, bulbX, bulbY, BULB_VERTICAL_OFFSET, MT_SS_LAMP_BULB);
	P_SetTarget(&core->tracer, mo);
	core->frame = 5; // no flags needed, just a plain invisible sprite

	// parallel bulb
	part = P_SpawnMobjFromMobj(core, 0, 0, 0, MT_SS_LAMP_BULB);
	part->angle = angle;
	part->flags2 |= MF2_LINKDRAW;
	P_SetTarget(&part->tracer, core);

	// perpendicular bulb (as 2 minecraft cross parts)
	angle += ANGLE_90;
	bulbX = FixedMul(core->info->radius, FCOS(angle));
	bulbY = FixedMul(core->info->radius, FSIN(angle));
	part = core;
	for (i = 0; i < 2; i++)
	{
		P_SetTarget(&part->hnext, P_SpawnMobjFromMobj(core, bulbX, bulbY, 0, MT_SS_LAMP_BULB));
		P_SetTarget(&part->hnext->hprev, part);
		part = part->hnext;
		part->angle = angle;
		part->frame = (part->frame & ~FF_FRAMEMASK) | 2;

		part->flags2 |= MF2_LINKDRAW;
		P_SetTarget(&part->tracer, core);
		part->dispoffset = 1;
		angle += ANGLE_180;
		bulbX = -bulbX;
		bulbY = -bulbY;
	}

	// aura
	part = P_SpawnMobjFromMobj(core, 0, 0, 0, MT_SS_LAMP_BULB);
	P_SetMobjState(part, S_SS_LAMP_AURA);
	part->dispoffset -= 1;
}

void Obj_SSWindowMapThingSpawn(mobj_t* mo, mapthing_t* mt)
{
	angle_t angle;
	fixed_t co, si, x, y, xofs, yofs;
	mobj_t *shinel, *shinem, *shiner;

	mo->scale = mo->destscale = FRACUNIT;
	angle = FixedAngle(mt->angle * FRACUNIT);

	co = FCOS(angle);
	si = FSIN(angle);

	x = 192 * co;
	y = 192 * si;

	xofs = 128 * si;
	yofs = -128 * co;

	shinel = P_SpawnMobjFromMobj(mo, x + xofs, y + yofs, 0, MT_SSWINDOW_SHINE);
	shinem = P_SpawnMobjFromMobj(mo, x, y, 64*FRACUNIT, MT_SSWINDOW_SHINE);
	shiner = P_SpawnMobjFromMobj(mo, x - xofs, y - yofs, 0, MT_SSWINDOW_SHINE);

	shinel->angle = angle;
	shinem->angle = angle;
	shiner->angle = angle;

	shinel->scale = mo->destscale = FRACUNIT;
	shinem->scale = mo->destscale = FRACUNIT;
	shiner->scale = mo->destscale = FRACUNIT;
}

void Obj_SLSTMaceMobjThink(mobj_t* mo)
{
	if (leveltime % 2 == 0)
	{
		var1 = 9;
		var2 = 0;
		A_GhostMe(mo);
	}
}

void Obj_SSBumperMobjSpawn(mobj_t* mo)
{
	mo->shadowscale = FRACUNIT;
	mo->destscale <<= 1;
	P_SetScale(mo, mo->destscale);
}

void Obj_SSChainMobjThink(mobj_t* mo)
{
	mo->angle += ANGLE_22h;

	if (mo->momx == 0 && P_IsObjectOnGround(mo))
	{
		P_RemoveMobj(mo);
	}
}

void Obj_SSGachaTargetMobjSpawn(mobj_t* mo)
{
	P_InstaScale(mo, 4*mo->scale);
}

void Obj_SSCabotronMobjSpawn(mobj_t* mo)
{
	int i;
	mobj_t *ball;

	for (i = 0; i < 4; i++)
	{
		ball = P_SpawnMobj(mo->x, mo->y, mo->z, MT_CABOTRONSTAR);
		ball->destscale = mo->scale;
		P_SetScale(ball, mo->scale);
		P_SetTarget(&ball->target, mo);

		ball->movedir = FixedAngle(FixedMul(FixedDiv(i<<FRACBITS, 5<<FRACBITS), 360<<FRACBITS));
		ball->threshold = ball->radius + mo->radius + FixedMul(11*FRACUNIT, ball->scale);
	}

	mo->scale *= 2;
	mo->destscale = mo->scale;
}

void Obj_SSCabotronMobjThink(mobj_t* mo)
{
	angle_t xdir = FSIN(mo->angle - ANGLE_90);
	angle_t ydir = FCOS(mo->angle + ANGLE_90);
	boolean didMove;

	didMove = P_TryMove(mo, mo->x - (xdir * mo->info->speed), mo->y - (ydir * mo->info->speed), false, NULL);
	if (!didMove)
	{
		// --print("Flipping sawblade")
		mo->angle += ANGLE_180;
	}
}

void Obj_SSCabotronStarMobjThink(mobj_t* mo)
{
	if (mo->target)
	{
		// "but ang just call the action in the soc :))))"
		var1 = 0;
		var2 = 0;
		A_UnidusBall(mo);
		// how about no because for some reason it will call the func ONCE and never again in its pitiful aimless life

		// and let's just hack FF_ANIMATE too shall we; since var1 is being used to make sure the fucking balls don't fly off the handle which means frame anims can't be handled by the soc
		// this game is popsicle sticks and i am out of glue.
	}
}
