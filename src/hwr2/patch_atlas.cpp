// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "patch_atlas.hpp"

#include <stb_rect_pack.h>

#include "../r_patch.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

rhi::Rect srb2::hwr2::trimmed_patch_dimensions(const patch_t* patch)
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

void srb2::hwr2::convert_patch_to_trimmed_rg8_pixels(const patch_t* patch, srb2::Vector<uint8_t>& out)
{
	Rect trimmed_rect = srb2::hwr2::trimmed_patch_dimensions(patch);
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

PatchAtlas::PatchAtlas(Handle<Texture> texture, uint32_t size) : tex_(texture), size_(size)
{
	rp_ctx = std::make_unique<stbrp_context>();
	rp_nodes = std::make_unique<stbrp_node[]>(size * 2);
	const size_t double_size = size * 2;
	for (size_t i = 0; i < double_size; i++)
	{
		rp_nodes[i] = {};
	}
	stbrp_init_target(rp_ctx.get(), size, size, rp_nodes.get(), double_size);
}

PatchAtlas::PatchAtlas(PatchAtlas&&) = default;
PatchAtlas& PatchAtlas::operator=(PatchAtlas&&) = default;

void PatchAtlas::pack_rects(tcb::span<stbrp_rect> rects)
{
	stbrp_pack_rects(rp_ctx.get(), rects.data(), rects.size());
}

std::optional<PatchAtlas::Entry> PatchAtlas::find_patch(srb2::NotNull<const patch_t*> patch) const
{
	auto itr = entries_.find(patch);
	if (itr == entries_.end())
	{
		return std::nullopt;
	}

	return itr->second;
}

PatchAtlasCache::PatchAtlasCache(uint32_t tex_size, size_t max_textures)
	: tex_size_(tex_size)
	, max_textures_(max_textures)
{
}

PatchAtlasCache::PatchAtlasCache(PatchAtlasCache&&) = default;
PatchAtlasCache& PatchAtlasCache::operator=(PatchAtlasCache&&) = default;
PatchAtlasCache::~PatchAtlasCache() = default;

bool PatchAtlasCache::need_to_reset() const
{
	if (atlases_.size() > max_textures_)
	{
		return true;
	}
	if (Patch_WasFreedThisFrame())
	{
		return true;
	}
	return false;
}

void PatchAtlasCache::reset(Rhi& rhi)
{
	for (auto& atlas : atlases_)
	{
		rhi.destroy_texture(atlas.texture());
	}

	atlases_.clear();
	patch_lookup_.clear();
}

bool PatchAtlasCache::ready_for_lookup() const
{
	if (!patches_to_pack_.empty())
	{
		return false;
	}

	return true;
}

static PatchAtlas create_atlas(Rhi& rhi, uint32_t size)
{
	Handle<Texture> texture = rhi.create_texture(
		{
			TextureFormat::kLuminanceAlpha,
			size,
			size,
			TextureWrapMode::kClamp,
			TextureWrapMode::kClamp
		}
	);

	PatchAtlas new_atlas(texture, size);

	return new_atlas;
}

void PatchAtlasCache::pack(Rhi& rhi)
{
	// Prepare stbrp rects for patches to be loaded.
	std::vector<stbrp_rect> rects;

	std::vector<const patch_t*> large_patches;

	std::vector<const patch_t*> patches;
	for (auto patch : patches_to_pack_)
	{
		patches.push_back(patch);
	}

	for (size_t i = 0; i < patches.size(); i++)
	{
		const patch_t* patch = patches[i];
		Rect trimmed_rect = trimmed_patch_dimensions(patch);

		if (rect_is_large(trimmed_rect.w, trimmed_rect.h))
		{
			large_patches.push_back(patch);
			continue;
		}

		stbrp_rect rect {};

		rect.id = i;
		rect.w = trimmed_rect.w;
		rect.h = trimmed_rect.h;
		rects.push_back(std::move(rect));
	}

	while (rects.size() > 0)
	{
		if (atlases_.size() == 0)
		{
			atlases_.push_back(create_atlas(rhi, tex_size_));
		}

		for (size_t atlas_index = 0; atlas_index < atlases_.size(); atlas_index++)
		{
			auto& atlas = atlases_[atlas_index];
			atlas.pack_rects(rects);
			for (auto itr = rects.begin(); itr != rects.end();)
			{
				auto& rect = *itr;
				if (rect.was_packed)
				{
					PatchAtlas::Entry entry;
					const patch_t* patch = patches[rect.id];
					Rect trimmed_rect = trimmed_patch_dimensions(patch);
					entry.x = static_cast<uint32_t>(rect.x);
					entry.y = static_cast<uint32_t>(rect.y);
					entry.w = static_cast<uint32_t>(rect.w);
					entry.h = static_cast<uint32_t>(rect.h);
					entry.trim_x = static_cast<uint32_t>(trimmed_rect.x);
					entry.trim_y = static_cast<uint32_t>(trimmed_rect.y);
					entry.orig_w = static_cast<uint32_t>(patch->width);
					entry.orig_h = static_cast<uint32_t>(patch->height);
					atlas.entries_.insert_or_assign(patch, std::move(entry));
					patch_lookup_.insert_or_assign(patch, atlas_index);
					patches_to_upload_.insert(patch);
					itr = rects.erase(itr);
					continue;
				}
				++itr;
			}

			// If we still have rects to pack, and we're at the last atlas, create another atlas.
			if (atlas_index == atlases_.size() - 1 && rects.size() > 0)
			{
				atlases_.push_back(create_atlas(rhi, tex_size_));
			}
		}
	}

	patches_to_pack_.clear();

	// TODO Create large patch "atlases"

	SRB2_ASSERT(ready_for_lookup());

	// Upload atlased patches
	srb2::Vector<uint8_t> patch_data;
	for (const patch_t* patch_to_upload : patches_to_upload_)
	{
		srb2::NotNull<PatchAtlas*> atlas = find_patch(patch_to_upload);

		std::optional<PatchAtlas::Entry> entry = atlas->find_patch(patch_to_upload);
		SRB2_ASSERT(entry.has_value());

		convert_patch_to_trimmed_rg8_pixels(patch_to_upload, patch_data);

		rhi.update_texture(
			atlas->tex_,
			{static_cast<int32_t>(entry->x), static_cast<int32_t>(entry->y), entry->w, entry->h},
			PixelFormat::kRG8,
			tcb::as_bytes(tcb::span(patch_data))
		);

		patch_data.clear();
	}
	patches_to_upload_.clear();
}

PatchAtlas* PatchAtlasCache::find_patch(srb2::NotNull<const patch_t*> patch)
{
	SRB2_ASSERT(ready_for_lookup());

	auto itr = patch_lookup_.find(patch);
	if (itr == patch_lookup_.end())
	{
		return nullptr;
	}

	size_t atlas_index = itr->second;

	SRB2_ASSERT(atlas_index < atlases_.size());

	return &atlases_[atlas_index];
}

const PatchAtlas* PatchAtlasCache::find_patch(srb2::NotNull<const patch_t*> patch) const
{
	SRB2_ASSERT(ready_for_lookup());

	auto itr = patch_lookup_.find(patch);
	if (itr == patch_lookup_.end())
	{
		return nullptr;
	}

	size_t atlas_index = itr->second;

	SRB2_ASSERT(atlas_index < atlases_.size());

	return &atlases_[atlas_index];
}

void PatchAtlasCache::queue_patch(srb2::NotNull<const patch_t*> patch)
{
	if (patch_lookup_.find(patch) != patch_lookup_.end())
	{
		return;
	}

	patches_to_pack_.insert(patch);
}
