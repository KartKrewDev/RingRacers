// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_AUDIO_MUSIC_PLAYER_HPP__
#define __SRB2_AUDIO_MUSIC_PLAYER_HPP__

#include <memory>
#include <optional>

#include <tcb/span.hpp>

#include "source.hpp"

struct stb_vorbis;

namespace srb2::audio
{

enum class MusicType
{
	kOgg,
	kMod
};

class MusicPlayer final : public Source<2>
{
public:
	MusicPlayer();
	MusicPlayer(tcb::span<std::byte> data);
	MusicPlayer(const MusicPlayer& rhs) = delete;
	MusicPlayer(MusicPlayer&& rhs) noexcept;

	MusicPlayer& operator=(const MusicPlayer& rhs) = delete;
	MusicPlayer& operator=(MusicPlayer&& rhs) noexcept;

	virtual std::size_t generate(tcb::span<Sample<2>> buffer) override final;

	void play(bool looping);
	void unpause();
	void pause();
	void stop();
	void seek(float position_seconds);
	void fade_to(float gain, float seconds);
	void fade_from_to(float from, float to, float seconds);
	void internal_gain(float gain);
	void stop_fade();
	void loop_point_seconds(float loop_point);
	bool playing() const;
	std::optional<MusicType> music_type() const;
	std::optional<float> duration_seconds() const;
	std::optional<float> loop_point_seconds() const;
	std::optional<float> position_seconds() const;
	bool fading() const;

	virtual ~MusicPlayer() final;

private:
	class Impl;

	std::unique_ptr<Impl> impl_;
};

} // namespace srb2::audio

#endif // __SRB2_AUDIO_MUSIC_PLAYER_HPP__
