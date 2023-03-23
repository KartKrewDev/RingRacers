// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "pass_twodee.hpp"

#include <unordered_set>

#include <stb_rect_pack.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../r_patch.h"
#include "../v_video.h"
#include "../z_zone.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

namespace
{

struct AtlasEntry
{
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;

	uint32_t trim_x;
	uint32_t trim_y;
	uint32_t orig_w;
	uint32_t orig_h;
};

struct Atlas
{
	Atlas() = default;
	Atlas(Atlas&&) = default;

	Handle<Texture> tex;
	uint32_t tex_width;
	uint32_t tex_height;
	std::unordered_map<const patch_t*, AtlasEntry> entries;

	std::unique_ptr<stbrp_context> rp_ctx {nullptr};
	std::unique_ptr<stbrp_node[]> rp_nodes {nullptr};

	Atlas& operator=(Atlas&&) = default;
};

} // namespace

struct srb2::hwr2::TwodeePassData
{
	Handle<Texture> default_tex;
	Handle<Texture> default_colormap_tex;
	std::vector<Atlas> patch_atlases;
	std::unordered_map<const patch_t*, size_t> patch_lookup;
	std::vector<const patch_t*> patches_to_upload;
	std::unordered_map<const uint8_t*, Handle<Texture>> colormaps;
	std::vector<const uint8_t*> colormaps_to_upload;
	std::unordered_map<TwodeePipelineKey, Handle<Pipeline>> pipelines;
	bool upload_default_tex = false;
};

std::shared_ptr<TwodeePassData> srb2::hwr2::make_twodee_pass_data()
{
	return std::make_shared<TwodeePassData>();
}

TwodeePass::TwodeePass() : Pass()
{
}

TwodeePass::~TwodeePass() = default;

static constexpr const uint32_t kVboInitSize = 32768;
static constexpr const uint32_t kIboInitSize = 4096;

static Rect trimmed_patch_dim(const patch_t* patch);

static void create_atlas(Rhi& rhi, TwodeePassData& pass_data)
{
	Atlas new_atlas;
	new_atlas.tex = rhi.create_texture({TextureFormat::kLuminanceAlpha, 2048, 2048});
	new_atlas.tex_width = 2048;
	new_atlas.tex_height = 2048;
	new_atlas.rp_ctx = std::make_unique<stbrp_context>();
	new_atlas.rp_nodes = std::make_unique<stbrp_node[]>(4096);
	for (size_t i = 0; i < 4096; i++)
	{
		new_atlas.rp_nodes[i] = {};
	}
	stbrp_init_target(new_atlas.rp_ctx.get(), 2048, 2048, new_atlas.rp_nodes.get(), 4096);
	// it is CRITICALLY important that the atlas is MOVED, not COPIED, otherwise the node ptrs will be broken
	pass_data.patch_atlases.push_back(std::move(new_atlas));
}

static void pack_patches(Rhi& rhi, TwodeePassData& pass_data, tcb::span<const patch_t*> patches)
{
	// Prepare stbrp rects for patches to be loaded.
	std::vector<stbrp_rect> rects;
	for (size_t i = 0; i < patches.size(); i++)
	{
		const patch_t* patch = patches[i];
		Rect trimmed_rect = trimmed_patch_dim(patch);
		stbrp_rect rect {};
		rect.id = i;
		rect.w = trimmed_rect.w;
		rect.h = trimmed_rect.h;
		rects.push_back(std::move(rect));
	}

	while (rects.size() > 0)
	{
		if (pass_data.patch_atlases.size() == 0)
		{
			create_atlas(rhi, pass_data);
		}

		for (size_t atlas_index = 0; atlas_index < pass_data.patch_atlases.size(); atlas_index++)
		{
			auto& atlas = pass_data.patch_atlases[atlas_index];

			stbrp_pack_rects(atlas.rp_ctx.get(), rects.data(), rects.size());
			for (auto itr = rects.begin(); itr != rects.end();)
			{
				auto& rect = *itr;
				if (rect.was_packed)
				{
					AtlasEntry entry;
					const patch_t* patch = patches[rect.id];
					// TODO prevent unnecessary recalculation of trim?
					Rect trimmed_rect = trimmed_patch_dim(patch);
					entry.x = static_cast<uint32_t>(rect.x);
					entry.y = static_cast<uint32_t>(rect.y);
					entry.w = static_cast<uint32_t>(rect.w);
					entry.h = static_cast<uint32_t>(rect.h);
					entry.trim_x = static_cast<uint32_t>(trimmed_rect.x);
					entry.trim_y = static_cast<uint32_t>(trimmed_rect.y);
					entry.orig_w = static_cast<uint32_t>(patch->width);
					entry.orig_h = static_cast<uint32_t>(patch->height);
					atlas.entries.insert_or_assign(patch, std::move(entry));
					pass_data.patch_lookup.insert_or_assign(patch, atlas_index);
					pass_data.patches_to_upload.push_back(patch);
					rects.erase(itr);
					continue;
				}
				++itr;
			}

			// If we still have rects to pack, and we're at the last atlas, create another atlas.
			// TODO This could end up in an infinite loop if the patches are bigger than an atlas. Such patches need to
			// be loaded as individual RHI textures instead.
			if (atlas_index == pass_data.patch_atlases.size() - 1 && rects.size() > 0)
			{
				create_atlas(rhi, pass_data);
			}
		}
	}
}

/// @brief Derive the subrect of the given patch with empty columns and rows excluded.
static Rect trimmed_patch_dim(const patch_t* patch)
{
	bool minx_found = false;
	int32_t minx = 0;
	int32_t maxx = 0;
	int32_t miny = patch->height;
	int32_t maxy = 0;
	for (int32_t x = 0; x < patch->width; x++)
	{
		const int32_t columnofs = patch->columnofs[x];
		const column_t* column = reinterpret_cast<const column_t*>(patch->columns + columnofs);

		// If the first pole is empty (topdelta = 255), there are no pixels in this column
		if (!minx_found && column->topdelta == 0xFF)
		{
			// Thus, the minx is at least one higher than the current column.
			minx = x + 1;
			continue;
		}
		minx_found = true;

		if (minx_found && column->topdelta != 0xFF)
		{
			maxx = x;
		}

		miny = std::min(static_cast<int32_t>(column->topdelta), miny);

		int32_t prevdelta = 0;
		int32_t topdelta = 0;
		while (column->topdelta != 0xFF)
		{
			topdelta = column->topdelta;

			// Tall patches hack
			if (topdelta <= prevdelta)
			{
				topdelta += prevdelta;
			}
			prevdelta = topdelta;

			maxy = std::max(topdelta + column->length, maxy);

			column = reinterpret_cast<const column_t*>(reinterpret_cast<const uint8_t*>(column) + column->length + 4);
		}
	}

	maxx += 1;
	maxx = std::max(minx, maxx);
	maxy = std::max(miny, maxy);

	return {minx, miny, static_cast<uint32_t>(maxx - minx), static_cast<uint32_t>(maxy - miny)};
}

static void convert_patch_to_trimmed_rg8_pixels(const patch_t* patch, std::vector<uint8_t>& out)
{
	Rect trimmed_rect = trimmed_patch_dim(patch);
	if (trimmed_rect.w % 2 > 0)
	{
		// In order to force 4-byte row alignment, an extra column is added to the image data.
		// Look up GL_UNPACK_ALIGNMENT (which defaults to 4 bytes)
		trimmed_rect.w += 1;
	}
	out.clear();
	// 2 bytes per pixel; 1 for the color index, 1 for the alpha. (RG8)
	out.resize(trimmed_rect.w * trimmed_rect.h * 2, 0);
	for (int32_t x = 0; x < static_cast<int32_t>(trimmed_rect.w) && x < (patch->width - trimmed_rect.x); x++)
	{
		const int32_t columnofs = patch->columnofs[x + trimmed_rect.x];
		const column_t* column = reinterpret_cast<const column_t*>(patch->columns + columnofs);

		int32_t prevdelta = 0;
		int32_t topdelta = 0;
		while (column->topdelta != 0xFF)
		{
			topdelta = column->topdelta;
			// prevdelta is used to implement tall patches hack
			if (topdelta <= prevdelta)
			{
				topdelta += prevdelta;
			}

			prevdelta = topdelta;
			const uint8_t* source = reinterpret_cast<const uint8_t*>(column) + 3;
			int32_t count = column->length; // is this byte order safe...?

			for (int32_t i = 0; i < count; i++)
			{
				int32_t output_y = topdelta + i - trimmed_rect.y;
				if (output_y < 0)
				{
					continue;
				}
				if (output_y >= static_cast<int32_t>(trimmed_rect.h))
				{
					break;
				}
				size_t pixel_index = (output_y * trimmed_rect.w + x) * 2;
				out[pixel_index + 0] = source[i]; // index in luminance/red channel
				out[pixel_index + 1] = 0xFF;	  // alpha/green value of 1
			}
			column = reinterpret_cast<const column_t*>(reinterpret_cast<const uint8_t*>(column) + column->length + 4);
		}
	}
}

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
	case Draw2dBlend::kAlphaTransparent:
		blend_desc.source_factor_color = BlendFactor::kSourceAlpha;
		blend_desc.dest_factor_color = BlendFactor::kOneMinusSourceAlpha;
		blend_desc.color_function = BlendFunction::kAdd;
		blend_desc.source_factor_alpha = BlendFactor::kOne;
		blend_desc.dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	case Draw2dBlend::kModulate:
		blend_desc.source_factor_color = BlendFactor::kDest;
		blend_desc.dest_factor_color = BlendFactor::kZero;
		blend_desc.color_function = BlendFunction::kAdd;
		blend_desc.source_factor_alpha = BlendFactor::kDestAlpha;
		blend_desc.dest_factor_alpha = BlendFactor::kZero;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	case Draw2dBlend::kAdditive:
		blend_desc.source_factor_color = BlendFactor::kSourceAlpha;
		blend_desc.dest_factor_color = BlendFactor::kOne;
		blend_desc.color_function = BlendFunction::kAdd;
		blend_desc.source_factor_alpha = BlendFactor::kOne;
		blend_desc.dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	case Draw2dBlend::kSubtractive:
		blend_desc.source_factor_color = BlendFactor::kSourceAlpha;
		blend_desc.dest_factor_color = BlendFactor::kOne;
		blend_desc.color_function = BlendFunction::kSubtract;
		blend_desc.source_factor_alpha = BlendFactor::kOne;
		blend_desc.dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	case Draw2dBlend::kReverseSubtractive:
		blend_desc.source_factor_color = BlendFactor::kSourceAlpha;
		blend_desc.dest_factor_color = BlendFactor::kOne;
		blend_desc.color_function = BlendFunction::kReverseSubtract;
		blend_desc.source_factor_alpha = BlendFactor::kOne;
		blend_desc.dest_factor_alpha = BlendFactor::kOneMinusSourceAlpha;
		blend_desc.alpha_function = BlendFunction::kAdd;
		break;
	case Draw2dBlend::kInvertDest:
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
		{{{{UniformName::kProjection}},
		  {{UniformName::kModelView, UniformName::kTexCoord0Transform, UniformName::kSampler0IsIndexedAlpha}}}},
		{{SamplerName::kSampler0, SamplerName::kSampler1, SamplerName::kSampler2}},
		std::nullopt,
		{PixelFormat::kRGBA8, blend_desc, {true, true, true, true}},
		key.lines ? PrimitiveType::kLines : PrimitiveType::kTriangles,
		CullMode::kNone,
		FaceWinding::kCounterClockwise,
		{0.f, 0.f, 0.f, 1.f}};
}

static void rewrite_patch_quad_vertices(Draw2dList& list, const Draw2dPatchQuad& cmd, TwodeePassData* data)
{
	// Patch quads are clipped according to the patch's atlas entry
	if (cmd.patch == nullptr)
	{
		return;
	}

	std::size_t atlas_index = data->patch_lookup[cmd.patch];
	auto& atlas = data->patch_atlases[atlas_index];
	auto& entry = atlas.entries[cmd.patch];

	// Rewrite the vertex data completely.
	// The UVs of the trimmed patch in atlas UV space.
	const float atlas_umin = static_cast<float>(entry.x) / atlas.tex_width;
	const float atlas_umax = static_cast<float>(entry.x + entry.w) / atlas.tex_width;
	const float atlas_vmin = static_cast<float>(entry.y) / atlas.tex_height;
	const float atlas_vmax = static_cast<float>(entry.y + entry.h) / atlas.tex_height;

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

void TwodeePass::prepass(Rhi& rhi)
{
	if (!ctx_ || !data_)
	{
		return;
	}

	if (data_->pipelines.size() == 0)
	{
		TwodeePipelineKey alpha_transparent_tris = {Draw2dBlend::kAlphaTransparent, false};
		TwodeePipelineKey modulate_tris = {Draw2dBlend::kModulate, false};
		TwodeePipelineKey additive_tris = {Draw2dBlend::kAdditive, false};
		TwodeePipelineKey subtractive_tris = {Draw2dBlend::kSubtractive, false};
		TwodeePipelineKey revsubtractive_tris = {Draw2dBlend::kReverseSubtractive, false};
		TwodeePipelineKey invertdest_tris = {Draw2dBlend::kInvertDest, false};
		TwodeePipelineKey alpha_transparent_lines = {Draw2dBlend::kAlphaTransparent, true};
		TwodeePipelineKey modulate_lines = {Draw2dBlend::kModulate, true};
		TwodeePipelineKey additive_lines = {Draw2dBlend::kAdditive, true};
		TwodeePipelineKey subtractive_lines = {Draw2dBlend::kSubtractive, true};
		TwodeePipelineKey revsubtractive_lines = {Draw2dBlend::kReverseSubtractive, true};
		TwodeePipelineKey invertdest_lines = {Draw2dBlend::kInvertDest, true};
		data_->pipelines.insert({alpha_transparent_tris, rhi.create_pipeline(make_pipeline_desc(alpha_transparent_tris))});
		data_->pipelines.insert({modulate_tris, rhi.create_pipeline(make_pipeline_desc(modulate_tris))});
		data_->pipelines.insert({additive_tris, rhi.create_pipeline(make_pipeline_desc(additive_tris))});
		data_->pipelines.insert({subtractive_tris, rhi.create_pipeline(make_pipeline_desc(subtractive_tris))});
		data_->pipelines.insert({revsubtractive_tris, rhi.create_pipeline(make_pipeline_desc(revsubtractive_tris))});
		data_->pipelines.insert({invertdest_tris, rhi.create_pipeline(make_pipeline_desc(invertdest_tris))});
		data_->pipelines.insert({alpha_transparent_lines, rhi.create_pipeline(make_pipeline_desc(alpha_transparent_lines))});
		data_->pipelines.insert({modulate_lines, rhi.create_pipeline(make_pipeline_desc(modulate_lines))});
		data_->pipelines.insert({additive_lines, rhi.create_pipeline(make_pipeline_desc(additive_lines))});
		data_->pipelines.insert({subtractive_lines, rhi.create_pipeline(make_pipeline_desc(subtractive_lines))});
		data_->pipelines.insert({revsubtractive_lines, rhi.create_pipeline(make_pipeline_desc(revsubtractive_lines))});
		data_->pipelines.insert({invertdest_lines, rhi.create_pipeline(make_pipeline_desc(revsubtractive_lines))});
	}

	if (!data_->default_tex)
	{
		data_->default_tex = rhi.create_texture({TextureFormat::kLuminanceAlpha, 2, 1});
		data_->upload_default_tex = true;
	}
	if (!data_->default_colormap_tex)
	{
		data_->default_colormap_tex = rhi.create_texture({TextureFormat::kLuminance, 256, 1});
		data_->upload_default_tex = true;
	}
	if (!render_pass_)
	{
		render_pass_ = rhi.create_render_pass(
			{std::nullopt, PixelFormat::kRGBA8, AttachmentLoadOp::kLoad, AttachmentStoreOp::kStore}
		);
	}

	// Check for patches that are being freed after this frame. Those patches must be present in the atlases for this
	// frame, but all atlases need to be cleared and rebuilt on next call to prepass.
	// This is based on the assumption that patches are very rarely freed during runtime; occasionally repacking the
	// atlases to free up space from patches that will never be referenced again is acceptable.
	if (rebuild_atlases_)
	{
		for (auto& atlas : data_->patch_atlases)
		{
			rhi.destroy_texture(atlas.tex);
		}
		data_->patch_atlases.clear();
		data_->patch_lookup.clear();
		rebuild_atlases_ = false;
	}

	if (data_->patch_atlases.size() > 2)
	{
		// Rebuild the atlases next frame because we have too many patches in the atlas cache.
		rebuild_atlases_ = true;
	}

	// Stage 1 - command list patch detection
	std::unordered_set<const patch_t*> found_patches;
	std::unordered_set<const uint8_t*> found_colormaps;
	for (const auto& list : *ctx_)
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
						found_colormaps.insert(cmd.colormap);
					}
				},
				[&](const Draw2dVertices& cmd) {}};
			std::visit(visitor, cmd);
		}
	}

	std::unordered_set<const patch_t*> patch_cache_hits;
	std::unordered_set<const patch_t*> patch_cache_misses;
	for (auto patch : found_patches)
	{
		if (data_->patch_lookup.find(patch) != data_->patch_lookup.end())
		{
			patch_cache_hits.insert(patch);
		}
		else
		{
			patch_cache_misses.insert(patch);
		}
	}

	for (auto colormap : found_colormaps)
	{
		if (data_->colormaps.find(colormap) == data_->colormaps.end())
		{
			Handle<Texture> colormap_tex = rhi.create_texture({TextureFormat::kLuminance, 256, 1});
			data_->colormaps.insert({colormap, colormap_tex});
		}

		data_->colormaps_to_upload.push_back(colormap);
	}

	// Stage 2 - pack rects into atlases
	std::vector<const patch_t*> patches_to_pack(patch_cache_misses.begin(), patch_cache_misses.end());
	pack_patches(rhi, *data_, patches_to_pack);
	// We now know what patches need to be uploaded.

	size_t list_index = 0;
	for (auto& list : *ctx_)
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
			// TODO actually implement flat drawing
			auto tex_visitor = srb2::Overload {
				[&](const Draw2dPatchQuad& cmd)
				{
					if (cmd.patch == nullptr)
					{
						new_cmd_needed = new_cmd_needed || (merged_cmd.texture != std::nullopt);
					}
					else
					{
						size_t atlas_index = data_->patch_lookup[cmd.patch];
						typeof(merged_cmd.texture) atlas_index_texture = atlas_index;
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
						typeof(merged_cmd.texture) flat_tex = MergedTwodeeCommandFlatTexture {cmd.flat_lump};
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
							the_new_one.texture = data_->patch_lookup[cmd.patch];
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
							typeof(the_new_one.texture) t = MergedTwodeeCommandFlatTexture {cmd.flat_lump};
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
					[&](const Draw2dPatchQuad& cmd) { rewrite_patch_quad_vertices(list, cmd, data_.get()); },
					[&](const Draw2dVertices& cmd) {}};
				std::visit(vtx_transform_visitor, cmd);
			}
		}

		cmd_lists_.push_back(std::move(merged_list));

		list_index++;
	}
}

void TwodeePass::transfer(Rhi& rhi, Handle<TransferContext> ctx)
{
	if (!ctx_ || !data_)
	{
		return;
	}

	if (data_->upload_default_tex)
	{
		std::array<uint8_t, 4> data = {0, 255, 0, 255};
		rhi.update_texture(ctx, data_->default_tex, {0, 0, 2, 1}, PixelFormat::kRG8, tcb::as_bytes(tcb::span(data)));

		std::array<uint8_t, 256> colormap_data;
		for (size_t i = 0; i < 256; i++)
		{
			colormap_data[i] = i;
		}
		rhi.update_texture(
			ctx,
			data_->default_colormap_tex,
			{0, 0, 256, 1},
			PixelFormat::kR8,
			tcb::as_bytes(tcb::span(colormap_data))
		);

		data_->upload_default_tex = false;
	}

	for (auto colormap : data_->colormaps_to_upload)
	{
		rhi.update_texture(
			ctx,
			data_->colormaps[colormap],
			{0, 0, 256, 1},
			rhi::PixelFormat::kR8,
			tcb::as_bytes(tcb::span(colormap, 256))
		);
	}
	data_->colormaps_to_upload.clear();

	// Convert patches to RG8 textures and upload to atlas pages
	std::vector<uint8_t> patch_data;
	for (const patch_t* patch_to_upload : data_->patches_to_upload)
	{
		Atlas& atlas = data_->patch_atlases[data_->patch_lookup[patch_to_upload]];
		AtlasEntry& entry = atlas.entries[patch_to_upload];

		convert_patch_to_trimmed_rg8_pixels(patch_to_upload, patch_data);

		rhi.update_texture(
			ctx,
			atlas.tex,
			{static_cast<int32_t>(entry.x), static_cast<int32_t>(entry.y), entry.w, entry.h},
			PixelFormat::kRG8,
			tcb::as_bytes(tcb::span(patch_data))
		);
	}
	data_->patches_to_upload.clear();

	Handle<Texture> palette_tex = palette_manager_->palette();

	// Update the buffers for each list
	auto ctx_list_itr = ctx_->begin();
	for (size_t i = 0; i < cmd_lists_.size() && ctx_list_itr != ctx_->end(); i++)
	{
		auto& merged_list = cmd_lists_[i];
		auto& orig_list = *ctx_list_itr;

		tcb::span<const std::byte> vertex_data = tcb::as_bytes(tcb::span(orig_list.vertices));
		tcb::span<const std::byte> index_data = tcb::as_bytes(tcb::span(orig_list.indices));
		rhi.update_buffer_contents(ctx, merged_list.vbo, 0, vertex_data);
		rhi.update_buffer_contents(ctx, merged_list.ibo, 0, index_data);

		// Update the binding sets for each individual merged command
		VertexAttributeBufferBinding vbos[] = {{0, merged_list.vbo}};
		for (auto& mcmd : merged_list.cmds)
		{
			TextureBinding tx[3];
			auto tex_visitor = srb2::Overload {
				[&](size_t atlas_index)
				{
					Atlas& atlas = data_->patch_atlases[atlas_index];
					tx[0] = {SamplerName::kSampler0, atlas.tex};
					tx[1] = {SamplerName::kSampler1, palette_tex};
				},
				[&](const MergedTwodeeCommandFlatTexture& tex)
				{
					Handle<Texture> th = flat_manager_->find_indexed(tex.lump);
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
				tx[0] = {SamplerName::kSampler0, data_->default_tex};
				tx[1] = {SamplerName::kSampler1, palette_tex};
			}

			const uint8_t* colormap = mcmd.colormap;
			Handle<Texture> colormap_h = data_->default_colormap_tex;
			if (colormap)
			{
				SRB2_ASSERT(data_->colormaps.find(colormap) != data_->colormaps.end());
				colormap_h = data_->colormaps[colormap];
			}
			tx[2] = {SamplerName::kSampler2, colormap_h};
			mcmd.binding_set =
				rhi.create_binding_set(ctx, data_->pipelines[mcmd.pipeline_key], {tcb::span(vbos), tcb::span(tx)});
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
	us_1 = rhi.create_uniform_set(ctx, {tcb::span(g1_uniforms)});
	us_2 = rhi.create_uniform_set(ctx, {tcb::span(g2_uniforms)});
}

static constexpr const glm::vec4 kClearColor = {0, 0, 0, 1};

void TwodeePass::graphics(Rhi& rhi, Handle<GraphicsContext> ctx)
{
	if (!ctx_ || !data_)
	{
		return;
	}

	if (output_)
	{
		rhi.begin_render_pass(ctx, {render_pass_, output_, std::nullopt, kClearColor});
	}
	else
	{
		rhi.begin_default_render_pass(ctx, false);
	}

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
			SRB2_ASSERT(data_->pipelines.find(cmd.pipeline_key) != data_->pipelines.end());
			Handle<Pipeline> pl = data_->pipelines[cmd.pipeline_key];
			rhi.bind_pipeline(ctx, pl);
			if (output_)
			{
				rhi.set_viewport(ctx, {0, 0, output_width_, output_height_});
			}
			rhi.bind_uniform_set(ctx, 0, us_1);
			rhi.bind_uniform_set(ctx, 1, us_2);
			rhi.bind_binding_set(ctx, cmd.binding_set);
			rhi.bind_index_buffer(ctx, list.ibo);
			rhi.draw_indexed(ctx, cmd.elements, cmd.index_offset);
		}
	}
	rhi.end_render_pass(ctx);
}

void TwodeePass::postpass(Rhi& rhi)
{
	if (!ctx_ || !data_)
	{
		return;
	}

	cmd_lists_.clear();
}
