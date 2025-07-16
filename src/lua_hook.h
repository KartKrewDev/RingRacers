// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2022 by Sonic Team Junior.
// Copyright (C) 2016 by John "JTE" Muniz.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hook.h
/// \brief hooks for Lua scripting

#ifndef __LUA_HOOK_H__
#define __LUA_HOOK_H__

#include "r_defs.h"
#include "d_player.h"
#include "s_sound.h"
#include "d_event.h"

#include "lua_hudlib_drawlist.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
Do you know what an 'X Macro' is? Such a macro is called over each element of
a list and expands the input. I use it for the hook lists because both an enum
and array of hook names need to be kept in order. The X Macro handles this
automatically.
*/

#define MOBJ_HOOK_LIST(X) \
	X (MobjSpawn),/* P_SpawnMobj */\
	X (MobjCollide),/* PIT_CheckThing */\
	X (MobjLineCollide),/* ditto */\
	X (MobjMoveCollide),/* tritto */\
	X (TouchSpecial),/* P_TouchSpecialThing */\
	X (MobjFuse),/* when mobj->fuse runs out */\
	X (MobjThinker),/* P_MobjThinker, P_SceneryThinker */\
	X (BossThinker),/* P_GenericBossThinker */\
	X (ShouldDamage),/* P_DamageMobj (Should mobj take damage?) */\
	X (MobjDamage),/* P_DamageMobj (Mobj actually takes damage!) */\
	X (MobjDeath),/* P_KillMobj */\
	X (BossDeath),/* A_BossDeath */\
	X (MobjRemoved),/* P_RemoveMobj */\
	X (BotRespawn),/* B_CheckRespawn */\
	X (MobjMoveBlocked),/* P_XYMovement (when movement is blocked) */\
	X (MapThingSpawn),/* P_SpawnMapThing */\
	X (FollowMobj),/* P_PlayerAfterThink Smiles mobj-following */\

#define HOOK_LIST(X) \
	X (NetVars),/* add to archive table (netsave) */\
	X (MapChange),/* (before map load) */\
	X (MapLoad),\
	X (PlayerJoin),/* Got_AddPlayer */\
	X (PreThinkFrame)/* frame (before mobj and player thinkers) */,\
	X (ThinkFrame),/* frame (after mobj and player thinkers) */\
	X (PostThinkFrame),/* frame (at end of tick, ie after overlays, precipitation, specials) */\
	X (JumpSpecial),/* P_DoJumpStuff (Any-jumping) */\
	X (AbilitySpecial),/* P_DoJumpStuff (Double-jumping) */\
	X (SpinSpecial),/* P_DoSpinAbility (Spin button effect) */\
	X (JumpSpinSpecial),/* P_DoJumpStuff (Spin button effect (mid-air)) */\
	X (BotTiccmd),/* B_BuildTiccmd */\
	X (PlayerMsg),/* chat messages */\
	X (HurtMsg),/* imhurttin */\
	X (PlayerSpawn),/* G_SpawnPlayer */\
	X (ShieldSpawn),/* P_SpawnShieldOrb */\
	X (ShieldSpecial),/* shield abilities */\
	X (PlayerCanDamage),/* P_PlayerCanDamage */\
	X (PlayerQuit),\
	X (IntermissionThinker),/* Y_Ticker */\
	X (TeamSwitch),/* team switching in... uh... *what* speak, spit it the fuck out */\
	X (ViewpointSwitch),/* spy mode (no trickstabs) */\
	X (SeenPlayer),/* MT_NAMECHECK */\
	X (PlayerThink),/* P_PlayerThink */\
	X (GameQuit),\
	X (PlayerCmd),/* building the player's ticcmd struct */\
	X (VoteThinker),/* Y_VoteTicker */\
	X (PreFillItemRoulette),/* K_FillItemRouletteData, before attempted reel build */\
	X (FillItemRoulette),/* K_FillItemRouletteData, after built reel is in place */\

#define STRING_HOOK_LIST(X) \
	X (SpecialExecute),\

#define HUD_HOOK_LIST(X) \
	X (game),\
	X (scores),/* emblems/multiplayer list */\
	X (title),/* titlescreen */\
	X (titlecard),\
	X (intermission),\

/*
I chose to access the hook enums through a macro as well. This could provide
a hint to lookup the macro's definition instead of the enum's definition.
(Since each enumeration is not defined in the source code, but by the list
macros above, it is not greppable.) The name passed to the macro can also be
grepped and found in the lists above.
*/

#define   MOBJ_HOOK(name)   mobjhook_ ## name
#define        HOOK(name)       hook_ ## name
#define    HUD_HOOK(name)    hudhook_ ## name
#define STRING_HOOK(name) stringhook_ ## name

#define ENUM(X) enum { X ## _LIST (X)  X(MAX) }

ENUM   (MOBJ_HOOK);
ENUM        (HOOK);
ENUM    (HUD_HOOK);
ENUM (STRING_HOOK);

#undef ENUM

/* dead simple, LUA_HOOK(GameQuit) */
#define LUA_HOOK(type) LUA_HookVoid(HOOK(type))
//#define LUA_HUDHOOK(type) LUA_HookHUD(HUD_HOOK(type))

extern boolean hook_cmd_running;

void LUA_HookVoid(int hook);
void LUA_HookHUD(huddrawlist_h, int hook);

int  LUA_HookMobj(mobj_t *, int hook);
int  LUA_Hook2Mobj(mobj_t *, mobj_t *, int hook);
void LUA_HookInt(INT32 integer, int hook);
void LUA_HookBool(boolean value, int hook);
int  LUA_HookPlayer(player_t *, int hook);
int  LUA_HookTiccmd(player_t *, ticcmd_t *, int hook);
int  LUA_HookKey(event_t *event, int hook); // Hooks for key events

void LUA_HookThinkFrame(void);
int  LUA_HookMobjLineCollide(mobj_t *, line_t *);
int  LUA_HookTouchSpecial(mobj_t *special, mobj_t *toucher);
int  LUA_HookShouldDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype);
int  LUA_HookMobjDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype);
int  LUA_HookMobjDeath(mobj_t *target, mobj_t *inflictor, mobj_t *source, UINT8 damagetype);
int  LUA_HookMobjMoveBlocked(mobj_t *, mobj_t *, line_t *);
void LUA_HookSpecialExecute(activator_t *activator, INT32 *args, char **stringargs);
int  LUA_HookPlayerMsg(int source, int target, int flags, char *msg, int mute);
int  LUA_HookHurtMsg(player_t *, mobj_t *inflictor, mobj_t *source, UINT8 damagetype);
int  LUA_HookMapThingSpawn(mobj_t *, mapthing_t *);
int  LUA_HookFollowMobj(player_t *, mobj_t *);
int  LUA_HookPlayerCanDamage(player_t *, mobj_t *);
void LUA_HookPlayerQuit(player_t *, kickreason_t);
//int  LUA_HookTeamSwitch(player_t *, int newteam, boolean fromspectators, boolean tryingautobalance, boolean tryingscramble);
int  LUA_HookViewpointSwitch(player_t *player, player_t *newdisplayplayer, boolean forced);
int  LUA_HookSeenPlayer(player_t *player, player_t *seenfriend);
int  LUA_HookPreFillItemRoulette(player_t *player, itemroulette_t *const roulette, boolean ringbox);
int  LUA_HookFillItemRoulette(player_t *player, itemroulette_t *const roulette, boolean ringbox);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __LUA_HOOK_H__
