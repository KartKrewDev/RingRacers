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

// Legacy FinishUpdate Draws
#include "d_netcmd.h"
#ifdef HAVE_DISCORDRPC
#include "discord.h"
#endif
#include "doomstat.h"
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
#include "m_avrecorder.h"
#endif
#include "st_stuff.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

namespace
{
struct InternalPassData
{
	std::shared_ptr<PassManager> resource_passmanager;
	std::shared_ptr<PassManager> normal_rendering;
	std::shared_ptr<PassManager> wipe_capture_start_rendering;
	std::shared_ptr<PassManager> wipe_capture_end_rendering;
	std::shared_ptr<PassManager> wipe_rendering;
};
} // namespace

static std::unique_ptr<InternalPassData> g_passes;
static Rhi* g_last_known_rhi = nullptr;
static bool g_imgui_frame_active = false;

Handle<Rhi> srb2::sys::g_current_rhi = kNullHandle;

static bool rhi_changed(Rhi* rhi)
{
	return g_last_known_rhi != rhi;
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

	ST_drawDebugInfo();

	OglSdlFinishUpdate(cv_vidwait.value);
}
#endif

static void temp_legacy_finishupdate_draws()
{
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
				for (int player = 1; player < MAXPLAYERS; player++)
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
#ifdef SRB2_CONFIG_ENABLE_WEBM_MOVIES
		M_AVRecorder_DrawFrameRate();
#endif
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

	ST_drawDebugInfo();
}

static InternalPassData build_pass_manager()
{
	auto framebuffer_manager = std::make_shared<FramebufferManager>();
	auto palette_manager = std::make_shared<MainPaletteManager>();
	auto common_resources_manager = std::make_shared<CommonResourcesManager>();
	auto flat_texture_manager = std::make_shared<FlatTextureManager>();
	auto resource_manager = std::make_shared<PassManager>();

	resource_manager->insert("framebuffer_manager", framebuffer_manager);
	resource_manager->insert("palette_manager", palette_manager);
	resource_manager->insert("common_resources_manager", common_resources_manager);
	resource_manager->insert("flat_texture_manager", flat_texture_manager);

	// Basic Rendering is responsible for drawing 3d, 2d, and postprocessing the image.
	// This is drawn to an alternating internal color buffer.
	// Normal Rendering will output the result via final composite and present.
	// Wipe Start Screen and Wipe End Screen will save to special color buffers used for Wipe Rendering.
	auto basic_rendering = std::make_shared<PassManager>();

	auto software_pass = std::make_shared<SoftwarePass>();
	auto blit_sw_pass = std::make_shared<BlitRectPass>(palette_manager, true);
	auto twodee = std::make_shared<TwodeePass>();
	twodee->flat_manager_ = flat_texture_manager;
	twodee->data_ = make_twodee_pass_data();
	twodee->ctx_ = &g_2d;
	auto pp_simple_blit_pass = std::make_shared<BlitRectPass>(false);
	auto screenshot_pass = std::make_shared<ScreenshotPass>();
	auto imgui_pass = std::make_shared<ImguiPass>();
	auto final_composite_pass = std::make_shared<BlitRectPass>(true);

	basic_rendering->insert(
		"3d_prepare",
		[framebuffer_manager](PassManager& mgr, Rhi&)
		{
			const bool sw_enabled = rendermode == render_soft && gamestate != GS_NULL;

			mgr.set_pass_enabled("software", sw_enabled);
			mgr.set_pass_enabled("blit_sw_prepare", sw_enabled);
			mgr.set_pass_enabled("blit_sw", sw_enabled && !g_wipeskiprender);
		}
	);
	basic_rendering->insert("software", software_pass);
	basic_rendering->insert(
		"blit_sw_prepare",
		[blit_sw_pass, software_pass, framebuffer_manager](PassManager&, Rhi&)
		{
			blit_sw_pass->set_texture(software_pass->screen_texture(), vid.width, vid.height);
			blit_sw_pass->set_output(framebuffer_manager->main_color(), vid.width, vid.height, false, false);
		}
	);
	basic_rendering->insert("blit_sw", blit_sw_pass);

	basic_rendering->insert(
		"2d_prepare",
		[twodee, framebuffer_manager, palette_manager](PassManager& mgr, Rhi&)
		{
			twodee->output_ = framebuffer_manager->main_color();
			twodee->palette_manager_ = palette_manager;
			twodee->output_width_ = vid.width;
			twodee->output_height_ = vid.height;
		}
	);
	basic_rendering->insert("2d", twodee);

	basic_rendering->insert(
		"pp_final_simple_blit_prepare",
		[pp_simple_blit_pass, framebuffer_manager](PassManager&, Rhi&)
		{
			framebuffer_manager->swap_post();
			pp_simple_blit_pass->set_texture(framebuffer_manager->main_color(), vid.width, vid.height);
			pp_simple_blit_pass
				->set_output(framebuffer_manager->current_post_color(), vid.width, vid.height, false, false);
		}
	);
	basic_rendering->insert("pp_final_simple_blit", pp_simple_blit_pass);

	auto screenshot_rendering = std::make_shared<PassManager>();

	screenshot_rendering->insert(
		"screenshot_prepare",
		[screenshot_pass, framebuffer_manager](PassManager&, Rhi&)
		{
			screenshot_pass->set_source(framebuffer_manager->current_post_color(), vid.width, vid.height);
		}
	);
	screenshot_rendering->insert("screenshot", screenshot_pass);

	// Composite-present takes the current postprocess result and outputs it to the default framebuffer.
	// It also renders imgui and presents the screen.
	auto composite_present_rendering = std::make_shared<PassManager>();

	composite_present_rendering->insert(
		"final_composite_prepare",
		[final_composite_pass, framebuffer_manager](PassManager&, Rhi&)
		{
			final_composite_pass->set_texture(framebuffer_manager->current_post_color(), vid.width, vid.height);
			final_composite_pass->set_output(kNullHandle, vid.realwidth, vid.realheight, true, false);
		}
	);
	composite_present_rendering->insert("final_composite", final_composite_pass);
	composite_present_rendering->insert("imgui", imgui_pass);
	composite_present_rendering->insert(
		"present",
		[](PassManager&, Rhi& rhi) {},
		[framebuffer_manager](PassManager&, Rhi& rhi)
		{
			g_imgui_frame_active = false;
			rhi.present();
			rhi.finish();
			framebuffer_manager->reset_post();
			I_NewImguiFrame();
		}
	);

	// Normal rendering combines basic rendering and composite-present.
	auto normal_rendering = std::make_shared<PassManager>();

	normal_rendering->insert("resource_manager", resource_manager);
	normal_rendering->insert("basic_rendering", basic_rendering);
	normal_rendering->insert("screenshot_rendering", screenshot_rendering);
	normal_rendering->insert("composite_present_rendering", composite_present_rendering);

	// Wipe Start Screen Capture rendering
	auto wipe_capture_start_rendering = std::make_shared<PassManager>();
	auto wipe_start_blit = std::make_shared<BlitRectPass>();

	wipe_capture_start_rendering->insert("resource_manager", resource_manager);
	wipe_capture_start_rendering->insert("basic_rendering", basic_rendering);
	wipe_capture_start_rendering->insert(
		"wipe_capture_prepare",
		[framebuffer_manager, wipe_start_blit](PassManager&, Rhi&)
		{
			wipe_start_blit->set_texture(framebuffer_manager->previous_post_color(), vid.width, vid.height);
			wipe_start_blit->set_output(framebuffer_manager->wipe_start_color(), vid.width, vid.height, false, true);
		}
	);
	wipe_capture_start_rendering->insert("wipe_capture", wipe_start_blit);

	// Wipe End Screen Capture rendering
	auto wipe_capture_end_rendering = std::make_shared<PassManager>();
	auto wipe_end_blit = std::make_shared<BlitRectPass>();
	auto wipe_end_blit_start_to_main = std::make_shared<BlitRectPass>();

	wipe_capture_end_rendering->insert("resource_manager", resource_manager);
	wipe_capture_end_rendering->insert("basic_rendering", basic_rendering);
	wipe_capture_end_rendering->insert(
		"wipe_capture_prepare",
		[framebuffer_manager, wipe_end_blit, wipe_end_blit_start_to_main](PassManager&, Rhi&)
		{
			wipe_end_blit->set_texture(framebuffer_manager->current_post_color(), vid.width, vid.height);
			wipe_end_blit->set_output(framebuffer_manager->wipe_end_color(), vid.width, vid.height, false, true);

			wipe_end_blit_start_to_main->set_texture(
				framebuffer_manager->wipe_start_color(),
				vid.width,
				vid.height
			);
			wipe_end_blit_start_to_main->set_output(
				framebuffer_manager->main_color(),
				vid.width,
				vid.height,
				false,
				true
			);
		}
	);
	wipe_capture_end_rendering->insert("wipe_capture", wipe_end_blit);
	wipe_capture_end_rendering->insert("wipe_end_blit_start_to_main", wipe_end_blit_start_to_main);

	// Wipe rendering only runs the wipe shader on the start and end screens, and adds composite-present.
	auto wipe_rendering = std::make_shared<PassManager>();

	auto pp_wipe_pass = std::make_shared<PostprocessWipePass>();

	wipe_rendering->insert("resource_manager", resource_manager);
	wipe_rendering->insert(
		"pp_final_wipe_prepare",
		[pp_wipe_pass, framebuffer_manager, common_resources_manager](PassManager&, Rhi&)
		{
			framebuffer_manager->swap_post();
			Handle<Texture> start = framebuffer_manager->main_color();
			Handle<Texture> end = framebuffer_manager->wipe_end_color();
			if (g_wipereverse)
			{
				std::swap(start, end);
			}
			pp_wipe_pass->set_start(start);
			pp_wipe_pass->set_end(end);
			pp_wipe_pass->set_target(framebuffer_manager->current_post_color(), vid.width, vid.height);
		}
	);
	wipe_rendering->insert("pp_final_wipe", pp_wipe_pass);
	wipe_rendering->insert("screenshot_rendering", screenshot_rendering);
	wipe_rendering->insert("composite_present_rendering", composite_present_rendering);

	InternalPassData ret;
	ret.resource_passmanager = resource_manager;
	ret.normal_rendering = normal_rendering;
	ret.wipe_capture_start_rendering = wipe_capture_start_rendering;
	ret.wipe_capture_end_rendering = wipe_capture_end_rendering;
	ret.wipe_rendering = wipe_rendering;

	return ret;
}

void I_NewTwodeeFrame(void)
{
	g_2d = Twodee();
}

void I_NewImguiFrame(void)
{
	if (g_imgui_frame_active)
	{
		ImGui::EndFrame();
		g_imgui_frame_active = false;
	}
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = vid.realwidth;
	io.DisplaySize.y = vid.realheight;
	ImGui::NewFrame();
	g_imgui_frame_active = true;
}

static void maybe_reinit_passes(Rhi* rhi)
{
	if (rhi_changed(rhi) || !g_passes)
	{
		g_last_known_rhi = rhi;
		g_passes = std::make_unique<InternalPassData>(build_pass_manager());
	}
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

	temp_legacy_finishupdate_draws();

	rhi::Rhi* rhi = sys::get_rhi(sys::g_current_rhi);

	if (rhi == nullptr)
	{
		// ???
		return;
	}

	maybe_reinit_passes(rhi);

	g_passes->normal_rendering->render(*rhi);
}

void I_FinishUpdateWipeStartScreen(void)
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

	temp_legacy_finishupdate_draws();

	rhi::Rhi* rhi = sys::get_rhi(sys::g_current_rhi);

	if (rhi == nullptr)
	{
		// ???
		return;
	}

	maybe_reinit_passes(rhi);

	g_passes->wipe_capture_start_rendering->render(*rhi);
	I_NewImguiFrame();
}

void I_FinishUpdateWipeEndScreen(void)
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

	temp_legacy_finishupdate_draws();

	rhi::Rhi* rhi = sys::get_rhi(sys::g_current_rhi);

	if (rhi == nullptr)
	{
		// ???
		return;
	}

	maybe_reinit_passes(rhi);

	g_passes->wipe_capture_end_rendering->render(*rhi);
	I_NewImguiFrame();
}

void I_FinishUpdateWipe(void)
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

	temp_legacy_finishupdate_draws();

	rhi::Rhi* rhi = sys::get_rhi(sys::g_current_rhi);

	if (rhi == nullptr)
	{
		// ???
		return;
	}

	maybe_reinit_passes(rhi);

	g_passes->wipe_rendering->render(*rhi);
}
