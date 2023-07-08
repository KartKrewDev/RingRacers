// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "pass_screenshot.hpp"

#include <tcb/span.hpp>

#include "../m_misc.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

ScreenshotPass::ScreenshotPass() = default;
ScreenshotPass::~ScreenshotPass() = default;

void ScreenshotPass::prepass(Rhi& rhi)
{
	if (!render_pass_)
	{
		render_pass_ = rhi.create_render_pass(
			{
				false,
				AttachmentLoadOp::kLoad,
				AttachmentStoreOp::kStore,
				AttachmentLoadOp::kDontCare,
				AttachmentStoreOp::kDontCare,
				AttachmentLoadOp::kDontCare,
				AttachmentStoreOp::kDontCare
			}
		);
	}

	doing_screenshot_ = takescreenshot || moviemode != MM_OFF;
}

void ScreenshotPass::transfer(Rhi& rhi, Handle<GraphicsContext> ctx)
{
}

void ScreenshotPass::graphics(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	if (!doing_screenshot_)
	{
		return;
	}

	pixel_data_.clear();
	pixel_data_.resize(width_ * height_ * 3); // 3 bytes per pixel for RGB8

	rhi.begin_render_pass(ctx, {render_pass_, source_, std::nullopt, {0.f, 0.f, 0.f, 0.f}});
	rhi.read_pixels(ctx, {0, 0, width_, height_}, PixelFormat::kRGB8, tcb::as_writable_bytes(tcb::span(pixel_data_)));
	rhi.end_render_pass(ctx);
}

void ScreenshotPass::postpass(Rhi& rhi)
{
	if (!doing_screenshot_)
	{
		return;
	}

	if (takescreenshot)
	{
		M_DoScreenShot(width_, height_, tcb::as_bytes(tcb::span(pixel_data_)));
	}

	if (moviemode != MM_OFF)
	{
		M_SaveFrame(width_, height_, tcb::as_bytes(tcb::span(pixel_data_)));
	}

	doing_screenshot_ = false;
}
