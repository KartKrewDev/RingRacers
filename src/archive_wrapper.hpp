// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by James Robert Roman
// Copyright (C) 2025 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------

#ifndef archive_wrapper_hpp
#define archive_wrapper_hpp

#include <type_traits>

#include "typedef.h"

#include "cxxutil.hpp"
#include "byteptr.h"
#include "doomdef.h" // p_saveg.h
#include "doomtype.h"
#include "m_fixed.h"
#include "p_mobj.h" // p_saveg.h
#include "p_saveg.h"

namespace srb2
{

struct ArchiveWrapperBase
{
	explicit ArchiveWrapperBase(savebuffer_t* save) : p_(save->p) {}

protected:
	template <typename T, typename U>
	void read(U& n) = delete;

	template <typename T, typename U>
	void write(const U& n) = delete;

private:
	UINT8*& p_;
};

#define READ_SPEC(T, arg) \
	template <> void ArchiveWrapperBase::read<T, T>(T& arg)

#define WRITE_SPEC(T, arg) \
	template <> void ArchiveWrapperBase::write<T, T>(const T& arg)

#define MACRO_PAIR(T) \
	READ_SPEC(T, n) { n = READ ## T(p_); } \
	WRITE_SPEC(T, n) { WRITE ## T(p_, n); }

MACRO_PAIR(UINT8)
MACRO_PAIR(SINT8)
MACRO_PAIR(UINT16)
MACRO_PAIR(INT16)
MACRO_PAIR(UINT32)
MACRO_PAIR(INT32)

READ_SPEC(vector2_t, n)
{
	read<fixed_t>(n.x);
	read<fixed_t>(n.y);
}

WRITE_SPEC(vector2_t, n)
{
	write<fixed_t>(n.x);
	write<fixed_t>(n.y);
}

READ_SPEC(vector3_t, n)
{
	read<fixed_t>(n.x);
	read<fixed_t>(n.y);
	read<fixed_t>(n.z);
}

WRITE_SPEC(vector3_t, n)
{
	write<fixed_t>(n.x);
	write<fixed_t>(n.y);
	write<fixed_t>(n.z);
}

// Specializations for boolean, so it can stored as char
// instead of int.

template <>
void ArchiveWrapperBase::read<bool, boolean>(boolean& n)
{
	SRB2_ASSERT(n == true || n == false);
	n = READUINT8(p_);
}

template <>
void ArchiveWrapperBase::write<bool, boolean>(const boolean& n)
{
	SRB2_ASSERT(n == true || n == false);
	WRITEUINT8(p_, n);
}

#undef MACRO_PAIR
#undef WRITE_SPEC
#undef READ_SPEC

struct ArchiveWrapper : ArchiveWrapperBase
{
	using ArchiveWrapperBase::ArchiveWrapperBase;

	template <typename T, typename U>
	void operator()(const U& n) { write<T, U>(n); }
};

struct UnArchiveWrapper : ArchiveWrapperBase
{
	using ArchiveWrapperBase::ArchiveWrapperBase;

	template <typename T, typename U>
	void operator()(U& n) { read<T, U>(n); }
};

template <typename T>
struct is_archive_wrapper : std::is_base_of<ArchiveWrapperBase, T> {};

template <typename T>
inline constexpr bool is_archive_wrapper_v = is_archive_wrapper<T>::value;

#define SRB2_ARCHIVE_WRAPPER_CALL(ar, T, var) ((ar). template operator()<T>(var))

}; // namespace

#endif/*archive_wrapper_hpp*/
