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
/// \file  g_demo.c
/// \brief Demo recording and playback

#include "doomdef.h"
#include "console.h"
#include "d_main.h"
#include "d_player.h"
#include "d_clisrv.h"
#include "p_setup.h"
#include "i_time.h"
#include "i_system.h"
#include "m_random.h"
#include "p_local.h"
#include "r_draw.h"
#include "r_main.h"
#include "g_game.h"
#include "g_demo.h"
#include "m_misc.h"
#include "k_menu.h"
#include "m_argv.h"
#include "hu_stuff.h"
#include "z_zone.h"
#include "i_video.h"
#include "byteptr.h"
#include "i_joy.h"
#include "r_local.h"
#include "r_skins.h"
#include "y_inter.h"
#include "v_video.h"
#include "lua_hook.h"
#include "md5.h" // demo checksums

// SRB2Kart
#include "d_netfil.h" // nameonly

#include "lua_script.h" // LUA_ArchiveDemo and LUA_UnArchiveDemo
#include "lua_libs.h" // gL (Lua state)

#include "k_kart.h"
#include "k_battle.h"
#include "k_respawn.h"
#include "k_bot.h"
#include "k_color.h"
#include "k_follower.h"

#ifdef TESTERS
static CV_PossibleValue_t recordmultiplayerdemos_cons_t[] = {{2, "Auto Save"}, {0, NULL}};
consvar_t cv_recordmultiplayerdemos = CVAR_INIT ("netdemo_record", "Auto Save", CV_SAVE, recordmultiplayerdemos_cons_t, NULL);
#else
static CV_PossibleValue_t recordmultiplayerdemos_cons_t[] = {{0, "Disabled"}, {1, "Manual Save"}, {2, "Auto Save"}, {0, NULL}};
consvar_t cv_recordmultiplayerdemos = CVAR_INIT ("netdemo_record", "Manual Save", CV_SAVE, recordmultiplayerdemos_cons_t, NULL);
#endif

static CV_PossibleValue_t netdemosyncquality_cons_t[] = {{1, "MIN"}, {35, "MAX"}, {0, NULL}};
consvar_t cv_netdemosyncquality = CVAR_INIT ("netdemo_syncquality", "1", CV_SAVE, netdemosyncquality_cons_t, NULL);

boolean nodrawers; // for comparative timing purposes
boolean noblit; // for comparative timing purposes
tic_t demostarttime; // for comparative timing purposes

static char demoname[MAX_WADPATH];
static UINT8 *demobuffer = NULL;
static UINT8 *demotime_p, *demoinfo_p;
UINT8 *demo_p;
static UINT8 *demoend;
static UINT8 demoflags;
boolean demosynced = true; // console warning message

struct demovars_s demo;

boolean metalrecording; // recording as metal sonic
mobj_t *metalplayback;
static UINT8 *metalbuffer = NULL;
static UINT8 *metal_p;
static UINT16 metalversion;

// extra data stuff (events registered this frame while recording)
static struct {
	UINT8 flags; // EZT flags

	// EZT_COLOR
	UINT8 color, lastcolor;

	// EZT_SCALE
	fixed_t scale, lastscale;

	// EZT_KART
	INT32 kartitem, kartamount, kartbumpers;

	UINT8 desyncframes; // Don't try to resync unless we've been off for two frames, to monkeypatch a few trouble spots

	// EZT_HIT
	UINT16 hits;
	mobj_t **hitlist;
} ghostext[MAXPLAYERS];

// Your naming conventions are stupid and useless.
// There is no conflict here.
demoghost *ghosts = NULL;

//
// DEMO RECORDING
//

#define DEMOVERSION 0x0007
#define DEMOHEADER  "\xF0" "KartReplay" "\x0F"

#define DF_GHOST        0x01 // This demo contains ghost data too!
#define DF_TIMEATTACK   0x02 // This demo is from Time Attack and contains its final completion time & best lap!
#define DF_BREAKTHECAPSULES 0x04 // This demo is from Break the Capsules and contains its final completion time!
#define DF_ATTACKMASK   0x06 // This demo is from ??? attack and contains ???

// 0x08 free

#define DF_NONETMP		0x10 // multiplayer but not netgame

#define DF_LUAVARS		0x20 // this demo contains extra lua vars

#define DF_ATTACKSHIFT  1
#define DF_ENCORE       0x40
#define DF_MULTIPLAYER  0x80 // This demo was recorded in multiplayer mode!

#define DEMO_SPECTATOR	0x01
#define DEMO_KICKSTART	0x02
#define DEMO_SHRINKME	0x04

// For demos
#define ZT_FWD		0x01
#define ZT_SIDE		0x02
#define ZT_TURNING	0x04
#define ZT_THROWDIR	0x08
#define ZT_BUTTONS	0x10
#define ZT_AIMING	0x20
#define ZT_LATENCY	0x40
#define ZT_FLAGS	0x80
// OUT OF ZIPTICS...

#define DEMOMARKER 0x80 // demoend

UINT8 demo_extradata[MAXPLAYERS];
UINT8 demo_writerng; // 0=no, 1=yes, 2=yes but on a timeout
static ticcmd_t oldcmd[MAXPLAYERS];

#define METALDEATH 0x44
#define METALSNICE 0x69

#define DW_END        0xFF // End of extradata block
#define DW_RNG        0xFE // Check RNG seed!

#define DW_EXTRASTUFF 0xFE // Numbers below this are reserved for writing player slot data

// Below consts are only used for demo extrainfo sections
#define DW_STANDING 0x00

// For Metal Sonic and time attack ghosts
#define GZT_XYZ    0x01
#define GZT_MOMXY  0x02
#define GZT_MOMZ   0x04
#define GZT_ANGLE  0x08
#define GZT_FRAME  0x10 // Animation frame
#define GZT_SPR2   0x20 // Player animations
#define GZT_EXTRA  0x40
#define GZT_FOLLOW 0x80 // Followmobj

// GZT_EXTRA flags
#define EZT_COLOR  0x001 // Changed color (Super transformation, Mario fireflowers/invulnerability, etc.)
#define EZT_FLIP   0x002 // Reversed gravity
#define EZT_SCALE  0x004 // Changed size
#define EZT_HIT    0x008 // Damaged a mobj
#define EZT_SPRITE 0x010 // Changed sprite set completely out of PLAY (NiGHTS, SOCs, whatever)
#define EZT_KART   0x020 // SRB2Kart: Changed current held item/quantity and bumpers for battle

// GZT_FOLLOW flags
#define FZT_SPAWNED 0x01 // just been spawned
#define FZT_SKIN 0x02 // has skin
#define FZT_LINKDRAW 0x04 // has linkdraw (combine with spawned only)
#define FZT_COLORIZED 0x08 // colorized (ditto)
#define FZT_SCALE 0x10 // different scale to object
// spare FZT slots 0x20 to 0x80

static mobj_t oldmetal, oldghost[MAXPLAYERS];

void G_SaveMetal(UINT8 **buffer)
{
	I_Assert(buffer != NULL && *buffer != NULL);

	WRITEUINT32(*buffer, metal_p - metalbuffer);
}

void G_LoadMetal(UINT8 **buffer)
{
	I_Assert(buffer != NULL && *buffer != NULL);

	G_DoPlayMetal();
	metal_p = metalbuffer + READUINT32(*buffer);
}

// Finds a skin with the closest stats if the expected skin doesn't exist.
static INT32 GetSkinNumClosestToStats(UINT8 kartspeed, UINT8 kartweight)
{
	INT32 i, closest_skin = 0;
	UINT8 closest_stats = UINT8_MAX, stat_diff;

	for (i = 0; i < numskins; i++)
	{
		stat_diff = abs(skins[i].kartspeed - kartspeed) + abs(skins[i].kartweight - kartweight);
		if (stat_diff < closest_stats)
		{
			closest_stats = stat_diff;
			closest_skin = i;
		}
	}

	return closest_skin;
}

static void FindClosestSkinForStats(UINT32 p, UINT8 kartspeed, UINT8 kartweight)
{
	INT32 closest_skin = GetSkinNumClosestToStats(kartspeed, kartweight);

	//CONS_Printf("Using %s instead...\n", skins[closest_skin].name);
	SetPlayerSkinByNum(p, closest_skin);
}

void G_ReadDemoExtraData(void)
{
	INT32 p, extradata, i;
	char name[17];

	if (leveltime > starttime)
	{
		rewind_t *rewind = CL_SaveRewindPoint(demo_p - demobuffer);
		if (rewind)
		{
			memcpy(rewind->oldcmd, oldcmd, sizeof (oldcmd));
			memcpy(rewind->oldghost, oldghost, sizeof (oldghost));
		}
	}

	memset(name, '\0', 17);

	p = READUINT8(demo_p);

	while (p < DW_EXTRASTUFF)
	{
		extradata = READUINT8(demo_p);

		if (extradata & DXD_RESPAWN)
		{
			if (players[p].mo)
			{
				// Is this how this should work..?
				K_DoIngameRespawn(&players[p]);
			}
		}
		if (extradata & DXD_SKIN)
		{
			UINT8 kartspeed, kartweight;

			// Skin
			M_Memcpy(name, demo_p, 16);
			demo_p += 16;
			SetPlayerSkin(p, name);

			kartspeed = READUINT8(demo_p);
			kartweight = READUINT8(demo_p);

			if (stricmp(skins[players[p].skin].name, name) != 0)
				FindClosestSkinForStats(p, kartspeed, kartweight);

			players[p].kartspeed = kartspeed;
			players[p].kartweight = kartweight;
		}
		if (extradata & DXD_COLOR)
		{
			// Color
			M_Memcpy(name, demo_p, 16);
			demo_p += 16;
			for (i = 0; i < numskincolors; i++)
				if (!stricmp(skincolors[i].name, name))				// SRB2kart
				{
					players[p].skincolor = i;
					if (players[p].mo)
						players[p].mo->color = i;
					break;
				}
		}
		if (extradata & DXD_NAME)
		{
			// Name
			M_Memcpy(player_names[p],demo_p,16);
			demo_p += 16;
		}
		if (extradata & DXD_FOLLOWER)
		{
			// Set our follower
			M_Memcpy(name, demo_p, 16);
			demo_p += 16;
			K_SetFollowerByName(p, name);

			// Follower's color
			M_Memcpy(name, demo_p, 16);
			demo_p += 16;
			for (i = 0; i < numskincolors +2; i++)	// +2 because of Match and Opposite
			{
					if (!stricmp(Followercolor_cons_t[i].strvalue, name))
					{
							players[p].followercolor = i;
							break;
					}
			}
		}
		if (extradata & DXD_PLAYSTATE)
		{
			i = READUINT8(demo_p);

			switch (i) {
			case DXD_PST_PLAYING:
				players[p].pflags |= PF_WANTSTOJOIN; // fuck you
				//CONS_Printf("player %s is despectating on tic %d\n", player_names[p], leveltime);
				break;

			case DXD_PST_SPECTATING:
				players[p].pflags &= ~PF_WANTSTOJOIN; // double-fuck you
				if (!playeringame[p])
				{
					CL_ClearPlayer(p);
					playeringame[p] = true;
					G_AddPlayer(p);
					players[p].spectator = true;
					//CONS_Printf("player %s is joining server on tic %d\n", player_names[p], leveltime);
				}
				else
				{
					//CONS_Printf("player %s is spectating on tic %d\n", player_names[p], leveltime);
					players[p].spectator = true;
					if (players[p].mo)
						P_DamageMobj(players[p].mo, NULL, NULL, 1, DMG_INSTAKILL);
					else
						players[p].playerstate = PST_REBORN;
				}
				break;

			case DXD_PST_LEFT:
				CL_RemovePlayer(p, 0);
				break;
			}

			G_ResetViews();

			// maybe these are necessary?
			K_CheckBumpers();
			P_CheckRacers();
		}
		if (extradata & DXD_WEAPONPREF)
		{
			i = READUINT8(demo_p);
			players[p].pflags &= ~(PF_KICKSTARTACCEL|PF_SHRINKME);
			if (i & 1)
				players[p].pflags |= PF_KICKSTARTACCEL;
			if (i & 2)
				players[p].pflags |= PF_SHRINKME;

			if (leveltime < 2)
			{
				// BAD HACK: No other place I tried to slot this in
				// made it work for the host when they initally host,
				// so this will have to do.
				K_UpdateShrinkCheat(&players[p]);
			}

			//CONS_Printf("weaponpref is %d for player %d\n", i, p);
		}

		p = READUINT8(demo_p);
	}

	while (p != DW_END)
	{
		UINT32 rng;

		switch (p)
		{
		case DW_RNG:
			for (i = 0; i < PRNUMCLASS; i++)
			{
				rng = READUINT32(demo_p);

				if (P_GetRandSeed(i) != rng)
				{
					P_SetRandSeed(i, rng);

					if (demosynced)
						CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced (RNG)!\n"));
					demosynced = false;
				}
			}
		}

		p = READUINT8(demo_p);
	}

	if (!(demoflags & DF_GHOST) && *demo_p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

void G_WriteDemoExtraData(void)
{
	INT32 i;
	char name[16];

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (demo_extradata[i])
		{
			WRITEUINT8(demo_p, i);
			WRITEUINT8(demo_p, demo_extradata[i]);

			//if (demo_extradata[i] & DXD_RESPAWN) has no extra data
			if (demo_extradata[i] & DXD_SKIN)
			{
				// Skin
				memset(name, 0, 16);
				strncpy(name, skins[players[i].skin].name, 16);
				M_Memcpy(demo_p,name,16);
				demo_p += 16;

				WRITEUINT8(demo_p, skins[players[i].skin].kartspeed);
				WRITEUINT8(demo_p, skins[players[i].skin].kartweight);

			}
			if (demo_extradata[i] & DXD_COLOR)
			{
				// Color
				memset(name, 0, 16);
				strncpy(name, skincolors[players[i].skincolor].name, 16);
				M_Memcpy(demo_p,name,16);
				demo_p += 16;
			}
			if (demo_extradata[i] & DXD_NAME)
			{
				// Name
				memset(name, 0, 16);
				strncpy(name, player_names[i], 16);
				M_Memcpy(demo_p,name,16);
				demo_p += 16;
			}
			if (demo_extradata[i] & DXD_FOLLOWER)
			{
				// write follower
				memset(name, 0, 16);
				if (players[i].followerskin == -1)
					strncpy(name, "None", 16);
				else
					strncpy(name, followers[players[i].followerskin].skinname, 16);
				M_Memcpy(demo_p, name, 16);
				demo_p += 16;

				// write follower color
				memset(name, 0, 16);
				strncpy(name, Followercolor_cons_t[(UINT16)(players[i].followercolor+2)].strvalue, 16);	// Not KartColor_Names because followercolor has extra values such as "Match"
				M_Memcpy(demo_p,name,16);
				demo_p += 16;

			}
			if (demo_extradata[i] & DXD_PLAYSTATE)
			{
				demo_writerng = 1;
				if (!playeringame[i])
					WRITEUINT8(demo_p, DXD_PST_LEFT);
				else if (
					players[i].spectator &&
					!(players[i].pflags & PF_WANTSTOJOIN) // <= fuck you specifically
				)
					WRITEUINT8(demo_p, DXD_PST_SPECTATING);
				else
					WRITEUINT8(demo_p, DXD_PST_PLAYING);
			}
			if (demo_extradata[i] & DXD_WEAPONPREF)
			{
				UINT8 prefs = 0;
				if (players[i].pflags & PF_KICKSTARTACCEL)
					prefs |= 1;
				if (players[i].pflags & PF_SHRINKME)
					prefs |= 2;
				WRITEUINT8(demo_p, prefs);
			}
		}

		demo_extradata[i] = 0;
	}

	// May not be necessary, but might as well play it safe...
	if ((leveltime & 255) == 128)
		demo_writerng = 1;

	{
		static UINT8 timeout = 0;

		if (timeout) timeout--;

		if (demo_writerng == 1 || (demo_writerng == 2 && timeout == 0))
		{
			demo_writerng = 0;
			timeout = 16;
			WRITEUINT8(demo_p, DW_RNG);

			for (i = 0; i < PRNUMCLASS; i++)
			{
				WRITEUINT32(demo_p, P_GetRandSeed(i));
			}
		}
	}

	WRITEUINT8(demo_p, DW_END);
}

void G_ReadDemoTiccmd(ticcmd_t *cmd, INT32 playernum)
{
	UINT8 ziptic;

	if (!demo_p || !demo.deferstart)
		return;

	ziptic = READUINT8(demo_p);

	if (ziptic & ZT_FWD)
		oldcmd[playernum].forwardmove = READSINT8(demo_p);
	if (ziptic & ZT_TURNING)
		oldcmd[playernum].turning = READINT16(demo_p);
	if (ziptic & ZT_THROWDIR)
		oldcmd[playernum].throwdir = READINT16(demo_p);
	if (ziptic & ZT_BUTTONS)
		oldcmd[playernum].buttons = READUINT16(demo_p);
	if (ziptic & ZT_AIMING)
		oldcmd[playernum].aiming = READINT16(demo_p);
	if (ziptic & ZT_LATENCY)
		oldcmd[playernum].latency = READUINT8(demo_p);
	if (ziptic & ZT_FLAGS)
		oldcmd[playernum].flags = READUINT8(demo_p);

	G_CopyTiccmd(cmd, &oldcmd[playernum], 1);

	if (!(demoflags & DF_GHOST) && *demo_p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

void G_WriteDemoTiccmd(ticcmd_t *cmd, INT32 playernum)
{
	char ziptic = 0;
	UINT8 *ziptic_p;
	(void)playernum;

	if (!demo_p)
		return;
	ziptic_p = demo_p++; // the ziptic, written at the end of this function

	if (cmd->forwardmove != oldcmd[playernum].forwardmove)
	{
		WRITESINT8(demo_p,cmd->forwardmove);
		oldcmd[playernum].forwardmove = cmd->forwardmove;
		ziptic |= ZT_FWD;
	}

	if (cmd->turning != oldcmd[playernum].turning)
	{
		WRITEINT16(demo_p,cmd->turning);
		oldcmd[playernum].turning = cmd->turning;
		ziptic |= ZT_TURNING;
	}

	if (cmd->throwdir != oldcmd[playernum].throwdir)
	{
		WRITEINT16(demo_p,cmd->throwdir);
		oldcmd[playernum].throwdir = cmd->throwdir;
		ziptic |= ZT_THROWDIR;
	}

	if (cmd->buttons != oldcmd[playernum].buttons)
	{
		WRITEUINT16(demo_p,cmd->buttons);
		oldcmd[playernum].buttons = cmd->buttons;
		ziptic |= ZT_BUTTONS;
	}

	if (cmd->aiming != oldcmd[playernum].aiming)
	{
		WRITEINT16(demo_p,cmd->aiming);
		oldcmd[playernum].aiming = cmd->aiming;
		ziptic |= ZT_AIMING;
	}

	if (cmd->latency != oldcmd[playernum].latency)
	{
		WRITEUINT8(demo_p,cmd->latency);
		oldcmd[playernum].latency = cmd->latency;
		ziptic |= ZT_LATENCY;
	}

	if (cmd->flags != oldcmd[playernum].flags)
	{
		WRITEUINT8(demo_p,cmd->flags);
		oldcmd[playernum].flags = cmd->flags;
		ziptic |= ZT_FLAGS;
	}

	*ziptic_p = ziptic;

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (!(demoflags & DF_GHOST) && ziptic_p > demoend - 9)
	{
		G_CheckDemoStatus(); // no more space
		return;
	}
}

void G_GhostAddFlip(INT32 playernum)
{
	if (!metalrecording && (!demo.recording || !(demoflags & DF_GHOST)))
		return;
	ghostext[playernum].flags |= EZT_FLIP;
}

void G_GhostAddColor(INT32 playernum, ghostcolor_t color)
{
	if (!demo.recording || !(demoflags & DF_GHOST))
		return;
	if (ghostext[playernum].lastcolor == (UINT16)color)
	{
		ghostext[playernum].flags &= ~EZT_COLOR;
		return;
	}
	ghostext[playernum].flags |= EZT_COLOR;
	ghostext[playernum].color = (UINT16)color;
}

void G_GhostAddScale(INT32 playernum, fixed_t scale)
{
	if (!metalrecording && (!demo.recording || !(demoflags & DF_GHOST)))
		return;
	if (ghostext[playernum].lastscale == scale)
	{
		ghostext[playernum].flags &= ~EZT_SCALE;
		return;
	}
	ghostext[playernum].flags |= EZT_SCALE;
	ghostext[playernum].scale = scale;
}

void G_GhostAddHit(INT32 playernum, mobj_t *victim)
{
	if (!demo.recording || !(demoflags & DF_GHOST))
		return;
	ghostext[playernum].flags |= EZT_HIT;
	ghostext[playernum].hits++;
	ghostext[playernum].hitlist = Z_Realloc(ghostext[playernum].hitlist, ghostext[playernum].hits * sizeof(mobj_t *), PU_LEVEL, NULL);
	P_SetTarget(ghostext[playernum].hitlist + (ghostext[playernum].hits-1), victim);
}

void G_WriteAllGhostTics(void)
{
	INT32 i, counter = leveltime;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		if (!players[i].mo)
			continue;

		counter++;

		if (multiplayer && ((counter % cv_netdemosyncquality.value) != 0)) // Only write 1 in this many ghost datas per tic to cut down on multiplayer replay size.
			continue;

		WRITEUINT8(demo_p, i);
		G_WriteGhostTic(players[i].mo, i);
	}
	WRITEUINT8(demo_p, 0xFF);

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (demo_p >= demoend - (13 + 9 + 9))
	{
		G_CheckDemoStatus(); // no more space
		return;
	}
}

void G_WriteGhostTic(mobj_t *ghost, INT32 playernum)
{
	char ziptic = 0;
	UINT8 *ziptic_p;
	UINT32 i;

	if (!demo_p)
		return;
	if (!(demoflags & DF_GHOST))
		return; // No ghost data to write.

	ziptic_p = demo_p++; // the ziptic, written at the end of this function

#define MAXMOM (0xFFFF<<8)

	// GZT_XYZ is only useful if you've moved 256 FRACUNITS or more in a single tic.
	if (abs(ghost->x-oldghost[playernum].x) > MAXMOM
	|| abs(ghost->y-oldghost[playernum].y) > MAXMOM
	|| abs(ghost->z-oldghost[playernum].z) > MAXMOM
	|| ((UINT8)(leveltime & 255) > 0 && (UINT8)(leveltime & 255) <= (UINT8)cv_netdemosyncquality.value)) // Hack to enable slightly nicer resyncing
	{
		oldghost[playernum].x = ghost->x;
		oldghost[playernum].y = ghost->y;
		oldghost[playernum].z = ghost->z;
		ziptic |= GZT_XYZ;
		WRITEFIXED(demo_p,oldghost[playernum].x);
		WRITEFIXED(demo_p,oldghost[playernum].y);
		WRITEFIXED(demo_p,oldghost[playernum].z);
	}
	else
	{
		// For moving normally:
		fixed_t momx = ghost->x-oldghost[playernum].x;
		fixed_t momy = ghost->y-oldghost[playernum].y;

		if (momx != oldghost[playernum].momx
		|| momy != oldghost[playernum].momy)
		{
			oldghost[playernum].momx = momx;
			oldghost[playernum].momy = momy;
			ziptic |= GZT_MOMXY;
			WRITEFIXED(demo_p,momx);
			WRITEFIXED(demo_p,momy);
		}

		momx = ghost->z-oldghost[playernum].z;
		if (momx != oldghost[playernum].momz)
		{
			oldghost[playernum].momz = momx;
			ziptic |= GZT_MOMZ;
			WRITEFIXED(demo_p,momx);
		}

		// This SHOULD set oldghost.x/y/z to match ghost->x/y/z
		oldghost[playernum].x += oldghost[playernum].momx;
		oldghost[playernum].y += oldghost[playernum].momy;
		oldghost[playernum].z +=oldghost[playernum].momz;
	}

#undef MAXMOM

	// Only store the 8 most relevant bits of angle
	// because exact values aren't too easy to discern to begin with when only 8 angles have different sprites
	// and it does not affect this mode of movement at all anyway.
	if (ghost->player && ghost->player->drawangle>>24 != oldghost[playernum].angle)
	{
		oldghost[playernum].angle = ghost->player->drawangle>>24;
		ziptic |= GZT_ANGLE;
		WRITEUINT8(demo_p,oldghost[playernum].angle);
	}

	// Store the sprite frame.
	if ((ghost->frame & FF_FRAMEMASK) != oldghost[playernum].frame)
	{
		oldghost[playernum].frame = (ghost->frame & FF_FRAMEMASK);
		ziptic |= GZT_FRAME;
		WRITEUINT8(demo_p,oldghost[playernum].frame);
	}

	if (ghost->sprite == SPR_PLAY
	&& ghost->sprite2 != oldghost[playernum].sprite2)
	{
		oldghost[playernum].sprite2 = ghost->sprite2;
		ziptic |= GZT_SPR2;
		WRITEUINT8(demo_p,oldghost[playernum].sprite2);
	}

	// Check for sprite set changes
	if (ghost->sprite != oldghost[playernum].sprite)
	{
		oldghost[playernum].sprite = ghost->sprite;
		ghostext[playernum].flags |= EZT_SPRITE;
	}

	if (ghost->player && (
			ghostext[playernum].kartitem != ghost->player->itemtype ||
			ghostext[playernum].kartamount != ghost->player->itemamount ||
			ghostext[playernum].kartbumpers != ghost->player->bumpers
		))
	{
		ghostext[playernum].flags |= EZT_KART;
		ghostext[playernum].kartitem = ghost->player->itemtype;
		ghostext[playernum].kartamount = ghost->player->itemamount;
		ghostext[playernum].kartbumpers = ghost->player->bumpers;
	}

	if (ghostext[playernum].flags)
	{
		ziptic |= GZT_EXTRA;

		if (ghostext[playernum].color == ghostext[playernum].lastcolor)
			ghostext[playernum].flags &= ~EZT_COLOR;
		if (ghostext[playernum].scale == ghostext[playernum].lastscale)
			ghostext[playernum].flags &= ~EZT_SCALE;

		WRITEUINT8(demo_p,ghostext[playernum].flags);
		if (ghostext[playernum].flags & EZT_COLOR)
		{
			WRITEUINT16(demo_p,ghostext[playernum].color);
			ghostext[playernum].lastcolor = ghostext[playernum].color;
		}
		if (ghostext[playernum].flags & EZT_SCALE)
		{
			WRITEFIXED(demo_p,ghostext[playernum].scale);
			ghostext[playernum].lastscale = ghostext[playernum].scale;
		}
		if (ghostext[playernum].flags & EZT_HIT)
		{
			WRITEUINT16(demo_p,ghostext[playernum].hits);
			for (i = 0; i < ghostext[playernum].hits; i++)
			{
				mobj_t *mo = ghostext[playernum].hitlist[i];
				//WRITEUINT32(demo_p,UINT32_MAX); // reserved for some method of determining exactly which mobj this is. (mobjnum doesn't work here.)
				WRITEUINT32(demo_p,mo->type);
				WRITEUINT16(demo_p,(UINT16)mo->health);
				WRITEFIXED(demo_p,mo->x);
				WRITEFIXED(demo_p,mo->y);
				WRITEFIXED(demo_p,mo->z);
				WRITEANGLE(demo_p,mo->angle);
				P_SetTarget(ghostext[playernum].hitlist+i, NULL);
			}
			Z_Free(ghostext[playernum].hitlist);
			ghostext[playernum].hits = 0;
			ghostext[playernum].hitlist = NULL;
		}
		if (ghostext[playernum].flags & EZT_SPRITE)
			WRITEUINT16(demo_p,oldghost[playernum].sprite);
		if (ghostext[playernum].flags & EZT_KART)
		{
			WRITEINT32(demo_p, ghostext[playernum].kartitem);
			WRITEINT32(demo_p, ghostext[playernum].kartamount);
			WRITEINT32(demo_p, ghostext[playernum].kartbumpers);
		}

		ghostext[playernum].flags = 0;
	}

	if (ghost->player && ghost->player->followmobj&& !(ghost->player->followmobj->sprite == SPR_NULL || (ghost->player->followmobj->renderflags & RF_DONTDRAW) == RF_DONTDRAW)) // bloats tails runs but what can ya do
	{
		fixed_t temp;
		UINT8 *followtic_p = demo_p++;
		UINT8 followtic = 0;

		ziptic |= GZT_FOLLOW;

		if (ghost->player->followmobj->skin)
			followtic |= FZT_SKIN;

		if (!(oldghost[playernum].flags2 & MF2_AMBUSH))
		{
			followtic |= FZT_SPAWNED;
			WRITEINT16(demo_p,ghost->player->followmobj->info->height>>FRACBITS);
			if (ghost->player->followmobj->flags2 & MF2_LINKDRAW)
				followtic |= FZT_LINKDRAW;
			if (ghost->player->followmobj->colorized)
				followtic |= FZT_COLORIZED;
			if (followtic & FZT_SKIN)
				WRITEUINT8(demo_p,(UINT8)(((skin_t *)(ghost->player->followmobj->skin))-skins));
			oldghost[playernum].flags2 |= MF2_AMBUSH;
		}

		if (ghost->player->followmobj->scale != ghost->scale)
		{
			followtic |= FZT_SCALE;
			WRITEFIXED(demo_p,ghost->player->followmobj->scale);
		}

		temp = ghost->player->followmobj->x-ghost->x;
		WRITEFIXED(demo_p,temp);
		temp = ghost->player->followmobj->y-ghost->y;
		WRITEFIXED(demo_p,temp);
		temp = ghost->player->followmobj->z-ghost->z;
		WRITEFIXED(demo_p,temp);
		if (followtic & FZT_SKIN)
			WRITEUINT8(demo_p,ghost->player->followmobj->sprite2);
		WRITEUINT16(demo_p,ghost->player->followmobj->sprite);
		WRITEUINT8(demo_p,(ghost->player->followmobj->frame & FF_FRAMEMASK));
		WRITEUINT16(demo_p,ghost->player->followmobj->color);

		*followtic_p = followtic;
	}
	else
		oldghost[playernum].flags2 &= ~MF2_AMBUSH;

	*ziptic_p = ziptic;
}

void G_ConsAllGhostTics(void)
{
	UINT8 p;

	if (!demo_p || !demo.deferstart)
		return;

	p = READUINT8(demo_p);

	while (p != 0xFF)
	{
		G_ConsGhostTic(p);
		p = READUINT8(demo_p);
	}

	if (*demo_p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

// Uses ghost data to do consistency checks on your position.
// This fixes desynchronising demos when fighting eggman.
void G_ConsGhostTic(INT32 playernum)
{
	UINT8 ziptic;
	INT32 px,py,pz,gx,gy,gz;
	mobj_t *testmo;
	UINT32 syncleeway;

	if (!(demoflags & DF_GHOST))
		return; // No ghost data to use.

	testmo = players[playernum].mo;

	// Grab ghost data.
	ziptic = READUINT8(demo_p);
	if (ziptic & GZT_XYZ)
	{
		oldghost[playernum].x = READFIXED(demo_p);
		oldghost[playernum].y = READFIXED(demo_p);
		oldghost[playernum].z = READFIXED(demo_p);
		syncleeway = 0;
	}
	else
	{
		if (ziptic & GZT_MOMXY)
		{
			oldghost[playernum].momx = READFIXED(demo_p);
			oldghost[playernum].momy = READFIXED(demo_p);
		}
		if (ziptic & GZT_MOMZ)
			oldghost[playernum].momz = READFIXED(demo_p);
		oldghost[playernum].x += oldghost[playernum].momx;
		oldghost[playernum].y += oldghost[playernum].momy;
		oldghost[playernum].z += oldghost[playernum].momz;
		syncleeway = FRACUNIT;
	}
	if (ziptic & GZT_ANGLE)
		demo_p++;
	if (ziptic & GZT_FRAME)
		demo_p++;
	if (ziptic & GZT_SPR2)
		demo_p++;

	if (ziptic & GZT_EXTRA)
	{ // But wait, there's more!
		UINT8 xziptic = READUINT8(demo_p);
		if (xziptic & EZT_COLOR)
			demo_p += sizeof(UINT16);
		if (xziptic & EZT_SCALE)
			demo_p += sizeof(fixed_t);
		if (xziptic & EZT_HIT)
		{ // Resync mob damage.
			UINT16 i, count = READUINT16(demo_p);
			thinker_t *th;
			mobj_t *mobj;

			UINT32 type;
			UINT16 health;
			fixed_t x;
			fixed_t y;
			fixed_t z;

			for (i = 0; i < count; i++)
			{
				//demo_p += 4; // reserved.
				type = READUINT32(demo_p);
				health = READUINT16(demo_p);
				x = READFIXED(demo_p);
				y = READFIXED(demo_p);
				z = READFIXED(demo_p);
				demo_p += sizeof(angle_t); // angle, unnecessary for cons.

				mobj = NULL;
				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
						continue;
					mobj = (mobj_t *)th;
					if (mobj->type == (mobjtype_t)type && mobj->x == x && mobj->y == y && mobj->z == z)
						break;
				}
				if (th != &thlist[THINK_MOBJ] && mobj->health != health) // Wasn't damaged?! This is desync! Fix it!
				{
					if (demosynced)
						CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced (health)!\n"));
					demosynced = false;
					P_DamageMobj(mobj, players[0].mo, players[0].mo, 1, DMG_NORMAL);
				}
			}
		}
		if (xziptic & EZT_SPRITE)
			demo_p += sizeof(UINT16);
		if (xziptic & EZT_KART)
		{
			ghostext[playernum].kartitem = READINT32(demo_p);
			ghostext[playernum].kartamount = READINT32(demo_p);
			ghostext[playernum].kartbumpers = READINT32(demo_p);
		}
	}

	if (ziptic & GZT_FOLLOW)
	{ // Even more...
		UINT8 followtic = READUINT8(demo_p);
		if (followtic & FZT_SPAWNED)
		{
			demo_p += sizeof(INT16);
			if (followtic & FZT_SKIN)
				demo_p++;
		}
		if (followtic & FZT_SCALE)
			demo_p += sizeof(fixed_t);
		// momx, momy and momz
		demo_p += sizeof(fixed_t) * 3;
		if (followtic & FZT_SKIN)
			demo_p++;
		demo_p += sizeof(UINT16);
		demo_p++;
		demo_p += sizeof(UINT16);
	}

	if (testmo)
	{
		// Re-synchronise
		px = testmo->x;
		py = testmo->y;
		pz = testmo->z;
		gx = oldghost[playernum].x;
		gy = oldghost[playernum].y;
		gz = oldghost[playernum].z;

		if (abs(px-gx) > syncleeway || abs(py-gy) > syncleeway || abs(pz-gz) > syncleeway)
		{
			ghostext[playernum].desyncframes++;

			if (ghostext[playernum].desyncframes >= 2)
			{
				if (demosynced)
					CONS_Alert(CONS_WARNING, "Demo playback has desynced (player %s)!\n", player_names[playernum]);
				demosynced = false;

				P_UnsetThingPosition(testmo);
				testmo->x = oldghost[playernum].x;
				testmo->y = oldghost[playernum].y;
				P_SetThingPosition(testmo);
				testmo->z = oldghost[playernum].z;

				if (abs(testmo->z - testmo->floorz) < 4*FRACUNIT)
					testmo->z = testmo->floorz; // Sync players to the ground when they're likely supposed to be there...

				ghostext[playernum].desyncframes = 2;
			}
		}
		else
			ghostext[playernum].desyncframes = 0;

		if (players[playernum].itemtype != ghostext[playernum].kartitem
			|| players[playernum].itemamount != ghostext[playernum].kartamount
			|| players[playernum].bumpers != ghostext[playernum].kartbumpers)
		{
			if (demosynced)
				CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced (item/bumpers)!\n"));
			demosynced = false;

			players[playernum].itemtype = ghostext[playernum].kartitem;
			players[playernum].itemamount = ghostext[playernum].kartamount;
			players[playernum].bumpers = ghostext[playernum].kartbumpers;
		}
	}

	if (*demo_p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

void G_GhostTicker(void)
{
	demoghost *g,*p;
	for(g = ghosts, p = NULL; g; g = g->next)
	{
		// Skip normal demo data.
		UINT8 ziptic = READUINT8(g->p);
		UINT8 xziptic = 0;

		while (ziptic != DW_END) // Get rid of extradata stuff
		{
			if (ziptic == 0) // Only support player 0 info for now
			{
				ziptic = READUINT8(g->p);
				if (ziptic & DXD_SKIN)
					g->p += 18; // We _could_ read this info, but it shouldn't change anything in record attack...
				if (ziptic & DXD_COLOR)
					g->p += 16; // Same tbh
				if (ziptic & DXD_NAME)
					g->p += 16; // yea
				if (ziptic & DXD_FOLLOWER)
					g->p += 32; // ok (32 because there's both the skin and the colour)
				if (ziptic & DXD_PLAYSTATE && READUINT8(g->p) != DXD_PST_PLAYING)
					I_Error("Ghost is not a record attack ghost PLAYSTATE"); //@TODO lmao don't blow up like this
				if (ziptic & DXD_WEAPONPREF)
					g->p++; // ditto
			}
			else if (ziptic == DW_RNG)
			{
				INT32 i;
				for (i = 0; i < PRNUMCLASS; i++)
				{
					g->p += 4; // RNG seed
				}
			}
			else
				I_Error("Ghost is not a record attack ghost DXD"); //@TODO lmao don't blow up like this

			ziptic = READUINT8(g->p);
		}

		ziptic = READUINT8(g->p);

		if (ziptic & ZT_FWD)
			g->p++;
		if (ziptic & ZT_TURNING)
			g->p += 2;
		if (ziptic & ZT_THROWDIR)
			g->p += 2;
		if (ziptic & ZT_BUTTONS)
			g->p += 2;
		if (ziptic & ZT_AIMING)
			g->p += 2;
		if (ziptic & ZT_LATENCY)
			g->p++;
		if (ziptic & ZT_FLAGS)
			g->p++;

		// Grab ghost data.
		ziptic = READUINT8(g->p);

		if (ziptic == 0xFF)
			goto skippedghosttic; // Didn't write ghost info this frame
		else if (ziptic != 0)
			I_Error("Ghost is not a record attack ghost ZIPTIC"); //@TODO lmao don't blow up like this
		ziptic = READUINT8(g->p);

		if (ziptic & GZT_XYZ)
		{
			g->oldmo.x = READFIXED(g->p);
			g->oldmo.y = READFIXED(g->p);
			g->oldmo.z = READFIXED(g->p);
		}
		else
		{
			if (ziptic & GZT_MOMXY)
			{
				g->oldmo.momx = READFIXED(g->p);
				g->oldmo.momy = READFIXED(g->p);
			}
			if (ziptic & GZT_MOMZ)
				g->oldmo.momz = READFIXED(g->p);
			g->oldmo.x += g->oldmo.momx;
			g->oldmo.y += g->oldmo.momy;
			g->oldmo.z += g->oldmo.momz;
		}
		if (ziptic & GZT_ANGLE)
			g->oldmo.angle = READUINT8(g->p)<<24;
		if (ziptic & GZT_FRAME)
			g->oldmo.frame = READUINT8(g->p);
		if (ziptic & GZT_SPR2)
			g->oldmo.sprite2 = READUINT8(g->p);

		// Update ghost
		P_UnsetThingPosition(g->mo);
		g->mo->x = g->oldmo.x;
		g->mo->y = g->oldmo.y;
		g->mo->z = g->oldmo.z;
		P_SetThingPosition(g->mo);
		g->mo->angle = g->oldmo.angle;
		g->mo->frame = g->oldmo.frame | tr_trans30<<FF_TRANSSHIFT;
		if (g->fadein)
		{
			g->mo->frame += (((--g->fadein)/6)<<FF_TRANSSHIFT); // this calc never exceeds 9 unless g->fadein is bad, and it's only set once, so...
			g->mo->renderflags &= ~RF_DONTDRAW;
		}
		g->mo->sprite2 = g->oldmo.sprite2;

		if (ziptic & GZT_EXTRA)
		{ // But wait, there's more!
			xziptic = READUINT8(g->p);
			if (xziptic & EZT_COLOR)
			{
				g->color = READUINT16(g->p);
				switch(g->color)
				{
				default:
				case GHC_RETURNSKIN:
					g->mo->skin = g->oldmo.skin;
					/* FALLTHRU */
				case GHC_NORMAL: // Go back to skin color
					g->mo->color = g->oldmo.color;
					break;
				// Handled below
				case GHC_SUPER:
				case GHC_INVINCIBLE:
					break;
				case GHC_FIREFLOWER: // Fireflower
					g->mo->color = SKINCOLOR_WHITE;
					break;
				}
			}
			if (xziptic & EZT_FLIP)
				g->mo->eflags ^= MFE_VERTICALFLIP;
			if (xziptic & EZT_SCALE)
			{
				g->mo->destscale = READFIXED(g->p);
				if (g->mo->destscale != g->mo->scale)
					P_SetScale(g->mo, g->mo->destscale);
			}
			if (xziptic & EZT_HIT)
			{ // Spawn hit poofs for killing things!
				UINT16 i, count = READUINT16(g->p), health;
				UINT32 type;
				fixed_t x,y,z;
				angle_t angle;
				mobj_t *poof;
				for (i = 0; i < count; i++)
				{
					//g->p += 4; // reserved
					type = READUINT32(g->p);
					health = READUINT16(g->p);
					x = READFIXED(g->p);
					y = READFIXED(g->p);
					z = READFIXED(g->p);
					angle = READANGLE(g->p);
					if (!(mobjinfo[type].flags & MF_SHOOTABLE)
					|| !(mobjinfo[type].flags & (MF_ENEMY|MF_MONITOR))
					|| health != 0 || i >= 4) // only spawn for the first 4 hits per frame, to prevent ghosts from splode-spamming too bad.
						continue;
					poof = P_SpawnMobj(x, y, z, MT_GHOST);
					poof->angle = angle;
					poof->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY; // make an ATTEMPT to curb crazy SOCs fucking stuff up...
					poof->health = 0;
					P_SetMobjStateNF(poof, S_XPLD1);
				}
			}
			if (xziptic & EZT_SPRITE)
				g->mo->sprite = READUINT16(g->p);
			if (xziptic & EZT_KART)
				g->p += 12; // kartitem, kartamount, kartbumpers
		}

#define follow g->mo->tracer
		if (ziptic & GZT_FOLLOW)
		{ // Even more...
			UINT8 followtic = READUINT8(g->p);
			fixed_t temp;
			if (followtic & FZT_SPAWNED)
			{
				if (follow)
					P_RemoveMobj(follow);
				P_SetTarget(&follow, P_SpawnMobjFromMobj(g->mo, 0, 0, 0, MT_GHOST));
				P_SetTarget(&follow->tracer, g->mo);
				follow->tics = -1;
				temp = READINT16(g->p)<<FRACBITS;
				follow->height = FixedMul(follow->scale, temp);

				if (followtic & FZT_LINKDRAW)
					follow->flags2 |= MF2_LINKDRAW;

				if (followtic & FZT_COLORIZED)
					follow->colorized = true;

				if (followtic & FZT_SKIN)
					follow->skin = &skins[READUINT8(g->p)];
			}
			if (follow)
			{
				if (followtic & FZT_SCALE)
					follow->destscale = READFIXED(g->p);
				else
					follow->destscale = g->mo->destscale;
				if (follow->destscale != follow->scale)
					P_SetScale(follow, follow->destscale);

				P_UnsetThingPosition(follow);
				temp = (g->version < 0x000e) ? READINT16(g->p)<<8 : READFIXED(g->p);
				follow->x = g->mo->x + temp;
				temp = (g->version < 0x000e) ? READINT16(g->p)<<8 : READFIXED(g->p);
				follow->y = g->mo->y + temp;
				temp = (g->version < 0x000e) ? READINT16(g->p)<<8 : READFIXED(g->p);
				follow->z = g->mo->z + temp;
				P_SetThingPosition(follow);
				if (followtic & FZT_SKIN)
					follow->sprite2 = READUINT8(g->p);
				else
					follow->sprite2 = 0;
				follow->sprite = READUINT16(g->p);
				follow->frame = (READUINT8(g->p)) | (g->mo->frame & FF_TRANSMASK);
				follow->angle = g->mo->angle;
				follow->color = READUINT16(g->p);

				if (!(followtic & FZT_SPAWNED))
				{
					if (xziptic & EZT_FLIP)
					{
						follow->flags2 ^= MF2_OBJECTFLIP;
						follow->eflags ^= MFE_VERTICALFLIP;
					}
				}
			}
		}
		else if (follow)
		{
			P_RemoveMobj(follow);
			P_SetTarget(&follow, NULL);
		}

skippedghosttic:
		// Tick ghost colors (Super and Mario Invincibility flashing)
		switch(g->color)
		{
		case GHC_SUPER: // Super (P_DoSuperStuff)
			if (g->mo->skin)
			{
				skin_t *skin = (skin_t *)g->mo->skin;
				g->mo->color = skin->supercolor;
			}
			else
				g->mo->color = SKINCOLOR_SUPERGOLD1;
			g->mo->color += abs( ( (signed)( (unsigned)leveltime >> 1 ) % 9) - 4);
			break;
		case GHC_INVINCIBLE: // Mario invincibility (P_CheckInvincibilityTimer)
			g->mo->color = K_RainbowColor(leveltime); // Passes through all saturated colours
			break;
		default:
			break;
		}

		if (READUINT8(g->p) != 0xFF) // Make sure there isn't other ghost data here.
			I_Error("Ghost is not a record attack ghost GHOSTEND"); //@TODO lmao don't blow up like this

		// Demo ends after ghost data.
		if (*g->p == DEMOMARKER)
		{
			g->mo->momx = g->mo->momy = g->mo->momz = 0;
#if 1 // freeze frame (maybe more useful for time attackers)
			g->mo->colorized = true;
			if (follow)
				follow->colorized = true;
#else // dissapearing act
			g->mo->fuse = TICRATE;
			if (follow)
				follow->fuse = TICRATE;
#endif
			if (p)
				p->next = g->next;
			else
				ghosts = g->next;
			Z_Free(g);
			continue;
		}

		p = g;
#undef follow
	}
}

// Demo rewinding functions
typedef struct rewindinfo_s {
	tic_t leveltime;

	struct {
		boolean ingame;
		player_t player;
		mobj_t mobj;
	} playerinfo[MAXPLAYERS];

	struct rewindinfo_s *prev;
} rewindinfo_t;

static tic_t currentrewindnum;
static rewindinfo_t *rewindhead = NULL; // Reverse chronological order

void G_InitDemoRewind(void)
{
	CL_ClearRewinds();

	while (rewindhead)
	{
		rewindinfo_t *p = rewindhead->prev;
		Z_Free(rewindhead);
		rewindhead = p;
	}

	currentrewindnum = 0;
}

void G_StoreRewindInfo(void)
{
	static UINT8 timetolog = 8;
	rewindinfo_t *info;
	size_t i;

	if (timetolog-- > 0)
		return;
	timetolog = 8;

	info = Z_Calloc(sizeof(rewindinfo_t), PU_STATIC, NULL);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			info->playerinfo[i].ingame = false;
			continue;
		}

		info->playerinfo[i].ingame = true;
		memcpy(&info->playerinfo[i].player, &players[i], sizeof(player_t));
		if (players[i].mo)
			memcpy(&info->playerinfo[i].mobj, players[i].mo, sizeof(mobj_t));
	}

	info->leveltime = leveltime;
	info->prev = rewindhead;
	rewindhead = info;
}

void G_PreviewRewind(tic_t previewtime)
{
	SINT8 i;
	//size_t j;
	fixed_t tweenvalue = 0;
	rewindinfo_t *info = rewindhead, *next_info = rewindhead;

	if (!info)
		return;

	while (info->leveltime > previewtime && info->prev)
	{
		next_info = info;
		info = info->prev;
	}
	if (info != next_info)
		tweenvalue = FixedDiv(previewtime - info->leveltime, next_info->leveltime - info->leveltime);


	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
		{
			if (info->playerinfo[i].player.mo)
			{
				//@TODO spawn temp object to act as a player display
			}

			continue;
		}

		if (!info->playerinfo[i].ingame || !info->playerinfo[i].player.mo)
		{
			if (players[i].mo)
				players[i].mo->renderflags |= RF_DONTDRAW;

			continue;
		}

		if (!players[i].mo)
			continue; //@TODO spawn temp object to act as a player display

		players[i].mo->renderflags &= ~RF_DONTDRAW;

		P_UnsetThingPosition(players[i].mo);
#define TWEEN(pr) info->playerinfo[i].mobj.pr + FixedMul((INT32) (next_info->playerinfo[i].mobj.pr - info->playerinfo[i].mobj.pr), tweenvalue)
		players[i].mo->x = TWEEN(x);
		players[i].mo->y = TWEEN(y);
		players[i].mo->z = TWEEN(z);
		players[i].mo->angle = TWEEN(angle);
#undef TWEEN
		P_SetThingPosition(players[i].mo);

		players[i].drawangle = info->playerinfo[i].player.drawangle + FixedMul((INT32) (next_info->playerinfo[i].player.drawangle - info->playerinfo[i].player.drawangle), tweenvalue);

		players[i].mo->sprite = info->playerinfo[i].mobj.sprite;
		players[i].mo->sprite2 = info->playerinfo[i].mobj.sprite2;
		players[i].mo->frame = info->playerinfo[i].mobj.frame;

		players[i].mo->hitlag = info->playerinfo[i].mobj.hitlag;

		players[i].realtime = info->playerinfo[i].player.realtime;
		// Genuinely CANNOT be fucked. I can redo lua and I can redo netsaves but I draw the line at this abysmal hack.
		/*for (j = 0; j < NUMKARTSTUFF; j++)
			players[i].kartstuff[j] = info->playerinfo[i].player.kartstuff[j];*/
	}

	for (i = splitscreen; i >= 0; i--)
		P_ResetCamera(&players[displayplayers[i]], &camera[i]);
}

void G_ConfirmRewind(tic_t rewindtime)
{
	SINT8 i;
	tic_t j;
	boolean oldmenuactive = menuactive, oldsounddisabled = sound_disabled;

	INT32 olddp1 = displayplayers[0], olddp2 = displayplayers[1], olddp3 = displayplayers[2], olddp4 = displayplayers[3];
	UINT8 oldss = splitscreen;

	menuactive = false; // Prevent loops

	CV_StealthSetValue(&cv_renderview, 0);

	if (rewindtime <= starttime)
	{
		demo.rewinding = true; // this doesn't APPEAR to cause any misery, and it allows us to prevent running all the wipes again
		G_DoPlayDemo(NULL); // Restart the current demo
	}
	else
	{
		rewind_t *rewind;
		sound_disabled = true; // Prevent sound spam
		demo.rewinding = true;

		rewind = CL_RewindToTime(rewindtime);

		if (rewind)
		{
			demo_p = demobuffer + rewind->demopos;
			memcpy(oldcmd, rewind->oldcmd, sizeof (oldcmd));
			memcpy(oldghost, rewind->oldghost, sizeof (oldghost));
			paused = false;
		}
		else
		{
			demo.rewinding = true;
			G_DoPlayDemo(NULL); // Restart the current demo
		}
	}

	for (j = 0; j < rewindtime && leveltime < rewindtime; j++)
	{
		G_Ticker((j % NEWTICRATERATIO) == 0);
	}

	demo.rewinding = false;
	menuactive = oldmenuactive; // Bring the menu back up
	sound_disabled = oldsounddisabled; // Re-enable SFX

	wipegamestate = gamestate; // No fading back in!

	COM_BufInsertText("renderview on\n");

	if (demo.freecam)
		return;	// don't touch from there

	splitscreen = oldss;
	displayplayers[0] = olddp1;
	displayplayers[1] = olddp2;
	displayplayers[2] = olddp3;
	displayplayers[3] = olddp4;
	R_ExecuteSetViewSize();
	G_ResetViews();

	for (i = splitscreen; i >= 0; i--)
		P_ResetCamera(&players[displayplayers[i]], &camera[i]);
}

void G_ReadMetalTic(mobj_t *metal)
{
	UINT8 ziptic;
	UINT8 xziptic = 0;

	if (!metal_p)
		return;

	if (!metal->health)
	{
		G_StopMetalDemo();
		return;
	}

	switch (*metal_p)
	{
		case METALSNICE:
			break;
		case METALDEATH:
			if (metal->tracer)
				P_RemoveMobj(metal->tracer);
			P_KillMobj(metal, NULL, NULL, DMG_NORMAL);
			/* FALLTHRU */
		case DEMOMARKER:
		default:
			// end of demo data stream
			G_StopMetalDemo();
			return;
	}
	metal_p++;

	ziptic = READUINT8(metal_p);

	// Read changes from the tic
	if (ziptic & GZT_XYZ)
	{
		// make sure the values are read in the right order
		oldmetal.x = READFIXED(metal_p);
		oldmetal.y = READFIXED(metal_p);
		oldmetal.z = READFIXED(metal_p);
		P_MoveOrigin(metal, oldmetal.x, oldmetal.y, oldmetal.z);
		oldmetal.x = metal->x;
		oldmetal.y = metal->y;
		oldmetal.z = metal->z;
	}
	else
	{
		if (ziptic & GZT_MOMXY)
		{
			oldmetal.momx = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
			oldmetal.momy = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
		}
		if (ziptic & GZT_MOMZ)
			oldmetal.momz = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
		oldmetal.x += oldmetal.momx;
		oldmetal.y += oldmetal.momy;
		oldmetal.z += oldmetal.momz;
	}
	if (ziptic & GZT_ANGLE)
		metal->angle = READUINT8(metal_p)<<24;
	if (ziptic & GZT_FRAME)
		oldmetal.frame = READUINT32(metal_p);
	if (ziptic & GZT_SPR2)
		oldmetal.sprite2 = READUINT8(metal_p);

	// Set movement, position, and angle
	// oldmetal contains where you're supposed to be.
	metal->momx = oldmetal.momx;
	metal->momy = oldmetal.momy;
	metal->momz = oldmetal.momz;
	P_UnsetThingPosition(metal);
	metal->x = oldmetal.x;
	metal->y = oldmetal.y;
	metal->z = oldmetal.z;
	P_SetThingPosition(metal);
	metal->frame = oldmetal.frame;
	metal->sprite2 = oldmetal.sprite2;

	if (ziptic & GZT_EXTRA)
	{ // But wait, there's more!
		xziptic = READUINT8(metal_p);
		if (xziptic & EZT_FLIP)
		{
			metal->eflags ^= MFE_VERTICALFLIP;
			metal->flags2 ^= MF2_OBJECTFLIP;
		}
		if (xziptic & EZT_SCALE)
		{
			metal->destscale = READFIXED(metal_p);
			if (metal->destscale != metal->scale)
				P_SetScale(metal, metal->destscale);
		}
		if (xziptic & EZT_SPRITE)
			metal->sprite = READUINT16(metal_p);
	}

#define follow metal->tracer
		if (ziptic & GZT_FOLLOW)
		{ // Even more...
			UINT8 followtic = READUINT8(metal_p);
			fixed_t temp;
			if (followtic & FZT_SPAWNED)
			{
				if (follow)
					P_RemoveMobj(follow);
				P_SetTarget(&follow, P_SpawnMobjFromMobj(metal, 0, 0, 0, MT_GHOST));
				P_SetTarget(&follow->tracer, metal);
				follow->tics = -1;
				temp = READINT16(metal_p)<<FRACBITS;
				follow->height = FixedMul(follow->scale, temp);

				if (followtic & FZT_LINKDRAW)
					follow->flags2 |= MF2_LINKDRAW;

				if (followtic & FZT_COLORIZED)
					follow->colorized = true;

				if (followtic & FZT_SKIN)
					follow->skin = &skins[READUINT8(metal_p)];
			}
			if (follow)
			{
				if (followtic & FZT_SCALE)
					follow->destscale = READFIXED(metal_p);
				else
					follow->destscale = metal->destscale;
				if (follow->destscale != follow->scale)
					P_SetScale(follow, follow->destscale);

				P_UnsetThingPosition(follow);
				temp = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
				follow->x = metal->x + temp;
				temp = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
				follow->y = metal->y + temp;
				temp = (metalversion < 0x000e) ? READINT16(metal_p)<<8 : READFIXED(metal_p);
				follow->z = metal->z + temp;
				P_SetThingPosition(follow);
				if (followtic & FZT_SKIN)
					follow->sprite2 = READUINT8(metal_p);
				else
					follow->sprite2 = 0;
				follow->sprite = READUINT16(metal_p);
				follow->frame = READUINT32(metal_p); // NOT & FF_FRAMEMASK here, so 32 bits
				follow->angle = metal->angle;
				follow->color = READUINT16(metal_p);

				if (!(followtic & FZT_SPAWNED))
				{
					if (xziptic & EZT_FLIP)
					{
						follow->flags2 ^= MF2_OBJECTFLIP;
						follow->eflags ^= MFE_VERTICALFLIP;
					}
				}
			}
		}
		else if (follow)
		{
			P_RemoveMobj(follow);
			P_SetTarget(&follow, NULL);
		}
#undef follow
}

void G_WriteMetalTic(mobj_t *metal)
{
	UINT8 ziptic = 0;
	UINT8 *ziptic_p;

	if (!demo_p) // demo_p will be NULL until the race start linedef executor is activated!
		return;

	WRITEUINT8(demo_p, METALSNICE);
	ziptic_p = demo_p++; // the ziptic, written at the end of this function

	#define MAXMOM (0xFFFF<<8)

	// GZT_XYZ is only useful if you've moved 256 FRACUNITS or more in a single tic.
	if (abs(metal->x-oldmetal.x) > MAXMOM
	|| abs(metal->y-oldmetal.y) > MAXMOM
	|| abs(metal->z-oldmetal.z) > MAXMOM)
	{
		oldmetal.x = metal->x;
		oldmetal.y = metal->y;
		oldmetal.z = metal->z;
		ziptic |= GZT_XYZ;
		WRITEFIXED(demo_p,oldmetal.x);
		WRITEFIXED(demo_p,oldmetal.y);
		WRITEFIXED(demo_p,oldmetal.z);
	}
	else
	{
		// For moving normally:
		// Store movement as a fixed value
		fixed_t momx = metal->x-oldmetal.x;
		fixed_t momy = metal->y-oldmetal.y;
		if (momx != oldmetal.momx
		|| momy != oldmetal.momy)
		{
			oldmetal.momx = momx;
			oldmetal.momy = momy;
			ziptic |= GZT_MOMXY;
			WRITEFIXED(demo_p,momx);
			WRITEFIXED(demo_p,momy);
		}
		momx = metal->z-oldmetal.z;
		if (momx != oldmetal.momz)
		{
			oldmetal.momz = momx;
			ziptic |= GZT_MOMZ;
			WRITEFIXED(demo_p,momx);
		}

		// This SHOULD set oldmetal.x/y/z to match metal->x/y/z
		oldmetal.x += oldmetal.momx;
		oldmetal.y += oldmetal.momy;
		oldmetal.z += oldmetal.momz;
	}

	#undef MAXMOM

	// Only store the 8 most relevant bits of angle
	// because exact values aren't too easy to discern to begin with when only 8 angles have different sprites
	// and it does not affect movement at all anyway.
	if (metal->player && metal->player->drawangle>>24 != oldmetal.angle)
	{
		oldmetal.angle = metal->player->drawangle>>24;
		ziptic |= GZT_ANGLE;
		WRITEUINT8(demo_p,oldmetal.angle);
	}

	// Store the sprite frame.
	if ((metal->frame & FF_FRAMEMASK) != oldmetal.frame)
	{
		oldmetal.frame = metal->frame; // NOT & FF_FRAMEMASK here, so 32 bits
		ziptic |= GZT_FRAME;
		WRITEUINT32(demo_p,oldmetal.frame);
	}

	if (metal->sprite == SPR_PLAY
	&& metal->sprite2 != oldmetal.sprite2)
	{
		oldmetal.sprite2 = metal->sprite2;
		ziptic |= GZT_SPR2;
		WRITEUINT8(demo_p,oldmetal.sprite2);
	}

	// Check for sprite set changes
	if (metal->sprite != oldmetal.sprite)
	{
		oldmetal.sprite = metal->sprite;
		ghostext[0].flags |= EZT_SPRITE;
	}

	if (ghostext[0].flags & ~(EZT_COLOR|EZT_HIT)) // these two aren't handled by metal ever
	{
		ziptic |= GZT_EXTRA;

		if (ghostext[0].scale == ghostext[0].lastscale)
			ghostext[0].flags &= ~EZT_SCALE;

		WRITEUINT8(demo_p,ghostext[0].flags);
		if (ghostext[0].flags & EZT_SCALE)
		{
			WRITEFIXED(demo_p,ghostext[0].scale);
			ghostext[0].lastscale = ghostext[0].scale;
		}
		if (ghostext[0].flags & EZT_SPRITE)
			WRITEUINT16(demo_p,oldmetal.sprite);
		ghostext[0].flags = 0;
	}

	if (metal->player && metal->player->followmobj && !(metal->player->followmobj->sprite == SPR_NULL || (metal->player->followmobj->renderflags & RF_DONTDRAW) == RF_DONTDRAW))
	{
		fixed_t temp;
		UINT8 *followtic_p = demo_p++;
		UINT8 followtic = 0;

		ziptic |= GZT_FOLLOW;

		if (metal->player->followmobj->skin)
			followtic |= FZT_SKIN;

		if (!(oldmetal.flags2 & MF2_AMBUSH))
		{
			followtic |= FZT_SPAWNED;
			WRITEINT16(demo_p,metal->player->followmobj->info->height>>FRACBITS);
			if (metal->player->followmobj->flags2 & MF2_LINKDRAW)
				followtic |= FZT_LINKDRAW;
			if (metal->player->followmobj->colorized)
				followtic |= FZT_COLORIZED;
			if (followtic & FZT_SKIN)
				WRITEUINT8(demo_p,(UINT8)(((skin_t *)(metal->player->followmobj->skin))-skins));
			oldmetal.flags2 |= MF2_AMBUSH;
		}

		if (metal->player->followmobj->scale != metal->scale)
		{
			followtic |= FZT_SCALE;
			WRITEFIXED(demo_p,metal->player->followmobj->scale);
		}

		temp = metal->player->followmobj->x-metal->x;
		WRITEFIXED(demo_p,temp);
		temp = metal->player->followmobj->y-metal->y;
		WRITEFIXED(demo_p,temp);
		temp = metal->player->followmobj->z-metal->z;
		WRITEFIXED(demo_p,temp);
		if (followtic & FZT_SKIN)
			WRITEUINT8(demo_p,metal->player->followmobj->sprite2);
		WRITEUINT16(demo_p,metal->player->followmobj->sprite);
		WRITEUINT32(demo_p,metal->player->followmobj->frame); // NOT & FF_FRAMEMASK here, so 32 bits
		WRITEUINT16(demo_p,metal->player->followmobj->color);

		*followtic_p = followtic;
	}
	else
		oldmetal.flags2 &= ~MF2_AMBUSH;

	*ziptic_p = ziptic;

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (demo_p >= demoend - 32)
	{
		G_StopMetalRecording(false); // no more space
		return;
	}
}

//
// G_RecordDemo
//
void G_RecordDemo(const char *name)
{
	INT32 maxsize;

	strcpy(demoname, name);
	strcat(demoname, ".lmp");
	//@TODO make a maxdemosize cvar
	maxsize = 1024*1024*2;
	if (M_CheckParm("-maxdemo") && M_IsNextParm())
		maxsize = atoi(M_GetNextParm()) * 1024;
//	if (demobuffer)
//		free(demobuffer);
	demo_p = NULL;
	demobuffer = malloc(maxsize);
	demoend = demobuffer + maxsize;

	demo.recording = true;
}

void G_RecordMetal(void)
{
	INT32 maxsize;
	maxsize = 1024*1024;
	if (M_CheckParm("-maxdemo") && M_IsNextParm())
		maxsize = atoi(M_GetNextParm()) * 1024;
	demo_p = NULL;
	demobuffer = malloc(maxsize);
	demoend = demobuffer + maxsize;
	metalrecording = true;
}

void G_BeginRecording(void)
{
	UINT8 i, p;
	char name[MAXCOLORNAME+1];
	player_t *player = &players[consoleplayer];

	char *filename;
	UINT8 totalfiles;
	UINT8 *m;

	if (demo_p)
		return;
	memset(name,0,sizeof(name));

	demo_p = demobuffer;
	demoflags = DF_GHOST|(multiplayer ? DF_MULTIPLAYER : (modeattacking<<DF_ATTACKSHIFT));

	if (multiplayer && !netgame)
		demoflags |= DF_NONETMP;

	if (encoremode)
		demoflags |= DF_ENCORE;

	if (multiplayer)
		demoflags |= DF_LUAVARS;

	// Setup header.
	M_Memcpy(demo_p, DEMOHEADER, 12); demo_p += 12;
	WRITEUINT8(demo_p,VERSION);
	WRITEUINT8(demo_p,SUBVERSION);
	WRITEUINT16(demo_p,DEMOVERSION);

	// Full replay title
	demo_p += 64;
	snprintf(demo.titlename, 64, "%s - %s", G_BuildMapTitle(gamemap), modeattacking ? "Record Attack" : connectedservername);

	// demo checksum
	demo_p += 16;

	// game data
	M_Memcpy(demo_p, "PLAY", 4); demo_p += 4;
	WRITESTRINGN(demo_p, mapheaderinfo[gamemap-1]->lumpname, MAXMAPLUMPNAME);
	M_Memcpy(demo_p, mapmd5, 16); demo_p += 16;

	WRITEUINT8(demo_p, demoflags);
	WRITEUINT8(demo_p, gametype & 0xFF);
	WRITEUINT8(demo_p, numlaps);

	// file list
	m = demo_p;/* file count */
	demo_p += 1;

	totalfiles = 0;
	for (i = mainwads; ++i < numwadfiles; )
		if (wadfiles[i]->important)
	{
		nameonly(( filename = va("%s", wadfiles[i]->filename) ));
		WRITESTRINGN(demo_p, filename, 64);
		WRITEMEM(demo_p, wadfiles[i]->md5sum, 16);

		totalfiles++;
	}

	WRITEUINT8(m, totalfiles);

	switch ((demoflags & DF_ATTACKMASK)>>DF_ATTACKSHIFT)
	{
		case ATTACKING_NONE: // 0
			break;
		case ATTACKING_TIME: // 1
			demotime_p = demo_p;
			WRITEUINT32(demo_p,UINT32_MAX); // time
			WRITEUINT32(demo_p,UINT32_MAX); // lap
			break;
		case ATTACKING_CAPSULES: // 2
			demotime_p = demo_p;
			WRITEUINT32(demo_p,UINT32_MAX); // time
			break;
		default: // 3
			break;
	}

	for (i = 0; i < PRNUMCLASS; i++)
	{
		WRITEUINT32(demo_p, P_GetInitSeed(i));
	}

	// Reserved for extrainfo location from start of file
	demoinfo_p = demo_p;
	WRITEUINT32(demo_p, 0);

	// Save netvar data
	CV_SaveDemoVars(&demo_p);

	// Now store some info for each in-game player

	// Lat' 12/05/19: Do note that for the first game you load, everything that gets saved here is total garbage;
	// The name will always be Player <n>, the skin sonic, the color None and the follower 0. This is only correct on subsequent games.
	// In the case of said first game, the skin and the likes are updated with Got_NameAndColor, which are then saved in extradata for the demo with DXD_SKIN in r_things.c for instance.


	for (p = 0; p < MAXPLAYERS; p++) {
		if (playeringame[p]) {
			player = &players[p];
			WRITEUINT8(demo_p, p);

			i = 0;
			if (player->spectator)
				i |= DEMO_SPECTATOR;
			if (player->pflags & PF_KICKSTARTACCEL)
				i |= DEMO_KICKSTART;
			if (player->pflags & PF_SHRINKME)
				i |= DEMO_SHRINKME;
			WRITEUINT8(demo_p, i);

			// Name
			memset(name, 0, 16);
			strncpy(name, player_names[p], 16);
			M_Memcpy(demo_p,name,16);
			demo_p += 16;

			// Skin
			memset(name, 0, 16);
			strncpy(name, skins[player->skin].name, 16);
			M_Memcpy(demo_p,name,16);
			demo_p += 16;

			// Color
			memset(name, 0, 16);
			strncpy(name, skincolors[player->skincolor].name, 16);
			M_Memcpy(demo_p,name,16);
			demo_p += 16;

			// Save follower's skin name
			// PS: We must check for 'follower' to determine if the followerskin is valid. It's going to be 0 if we don't have a follower, but 0 is also absolutely a valid follower!
			// Doesn't really matter if the follower mobj is valid so long as it exists in a way or another.

			memset(name, 0, 16);
			if (player->follower)
				strncpy(name, followers[player->followerskin].skinname, 16);
			else
				strncpy(name, "None", 16);	// Say we don't have one, then.

			M_Memcpy(demo_p,name,16);
			demo_p += 16;

			// Save follower's colour
			memset(name, 0, 16);
			strncpy(name, Followercolor_cons_t[(UINT16)(player->followercolor+2)].strvalue, 16);	// Not KartColor_Names because followercolor has extra values such as "Match"
			M_Memcpy(demo_p, name, 16);
			demo_p += 16;

			// Score, since Kart uses this to determine where you start on the map
			WRITEUINT32(demo_p, player->score);

			// Power Levels
			WRITEUINT16(demo_p, clientpowerlevels[p][gametype == GT_BATTLE ? PWRLV_BATTLE : PWRLV_RACE]);

			// Kart speed and weight
			WRITEUINT8(demo_p, skins[player->skin].kartspeed);
			WRITEUINT8(demo_p, skins[player->skin].kartweight);

			// And mobjtype_t is best with UINT32 too...
			WRITEUINT32(demo_p, player->followitem);
		}
	}

	WRITEUINT8(demo_p, 0xFF); // Denote the end of the player listing

	// player lua vars, always saved even if empty
	if (demoflags & DF_LUAVARS)
		LUA_Archive(&demo_p);

	memset(&oldcmd,0,sizeof(oldcmd));
	memset(&oldghost,0,sizeof(oldghost));
	memset(&ghostext,0,sizeof(ghostext));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		ghostext[i].lastcolor = ghostext[i].color = GHC_NORMAL;
		ghostext[i].lastscale = ghostext[i].scale = FRACUNIT;

		if (players[i].mo)
		{
			oldghost[i].x = players[i].mo->x;
			oldghost[i].y = players[i].mo->y;
			oldghost[i].z = players[i].mo->z;
			oldghost[i].angle = players[i].mo->angle;

			// preticker started us gravity flipped
			if (players[i].mo->eflags & MFE_VERTICALFLIP)
				ghostext[i].flags |= EZT_FLIP;
		}
	}
}

void G_BeginMetal(void)
{
	mobj_t *mo = players[consoleplayer].mo;

#if 0
	if (demo_p)
		return;
#endif

	demo_p = demobuffer;

	// Write header.
	M_Memcpy(demo_p, DEMOHEADER, 12); demo_p += 12;
	WRITEUINT8(demo_p,VERSION);
	WRITEUINT8(demo_p,SUBVERSION);
	WRITEUINT16(demo_p,DEMOVERSION);

	// demo checksum
	demo_p += 16;

	M_Memcpy(demo_p, "METL", 4); demo_p += 4;

	memset(&ghostext,0,sizeof(ghostext));
	ghostext[0].lastscale = ghostext[0].scale = FRACUNIT;

	// Set up our memory.
	memset(&oldmetal,0,sizeof(oldmetal));
	oldmetal.x = mo->x;
	oldmetal.y = mo->y;
	oldmetal.z = mo->z;
	oldmetal.angle = mo->angle>>24;
}

void G_WriteStanding(UINT8 ranking, char *name, INT32 skinnum, UINT16 color, UINT32 val)
{
	char temp[16];

	if (demoinfo_p && *(UINT32 *)demoinfo_p == 0)
	{
		WRITEUINT8(demo_p, DEMOMARKER); // add the demo end marker
		*(UINT32 *)demoinfo_p = demo_p - demobuffer;
	}

	WRITEUINT8(demo_p, DW_STANDING);
	WRITEUINT8(demo_p, ranking);

	// Name
	memset(temp, 0, 16);
	strncpy(temp, name, 16);
	M_Memcpy(demo_p,temp,16);
	demo_p += 16;

	// Skin
	memset(temp, 0, 16);
	strncpy(temp, skins[skinnum].name, 16);
	M_Memcpy(demo_p,temp,16);
	demo_p += 16;

	// Color
	memset(temp, 0, 16);
	strncpy(temp, skincolors[color].name, 16);
	M_Memcpy(demo_p,temp,16);
	demo_p += 16;

	// Score/time/whatever
	WRITEUINT32(demo_p, val);
}

void G_SetDemoTime(UINT32 ptime, UINT32 plap)
{
	if (!demo.recording || !demotime_p)
		return;
	if (demoflags & DF_TIMEATTACK)
	{
		WRITEUINT32(demotime_p, ptime);
		WRITEUINT32(demotime_p, plap);
		demotime_p = NULL;
	}
	else if (demoflags & DF_BREAKTHECAPSULES)
	{
		WRITEUINT32(demotime_p, ptime);
		(void)plap;
		demotime_p = NULL;
	}
}

static void G_LoadDemoExtraFiles(UINT8 **pp)
{
	UINT8 totalfiles;
	char filename[MAX_WADPATH];
	UINT8 md5sum[16];
	filestatus_t ncs;
	boolean toomany = false;
	boolean alreadyloaded;
	UINT8 i, j;

	totalfiles = READUINT8((*pp));
	for (i = 0; i < totalfiles; ++i)
	{
		if (toomany)
			SKIPSTRING((*pp));
		else
		{
			strlcpy(filename, (char *)(*pp), sizeof filename);
			SKIPSTRING((*pp));
		}
		READMEM((*pp), md5sum, 16);

		if (!toomany)
		{
			alreadyloaded = false;

			for (j = 0; j < numwadfiles; ++j)
			{
				if (memcmp(md5sum, wadfiles[j]->md5sum, 16) == 0)
				{
					alreadyloaded = true;
					break;
				}
			}

			if (alreadyloaded)
				continue;

			if (numwadfiles >= MAX_WADFILES)
				toomany = true;
			else
				ncs = findfile(filename, md5sum, false);

			if (toomany)
			{
				CONS_Alert(CONS_WARNING, M_GetText("Too many files loaded to add anymore for demo playback\n"));
				if (!CON_Ready())
					M_StartMessage(M_GetText("There are too many files loaded to add this demo's addons.\n\nDemo playback may desync.\n\nPress ESC\n"), NULL, MM_NOTHING);
			}
			else if (ncs != FS_FOUND)
			{
				if (ncs == FS_NOTFOUND)
					CONS_Alert(CONS_NOTICE, M_GetText("You do not have a copy of %s\n"), filename);
				else if (ncs == FS_MD5SUMBAD)
					CONS_Alert(CONS_NOTICE, M_GetText("Checksum mismatch on %s\n"), filename);
				else
					CONS_Alert(CONS_NOTICE, M_GetText("Unknown error finding file %s\n"), filename);

				if (!CON_Ready())
					M_StartMessage(M_GetText("There were errors trying to add this demo's addons. Check the console for more information.\n\nDemo playback may desync.\n\nPress ESC\n"), NULL, MM_NOTHING);
			}
			else
			{
				P_AddWadFile(filename);
			}
		}
	}
}

static void G_SkipDemoExtraFiles(UINT8 **pp)
{
	UINT8 totalfiles;
	UINT8 i;

	totalfiles = READUINT8((*pp));
	for (i = 0; i < totalfiles; ++i)
	{
		SKIPSTRING((*pp));// file name
		(*pp) += 16;// md5
	}
}

// G_CheckDemoExtraFiles: checks if our loaded WAD list matches the demo's.
// Enabling quick prevents filesystem checks to see if needed files are available to load.
static UINT8 G_CheckDemoExtraFiles(UINT8 **pp, boolean quick)
{
	UINT8 totalfiles, filesloaded, nmusfilecount;
	char filename[MAX_WADPATH];
	UINT8 md5sum[16];
	boolean toomany = false;
	boolean alreadyloaded;
	UINT8 i, j;
	UINT8 error = 0;

	totalfiles = READUINT8((*pp));
	filesloaded = 0;
	for (i = 0; i < totalfiles; ++i)
	{
		if (toomany)
			SKIPSTRING((*pp));
		else
		{
			strlcpy(filename, (char *)(*pp), sizeof filename);
			SKIPSTRING((*pp));
		}
		READMEM((*pp), md5sum, 16);

		if (!toomany)
		{
			alreadyloaded = false;
			nmusfilecount = 0;

			for (j = 0; j < numwadfiles; ++j)
			{
				if (wadfiles[j]->important && j > mainwads)
					nmusfilecount++;
				else
					continue;

				if (memcmp(md5sum, wadfiles[j]->md5sum, 16) == 0)
				{
					alreadyloaded = true;

					if (i != nmusfilecount-1 && error < DFILE_ERROR_OUTOFORDER)
						error |= DFILE_ERROR_OUTOFORDER;

					break;
				}
			}

			if (alreadyloaded)
			{
				filesloaded++;
				continue;
			}

			if (numwadfiles >= MAX_WADFILES)
				error = DFILE_ERROR_CANNOTLOAD;
			else if (!quick && findfile(filename, md5sum, false) != FS_FOUND)
				error = DFILE_ERROR_CANNOTLOAD;
			else if (error < DFILE_ERROR_INCOMPLETEOUTOFORDER)
				error |= DFILE_ERROR_NOTLOADED;
		} else
			error = DFILE_ERROR_CANNOTLOAD;
	}

	// Get final file count
	nmusfilecount = 0;

	for (j = 0; j < numwadfiles; ++j)
		if (wadfiles[j]->important && j > mainwads)
			nmusfilecount++;

	if (!error && filesloaded < nmusfilecount)
		error = DFILE_ERROR_EXTRAFILES;

	return error;
}

// Returns bitfield:
// 1 == new demo has lower time
// 2 == new demo has higher score
// 4 == new demo has higher rings
UINT8 G_CmpDemoTime(char *oldname, char *newname)
{
	UINT8 *buffer,*p;
	UINT8 flags;
	UINT32 oldtime, newtime, oldlap, newlap;
	UINT16 oldversion;
	size_t bufsize ATTRUNUSED;
	UINT8 c;
	UINT16 s ATTRUNUSED;
	UINT8 aflags = 0;
	boolean uselaps = false;

	// load the new file
	FIL_DefaultExtension(newname, ".lmp");
	bufsize = FIL_ReadFile(newname, &buffer);
	I_Assert(bufsize != 0);
	p = buffer;

	// read demo header
	I_Assert(!memcmp(p, DEMOHEADER, 12));
	p += 12; // DEMOHEADER
	c = READUINT8(p); // VERSION
	I_Assert(c == VERSION);
	c = READUINT8(p); // SUBVERSION
	I_Assert(c == SUBVERSION);
	s = READUINT16(p);
	I_Assert(s == DEMOVERSION);
	p += 64; // full demo title
	p += 16; // demo checksum
	I_Assert(!memcmp(p, "PLAY", 4));
	p += 4; // PLAY
	SKIPSTRING(p); // gamemap
	p += 16; // map md5
	flags = READUINT8(p); // demoflags
	p++; // gametype
	p++; // numlaps
	G_SkipDemoExtraFiles(&p);

	aflags = flags & (DF_TIMEATTACK|DF_BREAKTHECAPSULES);
	I_Assert(aflags);

	if (flags & DF_TIMEATTACK)
		uselaps = true; // get around uninitalized error

	newtime = READUINT32(p);
	if (uselaps)
		newlap = READUINT32(p);
	else
		newlap = UINT32_MAX;

	Z_Free(buffer);

	// load old file
	FIL_DefaultExtension(oldname, ".lmp");
	if (!FIL_ReadFile(oldname, &buffer))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), oldname);
		return UINT8_MAX;
	}
	p = buffer;

	// read demo header
	if (memcmp(p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	} p += 12; // DEMOHEADER
	p++; // VERSION
	p++; // SUBVERSION
	oldversion = READUINT16(p);
	switch(oldversion) // demoversion
	{
	case DEMOVERSION: // latest always supported
		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	}
	p += 64; // full demo title
	p += 16; // demo checksum
	if (memcmp(p, "PLAY", 4))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	} p += 4; // "PLAY"
	SKIPSTRING(p); // gamemap
	p += 16; // mapmd5
	flags = READUINT8(p);
	p++; // gametype
	p++; // numlaps
	G_SkipDemoExtraFiles(&p);
	if (!(flags & aflags))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' not from same game mode. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	}

	oldtime = READUINT32(p);
	if (uselaps)
		oldlap = READUINT32(p);
	else
		oldlap = 0;

	Z_Free(buffer);

	c = 0;

	if (uselaps)
	{
		if (newtime < oldtime
		|| (newtime == oldtime && (newlap < oldlap)))
			c |= 1; // Better time
		if (newlap < oldlap
		|| (newlap == oldlap && newtime < oldtime))
			c |= 1<<1; // Better lap time
	}
	else
	{
		if (newtime < oldtime)
			c |= 1; // Better time
	}

	return c;
}

void G_LoadDemoInfo(menudemo_t *pdemo)
{
	UINT8 *infobuffer, *info_p, *extrainfo_p;
	UINT8 version, subversion, pdemoflags;
	UINT16 pdemoversion, count;
	char mapname[MAXMAPLUMPNAME];
	INT32 i;

	if (!FIL_ReadFile(pdemo->filepath, &infobuffer))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), pdemo->filepath);
		infobuffer = NULL;
		goto badreplay;
	}

	info_p = infobuffer;

	if (memcmp(info_p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_ERROR, M_GetText("%s is not a Ring Racers replay file.\n"), pdemo->filepath);
		goto badreplay;
	}

	pdemo->type = MD_LOADED;

	info_p += 12; // DEMOHEADER

	version = READUINT8(info_p);
	subversion = READUINT8(info_p);
	pdemoversion = READUINT16(info_p);

	switch(pdemoversion)
	{
	case DEMOVERSION: // latest always supported
		// demo title
		M_Memcpy(pdemo->title, info_p, 64);
		info_p += 64;

		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_ERROR, M_GetText("%s is an incompatible replay format and cannot be played.\n"), pdemo->filepath);
		goto badreplay;
	}

	if (version != VERSION || subversion != SUBVERSION)
		pdemo->type = MD_OUTDATED;

	info_p += 16; // demo checksum
	if (memcmp(info_p, "PLAY", 4))
	{
		CONS_Alert(CONS_ERROR, M_GetText("%s is the wrong type of recording and cannot be played.\n"), pdemo->filepath);
		goto badreplay;
	}
	info_p += 4; // "PLAY"
	READSTRINGN(info_p, mapname, sizeof(mapname));
	pdemo->map = G_MapNumber(mapname);
	info_p += 16; // mapmd5

	pdemoflags = READUINT8(info_p);

	// temp?
	if (!(pdemoflags & DF_MULTIPLAYER))
	{
		CONS_Alert(CONS_ERROR, M_GetText("%s is not a multiplayer replay and can't be listed on this menu fully yet.\n"), pdemo->filepath);
		Z_Free(infobuffer);
		return;
	}

	pdemo->gametype = READUINT8(info_p);
	pdemo->numlaps = READUINT8(info_p);

	pdemo->addonstatus = G_CheckDemoExtraFiles(&info_p, true);

	for (i = 0; i < PRNUMCLASS; i++)
	{
		info_p += 4; // RNG seed
	}

	extrainfo_p = infobuffer + READUINT32(info_p); // The extra UINT32 read is for a blank 4 bytes?

	// Pared down version of CV_LoadNetVars to find the kart speed
	pdemo->kartspeed = KARTSPEED_NORMAL; // Default to normal speed
	count = READUINT16(info_p);
	while (count--)
	{
		UINT16 netid;
		char *svalue;

		netid = READUINT16(info_p);
		svalue = (char *)info_p;
		SKIPSTRING(info_p);
		info_p++; // stealth

		if (netid == cv_kartspeed.netid)
		{
			UINT8 j;
			for (j = 0; kartspeed_cons_t[j].strvalue; j++)
				if (!stricmp(kartspeed_cons_t[j].strvalue, svalue))
					pdemo->kartspeed = kartspeed_cons_t[j].value;
		}
	}

	if (pdemoflags & DF_ENCORE)
		pdemo->kartspeed |= DF_ENCORE;

	// Read standings!
	count = 0;

	while (READUINT8(extrainfo_p) == DW_STANDING) // Assume standings are always first in the extrainfo
	{
		char temp[16];

		pdemo->standings[count].ranking = READUINT8(extrainfo_p);

		// Name
		M_Memcpy(pdemo->standings[count].name, extrainfo_p, 16);
		extrainfo_p += 16;

		// Skin
		M_Memcpy(temp,extrainfo_p,16);
		extrainfo_p += 16;
		pdemo->standings[count].skin = UINT8_MAX;
		for (i = 0; i < numskins; i++)
			if (stricmp(skins[i].name, temp) == 0)
			{
				pdemo->standings[count].skin = i;
				break;
			}

		// Color
		M_Memcpy(temp,extrainfo_p,16);
		extrainfo_p += 16;
		for (i = 0; i < numskincolors; i++)
			if (!stricmp(skincolors[i].name,temp))				// SRB2kart
			{
				pdemo->standings[count].color = i;
				break;
			}

		// Score/time/whatever
		pdemo->standings[count].timeorscore = READUINT32(extrainfo_p);

		count++;

		if (count >= MAXPLAYERS)
			break; //@TODO still cycle through the rest of these if extra demo data is ever used
	}

	// I think that's everything we need?
	Z_Free(infobuffer);
	return;

badreplay:
	pdemo->type = MD_INVALID;
	sprintf(pdemo->title, "INVALID REPLAY");
	Z_Free(infobuffer);
}

//
// G_PlayDemo
//
void G_DeferedPlayDemo(const char *name)
{
	COM_BufAddText("playdemo \"");
	COM_BufAddText(name);
	COM_BufAddText("\" -addfiles\n");
}

//
// Start a demo from a .LMP file or from a wad resource
//

#define SKIPERRORS

void G_DoPlayDemo(char *defdemoname)
{
	UINT8 i, p;
	lumpnum_t l;
	char skin[17],color[MAXCOLORNAME+1],follower[17],mapname[MAXMAPLUMPNAME],*n,*pdemoname;
	UINT8 version,subversion;
	UINT32 randseed[PRNUMCLASS];
	char msg[1024];

	boolean spectator;
	UINT8 slots[MAXPLAYERS], kartspeed[MAXPLAYERS], kartweight[MAXPLAYERS], numslots = 0;

#if defined(SKIPERRORS) && !defined(DEVELOP)
	boolean skiperrors = false;
#endif

	G_InitDemoRewind();

	skin[16] = '\0';
	follower[16] = '\0';
	color[MAXCOLORNAME] = '\0';

	// No demo name means we're restarting the current demo
	if (defdemoname == NULL)
	{
		demo_p = demobuffer;
		pdemoname = ZZ_Alloc(1); // Easier than adding checks for this everywhere it's freed
	}
	else
	{
		n = defdemoname+strlen(defdemoname);
		while (*n != '/' && *n != '\\' && n != defdemoname)
			n--;
		if (n != defdemoname)
			n++;
		pdemoname = ZZ_Alloc(strlen(n)+1);
		strcpy(pdemoname,n);

		M_SetPlaybackMenuPointer();

		// Internal if no extension, external if one exists
		if (FIL_CheckExtension(defdemoname))
		{
			//FIL_DefaultExtension(defdemoname, ".lmp");
			if (!FIL_ReadFile(defdemoname, &demobuffer))
			{
				snprintf(msg, 1024, M_GetText("Failed to read file '%s'.\n"), defdemoname);
				CONS_Alert(CONS_ERROR, "%s", msg);
				gameaction = ga_nothing;
				M_StartMessage(msg, NULL, MM_NOTHING);
				return;
			}
			demo_p = demobuffer;
		}
		// load demo resource from WAD
		else if ((l = W_CheckNumForName(defdemoname)) == LUMPERROR)
		{
			snprintf(msg, 1024, M_GetText("Failed to read lump '%s'.\n"), defdemoname);
			CONS_Alert(CONS_ERROR, "%s", msg);
			gameaction = ga_nothing;
			M_StartMessage(msg, NULL, MM_NOTHING);
			return;
		}
		else // it's an internal demo
		{
			demobuffer = demo_p = W_CacheLumpNum(l, PU_STATIC);
#if defined(SKIPERRORS) && !defined(DEVELOP)
			skiperrors = true; // SRB2Kart: Don't print warnings for staff ghosts, since they'll inevitably happen when we make bugfixes/changes...
#endif
		}
	}

	// read demo header
	gameaction = ga_nothing;
	demo.playback = true;
	if (memcmp(demo_p, DEMOHEADER, 12))
	{
		snprintf(msg, 1024, M_GetText("%s is not a Ring Racers replay file.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demo.playback = false;
		demo.title = false;
		return;
	}
	demo_p += 12; // DEMOHEADER

	version = READUINT8(demo_p);
	subversion = READUINT8(demo_p);
	demo.version = READUINT16(demo_p);
	switch(demo.version)
	{
	case DEMOVERSION: // latest always supported
		break;
	// too old, cannot support.
	default:
		snprintf(msg, 1024, M_GetText("%s is an incompatible replay format and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demo.playback = false;
		demo.title = false;
		return;
	}

	// demo title
	M_Memcpy(demo.titlename, demo_p, 64);
	demo_p += 64;

	demo_p += 16; // demo checksum

	if (memcmp(demo_p, "PLAY", 4))
	{
		snprintf(msg, 1024, M_GetText("%s is the wrong type of recording and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demo.playback = false;
		demo.title = false;
		return;
	}
	demo_p += 4; // "PLAY"
	READSTRINGN(demo_p, mapname, sizeof(mapname)); // gamemap
	gamemap = G_MapNumber(mapname)+1;
	demo_p += 16; // mapmd5

	demoflags = READUINT8(demo_p);
	gametype = READUINT8(demo_p);
	G_SetGametype(gametype);
	numlaps = READUINT8(demo_p);

	if (demo.title) // Titledemos should always play and ought to always be compatible with whatever wadlist is running.
		G_SkipDemoExtraFiles(&demo_p);
	else if (demo.loadfiles)
		G_LoadDemoExtraFiles(&demo_p);
	else if (demo.ignorefiles)
		G_SkipDemoExtraFiles(&demo_p);
	else
	{
		UINT8 error = G_CheckDemoExtraFiles(&demo_p, false);

		if (error)
		{
			switch (error)
			{
			case DFILE_ERROR_NOTLOADED:
				snprintf(msg, 1024,
					M_GetText("Required files for this demo are not loaded.\n\nUse\n\"playdemo %s -addfiles\"\nto load them and play the demo.\n"),
				pdemoname);
				break;

			case DFILE_ERROR_OUTOFORDER:
				snprintf(msg, 1024,
					M_GetText("Required files for this demo are loaded out of order.\n\nUse\n\"playdemo %s -force\"\nto play the demo anyway.\n"),
				pdemoname);
				break;

			case DFILE_ERROR_INCOMPLETEOUTOFORDER:
				snprintf(msg, 1024,
					M_GetText("Required files for this demo are not loaded, and some are out of order.\n\nUse\n\"playdemo %s -addfiles\"\nto load needed files and play the demo.\n"),
				pdemoname);
				break;

			case DFILE_ERROR_CANNOTLOAD:
				snprintf(msg, 1024,
					M_GetText("Required files for this demo cannot be loaded.\n\nUse\n\"playdemo %s -force\"\nto play the demo anyway.\n"),
				pdemoname);
				break;

			case DFILE_ERROR_EXTRAFILES:
				snprintf(msg, 1024,
					M_GetText("You have additional files loaded beyond the demo's file list.\n\nUse\n\"playdemo %s -force\"\nto play the demo anyway.\n"),
				pdemoname);
				break;
			}

			CONS_Alert(CONS_ERROR, "%s", msg);
			if (!CON_Ready()) // In the console they'll just see the notice there! No point pulling them out.
				M_StartMessage(msg, NULL, MM_NOTHING);
			Z_Free(pdemoname);
			Z_Free(demobuffer);
			demo.playback = false;
			demo.title = false;
			return;
		}
	}

	modeattacking = (demoflags & DF_ATTACKMASK)>>DF_ATTACKSHIFT;
	multiplayer = !!(demoflags & DF_MULTIPLAYER);
	demo.netgame = (multiplayer && !(demoflags & DF_NONETMP));
	CON_ToggleOff();

	hu_demotime = UINT32_MAX;
	hu_demolap = UINT32_MAX;

	switch (modeattacking)
	{
		case ATTACKING_NONE: // 0
			break;
		case ATTACKING_TIME: // 1
			hu_demotime = READUINT32(demo_p);
			hu_demolap = READUINT32(demo_p);
			break;
		case ATTACKING_CAPSULES: // 2
			hu_demotime = READUINT32(demo_p);
			break;
		default: // 3
			modeattacking = ATTACKING_NONE;
			break;
	}

	// Random seed
	for (i = 0; i < PRNUMCLASS; i++)
	{
		randseed[i] = READUINT32(demo_p);
	}

	demo_p += 4; // Extrainfo location

	// ...*map* not loaded?
	if (!gamemap || (gamemap > nummapheaders) || !mapheaderinfo[gamemap-1] || mapheaderinfo[gamemap-1]->lumpnum == LUMPERROR)
	{
		snprintf(msg, 1024, M_GetText("%s features a course that is not currently loaded.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demo.playback = false;
		demo.title = false;
		return;
	}

	// net var data
	CV_LoadDemoVars(&demo_p);

	// Sigh ... it's an empty demo.
	if (*demo_p == DEMOMARKER)
	{
		snprintf(msg, 1024, M_GetText("%s contains no data to be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demo.playback = false;
		demo.title = false;
		return;
	}

	Z_Free(pdemoname);

	memset(&oldcmd,0,sizeof(oldcmd));
	memset(&oldghost,0,sizeof(oldghost));
	memset(&ghostext,0,sizeof(ghostext));

#if defined(SKIPERRORS) && !defined(DEVELOP)
	if ((VERSION != version || SUBVERSION != subversion) && !skiperrors)
#else
	if (VERSION != version || SUBVERSION != subversion)
#endif
		CONS_Alert(CONS_WARNING, M_GetText("Demo version does not match game version. Desyncs may occur.\n"));

	// console warning messages
#if defined(SKIPERRORS) && !defined(DEVELOP)
	demosynced = (!skiperrors);
#else
	demosynced = true;
#endif

	// didn't start recording right away.
	demo.deferstart = false;

	displayplayers[0] = consoleplayer = 0;
	memset(playeringame,0,sizeof(playeringame));

	// Load players that were in-game when the map started
	p = READUINT8(demo_p);

	for (i = 1; i < MAXSPLITSCREENPLAYERS; i++)
		displayplayers[i] = INT32_MAX;

	while (p != 0xFF)
	{
		UINT8 flags = READUINT8(demo_p);

		spectator = !!(flags & DEMO_SPECTATOR);

		if (spectator == true)
		{
			if (modeattacking)
			{
				snprintf(msg, 1024, M_GetText("%s is a Record Attack replay with spectators, and is thus invalid.\n"), pdemoname);
				CONS_Alert(CONS_ERROR, "%s", msg);
				M_StartMessage(msg, NULL, MM_NOTHING);
				Z_Free(pdemoname);
				Z_Free(demobuffer);
				demo.playback = false;
				demo.title = false;
				return;
			}
		}

		slots[numslots] = p;
		numslots++;

		if (modeattacking && numslots > 1)
		{
			snprintf(msg, 1024, M_GetText("%s is a Record Attack replay with multiple players, and is thus invalid.\n"), pdemoname);
			CONS_Alert(CONS_ERROR, "%s", msg);
			M_StartMessage(msg, NULL, MM_NOTHING);
			Z_Free(pdemoname);
			Z_Free(demobuffer);
			demo.playback = false;
			demo.title = false;
			return;
		}

		if (!playeringame[displayplayers[0]] || players[displayplayers[0]].spectator)
			displayplayers[0] = consoleplayer = serverplayer = p;

		playeringame[p] = true;
		players[p].spectator = spectator;

		if (flags & DEMO_KICKSTART)
			players[p].pflags |= PF_KICKSTARTACCEL;
		else
			players[p].pflags &= ~PF_KICKSTARTACCEL;

		if (flags & DEMO_SHRINKME)
			players[p].pflags |= PF_SHRINKME;
		else
			players[p].pflags &= ~PF_SHRINKME;

		K_UpdateShrinkCheat(&players[p]);

		// Name
		M_Memcpy(player_names[p],demo_p,16);
		demo_p += 16;

		/*if (players[p].spectator)
		{
			CONS_Printf("player %s is spectator at start\n", player_names[p]);
		}*/

		// Skin
		M_Memcpy(skin,demo_p,16);
		demo_p += 16;
		SetPlayerSkin(p, skin);

		// Color
		M_Memcpy(color,demo_p,16);
		demo_p += 16;
		for (i = 0; i < numskincolors; i++)
			if (!stricmp(skincolors[i].name,color))				// SRB2kart
			{
				players[p].skincolor = i;
				break;
			}

		// Follower
		M_Memcpy(follower, demo_p, 16);
		demo_p += 16;
		K_SetFollowerByName(p, follower);

		// Follower colour
		M_Memcpy(color, demo_p, 16);
		demo_p += 16;
		for (i = 0; i < numskincolors +2; i++)	// +2 because of Match and Opposite
		{
				if (!stricmp(Followercolor_cons_t[i].strvalue, color))
				{
						players[p].followercolor = i;
						break;
				}
		}

		// Score, since Kart uses this to determine where you start on the map
		players[p].score = READUINT32(demo_p);

		// Power Levels
		clientpowerlevels[p][gametype == GT_BATTLE ? PWRLV_BATTLE : PWRLV_RACE] = READUINT16(demo_p);

		// Kart stats, temporarily
		kartspeed[p] = READUINT8(demo_p);
		kartweight[p] = READUINT8(demo_p);

		if (stricmp(skins[players[p].skin].name, skin) != 0)
			FindClosestSkinForStats(p, kartspeed[p], kartweight[p]);

		// Followitem
		players[p].followitem = READUINT32(demo_p);

		// Look for the next player
		p = READUINT8(demo_p);
	}

	// end of player read (the 0xFF marker)
	// so this is where we are to read our lua variables (if possible!)
	if (demoflags & DF_LUAVARS)	// again, used for compability, lua shit will be saved to replays regardless of if it's even been loaded
	{
		if (!gL) // No Lua state! ...I guess we'll just start one...
			LUA_ClearState();

		// No modeattacking check, DF_LUAVARS won't be present here.
		LUA_UnArchive(&demo_p);
	}

	splitscreen = 0;

	if (demo.title)
	{
		splitscreen = M_RandomKey(6)-1;
		splitscreen = min(min(3, numslots-1), splitscreen); // Bias toward 1p and 4p views

		for (p = 0; p <= splitscreen; p++)
			G_ResetView(p+1, slots[M_RandomKey(numslots)], false);
	}

	R_ExecuteSetViewSize();

	for (i = 0; i < PRNUMCLASS; i++)
	{
		P_SetRandSeed(i, randseed[i]);
	}

	G_InitNew(demoflags & DF_ENCORE, gamemap, true, true, false); // Doesn't matter whether you reset or not here, given changes to resetplayer.

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (players[i].mo)
		{
			players[i].mo->color = players[i].skincolor;
			oldghost[i].x = players[i].mo->x;
			oldghost[i].y = players[i].mo->y;
			oldghost[i].z = players[i].mo->z;
		}

		// Set saved attribute values
		// No cheat checking here, because even if they ARE wrong...
		// it would only break the replay if we clipped them.
		players[i].kartspeed = kartspeed[i];
		players[i].kartweight = kartweight[i];
	}

	demo.deferstart = true;
}

void G_AddGhost(char *defdemoname)
{
	INT32 i;
	lumpnum_t l;
	char name[17],skin[17],color[MAXCOLORNAME+1],*n,*pdemoname,md5[16];
	demoghost *gh;
	UINT8 flags;
	UINT8 *buffer,*p;
	mapthing_t *mthing;
	UINT16 count, ghostversion;
	skin_t *ghskin = &skins[0];
	UINT8 kartspeed = UINT8_MAX, kartweight = UINT8_MAX;

	name[16] = '\0';
	skin[16] = '\0';
	color[16] = '\0';

	n = defdemoname+strlen(defdemoname);
	while (*n != '/' && *n != '\\' && n != defdemoname)
		n--;
	if (n != defdemoname)
		n++;
	pdemoname = ZZ_Alloc(strlen(n)+1);
	strcpy(pdemoname,n);

	// Internal if no extension, external if one exists
	if (FIL_CheckExtension(defdemoname))
	{
		//FIL_DefaultExtension(defdemoname, ".lmp");
		if (!FIL_ReadFileTag(defdemoname, &buffer, PU_LEVEL))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), defdemoname);
			Z_Free(pdemoname);
			return;
		}
		p = buffer;
	}
	// load demo resource from WAD
	else if ((l = W_CheckNumForName(defdemoname)) == LUMPERROR)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to read lump '%s'.\n"), defdemoname);
		Z_Free(pdemoname);
		return;
	}
	else // it's an internal demo
		buffer = p = W_CacheLumpNum(l, PU_LEVEL);

	// read demo header
	if (memcmp(p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Not a SRB2 replay.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	} p += 12; // DEMOHEADER

	p++; // VERSION
	p++; // SUBVERSION

	ghostversion = READUINT16(p);
	switch(ghostversion)
	{
	case DEMOVERSION: // latest always supported
		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Demo version incompatible.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}

	p += 64; // title
	M_Memcpy(md5, p, 16); p += 16; // demo checksum

	for (gh = ghosts; gh; gh = gh->next)
		if (!memcmp(md5, gh->checksum, 16)) // another ghost in the game already has this checksum?
		{ // Don't add another one, then!
			CONS_Debug(DBG_SETUP, "Rejecting duplicate ghost %s (MD5 was matched)\n", pdemoname);
			Z_Free(pdemoname);
			Z_Free(buffer);
			return;
		}

	if (memcmp(p, "PLAY", 4))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Demo format unacceptable.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	} p += 4; // "PLAY"


	SKIPSTRING(p); // gamemap
	p += 16; // mapmd5 (possibly check for consistency?)

	flags = READUINT8(p);
	if (!(flags & DF_GHOST))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: No ghost data in this demo.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}

	if (flags & DF_LUAVARS) // can't be arsed to add support for grinding away ported lua material
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Replay data contains luavars, cannot continue.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}

	p++; // gametype
	p++; // numlaps
	G_SkipDemoExtraFiles(&p); // Don't wanna modify the file list for ghosts.

	switch ((flags & DF_ATTACKMASK)>>DF_ATTACKSHIFT)
	{
		case ATTACKING_NONE: // 0
			break;
		case ATTACKING_TIME: // 1
			p += 8; // demo time, lap
			break;
		case ATTACKING_CAPSULES: // 2
			p += 4; // demo time
			break;
		default: // 3
			break;
	}

	for (i = 0; i < PRNUMCLASS; i++)
	{
		p += 4; // random seed
	}

	p += 4; // Extra data location reference

	// net var data
	count = READUINT16(p);
	while (count--)
	{
		SKIPSTRING(p);
		SKIPSTRING(p);
		p++;
	}

	if (*p == DEMOMARKER)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Failed to add ghost %s: Replay is empty.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}

	p++; // player number - doesn't really need to be checked, TODO maybe support adding multiple players' ghosts at once

	// any invalidating flags?
	if ((READUINT8(p) & (DEMO_SPECTATOR)) != 0)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Failed to add ghost %s: Invalid player slot.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}

	// Player name (TODO: Display this somehow if it doesn't match cv_playername!)
	M_Memcpy(name, p, 16);
	p += 16;

	// Skin
	M_Memcpy(skin, p, 16);
	p += 16;

	// Color
	M_Memcpy(color, p, 16);
	p += 16;

	// Follower data was here, skip it, we don't care about it for ghosts.
	p += 32;	// followerskin (16) + followercolor (16)

	p += 4; // score
	p += 2; // powerlevel

	kartspeed = READUINT8(p);
	kartweight = READUINT8(p);

	p += 4; // followitem (maybe change later)

	if (READUINT8(p) != 0xFF)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Failed to add ghost %s: Invalid player slot.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}

	for (i = 0; i < numskins; i++)
		if (!stricmp(skins[i].name,skin))
		{
			ghskin = &skins[i];
			break;
		}

	if (i == numskins)
	{
		if (kartspeed != UINT8_MAX && kartweight != UINT8_MAX)
			ghskin = &skins[GetSkinNumClosestToStats(kartspeed, kartweight)];

		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Invalid character. Falling back to %s.\n"), pdemoname, ghskin->name);
	}


	gh = Z_Calloc(sizeof(demoghost), PU_LEVEL, NULL);
	gh->next = ghosts;
	gh->buffer = buffer;
	M_Memcpy(gh->checksum, md5, 16);
	gh->p = p;

	ghosts = gh;

	gh->version = ghostversion;
	mthing = playerstarts[0] ? playerstarts[0] : deathmatchstarts[0]; // todo not correct but out of scope
	I_Assert(mthing);
	{ // A bit more complex than P_SpawnPlayer because ghosts aren't solid and won't just push themselves out of the ceiling.
		fixed_t z,f,c;
		fixed_t offset = mthing->z << FRACBITS;
		gh->mo = P_SpawnMobj(mthing->x << FRACBITS, mthing->y << FRACBITS, 0, MT_GHOST);
		gh->mo->angle = FixedAngle(mthing->angle << FRACBITS);
		f = gh->mo->floorz;
		c = gh->mo->ceilingz - mobjinfo[MT_PLAYER].height;
		if (!!(mthing->options & MTF_AMBUSH) ^ !!(mthing->options & MTF_OBJECTFLIP))
		{
			z = c - offset;
			if (z < f)
				z = f;
		}
		else
		{
			z = f + offset;
			if (z > c)
				z = c;
		}
		gh->mo->z = z;
	}

	gh->oldmo.x = gh->mo->x;
	gh->oldmo.y = gh->mo->y;
	gh->oldmo.z = gh->mo->z;

	gh->mo->state = states + S_KART_STILL;
	gh->mo->sprite = gh->mo->state->sprite;
	gh->mo->sprite2 = (gh->mo->state->frame & FF_FRAMEMASK);
	//gh->mo->frame = tr_trans30<<FF_TRANSSHIFT;
	gh->mo->renderflags |= RF_DONTDRAW;
	gh->fadein = (9-3)*6; // fade from invisible to trans30 over as close to 35 tics as possible
	gh->mo->tics = -1;

	// Set skin
	gh->mo->skin = gh->oldmo.skin = ghskin;

	// Set color
	gh->mo->color = ((skin_t*)gh->mo->skin)->prefcolor;
	for (i = 0; i < numskincolors; i++)
		if (!stricmp(skincolors[i].name,color))
		{
			gh->mo->color = (UINT16)i;
			break;
		}
	gh->oldmo.color = gh->mo->color;

	CONS_Printf(M_GetText("Added ghost %s from %s\n"), name, pdemoname);
	Z_Free(pdemoname);
}

// Clean up all ghosts
void G_FreeGhosts(void)
{
	while (ghosts)
	{
		demoghost *next = ghosts->next;
		Z_Free(ghosts);
		ghosts = next;
	}
	ghosts = NULL;
}

// A simplified version of G_AddGhost...
void G_UpdateStaffGhostName(lumpnum_t l)
{
	UINT8 *buffer,*p;
	UINT16 ghostversion;
	UINT8 flags;
	INT32 i;

	buffer = p = W_CacheLumpNum(l, PU_CACHE);

	// read demo header
	if (memcmp(p, DEMOHEADER, 12))
	{
		goto fail;
	}

	p += 12; // DEMOHEADER
	p++; // VERSION
	p++; // SUBVERSION

	ghostversion = READUINT16(p);
	switch(ghostversion)
	{
		case DEMOVERSION: // latest always supported
			break;

		// too old, cannot support.
		default:
			goto fail;
	}

	p += 64; // full demo title
	p += 16; // demo checksum

	if (memcmp(p, "PLAY", 4))
	{
		goto fail;
	}

	p += 4; // "PLAY"
	SKIPSTRING(p); // gamemap
	p += 16; // mapmd5 (possibly check for consistency?)

	flags = READUINT8(p);
	if (!(flags & DF_GHOST))
	{
		goto fail; // we don't NEED to do it here, but whatever
	}

	p++; // Gametype
	p++; // numlaps
	G_SkipDemoExtraFiles(&p);

	switch ((flags & DF_ATTACKMASK)>>DF_ATTACKSHIFT)
	{
		case ATTACKING_NONE: // 0
			break;
		case ATTACKING_TIME: // 1
			p += 8; // demo time, lap
			break;
		case ATTACKING_CAPSULES: // 2
			p += 4; // demo time
			break;
		default: // 3
			break;
	}

	for (i = 0; i < PRNUMCLASS; i++)
	{
		p += 4; // random seed
	}

	p += 4; // Extrainfo location marker

	// Ehhhh don't need ghostversion here (?) so I'll reuse the var here
	ghostversion = READUINT16(p);
	while (ghostversion--)
	{
		p += 2;
		SKIPSTRING(p);
		p++; // stealth
	}

	// Assert first player is in and then read name
	if (READUINT8(p) != 0)
		goto fail;
	M_Memcpy(dummystaffname, p,16);
	dummystaffname[16] = '\0';

	// Ok, no longer any reason to care, bye
fail:
	Z_Free(buffer);
	return;
}

//
// G_TimeDemo
// NOTE: name is a full filename for external demos
//
static INT32 restorecv_vidwait;

void G_TimeDemo(const char *name)
{
	nodrawers = M_CheckParm("-nodraw");
	noblit = M_CheckParm("-noblit");
	restorecv_vidwait = cv_vidwait.value;
	if (cv_vidwait.value)
		CV_Set(&cv_vidwait, "0");
	demo.timing = true;
	singletics = true;
	framecount = 0;
	demostarttime = I_GetTime();
	G_DeferedPlayDemo(name);
}

void G_DoPlayMetal(void)
{
	lumpnum_t l;
	mobj_t *mo = NULL;
	thinker_t *th;

	// it's an internal demo
	// TODO: Use map header to determine lump name
	if ((l = W_CheckNumForName(va("%sMS",G_BuildMapName(gamemap)))) == LUMPERROR)
	{
		CONS_Alert(CONS_WARNING, M_GetText("No bot recording for this map.\n"));
		return;
	}
	else
		metalbuffer = metal_p = W_CacheLumpNum(l, PU_STATIC);

	// find metal sonic
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)th;
		if (mo->type != MT_METALSONIC_RACE)
			continue;

		break;
	}
	if (th == &thlist[THINK_MOBJ])
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to find bot entity.\n"));
		Z_Free(metalbuffer);
		return;
	}

	// read demo header
	metal_p += 12; // DEMOHEADER
	metal_p++; // VERSION
	metal_p++; // SUBVERSION
	metalversion = READUINT16(metal_p);
	switch(metalversion)
	{
	case DEMOVERSION: // latest always supported
		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_WARNING, M_GetText("Failed to load bot recording for this map, format version incompatible.\n"));
		Z_Free(metalbuffer);
		return;
	}
	metal_p += 16; // demo checksum
	if (memcmp(metal_p, "METL", 4))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Failed to load bot recording for this map, wasn't recorded in Metal format.\n"));
		Z_Free(metalbuffer);
		return;
	} metal_p += 4; // "METL"

	// read initial tic
	memset(&oldmetal,0,sizeof(oldmetal));
	oldmetal.x = mo->x;
	oldmetal.y = mo->y;
	oldmetal.z = mo->z;
	metalplayback = mo;
}

void G_DoneLevelLoad(void)
{
	CONS_Printf(M_GetText("Loaded level in %f sec\n"), (double)(I_GetTime() - demostarttime) / TICRATE);
	framecount = 0;
	demostarttime = I_GetTime();
}

/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

// Writes the demo's checksum, or just random garbage if you can't do that for some reason.
static void WriteDemoChecksum(void)
{
	UINT8 *p = demobuffer+16; // checksum position
#ifdef NOMD5
	UINT8 i;
	for (i = 0; i < 16; i++, p++)
		*p = P_RandomByte(PR_UNDEFINED); // This MD5 was chosen by fair dice roll and most likely < 50% correct.
#else
	md5_buffer((char *)p+16, demo_p - (p+16), p); // make a checksum of everything after the checksum in the file.
#endif
}

// Stops metal sonic's demo. Separate from other functions because metal + replays can coexist
void G_StopMetalDemo(void)
{
	// Metal Sonic finishing doesn't end the game, dammit.
	Z_Free(metalbuffer);
	metalbuffer = NULL;
	metalplayback = NULL;
	metal_p = NULL;
}

// Stops metal sonic recording.
ATTRNORETURN void FUNCNORETURN G_StopMetalRecording(boolean kill)
{
	boolean saved = false;
	if (demo_p)
	{
		WRITEUINT8(demo_p, (kill) ? METALDEATH : DEMOMARKER); // add the demo end (or metal death) marker
		WriteDemoChecksum();
		saved = FIL_WriteFile(va("%sMS.LMP", G_BuildMapName(gamemap)), demobuffer, demo_p - demobuffer); // finally output the file.
	}
	free(demobuffer);
	metalrecording = false;
	if (saved)
		I_Error("Saved to %sMS.LMP", G_BuildMapName(gamemap));
	I_Error("Failed to save demo!");
}

// Stops timing a demo.
static void G_StopTimingDemo(void)
{
	INT32 demotime;
	double f1, f2;
	demotime = I_GetTime() - demostarttime;
	if (!demotime)
		return;
	G_StopDemo();
	demo.timing = false;
	f1 = (double)demotime;
	f2 = (double)framecount*TICRATE;

	CONS_Printf(M_GetText("timed %u gametics in %d realtics - %u frames\n%f seconds, %f avg fps\n"),
		leveltime,demotime,(UINT32)framecount,f1/TICRATE,f2/f1);

	// CSV-readable timedemo results, for external parsing
	if (timedemo_csv)
	{
		FILE *f;
		const char *csvpath = va("%s"PATHSEP"%s", srb2home, "timedemo.csv");
		const char *header = "id,demoname,seconds,avgfps,leveltime,demotime,framecount,ticrate,rendermode,vidmode,vidwidth,vidheight,procbits\n";
		const char *rowformat = "\"%s\",\"%s\",%f,%f,%u,%d,%u,%u,%u,%u,%u,%u,%u\n";
		boolean headerrow = !FIL_FileExists(csvpath);
		UINT8 procbits = 0;

		// Bitness
		if (sizeof(void*) == 4)
			procbits = 32;
		else if (sizeof(void*) == 8)
			procbits = 64;

		f = fopen(csvpath, "a+");

		if (f)
		{
			if (headerrow)
				fputs(header, f);
			fprintf(f, rowformat,
				timedemo_csv_id,timedemo_name,f1/TICRATE,f2/f1,leveltime,demotime,(UINT32)framecount,TICRATE,rendermode,vid.modenum,vid.width,vid.height,procbits);
			fclose(f);
			CONS_Printf("Timedemo results saved to '%s'\n", csvpath);
		}
		else
		{
			// Just print the CSV output to console
			CON_LogMessage(header);
			CONS_Printf(rowformat,
				timedemo_csv_id,timedemo_name,f1/TICRATE,f2/f1,leveltime,demotime,(UINT32)framecount,TICRATE,rendermode,vid.modenum,vid.width,vid.height,procbits);
		}
	}

	if (restorecv_vidwait != cv_vidwait.value)
		CV_SetValue(&cv_vidwait, restorecv_vidwait);
	D_AdvanceDemo();
}

// reset engine variable set for the demos
// called from stopdemo command, map command, and g_checkdemoStatus.
void G_StopDemo(void)
{
	Z_Free(demobuffer);
	demobuffer = NULL;
	demo.playback = false;
	if (demo.title)
		modeattacking = false;
	demo.title = false;
	demo.timing = false;
	singletics = false;

	demo.freecam = false;
	// reset democam shit too:
	democam.cam = NULL;
	democam.soundmobj = NULL;
	democam.localangle = 0;
	democam.localaiming = 0;
	democam.keyboardlook = false;

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission(); // cleanup

	if (gamestate == GS_VOTING)
		Y_EndVote();

	G_SetGamestate(GS_NULL);
	wipegamestate = GS_NULL;
	SV_StopServer();
	SV_ResetServer();
}

boolean G_CheckDemoStatus(void)
{
	G_FreeGhosts();

	// DO NOT end metal sonic demos here

	if (demo.timing)
	{
		G_StopTimingDemo();
		return true;
	}

	if (demo.playback)
	{
		if (demo.quitafterplaying)
			I_Quit();

		if (multiplayer && !demo.title)
			G_ExitLevel();
		else
		{
			G_StopDemo();

			if (modeattacking)
				M_EndModeAttackRun();
			else
				D_AdvanceDemo();
		}

		return true;
	}

	if (demo.recording && (modeattacking || demo.savemode != DSM_NOTSAVING))
	{
		G_SaveDemo();
		return true;
	}

	demo.recording = false;
	return false;
}

void G_SaveDemo(void)
{
	UINT8 *p = demobuffer+16; // after version
	UINT32 length;
#ifdef NOMD5
	UINT8 i;
#endif

	// Ensure extrainfo pointer is always available, even if no info is present.
	if (demoinfo_p && *(UINT32 *)demoinfo_p == 0)
	{
		WRITEUINT8(demo_p, DEMOMARKER); // add the demo end marker
		*(UINT32 *)demoinfo_p = demo_p - demobuffer;
	}
	WRITEUINT8(demo_p, DW_END); // Mark end of demo extra data.

	M_Memcpy(p, demo.titlename, 64); // Write demo title here
	p += 64;

	if (multiplayer)
	{
		// Change the demo's name to be a slug of the title
		char demo_slug[128];
		char *writepoint;
		size_t i, strindex = 0;
		boolean dash = true;

		for (i = 0; demo.titlename[i] && i < 127; i++)
		{
			if ((demo.titlename[i] >= 'a' && demo.titlename[i] <= 'z') ||
				(demo.titlename[i] >= '0' && demo.titlename[i] <= '9'))
			{
				demo_slug[strindex] = demo.titlename[i];
				strindex++;
				dash = false;
			}
			else if (demo.titlename[i] >= 'A' && demo.titlename[i] <= 'Z')
			{
				demo_slug[strindex] = demo.titlename[i] + 'a' - 'A';
				strindex++;
				dash = false;
			}
			else if (!dash)
			{
				demo_slug[strindex] = '-';
				strindex++;
				dash = true;
			}
		}

		demo_slug[strindex] = 0;
		if (dash) demo_slug[strindex-1] = 0;

		writepoint = strstr(strrchr(demoname, *PATHSEP), "-") + 1;
		demo_slug[128 - (writepoint - demoname) - 4] = 0;
		sprintf(writepoint, "%s.lmp", demo_slug);
	}

	length = *(UINT32 *)demoinfo_p;
	WRITEUINT32(demoinfo_p, length);

	// Doesn't seem like I can use WriteDemoChecksum here, correct me if I'm wrong -Sal
#ifdef NOMD5
	for (i = 0; i < 16; i++, p++)
		*p = M_RandomByte(); // This MD5 was chosen by fair dice roll and most likely < 50% correct.
#else
	// Make a checksum of everything after the checksum in the file up to the end of the standard data. Extrainfo is freely modifiable.
	md5_buffer((char *)p+16, (demobuffer + length) - (p+16), p);
#endif

	if (FIL_WriteFile(demoname, demobuffer, demo_p - demobuffer)) // finally output the file.
		demo.savemode = DSM_SAVED;
	free(demobuffer);
	demo.recording = false;

	if (!modeattacking)
	{
		if (demo.savemode == DSM_SAVED)
			CONS_Printf(M_GetText("Demo %s recorded\n"), demoname);
		else
			CONS_Alert(CONS_WARNING, M_GetText("Demo %s not saved\n"), demoname);
	}
}

boolean G_DemoTitleResponder(event_t *ev)
{
	size_t len;
	INT32 ch;

	if (ev->type != ev_keydown)
		return false;

	ch = (INT32)ev->data1;

	// Only ESC and non-keyboard keys abort connection
	if (ch == KEY_ESCAPE)
	{
		demo.savemode = (cv_recordmultiplayerdemos.value == 2) ? DSM_WILLAUTOSAVE : DSM_NOTSAVING;
		return true;
	}

	if (ch == KEY_ENTER || ch >= NUMKEYS)
	{
		demo.savemode = DSM_WILLSAVE;
		return true;
	}

	if ((ch >= HU_FONTSTART && ch <= HU_FONTEND && fontv[HU_FONT].font[ch-HU_FONTSTART])
	  || ch == ' ') // Allow spaces, of course
	{
		len = strlen(demo.titlename);
		if (len < 64)
		{
			demo.titlename[len+1] = 0;
			demo.titlename[len] = CON_ShiftChar(ch);
		}
	}
	else if (ch == KEY_BACKSPACE)
	{
		if (shiftdown)
			memset(demo.titlename, 0, sizeof(demo.titlename));
		else
		{
			len = strlen(demo.titlename);

			if (len > 0)
				demo.titlename[len-1] = 0;
		}
	}

	return true;
}
