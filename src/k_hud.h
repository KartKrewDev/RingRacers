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

#ifndef __K_HUD__
#define __K_HUD__

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

extern INT32 MINI_X, MINI_Y;

struct trackingResult_t
{
	fixed_t x, y;
	fixed_t scale;
	boolean onScreen;
	INT32 angle, pitch;
	fixed_t fov;
};

typedef struct position_t
{
	fixed_t x, y;
} position_t;

void K_ObjectTracking(trackingResult_t *result, const vector3_t *point, boolean reverse);

tic_t K_TranslateTimer(tic_t drawtime, UINT8 mode, INT32 *return_jitter);

const char *K_GetItemPatch(UINT8 item, boolean tiny);
void K_LoadKartHUDGraphics(void);
void K_drawKartHUD(void);
void K_drawKartFreePlay(void);
void K_drawKartPowerUps(void);
void K_drawSpectatorHUD(boolean director);
void K_drawKartTimestamp(tic_t drawtime, INT32 TX, INT32 TY, INT32 splitflags, UINT8 mode);
INT32 K_drawKartMicroTime(const char *todrawtext, INT32 workx, INT32 worky, INT32 splitflags);
void K_drawKart2PTimestamp(void);
void K_drawKart4PTimestamp(void);
void K_drawEmeraldWin(boolean overlay);
void K_DrawMapThumbnail2(fixed_t x, fixed_t y, fixed_t width, UINT32 flags, UINT16 map, const UINT8 *colormap, fixed_t accordion);
#define K_DrawMapThumbnail(x, y, w, f, m, c) K_DrawMapThumbnail2(x, y, w, f, m, c, FRACUNIT)
void K_DrawLikeMapThumbnail(fixed_t x, fixed_t y, fixed_t width, UINT32 flags, patch_t *patch, const UINT8 *colormap, fixed_t accordion);
void K_DrawMapAsFace(INT32 x, INT32 y, UINT32 flags, UINT16 map, const UINT8 *colormap, fixed_t accordion, INT32 unit);
void K_drawTargetHUD(const vector3_t *origin, player_t *player);
void K_drawButton(fixed_t x, fixed_t y, INT32 flags, patch_t *button[2], boolean pressed);
void K_drawButtonAnim(INT32 x, INT32 y, INT32 flags, patch_t *button[2], tic_t animtic);
void K_DrawSticker(INT32 x, INT32 y, INT32 width, INT32 flags, boolean isSmall);
void K_DrawMarginSticker(INT32 x, INT32 y, INT32 width, INT32 flags, boolean isSmall, boolean leftedge);
INT32 K_GetTransFlagFromFixed(fixed_t value, boolean midrace);

void K_DrawKartPositionNumXY(
	UINT8 num,
	UINT8 splitIndex,
	fixed_t fx, fixed_t fy, fixed_t scale, INT32 fflags,
	tic_t counter, boolean subtract,
	boolean exit, boolean lastLap, boolean losing
);

void K_DrawInputDisplay(float x, float y, INT32 flags, char mode, UINT8 pid, boolean local, boolean transparent);

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

void K_AddMessage(const char *msg, boolean interrupt, boolean persist);
void K_AddMessageForPlayer(player_t *player, const char *msg, boolean interrupt, boolean persist);
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
void K_DrawPlayerTag(fixed_t x, fixed_t y, player_t *p, playertagtype_t type, boolean foreground);

INT32 K_GetMinimapTransFlags(const boolean usingProgressBar);
INT32 K_GetMinimapSplitFlags(const boolean usingProgressBar);
position_t K_GetKartObjectPosToMinimapPos(fixed_t objx, fixed_t objy);

INT32 K_DrawGameControl(UINT16 x, UINT16 y, UINT8 player, const char *str, UINT8 alignment, UINT8 font, UINT32 flags);

void K_drawKartTeamScores(boolean fromintermission, INT32 interoffset);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
