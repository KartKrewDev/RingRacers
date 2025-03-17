// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __EGGTV_HPP__
#define __EGGTV_HPP__

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

#include "EggTVData.hpp"
#include "EggTVGraphics.hpp"

#include "../../core/vector.hpp"
#include "../../doomdef.h" // TICRATE
#include "../../i_time.h"
#include "../../k_menu.h"
#include "../../m_fixed.h"
#include "../../sounds.h"
#include "../../v_draw.hpp"

namespace srb2::menus::egg_tv
{

class EggTV : private EggTVData, private EggTVGraphics
{
public:
	struct InputReaction
	{
		bool bypass = false; // use default menu controls
		bool effect = false; // input was not ignored
		sfxenum_t sound = sfx_s3k5b;
	};

	explicit EggTV() : EggTVData() {}

	InputReaction input(int pid);

	bool select();
	void back();

	void watch() const;
	void erase();
	void toggle_favorite();
	void standings();

	bool favorited() const;

	void draw() const;

private:
	// TODO: separate some of these classes into separate files for general use
	class Animation
	{
	public:
		class Value
		{
		public:
			Value(fixed_t n) : n_(n) {}

			Value invert() const { return FRACUNIT - n_; }
			Value invert_if(bool yes) const { return yes ? invert() : *this; }

			operator fixed_t() const { return n_; }

		private:
			fixed_t n_;
		};

		enum Timing
		{
			kOnce,
			kLooping,
		};

		explicit Animation(unsigned int duration, Timing kind = kOnce) : kind_(kind), defaultDuration_(duration) {}

		void start(int offset = 0) { start_at(now() + offset); }
		void stop() { start_at(now() - duration()); }
		void start_with(const Animation& b) { start_at(b.starting_point()); }
		void start_after(const Animation& b) { start_at(b.stopping_point()); }
		void end_with(const Animation& b) { start_at(b.stopping_point() - duration()); }
		void end_before(const Animation& b) { start_at(b.starting_point() - duration()); }

		void delay() { start(1 - elapsed()); }
		void extend(float f) { duration_ += defaultDuration_ * f; }

		tic_t starting_point() const { return epoch_; }
		tic_t stopping_point() const { return epoch_ + duration(); }

		bool running() const
		{
			switch (kind_)
			{
				case kOnce:
					return elapsed() < duration();

				case kLooping:
					return !delayed();
			}

			return false;
		}

		bool delayed() const { return starting_point() > now(); }

		unsigned int elapsed() const { return delayed() ? 0 : now() - starting_point(); }
		unsigned int duration() const { return duration_; }

		Value variable() const { return variable_or(0); }
		Value reverse() const { return FRACUNIT - variable_or(FRACUNIT); }
		Value reverse_if(bool yes) const { return yes ? reverse() : variable(); }

		Animation operator +(int offset) const
		{
			Animation anim = *this;
			anim.epoch_ += offset;
			return anim;
		}

		Animation operator -(int offset) const { return *this + -(offset); }

	private:
		Timing kind_;
		unsigned int defaultDuration_;
		unsigned int duration_ = defaultDuration_;
		tic_t epoch_ = 0;

		void start_at(tic_t when)
		{
			epoch_ = when;
			duration_ = defaultDuration_;
		}

		fixed_t variable_or(fixed_t alt) const
		{
			switch (kind_)
			{
			case kOnce:
				return running() ? count() : alt;

			case kLooping:
				return count() % FRACUNIT;
			}

			return alt;
		}

		fixed_t count() const { return (elapsed() * FRACUNIT) / std::max(1u, duration()); }

		static tic_t now() { return gametic; }
	};

	class Mode
	{
	public:
		enum Value
		{
			kFolders,
			kGrid,
			kReplay,
			kStandings,
		};

		explicit Mode(Value mode) : next_(mode) {}

		void change(Value mode, tic_t when)
		{
			prev_ = next_;
			next_ = mode;
			stop_ = when;
		}

		Value get() const { return (stop_ <= gametic ? next_ : prev_); }
		Value next() const { return next_; }

		bool changing() const { return gametic < stop_; }
		bool changing_to(Value type) const { return changing() && next_ == type; }

		bool operator ==(Value b) const { return get() == b; }
		operator Value() const { return get(); }

	private:
		Value next_;
		Value prev_ = next_;
		tic_t stop_ = 0;
	};

	class Cursor
	{
	public:
		using limiter_t = std::function<int()>;
		using anims_t = srb2::Vector<Animation*>;

		explicit Cursor(anims_t anims, limiter_t limiter) : limiter_(limiter), anims_(anims) {}

		int pos() const { return pos_; }
		int end() const { return std::max(1, limiter_()); }
		float fraction() const { return pos() / static_cast<float>(end()); }
		int change() const { return change_; }

		bool wrap() const
		{
			const int prev = pos() - change();

			return prev < 0 || prev >= end();
		}

		Cursor& operator +=(int n)
		{
			if (n != 0)
			{
				pos_ += n;
				change_ = n;
			}

			if (pos_ < 0)
			{
				pos_ = end() -  1;
			}
			else
			{
				pos_ %= end();
			}

			if (n != 0)
			{
				// TODO?
				for (Animation* anim : anims_)
				{
					if (anim->running())
					{
						anim->delay();
					}
					else
					{
						anim->start();
					}
				}
			}

			return *this;
		}

		Cursor& operator =(int n)
		{
			pos_ = n;
			pos_ += 0;

			return *this;
		}

	private:
		limiter_t limiter_;

		int pos_ = 0;
		int change_ = 0;

		anims_t anims_;
	};

	Mode mode_{Mode::kFolders};

	Animation rowSlide_{4};
	Animation folderSlide_{8};
	Animation dashRise_{6};
	Animation dashTextRise_{8};
	Animation dashTextScroll_{12*TICRATE, Animation::kLooping};
	Animation gridFade_{4};
	Animation gridPopulate_{16};
	Animation gridSelectX_{3};
	Animation gridSelectY_{3};
	Animation gridSelectShake_{6};
	Animation enhanceMove_{2};
	Animation enhanceZoom_{4};
	Animation replaySlide_{6};
	Animation buttonSlide_{6};
	Animation buttonHover_{4};
	Animation replayTitleScroll_{24*TICRATE, Animation::kLooping};
	Animation favSlap_{8};
	Animation standingsRowSlide_{6};

	static constexpr int kGridColsPerRow = 5;

	Cursor folderRow_{{&rowSlide_}, [this]{ return folders_.size(); }};
	Cursor gridCol_{{&gridSelectX_}, []{ return kGridColsPerRow; }};
	Cursor gridRow_{{&gridSelectY_, &rowSlide_}, [this] { return grid_rows(); }};
	Cursor standingsRow_{{&standingsRowSlide_}, [this]{ return standings_rows(); }};

	std::size_t grid_index() const { return gridCol_.pos() + (gridRow_.pos() * kGridColsPerRow); }
	std::size_t grid_rows() const;

	std::size_t standings_rows() const;

	PatchManager::patch gametype_graphic(const Replay& replay) const;

	void draw_background() const;
	void draw_overlay() const;
	void draw_folder_header() const;
	void draw_folders() const;
	void draw_scroll_bar() const;
	void draw_dash() const;
	void draw_dash_text(const Replay& replay) const;

	enum class GridMode
	{
		kPopulating,
		kFinal,
	};

	struct GridOffsets;

	template <GridMode Mode>
	void draw_grid() const;

	void draw_grid_mesh(const GridOffsets& grid) const;
	void draw_grid_select(const GridOffsets& grid) const;
	void draw_grid_enhance(const GridOffsets& grid) const;

	void draw_replay(const Replay& replay) const;
	void draw_replay_photo(const Replay& replay, Draw pic) const;
	void draw_replay_buttons() const;

	void draw_standings(const Replay& replay) const;
};

}; // namespace srb2::menus::egg_tv

#endif // __EGGTV_HPP__
