// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022-2023 by Ronald "Eidolon" Kinard
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "gme.hpp"

#include <limits>
#include <stdexcept>

#include "../cxxutil.hpp"

using namespace srb2;
using namespace srb2::audio;

Gme::Gme() : memory_data_(), instance_(nullptr)
{
}

Gme::Gme(Gme&& rhs) noexcept : memory_data_(), instance_(nullptr)
{
	std::swap(memory_data_, rhs.memory_data_);
	std::swap(instance_, rhs.instance_);
}

Gme::Gme(std::vector<std::byte>&& data) : memory_data_(std::move(data)), instance_(nullptr)
{
	_init_with_data();
}

Gme::Gme(tcb::span<std::byte> data) : memory_data_(data.begin(), data.end()), instance_(nullptr)
{
	_init_with_data();
}

Gme& Gme::operator=(Gme&& rhs) noexcept
{
	std::swap(memory_data_, rhs.memory_data_);
	std::swap(instance_, rhs.instance_);

	return *this;
}

Gme::~Gme()
{
	if (instance_)
	{
		gme_delete(instance_);
		instance_ = nullptr;
	}
}

std::size_t Gme::get_samples(tcb::span<short> buffer)
{
	SRB2_ASSERT(instance_ != nullptr);

	gme_err_t err = gme_play(instance_, buffer.size(), buffer.data());
	if (err)
		throw GmeException(err);

	return buffer.size();
}

void Gme::seek(int sample)
{
	SRB2_ASSERT(instance_ != nullptr);

	gme_seek_samples(instance_, sample);
}

std::optional<float> Gme::duration_seconds() const
{
	SRB2_ASSERT(instance_ != nullptr);

	gme_info_t* info = nullptr;
	gme_err_t res = gme_track_info(instance_, &info, 0);
	if (res)
		throw GmeException(res);
	auto info_finally = srb2::finally([&info] { gme_free_info(info); });

	if (info->length == -1)
		return std::nullopt;

	// info lengths are in ms
	return static_cast<float>(info->length) / 1000.f;
}

std::optional<float> Gme::loop_point_seconds() const
{
	SRB2_ASSERT(instance_ != nullptr);

	gme_info_t* info = nullptr;
	gme_err_t res = gme_track_info(instance_, &info, 0);
	if (res)
		throw GmeException(res);
	auto info_finally = srb2::finally([&info] { gme_free_info(info); });

	int loop_point_ms = info->intro_length;
	if (loop_point_ms == -1)
		return std::nullopt;

	return loop_point_ms / 44100.f;
}

float Gme::position_seconds() const
{
	SRB2_ASSERT(instance_ != nullptr);

	gme_info_t* info = nullptr;
	gme_err_t res = gme_track_info(instance_, &info, 0);
	if (res)
		throw GmeException(res);
	auto info_finally = srb2::finally([&info] { gme_free_info(info); });

	int position = gme_tell(instance_);

	// adjust position, since GME's counter keeps going past loop
	if (info->length > 0)
		position %= info->length;
	else if (info->intro_length + info->loop_length > 0)
		position = position >= (info->intro_length + info->loop_length) ? (position % info->loop_length) : position;
	else
		position %= 150 * 1000; // 2.5 minutes

	return position / 1000.f;
}

void Gme::_init_with_data()
{
	if (instance_)
	{
		return;
	}

	if (memory_data_.size() >= std::numeric_limits<long>::max())
		throw std::invalid_argument("Buffer is too large for gme");
	if (memory_data_.size() == 0)
		throw std::invalid_argument("Insufficient data from stream");

	gme_err_t result =
		gme_open_data(reinterpret_cast<const void*>(memory_data_.data()), memory_data_.size(), &instance_, 44100);
	if (result)
		throw GmeException(result);

	// we no longer need the data, so there's no reason to keep the allocation
	memory_data_ = std::vector<std::byte>();

	result = gme_start_track(instance_, 0);
	if (result)
		throw GmeException(result);
}
