// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __V_DRAW_SETTER_HPP__
#define __V_DRAW_SETTER_HPP__

#include "r_draw.h" // R_GetTranslationColormap

namespace srb2
{

inline Draw::Chain& Draw::Chain::x(float x)
{
	x_ += x;
	return *this;
}

inline Draw::Chain& Draw::Chain::y(float y)
{
	y_ += y;
	return *this;
}

inline Draw::Chain& Draw::Chain::xy(float x, float y)
{
	x_ += x;
	y_ += y;
	return *this;
}

inline Draw::Chain& Draw::Chain::flags(INT32 flags)
{
	flags_ |= flags;
	return *this;
}

inline Draw::Chain& Draw::Chain::width(float width)
{
	width_ = width;
	return *this;
}

inline Draw::Chain& Draw::Chain::height(float height)
{
	height_ = height;
	return *this;
}

inline Draw::Chain& Draw::Chain::size(float width, float height)
{
	width_ = width;
	height_ = height;
	return *this;
}

inline Draw::Chain& Draw::Chain::scale(float scale)
{
	scale_ = scale;
	return *this;
}

inline Draw::Chain& Draw::Chain::font(Font font)
{
	font_ = font;
	return *this;
}

inline Draw::Chain& Draw::Chain::align(Align align)
{
	align_ = align;
	return *this;
}

inline Draw::Chain& Draw::Chain::stretch(Stretch stretch)
{
	stretch_ = stretch;
	return *this;
}

inline Draw::Chain& Draw::Chain::clipx(float left, float right)
{
	clipx1_ = left;
	clipx2_ = right;
	clip_ = true;
	return *this;
}

inline Draw::Chain& Draw::Chain::clipy(float top, float bottom)
{
	clipy1_ = top;
	clipy2_ = bottom;
	clip_ = true;
	return *this;
}

inline Draw::Chain& Draw::Chain::clip(bool yes)
{
	clip_ = yes;
	return *this;
}

inline Draw::Chain& Draw::Chain::colormap(const UINT8* colormap)
{
	colormap_ = colormap;
	return *this;
}

inline Draw::Chain& Draw::Chain::colormap(UINT16 color)
{
	return colormap(R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(color), GTC_CACHE));
}

inline Draw::Chain& Draw::Chain::colormap(INT32 skin, UINT16 color)
{
	return colormap(R_GetTranslationColormap(skin, static_cast<skincolornum_t>(color), GTC_CACHE));
}

inline Draw::Chain& Draw::Chain::colorize(UINT16 color)
{
	return colormap(R_GetTranslationColormap(TC_RAINBOW, static_cast<skincolornum_t>(color), GTC_CACHE));
}

}; // namespace srb2

#endif // __V_DRAW_SETTER_HPP__
