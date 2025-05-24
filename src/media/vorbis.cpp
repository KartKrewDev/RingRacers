// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <chrono>
#include <cstddef>
#include <stdexcept>

#include <fmt/format.h>
#include <vorbis/vorbisenc.h>

#include "../cxxutil.hpp"
#include "vorbis.hpp"
#include "vorbis_error.hpp"

using namespace srb2::media;

namespace
{

void runtime_assert(VorbisError error, const char *what)
{
	if (error != 0)
	{
		throw std::runtime_error(fmt::format("{}: {}", what, error));
	}
}

}; // namespace

VorbisEncoder::VorbisEncoder(Config cfg)
{
	const long max_bitrate = options_.get<int>("max_bitrate");
	const long nominal_bitrate = options_.get<int>("nominal_bitrate");
	const long min_bitrate = options_.get<int>("min_bitrate");

	vorbis_info_init(&vi_);

	if (max_bitrate != -1 || nominal_bitrate != -1 || min_bitrate != -1)
	{
		// managed bitrate mode
		VorbisError error =
			vorbis_encode_init(&vi_, cfg.channels, cfg.sample_rate, max_bitrate, nominal_bitrate, min_bitrate);

		if (error != 0)
		{
			throw std::invalid_argument(fmt::format(
				"vorbis_encode_init: {}, max_bitrate={}, nominal_bitrate={}, min_bitrate={}",
				error,
				max_bitrate,
				nominal_bitrate,
				min_bitrate
			));
		}
	}
	else
	{
		// variable bitrate mode
		const float quality = options_.get<float>("quality");

		VorbisError error = vorbis_encode_init_vbr(&vi_, cfg.channels, cfg.sample_rate, quality);

		if (error != 0)
		{
			throw std::invalid_argument(fmt::format("vorbis_encode_init: {}, quality={}", error, quality));
		}
	}

	runtime_assert(vorbis_analysis_init(&vd_, &vi_), "vorbis_analysis_init");
	runtime_assert(vorbis_block_init(&vd_, &vb_), "vorbis_block_init");
}

VorbisEncoder::~VorbisEncoder()
{
	vorbis_block_clear(&vb_);
	vorbis_dsp_clear(&vd_);
	vorbis_info_clear(&vi_);
}

VorbisEncoder::headers_t VorbisEncoder::generate_headers()
{
	headers_t op;

	vorbis_comment vc;
	vorbis_comment_init(&vc);

	VorbisError error = vorbis_analysis_headerout(&vd_, &vc, &op[0], &op[1], &op[2]);

	if (error != 0)
	{
		throw std::invalid_argument(fmt::format("vorbis_analysis_headerout: {}", error));
	}

	vorbis_comment_clear(&vc);

	return op;
}

void VorbisEncoder::analyse(sample_buffer_t in)
{
	const int ch = channels();

	const std::size_t n = in.size() / ch;
	float** fv = vorbis_analysis_buffer(&vd_, n);

	for (std::size_t i = 0; i < n; ++i)
	{
		auto s = in.subspan(i * ch, ch);

		fv[0][i] = s[0];
		fv[1][i] = s[1];
	}

	// automatically handles end of stream if n = 0
	runtime_assert(vorbis_analysis_wrote(&vd_, n), "vorbis_analysis_wrote");

	while (vorbis_analysis_blockout(&vd_, &vb_) > 0)
	{
		runtime_assert(vorbis_analysis(&vb_, nullptr), "vorbis_analysis");
		runtime_assert(vorbis_bitrate_addblock(&vb_), "vorbis_bitrate_addblock");

		ogg_packet op;

		while (vorbis_bitrate_flushpacket(&vd_, &op) > 0)
		{
			write_packet(&op);
		}
	}
}

void VorbisEncoder::write_packet(ogg_packet* op)
{
	using T = const std::byte;
	tcb::span<T> p(reinterpret_cast<T*>(op->packet), static_cast<std::size_t>(op->bytes));

	write_frame(p, std::chrono::duration<float>(vorbis_granule_time(&vd_, op->granulepos)), true);
}
