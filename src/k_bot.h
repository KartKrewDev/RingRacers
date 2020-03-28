// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2007-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  b_bot.h
/// \brief Basic bot handling

void K_AddBots(UINT8 numbots);
boolean K_PlayerUsesBotMovement(player_t *player);
void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd);
