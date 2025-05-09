// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_SOFTWARE_SCREEN_RENDERER_HPP_
#define __SRB2_HWR2_SOFTWARE_SCREEN_RENDERER_HPP_

#include <cstddef>
#include <vector>

#include "../rhi/rhi.hpp"

namespace srb2::hwr2
{

/// @brief Renders software player views in prepass and uploads the result to a texture in transfer.
class SoftwareScreenRenderer final
{
	rhi::Handle<rhi::Texture> screen_texture_;
	uint32_t width_ = 0;
	uint32_t height_ = 0;

	// Used to ensure the row spans are aligned on the unpack boundary for weird resolutions
	// Any resolution with a width divisible by 4 doesn't need this, but e.g. 1366x768 needs the intermediary copy
	std::vector<uint8_t> copy_buffer_;

public:
	SoftwareScreenRenderer();
	~SoftwareScreenRenderer();

	void draw(rhi::Rhi& rhi);

	rhi::Handle<rhi::Texture> screen() const { return screen_texture_; }
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_SOFTWARE_SCREEN_RENDERER_HPP_
