// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_WEBM_WRITER_HPP__
#define __SRB2_MEDIA_WEBM_WRITER_HPP__

#include <cstdio>
#include <string>

#include <mkvmuxer/mkvwriter.h>

#include "cfile.hpp"

namespace srb2::media
{

class WebmWriter : public CFile, public mkvmuxer::MkvWriter
{
public:
	WebmWriter(const std::string file_name) : CFile(file_name), MkvWriter(static_cast<std::FILE*>(*this)) {}
	~WebmWriter() { MkvWriter::Close(); }
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_WEBM_WRITER_HPP__
