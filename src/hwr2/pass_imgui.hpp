// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_IMGUI_HPP__
#define __SRB2_HWR2_PASS_IMGUI_HPP__

#include <vector>

#include "../rhi/rhi.hpp"
#include "pass.hpp"

namespace srb2::hwr2
{

class ImguiPass final : public Pass
{
	struct DrawCmd
	{
		rhi::Handle<rhi::Texture> tex;
		uint32_t v_offset;
		uint32_t elems;
		uint32_t i_offset;
		rhi::Rect clip;
		rhi::Handle<rhi::BindingSet> binding_set;
	};
	struct DrawList
	{
		void* list;
		rhi::Handle<rhi::Buffer> vbo;
		rhi::Handle<rhi::Buffer> ibo;
		rhi::Handle<rhi::UniformSet> us_1;
		rhi::Handle<rhi::UniformSet> us_2;
		std::vector<DrawCmd> cmds;
	};

	rhi::Handle<rhi::Pipeline> pipeline_;
	rhi::Handle<rhi::Texture> font_atlas_;

	std::vector<DrawList> draw_lists_;

public:
	ImguiPass();
	virtual ~ImguiPass();

	virtual void prepass(rhi::Rhi& rhi) override;

	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;

	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;

	virtual void postpass(rhi::Rhi& rhi) override;
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_IMGUI_HPP__
