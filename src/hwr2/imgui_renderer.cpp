// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "imgui_renderer.hpp"

#include <imgui.h>
#include <tcb/span.hpp>

#include "../v_video.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

ImguiRenderer::ImguiRenderer()
{
}

ImguiRenderer::~ImguiRenderer() = default;

void ImguiRenderer::render(Rhi& rhi)
{

	if (!program_)
	{
		const char* defines[2] = {
			"ENABLE_VA_TEXCOORD0",
			"ENABLE_VA_COLOR"
		};
		ProgramDesc desc;
		desc.name = "unshaded";
		desc.defines = tcb::make_span(defines);
		program_ = rhi.create_program(desc);
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

	if (!default_tex_)
	{
		uint32_t pixel = 0xFFFFFFFF;
		default_tex_ = rhi.create_texture({
			TextureFormat::kRGBA,
			1,
			1,
			TextureWrapMode::kRepeat,
			TextureWrapMode::kRepeat
		});
		rhi.update_texture(default_tex_, {0, 0, 1, 1}, rhi::PixelFormat::kRGBA8, tcb::as_bytes(tcb::span(&pixel, 1)));
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
			if (tex_id == 0)
			{
				draw_cmd.tex = default_tex_;
			}
			else
			{
				draw_cmd.tex = tex_id;
			}
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

	{
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		rhi.update_texture(
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
		rhi.update_buffer(vbo, 0, tcb::as_bytes(vert_span));

		tcb::span<ImDrawIdx> index_span = tcb::span(im_list->IdxBuffer.Data, im_list->IdxBuffer.size());
		rhi.update_buffer(ibo, 0, tcb::as_bytes(index_span));

		for (auto& draw_cmd : draw_list.cmds)
		{
			draw_cmd.vbo = vbo;
			draw_cmd.ibo = ibo;
			draw_cmd.tex = draw_cmd.tex;
		}
	}

	rhi.bind_program(program_);
	RasterizerStateDesc rs;

	rs.cull = CullMode::kNone;
	rs.winding = FaceWinding::kCounterClockwise;
	rs.depth_test = true;
	rs.depth_func = CompareFunc::kAlways;
	rs.blend_enabled = true;
	rs.blend_source_factor_color = BlendFactor::kSourceAlpha;
	rs.blend_dest_factor_color = BlendFactor::kOneMinusSourceAlpha;
	rs.blend_color_function = BlendFunction::kAdd;
	rs.blend_source_factor_alpha = BlendFactor::kOne;
	rs.blend_dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
	rs.blend_alpha_function = BlendFunction::kAdd;

	for (auto& draw_list : draw_lists_)
	{
		glm::mat4 projection = glm::mat4(
			glm::vec4(2.f / vid.realwidth, 0.f, 0.f, 0.f),
			glm::vec4(0.f, 2.f / vid.realheight, 0.f, 0.f),
			glm::vec4(0.f, 0.f, 1.f, 0.f),
			glm::vec4(-1.f, 1.f, 0.f, 1.f)
		);
		glm::mat4 modelview = glm::mat4(
			glm::vec4(1.f, 0.f, 0.f, 0.f),
			glm::vec4(0.f, -1.f, 0.f, 0.f),
			glm::vec4(0.f, 0.f, 1.f, 0.f),
			glm::vec4(0.f, 0, 0.f, 1.f)
		);
		glm::mat3 texcoord0_transform = glm::mat3(
			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(0.f, 1.f, 0.f),
			glm::vec3(0.f, 0.f, 1.f)
		);
		rhi.set_uniform("u_projection", projection);
		rhi.set_uniform("u_modelview", modelview);
		rhi.set_uniform("u_texcoord0_transform", texcoord0_transform);
		for (auto& cmd : draw_list.cmds)
		{
			rs.scissor_test = true;
			rs.scissor = cmd.clip;
			rhi.set_rasterizer_state(rs);
			rhi.bind_vertex_attrib("a_position", cmd.vbo, VertexAttributeFormat::kFloat3, offsetof(ImDrawVert, pos), sizeof(ImDrawVert));
			rhi.bind_vertex_attrib("a_texcoord0", cmd.vbo, VertexAttributeFormat::kFloat2, offsetof(ImDrawVert, uv), sizeof(ImDrawVert));
			rhi.bind_vertex_attrib("a_color", cmd.vbo, VertexAttributeFormat::kFloat4, offsetof(ImDrawVert, colf), sizeof(ImDrawVert));
			rhi.set_sampler("s_sampler0", 0, cmd.tex);
			rhi.bind_index_buffer(draw_list.ibo);
			rhi.draw_indexed(cmd.elems, cmd.i_offset);
		}
	}

	for (auto& list : draw_lists_)
	{
		rhi.destroy_buffer(list.vbo);
		rhi.destroy_buffer(list.ibo);
	}
	draw_lists_.clear();
}
