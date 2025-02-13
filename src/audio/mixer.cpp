// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "mixer.hpp"

#include <algorithm>

using std::shared_ptr;
using std::size_t;

using srb2::audio::Mixer;
using srb2::audio::Sample;
using srb2::audio::Source;

namespace
{

template <size_t C>
void default_init_sample_buffer(Sample<C>* buffer, size_t size)
{
	std::for_each(buffer, buffer + size, [](auto& i) { i = Sample<C> {}; });
}

template <size_t C>
void mix_sample_buffers(Sample<C>* dst, size_t size, Sample<C>* src, size_t src_size)
{
	for (size_t i = 0; i < size && i < src_size; i++)
	{
		dst[i] += src[i];
	}
}

} // namespace

template <size_t C>
size_t Mixer<C>::generate(tcb::span<Sample<C>> buffer)
{
	buffer_.resize(buffer.size());

	default_init_sample_buffer<C>(buffer.data(), buffer.size());

	for (auto& source : sources_)
	{
		size_t read = source->generate(buffer_);

		mix_sample_buffers<C>(buffer.data(), buffer.size(), buffer_.data(), read);
	}

	// because we initialized the out-buffer, we always generate size samples
	return buffer.size();
}

template <size_t C>
void Mixer<C>::add_source(const shared_ptr<Source<C>>& source)
{
	sources_.push_back(source);
}

template <size_t C>
Mixer<C>::~Mixer() = default;

template class srb2::audio::Mixer<1>;
template class srb2::audio::Mixer<2>;
