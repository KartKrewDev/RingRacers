// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "blit_postimg_screens.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

static const PipelineDesc kPostimgPipelineDesc =
{
	PipelineProgram::kPostimg,
	{{{sizeof(BlitVertex)}}, {{VertexAttributeName::kPosition, 0, 0}, {VertexAttributeName::kTexCoord0, 0, 12}}},
	{{{{UniformName::kTime, UniformName::kProjection, UniformName::kModelView, UniformName::kTexCoord0Transform, UniformName::kTexCoord0Min, UniformName::kTexCoord0Max, UniformName::kPostimgWater, UniformName::kPostimgHeat}}}},
	{{SamplerName::kSampler0}},
	std::nullopt,
	{std::nullopt, {true, true, true, true}},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.0, 0.0, 0.0, 1.0}
};

static const PipelineDesc kPostimgIndexedPipelineDesc =
{
	PipelineProgram::kPostimg,
	{{{sizeof(BlitVertex)}}, {{VertexAttributeName::kPosition, 0, 0}, {VertexAttributeName::kTexCoord0, 0, 12}}},
	{{{{UniformName::kTime, UniformName::kProjection, UniformName::kModelView, UniformName::kTexCoord0Transform, UniformName::kTexCoord0Min, UniformName::kTexCoord0Max, UniformName::kPostimgWater, UniformName::kPostimgHeat}}}},
	{{SamplerName::kSampler0, SamplerName::kSampler1}},
	std::nullopt,
	{std::nullopt, {true, true, true, true}},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.0, 0.0, 0.0, 1.0}
};

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
		BlitPostimgScreens::ScreenData& data = screen_data_[i];

		rhi.bind_pipeline(data.pipeline);
		rhi.set_viewport(get_screen_viewport(i, screens_, target_width_, target_height_));
		rhi.bind_uniform_set(0, data.uniform_set);
		rhi.bind_binding_set(data.binding_set);
		rhi.bind_index_buffer(quad_ibo_);
		rhi.draw_indexed(6, 0);
	}
}

void BlitPostimgScreens::prepass(Rhi& rhi)
{
	if (!renderpass_)
	{
		renderpass_ = rhi.create_render_pass(
			{
				false,
				AttachmentLoadOp::kClear,
				AttachmentStoreOp::kStore,
				AttachmentLoadOp::kDontCare,
				AttachmentStoreOp::kDontCare,
				AttachmentLoadOp::kDontCare,
				AttachmentStoreOp::kDontCare
			}
		);
	}

	if (!pipeline_)
	{
		pipeline_ = rhi.create_pipeline(kPostimgPipelineDesc);
	}

	if (!indexed_pipeline_)
	{
		indexed_pipeline_ = rhi.create_pipeline(kPostimgIndexedPipelineDesc);
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
			data.pipeline = indexed_pipeline_;
		}
		else
		{
			data.pipeline = pipeline_;
		}

		VertexAttributeBufferBinding vertex_bindings[] = {{0, quad_vbo_}};
		TextureBinding sampler_bindings[] =
		{
			{SamplerName::kSampler0, screen_config.source},
			{SamplerName::kSampler1, palette_mgr_->palette()}
		};

		data.binding_set = rhi.create_binding_set(
			data.pipeline,
			{
				vertex_bindings,
				tcb::span(sampler_bindings, screen_config.indexed ? 2 : 1)
			}
		);

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

		UniformVariant uniforms[] =
		{
			static_cast<float>(leveltime),
			projection,
			modelview,
			texcoord_transform,
			texcoord_min,
			texcoord_max,
			screen_config.post.water,
			screen_config.post.heat
		};

		data.uniform_set = rhi.create_uniform_set({uniforms});

		screen_data_[i] = std::move(data);
	}
}
