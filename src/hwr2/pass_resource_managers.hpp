// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
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
	rhi::Handle<rhi::Renderbuffer> main_depth_;
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
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx) override;
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
	rhi::Handle<rhi::Renderbuffer> main_depth() const noexcept { return main_depth_; }

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

public:
	MainPaletteManager();
	virtual ~MainPaletteManager();

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	rhi::Handle<rhi::Texture> palette() const noexcept { return palette_; }
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
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	rhi::Handle<rhi::Texture> black() const noexcept { return black_; }
	rhi::Handle<rhi::Texture> white() const noexcept { return white_; }
	rhi::Handle<rhi::Texture> transparent() const noexcept { return transparent_; }
};

/*
A note to the reader:

RHI/HWR2's architecture is intentionally decoupled in a data-oriented design fashion. Hash map lookups might technically
be slower than storing the RHI handle in a hypothetical Flat class object, but it frees us from worrying about the
validity of a given Handle when the RHI instance changes -- and it _can_, because this is designed to allow multiple
RHI backends -- because any given Pass must be disposed when the RHI changes. The implementation of I_FinishUpdate is
such that if the RHI is not the same as before, all passes must be reconstructed, and so we don't have to worry about
going around and resetting Handle references everywhere. If you're familiar with old GL, it's like decoupling GLmipmap_t
from patch_t.
*/

/// @brief Manages textures corresponding to specific flats indexed by lump number.
class FlatTextureManager final : public Pass
{
	std::unordered_map<lumpnum_t, rhi::Handle<rhi::Texture>> flats_;
	std::vector<lumpnum_t> to_upload_;
	std::vector<rhi::Handle<rhi::Texture>> disposed_textures_;

public:
	FlatTextureManager();
	virtual ~FlatTextureManager();

	virtual void prepass(rhi::Rhi& rhi) override;
	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx) override;
	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;
	virtual void postpass(rhi::Rhi& rhi) override;

	/// @brief Find the indexed texture for a given flat lump, or create one if it doesn't exist yet. Only call this
	/// in prepass.
	/// @param flat_lump
	/// @return
	rhi::Handle<rhi::Texture> find_or_create_indexed(rhi::Rhi& rhi, lumpnum_t flat_lump);

	rhi::Handle<rhi::Texture> find_indexed(lumpnum_t flat_lump) const;
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_RESOURCE_MANAGERS_HPP__
