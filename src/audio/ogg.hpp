// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_OGG_HPP__
#define __SRB2_AUDIO_OGG_HPP__

#include <exception>
#include <vector>

#include <stb_vorbis.h>
#include <tcb/span.hpp>

#include "../io/streams.hpp"
#include "sample.hpp"

namespace srb2::audio
{

class StbVorbisException final : public std::exception
{
	int code_;

public:
	explicit StbVorbisException(int code) noexcept;

	virtual const char* what() const noexcept;
};

struct OggComment
{
	String vendor;
	Vector<String> comments;
};

class Ogg final
{
	Vector<std::byte> memory_data_;
	stb_vorbis* instance_;

public:
	Ogg() noexcept;

	explicit Ogg(Vector<std::byte> data);
	explicit Ogg(tcb::span<std::byte> data);

	Ogg(const Ogg&) = delete;
	Ogg(Ogg&& rhs) noexcept;

	Ogg& operator=(const Ogg&) = delete;
	Ogg& operator=(Ogg&& rhs) noexcept;

	~Ogg();

	std::size_t get_samples(tcb::span<Sample<1>> buffer);
	std::size_t get_samples(tcb::span<Sample<2>> buffer);
	void seek(std::size_t sample);
	std::size_t position() const;
	float position_seconds() const;

	OggComment comment() const;
	std::size_t sample_rate() const;
	std::size_t channels() const;
	std::size_t duration_samples() const;
	float duration_seconds() const;

private:
	void _init_with_data();
};

template <typename I, typename std::enable_if_t<srb2::io::IsInputStreamV<I>, int> = 0>
inline Ogg load_ogg(I& stream)
{
	srb2::Vector<std::byte> data = srb2::io::read_to_vec(stream);
	return Ogg {std::move(data)};
}

} // namespace srb2::audio

#endif // __SRB2_AUDIO_OGG_HPP__
