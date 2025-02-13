// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_HARDWARE_STATE_HPP__
#define __SRB2_HWR2_HARDWARE_STATE_HPP__

#include "blit_postimg_screens.hpp"
#include "blit_rect.hpp"
#include "imgui_renderer.hpp"
#include "postprocess_wipe.hpp"
#include "resource_management.hpp"
#include "screen_capture.hpp"
#include "software_screen_renderer.hpp"
#include "twodee_renderer.hpp"
#include "upscale_backbuffer.hpp"

namespace srb2::hwr2
{

struct WipeFrames
{
	rhi::Handle<rhi::Texture> start;
	rhi::Handle<rhi::Texture> end;
};

struct HardwareState
{
	std::unique_ptr<PaletteManager> palette_manager;
	std::unique_ptr<FlatTextureManager> flat_manager;
	std::unique_ptr<PatchAtlasCache> patch_atlas_cache;
	std::unique_ptr<TwodeeRenderer> twodee_renderer;
	std::unique_ptr<SoftwareScreenRenderer> software_screen_renderer;
	std::unique_ptr<BlitPostimgScreens> blit_postimg_screens;
	std::unique_ptr<PostprocessWipePass> wipe;
	std::unique_ptr<BlitRectPass> blit_rect;
	std::unique_ptr<BlitRectPass> sharp_bilinear_blit_rect;
	std::unique_ptr<BlitRectPass> crt_blit_rect;
	std::unique_ptr<BlitRectPass> crtsharp_blit_rect;
	std::unique_ptr<ScreenshotPass> screen_capture;
	std::unique_ptr<UpscaleBackbuffer> backbuffer;
	std::unique_ptr<ImguiRenderer> imgui_renderer;
	WipeFrames wipe_frames;
};

} // srb2::hwr2

#endif // __SRB2_HWR2_HARDWARE_STATE_HPP__
