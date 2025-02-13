// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_SCREEN_CAPTURE_HPP__
#define __SRB2_HWR2_SCREEN_CAPTURE_HPP__

#include <cstddef>
#include <vector>

#include "../rhi/rhi.hpp"

namespace srb2::hwr2
{

class ScreenshotPass
{
	std::vector<uint8_t> pixel_data_;
	std::vector<uint8_t> packed_data_;
	uint32_t width_ = 0;
	uint32_t height_ = 0;

public:
	ScreenshotPass();
	~ScreenshotPass();

	void capture(rhi::Rhi& rhi);

	void set_source(uint32_t width, uint32_t height)
	{
		width_ = width;
		height_ = height;
	}
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_SCREEN_CAPTURE_HPP__
