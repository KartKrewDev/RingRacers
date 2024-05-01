// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
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

void UpscaleBackbuffer::begin_pass(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	uint32_t vid_width = static_cast<uint32_t>(vid.width);
	uint32_t vid_height = static_cast<uint32_t>(vid.height);

	bool remake = false;
	if (!color_ || !size_equal(rhi, color_, vid_width, vid_height))
	{
		remake = true;
	}

	auto new_renderpass = [&rhi = rhi](AttachmentLoadOp load_op, AttachmentStoreOp store_op)
	{
		RenderPassDesc desc {};
		desc.use_depth_stencil = false;
		desc.color_load_op = load_op;
		desc.color_store_op = store_op;
		desc.depth_load_op = load_op;
		desc.depth_store_op = store_op;
		desc.stencil_load_op = load_op;
		desc.stencil_store_op = store_op;
		return rhi.create_render_pass(desc);
	};

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

		RenderbufferDesc depth_tex {};
		depth_tex.width = vid_width;
		depth_tex.height = vid_height;

		if (!renderpass_clear_)
		{
			renderpass_clear_ = new_renderpass(AttachmentLoadOp::kClear, AttachmentStoreOp::kStore);
		}
	}
	else
	{
		if (!renderpass_)
		{
			renderpass_ = new_renderpass(AttachmentLoadOp::kLoad, AttachmentStoreOp::kStore);
		}
	}

	RenderPassBeginInfo begin_info {};
	begin_info.render_pass = remake ? renderpass_clear_ : renderpass_;
	begin_info.clear_color = {0, 0, 0, 1};
	begin_info.color_attachment = color_;
	rhi.begin_render_pass(ctx, begin_info);
}
