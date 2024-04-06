// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by James Robert Roman
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __V_DRAW_HPP__
#define __V_DRAW_HPP__

#include <string>
#include <string_view>
#include <optional>
#include <utility>

#include <fmt/core.h>

#include "doomdef.h" // skincolornum_t
#include "doomtype.h"
#include "screen.h" // BASEVIDWIDTH
#include "typedef.h"
#include "v_video.h"

namespace srb2
{

class Draw
{
public:
	enum class Font
	{
		kThin,
		kGamemode,
		kConsole,
		kFreeplay,
		kZVote,
		kPing,
		kTimer,
		kThinTimer,
		kMenu,
		kMedium,
		kRollingNum,
		kRollingNum4P,
	};

	enum class Align
	{
		kLeft,
		kCenter,
		kRight,
	};

	enum class Stretch
	{
		kNone,
		kWidth,
		kHeight,
		kBoth,
	};

	enum class Button
	{
		a,
		b,
		c,
		x,
		y,
		z,
		start,
		l,
		r,
		up,
		down,
		right,
		left,
	};

	class TextElement
	{
	public:
		explicit TextElement() {}

		explicit TextElement(std::string string) : string_(string) {}

		template <class... Args>
		explicit TextElement(fmt::format_string<Args...> format, Args&&... args) :
			TextElement(fmt::format(format, args...))
		{
		}

		const std::string& string() const { return string_; }
		std::optional<Font> font() const { return font_; }
		std::optional<INT32> flags() const { return flags_; }

		int width() const;

		TextElement& string(std::string string)
		{
			string_ = string;
			return *this;
		}

		TextElement& parse(std::string_view string);

		template <class... Args>
		TextElement& parse(fmt::format_string<Args...> format, Args&&... args)
		{
			return parse(fmt::format(format, args...));
		}

		TextElement& font(Font font)
		{
			font_ = font;
			return *this;
		}

		TextElement& flags(INT32 flags)
		{
			flags_ = flags;
			return *this;
		}

	private:
		std::string string_;
		std::optional<Font> font_;
		std::optional<INT32> flags_;
	};

	class Chain
	{
	public:
		float x() const { return x_; }
		float y() const { return y_; }
		INT32 flags() const { return flags_; }

		// Methods add relative to the current state
		Chain& x(float x);
		Chain& y(float y);
		Chain& xy(float x, float y);
		Chain& flags(INT32 flags);

		// Methods overwrite the current state
		Chain& width(float width);
		Chain& height(float height);
		Chain& size(float width, float height);
		Chain& scale(float scale);
		Chain& font(Font font);
		Chain& align(Align align);
		Chain& stretch(Stretch stretch);

		// Absolute screen coordinates
		Chain& clipx(float left, float right); // 0 to BASEVIDWIDTH
		Chain& clipy(float top, float bottom); // 0 to BASEVIDHEIGHT

		Chain& clipx() { return clipx(x_, x_ + width_); }
		Chain& clipy() { return clipy(y_, y_ + height_); }

		// True to use internal clipping state
		// False to use global state (default)
		// Changing the clipping dimensions implicitly sets
		// this to true
		Chain& clip(bool yes);

		Chain& colormap(const UINT8* colormap);
		Chain& colormap(UINT16 color);
		Chain& colormap(INT32 skin, UINT16 color);
		Chain& colorize(UINT16 color);

		void text(const char* str) const { string(str, flags_, font_); }
		void text(const std::string& str) const { text(str.c_str()); }
		void text(const TextElement& elm) const
		{
			string(elm.string().c_str(), elm.flags().value_or(flags_), elm.font().value_or(font_));
		}

		template <class... Args>
		void text(fmt::format_string<Args...> format, Args&&... args) const { text(fmt::format(format, args...)); }

		TextElement text() const { return TextElement().font(font_).flags(flags_); }

		void patch(patch_t* patch) const;
		void patch(const char* name) const { patch(Draw::cache_patch(name)); }
		void patch(const std::string& name) const { patch(name.c_str()); }

		void thumbnail(UINT16 mapnum) const;

		void fill(UINT8 color) const;

		void button(Button type, std::optional<bool> press = {}) const { button_(type, 0, press); }
		void small_button(Button type, std::optional<bool> press = {}) const { button_(type, 1, press); }

		void sticker(patch_t* end_graphic, UINT8 color) const;
		void sticker() const { sticker(Draw::cache_patch("K_STIKEN"), 24); }
		void small_sticker() const { sticker(Draw::cache_patch("K_STIKE2"), 24); }

	private:
		constexpr Chain() {}
		explicit Chain(float x, float y) : x_(x), y_(y) {}
		Chain(const Chain&) = default;
		Chain& operator=(const Chain&) = default;

		struct Clipper
		{
			explicit Clipper(const Chain& chain);
			~Clipper();

		private:
			cliprect_t save_;
		};

		float x_ = 0.f;
		float y_ = 0.f;
		float width_ = 0.f;
		float height_ = 0.f;
		float scale_ = 1.f;

		float clipx1_ = 0.f;
		float clipx2_ = BASEVIDWIDTH;
		float clipy1_ = 0.f;
		float clipy2_ = BASEVIDHEIGHT;
		bool clip_ = false;

		INT32 flags_ = 0;

		Font font_ = Font::kThin;
		Align align_ = Align::kLeft;
		Stretch stretch_ = Stretch::kNone;

		const UINT8* colormap_ = nullptr;

		void string(const char* str, INT32 flags, Font font) const;
		void button_(Button type, int ver, std::optional<bool> press = {}) const;

		friend Draw;
	};

	static patch_t* cache_patch(const char* name);

	constexpr Draw() {}
	explicit Draw(float x, float y) : chain_(x, y) {}
	Draw(const Chain& chain) : chain_(chain) {}

	// See class Chain for documentation

	float x() const { return chain_.x(); }
	float y() const { return chain_.y(); }
	INT32 flags() const { return chain_.flags(); }

#define METHOD(Name) \
	template <typename... Args>\
	Chain Name (Args&&... args) const { return Chain(chain_).Name(std::forward<Args>(args)...); }

	METHOD(x);
	METHOD(y);
	METHOD(xy);
	METHOD(flags);
	METHOD(width);
	METHOD(height);
	METHOD(size);
	METHOD(scale);
	METHOD(font);
	METHOD(align);
	METHOD(stretch);
	METHOD(clipx);
	METHOD(clipy);
	METHOD(clip);
	METHOD(colormap);
	METHOD(colorize);

#undef METHOD

#define VOID_METHOD(Name) \
	template <typename... Args>\
	auto Name (Args&&... args) const { return chain_.Name(std::forward<Args>(args)...); }

	VOID_METHOD(text);
	VOID_METHOD(patch);
	VOID_METHOD(thumbnail);
	VOID_METHOD(fill);
	VOID_METHOD(button);
	VOID_METHOD(small_button);
	VOID_METHOD(sticker);
	VOID_METHOD(small_sticker);

#undef VOID_METHOD

private:
	Chain chain_;

	static int font_to_fontno(Font font);
	static INT32 default_font_flags(Font font);
	static fixed_t font_width(Font font, INT32 flags, const char* string);
};

}; // namespace srb2

#include "v_draw_setter.hpp"

#endif // __V_DRAW_HPP__
