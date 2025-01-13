// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_CORE_STRING_HPP
#define SRB2_CORE_STRING_HPP

#ifdef __cplusplus

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>

#include "fmt/core.h"

#include "static_vec.hpp"
#include "vector.hpp"

namespace srb2
{

class Utf8Iter
{
public:
	using difference_type = size_t;
	using value_type = uint32_t;
	using pointer = void;
	using reference = void;
	using iterator_category = std::input_iterator_tag;

private:
	size_t i_;
	std::string_view s_;

	Utf8Iter(std::string_view s, size_t i) : i_(i), s_(s) {}

	friend class String;
	uint32_t do_codepoint() const;

public:
	Utf8Iter() = default;
	Utf8Iter(const Utf8Iter&) = default;
	Utf8Iter(Utf8Iter&&) noexcept = default;
	~Utf8Iter() = default;
	Utf8Iter& operator=(const Utf8Iter&) = default;
	Utf8Iter& operator=(Utf8Iter&&) noexcept = default;

	static Utf8Iter begin(std::string_view s) { return Utf8Iter(s, 0); }
	static Utf8Iter end(std::string_view s) { return Utf8Iter(s, s.size()); }

	bool operator==(const Utf8Iter& r) const noexcept
	{
		return s_ == r.s_ && i_ == r.i_;
	}

	bool operator!=(const Utf8Iter& r) const noexcept { return !(*this == r); }

	uint32_t operator*() const { return codepoint(); }
	Utf8Iter& operator++()
	{
		i_ += size();
		return *this;
	}
	Utf8Iter operator++(int)
	{
		Utf8Iter copy = *this;
		++*this;
		return copy;
	}
	uint32_t codepoint() const;
	bool valid() const;
	uint8_t size() const;
};

class Utf16Iter
{
public:
	using difference_type = size_t;
	using value_type = uint32_t;
	using pointer = void;
	using reference = void;
	using iterator_category = std::input_iterator_tag;

private:
	size_t i_;
	std::u16string_view s_;

	Utf16Iter(std::u16string_view s, size_t i) : i_(i), s_(s) {}

	uint32_t do_codepoint() const;

public:
	Utf16Iter() = default;
	Utf16Iter(const Utf16Iter&) = default;
	Utf16Iter(Utf16Iter&&) noexcept = default;
	~Utf16Iter() = default;
	Utf16Iter& operator=(const Utf16Iter&) = default;
	Utf16Iter& operator=(Utf16Iter&&) = default;

	static Utf16Iter begin(std::u16string_view s) { return Utf16Iter(s, 0); }
	static Utf16Iter end(std::u16string_view s) { return Utf16Iter(s, s.size()); }

	bool operator==(const Utf16Iter& r) const noexcept
	{
		return s_ == r.s_ && i_ == r.i_;
	}

	bool operator!=(const Utf16Iter& r) const noexcept { return !(*this == r); }

	uint32_t operator*() const { return codepoint(); }
	Utf16Iter& operator++()
	{
		i_ += size();
		return *this;
	}
	Utf16Iter operator++(int)
	{
		Utf16Iter copy = *this;
		++*this;
		return copy;
	}

	uint32_t codepoint() const;
	bool valid() const { return true; } // we allow unpaired surrogates in general
	uint8_t size() const;
};

class String
{
public:
	using size_type = uint32_t;
	using difference_type = int64_t;
	using value_type = uint8_t;
	using reference = uint8_t&;
	using const_reference = const uint8_t&;
	using pointer = uint8_t*;
	using const_pointer = const uint8_t*;
	using iterator = uint8_t*;
	using const_iterator = const uint8_t*;

private:
	srb2::Vector<uint8_t> data_;

public:

	friend struct std::hash<String>;

	static constexpr const size_type npos = -1;

	String() = default;
	String(const String&);
	String(String&&) noexcept;
	~String();
	String& operator=(const String&);
	String& operator=(String&&) noexcept;

	String(const char* s);
	String(const char* s, size_t len);
	String(const std::string&);

	explicit String(std::string_view view);

	operator std::string() const;
	operator std::string_view() const;

	size_type size() const noexcept;
	bool empty() const noexcept { return data_.empty(); }
	const char* c_str() const;
	uint8_t* data() noexcept { return data_.data(); }
	const uint8_t* data() const noexcept { return data_.data(); }
	void reserve(size_type capacity);

	uint8_t* begin() noexcept;
	uint8_t* end() noexcept;
	const uint8_t* cbegin() const noexcept;
	const uint8_t* cend() const noexcept;
	const uint8_t* begin() const noexcept { return cbegin(); }
	const uint8_t* end() const noexcept { return cend(); }

	uint8_t& at(size_type i);
	const uint8_t& at(size_type i) const;
	uint8_t& operator[](size_type i) { return data_[i]; }
	const uint8_t& operator[](size_type i) const { return data_[i]; }
	uint8_t& front() { return data_[0]; }
	const uint8_t& front() const { return data_[0]; }
	uint8_t& back() { return data_[size() - 1]; }
	const uint8_t& back() const { return data_[size() - 1]; }

	void clear() { data_.clear(); }
	String& insert(size_type index, size_type count, uint8_t ch);
	String& insert(size_type index, const char* s);
	String& insert(size_type index, const char* s, size_type count);
	String& insert(size_type index, std::string_view str);
	String& insert(size_type index, std::string_view str, size_t s_index, size_t count = npos);
	iterator insert(const_iterator pos, uint8_t ch);
	iterator insert(const_iterator pos, size_type count, uint8_t ch);

	template <
		typename InputIt,
		typename std::enable_if_t<
			std::is_constructible<
				uint8_t,
				typename std::iterator_traits<InputIt>::reference
			>::value,
			int
		> = 0
	>
	iterator insert(const_iterator pos, InputIt first, InputIt last)
	{
		size_type offset = pos - data();
		if (!empty())
		{
			// remove null byte
			data_.pop_back();
		}
		auto ret = data_.insert(pos, first, last);
		if (data_.size() > 0)
		{
			data_.push_back(0);
		}
		return data() + offset;
	}

	iterator insert(const_iterator pos, std::initializer_list<uint8_t> list);
	String& erase(size_type index = 0, size_type count = npos);
	iterator erase(const_iterator position);
	iterator erase(const_iterator first, const_iterator last);
	void push_back(uint8_t v);
	void pop_back();
	String& append(size_type count, uint8_t ch);
	String& append(const char* s, size_type count);
	String& append(const char* s);
	String& append(std::string_view str);
	String& append(std::string_view str, size_type pos, size_type count = npos);

	template <
		typename InputIt,
		typename std::enable_if_t<
			std::is_constructible<
				uint8_t,
				typename std::iterator_traits<InputIt>::reference
			>::value,
			int
		> = 0
	>
	String& append(InputIt first, InputIt last)
	{
		if (!empty())
		{
			// remove null byte
			data_.pop_back();
		}
		for (; first != last; first++)
		{
			data_.push_back(*first);
		}
		if (data_.size() > 0)
		{
			data_.push_back(0);
		}
		return *this;
	}

	String& append(std::initializer_list<uint8_t> ilist);
	String& operator+=(std::string_view r);
	String& operator+=(const char* r);
	String& operator+=(uint8_t r);
	String& operator+=(std::initializer_list<uint8_t> r);
	String& replace(size_type pos, size_type count, std::string_view str);
	String& replace(const_iterator first, const_iterator last, std::string_view str);
	String& replace(size_type pos, size_type count, std::string_view str, size_t pos2, size_t count2 = -1);
	String& replace(size_type pos, size_type count, const char* cstr, size_type count2);
	String& replace(const_iterator first, const_iterator last, const char* cstr, size_type count2);
	String& replace(size_type pos, size_type count, const char* cstr);
	String& replace(const_iterator first, const_iterator last, const char* cstr);
	// String& replace(size_type pos, size_type count, size_type count2, uint8_t ch);
	String& replace(const_iterator first, const_iterator last, uint8_t ch);

	template <
		typename InputIt,
		typename std::enable_if_t<
			std::is_constructible<
				uint8_t,
				typename std::iterator_traits<InputIt>::reference
			>::value,
			int
		> = 0
	>
	String& replace(const_iterator first, const_iterator last, InputIt first2, InputIt last2)
	{
		for (; first != last && first2 != last2; first++, first2++)
		{
			*const_cast<iterator>(first) = *first2;
		}
		return *this;
	}

	String& replace(const_iterator first, const_iterator last, std::initializer_list<uint8_t> ilist);
	size_type copy(uint8_t* dest, size_type count, size_type pos = 0) const;
	size_type copy(char* dest, size_type count, size_type pos = 0) const;
	void resize(size_type count);
	void resize(size_type count, uint8_t ch);
	void swap(String& other) noexcept;

	size_type find(const String& str, size_type pos = 0) const;
	size_type find(std::string_view str, size_type pos = 0) const;
	size_type find(const char* s, size_type pos, size_t count) const;
	size_type find(const char* s, size_type pos = 0) const;
	size_type find(uint8_t ch, size_type pos = 0) const;
	size_type rfind(const String& str, size_type pos = npos) const;
	size_type rfind(std::string_view str, size_type pos = npos) const;
	size_type rfind(const char* s, size_type pos, size_type count) const;
	size_type rfind(const char* s, size_type pos = npos) const;
	size_type rfind(uint8_t ch, size_type pos = npos) const;
	// size_type find_first_of(std::string_view str, size_type pos = 0) const;
	// size_type find_first_of(const char* s, size_type pos, size_type count) const;
	// size_type find_first_of(const char* s, size_type pos = 0) const;
	// size_type find_first_of(uint8_t ch, size_type pos = 0) const;
	// size_type find_first_not_of(std::string_view str, size_type pos = 0) const;
	// size_type find_first_not_of(const char* s, size_type pos, size_type count) const;
	// size_type find_first_not_of(const char* s, size_type pos = 0) const;
	// size_type find_first_not_of(uint8_t ch, size_type pos = 0) const;
	// size_type find_last_of(std::string_view str, size_type pos = npos) const;
	// size_type find_last_of(const char* s, size_type pos, size_type count) const;
	// size_type find_last_of(const char* s, size_type pos = npos) const;
	// size_type find_last_of(uint8_t ch, size_type pos = npos) const;
	// size_type find_last_not_of(std::string_view str, size_type pos = npos) const;
	// size_type find_last_not_of(const char* s, size_type pos, size_type count) const;
	// size_type find_last_not_of(const char* s, size_type pos = npos) const;
	// size_type find_last_not_of(uint8_t ch, size_type pos = npos) const;

	int compare(std::string_view str) const noexcept;
	// int compare(size_type pos1, size_type count1, std::string_view str) const;
	// int compare(size_type pos1, size_type count1, std::string_view str, size_type pos2, size_type count2 = npos) const;
	int compare(const char* s) const;
	// int compare(size_type pos1, size_type count1, const char* s) const;
	// int compare(size_type pos1, size_type count1, const char* s, size_type count2) const;
	String substr(size_type pos = 0, size_type count = npos) const;

	// Non-STL String functions
	bool valid_utf8() const noexcept;
	srb2::Vector<uint16_t> to_utf16() const;
	srb2::Vector<uint32_t> to_utf32() const;
	size_t length_decoded() const;

	Utf8Iter decode_begin() const { return Utf8Iter(*this, 0); }
	Utf8Iter decode_end() const { return Utf8Iter(*this, size()); };
};

String operator+(const String& lhs, const String& rhs);
String operator+(const String& lhs, const char* rhs);
String operator+(const String& lhs, uint8_t rhs);
String operator+(const String& lhs, std::string_view view);

bool operator==(const String& lhs, const String& rhs);
bool operator==(const String& lhs, const char* rhs);
// bool operator==(const String& lhs, std::string_view rhs);
bool operator!=(const String& lhs, const String& rhs);
bool operator!=(const String& lhs, const char* rhs);
// bool operator!=(const String& lhs, std::string_view rhs);
bool operator<(const String& lhs, const String& rhs);
bool operator<(const String& lhs, const char* rhs);
// bool operator<(const String& lhs, std::string_view rhs);
bool operator<=(const String& lhs, const String& rhs);
bool operator<=(const String& lhs, const char* rhs);
// bool operator<=(const String& lhs, std::string_view rhs);
bool operator>(const String& lhs, const String& rhs);
bool operator>(const String& lhs, const char* rhs);
// bool operator>(const String& lhs, std::string_view rhs);
bool operator>=(const String& lhs, const String& rhs);
bool operator>=(const String& lhs, const char* rhs);
// bool operator>=(const String& lhs, std::string_view rhs);

Vector<uint16_t> to_utf16(std::string_view utf8);
Vector<uint32_t> to_utf32(std::string_view utf8);

srb2::StaticVec<uint8_t, 4> to_utf8(uint32_t codepoint);

template <
	typename ItUTF32,
	typename std::enable_if_t<
		std::is_constructible<
			uint32_t,
			typename std::iterator_traits<ItUTF32>::reference
		>::value,
		int
	> = 0
>
srb2::String to_utf8(ItUTF32 begin, ItUTF32 end)
{
	srb2::String ret;
	for (auto itr = begin; itr != end; ++itr)
	{
		srb2::StaticVec<uint8_t, 4> utf8 = to_utf8(*itr);
		ret.append(utf8.begin(), utf8.end());
	}
	return ret;
}

srb2::String to_utf8(std::u32string_view utf32view);

// fmtlib
inline auto format_as(const String& str) { return fmt::string_view(static_cast<std::string_view>(str)); }

srb2::String vformat(fmt::string_view fmt, fmt::format_args args);

template <typename... T>
inline auto format(fmt::format_string<T...> fmt, T&&... args)
{
	return ::srb2::vformat(fmt, fmt::vargs<T...>{{args...}});
}

} // namespace srb2

namespace std
{

template <> struct hash<srb2::String>
{
	size_t operator()(const srb2::String& v);
};

} // namespace std

#endif // __cplusplus

#ifndef __cplusplus
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

int Str_IsValidUTF8(const char* str);
// int Str_ToUTF16(uint16_t* dst, size_t dstlen, const char* src);
// int Str_ToUTF32(uint32_t* dst, size_t dstlen, const char* src);
uint32_t Str_NextCodepointFromUTF8(const char** itr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SRB2_CORE_STRING_HPP
