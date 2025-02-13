// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef math_line_equation_hpp
#define math_line_equation_hpp

#include "fixed.hpp"
#include "line_segment.hpp"
#include "vec.hpp"

namespace srb2::math
{

template <typename T>
struct LineEquation
{
	using vec2 = Vec2<T>;
	using line_segment = LineSegment<T>;

	// Fixed-point: shift value by this amount during
	// multiplications and divisions to avoid overflows.
	static constexpr std::enable_if_t<std::is_same_v<T, Fixed>, fixed_t> kF = 1024; // fixed_t, not Fixed

	LineEquation() {}
	LineEquation(const vec2& p, const vec2& d) : d_(d), m_(d.y / d.x), b_(p.y - (p.x * m())) {}
	LineEquation(const line_segment& l) : LineEquation(l.a, l.b - l.a) {}

	const vec2& d() const { return d_; }
	T m() const { return m_; }
	T b() const { return b_; }
	T y(T x) const { return (m() * x) + b(); }

	vec2 intersect(const LineEquation& q) const
	{
		T x = (b() - q.b()) / (q.m() - m());
		return {x, y(x)};
	}

protected:
	vec2 d_{};
	T m_{}, b_{};
};

template <>
inline LineEquation<Fixed>::LineEquation(const vec2& p, const vec2& d) :
	d_(d), m_((d.y / d.x) / kF), b_((p.y / kF) - (p.x * m_))
{
}

template <>
inline Fixed LineEquation<Fixed>::m() const
{
	return m_ * kF;
}

template <>
inline Fixed LineEquation<Fixed>::b() const
{
	return b_ * kF;
}

template <>
inline Fixed LineEquation<Fixed>::y(Fixed x) const
{
	return ((m_ * x) + b_) * kF;
}

template <>
inline LineEquation<Fixed>::vec2 LineEquation<Fixed>::intersect(const LineEquation& q) const
{
	Fixed x = ((b_ - q.b_) / ((q.m_ - m_) * kF)) * kF;
	return {x, y(x)};
}

template <typename T>
struct LineEquationX : LineEquation<T>
{
	T x(T y) const { return (y - LineEquation<T>::b()) / LineEquation<T>::m(); }
};

template <>
struct LineEquationX<Fixed> : LineEquation<Fixed>
{
	LineEquationX() {}
	LineEquationX(const vec2& p, const vec2& d) : LineEquation(p, d), w_((d.x / d.y) / kF), a_((p.x / kF) - (p.y * w_)) {}
	LineEquationX(const line_segment& l) : LineEquationX(l.a, l.b - l.a) {}

	Fixed x(Fixed y) const { return ((w_ * y) + a_) * kF; }

protected:
	Fixed w_{}, a_{};
};

}; // namespace srb2::math

#endif/*math_line_equation_hpp*/
