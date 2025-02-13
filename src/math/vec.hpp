// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef math_vec_hpp
#define math_vec_hpp

#include <type_traits>

#include "traits.hpp"

namespace srb2::math
{

template <typename T>
struct Vec2
{
	T x, y;

	constexpr Vec2() : x{}, y{} {}
	constexpr Vec2(T x_, T y_) : x(x_), y(y_) {}
	constexpr Vec2(T z) : x(z), y(z) {}

	template <typename U>
	Vec2(const Vec2<U>& b) : Vec2(b.x, b.y) {}

	T magnitude() const { return Traits<T>::hypot(x, y); }
	Vec2 normal() const { return {-y, x}; }

#define X(op) \
	Vec2& operator op##=(const Vec2& b) \
	{ \
		x op##= b.x; \
		y op##= b.y; \
		return *this; \
	} \
	Vec2 operator op(const Vec2& b) const { return Vec2(x op b.x, y op b.y); } \

	X(+)
	X(-)
	X(*)
	X(/)

#undef X

	Vec2 operator-() const { return Vec2(-x, -y); }
};

template <typename>
struct is_vec2 : std::false_type {};

template <typename T>
struct is_vec2<Vec2<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_vec2_v = is_vec2<T>::value;

#define X(op) \
	template <typename T, typename U, std::enable_if_t<!is_vec2_v<T>, bool> = true> \
	Vec2<T> operator op(const T& a, const Vec2<U>& b) \
	{ \
		return Vec2 {a} op Vec2<T> {b}; \
	} \
	template <typename T, typename U, std::enable_if_t<!is_vec2_v<U>, bool> = true> \
	Vec2<U> operator op(const Vec2<T>& a, const U& b) \
	{ \
		return Vec2<U> {a} op Vec2 {b}; \
	} \

X(+)
X(-)
X(*)
X(/)

#undef X

}; // namespace srb2::math

#endif/*math_vec_hpp*/
