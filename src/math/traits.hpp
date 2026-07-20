// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef MATH_TRAITS_HPP
#define MATH_TRAITS_HPP

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

#endif/*MATH_TRAITS_HPP*/
