// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef __SRB2_CORE_STATIC_VEC_HPP__
#define __SRB2_CORE_STATIC_VEC_HPP__

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>

#include "../cxxutil.hpp"

namespace srb2
{

template <typename T, size_t Limit>
class StaticVec;

template <typename T, size_t Limit>
class StaticVec
{
	std::array<T, Limit> arr_ {{}};
	size_t size_ = 0;

public:
	using value_type = T;

	constexpr StaticVec() {}

	StaticVec(const StaticVec& rhs)
	{
		for (size_t i = size_; i > 0; i--)
		{
			arr_[i] = T();
		}
		size_ = rhs.size();
		for (size_t i = 0; i < size_; i++)
		{
			arr_[i] = rhs.arr_[i];
		}
	}

	StaticVec(StaticVec&& rhs) noexcept(std::is_nothrow_move_assignable_v<T>)
	{
		for (size_t i = size_; i > 0; i--)
		{
			arr_[i] = T();
		}
		size_ = rhs.size();
		for (size_t i = 0; i < size_; i++)
		{
			arr_[i] = std::move(rhs.arr_[i]);
		}
		while (rhs.size() > 0)
		{
			rhs.pop_back();
		}
	}

	constexpr StaticVec(std::initializer_list<T> list) noexcept(std::is_nothrow_move_assignable_v<T>)
	{
		size_ = list.size();
		size_t i = 0;
		for (auto itr = list.begin(); itr != list.end(); itr++)
		{
			arr_[i] = *itr;
			i++;
		}
	}

	~StaticVec() = default;

	StaticVec& operator=(const StaticVec& rhs)
	{
		for (size_t i = size_; i > 0; i--)
		{
			arr_[i] = T();
		}
		size_ = rhs.size();
		for (size_t i = 0; i < size_; i++)
		{
			arr_[i] = rhs.arr_[i];
		}
		return *this;
	}

	StaticVec& operator=(StaticVec&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
	{
		for (size_t i = size_; i > 0; i--)
		{
			arr_[i] = T();
		}
		size_ = rhs.size();
		for (size_t i = 0; i < size_; i++)
		{
			arr_[i] = std::move(rhs.arr_[i]);
		}
		while (rhs.size() > 0)
		{
			rhs.pop_back();
		}
		return *this;
	}

	void push_back(const T& value) { arr_[size_++] = value; }

	void pop_back() { arr_[--size_] = T(); }

	void resize(size_t size, T value = T())
	{
		if (size >= Limit)
		{
			throw std::length_error("new size >= Capacity");
		}

		if (size == size_)
		{
			return;
		}
		else if (size < size_)
		{
			while (size_ > size)
			{
				pop_back();
			}
		}
		else
		{
			while (size_ < size)
			{
				push_back(value);
			}
		}
	}

	void clear()
	{
		arr_ = {{}};
		size_ = 0;
	}

	using iterator = typename std::array<T, Limit>::iterator;
	using const_iterator = typename std::array<T, Limit>::const_iterator;

	constexpr iterator begin() noexcept { return arr_.begin(); }

	constexpr const_iterator begin() const noexcept { return arr_.cbegin(); }

	constexpr const_iterator cbegin() const noexcept { return arr_.cbegin(); }

	constexpr iterator end() noexcept { return std::next(arr_.begin(), size_); }

	constexpr const_iterator end() const noexcept { return cend(); }

	constexpr const_iterator cend() const noexcept { return std::next(arr_.cbegin(), size_); }

	constexpr std::reverse_iterator<iterator> rbegin() noexcept { return std::reverse_iterator(this->end()); }

	constexpr std::reverse_iterator<const_iterator> crbegin() const noexcept { return std::reverse_iterator(this->cend()); }

	constexpr std::reverse_iterator<iterator> rend() noexcept { return std::reverse_iterator(this->begin()); }

	constexpr std::reverse_iterator<const_iterator> crend() const noexcept { return std::reverse_iterator(this->cbegin()); }

	constexpr bool empty() const noexcept { return size_ == 0; }

	constexpr size_t size() const noexcept { return size_; }

	constexpr size_t capacity() const noexcept { return Limit; }

	constexpr size_t max_size() const noexcept { return Limit; }

	constexpr T& operator[](size_t index) noexcept { return arr_[index]; }

	T& at(size_t index)
	{
		if (index >= size_)
		{
			throw std::out_of_range("index >= size");
		}
		return this[index];
	}

	constexpr const T& operator[](size_t index) const noexcept { return arr_[index]; }

	const T& at(size_t index) const
	{
		if (index >= size_)
		{
			throw std::out_of_range("index >= size");
		}
		return this[index];
	}

	T& front() { return *arr_[0]; }

	T& back() { return *arr_[size_ - 1]; }
};

} // namespace srb2

template <typename T, size_t L1, size_t L2>
bool operator==(const srb2::StaticVec<T, L1>& lhs, const srb2::StaticVec<T, L2>& rhs)
{
	const size_t size = lhs.size();
	if (size != rhs.size())
	{
		return false;
	}
	for (size_t i = 0; i < lhs; i++)
	{
		if (rhs[i] != lhs[i])
		{
			return false;
		}
	}
	return true;
}

template <typename T, size_t L1, size_t L2>
bool operator!=(const srb2::StaticVec<T, L1>& lhs, const srb2::StaticVec<T, L2>& rhs)
{
	return !(lhs == rhs);
}

template <typename T, size_t Limit>
struct std::hash<srb2::StaticVec<T, Limit>>
{
	size_t operator()(const srb2::StaticVec<T, Limit>& input) const
	{
		constexpr size_t prime = sizeof(size_t) == 8 ? 0x00000100000001B3 : 0x01000193;
		constexpr size_t basis = sizeof(size_t) == 8 ? 0xcbf29ce484222325 : 0x811c9dc5;

		size_t ret = basis;
		for (auto itr = input.begin(); itr != input.end(); itr++)
		{
			ret = (ret * prime) ^ std::hash<T>(*itr);
		}
		return ret;
	}
};

#endif // __SRB2_CORE_STATIC_VEC_HPP__
