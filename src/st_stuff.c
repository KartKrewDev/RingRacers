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
/// \file  st_stuff.c
/// \brief Status bar code
///        Does the face/direction indicator animatin.
///        Does palette indicators as well (red pain/berserk, bright pickup)

#include "doomdef.h"
#include "g_game.h"
#include "r_local.h"
#include "p_local.h"
#include "f_finale.h"
#include "st_stuff.h"
#include "i_video.h"
#include "v_video.h"
#include "z_zone.h"
#include "hu_stuff.h"
#include "console.h"
#include "s_sound.h"
#include "i_system.h"
#include "k_menu.h"
#include "m_cheat.h"
#include "m_misc.h" // moviemode
#include "m_anigif.h" // cv_gif_downscale
#include "p_setup.h" // NiGHTS grading
#include "r_fps.h"
#include "m_random.h" // random index
#include "k_director.h" // K_DirectorIsEnabled

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "lua_hudlib_drawlist.h"
#include "lua_hud.h"
#include "lua_hook.h"

// SRB2Kart
#include "k_hud.h" // SRB2kart
#include "v_video.h"
#include "r_skins.h" // NUMFACES
#include "k_grandprix.h" // we need to know grandprix status for titlecards
#include "k_boss.h"
#include "k_zvote.h"
#include "music.h"
#include "i_sound.h"
#include "k_dialogue.h"
#include "m_easing.h"

UINT16 objectsdrawn = 0;

//
// STATUS BAR DATA
//

patch_t *faceprefix[MAXSKINS][NUMFACES];

// ------------------------------------------
//             status bar overlay
// ------------------------------------------

// Midnight Channel:
static patch_t *hud_tv1;
static patch_t *hud_tv2;

#ifdef HAVE_DISCORDRPC
// Discord Rich Presence
static patch_t *envelope;
#endif

static huddrawlist_h luahuddrawlist_game;
static huddrawlist_h luahuddrawlist_titlecard;

//
// STATUS BAR CODE
//

boolean ST_SameTeam(player_t *a, player_t *b)
{
	// Spectator chat.
	if (a->spectator && b->spectator)
	{
		return true;
	}

	// Team chat.
	if (G_GametypeHasTeams() == true)
	{
		// You get team messages if you're on the same team.
		return (a->team == b->team);
	}

	// Not that everyone's not on the same team, but team messages go to normal chat if everyone's not in the same team.
	return true;
}

static boolean st_stopped = true;

void ST_Ticker(boolean run)
{
	if (st_stopped)
		return;

	if (run)
		ST_runTitleCard();
}

// 0 is default, any others are special palettes.
INT32 st_palette = 0;
UINT32 st_translucency = 10;
fixed_t st_fadein = 0;

void ST_doPaletteStuff(void)
{
	INT32 palette;

	if (stplyr && stplyr->flashcount && cv_reducevfx.value == 0)
		palette = stplyr->flashpal;
	else
		palette = 0;

#ifdef HWRENDER
	if (rendermode == render_opengl)
		palette = 0; // No flashpals here in OpenGL
#endif

	if (palette != st_palette)
	{
		st_palette = palette;

		if (rendermode == render_soft)
		{
			//V_SetPaletteLump(GetPalette()); // Reset the palette -- is this needed?
			if (!r_splitscreen)
				V_SetPalette(palette);
		}
	}
}

void ST_UnloadGraphics(void)
{
	Patch_FreeTag(PU_HUDGFX);
}

void ST_LoadGraphics(void)
{
	// SRB2 border patch
	// st_borderpatchnum = W_GetNumForName("GFZFLR01");
	// scr_borderpatch = W_CacheLumpNum(st_borderpatchnum, PU_HUDGFX);

	// the original Doom uses 'STF' as base name for all face graphics
	// Graue 04-08-2004: face/name graphics are now indexed by skins
	//                   but load them in R_AddSkins, that gets called
	//                   first anyway
	// cache the status bar overlay icons (fullscreen mode)
	K_LoadKartHUDGraphics();
	K_UpdateMidVotePatches();

	// Midnight Channel:
	HU_UpdatePatch(&hud_tv1, "HUD_TV1");
	HU_UpdatePatch(&hud_tv2, "HUD_TV2");

#ifdef HAVE_DISCORDRPC
	// Discord Rich Presence
	HU_UpdatePatch(&envelope, "K_REQUES");
#endif
}

// made separate so that skins code can reload custom face graphics
void ST_LoadFaceGraphics(INT32 skinnum)
{
#define FACE_MAX (FACE_MINIMAP+1)
	spritedef_t *sprdef = &skins[skinnum]->sprites[SPR2_XTRA];
	spriteframe_t *sprframe;
	UINT8 i = 0, maxer = min(sprdef->numframes, FACE_MAX);
	while (i < maxer)
	{
		sprframe = &sprdef->spriteframes[i];
		faceprefix[skinnum][i] = W_CachePatchNum(sprframe->lumppat[0], PU_HUDGFX);
		i++;
	}
	if (i < FACE_MAX)
	{
		patch_t *missing = W_CachePatchName("MISSING", PU_HUDGFX);
		while (i < FACE_MAX)
		{
			faceprefix[skinnum][i] = missing;
			i++;
		}
	}
#undef FACE_MAX
}

void ST_ReloadSkinFaceGraphics(void)
{
	INT32 i;

	for (i = 0; i < numskins; i++)
		ST_LoadFaceGraphics(i);
}

static inline void ST_InitData(void)
{
	// 'link' the statusbar display to a player, which could be
	// another player than consoleplayer, for example, when you
	// change the view in a multiplayer demo with F12.
	stplyr = &players[displayplayers[0]];

	st_palette = -1;
}

static inline void ST_Stop(void)
{
	if (st_stopped)
		return;

#ifdef HWRENDER
	if (rendermode != render_opengl)
#endif
		V_SetPalette(0);

	st_stopped = true;
}

void ST_Start(void)
{
	if (!st_stopped)
		ST_Stop();

	ST_InitData();

	if (!dedicated)
		st_stopped = false;
}

//
// Initializes the status bar, sets the defaults border patch for the window borders.
//

// used by OpenGL mode, holds lumpnum of flat used to fill space around the viewwindow
lumpnum_t st_borderpatchnum;

void ST_Init(void)
{
	if (dedicated)
		return;

	ST_LoadGraphics();

	luahuddrawlist_game = LUA_HUD_CreateDrawList();
	luahuddrawlist_titlecard = LUA_HUD_CreateDrawList();
}

// change the status bar too, when pressing F12 while viewing a demo.
void ST_changeDemoView(void)
{
	// the same routine is called at multiplayer deathmatch spawn
	// so it can be called multiple times
	ST_Start();
}

// =========================================================================
//                         STATUS BAR OVERLAY
// =========================================================================

boolean st_overlay;

/*
static INT32 SCZ(INT32 z)
{
	return FixedInt(FixedMul(z<<FRACBITS, vid.fdupy));
}
*/

/*
static INT32 SCY(INT32 y)
{
	//31/10/99: fixed by Hurdler so it _works_ also in hardware mode
	// do not scale to resolution for hardware accelerated
	// because these modes always scale by default
	y = SCZ(y); // scale to resolution
	if (splitscreen)
	{
		y >>= 1;
		if (stplyr != &players[displayplayers[0]])
			y += vid.height / 2;
	}
	return y;
}
*/

/*
static INT32 STRINGY(INT32 y)
{
	//31/10/99: fixed by Hurdler so it _works_ also in hardware mode
	// do not scale to resolution for hardware accelerated
	// because these modes always scale by default
	if (splitscreen)
	{
		y >>= 1;
		if (stplyr != &players[displayplayers[0]])
			y += BASEVIDHEIGHT / 2;
	}
	return y;
}
*/

/*
static INT32 SPLITFLAGS(INT32 f)
{
	// Pass this V_SNAPTO(TOP|BOTTOM) and it'll trim them to account for splitscreen! -Red
	if (splitscreen)
	{
		if (stplyr != &players[displayplayers[0]])
			f &= ~V_SNAPTOTOP;
		else
			f &= ~V_SNAPTOBOTTOM;
	}
	return f;
}
*/

/*
static INT32 SCX(INT32 x)
{
	return FixedInt(FixedMul(x<<FRACBITS, vid.fdupx));
}
*/

#if 0
static INT32 SCR(INT32 r)
{
	fixed_t y;
		//31/10/99: fixed by Hurdler so it _works_ also in hardware mode
	// do not scale to resolution for hardware accelerated
	// because these modes always scale by default
	y = FixedMul(r*FRACUNIT, vid.fdupy); // scale to resolution
	if (splitscreen)
	{
		y >>= 1;
		if (stplyr != &players[displayplayers[0]])
			y += vid.height / 2;
	}
	return FixedInt(FixedDiv(y, vid.fdupy));
}
#endif

// =========================================================================
//                          INTERNAL DRAWING
// =========================================================================

// Devmode information

static void ST_pushRow(INT32 *height)
{
	*height -= 4;
}

static void ST_pushDebugString(INT32 *height, const char *string)
{
	V_DrawRightAlignedSmallString(319, *height, V_MONOSPACE, string);
	ST_pushRow(height);
}

static void ST_pushDebugStringHighlighted(INT32 *height, const char *string)
{
	V_DrawRightAlignedSmallString(319, *height, V_MONOSPACE | V_YELLOWMAP, string);
	ST_pushRow(height);
}

static void ST_pushDebugTimeMS(INT32 *height, const char *label, UINT32 ms)
{
	ST_pushDebugString(height, va("%s%02d:%05.2f", label,
				ms / 60000, ms % 60000 / 1000.f));
}

static void ST_drawMusicDebug(INT32 *height)
{
	const char *mname = Music_CurrentSong();
	boolean looping = Music_CanLoop(Music_CurrentId());
	UINT8 i = 0;

	const musicdef_t *def;
	const char *format;

	ST_pushDebugString(height, va("    Tune: %8s", Music_CurrentId()));
	ST_pushRow(height);

	if (!strcmp(mname, ""))
	{
		ST_pushDebugString(height, "Song: <NOTHING>");
		return;
	}

	def = S_FindMusicDef(mname, &i);
	format = I_SongType();

	ST_pushDebugTimeMS(height, " Elapsed: ", I_GetSongPosition());
	ST_pushDebugTimeMS(height, looping
			? "  Loop B: "
			: "Duration: ", I_GetSongLength());

	if (looping)
	{
		ST_pushDebugTimeMS(height, "  Loop A: ", I_GetSongLoopPoint());
	}

	ST_pushRow(height);

	if (format)
	{
		ST_pushDebugString(height, va("  Format: %8s", I_SongType()));
	}

	ST_pushDebugString(height, va("    Song: %8s", mname));

	if (def)
	{
		ST_pushRow(height);

		if (def->debug_volume != 0)
		{
			ST_pushDebugStringHighlighted(height, va("Debug Volume: %4d/100", def->debug_volume));
		}

		ST_pushDebugString(height, va("  Volume: %4d/100", def->volume));
	}
}

static void ST_drawRenderDebug(INT32 *height)
{
	const struct RenderStats *i = &g_renderstats;

	ST_pushDebugString(height, va("     Visplanes: %4s", sizeu1(i->visplanes)));
	ST_pushDebugString(height, va("      Drawsegs: %4s", sizeu1(i->drawsegs)));

	ST_pushRow(height);

	ST_pushDebugString(height, va("Skybox Portals: %4s", sizeu1(i->skybox_portals)));
}

static void ST_drawDemoDebug(INT32 *height)
{
	if (!demo.recording && !demo.playback)
		return;

	size_t needle = demo.buffer->p - demo.buffer->buffer;
	size_t size = demo.buffer->size;
	double percent = (double)needle / size * 100.0;
	double avg = (double)needle / leveltime;

	ST_pushDebugString(height, va("%s/%s bytes", sizeu1(needle), sizeu2(size)));
	ST_pushDebugString(height, va(
			"%.2f/%.2f MB %5.2f%%",
			needle / (1024.0 * 1024.0),
			size / (1024.0 * 1024.0),
			percent
	));
	ST_pushDebugString(height, va(
			"%.2f KB/s (ETA %.2f minutes)",
			avg * TICRATE / 1024.0,
			(size - needle) / (avg * TICRATE * 60.0)
	));
	ST_pushDebugString(height, va("Demo %04x (%s)", demo.version, demo.recording ? "recording" : "playback"));
}

void ST_drawDebugInfo(void)
{
	INT32 height = 192;

	const UINT8 screen = min(r_splitscreen, cv_devmode_screen.value - 1);

	// devmode_screen = 1..4
	stplyr = &players[displayplayers[screen]];

	if (!stplyr->mo)
		return;

	if (cv_ticrate.value)
	{
		height -= 20;
	}

	// was cv_showping compensation
	height -= 20;

	if (cht_debug & DBG_BASIC)
	{
		camera_t *cam = &camera[screen];
		if (stplyr->spectator || cam->freecam)
		{
			const fixed_t a = AngleFixed(cam->angle);
			fixed_t p = AngleFixed(cam->aiming);

			if (p > (180 * FRACUNIT))
			{
				p = -((360 * FRACUNIT) - p);
			}

			V_DrawRightAlignedString(320, height - 32, V_MONOSPACE, va("X: %6d", cam->x>>FRACBITS));
			V_DrawRightAlignedString(320, height - 24, V_MONOSPACE, va("Y: %6d", cam->y>>FRACBITS));
			V_DrawRightAlignedString(320, height - 16, V_MONOSPACE, va("Z: %6d", cam->z>>FRACBITS));
			V_DrawRightAlignedString(320, height - 8, V_MONOSPACE, va("A: %6d", FixedInt(a)));
			V_DrawRightAlignedString(320, height, V_MONOSPACE, va("P: %6d", FixedInt(p)));

			height -= 48;
		}
		else
		{
			const fixed_t d = AngleFixed(stplyr->mo->angle);

			V_DrawRightAlignedString(320, height - 24, V_MONOSPACE, va("X: %6d", stplyr->mo->x>>FRACBITS));
			V_DrawRightAlignedString(320, height - 16, V_MONOSPACE, va("Y: %6d", stplyr->mo->y>>FRACBITS));
			V_DrawRightAlignedString(320, height - 8, V_MONOSPACE, va("Z: %6d", stplyr->mo->z>>FRACBITS));
			V_DrawRightAlignedString(320, height, V_MONOSPACE, va("A: %6d", FixedInt(d)));

			height -= 40;
		}
	}

	if (cht_debug & DBG_DETAILED)
	{
		//V_DrawRightAlignedString(320, height - 104, V_MONOSPACE, va("SHIELD: %5x", stplyr->powers[pw_shield]));
		V_DrawRightAlignedString(320, height - 96,  V_MONOSPACE, va("SCALE: %5d%%", (stplyr->mo->scale*100)/FRACUNIT));
		//V_DrawRightAlignedString(320, height - 88,  V_MONOSPACE, va("DASH: %3d/%3d", stplyr->dashspeed>>FRACBITS, FixedMul(stplyr->maxdash,stplyr->mo->scale)>>FRACBITS));
		//V_DrawRightAlignedString(320, height - 80,  V_MONOSPACE, va("AIR: %4d, %3d", stplyr->powers[pw_underwater], stplyr->powers[pw_spacetime]));

		// Flags
		//V_DrawRightAlignedString(304-64, height - 72, V_MONOSPACE, "Flags:");
		//V_DrawString(304-60,             height - 72, (stplyr->jumping) ? V_GREENMAP : V_REDMAP, "JM");
		//V_DrawString(304-40,             height - 72, (stplyr->pflags & PF_JUMPED) ? V_GREENMAP : V_REDMAP, "JD");
		//V_DrawString(304-20,             height - 72, (stplyr->pflags & PF_SPINNING) ? V_GREENMAP : V_REDMAP, "SP");
		//V_DrawString(304,                height - 72, (stplyr->pflags & PF_STARTDASH) ? V_GREENMAP : V_REDMAP, "ST");

		V_DrawRightAlignedString(320, height - 64, V_MONOSPACE, va("CEILZ: %6d", stplyr->mo->ceilingz>>FRACBITS));
		V_DrawRightAlignedString(320, height - 56, V_MONOSPACE, va("FLOORZ: %6d", stplyr->mo->floorz>>FRACBITS));

		V_DrawRightAlignedString(320, height - 48, V_MONOSPACE, va("CNVX: %6d", stplyr->cmomx>>FRACBITS));
		V_DrawRightAlignedString(320, height - 40, V_MONOSPACE, va("CNVY: %6d", stplyr->cmomy>>FRACBITS));
		V_DrawRightAlignedString(320, height - 32, V_MONOSPACE, va("PLTZ: %6d", stplyr->mo->pmomz>>FRACBITS));

		V_DrawRightAlignedString(320, height - 24, V_MONOSPACE, va("MOMX: %6d", stplyr->rmomx>>FRACBITS));
		V_DrawRightAlignedString(320, height - 16, V_MONOSPACE, va("MOMY: %6d", stplyr->rmomy>>FRACBITS));
		V_DrawRightAlignedString(320, height - 8,  V_MONOSPACE, va("MOMZ: %6d", stplyr->mo->momz>>FRACBITS));
		V_DrawRightAlignedString(320, height,      V_MONOSPACE, va("SPEED: %6d", stplyr->speed>>FRACBITS));

		height -= 120;
	}

	if (cht_debug & DBG_RNG) // randomizer testing
	{
		// TODO: this only accounts for the undefined class,
		// which should be phased out as much as possible anyway.
		// Figure out some other way to display all of the RNG classes.

		fixed_t peekres = P_RandomPeek(PR_UNDEFINED);

		V_DrawRightAlignedString(320, height - 16, V_MONOSPACE, va("Init: %08x", P_GetInitSeed(PR_UNDEFINED)));
		V_DrawRightAlignedString(320, height - 8,  V_MONOSPACE, va("Seed: %08x", P_GetRandSeed(PR_UNDEFINED)));
		V_DrawRightAlignedString(320, height,      V_MONOSPACE, va("==  : %08x", peekres));

		height -= 32;
	}

	if (cht_debug & DBG_MUSIC)
	{
		ST_drawMusicDebug(&height);
	}

	if (cht_debug & DBG_RENDER)
	{
		ST_drawRenderDebug(&height);
	}

	if (cht_debug & DBG_DEMO)
	{
		ST_drawDemoDebug(&height);
	}

	if (cht_debug & DBG_MEMORY)
		V_DrawRightAlignedString(320, height,     V_MONOSPACE, va("Heap used: %7sKB", sizeu1(Z_TagsUsage(0, INT32_MAX)>>10)));
}

tic_t lt_ticker = 0, lt_lasttic = 0;
tic_t lt_exitticker = 0, lt_endtime = 0;
tic_t lt_fade = 0;

// SRB2KART: HUD shit for new titlecards:
static patch_t *tcchev1;
static patch_t *tcchev2;

static patch_t *tcol1;
static patch_t *tcol2;

static patch_t *tcroundbar;
static patch_t *tcround;
static patch_t *tcbonus;

static patch_t *tccircletop;
static patch_t *tccirclebottom;
static patch_t *tccirclebg;

static patch_t *tcbanner;
static patch_t *tcbanner2;

static patch_t *tcroundnum[10], *tsroundnum[10];
static patch_t *tcroundbonus, *tsroundbonus;

static patch_t *tcactnum[10];
static patch_t *tcact;

static patch_t *twarn[2];
static patch_t *ttext[2];

// some coordinates define to make my life easier....
#define FINAL_ROUNDX (24)
#define FINAL_EGGY (160)
#define FINAL_ROUNDY (16)
#define FINAL_BANNERY (160)

INT32 chev1x, chev1y, chev2x, chev2y, chevtflag;
INT32 roundx, roundy;
INT32 bannerx, bannery;

INT32 roundnumx, roundnumy;
INT32 eggx1, eggx2, eggy1, eggy2;

// These are all arbitrary values found by trial and error trying to align the hud lmao.
// But they'll work.
#define BASE_CHEV1X (252)
#define BASE_CHEV1Y (60)
#define BASE_CHEV2X (65)
#define BASE_CHEV2Y (135)

#define TTANIMTHRESHOLD (TICRATE)
#define TTFADESTART (TTANIMTHRESHOLD-4)
#define TTANIMSTART (TTANIMTHRESHOLD-16)
#define TTANIMENDTHRESHOLD (TICRATE*3)
#define TTANIMEND (TICRATE*4)

//
// Load the graphics for the title card.
// Don't let LJ see this
//
static void ST_cacheLevelTitle(void)
{
	UINT8 i;
	char buf[9];

	// SRB2KART
	tcchev1 = 		(patch_t *)W_CachePatchName("TCCHEV1W", PU_HUDGFX);
	tcchev2 = 		(patch_t *)W_CachePatchName("TCCHEV2W", PU_HUDGFX);

	tcol1 = 		(patch_t *)W_CachePatchName("TCCHOL1", PU_HUDGFX);
	tcol2 = 		(patch_t *)W_CachePatchName("TCCHOL2", PU_HUDGFX);

	tcroundbar = 	(patch_t *)W_CachePatchName("TCBB0", PU_HUDGFX);
	tcround = 		(patch_t *)W_CachePatchName("TCROUND", PU_HUDGFX);
	tcbonus = 		(patch_t *)W_CachePatchName("TCBONUS", PU_HUDGFX);

	tccircletop = 	(patch_t *)W_CachePatchName("TCSN1", PU_HUDGFX);
	tccirclebottom =(patch_t *)W_CachePatchName("TCSN2", PU_HUDGFX);
	tccirclebg = 	(patch_t *)W_CachePatchName("TCEG3", PU_HUDGFX);

	tcbanner = 		(patch_t *)W_CachePatchName("TCBSKA0", PU_HUDGFX);
	tcbanner2 = 	(patch_t *)W_CachePatchName("TCBC0", PU_HUDGFX);

	tcact =			(patch_t *)W_CachePatchName("TT_ACT", PU_HUDGFX);

	twarn[0] =		(patch_t *)W_CachePatchName("K_BOSW01", PU_HUDGFX);
	twarn[1] =		(patch_t *)W_CachePatchName("K_BOSW02", PU_HUDGFX);

	ttext[0] =		(patch_t *)W_CachePatchName("K_BOST01", PU_HUDGFX);
	ttext[1] =		(patch_t *)W_CachePatchName("K_BOST02", PU_HUDGFX);

	// Cache round #
	for (i=1; i <= 10; i++)
	{
		sprintf(buf, "TT_RND%d", i);
		tcroundnum[i-1] = (patch_t *)W_CachePatchName(buf, PU_HUDGFX);
	}
	tcroundbonus =	(patch_t *)W_CachePatchName("TT_RNDX", PU_HUDGFX);

	for (i=1; i <= 10; i++)
	{
		sprintf(buf, "TT_RNS%d", i);
		tsroundnum[i-1] = (patch_t *)W_CachePatchName(buf, PU_HUDGFX);
	}
	tsroundbonus =	(patch_t *)W_CachePatchName("TT_RNSX", PU_HUDGFX);

	// Cache act #
	for (i=0; i < 10; i++)
	{
		sprintf(buf, "TT_ACT%d", i);
		tcactnum[i] = (patch_t *)W_CachePatchName(buf, PU_HUDGFX);
	}

}

//
// Start the title card.
//
void ST_startTitleCard(void)
{
	// cache every HUD patch used
	ST_cacheLevelTitle();

	// Set most elements to start off-screen, ST_runTitleCard will have them slide in afterwards
	chev1x = BASE_CHEV1X +350;	// start off-screen
	chev1y = BASE_CHEV1Y;
	chev2x = BASE_CHEV2X -350;	// start off-screen
	chev2y = BASE_CHEV2Y;
	chevtflag = 0;

	roundx = -999;
	roundy = -999;

	roundnumx = -999;
	roundnumy = -999;
	eggx1 = -999;
	eggx2 = -999;
	eggy1 = -999;
	eggy2 = -999;

	bannery = 300;

	// initialize HUD variables
	lt_ticker = lt_exitticker = lt_lasttic = 0;
	lt_endtime = 4*TICRATE;	// + (10*NEWTICRATERATIO);
	lt_fade = 0;

	WipeStageTitle = false;
}

//
// What happens before drawing the title card.
// Which is just setting the HUD translucency.
//
void ST_preDrawTitleCard(void)
{
	if (!G_IsTitleCardAvailable())
		return;

	if (lt_ticker >= (lt_endtime + TICRATE))
		return;

	// Kart: nothing
}

patch_t *ST_getRoundPicture(boolean small)
{
	patch_t *roundico = NULL;

	if (roundqueue.position > 0 && roundqueue.position <= roundqueue.size)
	{
		if (roundqueue.entries[roundqueue.position-1].overridden == true)
		{
			roundico = small ? tsroundbonus : tcroundbonus;
		}
		else if (grandprixinfo.gp == true && grandprixinfo.eventmode != GPEVENT_NONE)
		{
			const char *gppic = (small ? gametypes[gametype]->gppicmini : gametypes[gametype]->gppic);
			if (gppic[0])
				roundico = W_CachePatchName(gppic, PU_PATCH);
			else
				roundico = tcroundbonus;
		}
		else if (roundqueue.roundnum > 0 && roundqueue.roundnum <= 10)
		{
			patch_t **source = small ? tsroundnum : tcroundnum;
			roundico = source[roundqueue.roundnum-1];
		}
	}

	return roundico;
}

//
// Run the title card.
// Called from ST_Ticker.
//
void ST_runTitleCard(void)
{
	boolean run = !(paused || P_AutoPause() || (g_fast_forward > 0 && demo.simplerewind == DEMO_REWIND_OFF));
	INT32 auxticker;
	boolean doroundicon = (ST_getRoundPicture(false) != NULL);

	if (run && lt_fade < 16)
	{
		lt_fade++;
	}

	if (!G_IsTitleCardAvailable())
		return;

	if (lt_ticker >= (lt_endtime + TICRATE))
		return;

	if (run || (lt_ticker < PRELEVELTIME))
	{
		// tick
		lt_ticker++;

		// SRB2KART
		// side Zig-Zag positions...
		if (K_CheckBossIntro() == true)
		{
			// Handle name info...
			if (bossinfo.enemyname)
			{
				UINT32 len = strlen(bossinfo.enemyname)+1;
				if (len > 1 && bossinfo.titleshow < len)
				{
					len = (lt_endtime-(TICRATE/2))/len;
					if (lt_ticker % len == 0)
					{
						char c = toupper(bossinfo.enemyname[bossinfo.titleshow]);
						bossinfo.titleshow++;
						c -= LT_FONTSTART;
						if (c < 0 || c >= LT_FONTSIZE || !fontv[GTFN_FONT].font[(INT32)c] || !bossinfo.titlesound)
						{
							;
						}
						else
						{
							S_StartSound(NULL, bossinfo.titlesound);
						}
					}
				}
			}
			// No matter the circumstances, scroll the WARN...
			{
				patch_t *localwarn = twarn[encoremode ? 1 : 0];
				patch_t *localtext = ttext[encoremode ? 1 : 0];

				if (localwarn->width)
				{
					bannerx = ((lt_ticker*2)%(localwarn->width));
				}

				if (localtext->width)
				{
					bannery = -((lt_ticker*4)%(localtext->width));
				}
			}

			if (run && lt_ticker < PRELEVELTIME)
			{
				lt_fade--;
			}
		}
		else
		{
			// LEVEL FADE
			if (run && lt_ticker < TTFADESTART)
			{
				lt_fade--; // don't fade yet
			}

			// TITLECARD START
			if (lt_ticker < TTANIMSTART)
			{
				chev1x = max(BASE_CHEV1X, (BASE_CHEV1X +350) - (INT32)(lt_ticker)*50);
				chev2x = min(BASE_CHEV2X, (BASE_CHEV2X -350) + (INT32)(lt_ticker)*50);
			}

			// OPEN ZIG-ZAGS 1 SECOND IN
			if (lt_ticker > TTANIMTHRESHOLD)
			{
				auxticker = (INT32)(lt_ticker) - TTANIMTHRESHOLD;

				chev1x = min(320, BASE_CHEV1X + auxticker*16);
				chev1y = max(0, BASE_CHEV1Y - auxticker*16);

				chev2x = max(0, BASE_CHEV2X - auxticker*16);
				chev2y = min(200, BASE_CHEV2Y + auxticker*16);

				// translucent fade after opening up.
				chevtflag = min(5, ((auxticker)/5)) << V_ALPHASHIFT;


				// OPEN ZIG-ZAG: END OF ANIMATION (they leave the screen borders)
				if (lt_ticker > TTANIMENDTHRESHOLD)
				{
					auxticker = (INT32)lt_ticker - TTANIMENDTHRESHOLD;

					chev1x += auxticker*16;
					chev1y -= auxticker*16;

					chev2x -= auxticker*16;
					chev2y += auxticker*16;
				}
			}

		// 	ROUND BAR + EGG

			eggy1 = FINAL_EGGY;	// Make sure to reset that each call so that Y position doesn't go bonkers

			// SLIDE BAR IN, SLIDE "ROUND" DOWNWARDS
			if (lt_ticker <= TTANIMTHRESHOLD)
			{
				INT32 interptimer = (INT32)lt_ticker - TTANIMSTART;
				// INT32 because tic_t is unsigned and we want this to be potentially negative

				if (interptimer >= 0)
				{
					INT32 interpdiff = ((TTANIMTHRESHOLD-TTANIMSTART) - interptimer);
					interpdiff *= interpdiff;	// interpdiff^2

					roundx = FINAL_ROUNDX - interpdiff;
					roundy = FINAL_ROUNDY - interpdiff;
					eggy1 = FINAL_EGGY + interpdiff;

				}
			}
			// SLIDE BAR OUT, SLIDE "ROUND" DOWNWARDS FASTER
			else if (lt_ticker >= TTANIMENDTHRESHOLD)
			{
				auxticker = (INT32)lt_ticker - TTANIMENDTHRESHOLD;

				roundx = FINAL_ROUNDX - auxticker*24;
				roundy = FINAL_ROUNDY + auxticker*48;
				eggy1 = FINAL_EGGY + auxticker*48;
			}

			// follow the round bar.
			eggx1 = roundx + tcroundbar->width/2;

			// initially, both halves are on the same coordinates.
			eggx2 = eggx1;
			eggy2 = eggy1;
			// same for the background (duh)
			roundnumx = eggx1;
			roundnumy = eggy1;

			// split both halves of the egg, but only do that in grand prix!
			if (doroundicon && lt_ticker > TTANIMTHRESHOLD + TICRATE/2)
			{
				auxticker = (INT32)lt_ticker - (TTANIMTHRESHOLD + TICRATE/2);

				eggx1 -= auxticker*12;
				eggy1 -= auxticker*12;

				eggx2 += auxticker*12;
				eggy2 += auxticker*12;

			}


		// SCROLLING BOTTOM BANNER

			// SLIDE BANNER UPWARDS WITH A FUNNY BOUNCE (this requires trig :death:)
			if (lt_ticker < TTANIMTHRESHOLD)
			{
				INT32 costimer = (INT32)lt_ticker - TTANIMSTART;
				// INT32 because tic_t is unsigned and we want this to be potentially negative

				if (costimer > 0)
				{
					// For this animation, we're going to do a tiny bit of stupid trigonometry.
					// Admittedly all of this is going to look like magic numbers, and honestly? They are.

					// start at angle 355 (where y = ~230 with our params)
					// and go to angle 131 (where y = ~160 with our params)

					UINT8 basey = 190;
					UINT8 amplitude = 45;
					fixed_t ang = (355 - costimer*14)*FRACUNIT;

					bannery = basey + (amplitude * FINECOSINE(FixedAngle(ang)>>ANGLETOFINESHIFT)) / FRACUNIT;
				}
			}
			// SLIDE BANNER DOWNWARDS OUT OF THE SCREEN AT THE END
			else if (lt_ticker >= TTANIMENDTHRESHOLD)
			{
				auxticker = (INT32)lt_ticker - TTANIMENDTHRESHOLD;
				bannery = FINAL_BANNERY + auxticker*16;
			}

			// No matter the circumstances, scroll the banner...
			bannerx = -((lt_ticker*2)%(tcbanner->width));
		}


		// used for hud slidein
		if (lt_ticker >= lt_endtime)
			lt_exitticker++;
	}
}

//
// Draw the title card itself.
//

void ST_drawTitleCard(void)
{
	char *lvlttl = mapheaderinfo[gamemap-1]->lvlttl;
	char *zonttl = mapheaderinfo[gamemap-1]->zonttl; // SRB2kart
	UINT8 actnum = mapheaderinfo[gamemap-1]->actnum;

	INT32 acttimer;
	fixed_t actscale;
	angle_t fakeangle;

	INT32 pad = ((vid.width/vid.dupx) - BASEVIDWIDTH)/2;
	INT32 bx = bannerx;	// We need to make a copy of that otherwise pausing will cause problems.

	if (!G_IsTitleCardAvailable())
		return;

	if (!LUA_HudEnabled(hud_stagetitle))
		goto luahook;

	if (lt_ticker >= (lt_endtime + TICRATE))
		goto luahook;

	if ((lt_ticker-lt_lasttic) > 1)
		lt_ticker = lt_lasttic+1;

	// Avoid HOMs while drawing the start of the titlecard
	if (lt_ticker < TTANIMSTART)
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, levelfadecol);

	if (K_CheckBossIntro() == true)
	{
		// WARNING!
		// https://twitter.com/matthewseiji/status/1485003284196716544
		// the above tweet is directly responsible for the existence of bosses in this game at all
		{
#define LOTIME 5
#define HITIME 15

			patch_t *localwarn;
			INT32 transp;
			boolean encorehack = ((levelfadecol == 0) && lt_ticker <= PRELEVELTIME+4);

#define DRAWBOSSWARN(pat) \
			localwarn = pat[encoremode ? 1 : 0];\
			\
			if ((localwarn->width > 0) && (lt_ticker + (HITIME-transp) <= lt_endtime)) \
			{ \
				if (transp > HITIME-1)\
				{ \
					transp = HITIME-1; \
				} \
				\
				transp = (((10*transp)/HITIME)<<V_ALPHASHIFT) | (encorehack ? V_SUBTRACT : V_ADD); \
				\
				while (bx > -pad) \
					bx -= localwarn->width; \
				while (bx < BASEVIDWIDTH+pad) \
				{ \
					V_DrawFixedPatch(bx*FRACUNIT, 60*FRACUNIT, FRACUNIT, V_SNAPTOLEFT|transp, localwarn, NULL); \
					bx += localwarn->width; \
				} \
			}

			transp = (lt_ticker+HITIME) % (LOTIME+HITIME);
			DRAWBOSSWARN(twarn);

			transp = (lt_ticker+HITIME+3) % (LOTIME+HITIME);
			bx = bannery;
			DRAWBOSSWARN(ttext);

#undef DRAWBOSSWARN
#undef LOTIME
#undef HITIME
		}

		// Everything else...
		if (bossinfo.enemyname)
		{
			bx = V_TitleCardStringWidth(bossinfo.enemyname, false);

			// Name.
			V_DrawTitleCardString((BASEVIDWIDTH - bx)/2, 80, bossinfo.enemyname, 0, true, bossinfo.titleshow, lt_exitticker, false);

			// Under-bar.
			{
				angle_t fakeang = 0;
				fixed_t scalex = FRACUNIT;

				// Handle scaling.
				if (lt_ticker <= 3)
				{
					fakeang = (lt_ticker*ANGLE_45)/2;
					scalex = FINESINE(fakeang>>ANGLETOFINESHIFT);
				}
				else if (lt_exitticker > 1)
				{
					if (lt_exitticker <= 4)
					{
						fakeang = ((lt_exitticker-1)*ANGLE_45)/2;
						scalex = FINECOSINE(fakeang>>ANGLETOFINESHIFT);
					}
					else
					{
						scalex = 0;
					}
				}
				// Handle subtitle.
				else if (bossinfo.subtitle && lt_ticker >= TICRATE/2)
				{
					INT32 by = 80+32;
					if (lt_ticker == TICRATE/2 || lt_exitticker == 1)
					{
						;
					}
					else if (lt_ticker == (TICRATE/2)+1 || lt_ticker == lt_endtime)
					{
						by += 3;
					}
					else
					{
						by += 5;
					}

					V_DrawRightAlignedThinString((BASEVIDWIDTH+bx)/2, by, V_FORCEUPPERCASE, bossinfo.subtitle);
				}

				// Now draw the under-bar itself.
				if (scalex > 0)
				{
					bx = FixedMul(bx, scalex);
					V_DrawFill((BASEVIDWIDTH-(bx+2))/2, 80+32, bx+2, 3, 31);
					V_DrawFill((BASEVIDWIDTH-(bx))/2, 80+32+1, bx, 1, 0);
				}
			}
		}
		lt_lasttic = lt_ticker;
		goto luahook;
	}

	// Background zig-zags
	V_DrawFixedPatch((chev1x)*FRACUNIT, (chev1y)*FRACUNIT, FRACUNIT, chevtflag, tcchev1, NULL);
	V_DrawFixedPatch((chev2x)*FRACUNIT, (chev2y)*FRACUNIT, FRACUNIT, chevtflag, tcchev2, NULL);

	patch_t *roundico = ST_getRoundPicture(false);

	// Draw ROUND bar, scroll it downwards.
	V_DrawFixedPatch(roundx*FRACUNIT, ((-32) + (lt_ticker%32))*FRACUNIT, FRACUNIT, V_SNAPTOTOP|V_SNAPTOLEFT, tcroundbar, NULL);
	// Draw ROUND text
	if (roundico)
	{
		V_DrawFixedPatch((roundx+10)*FRACUNIT, roundy*FRACUNIT, FRACUNIT, V_SNAPTOTOP|V_SNAPTOLEFT,
			((grandprixinfo.gp && grandprixinfo.eventmode != GPEVENT_NONE) ? tcbonus : tcround),
			NULL);
	}

	// round num background
	V_DrawFixedPatch(roundnumx*FRACUNIT, roundnumy*FRACUNIT, FRACUNIT, V_SNAPTOBOTTOM|V_SNAPTOLEFT, tccirclebg, NULL);

	// Scrolling banner
	if (tcbanner->width > 0)
	{
		while (bx > -pad)
			bx -= tcbanner->width;
		while (bx < BASEVIDWIDTH+pad)
		{
			V_DrawFixedPatch(bx*FRACUNIT, (bannery)*FRACUNIT, FRACUNIT, V_SNAPTOBOTTOM|V_SNAPTOLEFT, tcbanner, NULL);
			bx += tcbanner->width;
		}
	}

	// If possible, draw round number/icon
	if (roundico)
	{
		V_DrawFixedPatch(roundnumx*FRACUNIT, roundnumy*FRACUNIT, FRACUNIT, V_SNAPTOBOTTOM|V_SNAPTOLEFT, roundico, NULL);
	}

	// Draw both halves of the egg
	V_DrawFixedPatch(eggx1*FRACUNIT, eggy1*FRACUNIT, FRACUNIT, V_SNAPTOBOTTOM|V_SNAPTOLEFT, tccircletop, NULL);
	V_DrawFixedPatch(eggx2*FRACUNIT, eggy2*FRACUNIT, FRACUNIT, V_SNAPTOBOTTOM|V_SNAPTOLEFT, tccirclebottom, NULL);

	// Now the level name.
	V_DrawTitleCardString((actnum) ? 265 : 280, 60, lvlttl, V_SNAPTORIGHT, false, lt_ticker, TTANIMENDTHRESHOLD, false);

	if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
		V_DrawTitleCardString((actnum) ? 265 : 280, 60+32, strlen(zonttl) ? zonttl : "ZONE", V_SNAPTORIGHT, false, lt_ticker - strlen(lvlttl), TTANIMENDTHRESHOLD, false);

	// the act has a similar graphic animation, but we'll handle it here since it's only like 2 graphics lmfao.
	if (actnum && actnum < 10)
	{

		// compute delay before the act should appear.
		acttimer = lt_ticker - strlen(lvlttl);
		if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
			acttimer -= strlen((strlen(zonttl)) ? (zonttl) : ("ZONE"));

		actscale = 0;
		fakeangle = 0;

		if (acttimer >= 0)
		{

			if (acttimer < TTANIMENDTHRESHOLD)	// spin in
			{
				fakeangle = min(360 + 90, acttimer*41) * ANG1;
				actscale = FINESINE(fakeangle>>ANGLETOFINESHIFT);
			}
			else								// spin out
			{
				// Make letters disappear...
				acttimer -= TTANIMENDTHRESHOLD;

				fakeangle = max(0, (360+90) - acttimer*41)*ANG1;
				actscale = FINESINE(fakeangle>>ANGLETOFINESHIFT);
			}

			if (actscale)
			{
				// draw the top:
				V_DrawStretchyFixedPatch(286*FRACUNIT, 76*FRACUNIT, abs(actscale), FRACUNIT, V_SNAPTORIGHT|(actscale < 0 ? V_FLIP : 0), tcact, NULL);
				V_DrawStretchyFixedPatch(286*FRACUNIT, 123*FRACUNIT, abs(actscale), FRACUNIT, V_SNAPTORIGHT|(actscale < 0 ? V_FLIP : 0), tcactnum[actnum], NULL);
			}
		}
	}

	lt_lasttic = lt_ticker;

luahook:
	if (renderisnewtic)
	{
		LUA_HUD_ClearDrawList(luahuddrawlist_titlecard);
		LUA_HookHUD(luahuddrawlist_titlecard, HUD_HOOK(titlecard));
	}
	LUA_HUD_DrawList(luahuddrawlist_titlecard);
}

// Clear defined coordinates, we don't need them anymore
#undef FINAL_ROUNDX
#undef FINAL_EGGY
#undef FINAL_ROUNDY
#undef FINAL_BANNERY

#undef BASE_CHEV1X
#undef BASE_CHEV1Y
#undef BASE_CHEV2X
#undef BASE_CHEV2Y

#undef TTANIMTHRESHOLD
#undef TTANIMSTART
#undef TTANIMENDTHRESHOLD
#undef TTANIMEND

//
// Drawer for G_PreLevelTitleCard.
//
void ST_preLevelTitleCardDrawer(void)
{
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, levelfadecol);

	ST_drawTitleCard();
	I_OsPolling();
	I_UpdateNoBlit();
}

//
// Draw the status bar overlay, customisable: the user chooses which
// kind of information to overlay
//
static void ST_overlayDrawer(void)
{
	const UINT8 viewnum = R_GetViewNumber();

	// hu_showscores = auto hide score/time/rings when tab rankings are shown
	if (!(hu_showscores && (netgame || multiplayer)))
	{
		K_drawKartHUD();

		if (renderisnewtic)
		{
			LUA_HookHUD(luahuddrawlist_game, HUD_HOOK(game));
		}

		if (cv_showviewpointtext.value)
		{
			if (!demo.attract && !P_IsPartyPlayer(stplyr) && !camera[viewnum].freecam)
			{
				if (r_splitscreen <= 1)
				{
					INT32 flags = V_SNAPTOBOTTOM | V_SPLITSCREEN | V_HUDTRANS;
					INT32 x = BASEVIDWIDTH/2;
					INT32 y = (BASEVIDHEIGHT / (r_splitscreen + 1)) - 34;
					INT32 width = 50;

					const char *text = player_names[stplyr-players];
					int font = KART_FONT;
					fixed_t textwidth = V_StringScaledWidth(FRACUNIT, FRACUNIT, FRACUNIT, flags, font, text);
					fixed_t threshold = textwidth;

					// k_drawKartHUD
					if (gametyperules & GTR_CIRCUIT)
						threshold = 200*FRACUNIT;
					else if (gametyperules & GTR_BUMPERS)
						threshold = 100*FRACUNIT;

					if (LUA_HudEnabled(hud_gametypeinfo) && textwidth > threshold)
					{
						y += 5;
						font = TINY_FONT;
						textwidth = V_StringScaledWidth(FRACUNIT, FRACUNIT, FRACUNIT, flags, font, text);
					}

					if (r_splitscreen)
					{
						flags = (flags & ~V_ALPHAMASK) | V_HUDTRANSHALF;
						y += 4;
					}
					else
					{
						V_DrawFill(x - width/2, y + 6, width, 2, flags | 31);
						V_DrawCenteredThinString(x, y, flags | V_ORANGEMAP, "Watching");
					}

					V_DrawStringScaled(x*FRACUNIT - textwidth/2, (y+10)*FRACUNIT,
						FRACUNIT, FRACUNIT, FRACUNIT, flags, NULL, font, text);
				}
				else
				{
					INT32 y = BASEVIDHEIGHT/2 - 12;
					INT32 f = V_HUDTRANSHALF|V_SNAPTOBOTTOM|V_SPLITSCREEN;
					const char *s = player_names[stplyr-players];
					if (viewnum & 1)
						V_DrawThinString(12, y, f|V_SNAPTOLEFT, s);
					else
						V_DrawRightAlignedThinString(BASEVIDWIDTH/2 - 12, y, f|V_SNAPTORIGHT, s);
				}
			}
		}
	}

	K_DrawMidVote();
}

// MayonakaStatic: draw Midnight Channel's TV-like borders
static void ST_MayonakaStatic(void)
{
	INT32 flag = (leveltime%2) ? V_90TRANS : V_70TRANS;

	V_DrawFixedPatch(0, 0, FRACUNIT, V_SNAPTOTOP|V_SNAPTOLEFT|flag, hud_tv1, NULL);
	V_DrawFixedPatch(320<<FRACBITS, 0, FRACUNIT, V_SNAPTOTOP|V_SNAPTORIGHT|V_FLIP|flag, hud_tv1, NULL);
	V_DrawFixedPatch(0, 142<<FRACBITS, FRACUNIT, V_SNAPTOBOTTOM|V_SNAPTOLEFT|flag, hud_tv2, NULL);
	V_DrawFixedPatch(320<<FRACBITS, 142<<FRACBITS, FRACUNIT, V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_FLIP|flag, hud_tv2, NULL);
}

#ifdef HAVE_DISCORDRPC
void ST_AskToJoinEnvelope(void)
{
	const tic_t freq = TICRATE/2;

	if (menuactive)
		return;

	if ((leveltime % freq) < freq/2)
		return;

	V_DrawFixedPatch(296*FRACUNIT, 2*FRACUNIT, FRACUNIT, V_SNAPTOTOP|V_SNAPTORIGHT, envelope, NULL);
	// maybe draw number of requests with V_DrawPingNum ?
}
#endif

static INT32 ST_ServerSplash_OpacityFlag(INT32 opacity)
{
	if (opacity >= NUMTRANSMAPS)
	{
		return 0;
	}

	opacity = max(opacity, 1);
	return (NUMTRANSMAPS - opacity) << V_ALPHASHIFT;
}

#define SPLASH_LEN ((FRACUNIT * TICRATE) * 3)
#define SPLASH_WAIT ((FRACUNIT * TICRATE) / 2)

void ST_DrawServerSplash(boolean timelimited)
{
	static fixed_t splashTime = -SPLASH_WAIT;
	static char prevContext[8] = {0};

	if (memcmp(prevContext, server_context, 8) != 0)
	{
		// Context changed, we want to draw it again
		splashTime = -SPLASH_WAIT;
		memcpy(prevContext, server_context, 8);
	}

	if (lt_ticker < lt_endtime)
	{
		// Level title is running rn
		return;
	}

	if (timelimited
		&& splashTime >= SPLASH_LEN)
	{
		// We finished drawing it
		return;
	}

	splashTime += renderdeltatics;
	if (splashTime <= 0)
	{
		// We're waiting a tiny bit to draw it
		return;
	}

	const INT32 splashTic = splashTime >> FRACBITS;
	INT32 opacity = NUMTRANSMAPS;
	if (splashTic < NUMTRANSMAPS)
	{
		opacity = splashTic;
	}
	else if (timelimited
		&& splashTic > (SPLASH_LEN >> FRACBITS) - NUMTRANSMAPS)
	{
		opacity = (SPLASH_LEN >> FRACBITS) - splashTic;
	}

	INT32 opacityFlag = ST_ServerSplash_OpacityFlag(opacity);

	patch_t *gridPatch = W_CachePatchName("MOTDBG", PU_CACHE);

	if (gridPatch && gridPatch->width)
	{
		fixed_t gridX = -(splashTime / 3) % (gridPatch->width * FRACUNIT);
		fixed_t gridY = (gridPatch->height) * FRACUNIT;
		INT32 gridOpacity = ST_ServerSplash_OpacityFlag(opacity / 2);
		fixed_t maxX = (vid.width * FRACUNIT) / vid.dupx;

		while (gridX < maxX)
		{
			V_DrawFixedPatch(
				gridX, gridY,
				FRACUNIT,
				(V_SNAPTOLEFT|V_SNAPTOTOP) | V_SUBTRACT | V_VFLIP | gridOpacity,
				gridPatch,
				NULL
			);

			gridX += (gridPatch->width * FRACUNIT);
		}
	}

	// We're a bit crunched atm to do this but hopefully in the future
	// the icon can be made a bmp file on the hard drive that the server
	// sends on client join instead.
	patch_t *iconPatch = W_CachePatchName("MOTDICON", PU_CACHE);
	fixed_t iconX = (BASEVIDWIDTH - 16 - iconPatch->width) * FRACUNIT;
	fixed_t iconY = (8) * FRACUNIT;
	V_DrawFixedPatch(
		iconX, iconY,
		FRACUNIT,
		(V_SNAPTORIGHT|V_SNAPTOTOP) | opacityFlag,
		iconPatch,
		NULL
	);

	fixed_t textX = (BASEVIDWIDTH - 16 - 36) * FRACUNIT;
	fixed_t textY = (24 - 8) * FRACUNIT;
	fixed_t textW = V_StringScaledWidth(
		FRACUNIT, FRACUNIT, FRACUNIT,
		(V_SNAPTORIGHT|V_SNAPTOTOP) | opacityFlag,
		MED_FONT,
		connectedservername
	);

	V_DrawStringScaled(
		textX - textW, textY,
		FRACUNIT, FRACUNIT, FRACUNIT,
		(V_SNAPTORIGHT|V_SNAPTOTOP) | opacityFlag,
		NULL,
		MED_FONT,
		connectedservername
	);

	textY += 10*FRACUNIT;

	if (connectedservercontact[0] != 0)
	{
		V_DrawRightAlignedThinStringAtFixed(
			textX, textY,
			(V_SNAPTORIGHT|V_SNAPTOBOTTOM) | opacityFlag,
			va("Contact @ %c%s", '\x80' + cv_shoutcolor.value, connectedservercontact)
		);
	}
}

void ST_DrawSaveReplayHint(INT32 flags)
{
	const char *text;
	if (gamestate == GS_LEVEL && camera[0].freecam)
	{
		text = va(
			"<c> Disable Freecam to <b_pressed> %s replay",
			(demo.willsave && demo.titlename[0])
				? "rename"
				: "save"
		);
	}
	else if (demo.willsave && demo.titlename[0])
		text = "Replay will be saved.  <b> Change title";
	else
		text = "<b> Save replay";

	K_DrawGameControl(BASEVIDWIDTH - 2, 2, 0, text, 2, TINY_FONT, flags|V_YELLOWMAP);
}

static fixed_t ST_CalculateFadeIn(player_t *player)
{
	const tic_t length = TICRATE/4;

	if (player->tally.hudSlide != 0)
	{
		tic_t timer = length - player->tally.hudSlide;
		fixed_t f = timer * FRACUNIT;

		// Not interpolated at very beginning or end
		if (timer > 0 && timer < length)
		{
			f += FRACUNIT - rendertimefrac;
		}

		return f / length;
	}

	tic_t timer = lt_exitticker;

	if (K_CheckBossIntro() == true || G_IsTitleCardAvailable() == false)
	{
		if (timeinmap <= 16)
			timer = 0;
		else
			timer = timeinmap-16;
	}

	if (timer < length)
	{
		fixed_t f = timer * FRACUNIT;

		// Not interpolated at very beginning
		if (timer > 0)
		{
			f += rendertimefrac;
		}

		return f / length;
	}

	// Not interpolated at very end
	return FRACUNIT;
}

void ST_Drawer(void)
{
	boolean stagetitle = false; // Decide whether to draw the stage title or not

	// Doom's status bar only updated if necessary.
	// However, ours updates every frame regardless, so the "refresh" param was removed
	//(void)refresh;

	// force a set of the palette by using doPaletteStuff()
	if (vid.recalc)
		st_palette = -1;

	// Do red-/gold-shifts from damage/items
#ifdef HWRENDER
	//25/08/99: Hurdler: palette changes is done for all players,
	//                   not only player1! That's why this part
	//                   of code is moved somewhere else.
	if (rendermode == render_soft)
#endif
		if (rendermode != render_none) ST_doPaletteStuff();

#ifdef BETAVERSION

	if (g_takemapthumbnail == TMT_NO)
	{
		char nag[256];
		snprintf(nag, sizeof(nag), "KartKrew.org - %s %s - Pre-release testing version", SRB2VERSION, BETAVERSION);

		V_DrawCenteredMenuString(BASEVIDWIDTH/2, 2, V_60TRANS|V_SNAPTOTOP, nag);

		V_DrawCenteredMenuString(BASEVIDWIDTH/2, BASEVIDHEIGHT - 10, V_60TRANS|V_SNAPTOBOTTOM, nag);
	}

#endif

	fixed_t localfadein[MAXSPLITSCREENPLAYERS];

	// HUD fading for anything not tied to a single player,
	// i.e. the minimap. Since individual splitscreen
	// players' HUDs may fade away before other's, use the
	// the last one remaining.
	{
		fixed_t maxFade = 0;
		UINT8 i;

		for (i = 0; i <= r_splitscreen; i++)
		{
			localfadein[i] = ST_CalculateFadeIn(&players[displayplayers[i]]);

			if (localfadein[i] > maxFade)
			{
				maxFade = localfadein[i];
			}
		}

		st_translucency = FixedMul(10, maxFade);
	}

	// Check for a valid level title
	// If the HUD is enabled
	// And, if Lua is running, if the HUD library has the stage title enabled
	if ((stagetitle = (G_IsTitleCardAvailable() && *mapheaderinfo[gamemap-1]->lvlttl != '\0' && !(hu_showscores && (netgame || multiplayer)))))
		ST_preDrawTitleCard();

	if (st_overlay)
	{
		UINT8 i;
		if (renderisnewtic)
		{
			LUA_HUD_ClearDrawList(luahuddrawlist_game);
		}
		// No deadview!
		for (i = 0; i <= r_splitscreen; i++)
		{
			stplyr = &players[displayplayers[i]];
			st_fadein = localfadein[i];
			R_SetViewContext(VIEWCONTEXT_PLAYER1 + i);
			R_InterpolateView(rendertimefrac); // to assist with object tracking
			ST_overlayDrawer();
		}

		LUA_HUD_DrawList(luahuddrawlist_game);

		// draw Midnight Channel's overlay ontop
		if (mapheaderinfo[gamemap-1]->typeoflevel & TOL_TV)	// Very specific Midnight Channel stuff.
			ST_MayonakaStatic();
	}

	// See d_main.c and V_DrawCustomFadeScreen for the hacks that prevents this being here
	/*if (lt_fade < 16)
	{
		// Level fade-in
		V_DrawCustomFadeScreen(((levelfadecol == 0) ? "FADEMAP1" : "FADEMAP0"), 31-(lt_fade*2));
	}*/

	if (stagetitle)
		ST_drawTitleCard();

	K_DrawDialogue();

	// Replay manual-save stuff
	if (demo.recording && multiplayer && demo.savebutton && demo.savebutton + 3*TICRATE < leveltime)
	{
		tic_t fadeLength = TICRATE;
		tic_t t = leveltime - (demo.savebutton + 3*TICRATE);
		INT32 flags = V_SNAPTOTOP | V_SNAPTORIGHT |
			(Easing_Linear(min(t, fadeLength) * FRACUNIT / fadeLength, 9, 0) << V_ALPHASHIFT);

		ST_DrawSaveReplayHint(flags);
	}
}
