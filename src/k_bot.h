// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2007-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_bot.h
/// \brief Basic bot handling

#include "k_waypoint.h"
#include "d_player.h"

#define MAXBOTDIFFICULTY 9

// Path that bot will attempt to take
typedef struct botprediction_s {
	fixed_t x, y;
	fixed_t radius;
	angle_t dir;
} botprediction_t;

void K_AddBots(SINT8 numbots);
boolean K_PlayerUsesBotMovement(player_t *player);
boolean K_BotCanTakeCut(player_t *player);
fixed_t K_BotRubberband(player_t *player);
void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd);
