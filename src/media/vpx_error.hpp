// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_VPX_ERROR_HPP__
#define __SRB2_MEDIA_VPX_ERROR_HPP__

#include <string>

#include <fmt/format.h>
#include <vpx/vpx_codec.h>

class VpxError
{
public:
	VpxError(vpx_codec_ctx_t& ctx) : ctx_(&ctx) {}

	std::string description() const
	{
		const char* error = vpx_codec_error(ctx_);
		const char* detail = vpx_codec_error_detail(ctx_);

		return detail ? fmt::format("{}: {}", error, detail) : error;
	}

private:
	vpx_codec_ctx_t* ctx_;
};

template <>
struct fmt::formatter<VpxError> : formatter<std::string>
{
	template <typename FormatContext>
	auto format(const VpxError& error, FormatContext& ctx) const
	{
		return formatter<std::string>::format(error.description(), ctx);
	}
};

#endif // __SRB2_MEDIA_VPX_ERROR_HPP__
