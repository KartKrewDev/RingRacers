// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Kart Krew.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef WAD_PRIVATE_HPP
#define WAD_PRIVATE_HPP

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <optional>
#include <thread>

#include "cxxutil.hpp"

#include "doomtype.h"
#include "typedef.h"
#include "w_wad.h"

extern "C" void PrintMD5String(const UINT8 *md5, char *buf);

namespace srb2::wad
{

// Mutex for accessing wadfiles and numwadfiles from other
// threads. WARNING: this doesn't cover the lumpcache.
extern std::mutex g_wadfiles_mutex;

}; // namespace srb2::wad::detail

struct wadfile_private_t
{
	struct ChecksumState
	{
		explicit ChecksumState(wadfile_t* wad) : wad_(wad) {}

		const std::uint8_t* get() const
		{
			std::unique_lock lock(mutex_);
			cv_.wait(lock, [this]() -> bool { return valid_; });
			return md5sum_;
		}

		void fill(const std::uint8_t chk[16])
		{
			SRB2_ASSERT(!valid_ && !thread_.joinable());

			std::memcpy(md5sum_, chk, 16);
			valid_ = true;
		}

		void request()
		{
			SRB2_ASSERT(!valid_ && !thread_.joinable());

			thread_ = std::thread(&ChecksumState::worker, this);
		}

		void expect(const std::uint8_t chk[16])
		{
			std::lock_guard _(mutex_);

			expected_md5sum_ = std::array<uint8_t, 16>();
			std::memcpy(expected_md5sum_->data(), chk, 16);

			if (valid_)
			{
				check_expected();
			}
		}

		void join()
		{
			if (thread_.joinable())
			{
				thread_.join();
			}
		}

	private:
		wadfile_t* wad_;
		std::uint8_t md5sum_[16];
		std::optional<std::array<uint8_t, 16>> expected_md5sum_;
		std::atomic_bool valid_ = false;
		std::thread thread_;
		mutable std::mutex mutex_;
		mutable std::condition_variable cv_;

		void worker()
		{
			W_MakeFileMD5(wad_->filename, md5sum_);

			{
				std::lock_guard _(mutex_);
				check_expected();
				valid_ = true;
			}

			cv_.notify_all();

			check_collisions();
		}

		void check_collisions()
		{
			std::lock_guard _(srb2::wad::g_wadfiles_mutex);

			for (UINT16 i = 0; i < numwadfiles; ++i)
			{
				const ChecksumState& other = wadfiles[i]->internal_state->md5sum_;

				// That's us!
				if (&other == this)
				{
					continue;
				}

				// Don't block for threads in progress,
				// because they'll do their own check when
				// they're done.
				if (!other.valid_)
				{
					continue;
				}

				if (!std::memcmp(other.md5sum_, md5sum_, 16))
				{
					// FIXME: I_Error from a thread other than
					// main kind of messes up the program
					// state. It gets the message to the user,
					// but should ideally be replaced by some
					// communication with the main thread.
					I_Error(
						"MD5 checksum for '%s' matches a file already loaded.\n"
						"Was this file added twice? Check the command line parameters.\n",
						wad_->filename
					);
				}
			}
		}

		void check_expected() const
		{
			if (!expected_md5sum_ || !std::memcmp(md5sum_, expected_md5sum_->data(), 16))
			{
				return;
			}

			char got[33];
			char wanted[33];

			PrintMD5String(md5sum_, got);
			PrintMD5String(expected_md5sum_->data(), wanted);

			I_Error(
					"File is old, is corrupt or has been modified: %s (found md5: %s, wanted: %s)\n",
					wad_->filename,
					got,
					wanted
			);
		}
	};

	ChecksumState md5sum_;

	explicit wadfile_private_t(wadfile_t* wad) : md5sum_(wad) {}

	~wadfile_private_t()
	{
		md5sum_.join();
	}
};

#endif // WAD_PRIVATE_HPP
