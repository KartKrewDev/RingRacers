// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "i_video.h"

#include <algorithm>
#include <array>
#include <vector>

#include <imgui.h>

#include "cxxutil.hpp"
#include "f_finale.h"
#include "hwr2/pass_blit_rect.hpp"
#include "hwr2/pass_imgui.hpp"
#include "hwr2/pass_manager.hpp"
#include "hwr2/pass_postprocess.hpp"
#include "hwr2/pass_resource_managers.hpp"
#include "hwr2/pass_screenshot.hpp"
#include "hwr2/pass_software.hpp"
#include "hwr2/pass_twodee.hpp"
#include "hwr2/twodee.hpp"
#include "v_video.h"

// KILL THIS WHEN WE KILL OLD OGL SUPPORT PLEASE
#include "d_netcmd.h" // kill
#include "discord.h"  // kill
#include "doomstat.h" // kill
#include "s_sound.h"  // kill
#include "sdl/ogl_sdl.h"
#include "st_stuff.h" // kill

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

static std::shared_ptr<PassManager> g_passmanager;

Handle<Rhi> srb2::sys::g_current_rhi = kNullHandle;

static bool rhi_changed()
{
	return false;
}

#ifdef HWRENDER
static void finish_legacy_ogl_update()
{
	int player;

	SCR_CalculateFPS();

	if (st_overlay)
	{
		if (cv_songcredits.value)
			HU_DrawSongCredits();

		if (cv_ticrate.value)
			SCR_DisplayTicRate();

		if (cv_showping.value && netgame && (consoleplayer != serverplayer || !server_lagless))
		{
			if (server_lagless)
			{
				if (consoleplayer != serverplayer)
					SCR_DisplayLocalPing();
			}
			else
			{
				for (player = 1; player < MAXPLAYERS; player++)
				{
					if (D_IsPlayerHumanAndGaming(player))
					{
						SCR_DisplayLocalPing();
						break;
					}
				}
			}
		}
		if (cv_mindelay.value && consoleplayer == serverplayer && Playing())
			SCR_DisplayLocalPing();
	}

	if (marathonmode)
		SCR_DisplayMarathonInfo();

	// draw captions if enabled
	if (cv_closedcaptioning.value)
		SCR_ClosedCaptions();

#ifdef HAVE_DISCORDRPC
	if (discordRequestList != NULL)
		ST_AskToJoinEnvelope();
#endif

	OglSdlFinishUpdate(cv_vidwait.value);
}
#endif

static std::shared_ptr<PassManager> build_pass_manager()
{
	std::shared_ptr<PassManager> manager = std::make_shared<PassManager>();

	std::shared_ptr<FramebufferManager> framebuffer_manager = std::make_shared<FramebufferManager>();
	std::shared_ptr<MainPaletteManager> palette_manager = std::make_shared<MainPaletteManager>();
	std::shared_ptr<FlatTextureManager> flat_texture_manager = std::make_shared<FlatTextureManager>();

	std::shared_ptr<SoftwarePass> software_pass = std::make_shared<SoftwarePass>();
	std::shared_ptr<BlitRectPass> blit_sw_pass = std::make_shared<BlitRectPass>(palette_manager, true);
	std::shared_ptr<TwodeePass> twodee = std::make_shared<TwodeePass>();
	twodee->flat_manager_ = flat_texture_manager;
	twodee->data_ = make_twodee_pass_data();
	twodee->ctx_ = &g_2d;
	std::shared_ptr<BlitRectPass> pp_simple_blit_pass = std::make_shared<BlitRectPass>(false);
	std::shared_ptr<PostprocessWipePass> pp_wipe_pass = std::make_shared<PostprocessWipePass>();
	std::shared_ptr<ScreenshotPass> screenshot_pass = std::make_shared<ScreenshotPass>();
	std::shared_ptr<ImguiPass> imgui_pass = std::make_shared<ImguiPass>();
	std::shared_ptr<BlitRectPass> final_composite_pass = std::make_shared<BlitRectPass>(true);

	manager->insert("framebuffer_manager", framebuffer_manager);
	manager->insert("palette_manager", palette_manager);
	manager->insert("flat_texture_manager", flat_texture_manager);

	manager->insert(
		"3d_prepare",
		[framebuffer_manager](PassManager& mgr, Rhi&)
		{
			const bool sw_enabled = rendermode == render_soft;

			mgr.set_pass_enabled("software", !g_wipeskiprender && sw_enabled);
			mgr.set_pass_enabled("blit_sw_prepare", !g_wipeskiprender && sw_enabled);
			mgr.set_pass_enabled("blit_sw", !g_wipeskiprender && sw_enabled);
		},
		[framebuffer_manager](PassManager&, Rhi&)
		{
			if (!WipeInAction)
			{
				framebuffer_manager->swap_main();
			}
		}
	);
	manager->insert("software", software_pass);
	manager->insert(
		"blit_sw_prepare",
		[blit_sw_pass, software_pass, framebuffer_manager](PassManager&, Rhi&)
		{
			blit_sw_pass->set_texture(software_pass->screen_texture(), vid.width, vid.height);
			blit_sw_pass->set_output(framebuffer_manager->current_main_color(), vid.width, vid.height, false, false);
		}
	);
	manager->insert("blit_sw", blit_sw_pass);

	manager->insert(
		"2d_prepare",
		[twodee, framebuffer_manager, palette_manager](PassManager& mgr, Rhi&)
		{
			twodee->output_ = framebuffer_manager->current_main_color();
			twodee->palette_manager_ = palette_manager;
			twodee->output_width_ = vid.width;
			twodee->output_height_ = vid.height;
		}
	);
	manager->insert("2d", twodee);

	manager->insert(
		"pp_final_prepare",
		[](PassManager& mgr, Rhi&)
		{
			mgr.set_pass_enabled("pp_final_wipe_prepare", WipeInAction);
			mgr.set_pass_enabled("pp_final_wipe", WipeInAction);
			mgr.set_pass_enabled("pp_final_wipe_flip", WipeInAction);
		}
	);
	manager->insert(
		"pp_final_simple_blit_prepare",
		[pp_simple_blit_pass, framebuffer_manager](PassManager&, Rhi&)
		{
			Handle<Texture> color = framebuffer_manager->current_main_color();
			if (WipeInAction && !g_wipereverse)
			{
				// Non-reverse wipes are "fade-outs" from the previous frame.
				color = framebuffer_manager->previous_main_color();
			}
			pp_simple_blit_pass->set_texture(color, vid.width, vid.height);
			pp_simple_blit_pass
				->set_output(framebuffer_manager->current_post_color(), vid.width, vid.height, false, false);
		}
	);
	manager->insert("pp_final_simple_blit", pp_simple_blit_pass);
	manager->insert(
		"pp_final_simple_blit_flip",
		[framebuffer_manager](PassManager&, Rhi&) { framebuffer_manager->swap_post(); }
	);
	manager->insert(
		"pp_final_wipe_prepare",
		[pp_wipe_pass, framebuffer_manager](PassManager&, Rhi&)
		{
			pp_wipe_pass->set_source(framebuffer_manager->previous_post_color(), vid.width, vid.height);
			pp_wipe_pass->set_end(framebuffer_manager->current_main_color());
			pp_wipe_pass->set_target(framebuffer_manager->current_post_color(), vid.width, vid.height);
		}
	);
	manager->insert("pp_final_wipe", pp_wipe_pass);
	manager->insert(
		"pp_final_wipe_flip",
		[framebuffer_manager](PassManager&, Rhi&) { framebuffer_manager->swap_post(); }
	);

	manager->insert(
		"screenshot_prepare",
		[screenshot_pass, framebuffer_manager](PassManager&, Rhi&)
		{
			screenshot_pass->set_source(framebuffer_manager->previous_post_color(), vid.width, vid.height);
		}
	);
	manager->insert("screenshot", screenshot_pass);

	manager->insert(
		"final_composite_prepare",
		[final_composite_pass, framebuffer_manager](PassManager&, Rhi&)
		{
			final_composite_pass->set_texture(framebuffer_manager->previous_post_color(), vid.width, vid.height);
			final_composite_pass->set_output(kNullHandle, vid.realwidth, vid.realheight, true, false);
		}
	);
	manager->insert("final_composite", final_composite_pass);

	manager->insert("imgui", imgui_pass);

	manager->insert(
		"present",
		[](PassManager&, Rhi& rhi) {},
		[framebuffer_manager](PassManager&, Rhi& rhi)
		{
			rhi.present();
			rhi.finish();
			framebuffer_manager->reset_post();

			// TODO fix this: it's an ugly hack to work around issues with wipes
			// Why this works:
			// - Menus run F_RunWipe which is an inner update loop calling I_FinishUpdate, with this global set
			// - After exiting F_RunWipe, g_2d should normally be cleared by I_FinishUpdate
			// - Unfortunately, the menu has already run all its draw calls when exiting F_RunWipe
			// - That causes a single-frame flash of no 2d content, which is an epilepsy risk.
			// - By not clearing the 2d context, we are redrawing 2d every frame of the wipe
			// - This "works" because we draw 2d to the normal color buffer, not the postprocessed screen.
			// - It does result in the FPS counter being mangled during the wipe though.
			// - To fix the issues around wipes, wipes need to be a "sub" game state, and eliminate the inner tic loops.
			if (!WipeInAction)
			{
				g_2d = Twodee();
			}
		}
	);

	return manager;
}

void I_FinishUpdate(void)
{
	if (rendermode == render_none)
	{
		return;
	}

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		finish_legacy_ogl_update();
		return;
	}
#endif

	// TODO move this to srb2loop
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = vid.realwidth;
	io.DisplaySize.y = vid.realheight;
	ImGui::NewFrame();

	if (rhi_changed() || !g_passmanager)
	{
		g_passmanager = build_pass_manager();
	}

	rhi::Rhi* rhi = sys::get_rhi(sys::g_current_rhi);

	if (rhi == nullptr)
	{
		// ???
		return;
	}

	g_passmanager->render(*rhi);
}
