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
/// \file  g_demo.c
/// \brief Demo recording and playback

#include <algorithm>
#include <cstddef>

#include <tcb/span.hpp>
#include <nlohmann/json.hpp>

#include "doomdef.h"
#include "doomtype.h"
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
#include "m_cond.h"
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
#include "p_saveg.h" // savebuffer_t
#include "g_party.h"

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
#include "k_vote.h"
#include "k_credits.h"
#include "k_grandprix.h"

static menuitem_t TitleEntry[] =
{
	{IT_NOTHING | IT_SPACE, "Save Replay", NULL,
		NULL, {NULL}, 0, 0},
};

static menu_t TitleEntryDef = {
	sizeof (TitleEntry) / sizeof (menuitem_t),
	NULL,
	0,
	TitleEntry,
	0, 0,
	0, 0,
	MBF_SOUNDLESS,
	NULL,
	0, 0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

boolean nodrawers; // for comparative timing purposes
boolean noblit; // for comparative timing purposes
tic_t demostarttime; // for comparative timing purposes

static constexpr DemoBufferSizes get_buffer_sizes(UINT16 version)
{
	if (version < 0x000A) // old staff ghost support
		return {16, 16, 16};

	// These sizes are compatible as of version 0x000A
	static_assert(MAXPLAYERNAME == 21);
	static_assert(SKINNAMESIZE == 16);
	static_assert(MAXCOLORNAME == 32);
	return {21, 16, 32};
}

static DemoBufferSizes g_buffer_sizes;

size_t copy_fixed_buf(void* p, const void* s, size_t n)
{
	strncpy((char*)p, (const char*)s, n);
	((char*)p)[n] = '\0';
	return n;
}

static char demoname[MAX_WADPATH];
static savebuffer_t demobuf = {0};
static UINT8 *demotime_p, *demoinfo_p;
static UINT16 demoflags;
boolean demosynced = true; // console warning message

struct demovars_s demo;

// extra data stuff (events registered this frame while recording)
static struct {
	UINT8 flags; // EZT flags

	// EZT_COLOR
	UINT8 color, lastcolor;

	// EZT_SCALE
	fixed_t scale, lastscale;

	// EZT_ITEMDATA
	SINT8 itemtype;
	UINT8 itemamount;
	INT32 health;

	// EZT_STATDATA
	UINT8 skinid, kartspeed, kartweight;
	UINT32 charflags;

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

// Also supported:
// - 0x0009 (older staff ghosts)
//   - Player names, skin names and color names were 16
//     bytes. See get_buffer_sizes().
// - 0x000A (Ring Racers v2.0)
//   - A bug was preventing control after ending a drift.
//     Older behavior is kept around for staff ghost compat.
//   - Also, polyobject bounce-back was fixed!
// - 0x000B (Ring Racers v2.1 + In dev revisions)
//   - SPB cup TA replays were recorded at this time
//   - Slope physics changed with a scaling fix
// - 0x000C (Ring Racers v2.2)
// - 0x000D (Ring Racers v2.3)

#define DEMOVERSION 0x000D

boolean G_CompatLevel(UINT16 level)
{
	if (demo.playback)
	{
		// Check gameplay differences for older ghosts
		return (demo.version <= level);
	}

	return false;
}

#define DEMOHEADER  "\xF0" "KartReplay" "\x0F"

#define DF_ATTACKMASK   (ATTACKING_TIME|ATTACKING_LAP|ATTACKING_SPB) // This demo contains time/lap data

#define DF_GHOST		0x08 // This demo contains ghost data too!

#define DF_NONETMP		0x10 // multiplayer but not netgame

#define DF_LUAVARS		0x20 // this demo contains extra lua vars

// woah there pardner, if you modify this check k_menu.h too
#define DF_ENCORE       0x40
#define DF_MULTIPLAYER  0x80 // This demo was recorded in multiplayer mode!

#define DF_GRANDPRIX    0x0100

#define DEMO_SPECTATOR		0x01
#define DEMO_KICKSTART		0x02
#define DEMO_SHRINKME		0x04
#define DEMO_BOT			0x08
#define DEMO_AUTOROULETTE	0x10
#define DEMO_AUTORING		0x20

// For demos
#define ZT_FWD		0x0001
#define ZT_SIDE		0x0002
#define ZT_TURNING	0x0004
#define ZT_ANGLE	0x0008
#define ZT_THROWDIR	0x0010
#define ZT_BUTTONS	0x0020
#define ZT_AIMING	0x0040
#define ZT_LATENCY	0x0080
#define ZT_FLAGS	0x0100
#define ZT_BOT		0x8000
// Ziptics are UINT16 now, go nuts

#define ZT_BOT_TURN			0x0001
#define ZT_BOT_SPINDASH		0x0002
#define ZT_BOT_ITEM			0x0004

#define DEMOMARKER 0x80 // demobuf.end

UINT8 demo_extradata[MAXPLAYERS];
UINT8 demo_writerng; // 0=no, 1=yes, 2=yes but on a timeout
static ticcmd_t oldcmd[MAXPLAYERS];

#define DW_END        0xFF // End of extradata block
#define DW_RNG        0xFE // Check RNG seed!

#define DW_EXTRASTUFF 0xFE // Numbers below this are reserved for writing player slot data

// Below consts are only used for demo extrainfo sections
#define DW_STANDING 0x00
#define DW_STANDING2 0x01

// For time attack ghosts
#define GZT_XYZ    0x01
#define GZT_MOMXY  0x02
#define GZT_MOMZ   0x04
#define GZT_ANGLE  0x08
#define GZT_FRAME  0x10 // Animation frame
#define GZT_SPR2   0x20 // Player animations
#define GZT_EXTRA  0x40
#define GZT_FOLLOW 0x80 // Followmobj

// GZT_EXTRA flags (currently UINT8)
#define EZT_COLOR    0x01 // Changed color (Super transformation, Mario fireflowers/invulnerability, etc.)
#define EZT_FLIP     0x02 // Reversed gravity
#define EZT_SCALE    0x04 // Changed size
#define EZT_HIT      0x08 // Damaged a mobj
#define EZT_SPRITE   0x10 // Changed sprite set completely out of PLAY (NiGHTS, SOCs, whatever)
#define EZT_ITEMDATA 0x20 // Changed current held item/quantity and bumpers for battle
#define EZT_STATDATA 0x40 // Changed skin/stats

// GZT_FOLLOW flags
#define FZT_SPAWNED 0x01 // just been spawned
#define FZT_SKIN 0x02 // has skin
#define FZT_LINKDRAW 0x04 // has linkdraw (combine with spawned only)
#define FZT_COLORIZED 0x08 // colorized (ditto)
#define FZT_SCALE 0x10 // different scale to object
// spare FZT slots 0x20 to 0x80

static mobj_t oldghost[MAXPLAYERS];

void G_ReadDemoExtraData(void)
{
	INT32 p, extradata, i;
	char name[64];
	static_assert(sizeof name >= std::max({MAXPLAYERNAME+1u, SKINNAMESIZE+1u, MAXCOLORNAME+1u}));

	if (leveltime > starttime)
	{
		rewind_t *rewind = CL_SaveRewindPoint(demobuf.p - demobuf.buffer);
		if (rewind)
		{
			memcpy(rewind->oldcmd, oldcmd, sizeof (oldcmd));
			memcpy(rewind->oldghost, oldghost, sizeof (oldghost));
		}
	}

	memset(name, '\0', sizeof name);

	p = READUINT8(demobuf.p);

	while (p < DW_EXTRASTUFF)
	{
		extradata = READUINT8(demobuf.p);

		if (extradata & DXD_JOINDATA)
		{
			if (!playeringame[p])
			{
				G_AddPlayer(p, p);
			}

			for (i = 0; i < MAXAVAILABILITY; i++)
			{
				players[p].availabilities[i] = READUINT8(demobuf.p);
			}

			players[p].bot = !!(READUINT8(demobuf.p));
			if (players[p].bot)
			{
				players[p].botvars.difficulty = READUINT8(demobuf.p);
				players[p].botvars.diffincrease = READUINT8(demobuf.p); // needed to avoid having to duplicate logic
				players[p].botvars.rival = (boolean)READUINT8(demobuf.p);
			}
		}
		if (extradata & DXD_PLAYSTATE)
		{
			i = READUINT8(demobuf.p);

			switch (i) {
			case DXD_PST_PLAYING:
				if (players[p].spectator == true)
				{
					if (players[p].bot)
					{
						players[p].spectator = false;
					}
					else
					{
						players[p].pflags |= PF_WANTSTOJOIN;
					}
				}
				//CONS_Printf("player %s is despectating on tic %d\n", player_names[p], leveltime);
				break;

			case DXD_PST_SPECTATING:
				if (players[p].spectator)
				{
					players[p].pflags &= ~PF_WANTSTOJOIN;
				}
				else
				{
					if (players[p].mo)
					{
						P_DamageMobj(players[p].mo, NULL, NULL, 1, DMG_SPECTATOR);
					}
					P_SetPlayerSpectator(p);
				}

				break;

			case DXD_PST_LEFT:
				CL_RemovePlayer(p, static_cast<kickreason_t>(0));
				break;
			}

			G_ResetViews();

			// maybe these are necessary?
			K_CheckBumpers();
			P_CheckRacers();
		}
		if (extradata & DXD_SKIN)
		{
			UINT8 skinid;

			// Skin

			skinid = READUINT8(demobuf.p);
			if (skinid >= demo.numskins)
				skinid = 0;
			ghostext[p].skinid = demo.currentskinid[p] = skinid;
			SetPlayerSkinByNum(p, skinid);

			players[p].kartspeed = ghostext[p].kartspeed = demo.skinlist[skinid].kartspeed;
			players[p].kartweight = ghostext[p].kartweight = demo.skinlist[skinid].kartweight;
			players[p].charflags = ghostext[p].charflags = demo.skinlist[skinid].flags;
		}
		if (extradata & DXD_COLOR)
		{
			// Color
			demobuf.p += copy_fixed_buf(name, demobuf.p, g_buffer_sizes.color_name);
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
			demobuf.p += copy_fixed_buf(player_names[p], demobuf.p, g_buffer_sizes.player_name);
		}
		if (extradata & DXD_FOLLOWER)
		{
			// Set our follower
			demobuf.p += copy_fixed_buf(name, demobuf.p, g_buffer_sizes.skin_name);
			K_SetFollowerByName(p, name);

			// Follower's color
			demobuf.p += copy_fixed_buf(name, demobuf.p, g_buffer_sizes.color_name);
			for (i = 0; i < numskincolors +2; i++)	// +2 because of Match and Opposite
			{
				if (!stricmp(Followercolor_cons_t[i].strvalue, name))
				{
					players[p].followercolor = Followercolor_cons_t[i].value;
					break;
				}
			}
		}
		if (extradata & DXD_WEAPONPREF)
		{
			demobuf.p += WeaponPref_Parse(demobuf.p, p);

			//CONS_Printf("weaponpref is %d for player %d\n", i, p);
		}

		p = READUINT8(demobuf.p);
	}

	while (p != DW_END)
	{
		UINT32 rng;
		boolean storesynced = demosynced;

		switch (p)
		{
		case DW_RNG:
			for (i = 0; i < PRNUMSYNCED; i++)
			{
				rng = READUINT32(demobuf.p);

				if (P_GetRandSeed(static_cast<pr_class_t>(i)) != rng)
				{
					P_SetRandSeed(static_cast<pr_class_t>(i), rng);

					if (demosynced)
						CONS_Alert(CONS_WARNING, "Demo playback has desynced (RNG class %d)!\n", i);
					storesynced = false;
				}
			}
			demosynced = storesynced;
		}

		p = READUINT8(demobuf.p);
	}

	if (!(demoflags & DF_GHOST) && *demobuf.p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

void G_WriteDemoExtraData(void)
{
	INT32 i, j;
	char name[64];
	static_assert(sizeof name >= std::max({MAXPLAYERNAME+1u, SKINNAMESIZE+1u, MAXCOLORNAME+1u}));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (demo_extradata[i])
		{
			WRITEUINT8(demobuf.p, i);
			WRITEUINT8(demobuf.p, demo_extradata[i]);

			if (demo_extradata[i] & DXD_JOINDATA)
			{
				for (j = 0; j < MAXAVAILABILITY; j++)
				{
					WRITEUINT8(demobuf.p, players[i].availabilities[j]);
				}

				WRITEUINT8(demobuf.p, (UINT8)players[i].bot);
				if (players[i].bot)
				{
					WRITEUINT8(demobuf.p, players[i].botvars.difficulty);
					WRITEUINT8(demobuf.p, players[i].botvars.diffincrease); // needed to avoid having to duplicate logic
					WRITEUINT8(demobuf.p, (UINT8)players[i].botvars.rival);
				}
			}
			if (demo_extradata[i] & DXD_PLAYSTATE)
			{
				UINT8 pst = DXD_PST_PLAYING;

				demo_writerng = 1;

				if (!playeringame[i])
				{
					pst = DXD_PST_LEFT;
				}
				else if (
					players[i].spectator &&
					!(players[i].pflags & PF_WANTSTOJOIN) // <= fuck you specifically
				)
				{
					pst = DXD_PST_SPECTATING;
				}

				WRITEUINT8(demobuf.p, pst);
			}
			//if (demo_extradata[i] & DXD_RESPAWN) has no extra data
			if (demo_extradata[i] & DXD_SKIN)
			{
				// Skin
				WRITEUINT8(demobuf.p, players[i].skin);
			}
			if (demo_extradata[i] & DXD_COLOR)
			{
				// Color
				demobuf.p += copy_fixed_buf(demobuf.p, skincolors[players[i].skincolor].name, g_buffer_sizes.color_name);
			}
			if (demo_extradata[i] & DXD_NAME)
			{
				// Name
				demobuf.p += copy_fixed_buf(demobuf.p, player_names[i], g_buffer_sizes.player_name);
			}
			if (demo_extradata[i] & DXD_FOLLOWER)
			{
				// write follower
				if (players[i].followerskin == -1)
					demobuf.p += copy_fixed_buf(demobuf.p, "None", g_buffer_sizes.skin_name);
				else
					demobuf.p += copy_fixed_buf(demobuf.p, followers[players[i].followerskin].name, g_buffer_sizes.skin_name);

				// write follower color
				for (j = (numskincolors+2)-1; j > 0; j--)
				{
					if (Followercolor_cons_t[j].value == players[i].followercolor)
						break;
				}
				demobuf.p += copy_fixed_buf(demobuf.p, Followercolor_cons_t[j].strvalue, g_buffer_sizes.color_name);	// Not KartColor_Names because followercolor has extra values such as "Match"
			}
			if (demo_extradata[i] & DXD_WEAPONPREF)
			{
				WeaponPref_Save(&demobuf.p, i);
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
			WRITEUINT8(demobuf.p, DW_RNG);

			for (i = 0; i < PRNUMSYNCED; i++)
			{
				WRITEUINT32(demobuf.p, P_GetRandSeed(static_cast<pr_class_t>(i)));
			}
		}
	}

	WRITEUINT8(demobuf.p, DW_END);
}

void G_ReadDemoTiccmd(ticcmd_t *cmd, INT32 playernum)
{
	UINT16 ziptic;

	if (!demobuf.p || !demo.deferstart)
		return;

	ziptic = READUINT16(demobuf.p);

	if (ziptic & ZT_FWD)
		oldcmd[playernum].forwardmove = READSINT8(demobuf.p);
	if (ziptic & ZT_TURNING)
		oldcmd[playernum].turning = READINT16(demobuf.p);
	if (ziptic & ZT_ANGLE)
		oldcmd[playernum].angle = READINT16(demobuf.p);
	if (ziptic & ZT_THROWDIR)
		oldcmd[playernum].throwdir = READINT16(demobuf.p);
	if (ziptic & ZT_BUTTONS)
		oldcmd[playernum].buttons = READUINT16(demobuf.p);
	if (ziptic & ZT_AIMING)
		oldcmd[playernum].aiming = READINT16(demobuf.p);
	if (ziptic & ZT_LATENCY)
		oldcmd[playernum].latency = READUINT8(demobuf.p);
	if (ziptic & ZT_FLAGS)
		oldcmd[playernum].flags = READUINT8(demobuf.p);

	if (ziptic & ZT_BOT)
	{
		UINT16 botziptic = READUINT16(demobuf.p);

		if (botziptic & ZT_BOT_TURN)
			oldcmd[playernum].bot.turnconfirm = READSINT8(demobuf.p);
		if (botziptic & ZT_BOT_SPINDASH)
			oldcmd[playernum].bot.spindashconfirm = READSINT8(demobuf.p);
		if (botziptic & ZT_BOT_ITEM)
			oldcmd[playernum].bot.itemconfirm = READSINT8(demobuf.p);
	}

	G_CopyTiccmd(cmd, &oldcmd[playernum], 1);

	if (!(demoflags & DF_GHOST) && *demobuf.p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

void G_WriteDemoTiccmd(ticcmd_t *cmd, INT32 playernum)
{
	UINT16 ziptic = 0;
	UINT8 *ziptic_p;

	//(void)playernum;

	if (!demobuf.p)
		return;

	ziptic_p = demobuf.p; // the ziptic, written at the end of this function
	demobuf.p += 2;

	if (cmd->forwardmove != oldcmd[playernum].forwardmove)
	{
		WRITESINT8(demobuf.p,cmd->forwardmove);
		oldcmd[playernum].forwardmove = cmd->forwardmove;
		ziptic |= ZT_FWD;
	}

	if (cmd->turning != oldcmd[playernum].turning)
	{
		WRITEINT16(demobuf.p,cmd->turning);
		oldcmd[playernum].turning = cmd->turning;
		ziptic |= ZT_TURNING;
	}

	if (cmd->angle != oldcmd[playernum].angle)
	{
		WRITEINT16(demobuf.p,cmd->angle);
		oldcmd[playernum].angle = cmd->angle;
		ziptic |= ZT_ANGLE;
	}

	if (cmd->throwdir != oldcmd[playernum].throwdir)
	{
		WRITEINT16(demobuf.p,cmd->throwdir);
		oldcmd[playernum].throwdir = cmd->throwdir;
		ziptic |= ZT_THROWDIR;
	}

	if (cmd->buttons != oldcmd[playernum].buttons)
	{
		WRITEUINT16(demobuf.p,cmd->buttons);
		oldcmd[playernum].buttons = cmd->buttons;
		ziptic |= ZT_BUTTONS;
	}

	if (cmd->aiming != oldcmd[playernum].aiming)
	{
		WRITEINT16(demobuf.p,cmd->aiming);
		oldcmd[playernum].aiming = cmd->aiming;
		ziptic |= ZT_AIMING;
	}

	if (cmd->latency != oldcmd[playernum].latency)
	{
		WRITEUINT8(demobuf.p,cmd->latency);
		oldcmd[playernum].latency = cmd->latency;
		ziptic |= ZT_LATENCY;
	}

	if (cmd->flags != oldcmd[playernum].flags)
	{
		WRITEUINT8(demobuf.p,cmd->flags);
		oldcmd[playernum].flags = cmd->flags;
		ziptic |= ZT_FLAGS;
	}

	if (cmd->flags & TICCMD_BOT)
	{
		ziptic |= ZT_BOT;
	}

	WRITEUINT16(ziptic_p, ziptic);

	if (ziptic & ZT_BOT)
	{
		UINT16 botziptic = 0;
		UINT8 *botziptic_p;

		botziptic_p = demobuf.p; // the ziptic, written at the end of this function
		demobuf.p += 2;

		if (cmd->bot.turnconfirm != oldcmd[playernum].bot.turnconfirm)
		{
			WRITESINT8(demobuf.p, cmd->bot.turnconfirm);
			oldcmd[playernum].bot.turnconfirm = cmd->bot.turnconfirm;
			botziptic |= ZT_BOT_TURN;
		}

		if (cmd->bot.spindashconfirm != oldcmd[playernum].bot.spindashconfirm)
		{
			WRITESINT8(demobuf.p, cmd->bot.spindashconfirm);
			oldcmd[playernum].bot.spindashconfirm = cmd->bot.spindashconfirm;
			botziptic |= ZT_BOT_SPINDASH;
		}

		if (cmd->bot.itemconfirm != oldcmd[playernum].bot.itemconfirm)
		{
			WRITESINT8(demobuf.p, cmd->bot.itemconfirm);
			oldcmd[playernum].bot.itemconfirm = cmd->bot.itemconfirm;
			botziptic |= ZT_BOT_ITEM;
		}

		WRITEUINT16(botziptic_p, botziptic);
	}

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (!(demoflags & DF_GHOST) && ziptic_p > demobuf.end - 9)
	{
		G_CheckDemoStatus(); // no more space
		return;
	}
}

void G_GhostAddFlip(INT32 playernum)
{
	if ((!demo.recording || !(demoflags & DF_GHOST)))
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
	if ((!demo.recording || !(demoflags & DF_GHOST)))
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
	ghostext[playernum].hitlist = static_cast<mobj_t**>(Z_Realloc(ghostext[playernum].hitlist, ghostext[playernum].hits * sizeof(mobj_t *), PU_LEVEL, NULL));
	P_SetTarget(ghostext[playernum].hitlist + (ghostext[playernum].hits-1), victim);
}

void G_WriteAllGhostTics(void)
{
	boolean toobig = false;
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

		WRITEUINT8(demobuf.p, i);
		G_WriteGhostTic(players[i].mo, i);

		// attention here for the ticcmd size!
		// latest demos with mouse aiming byte in ticcmd
		if (demobuf.p >= demobuf.end - (13 + 9 + 9))
		{
			toobig = true;
			break;
		}
	}
	WRITEUINT8(demobuf.p, 0xFF);

	if (toobig)
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

	if (!demobuf.p)
		return;
	if (!(demoflags & DF_GHOST))
		return; // No ghost data to write.

	ziptic_p = demobuf.p++; // the ziptic, written at the end of this function

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
		WRITEFIXED(demobuf.p,oldghost[playernum].x);
		WRITEFIXED(demobuf.p,oldghost[playernum].y);
		WRITEFIXED(demobuf.p,oldghost[playernum].z);
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
			WRITEFIXED(demobuf.p,momx);
			WRITEFIXED(demobuf.p,momy);
		}

		momx = ghost->z-oldghost[playernum].z;
		if (momx != oldghost[playernum].momz)
		{
			oldghost[playernum].momz = momx;
			ziptic |= GZT_MOMZ;
			WRITEFIXED(demobuf.p,momx);
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
		WRITEUINT8(demobuf.p,oldghost[playernum].angle);
	}

	// Store the sprite frame.
	if ((ghost->frame & FF_FRAMEMASK) != oldghost[playernum].frame)
	{
		oldghost[playernum].frame = (ghost->frame & FF_FRAMEMASK);
		ziptic |= GZT_FRAME;
		WRITEUINT8(demobuf.p,oldghost[playernum].frame);
	}

	if (ghost->sprite == SPR_PLAY
	&& ghost->sprite2 != oldghost[playernum].sprite2)
	{
		oldghost[playernum].sprite2 = ghost->sprite2;
		ziptic |= GZT_SPR2;
		WRITEUINT8(demobuf.p,oldghost[playernum].sprite2);
	}

	// Check for sprite set changes
	if (ghost->sprite != oldghost[playernum].sprite)
	{
		oldghost[playernum].sprite = ghost->sprite;
		ghostext[playernum].flags |= EZT_SPRITE;
	}

	if (ghost->player && (
			ghostext[playernum].itemtype != ghost->player->itemtype ||
			ghostext[playernum].itemamount != ghost->player->itemamount ||
			ghostext[playernum].health != ghost->health
		))
	{
		ghostext[playernum].flags |= EZT_ITEMDATA;
		ghostext[playernum].itemtype = ghost->player->itemtype;
		ghostext[playernum].itemamount = ghost->player->itemamount;
		ghostext[playernum].health = ghost->health;
	}

	if (ghost->player && (
			ghostext[playernum].skinid != (UINT8)(((skin_t *)ghost->skin)-skins) ||
			ghostext[playernum].kartspeed != ghost->player->kartspeed ||
			ghostext[playernum].kartweight != ghost->player->kartweight ||
			ghostext[playernum].charflags != ghost->player->charflags
		))
	{
		ghostext[playernum].flags |= EZT_STATDATA;
		ghostext[playernum].skinid = (UINT8)(((skin_t *)ghost->skin)-skins);
		ghostext[playernum].kartspeed = ghost->player->kartspeed;
		ghostext[playernum].kartweight = ghost->player->kartweight;
		ghostext[playernum].charflags = ghost->player->charflags;
	}

	if (ghostext[playernum].flags)
	{
		ziptic |= GZT_EXTRA;

		if (ghostext[playernum].color == ghostext[playernum].lastcolor)
			ghostext[playernum].flags &= ~EZT_COLOR;
		if (ghostext[playernum].scale == ghostext[playernum].lastscale)
			ghostext[playernum].flags &= ~EZT_SCALE;

		WRITEUINT8(demobuf.p,ghostext[playernum].flags);
		if (ghostext[playernum].flags & EZT_COLOR)
		{
			WRITEUINT16(demobuf.p,ghostext[playernum].color);
			ghostext[playernum].lastcolor = ghostext[playernum].color;
		}
		if (ghostext[playernum].flags & EZT_SCALE)
		{
			WRITEFIXED(demobuf.p,ghostext[playernum].scale);
			ghostext[playernum].lastscale = ghostext[playernum].scale;
		}
		if (ghostext[playernum].flags & EZT_HIT)
		{
			WRITEUINT16(demobuf.p,ghostext[playernum].hits);
			for (i = 0; i < ghostext[playernum].hits; i++)
			{
				mobj_t *mo = ghostext[playernum].hitlist[i];
				//WRITEUINT32(demobuf.p,UINT32_MAX); // reserved for some method of determining exactly which mobj this is. (mobjnum doesn't work here.)
				WRITEUINT32(demobuf.p,mo->type);
				WRITEUINT16(demobuf.p,(UINT16)mo->health);
				WRITEFIXED(demobuf.p,mo->x);
				WRITEFIXED(demobuf.p,mo->y);
				WRITEFIXED(demobuf.p,mo->z);
				WRITEANGLE(demobuf.p,mo->angle);
				P_SetTarget(ghostext[playernum].hitlist+i, NULL);
			}
			Z_Free(ghostext[playernum].hitlist);
			ghostext[playernum].hits = 0;
			ghostext[playernum].hitlist = NULL;
		}
		if (ghostext[playernum].flags & EZT_SPRITE)
			WRITEUINT16(demobuf.p,oldghost[playernum].sprite);
		if (ghostext[playernum].flags & EZT_ITEMDATA)
		{
			WRITESINT8(demobuf.p, ghostext[playernum].itemtype);
			WRITEUINT8(demobuf.p, ghostext[playernum].itemamount);
			WRITEINT32(demobuf.p, ghostext[playernum].health);
		}
		if (ghostext[playernum].flags & EZT_STATDATA)
		{
			WRITEUINT8(demobuf.p,ghostext[playernum].skinid);
			WRITEUINT8(demobuf.p,ghostext[playernum].kartspeed);
			WRITEUINT8(demobuf.p,ghostext[playernum].kartweight);
			WRITEUINT32(demobuf.p, ghostext[playernum].charflags);
		}

		ghostext[playernum].flags = 0;
	}

	if (ghost->player && ghost->player->followmobj&& !(ghost->player->followmobj->sprite == SPR_NULL || (ghost->player->followmobj->renderflags & RF_DONTDRAW) == RF_DONTDRAW)) // bloats tails runs but what can ya do
	{
		fixed_t temp;
		UINT8 *followtic_p = demobuf.p++;
		UINT8 followtic = 0;

		ziptic |= GZT_FOLLOW;

		if (ghost->player->followmobj->skin)
			followtic |= FZT_SKIN;

		if (!(oldghost[playernum].flags2 & MF2_AMBUSH))
		{
			followtic |= FZT_SPAWNED;
			WRITEINT16(demobuf.p,ghost->player->followmobj->info->height>>FRACBITS);
			if (ghost->player->followmobj->flags2 & MF2_LINKDRAW)
				followtic |= FZT_LINKDRAW;
			if (ghost->player->followmobj->colorized)
				followtic |= FZT_COLORIZED;
			if (followtic & FZT_SKIN)
				WRITEUINT8(demobuf.p,(UINT8)(((skin_t *)(ghost->player->followmobj->skin))-skins));
			oldghost[playernum].flags2 |= MF2_AMBUSH;
		}

		if (ghost->player->followmobj->scale != ghost->scale)
		{
			followtic |= FZT_SCALE;
			WRITEFIXED(demobuf.p,ghost->player->followmobj->scale);
		}

		temp = ghost->player->followmobj->x-ghost->x;
		WRITEFIXED(demobuf.p,temp);
		temp = ghost->player->followmobj->y-ghost->y;
		WRITEFIXED(demobuf.p,temp);
		temp = ghost->player->followmobj->z-ghost->z;
		WRITEFIXED(demobuf.p,temp);
		if (followtic & FZT_SKIN)
			WRITEUINT8(demobuf.p,ghost->player->followmobj->sprite2);
		WRITEUINT16(demobuf.p,ghost->player->followmobj->sprite);
		WRITEUINT8(demobuf.p,(ghost->player->followmobj->frame & FF_FRAMEMASK));
		WRITEUINT16(demobuf.p,ghost->player->followmobj->color);

		*followtic_p = followtic;
	}
	else
		oldghost[playernum].flags2 &= ~MF2_AMBUSH;

	*ziptic_p = ziptic;
}

void G_ConsAllGhostTics(void)
{
	UINT8 p;

	if (!demobuf.p || !demo.deferstart)
		return;

	p = READUINT8(demobuf.p);

	while (p != 0xFF)
	{
		G_ConsGhostTic(p);
		p = READUINT8(demobuf.p);
	}

	if (*demobuf.p == DEMOMARKER)
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
	ziptic = READUINT8(demobuf.p);
	if (ziptic & GZT_XYZ)
	{
		oldghost[playernum].x = READFIXED(demobuf.p);
		oldghost[playernum].y = READFIXED(demobuf.p);
		oldghost[playernum].z = READFIXED(demobuf.p);
		syncleeway = 0;
	}
	else
	{
		if (ziptic & GZT_MOMXY)
		{
			oldghost[playernum].momx = READFIXED(demobuf.p);
			oldghost[playernum].momy = READFIXED(demobuf.p);
		}
		if (ziptic & GZT_MOMZ)
			oldghost[playernum].momz = READFIXED(demobuf.p);
		oldghost[playernum].x += oldghost[playernum].momx;
		oldghost[playernum].y += oldghost[playernum].momy;
		oldghost[playernum].z += oldghost[playernum].momz;
		syncleeway = FRACUNIT;
	}
	if (ziptic & GZT_ANGLE)
		demobuf.p++;
	if (ziptic & GZT_FRAME)
		demobuf.p++;
	if (ziptic & GZT_SPR2)
		demobuf.p++;

	if (ziptic & GZT_EXTRA)
	{ // But wait, there's more!
		UINT8 xziptic = READUINT8(demobuf.p);
		if (xziptic & EZT_COLOR)
			demobuf.p += sizeof(UINT16);
		if (xziptic & EZT_SCALE)
			demobuf.p += sizeof(fixed_t);
		if (xziptic & EZT_HIT)
		{ // Resync mob damage.
			UINT16 i, count = READUINT16(demobuf.p);
			thinker_t *th;
			mobj_t *mobj;

			UINT32 type;
			UINT16 health;
			fixed_t x;
			fixed_t y;
			fixed_t z;

			for (i = 0; i < count; i++)
			{
				//demobuf.p += 4; // reserved.
				type = READUINT32(demobuf.p);
				health = READUINT16(demobuf.p);
				x = READFIXED(demobuf.p);
				y = READFIXED(demobuf.p);
				z = READFIXED(demobuf.p);
				demobuf.p += sizeof(angle_t); // angle, unnecessary for cons.

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
			demobuf.p += sizeof(UINT16);
		if (xziptic & EZT_ITEMDATA)
		{
			ghostext[playernum].itemtype = READSINT8(demobuf.p);
			ghostext[playernum].itemamount = READUINT8(demobuf.p);
			ghostext[playernum].health = READINT32(demobuf.p);
		}
		if (xziptic & EZT_STATDATA)
		{
			ghostext[playernum].skinid = READUINT8(demobuf.p);
			if (ghostext[playernum].skinid >= demo.numskins)
				ghostext[playernum].skinid = 0;
			ghostext[playernum].kartspeed = READUINT8(demobuf.p);
			ghostext[playernum].kartweight = READUINT8(demobuf.p);
			ghostext[playernum].charflags = READUINT32(demobuf.p);
		}
	}

	if (ziptic & GZT_FOLLOW)
	{ // Even more...
		UINT8 followtic = READUINT8(demobuf.p);
		if (followtic & FZT_SPAWNED)
		{
			demobuf.p += sizeof(INT16);
			if (followtic & FZT_SKIN)
				demobuf.p++;
		}
		if (followtic & FZT_SCALE)
			demobuf.p += sizeof(fixed_t);
		// momx, momy and momz
		demobuf.p += sizeof(fixed_t) * 3;
		if (followtic & FZT_SKIN)
			demobuf.p++;
		demobuf.p += sizeof(UINT16);
		demobuf.p++;
		demobuf.p += sizeof(UINT16);
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

		if (players[playernum].itemtype != ghostext[playernum].itemtype
			|| players[playernum].itemamount != ghostext[playernum].itemamount
			|| testmo->health != ghostext[playernum].health)
		{
			if (demosynced)
				CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced (item/bumpers)!\n"));
			demosynced = false;

			players[playernum].itemtype = ghostext[playernum].itemtype;
			players[playernum].itemamount = ghostext[playernum].itemamount;
			testmo->health = ghostext[playernum].health;
		}

		if (players[playernum].kartspeed != ghostext[playernum].kartspeed
			|| players[playernum].kartweight != ghostext[playernum].kartweight
			|| players[playernum].charflags != ghostext[playernum].charflags ||
			demo.skinlist[ghostext[playernum].skinid].mapping != (UINT8)(((skin_t *)testmo->skin)-skins))
		{
			if (demosynced)
				CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced (Character/stats)!\n"));
			demosynced = false;

			testmo->skin = &skins[demo.skinlist[ghostext[playernum].skinid].mapping];
			players[playernum].kartspeed = ghostext[playernum].kartspeed;
			players[playernum].kartweight = ghostext[playernum].kartweight;
			players[playernum].charflags = ghostext[playernum].charflags;

			if (demo.skinlist[demo.currentskinid[playernum]].flags & SF_IRONMAN)
			{
				players[playernum].lastfakeskin = players[playernum].fakeskin;
				players[playernum].fakeskin =
					(ghostext[playernum].skinid == demo.currentskinid[playernum])
					? MAXSKINS
					: ghostext[playernum].skinid;
			}
		}
	}

	if (*demobuf.p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

void G_GhostTicker(void)
{
	demoghost *g,*p;
	for (g = ghosts, p = NULL; g; g = g->next)
	{
		UINT16 ziptic;
		UINT8 xziptic;

		if (g->done)
		{
			continue;
		}

		// Pause jhosts that cross until the timer starts.
		if (g->linecrossed && leveltime < starttime && G_TimeAttackStart())
			continue;

readghosttic:

		// Skip normal demo data.
		ziptic = READUINT8(g->p);
		xziptic = 0;

		while (ziptic != DW_END) // Get rid of extradata stuff
		{
			if (ziptic < MAXPLAYERS)
			{
#ifdef DEVELOP
				UINT8 playerid = ziptic;
#endif
				// We want to skip *any* player extradata because some demos have extradata for bogus players,
				// but if there is tic data later for those players *then* we'll consider it invalid.

				ziptic = READUINT8(g->p);
				if (ziptic & DXD_JOINDATA)
				{
					g->p += MAXAVAILABILITY;
					if (READUINT8(g->p) != 0)
						I_Error("Ghost is not a record attack ghost (bot JOINDATA)");
				}
				if (ziptic & DXD_PLAYSTATE)
				{
					UINT8 playstate = READUINT8(g->p);
					if (playstate != DXD_PST_PLAYING)
					{
#ifdef DEVELOP
						CONS_Alert(CONS_WARNING, "Ghost demo has non-playing playstate for player %d\n", playerid + 1);
#endif
						;
					}
				}
				if (ziptic & DXD_SKIN)
					g->p++; // We _could_ read this info, but it shouldn't change anything in record attack...
				if (ziptic & DXD_COLOR)
					g->p += g->sizes.color_name; // Same tbh
				if (ziptic & DXD_NAME)
					g->p += g->sizes.player_name; // yea
				if (ziptic & DXD_FOLLOWER)
					g->p += g->sizes.skin_name + g->sizes.color_name;
				if (ziptic & DXD_WEAPONPREF)
					g->p++; // ditto
				if (ziptic & DXD_START)
					g->linecrossed = true;
			}
			else if (ziptic == DW_RNG)
			{
				INT32 i;
				for (i = 0; i < PRNUMSYNCED; i++)
				{
					g->p += 4; // RNG seed
				}
			}
			else
			{
				I_Error("Ghost is not a record attack ghost DXD (ziptic = %u)", ziptic); //@TODO lmao don't blow up like this
			}

			ziptic = READUINT8(g->p);
		}

		ziptic = READUINT16(g->p);

		if (ziptic & ZT_FWD)
			g->p++;
		if (ziptic & ZT_TURNING)
			g->p += 2;
		if (ziptic & ZT_ANGLE)
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
		if (ziptic & ZT_BOT)
		{
			UINT16 botziptic = READUINT16(g->p);
			if (botziptic & ZT_BOT_TURN)
				g->p++;
			if (botziptic & ZT_BOT_SPINDASH)
				g->p++;
			if (botziptic & ZT_BOT_ITEM)
				g->p++;
		}

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

		if (ziptic & GZT_EXTRA)
		{ // But wait, there's more!
			xziptic = READUINT8(g->p);
			if (xziptic & EZT_COLOR)
			{
				g->color = READUINT16(g->p);
				switch(g->color)
				{
				default:
				case GHC_NORMAL: // Go back to skin color
					g->mo->color = g->oldmo.color;
					break;
				// Handled below
				case GHC_SUPER:
				case GHC_INVINCIBLE:
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
					|| !(mobjinfo[type].flags & MF_ENEMY)
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
				g->mo->sprite = static_cast<spritenum_t>(READUINT16(g->p));
			if (xziptic & EZT_ITEMDATA)
				g->p += 1 + 1 + 4; // itemtype, itemamount, health
			if (xziptic & EZT_STATDATA)
			{
				UINT8 skinid = READUINT8(g->p);
				if (skinid >= g->numskins)
					skinid = 0;
				g->mo->skin = &skins[g->skinlist[skinid].mapping];
				g->p += 6; // kartspeed, kartweight, charflags
			}
		}

		// todo better defaulting
		g->mo->sprite2 = g->oldmo.sprite2;
		g->mo->frame = g->oldmo.frame | tr_trans30<<FF_TRANSSHIFT;
		if (g->fadein)
		{
			g->mo->frame += (((--g->fadein)/6)<<FF_TRANSSHIFT); // this calc never exceeds 9 unless g->fadein is bad, and it's only set once, so...
			g->mo->renderflags &= ~RF_DONTDRAW;
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
				follow->sprite = static_cast<spritenum_t>(READUINT16(g->p));
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
#if 0 // freeze frame (maybe more useful for time attackers) (2024-03-11: you leave it behind anyway!)
			g->mo->colorized = true;
			g->mo->fuse = 10*TICRATE;
			if (follow)
				follow->colorized = true;
#else // dissapearing act
			g->mo->fuse = TICRATE;
			if (follow)
				follow->fuse = TICRATE;
#endif
			g->done = true;
			if (p)
			{
				p->next = g->next;
			}
			continue;
		}

		// If the timer started, skip ahead until the ghost starts too.
		if (starttime <= leveltime && !g->linecrossed && G_TimeAttackStart())
			goto readghosttic;

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

	info = static_cast<rewindinfo_t*>(Z_Calloc(sizeof(rewindinfo_t), PU_STATIC, NULL));

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
			demobuf.p = demobuf.buffer + rewind->demopos;
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

//
// G_RecordDemo
//
void G_RecordDemo(const char *name)
{
	if (demo.recording)
		G_CheckDemoStatus();

	extern consvar_t cv_netdemosize;

	INT32 maxsize;

	strcpy(demoname, name);
	strcat(demoname, ".lmp");
	maxsize = 1024 * 1024 * cv_netdemosize.value;

//	if (demobuf.buffer)
//		Z_Free(demobuf.buffer);

	// FIXME: this file doesn't manage its memory and actually free this when it's done using it
	Z_Free(demobuf.buffer);
	P_SaveBufferAlloc(&demobuf, maxsize);
	Z_SetUser(demobuf.buffer, (void**)&demobuf.buffer);
	demobuf.p = NULL;

	demo.recording = true;
	demo.buffer = &demobuf;

	/* FIXME: This whole file is in a wretched state. Take a
	look at G_WriteAllGhostTics and G_WriteDemoTiccmd, they
	write a lot of data. It's not realistic to refactor that
	code in order to know exactly HOW MANY bytes it can write
	out. So here's the deal. Reserve a decent block of memory
	at the end of the buffer and never use it. Those bastard
	functions will check if they overran the buffer, but it
	should be safe enough because they'll think there's less
	memory than there actually is and stop early. */
	const size_t deadspace = 1024;
	I_Assert(demobuf.size > deadspace);
	demobuf.size -= deadspace;
	demobuf.end -= deadspace;
}

static void G_SaveDemoExtraFiles(UINT8 **pp)
{
	char *filename;
	UINT8 totalfiles = 0, i;
	UINT8 *m = (*pp);/* file count */
	(*pp)++;

	for (i = mainwads; ++i < numwadfiles; )
		if (wadfiles[i]->important)
	{
		nameonly(( filename = va("%s", wadfiles[i]->filename) ));
		WRITESTRINGL((*pp), filename, MAX_WADPATH);
		WRITEMEM((*pp), wadfiles[i]->md5sum, 16);

		totalfiles++;
	}

	WRITEUINT8(m, totalfiles);
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
					M_StartMessage("Demo Playback", M_GetText("There are too many files loaded to add this demo's addons.\n\nDemo playback may desync.\n"), NULL, MM_NOTHING, NULL, NULL);
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
					M_StartMessage("Demo Playback", M_GetText("There were errors trying to add this demo's addons. Check the console for more information.\n\nDemo playback may desync.\n"), NULL, MM_NOTHING, NULL, NULL);
			}
			else
			{
				P_PartialAddWadFile(filename);
			}
		}
	}

	if (P_PartialAddGetStage() >= 0)
		P_MultiSetupWadFiles(true); // in case any partial adds were done
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
static UINT8 G_CheckDemoExtraFiles(savebuffer_t *info, boolean quick)
{
	UINT8 totalfiles, filesloaded, nmusfilecount;
	char filename[MAX_WADPATH];
	UINT8 md5sum[16];
	boolean toomany = false;
	boolean alreadyloaded;
	UINT8 i, j;
	UINT8 error = 0;

	if (P_SaveBufferRemaining(info) < 1)
	{
		return DFILE_ERROR_CORRUPT;
	}

	totalfiles = READUINT8(info->p);
	filesloaded = 0;
	for (i = 0; i < totalfiles; ++i)
	{
		if (!toomany)
		{
			strlcpy(filename, (char *)info->p, std::min(P_SaveBufferRemaining(info) + 1, sizeof filename));
		}
		SKIPSTRINGN(info->p, P_SaveBufferRemaining(info));

		if (P_SaveBufferRemaining(info) < 16)
		{
			return DFILE_ERROR_CORRUPT;
		}

		READMEM(info->p, md5sum, 16);

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

static void G_SaveDemoSkins(UINT8 **pp, const DemoBufferSizes &psizes)
{
	UINT8 i;
	UINT8 *availabilitiesbuffer = R_GetSkinAvailabilities(true, -1);

	WRITEUINT8((*pp), numskins);
	for (i = 0; i < numskins; i++)
	{
		// Skinname, for first attempt at identification.
		(*pp) += copy_fixed_buf((*pp), skins[i].name, psizes.skin_name);

		// Backup information for second pass.
		WRITEUINT8((*pp), skins[i].kartspeed);
		WRITEUINT8((*pp), skins[i].kartweight);
		WRITEUINT32((*pp), skins[i].flags);
	}

	for (i = 0; i < MAXAVAILABILITY; i++)
	{
		WRITEUINT8((*pp), availabilitiesbuffer[i]);
	}
}

static democharlist_t *G_LoadDemoSkins(const DemoBufferSizes &psizes, savebuffer_t *info, UINT8 *worknumskins, boolean getclosest)
{
	UINT8 i, byte, shif;
	democharlist_t *skinlist = NULL;

	if (P_SaveBufferRemaining(info) < 1)
	{
		return NULL;
	}

	(*worknumskins) = READUINT8(info->p);
	if (!(*worknumskins))
		return NULL;

	skinlist = static_cast<democharlist_t*>(Z_Calloc(sizeof(democharlist_t) * (*worknumskins), PU_STATIC, NULL));
	if (!skinlist)
	{
		I_Error("G_LoadDemoSkins: Insufficient memory to allocate list");
	}

	for (i = 0; i < (*worknumskins); i++)
	{
		INT32 result = -1;

		if (P_SaveBufferRemaining(info) < psizes.skin_name+1+1+4)
		{
			Z_Free(skinlist);
			return NULL;
		}

		info->p += copy_fixed_buf(skinlist[i].name, info->p, psizes.skin_name);
		skinlist[i].namehash = quickncasehash(skinlist[i].name, SKINNAMESIZE);
		skinlist[i].kartspeed = READUINT8(info->p);
		skinlist[i].kartweight = READUINT8(info->p);
		skinlist[i].flags = READUINT32(info->p);

		result = R_SkinAvailableEx(skinlist[i].name, false);
		if (result == -1)
		{
			if (!getclosest)
			{
				result = MAXSKINS;
			}
			else
			{
				result = GetSkinNumClosestToStats(skinlist[i].kartspeed, skinlist[i].kartweight, skinlist[i].flags, true);
			}
		}

		if (result != -1)
		{
			skinlist[i].mapping = (UINT8)result;
		}
	}

	for (byte = 0; byte < MAXAVAILABILITY; byte++)
	{
		if (P_SaveBufferRemaining(info) < 1)
		{
			Z_Free(skinlist);
			return NULL;
		}

		UINT8 availabilitiesbuffer = READUINT8(info->p);

		for (shif = 0; shif < 8; shif++)
		{
			i = (byte*8) + shif;

			if (i >= (*worknumskins))
				break;

			if (availabilitiesbuffer & (1 << shif))
			{
				skinlist[i].unlockrequired = true;
			}
		}
	}

	return skinlist;
}

static void G_SkipDemoSkins(UINT8 **pp, const DemoBufferSizes& psizes)
{
	UINT8 demonumskins;
	UINT8 i;

	demonumskins = READUINT8((*pp));
	for (i = 0; i < demonumskins; ++i)
	{
		(*pp) += psizes.skin_name; // name
		(*pp)++; // kartspeed
		(*pp)++; // kartweight
		(*pp) += 4; // flags
	}

	(*pp) += MAXAVAILABILITY;
}

void G_BeginRecording(void)
{
	UINT8 i, j, p;
	player_t *player = &players[consoleplayer];

	if (demobuf.p)
		return;

	demobuf.p = demobuf.buffer;

	demoflags = DF_GHOST;

	if (multiplayer)
	{
		demoflags |= DF_MULTIPLAYER;
		if (!netgame)
			demoflags |= DF_NONETMP;
	}
	else
	{
		demoflags |= modeattacking;
	}

	if (encoremode)
		demoflags |= DF_ENCORE;

	if (multiplayer)
		demoflags |= DF_LUAVARS;

	if (grandprixinfo.gp)
		demoflags |= DF_GRANDPRIX;

	// Setup header.
	M_Memcpy(demobuf.p, DEMOHEADER, 12); demobuf.p += 12;
	WRITEUINT8(demobuf.p,VERSION);
	WRITEUINT8(demobuf.p,SUBVERSION);
	WRITEUINT16(demobuf.p,DEMOVERSION);

	demo.version = DEMOVERSION;
	g_buffer_sizes = get_buffer_sizes(DEMOVERSION);

	// Full replay title
	demobuf.p += 64;
	{
		char *title = G_BuildMapTitle(gamemap);
		snprintf(demo.titlename, 64, "%s - %s", title, modeattacking ? "Record Attack" : connectedservername);
		Z_Free(title);
	}

	// demo checksum
	demobuf.p += 16;

	// game data
	M_Memcpy(demobuf.p, "PLAY", 4); demobuf.p += 4;
	WRITESTRINGN(demobuf.p, mapheaderinfo[gamemap-1]->lumpname, MAXMAPLUMPNAME);
	M_Memcpy(demobuf.p, mapmd5, 16); demobuf.p += 16;

	WRITEUINT16(demobuf.p, demoflags);

	WRITESTRINGN(demobuf.p, gametypes[gametype]->name, MAXGAMETYPELENGTH);

	WRITEUINT8(demobuf.p, numlaps);

	// file list
	G_SaveDemoExtraFiles(&demobuf.p);

	// character list
	G_SaveDemoSkins(&demobuf.p, g_buffer_sizes);

	if ((demoflags & DF_ATTACKMASK))
	{
		demotime_p = demobuf.p;

		if (demoflags & ATTACKING_TIME)
			WRITEUINT32(demobuf.p,UINT32_MAX); // time
		if (demoflags & ATTACKING_LAP)
			WRITEUINT32(demobuf.p,UINT32_MAX); // lap
	}
	else
	{
		demotime_p = NULL;
	}

	for (i = 0; i < PRNUMSYNCED; i++)
	{
		WRITEUINT32(demobuf.p, P_GetInitSeed(static_cast<pr_class_t>(i)));
	}

	// Reserved for extrainfo location from start of file
	demoinfo_p = demobuf.p;
	WRITEUINT32(demobuf.p, 0);

	// Save netvar data
	CV_SaveDemoVars(&demobuf.p);

	if ((demoflags & DF_GRANDPRIX))
	{
		WRITEUINT8(demobuf.p, grandprixinfo.gamespeed);
		WRITEUINT8(demobuf.p, grandprixinfo.masterbots == true);
		WRITEUINT8(demobuf.p, grandprixinfo.eventmode);
		WRITEUINT32(demobuf.p, grandprixinfo.specialDamage);
	}

	// Save netUnlocked from actual unlocks
	// (netUnlocked is used in m_cond.c M_CheckNetUnlockByID)
	WRITEUINT32(demobuf.p, MAXUNLOCKABLES);
	for (size_t unlockindex = 0; unlockindex < MAXUNLOCKABLES; unlockindex++)
	{
		UINT8 unlock = gamedata->unlocked[unlockindex];
		WRITEUINT8(demobuf.p, unlock);
	}

	// Save "mapmusrng" used for altmusic selection
	WRITEUINT8(demobuf.p, mapmusrng);

	// Now store some info for each in-game player

	// Lat' 12/05/19: Do note that for the first game you load, everything that gets saved here is total garbage;
	// The name will always be Player <n>, the skin sonic, the color None and the follower 0. This is only correct on subsequent games.
	// In the case of said first game, the skin and the likes are updated with Got_NameAndColor, which are then saved in extradata for the demo with DXD_SKIN in r_things.c for instance.


	for (p = 0; p < MAXPLAYERS; p++) {
		if (playeringame[p]) {
			player = &players[p];
			WRITEUINT8(demobuf.p, p);

			i = 0;
			if (player->spectator == true)
				i |= DEMO_SPECTATOR;
			if (player->pflags & PF_KICKSTARTACCEL)
				i |= DEMO_KICKSTART;
			if (player->pflags & PF_AUTOROULETTE)
				i |= DEMO_AUTOROULETTE;
			if (player->pflags & PF_AUTORING)
				i |= DEMO_AUTORING;
			if (player->pflags & PF_SHRINKME)
				i |= DEMO_SHRINKME;
			if (player->bot == true)
				i |= DEMO_BOT;
			WRITEUINT8(demobuf.p, i);

			if (i & DEMO_BOT)
			{
				WRITEUINT8(demobuf.p, player->botvars.difficulty);
				WRITEUINT8(demobuf.p, player->botvars.diffincrease); // needed to avoid having to duplicate logic
				WRITEUINT8(demobuf.p, (UINT8)player->botvars.rival);
			}

			// Name
			demobuf.p += copy_fixed_buf(demobuf.p, player_names[p], g_buffer_sizes.player_name);

			for (j = 0; j < MAXAVAILABILITY; j++)
			{
				WRITEUINT8(demobuf.p, player->availabilities[j]);
			}

			// Skin (now index into demo.skinlist)
			WRITEUINT8(demobuf.p, player->skin);
			WRITEUINT8(demobuf.p, player->lastfakeskin);

			// Color
			demobuf.p += copy_fixed_buf(demobuf.p, skincolors[player->skincolor].name, g_buffer_sizes.color_name);

			// Save follower's skin name
			// PS: We must check for 'follower' to determine if the followerskin is valid. It's going to be 0 if we don't have a follower, but 0 is also absolutely a valid follower!
			// Doesn't really matter if the follower mobj is valid so long as it exists in a way or another.

			if (player->follower)
				demobuf.p += copy_fixed_buf(demobuf.p, followers[player->followerskin].name, g_buffer_sizes.skin_name);
			else
				demobuf.p += copy_fixed_buf(demobuf.p, "None", g_buffer_sizes.skin_name);	// Say we don't have one, then.

			// Save follower's colour
			for (j = (numskincolors+2)-1; j > 0; j--)
			{
				if (Followercolor_cons_t[j].value == players[i].followercolor)
					break;
			}
			demobuf.p += copy_fixed_buf(demobuf.p, Followercolor_cons_t[j].strvalue, g_buffer_sizes.color_name);	// Not KartColor_Names because followercolor has extra values such as "Match"

			// Score, since Kart uses this to determine where you start on the map
			WRITEUINT32(demobuf.p, player->score);

			// Power Levels
			j = gametype == GT_BATTLE ? PWRLV_BATTLE : PWRLV_RACE;
			WRITEUINT16(demobuf.p, clientpowerlevels[p][j]);

			// And mobjtype_t is best with UINT32 too...
			WRITEUINT32(demobuf.p, player->followitem);

			// GP
			WRITESINT8(demobuf.p, player->lives);
			WRITEINT16(demobuf.p, player->totalring);
		}
	}

	WRITEUINT8(demobuf.p, 0xFF); // Denote the end of the player listing

	// player lua vars, always saved even if empty
	if (demoflags & DF_LUAVARS)
		LUA_Archive(&demobuf, false);

	memset(&oldcmd,0,sizeof(oldcmd));
	memset(&oldghost,0,sizeof(oldghost));
	memset(&ghostext,0,sizeof(ghostext));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		ghostext[i].lastcolor = ghostext[i].color = GHC_NORMAL;
		ghostext[i].lastscale = ghostext[i].scale = FRACUNIT;
		ghostext[i].skinid = players[i].skin;
		ghostext[i].kartspeed = players[i].kartspeed;
		ghostext[i].kartweight = players[i].kartweight;
		ghostext[i].charflags = players[i].charflags;

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

void srb2::write_current_demo_standings(const srb2::StandingsJson& standings)
{
	using namespace srb2;
	using json = nlohmann::json;

	// TODO populate standings data

	std::vector<uint8_t> ubjson = json::to_ubjson(standings);
	uint32_t bytes = ubjson.size();

	WRITEUINT8(demobuf.p, DW_STANDING2);

	WRITEUINT32(demobuf.p, bytes);
	WRITEMEM(demobuf.p, ubjson.data(), bytes);
}

void srb2::write_current_demo_end_marker()
{
	WRITEUINT8(demobuf.p, DEMOMARKER); // add the demo end marker
	*(UINT32 *)demoinfo_p = demobuf.p - demobuf.buffer;
}

void G_SetDemoTime(UINT32 ptime, UINT32 plap)
{
	if (!demo.recording || !demotime_p)
		return;
	if (demoflags & ATTACKING_TIME)
	{
		WRITEUINT32(demotime_p, ptime);
	}
	if (demoflags & ATTACKING_LAP)
	{
		WRITEUINT32(demotime_p, plap);
	}
	demotime_p = NULL;
}

// Returns bitfield:
// 1 == new demo has lower time
// 2 == new demo has higher score
// 4 == new demo has higher rings
UINT8 G_CmpDemoTime(char *oldname, char *newname)
{
	UINT8 *buffer,*p;
	UINT16 flags;
	UINT32 oldtime = UINT32_MAX, newtime = UINT32_MAX;
	UINT32 oldlap = UINT32_MAX, newlap = UINT32_MAX;
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
	flags = READUINT16(p); // demoflags
	SKIPSTRING(p); // gametype
	p++; // numlaps
	G_SkipDemoExtraFiles(&p);

	G_SkipDemoSkins(&p, get_buffer_sizes(DEMOVERSION));

	aflags = flags & DF_ATTACKMASK;
	I_Assert(aflags);

	if (aflags & ATTACKING_LAP)
		uselaps = true;

	if (aflags & ATTACKING_TIME)
		newtime = READUINT32(p);
	if (uselaps)
		newlap = READUINT32(p);

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
	case 0x0009: // older staff ghosts
	case 0x000A: // 2.0, 2.1
	case 0x000B: // 2.2 indev (staff ghosts)
	case 0x000C: // 2.2
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
	flags = READUINT16(p);
	SKIPSTRING(p); // gametype
	p++; // numlaps
	G_SkipDemoExtraFiles(&p);
	if (!(flags & aflags))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' not from same game mode. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	}

	G_SkipDemoSkins(&p, get_buffer_sizes(oldversion));

	if (flags & ATTACKING_TIME)
		oldtime = READUINT32(p);
	if (uselaps)
		oldlap = READUINT32(p);

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

static bool load_ubjson_standing(menudemo_t* pdemo, tcb::span<std::byte> slice, tcb::span<democharlist_t> demoskins)
{
	using namespace srb2;
	using json = nlohmann::json;

	StandingsJson js;
	try
	{
		js = json::from_ubjson(slice).template get<StandingsJson>();
	}
	catch (...)
	{
		return false;
	}

	size_t toread = std::min<size_t>(js.standings.size(), MAXPLAYERS);
	for (size_t i = 0; i < toread; i++)
	{
		StandingJson& jsstanding = js.standings[i];
		auto& memstanding = pdemo->standings[i];
		memstanding.ranking = jsstanding.ranking;
		strlcpy(memstanding.name, jsstanding.name.c_str(), sizeof memstanding.name);
		if (jsstanding.demoskin >= demoskins.size())
		{
			memstanding.skin = demoskins[0].mapping;
		}
		else
		{
			memstanding.skin = demoskins[jsstanding.demoskin].mapping;
		}
		memstanding.color = SKINCOLOR_NONE;
		for (size_t j = 0; j < numskincolors; j++)
		{
			skincolor_t& skincolor = skincolors[j];
			if (jsstanding.skincolor == skincolor.name)
			{
				memstanding.color = j;
				break;
			}
		}
		memstanding.timeorscore = jsstanding.timeorscore;
	}

	return true;
}

void G_LoadDemoInfo(menudemo_t *pdemo, boolean allownonmultiplayer)
{
	savebuffer_t info = {0};
	UINT8 *extrainfo_p;
	UINT8 version, subversion, worknumskins, skinid;
	UINT16 pdemoflags;
	democharlist_t *skinlist = NULL;
	UINT16 pdemoversion, count;
	UINT16 legacystandingplayercount;
	char mapname[MAXMAPLUMPNAME],gtname[MAXGAMETYPELENGTH];
	INT32 i;

	if (!P_SaveBufferFromFile(&info, pdemo->filepath))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), pdemo->filepath);
		goto badreplay;
	}

	if (info.size < 12)
	{
		goto corrupt;
	}

	if (memcmp(info.p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_ERROR, M_GetText("%s is not a Ring Racers replay file.\n"), pdemo->filepath);
		goto badreplay;
	}

	pdemo->type = MD_LOADED;

	info.p += 12; // DEMOHEADER

	if (P_SaveBufferRemaining(&info) < 1+1+2)
	{
		goto corrupt;
	}

	version = READUINT8(info.p);
	subversion = READUINT8(info.p);
	pdemoversion = READUINT16(info.p);

	switch(pdemoversion)
	{
	case DEMOVERSION: // latest always supported
	case 0x0009: // older staff ghosts
	case 0x000A: // 2.0, 2.1
	case 0x000B: // 2.2 indev (staff ghosts)
	case 0x000C: // 2.2
		if (P_SaveBufferRemaining(&info) < 64)
		{
			goto corrupt;
		}

		// demo title
		M_Memcpy(pdemo->title, info.p, 64);
		info.p += 64;

		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_ERROR, M_GetText("%s is an incompatible replay format and cannot be played.\n"), pdemo->filepath);
		goto badreplay;
	}

	if (version != VERSION || subversion != SUBVERSION)
		pdemo->type = MD_OUTDATED;

	info.p += 16; // demo checksum

	if (P_SaveBufferRemaining(&info) < 4)
	{
		goto corrupt;
	}

	if (memcmp(info.p, "PLAY", 4))
	{
		CONS_Alert(CONS_ERROR, M_GetText("%s is the wrong type of recording and cannot be played.\n"), pdemo->filepath);
		goto badreplay;
	}
	info.p += 4; // "PLAY"
	READSTRINGN(info.p, mapname, std::min(P_SaveBufferRemaining(&info), sizeof(mapname)));
	pdemo->map = G_MapNumber(mapname);
	info.p += 16; // mapmd5

	if (P_SaveBufferRemaining(&info) < 1)
	{
		goto corrupt;
	}

	pdemoflags = READUINT16(info.p);

	// temp?
	if (!(pdemoflags & DF_MULTIPLAYER) && !allownonmultiplayer)
	{
		CONS_Alert(CONS_ERROR, M_GetText("%s is not a multiplayer replay and can't be listed on this menu fully yet.\n"), pdemo->filepath);
		goto badreplay;
	}

	READSTRINGN(info.p, gtname, std::min(P_SaveBufferRemaining(&info), sizeof(gtname))); // gametype
	pdemo->gametype = G_GetGametypeByName(gtname);

	if (P_SaveBufferRemaining(&info) < 1)
	{
		goto corrupt;
	}

	pdemo->numlaps = READUINT8(info.p);

	pdemo->addonstatus = G_CheckDemoExtraFiles(&info, true);

	skinlist = G_LoadDemoSkins(get_buffer_sizes(pdemoversion), &info, &worknumskins, false);

	if (!skinlist)
	{
		CONS_Alert(CONS_ERROR, M_GetText("%s has an invalid skin list.\n"), pdemo->filepath);
		goto badreplay;
	}

	if ((pdemoflags & DF_ATTACKMASK))
	{
		if ((pdemoflags & ATTACKING_TIME))
		{
			info.p += 4; // time
		}
		if ((pdemoflags & ATTACKING_LAP))
		{
			info.p += 4; // lap
		}
	}

	for (i = 0; i < PRNUMSYNCED; i++)
	{
		info.p += 4; // RNG seed
	}

	if (P_SaveBufferRemaining(&info) < 4+2)
	{
		goto corrupt;
	}

	extrainfo_p = info.buffer + READUINT32(info.p); // The extra UINT32 read is for a blank 4 bytes?

	// Pared down version of CV_LoadNetVars to find the kart speed
	pdemo->kartspeed = KARTSPEED_NORMAL; // Default to normal speed
	count = READUINT16(info.p);
	while (count--)
	{
		UINT16 netid;
		char *svalue;

		if (P_SaveBufferRemaining(&info) < 2)
		{
			goto corrupt;
		}

		netid = READUINT16(info.p);
		svalue = (char *)info.p;
		SKIPSTRINGN(info.p, P_SaveBufferRemaining(&info));
		info.p++; // stealth

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

	if (pdemoflags & DF_GRANDPRIX)
		pdemo->gp = true;

	// Read standings!
	legacystandingplayercount = 0;

	info.p = extrainfo_p;

	while (P_SaveBufferRemaining(&info) > 1)
	{
		UINT8 extrainfotag = READUINT8(info.p);

		switch (extrainfotag)
		{
			case DW_STANDING:
			{
				// This is the only extrainfo tag that is not length prefixed. All others must be.
				constexpr size_t kLegacyStandingSize = 1+16+1+16+4;
				if (P_SaveBufferRemaining(&info) < kLegacyStandingSize)
				{
					goto corrupt;
				}
				if (legacystandingplayercount >= MAXPLAYERS)
				{
					info.p += kLegacyStandingSize;
					break; // switch
				}
				char temp[16+1];

				pdemo->standings[legacystandingplayercount].ranking = READUINT8(info.p);

				// Name
				info.p += copy_fixed_buf(pdemo->standings[legacystandingplayercount].name, info.p, 16);

				// Skin
				skinid = READUINT8(info.p);
				if (skinid > worknumskins)
					skinid = 0;
				pdemo->standings[legacystandingplayercount].skin = skinlist[skinid].mapping;

				// Color
				info.p += copy_fixed_buf(temp, info.p, 16);
				for (i = 0; i < numskincolors; i++)
					if (!stricmp(skincolors[i].name,temp))				// SRB2kart
					{
						pdemo->standings[legacystandingplayercount].color = i;
						break;
					}

				// Score/time/whatever
				pdemo->standings[legacystandingplayercount].timeorscore = READUINT32(info.p);

				legacystandingplayercount++;
				break;
			}
			case DW_STANDING2:
			{
				if (P_SaveBufferRemaining(&info) < 4)
				{
					goto corrupt;
				}
				UINT32 size = READUINT32(info.p);
				if (P_SaveBufferRemaining(&info) < size)
				{
					goto corrupt;
				}
				tcb::span<std::byte> slice = tcb::as_writable_bytes(tcb::span(info.p, size));
				tcb::span<democharlist_t> demoskins {skinlist, worknumskins};
				info.p += size;
				if (!load_ubjson_standing(pdemo, slice, demoskins))
				{
					goto corrupt;
				}
				break;
			}
			default:
			{
				// Gracefully ignore other extrainfo tags by skipping their data
				if (P_SaveBufferRemaining(&info) < 4)
				{
					goto corrupt;
				}
				UINT32 size = READUINT32(info.p);
				if (P_SaveBufferRemaining(&info) < size)
				{
					goto corrupt;
				}
				info.p += size;
				break;
			}
		}
	}

	if (P_SaveBufferRemaining(&info) == 0)
	{
		goto corrupt;
	}

	// I think that's everything we need?
	Z_Free(skinlist);
	P_SaveBufferFree(&info);
	return;

corrupt:
	CONS_Alert(CONS_ERROR, "%s is corrupt.\n", pdemo->filepath);

badreplay:
	pdemo->type = MD_INVALID;
	sprintf(pdemo->title, "INVALID REPLAY");
	Z_Free(skinlist);
	P_SaveBufferFree(&info);
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

void G_DoPlayDemoEx(const char *defdemoname, lumpnum_t deflumpnum)
{
	INT32 i;
	UINT8 p, numslots = 0;
	lumpnum_t l;
	char color[MAXCOLORNAME+1],follower[SKINNAMESIZE+1],mapname[MAXMAPLUMPNAME],gtname[MAXGAMETYPELENGTH];
	char *pdemoname;
	UINT8 availabilities[MAXPLAYERS][MAXAVAILABILITY];
	UINT8 version,subversion;
	UINT32 randseed[PRNUMSYNCED];
	char msg[1024];

	boolean spectator, bot;
	UINT8 slots[MAXPLAYERS], lastfakeskin[MAXPLAYERS];

#if defined(SKIPERRORS) && !defined(DEVELOP)
	// RR: Don't print warnings for staff ghosts, since they'll inevitably
	// happen when we make bugfixes/changes...
	// Unlike usual, true by default since most codepaths in this func are internal
	// lumps (or for restarted external files, shouldn't re-print existing errors)
	boolean skiperrors = true;
#endif

	G_InitDemoRewind();

	gtname[MAXGAMETYPELENGTH-1] = '\0';

	if (deflumpnum != LUMPERROR)
	{
		// Load demo resource from explicitly provided lump ID.
		// (we do NOT care for defdemoname here)

		P_SaveBufferFromLump(&demobuf, deflumpnum);
		pdemoname = Z_StrDup(wadfiles[WADFILENUM(deflumpnum)]->lumpinfo[LUMPNUM(deflumpnum)].fullname);
	}
	else if (defdemoname == NULL)
	{
		// No demo name means we're restarting the current demo!

		demobuf.p = demobuf.buffer;
		pdemoname = static_cast<char*>(ZZ_Alloc(1)); // Easier than adding checks for this everywhere it's freed
	}
	else
	{
		// We have to guess by the string provided... :face_holding_back_tears:

		// FIXME: this file doesn't manage its memory and actually free this when it's done using it
		//Z_Free(demobuf.buffer);
		demobuf.buffer = NULL;

		// This weird construct turns `./media/replay/online/2.0/lmao.lmp` into `lmao.lmp`
		// I'd wrap it in braces, but the VRES_GHOST feature needs access to n
		const char *n = defdemoname+strlen(defdemoname);
		while (*n != '/' && *n != '\\' && n != defdemoname)
			n--;
		if (n != defdemoname)
			n++;
		pdemoname = static_cast<char*>(ZZ_Alloc(strlen(n)+1));
		strcpy(pdemoname,n);

		M_SetPlaybackMenuPointer();

		// Internal if no extension, external if one exists
		if (FIL_CheckExtension(defdemoname))
		{
			//FIL_DefaultExtension(defdemoname, ".lmp");
			if (P_SaveBufferFromFile(&demobuf, defdemoname) == false)
			{
				snprintf(msg, 1024, M_GetText("Failed to read file '%s'.\n"), defdemoname);
				CONS_Alert(CONS_ERROR, "%s", msg);
				Z_Free(pdemoname);
				gameaction = ga_nothing;
				M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
				return;
			}

#if defined(SKIPERRORS) && !defined(DEVELOP)
			skiperrors = false; // DO print warnings for external lumps
#endif
		}
		else
		{
			// load demo resource from WAD by name
#ifdef VRES_GHOST
			if (n != defdemoname)
			{
				// vres GHOST_%u
				virtres_t *vRes;
				virtlump_t *vLump;
				UINT16 mapnum;
				size_t step = 0;

				step = 0;
				while (defdemoname+step < n-1)
				{
					mapname[step] = defdemoname[step];
					step++;
				}
				mapname[step] = '\0';

				mapnum = G_MapNumber(mapname);
				if (mapnum >= nummapheaders || mapheaderinfo[mapnum]->lumpnum == LUMPERROR)
				{
					snprintf(msg, 1024, M_GetText("Failed to read virtlump '%s (couldn't find map %s)'.\n"), defdemoname, mapname);
					CONS_Alert(CONS_ERROR, "%s", msg);
					Z_Free(pdemoname);
					gameaction = ga_nothing;
					M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
					return;
				}

				vRes = vres_GetMap(mapheaderinfo[mapnum]->lumpnum);
				vLump = vres_Find(vRes, pdemoname);

				if (vLump == NULL)
				{
					snprintf(msg, 1024, M_GetText("Failed to read virtlump '%s (couldn't find lump %s in %s)'.\n"), defdemoname, pdemoname, mapname);
					CONS_Alert(CONS_ERROR, "%s", msg);
					Z_Free(pdemoname);
					gameaction = ga_nothing;
					M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
					return;
				}

				// FIXME: this file doesn't manage its memory and actually free this when it's done using it
				Z_Free(demobuf.buffer);
				P_SaveBufferAlloc(&demobuf, vLump->size);
				Z_SetUser(demobuf.buffer, (void**)&demobuf.buffer);
				memcpy(demobuf.buffer, vLump->data, vLump->size);

				vres_Free(vRes);
			}
			else
#endif // VRES_GHOST
			{
				// Raw lump.
				if ((l = W_CheckNumForLongName(defdemoname)) == LUMPERROR)
				{
					snprintf(msg, 1024, M_GetText("Failed to read lump '%s'.\n"), defdemoname);
					CONS_Alert(CONS_ERROR, "%s", msg);
					Z_Free(pdemoname);
					gameaction = ga_nothing;
					M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
					return;
				}

				P_SaveBufferFromLump(&demobuf, l);
			}
		}
	}

	// read demo header
	gameaction = ga_nothing;
	demo.playback = true;
	demo.buffer = &demobuf;
	if (memcmp(demobuf.p, DEMOHEADER, 12))
	{
		snprintf(msg, 1024, M_GetText("%s is not a Ring Racers replay file.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
		Z_Free(pdemoname);
		Z_Free(demobuf.buffer);
		demo.playback = false;
		return;
	}
	demobuf.p += 12; // DEMOHEADER

	version = READUINT8(demobuf.p);
	subversion = READUINT8(demobuf.p);
	demo.version = READUINT16(demobuf.p);
	switch(demo.version)
	{
	case DEMOVERSION: // latest always supported
	case 0x0009: // older staff ghosts
	case 0x000A: // 2.0, 2.1
	case 0x000B: // 2.2 indev (staff ghosts)
	case 0x000C: // 2.2
		break;
	// too old, cannot support.
	default:
		snprintf(msg, 1024, M_GetText("%s is an incompatible replay format and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
		Z_Free(pdemoname);
		Z_Free(demobuf.buffer);
		demo.playback = false;
		return;
	}

	g_buffer_sizes = get_buffer_sizes(demo.version);

	// demo title
	M_Memcpy(demo.titlename, demobuf.p, 64);
	demobuf.p += 64;

	demobuf.p += 16; // demo checksum

	if (memcmp(demobuf.p, "PLAY", 4))
	{
		snprintf(msg, 1024, M_GetText("%s is the wrong type of recording and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
		Z_Free(pdemoname);
		Z_Free(demobuf.buffer);
		demo.playback = false;
		return;
	}
	demobuf.p += 4; // "PLAY"
	READSTRINGN(demobuf.p, mapname, sizeof(mapname)); // gamemap
	demobuf.p += 16; // mapmd5

	demoflags = READUINT16(demobuf.p);

	READSTRINGN(demobuf.p, gtname, sizeof(gtname)); // gametype

	numlaps = READUINT8(demobuf.p);

	if (demo.attract) // Attract demos should always play and ought to always be compatible with whatever wadlist is running.
		G_SkipDemoExtraFiles(&demobuf.p);
	else if (demo.loadfiles)
		G_LoadDemoExtraFiles(&demobuf.p);
	else if (demo.ignorefiles)
		G_SkipDemoExtraFiles(&demobuf.p);
	else
	{
		UINT8 error = G_CheckDemoExtraFiles(&demobuf, false);

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
				M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
			Z_Free(pdemoname);
			Z_Free(demobuf.buffer);
			demo.playback = false;
			return;
		}
	}

	gamemap = G_MapNumber(mapname)+1;

	i = G_GetGametypeByName(gtname);
	if (i < 0)
	{
		snprintf(msg, 1024, M_GetText("%s is in a gametype that is not currently loaded and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
		Z_Free(pdemoname);
		Z_Free(demobuf.buffer);
		demo.playback = false;
		return;
	}
	G_SetGametype(i);

	// character list
	demo.skinlist = G_LoadDemoSkins(g_buffer_sizes, &demobuf, &demo.numskins, true);
	if (!demo.skinlist)
	{
		snprintf(msg, 1024, M_GetText("%s has an invalid skin list and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
		Z_Free(pdemoname);
		Z_Free(demobuf.buffer);
		demo.playback = false;
		return;
	}

	modeattacking = (demoflags & DF_ATTACKMASK);
	multiplayer = !!(demoflags & DF_MULTIPLAYER);
	demo.netgame = (multiplayer && !(demoflags & DF_NONETMP));
	CON_ToggleOff();

	hu_demotime = UINT32_MAX;
	hu_demolap = UINT32_MAX;

	if (modeattacking & ATTACKING_TIME)
		hu_demotime = READUINT32(demobuf.p);
	if (modeattacking & ATTACKING_LAP)
		hu_demolap = READUINT32(demobuf.p);

	// Random seed
	for (i = 0; i < PRNUMSYNCED; i++)
	{
		randseed[i] = READUINT32(demobuf.p);
	}

	demobuf.p += 4; // Extrainfo location

	// ...*map* not loaded?
	if (!gamemap || (gamemap > nummapheaders) || !mapheaderinfo[gamemap-1] || mapheaderinfo[gamemap-1]->lumpnum == LUMPERROR)
	{
		snprintf(msg, 1024, M_GetText("%s features a course that is not currently loaded.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
		Z_Free(demo.skinlist);
		demo.skinlist = NULL;
		Z_Free(pdemoname);
		Z_Free(demobuf.buffer);
		demo.playback = false;
		return;
	}

	// net var data
	demobuf.p += CV_LoadDemoVars(demobuf.p);

	memset(&grandprixinfo, 0, sizeof grandprixinfo);
	if ((demoflags & DF_GRANDPRIX))
	{
		grandprixinfo.gp = true;
		grandprixinfo.gamespeed = READUINT8(demobuf.p);
		grandprixinfo.masterbots = READUINT8(demobuf.p) != 0;
		grandprixinfo.eventmode = static_cast<gpEvent_e>(READUINT8(demobuf.p));
		if (demo.version >= 0x000D)
		{
			grandprixinfo.specialDamage = READUINT32(demobuf.p);
		}
	}

	// Load unlocks into netUnlocked
	{
		UINT32 unlockables = READUINT32(demobuf.p);
		UINT32 unlocksread = std::min<UINT32>(unlockables, MAXUNLOCKABLES);
		for (size_t i = 0; i < unlocksread; i++)
		{
			netUnlocked[i] = static_cast<boolean>(READUINT8(demobuf.p));
		}
		// skip remainder
		demobuf.p += unlockables - unlocksread;
	}

	// Load "mapmusrng" used for altmusic selection
	mapmusrng = READUINT8(demobuf.p);

	// Sigh ... it's an empty demo.
	if (*demobuf.p == DEMOMARKER)
	{
		snprintf(msg, 1024, M_GetText("%s contains no data to be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
		Z_Free(demo.skinlist);
		demo.skinlist = NULL;
		Z_Free(pdemoname);
		Z_Free(demobuf.buffer);
		demo.playback = false;
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

	consoleplayer = 0;
	memset(playeringame,0,sizeof(playeringame));
	memset(displayplayers,0,sizeof(displayplayers));
	memset(camera,0,sizeof(camera)); // reset freecam

	// Load players that were in-game when the map started
	p = READUINT8(demobuf.p);

	while (p != 0xFF)
	{
		UINT8 flags = READUINT8(demobuf.p);

		spectator = !!(flags & DEMO_SPECTATOR);
		bot = !!(flags & DEMO_BOT);

		if ((spectator || bot))
		{
			if (modeattacking)
			{
				snprintf(msg, 1024, M_GetText("%s is a Record Attack replay with %s, and is thus invalid.\n"), pdemoname, (bot ? "bots" : "spectators"));
				CONS_Alert(CONS_ERROR, "%s", msg);
				M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
				Z_Free(demo.skinlist);
				demo.skinlist = NULL;
				Z_Free(pdemoname);
				Z_Free(demobuf.buffer);
				demo.playback = false;
				return;
			}
		}

		slots[numslots] = p;
		numslots++;

		if (modeattacking && numslots > 1)
		{
			snprintf(msg, 1024, M_GetText("%s is a Record Attack replay with multiple players, and is thus invalid.\n"), pdemoname);
			CONS_Alert(CONS_ERROR, "%s", msg);
			M_StartMessage("Demo Playback", msg, NULL, MM_NOTHING, NULL, "Return to Menu");
			Z_Free(demo.skinlist);
			demo.skinlist = NULL;
			Z_Free(pdemoname);
			Z_Free(demobuf.buffer);
			demo.playback = false;
			return;
		}

		if (!playeringame[displayplayers[0]] || players[displayplayers[0]].spectator)
			displayplayers[0] = consoleplayer = serverplayer = p;

		G_AddPlayer(p, p);
		players[p].spectator = spectator;

		if (flags & DEMO_KICKSTART)
			players[p].pflags |= PF_KICKSTARTACCEL;
		else
			players[p].pflags &= ~PF_KICKSTARTACCEL;

		if (flags & DEMO_AUTOROULETTE)
			players[p].pflags |= PF_AUTOROULETTE;
		else
			players[p].pflags &= ~PF_AUTOROULETTE;

		if (flags & DEMO_AUTORING)
			players[p].pflags |= PF_AUTORING;
		else
			players[p].pflags &= ~PF_AUTORING;

		if (flags & DEMO_SHRINKME)
			players[p].pflags |= PF_SHRINKME;
		else
			players[p].pflags &= ~PF_SHRINKME;

		if ((players[p].bot = bot) == true)
		{
			players[p].botvars.difficulty = READUINT8(demobuf.p);
			players[p].botvars.diffincrease = READUINT8(demobuf.p); // needed to avoid having to duplicate logic
			players[p].botvars.rival = (boolean)READUINT8(demobuf.p);
		}

		K_UpdateShrinkCheat(&players[p]);

		// Name
		demobuf.p += copy_fixed_buf(player_names[p], demobuf.p, g_buffer_sizes.player_name);

		for (i = 0; i < MAXAVAILABILITY; i++)
		{
			availabilities[p][i] = READUINT8(demobuf.p);
		}

		// Skin

		demo.currentskinid[p] = READUINT8(demobuf.p);
		if (demo.currentskinid[p] >= demo.numskins)
			demo.currentskinid[p] = 0;
		lastfakeskin[p] = READUINT8(demobuf.p);

		// Color
		demobuf.p += copy_fixed_buf(color, demobuf.p, g_buffer_sizes.color_name);
		for (i = 0; i < numskincolors; i++)
			if (!stricmp(skincolors[i].name,color))				// SRB2kart
			{
				players[p].skincolor = i;
				break;
			}

		// Follower
		demobuf.p += copy_fixed_buf(follower, demobuf.p, g_buffer_sizes.skin_name);
		K_SetFollowerByName(p, follower);

		// Follower colour
		demobuf.p += copy_fixed_buf(color, demobuf.p, g_buffer_sizes.color_name);
		for (i = 0; i < numskincolors +2; i++)	// +2 because of Match and Opposite
		{
			if (!stricmp(Followercolor_cons_t[i].strvalue, color))
			{
				players[p].followercolor = Followercolor_cons_t[i].value;
				break;
			}
		}

		// Score, since Kart uses this to determine where you start on the map
		players[p].score = READUINT32(demobuf.p);

		// Power Levels
		clientpowerlevels[p][gametype == GT_BATTLE ? PWRLV_BATTLE : PWRLV_RACE] = READUINT16(demobuf.p);

		// Followitem
		players[p].followitem = static_cast<mobjtype_t>(READUINT32(demobuf.p));

		// GP
		players[p].lives = READSINT8(demobuf.p);
		players[p].totalring = READINT16(demobuf.p);

		// Look for the next player
		p = READUINT8(demobuf.p);
	}

	// end of player read (the 0xFF marker)
	// so this is where we are to read our lua variables (if possible!)
	if (demoflags & DF_LUAVARS)	// again, used for compability, lua shit will be saved to replays regardless of if it's even been loaded
	{
		if (!gL) // No Lua state! ...I guess we'll just start one...
			LUA_ClearState();

		// No modeattacking check, DF_LUAVARS won't be present here.
		LUA_UnArchive(&demobuf, false);
	}

	splitscreen = 0;

	if (demo.attract == DEMO_ATTRACT_TITLE)
	{
		splitscreen = M_RandomKey(6)-1;
		splitscreen = std::min<int>(std::min(3, numslots-1), splitscreen); // Bias toward 1p and 4p views

		for (p = 0; p <= splitscreen; p++)
			G_ResetView(p+1, slots[M_RandomKey(numslots)], false);
	}

	R_ExecuteSetViewSize();

	for (i = 0; i < PRNUMSYNCED; i++)
	{
		P_SetRandSeed(static_cast<pr_class_t>(i), randseed[i]);
	}

	G_InitNew((demoflags & DF_ENCORE) != 0, gamemap, true, true); // Doesn't matter whether you reset or not here, given changes to resetplayer.

	for (i = 0; i < numslots; i++)
	{
		UINT8 j;

		p = slots[i];

		for (j = 0; j < MAXAVAILABILITY; j++)
		{
			players[p].availabilities[j] = availabilities[p][j];
		}

		ghostext[p].skinid = demo.currentskinid[p];
		SetPlayerSkinByNum(p, demo.currentskinid[p]);

		if (players[p].mo)
		{
			players[p].mo->color = players[p].skincolor;
			oldghost[p].x = players[p].mo->x;
			oldghost[p].y = players[p].mo->y;
			oldghost[p].z = players[p].mo->z;
		}

		// Set saved attribute values
		// No cheat checking here, because even if they ARE wrong...
		// it would only break the replay if we clipped them.
		players[p].kartspeed = ghostext[p].kartspeed = demo.skinlist[demo.currentskinid[p]].kartspeed;
		players[p].kartweight = ghostext[p].kartweight = demo.skinlist[demo.currentskinid[p]].kartweight;
		players[p].charflags = ghostext[p].charflags = demo.skinlist[demo.currentskinid[p]].flags;
		players[p].lastfakeskin = lastfakeskin[p];
	}

	demo.deferstart = true;

	CV_StealthSetValue(&cv_playbackspeed, 1);
}

void G_AddGhost(savebuffer_t *buffer, const char *defdemoname)
{
	INT32 i;
	char name[MAXPLAYERNAME+1], color[MAXCOLORNAME+1], md5[16];
	demoghost *gh;
	UINT16 flags;
	UINT8 *p;
	mapthing_t *mthing;
	UINT16 count, ghostversion;
	skin_t *ghskin = &skins[0];
	UINT8 worknumskins;
	democharlist_t *skinlist = NULL;

	p = buffer->buffer;

	// read demo header
	if (memcmp(p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Not a SRB2 replay.\n"), defdemoname);
		P_SaveBufferFree(buffer);
		return;
	} p += 12; // DEMOHEADER

	p++; // VERSION
	p++; // SUBVERSION

	ghostversion = READUINT16(p);
	switch(ghostversion)
	{
	case DEMOVERSION: // latest always supported
	case 0x0009: // older staff ghosts
	case 0x000A: // 2.0, 2.1
	case 0x000B: // 2.2 indev (staff ghosts)
	case 0x000C: // 2.2
		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Demo version incompatible.\n"), defdemoname);
		P_SaveBufferFree(buffer);
		return;
	}

	const DemoBufferSizes ghostsizes = get_buffer_sizes(ghostversion);

	p += 64; // title
	M_Memcpy(md5, p, 16); p += 16; // demo checksum

	for (gh = ghosts; gh; gh = gh->next)
		if (!memcmp(md5, gh->checksum, 16)) // another ghost in the game already has this checksum?
		{ // Don't add another one, then!
			CONS_Debug(DBG_SETUP, "Rejecting duplicate ghost %s (MD5 was matched)\n", defdemoname);
			P_SaveBufferFree(buffer);
			return;
		}

	if (memcmp(p, "PLAY", 4))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Demo format unacceptable.\n"), defdemoname);
		P_SaveBufferFree(buffer);
		return;
	} p += 4; // "PLAY"


	SKIPSTRING(p); // gamemap
	p += 16; // mapmd5 (possibly check for consistency?)

	flags = READUINT16(p);
	if (!(flags & DF_GHOST))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: No ghost data in this demo.\n"), defdemoname);
		P_SaveBufferFree(buffer);
		return;
	}

	if (flags & DF_LUAVARS) // can't be arsed to add support for grinding away ported lua material
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Replay data contains luavars, cannot continue.\n"), defdemoname);
		P_SaveBufferFree(buffer);
		return;
	}

	SKIPSTRING(p); // gametype
	p++; // numlaps
	G_SkipDemoExtraFiles(&p); // Don't wanna modify the file list for ghosts.

	{
		// FIXME: the rest of this function is not modifying buffer->p directly so fuck it
		buffer->p = p;
		skinlist = G_LoadDemoSkins(ghostsizes, buffer, &worknumskins, true);
		p = buffer->p;
	}
	if (!skinlist)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Replay data has invalid skin list, cannot continue.\n"), defdemoname);
		P_SaveBufferFree(buffer);
		return;
	}

	if (flags & ATTACKING_TIME)
		p += 4;
	if (flags & ATTACKING_LAP)
		p += 4;

	for (i = 0; i < PRNUMSYNCED; i++)
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

	if ((flags & DF_GRANDPRIX))
	{
		p += 3;
		if (ghostversion >= 0x000D)
			p++;
	}

	// Skip unlockables
	{
		UINT32 unlockables = READUINT32(p);
		p += unlockables;
	}

	p++; // mapmusrng

	if (*p == DEMOMARKER)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Failed to add ghost %s: Replay is empty.\n"), defdemoname);
		Z_Free(skinlist);
		P_SaveBufferFree(buffer);
		return;
	}

	p++; // player number - doesn't really need to be checked, TODO maybe support adding multiple players' ghosts at once

	// any invalidating flags?
	i = READUINT8(p);
	if ((i & (DEMO_SPECTATOR|DEMO_BOT)) != 0)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Failed to add ghost %s: Invalid player slot (spectator/bot)\n"), defdemoname);
		Z_Free(skinlist);
		P_SaveBufferFree(buffer);
		return;
	}

	// Player name (TODO: Display this somehow if it doesn't match cv_playername!)
	p += copy_fixed_buf(name, p, ghostsizes.player_name);

	p += MAXAVAILABILITY;

	// Skin
	i = READUINT8(p);
	if (i < worknumskins)
		ghskin = &skins[skinlist[i].mapping];
	p++; // lastfakeskin

	// Color
	p += copy_fixed_buf(color, p, ghostsizes.color_name);

	// Follower data was here, skip it, we don't care about it for ghosts.
	p += ghostsizes.skin_name + ghostsizes.color_name;

	p += 4; // score
	p += 2; // powerlevel

	p += 4; // followitem (maybe change later)

	p += 1; // lives
	p += 2; // rings

	if (READUINT8(p) != 0xFF)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Failed to add ghost %s: Invalid player slot (bad terminator)\n"), defdemoname);
		Z_Free(skinlist);
		P_SaveBufferFree(buffer);
		return;
	}


	gh = static_cast<demoghost*>(Z_Calloc(sizeof(demoghost), PU_LEVEL, NULL));
	gh->sizes = ghostsizes;
	gh->next = ghosts;
	gh->buffer = buffer->buffer;
	M_Memcpy(gh->checksum, md5, 16);
	gh->p = p;

	gh->numskins = worknumskins;
	gh->skinlist = skinlist;

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
		if (!!(mthing->thing_args[0]) ^ !!(mthing->options & MTF_OBJECTFLIP))
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

	CONS_Printf(M_GetText("Added ghost %s from %s\n"), name, defdemoname);
}

// Clean up all ghosts
void G_FreeGhosts(void)
{
	while (ghosts)
	{
		demoghost *next = ghosts->next;
		Z_Free(ghosts->skinlist);
		Z_Free(ghosts);
		ghosts = next;
	}
	ghosts = NULL;
}

// A simplified version of G_AddGhost...
staffbrief_t *G_GetStaffGhostBrief(UINT8 *buffer)
{
	UINT8 *p = buffer;
	UINT16 ghostversion;
	UINT16 flags;
	INT32 i;
	staffbrief_t temp = {0};
	staffbrief_t *ret = NULL;

	temp.name[0] = '\0';
	temp.time = temp.lap = UINT32_MAX;

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
		case 0x0009: // older staff ghosts
		case 0x000A: // 2.0, 2.1
		case 0x000B: // 2.2 indev (staff ghosts)
		case 0x000C: // 2.2
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

	flags = READUINT16(p);
	if (!(flags & DF_GHOST))
	{
		goto fail; // we don't NEED to do it here, but whatever
	}

	SKIPSTRING(p); // gametype
	p++; // numlaps
	G_SkipDemoExtraFiles(&p);

	G_SkipDemoSkins(&p, get_buffer_sizes(ghostversion));

	if (flags & ATTACKING_TIME)
		temp.time = READUINT32(p);
	if (flags & ATTACKING_LAP)
		temp.lap = READUINT32(p);

	for (i = 0; i < PRNUMSYNCED; i++)
	{
		p += 4; // random seed
	}

	p += 4; // Extrainfo location marker

	// Ehhhh don't need ghostversion here (?) so I'll reuse the var here
	ghostversion = READUINT16(p);
	while (ghostversion--)
	{
		SKIPSTRING(p);
		SKIPSTRING(p);
		p++; // stealth
	}

	if ((flags & DF_GRANDPRIX))
	{
		p += 3;
		if (ghostversion >= 0x000D)
			p++;
	}

	// Skip unlockables
	{
		UINT32 unlockables = READUINT32(p);
		p += unlockables;
	}

	p++; // mapmusrng

	// Assert first player is in and then read name
	if (READUINT8(p) != 0)
		goto fail;
	if (READUINT8(p) & (DEMO_SPECTATOR|DEMO_BOT))
		goto fail;

	copy_fixed_buf(temp.name, p, get_buffer_sizes(ghostversion).player_name);

	ret = static_cast<staffbrief_t*>(Z_Malloc(sizeof(staffbrief_t), PU_STATIC, NULL));
	if (ret)
		M_Memcpy(ret, &temp, sizeof(staffbrief_t));

	// Ok, no longer any reason to care, bye
fail:
	return ret;
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
	g_singletics = true;
	framecount = 0;
	demostarttime = I_GetTime();
	G_DeferedPlayDemo(name);
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

#if 0 // since it's not actually used anymore, just a reference...
// Writes the demo's checksum, or just random garbage if you can't do that for some reason.
static void WriteDemoChecksum(void)
{
	UINT8 *p = demobuf.buffer+16; // checksum position
#ifdef NOMD5
	UINT8 i;
	for (i = 0; i < 16; i++, p++)
		*p = P_RandomByte(PR_UNDEFINED); // This MD5 was chosen by fair dice roll and most likely < 50% correct.
#else
	md5_buffer((char *)p+16, demobuf.p - (p+16), p); // make a checksum of everything after the checksum in the file.
#endif
}
#endif

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
		const char *csvpath = va("%s" PATHSEP "%s", srb2home, "timedemo.csv");
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

	if (timedemo_quit)
		COM_ImmedExecute("quit");
	else
		D_StartTitle();
}

// reset engine variable set for the demos
// called from stopdemo command, map command, and g_checkdemoStatus.
void G_StopDemo(void)
{
	Z_Free(demobuf.buffer);
	demobuf.buffer = NULL;
	demo.playback = false;
	demo.timing = false;
	demo.waitingfortally = false;
	g_singletics = false;

	{
		UINT8 i;
		for (i = 0; i < MAXSPLITSCREENPLAYERS; ++i)
		{
			camera[i].freecam = false;
		}
	}

	Z_Free(demo.skinlist);
	demo.skinlist = NULL;

	D_ClearState();
}

boolean G_CheckDemoStatus(void)
{
	G_FreeGhosts();

	if (demo.timing)
	{
		G_StopTimingDemo();
		return true;
	}

	if (demo.playback)
	{
		if (demo.quitafterplaying)
			I_Quit();

		// When this replay was recorded, the player skipped
		// the Tally and ended the demo early.
		// Keep the demo open and don't boot to intermission
		// YET, pause demo playback.
		if (!demo.waitingfortally && modeattacking && exitcountdown)
			demo.waitingfortally = true;
		else if (!demo.attract)
			G_FinishExitLevel();
		else
		{
			UINT8 wasmodeattacking = modeattacking;
			G_StopDemo();

			if (timedemo_quit)
				COM_ImmedExecute("quit");
			else if (wasmodeattacking)
				M_EndModeAttackRun();
			else if (demo.attract == DEMO_ATTRACT_CREDITS)
				F_DeferContinueCredits();
			else
				D_SetDeferredStartTitle(true);
		}

		return true;
	}

	if (!demo.recording)
		return false;

	if (modeattacking || demo.willsave)
	{
		if (demobuf.p)
		{
			G_SaveDemo();
		}
		else
		{
			G_ResetDemoRecording();
		}
		return true;
	}

	Z_Free(demobuf.buffer);

	demo.recording = false;
	demo.waitingfortally = false;

	return false;
}

void G_ResetDemoRecording(void)
{
	Z_Free(demobuf.buffer);
	demo.recording = false;
}

void G_SaveDemo(void)
{
	UINT8 *p = demobuf.buffer+16; // after version
	UINT32 length;
#ifdef NOMD5
	UINT8 i;
#endif

	if (currentMenu == &TitleEntryDef)
		M_ClearMenus(true);

	// Ensure extrainfo pointer is always available, even if no info is present.
	if (demoinfo_p && *(UINT32 *)demoinfo_p == 0)
	{
		WRITEUINT8(demobuf.p, DEMOMARKER); // add the demo end marker
		*(UINT32 *)demoinfo_p = demobuf.p - demobuf.buffer;
	}
	WRITEUINT8(demobuf.p, DW_END); // Mark end of demo extra data.

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
			else if (strindex && !dash)
			{
				demo_slug[strindex] = '-';
				strindex++;
				dash = true;
			}
		}

		if (dash && strindex)
		{
			strindex--;
		}
		demo_slug[strindex] = '\0';

		if (demo_slug[0] != '\0')
		{
			// Slug is valid, write the chosen filename.
			writepoint = strstr(strrchr(demoname, *PATHSEP), "-") + 1;
			demo_slug[128 - (writepoint - demoname) - 4] = 0;
			sprintf(writepoint, "%s.lmp", demo_slug);
		}
		else if (demo.titlename[0] == '\0')
		{
			// Slug is completely blank? Will crash if we attempt to save
			// No bailout because empty seems like a good "no thanks" choice
			G_ResetDemoRecording();
			return;
		}
		// If a title that is invalid is provided, the user clearly wanted
		// to save. But we can't do so at that name, so we only apply the
		// title INSIDE the file, not in the naked filesystem.
		// (A hypothetical example is bamboozling bot behaviour causing
		// a player to write "?????????".) ~toast 010524
	}

	length = *(UINT32 *)demoinfo_p;
	WRITEUINT32(demoinfo_p, length);

	// Doesn't seem like I can use WriteDemoChecksum here, correct me if I'm wrong -Sal
#ifdef NOMD5
	for (i = 0; i < 16; i++, p++)
		*p = M_RandomByte(); // This MD5 was chosen by fair dice roll and most likely < 50% correct.
#else
	// Make a checksum of everything after the checksum in the file up to the end of the standard data. Extrainfo is freely modifiable.
	md5_buffer((char *)p+16, (demobuf.buffer + length) - (p+16), p);
#endif

	bool saved = FIL_WriteFile(demoname, demobuf.buffer, demobuf.p - demobuf.buffer); // finally output the file.
	G_ResetDemoRecording();

	if (!modeattacking)
	{
		if (saved)
		{
			CONS_Printf(M_GetText("Demo %s recorded\n"), demoname);
			if (gamedata->eversavedreplay == false)
			{
				gamedata->eversavedreplay = true;
				// The following will IMMEDIATELY happen on either next level load
				// or returning to menu, so don't make the sound just to get cut off
				//M_UpdateUnlockablesAndExtraEmblems(true, true);
				//G_SaveGameData();
				gamedata->deferredsave = true;
			}
		}
		else
			CONS_Alert(CONS_WARNING, M_GetText("Demo %s not saved\n"), demoname);
	}
}

boolean G_CheckDemoTitleEntry(void)
{
	if (menuactive || chat_on)
		return false;

	if (!G_PlayerInputDown(0, gc_b, 0) && !G_PlayerInputDown(0, gc_x, 0))
		return false;

	demo.willsave = true;
	M_OpenVirtualKeyboard(
		sizeof demo.titlename,
		[](const char* replace) -> const char*
		{
			if (replace)
				strlcpy(demo.titlename, replace, sizeof demo.titlename);
			return demo.titlename;
		},
		&TitleEntryDef
	);

	return true;
}

void G_SyncDemoParty(INT32 rem, INT32 newsplitscreen)
{
	int r_splitscreen_copy = r_splitscreen;
	INT32 displayplayers_copy[MAXSPLITSCREENPLAYERS];
	memcpy(displayplayers_copy, displayplayers, sizeof displayplayers);

	// If we switch away from someone's view, that player
	// should be removed from the party.
	// However, it is valid to have the player on multiple
	// viewports.

	// Remove this player
	G_LeaveParty(rem);

	// And reset the rest of the party
	for (int i = 0; i <= r_splitscreen_copy; ++i)
		G_LeaveParty(displayplayers_copy[i]);

	// Restore the party, without the removed player, and
	// with the order matching displayplayers
	for (int i = 0; i <= newsplitscreen; ++i)
		G_JoinParty(consoleplayer, displayplayers_copy[i]);

	// memcpy displayplayers back to preserve duplicates
	// (G_JoinParty will not create duplicates itself)
	r_splitscreen = newsplitscreen;
	memcpy(displayplayers, displayplayers_copy, sizeof displayplayers);
	R_ExecuteSetViewSize();
}
