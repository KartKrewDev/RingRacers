/// \brief Battle mode power-up code

#include "k_kart.h"
#include "k_objects.h"
#include "k_powerup.h"

tic_t K_PowerUpRemaining(const player_t* player, kartitems_t powerup)
{
	switch (powerup)
	{
	case POWERUP_SMONITOR:
		return player->powerup.superTimer;

	case POWERUP_SUPERFLICKY:
		return Obj_SuperFlickySwarmTime(player->powerup.flickyController);

	default:
		return 0u;
	}
}

void K_GivePowerUp(player_t* player, kartitems_t powerup, tic_t time)
{
	switch (powerup)
	{
	case POWERUP_SMONITOR:
		K_DoInvincibility(player, time);
		player->powerup.superTimer += time;
		break;

	case POWERUP_SUPERFLICKY:
		if (K_PowerUpRemaining(player, POWERUP_SUPERFLICKY))
		{
			Obj_ExtendSuperFlickySwarm(player->powerup.flickyController, time);
		}
		else
		{
			Obj_SpawnSuperFlickySwarm(player, time);
		}
		break;

	default:
		break;
	}
}

void K_DropPowerUps(player_t* player)
{
	auto simple_drop = [player](kartitems_t powerup, auto& timer)
	{
		tic_t remaining = K_PowerUpRemaining(player, powerup);

		if (remaining)
		{
			K_DropPaperItem(player, powerup, remaining);
			timer = 0;
		}
	};

	simple_drop(POWERUP_SMONITOR, player->powerup.superTimer);

	if (K_PowerUpRemaining(player, POWERUP_SUPERFLICKY))
	{
		mobj_t* swarm = player->powerup.flickyController;

		// Be sure to measure the remaining time before ending the power-up
		K_DropPaperItem(player, POWERUP_SUPERFLICKY, Obj_SuperFlickySwarmTime(swarm));

		Obj_EndSuperFlickySwarm(swarm);
	}
}
