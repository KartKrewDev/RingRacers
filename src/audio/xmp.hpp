// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_XMP_HPP__
#define __SRB2_AUDIO_XMP_HPP__

#include <array>
#include <cstddef>
#include <exception>
#include <utility>

#include <tcb/span.hpp>
#include <xmp.h>

#include "../core/vector.hpp"
#include "../io/streams.hpp"

namespace srb2::audio
{

class XmpException : public std::exception
{
	int code_;

public:
	XmpException(int code);
	virtual const char* what() const noexcept override final;
};

template <size_t C>
class Xmp final
{
	Vector<std::byte> data_;
	xmp_context instance_;
	bool module_loaded_;
	bool looping_;

public:
	Xmp();

	explicit Xmp(Vector<std::byte> data);
	explicit Xmp(tcb::span<std::byte> data);

	Xmp(const Xmp<C>&) = delete;
	Xmp(Xmp<C>&& rhs) noexcept;

	Xmp& operator=(const Xmp&) = delete;
	Xmp& operator=(Xmp&& rhs) noexcept;

	std::size_t play_buffer(tcb::span<std::array<int16_t, C>> buffer);
	bool looping() const { return looping_; };
	void looping(bool looping) { looping_ = looping; };
	void reset();
	float duration_seconds() const;
	float position_seconds() const;
	void seek(int position_ms);

	~Xmp();

private:
	void _init();
};

extern template class Xmp<1>;
extern template class Xmp<2>;

template <size_t C, typename I, typename std::enable_if_t<srb2::io::IsInputStreamV<I>, int> = 0>
inline Xmp<C> load_xmp(I& stream)
{
	Vector<std::byte> data = srb2::io::read_to_vec(stream);
	return Xmp<C> {std::move(data)};
}

} // namespace srb2::audio

#endif // __SRB2_AUDIO_XMP_HPP__
