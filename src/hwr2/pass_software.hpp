// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_SOFTWARE_HPP_
#define __SRB2_HWR2_PASS_SOFTWARE_HPP_

#include <cstddef>
#include <vector>

#include "pass.hpp"

namespace srb2::hwr2
{

/// @brief Renders software player views in prepass and uploads the result to a texture in transfer.
class SoftwarePass final : public Pass
{
	rhi::Handle<rhi::Texture> screen_texture_;
	uint32_t width_ = 0;
	uint32_t height_ = 0;

	// Used to ensure the row spans are aligned on the unpack boundary for weird resolutions
	// Any resolution with a width divisible by 4 doesn't need this, but e.g. 1366x768 needs the intermediary copy
	std::vector<uint8_t> copy_buffer_;

public:
	SoftwarePass();
	virtual ~SoftwarePass();

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	rhi::Handle<rhi::Texture> screen_texture() const noexcept { return screen_texture_; }
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_SOFTWARE_HPP_
