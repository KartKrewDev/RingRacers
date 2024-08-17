// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Vivian "toastergrl" Grannell.
// Copyright (C) 2024 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  objects/servant-hand.c
/// \brief Servant Hand direction pointer

#include "../doomdef.h"
#include "../p_local.h"
#include "../p_mobj.h"
#include "../d_player.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../v_video.h"
#include "../r_main.h"
#include "../g_game.h"

void Obj_ServantHandSpawning(player_t *player)
{
	if (player->pflags & PF_WRONGWAY || player->pflags & PF_POINTME)
	{
		if (player->handtimer < TICRATE)
		{
			player->handtimer++;
			if (player->hand == NULL && player->handtimer == TICRATE)
			{	
				fixed_t heightOffset = player->mo->height + 30*mapobjectscale;
				if (P_IsObjectFlipped(player->mo))
				{
					// This counteracts the offset added by K_FlipFromObject so it looks seamless from non-flipped.
					heightOffset += player->mo->height - FixedMul(player->mo->scale, player->mo->height);
					heightOffset *= P_MobjFlip(player->mo); // Fleep.
				}
				
				mobj_t *hand = P_SpawnMobj(
					player->mo->x,
					player->mo->y,
					player->mo->z + heightOffset,
					MT_SERVANTHAND
				);

				if (hand)
				{
					K_FlipFromObject(hand, player->mo);
					hand->old_z = hand->z;

					P_SetTarget(&hand->target, player->mo);
					P_SetTarget(&player->hand, hand);

					hand->fuse = 8;
				}
			}
		}
	}
	else if (player->handtimer != 0)
	{
		player->handtimer--;
	}
}

void Obj_ServantHandThink(mobj_t *hand)
{
	UINT8 handtimer = 0;
	player_t *player = NULL;

	if (P_MobjWasRemoved(hand->target))
	{
		P_RemoveMobj(hand);
		return;
	}

	if (hand->target->health && hand->target->player && hand->target->player->hand == hand)
	{
		player = hand->target->player;
		handtimer = hand->target->player->handtimer;
	}

	{
		const fixed_t handpokespeed = 4;
		const fixed_t looping = handpokespeed - abs((hand->threshold % (handpokespeed*2)) - handpokespeed);
		fixed_t xoffs = 0, yoffs = 0;

		if (hand->fuse != 0)
		{
			;
		}
		else if (looping != 0)
		{
			xoffs = FixedMul(2 * looping * mapobjectscale, FINECOSINE(hand->angle >> ANGLETOFINESHIFT)),
			yoffs = FixedMul(2 * looping * mapobjectscale, FINESINE(hand->angle >> ANGLETOFINESHIFT)),

			hand->threshold++;
		}
		else if (handtimer == 0)
		{
			hand->fuse = 8;
		}
		else
		{
			hand->threshold++;
		}

		if (hand->fuse != 0)
		{
			if ((hand->fuse > 4) ^ (handtimer < TICRATE/2))
			{
				hand->spritexscale = FRACUNIT/3;
				hand->spriteyscale = 3*FRACUNIT;
			}
			else
			{
				hand->spritexscale = 2*FRACUNIT;
				hand->spriteyscale = FRACUNIT/2;
			}
		}

		if (player != NULL)
		{
			hand->color = player->skincolor;
			hand->angle = player->besthanddirection;
			
			fixed_t heightOffset = player->mo->height + 30*mapobjectscale;
			if (P_IsObjectFlipped(player->mo))
				heightOffset *= P_MobjFlip(player->mo); // Fleep.

			K_FlipFromObject(hand, player->mo);
			P_MoveOrigin(hand,
				player->mo->x + xoffs,
				player->mo->y + yoffs,
				player->mo->z + heightOffset
			);

			hand->sprzoff = player->mo->sprzoff;

			hand->renderflags &= ~RF_DONTDRAW;
			hand->renderflags |= (RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(player));
		}
	}
}

void Obj_PointPlayersToXY(fixed_t x, fixed_t y)
{
	for(int i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator || !players[i].mo)
			continue;

		angle_t angletotarget = R_PointToAngle2(
			players[i].mo->x, players[i].mo->y,
			x, y);
		players[i].pflags |= PF_POINTME;
		players[i].besthanddirection = angletotarget;
	}
}
