// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef SRB2_CORE_JSON_HPP
#define SRB2_CORE_JSON_HPP

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <tcb/span.hpp>

#include "hash_map.hpp"
#include "string.h"
#include "vector.hpp"

#include "json_macro.inl"

namespace srb2
{

class JsonValue;

using JsonArray = Vector<JsonValue>;
using JsonObject = HashMap<String, JsonValue>;

class JsonError : public std::runtime_error
{
public:
	JsonError(const std::string& what);
	JsonError(const char* what);
	JsonError(const JsonError&) noexcept;
};

class JsonParseError : public JsonError
{
public:
	JsonParseError(const std::string& what);
	JsonParseError(const char* what);
	JsonParseError(const JsonParseError&) noexcept;
};

class JsonValue
{
public:
	enum class Type : int
	{
		kNull,
		kBoolean,
		kNumber,
		kString,
		kArray,
		kObject
	};

private:
	Type type_;
	union {
		struct {} dummy;
		bool boolean_;
		double number_;
		String string_;
		JsonArray array_;
		JsonObject object_;
	};

	static void value_to_ubjson(srb2::Vector<std::byte>& out);
	static void value_to_ubjson(srb2::Vector<std::byte>& out, bool value);
	static void value_to_ubjson(srb2::Vector<std::byte>& out, double value);
	static void value_to_ubjson(srb2::Vector<std::byte>& out, uint64_t value);
	static void value_to_ubjson(srb2::Vector<std::byte>& out, const String& value, bool include_type);
	static void value_to_ubjson(srb2::Vector<std::byte>& out, const JsonArray& value);
	static void value_to_ubjson(srb2::Vector<std::byte>& out, const JsonObject& value);

	void do_to_ubjson(srb2::Vector<std::byte>& out) const;

public:
	JsonValue();
	JsonValue(const JsonValue&);
	JsonValue(JsonValue&&) noexcept;
	~JsonValue();
	JsonValue& operator=(const JsonValue&);
	JsonValue& operator=(JsonValue&&) noexcept;

	JsonValue(const String& value);
	JsonValue(String&& value);
	JsonValue(const JsonArray& value);
	JsonValue(JsonArray&& value);
	JsonValue(const JsonObject& value);
	JsonValue(JsonObject&& value);
	JsonValue(float value);
	JsonValue(double value);
	JsonValue(int8_t value);
	JsonValue(int16_t value);
	JsonValue(int32_t value);
	JsonValue(int64_t value);
	JsonValue(uint8_t value);
	JsonValue(uint16_t value);
	JsonValue(uint32_t value);
	JsonValue(uint64_t value);
	JsonValue(bool value);

	static JsonValue array() { return JsonValue(JsonArray()); }
	static JsonValue object() { return JsonValue(JsonObject()); }

	bool operator==(const JsonValue& rhs) const;
	bool operator!=(const JsonValue& rhs) const { return !(*this == rhs); }

	void to_json(JsonValue& to) const;
	void from_json(const JsonValue& from);
	String to_json_string() const;
	static JsonValue from_json_string(const String& str);
	srb2::Vector<std::byte> to_ubjson() const;
	static JsonValue from_ubjson(tcb::span<const std::byte> ubjson);
	constexpr Type type() const noexcept { return type_; }

	template <typename V> V get() const;

	constexpr bool is_null() const noexcept { return type_ == Type::kNull; }
	constexpr bool is_boolean() const noexcept { return type_ == Type::kBoolean; }
	constexpr bool is_number() const noexcept { return type_ == Type::kNumber; }
	constexpr bool is_string() const noexcept { return type_ == Type::kString; }
	constexpr bool is_array() const noexcept { return type_ == Type::kArray; }
	constexpr bool is_object() const noexcept { return type_ == Type::kObject; }
	JsonArray& as_array();
	JsonObject& as_object();
	const JsonArray& as_array() const;
	const JsonObject& as_object() const;

	JsonValue& at(uint32_t i);
	const JsonValue& at(uint32_t i) const;
	JsonValue& at(const char* key);
	JsonValue& at(const String& key);
	const JsonValue& at(const char* key) const;
	const JsonValue& at(const String& key) const;

	template <typename V>
	V value(const char* key, const V& def) const;
	template <typename V>
	V value(const String& key, const V& def) const;
	template <typename V>
	V value(const char* key, V&& def) const;
	template <typename V>
	V value(const String& key, V&& def) const;

	bool contains(const char* key) const;
	bool contains(const String& key) const;

	JsonValue& operator[](uint32_t i);
	JsonValue& operator[](const char* key);
	JsonValue& operator[](const String& key);
};

void to_json(JsonValue& to, bool value);
void to_json(JsonValue& to, int8_t value);
void to_json(JsonValue& to, int16_t value);
void to_json(JsonValue& to, int32_t value);
void to_json(JsonValue& to, int64_t value);
void to_json(JsonValue& to, uint8_t value);
void to_json(JsonValue& to, uint16_t value);
void to_json(JsonValue& to, uint32_t value);
void to_json(JsonValue& to, uint64_t value);
void to_json(JsonValue& to, float value);
void to_json(JsonValue& to, double value);
void to_json(JsonValue& to, const String& value);
void to_json(JsonValue& to, const JsonArray& value);
void to_json(JsonValue& to, const JsonObject& value);

void from_json(const JsonValue& to, bool& value);
void from_json(const JsonValue& to, int8_t& value);
void from_json(const JsonValue& to, int16_t& value);
void from_json(const JsonValue& to, int32_t& value);
void from_json(const JsonValue& to, int64_t& value);
void from_json(const JsonValue& to, uint8_t& value);
void from_json(const JsonValue& to, uint16_t& value);
void from_json(const JsonValue& to, uint32_t& value);
void from_json(const JsonValue& to, uint64_t& value);
void from_json(const JsonValue& to, float& value);
void from_json(const JsonValue& to, double& value);
void from_json(const JsonValue& to, String& value);
void from_json(const JsonValue& to, JsonArray& value);
void from_json(const JsonValue& to, JsonObject& value);

template <typename T, size_t S>
inline void to_json(JsonValue& to, const std::array<T, S>& v)
{
	JsonArray arr;
	for (auto itr = v.begin(); itr != v.end(); itr++)
	{
		JsonValue conv;
		to_json(conv, *itr);
		arr.push_back(conv);
	}
	to = JsonValue(std::move(arr));
}

template <typename T, size_t S>
inline void from_json(const JsonValue& to, std::array<T, S>& v)
{
	if (!to.is_array())
	{
		throw JsonError("json value must be an array");
	}
	const JsonArray& arr = to.as_array();
	size_t si = 0;
	for (auto& i : arr)
	{
		if (si >= v.size())
		{
			break;
		}

		T conv;
		from_json(i, conv);
		v[si] = std::move(conv);
		si++;
	}
}

template <typename T, typename A>
inline void to_json(JsonValue& to, const std::vector<T, A>& v)
{
	JsonArray arr;
	for (auto itr = v.begin(); itr != v.end(); itr++)
	{
		JsonValue conv;
		to_json(conv, *itr);
		arr.push_back(conv);
	}
	to = JsonValue(std::move(arr));
}

template <typename T, typename A>
inline void from_json(const JsonValue& to, std::vector<T, A>& v)
{
	if (!to.is_array())
	{
		throw JsonError("json value must be an array");
	}
	v.clear();
	const JsonArray& arr = to.as_array();
	for (auto& i : arr)
	{
		T conv;
		from_json(i, conv);
		v.push_back(std::move(conv));
	}
}

template <typename T>
inline void to_json(JsonValue& to, const srb2::Vector<T>& v)
{
	JsonArray arr;
	for (auto itr = v.begin(); itr != v.end(); itr++)
	{
		JsonValue conv;
		to_json(conv, *itr);
		arr.push_back(conv);
	}
	to = JsonValue(std::move(arr));
}

template <typename T>
inline void from_json(const JsonValue& to, srb2::Vector<T>& v)
{
	if (!to.is_array())
	{
		throw JsonError("json value must be an array");
	}
	v.clear();
	const JsonArray& arr = to.as_array();
	for (auto& i : arr)
	{
		T conv;
		from_json(i, conv);
		v.push_back(std::move(conv));
	}
}

template <typename K, typename V, typename H, typename E, typename A>
inline void to_json(JsonValue& to, const std::unordered_map<K, V, H, E, A>& v)
{
	JsonObject obj;
	for (auto itr = v.begin(); itr != v.end(); itr++)
	{
		to_json(obj[itr->first], itr->second);
	}
	to = JsonValue(std::move(obj));
}

template <typename K, typename V, typename H, typename E, typename A>
inline void from_json(const JsonValue& to, std::unordered_map<K, V, H, E, A>& v)
{
	if (!to.is_object())
	{
		throw JsonError("json value must be an object");
	}
	v.clear();
	const JsonObject& obj = to.as_object();
	for (auto itr = obj.begin(); itr != obj.end(); itr++)
	{
		V conv;
		from_json(itr->second, conv);
		v[itr->first] = std::move(conv);
	}
}

template <typename K, typename V>
inline void to_json(JsonValue& to, const srb2::HashMap<K, V>& v)
{
	JsonObject obj;
	for (auto itr = v.begin(); itr != v.end(); itr++)
	{
		to_json(obj[itr->first], itr->second);
	}
	to = JsonValue(std::move(obj));
}

template <typename K, typename V>
inline void from_json(const JsonValue& to, srb2::HashMap<K, V>& v)
{
	if (!to.is_object())
	{
		throw JsonError("json value must be an object");
	}
	v.clear();
	const JsonObject& obj = to.as_object();
	for (auto itr = obj.begin(); itr != obj.end(); itr++)
	{
		V conv;
		from_json(itr->second, conv);
		v[itr->first] = std::move(conv);
	}
}

void from_json(const JsonValue& to, std::string& v);
void to_json(JsonValue& to, const std::string& v);

// template <typename K, typename V>
// inline void to_json(JsonValue& to, const srb2::OAHashMap<K, V>& v)
// {
// 	JsonObject obj;
// 	for (auto itr = v.begin(); itr != v.end(); itr++)
// 	{
// 		to_json(obj[itr->first], itr->second);
// 	}
// 	to = JsonValue(std::move(obj));
// }

// template <typename K, typename V>
// inline void from_json(const JsonValue& to, srb2::OAHashMap<K, V>& v)
// {
// 	if (!to.is_object())
// 	{
// 		throw JsonError("json value must be an object");
// 	}
// 	v.clear();
// 	const JsonObject& obj = to.as_object();
// 	for (auto itr = obj.begin(); itr != obj.end(); itr++)
// 	{
// 		V conv;
// 		from_json(itr->second, conv);
// 		v[itr->first] = std::move(conv);
// 	}
// }
//

template <typename V>
inline V JsonValue::get() const
{
	V v;
	from_json(*this, v);
	return v;
}

template <typename V>
inline V JsonValue::value(const char* key, const V& def) const
{
	V copy { def };
	return value(key, std::move(copy));
}

template <typename V>
inline V JsonValue::value(const String& key, const V& def) const
{
	return value(key.c_str(), def);
}

template <typename V>
inline V JsonValue::value(const char* key, V&& def) const
{
	const JsonObject& obj = as_object();
	auto itr = obj.find(key);
	if (itr != obj.end())
	{
		return itr->second.get<V>();
	}
	return def;
}

template <> bool JsonValue::get() const;
template <> int8_t JsonValue::get() const;
template <> int16_t JsonValue::get() const;
template <> int32_t JsonValue::get() const;
template <> int64_t JsonValue::get() const;
template <> uint8_t JsonValue::get() const;
template <> uint16_t JsonValue::get() const;
template <> uint32_t JsonValue::get() const;
template <> uint64_t JsonValue::get() const;
template <> float JsonValue::get() const;
template <> double JsonValue::get() const;
template <> String JsonValue::get() const;
template <> std::string JsonValue::get() const;
template <> std::string_view JsonValue::get() const;
template <> JsonArray JsonValue::get() const;
template <> JsonObject JsonValue::get() const;
template <> JsonValue JsonValue::get() const;

inline bool operator==(const JsonValue& lhs, bool rhs)
{
	if (!lhs.is_boolean())
	{
		return false;
	}
	return lhs.get<bool>() == rhs;
}

inline bool operator==(const JsonValue& lhs, int64_t rhs)
{
	if (!lhs.is_number())
	{
		return false;
	}
	return lhs.get<int64_t>() == rhs;
}

inline bool operator==(const JsonValue& lhs, uint64_t rhs)
{
	if (!lhs.is_number())
	{
		return false;
	}
	return lhs.get<uint64_t>() == rhs;
}

inline bool operator==(const JsonValue& lhs, double rhs)
{
	if (!lhs.is_number())
	{
		return false;
	}
	return lhs.get<double>() == rhs;
}

inline bool operator==(const JsonValue& lhs, std::string_view rhs)
{
	if (!lhs.is_string())
	{
		return false;
	}
	return lhs.get<std::string_view>() == rhs;
}

inline bool operator==(const JsonValue& lhs, const JsonArray& rhs)
{
	if (!lhs.is_array())
	{
		return false;
	}
	return lhs.as_array() == rhs;
}

inline bool operator==(const JsonValue& lhs, const JsonObject& rhs)
{
	if (!lhs.is_object())
	{
		return false;
	}
	return lhs.as_object() == rhs;
}

inline bool operator!=(const JsonValue& lhs, bool rhs)
{
	return !(lhs == rhs);
}

inline bool operator!=(const JsonValue& lhs, int64_t rhs)
{
	return !(lhs == rhs);
}

inline bool operator!=(const JsonValue& lhs, uint64_t rhs)
{
	return !(lhs == rhs);
}

inline bool operator!=(const JsonValue& lhs, double rhs)
{
	return !(lhs == rhs);
}

inline bool operator!=(const JsonValue& lhs, std::string_view rhs)
{
	return !(lhs == rhs);
}

inline bool operator!=(const JsonValue& lhs, const JsonArray& rhs)
{
	return !(lhs == rhs);
}

inline bool operator!=(const JsonValue& lhs, const JsonObject& rhs)
{
	return !(lhs == rhs);
}

extern template class Vector<srb2::JsonValue>;
extern template class HashMap<String, srb2::JsonValue>;

} // namespace srb2

#endif // SRB2_CORE_JSON_HPP
