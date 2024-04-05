// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by James Robert Roman
// Copyright (C) 2024 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_MEDIA_CFILE_HPP__
#define __SRB2_MEDIA_CFILE_HPP__

#include <cstdio>
#include <string>

namespace srb2::media
{

class CFile
{
public:
	CFile(const std::string file_name);
	~CFile();

	operator std::FILE*() const { return file_; }

	const char* name() const { return name_.c_str(); }

private:
	std::string name_;
	std::FILE* file_;
};

}; // namespace srb2::media

#endif // __SRB2_MEDIA_CFILE_HPP__
