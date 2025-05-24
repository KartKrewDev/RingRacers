// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

#include <tcb/span.hpp>

#include "webm_vorbis.hpp"

// https://www.matroska.org/technical/notes.html#xiph-lacing
// https://www.matroska.org/technical/codec_specs.html#a_vorbis

using namespace srb2::media;

static std::size_t lace_length(const ogg_packet& op)
{
	return (op.bytes / 255) + 1;
}

static void lace(std::vector<std::byte>& v, const ogg_packet& op)
{
	// The lacing size is encoded in at least one byte. If
	// the value is 255, add the value of the next byte in
	// sequence. This ends with a byte that is less than 255.

	std::fill_n(std::back_inserter(v), lace_length(op) - 1, std::byte {255});

	const unsigned char n = (op.bytes % 255);
	v.emplace_back(std::byte {n});
}

std::vector<std::byte> WebmVorbisEncoder::make_vorbis_private_data()
{
	const headers_t packets = generate_headers();

	std::vector<std::byte> v;

	// There are three Vorbis header packets. The lacing for
	// these packets in Matroska does not count the final
	// packet.

	// clang-format off
	v.reserve(
			1
			+ lace_length(packets[0])
			+ lace_length(packets[1])
			+ packets[0].bytes
			+ packets[1].bytes
			+ packets[2].bytes);
	// clang-format on

	// The first byte is the number of packets. Once again,
	// the last packet is not counted.
	v.resize(1);
	v[0] = std::byte {2};

	// Then the laced sizes for each packet.
	lace(v, packets[0]);
	lace(v, packets[1]);

	// Then each packet's data. The last packet's data
	// actually is written here.
	for (auto op : packets)
	{
		tcb::span<const std::byte> p(reinterpret_cast<const std::byte*>(op.packet), op.bytes);

		std::copy(p.begin(), p.end(), std::back_inserter(v));
	}

	return v;
}
