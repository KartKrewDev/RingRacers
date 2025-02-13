// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef math_line_segment_hpp
#define math_line_segment_hpp

#include <algorithm>
#include <utility>

#include "vec.hpp"

namespace srb2::math
{

template <typename T>
struct LineSegment
{
	using vec2 = Vec2<T>;
	using view = std::pair<const vec2&, const vec2&>;

	vec2 a, b;

	LineSegment(vec2 a_, vec2 b_) : a(a_), b(b_) {}

	template <typename U>
	LineSegment(const LineSegment<U>& b) : LineSegment(b.a, b.b) {}

	bool horizontal() const { return a.y == b.y; }
	bool vertical() const { return a.x == b.x; }

	view by_x() const { return std::minmax(a, b, [](auto& a, auto& b) { return a.x < b.x; }); }
	view by_y() const { return std::minmax(a, b, [](auto& a, auto& b) { return a.y < b.y; }); }
};

}; // namespace srb2

#endif/*math_line_segment_hpp*/
