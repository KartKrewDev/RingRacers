// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Ronald "Eidolon" Kinard
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#include "json.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

using namespace srb2;

JsonError::JsonError(const std::string& what) : std::runtime_error(what) {}
JsonError::JsonError(const char* what) : std::runtime_error(what) {}
JsonError::JsonError(const JsonError&) noexcept = default;
JsonParseError::JsonParseError(const std::string& what) : JsonError(what) {}
JsonParseError::JsonParseError(const char* what) : JsonError(what) {}
JsonParseError::JsonParseError(const JsonParseError&) noexcept = default;

JsonValue::JsonValue()
	: type_(Type::kNull)
{}

JsonValue::JsonValue(const JsonValue& r)
	: type_(Type::kNull)
{
	*this = r;
}

JsonValue::JsonValue(JsonValue&& r) noexcept
	: type_(Type::kNull)
{
	*this = std::move(r);
}

JsonValue& JsonValue::operator=(const JsonValue& r)
{
	switch (type_)
	{
	case Type::kString:
		string_.~String();
		break;
	case Type::kArray:
		array_.~JsonArray();
		break;
	case Type::kObject:
		object_.~JsonObject();
		break;
	default:
		break;
	}
	type_ = r.type_;
	switch (type_)
	{
	case Type::kNull:
		break;
	case Type::kBoolean:
		boolean_ = r.boolean_;
		break;
	case Type::kNumber:
		number_ = r.number_;
		break;
	case Type::kString:
		new (&string_) String(r.string_);
		break;
	case Type::kArray:
		new (&array_) JsonArray(r.array_);
		break;
	case Type::kObject:
		new (&object_) JsonObject(r.object_);
		break;
	}
	return *this;
}

JsonValue& JsonValue::operator=(JsonValue&& r) noexcept
{
	switch (type_)
	{
	case Type::kString:
		string_.~String();
		break;
	case Type::kArray:
		array_.~JsonArray();
		break;
	case Type::kObject:
		object_.~JsonObject();
		break;
	default:
		break;
	}
	type_ = r.type_;
	switch (type_)
	{
	case Type::kBoolean:
		boolean_ = r.boolean_;
		break;
	case Type::kNumber:
		number_ = r.number_;
		break;
	case Type::kString:
		new (&string_) String(std::move(r.string_));
		break;
	case Type::kArray:
		new (&array_) JsonArray(std::move(r.array_));
		break;
	case Type::kObject:
		new (&object_) JsonObject(std::move(r.object_));
		break;
	default:
		break;
	}
	switch (r.type_)
	{
	case Type::kString:
		r.string_.~String();
		break;
	case Type::kArray:
		r.array_.~JsonArray();
		break;
	case Type::kObject:
		r.object_.~JsonObject();
	default:
		break;
	}
	r.type_ = Type::kNull;
	return *this;
}

JsonValue::~JsonValue()
{
	switch (type_)
	{
	case Type::kString:
		string_.~String();
		break;
	case Type::kArray:
		array_.~JsonArray();
		break;
	case Type::kObject:
		object_.~JsonObject();
		break;
	default:
		break;
	}
	type_ = Type::kNull;
}

template <> bool JsonValue::get() const { return type_ == Type::kBoolean ? boolean_ : false; }
template <> int8_t JsonValue::get() const { return type_ == Type::kNumber ? number_ : 0; }
template <> int16_t JsonValue::get() const { return type_ == Type::kNumber ? number_ : 0; }
template <> int32_t JsonValue::get() const { return type_ == Type::kNumber ? number_ : 0; }
template <> int64_t JsonValue::get() const { return type_ == Type::kNumber ? number_ : 0; }
template <> uint8_t JsonValue::get() const { return type_ == Type::kNumber ? number_ : 0; }
template <> uint16_t JsonValue::get() const { return type_ == Type::kNumber ? (uint16_t)number_ : 0; }
template <> uint32_t JsonValue::get() const { return type_ == Type::kNumber ? (uint32_t)number_ : 0; }
template <> uint64_t JsonValue::get() const { return type_ == Type::kNumber ? (uint64_t)number_ : 0; }
template <> float JsonValue::get() const { return type_ == Type::kNumber ? number_ : 0; }
template <> double JsonValue::get() const { return type_ == Type::kNumber ? number_ : 0; }
template <> String JsonValue::get() const { return type_ == Type::kString ? string_ : String(); }
template <> std::string JsonValue::get() const { return type_ == Type::kString ? static_cast<std::string>(string_) : std::string(); }
template <> std::string_view JsonValue::get() const { return type_ == Type::kString ? static_cast<std::string_view>(string_) : std::string_view(); }
template <> JsonArray JsonValue::get() const { return type_ == Type::kArray ? array_ : JsonArray(); }
template <> JsonObject JsonValue::get() const { return type_ == Type::kObject ? object_ : JsonObject(); }
template <> JsonValue JsonValue::get() const { return *this; }

JsonArray& JsonValue::as_array()
{
	if (!is_array()) throw JsonError("accessing non-array as array");
	return array_;
}

const JsonArray& JsonValue::as_array() const
{
	if (!is_array()) throw JsonError("accessing non-array as array");
	return array_;
}

JsonObject& JsonValue::as_object()
{
	if (!is_object()) throw JsonError("accessing non-object as object");
	return object_;
}

const JsonObject& JsonValue::as_object() const
{
	if (!is_object()) throw JsonError("accessing non-array as object");
	return object_;
}

JsonValue& JsonValue::at(uint32_t i)
{
	JsonArray& arr = as_array();
	return arr.at(i);
}

const JsonValue& JsonValue::at(uint32_t i) const
{
	const JsonArray& arr = as_array();
	return arr.at(i);
}

JsonValue& JsonValue::at(const char* key)
{
	JsonObject& obj = as_object();
	return obj.at(key);
}

JsonValue& JsonValue::at(const String& key)
{
	JsonObject& obj = as_object();
	return obj.at(key);
}

const JsonValue& JsonValue::at(const char* key) const
{
	const JsonObject& obj = as_object();
	return obj.at(key);
}

const JsonValue& JsonValue::at(const String& key) const
{
	const JsonObject& obj = as_object();
	return obj.at(key);
}

JsonValue& JsonValue::operator[](uint32_t i)
{
	if (is_null() && !is_array())
	{
		*this = JsonArray();
	}
	else if (!is_array())
	{
		throw JsonError("indexing non-array as array");
	}
	JsonArray& arr = as_array();
	return arr[i];
}

JsonValue& JsonValue::operator[](const char* key)
{
	if (is_null() && !is_object())
	{
		*this = JsonObject();
	}
	else if (!is_object())
	{
		throw JsonError("indexing non-object as object");
	}
	JsonObject& obj = as_object();
	return obj[key];
}

JsonValue& JsonValue::operator[](const String& key)
{
	if (is_null() && !is_object())
	{
		*this = JsonObject();
	}
	else if (!is_object())
	{
		throw JsonError("indexing non-object as object");
	}
	JsonObject& obj = as_object();
	return obj[key];
}

bool JsonValue::contains(const String& key) const
{
	return contains(key.c_str());
}

bool JsonValue::contains(const char* key) const
{
	if (!is_object())
	{
		return false;
	}
	const JsonObject& obj = as_object();
	return obj.find(key) != obj.end();
}

void JsonValue::to_json(JsonValue& to) const
{
	to = *this;
}

void JsonValue::from_json(const JsonValue& from)
{
	*this = from;
}

String JsonValue::to_json_string() const
{
	String ret;

	switch (type_)
	{
	case Type::kNull:
		ret = "null";
		break;
	case Type::kBoolean:
		ret = boolean_ ? "true" : "false";
		break;
	case Type::kNumber:
		if (std::isnan(number_) || std::isinf(number_))
		{
			ret = "null";
			break;
		}
		ret = fmt::format("{}", number_);
		break;
	case Type::kString:
		ret = "\"";
		for (auto c : string_)
		{
			switch (c)
			{
			case '"': ret.append("\\\""); break;
			case '\\': ret.append("\\\\"); break;
			case '\b': ret.append("\\b"); break;
			case '\f': ret.append("\\f"); break;
			case '\n': ret.append("\\n"); break;
			case '\r': ret.append("\\r"); break;
			case '\t': ret.append("\\t"); break;
			default:
				if (c >= 0x00 && c <= 0x1f)
				{
					ret.append(fmt::format("\\u{:04x}", c));
				}
				else
				{
					ret.push_back(c);
				}
			}
		}
		ret.append("\"");
		break;
	case Type::kArray:
	{
		ret = "[";
		for (auto itr = array_.begin(); itr != array_.end(); itr++)
		{
			ret.append(JsonValue(*itr).to_json_string());
			if (std::next(itr) != array_.end())
			{
				ret.append(",");
			}
		}
		ret.append("]");
		break;
	}
	case Type::kObject:
	{
		ret = "{";
		for (auto itr = object_.begin(); itr != object_.end(); itr++)
		{
			ret.append(JsonValue(itr->first).to_json_string());
			ret.append(":");
			ret.append(JsonValue(itr->second).to_json_string());
			auto next = itr;
			++next;
			if (next != object_.end())
			{
				ret.append(",");
			}
		}
		ret.append("}");
		return ret;
	}
	}
	return ret;
}

srb2::Vector<std::byte> JsonValue::to_ubjson() const
{
	srb2::Vector<std::byte> out;

	do_to_ubjson(out);

	return out;
}

void JsonValue::value_to_ubjson(srb2::Vector<std::byte>& out)
{
	out.push_back(std::byte { 'Z' });
}

void JsonValue::value_to_ubjson(srb2::Vector<std::byte>& out, bool value)
{
	out.push_back(value ? std::byte { 'T' } : std::byte { 'F' });
}

void JsonValue::value_to_ubjson(srb2::Vector<std::byte>& out, double value)
{
	if (std::isnan(value) || std::isinf(value))
	{
		out.push_back(std::byte { 'Z' });
		return;
	}

	uint64_t num_as_int;
	std::byte buf[8];
	out.push_back(std::byte { 'D' });
	std::memcpy(&num_as_int, &value, sizeof(double));
	buf[0] = (std::byte)((num_as_int & 0xFF00000000000000) >> 56);
	buf[1] = (std::byte)((num_as_int & 0x00FF000000000000) >> 48);
	buf[2] = (std::byte)((num_as_int & 0x0000FF0000000000) >> 40);
	buf[3] = (std::byte)((num_as_int & 0x000000FF00000000) >> 32);
	buf[4] = (std::byte)((num_as_int & 0x00000000FF000000) >> 24);
	buf[5] = (std::byte)((num_as_int & 0x0000000000FF0000) >> 16);
	buf[6] = (std::byte)((num_as_int & 0x000000000000FF00) >> 8);
	buf[7] = (std::byte)((num_as_int & 0x00000000000000FF));
	for (int i = 0; i < 8; i++)
	{
		out.push_back(buf[i]);
	}
}

void JsonValue::value_to_ubjson(srb2::Vector<std::byte>& out, uint64_t value)
{

	std::byte buf[8];
	int64_t string_len_i64;
	int32_t string_len_32;
	int16_t string_len_16;
	uint8_t string_len_8;
	if (value < std::numeric_limits<uint8_t>().max())
	{
		string_len_8 = value;
		out.push_back(std::byte { 'U' });
		out.push_back((std::byte)(string_len_8));
	}
	else if (value < std::numeric_limits<int16_t>().max())
	{
		string_len_16 = value;
		buf[0] = (std::byte)((string_len_16 & 0xFF00) >> 8);
		buf[1] = (std::byte)((string_len_16 & 0x00FF) >> 0);
		out.push_back(std::byte { 'I' });
		out.push_back(buf[0]);
		out.push_back(buf[1]);
	}
	else if (value < std::numeric_limits<int32_t>().max())
	{
		string_len_32 = value;
		buf[0] = (std::byte)((string_len_32 & 0xFF000000) >> 24);
		buf[1] = (std::byte)((string_len_32 & 0x00FF0000) >> 16);
		buf[2] = (std::byte)((string_len_32 & 0x0000FF00) >> 8);
		buf[3] = (std::byte)((string_len_32 & 0x000000FF));
		out.push_back(std::byte { 'l' });
		out.push_back(buf[0]);
		out.push_back(buf[1]);
		out.push_back(buf[2]);
		out.push_back(buf[3]);
	}
	else if (value < std::numeric_limits<int64_t>().max())
	{
		string_len_i64 = value;
		buf[0] = (std::byte)((string_len_i64 & 0xFF00000000000000) >> 56);
		buf[1] = (std::byte)((string_len_i64 & 0x00FF000000000000) >> 48);
		buf[2] = (std::byte)((string_len_i64 & 0x0000FF0000000000) >> 40);
		buf[3] = (std::byte)((string_len_i64 & 0x000000FF00000000) >> 32);
		buf[4] = (std::byte)((string_len_i64 & 0x00000000FF000000) >> 24);
		buf[5] = (std::byte)((string_len_i64 & 0x0000000000FF0000) >> 16);
		buf[6] = (std::byte)((string_len_i64 & 0x000000000000FF00) >> 8);
		buf[7] = (std::byte)((string_len_i64 & 0x00000000000000FF));
		out.push_back(std::byte { 'L' });
		out.push_back(buf[0]);
		out.push_back(buf[1]);
		out.push_back(buf[2]);
		out.push_back(buf[3]);
		out.push_back(buf[4]);
		out.push_back(buf[5]);
		out.push_back(buf[6]);
		out.push_back(buf[7]);
	}
	else
	{
		throw JsonParseError("inexpressible integer");
	}
}

void JsonValue::value_to_ubjson(srb2::Vector<std::byte>& out, const String& value, bool include_type)
{
	if (include_type)
	{
		out.push_back(std::byte { 'S' });
	}

	value_to_ubjson(out, static_cast<uint64_t>(value.size()));
	for (auto c : value)
	{
		out.push_back((std::byte)(c));
	}
}

void JsonValue::value_to_ubjson(srb2::Vector<std::byte>& out, const JsonArray& value)
{
	out.push_back(std::byte { '[' });
	if (value.empty())
	{
		out.push_back(std::byte { ']' });
		return;
	}

	out.push_back(std::byte { '#' });
	value_to_ubjson(out, (uint64_t)value.size());
	for (auto& v : value)
	{
		v.do_to_ubjson(out);
	}

	// Don't emit end because we included size prefix
}

void JsonValue::value_to_ubjson(srb2::Vector<std::byte>& out, const JsonObject& value)
{
	out.push_back(std::byte { '{' });
	if (value.empty())
	{
		out.push_back(std::byte { '}' });
		return;
	}

	out.push_back(std::byte { '#' });
	value_to_ubjson(out, static_cast<uint64_t>(value.size()));
	for (auto& v : value)
	{
		value_to_ubjson(out, v.first, false);
		v.second.do_to_ubjson(out);
	}

	// Don't emit end because we included size prefix
}

void JsonValue::do_to_ubjson(srb2::Vector<std::byte>& out) const
{
	switch (type_)
	{
	case Type::kNull:
		value_to_ubjson(out);
		break;
	case Type::kBoolean:
		value_to_ubjson(out, boolean_);
		break;
	case Type::kNumber:
		value_to_ubjson(out, number_);
		break;
	case Type::kString:
		value_to_ubjson(out, string_, true);
		break;
	case Type::kArray:
		value_to_ubjson(out, array_);
		break;
	case Type::kObject:
		value_to_ubjson(out, object_);
		break;
	default:
		break;
	}
}

static uint8_t u8_from_ubjson(tcb::span<const std::byte>& ubjson)
{
	if (ubjson.size() < 1) throw JsonParseError("insufficient data");
	uint8_t ret = std::to_integer<uint8_t>(ubjson[0]);
	ubjson = ubjson.subspan(1);
	return ret;
}

static int8_t i8_from_ubjson(tcb::span<const std::byte>& ubjson)
{
	if (ubjson.size() < 1) throw JsonParseError("insufficient data");
	int8_t ret = std::to_integer<int8_t>(ubjson[0]);
	ubjson = ubjson.subspan(1);
	return ret;
}

static int16_t i16_from_ubjson(tcb::span<const std::byte>& ubjson)
{
	uint8_t b[2];
	uint16_t native;
	int16_t ret;
	if (ubjson.size() < 2) throw JsonParseError("insufficient data");
	b[0] = std::to_integer<uint8_t>(ubjson[0]);
	b[1] = std::to_integer<uint8_t>(ubjson[1]);
	native = b[0] << 8 | b[1];
	std::memcpy(&ret, &native, 2);
	ubjson = ubjson.subspan(2);
	return ret;
}

static int32_t i32_from_ubjson(tcb::span<const std::byte>& ubjson)
{
	uint8_t b[4];
	uint32_t native;
	int32_t ret;
	if (ubjson.size() < 4) throw JsonParseError("insufficient data");
	b[0] = std::to_integer<uint8_t>(ubjson[0]);
	b[1] = std::to_integer<uint8_t>(ubjson[1]);
	b[2] = std::to_integer<uint8_t>(ubjson[2]);
	b[3] = std::to_integer<uint8_t>(ubjson[3]);
	native = b[0] << 24 | b[1] << 16 | b[2] << 8 | b[3];
	std::memcpy(&ret, &native, 4);
	ubjson = ubjson.subspan(4);
	return ret;
}

static int64_t i64_from_ubjson(tcb::span<const std::byte>& ubjson)
{
	uint64_t b[8];
	uint64_t native;
	int64_t ret;
	if (ubjson.size() < 8) throw JsonParseError("insufficient data");
	b[0] = std::to_integer<uint64_t>(ubjson[0]);
	b[1] = std::to_integer<uint64_t>(ubjson[1]);
	b[2] = std::to_integer<uint64_t>(ubjson[2]);
	b[3] = std::to_integer<uint64_t>(ubjson[3]);
	b[4] = std::to_integer<uint64_t>(ubjson[4]);
	b[5] = std::to_integer<uint64_t>(ubjson[5]);
	b[6] = std::to_integer<uint64_t>(ubjson[6]);
	b[7] = std::to_integer<uint64_t>(ubjson[7]);
	native = b[0] << 56 | b[1] << 48 | b[2] << 40 | b[3] << 32 | b[4] << 24 | b[5] << 16 | b[6] << 8 | b[7];
	std::memcpy(&ret, &native, 8);
	ubjson = ubjson.subspan(8);
	return ret;
}

static float f32_from_ubjson(tcb::span<const std::byte>& ubjson)
{
	uint8_t b[8];
	uint32_t native;
	float ret;
	if (ubjson.size() < 4) throw JsonParseError("insufficient data");
	b[0] = std::to_integer<uint8_t>(ubjson[0]);
	b[1] = std::to_integer<uint8_t>(ubjson[1]);
	b[2] = std::to_integer<uint8_t>(ubjson[2]);
	b[3] = std::to_integer<uint8_t>(ubjson[3]);
	native = b[0] << 24 | b[1] << 16 | b[2] << 8 | b[3];
	std::memcpy(&ret, &native, 4);
	ubjson = ubjson.subspan(4);
	return ret;
}

static double f64_from_ubjson(tcb::span<const std::byte>& ubjson)
{
	uint64_t b[8];
	uint64_t native;
	double ret;
	if (ubjson.size() < 8) throw JsonParseError("insufficient data");
	b[0] = std::to_integer<uint64_t>(ubjson[0]);
	b[1] = std::to_integer<uint64_t>(ubjson[1]);
	b[2] = std::to_integer<uint64_t>(ubjson[2]);
	b[3] = std::to_integer<uint64_t>(ubjson[3]);
	b[4] = std::to_integer<uint64_t>(ubjson[4]);
	b[5] = std::to_integer<uint64_t>(ubjson[5]);
	b[6] = std::to_integer<uint64_t>(ubjson[6]);
	b[7] = std::to_integer<uint64_t>(ubjson[7]);
	native = b[0] << 56 | b[1] << 48 | b[2] << 40 | b[3] << 32 | b[4] << 24 | b[5] << 16 | b[6] << 8 | b[7];
	std::memcpy(&ret, &native, 8);
	ubjson = ubjson.subspan(8);
	return ret;
}

static uint64_t length_from_ubjson(tcb::span<const std::byte>& ubjson)
{
	if (ubjson.size() < 1) throw JsonParseError("insufficient data");
	uint64_t size = 0;
	switch ((char)(ubjson[0]))
	{
	case 'i':
		ubjson = ubjson.subspan(1);
		size = i8_from_ubjson(ubjson);
		break;
	case 'U':
		ubjson = ubjson.subspan(1);
		size = u8_from_ubjson(ubjson);
		break;
	case 'I':
		ubjson = ubjson.subspan(1);
		size = i16_from_ubjson(ubjson);
		break;
	case 'l':
		ubjson = ubjson.subspan(1);
		size = i32_from_ubjson(ubjson);
		break;
	case 'L':
		ubjson = ubjson.subspan(1);
		size = i64_from_ubjson(ubjson);
		break;
	default:
		throw JsonParseError("illegal data length type");
	}
	return size;
}

static String string_from_ubjson(tcb::span<const std::byte>& ubjson)
{
	uint64_t len = length_from_ubjson(ubjson);
	if (len > std::numeric_limits<size_t>().max())
	{
		throw JsonParseError("unloadable string length");
	}
	String ret;
	for (size_t i = 0; i < len; i++)
	{

	}
	auto strdata = ubjson.subspan(0, (size_t)len);
	ubjson = ubjson.subspan((size_t)len);
	for (auto itr = strdata.begin(); itr != strdata.end(); itr++)
	{
		ret.push_back((char)(*itr));
	}
	return ret;
}

template <typename F>
static void read_ubjson_array_elements(F f, JsonArray& arr, tcb::span<const std::byte>& ubjson, uint64_t len)
{
	ubjson = ubjson.subspan(1);
	for (uint64_t i = 0; i < len; i++)
	{
		arr.push_back((f)(ubjson));
	}
}

template <typename F>
static void read_ubjson_array_elements(F f, JsonArray& arr, tcb::span<const std::byte>& ubjson, uint64_t len, int depth)
{
	ubjson = ubjson.subspan(1);
	for (uint64_t i = 0; i < len; i++)
	{
		arr.push_back((f)(ubjson, depth));
	}
}

static JsonValue do_from_ubjson(tcb::span<const std::byte>& ubjson, int depth);
static JsonObject object_from_ubjson(tcb::span<const std::byte>& ubjson, int depth);

static JsonArray array_from_ubjson(tcb::span<const std::byte>& ubjson, int depth)
{
	char typecode = 0;
	if ((char)(ubjson[0]) == '$')
	{
		if (ubjson.size() < 2) throw JsonParseError("insufficient data");
		typecode = (char)(ubjson[1]);
		ubjson = ubjson.subspan(2);
	}
	bool has_len = false;
	uint64_t len = 0;
	if (ubjson.size() < 1) throw JsonParseError("insufficient data");
	if ((char)(ubjson[0]) == '#')
	{
		ubjson = ubjson.subspan(1);
		has_len = true;
		len = length_from_ubjson(ubjson);
	}
	JsonArray ret;
	if (has_len)
	{
		ret.reserve(len);
	}
	if (typecode != 0)
	{
		switch (typecode)
		{
		case 'i':
			read_ubjson_array_elements(i8_from_ubjson, ret, ubjson, len);
			break;
		case 'U':
			read_ubjson_array_elements(u8_from_ubjson, ret, ubjson, len);
			break;
		case 'I':
			read_ubjson_array_elements(i16_from_ubjson, ret, ubjson, len);
			break;
		case 'l':
			read_ubjson_array_elements(i32_from_ubjson, ret, ubjson, len);
			break;
		case 'L':
			read_ubjson_array_elements(i64_from_ubjson, ret, ubjson, len);
			break;
		case 'd':
			read_ubjson_array_elements(f32_from_ubjson, ret, ubjson, len);
			break;
		case 'D':
			read_ubjson_array_elements(f64_from_ubjson, ret, ubjson, len);
			break;
		case 'S':
			read_ubjson_array_elements(string_from_ubjson, ret, ubjson, len);
			break;
		case '[':
			read_ubjson_array_elements(array_from_ubjson, ret, ubjson, len, depth + 1);
			break;
		case '{':
			read_ubjson_array_elements(object_from_ubjson, ret, ubjson, len, depth + 1);
			break;
		default:
			throw JsonParseError("invalid typecode for array");
		}
	}
	else
	{
		if (has_len)
		{
			for (uint64_t i = 0; i < len; i++)
			{
				JsonValue v = do_from_ubjson(ubjson, depth);
				ret.push_back(std::move(v));
			}
		}
		else
		{
			if (ubjson.size() < 1) throw JsonParseError("insufficient data");
			while ((char)(ubjson[0]) != ']')
			{
				JsonValue v = do_from_ubjson(ubjson, depth);
				ret.push_back(std::move(v));
			}
			ubjson = ubjson.subspan(1);
		}
	}
	return ret;
}

template <typename F>
static void read_ubjson_object_elements(F f, JsonObject& obj, tcb::span<const std::byte>& ubjson, uint64_t len)
{
	ubjson = ubjson.subspan(1);
	for (uint64_t i = 0; i < len; i++)
	{
		String key = string_from_ubjson(ubjson);
		JsonValue value = (f)(ubjson);
		obj[std::move(key)] = std::move(value);
	}
}

template <typename F>
static void read_ubjson_object_elements(F f, JsonObject& obj, tcb::span<const std::byte>& ubjson, uint64_t len, int depth)
{
	ubjson = ubjson.subspan(1);
	for (uint64_t i = 0; i < len; i++)
	{
		String key = string_from_ubjson(ubjson);
		JsonValue value = (f)(ubjson, depth);
		obj[std::move(key)] = std::move(value);
	}
}

static JsonObject object_from_ubjson(tcb::span<const std::byte>& ubjson, int depth)
{
	char typecode = 0;
	if ((char)(ubjson[0]) == '$')
	{
		if (ubjson.size() < 2) throw JsonParseError("insufficient data");
		typecode = (char)(ubjson[1]);
		ubjson = ubjson.subspan(2);
	}
	bool has_len = false;
	uint64_t len = 0;
	if (ubjson.size() < 1) throw JsonParseError("insufficient data");
	if ((char)(ubjson[0]) == '#')
	{
		ubjson = ubjson.subspan(1);
		has_len = true;
		len = length_from_ubjson(ubjson);
	}
	JsonObject ret;
	if (has_len)
	{
		ret.rehash(len);
	}
	if (typecode != 0)
	{
		switch (typecode)
		{
		case 'i':
			read_ubjson_object_elements(i8_from_ubjson, ret, ubjson, len);
			break;
		case 'U':
			read_ubjson_object_elements(u8_from_ubjson, ret, ubjson, len);
			break;
		case 'I':
			read_ubjson_object_elements(i16_from_ubjson, ret, ubjson, len);
			break;
		case 'l':
			read_ubjson_object_elements(i32_from_ubjson, ret, ubjson, len);
			break;
		case 'L':
			read_ubjson_object_elements(i64_from_ubjson, ret, ubjson, len);
			break;
		case 'd':
			read_ubjson_object_elements(f32_from_ubjson, ret, ubjson, len);
			break;
		case 'D':
			read_ubjson_object_elements(f64_from_ubjson, ret, ubjson, len);
			break;
		case 'S':
			read_ubjson_object_elements(string_from_ubjson, ret, ubjson, len);
			break;
		case '[':
			read_ubjson_object_elements(array_from_ubjson, ret, ubjson, len, depth + 1);
			break;
		case '{':
			read_ubjson_object_elements(object_from_ubjson, ret, ubjson, len, depth + 1);
			break;
		default:
			throw JsonParseError("invalid typecode for array");
		}
	}
	else
	{
		if (has_len)
		{
			for (uint64_t i = 0; i < len; i++)
			{
				String key = string_from_ubjson(ubjson);
				JsonValue value = do_from_ubjson(ubjson, depth);
				ret[std::move(key)] = std::move(value);
			}
		}
		else
		{
			if (ubjson.size() < 1) throw JsonParseError("insufficient data");
			while ((char)(ubjson[0]) != '}')
			{
				String key = string_from_ubjson(ubjson);
				JsonValue value = do_from_ubjson(ubjson, depth);
				ret[std::move(key)] = std::move(value);
			}
			ubjson = ubjson.subspan(1);
		}
	}
	return ret;
}

static JsonValue do_from_ubjson(tcb::span<const std::byte>& ubjson, int depth)
{
	if (depth > 1000)
	{
		throw JsonParseError("ubjson depth limit exceeded");
	}

	if (ubjson.empty())
	{
		throw JsonParseError("empty ubjson payload");
	}

	char typecode = (char)(ubjson[0]);
	switch (typecode)
	{
	case 'Z':
		ubjson = ubjson.subspan(1);
		return JsonValue();
	case 'F':
		ubjson = ubjson.subspan(1);
		return JsonValue(false);
	case 'T':
		ubjson = ubjson.subspan(1);
		return JsonValue(true);
	case 'i':
		ubjson = ubjson.subspan(1);
		return i8_from_ubjson(ubjson);
	case 'U':
		ubjson = ubjson.subspan(1);
		return u8_from_ubjson(ubjson);
	case 'I':
		ubjson = ubjson.subspan(1);
		return i16_from_ubjson(ubjson);
	case 'l':
		ubjson = ubjson.subspan(1);
		return i32_from_ubjson(ubjson);
	case 'L':
		ubjson = ubjson.subspan(1);
		return i64_from_ubjson(ubjson);
	case 'd':
		ubjson = ubjson.subspan(1);
		return f32_from_ubjson(ubjson);
	case 'D':
		ubjson = ubjson.subspan(1);
		return f64_from_ubjson(ubjson);
	case 'S':
		ubjson = ubjson.subspan(1);
		return string_from_ubjson(ubjson);
	case '[':
		ubjson = ubjson.subspan(1);
		return array_from_ubjson(ubjson, depth);
	case '{':
		ubjson = ubjson.subspan(1);
		return object_from_ubjson(ubjson, depth);
	default:
		throw JsonParseError(fmt::format("unrecognized ubjson typecode 0x{:02x}", typecode));
	}
}

JsonValue JsonValue::from_ubjson(tcb::span<const std::byte> ubjson)
{
	return do_from_ubjson(ubjson, 0);
}

namespace
{
struct Token
{
	enum class Type
	{
		kEof,
		kOpenCurly,
		kCloseCurly,
		kColon,
		kOpenSquare,
		kCloseSquare,
		kComma,
		kBoolean,
		kString,
		kNumber,
		kNull
	};
	Type type = Type::kEof;
	std::string_view slice;
};

class Tokenizer
{
	std::string_view in_;

	void consume(size_t len);
public:
	Tokenizer(const std::string_view& in) : in_(in) {}
	Token peek();
	Token next();
};

Token Tokenizer::peek()
{
	Token ret;
	while (!in_.empty() && ret.type == Token::Type::kEof)
	{
		unsigned char next = in_[0];
		switch (next)
		{
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			in_.remove_prefix(1);
			break;
		case 'n':
			if (in_.size() < 4) throw JsonParseError("reached end of buffer parsing null");
			ret = Token { Token::Type::kNull, in_.substr(0, 4) };
			if (ret.slice != "null") throw JsonParseError("invalid null token");
			break;
		case 'f':
			if (in_.size() < 5) throw JsonParseError("reached end of buffer parsing false");
			ret = Token { Token::Type::kBoolean, in_.substr(0, 5) };
			if (ret.slice != "false") throw JsonParseError("invalid boolean token");
			break;
		case 't':
			if (in_.size() < 4) throw JsonParseError("reached end of buffer parsing true");
			ret = Token { Token::Type::kBoolean, in_.substr(0, 4) };
			if (ret.slice != "true") throw JsonParseError("invalid boolean token");
			break;
		case '{':
			ret = Token { Token::Type::kOpenCurly, in_.substr(0, 1) };
			break;
		case '}':
			ret = Token { Token::Type::kCloseCurly, in_.substr(0, 1) };
			break;
		case '[':
			ret = Token { Token::Type::kOpenSquare, in_.substr(0, 1) };
			break;
		case ']':
			ret = Token { Token::Type::kCloseSquare, in_.substr(0, 1) };
			break;
		case ':':
			ret = Token { Token::Type::kColon, in_.substr(0, 1) };
			break;
		case ',':
			ret = Token { Token::Type::kComma, in_.substr(0, 1) };
			break;
		case '"':
		{
			bool skipnextquote = false;
			size_t len;
			for (len = 1; len < in_.size(); len++)
			{
				char c = in_[len];
				bool shouldbreak = false;
				switch (c)
				{
				case '\r':
					throw JsonParseError("illegal carriage return in string literal");
				case '\n':
					throw JsonParseError("illegal line feed in string literal");
				case '\\':
					skipnextquote = true;
					break;
				case '"':
					if (skipnextquote) skipnextquote = false;
					else shouldbreak = true;
					break;
				default:
					if (skipnextquote) skipnextquote = false;
				}
				if (shouldbreak) break;
			}
			if (in_[len] != '"') throw JsonParseError("found unterminated string");
			ret = Token { Token::Type::kString, in_.substr(0, len + 1) };
			break;
		}
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		{
			size_t len;
			bool firstiszero = next == '0';
			bool zeroseen = next == '0';
			bool integerseen = next >= '0' && next <= '9';
			bool periodseen = false;
			bool periodlast = false;
			bool exponentseen = false;
			for (len = 1; len < in_.size(); len++)
			{
				char c = in_[len];
				bool shouldbreak = false;
				switch (c)
				{
				default:
					shouldbreak = true;
					break;
				case '.':
					if (periodseen || exponentseen || (!periodseen && !zeroseen && !integerseen))
						throw JsonParseError("unexpected period in number token");
					periodseen = true;
					periodlast = true;
					break;
				case '0':
					if (firstiszero && len == 1) throw JsonParseError("more than 1 preceding 0");
					if ((next == '-' || next == '+') && len == 1) firstiszero = true;
					zeroseen = true;
					integerseen = true;
					break;
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (firstiszero && !periodseen && !exponentseen) throw JsonParseError("nonzero integral part of number preceded by 0");
					periodlast = false;
					integerseen = true;
					break;
				case 'e':
				case 'E':
					if (exponentseen) throw JsonParseError("multiple exponent");
					exponentseen = true;
					break;
				case '+':
				case '-':
					if (!exponentseen && len != 0) throw JsonParseError("sign after number started");
					break;
				}
				if (shouldbreak) break;
			}
			if (periodlast) throw JsonParseError("real number without fractional part");
			ret = Token { Token::Type::kNumber, in_.substr(0, len) };
			break;
		}
		default:
			throw JsonParseError(fmt::format("unexpected character {:c}", next));
		}
	}
	std::string copy { ret.slice };
	return ret;
}

void Tokenizer::consume(size_t len)
{
	in_.remove_prefix(len);
}

Token Tokenizer::next()
{
	Token peeked = peek();
	consume(peeked.slice.size());
	std::string p { peeked.slice };
	return peeked;
}

} // namespace

static JsonValue parse_value(Tokenizer& tokenizer, int depth);

static JsonValue parse_number(const Token& token)
{
	std::string_view s { token.slice };
	if (s.empty()) throw JsonParseError("empty number token");
	while (s[0] == ' ' || s[0] == '\t' || s[0] == '\n' || s[0] == '\r')
	{
		s.remove_prefix(1);
	}

	bool negative = false;
	if (s[0] == '-')
	{
		negative = true;
		s.remove_prefix(1);
	}
	else if (s[0] == '+')
	{
		negative = false;
		s.remove_prefix(1);
	}

	if (s.empty())
	{
		throw JsonParseError("only sign present on number");
	}

	std::string_view::const_iterator integral_start = s.begin();
	std::string_view::const_iterator integral_end;
	bool decimal_found = false;
	std::string_view::const_iterator decimal;
	size_t pos = 0;
	while (pos < s.size())
	{
		if (s[pos] == '.')
		{
			decimal_found = true;
			decimal = std::next(s.begin(), pos);
			integral_end = std::next(s.begin(), pos);
			pos += 1;
			break;
		}
		else if (s[pos] < '0' || s[pos] > '9')
		{
			integral_end = std::next(s.begin(), pos - 1);
			break;
		}
		integral_end = std::next(s.begin(), pos + 1);
		pos += 1;
	}

	std::string_view::const_iterator decimal_start = s.end();
	std::string_view::const_iterator decimal_end = s.end();
	std::string_view::const_iterator exponent_start = s.end();
	std::string_view::const_iterator exponent_end = s.end();
	bool should_have_exponent = false;
	if (decimal_found && (decimal + 1) < s.end())
	{
		decimal_start = decimal + 1;
	}
	while (pos < s.size())
	{
		// ingest decimal
		if (s[pos] == 'E' || s[pos] == 'e')
		{
			if (decimal_start != s.end()) decimal_end = s.begin();
			exponent_start = std::next(s.begin(), pos + 1);
			should_have_exponent = true;
			pos += 1;
			break;
		}
		else if ((s[pos] < '0' || s[pos] > '9') && s[pos] != '+' && s[pos] != '-')
		{
			throw JsonParseError("invalid character after decimal");
		}
		decimal_end = std::next(s.begin(), pos + 1);
		pos += 1;
	}

	bool exponent_negative = false;

	if (should_have_exponent)
	{
		if (pos >= s.size())
		{
			throw JsonParseError("exponent started but not specified");
		}
		bool exponent_was_signed = false;
		while (!s.empty())
		{
			if (s[pos] == '-')
			{
				if (exponent_was_signed) throw JsonParseError("multiple signs on exponent");
				exponent_negative = true;
				exponent_start++;
				exponent_was_signed = true;
				pos += 1;
				continue;
			}
			else if (s[pos] == '+')
			{
				if (exponent_was_signed) throw JsonParseError("multiple signs on exponent");
				exponent_start++;
				exponent_was_signed = true;
				pos += 1;
				continue;
			}

			if (s[pos] < '0' || s[pos] > '9')
			{
				throw JsonParseError("invalid character after exponent");
			}
			exponent_end = std::next(s.begin(), pos + 1);
			pos += 1;
		}
		if ((exponent_end - exponent_start) == 0)
		{
			throw JsonParseError("exponent started but not specified");
		}
	}

	std::string_view integral_view = "";
	if (integral_start != s.end())
	{
		integral_view = std::string_view { &*integral_start, (size_t)(integral_end - integral_start) };
	}
	std::string_view decimal_view = "";
	if (decimal_start != s.end())
	{
		decimal_view = std::string_view { &*decimal_start, (size_t)(decimal_end - decimal_start) };
	}
	std::string_view exponent_view = "";
	if (exponent_start != s.end())
	{
		std::string_view { &*exponent_start, (size_t)(exponent_end - exponent_start) };
	}

	if (should_have_exponent && decimal_start != s.end() && decimal_view.empty())
	{
		throw JsonParseError("exponent after decimal but no decimal value");
	}
	if (should_have_exponent && exponent_view.empty())
	{
		throw JsonParseError("no exponent despite e/E +/-");
	}
	// if (!exponent_negative && exponent_view == "1")
	// {
	// 	throw JsonParseError("exponent of 1 not allowed");
	// }

	double number = 0.0;
	uint64_t integral_int = 0;
	for (auto i : integral_view)
	{
		integral_int = 10 * integral_int + (i - '0');
	}
	double decimal_value = 0.0;
	for (auto i : decimal_view)
	{
		decimal_value = (decimal_value / 10) + ((double)(i - '0') / 10);
	}
	uint64_t exponent_int = 0;
	for (auto i : exponent_view)
	{
		exponent_int = 10 * exponent_int + (i - '0');
	}
	if (negative)
	{
		integral_int *= -1;
	}
	if (exponent_negative)
	{
		exponent_int *= -1;
	}
	number = std::pow(10, exponent_int) * (double)integral_int + decimal_value;

	return JsonValue(number);
}

static char hexconv(char c)
{
	switch (c)
	{
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
		c += 32;
		break;
	default:
		break;
	}
	switch (c)
	{
	case '0': return 0;
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case 'a': return 10;
	case 'b': return 11;
	case 'c': return 12;
	case 'd': return 13;
	case 'e': return 14;
	case 'f': return 15;
	default: throw JsonParseError("illegal unicode escape sequence character");
	}
}

static constexpr bool is_surrogate(uint16_t code)
{
	return (0xf800 & code) == 0xd800;
}

static constexpr bool is_high_surrogate(uint16_t code)
{
	return (code & 0xfc00) == 0xd800;
}

static constexpr bool is_low_surrogate(uint16_t code)
{
	return (code & 0xfc00) == 0xdc00;
}

static constexpr uint32_t merge_surrogate_pair(uint16_t low, uint16_t high)
{
	return ((high - 0xd800) << 10) + ((low - 0xdc00) + 0x10000);
}

static JsonValue parse_string(const Token& token)
{
	std::string_view read { token.slice.substr(1, token.slice.size() - 2) };
	String conv;
	for (auto itr = read.begin(); itr != read.end();)
	{
		int c = *itr & 0xFF;
		switch (c)
		{
		case '\\':
		{
			// Reverse solidus indicates escape sequence
			if (std::next(itr) == read.end()) throw JsonParseError("unterminated string escape sequence");
			char control = *++itr;
			switch (control)
			{
			case '"':
			case '\\':
			case '/':
				conv.push_back(control);
				++itr;
				break;
			case 'b':
				conv.push_back('\b');
				++itr;
				break;
			case 'f':
				conv.push_back('\f');
				++itr;
				break;
			case 'n':
				conv.push_back('\n');
				++itr;
				break;
			case 'r':
				conv.push_back('\r');
				++itr;
				break;
			case 't':
				conv.push_back('\t');
				++itr;
				break;
			case 'x':
			{
				if (std::distance(itr, read.end()) < 3) throw JsonParseError("unterminated hex char sequence");
				char hex[2];
				hex[0] = *++itr;
				hex[1] = *++itr;
				char byte = (hexconv(hex[0]) << 4) | hexconv(hex[1]);
				if (byte < 0x20) throw JsonParseError("bad escaped control code");
				conv.push_back(byte);
				++itr;
				break;
			}
			case 'u':
			{
				if (std::distance(itr, read.end()) < 5) throw JsonParseError("unterminated utf16 code sequence");
				// Next 4 characters are a hex sequence representing a UTF-16 surrogate
				// We have to do silly things to make this work correctly
				char hex[4];
				hex[0] = *++itr;
				hex[1] = *++itr;
				hex[2] = *++itr;
				hex[3] = *++itr;
				if (hex[0] == -1 || hex[1] == -1 || hex[2] == -1 || hex[3] == -1)
					throw JsonParseError("invalid unicode escape");
				uint16_t utf16 = hexconv(hex[0]) << 12 | hexconv(hex[1]) << 8 | hexconv(hex[2]) << 4 | hexconv(hex[3]);
				bool valid_codepoint = false;
				uint32_t codepoint = 0;
				if (is_low_surrogate(utf16))
				{
					// invalid surrogate pair -- high must precede low
					conv.push_back('\\');
					conv.push_back('u');
					conv.push_back(hex[0]);
					conv.push_back(hex[1]);
					conv.push_back(hex[2]);
					conv.push_back(hex[3]);
				}
				else if (is_high_surrogate(utf16))
				{
					if (std::distance(itr, read.end()) < 6)
					{
						// invalid surrogate pair -- high must precede low
						conv += "\\u";
					}
					else
					{
						// potentially valid...
						if (
							*itr == '\\' &&
							*(itr + 1) == 'u' &&
							(hex[0] = *(itr + 2)) != -1 &&
							(hex[1] = *(itr + 3)) != -1 &&
							(hex[2] = *(itr + 4)) != -1 &&
							(hex[3] = *(itr + 5)) != -1
						)
						{
							uint16_t utf16_2 = hexconv(hex[0]) << 12 | hexconv(hex[1]) << 8 | hexconv(hex[2]) << 4 | hexconv(hex[3]);
							if (is_low_surrogate(utf16_2))
							{
								itr += 6;
								codepoint = merge_surrogate_pair(utf16_2, utf16);
								valid_codepoint = true;
							}
							else
							{
								itr += 2;
								conv += "\\u";
							}
						}
						else
						{
							conv += "\\u";
						}
					}
				}
				else
				{
					// non-surrogate represents unicode codepoint
					codepoint = utf16;
					valid_codepoint = true;
				}

				if (valid_codepoint)
				{
					char encoded[4];
					int len;
					// encode codepoint as UTF-8
					if (codepoint <= 0x7f)
					{
						encoded[0] = codepoint;
						len = 1;
					}
					else if (codepoint <= 0x7ff)
					{
						encoded[0] = 0300 | (codepoint >> 6);
						encoded[1] = 0200 | (codepoint & 077);
						len = 2;
					}
					else if (codepoint <= 0xffff)
					{
						if (is_surrogate(codepoint))
						{
							codepoint = 0xfffd;
						}
						encoded[0] = 0340 | (codepoint >> 2);
						encoded[1] = 0200 | ((codepoint >> 6) & 077);
						encoded[2] = 0200 | (codepoint & 077);
						len = 3;
					}
					else if (~(codepoint >> 18) & 007)
					{
						encoded[0] = 0360 | (codepoint >> 18);
						encoded[1] = 0200 | ((codepoint >> 12) & 077);
						encoded[2] = 0200 | ((c >> 6) & 077);
						encoded[3] = 0200 | (c & 077);
						len = 4;
					}
					else
					{
						encoded[0] = 0xef;
						encoded[1] = 0xbf;
						encoded[2] = 0xbd;
						len = 3;
					}
					conv.append(encoded, len);
				}

				++itr;
				break;
			}
			default:
				throw JsonParseError("invalid string escape control code");
			}
			break;
		}
		default:
			if (c < 0x20) throw JsonParseError("unescaped control code");
			conv.push_back(c);
			++itr;
			break;
		}
	}
	return JsonValue(conv);
}

static JsonValue parse_object(Tokenizer& tokenizer, int depth)
{
	JsonObject obj;
	bool done = false;
	if (tokenizer.peek().type == Token::Type::kCloseCurly)
	{
		tokenizer.next();
		return obj;
	}
	while (!done)
	{
		Token key_token = tokenizer.next();
		if (key_token.type != Token::Type::kString) throw JsonParseError("unexpected token; expected string (for key)");
		String key_string = parse_string(key_token).get<String>();
		Token colon = tokenizer.next();
		if (colon.type != Token::Type::kColon) throw JsonParseError("unexpected token; expected colon (after key)");
		JsonValue value = parse_value(tokenizer, depth + 1);
		Token last = tokenizer.next();
		if (last.type == Token::Type::kCloseCurly) done = true;
		else if (last.type != Token::Type::kComma) throw JsonParseError("unexpected token; expected comma (after value)");

		obj.insert_or_assign(std::move(key_string), std::move(value));
	}
	return obj;
}

static JsonArray parse_array(Tokenizer& tokenizer, int depth)
{
	JsonArray arr;
	bool done = false;
	if (tokenizer.peek().type == Token::Type::kCloseSquare)
	{
		tokenizer.next();
		return arr;
	}
	while (!done)
	{
		JsonValue value = parse_value(tokenizer, depth + 1);
		Token last = tokenizer.next();
		if (last.type == Token::Type::kCloseSquare) done = true;
		else if (last.type != Token::Type::kComma) throw JsonParseError("unexpected token; expected comma (after value)");
		arr.push_back(value);
	}
	return arr;
}

constexpr const int kMaxDepth = 1000;

static JsonValue parse_value(Tokenizer& tokenizer, int depth)
{
	using Type = Token::Type;
	JsonValue ret;
	Token token = tokenizer.next();

	if (depth >= kMaxDepth)
	{
		throw JsonParseError("parse depth limit exceeded");
	}

	switch (token.type)
	{
	case Type::kNull:
		ret = JsonValue();
		break;
	case Type::kBoolean:
		if (token.slice == "true") ret = JsonValue(true);
		else if (token.slice == "false") ret = JsonValue(false);
		else throw JsonParseError("illegal boolean token");
		break;
	case Type::kNumber:
		ret = parse_number(token);
		break;
	case Type::kString:
		ret = parse_string(token);
		break;
	case Type::kOpenCurly:
		ret = parse_object(tokenizer, depth);
		break;
	case Type::kOpenSquare:
		ret = parse_array(tokenizer, depth);
		break;
	case Type::kEof:
		throw JsonParseError("reached EOF before parsing value");
	default:
		throw JsonParseError("unexpected token");
	}

	return ret;
}

JsonValue JsonValue::from_json_string(const String& str)
{
	JsonValue ret;
	Tokenizer tokenizer { str };
	ret = parse_value(tokenizer, 0);

	Token peek = tokenizer.peek();
	if (peek.type != Token::Type::kEof) throw JsonParseError("unexpected token after expression");
	return ret;
}

JsonValue::JsonValue(bool value) : type_(Type::kBoolean), boolean_(value) {}
JsonValue::JsonValue(float value) : type_(Type::kNumber), number_((float)value) {}
JsonValue::JsonValue(double value) : type_(Type::kNumber), number_((double)value) {}
JsonValue::JsonValue(int8_t value) : type_(Type::kNumber), number_((int8_t)value) {}
JsonValue::JsonValue(int16_t value) : type_(Type::kNumber), number_((int16_t)value) {}
JsonValue::JsonValue(int32_t value) : type_(Type::kNumber), number_((int32_t)value) {}
JsonValue::JsonValue(int64_t value) : type_(Type::kNumber), number_((int64_t)value) {}
JsonValue::JsonValue(uint8_t value) : type_(Type::kNumber), number_((uint8_t)value) {}
JsonValue::JsonValue(uint16_t value) : type_(Type::kNumber), number_((double)value) {}
JsonValue::JsonValue(uint32_t value) : type_(Type::kNumber), number_((double)value) {}
JsonValue::JsonValue(uint64_t value) : type_(Type::kNumber), number_((double)value) {}
JsonValue::JsonValue(const String& value) : type_(Type::kString), string_(value) {}
JsonValue::JsonValue(String&& value) : type_(Type::kString), string_(std::move(value)) {}
JsonValue::JsonValue(const JsonArray& value) : type_(Type::kArray), array_(value) {}
JsonValue::JsonValue(JsonArray&& value) : type_(Type::kArray), array_(std::move(value)) {}
JsonValue::JsonValue(const JsonObject& value) : type_(Type::kObject), object_(value) {}
JsonValue::JsonValue(JsonObject&& value) : type_(Type::kObject), object_(std::move(value)) {}

bool JsonValue::operator==(const JsonValue& rhs) const
{
	if (type_ != rhs.type_)
	{
		return false;
	}
	switch (type_)
	{
	case Type::kNull:
		return true;
	case Type::kBoolean:
		return boolean_ == rhs.boolean_;
	case Type::kNumber:
		return number_ == rhs.number_;
	case Type::kString:
		return string_ == rhs.string_;
	case Type::kArray:
		return array_ == rhs.array_;
	case Type::kObject:
		return object_ == rhs.object_;
	}
	return false;
}

void srb2::to_json(JsonValue& to, bool value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, int8_t value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, int16_t value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, int32_t value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, int64_t value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, uint8_t value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, uint16_t value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, uint32_t value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, uint64_t value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, float value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, double value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, const String& value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, const JsonArray& value) { to = JsonValue(value); }
void srb2::to_json(JsonValue& to, const JsonObject& value) { to = JsonValue(value); }

void srb2::to_json(JsonValue& to, const std::string& v)
{
	to = JsonValue(static_cast<String>(v));
}

void srb2::from_json(const JsonValue& from, bool& value) { value = from.get<bool>(); }
void srb2::from_json(const JsonValue& from, int8_t& value) { value = from.get<int8_t>(); }
void srb2::from_json(const JsonValue& from, int16_t& value) { value = from.get<int16_t>(); }
void srb2::from_json(const JsonValue& from, int32_t& value) { value = from.get<int32_t>(); }
void srb2::from_json(const JsonValue& from, int64_t& value) { value = from.get<int64_t>(); }
void srb2::from_json(const JsonValue& from, uint8_t& value) { value = from.get<uint8_t>(); }
void srb2::from_json(const JsonValue& from, uint16_t& value) { value = from.get<uint16_t>(); }
void srb2::from_json(const JsonValue& from, uint32_t& value) { value = from.get<uint32_t>(); }
void srb2::from_json(const JsonValue& from, uint64_t& value) { value = from.get<uint64_t>(); }
void srb2::from_json(const JsonValue& from, float& value) { value = from.get<float>(); }
void srb2::from_json(const JsonValue& from, double& value) { value = from.get<double>(); }
void srb2::from_json(const JsonValue& from, String& value) { value = from.get<String>(); }
void srb2::from_json(const JsonValue& from, JsonArray& value) { value = from.get<JsonArray>(); }
void srb2::from_json(const JsonValue& from, JsonObject& value) { value = from.get<JsonObject>(); }

void srb2::from_json(const JsonValue& from, std::string& to)
{
	to = static_cast<std::string>(from.get<String>());
}

template class srb2::Vector<srb2::JsonValue>;
template class srb2::HashMap<srb2::String, srb2::JsonValue>;
