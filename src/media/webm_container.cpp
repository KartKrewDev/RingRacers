// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include <cstdint>
#include <memory>
#include <stdexcept>

#include <fmt/format.h>

#include "../cxxutil.hpp"
#include "webm_vorbis.hpp"
#include "webm_vp8.hpp"

using namespace srb2::media;

using time_unit_t = MediaEncoder::time_unit_t;

WebmContainer::WebmContainer(const Config cfg) : writer_(cfg.file_name), dtor_cb_(cfg.destructor_callback)
{
	if (!segment_.Init(&writer_))
	{
		throw std::runtime_error("mkvmuxer::Segment::Init");
	}
}

WebmContainer::~WebmContainer()
{
	flush_queue();

	if (!segment_.Finalize())
	{
		CONS_Alert(CONS_WARNING, "mkvmuxer::Segment::Finalize has failed\n");
	}

	finalized_ = true;

	if (dtor_cb_)
	{
		dtor_cb_(*this);
	}
}

std::unique_ptr<AudioEncoder> WebmContainer::make_audio_encoder(AudioEncoder::Config cfg)
{
	const uint64_t tid = segment_.AddAudioTrack(cfg.sample_rate, cfg.channels, 0);

	return std::make_unique<WebmVorbisEncoder>(*this, tid, cfg);
}

std::unique_ptr<VideoEncoder> WebmContainer::make_video_encoder(VideoEncoder::Config cfg)
{
	const uint64_t tid = segment_.AddVideoTrack(cfg.width, cfg.height, 0);

	return std::make_unique<WebmVP8Encoder>(*this, tid, cfg);
}

time_unit_t WebmContainer::duration() const
{
	if (finalized_)
	{
		const auto& si = *segment_.segment_info();

		return webm::duration(static_cast<uint64_t>(si.duration() * si.timecode_scale()));
	}

	auto _ = queue_guard();

	return webm::duration(latest_timestamp_);
}

std::size_t WebmContainer::size() const
{
	if (finalized_)
	{
		return writer_.Position();
	}

	auto _ = queue_guard();

	return writer_.Position() + queue_size_;
}

std::size_t WebmContainer::track_size(webm::track trackid) const
{
	auto _ = queue_guard();

	return queue_.at(trackid).data_size;
}

time_unit_t WebmContainer::track_duration(webm::track trackid) const
{
	auto _ = queue_guard();

	return webm::duration(queue_.at(trackid).flushed_timestamp);
}

void WebmContainer::write_frame(
	tcb::span<const std::byte> buffer,
	webm::track trackid,
	webm::timestamp timestamp,
	bool is_key_frame
)
{
	if (!segment_.AddFrame(
			reinterpret_cast<const uint8_t*>(buffer.data()),
			buffer.size_bytes(),
			trackid,
			timestamp,
			is_key_frame
		))
	{
		throw std::runtime_error(fmt::format(
			"mkvmuxer::Segment::AddFrame, size={}, track={}, ts={}, key={}",
			buffer.size_bytes(),
			trackid,
			timestamp,
			is_key_frame
		));
	}

	queue_[trackid].data_size += buffer.size_bytes();
}

void WebmContainer::queue_frame(
	tcb::span<const std::byte> buffer,
	webm::track trackid,
	webm::timestamp timestamp,
	bool is_key_frame
)
{
	auto _ = queue_guard();

	auto& q = queue_.at(trackid);

	// If another track is behind this one, queue this
	// frame until the other track catches up.

	if (flush_queue() < timestamp)
	{
		q.frames.emplace_back(buffer, timestamp, is_key_frame);
		queue_size_ += buffer.size_bytes();
	}
	else
	{
		// Nothing is waiting; this frame can be written
		// immediately.

		write_frame(buffer, trackid, timestamp, is_key_frame);
		q.flushed_timestamp = timestamp;
	}

	q.queued_timestamp = timestamp;
	latest_timestamp_ = timestamp;
}

webm::timestamp WebmContainer::flush_queue()
{
	webm::timestamp goal = latest_timestamp_;

	// Flush all tracks' queues, not beyond the end of the
	// shortest track.

	for (const auto& [_, q] : queue_)
	{
		if (q.queued_timestamp < goal)
		{
			goal = q.queued_timestamp;
		}
	}

	webm::timestamp shortest;

	do
	{
		shortest = goal;

		for (const auto& [tid, q] : queue_)
		{
			const webm::timestamp flushed = flush_single_queue(tid, q.queued_timestamp);

			if (flushed < shortest)
			{
				shortest = flushed;
			}
		}
	} while (shortest < goal);

	return shortest;
}

webm::timestamp WebmContainer::flush_single_queue(webm::track trackid, webm::timestamp flushed_timestamp)
{
	webm::timestamp goal = flushed_timestamp;

	// Find the lowest timestamp yet flushed from all other
	// tracks. We cannot write a frame beyond this timestamp
	// because PTS must only increase.

	for (const auto& [tid, other] : queue_)
	{
		if (tid != trackid && other.flushed_timestamp < goal)
		{
			goal = other.flushed_timestamp;
		}
	}

	auto& q = queue_.at(trackid);
	auto it = q.frames.cbegin();

	// Flush previously queued frames in this track.

	for (; it != q.frames.cend(); ++it)
	{
		const auto& frame = *it;

		if (frame.timestamp > goal)
		{
			q.flushed_timestamp = frame.timestamp;
			break;
		}

		write_frame(frame.buffer, trackid, frame.timestamp, frame.is_key_frame);

		queue_size_ -= frame.buffer.size();
	}

	q.frames.erase(q.frames.cbegin(), it);

	if (q.frames.empty())
	{
		q.flushed_timestamp = flushed_timestamp;
	}

	return goal;
}
