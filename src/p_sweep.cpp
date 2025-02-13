// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <optional>

#include "p_sweep.hpp"

using namespace srb2::math;
using namespace srb2::sweep;

Result SlopeAABBvsLine::vs_slope(const line_segment& l) const
{
	auto [a, b] = l.by_x(); // left, right
	LineEquation<unit> ql{l};
	unit ls = copysign(kUnit, ql.m());

	auto hit = [&, &a = a, &b = b](const vec2& k, unit xr, unit x, const vec2& n) -> Contact
	{
		std::optional<vec2> k2;

		if (l.horizontal())
		{
			// Horizontal line: create second contact point on opposite corner.
			// TODO: avoid duplicate point
			k2 = vec2(std::clamp(x + xr, a.x, b.x), k.y);
		}

		return {time(x), n, k, k2};
	};

	auto slide = [&](const vec2& k, const vec2& s) -> std::optional<Contact>
	{
		vec2 kf = k * s;
		vec2 r = r_ * s;
		vec2 p = k - r;

		// Slide vertically along AABB left/right edge.
		unit f = q_.y(p.x) * s.y;

		if (f - r_ > kf.y)
		{
			// Out of bounds detection.
			// This should never slide in front.
			// If it does, there was never a hit.
			return {};
		}

		if (f + r_ < kf.y)
		{
			// Slid behind contact point.
			// Try sliding horizontally along AABB top/bottom
			// edge.

			if (q_.m() == kZero)
			{
				// Sweep is horizontal.
				// It is impossible to slide against a line's
				// end by the X axis because the line segment
				// lies on that axis.
				return {};
			}

			p.x = q_.x(p.y);
			f = p.x * s.x;

			if (f - r_ > kf.x)
			{
				// Slid beyond contact point.
				return {};
			}

			if (f + r_ < kf.x)
			{
				// Out of bounds detection.
				// This should never slide behind.
				// If it does, there was never a hit.
				return {};
			}

			return hit(k, r.x, p.x, {kZero, -s.y});
		}

		return hit(k, r.x, p.x, {-s.x, kZero});
	};

	// xrs.x = x radius
	// xrs.y = x sign
	auto bind = [&, &a = a, &b = b](const vec2& k, const vec2& xrs, unit ns) -> std::optional<Contact>
	{
		if (k.x < a.x)
		{
			return slide(a, {xrs.y, ls});
		}

		if (k.x > b.x)
		{
			return slide(b, {xrs.y, -ls});
		}

		return hit(k, xrs.x, k.x + xrs.x, normal(l) * ns);
	};

	if (ql.m() == q_.m())
	{
		// Parallel lines can only cross at the ends.
		vec2 s{kUnit, ls};
		return order(slide(a, s), slide(b, -s), ds_.x);
	}

	vec2 i = ql.intersect(q_);

	// Compare slopes to determine if ray is moving upward or
	// downward into line.
	// For a positive line, AABB top left corner hits the
	// line first if the ray is moving upward.
	// Swap diagonal corners to bottom right if moving
	// downward.
	unit ys = q_.m() * ds_.x < ql.m() * ds_.x ? -kUnit : kUnit;
	unit yr = r_ * ys;

	// Swap left/right corners if line is negative.
	unit xr = yr * ls;

	// Intersection as if ray were offset -r, +r.
	vec2 v = [&]
	{
		unit y = (q_.m() * xr) + yr;
		unit x = y / (ql.m() - q_.m());
		return vec2 {x, (x * q_.m()) + y};
	}();

	// Find the intersection along diagonally oppposing AABB
	// corners.
	vec2 xrs{xr, ds_.x};
	return {bind(i + v, xrs, -ys), bind(i - v, -xrs, -ys)};
}

// TODO: Comments. Bitch.
Result SlopeAABBvsLine::vs_vertical(const line_segment& l) const
{
	auto [a, b] = l.by_y(); // bottom, top

	auto hit = [&](const vec2& p, std::optional<vec2> q, unit x, const vec2& n) -> Contact { return {time(x), n, p, q}; };

	auto bind = [&](const vec2& k, const vec2& a, const vec2& b, const vec2& s, auto limit) -> std::optional<Contact>
	{
		vec2 r = r_ * s;
		vec2 af = a * s;
		unit kyf = k.y * s.y;

		if (kyf + r_ < af.y)
		{
			if (q_.m() == kZero)
			{
				return {};
			}

			unit x = q_.x(a.y - r.y);

			if ((x * s.x) - r_ > af.x)
			{
				return {};
			}

			return hit(a, {}, x, {kZero, -s.y});
		}

		// TODO: avoid duplicate point
		vec2 k2{k.x, limit(k.y - r.y, a.y)};
		unit byf = b.y * s.y;
		vec2 n{-s.x, kZero};

		if (kyf + r_ > byf)
		{
			if (kyf - r_ > byf)
			{
				return {};
			}

			return hit(b, k2, k.x - r.x, n);
		}

		return hit(vec2(k.x, k.y + r.y), k2, k.x - r.x, n);
	};

	vec2 i{a.x, q_.y(a.x)};
	vec2 v{kZero, q_.m() * r_ * ds_.x * ds_.y};
	vec2 s = ds_ * ds_.y;

	// Damn you, template overloads!
	auto min = [](unit x, unit y) { return std::min(x, y); };
	auto max = [](unit x, unit y) { return std::max(x, y); };

	return order(bind(i - v, a, b, s, max), bind(i + v, b, a, -s, min), ds_.y);
}

Result VerticalAABBvsLine::vs_slope(const line_segment& l) const
{
	auto [a, b] = l.by_x(); // left, right
	LineEquation<unit> ql{l};

	auto hit = [&, &a = a, &b = b](const vec2& k, unit xr, unit y, const vec2& n) -> Contact
	{
		std::optional<vec2> k2;

		if (l.horizontal())
		{
			// Horizontal line: create second contact point on opposite corner.
			// TODO: avoid duplicate point
			k2 = vec2(std::clamp(x_ + xr, a.x, b.x), k.y);
		}

		return {time(y), n, k, k2};
	};

	auto bind = [&](const vec2& a, const vec2& b, const vec2& s) -> std::optional<Contact>
	{
		vec2 r = r_ * s;
		unit xf = x_ * s.x;

		if (xf - r_ > b.x * s.x)
		{
			return {};
		}

		unit axf = a.x * s.x;

		if (xf - r_ < axf)
		{
			if (xf + r_ < axf)
			{
				return {};
			}

			return hit(a, r.x, a.y - r.y, {kZero, -s.y});
		}

		vec2 i{x_, ql.y(x_)};
		vec2 v{r.x, ql.m() * r.x};
		vec2 k = i - v;
		return hit(k, r.x, k.y - r.y, normal(l) * -s.y);
	};

	unit mys = copysign(kUnit, ql.m() * ds_.y);
	vec2 s{kUnit, ds_.y * mys};
	return order(bind(a, b, s), bind(b, a, -s), mys);
}

Result VerticalAABBvsLine::vs_vertical(const line_segment& l) const
{
	// Box does not overlap Y plane.
	if (x_ + r_ < l.a.x || x_ - r_ > l.a.x)
	{
		return {};
	}

	auto [a, b] = l.by_y(); // bottom, top

	auto hit = [&](const vec2& k, unit yr) -> Contact { return {time(k.y + yr), {kZero, -ds_.y}, k}; };

	// Offset away from line ends.
	// Contacts are opposite when swept downward.
	return order(hit(a, -r_), hit(b, r_), ds_.y);
}
