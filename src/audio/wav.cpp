// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "wav.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>

#include "../cxxutil.hpp"

using namespace srb2;
using srb2::audio::Wav;

namespace
{

constexpr const uint32_t kMagicRIFF = 0x46464952;
constexpr const uint32_t kMagicWAVE = 0x45564157;
constexpr const uint32_t kMagicFmt = 0x20746d66;
constexpr const uint32_t kMagicData = 0x61746164;

constexpr const uint16_t kFormatPcm = 1;

constexpr const std::size_t kRiffHeaderLength = 8;

struct RiffHeader
{
	uint32_t magic;
	std::size_t filesize;
};

struct TagHeader
{
	uint32_t type;
	std::size_t length;
};

struct FmtTag
{
	uint16_t format;
	uint16_t channels;
	uint32_t rate;
	uint32_t bytes_per_second;
	uint32_t bytes_per_sample;
	uint16_t bit_width;
};

struct DataTag
{
};

RiffHeader parse_riff_header(io::SpanStream& stream)
{
	if (io::remaining(stream) < kRiffHeaderLength)
		throw std::runtime_error("insufficient bytes remaining in stream");

	RiffHeader ret;
	ret.magic = io::read_uint32(stream);
	ret.filesize = io::read_uint32(stream);
	return ret;
}

TagHeader parse_tag_header(io::SpanStream& stream)
{
	if (io::remaining(stream) < 8)
		throw std::runtime_error("insufficient bytes remaining in stream");

	TagHeader header;
	header.type = io::read_uint32(stream);
	header.length = io::read_uint32(stream);
	return header;
}

FmtTag parse_fmt_tag(io::SpanStream& stream)
{
	if (io::remaining(stream) < 16)
		throw std::runtime_error("insufficient bytes in stream");

	FmtTag tag;
	tag.format = io::read_uint16(stream);
	tag.channels = io::read_uint16(stream);
	tag.rate = io::read_uint32(stream);
	tag.bytes_per_second = io::read_uint32(stream);
	tag.bytes_per_sample = io::read_uint16(stream);
	tag.bit_width = io::read_uint16(stream);

	return tag;
}

template <typename Visitor>
void visit_tag(Visitor& visitor, io::SpanStream& stream, const TagHeader& header)
{
	if (io::remaining(stream) < header.length)
		throw std::runtime_error("insufficient bytes in stream");

	const io::StreamSize start = stream.seek(io::SeekFrom::kCurrent, 0);
	const io::StreamSize dest = start + header.length;

	switch (header.type)
	{
	case kMagicFmt:
		{
			FmtTag fmt_tag {parse_fmt_tag(stream)};
			visitor(fmt_tag);
			break;
		}
	case kMagicData:
		{
			DataTag data_tag;
			visitor(data_tag);
			break;
		}
	default:
		// Unrecognized tags are ignored.
		break;
	}

	stream.seek(io::SeekFrom::kStart, dest);
}

Vector<uint8_t> read_uint8_samples_from_stream(io::SpanStream& stream, std::size_t count)
{
	Vector<uint8_t> samples;
	samples.reserve(count);
	for (std::size_t i = 0; i < count; i++)
	{
		samples.push_back(io::read_uint8(stream));
	}
	return samples;
}

Vector<int16_t> read_int16_samples_from_stream(io::SpanStream& stream, std::size_t count)
{
	Vector<int16_t> samples;
	samples.reserve(count);
	for (std::size_t i = 0; i < count; i++)
	{
		samples.push_back(io::read_int16(stream));
	}
	return samples;
}

} // namespace

Wav::Wav() = default;

Wav::Wav(tcb::span<std::byte> data)
{
	io::SpanStream stream {data};

	auto [magic, filesize] = parse_riff_header(stream);

	if (magic != kMagicRIFF)
	{
		throw std::runtime_error("invalid RIFF magic");
	}

	if (io::remaining(stream) < filesize)
	{
		throw std::runtime_error("insufficient data in stream for RIFF's reported filesize");
	}

	const io::StreamSize riff_end = stream.seek(io::SeekFrom::kCurrent, 0) + filesize;

	uint32_t type = io::read_uint32(stream);
	if (type != kMagicWAVE)
	{
		throw std::runtime_error("RIFF in stream is not a WAVE");
	}

	std::optional<FmtTag> read_fmt;
	std::variant<Vector<uint8_t>, Vector<int16_t>> interleaved_samples;

	while (stream.seek(io::SeekFrom::kCurrent, 0) < riff_end)
	{
		TagHeader tag_header {parse_tag_header(stream)};
		if (io::remaining(stream) < tag_header.length)
		{
			throw std::runtime_error("WAVE tag length exceeds stream length");
		}

		auto tag_visitor = srb2::Overload {
			[&](const FmtTag& fmt)
			{
				if (read_fmt)
				{
					throw std::runtime_error("WAVE has multiple 'fmt' tags");
				}
				if (fmt.format != kFormatPcm)
				{
					throw std::runtime_error("Unsupported WAVE format (only PCM is supported)");
				}
				read_fmt = fmt;
			},
			[&](const DataTag& data)
			{
				if (!read_fmt)
				{
					throw std::runtime_error("unable to read data tag because no fmt tag was read");
				}

				if (tag_header.length % read_fmt->bytes_per_sample != 0)
				{
					throw std::runtime_error("data tag length not divisible by bytes_per_sample");
				}

				const std::size_t sample_count = tag_header.length / read_fmt->bytes_per_sample;

				switch (read_fmt->bit_width)
				{
				case 8:
					interleaved_samples = read_uint8_samples_from_stream(stream, sample_count);
					break;
				case 16:
					interleaved_samples = read_int16_samples_from_stream(stream, sample_count);
					break;
				default:
					throw std::runtime_error("unsupported sample amplitude bit width");
				}
			}};

		visit_tag(tag_visitor, stream, tag_header);
	}

	if (!read_fmt)
	{
		throw std::runtime_error("WAVE did not have a fmt tag");
	}

	interleaved_samples_ = std::move(interleaved_samples);
	channels_ = read_fmt->channels;
	sample_rate_ = read_fmt->rate;
}

namespace
{

template <typename T>
std::size_t read_samples(
	std::size_t channels,
	std::size_t offset,
	const Vector<T>& samples,
	tcb::span<audio::Sample<1>> buffer
) noexcept
{
	const std::size_t offset_interleaved = offset * channels;
	const std::size_t samples_size = samples.size();
	const std::size_t buffer_size = buffer.size();

	if (offset_interleaved >= samples_size)
	{
		return 0;
	}

	const std::size_t remainder = (samples_size - offset_interleaved) / channels;
	const std::size_t samples_to_read = std::min(buffer_size, remainder);

	for (std::size_t i = 0; i < samples_to_read; i++)
	{
		buffer[i].amplitudes[0] = 0.f;
		for (std::size_t j = 0; j < channels; j++)
		{
			buffer[i].amplitudes[0] += audio::sample_to_float(samples[i * channels + j + offset_interleaved]);
		}
		buffer[i].amplitudes[0] /= static_cast<float>(channels);
	}

	return samples_to_read;
}

} // namespace

std::size_t Wav::get_samples(std::size_t offset, tcb::span<audio::Sample<1>> buffer) const noexcept
{
	auto samples_visitor = srb2::Overload {
		[&](const Vector<uint8_t>& samples) { return read_samples<uint8_t>(channels(), offset, samples, buffer); },
		[&](const Vector<int16_t>& samples)
		{ return read_samples<int16_t>(channels(), offset, samples, buffer); }};

	return std::visit(samples_visitor, interleaved_samples_);
}

std::size_t Wav::interleaved_length() const noexcept
{
	auto samples_visitor = srb2::Overload {
		[](const Vector<uint8_t>& samples) { return samples.size(); },
		[](const Vector<int16_t>& samples) { return samples.size(); }};
	return std::visit(samples_visitor, interleaved_samples_);
}
