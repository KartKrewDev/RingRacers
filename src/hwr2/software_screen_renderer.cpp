// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
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

void SoftwareScreenRenderer::draw(Rhi& rhi, Handle<GraphicsContext> ctx)
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
		std::size_t padded_width = (width_ + (kPixelRowUnpackAlignment - 1)) & !kPixelRowUnpackAlignment;
		copy_buffer_.clear();
		copy_buffer_.reserve(padded_width * height_);
		for (std::size_t y = 0; y < height_; y++)
		{
			for (std::size_t x = 0; x < width_; x++)
			{
				copy_buffer_.push_back(vid.buffer[(width_ * y) + x]);
			}

			// Padding to unpack alignment
			for (std::size_t i = 0; i < padded_width - width_; i++)
			{
				copy_buffer_.push_back(0);
			}
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

	rhi.update_texture(ctx, screen_texture_, {0, 0, width_, height_}, PixelFormat::kR8, screen_span);
}
