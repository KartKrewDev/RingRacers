// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <fmt/format.h>

#include "cfile.hpp"

using namespace srb2::media;

CFile::CFile(const srb2::String& file_name) : name_(file_name)
{
	file_ = std::fopen(name(), "wb");

	if (file_ == nullptr)
	{
		throw std::invalid_argument(fmt::format("{}: {}", name(), std::strerror(errno)));
	}
}

CFile::~CFile()
{
	std::fclose(file_);
}
