// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2016 by Kay "Kaito" Sinclaire.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  f_wipe.c
/// \brief SRB2 2.1 custom fade mask "wipe" behavior.

#include "f_finale.h"
#include "i_video.h"
#include "v_video.h"

#include "r_data.h" // NearestColor
#include "r_draw.h" // transtable
#include "p_pspr.h" // tr_transxxx

#include "w_wad.h"
#include "z_zone.h"

#include "i_time.h"
#include "i_system.h"
#include "i_threads.h"
#include "console.h"
#include "d_main.h"
#include "m_misc.h" // movie mode
#include "d_clisrv.h" // So the network state can be updated during the wipe

#include "g_game.h"
#include "st_stuff.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#if NUMSCREENS < 5
#define NOWIPE // do not enable wipe image post processing for ARM, SH and MIPS CPUs
#endif

// SRB2Kart
#include "k_menu.h"

using namespace srb2;

typedef struct fademask_s {
	UINT8* mask;
	UINT16 width, height;
	size_t size;
	fixed_t xscale, yscale;
} fademask_t;

UINT8 wipedefs[NUMWIPEDEFS] = {
	99, // wipe_credits_intermediate (0)

	0,  // wipe_level_toblack
	0,  // wipe_intermission_toblack
	0,  // wipe_voting_toblack,
	0,  // wipe_continuing_toblack
	0,  // wipe_titlescreen_toblack
	1,  // wipe_menu_toblack
	99, // wipe_credits_toblack
	0,  // wipe_evaluation_toblack
	0,  // wipe_ceremony_toblack
	UINT8_MAX, // wipe_intro_toblack (hardcoded)
	99, // wipe_cutscene_toblack (hardcoded)

	72, // wipe_encore_toinvert
	99, // wipe_encore_towhite

	UINT8_MAX, // wipe_level_final
	0,  // wipe_intermission_final
	0,  // wipe_voting_final
	0,  // wipe_continuing_final
	0,  // wipe_titlescreen_final
	1,  // wipe_menu_final
	99, // wipe_credits_final
	0,  // wipe_evaluation_final
	0,  // wipe_ceremony_final
	99, // wipe_intro_final (hardcoded)
	99  // wipe_cutscene_final (hardcoded)
};

static boolean g_wipedef_toblack[NUMWIPEDEFS] = {
	true, // wipe_credits_intermediate (0)

	true, // wipe_level_toblack
	true, // wipe_intermission_toblack
	true, // wipe_voting_toblack,
	true, // wipe_continuing_toblack
	true, // wipe_titlescreen_toblack
	true, // wipe_menu_toblack
	true, // wipe_credits_toblack
	true, // wipe_evaluation_toblack
	true, // wipe_ceremony_toblack
	true, // wipe_intro_toblack (hardcoded)
	true, // wipe_cutscene_toblack (hardcoded)

	false, // wipe_encore_toinvert
	false, // wipe_encore_towhite

	true, // wipe_level_final
	true, // wipe_intermission_final
	true, // wipe_voting_final
	true, // wipe_continuing_final
	true, // wipe_titlescreen_final
	true, // wipe_menu_final
	true, // wipe_credits_final
	true, // wipe_evaluation_final
	true, // wipe_ceremony_final
	true, // wipe_intro_final (hardcoded)
	true  // wipe_cutscene_final (hardcoded)
};

static boolean g_wipedef_toinvert[NUMWIPEDEFS] = {
	false, // wipe_credits_intermediate (0)

	false, // wipe_level_toblack
	false, // wipe_intermission_toblack
	false, // wipe_voting_toblack,
	false, // wipe_continuing_toblack
	false, // wipe_titlescreen_toblack
	false, // wipe_menu_toblack
	false, // wipe_credits_toblack
	false, // wipe_evaluation_toblack
	false, // wipe_ceremony_toblack
	false, // wipe_intro_toblack (hardcoded)
	false, // wipe_cutscene_toblack (hardcoded)

	true, // wipe_encore_toinvert
	false, // wipe_encore_towhite

	false, // wipe_level_final
	false, // wipe_intermission_final
	false, // wipe_voting_final
	false, // wipe_continuing_final
	false, // wipe_titlescreen_final
	false, // wipe_menu_final
	false, // wipe_credits_final
	false, // wipe_evaluation_final
	false, // wipe_ceremony_final
	false, // wipe_intro_final (hardcoded)
	false  // wipe_cutscene_final (hardcoded)
};

static boolean g_wipedef_towhite[NUMWIPEDEFS] = {
	false, // wipe_credits_intermediate (0)

	false, // wipe_level_toblack
	false, // wipe_intermission_toblack
	false, // wipe_voting_toblack,
	false, // wipe_continuing_toblack
	false, // wipe_titlescreen_toblack
	false, // wipe_menu_toblack
	false, // wipe_credits_toblack
	false, // wipe_evaluation_toblack
	false, // wipe_ceremony_toblack
	false, // wipe_intro_toblack (hardcoded)
	false, // wipe_cutscene_toblack (hardcoded)

	false, // wipe_encore_toinvert
	true, // wipe_encore_towhite

	false, // wipe_level_final
	false, // wipe_intermission_final
	false, // wipe_voting_final
	false, // wipe_continuing_final
	false, // wipe_titlescreen_final
	false, // wipe_menu_final
	false, // wipe_credits_final
	false, // wipe_evaluation_final
	false, // wipe_ceremony_final
	false, // wipe_intro_final (hardcoded)
	false  // wipe_cutscene_final (hardcoded)
};

static boolean g_wipedef_crossfade[NUMWIPEDEFS] = {
	false, // wipe_credits_intermediate (0)

	false, // wipe_level_toblack
	false, // wipe_intermission_toblack
	false, // wipe_voting_toblack,
	false, // wipe_continuing_toblack
	false, // wipe_titlescreen_toblack
	false, // wipe_menu_toblack
	false, // wipe_credits_toblack
	false, // wipe_evaluation_toblack
	false, // wipe_ceremony_toblack
	false, // wipe_intro_toblack (hardcoded)
	false, // wipe_cutscene_toblack (hardcoded)

	false, // wipe_encore_toinvert
	false, // wipe_encore_towhite

	true, // wipe_level_final
	true, // wipe_intermission_final
	true, // wipe_voting_final
	true, // wipe_continuing_final
	true, // wipe_titlescreen_final
	true, // wipe_menu_final
	true, // wipe_credits_final
	true, // wipe_evaluation_final
	true, // wipe_ceremony_final
	true, // wipe_intro_final (hardcoded)
	true  // wipe_cutscene_final (hardcoded)
};

//--------------------------------------------------------------------------
//                        SCREEN WIPE PACKAGE
//--------------------------------------------------------------------------

boolean WipeInAction = false;
UINT8 g_wipemode = 0;
UINT8 g_wipetype = 0;
UINT8 g_wipeframe = 0;
boolean g_wipereverse = false;
boolean g_wipeencorewiggle = false;
boolean WipeStageTitle = false;
INT32 lastwipetic = 0;

#ifndef NOWIPE

#define GENLEN 31

static UINT8 *wipe_scr; //screen 0 (main drawing)
static UINT8 pallen;
static fixed_t paldiv;

/** Create fademask_t from lump
  *
  * \param	lump	Lump name to get data from
  * \return	fademask_t for lump
  */
static fademask_t *F_GetFadeMask(UINT8 masknum, UINT8 scrnnum) {
	static char lumpname[9] = "FADEmmss";
	static fademask_t fm = {NULL,0,0,0,0,0};
	lumpnum_t lumpnum;
	UINT8 *lump, *mask;
	size_t lsize;
	RGBA_t *pcolor;

	if (masknum > 99 || scrnnum > 99)
		goto freemask;

	// SRB2Kart: This suddenly triggers ERRORMODE now
	//sprintf(&lumpname[4], "%.2hu%.2hu", (UINT16)masknum, (UINT16)scrnnum);

	lumpname[4] = '0'+(masknum/10);
	lumpname[5] = '0'+(masknum%10);

	lumpname[6] = '0'+(scrnnum/10);
	lumpname[7] = '0'+(scrnnum%10);

	lumpnum = W_CheckNumForName(lumpname);
	if (lumpnum == LUMPERROR)
		goto freemask;

	lump = static_cast<UINT8*>(W_CacheLumpNum(lumpnum, PU_CACHE));
	lsize = W_LumpLength(lumpnum);
	switch (lsize)
	{
		case 256000: // 640x400
			fm.width = 640;
			fm.height = 400;
			break;
		case 64000: // 320x200
			fm.width = 320;
			fm.height = 200;
			break;
		case 16000: // 160x100
			fm.width = 160;
			fm.height = 100;
			break;
		case 4000: // 80x50 (minimum)
			fm.width = 80;
			fm.height = 50;
			break;

		default: // bad lump
			CONS_Alert(CONS_WARNING, "Fade mask lump %s of incorrect size, ignored\n", lumpname);
		case 0: // end marker (not bad!, but still need clearing)
			goto freemask;
	}
	if (lsize != fm.size)
		fm.mask = reinterpret_cast<UINT8*>(Z_Realloc(fm.mask, lsize, PU_STATIC, NULL));
	fm.size = lsize;

	mask = fm.mask;

	while (lsize--)
	{
		// Determine pixel to use from fademask
		pcolor = &pMasterPalette[*lump++];
		*mask++ = FixedDiv((pcolor->s.red+1)<<FRACBITS, paldiv)>>FRACBITS;
	}

	fm.xscale = FixedDiv(vid.width<<FRACBITS, fm.width<<FRACBITS);
	fm.yscale = FixedDiv(vid.height<<FRACBITS, fm.height<<FRACBITS);
	return &fm;

	// Landing point for freeing data -- do this instead of just returning NULL
	// this ensures the fade data isn't remaining in memory, unused
	// (could be up to 256,000 bytes if it's a HQ fade!)
	freemask:
	if (fm.mask)
	{
		Z_Free(fm.mask);
		fm.mask = NULL;
		fm.size = 0;
	}

	return NULL;
}

#endif

static void refresh_wipe_screen_texture(rhi::Rhi& rhi, rhi::Handle<rhi::Texture>& tex)
{
	bool recreate = false;
	if (!tex)
	{
		recreate = true;
	}
	else
	{
		rhi::TextureDetails deets = rhi.get_texture_details(tex);
		if (deets.width != static_cast<uint32_t>(vid.width) || deets.height != static_cast<uint32_t>(vid.height))
		{
			recreate = true;
			rhi.destroy_texture(tex);
			tex = rhi::kNullHandle;
		}
	}

	if (!recreate)
	{
		return;
	}

	tex = rhi.create_texture({
		rhi::TextureFormat::kRGBA,
		static_cast<uint32_t>(vid.width),
		static_cast<uint32_t>(vid.height),
		rhi::TextureWrapMode::kClamp,
		rhi::TextureWrapMode::kClamp
	});
}

/** Save the "before" screen of a wipe.
  */
void F_WipeStartScreen(void)
{
#ifndef NOWIPE
#ifdef HWRENDER
	if(rendermode == render_opengl)
	{
		HWR_StartScreenWipe();
		return;
	}
#endif

	rhi::Rhi* rhi = srb2::sys::get_rhi(srb2::sys::g_current_rhi);

	if (!rhi)
	{
		return;
	}

	hwr2::HardwareState* hw_state = srb2::sys::main_hardware_state();

	refresh_wipe_screen_texture(*rhi, hw_state->wipe_frames.start);

	hw_state->twodee_renderer->flush(*rhi, g_2d);

	rhi::Rect dst_region = {0, 0, static_cast<uint32_t>(vid.width), static_cast<uint32_t>(vid.height)};
	rhi::TextureDetails backbuf_deets = rhi->get_texture_details(hw_state->backbuffer->color());
	dst_region.w = std::min(dst_region.w, backbuf_deets.width);
	dst_region.h = std::min(dst_region.h, backbuf_deets.height);
	rhi->copy_framebuffer_to_texture(hw_state->wipe_frames.start, dst_region, dst_region);

	I_FinishUpdate();
#endif
}

/** Save the "after" screen of a wipe.
  */
void F_WipeEndScreen(void)
{
#ifndef NOWIPE
#ifdef HWRENDER
	if(rendermode == render_opengl)
	{
		HWR_EndScreenWipe();
		return;
	}
#endif

	rhi::Rhi* rhi = srb2::sys::get_rhi(srb2::sys::g_current_rhi);

	if (!rhi)
	{
		return;
	}

	hwr2::HardwareState* hw_state = srb2::sys::main_hardware_state();

	refresh_wipe_screen_texture(*rhi, hw_state->wipe_frames.end);

	hw_state->twodee_renderer->flush(*rhi, g_2d);

	rhi::Rect dst_region = {0, 0, static_cast<uint32_t>(vid.width), static_cast<uint32_t>(vid.height)};
	rhi::TextureDetails backbuf_deets = rhi->get_texture_details(hw_state->backbuffer->color());
	dst_region.w = std::min(dst_region.w, backbuf_deets.width);
	dst_region.h = std::min(dst_region.h, backbuf_deets.height);
	rhi->copy_framebuffer_to_texture(hw_state->wipe_frames.end, dst_region, dst_region);

	hw_state->blit_rect->set_output(0, 0, dst_region.w, dst_region.h, false, true);
	rhi::TextureDetails start_deets = rhi->get_texture_details(hw_state->wipe_frames.start);
	hw_state->blit_rect->set_texture(hw_state->wipe_frames.start, start_deets.width, start_deets.height);
	hw_state->blit_rect->draw(*rhi);

	I_FinishUpdate();
#endif
}

/** Draw the stage title.
  */
void F_WipeStageTitle(void)
{
	// draw level title
	if ((WipeStageTitle) && G_IsTitleCardAvailable())
	{
		ST_runTitleCard();
		ST_drawTitleCard();
	}
}

/** After setting up the screens you want to wipe,
  * calling this will do a 'typical' wipe.
  */
void F_RunWipe(UINT8 wipemode, UINT8 wipetype, boolean drawMenu, const char *colormap, boolean reverse, boolean encorewiggle)
{
#ifdef NOWIPE
	(void)wipemode;
	(void)wipetype;
	(void)drawMenu;
	(void)colormap;
	(void)reverse;
	(void)encorewiggle;
#else
	tic_t nowtime;
	UINT8 wipeframe = 0;
	fademask_t *fmask;

	lumpnum_t clump = LUMPERROR;
	lighttable_t *fcolor = NULL;

	if (staffsync)
		return;

	if (colormap != NULL)
		clump = W_GetNumForName(colormap);

	if (clump != LUMPERROR && wipetype != UINT8_MAX)
	{
		pallen = 32;
		fcolor = static_cast<lighttable_t*>(Z_MallocAlign((256 * pallen), PU_STATIC, NULL, 8));
		W_ReadLump(clump, fcolor);
	}
	else
	{
		pallen = 11;
		reverse = false;
	}

	paldiv = FixedDiv(257<<FRACBITS, pallen<<FRACBITS);

	// Init the wipe
	WipeInAction = true;
	wipe_scr = screens[0];

	// lastwipetic should either be 0 or the tic we last wiped
	// on for fade-to-black
	for (;;)
	{
		// get fademask first so we can tell if it exists or not
		fmask = F_GetFadeMask(wipetype, wipeframe++);
		if (!fmask)
			break;

		// wait loop
		while (!((nowtime = I_GetTime()) - lastwipetic))
		{
			I_Sleep(cv_sleep.value);
			I_UpdateTime();
		}
		lastwipetic = nowtime;

#ifdef HWRENDER
		if (rendermode == render_opengl)
			HWR_DoWipe(wipetype, wipeframe-1); // send in the wipe type and wipeframe because we need to cache the graphic
		else
#endif

		if (rendermode != render_none) //this allows F_RunWipe to be called in dedicated servers
		{
			// F_DoWipe(fmask, fcolor, reverse);
			g_wipemode = wipemode;
			g_wipetype = wipetype;
			g_wipeframe = wipeframe - 1;
			g_wipereverse = reverse;

			if (encorewiggle)
			{
				g_wipeencorewiggle = wipeframe - 1;
			}
			else
			{
				g_wipeencorewiggle = 0;
			}
			rhi::Rhi* rhi = srb2::sys::get_rhi(srb2::sys::g_current_rhi);
			hwr2::HardwareState* hw_state = srb2::sys::main_hardware_state();

			if (reverse)
			{
				hw_state->wipe->set_start(hw_state->wipe_frames.end);
				hw_state->wipe->set_end(hw_state->wipe_frames.start);
			}
			else
			{
				hw_state->wipe->set_start(hw_state->wipe_frames.start);
				hw_state->wipe->set_end(hw_state->wipe_frames.end);
			}

			hw_state->wipe->set_target_size(static_cast<uint32_t>(vid.width), static_cast<uint32_t>(vid.height));
			hw_state->wipe->draw(*rhi);
		}

		I_OsPolling();
		// The event buffer is rather small so we need to
		// process these events immediately, to make sure
		// inputs don't get stuck (would happen a lot with
		// some controllers that send a lot of analog
		// events).
		D_ProcessEvents(false);
		I_UpdateNoBlit();

		if (drawMenu && rendermode != render_none)
		{
#ifdef HAVE_THREADS
			I_lock_mutex(&k_menu_mutex);
#endif
			M_Drawer(); // menu is drawn even on top of wipes
#ifdef HAVE_THREADS
			I_unlock_mutex(k_menu_mutex);
#endif
		}

		I_FinishUpdate(); // page flip or blit buffer

#ifdef HWRENDER
		if (moviemode && rendermode == render_opengl)
			M_LegacySaveFrame();
		else
#endif
		if (moviemode && rendermode == render_soft)
			I_CaptureVideoFrame();

		NetKeepAlive(); // Update the network so we don't cause timeouts
	}

	WipeInAction = false;

	if (fcolor)
	{
		Z_Free(fcolor);
		fcolor = NULL;
	}
#endif
}

/** Returns tic length of wipe
  * One lump equals one tic
  */
tic_t F_GetWipeLength(UINT8 wipetype)
{
#ifdef NOWIPE
	(void)wipetype;
	return 0;
#else
	static char lumpname[10] = "FADEmmss";
	lumpnum_t lumpnum;
	UINT8 wipeframe;

	if (wipetype > 99)
		return 0;

	for (wipeframe = 0; wipeframe < 100; wipeframe++)
	{
		sprintf(&lumpname[4], "%.2hu%.2hu", (UINT16)wipetype, (UINT16)wipeframe);

		lumpnum = W_CheckNumForName(lumpname);
		if (lumpnum == LUMPERROR)
			return --wipeframe;
	}
	return --wipeframe;
#endif
}

/** Does the specified wipe exist?
  */
boolean F_WipeExists(UINT8 wipetype)
{
#ifdef NOWIPE
	(void)wipetype;
	return false;
#else
	static char lumpname[10] = "FADEmm00";
	lumpnum_t lumpnum;

	if (wipetype > 99)
		return false;

	sprintf(&lumpname[4], "%.2hu00", (UINT16)wipetype);

	lumpnum = W_CheckNumForName(lumpname);
	return !(lumpnum == LUMPERROR);
#endif
}

boolean F_WipeIsToBlack(UINT8 wipemode)
{
	return g_wipedef_toblack[wipemode];
}

boolean F_WipeIsToWhite(UINT8 wipemode)
{
	return g_wipedef_towhite[wipemode];
}

boolean F_WipeIsToInvert(UINT8 wipemode)
{
	return g_wipedef_toinvert[wipemode];
}

boolean F_WipeIsCrossfade(UINT8 wipemode)
{
	return g_wipedef_crossfade[wipemode];
}

