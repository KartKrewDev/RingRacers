// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "resource_management.hpp"

#include "../r_state.h"
#include "../v_video.h"
#include "../z_zone.h"

using namespace srb2;
using namespace rhi;
using namespace hwr2;

PaletteManager::PaletteManager() = default;
PaletteManager::PaletteManager(PaletteManager&&) = default;
PaletteManager::~PaletteManager() = default;
PaletteManager& PaletteManager::operator=(PaletteManager&&) = default;

constexpr std::size_t kPaletteSize = 256;
constexpr std::size_t kLighttableRows = LIGHTLEVELS;

void PaletteManager::update(Rhi& rhi)
{
	if (!palette_)
	{
		palette_ = rhi.create_texture({TextureFormat::kRGBA, kPaletteSize, 1, TextureWrapMode::kClamp, TextureWrapMode::kClamp});
	}

#if 0
	if (!lighttable_)
	{
		lighttable_ = rhi.create_texture({TextureFormat::kLuminance, kPaletteSize, kLighttableRows, TextureWrapMode::kClamp, TextureWrapMode::kClamp});
	}

	if (!encore_lighttable_)
	{
		encore_lighttable_ = rhi.create_texture({TextureFormat::kLuminance, kPaletteSize, kLighttableRows, TextureWrapMode::kClamp, TextureWrapMode::kClamp});
	}
#endif

	if (!default_colormap_)
	{
		default_colormap_ = rhi.create_texture({TextureFormat::kLuminance, kPaletteSize, 1, TextureWrapMode::kClamp, TextureWrapMode::kClamp});
	}

	// Palette
	{
		std::array<byteColor_t, kPaletteSize> palette_32;
		for (std::size_t i = 0; i < kPaletteSize; i++)
		{
			palette_32[i] = V_GetColor(i).s;
		}
		rhi.update_texture(palette_, {0, 0, kPaletteSize, 1}, PixelFormat::kRGBA8, tcb::as_bytes(tcb::span(palette_32)));
	}

#if 0
	// Lighttables
	{
		if (colormaps != nullptr)
		{
			tcb::span<const std::byte> colormap_bytes = tcb::as_bytes(tcb::span(colormaps, kPaletteSize * kLighttableRows));
			rhi.update_texture(lighttable_, {0, 0, kPaletteSize, kLighttableRows}, PixelFormat::kR8, colormap_bytes);
		}

		// FIXME: This is broken, encoremap should not be used directly.
		// Instead, use colormaps + COLORMAP_REMAPOFFSET. See R_ReInitColormaps.
		if (encoremap != nullptr)
		{
			tcb::span<const std::byte> encoremap_bytes = tcb::as_bytes(tcb::span(encoremap, kPaletteSize * kLighttableRows));
			rhi.update_texture(encore_lighttable_, {0, 0, kPaletteSize, kLighttableRows}, PixelFormat::kR8, encoremap_bytes);
		}
	}
#endif

	// Default colormap
	{
		std::array<uint8_t, kPaletteSize> data;
		for (std::size_t i = 0; i < kPaletteSize; i++)
		{
			data[i] = i;
		}
		rhi.update_texture(default_colormap_, {0, 0, kPaletteSize, 1}, PixelFormat::kR8, tcb::as_bytes(tcb::span(data)));
	}
}

void PaletteManager::destroy_per_frame_resources(Rhi& rhi)
{
	for (auto colormap_tex : colormaps_)
	{
		rhi.destroy_texture(colormap_tex.second);
	}

	colormaps_.clear();

	for (auto lighttable_tex : lighttables_)
	{
		rhi.destroy_texture(lighttable_tex.second);
	}

	lighttables_.clear();
}

Handle<Texture> PaletteManager::find_or_create_colormap(Rhi& rhi, srb2::NotNull<const uint8_t*> colormap)
{
	if (colormaps_.find(colormap) != colormaps_.end())
	{
		return colormaps_[colormap];
	}

	Handle<Texture> texture = rhi.create_texture({TextureFormat::kLuminance, kPaletteSize, 1, TextureWrapMode::kClamp, TextureWrapMode::kClamp});

	tcb::span<const std::byte> map_bytes = tcb::as_bytes(tcb::span(colormap.get(), kPaletteSize));
	rhi.update_texture(texture, {0, 0, kPaletteSize, 1}, PixelFormat::kR8, map_bytes);

	colormaps_.insert_or_assign(colormap, texture);
	return texture;
}

Handle<Texture> PaletteManager::find_or_create_extra_lighttable(Rhi& rhi, srb2::NotNull<const uint8_t*> lighttable)
{
	if (lighttables_.find(lighttable) != lighttables_.end())
	{
		return lighttables_[lighttable];
	}

	Handle<Texture> texture = rhi.create_texture({TextureFormat::kLuminance, kPaletteSize, kLighttableRows, TextureWrapMode::kClamp, TextureWrapMode::kClamp});

	tcb::span<const std::byte> lighttable_bytes = tcb::as_bytes(tcb::span(lighttable.get(), kPaletteSize * kLighttableRows));
	rhi.update_texture(texture, {0, 0, kPaletteSize, kLighttableRows}, PixelFormat::kR8, lighttable_bytes);
	lighttables_.insert_or_assign(lighttable, texture);

	return texture;
}

FlatTextureManager::FlatTextureManager() = default;
FlatTextureManager::~FlatTextureManager() = default;

static uint32_t get_flat_size(lumpnum_t lump)
{
	SRB2_ASSERT(lump != LUMPERROR);

	std::size_t lumplength = W_LumpLength(lump);
	if (lumplength == 0)
	{
		return 0;
	}

	if ((lumplength & (lumplength - 1)) != 0)
	{
		// Lump length is not a power of two and therefore not a flat.
		return 0;
	}
	uint32_t lumpsize = std::pow(2, std::log2(lumplength) / 2);
	return lumpsize;
}

Handle<Texture> FlatTextureManager::find_or_create_indexed(Rhi& rhi, lumpnum_t lump)
{
	SRB2_ASSERT(lump != LUMPERROR);

	auto flat_itr = flats_.find(lump);
	if (flat_itr != flats_.end())
	{
		return flat_itr->second;
	}

	uint32_t flat_size = get_flat_size(lump);
	Handle<Texture> new_tex = rhi.create_texture({
		TextureFormat::kLuminanceAlpha,
		flat_size,
		flat_size,
		TextureWrapMode::kRepeat,
		TextureWrapMode::kRepeat
	});
	flats_.insert({lump, new_tex});

	srb2::Vector<std::array<uint8_t, 2>> flat_data;
	std::size_t lump_length = W_LumpLength(lump);
	flat_data.reserve(flat_size * flat_size);

	const uint8_t* flat_memory = static_cast<const uint8_t*>(W_CacheLumpNum(lump, PU_PATCH));
	SRB2_ASSERT(flat_memory != nullptr);

	tcb::span<const uint8_t> flat_bytes = tcb::span(flat_memory, lump_length);
	for (const uint8_t index : flat_bytes)
	{
		// The alpha/green channel is set to 0 if it's index 247; this is not usually used but fake floors can be
		// masked sometimes, so we need to treat it as transparent when rendering them.
		// See https://zdoom.org/wiki/Palette for remarks on fake 247 transparency
		flat_data.push_back({index, index == 247 ? static_cast<uint8_t>(0) : static_cast<uint8_t>(255)});
	}

	// A flat size of 1 would end up being 2 bytes, so we need 2 more bytes to be unpack-aligned on texture upload
	// Any other size would implicitly be aligned.
	// Sure hope nobody tries to load any flats that are too big for the gpu!
	if (flat_size == 1)
	{
		flat_data.push_back({0, 0});
	}

	tcb::span<const std::byte> data_bytes = tcb::as_bytes(tcb::span(flat_data));
	rhi.update_texture(new_tex, {0, 0, flat_size, flat_size}, rhi::PixelFormat::kRG8, data_bytes);

	return new_tex;
}
