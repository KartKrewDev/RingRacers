// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "filter.hpp"

using std::shared_ptr;
using std::size_t;

using srb2::audio::Filter;
using srb2::audio::Sample;
using srb2::audio::Source;

template <size_t IC, size_t OC>
size_t Filter<IC, OC>::generate(tcb::span<Sample<OC>> buffer)
{
	input_buffer_.clear();
	input_buffer_.resize(buffer.size());

	input_->generate(input_buffer_);

	return filter(input_buffer_, buffer);
}

template <size_t IC, size_t OC>
void Filter<IC, OC>::bind(const shared_ptr<Source<IC>>& input)
{
	input_ = input;
}

template <size_t IC, size_t OC>
Filter<IC, OC>::~Filter() = default;

template class srb2::audio::Filter<1, 1>;
template class srb2::audio::Filter<1, 2>;
template class srb2::audio::Filter<2, 1>;
template class srb2::audio::Filter<2, 2>;
