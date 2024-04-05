// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by James Robert Roman
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_VORBIS_ERROR_HPP__
#define __SRB2_MEDIA_VORBIS_ERROR_HPP__

#include <string>

#include <fmt/format.h>
#include <vorbis/codec.h>

class VorbisError
{
public:
	VorbisError(int error) : error_(error) {}

	operator int() const { return error_; }

	std::string name() const
	{
		switch (error_)
		{
		case OV_EFAULT:
			return "Internal error (OV_EFAULT)";
		case OV_EINVAL:
			return "Invalid settings (OV_EINVAL)";
		case OV_EIMPL:
			return "Invalid settings (OV_EIMPL)";
		default:
			return fmt::format("error {}", error_);
		}
	}

private:
	int error_;
};

template <>
struct fmt::formatter<VorbisError> : formatter<std::string>
{
	template <typename FormatContext>
	auto format(const VorbisError& error, FormatContext& ctx) const
	{
		return formatter<std::string>::format(error.name(), ctx);
	}
};

#endif // __SRB2_MEDIA_VORBIS_ERROR_HPP__
