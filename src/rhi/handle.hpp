// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_RHI_HANDLE_HPP__
#define __SRB2_RHI_HANDLE_HPP__

#include <atomic>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "../cxxutil.hpp"

namespace srb2::rhi
{

struct NullHandleType
{
};

constexpr const NullHandleType kNullHandle = NullHandleType {};

template <typename T = void>
class Handle
{
	uint32_t id_;
	uint32_t generation_;

public:
	Handle(uint32_t id, uint32_t generation) noexcept : id_(id), generation_(generation) {}
	Handle(uint64_t combined) noexcept : id_(combined & 0xFFFFFFFF), generation_((combined & 0xFFFFFFFF00000000) >> 32)
	{
	}
	Handle() noexcept : Handle(0, 0) {}

	Handle(NullHandleType) noexcept : Handle() {}

	Handle(const Handle&) noexcept = default;
	Handle(Handle&&) noexcept = default;

	Handle& operator=(const Handle&) noexcept = default;
	Handle& operator=(Handle&&) noexcept = default;

	// Conversions from Handles of derived type U to base type T

	template <typename U, typename std::enable_if_t<std::is_base_of_v<T, U>, bool> = true>
	Handle(const Handle<U>& rhs) noexcept : id_(rhs.id()), generation_(rhs.generation())
	{
	}

	template <typename U, typename std::enable_if_t<std::is_base_of_v<T, U>, bool> = true>
	Handle& operator=(const Handle<U>& rhs) noexcept
	{
		id_ = rhs.id_;
		generation_ = rhs.generation_;
	}

	uint32_t id() const noexcept { return id_; }
	uint32_t generation() const noexcept { return generation_; }

	/// @return true if this Handle is valid (belonging to a generation > 0); false otherwise
	bool valid() const noexcept { return generation_ != 0; }

	bool operator==(const Handle& handle) const noexcept
	{
		return handle.generation_ == generation_ && handle.id_ == id_;
	}
	bool operator!=(const Handle& handle) const noexcept { return !(handle == *this); }
	bool operator==(const NullHandleType&) const noexcept { return generation_ == 0; }
	bool operator!=(const NullHandleType&) const noexcept { return generation_ != 0; }

	operator bool() const { return id_ != 0 || generation_ != 0; }
	operator uint64_t() const { return static_cast<uint64_t>(id_) + (static_cast<uint64_t>(generation_) << 32); }
};

template <typename T>
inline bool operator==(const Handle<T>& lhs, const NullHandleType&) noexcept
{
	return lhs.generation() == 0 || lhs.id() == 0;
}

template <typename T>
inline bool operator==(const Handle<T>& lhs, const std::nullptr_t&) noexcept
{
	return lhs.generation() == 0 || lhs.id() == 0;
}

// Non-member equality of base Handle<T> to derived Handle<U>

template <typename T, typename U, typename std::enable_if_t<std::is_base_of_v<T, U>, bool> = true>
inline bool operator==(const Handle<T>& lhs, const Handle<U>& rhs) noexcept
{
	return lhs.generation() == rhs.generation() && lhs.id() == rhs.id();
}

template <typename T, typename U, typename std::enable_if_t<std::is_base_of_v<T, U>, bool> = true>
inline bool operator!=(const Handle<T>& lhs, const Handle<U>& rhs) noexcept
{
	return !(lhs == rhs);
}

template <typename T = void>
class HandlePool
{
	std::atomic_uint32_t current_id_ {0};
	std::atomic_uint32_t current_gen_ {1};

public:
	HandlePool() = default;
	HandlePool(const HandlePool& rhs) = delete;
	HandlePool(HandlePool&& rhs) = default;

	HandlePool& operator=(const HandlePool& rhs) = delete;
	HandlePool& operator=(HandlePool&& rhs) = default;

	/// @brief Create a new unique Handle in the current generation.
	/// @return the new Handle.
	Handle<T> create() noexcept
	{
		const uint32_t id = current_id_.fetch_add(1);
		SRB2_ASSERT(id != UINT32_MAX);
		const uint32_t gen = current_gen_.load();
		return Handle<T>(id, gen);
	}

	/// @brief Increment the generation. All handles created after this will belong to a new generation.
	void generation() noexcept
	{
		const uint32_t old_gen = current_gen_.fetch_add(1);
		SRB2_ASSERT(old_gen != UINT32_MAX);
	}
};

template <typename T>
class Slab;

template <typename T>
class SlabIterator
{
	size_t index_ = 0;
	const Slab<std::remove_const_t<T>>* slab_ = nullptr;

	SlabIterator(size_t index, const Slab<std::remove_const_t<T>>* slab) : index_(index), slab_(slab) {}

	friend Slab<std::remove_const_t<T>>;

public:
	SlabIterator() = default;
	SlabIterator(const SlabIterator&) = default;
	SlabIterator(SlabIterator&&) = default;

	SlabIterator& operator=(const SlabIterator&) = default;
	SlabIterator& operator=(SlabIterator&&) = default;

	T& operator*() const noexcept { return slab_->vec_[index_].item; }

	SlabIterator& operator++() noexcept
	{
		index_++;
		return *this;
	}

	SlabIterator& operator--() noexcept
	{
		index_--;
		return *this;
	}

	SlabIterator operator++(int) noexcept
	{
		index_++;
		return SlabIterator {index_ - 1, slab_};
	}

	SlabIterator operator--(int) noexcept
	{
		index_--;
		return SlabIterator {index_ + 1, slab_};
	}

	bool operator==(const SlabIterator& rhs) const noexcept { return slab_ == rhs.slab_ && index_ == rhs.index_; }

	bool operator!=(const SlabIterator& rhs) const noexcept { return !(*this == rhs); }
};

template <typename T>
class Slab
{
	struct SlabStorage
	{
		T item;
		uint32_t gen;
	};
	std::vector<SlabStorage> vec_;
	std::vector<uint32_t> free_list_;
	uint32_t gen_ = 1;

	friend SlabIterator<T>;
	friend SlabIterator<const T>;

public:
	Slab() = default;
	Slab(const Slab&) = delete;
	Slab& operator=(const Slab&) = delete;

	Handle<T> insert(T&& value)
	{
		uint32_t ret_id = 0;
		if (!free_list_.empty())
		{
			ret_id = free_list_.back();
			free_list_.pop_back();
			SlabStorage& storage = vec_[ret_id];
			storage.item = std::move(value);
			storage.gen = gen_;
		}
		else
		{
			ret_id = vec_.size();
			vec_.push_back(SlabStorage {std::move(value), gen_});
		}
		return Handle<T>(ret_id, gen_);
	}

	template <typename U, typename std::enable_if_t<std::is_base_of_v<U, T>, bool> = true>
	T remove(Handle<U> handle)
	{
		uint32_t handle_id = handle.id();
		uint32_t handle_gen = handle.generation();
		if (handle_id >= vec_.size())
		{
			return T();
		}
		SlabStorage& storage = vec_[handle_id];
		if (storage.gen > handle_gen)
		{
			return T();
		}
		T ret = std::move(storage.item);
		storage.item = T();
		free_list_.push_back(handle_id);
		gen_ += 1;
		if (gen_ == 0)
		{
			gen_ = 1;
		}
		return ret;
	}

	template <typename U, typename std::enable_if_t<std::is_base_of_v<U, T>, bool> = true>
	bool is_valid(Handle<U> handle)
	{
		uint32_t handle_id = handle.id();
		uint32_t handle_gen = handle.generation();
		if (handle_id >= vec_.size())
		{
			return false;
		}
		SlabStorage& storage = vec_[handle_id];
		if (storage.gen > handle_gen)
		{
			return false;
		}
		return true;
	}

	void clear()
	{
		vec_.clear();
		free_list_.clear();
		gen_ += 1;
		if (gen_ == 0)
		{
			gen_ = 1;
		}
	}

	template <typename U, typename std::enable_if_t<std::is_base_of_v<U, T>, bool> = true>
	T& operator[](Handle<U> handle)
	{
		SRB2_ASSERT(is_valid(handle));
		return vec_[handle.id()].item;
	}

	SlabIterator<T> begin() { return SlabIterator<T> {0, this}; }

	SlabIterator<T> end() { return SlabIterator<T> {vec_.size(), this}; }

	SlabIterator<const T> cbegin() const { return SlabIterator<const T> {0, this}; }

	SlabIterator<const T> cend() const { return SlabIterator<const T> {vec_.size(), this}; }
};

} // namespace srb2::rhi

namespace std
{

template <typename T>
struct hash<srb2::rhi::Handle<T>>
{
	std::size_t operator()(const srb2::rhi::Handle<T>& e) const
	{
		return std::hash<uint32_t>()(e.generation()) ^ (std::hash<uint32_t>()(e.id()) << 1);
	}
};

} // namespace std

#endif // __SRB2_RHI_HANDLE_HPP__
