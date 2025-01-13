// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_RESOURCE_MANAGEMENT_HPP__
#define __SRB2_HWR2_RESOURCE_MANAGEMENT_HPP__

#include "../core/hash_map.hpp"
#include "../core/vector.hpp"
#include "../rhi/rhi.hpp"

namespace srb2::hwr2
{

class PaletteManager
{
	rhi::Handle<rhi::Texture> palette_;
#if 0
	rhi::Handle<rhi::Texture> lighttable_;
	rhi::Handle<rhi::Texture> encore_lighttable_;
#endif
	rhi::Handle<rhi::Texture> default_colormap_;

	srb2::HashMap<const uint8_t*, rhi::Handle<rhi::Texture>> colormaps_;
	srb2::HashMap<const uint8_t*, rhi::Handle<rhi::Texture>> lighttables_;

public:
	PaletteManager();
	PaletteManager(const PaletteManager&) = delete;
	PaletteManager(PaletteManager&&);
	~PaletteManager();
	PaletteManager& operator=(const PaletteManager&) = delete;
	PaletteManager& operator=(PaletteManager&&);

	rhi::Handle<rhi::Texture> palette() const noexcept { return palette_; }
#if 0
	rhi::Handle<rhi::Texture> lighttable() const noexcept { return lighttable_; }
	rhi::Handle<rhi::Texture> encore_lighttable() const noexcept { return encore_lighttable_; }
#endif
	rhi::Handle<rhi::Texture> default_colormap() const noexcept { return default_colormap_; }

	void update(rhi::Rhi& rhi);
	void destroy_per_frame_resources(rhi::Rhi& rhi);

	rhi::Handle<rhi::Texture> find_or_create_colormap(rhi::Rhi& rhi, srb2::NotNull<const uint8_t*> colormap);
	rhi::Handle<rhi::Texture> find_or_create_extra_lighttable(rhi::Rhi& rhi, srb2::NotNull<const uint8_t*> lighttable);
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
class FlatTextureManager
{
	srb2::HashMap<lumpnum_t, rhi::Handle<rhi::Texture>> flats_;
	srb2::Vector<lumpnum_t> to_upload_;
	srb2::Vector<rhi::Handle<rhi::Texture>> disposed_textures_;

public:
	FlatTextureManager();
	~FlatTextureManager();

	/// @brief Find the indexed texture for a given flat lump, or create one if it doesn't exist yet. Only call this
	/// in prepass.
	/// @param flat_lump
	/// @return
	rhi::Handle<rhi::Texture> find_or_create_indexed(rhi::Rhi& rhi, lumpnum_t flat_lump);
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_RESOURCE_MANAGEMENT_HPP__
