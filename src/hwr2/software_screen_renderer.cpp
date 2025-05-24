// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "software_screen_renderer.hpp"

#include "../i_video.h"
#include "../v_video.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

SoftwareScreenRenderer::SoftwareScreenRenderer() = default;
SoftwareScreenRenderer::~SoftwareScreenRenderer() = default;

void SoftwareScreenRenderer::draw(Rhi& rhi)
{
	// Render the player views... or not yet? Needs to be moved out of D_Display in d_main.c
	// Assume it's already been done and vid.buffer contains the composited splitscreen view.
	// In the future though, we will want to treat each player viewport separately for postprocessing.

	// Prepare RHI resources
	if (screen_texture_ && (static_cast<int32_t>(width_) != vid.width || static_cast<int32_t>(height_) != vid.height))
	{
		// Mode changed, recreate texture
		rhi.destroy_texture(screen_texture_);
		screen_texture_ = kNullHandle;
	}

	width_ = vid.width;
	height_ = vid.height;

	if (!screen_texture_)
	{
		screen_texture_ = rhi.create_texture({
			TextureFormat::kLuminance,
			width_,
			height_,
			TextureWrapMode::kClamp,
			TextureWrapMode::kClamp
		});
	}

	// If the screen width won't fit the unpack alignment, we need to copy the screen.
	if (width_ % kPixelRowUnpackAlignment > 0)
	{
		std::size_t padded_width = (width_ + (kPixelRowUnpackAlignment - 1)) & ~(kPixelRowUnpackAlignment - 1);
		copy_buffer_.clear();
		copy_buffer_.resize(padded_width * height_, 0);
		for (std::size_t y = 0; y < height_; y++)
		{
			std::copy(&vid.buffer[width_ * y], &vid.buffer[width_ * (y + 1)], &copy_buffer_[padded_width * y]);
		}
	}

	// Upload screen
	tcb::span<const std::byte> screen_span;
	if (width_ % kPixelRowUnpackAlignment > 0)
	{
		screen_span = tcb::as_bytes(tcb::span(copy_buffer_));
	}
	else
	{
		screen_span = tcb::as_bytes(tcb::span(vid.buffer, width_ * height_));
	}

	rhi.update_texture(screen_texture_, {0, 0, width_, height_}, PixelFormat::kR8, screen_span);
}
