// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

/* object-specific code */
#ifndef k_objects_H
#define k_objects_H

#include "doomdef.h"
#include "m_fixed.h"
#include "tables.h" // angle_t
#include "taglist.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hyudoro */
void Obj_InitHyudoroCenter(mobj_t *center, mobj_t *master);
void Obj_HyudoroDeploy(mobj_t *master);
void Obj_HyudoroThink(mobj_t *actor);
void Obj_HyudoroCenterThink(mobj_t *actor);
void Obj_HyudoroCollide(mobj_t *special, mobj_t *toucher);
boolean Obj_HyudoroShadowZ(mobj_t *actor, fixed_t *return_z, pslope_t **return_slope);

/* Garden Top */
void Obj_GardenTopDeploy(mobj_t *rider);
mobj_t *Obj_GardenTopThrow(player_t *player);
mobj_t *Obj_GardenTopDestroy(player_t *player);
void Obj_GardenTopThink(mobj_t *top);
void Obj_GardenTopSparkThink(mobj_t *spark);
void Obj_GardenTopArrowThink(mobj_t *arrow);
boolean Obj_GardenTopPlayerIsGrinding(const player_t *player);
boolean Obj_GardenTopPlayerNeedsHelp(const mobj_t *top);

/* Shrink */
void Obj_PohbeeThinker(mobj_t *pohbee);
void Obj_PohbeeRemoved(mobj_t *pohbee);
void Obj_ShrinkGunRemoved(mobj_t *gun);
boolean Obj_ShrinkLaserCollide(mobj_t *gun, mobj_t *victim);
void Obj_CreateShrinkPohbees(player_t *owner);

/* Item Debris */
void Obj_SpawnItemDebrisEffects(mobj_t *collectible, mobj_t *collector);
void Obj_ItemDebrisThink(mobj_t *debris);
fixed_t Obj_ItemDebrisBounce(mobj_t *debris, fixed_t momz);

/* SPB */
void Obj_SPBThrown(mobj_t *spb, fixed_t finalspeed);
void Obj_SPBThink(mobj_t *spb);
void Obj_SPBExplode(mobj_t *spb);
void Obj_SPBTouch(mobj_t *spb, mobj_t *toucher);
void Obj_SPBEradicateCapsules(void);

/* SPB Juicebox Rings */
void Obj_MantaRingThink(mobj_t *manta);
mobj_t *Obj_MantaRingCreate(mobj_t *spb, mobj_t *owner, mobj_t *chase);

/* Orbinaut */
void Obj_OrbinautThink(mobj_t *th);
boolean Obj_OrbinautJawzCollide(mobj_t *t1, mobj_t *t2);
void Obj_OrbinautThrown(mobj_t *th, fixed_t finalSpeed, fixed_t dir);
void Obj_GachaBomThrown(mobj_t *th, fixed_t finalSpeed, fixed_t dir);
void Obj_OrbinautJawzMoveHeld(player_t *player);
boolean Obj_GachaBomWasTossed(mobj_t *th);
void Obj_OrbinautDrop(mobj_t *th);
boolean Obj_OrbinautCanRunOnWater(mobj_t *th);

/* Jawz */
void Obj_JawzThink(mobj_t *th);
void Obj_JawzThrown(mobj_t *th, fixed_t finalSpeed, fixed_t dir);

/* Duel Bomb */
void Obj_DuelBombThink(mobj_t *bomb);
void Obj_DuelBombReverse(mobj_t *bomb);
void Obj_DuelBombTouch(mobj_t *bomb, mobj_t *toucher);
void Obj_DuelBombInit(mobj_t *bomb);

/* Broly Ki */
mobj_t *Obj_SpawnBrolyKi(mobj_t *source, tic_t duration);
mobj_t *Obj_SpawnCustomBrolyKi(mobj_t *source, tic_t duration, fixed_t start, fixed_t end);
boolean Obj_BrolyKiThink(mobj_t *ki);

/* Special Stage UFO */
waypoint_t *K_GetSpecialUFOWaypoint(mobj_t *ufo);
void Obj_SpecialUFOThinker(mobj_t *ufo);
boolean Obj_SpecialUFODamage(mobj_t *ufo, mobj_t *inflictor, mobj_t *source, UINT8 damageType);
void Obj_PlayerUFOCollide(mobj_t *ufo, mobj_t *other);
boolean Obj_UFOEmeraldCollect(mobj_t *ufo, mobj_t *toucher);
void Obj_UFOPieceThink(mobj_t *piece);
void Obj_UFOPieceDead(mobj_t *piece);
void Obj_UFOPieceRemoved(mobj_t *piece);
mobj_t *Obj_CreateSpecialUFO(void);
UINT32 K_GetSpecialUFODistance(void);
void Obj_UFOEmeraldThink(mobj_t *emerald);

/* Monitors */
mobj_t *Obj_SpawnMonitor(mobj_t *origin, UINT8 numItemTypes, UINT8 emerald);
void Obj_MonitorSpawnParts(mobj_t *monitor);
void Obj_MonitorThink(mobj_t *monitor);
void Obj_MonitorPartThink(mobj_t *part);
fixed_t Obj_MonitorGetDamage(mobj_t *monitor, mobj_t *inflictor, UINT8 damagetype);
void Obj_MonitorOnDamage(mobj_t *monitor, mobj_t *inflictor, INT32 damage);
void Obj_MonitorOnDeath(mobj_t *monitor, mobj_t *source);
void Obj_MonitorShardThink(mobj_t *shard);
UINT32 Obj_MonitorGetEmerald(const mobj_t *monitor);
void Obj_MonitorSetItemSpot(mobj_t *monitor, mobj_t *spot);

/* Item Spot */
boolean Obj_ItemSpotIsAvailable(const mobj_t *spot);
void Obj_ItemSpotAssignMonitor(mobj_t *spot, mobj_t *monitor);
void Obj_ItemSpotUpdate(mobj_t *spot);

/* Loops */
mobj_t *Obj_FindLoopCenter(const mtag_t tag);
void Obj_InitLoopEndpoint(mobj_t *end, mobj_t *anchor);
void Obj_InitLoopCenter(mobj_t *center);
void Obj_LinkLoopAnchor(mobj_t *anchor, mobj_t *center, UINT8 type);
void Obj_LoopEndpointCollide(mobj_t *special, mobj_t *toucher);

/* Drop Target */
void Obj_BeginDropTargetMorph(mobj_t *target, skincolornum_t color);
boolean Obj_DropTargetMorphThink(mobj_t *morph);

/* Instawhip */
void Obj_InstaWhipThink(mobj_t *whip);
void Obj_SpawnInstaWhipRecharge(player_t *player, angle_t angleOffset);
void Obj_InstaWhipRechargeThink(mobj_t *mobj);
void Obj_SpawnInstaWhipReject(player_t *player);
void Obj_InstaWhipRejectThink(mobj_t *mobj);

/* Block VFX */
void Obj_BlockRingThink(mobj_t *ring);
void Obj_BlockBodyThink(mobj_t *body);
void Obj_GuardBreakThink(mobj_t *fx);

void Obj_AmpRingThink(mobj_t *amp);
void Obj_AmpBodyThink(mobj_t *amp);
void Obj_AmpAuraThink(mobj_t *amp);
void Obj_AmpBurstThink(mobj_t *amp);

void Obj_AmpsThink(mobj_t *amps);

void Obj_ExpThink(mobj_t *exp);

void Obj_ChargeAuraThink(mobj_t *aura);
void Obj_ChargeFallThink(mobj_t *charge);
void Obj_ChargeReleaseThink(mobj_t *release);
void Obj_ChargeExtraThink(mobj_t *extra);

/* Bail VFX */
void Obj_BailThink(mobj_t *aura);
void Obj_BailChargeThink(mobj_t *aura);

/* Ring Shooter */
boolean Obj_RingShooterThinker(mobj_t *mo);
boolean Obj_PlayerRingShooterFreeze(const player_t *player);
void Obj_RingShooterInput(player_t *player);
void Obj_PlayerUsedRingShooter(mobj_t *base, player_t *player);
void Obj_RingShooterDelete(mobj_t *mo);
void Obj_UpdateRingShooterFace(mobj_t *part);

/* Follower Audience */
void Obj_AudienceInit(mobj_t * mobj, mapthing_t *mthing, INT32 followerpick);
void Obj_AudienceThink(mobj_t * mobj, boolean focusonplayer, boolean checkdeathpit);

/* Random Item Boxes */
void Obj_RandomItemVisuals(mobj_t *mobj);
boolean Obj_RandomItemSpawnIn(mobj_t *mobj);
fixed_t Obj_RandomItemScale(fixed_t oldScale);
void Obj_RandomItemSpawn(mobj_t *mobj);
#define RINGBOX_TIME (105)

/* Gachabom Rebound */
void Obj_GachaBomReboundThink(mobj_t *mobj);
void Obj_SpawnGachaBomRebound(mobj_t *source, mobj_t *target);

/* Servant Hand */
void Obj_ServantHandSpawning(player_t *player);
void Obj_ServantHandThink(mobj_t *hand);
void Obj_PointPlayersToXY(fixed_t x, fixed_t y);

/* Super Flicky Controller */
void Obj_SpawnSuperFlickySwarm(player_t *owner, tic_t time);
void Obj_SuperFlickyControllerThink(mobj_t *controller);
void Obj_EndSuperFlickySwarm(mobj_t *controller);
void Obj_ExtendSuperFlickySwarm(mobj_t *controller, tic_t time);
tic_t Obj_SuperFlickySwarmTime(mobj_t *controller);

/* Super Flicky */
void Obj_SuperFlickyThink(mobj_t *flicky);
void Obj_WhipSuperFlicky(mobj_t *flicky);
void Obj_BlockSuperFlicky(mobj_t *flicky);
void Obj_SuperFlickyPlayerCollide(mobj_t *flicky, mobj_t *player);
void Obj_SuperFlickyLanding(mobj_t *flicky);
mobj_t *Obj_SuperFlickyOwner(const mobj_t *flicky);
boolean Obj_IsSuperFlickyWhippable(const mobj_t *flicky, const mobj_t *target);
boolean Obj_IsSuperFlickyTargettingYou(const mobj_t *flicky, mobj_t *player);

/* Battle/Power-UP UFO */
void Obj_BattleUFOLegThink(mobj_t *leg);
void Obj_BattleUFOThink(mobj_t *ufo);
void Obj_SpawnBattleUFOLegs(mobj_t *ufo);
void Obj_BattleUFODeath(mobj_t *ufo, mobj_t *inflictor);
void Obj_LinkBattleUFOSpawner(mobj_t *spawner);
void Obj_UnlinkBattleUFOSpawner(mobj_t *spawner);
void Obj_SpawnBattleUFOFromSpawner(void);
INT32 Obj_RandomBattleUFOSpawnerID(void);
void Obj_BattleUFOBeamThink(mobj_t *beam);
INT32 Obj_BattleUFOSpawnerID(const mobj_t *spawner);
mobj_t *Obj_GetNextUFOSpawner(void);

/* Power-Up Aura */
void Obj_SpawnPowerUpAura(player_t* player);
void Obj_PowerUpAuraThink(mobj_t* mobj);

/* Ark Arrows */
void Obj_ArkArrowSpawn(mobj_t *mobj);
void Obj_ArkArrowSetup(mobj_t *mobj, mapthing_t *mthing);
void Obj_ArkArrowThink(mobj_t *mobj);

/* Dash Rings */
void Obj_RegularDashRingSpawn(mobj_t *mobj);
void Obj_RainbowDashRingSpawn(mobj_t *mobj);
void Obj_DashRingSetup(mobj_t *mobj, mapthing_t *mthing);
void Obj_RainbowDashRingThink(mobj_t *mobj);
void Obj_DashRingTouch(mobj_t *mobj, player_t *player);
void Obj_DashRingPlayerThink(player_t *player);
boolean Obj_DashRingPlayerHasNoGravity(player_t *player);
boolean Obj_DashRingIsUsableByPlayer(mobj_t *mobj, player_t *player);

/* Adventure Dash Ring */
void Obj_AdventureAirBoosterSetup(mobj_t *mobj, mapthing_t *mthing);
void Obj_AdventureAirBoosterHitboxTouch(mobj_t *hitbox, player_t *player);
void Obj_AdventureAirBoosterFuse(mobj_t *mobj);

/* Sneaker Panels */
void Obj_SneakerPanelSpriteScale(mobj_t *mobj);
void Obj_SneakerPanelSpawn(mobj_t *mobj);
void Obj_SneakerPanelSetup(mobj_t *mobj, mapthing_t *mthing);
void Obj_SneakerPanelCollide(mobj_t *pad, mobj_t *mo);
void Obj_SneakerPanelSpawnerSpawn(mobj_t *mobj);
void Obj_SneakerPanelSpawnerSetup(mobj_t *mobj, mapthing_t *mthing);
void Obj_SneakerPanelSpawnerFuse(mobj_t *mobj);

/* Emerald */
void Obj_SpawnEmeraldSparks(mobj_t *source);
void Obj_EmeraldThink(mobj_t *emerald);
void Obj_EmeraldFlareThink(mobj_t *flare);
void Obj_BeginEmeraldOrbit(mobj_t *emerald, mobj_t *target, fixed_t radius, INT32 revolution_time, tic_t fuse);
void Obj_GiveEmerald(mobj_t *emerald);
void Obj_SetEmeraldAwardee(mobj_t *emerald, mobj_t *awardee);
boolean Obj_EmeraldCanHUDTrack(const mobj_t *emerald);

/* Fake Shadow */
mobj_t *Obj_SpawnFakeShadow(mobj_t *from);
void Obj_FakeShadowThink(mobj_t *shadow);
boolean Obj_FakeShadowZ(const mobj_t *shadow, fixed_t *return_z, pslope_t **return_slope);

/* Checkpoints */
void Obj_LinkCheckpoint(mobj_t *end);
void Obj_UnlinkCheckpoint(mobj_t *end);
void Obj_CheckpointThink(mobj_t *end);
void Obj_CrossCheckpoints(player_t *player, fixed_t old_x, fixed_t old_y);
mobj_t *Obj_FindCheckpoint(INT32 id);
boolean Obj_GetCheckpointRespawnPosition(const mobj_t *checkpoint, vector3_t *return_pos);
angle_t Obj_GetCheckpointRespawnAngle(const mobj_t *checkpoint);
void Obj_ActivateCheckpointInstantly(mobj_t* mobj);
UINT32 Obj_GetCheckpointCount();
void Obj_ClearCheckpoints();
void Obj_DeactivateCheckpoints();

/* Rideroid / Rideroid Node */
void Obj_RideroidThink(mobj_t *mo);
void Obj_RideroidNodeSpawn(mobj_t *mo);
void Obj_RideroidNodeThink(mobj_t *mo);
void Obj_getPlayerOffRideroid(mobj_t *mo);	// used in p_map.c to get off of em when passing transfer lines.

/* LSZ Bungee */
void Obj_BungeeSpecial(mobj_t *mo, player_t *p);	// used when the player touches the bungee, to be used in p_inter.c
void Obj_playerBungeeThink(player_t *p);			// player interaction with the bungee. The bungee is to be stored in p->mo->tracer.
void Obj_EndBungee(player_t *p);

/* LSZ Balls */
void Obj_EggBallSpawnerThink(mobj_t *mo);
void Obj_EggBallThink(mobj_t *mo);

/* DLZ Rockets */
void Obj_DLZRocketThink(mobj_t *mo);
void Obj_DLZRocketSpecial(mobj_t *mo, player_t *p);	// touch activation
void Obj_playerDLZRocket(player_t *p);				// player looping thinker
void Obj_DLZRocketDismount(player_t *p);			// used in p_map.c to get off the rocket when we cross transfer lines.

/* DLZ Hover */
void Obj_DLZHoverSpawn(mobj_t *mo);
void Obj_DLZHoverCollide(mobj_t *mo, mobj_t *mo2);

/* DLZ Ring Vaccum */
void Obj_DLZRingVaccumSpawn(mobj_t *mo);
void Obj_DLZRingVaccumCollide(mobj_t *mo, mobj_t *mo2);
void Obj_DLZSuckedRingThink(mobj_t *mo);

/* WPZ Turbine */
void Obj_WPZTurbineSpawn(mobj_t *mo);
void Obj_WPZTurbineThinker(mobj_t *mo);
void Obj_playerWPZTurbine(player_t *p);
void Obj_WPZBubbleThink(mobj_t *mo);

/* WPZ Fountains */
void Obj_WPZFountainThink(mobj_t *mo);

/* WPZ Kuragens */
void Obj_WPZKuragenThink(mobj_t *mo);
void Obj_WPZKuragenBombThink(mobj_t *mo);

/* Ball Switch */
void Obj_BallSwitchInit(mobj_t *mobj);
void Obj_BallSwitchThink(mobj_t *mobj);
void Obj_BallSwitchTouched(mobj_t *mobj, mobj_t *toucher);
void Obj_BallSwitchDamaged(mobj_t *mobj, mobj_t *inflictor, mobj_t *source);

/* Barrier Power-Up */
void Obj_SpawnMegaBarrier(player_t *player);
boolean Obj_MegaBarrierThink(mobj_t *mobj);

/* DLZ Seasaw */
void Obj_DLZSeasawSpawn(mobj_t *mo);
void Obj_DLZSeasawThink(mobj_t *mo);
void Obj_DLZSeasawCollide(mobj_t *mo, mobj_t *mo2);

/* GPZ Seasaw */
void Obj_GPZSeasawSpawn(mobj_t *mo);
void Obj_GPZSeasawThink(mobj_t *mo);
void Obj_GPZSeasawCollide(mobj_t *mo, mobj_t *mo2);

/* Frozen Production Frost Thrower */
void Obj_FreezeThrusterInit(mobj_t *mobj);
void Obj_FreezeThrusterThink(mobj_t *mobj);
void Obj_IceDustCollide(mobj_t *t1, mobj_t *t2);
boolean Obj_IceCubeThink(mobj_t *mobj);
void Obj_IceCubeInput(player_t *player);
void Obj_IceCubeBurst(player_t *player);
void Obj_SidewaysFreezeThrusterInit(mobj_t *mobj);
void Obj_SidewaysFreezeThrusterThink(mobj_t *mobj);

/* Ivo Balls */
void Obj_IvoBallInit(mobj_t *mo);
void Obj_IvoBallThink(mobj_t *mo);
void Obj_IvoBallTouch(mobj_t *special, mobj_t *toucher);
void Obj_PatrolIvoBallInit(mobj_t *mo);
void Obj_PatrolIvoBallThink(mobj_t *mo);
void Obj_PatrolIvoBallTouch(mobj_t *special, mobj_t *toucher);

/* SA2 Crates / Ice Cap Blocks */
void Obj_BoxSideThink(mobj_t *mo);
void Obj_TryCrateInit(mobj_t *mo);
boolean Obj_TryCrateThink(mobj_t *mo);
void Obj_TryCrateTouch(mobj_t *special, mobj_t *toucher);
boolean Obj_TryCrateDamage(mobj_t *target, mobj_t *inflictor);
boolean Obj_SA2CrateIsMetal(mobj_t *mo);

/* Lavender Shrine Spears */
void Obj_SpearInit(mobj_t *mo);
void Obj_SpearThink(mobj_t *mo);

/* Lost Colony Fuel Canister */
void Obj_FuelCanisterEmitterInit(mobj_t *mo);
boolean Obj_FuelCanisterVisualThink(mobj_t *mo);
boolean Obj_FuelCanisterEmitterThink(mobj_t *mo);
boolean Obj_FuelCanisterThink(mobj_t *mo);
void Obj_FuelCanisterTouch(mobj_t *special, mobj_t *toucher);
void Obj_FuelCanisterExplosionTouch(mobj_t *special, mobj_t *toucher);
boolean Obj_FuelCanisterExplosionThink(mobj_t *mo);

/* Bustable Rocks */
void Obj_LinkRocks(mobj_t *mo);
void Obj_UnlinkRocks(mobj_t *mo);
void Obj_TouchRocks(mobj_t *special, mobj_t *toucher);
void Obj_UpdateRocks(void);
void Obj_AnimateEndlessMineRocks(mobj_t *mo);

/* Enldess Mine Faucet */
void Obj_EMZFaucetThink(mobj_t *mo);
void Obj_EMZDripDeath(mobj_t *mo);
void Obj_EMZRainGenerator(mobj_t *mo);

/* Joypolis Trick Balloons */
void Obj_TrickBalloonMobjSpawn(mobj_t* mobj);
void Obj_TrickBalloonTouchSpecial(mobj_t* special, mobj_t* toucher);

/* AHZ/AGZ/SSZ Clouds */
void Obj_CloudSpawn(mobj_t *mobj);
void Obj_TulipSpawnerThink(mobj_t *mobj);
void Obj_PlayerCloudThink(player_t *player);
void Obj_PlayerBulbThink(player_t *player);
void Obj_CloudTouched(mobj_t *special, mobj_t *toucher);
void Obj_BulbTouched(mobj_t *special, mobj_t *toucher);

/* Waterfall Particles Spawner */
void Obj_WaterfallParticleThink(mobj_t *mo);

/* Sealed Star objects */
void Obj_SSCandleMobjThink(mobj_t* mo);
void Obj_SSHologramRotatorMapThingSpawn(mobj_t* mo, mapthing_t* mt);
void Obj_SSHologramRotatorMobjThink(mobj_t* mo);
void Obj_SSHologramMobjSpawn(mobj_t* mo);
void Obj_SSHologramMobjFuse(mobj_t* mo);
void Obj_SSHologramMapThingSpawn(mobj_t* mo, mapthing_t* mt);
void Obj_SSCoinCloudMapThingSpawn(mobj_t* mo, mapthing_t* mt);
void Obj_SSCoinMobjThink(mobj_t* mo);
void Obj_SSGobletCloudMapThingSpawn(mobj_t* mo, mapthing_t* mt);
void Obj_SSGobletMobjThink(mobj_t* mo);
void Obj_SSLampMapThingSpawn(mobj_t* mo, mapthing_t* mt);
void Obj_SSWindowMapThingSpawn(mobj_t* mo, mapthing_t* mt);
void Obj_SLSTMaceMobjThink(mobj_t* mo);
void Obj_SSBumperMobjSpawn(mobj_t* mo);
void Obj_SSChainMobjThink(mobj_t* mo);
void Obj_SSGachaTargetMobjSpawn(mobj_t* mo);
void Obj_SSCabotronMobjSpawn(mobj_t* mo);
void Obj_SSCabotronMobjThink(mobj_t* mo);
void Obj_SSCabotronStarMobjThink(mobj_t* mo);

/* Talk Point */
void Obj_TalkPointInit(mobj_t* mo);
void Obj_TalkPointThink(mobj_t* mo);
void Obj_TalkPointOrbThink(mobj_t* mo);

/* Power-up Spinner */
void Obj_SpawnPowerUpSpinner(mobj_t *source, INT32 powerup, tic_t duration);
void Obj_TickPowerUpSpinner(mobj_t *mobj);

/* Destroyed Kart */
void Obj_SpawnDestroyedKart(mobj_t *player);
void Obj_DestroyedKartThink(mobj_t *kart);
boolean Obj_DestroyKart(mobj_t *kart);
void Obj_DestroyedKartParticleThink(mobj_t *part);
void Obj_DestroyedKartParticleLanding(mobj_t *part);

/* Flybot767 (stun) */
void Obj_SpawnFlybotsForPlayer(player_t *player);
void Obj_FlybotThink(mobj_t *flybot);
void Obj_FlybotDeath(mobj_t *flybot);
void Obj_FlybotRemoved(mobj_t *flybot);

/* Pulley */
void Obj_PulleyThink(mobj_t *root);
void Obj_PulleyHookTouch(mobj_t *special, mobj_t *toucher);

/* Ballhog */
UINT8 K_HogChargeToHogCount(INT32 charge, UINT8 cap);
void K_UpdateBallhogReticules(player_t *player, UINT8 num_hogs, boolean on_release);
void K_DoBallhogAttack(player_t *player, UINT8 num_hogs);

/* Bubble Shield */
void Obj_SpawnBubbleShieldVisuals(mobj_t *source);
boolean Obj_TickBubbleShieldVisual(mobj_t *mobj);

/* Lightning Shield */
void Obj_SpawnLightningShieldVisuals(mobj_t *source);
boolean Obj_TickLightningShieldVisual(mobj_t *mobj);

/* Lightning Attack */
void Obj_SpawnLightningAttackVisuals(mobj_t *source);
boolean Obj_TickLightningAttackVisual(mobj_t *mobj);

/* Flame Shield */
void Obj_SpawnFlameShieldVisuals(mobj_t *source);
boolean Obj_TickFlameShieldVisual(mobj_t *mobj);

/* Stone Shoe */
mobj_t *Obj_SpawnStoneShoe(INT32 owner, mobj_t *victim);
boolean Obj_TickStoneShoe(mobj_t *shoe);
boolean Obj_TickStoneShoeChain(mobj_t *chain);
player_t *Obj_StoneShoeOwnerPlayer(mobj_t *shoe);
void Obj_CollideStoneShoe(mobj_t *mover, mobj_t *mobj);

/* Toxomister */
void Obj_InitToxomisterPole(mobj_t *pole);
boolean Obj_TickToxomisterPole(mobj_t *pole);
boolean Obj_TickToxomisterEye(mobj_t *eye);
boolean Obj_TickToxomisterCloud(mobj_t *cloud);
boolean Obj_ToxomisterPoleCollide(mobj_t *pole, mobj_t *toucher);
boolean Obj_ToxomisterCloudCollide(mobj_t *cloud, mobj_t *toucher);
fixed_t Obj_GetToxomisterCloudDrag(mobj_t *cloud);

/* Ancient Gear */
void Obj_AncientGearSpawn(mobj_t *gear);
void Obj_AncientGearPartThink(mobj_t *part);
void Obj_AncientGearRemoved(mobj_t *gear);
void Obj_AncientGearTouch(mobj_t *gear, mobj_t *toucher);
void Obj_AncientGearDeath(mobj_t *gear, mobj_t *source);
void Obj_AncientGearDeadThink(mobj_t *gear);
boolean Obj_AllowNextAncientGearSpawn(void);
void Obj_AncientGearSetup(mobj_t *gear, mapthing_t *mt);
void Obj_AncientGearLevelInit(void);
player_t *Obj_GetAncientGearCollectingPlayer(void);
boolean Obj_AllAncientGearsCollected(void);
mobj_t *Obj_GetAncientGearMinimapMobj(void);

void Obj_MushroomHillPolePlayerThink(player_t *player);
void Obj_MushroomHillPoleTouch(mobj_t *pole, mobj_t *toucher);
void Obj_MushroomHillPoleFuse(mobj_t *pole);


#ifdef __cplusplus
} // extern "C"
#endif

#endif/*k_objects_H*/
