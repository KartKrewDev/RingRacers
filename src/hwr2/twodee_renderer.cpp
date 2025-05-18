// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "twodee_renderer.hpp"

#include <algorithm>

#include <stb_rect_pack.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../core/hash_set.hpp"
#include "blendmode.hpp"
#include "../r_patch.h"
#include "../v_video.h"
#include "../z_zone.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

TwodeeRenderer::TwodeeRenderer(
	srb2::NotNull<PaletteManager*> palette_manager,
	srb2::NotNull<FlatTextureManager*> flat_manager,
	srb2::NotNull<PatchAtlasCache*> patch_atlas_cache
) : palette_manager_(palette_manager), flat_manager_(flat_manager), patch_atlas_cache_(patch_atlas_cache)
{}
TwodeeRenderer::TwodeeRenderer(TwodeeRenderer&&) = default;
TwodeeRenderer::~TwodeeRenderer() = default;
TwodeeRenderer& TwodeeRenderer::operator=(TwodeeRenderer&&) = default;

static constexpr const uint32_t kVboInitSize = 32768;
static constexpr const uint32_t kIboInitSize = 4096;

static TwodeePipelineKey pipeline_key_for_cmd(const Draw2dCmd& cmd)
{
	return {hwr2::get_blend_mode(cmd), hwr2::is_draw_lines(cmd)};
}

static void set_blend_state(RasterizerStateDesc& desc, BlendMode blend)
{
	switch (blend)
	{
	case BlendMode::kAlphaTransparent:
		desc.blend_source_factor_color = BlendFactor::kSourceAlpha;
		desc.blend_dest_factor_color = BlendFactor::kOneMinusSourceAlpha;
		desc.blend_color_function = BlendFunction::kAdd;
		desc.blend_source_factor_alpha = BlendFactor::kOne;
		desc.blend_dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		desc.blend_alpha_function = BlendFunction::kAdd;
		break;
	case BlendMode::kModulate:
		desc.blend_source_factor_color = BlendFactor::kDest;
		desc.blend_dest_factor_color = BlendFactor::kZero;
		desc.blend_color_function = BlendFunction::kAdd;
		desc.blend_source_factor_alpha = BlendFactor::kDestAlpha;
		desc.blend_dest_factor_alpha = BlendFactor::kZero;
		desc.blend_alpha_function = BlendFunction::kAdd;
		break;
	case BlendMode::kAdditive:
		desc.blend_source_factor_color = BlendFactor::kSourceAlpha;
		desc.blend_dest_factor_color = BlendFactor::kOne;
		desc.blend_color_function = BlendFunction::kAdd;
		desc.blend_source_factor_alpha = BlendFactor::kOne;
		desc.blend_dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		desc.blend_alpha_function = BlendFunction::kAdd;
		break;
	case BlendMode::kSubtractive:
		desc.blend_source_factor_color = BlendFactor::kSourceAlpha;
		desc.blend_dest_factor_color = BlendFactor::kOne;
		desc.blend_color_function = BlendFunction::kSubtract;
		desc.blend_source_factor_alpha = BlendFactor::kOne;
		desc.blend_dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		desc.blend_alpha_function = BlendFunction::kAdd;
		break;
	case BlendMode::kReverseSubtractive:
		desc.blend_source_factor_color = BlendFactor::kSourceAlpha;
		desc.blend_dest_factor_color = BlendFactor::kOne;
		desc.blend_color_function = BlendFunction::kReverseSubtract;
		desc.blend_source_factor_alpha = BlendFactor::kOne;
		desc.blend_dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		desc.blend_alpha_function = BlendFunction::kAdd;
		break;
	case BlendMode::kInvertDest:
		desc.blend_source_factor_color = BlendFactor::kOne;
		desc.blend_dest_factor_color = BlendFactor::kOne;
		desc.blend_color_function = BlendFunction::kSubtract;
		desc.blend_source_factor_alpha = BlendFactor::kZero;
		desc.blend_dest_factor_alpha = BlendFactor::kDestAlpha;
		desc.blend_alpha_function = BlendFunction::kAdd;
		break;
	}
}

void TwodeeRenderer::rewrite_patch_quad_vertices(Draw2dList& list, const Draw2dPatchQuad& cmd) const
{
	// Patch quads are clipped according to the patch's atlas entry
	const patch_t* patch = cmd.patch;
	if (patch == nullptr)
	{
		return;
	}

	srb2::NotNull<const PatchAtlas*> atlas = patch_atlas_cache_->find_patch(patch);
	std::optional<PatchAtlas::Entry> entry_optional = atlas->find_patch(patch);
	SRB2_ASSERT(entry_optional.has_value());
	PatchAtlas::Entry entry = *entry_optional;

	// Rewrite the vertex data completely.
	// The UVs of the trimmed patch in atlas UV space.
	const float atlas_umin = static_cast<float>(entry.x) / atlas->texture_size();
	const float atlas_umax = static_cast<float>(entry.x + entry.w) / atlas->texture_size();
	const float atlas_vmin = static_cast<float>(entry.y) / atlas->texture_size();
	const float atlas_vmax = static_cast<float>(entry.y + entry.h) / atlas->texture_size();

	// The UVs of the trimmed patch in untrimmed UV space.
	// The command's UVs are in untrimmed UV space.
	const float trim_umin = static_cast<float>(entry.trim_x) / entry.orig_w;
	const float trim_umax = static_cast<float>(entry.trim_x + entry.w) / entry.orig_w;
	const float trim_vmin = static_cast<float>(entry.trim_y) / entry.orig_h;
	const float trim_vmax = static_cast<float>(entry.trim_y + entry.h) / entry.orig_h;

	// Calculate positions
	const float cmd_xrange = cmd.xmax - cmd.xmin;
	const float cmd_yrange = cmd.ymax - cmd.ymin;

	const float clipped_xmin = cmd.clip ? std::clamp(cmd.xmin, cmd.clip_xmin, std::max(cmd.clip_xmax, cmd.clip_xmin)) : cmd.xmin;
	const float clipped_xmax = cmd.clip ? std::clamp(cmd.xmax, cmd.clip_xmin, std::max(cmd.clip_xmax, cmd.clip_xmin)) : cmd.xmax;
	const float clipped_ymin = cmd.clip ? std::clamp(cmd.ymin, cmd.clip_ymin, std::max(cmd.clip_ymax, cmd.clip_ymin)) : cmd.ymin;
	const float clipped_ymax = cmd.clip ? std::clamp(cmd.ymax, cmd.clip_ymin, std::max(cmd.clip_ymax, cmd.clip_ymin)) : cmd.ymax;

	const float trimmed_left = cmd.flip ? (1.f - trim_umax) : trim_umin;
	const float trimmed_right = cmd.flip ? trim_umin : (1.f - trim_umax);
	const float trimmed_top = cmd.vflip ? (1.f - trim_vmax) : trim_vmin;
	const float trimmed_bottom = cmd.vflip ? trim_vmin : (1.f - trim_vmax);

	const float trimmed_xmin = cmd.xmin + trimmed_left * cmd_xrange;
	const float trimmed_xmax = cmd.xmax - trimmed_right * cmd_xrange;
	const float trimmed_ymin = cmd.ymin + trimmed_top * cmd_yrange;
	const float trimmed_ymax = cmd.ymax - trimmed_bottom * cmd_yrange;

	const float trimmed_xrange = trimmed_xmax - trimmed_xmin;
	const float trimmed_yrange = trimmed_ymax - trimmed_ymin;

	float clipped_trimmed_xmin = std::max(clipped_xmin, trimmed_xmin);
	float clipped_trimmed_xmax = std::min(clipped_xmax, trimmed_xmax);
	float clipped_trimmed_ymin = std::max(clipped_ymin, trimmed_ymin);
	float clipped_trimmed_ymax = std::min(clipped_ymax, trimmed_ymax);

	clipped_trimmed_xmin = std::min(clipped_trimmed_xmin, clipped_trimmed_xmax);
	clipped_trimmed_ymin = std::min(clipped_trimmed_ymin, clipped_trimmed_ymax);

	// Calculate UVs
	// Start from trimmed dimensions as 0..1 and clip UVs based on that
	// UVs in trimmed UV space (if clipped_xmin = trimmed_xmin, it'll be 0)
	float clipped_umin;
	float clipped_umax;
	float clipped_vmin;
	float clipped_vmax;

	if (cmd.flip)
	{
		clipped_umin = std::max(0.f, 1.f - (clipped_trimmed_xmin - trimmed_xmin) / trimmed_xrange);
		clipped_umax = std::min(1.f, (trimmed_xmax - clipped_trimmed_xmax) / trimmed_xrange);
	}
	else
	{
		clipped_umin = std::min(1.f, (clipped_trimmed_xmin - trimmed_xmin) / trimmed_xrange);
		clipped_umax = std::max(0.f, 1.f - (trimmed_xmax - clipped_trimmed_xmax) / trimmed_xrange);
	}

	if (cmd.vflip)
	{
		clipped_vmin = std::max(0.f, 1.f - (clipped_trimmed_ymin - trimmed_ymin) / trimmed_yrange);
		clipped_vmax = std::min(1.f, (trimmed_ymax - clipped_trimmed_ymax) / trimmed_yrange);
	}
	else
	{
		clipped_vmin = std::min(1.f, 0.f + (clipped_trimmed_ymin - trimmed_ymin) / trimmed_yrange);
		clipped_vmax = std::max(0.f, 1.f - (trimmed_ymax - clipped_trimmed_ymax) / trimmed_yrange);
	}

	// convert from trimmed UV space to atlas space
	clipped_umin = (atlas_umax - atlas_umin) * clipped_umin + atlas_umin;
	clipped_umax = (atlas_umax - atlas_umin) * clipped_umax + atlas_umin;
	clipped_vmin = (atlas_vmax - atlas_vmin) * clipped_vmin + atlas_vmin;
	clipped_vmax = (atlas_vmax - atlas_vmin) * clipped_vmax + atlas_vmin;

	std::size_t vtx_offs = cmd.begin_index;
	// Vertex order is always min/min, max/min, max/max, min/max
	list.vertices[vtx_offs + 0].x = clipped_trimmed_xmin;
	list.vertices[vtx_offs + 0].y = clipped_trimmed_ymin;
	list.vertices[vtx_offs + 0].u = clipped_umin;
	list.vertices[vtx_offs + 0].v = clipped_vmin;
	list.vertices[vtx_offs + 1].x = clipped_trimmed_xmax;
	list.vertices[vtx_offs + 1].y = clipped_trimmed_ymin;
	list.vertices[vtx_offs + 1].u = clipped_umax;
	list.vertices[vtx_offs + 1].v = clipped_vmin;
	list.vertices[vtx_offs + 2].x = clipped_trimmed_xmax;
	list.vertices[vtx_offs + 2].y = clipped_trimmed_ymax;
	list.vertices[vtx_offs + 2].u = clipped_umax;
	list.vertices[vtx_offs + 2].v = clipped_vmax;
	list.vertices[vtx_offs + 3].x = clipped_trimmed_xmin;
	list.vertices[vtx_offs + 3].y = clipped_trimmed_ymax;
	list.vertices[vtx_offs + 3].u = clipped_umin;
	list.vertices[vtx_offs + 3].v = clipped_vmax;
}

void TwodeeRenderer::initialize(Rhi& rhi)
{
	ProgramDesc prog_desc;
	prog_desc.name = "unshadedpaletted";
	const char* defines[] = {
		"ENABLE_U_SAMPLER0_IS_INDEXED_ALPHA",
		"ENABLE_S_SAMPLER2",
		"ENABLE_VA_TEXCOORD0",
		"ENABLE_VA_COLOR"
	};
	prog_desc.defines = tcb::make_span(defines);
	program_ = rhi.create_program(prog_desc);

	{
		default_tex_ = rhi.create_texture({
			TextureFormat::kLuminanceAlpha,
			2,
			1,
			TextureWrapMode::kClamp,
			TextureWrapMode::kClamp
		});
		std::array<uint8_t, 4> data = {0, 255, 0, 255};
		rhi.update_texture(default_tex_, {0, 0, 2, 1}, PixelFormat::kRG8, tcb::as_bytes(tcb::span(data)));
	}

	initialized_ = true;
}

void TwodeeRenderer::flush(Rhi& rhi, Twodee& twodee)
{
	if (!initialized_)
	{
		initialize(rhi);
	}

	// Stage 1 - command list patch detection
	srb2::HashSet<const patch_t*> found_patches;
	for (const auto& list : twodee)
	{
		for (const auto& cmd : list.cmds)
		{
			auto visitor = srb2::Overload {
				[&](const Draw2dPatchQuad& cmd)
				{
					if (cmd.patch != nullptr)
					{
						found_patches.insert(cmd.patch);
					}
					if (cmd.colormap != nullptr)
					{
						palette_manager_->find_or_create_colormap(rhi, cmd.colormap);
					}
				},
				[&](const Draw2dVertices& cmd) {}};
			std::visit(visitor, cmd);
		}
	}

	for (auto patch : found_patches)
	{
		patch_atlas_cache_->queue_patch(patch);
	}
	patch_atlas_cache_->pack(rhi);

	size_t list_index = 0;
	for (auto& list : twodee)
	{
		Handle<Buffer> vbo;
		uint32_t vertex_data_size = tcb::as_bytes(tcb::span(list.vertices)).size();
		uint32_t needed_vbo_size = std::max(
			kVboInitSize,
			((static_cast<uint32_t>(vertex_data_size) + kVboInitSize - 1) / kVboInitSize) * kVboInitSize
		);

		// Get the existing buffer objects. Recreate them if they don't exist, or needs to be bigger.

		if (list_index >= vbos_.size())
		{
			vbo = rhi.create_buffer({needed_vbo_size, BufferType::kVertexBuffer, BufferUsage::kDynamic});
			vbos_.push_back({vbo, needed_vbo_size});
		}
		else
		{
			uint32_t existing_size = std::get<1>(vbos_[list_index]);
			if (needed_vbo_size > existing_size)
			{
				rhi.destroy_buffer(std::get<0>(vbos_[list_index]));
				vbo = rhi.create_buffer({needed_vbo_size, BufferType::kVertexBuffer, BufferUsage::kDynamic});
				vbos_[list_index] = {vbo, needed_vbo_size};
			}
			vbo = std::get<0>(vbos_[list_index]);
		}

		Handle<Buffer> ibo;
		uint32_t index_data_size = tcb::as_bytes(tcb::span(list.indices)).size();
		uint32_t needed_ibo_size = std::max(
			kIboInitSize,
			((static_cast<uint32_t>(index_data_size) + kIboInitSize - 1) / kIboInitSize) * kIboInitSize
		);

		if (list_index >= ibos_.size())
		{
			ibo = rhi.create_buffer({needed_ibo_size, BufferType::kIndexBuffer, BufferUsage::kDynamic});
			ibos_.push_back({ibo, needed_ibo_size});
		}
		else
		{
			uint32_t existing_size = std::get<1>(ibos_[list_index]);
			if (needed_ibo_size > existing_size)
			{
				rhi.destroy_buffer(std::get<0>(ibos_[list_index]));
				ibo = rhi.create_buffer({needed_ibo_size, BufferType::kIndexBuffer, BufferUsage::kDynamic});
				ibos_[list_index] = {ibo, needed_ibo_size};
			}
			ibo = std::get<0>(ibos_[list_index]);
		}

		// Create a merged command list
		MergedTwodeeCommandList merged_list;
		merged_list.vbo = vbo;
		merged_list.vbo_size = needed_vbo_size;
		merged_list.ibo = ibo;
		merged_list.ibo_size = needed_ibo_size;

		MergedTwodeeCommand new_cmd;
		new_cmd.index_offset = 0;
		new_cmd.elements = 0;
		new_cmd.colormap = nullptr;
		// safety: a command list is required to have at least 1 command
		new_cmd.pipeline_key = pipeline_key_for_cmd(list.cmds[0]);
		new_cmd.primitive = new_cmd.pipeline_key.lines ? PrimitiveType::kLines : PrimitiveType::kTriangles;
		new_cmd.blend_mode = new_cmd.pipeline_key.blend;
		merged_list.cmds.push_back(std::move(new_cmd));

		for (auto& cmd : list.cmds)
		{
			auto& merged_cmd = *merged_list.cmds.rbegin();
			bool new_cmd_needed = false;
			TwodeePipelineKey pk = pipeline_key_for_cmd(cmd);
			new_cmd_needed = new_cmd_needed || (pk != merged_cmd.pipeline_key);

			// We need to split the merged commands based on the kind of texture
			// Patches are converted to atlas texture indexes, which we've just packed the patch rects for
			// Flats are uploaded as individual textures.
			auto tex_visitor = srb2::Overload {
				[&](const Draw2dPatchQuad& cmd)
				{
					if (cmd.patch == nullptr)
					{
						new_cmd_needed = new_cmd_needed || (merged_cmd.texture != std::nullopt);
					}
					else
					{
						srb2::NotNull<const PatchAtlas*> atlas = patch_atlas_cache_->find_patch(cmd.patch);
						std::optional<MergedTwodeeCommand::Texture> atlas_index_texture = atlas->texture();
						new_cmd_needed = new_cmd_needed || (merged_cmd.texture != atlas_index_texture);
					}

					new_cmd_needed = new_cmd_needed || (merged_cmd.colormap != cmd.colormap);
				},
				[&](const Draw2dVertices& cmd)
				{
					if (cmd.flat_lump == LUMPERROR)
					{
						new_cmd_needed |= (merged_cmd.texture != std::nullopt);
					}
					else
					{
						std::optional<MergedTwodeeCommand::Texture> flat_tex = MergedTwodeeCommandFlatTexture {cmd.flat_lump};
						new_cmd_needed |= (merged_cmd.texture != flat_tex);
					}

					new_cmd_needed = new_cmd_needed || (merged_cmd.colormap != nullptr);
				}};
			std::visit(tex_visitor, cmd);

			if (new_cmd_needed)
			{
				MergedTwodeeCommand the_new_one;
				the_new_one.index_offset = merged_cmd.index_offset + merged_cmd.elements;

				// Map to the merged version of the texture variant. Yay...!
				auto tex_visitor_again = srb2::Overload {
					[&](const Draw2dPatchQuad& cmd)
					{
						if (cmd.patch != nullptr)
						{
							srb2::NotNull<const PatchAtlas*> atlas = patch_atlas_cache_->find_patch(cmd.patch);
							the_new_one.texture = atlas->texture();
						}
						else
						{
							the_new_one.texture = std::nullopt;
						}
						the_new_one.colormap = cmd.colormap;
					},
					[&](const Draw2dVertices& cmd)
					{
						if (cmd.flat_lump != LUMPERROR)
						{
							flat_manager_->find_or_create_indexed(rhi, cmd.flat_lump);
							std::optional<MergedTwodeeCommand::Texture> t = MergedTwodeeCommandFlatTexture {cmd.flat_lump};
							the_new_one.texture = t;
						}
						else
						{
							the_new_one.texture = std::nullopt;
						}

						the_new_one.colormap = nullptr;
					}};
				std::visit(tex_visitor_again, cmd);
				the_new_one.pipeline_key = pipeline_key_for_cmd(cmd);
				the_new_one.primitive = the_new_one.pipeline_key.lines ? PrimitiveType::kLines : PrimitiveType::kTriangles;
				the_new_one.blend_mode = the_new_one.pipeline_key.blend;
				merged_list.cmds.push_back(std::move(the_new_one));
			}

			// There may or may not be a new current command; update its element count
			auto& new_merged_cmd = *merged_list.cmds.rbegin();
			// We know for sure that all commands in a command list have a contiguous range of elements in the IBO
			// So we can draw them in batch if the pipeline key and textures match
			new_merged_cmd.elements += hwr2::elements(cmd);

			// Perform coordinate transformations
			{
				auto vtx_transform_visitor = srb2::Overload {
					[&](const Draw2dPatchQuad& cmd) { rewrite_patch_quad_vertices(list, cmd); },
					[&](const Draw2dVertices& cmd) {}};
				std::visit(vtx_transform_visitor, cmd);
			}
		}

		cmd_lists_.push_back(std::move(merged_list));

		list_index++;
	}

	Handle<Texture> palette_tex = palette_manager_->palette();

	// Update the buffers for each list
	auto ctx_list_itr = twodee.begin();
	for (size_t i = 0; i < cmd_lists_.size() && ctx_list_itr != twodee.end(); i++)
	{
		auto& merged_list = cmd_lists_[i];
		auto& orig_list = *ctx_list_itr;

		tcb::span<const std::byte> vertex_data = tcb::as_bytes(tcb::span(orig_list.vertices));
		tcb::span<const std::byte> index_data = tcb::as_bytes(tcb::span(orig_list.indices));
		rhi.update_buffer(merged_list.vbo, 0, vertex_data);
		rhi.update_buffer(merged_list.ibo, 0, index_data);

		for (auto& mcmd : merged_list.cmds)
		{
			auto tex_visitor = srb2::Overload {
				[&](Handle<Texture> texture)
				{
					mcmd.texture_handle = texture;
				},
				[&](const MergedTwodeeCommandFlatTexture& tex)
				{
					Handle<Texture> th = flat_manager_->find_or_create_indexed(rhi, tex.lump);
					SRB2_ASSERT(th != kNullHandle);
					mcmd.texture_handle = th;
				}};
			if (mcmd.texture)
			{
				std::visit(tex_visitor, *mcmd.texture);
			}
			else
			{
				mcmd.texture_handle = default_tex_;
			}

			const uint8_t* colormap = mcmd.colormap;
			Handle<Texture> colormap_h = palette_manager_->default_colormap();
			if (colormap)
			{
				colormap_h = palette_manager_->find_or_create_colormap(rhi, colormap);
				SRB2_ASSERT(colormap_h != kNullHandle);
			}
			mcmd.colormap_handle = colormap_h;
		}

		ctx_list_itr++;
	}

	// Presumably, we're already in a renderpass when flush is called
	rhi.bind_program(program_);
	rhi.set_uniform("u_projection", glm::mat4(
		glm::vec4(2.f / vid.width, 0.f, 0.f, 0.f),
		glm::vec4(0.f, -2.f / vid.height, 0.f, 0.f),
		glm::vec4(0.f, 0.f, 1.f, 0.f),
		glm::vec4(-1.f, 1.f, 0.f, 1.f)
	));
	rhi.set_uniform("u_modelview", glm::identity<glm::mat4>());
	rhi.set_uniform("u_texcoord0_transform", glm::identity<glm::mat3>());
	rhi.set_uniform("u_sampler0_is_indexed_alpha", static_cast<int32_t>(1));
	rhi.set_sampler("s_sampler1", 1, palette_tex);
	for (auto& list : cmd_lists_)
	{
		rhi.bind_vertex_attrib("a_position", list.vbo, VertexAttributeFormat::kFloat3, offsetof(TwodeeVertex, x), sizeof(TwodeeVertex));
		rhi.bind_vertex_attrib("a_texcoord0", list.vbo, VertexAttributeFormat::kFloat2, offsetof(TwodeeVertex, u), sizeof(TwodeeVertex));
		rhi.bind_vertex_attrib("a_color", list.vbo, VertexAttributeFormat::kFloat4, offsetof(TwodeeVertex, r), sizeof(TwodeeVertex));
		for (auto& cmd : list.cmds)
		{
			if (cmd.elements == 0)
			{
				// Don't do anything for 0-element commands
				// This shouldn't happen, but, just in case...
				continue;
			}
			RasterizerStateDesc desc;
			desc.cull = CullMode::kNone;
			desc.primitive = cmd.primitive;
			desc.blend_enabled = true;
			set_blend_state(desc, cmd.blend_mode);
			// Set blend and primitives
			rhi.set_rasterizer_state(desc);
			rhi.set_viewport({0, 0, static_cast<uint32_t>(vid.width), static_cast<uint32_t>(vid.height)});
			rhi.set_sampler("s_sampler0", 0, cmd.texture_handle);
			rhi.set_sampler("s_sampler2", 2, cmd.colormap_handle);
			rhi.bind_index_buffer(list.ibo);
			rhi.draw_indexed(cmd.elements, cmd.index_offset);
		}
	}

	cmd_lists_.clear();

	// Reset context for next drawing batch
	twodee = Twodee();

	// Reset the patch atlas if needed
	if (patch_atlas_cache_->need_to_reset())
	{
		patch_atlas_cache_->reset(rhi);
	}
}
