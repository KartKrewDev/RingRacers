// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_cheat.c
/// \brief Cheat sequence checking

#include "doomdef.h"
#include "g_input.h"
#include "g_game.h"
#include "s_sound.h"

#include "r_local.h"
#include "p_local.h"
#include "p_setup.h"
#include "d_net.h"

#include "m_cheat.h"
#include "k_menu.h"
#include "m_random.h"
#include "m_misc.h"

#include "hu_stuff.h"

#include "v_video.h"
#include "z_zone.h"
#include "p_slopes.h"

#include "k_kart.h" // srb2kart

#include "lua_script.h"
#include "lua_hook.h"

#include "fastcmp.h"

#include "g_party.h"

// Console cheat commands rely on these a lot...
#define REQUIRE_CHEATS if (!CV_CheatsEnabled())\
{ CONS_Printf(M_GetText("Cheats must be enabled.\n")); return; }

#define REQUIRE_OBJECTPLACE if (!objectplacing)\
{ CONS_Printf(M_GetText("OBJECTPLACE must be enabled.\n")); return; }

#define REQUIRE_INLEVEL if (gamestate != GS_LEVEL || demo.playback)\
{ CONS_Printf(M_GetText("You must be in a level (and not a replay) to use this.\n")); return; }

#define REQUIRE_INLEVEL_OR_REPLAY if (gamestate != GS_LEVEL)\
{ CONS_Printf(M_GetText("You must be in a level to use this.\n")); return; }

#define REQUIRE_SINGLEPLAYER if (netgame)\
{ CONS_Printf(M_GetText("This only works offline.\n")); return; }

// command that can be typed at the console!
void Command_CheatNoClip_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_NOCLIP);
}

void Command_CheatGod_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_GOD);
}

void Command_CheatFreeze_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_FREEZE);
}

void Command_Scale_f(void)
{
	const double scaled = atof(COM_Argv(1));
	fixed_t scale = FLOAT_TO_FIXED(scaled);

	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	if (scale < FRACUNIT/100 || scale > 100*FRACUNIT) //COM_Argv(1) will return a null string if they did not give a paramater, so...
	{
		CONS_Printf(M_GetText("scale <value> (0.01-100.0): set player scale size\n"));
		return;
	}

	D_Cheat(consoleplayer, CHEAT_SCALE, scale);
}

void Command_Gravflip_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_FLIP);
}

void Command_Hurtme_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("hurtme <damage>: Damage yourself with specific flags\n"));
		CONS_Printf(M_GetText("Norm 0    Wipe 1    Expl 2    Tumb 3    Stng 4\n"));
		CONS_Printf(M_GetText("Krma 5    Volt 6    Stmb 7    Whmb 8\n"));
		CONS_Printf(M_GetText("Ikll 128  Dpit 129  Crsh 130  Spec 131  Time 132\n"));
		CONS_Printf(M_GetText("Wmbo 16   Stel 32   Hslf 64\n"));
		return;
	}

	D_Cheat(consoleplayer, CHEAT_HURT, atoi(COM_Argv(1)));
}

void Command_Spinout_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_HURT, DMG_NORMAL);
}

void Command_Wipeout_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_HURT, DMG_WIPEOUT);
}

void Command_Explode_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_HURT, DMG_EXPLODE);
}

void Command_Sting_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_HURT, DMG_STING);
}


void Command_Tumble_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_HURT, DMG_TUMBLE);
}

void Command_Stumble_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_HURT, DMG_STUMBLE);
}

void Command_Whumble_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_HURT, DMG_WHUMBLE);
}

void Command_Kill_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_HURT, DMG_INSTAKILL);
}

void Command_RTeleport_f(void)
{
	float x = atof(COM_Argv(1));
	float y = atof(COM_Argv(2));
	float z = atof(COM_Argv(3));

	REQUIRE_CHEATS;
	REQUIRE_INLEVEL_OR_REPLAY;

	if (COM_Argc() != 4)
	{
		CONS_Printf(M_GetText("rteleport <x> <y> <z>: relative teleport to a location\n"));
		return;
	}

	D_Cheat(consoleplayer, CHEAT_RELATIVE_TELEPORT,
			FLOAT_TO_FIXED(x), FLOAT_TO_FIXED(y), FLOAT_TO_FIXED(z));
}

void Command_Teleport_f(void)
{
	float x = atof(COM_Argv(1));
	float y = atof(COM_Argv(2));
	float z = atof(COM_Argv(3));

	REQUIRE_CHEATS;
	REQUIRE_INLEVEL_OR_REPLAY;

	if (COM_Argc() != 4)
	{
		CONS_Printf(M_GetText("teleport <x> <y> <z>: teleport to a location\n"));
		return;
	}

	D_Cheat(consoleplayer, CHEAT_TELEPORT,
			FLOAT_TO_FIXED(x), FLOAT_TO_FIXED(y), FLOAT_TO_FIXED(z));
}

void Command_Skynum_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("skynum <texture name>: change the sky\n"));
		CONS_Printf(M_GetText("Current sky is %s\n"), levelskytexture);
		return;
	}

	CONS_Printf(M_GetText("Previewing sky %s...\n"), COM_Argv(1));

	P_SetupLevelSky(COM_Argv(1), false);
}

void Command_Weather_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER; // TODO: make multiplayer compatible

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("weather <weather#>: change the weather\n"));
		CONS_Printf(M_GetText("Current weather is %d\n"), curWeather);
		return;
	}

	CONS_Printf(M_GetText("Previewing weather %s...\n"), COM_Argv(1));

	P_SwitchWeather(atoi(COM_Argv(1)));
}

#ifdef _DEBUG
// You never thought you needed this, did you? >=D
// Yes, this has the specific purpose of completely screwing you up
// to see if the consistency restoration code can fix you.
// Don't enable this for normal builds...
void Command_CauseCfail_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	if (consoleplayer == serverplayer)
	{
		CONS_Printf(M_GetText("Only remote players can use this command.\n"));
		return;
	}

	P_UnsetThingPosition(players[consoleplayer].mo);
	P_RandomFixed(PR_UNDEFINED);
	P_RandomByte(PR_UNDEFINED);
	P_RandomFixed(PR_UNDEFINED);
	players[consoleplayer].mo->x = 0;
	players[consoleplayer].mo->y = 123311; //cfail cansuled kthxbye
	players[consoleplayer].mo->z = 123311;
	players[consoleplayer].score = 1337;
	players[consoleplayer].rings = 1337;
	players[consoleplayer].mo->destscale = 25;
	P_SetThingPosition(players[consoleplayer].mo);
}
#endif

#ifdef LUA_ALLOW_BYTECODE
void Command_Dumplua_f(void)
{
	if (modifiedgame)
	{
		CONS_Printf(M_GetText("This command has been disabled in modified games, to prevent scripted attacks.\n"));
		return;
	}

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("dumplua <filename>: Compile a plain text lua script (not a wad!) into bytecode.\n"));
		CONS_Alert(CONS_WARNING, M_GetText("This command makes irreversible file changes, please use with caution!\n"));
		return;
	}

	LUA_DumpFile(COM_Argv(1));
}
#endif

void Command_Savecheckpoint_f(void)
{
	mobj_t *thing = players[consoleplayer].mo;

	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	if (camera[G_PartyPosition(consoleplayer)].freecam || players[consoleplayer].spectator)
	{
		D_Cheat(consoleplayer, CHEAT_SAVECHECKPOINT, camera[0].x, camera[0].y, camera[0].z);
	}
	else if (!P_MobjWasRemoved(thing))
	{
		D_Cheat(consoleplayer, CHEAT_SAVECHECKPOINT, thing->x, thing->y, thing->z);
	}
}

//
// Devmode
//

UINT32 cht_debug;

struct debugFlagNames_s const debug_flag_names[] =
{
	{"None", DBG_NONE},
	{"Basic", DBG_BASIC},
	{"Detailed", DBG_DETAILED},
	{"Player", DBG_PLAYER},
	{"Render", DBG_RENDER},
	{"Renderer", DBG_RENDER}, // alt name
	{"Polyobj", DBG_POLYOBJ},
	{"GameLogic", DBG_GAMELOGIC},
	{"Game", DBG_GAMELOGIC}, // alt name
	{"Netplay", DBG_NETPLAY},
	{"Memory", DBG_MEMORY},
	{"Setup", DBG_SETUP},
	{"Lua", DBG_LUA},
	{"RNG", DBG_RNG},
	{"Randomizer", DBG_RNG}, // alt name
	{"Music", DBG_MUSIC},
	{"PwrLv", DBG_PWRLV},
	{"PowerLevel", DBG_PWRLV}, // alt name
	{"Demo", DBG_DEMO},
	{"Replay", DBG_DEMO}, // alt name
	{"Teams", DBG_TEAMS},
	{"Teamplay", DBG_TEAMS}, // alt name
	{NULL, 0}
};

void Command_Devmode_f(void)
{
	size_t argc = 0;

	REQUIRE_CHEATS;

	argc = COM_Argc();
	if (argc > 1)
	{
		UINT32 flags = 0;
		size_t i;

		for (i = 1; i < argc; i++)
		{
			const char *arg = COM_Argv(i);
			size_t j;

			// Try it as a string
			for (j = 0; debug_flag_names[j].str; j++)
			{
				if (stricmp(arg, debug_flag_names[j].str) == 0)
				{
					break;
				}
			}

			if (debug_flag_names[j].str)
			{
				flags |= debug_flag_names[j].flag;
				continue;
			}

			// Try it as a number
			if (arg[0] && arg[0] == '0' &&
				arg[1] && arg[1] == 'x') // Use hexadecimal!
			{
				flags |= axtoi(arg+2);
			}
			else
			{
				flags |= atoi(arg);
			}
		}

		D_Cheat(consoleplayer, CHEAT_DEVMODE, flags);
	}
	else
	{
		CONS_Printf(M_GetText("devmode <flags>: Enable debugging info. Prepend with 0x to use hexadecimal\n"));
		return;
	}
}

void Command_Setrings_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_RINGS, atoi(COM_Argv(1)));
}

void Command_Setspheres_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_SPHERES, atoi(COM_Argv(1)));
}

void Command_Setamps_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_AMPS, atoi(COM_Argv(1)));
}

void Command_Setlives_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_LIVES, atoi(COM_Argv(1)));
}

void Command_Setroundscore_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	D_Cheat(consoleplayer, CHEAT_SCORE, atoi(COM_Argv(1)));
}

void Command_Grayscale_f(void)
{
	REQUIRE_CHEATS;

	COM_ImmedExecute("toggle palette \"\" GRAYPAL");
}

void Command_Goto_f(void)
{
	const INT32 id = atoi(COM_Argv(1));
	const waypoint_t *wayp = K_GetWaypointFromID(id);

	REQUIRE_CHEATS;
	REQUIRE_INLEVEL_OR_REPLAY;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("goto <waypoint id>: teleport to a waypoint\n"));
		return;
	}

	if (wayp == NULL)
	{
		CONS_Alert(CONS_WARNING, "goto %d: no waypoint with that ID\n", id);
		return;
	}

	D_Cheat(consoleplayer, CHEAT_TELEPORT,
			wayp->mobj->x, wayp->mobj->y, wayp->mobj->z);
}

void Command_Angle_f(void)
{
	const float anglef = atof(COM_Argv(1));
	const angle_t angle = FixedAngle(FLOAT_TO_FIXED(anglef));

	REQUIRE_CHEATS;
	REQUIRE_INLEVEL_OR_REPLAY;

	D_Cheat(consoleplayer, CHEAT_ANGLE, angle);
}

void Command_RespawnAt_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("respawnat <waypoint id>: lightsnake to a specific waypoint\n"));
		return;
	}

	D_Cheat(consoleplayer, CHEAT_RESPAWNAT, atoi(COM_Argv(1)));
}

void Command_GotoSkybox_f(void)
{
	REQUIRE_CHEATS;
	REQUIRE_INLEVEL_OR_REPLAY;

	mobj_t *skybox = players[consoleplayer].skybox.viewpoint;

	if (P_MobjWasRemoved(skybox))
	{
		CONS_Printf("There is no visible skybox for local Player 1.\n");
		return;
	}

	D_Cheat(consoleplayer, CHEAT_TELEPORT, skybox->x, skybox->y, skybox->z);
	D_Cheat(consoleplayer, CHEAT_ANGLE, skybox->angle);
}

//
// OBJECTPLACE (and related variables)
//

boolean objectplacing = false;
mobjtype_t op_currentthing = 0; // For the object placement mode
UINT16 op_currentdoomednum = 0; // For display, etc
UINT32 op_displayflags = 0; // for display in ST_stuff

static pflags_t op_oldpflags = 0;
static mobjflag_t op_oldflags1 = 0;
static mobjflag2_t op_oldflags2 = 0;
static UINT32 op_oldeflags = 0;
static fixed_t op_oldmomx = 0, op_oldmomy = 0, op_oldmomz = 0, op_oldheight = 0;
static statenum_t op_oldstate = 0;
static UINT16 op_oldcolor = 0;

//
// Static calculation / common output help
//
static void OP_CycleThings(INT32 amt)
{
	INT32 add = (amt > 0 ? 1 : -1);

	while (amt)
	{
		do
		{
			op_currentthing += add;
			if (op_currentthing <= 0)
				op_currentthing = NUMMOBJTYPES-1;
			if (op_currentthing >= NUMMOBJTYPES)
				op_currentthing = 0;
		} while
		(mobjinfo[op_currentthing].doomednum == -1
			|| mobjinfo[op_currentthing].flags & MF_NOSECTOR
			|| (states[mobjinfo[op_currentthing].spawnstate].sprite == SPR_NULL
			 && states[mobjinfo[op_currentthing].seestate].sprite == SPR_NULL)
		);
		amt -= add;
	}

	// HACK, minus has SPR_NULL sprite
	if (states[mobjinfo[op_currentthing].spawnstate].sprite == SPR_NULL)
	{
		states[S_OBJPLACE_DUMMY].sprite = states[mobjinfo[op_currentthing].seestate].sprite;
		states[S_OBJPLACE_DUMMY].frame = states[mobjinfo[op_currentthing].seestate].frame;
	}
	else
	{
		states[S_OBJPLACE_DUMMY].sprite = states[mobjinfo[op_currentthing].spawnstate].sprite;
		states[S_OBJPLACE_DUMMY].frame = states[mobjinfo[op_currentthing].spawnstate].frame;
	}
	if (players[0].mo->eflags & MFE_VERTICALFLIP) // correct z when flipped
		players[0].mo->z += players[0].mo->height - FixedMul(mobjinfo[op_currentthing].height, players[0].mo->scale);
	players[0].mo->height = FixedMul(mobjinfo[op_currentthing].height, players[0].mo->scale);
	P_SetPlayerMobjState(players[0].mo, S_OBJPLACE_DUMMY);

	op_currentdoomednum = mobjinfo[op_currentthing].doomednum;
}

static boolean OP_HeightOkay(player_t *player, UINT8 ceiling)
{
	sector_t *sec = player->mo->subsector->sector;

	if (ceiling)
	{
		// Truncate position to match where mapthing would be when spawned
		// (this applies to every further P_GetSlopeZAt call as well)
		fixed_t cheight = P_GetSectorCeilingZAt(sec, player->mo->x & 0xFFFF0000, player->mo->y & 0xFFFF0000);

		if (((cheight - player->mo->z - player->mo->height)>>FRACBITS) >= (1 << (16-ZSHIFT)))
		{
			CONS_Printf(M_GetText("Sorry, you're too %s to place this object (max: %d %s).\n"), M_GetText("low"),
				(1 << (16-ZSHIFT)), M_GetText("below top ceiling"));
			return false;
		}
	}
	else
	{
		fixed_t fheight = P_GetSectorFloorZAt(sec, player->mo->x & 0xFFFF0000, player->mo->y & 0xFFFF0000);
		if (((player->mo->z - fheight)>>FRACBITS) >= (1 << (16-ZSHIFT)))
		{
			CONS_Printf(M_GetText("Sorry, you're too %s to place this object (max: %d %s).\n"), M_GetText("high"),
				(1 << (16-ZSHIFT)), M_GetText("above bottom floor"));
			return false;
		}
	}
	return true;
}

static mapthing_t *OP_CreateNewMapThing(player_t *player, UINT16 type, boolean ceiling)
{
	mapthing_t *mt = mapthings;
	sector_t *sec = player->mo->subsector->sector;

	LUA_InvalidateMapthings();

	mapthings = Z_Realloc(mapthings, ++nummapthings * sizeof (*mapthings), PU_LEVEL, NULL);

	// as Z_Realloc can relocate mapthings, quickly go through thinker list and correct
	// the spawnpoints of any objects that have them to the new location
	if (mt != mapthings)
	{
		thinker_t *th;
		mobj_t *mo;

		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			mo = (mobj_t *)th;
			// get offset from mt, which points to old mapthings, then add new location
			if (!mo->spawnpoint)
				continue;
			mo->spawnpoint = (mo->spawnpoint - mt) + mapthings;
		}
	}

	mt = (mapthings+nummapthings-1);

	mt->type = type;
	mt->x = (INT16)(player->mo->x>>FRACBITS);
	mt->y = (INT16)(player->mo->y>>FRACBITS);
	if (ceiling)
	{
		fixed_t cheight = P_GetSectorCeilingZAt(sec, mt->x << FRACBITS, mt->y << FRACBITS);
		mt->z = (UINT16)((cheight - player->mo->z - player->mo->height)>>FRACBITS);
	}
	else
	{
		fixed_t fheight = P_GetSectorFloorZAt(sec, mt->x << FRACBITS, mt->y << FRACBITS);
		mt->z = (UINT16)((player->mo->z - fheight)>>FRACBITS);
	}
	mt->angle = (INT16)(FixedInt(AngleFixed(player->mo->angle)));

	mt->options = (mt->z << ZSHIFT) | (UINT16)cv_opflags.value;
	mt->scale = FixedDiv(player->mo->scale, mapobjectscale);
	mt->spritexscale = FRACUNIT;
	mt->spriteyscale = FRACUNIT;
	memset(mt->thing_args, 0, NUM_MAPTHING_ARGS*sizeof(*mt->thing_args));
	memset(mt->thing_stringargs, 0x00, NUM_MAPTHING_STRINGARGS*sizeof(*mt->thing_stringargs));
	mt->special = 0;
	memset(mt->script_args, 0, NUM_SCRIPT_ARGS*sizeof(*mt->script_args));
	memset(mt->script_stringargs, 0x00, NUM_SCRIPT_STRINGARGS*sizeof(*mt->script_stringargs));
	mt->pitch = mt->roll = 0;
	return mt;
}

//
// Helper functions
//
boolean OP_FreezeObjectplace(void)
{
	if (!objectplacing)
		return false;

	return true;
}

void OP_ResetObjectplace(void)
{
	objectplacing = false;
	op_currentthing = 0;
}

//
// OP_ObjectplaceMovement
//
// Control code for Objectplace mode
//
void OP_ObjectplaceMovement(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;

	player->drawangle = player->mo->angle = player->angleturn;

	ticruned++;
	if (!(cmd->flags & TICCMD_RECEIVED))
		ticmiss++;

	if (cmd->buttons & BT_ACCELERATE)
		player->mo->z += player->mo->scale * cv_speed.value;
	else if (cmd->buttons & BT_BRAKE)
		player->mo->z -= player->mo->scale * cv_speed.value;

	if (cmd->forwardmove != 0)
	{
		P_Thrust(player->mo, player->mo->angle, (cmd->forwardmove*player->mo->scale/MAXPLMOVE)*cv_speed.value);
		P_MoveOrigin(player->mo, player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, player->mo->z);
		player->mo->momx = player->mo->momy = 0;
	}

	if (player->mo->z > player->mo->ceilingz - player->mo->height)
		player->mo->z = player->mo->ceilingz - player->mo->height;
	if (player->mo->z < player->mo->floorz)
		player->mo->z = player->mo->floorz;

	if (cv_opflags.value & MTF_OBJECTFLIP)
		player->mo->eflags |= MFE_VERTICALFLIP;
	else
		player->mo->eflags &= ~MFE_VERTICALFLIP;

	// make sure viewz follows player if in 1st person mode
	player->deltaviewheight = 0;
	player->viewheight = P_GetPlayerViewHeight(player);
	if (player->mo->eflags & MFE_VERTICALFLIP)
		player->viewz = player->mo->z + player->mo->height - player->viewheight;
	else
		player->viewz = player->mo->z + player->viewheight;

	// Display flag information
	// Moved up so it always updates.
	{
		sector_t *sec = player->mo->subsector->sector;

		if (!!(mobjinfo[op_currentthing].flags & MF_SPAWNCEILING) ^ !!(cv_opflags.value & MTF_OBJECTFLIP))
		{
			fixed_t cheight = P_GetSectorCeilingZAt(sec, player->mo->x & 0xFFFF0000, player->mo->y & 0xFFFF0000);
			op_displayflags = (UINT16)((cheight - player->mo->z - mobjinfo[op_currentthing].height)>>FRACBITS);
		}
		else
		{
			fixed_t fheight = P_GetSectorFloorZAt(sec, player->mo->x & 0xFFFF0000, player->mo->y & 0xFFFF0000);
			op_displayflags = (UINT16)((player->mo->z - fheight)>>FRACBITS);
		}
		op_displayflags <<= ZSHIFT;
		op_displayflags |= (UINT16)cv_opflags.value;
	}


	if (player->pflags & PF_STASIS)
	{
		// Are ANY objectplace buttons pressed?  If no, remove flag.
		if (!(cmd->buttons & (BT_ATTACK|BT_DRIFT)))
			player->pflags &= ~PF_STASIS;

		// Do nothing.
		return;
	}

	/*if (cmd->buttons & BT_FORWARD)
	{
		OP_CycleThings(-1);
		player->pflags |= PF_STASIS;
	}
	else*/ if (cmd->buttons & BT_DRIFT)
	{
		OP_CycleThings(1);
		player->pflags |= PF_STASIS;
	}

	// Place an object and add it to the maplist
	if (cmd->buttons & BT_ATTACK)
	{
		mapthing_t *mt;
		mobjtype_t spawnmid = op_currentthing;
		mobjtype_t spawnthing = op_currentdoomednum;
		boolean ceiling;

		player->pflags |= PF_STASIS;

		if (cv_mapthingnum.value > 0 && cv_mapthingnum.value < 4096)
		{
			// find which type to spawn
			for (spawnmid = 0; spawnmid < NUMMOBJTYPES; ++spawnmid)
				if (cv_mapthingnum.value == mobjinfo[spawnmid].doomednum)
					break;

			if (spawnmid == NUMMOBJTYPES)
			{
				CONS_Alert(CONS_ERROR, M_GetText("Can't place an object with mapthingnum %d.\n"), cv_mapthingnum.value);
				return;
			}
			spawnthing = mobjinfo[spawnmid].doomednum;
		}

		ceiling = !!(mobjinfo[spawnmid].flags & MF_SPAWNCEILING) ^ !!(cv_opflags.value & MTF_OBJECTFLIP);
		if (!OP_HeightOkay(player, ceiling))
			return;

		mt = OP_CreateNewMapThing(player, (UINT16)spawnthing, ceiling);
		if (mt->type >= 600 && mt->type <= 611) // Placement patterns
			P_SpawnItemPattern(mt);
		else if (mt->type == 1713) // NiGHTS Hoops
			P_SpawnHoop(mt);
		else
			P_SpawnMapThing(mt);

		CONS_Printf(M_GetText("Placed object type %d at %d, %d, %d, %d\n"), mt->type, mt->x, mt->y, mt->z, mt->angle);
	}
}

//
// Objectplace related commands.
//
/*void Command_Writethings_f(void)
{
	REQUIRE_INLEVEL;
	REQUIRE_OBJECTPLACE;

	P_WriteThings();
}*/

void Command_ObjectPlace_f(void)
{
	size_t thingarg;
	size_t silent;

	REQUIRE_INLEVEL;
	REQUIRE_CHEATS;
	REQUIRE_SINGLEPLAYER; // this one will very likely never be multiplayer compatible...

	silent = COM_CheckParm("-silent");

	thingarg = 2 - ( silent != 1 );

	// Entering objectplace?
	if (!objectplacing || thingarg < COM_Argc())
	{
		if (!objectplacing)
		{
			objectplacing = true;

			if (! silent)
			{
				HU_SetCEchoFlags(V_MONOSPACE);
				HU_SetCEchoDuration(10);
				HU_DoCEcho(va(M_GetText(
					"\\\\\\\\\\\\\\\\\\\\\\\\\x82"
					"   Objectplace Controls:   \x80\\\\"
					"Weapon Next/Prev: Cycle mapthings\\"
					"            Jump: Float up       \\"
					"            Spin: Float down     \\"
					"       Fire Ring: Place object   \\")));
			}

			// Save all the player's data.
			op_oldflags1 = players[0].mo->flags;
			op_oldflags2 = players[0].mo->flags2;
			op_oldeflags = players[0].mo->eflags;
			op_oldpflags = players[0].pflags;
			op_oldmomx = players[0].mo->momx;
			op_oldmomy = players[0].mo->momy;
			op_oldmomz = players[0].mo->momz;
			op_oldheight = players[0].mo->height;
			op_oldstate = S_KART_STILL;
			op_oldcolor = players[0].mo->color; // save color too in case of super/fireflower

			// Remove ALL flags and motion.
			P_UnsetThingPosition(players[0].mo);
			players[0].pflags = 0;
			players[0].mo->flags2 = 0;
			players[0].mo->eflags = 0;
			players[0].mo->flags = (MF_NOCLIP|MF_NOGRAVITY|MF_NOBLOCKMAP);
			players[0].mo->momx = players[0].mo->momy = players[0].mo->momz = 0;
			P_SetThingPosition(players[0].mo);

			// Take away color so things display properly
			players[0].mo->color = 0;

			// Like the classics, recover from death by entering objectplace
			if (players[0].mo->health <= 0)
			{
				players[0].mo->health = 1;
				players[0].deadtimer = 0;
				op_oldflags1 = mobjinfo[MT_PLAYER].flags;
				++players[0].lives;
				players[0].playerstate = PST_LIVE;
			}
			else
				op_oldstate = (statenum_t)(players[0].mo->state-states);
		}

		if (thingarg < COM_Argc())
		{
			UINT16 mapthingnum = atoi(COM_Argv(thingarg));
			mobjtype_t type = P_GetMobjtype(mapthingnum);
			if (type == MT_UNKNOWN)
				CONS_Printf(M_GetText("No mobj type delegated to thing type %d.\n"), mapthingnum);
			else
				op_currentthing = type;
		}

		// If no thing set, then cycle a little
		if (!op_currentthing)
		{
			op_currentthing = 1;
			OP_CycleThings(1);
		}
		else
			OP_CycleThings(0); // sets all necessary height values without cycling op_currentthing

		P_SetPlayerMobjState(players[0].mo, S_OBJPLACE_DUMMY);
	}
	// Or are we leaving it instead?
	else
	{
		objectplacing = false;

		// Don't touch if the mo mysteriously vanished.
		if (!players[0].mo)
			return;

		// If still in dummy state, get out of it.
		if (players[0].mo->state == &states[S_OBJPLACE_DUMMY])
			P_SetPlayerMobjState(players[0].mo, op_oldstate);

		// Reset everything back to how it was before we entered objectplace.
		P_UnsetThingPosition(players[0].mo);
		players[0].mo->flags = op_oldflags1;
		players[0].mo->flags2 = op_oldflags2;
		players[0].mo->eflags = op_oldeflags;
		players[0].pflags = op_oldpflags;
		players[0].mo->momx = op_oldmomx;
		players[0].mo->momy = op_oldmomy;
		players[0].mo->momz = op_oldmomz;
		players[0].mo->height = op_oldheight;
		P_SetThingPosition(players[0].mo);

		// Return their color to normal.
		players[0].mo->color = op_oldcolor;

		// This is necessary for recovery of dying players.
		players[0].flashing = K_GetKartFlashing(&players[0]);
	}
}
