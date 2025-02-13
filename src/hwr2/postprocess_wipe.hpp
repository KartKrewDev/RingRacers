// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_POSTPROCESS_WIPE_HPP__
#define __SRB2_HWR2_POSTPROCESS_WIPE_HPP__

#include <vector>

#include "../rhi/rhi.hpp"

namespace srb2::hwr2
{

class PostprocessWipePass final
{
	// Internal RHI resources
	rhi::Handle<rhi::Program> program_;
	rhi::Handle<rhi::Buffer> vbo_;
	bool upload_vbo_ = false;
	rhi::Handle<rhi::Buffer> ibo_;
	bool upload_ibo_ = false;
	rhi::Handle<rhi::Texture> wipe_tex_;
	int wipe_color_mode_ = 0;
	int wipe_swizzle_ = 0;

	// Pass parameters
	rhi::Handle<rhi::Texture> start_;
	rhi::Handle<rhi::Texture> end_;
	uint32_t width_;
	uint32_t height_;

	// Mask lump loading
	std::vector<uint8_t> mask_data_;
	uint32_t mask_w_ = 0;
	uint32_t mask_h_ = 0;

	void prepass(rhi::Rhi& rhi);
	void transfer(rhi::Rhi& rhi);
	void graphics(rhi::Rhi& rhi);
	void postpass(rhi::Rhi& rhi);

public:
	PostprocessWipePass();
	virtual ~PostprocessWipePass();

	void draw(rhi::Rhi& rhi);

	void set_start(rhi::Handle<rhi::Texture> start) noexcept { start_ = start; }

	void set_end(rhi::Handle<rhi::Texture> end) noexcept { end_ = end; }

	void set_target_size(uint32_t width, uint32_t height) noexcept
	{
		width_ = width;
		height_ = height;
	}
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_POSTPROCESS_WIPE_HPP__
