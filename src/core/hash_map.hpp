// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_CORE_HASH_MAP_HPP
#define SRB2_CORE_HASH_MAP_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <utility>

#include "../cxxutil.hpp"

namespace srb2
{

template <typename V>
class HashSet;

template <typename K, typename V>
class HashMap
{
	struct Elem;

	using Hasher = std::hash<K>;
	using KeyEqual = std::equal_to<K>;

public:
	using Entry = std::pair<K, V>;

	class ConstIter;

	class Iter
	{
		const HashMap* self_;
		uint32_t bucket_;
		Elem* cur_;

		Iter(const HashMap* self, uint32_t bucket, Elem* cur)
			: self_(self)
			, bucket_(bucket)
			, cur_(cur)
		{}

		Iter(const ConstIter& r)
			: self_(r.iter_.self_)
			, bucket_(r.iter_.bucket_)
			, cur_(r.iter_.cur_)
		{}

		friend class HashMap;
		friend class HashSet<K>;
		friend class ConstIter;

	public:
		Iter() : Iter(nullptr, 0, nullptr) {}
		Iter(const Iter&) = default;
		Iter(Iter&&) noexcept = default;
		~Iter() = default;
		Iter& operator=(const Iter&) = default;
		Iter& operator=(Iter&&) noexcept = default;

		Entry& operator*() const noexcept { return cur_->entry; }
		Entry* operator->() const noexcept { return &cur_->entry; }
		bool operator==(const Iter& r) const noexcept
		{
			return self_ == r.self_ && bucket_ == r.bucket_ && cur_ == r.cur_;
		}
		bool operator!=(const Iter& r) const noexcept { return !(*this == r); }

		Iter& operator++()
		{
			if (cur_ == nullptr)
			{
				return *this;
			}

			if (cur_->next == nullptr)
			{
				do
				{
					if (bucket_ < self_->buckets_ - 1)
					{
						bucket_++;
						cur_ = self_->heads_[bucket_];
					}
					else
					{
						self_ = nullptr;
						bucket_ = 0;
						cur_ = nullptr;
						return *this;
					}
				} while (cur_ == nullptr);
			}
			else
			{
				cur_ = cur_->next;
			}

			return *this;
		}

		Iter operator++(int)
		{
			Iter itr = *this;
			++(*this);
			return itr;
		}
	};

	class ConstIter
	{
		mutable Iter iter_;

		friend class HashMap;
	public:
		ConstIter() : iter_() {}
		ConstIter(const ConstIter&) = default;
		ConstIter(ConstIter&&) noexcept = default;
		~ConstIter() = default;
		ConstIter& operator=(const ConstIter&) = default;
		ConstIter& operator=(ConstIter&&) noexcept = default;

		ConstIter(const Iter& iter) : iter_(iter) {}

		const Entry& operator*() const noexcept { return iter_.cur_->entry; }
		const Entry* operator->() const noexcept { return &iter_.cur_->entry; }
		bool operator==(const ConstIter& r) const noexcept { return iter_ == r.iter_; }
		bool operator!=(const ConstIter& r) const noexcept { return !(*this == r); }
		ConstIter& operator++() noexcept
		{
			++iter_;
			return *this;
		}
		ConstIter operator++(int) noexcept
		{
			ConstIter itr = *this;
			++*this;
			return itr;
		}
	};

	// iter traits
	using key_type = K;
	using mapped_type = V;
	using value_type = Entry;
	using size_type = uint32_t;
	using difference_type = int64_t;
	using hasher = Hasher;
	using key_equal = KeyEqual;
	using reference = Entry&;
	using const_reference = const Entry&;
	using pointer = Entry*;
	using const_pointer = const Entry*;
	using iterator = Iter;
	using const_iterator = ConstIter;

private:
	struct Elem
	{
		Elem* prev;
		Elem* next;
		Entry entry;
	};

	Elem** heads_ = nullptr;
	size_t size_ = 0;
	uint32_t buckets_;
	Hasher hasher_;
	KeyEqual key_equal_;

	constexpr static uint32_t kDefaultBuckets = 16;

	void init_buckets()
	{
		if (buckets_ == 0)
		{
			buckets_ = kDefaultBuckets;
		}

		std::allocator<Elem*> allocator;
		heads_ = allocator.allocate(buckets_);
		for (uint32_t i = 0; i < buckets_; i++)
		{
			heads_[i] = nullptr;
		}
	}

	void init_if_needed()
	{
		if (heads_ == nullptr)
		{
			init_buckets();
		}
	}

	void rehash_if_needed(size_t target_size)
	{
		if (target_size >= buckets_)
		{
			rehash(std::max<size_t>(target_size, buckets_ * 2));
		}
	}


	friend class Iter;
	friend class ConstIter;

public:
	HashMap() : heads_(nullptr), size_(0), buckets_(0), hasher_(), key_equal_()
	{

	}

	HashMap(uint32_t buckets)
		: heads_(nullptr)
		, size_(0)
		, buckets_(buckets)
		, hasher_()
		, key_equal_()
	{
		init_buckets();
	}

	HashMap(const HashMap& r)
	{
		*this = r;
	}

	HashMap(HashMap&& r) noexcept
	{
		buckets_ = r.buckets_;
		r.buckets_ = 0;
		size_ = r.size_;
		r.size_ = 0;
		heads_ = r.heads_;
		r.heads_ = nullptr;
		hasher_ = r.hasher_;
		r.hasher_ = {};
		key_equal_ = r.key_equal_;
		r.key_equal_ = {};
	};

	~HashMap()
	{
		clear();
	}

	HashMap(std::initializer_list<Entry> list) : HashMap(list.size())
	{
		for (auto v : list)
		{
			insert(v);
		}
	}

	HashMap& operator=(const HashMap& r)
	{
		clear();
		buckets_ = r.buckets_;
		if (buckets_ == 0)
		{
			return *this;
		}

		init_buckets();
		for (auto itr = r.begin(); itr != r.end(); itr++)
		{
			insert({itr->first, itr->second});
		}

		return *this;
	}

	HashMap& operator=(HashMap&& r) noexcept
	{
		std::swap(buckets_, r.buckets_);
		std::swap(size_, r.size_);
		std::swap(heads_, r.heads_);
		std::swap(hasher_, r.hasher_);
		std::swap(key_equal_, r.key_equal_);
		return *this;
	};

	constexpr bool empty() const noexcept { return size_ == 0; }
	constexpr size_t size() const noexcept { return size_; }
	constexpr uint32_t buckets() const noexcept { return buckets_; }

	Iter begin() noexcept
	{
		if (size_ == 0)
		{
			return end();
		}

		Iter ret = end();
		for (uint32_t i = 0; i < buckets_; i++)
		{
			Elem* ptr = heads_[i];
			if (ptr != nullptr)
			{
				ret = Iter { this, i, ptr };
				break;
			}
		}
		return ret;
	}

	ConstIter cbegin() const noexcept
	{
		ConstIter ret = end();
		for (uint32_t i = 0; i < buckets_; i++)
		{
			Elem* ptr = heads_[i];
			if (ptr != nullptr)
			{
				ret = ConstIter { Iter { this, i, ptr } };
				break;
			}
		}
		return ret;
	}

	ConstIter begin() const noexcept { return cbegin(); }

	constexpr Iter end() noexcept { return Iter(); }

	constexpr ConstIter cend() const noexcept { return ConstIter(); }

	constexpr ConstIter end() const noexcept { return cend(); }

	void clear()
	{
		if (heads_)
		{
			std::allocator<Elem> elem_allocator;
			for (uint32_t i = 0; i < buckets_; i++)
			{
				auto itr = heads_[i];
				while (itr != nullptr)
				{
					auto nextitr = itr->next;
					itr->~Elem();
					elem_allocator.deallocate(itr, 1);
					itr = nextitr;
				}
			}

			std::allocator<Elem*> buckets_allocator;
			buckets_allocator.deallocate(heads_, buckets_);
		}
		heads_ = nullptr;
		size_ = 0;
	}

	Iter find(const K& key)
	{
		if (buckets_ == 0 || heads_ == nullptr)
		{
			return end();
		}

		uint32_t bucket = (hasher_)(key) % buckets_;
		for (Elem* p = heads_[bucket]; p != nullptr; p = p->next)
		{
			if ((key_equal_)(p->entry.first, key))
			{
				return Iter { this, bucket, p };
			}
		};
		return end();
	}

	ConstIter find(const K& key) const
	{
		Iter iter = const_cast<HashMap*>(this)->find(key);
		return ConstIter { iter };
	}

	std::pair<Iter, bool> insert(const Entry& value)
	{
		Entry copy = value;
		return insert(std::move(copy));
	}

	void rehash(uint32_t count)
	{
		count = size_ > count ? size_ : count;
		HashMap rehashed { count };
		for (Iter itr = begin(); itr != end();)
		{
			Iter itrcopy = itr;
			++itr;

			uint32_t oldbucket = itrcopy.bucket_;
			if (heads_[oldbucket] == itrcopy.cur_)
			{
				heads_[oldbucket] = nullptr;
			}
			itrcopy.self_ = &rehashed;
			itrcopy.bucket_ = (rehashed.hasher_)(itrcopy.cur_->entry.first) % count;
			Elem* p = rehashed.heads_[itrcopy.bucket_];
			if (p == nullptr)
			{
				p = rehashed.heads_[itrcopy.bucket_] = itrcopy.cur_;
				size_ -= 1;
				rehashed.size_ += 1;

				if (p->next)
				{
					p->next->prev = nullptr;
				}
				if (p->prev)
				{
					p->prev->next = nullptr;
				}
				p->next = nullptr;
				p->prev = nullptr;
			}
			else
			{
				for (; p != nullptr; p = p->next)
				{
					if (p->next != nullptr)
					{
						continue;
					}
					p->next = itrcopy.cur_;
					size_ -= 1;
					rehashed.size_ += 1;
					p->next->prev = p;
					p->next->next = nullptr;
					break;
				}
			}
		}

		if (heads_)
		{
			std::allocator<Elem*> buckets_allocator;
			buckets_allocator.deallocate(heads_, buckets_);
			heads_ = nullptr;
		}

		*this = std::move(rehashed);
	}

	std::pair<Iter, bool> insert(Entry&& value)
	{
		std::pair<Iter, bool> ret { end(), false };
		std::allocator<Elem> allocator;

		init_if_needed();

		rehash_if_needed(size_ + 1);

		uint32_t bucket = (hasher_)(value.first) % buckets_;
		Elem* p = heads_[bucket];
		if (p == nullptr)
		{
			// Make a new slot
			ret.second = true;
			Elem* newslot = allocator.allocate(1);
			newslot->prev = nullptr;
			newslot->next = nullptr;
			heads_[bucket] = newslot;
			new (&newslot->entry) Entry { std::move(value) };
			size_++;
			ret.first = Iter { this, bucket, newslot };
			return ret;
		}

		for(; p != nullptr;)
		{
			if ((key_equal_)(p->entry.first, value.first))
			{
				ret.second = false;
				ret.first = Iter { this, bucket, p };
				return ret;
			}

			if (p->next == nullptr)
			{
				// Make a new slot
				ret.second = true;
				Elem* newslot = allocator.allocate(1);
				newslot->prev = p;
				newslot->next = nullptr;
				p->next = newslot;
				new (&newslot->entry) Entry { std::move(value) };
				size_++;
				ret.first = Iter { this, bucket, newslot };
				return ret;
			}
			p = p->next;
		}

		return ret;
	}

	template <typename M>
	std::pair<Iter, bool> insert_or_assign(const K& key, M&& value)
	{
		K kcopy = key;
		return insert_or_assign(std::move(kcopy), std::forward<M>(value));
	}

	template <typename M>
	std::pair<Iter, bool> insert_or_assign(K&& key, M&& value)
	{
		std::pair<Iter, bool> ret { find(key), false };
		if (ret.first != end())
		{
			ret.first->second = std::forward<M>(value);
		}
		else
		{
			ret = insert({ std::move(key), std::forward<M>(value) });
		}
		return ret;
	}

	template <typename ...Args>
	std::pair<Iter, bool> emplace(Args&&... args)
	{
		Entry entry { std::forward<Args>(args)... };
		return insert(std::move(entry));
	}

	template <typename ...Args>
	std::pair<Iter, bool> try_emplace(const K& key, Args&&... args)
	{
		Iter itr = find(key);
		if (itr != end())
		{
			return { itr, false };
		}

		return insert({ key, V(std::forward<Args>(args)...) });
	}

	template <typename ...Args>
	std::pair<Iter, bool> try_emplace(K&& key, Args... args)
	{
		std::pair<Iter, bool> ret { end(), false };
		return insert({ std::move(key), V(std::forward<Args>(args)...)});
	}

	V& operator[](const K& key)
	{
		K copy { key };
		return this->operator[](std::move(copy));
	}

	V& operator[](K&& key)
	{
		std::allocator<Elem> allocator;
		init_if_needed();

		rehash_if_needed(size_ + 1);

		uint32_t bucket = (hasher_)(key) % buckets_;
		Elem* p = heads_[bucket];
		if (p == nullptr)
		{
			// Make a new slot
			Elem* newslot = allocator.allocate(1);
			newslot->prev = nullptr;
			newslot->next = nullptr;
			heads_[bucket] = newslot;
			new (&newslot->entry) Entry { std::move(key), {} };
			size_++;
			return newslot->entry.second;
		}
		for (; p != nullptr;)
		{
			if ((key_equal_)(p->entry.first, key))
			{
				return p->entry.second;
			}

			if (p->next == nullptr)
			{
				// Make a new slot
				Elem* newslot = allocator.allocate(1);
				newslot->prev = p;
				newslot->next = nullptr;
				p->next = newslot;
				new (&newslot->entry) Entry { std::move(key), {} };
				size_++;
				return newslot->entry.second;
			}
			p = p->next;
		}
		return end()->second;
	}

	V& at(const K& key)
	{
		auto itr = find(key);
		if (itr == end())
		{
			throw std::out_of_range("key not found");
		}
		return itr->second;
	}

	const V& at(const K& key) const
	{
		auto itr = find(key);
		if (itr == cend())
		{
			throw std::out_of_range("key not found");
		}
		return itr->second;
	}

	Iter erase(Iter pos)
	{
		Iter ret = pos;
		if (pos == end())
		{
			return end();
		}

		std::allocator<Elem> allocator;
		uint32_t bucket = (hasher_)(pos.cur_->entry.first) % buckets_;
		Elem* p = heads_[bucket];
		for (; p != nullptr;)
		{
			if ((key_equal_)(p->entry.first, pos.cur_->entry.first))
			{
				// found it; remove, return next iter
				++ret;
				if (p->next != nullptr)
				{
					p->next->prev = p->prev;
				}
				if (p->prev != nullptr)
				{
					p->prev->next = p->next;
				}
				if (p == heads_[bucket])
				{
					heads_[bucket] = p->next;
				}
				p->~Elem();
				allocator.deallocate(p, 1);
				size_ -= 1;
				return ret;
			}
			p = p->next;
		}
		return ret;
	}

	Iter erase(ConstIter pos)
	{
		return erase(pos.iter_);
	}

	uint32_t erase(const K& key)
	{
		Iter pos = find(key);
		if (pos != end())
		{
			erase(pos);
			return 1;
		}
		return 0;
	}
};

} // namespace srb2

#endif // SRB2_CORE_HASH_MAP_HPP
