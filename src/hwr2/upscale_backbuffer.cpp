// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "upscale_backbuffer.hpp"

#include "../i_video.h"

using namespace srb2;
using namespace srb2::rhi;
using namespace srb2::hwr2;

UpscaleBackbuffer::UpscaleBackbuffer() = default;
UpscaleBackbuffer::UpscaleBackbuffer(UpscaleBackbuffer&&) = default;
UpscaleBackbuffer::~UpscaleBackbuffer() = default;
UpscaleBackbuffer& UpscaleBackbuffer::operator=(UpscaleBackbuffer&&) = default;

static bool size_equal(Rhi& rhi, Handle<Texture> tex, uint32_t width, uint32_t height)
{
	TextureDetails deets = rhi.get_texture_details(tex);
	return deets.width == width && deets.height == height;
}

void UpscaleBackbuffer::begin_pass(Rhi& rhi)
{
	uint32_t vid_width = static_cast<uint32_t>(vid.width);
	uint32_t vid_height = static_cast<uint32_t>(vid.height);

	bool remake = false;
	if (!color_ || !size_equal(rhi, color_, vid_width, vid_height))
	{
		remake = true;
	}

	if (remake)
	{
		if (color_)
		{
			rhi.destroy_texture(color_);
			color_ = kNullHandle;
		}

		TextureDesc color_tex {};
		color_tex.format = TextureFormat::kRGBA;
		color_tex.width = vid_width;
		color_tex.height = vid_height;
		color_tex.u_wrap = TextureWrapMode::kClamp;
		color_tex.v_wrap = TextureWrapMode::kClamp;
		color_ = rhi.create_texture(color_tex);
	}

	RenderPassBeginInfo begin_info {};
	begin_info.clear_color = {0, 0, 0, 1};
	begin_info.color_attachment = color_;
	begin_info.color_load_op = rhi::AttachmentLoadOp::kLoad;
	begin_info.color_store_op = rhi::AttachmentStoreOp::kStore;
	begin_info.depth_load_op = rhi::AttachmentLoadOp::kLoad;
	begin_info.depth_store_op = rhi::AttachmentStoreOp::kStore;
	begin_info.stencil_load_op = rhi::AttachmentLoadOp::kLoad;
	begin_info.stencil_store_op = rhi::AttachmentStoreOp::kStore;
	rhi.push_render_pass(begin_info);
}
