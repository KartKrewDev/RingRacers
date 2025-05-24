// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_SAMPLE_HPP__
#define __SRB2_AUDIO_SAMPLE_HPP__

#include <array>
#include <cstddef>
#include <cstdint>

namespace srb2::audio
{

template <size_t C>
struct Sample
{
	std::array<float, C> amplitudes;

	constexpr Sample& operator+=(const Sample& rhs) noexcept
	{
		for (std::size_t i = 0; i < C; i++)
		{
			amplitudes[i] += rhs.amplitudes[i];
		}
		return *this;
	}

	constexpr Sample& operator*=(float rhs) noexcept
	{
		for (std::size_t i = 0; i < C; i++)
		{
			amplitudes[i] *= rhs;
		}
		return *this;
	}
};

template <size_t C>
constexpr Sample<C> operator+(const Sample<C>& lhs, const Sample<C>& rhs) noexcept
{
	Sample<C> out;
	for (std::size_t i = 0; i < C; i++)
	{
		out.amplitudes[i] = lhs.amplitudes[i] + rhs.amplitudes[i];
	}
	return out;
}

template <size_t C>
constexpr Sample<C> operator-(const Sample<C>& lhs, const Sample<C>& rhs) noexcept
{
	Sample<C> out;
	for (std::size_t i = 0; i < C; i++)
	{
		out.amplitudes[i] = lhs.amplitudes[i] - rhs.amplitudes[i];
	}
	return out;
}

template <size_t C>
constexpr Sample<C> operator*(const Sample<C>& lhs, float rhs) noexcept
{
	Sample<C> out;
	for (std::size_t i = 0; i < C; i++)
	{
		out.amplitudes[i] = lhs.amplitudes[i] * rhs;
	}
	return out;
}

template <class T>
static constexpr float sample_to_float(T sample) noexcept;

template <>
constexpr float sample_to_float<uint8_t>(uint8_t sample) noexcept
{
	return (sample / 128.f) - 1.f;
}

template <>
constexpr float sample_to_float<int16_t>(int16_t sample) noexcept
{
	return sample / 32768.f;
}

} // namespace srb2::audio

#endif // __SRB2_AUDIO_SAMPLE_HPP__
