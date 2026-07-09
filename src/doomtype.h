// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by Kart Krew.
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2000 by DooM Legacy Team.
// Copyright (C) 1996 by id Software, Inc.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  doomtype.h
/// \brief SRB2 standard types
///
///        Simple basic typedefs, isolated here to make it easier
///        separating modules.

#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#include <string.h>
#include <stdint.h>

#include "config.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
// win32 sucks
#undef min
#undef max
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 7.18.1.1  Exact-width integer types */

#ifndef _MSC_VER
#define UINT8 uint8_t

#define UINT16 uint16_t
#define INT16 int16_t

#define INT32 int32_t
#define UINT32 uint32_t
#define INT64  int64_t
#define UINT64 uint64_t
#endif

#define SINT8 int8_t

/* Strings and some misc platform specific stuff */

char *nongnu_strcasestr(const char *in, const char *what);
#ifndef HAVE_STRCASESTR
#define strcasestr nongnu_strcasestr
#endif
#if !defined(HAVE_STRCASECMP) && defined(HAVE_STRICMP)
#define strcasecmp stricmp
#endif
#if !defined(HAVE_STRNCASECMP) && defined(HAVE_STRNICMP)
#define strncasecmp strnicmp
#endif
#if !defined(HAVE_STRICMP) && defined(HAVE_STRCASECMP)
#define stricmp strcasecmp
#endif
#if !defined(HAVE_STRNICMP) && defined(HAVE_STRNCASECMP)
#define strnicmp strncasecmp
#endif


int srb2_strupr(char *n); // from dosstr.c
int srb2_strlwr(char *n); // from dosstr.c
#ifndef HAVE_STRUPR
#define strupr srb2_strupr
#endif
#ifndef HAVE_STRLWR
#define strlwr srb2_strlwr
#endif

#include <stddef.h> // for size_t

size_t srb2_strlcat(char *dst, const char *src, size_t siz);
size_t srb2_strlcpy(char *dst, const char *src, size_t siz);
#ifndef HAVE_STRLCAT
#define strlcat srb2_strlcat
#endif
#ifndef HAVE_STRLCPY
#define strlcpy srb2_strlcpy
#endif

// Macro for use with char foo[FOOSIZE+1] type buffers.
// Never use this with a buffer that is a "char *" or passed
// into the function as an argument.
//
// In those cases sizeof will return the size of the pointer,
// not the number of bytes in the buffer.
#define STRBUFCPY(dst,src) strlcpy(dst, src, sizeof dst)

/* Boolean type definition */

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifndef _WIN32
typedef int32_t boolean;
#else
// Did you know including windows.h on mingw typedefs boolean?
// I wonder how long this has quietly broken rpcndr.h.
// Anyway, this should probably be fixed. God I hate WIN32.
#define boolean BOOL
#endif

/* Compiler-specific attributes and other macros */

#ifdef __GNUC__ // __attribute__ ((X))
	#define FUNCNORETURN __attribute__ ((noreturn))
	#define FUNCPRINTF __attribute__ ((format(printf, 1, 2)))
	#define FUNCDEBUG  __attribute__ ((format(printf, 2, 3)))
	#define FUNCIERROR __attribute__ ((format(printf, 1, 2),noreturn))
	#define FUNCMATH __attribute__((const))
	#define FUNCDEAD __attribute__ ((deprecated))
	#define FUNCINLINE __attribute__((always_inline))
	#define FUNCNONNULL __attribute__((nonnull))
	#define FUNCNOINLINE __attribute__((noinline))

	#ifdef __i386__ // i386 only
		#define FUNCTARGET(X)  __attribute__ ((__target__ (X)))
	#endif

	#if defined(__MINGW32__) && !defined(__clang__) // MinGW, not Clang
		#define ATTRPACK __attribute__((packed, gcc_struct))
	#else
		#define ATTRPACK __attribute__((packed))
	#endif

	#define ATTRUNUSED __attribute__((unused))
	#define ATTRUNOPTIMIZE __attribute__((optimize("O0")))
#elif defined (_MSC_VER)
	#define ATTRNORETURN __declspec(noreturn)
	#define ATTRINLINE __forceinline
	#define ATTRNOINLINE __declspec(noinline)
#endif

#ifndef FUNCPRINTF
#define FUNCPRINTF
#endif
#ifndef FUNCDEBUG
#define FUNCDEBUG
#endif
#ifndef FUNCNORETURN
#define FUNCNORETURN
#endif
#ifndef FUNCIERROR
#define FUNCIERROR
#endif
#ifndef FUNCMATH
#define FUNCMATH
#endif
#ifndef FUNCDEAD
#define FUNCDEAD
#endif
#ifndef FUNCINLINE
#define FUNCINLINE
#endif
#ifndef FUNCNONNULL
#define FUNCNONNULL
#endif
#ifndef FUNCNOINLINE
#define FUNCNOINLINE
#endif
#ifndef FUNCTARGET
#define FUNCTARGET(x)
#endif
#ifndef ATTRPACK
#define ATTRPACK
#endif
#ifndef ATTRUNUSED
#define ATTRUNUSED
#endif
#ifndef ATTRNORETURN
#define ATTRNORETURN
#endif
#ifndef ATTRINLINE
#define ATTRINLINE inline
#endif
#ifndef ATTRNOINLINE
#define ATTRNOINLINE
#endif

/* Miscellaneous types that don't fit anywhere else (Can this be changed?) */

typedef struct
{
	UINT8 red;
	UINT8 green;
	UINT8 blue;
	UINT8 alpha;
} byteColor_t;

union FColorRGBA
{
	UINT32 rgba;
	byteColor_t s;
} ATTRPACK;
typedef union FColorRGBA RGBA_t;

typedef enum
{
	postimg_none,
	postimg_water,
	postimg_motion,
	postimg_flip,
	postimg_heat,
	postimg_mirror
} postimg_t;

typedef UINT32 lumpnum_t; // 16 : 16 unsigned long (wad num: lump num)
#define LUMPERROR UINT32_MAX

typedef UINT32 tic_t;
#define INFTICS UINT32_MAX

#include "endian.h" // This is needed to make sure the below macro acts correctly in big endian builds

#ifdef SRB2_BIG_ENDIAN
#define UINT2RGBA(a) a
#else
#define UINT2RGBA(a) (UINT32)((a&0xff)<<24)|((a&0xff00)<<8)|((a&0xff0000)>>8)|(((UINT32)a&0xff000000)>>24)
#endif

#define TOSTR(x) #x
#define TOSTR2(x) TOSTR(x) // expand x first

/* preprocessor dumb and needs second macro to expand input */
#define WSTRING2(s) L ## s
#define WSTRING(s) WSTRING2 (s)

/*
A hack by Monster Iestyn: Return a pointer to a field of
a struct from a pointer to another field in the struct.
Needed for some lua shenanigans.
*/
#define FIELDFROM( type, field, have, want ) \
	(void *)((intptr_t)(field) - offsetof (type, have) + offsetof (type, want))

typedef UINT8 bitarray_t;

#define BIT_ARRAY_SIZE(n) (((n) + 7) >> 3)

static inline int
in_bit_array (const bitarray_t * const array, const int value)
{
	return (array[value >> 3] & (1<<(value & 7)));
}

static inline void
set_bit_array (bitarray_t * const array, const int value)
{
	array[value >> 3] |= (1<<(value & 7));
}

static inline void
unset_bit_array (bitarray_t * const array, const int value)
{
	array[value >> 3] &= ~(1<<(value & 7));
}

typedef UINT64 precise_t;

#define intsign(n) \
	((n) < 0 ? -1 : (n) > 0 ? 1 : 0)

// ISO C forbids function pointer -> void pointer cast but
// if it's wrapped in a struct, we can take a pointer to
// that struct and it's fine...

// Cast function pointer to (void*)
typedef union {
    void (*f)(void);
    void *v;
} func_ptr_cast_union;

#define FUNCPTRCAST(p) (((func_ptr_cast_union){(void(*)(void))(p)}).v)

#include "typedef.h"

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__DOOMTYPE__
