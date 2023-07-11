#include "../doomdef.h"
#include "../p_local.h"
#include "../p_mobj.h"
#include "../d_player.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../v_video.h"
#include "../r_main.h"
#include "../g_game.h"

void Obj_ServantHandHandling(player_t *player)
{
	if (player->pflags & PF_WRONGWAY || player->pflags & PF_POINTME)
	{
		if (player->handtimer < TICRATE)
		{
			player->handtimer++;
			if (player->hand == NULL && player->handtimer == TICRATE)
			{
				mobj_t *hand = P_SpawnMobj(
					player->mo->x,
					player->mo->y,
					player->mo->z + player->mo->height + 30*mapobjectscale,
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

		if (player->hand)
		{
			player->hand->destscale = mapobjectscale;
		}
	}
	else if (player->handtimer != 0)
	{
		player->handtimer--;
	}

	if (player->hand)
	{
		const fixed_t handpokespeed = 4;
		const fixed_t looping = handpokespeed - abs((player->hand->threshold % (handpokespeed*2)) - handpokespeed);
		fixed_t xoffs = 0, yoffs = 0;

		player->hand->color = player->skincolor;
		player->hand->angle = player->besthanddirection;

		if (player->hand->fuse != 0)
		{
			;
		}
		else if (looping != 0)
		{
			xoffs = FixedMul(2 * looping * mapobjectscale, FINECOSINE(player->hand->angle >> ANGLETOFINESHIFT)),
			yoffs = FixedMul(2 * looping * mapobjectscale, FINESINE(player->hand->angle >> ANGLETOFINESHIFT)),

			player->hand->threshold++;
		}
		else if (player->handtimer == 0)
		{
			player->hand->fuse = 8;
		}
		else
		{
			player->hand->threshold++;
		}

		if (player->hand->fuse != 0)
		{
			if ((player->hand->fuse > 4) ^ (player->handtimer < TICRATE/2))
			{
				player->hand->spritexscale = FRACUNIT/3;
				player->hand->spriteyscale = 3*FRACUNIT;
			}
			else
			{
				player->hand->spritexscale = 2*FRACUNIT;
				player->hand->spriteyscale = FRACUNIT/2;
			}
		}

		P_MoveOrigin(player->hand,
			player->mo->x + xoffs,
			player->mo->y + yoffs,
			player->mo->z + player->mo->height + 30*mapobjectscale
		);
		K_FlipFromObject(player->hand, player->mo);

		player->hand->sprzoff = player->mo->sprzoff;

		player->hand->renderflags &= ~RF_DONTDRAW;
		player->hand->renderflags |= (RF_DONTDRAW & ~K_GetPlayerDontDrawFlag(player));
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
