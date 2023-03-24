// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "pass_blit_rect.hpp"

#include <optional>

#include <glm/gtc/matrix_transform.hpp>
#include <tcb/span.hpp>

#include "../cxxutil.hpp"

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

/// @brief Pipeline used for paletted source textures. Requires the texture and the palette texture.
static const PipelineDesc kPalettedPipelineDescription = {
	PipelineProgram::kUnshadedPaletted,
	{{{sizeof(BlitVertex)}}, {{VertexAttributeName::kPosition, 0, 0}, {VertexAttributeName::kTexCoord0, 0, 12}}},
	{{{{UniformName::kProjection}}, {{UniformName::kModelView, UniformName::kTexCoord0Transform}}}},
	{{// R8 index texture
	  SamplerName::kSampler0,
	  // 256x1 palette texture
	  SamplerName::kSampler1}},
	std::nullopt,
	{std::nullopt, {true, true, true, true}},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.f, 0.f, 0.f, 1.f}};

/// @brief Pipeline used for non-paletted source textures.
static const PipelineDesc kUnshadedPipelineDescription = {
	PipelineProgram::kUnshaded,
	{{{sizeof(BlitVertex)}}, {{VertexAttributeName::kPosition, 0, 0}, {VertexAttributeName::kTexCoord0, 0, 12}}},
	{{{{UniformName::kProjection}}, {{UniformName::kModelView, UniformName::kTexCoord0Transform}}}},
	{{// RGB/A texture
	  SamplerName::kSampler0}},
	std::nullopt,
	{std::nullopt, {true, true, true, true}},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.f, 0.f, 0.f, 1.f}};

BlitRectPass::BlitRectPass() : Pass()
{
}

BlitRectPass::BlitRectPass(bool output_clear) : Pass(), output_clear_(output_clear)
{
}

BlitRectPass::BlitRectPass(const std::shared_ptr<MainPaletteManager>& palette_mgr, bool output_clear)
	: Pass(), output_clear_(output_clear), palette_mgr_(palette_mgr)
{
}

BlitRectPass::~BlitRectPass() = default;

void BlitRectPass::prepass(Rhi& rhi)
{
	if (!pipeline_)
	{
		if (palette_mgr_)
		{
			pipeline_ = rhi.create_pipeline(kPalettedPipelineDescription);
		}
		else
		{
			pipeline_ = rhi.create_pipeline(kUnshadedPipelineDescription);
		}
	}

	if (!quad_vbo_)
	{
		quad_vbo_ = rhi.create_buffer({sizeof(kVerts), BufferType::kVertexBuffer, BufferUsage::kImmutable});
		quad_vbo_needs_upload_ = true;
	}

	if (!quad_ibo_)
	{
		quad_ibo_ = rhi.create_buffer({sizeof(kIndices), BufferType::kIndexBuffer, BufferUsage::kImmutable});
		quad_ibo_needs_upload_ = true;
	}

	if (!render_pass_)
	{
		render_pass_ = rhi.create_render_pass(
			{
				false,
			 	output_clear_ ? AttachmentLoadOp::kClear : AttachmentLoadOp::kLoad,
			 	AttachmentStoreOp::kStore,
				AttachmentLoadOp::kDontCare,
				AttachmentStoreOp::kDontCare,
				AttachmentLoadOp::kDontCare,
				AttachmentStoreOp::kDontCare
			}
		);
	}
}

void BlitRectPass::transfer(Rhi& rhi, Handle<TransferContext> ctx)
{
	if (quad_vbo_needs_upload_ && quad_vbo_)
	{
		rhi.update_buffer(ctx, quad_vbo_, 0, tcb::as_bytes(tcb::span(kVerts)));
		quad_vbo_needs_upload_ = false;
	}

	if (quad_ibo_needs_upload_ && quad_ibo_)
	{
		rhi.update_buffer(ctx, quad_ibo_, 0, tcb::as_bytes(tcb::span(kIndices)));
		quad_ibo_needs_upload_ = false;
	}

	float aspect = 1.0;
	float output_aspect = 1.0;
	if (output_correct_aspect_)
	{
		aspect = static_cast<float>(texture_width_) / static_cast<float>(texture_height_);
		output_aspect = static_cast<float>(output_width_) / static_cast<float>(output_height_);
	}
	bool taller = aspect > output_aspect;

	std::array<rhi::UniformVariant, 1> g1_uniforms = {{
		// Projection
		glm::scale(
			glm::identity<glm::mat4>(),
			glm::vec3(taller ? 1.f : 1.f / output_aspect, taller ? -1.f / (1.f / output_aspect) : -1.f, 1.f)
		)
	}};

	std::array<rhi::UniformVariant, 2> g2_uniforms = {
		// ModelView
		glm::scale(
			glm::identity<glm::mat4>(),
			glm::vec3(taller ? 2.f : 2.f * aspect, taller ? 2.f * (1.f / aspect) : 2.f, 1.f)
		),
		// Texcoord0 Transform
		glm::mat3(
			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(0.f, output_flip_ ? -1.f : 1.f, 0.f),
			glm::vec3(0.f, 0.f, 1.f)
		)
	};

	uniform_sets_[0] = rhi.create_uniform_set(ctx, {g1_uniforms});
	uniform_sets_[1] = rhi.create_uniform_set(ctx, {g2_uniforms});

	std::array<rhi::VertexAttributeBufferBinding, 1> vbs = {{{0, quad_vbo_}}};
	if (palette_mgr_)
	{
		std::array<rhi::TextureBinding, 2> tbs = {
			{{rhi::SamplerName::kSampler0, texture_}, {rhi::SamplerName::kSampler1, palette_mgr_->palette()}}};
		binding_set_ = rhi.create_binding_set(ctx, pipeline_, {vbs, tbs});
	}
	else
	{
		std::array<rhi::TextureBinding, 1> tbs = {{{rhi::SamplerName::kSampler0, texture_}}};
		binding_set_ = rhi.create_binding_set(ctx, pipeline_, {vbs, tbs});
	}
}

static constexpr const glm::vec4 kClearColor = {0, 0, 0, 1};

void BlitRectPass::graphics(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	if (output_)
	{
		rhi.begin_render_pass(ctx, {render_pass_, output_, std::nullopt, kClearColor});
	}
	else
	{
		rhi.begin_default_render_pass(ctx, output_clear_);
	}

	rhi.bind_pipeline(ctx, pipeline_);
	if (output_)
	{
		rhi.set_viewport(ctx, {0, 0, output_width_, output_height_});
	}
	rhi.bind_uniform_set(ctx, 0, uniform_sets_[0]);
	rhi.bind_uniform_set(ctx, 1, uniform_sets_[1]);
	rhi.bind_binding_set(ctx, binding_set_);
	rhi.bind_index_buffer(ctx, quad_ibo_);
	rhi.draw_indexed(ctx, 6, 0);
	rhi.end_render_pass(ctx);
}

void BlitRectPass::postpass(Rhi& rhi)
{
}
