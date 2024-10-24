// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "blit_rect.hpp"

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

/// @brief Pipeline used for non-paletted source textures.
static const PipelineDesc kUnshadedPipelineDescription = {
	PipelineProgram::kUnshaded,
	{{{sizeof(BlitVertex)}}, {{VertexAttributeName::kPosition, 0, 0}, {VertexAttributeName::kTexCoord0, 0, 12}}},
	{{{UniformName::kProjection}, {{UniformName::kModelView, UniformName::kTexCoord0Transform}}}},
	{{// RGB/A texture
	  SamplerName::kSampler0}},
	std::nullopt,
	{std::nullopt, {true, true, true, true}},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.f, 0.f, 0.f, 1.f}};

/// @brief Pipeline used for sharp bilinear special blit.
static const PipelineDesc kSharpBilinearPipelineDescription = {
	PipelineProgram::kSharpBilinear,
	{{{sizeof(BlitVertex)}}, {{VertexAttributeName::kPosition, 0, 0}, {VertexAttributeName::kTexCoord0, 0, 12}}},
	{{{UniformName::kProjection}, {{UniformName::kModelView, UniformName::kTexCoord0Transform, UniformName::kSampler0Size}}}},
	{{// RGB/A texture
	  SamplerName::kSampler0}},
	std::nullopt,
	{std::nullopt, {true, true, true, true}},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.f, 0.f, 0.f, 1.f}};

/// @brief Pipeline used for CRT special blit
static const PipelineDesc kCrtPipelineDescription = {
	PipelineProgram::kCrt,
	{{{sizeof(BlitVertex)}}, {{VertexAttributeName::kPosition, 0, 0}, {VertexAttributeName::kTexCoord0, 0, 12}}},
	{{{UniformName::kProjection}, {{UniformName::kModelView, UniformName::kTexCoord0Transform, UniformName::kSampler0Size}}}},
	{{// RGB/A texture
	  SamplerName::kSampler0, SamplerName::kSampler1}},
	std::nullopt,
	{std::nullopt, {true, true, true, true}},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.f, 0.f, 0.f, 1.f}};

/// @brief Pipeline used for CRT special blit (sharp)
static const PipelineDesc kCrtSharpPipelineDescription = {
	PipelineProgram::kCrtSharp,
	{{{sizeof(BlitVertex)}}, {{VertexAttributeName::kPosition, 0, 0}, {VertexAttributeName::kTexCoord0, 0, 12}}},
	{{{UniformName::kProjection}, {{UniformName::kModelView, UniformName::kTexCoord0Transform, UniformName::kSampler0Size}}}},
	{{// RGB/A texture
	  SamplerName::kSampler0, SamplerName::kSampler1}},
	std::nullopt,
	{std::nullopt, {true, true, true, true}},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.f, 0.f, 0.f, 1.f}};

BlitRectPass::BlitRectPass() : BlitRectPass(BlitRectPass::BlitMode::kNearest) {}
BlitRectPass::BlitRectPass(BlitRectPass::BlitMode blit_mode) : blit_mode_(blit_mode) {}
BlitRectPass::~BlitRectPass() = default;

void BlitRectPass::draw(Rhi& rhi)
{
	prepass(rhi);
	transfer(rhi);
	graphics(rhi);
}

void BlitRectPass::prepass(Rhi& rhi)
{
	if (!pipeline_)
	{
		switch (blit_mode_)
		{
			case BlitRectPass::BlitMode::kNearest:
				pipeline_ = rhi.create_pipeline(kUnshadedPipelineDescription);
				break;
			case BlitRectPass::BlitMode::kSharpBilinear:
				pipeline_ = rhi.create_pipeline(kSharpBilinearPipelineDescription);
				break;
			case BlitRectPass::BlitMode::kCrt:
				pipeline_ = rhi.create_pipeline(kCrtPipelineDescription);
				break;
			case BlitRectPass::BlitMode::kCrtSharp:
				pipeline_ = rhi.create_pipeline(kCrtSharpPipelineDescription);
				break;
			default:
				std::terminate();
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

	if ((blit_mode_ == BlitRectPass::BlitMode::kCrt || blit_mode_ == BlitRectPass::BlitMode::kCrtSharp) && !dot_pattern_)
	{
		dot_pattern_ = rhi.create_texture({
			rhi::TextureFormat::kRGBA,
			12,
			4,
			rhi::TextureWrapMode::kRepeat,
			rhi::TextureWrapMode::kRepeat,
			rhi::TextureFilterMode::kLinear,
			rhi::TextureFilterMode::kLinear
		});
		dot_pattern_needs_upload_ = true;
	}
}

void BlitRectPass::transfer(Rhi& rhi)
{
	if (quad_vbo_needs_upload_ && quad_vbo_)
	{
		rhi.update_buffer(quad_vbo_, 0, tcb::as_bytes(tcb::span(kVerts)));
		quad_vbo_needs_upload_ = false;
	}

	if (quad_ibo_needs_upload_ && quad_ibo_)
	{
		rhi.update_buffer(quad_ibo_, 0, tcb::as_bytes(tcb::span(kIndices)));
		quad_ibo_needs_upload_ = false;
	}

	if (dot_pattern_needs_upload_ && dot_pattern_)
	{
		// Listen. I'm a *very* particular kind of lazy.
		// If I'm being honest, I just don't want to have to embed a .png in the pk3s and deal with that.
		static const uint8_t kDotPattern[] = {
			255, 0, 0, 255,
			0, 0, 0, 255,
			0, 255, 0, 255,
			0, 0, 0, 255,
			0, 0, 255, 255,
			0, 0, 0, 255,
			255, 0, 0, 255,
			0, 0, 0, 255,
			0, 255, 0, 255,
			0, 0, 0, 255,
			0, 0, 255, 255,
			0, 0, 0, 255,

			255, 0, 0, 255,
			0, 0, 0, 255,
			0, 255, 0, 255,
			0, 0, 0, 255,
			0, 0, 255, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,

			255, 0, 0, 255,
			0, 0, 0, 255,
			0, 255, 0, 255,
			0, 0, 0, 255,
			0, 0, 255, 255,
			0, 0, 0, 255,
			255, 0, 0, 255,
			0, 0, 0, 255,
			0, 255, 0, 255,
			0, 0, 0, 255,
			0, 0, 255, 255,
			0, 0, 0, 255,

			0, 0, 0, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,
			0, 0, 0, 255,
			255, 0, 0, 255,
			0, 0, 0, 255,
			0, 255, 0, 255,
			0, 0, 0, 255,
			0, 0, 255, 255,
			0, 0, 0, 255,
		};
		rhi.update_texture(dot_pattern_, {0, 0, 12, 4}, PixelFormat::kRGBA8, tcb::as_bytes(tcb::span(kDotPattern)));
		dot_pattern_needs_upload_ = false;
	}

	float aspect = 1.0;
	float output_aspect = 1.0;
	if (output_correct_aspect_)
	{
		aspect = static_cast<float>(texture_width_) / static_cast<float>(texture_height_);
		output_aspect = static_cast<float>(output_position_.w) / static_cast<float>(output_position_.h);
	}
	bool taller = aspect > output_aspect;

	rhi::TextureDetails texture_details = rhi.get_texture_details(texture_);

	std::array<rhi::UniformVariant, 1> g1_uniforms = {{
		// Projection
		glm::scale(
			glm::identity<glm::mat4>(),
			glm::vec3(taller ? 1.f : 1.f / output_aspect, taller ? -1.f / (1.f / output_aspect) : -1.f, 1.f)
		)
	}};

	uniform_sets_[0] = rhi.create_uniform_set({g1_uniforms});

	switch (blit_mode_)
	{
	case BlitRectPass::BlitMode::kCrt:
	{
		std::array<rhi::UniformVariant, 3> g2_uniforms = {
			// ModelView
			glm::scale(
				glm::identity<glm::mat4>(),
				glm::vec3(taller ? 2.f : 2.f * aspect, taller ? 2.f * (1.f / aspect) : 2.f, 1.f)
			),
			// Texcoord0 Transform
			glm::mat3(
				glm::vec3(1.f, 0.f, 0.f),
				glm::vec3(0.f, output_flip_ ? -1.f : 1.f, 0.f),
				glm::vec3(0.f, output_flip_ ? 1.f : 0.f, 1.f)
			),
			// Sampler 0 Size
			glm::vec2(texture_details.width, texture_details.height)
		};
		uniform_sets_[1] = rhi.create_uniform_set({g2_uniforms});

		std::array<rhi::VertexAttributeBufferBinding, 1> vbs = {{{0, quad_vbo_}}};
		std::array<rhi::TextureBinding, 2> tbs = {{{rhi::SamplerName::kSampler0, texture_}, {rhi::SamplerName::kSampler1, dot_pattern_}}};
		binding_set_ = rhi.create_binding_set(pipeline_, {vbs, tbs});
		break;
	}
	case BlitRectPass::BlitMode::kCrtSharp:
	{
		std::array<rhi::UniformVariant, 3> g2_uniforms = {
			// ModelView
			glm::scale(
				glm::identity<glm::mat4>(),
				glm::vec3(taller ? 2.f : 2.f * aspect, taller ? 2.f * (1.f / aspect) : 2.f, 1.f)
			),
			// Texcoord0 Transform
			glm::mat3(
				glm::vec3(1.f, 0.f, 0.f),
				glm::vec3(0.f, output_flip_ ? -1.f : 1.f, 0.f),
				glm::vec3(0.f, output_flip_ ? 1.f : 0.f, 1.f)
			),
			// Sampler 0 Size
			glm::vec2(texture_details.width, texture_details.height)
		};
		uniform_sets_[1] = rhi.create_uniform_set({g2_uniforms});

		std::array<rhi::VertexAttributeBufferBinding, 1> vbs = {{{0, quad_vbo_}}};
		std::array<rhi::TextureBinding, 2> tbs = {{{rhi::SamplerName::kSampler0, texture_}, {rhi::SamplerName::kSampler1, dot_pattern_}}};
		binding_set_ = rhi.create_binding_set(pipeline_, {vbs, tbs});
		break;
	}
	case BlitRectPass::BlitMode::kSharpBilinear:
	{
		std::array<rhi::UniformVariant, 3> g2_uniforms = {
			// ModelView
			glm::scale(
				glm::identity<glm::mat4>(),
				glm::vec3(taller ? 2.f : 2.f * aspect, taller ? 2.f * (1.f / aspect) : 2.f, 1.f)
			),
			// Texcoord0 Transform
			glm::mat3(
				glm::vec3(1.f, 0.f, 0.f),
				glm::vec3(0.f, output_flip_ ? -1.f : 1.f, 0.f),
				glm::vec3(0.f, output_flip_ ? 1.f : 0.f, 1.f)
			),
			// Sampler0 size
			glm::vec2(texture_details.width, texture_details.height)
		};
		uniform_sets_[1] = rhi.create_uniform_set({g2_uniforms});

		std::array<rhi::VertexAttributeBufferBinding, 1> vbs = {{{0, quad_vbo_}}};
		std::array<rhi::TextureBinding, 1> tbs = {{{rhi::SamplerName::kSampler0, texture_}}};
		binding_set_ = rhi.create_binding_set(pipeline_, {vbs, tbs});
		break;
	}
	default:
	{
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
				glm::vec3(0.f, output_flip_ ? 1.f : 0.f, 1.f)
			)
		};
		uniform_sets_[1] = rhi.create_uniform_set({g2_uniforms});

		std::array<rhi::VertexAttributeBufferBinding, 1> vbs = {{{0, quad_vbo_}}};
		std::array<rhi::TextureBinding, 1> tbs = {{{rhi::SamplerName::kSampler0, texture_}}};
		binding_set_ = rhi.create_binding_set(pipeline_, {vbs, tbs});
		break;
	}
	}
}

void BlitRectPass::graphics(Rhi& rhi)
{
	rhi.bind_pipeline(pipeline_);
	rhi.set_viewport(output_position_);
	rhi.bind_uniform_set(0, uniform_sets_[0]);
	rhi.bind_uniform_set(1, uniform_sets_[1]);
	rhi.bind_binding_set(binding_set_);
	rhi.bind_index_buffer(quad_ibo_);
	rhi.draw_indexed(6, 0);
}
