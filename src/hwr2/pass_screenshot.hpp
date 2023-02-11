// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_SCREENSHOT_HPP__
#define __SRB2_HWR2_PASS_SCREENSHOT_HPP__

#include <cstddef>
#include <vector>

#include "pass.hpp"

namespace srb2::hwr2
{

class ScreenshotPass : public Pass
{
	bool doing_screenshot_ = false;
	rhi::Handle<rhi::Texture> source_;
	rhi::Handle<rhi::RenderPass> render_pass_;
	std::vector<uint8_t> pixel_data_;
	uint32_t width_ = 0;
	uint32_t height_ = 0;

public:
	ScreenshotPass();
	virtual ~ScreenshotPass();

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	void set_source(rhi::Handle<rhi::Texture> source, uint32_t width, uint32_t height)
	{
		source_ = source;
		width_ = width;
		height_ = height;
	}
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_SCREENSHOT_HPP__
