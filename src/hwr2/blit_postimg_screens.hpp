// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
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
#include "resource_management.hpp"

namespace srb2::hwr2
{

class BlitPostimgScreens
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
		rhi::Handle<rhi::Program> program;
	};

	rhi::Handle<rhi::Program> program_;
	rhi::Handle<rhi::Program> indexed_program_;
	rhi::Handle<rhi::Buffer> quad_vbo_;
	rhi::Handle<rhi::Buffer> quad_ibo_;
	bool upload_quad_buffer_;

	uint32_t screens_;
	std::array<ScreenConfig, 4> screen_configs_;
	srb2::StaticVec<ScreenData, 4> screen_data_;
	uint32_t target_width_;
	uint32_t target_height_;

	PaletteManager* palette_mgr_;

	void prepass(rhi::Rhi& rhi);
	void transfer(rhi::Rhi& rhi);

public:
	explicit BlitPostimgScreens(PaletteManager* palette_mgr);

	void draw(rhi::Rhi& rhi);

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

	void set_target(uint32_t width, uint32_t height) noexcept
	{
		target_width_ = width;
		target_height_ = height;
	}
};

}; // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_BLIT_POSTIMG_SCREENS__
