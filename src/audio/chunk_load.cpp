// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "chunk_load.hpp"

#include <stb_vorbis.h>

#include "../io/streams.hpp"
#include "ogg.hpp"
#include "ogg_player.hpp"
#include "resample.hpp"
#include "sound_chunk.hpp"
#include "wav.hpp"
#include "wav_player.hpp"

using std::nullopt;
using std::optional;
using std::size_t;

using namespace srb2::audio;
using namespace srb2;

namespace
{

// Utility for leveraging Resampler...
class SoundChunkSource : public Source<1>
{
public:
	explicit SoundChunkSource(std::unique_ptr<SoundChunk>&& chunk)
		: chunk_(std::forward<std::unique_ptr<SoundChunk>>(chunk))
	{
	}

	virtual size_t generate(tcb::span<Sample<1>> buffer) override final
	{
		if (!chunk_)
			return 0;

		size_t written = 0;
		for (; pos_ < chunk_->samples.size() && written < buffer.size(); pos_++)
		{
			buffer[written] = chunk_->samples[pos_];
			written++;
		}
		return written;
	}

private:
	std::unique_ptr<SoundChunk> chunk_;
	size_t pos_ {0};
};

template <class I>
std::vector<Sample<1>> generate_to_vec(I& source, std::size_t estimate = 0)
{
	std::vector<Sample<1>> generated;

	size_t total = 0;
	size_t read = 0;
	generated.reserve(estimate);
	do
	{
		generated.resize(total + 4096);
		read = source.generate(tcb::span {generated.data() + total, 4096});
		total += read;
	} while (read != 0);
	generated.resize(total);
	return generated;
}

optional<SoundChunk> try_load_dmx(tcb::span<std::byte> data)
{
	io::SpanStream stream {data};

	if (io::remaining(stream) < 8)
		return nullopt;

	uint16_t version = io::read_uint16(stream);
	if (version != 3)
		return nullopt;

	uint16_t rate = io::read_uint16(stream);
	uint32_t length = io::read_uint32(stream) - 32u;

	if (io::remaining(stream) < (length + 32u))
		return nullopt;

	stream.seek(io::SeekFrom::kCurrent, 16);

	std::vector<Sample<1>> samples;
	for (size_t i = 0; i < length; i++)
	{
		uint8_t doom_sample = io::read_uint8(stream);
		float float_sample = audio::sample_to_float(doom_sample);
		samples.push_back(Sample<1> {float_sample});
	}
	size_t samples_len = samples.size();

	if (rate == 44100)
	{
		return SoundChunk {samples};
	}

	std::unique_ptr<SoundChunkSource> chunk_source =
		std::make_unique<SoundChunkSource>(std::make_unique<SoundChunk>(SoundChunk {std::move(samples)}));
	Resampler<1> resampler(std::move(chunk_source), rate / static_cast<float>(kSampleRate));

	std::vector<Sample<1>> resampled;

	size_t total = 0;
	size_t read = 0;
	resampled.reserve(samples_len * (static_cast<float>(kSampleRate) / rate));
	do
	{
		resampled.resize(total + 4096);
		read = resampler.generate(tcb::span {resampled.data() + total, 4096});
		total += read;
	} while (read != 0);
	resampled.resize(total);

	return SoundChunk {std::move(resampled)};
}

optional<SoundChunk> try_load_wav(tcb::span<std::byte> data)
{
	io::SpanStream stream {data};

	audio::Wav wav;
	std::size_t sample_rate;

	try
	{
		wav = audio::load_wav(stream);
	}
	catch (const std::exception& ex)
	{
		return nullopt;
	}

	sample_rate = wav.sample_rate();

	audio::Resampler<1> resampler(
		std::make_unique<WavPlayer>(std::move(wav)),
		sample_rate / static_cast<float>(kSampleRate)
	);

	SoundChunk chunk {generate_to_vec(resampler)};
	return chunk;
}

optional<SoundChunk> try_load_ogg(tcb::span<std::byte> data)
{
	std::shared_ptr<audio::OggPlayer<1>> player;
	try
	{
		io::SpanStream data_stream {data};
		audio::Ogg ogg = audio::load_ogg(data_stream);
		player = std::make_shared<audio::OggPlayer<1>>(std::move(ogg));
	}
	catch (...)
	{
		return nullopt;
	}
	player->looping(false);
	player->playing(true);
	player->reset();
	std::size_t sample_rate = player->sample_rate();
	audio::Resampler<1> resampler(player, sample_rate / 44100.);
	std::vector<Sample<1>> resampled {generate_to_vec(resampler)};

	SoundChunk chunk {std::move(resampled)};
	return chunk;
}

} // namespace

optional<SoundChunk> srb2::audio::try_load_chunk(tcb::span<std::byte> data)
{
	optional<SoundChunk> ret = nullopt;

	if (data.size() == 0)
	{
		return ret;
	}

	ret = try_load_dmx(data);
	if (ret)
		return ret;

	ret = try_load_wav(data);
	if (ret)
		return ret;

	ret = try_load_ogg(data);
	if (ret)
		return ret;

	return ret;
}
