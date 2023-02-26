#include "pass_software.hpp"

#include <optional>

#include <tcb/span.hpp>

#include "../cxxutil.hpp"
#include "../d_netcmd.h"
#ifdef HAVE_DISCORDRPC
#include "../discord.h"
#endif
#include "../doomstat.h"
#include "../m_avrecorder.h"
#include "../st_stuff.h"
#include "../s_sound.h"
#include "../v_video.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

SoftwareBlitPass::~SoftwareBlitPass() = default;

namespace
{
struct SwBlitVertex
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float u = 0.f;
	float v = 0.f;
};
} // namespace

static const SwBlitVertex kVerts[] =
{
	{-.5f, -.5f, 0.f, 0.f, 0.f},
	{.5f, -.5f, 0.f, 1.f, 0.f},
	{-.5f, .5f, 0.f, 0.f, 1.f},
	{.5f, .5f, 0.f, 1.f, 1.f}
};

static const uint16_t kIndices[] = {0, 1, 2, 1, 3, 2};

static const PipelineDesc kPipelineDescription =
{
	PipelineProgram::kUnshadedPaletted,
	{
		{
			{sizeof(SwBlitVertex)}
		},
		{
			{VertexAttributeName::kPosition, 0, 0},
			{VertexAttributeName::kTexCoord0, 0, 12}
		}
	},
	{{
		{{UniformName::kProjection}},
		{{UniformName::kModelView, UniformName::kTexCoord0Transform}}
	}},
	{{
		// R8 index texture
		SamplerName::kSampler0,
		// 256x1 palette texture
		SamplerName::kSampler1
	}},
	std::nullopt,
	{
		PixelFormat::kRGBA8,
		std::nullopt,
		{true, true, true, true}
	},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.f, 0.f, 0.f, 1.f}
};

static uint32_t next_pow_of_2(uint32_t in)
{
	in--;
	in |= in >> 1;
	in |= in >> 2;
	in |= in >> 4;
	in |= in >> 8;
	in |= in >> 16;
	in++;
	return in;
}

static void temp_legacy_finishupdate_draws()
{
	SCR_CalculateFPS();
	if (st_overlay)
	{
		if (cv_songcredits.value)
			HU_DrawSongCredits();

		if (cv_ticrate.value)
			SCR_DisplayTicRate();

		if (cv_showping.value && netgame &&
				( consoleplayer != serverplayer || ! server_lagless ))
		{
			if (server_lagless)
			{
				if (consoleplayer != serverplayer)
					SCR_DisplayLocalPing();
			}
			else
			{
				for (
						int player = 1;
						player < MAXPLAYERS;
						player++
				){
					if (D_IsPlayerHumanAndGaming(player))
					{
						SCR_DisplayLocalPing();
						break;
					}
				}
			}
		}
		if (cv_mindelay.value && consoleplayer == serverplayer && Playing())
			SCR_DisplayLocalPing();

		M_AVRecorder_DrawFrameRate();
	}

	if (marathonmode)
		SCR_DisplayMarathonInfo();

	// draw captions if enabled
	if (cv_closedcaptioning.value)
		SCR_ClosedCaptions();

#ifdef HAVE_DISCORDRPC
	if (discordRequestList != NULL)
		ST_AskToJoinEnvelope();
#endif
}

void SoftwareBlitPass::prepass(Rhi& rhi)
{
	if (!pipeline_)
	{
		pipeline_ = rhi.create_pipeline(kPipelineDescription);
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

	temp_legacy_finishupdate_draws();

	uint32_t vid_width = static_cast<uint32_t>(vid.width);
	uint32_t vid_height = static_cast<uint32_t>(vid.height);

	if (screen_tex_ && (screen_tex_width_ < vid_width || screen_tex_height_ < vid_height))
	{
		rhi.destroy_texture(screen_tex_);
		screen_tex_ = kNullHandle;
	}

	if (!screen_tex_)
	{
		screen_tex_width_ = next_pow_of_2(vid_width);
		screen_tex_height_ = next_pow_of_2(vid_height);
		screen_tex_ = rhi.create_texture({TextureFormat::kLuminance, screen_tex_width_, screen_tex_height_});
	}

	if (!palette_tex_)
	{
		palette_tex_ = rhi.create_texture({TextureFormat::kRGBA, 256, 1});
	}
}

void SoftwareBlitPass::upload_screen(Rhi& rhi, Handle<TransferContext> ctx)
{
	rhi::Rect screen_rect = {
		0,
		0,
		static_cast<uint32_t>(vid.width),
		static_cast<uint32_t>(vid.height)
	};

	tcb::span<uint8_t> screen_span = tcb::span(vid.buffer, static_cast<size_t>(vid.width * vid.height));
	rhi.update_texture(ctx, screen_tex_, screen_rect, rhi::PixelFormat::kR8, tcb::as_bytes(screen_span));
}

void SoftwareBlitPass::upload_palette(Rhi& rhi, Handle<TransferContext> ctx)
{
	// Unfortunately, pMasterPalette must be swizzled to get a linear layout.
	// Maybe some adjustments to palette storage can make this a straight upload.
	std::array<byteColor_t, 256> palette_32;
	for (size_t i = 0; i < 256; i++)
	{
		palette_32[i] = pMasterPalette[i].s;
	}
	rhi.update_texture(ctx, palette_tex_, {0, 0, 256, 1}, rhi::PixelFormat::kRGBA8, tcb::as_bytes(tcb::span(palette_32)));
}

void SoftwareBlitPass::transfer(Rhi& rhi, Handle<TransferContext> ctx)
{
	if (quad_vbo_needs_upload_ && quad_vbo_)
	{
		rhi.update_buffer_contents(ctx, quad_vbo_, 0, tcb::as_bytes(tcb::span(kVerts)));
		quad_vbo_needs_upload_ = false;
	}

	if (quad_ibo_needs_upload_ && quad_ibo_)
	{
		rhi.update_buffer_contents(ctx, quad_ibo_, 0, tcb::as_bytes(tcb::span(kIndices)));
		quad_ibo_needs_upload_ = false;
	}

	upload_screen(rhi, ctx);
	upload_palette(rhi, ctx);

	// Calculate aspect ratio for black borders
	float aspect = static_cast<float>(vid.width) / static_cast<float>(vid.height);
	float real_aspect = static_cast<float>(vid.realwidth) / static_cast<float>(vid.realheight);
	bool taller = aspect > real_aspect;

	std::array<rhi::UniformVariant, 1> g1_uniforms = {{
		// Projection
		std::array<std::array<float, 4>, 4> {{
			{taller ? 1.f : 1.f / real_aspect, 0.f, 0.f, 0.f},
			{0.f, taller ? -1.f / (1.f / real_aspect) : -1.f, 0.f, 0.f},
			{0.f, 0.f, 1.f, 0.f},
			{0.f, 0.f, 0.f, 1.f}
		}},
	}};

	std::array<rhi::UniformVariant, 2> g2_uniforms =
	{{
		// ModelView
		std::array<std::array<float, 4>, 4>
		{{
			{taller ? 2.f : 2.f * aspect, 0.f, 0.f, 0.f},
			{0.f, taller ? 2.f * (1.f / aspect) : 2.f, 0.f, 0.f},
			{0.f, 0.f, 1.f, 0.f},
			{0.f, 0.f, 0.f, 1.f}
		}},
		// Texcoord0 Transform
		std::array<std::array<float, 3>, 3>
		{{
			{vid.width / static_cast<float>(screen_tex_width_), 0.f, 0.f},
			{0.f, vid.height / static_cast<float>(screen_tex_height_), 0.f},
			{0.f, 0.f, 1.f}
		}}
	}};

	uniform_sets_[0] = rhi.create_uniform_set(ctx, {g1_uniforms});
	uniform_sets_[1] = rhi.create_uniform_set(ctx, {g2_uniforms});

	std::array<rhi::VertexAttributeBufferBinding, 1> vbs = {{{0, quad_vbo_}}};
	std::array<rhi::TextureBinding, 2> tbs = {{
		{rhi::SamplerName::kSampler0, screen_tex_},
		{rhi::SamplerName::kSampler1, palette_tex_}
	}};
	binding_set_ = rhi.create_binding_set(ctx, pipeline_, {vbs, tbs});
}

void SoftwareBlitPass::graphics(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	rhi.begin_default_render_pass(ctx, true);
	rhi.bind_pipeline(ctx, pipeline_);
	rhi.bind_uniform_set(ctx, 0, uniform_sets_[0]);
	rhi.bind_uniform_set(ctx, 1, uniform_sets_[1]);
	rhi.bind_binding_set(ctx, binding_set_);
	rhi.bind_index_buffer(ctx, quad_ibo_);
	rhi.draw_indexed(ctx, 6, 0);
	rhi.end_render_pass(ctx);
}

void SoftwareBlitPass::postpass(Rhi& rhi)
{
	// no-op
}
