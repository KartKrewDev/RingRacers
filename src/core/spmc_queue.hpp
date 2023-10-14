/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// This class is derived from Conor Williams' implementation of a concurrent deque
// https://github.com/ConorWilliams/ConcurrentDeque
// Copyright (C) 2021 Conor Williams

// The original version was for C++23. This one has been shrunk slightly and adapted to our conventions.

#ifndef __SRB2_CORE_SPMC_QUEUE_HPP__
#define __SRB2_CORE_SPMC_QUEUE_HPP__

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>

#include <tracy/tracy/Tracy.hpp>

#include "../cxxutil.hpp"

namespace srb2
{

template <typename T>
class SpMcQueue
{
	struct RingBuff {
	public:
		explicit RingBuff(int64_t cap) : _cap{cap}, _mask{cap - 1}
		{
			SRB2_ASSERT(cap && (!(cap & (cap - 1))) && "Capacity must be buf power of 2!");
			_buff = std::unique_ptr<T[]>(new T[_cap]);
		}

		std::int64_t capacity() const noexcept { return _cap; }

		// Store (copy) at modulo index
		void store(int64_t i, T&& x) noexcept
		{
			_buff[i & _mask] = std::move(x);
		}

		// Load (copy) at modulo index
		T load(int64_t i) const noexcept
		{
			return _buff[i & _mask];
		}

		// Allocates and returns a new ring buffer, copies elements in range [b, t) into the new buffer.
		RingBuff* resize(std::int64_t b, std::int64_t t) const
		{
			ZoneScoped;
			RingBuff* ptr = new RingBuff{2 * _cap};
			for (std::int64_t i = t; i != b; ++i) {
				ptr->store(i, load(i));
			}
			return ptr;
		}

	private:
		int64_t _cap;   // Capacity of the buffer
		int64_t _mask;  // Bit mask to perform modulo capacity operations

		std::unique_ptr<T[]> _buff;
	};

	alignas(64) std::atomic<int64_t> bottom_;
	alignas(64) std::atomic<int64_t> top_;
	alignas(64) std::atomic<RingBuff*> buffer_;

	std::vector<std::unique_ptr<RingBuff>> garbage_;

public:
	SpMcQueue(size_t capacity) : bottom_(0), top_(0), buffer_(new RingBuff(capacity))
	{
		garbage_.reserve(32);
	}
	~SpMcQueue() noexcept
	{
		delete buffer_.load(std::memory_order_relaxed);
	};

	size_t size() const noexcept
	{
		int64_t bottom = bottom_.load(std::memory_order_relaxed);
		int64_t top = top_.load(std::memory_order_relaxed);
		return static_cast<size_t>(bottom >= top ? bottom - top : 0);
	}

	bool empty() const noexcept
	{
		return size() == 0;
	}

	void push(T&& v) noexcept
	{
		int64_t bottom = bottom_.load(std::memory_order_relaxed);
		int64_t top = top_.load(std::memory_order_acquire);
		RingBuff* buf = buffer_.load(std::memory_order_relaxed);

		if (buf->capacity() < (bottom - top) + 1)
		{
			// Queue is full, build a new one
			RingBuff* newbuf = buf->resize(bottom, top);
			garbage_.emplace_back(buf);
			buffer_.store(newbuf, std::memory_order_relaxed);
			buf = newbuf;
		}

		// Construct new object, this does not have to be atomic as no one can steal this item until after we
		// store the new value of bottom, ordering is maintained by surrounding atomics.
		buf->store(bottom, std::move(v));

		std::atomic_thread_fence(std::memory_order_release);
		bottom_.store(bottom + 1, std::memory_order_relaxed);
	}

	std::optional<T> pop() noexcept
	{
		int64_t bottom = bottom_.load(std::memory_order_relaxed) - 1;
		RingBuff* buf = buffer_.load(std::memory_order_relaxed);

		bottom_.store(bottom, std::memory_order_relaxed);  // Stealers can no longer steal

		std::atomic_thread_fence(std::memory_order_seq_cst);
		int64_t top = top_.load(std::memory_order_relaxed);

		if (top <= bottom)
		{
			// Non-empty deque
			if (top == bottom)
			{
				// The last item could get stolen, by a stealer that loaded bottom before our write above
				if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
				{
					// Failed race, thief got the last item.
					bottom_.store(bottom + 1, std::memory_order_relaxed);
					return std::nullopt;
				}
				bottom_.store(bottom + 1, std::memory_order_relaxed);
			}

			// Can delay load until after acquiring slot as only this thread can push(), this load is not
			// required to be atomic as we are the exclusive writer.
			return buf->load(bottom);

		}
		else
		{
			bottom_.store(bottom + 1, std::memory_order_relaxed);
			return std::nullopt;
		}
	}

	std::optional<T> steal() noexcept
	{
		int64_t top = top_.load(std::memory_order_acquire);
		std::atomic_thread_fence(std::memory_order_seq_cst);
		int64_t bottom = bottom_.load(std::memory_order_acquire);

		if (top < bottom)
		{
			// Must load *before* acquiring the slot as slot may be overwritten immediately after acquiring.
			// This load is NOT required to be atomic even-though it may race with an overrite as we only
			// return the value if we win the race below garanteeing we had no race during our read. If we
			// loose the race then 'x' could be corrupt due to read-during-write race but as T is trivially
			// destructible this does not matter.
			T x = buffer_.load(std::memory_order_consume)->load(top);

			if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
			{
				return std::nullopt;
			}

			return x;

		}
		else
		{
			return std::nullopt;
		}
	}
};

} // namespace srb2

#endif // __SRB2_CORE_SPMC_QUEUE_HPP__
