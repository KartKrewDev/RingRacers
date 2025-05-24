// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_HWR2_TWODEE_HPP__
#define __SRB2_HWR2_TWODEE_HPP__

#include <array>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include <tcb/span.hpp>

#include "blendmode.hpp"
#include "../cxxutil.hpp"
#include "../doomtype.h"

namespace srb2::hwr2
{

struct TwodeeVertex
{
	float x;
	float y;
	float z;
	float u;
	float v;
	float r;
	float g;
	float b;
	float a;
};

struct Draw2dPatchQuad
{
	std::size_t begin_index = 0;
	std::size_t begin_element = 0;

	// A null patch ptr means no patch is drawn
	const patch_t* patch = nullptr;
	const uint8_t* colormap = nullptr;
	BlendMode blend;
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;
	float a = 0.f;

	// Size fields are made available to let the consumer modify the vertex data for optimization
	float xmin = 0.f;
	float ymin = 0.f;
	float xmax = 0.f;
	float ymax = 0.f;
	float clip_xmin = 0.f;
	float clip_xmax = 0.f;
	float clip_ymin = 0.f;
	float clip_ymax = 0.f;
	bool clip = false;
	bool flip = false;
	bool vflip = false;
};

struct Draw2dVertices
{
	std::size_t begin_index = 0;
	std::size_t begin_element = 0;
	std::size_t elements = 0;
	BlendMode blend = BlendMode::kAlphaTransparent;
	lumpnum_t flat_lump = UINT32_MAX; // LUMPERROR but not loading w_wad.h from this header
	bool lines = false;
};

using Draw2dCmd = std::variant<Draw2dPatchQuad, Draw2dVertices>;

BlendMode get_blend_mode(const Draw2dCmd& cmd) noexcept;
bool is_draw_lines(const Draw2dCmd& cmd) noexcept;
std::size_t elements(const Draw2dCmd& cmd) noexcept;

struct Draw2dList
{
	std::vector<TwodeeVertex> vertices;
	std::vector<uint16_t> indices;
	std::vector<Draw2dCmd> cmds;

	static constexpr const std::size_t kMaxVertices = 65536;
};

class Draw2dQuadBuilder;
class Draw2dVerticesBuilder;

/// @brief Buffered 2D drawing context
class Twodee
{
	std::vector<Draw2dList> lists_;
	std::vector<TwodeeVertex> current_verts_;
	std::vector<uint16_t> current_indices_;

	friend class Draw2dQuadBuilder;
	friend class Draw2dVerticesBuilder;

public:
	Twodee();
	Twodee(const Twodee&);
	Twodee(Twodee&&) noexcept;

	Twodee& operator=(const Twodee&);
	Twodee& operator=(Twodee&&) noexcept;

	Draw2dQuadBuilder begin_quad() noexcept;
	Draw2dVerticesBuilder begin_verts() noexcept;

	typename std::vector<Draw2dList>::iterator begin() noexcept { return lists_.begin(); }
	typename std::vector<Draw2dList>::iterator end() noexcept { return lists_.end(); }
	typename std::vector<Draw2dList>::const_iterator begin() const noexcept { return lists_.cbegin(); }
	typename std::vector<Draw2dList>::const_iterator end() const noexcept { return lists_.cend(); }
	typename std::vector<Draw2dList>::const_iterator cbegin() const noexcept { return lists_.cbegin(); }
	typename std::vector<Draw2dList>::const_iterator cend() const noexcept { return lists_.cend(); }
};

class Draw2dQuadBuilder
{
	Draw2dPatchQuad quad_;
	Twodee& ctx_;

	Draw2dQuadBuilder(Twodee& ctx) : quad_ {}, ctx_ {ctx} {}

	friend class Twodee;

public:
	Draw2dQuadBuilder(const Draw2dQuadBuilder&) = delete;
	Draw2dQuadBuilder(Draw2dQuadBuilder&&) = default;
	Draw2dQuadBuilder& operator=(const Draw2dQuadBuilder&) = delete;
	Draw2dQuadBuilder& operator=(Draw2dQuadBuilder&&) = delete;

	Draw2dQuadBuilder& rect(float x, float y, float w, float h)
	{
		quad_.xmin = x;
		quad_.xmax = x + w;
		quad_.ymin = y;
		quad_.ymax = y + h;
		return *this;
	}

	Draw2dQuadBuilder& flip(bool flip)
	{
		quad_.flip = flip;
		return *this;
	}

	Draw2dQuadBuilder& vflip(bool vflip)
	{
		quad_.vflip = vflip;
		return *this;
	}

	Draw2dQuadBuilder& clip(float xmin, float ymin, float xmax, float ymax)
	{
		quad_.clip_xmin = xmin;
		quad_.clip_ymin = ymin;
		quad_.clip_xmax = xmax;
		quad_.clip_ymax = ymax;
		quad_.clip = true;
		return *this;
	}

	Draw2dQuadBuilder& color(float r, float g, float b, float a)
	{
		quad_.r = r;
		quad_.g = g;
		quad_.b = b;
		quad_.a = a;
		return *this;
	}

	Draw2dQuadBuilder& patch(const patch_t* patch)
	{
		quad_.patch = patch;
		return *this;
	}

	Draw2dQuadBuilder& blend(BlendMode blend)
	{
		quad_.blend = blend;
		return *this;
	}

	Draw2dQuadBuilder& colormap(const uint8_t* colormap)
	{
		quad_.colormap = colormap;
		return *this;
	}

	void done();
};

class Draw2dVerticesBuilder
{
	Draw2dVertices tris_;
	Twodee& ctx_;
	std::vector<std::array<float, 4>> verts_;
	float r_ = 1.f;
	float g_ = 1.f;
	float b_ = 1.f;
	float a_ = 1.f;

	Draw2dVerticesBuilder(Twodee& ctx) : tris_ {}, ctx_ {ctx} {}

	friend class Twodee;

public:
	Draw2dVerticesBuilder(const Draw2dVerticesBuilder&) = delete;
	Draw2dVerticesBuilder(Draw2dVerticesBuilder&&) = default;
	Draw2dVerticesBuilder& operator=(const Draw2dVerticesBuilder&) = delete;
	Draw2dVerticesBuilder& operator=(Draw2dVerticesBuilder&&) = delete;

	Draw2dVerticesBuilder& vert(float x, float y, float u = 0, float v = 0)
	{
		verts_.push_back({x, y, u, v});
		tris_.elements += 1;
		return *this;
	}

	Draw2dVerticesBuilder& color(float r, float g, float b, float a)
	{
		r_ = r;
		g_ = g;
		b_ = b;
		a_ = a;
		return *this;
	}

	Draw2dVerticesBuilder& blend(BlendMode blend)
	{
		tris_.blend = blend;
		return *this;
	}

	Draw2dVerticesBuilder& lines(bool lines)
	{
		tris_.lines = lines;
		return *this;
	}

	Draw2dVerticesBuilder& flat(lumpnum_t lump)
	{
		tris_.flat_lump = lump;
		return *this;
	}

	void done();
};

inline Draw2dQuadBuilder Twodee::begin_quad() noexcept
{
	return Draw2dQuadBuilder(*this);
}

inline Draw2dVerticesBuilder Twodee::begin_verts() noexcept
{
	return Draw2dVerticesBuilder(*this);
}

} // namespace srb2::hwr2

#endif // __SRB2_HWR2_TWODEE_HPP__
