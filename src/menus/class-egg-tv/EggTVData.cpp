// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by James Robert Roman
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <fmt/std.h> // std::filesystem::path formatter
#include <nlohmann/json.hpp>

#include "../../cxxutil.hpp"
#include "EggTVData.hpp"

#include "../../doomdef.h" // CONS_Alert
#include "../../doomstat.h" // gametypes
#include "../../g_demo.h"
#include "../../k_menu.h" // DF_ENCORE
#include "../../r_skins.h"

using namespace srb2::menus::egg_tv;

namespace fs = std::filesystem;

using nlohmann::json;

template <>
struct fmt::formatter<fs::filesystem_error> : formatter<std::string>
{
	template <typename FormatContext>
	auto format(const fs::filesystem_error& ex, FormatContext& ctx) const
	{
		return formatter<std::string>::format(
			fmt::format("{}, path1={}, path2={}", ex.what(), ex.path1(), ex.path2()),
			ctx
		);
	}
};

namespace
{

template <class... Args>
void print_error(fmt::format_string<Args...> format, Args&&... args)
{
	CONS_Alert(CONS_ERROR, "Egg TV: %s\n", fmt::format(format, args...).c_str());
}

template <class To, class From>
To time_point_conv(From time)
{
	// https://stackoverflow.com/a/58237530/10850779
	return std::chrono::time_point_cast<typename To::duration>(To::clock::now() + (time - From::clock::now()));
}

json& ensure_array(json& object, const char* key)
{
	json& array = object[key];

	if (!array.is_array())
	{
		array = json::array();
	}

	return array;
}

}; // namespace

EggTVData::EggTVData() : favorites_(ensure_array(favoritesFile_, "favorites"))
{
	try
	{
		cache_folders();
	}
	catch (const fs::filesystem_error& ex)
	{
		print_error("{}", ex);
	}
}

json EggTVData::cache_favorites() const
{
	json object;

	try
	{
		std::ifstream f(favoritesPath_);

		if (f.is_open())
		{
			f >> object;
		}
	}
	catch (const std::exception& ex)
	{
		print_error("{}", ex.what());
	}

	return object;
}

EggTVData::Folder::Cache::Cache(Folder& folder) : folder_(folder)
{
	try
	{
		for (const fs::directory_entry& entry : fs::directory_iterator(folder_.path()))
		{
			try
			{
				if (!entry.is_regular_file())
				{
					continue;
				}

				replays_.emplace_back(
					*this,
					entry.path().filename(),
					time_point_conv<time_point_t>(entry.last_write_time())
				);
			}
			catch (const fs::filesystem_error& ex)
			{
				print_error("{}", ex);
			}
		}
	}
	catch (const fs::filesystem_error& ex)
	{
		print_error("{}", ex);
	}

	auto predicate = [](const ReplayRef& a, const ReplayRef& b)
	{
		// Favorites come first
		if (a.favorited() != b.favorited())
		{
			return a.favorited();
		}

		return a.time() > b.time(); // sort newest to oldest
	};

	std::sort(replays_.begin(), replays_.end(), predicate);

	// Refresh folder size
	folder_.size_ = replays_.size();
}

std::shared_ptr<EggTVData::Replay> EggTVData::Folder::Cache::replay(std::size_t idx)
{
	if (idx >= size())
	{
		return {};
	}

	return replays_[idx].replay();
}

void EggTVData::Folder::Cache::release(const ReplayRef& ref)
{
	const auto& it = std::find_if(replays_.begin(), replays_.end(), [&ref](const ReplayRef& b) { return &b == &ref; });

	SRB2_ASSERT(it != replays_.end());

	replays_.erase(it);
	folder_.size_--;
}

EggTVData::Folder::Folder(EggTVData& tv, const fs::directory_entry& entry) :
	tv_(&tv),
	name_(entry.path().filename().string())
{
	SRB2_ASSERT(entry.path().parent_path() == tv_->root_);

	time_ = time_point_t::min();
	size_ = 0;

	for (const fs::directory_entry& entry : fs::directory_iterator(entry.path()))
	{
		const time_point_t t = time_point_conv<time_point_t>(entry.last_write_time());

		if (time_ < t)
			time_ = t;

		size_++;
	}
}

EggTVData::Replay::Title::operator const std::string() const
{
	return second().empty() ? first() : fmt::format("{} - {}", first(), second());
}

EggTVData::Replay::Replay(Folder::Cache::ReplayRef& ref) : ref_(&ref)
{
	const fs::path path = this->path();

	menudemo_t info = {};

	if (path.native().size() >= sizeof info.filepath)
	{
		return;
	}

	std::copy_n(path.string().c_str(), path.native().size() + 1, info.filepath);

	G_LoadDemoInfo(&info);

	if (info.type != MD_LOADED)
	{
		return;
	}

	{
		constexpr std::string_view kDelimiter = " - ";

		const std::string_view str = info.title;
		const std::size_t mid = str.find(kDelimiter);

		title_ = Title(str.substr(0, mid), mid == std::string::npos ? "" : str.substr(mid + kDelimiter.size()));
		//title_ = Title("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW", "WWWWWWWWWWWWWWWWWWWWWWWWWWW");
	}

	map_ = info.map;

	if (info.gametype == GT_RACE)
	{
		gametype_ = Gametype(GT_RACE, info.gp, Gametype::Race {
			info.numlaps,
			kartspeed_cons_t[(info.kartspeed & ~(DF_ENCORE)) + 1].strvalue,
			(info.kartspeed & DF_ENCORE) != 0,
		});
	}
	else
	{
		gametype_ = Gametype(info.gametype, info.gp);
	}

	for (const auto& data : info.standings)
	{
		if (!data.ranking)
		{
			break;
		}

		Standing& standing = standings_.emplace_back();

		standing.name = data.name;

		if (data.skin < numskins)
		{
			standing.skin = data.skin;
		}

		standing.color = data.color;

		if (data.timeorscore != UINT32_MAX - 1) // NO CONTEST
		{
			if (gametype_.ranks_time())
			{
				standing.time = data.timeorscore;
			}
			else if (gametype_.ranks_points())
			{
				standing.score = data.timeorscore;
			}
		}
	}

	invalid_ = false;
}

EggTVData::Replay::~Replay()
{
	if (!erased_)
	{
		return;
	}

	// Delayed erase function ensures there are no references
	// left before ReplayRef is removed from cache.

	try
	{
		fs::remove(path());
	}
	catch (const fs::filesystem_error& ex)
	{
		// catch inside loop so individual errors don't bail completely
		print_error("{}", ex);
	}

	// Clear deleted replays from favorites too!
	if (favorited())
	{
		toggle_favorite();
	}

	ref_->cache().release(*ref_);
}

void EggTVData::Replay::toggle_favorite() const
{
	const auto& it = ref_->iterator_to_favorite();

	if (it != ref_->favorites().end())
	{
		ref_->favorites().erase(it);
	}
	else
	{
		ref_->favorites().emplace_back(ref_->favorites_path());
	}

	ref_->cache().folder().tv().save_favorites();
}

void EggTVData::cache_folders()
{
	for (const fs::directory_entry& entry : fs::directory_iterator(root_))
	{
		try
		{
			if (!entry.is_directory())
			{
				continue;
			}

			Folder folder(*this, entry);

			if (!folder.empty())
			{
				folders_.push_back(folder);
			}
		}
		catch (const fs::filesystem_error& ex)
		{
			// catch inside loop so individual errors don't bail completely
			print_error("{}", ex);
		}
	}

	sort_folders();
}

void EggTVData::sort_folders()
{
	auto predicate = [this](const Folder& a, const Folder& b)
	{
		switch (folderSort_)
		{
		case FolderSort::kDate:
			return a.time() > b.time();

		case FolderSort::kName:
			return a.name() < b.name(); // ascending order

		case FolderSort::kSize:
			return a.size() > b.size();
		}

		return false;
	};

	std::sort(folders_.begin(), folders_.end(), predicate);
}

void EggTVData::save_favorites() const
{
	try
	{
		std::ofstream(favoritesPath_) << favoritesFile_;
	}
	catch (const std::exception& ex)
	{
		print_error("{}", ex.what());
	}
}
