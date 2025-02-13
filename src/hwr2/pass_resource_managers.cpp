// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "pass_resource_managers.hpp"

#include <algorithm>
#include <cmath>

#include "../r_state.h"
#include "../v_video.h"
#include "../z_zone.h"

using namespace srb2;
using namespace srb2::hwr2;
using namespace srb2::rhi;

FramebufferManager::FramebufferManager()
{
}

FramebufferManager::~FramebufferManager() = default;

void FramebufferManager::prepass(Rhi& rhi)
{
	uint32_t current_width = vid.width;
	uint32_t current_height = vid.height;

	// Destroy the framebuffer textures if they exist and the video size changed
	if (width_ != current_width || height_ != current_height)
	{
		if (main_color_ != kNullHandle)
		{
			rhi.destroy_texture(main_color_);
			main_color_ = kNullHandle;
		}

		if (post_colors_[0] != kNullHandle)
		{
			rhi.destroy_texture(post_colors_[0]);
			post_colors_[0] = kNullHandle;
		}
		if (post_colors_[1] != kNullHandle)
		{
			rhi.destroy_texture(post_colors_[1]);
			post_colors_[1] = kNullHandle;
		}
		if (wipe_start_color_ != kNullHandle)
		{
			rhi.destroy_texture(wipe_start_color_);
			wipe_start_color_ = kNullHandle;
		}
		if (wipe_end_color_ != kNullHandle)
		{
			rhi.destroy_texture(wipe_end_color_);
			wipe_end_color_ = kNullHandle;
		}
	}
	width_ = current_width;
	height_ = current_height;

	// Recreate the framebuffer textures
	if (main_color_ == kNullHandle)
	{
		main_color_ = rhi.create_texture({
			TextureFormat::kRGBA,
			current_width,
			current_height,
			TextureWrapMode::kClamp,
			TextureWrapMode::kClamp
		});
	}

	if (post_colors_[0] == kNullHandle)
	{
		post_colors_[0] = rhi.create_texture({
			TextureFormat::kRGBA,
			current_width,
			current_height,
			TextureWrapMode::kClamp,
			TextureWrapMode::kClamp
		});
	}
	if (post_colors_[1] == kNullHandle)
	{
		post_colors_[1] = rhi.create_texture({
			TextureFormat::kRGBA,
			current_width,
			current_height,
			TextureWrapMode::kClamp,
			TextureWrapMode::kClamp
		});
	}

	if (wipe_start_color_ == kNullHandle)
	{
		wipe_start_color_ = rhi.create_texture({
			TextureFormat::kRGBA,
			current_width,
			current_height,
			TextureWrapMode::kClamp,
			TextureWrapMode::kClamp
		});
	}
	if (wipe_end_color_ == kNullHandle)
	{
		wipe_end_color_ = rhi.create_texture({
			TextureFormat::kRGBA,
			current_width,
			current_height,
			TextureWrapMode::kClamp,
			TextureWrapMode::kClamp
		});
	}
}

void FramebufferManager::transfer(Rhi& rhi)
{
}

void FramebufferManager::graphics(Rhi& rhi)
{
}

void FramebufferManager::postpass(Rhi& rhi)
{
}

MainPaletteManager::MainPaletteManager() = default;
MainPaletteManager::~MainPaletteManager() = default;

constexpr std::size_t kPaletteSize = 256;
constexpr std::size_t kLighttableRows = LIGHTLEVELS;

void MainPaletteManager::prepass(Rhi& rhi)
{
	if (!palette_)
	{
		palette_ = rhi.create_texture({TextureFormat::kRGBA, kPaletteSize, 1, TextureWrapMode::kClamp, TextureWrapMode::kClamp});
	}

	if (!lighttable_)
	{
		lighttable_ = rhi.create_texture({TextureFormat::kLuminance, kPaletteSize, kLighttableRows, TextureWrapMode::kClamp, TextureWrapMode::kClamp});
	}

	if (!encore_lighttable_)
	{
		encore_lighttable_ = rhi.create_texture({TextureFormat::kLuminance, kPaletteSize, kLighttableRows, TextureWrapMode::kClamp, TextureWrapMode::kClamp});
	}

	if (!default_colormap_)
	{
		default_colormap_ = rhi.create_texture({TextureFormat::kLuminance, kPaletteSize, 1, TextureWrapMode::kClamp, TextureWrapMode::kClamp});
	}
}

void MainPaletteManager::upload_palette(Rhi& rhi)
{
	std::array<byteColor_t, kPaletteSize> palette_32;
	for (std::size_t i = 0; i < kPaletteSize; i++)
	{
		palette_32[i] = V_GetColor(i).s;
	}
	rhi.update_texture(palette_, {0, 0, kPaletteSize, 1}, PixelFormat::kRGBA8, tcb::as_bytes(tcb::span(palette_32)));
}

void MainPaletteManager::upload_lighttables(Rhi& rhi)
{
	if (colormaps != nullptr)
	{
		tcb::span<const std::byte> colormap_bytes = tcb::as_bytes(tcb::span(colormaps, kPaletteSize * kLighttableRows));
		rhi.update_texture(lighttable_, {0, 0, kPaletteSize, kLighttableRows}, PixelFormat::kR8, colormap_bytes);
	}

	if (encoremap != nullptr)
	{
		tcb::span<const std::byte> encoremap_bytes = tcb::as_bytes(tcb::span(encoremap, kPaletteSize * kLighttableRows));
		rhi.update_texture(encore_lighttable_, {0, 0, kPaletteSize, kLighttableRows}, PixelFormat::kR8, encoremap_bytes);
	}

	if (!lighttables_to_upload_.empty())
	{
		for (auto lighttable : lighttables_to_upload_)
		{
			Handle<Texture> lighttable_tex = find_extra_lighttable(lighttable);
			SRB2_ASSERT(lighttable_tex != kNullHandle);
			tcb::span<const std::byte> lighttable_bytes = tcb::as_bytes(tcb::span(lighttable, kPaletteSize * kLighttableRows));
			rhi.update_texture(lighttable_tex, {0, 0, kPaletteSize, kLighttableRows}, PixelFormat::kR8, lighttable_bytes);
		}
		lighttables_to_upload_.clear();
	}
}

void MainPaletteManager::upload_default_colormap(Rhi& rhi)
{
	std::array<uint8_t, kPaletteSize> data;
	for (std::size_t i = 0; i < kPaletteSize; i++)
	{
		data[i] = i;
	}
	rhi.update_texture(default_colormap_, {0, 0, kPaletteSize, 1}, PixelFormat::kR8, tcb::as_bytes(tcb::span(data)));
}

void MainPaletteManager::upload_colormaps(Rhi& rhi)
{
	for (auto to_upload : colormaps_to_upload_)
	{
		SRB2_ASSERT(to_upload != nullptr);
		SRB2_ASSERT(colormaps_.find(to_upload) != colormaps_.end());

		rhi::Handle<rhi::Texture> map_texture = colormaps_.at(to_upload);

		tcb::span<const std::byte> map_bytes = tcb::as_bytes(tcb::span(to_upload, kPaletteSize));
		rhi.update_texture(map_texture, {0, 0, kPaletteSize, 1}, PixelFormat::kR8, map_bytes);
	}
	colormaps_to_upload_.clear();
}

rhi::Handle<rhi::Texture> MainPaletteManager::find_or_create_colormap(rhi::Rhi& rhi, srb2::NotNull<const uint8_t*> colormap)
{
	if (colormaps_.find(colormap) != colormaps_.end())
	{
		return colormaps_[colormap];
	}

	Handle<Texture> texture = rhi.create_texture({TextureFormat::kLuminance, kPaletteSize, 1, TextureWrapMode::kClamp, TextureWrapMode::kClamp});

	colormaps_.insert_or_assign(colormap, texture);
	colormaps_to_upload_.push_back(colormap);
	return texture;
}

rhi::Handle<rhi::Texture> MainPaletteManager::find_colormap(srb2::NotNull<const uint8_t*> colormap) const
{
	if (colormaps_.find(colormap) == colormaps_.end())
	{
		return kNullHandle;
	}

	return colormaps_.at(colormap);
}

rhi::Handle<rhi::Texture> MainPaletteManager::find_or_create_extra_lighttable(Rhi& rhi, srb2::NotNull<const uint8_t*> lighttable)
{
	if (lighttables_.find(lighttable) != lighttables_.end())
	{
		return lighttables_[lighttable];
	}

	Handle<Texture> texture = rhi.create_texture({TextureFormat::kLuminance, kPaletteSize, kLighttableRows, TextureWrapMode::kClamp, TextureWrapMode::kClamp});

	lighttables_.insert_or_assign(lighttable, texture);
	lighttables_to_upload_.push_back(lighttable);
	return texture;
}

rhi::Handle<rhi::Texture> MainPaletteManager::find_extra_lighttable(srb2::NotNull<const uint8_t*> lighttable) const
{
	if (lighttables_.find(lighttable) == lighttables_.end())
	{
		return kNullHandle;
	}

	return lighttables_.at(lighttable);
}

void MainPaletteManager::transfer(Rhi& rhi)
{
	upload_palette(rhi);
	upload_lighttables(rhi);
	upload_default_colormap(rhi);
	upload_colormaps(rhi);
}

void MainPaletteManager::graphics(Rhi& rhi)
{
}

void MainPaletteManager::postpass(Rhi& rhi)
{
	// Delete all colormap textures so they'll be recreated next frame
	// Unfortunately the 256x1 translation colormaps are sometimes freed at runtime
	for (auto& cm : colormaps_)
	{
		rhi.destroy_texture(cm.second);
	}
	colormaps_.clear();

	for (auto& lt : lighttables_)
	{
		rhi.destroy_texture(lt.second);
	}
	lighttables_.clear();
}

CommonResourcesManager::CommonResourcesManager() = default;
CommonResourcesManager::~CommonResourcesManager() = default;

void CommonResourcesManager::prepass(Rhi& rhi)
{
	if (!init_)
	{
		black_ = rhi.create_texture({TextureFormat::kRGBA, 1, 1, TextureWrapMode::kClamp, TextureWrapMode::kClamp});
		white_ = rhi.create_texture({TextureFormat::kRGBA, 1, 1, TextureWrapMode::kClamp, TextureWrapMode::kClamp});
		transparent_ = rhi.create_texture({
			TextureFormat::kRGBA,
			1,
			1,
			TextureWrapMode::kClamp,
			TextureWrapMode::kClamp
		});
	}
}

void CommonResourcesManager::transfer(Rhi& rhi)
{
	if (!init_)
	{
		uint8_t black[4] = {0, 0, 0, 255};
		tcb::span<const std::byte> black_bytes = tcb::as_bytes(tcb::span(black, 4));
		uint8_t white[4] = {255, 255, 255, 255};
		tcb::span<const std::byte> white_bytes = tcb::as_bytes(tcb::span(white, 4));
		uint8_t transparent[4] = {0, 0, 0, 0};
		tcb::span<const std::byte> transparent_bytes = tcb::as_bytes(tcb::span(transparent, 4));

		rhi.update_texture(black_, {0, 0, 1, 1}, PixelFormat::kRGBA8, black_bytes);
		rhi.update_texture(white_, {0, 0, 1, 1}, PixelFormat::kRGBA8, white_bytes);
		rhi.update_texture(transparent_, {0, 0, 1, 1}, PixelFormat::kRGBA8, transparent_bytes);
	}
}

void CommonResourcesManager::graphics(Rhi& rhi)
{
}

void CommonResourcesManager::postpass(Rhi& rhi)
{
	init_ = true;
}
