// DR ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef MUSIC_MANAGER_HPP
#define MUSIC_MANAGER_HPP

#include <algorithm>
#include <cstdlib>
#include <string>
#include <unordered_map>

#include "cxxutil.hpp"
#include "music_tune.hpp"

namespace srb2::music
{

class TuneManager
{
public:
	const std::string& current_song() const { return current_song_; }

	Tune* current_tune() const
	{
		auto it = current_iterator();
		return it != map_.end() ? const_cast<Tune*>(&it->second) : nullptr;
	}

	const char* current_id() const
	{
		auto it = current_iterator();
		return it != map_.end() ? it->first.c_str() : "";
	}

	Tune* find(const char* id) const
	{
		auto it = map_.find(id);
		return it != map_.end() ? const_cast<Tune*>(&it->second) : nullptr;
	}

	Tune& insert(const char* id, const Tune* original = nullptr)
	{
		auto res = map_.emplace(id, original ? *original : Tune{});

		SRB2_ASSERT(res.second);

		return res.first->second;
	}

	void flip() { current_song_ = {}; }

	void tick();
	void pause_unpause() const;

	void stop(Tune& tune)
	{
		tune.stop();
		stop_credit_ = true;
	}

	void stop_all()
	{
		for_each([](Tune& tune) { tune.stop(); });
		stop_credit_ = true;
	}

	template <typename F>
	void for_each(F&& f)
	{
		for (auto& [_, tune] : map_)
		{
			if (tune.resist_once)
			{
				tune.resist_once = false;
				continue;
			}

			if (!tune.resist)
			{
				f(tune);
			}
		}
	}

private:
	std::unordered_map<std::string, Tune> map_;
	std::string current_song_;

	tic_t time_sync_;
	tic_t time_local_;

	bool stop_credit_ = false;

	bool gme_; // hack

	decltype(map_)::const_iterator current_iterator() const
	{
		return std::max_element(
			map_.begin(),
			map_.end(),
			[](const auto& a, const auto& b) { return a.second < b.second; }
		);
	}

	bool load() const;
	void adjust_volume() const;

	bool resync();

	static void seek(Tune* tune);
};

}; // namespace srb2::music

#endif // MUSIC_MANAGER_HPP
