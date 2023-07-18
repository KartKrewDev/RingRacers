// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_POSTPROCESS_HPP__
#define __SRB2_HWR2_PASS_POSTPROCESS_HPP__

#include "pass.hpp"

#include <vector>

namespace srb2::hwr2
{

class PostprocessWipePass final : public Pass
{
	// Internal RHI resources
	rhi::Handle<rhi::RenderPass> render_pass_;
	rhi::Handle<rhi::Pipeline> pipeline_;
	rhi::Handle<rhi::Buffer> vbo_;
	bool upload_vbo_ = false;
	rhi::Handle<rhi::Buffer> ibo_;
	bool upload_ibo_ = false;
	rhi::Handle<rhi::UniformSet> us_;
	rhi::Handle<rhi::BindingSet> bs_;
	rhi::Handle<rhi::Texture> wipe_tex_;
	int wipe_color_mode_ = 0;
	int wipe_swizzle_ = 0;

	// Pass parameters
	rhi::Handle<rhi::Texture> start_;
	rhi::Handle<rhi::Texture> end_;
	rhi::Handle<rhi::Texture> target_;
	uint32_t target_w_ = 0;
	uint32_t target_h_ = 0;

	// Mask lump loading
	std::vector<uint8_t> mask_data_;
	uint32_t mask_w_ = 0;
	uint32_t mask_h_ = 0;

public:
	PostprocessWipePass();
	virtual ~PostprocessWipePass();

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	void set_start(rhi::Handle<rhi::Texture> start) noexcept { start_ = start; }

	void set_end(rhi::Handle<rhi::Texture> end) noexcept { end_ = end; }

	void set_target(rhi::Handle<rhi::Texture> target, uint32_t width, uint32_t height) noexcept
	{
		target_ = target;
		target_w_ = width;
		target_h_ = height;
	}
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_POSTPROCESS_HPP__
