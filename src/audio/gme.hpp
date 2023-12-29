// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_GME_HPP__
#define __SRB2_AUDIO_GME_HPP__

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gme/gme.h>
#undef byte	 // BLARGG!! NO!!
#undef check // STOP IT!!!!

#include "../io/streams.hpp"

namespace srb2::audio
{

class GmeException : public std::exception
{
	std::string msg_;

public:
	explicit GmeException(gme_err_t msg) : msg_(msg == nullptr ? "" : msg) {}

	virtual const char* what() const noexcept override { return msg_.c_str(); }
};

class Gme
{
	std::vector<std::byte> memory_data_;
	Music_Emu* instance_;

public:
	Gme();
	Gme(const Gme&) = delete;
	Gme(Gme&& rhs) noexcept;

	Gme& operator=(const Gme&) = delete;
	Gme& operator=(Gme&& rhs) noexcept;

	explicit Gme(std::vector<std::byte>&& data);
	explicit Gme(tcb::span<std::byte> data);

	std::size_t get_samples(tcb::span<short> buffer);
	void seek(int position_ms);

	float duration_seconds() const;
	std::optional<float> loop_point_seconds() const;
	float position_seconds() const;

	~Gme();

private:
	struct AsyncOp;

	std::unique_ptr<AsyncOp> seeking_;

	void _init_with_data();
};

template <typename I, typename std::enable_if_t<srb2::io::IsInputStreamV<I>, int> = 0>
inline Gme load_gme(I& stream)
{
	std::vector<std::byte> data = srb2::io::read_to_vec(stream);
	return Gme {std::move(data)};
}

} // namespace srb2::audio

#endif // __SRB2_AUDIO_GME_HPP__
