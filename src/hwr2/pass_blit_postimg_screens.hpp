// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_BLIT_POSTIMG_SCREENS__
#define __SRB2_HWR2_PASS_BLIT_POSTIMG_SCREENS__

#include <array>
#include <cstdint>

#include <glm/vec2.hpp>

#include "../rhi/rhi.hpp"
#include "../doomdef.h"
#include "pass.hpp"
#include "pass_resource_managers.hpp"

namespace srb2::hwr2
{

class BlitPostimgScreens : public Pass
{
public:
	struct PostImgConfig
	{
		bool water;
		bool heat;
		bool flip;
		bool mirror;
	};

	struct ScreenConfig
	{
		rhi::Handle<rhi::Texture> source;
		bool indexed = false;
		glm::vec2 uv_offset {};
		glm::vec2 uv_size {};
		PostImgConfig post;
	};

private:
	struct ScreenData
	{
		rhi::Handle<rhi::Pipeline> pipeline;
		rhi::Handle<rhi::BindingSet> binding_set;
		rhi::Handle<rhi::UniformSet> uniform_set;
	};

	rhi::Handle<rhi::Pipeline> pipeline_;
	rhi::Handle<rhi::Pipeline> indexed_pipeline_;
	rhi::Handle<rhi::RenderPass> renderpass_;
	rhi::Handle<rhi::Buffer> quad_vbo_;
	rhi::Handle<rhi::Buffer> quad_ibo_;
	bool upload_quad_buffer_;

	uint32_t screens_;
	std::array<ScreenConfig, 4> screen_configs_;
	srb2::StaticVec<ScreenData, 4> screen_data_;
	rhi::Handle<rhi::Texture> target_;
	uint32_t target_width_;
	uint32_t target_height_;

	std::shared_ptr<MainPaletteManager> palette_mgr_;

public:
	BlitPostimgScreens(const std::shared_ptr<MainPaletteManager>& palette_mgr);
	virtual ~BlitPostimgScreens();

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	void set_num_screens(uint32_t screens) noexcept
	{
		SRB2_ASSERT(screens > 0 && screens <= MAXSPLITSCREENPLAYERS);
		screens_ = screens;
	}

	void set_screen(uint32_t screen_index, const ScreenConfig& config) noexcept
	{
		SRB2_ASSERT(screen_index < MAXSPLITSCREENPLAYERS);
		screen_configs_[screen_index] = config;
	}

	void set_target(rhi::Handle<rhi::Texture> target, uint32_t width, uint32_t height) noexcept
	{
		target_ = target;
		target_width_ = width;
		target_height_ = height;
	}
};

}; // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_BLIT_POSTIMG_SCREENS__
