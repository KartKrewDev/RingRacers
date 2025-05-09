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

#include <fmt/format.h>
#include <vpx/vpx_codec.h>

#include "../core/string.h"

class VpxError
{
public:
	VpxError(vpx_codec_ctx_t& ctx) : ctx_(&ctx) {}

	srb2::String description() const
	{
		const char* error = vpx_codec_error(ctx_);
		const char* detail = vpx_codec_error_detail(ctx_);

		return detail ? srb2::format("{}: {}", error, detail) : error;
	}

private:
	vpx_codec_ctx_t* ctx_;
};

template <>
struct fmt::formatter<VpxError> : formatter<srb2::String>
{
	template <typename FormatContext>
	auto format(const VpxError& error, FormatContext& ctx) const
	{
		return formatter<srb2::String>::format(error.description(), ctx);
	}
};

#endif // __SRB2_MEDIA_VPX_ERROR_HPP__
