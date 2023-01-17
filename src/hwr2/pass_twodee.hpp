// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_PASS_TWODEE_HPP__
#define __SRB2_HWR2_PASS_TWODEE_HPP__

#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../cxxutil.hpp"
#include "pass.hpp"
#include "pass_resource_managers.hpp"
#include "twodee.hpp"

namespace srb2::hwr2
{

class TwodeePass;

/// @brief Shared structures to allow multiple 2D instances to share the same atlases
struct TwodeePassData;

/// @brief Hash map key for caching pipelines
struct TwodeePipelineKey
{
	Draw2dBlend blend;
	bool lines;

	bool operator==(const TwodeePipelineKey& r) const noexcept { return !(blend != r.blend || lines != r.lines); }
	bool operator!=(const TwodeePipelineKey& r) const noexcept { return !(*this == r); }
};

struct MergedTwodeeCommandFlatTexture
{
	lumpnum_t lump;

	bool operator==(const MergedTwodeeCommandFlatTexture& rhs) const noexcept { return lump == rhs.lump; }
	bool operator!=(const MergedTwodeeCommandFlatTexture& rhs) const noexcept { return !(*this == rhs); }
};

struct MergedTwodeeCommand
{
	TwodeePipelineKey pipeline_key = {};
	rhi::Handle<rhi::BindingSet> binding_set = {};
	std::optional<std::variant<size_t, MergedTwodeeCommandFlatTexture>> texture;
	const uint8_t* colormap;
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

std::shared_ptr<TwodeePassData> make_twodee_pass_data();

struct TwodeePass final : public Pass
{
	Twodee* ctx_ = nullptr;
	std::variant<rhi::Handle<rhi::Texture>, rhi::Handle<rhi::Renderbuffer>> out_color_;

	std::shared_ptr<TwodeePassData> data_;
	std::shared_ptr<FlatTextureManager> flat_manager_;
	rhi::Handle<rhi::UniformSet> us_1;
	rhi::Handle<rhi::UniformSet> us_2;
	std::vector<MergedTwodeeCommandList> cmd_lists_;
	std::vector<std::tuple<rhi::Handle<rhi::Buffer>, std::size_t>> vbos_;
	std::vector<std::tuple<rhi::Handle<rhi::Buffer>, std::size_t>> ibos_;
	bool rebuild_atlases_ = false;
	rhi::Handle<rhi::RenderPass> render_pass_;
	rhi::Handle<rhi::Texture> output_;
	uint32_t output_width_ = 0;
	uint32_t output_height_ = 0;

	TwodeePass();
	virtual ~TwodeePass();

	virtual void prepass(rhi::Rhi& rhi) override;

	virtual void transfer(rhi::Rhi& rhi, rhi::Handle<rhi::TransferContext> ctx) override;

	virtual void graphics(rhi::Rhi& rhi, rhi::Handle<rhi::GraphicsContext> ctx) override;

	virtual void postpass(rhi::Rhi& rhi) override;
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

#endif // __SRB2_HWR2_PASS_TWODEE_HPP__
