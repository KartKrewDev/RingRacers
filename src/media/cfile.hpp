// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_CFILE_HPP__
#define __SRB2_MEDIA_CFILE_HPP__

#include <cstdio>

#include "../core/string.h"

namespace srb2::media
{

class CFile
{
public:
	CFile(const srb2::String& file_name);
	~CFile();

	operator std::FILE*() const { return file_; }

	const char* name() const { return name_.c_str(); }

private:
	srb2::String name_;
	std::FILE* file_;
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_CFILE_HPP__
