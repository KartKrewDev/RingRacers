// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  deh_tables.c
/// \brief Define DeHackEd tables.

#include "doomdef.h" // Constants
#include "s_sound.h" // Sound constants
#include "info.h" // Mobj, state, sprite, etc constants
#include "k_menu.h" // Menu constants
#include "y_inter.h" // Intermission constants
#include "p_local.h" // some more constants
#include "r_draw.h" // Colormap constants
#include "lua_script.h" // Lua stuff
#include "m_cond.h" // Emblem constants
#include "v_video.h" // video flags (for lua)
#include "g_state.h" // gamestate_t (for lua)
#include "r_data.h" // patchalphastyle_t
#include "k_boss.h" // spottype_t (for lua)

#include "deh_tables.h"

char *FREE_STATES[NUMSTATEFREESLOTS];
char *FREE_MOBJS[NUMMOBJFREESLOTS];
char *FREE_SKINCOLORS[NUMCOLORFREESLOTS];
UINT8 used_spr[(NUMSPRITEFREESLOTS / 8) + 1]; // Bitwise flag for sprite freeslot in use! I would use ceil() here if I could, but it only saves 1 byte of memory anyway.

// IMPORTANT!
// DO NOT FORGET TO SYNC THIS LIST WITH THE ACTIONNUM ENUM IN INFO.H
actionpointer_t actionpointers[] =
{
	{{A_Explode},                "A_EXPLODE"},
	{{A_Pain},                   "A_PAIN"},
	{{A_Fall},                   "A_FALL"},
	{{A_Look},                   "A_LOOK"},
	{{A_Chase},                  "A_CHASE"},
	{{A_FaceStabChase},          "A_FACESTABCHASE"},
	{{A_FaceStabRev},            "A_FACESTABREV"},
	{{A_FaceStabHurl},           "A_FACESTABHURL"},
	{{A_FaceStabMiss},           "A_FACESTABMISS"},
	{{A_StatueBurst},            "A_STATUEBURST"},
	{{A_FaceTarget},             "A_FACETARGET"},
	{{A_FaceTracer},             "A_FACETRACER"},
	{{A_Scream},                 "A_SCREAM"},
	{{A_BossDeath},              "A_BOSSDEATH"},
	{{A_RingBox},                "A_RINGBOX"},
	{{A_BunnyHop},               "A_BUNNYHOP"},
	{{A_BubbleSpawn},            "A_BUBBLESPAWN"},
	{{A_FanBubbleSpawn},         "A_FANBUBBLESPAWN"},
	{{A_BubbleRise},             "A_BUBBLERISE"},
	{{A_BubbleCheck},            "A_BUBBLECHECK"},
	{{A_AwardScore},             "A_AWARDSCORE"},
	{{A_ScoreRise},              "A_SCORERISE"},
	{{A_AttractChase},           "A_ATTRACTCHASE"},
	{{A_DropMine},               "A_DROPMINE"},
	{{A_FishJump},               "A_FISHJUMP"},
	{{A_SetSolidSteam},          "A_SETSOLIDSTEAM"},
	{{A_UnsetSolidSteam},        "A_UNSETSOLIDSTEAM"},
	{{A_OverlayThink},           "A_OVERLAYTHINK"},
	{{A_JetChase},               "A_JETCHASE"},
	{{A_JetbThink},              "A_JETBTHINK"},
	{{A_JetgThink},              "A_JETGTHINK"},
	{{A_JetgShoot},              "A_JETGSHOOT"},
	{{A_ShootBullet},            "A_SHOOTBULLET"},
	{{A_MinusDigging},           "A_MINUSDIGGING"},
	{{A_MinusPopup},             "A_MINUSPOPUP"},
	{{A_MinusCheck},             "A_MINUSCHECK"},
	{{A_ChickenCheck},           "A_CHICKENCHECK"},
	{{A_MouseThink},             "A_MOUSETHINK"},
	{{A_DetonChase},             "A_DETONCHASE"},
	{{A_CapeChase},              "A_CAPECHASE"},
	{{A_RotateSpikeBall},        "A_ROTATESPIKEBALL"},
	{{A_SlingAppear},            "A_SLINGAPPEAR"},
	{{A_UnidusBall},             "A_UNIDUSBALL"},
	{{A_RockSpawn},              "A_ROCKSPAWN"},
	{{A_SetFuse},                "A_SETFUSE"},
	{{A_CrawlaCommanderThink},   "A_CRAWLACOMMANDERTHINK"},
	{{A_SmokeTrailer},           "A_SMOKETRAILER"},
	{{A_RingExplode},            "A_RINGEXPLODE"},
	{{A_OldRingExplode},         "A_OLDRINGEXPLODE"},
	{{A_MixUp},                  "A_MIXUP"},
	{{A_Boss1Chase},             "A_BOSS1CHASE"},
	{{A_FocusTarget},            "A_FOCUSTARGET"},
	{{A_Boss2Chase},             "A_BOSS2CHASE"},
	{{A_Boss2Pogo},              "A_BOSS2POGO"},
	{{A_BossZoom},               "A_BOSSZOOM"},
	{{A_BossScream},             "A_BOSSSCREAM"},
	{{A_Boss2TakeDamage},        "A_BOSS2TAKEDAMAGE"},
	{{A_GoopSplat},              "A_GOOPSPLAT"},
	{{A_Boss2PogoSFX},           "A_BOSS2POGOSFX"},
	{{A_Boss2PogoTarget},        "A_BOSS2POGOTARGET"},
	{{A_EggmanBox},              "A_EGGMANBOX"},
	{{A_TurretFire},             "A_TURRETFIRE"},
	{{A_SuperTurretFire},        "A_SUPERTURRETFIRE"},
	{{A_TurretStop},             "A_TURRETSTOP"},
	{{A_JetJawRoam},             "A_JETJAWROAM"},
	{{A_JetJawChomp},            "A_JETJAWCHOMP"},
	{{A_PointyThink},            "A_POINTYTHINK"},
	{{A_CheckBuddy},             "A_CHECKBUDDY"},
	{{A_HoodFire},               "A_HOODFIRE"},
	{{A_HoodThink},              "A_HOODTHINK"},
	{{A_HoodFall},               "A_HOODFALL"},
	{{A_ArrowBonks},             "A_ARROWBONKS"},
	{{A_SnailerThink},           "A_SNAILERTHINK"},
	{{A_SharpChase},             "A_SHARPCHASE"},
	{{A_SharpSpin},              "A_SHARPSPIN"},
	{{A_SharpDecel},             "A_SHARPDECEL"},
	{{A_CrushstaceanWalk},       "A_CRUSHSTACEANWALK"},
	{{A_CrushstaceanPunch},      "A_CRUSHSTACEANPUNCH"},
	{{A_CrushclawAim},           "A_CRUSHCLAWAIM"},
	{{A_CrushclawLaunch},        "A_CRUSHCLAWLAUNCH"},
	{{A_VultureVtol},            "A_VULTUREVTOL"},
	{{A_VultureCheck},           "A_VULTURECHECK"},
	{{A_VultureHover},           "A_VULTUREHOVER"},
	{{A_VultureBlast},           "A_VULTUREBLAST"},
	{{A_VultureFly},             "A_VULTUREFLY"},
	{{A_SkimChase},              "A_SKIMCHASE"},
	{{A_SkullAttack},            "A_SKULLATTACK"},
	{{A_LobShot},                "A_LOBSHOT"},
	{{A_FireShot},               "A_FIRESHOT"},
	{{A_SuperFireShot},          "A_SUPERFIRESHOT"},
	{{A_BossFireShot},           "A_BOSSFIRESHOT"},
	{{A_Boss7FireMissiles},      "A_BOSS7FIREMISSILES"},
	{{A_Boss4Reverse},           "A_BOSS4REVERSE"},
	{{A_Boss4SpeedUp},           "A_BOSS4SPEEDUP"},
	{{A_Boss4Raise},             "A_BOSS4RAISE"},
	{{A_SparkFollow},            "A_SPARKFOLLOW"},
	{{A_BuzzFly},                "A_BUZZFLY"},
	{{A_GuardChase},             "A_GUARDCHASE"},
	{{A_EggShield},              "A_EGGSHIELD"},
	{{A_SetReactionTime},        "A_SETREACTIONTIME"},
	{{A_Boss3TakeDamage},        "A_BOSS3TAKEDAMAGE"},
	{{A_Boss3Path},              "A_BOSS3PATH"},
	{{A_Boss3ShockThink},        "A_BOSS3SHOCKTHINK"},
	{{A_LinedefExecute},         "A_LINEDEFEXECUTE"},
	{{A_LinedefExecuteFromArg},  "A_LINEDEFEXECUTEFROMARG"},
	{{A_PlaySeeSound},           "A_PLAYSEESOUND"},
	{{A_PlayAttackSound},        "A_PLAYATTACKSOUND"},
	{{A_PlayActiveSound},        "A_PLAYACTIVESOUND"},
	{{A_SpawnObjectAbsolute},    "A_SPAWNOBJECTABSOLUTE"},
	{{A_SpawnObjectRelative},    "A_SPAWNOBJECTRELATIVE"},
	{{A_ChangeAngleRelative},    "A_CHANGEANGLERELATIVE"},
	{{A_ChangeAngleAbsolute},    "A_CHANGEANGLEABSOLUTE"},
	{{A_RollAngle},              "A_ROLLANGLE"},
	{{A_ChangeRollAngleRelative},"A_CHANGEROLLANGLERELATIVE"},
	{{A_ChangeRollAngleAbsolute},"A_CHANGEROLLANGLEABSOLUTE"},
	{{A_PlaySound},              "A_PLAYSOUND"},
	{{A_FindTarget},             "A_FINDTARGET"},
	{{A_FindTracer},             "A_FINDTRACER"},
	{{A_SetTics},                "A_SETTICS"},
	{{A_SetRandomTics},          "A_SETRANDOMTICS"},
	{{A_ChangeColorRelative},    "A_CHANGECOLORRELATIVE"},
	{{A_ChangeColorAbsolute},    "A_CHANGECOLORABSOLUTE"},
	{{A_Dye},                    "A_DYE"},
	{{A_MoveRelative},           "A_MOVERELATIVE"},
	{{A_MoveAbsolute},           "A_MOVEABSOLUTE"},
	{{A_Thrust},                 "A_THRUST"},
	{{A_ZThrust},                "A_ZTHRUST"},
	{{A_SetTargetsTarget},       "A_SETTARGETSTARGET"},
	{{A_SetObjectFlags},         "A_SETOBJECTFLAGS"},
	{{A_SetObjectFlags2},        "A_SETOBJECTFLAGS2"},
	{{A_RandomState},            "A_RANDOMSTATE"},
	{{A_RandomStateRange},       "A_RANDOMSTATERANGE"},
	{{A_StateRangeByAngle},      "A_STATERANGEBYANGLE"},
	{{A_StateRangeByParameter},  "A_STATERANGEBYPARAMETER"},
	{{A_DualAction},             "A_DUALACTION"},
	{{A_RemoteAction},           "A_REMOTEACTION"},
	{{A_ToggleFlameJet},         "A_TOGGLEFLAMEJET"},
	{{A_OrbitNights},            "A_ORBITNIGHTS"},
	{{A_GhostMe},                "A_GHOSTME"},
	{{A_SetObjectState},         "A_SETOBJECTSTATE"},
	{{A_SetObjectTypeState},     "A_SETOBJECTTYPESTATE"},
	{{A_KnockBack},              "A_KNOCKBACK"},
	{{A_PushAway},               "A_PUSHAWAY"},
	{{A_RingDrain},              "A_RINGDRAIN"},
	{{A_SplitShot},              "A_SPLITSHOT"},
	{{A_MissileSplit},           "A_MISSILESPLIT"},
	{{A_MultiShot},              "A_MULTISHOT"},
	{{A_InstaLoop},              "A_INSTALOOP"},
	{{A_Custom3DRotate},         "A_CUSTOM3DROTATE"},
	{{A_SearchForPlayers},       "A_SEARCHFORPLAYERS"},
	{{A_CheckRandom},            "A_CHECKRANDOM"},
	{{A_CheckTargetRings},       "A_CHECKTARGETRINGS"},
	{{A_CheckRings},             "A_CHECKRINGS"},
	{{A_CheckTotalRings},        "A_CHECKTOTALRINGS"},
	{{A_CheckHealth},            "A_CHECKHEALTH"},
	{{A_CheckRange},             "A_CHECKRANGE"},
	{{A_CheckHeight},            "A_CHECKHEIGHT"},
	{{A_CheckTrueRange},         "A_CHECKTRUERANGE"},
	{{A_CheckThingCount},        "A_CHECKTHINGCOUNT"},
	{{A_CheckAmbush},            "A_CHECKAMBUSH"},
	{{A_CheckCustomValue},       "A_CHECKCUSTOMVALUE"},
	{{A_CheckCusValMemo},        "A_CHECKCUSVALMEMO"},
	{{A_SetCustomValue},         "A_SETCUSTOMVALUE"},
	{{A_UseCusValMemo},          "A_USECUSVALMEMO"},
	{{A_RelayCustomValue},       "A_RELAYCUSTOMVALUE"},
	{{A_CusValAction},           "A_CUSVALACTION"},
	{{A_ForceStop},              "A_FORCESTOP"},
	{{A_ForceWin},               "A_FORCEWIN"},
	{{A_SpikeRetract},           "A_SPIKERETRACT"},
	{{A_InfoState},              "A_INFOSTATE"},
	{{A_Repeat},                 "A_REPEAT"},
	{{A_SetScale},               "A_SETSCALE"},
	{{A_RemoteDamage},           "A_REMOTEDAMAGE"},
	{{A_HomingChase},            "A_HOMINGCHASE"},
	{{A_TrapShot},               "A_TRAPSHOT"},
	{{A_VileTarget},             "A_VILETARGET"},
	{{A_VileAttack},             "A_VILEATTACK"},
	{{A_VileFire},               "A_VILEFIRE"},
	{{A_BrakChase},              "A_BRAKCHASE"},
	{{A_BrakFireShot},           "A_BRAKFIRESHOT"},
	{{A_BrakLobShot},            "A_BRAKLOBSHOT"},
	{{A_NapalmScatter},          "A_NAPALMSCATTER"},
	{{A_SpawnFreshCopy},         "A_SPAWNFRESHCOPY"},
	{{A_FlickySpawn},            "A_FLICKYSPAWN"},
	{{A_FlickyCenter},           "A_FLICKYCENTER"},
	{{A_FlickyAim},              "A_FLICKYAIM"},
	{{A_FlickyFly},              "A_FLICKYFLY"},
	{{A_FlickySoar},             "A_FLICKYSOAR"},
	{{A_FlickyCoast},            "A_FLICKYCOAST"},
	{{A_FlickyHop},              "A_FLICKYHOP"},
	{{A_FlickyFlounder},         "A_FLICKYFLOUNDER"},
	{{A_FlickyCheck},            "A_FLICKYCHECK"},
	{{A_FlickyHeightCheck},      "A_FLICKYHEIGHTCHECK"},
	{{A_FlickyFlutter},          "A_FLICKYFLUTTER"},
	{{A_FlameParticle},          "A_FLAMEPARTICLE"},
	{{A_FadeOverlay},            "A_FADEOVERLAY"},
	{{A_Boss5Jump},              "A_BOSS5JUMP"},
	{{A_LightBeamReset},         "A_LIGHTBEAMRESET"},
	{{A_MineExplode},            "A_MINEEXPLODE"},
	{{A_MineRange},              "A_MINERANGE"},
	{{A_ConnectToGround},        "A_CONNECTTOGROUND"},
	{{A_SpawnParticleRelative},  "A_SPAWNPARTICLERELATIVE"},
	{{A_MultiShotDist},          "A_MULTISHOTDIST"},
	{{A_WhoCaresIfYourSonIsABee},"A_WHOCARESIFYOURSONISABEE"},
	{{A_ParentTriesToSleep},     "A_PARENTTRIESTOSLEEP"},
	{{A_CryingToMomma},          "A_CRYINGTOMOMMA"},
	{{A_CheckFlags2},            "A_CHECKFLAGS2"},
	{{A_DoNPCSkid},              "A_DONPCSKID"},
	{{A_DoNPCPain},              "A_DONPCPAIN"},
	{{A_PrepareRepeat},          "A_PREPAREREPEAT"},
	{{A_Boss5ExtraRepeat},       "A_BOSS5EXTRAREPEAT"},
	{{A_Boss5Calm},              "A_BOSS5CALM"},
	{{A_Boss5CheckOnGround},     "A_BOSS5CHECKONGROUND"},
	{{A_Boss5CheckFalling},      "A_BOSS5CHECKFALLING"},
	{{A_Boss5PinchShot},         "A_BOSS5PINCHSHOT"},
	{{A_Boss5MakeItRain},        "A_BOSS5MAKEITRAIN"},
	{{A_LookForBetter},          "A_LOOKFORBETTER"},
	{{A_Boss5BombExplode},       "A_BOSS5BOMBEXPLODE"},
	{{A_TNTExplode},             "A_TNTEXPLODE"},
	{{A_DebrisRandom},           "A_DEBRISRANDOM"},
	{{A_CanarivoreGas},          "A_CANARIVOREGAS"},
	{{A_KillSegments},           "A_KILLSEGMENTS"},
	{{A_SnapperSpawn},           "A_SNAPPERSPAWN"},
	{{A_SnapperThinker},         "A_SNAPPERTHINKER"},
	{{A_SaloonDoorSpawn},        "A_SALOONDOORSPAWN"},
	{{A_MinecartSparkThink},     "A_MINECARTSPARKTHINK"},
	{{A_ModuloToState},          "A_MODULOTOSTATE"},
	{{A_LavafallRocks},          "A_LAVAFALLROCKS"},
	{{A_LavafallLava},           "A_LAVAFALLLAVA"},
	{{A_FallingLavaCheck},       "A_FALLINGLAVACHECK"},
	{{A_FireShrink},             "A_FIRESHRINK"},
	{{A_PterabyteHover},         "A_PTERABYTEHOVER"},
	{{A_RolloutSpawn},           "A_ROLLOUTSPAWN"},
	{{A_RolloutRock},            "A_ROLLOUTROCK"},
	{{A_DragonWing},             "A_DRAGONWING"},
	{{A_DragonSegment},          "A_DRAGONSEGMENT"},
	{{A_ChangeHeight},           "A_CHANGEHEIGHT"},

	// SRB2Kart
	{{A_JawzExplode},            "A_JAWZEXPLODE"},
	{{A_SSMineSearch},           "A_SSMINESEARCH"},
	{{A_SSMineExplode},          "A_SSMINEEXPLODE"},
	{{A_SSMineFlash},            "A_SSMINEFLASH"},
	{{A_LandMineExplode},		 "A_LANDMINEEXPLODE"},
	{{A_BallhogExplode},         "A_BALLHOGEXPLODE"},
	{{A_SpecialStageBombExplode},"A_SPECIALSTAGEBOMBEXPLODE"},
	{{A_LightningFollowPlayer},  "A_LIGHTNINGFOLLOWPLAYER"},
	{{A_FZBoomFlash},            "A_FZBOOMFLASH"},
	{{A_FZBoomSmoke},            "A_FZBOOMSMOKE"},
	{{A_RandomShadowFrame},      "A_RANDOMSHADOWFRAME"},
	{{A_MayonakaArrow},          "A_MAYONAKAARROW"},
	{{A_FlameShieldPaper},       "A_FLAMESHIELDPAPER"},
	{{A_InvincSparkleRotate},    "A_INVINCSPARKLEROTATE"},
	{{A_SpawnItemDebrisCloud},   "A_SPAWNITEMDEBRISCLOUD"},
	{{A_RingShooterFace},        "A_RINGSHOOTERFACE"},
	{{A_SpawnSneakerPanel},      "A_SPAWNSNEAKERPANEL"},
	{{A_MakeSSCandle},           "A_MAKESSCANDLE"},
	{{A_HologramRandomTranslucency}, "A_HOLOGRAMRANDOMTRANSLUCENCY"},
	{{A_SSChainShatter}, "A_SSCHAINSHATTER"},

	{{NULL},                     "NONE"},

	// This NULL entry must be the last in the list
	{{NULL},                   NULL},
};

////////////////////////////////////////////////////////////////////////////////
// CRAZY LIST OF STATE NAMES AND ALL FROM HERE DOWN
// TODO: Make this all a seperate file or something, like part of info.c??
// TODO: Read the list from a text lump in a WAD as necessary instead
// or something, don't just keep it all in memory like this.
// TODO: Make the lists public so we can start using actual mobj
// and state names in warning and error messages! :D

// RegEx to generate this from info.h: ^\tS_([^,]+), --> \t"S_\1",
// I am leaving the prefixes solely for clarity to programmers,
// because sadly no one remembers this place while searching for full state names.
const char *const STATE_LIST[] = { // array length left dynamic for sanity testing later.
	"S_NULL",
	"S_UNKNOWN",
	"S_INVISIBLE", // state for invisible sprite

	"S_SPAWNSTATE",
	"S_SEESTATE",
	"S_MELEESTATE",
	"S_MISSILESTATE",
	"S_DEATHSTATE",
	"S_XDEATHSTATE",
	"S_RAISESTATE",

	"S_THOK",
	"S_SHADOW",

	// SRB2kart Frames
	"S_KART_STILL",
	"S_KART_STILL_L",
	"S_KART_STILL_R",
	"S_KART_STILL_GLANCE_L",
	"S_KART_STILL_GLANCE_R",
	"S_KART_STILL_LOOK_L",
	"S_KART_STILL_LOOK_R",
	"S_KART_SLOW",
	"S_KART_SLOW_L",
	"S_KART_SLOW_R",
	"S_KART_SLOW_GLANCE_L",
	"S_KART_SLOW_GLANCE_R",
	"S_KART_SLOW_LOOK_L",
	"S_KART_SLOW_LOOK_R",
	"S_KART_FAST",
	"S_KART_FAST_L",
	"S_KART_FAST_R",
	"S_KART_FAST_GLANCE_L",
	"S_KART_FAST_GLANCE_R",
	"S_KART_FAST_LOOK_L",
	"S_KART_FAST_LOOK_R",
	"S_KART_DRIFT_L",
	"S_KART_DRIFT_L_OUT",
	"S_KART_DRIFT_L_IN",
	"S_KART_DRIFT_R",
	"S_KART_DRIFT_R_OUT",
	"S_KART_DRIFT_R_IN",
	"S_KART_SPINOUT",
	"S_KART_DEAD",
	"S_KART_SIGN",
	"S_KART_SIGL",

	// technically the player goes here but it's an infinite tic state
	"S_OBJPLACE_DUMMY",

	"S_KART_LEFTOVER",
	"S_KART_LEFTOVER_NOTIRES",

	"S_KART_TIRE1",
	"S_KART_TIRE2",

	"S_KART_FIRE",
	"S_KART_SMOKE",

	"S_KART_XPL01",
	"S_KART_XPL02",
	"S_KART_XPL03",

	// Boss Explosion
	"S_BOSSEXPLODE",

	// S3&K Boss Explosion
	"S_SONIC3KBOSSEXPLOSION1",
	"S_SONIC3KBOSSEXPLOSION2",
	"S_SONIC3KBOSSEXPLOSION3",
	"S_SONIC3KBOSSEXPLOSION4",
	"S_SONIC3KBOSSEXPLOSION5",
	"S_SONIC3KBOSSEXPLOSION6",

	// Ring
	"S_RING",
	"S_FASTRING1",
	"S_FASTRING2",
	"S_FASTRING3",
	"S_FASTRING4",
	"S_FASTRING5",
	"S_FASTRING6",
	"S_FASTRING7",
	"S_FASTRING8",
	"S_FASTRING9",
	"S_FASTRING10",
	"S_FASTRING11",
	"S_FASTRING12",

	// Blue Sphere
	"S_BLUESPHERE",
	"S_BLUESPHERE_SPAWN",

	"S_BLUESPHERE_BOUNCE1",
	"S_BLUESPHERE_BOUNCE2",

	"S_BLUESPHERE_BOUNCE3",
	"S_BLUESPHERE_BOUNCE4",

	"S_BLUESPHERE_BOUNCE5",
	"S_BLUESPHERE_BOUNCE6",
	"S_BLUESPHERE_BOUNCE7",
	"S_BLUESPHERE_BOUNCE8",

	"S_BLUESPHERE_BOUNCE9",
	"S_BLUESPHERE_BOUNCE10",
	"S_BLUESPHERE_BOUNCE11",
	"S_BLUESPHERE_BOUNCE12",

	"S_BLUESPHERE_BOUNCE13",
	"S_BLUESPHERE_BOUNCE14",
	"S_BLUESPHERE_BOUNCE15",
	"S_BLUESPHERE_BOUNCE16",
	"S_BLUESPHERE_BOUNCE17",
	"S_BLUESPHERE_BOUNCE18",
	"S_BLUESPHERE_BOUNCE19",
	"S_BLUESPHERE_BOUNCE20",

	"S_BLUESPHERE_BOUNCE21",
	"S_BLUESPHERE_BOUNCE22",
	"S_BLUESPHERE_BOUNCE23",
	"S_BLUESPHERE_BOUNCE24",
	"S_BLUESPHERE_BOUNCE25",
	"S_BLUESPHERE_BOUNCE26",
	"S_BLUESPHERE_BOUNCE27",
	"S_BLUESPHERE_BOUNCE28",

	// Emblem
	"S_EMBLEM1",
	"S_EMBLEM2",
	"S_EMBLEM3",
	"S_EMBLEM4",
	"S_EMBLEM5",
	"S_EMBLEM6",
	"S_EMBLEM7",
	"S_EMBLEM8",
	"S_EMBLEM9",
	"S_EMBLEM10",
	"S_EMBLEM11",
	"S_EMBLEM12",
	"S_EMBLEM13",
	"S_EMBLEM14",
	"S_EMBLEM15",
	"S_EMBLEM16",
	"S_EMBLEM17",
	"S_EMBLEM18",
	"S_EMBLEM19",
	"S_EMBLEM20",
	"S_EMBLEM21",
	"S_EMBLEM22",
	"S_EMBLEM23",
	"S_EMBLEM24",
	"S_EMBLEM25",
	"S_EMBLEM26",

	// Spray Can
	"S_SPRAYCAN",

	// Ancient Shrine
	"S_ANCIENTSHRINE",

	"S_MORB1",
	"S_MORB2",
	"S_MORB3",
	"S_MORB4",
	"S_MORB5",
	"S_MORB6",
	"S_MORB7",
	"S_MORB8",
	"S_MORB9",
	"S_MORB10",
	"S_MORB11",
	"S_MORB12",
	"S_MORB13",
	"S_MORB14",
	"S_MORB15",

	// Chaos Emeralds
	"S_CHAOSEMERALD1",
	"S_CHAOSEMERALD2",
	"S_CHAOSEMERALD_UNDER",

	// Super Emeralds
	"S_SUPEREMERALD1",
	"S_SUPEREMERALD2",
	"S_SUPEREMERALD_UNDER",

	"S_EMERALDSPARK1",
	"S_EMERALDSPARK2",
	"S_EMERALDSPARK3",
	"S_EMERALDSPARK4",
	"S_EMERALDSPARK5",
	"S_EMERALDSPARK6",
	"S_EMERALDSPARK7",

	"S_EMERALDFLARE1",

	// Prison Egg Drops
	"S_PRISONEGGDROP_CD",
	"S_PRISONEGGDROP_FLAREA1",
	"S_PRISONEGGDROP_FLAREA2",
	"S_PRISONEGGDROP_FLAREB1",
	"S_PRISONEGGDROP_FLAREB2",

	// Bubble Source
	"S_BUBBLES1",
	"S_BUBBLES2",
	"S_BUBBLES3",
	"S_BUBBLES4",

	// Level End Sign
	"S_SIGN_POLE",
	"S_SIGN_BACK",
	"S_SIGN_SIDE",
	"S_SIGN_FACE",
	"S_SIGN_ERROR",

	// Spike Ball
	"S_SPIKEBALL1",
	"S_SPIKEBALL2",
	"S_SPIKEBALL3",
	"S_SPIKEBALL4",
	"S_SPIKEBALL5",
	"S_SPIKEBALL6",
	"S_SPIKEBALL7",
	"S_SPIKEBALL8",

	// Elemental Shield's Spawn
	"S_SPINFIRE1",
	"S_SPINFIRE2",
	"S_SPINFIRE3",
	"S_SPINFIRE4",
	"S_SPINFIRE5",
	"S_SPINFIRE6",

	// Spikes
	"S_SPIKE1",
	"S_SPIKE2",
	"S_SPIKE3",
	"S_SPIKE4",
	"S_SPIKE5",
	"S_SPIKE6",
	"S_SPIKED1",
	"S_SPIKED2",

	// Wall spikes
	"S_WALLSPIKE1",
	"S_WALLSPIKE2",
	"S_WALLSPIKE3",
	"S_WALLSPIKE4",
	"S_WALLSPIKE5",
	"S_WALLSPIKE6",
	"S_WALLSPIKEBASE",
	"S_WALLSPIKED1",
	"S_WALLSPIKED2",

	// Cannon Launcher
	"S_CANNONLAUNCHER1",
	"S_CANNONLAUNCHER2",
	"S_CANNONLAUNCHER3",

	// Cannonball
	"S_CANNONBALL1",

	// GFZ flowers
	"S_GFZFLOWERA",
	"S_GFZFLOWERB",
	"S_GFZFLOWERC",

	"S_BLUEBERRYBUSH",
	"S_BERRYBUSH",
	"S_BUSH",

	// Trees (both GFZ and misc)
	"S_GFZTREE",
	"S_GFZBERRYTREE",
	"S_GFZCHERRYTREE",
	"S_CHECKERTREE",
	"S_CHECKERSUNSETTREE",
	"S_FHZTREE", // Frozen Hillside
	"S_FHZPINKTREE",
	"S_POLYGONTREE",
	"S_BUSHTREE",
	"S_BUSHREDTREE",
	"S_SPRINGTREE",

	// THZ flowers
	"S_THZFLOWERA", // THZ1 Steam flower
	"S_THZFLOWERB", // THZ1 Spin flower (red)
	"S_THZFLOWERC", // THZ1 Spin flower (yellow)

	// THZ Steam Whistle tree/bush
	"S_THZTREE",
	"S_THZTREEBRANCH1",
	"S_THZTREEBRANCH2",
	"S_THZTREEBRANCH3",
	"S_THZTREEBRANCH4",
	"S_THZTREEBRANCH5",
	"S_THZTREEBRANCH6",
	"S_THZTREEBRANCH7",
	"S_THZTREEBRANCH8",
	"S_THZTREEBRANCH9",
	"S_THZTREEBRANCH10",
	"S_THZTREEBRANCH11",
	"S_THZTREEBRANCH12",
	"S_THZTREEBRANCH13",

	// THZ Alarm
	"S_ALARM1",

	// Deep Sea Gargoyle
	"S_GARGOYLE",
	"S_BIGGARGOYLE",

	// DSZ Seaweed
	"S_SEAWEED1",
	"S_SEAWEED2",
	"S_SEAWEED3",
	"S_SEAWEED4",
	"S_SEAWEED5",
	"S_SEAWEED6",

	// Dripping Water
	"S_DRIPA1",
	"S_DRIPA2",
	"S_DRIPA3",
	"S_DRIPA4",
	"S_DRIPB1",
	"S_DRIPC1",
	"S_DRIPC2",

	// Coral
	"S_CORAL1",
	"S_CORAL2",
	"S_CORAL3",
	"S_CORAL4",
	"S_CORAL5",

	// Blue Crystal
	"S_BLUECRYSTAL1",

	// Kelp,
	"S_KELP",

	// Animated algae
	"S_ANIMALGAETOP1",
	"S_ANIMALGAETOP2",
	"S_ANIMALGAESEG",

	// DSZ Stalagmites
	"S_DSZSTALAGMITE",
	"S_DSZ2STALAGMITE",

	// DSZ Light beam
	"S_LIGHTBEAM1",
	"S_LIGHTBEAM2",
	"S_LIGHTBEAM3",
	"S_LIGHTBEAM4",
	"S_LIGHTBEAM5",
	"S_LIGHTBEAM6",
	"S_LIGHTBEAM7",
	"S_LIGHTBEAM8",
	"S_LIGHTBEAM9",
	"S_LIGHTBEAM10",
	"S_LIGHTBEAM11",
	"S_LIGHTBEAM12",

	// CEZ Chain
	"S_CEZCHAIN",

	// Flame
	"S_FLAME",
	"S_FLAMEPARTICLE",
	"S_FLAMEREST",

	// Eggman Statue
	"S_EGGSTATUE1",

	// CEZ hidden sling
	"S_SLING1",
	"S_SLING2",

	// CEZ maces and chains
	"S_SMALLMACECHAIN",
	"S_BIGMACECHAIN",
	"S_SMALLMACE",
	"S_BIGMACE",
	"S_SMALLGRABCHAIN",
	"S_BIGGRABCHAIN",

	// Small Firebar
	"S_SMALLFIREBAR1",
	"S_SMALLFIREBAR2",
	"S_SMALLFIREBAR3",
	"S_SMALLFIREBAR4",
	"S_SMALLFIREBAR5",
	"S_SMALLFIREBAR6",
	"S_SMALLFIREBAR7",
	"S_SMALLFIREBAR8",
	"S_SMALLFIREBAR9",
	"S_SMALLFIREBAR10",
	"S_SMALLFIREBAR11",
	"S_SMALLFIREBAR12",
	"S_SMALLFIREBAR13",
	"S_SMALLFIREBAR14",
	"S_SMALLFIREBAR15",
	"S_SMALLFIREBAR16",

	// Big Firebar
	"S_BIGFIREBAR1",
	"S_BIGFIREBAR2",
	"S_BIGFIREBAR3",
	"S_BIGFIREBAR4",
	"S_BIGFIREBAR5",
	"S_BIGFIREBAR6",
	"S_BIGFIREBAR7",
	"S_BIGFIREBAR8",
	"S_BIGFIREBAR9",
	"S_BIGFIREBAR10",
	"S_BIGFIREBAR11",
	"S_BIGFIREBAR12",
	"S_BIGFIREBAR13",
	"S_BIGFIREBAR14",
	"S_BIGFIREBAR15",
	"S_BIGFIREBAR16",

	"S_CEZFLOWER",
	"S_CEZPOLE",
	"S_CEZBANNER1",
	"S_CEZBANNER2",
	"S_PINETREE",
	"S_CEZBUSH1",
	"S_CEZBUSH2",
	"S_CANDLE",
	"S_CANDLEPRICKET",
	"S_FLAMEHOLDER",
	"S_FIRETORCH",
	"S_WAVINGFLAG",
	"S_WAVINGFLAGSEG1",
	"S_WAVINGFLAGSEG2",
	"S_CRAWLASTATUE",
	"S_BRAMBLES",

	// Big Tumbleweed
	"S_BIGTUMBLEWEED",
	"S_BIGTUMBLEWEED_ROLL1",
	"S_BIGTUMBLEWEED_ROLL2",
	"S_BIGTUMBLEWEED_ROLL3",
	"S_BIGTUMBLEWEED_ROLL4",
	"S_BIGTUMBLEWEED_ROLL5",
	"S_BIGTUMBLEWEED_ROLL6",
	"S_BIGTUMBLEWEED_ROLL7",
	"S_BIGTUMBLEWEED_ROLL8",

	// Little Tumbleweed
	"S_LITTLETUMBLEWEED",
	"S_LITTLETUMBLEWEED_ROLL1",
	"S_LITTLETUMBLEWEED_ROLL2",
	"S_LITTLETUMBLEWEED_ROLL3",
	"S_LITTLETUMBLEWEED_ROLL4",
	"S_LITTLETUMBLEWEED_ROLL5",
	"S_LITTLETUMBLEWEED_ROLL6",
	"S_LITTLETUMBLEWEED_ROLL7",
	"S_LITTLETUMBLEWEED_ROLL8",

	// Cacti
	"S_CACTI1",
	"S_CACTI2",
	"S_CACTI3",
	"S_CACTI4",
	"S_CACTI5",
	"S_CACTI6",
	"S_CACTI7",
	"S_CACTI8",
	"S_CACTI9",
	"S_CACTI10",
	"S_CACTI11",
	"S_CACTITINYSEG",
	"S_CACTISMALLSEG",

	// Warning signs
	"S_ARIDSIGN_CAUTION",
	"S_ARIDSIGN_CACTI",
	"S_ARIDSIGN_SHARPTURN",

	// Oil lamp
	"S_OILLAMP",
	"S_OILLAMPFLARE",

	// TNT barrel
	"S_TNTBARREL_STND1",
	"S_TNTBARREL_EXPL1",
	"S_TNTBARREL_EXPL2",
	"S_TNTBARREL_EXPL3",
	"S_TNTBARREL_EXPL4",
	"S_TNTBARREL_EXPL5",
	"S_TNTBARREL_EXPL6",
	"S_TNTBARREL_EXPL7",
	"S_TNTBARREL_FLYING",
	"S_TNTDUST_1",
	"S_TNTDUST_2",
	"S_TNTDUST_3",
	"S_TNTDUST_4",
	"S_TNTDUST_5",
	"S_TNTDUST_6",
	"S_TNTDUST_7",
	"S_TNTDUST_8",

	// TNT proximity shell
	"S_PROXIMITY_TNT",
	"S_PROXIMITY_TNT_TRIGGER1",
	"S_PROXIMITY_TNT_TRIGGER2",
	"S_PROXIMITY_TNT_TRIGGER3",
	"S_PROXIMITY_TNT_TRIGGER4",
	"S_PROXIMITY_TNT_TRIGGER5",
	"S_PROXIMITY_TNT_TRIGGER6",
	"S_PROXIMITY_TNT_TRIGGER7",
	"S_PROXIMITY_TNT_TRIGGER8",
	"S_PROXIMITY_TNT_TRIGGER9",
	"S_PROXIMITY_TNT_TRIGGER10",
	"S_PROXIMITY_TNT_TRIGGER11",
	"S_PROXIMITY_TNT_TRIGGER12",
	"S_PROXIMITY_TNT_TRIGGER13",
	"S_PROXIMITY_TNT_TRIGGER14",
	"S_PROXIMITY_TNT_TRIGGER15",
	"S_PROXIMITY_TNT_TRIGGER16",
	"S_PROXIMITY_TNT_TRIGGER17",
	"S_PROXIMITY_TNT_TRIGGER18",
	"S_PROXIMITY_TNT_TRIGGER19",
	"S_PROXIMITY_TNT_TRIGGER20",
	"S_PROXIMITY_TNT_TRIGGER21",
	"S_PROXIMITY_TNT_TRIGGER22",
	"S_PROXIMITY_TNT_TRIGGER23",

	// Dust devil
	"S_DUSTDEVIL",
	"S_DUSTLAYER1",
	"S_DUSTLAYER2",
	"S_DUSTLAYER3",
	"S_DUSTLAYER4",
	"S_DUSTLAYER5",
	"S_ARIDDUST1",
	"S_ARIDDUST2",
	"S_ARIDDUST3",

	// Flame jet
	"S_FLAMEJETSTND",
	"S_FLAMEJETSTART",
	"S_FLAMEJETSTOP",
	"S_FLAMEJETFLAME1",
	"S_FLAMEJETFLAME2",
	"S_FLAMEJETFLAME3",
	"S_FLAMEJETFLAME4",
	"S_FLAMEJETFLAME5",
	"S_FLAMEJETFLAME6",
	"S_FLAMEJETFLAME7",
	"S_FLAMEJETFLAME8",
	"S_FLAMEJETFLAME9",

	// Spinning flame jets
	"S_FJSPINAXISA1", // Counter-clockwise
	"S_FJSPINAXISA2",
	"S_FJSPINAXISB1", // Clockwise
	"S_FJSPINAXISB2",

	// Blade's flame
	"S_FLAMEJETFLAMEB1",
	"S_FLAMEJETFLAMEB2",
	"S_FLAMEJETFLAMEB3",

	// Lavafall
	"S_LAVAFALL_DORMANT",
	"S_LAVAFALL_TELL",
	"S_LAVAFALL_SHOOT",
	"S_LAVAFALL_LAVA1",
	"S_LAVAFALL_LAVA2",
	"S_LAVAFALL_LAVA3",
	"S_LAVAFALLROCK",

	// RVZ scenery
	"S_BIGFERNLEAF",
	"S_BIGFERN1",
	"S_BIGFERN2",
	"S_JUNGLEPALM",
	"S_TORCHFLOWER",
	"S_WALLVINE_LONG",
	"S_WALLVINE_SHORT",

	// Stalagmites
	"S_STG0",
	"S_STG1",
	"S_STG2",
	"S_STG3",
	"S_STG4",
	"S_STG5",
	"S_STG6",
	"S_STG7",
	"S_STG8",
	"S_STG9",

	// Xmas-specific stuff
	"S_XMASPOLE",
	"S_CANDYCANE",
	"S_SNOWMAN",    // normal
	"S_SNOWMANHAT", // with hat + scarf
	"S_LAMPPOST1",  // normal
	"S_LAMPPOST2",  // with snow
	"S_HANGSTAR",
	"S_MISTLETOE",
	// Xmas GFZ bushes
	"S_XMASBLUEBERRYBUSH",
	"S_XMASBERRYBUSH",
	"S_XMASBUSH",
	// FHZ
	"S_FHZICE1",
	"S_FHZICE2",

	// Halloween Scenery
	// Pumpkins
	"S_JACKO1",
	"S_JACKO1OVERLAY_1",
	"S_JACKO1OVERLAY_2",
	"S_JACKO1OVERLAY_3",
	"S_JACKO1OVERLAY_4",
	"S_JACKO2",
	"S_JACKO2OVERLAY_1",
	"S_JACKO2OVERLAY_2",
	"S_JACKO2OVERLAY_3",
	"S_JACKO2OVERLAY_4",
	"S_JACKO3",
	"S_JACKO3OVERLAY_1",
	"S_JACKO3OVERLAY_2",
	"S_JACKO3OVERLAY_3",
	"S_JACKO3OVERLAY_4",
	// Dr Seuss Trees
	"S_HHZTREE_TOP",
	"S_HHZTREE_TRUNK",
	"S_HHZTREE_LEAF",
	// Mushroom
	"S_HHZSHROOM_1",
	"S_HHZSHROOM_2",
	"S_HHZSHROOM_3",
	"S_HHZSHROOM_4",
	"S_HHZSHROOM_5",
	"S_HHZSHROOM_6",
	"S_HHZSHROOM_7",
	"S_HHZSHROOM_8",
	"S_HHZSHROOM_9",
	"S_HHZSHROOM_10",
	"S_HHZSHROOM_11",
	"S_HHZSHROOM_12",
	"S_HHZSHROOM_13",
	"S_HHZSHROOM_14",
	"S_HHZSHROOM_15",
	"S_HHZSHROOM_16",
	// Misc
	"S_HHZGRASS",
	"S_HHZTENT1",
	"S_HHZTENT2",
	"S_HHZSTALAGMITE_TALL",
	"S_HHZSTALAGMITE_SHORT",

	// Botanic Serenity's loads of scenery states
	"S_BSZTALLFLOWER_RED",
	"S_BSZTALLFLOWER_PURPLE",
	"S_BSZTALLFLOWER_BLUE",
	"S_BSZTALLFLOWER_CYAN",
	"S_BSZTALLFLOWER_YELLOW",
	"S_BSZTALLFLOWER_ORANGE",
	"S_BSZFLOWER_RED",
	"S_BSZFLOWER_PURPLE",
	"S_BSZFLOWER_BLUE",
	"S_BSZFLOWER_CYAN",
	"S_BSZFLOWER_YELLOW",
	"S_BSZFLOWER_ORANGE",
	"S_BSZSHORTFLOWER_RED",
	"S_BSZSHORTFLOWER_PURPLE",
	"S_BSZSHORTFLOWER_BLUE",
	"S_BSZSHORTFLOWER_CYAN",
	"S_BSZSHORTFLOWER_YELLOW",
	"S_BSZSHORTFLOWER_ORANGE",
	"S_BSZTULIP_RED",
	"S_BSZTULIP_PURPLE",
	"S_BSZTULIP_BLUE",
	"S_BSZTULIP_CYAN",
	"S_BSZTULIP_YELLOW",
	"S_BSZTULIP_ORANGE",
	"S_BSZCLUSTER_RED",
	"S_BSZCLUSTER_PURPLE",
	"S_BSZCLUSTER_BLUE",
	"S_BSZCLUSTER_CYAN",
	"S_BSZCLUSTER_YELLOW",
	"S_BSZCLUSTER_ORANGE",
	"S_BSZBUSH_RED",
	"S_BSZBUSH_PURPLE",
	"S_BSZBUSH_BLUE",
	"S_BSZBUSH_CYAN",
	"S_BSZBUSH_YELLOW",
	"S_BSZBUSH_ORANGE",
	"S_BSZVINE_RED",
	"S_BSZVINE_PURPLE",
	"S_BSZVINE_BLUE",
	"S_BSZVINE_CYAN",
	"S_BSZVINE_YELLOW",
	"S_BSZVINE_ORANGE",
	"S_BSZSHRUB",
	"S_BSZCLOVER",
	"S_BIG_PALMTREE_TRUNK",
	"S_BIG_PALMTREE_TOP",
	"S_PALMTREE_TRUNK",
	"S_PALMTREE_TOP",

	"S_DBALL1",
	"S_DBALL2",
	"S_DBALL3",
	"S_DBALL4",
	"S_DBALL5",
	"S_DBALL6",
	"S_EGGSTATUE2",

	// Super Sonic Spark
	"S_SSPK1",
	"S_SSPK2",
	"S_SSPK3",
	"S_SSPK4",
	"S_SSPK5",

	// Flicky-sized bubble
	"S_FLICKY_BUBBLE",

	// Bluebird
	"S_FLICKY_01_OUT",
	"S_FLICKY_01_FLAP1",
	"S_FLICKY_01_FLAP2",
	"S_FLICKY_01_FLAP3",
	"S_FLICKY_01_STAND",
	"S_FLICKY_01_CENTER",

	// Rabbit
	"S_FLICKY_02_OUT",
	"S_FLICKY_02_AIM",
	"S_FLICKY_02_HOP",
	"S_FLICKY_02_UP",
	"S_FLICKY_02_DOWN",
	"S_FLICKY_02_STAND",
	"S_FLICKY_02_CENTER",

	// Chicken
	"S_FLICKY_03_OUT",
	"S_FLICKY_03_AIM",
	"S_FLICKY_03_HOP",
	"S_FLICKY_03_UP",
	"S_FLICKY_03_FLAP1",
	"S_FLICKY_03_FLAP2",
	"S_FLICKY_03_STAND",
	"S_FLICKY_03_CENTER",

	// Seal
	"S_FLICKY_04_OUT",
	"S_FLICKY_04_AIM",
	"S_FLICKY_04_HOP",
	"S_FLICKY_04_UP",
	"S_FLICKY_04_DOWN",
	"S_FLICKY_04_SWIM1",
	"S_FLICKY_04_SWIM2",
	"S_FLICKY_04_SWIM3",
	"S_FLICKY_04_SWIM4",
	"S_FLICKY_04_STAND",
	"S_FLICKY_04_CENTER",

	// Pig
	"S_FLICKY_05_OUT",
	"S_FLICKY_05_AIM",
	"S_FLICKY_05_HOP",
	"S_FLICKY_05_UP",
	"S_FLICKY_05_DOWN",
	"S_FLICKY_05_STAND",
	"S_FLICKY_05_CENTER",

	// Chipmunk
	"S_FLICKY_06_OUT",
	"S_FLICKY_06_AIM",
	"S_FLICKY_06_HOP",
	"S_FLICKY_06_UP",
	"S_FLICKY_06_DOWN",
	"S_FLICKY_06_STAND",
	"S_FLICKY_06_CENTER",

	// Penguin
	"S_FLICKY_07_OUT",
	"S_FLICKY_07_AIML",
	"S_FLICKY_07_HOPL",
	"S_FLICKY_07_UPL",
	"S_FLICKY_07_DOWNL",
	"S_FLICKY_07_AIMR",
	"S_FLICKY_07_HOPR",
	"S_FLICKY_07_UPR",
	"S_FLICKY_07_DOWNR",
	"S_FLICKY_07_SWIM1",
	"S_FLICKY_07_SWIM2",
	"S_FLICKY_07_SWIM3",
	"S_FLICKY_07_STAND",
	"S_FLICKY_07_CENTER",

	// Fish
	"S_FLICKY_08_OUT",
	"S_FLICKY_08_AIM",
	"S_FLICKY_08_HOP",
	"S_FLICKY_08_FLAP1",
	"S_FLICKY_08_FLAP2",
	"S_FLICKY_08_FLAP3",
	"S_FLICKY_08_FLAP4",
	"S_FLICKY_08_SWIM1",
	"S_FLICKY_08_SWIM2",
	"S_FLICKY_08_SWIM3",
	"S_FLICKY_08_SWIM4",
	"S_FLICKY_08_STAND",
	"S_FLICKY_08_CENTER",

	// Ram
	"S_FLICKY_09_OUT",
	"S_FLICKY_09_AIM",
	"S_FLICKY_09_HOP",
	"S_FLICKY_09_UP",
	"S_FLICKY_09_DOWN",
	"S_FLICKY_09_STAND",
	"S_FLICKY_09_CENTER",

	// Puffin
	"S_FLICKY_10_OUT",
	"S_FLICKY_10_FLAP1",
	"S_FLICKY_10_FLAP2",
	"S_FLICKY_10_STAND",
	"S_FLICKY_10_CENTER",

	// Cow
	"S_FLICKY_11_OUT",
	"S_FLICKY_11_AIM",
	"S_FLICKY_11_RUN1",
	"S_FLICKY_11_RUN2",
	"S_FLICKY_11_RUN3",
	"S_FLICKY_11_STAND",
	"S_FLICKY_11_CENTER",

	// Rat
	"S_FLICKY_12_OUT",
	"S_FLICKY_12_AIM",
	"S_FLICKY_12_RUN1",
	"S_FLICKY_12_RUN2",
	"S_FLICKY_12_RUN3",
	"S_FLICKY_12_STAND",
	"S_FLICKY_12_CENTER",

	// Bear
	"S_FLICKY_13_OUT",
	"S_FLICKY_13_AIM",
	"S_FLICKY_13_HOP",
	"S_FLICKY_13_UP",
	"S_FLICKY_13_DOWN",
	"S_FLICKY_13_STAND",
	"S_FLICKY_13_CENTER",

	// Dove
	"S_FLICKY_14_OUT",
	"S_FLICKY_14_FLAP1",
	"S_FLICKY_14_FLAP2",
	"S_FLICKY_14_FLAP3",
	"S_FLICKY_14_STAND",
	"S_FLICKY_14_CENTER",

	// Cat
	"S_FLICKY_15_OUT",
	"S_FLICKY_15_AIM",
	"S_FLICKY_15_HOP",
	"S_FLICKY_15_UP",
	"S_FLICKY_15_DOWN",
	"S_FLICKY_15_STAND",
	"S_FLICKY_15_CENTER",

	// Canary
	"S_FLICKY_16_OUT",
	"S_FLICKY_16_FLAP1",
	"S_FLICKY_16_FLAP2",
	"S_FLICKY_16_FLAP3",
	"S_FLICKY_16_STAND",
	"S_FLICKY_16_CENTER",

	// Spider
	"S_SECRETFLICKY_01_OUT",
	"S_SECRETFLICKY_01_AIM",
	"S_SECRETFLICKY_01_HOP",
	"S_SECRETFLICKY_01_UP",
	"S_SECRETFLICKY_01_DOWN",
	"S_SECRETFLICKY_01_STAND",
	"S_SECRETFLICKY_01_CENTER",

	// Bat
	"S_SECRETFLICKY_02_OUT",
	"S_SECRETFLICKY_02_FLAP1",
	"S_SECRETFLICKY_02_FLAP2",
	"S_SECRETFLICKY_02_FLAP3",
	"S_SECRETFLICKY_02_STAND",
	"S_SECRETFLICKY_02_CENTER",

	// Steam Riser
	"S_STEAM1",
	"S_STEAM2",
	"S_STEAM3",
	"S_STEAM4",
	"S_STEAM5",
	"S_STEAM6",
	"S_STEAM7",
	"S_STEAM8",

	// Balloons
	"S_BALLOON",
	"S_BALLOONPOP1",
	"S_BALLOONPOP2",
	"S_BALLOONPOP3",
	"S_BALLOONPOP4",
	"S_BALLOONPOP5",
	"S_BALLOONPOP6",

	// Yellow Spring
	"S_YELLOWSPRING1",
	"S_YELLOWSPRING2",
	"S_YELLOWSPRING3",
	"S_YELLOWSPRING4",

	// Red Spring
	"S_REDSPRING1",
	"S_REDSPRING2",
	"S_REDSPRING3",
	"S_REDSPRING4",

	// Blue Spring
	"S_BLUESPRING1",
	"S_BLUESPRING2",
	"S_BLUESPRING3",
	"S_BLUESPRING4",

	// Grey Spring
	"S_GREYSPRING1",
	"S_GREYSPRING2",
	"S_GREYSPRING3",
	"S_GREYSPRING4",

	// Orange Spring (Pogo)
	"S_POGOSPRING1",
	"S_POGOSPRING2",
	"S_POGOSPRING2B",
	"S_POGOSPRING3",
	"S_POGOSPRING4",

	// Yellow Diagonal Spring
	"S_YDIAG1",
	"S_YDIAG2",
	"S_YDIAG3",
	"S_YDIAG4",

	// Red Diagonal Spring
	"S_RDIAG1",
	"S_RDIAG2",
	"S_RDIAG3",
	"S_RDIAG4",

	// Blue Diagonal Spring
	"S_BDIAG1",
	"S_BDIAG2",
	"S_BDIAG3",
	"S_BDIAG4",

	// Grey Diagonal Spring
	"S_GDIAG1",
	"S_GDIAG2",
	"S_GDIAG3",
	"S_GDIAG4",

	// Yellow Horizontal Spring
	"S_YHORIZ1",
	"S_YHORIZ2",
	"S_YHORIZ3",
	"S_YHORIZ4",

	// Red Horizontal Spring
	"S_RHORIZ1",
	"S_RHORIZ2",
	"S_RHORIZ3",
	"S_RHORIZ4",

	// Blue Horizontal Spring
	"S_BHORIZ1",
	"S_BHORIZ2",
	"S_BHORIZ3",
	"S_BHORIZ4",

	// Grey Horizontal Spring
	"S_GHORIZ1",
	"S_GHORIZ2",
	"S_GHORIZ3",
	"S_GHORIZ4",

	// Rain
	"S_RAIN1",
	"S_RAINRETURN",

	// Snowflake
	"S_SNOW1",
	"S_SNOW2",
	"S_SNOW3",

	// Blizzard Snowball
	"S_BLIZZARDSNOW1",
	"S_BLIZZARDSNOW2",
	"S_BLIZZARDSNOW3",

	// Water Splish
	"S_SPLISH1",
	"S_SPLISH2",
	"S_SPLISH3",
	"S_SPLISH4",
	"S_SPLISH5",
	"S_SPLISH6",
	"S_SPLISH7",
	"S_SPLISH8",
	"S_SPLISH9",

	// Lava Splish
	"S_LAVASPLISH",

	// added water splash
	"S_SPLASH1",
	"S_SPLASH2",
	"S_SPLASH3",

	// lava/slime damage burn smoke
	"S_SMOKE1",
	"S_SMOKE2",
	"S_SMOKE3",
	"S_SMOKE4",
	"S_SMOKE5",

	// Bubbles
	"S_SMALLBUBBLE",
	"S_MEDIUMBUBBLE",
	"S_LARGEBUBBLE1",
	"S_LARGEBUBBLE2",
	"S_EXTRALARGEBUBBLE", // breathable

	"S_POP1", // Extra Large bubble goes POP!

	"S_WATERZAP",

	// Spindash dust
	"S_SPINDUST1",
	"S_SPINDUST2",
	"S_SPINDUST3",
	"S_SPINDUST4",
	"S_SPINDUST_BUBBLE1",
	"S_SPINDUST_BUBBLE2",
	"S_SPINDUST_BUBBLE3",
	"S_SPINDUST_BUBBLE4",
	"S_SPINDUST_FIRE1",
	"S_SPINDUST_FIRE2",
	"S_SPINDUST_FIRE3",
	"S_SPINDUST_FIRE4",

	"S_SEED",

	"S_PARTICLE",

	// Drowning Timer Numbers
	"S_ZERO1",
	"S_ONE1",
	"S_TWO1",
	"S_THREE1",
	"S_FOUR1",
	"S_FIVE1",

	"S_ZERO2",
	"S_ONE2",
	"S_TWO2",
	"S_THREE2",
	"S_FOUR2",
	"S_FIVE2",

	"S_CORK",
	"S_LHRT",

	"S_RINGEXPLODE",

	"S_HOOP",
	"S_HOOP_XMASA",
	"S_HOOP_XMASB",

	"S_EGGCAPSULE",

	// Secret badniks and hazards, shhhh
	"S_SMASHSPIKE_FLOAT",
	"S_SMASHSPIKE_EASE1",
	"S_SMASHSPIKE_EASE2",
	"S_SMASHSPIKE_FALL",
	"S_SMASHSPIKE_STOMP1",
	"S_SMASHSPIKE_STOMP2",
	"S_SMASHSPIKE_RISE1",
	"S_SMASHSPIKE_RISE2",

	"S_CRUMBLE1",
	"S_CRUMBLE2",

	// Spark
	"S_SPRK1",
	"S_SPRK2",
	"S_SPRK3",

	// Robot Explosion
	"S_XPLD_FLICKY",
	"S_XPLD1",
	"S_XPLD2",
	"S_XPLD3",
	"S_XPLD4",
	"S_XPLD5",
	"S_XPLD6",
	"S_XPLD_EGGTRAP",

	// Underwater Explosion
	"S_WPLD1",
	"S_WPLD2",
	"S_WPLD3",
	"S_WPLD4",
	"S_WPLD5",
	"S_WPLD6",

	"S_DUST1",
	"S_DUST2",
	"S_DUST3",
	"S_DUST4",

	"S_ROCKSPAWN",

	"S_ROCKCRUMBLEA",
	"S_ROCKCRUMBLEB",
	"S_ROCKCRUMBLEC",
	"S_ROCKCRUMBLED",
	"S_ROCKCRUMBLEE",
	"S_ROCKCRUMBLEF",
	"S_ROCKCRUMBLEG",
	"S_ROCKCRUMBLEH",
	"S_ROCKCRUMBLEI",
	"S_ROCKCRUMBLEJ",
	"S_ROCKCRUMBLEK",
	"S_ROCKCRUMBLEL",
	"S_ROCKCRUMBLEM",
	"S_ROCKCRUMBLEN",
	"S_ROCKCRUMBLEO",
	"S_ROCKCRUMBLEP",

	// Level debris
	"S_GFZDEBRIS",
	"S_BRICKDEBRIS",
	"S_WOODDEBRIS",

	//{ Random Item Box
	"S_RANDOMITEM1",
	"S_RANDOMITEM2",
	"S_RANDOMITEM3",
	"S_RANDOMITEM4",
	"S_RANDOMITEM5",
	"S_RANDOMITEM6",
	"S_RANDOMITEM7",
	"S_RANDOMITEM8",
	"S_RANDOMITEM9",
	"S_RANDOMITEM10",
	"S_RANDOMITEM11",
	"S_RANDOMITEM12",

	// Ring Box
	"S_RINGBOX1",
	"S_RINGBOX2",
	"S_RINGBOX3",
	"S_RINGBOX4",
	"S_RINGBOX5",
	"S_RINGBOX6",
	"S_RINGBOX7",
	"S_RINGBOX8",
	"S_RINGBOX9",
	"S_RINGBOX10",
	"S_RINGBOX11",
	"S_RINGBOX12",

	// Sphere Box (for Battle)
	"S_SPHEREBOX1",
	"S_SPHEREBOX2",
	"S_SPHEREBOX3",
	"S_SPHEREBOX4",
	"S_SPHEREBOX5",
	"S_SPHEREBOX6",
	"S_SPHEREBOX7",
	"S_SPHEREBOX8",
	"S_SPHEREBOX9",
	"S_SPHEREBOX10",
	"S_SPHEREBOX11",
	"S_SPHEREBOX12",

	"S_ITEM_DEBRIS",
	"S_ITEM_DEBRIS_CLOUD_SPAWNER1",
	"S_ITEM_DEBRIS_CLOUD_SPAWNER2",

	"S_ITEMICON",
	"S_ITEMBACKDROP",

	// Item capsules
	"S_ITEMCAPSULE",
	"S_ITEMCAPSULE_TOP_SIDE",
	"S_ITEMCAPSULE_BOTTOM_SIDE_AIR",
	"S_ITEMCAPSULE_BOTTOM_SIDE_GROUND",
	//"S_ITEMCAPSULE_TOP",
	//"S_ITEMCAPSULE_BOTTOM",
	//"S_ITEMCAPSULE_INSIDE",

	"S_MONITOR_DAMAGE",
	"S_MONITOR_DEATH",
	"S_MONITOR_SCREEN1A",
	"S_MONITOR_SCREEN1B",
	"S_MONITOR_SCREEN2A",
	"S_MONITOR_SCREEN2B",
	"S_MONITOR_SCREEN3A",
	"S_MONITOR_SCREEN3B",
	"S_MONITOR_SCREEN4A",
	"S_MONITOR_SCREEN4B",
	"S_MONITOR_STAND",
	"S_MONITOR_CRACKA",
	"S_MONITOR_CRACKB",

	"S_MONITOR_BIG_SHARD",
	"S_MONITOR_SMALL_SHARD",
	"S_MONITOR_TWINKLE",

	"S_MAGICIANBOX",
	"S_MAGICIANBOXTOP",
	"S_MAGICIANBOXBOTTOM",

	"S_WAVEDASH",

	"S_INSTAWHIP",
	"S_INSTAWHIP_RECHARGE1",
	"S_INSTAWHIP_RECHARGE2",
	"S_INSTAWHIP_RECHARGE3",
	"S_INSTAWHIP_RECHARGE4",
	"S_INSTAWHIP_REJECT",
	"S_BLOCKRING",
	"S_BLOCKBODY",

	"S_CHARGEAURA",
	"S_CHARGEFALL",
	"S_CHARGEFLICKER",
	"S_CHARGESPARK",
	"S_CHARGERELEASE",
	"S_CHARGEEXTRA",

	"S_SERVANTHAND",

	"S_HORNCODE",

	// Signpost sparkles
	"S_SIGNSPARK1",
	"S_SIGNSPARK2",
	"S_SIGNSPARK3",
	"S_SIGNSPARK4",
	"S_SIGNSPARK5",
	"S_SIGNSPARK6",
	"S_SIGNSPARK7",
	"S_SIGNSPARK8",
	"S_SIGNSPARK9",
	"S_SIGNSPARK10",
	"S_SIGNSPARK11",

	// Drift Sparks
	"S_DRIFTSPARK_A1",
	"S_DRIFTSPARK_A2",
	"S_DRIFTSPARK_A3",
	"S_DRIFTSPARK_B1",
	"S_DRIFTSPARK_C1",
	"S_DRIFTSPARK_C2",
	"S_DRIFTSPARK_D1",
	"S_DRIFTSPARK_D2",

	// Brake drift sparks
	"S_BRAKEDRIFT",

	// Brake dust
	"S_BRAKEDUST1",
	"S_BRAKEDUST2",

	// Drift Smoke
	"S_DRIFTDUST1",
	"S_DRIFTDUST2",
	"S_DRIFTDUST3",
	"S_DRIFTDUST4",

	// Drift Sparkles
	"S_DRIFTWARNSPARK1",
	"S_DRIFTWARNSPARK2",
	"S_DRIFTWARNSPARK3",
	"S_DRIFTWARNSPARK4",

	// Drift electricity
	"S_DRIFTELECTRICITY",
	"S_DRIFTELECTRICSPARK",

	// Fast lines
	"S_FASTLINE1",
	"S_FASTLINE2",
	"S_FASTLINE3",
	"S_FASTLINE4",
	"S_FASTLINE5",

	// Fast dust release
	"S_FASTDUST1",
	"S_FASTDUST2",
	"S_FASTDUST3",
	"S_FASTDUST4",
	"S_FASTDUST5",
	"S_FASTDUST6",
	"S_FASTDUST7",

	// Drift boost effect
	"S_DRIFTEXPLODE1",
	"S_DRIFTEXPLODE2",
	"S_DRIFTEXPLODE3",
	"S_DRIFTEXPLODE4",
	"S_DRIFTEXPLODE5",
	"S_DRIFTEXPLODE6",
	"S_DRIFTEXPLODE7",
	"S_DRIFTEXPLODE8",

	// Drift boost clip
	"S_DRIFTCLIPA1",
	"S_DRIFTCLIPA2",
	"S_DRIFTCLIPA3",
	"S_DRIFTCLIPA4",
	"S_DRIFTCLIPA5",
	"S_DRIFTCLIPA6",
	"S_DRIFTCLIPA7",
	"S_DRIFTCLIPA8",
	"S_DRIFTCLIPA9",
	"S_DRIFTCLIPA10",
	"S_DRIFTCLIPA11",
	"S_DRIFTCLIPA12",
	"S_DRIFTCLIPA13",
	"S_DRIFTCLIPA14",
	"S_DRIFTCLIPA15",
	"S_DRIFTCLIPA16",
	"S_DRIFTCLIPB1",
	"S_DRIFTCLIPB2",
	"S_DRIFTCLIPB3",
	"S_DRIFTCLIPB4",
	"S_DRIFTCLIPB5",
	"S_DRIFTCLIPB6",
	"S_DRIFTCLIPB7",
	"S_DRIFTCLIPB8",

	// Drift boost clip spark
	"S_DRIFTCLIPSPARK",

	// Sneaker boost effect
	"S_BOOSTFLAME",
	"S_BOOSTSMOKESPAWNER",
	"S_BOOSTSMOKE1",
	"S_BOOSTSMOKE2",
	"S_BOOSTSMOKE3",
	"S_BOOSTSMOKE4",
	"S_BOOSTSMOKE5",
	"S_BOOSTSMOKE6",

	// Sneaker Fire Trail
	"S_KARTFIRE1",
	"S_KARTFIRE2",
	"S_KARTFIRE3",
	"S_KARTFIRE4",
	"S_KARTFIRE5",
	"S_KARTFIRE6",
	"S_KARTFIRE7",
	"S_KARTFIRE8",

	// Angel Island Drift Strat Dust (what a mouthful!)
	"S_KARTAIZDRIFTSTRAT",

	// Invincibility Sparks
	"S_KARTINVULN1",
	"S_KARTINVULN2",
	"S_KARTINVULN3",
	"S_KARTINVULN4",
	"S_KARTINVULN5",
	"S_KARTINVULN6",
	"S_KARTINVULN7",
	"S_KARTINVULN8",
	"S_KARTINVULN9",
	"S_KARTINVULN10",
	"S_KARTINVULN11",
	"S_KARTINVULN12",

	"S_KARTINVULNB1",
	"S_KARTINVULNB2",
	"S_KARTINVULNB3",
	"S_KARTINVULNB4",
	"S_KARTINVULNB5",
	"S_KARTINVULNB6",
	"S_KARTINVULNB7",
	"S_KARTINVULNB8",
	"S_KARTINVULNB9",
	"S_KARTINVULNB10",
	"S_KARTINVULNB11",
	"S_KARTINVULNB12",

	// Invincibility flash overlay
	"S_INVULNFLASH1",
	"S_INVULNFLASH2",
	"S_INVULNFLASH3",
	"S_INVULNFLASH4",

	"S_KARTINVLINES1",
	"S_KARTINVLINES2",
	"S_KARTINVLINES3",
	"S_KARTINVLINES4",
	"S_KARTINVLINES5",
	"S_KARTINVLINES6",
	"S_KARTINVLINES7",
	"S_KARTINVLINES8",
	"S_KARTINVLINES9",
	"S_KARTINVLINES10",
	"S_KARTINVLINES11",
	"S_KARTINVLINES12",
	"S_KARTINVLINES13",
	"S_KARTINVLINES14",
	"S_KARTINVLINES15",

	// Wipeout dust trail
	"S_WIPEOUTTRAIL1",
	"S_WIPEOUTTRAIL2",
	"S_WIPEOUTTRAIL3",
	"S_WIPEOUTTRAIL4",
	"S_WIPEOUTTRAIL5",
	"S_WIPEOUTTRAIL6",
	"S_WIPEOUTTRAIL7",
	"S_WIPEOUTTRAIL8",
	"S_WIPEOUTTRAIL9",
	"S_WIPEOUTTRAIL10",
	"S_WIPEOUTTRAIL11",

	// Rocket sneaker
	"S_ROCKETSNEAKER_L",
	"S_ROCKETSNEAKER_R",
	"S_ROCKETSNEAKER_LVIBRATE",
	"S_ROCKETSNEAKER_RVIBRATE",

	//{ Eggman Monitor
	"S_EGGMANITEM1",
	"S_EGGMANITEM2",
	"S_EGGMANITEM3",
	"S_EGGMANITEM4",
	"S_EGGMANITEM5",
	"S_EGGMANITEM6",
	"S_EGGMANITEM7",
	"S_EGGMANITEM8",
	"S_EGGMANITEM9",
	"S_EGGMANITEM10",
	"S_EGGMANITEM11",
	"S_EGGMANITEM12",
	"S_EGGMANITEM_DEAD",
	//}

	// Banana
	"S_BANANA",
	"S_BANANA_DEAD",

	"S_BANANA_SPARK",
	"S_BANANA_SPARK2",
	"S_BANANA_SPARK3",
	"S_BANANA_SPARK4",

	//{ Orbinaut
	"S_ORBINAUT1",
	"S_ORBINAUT2",
	"S_ORBINAUT3",
	"S_ORBINAUT4",
	"S_ORBINAUT5",
	"S_ORBINAUT6",
	"S_ORBINAUT_DEAD",
	"S_ORBINAUT_SHIELD1",
	"S_ORBINAUT_SHIELD2",
	"S_ORBINAUT_SHIELD3",
	"S_ORBINAUT_SHIELD4",
	"S_ORBINAUT_SHIELD5",
	"S_ORBINAUT_SHIELD6",
	"S_ORBINAUT_SHIELDDEAD",
	//}
	//{ Jawz
	"S_JAWZ1",
	"S_JAWZ2",
	"S_JAWZ3",
	"S_JAWZ4",
	"S_JAWZ5",
	"S_JAWZ6",
	"S_JAWZ7",
	"S_JAWZ8",
	"S_JAWZ_SHIELD1",
	"S_JAWZ_SHIELD2",
	"S_JAWZ_SHIELD3",
	"S_JAWZ_SHIELD4",
	"S_JAWZ_SHIELD5",
	"S_JAWZ_SHIELD6",
	"S_JAWZ_SHIELD7",
	"S_JAWZ_SHIELD8",
	"S_JAWZ_DEAD1",
	"S_JAWZ_DEAD2",
	//}

	"S_PLAYERRETICULE", // Player reticule

	// Special Stage Mine
	"S_SSMINE1",
	"S_SSMINE2",
	"S_SSMINE3",
	"S_SSMINE4",
	"S_SSMINE_SHIELD1",
	"S_SSMINE_SHIELD2",
	"S_SSMINE_AIR1",
	"S_SSMINE_AIR2",
	"S_SSMINE_DEPLOY1",
	"S_SSMINE_DEPLOY2",
	"S_SSMINE_DEPLOY3",
	"S_SSMINE_DEPLOY4",
	"S_SSMINE_DEPLOY5",
	"S_SSMINE_DEPLOY6",
	"S_SSMINE_DEPLOY7",
	"S_SSMINE_DEPLOY8",
	"S_SSMINE_DEPLOY9",
	"S_SSMINE_DEPLOY10",
	"S_SSMINE_DEPLOY11",
	"S_SSMINE_DEPLOY12",
	"S_SSMINE_DEPLOY13",
	"S_SSMINE_EXPLODE",
	"S_SSMINE_EXPLODE2",

	// New explosion
	"S_QUICKBOOM1",
	"S_QUICKBOOM2",
	"S_QUICKBOOM3",
	"S_QUICKBOOM4",
	"S_QUICKBOOM5",
	"S_QUICKBOOM6",
	"S_QUICKBOOM7",
	"S_QUICKBOOM8",
	"S_QUICKBOOM9",
	"S_QUICKBOOM10",

	"S_SLOWBOOM1",
	"S_SLOWBOOM2",
	"S_SLOWBOOM3",
	"S_SLOWBOOM4",
	"S_SLOWBOOM5",
	"S_SLOWBOOM6",
	"S_SLOWBOOM7",
	"S_SLOWBOOM8",
	"S_SLOWBOOM9",
	"S_SLOWBOOM10",

	// Land Mine
	"S_LANDMINE",
	"S_LANDMINE_EXPLODE",

	// Drop Target
	"S_DROPTARGET",
	"S_DROPTARGET_SPIN",

	// Ballhog
	"S_BALLHOG1",
	"S_BALLHOG2",
	"S_BALLHOG3",
	"S_BALLHOG4",
	"S_BALLHOG5",
	"S_BALLHOG6",
	"S_BALLHOG7",
	"S_BALLHOG8",
	"S_BALLHOG_DEAD",
	"S_BALLHOGBOOM1",
	"S_BALLHOGBOOM2",
	"S_BALLHOGBOOM3",
	"S_BALLHOGBOOM4",
	"S_BALLHOGBOOM5",
	"S_BALLHOGBOOM6",
	"S_BALLHOGBOOM7",
	"S_BALLHOGBOOM8",
	"S_BALLHOGBOOM9",
	"S_BALLHOGBOOM10",
	"S_BALLHOGBOOM11",
	"S_BALLHOGBOOM12",
	"S_BALLHOGBOOM13",
	"S_BALLHOGBOOM14",
	"S_BALLHOGBOOM15",
	"S_BALLHOGBOOM16",

	// Self-Propelled Bomb - just an explosion for now...
	"S_SPB1",
	"S_SPB2",
	"S_SPB3",
	"S_SPB4",
	"S_SPB5",
	"S_SPB6",
	"S_SPB7",
	"S_SPB8",
	"S_SPB9",
	"S_SPB10",
	"S_SPB11",
	"S_SPB12",
	"S_SPB13",
	"S_SPB14",
	"S_SPB15",
	"S_SPB16",
	"S_SPB17",
	"S_SPB18",
	"S_SPB19",
	"S_SPB20",
	"S_SPB_DEAD",

	// Juicebox for SPB
	"S_MANTA1",
	"S_MANTA2",

	// Lightning Shield
	"S_LIGHTNINGSHIELD1",
	"S_LIGHTNINGSHIELD2",
	"S_LIGHTNINGSHIELD3",
	"S_LIGHTNINGSHIELD4",
	"S_LIGHTNINGSHIELD5",
	"S_LIGHTNINGSHIELD6",
	"S_LIGHTNINGSHIELD7",
	"S_LIGHTNINGSHIELD8",
	"S_LIGHTNINGSHIELD9",
	"S_LIGHTNINGSHIELD10",
	"S_LIGHTNINGSHIELD11",
	"S_LIGHTNINGSHIELD12",
	"S_LIGHTNINGSHIELD13",
	"S_LIGHTNINGSHIELD14",
	"S_LIGHTNINGSHIELD15",
	"S_LIGHTNINGSHIELD16",
	"S_LIGHTNINGSHIELD17",
	"S_LIGHTNINGSHIELD18",
	"S_LIGHTNINGSHIELD19",
	"S_LIGHTNINGSHIELD20",
	"S_LIGHTNINGSHIELD21",
	"S_LIGHTNINGSHIELD22",
	"S_LIGHTNINGSHIELD23",
	"S_LIGHTNINGSHIELD24",

	// Bubble Shield
	"S_BUBBLESHIELD1",
	"S_BUBBLESHIELD2",
	"S_BUBBLESHIELD3",
	"S_BUBBLESHIELD4",
	"S_BUBBLESHIELD5",
	"S_BUBBLESHIELD6",
	"S_BUBBLESHIELD7",
	"S_BUBBLESHIELD8",
	"S_BUBBLESHIELD9",
	"S_BUBBLESHIELD10",
	"S_BUBBLESHIELD11",
	"S_BUBBLESHIELD12",
	"S_BUBBLESHIELD13",
	"S_BUBBLESHIELD14",
	"S_BUBBLESHIELD15",
	"S_BUBBLESHIELD16",
	"S_BUBBLESHIELD17",
	"S_BUBBLESHIELD18",
	"S_BUBBLESHIELDBLOWUP",
	"S_BUBBLESHIELDTRAP1",
	"S_BUBBLESHIELDTRAP2",
	"S_BUBBLESHIELDTRAP3",
	"S_BUBBLESHIELDTRAP4",
	"S_BUBBLESHIELDTRAP5",
	"S_BUBBLESHIELDTRAP6",
	"S_BUBBLESHIELDTRAP7",
	"S_BUBBLESHIELDTRAP8",
	"S_BUBBLESHIELDWAVE1",
	"S_BUBBLESHIELDWAVE2",
	"S_BUBBLESHIELDWAVE3",
	"S_BUBBLESHIELDWAVE4",
	"S_BUBBLESHIELDWAVE5",
	"S_BUBBLESHIELDWAVE6",

	// Flame Shield
	"S_FLAMESHIELD1",
	"S_FLAMESHIELD2",
	"S_FLAMESHIELD3",
	"S_FLAMESHIELD4",
	"S_FLAMESHIELD5",
	"S_FLAMESHIELD6",
	"S_FLAMESHIELD7",
	"S_FLAMESHIELD8",
	"S_FLAMESHIELD9",
	"S_FLAMESHIELD10",
	"S_FLAMESHIELD11",
	"S_FLAMESHIELD12",
	"S_FLAMESHIELD13",
	"S_FLAMESHIELD14",
	"S_FLAMESHIELD15",
	"S_FLAMESHIELD16",
	"S_FLAMESHIELD17",
	"S_FLAMESHIELD18",

	"S_FLAMESHIELDDASH1",
	"S_FLAMESHIELDDASH2",
	"S_FLAMESHIELDDASH3",
	"S_FLAMESHIELDDASH4",
	"S_FLAMESHIELDDASH5",
	"S_FLAMESHIELDDASH6",
	"S_FLAMESHIELDDASH7",
	"S_FLAMESHIELDDASH8",
	"S_FLAMESHIELDDASH9",
	"S_FLAMESHIELDDASH10",
	"S_FLAMESHIELDDASH11",
	"S_FLAMESHIELDDASH12",

	"S_FLAMESHIELDDASH2_UNDERLAY",
	"S_FLAMESHIELDDASH5_UNDERLAY",
	"S_FLAMESHIELDDASH8_UNDERLAY",
	"S_FLAMESHIELDDASH11_UNDERLAY",

	"S_FLAMESHIELDPAPER",
	"S_FLAMESHIELDLINE1",
	"S_FLAMESHIELDLINE2",
	"S_FLAMESHIELDLINE3",
	"S_FLAMESHIELDFLASH",

	// Marble Garden Zone Spinning Top
	"S_GARDENTOP_FLOATING",
	"S_GARDENTOP_SINKING1",
	"S_GARDENTOP_SINKING2",
	"S_GARDENTOP_SINKING3",
	"S_GARDENTOP_DEAD",
	"S_GARDENTOPSPARK",
	"S_GARDENTOPARROW",

	// Caked-Up Booty-Sheet Ghost
	"S_HYUDORO",
	"S_HYUDORO_RETURNING",

	// Grow
	"S_GROW_PARTICLE",

	// Shrink
	"S_SHRINK_POHBEE",
	"S_SHRINK_POHBEE2",
	"S_SHRINK_POHBEE3",
	"S_SHRINK_POHBEE4",
	"S_SHRINK_POHBEE5",
	"S_SHRINK_POHBEE6",
	"S_SHRINK_POHBEE7",
	"S_SHRINK_POHBEE8",

	"S_SHRINK_CHAIN",

	"S_SHRINK_GUN",
	"S_SHRINK_GUN_OVERLAY",

	"S_SHRINK_LASER",
	"S_SHRINK_PARTICLE",

	// The legend
	"S_SINK",
	"S_SINK_SHIELD",
	"S_SINKTRAIL1",
	"S_SINKTRAIL2",
	"S_SINKTRAIL3",

	// Battle Mode bumper
	"S_BATTLEBUMPER1",
	"S_BATTLEBUMPER2",
	"S_BATTLEBUMPER3",

	"S_BATTLEBUMPER_EXCRYSTALA1",
	"S_BATTLEBUMPER_EXCRYSTALA2",
	"S_BATTLEBUMPER_EXCRYSTALA3",
	"S_BATTLEBUMPER_EXCRYSTALA4",

	"S_BATTLEBUMPER_EXCRYSTALB1",
	"S_BATTLEBUMPER_EXCRYSTALB2",
	"S_BATTLEBUMPER_EXCRYSTALB3",
	"S_BATTLEBUMPER_EXCRYSTALB4",

	"S_BATTLEBUMPER_EXCRYSTALC1",
	"S_BATTLEBUMPER_EXCRYSTALC2",
	"S_BATTLEBUMPER_EXCRYSTALC3",
	"S_BATTLEBUMPER_EXCRYSTALC4",

	"S_BATTLEBUMPER_EXSHELLA1",
	"S_BATTLEBUMPER_EXSHELLA2",

	"S_BATTLEBUMPER_EXSHELLB1",
	"S_BATTLEBUMPER_EXSHELLB2",

	"S_BATTLEBUMPER_EXSHELLC1",
	"S_BATTLEBUMPER_EXSHELLC2",

	"S_BATTLEBUMPER_EXDEBRIS1",
	"S_BATTLEBUMPER_EXDEBRIS2",

	"S_BATTLEBUMPER_EXBLAST1",
	"S_BATTLEBUMPER_EXBLAST2",
	"S_BATTLEBUMPER_EXBLAST3",
	"S_BATTLEBUMPER_EXBLAST4",
	"S_BATTLEBUMPER_EXBLAST5",
	"S_BATTLEBUMPER_EXBLAST6",
	"S_BATTLEBUMPER_EXBLAST7",
	"S_BATTLEBUMPER_EXBLAST8",
	"S_BATTLEBUMPER_EXBLAST9",
	"S_BATTLEBUMPER_EXBLAST10",

	// Tripwire
	"S_TRIPWIREBOOST_TOP",
	"S_TRIPWIREBOOST_BOTTOM",
	"S_TRIPWIREBOOST_BLAST_TOP",
	"S_TRIPWIREBOOST_BLAST_BOTTOM",

	"S_SMOOTHLANDING",

	"S_TRICKINDICATOR_OVERLAY",
	"S_TRICKINDICATOR_UNDERLAY",
	"S_TRICKINDICATOR_OVERLAY_ARROW",
	"S_TRICKINDICATOR_UNDERLAY_ARROW",
	"S_TRICKINDICATOR_UNDERLAY_ARROW2",

	"S_SIDETRICK",
	"S_BACKTRICK",
	"S_FORWARDTRICK",

	// DEZ Ring Shooter
	"S_TIREGRABBER",
	"S_RINGSHOOTER_SIDE",
	"S_RINGSHOOTER_NIPPLES",
	"S_RINGSHOOTER_SCREEN",
	"S_RINGSHOOTER_NUMBERBACK",
	"S_RINGSHOOTER_NUMBERFRONT",
	"S_RINGSHOOTER_FACE",

	// DEZ respawn laser
	"S_DEZLASER",
	"S_DEZLASER_TRAIL1",
	"S_DEZLASER_TRAIL2",
	"S_DEZLASER_TRAIL3",
	"S_DEZLASER_TRAIL4",
	"S_DEZLASER_TRAIL5",

	// 1.0 Kart Decoratives
	"S_FLAYM1",
	"S_FLAYM2",
	"S_FLAYM3",
	"S_FLAYM4",
	"S_PALMTREE2",
	"S_PURPLEFLOWER1",
	"S_PURPLEFLOWER2",
	"S_YELLOWFLOWER1",
	"S_YELLOWFLOWER2",
	"S_PLANT2",
	"S_PLANT3",
	"S_PLANT4",

	// Chaotix Big Ring
	"S_BIGRING01",
	"S_BIGRING02",
	"S_BIGRING03",
	"S_BIGRING04",
	"S_BIGRING05",
	"S_BIGRING06",
	"S_BIGRING07",
	"S_BIGRING08",
	"S_BIGRING09",
	"S_BIGRING10",
	"S_BIGRING11",
	"S_BIGRING12",

	// Ark Arrows
	"S_ARKARROW_0",
	"S_ARKARROW_1",
	"S_ARKARROW_2",
	"S_ARKARROW_3",
	"S_ARKARROW_4",
	"S_ARKARROW_5",
	"S_ARKARROW_6",
	"S_ARKARROW_7",
	"S_ARKARROW_8",
	"S_ARKARROW_9",
	"S_ARKARROW_10",
	"S_ARKARROW_11",
	"S_ARKARROW_12",
	"S_ARKARROW_13",
	"S_ARKARROW_14",
	"S_ARKARROW_15",
	"S_ARKARROW_16",
	"S_ARKARROW_17",
	"S_ARKARROW_18",
	"S_ARKARROW_19",
	"S_ARKARROW_20",
	"S_ARKARROW_21",
	"S_ARKARROW_22",
	"S_ARKARROW_23",
	"S_ARKARROW_24",
	"S_ARKARROW_25",
	"S_ARKARROW_26",
	"S_ARKARROW_27",
	"S_ARKARROW_28",
	"S_ARKARROW_29",
	"S_ARKARROW_30",
	"S_ARKARROW_31",
	"S_ARKARROW_32",
	"S_ARKARROW_33",
	"S_ARKARROW_34",
	"S_ARKARROW_35",
	"S_ARKARROW_36",
	"S_ARKARROW_37",
	"S_ARKARROW_38",
	"S_ARKARROW_39",
	"S_ARKARROW_40",
	"S_ARKARROW_41",

	"S_BUMP1",
	"S_BUMP2",
	"S_BUMP3",

	"S_FLINGENERGY1",
	"S_FLINGENERGY2",
	"S_FLINGENERGY3",

	"S_CLASH1",
	"S_CLASH2",
	"S_CLASH3",
	"S_CLASH4",
	"S_CLASH5",
	"S_CLASH6",

	"S_FIREDITEM1",
	"S_FIREDITEM2",
	"S_FIREDITEM3",
	"S_FIREDITEM4",

	"S_INSTASHIELDA1", // No damage instashield effect
	"S_INSTASHIELDA2",
	"S_INSTASHIELDA3",
	"S_INSTASHIELDA4",
	"S_INSTASHIELDA5",
	"S_INSTASHIELDA6",
	"S_INSTASHIELDA7",
	"S_INSTASHIELDB1",
	"S_INSTASHIELDB2",
	"S_INSTASHIELDB3",
	"S_INSTASHIELDB4",
	"S_INSTASHIELDB5",
	"S_INSTASHIELDB6",
	"S_INSTASHIELDB7",

	"S_POWERCLASH", // Invinc/Grow no damage collide VFX
	"S_GUARDBREAK", // Guard break

	"S_PLAYERBOMB1", // Player bomb overlay
	"S_PLAYERBOMB2",
	"S_PLAYERBOMB3",
	"S_PLAYERBOMB4",
	"S_PLAYERBOMB5",
	"S_PLAYERBOMB6",
	"S_PLAYERBOMB7",
	"S_PLAYERBOMB8",
	"S_PLAYERBOMB9",
	"S_PLAYERBOMB10",
	"S_PLAYERBOMB11",
	"S_PLAYERBOMB12",
	"S_PLAYERBOMB13",
	"S_PLAYERBOMB14",
	"S_PLAYERBOMB15",
	"S_PLAYERBOMB16",
	"S_PLAYERBOMB17",
	"S_PLAYERBOMB18",
	"S_PLAYERBOMB19",
	"S_PLAYERBOMB20",

	"S_PLAYERITEM1", // Player item overlay
	"S_PLAYERITEM2",
	"S_PLAYERITEM3",
	"S_PLAYERITEM4",
	"S_PLAYERITEM5",
	"S_PLAYERITEM6",
	"S_PLAYERITEM7",
	"S_PLAYERITEM8",
	"S_PLAYERITEM9",
	"S_PLAYERITEM10",
	"S_PLAYERITEM11",
	"S_PLAYERITEM12",

	"S_PLAYERFAKE1", // Player fake overlay
	"S_PLAYERFAKE2",
	"S_PLAYERFAKE3",
	"S_PLAYERFAKE4",
	"S_PLAYERFAKE5",
	"S_PLAYERFAKE6",
	"S_PLAYERFAKE7",
	"S_PLAYERFAKE8",
	"S_PLAYERFAKE9",
	"S_PLAYERFAKE10",
	"S_PLAYERFAKE11",
	"S_PLAYERFAKE12",

	"S_KARMAWHEEL", // Karma player wheels

	"S_BATTLEPOINT1A", // Battle point indicators
	"S_BATTLEPOINT1B",
	"S_BATTLEPOINT1C",
	"S_BATTLEPOINT1D",
	"S_BATTLEPOINT1E",
	"S_BATTLEPOINT1F",
	"S_BATTLEPOINT1G",
	"S_BATTLEPOINT1H",
	"S_BATTLEPOINT1I",

	"S_BATTLEPOINT2A",
	"S_BATTLEPOINT2B",
	"S_BATTLEPOINT2C",
	"S_BATTLEPOINT2D",
	"S_BATTLEPOINT2E",
	"S_BATTLEPOINT2F",
	"S_BATTLEPOINT2G",
	"S_BATTLEPOINT2H",
	"S_BATTLEPOINT2I",

	"S_BATTLEPOINT3A",
	"S_BATTLEPOINT3B",
	"S_BATTLEPOINT3C",
	"S_BATTLEPOINT3D",
	"S_BATTLEPOINT3E",
	"S_BATTLEPOINT3F",
	"S_BATTLEPOINT3G",
	"S_BATTLEPOINT3H",
	"S_BATTLEPOINT3I",

	// Lightning shield use stuff;
	"S_KSPARK1",	// Sparkling Radius
	"S_KSPARK2",
	"S_KSPARK3",
	"S_KSPARK4",
	"S_KSPARK5",
	"S_KSPARK6",
	"S_KSPARK7",
	"S_KSPARK8",
	"S_KSPARK9",
	"S_KSPARK10",
	"S_KSPARK11",
	"S_KSPARK12",
	"S_KSPARK13",	// ... that's an awful lot.

	"S_LZIO11",	// Straight lightning bolt
	"S_LZIO12",
	"S_LZIO13",
	"S_LZIO14",
	"S_LZIO15",
	"S_LZIO16",
	"S_LZIO17",
	"S_LZIO18",
	"S_LZIO19",

	"S_LZIO21",	// Straight lightning bolt (flipped)
	"S_LZIO22",
	"S_LZIO23",
	"S_LZIO24",
	"S_LZIO25",
	"S_LZIO26",
	"S_LZIO27",
	"S_LZIO28",
	"S_LZIO29",

	"S_KLIT1",	// Diagonal lightning. No, it not being straight doesn't make it gay.
	"S_KLIT2",
	"S_KLIT3",
	"S_KLIT4",
	"S_KLIT5",
	"S_KLIT6",
	"S_KLIT7",
	"S_KLIT8",
	"S_KLIT9",
	"S_KLIT10",
	"S_KLIT11",
	"S_KLIT12",

	"S_FZEROSMOKE1", // F-Zero NO CONTEST explosion
	"S_FZEROSMOKE2",
	"S_FZEROSMOKE3",
	"S_FZEROSMOKE4",
	"S_FZEROSMOKE5",

	"S_FZEROBOOM1",
	"S_FZEROBOOM2",
	"S_FZEROBOOM3",
	"S_FZEROBOOM4",
	"S_FZEROBOOM5",
	"S_FZEROBOOM6",
	"S_FZEROBOOM7",
	"S_FZEROBOOM8",
	"S_FZEROBOOM9",
	"S_FZEROBOOM10",
	"S_FZEROBOOM11",
	"S_FZEROBOOM12",

	"S_FZSLOWSMOKE1",
	"S_FZSLOWSMOKE2",
	"S_FZSLOWSMOKE3",
	"S_FZSLOWSMOKE4",
	"S_FZSLOWSMOKE5",

	// Dash Rings
	"S_DASHRING_HORIZONTAL",
	"S_DASHRING_30DEGREES",
	"S_DASHRING_60DEGREES",
	"S_DASHRING_VERTICAL",
	"S_DASHRING_HORIZONTAL_FLASH1",
	"S_DASHRING_HORIZONTAL_FLASH2",
	"S_DASHRING_30DEGREES_FLASH1",
	"S_DASHRING_30DEGREES_FLASH2",
	"S_DASHRING_60DEGREES_FLASH1",
	"S_DASHRING_60DEGREES_FLASH2",
	"S_DASHRING_VERTICAL_FLASH1",
	"S_DASHRING_VERTICAL_FLASH2",

	// Adventure Air Booster
	"S_ADVENTUREAIRBOOSTER",
	"S_ADVENTUREAIRBOOSTER_EXHAUST1",
	"S_ADVENTUREAIRBOOSTER_EXHAUST2",
	"S_ADVENTUREAIRBOOSTER_FRAME",
	"S_ADVENTUREAIRBOOSTER_ARROW",

	// Sneaker Panels
	"S_SNEAKERPANEL",
	"S_SNEAKERPANEL_SMALL",
	"S_SNEAKERPANEL_TINY",

	// Marble Zone
	"S_MARBLEFLAMEPARTICLE",
	"S_MARBLETORCH",
	"S_MARBLELIGHT",
	"S_MARBLEBURNER",

	// CD Special Stage
	"S_CDUFO",
	"S_CDUFO_DIE",

	// Rusty Rig
	"S_RUSTYLAMP_ORANGE",
	"S_RUSTYCHAIN",

	// Ports of gardens
	"S_PGTREE",

	// Daytona Speedway
	"S_PINETREE",
	"S_PINETREE_SIDE",

	// Egg Zeppelin
	"S_EZZPROPELLER",
	"S_EZZPROPELLER_BLADE",

	// Desert Palace
	"S_DP_PALMTREE",

	// Aurora Atoll
	"S_AAZTREE_SEG",
	"S_AAZTREE_COCONUT",
	"S_AAZTREE_LEAF",

	// Barren Badlands
	"S_BBZDUST1", // Dust
	"S_BBZDUST2",
	"S_BBZDUST3",
	"S_BBZDUST4",
	"S_FROGGER", // Frog badniks
	"S_FROGGER_ATTACK",
	"S_FROGGER_JUMP",
	"S_FROGTONGUE",
	"S_FROGTONGUE_JOINT",
	"S_ROBRA", // Black cobra badniks
	"S_ROBRA_HEAD",
	"S_ROBRA_JOINT",
	"S_ROBRASHELL_INSIDE",
	"S_ROBRASHELL_OUTSIDE",
	"S_BLUEROBRA", // Blue cobra badniks
	"S_BLUEROBRA_HEAD",
	"S_BLUEROBRA_JOINT",

	// Eerie Grove
	"S_EERIEFOG1",
	"S_EERIEFOG2",
	"S_EERIEFOG3",
	"S_EERIEFOG4",
	"S_EERIEFOG5",

	// Chaos Chute
	"S_SPECIALSTAGEARCH",
	"S_SPECIALSTAGEBOMB",
	"S_SPECIALSTAGEBOMB_DISARM",
	"S_SPECIALSTAGEBOMB_EXPLODE",
	"S_SPECIALSTAGEBOMB_DISAPPEAR",
	"S_SPECIALSTAGEBOMB_FLICKER1",
	"S_SPECIALSTAGEBOMB_FLICKER2",
	"S_SPECIALSTAGEBOMB_FLICKERLOOP",
	"S_SPECIALSTAGEBOMB_RESET",

	// Hanagumi Hall
	"S_HANAGUMIHALL_STEAM",
	"S_ALFONSO",
	"S_SAKURA",
	"S_SUMIRE",
	"S_MARIA",
	"S_IRIS",
	"S_KOHRAN",
	"S_KANNA",
	"S_OGAMI",

	// Dimension Disaster
	"S_DVDTRUMPET",
	"S_DVDSHINE1",
	"S_DVDSHINE2",
	"S_DVDSHINE3",
	"S_DVDSHINE4",
	"S_DVDSHINE5",
	"S_DVDSPARK1",
	"S_DVDSPARK2",
	"S_DVDSPARK3",

	"S_SUNBEAMPALM_STEM",
	"S_SUNBEAMPALM_LEAF",

	"S_KARMAFIREWORK1",
	"S_KARMAFIREWORK2",
	"S_KARMAFIREWORK3",
	"S_KARMAFIREWORK4",
	"S_KARMAFIREWORKTRAIL",

	// Opaque smoke version, to prevent lag
	"S_OPAQUESMOKE1",
	"S_OPAQUESMOKE2",
	"S_OPAQUESMOKE3",
	"S_OPAQUESMOKE4",
	"S_OPAQUESMOKE5",

	"S_FOLLOWERBUBBLE_FRONT",
	"S_FOLLOWERBUBBLE_BACK",

	"S_GCHAOIDLE",
	"S_GCHAOFLY",
	"S_GCHAOSAD1",
	"S_GCHAOSAD2",
	"S_GCHAOSAD3",
	"S_GCHAOSAD4",
	"S_GCHAOHAPPY1",
	"S_GCHAOHAPPY2",
	"S_GCHAOHAPPY3",
	"S_GCHAOHAPPY4",

	"S_CHEESEIDLE",
	"S_CHEESEFLY",
	"S_CHEESESAD1",
	"S_CHEESESAD2",
	"S_CHEESESAD3",
	"S_CHEESESAD4",
	"S_CHEESEHAPPY1",
	"S_CHEESEHAPPY2",
	"S_CHEESEHAPPY3",
	"S_CHEESEHAPPY4",

	"S_RINGDEBT",
	"S_RINGSPARKS1",
	"S_RINGSPARKS2",
	"S_RINGSPARKS3",
	"S_RINGSPARKS4",
	"S_RINGSPARKS5",
	"S_RINGSPARKS6",
	"S_RINGSPARKS7",
	"S_RINGSPARKS8",
	"S_RINGSPARKS9",
	"S_RINGSPARKS10",
	"S_RINGSPARKS11",
	"S_RINGSPARKS12",
	"S_RINGSPARKS13",
	"S_RINGSPARKS14",
	"S_RINGSPARKS15",

	"S_GAINAX_TINY",
	"S_GAINAX_HUGE",
	"S_GAINAX_MID1",
	"S_GAINAX_MID2",

	"S_DRAFTDUST1",
	"S_DRAFTDUST2",
	"S_DRAFTDUST3",
	"S_DRAFTDUST4",
	"S_DRAFTDUST5",

	"S_TIREGREASE",

	"S_OVERTIME_BULB1",
	"S_OVERTIME_BULB2",
	"S_OVERTIME_LASER",
	"S_OVERTIME_CENTER",

	"S_BATTLECAPSULE_SIDE1",
	"S_BATTLECAPSULE_SIDE2",
	"S_BATTLECAPSULE_TOP",
	"S_BATTLECAPSULE_BUTTON",
	"S_BATTLECAPSULE_SUPPORT",
	"S_BATTLECAPSULE_SUPPORTFLY",

	"S_WAYPOINTORB",
	"S_WAYPOINTSPLAT",
	"S_EGOORB",

	"S_WATERTRAIL1",
	"S_WATERTRAIL2",
	"S_WATERTRAIL3",
	"S_WATERTRAIL4",
	"S_WATERTRAIL5",
	"S_WATERTRAIL6",
	"S_WATERTRAIL7",
	"S_WATERTRAIL8",
	"S_WATERTRAILUNDERLAY1",
	"S_WATERTRAILUNDERLAY2",
	"S_WATERTRAILUNDERLAY3",
	"S_WATERTRAILUNDERLAY4",
	"S_WATERTRAILUNDERLAY5",
	"S_WATERTRAILUNDERLAY6",
	"S_WATERTRAILUNDERLAY7",
	"S_WATERTRAILUNDERLAY8",

	"S_SPINDASHDUST",
	"S_SPINDASHWIND",

	"S_SOFTLANDING1",
	"S_SOFTLANDING2",
	"S_SOFTLANDING3",
	"S_SOFTLANDING4",
	"S_SOFTLANDING5",

	"S_DOWNLINE1",
	"S_DOWNLINE2",
	"S_DOWNLINE3",
	"S_DOWNLINE4",
	"S_DOWNLINE5",

	"S_HOLDBUBBLE",

	// Finish line beam
	"S_FINISHBEAM1",
	"S_FINISHBEAM2",
	"S_FINISHBEAM3",
	"S_FINISHBEAM4",
	"S_FINISHBEAM5",
	"S_FINISHBEAMEND1",
	"S_FINISHBEAMEND2",

	// Funny Spike
	"S_DEBTSPIKE1",
	"S_DEBTSPIKE2",
	"S_DEBTSPIKE3",
	"S_DEBTSPIKE4",
	"S_DEBTSPIKE5",
	"S_DEBTSPIKE6",
	"S_DEBTSPIKE7",
	"S_DEBTSPIKE8",
	"S_DEBTSPIKE9",
	"S_DEBTSPIKEA",
	"S_DEBTSPIKEB",
	"S_DEBTSPIKEC",
	"S_DEBTSPIKED",
	"S_DEBTSPIKEE",

	// Sparks when driving on stairs
	"S_JANKSPARK1",
	"S_JANKSPARK2",
	"S_JANKSPARK3",
	"S_JANKSPARK4",

	"S_HITLAG_1",
	"S_HITLAG_2",
	"S_HITLAG_3",
	"S_HITLAG_4",
	"S_HITLAG_5",
	"S_HITLAG_6",
	"S_HITLAG_8",
	"S_HITLAG_9",
	"S_HITLAG_10",


	// Broly Ki Orb
	"S_BROLY1",
	"S_BROLY2",

	"S_SPECIAL_UFO_POD",
	"S_SPECIAL_UFO_OVERLAY",
	"S_SPECIAL_UFO_GLASS",
	"S_SPECIAL_UFO_GLASS_UNDER",
	"S_SPECIAL_UFO_ARM",
	"S_SPECIAL_UFO_STEM",

	"S_GACHABOM",
	"S_GACHABOM_DEAD",

	"S_GACHABOM_EXPLOSION_1",
	"S_GACHABOM_EXPLOSION_2",
	"S_GACHABOM_EXPLOSION_3A",
	"S_GACHABOM_EXPLOSION_3B",
	"S_GACHABOM_EXPLOSION_4",
	"S_GACHABOM_WAITING",
	"S_GACHABOM_RETURNING",

	"S_SUPER_FLICKY",

	"S_BATTLEUFO",
	"S_BATTLEUFO_LEG",
	"S_BATTLEUFO_DIE",
	"S_BATTLEUFO_BEAM1",
	"S_BATTLEUFO_BEAM2",

	"S_POWERUP_AURA",

	"S_CHECKPOINT",
	"S_CHECKPOINT_ARM",
	"S_CHECKPOINT_ORB_DEAD",
	"S_CHECKPOINT_ORB_LIVE",
	"S_CHECKPOINT_SPARK1",
	"S_CHECKPOINT_SPARK2",
	"S_CHECKPOINT_SPARK3",
	"S_CHECKPOINT_SPARK4",
	"S_CHECKPOINT_SPARK5",
	"S_CHECKPOINT_SPARK6",
	"S_CHECKPOINT_SPARK7",
	"S_CHECKPOINT_SPARK8",
	"S_CHECKPOINT_SPARK9",
	"S_CHECKPOINT_SPARK10",
	"S_CHECKPOINT_SPARK11",

	"S_RIDEROID",
	"S_RIDEROID_ICON",

	"S_EGGBALL",

	"S_DLZHOVER",
	"S_DLZROCKET_L",
	"S_DLZROCKET_R",

	"S_WPZFOUNTAIN",
	"S_WPZFOUNTAINANIM",
	"S_KURAGEN",
	"S_KURAGENBOMB",

	"S_BALLSWITCH_BALL",
	"S_BALLSWITCH_BALL_ACTIVE",
	"S_BALLSWITCH_PAD",
	"S_BALLSWITCH_PAD_ACTIVE",

	"S_SPIKEDTARGET",
	"S_SPIKEDLENS",

	"S_BLENDEYE_MAIN",
	"S_BLENDEYE_MAIN_LAUNCHED",
	"S_BLENDEYE_EYE",
	"S_BLENDEYE_EYE_FLASH",
	"S_BLENDEYE_GLASS",
	"S_BLENDEYE_GLASS_STRESS",

	"S_BLENDEYE_SHIELD",
	"S_BLENDEYE_SHIELD_L",
	"S_BLENDEYE_SHIELD_R",
	"S_BLENDEYE_SHIELD_BUSTED",
	"S_BLENDEYE_SHIELD_BUSTED_L",
	"S_BLENDEYE_SHIELD_BUSTED_R",

	"S_BLENDEYE_EGGBEATER_EXTEND_1",
	"S_BLENDEYE_EGGBEATER_EXTEND_2",
	"S_BLENDEYE_EGGBEATER",
	"S_BLENDEYE_EGGBEATER_SPIN",

	"S_BLENDEYE_FLAME",

	"S_BLENDEYE_GENERATOR",
	"S_BLENDEYE_GENERATOR_BUSTED_L",
	"S_BLENDEYE_GENERATOR_BUSTED_R",

	"S_BLENDEYE_PUYO_SPAWN_1",
	"S_BLENDEYE_PUYO_SPAWN_2",
	"S_BLENDEYE_PUYO_SPAWN_3",
	"S_BLENDEYE_PUYO",
	"S_BLENDEYE_PUYO_LAND_1",
	"S_BLENDEYE_PUYO_LAND_2",
	"S_BLENDEYE_PUYO_LAND_3",
	"S_BLENDEYE_PUYO_LAND_4",
	"S_BLENDEYE_PUYO_SHOCK",
	"S_BLENDEYE_PUYO_DIE",
	"S_BLENDEYE_PUYO_DUST",

	"S_AHZCLOUD",

	"S_AGZBULB_BASE",
	"S_AGZBULB_NEUTRAL",
	"S_AGZBULB_ANIM1",
	"S_AGZBULB_ANIM2",
	"S_AGZBULB_ANIM3",
	"S_AGTR",
	"S_AGFL",
	"S_AGFF",
	"S_AGZCLOUD",

	"S_SSZCLOUD",

	"S_MEGABARRIER1",
	"S_MEGABARRIER2",
	"S_MEGABARRIER3",

	"S_GPZ_TREETHING_B",
	"S_GPZ_TREETHING_M",
	"S_GPZ_TREETHING_S",

	// MT_GGZFREEZETHRUSTER
	"S_GGZFREEZETHRUSTER",

	// MT_GGZICEDUST
	"S_GGZICEDUST1",
	"S_GGZICEDUST2",
	"S_GGZICEDUST3",
	"S_GGZICEDUST4",
	"S_GGZICEDUST5",
	"S_GGZICEDUST6",
	"S_GGZICEDUST7",
	"S_GGZICEDUST8",
	"S_GGZICEDUST9",
	"S_GGZICEDUST10",
	"S_GGZICEDUST11",
	"S_GGZPARTICLE11",
	"S_GGZPARTICLE12",
	"S_GGZPARTICLE13",
	"S_GGZPARTICLE14",
	"S_GGZPARTICLE15",
	"S_GGZPARTICLE16",
	"S_GGZPARTICLE17",
	"S_GGZPARTICLE18",
	"S_GGZPARTICLE21",
	"S_GGZPARTICLE22",
	"S_GGZPARTICLE23",
	"S_GGZPARTICLE24",

	"S_GGZICECUBE",

	// MT_THRUSTERPART
	"S_THRUSTERPART",

	// MT_IVOBALL
	"S_IVOBALL",

	"S_SA2_CRATE_DEBRIS",
	"S_SA2_CRATE_DEBRIS_E",
	"S_SA2_CRATE_DEBRIS_F",
	"S_SA2_CRATE_DEBRIS_G",
	"S_SA2_CRATE_DEBRIS_H",
	"S_SA2_CRATE_DEBRIS_METAL",

	"S_ICECAPBLOCK_DEBRIS",
	"S_ICECAPBLOCK_DEBRIS_C",
	"S_ICECAPBLOCK_DEBRIS_D",
	"S_ICECAPBLOCK_DEBRIS_E",
	"S_ICECAPBLOCK_DEBRIS_F",

	// MT_SPEAR
	"S_SPEAR_ROD",
	"S_SPEAR_TIP",
	"S_SPEAR_HILT_FRONT",
	"S_SPEAR_HILT_BACK",
	"S_SPEAR_WALL",

	// MT_BSZLAMP_S
	"S_BLMS",
	"S_BLMM",
	"S_BLML",

	// MT_BSZSLAMP
	"S_BSWL",
	"S_BSWC",

	"S_BETA_PARTICLE_WHEEL",
	"S_BETA_PARTICLE_ICON",
	"S_BETA_PARTICLE_EXPLOSION",

	// MT_AIZ_REDFERN
	"S_AIZFL1",
	"S_AIZFR1",
	"S_AIZFR2",
	"S_AIZTRE",
	"S_AIZFR3",
	"S_AIZDB1",
	"S_AIZDB2",
	"S_AIZDB3",
	"S_AIZDB4",
	"S_AIZDB5",
	"S_AIZDB6",
	"S_AIZDB7",
	"S_AIZDB8",

	// MT_AZROCKS
	"S_AZROCKS",
	"S_AZROCKS_RESPAWN",
	"S_AZROCKS_PARTICLE1",

	// MT_EMROCKS
	"S_EMROCKS",
	"S_EMROCKS_RESPAWN",
	"S_EMROCKS_PARTICLE1",
	"S_EMROCKS_PARTICLE2",

	// MT_EMFAUCET
	"S_EMFAUCET",

	// MT_EMFAUCET_DRIP
	"S_EMROCKS_DRIP",

	// MT_EMFAUCET_PARTICLE
	"S_EMFAUCET_PARTICLE",

	// MT_TRICKBALLOON_RED
	"S_TRICKBALLOON_RED1",
	"S_TRICKBALLOON_RED2",
	"S_TRICKBALLOON_RED_POP1",
	"S_TRICKBALLOON_RED_POP2",
	"S_TRICKBALLOON_RED_POP3",
	"S_TRICKBALLOON_RED_GONE",
	"S_TRICKBALLOON_RED_INFLATE1",
	"S_TRICKBALLOON_RED_INFLATE2",
	"S_TRICKBALLOON_RED_INFLATE3",
	"S_TRICKBALLOON_RED_INFLATE4",
	"S_TRICKBALLOON_RED_INFLATE5",

	// MT_TRICKBALLOON_RED_POINT
	"S_TRICKBALLOON_RED_POINT1",

	// MT_TRICKBALLOON_YELLOW
	"S_TRICKBALLOON_YELLOW1",
	"S_TRICKBALLOON_YELLOW2",
	"S_TRICKBALLOON_YELLOW_POP1",
	"S_TRICKBALLOON_YELLOW_POP2",
	"S_TRICKBALLOON_YELLOW_POP3",
	"S_TRICKBALLOON_YELLOW_GONE",
	"S_TRICKBALLOON_YELLOW_INFLATE1",
	"S_TRICKBALLOON_YELLOW_INFLATE2",
	"S_TRICKBALLOON_YELLOW_INFLATE3",
	"S_TRICKBALLOON_YELLOW_INFLATE4",
	"S_TRICKBALLOON_YELLOW_INFLATE5",

	// MT_TRICKBALLOON_YELLOW_POINT
	"S_TRICKBALLOON_YELLOW_POINT1",

	// MT_WATERFALLPARTICLESPAWNER
	"S_WATERFALLPARTICLE",

	// Sealed Stars
	// MT_SSCANDLE
	"S_SSCANDLE_INIT",
	"S_SSCANDLE",

	// MT_SSCANDLE_SIDE
	"S_SSCANDLE_SIDE",

	// MT_SSCANDLE_FLAME
	"S_SSCANDLE_FLAME",

	// MT_SS_HOLOGRAM
	"S_HOLOGRAM_BIRD",
	"S_HOLOGRAM_CRAB",
	"S_HOLOGRAM_FISH",
	"S_HOLOGRAM_SQUID",

	// MT_SS_COIN
	"S_SS_COIN",

	// MT_SS_GOBLET
	"S_SS_GOBLET",

	// MT_SS_LAMP
	"S_SS_LAMP",

	// MT_SS_LAMP_BULB
	"S_SS_LAMP_BULB",
	"S_SS_LAMP_AURA",

	// MT_SSWINDOW
	"S_SSWINDOW_INIT",
	"S_SSWINDOW",

	// MT_SSWINDOW_SHINE
	"S_SSWINDOW_SHINE",

	// MT_SSCHAINSOUND
	"S_SSCHAINSOUND",

	// MT_SLSTMACE
	"S_SLSTMACE",

	// MT_SEALEDSTAR_BUMPER
	"S_SEALEDSTAR_BUMPER",
	"S_SEALEDSTAR_BUMPERHIT",

	// MT_SSCHAIN_SPAWNER
	"S_SSCHAIN_SPAWNER_SHATTER",

	// MT_SSCHAIN
	"S_SSCHAIN1",

	// MT_GACHATARGET
	"S_GACHATARGET",
	"S_GACHATARGETSPIN",
	"S_GACHATARGETOK",

	// MT_CABOTRON
	"S_CABOTRON",

	// MT_CABOTRONSTAR
	"S_CABOTRONSTAR",

	// MT_STARSTREAM
	"S_STARSTREAM",

	// MT_SCRIPT_THING
	"S_TALKPOINT",
	"S_TALKPOINT_ORB",

	"S_BADNIK_EXPLOSION_SHOCKWAVE1",
	"S_BADNIK_EXPLOSION_SHOCKWAVE2",
	"S_BADNIK_EXPLOSION1",
	"S_BADNIK_EXPLOSION2",
};

// RegEx to generate this from info.h: ^\tMT_([^,]+), --> \t"MT_\1",
// I am leaving the prefixes solely for clarity to programmers,
// because sadly no one remembers this place while searching for full state names.
const char *const MOBJTYPE_LIST[] = {  // array length left dynamic for sanity testing later.
	"MT_NULL",
	"MT_RAY",
	"MT_UNKNOWN",

	"MT_THOK", // Thok! mobj
	"MT_SHADOW", // Linkdraw Shadow (for invisible objects)
	"MT_PLAYER",
	"MT_KART_LEFTOVER",
	"MT_KART_TIRE",
	"MT_KART_PARTICLE",

	// Generic Boss Items
	"MT_BOSSEXPLODE",
	"MT_SONIC3KBOSSEXPLODE",
	"MT_BOSSFLYPOINT",
	"MT_EGGTRAP",
	"MT_BOSS3WAYPOINT",

	// Collectible Items
	"MT_RING",
	"MT_FLINGRING", // Lost ring
	"MT_DEBTSPIKE", // Ring debt funny spike
	"MT_BLUESPHERE",  // Blue sphere for special stages
	"MT_FLINGBLUESPHERE", // Lost blue sphere
	"MT_EMBLEM",
	"MT_SPRAYCAN",
	"MT_ANCIENTSHRINE",
	"MT_EMERALD",
	"MT_EMERALDSPARK",
	"MT_EMERALDFLARE",
	"MT_PRISONEGGDROP",

	// Springs and others
	"MT_STEAM",
	"MT_BALLOON",

	"MT_YELLOWSPRING",
	"MT_REDSPRING",
	"MT_BLUESPRING",
	"MT_GREYSPRING",
	"MT_POGOSPRING",
	"MT_YELLOWDIAG", // Yellow Diagonal Spring
	"MT_REDDIAG", // Red Diagonal Spring
	"MT_BLUEDIAG", // Blue Diagonal Spring
	"MT_GREYDIAG", // Grey Diagonal Spring
	"MT_YELLOWHORIZ", // Yellow Horizontal Spring
	"MT_REDHORIZ", // Red Horizontal Spring
	"MT_BLUEHORIZ", // Blue Horizontal Spring
	"MT_GREYHORIZ", // Grey Horizontal Spring

	// Interactive Objects
	"MT_BUBBLES", // Bubble source
	"MT_SIGN", // Level end sign
	"MT_SIGN_PIECE",
	"MT_SPIKEBALL", // Spike Ball
	"MT_SPINFIRE",
	"MT_SPIKE",
	"MT_WALLSPIKE",
	"MT_WALLSPIKEBASE",
	"MT_CHEATCHECK",
	"MT_BLASTEXECUTOR",
	"MT_CANNONLAUNCHER",
	"MT_CANNONBALL", // Cannonball
	"MT_CANNONBALLDECOR", // Decorative/still cannonball

	// Greenflower Scenery
	"MT_GFZFLOWER1",
	"MT_GFZFLOWER2",
	"MT_GFZFLOWER3",

	"MT_BLUEBERRYBUSH",
	"MT_BERRYBUSH",
	"MT_BUSH",

	// Trees (both GFZ and misc)
	"MT_GFZTREE",
	"MT_GFZBERRYTREE",
	"MT_GFZCHERRYTREE",
	"MT_CHECKERTREE",
	"MT_CHECKERSUNSETTREE",
	"MT_FHZTREE", // Frozen Hillside
	"MT_FHZPINKTREE",
	"MT_POLYGONTREE",
	"MT_BUSHTREE",
	"MT_BUSHREDTREE",
	"MT_SPRINGTREE",

	// Techno Hill Scenery
	"MT_THZFLOWER1",
	"MT_THZFLOWER2",
	"MT_THZFLOWER3",
	"MT_THZTREE", // Steam whistle tree/bush
	"MT_THZTREEBRANCH", // branch of said tree
	"MT_ALARM",

	// Deep Sea Scenery
	"MT_GARGOYLE", // Deep Sea Gargoyle
	"MT_BIGGARGOYLE", // Deep Sea Gargoyle (Big)
	"MT_SEAWEED", // DSZ Seaweed
	"MT_WATERDRIP", // Dripping Water source
	"MT_WATERDROP", // Water drop from dripping water
	"MT_CORAL1", // Coral
	"MT_CORAL2",
	"MT_CORAL3",
	"MT_CORAL4",
	"MT_CORAL5",
	"MT_BLUECRYSTAL", // Blue Crystal
	"MT_KELP", // Kelp
	"MT_ANIMALGAETOP", // Animated algae top
	"MT_ANIMALGAESEG", // Animated algae segment
	"MT_DSZSTALAGMITE", // Deep Sea 1 Stalagmite
	"MT_DSZ2STALAGMITE", // Deep Sea 2 Stalagmite
	"MT_LIGHTBEAM", // DSZ Light beam

	// Castle Eggman Scenery
	"MT_CHAIN", // CEZ Chain
	"MT_FLAME", // Flame (has corona)
	"MT_FLAMEPARTICLE",
	"MT_EGGSTATUE", // Eggman Statue
	"MT_MACEPOINT", // Mace rotation point
	"MT_CHAINMACEPOINT", // Combination of chains and maces point
	"MT_CHAINPOINT", // Mace chain
	"MT_HIDDEN_SLING", // Spin mace chain (activatable)
	"MT_FIREBARPOINT", // Firebar
	"MT_CUSTOMMACEPOINT", // Custom mace
	"MT_SMALLMACECHAIN", // Small Mace Chain
	"MT_BIGMACECHAIN", // Big Mace Chain
	"MT_SMALLMACE", // Small Mace
	"MT_BIGMACE", // Big Mace
	"MT_SMALLGRABCHAIN", // Small Grab Chain
	"MT_BIGGRABCHAIN", // Big Grab Chain
	"MT_SMALLFIREBAR", // Small Firebar
	"MT_BIGFIREBAR", // Big Firebar
	"MT_CEZFLOWER", // Flower
	"MT_CEZPOLE1", // Pole (with red banner)
	"MT_CEZPOLE2", // Pole (with blue banner)
	"MT_CEZBANNER1", // Banner (red)
	"MT_CEZBANNER2", // Banner (blue)
	"MT_PINETREE", // Pine Tree
	"MT_CEZBUSH1", // Bush 1
	"MT_CEZBUSH2", // Bush 2
	"MT_CANDLE", // Candle
	"MT_CANDLEPRICKET", // Candle pricket
	"MT_FLAMEHOLDER", // Flame holder
	"MT_FIRETORCH", // Fire torch
	"MT_WAVINGFLAG1", // Waving flag (red)
	"MT_WAVINGFLAG2", // Waving flag (blue)
	"MT_WAVINGFLAGSEG1", // Waving flag segment (red)
	"MT_WAVINGFLAGSEG2", // Waving flag segment (blue)
	"MT_CRAWLASTATUE", // Crawla statue
	"MT_BRAMBLES", // Brambles

	// Arid Canyon Scenery
	"MT_BIGTUMBLEWEED",
	"MT_LITTLETUMBLEWEED",
	"MT_CACTI1", // Tiny Red Flower Cactus
	"MT_CACTI2", // Small Red Flower Cactus
	"MT_CACTI3", // Tiny Blue Flower Cactus
	"MT_CACTI4", // Small Blue Flower Cactus
	"MT_CACTI5", // Prickly Pear
	"MT_CACTI6", // Barrel Cactus
	"MT_CACTI7", // Tall Barrel Cactus
	"MT_CACTI8", // Armed Cactus
	"MT_CACTI9", // Ball Cactus
	"MT_CACTI10", // Tiny Cactus
	"MT_CACTI11", // Small Cactus
	"MT_CACTITINYSEG", // Tiny Cactus Segment
	"MT_CACTISMALLSEG", // Small Cactus Segment
	"MT_ARIDSIGN_CAUTION", // Caution Sign
	"MT_ARIDSIGN_CACTI", // Cacti Sign
	"MT_ARIDSIGN_SHARPTURN", // Sharp Turn Sign
	"MT_OILLAMP",
	"MT_TNTBARREL",
	"MT_TNTDUST",
	"MT_PROXIMITYTNT",
	"MT_DUSTDEVIL",
	"MT_DUSTLAYER",
	"MT_ARIDDUST",

	// Red Volcano Scenery
	"MT_FLAMEJET",
	"MT_VERTICALFLAMEJET",
	"MT_FLAMEJETFLAME",

	"MT_FJSPINAXISA", // Counter-clockwise
	"MT_FJSPINAXISB", // Clockwise

	"MT_FLAMEJETFLAMEB", // Blade's flame

	"MT_LAVAFALL",
	"MT_LAVAFALL_LAVA",
	"MT_LAVAFALLROCK",

	"MT_BIGFERNLEAF",
	"MT_BIGFERN",
	"MT_JUNGLEPALM",
	"MT_TORCHFLOWER",
	"MT_WALLVINE_LONG",
	"MT_WALLVINE_SHORT",

	// Dark City Scenery

	// Egg Rock Scenery

	// Stalagmites
	"MT_STALAGMITE0",
	"MT_STALAGMITE1",
	"MT_STALAGMITE2",
	"MT_STALAGMITE3",
	"MT_STALAGMITE4",
	"MT_STALAGMITE5",
	"MT_STALAGMITE6",
	"MT_STALAGMITE7",
	"MT_STALAGMITE8",
	"MT_STALAGMITE9",

	// Christmas Scenery
	"MT_XMASPOLE",
	"MT_CANDYCANE",
	"MT_SNOWMAN",    // normal
	"MT_SNOWMANHAT", // with hat + scarf
	"MT_LAMPPOST1",  // normal
	"MT_LAMPPOST2",  // with snow
	"MT_HANGSTAR",
	"MT_MISTLETOE",
	// Xmas GFZ bushes
	"MT_XMASBLUEBERRYBUSH",
	"MT_XMASBERRYBUSH",
	"MT_XMASBUSH",
	// FHZ
	"MT_FHZICE1",
	"MT_FHZICE2",

	// Halloween Scenery
	// Pumpkins
	"MT_JACKO1",
	"MT_JACKO2",
	"MT_JACKO3",
	// Dr Seuss Trees
	"MT_HHZTREE_TOP",
	"MT_HHZTREE_PART",
	// Misc
	"MT_HHZSHROOM",
	"MT_HHZGRASS",
	"MT_HHZTENTACLE1",
	"MT_HHZTENTACLE2",
	"MT_HHZSTALAGMITE_TALL",
	"MT_HHZSTALAGMITE_SHORT",

	// Botanic Serenity scenery
	"MT_BSZTALLFLOWER_RED",
	"MT_BSZTALLFLOWER_PURPLE",
	"MT_BSZTALLFLOWER_BLUE",
	"MT_BSZTALLFLOWER_CYAN",
	"MT_BSZTALLFLOWER_YELLOW",
	"MT_BSZTALLFLOWER_ORANGE",
	"MT_BSZFLOWER_RED",
	"MT_BSZFLOWER_PURPLE",
	"MT_BSZFLOWER_BLUE",
	"MT_BSZFLOWER_CYAN",
	"MT_BSZFLOWER_YELLOW",
	"MT_BSZFLOWER_ORANGE",
	"MT_BSZSHORTFLOWER_RED",
	"MT_BSZSHORTFLOWER_PURPLE",
	"MT_BSZSHORTFLOWER_BLUE",
	"MT_BSZSHORTFLOWER_CYAN",
	"MT_BSZSHORTFLOWER_YELLOW",
	"MT_BSZSHORTFLOWER_ORANGE",
	"MT_BSZTULIP_RED",
	"MT_BSZTULIP_PURPLE",
	"MT_BSZTULIP_BLUE",
	"MT_BSZTULIP_CYAN",
	"MT_BSZTULIP_YELLOW",
	"MT_BSZTULIP_ORANGE",
	"MT_BSZCLUSTER_RED",
	"MT_BSZCLUSTER_PURPLE",
	"MT_BSZCLUSTER_BLUE",
	"MT_BSZCLUSTER_CYAN",
	"MT_BSZCLUSTER_YELLOW",
	"MT_BSZCLUSTER_ORANGE",
	"MT_BSZBUSH_RED",
	"MT_BSZBUSH_PURPLE",
	"MT_BSZBUSH_BLUE",
	"MT_BSZBUSH_CYAN",
	"MT_BSZBUSH_YELLOW",
	"MT_BSZBUSH_ORANGE",
	"MT_BSZVINE_RED",
	"MT_BSZVINE_PURPLE",
	"MT_BSZVINE_BLUE",
	"MT_BSZVINE_CYAN",
	"MT_BSZVINE_YELLOW",
	"MT_BSZVINE_ORANGE",
	"MT_BSZSHRUB",
	"MT_BSZCLOVER",
	"MT_BIG_PALMTREE_TRUNK",
	"MT_BIG_PALMTREE_TOP",
	"MT_PALMTREE_TRUNK",
	"MT_PALMTREE_TOP",

	// Misc scenery
	"MT_DBALL",
	"MT_EGGSTATUE2",

	"MT_SUPERSPARK", // Super Sonic Spark

	// Flickies
	"MT_FLICKY_01", // Bluebird
	"MT_FLICKY_01_CENTER",
	"MT_FLICKY_02", // Rabbit
	"MT_FLICKY_02_CENTER",
	"MT_FLICKY_03", // Chicken
	"MT_FLICKY_03_CENTER",
	"MT_FLICKY_04", // Seal
	"MT_FLICKY_04_CENTER",
	"MT_FLICKY_05", // Pig
	"MT_FLICKY_05_CENTER",
	"MT_FLICKY_06", // Chipmunk
	"MT_FLICKY_06_CENTER",
	"MT_FLICKY_07", // Penguin
	"MT_FLICKY_07_CENTER",
	"MT_FLICKY_08", // Fish
	"MT_FLICKY_08_CENTER",
	"MT_FLICKY_09", // Ram
	"MT_FLICKY_09_CENTER",
	"MT_FLICKY_10", // Puffin
	"MT_FLICKY_10_CENTER",
	"MT_FLICKY_11", // Cow
	"MT_FLICKY_11_CENTER",
	"MT_FLICKY_12", // Rat
	"MT_FLICKY_12_CENTER",
	"MT_FLICKY_13", // Bear
	"MT_FLICKY_13_CENTER",
	"MT_FLICKY_14", // Dove
	"MT_FLICKY_14_CENTER",
	"MT_FLICKY_15", // Cat
	"MT_FLICKY_15_CENTER",
	"MT_FLICKY_16", // Canary
	"MT_FLICKY_16_CENTER",
	"MT_SECRETFLICKY_01", // Spider
	"MT_SECRETFLICKY_01_CENTER",
	"MT_SECRETFLICKY_02", // Bat
	"MT_SECRETFLICKY_02_CENTER",
	"MT_SEED",

	// Environmental Effects
	"MT_RAIN", // Rain
	"MT_SNOWFLAKE", // Snowflake
	"MT_BLIZZARDSNOW", // Blizzard Snowball
	"MT_SPLISH", // Water splish!
	"MT_LAVASPLISH", // Lava splish!
	"MT_SMOKE",
	"MT_SMALLBUBBLE", // small bubble
	"MT_MEDIUMBUBBLE", // medium bubble
	"MT_EXTRALARGEBUBBLE", // extra large bubble
	"MT_WATERZAP",
	"MT_SPINDUST", // Spindash dust
	"MT_PARTICLE",
	"MT_PARTICLEGEN", // For fans, etc.

	// Game Indicators
	"MT_DROWNNUMBERS", // Drowning Timer

	// Ambient Sounds
	"MT_AMBIENT",

	"MT_CORK",
	"MT_LHRT",

	// NiGHTS Stuff
	"MT_AXIS",
	"MT_AXISTRANSFER",
	"MT_AXISTRANSFERLINE",
	"MT_HOOP",
	"MT_HOOPCOLLIDE", // Collision detection for NiGHTS hoops
	"MT_HOOPCENTER", // Center of a hoop
	"MT_EGGCAPSULE",

	"MT_SMASHINGSPIKEBALL",

	// Utility Objects
	"MT_TELEPORTMAN",
	"MT_ALTVIEWMAN",
	"MT_CRUMBLEOBJ", // Sound generator for crumbling platform
	"MT_TUBEWAYPOINT",
	"MT_PUSH",
	"MT_GHOST",
	"MT_FAKESHADOW",
	"MT_OVERLAY",
	"MT_ANGLEMAN",
	"MT_POLYANCHOR",
	"MT_POLYSPAWN",
	"MT_MINIMAPBOUND",

	// Skybox objects
	"MT_SKYBOX",

	// Debris
	"MT_SPARK", //spark, only used for debugging, actually
	"MT_EXPLODE", // Robot Explosion
	"MT_UWEXPLODE", // Underwater Explosion
	"MT_DUST",
	"MT_ROCKSPAWNER",
	"MT_FALLINGROCK",
	"MT_ROCKCRUMBLE1",
	"MT_ROCKCRUMBLE2",
	"MT_ROCKCRUMBLE3",
	"MT_ROCKCRUMBLE4",
	"MT_ROCKCRUMBLE5",
	"MT_ROCKCRUMBLE6",
	"MT_ROCKCRUMBLE7",
	"MT_ROCKCRUMBLE8",
	"MT_ROCKCRUMBLE9",
	"MT_ROCKCRUMBLE10",
	"MT_ROCKCRUMBLE11",
	"MT_ROCKCRUMBLE12",
	"MT_ROCKCRUMBLE13",
	"MT_ROCKCRUMBLE14",
	"MT_ROCKCRUMBLE15",
	"MT_ROCKCRUMBLE16",

	// Level debris
	"MT_GFZDEBRIS",
	"MT_BRICKDEBRIS",
	"MT_WOODDEBRIS",

	// SRB2kart
	"MT_RANDOMITEM",
	"MT_SPHEREBOX",
	"MT_FLOATINGITEM",
	"MT_GOTPOWERUP",
	"MT_ITEMCAPSULE",
	"MT_ITEMCAPSULE_PART",
	"MT_MONITOR",
	"MT_MONITOR_PART",
	"MT_MONITOR_SHARD",
	"MT_MAGICIANBOX",
	"MT_WAVEDASH",

	"MT_INSTAWHIP",
	"MT_INSTAWHIP_RECHARGE",
	"MT_INSTAWHIP_REJECT",
	"MT_BLOCKRING",
	"MT_BLOCKBODY",

	"MT_CHARGEAURA",
	"MT_CHARGEFALL",
	"MT_CHARGEFLICKER",
	"MT_CHARGESPARK",
	"MT_CHARGERELEASE",
	"MT_CHARGEEXTRA",

	"MT_SERVANTHAND",

	"MT_HORNCODE",

	"MT_SIGNSPARKLE",

	"MT_FASTLINE",
	"MT_FASTDUST",
	"MT_DRIFTEXPLODE",
	"MT_DRIFTCLIP",
	"MT_DRIFTCLIPSPARK",
	"MT_BOOSTFLAME",
	"MT_BOOSTSMOKE",
	"MT_SNEAKERTRAIL",
	"MT_AIZDRIFTSTRAT",
	"MT_SPARKLETRAIL",
	"MT_INVULNFLASH",
	"MT_WIPEOUTTRAIL",
	"MT_DRIFTSPARK",
	"MT_BRAKEDRIFT",
	"MT_BRAKEDUST",
	"MT_DRIFTDUST",
	"MT_ITEM_DEBRIS",
	"MT_ITEM_DEBRIS_CLOUD_SPAWNER",
	"MT_DRIFTELECTRICITY",
	"MT_DRIFTELECTRICSPARK",
	"MT_JANKSPARK",
	"MT_HITLAG",

	"MT_ROCKETSNEAKER", // Rocket sneakers

	"MT_EGGMANITEM", // Eggman items
	"MT_EGGMANITEM_SHIELD",

	"MT_BANANA", // Banana Stuff
	"MT_BANANA_SHIELD",
	"MT_BANANA_SPARK",

	"MT_ORBINAUT", // Orbinaut stuff
	"MT_ORBINAUT_SHIELD",

	"MT_JAWZ", // Jawz stuff
	"MT_JAWZ_SHIELD",

	"MT_PLAYERRETICULE", // Jawz reticule

	"MT_SSMINE_SHIELD", // Special Stage Mine stuff
	"MT_SSMINE",

	"MT_SMOLDERING", // New explosion
	"MT_BOOMEXPLODE",
	"MT_BOOMPARTICLE",

	"MT_LANDMINE", // Land Mine

	"MT_DROPTARGET", // Drop Target
	"MT_DROPTARGET_SHIELD",
	"MT_DROPTARGET_MORPH",

	"MT_BALLHOG", // Ballhog
	"MT_BALLHOGBOOM",

	"MT_SPB", // Self-Propelled Bomb
	"MT_SPBEXPLOSION",
	"MT_MANTARING", // Juicebox for SPB

	"MT_LIGHTNINGSHIELD", // Shields
	"MT_BUBBLESHIELD",
	"MT_FLAMESHIELD",
	"MT_FLAMESHIELDUNDERLAY",
	"MT_FLAMESHIELDPAPER",
	"MT_BUBBLESHIELDTRAP",
	"MT_GARDENTOP",
	"MT_GARDENTOPSPARK",
	"MT_GARDENTOPARROW",

	"MT_HYUDORO",
	"MT_HYUDORO_CENTER",

	"MT_GROW_PARTICLE",

	"MT_SHRINK_POHBEE",
	"MT_SHRINK_GUN",
	"MT_SHRINK_CHAIN",
	"MT_SHRINK_LASER",
	"MT_SHRINK_PARTICLE",

	"MT_SINK", // Kitchen Sink Stuff
	"MT_SINK_SHIELD",
	"MT_SINKTRAIL",

	"MT_GACHABOM",
	"MT_GACHABOM_REBOUND",

	"MT_DUELBOMB", // Duel mode bombs

	"MT_BATTLEBUMPER", // Battle Mode bumper
	"MT_BATTLEBUMPER_DEBRIS",
	"MT_BATTLEBUMPER_BLAST",

	"MT_TRIPWIREBOOST",

	"MT_SMOOTHLANDING",
	"MT_TRICKINDICATOR",
	"MT_SIDETRICK",
	"MT_FORWARDTRICK",

	"MT_TIREGRABBER",
	"MT_RINGSHOOTER",
	"MT_RINGSHOOTER_PART",
	"MT_RINGSHOOTER_SCREEN",

	"MT_DEZLASER",

	"MT_WAYPOINT",
	"MT_WAYPOINT_RISER",
	"MT_WAYPOINT_ANCHOR",

	"MT_BOTHINT",

	"MT_RANDOMAUDIENCE",

	"MT_FLAYM",

	"MT_PALMTREE2",
	"MT_PURPLEFLOWER1",
	"MT_PURPLEFLOWER2",
	"MT_YELLOWFLOWER1",
	"MT_YELLOWFLOWER2",
	"MT_PLANT2",
	"MT_PLANT3",
	"MT_PLANT4",

	"MT_BIGRING",

	"MT_ARKARROW", // Ark Arrows

	"MT_BUMP",

	"MT_FLINGENERGY",

	"MT_ITEMCLASH",

	"MT_FIREDITEM",

	"MT_INSTASHIELDA",
	"MT_INSTASHIELDB",

	"MT_POWERCLASH", // Invinc/Grow no damage clash VFX
	"MT_GUARDBREAK", // Guard break

	"MT_KARMAHITBOX",
	"MT_KARMAWHEEL",

	"MT_BATTLEPOINT",

	"MT_FZEROBOOM",

	// Dash Rings
	"MT_DASHRING",
	"MT_RAINBOWDASHRING",

	// Adventure Air Booster
	"MT_ADVENTUREAIRBOOSTER",
	"MT_ADVENTUREAIRBOOSTER_HITBOX",
	"MT_ADVENTUREAIRBOOSTER_PART",

	// Sneaker Panels
	"MT_SNEAKERPANEL",
	"MT_SNEAKERPANELSPAWNER",

	// Marble Zone
	"MT_MARBLEFLAMEPARTICLE",
	"MT_MARBLETORCH",
	"MT_MARBLELIGHT",
	"MT_MARBLEBURNER",

	// CD Special Stage
	"MT_CDUFO",

	// Rusty Rig
	"MT_RUSTYLAMP_ORANGE",
	"MT_RUSTYCHAIN",

	"MT_PGTREE",

	// Daytona Speedway
	"MT_DAYTONAPINETREE",
	"MT_DAYTONAPINETREE_SIDE",

	// Egg Zeppelin
	"MT_EZZPROPELLER",
	"MT_EZZPROPELLER_BLADE",

	// Desert Palace
	"MT_DP_PALMTREE",

	// Aurora Atoll
	"MT_AAZTREE_HELPER",
	"MT_AAZTREE_SEG",
	"MT_AAZTREE_COCONUT",
	"MT_AAZTREE_LEAF",

	// Barren Badlands
	"MT_BBZDUST",
	"MT_FROGGER",
	"MT_FROGTONGUE",
	"MT_FROGTONGUE_JOINT",
	"MT_ROBRA",
	"MT_ROBRA_HEAD",
	"MT_ROBRA_JOINT",
	"MT_BLUEROBRA",
	"MT_BLUEROBRA_HEAD",
	"MT_BLUEROBRA_JOINT",

	// Eerie Grove
	"MT_EERIEFOG",
	"MT_EERIEFOGGEN",

	// Chaos Chute
	"MT_SPECIALSTAGEARCH",
	"MT_SPECIALSTAGEBOMB",

	// Hanagumi Hall
	"MT_HANAGUMIHALL_STEAM",
	"MT_HANAGUMIHALL_NPC",

	// Dimension Disaster
	"MT_DVDTRUMPET",
	"MT_DVDPARTICLE",

	"MT_SUNBEAMPALM_STEM",
	"MT_SUNBEAMPALM_LEAF",

	"MT_KARMAFIREWORK",
	"MT_RINGSPARKS",
	"MT_GAINAX",
	"MT_DRAFTDUST",
	"MT_SPBDUST",
	"MT_TIREGREASE",

	"MT_OVERTIME_PARTICLE",
	"MT_OVERTIME_CENTER",

	"MT_BATTLECAPSULE",
	"MT_BATTLECAPSULE_PIECE",

	"MT_FOLLOWER",
	"MT_FOLLOWERBUBBLE_FRONT",
	"MT_FOLLOWERBUBBLE_BACK",

	"MT_WATERTRAIL",
	"MT_WATERTRAILUNDERLAY",

	"MT_SPINDASHDUST",
	"MT_SPINDASHWIND",
	"MT_SOFTLANDING",
	"MT_DOWNLINE",
	"MT_HOLDBUBBLE",

	"MT_PAPERITEMSPOT",

	"MT_BEAMPOINT",

	"MT_BROLY",

	"MT_SPECIAL_UFO",
	"MT_SPECIAL_UFO_PIECE",

	"MT_LOOPENDPOINT",
	"MT_LOOPCENTERPOINT",

	"MT_SUPER_FLICKY",
	"MT_SUPER_FLICKY_CONTROLLER",

	"MT_BATTLEUFO_SPAWNER",
	"MT_BATTLEUFO",
	"MT_BATTLEUFO_LEG",
	"MT_BATTLEUFO_BEAM",

	"MT_POWERUP_AURA",

	"MT_CHECKPOINT_END",
	"MT_SCRIPT_THING",
	"MT_SCRIPT_THING_ORB",

	"MT_RIDEROID",
	"MT_RIDEROIDNODE",

	"MT_LSZ_BUNGEE",
	"MT_LSZ_EGGBALLSPAWNER",
	"MT_LSZ_EGGBALL",

	"MT_DLZ_HOVER",
	"MT_DLZ_ROCKET",
	"MT_DLZ_RINGVACCUM",
	"MT_DLZ_SUCKEDRING",

	"MT_WATERPALACETURBINE",
	"MT_WATERPALACEBUBBLE",
	"MT_WATERPALACEFOUNTAIN",
	"MT_KURAGEN",
	"MT_KURAGENBOMB",

	"MT_BALLSWITCH_BALL",
	"MT_BALLSWITCH_PAD",

	"MT_BOSSARENACENTER",
	"MT_SPIKEDTARGET",

	"MT_BLENDEYE_MAIN",
	"MT_BLENDEYE_EYE",
	"MT_BLENDEYE_GLASS",
	"MT_BLENDEYE_SHIELD",
	"MT_BLENDEYE_EGGBEATER",
	"MT_BLENDEYE_GENERATOR",
	"MT_BLENDEYE_PUYO",
	"MT_BLENDEYE_PUYO_DUST",
	"MT_BLENDEYE_PUYO_DUST_COFFEE",

	"MT_AHZ_CLOUD",
	"MT_AHZ_CLOUDCLUSTER",

	"MT_AGZ_BULB",
	"MT_AGZ_BULB_PART",
	"MT_AGZ_TREE",
	"MT_AGZ_AGFL",
	"MT_AGZ_AGFF",
	"MT_AGZ_CLOUD",
	"MT_AGZ_CLOUDCLUSTER",
	
	"MT_SSZ_CLOUD",
	"MT_SSZ_CLOUDCLUSTER",

	"MT_MEGABARRIER",

	"MT_SEASAW_VISUAL",
	"MT_DLZ_SEASAW_SPAWN",
	"MT_DLZ_SEASAW_HITBOX",
	"MT_GPZ_SEASAW_SPAWN",
	"MT_GPZ_SEASAW_HITBOX",

	"MT_GPZ_TREETHING_B",
	"MT_GPZ_TREETHING_M",
	"MT_GPZ_TREETHING_S",

	"MT_GGZFREEZETHRUSTER",
	"MT_GGZICEDUST",
	"MT_GGZICECUBE",
	"MT_GGZICESHATTER",
	"MT_SIDEWAYSFREEZETHRUSTER",
	"MT_THRUSTERPART",

	"MT_IVOBALL",
	"MT_PATROLIVOBALL",
	"MT_AIRIVOBALL",

	"MT_BOX_SIDE",
	"MT_BOX_DEBRIS",
	"MT_SA2_CRATE",
	"MT_ICECAPBLOCK",

	"MT_SPEAR",
	"MT_SPEARVISUAL",
	"MT_BSZLAMP_S",
	"MT_BSZLAMP_M",
	"MT_BSZLAMP_L",
	"MT_BSZSLAMP",
	"MT_BSZSLCHA",

	"MT_BETA_EMITTER",
	"MT_BETA_PARTICLE_PHYSICAL",
	"MT_BETA_PARTICLE_VISUAL",
	"MT_BETA_PARTICLE_EXPLOSION",

	"MT_AIZ_REDFERN",
	"MT_AIZ_FERN1",
	"MT_AIZ_FERN2",
	"MT_AIZ_TREE",
	"MT_AIZ_FERN3",
	"MT_AIZ_DDB",

	"MT_AZROCKS",
	"MT_AZROCKS_PARTICLE",

	"MT_EMROCKS",
	"MT_EMROCKS_PARTICLE",

	"MT_EMFAUCET",
	"MT_EMFAUCET_DRIP",
	"MT_EMFAUCET_PARTICLE",
	"MT_EMRAINGEN",

	"MT_TRICKBALLOON_RED",
	"MT_TRICKBALLOON_RED_POINT",
	"MT_TRICKBALLOON_YELLOW",
	"MT_TRICKBALLOON_YELLOW_POINT",

	"MT_WATERFALLPARTICLESPAWNER",

	"MT_SSCANDLE",
	"MT_SSCANDLE_SIDE",
	"MT_SSCANDLE_FLAME",
	"MT_SS_HOLOGRAM",
	"MT_SS_HOLOGRAM_ROTATOR",
	"MT_SS_COIN",
	"MT_SS_COIN_CLOUD",
	"MT_SS_GOBLET",
	"MT_SS_GOBLET_CLOUD",
	"MT_SS_LAMP",
	"MT_SS_LAMP_BULB",
	"MT_SSWINDOW",
	"MT_SSWINDOW_SHINE",
	"MT_SSCHAINSOUND",
	"MT_SLSTMACE",
	"MT_SEALEDSTAR_BUMPER",
	"MT_SSCHAIN_SPAWNER",
	"MT_SSCHAIN",
	"MT_GACHATARGET",
	"MT_CABOTRON",
	"MT_CABOTRONSTAR",
	"MT_STARSTREAM",

	"MT_IPULLUP",
	"MT_PULLUPHOOK",
};

const char *const MOBJFLAG_LIST[] = {
	"SPECIAL",
	"SOLID",
	"SHOOTABLE",
	"NOSECTOR",
	"NOBLOCKMAP",
	"PAPERCOLLISION",
	"PUSHABLE",
	"BOSS",
	"SPAWNCEILING",
	"NOGRAVITY",
	"DRAWFROMFARAWAY",
	"SLIDEME",
	"NOCLIP",
	"FLOAT",
	"SLOPE",
	"MISSILE",
	"SPRING",
	"ELEMENTAL",
	"NOTHINK",
	"NOCLIPHEIGHT",
	"ENEMY",
	"SCENERY",
	"PAIN",
	"DONTPUNT",
	"APPLYTERRAIN",
	"NOCLIPTHING",
	"GRENADEBOUNCE",
	"RUNSPAWNFUNC",
	"DONTENCOREMAP",
	"PICKUPFROMBELOW",
	"NOSQUISH",
	"NOHITLAGFORME",
	NULL
};

// \tMF2_(\S+).*// (.+) --> \t"\1", // \2
const char *const MOBJFLAG2_LIST[] = {
	"AXIS",			  // It's a NiGHTS axis! (For faster checking)
	"\x01",			  // free: 1<<1 (name un-matchable)
	"DONTRESPAWN",	  // Don't respawn this object!
	"\x01",			  // free: 1<<3 (name un-matchable)
	"AUTOMATIC",	  // Thrown ring has automatic properties
	"RAILRING",		  // Thrown ring has rail properties
	"BOUNCERING",	  // Thrown ring has bounce properties
	"EXPLOSION",	  // Thrown ring has explosive properties
	"SCATTER",		  // Thrown ring has scatter properties
	"BEYONDTHEGRAVE", // Source of this missile has died and has since respawned.
	"SLIDEPUSH",	  // MF_PUSHABLE that pushes continuously.
	"CLASSICPUSH",    // Drops straight down when object has negative momz.
	"INVERTAIMABLE",  // Flips whether it's targetable by A_LookForEnemies (enemies no, decoys yes)
	"INFLOAT",		  // Floating to a height for a move, don't auto float to target's height.
	"DEBRIS",		  // Splash ring from explosion ring
	"NIGHTSPULL",	  // Attracted from a paraloop
	"JUSTATTACKED",	  // can be pushed by other moving mobjs
	"FIRING",		  // turret fire
	"SUPERFIRE",	  // Firing something with Super Sonic-stopping properties. Or, if mobj has MF_MISSILE, this is the actual fire from it.
	"ALREADYHIT",	  // This object was already damaged THIS tic, resets even during hitlag
	"STRONGBOX",	  // Flag used for "strong" random monitors.
	"OBJECTFLIP",	  // Flag for objects that always have flipped gravity.
	"SKULLFLY",		  // Special handling: skull in flight.
	"FRET",			  // Flashing from a previous hit
	"BOSSNOTRAP",	  // No Egg Trap after boss
	"BOSSFLEE",		  // Boss is fleeing!
	"BOSSDEAD",		  // Boss is dead! (Not necessarily fleeing, if a fleeing point doesn't exist.)
	"AMBUSH",         // Alternate behaviour typically set by MTF_AMBUSH
	"LINKDRAW",       // Draw vissprite of mobj immediately before/after tracer's vissprite (dependent on dispoffset and position)
	"SHIELD",         // Thinker calls P_AddShield/P_ShieldLook (must be partnered with MF_SCENERY to use)
	NULL
};

const char *const MOBJEFLAG_LIST[] = {
	"ONGROUND", // The mobj stands on solid floor (not on another mobj or in air)
	"JUSTHITFLOOR", // The mobj just hit the floor while falling, this is cleared on next frame
	"TOUCHWATER", // The mobj stands in a sector with water, and touches the surface
	"UNDERWATER", // The mobj stands in a sector with water, and his waist is BELOW the water surface
	"JUSTSTEPPEDDOWN", // used for ramp sectors
	"VERTICALFLIP", // Vertically flip sprite/allow upside-down physics
	"GOOWATER", // Goo water
	"TOUCHLAVA", // The mobj is touching a lava block
	"PUSHED", // Mobj was already pushed this tic
	"SPRUNG", // Mobj was already sprung this tic
	"APPLYPMOMZ", // Platform movement
	"TRACERANGLE", // Compute and trigger on mobj angle relative to tracer
	"JUSTBOUNCEDWALL",
	"DAMAGEHITLAG",
	"SLOPELAUNCHED",
	NULL
};

const char *const MAPTHINGFLAG_LIST[4] = {
	"EXTRA", // Extra flag for objects.
	"OBJECTFLIP", // Reverse gravity flag for objects.
	"OBJECTSPECIAL", // Special flag used with certain objects.
	"AMBUSH" // Deaf monsters/do not react to sound.
};

const char *const PLAYERFLAG_LIST[] = {
	"GODMODE",

	"UPDATEMYRESPAWN", // Scripted sequences / fastfall can set this to force a respawn waypoint update

	"AUTOROULETTE", // Item box accessibility

	// Look back VFX has been spawned
	// TODO: Is there a better way to track this?
	"GAINAX",

	// Accessibility and cheats
	"KICKSTARTACCEL", // Accessibility feature: Is accelerate in kickstart mode?
	"POINTME", // An object is calling for my attention (via Obj_PointPlayersToMobj). Unset every frame!

	"CASTSHADOW", // Something is casting a shadow on the player

	"WANTSTOJOIN", // Spectator that wants to join

	"STASIS", // Player is not allowed to move
	"FAULT", // F A U L T
	"ELIMINATED", // Battle-style elimination, no extra penalty
	"NOCONTEST", // Did not finish (last place explosion)
	"LOSTLIFE", // Do not lose life more than once

	"RINGLOCK", // Prevent picking up rings while SPB is locked on

	"ANALOGSTICK", // This player is using an analog joystick
	"TRUSTWAYPOINTS", // Do not activate lap cheat prevention next time finish line distance is updated
	"FREEZEWAYPOINTS", // Skip the next waypoint/finish line distance update
	"AUTORING", // Accessibility: Non-deterministic item box, no manual stop.

	"DRIFTINPUT", // Drifting!
	"GETSPARKS", // Can get sparks
	"DRIFTEND", // Drift has ended, used to adjust character angle after drift
	"BRAKEDRIFT", // Helper for brake-drift spark spawning

	"AIRFAILSAFE", // Whenever or not try the air boost
	"TRICKDELAY", // Prevent tricks until control stick is neutral

	"TUMBLELASTBOUNCE", // One more time for the funny
	"TUMBLESOUND", // Don't play more than once

	"HITFINISHLINE", // Already hit the finish line this tic
	"WRONGWAY", // Moving the wrong way with respect to waypoints?

	"SHRINKME", // "Shrink me" cheat preference
	"SHRINKACTIVE", // "Shrink me" cheat is in effect. (Can't be disabled mid-race)

	"VOID", // Removed from reality! When leaving hitlag, reenable visibility+collision and kill speed.
	"NOFASTFALL", // Has already done ebrake/fastfall behavior for this input. Fastfalling needs a new input to prevent unwanted bounces on unexpected airtime.

	NULL // stop loop here.
};

const char *const GAMETYPERULE_LIST[] = {
	"CIRCUIT",
	"BOTS",

	"BUMPERS",
	"SPHERES",
	"CLOSERPLAYERS",

	"BATTLESTARTS",
	"PAPERITEMS",
	"POWERSTONES",
	"KARMA",
	"ITEMARROWS",

	"CHECKPOINTS",
	"PRISONS",
	"CATCHER",
	"ROLLINGSTART",
	"SPECIALSTART",
	"BOSS",

	"POINTLIMIT",
	"TIMELIMIT",
	"OVERTIME",
	"ENCORE",

	"TEAMS",
	"NOTEAMS",
	"TEAMSTARTS",

	"NOMP",
	"NOCUPSELECT",
	"NOPOSITION",
	NULL
};

// Linedef flags
const char *const ML_LIST[] = {
	"IMPASSABLE",
	"BLOCKPLAYERS",
	"TWOSIDED",
	"DONTPEGTOP",
	"DONTPEGBOTTOM",
	"SKEWTD",
	"NOCLIMB",
	"NOSKEW",
	"MIDPEG",
	"MIDSOLID",
	"WRAPMIDTEX",
	"NETONLY",
	"NONET",
	"BLOCKMONSTERS",
	"NOTBOUNCY",
	"TFERLINE",
	NULL
};

// Sector flags
const char *const MSF_LIST[] = {
	"FLIPSPECIAL_FLOOR",
	"FLIPSPECIAL_CEILING",
	"TRIGGERSPECIAL_TOUCH",
	"TRIGGERSPECIAL_HEADBUMP",
	"TRIGGERLINE_PLANE",
	"TRIGGERLINE_MOBJ",
	"INVERTPRECIP",
	"GRAVITYFLIP",
	"HEATWAVE",
	"NOCLIPCAMERA",
	"RIPPLE_FLOOR",
	"RIPPLE_CEILING",
	"INVERTENCORE",
	"FLATLIGHTING",
	"DIRECTIONLIGHTING",
	NULL
};

// Sector special flags
const char *const SSF_LIST[] = {
	"DELETEITEMS",
	"DOUBLESTEPUP",
	"WINDCURRENT",
	"CONVEYOR",
	"\x01",			  // free (name un-matchable)
	"CHEATCHECKACTIVATOR",
	"EXIT",
	"\x01",			  // free (name un-matchable)
	"\x01",			  // free (name un-matchable)
	"\x01",			  // free (name un-matchable)
	"\x01",			  // free (name un-matchable)
	"FAN",
	"\x01",			  // free (name un-matchable)
	"\x01",			  // free (name un-matchable)
	"ZOOMTUBESTART",
	"ZOOMTUBEEND",
	"\x01",			  // free (name un-matchable)
	"\x01",			  // free (name un-matchable)
	NULL
};

// Sector damagetypes
const char *const SD_LIST[] = {
	"NONE",
	"GENERIC",
	"LAVA",
	"DEATHPIT",
	"INSTAKILL",
	NULL
};

// Sector triggerer
const char *const TO_LIST[] = {
	"PLAYER",
	"ALLPLAYERS",
	"MOBJ",
	NULL
};

const char *COLOR_ENUMS[] = {
	"NONE",			// SKINCOLOR_NONE
	"WHITE",		// SKINCOLOR_WHITE
	"SILVER",		// SKINCOLOR_SILVER
	"GREY",			// SKINCOLOR_GREY
	"NICKEL",		// SKINCOLOR_NICKEL
	"BLACK",		// SKINCOLOR_BLACK
	"SKUNK",		// SKINCOLOR_SKUNK
	"FAIRY",		// SKINCOLOR_FAIRY
	"POPCORN",		// SKINCOLOR_POPCORN
	"ARTICHOKE",	// SKINCOLOR_ARTICHOKE
	"PIGEON",		// SKINCOLOR_PIGEON
	"SEPIA",		// SKINCOLOR_SEPIA
	"BEIGE",		// SKINCOLOR_BEIGE
	"CARAMEL",		// SKINCOLOR_CARAMEL
	"PEACH",		// SKINCOLOR_PEACH
	"BROWN",		// SKINCOLOR_BROWN
	"LEATHER",		// SKINCOLOR_LEATHER
	"PINK",			// SKINCOLOR_PINK
	"ROSE",			// SKINCOLOR_ROSE
	"CINNAMON",		// SKINCOLOR_CINNAMON
	"RUBY",			// SKINCOLOR_RUBY
	"RASPBERRY",	// SKINCOLOR_RASPBERRY
	"RED",			// SKINCOLOR_RED
	"CRIMSON",		// SKINCOLOR_CRIMSON
	"MAROON",		// SKINCOLOR_MAROON
	"LEMONADE",		// SKINCOLOR_LEMONADE
	"SCARLET",		// SKINCOLOR_SCARLET
	"KETCHUP",		// SKINCOLOR_KETCHUP
	"DAWN",			// SKINCOLOR_DAWN
	"SUNSLAM",		// SKINCOLOR_SUNSLAM
	"CREAMSICLE",	// SKINCOLOR_CREAMSICLE
	"ORANGE",		// SKINCOLOR_ORANGE
	"ROSEWOOD",		// SKINCOLOR_ROSEWOOD
	"TANGERINE",	// SKINCOLOR_TANGERINE
	"TAN",			// SKINCOLOR_TAN
	"CREAM",		// SKINCOLOR_CREAM
	"GOLD",			// SKINCOLOR_GOLD
	"ROYAL",		// SKINCOLOR_ROYAL
	"BRONZE",		// SKINCOLOR_BRONZE
	"COPPER",		// SKINCOLOR_COPPER
	"YELLOW",		// SKINCOLOR_YELLOW
	"MUSTARD",		// SKINCOLOR_MUSTARD
	"BANANA",		// SKINCOLOR_BANANA
	"OLIVE",		// SKINCOLOR_OLIVE
	"CROCODILE",	// SKINCOLOR_CROCODILE
	"PERIDOT",		// SKINCOLOR_PERIDOT
	"VOMIT",		// SKINCOLOR_VOMIT
	"GARDEN",		// SKINCOLOR_GARDEN
	"LIME",			// SKINCOLOR_LIME
	"HANDHELD",		// SKINCOLOR_HANDHELD
	"TEA",			// SKINCOLOR_TEA
	"PISTACHIO",	// SKINCOLOR_PISTACHIO
	"MOSS",			// SKINCOLOR_MOSS
	"CAMOUFLAGE",	// SKINCOLOR_CAMOUFLAGE
	"MINT",			// SKINCOLOR_MINT
	"GREEN",		// SKINCOLOR_GREEN
	"PINETREE",		// SKINCOLOR_PINETREE
	"TURTLE",		// SKINCOLOR_TURTLE
	"SWAMP",		// SKINCOLOR_SWAMP
	"DREAM",		// SKINCOLOR_DREAM
	"PLAGUE",		// SKINCOLOR_PLAGUE
	"EMERALD",		// SKINCOLOR_EMERALD
	"ALGAE",		// SKINCOLOR_ALGAE
	"AQUAMARINE",	// SKINCOLOR_AQUAMARINE
	"TURQUOISE",	// SKINCOLOR_TURQUOISE
	"TEAL",			// SKINCOLOR_TEAL
	"ROBIN",		// SKINCOLOR_ROBIN
	"CYAN",			// SKINCOLOR_CYAN
	"JAWZ",			// SKINCOLOR_JAWZ
	"CERULEAN",		// SKINCOLOR_CERULEAN
	"NAVY",			// SKINCOLOR_NAVY
	"PLATINUM",		// SKINCOLOR_PLATINUM
	"SLATE",		// SKINCOLOR_SLATE
	"STEEL",		// SKINCOLOR_STEEL
	"THUNDER",		// SKINCOLOR_THUNDER
	"NOVA",			// SKINCOLOR_NOVA
	"RUST",			// SKINCOLOR_RUST
	"WRISTWATCH",	// SKINCOLOR_WRISTWATCH
	"JET",			// SKINCOLOR_JET
	"SAPPHIRE",		// SKINCOLOR_SAPPHIRE
	"ULTRAMARINE",	// SKINCOLOR_ULTRAMARINE
	"PERIWINKLE",	// SKINCOLOR_PERIWINKLE
	"BLUE",			// SKINCOLOR_BLUE
	"MIDNIGHT",		// SKINCOLOR_MIDNIGHT
	"BLUEBERRY",	// SKINCOLOR_BLUEBERRY
	"THISTLE",		// SKINCOLOR_THISTLE
	"PURPLE",		// SKINCOLOR_PURPLE
	"PASTEL",		// SKINCOLOR_PASTEL
	"MOONSET",		// SKINCOLOR_MOONSET
	"DUSK",			// SKINCOLOR_DUSK
	"VIOLET",		// SKINCOLOR_VIOLET
	"MAGENTA",		// SKINCOLOR_MAGENTA
	"FUCHSIA",		// SKINCOLOR_FUCHSIA
	"TOXIC",		// SKINCOLOR_TOXIC
	"MAUVE",		// SKINCOLOR_MAUVE
	"LAVENDER",		// SKINCOLOR_LAVENDER
	"BYZANTIUM",	// SKINCOLOR_BYZANTIUM
	"POMEGRANATE",	// SKINCOLOR_POMEGRANATE
	"LILAC",		// SKINCOLOR_LILAC
	"BLOSSOM",		// SKINCOLOR_BLOSSOM
	"TAFFY",		// SKINCOLOR_TAFFY

	// Special super colors
	// Super Sonic Yellow
	"SUPERSILVER1",
	"SUPERSILVER2",
	"SUPERSILVER3",
	"SUPERSILVER4",
	"SUPERSILVER5",

	"SUPERRED1",
	"SUPERRED2",
	"SUPERRED3",
	"SUPERRED4",
	"SUPERRED5",

	"SUPERORANGE1",
	"SUPERORANGE2",
	"SUPERORANGE3",
	"SUPERORANGE4",
	"SUPERORANGE5",

	"SUPERGOLD1",
	"SUPERGOLD2",
	"SUPERGOLD3",
	"SUPERGOLD4",
	"SUPERGOLD5",

	"SUPERPERIDOT1",
	"SUPERPERIDOT2",
	"SUPERPERIDOT3",
	"SUPERPERIDOT4",
	"SUPERPERIDOT5",

	"SUPERSKY1",
	"SUPERSKY2",
	"SUPERSKY3",
	"SUPERSKY4",
	"SUPERSKY5",

	"SUPERPURPLE1",
	"SUPERPURPLE2",
	"SUPERPURPLE3",
	"SUPERPURPLE4",
	"SUPERPURPLE5",

	"SUPERRUST1",
	"SUPERRUST2",
	"SUPERRUST3",
	"SUPERRUST4",
	"SUPERRUST5",

	"SUPERTAN1",
	"SUPERTAN2",
	"SUPERTAN3",
	"SUPERTAN4",
	"SUPERTAN5",

	"CHAOSEMERALD1",
	"CHAOSEMERALD2",
	"CHAOSEMERALD3",
	"CHAOSEMERALD4",
	"CHAOSEMERALD5",
	"CHAOSEMERALD6",
	"CHAOSEMERALD7",

	"INVINCFLASH",
	"POSNUM",
	"POSNUM_WIN1",
	"POSNUM_WIN2",
	"POSNUM_WIN3",
	"POSNUM_LOSE1",
	"POSNUM_LOSE2",
	"POSNUM_LOSE3",
	"POSNUM_BEST1",
	"POSNUM_BEST2",
	"POSNUM_BEST3",
	"POSNUM_BEST4",
	"POSNUM_BEST5",
	"POSNUM_BEST6",

	"INTERMISSION1",
	"INTERMISSION2",
	"INTERMISSION3",
};

const char *const KARTHUD_LIST[] = {
	"ITEMBLINK",
	"ITEMBLINKMODE",
	"ROULETTEOFFSET",
	"RINGFRAME",
	"RINGTICS",
	"RINGDELAY",
	"RINGSPBLOCK",
	"LAPANIMATION",
	"LAPHAND",
	"FINISH",
	"FAULT",
	"BOOSTCAM",
	"DESTBOOSTCAM",
	"TIMEOVERCAM",
	"AIRCAM",
	"ENGINESND",
	"VOICES",
	"TAUNTVOICES",
	"TAUNTHORNS",
	"YOUGOTEM",
	"TRICKCOOL"
};

const char *const HUDITEMS_LIST[] = {
	"LIVES",

	"RINGS",
	"RINGSNUM",
	"RINGSNUMTICS",

	"SCORE",
	"SCORENUM",

	"TIME",
	"MINUTES",
	"TIMECOLON",
	"SECONDS",
	"TIMETICCOLON",
	"TICS",

	"SS_TOTALRINGS",

	"GETRINGS",
	"GETRINGSNUM",
	"TIMELEFT",
	"TIMELEFTNUM",
	"TIMEUP",
	"HUNTPICS",
	"POWERUPS"
};

struct int_const_s const INT_CONST[] = {
	// If a mod removes some variables here,
	// please leave the names in-tact and just set
	// the value to 0 or something.

	// integer type limits, from doomtype.h
	// INT64 and UINT64 limits not included, they're too big for most purposes anyway
	// signed
	{"INT8_MIN",INT8_MIN},
	{"INT16_MIN",INT16_MIN},
	{"INT32_MIN",INT32_MIN},
	{"INT8_MAX",INT8_MAX},
	{"INT16_MAX",INT16_MAX},
	{"INT32_MAX",INT32_MAX},
	// unsigned
	{"UINT8_MAX",UINT8_MAX},
	{"UINT16_MAX",UINT16_MAX},
	{"UINT32_MAX",UINT32_MAX},

	// fixed_t constants, from m_fixed.h
	{"FRACUNIT",FRACUNIT},
	{"FU"      ,FRACUNIT},
	{"FRACBITS",FRACBITS},
	{"M_TAU_FIXED",M_TAU_FIXED},

	// doomdef.h constants
	{"TICRATE",TICRATE},
	{"MUSICRATE",MUSICRATE},
	{"RING_DIST",RING_DIST},
	{"PUSHACCEL",PUSHACCEL},
	{"MODVERSION",MODVERSION}, // or what version of the mod this is.
	{"CODEBASE",CODEBASE}, // or what release of SRB2 this is.
	{"NEWTICRATE",NEWTICRATE}, // TICRATE*NEWTICRATERATIO
	{"NEWTICRATERATIO",NEWTICRATERATIO},

	// Special linedef executor tag numbers!
	{"LE_PINCHPHASE",LE_PINCHPHASE}, // A boss entered pinch phase (and, in most cases, is preparing their pinch phase attack!)
	{"LE_ALLBOSSESDEAD",LE_ALLBOSSESDEAD}, // All bosses in the map are dead (Egg capsule raise)
	{"LE_BOSSDEAD",LE_BOSSDEAD}, // A boss in the map died (Chaos mode boss tally)
	{"LE_BOSS4DROP",LE_BOSS4DROP}, // CEZ boss dropped its cage
	{"LE_BRAKVILEATACK",LE_BRAKVILEATACK}, // Brak's doing his LOS attack, oh noes
	{"LE_TURRET",LE_TURRET}, // THZ turret
	{"LE_BRAKPLATFORM",LE_BRAKPLATFORM}, // v2.0 Black Eggman destroys platform
	{"LE_CAPSULE2",LE_CAPSULE2}, // Egg Capsule
	{"LE_CAPSULE1",LE_CAPSULE1}, // Egg Capsule
	{"LE_CAPSULE0",LE_CAPSULE0}, // Egg Capsule
	{"LE_KOOPA",LE_KOOPA}, // Distant cousin to Gay Bowser
	{"LE_AXE",LE_AXE}, // MKB Axe object
	{"LE_PARAMWIDTH",LE_PARAMWIDTH},  // If an object that calls LinedefExecute has a nonzero parameter value, this times the parameter will be subtracted. (Mostly for the purpose of coexisting bosses...)

	/// \todo Get all this stuff into its own sections, maybe. Maybe.

	// Frame settings
	{"FF_FRAMEMASK",FF_FRAMEMASK},
	{"FF_SPR2SUPER",FF_SPR2SUPER},
	{"FF_SPR2ENDSTATE",FF_SPR2ENDSTATE},
	{"FF_SPR2MIDSTART",FF_SPR2MIDSTART},
	{"FF_ANIMATE",FF_ANIMATE},
	{"FF_RANDOMANIM",FF_RANDOMANIM},
	{"FF_GLOBALANIM",FF_GLOBALANIM},
	{"FF_REVERSEANIM",FF_REVERSEANIM},
	{"FF_FULLBRIGHT",FF_FULLBRIGHT},
	{"FF_FULLDARK",FF_FULLDARK},
	{"FF_SEMIBRIGHT",FF_SEMIBRIGHT},
	{"FF_VERTICALFLIP",FF_VERTICALFLIP},
	{"FF_HORIZONTALFLIP",FF_HORIZONTALFLIP},
	{"FF_PAPERSPRITE",FF_PAPERSPRITE},
	{"FF_FLOORSPRITE",FF_FLOORSPRITE},
	{"FF_BLENDMASK",FF_BLENDMASK},
	{"FF_BLENDSHIFT",FF_BLENDSHIFT},
	{"FF_ADD",FF_ADD},
	{"FF_SUBTRACT",FF_SUBTRACT},
	{"FF_REVERSESUBTRACT",FF_REVERSESUBTRACT},
	{"FF_MODULATE",FF_MODULATE},
	{"FF_OVERLAY",FF_OVERLAY},
	{"FF_TRANSMASK",FF_TRANSMASK},
	{"FF_TRANSSHIFT",FF_TRANSSHIFT},
	// new preshifted translucency (used in source)
	{"FF_TRANS10",FF_TRANS10},
	{"FF_TRANS20",FF_TRANS20},
	{"FF_TRANS30",FF_TRANS30},
	{"FF_TRANS40",FF_TRANS40},
	{"FF_TRANS50",FF_TRANS50},
	{"FF_TRANS60",FF_TRANS60},
	{"FF_TRANS70",FF_TRANS70},
	{"FF_TRANS80",FF_TRANS80},
	{"FF_TRANS90",FF_TRANS90},
	// temporary, for testing
	{"FF_TRANSADD",FF_ADD},
	{"FF_TRANSSUB",FF_SUBTRACT},
	// compatibility
	// Transparency for SOCs is pre-shifted
	{"TR_TRANS10",tr_trans10<<FF_TRANSSHIFT},
	{"TR_TRANS20",tr_trans20<<FF_TRANSSHIFT},
	{"TR_TRANS30",tr_trans30<<FF_TRANSSHIFT},
	{"TR_TRANS40",tr_trans40<<FF_TRANSSHIFT},
	{"TR_TRANS50",tr_trans50<<FF_TRANSSHIFT},
	{"TR_TRANS60",tr_trans60<<FF_TRANSSHIFT},
	{"TR_TRANS70",tr_trans70<<FF_TRANSSHIFT},
	{"TR_TRANS80",tr_trans80<<FF_TRANSSHIFT},
	{"TR_TRANS90",tr_trans90<<FF_TRANSSHIFT},
	// Transparency for Lua is not, unless capitalized as above.
	{"tr_trans10",tr_trans10},
	{"tr_trans20",tr_trans20},
	{"tr_trans30",tr_trans30},
	{"tr_trans40",tr_trans40},
	{"tr_trans50",tr_trans50},
	{"tr_trans60",tr_trans60},
	{"tr_trans70",tr_trans70},
	{"tr_trans80",tr_trans80},
	{"tr_trans90",tr_trans90},
	{"NUMTRANSMAPS",NUMTRANSMAPS},

	// Alpha styles (blend modes)
	//{"AST_COPY",AST_COPY}, -- not useful in lua
	//{"AST_TRANSLUCENT",AST_TRANSLUCENT}, -- ditto
	{"AST_ADD",AST_ADD},
	{"AST_SUBTRACT",AST_SUBTRACT},
	{"AST_REVERSESUBTRACT",AST_REVERSESUBTRACT},
	{"AST_MODULATE",AST_MODULATE},
	{"AST_OVERLAY",AST_OVERLAY},
	{"AST_FOG",AST_FOG},

	// Render flags
	{"RF_HORIZONTALFLIP",RF_HORIZONTALFLIP},
	{"RF_VERTICALFLIP",RF_VERTICALFLIP},
	{"RF_ABSOLUTEOFFSETS",RF_ABSOLUTEOFFSETS},
	{"RF_FLIPOFFSETS",RF_FLIPOFFSETS},
	{"RF_SPLATMASK",RF_SPLATMASK},
	{"RF_SLOPESPLAT",RF_SLOPESPLAT},
	{"RF_OBJECTSLOPESPLAT",RF_OBJECTSLOPESPLAT},
	{"RF_NOSPLATBILLBOARD",RF_NOSPLATBILLBOARD},
	{"RF_NOSPLATROLLANGLE",RF_NOSPLATROLLANGLE},
	{"RF_BRIGHTMASK",RF_BRIGHTMASK},
	{"RF_FULLBRIGHT",RF_FULLBRIGHT},
	{"RF_FULLDARK",RF_FULLDARK},
	{"RF_SEMIBRIGHT",RF_SEMIBRIGHT},
	{"RF_NOCOLORMAPS",RF_NOCOLORMAPS},
	{"RF_ALWAYSONTOP",RF_ALWAYSONTOP},
	{"RF_SPRITETYPEMASK",RF_SPRITETYPEMASK},
	{"RF_PAPERSPRITE",RF_PAPERSPRITE},
	{"RF_FLOORSPRITE",RF_FLOORSPRITE},
	{"RF_SHADOWDRAW",RF_SHADOWDRAW},
	{"RF_SHADOWEFFECTS",RF_SHADOWEFFECTS},
	{"RF_DROPSHADOW",RF_DROPSHADOW},
	{"RF_ABSOLUTELIGHTLEVEL",RF_ABSOLUTELIGHTLEVEL},
	{"RF_REDUCEVFX",RF_REDUCEVFX},
	{"RF_HIDEINSKYBOX",RF_HIDEINSKYBOX},
	{"RF_DONTDRAW",RF_DONTDRAW},
	{"RF_DONTDRAWP1",RF_DONTDRAWP1},
	{"RF_DONTDRAWP2",RF_DONTDRAWP2},
	{"RF_DONTDRAWP3",RF_DONTDRAWP3},
	{"RF_DONTDRAWP4",RF_DONTDRAWP4},
	{"RF_BLENDMASK",RF_BLENDMASK},
	{"RF_BLENDSHIFT",RF_BLENDSHIFT},
	{"RF_ADD",RF_ADD},
	{"RF_SUBTRACT",RF_SUBTRACT},
	{"RF_REVERSESUBTRACT",RF_REVERSESUBTRACT},
	{"RF_MODULATE",RF_MODULATE},
	{"RF_OVERLAY",RF_OVERLAY},
	{"RF_TRANSMASK",RF_TRANSMASK},
	{"RF_TRANSSHIFT",RF_TRANSSHIFT},
	{"RF_TRANS10",RF_TRANS10},
	{"RF_TRANS20",RF_TRANS20},
	{"RF_TRANS30",RF_TRANS30},
	{"RF_TRANS40",RF_TRANS40},
	{"RF_TRANS50",RF_TRANS50},
	{"RF_TRANS60",RF_TRANS60},
	{"RF_TRANS70",RF_TRANS70},
	{"RF_TRANS80",RF_TRANS80},
	{"RF_TRANS90",RF_TRANS90},
	{"RF_GHOSTLY",RF_GHOSTLY},
	{"RF_GHOSTLYMASK",RF_GHOSTLYMASK},

	// Level flags
	{"LF_NOZONE",LF_NOZONE},
	{"LF_SECTIONRACE",LF_SECTIONRACE},
	{"LF_SUBTRACTNUM",LF_SUBTRACTNUM},
	// And map flags
	{"LF2_HIDEINMENU",LF2_HIDEINMENU},
	{"LF2_NOTIMEATTACK",LF2_NOTIMEATTACK},
	{"LF2_NOVISITNEEDED",LF2_NOVISITNEEDED},
	{"LF2_FINISHNEEDED",LF2_FINISHNEEDED},

	// Emeralds
	{"EMERALD_CHAOS1",EMERALD_CHAOS1},
	{"EMERALD_CHAOS2",EMERALD_CHAOS2},
	{"EMERALD_CHAOS3",EMERALD_CHAOS3},
	{"EMERALD_CHAOS4",EMERALD_CHAOS4},
	{"EMERALD_CHAOS5",EMERALD_CHAOS5},
	{"EMERALD_CHAOS6",EMERALD_CHAOS6},
	{"EMERALD_CHAOS7",EMERALD_CHAOS7},
	{"EMERALD_ALLCHAOS",EMERALD_ALLCHAOS},
	{"EMERALD_SUPER1",EMERALD_SUPER1},
	{"EMERALD_SUPER2",EMERALD_SUPER2},
	{"EMERALD_SUPER3",EMERALD_SUPER3},
	{"EMERALD_SUPER4",EMERALD_SUPER4},
	{"EMERALD_SUPER5",EMERALD_SUPER5},
	{"EMERALD_SUPER6",EMERALD_SUPER6},
	{"EMERALD_SUPER7",EMERALD_SUPER7},
	{"EMERALD_ALLSUPER",EMERALD_ALLSUPER},
	{"EMERALD_ALL",EMERALD_ALL},

	// SKINCOLOR_ doesn't include these..!
	{"MAXSKINCOLORS",MAXSKINCOLORS},
	{"FIRSTSUPERCOLOR",FIRSTSUPERCOLOR},
	{"NUMSUPERCOLORS",NUMSUPERCOLORS},

	// Precipitation
	{"PRECIP_NONE",PRECIP_NONE},
	{"PRECIP_RAIN",PRECIP_RAIN},
	{"PRECIP_SNOW",PRECIP_SNOW},
	{"PRECIP_BLIZZARD",PRECIP_BLIZZARD},
	{"PRECIP_STORM",PRECIP_STORM},
	{"PRECIP_STORM_NORAIN",PRECIP_STORM_NORAIN},
	{"PRECIP_STORM_NOSTRIKES",PRECIP_STORM_NOSTRIKES},

	// Carrying
	{"CR_NONE",CR_NONE},
	{"CR_ZOOMTUBE",CR_ZOOMTUBE},

	// Character flags (skinflags_t)
	{"SF_MACHINE",SF_MACHINE},
	{"SF_IRONMAN",SF_IRONMAN},
	{"SF_BADNIK",SF_BADNIK},

	// Sound flags
	{"SF_TOTALLYSINGLE",SF_TOTALLYSINGLE},
	{"SF_NOMULTIPLESOUND",SF_NOMULTIPLESOUND},
	{"SF_OUTSIDESOUND",SF_OUTSIDESOUND},
	{"SF_X4AWAYSOUND",SF_X4AWAYSOUND},
	{"SF_X8AWAYSOUND",SF_X8AWAYSOUND},
	{"SF_NOINTERRUPT",SF_NOINTERRUPT},
	{"SF_X2AWAYSOUND",SF_X2AWAYSOUND},

	// Global emblem var flags
	{"GE_NOTMEDAL", GE_NOTMEDAL},
	{"GE_TIMED", GE_TIMED},
	{"GE_FOLLOWER", GE_FOLLOWER},

	// Map emblem var flags
	{"ME_ENCORE",ME_ENCORE},
	{"ME_SPBATTACK",ME_SPBATTACK},

	// Automedal SOC tags
	{"AUTOMEDAL_BRONZE",AUTOMEDAL_BRONZE},
	{"AUTOMEDAL_SILVER",AUTOMEDAL_SILVER},
	{"AUTOMEDAL_GOLD",AUTOMEDAL_GOLD},
	{"AUTOMEDAL_PLATINUM",AUTOMEDAL_PLATINUM},

	// p_local.h constants
	{"FLOATSPEED",FLOATSPEED},
	{"MAXSTEPMOVE",MAXSTEPMOVE},
	{"USERANGE",USERANGE},
	{"MELEERANGE",MELEERANGE},
	{"MISSILERANGE",MISSILERANGE},
	{"ONFLOORZ",ONFLOORZ}, // INT32_MIN
	{"ONCEILINGZ",ONCEILINGZ}, //INT32_MAX

	// for P_FlashPal
	{"PAL_WHITE",PAL_WHITE},
	{"PAL_MIXUP",PAL_MIXUP},
	{"PAL_RECYCLE",PAL_RECYCLE},
	{"PAL_NUKE",PAL_NUKE},

	// for P_DamageMobj
	//// Damage types
	{"DMG_NORMAL",DMG_NORMAL},
	{"DMG_WIPEOUT",DMG_WIPEOUT},
	{"DMG_EXPLODE",DMG_EXPLODE},
	{"DMG_TUMBLE",DMG_TUMBLE},
	{"DMG_STING",DMG_STING},
	{"DMG_KARMA",DMG_KARMA},
	{"DMG_VOLTAGE",DMG_VOLTAGE},
	{"DMG_STUMBLE",DMG_STUMBLE},
	{"DMG_WHUMBLE",DMG_WHUMBLE},
	//// Death types
	{"DMG_INSTAKILL",DMG_INSTAKILL},
	{"DMG_DEATHPIT",DMG_DEATHPIT},
	{"DMG_CRUSHED",DMG_CRUSHED},
	{"DMG_SPECTATOR",DMG_SPECTATOR},
	{"DMG_TIMEOVER",DMG_TIMEOVER},
	//// Masks
	{"DMG_STEAL",DMG_STEAL},
	{"DMG_CANTHURTSELF",DMG_CANTHURTSELF},
	{"DMG_WOMBO", DMG_WOMBO},
	{"DMG_DEATHMASK",DMG_DEATHMASK},
	{"DMG_TYPEMASK",DMG_TYPEMASK},

	// Intermission types
	{"int_none",int_none},
	{"int_time",int_time},
	{"int_score",int_score},
	{"int_scoreortimeattack", int_scoreortimeattack},

	// Overlay exception settings
	{"OV_DONTSCREENOFFSET", OV_DONTSCREENOFFSET},
	{"OV_DONT3DOFFSET", OV_DONT3DOFFSET},
	{"OV_DONTXYSCALE", OV_DONTXYSCALE},
	{"OV_DONTROLL", OV_DONTROLL},

	// Player state (playerstate_t)
	{"PST_LIVE",PST_LIVE}, // Playing or camping.
	{"PST_DEAD",PST_DEAD}, // Dead on the ground, view follows killer.
	{"PST_REBORN",PST_REBORN}, // Ready to restart/respawn???

	// Player animation (panim_t)
	{"PA_ETC",PA_ETC},
	{"PA_STILL",PA_STILL},
	{"PA_SLOW",PA_SLOW},
	{"PA_FAST",PA_FAST},
	{"PA_DRIFT",PA_DRIFT},
	{"PA_HURT",PA_HURT},

	// Customisable sounds for Skins, from sounds.h
	{"SKSSPIN",SKSSPIN},
	{"SKSPUTPUT",SKSPUTPUT},
	{"SKSPUDPUD",SKSPUDPUD},
	{"SKSPLPAN1",SKSPLPAN1}, // Ouchies
	{"SKSPLPAN2",SKSPLPAN2},
	{"SKSPLPAN3",SKSPLPAN3},
	{"SKSPLPAN4",SKSPLPAN4},
	{"SKSPLDET1",SKSPLDET1}, // Deaths
	{"SKSPLDET2",SKSPLDET2},
	{"SKSPLDET3",SKSPLDET3},
	{"SKSPLDET4",SKSPLDET4},
	{"SKSPLVCT1",SKSPLVCT1}, // Victories
	{"SKSPLVCT2",SKSPLVCT2},
	{"SKSPLVCT3",SKSPLVCT3},
	{"SKSPLVCT4",SKSPLVCT4},
	{"SKSTHOK",SKSTHOK},
	{"SKSSPNDSH",SKSSPNDSH},
	{"SKSZOOM",SKSZOOM},
	{"SKSSKID",SKSSKID},
	{"SKSGASP",SKSGASP},
	{"SKSJUMP",SKSJUMP},
	// SRB2kart
	{"SKSKWIN",SKSKWIN}, // Win quote
	{"SKSKLOSE",SKSKLOSE}, // Lose quote
	{"SKSKPAN1",SKSKPAN1}, // Pain
	{"SKSKPAN2",SKSKPAN2},
	{"SKSKATK1",SKSKATK1}, // Offense item taunt
	{"SKSKATK2",SKSKATK2},
	{"SKSKBST1",SKSKBST1}, // Boost item taunt
	{"SKSKBST2",SKSKBST2},
	{"SKSKSLOW",SKSKSLOW}, // Overtake taunt
	{"SKSKHITM",SKSKHITM}, // Hit confirm taunt
	{"SKSKPOWR",SKSKPOWR}, // Power item taunt
	{"SKSKTALK",SKSKTALK}, // Dialogue

	// 3D Floor/Fake Floor/FOF/whatever flags
	{"FOF_EXISTS",FOF_EXISTS},                   ///< Always set, to check for validity.
	{"FOF_BLOCKPLAYER",FOF_BLOCKPLAYER},         ///< Solid to player, but nothing else
	{"FOF_BLOCKOTHERS",FOF_BLOCKOTHERS},         ///< Solid to everything but player
	{"FOF_SOLID",FOF_SOLID},                     ///< Clips things.
	{"FOF_RENDERSIDES",FOF_RENDERSIDES},         ///< Renders the sides.
	{"FOF_RENDERPLANES",FOF_RENDERPLANES},       ///< Renders the floor/ceiling.
	{"FOF_RENDERALL",FOF_RENDERALL},             ///< Renders everything.
	{"FOF_SWIMMABLE",FOF_SWIMMABLE},             ///< Is a water block.
	{"FOF_NOSHADE",FOF_NOSHADE},                 ///< Messes with the lighting?
	{"FOF_CUTSOLIDS",FOF_CUTSOLIDS},             ///< Cuts out hidden solid pixels.
	{"FOF_CUTEXTRA",FOF_CUTEXTRA},               ///< Cuts out hidden translucent pixels.
	{"FOF_CUTLEVEL",FOF_CUTLEVEL},               ///< Cuts out all hidden pixels.
	{"FOF_CUTSPRITES",FOF_CUTSPRITES},           ///< Final step in making 3D water.
	{"FOF_BOTHPLANES",FOF_BOTHPLANES},           ///< Render inside and outside planes.
	{"FOF_EXTRA",FOF_EXTRA},                     ///< Gets cut by ::FOF_CUTEXTRA.
	{"FOF_TRANSLUCENT",FOF_TRANSLUCENT},         ///< See through!
	{"FOF_FOG",FOF_FOG},                         ///< Fog "brush."
	{"FOF_INVERTPLANES",FOF_INVERTPLANES},       ///< Only render inside planes.
	{"FOF_ALLSIDES",FOF_ALLSIDES},               ///< Render inside and outside sides.
	{"FOF_INVERTSIDES",FOF_INVERTSIDES},         ///< Only render inside sides.
	{"FOF_DOUBLESHADOW",FOF_DOUBLESHADOW},       ///< Make two lightlist entries to reset light?
	{"FOF_FLOATBOB",FOF_FLOATBOB},               ///< Floats on water and bobs if you step on it.
	{"FOF_NORETURN",FOF_NORETURN},               ///< Used with ::FOF_CRUMBLE. Will not return to its original position after falling.
	{"FOF_CRUMBLE",FOF_CRUMBLE},                 ///< Falls 2 seconds after being stepped on, and randomly brings all touching crumbling 3dfloors down with it, providing their master sectors share the same tag (allows crumble platforms above or below, to also exist).
	{"FOF_GOOWATER",FOF_GOOWATER},               ///< Used with ::FOF_SWIMMABLE. Makes thick bouncey goop.
	{"FOF_MARIO",FOF_MARIO},                     ///< Acts like a question block when hit from underneath. Goodie spawned at top is determined by master sector.
	{"FOF_BUSTUP",FOF_BUSTUP},                   ///< You can spin through/punch this block and it will crumble!
	{"FOF_QUICKSAND",FOF_QUICKSAND},             ///< Quicksand!
	{"FOF_PLATFORM",FOF_PLATFORM},               ///< You can jump up through this to the top.
	{"FOF_REVERSEPLATFORM",FOF_REVERSEPLATFORM}, ///< A fall-through floor in normal gravity, a platform in reverse gravity.
	{"FOF_INTANGIBLEFLATS",FOF_INTANGIBLEFLATS}, ///< Both flats are intangible, but the sides are still solid.
	{"FOF_RIPPLE",FOF_RIPPLE},                   ///< Ripple the flats
	{"FOF_COLORMAPONLY",FOF_COLORMAPONLY},       ///< Only copy the colormap, not the lightlevel
	{"FOF_BOUNCY",FOF_BOUNCY},                   ///< Bounces players
	{"FOF_SPLAT",FOF_SPLAT},                     ///< Use splat flat renderer (treat cyan pixels as invisible)

	// Old FOF flags for backwards compatibility
	{"FF_EXISTS",FF_OLD_EXISTS},
	{"FF_BLOCKPLAYER",FF_OLD_BLOCKPLAYER},
	{"FF_BLOCKOTHERS",FF_OLD_BLOCKOTHERS},
	{"FF_SOLID",FF_OLD_SOLID},
	{"FF_RENDERSIDES",FF_OLD_RENDERSIDES},
	{"FF_RENDERPLANES",FF_OLD_RENDERPLANES},
	{"FF_RENDERALL",FF_OLD_RENDERALL},
	{"FF_SWIMMABLE",FF_OLD_SWIMMABLE},
	{"FF_NOSHADE",FF_OLD_NOSHADE},
	{"FF_CUTSOLIDS",FF_OLD_CUTSOLIDS},
	{"FF_CUTEXTRA",FF_OLD_CUTEXTRA},
	{"FF_CUTLEVEL",FF_OLD_CUTLEVEL},
	{"FF_CUTSPRITES",FF_OLD_CUTSPRITES},
	{"FF_BOTHPLANES",FF_OLD_BOTHPLANES},
	{"FF_EXTRA",FF_OLD_EXTRA},
	{"FF_TRANSLUCENT",FF_OLD_TRANSLUCENT},
	{"FF_FOG",FF_OLD_FOG},
	{"FF_INVERTPLANES",FF_OLD_INVERTPLANES},
	{"FF_ALLSIDES",FF_OLD_ALLSIDES},
	{"FF_INVERTSIDES",FF_OLD_INVERTSIDES},
	{"FF_DOUBLESHADOW",FF_OLD_DOUBLESHADOW},
	{"FF_FLOATBOB",FF_OLD_FLOATBOB},
	{"FF_NORETURN",FF_OLD_NORETURN},
	{"FF_CRUMBLE",FF_OLD_CRUMBLE},
	{"FF_SHATTERBOTTOM",FF_OLD_SHATTERBOTTOM},
	{"FF_GOOWATER",FF_OLD_GOOWATER},
	{"FF_MARIO",FF_OLD_MARIO},
	{"FF_BUSTUP",FF_OLD_BUSTUP},
	{"FF_QUICKSAND",FF_OLD_QUICKSAND},
	{"FF_PLATFORM",FF_OLD_PLATFORM},
	{"FF_REVERSEPLATFORM",FF_OLD_REVERSEPLATFORM},
	{"FF_INTANGIBLEFLATS",FF_OLD_INTANGIBLEFLATS},
	{"FF_INTANGABLEFLATS",FF_OLD_INTANGIBLEFLATS},
	{"FF_SHATTER",FF_OLD_SHATTER},
	{"FF_SPINBUST",FF_OLD_SPINBUST},
	{"FF_STRONGBUST",FF_OLD_STRONGBUST},
	{"FF_RIPPLE",FF_OLD_RIPPLE},
	{"FF_COLORMAPONLY",FF_OLD_COLORMAPONLY},

	// FOF bustable flags
	{"FB_PUSHABLES",FB_PUSHABLES},
	{"FB_EXECUTOR",FB_EXECUTOR},
	{"FB_ONLYBOTTOM",FB_ONLYBOTTOM},

	// Bustable FOF type
	{"BT_TOUCH",BT_TOUCH},
	{"BT_SPINBUST",BT_SPINBUST},
	{"BT_REGULAR",BT_REGULAR},
	{"BT_STRONG",BT_STRONG},

	// PolyObject flags
	{"POF_CLIPLINES",POF_CLIPLINES},               ///< Test against lines for collision
	{"POF_CLIPPLANES",POF_CLIPPLANES},             ///< Test against tops and bottoms for collision
	{"POF_SOLID",POF_SOLID},                       ///< Clips things.
	{"POF_TESTHEIGHT",POF_TESTHEIGHT},             ///< Test line collision with heights
	{"POF_RENDERSIDES",POF_RENDERSIDES},           ///< Renders the sides.
	{"POF_RENDERTOP",POF_RENDERTOP},               ///< Renders the top.
	{"POF_RENDERBOTTOM",POF_RENDERBOTTOM},         ///< Renders the bottom.
	{"POF_RENDERPLANES",POF_RENDERPLANES},         ///< Renders top and bottom.
	{"POF_RENDERALL",POF_RENDERALL},               ///< Renders everything.
	{"POF_INVERT",POF_INVERT},                     ///< Inverts collision (like a cage).
	{"POF_INVERTPLANES",POF_INVERTPLANES},         ///< Render inside planes.
	{"POF_INVERTPLANESONLY",POF_INVERTPLANESONLY}, ///< Only render inside planes.
	{"POF_PUSHABLESTOP",POF_PUSHABLESTOP},         ///< Pushables will stop movement.
	{"POF_LDEXEC",POF_LDEXEC},                     ///< This PO triggers a linedef executor.
	{"POF_ONESIDE",POF_ONESIDE},                   ///< Only use the first side of the linedef.
	{"POF_NOSPECIALS",POF_NOSPECIALS},             ///< Don't apply sector specials.
	{"POF_SPLAT",POF_SPLAT},                       ///< Use splat flat renderer (treat cyan pixels as invisible).

#ifdef HAVE_LUA_SEGS
	// Node flags
	{"NF_SUBSECTOR",NF_SUBSECTOR}, // Indicate a leaf.
#endif

	// Slope flags
	{"SL_NOPHYSICS",SL_NOPHYSICS},
	{"SL_DYNAMIC",SL_DYNAMIC},

	// Angles
	{"ANG1",ANG1},
	{"ANG2",ANG2},
	{"ANG10",ANG10},
	{"ANG15",ANG15},
	{"ANG20",ANG20},
	{"ANG30",ANG30},
	{"ANG60",ANG60},
	{"ANG64h",ANG64h},
	{"ANG105",ANG105},
	{"ANG210",ANG210},
	{"ANG255",ANG255},
	{"ANG340",ANG340},
	{"ANG350",ANG350},
	{"ANGLE_11hh",ANGLE_11hh},
	{"ANGLE_22h",ANGLE_22h},
	{"ANGLE_45",ANGLE_45},
	{"ANGLE_67h",ANGLE_67h},
	{"ANGLE_90",ANGLE_90},
	{"ANGLE_112h",ANGLE_112h},
	{"ANGLE_135",ANGLE_135},
	{"ANGLE_157h",ANGLE_157h},
	{"ANGLE_180",ANGLE_180},
	{"ANGLE_202h",ANGLE_202h},
	{"ANGLE_225",ANGLE_225},
	{"ANGLE_247h",ANGLE_247h},
	{"ANGLE_270",ANGLE_270},
	{"ANGLE_292h",ANGLE_292h},
	{"ANGLE_315",ANGLE_315},
	{"ANGLE_337h",ANGLE_337h},
	{"ANGLE_MAX",ANGLE_MAX},

	// P_Chase directions (dirtype_t)
	{"DI_NODIR",DI_NODIR},
	{"DI_EAST",DI_EAST},
	{"DI_NORTHEAST",DI_NORTHEAST},
	{"DI_NORTH",DI_NORTH},
	{"DI_NORTHWEST",DI_NORTHWEST},
	{"DI_WEST",DI_WEST},
	{"DI_SOUTHWEST",DI_SOUTHWEST},
	{"DI_SOUTH",DI_SOUTH},
	{"DI_SOUTHEAST",DI_SOUTHEAST},
	{"NUMDIRS",NUMDIRS},

	// Buttons (ticcmd_t)
	{"BT_ACCELERATE",BT_ACCELERATE},
	{"BT_DRIFT",BT_DRIFT},
	{"BT_BRAKE",BT_BRAKE},
	{"BT_ATTACK",BT_ATTACK},
	{"BT_LOOKBACK",BT_LOOKBACK},
	{"BT_RESPAWN",BT_RESPAWN},
	{"BT_VOTE",BT_VOTE},
	{"BT_EBRAKEMASK",BT_EBRAKEMASK}, // Macro button
	{"BT_SPINDASHMASK",BT_SPINDASHMASK}, // Macro button
	{"BT_LUAA",BT_LUAA}, // Lua customizable
	{"BT_LUAB",BT_LUAB}, // Lua customizable
	{"BT_LUAC",BT_LUAC}, // Lua customizable

	// Lua command registration flags
	{"COM_ADMIN",COM_ADMIN},
	{"COM_LOCAL",COM_LOCAL},
	{"COM_NOSHOWHELP",COM_NOSHOWHELP},
	{"COM_PLAYER2",COM_PLAYER2},
	{"COM_PLAYER3",COM_PLAYER3},
	{"COM_PLAYER4",COM_PLAYER4},
	{"COM_SPLITSCREEN",COM_SPLITSCREEN},
	{"COM_SSSHIFT",COM_SSSHIFT},

	// cvflags_t
	{"CV_SAVE",CV_SAVE},
	{"CV_CALL",CV_CALL},
	{"CV_NETVAR",CV_NETVAR},
	{"CV_NOINIT",CV_NOINIT},
	{"CV_FLOAT",CV_FLOAT},
	{"CV_NOTINNET",CV_NOTINNET},
	{"CV_MODIFIED",CV_MODIFIED},
	{"CV_SHOWMODIF",CV_SHOWMODIF},
	{"CV_SHOWMODIFONETIME",CV_SHOWMODIFONETIME},
	{"CV_NOSHOWHELP",CV_NOSHOWHELP},
	{"CV_HIDDEN",CV_HIDDEN},
	{"CV_CHEAT",CV_CHEAT},
	{"CV_NOLUA",CV_NOLUA},
	{"CV_ADDEDBYLUA",CV_ADDEDBYLUA},

	// v_video flags
	{"V_NOSCALEPATCH",V_NOSCALEPATCH},
	{"V_SMALLSCALEPATCH",V_SMALLSCALEPATCH},
	{"V_MEDSCALEPATCH",V_MEDSCALEPATCH},
	{"V_6WIDTHSPACE",V_6WIDTHSPACE},
	{"V_OLDSPACING",V_OLDSPACING},
	{"V_MONOSPACE",V_MONOSPACE},

	{"V_PURPLEMAP",V_PURPLEMAP},
	{"V_YELLOWMAP",V_YELLOWMAP},
	{"V_GREENMAP",V_GREENMAP},
	{"V_BLUEMAP",V_BLUEMAP},
	{"V_REDMAP",V_REDMAP},
	{"V_GRAYMAP",V_GRAYMAP},
	{"V_ORANGEMAP",V_ORANGEMAP},
	{"V_SKYMAP",V_SKYMAP},
	{"V_LAVENDERMAP",V_LAVENDERMAP},
	{"V_GOLDMAP",V_GOLDMAP},
	{"V_AQUAMAP",V_AQUAMAP},
	{"V_MAGENTAMAP",V_MAGENTAMAP},
	{"V_PINKMAP",V_PINKMAP},
	{"V_BROWNMAP",V_BROWNMAP},
	{"V_TANMAP",V_TANMAP},

	{"V_TRANSLUCENT",V_TRANSLUCENT},
	{"V_10TRANS",V_10TRANS},
	{"V_20TRANS",V_20TRANS},
	{"V_30TRANS",V_30TRANS},
	{"V_40TRANS",V_40TRANS},
	{"V_50TRANS",V_TRANSLUCENT}, // alias
	{"V_60TRANS",V_60TRANS},
	{"V_70TRANS",V_70TRANS},
	{"V_80TRANS",V_80TRANS},
	{"V_90TRANS",V_90TRANS},
	{"V_HUDTRANSHALF",V_HUDTRANSHALF},
	{"V_HUDTRANS",V_HUDTRANS},
	{"V_BLENDSHIFT",V_BLENDSHIFT},
	{"V_BLENDMASK",V_BLENDMASK},
	{"V_ADD",V_ADD},
	{"V_SUBTRACT",V_SUBTRACT},
	{"V_REVERSESUBTRACT",V_REVERSESUBTRACT},
	{"V_MODULATE",V_MODULATE},
	{"V_OVERLAY",V_OVERLAY},
	{"V_FORCEUPPERCASE",V_FORCEUPPERCASE},
	{"V_FLIP",V_FLIP},
	{"V_VFLIP",V_VFLIP},
	{"V_SNAPTOTOP",V_SNAPTOTOP},
	{"V_SNAPTOBOTTOM",V_SNAPTOBOTTOM},
	{"V_SNAPTOLEFT",V_SNAPTOLEFT},
	{"V_SNAPTORIGHT",V_SNAPTORIGHT},
	{"V_NOSCALESTART",V_NOSCALESTART},
	{"V_SPLITSCREEN",V_SPLITSCREEN},
	{"V_SLIDEIN",V_SLIDEIN},

	{"V_PARAMMASK",V_PARAMMASK},
	{"V_SCALEPATCHMASK",V_SCALEPATCHMASK},
	{"V_SPACINGMASK",V_SPACINGMASK},
	{"V_CHARCOLORMASK",V_CHARCOLORMASK},
	{"V_ALPHAMASK",V_ALPHAMASK},

	{"V_CHARCOLORSHIFT",V_CHARCOLORSHIFT},
	{"V_ALPHASHIFT",V_ALPHASHIFT},

	//Kick Reasons
	{"KR_KICK",KR_KICK},
	{"KR_PINGLIMIT",KR_PINGLIMIT},
	{"KR_SYNCH",KR_SYNCH},
	{"KR_TIMEOUT",KR_TIMEOUT},
	{"KR_BAN",KR_BAN},
	{"KR_LEAVE",KR_LEAVE},

	// translation colormaps
	{"TC_DEFAULT",TC_DEFAULT},
	{"TC_BOSS",TC_BOSS},
	{"TC_METALSONIC",TC_METALSONIC},
	{"TC_ALLWHITE",TC_ALLWHITE},
	{"TC_RAINBOW",TC_RAINBOW},
	{"TC_BLINK",TC_BLINK},
	{"TC_DASHMODE",TC_DASHMODE},
	{"TC_HITLAG",TC_HITLAG},
	{"TC_INTERMISSION",TC_INTERMISSION},

	// marathonmode flags
	{"MA_INIT",MA_INIT},
	{"MA_RUNNING",MA_RUNNING},
	{"MA_NOCUTSCENES",MA_NOCUTSCENES},
	{"MA_INGAME",MA_INGAME},

	// gamestates
	{"GS_NULL",GS_NULL},
	{"GS_LEVEL",GS_LEVEL},
	{"GS_INTERMISSION",GS_INTERMISSION},
	{"GS_CONTINUING",GS_CONTINUING},
	{"GS_TITLESCREEN",GS_TITLESCREEN},
	{"GS_MENU",GS_MENU},
	{"GS_CREDITS",GS_CREDITS},
	{"GS_EVALUATION",GS_EVALUATION},
	{"GS_INTRO",GS_INTRO},
	{"GS_CUTSCENE",GS_CUTSCENE},
	{"GS_DEDICATEDSERVER",GS_DEDICATEDSERVER},
	{"GS_WAITINGPLAYERS",GS_WAITINGPLAYERS},

	// SRB2Kart
	// kartitems_t
#define FOREACH( name, n ) { TOSTR (KITEM_ ## name), KITEM_ ## name }
	KART_ITEM_ITERATOR, // Actual items (can be set for k_itemtype)
#undef  FOREACH
	{"NUMKARTITEMS",NUMKARTITEMS},
	{"KRITEM_DUALSNEAKER",KRITEM_DUALSNEAKER}, // Additional roulette IDs (not usable for much in Lua besides K_GetItemPatch)
	{"KRITEM_TRIPLESNEAKER",KRITEM_TRIPLESNEAKER},
	{"KRITEM_TRIPLEBANANA",KRITEM_TRIPLEBANANA},
	{"KRITEM_TRIPLEORBINAUT",KRITEM_TRIPLEORBINAUT},
	{"KRITEM_QUADORBINAUT",KRITEM_QUADORBINAUT},
	{"KRITEM_DUALJAWZ",KRITEM_DUALJAWZ},
	{"KRITEM_TRIPLEGACHABOM",KRITEM_TRIPLEGACHABOM},
	{"NUMKARTRESULTS",NUMKARTRESULTS},
	{"FIRSTPOWERUP",FIRSTPOWERUP},
	{"POWERUP_SMONITOR",POWERUP_SMONITOR},
	{"POWERUP_BARRIER",POWERUP_BARRIER},
	{"POWERUP_BUMPER",POWERUP_BUMPER},
	{"POWERUP_BADGE",POWERUP_BADGE},
	{"POWERUP_SUPERFLICKY",POWERUP_SUPERFLICKY},
	{"POWERUP_POINTS",POWERUP_POINTS},
	{"ENDOFPOWERUPS",ENDOFPOWERUPS},
	{"LASTPOWERUP",LASTPOWERUP},
	{"NUMPOWERUPS",NUMPOWERUPS},

	// kartshields_t
	{"KSHIELD_NONE",KSHIELD_NONE},
	{"KSHIELD_LIGHTNING",KSHIELD_LIGHTNING},
	{"KSHIELD_BUBBLE",KSHIELD_BUBBLE},
	{"KSHIELD_FLAME",KSHIELD_FLAME},
	{"KSHIELD_TOP",KSHIELD_TOP},
	{"NUMKARTSHIELDS",NUMKARTSHIELDS},

	// kartspinoutflags_t
	{"KSPIN_THRUST",KSPIN_THRUST},
	{"KSPIN_IFRAMES",KSPIN_IFRAMES},
	{"KSPIN_AIRTIMER",KSPIN_AIRTIMER},

	{"KSPIN_TYPEBIT",KSPIN_TYPEBIT},
	{"KSPIN_TYPEMASK",KSPIN_TYPEMASK},

	{"KSPIN_SPINOUT",KSPIN_SPINOUT},
	{"KSPIN_WIPEOUT",KSPIN_WIPEOUT},
	{"KSPIN_STUNG",KSPIN_STUNG},
	{"KSPIN_EXPLOSION",KSPIN_EXPLOSION},

	// spottype_t
	{"SPOT_NONE",SPOT_NONE},
	{"SPOT_WEAK",SPOT_WEAK},
	{"SPOT_BUMP",SPOT_BUMP},

	// precipeffect_t
	{"PRECIPFX_THUNDER",PRECIPFX_THUNDER},
	{"PRECIPFX_LIGHTNING",PRECIPFX_LIGHTNING},
	{"PRECIPFX_WATERPARTICLES",PRECIPFX_WATERPARTICLES},

	{NULL,0}
};

// For this to work compile-time without being in this file,
// this function would need to check sizes at runtime, without sizeof
void DEH_TableCheck(void)
{
#if defined(_DEBUG) || defined(PARANOIA)
	const size_t dehstates = sizeof(STATE_LIST)/sizeof(const char*);
	const size_t dehmobjs  = sizeof(MOBJTYPE_LIST)/sizeof(const char*);
	const size_t dehcolors = sizeof(COLOR_ENUMS)/sizeof(const char*);

	if (dehstates != S_FIRSTFREESLOT)
		I_Error("You forgot to update the Dehacked states list, you dolt!\n(%d states defined, versus %s in the Dehacked list)\n", S_FIRSTFREESLOT, sizeu1(dehstates));

	if (dehmobjs != MT_FIRSTFREESLOT)
		I_Error("You forgot to update the Dehacked mobjtype list, you dolt!\n(%d mobj types defined, versus %s in the Dehacked list)\n", MT_FIRSTFREESLOT, sizeu1(dehmobjs));

	if (dehcolors != SKINCOLOR_FIRSTFREESLOT)
		I_Error("You forgot to update the Dehacked colors list, you dolt!\n(%d colors defined, versus %s in the Dehacked list)\n", SKINCOLOR_FIRSTFREESLOT, sizeu1(dehcolors));
#endif
}
