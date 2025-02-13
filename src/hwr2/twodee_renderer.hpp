// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_TWODEE_HPP__
#define __SRB2_HWR2_PASS_TWODEE_HPP__

#include <optional>
#include <tuple>
#include <variant>
#include <vector>

#include "../cxxutil.hpp"
#include "patch_atlas.hpp"
#include "resource_management.hpp"
#include "twodee.hpp"

namespace srb2::hwr2
{

class TwodeeRenderer;

/// @brief Hash map key for caching pipelines
struct TwodeePipelineKey
{
	BlendMode blend;
	bool lines;

	bool operator==(const TwodeePipelineKey& r) const noexcept { return !(blend != r.blend || lines != r.lines); }
	bool operator!=(const TwodeePipelineKey& r) const noexcept { return !(*this == r); }
};

} // namespace srb2::hwr2

template <>
struct std::hash<srb2::hwr2::TwodeePipelineKey>
{
	std::size_t operator()(const srb2::hwr2::TwodeePipelineKey& v) const
	{
		std::size_t hash = 0;
		srb2::hash_combine(hash, v.blend, v.lines);
		return hash;
	}
};

namespace srb2::hwr2
{

struct MergedTwodeeCommandFlatTexture
{
	lumpnum_t lump;

	bool operator==(const MergedTwodeeCommandFlatTexture& rhs) const noexcept { return lump == rhs.lump; }
	bool operator!=(const MergedTwodeeCommandFlatTexture& rhs) const noexcept { return !(*this == rhs); }
};

struct MergedTwodeeCommand
{
	using Texture = std::variant<rhi::Handle<rhi::Texture>, MergedTwodeeCommandFlatTexture>;
	rhi::PrimitiveType primitive;
	BlendMode blend_mode;
	std::optional<Texture> texture;
	TwodeePipelineKey pipeline_key;
	rhi::Handle<rhi::Texture> texture_handle;
	const uint8_t* colormap;
	rhi::Handle<rhi::Texture> colormap_handle;
	uint32_t index_offset = 0;
	uint32_t elements = 0;
};

struct MergedTwodeeCommandList
{
	rhi::Handle<rhi::Buffer> vbo {};
	uint32_t vbo_size = 0;
	rhi::Handle<rhi::Buffer> ibo {};
	uint32_t ibo_size = 0;

	std::vector<MergedTwodeeCommand> cmds;
};

class TwodeeRenderer final
{
	bool initialized_ = false;
	std::variant<rhi::Handle<rhi::Texture>, rhi::Handle<rhi::Renderbuffer>> out_color_;

	PaletteManager* palette_manager_;
	FlatTextureManager* flat_manager_;
	PatchAtlasCache* patch_atlas_cache_;
	std::vector<MergedTwodeeCommandList> cmd_lists_;
	std::vector<std::tuple<rhi::Handle<rhi::Buffer>, std::size_t>> vbos_;
	std::vector<std::tuple<rhi::Handle<rhi::Buffer>, std::size_t>> ibos_;
	rhi::Handle<rhi::Texture> output_;
	rhi::Handle<rhi::Texture> default_tex_;
	rhi::Handle<rhi::Program> program_;

	void rewrite_patch_quad_vertices(Draw2dList& list, const Draw2dPatchQuad& cmd) const;

	void initialize(rhi::Rhi& rhi);

public:
	TwodeeRenderer(
		srb2::NotNull<PaletteManager*> palette_manager,
		srb2::NotNull<FlatTextureManager*> flat_manager,
		srb2::NotNull<PatchAtlasCache*> patch_atlas_cache
	);
	TwodeeRenderer(const TwodeeRenderer&) = delete;
	TwodeeRenderer(TwodeeRenderer&&);
	~TwodeeRenderer();
	TwodeeRenderer& operator=(const TwodeeRenderer&) = delete;
	TwodeeRenderer& operator=(TwodeeRenderer&&);

	/// @brief Flush accumulated Twodee state and perform draws.
	/// @param rhi
	/// @param ctx
	void flush(rhi::Rhi& rhi, Twodee& twodee);
};

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_PASS_TWODEE_HPP__
