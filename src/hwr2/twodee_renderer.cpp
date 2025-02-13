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
#include <unordered_set>

#include <stb_rect_pack.h>
#include <glm/gtc/matrix_transform.hpp>

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

static PipelineDesc make_pipeline_desc(TwodeePipelineKey key)
{
	constexpr const VertexInputDesc kTwodeeVertexInput = {
		{{sizeof(TwodeeVertex)}},
		{{VertexAttributeName::kPosition, 0, 0},
		 {VertexAttributeName::kTexCoord0, 0, 12},
		 {VertexAttributeName::kColor, 0, 20}}};
	BlendDesc blend_desc;
	switch (key.blend)
	{
	case BlendMode::kAlphaTransparent:
		blend_desc.source_factor_color = BlendFactor::kSourceAlpha;
		blend_desc.dest_factor_color = BlendFactor::kOneMinusSourceAlpha;
		blend_desc.color_function = BlendFunction::kAdd;
		blend_desc.source_factor_alpha = BlendFactor::kOne;
		blend_desc.dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	case BlendMode::kModulate:
		blend_desc.source_factor_color = BlendFactor::kDest;
		blend_desc.dest_factor_color = BlendFactor::kZero;
		blend_desc.color_function = BlendFunction::kAdd;
		blend_desc.source_factor_alpha = BlendFactor::kDestAlpha;
		blend_desc.dest_factor_alpha = BlendFactor::kZero;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	case BlendMode::kAdditive:
		blend_desc.source_factor_color = BlendFactor::kSourceAlpha;
		blend_desc.dest_factor_color = BlendFactor::kOne;
		blend_desc.color_function = BlendFunction::kAdd;
		blend_desc.source_factor_alpha = BlendFactor::kOne;
		blend_desc.dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	case BlendMode::kSubtractive:
		blend_desc.source_factor_color = BlendFactor::kSourceAlpha;
		blend_desc.dest_factor_color = BlendFactor::kOne;
		blend_desc.color_function = BlendFunction::kSubtract;
		blend_desc.source_factor_alpha = BlendFactor::kOne;
		blend_desc.dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	case BlendMode::kReverseSubtractive:
		blend_desc.source_factor_color = BlendFactor::kSourceAlpha;
		blend_desc.dest_factor_color = BlendFactor::kOne;
		blend_desc.color_function = BlendFunction::kReverseSubtract;
		blend_desc.source_factor_alpha = BlendFactor::kOne;
		blend_desc.dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	case BlendMode::kInvertDest:
		blend_desc.source_factor_color = BlendFactor::kOne;
		blend_desc.dest_factor_color = BlendFactor::kOne;
		blend_desc.color_function = BlendFunction::kSubtract;
		blend_desc.source_factor_alpha = BlendFactor::kZero;
		blend_desc.dest_factor_alpha = BlendFactor::kDestAlpha;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	}

	return {
		PipelineProgram::kUnshadedPaletted,
		kTwodeeVertexInput,
		{{{UniformName::kProjection},
		  {{UniformName::kModelView, UniformName::kTexCoord0Transform, UniformName::kSampler0IsIndexedAlpha}}}},
		{{SamplerName::kSampler0, SamplerName::kSampler1, SamplerName::kSampler2}},
		std::nullopt,
		{blend_desc, {true, true, true, true}},
		key.lines ? PrimitiveType::kLines : PrimitiveType::kTriangles,
		CullMode::kNone,
		FaceWinding::kCounterClockwise,
		{0.f, 0.f, 0.f, 1.f}};
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

	const float clipped_xmin = cmd.clip ? std::clamp(cmd.xmin, cmd.clip_xmin, cmd.clip_xmax) : cmd.xmin;
	const float clipped_xmax = cmd.clip ? std::clamp(cmd.xmax, cmd.clip_xmin, cmd.clip_xmax) : cmd.xmax;
	const float clipped_ymin = cmd.clip ? std::clamp(cmd.ymin, cmd.clip_ymin, cmd.clip_ymax) : cmd.ymin;
	const float clipped_ymax = cmd.clip ? std::clamp(cmd.ymax, cmd.clip_ymin, cmd.clip_ymax) : cmd.ymax;

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

void TwodeeRenderer::initialize(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	{
		TwodeePipelineKey alpha_transparent_tris = {BlendMode::kAlphaTransparent, false};
		TwodeePipelineKey modulate_tris = {BlendMode::kModulate, false};
		TwodeePipelineKey additive_tris = {BlendMode::kAdditive, false};
		TwodeePipelineKey subtractive_tris = {BlendMode::kSubtractive, false};
		TwodeePipelineKey revsubtractive_tris = {BlendMode::kReverseSubtractive, false};
		TwodeePipelineKey invertdest_tris = {BlendMode::kInvertDest, false};
		TwodeePipelineKey alpha_transparent_lines = {BlendMode::kAlphaTransparent, true};
		TwodeePipelineKey modulate_lines = {BlendMode::kModulate, true};
		TwodeePipelineKey additive_lines = {BlendMode::kAdditive, true};
		TwodeePipelineKey subtractive_lines = {BlendMode::kSubtractive, true};
		TwodeePipelineKey revsubtractive_lines = {BlendMode::kReverseSubtractive, true};
		TwodeePipelineKey invertdest_lines = {BlendMode::kInvertDest, true};
		pipelines_.insert({alpha_transparent_tris, rhi.create_pipeline(make_pipeline_desc(alpha_transparent_tris))});
		pipelines_.insert({modulate_tris, rhi.create_pipeline(make_pipeline_desc(modulate_tris))});
		pipelines_.insert({additive_tris, rhi.create_pipeline(make_pipeline_desc(additive_tris))});
		pipelines_.insert({subtractive_tris, rhi.create_pipeline(make_pipeline_desc(subtractive_tris))});
		pipelines_.insert({revsubtractive_tris, rhi.create_pipeline(make_pipeline_desc(revsubtractive_tris))});
		pipelines_.insert({invertdest_tris, rhi.create_pipeline(make_pipeline_desc(invertdest_tris))});
		pipelines_.insert({alpha_transparent_lines, rhi.create_pipeline(make_pipeline_desc(alpha_transparent_lines))});
		pipelines_.insert({modulate_lines, rhi.create_pipeline(make_pipeline_desc(modulate_lines))});
		pipelines_.insert({additive_lines, rhi.create_pipeline(make_pipeline_desc(additive_lines))});
		pipelines_.insert({subtractive_lines, rhi.create_pipeline(make_pipeline_desc(subtractive_lines))});
		pipelines_.insert({revsubtractive_lines, rhi.create_pipeline(make_pipeline_desc(revsubtractive_lines))});
		pipelines_.insert({invertdest_lines, rhi.create_pipeline(make_pipeline_desc(revsubtractive_lines))});
	}

	{
		default_tex_ = rhi.create_texture({
			TextureFormat::kLuminanceAlpha,
			2,
			1,
			TextureWrapMode::kClamp,
			TextureWrapMode::kClamp
		});
		std::array<uint8_t, 4> data = {0, 255, 0, 255};
		rhi.update_texture(ctx, default_tex_, {0, 0, 2, 1}, PixelFormat::kRG8, tcb::as_bytes(tcb::span(data)));
	}

	initialized_ = true;
}

void TwodeeRenderer::flush(Rhi& rhi, Handle<GraphicsContext> ctx, Twodee& twodee)
{
	if (!initialized_)
	{
		initialize(rhi, ctx);
	}

	// Stage 1 - command list patch detection
	std::unordered_set<const patch_t*> found_patches;
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
						palette_manager_->find_or_create_colormap(rhi, ctx, cmd.colormap);
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
	patch_atlas_cache_->pack(rhi, ctx);

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
							flat_manager_->find_or_create_indexed(rhi, ctx, cmd.flat_lump);
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
		rhi.update_buffer(ctx, merged_list.vbo, 0, vertex_data);
		rhi.update_buffer(ctx, merged_list.ibo, 0, index_data);

		// Update the binding sets for each individual merged command
		VertexAttributeBufferBinding vbos[] = {{0, merged_list.vbo}};
		for (auto& mcmd : merged_list.cmds)
		{
			TextureBinding tx[3];
			auto tex_visitor = srb2::Overload {
				[&](Handle<Texture> texture)
				{
					tx[0] = {SamplerName::kSampler0, texture};
					tx[1] = {SamplerName::kSampler1, palette_tex};
				},
				[&](const MergedTwodeeCommandFlatTexture& tex)
				{
					Handle<Texture> th = flat_manager_->find_or_create_indexed(rhi, ctx, tex.lump);
					SRB2_ASSERT(th != kNullHandle);
					tx[0] = {SamplerName::kSampler0, th};
					tx[1] = {SamplerName::kSampler1, palette_tex};
				}};
			if (mcmd.texture)
			{
				std::visit(tex_visitor, *mcmd.texture);
			}
			else
			{
				tx[0] = {SamplerName::kSampler0, default_tex_};
				tx[1] = {SamplerName::kSampler1, palette_tex};
			}

			const uint8_t* colormap = mcmd.colormap;
			Handle<Texture> colormap_h = palette_manager_->default_colormap();
			if (colormap)
			{
				colormap_h = palette_manager_->find_or_create_colormap(rhi, ctx, colormap);
				SRB2_ASSERT(colormap_h != kNullHandle);
			}
			tx[2] = {SamplerName::kSampler2, colormap_h};
			mcmd.binding_set =
				rhi.create_binding_set(ctx, pipelines_[mcmd.pipeline_key], {tcb::span(vbos), tcb::span(tx)});
		}

		ctx_list_itr++;
	}

	// Uniform sets
	std::array<UniformVariant, 1> g1_uniforms = {
		// Projection
		glm::mat4(
			glm::vec4(2.f / vid.width, 0.f, 0.f, 0.f),
			glm::vec4(0.f, -2.f / vid.height, 0.f, 0.f),
			glm::vec4(0.f, 0.f, 1.f, 0.f),
			glm::vec4(-1.f, 1.f, 0.f, 1.f)
		),
	};
	std::array<UniformVariant, 3> g2_uniforms = {
		// ModelView
		glm::identity<glm::mat4>(),
		// Texcoord0 Transform
		glm::identity<glm::mat3>(),
		// Sampler 0 Is Indexed Alpha (yes, it always is)
		static_cast<int32_t>(1)
	};
	Handle<UniformSet> us_1 = rhi.create_uniform_set(ctx, {tcb::span(g1_uniforms)});
	Handle<UniformSet> us_2 = rhi.create_uniform_set(ctx, {tcb::span(g2_uniforms)});

	// Presumably, we're already in a renderpass when flush is called
	for (auto& list : cmd_lists_)
	{
		for (auto& cmd : list.cmds)
		{
			if (cmd.elements == 0)
			{
				// Don't do anything for 0-element commands
				// This shouldn't happen, but, just in case...
				continue;
			}
			SRB2_ASSERT(pipelines_.find(cmd.pipeline_key) != pipelines_.end());
			Handle<Pipeline> pl = pipelines_[cmd.pipeline_key];
			rhi.bind_pipeline(ctx, pl);
			rhi.set_viewport(ctx, {0, 0, static_cast<uint32_t>(vid.width), static_cast<uint32_t>(vid.height)});
			rhi.bind_uniform_set(ctx, 0, us_1);
			rhi.bind_uniform_set(ctx, 1, us_2);
			rhi.bind_binding_set(ctx, cmd.binding_set);
			rhi.bind_index_buffer(ctx, list.ibo);
			rhi.draw_indexed(ctx, cmd.elements, cmd.index_offset);
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
