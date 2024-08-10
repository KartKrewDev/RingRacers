// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2024 by Kart Krew
// Copyright (C) 2021 by ZDoom + GZDoom teams, and contributors
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_terrain.c
/// \brief Implementation of TERRAIN lump from GZDoom codebase for DRRR.

#include "k_terrain.h"

#include "dehacked.h" // get_number
#include "deh_soc.h" // get_mobjtype
#include "doomdata.h"
#include "doomdef.h"
#include "doomtype.h"
#include "fastcmp.h"
#include "m_fixed.h"
#include "m_random.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_textures.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

#include "k_kart.h" // on the chopping block...

static t_splash_t *splashDefs = NULL;
static size_t numSplashDefs = 0;

static t_footstep_t *footstepDefs = NULL;
static size_t numFootstepDefs = 0;

static t_overlay_t *overlayDefs = NULL;
static size_t numOverlayDefs = 0;

static terrain_t *terrainDefs = NULL;
static size_t numTerrainDefs = 0;

static t_floor_t *terrainFloorDefs = NULL;
static size_t numTerrainFloorDefs = 0;

static size_t defaultTerrain = SIZE_MAX;
static size_t defaultOffroadFootstep = SIZE_MAX;

/*--------------------------------------------------
	size_t K_GetSplashHeapIndex(t_splash_t *splash)

		See header file for description.
--------------------------------------------------*/
size_t K_GetSplashHeapIndex(t_splash_t *splash)
{
	if (splash == NULL)
	{
		return SIZE_MAX;
	}

	return (splash - splashDefs);
}

/*--------------------------------------------------
	size_t K_GetNumSplashDefs(void)

		See header file for description.
--------------------------------------------------*/
size_t K_GetNumSplashDefs(void)
{
	return numSplashDefs;
}

/*--------------------------------------------------
	t_splash_t *K_GetSplashByIndex(size_t checkIndex)

		See header file for description.
--------------------------------------------------*/
t_splash_t *K_GetSplashByIndex(size_t checkIndex)
{
	if (checkIndex >= numSplashDefs)
	{
		return NULL;
	}

	return &splashDefs[checkIndex];
}

/*--------------------------------------------------
	t_splash_t *K_GetSplashByName(const char *checkName)

		See header file for description.
--------------------------------------------------*/
t_splash_t *K_GetSplashByName(const char *checkName)
{
	UINT32 checkHash = quickncasehash(checkName, TERRAIN_NAME_LEN);
	size_t i;

	if (numSplashDefs == 0)
	{
		return NULL;
	}

	for (i = 0; i < numSplashDefs; i++)
	{
		t_splash_t *s = &splashDefs[i];

		if (checkHash == s->hash && !strncmp(checkName, s->name, TERRAIN_NAME_LEN))
		{
			// Name matches.
			return s;
		}
	}

	return NULL;
}

/*--------------------------------------------------
	size_t K_GetFootstepHeapIndex(t_footstep_t *footstep)

		See header file for description.
--------------------------------------------------*/
size_t K_GetFootstepHeapIndex(t_footstep_t *footstep)
{
	if (footstep == NULL)
	{
		return SIZE_MAX;
	}

	return (footstep - footstepDefs);
}

/*--------------------------------------------------
	size_t K_GetNumFootstepDefs(void)

		See header file for description.
--------------------------------------------------*/
size_t K_GetNumFootstepDefs(void)
{
	return numFootstepDefs;
}

/*--------------------------------------------------
	t_footstep_t *K_GetFootstepByIndex(size_t checkIndex)

		See header file for description.
--------------------------------------------------*/
t_footstep_t *K_GetFootstepByIndex(size_t checkIndex)
{
	if (checkIndex >= numFootstepDefs)
	{
		return NULL;
	}

	return &footstepDefs[checkIndex];
}

/*--------------------------------------------------
	t_footstep_t *K_GetFootstepByName(const char *checkName)

		See header file for description.
--------------------------------------------------*/
t_footstep_t *K_GetFootstepByName(const char *checkName)
{
	UINT32 checkHash = quickncasehash(checkName, TERRAIN_NAME_LEN);
	size_t i;

	if (numFootstepDefs == 0)
	{
		return NULL;
	}

	for (i = 0; i < numFootstepDefs; i++)
	{
		t_footstep_t *fs = &footstepDefs[i];

		if (checkHash == fs->hash && !strncmp(checkName, fs->name, TERRAIN_NAME_LEN))
		{
			// Name matches.
			return fs;
		}
	}

	return NULL;
}

/*--------------------------------------------------
	size_t K_GetOverlayHeapIndex(t_overlay_t *overlay)

		See header file for description.
--------------------------------------------------*/
size_t K_GetOverlayHeapIndex(t_overlay_t *overlay)
{
	if (overlay == NULL)
	{
		return SIZE_MAX;
	}

	return (overlay - overlayDefs);
}

/*--------------------------------------------------
	size_t K_GetNumOverlayDefs(void)

		See header file for description.
--------------------------------------------------*/
size_t K_GetNumOverlayDefs(void)
{
	return numOverlayDefs;
}

/*--------------------------------------------------
	t_overlay_t *K_GetOverlayByIndex(size_t checkIndex)

		See header file for description.
--------------------------------------------------*/
t_overlay_t *K_GetOverlayByIndex(size_t checkIndex)
{
	if (checkIndex >= numOverlayDefs)
	{
		return NULL;
	}

	return &overlayDefs[checkIndex];
}

/*--------------------------------------------------
	t_overlay_t *K_GetOverlayByName(const char *checkName)

		See header file for description.
--------------------------------------------------*/
t_overlay_t *K_GetOverlayByName(const char *checkName)
{
	UINT32 checkHash = quickncasehash(checkName, TERRAIN_NAME_LEN);
	size_t i;

	if (numOverlayDefs == 0)
	{
		return NULL;
	}

	for (i = 0; i < numOverlayDefs; i++)
	{
		t_overlay_t *o = &overlayDefs[i];

		if (checkHash == o->hash && !strncmp(checkName, o->name, TERRAIN_NAME_LEN))
		{
			// Name matches.
			return o;
		}
	}

	return NULL;
}

/*--------------------------------------------------
	size_t K_GetTerrainHeapIndex(terrain_t *terrain)

		See header file for description.
--------------------------------------------------*/
size_t K_GetTerrainHeapIndex(terrain_t *terrain)
{
	if (terrain == NULL)
	{
		return SIZE_MAX;
	}

	return (terrain - terrainDefs);
}

/*--------------------------------------------------
	size_t K_GetNumTerrainDefs(void)

		See header file for description.
--------------------------------------------------*/
size_t K_GetNumTerrainDefs(void)
{
	return numTerrainDefs;
}

/*--------------------------------------------------
	terrain_t *K_GetTerrainByIndex(size_t checkIndex)

		See header file for description.
--------------------------------------------------*/
terrain_t *K_GetTerrainByIndex(size_t checkIndex)
{
	if (checkIndex >= numTerrainDefs)
	{
		return NULL;
	}

	return &terrainDefs[checkIndex];
}

/*--------------------------------------------------
	terrain_t *K_GetTerrainByName(const char *checkName)

		See header file for description.
--------------------------------------------------*/
terrain_t *K_GetTerrainByName(const char *checkName)
{
	UINT32 checkHash = quickncasehash(checkName, TERRAIN_NAME_LEN);
	size_t i;

	if (numTerrainDefs > 0)
	{
		for (i = 0; i < numTerrainDefs; i++)
		{
			terrain_t *t = &terrainDefs[i];

			if (checkHash == t->hash && !strncmp(checkName, t->name, TERRAIN_NAME_LEN))
			{
				// Name matches.
				return t;
			}
		}
	}

	return NULL;
}

/*--------------------------------------------------
	terrain_t *K_GetDefaultTerrain(void)

		See header file for description.
--------------------------------------------------*/
terrain_t *K_GetDefaultTerrain(void)
{
	return K_GetTerrainByIndex(defaultTerrain);
}

/*--------------------------------------------------
	size_t K_GetDefaultTerrainID(void)

		See header file for description.
--------------------------------------------------*/
size_t K_GetDefaultTerrainID(void)
{
	return defaultTerrain;
}

/*--------------------------------------------------
	terrain_t *K_GetTerrainForTextureName(const char *checkName)

		See header file for description.
--------------------------------------------------*/
terrain_t *K_GetTerrainForTextureName(const char *checkName)
{
	UINT32 checkHash = quickncasehash(checkName, 8);
	size_t i;

	if (numTerrainFloorDefs > 0)
	{
		for (i = 0; i < numTerrainFloorDefs; i++)
		{
			t_floor_t *f = &terrainFloorDefs[i];

			if (checkHash == f->textureHash && !strncasecmp(checkName, f->textureName, 8))
			{
				return K_GetTerrainByIndex(f->terrainID);
			}
		}
	}

	// This texture doesn't have a terrain directly applied to it,
	// so we fallback to the default terrain.
	return K_GetDefaultTerrain();
}

/*--------------------------------------------------
	size_t K_GetTerrainIDForTextureName(const char *checkName)

		See header file for description.
--------------------------------------------------*/
size_t K_GetTerrainIDForTextureName(const char *checkName)
{
	UINT32 checkHash = quickncasehash(checkName, 8);
	size_t i;

	if (numTerrainFloorDefs > 0)
	{
		for (i = 0; i < numTerrainFloorDefs; i++)
		{
			t_floor_t *f = &terrainFloorDefs[i];

			if (checkHash == f->textureHash && !strncasecmp(checkName, f->textureName, 8))
			{
				return f->terrainID;
			}
		}
	}

	// This texture doesn't have a terrain directly applied to it,
	// so we fallback to the default terrain.
	return K_GetDefaultTerrainID();
}

/*--------------------------------------------------
	terrain_t *K_GetTerrainForTextureNum(INT32 textureNum)

		See header file for description.
--------------------------------------------------*/
terrain_t *K_GetTerrainForTextureNum(INT32 textureNum)
{
	if (textureNum >= 0 && textureNum < numtextures)
	{
		texture_t *tex = textures[textureNum];
		return K_GetTerrainByIndex(tex->terrainID);
	}

	// This texture doesn't have a terrain directly applied to it,
	// so we fallback to the default terrain.
	return K_GetDefaultTerrain();
}

/*--------------------------------------------------
	terrain_t *K_GetTerrainForFlatNum(INT32 flatID)

		See header file for description.
--------------------------------------------------*/
terrain_t *K_GetTerrainForFlatNum(INT32 flatID)
{
	if (flatID < 0 || flatID >= (signed)numlevelflats)
	{
		// Clearly invalid floor...
		return NULL;
	}

	return levelflats[flatID].terrain;
}

/*--------------------------------------------------
	void K_UpdateMobjTerrain(mobj_t *mo, INT32 flatID)

		See header file for description.
--------------------------------------------------*/
void K_UpdateMobjTerrain(mobj_t *mo, INT32 flatID)
{
	if (mo == NULL || P_MobjWasRemoved(mo) == true)
	{
		// Invalid object.
		return;
	}

	if (mo->flags & MF_NOCLIPHEIGHT)
	{
		// You can't collide with floors anyway!
		mo->terrain = NULL;
		return;
	}

	if (mo->player != NULL && mo->player->spectator == true)
	{
		// We don't want a terrain pointer for spectators.
		mo->terrain = NULL;
		return;
	}

	// Update the object's terrain pointer.
	mo->terrain = K_GetTerrainForFlatNum(flatID);
}

/*--------------------------------------------------
	void K_ProcessTerrainEffect(mobj_t *mo)

		See header file for description.
--------------------------------------------------*/
void K_ProcessTerrainEffect(mobj_t *mo)
{
	player_t *player = NULL;
	terrain_t *terrain = NULL;

	if (mo == NULL || P_MobjWasRemoved(mo) == true)
	{
		// Invalid object.
		return;
	}

	if (mo->terrain == NULL)
	{
		// No terrain type.
		return;
	}

	terrain = mo->terrain;
	player = mo->player;

	if (player == NULL)
	{
		// maybe can support regualar mobjs later? :)
		return;
	}

	// Milky Way road effect
	player->outrun = terrain->outrun;

	// Damage effects
	if (terrain->damageType > 0)
	{
		UINT8 dmg = (terrain->damageType & 0xFF);
		P_DamageMobj(mo, NULL, NULL, 1, dmg);
	}

	// Sneaker panel
	if (terrain->flags & TRF_SNEAKERPANEL)
	{
		if (player->floorboost == 0)
			player->floorboost = 3;
		else
			player->floorboost = 2;

		K_DoSneaker(player, 0);
	}

	// Trick panel
	if (terrain->trickPanel > 0 && !(mo->eflags & MFE_SPRUNG))
	{
		const fixed_t hscale = mapobjectscale + (mapobjectscale - mo->scale);
		const fixed_t minspeed = 24*hscale;
		fixed_t speed = FixedHypot(mo->momx, mo->momy);
		fixed_t upwards = 16 * terrain->trickPanel;

		K_DoPogoSpring(mo, upwards, 1);

		// Reduce speed
		speed /= 2;

		if (speed < minspeed)
		{
			speed = minspeed;
		}

		P_InstaThrust(mo, mo->angle, speed);
	}

	// Speed pad
	if (terrain->speedPad > 0)
	{
		if (player->floorboost != 0)
		{
			player->floorboost = 2;
		}
		else
		{
			fixed_t thrustSpeed = terrain->speedPad;
			angle_t thrustAngle = terrain->speedPadAngle;
			fixed_t playerSpeed = P_AproxDistance(player->mo->momx, player->mo->momy);

			// FIXME: come up with a better way to get the touched
			// texture's rotation to this function. At least this
			// will work for 90% of scenarios...

			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				if (player->mo->ceilingrover != NULL)
				{
					thrustAngle -= *player->mo->ceilingrover->bottomangle;
				}
				else
				{
					thrustAngle -= player->mo->subsector->sector->ceilingpic_angle;
				}
			}
			else
			{
				if (player->mo->floorrover != NULL)
				{
					thrustAngle -= *player->mo->floorrover->topangle;
				}
				else
				{
					thrustAngle -= player->mo->subsector->sector->floorpic_angle;
				}
			}

			// Map scale for Shrink, object scale for Grow.
			thrustSpeed = FixedMul(thrustSpeed, max(mapobjectscale, player->mo->scale));

			thrustAngle = K_ReflectAngle(
				K_MomentumAngle(player->mo), thrustAngle,
				playerSpeed, thrustSpeed
			);

			P_InstaThrust(player->mo, thrustAngle, max(thrustSpeed, 2*playerSpeed));

			player->dashpadcooldown = TICRATE/3;
			player->trickpanel = TRICKSTATE_NONE;
			player->floorboost = 2;

			S_StartSound(player->mo, sfx_cdfm62);
		}
	}

	// Spring
	if (terrain->springStrength)
	{
		sector_t *sector = player->mo->subsector->sector;

		const pslope_t *slope;
		angle_t angle = 0;

		fixed_t co = FRACUNIT;
		fixed_t si = 0;

		// FIXME: come up with a better way to get the touched
		// texture's slope to this function. At least this
		// will work for 90% of scenarios...

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if (player->mo->ceilingrover != NULL)
			{
				slope = *player->mo->ceilingrover->b_slope;
			}
			else
			{
				slope = sector->c_slope;
			}
		}
		else
		{
			if (player->mo->floorrover != NULL)
			{
				slope = *player->mo->floorrover->t_slope;
			}
			else
			{
				slope = sector->f_slope;
			}
		}

		if (slope)
		{
			const angle_t fa = (slope->zangle >> ANGLETOFINESHIFT);

			co = FINECOSINE(fa) * P_MobjFlip(player->mo);
			si = -(FINESINE(fa));

			angle = slope->xydirection;
		}

		P_DoSpringEx(player->mo, mapobjectscale,
				FixedMul(terrain->springStrength, co),
				FixedMul(terrain->springStrength, si),
				angle, terrain->springStarColor);

		sector->soundorg.z = player->mo->z;
		S_StartSound(&sector->soundorg, sfx_s3kb1);
	}

	// Bumpy floor
	if (terrain->flags & TRF_STAIRJANK && player->speed != 0)
	{
		/* use a shorter sound if not two tics have passed
		 * since the last step */
		S_ReducedVFXSound(mo, player->stairjank
				>= 16 ?  sfx_s23b : sfx_s268, NULL);

		if (player->stairjank == 0)
		{
			mobj_t *spark = P_SpawnMobjFromMobj(mo,
					0, 0, 0, MT_JANKSPARK);
			spark->fuse = 9;
			spark->cusval = K_StairJankFlip(ANGLE_90);
			P_SetTarget(&spark->target, mo);
			P_SetTarget(&spark->owner, mo);
			spark->renderflags |= RF_REDUCEVFX;
		}

		player->stairjank = 17;
	}

	// (Offroad is handled elsewhere!)
}

/*--------------------------------------------------
	void K_SetDefaultFriction(mobj_t *mo)

		See header file for description.
--------------------------------------------------*/
void K_SetDefaultFriction(mobj_t *mo)
{
	boolean isPlayer = false;

	if (mo == NULL || P_MobjWasRemoved(mo) == true)
	{
		// Invalid object.
		return;
	}

	isPlayer = (mo->player != NULL);

	mo->friction = ORIG_FRICTION;

	if (isPlayer == true)
	{
		mo->movefactor = FRACUNIT;
	}

	if (mo->terrain != NULL)
	{
		fixed_t strength = mo->terrain->friction;

		fixed_t newFriction = INT32_MAX;
		fixed_t newMovefactor = INT32_MAX;

		if (strength > 0) // sludge
		{
			strength = strength * 2; // otherwise, the maximum sludginess value is +967...
		}

		// The following might seem odd. At the time of movement,
		// the move distance is multiplied by 'friction/0x10000', so a
		// higher friction value actually means 'less friction'.
		newFriction = ORIG_FRICTION - FixedMul(0x1EB8, strength) / 0x80; // ORIG_FRICTION is 0xE800

		if (newFriction > FRACUNIT)
		{
			newFriction = FRACUNIT;
		}

		if (newFriction < 0)
		{
			newFriction = 0;
		}

		mo->friction = newFriction;

		if (isPlayer == true)
		{
			newMovefactor = FixedDiv(ORIG_FRICTION, newFriction);

			if (newMovefactor < FRACUNIT)
			{
				newMovefactor = 19*newMovefactor - 18*FRACUNIT;
			}
			else
			{
				newMovefactor = FRACUNIT;
			}

			mo->movefactor = newMovefactor;
		}
	}
}

/*--------------------------------------------------
	static void K_SpawnSplashParticles(mobj_t *mo, t_splash_t *s, fixed_t impact)

		Creates all of the splash particles for an object
		from a splash definition.

	Input Arguments:-
		mo - The object to spawn the splash particles for.
		s - The splash definition to use.
		impact - How hard the object hit the surface.

	Return:-
		N/A
--------------------------------------------------*/
static void K_SpawnSplashParticles(mobj_t *mo, t_splash_t *s, fixed_t impact)
{
	const UINT8 numParticles = s->numParticles;
	const angle_t particleSpread = ANGLE_MAX / numParticles;

	fixed_t momH = INT32_MAX;
	fixed_t momV = INT32_MAX;

	size_t i;

	momH = FixedMul(impact, s->pushH);
	momV = FixedMul(impact, s->pushV);

	for (i = 0; i < numParticles; i++)
	{
		mobj_t *dust = NULL;
		angle_t pushAngle = (particleSpread * i);

		fixed_t xOff = 0;
		fixed_t yOff = 0;

		if (numParticles == 1)
		{
			// Random angle.
			pushAngle = P_RandomRange(PR_TERRAIN, 0, ANGLE_MAX);
		}

		if (s->spread > 0)
		{
			xOff = P_RandomRange(PR_TERRAIN, -s->spread / FRACUNIT, s->spread / FRACUNIT) * FRACUNIT;
			yOff = P_RandomRange(PR_TERRAIN, -s->spread / FRACUNIT, s->spread / FRACUNIT) * FRACUNIT;
		}

		if (s->cone > 0)
		{
			pushAngle += P_RandomRange(PR_TERRAIN, -s->cone / ANG1, s->cone / ANG1) * ANG1;
		}

		dust = P_SpawnMobjFromMobj(
			mo,
			xOff + (12 * FINECOSINE(pushAngle >> ANGLETOFINESHIFT)),
			yOff + (12 * FINESINE(pushAngle >> ANGLETOFINESHIFT)),
			0, //P_RandomRange(PR_TERRAIN, 0, s->spread / FRACUNIT) * FRACUNIT,
			s->mobjType
		);

		P_SetTarget(&dust->target, mo);
		dust->angle = pushAngle;

		dust->destscale = FixedMul(mo->scale, s->scale);
		P_SetScale(dust, dust->destscale);

		dust->momx = mo->momx / 2;
		dust->momy = mo->momy / 2;
		dust->momz = 0;

		dust->momx += FixedMul(momH, FINECOSINE(pushAngle >> ANGLETOFINESHIFT));
		dust->momy += FixedMul(momH, FINESINE(pushAngle >> ANGLETOFINESHIFT));
		dust->momz += (momV / 16) * P_MobjFlip(mo);

		if (s->color != SKINCOLOR_NONE)
		{
			dust->color = s->color;
		}

		if (s->sfx != sfx_None)
		{
			S_StartSound(mo, s->sfx);
		}
	}
}

/*--------------------------------------------------
	void K_SpawnSplashForMobj(mobj_t *mo, fixed_t impact)

		See header file for description.
--------------------------------------------------*/
void K_SpawnSplashForMobj(mobj_t *mo, fixed_t impact)
{
	const fixed_t minImpact = mo->scale;
	t_splash_t *s = NULL;

	if (mo == NULL || P_MobjWasRemoved(mo) == true)
	{
		// Invalid object.
		return;
	}

	if (!(mo->flags & MF_APPLYTERRAIN))
	{
		// No TERRAIN effects for this object.
		return;
	}

	if (mo->terrain == NULL || mo->terrain->splashID == SIZE_MAX)
	{
		// No impact for this terrain type.
		return;
	}
	else
	{
		s = K_GetSplashByIndex(mo->terrain->splashID);
	}

	if (s == NULL || s->mobjType == MT_NULL || s->numParticles == 0)
	{
		// No particles to spawn.
		return;
	}

	impact /= 4;

	if (impact < minImpact)
	{
		impact = minImpact;
	}

	// Idea for later: if different spawning styles are desired,
	// we can put a switch case here!
	K_SpawnSplashParticles(mo, s, impact);
}

/*--------------------------------------------------
	static void K_SpawnFootstepParticle(mobj_t *mo, t_footstep_t *fs)

		Creates a new footstep particle for an object
		from a footstep definition.

	Input Arguments:-
		mo - The object to spawn the footstep particle for.
		fs - The footstep definition to use.
		timer - Spawning frequency timer.

	Return:-
		N/A
--------------------------------------------------*/
static void K_SpawnFootstepParticle(mobj_t *mo, t_footstep_t *fs, tic_t timer)
{
	mobj_t *dust = NULL;
	angle_t pushAngle = ANGLE_MAX;
	angle_t tireAngle = ANGLE_MAX;
	fixed_t momentum = INT32_MAX;
	fixed_t speedValue = INT32_MAX;
	fixed_t momH = INT32_MAX;
	fixed_t momV = INT32_MAX;
	fixed_t xOff = 0;
	fixed_t yOff = 0;

	if (timer % fs->frequency != 0)
	{
		return;
	}

	momentum = P_AproxDistance(mo->momx, mo->momy);

	if (mo->player != NULL)
	{
		tireAngle = (mo->player->drawangle + ANGLE_180);
		speedValue = K_GetKartSpeedFromStat(mo->player->kartspeed);
	}
	else
	{
		tireAngle = (mo->angle + ANGLE_180);
		speedValue = K_GetKartSpeedFromStat(5);
	}

	speedValue = FixedMul(speedValue, mo->scale);
	speedValue = FixedMul(speedValue, fs->requiredSpeed);

	if (momentum < speedValue)
	{
		return;
	}

	pushAngle = K_MomentumAngle(mo) + ANGLE_180;

	if (((timer / fs->frequency) / 2) & 1)
	{
		tireAngle -= ANGLE_45;
		tireAngle -= P_RandomRange(PR_TERRAIN, 0, fs->cone / ANG1) * ANG1;
		pushAngle -= P_RandomRange(PR_TERRAIN, 0, fs->cone / ANG1) * ANG1;
	}
	else
	{
		tireAngle += ANGLE_45;
		tireAngle += P_RandomRange(PR_TERRAIN, 0, fs->cone / ANG1) * ANG1;
		pushAngle += P_RandomRange(PR_TERRAIN, 0, fs->cone / ANG1) * ANG1;
	}

	if (fs->spread > 0)
	{
		xOff = P_RandomRange(PR_TERRAIN, -fs->spread / FRACUNIT, fs->spread / FRACUNIT) * FRACUNIT;
		yOff = P_RandomRange(PR_TERRAIN, -fs->spread / FRACUNIT, fs->spread / FRACUNIT) * FRACUNIT;
	}

	dust = P_SpawnMobjFromMobj(
		mo,
		xOff + (24 * FINECOSINE(tireAngle >> ANGLETOFINESHIFT)),
		yOff + (24 * FINESINE(tireAngle >> ANGLETOFINESHIFT)),
		0, fs->mobjType
	);

	P_SetTarget(&dust->target, mo);
	dust->angle = K_MomentumAngle(mo);

	dust->destscale = FixedMul(mo->scale, fs->scale);
	P_SetScale(dust, dust->destscale);

	dust->momx = mo->momx;
	dust->momy = mo->momy;
	dust->momz = P_GetMobjZMovement(mo);

	momH = FixedMul(momentum, fs->pushH);
	momV = FixedMul(momentum, fs->pushV);

	dust->momx += FixedMul(momH, FINECOSINE(pushAngle >> ANGLETOFINESHIFT));
	dust->momy += FixedMul(momH, FINESINE(pushAngle >> ANGLETOFINESHIFT));
	dust->momz += (momV / 16) * P_MobjFlip(mo);

	if (fs->color != SKINCOLOR_NONE)
	{
		dust->color = fs->color;
	}

	if ((fs->sfx != sfx_None) && (fs->sfxFreq > 0) && (timer % fs->sfxFreq == 0))
	{
		S_StartSound(mo, fs->sfx);
	}
}

/*--------------------------------------------------
	void K_HandleFootstepParticles(mobj_t *mo)

		See header file for description.
--------------------------------------------------*/

#define INVALIDFOOTSTEP (fs == NULL || fs->mobjType == MT_NULL || fs->frequency <= 0)

void K_HandleFootstepParticles(mobj_t *mo)
{
	tic_t timer = leveltime;
	t_footstep_t *fs = NULL;

	if (mo == NULL || P_MobjWasRemoved(mo) == true)
	{
		// Invalid object.
		return;
	}

	if (mo->player)
	{
		// Offset the timer.
		timer += mo->player - players;
	}

	if (!(mo->flags & MF_APPLYTERRAIN) || mo->terrain == NULL)
	{
		// No TERRAIN effects for this object.
		goto offroadhandle;
	}

	fs = K_GetFootstepByIndex(mo->terrain->footstepID);

	if (INVALIDFOOTSTEP)
	{
		// No particles to spawn.
		goto offroadhandle;
	}

	// Idea for later: if different spawning styles are desired,
	// we can put a switch case here!
	K_SpawnFootstepParticle(mo, fs, timer);

	return;

offroadhandle:
	// ...unless you're
	// - A player and
	if (mo->player == NULL)
	{
		return;
	}

	// - Being affected by offroad
	if (mo->player->boostpower >= FRACUNIT)
	{
		return;
	}

	// in which case make default offroad footstep!
	fs = K_GetFootstepByIndex(defaultOffroadFootstep);

	if (INVALIDFOOTSTEP)
	{
		// No particles to spawn.
		return;
	}

	K_SpawnFootstepParticle(mo, fs, timer);
}

#undef INVALIDFOOTSTEP

/*--------------------------------------------------
	static void K_CleanupTerrainOverlay(mobj_t *mo)

		Removes an object's terrain overlay.

	Input Arguments:-
		mo - The object to remove the overlay from.

	Return:-
		N/A
--------------------------------------------------*/
static void K_CleanupTerrainOverlay(mobj_t *mo)
{
	if (mo->terrainOverlay != NULL && P_MobjWasRemoved(mo->terrainOverlay) == false)
	{
		P_RemoveMobj(mo->terrainOverlay);
	}
}

/*--------------------------------------------------
	static boolean K_InitTerrainOverlay(mobj_t *mo)

		Creates a new terrain overlay for an object.

	Input Arguments:-
		mo - The object to give an overlay to.

	Return:-
		true if successful, otherwise false.
--------------------------------------------------*/
static boolean K_InitTerrainOverlay(mobj_t *mo)
{
	mobj_t *new = P_SpawnMobjFromMobj(mo, 0, 0, 0, MT_OVERLAY);

	// Tells the overlay that we haven't set up a state yet.
	new->extravalue1 = TOV_UNDEFINED;

	// Set up our pointers.
	P_SetTarget(&new->target, mo);
	P_SetTarget(&mo->terrainOverlay, new);

	return true;
}

/*--------------------------------------------------
	static t_overlay_state_t K_DesiredTerrainOverlayAction(mobj_t *mo)

		Figures out the overlay action to use for an object.

	Input Arguments:-
		mo - The object
		st - The terrain overlay state.

	Return:-
		The overlay action enum to use for the object.
--------------------------------------------------*/
static t_overlay_action_t K_DesiredTerrainOverlayAction(mobj_t *mo)
{
	const boolean moving = (P_AproxDistance(mo->momx, mo->momy) >= (mo->scale >> 1));

	if (moving == true)
	{
		return TOV_MOVING;
	}

	return TOV_STILL;
}

/*--------------------------------------------------
	static statenum_t K_GetTerrainOverlayState(t_overlay_t *o, t_overlay_action_t act)

		Converts our overlay's action enum into an actual state ID.

	Input Arguments:-
		o - The overlay properties.
		act - The terrain overlay action.

	Return:-
		The actual state ID, for use with P_SetMobjState.
--------------------------------------------------*/
static statenum_t K_GetTerrainOverlayState(t_overlay_t *o, t_overlay_action_t act)
{
	if (act >= 0 && act < TOV__MAX)
	{
		return o->states[act];
	}

	return S_NULL;
}

/*--------------------------------------------------
	static void K_SetTerrainOverlayState(mobj_t *mo, t_overlay_action_t act, statenum_t st)

		Updates our overlay's current state.

	Input Arguments:-
		o - The overlay properties.
		act - The terrain overlay action.
		st - The new object's state.

	Return:-
		N/A
--------------------------------------------------*/
static void K_SetTerrainOverlayState(mobj_t *mo, t_overlay_action_t act, statenum_t st)
{
	if (act == mo->terrainOverlay->extravalue1)
	{
		// Already set the state, so leave it alone.
		return;
	}

	P_SetMobjState(mo->terrainOverlay, st);
	mo->terrainOverlay->extravalue1 = act;
}

/*--------------------------------------------------
	static void K_UpdateTerrainOverlay(mobj_t *mo)

		See header file for description.
--------------------------------------------------*/
void K_UpdateTerrainOverlay(mobj_t *mo)
{
	t_overlay_t *o = NULL;
	t_overlay_action_t act = TOV_UNDEFINED;
	statenum_t st = S_NULL;

	if (mo == NULL || P_MobjWasRemoved(mo) == true)
	{
		// Invalid object.
		return;
	}

	if (!(mo->flags & MF_APPLYTERRAIN))
	{
		// No TERRAIN effects for this object.
		K_CleanupTerrainOverlay(mo);
		return;
	}

	if (mo->terrain == NULL || mo->terrain->overlayID == SIZE_MAX)
	{
		// No overlay for this terrain type.
		K_CleanupTerrainOverlay(mo);
		return;
	}
	else
	{
		o = K_GetOverlayByIndex(mo->terrain->overlayID);
	}

	if (o == NULL)
	{
		// No overlay to use.
		K_CleanupTerrainOverlay(mo);
		return;
	}

	// Determine the state to use. We want to do this before creating
	// the overlay, so that we keep it despawned if the state is S_NULL.
	act = K_DesiredTerrainOverlayAction(mo);
	st = K_GetTerrainOverlayState(o, act);

	if (st == S_NULL)
	{
		// No state to use for this action.
		K_CleanupTerrainOverlay(mo);
		return;
	}

	if (mo->terrainOverlay == NULL || P_MobjWasRemoved(mo->terrainOverlay) == true)
	{
		// Doesn't exist currently, so try to create
		// a new terrain overlay.

		if (K_InitTerrainOverlay(mo) == false)
		{
			// We were unsuccessful, get out of here.
			return;
		}
	}

	mo->terrainOverlay->spriteyoffset = -mo->terrain->floorClip;
	mo->terrainOverlay->color = o->color;
	mo->terrainOverlay->movefactor = o->scale;

	K_SetTerrainOverlayState(mo, act, st);

	if (mo->state->tics > 1 && o->speed > 0)
	{
		const fixed_t maxSpeed = 60 * mapobjectscale;
		fixed_t speed = P_AproxDistance(mo->momx, mo->momy);
		fixed_t speedDiv = FRACUNIT + FixedMul(FixedDiv(speed, maxSpeed), o->speed);
		tic_t animSpeed = max(FixedDiv(mo->state->tics, speedDiv), 1);

		mo->tics = min((tic_t)mo->tics, animSpeed);
	}
}

/*--------------------------------------------------
	static void K_FlagBoolean(UINT32 *inputFlags, UINT32 newFlag, char *val)

		Sets a flag to true or false depending on
		the string input.

	Input Arguments:-
		inputFlags - Pointer to flags value to modify.
		newFlag - The flag(s) to set / unset.
		val - The string input from the file.

	Return:-
		None
--------------------------------------------------*/
static void K_FlagBoolean(UINT32 *inputFlags, UINT32 newFlag, char *val)
{
	if (stricmp(val, "true") == 0)
	{
		*inputFlags |= newFlag;
	}
	else if (stricmp(val, "false") == 0)
	{
		*inputFlags &= ~newFlag;
	}
}

/*--------------------------------------------------
	static void K_SplashDefaults(t_splash_t *splash)

		Sets the defaults for a new Splash block.

	Input Arguments:-
		splash - Terrain Splash structure to default.

	Return:-
		None
--------------------------------------------------*/
static void K_SplashDefaults(t_splash_t *splash)
{
	splash->mobjType = MT_NULL;
	splash->sfx = sfx_None;
	splash->scale = FRACUNIT;
	splash->color = SKINCOLOR_NONE;

	splash->pushH = FRACUNIT/4;
	splash->pushV = FRACUNIT/64;
	splash->spread = 2*FRACUNIT;
	splash->cone = ANGLE_11hh;

	splash->numParticles = 8;
}

/*--------------------------------------------------
	static void K_NewSplashDefs(void)

		Increases the size of splashDefs by 1, and
		sets the new struct's values to their defaults.

	Input Arguments:-
		None

	Return:-
		None
--------------------------------------------------*/
static void K_NewSplashDefs(void)
{
	numSplashDefs++;
	splashDefs = (t_splash_t *)Z_Realloc(splashDefs, sizeof(t_splash_t) * (numSplashDefs + 1), PU_STATIC, NULL);
	K_SplashDefaults( &splashDefs[numSplashDefs - 1] );
}

/*--------------------------------------------------
	static void K_ParseSplashParameter(size_t i, char *param, char *val)

		Parser function for Splash blocks.

	Input Arguments:-
		i - Struct ID
		param - Parameter string
		val - Value string

	Return:-
		None
--------------------------------------------------*/
static void K_ParseSplashParameter(size_t i, char *param, char *val)
{
	t_splash_t *splash = &splashDefs[i];

	if (stricmp(param, "mobjType") == 0)
	{
		splash->mobjType = get_number(val);
	}
	else if (stricmp(param, "sfx") == 0)
	{
		splash->sfx = get_number(val);
	}
	else if (stricmp(param, "scale") == 0)
	{
		splash->scale = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "color") == 0)
	{
		splash->color = get_number(val);
	}
	else if (stricmp(param, "pushH") == 0)
	{
		splash->pushH = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "pushV") == 0)
	{
		splash->pushV = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "spread") == 0)
	{
		splash->spread = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "cone") == 0)
	{
		splash->cone = FloatToAngle(atof(val));
	}
	else if (stricmp(param, "numParticles") == 0)
	{
		splash->numParticles = (UINT8)atoi(val);
	}
}

/*--------------------------------------------------
	static void K_FootstepDefaults(t_footstep_t *footstep)

		Sets the defaults for a new Footstep block.

	Input Arguments:-
		footstep - Terrain Footstep structure to default.

	Return:-
		None
--------------------------------------------------*/
static void K_FootstepDefaults(t_footstep_t *footstep)
{
	footstep->mobjType = MT_NULL;
	footstep->sfx = sfx_None;
	footstep->scale = FRACUNIT;
	footstep->color = SKINCOLOR_NONE;

	footstep->pushH = FRACUNIT/2;
	footstep->pushV = FRACUNIT/32;
	footstep->spread = 2*FRACUNIT;
	footstep->cone = ANGLE_11hh;

	footstep->sfxFreq = 6;
	footstep->frequency = 1;
	footstep->requiredSpeed = 0;
}

/*--------------------------------------------------
	static void K_NewFootstepDefs(void)

		Increases the size of footstepDefs by 1, and
		sets the new struct's values to their defaults.

	Input Arguments:-
		None

	Return:-
		None
--------------------------------------------------*/
static void K_NewFootstepDefs(void)
{
	numFootstepDefs++;
	footstepDefs = (t_footstep_t *)Z_Realloc(footstepDefs, sizeof(t_footstep_t) * (numFootstepDefs + 1), PU_STATIC, NULL);
	K_FootstepDefaults( &footstepDefs[numFootstepDefs - 1] );
}

/*--------------------------------------------------
	static void K_ParseFootstepParameter(size_t i, char *param, char *val)

		Parser function for Footstep blocks.

	Input Arguments:-
		i - Struct ID
		param - Parameter string
		val - Value string

	Return:-
		None
--------------------------------------------------*/
static void K_ParseFootstepParameter(size_t i, char *param, char *val)
{
	t_footstep_t *footstep = &footstepDefs[i];

	if (stricmp(param, "mobjType") == 0)
	{
		footstep->mobjType = get_number(val);
	}
	else if (stricmp(param, "sfx") == 0)
	{
		footstep->sfx = get_number(val);
	}
	else if (stricmp(param, "scale") == 0)
	{
		footstep->scale = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "color") == 0)
	{
		footstep->color = get_number(val);
	}
	else if (stricmp(param, "pushH") == 0)
	{
		footstep->pushH = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "pushV") == 0)
	{
		footstep->pushV = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "spread") == 0)
	{
		footstep->spread = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "cone") == 0)
	{
		footstep->cone = FloatToAngle(atof(val));
	}
	else if (stricmp(param, "sfxFreq") == 0)
	{
		footstep->sfxFreq = (tic_t)atoi(val);
	}
	else if (stricmp(param, "frequency") == 0)
	{
		footstep->frequency = (tic_t)atoi(val);
	}
	else if (stricmp(param, "requiredSpeed") == 0)
	{
		footstep->requiredSpeed = FLOAT_TO_FIXED(atof(val));
	}
}

/*--------------------------------------------------
	static void K_OverlayDefaults(t_overlay_t *overlay)

		Sets the defaults for a new Overlay block.

	Input Arguments:-
		overlay - Terrain Overlay structure to default.

	Return:-
		None
--------------------------------------------------*/
static void K_OverlayDefaults(t_overlay_t *overlay)
{
	size_t i;

	for (i = 0; i < TOV__MAX; i++)
	{
		overlay->states[i] = S_NULL;
	}

	overlay->scale = FRACUNIT;
	overlay->color = SKINCOLOR_NONE;
	overlay->speed = FRACUNIT;
}

/*--------------------------------------------------
	static void K_NewOverlayDefs(void)

		Increases the size of overlayDefs by 1, and
		sets the new struct's values to their defaults.

	Input Arguments:-
		None

	Return:-
		None
--------------------------------------------------*/
static void K_NewOverlayDefs(void)
{
	numOverlayDefs++;
	overlayDefs = (t_overlay_t *)Z_Realloc(overlayDefs, sizeof(t_overlay_t) * (numOverlayDefs + 1), PU_STATIC, NULL);
	K_OverlayDefaults( &overlayDefs[numOverlayDefs - 1] );
}

/*--------------------------------------------------
	static void K_ParseOverlayParameter(size_t i, char *param, char *val)

		Parser function for Overlay blocks.

	Input Arguments:-
		i - Struct ID
		param - Parameter string
		val - Value string

	Return:-
		None
--------------------------------------------------*/
static void K_ParseOverlayParameter(size_t i, char *param, char *val)
{
	t_overlay_t *overlay = &overlayDefs[i];

	if (stricmp(param, "stillState") == 0)
	{
		overlay->states[TOV_STILL] = get_number(val);
	}
	else if (stricmp(param, "movingState") == 0)
	{
		overlay->states[TOV_MOVING] = get_number(val);
	}
	else if (stricmp(param, "scale") == 0)
	{
		overlay->scale = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "color") == 0)
	{
		overlay->color = get_number(val);
	}
	else if (stricmp(param, "speed") == 0)
	{
		overlay->speed = FLOAT_TO_FIXED(atof(val));
	}
}

/*--------------------------------------------------
	static void K_TerrainDefaults(terrain_t *terrain)

		Sets the defaults for a new Terrain block.

	Input Arguments:-
		terrain - Terrain structure to default.

	Return:-
		None
--------------------------------------------------*/
static void K_TerrainDefaults(terrain_t *terrain)
{
	terrain->splashID = SIZE_MAX;
	terrain->footstepID = SIZE_MAX;
	terrain->overlayID = SIZE_MAX;

	terrain->friction = 0;
	terrain->offroad = 0;
	terrain->damageType = -1;
	terrain->trickPanel = 0;
	terrain->speedPad = 0;
	terrain->speedPadAngle = 0;
	terrain->springStrength = 0;
	terrain->springStarColor = SKINCOLOR_NONE;
	terrain->flags = TRF_REMAP;
}

/*--------------------------------------------------
	boolean K_TerrainHasAffect(terrain_t *terrain)

		See header file for description.
--------------------------------------------------*/

boolean K_TerrainHasAffect(terrain_t *terrain, boolean badonly)
{
	if (terrain->friction > 0
	|| terrain->offroad != 0
	|| terrain->damageType != -1
	|| (terrain->flags & TRF_STAIRJANK))
		return true;

	if (badonly)
		return false;

	return (terrain->friction != 0
	|| terrain->trickPanel != 0
	|| terrain->speedPad != 0
	|| terrain->springStrength != 0
	|| terrain->flags != 0);
}

/*--------------------------------------------------
	static void K_NewTerrainDefs(void)

		Increases the size of terrainDefs by 1, and
		sets the new struct's values to their defaults.

	Input Arguments:-
		None

	Return:-
		None
--------------------------------------------------*/
static void K_NewTerrainDefs(void)
{
	numTerrainDefs++;
	terrainDefs = (terrain_t *)Z_Realloc(terrainDefs, sizeof(terrain_t) * (numTerrainDefs + 1), PU_STATIC, NULL);
	K_TerrainDefaults( &terrainDefs[numTerrainDefs - 1] );
}

/*--------------------------------------------------
	static void K_ParseTerrainParameter(size_t i, char *param, char *val)

		Parser function for Terrain blocks.

	Input Arguments:-
		i - Struct ID
		param - Parameter string
		val - Value string

	Return:-
		None
--------------------------------------------------*/
static void K_ParseTerrainParameter(size_t i, char *param, char *val)
{
	terrain_t *terrain = &terrainDefs[i];

	if (stricmp(param, "splash") == 0)
	{
		t_splash_t *splash = K_GetSplashByName(val);
		terrain->splashID = K_GetSplashHeapIndex(splash);
	}
	else if (stricmp(param, "footstep") == 0)
	{
		t_footstep_t *footstep = K_GetFootstepByName(val);
		terrain->footstepID = K_GetFootstepHeapIndex(footstep);
	}
	else if (stricmp(param, "overlay") == 0)
	{
		t_overlay_t *overlay = K_GetOverlayByName(val);
		terrain->overlayID = K_GetOverlayHeapIndex(overlay);
	}
	else if (stricmp(param, "friction") == 0)
	{
		terrain->friction = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "offroad") == 0)
	{
		terrain->offroad = (UINT8)get_number(val); // offroad strength enum?
	}
	else if (stricmp(param, "damageType") == 0)
	{
		terrain->damageType = (INT16)get_number(val);
	}
	else if (stricmp(param, "trickPanel") == 0)
	{
		terrain->trickPanel = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "speedPad") == 0)
	{
		terrain->speedPad = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "speedPadAngle") == 0)
	{
		terrain->speedPadAngle = FixedAngle(FLOAT_TO_FIXED(atof(val)));
	}
	else if (stricmp(param, "springStrength") == 0)
	{
		const double fval = atof(val);

		if (fpclassify(fval) == FP_ZERO)
		{
			terrain->springStrength = 0;
		}
		else
		{
			// Springs increase in stength by 1.6 times the
			// previous strength. Grey spring is 25 and
			// 25/1.6 = 15.625
			terrain->springStrength =
				FLOAT_TO_FIXED(15.625 * pow(1.6, fval));
		}
	}
	else if (stricmp(param, "springStarColor") == 0)
	{
		terrain->springStarColor = get_number(val);
	}
	else if (stricmp(param, "outrun") == 0 || stricmp(param, "speedIncrease") == 0)
	{
		terrain->outrun = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "floorClip") == 0)
	{
		terrain->floorClip = FLOAT_TO_FIXED(atof(val));
	}
	else if (stricmp(param, "liquid") == 0)
	{
		K_FlagBoolean(&terrain->flags, TRF_LIQUID, val);
	}
	else if (stricmp(param, "sneakerPanel") == 0)
	{
		K_FlagBoolean(&terrain->flags, TRF_SNEAKERPANEL, val);
	}
	else if (stricmp(param, "bumpy") == 0 || stricmp(param, "stairJank") == 0)
	{
		K_FlagBoolean(&terrain->flags, TRF_STAIRJANK, val);
	}
	else if (stricmp(param, "tripwire") == 0)
	{
		K_FlagBoolean(&terrain->flags, TRF_TRIPWIRE, val);
	}
	else if (stricmp(param, "remap") == 0)
	{
		K_FlagBoolean(&terrain->flags, TRF_REMAP, val);
	}
}

/*--------------------------------------------------
	static void K_NewTerrainFloorDefs(void)

		Increases the size of numTerrainFloorDefs by 1.

	Input Arguments:-
		None

	Return:-
		None
--------------------------------------------------*/
static void K_NewTerrainFloorDefs(void)
{
	numTerrainFloorDefs++;
	terrainFloorDefs = (t_floor_t *)Z_Realloc(terrainFloorDefs, sizeof(t_floor_t) * (numTerrainFloorDefs + 1), PU_STATIC, NULL);
}

/*--------------------------------------------------
	static boolean K_DoTERRAINLumpParse(size_t num, void (*parser)(UINT32, char *, char *))

		Runs another parser function for the TERRAIN
		lump, handling the nitty-gritty parts of the
		token handling.

	Input Arguments:-
		num - Struct ID to modify. Which one it will modify depends on the parser function.
		parser - The parser function. Takes three inputs: Struct ID, Parameter String, and Value String.

	Return:-
		false if any errors occured, otherwise true.
--------------------------------------------------*/
static boolean K_DoTERRAINLumpParse(size_t num, void (*parser)(size_t, char *, char *))
{
	char *param, *val;

	param = M_GetToken(NULL);

	if (!fastcmp(param, "{"))
	{
		Z_Free(param);
		CONS_Alert(CONS_WARNING, "Invalid TERRAIN data capsule!\n");
		return false;
	}

	Z_Free(param);

	while (true)
	{
		param = M_GetToken(NULL);

		if (fastcmp(param, "}"))
		{
			Z_Free(param);
			break;
		}

		val = M_GetToken(NULL);
		parser(num, param, val);

		Z_Free(param);
		Z_Free(val);
	}

	return true;
}

/*--------------------------------------------------
	static boolean K_TERRAINLumpParser(char *data, size_t size)

		Parses inputted lump data as a TERRAIN lump.

	Input Arguments:-
		data - Pointer to lump data.
		size - The length of the lump data.

	Return:-
		false if any errors occured, otherwise true.
--------------------------------------------------*/
static boolean K_TERRAINLumpParser(char *data, size_t size)
{
	char *tkn = M_GetToken(data);
	UINT32 tknHash = 0;
	size_t pos = 0;
	size_t i;

	while (tkn && (pos = M_GetTokenPos()) < size)
	{
		boolean valid = true;

		// Avoid anything inside bracketed stuff, only look for external keywords.
		if (fastcmp(tkn, "{") || fastcmp(tkn, "}"))
		{
			CONS_Alert(CONS_ERROR, "Rogue bracket detected in TERRAIN lump.\n");
			valid = false;
		}
		// Check for valid fields.
		else if (stricmp(tkn, "splash") == 0)
		{
			Z_Free(tkn);
			tkn = M_GetToken(NULL);
			pos = M_GetTokenPos();

			if (tkn && pos <= size)
			{
				t_splash_t *s = NULL;

				tknHash = quickncasehash(tkn, TERRAIN_NAME_LEN);

				for (i = 0; i < numSplashDefs; i++)
				{
					s = &splashDefs[i];

					if (tknHash == s->hash && !strncmp(tkn, s->name, TERRAIN_NAME_LEN))
					{
						break;
					}
				}

				if (i == numSplashDefs)
				{
					K_NewSplashDefs();
					s = &splashDefs[i];

					strncpy(s->name, tkn, TERRAIN_NAME_LEN);
					s->hash = tknHash;

					CONS_Printf("Created new Splash type '%s'\n", s->name);
				}

				valid = K_DoTERRAINLumpParse(i, K_ParseSplashParameter);
			}
			else
			{
				CONS_Alert(CONS_ERROR, "No Splash type name.\n");
				valid = false;
			}
		}
		else if (stricmp(tkn, "footstep") == 0)
		{
			Z_Free(tkn);
			tkn = M_GetToken(NULL);
			pos = M_GetTokenPos();

			if (tkn && pos <= size)
			{
				t_footstep_t *fs = NULL;

				tknHash = quickncasehash(tkn, TERRAIN_NAME_LEN);

				for (i = 0; i < numFootstepDefs; i++)
				{
					fs = &footstepDefs[i];

					if (tknHash == fs->hash && !strncmp(tkn, fs->name, TERRAIN_NAME_LEN))
					{
						break;
					}
				}

				if (i == numFootstepDefs)
				{
					K_NewFootstepDefs();
					fs = &footstepDefs[i];

					strncpy(fs->name, tkn, TERRAIN_NAME_LEN);
					fs->hash = tknHash;

					CONS_Printf("Created new Footstep type '%s'\n", fs->name);
				}

				valid = K_DoTERRAINLumpParse(i, K_ParseFootstepParameter);
			}
			else
			{
				CONS_Alert(CONS_ERROR, "No Footstep type name.\n");
				valid = false;
			}
		}
		else if (stricmp(tkn, "overlay") == 0)
		{
			Z_Free(tkn);
			tkn = M_GetToken(NULL);
			pos = M_GetTokenPos();

			if (tkn && pos <= size)
			{
				t_overlay_t *o = NULL;

				tknHash = quickncasehash(tkn, TERRAIN_NAME_LEN);

				for (i = 0; i < numOverlayDefs; i++)
				{
					o = &overlayDefs[i];

					if (tknHash == o->hash && !strncmp(tkn, o->name, TERRAIN_NAME_LEN))
					{
						break;
					}
				}

				if (i == numOverlayDefs)
				{
					K_NewOverlayDefs();
					o = &overlayDefs[i];

					strncpy(o->name, tkn, TERRAIN_NAME_LEN);
					o->hash = tknHash;

					CONS_Printf("Created new Overlay type '%s'\n", o->name);
				}

				valid = K_DoTERRAINLumpParse(i, K_ParseOverlayParameter);
			}
			else
			{
				CONS_Alert(CONS_ERROR, "No Overlay type name.\n");
				valid = false;
			}
		}
		else if (stricmp(tkn, "terrain") == 0)
		{
			Z_Free(tkn);
			tkn = M_GetToken(NULL);
			pos = M_GetTokenPos();

			if (tkn && pos <= size)
			{
				terrain_t *t = NULL;

				tknHash = quickncasehash(tkn, TERRAIN_NAME_LEN);

				for (i = 0; i < numTerrainDefs; i++)
				{
					t = &terrainDefs[i];

					if (tknHash == t->hash && !strncmp(tkn, t->name, TERRAIN_NAME_LEN))
					{
						break;
					}
				}

				if (i == numTerrainDefs)
				{
					K_NewTerrainDefs();
					t = &terrainDefs[i];

					strncpy(t->name, tkn, TERRAIN_NAME_LEN);
					t->hash = tknHash;

					CONS_Printf("Created new Terrain type '%s'\n", t->name);
				}

				valid = K_DoTERRAINLumpParse(i, K_ParseTerrainParameter);
			}
			else
			{
				CONS_Alert(CONS_ERROR, "No Terrain type name.\n");
				valid = false;
			}
		}
		else if (stricmp(tkn, "floor") == 0 || stricmp(tkn, "texture") == 0)
		{
			Z_Free(tkn);
			tkn = M_GetToken(NULL);
			pos = M_GetTokenPos();
			
			if (tkn && pos <= size)
			{
				if (stricmp(tkn, "optional") == 0)
				{
					// "optional" is ZDoom syntax
					// We don't use it, but we can ignore it.
					Z_Free(tkn);
					tkn = M_GetToken(NULL);
					pos = M_GetTokenPos();
				}

				if (tkn && pos <= size)
				{
					t_floor_t *f = NULL;

					tknHash = quickncasehash(tkn, 8);

					for (i = 0; i < numTerrainFloorDefs; i++)
					{
						f = &terrainFloorDefs[i];

						if (f->textureHash == tknHash && !strncmp(tkn, f->textureName, 8))
						{
							break;
						}
					}

					if (i == numTerrainFloorDefs)
					{
						K_NewTerrainFloorDefs();
						f = &terrainFloorDefs[i];

						strncpy(f->textureName, tkn, 8);
						f->textureHash = tknHash;
					}

					Z_Free(tkn);
					tkn = M_GetToken(NULL);
					pos = M_GetTokenPos();

					if (tkn && pos <= size)
					{
						terrain_t *t = K_GetTerrainByName(tkn);

						if (t == NULL)
						{
							CONS_Alert(CONS_ERROR, "Invalid Terrain type '%s'.\n", tkn);
							valid = false;
						}
						else
						{
							f->terrainID = K_GetTerrainHeapIndex(t);
							CONS_Printf("Texture '%s' set to Terrain '%s'\n", f->textureName, tkn);

							INT32 tex = R_CheckTextureNumForName(f->textureName);
							if (tex != -1)
							{
								textures[tex]->terrainID = f->terrainID;
							}
						}
					}
					else
					{
						CONS_Alert(CONS_ERROR, "No terrain for floor definition.\n");
						valid = false;
					}
				}
				else
				{
					CONS_Alert(CONS_ERROR, "No texture for floor definition.\n");
					valid = false;
				}
			}
			else
			{
				CONS_Alert(CONS_ERROR, "No texture for floor definition.\n");
				valid = false;
			}
		}
		else if (stricmp(tkn, "defaultTerrain") == 0)
		{
			Z_Free(tkn);
			tkn = M_GetToken(NULL);
			pos = M_GetTokenPos();

			if (tkn && pos <= size)
			{
				terrain_t *t = NULL;

				tknHash = quickncasehash(tkn, TERRAIN_NAME_LEN);

				for (i = 0; i < numTerrainDefs; i++)
				{
					t = &terrainDefs[i];

					if (tknHash == t->hash && !strncmp(tkn, t->name, TERRAIN_NAME_LEN))
					{
						break;
					}
				}

				if (i == numTerrainDefs)
				{
					CONS_Alert(CONS_ERROR, "Invalid DefaultTerrain type.\n");
					valid = false;
				}
				else
				{
					defaultTerrain = i;
					CONS_Printf("DefaultTerrain set to '%s'\n", tkn);
				}
			}
			else
			{
				CONS_Alert(CONS_ERROR, "No DefaultTerrain type.\n");
				valid = false;
			}
		}
		else if (stricmp(tkn, "defaultOffroadFootstep") == 0)
		{
			Z_Free(tkn);
			tkn = M_GetToken(NULL);
			pos = M_GetTokenPos();

			if (tkn && pos <= size)
			{
				t_footstep_t *fs = NULL;

				tknHash = quickncasehash(tkn, TERRAIN_NAME_LEN);

				for (i = 0; i < numFootstepDefs; i++)
				{
					fs = &footstepDefs[i];

					if (tknHash == fs->hash && !strncmp(tkn, fs->name, TERRAIN_NAME_LEN))
					{
						break;
					}
				}

				if (i == numFootstepDefs)
				{
					CONS_Alert(CONS_ERROR, "Invalid DefaultOffroadFootstep type.\n");
					valid = false;
				}
				else
				{
					defaultOffroadFootstep = i;
					CONS_Printf("DefaultOffroadFootstep set to '%s'\n", tkn);
				}
			}
			else
			{
				CONS_Alert(CONS_ERROR, "No DefaultOffroadFootstep type.\n");
				valid = false;
			}
		}
		else
		{
			CONS_Alert(CONS_ERROR, "Unknown field '%s' found in TERRAIN lump.\n", tkn);
			valid = false;
		}

		Z_Free(tkn);

		if (valid == false)
		{
			return false;
		}

		tkn = M_GetToken(NULL);
	}

	Z_Free(tkn);
	return true;
}

/*--------------------------------------------------
	void K_InitTerrain(UINT16 wadNum)

		See header file for description.
--------------------------------------------------*/
void K_InitTerrain(UINT16 wadNum)
{
	UINT16 lumpNum;
	lumpinfo_t *lump_p = wadfiles[wadNum]->lumpinfo;

	// Iterate through all lumps and compare the name individually.
	// In PK3 files, you can potentially have multiple TERRAIN differentiated by
	// their file extension.
	for (lumpNum = 0; lumpNum < wadfiles[wadNum]->numlumps; lumpNum++, lump_p++)
	{
		UINT8 *data;

		if (memcmp(lump_p->name, "TERRAIN", 8) != 0)
		{
			continue;
		}

		data = (UINT8 *)W_CacheLumpNumPwad(wadNum, lumpNum, PU_STATIC);

		// If that didn't exist, we have nothing to do here.
		if (data == NULL)
		{
			continue;
		}
		else
		{
			size_t size = W_LumpLengthPwad(wadNum, lumpNum);
			char *datacopy;

			size_t nameLength = strlen(wadfiles[wadNum]->filename) + 1 + strlen(lump_p->fullname); // length of file name, '|', and lump name
			char *name = malloc(nameLength + 1);

			sprintf(name, "%s|%s", wadfiles[wadNum]->filename, lump_p->fullname);
			name[nameLength] = '\0';

			size = W_LumpLengthPwad(wadNum, lumpNum);

			CONS_Printf(M_GetText("Loading TERRAIN from %s\n"), name);

			datacopy = (char *)Z_Malloc((size+1)*sizeof(char),PU_STATIC,NULL);
			memmove(datacopy,data,size);
			datacopy[size] = '\0';

			Z_Free(data);

			K_TERRAINLumpParser(datacopy, size);

			Z_Free(datacopy);

			free(name);
		}
	}

	R_ClearTextureNumCache(false);
}
