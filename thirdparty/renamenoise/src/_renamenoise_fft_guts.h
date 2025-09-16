/*
   Copyright (c) 2024,      The Mumble Developers
   Copyright (c) 2003-2004, Mark Borgerding

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

#ifndef RENAMENOISE_FFT_GUTS_H
#define RENAMENOISE_FFT_GUTS_H

/**
  renamenoise_fft.h
  Defines renamenoise_fft_scalar as either short or a float type
  and defines
  typedef struct {
	renamenoise_fft_scalar r;
	renamenoise_fft_scalar i;
  } renamenoise_fft_cpx;
**/
#include "renamenoise_fft.h"

/*
  Explanation of macros dealing with complex math:

   C_MUL(m,a,b)         : m = a*b
   C_SUB( res, a,b)     : res = a - b
   C_SUBFROM( res , a)  : res -= a
   C_ADDTO( res , a)    : res += a
 */

#define RENAMENOISE_S_MUL(a, b) ((a) * (b))

#define RENAMENOISE_C_MUL(m, a, b)             \
	do {                                       \
		(m).r = (a).r * (b).r - (a).i * (b).i; \
		(m).i = (a).r * (b).i + (a).i * (b).r; \
	} while (0)

#define RENAMENOISE_C_MULC(m, a, b)            \
	do {                                       \
		(m).r = (a).r * (b).r + (a).i * (b).i; \
		(m).i = (a).i * (b).r - (a).r * (b).i; \
	} while (0)

#define RENAMENOISE_C_MUL4(m, a, b) RENAMENOISE_C_MUL(m, a, b)

#define RENAMENOISE_C_MULBYSCALAR(c, s) \
	do {                                \
		(c).r *= (s);                   \
		(c).i *= (s);                   \
	} while (0)

#ifndef RENAMENOISE_C_ADD
#	define RENAMENOISE_C_ADD(res, a, b) \
		do {                             \
			(res).r = (a).r + (b).r;     \
			(res).i = (a).i + (b).i;     \
		} while (0)

#	define RENAMENOISE_C_SUB(res, a, b) \
		do {                             \
			(res).r = (a).r - (b).r;     \
			(res).i = (a).i - (b).i;     \
		} while (0)

#	define RENAMENOISE_C_ADDTO(res, a) \
		do {                            \
			(res).r += (a).r;           \
			(res).i += (a).i;           \
		} while (0)

#	define RENAMENOISE_C_SUBFROM(res, a) \
		do {                              \
			(res).r -= (a).r;             \
			(res).i -= (a).i;             \
		} while (0)
#endif /* !RENAMENOISE_C_ADD defined */

#define RENAMENOISE_FFT_COS(phase) (renamenoise_fft_scalar) cos(phase)
#define RENAMENOISE_FFT_SIN(phase) (renamenoise_fft_scalar) sin(phase)

#define renamenoise_kf_cexp(x, phase)        \
	do {                                     \
		(x)->r = RENAMENOISE_FFT_COS(phase); \
		(x)->i = RENAMENOISE_FFT_SIN(phase); \
	} while (0)

#endif /* RENAMENOISE_FFT_GUTS_H */
