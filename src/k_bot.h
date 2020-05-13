#ifndef __K_BOT__
#define __K_BOT__

#include "k_waypoint.h"
#include "d_player.h"

#define MAXBOTDIFFICULTY 9

// Path that bot will attempt to take
typedef struct botprediction_s {
	fixed_t x, y;
	fixed_t radius;
	angle_t dir;
} botprediction_t;

boolean K_AddBot(UINT8 skin, UINT8 difficulty, UINT8 *newplayernum);
void K_UpdateMatchRaceBots(void);
boolean K_PlayerUsesBotMovement(player_t *player);
boolean K_BotCanTakeCut(player_t *player);
fixed_t K_BotRubberband(player_t *player);
void K_BuildBotTiccmd(player_t *player, ticcmd_t *cmd);

#endif
