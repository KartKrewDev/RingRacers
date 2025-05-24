// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "resample.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

using std::shared_ptr;
using std::size_t;

using namespace srb2::audio;

template <size_t C>
Resampler<C>::Resampler(std::shared_ptr<Source<C>>&& source, float ratio)
	: source_(std::forward<std::shared_ptr<Source<C>>>(source)), ratio_(ratio)
{
}

template <size_t C>
Resampler<C>::Resampler(Resampler<C>&& r) = default;

template <size_t C>
Resampler<C>::~Resampler() = default;

template <size_t C>
Resampler<C>& Resampler<C>::operator=(Resampler<C>&& r) = default;

template <size_t C>
size_t Resampler<C>::generate(tcb::span<Sample<C>> buffer)
{
	if (!source_)
		return 0;

	if (ratio_ == 1.f)
	{
		// fast path - generate directly from source
		size_t source_read = source_->generate(buffer);
		return source_read;
	}

	size_t written = 0;

	while (written < buffer.size())
	{
		// do we need a refill?
		if (buf_.size() == 0 || pos_ >= static_cast<int>(buf_.size() - 1))
		{
			pos_ -= buf_.size();
			last_ = buf_.size() == 0 ? Sample<C> {} : buf_.back();
			buf_.clear();
			buf_.resize(512);
			size_t source_read = source_->generate(buf_);
			buf_.resize(source_read);
			if (source_read == 0)
			{
				break;
			}
		}

		if (pos_ < 0)
		{
			buffer[written] = (buf_[0] - last_) * pos_frac_ + last_;
			advance(ratio_);
			written++;
			continue;
		}

		buffer[written] = (buf_[pos_ + 1] - buf_[pos_]) * pos_frac_ + buf_[pos_];
		advance(ratio_);
		written++;
	}

	return written;
}

template <size_t C>
void Resampler<C>::ratio(float new_ratio)
{
	ratio_ = std::max(new_ratio, 0.f);
}

template class srb2::audio::Resampler<1>;
template class srb2::audio::Resampler<2>;
