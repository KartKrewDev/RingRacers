// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by James Robert Roman.
// Copyright (C) 2024 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  hyudoro.c
/// \brief Hyudoro item code.

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../k_roulette.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../g_game.h"
#include "../k_hitlag.h"
#include "../p_slopes.h"

enum {
	HYU_PATROL,
	HYU_RETURN,
	HYU_HOVER,
	HYU_ORBIT,
};

// TODO: make these general functions

static fixed_t
K_GetSpeed (mobj_t *mobj)
{
	return FixedHypot(mobj->momx, mobj->momy);
}

#define hyudoro_mode(o) ((o)->extravalue1)
#define hyudoro_itemtype(o) ((o)->movefactor)
#define hyudoro_itemcount(o) ((o)->movecount)
#define hyudoro_hover_stack(o) ((o)->threshold)
#define hyudoro_next(o) ((o)->tracer)
#define hyudoro_stackpos(o) ((o)->reactiontime)

// cannot be combined
#define hyudoro_center(o) ((o)->target)
#define hyudoro_target(o) ((o)->target)

#define hyudoro_stolefrom(o) ((o)->hnext)
#define hyudoro_capsule(o) ((o)->hprev)
#define hyudoro_timer(o) ((o)->movedir)

#define hyudoro_center_max_radius(o) ((o)->threshold)
#define hyudoro_center_master(o) ((o)->target)

#define HYU_VISUAL_HEIGHT (24)

static angle_t
trace_angle (mobj_t *hyu)
{
	mobj_t *center = hyu->target;

	if (hyu->x != center->x || hyu->y != center->y)
	{
		return R_PointToAngle2(
				center->x, center->y, hyu->x, hyu->y);
	}
	else
		return hyu->angle;
}

static angle_t
get_look_angle (mobj_t *thing)
{
	player_t *player = thing->player;

	return player ? player->angleturn : thing->angle;
}

static boolean
is_hyudoro (mobj_t *thing)
{
	return !P_MobjWasRemoved(thing) &&
		thing->type == MT_HYUDORO;
}

static mobj_t *
get_hyudoro_master (mobj_t *hyu)
{
	mobj_t *center = hyudoro_center(hyu);

	return center ? hyudoro_center_master(center) : NULL;
}

static player_t *
get_hyudoro_target_player (mobj_t *hyu)
{
	mobj_t *target = hyudoro_target(hyu);

	return target ? target->player : NULL;
}

static void
sine_bob
(		mobj_t * hyu,
		angle_t a,
		fixed_t sineofs)
{
	hyu->sprzoff = FixedMul(HYU_VISUAL_HEIGHT * hyu->scale,
			sineofs + FINESINE(a >> ANGLETOFINESHIFT)) * P_MobjFlip(hyu);
			
	if (P_IsObjectFlipped(hyu))
		hyu->sprzoff -= hyu->height;
}

static void
bob_in_place
(		mobj_t * hyu,
		INT32 bob_speed)
{
	sine_bob(hyu,
			(leveltime & (bob_speed - 1)) *
			(ANGLE_MAX / bob_speed), -(3*FRACUNIT/4));
}

static void
reset_shadow (mobj_t *hyu)
{
	hyu->shadowcolor = 15;
	hyu->whiteshadow = true;
}

static void
project_hyudoro (mobj_t *hyu)
{
	mobj_t *center = hyudoro_center(hyu);

	angle_t angleStep = FixedMul(5 * ANG1,
			FixedDiv(hyudoro_center_max_radius(center),
				center->radius));

	angle_t angle = trace_angle(hyu) + angleStep;

	fixed_t d = center->radius;

	fixed_t x = P_ReturnThrustX(center, angle, d);
	fixed_t y = P_ReturnThrustY(center, angle, d);

	hyu->momx = (center->x + x) - hyu->x;
	hyu->momy = (center->y + y) - hyu->y;
	hyu->angle = angle + ANGLE_90;

	sine_bob(hyu, angle, FRACUNIT);

	hyu->z = P_GetZAt(center->standingslope, hyu->x, hyu->y,
			P_GetMobjGround(center));
}

static void
rise_thru_stack (mobj_t *hyu)
{
	mobj_t *target = hyudoro_target(hyu);

	fixed_t spacer = ((target->height / 2) +
			(HYU_VISUAL_HEIGHT * hyu->scale * 2));

	fixed_t sink = hyudoro_stackpos(hyu) * spacer;

	fixed_t zofs = abs(hyu->momz);
	fixed_t d = (zofs - sink);
	fixed_t speed = d / 8;

	if (abs(d) < abs(speed))
		zofs = sink;
	else
		zofs -= speed;

	hyu->momz = zofs * P_MobjFlip(target);
}

static void
project_hyudoro_hover (mobj_t *hyu)
{
	mobj_t *target = hyudoro_target(hyu);

	// Turns a bit toward its target
	angle_t ang = get_look_angle(target) + ANGLE_67h;
	fixed_t rad = (target->radius * 2) + hyu->radius;

	P_MoveOrigin(hyu,
			target->x - P_ReturnThrustX(hyu, ang, rad),
			target->y - P_ReturnThrustY(hyu, ang, rad),
			target->z);

	// Cancel momentum from HYU_RETURN.
	// (And anything else! I don't trust this game!!)
	hyu->momx = 0;
	hyu->momy = 0;

	rise_thru_stack(hyu);

	hyu->angle = ang;

	// copies sprite tilting
	hyu->pitch = target->pitch;
	hyu->roll = target->roll;

	bob_in_place(hyu, 64);
}

static boolean
project_hyudoro_orbit (mobj_t *hyu)
{
	mobj_t *orbit = hyudoro_target(hyu);

	if (P_MobjWasRemoved(orbit))
	{
		return false;
	}

	P_MoveOrigin(hyu, orbit->x, orbit->y, orbit->z);
	hyu->destscale = orbit->scale;

	mobj_t *facing = orbit->target;

	if (!P_MobjWasRemoved(facing))
	{
		hyu->angle = R_PointToAngle2(
				hyu->x, hyu->y, facing->x, facing->y);
	}

	return true;
}

static mobj_t *
find_duel_target (mobj_t *ignore)
{
	mobj_t *ret = NULL;
	UINT8 bestPosition = UINT8_MAX;
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *player = NULL;

		if (playeringame[i] == false)
		{
			continue;
		}

		player = &players[i];
		if (player->spectator || player->exiting)
		{
			continue;
		}

		if (!player->mo || P_MobjWasRemoved(player->mo))
		{
			continue;
		}

		if (ignore != NULL && player->mo == ignore)
		{
			continue;
		}

		if (player->position < bestPosition)
		{
			ret = player->mo;
			bestPosition = player->position;

			if (bestPosition <= 1)
			{
				// Can't get any lower
				break;
			}
		}
	}

	return ret;
}

static void
do_confused (mobj_t *hyu)
{
	// Hyudoro is confused.
	// Spin around, try to find a new target.

	// Try to find new target
	P_SetTarget(&hyudoro_target(hyu),
		find_duel_target(hyudoro_stolefrom(hyu)));

	// Spin in circles
	hyu->angle += ANGLE_45;

	// Bob very fast
	bob_in_place(hyu, 32);

	hyu->sprzoff += HYU_VISUAL_HEIGHT * hyu->scale;
}

static void
move_to_player (mobj_t *hyu)
{
	mobj_t *target = hyudoro_target(hyu);

	angle_t angle;
	fixed_t speed;

	if (!target || P_MobjWasRemoved(target))
	{
		do_confused(hyu);
		return;
	}

	angle = R_PointToAngle2(
			hyu->x, hyu->y, target->x, target->y);

	speed = (hyu->radius / 2) +
		max(hyu->radius, K_GetSpeed(target));

	// For first place only: cap hyudoro speed at 50%
	// target player's kart speed
	if (target->player && target->player->position == 1)
	{
		const fixed_t normalspeed =
			K_GetKartSpeed(target->player, false, false) / 2;

		speed = min(speed, normalspeed);
	}

	P_InstaThrust(hyu, angle, speed);

	hyu->z = target->z; // stay level with target
	hyu->angle = angle;

	hyu->color = target->color;
}

static void
deliver_item (mobj_t *hyu)
{
	/* set physical position to visual position in stack */
	hyu->z += hyu->momz;
	hyu->momz = 0;

	mobj_t *emerald = P_SpawnMobjFromMobj(
			hyu, 0, 0, 0, MT_EMERALD);

	/* only want emerald for its orbiting behavior, so make
	   it invisible */
	P_SetMobjState(emerald, S_INVISIBLE);

	Obj_BeginEmeraldOrbit(
			emerald, hyudoro_target(hyu), 0, 64, 128);

	/* See Obj_GiveEmerald. I won't risk relying on the
	   Hyudoro object in case it is removed first. So go
	   through the capsule instead. */
	Obj_SetEmeraldAwardee(emerald, hyudoro_capsule(hyu));

	/* hyudoro will teleport to emerald (orbit the player) */
	hyudoro_mode(hyu) = HYU_ORBIT;
	P_SetTarget(&hyudoro_target(hyu), emerald);

	hyu->renderflags &= ~(RF_DONTDRAW | RF_BLENDMASK);
	reset_shadow(hyu);
}

static void
append_hyudoro
(		mobj_t ** head,
		mobj_t * hyu)
{
	INT32 lastpos = 0;

	while (is_hyudoro(*head))
	{
		lastpos = hyudoro_stackpos(*head);
		head = &hyudoro_next(*head);
	}

	hyudoro_stackpos(hyu) = lastpos + 1;
	P_SetTarget(head, hyu);

	/* only first in list gets a shadow */
	if (lastpos == 0)
	{
		reset_shadow(hyu);
	}
	else
	{
		hyu->shadowcolor = 31;/* black - hide it */
	}
}

static void
pop_hyudoro (mobj_t **head)
{
	mobj_t *hyu = *head;

	if (!is_hyudoro(hyu))
	{
		return;
	}

	INT32 lastpos = hyudoro_stackpos(hyu);

	{
		mobj_t *next = hyudoro_next(hyu);

		P_SetTarget(head, next);
		P_SetTarget(&hyudoro_next(hyu), NULL);

		hyu = next;
	}

	if (!is_hyudoro(hyu))
	{
		return;
	}

	reset_shadow(hyu);/* show it */

	do
	{
		INT32 thispos = hyudoro_stackpos(hyu);

		hyudoro_stackpos(hyu) = lastpos;
		lastpos = thispos;

		hyu = hyudoro_next(hyu);
	}
	while (is_hyudoro(hyu));
}

static mobj_t *
spawn_capsule (mobj_t *hyu)
{
	mobj_t *caps = P_SpawnMobjFromMobj(
			hyu, 0, 0, 0, MT_ITEMCAPSULE);

	/* hyudoro only needs its own shadow */
	caps->shadowscale = 0;

	caps->flags |=
		MF_NOGRAVITY |
		MF_NOCLIP |
		MF_NOCLIPTHING |
		MF_NOCLIPHEIGHT;

	/* signal that this item capsule always puts items in the HUD */
	caps->flags2 |= MF2_STRONGBOX;

	P_SetTarget(&hyudoro_capsule(hyu), caps);

	/* capsule teleports to hyudoro */
	P_SetTarget(&caps->target, hyu);

	/* so it looks like hyudoro is holding it */
	caps->sprzoff = 20 * hyu->scale;

	return caps;
}

static void
update_capsule_position (mobj_t *hyu)
{
	mobj_t *caps = hyudoro_capsule(hyu);

	if (P_MobjWasRemoved(caps))
		return;

	caps->extravalue1 = hyu->scale / 3;

	/* hold it in the hyudoro's hands */
	const fixed_t r = hyu->radius;
	caps->sprxoff = FixedMul(r, FCOS(hyu->angle));
	caps->spryoff = FixedMul(r, FSIN(hyu->angle));
}

static void
set_item
(		mobj_t * hyu,
		INT32 item,
		INT32 amount)
{
	mobj_t *caps = P_MobjWasRemoved(hyudoro_capsule(hyu))
		? spawn_capsule(hyu) : hyudoro_capsule(hyu);

	hyudoro_itemtype(hyu) = item;
	hyudoro_itemcount(hyu) = amount;

	caps->threshold = hyudoro_itemtype(hyu);
	caps->movecount = hyudoro_itemcount(hyu);
}

static void
hyudoro_set_held_item_from_player
(		mobj_t * hyu,
		player_t * player)
{
	if (K_ItemEnabled(KITEM_KITCHENSINK))
	{
		boolean convert = false;

		switch (player->itemtype)
		{
			// The following permits a case-by-case balancing for items
			// we don't want ending up in 2nd place's hands too often...
			case KITEM_SPB:
				convert = true;
				break;
			default:
				break;
		}

		if (convert == true)
		{
			if (player->itemtype > 0 && player->itemtype < NUMKARTITEMS)
			{
				// A conversion has occoured, this is no longer on the
				// playing field... someone else must manifest it!?
				itemCooldowns[player->itemtype - 1] = 0;
			}

			set_item(hyu, KITEM_KITCHENSINK, 1);

			return;
		}
	}

	set_item(hyu, player->itemtype, player->itemamount);
}

static boolean
hyudoro_patrol_hit_player
(		mobj_t * hyu,
		mobj_t * toucher)
{
	player_t *player = toucher->player;

	mobj_t *center = hyudoro_center(hyu);

	mobj_t *master = NULL;

	if (!player)
		return false;

	// Cannot hit its master
	master = get_hyudoro_master(hyu);
	if (toucher == master)
		return false;

	// Don't punish a punished player
	if (player->hyudorotimer)
		return false;

	player->pflags |= PF_CASTSHADOW;

	// NO ITEM?
	if (!player->itemamount)
		return false;

	K_AddHitLag(toucher, TICRATE/2, false);

	hyudoro_mode(hyu) = HYU_RETURN;

	hyudoro_set_held_item_from_player(hyu, player);

	if (!P_MobjWasRemoved(hyudoro_capsule(hyu)))
	{
		hyudoro_capsule(hyu)->extravalue2 = player->skincolor;
	}

	K_StripItems(player);

	S_StartSound(toucher, sfx_s3k92);

	/* do not make 1st place invisible */
	if (player->position != 1)
	{
		player->hyudorotimer = hyudorotime;
	}

	player->stealingtimer = hyudorotime;

	P_SetTarget(&hyudoro_stolefrom(hyu), toucher);

	if (master == NULL || P_MobjWasRemoved(master))
	{
		// if master is NULL, it is probably a DUEL
		master = find_duel_target(toucher);
	}

	P_SetTarget(&hyudoro_target(hyu), master);

	if (center)
		P_RemoveMobj(center);

	hyu->renderflags &= ~(RF_DONTDRAW);

	// Reset shadow to default (after alt_shadow)
	reset_shadow(hyu);

	// This will flicker the shadow
	hyudoro_timer(hyu) = 18;

	P_SetMobjState(hyu, S_HYUDORO_RETURNING);

	return true;
}

static boolean
award_immediately (mobj_t *hyu)
{
	player_t *player = get_hyudoro_target_player(hyu);

	if (player)
	{
		if (player->position == 1)
		{
			return false;
		}

		if (!P_CanPickupItem(player, 1))
			return false;

		// Prevent receiving any more items or even stacked
		// Hyudoros! Put on a timer so roulette cannot become
		// locked permanently.
		player->itemRoulette.reserved = 2*TICRATE;
	}

	deliver_item(hyu);

	return true;
}

static boolean
hyudoro_return_hit_player
(		mobj_t * hyu,
		mobj_t * toucher)
{
	if (toucher != hyudoro_target(hyu))
		return false;

	// If the player already has an item, just hover beside
	// them until they use/lose it.
	if (!award_immediately(hyu))
	{
		S_StartSound(hyudoro_target(hyu), sfx_kc3d);
		hyudoro_mode(hyu) = HYU_HOVER;
		append_hyudoro(&toucher->player->hoverhyudoro, hyu);
	}

	return true;
}

static boolean
hyudoro_hover_await_stack (mobj_t *hyu)
{
	player_t *player = get_hyudoro_target_player(hyu);

	if (!player)
		return false;

	// First in stack goes first
	if (hyu != player->hoverhyudoro)
		return false;

	if (!award_immediately(hyu))
		return false;

	pop_hyudoro(&player->hoverhyudoro);

	return true;
}

static void
trail_ghosts
(		mobj_t * hyu,
		boolean colorize)
{
	// Spawns every other frame
	if (leveltime & 1)
	{
		return;
	}

	mobj_t *ghost = P_SpawnGhostMobj(hyu);

	// Flickers every frame
	ghost->extravalue1 = 1;
	ghost->extravalue2 = 2;

	// copy per-splitscreen-player visibility
	ghost->renderflags =
		(hyu->renderflags & RF_DONTDRAW);

	ghost->colorized = colorize;

	ghost->tics = 8;

	P_SetTarget(&ghost->tracer, hyu);
}

static void
trail_glow (mobj_t *hyu)
{
	mobj_t *ghost = P_SpawnGhostMobj(hyu);

	// Flickers every frame
	ghost->extravalue1 = 1;
	ghost->extravalue2 = 0;

	ghost->renderflags = RF_ADD | RF_TRANS80;

	ghost->tics = 2; // this actually does last one tic

	ghost->momz = hyu->momz; // copy stack position
}

static void
blend_hover_hyudoro (mobj_t *hyu)
{
	player_t *player = get_hyudoro_target_player(hyu);

	hyu->renderflags &= ~(RF_BLENDMASK);

	if (!player)
	{
		return;
	}

	/* 1st place: Hyudoro stack is unusable, so make a visual
	   indication */
	if (player->position == 1)
	{
		hyu->renderflags |= RF_MODULATE;
		trail_glow(hyu);
	}
}

static void
alt_shadow (mobj_t *hyu)
{
	/* spaced out pulse, fake randomness */
	switch (leveltime % (7 + ((leveltime / 8) % 3)))
	{
		default:
			hyu->shadowcolor = 15;
			hyu->whiteshadow = false;
			break;
		case 1:
			hyu->shadowcolor = 5;
			hyu->whiteshadow = true;
			break;
		case 2:
			hyu->shadowcolor = 181;
			hyu->whiteshadow = true;
			break;
		case 3:
			hyu->shadowcolor = 255;
			hyu->whiteshadow = true;
			break;
	}
}

void
Obj_InitHyudoroCenter (mobj_t * center, mobj_t * master)
{
	mobj_t *hyu = P_SpawnMobjFromMobj(
			center, 0, 0, 0, MT_HYUDORO);

	// This allows a Lua override
	if (!hyudoro_center_max_radius(center))
	{
		hyudoro_center_max_radius(center) =
			128 * center->scale;
	}

	center->radius = hyu->radius;

	hyu->angle = center->angle;
	P_SetTarget(&hyudoro_center(hyu), center);
	P_SetTarget(&hyudoro_center_master(center), master);

	hyudoro_mode(hyu) = HYU_PATROL;

	// Set splitscreen player visibility
	hyu->renderflags |= RF_DONTDRAW;
	if (master && !P_MobjWasRemoved(master) && master->player)
	{
		hyu->renderflags &= ~(K_GetPlayerDontDrawFlag(master->player));
	}

	Obj_SpawnFakeShadow(hyu); // this sucks btw
}

void
Obj_HyudoroDeploy (mobj_t *master)
{
	mobj_t *center = P_SpawnMobjFromMobj(
			master, 0, 0, 0, MT_HYUDORO_CENTER);

	center->angle = master->angle;
	Obj_InitHyudoroCenter(center, master);

	// Update floorz to the correct position by indicating
	// that it should be recalculated by P_MobjThinker.
	center->floorz = master->z;
	center->ceilingz = master->z + master->height;
	center->z = P_GetMobjGround(center) - P_MobjFlip(center);

	S_StartSound(master, sfx_s3k92); // scary ghost noise
}

void
Obj_HyudoroThink (mobj_t *hyu)
{
	switch (hyudoro_mode(hyu))
	{
		case HYU_PATROL:
			if (hyudoro_center(hyu))
				project_hyudoro(hyu);

			trail_ghosts(hyu, false);
			alt_shadow(hyu);
			break;

		case HYU_RETURN:
			move_to_player(hyu);
			trail_ghosts(hyu, true);

			if (hyudoro_timer(hyu) > 0)
				hyu->whiteshadow = !hyu->whiteshadow;
			break;

		case HYU_HOVER:
			if (hyudoro_target(hyu))
			{
				project_hyudoro_hover(hyu);

				if (hyudoro_hover_await_stack(hyu))
					break;
			}
			blend_hover_hyudoro(hyu);
			break;

		case HYU_ORBIT:
			if (!project_hyudoro_orbit(hyu))
			{
				P_RemoveMobj(hyu);
				return;
			}
			break;
	}

	update_capsule_position(hyu);

	if (hyudoro_timer(hyu) > 0)
		hyudoro_timer(hyu)--;
}

void
Obj_HyudoroCenterThink (mobj_t *center)
{
	fixed_t max_radius = hyudoro_center_max_radius(center);

	if (center->radius < max_radius)
		center->radius += max_radius / 64;
}

void
Obj_HyudoroCollide
(		mobj_t * hyu,
		mobj_t * toucher)
{
	switch (hyudoro_mode(hyu))
	{
		case HYU_PATROL:
			hyudoro_patrol_hit_player(hyu, toucher);
			break;

		case HYU_RETURN:
			hyudoro_return_hit_player(hyu, toucher);
			break;
	}
}

boolean
Obj_HyudoroShadowZ
(		mobj_t * hyu,
		fixed_t * return_z,
		pslope_t ** return_slope)
{
	if (hyudoro_mode(hyu) != HYU_PATROL)
		return false;

	if (P_MobjWasRemoved(hyudoro_center(hyu)))
		return false;

	*return_z = hyu->z;
	*return_slope = hyudoro_center(hyu)->standingslope;

	return true;
}
