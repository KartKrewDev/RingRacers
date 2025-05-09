// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef math_fixed_hpp
#define math_fixed_hpp

#include <type_traits>

#include "traits.hpp"

#include "../m_fixed.h"

namespace srb2::math
{

struct Fixed
{
	static Fixed copysign(fixed_t x, fixed_t y) { return (x < 0) != (y < 0) ? -x : x; }
	static Fixed hypot(fixed_t x, fixed_t y) { return FixedHypot(x, y); }

	constexpr Fixed() : val_(0) {}
	constexpr Fixed(fixed_t val) : val_(val) {}

	template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
	Fixed(T val) : val_(FloatToFixed(val)) {}

	Fixed(const Fixed& b) = default;
	Fixed& operator=(const Fixed& b) = default;

	fixed_t value() const { return val_; }
	int sign() const { return val_ < 0 ? -1 : 1; }

	operator fixed_t() const { return val_; }
	explicit operator float() const { return FixedToFloat(val_); }

	Fixed& operator+=(const Fixed& b)
	{
		val_ += b.val_;
		return *this;
	}

	Fixed& operator-=(const Fixed& b)
	{
		val_ -= b.val_;
		return *this;
	}

	Fixed& operator*=(const Fixed& b)
	{
		val_ = FixedMul(val_, b.val_);
		return *this;
	}

	Fixed& operator/=(const Fixed& b)
	{
		val_ = FixedDiv(val_, b.val_);
		return *this;
	}

	Fixed operator-() const { return -val_; }

#define X(op) \
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>> \
	Fixed operator op(const T& b) const { return val_ op b; } \
	Fixed operator op(const Fixed& b) const \
	{ \
		Fixed f{val_};\
		f op##= b;\
		return f;\
	} \
	template <typename T> \
	Fixed& operator op##=(const T& b) \
	{ \
		val_ op##= b; \
		return *this; \
	}

	X(+)
	X(-)
	X(*)
	X(/)

#undef X

private:
	fixed_t val_;
};

template <>
struct Traits<Fixed>
{
	static constexpr Fixed kZero = 0;
	static constexpr Fixed kUnit = FRACUNIT;

	static constexpr auto copysign = Fixed::copysign;
	static constexpr auto hypot = Fixed::hypot;
};

}; // namespace srb2::math

#endif/*math_fixed_hpp*/
