// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <unordered_map>

#include "doomdef.h" // skincolornum_t
#include "doomtype.h"
#include "hu_stuff.h"
#include "i_time.h"
#include "k_hud.h"
#include "m_fixed.h"
#include "r_draw.h"
#include "v_draw.hpp"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

using srb2::Draw;
using Chain = Draw::Chain;

int Draw::TextElement::width() const
{
	return font_ ? font_width(*font_, default_font_flags(*font_) | flags_.value_or(0), string_.c_str()) / FRACUNIT : 0;
}

Draw::TextElement& Draw::TextElement::parse(std::string_view raw)
{
	static const std::unordered_map<std::string_view, char> translation = {
#define BUTTON(str, lower_bits) \
		{str,             0xB0 | lower_bits},\
		{str "_animated", 0xA0 | lower_bits},\
		{str "_pressed",  0x90 | lower_bits}

		BUTTON("up", 0x00),
		BUTTON("down", 0x01),
		BUTTON("right", 0x02),
		BUTTON("left", 0x03),

		BUTTON("r", 0x07),
		BUTTON("l", 0x08),
		BUTTON("start", 0x09),

		BUTTON("a", 0x0A),
		BUTTON("b", 0x0B),
		BUTTON("c", 0x0C),

		BUTTON("x", 0x0D),
		BUTTON("y", 0x0E),
		BUTTON("z", 0x0F),

#undef BUTTON

		{"white", 0x80},
		{"purple", 0x81},
		{"yellow", 0x82},
		{"green", 0x83},
		{"blue", 0x84},
		{"red", 0x85},
		{"gray", 0x86},
		{"orange", 0x87},
		{"sky", 0x88},
		{"lavender", 0x89},
		{"gold", 0x8A},
		{"aqua", 0x8B},
		{"magenta", 0x8C},
		{"pink", 0x8D},
		{"brown", 0x8E},
		{"tan", 0x8F},
	};

	string_.clear();
	string_.reserve(raw.size());

	using std::size_t;
	using std::string_view;

	for (;;)
	{
		size_t p = raw.find('<');

		// Copy characters until the start tag
		string_.append(raw.substr(0, p));

		if (p == raw.npos)
		{
			break; // end of string
		}

		raw.remove_prefix(p);

		// Find end tag
		p = raw.find('>');

		if (p == raw.npos)
		{
			break; // no end tag
		}

		string_view code = raw.substr(1, p - 1);

		if (auto it = translation.find(code); it != translation.end())
		{
			string_.push_back(it->second); // replace with character code
		}
		else
		{
			string_.append(raw.substr(0, p + 1)); // code not found, leave text verbatim
		}

		raw.remove_prefix(p + 1); // past end of tag
	}

	return *this;
}

void Chain::patch(patch_t* patch) const
{
	const auto _ = Clipper(*this);

	const bool stretchH = stretch_ == Stretch::kWidth || stretch_ == Stretch::kBoth;
	const bool stretchV = stretch_ == Stretch::kHeight || stretch_ == Stretch::kBoth;

	const fixed_t h = stretchH ? FloatToFixed(width_ / patch->width) : FRACUNIT;
	const fixed_t v = stretchV ? FloatToFixed(height_ / patch->height) : FRACUNIT;

	V_DrawStretchyFixedPatch(FloatToFixed(x_), FloatToFixed(y_), h * scale_, v * scale_, flags_, patch, colormap_);
}

void Chain::thumbnail(UINT16 mapnum) const
{
	const auto _ = Clipper(*this);

	K_DrawMapThumbnail(FloatToFixed(x_), FloatToFixed(y_), FloatToFixed(width_), flags_, mapnum, colormap_);
}

void Chain::fill(UINT8 color) const
{
	const auto _ = Clipper(*this);

	V_DrawFill(x_, y_, width_, height_, color|(flags_ & ~0xFF));
}

void Chain::string(const char* str, INT32 flags, Font font) const
{
	const auto _ = Clipper(*this);

	flags |= default_font_flags(font);

	fixed_t x = FloatToFixed(x_);
	fixed_t y = FloatToFixed(y_);

	switch (align_)
	{
	case Align::kLeft:
		break;

	case Align::kCenter:
		x -= (font_width(font, flags, str) / 2) * scale_;
		break;

	case Align::kRight:
		x -= font_width(font, flags, str) * scale_;
		break;
	}

	V_DrawStringScaled(x, y, FloatToFixed(scale_), FRACUNIT, FRACUNIT, flags, colormap_, font_to_fontno(font), str);
}

namespace
{

patch_t** get_button_patch(Draw::Button type, int ver)
{
	switch (type)
	{
#define X(x) \
	case Draw::Button::x:\
		return kp_button_ ## x

	X(a)[ver];
	X(b)[ver];
	X(c)[ver];
	X(x)[ver];
	X(y)[ver];
	X(z)[ver];
	X(start);
	X(l);
	X(r);
	X(up);
	X(down);
	X(right);
	X(left);

#undef X
	}

	return nullptr;
};

}; // namespace

void Chain::button_(Button type, int ver, std::optional<bool> press) const
{
	const auto _ = Clipper(*this);

	if (press)
	{
		K_drawButton(FloatToFixed(x_), FloatToFixed(y_), flags_, get_button_patch(type, ver), *press);
	}
	else
	{
		K_drawButtonAnim(x_, y_, flags_, get_button_patch(type, ver), I_GetTime());
	}
}

void Chain::sticker(patch_t* end_graphic, UINT8 color) const
{
	const auto _ = Clipper(*this);

	INT32 x = x_;
	INT32 y = y_;
	INT32 width = width_;
	INT32 flags = flags_ | V_FLIP;

	auto fill = [&](int x, int width) { V_DrawFill(x, y, width, SHORT(end_graphic->height), color | (flags_ & ~0xFF)); };

	if (align_ == Align::kRight)
	{
		width = -(width);
		flags ^= V_FLIP;
		fill(x + width, -(width));
	}
	else
	{
		fill(x, width);
	}

	V_DrawScaledPatch(x + width, y, flags, end_graphic);

	if (align_ == Align::kCenter)
	{
		V_DrawScaledPatch(x, y, flags ^ V_FLIP, end_graphic);
	}
}

Chain::Clipper::Clipper(const Chain& chain)
{
	V_SaveClipRect(&save_);

	if (chain.clip_)
	{
		V_SetClipRect(
			FloatToFixed(chain.clipx1_),
			FloatToFixed(chain.clipy1_),
			FloatToFixed(chain.clipx2_ - chain.clipx1_),
			FloatToFixed(chain.clipy2_ - chain.clipy1_),
			chain.flags_
		);
	}
}

Chain::Clipper::~Clipper()
{
	V_RestoreClipRect(&save_);
}

patch_t* Draw::cache_patch(const char* name)
{
	return static_cast<patch_t*>(W_CachePatchName(name, PU_CACHE));
}

int Draw::font_to_fontno(Font font)
{
	switch (font)
	{
	case Font::kThin:
		return TINY_FONT;

	case Font::kGamemode:
		return GM_FONT;

	case Font::kConsole:
		return HU_FONT;

	case Font::kFreeplay:
		return KART_FONT;

	case Font::kZVote:
		return OPPRF_FONT;

	case Font::kPing:
		return PINGF_FONT;

	case Font::kTimer:
		return TIMER_FONT;

	case Font::kThinTimer:
		return TINYTIMER_FONT;

	case Font::kMenu:
		return MENU_FONT;
	}

	return TINY_FONT;
};

INT32 Draw::default_font_flags(Font font)
{
	INT32 flags = 0;

	(void)font;

	return flags;
};

fixed_t Draw::font_width(Font font, INT32 flags, const char* string)
{
	return V_StringScaledWidth(FRACUNIT, FRACUNIT, FRACUNIT, flags, font_to_fontno(font), string);
}
