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
/// \file  p_local.h
/// \brief Play functions, animation, global header

#ifndef __P_LOCAL__
#define __P_LOCAL__

#include "command.h"
#include "d_player.h"
#include "d_think.h"
#include "m_fixed.h"
#include "m_bbox.h"
#include "p_tick.h"
#include "r_defs.h"
#include "p_maputl.h"
#include "doomstat.h" // MAXSPLITSCREENPLAYERS

#define FLOATSPEED (FRACUNIT*4)

//#define VIEWHEIGHTS "41"

// Maximum player score.
#define MAXSCORE 99999990 // 999999990

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS 128
#define MAPBLOCKSIZE  (MAPBLOCKUNITS*FRACUNIT)
#define MAPBLOCKSHIFT (FRACBITS+7)
#define MAPBMASK      (MAPBLOCKSIZE-1)
#define MAPBTOFRAC    (MAPBLOCKSHIFT-FRACBITS)

// Convenience macro to fix issue with collision along bottom/left edges of blockmap -Red
#define BMBOUNDFIX(xl, xh, yl, yh) {if (xl > xh) xl = 0; if (yl > yh) yl = 0;}

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MAXRADIUS (32*FRACUNIT)

// max Z move up or down without jumping
// above this, a height difference is considered as a 'dropoff'
#define MAXSTEPMOVE (24*FRACUNIT)

#define USERANGE (64*FRACUNIT)
#define MELEERANGE (64*FRACUNIT)
#define MISSILERANGE (32*64*FRACUNIT)

#define AIMINGTOSLOPE(aiming) FINESINE((aiming>>ANGLETOFINESHIFT) & FINEMASK)

#define P_GetPlayerViewHeight(player) (41*player->mo->height/48)

typedef enum
{
	THINK_POLYOBJ,
	THINK_MAIN,
	THINK_MOBJ,
	THINK_DYNSLOPE,
	THINK_PRECIP,
	NUM_THINKERLISTS
} thinklistnum_t; /**< Thinker lists. */
extern thinker_t thlist[];

void P_InitThinkers(void);
void P_AddThinker(const thinklistnum_t n, thinker_t *thinker);
void P_RemoveThinker(thinker_t *thinker);

//
// P_USER
//
typedef struct camera_s
{
	boolean chase;
	angle_t aiming;

	// Things used by FS cameras.
	fixed_t viewheight;
	angle_t startangle;

	// Camera demobjerization
	// Info for drawing: position.
	fixed_t x, y, z;

	//More drawing info: to determine current sprite.
	angle_t angle; // orientation

	struct subsector_s *subsector;

	// The closest interval over all contacted Sectors (or Things).
	fixed_t floorz;
	fixed_t ceilingz;

	// For movement checking.
	fixed_t radius;
	fixed_t height;

	fixed_t relativex;

	// Momentums, used to update position.
	fixed_t momx, momy, momz;

	// SRB2Kart: camera pans while drifting
	fixed_t pan;
	// SRB2Kart: camera pitches on slopes
	angle_t pitch;
} camera_t;

// demo freecam or something before i commit die
struct demofreecam_s {

	camera_t *cam;	// this is useful when the game is paused, notably
	mobj_t *soundmobj;	// mobj to play sound from, used in s_sound
	
	angle_t localangle;	// keeps track of the cam angle for cmds
	angle_t localaiming;	// ditto with aiming
	boolean turnheld;	// holding turn button for gradual turn speed
	boolean keyboardlook;	// keyboard look
};

extern struct demofreecam_s democam;

extern camera_t camera[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_cam_dist[MAXSPLITSCREENPLAYERS], cv_cam_still[MAXSPLITSCREENPLAYERS], cv_cam_height[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_cam_speed[MAXSPLITSCREENPLAYERS], cv_cam_rotate[MAXSPLITSCREENPLAYERS];

extern fixed_t t_cam_dist[MAXSPLITSCREENPLAYERS], t_cam_height[MAXSPLITSCREENPLAYERS], t_cam_rotate[MAXSPLITSCREENPLAYERS];

void P_AddPlayerScore(player_t *player, UINT32 amount);
void P_ResetCamera(player_t *player, camera_t *thiscam);
boolean P_TryCameraMove(fixed_t x, fixed_t y, camera_t *thiscam);
void P_SlideCameraMove(camera_t *thiscam);
void P_DemoCameraMovement(camera_t *cam);
boolean P_MoveChaseCamera(player_t *player, camera_t *thiscam, boolean resetcalled);
void P_InitCameraCmd(void);

boolean P_PlayerInPain(player_t *player);
void P_ResetPlayer(player_t *player);
boolean P_PlayerCanDamage(player_t *player, mobj_t *thing);

boolean P_IsLocalPlayer(player_t *player);
boolean P_IsDisplayPlayer(player_t *player);

void P_SetPlayerAngle(player_t *player, angle_t angle);
angle_t P_GetLocalAngle(player_t *player);
void P_ForceLocalAngle(player_t *player, angle_t angle);
boolean P_PlayerFullbright(player_t *player);

boolean P_IsObjectInGoop(mobj_t *mo);
boolean P_IsObjectOnGround(mobj_t *mo);
boolean P_IsObjectOnGroundIn(mobj_t *mo, sector_t *sec);
boolean P_IsObjectOnRealGround(mobj_t *mo, sector_t *sec); // SRB2Kart
boolean P_InQuicksand(mobj_t *mo);
boolean P_PlayerHitFloor(player_t *player, boolean dorollstuff);

void P_SetObjectMomZ(mobj_t *mo, fixed_t value, boolean relative);
void P_RestoreMusic(player_t *player);
boolean P_EndingMusic(player_t *player);
void P_SpawnShieldOrb(player_t *player);
void P_SwitchShield(player_t *player, UINT16 shieldtype);
mobj_t *P_SpawnGhostMobj(mobj_t *mobj);
void P_GivePlayerRings(player_t *player, INT32 num_rings);
void P_GivePlayerSpheres(player_t *player, INT32 num_spheres);
void P_GivePlayerLives(player_t *player, INT32 numlives);
UINT8 P_GetNextEmerald(void);
void P_GiveEmerald(boolean spawnObj);
void P_GiveFinishFlags(player_t *player);
#if 0
void P_ResetScore(player_t *player);
#else
#define P_ResetScore(player) player->scoreadd = 0
#endif
boolean P_AutoPause(void);

void P_ElementalFire(player_t *player, boolean cropcircle);
void P_SpawnSkidDust(player_t *player, fixed_t radius, boolean sound);

void P_MovePlayer(player_t *player);
void P_PlayerThink(player_t *player);
void P_PlayerAfterThink(player_t *player);
void P_DoPlayerExit(player_t *player);
void P_DoTimeOver(player_t *player);

void P_InstaThrust(mobj_t *mo, angle_t angle, fixed_t move);
fixed_t P_ReturnThrustX(mobj_t *mo, angle_t angle, fixed_t move);
fixed_t P_ReturnThrustY(mobj_t *mo, angle_t angle, fixed_t move);

mobj_t *P_LookForFocusTarget(player_t *player, mobj_t *exclude, SINT8 direction, UINT8 lockonflags);

void P_NukeEnemies(mobj_t *inflictor, mobj_t *source, fixed_t radius);

UINT8 P_FindLowestLap(void);
UINT8 P_FindHighestLap(void);

boolean P_PlayerMoving(INT32 pnum);

void P_PlayLivesJingle(player_t *player);
void P_PlayRinglossSound(mobj_t *source);
void P_PlayDeathSound(mobj_t *source);
void P_PlayVictorySound(mobj_t *source);

boolean P_GetLives(player_t *player);
boolean P_SpectatorJoinGame(player_t *player);
void P_RestoreMultiMusic(player_t *player);

/// ------------------------
/// Jingle stuff
/// ------------------------

typedef enum
{
	JT_NONE,   // Null state
	JT_OTHER,  // Other state
	JT_MASTER, // Main level music
	JT_1UP, // Extra life
	JT_SHOES,  // Speed shoes
	JT_INV, // Invincibility
	JT_MINV, // Mario Invincibility
	JT_DROWN,  // Drowning
	JT_SUPER,  // Super Sonic
	JT_GOVER, // Game Over
	JT_NIGHTSTIMEOUT, // NiGHTS Time Out (10 seconds)
	JT_SSTIMEOUT, // NiGHTS Special Stage Time Out (10 seconds)

	// these are not jingles
	// JT_LCLEAR, // Level Clear
	// JT_RACENT, // Multiplayer Intermission
	// JT_CONTSC, // Continue

	NUMJINGLES
} jingletype_t;

typedef struct
{
	char musname[7];
	boolean looping;
} jingle_t;

extern jingle_t jingleinfo[NUMJINGLES];

#define JINGLEPOSTFADE 1000

void P_PlayJingle(player_t *player, jingletype_t jingletype);
boolean P_EvaluateMusicStatus(UINT16 status, const char *musname);
void P_PlayJingleMusic(player_t *player, const char *musname, UINT16 musflags, boolean looping, UINT16 status);

//
// P_MOBJ
//
#define ONFLOORZ INT32_MIN
#define ONCEILINGZ INT32_MAX

// Time interval for item respawning.
// WARNING MUST be a power of 2
#define ITEMQUESIZE 1024

extern mapthing_t *itemrespawnque[ITEMQUESIZE];
extern tic_t itemrespawntime[ITEMQUESIZE];
extern size_t iquehead, iquetail;
extern consvar_t cv_gravity, cv_movebob;

void P_RespawnBattleBoxes(void);
mobjtype_t P_GetMobjtype(UINT16 mthingtype);

void P_RespawnSpecials(void);

mobj_t *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);

void P_RecalcPrecipInSector(sector_t *sector);
void P_PrecipitationEffects(void);

void P_RemoveMobj(mobj_t *th);
boolean P_MobjWasRemoved(mobj_t *th);
void P_RemoveSavegameMobj(mobj_t *th);
boolean P_SetPlayerMobjState(mobj_t *mobj, statenum_t state);
boolean P_SetMobjState(mobj_t *mobj, statenum_t state);
void P_RunShields(void);
void P_RunOverlays(void);
void P_HandleMinecartSegments(mobj_t *mobj);
void P_MobjThinker(mobj_t *mobj);
boolean P_RailThinker(mobj_t *mobj);
void P_PushableThinker(mobj_t *mobj);
void P_SceneryThinker(mobj_t *mobj);


fixed_t P_MobjFloorZ(mobj_t *mobj, sector_t *sector, sector_t *boundsec, fixed_t x, fixed_t y, line_t *line, boolean lowest, boolean perfect);
fixed_t P_MobjCeilingZ(mobj_t *mobj, sector_t *sector, sector_t *boundsec, fixed_t x, fixed_t y, line_t *line, boolean lowest, boolean perfect);
#define P_GetFloorZ(mobj, sector, x, y, line) P_MobjFloorZ(mobj, sector, NULL, x, y, line, false, false)
#define P_GetCeilingZ(mobj, sector, x, y, line) P_MobjCeilingZ(mobj, sector, NULL, x, y, line, true, false)
#define P_GetFOFTopZ(mobj, sector, fof, x, y, line) P_MobjCeilingZ(mobj, sectors + fof->secnum, sector, x, y, line, false, false)
#define P_GetFOFBottomZ(mobj, sector, fof, x, y, line) P_MobjFloorZ(mobj, sectors + fof->secnum, sector, x, y, line, true, false)
#define P_GetSpecialBottomZ(mobj, src, bound) P_MobjFloorZ(mobj, src, bound, mobj->x, mobj->y, NULL, src != bound, true)
#define P_GetSpecialTopZ(mobj, src, bound) P_MobjCeilingZ(mobj, src, bound, mobj->x, mobj->y, NULL, src == bound, true)

fixed_t P_CameraFloorZ(camera_t *mobj, sector_t *sector, sector_t *boundsec, fixed_t x, fixed_t y, line_t *line, boolean lowest, boolean perfect);
fixed_t P_CameraCeilingZ(camera_t *mobj, sector_t *sector, sector_t *boundsec, fixed_t x, fixed_t y, line_t *line, boolean lowest, boolean perfect);
#define P_CameraGetFloorZ(mobj, sector, x, y, line) P_CameraFloorZ(mobj, sector, NULL, x, y, line, false, false)
#define P_CameraGetCeilingZ(mobj, sector, x, y, line) P_CameraCeilingZ(mobj, sector, NULL, x, y, line, true, false)
#define P_CameraGetFOFTopZ(mobj, sector, fof, x, y, line) P_CameraCeilingZ(mobj, sectors + fof->secnum, sector, x, y, line, false, false)
#define P_CameraGetFOFBottomZ(mobj, sector, fof, x, y, line) P_CameraFloorZ(mobj, sectors + fof->secnum, sector, x, y, line, true, false)

boolean P_InsideANonSolidFFloor(mobj_t *mobj, ffloor_t *rover);
boolean P_CheckDeathPitCollide(mobj_t *mo);
boolean P_CheckSolidLava(ffloor_t *rover);
void P_AdjustMobjFloorZ_FFloors(mobj_t *mo, sector_t *sector, UINT8 motype);

mobj_t *P_SpawnMobjFromMobj(mobj_t *mobj, fixed_t xofs, fixed_t yofs, fixed_t zofs, mobjtype_t type);

mobj_t *P_SpawnMissile(mobj_t *source, mobj_t *dest, mobjtype_t type);
mobj_t *P_SpawnXYZMissile(mobj_t *source, mobj_t *dest, mobjtype_t type, fixed_t x, fixed_t y, fixed_t z);
mobj_t *P_SpawnPointMissile(mobj_t *source, fixed_t xa, fixed_t ya, fixed_t za, mobjtype_t type, fixed_t x, fixed_t y, fixed_t z);
mobj_t *P_SpawnAlteredDirectionMissile(mobj_t *source, mobjtype_t type, fixed_t x, fixed_t y, fixed_t z, INT32 shiftingAngle);
mobj_t *P_SPMAngle(mobj_t *source, mobjtype_t type, angle_t angle, UINT8 aimtype, UINT32 flags2);
#define P_SpawnPlayerMissile(s,t,f) P_SPMAngle(s,t,s->angle,true,f)
#ifdef SEENAMES
#define P_SpawnNameFinder(s,t) P_SPMAngle(s,t,s->angle,true,0)
#endif
void P_ColorTeamMissile(mobj_t *missile, player_t *source);
SINT8 P_MobjFlip(mobj_t *mobj);
fixed_t P_GetMobjGravity(mobj_t *mo);

void P_CalcChasePostImg(player_t *player, camera_t *thiscam);
boolean P_CameraThinker(player_t *player, camera_t *thiscam, boolean resetcalled);

void P_Attract(mobj_t *source, mobj_t *enemy, boolean nightsgrab);
mobj_t *P_GetClosestAxis(mobj_t *source);

boolean P_CanRunOnWater(player_t *player, ffloor_t *rover);

void P_MaceRotate(mobj_t *center, INT32 baserot, INT32 baseprevrot);

void P_FlashPal(player_t *pl, UINT16 type, UINT16 duration);
#define PAL_WHITE    1
#define PAL_MIXUP    2
#define PAL_RECYCLE  3
#define PAL_NUKE     4

//
// P_ENEMY
//

// main player in game
extern player_t *stplyr; // for splitscreen correct palette changes and overlay

// Is there a better place for these?
extern INT32 var1;
extern INT32 var2;

boolean P_CheckMeleeRange(mobj_t *actor);
boolean P_JetbCheckMeleeRange(mobj_t *actor);
boolean P_FaceStabCheckMeleeRange(mobj_t *actor);
boolean P_SkimCheckMeleeRange(mobj_t *actor);
boolean P_CheckMissileRange(mobj_t *actor);

void P_NewChaseDir(mobj_t *actor);
boolean P_LookForPlayers(mobj_t *actor, boolean allaround, boolean tracer, fixed_t dist);

mobj_t *P_InternalFlickySpawn(mobj_t *actor, mobjtype_t flickytype, fixed_t momz, boolean lookforplayers, SINT8 moveforward);
void P_InternalFlickySetColor(mobj_t *actor, UINT8 extrainfo);
#define P_IsFlickyCenter(type) (type > MT_FLICKY_01 && type < MT_SEED && (type - MT_FLICKY_01) % 2 ? 1 : 0)
void P_InternalFlickyBubble(mobj_t *actor);
void P_InternalFlickyFly(mobj_t *actor, fixed_t flyspeed, fixed_t targetdist, fixed_t chasez);
void P_InternalFlickyHop(mobj_t *actor, fixed_t momz, fixed_t momh, angle_t angle);

//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern boolean floatok;
extern fixed_t tmfloorz;
extern fixed_t tmceilingz;
extern ffloor_t *tmfloorrover, *tmceilingrover;
extern mobj_t *tmfloorthing, *tmhitthing, *tmthing;
extern camera_t *mapcampointer;
extern fixed_t tmx;
extern fixed_t tmy;
extern pslope_t *tmfloorslope, *tmceilingslope;

/* cphipps 2004/08/30 */
extern void P_MapStart(void);
extern void P_MapEnd(void);

extern line_t *ceilingline;
extern line_t *blockingline;
extern msecnode_t *sector_list;

extern mprecipsecnode_t *precipsector_list;

void P_UnsetThingPosition(mobj_t *thing);
void P_SetThingPosition(mobj_t *thing);
void P_SetUnderlayPosition(mobj_t *thing);

boolean P_IsLineBlocking(const line_t *ld, const mobj_t *thing);
boolean P_CheckPosition(mobj_t *thing, fixed_t x, fixed_t y);
boolean P_CheckCameraPosition(fixed_t x, fixed_t y, camera_t *thiscam);
boolean P_TryMove(mobj_t *thing, fixed_t x, fixed_t y, boolean allowdropoff);
boolean P_Move(mobj_t *actor, fixed_t speed);
boolean P_TeleportMove(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z);
void P_SlideMove(mobj_t *mo);
void P_BouncePlayerMove(mobj_t *mo);
void P_BounceMove(mobj_t *mo);
boolean P_CheckSight(mobj_t *t1, mobj_t *t2);
boolean P_TraceBlockingLines(mobj_t *t1, mobj_t *t2);
void P_CheckHoopPosition(mobj_t *hoopthing, fixed_t x, fixed_t y, fixed_t z, fixed_t radius);

boolean P_CheckSector(sector_t *sector, boolean crunch);

void P_DelSeclist(msecnode_t *node);
void P_DelPrecipSeclist(mprecipsecnode_t *node);

void P_CreateSecNodeList(mobj_t *thing, fixed_t x, fixed_t y);
void P_Initsecnode(void);

void P_RadiusAttack(mobj_t *spot, mobj_t *source, fixed_t damagedist, UINT8 damagetype, boolean sightcheck);

fixed_t P_FloorzAtPos(fixed_t x, fixed_t y, fixed_t z, fixed_t height);
fixed_t P_CeilingzAtPos(fixed_t x, fixed_t y, fixed_t z, fixed_t height);
boolean PIT_PushableMoved(mobj_t *thing);

boolean P_DoSpring(mobj_t *spring, mobj_t *object);

fixed_t P_GetFOFTopZAt (ffloor_t *rover, fixed_t x, fixed_t y);
fixed_t P_GetFOFBottomZAt (ffloor_t *rover, fixed_t x, fixed_t y);

fixed_t P_VeryTopOfFOF (ffloor_t *rover);
fixed_t P_VeryBottomOfFOF (ffloor_t *rover);

//
// P_SETUP
//
extern UINT8 *rejectmatrix; // for fast sight rejection
extern INT32 *blockmaplump; // offsets in blockmap are from here
extern INT32 *blockmap; // Big blockmap
extern INT32 bmapwidth;
extern INT32 bmapheight; // in mapblocks
extern fixed_t bmaporgx;
extern fixed_t bmaporgy; // origin of block map
extern mobj_t **blocklinks; // for thing chains

//
// P_INTER
//
typedef struct BasicFF_s
{
	INT32 ForceX; ///< The X of the Force's Vel
	INT32 ForceY; ///< The Y of the Force's Vel
	const player_t *player; ///< Player of Rumble
	//All
	UINT32 Duration; ///< The total duration of the effect, in microseconds
	INT32 Gain; ///< /The gain to be applied to the effect, in the range from 0 through 10,000.
	//All, CONSTANTFORCE �10,000 to 10,000
	INT32 Magnitude; ///< Magnitude of the effect, in the range from 0 through 10,000.
} BasicFF_t;

/* Damage/death types, for P_DamageMobj and related */
//// Damage types
#define DMG_NORMAL  0x00
#define DMG_WIPEOUT 0x01 // Normal, but with extra flashy effects
#define DMG_EXPLODE 0x02
#define DMG_SQUISH  0x03
#define DMG_STING   0x04
#define DMG_KARMA   0x05 // Karma Bomb explosion -- works like DMG_EXPLODE, but steals half of their bumpers & deletes the rest
//// Death types - cannot be combined with damage types
#define DMG_INSTAKILL  0x80
#define DMG_DEATHPIT   0x81
#define DMG_CRUSHED    0x82
#define DMG_SPECTATOR  0x83
#define DMG_TIMEOVER   0x84
// Masks
#define DMG_STEAL        0x20 // Flag - can steal bumpers, will only deal damage to players, and will not deal damage outside Battle Mode.
#define DMG_CANTHURTSELF 0x40 // Flag - cannot hurt your self or your team
#define DMG_DEATHMASK    DMG_INSTAKILL // if bit 7 is set, this is a death type instead of a damage type
#define DMG_TYPEMASK     0x0F // Get type without any flags

void P_ForceFeed(const player_t *player, INT32 attack, INT32 fade, tic_t duration, INT32 period);
void P_ForceConstant(const BasicFF_t *FFInfo);
void P_RampConstant(const BasicFF_t *FFInfo, INT32 Start, INT32 End);
void P_RemoveShield(player_t *player);
void P_SpecialStageDamage(player_t *player, mobj_t *inflictor, mobj_t *source);
boolean P_DamageMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype);
void P_KillMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, UINT8 damagetype);
void P_PlayerRingBurst(player_t *player, INT32 num_rings); /// \todo better fit in p_user.c

void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher, boolean heightcheck);
void P_TouchStarPost(mobj_t *starpost, player_t *player, boolean snaptopost);
void P_CheckTimeLimit(void);
void P_CheckPointLimit(void);
boolean P_CheckRacers(void);

boolean P_CanPickupItem(player_t *player, UINT8 weapon);

//
// P_SPEC
//
#include "p_spec.h"

extern INT32 ceilmovesound;

// Factor to scale scrolling effect into mobj-carrying properties = 3/32.
// (This is so scrolling floors and objects on them can move at same speed.)
#define CARRYFACTOR (FRACUNIT-ORIG_FRICTION)

void P_MixUp(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, angle_t angle,
			INT16 starpostx, INT16 starposty, INT16 starpostz,
			INT32 starpostnum, tic_t starposttime, angle_t starpostangle,
			fixed_t starpostscale, angle_t drawangle, INT32 flags2);
boolean P_Teleport(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, angle_t angle, boolean flash, boolean dontstopmove);
boolean P_SetMobjStateNF(mobj_t *mobj, statenum_t state);
boolean P_CheckMissileSpawn(mobj_t *th);
void P_Thrust(mobj_t *mo, angle_t angle, fixed_t move);
void P_ExplodeMissile(mobj_t *mo);
void P_CheckGravity(mobj_t *mo, boolean affect);
void P_SetPitchRollFromSlope(mobj_t *mo, pslope_t *slope);

#endif // __P_LOCAL__
