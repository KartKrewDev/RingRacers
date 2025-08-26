// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_IO_STREAMS_HPP__
#define __SRB2_IO_STREAMS_HPP__

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <tcb/span.hpp>
#include <zlib.h>

#include "../core/string.h"
#include "../core/vector.hpp"

namespace srb2::io
{

using StreamSize = uint64_t;
using StreamOffset = int64_t;

enum class SeekFrom {
	kStart,
	kCurrent,
	kEnd
};

template <typename T>
struct IsInputStream
	: public std::is_same<decltype(std::declval<T&>().read(std::declval<tcb::span<std::byte>>())), StreamSize> {};

template <typename T>
struct IsOutputStream
	: public std::is_same<decltype(std::declval<T&>().write(std::declval<tcb::span<const std::byte>>())), StreamSize> {
};

template <typename T>
struct IsSeekableStream
	: public std::is_same<decltype(std::declval<T&>().seek(std::declval<SeekFrom>(), std::declval<StreamOffset>())),
						  StreamSize> {};

template <typename T>
struct IsFlushableStream : public std::is_same<decltype(std::declval<T&>().flush()), void> {};

template <typename T>
struct IsStream : public std::disjunction<IsInputStream<T>, IsOutputStream<T>> {};

template <typename T>
struct IsInputOutputStream : public std::conjunction<IsInputStream<T>, IsOutputStream<T>> {};

template <typename T>
inline constexpr const bool IsInputStreamV = IsInputStream<T>::value;
template <typename T>
inline constexpr const bool IsOutputStreamV = IsOutputStream<T>::value;
template <typename T>
inline constexpr const bool IsSeekableStreamV = IsSeekableStream<T>::value;
template <typename T>
inline constexpr const bool IsFlushableStreamV = IsFlushableStream<T>::value;
template <typename T>
inline constexpr const bool IsStreamV = IsStream<T>::value;
template <typename T>
inline constexpr const bool IsInputOutputStreamV = IsInputOutputStream<T>::value;

class UnexpectedEof : public std::logic_error
{
	using std::logic_error::logic_error;
};

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read_exact(I& stream, tcb::span<std::byte> buffer) {
	std::size_t total = 0;
	const std::size_t buf_size = buffer.size();
	while (total < buf_size) {
		total += stream.read(buffer.subspan(total, buf_size - total));
	}
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write_exact(O& stream, tcb::span<const std::byte> buffer) {
	std::size_t total = 0;
	const std::size_t buf_size = buffer.size();
	while (total < buf_size) {
		total += stream.write(buffer.subspan(total, buf_size - total));
	}
}

enum class Endian {
	kLE,
	kBE,
};

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(std::byte& value, I& stream) {
	read_exact(stream, tcb::span {&value, 1});
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(std::byte value, O& stream) {
	write_exact(stream, tcb::span {&value, 1});
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(uint8_t& value, I& stream) {
	std::byte in;
	read_exact(stream, tcb::span {&in, 1});
	value = std::to_integer<uint8_t>(in);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
uint8_t read_uint8(I& stream) {
	uint8_t ret;
	read(ret, stream);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(uint8_t value, O& stream) {
	std::byte out {value};

	write_exact(stream, tcb::span {&out, 1});
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(bool& value, I& stream) {
	uint8_t v;
	read(v, stream);
	value = !(v == 0);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
bool read_bool(I& stream) {
	bool ret;
	read(ret, stream);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(bool value, O& stream) {
	uint8_t out;
	if (value)
		out = 1;
	else
		out = 0;

	write(out, stream);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(int8_t& value, I& stream) {
	uint8_t in;
	read(in, stream);
	value = *reinterpret_cast<int8_t*>(&in);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
int8_t read_int8(I& stream) {
	int8_t ret;
	read(ret, stream);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(int8_t value, O& stream) {
	write(*reinterpret_cast<uint8_t*>(&value), stream);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(uint16_t& value, I& stream, Endian endian = Endian::kLE) {
	std::array<std::byte, 2> out;
	read_exact(stream, tcb::make_span(out));
	if (endian == Endian::kBE)
		value = std::to_integer<uint16_t>(out[1]) + (std::to_integer<uint16_t>(out[0]) << 8);
	else
		value = std::to_integer<uint16_t>(out[0]) + (std::to_integer<uint16_t>(out[1]) << 8);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
uint16_t read_uint16(I& stream, Endian endian = Endian::kLE) {
	uint16_t ret;
	read(ret, stream, endian);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(uint16_t value, O& stream, Endian endian = Endian::kLE) {
	std::array<std::byte, 2> out;

	if (endian == Endian::kBE)
		out = {std::byte {static_cast<uint8_t>((value & 0xFF00) >> 8)},
			   std::byte {static_cast<uint8_t>((value & 0x00FF) >> 0)}};
	else
		out = {std::byte {static_cast<uint8_t>((value & 0x00FF) >> 0)},
			   std::byte {static_cast<uint8_t>((value & 0xFF00) >> 8)}};

	write_exact(stream, tcb::make_span(out));
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(int16_t& value, I& stream, Endian endian = Endian::kLE) {
	uint16_t r;
	read(r, stream, endian);
	value = *reinterpret_cast<int16_t*>(&r);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
int16_t read_int16(I& stream, Endian endian = Endian::kLE) {
	int16_t ret;
	read(ret, stream, endian);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(int16_t value, O& stream, Endian endian = Endian::kLE) {
	write(*reinterpret_cast<int16_t*>(&value), stream, endian);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(uint32_t& value, I& stream, Endian endian = Endian::kLE) {
	std::array<std::byte, 4> out;
	read_exact(stream, tcb::make_span(out));
	if (endian == Endian::kBE)
		value = std::to_integer<uint32_t>(out[3]) + (std::to_integer<uint32_t>(out[2]) << 8) +
				(std::to_integer<uint32_t>(out[1]) << 16) + (std::to_integer<uint32_t>(out[0]) << 24);
	else
		value = std::to_integer<uint32_t>(out[0]) + (std::to_integer<uint32_t>(out[1]) << 8) +
				(std::to_integer<uint32_t>(out[2]) << 16) + (std::to_integer<uint32_t>(out[3]) << 24);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
uint32_t read_uint32(I& stream, Endian endian = Endian::kLE) {
	uint32_t ret;
	read(ret, stream, endian);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(uint32_t value, O& stream, Endian endian = Endian::kLE) {
	std::array<std::byte, 4> out;

	if (endian == Endian::kBE)
		out = {std::byte {static_cast<uint8_t>((value & 0xFF000000) >> 24)},
			   std::byte {static_cast<uint8_t>((value & 0x00FF0000) >> 16)},
			   std::byte {static_cast<uint8_t>((value & 0x0000FF00) >> 8)},
			   std::byte {static_cast<uint8_t>((value & 0x000000FF) >> 0)}};
	else
		out = {std::byte {static_cast<uint8_t>((value & 0x000000FF) >> 0)},
			   std::byte {static_cast<uint8_t>((value & 0x0000FF00) >> 8)},
			   std::byte {static_cast<uint8_t>((value & 0x00FF0000) >> 16)},
			   std::byte {static_cast<uint8_t>((value & 0xFF000000) >> 24)}};

	write_exact(stream, tcb::make_span(out));
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(int32_t& value, I& stream, Endian endian = Endian::kLE) {
	uint32_t r;
	read(r, stream, endian);
	value = *reinterpret_cast<int32_t*>(&r);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
int32_t read_int32(I& stream, Endian endian = Endian::kLE) {
	int32_t ret;
	read(ret, stream, endian);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(int32_t value, O& stream, Endian endian = Endian::kLE) {
	write(*reinterpret_cast<uint32_t*>(&value), stream, endian);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(uint64_t& value, I& stream, Endian endian = Endian::kLE) {
	std::array<std::byte, 8> out;
	read_exact(stream, tcb::make_span(out));
	if (endian == Endian::kBE)
		value = std::to_integer<uint64_t>(out[7]) + (std::to_integer<uint64_t>(out[6]) << 8) +
				(std::to_integer<uint64_t>(out[5]) << 16) + (std::to_integer<uint64_t>(out[4]) << 24) +
				(std::to_integer<uint64_t>(out[3]) << 32) + (std::to_integer<uint64_t>(out[2]) << 40) +
				(std::to_integer<uint64_t>(out[1]) << 48) + (std::to_integer<uint64_t>(out[0]) << 56);
	else
		value = std::to_integer<uint64_t>(out[0]) + (std::to_integer<uint64_t>(out[1]) << 8) +
				(std::to_integer<uint64_t>(out[2]) << 16) + (std::to_integer<uint64_t>(out[3]) << 24) +
				(std::to_integer<uint64_t>(out[4]) << 32) + (std::to_integer<uint64_t>(out[5]) << 40) +
				(std::to_integer<uint64_t>(out[6]) << 48) + (std::to_integer<uint64_t>(out[7]) << 56);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
uint64_t read_uint64(I& stream, Endian endian = Endian::kLE) {
	uint64_t ret;
	read(ret, stream, endian);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(uint64_t value, O& stream, Endian endian = Endian::kLE) {
	std::array<std::byte, 8> out;

	if (endian == Endian::kBE)
		out = {std::byte {static_cast<uint8_t>((value & 0xFF00000000000000) >> 56)},
			   std::byte {static_cast<uint8_t>((value & 0x00FF000000000000) >> 48)},
			   std::byte {static_cast<uint8_t>((value & 0x0000FF0000000000) >> 40)},
			   std::byte {static_cast<uint8_t>((value & 0x000000FF00000000) >> 32)},
			   std::byte {static_cast<uint8_t>((value & 0x00000000FF000000) >> 24)},
			   std::byte {static_cast<uint8_t>((value & 0x0000000000FF0000) >> 16)},
			   std::byte {static_cast<uint8_t>((value & 0x000000000000FF00) >> 8)},
			   std::byte {static_cast<uint8_t>((value & 0x00000000000000FF) >> 0)}};
	else
		out = {std::byte {static_cast<uint8_t>((value & 0x00000000000000FF) >> 0)},
			   std::byte {static_cast<uint8_t>((value & 0x000000000000FF00) >> 8)},
			   std::byte {static_cast<uint8_t>((value & 0x0000000000FF0000) >> 16)},
			   std::byte {static_cast<uint8_t>((value & 0x00000000FF000000) >> 24)},
			   std::byte {static_cast<uint8_t>((value & 0x000000FF00000000) >> 32)},
			   std::byte {static_cast<uint8_t>((value & 0x0000FF0000000000) >> 40)},
			   std::byte {static_cast<uint8_t>((value & 0x00FF000000000000) >> 48)},
			   std::byte {static_cast<uint8_t>((value & 0xFF00000000000000) >> 56)}};

	write_exact(stream, tcb::make_span(out));
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(int64_t& value, I& stream, Endian endian = Endian::kLE) {
	uint64_t r;
	read(r, stream, endian);
	value = *reinterpret_cast<int64_t*>(&r);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
int64_t read_int64(I& stream, Endian endian = Endian::kLE) {
	int64_t ret;
	read(ret, stream, endian);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(int64_t value, O& stream, Endian endian = Endian::kLE) {
	write(*reinterpret_cast<uint64_t*>(&value), stream, endian);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(float& value, I& stream, Endian endian = Endian::kLE) {
	uint32_t r;
	read(r, stream, endian);
	value = *reinterpret_cast<float*>(&r);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
float read_float(I& stream, Endian endian = Endian::kLE) {
	float ret;
	read(ret, stream, endian);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(float value, O& stream, Endian endian = Endian::kLE) {
	write(*reinterpret_cast<int32_t*>(&value), stream, endian);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
void read(double& value, I& stream, Endian endian = Endian::kLE) {
	uint64_t r;
	read(r, stream, endian);
	value = *reinterpret_cast<double*>(&r);
}

template <typename I, typename std::enable_if_t<IsInputStreamV<I>>* = nullptr>
double read_double(I& stream, Endian endian = Endian::kLE) {
	double ret;
	read(ret, stream, endian);
	return ret;
}

template <typename O, typename std::enable_if_t<IsOutputStreamV<O>>* = nullptr>
void write(double value, O& stream, Endian endian = Endian::kLE) {
	write(*reinterpret_cast<int64_t*>(&value), stream, endian);
}

template <typename S, typename std::enable_if_t<IsSeekableStreamV<S>>* = nullptr>
StreamSize remaining(S& stream) {
	const StreamSize current = stream.seek(SeekFrom::kCurrent, 0);
	const StreamSize end = stream.seek(SeekFrom::kEnd, 0);
	stream.seek(SeekFrom::kStart, current);
	return end - current;
}

// Kinds of streams

class SpanStream {
public:
	SpanStream() noexcept = default;
	SpanStream(tcb::span<std::byte> span) : span_(span), head_(0) {
		if (span_.size() > static_cast<StreamSize>(static_cast<StreamOffset>(-1))) {
			throw std::logic_error("Span must not be greater than 2 billion bytes");
		}
	};

	StreamSize read(tcb::span<std::byte> buffer) {
		if (head_ >= span_.size())
			return 0;

		auto begin = buffer.begin();
		auto end = std::copy(
			span_.begin() + head_, span_.begin() + head_ + std::min<size_t>(buffer.size(), span_.size() - head_), begin);
		head_ += std::distance(begin, end);
		return std::distance(begin, end);
	}

	StreamSize write(tcb::span<const std::byte> buffer) {
		if (head_ >= span_.size())
			return 0;

		auto begin = span_.begin() + head_;
		auto end =
			std::copy(buffer.begin(), buffer.begin() + std::min<size_t>(span_.size() - head_, buffer.size()), begin);
		head_ += std::distance(begin, end);
		return std::distance(begin, end);
	}

	StreamSize seek(SeekFrom seek_from, StreamOffset offset) {
		std::size_t head = 0;

		switch (seek_from) {
		case SeekFrom::kStart:
			if (offset < 0) {
				throw std::logic_error("start offset is out of bounds");
			}
			head = offset;
			break;
		case SeekFrom::kEnd:
			if (static_cast<StreamOffset>(span_.size()) + offset < 0) {
				throw std::logic_error("end offset is out of bounds");
			}
			head = span_.size() + offset;
			break;
		case SeekFrom::kCurrent:
			if (head_ + offset < 0) {
				throw std::logic_error("offset is out of bounds");
			}
			head = head_ + offset;
			break;
		}

		std::swap(head, head_);
		return head_;
	}

	friend void read_exact(SpanStream& stream, tcb::span<std::byte> buffer);

private:
	tcb::span<std::byte> span_;
	std::size_t head_ {0};
};

inline void read_exact(SpanStream& stream, tcb::span<std::byte> buffer)
{
	const std::size_t remaining = stream.span_.size() - stream.head_;
	const std::size_t buffer_size = buffer.size();
	if (buffer_size > remaining)
	{
		// The span's size will never change, so the generic impl of read_exact will enter an inifinite loop. We can
		// throw out early.
		throw UnexpectedEof("read buffer size > remaining bytes in span");
	}
	if (buffer_size == 0)
	{
		return;
	}

	auto copy_begin = std::next(stream.span_.begin(), stream.head_);
	auto copy_end = std::next(stream.span_.begin(), stream.head_ + buffer_size);
	stream.head_ += buffer_size;

	std::copy(copy_begin, copy_end, buffer.begin());
}

class VecStream {
	srb2::Vector<std::byte> vec_;
	std::size_t head_ {0};

public:
	VecStream() = default;
	VecStream(const srb2::Vector<std::byte>& vec) : vec_(vec) {}
	VecStream(srb2::Vector<std::byte>&& vec) : vec_(std::move(vec)) {}
	VecStream(const VecStream& rhs) = default;
	VecStream(VecStream&& rhs) = default;

	VecStream& operator=(const VecStream& rhs) = default;
	VecStream& operator=(VecStream&& rhs) = default;

	StreamSize read(tcb::span<std::byte> buffer) {
		if (head_ >= vec_.size())
			return 0;

		auto begin = buffer.begin();
		auto end =
			std::copy(vec_.begin() + head_, vec_.begin() + head_ + std::min<size_t>(buffer.size(), vec_.size() - head_), begin);
		head_ += std::distance(begin, end);
		return std::distance(begin, end);
	}

	StreamSize write(tcb::span<const std::byte> buffer) {
		const std::size_t buffer_size = buffer.size();
		if (head_ + buffer_size >= vec_.size()) {
			vec_.resize(head_ + buffer_size);
		}

		auto begin = vec_.begin() + head_;
		auto end =
			std::copy(buffer.begin(), buffer.begin() + std::min<size_t>(vec_.size() - head_, buffer.size()), begin);
		head_ += std::distance(begin, end);
		return std::distance(begin, end);
	}

	StreamSize seek(SeekFrom seek_from, StreamOffset offset) {
		std::size_t head = 0;

		switch (seek_from) {
		case SeekFrom::kStart:
			if (offset < 0) {
				throw std::logic_error("start offset is out of bounds");
			}
			head = offset;
			break;
		case SeekFrom::kEnd:
			if (static_cast<StreamOffset>(vec_.size()) + offset < 0) {
				throw std::logic_error("end offset is out of bounds");
			}
			head = vec_.size() + offset;
			break;
		case SeekFrom::kCurrent:
			if (head_ + offset < 0) {
				throw std::logic_error("offset is out of bounds");
			}
			head = head_ + offset;
			break;
		}

		std::swap(head, head_);
		return head_;
	}

	srb2::Vector<std::byte>& vector() { return vec_; }

	friend void read_exact(VecStream& stream, tcb::span<std::byte> buffer);
};

inline void read_exact(VecStream& stream, tcb::span<std::byte> buffer)
{
	const std::size_t remaining = stream.vec_.size() - stream.head_;
	const std::size_t buffer_size = buffer.size();
	if (buffer_size > remaining)
	{
		// VecStream is not thread safe, so the generic impl of read_exact would enter an infinite loop under
		// correct usage. We know when we've reached the end and can throw out early.
		throw UnexpectedEof("read buffer size > remaining bytes in vector");
	}
	if (buffer_size == 0)
	{
		return;
	}

	auto copy_begin = std::next(stream.vec_.begin(), stream.head_);
	auto copy_end = std::next(stream.vec_.begin(), stream.head_ + buffer_size);
	stream.head_ += buffer_size;

	std::copy(copy_begin, copy_end, buffer.begin());
}

enum class FileStreamMode
{
	kRead,
	kWrite,
	kAppend,
};

class FileStreamException final : public std::exception
{
	std::string msg_;

public:
	explicit FileStreamException(const char* msg);
	explicit FileStreamException(const std::string& msg);
	FileStreamException(const FileStreamException&);
	FileStreamException(FileStreamException&&) noexcept;
	~FileStreamException();

	FileStreamException& operator=(const FileStreamException&);
	FileStreamException& operator=(FileStreamException&&) noexcept;

	virtual const char* what() const noexcept override;
};

class FileStream final
{
	void* file_ = nullptr; // Type is omitted to avoid include cstdio
	FileStreamMode mode_;

public:
	FileStream() noexcept;
	FileStream(const FileStream&) = delete;
	FileStream(FileStream&&) noexcept;
	FileStream(const std::string& path, FileStreamMode mode = FileStreamMode::kRead);
	~FileStream();

	FileStream& operator=(const FileStream&) = delete;
	FileStream& operator=(FileStream&&) noexcept;

	StreamSize read(tcb::span<std::byte> buffer);
	StreamSize write(tcb::span<const std::byte> buffer);
	StreamSize seek(SeekFrom seek_from, StreamOffset offset);

	void close();
};

class ZlibException : public std::exception {
	int err_ {0};
	std::string msg_;

public:
	ZlibException(int err, const char* msg = nullptr) : err_(err), msg_("srb2::io::ZlibException: zlib error: ") {
		const char* err_msg = "(UNKNOWN) ";
		switch (err_) {
		case Z_OK:
			err_msg = "(Z_OK) ";
			break;
		case Z_STREAM_END:
			err_msg = "(Z_STREAM_END) ";
			break;
		case Z_NEED_DICT:
			err_msg = "(Z_NEED_DICT) ";
			break;
		case Z_ERRNO:
			err_msg = "(Z_ERRNO) ";
			break;
		case Z_STREAM_ERROR:
			err_msg = "(Z_STREAM_ERROR) ";
			break;
		case Z_DATA_ERROR:
			err_msg = "(Z_DATA_ERROR) ";
			break;
		case Z_MEM_ERROR:
			err_msg = "(Z_MEM_ERROR) ";
			break;
		case Z_BUF_ERROR:
			err_msg = "(Z_BUF_ERROR) ";
			break;
		case Z_VERSION_ERROR:
			err_msg = "(Z_VERSION_ERROR) ";
			break;
		}
		msg_.append(err_msg);
		if (msg != nullptr)
			msg_.append(msg);
		else
			msg_.append("nullptr");
	}

	virtual const char* what() const noexcept override final { return msg_.c_str(); }
};

template <typename I,
		  typename std::enable_if_t<IsInputStreamV<I> && std::is_move_constructible_v<I> &&
									std::is_move_assignable_v<I>>* = nullptr>
class ZlibInputStream {
	I inner_;
	z_stream stream_;
	srb2::Vector<std::byte> buf_;
	std::size_t buf_head_;
	bool zstream_initialized_;
	bool zstream_ended_;

public:
	ZlibInputStream(I&& inner)
		: inner_(std::move(inner))
		, stream_ {}
		, buf_()
		, buf_head_(0)
		, zstream_initialized_ {false}
		, zstream_ended_ {false} {}

	ZlibInputStream(const ZlibInputStream& rhs) = delete;
	ZlibInputStream(ZlibInputStream&& rhs) = delete;

	ZlibInputStream& operator=(const ZlibInputStream& rhs) = delete;
	ZlibInputStream& operator=(ZlibInputStream&& rhs) = delete;

	StreamSize read(tcb::span<std::byte> buffer) {
		if (zstream_ended_)
			return 0;

		std::size_t written = 0;
		const std::size_t buffer_size = buffer.size();
		while (written < buffer_size && !zstream_ended_) {
			_fill_read_buffer();

			if (buf_.size() == 0) {
				break;
			}

			const std::size_t written_this_time = _inflate(buffer.subspan(written));
			written += written_this_time;
		}
		return written;
	}

	I& stream() { return inner_; }

	void close() {
		if (!zstream_initialized_)
			return;

		int ret = inflateEnd(&stream_);
		if (ret != Z_OK)
			throw ZlibException {ret, stream_.msg};
		zstream_initialized_ = false;
		zstream_ended_ = true;
	}

	~ZlibInputStream() {
		if (zstream_initialized_) {
			int ret = inflateEnd(&stream_);
			if (ret != Z_OK)
				// can't throw exceptions in destructors
				std::terminate();
			zstream_initialized_ = false;
			zstream_ended_ = true;
		}
	};

private:
	constexpr static const std::size_t kReadHighWater = 2048;

	void _init() {
		stream_.avail_in = buf_.size() - buf_head_;
		const std::size_t start_avail_in = stream_.avail_in;
		stream_.next_in = reinterpret_cast<Bytef*>(buf_.data() + buf_head_);
		int ret = inflateInit2(&stream_, 32);
		if (ret != Z_OK) {
			throw ZlibException {ret, stream_.msg};
		}
		buf_head_ += start_avail_in - stream_.avail_in;
		_move_buf_backwards();
		zstream_initialized_ = true;
		zstream_ended_ = false;
	}

	void _fill_read_buffer() {
		const std::size_t old_size = buf_.size();
		if (old_size < kReadHighWater) {
			buf_.resize(kReadHighWater);
			const std::size_t read = inner_.read(tcb::span(buf_.data() + old_size, buf_.size() - old_size));
			buf_.resize(old_size + read);
		}
	}

	StreamSize _inflate(tcb::span<std::byte> out) {
		if (!zstream_initialized_) {
			_init();
		}
		if (zstream_ended_)
			return 0;

		const std::size_t out_size = out.size();

		stream_.avail_in = buf_.size() - buf_head_;
		const std::size_t start_avail_in = stream_.avail_in;
		stream_.next_in = reinterpret_cast<Bytef*>(buf_.data() + buf_head_);
		stream_.avail_out = out_size;
		const std::size_t start_avail_out = stream_.avail_out;
		stream_.next_out = reinterpret_cast<Bytef*>(out.data());

		int ret = inflate(&stream_, Z_NO_FLUSH);
		if (ret == Z_STREAM_END) {
			zstream_ended_ = true;
		} else if (ret != Z_OK && ret != Z_BUF_ERROR) {
			throw ZlibException {ret, stream_.msg};
		}

		buf_head_ += start_avail_in - stream_.avail_in;
		const std::size_t written = start_avail_out - stream_.avail_out;

		_move_buf_backwards();

		return written;
	}

	void _move_buf_backwards() {
		if (buf_head_ == 0) {
			return;
		}

		if (buf_head_ >= buf_.size()) {
			buf_.clear();
			buf_head_ = 0;
			return;
		}

		auto end = std::move(buf_.begin() + buf_head_, buf_.end(), buf_.begin());
		buf_.resize(end - buf_.begin());
		buf_head_ = 0;
	}
};

extern template class ZlibInputStream<SpanStream>;
extern template class ZlibInputStream<VecStream>;

template <typename O,
		  typename std::enable_if_t<IsOutputStreamV<O> && std::is_move_constructible_v<O> &&
									std::is_move_assignable_v<O>>* = nullptr>
class BufferedOutputStream final
{
	O inner_;
	srb2::Vector<std::byte> buf_;
	tcb::span<const std::byte>::size_type cap_;

public:
	explicit BufferedOutputStream(O&& o) : inner_(std::forward<O>(o)), buf_(), cap_(8192) {}
	BufferedOutputStream(O&& o, tcb::span<const std::byte>::size_type capacity) : inner_(std::forward<O>(o)), buf_(), cap_(capacity) {}
	BufferedOutputStream(const BufferedOutputStream&) = delete;
	BufferedOutputStream(BufferedOutputStream&&) = default;
	~BufferedOutputStream() = default;

	BufferedOutputStream& operator=(const BufferedOutputStream&) = delete;
	BufferedOutputStream& operator=(BufferedOutputStream&&) = default;

	StreamSize write(tcb::span<const std::byte> buffer)
	{
		StreamSize totalwritten = 0;
		while (buffer.size() > 0)
		{
			std::size_t tocopy = std::min(std::min(cap_, cap_ - buf_.size()), buffer.size());
			tcb::span<const std::byte> copy_slice = buffer.subspan(0, tocopy);
			buf_.reserve(cap_);
			buf_.insert(buf_.end(), copy_slice.begin(), copy_slice.end());
			flush();
			totalwritten += copy_slice.size();

			buffer = buffer.subspan(tocopy);
		}

		return totalwritten;
	}

	void flush()
	{
		tcb::span<const std::byte> writebuf = tcb::make_span(buf_);
		write_exact(inner_, writebuf);
		buf_.resize(0);
	}

	O&& stream() noexcept
	{
		return std::move(inner_);
	}
};

extern template class BufferedOutputStream<FileStream>;

template <typename I,
		  typename std::enable_if_t<IsInputStreamV<I> && std::is_move_constructible_v<I> &&
									std::is_move_assignable_v<I>>* = nullptr>
class BufferedInputStream final
{
	I inner_;
	srb2::Vector<std::byte> buf_;
	tcb::span<std::byte>::size_type cap_;

public:
	template <typename std::enable_if_t<std::is_default_constructible_v<I>>* = nullptr>
	BufferedInputStream() : inner_(), buf_(), cap_(8192) {}

	explicit BufferedInputStream(I&& i) : inner_(std::forward<I>(i)), buf_(), cap_(8192) {}
	BufferedInputStream(I&& i, tcb::span<std::byte>::size_type capacity) : inner_(std::forward<I>(i)), buf_(), cap_(capacity) {}
	BufferedInputStream(const BufferedInputStream&) = delete;
	BufferedInputStream(BufferedInputStream&&) = default;
	~BufferedInputStream() = default;

	BufferedInputStream& operator=(const BufferedInputStream&) = delete;
	BufferedInputStream& operator=(BufferedInputStream&&) = default;

	StreamSize read(tcb::span<std::byte> buffer)
	{
		StreamSize totalread = 0;
		buf_.reserve(cap_);
		while (buffer.size() > 0)
		{
			std::size_t toread = cap_ - buf_.size();
			std::size_t prereadsize = buf_.size();
			buf_.resize(prereadsize + toread);
			tcb::span<std::byte> readspan{buf_.data() + prereadsize, buf_.data() + prereadsize + toread};
			StreamSize bytesread = inner_.read(readspan);
			buf_.resize(prereadsize + bytesread);

			StreamSize tocopyfrombuf = std::min<StreamSize>(buffer.size(), buf_.size());
			std::copy(buf_.begin(), std::next(buf_.begin(), tocopyfrombuf), buffer.begin());
			buffer = buffer.subspan(tocopyfrombuf);
			totalread += tocopyfrombuf;

			// Move the remaining buffer backwards
			std::size_t bufremaining = buf_.size() - tocopyfrombuf;
			std::move(std::next(buf_.begin(), tocopyfrombuf), buf_.end(), buf_.begin());
			buf_.resize(bufremaining);

			// If we read 0 bytes from the stream, assume the inner stream won't return more for a while.
			// The caller can read in a loop if it must (i.e. read_exact)
			if (bytesread == 0)
			{
				break;
			}
		}
		return totalread;
	}

	I&& stream() noexcept
	{
		return std::move(inner_);
	}
};

extern template class BufferedInputStream<FileStream>;

// Utility functions

template <typename I, typename O>
StreamSize pipe_all(I& input, O& output) {
	srb2::Vector<std::byte> buf;

	StreamSize total_written = 0;
	StreamSize read_this_time = 0;
	do {
		buf.clear();
		buf.resize(2048);
		read_this_time = input.read(tcb::make_span(buf));
		buf.resize(read_this_time);

		write_exact(output, tcb::make_span(buf));
		total_written += read_this_time;
	} while (read_this_time != 0);

	return total_written;
}

template <typename I>
srb2::Vector<std::byte> read_to_vec(I& input) {
	VecStream out;
	pipe_all(input, out);
	return std::move(out.vector());
}

// Instantiated templates

extern template class ZlibInputStream<SpanStream>;
extern template class ZlibInputStream<VecStream>;

} // namespace srb2::io

#endif // __SRB2_IO_STREAMS_HPP__
