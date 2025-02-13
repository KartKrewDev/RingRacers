// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "blit_rect.hpp"

#include <optional>
#include <vector>

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
	if (!program_)
	{
		ProgramDesc desc {};
		const char* defines[1] = {"ENABLE_VA_TEXCOORD0"};
		desc.defines = tcb::make_span(defines);
		switch (blit_mode_)
		{
		case BlitRectPass::BlitMode::kNearest:
			desc.name = "unshaded";
			break;
		case BlitRectPass::BlitMode::kSharpBilinear:
			desc.name = "sharpbilinear";
			break;
		case BlitRectPass::BlitMode::kCrt:
			desc.name = "crt";
			break;
		case BlitRectPass::BlitMode::kCrtSharp:
			desc.name = "crtsharp";
			break;
		default:
			std::terminate();
		}
		program_ = rhi.create_program(desc);
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
}

void BlitRectPass::graphics(Rhi& rhi)
{
	rhi.bind_program(program_);

	RasterizerStateDesc rs {};
	rs.cull = CullMode::kNone;

	rhi.set_rasterizer_state(rs);
	rhi.bind_vertex_attrib("a_position", quad_vbo_, VertexAttributeFormat::kFloat3, offsetof(BlitVertex, x), sizeof(BlitVertex));
	rhi.bind_vertex_attrib("a_texcoord0", quad_vbo_, VertexAttributeFormat::kFloat2, offsetof(BlitVertex, u), sizeof(BlitVertex));

	float aspect = 1.0;
	float output_aspect = 1.0;
	if (output_correct_aspect_)
	{
		aspect = static_cast<float>(texture_width_) / static_cast<float>(texture_height_);
		output_aspect = static_cast<float>(output_position_.w) / static_cast<float>(output_position_.h);
	}
	bool taller = aspect > output_aspect;

	rhi::TextureDetails texture_details = rhi.get_texture_details(texture_);

	glm::mat4 projection = glm::scale(
		glm::identity<glm::mat4>(),
		glm::vec3(taller ? 1.f : 1.f / output_aspect, taller ? -1.f / (1.f / output_aspect) : -1.f, 1.f)
	);
	glm::mat4 modelview = glm::scale(
		glm::identity<glm::mat4>(),
		glm::vec3(taller ? 2.f : 2.f * aspect, taller ? 2.f * (1.f / aspect) : 2.f, 1.f)
	);;
	glm::mat3 texcoord0_transform = glm::mat3(
		glm::vec3(1.f, 0.f, 0.f),
		glm::vec3(0.f, output_flip_ ? -1.f : 1.f, 0.f),
		glm::vec3(0.f, output_flip_ ? 1.f : 0.f, 1.f)
	);
	glm::vec2 sampler0_size = glm::vec2(texture_details.width, texture_details.height);

	rhi.set_uniform("u_projection", projection);
	rhi.set_uniform("u_modelview", modelview);
	rhi.set_uniform("u_texcoord0_transform", texcoord0_transform);

	switch (blit_mode_)
	{
	case BlitRectPass::BlitMode::kNearest:
		break;
	default:
		rhi.set_uniform("u_sampler0_size", sampler0_size);
		break;
	}
	rhi.set_sampler("s_sampler0", 0, texture_);
	switch (blit_mode_)
	{
	case BlitRectPass::BlitMode::kCrt:
		rhi.set_sampler("s_sampler1", 1, dot_pattern_);
		break;
	case BlitRectPass::BlitMode::kCrtSharp:
		rhi.set_sampler("s_sampler1", 1, dot_pattern_);
		break;
	default:
		break;
	}
	rhi.set_viewport(output_position_);
	rhi.bind_index_buffer(quad_ibo_);
	rhi.draw_indexed(6, 0);
}
