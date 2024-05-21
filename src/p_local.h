// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
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

#ifdef __cplusplus
extern "C" {
#endif

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
#define MAXRADIUS (MAPBLOCKSIZE >> 1)

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

	// Lists after this may exist but they do not call an
	// action in P_RunThinkers
	NUM_ACTIVETHINKERLISTS,

	THINK_PRECIP = NUM_ACTIVETHINKERLISTS,

	NUM_THINKERLISTS
} thinklistnum_t; /**< Thinker lists. */
extern thinker_t thlist[];
extern mobj_t *mobjcache;

void P_InitThinkers(void);
void P_InvalidateThinkersWithoutInit(void);
void P_AddThinker(const thinklistnum_t n, thinker_t *thinker);
void P_RemoveThinker(thinker_t *thinker);
void P_UnlinkThinker(thinker_t *thinker);

//
// P_USER
//
struct camera_t
{
	boolean chase;
	boolean freecam;

	angle_t aiming;

	// Things used by FS cameras.
	fixed_t viewheight;
	angle_t startangle;

	// Camera demobjerization
	// Info for drawing: position.
	fixed_t x, y, z;

	//More drawing info: to determine current sprite.
	angle_t angle; // orientation

	subsector_t *subsector;

	// The closest interval over all contacted Sectors (or Things).
	fixed_t floorz;
	fixed_t ceilingz;

	// From the player
	fixed_t centerfloorz;
	fixed_t centerceilingz;

	// For movement checking.
	fixed_t radius;
	fixed_t height;

	fixed_t relativex;

	// Momentums, used to update position.
	fixed_t momx, momy, momz;
	fixed_t pmomz;

	// SRB2Kart: camera pans while drifting
	fixed_t pan;
	// SRB2Kart: camera pitches on slopes
	angle_t pitch;

	// Freecam: A button was held since entering from menu, so don't move camera
	UINT8 button_a_held;

	// Freecam: aiming needs to be reset after switching from chasecam
	boolean reset_aiming;

	// Hold up/down to pan the camera vertically
	SINT8 dpad_y_held;

	// Interpolation data
	fixed_t old_x, old_y, old_z;
	angle_t old_angle, old_aiming;
};

extern camera_t camera[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_cam_dist[MAXSPLITSCREENPLAYERS], cv_cam_still[MAXSPLITSCREENPLAYERS], cv_cam_height[MAXSPLITSCREENPLAYERS];
extern consvar_t cv_cam_speed[MAXSPLITSCREENPLAYERS], cv_cam_rotate[MAXSPLITSCREENPLAYERS];

extern consvar_t cv_tilting;

extern fixed_t t_cam_dist[MAXSPLITSCREENPLAYERS], t_cam_height[MAXSPLITSCREENPLAYERS], t_cam_rotate[MAXSPLITSCREENPLAYERS];

void P_AddPlayerScore(player_t *player, INT32 amount);
void P_ResetCamera(player_t *player, camera_t *thiscam);
boolean P_TryCameraMove(fixed_t x, fixed_t y, camera_t *thiscam);
void P_SlideCameraMove(camera_t *thiscam);
void P_DemoCameraMovement(camera_t *cam, UINT8 num);
boolean P_MoveChaseCamera(player_t *player, camera_t *thiscam, boolean resetcalled);
void P_ToggleDemoCamera(UINT8 viewnum);

boolean P_PlayerInPain(const player_t *player);
void P_ResetPlayer(player_t *player);
boolean P_PlayerCanDamage(player_t *player, mobj_t *thing);

boolean P_IsPartyPlayer(const player_t *player);
boolean P_IsMachineLocalPlayer(const player_t *player); // TODO: rename back to P_IsLocalPlayer?
boolean P_IsDisplayPlayer(const player_t *player);

void P_SetPlayerAngle(player_t *player, angle_t angle);
void P_ForceLocalAngle(player_t *player, angle_t angle);
boolean P_PlayerFullbright(player_t *player);

boolean P_IsObjectInGoop(const mobj_t *mo);
boolean P_IsObjectOnGround(const mobj_t *mo);
boolean P_IsObjectOnGroundIn(const mobj_t *mo, const sector_t *sec);
boolean P_IsObjectOnRealGround(const mobj_t *mo, const sector_t *sec); // SRB2Kart
#define P_IsObjectFlipped(o) (((o)->eflags & MFE_VERTICALFLIP) == MFE_VERTICALFLIP)
boolean P_InQuicksand(const mobj_t *mo);
boolean P_PlayerHitFloor(player_t *player, boolean fromAir, angle_t oldPitch, angle_t oldRoll);

void P_SetObjectMomZ(mobj_t *mo, fixed_t value, boolean relative);
void P_StartPositionMusic(boolean exact);
void P_EndingMusic(void);
void P_InvincGrowMusic(void);
mobj_t *P_SpawnGhostMobj(mobj_t *mobj);
mobj_t *P_SpawnFakeShadow(mobj_t *mobj, UINT8 offset);
INT32 P_GivePlayerRings(player_t *player, INT32 num_rings);
INT32 P_GivePlayerSpheres(player_t *player, INT32 num_spheres);
void P_GivePlayerLives(player_t *player, INT32 numlives);
UINT8 P_GetNextEmerald(void);
boolean P_AutoPause(void);

void P_ElementalFire(player_t *player, boolean cropcircle);
void P_SpawnSkidDust(player_t *player, fixed_t radius, boolean sound);

void P_SprayCanInit(mobj_t* mobj);

void P_HaltPlayerOrbit(player_t *player);
void P_ExitPlayerOrbit(player_t *player);
boolean P_PlayerOrbit(player_t *player);

void P_TickAltView(altview_t *view);

void P_MovePlayer(player_t *player);
void P_PlayerThink(player_t *player);
void P_PlayerAfterThink(player_t *player);
void P_DoPlayerExit(player_t *player, pflags_t flags);
void P_DoAllPlayersExit(pflags_t flags, boolean givelife);
void P_DoTimeOver(player_t *player);
void P_IncrementGriefValue(player_t *player, UINT32 *grief, const UINT32 griefMax);
void P_CheckRaceGriefing(player_t *player, boolean dopunishment);

void P_ResetPlayerCheats(void);

void P_InstaThrust(mobj_t *mo, angle_t angle, fixed_t move);
fixed_t P_ReturnThrustX(mobj_t *mo, angle_t angle, fixed_t move);
fixed_t P_ReturnThrustY(mobj_t *mo, angle_t angle, fixed_t move);

mobj_t *P_LookForFocusTarget(player_t *player, mobj_t *exclude, SINT8 direction, UINT8 lockonflags);

void P_NukeEnemies(mobj_t *inflictor, mobj_t *source, fixed_t radius);

UINT8 P_FindLowestLap(void);
UINT8 P_FindHighestLap(void);

boolean P_PlayerMoving(INT32 pnum);

void P_PlayRinglossSound(mobj_t *source);
void P_PlayDeathSound(mobj_t *source);
void P_PlayVictorySound(mobj_t *source);

boolean P_GetLives(player_t *player);
boolean P_SpectatorJoinGame(player_t *player);
void P_RestoreMultiMusic(player_t *player);

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

fixed_t P_GetMobjDefaultScale(mobj_t *mobj);
mobj_t *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);

void P_CalculatePrecipFloor(precipmobj_t *mobj);
void P_RecalcPrecipInSector(sector_t *sector);
void P_PrecipitationEffects(void);

void P_RemoveMobj(mobj_t *th);
void P_RemoveSavegameMobj(mobj_t *th);
boolean P_SetPlayerMobjState(mobj_t *mobj, statenum_t state);
boolean P_SetMobjState(mobj_t *mobj, statenum_t state);

void P_RunOverlays(void);
#define OV_DONTSCREENOFFSET	1
#define OV_DONT3DOFFSET		1<<1
#define OV_DONTXYSCALE		1<<2
#define OV_DONTROLL			1<<3

void P_HandleMinecartSegments(mobj_t *mobj);
void P_MobjThinker(mobj_t *mobj);
boolean P_RailThinker(mobj_t *mobj);
void P_PushableThinker(mobj_t *mobj);
void P_SceneryThinker(mobj_t *mobj);


fixed_t P_MobjFloorZ(const mobj_t *mobj, const sector_t *sector, const sector_t *boundsec, fixed_t x, fixed_t y, const line_t *line, boolean lowest, boolean perfect);
fixed_t P_MobjCeilingZ(const mobj_t *mobj, const sector_t *sector, const sector_t *boundsec, fixed_t x, fixed_t y, const line_t *line, boolean lowest, boolean perfect);
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
boolean P_CheckSolidLava(mobj_t *mobj, ffloor_t *rover);
void P_AdjustMobjFloorZ_FFloors(mobj_t *mo, sector_t *sector, UINT8 motype);

mobj_t *P_SpawnMobjFromMobjUnscaled(mobj_t *mobj, fixed_t xofs, fixed_t yofs, fixed_t zofs, mobjtype_t type);
mobj_t *P_SpawnMobjFromMobj(mobj_t *mobj, fixed_t xofs, fixed_t yofs, fixed_t zofs, mobjtype_t type);

mobj_t *P_SpawnMissile(mobj_t *source, mobj_t *dest, mobjtype_t type);
mobj_t *P_SpawnXYZMissile(mobj_t *source, mobj_t *dest, mobjtype_t type, fixed_t x, fixed_t y, fixed_t z);
mobj_t *P_SpawnPointMissile(mobj_t *source, fixed_t xa, fixed_t ya, fixed_t za, mobjtype_t type, fixed_t x, fixed_t y, fixed_t z);
mobj_t *P_SpawnAlteredDirectionMissile(mobj_t *source, mobjtype_t type, fixed_t x, fixed_t y, fixed_t z, INT32 shiftingAngle);
mobj_t *P_SPMAngle(mobj_t *source, mobjtype_t type, angle_t angle, UINT8 aimtype, UINT32 flags2);
#define P_SpawnPlayerMissile(s,t,f) P_SPMAngle(s,t,s->angle,true,f)
#define P_SpawnNameFinder(s,t) P_SPMAngle(s,t,s->angle,true,0)
void P_ColorTeamMissile(mobj_t *missile, player_t *source);
SINT8 P_MobjFlip(const mobj_t *mobj);
fixed_t P_GetMobjGravity(mobj_t *mo);

void P_CalcChasePostImg(player_t *player, camera_t *thiscam);
boolean P_CameraThinker(player_t *player, camera_t *thiscam, boolean resetcalled);

void P_Attract(mobj_t *source, mobj_t *enemy, boolean nightsgrab);
mobj_t *P_GetClosestAxis(mobj_t *source);

boolean P_CanRunOnWater(mobj_t *mobj, ffloor_t *rover);
boolean P_CheckSolidFFloorSurface(mobj_t *mobj, ffloor_t *rover);

void P_MaceRotate(mobj_t *center, INT32 baserot, INT32 baseprevrot);

void P_FlashPal(player_t *pl, UINT16 type, UINT16 duration);
#define PAL_WHITE    1
#define PAL_MIXUP    2
#define PAL_RECYCLE  3
#define PAL_NUKE     4

boolean P_MobjIsFrozen(mobj_t *mobj);

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
void P_InternalFlickySetColor(mobj_t *actor, UINT8 color);
#define P_IsFlickyCenter(type) (type > MT_FLICKY_01 && type < MT_SEED && (type - MT_FLICKY_01) % 2 ? 1 : 0)
void P_InternalFlickyBubble(mobj_t *actor);
void P_InternalFlickyFly(mobj_t *actor, fixed_t flyspeed, fixed_t targetdist, fixed_t chasez);
void P_InternalFlickyHop(mobj_t *actor, fixed_t momz, fixed_t momh, angle_t angle);

//
// P_MAP
//

struct tm_t
{
	mobj_t *thing;
	fixed_t x, y;
	fixed_t bbox[4];
	INT32 flags;

	precipmobj_t *precipthing;
	fixed_t precipbbox[4];

	// If "floatok" true, move would be ok
	// if within "tm.floorz - tm.ceilingz".
	boolean floatok;

	fixed_t floorz, ceilingz;
	fixed_t dropoffz, drpoffceilz; // drop-off floor/ceiling heights
	mobj_t *floorthing; // the thing corresponding to tm.floorz or NULL if tm.floorz is from a sector
	mobj_t *hitthing; // the solid thing you bumped into (for collisions)
	ffloor_t *floorrover, *ceilingrover;
	pslope_t *floorslope, *ceilingslope;
	INT32 floorpic, ceilingpic;
	fixed_t floorstep, ceilingstep;

	// keep track of the line that lowers the ceiling,
	// so missiles don't explode against sky hack walls
	line_t *ceilingline;

	// P_CheckPosition: this position blocks movement
	boolean blocking;

	// P_CheckPosition: set this before each call to
	// P_CheckPosition to enable a line sweep on collided
	// lines
	boolean sweep;

	// sweep: max step up at tm.x, tm.y
	fixed_t maxstep;
};

extern tm_t g_tm;

void P_RestoreTMStruct(tm_t tmrestore);

extern camera_t *mapcampointer;

/* cphipps 2004/08/30 */
extern void P_MapStart(void);
extern void P_MapEnd(void);

extern msecnode_t *sector_list;

extern mprecipsecnode_t *precipsector_list;

void P_UnsetThingPosition(mobj_t *thing);
void P_SetThingPosition(mobj_t *thing);
void P_SetUnderlayPosition(mobj_t *thing);

struct TryMoveResult_t
{
	boolean success;
	line_t *line;
	mobj_t *mo;
	vector2_t normal;
};

boolean P_CheckPosition(mobj_t *thing, fixed_t x, fixed_t y, TryMoveResult_t *result);
boolean P_CheckMove(mobj_t *thing, fixed_t x, fixed_t y, boolean allowdropoff, TryMoveResult_t *result);
boolean P_TryMove(mobj_t *thing, fixed_t x, fixed_t y, boolean allowdropoff, TryMoveResult_t *result);
boolean P_SceneryTryMove(mobj_t *thing, fixed_t x, fixed_t y, TryMoveResult_t *result);

void P_TestLine(line_t *ld);
void P_ClearTestLines(void);
line_t *P_SweepTestLines(fixed_t ax, fixed_t ay, fixed_t bx, fixed_t by, fixed_t r, vector2_t *return_normal);

boolean P_IsLineBlocking(const line_t *ld, const mobj_t *thing);
boolean P_IsLineTripWire(const line_t *ld);
boolean P_CheckCameraPosition(fixed_t x, fixed_t y, camera_t *thiscam);
fixed_t P_BaseStepUp(void);
fixed_t P_GetThingStepUp(mobj_t *thing, fixed_t destX, fixed_t destY);
boolean P_Move(mobj_t *actor, fixed_t speed);
boolean P_SetOrigin(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z);
boolean P_MoveOrigin(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z);
void P_SlideMove(mobj_t *mo, TryMoveResult_t *result);
void P_BounceMove(mobj_t *mo, TryMoveResult_t *result);
boolean P_CheckSight(mobj_t *t1, mobj_t *t2);
boolean P_TraceBlockingLines(mobj_t *t1, mobj_t *t2);
boolean P_TraceBotTraversal(mobj_t *t1, mobj_t *t2);
boolean P_TraceWaypointTraversal(mobj_t *t1, mobj_t *t2);
void P_CheckHoopPosition(mobj_t *hoopthing, fixed_t x, fixed_t y, fixed_t z, fixed_t radius);

boolean P_CheckSector(sector_t *sector, boolean crunch);

void P_DelSeclist(msecnode_t *node);
void P_DelPrecipSeclist(mprecipsecnode_t *node);

void P_CreateSecNodeList(mobj_t *thing, fixed_t x, fixed_t y);
void P_Initsecnode(void);

void P_RadiusAttack(mobj_t *spot, mobj_t *source, fixed_t damagedist, UINT8 damagetype, boolean sightcheck);

fixed_t P_FloorzAtPos(fixed_t x, fixed_t y, fixed_t z, fixed_t height);
fixed_t P_CeilingzAtPos(fixed_t x, fixed_t y, fixed_t z, fixed_t height);
BlockItReturn_t PIT_PushableMoved(mobj_t *thing);

void P_DoSpringEx(mobj_t *object, fixed_t scaleVal, fixed_t vertispeed, fixed_t horizspeed, angle_t finalAngle, UINT16 starcolor);
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
extern precipmobj_t **precipblocklinks; // special blockmap for precip rendering

extern struct minimapinfo
{
	patch_t *minimap_pic;
	INT32 min_x, min_y;
	INT32 max_x, max_y;
	INT32 map_w, map_h;
	INT32 minimap_w, minimap_h;
	fixed_t offs_x, offs_y;
	fixed_t zoom;
} minimapinfo;

//
// P_INTER
//
struct BasicFF_t
{
	INT32 ForceX; ///< The X of the Force's Vel
	INT32 ForceY; ///< The Y of the Force's Vel
	const player_t *player; ///< Player of Rumble
	//All
	UINT32 Duration; ///< The total duration of the effect, in microseconds
	INT32 Gain; ///< /The gain to be applied to the effect, in the range from 0 through 10,000.
	//All, CONSTANTFORCE �10,000 to 10,000
	INT32 Magnitude; ///< Magnitude of the effect, in the range from 0 through 10,000.
};

/* Damage/death types, for P_DamageMobj and related */
//// Damage types
#define DMG_NORMAL  0x00
#define DMG_WIPEOUT 0x01 // Normal, but with extra flashy effects
#define DMG_EXPLODE 0x02
#define DMG_TUMBLE  0x03
#define DMG_STING   0x04
#define DMG_KARMA   0x05 // Karma Bomb explosion -- works like DMG_EXPLODE, but steals half of their bumpers & deletes the rest
#define DMG_VOLTAGE 0x06
#define DMG_STUMBLE 0x07 // Does not award points in Battle
#define DMG_WHUMBLE 0x08 // <-- But this one DOES!
//// Death types - cannot be combined with damage types
#define DMG_INSTAKILL  0x80
#define DMG_DEATHPIT   0x81
#define DMG_CRUSHED    0x82
#define DMG_SPECTATOR  0x83
#define DMG_TIMEOVER   0x84
// Masks
#define DMG_WOMBO		 0x10 // Flag - setting this flag allows objects to damage you if you're already in spinout. The effect is reversed on objects with MF_MISSILE (setting it prevents them from comboing in spinout)
#define DMG_STEAL        0x20 // Flag - can steal bumpers, will only deal damage to players, and will not deal damage outside Battle Mode.
#define DMG_CANTHURTSELF 0x40 // Flag - cannot hurt your self or your team
#define DMG_DEATHMASK    DMG_INSTAKILL // if bit 7 is set, this is a death type instead of a damage type
#define DMG_TYPEMASK     0x0F // Get type without any flags

void P_ForceFeed(const player_t *player, INT32 attack, INT32 fade, tic_t duration, INT32 period);
void P_ForceConstant(const BasicFF_t *FFInfo);
void P_RampConstant(const BasicFF_t *FFInfo, INT32 Start, INT32 End);
void P_SpecialStageDamage(player_t *player, mobj_t *inflictor, mobj_t *source);
boolean P_DamageMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype);
void P_KillMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, UINT8 damagetype);
void P_PlayerRingBurst(player_t *player, INT32 num_rings); /// \todo better fit in p_user.c

void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher, boolean heightcheck);
void P_TouchCheatcheck(mobj_t *cheatcheck, player_t *player, boolean snaptopost);
void P_CheckTimeLimit(void);
void P_CheckPointLimit(void);
boolean P_CheckRacers(void);

boolean P_CanPickupItem(player_t *player, UINT8 weapon);
boolean P_IsPickupCheesy(player_t *player, UINT8 type);
void P_UpdateLastPickup(player_t *player, UINT8 type);
boolean P_CanPickupEmblem(player_t *player, INT32 emblemID);
boolean P_EmblemWasCollected(INT32 emblemID);

void P_TrackRoundConditionTargetDamage(targetdamaging_t targetdamaging);

//
// P_SPEC
//
#include "p_spec.h"

extern INT32 ceilmovesound;

// Factor to scale scrolling effect into mobj-carrying properties = 3/32.
// (This is so scrolling floors and objects on them can move at same speed.)
#define CARRYFACTOR (FRACUNIT-ORIG_FRICTION)

void P_MixUp(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, angle_t angle,
			INT16 cheatcheckx, INT16 cheatchecky, INT16 cheatcheckz,
			INT32 cheatchecknum, tic_t cheatchecktime, angle_t cheatcheckangle,
			fixed_t cheatcheckscale, angle_t drawangle, INT32 flags2);
boolean P_Teleport(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, angle_t angle, boolean flash, boolean dontstopmove);
boolean P_SetMobjStateNF(mobj_t *mobj, statenum_t state);
boolean P_CheckMissileSpawn(mobj_t *th);
void P_Thrust(mobj_t *mo, angle_t angle, fixed_t move);
void P_ExplodeMissile(mobj_t *mo);
void P_CheckGravity(mobj_t *mo, boolean affect);
void P_SetPitchRollFromSlope(mobj_t *mo, pslope_t *slope);
void P_SetPitchRoll(mobj_t *mo, angle_t pitch, angle_t yaw);
void P_ResetPitchRoll(mobj_t *mo);
fixed_t P_ScaleFromMap(fixed_t n, fixed_t scale);
fixed_t P_GetMobjHead(const mobj_t *);
fixed_t P_GetMobjFeet(const mobj_t *);
fixed_t P_GetMobjGround(const mobj_t *);
fixed_t P_GetMobjZMovement(mobj_t *mo);
boolean P_MobjCanChangeFlip(mobj_t *mobj);

void P_InitTIDHash(void);
void P_AddThingTID(mobj_t *mo);
void P_RemoveThingTID(mobj_t *mo);
void P_SetThingTID(mobj_t *mo, mtag_t tid);
mobj_t *P_FindMobjFromTID(mtag_t tid, mobj_t *i, mobj_t *activator);

void P_DeleteMobjStringArgs(mobj_t *mobj);

tic_t P_MobjIsReappearing(const mobj_t *mobj);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __P_LOCAL__
