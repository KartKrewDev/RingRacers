// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2018 by ZarroTsu.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_kart.h
/// \brief SRB2kart stuff.

#ifndef __K_KART__
#define __K_KART__

#include "doomdef.h"
#include "d_player.h" // Need for player_t
#include "command.h" // Need for player_t

#ifdef __cplusplus
extern "C" {
#endif

#define KART_FULLTURN 800

/*
Number of animations for the invincibility sparkles
If states are ever added or removed
Make sure this matches the actual number of states
*/
#define KART_NUMINVSPARKLESANIM 12

#define GROW_SCALE (2*FRACUNIT)
#define SHRINK_SCALE (FRACUNIT/2)

#define GROW_PHYSICS_SCALE (3*FRACUNIT/2)
#define SHRINK_PHYSICS_SCALE (3*FRACUNIT/4)

#define INSTAWHIP_DURATION (12)
#define INSTAWHIP_CHARGETIME (3*TICRATE/4)
#define INSTAWHIP_RINGDRAINEVERY (TICRATE/2)
#define INSTAWHIP_HOLD_DELAY (TICRATE*2)
// MUST be longer or equal to INSTAWHIP_CHARGETIME.
#define INSTAWHIP_TETHERBLOCK (3*TICRATE/4)
#define PUNISHWINDOW (G_CompatLevel(0x0010) ? 7*TICRATE/10 : 10*TICRATE/10)

#define BAIL_MAXCHARGE (84) // tics to bail when in painstate nad in air, on ground is half, if you touch this, also update Obj_BailChargeThink synced animation logic
#define BAIL_DROP (FRACUNIT) // How many rings it has to drop before stun starts
#define BAIL_BOOST (6*FRACUNIT/5) // How fast bail itself is
#define BAIL_CREDIT_DEBTRINGS (true)
#define BAIL_DROPFREQUENCY (2) // How quickly the rings spill out
#define BAILSTUN (TICRATE*6) // The fixed length of stun after baildrop is over

#define MAXCOMBOTHRUST (mapobjectscale*20)
#define MAXCOMBOFLOAT (mapobjectscale*10)
#define MINCOMBOTHRUST (mapobjectscale*2)
#define MINCOMBOFLOAT (mapobjectscale*1)
#define MAXCOMBOTIME (TICRATE*4)

#define STARTBOOST_DURATION (125)

#define TIMEATTACK_START (TICRATE*10)

#define LIGHTNING_CHARGE (TICRATE*2)
#define LIGHTNING_SOUND (sfx_s3k84)

#define OVERDRIVE_STARTUP (0)

#define AMPLEVEL (15)

#define FLAMESHIELD_MAX (120)

#define BALLHOG_BURST_FUSE (TICRATE*2)

#define RR_PROJECTILE_FUSE (8*TICRATE)

#define SCAMDIST (2000)

#define EARLY_ITEM_FLICKER (NUMTRANSMAPS)

#define TRIPWIRE_OK_SOUND (sfx_sonbo2)
#define TRIPWIRE_NG_SOUND (sfx_gshaf)

// 2023-08-26 +ang20 to Sal's OG values to make them friendlier - Tyron
#define STUMBLE_STEEP_VAL (ANG60 + ANG20)
#define STUMBLE_STEEP_VAL_AIR (ANG30 + ANG10 + ANG20)
#define STUMBLE_AIRTIME TICRATE

#define MAXRINGVOLUME 255
#define MAXRINGTRANSPARENCY 255
#define MINRINGVOLUME 100
#define MINRINGTRANSPARENCY 100
#define RINGVOLUMECOLLECTPENALTY 3
#define RINGTRANSPARENCYCOLLECTPENALTY 0
#define RINGVOLUMEUSEPENALTY 15
#define RINGTRANSPARENCYUSEPENALTY 15
#define RINGVOLUMEREGEN 1
#define RINGTRANSPARENCYREGEN 3

#define FAILSAFETIME (4*TICRATE)

#define DUELOVERTIME (cv_dueltimelimit.value)
#define DUELWINNINGSCORE (cv_duelscorelimit.value)

#define MIN_WAVEDASH_CHARGE ((11*TICRATE/16)*9)

#define MAXTOPACCEL (12*FRACUNIT)
#define TOPACCELREGEN (FRACUNIT/16)

#define BUBBLESCAM (4*FRACUNIT)

// Handling boosts and sliptide conditions got weird.
// You must be under a handling boost of at least SLIPTIDEHANDLING to sliptide.
// HANDLESCALING is used to adjust all handling boosts simultaneously (weight factors in the future?)
// If you need to touch this in an involved way later, please just make sliptide eligibility a flag LMAO
#define HANDLESCALING (7*FRACUNIT/8)
#define SLIPTIDEHANDLING (HANDLESCALING/2)

// Mispredicted turns can generate phantom sliptide inputs for a few tics.
// Delay the wavedash visuals until we're reasonably sure that it's a deliberate turn.
#define HIDEWAVEDASHCHARGE (60)

// Auto-respawn timer for when lap cheating or out of bounds
// is detected.
#define AUTORESPAWN_TIME (10*TICRATE)
#define AUTORESPAWN_THRESHOLD (7*TICRATE)

UINT8 K_SetPlayerItemAmount(player_t *player, INT32 amount);
UINT8 K_SetPlayerBackupItemAmount(player_t *player, INT32 amount);
UINT8 K_AdjustPlayerItemAmount(player_t *player, INT32 amount);
UINT8 K_AdjustPlayerBackupItemAmount(player_t *player, INT32 amount);

angle_t K_ReflectAngle(angle_t angle, angle_t against, fixed_t maxspeed, fixed_t yourspeed);

void K_PopBubbleShield(player_t *player);

boolean K_IsDuelItem(mobjtype_t type);
boolean K_DuelItemAlwaysSpawns(mapthing_t *mt);
boolean K_InRaceDuel(void);
player_t *K_DuelOpponent(player_t *player);

fixed_t K_FinalCheckpointPower(void);
fixed_t K_EffectiveGradingFactor(const player_t *player);
#define MINGRADINGFACTOR (FRACUNIT/2)
#define MINFRANTICFACTOR (8*FRACUNIT/10)

void K_TimerReset(void);
void K_TimerInit(void);

UINT32 K_GetPlayerDontDrawFlag(player_t *player);
void K_ReduceVFXForEveryone(mobj_t *mo);

boolean K_IsPlayerLosing(player_t *player);
fixed_t K_PlayerScamPercentage(const player_t *player, fixed_t mult);
fixed_t K_GetKartGameSpeedScalar(SINT8 value);

INT32 K_GetShieldFromItem(INT32 item);
SINT8 K_ItemResultToType(SINT8 getitem);
UINT8 K_ItemResultToAmount(SINT8 getitem, const itemroulette_t *roulette);
tic_t K_GetItemCooldown(SINT8 itemResult);
void K_SetItemCooldown(SINT8 itemResult, tic_t time);
void K_RunItemCooldowns(void);

boolean K_TimeAttackRules(void);
boolean K_CapsuleTimeAttackRules(void);

fixed_t K_GetMobjWeight(mobj_t *mobj, mobj_t *against);
void K_PlayerJustBumped(player_t *player);
boolean K_KartBouncing(mobj_t *mobj1, mobj_t *mobj2);
boolean K_KartSolidBounce(mobj_t *bounceMobj, mobj_t *solidMobj);
void K_KartPainEnergyFling(player_t *player);
void K_MatchFlipFlags(mobj_t *mo, mobj_t *master);
void K_FlipFromObject(mobj_t *mo, mobj_t *master);
void K_FlipFromObjectNoInterp(mobj_t *mo, mobj_t *master);
void K_MatchGenericExtraFlags(mobj_t *mo, mobj_t *master);
void K_MatchGenericExtraFlagsNoInterp(mobj_t *mo, mobj_t *master);
void K_MatchGenericExtraFlagsNoZAdjust(mobj_t *mo, mobj_t *master);
void K_SpawnDashDustRelease(player_t *player);
void K_SpawnDriftBoostClip(player_t *player);
void K_SpawnDriftBoostClipSpark(mobj_t *clip);
void K_SpawnNormalSpeedLines(player_t *player);
void K_SpawnGardenTopSpeedLines(player_t *player);
void K_SpawnInvincibilitySpeedLines(mobj_t *mo);
void K_SpawnBumpEffect(mobj_t *mo);
void K_KartMoveAnimation(player_t *player);
void K_KartPlayerHUDUpdate(player_t *player);
void K_KartResetPlayerColor(player_t *player);
boolean K_PressingEBrake(const player_t *player);
void K_KartPlayerThink(player_t *player, ticcmd_t *cmd);
void K_KartPlayerAfterThink(player_t *player);
angle_t K_MomentumAngleEx(const mobj_t *mo, const fixed_t threshold);
angle_t K_MomentumAngleReal(const mobj_t *mo);
#define K_MomentumAngle(mo) K_MomentumAngleEx(mo, 6 * mo->scale)
boolean K_PvPAmpReward(UINT32 award, player_t *attacker, player_t *defender);
void K_SpawnAmps(player_t *player, UINT8 amps, mobj_t *impact);
void K_SpawnEXP(player_t *player, UINT8 exp, mobj_t *impact);
void K_AwardPlayerAmps(player_t *player, UINT8 amps);
void K_CheckpointCrossAward(player_t *player);
void K_AwardPlayerRings(player_t *player, UINT16 rings, boolean overload);
boolean K_Overdrive(player_t *player);
boolean K_DefensiveOverdrive(player_t *player);
void K_DoInstashield(player_t *player);
void K_DoPowerClash(mobj_t *t1, mobj_t *t2);
void K_DoGuardBreak(mobj_t *t1, mobj_t *t2);
void K_BattleAwardHit(player_t *player, player_t *victim, mobj_t *inflictor, UINT8 bumpersRemoved);
void K_RemoveGrowShrink(player_t *player);
boolean K_IsBigger(mobj_t *compare, mobj_t *other);
void K_SpinPlayer(player_t *player, mobj_t *inflictor, mobj_t *source, INT32 type);
void K_TumblePlayer(player_t *player, mobj_t *inflictor, mobj_t *source, boolean soften);
void K_TumbleInterrupt(player_t *player);
angle_t K_StumbleSlope(angle_t angle, angle_t pitch, angle_t roll);
void K_StumblePlayer(player_t *player);
boolean K_CheckStumble(player_t *player, angle_t oldPitch, angle_t oldRoll, boolean fromAir);
void K_InitStumbleIndicator(player_t *player);
void K_InitWavedashIndicator(player_t *player);
void K_InitTrickIndicator(player_t *player);
void K_UpdateStumbleIndicator(player_t *player);
void K_UpdateWavedashIndicator(player_t *player);
void K_UpdateTrickIndicator(player_t *player);
INT32 K_ExplodePlayer(player_t *player, mobj_t *inflictor, mobj_t *source);
void K_DebtStingPlayer(player_t *player, mobj_t *source);
void K_GiveBumpersToPlayer(player_t *player, player_t *victim, UINT8 amount);
void K_TakeBumpersFromPlayer(player_t *player, player_t *victim, UINT8 amount);
void K_GivePointsToPlayer(player_t *player, player_t *victim, UINT8 amount);
void K_MineFlashScreen(mobj_t *source);
void K_SpawnMineExplosion(mobj_t *source, skincolornum_t color, tic_t delay);
void K_SpawnLandMineExplosion(mobj_t *source, skincolornum_t color, tic_t delay);
void K_RunFinishLineBeam(void);
UINT16 K_DriftSparkColor(player_t *player, INT32 charge);
void K_SpawnBoostTrail(player_t *player);
void K_SpawnSparkleTrail(mobj_t *mo);
void K_SpawnWipeoutTrail(mobj_t *mo);
void K_SpawnFireworkTrail(mobj_t *mo);
void K_SpawnDraftDust(mobj_t *mo);
void K_SpawnMagicianParticles(mobj_t *mo, int spread);
void K_DriftDustHandling(mobj_t *spawner);
void K_Squish(mobj_t *mo);
mobj_t *K_ThrowKartItemEx(player_t *player, boolean missile, mobjtype_t mapthing, INT32 defaultDir, INT32 altthrow, angle_t angleOffset, fixed_t tossX, fixed_t tossY);
mobj_t *K_ThrowKartItem(player_t *player, boolean missile, mobjtype_t mapthing, INT32 defaultDir, INT32 altthrow, angle_t angleOffset);
void K_PuntMine(mobj_t *mine, mobj_t *punter);
void K_DoSneaker(player_t *player, INT32 type);
void K_DoPogoSpring(mobj_t *mo, fixed_t vertispeed, UINT8 sound);
void K_DoInvincibility(player_t *player, tic_t time);
void K_KillBananaChain(mobj_t *banana, mobj_t *inflictor, mobj_t *source);
void K_UpdateHnextList(player_t *player, boolean clean);
void K_DropHnextList(player_t *player);
void K_RepairOrbitChain(mobj_t *orbit);
void K_CalculateBananaSlope(mobj_t *mobj, fixed_t x, fixed_t y, fixed_t z, fixed_t radius, fixed_t height, boolean flip, boolean player);
mobj_t *K_FindJawzTarget(mobj_t *actor, player_t *source, angle_t range);
INT32 K_GetKartRingPower(const player_t *player, boolean boosted);
INT32 K_GetFullKartRingPower(const player_t *player, boolean boosted);
boolean K_CheckPlayersRespawnColliding(INT32 playernum, fixed_t x, fixed_t y);
INT16 K_UpdateSteeringValue(INT16 inputSteering, INT16 destSteering);
INT16 K_GetKartTurnValue(const player_t *player, INT16 turnvalue);
INT32 K_GetUnderwaterTurnAdjust(const player_t *player);
INT32 K_GetKartDriftSparkValue(const player_t *player);
INT32 K_StairJankFlip(INT32 value);
INT32 K_GetKartDriftSparkValueForStage(const player_t *player, UINT8 stage);
void K_SpawnDriftBoostExplosion(player_t *player, int stage);
void K_SpawnDriftElectricSparks(player_t *player, int color, boolean shockwave);
void K_KartUpdatePosition(player_t *player);
void K_UpdateAllPlayerPositions(void);
SINT8 K_GetTotallyRandomResult(UINT8 useodds);
mobj_t *K_CreatePaperItem(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT8 type, UINT16 amount);
mobj_t *K_FlingPaperItem(fixed_t x, fixed_t y, fixed_t z, angle_t angle, SINT8 flip, UINT8 type, UINT16 amount);
void K_DropPaperItem(player_t *player, UINT8 itemtype, UINT16 itemamount);
void K_PopPlayerShield(player_t *player);
void K_DropItems(player_t *player);
void K_DropRocketSneaker(player_t *player);
void K_DropKitchenSink(player_t *player);
void K_StripItems(player_t *player);
void K_StripOther(player_t *player);
void K_MomentumToFacing(player_t *player);
boolean K_ApplyOffroad(const player_t *player);
boolean K_SlopeResistance(const player_t *player);
fixed_t K_PlayerTripwireSpeedThreshold(const player_t *player);
tripwirepass_t K_TripwirePassConditions(const player_t *player);
boolean K_TripwirePass(const player_t *player);
boolean K_MovingHorizontally(mobj_t *mobj);
boolean K_WaterRun(mobj_t *mobj);
boolean K_WaterSkip(mobj_t *mobj);
void K_SpawnWaterRunParticles(mobj_t *mobj);
boolean K_IsRidingFloatingTop(const player_t *player);
boolean K_IsHoldingDownTop(const player_t *player);
mobj_t *K_GetGardenTop(const player_t *player);
void K_ApplyTripWire(player_t *player, tripwirestate_t state);
INT16 K_GetSpindashChargeTime(const player_t *player);
fixed_t K_GetSpindashChargeSpeed(const player_t *player);
fixed_t K_GrowShrinkSpeedMul(const player_t *player);
fixed_t K_GetKartSpeedFromStat(UINT8 kartspeed);
fixed_t K_GetKartSpeed(const player_t *player, boolean doboostpower, boolean dorubberbanding);
fixed_t K_GetKartAccel(const player_t *player);
UINT16 K_GetKartFlashing(const player_t *player);
boolean K_PlayerShrinkCheat(const player_t *player);
void K_UpdateShrinkCheat(player_t *player);
boolean K_KartKickstart(const player_t *player);
UINT16 K_GetKartButtons(const player_t *player);
SINT8 K_GetForwardMove(const player_t *player);
fixed_t K_GetNewSpeed(const player_t *player);
fixed_t K_3dKartMovement(const player_t *player);
boolean K_PlayerEBrake(const player_t *player);
boolean K_PlayerGuard(const player_t *player);
SINT8 K_Sliptiding(const player_t *player);
boolean K_FastFallBounce(player_t *player);
void K_DappleEmployment(player_t *player);
fixed_t K_PlayerBaseFriction(const player_t *player, fixed_t original);
void K_AdjustPlayerFriction(player_t *player);
void K_MoveKartPlayer(player_t *player, boolean onground);
void K_CheckSpectateStatus(boolean considermapreset);
UINT8 K_GetInvincibilityItemFrame(void);
UINT8 K_GetOrbinautItemFrame(UINT8 count);
boolean K_IsSPBInGame(void);
void K_KartEbrakeVisuals(player_t *p);
void K_HandleDirectionalInfluence(player_t *player);
fixed_t K_DefaultPlayerRadius(player_t *player);

// sound stuff for lua
void K_PlayAttackTaunt(mobj_t *source);
void K_PlayBoostTaunt(mobj_t *source);
void K_PlayOvertakeSound(mobj_t *source);
void K_PlayPainSound(mobj_t *source, mobj_t *other);
void K_PlayHitEmSound(mobj_t *source, mobj_t *other);
void K_TryHurtSoundExchange(mobj_t *victim, mobj_t *attacker);
void K_PlayPowerGloatSound(mobj_t *source);

fixed_t K_GetItemScaleConst(fixed_t scale);
fixed_t K_ItemScaleFromConst(UINT8 item_scale_const);
fixed_t K_ItemScaleForPlayer(player_t *player);
void K_SetItemOut(player_t *player);
void K_UnsetItemOut(player_t *player);

void K_UpdateMobjItemOverlay(mobj_t *part, SINT8 itemType, UINT8 itemCount);

void K_EggmanTransfer(player_t *source, player_t *victim);

tic_t K_TimeLimitForGametype(void);
UINT32 K_PointLimitForGametype(void);

boolean K_Cooperative(void);

// lat: used for when the player is in some weird state where it wouldn't be wise for it to be overwritten by another object that does similarly wacky shit.
boolean K_isPlayerInSpecialState(player_t *p);

void K_SetTireGrease(player_t *player, tic_t tics);

boolean K_IsPlayingDisplayPlayer(player_t *player);

boolean K_PlayerCanPunt(player_t *player);
void K_MakeObjectReappear(mobj_t *mo);

void K_BumperInflate(player_t *player);

boolean K_ThunderDome(void);

boolean K_PlayerCanUseItem(player_t *player);

fixed_t K_GetGradingFactorAdjustment(player_t *player, UINT32 gradingpoint);
fixed_t K_GetGradingFactorMinMax(player_t *player, boolean max);
UINT16 K_GetEXP(player_t *player);

UINT32 K_GetNumGradingPoints(void);

boolean K_LegacyRingboost(const player_t *player);

void K_BotHitPenalty(player_t *player);

boolean K_IsPickMeUpItem(mobjtype_t type);

boolean K_TryPickMeUp(mobj_t *m1, mobj_t *m2, boolean allowHostile);

fixed_t K_TeamComebackMultiplier(player_t *player);

void K_ApplyStun(player_t *player, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype);

boolean K_CanSuperTransfer(player_t *player);

#ifdef __cplusplus
} // extern "C"
#endif

// =========================================================================
#endif  // __K_KART__
