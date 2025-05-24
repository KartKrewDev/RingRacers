// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "xmp.hpp"

#include <limits>

#include "../cxxutil.hpp"

using namespace srb2;
using namespace srb2::audio;

XmpException::XmpException(int code) : code_(code)
{
}

const char* XmpException::what() const noexcept
{
	switch (code_)
	{
	case -XMP_ERROR_INTERNAL:
		return "XMP_ERROR_INTERNAL";
	case -XMP_ERROR_FORMAT:
		return "XMP_ERROR_FORMAT";
	case -XMP_ERROR_LOAD:
		return "XMP_ERROR_LOAD";
	case -XMP_ERROR_DEPACK:
		return "XMP_ERROR_DEPACK";
	case -XMP_ERROR_SYSTEM:
		return "XMP_ERROR_SYSTEM";
	case -XMP_ERROR_INVALID:
		return "XMP_ERROR_INVALID";
	case -XMP_ERROR_STATE:
		return "XMP_ERROR_STATE";
	default:
		return "unknown";
	}
}

template <size_t C>
Xmp<C>::Xmp() : data_(), instance_(nullptr), module_loaded_(false), looping_(false)
{
}

template <size_t C>
Xmp<C>::Xmp(Vector<std::byte> data)
	: data_(std::move(data)), instance_(nullptr), module_loaded_(false), looping_(false)
{
	_init();
}

template <size_t C>
Xmp<C>::Xmp(tcb::span<std::byte> data)
	: data_(data.begin(), data.end()), instance_(nullptr), module_loaded_(false), looping_(false)
{
	_init();
}

template <size_t C>
Xmp<C>::Xmp(Xmp<C>&& rhs) noexcept : Xmp<C>()
{
	std::swap(data_, rhs.data_);
	std::swap(instance_, rhs.instance_);
	std::swap(module_loaded_, rhs.module_loaded_);
	std::swap(looping_, rhs.looping_);
}

template <size_t C>
Xmp<C>& Xmp<C>::operator=(Xmp<C>&& rhs) noexcept
{
	std::swap(data_, rhs.data_);
	std::swap(instance_, rhs.instance_);
	std::swap(module_loaded_, rhs.module_loaded_);
	std::swap(looping_, rhs.looping_);

	return *this;
};

template <size_t C>
Xmp<C>::~Xmp()
{
	if (instance_)
	{
		xmp_free_context(instance_);
		instance_ = nullptr;
	}
}

template <size_t C>
std::size_t Xmp<C>::play_buffer(tcb::span<std::array<int16_t, C>> buffer)
{
	SRB2_ASSERT(instance_ != nullptr);
	SRB2_ASSERT(module_loaded_ == true);

	int result = xmp_play_buffer(instance_, buffer.data(), buffer.size_bytes(), !looping_);

	if (result == -XMP_END)
		return 0;

	if (result != 0)
		throw XmpException(result);

	return buffer.size();
}

template <size_t C>
void Xmp<C>::reset()
{
	SRB2_ASSERT(instance_ != nullptr);
	SRB2_ASSERT(module_loaded_ == true);

	xmp_restart_module(instance_);
}

template <size_t C>
float Xmp<C>::duration_seconds() const
{
	SRB2_ASSERT(instance_ != nullptr);
	SRB2_ASSERT(module_loaded_ == true);

	xmp_frame_info info;
	xmp_get_frame_info(instance_, &info);
	return static_cast<float>(info.total_time) / 1000.f;
}

template <size_t C>
float Xmp<C>::position_seconds() const {
	SRB2_ASSERT(instance_ != nullptr);
	SRB2_ASSERT(module_loaded_ == true);

	xmp_frame_info info;
	xmp_get_frame_info(instance_, &info);
	return static_cast<float>(info.time) / 1000.f;
}

template <size_t C>
void Xmp<C>::seek(int position_ms)
{
	SRB2_ASSERT(instance_ != nullptr);
	SRB2_ASSERT(module_loaded_ == true);

	int pos = xmp_seek_time(instance_, position_ms);
	if (pos < 0)
	{
		throw XmpException(pos);
	}
}

template <size_t C>
void Xmp<C>::_init()
{
	if (instance_)
		return;

	if (data_.size() >= std::numeric_limits<long>::max())
		throw std::logic_error("Buffer is too large for xmp");
	if (data_.size() == 0)
		throw std::logic_error("Insufficient data from stream");

	instance_ = xmp_create_context();
	if (instance_ == nullptr)
	{
		throw std::bad_alloc();
	}

	int result = xmp_load_module_from_memory(instance_, data_.data(), data_.size());
	if (result != 0)
	{
		xmp_free_context(instance_);
		instance_ = nullptr;
		throw XmpException(result);
	}
	module_loaded_ = true;

	int flags = 0;
	if constexpr (C == 1)
	{
		flags |= XMP_FORMAT_MONO;
	}
	result = xmp_start_player(instance_, 44100, flags);
	if (result != 0)
	{
		xmp_release_module(instance_);
		module_loaded_ = false;
		xmp_free_context(instance_);
		instance_ = nullptr;
		throw XmpException(result);
	}
}

template class srb2::audio::Xmp<1>;
template class srb2::audio::Xmp<2>;
