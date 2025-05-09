// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __EGGTVDATA_HPP__
#define __EGGTVDATA_HPP__

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "../../core/string.h"
#include "../../core/json.hpp"
#include "../../cxxutil.hpp"

#include "../../d_main.h" // srb2home
#include "../../doomstat.h" // gametype_t
#include "../../doomtype.h"

namespace srb2::menus::egg_tv
{

class EggTVData
{
private:
	const std::filesystem::path root_ = std::filesystem::path{srb2home} / "media/replay/online";
	const std::filesystem::path favoritesPath_ = root_ / "favorites.json";

	JsonValue favoritesFile_ = cache_favorites();
	JsonValue& favorites_;

	JsonValue cache_favorites() const;

	void cache_folders();
	void save_favorites() const;

public:
	using time_point_t = std::chrono::time_point<std::chrono::system_clock>;

	explicit EggTVData();

	class Replay;

	class Folder
	{
	public:
		class Cache
		{
		public:
			class ReplayRef
			{
			public:
				explicit ReplayRef(Cache& cache, std::filesystem::path filename, time_point_t time) :
					cache_(&cache), filename_(filename), time_(time)
				{
				}

				Cache& cache() const { return *cache_; }
				const std::filesystem::path& filename() const { return filename_; }
				const time_point_t& time() const { return time_; }

				std::shared_ptr<Replay> replay()
				{
					SRB2_ASSERT(!released_); // do not call after released

					if (!replay_)
					{
						replay_ = std::make_shared<Replay>(*this);
					}

					return replay_;
				}

				void release()
				{
					replay_.reset();
					released_ = true;
				}

				bool favorited() const { return iterator_to_favorite() != favorites().as_array().end(); }
				JsonValue& favorites() const { return cache().folder().tv().favorites_; }

				srb2::String favorites_path() const
				{
					// path::generic_string converts to forward
					// slashes on Windows. This should suffice to make
					// the JSON file portable across installations.
					return (std::filesystem::path{std::string_view(cache().folder().name())} / filename()).generic_string();
				}

				JsonArray::const_iterator iterator_to_favorite() const
				{
					srb2::String path = favorites_path();
					return std::find(favorites().as_array().begin(), favorites().as_array().end(), static_cast<std::string_view>(path));
				}

			private:
				Cache* cache_;
				std::filesystem::path filename_;
				time_point_t time_;
				std::shared_ptr<Replay> replay_;
				bool released_ = false;
			};

			explicit Cache(Folder& folder);

			std::shared_ptr<Replay> replay(std::size_t idx);
			void release(const ReplayRef& ref);

			std::size_t size() const { return folder_.size(); }
			Folder& folder() const { return folder_; }

		private:
			Folder& folder_;
			std::vector<ReplayRef> replays_;
		};

		explicit Folder(EggTVData& tv, const std::filesystem::directory_entry& entry);

		int x = 0;
		int y = 0;

		bool empty() { return size() == 0; }
		std::filesystem::path path() const { return tv_->root_ / std::string_view(name_); }

		EggTVData& tv() const { return *tv_; }

		std::size_t size() const { return size_; }
		const time_point_t& time() const { return time_; }
		const srb2::String& name() const { return name_; }

		std::unique_ptr<Cache> load() { return std::make_unique<Cache>(*this); };

		bool operator ==(const Folder& b) const { return this == &b; }

	private:
		std::size_t size_;
		time_point_t time_;
		EggTVData* tv_;
		srb2::String name_;
	};

	class Replay
	{
	public:
		class Title
		{
		public:
			explicit Title() {}
			explicit Title(const std::string_view& first, const std::string_view& second) :
				first_(first), second_(second)
			{
			}

			const srb2::String& first() const { return first_; }
			const srb2::String& second() const { return second_; }

			operator const srb2::String() const;

		private:
			srb2::String first_, second_;
		};

		struct Standing
		{
			srb2::String name;
			std::optional<std::size_t> skin;
			std::size_t color;
			std::optional<tic_t> time;
			std::optional<UINT32> score;
		};

		class Gametype
		{
		public:
			struct Race
			{
				int laps;
				std::string_view speed;
				bool encore;
			};

			explicit Gametype() {}
			explicit Gametype(INT16 gt, bool gp) : gametype_(get(gt)), name_(get_name(gt, gp)) {}
			explicit Gametype(INT16 gt, bool gp, Race race) : Gametype(gt, gp) { var_ = race; }

			bool valid() const { return gametype_; }

			std::string_view name() const { return name_; }
			UINT32 rules() const { return valid() ? gametype_->rules : 0u; }

			bool ranks_time() const { return !ranks_points(); }
			bool ranks_points() const { return rules() & GTR_POINTLIMIT; }

			const Race* race() const { return std::get_if<Race>(&var_); }

		private:
			const gametype_t* gametype_ = nullptr;
			std::string_view name_;
			std::variant<std::monostate, Race> var_;

			std::string_view get_name(INT16 gt, bool gp) const
			{
				if (!valid())
				{
					return "<Unknown gametype>";
				}

				if (gt == GT_SPECIAL)
				{
					return "Sealed Star";
				}

				if ((rules() & GTR_PRISONS) && gp)
				{
					return "Prison Break";
				}

				return gametype_->name;
			}

			static gametype_t* get(INT16 gt) { return gt >= 0 && gt < numgametypes ? gametypes[gt] : nullptr; }
		};

		explicit Replay(Folder::Cache::ReplayRef& ref);
		~Replay();

		void mark_for_deletion()
		{
			erased_ = true;
			ref_->release();
		}

		void toggle_favorite() const;

		bool invalid() const { return invalid_; }
		bool favorited() const { return ref_->iterator_to_favorite() != ref_->favorites().as_array().end(); }

		std::filesystem::path path() const { return ref_->cache().folder().path() / ref_->filename(); }
		const time_point_t& date() const { return ref_->time(); }

		std::size_t map() const { return map_; }
		const Title& title() const { return title_; }
		const Gametype& gametype() const { return gametype_; }

		const std::vector<Standing>& standings() const { return standings_; }
		const Standing* winner() const { return standings_.empty() ? nullptr : &standings_.front(); }

	private:
		Folder::Cache::ReplayRef* ref_;

		bool invalid_ = true;
		bool erased_ = false;

		std::vector<Standing> standings_;
		std::size_t map_;
		Title title_;
		Gametype gametype_;
	};

	enum class FolderSort
	{
		kDate,
		kName,
		kSize,
	};

	std::vector<Folder> folders_;
	std::unique_ptr<Folder::Cache> cache_;
	FolderSort folderSort_ = FolderSort::kDate;

	void sort_folders();
};

}; // namsepace srb2::menus::egg_tv

#endif // __EGGTVDATA_HPP__
