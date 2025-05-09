// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  screen.c
/// \brief Handles multiple resolutions, 8bpp/16bpp(highcolor) modes

#include "doomdef.h"
#include "doomstat.h"
#include "screen.h"
#include "console.h"
#include "am_map.h"
#include "i_time.h"
#include "i_system.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "m_argv.h"
#include "m_misc.h"
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "z_zone.h"
#include "d_main.h"
#include "d_clisrv.h"
#include "f_finale.h"
#include "y_inter.h" // usebuffer
#include "i_sound.h" // closed captions
#include "s_sound.h" // ditto
#include "g_game.h" // ditto
#include "p_local.h" // P_AutoPause()

#ifdef HWRENDER
#include "hardware/hw_main.h"
#include "hardware/hw_light.h"
#include "hardware/hw_model.h"
#endif

// SRB2Kart
#include "r_fps.h" // R_GetFramerateCap

#if defined (USEASM) && !defined (NORUSEASM)//&& (!defined (_MSC_VER) || (_MSC_VER <= 1200))
#define RUSEASM //MSC.NET can't patch itself
#endif

// ------------------
// global video state
// ------------------
viddef_t vid;
INT32 setmodeneeded; //video mode change needed if > 0 (the mode number to set + 1)
UINT8 setrenderneeded = 0;

CV_PossibleValue_t cv_renderer_t[] = {
	{1, "Software"},
#ifdef HWRENDER
	{2, "Legacy GL"},
#endif
	{0, NULL}
};

// =========================================================================
//                           SCREEN VARIABLES
// =========================================================================

INT32 scr_bpp; // current video mode bytes per pixel
UINT8 *scr_borderpatch; // flat used to fill the reduced view borders set at ST_Init()

// =========================================================================

void SCR_SetDrawFuncs(void)
{
	//
	//  setup the right draw routines
	//

	colfuncs[BASEDRAWFUNC] = R_DrawColumn;
	colfuncs[COLDRAWFUNC_FUZZY] = R_DrawTranslucentColumn;
	colfuncs[COLDRAWFUNC_TRANS] = R_DrawTranslatedColumn;
	colfuncs[COLDRAWFUNC_SHADOWED] = R_DrawColumnShadowed;
	colfuncs[COLDRAWFUNC_TRANSTRANS] = R_DrawTranslatedTranslucentColumn;
	colfuncs[COLDRAWFUNC_TWOSMULTIPATCH] = R_Draw2sMultiPatchColumn;
	colfuncs[COLDRAWFUNC_TWOSMULTIPATCHTRANS] = R_Draw2sMultiPatchTranslucentColumn;
	colfuncs[COLDRAWFUNC_FOG] = R_DrawFogColumn;
	colfuncs[COLDRAWFUNC_DROPSHADOW] = R_DrawDropShadowColumn;

	colfuncs_bm[BASEDRAWFUNC] = R_DrawColumn_Brightmap;
	colfuncs_bm[COLDRAWFUNC_FUZZY] = R_DrawTranslucentColumn_Brightmap;
	colfuncs_bm[COLDRAWFUNC_TRANS] = R_DrawTranslatedColumn_Brightmap;
	colfuncs_bm[COLDRAWFUNC_SHADOWED] = R_DrawColumnShadowed_Brightmap;
	colfuncs_bm[COLDRAWFUNC_TRANSTRANS] = R_DrawTranslatedTranslucentColumn_Brightmap;
	colfuncs_bm[COLDRAWFUNC_TWOSMULTIPATCH] = R_Draw2sMultiPatchColumn_Brightmap;
	colfuncs_bm[COLDRAWFUNC_TWOSMULTIPATCHTRANS] = R_Draw2sMultiPatchTranslucentColumn_Brightmap;
	colfuncs_bm[COLDRAWFUNC_FOG] = NULL; // Not needed
	colfuncs_bm[COLDRAWFUNC_DROPSHADOW] = NULL; // Not needed

	spanfuncs[BASEDRAWFUNC] = R_DrawSpan;
	spanfuncs[SPANDRAWFUNC_TRANS] = R_DrawTranslucentSpan;
	spanfuncs[SPANDRAWFUNC_TILTED] = R_DrawSpan_Tilted;
	spanfuncs[SPANDRAWFUNC_TILTEDTRANS] = R_DrawTranslucentSpan_Tilted;
	spanfuncs[SPANDRAWFUNC_SPLAT] = R_DrawSplat;
	spanfuncs[SPANDRAWFUNC_TRANSSPLAT] = R_DrawTranslucentSplat;
	spanfuncs[SPANDRAWFUNC_TILTEDSPLAT] = R_DrawSplat_Tilted;
	spanfuncs[SPANDRAWFUNC_TILTEDTRANSSPLAT] = R_DrawTranslucentSplat_Tilted;
	spanfuncs[SPANDRAWFUNC_SPRITE] = R_DrawFloorSprite;
	spanfuncs[SPANDRAWFUNC_TRANSSPRITE] = R_DrawTranslucentFloorSprite;
	spanfuncs[SPANDRAWFUNC_TILTEDSPRITE] = R_DrawFloorSprite_Tilted;
	spanfuncs[SPANDRAWFUNC_TILTEDTRANSSPRITE] = R_DrawTranslucentFloorSprite_Tilted;
	spanfuncs[SPANDRAWFUNC_WATER] = R_DrawTranslucentWaterSpan;
	spanfuncs[SPANDRAWFUNC_TILTEDWATER] = R_DrawTranslucentWaterSpan_Tilted;
	spanfuncs[SPANDRAWFUNC_FOG] = R_DrawFogSpan;
	spanfuncs[SPANDRAWFUNC_TILTEDFOG] = R_DrawFogSpan_Tilted;

	spanfuncs_bm[BASEDRAWFUNC] = R_DrawSpan_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_TRANS] = R_DrawTranslucentSpan_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_TILTED] = R_DrawSpan_Tilted_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_TILTEDTRANS] = R_DrawTranslucentSpan_Tilted_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_SPLAT] = R_DrawSplat_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_TRANSSPLAT] = R_DrawTranslucentSplat_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_TILTEDSPLAT] = R_DrawSplat_Tilted_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_TILTEDTRANSSPLAT] = R_DrawTranslucentSplat_Tilted_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_SPRITE] = R_DrawFloorSprite_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_TRANSSPRITE] = R_DrawTranslucentFloorSprite_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_TILTEDSPRITE] = R_DrawFloorSprite_Tilted_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_TILTEDTRANSSPRITE] = R_DrawTranslucentFloorSprite_Tilted_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_WATER] = R_DrawTranslucentWaterSpan_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_TILTEDWATER] = R_DrawTranslucentWaterSpan_Tilted_Brightmap;
	spanfuncs_bm[SPANDRAWFUNC_FOG] = NULL; // Not needed
	spanfuncs_bm[SPANDRAWFUNC_TILTEDFOG] = NULL; // Not needed

	// Lactozilla: Non-powers-of-two
	spanfuncs_npo2[BASEDRAWFUNC] = R_DrawSpan_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_TRANS] = R_DrawTranslucentSpan_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_TILTED] = R_DrawSpan_Tilted_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_TILTEDTRANS] = R_DrawTranslucentSpan_Tilted_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_SPLAT] = R_DrawSplat_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_TRANSSPLAT] = R_DrawTranslucentSplat_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_TILTEDSPLAT] = R_DrawSplat_Tilted_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_TILTEDTRANSSPLAT] = R_DrawTranslucentSplat_Tilted_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_SPRITE] = R_DrawFloorSprite_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_TRANSSPRITE] = R_DrawTranslucentFloorSprite_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_TILTEDSPRITE] = R_DrawFloorSprite_Tilted_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_TILTEDTRANSSPRITE] = R_DrawTranslucentFloorSprite_Tilted_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_WATER] = R_DrawTranslucentWaterSpan_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_TILTEDWATER] = R_DrawTranslucentWaterSpan_Tilted_NPO2;
	spanfuncs_npo2[SPANDRAWFUNC_FOG] = NULL; // Not needed
	spanfuncs_npo2[SPANDRAWFUNC_TILTEDFOG] = NULL; // Not needed

	spanfuncs_bm_npo2[BASEDRAWFUNC] = R_DrawSpan_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_TRANS] = R_DrawTranslucentSpan_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_TILTED] = R_DrawSpan_Tilted_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_TILTEDTRANS] = R_DrawTranslucentSpan_Tilted_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_SPLAT] = R_DrawSplat_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_TRANSSPLAT] = R_DrawTranslucentSplat_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_TILTEDSPLAT] = R_DrawSplat_Tilted_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_TILTEDTRANSSPLAT] = R_DrawTranslucentSplat_Tilted_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_SPRITE] = R_DrawFloorSprite_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_TRANSSPRITE] = R_DrawTranslucentFloorSprite_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_TILTEDSPRITE] = R_DrawFloorSprite_Tilted_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_TILTEDTRANSSPRITE] = R_DrawTranslucentFloorSprite_Tilted_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_WATER] = R_DrawTranslucentWaterSpan_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_TILTEDWATER] = R_DrawTranslucentWaterSpan_Tilted_Brightmap_NPO2;
	spanfuncs_bm_npo2[SPANDRAWFUNC_FOG] = NULL; // Not needed
	spanfuncs_bm_npo2[SPANDRAWFUNC_TILTEDFOG] = NULL; // Not needed

	// Debugging - highlight surfaces in flat colors
	spanfuncs_flat[BASEDRAWFUNC] = R_DrawSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TRANS] = R_DrawSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TILTED] = R_DrawTiltedSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TILTEDTRANS] = R_DrawTiltedSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_SPLAT] = R_DrawSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TRANSSPLAT] = R_DrawSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TILTEDSPLAT] = R_DrawTiltedSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TILTEDTRANSSPLAT] = R_DrawTiltedSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_SPRITE] = R_DrawSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TRANSSPRITE] = R_DrawSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TILTEDSPRITE] = R_DrawTiltedSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TILTEDTRANSSPRITE] = R_DrawTiltedSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_WATER] = R_DrawSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TILTEDWATER] = R_DrawTiltedSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_FOG] = R_DrawSpan_Flat;
	spanfuncs_flat[SPANDRAWFUNC_TILTEDFOG] = R_DrawTiltedSpan_Flat;

	R_SetColumnFunc(BASEDRAWFUNC, false);
	R_SetSpanFunc(BASEDRAWFUNC, false, false);
}

void R_SetColumnFunc(size_t id, boolean brightmapped)
{
	I_Assert(id < COLDRAWFUNC_MAX);

	colfunctype = id;

	if (debugrender_highlight != 0)
	{
		colfunc = R_DrawColumn_Flat;
	}
	else if (brightmapped == true && colfuncs_bm[id] != NULL)
	{
		colfunc = colfuncs_bm[id];
	}
	else
	{
		colfunc = colfuncs[id];
	}
}

void R_SetSpanFunc(size_t id, boolean npo2, boolean brightmapped)
{
	I_Assert(id < SPANDRAWFUNC_MAX);

	if (debugrender_highlight != 0 && R_SetSpanFuncFlat(id))
	{
		return;
	}

	if (brightmapped == true && spanfuncs_bm[id] != NULL)
	{
		if (npo2 == true && spanfuncs_bm_npo2[id] != NULL)
		{
			spanfunc = spanfuncs_bm_npo2[id];
		}
		else
		{
			spanfunc = spanfuncs_bm[id];
		}
	}
	else
	{
		if (npo2 == true && spanfuncs_npo2[id] != NULL)
		{
			spanfunc = spanfuncs_npo2[id];
		}
		else
		{
			spanfunc = spanfuncs[id];
		}
	}
}

boolean R_SetSpanFuncFlat(size_t id)
{
	I_Assert(id < SPANDRAWFUNC_MAX);

	if (spanfuncs_flat[id] == NULL)
	{
		return false;
	}

	spanfunc = spanfuncs_flat[id];

	return true;
}

boolean R_CheckColumnFunc(size_t id)
{
	size_t i;

	if (colfunc == NULL)
	{
		// Shouldn't happen.
		return false;
	}

	for (i = 0; i < COLDRAWFUNC_MAX; i++)
	{
		if (colfunc == colfuncs[id] || colfunc == colfuncs_bm[id])
		{
			return true;
		}
	}

	return false;
}

void SCR_SetMode(void)
{
	if (dedicated)
		return;

	if (!(setmodeneeded || setrenderneeded) || WipeInAction)
		return; // should never happen and don't change it during a wipe, BAD!

	// Lactozilla: Renderer switching
	if (setrenderneeded)
	{
		// stop recording movies (APNG only)
		if (setrenderneeded && (moviemode == MM_APNG))
			M_StopMovie();

		// VID_SetMode will call VID_CheckRenderer itself,
		// so no need to do this in here.
		if (!setmodeneeded)
			VID_CheckRenderer();

		vid.recalc = 1;
	}

	// Set the video mode in the video interface.
	if (setmodeneeded)
		VID_SetMode(setmodeneeded - 1);

	V_SetPalette(0);

	SCR_SetDrawFuncs();

	// set the apprpriate drawer for the sky (tall or INT16)
	setmodeneeded = 0;
	setrenderneeded = 0;
}

// do some initial settings for the game loading screen
//
void SCR_Startup(void)
{
	if (dedicated)
	{
		V_Init();
		V_SetPalette(0);
		return;
	}

	vid.modenum = 0;

	V_Init();
	V_Recalc();

	{
		extern struct CVarList *cvlist_screen;
		CV_RegisterList(cvlist_screen);
	}

	V_SetPalette(0);
}

// Called at new frame, if the video mode has changed
//
void SCR_Recalc(void)
{
	if (dedicated)
		return;

	// bytes per pixel quick access
	scr_bpp = vid.bpp;

	V_Recalc();

	// toggle off (then back on) the automap because some screensize-dependent values will
	// be calculated next time the automap is activated.
	if (automapactive)
	{
		am_recalc = true;
		AM_Start();
	}

	// set the screen[x] ptrs on the new vidbuffers
	V_Init();

	// scr_viewsize doesn't change, neither detailLevel, but the pixels
	// per screenblock is different now, since we've changed resolution.
	R_SetViewSize(); //just set setsizeneeded true now ..

	// vid.recalc lasts only for the next refresh...
	con_recalc = true;
	am_recalc = true;
}

// Check for screen cmd-line parms: to force a resolution.
//
// Set the video mode to set at the 1st display loop (setmodeneeded)
//

void SCR_CheckDefaultMode(void)
{
	INT32 scr_forcex, scr_forcey; // resolution asked from the cmd-line

	if (dedicated)
		return;

	// 0 means not set at the cmd-line
	scr_forcex = scr_forcey = 0;

	if (M_CheckParm("-width") && M_IsNextParm())
		scr_forcex = atoi(M_GetNextParm());

	if (M_CheckParm("-height") && M_IsNextParm())
		scr_forcey = atoi(M_GetNextParm());

	if (scr_forcex && scr_forcey)
	{
		CONS_Printf(M_GetText("Using resolution: %d x %d\n"), scr_forcex, scr_forcey);
		// returns -1 if not found, thus will be 0 (no mode change) if not found
		setmodeneeded = VID_GetModeForSize(scr_forcex, scr_forcey) + 1;
	}
	else
	{
		CONS_Printf(M_GetText("Default resolution: %d x %d (%d bits)\n"), cv_scr_width.value,
			cv_scr_height.value, cv_scr_depth.value);
		// see note above
		setmodeneeded = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value) + 1;
	}

	if (cv_renderer.value != (signed)rendermode)
	{
		if (chosenrendermode == render_none) // nothing set at command line
			SCR_ChangeRenderer();
		else
		{
			// Set cv_renderer to the current render mode
			CV_StealthSetValue(&cv_renderer, rendermode);
		}
	}
}

// sets the modenum as the new default video mode to be saved in the config file
void SCR_SetDefaultMode(void)
{
	// remember the default screen size
	CV_SetValue(&cv_scr_width, vid.width);
	CV_SetValue(&cv_scr_height, vid.height);
}

// Change fullscreen on/off according to cv_fullscreen
void SCR_ChangeFullscreen(void);
void SCR_ChangeFullscreen(void)
{
#ifdef DIRECTFULLSCREEN
	// allow_fullscreen is set by VID_PrepareModeList
	// it is used to prevent switching to fullscreen during startup
	if (!allow_fullscreen)
		return;

	if (graphics_started)
	{
		VID_PrepareModeList();
		setmodeneeded = VID_GetModeForSize(vid.width, vid.height) + 1;
	}
	return;
#endif
}

void SCR_ChangeRenderer(void)
{
	if (chosenrendermode != render_none
	|| (signed)rendermode == cv_renderer.value)
		return;

#ifdef HWRENDER
	// Check if OpenGL loaded successfully (or wasn't disabled) before switching to it.
	if ((vid.glstate == VID_GL_LIBRARY_ERROR)
	&& (cv_renderer.value == render_opengl))
	{
		if (M_CheckParm("-nogl"))
			CONS_Alert(CONS_ERROR, "OpenGL rendering was disabled!\n");
		else
			CONS_Alert(CONS_ERROR, "OpenGL never loaded\n");
		return;
	}

	if (rendermode == render_opengl && (vid.glstate == VID_GL_LIBRARY_LOADED)) // Clear these out before switching to software
		HWR_ClearAllTextures();

#endif

	// Set the new render mode
	setrenderneeded = cv_renderer.value;
}

boolean SCR_IsAspectCorrect(INT32 width, INT32 height)
{
	return
	 (  width % BASEVIDWIDTH == 0
	 && height % BASEVIDHEIGHT == 0
	 && width / BASEVIDWIDTH == height / BASEVIDHEIGHT
	 );
}

double averageFPS = 0.0f;

#define USE_FPS_SAMPLES

#ifdef USE_FPS_SAMPLES
#define MAX_FRAME_TIME 0.05
#define NUM_FPS_SAMPLES (16) // Number of samples to store

static double total_frame_time = 0.0;
static int frame_index;
#endif

static boolean fps_init = false;
static precise_t fps_enter = 0;

void SCR_CalculateFPS(void)
{
	precise_t fps_finish = 0;

	double frameElapsed = 0.0;

	if (fps_init == false)
	{
		fps_enter = I_GetPreciseTime();
		fps_init = true;
	}

	fps_finish = I_GetPreciseTime();
	frameElapsed = (double)((INT64)(fps_finish - fps_enter)) / I_GetPrecisePrecision();
	fps_enter = fps_finish;

#ifdef USE_FPS_SAMPLES
	total_frame_time += frameElapsed;
	if (frame_index++ >= NUM_FPS_SAMPLES || total_frame_time >= MAX_FRAME_TIME)
	{
		averageFPS = 1.0 / (total_frame_time / frame_index);
		total_frame_time = 0.0;
		frame_index = 0;
	}
#else
	// Direct, unsampled counter.
	averageFPS = 1.0 / frameElapsed;
#endif
}

void SCR_DisplayTicRate(void)
{
	const UINT8 *ticcntcolor = NULL;
	UINT32 cap = R_GetFramerateCap();
	UINT32 benchmark = (cap == 0) ? I_GetRefreshRate() : cap;
	INT32 x = 317;
	double fps = round(averageFPS);

	if (fps > (benchmark * 0.9))
		ticcntcolor = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_MINT, GTC_CACHE);
	else if (fps < (benchmark * 0.5))
		ticcntcolor = R_GetTranslationColormap(TC_RAINBOW, SKINCOLOR_RASPBERRY, GTC_CACHE);

	if (cap != 0)
	{
		UINT32 digits = 1;
		UINT32 c2 = cap;

		while (c2 > 0)
		{
			c2 = c2 / 10;
			digits++;
		}

		// draw total frame:
		V_DrawPingNum(x<<FRACBITS, 190<<FRACBITS, V_SNAPTOBOTTOM|V_SNAPTORIGHT, cap, ticcntcolor);
		x -= digits * 4;

		// draw "/"
		V_DrawFixedPatch(x<<FRACBITS, 190<<FRACBITS, FRACUNIT, V_SNAPTOBOTTOM|V_SNAPTORIGHT, frameslash, ticcntcolor);
	}

	// draw our actual framerate
	V_DrawPingNum(x<<FRACBITS, 190<<FRACBITS, V_SNAPTOBOTTOM|V_SNAPTORIGHT, fps, ticcntcolor);
}

// SCR_DisplayLocalPing
// Used to draw the user's local ping next to the framerate for a quick check without having to hold TAB for instance. By default, it only shows up if your ping is too high and risks getting you kicked.

void SCR_DisplayLocalPing(void)
{
	// Splitscreen party has its own ping counter, but show the
	// 1P version anyway in some cases:
	// - On intermission, vote, etc. gamestates where the player
	//   HUD is not drawn.
	// - If the menu is opened, since it draws over the player
	//   HUD.
	if (r_splitscreen && gamestate == GS_LEVEL && !menuactive)
	{
		return;
	}

	UINT32 mindelay = playerdelaytable[consoleplayer];
	UINT32 ping = playerpingtable[consoleplayer];
	UINT32 pl = playerpacketlosstable[consoleplayer];

	INT32 dispy = cv_ticrate.value ? 170 : 181;

	HU_drawPing(307 * FRACUNIT, dispy * FRACUNIT, ping, mindelay, pl, V_SNAPTORIGHT | V_SNAPTOBOTTOM, 0);
}


void SCR_ClosedCaptions(void)
{
	UINT8 i;
	boolean gamestopped = (paused || P_AutoPause());
	INT32 basey = BASEVIDHEIGHT;

	if (gamestate != wipegamestate)
		return;

	if (gamestate == GS_LEVEL)
	{
		if (promptactive)
			basey -= 42;
		else if (splitscreen)
			basey -= 8;
	}

	for (i = 0; i < NUMCAPTIONS; i++)
	{
		INT32 flags, y;
		char dot;
		boolean music;

		if (!closedcaptions[i].s)
			continue;

		music = (closedcaptions[i].s-S_sfx == sfx_None);

		if (music && !gamestopped && (closedcaptions[i].t < flashingtics) && (closedcaptions[i].t & 1))
			continue;

		flags = V_SNAPTORIGHT|V_SNAPTOBOTTOM;
		y = basey-((i + 2)*10);

		if (closedcaptions[i].b)
		{
			y -= closedcaptions[i].b * vid.dupy;
			if (renderisnewtic)
			{
				closedcaptions[i].b--;
			}
		}

		if (closedcaptions[i].t < CAPTIONFADETICS)
			flags |= (((CAPTIONFADETICS-closedcaptions[i].t)/2)*V_10TRANS);

		if (music)
			dot = '\x19';
		else if (closedcaptions[i].c && closedcaptions[i].c->origin)
			dot = '\x1E';
		else
			dot = ' ';

		V_DrawRightAlignedString(BASEVIDWIDTH - 20, y, flags,
			va("%c [%s]", dot, (closedcaptions[i].s->caption[0] ? closedcaptions[i].s->caption : closedcaptions[i].s->name)));
	}
}

void SCR_DisplayMarathonInfo(void)
{
	INT32 flags = V_SNAPTOBOTTOM;
	static tic_t entertic, oldentertics = 0, antisplice[2] = {48,0};
	const char *str;
#if 0 // eh, this probably isn't going to be a problem
	if (((signed)marathontime) < 0)
	{
		flags |= V_REDMAP;
		str = "No waiting out the clock to submit a bogus time.";
	}
	else
#endif
	{
		entertic = I_GetTime();
		if (gamecomplete)
			flags |= V_YELLOWMAP;
		else if (marathonmode & MA_INGAME)
			; // see also G_Ticker
		else if (marathonmode & MA_INIT)
			marathonmode &= ~MA_INIT;
		else
			marathontime += entertic - oldentertics;

		// Create a sequence of primes such that their LCM is nice and big.
#define PRIMEV1 13
#define PRIMEV2 17 // I can't believe it! I'm on TV!
		antisplice[0] += (entertic - oldentertics)*PRIMEV2;
		antisplice[0] %= PRIMEV1*((vid.width/vid.dupx)+1);
		antisplice[1] += (entertic - oldentertics)*PRIMEV1;
		antisplice[1] %= PRIMEV1*((vid.width/vid.dupx)+1);
		str = va("%i:%02i:%02i.%02i",
			G_TicsToHours(marathontime),
			G_TicsToMinutes(marathontime, false),
			G_TicsToSeconds(marathontime),
			G_TicsToCentiseconds(marathontime));
		oldentertics = entertic;
	}
	V_DrawFill((antisplice[0]/PRIMEV1)-1, BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTOLEFT);
	V_DrawFill((antisplice[0]/PRIMEV1),   BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTOLEFT|31);
	V_DrawFill(BASEVIDWIDTH-((antisplice[1]/PRIMEV1)-1), BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTORIGHT);
	V_DrawFill(BASEVIDWIDTH-((antisplice[1]/PRIMEV1)),   BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTORIGHT|31);
#undef PRIMEV1
#undef PRIMEV2
	V_DrawPromptBack(-8, cons_backcolor.value);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-8, flags, str);
}
