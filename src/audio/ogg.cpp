// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "ogg.hpp"

#include <limits>

#include "../cxxutil.hpp"

using namespace srb2;
using namespace srb2::audio;

StbVorbisException::StbVorbisException(int code) noexcept : code_(code)
{
}

const char* StbVorbisException::what() const noexcept
{
	switch (code_)
	{
	case VORBIS__no_error:
		return "No error";
	case VORBIS_need_more_data:
		return "Need more data";
	case VORBIS_invalid_api_mixing:
		return "Invalid API mixing";
	case VORBIS_outofmem:
		return "Out of memory";
	case VORBIS_feature_not_supported:
		return "Feature not supported";
	case VORBIS_too_many_channels:
		return "Too many channels";
	case VORBIS_file_open_failure:
		return "File open failure";
	case VORBIS_seek_without_length:
		return "Seek without length";
	case VORBIS_unexpected_eof:
		return "Unexpected EOF";
	case VORBIS_seek_invalid:
		return "Seek invalid";
	case VORBIS_invalid_setup:
		return "Invalid setup";
	case VORBIS_invalid_stream:
		return "Invalid stream";
	case VORBIS_missing_capture_pattern:
		return "Missing capture pattern";
	case VORBIS_invalid_stream_structure_version:
		return "Invalid stream structure version";
	case VORBIS_continued_packet_flag_invalid:
		return "Continued packet flag invalid";
	case VORBIS_incorrect_stream_serial_number:
		return "Incorrect stream serial number";
	case VORBIS_invalid_first_page:
		return "Invalid first page";
	case VORBIS_bad_packet_type:
		return "Bad packet type";
	case VORBIS_cant_find_last_page:
		return "Can't find last page";
	case VORBIS_seek_failed:
		return "Seek failed";
	case VORBIS_ogg_skeleton_not_supported:
		return "OGG skeleton not supported";
	default:
		return "Unrecognized error code";
	}
}

Ogg::Ogg() noexcept : memory_data_(), instance_(nullptr)
{
}

Ogg::Ogg(Vector<std::byte> data) : memory_data_(std::move(data)), instance_(nullptr)
{
	_init_with_data();
}

Ogg::Ogg(tcb::span<std::byte> data) : memory_data_(data.begin(), data.end()), instance_(nullptr)
{
	_init_with_data();
}

Ogg::Ogg(Ogg&& rhs) noexcept : memory_data_(), instance_(nullptr)
{
	std::swap(memory_data_, rhs.memory_data_);
	std::swap(instance_, rhs.instance_);
}

Ogg& Ogg::operator=(Ogg&& rhs) noexcept
{
	std::swap(memory_data_, rhs.memory_data_);
	std::swap(instance_, rhs.instance_);

	return *this;
}

Ogg::~Ogg()
{
	if (instance_)
	{
		stb_vorbis_close(instance_);
		instance_ = nullptr;
	}
}

std::size_t Ogg::get_samples(tcb::span<Sample<1>> buffer)
{
	SRB2_ASSERT(instance_ != nullptr);

	size_t read = stb_vorbis_get_samples_float_interleaved(
		instance_,
		1,
		reinterpret_cast<float*>(buffer.data()),
		buffer.size() * 1
	);

	return read;
}

std::size_t Ogg::get_samples(tcb::span<Sample<2>> buffer)
{
	SRB2_ASSERT(instance_ != nullptr);

	size_t read = stb_vorbis_get_samples_float_interleaved(
		instance_,
		2,
		reinterpret_cast<float*>(buffer.data()),
		buffer.size() * 2
	);

	stb_vorbis_info info = stb_vorbis_get_info(instance_);
	if (info.channels == 1)
	{
		for (auto& sample : buffer.subspan(0, read))
		{
			sample.amplitudes[1] = sample.amplitudes[0];
		}
	}

	return read;
}

OggComment Ogg::comment() const
{
	SRB2_ASSERT(instance_ != nullptr);

	stb_vorbis_comment c_comment = stb_vorbis_get_comment(instance_);

	return OggComment {
		String(c_comment.vendor),
		Vector<String>(c_comment.comment_list, c_comment.comment_list + c_comment.comment_list_length)};
}

std::size_t Ogg::sample_rate() const
{
	SRB2_ASSERT(instance_ != nullptr);

	stb_vorbis_info info = stb_vorbis_get_info(instance_);
	return info.sample_rate;
}

void Ogg::seek(std::size_t sample)
{
	SRB2_ASSERT(instance_ != nullptr);

	stb_vorbis_seek(instance_, sample);
}

std::size_t Ogg::position() const
{
	SRB2_ASSERT(instance_ != nullptr);

	return stb_vorbis_get_sample_offset(instance_);
}

float Ogg::position_seconds() const
{
	return position() / static_cast<float>(sample_rate());
}

std::size_t Ogg::duration_samples() const
{
	SRB2_ASSERT(instance_ != nullptr);

	return stb_vorbis_stream_length_in_samples(instance_);
}

float Ogg::duration_seconds() const
{
	SRB2_ASSERT(instance_ != nullptr);

	return stb_vorbis_stream_length_in_seconds(instance_);
}

std::size_t Ogg::channels() const
{
	SRB2_ASSERT(instance_ != nullptr);

	stb_vorbis_info info = stb_vorbis_get_info(instance_);
	return info.channels;
}

void Ogg::_init_with_data()
{
	if (instance_)
	{
		return;
	}

	if (memory_data_.size() >= std::numeric_limits<int>::max())
		throw std::logic_error("Buffer is too large for stb_vorbis");
	if (memory_data_.size() == 0)
		throw std::logic_error("Insufficient data from stream");

	int vorbis_result;
	instance_ = stb_vorbis_open_memory(
		reinterpret_cast<const unsigned char*>(memory_data_.data()),
		memory_data_.size(),
		&vorbis_result,
		NULL
	);

	if (vorbis_result != VORBIS__no_error)
		throw StbVorbisException(vorbis_result);
}
