// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_WAV_HPP__
#define __SRB2_AUDIO_WAV_HPP__

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <variant>

#include <tcb/span.hpp>

#include "../core/vector.hpp"
#include "../io/streams.hpp"
#include "sample.hpp"

namespace srb2::audio
{

class Wav final
{
	std::variant<Vector<uint8_t>, Vector<int16_t>> interleaved_samples_;
	std::size_t channels_ = 1;
	std::size_t sample_rate_ = 44100;

public:
	Wav();

	explicit Wav(tcb::span<std::byte> data);

	std::size_t get_samples(std::size_t offset, tcb::span<Sample<1>> buffer) const noexcept;
	std::size_t interleaved_length() const noexcept;
	std::size_t length() const noexcept { return interleaved_length() / channels(); };
	std::size_t channels() const noexcept { return channels_; };
	std::size_t sample_rate() const noexcept { return sample_rate_; };
};

template <typename I, typename std::enable_if_t<srb2::io::IsInputStreamV<I>, int> = 0>
inline Wav load_wav(I& stream)
{
	Vector<std::byte> data = srb2::io::read_to_vec(stream);
	return Wav {data};
}

} // namespace srb2::audio

#endif // __SRB2_AUDIO_WAV_HPP__
