// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_CORE_HASH_SET_HPP
#define SRB2_CORE_HASH_SET_HPP

#include <cstdint>
#include <string>

#include "hash_map.hpp"
#include "string.h"

namespace srb2
{

template <typename V>
class HashSet
{
	using Inner = HashMap<V, uint8_t>;

public:
	class Iter
	{
		typename Inner::Iter iter_;

		Iter(typename Inner::Iter iter) : iter_(iter) {}

		friend class HashSet;
		friend class ConstIter;

	public:
		Iter() = default;
		Iter(const Iter&) = default;
		Iter(Iter&&) noexcept = default;
		~Iter() = default;

		Iter& operator=(const Iter&) = default;
		Iter& operator=(Iter&&) noexcept = default;

		bool operator==(const Iter& r) const noexcept { return iter_ == r.iter_; }
		bool operator!=(const Iter& r) const noexcept { return !(*this == r); }

		V& operator*() const noexcept { return iter_->first; }
		V* operator->() const noexcept { return &iter_->first; }
		Iter& operator++() noexcept
		{
			++iter_;
			return *this;
		}
		Iter operator++(int) noexcept
		{
			Iter self = *this;
			++*this;
			return self;
		}
	};

	class ConstIter
	{
		typename Inner::ConstIter iter_;

		friend class HashSet;
		friend class Iter;

	public:
		ConstIter() = default;
		ConstIter(const ConstIter&) = default;
		ConstIter(ConstIter&&) noexcept = default;
		~ConstIter() = default;

		ConstIter(const Iter& iter) : iter_(iter.iter_) {}
		ConstIter(const typename Inner::ConstIter& iter) : iter_(iter) {}

		ConstIter& operator=(const ConstIter&) = default;
		ConstIter& operator=(ConstIter&&) noexcept = default;

		bool operator==(const ConstIter& r) const noexcept { return iter_ == r.iter_; }
		bool operator!=(const ConstIter& r) const noexcept { return !(*this == r); }

		const V& operator*() const noexcept { return iter_->first; }
		const V* operator->() const noexcept { return &iter_->first; }
		ConstIter& operator++() noexcept
		{
			++iter_;
			return *this;
		}
		ConstIter operator++(int) noexcept
		{
			ConstIter self = *this;
			++*this;
			return self;
		}
	};

private:
	Inner map_;

public:
	using key_type = V;
	using value_type = V;
	using size_type = typename Inner::size_type;
	using difference_type = typename Inner::difference_type;
	using hasher = typename Inner::hasher;
	using key_equal = typename Inner::key_equal;
	using reference = V&;
	using const_reference = const V&;
	using pointer = V*;
	using const_pointer = const V*;
	using iterator = Iter;
	using const_iterator = ConstIter;

	HashSet() = default;
	HashSet(const HashSet&) = default;
	HashSet(HashSet&&) noexcept = default;
	~HashSet() = default;

	HashSet& operator=(const HashSet&) = default;
	HashSet& operator=(HashSet&&) noexcept = default;

	Iter begin() { return map_.begin(); }
	Iter end() { return map_.end(); }
	ConstIter cbegin() const { return map_.cbegin(); }
	ConstIter cend() const { return map_.cend(); }
	ConstIter begin() const { return cbegin(); }
	ConstIter end() const { return cend(); }

	bool empty() const noexcept { return map_.empty(); }
	size_t size() const noexcept { return map_.size(); }

	void clear() { map_.clear(); }

	std::pair<Iter, bool> insert(const V& value)
	{
		V copy { value };
		return insert(std::move(copy));
	}

	std::pair<Iter, bool> insert(V&& value)
	{
		return emplace(std::move(value));
	}

	template <typename ...Args>
	std::pair<Iter, bool> emplace(Args&&... args)
	{
		std::pair<Iter, bool> res;
		auto [map_iter, map_inserted] = map_.emplace(std::forward<Args&&>(args)..., 0);
		res.first = map_iter;
		res.second = map_inserted;
		return res;
	}

	Iter erase(Iter pos)
	{
		auto map_ret = map_.erase(pos.iter_);
		return map_ret;
	}

	Iter erase(ConstIter pos)
	{
		auto map_ret = map_.erase(pos.iter_);
		return map_ret;
	}

	Iter erase(ConstIter first, ConstIter last)
	{
		ConstIter iter;
		for (iter = first; iter != last;)
		{
			iter = erase(iter);
		}
		return typename Inner::Iter(iter.iter_);
	}

	Iter find(const V& value)
	{
		auto map_ret = map_.find(value);
		return map_ret;
	}

	ConstIter find(const V& value) const
	{
		auto map_ret = map_.find(value);
		return map_ret;
	}

	void rehash(size_t count)
	{
		map_.rehash(count);
	}
};

extern template class HashSet<bool>;
extern template class HashSet<uint8_t>;
extern template class HashSet<uint16_t>;
extern template class HashSet<uint32_t>;
extern template class HashSet<uint64_t>;
extern template class HashSet<int8_t>;
extern template class HashSet<int16_t>;
extern template class HashSet<int32_t>;
extern template class HashSet<int64_t>;
extern template class HashSet<String>;
extern template class HashSet<std::string>;

} // namespace srb2

#endif // SRB2_CORE_HASH_SET_HPP
