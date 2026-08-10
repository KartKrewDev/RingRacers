// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_hud.h
/// \brief HUD drawing functions exclusive to Kart

#ifndef K_HUD_H
#define K_HUD_H

#include "doomtype.h"
#include "doomstat.h"
#include "hu_stuff.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RINGANIM_NUMFRAMES 10
#define RINGANIM_DELAYMAX 5

#define POS_DELAY_TIME 10

#define MARGINLEVELS 24

#define HUDTRANS_CAMHEIGHT_MAX (120*FRACUNIT) // The camera height past this point where hud transparency should take affect

extern int32_t MINI_X, MINI_Y;

struct trackingResult_t
{
	fixed_t x, y;
	fixed_t scale;
	dboolean onScreen;
	int32_t angle, pitch;
	fixed_t fov;
};

typedef struct position_t
{
	fixed_t x, y;
} position_t;

void K_ObjectTracking(trackingResult_t *result, const vector3_t *point, dboolean reverse);

tic_t K_TranslateTimer(tic_t drawtime, uint8_t mode, int32_t *return_jitter);

const char *K_GetItemPatch(uint8_t item, dboolean tiny);
void K_LoadKartHUDGraphics(void);
void K_drawKartHUD(void);
void K_drawKartFreePlay(void);
void K_drawKartPowerUps(void);
void K_drawSpectatorHUD(dboolean director);
void K_drawKartTimestamp(tic_t drawtime, int32_t TX, int32_t TY, int32_t splitflags, uint8_t mode);
int32_t K_drawKartMicroTime(const char *todrawtext, int32_t workx, int32_t worky, int32_t splitflags);
void K_drawKart2PTimestamp(void);
void K_drawKart4PTimestamp(void);
void K_drawEmeraldWin(dboolean overlay);
void K_DrawMapThumbnail2(fixed_t x, fixed_t y, fixed_t width, uint32_t flags, uint16_t map, const uint8_t *colormap, fixed_t accordion);
#define K_DrawMapThumbnail(x, y, w, f, m, c) K_DrawMapThumbnail2(x, y, w, f, m, c, FRACUNIT)
void K_DrawLikeMapThumbnail(fixed_t x, fixed_t y, fixed_t width, uint32_t flags, patch_t *patch, const uint8_t *colormap, fixed_t accordion);
void K_DrawMapAsFace(int32_t x, int32_t y, uint32_t flags, uint16_t map, const uint8_t *colormap, fixed_t accordion, int32_t unit);
void K_drawTargetHUD(const vector3_t *origin, player_t *player);
void K_drawButton(fixed_t x, fixed_t y, int32_t flags, patch_t *button[2], dboolean pressed);
void K_drawButtonAnim(int32_t x, int32_t y, int32_t flags, patch_t *button[2], tic_t animtic);
void K_DrawSticker(int32_t x, int32_t y, int32_t width, int32_t flags, dboolean isSmall);
void K_DrawMarginSticker(int32_t x, int32_t y, int32_t width, int32_t flags, dboolean isSmall, dboolean leftedge);
int32_t K_GetTransFlagFromFixed(fixed_t value, dboolean midrace);

void K_DrawKartPositionNumXY(
	uint8_t num,
	uint8_t splitIndex,
	fixed_t fx, fixed_t fy, fixed_t scale, int32_t fflags,
	tic_t counter, dboolean subtract,
	dboolean exit, dboolean lastLap, dboolean losing
);

void K_DrawInputDisplay(float x, float y, int32_t flags, char mode, uint8_t pid, dboolean local, dboolean transparent);

extern patch_t *kp_capsuletarget_arrow[2][2];
extern patch_t *kp_capsuletarget_icon[2];
extern patch_t *kp_capsuletarget_far[2][2];
extern patch_t *kp_capsuletarget_far_text[2];
extern patch_t *kp_capsuletarget_near[2][8];

extern patch_t *kp_superflickytarget[2][4];

extern patch_t *kp_spraycantarget_far[2][6];
extern patch_t *kp_spraycantarget_near[2][6];

extern patch_t *kp_autoroulette;
extern patch_t *kp_autoring;

extern patch_t *kp_button_a[2][2];
extern patch_t *kp_button_b[2][2];
extern patch_t *kp_button_c[2][2];
extern patch_t *kp_button_x[2][2];
extern patch_t *kp_button_y[2][2];
extern patch_t *kp_button_z[2][2];
extern patch_t *kp_button_start[2][2];
extern patch_t *kp_button_l[2][2];
extern patch_t *kp_button_r[2][2];
extern patch_t *kp_button_up[2][2];
extern patch_t *kp_button_down[2][2];
extern patch_t *kp_button_right[2][2];
extern patch_t *kp_button_left[2][2];
extern patch_t *kp_button_lua1[2][2];
extern patch_t *kp_button_lua2[2][2];
extern patch_t *kp_button_lua3[2][2];

extern patch_t *gen_button_a[2][2];
extern patch_t *gen_button_b[2][2];
extern patch_t *gen_button_x[2][2];
extern patch_t *gen_button_y[2][2];
extern patch_t *gen_button_lb[2][2];
extern patch_t *gen_button_rb[2][2];
extern patch_t *gen_button_lt[2][2];
extern patch_t *gen_button_rt[2][2];
extern patch_t *gen_button_start[2][2];
extern patch_t *gen_button_back[2][2];
extern patch_t *gen_button_ls[2][2];
extern patch_t *gen_button_rs[2][2];
extern patch_t *gen_button_dpad[2][2];

extern patch_t *gen_button_keyleft[2];
extern patch_t *gen_button_keyright[2];
extern patch_t *gen_button_keycenter[2];

extern patch_t *gen_button_keyleft[2];
extern patch_t *gen_button_keyright[2];
extern patch_t *gen_button_keycenter[2];

extern patch_t *kp_eggnum[6];
extern patch_t *kp_facenum[MAXPLAYERS+1];

extern patch_t *kp_pickmeup[2];

extern patch_t *kp_unknownminimap;

void K_AddMessage(const char *msg, dboolean interrupt, dboolean persist);
void K_AddMessageForPlayer(player_t *player, const char *msg, dboolean interrupt, dboolean persist);
void K_ClearPersistentMessages(void);
void K_ClearPersistentMessageForPlayer(player_t *player);
void K_TickMessages(void);

patch_t *K_GetSmallStaticCachedItemPatch(kartitems_t item);

typedef enum
{
	PLAYERTAG_NONE,
	PLAYERTAG_LOCAL,
	PLAYERTAG_CPU,
	PLAYERTAG_RIVAL,
	PLAYERTAG_NAME,
	PLAYERTAG_TUTORIALFAKELOCAL,
}
playertagtype_t;

playertagtype_t K_WhichPlayerTag(player_t *p);
void K_DrawPlayerTag(fixed_t x, fixed_t y, player_t *p, playertagtype_t type, dboolean foreground);

int32_t K_GetMinimapTransFlags(const dboolean usingProgressBar);
int32_t K_GetMinimapSplitFlags(const dboolean usingProgressBar);
position_t K_GetKartObjectPosToMinimapPos(fixed_t objx, fixed_t objy);

int32_t K_DrawGameControl(uint16_t x, uint16_t y, uint8_t player, const char *str, uint8_t alignment, uint8_t font, uint32_t flags);

void K_drawKartTeamScores(dboolean fromintermission, int32_t interoffset);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
