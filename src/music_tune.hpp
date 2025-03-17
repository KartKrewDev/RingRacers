// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef MUSIC_TUNE_HPP
#define MUSIC_TUNE_HPP

#include <algorithm>
#include <cstdint>
#include <optional>

#include "core/string.h"
#include "doomdef.h"
#include "doomtype.h"
#include "k_boss.h"
#include "music_detail.hpp"

namespace srb2::music
{

class Tune
{
public:
	explicit Tune() {}

	srb2::String song; // looks up the lump

	// Higher priority tunes play first.
	int priority = 0;

	// Fade at the beginning or end of the tune.
	int fade_in = 0; // in milliseconds
	int fade_out = 0;

	// Fade time subtracts from the duration of the song?
	int fade_out_inclusive = true;

	// Fade in when a higher priority tune ends and this one
	// resumes.
	int resume_fade_in = 0;

	// Adjust volume based on level context.
	bool use_level_volume = false;

	// Sync this tune to game logic.
	bool sync = false;

	// This tune loops at the end.
	bool loop = true;

	// When this tune runs out of duration, keep playing
	// silence.
	bool keep_open = false;

	// This tune does not respect mass stop or pause actions
	// from TuneManager::stop_all etc. It must be
	// stopped/paused individually.
	bool resist = false;
	bool resist_once = false; // set at runtime

	// This tune shows a credit when first played (not
	// resumed).
	bool credit = false;

	// This tune will vape in encoremode contexts.
	bool vapes = false;
	bool nightcoreable = false;

	// Start playing this number of tics into the tune.
	std::uint32_t seek = 0;

	// these track state
	bool can_fade_out = true;
	bool needs_seek = false;
	bool resume = false;
	bool ending = false;
	bool suspend = false;

	tic_t elapsed() const { return std::max(pause_.value_or(detail::tic_time()), begin_) - begin_; }
	tic_t time_remaining() const { return end_ - std::min(pause_.value_or(detail::tic_time()), end_); }
	tic_t duration() const { return end_ - begin_; }

	bool playing() const { return begin_ <= detail::tic_time() && (!ending || time_remaining()); }
	bool paused() const { return pause_.has_value(); }
	bool can_end() const { return end_ != INFTICS; }

	// Slow level music down a bit in Encore. (Values are vibe-based. WE GET IT YOU VAPE)
	const float encoremul = 0.86471f;

	float speed() const
	{
		// Apply to all tunes that support vape mode.
		if (encoremode && vapes)
		{
			if (nightcoreable && K_CheckBossIntro())
			{
				// In Versus, the vape makes you think you can start a nightcore YT channel
				return (1.f/encoremul);
			}

			if (!nightcoreable
			|| mapheaderinfo[gamemap-1]->encoremusname_size == 0)
			{
				// We only vape if the level doesn't have alternate tracks.
				return encoremul;
			}
		}

		return 1.f;
	}

	bool operator <(const Tune& b) const
	{
		// If this song is not playing, it has lowest
		// priority.
		if (!playing() || suspend)
		{
			return true;
		}

		// If the other song is not playing, we automatically
		// have higher priority.
		if (!b.playing() || b.suspend)
		{
			return false;
		}

		// The highest priority song is preferred.
		if (priority != b.priority)
		{
			return priority < b.priority;
		}

		// If both songs have the same priority, prefer the
		// one that begun later.
		return begin_ < b.begin_;
	}

	void play()
	{
		if (!needs_seek)
		{
			seek = 0;
		}

		can_fade_out = true;
		needs_seek = true;
		resume = false;
		ending = false;
		suspend = false;

		begin_ = detail::tic_time();
		end_ = INFTICS;
		pause_.reset();
	}

	void delay_end(tic_t duration)
	{
		end_ = detail::tic_time() + duration;

		if (!fade_out_inclusive)
		{
			end_ += detail::msec_to_tics(fade_out);
		}

		if (playing())
		{
			can_fade_out = true;
			ending = false;
		}

		suspend = false;
	}

	void stop()
	{
		begin_ = INFTICS;
		end_ = 0;
		pause_.reset();
	}

	void pause()
	{
		pause_ = detail::tic_time();
	}

	void unpause()
	{
		if (!pause_)
		{
			return;
		}

		if (playing())
		{
			tic_t n = detail::tic_time() - *pause_;

			begin_ += n;

			if (can_end())
			{
				end_ += n;
			}
		}

		pause_.reset();
	}

private:
	tic_t begin_ = INFTICS;
	tic_t end_ = 0;
	std::optional<tic_t> pause_;
};

}; // namespace srb2::music

#endif // MUSIC_TUNE_HPP
