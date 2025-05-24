// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef math_traits_hpp
#define math_traits_hpp

#include <cmath>
#include <type_traits>

namespace srb2::math
{

template <typename T, typename = void>
struct Traits;

template <typename T>
struct Traits<T, std::enable_if_t<std::is_floating_point_v<T>>>
{
	static constexpr T kZero = 0.0;
	static constexpr T kUnit = 1.0;

	static T copysign(T x, T y) { return std::copysign(x, y); }
	static T hypot(T x, T y) { return std::hypot(x, y); }
};

}; // namespace srb2::math

#endif/*math_traits_hpp*/
