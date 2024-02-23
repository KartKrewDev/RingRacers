// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "streams.hpp"

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <stdexcept>

template class srb2::io::ZlibInputStream<srb2::io::SpanStream>;
template class srb2::io::ZlibInputStream<srb2::io::VecStream>;
template class srb2::io::BufferedOutputStream<srb2::io::FileStream>;
template class srb2::io::BufferedInputStream<srb2::io::FileStream>;

using namespace srb2::io;

static FileStreamException make_exception_from_errno(int err)
{
	char* errnostr = strerror(err);
	return FileStreamException(std::string(errnostr));
}

FileStreamException::FileStreamException(const char* msg) : msg_(msg) {}
FileStreamException::FileStreamException(const std::string& msg) : msg_(msg) {}

FileStreamException::FileStreamException(const FileStreamException&) = default;
FileStreamException::FileStreamException(FileStreamException&& r) noexcept = default;

FileStreamException::~FileStreamException() = default;

FileStreamException& FileStreamException::operator=(const FileStreamException&) = default;
FileStreamException& FileStreamException::operator=(FileStreamException&&) noexcept = default;

const char* FileStreamException::what() const noexcept
{
	return msg_.c_str();
}

FileStream::FileStream() noexcept = default;
FileStream::FileStream(FileStream&& r) noexcept
{
	*this = std::move(r);
};

FileStream::~FileStream()
{
	if (file_)
	{
		// We don't care about the result. Exceptions can't be thrown in destructors.
		std::fclose((std::FILE*)(this->file_));
	}
	file_ = nullptr;
}

FileStream::FileStream(const std::string& path, FileStreamMode mode) : file_(nullptr), mode_(mode)
{
	const char* fopenmode = "r";
	switch (mode_)
	{
		case FileStreamMode::kRead:
			fopenmode = "rb";
			break;
		case FileStreamMode::kWrite:
			fopenmode = "wb";
			break;
		case FileStreamMode::kAppend:
			fopenmode = "ab";
			break;
		default:
			throw std::invalid_argument("file stream mode unsupported");
	}

	std::FILE* file = std::fopen(path.c_str(), fopenmode);
	if (file == nullptr)
	{
		int err = errno;
		throw make_exception_from_errno(err);
	}

	// We want raw, unbuffered IO for the stream. Buffering can be layered on top.
	if (std::setvbuf(file, nullptr, _IONBF, 0) != 0)
	{
		int err = errno;
		throw make_exception_from_errno(err);
	}

	this->file_ = (void*) file;
}

FileStream& FileStream::operator=(FileStream&& r) noexcept
{
	file_ = r.file_;
	r.file_ = nullptr;
	mode_ = r.mode_;
	return *this;
};

StreamSize FileStream::read(tcb::span<std::byte> buffer)
{
	if (this->file_ == nullptr)
	{
		throw std::domain_error("FileStream is empty");
	}

	if (this->mode_ != FileStreamMode::kRead)
	{
		throw std::domain_error("FileStream is not in read mode");
	}

	void* cbuf = (void*)(buffer.data());
	std::size_t cbufsize = buffer.size_bytes();
	std::size_t bytesread = fread(cbuf, 1, cbufsize, (std::FILE*)(this->file_));
	if (std::ferror((std::FILE*)(this->file_)) != 0)
	{
		int err = errno;
		throw make_exception_from_errno(err);
	}
	return bytesread;
}

StreamSize FileStream::write(tcb::span<const std::byte> buffer)
{
	if (this->file_ == nullptr)
	{
		throw std::domain_error("FileStream is empty");
	}

	if (this->mode_ == FileStreamMode::kRead)
	{
		throw std::domain_error("FileStream is not in writable mode");
	}

	void* cbuf = (void*)(buffer.data());
	std::size_t cbufsize = buffer.size_bytes();
	std::size_t byteswritten = std::fwrite(cbuf, 1, cbufsize, (std::FILE*)(this->file_));
	if (std::ferror((std::FILE*)(this->file_)) != 0)
	{
		int err = errno;
		throw make_exception_from_errno(err);
	}
	return byteswritten;
}

void FileStream::close()
{
	if (!file_)
	{
		return;
	}

	if (std::fclose((std::FILE*)(this->file_)) != 0)
	{
		// The FILE is now invalid even though fclose failed.
		// There is nothing we can do but abandon the pointer.
		file_ = nullptr;
		int err = errno;
		throw make_exception_from_errno(err);
	}

	file_ = nullptr;
}
