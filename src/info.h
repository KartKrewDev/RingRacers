// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  info.h
/// \brief Thing frame/state LUT

#ifndef __INFO__
#define __INFO__

// Needed for action function pointer handling.
#include "d_think.h"
#include "sounds.h"
#include "m_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

// deh_tables.c now has lists for the more named enums! PLEASE keep them up to date!
// For great modding!!

// IMPORTANT!
// DO NOT FORGET TO SYNC THIS LIST WITH THE ACTIONPOINTERS ARRAY IN DEH_TABLES.C
enum actionnum
{
	A_EXPLODE = 0,
	A_PAIN,
	A_FALL,
	A_LOOK,
	A_CHASE,
	A_FACESTABCHASE,
	A_FACESTABREV,
	A_FACESTABHURL,
	A_FACESTABMISS,
	A_STATUEBURST,
	A_FACETARGET,
	A_FACETRACER,
	A_SCREAM,
	A_BOSSDEATH,
	A_RINGBOX,
	A_BUNNYHOP,
	A_BUBBLESPAWN,
	A_FANBUBBLESPAWN,
	A_BUBBLERISE,
	A_BUBBLECHECK,
	A_AWARDSCORE,
	A_SCORERISE,
	A_ATTRACTCHASE,
	A_DROPMINE,
	A_FISHJUMP,
	A_SETSOLIDSTEAM,
	A_UNSETSOLIDSTEAM,
	A_OVERLAYTHINK,
	A_JETCHASE,
	A_JETBTHINK,
	A_JETGTHINK,
	A_JETGSHOOT,
	A_SHOOTBULLET,
	A_MINUSDIGGING,
	A_MINUSPOPUP,
	A_MINUSCHECK,
	A_CHICKENCHECK,
	A_MOUSETHINK,
	A_DETONCHASE,
	A_CAPECHASE,
	A_ROTATESPIKEBALL,
	A_SLINGAPPEAR,
	A_UNIDUSBALL,
	A_ROCKSPAWN,
	A_SETFUSE,
	A_CRAWLACOMMANDERTHINK,
	A_SMOKETRAILER,
	A_RINGEXPLODE,
	A_OLDRINGEXPLODE,
	A_MIXUP,
	A_BOSS1CHASE,
	A_FOCUSTARGET,
	A_BOSS2CHASE,
	A_BOSS2POGO,
	A_BOSSZOOM,
	A_BOSSSCREAM,
	A_BOSS2TAKEDAMAGE,
	A_BOSS7CHASE,
	A_GOOPSPLAT,
	A_BOSS2POGOSFX,
	A_BOSS2POGOTARGET,
	A_BOSSJETFUME,
	A_EGGMANBOX,
	A_TURRETFIRE,
	A_SUPERTURRETFIRE,
	A_TURRETSTOP,
	A_JETJAWROAM,
	A_JETJAWCHOMP,
	A_POINTYTHINK,
	A_CHECKBUDDY,
	A_HOODFIRE,
	A_HOODTHINK,
	A_HOODFALL,
	A_ARROWBONKS,
	A_SNAILERTHINK,
	A_SHARPCHASE,
	A_SHARPSPIN,
	A_SHARPDECEL,
	A_CRUSHSTACEANWALK,
	A_CRUSHSTACEANPUNCH,
	A_CRUSHCLAWAIM,
	A_CRUSHCLAWLAUNCH,
	A_VULTUREVTOL,
	A_VULTURECHECK,
	A_VULTUREHOVER,
	A_VULTUREBLAST,
	A_VULTUREFLY,
	A_SKIMCHASE,
	A_SKULLATTACK,
	A_LOBSHOT,
	A_FIRESHOT,
	A_SUPERFIRESHOT,
	A_BOSSFIRESHOT,
	A_BOSS7FIREMISSILES,
	A_BOSS1LASER,
	A_BOSS4REVERSE,
	A_BOSS4SPEEDUP,
	A_BOSS4RAISE,
	A_SPARKFOLLOW,
	A_BUZZFLY,
	A_GUARDCHASE,
	A_EGGSHIELD,
	A_SETREACTIONTIME,
	A_BOSS1SPIKEBALLS,
	A_BOSS3TAKEDAMAGE,
	A_BOSS3PATH,
	A_BOSS3SHOCKTHINK,
	A_LINEDEFEXECUTE,
	A_LINEDEFEXECUTEFROMARG,
	A_PLAYSEESOUND,
	A_PLAYATTACKSOUND,
	A_PLAYACTIVESOUND,
	A_SPAWNOBJECTABSOLUTE,
	A_SPAWNOBJECTRELATIVE,
	A_CHANGEANGLERELATIVE,
	A_CHANGEANGLEABSOLUTE,
	A_ROLLANGLE,
	A_CHANGEROLLANGLERELATIVE,
	A_CHANGEROLLANGLEABSOLUTE,
	A_PLAYSOUND,
	A_FINDTARGET,
	A_FINDTRACER,
	A_SETTICS,
	A_SETRANDOMTICS,
	A_CHANGECOLORRELATIVE,
	A_CHANGECOLORABSOLUTE,
	A_DYE,
	A_MOVERELATIVE,
	A_MOVEABSOLUTE,
	A_THRUST,
	A_ZTHRUST,
	A_SETTARGETSTARGET,
	A_SETOBJECTFLAGS,
	A_SETOBJECTFLAGS2,
	A_RANDOMSTATE,
	A_RANDOMSTATERANGE,
	A_STATERANGEBYANGLE,
	A_STATERANGEBYPARAMETER,
	A_DUALACTION,
	A_REMOTEACTION,
	A_TOGGLEFLAMEJET,
	A_ORBITNIGHTS,
	A_GHOSTME,
	A_SETOBJECTSTATE,
	A_SETOBJECTTYPESTATE,
	A_KNOCKBACK,
	A_PUSHAWAY,
	A_RINGDRAIN,
	A_SPLITSHOT,
	A_MISSILESPLIT,
	A_MULTISHOT,
	A_INSTALOOP,
	A_CUSTOM3DROTATE,
	A_SEARCHFORPLAYERS,
	A_CHECKRANDOM,
	A_CHECKTARGETRINGS,
	A_CHECKRINGS,
	A_CHECKTOTALRINGS,
	A_CHECKHEALTH,
	A_CHECKRANGE,
	A_CHECKHEIGHT,
	A_CHECKTRUERANGE,
	A_CHECKTHINGCOUNT,
	A_CHECKAMBUSH,
	A_CHECKCUSTOMVALUE,
	A_CHECKCUSVALMEMO,
	A_SETCUSTOMVALUE,
	A_USECUSVALMEMO,
	A_RELAYCUSTOMVALUE,
	A_CUSVALACTION,
	A_FORCESTOP,
	A_FORCEWIN,
	A_SPIKERETRACT,
	A_INFOSTATE,
	A_REPEAT,
	A_SETSCALE,
	A_REMOTEDAMAGE,
	A_HOMINGCHASE,
	A_TRAPSHOT,
	A_VILETARGET,
	A_VILEATTACK,
	A_VILEFIRE,
	A_BRAKCHASE,
	A_BRAKFIRESHOT,
	A_BRAKLOBSHOT,
	A_NAPALMSCATTER,
	A_SPAWNFRESHCOPY,
	A_FLICKYSPAWN,
	A_FLICKYCENTER,
	A_FLICKYAIM,
	A_FLICKYFLY,
	A_FLICKYSOAR,
	A_FLICKYCOAST,
	A_FLICKYHOP,
	A_FLICKYFLOUNDER,
	A_FLICKYCHECK,
	A_FLICKYHEIGHTCHECK,
	A_FLICKYFLUTTER,
	A_FLAMEPARTICLE,
	A_FADEOVERLAY,
	A_BOSS5JUMP,
	A_LIGHTBEAMRESET,
	A_MINEEXPLODE,
	A_MINERANGE,
	A_CONNECTTOGROUND,
	A_SPAWNPARTICLERELATIVE,
	A_MULTISHOTDIST,
	A_WHOCARESIFYOURSONISABEE,
	A_PARENTTRIESTOSLEEP,
	A_CRYINGTOMOMMA,
	A_CHECKFLAGS2,
	A_BOSS5FINDWAYPOINT,
	A_DONPCSKID,
	A_DONPCPAIN,
	A_PREPAREREPEAT,
	A_BOSS5EXTRAREPEAT,
	A_BOSS5CALM,
	A_BOSS5CHECKONGROUND,
	A_BOSS5CHECKFALLING,
	A_BOSS5PINCHSHOT,
	A_BOSS5MAKEITRAIN,
	A_BOSS5MAKEJUNK,
	A_LOOKFORBETTER,
	A_BOSS5BOMBEXPLODE,
	A_TNTEXPLODE,
	A_DEBRISRANDOM,
	A_TRAINCAMEO,
	A_TRAINCAMEO2,
	A_CANARIVOREGAS,
	A_KILLSEGMENTS,
	A_SNAPPERSPAWN,
	A_SNAPPERTHINKER,
	A_SALOONDOORSPAWN,
	A_MINECARTSPARKTHINK,
	A_MODULOTOSTATE,
	A_LAVAFALLROCKS,
	A_LAVAFALLLAVA,
	A_FALLINGLAVACHECK,
	A_FIRESHRINK,
	A_SPAWNPTERABYTES,
	A_PTERABYTEHOVER,
	A_ROLLOUTSPAWN,
	A_ROLLOUTROCK,
	A_DRAGONBOMBERSPAWN,
	A_DRAGONWING,
	A_DRAGONSEGMENT,
	A_CHANGEHEIGHT,
	A_JAWZEXPLODE,
	A_SSMINESEARCH,
	A_SSMINEEXPLODE,
	A_LANDMINEEXPLODE,
	A_BALLHOGEXPLODE,
	A_SPECIALSTAGEBOMBEXPLODE,
	A_LIGHTNINGFOLLOWPLAYER,
	A_FZBOOMFLASH,
	A_FZBOOMSMOKE,
	A_RANDOMSHADOWFRAME,
	A_ROAMINGSHADOWTHINKER,
	A_MAYONAKAARROW,
	A_MEMENTOSTPPARTICLES,
	A_REAPERTHINKER,
	A_FLAMESHIELDPAPER,
	A_INVINCSPARKLEROTATE,
	A_SPAWNITEMDEBRISCLOUD,
	A_RINGSHOOTERFACE,
	A_SPAWNSNEAKERPANEL,
	A_BLENDEYEPUYOHACK,
	NUMACTIONS
};

// IMPORTANT NOTE: If you add/remove from this list of action
// functions, don't forget to update them in deh_tables.c!
void A_Explode();
void A_Pain();
void A_Fall();
void A_Look();
void A_Chase();
void A_FaceStabChase();
void A_FaceStabRev();
void A_FaceStabHurl();
void A_FaceStabMiss();
void A_StatueBurst();
void A_FaceTarget();
void A_FaceTracer();
void A_Scream();
void A_BossDeath();
void A_RingBox(); // Obtained Ring Box Tails
void A_BunnyHop(); // have bunny hop tails
void A_BubbleSpawn(); // Randomly spawn bubbles
void A_FanBubbleSpawn();
void A_BubbleRise(); // Bubbles float to surface
void A_BubbleCheck(); // Don't draw if not underwater
void A_AwardScore();
void A_ScoreRise(); // Rise the score logo
void A_AttractChase(); // Ring Chase
void A_DropMine(); // Drop Mine from Skim or Jetty-Syn Bomber
void A_FishJump(); // Fish Jump
void A_SetSolidSteam();
void A_UnsetSolidSteam();
void A_OverlayThink();
void A_JetChase();
void A_JetbThink(); // Jetty-Syn Bomber Thinker
void A_JetgThink(); // Jetty-Syn Gunner Thinker
void A_JetgShoot(); // Jetty-Syn Shoot Function
void A_ShootBullet(); // JetgShoot without reactiontime setting
void A_MinusDigging();
void A_MinusPopup();
void A_MinusCheck();
void A_ChickenCheck();
void A_MouseThink(); // Mouse Thinker
void A_DetonChase(); // Deton Chaser
void A_CapeChase(); // Fake little Super Sonic cape
void A_RotateSpikeBall(); // Spike ball rotation
void A_SlingAppear();
void A_UnidusBall();
void A_RockSpawn();
void A_SetFuse();
void A_CrawlaCommanderThink(); // Crawla Commander
void A_SmokeTrailer();
void A_RingExplode();
void A_OldRingExplode();
void A_MixUp();
void A_BossScream();
void A_Boss2TakeDamage();
void A_GoopSplat();
void A_Boss2PogoSFX();
void A_Boss2PogoTarget();
void A_EggmanBox();
void A_TurretFire();
void A_SuperTurretFire();
void A_TurretStop();
void A_JetJawRoam();
void A_JetJawChomp();
void A_PointyThink();
void A_CheckBuddy();
void A_HoodFire();
void A_HoodThink();
void A_HoodFall();
void A_ArrowBonks();
void A_SnailerThink();
void A_SharpChase();
void A_SharpSpin();
void A_SharpDecel();
void A_CrushstaceanWalk();
void A_CrushstaceanPunch();
void A_CrushclawAim();
void A_CrushclawLaunch();
void A_VultureVtol();
void A_VultureCheck();
void A_VultureHover();
void A_VultureBlast();
void A_VultureFly();
void A_SkimChase();
void A_SkullAttack();
void A_LobShot();
void A_FireShot();
void A_SuperFireShot();
void A_BossFireShot();
void A_Boss7FireMissiles();
void A_Boss1Laser();
void A_FocusTarget();
void A_Boss4Reverse();
void A_Boss4SpeedUp();
void A_Boss4Raise();
void A_SparkFollow();
void A_BuzzFly();
void A_GuardChase();
void A_EggShield();
void A_SetReactionTime();
void A_Boss1Spikeballs();
void A_Boss3TakeDamage();
void A_Boss3Path();
void A_Boss3ShockThink();
void A_LinedefExecute();
void A_LinedefExecuteFromArg();
void A_PlaySeeSound();
void A_PlayAttackSound();
void A_PlayActiveSound();
void A_BossZoom(); //Unused
void A_Boss1Chase();
void A_Boss2Chase();
void A_Boss2Pogo();
void A_BossJetFume();
void A_SpawnObjectAbsolute();
void A_SpawnObjectRelative();
void A_ChangeAngleRelative();
void A_ChangeAngleAbsolute();
void A_RollAngle();
void A_ChangeRollAngleRelative();
void A_ChangeRollAngleAbsolute();
void A_PlaySound();
void A_FindTarget();
void A_FindTracer();
void A_SetTics();
void A_SetRandomTics();
void A_ChangeColorRelative();
void A_ChangeColorAbsolute();
void A_Dye();
void A_MoveRelative();
void A_MoveAbsolute();
void A_Thrust();
void A_ZThrust();
void A_SetTargetsTarget();
void A_SetObjectFlags();
void A_SetObjectFlags2();
void A_RandomState();
void A_RandomStateRange();
void A_StateRangeByAngle();
void A_StateRangeByParameter();
void A_DualAction();
void A_RemoteAction();
void A_ToggleFlameJet();
void A_OrbitNights();
void A_GhostMe();
void A_SetObjectState();
void A_SetObjectTypeState();
void A_KnockBack();
void A_PushAway();
void A_RingDrain();
void A_SplitShot();
void A_MissileSplit();
void A_MultiShot();
void A_InstaLoop();
void A_Custom3DRotate();
void A_SearchForPlayers();
void A_CheckRandom();
void A_CheckTargetRings();
void A_CheckRings();
void A_CheckTotalRings();
void A_CheckHealth();
void A_CheckRange();
void A_CheckHeight();
void A_CheckTrueRange();
void A_CheckThingCount();
void A_CheckAmbush();
void A_CheckCustomValue();
void A_CheckCusValMemo();
void A_SetCustomValue();
void A_UseCusValMemo();
void A_RelayCustomValue();
void A_CusValAction();
void A_ForceStop();
void A_ForceWin();
void A_SpikeRetract();
void A_InfoState();
void A_Repeat();
void A_SetScale();
void A_RemoteDamage();
void A_HomingChase();
void A_TrapShot();
void A_VileTarget();
void A_VileAttack();
void A_VileFire();
void A_BrakChase();
void A_BrakFireShot();
void A_BrakLobShot();
void A_NapalmScatter();
void A_SpawnFreshCopy();
void A_FlickySpawn();
void A_FlickyCenter();
void A_FlickyAim();
void A_FlickyFly();
void A_FlickySoar();
void A_FlickyCoast();
void A_FlickyHop();
void A_FlickyFlounder();
void A_FlickyCheck();
void A_FlickyHeightCheck();
void A_FlickyFlutter();
void A_FlameParticle();
void A_FadeOverlay();
void A_Boss5Jump();
void A_LightBeamReset();
void A_MineExplode();
void A_MineRange();
void A_ConnectToGround();
void A_SpawnParticleRelative();
void A_MultiShotDist();
void A_WhoCaresIfYourSonIsABee();
void A_ParentTriesToSleep();
void A_CryingToMomma();
void A_CheckFlags2();
void A_Boss5FindWaypoint();
void A_DoNPCSkid();
void A_DoNPCPain();
void A_PrepareRepeat();
void A_Boss5ExtraRepeat();
void A_Boss5Calm();
void A_Boss5CheckOnGround();
void A_Boss5CheckFalling();
void A_Boss5PinchShot();
void A_Boss5MakeItRain();
void A_Boss5MakeJunk();
void A_LookForBetter();
void A_Boss5BombExplode();
void A_TNTExplode();
void A_DebrisRandom();
void A_TrainCameo();
void A_TrainCameo2();
void A_CanarivoreGas();
void A_KillSegments();
void A_SnapperSpawn();
void A_SnapperThinker();
void A_SaloonDoorSpawn();
void A_MinecartSparkThink();
void A_ModuloToState();
void A_LavafallRocks();
void A_LavafallLava();
void A_FallingLavaCheck();
void A_FireShrink();
void A_SpawnPterabytes();
void A_PterabyteHover();
void A_RolloutSpawn();
void A_RolloutRock();
void A_DragonbomberSpawn();
void A_DragonWing();
void A_DragonSegment();
void A_ChangeHeight();

//
// SRB2Kart
//
void A_JawzExplode();
void A_SSMineSearch();
void A_SSMineExplode();
void A_SSMineFlash();
void A_LandMineExplode();
void A_LandMineExplode();
void A_BallhogExplode();
void A_SpecialStageBombExplode();
void A_LightningFollowPlayer();
void A_FZBoomFlash();
void A_FZBoomSmoke();
void A_RandomShadowFrame();
void A_RoamingShadowThinker();
void A_MayonakaArrow();
void A_ReaperThinker();
void A_MementosTPParticles();
void A_FlameShieldPaper();
void A_InvincSparkleRotate();
void A_SpawnItemDebrisCloud();
void A_RingShooterFace();
void A_SpawnSneakerPanel();
void A_BlendEyePuyoHack();

extern boolean actionsoverridden[NUMACTIONS];

// ratio of states to sprites to mobj types is roughly 6 : 1 : 1
#define NUMMOBJFREESLOTS 1024
#define NUMSPRITEFREESLOTS NUMMOBJFREESLOTS
#define NUMSTATEFREESLOTS (NUMMOBJFREESLOTS*8)

// Hey, moron! If you change this table, don't forget about sprnames in info.c and the sprite lights in hw_light.c!
typedef enum sprite
{
	SPR_NULL, // invisible object
	SPR_NONE, // invisible but still rendered
	SPR_UNKN,

	SPR_THOK, // Thok! mobj
	SPR_PLAY,
	SPR_KART,
	SPR_TIRE,

	// Enemies
	SPR_POSS, // Crawla (Blue)
	SPR_SPOS, // Crawla (Red)
	SPR_FISH, // SDURF
	SPR_BUZZ, // Buzz (Gold)
	SPR_RBUZ, // Buzz (Red)
	SPR_JETB, // Jetty-Syn Bomber
	SPR_JETG, // Jetty-Syn Gunner
	SPR_CCOM, // Crawla Commander
	SPR_DETN, // Deton
	SPR_SKIM, // Skim mine dropper
	SPR_TRET, // Industrial Turret
	SPR_TURR, // Pop-Up Turret
	SPR_SHRP, // Sharp
	//SPR_CRAB, // Crushstacean
	SPR_CR2B, // Banpyura
	SPR_CSPR, // Banpyura spring
	SPR_JJAW, // Jet Jaw
	SPR_SNLR, // Snailer
	SPR_VLTR, // BASH
	SPR_PNTY, // Pointy
	SPR_ARCH, // Robo-Hood
	SPR_CBFS, // Castlebot Facestabber
	SPR_STAB, // Castlebot Facestabber spear aura
	SPR_SPSH, // Egg Guard
	SPR_ESHI, // Egg Guard's shield
	SPR_GSNP, // Green Snapper
	SPR_GSNL, // Green Snapper leg
	SPR_GSNH, // Green Snapper head
	SPR_MNUS, // Minus
	SPR_MNUD, // Minus dirt
	SPR_SSHL, // Spring Shell
	SPR_UNID, // Unidus
	SPR_CANA, // Canarivore
	SPR_CANG, // Canarivore gas
	SPR_PYRE, // Pyre Fly
	SPR_PTER, // Pterabyte
	SPR_DRAB, // Dragonbomber

	// Generic Boss Items
	SPR_JETF, // Boss jet fumes

	// Boss 1 (Greenflower)
	SPR_EGGM, // Boss 1
	SPR_EGLZ, // Boss 1 Junk

	// Boss 2 (Techno Hill)
	SPR_EGGN, // Boss 2
	SPR_TANK, // Boss 2 Junk
	SPR_GOOP, // Boss 2 Goop

	// Boss 3 (Deep Sea)
	SPR_EGGO, // Boss 3
	SPR_SEBH, // Boss 3 Junk
	SPR_FAKE, // Boss 3 Fakemobile
	SPR_SHCK, // Boss 3 Shockwave

	// Boss 4 (Castle Eggman)
	SPR_EGGP,
	SPR_EFIR, // Boss 4 jet flame
	SPR_EGR1, // Boss 4 Spectator Eggrobo

	// Boss 5 (Arid Canyon)
	SPR_FANG, // replaces EGGQ
	SPR_BRKN,
	SPR_WHAT,
	SPR_VWRE,
	SPR_PROJ, // projector light
	SPR_FBOM,
	SPR_FSGN,
	SPR_BARX, // bomb explosion (also used by barrel)
	SPR_BARD, // bomb dust (also used by barrel)

	// Boss 6 (Red Volcano)
	SPR_EGGR,

	// Boss 7 (Dark City)
	SPR_BGOO, // Goop
	SPR_BMSL,

	// Boss 8 (Egg Rock)
	SPR_EGGT,

	// Cy-Brak-Demon; uses SPR_BRAK as well, but has some extras
	SPR_RCKT, // Rockets!
	SPR_ELEC, // Electricity!
	SPR_TARG, // Targeting reticules!
	SPR_NPLM, // Big napalm bombs!
	SPR_MNPL, // Mini napalm bombs!

	// Metal Sonic
	SPR_METL,
	SPR_MSCF,
	SPR_MSCB,

	// Collectible Items
	SPR_RING,
	SPR_DEBT,
	SPR_TRNG, // Team Rings
	SPR_TOKE, // Special Stage Token
	SPR_RFLG, // Red CTF Flag
	SPR_BFLG, // Blue CTF Flag
	SPR_BSPH, // Sphere
	SPR_NCHP, // NiGHTS chip
	SPR_NSTR, // NiGHTS star
	SPR_EMBM, // Emblem
	SPR_SPCN, // Spray Can
	SPR_MMSH, // Ancient Shrine
	SPR_MORB, // One Morbillion
	SPR_EMRC, // Chaos Emeralds
	SPR_SEMR, // Super Emeralds
	SPR_ESPK,

	// Prison Egg Drops
	SPR_ALTM,

	// Interactive Objects
	SPR_BBLS, // water bubble source
	SPR_SIGN, // Level end sign
	SPR_SPIK, // Spike Ball
	SPR_SFLM, // Spin fire
	SPR_TFLM, // Spin fire (team)
	SPR_USPK, // Floor spike
	SPR_WSPK, // Wall spike
	SPR_WSPB, // Wall spike base
	SPR_BMNE, // Big floating mine
	SPR_PUMI, // Rollout Rock

	// Monitor Boxes
	SPR_MSTV, // MiSc TV sprites
	SPR_XLTV, // eXtra Large TV sprites

	SPR_TRRI, // Red team:  10 RIngs
	SPR_TBRI, // Blue team: 10 RIngs

	SPR_TVRI, // 10 RIng
	SPR_TVPI, // PIty shield
	SPR_TVAT, // ATtraction shield
	SPR_TVFO, // FOrce shield
	SPR_TVAR, // ARmageddon shield
	SPR_TVWW, // WhirlWind shield
	SPR_TVEL, // ELemental shield
	SPR_TVSS, // Super Sneakers
	SPR_TVIV, // InVincibility
	SPR_TV1U, // 1Up
	SPR_TV1P, // 1uP (textless)
	SPR_TVEG, // EGgman
	SPR_TVMX, // MiXup
	SPR_TVMY, // MYstery
	SPR_TVGV, // GraVity boots
	SPR_TVRC, // ReCycler
	SPR_TV1K, // 1,000 points  (1 K)
	SPR_TVTK, // 10,000 points (Ten K)
	SPR_TVFL, // FLame shield
	SPR_TVBB, // BuBble shield
	SPR_TVZP, // Thunder shield (ZaP)

	// Projectiles
	SPR_MISL,
	SPR_LASR, // GFZ3 laser
	SPR_LASF, // GFZ3 laser flames
	SPR_TORP, // Torpedo
	SPR_ENRG, // Energy ball
	SPR_MINE, // Skim mine
	SPR_JBUL, // Jetty-Syn Bullet
	SPR_TRLS,
	SPR_CBLL, // Cannonball
	SPR_AROW, // Arrow
	SPR_CFIR, // Colored fire of various sorts

	// The letter
	SPR_LETR,

	// Greenflower Scenery
	SPR_FWR1,
	SPR_FWR2, // GFZ Sunflower
	SPR_FWR3, // GFZ budding flower
	SPR_FWR4,
	SPR_BUS1, // GFZ Bush w/ berries
	SPR_BUS2, // GFZ Bush w/o berries
	SPR_BUS3, // GFZ Bush w/ BLUE berries
	// Trees (both GFZ and misc)
	SPR_TRE1, // GFZ
	SPR_TRE2, // Checker
	SPR_TRE3, // Frozen Hillside
	SPR_TRE4, // Polygon
	SPR_TRE5, // Bush tree
	SPR_TRE6, // Spring tree

	// Techno Hill Scenery
	SPR_THZP, // THZ1 Steam Flower
	SPR_FWR5, // THZ1 Spin flower (red)
	SPR_FWR6, // THZ1 Spin flower (yellow)
	SPR_THZT, // Steam Whistle tree/bush
	SPR_ALRM, // THZ2 Alarm

	// Deep Sea Scenery
	SPR_GARG, // Deep Sea Gargoyle
	SPR_SEWE, // Deep Sea Seaweed
	SPR_DRIP, // Dripping water
	SPR_CORL, // Coral
	SPR_BCRY, // Blue Crystal
	SPR_KELP, // Kelp
	SPR_ALGA, // Animated algae top
	SPR_ALGB, // Animated algae segment
	SPR_DSTG, // DSZ Stalagmites
	SPR_LIBE, // DSZ Light beam

	// Castle Eggman Scenery
	SPR_CHAN, // CEZ Chain
	SPR_FLAM, // Flame
	SPR_ESTA, // Eggman esta una estatua!
	SPR_SMCH, // Small Mace Chain
	SPR_BMCH, // Big Mace Chain
	SPR_SMCE, // Small Mace
	SPR_BMCE, // Big Mace
	SPR_YSPB, // Yellow spring on a ball
	SPR_RSPB, // Red spring on a ball
	SPR_SFBR, // Small Firebar
	SPR_BFBR, // Big Firebar
	SPR_BANR, // Banner/pole
	SPR_PINE, // Pine Tree
	SPR_CEZB, // Bush
	SPR_CNDL, // Candle/pricket
	SPR_FLMH, // Flame holder
	SPR_CTRC, // Fire torch
	SPR_CFLG, // Waving flag/segment
	SPR_CSTA, // Crawla statue
	SPR_CBBS, // Facestabber statue
	SPR_CABR, // Brambles

	// Arid Canyon Scenery
	SPR_BTBL, // Big tumbleweed
	SPR_STBL, // Small tumbleweed
	SPR_CACT, // Cacti
	SPR_WWSG, // Caution Sign
	SPR_WWS2, // Cacti Sign
	SPR_WWS3, // Sharp Turn Sign
	SPR_OILL, // Oil lamp
	SPR_OILF, // Oil lamp flare
	SPR_BARR, // TNT barrel
	SPR_REMT, // TNT proximity shell
	SPR_TAZD, // Dust devil
	SPR_ADST, // Arid dust
	SPR_MCRT, // Minecart
	SPR_MCSP, // Minecart spark
	SPR_SALD, // Saloon door
	SPR_TRAE, // Train cameo locomotive
	SPR_TRAI, // Train cameo wagon
	SPR_STEA, // Train steam

	// Red Volcano Scenery
	SPR_FLME, // Flame jet
	SPR_DFLM, // Blade's flame
	SPR_LFAL, // Lavafall
	SPR_JPLA, // Jungle palm
	SPR_TFLO, // Torch flower
	SPR_WVIN, // Wall vines

	// Dark City Scenery

	// Egg Rock Scenery

	// Christmas Scenery
	SPR_XMS1, // Christmas Pole
	SPR_XMS2, // Candy Cane
	SPR_XMS3, // Snowman
	SPR_XMS4, // Lamppost
	SPR_XMS5, // Hanging Star
	SPR_XMS6, // Mistletoe
	SPR_FHZI, // FHZ Ice
	SPR_ROSY,

	// Halloween Scenery
	SPR_PUMK, // Pumpkins
	SPR_HHPL, // Dr Seuss Trees
	SPR_SHRM, // Mushroom
	SPR_HHZM, // Misc

	// Azure Temple Scenery
	SPR_BGAR, // ATZ Gargoyles
	SPR_RCRY, // ATZ Red Crystal (Target)
	SPR_CFLM, // Green torch flame

	// Botanic Serenity Scenery
	SPR_BSZ1, // Tall flowers
	SPR_BSZ2, // Medium flowers
	SPR_BSZ3, // Small flowers
	//SPR_BSZ4, -- Tulips
	SPR_BST1, // Red tulip
	SPR_BST2, // Purple tulip
	SPR_BST3, // Blue tulip
	SPR_BST4, // Cyan tulip
	SPR_BST5, // Yellow tulip
	SPR_BST6, // Orange tulip
	SPR_BSZ5, // Cluster of Tulips
	SPR_BSZ6, // Bush
	SPR_BSZ7, // Vine
	SPR_BSZ8, // Misc things

	// Misc Scenery
	SPR_STLG, // Stalagmites
	SPR_DBAL, // Disco

	// Powerup Indicators
	SPR_ARMA, // Armageddon Shield Orb
	SPR_ARMF, // Armageddon Shield Ring, Front
	SPR_ARMB, // Armageddon Shield Ring, Back
	SPR_WIND, // Whirlwind Shield Orb
	SPR_MAGN, // Attract Shield Orb
	SPR_ELEM, // Elemental Shield Orb
	SPR_FORC, // Force Shield Orb
	SPR_PITY, // Pity Shield Orb
	SPR_FIRS, // Flame Shield Orb
	//SPR_BUBS, // Bubble Shield Orb
	SPR_ZAPS, // Thunder Shield Orb
	SPR_IVSP, // invincibility sparkles
	SPR_SSPK, // Super Sonic Spark

	SPR_GOAL, // Special Stage goal (here because lol NiGHTS)

	// Flickies
	SPR_FBUB, // Flicky-sized bubble
	SPR_FL01, // Bluebird
	SPR_FL02, // Rabbit
	SPR_FL03, // Chicken
	SPR_FL04, // Seal
	SPR_FL05, // Pig
	SPR_FL06, // Chipmunk
	SPR_FL07, // Penguin
	SPR_FL08, // Fish
	SPR_FL09, // Ram
	SPR_FL10, // Puffin
	SPR_FL11, // Cow
	SPR_FL12, // Rat
	SPR_FL13, // Bear
	SPR_FL14, // Dove
	SPR_FL15, // Cat
	SPR_FL16, // Canary
	SPR_FS01, // Spider
	SPR_FS02, // Bat

	// Springs
	SPR_FANS, // Fan
	SPR_STEM, // Steam riser
	SPR_BMPR, // Bumpers
	SPR_BLON, // Balloons
	SPR_SPVY, // Yellow Vertical Spring
	SPR_SPVR, // Red Vertical Spring
	SPR_SPVB, // Blue Vertical Spring
	SPR_SPVG, // Grey Vertical Spring
	SPR_SPDY, // Yellow Diagonal Spring
	SPR_SPDR, // Red Diagonal Spring
	SPR_SPDB, // Blue Diagonal Spring
	SPR_SPDG, // Grey Diagonal Spring
	SPR_SPHY, // Yellow Horizontal Spring
	SPR_SPHR, // Red Horizontal Spring
	SPR_SPHB, // Blue Horizontal Spring
	SPR_SPHG, // Grey Horizontal Spring
	SPR_POGS, // Pogo Spring
	SPR_BSTY, // Yellow Booster
	SPR_BSTR, // Red Booster

	// Environmental Effects
	SPR_RAIN, // Rain
	SPR_SNO1, // Snowflake
	SPR_SNO2, // Blizzard Snowball
	SPR_SPLH, // Water Splish
	SPR_LSPL, // Lava Splish
	SPR_SPLA, // Water Splash
	SPR_SMOK,
	SPR_BUBL, // Bubble
	SPR_WZAP,
	SPR_DUST, // Spindash dust
	SPR_FPRT, // Spindash dust (flame)
	SPR_TFOG, // Teleport Fog
	SPR_SEED, // Sonic CD flower seed
	SPR_PRTL, // Particle (for fans, etc.)

	// Game Indicators
	SPR_SCOR, // Score logo
	SPR_DRWN, // Drowning Timer
	SPR_FLII, // AI flight indicator
	SPR_LCKN, // Target
	SPR_TTAG, // Tag Sign
	SPR_GFLG, // Got Flag sign
	SPR_FNSF, // Finish flag

	SPR_CORK,
	SPR_LHRT,

	// Ring Weapons
	SPR_RRNG, // Red Ring
	SPR_RNGB, // Bounce Ring
	SPR_RNGR, // Rail Ring
	SPR_RNGI, // Infinity Ring
	SPR_RNGA, // Automatic Ring
	SPR_RNGE, // Explosion Ring
	SPR_RNGS, // Scatter Ring
	SPR_RNGG, // Grenade Ring

	SPR_PIKB, // Bounce Ring Pickup
	SPR_PIKR, // Rail Ring Pickup
	SPR_PIKA, // Automatic Ring Pickup
	SPR_PIKE, // Explosion Ring Pickup
	SPR_PIKS, // Scatter Ring Pickup
	SPR_PIKG, // Grenade Ring Pickup

	SPR_TAUT, // Thrown Automatic Ring
	SPR_TGRE, // Thrown Grenade Ring
	SPR_TSCR, // Thrown Scatter Ring

	// Mario-specific stuff
	SPR_COIN,
	SPR_CPRK,
	SPR_GOOM,
	SPR_BGOM,
	SPR_FFWR,
	SPR_FBLL,
	SPR_SHLL,
	SPR_PUMA,
	SPR_HAMM,
	SPR_KOOP,
	SPR_BFLM,
	SPR_MAXE,
	SPR_MUS1,
	SPR_MUS2,
	SPR_TOAD,

	// NiGHTS Stuff
	SPR_NDRN, // NiGHTS drone
	SPR_NSPK, // NiGHTS sparkle
	SPR_NBMP, // NiGHTS Bumper
	SPR_HOOP, // NiGHTS hoop sprite
	SPR_NSCR, // NiGHTS score sprite
	SPR_NPRU, // Nights Powerups
	SPR_CAPS, // Capsule thingy for NiGHTS
	SPR_IDYA, // Ideya
	SPR_NTPN, // Nightopian
	SPR_SHLP, // Shleep

	// Secret badniks and hazards, shhhh
	SPR_PENG,
	SPR_POPH,
	SPR_HIVE,
	SPR_BUMB,
	SPR_BBUZ,
	SPR_FMCE,
	SPR_HMCE,
	SPR_CACO,
	SPR_BAL2,
	SPR_SBOB,
	SPR_SBFL,
	SPR_SBSK,
	SPR_HBAT,

	// Debris
	SPR_SPRK, // Sparkle
	SPR_BOM1, // Robot Explosion
	SPR_BOM2, // Boss Explosion 1
	SPR_BOM3, // Boss Explosion 2
	SPR_BOM4, // Underwater Explosion
	SPR_BMNB, // Mine Explosion
	SPR_LSSJ, // My ki is overflowing!!

	// Crumbly rocks
	SPR_ROIA,
	SPR_ROIB,
	SPR_ROIC,
	SPR_ROID,
	SPR_ROIE,
	SPR_ROIF,
	SPR_ROIG,
	SPR_ROIH,
	SPR_ROII,
	SPR_ROIJ,
	SPR_ROIK,
	SPR_ROIL,
	SPR_ROIM,
	SPR_ROIN,
	SPR_ROIO,
	SPR_ROIP,

	// Level debris
	SPR_GFZD, // GFZ debris
	SPR_BRIC, // Bricks
	SPR_WDDB, // Wood Debris
	SPR_BRIR, // CEZ3 colored bricks
	SPR_BRIB,
	SPR_BRIY,

	// Gravity Well Objects
	SPR_GWLG,
	SPR_GWLR,

	// SRB2Kart
	SPR_RNDM, // Random Item Box
	SPR_SBOX, // Sphere Box (for Battle)
	SPR_RBOX, // Ring Box
	SPR_RPOP, // Random Item Box Pop
	SPR_ITRI, // Item Box Debris
	SPR_ITPA, // Paper item backdrop
	SPR_SGNS, // Signpost sparkle
	SPR_FAST, // Speed boost trail
	SPR_DSHR, // Speed boost dust release
	SPR_BOST, // Sneaker booster flame
	SPR_BOSM, // Sneaker booster smoke
	SPR_KFRE, // Sneaker fire trail
	SPR_KINV, // Lighter invincibility sparkle trail
	SPR_KINB, // Darker invincibility sparkle trail
	SPR_KINF, // Invincibility flash
	SPR_INVI, // Invincibility speedlines
	SPR_ICAP, // Item capsules
	SPR_IMON, // Item Monitor
	SPR_MGBX, // Heavy Magician transform box
	SPR_MGBT, // Heavy Magician transform box top
	SPR_MGBB, // Heavy Magician transform box bottom
	SPR_MSHD, // Item Monitor Big Shard
	SPR_IMDB, // Item Monitor Small Shard (Debris)
	SPR_MTWK, // Item Monitor Glass Twinkle

	SPR_SLPT, // Wavedash indicator
	SPR_TRBS, // Trickdash indicator

	SPR_IWHP, // Instawhip
	SPR_WPRE, // Instawhip Recharge
	SPR_WPRJ, // Instawhip Reject
	SPR_GRNG, // Guard ring
	SPR_GBDY, // Guard body

	SPR_TRC1, // Charge aura
	SPR_TRC2, // Charge fall
	SPR_TRC3, // Charge flicker/sparks
	SPR_TRC4, // Charge release
	SPR_TRC5, // Charge extra

	SPR_DHND, // Servant Hand

	SPR_HORN, // Horncode

	SPR_WIPD, // Wipeout dust trail
	SPR_DRIF, // Drift Sparks
	SPR_BDRF, // Brake drift sparks
	SPR_BRAK, // Brake brak
	SPR_DRWS, // Drift dust sparks
	SPR_DREL, // Drift electricity
	SPR_DRES, // Drift electric sparks
	SPR_JANK, // Stair janking sparks
	SPR_HFX1, // Hitlag stage 1
	SPR_HFX2, // Hitlag stage 2
	SPR_HFX3, // Hitlag stage 3
	SPR_HFX4, // Hitlag stage 4
	SPR_HFX5, // Hitlag stage 5
	SPR_HFX6, // Hitlag stage 6
	SPR_HFX8, // Hitlag stage 8
	SPR_HFX9, // Hitlag stage 9
	SPR_HFXX, // Hitlag stage 10

	// Kart Items
	SPR_RSHE, // Rocket sneaker
	SPR_FITM, // Eggman Monitor
	SPR_BANA, // Banana Peel
	SPR_BAND, // Banana Peel death particles
	SPR_ORBN, // Orbinaut
	SPR_JAWZ, // Jawz
	SPR_SSMN, // SS Mine
	SPR_KRBM, // SS Mine BOOM
	SPR_LNDM, // Land Mine
	SPR_DTRG, // Drop Target
	SPR_BHOG, // Ballhog
	SPR_BHBM, // Ballhog BOOM
	SPR_SPBM, // Self-Propelled Bomb
	SPR_TRIS, // SPB Manta Ring start
	SPR_TRNQ, // SPB Manta Ring loop
	SPR_THNS, // Thunder Shield
	SPR_BUBS, // Bubble Shield (not Bubs)
	SPR_BWVE, // Bubble Shield waves
	SPR_FLMS, // Flame Shield
	SPR_FLMD, // Flame Shield dash
	SPR_FLMP, // Flame Shield paper sprites
	SPR_FLML, // Flame Shield speed lines
	SPR_FLMF, // Flame Shield flash
	SPR_GTOP, // Marble Garden Zone Spinning Top
	SPR_GTAR, // Garden Top Arrow
	SPR_HYUU, // Hyudoro
	SPR_GRWP, // Grow
	SPR_POHB, // Shrink Poh-Bee
	SPR_POHC, // Shrink Poh-Bee chain
	SPR_SHRG, // Shrink gun
	SPR_SHRL, // Shrink laser
	SPR_SINK, // Kitchen Sink
	SPR_SITR, // Kitchen Sink Trail
	SPR_KBLN, // Battle Mode Bumper
	SPR_BEXC, // Battle Bumper Explosion: Crystal
	SPR_BEXS, // Battle Bumper Explosion: Shell
	SPR_BDEB, // Battle Bumper Explosion: Debris
	SPR_BEXB, // Battle Bumper Explosion: Blast
	SPR_TWBS, // Tripwire Boost
	SPR_TWBT, // Tripwire BLASTER
	SPR_SMLD, // Smooth landing

	// Trick Effects
	SPR_TRK1,
	SPR_TRK2,
	SPR_TRK3,
	SPR_TRK4,
	SPR_TRK5,
	SPR_TRK6,
	SPR_TRK7,

	SPR_TIRG, // Tire grabbers
	SPR_RSHT, // DEZ Ring Shooter

	SPR_DEZL, // DEZ Laser respawn

	// Additional Kart Objects
	SPR_POKE, // Pokey
	SPR_AUDI, // Audience members
	SPR_DECO, // Old 1.0 Kart Decoratives + New misc ones
	SPR_SNES, // Sprites for SNES remake maps
	SPR_GBAS, // Sprites for GBA remake maps
	SPR_SPRS, // Sapphire Coast Spring Shell
	SPR_BUZB, // Sapphire Coast Buzz Mk3
	SPR_CHOM, // Sapphire Coast Chomper
	SPR_SACO, // Sapphire Coast Fauna
	SPR_CRAB, // Crystal Abyss mobs
	SPR_BRNG, // Chaotix Big Ring

	// Ark Arrows
	SPR_SYM0,
	SPR_SYM1,
	SPR_SYM2,
	SPR_SYM3,
	SPR_SYM4,
	SPR_SYM5,
	SPR_SYM6,
	SPR_SYM7,
	SPR_SYM8,
	SPR_SYM9,
	SPR_SYMA,
	SPR_SYMB,
	SPR_SYMC,
	SPR_SYMD,
	SPR_SYME,
	SPR_SYMF,
	SPR_SYMG,
	SPR_SYMH,
	SPR_SYMI,
	SPR_SYMJ,
	SPR_SYMK,
	SPR_SYML,
	SPR_SYMM,
	SPR_SYMN,
	SPR_SYMO,
	SPR_SYMP,
	SPR_SYMQ,
	SPR_SYMR,
	SPR_SYMS,
	SPR_SYMT,
	SPR_SYMU,
	SPR_SYMV,
	SPR_SYMW,
	SPR_SYMX,
	SPR_SYMY,
	SPR_SYMZ,
	SPR_ARK0,
	SPR_ARK1,
	SPR_ARK2,
	SPR_ARK3,
	SPR_ARK4,
	SPR_ARK5,


	SPR_BUMP, // Player/shell bump
	SPR_FLEN, // Shell hit graphics stuff
	SPR_CLAS, // items clash
	SPR_PSHW, // thrown indicator
	SPR_ISTA, // instashield layer A
	SPR_ISTB, // instashield layer B

	SPR_PWCL, // Invinc/grow clash VFX
	SPR_GBRK, // Guard break

	SPR_ARRO, // player arrows
	SPR_ITEM,
	SPR_ITMO,
	SPR_ITMI,
	SPR_ITMN,
	SPR_PWRB,
	SPR_RBOW, // power-up aura
	SPR_WANT,

	SPR_PBOM, // player bomb

	SPR_HIT1, // battle points
	SPR_HIT2, // battle points
	SPR_HIT3, // battle points

	SPR_RETI, // player reticule

	SPR_AIDU,

	SPR_KSPK, // Spark radius for the lightning shield
	SPR_LZI1, // Lightning that falls on the player for lightning shield
	SPR_LZI2, // ditto
	SPR_KLIT, // You have a twisted mind. But this actually is for the diagonal lightning.

	SPR_FZSM, // F-Zero NO CONTEST explosion
	SPR_FZBM,

	// Dash Rings
	SPR_RAIR,

	// Sneaker Panels
	SPR_BSTP,
	SPR_BSTS,
	SPR_BSTT,

	// Various plants
	SPR_SBUS,

	SPR_MARB, // Marble Zone sprites
	SPR_FUFO, // CD Special Stage UFO (don't ask me why it begins with an F)

	SPR_RUST, // Rusty Rig sprites

	SPR_VAPE, // Volcanic Valley

	// Hill Top Zone
	SPR_HTZA,
	SPR_HTZB,

	// Ports of gardens
	SPR_SGVA,
	SPR_SGVB,
	SPR_SGVC,
	SPR_PGTR,
	SPR_PGF1,
	SPR_PGF2,
	SPR_PGF3,
	SPR_PGBH,
	SPR_DPLR,

	// Midnight Channel stuff:
	SPR_SPTL,	// Spotlight
	SPR_ENM1,	// Shadows (Roaming and static)
	SPR_GARU,	// Wind attack roaming shadows use.
	SPR_MARR,	// Mayonaka Arrow

	//Mementos stuff:
	SPR_REAP,

	SPR_JITB,	// Jack In The Box

	// Color Drive stuff:
	SPR_CDMO,
	SPR_CDBU,

	// Daytona Speedway
	SPR_DPIN,

	// Egg Zeppelin
	SPR_PPLR,

	// Desert Palace
	SPR_DPPT,

	// Aurora Atoll
	SPR_AATR,
	SPR_COCO,

	// Barren Badlands
	SPR_BDST,
	SPR_FROG,
	SPR_CBRA,
	SPR_HOLE,
	SPR_BBRA,

	// Eerie Grove
	SPR_EGFG,

	// Chaos Chute
	SPR_SARC,
	SPR_SSBM,

	// Hanagumi Hall
	SPR_HGSP,
	SPR_HGC0,
	SPR_HGCA,
	SPR_HGCB,
	SPR_HGCC,
	SPR_HGCD,
	SPR_HGCE,
	SPR_HGCF,
	SPR_HGCG,

	// SMK ports
	SPR_SMKP,
	SPR_MTYM,
	SPR_THWP,
	SPR_SNOB,
	SPR_ICEB,

	// Ezo's maps - many single-use sprites!
	SPR_ECND,
	SPR_DOCH,
	SPR_DUCK,
	SPR_GTRE,
	SPR_CHES,
	SPR_CHIM,
	SPR_DRGN,
	SPR_LZMN,
	SPR_PGSS,
	SPR_ZTCH,
	SPR_MKMA,
	SPR_MKMP,
	SPR_RTCH,
	SPR_BOWL,
	SPR_BOWH,
	SPR_BRRL,
	SPR_BRRR,
	SPR_HRSE,
	SPR_TOAH,
	SPR_BFRT,
	SPR_OFRT,
	SPR_RFRT,
	SPR_PFRT,
	SPR_ASPK,
	SPR_HBST,
	SPR_HBSO,
	SPR_HBSF,
	SPR_WBLZ,
	SPR_WBLN,

	SPR_TUST,
	SPR_TULE,

	SPR_FWRK,
	SPR_MXCL,
	SPR_RGSP,
	SPR_LENS,
	SPR_DRAF,
	SPR_GRES,

	SPR_OTBU,
	SPR_OTLS,
	SPR_OTCP,

	SPR_DBOS, // Drift boost flame

	SPR_WAYP,
	SPR_EGOO,

	SPR_WTRL, // Water Trail

	SPR_GCHA, // follower: generic chao
	SPR_CHEZ, // follower: cheese

	SPR_DBCL, // Drift boost clip
	SPR_DBNC, // Drift boost clip's sparks
	SPR_DBST, // Drift boost plume

	SPR_SDDS, // Spindash dust
	SPR_SDWN, // Spindash wind
	SPR_EBRK, // Soft Landing / Ebrake aura stuff.
	SPR_HMTR, // downwards line
	SPR_HBUB, // HOLD! Bubble

	SPR_TRCK,

	SPR_FLBM, // Finish line beam

	SPR_UFOB,
	SPR_UFOA,
	SPR_UFOS,
	SPR_SSCA,
	SPR_SSCB,

	SPR_UQMK,

	SPR_GBOM,
	SPR_GCHX,

	SPR_3DFR,

	SPR_BUFO, // Battle/Power-UP UFO

	SPR_CPT1, // Checkpoint Orb
	SPR_CPT2, // Checkpoint Stick
	SPR_CPT3, // Checkpoint Base

	SPR_RDRD, // rideroid
	SPR_RDRA, // rideroid node sprites
	SPR_RDRC,
	SPR_RDRL,

	SPR_LSZB,	// eggman ball.

	SPR_DLZH,	// DLZ Hover
	SPR_DLZR,	// DLZ Rocket
	SPR_DLZS,	// DLZ Seasaw
	SPR_DLZA,	// Helper arrows for rocket

	SPR_WPWL,	// turbine
	SPR_WPZF,	// fountain
	SPR_WPZK,	// klagen

	SPR_SA2S, // SA2-style Ball Switch

	SPR_STRG, // Spiked Target

	SPR_BLEA, // m'A'in unit
	SPR_BLEB, // o'B'server
	SPR_BLEC, // 'C'lear glass
	SPR_BLED, // shiel'D'
	SPR_BLEE, // 'E'ggbeater
	SPR_BLEF, // 'F'lamejet
	SPR_BLEG, // 'G'enerator

	// Puyo hazards
	SPR_PUYA,
	SPR_PUYB,
	SPR_PUYC,
	SPR_PUYD,
	SPR_PUYE,

	SPR_MGSH, // Mega Barrier

	// GPZ Seasaw
	SPR_GPPS,
	SPR_GPZS,

	// Gust Planet Trees
	SPR_GPTB,
	SPR_GPTM,
	SPR_GPTS,

	SPR_GGZ1,
	SPR_GGZ2,
	SPR_GGZ3,
	SPR_GGZ6,
	SPR_GGZ7,
	SPR_GGZ8,
	SPR_FBTN,
	SPR_SFTR,

	SPR_SABX,
	SPR_ICBL,

	SPR_BSSP,
	SPR_BSPB,
	SPR_BSPR,
	SPR_BSSR,
	SPR_BLMS,
	SPR_BLMM,
	SPR_BLML,
	SPR_BSWL,
	SPR_BSWC,

	SPR_LCLA,

	// First person view sprites; this is a sprite so that it can be replaced by a specialized MD2 draw later
	SPR_VIEW,

	SPR_FIRSTFREESLOT,
	SPR_LASTFREESLOT = SPR_FIRSTFREESLOT + NUMSPRITEFREESLOTS - 1,
	NUMSPRITES
} spritenum_t;

// Make sure to be conscious of FF_FRAMEMASK and the fact sprite2 is stored as a UINT8 whenever you change this table.
// Currently, FF_FRAMEMASK is 0xff, or 255 - but the second half is used by FF_SPR2SUPER, so the limitation is 0x7f.
// Since this is zero-based, there can be at most 128 different SPR2_'s without changing that.
typedef enum playersprite
{
	SPR2_STIN = 0, SPR2_STIL, SPR2_STIR,
	SPR2_STGL, SPR2_STGR, SPR2_STLL, SPR2_STLR,
	SPR2_SLWN, SPR2_SLWL, SPR2_SLWR,
	SPR2_SLGL, SPR2_SLGR, SPR2_SLLL, SPR2_SLLR,
	SPR2_FSTN, SPR2_FSTL, SPR2_FSTR,
	SPR2_FSGL, SPR2_FSGR, SPR2_FSLL, SPR2_FSLR,
	SPR2_DRLN, SPR2_DRLO, SPR2_DRLI,
	SPR2_DRRN, SPR2_DRRO, SPR2_DRRI,
	SPR2_SPIN,
	SPR2_DEAD,
	SPR2_SIGN, SPR2_SIGL,
	SPR2_XTRA,
	SPR2_TALK,

	SPR2_FIRSTFREESLOT,
	SPR2_LASTFREESLOT = 0x7f,
	NUMPLAYERSPRITES
} playersprite_t;

typedef enum state
{
	S_NULL,
	S_UNKNOWN,
	S_INVISIBLE, // state for invisible sprite

	S_SPAWNSTATE,
	S_SEESTATE,
	S_MELEESTATE,
	S_MISSILESTATE,
	S_DEATHSTATE,
	S_XDEATHSTATE,
	S_RAISESTATE,

	S_THOK,
	S_SHADOW,

	S_KART_STILL,
	S_KART_STILL_L,
	S_KART_STILL_R,
	S_KART_STILL_GLANCE_L,
	S_KART_STILL_GLANCE_R,
	S_KART_STILL_LOOK_L,
	S_KART_STILL_LOOK_R,
	S_KART_SLOW,
	S_KART_SLOW_L,
	S_KART_SLOW_R,
	S_KART_SLOW_GLANCE_L,
	S_KART_SLOW_GLANCE_R,
	S_KART_SLOW_LOOK_L,
	S_KART_SLOW_LOOK_R,
	S_KART_FAST,
	S_KART_FAST_L,
	S_KART_FAST_R,
	S_KART_FAST_GLANCE_L,
	S_KART_FAST_GLANCE_R,
	S_KART_FAST_LOOK_L,
	S_KART_FAST_LOOK_R,
	S_KART_DRIFT_L,
	S_KART_DRIFT_L_OUT,
	S_KART_DRIFT_L_IN,
	S_KART_DRIFT_R,
	S_KART_DRIFT_R_OUT,
	S_KART_DRIFT_R_IN,
	S_KART_SPINOUT,
	S_KART_DEAD,
	S_KART_SIGN,
	S_KART_SIGL,

	// technically the player goes here but it's an infinite tic state
	S_OBJPLACE_DUMMY,

	S_KART_LEFTOVER,
	S_KART_LEFTOVER_NOTIRES,

	S_KART_TIRE1,
	S_KART_TIRE2,

	// Blue Crawla
	S_POSS_STND,
	S_POSS_RUN1,
	S_POSS_RUN2,
	S_POSS_RUN3,
	S_POSS_RUN4,
	S_POSS_RUN5,
	S_POSS_RUN6,

	// Red Crawla
	S_SPOS_STND,
	S_SPOS_RUN1,
	S_SPOS_RUN2,
	S_SPOS_RUN3,
	S_SPOS_RUN4,
	S_SPOS_RUN5,
	S_SPOS_RUN6,

	// Greenflower Fish
	S_FISH1,
	S_FISH2,
	S_FISH3,
	S_FISH4,

	// Buzz (Gold)
	S_BUZZLOOK1,
	S_BUZZLOOK2,
	S_BUZZFLY1,
	S_BUZZFLY2,

	// Buzz (Red)
	S_RBUZZLOOK1,
	S_RBUZZLOOK2,
	S_RBUZZFLY1,
	S_RBUZZFLY2,

	// Jetty-Syn Bomber
	S_JETBLOOK1,
	S_JETBLOOK2,
	S_JETBZOOM1,
	S_JETBZOOM2,

	// Jetty-Syn Gunner
	S_JETGLOOK1,
	S_JETGLOOK2,
	S_JETGZOOM1,
	S_JETGZOOM2,
	S_JETGSHOOT1,
	S_JETGSHOOT2,

	// Crawla Commander
	S_CCOMMAND1,
	S_CCOMMAND2,
	S_CCOMMAND3,
	S_CCOMMAND4,

	// Deton
	S_DETON1,
	S_DETON2,
	S_DETON3,
	S_DETON4,
	S_DETON5,
	S_DETON6,
	S_DETON7,
	S_DETON8,
	S_DETON9,
	S_DETON10,
	S_DETON11,
	S_DETON12,
	S_DETON13,
	S_DETON14,
	S_DETON15,

	// Skim Mine Dropper
	S_SKIM1,
	S_SKIM2,
	S_SKIM3,
	S_SKIM4,

	// THZ Turret
	S_TURRET,
	S_TURRETFIRE,
	S_TURRETSHOCK1,
	S_TURRETSHOCK2,
	S_TURRETSHOCK3,
	S_TURRETSHOCK4,
	S_TURRETSHOCK5,
	S_TURRETSHOCK6,
	S_TURRETSHOCK7,
	S_TURRETSHOCK8,
	S_TURRETSHOCK9,

	// Popup Turret
	S_TURRETLOOK,
	S_TURRETSEE,
	S_TURRETPOPUP1,
	S_TURRETPOPUP2,
	S_TURRETPOPUP3,
	S_TURRETPOPUP4,
	S_TURRETPOPUP5,
	S_TURRETPOPUP6,
	S_TURRETPOPUP7,
	S_TURRETPOPUP8,
	S_TURRETSHOOT,
	S_TURRETPOPDOWN1,
	S_TURRETPOPDOWN2,
	S_TURRETPOPDOWN3,
	S_TURRETPOPDOWN4,
	S_TURRETPOPDOWN5,
	S_TURRETPOPDOWN6,
	S_TURRETPOPDOWN7,
	S_TURRETPOPDOWN8,

	// Spincushion
	S_SPINCUSHION_LOOK,
	S_SPINCUSHION_CHASE1,
	S_SPINCUSHION_CHASE2,
	S_SPINCUSHION_CHASE3,
	S_SPINCUSHION_CHASE4,
	S_SPINCUSHION_AIM1,
	S_SPINCUSHION_AIM2,
	S_SPINCUSHION_AIM3,
	S_SPINCUSHION_AIM4,
	S_SPINCUSHION_AIM5,
	S_SPINCUSHION_SPIN1,
	S_SPINCUSHION_SPIN2,
	S_SPINCUSHION_SPIN3,
	S_SPINCUSHION_SPIN4,
	S_SPINCUSHION_STOP1,
	S_SPINCUSHION_STOP2,
	S_SPINCUSHION_STOP3,
	S_SPINCUSHION_STOP4,

	// Crushstacean
	S_CRUSHSTACEAN_ROAM1,
	S_CRUSHSTACEAN_ROAM2,
	S_CRUSHSTACEAN_ROAM3,
	S_CRUSHSTACEAN_ROAM4,
	S_CRUSHSTACEAN_ROAMPAUSE,
	S_CRUSHSTACEAN_PUNCH1,
	S_CRUSHSTACEAN_PUNCH2,
	S_CRUSHCLAW_AIM,
	S_CRUSHCLAW_OUT,
	S_CRUSHCLAW_STAY,
	S_CRUSHCLAW_IN,
	S_CRUSHCLAW_WAIT,
	S_CRUSHCHAIN,

	// Banpyura
	S_BANPYURA_ROAM1,
	S_BANPYURA_ROAM2,
	S_BANPYURA_ROAM3,
	S_BANPYURA_ROAM4,
	S_BANPYURA_ROAMPAUSE,
	S_CDIAG1,
	S_CDIAG2,
	S_CDIAG3,
	S_CDIAG4,
	S_CDIAG5,
	S_CDIAG6,
	S_CDIAG7,
	S_CDIAG8,

	// Jet Jaw
	S_JETJAW_ROAM1,
	S_JETJAW_ROAM2,
	S_JETJAW_ROAM3,
	S_JETJAW_ROAM4,
	S_JETJAW_ROAM5,
	S_JETJAW_ROAM6,
	S_JETJAW_ROAM7,
	S_JETJAW_ROAM8,
	S_JETJAW_CHOMP1,
	S_JETJAW_CHOMP2,
	S_JETJAW_CHOMP3,
	S_JETJAW_CHOMP4,
	S_JETJAW_CHOMP5,
	S_JETJAW_CHOMP6,
	S_JETJAW_CHOMP7,
	S_JETJAW_CHOMP8,
	S_JETJAW_CHOMP9,
	S_JETJAW_CHOMP10,
	S_JETJAW_CHOMP11,
	S_JETJAW_CHOMP12,
	S_JETJAW_CHOMP13,
	S_JETJAW_CHOMP14,
	S_JETJAW_CHOMP15,
	S_JETJAW_CHOMP16,
	S_JETJAW_SOUND,

	// Snailer
	S_SNAILER1,
	S_SNAILER_FLICKY,

	// Vulture
	S_VULTURE_STND,
	S_VULTURE_DRIFT,
	S_VULTURE_ZOOM1,
	S_VULTURE_ZOOM2,
	S_VULTURE_STUNNED,

	// Pointy
	S_POINTY1,
	S_POINTYBALL1,

	// Robo-Hood
	S_ROBOHOOD_LOOK,
	S_ROBOHOOD_STAND,
	S_ROBOHOOD_FIRE1,
	S_ROBOHOOD_FIRE2,
	S_ROBOHOOD_JUMP1,
	S_ROBOHOOD_JUMP2,
	S_ROBOHOOD_JUMP3,

	// Castlebot Facestabber
	S_FACESTABBER_STND1,
	S_FACESTABBER_STND2,
	S_FACESTABBER_STND3,
	S_FACESTABBER_STND4,
	S_FACESTABBER_STND5,
	S_FACESTABBER_STND6,
	S_FACESTABBER_CHARGE1,
	S_FACESTABBER_CHARGE2,
	S_FACESTABBER_CHARGE3,
	S_FACESTABBER_CHARGE4,
	S_FACESTABBER_PAIN,
	S_FACESTABBER_DIE1,
	S_FACESTABBER_DIE2,
	S_FACESTABBER_DIE3,
	S_FACESTABBERSPEAR,

	// Egg Guard
	S_EGGGUARD_STND,
	S_EGGGUARD_WALK1,
	S_EGGGUARD_WALK2,
	S_EGGGUARD_WALK3,
	S_EGGGUARD_WALK4,
	S_EGGGUARD_MAD1,
	S_EGGGUARD_MAD2,
	S_EGGGUARD_MAD3,
	S_EGGGUARD_RUN1,
	S_EGGGUARD_RUN2,
	S_EGGGUARD_RUN3,
	S_EGGGUARD_RUN4,

	// Egg Shield for Egg Guard
	S_EGGSHIELD,
	S_EGGSHIELDBREAK,

	// Green Snapper
	S_SNAPPER_SPAWN,
	S_SNAPPER_SPAWN2,
	S_GSNAPPER_STND,
	S_GSNAPPER1,
	S_GSNAPPER2,
	S_GSNAPPER3,
	S_GSNAPPER4,
	S_SNAPPER_XPLD,
	S_SNAPPER_LEG,
	S_SNAPPER_LEGRAISE,
	S_SNAPPER_HEAD,

	// Minus
	S_MINUS_INIT,
	S_MINUS_STND,
	S_MINUS_DIGGING1,
	S_MINUS_DIGGING2,
	S_MINUS_DIGGING3,
	S_MINUS_DIGGING4,
	S_MINUS_BURST0,
	S_MINUS_BURST1,
	S_MINUS_BURST2,
	S_MINUS_BURST3,
	S_MINUS_BURST4,
	S_MINUS_BURST5,
	S_MINUS_POPUP,
	S_MINUS_AERIAL1,
	S_MINUS_AERIAL2,
	S_MINUS_AERIAL3,
	S_MINUS_AERIAL4,

	// Minus dirt
	S_MINUSDIRT1,
	S_MINUSDIRT2,
	S_MINUSDIRT3,
	S_MINUSDIRT4,
	S_MINUSDIRT5,
	S_MINUSDIRT6,
	S_MINUSDIRT7,

	// Spring Shell
	S_SSHELL_STND,
	S_SSHELL_RUN1,
	S_SSHELL_RUN2,
	S_SSHELL_RUN3,
	S_SSHELL_RUN4,
	S_SSHELL_SPRING1,
	S_SSHELL_SPRING2,
	S_SSHELL_SPRING3,
	S_SSHELL_SPRING4,

	// Spring Shell (yellow)
	S_YSHELL_STND,
	S_YSHELL_RUN1,
	S_YSHELL_RUN2,
	S_YSHELL_RUN3,
	S_YSHELL_RUN4,
	S_YSHELL_SPRING1,
	S_YSHELL_SPRING2,
	S_YSHELL_SPRING3,
	S_YSHELL_SPRING4,

	// Unidus
	S_UNIDUS_STND,
	S_UNIDUS_RUN,
	S_UNIDUS_BALL,

	// Canarivore
	S_CANARIVORE_LOOK,
	S_CANARIVORE_AWAKEN1,
	S_CANARIVORE_AWAKEN2,
	S_CANARIVORE_AWAKEN3,
	S_CANARIVORE_GAS1,
	S_CANARIVORE_GAS2,
	S_CANARIVORE_GAS3,
	S_CANARIVORE_GAS4,
	S_CANARIVORE_GAS5,
	S_CANARIVORE_GASREPEAT,
	S_CANARIVORE_CLOSE1,
	S_CANARIVORE_CLOSE2,
	S_CANARIVOREGAS_1,
	S_CANARIVOREGAS_2,
	S_CANARIVOREGAS_3,
	S_CANARIVOREGAS_4,
	S_CANARIVOREGAS_5,
	S_CANARIVOREGAS_6,
	S_CANARIVOREGAS_7,
	S_CANARIVOREGAS_8,

	// Pyre Fly
	S_PYREFLY_FLY,
	S_PYREFLY_BURN,
	S_PYREFIRE1,
	S_PYREFIRE2,

	// Pterabyte
	S_PTERABYTESPAWNER,
	S_PTERABYTEWAYPOINT,
	S_PTERABYTE_FLY1,
	S_PTERABYTE_FLY2,
	S_PTERABYTE_FLY3,
	S_PTERABYTE_FLY4,
	S_PTERABYTE_SWOOPDOWN,
	S_PTERABYTE_SWOOPUP,

	// Dragonbomber
	S_DRAGONBOMBER,
	S_DRAGONWING1,
	S_DRAGONWING2,
	S_DRAGONWING3,
	S_DRAGONWING4,
	S_DRAGONTAIL_LOADED,
	S_DRAGONTAIL_EMPTY,
	S_DRAGONTAIL_EMPTYLOOP,
	S_DRAGONTAIL_RELOAD,
	S_DRAGONMINE,
	S_DRAGONMINE_LAND1,
	S_DRAGONMINE_LAND2,
	S_DRAGONMINE_SLOWFLASH1,
	S_DRAGONMINE_SLOWFLASH2,
	S_DRAGONMINE_SLOWLOOP,
	S_DRAGONMINE_FASTFLASH1,
	S_DRAGONMINE_FASTFLASH2,
	S_DRAGONMINE_FASTLOOP,

	// Boss Explosion
	S_BOSSEXPLODE,

	// S3&K Boss Explosion
	S_SONIC3KBOSSEXPLOSION1,
	S_SONIC3KBOSSEXPLOSION2,
	S_SONIC3KBOSSEXPLOSION3,
	S_SONIC3KBOSSEXPLOSION4,
	S_SONIC3KBOSSEXPLOSION5,
	S_SONIC3KBOSSEXPLOSION6,

	S_JETFUME1,

	// Boss 1
	S_EGGMOBILE_STND,
	S_EGGMOBILE_ROFL,
	S_EGGMOBILE_LATK1,
	S_EGGMOBILE_LATK2,
	S_EGGMOBILE_LATK3,
	S_EGGMOBILE_LATK4,
	S_EGGMOBILE_LATK5,
	S_EGGMOBILE_LATK6,
	S_EGGMOBILE_LATK7,
	S_EGGMOBILE_LATK8,
	S_EGGMOBILE_LATK9,
	S_EGGMOBILE_RATK1,
	S_EGGMOBILE_RATK2,
	S_EGGMOBILE_RATK3,
	S_EGGMOBILE_RATK4,
	S_EGGMOBILE_RATK5,
	S_EGGMOBILE_RATK6,
	S_EGGMOBILE_RATK7,
	S_EGGMOBILE_RATK8,
	S_EGGMOBILE_RATK9,
	S_EGGMOBILE_PANIC1,
	S_EGGMOBILE_PANIC2,
	S_EGGMOBILE_PANIC3,
	S_EGGMOBILE_PANIC4,
	S_EGGMOBILE_PANIC5,
	S_EGGMOBILE_PANIC6,
	S_EGGMOBILE_PANIC7,
	S_EGGMOBILE_PANIC8,
	S_EGGMOBILE_PANIC9,
	S_EGGMOBILE_PANIC10,
	S_EGGMOBILE_PANIC11,
	S_EGGMOBILE_PANIC12,
	S_EGGMOBILE_PANIC13,
	S_EGGMOBILE_PANIC14,
	S_EGGMOBILE_PANIC15,
	S_EGGMOBILE_PAIN,
	S_EGGMOBILE_PAIN2,
	S_EGGMOBILE_DIE1,
	S_EGGMOBILE_DIE2,
	S_EGGMOBILE_DIE3,
	S_EGGMOBILE_DIE4,
	S_EGGMOBILE_FLEE1,
	S_EGGMOBILE_FLEE2,
	S_EGGMOBILE_BALL,
	S_EGGMOBILE_TARGET,

	S_BOSSEGLZ1,
	S_BOSSEGLZ2,

	// Boss 2
	S_EGGMOBILE2_STND,
	S_EGGMOBILE2_POGO1,
	S_EGGMOBILE2_POGO2,
	S_EGGMOBILE2_POGO3,
	S_EGGMOBILE2_POGO4,
	S_EGGMOBILE2_POGO5,
	S_EGGMOBILE2_POGO6,
	S_EGGMOBILE2_POGO7,
	S_EGGMOBILE2_PAIN,
	S_EGGMOBILE2_PAIN2,
	S_EGGMOBILE2_DIE1,
	S_EGGMOBILE2_DIE2,
	S_EGGMOBILE2_DIE3,
	S_EGGMOBILE2_DIE4,
	S_EGGMOBILE2_FLEE1,
	S_EGGMOBILE2_FLEE2,

	S_BOSSTANK1,
	S_BOSSTANK2,
	S_BOSSSPIGOT,

	// Boss 2 Goop
	S_GOOP1,
	S_GOOP2,
	S_GOOP3,
	S_GOOPTRAIL,

	// Boss 3
	S_EGGMOBILE3_STND,
	S_EGGMOBILE3_SHOCK,
	S_EGGMOBILE3_ATK1,
	S_EGGMOBILE3_ATK2,
	S_EGGMOBILE3_ATK3A,
	S_EGGMOBILE3_ATK3B,
	S_EGGMOBILE3_ATK3C,
	S_EGGMOBILE3_ATK3D,
	S_EGGMOBILE3_ATK4,
	S_EGGMOBILE3_ATK5,
	S_EGGMOBILE3_ROFL,
	S_EGGMOBILE3_PAIN,
	S_EGGMOBILE3_PAIN2,
	S_EGGMOBILE3_DIE1,
	S_EGGMOBILE3_DIE2,
	S_EGGMOBILE3_DIE3,
	S_EGGMOBILE3_DIE4,
	S_EGGMOBILE3_FLEE1,
	S_EGGMOBILE3_FLEE2,

	// Boss 3 Pinch
	S_FAKEMOBILE_INIT,
	S_FAKEMOBILE,
	S_FAKEMOBILE_ATK1,
	S_FAKEMOBILE_ATK2,
	S_FAKEMOBILE_ATK3A,
	S_FAKEMOBILE_ATK3B,
	S_FAKEMOBILE_ATK3C,
	S_FAKEMOBILE_ATK3D,
	S_FAKEMOBILE_DIE1,
	S_FAKEMOBILE_DIE2,

	S_BOSSSEBH1,
	S_BOSSSEBH2,

	// Boss 3 Shockwave
	S_SHOCKWAVE1,
	S_SHOCKWAVE2,

	// Boss 4
	S_EGGMOBILE4_STND,
	S_EGGMOBILE4_LATK1,
	S_EGGMOBILE4_LATK2,
	S_EGGMOBILE4_LATK3,
	S_EGGMOBILE4_LATK4,
	S_EGGMOBILE4_LATK5,
	S_EGGMOBILE4_LATK6,
	S_EGGMOBILE4_RATK1,
	S_EGGMOBILE4_RATK2,
	S_EGGMOBILE4_RATK3,
	S_EGGMOBILE4_RATK4,
	S_EGGMOBILE4_RATK5,
	S_EGGMOBILE4_RATK6,
	S_EGGMOBILE4_RAISE1,
	S_EGGMOBILE4_RAISE2,
	S_EGGMOBILE4_PAIN1,
	S_EGGMOBILE4_PAIN2,
	S_EGGMOBILE4_DIE1,
	S_EGGMOBILE4_DIE2,
	S_EGGMOBILE4_DIE3,
	S_EGGMOBILE4_DIE4,
	S_EGGMOBILE4_FLEE1,
	S_EGGMOBILE4_FLEE2,
	S_EGGMOBILE4_MACE,
	S_EGGMOBILE4_MACE_DIE1,
	S_EGGMOBILE4_MACE_DIE2,
	S_EGGMOBILE4_MACE_DIE3,

	// Boss 4 jet flame
	S_JETFLAME,

	// Boss 4 Spectator Eggrobo
	S_EGGROBO1_STND,
	S_EGGROBO1_BSLAP1,
	S_EGGROBO1_BSLAP2,
	S_EGGROBO1_PISSED,

	// Boss 4 Spectator Eggrobo jet flame
	S_EGGROBOJET,

	// Boss 5
	S_FANG_SETUP,
	S_FANG_INTRO0,
	S_FANG_INTRO1,
	S_FANG_INTRO2,
	S_FANG_INTRO3,
	S_FANG_INTRO4,
	S_FANG_INTRO5,
	S_FANG_INTRO6,
	S_FANG_INTRO7,
	S_FANG_INTRO8,
	S_FANG_INTRO9,
	S_FANG_INTRO10,
	S_FANG_INTRO11,
	S_FANG_INTRO12,
	S_FANG_CLONE1,
	S_FANG_CLONE2,
	S_FANG_CLONE3,
	S_FANG_CLONE4,
	S_FANG_IDLE0,
	S_FANG_IDLE1,
	S_FANG_IDLE2,
	S_FANG_IDLE3,
	S_FANG_IDLE4,
	S_FANG_IDLE5,
	S_FANG_IDLE6,
	S_FANG_IDLE7,
	S_FANG_IDLE8,
	S_FANG_PAIN1,
	S_FANG_PAIN2,
	S_FANG_PATHINGSTART1,
	S_FANG_PATHINGSTART2,
	S_FANG_PATHING,
	S_FANG_BOUNCE1,
	S_FANG_BOUNCE2,
	S_FANG_BOUNCE3,
	S_FANG_BOUNCE4,
	S_FANG_FALL1,
	S_FANG_FALL2,
	S_FANG_CHECKPATH1,
	S_FANG_CHECKPATH2,
	S_FANG_PATHINGCONT1,
	S_FANG_PATHINGCONT2,
	S_FANG_PATHINGCONT3,
	S_FANG_SKID1,
	S_FANG_SKID2,
	S_FANG_SKID3,
	S_FANG_CHOOSEATTACK,
	S_FANG_FIRESTART1,
	S_FANG_FIRESTART2,
	S_FANG_FIRE1,
	S_FANG_FIRE2,
	S_FANG_FIRE3,
	S_FANG_FIRE4,
	S_FANG_FIREREPEAT,
	S_FANG_LOBSHOT0,
	S_FANG_LOBSHOT1,
	S_FANG_LOBSHOT2,
	S_FANG_WAIT1,
	S_FANG_WAIT2,
	S_FANG_WALLHIT,
	S_FANG_PINCHPATHINGSTART1,
	S_FANG_PINCHPATHINGSTART2,
	S_FANG_PINCHPATHING,
	S_FANG_PINCHBOUNCE0,
	S_FANG_PINCHBOUNCE1,
	S_FANG_PINCHBOUNCE2,
	S_FANG_PINCHBOUNCE3,
	S_FANG_PINCHBOUNCE4,
	S_FANG_PINCHFALL0,
	S_FANG_PINCHFALL1,
	S_FANG_PINCHFALL2,
	S_FANG_PINCHSKID1,
	S_FANG_PINCHSKID2,
	S_FANG_PINCHLOBSHOT0,
	S_FANG_PINCHLOBSHOT1,
	S_FANG_PINCHLOBSHOT2,
	S_FANG_PINCHLOBSHOT3,
	S_FANG_PINCHLOBSHOT4,
	S_FANG_DIE1,
	S_FANG_DIE2,
	S_FANG_DIE3,
	S_FANG_DIE4,
	S_FANG_DIE5,
	S_FANG_DIE6,
	S_FANG_DIE7,
	S_FANG_DIE8,
	S_FANG_FLEEPATHING1,
	S_FANG_FLEEPATHING2,
	S_FANG_FLEEBOUNCE1,
	S_FANG_FLEEBOUNCE2,
	S_FANG_KO,

	S_BROKENROBOTRANDOM,
	S_BROKENROBOTA,
	S_BROKENROBOTB,
	S_BROKENROBOTC,
	S_BROKENROBOTD,
	S_BROKENROBOTE,
	S_BROKENROBOTF,

	S_ALART1,
	S_ALART2,

	S_VWREF,
	S_VWREB,

	S_PROJECTORLIGHT1,
	S_PROJECTORLIGHT2,
	S_PROJECTORLIGHT3,
	S_PROJECTORLIGHT4,
	S_PROJECTORLIGHT5,

	S_FBOMB1,
	S_FBOMB2,
	S_FBOMB_EXPL1,
	S_FBOMB_EXPL2,
	S_FBOMB_EXPL3,
	S_FBOMB_EXPL4,
	S_FBOMB_EXPL5,
	S_FBOMB_EXPL6,
	S_TNTDUST_1,
	S_TNTDUST_2,
	S_TNTDUST_3,
	S_TNTDUST_4,
	S_TNTDUST_5,
	S_TNTDUST_6,
	S_TNTDUST_7,
	S_TNTDUST_8,
	S_FSGNA,
	S_FSGNB,
	S_FSGNC,
	S_FSGND,

	// Metal Sonic (Race)
	S_METALSONIC_RACE,
	// Metal Sonic (Battle)
	S_METALSONIC_FLOAT,
	S_METALSONIC_VECTOR,
	S_METALSONIC_STUN,
	S_METALSONIC_RAISE,
	S_METALSONIC_GATHER,
	S_METALSONIC_DASH,
	S_METALSONIC_BOUNCE,
	S_METALSONIC_BADBOUNCE,
	S_METALSONIC_SHOOT,
	S_METALSONIC_PAIN,
	S_METALSONIC_DEATH1,
	S_METALSONIC_DEATH2,
	S_METALSONIC_DEATH3,
	S_METALSONIC_DEATH4,
	S_METALSONIC_FLEE1,
	S_METALSONIC_FLEE2,

	S_MSSHIELD_F1,
	S_MSSHIELD_F2,

	// Ring
	S_RING,
	S_FASTRING1,
	S_FASTRING2,
	S_FASTRING3,
	S_FASTRING4,
	S_FASTRING5,
	S_FASTRING6,
	S_FASTRING7,
	S_FASTRING8,
	S_FASTRING9,
	S_FASTRING10,
	S_FASTRING11,
	S_FASTRING12,

	// Blue Sphere for special stages
	S_BLUESPHERE,
	S_BLUESPHERE_SPAWN,

	S_BLUESPHERE_BOUNCE1,
	S_BLUESPHERE_BOUNCE2,

	S_BLUESPHERE_BOUNCE3,
	S_BLUESPHERE_BOUNCE4,

	S_BLUESPHERE_BOUNCE5,
	S_BLUESPHERE_BOUNCE6,
	S_BLUESPHERE_BOUNCE7,
	S_BLUESPHERE_BOUNCE8,

	S_BLUESPHERE_BOUNCE9,
	S_BLUESPHERE_BOUNCE10,
	S_BLUESPHERE_BOUNCE11,
	S_BLUESPHERE_BOUNCE12,

	S_BLUESPHERE_BOUNCE13,
	S_BLUESPHERE_BOUNCE14,
	S_BLUESPHERE_BOUNCE15,
	S_BLUESPHERE_BOUNCE16,
	S_BLUESPHERE_BOUNCE17,
	S_BLUESPHERE_BOUNCE18,
	S_BLUESPHERE_BOUNCE19,
	S_BLUESPHERE_BOUNCE20,

	S_BLUESPHERE_BOUNCE21,
	S_BLUESPHERE_BOUNCE22,
	S_BLUESPHERE_BOUNCE23,
	S_BLUESPHERE_BOUNCE24,
	S_BLUESPHERE_BOUNCE25,
	S_BLUESPHERE_BOUNCE26,
	S_BLUESPHERE_BOUNCE27,
	S_BLUESPHERE_BOUNCE28,

	// Bomb Sphere
	S_BOMBSPHERE1,
	S_BOMBSPHERE2,
	S_BOMBSPHERE3,
	S_BOMBSPHERE4,

	// NiGHTS Chip
	S_NIGHTSCHIP,
	S_NIGHTSCHIPBONUS,

	// NiGHTS Star
	S_NIGHTSSTAR,
	S_NIGHTSSTARXMAS,

	// Gravity Wells for special stages
	S_GRAVWELLGREEN,
	S_GRAVWELLRED,

	// Individual Team Rings
	S_TEAMRING,

	// Special Stage Token
	S_TOKEN,

	// CTF Flags
	S_REDFLAG,
	S_BLUEFLAG,

	// Emblem
	S_EMBLEM1,
	S_EMBLEM2,
	S_EMBLEM3,
	S_EMBLEM4,
	S_EMBLEM5,
	S_EMBLEM6,
	S_EMBLEM7,
	S_EMBLEM8,
	S_EMBLEM9,
	S_EMBLEM10,
	S_EMBLEM11,
	S_EMBLEM12,
	S_EMBLEM13,
	S_EMBLEM14,
	S_EMBLEM15,
	S_EMBLEM16,
	S_EMBLEM17,
	S_EMBLEM18,
	S_EMBLEM19,
	S_EMBLEM20,
	S_EMBLEM21,
	S_EMBLEM22,
	S_EMBLEM23,
	S_EMBLEM24,
	S_EMBLEM25,
	S_EMBLEM26,

	// Spray Can
	S_SPRAYCAN,

	// Ancient Shrine
	S_ANCIENTSHRINE,

	S_MORB1,
	S_MORB2,
	S_MORB3,
	S_MORB4,
	S_MORB5,
	S_MORB6,
	S_MORB7,
	S_MORB8,
	S_MORB9,
	S_MORB10,
	S_MORB11,
	S_MORB12,
	S_MORB13,
	S_MORB14,
	S_MORB15,

	// Chaos Emeralds
	S_CHAOSEMERALD1,
	S_CHAOSEMERALD2,
	S_CHAOSEMERALD_UNDER,

	// Super Emeralds
	S_SUPEREMERALD1,
	S_SUPEREMERALD2,
	S_SUPEREMERALD_UNDER,

	S_EMERALDSPARK1,
	S_EMERALDSPARK2,
	S_EMERALDSPARK3,
	S_EMERALDSPARK4,
	S_EMERALDSPARK5,
	S_EMERALDSPARK6,
	S_EMERALDSPARK7,

	S_EMERALDFLARE1,

	// Prison Egg Drops
	S_PRISONEGGDROP_CD,

	// Bubble Source
	S_BUBBLES1,
	S_BUBBLES2,
	S_BUBBLES3,
	S_BUBBLES4,

	// Level End Sign
	S_SIGN_POLE,
	S_SIGN_BACK,
	S_SIGN_SIDE,
	S_SIGN_FACE,
	S_SIGN_ERROR,

	// Spike Ball
	S_SPIKEBALL1,
	S_SPIKEBALL2,
	S_SPIKEBALL3,
	S_SPIKEBALL4,
	S_SPIKEBALL5,
	S_SPIKEBALL6,
	S_SPIKEBALL7,
	S_SPIKEBALL8,

	// Elemental Shield's Spawn
	S_SPINFIRE1,
	S_SPINFIRE2,
	S_SPINFIRE3,
	S_SPINFIRE4,
	S_SPINFIRE5,
	S_SPINFIRE6,

	S_TEAM_SPINFIRE1,
	S_TEAM_SPINFIRE2,
	S_TEAM_SPINFIRE3,
	S_TEAM_SPINFIRE4,
	S_TEAM_SPINFIRE5,
	S_TEAM_SPINFIRE6,

	// Spikes
	S_SPIKE1,
	S_SPIKE2,
	S_SPIKE3,
	S_SPIKE4,
	S_SPIKE5,
	S_SPIKE6,
	S_SPIKED1,
	S_SPIKED2,

	// Wall spikes
	S_WALLSPIKE1,
	S_WALLSPIKE2,
	S_WALLSPIKE3,
	S_WALLSPIKE4,
	S_WALLSPIKE5,
	S_WALLSPIKE6,
	S_WALLSPIKEBASE,
	S_WALLSPIKED1,
	S_WALLSPIKED2,

	// Big floating mine
	S_BIGMINE_IDLE,
	S_BIGMINE_ALERT1,
	S_BIGMINE_ALERT2,
	S_BIGMINE_ALERT3,
	S_BIGMINE_SET1,
	S_BIGMINE_SET2,
	S_BIGMINE_SET3,
	S_BIGMINE_BLAST1,
	S_BIGMINE_BLAST2,
	S_BIGMINE_BLAST3,
	S_BIGMINE_BLAST4,
	S_BIGMINE_BLAST5,

	// Cannon Launcher
	S_CANNONLAUNCHER1,
	S_CANNONLAUNCHER2,
	S_CANNONLAUNCHER3,

	// Monitor Miscellany
	S_BOXSPARKLE1,
	S_BOXSPARKLE2,
	S_BOXSPARKLE3,
	S_BOXSPARKLE4,

	S_BOX_FLICKER,
	S_BOX_POP1,
	S_BOX_POP2,

	S_GOLDBOX_FLICKER,
	S_GOLDBOX_OFF1,
	S_GOLDBOX_OFF2,
	S_GOLDBOX_OFF3,
	S_GOLDBOX_OFF4,
	S_GOLDBOX_OFF5,
	S_GOLDBOX_OFF6,
	S_GOLDBOX_OFF7,

	// Monitor States (one per box)
	S_MYSTERY_BOX,
	S_RING_BOX,
	S_PITY_BOX,
	S_ATTRACT_BOX,
	S_FORCE_BOX,
	S_ARMAGEDDON_BOX,
	S_WHIRLWIND_BOX,
	S_ELEMENTAL_BOX,
	S_SNEAKERS_BOX,
	S_INVULN_BOX,
	S_1UP_BOX,
	S_EGGMAN_BOX,
	S_MIXUP_BOX,
	S_GRAVITY_BOX,
	S_RECYCLER_BOX,
	S_SCORE1K_BOX,
	S_SCORE10K_BOX,
	S_FLAMEAURA_BOX,
	S_BUBBLEWRAP_BOX,
	S_THUNDERCOIN_BOX,

	// Gold Repeat Monitor States (one per box)
	S_PITY_GOLDBOX,
	S_ATTRACT_GOLDBOX,
	S_FORCE_GOLDBOX,
	S_ARMAGEDDON_GOLDBOX,
	S_WHIRLWIND_GOLDBOX,
	S_ELEMENTAL_GOLDBOX,
	S_SNEAKERS_GOLDBOX,
	S_INVULN_GOLDBOX,
	S_EGGMAN_GOLDBOX,
	S_GRAVITY_GOLDBOX,
	S_FLAMEAURA_GOLDBOX,
	S_BUBBLEWRAP_GOLDBOX,
	S_THUNDERCOIN_GOLDBOX,

	// Team Ring Boxes (these are special)
	S_RING_REDBOX1,
	S_RING_REDBOX2,
	S_REDBOX_POP1,
	S_REDBOX_POP2,

	S_RING_BLUEBOX1,
	S_RING_BLUEBOX2,
	S_BLUEBOX_POP1,
	S_BLUEBOX_POP2,

	// Box Icons -- 2 states each, animation and action
	S_RING_ICON1,
	S_RING_ICON2,

	S_PITY_ICON1,
	S_PITY_ICON2,

	S_ATTRACT_ICON1,
	S_ATTRACT_ICON2,

	S_FORCE_ICON1,
	S_FORCE_ICON2,

	S_ARMAGEDDON_ICON1,
	S_ARMAGEDDON_ICON2,

	S_WHIRLWIND_ICON1,
	S_WHIRLWIND_ICON2,

	S_ELEMENTAL_ICON1,
	S_ELEMENTAL_ICON2,

	S_SNEAKERS_ICON1,
	S_SNEAKERS_ICON2,

	S_INVULN_ICON1,
	S_INVULN_ICON2,

	S_1UP_ICON1,
	S_1UP_ICON2,

	S_EGGMAN_ICON1,
	S_EGGMAN_ICON2,

	S_MIXUP_ICON1,
	S_MIXUP_ICON2,

	S_GRAVITY_ICON1,
	S_GRAVITY_ICON2,

	S_RECYCLER_ICON1,
	S_RECYCLER_ICON2,

	S_SCORE1K_ICON1,
	S_SCORE1K_ICON2,

	S_SCORE10K_ICON1,
	S_SCORE10K_ICON2,

	S_FLAMEAURA_ICON1,
	S_FLAMEAURA_ICON2,

	S_BUBBLEWRAP_ICON1,
	S_BUBBLEWRAP_ICON2,

	S_THUNDERCOIN_ICON1,
	S_THUNDERCOIN_ICON2,

	// ---

	S_ROCKET,

	S_LASER,
	S_LASER2,
	S_LASERFLASH,

	S_LASERFLAME1,
	S_LASERFLAME2,
	S_LASERFLAME3,
	S_LASERFLAME4,
	S_LASERFLAME5,

	S_TORPEDO,

	S_ENERGYBALL1,
	S_ENERGYBALL2,

	// Skim Mine, also used by Jetty-Syn bomber
	S_MINE1,
	S_MINE_BOOM1,
	S_MINE_BOOM2,
	S_MINE_BOOM3,
	S_MINE_BOOM4,

	// Jetty-Syn Bullet
	S_JETBULLET1,
	S_JETBULLET2,

	S_TURRETLASER,
	S_TURRETLASEREXPLODE1,
	S_TURRETLASEREXPLODE2,

	// Cannonball
	S_CANNONBALL1,

	// Arrow
	S_ARROW,
	S_ARROWBONK,

	// Glaregoyle Demon fire
	S_DEMONFIRE,

	// The letter
	S_LETTER,

	// GFZ flowers
	S_GFZFLOWERA,
	S_GFZFLOWERB,
	S_GFZFLOWERC,

	S_BLUEBERRYBUSH,
	S_BERRYBUSH,
	S_BUSH,

	// Trees (both GFZ and misc)
	S_GFZTREE,
	S_GFZBERRYTREE,
	S_GFZCHERRYTREE,
	S_CHECKERTREE,
	S_CHECKERSUNSETTREE,
	S_FHZTREE, // Frozen Hillside
	S_FHZPINKTREE,
	S_POLYGONTREE,
	S_BUSHTREE,
	S_BUSHREDTREE,
	S_SPRINGTREE,

	// THZ flowers
	S_THZFLOWERA, // THZ1 Steam flower
	S_THZFLOWERB, // THZ1 Spin flower (red)
	S_THZFLOWERC, // THZ1 Spin flower (yellow)

	// THZ Steam Whistle tree/bush
	S_THZTREE,
	S_THZTREEBRANCH1,
	S_THZTREEBRANCH2,
	S_THZTREEBRANCH3,
	S_THZTREEBRANCH4,
	S_THZTREEBRANCH5,
	S_THZTREEBRANCH6,
	S_THZTREEBRANCH7,
	S_THZTREEBRANCH8,
	S_THZTREEBRANCH9,
	S_THZTREEBRANCH10,
	S_THZTREEBRANCH11,
	S_THZTREEBRANCH12,
	S_THZTREEBRANCH13,

	// THZ Alarm
	S_ALARM1,

	// Deep Sea Gargoyle
	S_GARGOYLE,
	S_BIGGARGOYLE,

	// DSZ Seaweed
	S_SEAWEED1,
	S_SEAWEED2,
	S_SEAWEED3,
	S_SEAWEED4,
	S_SEAWEED5,
	S_SEAWEED6,

	// Dripping Water
	S_DRIPA1,
	S_DRIPA2,
	S_DRIPA3,
	S_DRIPA4,
	S_DRIPB1,
	S_DRIPC1,
	S_DRIPC2,

	// Coral
	S_CORAL1,
	S_CORAL2,
	S_CORAL3,
	S_CORAL4,
	S_CORAL5,

	// Blue Crystal
	S_BLUECRYSTAL1,

	// Kelp,
	S_KELP,

	// Animated algae
	S_ANIMALGAETOP1,
	S_ANIMALGAETOP2,
	S_ANIMALGAESEG,

	// DSZ Stalagmites
	S_DSZSTALAGMITE,
	S_DSZ2STALAGMITE,

	// DSZ Light beam
	S_LIGHTBEAM1,
	S_LIGHTBEAM2,
	S_LIGHTBEAM3,
	S_LIGHTBEAM4,
	S_LIGHTBEAM5,
	S_LIGHTBEAM6,
	S_LIGHTBEAM7,
	S_LIGHTBEAM8,
	S_LIGHTBEAM9,
	S_LIGHTBEAM10,
	S_LIGHTBEAM11,
	S_LIGHTBEAM12,

	// CEZ Chain
	S_CEZCHAIN,

	// Flame
	S_FLAME,
	S_FLAMEPARTICLE,
	S_FLAMEREST,

	// Eggman Statue
	S_EGGSTATUE1,

	// CEZ hidden sling
	S_SLING1,
	S_SLING2,

	// CEZ maces and chains
	S_SMALLMACECHAIN,
	S_BIGMACECHAIN,
	S_SMALLMACE,
	S_BIGMACE,
	S_SMALLGRABCHAIN,
	S_BIGGRABCHAIN,

	// Yellow spring on a ball
	S_YELLOWSPRINGBALL,
	S_YELLOWSPRINGBALL2,
	S_YELLOWSPRINGBALL3,
	S_YELLOWSPRINGBALL4,
	S_YELLOWSPRINGBALL5,

	// Red spring on a ball
	S_REDSPRINGBALL,
	S_REDSPRINGBALL2,
	S_REDSPRINGBALL3,
	S_REDSPRINGBALL4,
	S_REDSPRINGBALL5,

	// Small Firebar
	S_SMALLFIREBAR1,
	S_SMALLFIREBAR2,
	S_SMALLFIREBAR3,
	S_SMALLFIREBAR4,
	S_SMALLFIREBAR5,
	S_SMALLFIREBAR6,
	S_SMALLFIREBAR7,
	S_SMALLFIREBAR8,
	S_SMALLFIREBAR9,
	S_SMALLFIREBAR10,
	S_SMALLFIREBAR11,
	S_SMALLFIREBAR12,
	S_SMALLFIREBAR13,
	S_SMALLFIREBAR14,
	S_SMALLFIREBAR15,
	S_SMALLFIREBAR16,

	// Big Firebar
	S_BIGFIREBAR1,
	S_BIGFIREBAR2,
	S_BIGFIREBAR3,
	S_BIGFIREBAR4,
	S_BIGFIREBAR5,
	S_BIGFIREBAR6,
	S_BIGFIREBAR7,
	S_BIGFIREBAR8,
	S_BIGFIREBAR9,
	S_BIGFIREBAR10,
	S_BIGFIREBAR11,
	S_BIGFIREBAR12,
	S_BIGFIREBAR13,
	S_BIGFIREBAR14,
	S_BIGFIREBAR15,
	S_BIGFIREBAR16,

	S_CEZFLOWER,
	S_CEZPOLE,
	S_CEZBANNER1,
	S_CEZBANNER2,
	S_PINETREE,
	S_CEZBUSH1,
	S_CEZBUSH2,
	S_CANDLE,
	S_CANDLEPRICKET,
	S_FLAMEHOLDER,
	S_FIRETORCH,
	S_WAVINGFLAG,
	S_WAVINGFLAGSEG1,
	S_WAVINGFLAGSEG2,
	S_CRAWLASTATUE,
	S_FACESTABBERSTATUE,
	S_SUSPICIOUSFACESTABBERSTATUE_WAIT,
	S_SUSPICIOUSFACESTABBERSTATUE_BURST1,
	S_SUSPICIOUSFACESTABBERSTATUE_BURST2,
	S_BRAMBLES,

	// Big Tumbleweed
	S_BIGTUMBLEWEED,
	S_BIGTUMBLEWEED_ROLL1,
	S_BIGTUMBLEWEED_ROLL2,
	S_BIGTUMBLEWEED_ROLL3,
	S_BIGTUMBLEWEED_ROLL4,
	S_BIGTUMBLEWEED_ROLL5,
	S_BIGTUMBLEWEED_ROLL6,
	S_BIGTUMBLEWEED_ROLL7,
	S_BIGTUMBLEWEED_ROLL8,

	// Little Tumbleweed
	S_LITTLETUMBLEWEED,
	S_LITTLETUMBLEWEED_ROLL1,
	S_LITTLETUMBLEWEED_ROLL2,
	S_LITTLETUMBLEWEED_ROLL3,
	S_LITTLETUMBLEWEED_ROLL4,
	S_LITTLETUMBLEWEED_ROLL5,
	S_LITTLETUMBLEWEED_ROLL6,
	S_LITTLETUMBLEWEED_ROLL7,
	S_LITTLETUMBLEWEED_ROLL8,

	// Cacti
	S_CACTI1,
	S_CACTI2,
	S_CACTI3,
	S_CACTI4,
	S_CACTI5,
	S_CACTI6,
	S_CACTI7,
	S_CACTI8,
	S_CACTI9,
	S_CACTI10,
	S_CACTI11,
	S_CACTITINYSEG,
	S_CACTISMALLSEG,

	// Warning signs
	S_ARIDSIGN_CAUTION,
	S_ARIDSIGN_CACTI,
	S_ARIDSIGN_SHARPTURN,

	// Oil lamp
	S_OILLAMP,
	S_OILLAMPFLARE,

	// TNT barrel
	S_TNTBARREL_STND1,
	S_TNTBARREL_EXPL1,
	S_TNTBARREL_EXPL2,
	S_TNTBARREL_EXPL3,
	S_TNTBARREL_EXPL4,
	S_TNTBARREL_EXPL5,
	S_TNTBARREL_EXPL6,
	S_TNTBARREL_EXPL7,
	S_TNTBARREL_FLYING,

	// TNT proximity shell
	S_PROXIMITY_TNT,
	S_PROXIMITY_TNT_TRIGGER1,
	S_PROXIMITY_TNT_TRIGGER2,
	S_PROXIMITY_TNT_TRIGGER3,
	S_PROXIMITY_TNT_TRIGGER4,
	S_PROXIMITY_TNT_TRIGGER5,
	S_PROXIMITY_TNT_TRIGGER6,
	S_PROXIMITY_TNT_TRIGGER7,
	S_PROXIMITY_TNT_TRIGGER8,
	S_PROXIMITY_TNT_TRIGGER9,
	S_PROXIMITY_TNT_TRIGGER10,
	S_PROXIMITY_TNT_TRIGGER11,
	S_PROXIMITY_TNT_TRIGGER12,
	S_PROXIMITY_TNT_TRIGGER13,
	S_PROXIMITY_TNT_TRIGGER14,
	S_PROXIMITY_TNT_TRIGGER15,
	S_PROXIMITY_TNT_TRIGGER16,
	S_PROXIMITY_TNT_TRIGGER17,
	S_PROXIMITY_TNT_TRIGGER18,
	S_PROXIMITY_TNT_TRIGGER19,
	S_PROXIMITY_TNT_TRIGGER20,
	S_PROXIMITY_TNT_TRIGGER21,
	S_PROXIMITY_TNT_TRIGGER22,
	S_PROXIMITY_TNT_TRIGGER23,

	// Dust devil
	S_DUSTDEVIL,
	S_DUSTLAYER1,
	S_DUSTLAYER2,
	S_DUSTLAYER3,
	S_DUSTLAYER4,
	S_DUSTLAYER5,
	S_ARIDDUST1,
	S_ARIDDUST2,
	S_ARIDDUST3,

	// Minecart
	S_MINECART_IDLE,
	S_MINECART_DTH1,
	S_MINECARTEND,
	S_MINECARTSEG_FRONT,
	S_MINECARTSEG_BACK,
	S_MINECARTSEG_LEFT,
	S_MINECARTSEG_RIGHT,
	S_MINECARTSIDEMARK1,
	S_MINECARTSIDEMARK2,
	S_MINECARTSPARK,

	// Saloon door
	S_SALOONDOOR,
	S_SALOONDOORCENTER,

	// Train cameo
	S_TRAINCAMEOSPAWNER_1,
	S_TRAINCAMEOSPAWNER_2,
	S_TRAINCAMEOSPAWNER_3,
	S_TRAINCAMEOSPAWNER_4,
	S_TRAINCAMEOSPAWNER_5,
	S_TRAINPUFFMAKER,

	// Train
	S_TRAINDUST,
	S_TRAINSTEAM,

	// Flame jet
	S_FLAMEJETSTND,
	S_FLAMEJETSTART,
	S_FLAMEJETSTOP,
	S_FLAMEJETFLAME1,
	S_FLAMEJETFLAME2,
	S_FLAMEJETFLAME3,
	S_FLAMEJETFLAME4,
	S_FLAMEJETFLAME5,
	S_FLAMEJETFLAME6,
	S_FLAMEJETFLAME7,
	S_FLAMEJETFLAME8,
	S_FLAMEJETFLAME9,

	// Spinning flame jets
	S_FJSPINAXISA1, // Counter-clockwise
	S_FJSPINAXISA2,
	S_FJSPINAXISB1, // Clockwise
	S_FJSPINAXISB2,

	// Blade's flame
	S_FLAMEJETFLAMEB1,
	S_FLAMEJETFLAMEB2,
	S_FLAMEJETFLAMEB3,

	// Lavafall
	S_LAVAFALL_DORMANT,
	S_LAVAFALL_TELL,
	S_LAVAFALL_SHOOT,
	S_LAVAFALL_LAVA1,
	S_LAVAFALL_LAVA2,
	S_LAVAFALL_LAVA3,
	S_LAVAFALLROCK,

	// Rollout Rock
	S_ROLLOUTSPAWN,
	S_ROLLOUTROCK,

	// RVZ scenery
	S_BIGFERNLEAF,
	S_BIGFERN1,
	S_BIGFERN2,
	S_JUNGLEPALM,
	S_TORCHFLOWER,
	S_WALLVINE_LONG,
	S_WALLVINE_SHORT,

	// Glaregoyles
	S_GLAREGOYLE,
	S_GLAREGOYLE_CHARGE,
	S_GLAREGOYLE_BLINK,
	S_GLAREGOYLE_HOLD,
	S_GLAREGOYLE_FIRE,
	S_GLAREGOYLE_LOOP,
	S_GLAREGOYLE_COOLDOWN,
	S_GLAREGOYLEUP,
	S_GLAREGOYLEUP_CHARGE,
	S_GLAREGOYLEUP_BLINK,
	S_GLAREGOYLEUP_HOLD,
	S_GLAREGOYLEUP_FIRE,
	S_GLAREGOYLEUP_LOOP,
	S_GLAREGOYLEUP_COOLDOWN,
	S_GLAREGOYLEDOWN,
	S_GLAREGOYLEDOWN_CHARGE,
	S_GLAREGOYLEDOWN_BLINK,
	S_GLAREGOYLEDOWN_HOLD,
	S_GLAREGOYLEDOWN_FIRE,
	S_GLAREGOYLEDOWN_LOOP,
	S_GLAREGOYLEDOWN_COOLDOWN,
	S_GLAREGOYLELONG,
	S_GLAREGOYLELONG_CHARGE,
	S_GLAREGOYLELONG_BLINK,
	S_GLAREGOYLELONG_HOLD,
	S_GLAREGOYLELONG_FIRE,
	S_GLAREGOYLELONG_LOOP,
	S_GLAREGOYLELONG_COOLDOWN,

	// ATZ's Red Crystal/Target
	S_TARGET_IDLE,
	S_TARGET_HIT1,
	S_TARGET_HIT2,
	S_TARGET_RESPAWN,
	S_TARGET_ALLDONE,

	// ATZ's green flame
	S_GREENFLAME,

	// ATZ Blue Gargoyle
	S_BLUEGARGOYLE,

	// Stalagmites
	S_STG0,
	S_STG1,
	S_STG2,
	S_STG3,
	S_STG4,
	S_STG5,
	S_STG6,
	S_STG7,
	S_STG8,
	S_STG9,

	// Xmas-specific stuff
	S_XMASPOLE,
	S_CANDYCANE,
	S_SNOWMAN,    // normal
	S_SNOWMANHAT, // with hat + scarf
	S_LAMPPOST1,  // normal
	S_LAMPPOST2,  // with snow
	S_HANGSTAR,
	S_MISTLETOE,
	// Xmas GFZ bushes
	S_XMASBLUEBERRYBUSH,
	S_XMASBERRYBUSH,
	S_XMASBUSH,
	// FHZ
	S_FHZICE1,
	S_FHZICE2,
	S_ROSY_IDLE1,
	S_ROSY_IDLE2,
	S_ROSY_IDLE3,
	S_ROSY_IDLE4,
	S_ROSY_JUMP,
	S_ROSY_WALK,
	S_ROSY_HUG,
	S_ROSY_PAIN,
	S_ROSY_STND,
	S_ROSY_UNHAPPY,

	// Halloween Scenery
	// Pumpkins
	S_JACKO1,
	S_JACKO1OVERLAY_1,
	S_JACKO1OVERLAY_2,
	S_JACKO1OVERLAY_3,
	S_JACKO1OVERLAY_4,
	S_JACKO2,
	S_JACKO2OVERLAY_1,
	S_JACKO2OVERLAY_2,
	S_JACKO2OVERLAY_3,
	S_JACKO2OVERLAY_4,
	S_JACKO3,
	S_JACKO3OVERLAY_1,
	S_JACKO3OVERLAY_2,
	S_JACKO3OVERLAY_3,
	S_JACKO3OVERLAY_4,
	// Dr Seuss Trees
	S_HHZTREE_TOP,
	S_HHZTREE_TRUNK,
	S_HHZTREE_LEAF,
	// Mushroom
	S_HHZSHROOM_1,
	S_HHZSHROOM_2,
	S_HHZSHROOM_3,
	S_HHZSHROOM_4,
	S_HHZSHROOM_5,
	S_HHZSHROOM_6,
	S_HHZSHROOM_7,
	S_HHZSHROOM_8,
	S_HHZSHROOM_9,
	S_HHZSHROOM_10,
	S_HHZSHROOM_11,
	S_HHZSHROOM_12,
	S_HHZSHROOM_13,
	S_HHZSHROOM_14,
	S_HHZSHROOM_15,
	S_HHZSHROOM_16,
	// Misc
	S_HHZGRASS,
	S_HHZTENT1,
	S_HHZTENT2,
	S_HHZSTALAGMITE_TALL,
	S_HHZSTALAGMITE_SHORT,

	// Botanic Serenity's loads of scenery states
	S_BSZTALLFLOWER_RED,
	S_BSZTALLFLOWER_PURPLE,
	S_BSZTALLFLOWER_BLUE,
	S_BSZTALLFLOWER_CYAN,
	S_BSZTALLFLOWER_YELLOW,
	S_BSZTALLFLOWER_ORANGE,
	S_BSZFLOWER_RED,
	S_BSZFLOWER_PURPLE,
	S_BSZFLOWER_BLUE,
	S_BSZFLOWER_CYAN,
	S_BSZFLOWER_YELLOW,
	S_BSZFLOWER_ORANGE,
	S_BSZSHORTFLOWER_RED,
	S_BSZSHORTFLOWER_PURPLE,
	S_BSZSHORTFLOWER_BLUE,
	S_BSZSHORTFLOWER_CYAN,
	S_BSZSHORTFLOWER_YELLOW,
	S_BSZSHORTFLOWER_ORANGE,
	S_BSZTULIP_RED,
	S_BSZTULIP_PURPLE,
	S_BSZTULIP_BLUE,
	S_BSZTULIP_CYAN,
	S_BSZTULIP_YELLOW,
	S_BSZTULIP_ORANGE,
	S_BSZCLUSTER_RED,
	S_BSZCLUSTER_PURPLE,
	S_BSZCLUSTER_BLUE,
	S_BSZCLUSTER_CYAN,
	S_BSZCLUSTER_YELLOW,
	S_BSZCLUSTER_ORANGE,
	S_BSZBUSH_RED,
	S_BSZBUSH_PURPLE,
	S_BSZBUSH_BLUE,
	S_BSZBUSH_CYAN,
	S_BSZBUSH_YELLOW,
	S_BSZBUSH_ORANGE,
	S_BSZVINE_RED,
	S_BSZVINE_PURPLE,
	S_BSZVINE_BLUE,
	S_BSZVINE_CYAN,
	S_BSZVINE_YELLOW,
	S_BSZVINE_ORANGE,
	S_BSZSHRUB,
	S_BSZCLOVER,
	S_BIG_PALMTREE_TRUNK,
	S_BIG_PALMTREE_TOP,
	S_PALMTREE_TRUNK,
	S_PALMTREE_TOP,

	S_DBALL1,
	S_DBALL2,
	S_DBALL3,
	S_DBALL4,
	S_DBALL5,
	S_DBALL6,
	S_EGGSTATUE2,

	// Shield Orb
	S_ARMA1,
	S_ARMA2,
	S_ARMA3,
	S_ARMA4,
	S_ARMA5,
	S_ARMA6,
	S_ARMA7,
	S_ARMA8,
	S_ARMA9,
	S_ARMA10,
	S_ARMA11,
	S_ARMA12,
	S_ARMA13,
	S_ARMA14,
	S_ARMA15,
	S_ARMA16,

	S_ARMF1,
	S_ARMF2,
	S_ARMF3,
	S_ARMF4,
	S_ARMF5,
	S_ARMF6,
	S_ARMF7,
	S_ARMF8,
	S_ARMF9,
	S_ARMF10,
	S_ARMF11,
	S_ARMF12,
	S_ARMF13,
	S_ARMF14,
	S_ARMF15,
	S_ARMF16,
	S_ARMF17,
	S_ARMF18,
	S_ARMF19,
	S_ARMF20,
	S_ARMF21,
	S_ARMF22,
	S_ARMF23,
	S_ARMF24,
	S_ARMF25,
	S_ARMF26,
	S_ARMF27,
	S_ARMF28,
	S_ARMF29,
	S_ARMF30,
	S_ARMF31,
	S_ARMF32,

	S_ARMB1,
	S_ARMB2,
	S_ARMB3,
	S_ARMB4,
	S_ARMB5,
	S_ARMB6,
	S_ARMB7,
	S_ARMB8,
	S_ARMB9,
	S_ARMB10,
	S_ARMB11,
	S_ARMB12,
	S_ARMB13,
	S_ARMB14,
	S_ARMB15,
	S_ARMB16,
	S_ARMB17,
	S_ARMB18,
	S_ARMB19,
	S_ARMB20,
	S_ARMB21,
	S_ARMB22,
	S_ARMB23,
	S_ARMB24,
	S_ARMB25,
	S_ARMB26,
	S_ARMB27,
	S_ARMB28,
	S_ARMB29,
	S_ARMB30,
	S_ARMB31,
	S_ARMB32,

	S_WIND1,
	S_WIND2,
	S_WIND3,
	S_WIND4,
	S_WIND5,
	S_WIND6,
	S_WIND7,
	S_WIND8,

	S_MAGN1,
	S_MAGN2,
	S_MAGN3,
	S_MAGN4,
	S_MAGN5,
	S_MAGN6,
	S_MAGN7,
	S_MAGN8,
	S_MAGN9,
	S_MAGN10,
	S_MAGN11,
	S_MAGN12,
	S_MAGN13,

	S_FORC1,
	S_FORC2,
	S_FORC3,
	S_FORC4,
	S_FORC5,
	S_FORC6,
	S_FORC7,
	S_FORC8,
	S_FORC9,
	S_FORC10,

	S_FORC11,
	S_FORC12,
	S_FORC13,
	S_FORC14,
	S_FORC15,
	S_FORC16,
	S_FORC17,
	S_FORC18,
	S_FORC19,
	S_FORC20,

	S_FORC21,

	S_ELEM1,
	S_ELEM2,
	S_ELEM3,
	S_ELEM4,
	S_ELEM5,
	S_ELEM6,
	S_ELEM7,
	S_ELEM8,
	S_ELEM9,
	S_ELEM10,
	S_ELEM11,
	S_ELEM12,

	S_ELEM13,
	S_ELEM14,

	S_ELEMF1,
	S_ELEMF2,
	S_ELEMF3,
	S_ELEMF4,
	S_ELEMF5,
	S_ELEMF6,
	S_ELEMF7,
	S_ELEMF8,
	S_ELEMF9,
	S_ELEMF10,

	S_PITY1,
	S_PITY2,
	S_PITY3,
	S_PITY4,
	S_PITY5,
	S_PITY6,
	S_PITY7,
	S_PITY8,
	S_PITY9,
	S_PITY10,
	S_PITY11,
	S_PITY12,

	S_FIRS1,
	S_FIRS2,
	S_FIRS3,
	S_FIRS4,
	S_FIRS5,
	S_FIRS6,
	S_FIRS7,
	S_FIRS8,
	S_FIRS9,

	S_FIRS10,
	S_FIRS11,

	S_FIRSB1,
	S_FIRSB2,
	S_FIRSB3,
	S_FIRSB4,
	S_FIRSB5,
	S_FIRSB6,
	S_FIRSB7,
	S_FIRSB8,
	S_FIRSB9,

	S_FIRSB10,

	S_BUBS1,
	S_BUBS2,
	S_BUBS3,
	S_BUBS4,
	S_BUBS5,
	S_BUBS6,
	S_BUBS7,
	S_BUBS8,
	S_BUBS9,

	S_BUBS10,
	S_BUBS11,

	S_BUBSB1,
	S_BUBSB2,
	S_BUBSB3,
	S_BUBSB4,

	S_BUBSB5,
	S_BUBSB6,

	S_ZAPS1,
	S_ZAPS2,
	S_ZAPS3,
	S_ZAPS4,
	S_ZAPS5,
	S_ZAPS6,
	S_ZAPS7,
	S_ZAPS8,
	S_ZAPS9,
	S_ZAPS10,
	S_ZAPS11,
	S_ZAPS12,
	S_ZAPS13, // blank frame
	S_ZAPS14,
	S_ZAPS15,
	S_ZAPS16,

	S_ZAPSB1, // blank frame
	S_ZAPSB2,
	S_ZAPSB3,
	S_ZAPSB4,
	S_ZAPSB5,
	S_ZAPSB6,
	S_ZAPSB7,
	S_ZAPSB8,
	S_ZAPSB9,
	S_ZAPSB10,
	S_ZAPSB11, // blank frame

	//Thunder spark
	S_THUNDERCOIN_SPARK,

	// Invincibility Sparkles
	S_IVSP,

	// Super Sonic Spark
	S_SSPK1,
	S_SSPK2,
	S_SSPK3,
	S_SSPK4,
	S_SSPK5,

	// Flicky-sized bubble
	S_FLICKY_BUBBLE,

	// Bluebird
	S_FLICKY_01_OUT,
	S_FLICKY_01_FLAP1,
	S_FLICKY_01_FLAP2,
	S_FLICKY_01_FLAP3,
	S_FLICKY_01_STAND,
	S_FLICKY_01_CENTER,

	// Rabbit
	S_FLICKY_02_OUT,
	S_FLICKY_02_AIM,
	S_FLICKY_02_HOP,
	S_FLICKY_02_UP,
	S_FLICKY_02_DOWN,
	S_FLICKY_02_STAND,
	S_FLICKY_02_CENTER,

	// Chicken
	S_FLICKY_03_OUT,
	S_FLICKY_03_AIM,
	S_FLICKY_03_HOP,
	S_FLICKY_03_UP,
	S_FLICKY_03_FLAP1,
	S_FLICKY_03_FLAP2,
	S_FLICKY_03_STAND,
	S_FLICKY_03_CENTER,

	// Seal
	S_FLICKY_04_OUT,
	S_FLICKY_04_AIM,
	S_FLICKY_04_HOP,
	S_FLICKY_04_UP,
	S_FLICKY_04_DOWN,
	S_FLICKY_04_SWIM1,
	S_FLICKY_04_SWIM2,
	S_FLICKY_04_SWIM3,
	S_FLICKY_04_SWIM4,
	S_FLICKY_04_STAND,
	S_FLICKY_04_CENTER,

	// Pig
	S_FLICKY_05_OUT,
	S_FLICKY_05_AIM,
	S_FLICKY_05_HOP,
	S_FLICKY_05_UP,
	S_FLICKY_05_DOWN,
	S_FLICKY_05_STAND,
	S_FLICKY_05_CENTER,

	// Chipmunk
	S_FLICKY_06_OUT,
	S_FLICKY_06_AIM,
	S_FLICKY_06_HOP,
	S_FLICKY_06_UP,
	S_FLICKY_06_DOWN,
	S_FLICKY_06_STAND,
	S_FLICKY_06_CENTER,

	// Penguin
	S_FLICKY_07_OUT,
	S_FLICKY_07_AIML,
	S_FLICKY_07_HOPL,
	S_FLICKY_07_UPL,
	S_FLICKY_07_DOWNL,
	S_FLICKY_07_AIMR,
	S_FLICKY_07_HOPR,
	S_FLICKY_07_UPR,
	S_FLICKY_07_DOWNR,
	S_FLICKY_07_SWIM1,
	S_FLICKY_07_SWIM2,
	S_FLICKY_07_SWIM3,
	S_FLICKY_07_STAND,
	S_FLICKY_07_CENTER,

	// Fish
	S_FLICKY_08_OUT,
	S_FLICKY_08_AIM,
	S_FLICKY_08_HOP,
	S_FLICKY_08_FLAP1,
	S_FLICKY_08_FLAP2,
	S_FLICKY_08_FLAP3,
	S_FLICKY_08_FLAP4,
	S_FLICKY_08_SWIM1,
	S_FLICKY_08_SWIM2,
	S_FLICKY_08_SWIM3,
	S_FLICKY_08_SWIM4,
	S_FLICKY_08_STAND,
	S_FLICKY_08_CENTER,

	// Ram
	S_FLICKY_09_OUT,
	S_FLICKY_09_AIM,
	S_FLICKY_09_HOP,
	S_FLICKY_09_UP,
	S_FLICKY_09_DOWN,
	S_FLICKY_09_STAND,
	S_FLICKY_09_CENTER,

	// Puffin
	S_FLICKY_10_OUT,
	S_FLICKY_10_FLAP1,
	S_FLICKY_10_FLAP2,
	S_FLICKY_10_STAND,
	S_FLICKY_10_CENTER,

	// Cow
	S_FLICKY_11_OUT,
	S_FLICKY_11_AIM,
	S_FLICKY_11_RUN1,
	S_FLICKY_11_RUN2,
	S_FLICKY_11_RUN3,
	S_FLICKY_11_STAND,
	S_FLICKY_11_CENTER,

	// Rat
	S_FLICKY_12_OUT,
	S_FLICKY_12_AIM,
	S_FLICKY_12_RUN1,
	S_FLICKY_12_RUN2,
	S_FLICKY_12_RUN3,
	S_FLICKY_12_STAND,
	S_FLICKY_12_CENTER,

	// Bear
	S_FLICKY_13_OUT,
	S_FLICKY_13_AIM,
	S_FLICKY_13_HOP,
	S_FLICKY_13_UP,
	S_FLICKY_13_DOWN,
	S_FLICKY_13_STAND,
	S_FLICKY_13_CENTER,

	// Dove
	S_FLICKY_14_OUT,
	S_FLICKY_14_FLAP1,
	S_FLICKY_14_FLAP2,
	S_FLICKY_14_FLAP3,
	S_FLICKY_14_STAND,
	S_FLICKY_14_CENTER,

	// Cat
	S_FLICKY_15_OUT,
	S_FLICKY_15_AIM,
	S_FLICKY_15_HOP,
	S_FLICKY_15_UP,
	S_FLICKY_15_DOWN,
	S_FLICKY_15_STAND,
	S_FLICKY_15_CENTER,

	// Canary
	S_FLICKY_16_OUT,
	S_FLICKY_16_FLAP1,
	S_FLICKY_16_FLAP2,
	S_FLICKY_16_FLAP3,
	S_FLICKY_16_STAND,
	S_FLICKY_16_CENTER,

	// Spider
	S_SECRETFLICKY_01_OUT,
	S_SECRETFLICKY_01_AIM,
	S_SECRETFLICKY_01_HOP,
	S_SECRETFLICKY_01_UP,
	S_SECRETFLICKY_01_DOWN,
	S_SECRETFLICKY_01_STAND,
	S_SECRETFLICKY_01_CENTER,

	// Bat
	S_SECRETFLICKY_02_OUT,
	S_SECRETFLICKY_02_FLAP1,
	S_SECRETFLICKY_02_FLAP2,
	S_SECRETFLICKY_02_FLAP3,
	S_SECRETFLICKY_02_STAND,
	S_SECRETFLICKY_02_CENTER,

	// Fan
	S_FAN,
	S_FAN2,
	S_FAN3,
	S_FAN4,
	S_FAN5,

	// Steam Riser
	S_STEAM1,
	S_STEAM2,
	S_STEAM3,
	S_STEAM4,
	S_STEAM5,
	S_STEAM6,
	S_STEAM7,
	S_STEAM8,

	// Bumpers
	S_BUMPER,
	S_BUMPERHIT,

	// Balloons
	S_BALLOON,
	S_BALLOONPOP1,
	S_BALLOONPOP2,
	S_BALLOONPOP3,
	S_BALLOONPOP4,
	S_BALLOONPOP5,
	S_BALLOONPOP6,

	// Yellow Spring
	S_YELLOWSPRING1,
	S_YELLOWSPRING2,
	S_YELLOWSPRING3,
	S_YELLOWSPRING4,

	// Red Spring
	S_REDSPRING1,
	S_REDSPRING2,
	S_REDSPRING3,
	S_REDSPRING4,

	// Blue Spring
	S_BLUESPRING1,
	S_BLUESPRING2,
	S_BLUESPRING3,
	S_BLUESPRING4,

	// Grey Spring
	S_GREYSPRING1,
	S_GREYSPRING2,
	S_GREYSPRING3,
	S_GREYSPRING4,

	// Orange Spring (Pogo)
	S_POGOSPRING1,
	S_POGOSPRING2,
	S_POGOSPRING2B,
	S_POGOSPRING3,
	S_POGOSPRING4,

	// Yellow Diagonal Spring
	S_YDIAG1,
	S_YDIAG2,
	S_YDIAG3,
	S_YDIAG4,

	// Red Diagonal Spring
	S_RDIAG1,
	S_RDIAG2,
	S_RDIAG3,
	S_RDIAG4,

	// Blue Diagonal Spring
	S_BDIAG1,
	S_BDIAG2,
	S_BDIAG3,
	S_BDIAG4,

	// Grey Diagonal Spring
	S_GDIAG1,
	S_GDIAG2,
	S_GDIAG3,
	S_GDIAG4,

	// Yellow Horizontal Spring
	S_YHORIZ1,
	S_YHORIZ2,
	S_YHORIZ3,
	S_YHORIZ4,

	// Red Horizontal Spring
	S_RHORIZ1,
	S_RHORIZ2,
	S_RHORIZ3,
	S_RHORIZ4,

	// Blue Horizontal Spring
	S_BHORIZ1,
	S_BHORIZ2,
	S_BHORIZ3,
	S_BHORIZ4,

	// Grey Horizontal Spring
	S_GHORIZ1,
	S_GHORIZ2,
	S_GHORIZ3,
	S_GHORIZ4,

	// Booster
	S_BOOSTERSOUND,
	S_YELLOWBOOSTERROLLER,
	S_YELLOWBOOSTERSEG_LEFT,
	S_YELLOWBOOSTERSEG_RIGHT,
	S_YELLOWBOOSTERSEG_FACE,
	S_REDBOOSTERROLLER,
	S_REDBOOSTERSEG_LEFT,
	S_REDBOOSTERSEG_RIGHT,
	S_REDBOOSTERSEG_FACE,

	// Rain
	S_RAIN1,
	S_RAINRETURN,

	// Snowflake
	S_SNOW1,
	S_SNOW2,
	S_SNOW3,

	// Blizzard Snowball
	S_BLIZZARDSNOW1,
	S_BLIZZARDSNOW2,
	S_BLIZZARDSNOW3,

	// Water Splish
	S_SPLISH1,
	S_SPLISH2,
	S_SPLISH3,
	S_SPLISH4,
	S_SPLISH5,
	S_SPLISH6,
	S_SPLISH7,
	S_SPLISH8,
	S_SPLISH9,

	// Lava Splish
	S_LAVASPLISH,

	// added water splash
	S_SPLASH1,
	S_SPLASH2,
	S_SPLASH3,

	// lava/slime damage burn smoke
	S_SMOKE1,
	S_SMOKE2,
	S_SMOKE3,
	S_SMOKE4,
	S_SMOKE5,

	// Bubbles
	S_SMALLBUBBLE,
	S_MEDIUMBUBBLE,
	S_LARGEBUBBLE1,
	S_LARGEBUBBLE2,
	S_EXTRALARGEBUBBLE, // breathable

	S_POP1, // Extra Large bubble goes POP!

	S_WATERZAP,

	// Spindash dust
	S_SPINDUST1,
	S_SPINDUST2,
	S_SPINDUST3,
	S_SPINDUST4,
	S_SPINDUST_BUBBLE1,
	S_SPINDUST_BUBBLE2,
	S_SPINDUST_BUBBLE3,
	S_SPINDUST_BUBBLE4,
	S_SPINDUST_FIRE1,
	S_SPINDUST_FIRE2,
	S_SPINDUST_FIRE3,
	S_SPINDUST_FIRE4,

	S_FOG1,
	S_FOG2,
	S_FOG3,
	S_FOG4,
	S_FOG5,
	S_FOG6,
	S_FOG7,
	S_FOG8,
	S_FOG9,
	S_FOG10,
	S_FOG11,
	S_FOG12,
	S_FOG13,
	S_FOG14,

	S_SEED,

	S_PARTICLE,

	// Score Logos
	S_SCRA, // 100
	S_SCRB, // 200
	S_SCRC, // 500
	S_SCRD, // 1000
	S_SCRE, // 10000
	S_SCRF, // 400 (mario)
	S_SCRG, // 800 (mario)
	S_SCRH, // 2000 (mario)
	S_SCRI, // 4000 (mario)
	S_SCRJ, // 8000 (mario)
	S_SCRK, // 1UP (mario)
	S_SCRL, // 10

	// Drowning Timer Numbers
	S_ZERO1,
	S_ONE1,
	S_TWO1,
	S_THREE1,
	S_FOUR1,
	S_FIVE1,

	S_ZERO2,
	S_ONE2,
	S_TWO2,
	S_THREE2,
	S_FOUR2,
	S_FIVE2,

	S_FLIGHTINDICATOR,

	S_LOCKON1,
	S_LOCKON2,
	S_LOCKON3,
	S_LOCKON4,
	S_LOCKONINF1,
	S_LOCKONINF2,
	S_LOCKONINF3,
	S_LOCKONINF4,

	// Tag Sign
	S_TTAG,

	// Got Flag Sign
	S_GOTFLAG,

	// Finish flag
	S_FINISHFLAG,

	S_CORK,
	S_LHRT,

	// Red Ring
	S_RRNG1,
	S_RRNG2,
	S_RRNG3,
	S_RRNG4,
	S_RRNG5,
	S_RRNG6,
	S_RRNG7,

	// Weapon Ring Ammo
	S_BOUNCERINGAMMO,
	S_RAILRINGAMMO,
	S_INFINITYRINGAMMO,
	S_AUTOMATICRINGAMMO,
	S_EXPLOSIONRINGAMMO,
	S_SCATTERRINGAMMO,
	S_GRENADERINGAMMO,

	// Weapon pickup
	S_BOUNCEPICKUP,
	S_BOUNCEPICKUPFADE1,
	S_BOUNCEPICKUPFADE2,
	S_BOUNCEPICKUPFADE3,
	S_BOUNCEPICKUPFADE4,
	S_BOUNCEPICKUPFADE5,
	S_BOUNCEPICKUPFADE6,
	S_BOUNCEPICKUPFADE7,
	S_BOUNCEPICKUPFADE8,

	S_RAILPICKUP,
	S_RAILPICKUPFADE1,
	S_RAILPICKUPFADE2,
	S_RAILPICKUPFADE3,
	S_RAILPICKUPFADE4,
	S_RAILPICKUPFADE5,
	S_RAILPICKUPFADE6,
	S_RAILPICKUPFADE7,
	S_RAILPICKUPFADE8,

	S_AUTOPICKUP,
	S_AUTOPICKUPFADE1,
	S_AUTOPICKUPFADE2,
	S_AUTOPICKUPFADE3,
	S_AUTOPICKUPFADE4,
	S_AUTOPICKUPFADE5,
	S_AUTOPICKUPFADE6,
	S_AUTOPICKUPFADE7,
	S_AUTOPICKUPFADE8,

	S_EXPLODEPICKUP,
	S_EXPLODEPICKUPFADE1,
	S_EXPLODEPICKUPFADE2,
	S_EXPLODEPICKUPFADE3,
	S_EXPLODEPICKUPFADE4,
	S_EXPLODEPICKUPFADE5,
	S_EXPLODEPICKUPFADE6,
	S_EXPLODEPICKUPFADE7,
	S_EXPLODEPICKUPFADE8,

	S_SCATTERPICKUP,
	S_SCATTERPICKUPFADE1,
	S_SCATTERPICKUPFADE2,
	S_SCATTERPICKUPFADE3,
	S_SCATTERPICKUPFADE4,
	S_SCATTERPICKUPFADE5,
	S_SCATTERPICKUPFADE6,
	S_SCATTERPICKUPFADE7,
	S_SCATTERPICKUPFADE8,

	S_GRENADEPICKUP,
	S_GRENADEPICKUPFADE1,
	S_GRENADEPICKUPFADE2,
	S_GRENADEPICKUPFADE3,
	S_GRENADEPICKUPFADE4,
	S_GRENADEPICKUPFADE5,
	S_GRENADEPICKUPFADE6,
	S_GRENADEPICKUPFADE7,
	S_GRENADEPICKUPFADE8,

	// Thrown Weapon Rings
	S_THROWNBOUNCE1,
	S_THROWNBOUNCE2,
	S_THROWNBOUNCE3,
	S_THROWNBOUNCE4,
	S_THROWNBOUNCE5,
	S_THROWNBOUNCE6,
	S_THROWNBOUNCE7,
	S_THROWNINFINITY1,
	S_THROWNINFINITY2,
	S_THROWNINFINITY3,
	S_THROWNINFINITY4,
	S_THROWNINFINITY5,
	S_THROWNINFINITY6,
	S_THROWNINFINITY7,
	S_THROWNAUTOMATIC1,
	S_THROWNAUTOMATIC2,
	S_THROWNAUTOMATIC3,
	S_THROWNAUTOMATIC4,
	S_THROWNAUTOMATIC5,
	S_THROWNAUTOMATIC6,
	S_THROWNAUTOMATIC7,
	S_THROWNEXPLOSION1,
	S_THROWNEXPLOSION2,
	S_THROWNEXPLOSION3,
	S_THROWNEXPLOSION4,
	S_THROWNEXPLOSION5,
	S_THROWNEXPLOSION6,
	S_THROWNEXPLOSION7,
	S_THROWNGRENADE1,
	S_THROWNGRENADE2,
	S_THROWNGRENADE3,
	S_THROWNGRENADE4,
	S_THROWNGRENADE5,
	S_THROWNGRENADE6,
	S_THROWNGRENADE7,
	S_THROWNGRENADE8,
	S_THROWNGRENADE9,
	S_THROWNGRENADE10,
	S_THROWNGRENADE11,
	S_THROWNGRENADE12,
	S_THROWNGRENADE13,
	S_THROWNGRENADE14,
	S_THROWNGRENADE15,
	S_THROWNGRENADE16,
	S_THROWNGRENADE17,
	S_THROWNGRENADE18,
	S_THROWNSCATTER,

	S_RINGEXPLODE,

	S_COIN1,
	S_COIN2,
	S_COIN3,
	S_COINSPARKLE1,
	S_COINSPARKLE2,
	S_COINSPARKLE3,
	S_COINSPARKLE4,
	S_GOOMBA1,
	S_GOOMBA1B,
	S_GOOMBA2,
	S_GOOMBA3,
	S_GOOMBA4,
	S_GOOMBA5,
	S_GOOMBA6,
	S_GOOMBA7,
	S_GOOMBA8,
	S_GOOMBA9,
	S_GOOMBA_DEAD,
	S_BLUEGOOMBA1,
	S_BLUEGOOMBA1B,
	S_BLUEGOOMBA2,
	S_BLUEGOOMBA3,
	S_BLUEGOOMBA4,
	S_BLUEGOOMBA5,
	S_BLUEGOOMBA6,
	S_BLUEGOOMBA7,
	S_BLUEGOOMBA8,
	S_BLUEGOOMBA9,
	S_BLUEGOOMBA_DEAD,

	// Mario-specific stuff
	S_FIREFLOWER1,
	S_FIREFLOWER2,
	S_FIREFLOWER3,
	S_FIREFLOWER4,
	S_FIREBALL,
	S_FIREBALLTRAIL1,
	S_FIREBALLTRAIL2,
	S_SHELL,
	S_PUMA_START1,
	S_PUMA_START2,
	S_PUMA_UP1,
	S_PUMA_UP2,
	S_PUMA_UP3,
	S_PUMA_DOWN1,
	S_PUMA_DOWN2,
	S_PUMA_DOWN3,
	S_PUMATRAIL1,
	S_PUMATRAIL2,
	S_PUMATRAIL3,
	S_PUMATRAIL4,
	S_HAMMER,
	S_KOOPA1,
	S_KOOPA2,
	S_KOOPAFLAME1,
	S_KOOPAFLAME2,
	S_KOOPAFLAME3,
	S_AXE1,
	S_AXE2,
	S_AXE3,
	S_MARIOBUSH1,
	S_MARIOBUSH2,
	S_TOAD,

	// Nights-specific stuff
	S_NIGHTSDRONE_MAN1,
	S_NIGHTSDRONE_MAN2,
	S_NIGHTSDRONE_SPARKLING1,
	S_NIGHTSDRONE_SPARKLING2,
	S_NIGHTSDRONE_SPARKLING3,
	S_NIGHTSDRONE_SPARKLING4,
	S_NIGHTSDRONE_SPARKLING5,
	S_NIGHTSDRONE_SPARKLING6,
	S_NIGHTSDRONE_SPARKLING7,
	S_NIGHTSDRONE_SPARKLING8,
	S_NIGHTSDRONE_SPARKLING9,
	S_NIGHTSDRONE_SPARKLING10,
	S_NIGHTSDRONE_SPARKLING11,
	S_NIGHTSDRONE_SPARKLING12,
	S_NIGHTSDRONE_SPARKLING13,
	S_NIGHTSDRONE_SPARKLING14,
	S_NIGHTSDRONE_SPARKLING15,
	S_NIGHTSDRONE_SPARKLING16,
	S_NIGHTSDRONE_GOAL1,
	S_NIGHTSDRONE_GOAL2,
	S_NIGHTSDRONE_GOAL3,
	S_NIGHTSDRONE_GOAL4,

	S_NIGHTSPARKLE1,
	S_NIGHTSPARKLE2,
	S_NIGHTSPARKLE3,
	S_NIGHTSPARKLE4,
	S_NIGHTSPARKLESUPER1,
	S_NIGHTSPARKLESUPER2,
	S_NIGHTSPARKLESUPER3,
	S_NIGHTSPARKLESUPER4,
	S_NIGHTSLOOPHELPER,

	// NiGHTS bumper
	S_NIGHTSBUMPER1,
	S_NIGHTSBUMPER2,
	S_NIGHTSBUMPER3,
	S_NIGHTSBUMPER4,
	S_NIGHTSBUMPER5,
	S_NIGHTSBUMPER6,
	S_NIGHTSBUMPER7,
	S_NIGHTSBUMPER8,
	S_NIGHTSBUMPER9,
	S_NIGHTSBUMPER10,
	S_NIGHTSBUMPER11,
	S_NIGHTSBUMPER12,

	S_HOOP,
	S_HOOP_XMASA,
	S_HOOP_XMASB,

	S_NIGHTSCORE10,
	S_NIGHTSCORE20,
	S_NIGHTSCORE30,
	S_NIGHTSCORE40,
	S_NIGHTSCORE50,
	S_NIGHTSCORE60,
	S_NIGHTSCORE70,
	S_NIGHTSCORE80,
	S_NIGHTSCORE90,
	S_NIGHTSCORE100,
	S_NIGHTSCORE10_2,
	S_NIGHTSCORE20_2,
	S_NIGHTSCORE30_2,
	S_NIGHTSCORE40_2,
	S_NIGHTSCORE50_2,
	S_NIGHTSCORE60_2,
	S_NIGHTSCORE70_2,
	S_NIGHTSCORE80_2,
	S_NIGHTSCORE90_2,
	S_NIGHTSCORE100_2,

	// NiGHTS Paraloop Powerups
	S_NIGHTSSUPERLOOP,
	S_NIGHTSDRILLREFILL,
	S_NIGHTSHELPER,
	S_NIGHTSEXTRATIME,
	S_NIGHTSLINKFREEZE,
	S_EGGCAPSULE,

	// Orbiting Chaos Emeralds
	S_ORBITEM1,
	S_ORBITEM2,
	S_ORBITEM3,
	S_ORBITEM4,
	S_ORBITEM5,
	S_ORBITEM6,
	S_ORBITEM7,
	S_ORBITEM8,
	S_ORBIDYA1,
	S_ORBIDYA2,
	S_ORBIDYA3,
	S_ORBIDYA4,
	S_ORBIDYA5,

	// "Flicky" helper
	S_NIGHTOPIANHELPER1,
	S_NIGHTOPIANHELPER2,
	S_NIGHTOPIANHELPER3,
	S_NIGHTOPIANHELPER4,
	S_NIGHTOPIANHELPER5,
	S_NIGHTOPIANHELPER6,
	S_NIGHTOPIANHELPER7,
	S_NIGHTOPIANHELPER8,
	S_NIGHTOPIANHELPER9,

	// Nightopian
	S_PIAN0,
	S_PIAN1,
	S_PIAN2,
	S_PIAN3,
	S_PIAN4,
	S_PIAN5,
	S_PIAN6,
	S_PIANSING,

	// Shleep
	S_SHLEEP1,
	S_SHLEEP2,
	S_SHLEEP3,
	S_SHLEEP4,
	S_SHLEEPBOUNCE1,
	S_SHLEEPBOUNCE2,
	S_SHLEEPBOUNCE3,

	// Secret badniks and hazards, shhhh
	S_PENGUINATOR_LOOK,
	S_PENGUINATOR_WADDLE1,
	S_PENGUINATOR_WADDLE2,
	S_PENGUINATOR_WADDLE3,
	S_PENGUINATOR_WADDLE4,
	S_PENGUINATOR_SLIDE1,
	S_PENGUINATOR_SLIDE2,
	S_PENGUINATOR_SLIDE3,
	S_PENGUINATOR_SLIDE4,
	S_PENGUINATOR_SLIDE5,

	S_POPHAT_LOOK,
	S_POPHAT_SHOOT1,
	S_POPHAT_SHOOT2,
	S_POPHAT_SHOOT3,
	S_POPHAT_SHOOT4,
	S_POPSHOT,
	S_POPSHOT_TRAIL,

	S_HIVEELEMENTAL_LOOK,
	S_HIVEELEMENTAL_PREPARE1,
	S_HIVEELEMENTAL_PREPARE2,
	S_HIVEELEMENTAL_SHOOT1,
	S_HIVEELEMENTAL_SHOOT2,
	S_HIVEELEMENTAL_DORMANT,
	S_HIVEELEMENTAL_PAIN,
	S_HIVEELEMENTAL_DIE1,
	S_HIVEELEMENTAL_DIE2,
	S_HIVEELEMENTAL_DIE3,

	S_BUMBLEBORE_SPAWN,
	S_BUMBLEBORE_LOOK1,
	S_BUMBLEBORE_LOOK2,
	S_BUMBLEBORE_FLY1,
	S_BUMBLEBORE_FLY2,
	S_BUMBLEBORE_RAISE,
	S_BUMBLEBORE_FALL1,
	S_BUMBLEBORE_FALL2,
	S_BUMBLEBORE_STUCK1,
	S_BUMBLEBORE_STUCK2,
	S_BUMBLEBORE_DIE,

	S_BUGGLEIDLE,
	S_BUGGLEFLY,

	S_SMASHSPIKE_FLOAT,
	S_SMASHSPIKE_EASE1,
	S_SMASHSPIKE_EASE2,
	S_SMASHSPIKE_FALL,
	S_SMASHSPIKE_STOMP1,
	S_SMASHSPIKE_STOMP2,
	S_SMASHSPIKE_RISE1,
	S_SMASHSPIKE_RISE2,

	S_CACO_LOOK,
	S_CACO_WAKE1,
	S_CACO_WAKE2,
	S_CACO_WAKE3,
	S_CACO_WAKE4,
	S_CACO_ROAR,
	S_CACO_CHASE,
	S_CACO_CHASE_REPEAT,
	S_CACO_RANDOM,
	S_CACO_PREPARE_SOUND,
	S_CACO_PREPARE1,
	S_CACO_PREPARE2,
	S_CACO_PREPARE3,
	S_CACO_SHOOT_SOUND,
	S_CACO_SHOOT1,
	S_CACO_SHOOT2,
	S_CACO_CLOSE,
	S_CACO_DIE_FLAGS,
	S_CACO_DIE_GIB1,
	S_CACO_DIE_GIB2,
	S_CACO_DIE_SCREAM,
	S_CACO_DIE_SHATTER,
	S_CACO_DIE_FALL,
	S_CACOSHARD_RANDOMIZE,
	S_CACOSHARD1_1,
	S_CACOSHARD1_2,
	S_CACOSHARD2_1,
	S_CACOSHARD2_2,
	S_CACOFIRE1,
	S_CACOFIRE2,
	S_CACOFIRE3,
	S_CACOFIRE_EXPLODE1,
	S_CACOFIRE_EXPLODE2,
	S_CACOFIRE_EXPLODE3,
	S_CACOFIRE_EXPLODE4,

	S_SPINBOBERT_MOVE_FLIPUP,
	S_SPINBOBERT_MOVE_UP,
	S_SPINBOBERT_MOVE_FLIPDOWN,
	S_SPINBOBERT_MOVE_DOWN,
	S_SPINBOBERT_FIRE_MOVE,
	S_SPINBOBERT_FIRE_GHOST,
	S_SPINBOBERT_FIRE_TRAIL1,
	S_SPINBOBERT_FIRE_TRAIL2,
	S_SPINBOBERT_FIRE_TRAIL3,

	S_HANGSTER_LOOK,
	S_HANGSTER_SWOOP1,
	S_HANGSTER_SWOOP2,
	S_HANGSTER_ARC1,
	S_HANGSTER_ARC2,
	S_HANGSTER_ARC3,
	S_HANGSTER_FLY1,
	S_HANGSTER_FLY2,
	S_HANGSTER_FLY3,
	S_HANGSTER_FLY4,
	S_HANGSTER_FLYREPEAT,
	S_HANGSTER_ARCUP1,
	S_HANGSTER_ARCUP2,
	S_HANGSTER_ARCUP3,
	S_HANGSTER_RETURN1,
	S_HANGSTER_RETURN2,
	S_HANGSTER_RETURN3,

	S_CRUMBLE1,
	S_CRUMBLE2,

	// Spark
	S_SPRK1,
	S_SPRK2,
	S_SPRK3,

	// Robot Explosion
	S_XPLD_FLICKY,
	S_XPLD1,
	S_XPLD2,
	S_XPLD3,
	S_XPLD4,
	S_XPLD5,
	S_XPLD6,
	S_XPLD_EGGTRAP,

	// Underwater Explosion
	S_WPLD1,
	S_WPLD2,
	S_WPLD3,
	S_WPLD4,
	S_WPLD5,
	S_WPLD6,

	S_DUST1,
	S_DUST2,
	S_DUST3,
	S_DUST4,

	S_ROCKSPAWN,

	S_ROCKCRUMBLEA,
	S_ROCKCRUMBLEB,
	S_ROCKCRUMBLEC,
	S_ROCKCRUMBLED,
	S_ROCKCRUMBLEE,
	S_ROCKCRUMBLEF,
	S_ROCKCRUMBLEG,
	S_ROCKCRUMBLEH,
	S_ROCKCRUMBLEI,
	S_ROCKCRUMBLEJ,
	S_ROCKCRUMBLEK,
	S_ROCKCRUMBLEL,
	S_ROCKCRUMBLEM,
	S_ROCKCRUMBLEN,
	S_ROCKCRUMBLEO,
	S_ROCKCRUMBLEP,

	// Level debris
	S_GFZDEBRIS,
	S_BRICKDEBRIS,
	S_WOODDEBRIS,
	S_REDBRICKDEBRIS, // for CEZ3
	S_BLUEBRICKDEBRIS, // for CEZ3
	S_YELLOWBRICKDEBRIS, // for CEZ3

	//{ Random Item Box
	S_RANDOMITEM1,
	S_RANDOMITEM2,
	S_RANDOMITEM3,
	S_RANDOMITEM4,
	S_RANDOMITEM5,
	S_RANDOMITEM6,
	S_RANDOMITEM7,
	S_RANDOMITEM8,
	S_RANDOMITEM9,
	S_RANDOMITEM10,
	S_RANDOMITEM11,
	S_RANDOMITEM12,

	// Ring Box
	S_RINGBOX1,
	S_RINGBOX2,
	S_RINGBOX3,
	S_RINGBOX4,
	S_RINGBOX5,
	S_RINGBOX6,
	S_RINGBOX7,
	S_RINGBOX8,
	S_RINGBOX9,
	S_RINGBOX10,
	S_RINGBOX11,
	S_RINGBOX12,

	// Sphere Box (for Battle)
	S_SPHEREBOX1,
	S_SPHEREBOX2,
	S_SPHEREBOX3,
	S_SPHEREBOX4,
	S_SPHEREBOX5,
	S_SPHEREBOX6,
	S_SPHEREBOX7,
	S_SPHEREBOX8,
	S_SPHEREBOX9,
	S_SPHEREBOX10,
	S_SPHEREBOX11,
	S_SPHEREBOX12,

	// Random Item Pop
	S_RANDOMITEMPOP1,
	S_RANDOMITEMPOP2,
	S_RANDOMITEMPOP3,
	S_RANDOMITEMPOP4,
	//}

	S_ITEM_DEBRIS,
	S_ITEM_DEBRIS_CLOUD_SPAWNER1,
	S_ITEM_DEBRIS_CLOUD_SPAWNER2,

	S_ITEMICON,
	S_ITEMBACKDROP,

	// Item capsules
	S_ITEMCAPSULE,
	S_ITEMCAPSULE_TOP_SIDE,
	S_ITEMCAPSULE_BOTTOM_SIDE_AIR,
	S_ITEMCAPSULE_BOTTOM_SIDE_GROUND,
	//S_ITEMCAPSULE_TOP,
	//S_ITEMCAPSULE_BOTTOM,
	//S_ITEMCAPSULE_INSIDE,

	S_MONITOR_DAMAGE,
	S_MONITOR_DEATH,
	S_MONITOR_SCREEN1A,
	S_MONITOR_SCREEN1B,
	S_MONITOR_SCREEN2A,
	S_MONITOR_SCREEN2B,
	S_MONITOR_SCREEN3A,
	S_MONITOR_SCREEN3B,
	S_MONITOR_SCREEN4A,
	S_MONITOR_SCREEN4B,
	S_MONITOR_STAND,
	S_MONITOR_CRACKA,
	S_MONITOR_CRACKB,

	S_MONITOR_BIG_SHARD,
	S_MONITOR_SMALL_SHARD,
	S_MONITOR_TWINKLE,

	S_MAGICIANBOX,
	S_MAGICIANBOX_TOP,
	S_MAGICIANBOX_BOTTOM,

	S_WAVEDASH,

	S_INSTAWHIP,
	S_INSTAWHIP_RECHARGE1,
	S_INSTAWHIP_RECHARGE2,
	S_INSTAWHIP_RECHARGE3,
	S_INSTAWHIP_RECHARGE4,
	S_INSTAWHIP_REJECT,
	S_BLOCKRING,
	S_BLOCKBODY,

	S_CHARGEAURA,
	S_CHARGEFALL,
	S_CHARGEFLICKER,
	S_CHARGESPARK,
	S_CHARGERELEASE,
	S_CHARGEEXTRA,

	S_SERVANTHAND,

	S_HORNCODE,

	// Signpost sparkles
	S_SIGNSPARK1,
	S_SIGNSPARK2,
	S_SIGNSPARK3,
	S_SIGNSPARK4,
	S_SIGNSPARK5,
	S_SIGNSPARK6,
	S_SIGNSPARK7,
	S_SIGNSPARK8,
	S_SIGNSPARK9,
	S_SIGNSPARK10,
	S_SIGNSPARK11,

	// Drift Sparks
	S_DRIFTSPARK_A1,
	S_DRIFTSPARK_A2,
	S_DRIFTSPARK_A3,
	S_DRIFTSPARK_B1,
	S_DRIFTSPARK_C1,
	S_DRIFTSPARK_C2,
	S_DRIFTSPARK_D1,
	S_DRIFTSPARK_D2,

	// Brake drift sparks
	S_BRAKEDRIFT,

	// Brake dust
	S_BRAKEDUST1,
	S_BRAKEDUST2,

	// Drift Smoke
	S_DRIFTDUST1,
	S_DRIFTDUST2,
	S_DRIFTDUST3,
	S_DRIFTDUST4,

	// Drift Sparkles
	S_DRIFTWARNSPARK1,
	S_DRIFTWARNSPARK2,
	S_DRIFTWARNSPARK3,
	S_DRIFTWARNSPARK4,

	// Drift electricity
	S_DRIFTELECTRICITY,
	S_DRIFTELECTRICSPARK,

	// Fast lines
	S_FASTLINE1,
	S_FASTLINE2,
	S_FASTLINE3,
	S_FASTLINE4,
	S_FASTLINE5,

	// Fast dust release
	S_FASTDUST1,
	S_FASTDUST2,
	S_FASTDUST3,
	S_FASTDUST4,
	S_FASTDUST5,
	S_FASTDUST6,
	S_FASTDUST7,

	// Drift boost effect
	S_DRIFTEXPLODE1,
	S_DRIFTEXPLODE2,
	S_DRIFTEXPLODE3,
	S_DRIFTEXPLODE4,
	S_DRIFTEXPLODE5,
	S_DRIFTEXPLODE6,
	S_DRIFTEXPLODE7,
	S_DRIFTEXPLODE8,

	// Drift boost clip
	S_DRIFTCLIPA1,
	S_DRIFTCLIPA2,
	S_DRIFTCLIPA3,
	S_DRIFTCLIPA4,
	S_DRIFTCLIPA5,
	S_DRIFTCLIPA6,
	S_DRIFTCLIPA7,
	S_DRIFTCLIPA8,
	S_DRIFTCLIPA9,
	S_DRIFTCLIPA10,
	S_DRIFTCLIPA11,
	S_DRIFTCLIPA12,
	S_DRIFTCLIPA13,
	S_DRIFTCLIPA14,
	S_DRIFTCLIPA15,
	S_DRIFTCLIPA16,
	S_DRIFTCLIPB1,
	S_DRIFTCLIPB2,
	S_DRIFTCLIPB3,
	S_DRIFTCLIPB4,
	S_DRIFTCLIPB5,
	S_DRIFTCLIPB6,
	S_DRIFTCLIPB7,
	S_DRIFTCLIPB8,

	// Drift boost clip sparks
	S_DRIFTCLIPSPARK,

	// Sneaker boost effect
	S_BOOSTFLAME,
	S_BOOSTSMOKESPAWNER,
	S_BOOSTSMOKE1,
	S_BOOSTSMOKE2,
	S_BOOSTSMOKE3,
	S_BOOSTSMOKE4,
	S_BOOSTSMOKE5,
	S_BOOSTSMOKE6,

	// Sneaker Fire Trail
	S_KARTFIRE1,
	S_KARTFIRE2,
	S_KARTFIRE3,
	S_KARTFIRE4,
	S_KARTFIRE5,
	S_KARTFIRE6,
	S_KARTFIRE7,
	S_KARTFIRE8,

	// Angel Island Drift Strat Dust (what a mouthful!)
	S_KARTAIZDRIFTSTRAT,

	// Invincibility Sparks
	S_KARTINVULN1,
	S_KARTINVULN2,
	S_KARTINVULN3,
	S_KARTINVULN4,
	S_KARTINVULN5,
	S_KARTINVULN6,
	S_KARTINVULN7,
	S_KARTINVULN8,
	S_KARTINVULN9,
	S_KARTINVULN10,
	S_KARTINVULN11,
	S_KARTINVULN12,

	S_KARTINVULNB1,
	S_KARTINVULNB2,
	S_KARTINVULNB3,
	S_KARTINVULNB4,
	S_KARTINVULNB5,
	S_KARTINVULNB6,
	S_KARTINVULNB7,
	S_KARTINVULNB8,
	S_KARTINVULNB9,
	S_KARTINVULNB10,
	S_KARTINVULNB11,
	S_KARTINVULNB12,

	// Invincibility flash
	S_INVULNFLASH1,
	S_INVULNFLASH2,
	S_INVULNFLASH3,
	S_INVULNFLASH4,

	S_KARTINVLINES1,
	S_KARTINVLINES2,
	S_KARTINVLINES3,
	S_KARTINVLINES4,
	S_KARTINVLINES5,
	S_KARTINVLINES6,
	S_KARTINVLINES7,
	S_KARTINVLINES8,
	S_KARTINVLINES9,
	S_KARTINVLINES10,
	S_KARTINVLINES11,
	S_KARTINVLINES12,
	S_KARTINVLINES13,
	S_KARTINVLINES14,
	S_KARTINVLINES15,

	// Wipeout dust trail
	S_WIPEOUTTRAIL1,
	S_WIPEOUTTRAIL2,
	S_WIPEOUTTRAIL3,
	S_WIPEOUTTRAIL4,
	S_WIPEOUTTRAIL5,
	S_WIPEOUTTRAIL6,
	S_WIPEOUTTRAIL7,
	S_WIPEOUTTRAIL8,
	S_WIPEOUTTRAIL9,
	S_WIPEOUTTRAIL10,
	S_WIPEOUTTRAIL11,

	// Rocket sneaker
	S_ROCKETSNEAKER_L,
	S_ROCKETSNEAKER_R,
	S_ROCKETSNEAKER_LVIBRATE,
	S_ROCKETSNEAKER_RVIBRATE,

	//{ Eggman Monitor
	S_EGGMANITEM1,
	S_EGGMANITEM2,
	S_EGGMANITEM3,
	S_EGGMANITEM4,
	S_EGGMANITEM5,
	S_EGGMANITEM6,
	S_EGGMANITEM7,
	S_EGGMANITEM8,
	S_EGGMANITEM9,
	S_EGGMANITEM10,
	S_EGGMANITEM11,
	S_EGGMANITEM12,
	S_EGGMANITEM_DEAD,
	//}

	// Banana
	S_BANANA,
	S_BANANA_DEAD,

	S_BANANA_SPARK,
	S_BANANA_SPARK2,
	S_BANANA_SPARK3,
	S_BANANA_SPARK4,

	//{ Orbinaut
	S_ORBINAUT1,
	S_ORBINAUT2,
	S_ORBINAUT3,
	S_ORBINAUT4,
	S_ORBINAUT5,
	S_ORBINAUT6,
	S_ORBINAUT_DEAD,
	S_ORBINAUT_SHIELD1,
	S_ORBINAUT_SHIELD2,
	S_ORBINAUT_SHIELD3,
	S_ORBINAUT_SHIELD4,
	S_ORBINAUT_SHIELD5,
	S_ORBINAUT_SHIELD6,
	S_ORBINAUT_SHIELDDEAD,
	//}
	//{ Jawz
	S_JAWZ1,
	S_JAWZ2,
	S_JAWZ3,
	S_JAWZ4,
	S_JAWZ5,
	S_JAWZ6,
	S_JAWZ7,
	S_JAWZ8,
	S_JAWZ_SHIELD1,
	S_JAWZ_SHIELD2,
	S_JAWZ_SHIELD3,
	S_JAWZ_SHIELD4,
	S_JAWZ_SHIELD5,
	S_JAWZ_SHIELD6,
	S_JAWZ_SHIELD7,
	S_JAWZ_SHIELD8,
	S_JAWZ_DEAD1,
	S_JAWZ_DEAD2,
	//}

	S_PLAYERRETICULE, // Player reticule

	// Special Stage Mine
	S_SSMINE1,
	S_SSMINE2,
	S_SSMINE3,
	S_SSMINE4,
	S_SSMINE_SHIELD1,
	S_SSMINE_SHIELD2,
	S_SSMINE_AIR1,
	S_SSMINE_AIR2,
	S_SSMINE_DEPLOY1,
	S_SSMINE_DEPLOY2,
	S_SSMINE_DEPLOY3,
	S_SSMINE_DEPLOY4,
	S_SSMINE_DEPLOY5,
	S_SSMINE_DEPLOY6,
	S_SSMINE_DEPLOY7,
	S_SSMINE_DEPLOY8,
	S_SSMINE_DEPLOY9,
	S_SSMINE_DEPLOY10,
	S_SSMINE_DEPLOY11,
	S_SSMINE_DEPLOY12,
	S_SSMINE_DEPLOY13,
	S_SSMINE_EXPLODE,
	S_SSMINE_EXPLODE2,

	// New explosion
	S_QUICKBOOM1,
	S_QUICKBOOM2,
	S_QUICKBOOM3,
	S_QUICKBOOM4,
	S_QUICKBOOM5,
	S_QUICKBOOM6,
	S_QUICKBOOM7,
	S_QUICKBOOM8,
	S_QUICKBOOM9,
	S_QUICKBOOM10,

	S_SLOWBOOM1,
	S_SLOWBOOM2,
	S_SLOWBOOM3,
	S_SLOWBOOM4,
	S_SLOWBOOM5,
	S_SLOWBOOM6,
	S_SLOWBOOM7,
	S_SLOWBOOM8,
	S_SLOWBOOM9,
	S_SLOWBOOM10,

	// Land mine
	S_LANDMINE,
	S_LANDMINE_EXPLODE,

	// Drop Target
	S_DROPTARGET,
	S_DROPTARGET_SPIN,

	// Ballhog
	S_BALLHOG1,
	S_BALLHOG2,
	S_BALLHOG3,
	S_BALLHOG4,
	S_BALLHOG5,
	S_BALLHOG6,
	S_BALLHOG7,
	S_BALLHOG8,
	S_BALLHOG_DEAD,
	S_BALLHOGBOOM1,
	S_BALLHOGBOOM2,
	S_BALLHOGBOOM3,
	S_BALLHOGBOOM4,
	S_BALLHOGBOOM5,
	S_BALLHOGBOOM6,
	S_BALLHOGBOOM7,
	S_BALLHOGBOOM8,
	S_BALLHOGBOOM9,
	S_BALLHOGBOOM10,
	S_BALLHOGBOOM11,
	S_BALLHOGBOOM12,
	S_BALLHOGBOOM13,
	S_BALLHOGBOOM14,
	S_BALLHOGBOOM15,
	S_BALLHOGBOOM16,

	// Self-Propelled Bomb
	S_SPB1,
	S_SPB2,
	S_SPB3,
	S_SPB4,
	S_SPB5,
	S_SPB6,
	S_SPB7,
	S_SPB8,
	S_SPB9,
	S_SPB10,
	S_SPB11,
	S_SPB12,
	S_SPB13,
	S_SPB14,
	S_SPB15,
	S_SPB16,
	S_SPB17,
	S_SPB18,
	S_SPB19,
	S_SPB20,
	S_SPB_DEAD,

	// Juicebox for SPB
	S_MANTA1,
	S_MANTA2,

	// Thunder Shield
	S_LIGHTNINGSHIELD1,
	S_LIGHTNINGSHIELD2,
	S_LIGHTNINGSHIELD3,
	S_LIGHTNINGSHIELD4,
	S_LIGHTNINGSHIELD5,
	S_LIGHTNINGSHIELD6,
	S_LIGHTNINGSHIELD7,
	S_LIGHTNINGSHIELD8,
	S_LIGHTNINGSHIELD9,
	S_LIGHTNINGSHIELD10,
	S_LIGHTNINGSHIELD11,
	S_LIGHTNINGSHIELD12,
	S_LIGHTNINGSHIELD13,
	S_LIGHTNINGSHIELD14,
	S_LIGHTNINGSHIELD15,
	S_LIGHTNINGSHIELD16,
	S_LIGHTNINGSHIELD17,
	S_LIGHTNINGSHIELD18,
	S_LIGHTNINGSHIELD19,
	S_LIGHTNINGSHIELD20,
	S_LIGHTNINGSHIELD21,
	S_LIGHTNINGSHIELD22,
	S_LIGHTNINGSHIELD23,
	S_LIGHTNINGSHIELD24,

	// Bubble Shield
	S_BUBBLESHIELD1,
	S_BUBBLESHIELD2,
	S_BUBBLESHIELD3,
	S_BUBBLESHIELD4,
	S_BUBBLESHIELD5,
	S_BUBBLESHIELD6,
	S_BUBBLESHIELD7,
	S_BUBBLESHIELD8,
	S_BUBBLESHIELD9,
	S_BUBBLESHIELD10,
	S_BUBBLESHIELD11,
	S_BUBBLESHIELD12,
	S_BUBBLESHIELD13,
	S_BUBBLESHIELD14,
	S_BUBBLESHIELD15,
	S_BUBBLESHIELD16,
	S_BUBBLESHIELD17,
	S_BUBBLESHIELD18,
	S_BUBBLESHIELDBLOWUP,
	S_BUBBLESHIELDTRAP1,
	S_BUBBLESHIELDTRAP2,
	S_BUBBLESHIELDTRAP3,
	S_BUBBLESHIELDTRAP4,
	S_BUBBLESHIELDTRAP5,
	S_BUBBLESHIELDTRAP6,
	S_BUBBLESHIELDTRAP7,
	S_BUBBLESHIELDTRAP8,
	S_BUBBLESHIELDWAVE1,
	S_BUBBLESHIELDWAVE2,
	S_BUBBLESHIELDWAVE3,
	S_BUBBLESHIELDWAVE4,
	S_BUBBLESHIELDWAVE5,
	S_BUBBLESHIELDWAVE6,

	// Flame Shield
	S_FLAMESHIELD1,
	S_FLAMESHIELD2,
	S_FLAMESHIELD3,
	S_FLAMESHIELD4,
	S_FLAMESHIELD5,
	S_FLAMESHIELD6,
	S_FLAMESHIELD7,
	S_FLAMESHIELD8,
	S_FLAMESHIELD9,
	S_FLAMESHIELD10,
	S_FLAMESHIELD11,
	S_FLAMESHIELD12,
	S_FLAMESHIELD13,
	S_FLAMESHIELD14,
	S_FLAMESHIELD15,
	S_FLAMESHIELD16,
	S_FLAMESHIELD17,
	S_FLAMESHIELD18,

	S_FLAMESHIELDDASH1,
	S_FLAMESHIELDDASH2,
	S_FLAMESHIELDDASH3,
	S_FLAMESHIELDDASH4,
	S_FLAMESHIELDDASH5,
	S_FLAMESHIELDDASH6,
	S_FLAMESHIELDDASH7,
	S_FLAMESHIELDDASH8,
	S_FLAMESHIELDDASH9,
	S_FLAMESHIELDDASH10,
	S_FLAMESHIELDDASH11,
	S_FLAMESHIELDDASH12,

	S_FLAMESHIELDDASH2_UNDERLAY,
	S_FLAMESHIELDDASH5_UNDERLAY,
	S_FLAMESHIELDDASH8_UNDERLAY,
	S_FLAMESHIELDDASH11_UNDERLAY,

	S_FLAMESHIELDPAPER,
	S_FLAMESHIELDLINE1,
	S_FLAMESHIELDLINE2,
	S_FLAMESHIELDLINE3,
	S_FLAMESHIELDFLASH,

	// Marble Garden Zone Spinning Top
	S_GARDENTOP_FLOATING,
	S_GARDENTOP_SINKING1,
	S_GARDENTOP_SINKING2,
	S_GARDENTOP_SINKING3,
	S_GARDENTOP_DEAD,
	S_GARDENTOPSPARK,
	S_GARDENTOPARROW,

	// Caked-Up Booty-Sheet Ghost
	S_HYUDORO,
	S_HYUDORO_RETURNING,

	// Grow
	S_GROW_PARTICLE,

	// Shrink
	S_SHRINK_POHBEE,
	S_SHRINK_POHBEE2,
	S_SHRINK_POHBEE3,
	S_SHRINK_POHBEE4,
	S_SHRINK_POHBEE5,
	S_SHRINK_POHBEE6,
	S_SHRINK_POHBEE7,
	S_SHRINK_POHBEE8,

	S_SHRINK_CHAIN,

	S_SHRINK_GUN,
	S_SHRINK_GUN_OVERLAY,

	S_SHRINK_LASER,
	S_SHRINK_PARTICLE,

	// The legend
	S_SINK,
	S_SINK_SHIELD,
	S_SINKTRAIL1,
	S_SINKTRAIL2,
	S_SINKTRAIL3,

	// Battle Mode bumpers
	S_BATTLEBUMPER1,
	S_BATTLEBUMPER2,
	S_BATTLEBUMPER3,

	S_BATTLEBUMPER_EXCRYSTALA1,
	S_BATTLEBUMPER_EXCRYSTALA2,
	S_BATTLEBUMPER_EXCRYSTALA3,
	S_BATTLEBUMPER_EXCRYSTALA4,

	S_BATTLEBUMPER_EXCRYSTALB1,
	S_BATTLEBUMPER_EXCRYSTALB2,
	S_BATTLEBUMPER_EXCRYSTALB3,
	S_BATTLEBUMPER_EXCRYSTALB4,

	S_BATTLEBUMPER_EXCRYSTALC1,
	S_BATTLEBUMPER_EXCRYSTALC2,
	S_BATTLEBUMPER_EXCRYSTALC3,
	S_BATTLEBUMPER_EXCRYSTALC4,

	S_BATTLEBUMPER_EXSHELLA1,
	S_BATTLEBUMPER_EXSHELLA2,

	S_BATTLEBUMPER_EXSHELLB1,
	S_BATTLEBUMPER_EXSHELLB2,

	S_BATTLEBUMPER_EXSHELLC1,
	S_BATTLEBUMPER_EXSHELLC2,

	S_BATTLEBUMPER_EXDEBRIS1,
	S_BATTLEBUMPER_EXDEBRIS2,

	S_BATTLEBUMPER_EXBLAST1,
	S_BATTLEBUMPER_EXBLAST2,
	S_BATTLEBUMPER_EXBLAST3,
	S_BATTLEBUMPER_EXBLAST4,
	S_BATTLEBUMPER_EXBLAST5,
	S_BATTLEBUMPER_EXBLAST6,
	S_BATTLEBUMPER_EXBLAST7,
	S_BATTLEBUMPER_EXBLAST8,
	S_BATTLEBUMPER_EXBLAST9,
	S_BATTLEBUMPER_EXBLAST10,

	S_TRIPWIREBOOST_TOP,
	S_TRIPWIREBOOST_BOTTOM,
	S_TRIPWIREBOOST_BLAST_TOP,
	S_TRIPWIREBOOST_BLAST_BOTTOM,

	S_SMOOTHLANDING,

	S_TRICKINDICATOR_OVERLAY,
	S_TRICKINDICATOR_UNDERLAY,
	S_TRICKINDICATOR_OVERLAY_ARROW,
	S_TRICKINDICATOR_UNDERLAY_ARROW,
	S_TRICKINDICATOR_UNDERLAY_ARROW2,

	S_SIDETRICK,
	S_BACKTRICK,
	S_FORWARDTRICK,

	// DEZ Ring Shooter
	S_TIREGRABBER,
	S_RINGSHOOTER_SIDE,
	S_RINGSHOOTER_NIPPLES,
	S_RINGSHOOTER_SCREEN,
	S_RINGSHOOTER_NUMBERBACK,
	S_RINGSHOOTER_NUMBERFRONT,
	S_RINGSHOOTER_FACE,

	// DEZ Laser respawn
	S_DEZLASER,
	S_DEZLASER_TRAIL1,
	S_DEZLASER_TRAIL2,
	S_DEZLASER_TRAIL3,
	S_DEZLASER_TRAIL4,
	S_DEZLASER_TRAIL5,

	// 1.0 Kart Decoratives
	S_FLAYM1,
	S_FLAYM2,
	S_FLAYM3,
	S_FLAYM4,
	S_DEVIL,
	S_ANGEL,
	S_PALMTREE,
	S_FLAG,
	S_HEDGEHOG, // (Rimshot)
	S_BUSH1,
	S_TWEE,
	S_HYDRANT,

	// New Misc Decorations
	S_BIGPUMA1,
	S_BIGPUMA2,
	S_BIGPUMA3,
	S_BIGPUMA4,
	S_BIGPUMA5,
	S_BIGPUMA6,
	S_APPLE1,
	S_APPLE2,
	S_APPLE3,
	S_APPLE4,
	S_APPLE5,
	S_APPLE6,
	S_APPLE7,
	S_APPLE8,

	// Chaotix Big Ring
	S_BIGRING01,
	S_BIGRING02,
	S_BIGRING03,
	S_BIGRING04,
	S_BIGRING05,
	S_BIGRING06,
	S_BIGRING07,
	S_BIGRING08,
	S_BIGRING09,
	S_BIGRING10,
	S_BIGRING11,
	S_BIGRING12,

	// SNES Objects
	S_SNES_DONUTBUSH1,
	S_SNES_DONUTBUSH2,
	S_SNES_DONUTBUSH3,

	// GBA Objects
	S_GBA_BOO1,
	S_GBA_BOO2,
	S_GBA_BOO3,
	S_GBA_BOO4,

	// Sapphire Coast Mobs
	S_BUZZBOMBER_LOOK1,
	S_BUZZBOMBER_LOOK2,
	S_BUZZBOMBER_FLY1,
	S_BUZZBOMBER_FLY2,
	S_BUZZBOMBER_FLY3,
	S_BUZZBOMBER_FLY4,

	S_CHOMPER_SPAWN,
	S_CHOMPER_HOP1,
	S_CHOMPER_HOP2,
	S_CHOMPER_TURNAROUND,

	S_PALMTREE2,
	S_PURPLEFLOWER1,
	S_PURPLEFLOWER2,
	S_YELLOWFLOWER1,
	S_YELLOWFLOWER2,
	S_PLANT2,
	S_PLANT3,
	S_PLANT4,

	// Crystal Abyss Mobs
	S_SKULL,
	S_PHANTREE,
	S_FLYINGGARG1,
	S_FLYINGGARG2,
	S_FLYINGGARG3,
	S_FLYINGGARG4,
	S_FLYINGGARG5,
	S_FLYINGGARG6,
	S_FLYINGGARG7,
	S_FLYINGGARG8,
	S_LAMPPOST,
	S_MOSSYTREE,

	// Ark Arrows
	S_ARKARROW_0,
	S_ARKARROW_1,
	S_ARKARROW_2,
	S_ARKARROW_3,
	S_ARKARROW_4,
	S_ARKARROW_5,
	S_ARKARROW_6,
	S_ARKARROW_7,
	S_ARKARROW_8,
	S_ARKARROW_9,
	S_ARKARROW_10,
	S_ARKARROW_11,
	S_ARKARROW_12,
	S_ARKARROW_13,
	S_ARKARROW_14,
	S_ARKARROW_15,
	S_ARKARROW_16,
	S_ARKARROW_17,
	S_ARKARROW_18,
	S_ARKARROW_19,
	S_ARKARROW_20,
	S_ARKARROW_21,
	S_ARKARROW_22,
	S_ARKARROW_23,
	S_ARKARROW_24,
	S_ARKARROW_25,
	S_ARKARROW_26,
	S_ARKARROW_27,
	S_ARKARROW_28,
	S_ARKARROW_29,
	S_ARKARROW_30,
	S_ARKARROW_31,
	S_ARKARROW_32,
	S_ARKARROW_33,
	S_ARKARROW_34,
	S_ARKARROW_35,
	S_ARKARROW_36,
	S_ARKARROW_37,
	S_ARKARROW_38,
	S_ARKARROW_39,
	S_ARKARROW_40,
	S_ARKARROW_41,

	S_BUMP1,
	S_BUMP2,
	S_BUMP3,

	S_FLINGENERGY1,
	S_FLINGENERGY2,
	S_FLINGENERGY3,

	S_CLASH1,
	S_CLASH2,
	S_CLASH3,
	S_CLASH4,
	S_CLASH5,
	S_CLASH6,

	S_FIREDITEM1,
	S_FIREDITEM2,
	S_FIREDITEM3,
	S_FIREDITEM4,

	S_INSTASHIELDA1, // No damage instashield effect
	S_INSTASHIELDA2,
	S_INSTASHIELDA3,
	S_INSTASHIELDA4,
	S_INSTASHIELDA5,
	S_INSTASHIELDA6,
	S_INSTASHIELDA7,
	S_INSTASHIELDB1,
	S_INSTASHIELDB2,
	S_INSTASHIELDB3,
	S_INSTASHIELDB4,
	S_INSTASHIELDB5,
	S_INSTASHIELDB6,
	S_INSTASHIELDB7,

	S_POWERCLASH, // Grow/Invinc clash VFX
	S_GUARDBREAK,

	S_PLAYERARROW, // Above player arrow
	S_PLAYERARROW_BOX,
	S_PLAYERARROW_ITEM,
	S_PLAYERARROW_NUMBER,
	S_PLAYERARROW_X,
	S_PLAYERARROW_WANTED1,
	S_PLAYERARROW_WANTED2,
	S_PLAYERARROW_WANTED3,
	S_PLAYERARROW_WANTED4,
	S_PLAYERARROW_WANTED5,
	S_PLAYERARROW_WANTED6,
	S_PLAYERARROW_WANTED7,

	S_PLAYERBOMB1, // Karma player overlays
	S_PLAYERBOMB2,
	S_PLAYERBOMB3,
	S_PLAYERBOMB4,
	S_PLAYERBOMB5,
	S_PLAYERBOMB6,
	S_PLAYERBOMB7,
	S_PLAYERBOMB8,
	S_PLAYERBOMB9,
	S_PLAYERBOMB10,
	S_PLAYERBOMB11,
	S_PLAYERBOMB12,
	S_PLAYERBOMB13,
	S_PLAYERBOMB14,
	S_PLAYERBOMB15,
	S_PLAYERBOMB16,
	S_PLAYERBOMB17,
	S_PLAYERBOMB18,
	S_PLAYERBOMB19,
	S_PLAYERBOMB20,

	S_PLAYERITEM1,
	S_PLAYERITEM2,
	S_PLAYERITEM3,
	S_PLAYERITEM4,
	S_PLAYERITEM5,
	S_PLAYERITEM6,
	S_PLAYERITEM7,
	S_PLAYERITEM8,
	S_PLAYERITEM9,
	S_PLAYERITEM10,
	S_PLAYERITEM11,
	S_PLAYERITEM12,

	S_PLAYERFAKE1,
	S_PLAYERFAKE2,
	S_PLAYERFAKE3,
	S_PLAYERFAKE4,
	S_PLAYERFAKE5,
	S_PLAYERFAKE6,
	S_PLAYERFAKE7,
	S_PLAYERFAKE8,
	S_PLAYERFAKE9,
	S_PLAYERFAKE10,
	S_PLAYERFAKE11,
	S_PLAYERFAKE12,

	S_KARMAWHEEL,

	S_BATTLEPOINT1A, // Battle point indicators
	S_BATTLEPOINT1B,
	S_BATTLEPOINT1C,
	S_BATTLEPOINT1D,
	S_BATTLEPOINT1E,
	S_BATTLEPOINT1F,
	S_BATTLEPOINT1G,
	S_BATTLEPOINT1H,
	S_BATTLEPOINT1I,

	S_BATTLEPOINT2A,
	S_BATTLEPOINT2B,
	S_BATTLEPOINT2C,
	S_BATTLEPOINT2D,
	S_BATTLEPOINT2E,
	S_BATTLEPOINT2F,
	S_BATTLEPOINT2G,
	S_BATTLEPOINT2H,
	S_BATTLEPOINT2I,

	S_BATTLEPOINT3A,
	S_BATTLEPOINT3B,
	S_BATTLEPOINT3C,
	S_BATTLEPOINT3D,
	S_BATTLEPOINT3E,
	S_BATTLEPOINT3F,
	S_BATTLEPOINT3G,
	S_BATTLEPOINT3H,
	S_BATTLEPOINT3I,

	// Thunder shield use stuff;
	S_KSPARK1,	// Sparkling Radius
	S_KSPARK2,
	S_KSPARK3,
	S_KSPARK4,
	S_KSPARK5,
	S_KSPARK6,
	S_KSPARK7,
	S_KSPARK8,
	S_KSPARK9,
	S_KSPARK10,
	S_KSPARK11,
	S_KSPARK12,
	S_KSPARK13,	// ... that's an awful lot.

	S_LZIO11,	// Straight lightning bolt
	S_LZIO12,
	S_LZIO13,
	S_LZIO14,
	S_LZIO15,
	S_LZIO16,
	S_LZIO17,
	S_LZIO18,
	S_LZIO19,

	S_LZIO21,	// Straight lightning bolt (flipped)
	S_LZIO22,
	S_LZIO23,
	S_LZIO24,
	S_LZIO25,
	S_LZIO26,
	S_LZIO27,
	S_LZIO28,
	S_LZIO29,

	S_KLIT1,	// Diagonal lightning. No, it not being straight doesn't make it gay.
	S_KLIT2,
	S_KLIT3,
	S_KLIT4,
	S_KLIT5,
	S_KLIT6,
	S_KLIT7,
	S_KLIT8,
	S_KLIT9,
	S_KLIT10,
	S_KLIT11,
	S_KLIT12,

	S_FZEROSMOKE1, // F-Zero NO CONTEST explosion
	S_FZEROSMOKE2,
	S_FZEROSMOKE3,
	S_FZEROSMOKE4,
	S_FZEROSMOKE5,

	S_FZEROBOOM1,
	S_FZEROBOOM2,
	S_FZEROBOOM3,
	S_FZEROBOOM4,
	S_FZEROBOOM5,
	S_FZEROBOOM6,
	S_FZEROBOOM7,
	S_FZEROBOOM8,
	S_FZEROBOOM9,
	S_FZEROBOOM10,
	S_FZEROBOOM11,
	S_FZEROBOOM12,

	S_FZSLOWSMOKE1,
	S_FZSLOWSMOKE2,
	S_FZSLOWSMOKE3,
	S_FZSLOWSMOKE4,
	S_FZSLOWSMOKE5,

	// Dash Rings
	S_DASHRING_HORIZONTAL,
	S_DASHRING_30DEGREES,
	S_DASHRING_60DEGREES,
	S_DASHRING_VERTICAL,
	S_DASHRING_HORIZONTAL_FLASH1,
	S_DASHRING_HORIZONTAL_FLASH2,
	S_DASHRING_30DEGREES_FLASH1,
	S_DASHRING_30DEGREES_FLASH2,
	S_DASHRING_60DEGREES_FLASH1,
	S_DASHRING_60DEGREES_FLASH2,
	S_DASHRING_VERTICAL_FLASH1,
	S_DASHRING_VERTICAL_FLASH2,

	// Sneaker Panels
	S_SNEAKERPANEL,
	S_SNEAKERPANEL_SMALL,
	S_SNEAKERPANEL_TINY,

	// Various plants
	S_SONICBUSH,

	// Marble Zone
	S_MARBLEFLAMEPARTICLE,
	S_MARBLETORCH,
	S_MARBLELIGHT,
	S_MARBLEBURNER,

	// CD Special Stage
	S_CDUFO,
	S_CDUFO_DIE,

	// Rusty Rig
	S_RUSTYLAMP_ORANGE,
	S_RUSTYCHAIN,

	// Smokin' & Vapin' (Don't try this at home, kids!)
	S_PETSMOKE0,
	S_PETSMOKE1,
	S_PETSMOKE2,
	S_PETSMOKE3,
	S_PETSMOKE4,
	S_PETSMOKE5,
	S_VVVAPING0,
	S_VVVAPING1,
	S_VVVAPING2,
	S_VVVAPING3,
	S_VVVAPING4,
	S_VVVAPING5,
	S_VVVAPE,

	// Hill Top Zone
	S_HTZTREE,
	S_HTZBUSH,

	// Ports of gardens
	S_SGVINE1,
	S_SGVINE2,
	S_SGVINE3,
	S_PGTREE,
	S_PGFLOWER1,
	S_PGFLOWER2,
	S_PGFLOWER3,
	S_PGBUSH,
	S_DHPILLAR,

	// Midnight Channel stuff:
	S_SPOTLIGHT,	// Spotlight decoration
	S_RANDOMSHADOW,	// Random Shadow. They're static and don't do nothing.
	S_GARU1,
	S_GARU2,
	S_GARU3,
	S_TGARU0,
	S_TGARU1,
	S_TGARU2,
	S_TGARU3,	// Wind attack used by Roaming Shadows on Players.
	S_ROAMINGSHADOW,	// Roaming Shadow (the one that uses above's wind attack or smth)
	S_MAYONAKAARROW,	// Arrow sign

	// Mementos stuff:
	S_REAPER_INVIS,		// Reaper waiting for spawning
	S_REAPER,			// Reaper main frame where its thinker is handled
	S_MEMENTOSTP,		// Mementos teleporter state. (Used for spawning particles)

	// JackInTheBox
	S_JITB1,
	S_JITB2,
	S_JITB3,
	S_JITB4,
	S_JITB5,
	S_JITB6,

	// Color Drive
	S_CDMOONSP,
	S_CDBUSHSP,
	S_CDTREEASP,
	S_CDTREEBSP,

	// Daytona Speedway
	S_DAYTONAPINETREE,
	S_DAYTONAPINETREE_SIDE,

	// Egg Zeppelin
	S_EZZPROPELLER,
	S_EZZPROPELLER_BLADE,

	// Desert Palace
	S_DP_PALMTREE,

	// Aurora Atoll
	S_AAZTREE_SEG,
	S_AAZTREE_COCONUT,
	S_AAZTREE_LEAF,

	// Barren Badlands
	S_BBZDUST1, // Dust
	S_BBZDUST2,
	S_BBZDUST3,
	S_BBZDUST4,
	S_FROGGER, // Frog badniks
	S_FROGGER_ATTACK,
	S_FROGGER_JUMP,
	S_FROGTONGUE,
	S_FROGTONGUE_JOINT,
	S_ROBRA, // Black cobra badniks
	S_ROBRA_HEAD,
	S_ROBRA_JOINT,
	S_ROBRASHELL_INSIDE,
	S_ROBRASHELL_OUTSIDE,
	S_BLUEROBRA, // Blue cobra badniks
	S_BLUEROBRA_HEAD,
	S_BLUEROBRA_JOINT,

	// Eerie Grove
	S_EERIEFOG1,
	S_EERIEFOG2,
	S_EERIEFOG3,
	S_EERIEFOG4,
	S_EERIEFOG5,

	// Chaos Chute
	S_SPECIALSTAGEARCH,
	S_SPECIALSTAGEBOMB,
	S_SPECIALSTAGEBOMB_DISARM,
	S_SPECIALSTAGEBOMB_EXPLODE,
	S_SPECIALSTAGEBOMB_DISAPPEAR,
	S_SPECIALSTAGEBOMB_FLICKER1,
	S_SPECIALSTAGEBOMB_FLICKER2,
	S_SPECIALSTAGEBOMB_FLICKERLOOP,
	S_SPECIALSTAGEBOMB_RESET,

	// Hanagumi Hall
	S_HANAGUMIHALL_STEAM,
	S_ALFONSO,
	S_SAKURA,
	S_SUMIRE,
	S_MARIA,
	S_IRIS,
	S_KOHRAN,
	S_KANNA,
	S_OGAMI,

	// SMK ports
	S_SMK_PIPE1, // Generic pipes
	S_SMK_PIPE2,
	S_SMK_MOLE, // Donut Plains Monty Moles
	S_SMK_THWOMP, // Bowser Castle Thwomps
	S_SMK_SNOWBALL, // Vanilla Lake snowballs
	S_SMK_ICEBLOCK, // Vanilla Lake breakable ice blocks
	S_SMK_ICEBLOCK2,
	S_SMK_ICEBLOCK_DEBRIS,
	S_SMK_ICEBLOCK_DEBRIS2,

	// Ezo's maps
	S_BLUEFIRE1,
	S_BLUEFIRE2,
	S_BLUEFIRE3,
	S_BLUEFIRE4,

	S_GREENFIRE1,
	S_GREENFIRE2,
	S_GREENFIRE3,
	S_GREENFIRE4,

	S_REGALCHEST,
	S_CHIMERASTATUE,
	S_DRAGONSTATUE,
	S_LIZARDMANSTATUE,
	S_PEGASUSSTATUE,

	S_ZELDAFIRE1,
	S_ZELDAFIRE2,
	S_ZELDAFIRE3,
	S_ZELDAFIRE4,

	S_GANBARETHING,
	S_GANBAREDUCK,
	S_GANBARETREE,

	S_MONOIDLE,
	S_MONOCHASE1,
	S_MONOCHASE2,
	S_MONOCHASE3,
	S_MONOCHASE4,
	S_MONOPAIN,

	S_REDZELDAFIRE1,
	S_REDZELDAFIRE2,
	S_REDZELDAFIRE3,
	S_REDZELDAFIRE4,

	S_BOWLINGPIN,
	S_BOWLINGHIT1,
	S_BOWLINGHIT2,
	S_BOWLINGHIT3,
	S_BOWLINGHIT4,

	S_ARIDTOAD,
	S_TOADHIT1,
	S_TOADHIT2,
	S_TOADHIT3,
	S_TOADHIT4,

	S_EBARRELIDLE,
	S_EBARREL1,
	S_EBARREL2,
	S_EBARREL3,
	S_EBARREL4,
	S_EBARREL5,
	S_EBARREL6,
	S_EBARREL7,
	S_EBARREL8,
	S_EBARREL9,
	S_EBARREL10,
	S_EBARREL11,
	S_EBARREL12,
	S_EBARREL13,
	S_EBARREL14,
	S_EBARREL15,
	S_EBARREL16,
	S_EBARREL17,
	S_EBARREL18,
	S_EBARREL19,

	S_MERRYHORSE,

	S_BLUEFRUIT,
	S_ORANGEFRUIT,
	S_REDFRUIT,
	S_PINKFRUIT,

	S_ADVENTURESPIKEA1,
	S_ADVENTURESPIKEA2,
	S_ADVENTURESPIKEB1,
	S_ADVENTURESPIKEB2,
	S_ADVENTURESPIKEC1,
	S_ADVENTURESPIKEC2,

	S_BOOSTPROMPT1,
	S_BOOSTPROMPT2,

	S_BOOSTOFF1,
	S_BOOSTOFF2,

	S_BOOSTON1,
	S_BOOSTON2,

	S_LIZARDMAN,
	S_LIONMAN,

	S_SUNBEAMPALM_STEM,
	S_SUNBEAMPALM_LEAF,

	S_KARMAFIREWORK1,
	S_KARMAFIREWORK2,
	S_KARMAFIREWORK3,
	S_KARMAFIREWORK4,
	S_KARMAFIREWORKTRAIL,

	S_OPAQUESMOKE1,
	S_OPAQUESMOKE2,
	S_OPAQUESMOKE3,
	S_OPAQUESMOKE4,
	S_OPAQUESMOKE5,

	// followers:

	// bubble:
	S_FOLLOWERBUBBLE_FRONT,
	S_FOLLOWERBUBBLE_BACK,

	// generic chao:
	S_GCHAOIDLE,
	S_GCHAOFLY,
	S_GCHAOSAD1,
	S_GCHAOSAD2,
	S_GCHAOSAD3,
	S_GCHAOSAD4,
	S_GCHAOHAPPY1,
	S_GCHAOHAPPY2,
	S_GCHAOHAPPY3,
	S_GCHAOHAPPY4,

	// cheese:
	S_CHEESEIDLE,
	S_CHEESEFLY,
	S_CHEESESAD1,
	S_CHEESESAD2,
	S_CHEESESAD3,
	S_CHEESESAD4,
	S_CHEESEHAPPY1,
	S_CHEESEHAPPY2,
	S_CHEESEHAPPY3,
	S_CHEESEHAPPY4,

	S_RINGDEBT,
	S_RINGSPARKS1,
	S_RINGSPARKS2,
	S_RINGSPARKS3,
	S_RINGSPARKS4,
	S_RINGSPARKS5,
	S_RINGSPARKS6,
	S_RINGSPARKS7,
	S_RINGSPARKS8,
	S_RINGSPARKS9,
	S_RINGSPARKS10,
	S_RINGSPARKS11,
	S_RINGSPARKS12,
	S_RINGSPARKS13,
	S_RINGSPARKS14,
	S_RINGSPARKS15,

	S_GAINAX_TINY,
	S_GAINAX_HUGE,
	S_GAINAX_MID1,
	S_GAINAX_MID2,

	S_DRAFTDUST1,
	S_DRAFTDUST2,
	S_DRAFTDUST3,
	S_DRAFTDUST4,
	S_DRAFTDUST5,

	S_TIREGREASE,

	S_OVERTIME_BULB1,
	S_OVERTIME_BULB2,
	S_OVERTIME_LASER,
	S_OVERTIME_CENTER,

	S_BATTLECAPSULE_SIDE1,
	S_BATTLECAPSULE_SIDE2,
	S_BATTLECAPSULE_TOP,
	S_BATTLECAPSULE_BUTTON,
	S_BATTLECAPSULE_SUPPORT,
	S_BATTLECAPSULE_SUPPORTFLY,

	S_WAYPOINTORB,
	S_WAYPOINTSPLAT,
	S_EGOORB,

	S_WATERTRAIL1,
	S_WATERTRAIL2,
	S_WATERTRAIL3,
	S_WATERTRAIL4,
	S_WATERTRAIL5,
	S_WATERTRAIL6,
	S_WATERTRAIL7,
	S_WATERTRAIL8,
	S_WATERTRAILUNDERLAY1,
	S_WATERTRAILUNDERLAY2,
	S_WATERTRAILUNDERLAY3,
	S_WATERTRAILUNDERLAY4,
	S_WATERTRAILUNDERLAY5,
	S_WATERTRAILUNDERLAY6,
	S_WATERTRAILUNDERLAY7,
	S_WATERTRAILUNDERLAY8,

	S_SPINDASHDUST,
	S_SPINDASHWIND,

	S_SOFTLANDING1,
	S_SOFTLANDING2,
	S_SOFTLANDING3,
	S_SOFTLANDING4,
	S_SOFTLANDING5,

	S_DOWNLINE1,
	S_DOWNLINE2,
	S_DOWNLINE3,
	S_DOWNLINE4,
	S_DOWNLINE5,

	S_HOLDBUBBLE,

	S_FINISHBEAM1,
	S_FINISHBEAM2,
	S_FINISHBEAM3,
	S_FINISHBEAM4,
	S_FINISHBEAM5,
	S_FINISHBEAMEND1,
	S_FINISHBEAMEND2,

	S_DEBTSPIKE1,
	S_DEBTSPIKE2,
	S_DEBTSPIKE3,
	S_DEBTSPIKE4,
	S_DEBTSPIKE5,
	S_DEBTSPIKE6,
	S_DEBTSPIKE7,
	S_DEBTSPIKE8,
	S_DEBTSPIKE9,
	S_DEBTSPIKEA,
	S_DEBTSPIKEB,
	S_DEBTSPIKEC,
	S_DEBTSPIKED,
	S_DEBTSPIKEE,

	S_JANKSPARK1,
	S_JANKSPARK2,
	S_JANKSPARK3,
	S_JANKSPARK4,

	S_HITLAG_1,
	S_HITLAG_2,
	S_HITLAG_3,
	S_HITLAG_4,
	S_HITLAG_5,
	S_HITLAG_6,
	S_HITLAG_8,
	S_HITLAG_9,
	S_HITLAG_10,

	// Broly Ki Orb
	S_BROLY1,
	S_BROLY2,

	S_SPECIAL_UFO_POD,
	S_SPECIAL_UFO_OVERLAY,
	S_SPECIAL_UFO_GLASS,
	S_SPECIAL_UFO_GLASS_UNDER,
	S_SPECIAL_UFO_ARM,
	S_SPECIAL_UFO_STEM,

	S_GACHABOM,
	S_GACHABOM_DEAD,

	S_GACHABOM_EXPLOSION_1,
	S_GACHABOM_EXPLOSION_2,
	S_GACHABOM_EXPLOSION_3A,
	S_GACHABOM_EXPLOSION_3B,
	S_GACHABOM_EXPLOSION_4,
	S_GACHABOM_WAITING,
	S_GACHABOM_RETURNING,

	S_SUPER_FLICKY,

	S_BATTLEUFO,
	S_BATTLEUFO_LEG,
	S_BATTLEUFO_DIE,
	S_BATTLEUFO_BEAM1,
	S_BATTLEUFO_BEAM2,

	S_POWERUP_AURA,

	S_CHECKPOINT,
	S_CHECKPOINT_ARM,
	S_CHECKPOINT_ORB_DEAD,
	S_CHECKPOINT_ORB_LIVE,
	S_CHECKPOINT_SPARK1,
	S_CHECKPOINT_SPARK2,
	S_CHECKPOINT_SPARK3,
	S_CHECKPOINT_SPARK4,
	S_CHECKPOINT_SPARK5,
	S_CHECKPOINT_SPARK6,
	S_CHECKPOINT_SPARK7,
	S_CHECKPOINT_SPARK8,
	S_CHECKPOINT_SPARK9,
	S_CHECKPOINT_SPARK10,
	S_CHECKPOINT_SPARK11,

	// rideroid
	S_RIDEROID,
	S_RIDEROID_ICON,

	// leaf storm
	S_EGGBALL,

	// dead line zone
	S_DLZHOVER,
	S_DLZROCKET_L,
	S_DLZROCKET_R,

	// water palace zone
	S_WPZFOUNTAIN,
	S_WPZFOUNTAINANIM,
	S_KURAGEN,
	S_KURAGENBOMB,

	S_BALLSWITCH_BALL,
	S_BALLSWITCH_BALL_ACTIVE,
	S_BALLSWITCH_PAD,
	S_BALLSWITCH_PAD_ACTIVE,

	S_SPIKEDTARGET,
	S_SPIKEDLENS,

	S_BLENDEYE_MAIN,
	S_BLENDEYE_MAIN_LAUNCHED,
	S_BLENDEYE_EYE,
	S_BLENDEYE_EYE_FLASH,
	S_BLENDEYE_GLASS,
	S_BLENDEYE_GLASS_STRESS,

	S_BLENDEYE_SHIELD,
	S_BLENDEYE_SHIELD_L,
	S_BLENDEYE_SHIELD_R,
	S_BLENDEYE_SHIELD_BUSTED,
	S_BLENDEYE_SHIELD_BUSTED_L,
	S_BLENDEYE_SHIELD_BUSTED_R,

	S_BLENDEYE_EGGBEATER_EXTEND_1,
	S_BLENDEYE_EGGBEATER_EXTEND_2,
	S_BLENDEYE_EGGBEATER,
	S_BLENDEYE_EGGBEATER_SPIN,

	S_BLENDEYE_FLAME,

	S_BLENDEYE_GENERATOR,
	S_BLENDEYE_GENERATOR_BUSTED_L,
	S_BLENDEYE_GENERATOR_BUSTED_R,

	S_BLENDEYE_PUYO_SPAWN_1,
	S_BLENDEYE_PUYO_SPAWN_2,
	S_BLENDEYE_PUYO_SPAWN_3,
	S_BLENDEYE_PUYO,
	S_BLENDEYE_PUYO_LAND_1,
	S_BLENDEYE_PUYO_LAND_2,
	S_BLENDEYE_PUYO_LAND_3,
	S_BLENDEYE_PUYO_LAND_4,
	S_BLENDEYE_PUYO_SHOCK,
	S_BLENDEYE_PUYO_DIE,
	S_BLENDEYE_PUYO_DUST,

	S_MEGABARRIER1,
	S_MEGABARRIER2,
	S_MEGABARRIER3,

	S_GPZ_TREETHING_B,
	S_GPZ_TREETHING_M,
	S_GPZ_TREETHING_S,

	// MT_GGZFREEZETHRUSTER
	S_GGZFREEZETHRUSTER,

	// MT_GGZICEDUST
	S_GGZICEDUST1,
	S_GGZICEDUST2,
	S_GGZICEDUST3,
	S_GGZICEDUST4,
	S_GGZICEDUST5,
	S_GGZICEDUST6,
	S_GGZICEDUST7,
	S_GGZICEDUST8,
	S_GGZICEDUST9,
	S_GGZICEDUST10,
	S_GGZICEDUST11,
	S_GGZPARTICLE11,
	S_GGZPARTICLE12,
	S_GGZPARTICLE13,
	S_GGZPARTICLE14,
	S_GGZPARTICLE15,
	S_GGZPARTICLE16,
	S_GGZPARTICLE17,
	S_GGZPARTICLE18,
	S_GGZPARTICLE21,
	S_GGZPARTICLE22,
	S_GGZPARTICLE23,
	S_GGZPARTICLE24,

	S_GGZICECUBE,

	// MT_THRUSTERPART
	S_THRUSTERPART,

	// MT_IVOBALL
	S_IVOBALL,

	S_SA2_CRATE_DEBRIS,
	S_SA2_CRATE_DEBRIS_E,
	S_SA2_CRATE_DEBRIS_F,
	S_SA2_CRATE_DEBRIS_G,
	S_SA2_CRATE_DEBRIS_H,
	S_SA2_CRATE_DEBRIS_METAL,

	S_ICECAPBLOCK_DEBRIS,
	S_ICECAPBLOCK_DEBRIS_C,
	S_ICECAPBLOCK_DEBRIS_D,
	S_ICECAPBLOCK_DEBRIS_E,
	S_ICECAPBLOCK_DEBRIS_F,

	// MT_SPEAR
	S_SPEAR_ROD,
	S_SPEAR_TIP,
	S_SPEAR_HILT_FRONT,
	S_SPEAR_HILT_BACK,
	S_SPEAR_WALL,

	// MT_BSZLAMP_S
	S_BLMS,
	S_BLMM,
	S_BLML,

	// MT_BSZSLAMP
	S_BSWL,
	S_BSWC,

	S_BETA_PARTICLE_WHEEL,
	S_BETA_PARTICLE_ICON,
	S_BETA_PARTICLE_EXPLOSION,

	S_FIRSTFREESLOT,
	S_LASTFREESLOT = S_FIRSTFREESLOT + NUMSTATEFREESLOTS - 1,
	NUMSTATES
} statenum_t;

struct state_t
{
	spritenum_t sprite;
	UINT32 frame; // we use the upper 16 bits for translucency and other shade effects
	INT32 tics;
	actionf_t action;
	INT32 var1;
	INT32 var2;
	statenum_t nextstate;
};

extern state_t states[NUMSTATES];
extern char sprnames[NUMSPRITES + 1][5];
extern char spr2names[NUMPLAYERSPRITES][5];
extern playersprite_t spr2defaults[NUMPLAYERSPRITES];
extern state_t *astate;
extern playersprite_t free_spr2;

typedef enum mobj_type
{
	MT_NULL,
	MT_RAY, // General purpose mobj
	MT_UNKNOWN,

	MT_THOK, // Thok! mobj
	MT_SHADOW, // Linkdraw Shadow (for invisible objects)
	MT_PLAYER,
	MT_KART_LEFTOVER,
	MT_KART_TIRE,

	// Enemies
	MT_BLUECRAWLA, // Crawla (Blue)
	MT_REDCRAWLA, // Crawla (Red)
	MT_GFZFISH, // SDURF
	MT_GOLDBUZZ, // Buzz (Gold)
	MT_REDBUZZ, // Buzz (Red)
	MT_JETTBOMBER, // Jetty-Syn Bomber
	MT_JETTGUNNER, // Jetty-Syn Gunner
	MT_CRAWLACOMMANDER, // Crawla Commander
	MT_DETON, // Deton
	MT_SKIM, // Skim mine dropper
	MT_TURRET, // Industrial Turret
	MT_POPUPTURRET, // Pop-Up Turret
	MT_SPINCUSHION, // Spincushion
	MT_CRUSHSTACEAN, // Crushstacean
	MT_CRUSHCLAW, // Big meaty claw
	MT_CRUSHCHAIN, // Chain
	MT_BANPYURA, // Banpyura
	MT_BANPSPRING, // Banpyura spring
	MT_JETJAW, // Jet Jaw
	MT_SNAILER, // Snailer
	MT_VULTURE, // BASH
	MT_POINTY, // Pointy
	MT_POINTYBALL, // Pointy Ball
	MT_ROBOHOOD, // Robo-Hood
	MT_FACESTABBER, // Castlebot Facestabber
	MT_FACESTABBERSPEAR, // Castlebot Facestabber spear aura
	MT_EGGGUARD, // Egg Guard
	MT_EGGSHIELD, // Egg Guard's shield
	MT_GSNAPPER, // Green Snapper
	MT_SNAPPER_LEG, // Green Snapper leg
	MT_SNAPPER_HEAD, // Green Snapper head
	MT_MINUS, // Minus
	MT_MINUSDIRT, // Minus dirt
	MT_SPRINGSHELL, // Spring Shell
	MT_YELLOWSHELL, // Spring Shell (yellow)
	MT_UNIDUS, // Unidus
	MT_UNIBALL, // Unidus Ball
	MT_CANARIVORE, // Canarivore
	MT_CANARIVORE_GAS, // Canarivore gas
	MT_PYREFLY, // Pyre Fly
	MT_PYREFLY_FIRE, // Pyre Fly fire
	MT_PTERABYTESPAWNER, // Pterabyte spawner
	MT_PTERABYTEWAYPOINT, // Pterabyte waypoint
	MT_PTERABYTE, // Pterabyte
	MT_DRAGONBOMBER, // Dragonbomber
	MT_DRAGONWING, // Dragonbomber wing
	MT_DRAGONTAIL, // Dragonbomber tail segment
	MT_DRAGONMINE, // Dragonbomber mine

	// Generic Boss Items
	MT_BOSSEXPLODE,
	MT_SONIC3KBOSSEXPLODE,
	MT_BOSSFLYPOINT,
	MT_EGGTRAP,
	MT_BOSS3WAYPOINT,
	MT_BOSS9GATHERPOINT,
	MT_BOSSJUNK,

	// Boss 1
	MT_EGGMOBILE,
	MT_JETFUME1,
	MT_EGGMOBILE_BALL,
	MT_EGGMOBILE_TARGET,
	MT_EGGMOBILE_FIRE,

	// Boss 2
	MT_EGGMOBILE2,
	MT_EGGMOBILE2_POGO,
	MT_GOOP,
	MT_GOOPTRAIL,

	// Boss 3
	MT_EGGMOBILE3,
	MT_FAKEMOBILE,
	MT_SHOCKWAVE,

	// Boss 4
	MT_EGGMOBILE4,
	MT_EGGMOBILE4_MACE,
	MT_JETFLAME,
	MT_EGGROBO1,
	MT_EGGROBO1JET,

	// Boss 5
	MT_FANG,
	MT_BROKENROBOT,
	MT_VWREF,
	MT_VWREB,
	MT_PROJECTORLIGHT,
	MT_FBOMB,
	MT_TNTDUST, // also used by barrel
	MT_FSGNA,
	MT_FSGNB,
	MT_FANGWAYPOINT,

	// Metal Sonic (Boss 9)
	MT_METALSONIC_RACE,
	MT_METALSONIC_BATTLE,
	MT_MSSHIELD_FRONT,
	MT_MSGATHER,

	// Collectible Items
	MT_RING,
	MT_FLINGRING, // Lost ring
	MT_DEBTSPIKE, // Ring debt funny spike
	MT_BLUESPHERE,  // Blue sphere for special stages
	MT_FLINGBLUESPHERE, // Lost blue sphere
	MT_BOMBSPHERE,
	MT_REDTEAMRING,  //Rings collectable by red team.
	MT_BLUETEAMRING, //Rings collectable by blue team.
	MT_TOKEN, // Special Stage token for special stage
	MT_REDFLAG, // Red CTF Flag
	MT_BLUEFLAG, // Blue CTF Flag
	MT_EMBLEM,
	MT_SPRAYCAN,
	MT_ANCIENTSHRINE,
	MT_EMERALD,
	MT_EMERALDSPARK,
	MT_EMERALDFLARE,
	MT_PRISONEGGDROP,

	// Springs and others
	MT_FAN,
	MT_STEAM,
	MT_BUMPER,
	MT_BALLOON,

	MT_YELLOWSPRING,
	MT_REDSPRING,
	MT_BLUESPRING,
	MT_GREYSPRING,
	MT_POGOSPRING,
	MT_YELLOWDIAG, // Yellow Diagonal Spring
	MT_REDDIAG, // Red Diagonal Spring
	MT_BLUEDIAG, // Blue Diagonal Spring
	MT_GREYDIAG, // Grey Diagonal Spring
	MT_YELLOWHORIZ, // Yellow Horizontal Spring
	MT_REDHORIZ, // Red Horizontal Spring
	MT_BLUEHORIZ, // Blue Horizontal Spring
	MT_GREYHORIZ, // Grey Horizontal Spring

	MT_BOOSTERSEG,
	MT_BOOSTERROLLER,
	MT_YELLOWBOOSTER,
	MT_REDBOOSTER,

	// Interactive Objects
	MT_BUBBLES, // Bubble source
	MT_SIGN, // Level end sign
	MT_SIGN_PIECE,
	MT_SPIKEBALL, // Spike Ball
	MT_SPINFIRE,
	MT_SPIKE,
	MT_WALLSPIKE,
	MT_WALLSPIKEBASE,
	MT_CHEATCHECK,
	MT_BIGMINE,
	MT_BLASTEXECUTOR,
	MT_CANNONLAUNCHER,

	// Monitor miscellany
	MT_BOXSPARKLE,

	// Monitor boxes -- regular
	MT_RING_BOX,
	MT_PITY_BOX,
	MT_ATTRACT_BOX,
	MT_FORCE_BOX,
	MT_ARMAGEDDON_BOX,
	MT_WHIRLWIND_BOX,
	MT_ELEMENTAL_BOX,
	MT_SNEAKERS_BOX,
	MT_INVULN_BOX,
	MT_1UP_BOX,
	MT_EGGMAN_BOX,
	MT_MIXUP_BOX,
	MT_MYSTERY_BOX,
	MT_GRAVITY_BOX,
	MT_RECYCLER_BOX,
	MT_SCORE1K_BOX,
	MT_SCORE10K_BOX,
	MT_FLAMEAURA_BOX,
	MT_BUBBLEWRAP_BOX,
	MT_THUNDERCOIN_BOX,

	// Monitor boxes -- repeating (big) boxes
	MT_PITY_GOLDBOX,
	MT_ATTRACT_GOLDBOX,
	MT_FORCE_GOLDBOX,
	MT_ARMAGEDDON_GOLDBOX,
	MT_WHIRLWIND_GOLDBOX,
	MT_ELEMENTAL_GOLDBOX,
	MT_SNEAKERS_GOLDBOX,
	MT_INVULN_GOLDBOX,
	MT_EGGMAN_GOLDBOX,
	MT_GRAVITY_GOLDBOX,
	MT_FLAMEAURA_GOLDBOX,
	MT_BUBBLEWRAP_GOLDBOX,
	MT_THUNDERCOIN_GOLDBOX,

	// Monitor boxes -- special
	MT_RING_REDBOX,
	MT_RING_BLUEBOX,

	// Monitor icons
	MT_RING_ICON,
	MT_PITY_ICON,
	MT_ATTRACT_ICON,
	MT_FORCE_ICON,
	MT_ARMAGEDDON_ICON,
	MT_WHIRLWIND_ICON,
	MT_ELEMENTAL_ICON,
	MT_SNEAKERS_ICON,
	MT_INVULN_ICON,
	MT_1UP_ICON,
	MT_EGGMAN_ICON,
	MT_MIXUP_ICON,
	MT_GRAVITY_ICON,
	MT_RECYCLER_ICON,
	MT_SCORE1K_ICON,
	MT_SCORE10K_ICON,
	MT_FLAMEAURA_ICON,
	MT_BUBBLEWRAP_ICON,
	MT_THUNDERCOIN_ICON,

	// Projectiles
	MT_ROCKET,
	MT_LASER,
	MT_TORPEDO,
	MT_TORPEDO2, // silent
	MT_ENERGYBALL,
	MT_MINE, // Skim/Jetty-Syn mine
	MT_JETTBULLET, // Jetty-Syn Bullet
	MT_TURRETLASER,
	MT_CANNONBALL, // Cannonball
	MT_CANNONBALLDECOR, // Decorative/still cannonball
	MT_ARROW, // Arrow
	MT_DEMONFIRE, // Glaregoyle fire

	// The letter
	MT_LETTER,

	// Greenflower Scenery
	MT_GFZFLOWER1,
	MT_GFZFLOWER2,
	MT_GFZFLOWER3,

	MT_BLUEBERRYBUSH,
	MT_BERRYBUSH,
	MT_BUSH,

	// Trees (both GFZ and misc)
	MT_GFZTREE,
	MT_GFZBERRYTREE,
	MT_GFZCHERRYTREE,
	MT_CHECKERTREE,
	MT_CHECKERSUNSETTREE,
	MT_FHZTREE, // Frozen Hillside
	MT_FHZPINKTREE,
	MT_POLYGONTREE,
	MT_BUSHTREE,
	MT_BUSHREDTREE,
	MT_SPRINGTREE,

	// Techno Hill Scenery
	MT_THZFLOWER1,
	MT_THZFLOWER2,
	MT_THZFLOWER3,
	MT_THZTREE, // Steam whistle tree/bush
	MT_THZTREEBRANCH, // branch of said tree
	MT_ALARM,

	// Deep Sea Scenery
	MT_GARGOYLE, // Deep Sea Gargoyle
	MT_BIGGARGOYLE, // Deep Sea Gargoyle (Big)
	MT_SEAWEED, // DSZ Seaweed
	MT_WATERDRIP, // Dripping Water source
	MT_WATERDROP, // Water drop from dripping water
	MT_CORAL1, // Coral
	MT_CORAL2,
	MT_CORAL3,
	MT_CORAL4,
	MT_CORAL5,
	MT_BLUECRYSTAL, // Blue Crystal
	MT_KELP, // Kelp
	MT_ANIMALGAETOP, // Animated algae top
	MT_ANIMALGAESEG, // Animated algae segment
	MT_DSZSTALAGMITE, // Deep Sea 1 Stalagmite
	MT_DSZ2STALAGMITE, // Deep Sea 2 Stalagmite
	MT_LIGHTBEAM, // DSZ Light beam

	// Castle Eggman Scenery
	MT_CHAIN, // CEZ Chain
	MT_FLAME, // Flame (has corona)
	MT_FLAMEPARTICLE,
	MT_EGGSTATUE, // Eggman Statue
	MT_MACEPOINT, // Mace rotation point
	MT_CHAINMACEPOINT, // Combination of chains and maces point
	MT_SPRINGBALLPOINT, // Spring ball point
	MT_CHAINPOINT, // Mace chain
	MT_HIDDEN_SLING, // Spin mace chain (activatable)
	MT_FIREBARPOINT, // Firebar
	MT_CUSTOMMACEPOINT, // Custom mace
	MT_SMALLMACECHAIN, // Small Mace Chain
	MT_BIGMACECHAIN, // Big Mace Chain
	MT_SMALLMACE, // Small Mace
	MT_BIGMACE, // Big Mace
	MT_SMALLGRABCHAIN, // Small Grab Chain
	MT_BIGGRABCHAIN, // Big Grab Chain
	MT_YELLOWSPRINGBALL, // Yellow spring on a ball
	MT_REDSPRINGBALL, // Red spring on a ball
	MT_SMALLFIREBAR, // Small Firebar
	MT_BIGFIREBAR, // Big Firebar
	MT_CEZFLOWER, // Flower
	MT_CEZPOLE1, // Pole (with red banner)
	MT_CEZPOLE2, // Pole (with blue banner)
	MT_CEZBANNER1, // Banner (red)
	MT_CEZBANNER2, // Banner (blue)
	MT_PINETREE, // Pine Tree
	MT_CEZBUSH1, // Bush 1
	MT_CEZBUSH2, // Bush 2
	MT_CANDLE, // Candle
	MT_CANDLEPRICKET, // Candle pricket
	MT_FLAMEHOLDER, // Flame holder
	MT_FIRETORCH, // Fire torch
	MT_WAVINGFLAG1, // Waving flag (red)
	MT_WAVINGFLAG2, // Waving flag (blue)
	MT_WAVINGFLAGSEG1, // Waving flag segment (red)
	MT_WAVINGFLAGSEG2, // Waving flag segment (blue)
	MT_CRAWLASTATUE, // Crawla statue
	MT_FACESTABBERSTATUE, // Facestabber statue
	MT_SUSPICIOUSFACESTABBERSTATUE, // :eggthinking:
	MT_BRAMBLES, // Brambles

	// Arid Canyon Scenery
	MT_BIGTUMBLEWEED,
	MT_LITTLETUMBLEWEED,
	MT_CACTI1, // Tiny Red Flower Cactus
	MT_CACTI2, // Small Red Flower Cactus
	MT_CACTI3, // Tiny Blue Flower Cactus
	MT_CACTI4, // Small Blue Flower Cactus
	MT_CACTI5, // Prickly Pear
	MT_CACTI6, // Barrel Cactus
	MT_CACTI7, // Tall Barrel Cactus
	MT_CACTI8, // Armed Cactus
	MT_CACTI9, // Ball Cactus
	MT_CACTI10, // Tiny Cactus
	MT_CACTI11, // Small Cactus
	MT_CACTITINYSEG, // Tiny Cactus Segment
	MT_CACTISMALLSEG, // Small Cactus Segment
	MT_ARIDSIGN_CAUTION, // Caution Sign
	MT_ARIDSIGN_CACTI, // Cacti Sign
	MT_ARIDSIGN_SHARPTURN, // Sharp Turn Sign
	MT_OILLAMP,
	MT_TNTBARREL,
	MT_PROXIMITYTNT,
	MT_DUSTDEVIL,
	MT_DUSTLAYER,
	MT_ARIDDUST,
	MT_MINECART,
	MT_MINECARTSEG,
	MT_MINECARTSPAWNER,
	MT_MINECARTEND,
	MT_MINECARTENDSOLID,
	MT_MINECARTSIDEMARK,
	MT_MINECARTSPARK,
	MT_SALOONDOOR,
	MT_SALOONDOORCENTER,
	MT_TRAINCAMEOSPAWNER,
	MT_TRAINSEG,
	MT_TRAINDUSTSPAWNER,
	MT_TRAINSTEAMSPAWNER,
	MT_MINECARTSWITCHPOINT,

	// Red Volcano Scenery
	MT_FLAMEJET,
	MT_VERTICALFLAMEJET,
	MT_FLAMEJETFLAME,

	MT_FJSPINAXISA, // Counter-clockwise
	MT_FJSPINAXISB, // Clockwise

	MT_FLAMEJETFLAMEB, // Blade's flame

	MT_LAVAFALL,
	MT_LAVAFALL_LAVA,
	MT_LAVAFALLROCK,

	MT_ROLLOUTSPAWN,
	MT_ROLLOUTROCK,

	MT_BIGFERNLEAF,
	MT_BIGFERN,
	MT_JUNGLEPALM,
	MT_TORCHFLOWER,
	MT_WALLVINE_LONG,
	MT_WALLVINE_SHORT,

	// Dark City Scenery

	// Egg Rock Scenery

	// Azure Temple Scenery
	MT_GLAREGOYLE,
	MT_GLAREGOYLEUP,
	MT_GLAREGOYLEDOWN,
	MT_GLAREGOYLELONG,
	MT_TARGET, // AKA Red Crystal
	MT_GREENFLAME,
	MT_BLUEGARGOYLE,

	// Stalagmites
	MT_STALAGMITE0,
	MT_STALAGMITE1,
	MT_STALAGMITE2,
	MT_STALAGMITE3,
	MT_STALAGMITE4,
	MT_STALAGMITE5,
	MT_STALAGMITE6,
	MT_STALAGMITE7,
	MT_STALAGMITE8,
	MT_STALAGMITE9,

	// Christmas Scenery
	MT_XMASPOLE,
	MT_CANDYCANE,
	MT_SNOWMAN,    // normal
	MT_SNOWMANHAT, // with hat + scarf
	MT_LAMPPOST1,  // normal
	MT_LAMPPOST2,  // with snow
	MT_HANGSTAR,
	MT_MISTLETOE,
	// Xmas GFZ bushes
	MT_XMASBLUEBERRYBUSH,
	MT_XMASBERRYBUSH,
	MT_XMASBUSH,
	// FHZ
	MT_FHZICE1,
	MT_FHZICE2,
	MT_ROSY,
	MT_CDLHRT,

	// Halloween Scenery
	// Pumpkins
	MT_JACKO1,
	MT_JACKO2,
	MT_JACKO3,
	// Dr Seuss Trees
	MT_HHZTREE_TOP,
	MT_HHZTREE_PART,
	// Misc
	MT_HHZSHROOM,
	MT_HHZGRASS,
	MT_HHZTENTACLE1,
	MT_HHZTENTACLE2,
	MT_HHZSTALAGMITE_TALL,
	MT_HHZSTALAGMITE_SHORT,

	// Botanic Serenity scenery
	MT_BSZTALLFLOWER_RED,
	MT_BSZTALLFLOWER_PURPLE,
	MT_BSZTALLFLOWER_BLUE,
	MT_BSZTALLFLOWER_CYAN,
	MT_BSZTALLFLOWER_YELLOW,
	MT_BSZTALLFLOWER_ORANGE,
	MT_BSZFLOWER_RED,
	MT_BSZFLOWER_PURPLE,
	MT_BSZFLOWER_BLUE,
	MT_BSZFLOWER_CYAN,
	MT_BSZFLOWER_YELLOW,
	MT_BSZFLOWER_ORANGE,
	MT_BSZSHORTFLOWER_RED,
	MT_BSZSHORTFLOWER_PURPLE,
	MT_BSZSHORTFLOWER_BLUE,
	MT_BSZSHORTFLOWER_CYAN,
	MT_BSZSHORTFLOWER_YELLOW,
	MT_BSZSHORTFLOWER_ORANGE,
	MT_BSZTULIP_RED,
	MT_BSZTULIP_PURPLE,
	MT_BSZTULIP_BLUE,
	MT_BSZTULIP_CYAN,
	MT_BSZTULIP_YELLOW,
	MT_BSZTULIP_ORANGE,
	MT_BSZCLUSTER_RED,
	MT_BSZCLUSTER_PURPLE,
	MT_BSZCLUSTER_BLUE,
	MT_BSZCLUSTER_CYAN,
	MT_BSZCLUSTER_YELLOW,
	MT_BSZCLUSTER_ORANGE,
	MT_BSZBUSH_RED,
	MT_BSZBUSH_PURPLE,
	MT_BSZBUSH_BLUE,
	MT_BSZBUSH_CYAN,
	MT_BSZBUSH_YELLOW,
	MT_BSZBUSH_ORANGE,
	MT_BSZVINE_RED,
	MT_BSZVINE_PURPLE,
	MT_BSZVINE_BLUE,
	MT_BSZVINE_CYAN,
	MT_BSZVINE_YELLOW,
	MT_BSZVINE_ORANGE,
	MT_BSZSHRUB,
	MT_BSZCLOVER,
	MT_BIG_PALMTREE_TRUNK,
	MT_BIG_PALMTREE_TOP,
	MT_PALMTREE_TRUNK,
	MT_PALMTREE_TOP,

	// Misc scenery
	MT_DBALL,
	MT_EGGSTATUE2,

	// Powerup Indicators
	MT_ELEMENTAL_ORB, // Elemental shield mobj
	MT_ATTRACT_ORB, // Attract shield mobj
	MT_FORCE_ORB, // Force shield mobj
	MT_ARMAGEDDON_ORB, // Armageddon shield mobj
	MT_WHIRLWIND_ORB, // Whirlwind shield mobj
	MT_PITY_ORB, // Pity shield mobj
	MT_FLAMEAURA_ORB, // Flame shield mobj
	MT_BUBBLEWRAP_ORB, // Bubble shield mobj
	MT_THUNDERCOIN_ORB, // Thunder shield mobj
	MT_THUNDERCOIN_SPARK, // Thunder spark
	MT_IVSP, // Invincibility sparkles
	MT_SUPERSPARK, // Super Sonic Spark

	// Flickies
	MT_FLICKY_01, // Bluebird
	MT_FLICKY_01_CENTER,
	MT_FLICKY_02, // Rabbit
	MT_FLICKY_02_CENTER,
	MT_FLICKY_03, // Chicken
	MT_FLICKY_03_CENTER,
	MT_FLICKY_04, // Seal
	MT_FLICKY_04_CENTER,
	MT_FLICKY_05, // Pig
	MT_FLICKY_05_CENTER,
	MT_FLICKY_06, // Chipmunk
	MT_FLICKY_06_CENTER,
	MT_FLICKY_07, // Penguin
	MT_FLICKY_07_CENTER,
	MT_FLICKY_08, // Fish
	MT_FLICKY_08_CENTER,
	MT_FLICKY_09, // Ram
	MT_FLICKY_09_CENTER,
	MT_FLICKY_10, // Puffin
	MT_FLICKY_10_CENTER,
	MT_FLICKY_11, // Cow
	MT_FLICKY_11_CENTER,
	MT_FLICKY_12, // Rat
	MT_FLICKY_12_CENTER,
	MT_FLICKY_13, // Bear
	MT_FLICKY_13_CENTER,
	MT_FLICKY_14, // Dove
	MT_FLICKY_14_CENTER,
	MT_FLICKY_15, // Cat
	MT_FLICKY_15_CENTER,
	MT_FLICKY_16, // Canary
	MT_FLICKY_16_CENTER,
	MT_SECRETFLICKY_01, // Spider
	MT_SECRETFLICKY_01_CENTER,
	MT_SECRETFLICKY_02, // Bat
	MT_SECRETFLICKY_02_CENTER,
	MT_SEED,

	// Environmental Effects
	MT_RAIN, // Rain
	MT_SNOWFLAKE, // Snowflake
	MT_BLIZZARDSNOW, // Blizzard Snowball
	MT_SPLISH, // Water splish!
	MT_LAVASPLISH, // Lava splish!
	MT_SMOKE,
	MT_SMALLBUBBLE, // small bubble
	MT_MEDIUMBUBBLE, // medium bubble
	MT_EXTRALARGEBUBBLE, // extra large bubble
	MT_WATERZAP,
	MT_SPINDUST, // Spindash dust
	MT_TFOG,
	MT_PARTICLE,
	MT_PARTICLEGEN, // For fans, etc.

	// Game Indicators
	MT_SCORE, // score logo
	MT_DROWNNUMBERS, // Drowning Timer
	MT_GOTEMERALD, // Chaos Emerald (intangible)
	MT_LOCKON, // Target
	MT_LOCKONINF, // In-level Target
	MT_TAG, // Tag Sign
	MT_GOTFLAG, // Got Flag sign
	MT_FINISHFLAG, // Finish flag

	// Ambient Sounds
	MT_AMBIENT,

	MT_CORK,
	MT_LHRT,

	// Ring Weapons
	MT_REDRING,
	MT_BOUNCERING,
	MT_RAILRING,
	MT_INFINITYRING,
	MT_AUTOMATICRING,
	MT_EXPLOSIONRING,
	MT_SCATTERRING,
	MT_GRENADERING,

	MT_BOUNCEPICKUP,
	MT_RAILPICKUP,
	MT_AUTOPICKUP,
	MT_EXPLODEPICKUP,
	MT_SCATTERPICKUP,
	MT_GRENADEPICKUP,

	MT_THROWNBOUNCE,
	MT_THROWNINFINITY,
	MT_THROWNAUTOMATIC,
	MT_THROWNSCATTER,
	MT_THROWNEXPLOSION,
	MT_THROWNGRENADE,

	// Mario-specific stuff
	MT_COIN,
	MT_FLINGCOIN,
	MT_GOOMBA,
	MT_BLUEGOOMBA,
	MT_FIREFLOWER,
	MT_FIREBALL,
	MT_FIREBALLTRAIL,
	MT_SHELL,
	MT_PUMA,
	MT_PUMATRAIL,
	MT_HAMMER,
	MT_KOOPA,
	MT_KOOPAFLAME,
	MT_AXE,
	MT_MARIOBUSH1,
	MT_MARIOBUSH2,
	MT_TOAD,

	// NiGHTS Stuff
	MT_AXIS,
	MT_AXISTRANSFER,
	MT_AXISTRANSFERLINE,
	MT_NIGHTSDRONE,
	MT_NIGHTSDRONE_MAN,
	MT_NIGHTSDRONE_SPARKLING,
	MT_NIGHTSDRONE_GOAL,
	MT_NIGHTSPARKLE,
	MT_NIGHTSLOOPHELPER,
	MT_NIGHTSBUMPER, // NiGHTS Bumper
	MT_HOOP,
	MT_HOOPCOLLIDE, // Collision detection for NiGHTS hoops
	MT_HOOPCENTER, // Center of a hoop
	MT_NIGHTSCORE,
	MT_NIGHTSCHIP, // NiGHTS Chip
	MT_FLINGNIGHTSCHIP, // Lost NiGHTS Chip
	MT_NIGHTSSTAR, // NiGHTS Star
	MT_FLINGNIGHTSSTAR, // Lost NiGHTS Star
	MT_NIGHTSSUPERLOOP,
	MT_NIGHTSDRILLREFILL,
	MT_NIGHTSHELPER,
	MT_NIGHTSEXTRATIME,
	MT_NIGHTSLINKFREEZE,
	MT_EGGCAPSULE,
	MT_IDEYAANCHOR,
	MT_NIGHTOPIANHELPER, // the actual helper object that orbits you
	MT_PIAN, // decorative singing friend
	MT_SHLEEP, // almost-decorative sleeping enemy

	// Secret badniks and hazards, shhhh
	MT_PENGUINATOR,
	MT_POPHAT,
	MT_POPSHOT,
	MT_POPSHOT_TRAIL,

	MT_HIVEELEMENTAL,
	MT_BUMBLEBORE,

	MT_BUGGLE,

	MT_SMASHINGSPIKEBALL,
	MT_CACOLANTERN,
	MT_CACOSHARD,
	MT_CACOFIRE,
	MT_SPINBOBERT,
	MT_SPINBOBERT_FIRE1,
	MT_SPINBOBERT_FIRE2,
	MT_HANGSTER,

	// Utility Objects
	MT_TELEPORTMAN,
	MT_ALTVIEWMAN,
	MT_CRUMBLEOBJ, // Sound generator for crumbling platform
	MT_TUBEWAYPOINT,
	MT_PUSH,
	MT_GHOST,
	MT_FAKESHADOW,
	MT_OVERLAY,
	MT_ANGLEMAN,
	MT_POLYANCHOR,
	MT_POLYSPAWN,
	MT_MINIMAPBOUND,

	// Skybox objects
	MT_SKYBOX,

	// Debris
	MT_SPARK, //spark, only used for debugging, actually
	MT_EXPLODE, // Robot Explosion
	MT_UWEXPLODE, // Underwater Explosion
	MT_DUST,
	MT_ROCKSPAWNER,
	MT_FALLINGROCK,
	MT_ROCKCRUMBLE1,
	MT_ROCKCRUMBLE2,
	MT_ROCKCRUMBLE3,
	MT_ROCKCRUMBLE4,
	MT_ROCKCRUMBLE5,
	MT_ROCKCRUMBLE6,
	MT_ROCKCRUMBLE7,
	MT_ROCKCRUMBLE8,
	MT_ROCKCRUMBLE9,
	MT_ROCKCRUMBLE10,
	MT_ROCKCRUMBLE11,
	MT_ROCKCRUMBLE12,
	MT_ROCKCRUMBLE13,
	MT_ROCKCRUMBLE14,
	MT_ROCKCRUMBLE15,
	MT_ROCKCRUMBLE16,

	// Level debris
	MT_GFZDEBRIS,
	MT_BRICKDEBRIS,
	MT_WOODDEBRIS,
	MT_REDBRICKDEBRIS, // for CEZ3
	MT_BLUEBRICKDEBRIS, // for CEZ3
	MT_YELLOWBRICKDEBRIS, // for CEZ3

	// SRB2kart
	MT_RANDOMITEM,
	MT_SPHEREBOX,
	MT_RANDOMITEMPOP,
	MT_FLOATINGITEM,
	MT_ITEMCAPSULE,
	MT_ITEMCAPSULE_PART,
	MT_MONITOR,
	MT_MONITOR_PART,
	MT_MONITOR_SHARD,
	MT_MAGICIANBOX,
	MT_WAVEDASH,

	MT_INSTAWHIP,
	MT_INSTAWHIP_RECHARGE,
	MT_INSTAWHIP_REJECT,
	MT_BLOCKRING,
	MT_BLOCKBODY,

	MT_CHARGEAURA,
	MT_CHARGEFALL,
	MT_CHARGEFLICKER,
	MT_CHARGESPARK,
	MT_CHARGERELEASE,
	MT_CHARGEEXTRA,

	MT_SERVANTHAND,

	MT_HORNCODE,

	MT_SIGNSPARKLE,

	MT_FASTLINE,
	MT_FASTDUST,
	MT_DRIFTEXPLODE,
	MT_DRIFTCLIP,
	MT_DRIFTCLIPSPARK,
	MT_BOOSTFLAME,
	MT_BOOSTSMOKE,
	MT_SNEAKERTRAIL,
	MT_AIZDRIFTSTRAT,
	MT_SPARKLETRAIL,
	MT_INVULNFLASH,
	MT_WIPEOUTTRAIL,
	MT_DRIFTSPARK,
	MT_BRAKEDRIFT,
	MT_BRAKEDUST,
	MT_DRIFTDUST,
	MT_ITEM_DEBRIS,
	MT_ITEM_DEBRIS_CLOUD_SPAWNER,
	MT_DRIFTELECTRICITY,
	MT_DRIFTELECTRICSPARK,
	MT_JANKSPARK,
	MT_HITLAG,

	MT_ROCKETSNEAKER,

	MT_EGGMANITEM, // Eggman items
	MT_EGGMANITEM_SHIELD,

	MT_BANANA, // Banana Stuff
	MT_BANANA_SHIELD,
	MT_BANANA_SPARK,

	MT_ORBINAUT, // Orbinaut stuff
	MT_ORBINAUT_SHIELD,

	MT_JAWZ, // Jawz stuff
	MT_JAWZ_SHIELD,

	MT_PLAYERRETICULE, // Jawz reticule

	MT_SSMINE, // Mine stuff
	MT_SSMINE_SHIELD,

	MT_SMOLDERING, // New explosion
	MT_BOOMEXPLODE,
	MT_BOOMPARTICLE,

	MT_LANDMINE, // Land Mine

	MT_DROPTARGET, // Drop Target
	MT_DROPTARGET_SHIELD,
	MT_DROPTARGET_MORPH,

	MT_BALLHOG, // Ballhog
	MT_BALLHOGBOOM,

	MT_SPB, // SPB stuff
	MT_SPBEXPLOSION,
	MT_MANTARING, // Juicebox for SPB

	MT_LIGHTNINGSHIELD, // Shields
	MT_BUBBLESHIELD,
	MT_FLAMESHIELD,
	MT_FLAMESHIELDUNDERLAY,
	MT_FLAMESHIELDPAPER,
	MT_BUBBLESHIELDTRAP,
	MT_GARDENTOP,
	MT_GARDENTOPSPARK,
	MT_GARDENTOPARROW,

	MT_HYUDORO,
	MT_HYUDORO_CENTER,

	MT_GROW_PARTICLE,

	MT_SHRINK_POHBEE,
	MT_SHRINK_GUN,
	MT_SHRINK_CHAIN,
	MT_SHRINK_LASER,
	MT_SHRINK_PARTICLE,

	MT_SINK, // Kitchen Sink Stuff
	MT_SINK_SHIELD,
	MT_SINKTRAIL,

	MT_GACHABOM,
	MT_GACHABOM_REBOUND,

	MT_DUELBOMB, // Duel mode bombs

	MT_BATTLEBUMPER, // Battle Mode bumpers
	MT_BATTLEBUMPER_DEBRIS,
	MT_BATTLEBUMPER_BLAST,

	MT_TRIPWIREBOOST,

	MT_SMOOTHLANDING,
	MT_TRICKINDICATOR,
	MT_SIDETRICK,
	MT_FORWARDTRICK,

	MT_TIREGRABBER,
	MT_RINGSHOOTER,
	MT_RINGSHOOTER_PART,
	MT_RINGSHOOTER_SCREEN,

	MT_DEZLASER,

	MT_WAYPOINT,
	MT_WAYPOINT_RISER,
	MT_WAYPOINT_ANCHOR,

	MT_BOTHINT,

	MT_RANDOMAUDIENCE,

	MT_FLAYM,
	MT_DEVIL,
	MT_ANGEL,
	MT_PALMTREE,
	MT_FLAG,
	MT_HEDGEHOG,
	MT_BUSH1,
	MT_TWEE,
	MT_HYDRANT,

	MT_BIGPUMA,
	MT_APPLE,

	MT_BIGRING,

	MT_SNES_DONUTBUSH1,
	MT_SNES_DONUTBUSH2,
	MT_SNES_DONUTBUSH3,

	MT_GBA_BOO,

	MT_BUZZBOMBER,
	MT_CHOMPER,
	MT_PALMTREE2,
	MT_PURPLEFLOWER1,
	MT_PURPLEFLOWER2,
	MT_YELLOWFLOWER1,
	MT_YELLOWFLOWER2,
	MT_PLANT2,
	MT_PLANT3,
	MT_PLANT4,

	MT_SKULL,
	MT_PHANTREE,
	MT_FLYINGGARG,
	MT_LAMPPOST,
	MT_MOSSYTREE,

	MT_ARKARROW, // Ark Arrows

	MT_BUMP,

	MT_FLINGENERGY,

	MT_ITEMCLASH,

	MT_FIREDITEM,

	MT_INSTASHIELDA,
	MT_INSTASHIELDB,

	MT_POWERCLASH, // Grow/Invinc clash VFX
	MT_GUARDBREAK,

	MT_PLAYERARROW,
	MT_PLAYERWANTED,

	MT_KARMAHITBOX,
	MT_KARMAWHEEL,

	MT_BATTLEPOINT,

	MT_FZEROBOOM,

	// Dash Rings
	MT_DASHRING,
	MT_RAINBOWDASHRING,

	// Sneaker Panels
	MT_SNEAKERPANEL,
	MT_SNEAKERPANELSPAWNER,

	// Various plants
	MT_SONICBUSH,

	// Marble Zone
	MT_MARBLEFLAMEPARTICLE,
	MT_MARBLETORCH,
	MT_MARBLELIGHT,
	MT_MARBLEBURNER,

	// CD Special Stage
	MT_CDUFO,

	// Rusty Rig
	MT_RUSTYLAMP_ORANGE,
	MT_RUSTYCHAIN,

	// Smokin' & Vapin' (Don't try this at home, kids!)
	MT_PETSMOKER,
	MT_PETSMOKE,
	MT_VVVAPE,

	// Hill Top Zone
	MT_HTZTREE,
	MT_HTZBUSH,

	// Ports of gardens
	MT_SGVINE1,
	MT_SGVINE2,
	MT_SGVINE3,
	MT_PGTREE,
	MT_PGFLOWER1,
	MT_PGFLOWER2,
	MT_PGFLOWER3,
	MT_PGBUSH,
	MT_DHPILLAR,

	// Midnight Channel stuff:
	MT_SPOTLIGHT,		// Spotlight Object
	MT_RANDOMSHADOW,	// Random static Shadows.
	MT_ROAMINGSHADOW,	// Roaming Shadows.
	MT_MAYONAKAARROW,	// Arrow static signs for Mayonaka

	// Mementos stuff
	MT_REAPERWAYPOINT,
	MT_REAPER,
	MT_MEMENTOSTP,
	MT_MEMENTOSPARTICLE,

	MT_JACKINTHEBOX,

	// Color Drive:
	MT_CDMOON,
	MT_CDBUSH,
	MT_CDTREEA,
	MT_CDTREEB,

	// Daytona Speedway
	MT_DAYTONAPINETREE,
	MT_DAYTONAPINETREE_SIDE,

	// Egg Zeppelin
	MT_EZZPROPELLER,
	MT_EZZPROPELLER_BLADE,

	// Desert Palace
	MT_DP_PALMTREE,

	// Aurora Atoll
	MT_AAZTREE_HELPER,
	MT_AAZTREE_SEG,
	MT_AAZTREE_COCONUT,
	MT_AAZTREE_LEAF,

	// Barren Badlands
	MT_BBZDUST,
	MT_FROGGER,
	MT_FROGTONGUE,
	MT_FROGTONGUE_JOINT,
	MT_ROBRA,
	MT_ROBRA_HEAD,
	MT_ROBRA_JOINT,
	MT_BLUEROBRA,
	MT_BLUEROBRA_HEAD,
	MT_BLUEROBRA_JOINT,

	// Eerie Grove
	MT_EERIEFOG,
	MT_EERIEFOGGEN,

	// Chaos Chute
	MT_SPECIALSTAGEARCH,
	MT_SPECIALSTAGEBOMB,

	// Hanagumi Hall
	MT_HANAGUMIHALL_STEAM,
	MT_HANAGUMIHALL_NPC,

	// SMK ports
	MT_SMK_PIPE,
	MT_SMK_MOLESPAWNER,
	MT_SMK_MOLE,
	MT_SMK_THWOMP,
	MT_SMK_SNOWBALL,
	MT_SMK_ICEBLOCK,
	MT_SMK_ICEBLOCK_SIDE,
	MT_SMK_ICEBLOCK_DEBRIS,

	// Ezo's maps
	MT_BLUEFIRE,
	MT_GREENFIRE,
	MT_REGALCHEST,
	MT_CHIMERASTATUE,
	MT_DRAGONSTATUE,
	MT_LIZARDMANSTATUE,
	MT_PEGASUSSTATUE,
	MT_ZELDAFIRE,
	MT_GANBARETHING,
	MT_GANBAREDUCK,
	MT_GANBARETREE,
	MT_MONOKUMA,
	MT_REDZELDAFIRE,
	MT_BOWLINGPIN,
	MT_MERRYAMBIENCE,
	MT_TWINKLECARTAMBIENCE,
	MT_EXPLODINGBARREL,
	MT_MERRYHORSE,
	MT_ARIDTOAD,
	MT_BLUEFRUIT,
	MT_ORANGEFRUIT,
	MT_REDFRUIT,
	MT_PINKFRUIT,
	MT_ADVENTURESPIKEA,
	MT_ADVENTURESPIKEB,
	MT_ADVENTURESPIKEC,
	MT_BOOSTPROMPT,
	MT_BOOSTOFF,
	MT_BOOSTON,
	MT_LIZARDMAN,
	MT_LIONMAN,

	MT_SUNBEAMPALM_STEM,
	MT_SUNBEAMPALM_LEAF,

	MT_KARMAFIREWORK,
	MT_RINGSPARKS,
	MT_GAINAX,
	MT_DRAFTDUST,
	MT_SPBDUST,
	MT_TIREGREASE,

	MT_OVERTIME_PARTICLE,
	MT_OVERTIME_CENTER,

	MT_BATTLECAPSULE,
	MT_BATTLECAPSULE_PIECE,

	MT_FOLLOWER,
	MT_FOLLOWERBUBBLE_FRONT,
	MT_FOLLOWERBUBBLE_BACK,

	MT_WATERTRAIL,
	MT_WATERTRAILUNDERLAY,

	MT_SPINDASHDUST,
	MT_SPINDASHWIND,
	MT_SOFTLANDING,
	MT_DOWNLINE,
	MT_HOLDBUBBLE,

	MT_PAPERITEMSPOT,

	MT_BEAMPOINT,

	MT_BROLY,

	MT_SPECIAL_UFO,
	MT_SPECIAL_UFO_PIECE,

	MT_LOOPENDPOINT,
	MT_LOOPCENTERPOINT,

	MT_SUPER_FLICKY,
	MT_SUPER_FLICKY_CONTROLLER,

	MT_BATTLEUFO_SPAWNER,
	MT_BATTLEUFO,
	MT_BATTLEUFO_LEG,
	MT_BATTLEUFO_BEAM,

	MT_POWERUP_AURA,

	MT_CHECKPOINT_END,
	MT_SCRIPT_THING,

	MT_RIDEROID,
	MT_RIDEROIDNODE,

	MT_LSZ_BUNGEE,
	MT_LSZ_EGGBALLSPAWNER,
	MT_LSZ_EGGBALL,

	MT_DLZ_HOVER,
	MT_DLZ_ROCKET,
	MT_DLZ_RINGVACCUM,
	MT_DLZ_SUCKEDRING,

	MT_WATERPALACETURBINE,
	MT_WATERPALACEBUBBLE,
	MT_WATERPALACEFOUNTAIN,
	MT_KURAGEN,
	MT_KURAGENBOMB,

	MT_BALLSWITCH_BALL,
	MT_BALLSWITCH_PAD,

	MT_BOSSARENACENTER,
	MT_SPIKEDTARGET,

	MT_BLENDEYE_MAIN,
	MT_BLENDEYE_EYE,
	MT_BLENDEYE_GLASS,
	MT_BLENDEYE_SHIELD,
	MT_BLENDEYE_EGGBEATER,
	MT_BLENDEYE_GENERATOR,
	MT_BLENDEYE_PUYO,
	MT_BLENDEYE_PUYO_DUST,
	MT_BLENDEYE_PUYO_DUST_COFFEE,

	MT_MEGABARRIER,

	MT_SEASAW_VISUAL,
	MT_DLZ_SEASAW_SPAWN,
	MT_DLZ_SEASAW_HITBOX,
	MT_GPZ_SEASAW_SPAWN,
	MT_GPZ_SEASAW_HITBOX,

	MT_GPZ_TREETHING_B,
	MT_GPZ_TREETHING_M,
	MT_GPZ_TREETHING_S,

	MT_GGZFREEZETHRUSTER,
	MT_GGZICEDUST,
	MT_GGZICECUBE,
	MT_GGZICESHATTER,
	MT_SIDEWAYSFREEZETHRUSTER,
	MT_THRUSTERPART,

	MT_IVOBALL,
	MT_PATROLIVOBALL,
	MT_AIRIVOBALL,

	MT_BOX_SIDE,
	MT_BOX_DEBRIS,
	MT_SA2_CRATE,
	MT_ICECAPBLOCK,

	MT_SPEAR,
	MT_SPEARVISUAL,
	MT_BSZLAMP_S,
	MT_BSZLAMP_M,
	MT_BSZLAMP_L,
	MT_BSZSLAMP,
	MT_BSZSLCHA,

	MT_BETA_EMITTER,
	MT_BETA_PARTICLE_PHYSICAL,
	MT_BETA_PARTICLE_VISUAL,
	MT_BETA_PARTICLE_EXPLOSION,

	MT_FIRSTFREESLOT,
	MT_LASTFREESLOT = MT_FIRSTFREESLOT + NUMMOBJFREESLOTS - 1,
	NUMMOBJTYPES
} mobjtype_t;

struct mobjinfo_t
{
	INT32 doomednum;
	statenum_t spawnstate;
	INT32 spawnhealth;
	statenum_t seestate;
	sfxenum_t seesound;
	INT32 reactiontime;
	sfxenum_t attacksound;
	statenum_t painstate;
	INT32 painchance;
	sfxenum_t painsound;
	statenum_t meleestate;
	statenum_t missilestate;
	statenum_t deathstate;
	statenum_t xdeathstate;
	sfxenum_t deathsound;
	fixed_t speed;
	fixed_t radius;
	fixed_t height;
	INT32 dispoffset;
	INT32 mass;
	INT32 damage;
	sfxenum_t activesound;
	UINT32 flags;
	statenum_t raisestate;
};

extern mobjinfo_t mobjinfo[NUMMOBJTYPES];

void P_PatchInfoTables(void);

void P_BackupTables(void);

void P_ResetData(INT32 flags);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
