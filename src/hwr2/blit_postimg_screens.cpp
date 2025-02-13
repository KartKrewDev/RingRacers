// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "blit_postimg_screens.hpp"

#include <cstddef>

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tcb/span.hpp>

#include "../p_tick.h"
#include "../i_time.h"
#include "../screen.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

namespace
{
struct BlitVertex
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float u = 0.f;
	float v = 0.f;
};
} // namespace

static const BlitVertex kVerts[] =
	{{-.5f, -.5f, 0.f, 0.f, 0.f}, {.5f, -.5f, 0.f, 1.f, 0.f}, {-.5f, .5f, 0.f, 0.f, 1.f}, {.5f, .5f, 0.f, 1.f, 1.f}};

static const uint16_t kIndices[] = {0, 1, 2, 1, 3, 2};


static Rect get_screen_viewport(uint32_t screen, uint32_t screens, uint32_t w, uint32_t h)
{
	switch (screens)
	{
	case 1:
		return {0, 0, w, h};
	case 2:
		return {0, screen == 0 ? (static_cast<int32_t>(h) / 2) : 0, w, (h / 2)};
	default:
		switch (screen)
		{
		case 2:
			return {0, 0, w / 2, h / 2};
		case 3:
			return {static_cast<int32_t>(w) / 2, 0, w / 2, h / 2};
		case 0:
			return {0, static_cast<int32_t>(h) / 2, w / 2, h / 2};
		case 1:
			return {static_cast<int32_t>(w) / 2, static_cast<int32_t>(h) / 2, w / 2, h / 2};
		}
	}
	return {0, 0, w, h};
}

BlitPostimgScreens::BlitPostimgScreens(PaletteManager* palette_mgr)
	: palette_mgr_(palette_mgr)
{
}

void BlitPostimgScreens::draw(Rhi& rhi)
{
	prepass(rhi);
	transfer(rhi);

	for (uint32_t i = 0; i < screens_; i++)
	{
		BlitPostimgScreens::ScreenConfig& screen_config = screen_configs_[i];
		BlitPostimgScreens::ScreenData& data = screen_data_[i];

		glm::mat4 projection = glm::scale(glm::identity<glm::mat4>(), glm::vec3(2.f, -2.f, 1.f));
		glm::mat4 modelview = glm::identity<glm::mat4>();

		glm::vec2 flip_mirror_uv_displace {0.0, 0.0};
		if (screen_config.post.mirror)
		{
			flip_mirror_uv_displace.x = 1 - (1 - screen_config.uv_size.x);
		}
		if (screen_config.post.flip)
		{
			flip_mirror_uv_displace.y = 1 - (1 - screen_config.uv_size.y);
		}

		glm::mat3 texcoord_transform =
		{
			glm::vec3(screen_config.uv_size.x * (screen_config.post.mirror ? -1 : 1), 0.0, 0.0),
			glm::vec3(0.0, screen_config.uv_size.y * (screen_config.post.flip ? -1 : 1), 0.0),
			glm::vec3(screen_config.uv_offset + flip_mirror_uv_displace, 1.0)
		};

		glm::vec2 texcoord_min = screen_config.uv_offset;
		glm::vec2 texcoord_max = screen_config.uv_offset + screen_config.uv_size;

		RasterizerStateDesc r_state {};
		r_state.cull = CullMode::kNone;

		rhi.bind_program(data.program);
		rhi.set_rasterizer_state(r_state);
		rhi.set_viewport(get_screen_viewport(i, screens_, target_width_, target_height_));
		rhi.bind_vertex_attrib("a_position", quad_vbo_, rhi::VertexAttributeFormat::kFloat3, offsetof(BlitVertex, x), sizeof(BlitVertex));
		rhi.bind_vertex_attrib("a_texcoord0", quad_vbo_, rhi::VertexAttributeFormat::kFloat2, offsetof(BlitVertex, u), sizeof(BlitVertex));
		rhi.bind_index_buffer(quad_ibo_);
		rhi.set_uniform("u_time", static_cast<float>(leveltime));
		rhi.set_uniform("u_projection", projection);
		rhi.set_uniform("u_modelview", modelview);
		rhi.set_uniform("u_texcoord0_transform", texcoord_transform);
		rhi.set_uniform("u_texcoord0_min", texcoord_min);
		rhi.set_uniform("u_texcoord0_max", texcoord_max);
		rhi.set_uniform("u_postimg_water", screen_config.post.water);
		rhi.set_uniform("u_postimg_heat", screen_config.post.heat);
		rhi.set_sampler("s_sampler0", 0, screen_config.source);
		if (screen_config.indexed)
		{
			rhi.set_sampler("s_sampler1", 1, palette_mgr_->palette());
		}
		rhi.draw_indexed(6, 0);
	}
}

void BlitPostimgScreens::prepass(Rhi& rhi)
{
	if (!program_)
	{
		static const char* defines[1] = {
			"ENABLE_S_SAMPLER0"
		};
		ProgramDesc desc {};
		desc.name = "postimg";
		desc.defines = tcb::make_span(defines);
		program_ = rhi.create_program(desc);
	}

	if (!indexed_program_)
	{
		static const char* defines[2] = {
			"ENABLE_S_SAMPLER0",
			"ENABLE_S_SAMPLER1"
		};
		ProgramDesc desc {};
		desc.name = "postimg";
		desc.defines = tcb::make_span(defines);
		indexed_program_ = rhi.create_program(desc);
	}

	if (!quad_vbo_)
	{
		quad_vbo_ = rhi.create_buffer({sizeof(kVerts), BufferType::kVertexBuffer, BufferUsage::kImmutable});
		upload_quad_buffer_ = true;
	}

	if (!quad_ibo_)
	{
		quad_ibo_ = rhi.create_buffer({sizeof(kIndices), BufferType::kIndexBuffer, BufferUsage::kImmutable});
		upload_quad_buffer_ = true;
	}

	screen_data_.clear();
}

void BlitPostimgScreens::transfer(Rhi& rhi)
{
	// Upload needed buffers
	if (upload_quad_buffer_)
	{
		rhi.update_buffer(quad_vbo_, 0, tcb::as_bytes(tcb::span(kVerts)));
		rhi.update_buffer(quad_ibo_, 0, tcb::as_bytes(tcb::span(kIndices)));
		upload_quad_buffer_ = false;
	}

	for (uint32_t i = 0; i < screens_; i++)
	{
		BlitPostimgScreens::ScreenConfig& screen_config = screen_configs_[i];
		BlitPostimgScreens::ScreenData data {};

		if (screen_config.indexed)
		{
			data.program = indexed_program_;
		}
		else
		{
			data.program = program_;
		}

		screen_data_[i] = std::move(data);
	}
}
