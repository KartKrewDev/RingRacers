/// \file  k_collide.c
/// \brief SRB2Kart item collision hooks

#include "k_collide.h"
#include "doomtype.h"
#include "p_mobj.h"
#include "k_kart.h"
#include "p_local.h"
#include "s_sound.h"
#include "r_main.h" // R_PointToAngle2, R_PointToDist2
#include "hu_stuff.h" // Sink snipe print
#include "doomdef.h" // Sink snipe print
#include "g_game.h" // Sink snipe print

boolean K_OrbinautJawzCollide(mobj_t *t1, mobj_t *t2)
{
	boolean damageitem = false;

	if (((t1->target == t2) || (t1->target == t2->target)) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if ((t1->type == MT_ORBINAUT_SHIELD || t1->type == MT_JAWZ_SHIELD) && t1->lastlook
		&& (t2->type == MT_ORBINAUT_SHIELD || t2->type == MT_JAWZ_SHIELD) && t2->lastlook
		&& (t1->target == t2->target)) // Don't hit each other if you have the same target
		return true;

	if (t2->player)
	{
		if (t2->player->powers[pw_flashing]
			&& !(t1->type == MT_ORBINAUT || t1->type == MT_JAWZ || t1->type == MT_JAWZ_DUD))
			return true;

		if (t2->player->kartstuff[k_hyudorotimer])
			return true; // no interaction

		// Player Damage
		P_DamageMobj(t2, t1, t1->target, 1);
		K_KartBouncing(t2, t1, false, false);
		S_StartSound(t2, sfx_s3k7b);

		damageitem = true;
	}
	else if (t2->type == MT_ORBINAUT || t2->type == MT_JAWZ || t2->type == MT_JAWZ_DUD
		|| t2->type == MT_ORBINAUT_SHIELD || t2->type == MT_JAWZ_SHIELD
		|| t2->type == MT_BANANA || t2->type == MT_BANANA_SHIELD
		|| t2->type == MT_BALLHOG)
	{
		// Other Item Damage
		if (t2->eflags & MFE_VERTICALFLIP)
			t2->z -= t2->height;
		else
			t2->z += t2->height;

		S_StartSound(t2, t2->info->deathsound);
		P_KillMobj(t2, t1, t1);

		P_SetObjectMomZ(t2, 8*FRACUNIT, false);
		P_InstaThrust(t2, R_PointToAngle2(t1->x, t1->y, t2->x, t2->y)+ANGLE_90, 16*FRACUNIT);

		P_SpawnMobj(t2->x/2 + t1->x/2, t2->y/2 + t1->y/2, t2->z/2 + t1->z/2, MT_ITEMCLASH);

		damageitem = true;
	}
	else if (t2->type == MT_SSMINE_SHIELD || t2->type == MT_SSMINE)
	{
		damageitem = true;
		// Bomb death
		P_KillMobj(t2, t1, t1);
	}
	else if (t2->flags & MF_SPRING && (t1->type != MT_ORBINAUT_SHIELD && t1->type != MT_JAWZ_SHIELD))
	{
		// Let thrown items hit springs!
		P_DoSpring(t2, t1);
	}
	else if (t2->flags & MF_SHOOTABLE)
	{
		// Shootable damage
		P_DamageMobj(t2, t2, t1->target, 1);
		damageitem = true;
	}

	if (damageitem)
	{
		// This Item Damage
		if (t1->eflags & MFE_VERTICALFLIP)
			t1->z -= t1->height;
		else
			t1->z += t1->height;

		S_StartSound(t1, t1->info->deathsound);
		P_KillMobj(t1, t2, t2);

		P_SetObjectMomZ(t1, 8*FRACUNIT, false);
		P_InstaThrust(t1, R_PointToAngle2(t2->x, t2->y, t1->x, t1->y)+ANGLE_90, 16*FRACUNIT);
	}

	return true;
}

boolean K_BananaBallhogCollide(mobj_t *t1, mobj_t *t2)
{
	boolean damageitem = false;

	if (((t1->target == t2) || (t1->target == t2->target)) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if (((t1->type == MT_BANANA_SHIELD) && (t2->type == MT_BANANA_SHIELD))
		&& (t1->target == t2->target)) // Don't hit each other if you have the same target
		return true;

	if (t1->type == MT_BALLHOG && t2->type == MT_BALLHOG)
		return true; // Ballhogs don't collide with eachother

	if (t2->player)
	{
		if (t2->player->powers[pw_flashing])
			return true;

		// Banana snipe!
		if (t1->type == MT_BANANA && t1->health > 1)
			S_StartSound(t2, sfx_bsnipe);

		// Player Damage
		K_SpinPlayer(t2->player, t1->target, 0, t1, (t1->type == MT_BANANA || t1->type == MT_BANANA_SHIELD));

		damageitem = true;
	}
	else if (t2->type == MT_BANANA || t2->type == MT_BANANA_SHIELD
		|| t2->type == MT_ORBINAUT || t2->type == MT_ORBINAUT_SHIELD
		|| t2->type == MT_JAWZ || t2->type == MT_JAWZ_DUD || t2->type == MT_JAWZ_SHIELD
		|| t2->type == MT_BALLHOG)
	{
		// Other Item Damage
		if (t2->eflags & MFE_VERTICALFLIP)
			t2->z -= t2->height;
		else
			t2->z += t2->height;

		S_StartSound(t2, t2->info->deathsound);
		P_KillMobj(t2, t1, t1);

		P_SetObjectMomZ(t2, 8*FRACUNIT, false);
		P_InstaThrust(t2, R_PointToAngle2(t1->x, t1->y, t2->x, t2->y)+ANGLE_90, 16*FRACUNIT);

		P_SpawnMobj(t2->x/2 + t1->x/2, t2->y/2 + t1->y/2, t2->z/2 + t1->z/2, MT_ITEMCLASH);

		damageitem = true;
	}
	else if (t2->flags & MF_SHOOTABLE)
	{
		// Shootable damage
		P_DamageMobj(t2, t2, t1->target, 1);
		damageitem = true;
	}

	if (damageitem)
	{
		// This Item Damage
		if (t1->eflags & MFE_VERTICALFLIP)
			t1->z -= t1->height;
		else
			t1->z += t1->height;

		S_StartSound(t1, t1->info->deathsound);
		P_KillMobj(t1, t2, t2);

		P_SetObjectMomZ(t1, 8*FRACUNIT, false);
		P_InstaThrust(t1, R_PointToAngle2(t2->x, t2->y, t1->x, t1->y)+ANGLE_90, 16*FRACUNIT);
	}

	return true;
}

boolean K_EggItemCollide(mobj_t *t1, mobj_t *t2)
{
	// Push fakes out of other item boxes
	if (t2->type == MT_RANDOMITEM || t2->type == MT_EGGMANITEM)
		P_InstaThrust(t1, R_PointToAngle2(t2->x, t2->y, t1->x, t1->y), t2->radius/4);

	if (t1->type == MT_EGGMANITEM && t2->player)
		P_TouchSpecialThing(t1, t2, false);

	return true;
}

boolean K_MineCollide(mobj_t *t1, mobj_t *t2)
{
	if (((t1->target == t2) || (t1->target == t2->target)) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t1->health <= 0 || t2->health <= 0)
		return true;

	if (t2->player)
	{
		if (t2->player->powers[pw_flashing])
			return true;

		// Bomb punting
		if ((t1->state >= &states[S_SSMINE1] && t1->state <= &states[S_SSMINE4])
			|| (t1->state >= &states[S_SSMINE_DEPLOY8] && t1->state <= &states[S_SSMINE_DEPLOY13]))
			P_KillMobj(t1, t2, t2);
		else
			K_PuntMine(t1, t2);
	}
	else if (t2->type == MT_ORBINAUT || t2->type == MT_JAWZ || t2->type == MT_JAWZ_DUD
		|| t2->type == MT_ORBINAUT_SHIELD || t2->type == MT_JAWZ_SHIELD)
	{
		// Bomb death
		P_KillMobj(t1, t2, t2);

		// Other Item Damage
		if (t2->eflags & MFE_VERTICALFLIP)
			t2->z -= t2->height;
		else
			t2->z += t2->height;

		S_StartSound(t2, t2->info->deathsound);
		P_KillMobj(t2, t1, t1);

		P_SetObjectMomZ(t2, 8*FRACUNIT, false);
		P_InstaThrust(t2, R_PointToAngle2(t1->x, t1->y, t2->x, t2->y)+ANGLE_90, 16*FRACUNIT);
	}
	else if (t2->flags & MF_SHOOTABLE)
	{
		// Bomb death
		P_KillMobj(t1, t2, t2);
		// Shootable damage
		P_DamageMobj(t2, t2, t1->target, 1);
	}

	return true;
}

boolean K_MineExplosionCollide(mobj_t *t1, mobj_t *t2)
{
	if (t2->player)
	{
		if (t2->player->powers[pw_flashing])
			return true;

		if (t1->state == &states[S_MINEEXPLOSION1])
			K_ExplodePlayer(t2->player, t1->target, t1);
		else
			K_SpinPlayer(t2->player, t1->target, 0, t1, false);
	}
	else if (t2->flags & MF_SHOOTABLE)
	{
		// Shootable damage
		P_DamageMobj(t2, t2, t1->target, 1);
	}

	return true;
}

boolean K_KitchenSinkCollide(mobj_t *t1, mobj_t *t2)
{
	if (((t1->target == t2) || (t1->target == t2->target)) && (t1->threshold > 0 || (t2->type != MT_PLAYER && t2->threshold > 0)))
		return true;

	if (t2->player)
	{
		if (t2->player->powers[pw_flashing])
			return true;

		S_StartSound(NULL, sfx_bsnipe); // let all players hear it.
		HU_SetCEchoFlags(0);
		HU_SetCEchoDuration(5);
		HU_DoCEcho(va("%s\\was hit by a kitchen sink.\\\\\\\\", player_names[t2->player-players]));
		I_OutputMsg("%s was hit by a kitchen sink.\n", player_names[t2->player-players]);
		P_DamageMobj(t2, t1, t1->target, 10000);
		P_KillMobj(t1, t2, t2);
	}
	else if (t2->flags & MF_SHOOTABLE)
	{
		// Shootable damage
		P_KillMobj(t2, t2, t1->target);
		// This item damage
		P_KillMobj(t1, t2, t2);
	}

	return true;
}

boolean K_FallingRockCollide(mobj_t *t1, mobj_t *t2)
{
	if (t2->player || t2->type == MT_FALLINGROCK)
		K_KartBouncing(t2, t1, false, false);
	return true;
}

boolean K_SMKIceBlockCollide(mobj_t *t1, mobj_t *t2)
{
	if (!(t2->flags & MF_SOLID || t2->flags & MF_SHOOTABLE || t2->flags & MF_BOUNCE))
		return true;

	if (!(t2->health))
		return true;

	if (t2->type == MT_BANANA || t2->type == MT_BANANA_SHIELD
		|| t2->type == MT_EGGMANITEM || t2->type == MT_EGGMANITEM_SHIELD
		|| t2->type == MT_SSMINE || t2->type == MT_SSMINE_SHIELD
		|| t2->type == MT_ORBINAUT_SHIELD || t2->type == MT_JAWZ_SHIELD)
		return false;

	if (t1->health)
		P_KillMobj(t1, t2, t2);

	/*if (t2->player && (t2->player->kartstuff[k_invincibilitytimer] > 0
		|| t2->player->kartstuff[k_growshrinktimer] > 0))
		return true;*/

	K_KartBouncing(t2, t1, false, true);
	return false;
}
