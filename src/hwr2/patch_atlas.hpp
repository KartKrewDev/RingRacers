// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PATCH_ATLAS_HPP__
#define __SRB2_HWR2_PATCH_ATLAS_HPP__

#include <cstdint>
#include <memory>
#include <optional>

#include <tcb/span.hpp>

#include "../core/hash_map.hpp"
#include "../core/hash_set.hpp"
#include "../core/vector.hpp"
#include "../r_defs.h"
#include "../rhi/rhi.hpp"

extern "C"
{
// Forward declare the stb_rect_pack types since they are only pointed to

struct stbrp_context;
struct stbrp_node;
struct stbrp_rect;
};

namespace srb2::hwr2
{

class PatchAtlas
{
public:
	struct Entry
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

private:
	rhi::Handle<rhi::Texture> tex_;
	uint32_t size_;

	srb2::HashMap<const patch_t*, Entry> entries_;

	std::unique_ptr<stbrp_context> rp_ctx {nullptr};
	std::unique_ptr<stbrp_node[]> rp_nodes {nullptr};

	friend class PatchAtlasCache;

public:
	PatchAtlas(rhi::Handle<rhi::Texture> tex, uint32_t size);
	PatchAtlas(const PatchAtlas&) = delete;
	PatchAtlas& operator=(const PatchAtlas&) = delete;
	PatchAtlas(PatchAtlas&&);
	PatchAtlas& operator=(PatchAtlas&&);

	/// @brief Get the Luminance-Alpha RHI texture handle for this atlas texture
	rhi::Handle<rhi::Texture> texture() const noexcept { return tex_; }

	uint32_t texture_size() const noexcept { return size_; }

	std::optional<Entry> find_patch(srb2::NotNull<const patch_t*> patch) const;

	void pack_rects(tcb::span<stbrp_rect> rects);
};

/// @brief A resource-managing pass which creates and manages a set of Atlas Textures with
/// optimally packed Patches, allowing drawing passes to reuse the same texture binds for
/// drawing things like sprites and 2D elements.
class PatchAtlasCache
{
	srb2::Vector<PatchAtlas> atlases_;
	srb2::HashMap<const patch_t*, size_t> patch_lookup_;

	srb2::HashSet<const patch_t*> patches_to_pack_;
	srb2::HashSet<const patch_t*> patches_to_upload_;

	uint32_t tex_size_ = 2048;
	size_t max_textures_ = 2;

	bool ready_for_lookup() const;

	/// @brief Decide if a rect's dimensions are Large, that is, the rect should not be packed and instead its patch
	/// should be uploaded in isolation.
	bool rect_is_large(uint32_t w, uint32_t h) const noexcept { return false; }

public:
	PatchAtlasCache(uint32_t tex_size, size_t max_textures);

	PatchAtlasCache(const PatchAtlasCache&) = delete;
	PatchAtlasCache(PatchAtlasCache&&);
	PatchAtlasCache& operator=(const PatchAtlasCache&) = delete;
	PatchAtlasCache& operator=(PatchAtlasCache&&);
	~PatchAtlasCache();

	/// @brief Queue a patch to be packed. All patches will be packed after the prepass phase,
	/// or the owner can explicitly request a pack.
	void queue_patch(srb2::NotNull<const patch_t*> patch);

	/// @brief Pack queued patches, allowing them to be looked up with find_patch.
	void pack(rhi::Rhi& rhi);

	/// @brief Find the atlas a patch belongs to, or nullopt if it is not cached.
	/// This may not be called if there are still patches that need to be packed.
	/// The return value of this function may change between invocations of prepass for any given input.
	const PatchAtlas* find_patch(srb2::NotNull<const patch_t*> patch) const;
	PatchAtlas* find_patch(srb2::NotNull<const patch_t*> patch);

	bool need_to_reset() const;

	/// @brief Clear the atlases and reset for lookup.
	void reset(rhi::Rhi& rhi);
};

/// @brief Calculate the subregion of the patch which excludes empty space on the borders.
rhi::Rect trimmed_patch_dimensions(const patch_t* patch);

/// @brief Convert a patch to RG8 pixel data. If the patch's trimmed width is not a multiple of 2,
/// an additional blank column will be emitted to the output; this pixel data is ignored by RHI
/// during upload, but required for the RHI device's Unpack Alignment of 4 bytes.
/// @param patch the patch to convert
/// @param out the output vector, cleared before writing.
void convert_patch_to_trimmed_rg8_pixels(const patch_t* patch, srb2::Vector<uint8_t>& out);

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PATCH_ATLAS_HPP__
