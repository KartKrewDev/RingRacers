// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_CORE_VEC_HPP
#define SRB2_CORE_VEC_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace srb2
{

class AbstractVector
{
protected:
	size_t size_;
	size_t capacity_;
	size_t elem_size_;
	void* data_;

	using Move = void(*)(void* dst, void* src, size_t size);

	template <typename T>
	static void _move(void* dst, void* src, size_t size) noexcept
	{
		T* d = (T*)dst;
		T* s = (T*)src;
		for (size_t i = 0; i < size; i++)
		{
			new (&d[i]) T(std::move(s[i]));
		}
	}

	struct GrowResult
	{
		void* data;
		size_t cap;
	};
	static GrowResult _realloc_mem(void* data, size_t size, size_t old_cap, size_t elem_size, Move move, size_t cap) noexcept;
	static void _free_mem(void* data, size_t cap, size_t elem_size) noexcept;

	template <typename T>
	void _reserve_mem(size_t c) noexcept
	{
		if (c > capacity_)
		{
			GrowResult r = _realloc_mem(data_, size_, capacity_, elem_size_, _move<T>, c);
			data_ = r.data;
			capacity_ = r.cap;
		}
	}

	constexpr AbstractVector() : size_(0), capacity_(0), elem_size_(0), data_(nullptr) {}
};

template <typename T>
class Vector : AbstractVector
{
public:
	// iter traits
	using value_type = T;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using iterator = T*;
	using const_iterator = const T*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	Vector() : AbstractVector()
	{
		elem_size_ = sizeof(T);
	}
	Vector(const Vector& rhs) : Vector()
	{
		*this = rhs;
	}
	Vector(Vector&& rhs) noexcept : AbstractVector()
	{
		elem_size_ = sizeof(T);
		*this = std::move(rhs);
	}
	~Vector()
	{
		if (data_)
		{
			for (size_type i = 0; i < size_; i++)
			{
				((T*)(data_))[(size_ - i - 1)].~T();
			}
			_free_mem(data_, capacity_, elem_size_);
		}
		data_ = nullptr;
	}

	explicit Vector(size_type capacity) : AbstractVector()
	{
		elem_size_ = sizeof(T);
		_reserve_mem<T>(capacity);
	}
	Vector(std::initializer_list<T> l) : AbstractVector()
	{
		elem_size_ = sizeof(T);
		if (l.size() == 0)
		{
			return;
		}

		_reserve_mem<T>(l.size());
		for (auto itr = l.begin(); itr != l.end(); itr++)
		{
			emplace_back(*itr);
		}
	}

	template <
		typename It,
		typename std::enable_if_t<
			std::is_constructible_v<
				T,
				typename std::iterator_traits<It>::reference
			>,
			int
		> = 0
	>
	Vector(It begin, It end) : AbstractVector()
	{
		elem_size_ = sizeof(T);
		for (auto itr = begin; itr != end; ++itr)
		{
			push_back(*itr);
		}
	}

	Vector& operator=(const Vector& rhs)
	{
		clear();
		for (auto itr = rhs.begin(); itr != rhs.end(); itr++)
		{
			push_back(*itr);
		}
		return *this;
	}

	Vector& operator=(Vector&& rhs) noexcept
	{
		std::swap(size_, rhs.size_);
		std::swap(capacity_, rhs.capacity_);
		std::swap(data_, rhs.data_);
		return *this;
	}

	constexpr size_type size() const noexcept { return size_; }
	constexpr size_type capacity() const noexcept { return capacity_; }
	constexpr bool empty() const noexcept { return size_ == 0; }

	constexpr T* data() noexcept { return (T*) data_; }
	constexpr const T* data() const noexcept { return (const T*) data_; }
	constexpr T* begin() noexcept { return data(); }
	constexpr const T* begin() const noexcept { return data(); }
	constexpr T* end() noexcept { return data() + size(); }
	constexpr const T* end() const noexcept { return data() + size(); }
	constexpr const T* cbegin() const noexcept { return data(); }
	constexpr const T* cend() const noexcept { return end(); }
	constexpr auto rbegin() noexcept { return std::reverse_iterator(end()); }
	constexpr auto rbegin() const noexcept { return std::reverse_iterator(end()); }
	constexpr auto rend() noexcept { return std::reverse_iterator(begin()); }
	constexpr auto rend() const noexcept { return std::reverse_iterator(begin()); }
	constexpr auto crbegin() const noexcept { return rbegin(); }
	constexpr auto crend() const noexcept { return rend(); }
	T& front() noexcept { return data()[0]; }
	const T& front() const noexcept { return data()[0]; }
	T& back() noexcept { return data()[size() - 1]; }
	const T& back() const noexcept { return data()[size() - 1]; }

	void push_back(const T& t)
	{
		reserve(size() + 1);
		new (&data()[size()]) T(t);
		size_++;
	}

	void push_back(T&& t)
	{
		reserve(size() + 1);
		new (&data()[size()]) T(std::move(t));
		size_++;
	}

	void pop_back()
	{
		T* end = &data()[size() - 1];
		end->~T();
		size_--;
	}

	void clear() { for (auto& x : *this) x.~T(); size_ = 0; }

	void reserve(size_type c)
	{
		_reserve_mem<T>(c);
	}

	void resize(size_type s)
	{
		resize(s, T());
	}

	void resize(size_type s, const T& value)
	{
		if (s <= size())
		{
			auto itr_begin = rbegin();
			auto itr_end = std::prev(rend(), s);
			size_t count_destroyed = 0;
			for (auto itr = itr_begin; itr != itr_end; itr++)
			{
				if constexpr (std::is_destructible_v<T>)
				{
					(*itr).~T();
				}
				count_destroyed++;
			}
			size_ = s;
		}
		else
		{
			reserve(s);
			size_type oldsize = size();
			size_ = s;
			for (auto itr = std::next(begin(), oldsize); itr != end(); itr++)
			{
				try
				{
					new (itr) T(value);
				}
				catch (...)
				{
					size_ = oldsize;
					std::rethrow_exception(std::current_exception());
				}
			}
		}
	}

	const T& at(size_type i) const { if (i >= size()) throw std::out_of_range("index out of range"); return data()[i]; }
	T& at(size_type i) { if (i >= size()) throw std::out_of_range("index out of range"); return data()[i]; }

	T& operator[](size_type i) { return data()[i]; }
	const T& operator[](size_type i) const { return data()[i]; }

	iterator erase(const_iterator first) { return erase(first, first + 1); }
	iterator erase(const_iterator first, const_iterator last)
	{
		iterator firstm = const_cast<iterator>(first);
		iterator lastm = const_cast<iterator>(last);
		if (first == last) return firstm;

		auto diff = last - first;
		if (last != end()) std::move(lastm, end(), firstm);
		resize(size_ - diff);

		return firstm;
	}

	iterator insert(const_iterator pos, const T& value)
	{
		return insert(pos, (size_type)1, std::forward<const T&>(value));
	}

	iterator insert(const_iterator pos, T&& value)
	{
		size_type oldsize = size();
		difference_type offs = pos - data();
		push_back(std::move(value));
		std::rotate(data() + offs, data() + oldsize, data() + size());
		return data() + offs;
	}

	iterator insert(const_iterator pos, size_type count, const T& value)
	{
		size_type oldsize = size();
		difference_type offs = pos - data();
		reserve(oldsize + count);
		T* d = data();
		for (uint32_t i = 0; i < count; i++)
		{
			// must be copy-initialized;
			// value is currently uninitialized
			new ((T*)(&d[oldsize + i])) T(value);
		}
		size_ = oldsize + count;
		std::rotate(d + offs, d + oldsize, d + (oldsize + count));
		return data() + offs;
	}

	template <
		class InputIt,
		typename std::enable_if_t<
			std::is_constructible<
				T,
				typename std::iterator_traits<InputIt>::reference
			>::value,
			int
		> = 0
	>
	iterator insert(const_iterator pos, InputIt first, InputIt last)
	{
		size_type oldsize = size();
		difference_type offs = pos - data();

		size_type count = 0;
		while (first != last)
		{
			push_back(*first);
			++first;
			++count;
		}
		std::rotate(data() + offs, data() + oldsize, data() + (oldsize + count));
		return data() + offs;
	}

	template <typename... A>
	iterator emplace(const_iterator position, A&&... args)
	{
		return insert(position, T(std::forward<A>(args)...));
	}

	template <typename... A>
	T& emplace_back(A&&... args)
	{
		reserve(size() + 1);
		new (&data()[size()]) T(std::forward<A>(args)...);
		size_++;
		return (*this)[size_ - 1];
	}
};

class String;

extern template class Vector<bool>;
extern template class Vector<std::byte>;
extern template class Vector<uint8_t>;
extern template class Vector<uint16_t>;
extern template class Vector<uint32_t>;
extern template class Vector<uint64_t>;
extern template class Vector<int8_t>;
extern template class Vector<int16_t>;
extern template class Vector<int32_t>;
extern template class Vector<int64_t>;
extern template class Vector<String>;

} // namespace srb2

#endif // SRB2_CORE_VEC_HPP
