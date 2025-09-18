// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "string.h"
#include "fmt/format.h"

#include <cstring>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>

namespace srb2
{

String::String(const String&) = default;
String::String(String&&) noexcept = default;
String::~String() = default;
String& String::operator=(const String&) = default;
String& String::operator=(String&&) noexcept = default;

String::String(const char* rhs) : String(std::string_view { rhs })
{}

String::String(const char* rhs, size_t len) : String(std::string_view { rhs, len })
{}

String::String(const std::string& rhs) : String(std::string_view { rhs })
{}

String::String(std::string_view view) : String()
{
	append(view);
}

String::operator std::string() const
{
	std::string_view view = *this;
	return std::string(view);
}

String::operator std::string_view() const
{
	return std::string_view((const char*)data(), size());
}

uint32_t String::size() const noexcept
{
	if (data_.empty())
	{
		return 0;
	}
	return data_.size() - 1;
}

static const char* kEmptyString = "";

const char* String::c_str() const
{
	if (data_.empty())
	{
		return kEmptyString;
	}
	return reinterpret_cast<const char*>(data_.data());
}

void String::reserve(size_type capacity)
{
	if (capacity == 0)
	{
		data_.reserve(0);
		return;
	}
	data_.reserve(capacity + 1);
}

uint8_t* String::begin() noexcept
{
	if (data_.empty())
	{
		return nullptr;
	}
	return data();
}

uint8_t* String::end() noexcept
{
	if (data_.empty())
	{
		return nullptr;
	}
	return data() + size();
}

const uint8_t* String::cbegin() const noexcept
{
	if (data_.empty())
	{
		return nullptr;
	}
	return data();
}

const uint8_t* String::cend() const noexcept
{
	if (data_.empty())
	{
		return nullptr;
	}
	return data() + size();
}

uint8_t& String::at(size_type i)
{
	if (i >= size())
	{
		throw std::out_of_range("string byte index out of bounds");
	}
	return data_.at(i);
}

const uint8_t& String::at(size_type i) const
{
	if (i >= size())
	{
		throw std::out_of_range("string byte index out of bounds");
	}
	return data_.at(i);
}

String& String::insert(size_type index, size_type count, uint8_t ch)
{
	if (index > size())
	{
		throw std::out_of_range("string byte index out of bounds");
	}
	data_.insert(data_.begin() + index, count, ch);
	return *this;
}

String& String::insert(size_type index, const char* s)
{
	return insert(index, s, (size_type)std::strlen(s));
}

String& String::insert(size_type index, const char* s, size_type count)
{
	if (index > size())
	{
		throw std::out_of_range("string byte index out of bounds");
	}
	if (!empty())
	{
		// remove null byte
		data_.pop_back();
	}
	data_.insert(data_.begin() + index, s, s + count);
	if (data_.size() > 0)
	{
		data_.push_back(0);
	}
	return *this;
}

String& String::insert(size_type index, std::string_view str)
{
	if (str.empty()) return *this;
	return insert(index, &*str.begin(), (size_type)str.size());
}

String& String::insert(size_type index, std::string_view str, size_t s_index, size_t count)
{
	if (str.empty()) return *this;
	if (s_index > str.size())
	{
		throw std::out_of_range("s_index > str.size()");
	}
	return insert(index, str.substr(s_index, std::max(str.size() - s_index, count)));
}

String::iterator String::insert(const_iterator pos, uint8_t ch)
{
	if (pos < cbegin() || pos > cend())
	{
		throw std::out_of_range("insert iterator out of bounds");
	}
	return data_.insert(pos, ch);
}

String::iterator String::insert(const_iterator pos, size_type count, uint8_t ch)
{
	if (pos < cbegin() || pos > cend())
	{
		throw std::out_of_range("insert iterator out of bounds");
	}
	if (!empty())
	{
		data_.pop_back();
	}
	for (size_type i = 0; i < count; i++)
	{
		data_.insert(pos, ch);
	}
	if (data_.size() > 0)
	{
		data_.push_back(0);
	}
	return const_cast<iterator>(pos);
}

String& String::erase(size_type index, size_type count)
{
	if (index + count >= size())
	{
		throw std::out_of_range("string byte index out of bounds");
	}
	const_iterator first = begin() + index;
	const_iterator last = first + count;
	data_.erase(first, last);
	if (data_.size() == 1)
	{
		data_.pop_back();
	}
	return *this;
}

String::iterator String::erase(const_iterator position)
{
	return data_.erase(position);
}

String::iterator String::erase(const_iterator first, const_iterator last)
{
	return data_.erase(first, last);
}

void String::push_back(uint8_t v)
{
	if (data_.empty())
	{
		data_.push_back(v);
		data_.push_back(0);
		return;
	}
	data_[data_.size() - 1] = v;
	data_.push_back(0);
}

void String::pop_back()
{
	data_.pop_back();
	if (data_.size() == 1)
	{
		data_.pop_back();
	}
	else
	{
		data_[data_.size() - 1] = 0;
	}
}

String& String::append(size_type count, uint8_t ch)
{
	if (count == 0)
	{
		return *this;
	}

	if (!data_.empty())
	{
		data_.pop_back();
	}

	for (size_type i = 0; i < count; i++)
	{
		data_.push_back(ch);
	}
	data_.push_back(0);
	return *this;
}

String& String::append(const char* s, size_type count)
{
	insert(size(), s, count);
	return *this;
}

String& String::append(const char* s)
{
	insert(size(), s);
	return *this;
}

String& String::append(std::string_view str)
{
	insert(size(), str);
	return *this;
}

String& String::append(std::string_view str, size_type pos, size_type count)
{
	insert(size(), str, pos, count);
	return *this;
}

String& String::operator+=(std::string_view r)
{
	insert(size(), r);
	return *this;
}

String& String::operator+=(const char* r)
{
	insert(size(), r);
	return *this;
}

String& String::operator+=(uint8_t r)
{
	push_back(r);
	return *this;
}

String& String::operator+=(std::initializer_list<uint8_t> r)
{
	append(r.begin(), r.end());
	return *this;
}

String& String::replace(size_type pos, size_type count, std::string_view str)
{
	return replace(pos, count, str, 0, str.size());
}

String& String::replace(const_iterator first, const_iterator last, std::string_view str)
{
	if (first < begin() || last > end() || first + str.size() > end())
	{
		throw std::out_of_range("string replacement range out of bounds");
	}
	size_type index = &*first - data_.data();
	size_type count = last - first;

	return replace(index, count, str);
}

String& String::replace(size_type pos, size_type count, std::string_view str, size_t pos2, size_t count2)
{
	if (pos >= size())
	{
		throw std::out_of_range("string replacement range out of bounds");
	}
	if (pos2 >= str.size())
	{
		throw std::out_of_range("string replacement string_view range out of bounds");
	}
	erase(pos, count);
	insert(pos, str, pos2, count2);

	return *this;
}

String& String::replace(size_type pos, size_type count, const char* cstr, size_type count2)
{
	size_t len = std::strlen(cstr);
	return replace(pos, count, std::string_view(cstr, len), count2);
}

String& String::replace(const_iterator first, const_iterator last, const char* cstr, size_type count2)
{
	size_type index = first - data_.data();
	size_type count = last - first;

	return replace(index, count, cstr, count2);
}

String& String::replace(size_type pos, size_type count, const char* cstr)
{
	size_t len = std::strlen(cstr);
	return replace(pos, count, std::string_view(cstr, len));
}

String& String::replace(const_iterator first, const_iterator last, const char* cstr)
{
	size_type index = first - data_.data();
	size_type count = last - first;

	return replace(index, count, cstr);
}

String& String::replace(const_iterator first, const_iterator last, uint8_t ch)
{
	if (first < begin() || last > end())
	{
		throw std::out_of_range("string iterators out of range");
	}
	for (; first != last; first++)
	{
		*const_cast<iterator>(first) = ch;
	}
	return *this;
}

String& String::replace(const_iterator first, const_iterator last, std::initializer_list<uint8_t> ilist)
{
	return replace(first, last, ilist.begin(), ilist.end());
}

String::size_type String::copy(uint8_t* dest, size_type count, size_type pos) const
{
	if (pos > size())
	{
		throw std::out_of_range("string byte index out of bounds");
	}
	size_type copied = 0;
	for (size_type i = 0; i < count && (i + pos) < size(); i++)
	{
		dest[i] = data_[i + pos];
		copied += 1;
	}
	return copied;
}

String::size_type String::copy(char* dest, size_type count, size_type pos) const
{
	if (pos > size())
	{
		throw std::out_of_range("string byte index out of bounds");
	}
	size_type copied = 0;
	for (size_type i = 0; i < count && (i + pos) < size(); i++)
	{
		dest[i] = data_[i + pos];
		copied += 1;
	}
	return copied;
}

void String::resize(size_type count)
{
	if (count == 0)
	{
		data_.clear();
		return;
	}
	data_.resize(count + 1);
	data_[count] = 0;
}

void String::resize(size_type count, uint8_t ch)
{
	if (count == 0)
	{
		data_.clear();
		return;
	}
	data_.resize(count + 1, ch);
	data_[count] = 0;
}

void String::swap(String& other) noexcept
{
	std::swap(this->data_, other.data_);
}

String::size_type String::find(const String& str, size_type pos) const
{
	return find(static_cast<std::string_view>(str), pos);
}

String::size_type String::find(std::string_view str, size_type pos) const
{
	if (size() == 0)
	{
		return npos;
	}

	for (size_type i = pos; i < size(); i++)
	{
		bool found = true;
		for (size_t j = 0; j < str.size() && found; j++)
		{
			if (i + j >= size() || data_[i + j] != str[j])
			{
				found = false;
			}
		}
		if (found)
		{
			return i;
		}
	}
	return npos;
}

String::size_type String::find(const char* s, size_type pos, size_t count) const
{
	return find(std::string_view(s, count), pos);
}

String::size_type String::find(const char* s, size_type pos) const
{
	size_t len = std::strlen(s);
	return find(std::string_view(s, len), pos);
}

String::size_type String::find(uint8_t ch, size_type pos) const
{
	for (size_type i = pos; i < size(); i++)
	{
		if (data_[i] == ch)
		{
			return i;
		}
	}
	return npos;
}

String::size_type String::rfind(const String& str, size_type pos) const
{
	return rfind(static_cast<std::string_view>(str), pos);
}

String::size_type String::rfind(std::string_view str, size_type pos) const
{
	if (str.empty())
	{
		return std::min(pos, size());
	}
	if (size() == 0)
	{
		return npos;
	}

	for (size_type i = std::min(pos, size()); i >= 0; i--)
	{
		bool found = true;
		for (size_t j = 0; j < str.size() && found; j++)
		{
			if (i + j >= size() || data_[i + j] != str[j])
			{
				found = false;
			}
		}
		if (found)
		{
			return i;
		}
	}
	return npos;
}

String::size_type String::rfind(const char* s, size_type pos, size_type count) const
{
	return rfind(std::string_view(s, count), pos);
}

String::size_type String::rfind(const char* s, size_type pos) const
{
	size_t len = std::strlen(s);
	return rfind(std::string_view(s, len), pos);
}

String::size_type String::rfind(uint8_t ch, size_type pos) const
{
	if (empty())
	{
		return npos;
	}

	for (size_type i = std::min(pos, size()); i >= 0; i--)
	{
		if (data_[i] == ch)
		{
			return i;
		}
	}

	return npos;
}

int String::compare(std::string_view str) const noexcept
{
	std::string_view self = *this;
	return self.compare(str);
}

int String::compare(const char* s) const
{
	std::string_view self = *this;
	std::string_view that { s, std::strlen(s) };
	return self.compare(that);
}

String String::substr(size_type pos, size_type count) const
{
	String ret;
	if (pos >= size())
	{
		throw std::out_of_range("string byte index invalid");
	}
	size_type start = pos;
	size_type end = std::min(pos + count, size() - pos);
	ret.reserve(end - start);
	for (size_type i = start; i < end; i++)
	{
		ret.push_back(data_[i]);
	}
	return ret;
}

bool String::valid_utf8() const noexcept
{
	for (auto itr = decode_begin(); itr != decode_end(); itr++)
	{
		if (!itr.valid())
		{
			return false;
		}
	}
	return true;
}

Vector<uint16_t> String::to_utf16() const
{
	return ::srb2::to_utf16(static_cast<std::string_view>(*this));
}

Vector<uint16_t> to_utf16(std::string_view utf8)
{
	Vector<uint16_t> ret;
	for (auto itr = Utf8Iter::begin(utf8); itr != Utf8Iter::end(utf8); itr++)
	{
		uint32_t codepoint = *itr;
		if (codepoint < 0x10000)
		{
			ret.push_back(static_cast<uint16_t>(codepoint));
			continue;
		}
		// high surrogate
		ret.push_back(static_cast<uint16_t>(
			(((codepoint - 0x10000) & 0b11111111110000000000) >> 10) + 0xD800
		));
		// low surrogate
		ret.push_back(static_cast<uint16_t>(
			(((codepoint - 0x10000) & 0b1111111111)) + 0xDC00
		));
	}
	return ret;
}

Vector<uint32_t> to_utf32(std::string_view utf8)
{
	Vector<uint32_t> ret;
	for (auto itr = Utf8Iter::begin(utf8); itr != Utf8Iter::end(utf8); itr++)
	{
		ret.push_back(itr.codepoint());
	}
	return ret;
}

StaticVec<uint8_t, 4> to_utf8(uint32_t codepoint)
{
	StaticVec<uint8_t, 4> enc;
	if (codepoint < 0x80)
	{
		enc.push_back(static_cast<uint8_t>(codepoint));
	}
	else if (codepoint >= 0x80 && codepoint < 0x800)
	{
		enc.push_back(((codepoint >> 6) & 0b11111) + 0xC0);
		enc.push_back((codepoint & 0b111111) + 0x80);
	}
	else if (codepoint >= 0x800 && codepoint < 0x10000)
	{
		enc.push_back(((codepoint >> 12) & 0b1111) + 0xE0);
		enc.push_back(((codepoint >> 6) & 0b111111) + 0x80);
		enc.push_back((codepoint & 0b111111) + 0x80);
	}
	else if (codepoint >= 0x10000 && codepoint < 0x110000)
	{
		enc.push_back(((codepoint >> 18) & 0b111) + 0xF0);
		enc.push_back(((codepoint >> 12) & 0b111111) + 0x80);
		enc.push_back(((codepoint >> 6) & 0b111111) + 0x80);
		enc.push_back((codepoint & 0b111111) + 0x80);
	}
	else
	{
		// replacement char due to invalid codepoint
		enc = to_utf8(0xFFFD);
	}
	return enc;
}

String to_utf8(std::u32string_view utf32view)
{
	return to_utf8(utf32view.begin(), utf32view.end());
}

String operator+(const String& lhs, const String& rhs)
{
	String ret;
	ret.append(lhs);
	ret.append(rhs);
	return ret;
}

String operator+(const String& lhs, const char* rhs)
{
	String ret;
	size_t len = std::strlen(rhs);
	ret.append(lhs);
	ret.append(std::string_view(rhs, len));
	return ret;
}

String operator+(const String& lhs, uint8_t rhs)
{
	String ret;
	ret.append(lhs);
	ret.push_back(rhs);
	return ret;
}

String operator+(const String& lhs, std::string_view view)
{
	String ret;
	ret.append(lhs);
	ret.append(view);
	return ret;
}

bool operator==(const String& lhs, const String& rhs)
{
	return lhs.compare(rhs) == 0;
}

bool operator==(const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) == 0;
}

// bool operator==(const String& lhs, std::string_view rhs)
// {
// 	return lhs.compare(rhs) == 0;
// }

bool operator!=(const String& lhs, const String& rhs)
{
	return !(lhs == rhs);
}

bool operator!=(const String& lhs, const char* rhs)
{
	return !(lhs == rhs);
}

// bool operator!=(const String& lhs, std::string_view rhs)
// {
// 	return !(lhs == rhs);
// }

bool operator<(const String& lhs, const String& rhs)
{
	return lhs.compare(rhs) < 0;
}

bool operator<(const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) < 0;
}

// bool operator<(const String& lhs, std::string_view rhs)
// {
// 	return lhs.compare(rhs) < 0;
// }

bool operator<=(const String& lhs, const String& rhs)
{
	return lhs.compare(rhs) <= 0;
}

bool operator<=(const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) <= 0;
}

// bool operator<=(const String& lhs, std::string_view rhs)
// {
// 	return lhs.compare(rhs) <= 0;
// }

bool operator>(const String& lhs, const String& rhs)
{
	return lhs.compare(rhs) > 0;
}

bool operator>(const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) > 0;
}

// bool operator>(const String& lhs, std::string_view rhs)
// {
// 	return lhs.compare(rhs) > 0;
// }

bool operator>=(const String& lhs, const String& rhs)
{
	return lhs.compare(rhs) >= 0;
}

bool operator>=(const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) >= 0;
}

// bool operator>=(const String& lhs, std::string_view rhs)
// {
// 	return lhs.compare(rhs) >= 0;
// }

static constexpr bool is_utf8_byte(uint8_t b)
{
	return b != 0xC0 && b != 0xC1 && b < 0xF5;
}

static constexpr bool is_utf8_continuation(uint8_t b)
{
	return b >= 0x80 && b < 0xC0;
}

uint32_t Utf8Iter::do_codepoint() const
{
	uint8_t b[4];
	uint8_t s;
	bool v = true;
	b[0] = s_[i_];
	if (b[0] < 0x80) s = 1;
	else if (b[0] >= 0x80 && b[0] < 0xC0)
	{
		// invalid, first byte continuation
		s = 1;
		v = false;
	}
	else if (b[0] >= 0xC0 && b[0] < 0xE0)
	{
		// 2 byte
		if (s_.size() - i_ < 2)
		{
			// invalid, truncated
			s = 1;
			v = false;
			goto decode;
		}

		b[1] = s_[i_ + 1];

		if (!is_utf8_continuation(b[1]))
		{
			// invalid, not a continuation
			s = 1;
			v = false;
			goto decode;
		}

		s = 2;
	}
	else if (b[0] >= 0xE0 && b[0] < 0xF0)
	{
		// 3 byte
		if (s_.size() - i_ < 2)
		{
			// invalid, truncated
			s = 1;
			v = false;
			goto decode;
		}
		if (s_.size() - i_ < 3)
		{
			// invalid, truncated
			s = 2;
			v = false;
			goto decode;
		}

		b[1] = s_[i_ + 1];
		b[2] = s_[i_ + 2];

		if (!is_utf8_continuation(b[1]))
		{
			// invalid, not a continuation
			s = 1;
			v = false;
			goto decode;
		}

		if (!is_utf8_continuation(b[2]))
		{
			// invalid, not a continuation
			s = 2;
			v = false;
			goto decode;
		}

		s = 3;
	}
	else if (b[0] >= 0xF0 && b[0] < 0xF5)
	{
		// 4 byte
		if (s_.size() - i_ < 2)
		{
			// invalid, truncated
			s = 1;
			v = false;
			goto decode;
		}
		if (s_.size() - i_ < 3)
		{
			// invalid, truncated
			s = 2;
			v = false;
			goto decode;
		}
		if (s_.size() - i_ < 4)
		{
			// invalid, truncated
			s = 3;
			v = false;
			goto decode;
		}

		b[1] = s_[i_ + 1];
		b[2] = s_[i_ + 2];
		b[3] = s_[i_ + 3];

		if (!is_utf8_continuation(b[1]))
		{
			// invalid, not a continuation
			s = 1;
			v = false;
			goto decode;
		}

		if (!is_utf8_continuation(b[2]))
		{
			// invalid, not a continuation
			s = 2;
			v = false;
			goto decode;
		}

		if (!is_utf8_continuation(b[3]))
		{
			// invalid, not a continuation
			s = 3;
			v = false;
			goto decode;
		}

		s = 4;
	}
	else
	{
		// invalid
		s = 1;
		v = false;
	}

decode:
	// bit 29 indicates unparseable (immediately invalid, replacement char U+FFFD)
	// bit 30-31 indicates byte size (0-3)
	if (v == false) return 0xFFFD + ((s - 1) << 30) + (1 << 29);

	switch (s)
	{
	default:
	case 1: return b[0] & 0x7f;
	case 2: return (b[1] & 0x3f) + ((b[0] & 0x1f) << 6) + (1 << 30);
	case 3: return (b[2] & 0x3f) + ((b[1] & 0x3f) << 6) + ((b[0] & 0x0f) << 12) + (2 << 30);
	case 4: return (b[3] & 0x3f) + ((b[2] & 0x3f) << 6) + ((b[1] & 0x3f) << 12) + ((b[2] & 0x7) << 18) + (3 << 30);
	}
}

uint32_t Utf8Iter::codepoint() const
{
	uint32_t c = do_codepoint();
	uint32_t ret = c & 0x001fffff;
	uint8_t s = c >> 30;

	// overlong encodings are still invalid and should be replaced,
	// even if bit 29 is unset
	switch (s)
	{
	default:
	case 0: return ret >= (2 << 8) ? 0xFFFD : ret;
	case 1: return ret >= (2 << 12) ? 0xFFFD : ret;
	case 2: return ret >= (2 << 17) ? 0xFFFD : ret;
	case 3: return ret;
	}
}

bool Utf8Iter::valid() const
{
	uint32_t c = do_codepoint();
	uint32_t ret = c & 0x001fffff;
	if ((c >> 29) & 1) return false;
	uint8_t s = c >> 30;

	switch (s)
	{
	default:
	case 0: return ret >= (2 << 8) ? false : true;
	case 1: return ret >= (2 << 12) ? false : true;
	case 2: return ret >= (2 << 17) ? false : true;
	case 3: return true;
	}
}

uint8_t Utf8Iter::size() const
{
	uint32_t c = do_codepoint();
	uint8_t s = (c >> 30);
	return s + 1;
}

static constexpr bool utf16_is_low_surrogate(uint16_t word)
{
	return word >= 0xDC00 && word < 0xDFFF;
}

static constexpr bool utf16_is_high_surrogate(uint16_t word)
{
	return word >= 0xD800 && word < 0xDBFF;
}

static constexpr bool utf16_is_surrogate(uint16_t word)
{
	return utf16_is_high_surrogate(word) || utf16_is_low_surrogate(word);
}

uint32_t Utf16Iter::do_codepoint() const
{
	uint16_t words[2];
	words[0] = s_[i_];
	if (!utf16_is_high_surrogate(words[0]))
	{
		// unpaired low surrogates allowed as-is for windows compatibility
		return words[0];
	}
	if (s_.size() - i_ < 2)
	{
		// unpaired high surrogates allowed as-is for windows compatibility
		return words[0];
	}
	words[1] = s_[i_ + 1];
	return ((words[1] - 0xDC00) & 0x3FF)
		+ ((words[0] - 0xD800) & 0x3FF)
		+ 0x10000;
}

uint32_t Utf16Iter::codepoint() const
{
	uint32_t c = do_codepoint();
	uint32_t ret = c & 0x001fffff;

	return ret;
}

uint8_t Utf16Iter::size() const
{
	uint32_t c = do_codepoint() & 0x001fffff;
	return c >= 0x10000 ? 2 : 1;
}

// fmtlib

String vformat(fmt::string_view fmt, fmt::format_args args)
{
	auto buf = fmt::memory_buffer();
	vformat_to(buf, fmt, args);
	return { buf.data(), buf.size() };
}

} // namespace srb2

size_t std::hash<srb2::String>::operator()(const srb2::String& v)
{
	std::string_view str = v;
	return std::hash<std::string_view>()(str);
}

// C functions

int Str_IsValidUTF8(const char* str)
{
	size_t len = std::strlen(str);
	if (len == 0)
	{
		return 1;
	}

	for (auto itr = srb2::Utf8Iter::begin(str); itr != srb2::Utf8Iter::end(str + len - 1); ++itr)
	{
		if (!itr.valid())
		{
			return false;
		}
	}
	return true;
}

uint32_t Str_NextCodepointFromUTF8(const char** itr)
{
	auto i = srb2::Utf8Iter::begin(*itr);
	uint32_t ret = i.codepoint();
	uint8_t s = i.size();
	*itr += s;
	return ret;
}
