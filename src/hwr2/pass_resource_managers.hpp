// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_RESOURCE_MANAGERS_HPP__
#define __SRB2_HWR2_PASS_RESOURCE_MANAGERS_HPP__

#include <array>
#include <cstddef>
#include <unordered_map>
#include <vector>

#include "pass.hpp"

namespace srb2::hwr2
{

class FramebufferManager final : public Pass
{
	rhi::Handle<rhi::Texture> main_color_;
	std::array<rhi::Handle<rhi::Texture>, 2> post_colors_;
	rhi::Handle<rhi::Texture> wipe_start_color_;
	rhi::Handle<rhi::Texture> wipe_end_color_;
	std::size_t post_index_ = 0;
	std::size_t width_ = 0;
	std::size_t height_ = 0;
	bool first_postprocess_ = true;

public:
	FramebufferManager();
	virtual ~FramebufferManager();

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	/// @brief Swap the current and previous postprocess FB textures. Use between pass prepass phases to alternate.
	void swap_post() noexcept
	{
		post_index_ = post_index_ == 0 ? 1 : 0;
		first_postprocess_ = false;
	}

	void reset_post() noexcept { first_postprocess_ = true; }

	rhi::Handle<rhi::Texture> main_color() const noexcept { return main_color_; }

	rhi::Handle<rhi::Texture> current_post_color() const noexcept { return post_colors_[post_index_]; }

	rhi::Handle<rhi::Texture> previous_post_color() const noexcept
	{
		if (first_postprocess_)
		{
			return main_color();
		}
		return post_colors_[1 - post_index_];
	};

	rhi::Handle<rhi::Texture> wipe_start_color() const noexcept { return wipe_start_color_; }
	rhi::Handle<rhi::Texture> wipe_end_color() const noexcept { return wipe_end_color_; }

	std::size_t width() const noexcept { return width_; }
	std::size_t height() const noexcept { return height_; }
};

class MainPaletteManager final : public Pass
{
	rhi::Handle<rhi::Texture> palette_;
	rhi::Handle<rhi::Texture> lighttable_;
	rhi::Handle<rhi::Texture> encore_lighttable_;
	rhi::Handle<rhi::Texture> default_colormap_;

	std::unordered_map<const uint8_t*, rhi::Handle<rhi::Texture>> colormaps_;
	std::unordered_map<const uint8_t*, rhi::Handle<rhi::Texture>> lighttables_;
	std::vector<const uint8_t*> colormaps_to_upload_;
	std::vector<const uint8_t*> lighttables_to_upload_;

	void upload_palette(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx);
	void upload_lighttables(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx);
	void upload_default_colormap(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx);
	void upload_colormaps(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx);

public:
	MainPaletteManager();
	virtual ~MainPaletteManager();

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	rhi::Handle<rhi::Texture> palette() const noexcept { return palette_; }
	rhi::Handle<rhi::Texture> lighttable() const noexcept { return lighttable_; }
	rhi::Handle<rhi::Texture> encore_lighttable() const noexcept { return encore_lighttable_; }
	rhi::Handle<rhi::Texture> default_colormap() const noexcept { return default_colormap_; }

	rhi::Handle<rhi::Texture> find_or_create_colormap(rhi::Rhi& rhi, srb2::NotNull<const uint8_t*> colormap);
	rhi::Handle<rhi::Texture> find_colormap(srb2::NotNull<const uint8_t*> colormap) const;
	rhi::Handle<rhi::Texture> find_or_create_extra_lighttable(rhi::Rhi& rhi, srb2::NotNull<const uint8_t*> lighttable);
	rhi::Handle<rhi::Texture> find_extra_lighttable(srb2::NotNull<const uint8_t*> lighttable) const;
};

class CommonResourcesManager final : public Pass
{
	bool init_ = false;
	rhi::Handle<rhi::Texture> black_;
	rhi::Handle<rhi::Texture> white_;
	rhi::Handle<rhi::Texture> transparent_;

public:
	CommonResourcesManager();
	virtual ~CommonResourcesManager();

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	rhi::Handle<rhi::Texture> black() const noexcept { return black_; }
	rhi::Handle<rhi::Texture> white() const noexcept { return white_; }
	rhi::Handle<rhi::Texture> transparent() const noexcept { return transparent_; }
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_RESOURCE_MANAGERS_HPP__
