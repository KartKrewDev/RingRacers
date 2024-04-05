// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Ronald "Eidolon" Kinard
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "pass_imgui.hpp"

#include <imgui.h>

#include "../v_video.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

static const PipelineDesc kPipelineDesc = {
	PipelineProgram::kUnshaded,
	{{{sizeof(ImDrawVert)}},
	 {{VertexAttributeName::kPosition, 0, 0},
	  {VertexAttributeName::kTexCoord0, 0, 12},
	  {VertexAttributeName::kColor, 0, 24}}},
	{{{UniformName::kProjection}, {{UniformName::kModelView, UniformName::kTexCoord0Transform}}}},
	{{SamplerName::kSampler0}},
	PipelineDepthStencilStateDesc {true, true, CompareFunc::kAlways, false, {}, {}},
	{BlendDesc {
		 BlendFactor::kSourceAlpha,
		 BlendFactor::kOneMinusSourceAlpha,
		 BlendFunction::kAdd,
		 BlendFactor::kOne,
		 BlendFactor::kOneMinusSourceAlpha,
		 BlendFunction::kAdd},
	 {true, true, true, true}},
	PrimitiveType::kTriangles,
	CullMode::kNone,
	FaceWinding::kCounterClockwise,
	{0.f, 0.f, 0.f, 1.f}};

ImguiPass::ImguiPass() : Pass()
{
}

ImguiPass::~ImguiPass() = default;

void ImguiPass::prepass(Rhi& rhi)
{
	if (!pipeline_)
	{
		pipeline_ = rhi.create_pipeline(kPipelineDesc);
	}

	ImGuiIO& io = ImGui::GetIO();

	if (!font_atlas_)
	{
		unsigned char* pixels;
		int width;
		int height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		uint32_t uwidth = static_cast<uint32_t>(width);
		uint32_t uheight = static_cast<uint32_t>(height);

		font_atlas_ = rhi.create_texture({
			TextureFormat::kRGBA,
			uwidth,
			uheight,
			TextureWrapMode::kRepeat,
			TextureWrapMode::kRepeat
		});
		io.Fonts->SetTexID(font_atlas_);
	}

	ImGui::Render();

	ImDrawData* data = ImGui::GetDrawData();
	ImVec2 clip_off(data->DisplayPos);
	ImVec2 clip_scale(data->FramebufferScale);
	tcb::span<ImDrawList*> draw_lists = tcb::span(data->CmdLists, data->CmdListsCount);

	for (auto list : draw_lists)
	{
		Handle<Buffer> vbo = rhi.create_buffer(
			{static_cast<uint32_t>(list->VtxBuffer.size_in_bytes()), BufferType::kVertexBuffer, BufferUsage::kImmutable}
		);
		Handle<Buffer> ibo = rhi.create_buffer(
			{static_cast<uint32_t>(list->IdxBuffer.size_in_bytes()), BufferType::kIndexBuffer, BufferUsage::kImmutable}
		);

		DrawList hwr2_list;
		hwr2_list.list = list;
		hwr2_list.vbo = vbo;
		hwr2_list.ibo = ibo;

		for (auto& cmd : list->CmdBuffer)
		{
			if (cmd.UserCallback)
			{
				cmd.UserCallback(list, &cmd);
				continue;
			}

			ImVec2 clip_min((cmd.ClipRect.x - clip_off.x) * clip_scale.x, (cmd.ClipRect.y - clip_off.y) * clip_scale.y);
			ImVec2 clip_max((cmd.ClipRect.z - clip_off.x) * clip_scale.x, (cmd.ClipRect.w - clip_off.y) * clip_scale.y);
			if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
			{
				continue;
			}

			DrawCmd draw_cmd;
			ImTextureID tex_id = cmd.GetTexID();
			draw_cmd.tex = tex_id;
			draw_cmd.v_offset = cmd.VtxOffset;
			draw_cmd.i_offset = cmd.IdxOffset;
			draw_cmd.elems = cmd.ElemCount;
			draw_cmd.clip = {
				static_cast<int32_t>(clip_min.x),
				static_cast<int32_t>((data->DisplaySize.y * data->FramebufferScale.y) - clip_max.y),
				static_cast<uint32_t>(clip_max.x - clip_min.x),
				static_cast<uint32_t>(clip_max.y - clip_min.y)};
			hwr2_list.cmds.push_back(std::move(draw_cmd));
		}
		draw_lists_.push_back(std::move(hwr2_list));
	}
}

void ImguiPass::transfer(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	ImGuiIO& io = ImGui::GetIO();

	{
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		rhi.update_texture(
			ctx,
			font_atlas_,
			{0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
			rhi::PixelFormat::kRGBA8,
			tcb::as_bytes(tcb::span(pixels, static_cast<size_t>(width * height * 4)))
		);
	}

	for (auto& draw_list : draw_lists_)
	{
		Handle<Buffer> vbo = draw_list.vbo;
		Handle<Buffer> ibo = draw_list.ibo;

		ImDrawList* im_list = static_cast<ImDrawList*>(draw_list.list);

		for (auto& vtx : im_list->VtxBuffer)
		{
			vtx.pos.z = 0.f;
			vtx.colf[0] = ((vtx.col & 0xFF) >> 0) / 255.f;
			vtx.colf[1] = ((vtx.col & 0xFF00) >> 8) / 255.f;
			vtx.colf[2] = ((vtx.col & 0xFF0000) >> 16) / 255.f;
			vtx.colf[3] = ((vtx.col & 0xFF000000) >> 24) / 255.f;
		}

		tcb::span<ImDrawVert> vert_span = tcb::span(im_list->VtxBuffer.Data, im_list->VtxBuffer.size());
		rhi.update_buffer(ctx, vbo, 0, tcb::as_bytes(vert_span));

		tcb::span<ImDrawIdx> index_span = tcb::span(im_list->IdxBuffer.Data, im_list->IdxBuffer.size());
		rhi.update_buffer(ctx, ibo, 0, tcb::as_bytes(index_span));

		// Uniform sets
		std::array<UniformVariant, 1> g1_uniforms = {
			// Projection
			glm::mat4(
				glm::vec4(2.f / vid.realwidth, 0.f, 0.f, 0.f),
				glm::vec4(0.f, 2.f / vid.realheight, 0.f, 0.f),
				glm::vec4(0.f, 0.f, 1.f, 0.f),
				glm::vec4(-1.f, 1.f, 0.f, 1.f)
			)
		};
		std::array<UniformVariant, 2> g2_uniforms = {
			// ModelView
			glm::mat4(
				glm::vec4(1.f, 0.f, 0.f, 0.f),
				glm::vec4(0.f, -1.f, 0.f, 0.f),
				glm::vec4(0.f, 0.f, 1.f, 0.f),
				glm::vec4(0.f, 0, 0.f, 1.f)
			),
			// Texcoord0 Transform
			glm::mat3(
				glm::vec3(1.f, 0.f, 0.f),
				glm::vec3(0.f, 1.f, 0.f),
				glm::vec3(0.f, 0.f, 1.f)
			)
		};
		Handle<UniformSet> us_1 = rhi.create_uniform_set(ctx, {g1_uniforms});
		Handle<UniformSet> us_2 = rhi.create_uniform_set(ctx, {g2_uniforms});

		draw_list.us_1 = us_1;
		draw_list.us_2 = us_2;

		for (auto& draw_cmd : draw_list.cmds)
		{
			// Binding set
			std::array<rhi::VertexAttributeBufferBinding, 1> vbos = {{{0, vbo}}};
			std::array<rhi::TextureBinding, 1> tbs = {{{rhi::SamplerName::kSampler0, draw_cmd.tex}}};
			rhi::Handle<rhi::BindingSet> binding_set = rhi.create_binding_set(ctx, pipeline_, {vbos, tbs});
			draw_cmd.binding_set = binding_set;
		}
	}
}

void ImguiPass::graphics(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	rhi.begin_default_render_pass(ctx, false);
	rhi.bind_pipeline(ctx, pipeline_);
	for (auto& draw_list : draw_lists_)
	{
		rhi.bind_uniform_set(ctx, 0, draw_list.us_1);
		rhi.bind_uniform_set(ctx, 1, draw_list.us_2);
		for (auto& cmd : draw_list.cmds)
		{
			rhi.bind_binding_set(ctx, cmd.binding_set);
			rhi.bind_index_buffer(ctx, draw_list.ibo);
			rhi.set_scissor(ctx, cmd.clip);
			rhi.draw_indexed(ctx, cmd.elems, cmd.i_offset);
		}
	}
	rhi.end_render_pass(ctx);
}

void ImguiPass::postpass(Rhi& rhi)
{
	for (auto& list : draw_lists_)
	{
		rhi.destroy_buffer(list.vbo);
		rhi.destroy_buffer(list.ibo);
	}
	draw_lists_.clear();
}
