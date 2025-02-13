// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef p_sweep_hpp
#define p_sweep_hpp

#include <optional>
#include <variant>

#include "math/fixed.hpp"
#include "math/line_equation.hpp"
#include "math/line_segment.hpp"
#include "math/vec.hpp"

namespace srb2::sweep
{

using unit = math::Fixed;
using vec2 = math::Vec2<unit>;
using line_segment = math::LineSegment<unit>;

struct Contact
{
	unit z; // time
	vec2 n; // normal TODO REMOVE duplicate for each contact
	vec2 p; // contact point 1
	std::optional<vec2> q; // AABBvsLine: contact point 2
};

struct Result
{
	std::optional<Contact> hit, exit; // TODO result itself should be optional, not each contact
};

namespace detail
{

template <typename T>
struct BaseAABBvsLine : protected srb2::math::Traits<unit>
{
public:
	Result operator()(const line_segment& l) const
	{
		auto derived = static_cast<const T*>(this);
		return l.vertical() ? derived->vs_vertical(l) : derived->vs_slope(l);
	}

protected:
	unit r_; // AABB radius
	vec2 ds_; // sweep direction signs

	BaseAABBvsLine(unit r, const vec2& d, unit pz, unit dz) :
		r_(r), ds_(copysign(kUnit, d.x), copysign(kUnit, d.y)), t_(pz, dz) {}

	unit time(unit x) const { return (x - t_.x) / t_.y; }

	static Result order(std::optional<Contact>&& t1, std::optional<Contact>&& t2, unit s)
	{
		return s > kZero ? Result {t1, t2} : Result {t2, t1};
	}

	static vec2 normal(const vec2& v)
	{
		// Normalize vector so that x is positive -- normal always points up.
		return v.normal() * (copysign(kUnit, v.x) / v.magnitude());
	}

	static vec2 normal(const line_segment& l) { return normal(l.b - l.a); }

private:
	vec2 t_; // origin and length for calculating time
};

}; // namespace detail

// Sweep can be represented as y = mx + b
struct SlopeAABBvsLine : detail::BaseAABBvsLine<SlopeAABBvsLine>
{
	SlopeAABBvsLine(unit r, const line_segment& l) : SlopeAABBvsLine(r, l.a, l.b - l.a) {}

	Result vs_slope(const line_segment& l) const;
	Result vs_vertical(const line_segment& l) const;

private:
	math::LineEquationX<unit> q_;

	SlopeAABBvsLine(unit r, const vec2& p, const vec2& d) : BaseAABBvsLine(r, d, p.x, d.x), q_(p, d) {}
};

// Sweep is vertical
struct VerticalAABBvsLine : detail::BaseAABBvsLine<VerticalAABBvsLine>
{
	VerticalAABBvsLine(unit r, const line_segment& l) : VerticalAABBvsLine(r, l.a, l.b - l.a) {}

	Result vs_slope(const line_segment& l) const;
	Result vs_vertical(const line_segment& l) const;

private:
	unit x_;

	VerticalAABBvsLine(unit r, const vec2& p, const vec2& d) : BaseAABBvsLine(r, d, p.y, d.y), x_(p.x) {}
};

struct AABBvsLine
{
	AABBvsLine(unit r, const line_segment& l) :
		var_(l.vertical() ? var_t {VerticalAABBvsLine(r, l)} : var_t {SlopeAABBvsLine(r, l)})
	{
	}

	Result operator()(const line_segment& l) const
	{
		Result rs;
		std::visit([&](auto& sweeper) { rs = sweeper(l); }, var_);
		return rs;
	}

private:
	using var_t = std::variant<SlopeAABBvsLine, VerticalAABBvsLine>;
	var_t var_;
};

}; // namespace srb2::sweep

#endif/*p_sweep_hpp*/
