// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_IMGUI_HPP__
#define __SRB2_HWR2_PASS_IMGUI_HPP__

#include <vector>

#include "../rhi/rhi.hpp"

namespace srb2::hwr2
{

class ImguiRenderer final
{
	struct DrawCmd
	{
		uint32_t v_offset;
		uint32_t elems;
		uint32_t i_offset;
		rhi::Rect clip;
		rhi::Handle<rhi::Buffer> vbo;
		rhi::Handle<rhi::Buffer> ibo;
		rhi::Handle<rhi::Texture> tex;
	};
	struct DrawList
	{
		void* list;
		rhi::Handle<rhi::Buffer> vbo;
		rhi::Handle<rhi::Buffer> ibo;
		std::vector<DrawCmd> cmds;
	};

	rhi::Handle<rhi::Program> program_;
	rhi::Handle<rhi::Texture> font_atlas_;
	rhi::Handle<rhi::Texture> default_tex_;

	std::vector<DrawList> draw_lists_;

public:
	ImguiRenderer();
	virtual ~ImguiRenderer();

	void render(rhi::Rhi& rhi);
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_IMGUI_HPP__
