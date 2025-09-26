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
#include <cstddef>
#include <memory>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include "../../core/string.h"
#include "../../cxxutil.hpp"
#include "../../v_draw.hpp"
#include "EggTV.hpp"

#include "../../doomstat.h" // gametic
//#include "../../doomtype.h" // intsign
#include "../../g_demo.h"
#include "../../g_game.h" // G_TicsToSeconds
#include "../../k_menu.h"
#include "../../m_easing.h"
#include "../../m_fixed.h"
#include "../../sounds.h"
#include "../../st_stuff.h" // faceprefix
#include "../../v_video.h"

using namespace srb2::menus::egg_tv;

using srb2::Draw;

const EggTVGraphics::PatchManager EggTVGraphics::patches_;
const EggTVGraphics::ColorManager EggTVGraphics::colors_;

namespace
{

struct FolderOffsets
{
	static constexpr int kTop = 51;
	static constexpr int kBottom = 184;
	static constexpr int kRowHeight = 15;
	static constexpr int kRowsPerPage = (kBottom - kTop) / kRowHeight;
};

struct StandingsOffsets
{
	static constexpr int kLeft = 184;
	static constexpr int kRight = 298;
	static constexpr int kTop = 56;
	static constexpr int kBottom = 188;
	static constexpr int kRowHeight = 19;
	static constexpr int kRowsPerPage = (kBottom - kTop) / kRowHeight;
};

void draw_face(const Draw& draw, const EggTVData::Replay::Standing& player, facepatches face)
{
	const skincolornum_t color = static_cast<skincolornum_t>(player.color);

	if (player.skin)
	{
		draw.colormap(*player.skin, color).patch(faceprefix[*player.skin][face]);
	}
	else
	{
		switch (face)
		{
		case FACE_RANK:
			draw.colorize(color).patch("M_NORANK");
			break;

		case FACE_WANTED:
			draw.colorize(color).patch("M_NOWANT");
			break;

		default:
			break;
		}
	}
}

srb2::String player_time_string(const EggTVData::Replay::Standing& player)
{
	if (player.time)
	{
		return srb2::format(
			R"({}'{}"{})",
			G_TicsToMinutes(*player.time, true),
			G_TicsToSeconds(*player.time),
			G_TicsToCentiseconds(*player.time)
		);
	}
	else
	{
		return "NO CONTEST";
	}
}

srb2::String player_points_string(const EggTVData::Replay::Standing& player)
{
	return player.score ? fmt::format("{} PTS", *player.score) : "NO CONTEST";
}

}; // namespace

struct EggTV::GridOffsets
{
	static constexpr int
		kTop = 47,
		kBottom = 174,
		kRight = 304,
		kLeft = 15,

		kGridHeight = 148,
		kRowHeight = 37,
		kCellWidth = 58,

		kRowsPerPage = kGridHeight / kRowHeight;

	Draw draw;
	Draw select;

	int row;

	explicit GridOffsets(const EggTV& tv)
	{
		// Center the second row by pushing it down
		constexpr int kCenter = kTop + (((kBottom - kTop) - (kGridHeight - kRowHeight)) / 2);

		constexpr int kFlushBottom = kBottom - kGridHeight;

		const Cursor& p = tv.gridRow_;

		const int padTop = 1;
		const int padBottom = p.end() - 3;

		const bool multiPage = padTop < padBottom;

		float y;

		if (p.pos() <= padTop)
		{
			y = kTop;
		}
		else if (multiPage && p.pos() <= padBottom)
		{
			y = kCenter;
		}
		else
		{
			y = kFlushBottom;
		}

		const int high = p.change() > 0 ? p.pos() - p.change() : p.pos();

		if (high >= padTop && high <= padBottom)
		{
			// Single page is a very special case. Scrolling
			// by a single row is too tall. Make sure not to
			// scroll past the end of the page!
			const int gap = multiPage ? kRowHeight : (kTop - kFlushBottom);

			y += Easing_Linear(tv.rowSlide_.reverse(), 0, gap * p.change());
		}

		row = std::max(0, std::min(p.pos(), padBottom) - padTop);

		// Y may be offset downward when scrolling or due to
		// centering. In this case, draw an extra row above so
		// it doesn't leave any empty space.
		if (y > kTop && multiPage)
		{
			SRB2_ASSERT(row > 0);
			row--;

			y -= kRowHeight;
		}

		draw = Draw(kLeft, y);
		select = draw.xy(tv.gridCol_.pos() * kCellWidth, (tv.gridRow_.pos() - row) * kRowHeight);
	}
};

template <int Size>
EggTVGraphics::PatchManager::patch EggTVGraphics::PatchManager::Animation<Size>::animate(int interval) const
{
	return data[(gametic / interval) % data.size()];
}

EggTV::InputReaction EggTV::input(int pid)
{
	if (mode_.changing())
	{
		return {};
	}

	InputReaction reaction;

	// Call this to make a sound effect! :D
	auto react = [&reaction](bool is = true) { reaction.effect |= is; };

	const menucmd_t& cmd = menucmd[pid];

	switch (mode_)
	{
	case Mode::kFolders:
		folderRow_ += cmd.dpad_ud;

		react(cmd.dpad_ud != 0);
		break;

	case Mode::kGrid:
		gridRow_ += cmd.dpad_ud;
		gridCol_ += cmd.dpad_lr;

		if (cmd.dpad_ud != 0 || cmd.dpad_lr != 0)
		{
			dashTextScroll_.start(24);
			react();
		}
		break;

	case Mode::kReplay:
		if (cmd.dpad_ud != 0)
		{
			buttonHover_.start();
		}

		// Use default menu handler for these options (see EXTRAS_EggTV)
		reaction.bypass = true;
		break;

	case Mode::kStandings:
		standingsRow_ += cmd.dpad_ud;

		// If scroll wraps around, do not smooth scroll. That looks very weird.
		if (standingsRow_.wrap())
		{
			standingsRowSlide_.stop();
		}

		react(cmd.dpad_ud != 0);
		break;
	}

	if (M_MenuConfirmPressed(pid))
	{
		if (select())
		{
			if (mode_ == Mode::kGrid)
			{
				reaction.sound = sfx_s3k63;
			}
		}
		else
		{
			if (mode_ == Mode::kGrid)
			{
				gridSelectShake_.start();
			}

			reaction.sound = sfx_s3kb2;
		}

		react();
	}
	else if (M_MenuBackPressed(pid))
	{
		if (mode_ == Mode::kReplay)
		{
			// ...Except for backing out of the menu.
			// Don't exit Egg TV entirely.
			reaction.bypass = false;
		}

		back();
	}
	else if (M_MenuButtonPressed(pid, MBT_R))
	{
		if (mode_ == Mode::kFolders)
		{
			switch (folderSort_)
			{
			case FolderSort::kDate:
				folderSort_ = FolderSort::kName;
				break;

			case FolderSort::kName:
				folderSort_ = FolderSort::kSize;
				break;

			case FolderSort::kSize:
				folderSort_ = FolderSort::kDate;
				break;
			}

			sort_folders();
		}
	}

	return reaction;
}

bool EggTV::select()
{
	switch (mode_)
	{
		case Mode::kFolders:
			if (folders_.empty())
			{
				break;
			}

			{
				Folder& folder = folders_[folderRow_.pos()];

				cache_ = folder.load();

				// Restore previous selection.
				gridCol_ = folder.x;
				gridRow_ = folder.y;
			}

			folderSlide_.start();
			dashRise_.start_with(folderSlide_ + 6);
			gridFade_.start_after(folderSlide_);
			gridPopulate_.start_with(gridFade_);
			dashTextRise_.end_with(gridPopulate_ + 2);
			dashTextScroll_.start_after(dashTextRise_ + 4);

			mode_.change(Mode::kGrid, folderSlide_.stopping_point());

			return true;

		case Mode::kGrid:
			if (grid_index() >= cache_->size())
			{
				break;
			}

			if (cache_->replay(grid_index())->invalid())
			{
				break;
			}

			M_EggTV_RefreshButtonLabels();

			dashTextRise_.start();
			enhanceMove_.start_after(dashTextRise_);
			enhanceMove_.extend(gridCol_.pos() * 0.75); // extend time with distance
			enhanceZoom_.start_after(enhanceMove_);
			replaySlide_.start_after(enhanceZoom_);
			buttonSlide_.start_with(replaySlide_);
			replayTitleScroll_.start_after(replaySlide_);
			gridPopulate_.end_with(enhanceZoom_);
			gridFade_.end_with(gridPopulate_);

			mode_.change(Mode::kReplay, enhanceZoom_.stopping_point());
			itemOn = 0; // WATCH REPLAY

			return true;

		case Mode::kReplay:
			break;

		case Mode::kStandings:
			back();
			return true;
	}

	return false;
}

void EggTV::back()
{
	switch (mode_)
	{
		case Mode::kFolders:
			M_GoBack(0);
			break;

		case Mode::kGrid:
			{
				Folder& folder = cache_->folder();

				// Save current selection.
				folder.x = gridCol_.pos();
				folder.y = gridRow_.pos();
			}

			dashTextRise_.start();
			gridPopulate_.start();
			gridPopulate_.extend(-0.5);
			gridFade_.end_with(gridPopulate_);
			dashRise_.start_after(gridFade_);
			folderSlide_.start_with(dashRise_);

			mode_.change(Mode::kFolders, folderSlide_.stopping_point());
			break;

		case Mode::kReplay:
			replaySlide_.start();
			buttonSlide_.start();
			replaySlide_.extend(-0.5);
			buttonSlide_.extend(-0.5);
			gridPopulate_.start_after(replaySlide_);
			gridPopulate_.extend(-0.75);
			gridFade_.start_with(gridPopulate_);
			enhanceZoom_.start_with(gridPopulate_);
			enhanceMove_.end_with(gridPopulate_);
			dashTextRise_.end_with(gridPopulate_);
			dashTextScroll_.start_after(dashTextRise_ + 4);

			mode_.change(Mode::kGrid, gridPopulate_.stopping_point());
			break;

		case Mode::kStandings:
			buttonSlide_.start();
			buttonSlide_.extend(-0.125);
			mode_.change(Mode::kReplay, buttonSlide_.stopping_point());
			break;
	}
}

void EggTV::watch() const
{
	SRB2_ASSERT(cache_.get());

	std::shared_ptr<const Replay> replay = cache_->replay(grid_index());

	if (replay)
	{
		restoreMenu = currentMenu;

		M_ClearMenusNoTitle(false);

		demo.loadfiles = true;
		demo.ignorefiles = false;

		G_DoPlayDemo(replay->path().string().c_str());
	}
}

void EggTV::erase()
{
	SRB2_ASSERT(cache_.get());

	{
		std::shared_ptr<Replay> replay = cache_->replay(grid_index());

		if (replay)
		{
			// Will not be deleted until shared_ptr is released
			replay->mark_for_deletion();
		}
	}

	cache_ = cache_->folder().load();

	if (cache_->folder().empty())
	{
		// Remove empty folder from list
		folders_.erase(std::find(folders_.begin(), folders_.end(), cache_->folder()));
		folderRow_ += 0; // clamp cursor

		cache_.reset();

		mode_.change(Mode::kFolders, 0);
	}
	else
	{
		mode_.change(Mode::kGrid, 0);
	}
}

void EggTV::toggle_favorite()
{
	SRB2_ASSERT(cache_.get());

	std::shared_ptr<const Replay> replay = cache_->replay(grid_index());

	if (replay)
	{
		replay->toggle_favorite();

		M_EggTV_RefreshButtonLabels();

		favSlap_.start();
	}
}

bool EggTV::favorited() const
{
	if (!cache_)
	{
		return false;
	}

	std::shared_ptr<const Replay> replay = cache_->replay(grid_index());

	return replay && replay->favorited();
}

void EggTV::standings()
{
	standingsRow_ = 0;

	buttonSlide_.start();

	mode_.change(Mode::kStandings, buttonSlide_.stopping_point());
}

std::size_t EggTV::grid_rows() const
{
	return std::max(static_cast<std::size_t>(GridOffsets::kRowsPerPage), cache_->size() / kGridColsPerRow);
}

std::size_t EggTV::standings_rows() const
{
	constexpr int kPerPage = StandingsOffsets::kRowsPerPage;

	return (cache_->replay(grid_index())->standings().size() + (kPerPage - 1)) / kPerPage;
}

EggTVGraphics::PatchManager::patch EggTV::gametype_graphic(const Replay& replay) const
{
	const auto& it = patches_.gametype.find(replay.gametype().name());

	return it != patches_.gametype.end() ? it->second : nullptr;
}

void EggTV::draw() const
{
	draw_background();

	switch (mode_)
	{
	case Mode::kFolders:
		draw_folders();

		if (dashRise_.running())
		{
			draw_dash();
		}
		break;

	case Mode::kGrid:
		if (gridPopulate_.running())
		{
			draw_grid<GridMode::kPopulating>();
		}
		else if (mode_.changing_to(Mode::kFolders))
		{
			draw_folders();
		}
		else
		{
			draw_grid<GridMode::kFinal>();
		}

		draw_dash();
		break;

	case Mode::kReplay:
	case Mode::kStandings:
		if (mode_.changing_to(Mode::kGrid) && !replaySlide_.running())
		{
			draw_grid<GridMode::kPopulating>();
			draw_dash();
		}
		else
		{
			std::shared_ptr<const Replay> replay = cache_->replay(grid_index());
			SRB2_ASSERT(replay);
			draw_replay(*replay);
		}
		break;
	}

	draw_overlay();

	if (mode_ == Mode::kFolders)
	{
		draw_folder_header();
	}

	if (mode_ != Mode::kReplay && !gridPopulate_.running() && !folderSlide_.running() && !buttonSlide_.running())
	{
		draw_scroll_bar();
	}
}

void EggTV::draw_background() const
{
	Draw(0, 0).patch(patches_.bg.animate(2));
}

void EggTV::draw_overlay() const
{
	Draw(0, 0).flags(V_MODULATE).patch(patches_.mod);
	Draw(0, 0).patch(patches_.overlay);

	Draw(160, 3)
		.font(Draw::Font::kGamemode)
		.align(Draw::Align::kCenter)
		.text("Egg TV");
}

void EggTV::draw_folder_header() const
{
	const Draw header = Draw(0, 39).flags(V_YELLOWMAP).font(Draw::Font::kThin);

	header.x(32).text("GAME VERSION:");
	header.x(198).text("REPLAYS:");
	header.x(245).text("LAST PLAYED:");
}

void EggTV::draw_folders() const
{
	constexpr int kPadding = 4;

	const float x = Easing_InExpo(folderSlide_.reverse_if(mode_.changing_to(Mode::kFolders)), 0, -160);

	Draw row(x, FolderOffsets::kTop);

	int start = std::max(0, folderRow_.pos() - kPadding);
	const int stop = folders_.size();

	for (int i = start; i < stop; ++i)
	{
		const Folder& folder = folders_[i];

		const int mode = (i == folderRow_.pos() ? 1 : 0);

		row.x(26).flags(V_TRANSLUCENT).colormap(colors_.bar[mode]).patch(patches_.bar);
		row.x(32).colormap(colors_.folder[mode]).patch(patches_.folder[mode]);

		const Draw text = row.y(2).flags(mode ? V_YELLOWMAP : 0);

		text.x(52).clipx(0, 186).text(folder.name());
		text.x(228).align(Draw::Align::kRight).text("{}", folder.size());
		text.x(244).text("{:%d %b %Y}", folder.time());

		row = row.y(FolderOffsets::kRowHeight);

		// went below the viewport
		if (row.y() >= FolderOffsets::kBottom)
		{
			break;
		}
	}
}

void EggTV::draw_scroll_bar() const
{
	Draw bar = Draw(311, 0).colormap(colors_.scroll);

	bar.y(41).patch(patches_.scroll.arrow.up);
	bar.y(191).patch(patches_.scroll.arrow.down);

	constexpr int kTop = 48;
	constexpr int kBottom = 190;
	constexpr int kHeight = (kBottom - kTop);
	constexpr int kBeadHeight = 3;
	constexpr int kMaxMid = (kHeight - (2 * kBeadHeight));

	float thisPage = 0.f;
	float pages = 1.f;

	auto curse = [&](const Cursor& p, float perPage)
	{
		thisPage = p.pos() / std::max(1.f, p.end() - 1.f);
		pages = std::max(1.f, p.end() / perPage);
	};

	switch (mode_)
	{
	case Mode::kFolders:
		curse(folderRow_, FolderOffsets::kRowsPerPage);
		break;

	case Mode::kGrid:
		curse(gridRow_, GridOffsets::kRowsPerPage);
		break;

	case Mode::kStandings:
		curse(standingsRow_, 1);
		break;

	default:
		break;
	}

	SRB2_ASSERT(pages >= 1.f);

	const float mid = std::max(static_cast<int>(kMaxMid / pages), kBeadHeight);
	const float y = thisPage * (kMaxMid - mid);

	bar = bar.y(kTop + y);

	bar.patch(patches_.scroll.bead.top);
	bar = bar.y(kBeadHeight);
	bar.height(mid).stretch(Draw::Stretch::kHeight).patch(patches_.scroll.bead.mid);
	bar.y(mid).patch(patches_.scroll.bead.bottom);
}

void EggTV::draw_dash() const
{
	const float y = Easing_Linear(dashRise_.reverse().invert_if(mode_.changing_to(Mode::kFolders)), 174, 188);
	Draw(15, y).patch(patches_.dash);

	if (!mode_.changing_to(Mode::kReplay) || dashTextRise_.running())
	{
		std::shared_ptr<const Replay> replay = cache_->replay(grid_index());

		if (replay)
		{
			draw_dash_text(*replay);
		}
	}
}

void EggTV::draw_dash_text(const Replay& replay) const
{
	const Draw::TextElement text = Draw::TextElement(replay.title()).font(Draw::Font::kThin);

	const int halfWidth = text.width() / 2;

	const fixed_t t = (dashTextScroll_.variable() + (FRACUNIT/2)) % FRACUNIT;
	const float x = Easing_Linear(t, GridOffsets::kRight + halfWidth, GridOffsets::kLeft - halfWidth);
	const float y = Easing_Linear(dashTextRise_.reverse().invert_if(mode_.next() != Mode::kGrid), 177, 188);

	Draw(x, y).align(Draw::Align::kCenter).text(text);
}

template <EggTV::GridMode K>
void EggTV::draw_grid() const
{
	const GridOffsets grid(*this);

	const std::size_t firstIdx = grid.row * kGridColsPerRow;

	Draw row = grid.draw;
	std::size_t idx = firstIdx;

	auto draw_cell_populating = [&](Draw cell, const Replay* replay)
	{
		if (!replay || (mode_.changing_to(Mode::kReplay) && idx == grid_index()))
		{
			cell.flags(V_MODULATE).patch(patches_.empty);
			return;
		}

		if (replay->invalid())
		{
			cell.flags(V_MODULATE).colorize(SKINCOLOR_RED).patch(patches_.empty);
			return;
		}

		cell.patch(patches_.tv.animate(2));
	};

	auto draw_cell_background = [&](Draw cell, const Replay* replay)
	{
		const int mode = idx == grid_index() ? 1 : 0;

		if (!replay)
		{
			cell.flags(V_MODULATE).patch(patches_.nodata[mode].animate(2));
			return;
		}

		if (replay->invalid())
		{
			cell.flags(V_MODULATE).patch(patches_.corrupt[mode].animate(2));
			return;
		}

		cell.width(GridOffsets::kCellWidth).thumbnail(replay->map());
	};

	auto draw_cell_foreground = [&](Draw cell, const Replay* replay)
	{
		if (!replay || replay->invalid())
		{
			return;
		}

		if (replay->winner())
		{
			draw_face(cell.xy(1, 20), *replay->winner(), FACE_RANK);
		}

		PatchManager::patch gt = gametype_graphic(*replay);

		if (gt)
		{
			cell.xy(40, 1).patch(gt);
		}

		cell
			.xy(GridOffsets::kCellWidth - 3, 25)
			.align(Draw::Align::kRight)
			.font(Draw::Font::kThin)
			.text("{:%d %b %y}", replay->date());
	};

	auto draw_cell_fav = [&](Draw cell, const Replay* replay)
	{
		if (replay && replay->favorited())
		{
			cell.xy(1, 1).patch(patches_.fav);
		}
	};

	auto draw_row = [&](const int rightEdge, auto draw_cell)
	{
		Draw cell = row;

		while (cell.x() < rightEdge)
		{
			std::shared_ptr<const Replay> replay = cache_->replay(idx);

			draw_cell(cell, replay.get());

			cell = cell.x(GridOffsets::kCellWidth);
			idx++;
		}

		row = row.y(GridOffsets::kRowHeight);
	};

	if constexpr (K == GridMode::kPopulating)
	{
		int cols = Easing_Linear(gridPopulate_.reverse().invert_if(mode_.next() != Mode::kGrid), 8, 0);

		while (cols > 0 && row.y() < GridOffsets::kBottom)
		{
			const int next = idx + kGridColsPerRow;

			draw_row(std::min(GridOffsets::kRight, static_cast<int>(row.x()) + (cols * 58)), draw_cell_populating);
			cols--;

			// During the animation, some cells are skipped
			// entirely, so always update the index to the
			// next row.
			idx = next;
		}

		draw_grid_mesh(grid);

		if (mode_ == Mode::kReplay || mode_.changing_to(Mode::kReplay))
		{
			draw_grid_enhance(grid);
		}
	}
	else if constexpr (K == GridMode::kFinal)
	{
		auto loop = [&](auto draw_cell)
		{
			row = grid.draw;
			idx = firstIdx;

			while (row.y() < GridOffsets::kBottom)
			{
				draw_row(GridOffsets::kRight, draw_cell);
			}
		};

		loop(draw_cell_background);

		draw_grid_mesh(grid);

		// Foreground elements must be drawn over mesh.
		loop(draw_cell_foreground);

		draw_grid_select(grid);

		// Fav star is drawn over select =)
		loop(draw_cell_fav);
	}
}

void EggTV::draw_grid_mesh(const GridOffsets& grid) const
{
	const fixed_t t = gridFade_.reverse_if(mode_.next() == Mode::kGrid);
	const INT32 transFlag = t < FRACUNIT/2 ? 0 : V_TRANSLUCENT;

	// FIXME, hwr2d transparency does not work for other blend modes yet
#if 0
	Draw mesh = grid.draw.flags(V_MODULATE | transFlag);
#else
	Draw mesh = grid.draw.flags(transFlag);
#endif

	while (mesh.y() < GridOffsets::kBottom)
	{
		mesh.patch(patches_.grid);
		mesh = mesh.y(GridOffsets::kGridHeight);
	}
}

void EggTV::draw_grid_select(const GridOffsets& grid) const
{
	const float x = Easing_Linear(gridSelectX_.reverse(), 0, GridOffsets::kCellWidth * gridCol_.change());
	const float y = Easing_Linear(gridSelectY_.reverse(), 0, GridOffsets::kRowHeight * gridRow_.change());

	// TODO, make this part of the Animation class...?
	const float shake = [this]
	{
		fixed_t t = gridSelectShake_.variable();

		if (t >= 3*FRACUNIT/4)
		{
			t = -(FRACUNIT) + t;
		}
		else if (t >= FRACUNIT/4)
		{
			t = (FRACUNIT/2) - t;
		}

		return Easing_Linear(t, 0, 6 * 4);
	}();

	const skincolornum_t color = colors_.select[gridSelectShake_.running() ? 1 : 0];

	grid.select.xy(-(x) + shake, -(y)).colorize(color).patch(patches_.select);
}

void EggTV::draw_grid_enhance(const GridOffsets& grid) const
{
	const bool invert = mode_.changing_to(Mode::kGrid);

	const fixed_t move = enhanceMove_.reverse().invert_if(invert);
	const fixed_t zoom = enhanceZoom_.reverse().invert_if(invert);

	const float x = Easing_Linear(move, 22, grid.select.x());
	const float y = Easing_Linear(move, 54, grid.select.y());

	const float width = Easing_Linear(zoom, 160, GridOffsets::kCellWidth);
	const float height = Easing_Linear(zoom, 100, GridOffsets::kRowHeight);

	draw_replay_photo(*cache_->replay(grid_index()), Draw(x, y).size(width, height));
}

void EggTV::draw_replay(const Replay& replay) const
{
	const Replay::Gametype::Race* race = replay.gametype().race();
	const Replay::Standing* winner = replay.winner();

	Draw pic = Draw(22, 54).size(160, 100);

	draw_replay_photo(replay, pic);

	if (mode_ == Mode::kReplay || buttonSlide_.running())
	{
		draw_replay_buttons();
	}

	if (mode_ == Mode::kStandings || mode_.changing_to(Mode::kStandings))
	{
		draw_standings(replay);
	}

	Draw box = pic.x(Easing_OutQuad(replaySlide_.reverse().invert_if(mode_.next() == Mode::kGrid), 0, -182));

	{
		Draw row = box.y(2).font(Draw::Font::kThin);

		row.x(1).patch(gametype_graphic(replay));

		if (race)
		{
			row.x(19).align(Draw::Align::kLeft).text("({} laps)", race->laps);
		}

		row.x(160 - 3).align(Draw::Align::kRight).text("{:%d %B %Y}", replay.date());
	}

	{
		Draw row = box.xy(39, 104).align(Draw::Align::kLeft);

		auto pair = [&row](int x, int y, auto label, auto text)
		{
			row = row.y(10);
			row.flags(V_AQUAMAP).font(Draw::Font::kThin).text(label);
			row.xy(x, y).font(Draw::Font::kMenu).text(text);
		};

		Draw gametype = row.font(Draw::Font::kMenu);

		if (race)
		{
			gametype.text("Race ({})", race->speed);
		}
		else
		{
			gametype.text(replay.gametype().name());
		}

		if (winner)
		{
			pair(38, 1, "WINNER", winner->name);

			if (replay.gametype().ranks_time())
			{
				pair(32, 0, "TIME", player_time_string(*winner));
			}
			else if (replay.gametype().ranks_points())
			{
				pair(32, 0, "SCORE", player_points_string(*winner));
			}
		}
	}

	if (winner)
	{
		draw_face(box.xy(2, 101), *winner, FACE_WANTED);
	}

	{
		constexpr Draw::Font kFont = Draw::Font::kThin;

		constexpr int kFavWidth = 11;
		constexpr int kMargin = 1;

		constexpr int kLeft = 11 + kFavWidth + kMargin;
		constexpr int kRight = 160 - kMargin;
		constexpr int kInnerWidth = kRight - kLeft;

		const Draw::TextElement upper = Draw::TextElement(replay.title().first()).font(kFont);
		const Draw::TextElement lower = Draw::TextElement(replay.title().second()).font(kFont);

		const float upperWidth = upper.width();
		const float lowerWidth = lower.width();
		const float widest = std::max(upperWidth, lowerWidth);
		const float inside = std::min(widest, kInnerWidth + 0.f);

		constexpr fixed_t kPad = FRACUNIT/4;

		const Animation::Value val = replayTitleScroll_.variable();
		const fixed_t n = (val.invert_if(val > FRACUNIT/2) * 2) % (FRACUNIT + 1);
		const fixed_t t = FixedDiv(std::clamp(n, kPad, FRACUNIT - kPad) - kPad, kPad * 2);

		const float scroll = Easing_Linear(t, widest - kInnerWidth, 0) - widest;
		const float upperScroll = upperWidth > kInnerWidth ? scroll : -(inside);
		const float lowerScroll = lowerWidth > kInnerWidth ? scroll : -(inside);

		Draw inner = box.xy(kLeft, 82).width(kInnerWidth).clipx();
		Draw title = inner.x(kInnerWidth);

		title.x(upperScroll).text(upper);
		title.xy(lowerScroll, 8).flags(V_AQUAMAP).text(lower);

		if (replay.favorited())
		{
			const Animation::Value t = favSlap_.reverse();

			box
				.xy(kRight - (kFavWidth + kMargin + inside), 82)
				.scale(FixedToFloat(Easing_InBack(t, FRACUNIT, 4*FRACUNIT)))
				.patch(patches_.fav);
		}
	}
}

void EggTV::draw_replay_photo(const Replay& replay, Draw pic) const
{
	pic.xy(2, 2).fill(0x1F);
	pic.thumbnail(replay.map());
}

void EggTV::draw_replay_buttons() const
{
	const float x = Easing_OutQuad(buttonSlide_.reverse().invert_if(mode_.next() != Mode::kReplay), 192, 305);

	Draw row(x, 54);

	for (INT16 i = 0; i < currentMenu->numitems; ++i)
	{
		const menuitem_t& item = currentMenu->menuitems[i];

		if (item.status & IT_STRING)
		{
			SRB2_ASSERT(item.text != nullptr);

			const int mode = (i == itemOn ? 1 : 0);

			Draw button = row.x(mode ? Easing_InSine(buttonHover_.reverse(), -6, 0) : 0);

			if (!(item.status & IT_TRANSTEXT))
			{
				button.colormap(colors_.button[mode]).patch(patches_.button);
			}

			button.xy(13, 1).font(Draw::Font::kFreeplay).flags(mode ? V_YELLOWMAP : 0).text(item.text);
		}

		row = row.y(18);
	}
}

void EggTV::draw_standings(const Replay& replay) const
{
	constexpr int kWidth = StandingsOffsets::kRight - StandingsOffsets::kLeft;

	const float x = Easing_InQuad(buttonSlide_.reverse().invert_if(mode_.changing_to(Mode::kReplay)), 0, kWidth);
	const float y = Easing_Linear(
		standingsRowSlide_.reverse_if(standingsRow_.change() > 0),
		0,
		StandingsOffsets::kRowHeight
	);

	Draw row = Draw(StandingsOffsets::kLeft - x, StandingsOffsets::kTop + y)
		.clipx(StandingsOffsets::kLeft, StandingsOffsets::kRight).font(Draw::Font::kMenu).align(Draw::Align::kRight);

	std::size_t start = standingsRow_.pos();

	const bool overdraw = start > 0 && !(standingsRowSlide_.running() && standingsRow_.change() < 0);

	if (overdraw)
	{
		start--;
	}

	if (overdraw || standingsRowSlide_.running())
	{
		row = row.y(-(StandingsOffsets::kRowHeight));
	}

	for (std::size_t i = start; i < replay.standings().size(); ++i)
	{
		const Replay::Standing& player = replay.standings()[i];

		row.x(18).flags(V_AQUAMAP).text("{}", 1 + i);

		draw_face(row.x(kWidth - 16), player, FACE_RANK);

		{
			Draw text = row.font(Draw::Font::kThin).align(Draw::Align::kLeft);

			text.x(22).text(player.name);

			if (replay.gametype().ranks_time())
			{
				text.xy(26, 8).text(player_time_string(player));
			}
			else if (replay.gametype().ranks_points())
			{
				text.xy(26, 8).text(player_time_string(player));
			}
		}

		row = row.y(StandingsOffsets::kRowHeight);
	}
}
