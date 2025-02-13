// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "postprocess_wipe.hpp"

#include <string>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>
#include <tcb/span.hpp>

#include "../f_finale.h"
#include "../w_wad.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

namespace
{
struct PostprocessVertex
{
	float x;
	float y;
	float z;
	float u;
	float v;
};

static const PostprocessVertex kPostprocessVerts[] =
	{{-.5f, -.5f, 0.f, 0.f, 0.f}, {.5f, -.5f, 0.f, 1.f, 0.f}, {-.5f, .5f, 0.f, 0.f, 1.f}, {.5f, .5f, 0.f, 1.f, 1.f}};

static const uint16_t kPostprocessIndices[] = {0, 1, 2, 1, 3, 2};

} // namespace

static const PipelineDesc kWipePipelineDesc = {
	PipelineProgram::kPostprocessWipe,
	{{{sizeof(PostprocessVertex)}},
	 {
		 {VertexAttributeName::kPosition, 0, 0},
		 {VertexAttributeName::kTexCoord0, 0, 12},
	 }},
	{{{{UniformName::kProjection, UniformName::kWipeColorizeMode, UniformName::kWipeEncoreSwizzle}}}},
	{{SamplerName::kSampler0, SamplerName::kSampler1, SamplerName::kSampler2}},
	std::nullopt,
	{std::nullopt, {true, true, true, true}},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.f, 0.f, 0.f, 1.f}};

PostprocessWipePass::PostprocessWipePass()
{
}

PostprocessWipePass::~PostprocessWipePass() = default;

void PostprocessWipePass::draw(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	prepass(rhi);
	transfer(rhi, ctx);
	graphics(rhi, ctx);
	postpass(rhi);
}

void PostprocessWipePass::prepass(Rhi& rhi)
{
	if (!pipeline_)
	{
		pipeline_ = rhi.create_pipeline(kWipePipelineDesc);
	}

	if (!vbo_)
	{
		vbo_ = rhi.create_buffer({sizeof(PostprocessVertex) * 4, BufferType::kVertexBuffer, BufferUsage::kImmutable});
		upload_vbo_ = true;
	}
	if (!ibo_)
	{
		ibo_ = rhi.create_buffer({2 * 6, BufferType::kIndexBuffer, BufferUsage::kImmutable});
		upload_ibo_ = true;
	}

	uint32_t wipe_mode = g_wipemode;
	uint32_t wipe_type = g_wipetype;
	uint32_t wipe_frame = g_wipeframe;
	bool wipe_reverse = g_wipereverse;

	wipe_color_mode_ = 0; // TODO 0 = modulate, 1 = invert, 2 = MD to black, 3 = MD to white
	if (F_WipeIsToBlack(wipe_mode))
	{
		wipe_color_mode_ = 2;
	}
	else if (F_WipeIsToWhite(wipe_mode))
	{
		wipe_color_mode_ = 3;
	}
	else if (F_WipeIsToInvert(wipe_mode))
	{
		wipe_color_mode_ = 1;
	}
	else if (F_WipeIsCrossfade(wipe_mode))
	{
		wipe_color_mode_ = 0;
	}

	wipe_swizzle_ = g_wipeencorewiggle;

	if (wipe_type >= 100 || wipe_frame >= 100)
	{
		return;
	}

	std::string lumpname = fmt::format(FMT_STRING("FADE{:02d}{:02d}"), wipe_type, wipe_frame);
	lumpnum_t mask_lump = W_CheckNumForName(lumpname.c_str());
	if (mask_lump == LUMPERROR)
	{
		return;
	}

	std::size_t mask_lump_size = W_LumpLength(mask_lump);
	switch (mask_lump_size)
	{
	case 256000:
		mask_w_ = 640;
		mask_h_ = 400;
		break;
	case 64000:
		mask_w_ = 320;
		mask_h_ = 200;
		break;
	case 16000:
		mask_w_ = 160;
		mask_h_ = 100;
		break;
	case 4000:
		mask_w_ = 80;
		mask_h_ = 50;
		break;
	default:
		return;
	}

	mask_data_.clear();
	mask_data_.resize(mask_lump_size, 0);
	W_ReadLump(mask_lump, mask_data_.data());
	if (wipe_reverse)
	{
		for (auto& b : mask_data_)
		{
			b = 32 - b;
		}
	}

	wipe_tex_ = rhi.create_texture({
		TextureFormat::kLuminance,
		mask_w_,
		mask_h_,
		TextureWrapMode::kClamp,
		TextureWrapMode::kClamp
	});
}

void PostprocessWipePass::transfer(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	if (wipe_tex_ == kNullHandle)
	{
		return;
	}

	if (start_ == kNullHandle || end_ == kNullHandle)
	{
		return;
	}

	if (upload_vbo_)
	{
		rhi.update_buffer(ctx, vbo_, 0, tcb::as_bytes(tcb::span(kPostprocessVerts)));
		upload_vbo_ = false;
	}

	if (upload_ibo_)
	{
		rhi.update_buffer(ctx, ibo_, 0, tcb::as_bytes(tcb::span(kPostprocessIndices)));
		upload_ibo_ = false;
	}

	tcb::span<const std::byte> data = tcb::as_bytes(tcb::span(mask_data_));
	rhi.update_texture(ctx, wipe_tex_, {0, 0, mask_w_, mask_h_}, PixelFormat::kR8, data);

	UniformVariant uniforms[] = {
		glm::scale(glm::identity<glm::mat4>(), glm::vec3(2.f, 2.f, 1.f)),
		static_cast<int32_t>(wipe_color_mode_),
		static_cast<int32_t>(wipe_swizzle_)
	};
	us_ = rhi.create_uniform_set(ctx, {tcb::span(uniforms)});

	VertexAttributeBufferBinding vbos[] = {{0, vbo_}};
	TextureBinding tx[] = {
		{SamplerName::kSampler0, start_},
		{SamplerName::kSampler1, end_},
		{SamplerName::kSampler2, wipe_tex_}};
	bs_ = rhi.create_binding_set(ctx, pipeline_, {vbos, tx});
}

void PostprocessWipePass::graphics(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	if (wipe_tex_ == kNullHandle)
	{
		return;
	}

	rhi.bind_pipeline(ctx, pipeline_);
	rhi.set_viewport(ctx, {0, 0, width_, height_});
	rhi.bind_uniform_set(ctx, 0, us_);
	rhi.bind_binding_set(ctx, bs_);
	rhi.bind_index_buffer(ctx, ibo_);
	rhi.draw_indexed(ctx, 6, 0);
}

void PostprocessWipePass::postpass(Rhi& rhi)
{
	if (wipe_tex_)
	{
		rhi.destroy_texture(wipe_tex_);
		wipe_tex_ = kNullHandle;
	}

	mask_data_.clear();
}
