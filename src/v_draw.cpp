// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "core/hash_map.hpp"
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
#include "k_profiles.h" // controls
#include "p_local.h" // stplyr
#include "r_fps.h" // R_GetViewNumber()

using srb2::Draw;
using Chain = Draw::Chain;

int Draw::TextElement::width() const
{
	return font_ ? font_width(*font_, default_font_flags(*font_) | flags_.value_or(0), string_.c_str()) / FRACUNIT : 0;
}

Draw::TextElement& Draw::TextElement::parse(std::string_view raw)
{
	static const srb2::HashMap<std::string_view, char> translation = {
#define BUTTON(str, lower_bits) \
		{str,             0xB0 | lower_bits},\
		{str "_animated", 0xA0 | lower_bits},\
		{str "_pressed",  0x90 | lower_bits}

		BUTTON("up", sb_up),
		BUTTON("down", sb_down),
		BUTTON("right", sb_right),
		BUTTON("left", sb_left),

		BUTTON("lua1", sb_lua1),
		BUTTON("lua2", sb_lua2),
		BUTTON("lua3", sb_lua3),

		BUTTON("r", sb_r),
		BUTTON("l", sb_l),
		BUTTON("start", sb_start),

		BUTTON("a", sb_a),
		BUTTON("b", sb_b),
		BUTTON("c", sb_c),

		BUTTON("x", sb_x),
		BUTTON("y", sb_y),
		BUTTON("z", sb_z),

#undef BUTTON

		{"large", 0xEB},

		{"box", 0xEC},
		{"box_pressed", 0xED},
		{"box_animated", 0xEE},

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

	// When we encounter a Saturn button, what gamecontrol does it represent?
	static const srb2::HashMap<char, gamecontrols_e> inputdefinition = {
		{sb_up, gc_up},
		{sb_down, gc_down},
		{sb_right, gc_right},
		{sb_left, gc_left},

		{sb_lua1, gc_lua1},
		{sb_lua2, gc_lua2},
		{sb_lua3, gc_lua3},

		{sb_r, gc_r},
		{sb_l, gc_l},
		{sb_start, gc_start},

		{sb_a, gc_a},
		{sb_b, gc_b},
		{sb_c, gc_c},

		{sb_x, gc_x},
		{sb_y, gc_y},
		{sb_z, gc_z},
	};

	// What physical binds should appear as Saturn icons anyway?
	// (We don't have generic binds for stick/dpad directions, so
	// using the existing arrow graphics is the best thing here.)
	static const srb2::HashMap<INT32, char> prettyinputs = {
		{KEY_UPARROW, sb_up},
		{KEY_DOWNARROW, sb_down},
		{KEY_LEFTARROW, sb_left},
		{KEY_RIGHTARROW, sb_right},
		{nc_hatup, sb_up},
		{nc_hatdown, sb_down},
		{nc_hatleft, sb_left},
		{nc_hatright, sb_right},
		{nc_lsup, sb_up},
		{nc_lsdown, sb_down},
		{nc_lsleft, sb_left},
		{nc_lsright, sb_right},
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

		if (code == "dpad" || code == "dpad_pressed" || code == "dpad_animated")
		{
			// SPECIAL: Generic button that we invoke explicitly, not via gamecontrol reference.
			// If we ever add anything else to this category, I promise I will create a real abstraction,
			// but for now, just hardcode the character replacements and pray for forgiveness.

			string_.push_back(0xEF); // Control code: "switch to descriptive input mode"
			string_.push_back(0xEB); // Control code: "large button"
			if (code == "dpad")
				string_.push_back(0xBC);
			else if (code == "dpad_pressed")
				string_.push_back(0x9C);
			else
				string_.push_back(0xAC);
		}
		else if (auto it = translation.find(code); it != translation.end()) // This represents a gamecontrol, turn into Saturn button or generic button.
		{

			UINT8 localplayer = R_GetViewNumber();

			if (as_.has_value())
			{
				UINT8 indexedplayer = as_.value();
				for (UINT8 i = 0; i < MAXSPLITSCREENPLAYERS; i++)
				{
					if (g_localplayers[i] == indexedplayer)
					{
						localplayer = i;
						break;
					}
				}
			}


			// This isn't how v_video.cpp checks for buttons and I don't know why.
			if (cv_descriptiveinput[localplayer].value && ((it->second & 0xF0) != 0x80)) // Should we do game control translation?
			{
				if (auto id = inputdefinition.find(it->second & (~0xB0)); id != inputdefinition.end()) // This is a game control, do descriptive input translation!
				{
					// Grab our local controls  - if pid set in the call to parse(), use stplyr's controls
					INT32 bind = G_FindPlayerBindForGameControl(localplayer, id->second);

					// EXTRA: descriptiveinput values above 1 translate binds back to Saturn buttons,
					// with various modes for various fucked up 6bt pads
					srb2::HashMap<INT32, char> padconfig = {};
					switch (cv_descriptiveinput[localplayer].value)
					{
						case 1:
							padconfig = standardpad;
							break;
						case 2:
							padconfig = flippedpad;
							break;
						case 3:
						{
							// Most players will map gc_L to their physical L button,
							// and gc_R to their physical R button. Assuming this is
							// true, try to guess their physical layout based on what
							// they've chosen.

							INT32 leftbumper = G_FindPlayerBindForGameControl(localplayer, gc_l);
							INT32 rightbumper = G_FindPlayerBindForGameControl(localplayer, gc_r);

							if (leftbumper == nc_lb && rightbumper == nc_lt)
							{
								padconfig = saturntypeA;
							}
							else if (leftbumper == nc_lt && rightbumper == nc_rt)
							{
								padconfig = saturntypeB;
							}
							else if (leftbumper == nc_lb && rightbumper == nc_rb)
							{
								padconfig = saturntypeC;
							}
							else if (leftbumper == nc_ls && rightbumper == nc_lb)
							{
								padconfig = saturntypeE;
							}
							else if (leftbumper == nc_rs && rightbumper == nc_lt)
							{
								padconfig = saturntypeE; // Not a typo! Users might bind a Hori layout pad to either bumpers or triggers
							}
							else
							{
								padconfig = saturntypeA; // :( ???
							}
							break;
						}
						case 4:
							padconfig = saturntypeA;
							break;
						case 5:
							padconfig = saturntypeB;
							break;
						case 6:
							padconfig = saturntypeC;
							break;
						case 7:
							padconfig = saturntypeD;
							break;
						case 8:
							padconfig = saturntypeE;
							break;
						case 9:
							padconfig = saturntypeF;
							break;
						case 10:
							padconfig = saturntypeG;
							break;
					}

					if (auto pretty = prettyinputs.find(bind); pretty != prettyinputs.end()) // Gamepad direction or keyboard arrow, use something nice-looking
					{
						string_.push_back((it->second & 0xF0) | pretty->second); // original invocation has the animation bits, but the glyph bits come from the table
					}
					else if (auto pad = padconfig.find(bind); pad != padconfig.end())
					{
						// If high bits are set, this is meant to be a generic button.
						if (pad->second & 0xF0)
						{
							string_.push_back(0xEF); // Control code: "switch to descriptive input mode" - buttons will draw as generics
							string_.push_back(0xEB); // Control code: "large button"
						}

						// Clear high bits so we can add animation bits back cleanly.
						pad->second = pad->second & (0x0F);

						// original invocation has the animation bits, but the glyph bits come from the table
						string_.push_back((it->second & 0xF0) | pad->second);
					}
					else
					{
						UINT8 fragment = (it->second & 0xB0);
						UINT8 code = '\xEE'; // Control code: "toggle boxed drawing"

						if (fragment == 0xA0)
							code = '\xED'; // ... but animated
						else if (fragment == 0x90)
							code = '\xEC'; // ... but pressed

						string_.push_back(code);

						if (bind == -1)
							string_.append("N/A");
						else
							string_.append((G_KeynumToShortString(bind)));

						string_.push_back(code);
					}
				}
				else // This is a color code or some other generic glyph, treat it as is.
				{
					string_.push_back(it->second); // replace with character code
				}
			}
			else // We don't care whether this is a generic glyph, because input translation isn't on.
			{
				string_.push_back(it->second); // replace with character code
			}
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
	if (!str)
	{
		return;
	}

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
	X(start)[ver];
	X(l)[ver];
	X(r)[ver];
	X(up)[ver];
	X(down)[ver];
	X(right)[ver];
	X(left)[ver];
	X(lua1)[ver];
	X(lua2)[ver];
	X(lua3)[ver];

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

patch_t** get_button_patch(Draw::GenericButton type, int ver)
{
	switch (type)
	{
#define X(x) \
	case Draw::GenericButton::x:\
		return gen_button_ ## x

	X(a)[ver];
	X(b)[ver];
	X(x)[ver];
	X(y)[ver];
	X(lb)[ver];
	X(rb)[ver];
	X(lt)[ver];
	X(rt)[ver];
	X(start)[ver];
	X(back)[ver];
	X(ls)[ver];
	X(rs)[ver];
	X(dpad)[ver];

#undef X
	}

	return nullptr;
};

void Chain::generic_button_(GenericButton type, int ver, std::optional<bool> press) const
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

Draw::Font Draw::fontno_to_font(int font)
{
	switch (font)
	{
	case TINY_FONT:
	default:
		return Font::kThin;

	case GM_FONT:
		return Font::kGamemode;

	case GENESIS_FONT:
		return Font::kGenesis;

	case HU_FONT:
		return Font::kConsole;

	case KART_FONT:
		return Font::kFreeplay;

	case OPPRF_FONT:
		return Font::kZVote;

	case PINGF_FONT:
		return Font::kPing;

	case TIMER_FONT:
		return Font::kTimer;

	case TINYTIMER_FONT:
		return Font::kThinTimer;

	case MENU_FONT:
		return Font::kMenu;

	case MED_FONT:
		return Font::kMedium;

	case ROLNUM_FONT:
		return Font::kRollingNum;

	case RO4NUM_FONT:
		return Font::kRollingNum4P;
	}
};

int Draw::font_to_fontno(Font font)
{
	switch (font)
	{
	case Font::kThin:
	default:
		return TINY_FONT;

	case Font::kGamemode:
		return GM_FONT;

	case Font::kGenesis:
		return GENESIS_FONT;

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

	case Font::kMedium:
		return MED_FONT;

	case Font::kRollingNum:
		return ROLNUM_FONT;

	case Font::kRollingNum4P:
		return RO4NUM_FONT;
	}
};

INT32 Draw::default_font_flags(Font font)
{
	INT32 flags = 0;

	(void)font;

	return flags;
};

fixed_t Draw::font_width(Font font, INT32 flags, const char* string)
{
	if (!string)
	{
		return 0;
	}

	return V_StringScaledWidth(FRACUNIT, FRACUNIT, FRACUNIT, flags, font_to_fontno(font), string);
}
