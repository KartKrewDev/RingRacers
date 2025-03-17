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

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>
#include <tcb/span.hpp>

#include "../core/string.h"
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

PostprocessWipePass::PostprocessWipePass()
{
}

PostprocessWipePass::~PostprocessWipePass() = default;

void PostprocessWipePass::draw(Rhi& rhi)
{
	prepass(rhi);
	transfer(rhi);
	graphics(rhi);
	postpass(rhi);
}

void PostprocessWipePass::prepass(Rhi& rhi)
{
	if (!program_)
	{
		ProgramDesc desc;
		desc.name = "postprocesswipe";
		desc.defines = tcb::span<const char*>();
		program_ = rhi.create_program(desc);
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

	String lumpname = format(FMT_STRING("FADE{:02d}{:02d}"), wipe_type, wipe_frame);
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

void PostprocessWipePass::transfer(Rhi& rhi)
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
		rhi.update_buffer(vbo_, 0, tcb::as_bytes(tcb::span(kPostprocessVerts)));
		upload_vbo_ = false;
	}

	if (upload_ibo_)
	{
		rhi.update_buffer(ibo_, 0, tcb::as_bytes(tcb::span(kPostprocessIndices)));
		upload_ibo_ = false;
	}

	tcb::span<const std::byte> data = tcb::as_bytes(tcb::span(mask_data_));
	rhi.update_texture(wipe_tex_, {0, 0, mask_w_, mask_h_}, PixelFormat::kR8, data);
}

void PostprocessWipePass::graphics(Rhi& rhi)
{
	if (wipe_tex_ == kNullHandle)
	{
		return;
	}

	rhi.bind_program(program_);

	RasterizerStateDesc desc {};
	desc.cull = CullMode::kNone;

	rhi.set_rasterizer_state(desc);
	rhi.set_uniform("u_projection", glm::scale(glm::identity<glm::mat4>(), glm::vec3(2.f, 2.f, 1.f)));
	rhi.set_uniform("u_wipe_colorize_mode", static_cast<int32_t>(wipe_color_mode_));
	rhi.set_uniform("u_wipe_encore_swizzle", static_cast<int32_t>(wipe_swizzle_));
	rhi.set_sampler("s_sampler0", 0, start_);
	rhi.set_sampler("s_sampler1", 1, end_);
	rhi.set_sampler("s_sampler2", 2, wipe_tex_);
	rhi.bind_vertex_attrib("a_position", vbo_, VertexAttributeFormat::kFloat3, offsetof(PostprocessVertex, x), sizeof(PostprocessVertex));
	rhi.bind_vertex_attrib("a_texcoord0", vbo_, VertexAttributeFormat::kFloat2, offsetof(PostprocessVertex, u), sizeof(PostprocessVertex));
	rhi.set_viewport({0, 0, width_, height_});
	rhi.bind_index_buffer(ibo_);
	rhi.draw_indexed(6, 0);
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
