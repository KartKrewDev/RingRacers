/*
   Copyright (c) 2024,      The Mumble Developers
   Copyright (c) 2007-2009, Xiph.Org Foundation
   Copyright (c) 2007-2008, CSIRO
   Copyright (c) 2003-2008, Jean-Marc Valin

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
   @file arch.h
   @brief Various architecture definitions for ReNameNoise
*/

#ifndef ARCH_H
#define ARCH_H

#include "common.h"
#include "renamenoise_types.h"

#if !defined(__GNUC_PREREQ)
#	if defined(__GNUC__) && defined(__GNUC_MINOR__)
#		define __GNUC_PREREQ(_maj, _min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((_maj) << 16) + (_min))
#	else
#		define __GNUC_PREREQ(_maj, _min) 0
#	endif
#endif

#ifndef M_PI
#	define M_PI (3.14159265358979323846)
#endif

#define renamenoise_fatal(str) _renamenoise_fatal(str, __FILE__, __LINE__);

#ifdef ENABLE_ASSERTIONS

#	include <stdio.h>
#	include <stdlib.h>

#	ifdef __GNUC__
__attribute__((noreturn))
#	endif

static RENAMENOISE_INLINE void
_renamenoise_fatal(const char *str, const char *file, int line) {
	fprintf(stderr, "Fatal (internal) error in %s, line %d: %s\n", file, line, str);
	abort();
}

#	define renamenoise_assert(cond)                           \
		{                                                      \
			if (!(cond)) {                                     \
				renamenoise_fatal("assertion failed: " #cond); \
			}                                                  \
		}

#	define renamenoise_assert2(cond, message)                              \
		{                                                                   \
			if (!(cond)) {                                                  \
				renamenoise_fatal("assertion failed: " #cond "\n" message); \
			}                                                               \
		}

#else

#	define renamenoise_assert(cond)

#	define renamenoise_assert2(cond, message)

#endif /* !ENABLE_ASSERTIONS defined */

#define RENAMENOISE_MIN16(a, b) ((a) < (b) ? (a) : (b))

#define RENAMENOISE_MAX16(a, b) ((a) > (b) ? (a) : (b))

#define RENAMENOISE_MIN32(a, b) ((a) < (b) ? (a) : (b))

#define RENAMENOISE_MAX32(a, b) ((a) > (b) ? (a) : (b))

typedef float renamenoise_val16;
typedef float renamenoise_val32;

typedef float renamenoise_sig;

#ifdef RENAMENOISE_FLOAT_APPROX
// This code should reliably detect NaN/inf even when -ffast-math is used.
// Assumes IEEE 754 format.
static RENAMENOISE_INLINE int renamenoise_isnan(float x) {
	union {
		float f;
		renamenoise_uint32 i;
	} in;
	in.f = x;
	return ((in.i >> 23) & 0xFF) == 0xFF && (in.i & 0x007FFFFF) != 0;
}
#else
#	ifdef __FAST_MATH__
#		error Cannot build renamenoise with -ffast-math unless RENAMENOISE_FLOAT_APPROX is defined. This could result in crashes on extreme (e.g. NaN) input
#	endif
#	define renamenoise_isnan(x) ((x) != (x))
#endif

#define RENAMENOISE_Q15ONE 1.0f

#define RENAMENOISE_EPSILON 1e-15f
#define RENAMENOISE_VERY_SMALL 1e-30f
#define RENAMENOISE_VERY_LARGE16 1e15f

#define RENAMENOISE_HALF(x) (.5f * (x))

#define RENAMENOISE_ADD(a, b) ((a) + (b))
#define RENAMENOISE_SUB(a, b) ((a) - (b))

#define RENAMENOISE_MAC(c, a, b) ((c) + (renamenoise_val32) (a) * (renamenoise_val32) (b))

#define RENAMENOISE_MULT(a, b) ((a) * (b))

#if __STDC_VERSION__ < 199901L || (__STDC_VERSION__ > 201000L && __STDC_NO_VLA__ == 1)
#	define RENAMENOISE_NO_VLA
#endif

#ifdef RENAMENOISE_NO_VLA
#	include <malloc.h>
#	define renamenoise_stackalloc(type, id, len) type *id = alloca((len) * sizeof(type))
#else
#	define renamenoise_stackalloc(type, id, len) type id[len]
#endif

// Portable macros for denoting unreachable code.
// In such a scenario, perform an early exit ('panic')

#if _MSC_VER // MSVC
#	define renamenoise_unreachable() __assume(0)
#elif __GNUC__ || __clang__ || __MINGW32__
#	define renamenoise_unreachable() __builtin_unreachable()
// #elif __BORLANDC__
//   #define renamenoise_unreachable() __builtin_unreachable() // unknown. needs investigation
#else
#	define renamenoise_unreachable()
#endif

#endif /* ARCH_H */
