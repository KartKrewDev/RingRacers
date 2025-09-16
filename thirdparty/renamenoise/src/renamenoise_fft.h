/*
   Copyright (c) 2024,      The Mumble Developers
   Copyright (c) 2008,      Xiph.Org Foundation, CSIRO
   Copyright (c) 2005-2007, Xiph.Org Foundation
   Lots of modifications by Jean-Marc Valin
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

#ifndef RENAMENOISE_FFT_H
#define RENAMENOISE_FFT_H

#include "arch.h"

#include <math.h>
#include <stdlib.h>
#define renamenoise_alloc2(x) malloc(x)
#define renamenoise_free2(x) free(x)

#ifdef __cplusplus
extern "C" {
#endif

#define RENAMENOISE_FFT_MALLOC renamenoise_alloc2

#ifndef renamenoise_fft_scalar
//  default is float
#	define renamenoise_fft_scalar float
#	define renamenoise_twiddle_scalar float
#endif

typedef struct {
	renamenoise_fft_scalar r;
	renamenoise_fft_scalar i;
} renamenoise_fft_cpx;

typedef struct {
	renamenoise_twiddle_scalar r;
	renamenoise_twiddle_scalar i;
} renamenoise_twiddle_cpx;

#define RENAMENOISE_MAXFACTORS 8
// e.g. an fft of length 128 has 4 factors
// as far as renamenoisefft is concerned
// 4*4*4*2

typedef struct renamenoise_arch_fft_state {
	int is_supported;
	void *priv;
} renamenoise_arch_fft_state;

typedef struct renamenoise_fft_state {
	int nfft;
	renamenoise_val16 scale;
	int shift;
	renamenoise_int16 factors[2 * RENAMENOISE_MAXFACTORS];
	const renamenoise_int16 *bitrev;
	const renamenoise_twiddle_cpx *twiddles;
	renamenoise_arch_fft_state *arch_fft;
} renamenoise_fft_state;

// typedef struct renamenoise_fft_state* renamenoise_fft_cfg;

/**
 *  renamenoise_fft_alloc
 *
 *  Initialize a FFT (or IFFT) algorithm's cfg/state buffer.
 *
 *  typical usage:      renamenoise_fft_cfg
 * mycfg=renamenoise_fft_alloc(1024,0,NULL,NULL);
 *
 *  The return value from fft_alloc is a cfg buffer used internally
 *  by the fft routine or NULL.
 *
 *  If lenmem is NULL, then renamenoise_fft_alloc will allocate a cfg buffer
 * using malloc. The returned value should be free()d when done to avoid memory
 * leaks.
 *
 *  The state can be placed in a user supplied buffer 'mem':
 *  If lenmem is not NULL and mem is not NULL and *lenmem is large enough,
 *      then the function places the cfg in mem and the size used in *lenmem
 *      and returns mem.
 *
 *  If lenmem is not NULL and ( mem is NULL or *lenmem is not large enough),
 *      then the function returns NULL and places the minimum cfg
 *      buffer size in *lenmem.
 */

renamenoise_fft_state *renamenoise_fft_alloc_twiddles(int nfft, void *mem, size_t *lenmem, const renamenoise_fft_state *base, int arch);

renamenoise_fft_state *renamenoise_fft_alloc(int nfft, void *mem, size_t *lenmem, int arch);

/**
 * renamenoise_fft(cfg,in_out_buf)
 *
 * Perform an FFT on a complex input buffer.
 * for a forward FFT,
 * fin should be  f[0] , f[1] , ... ,f[nfft-1]
 * fout will be   F[0] , F[1] , ... ,F[nfft-1]
 * Note that each element is complex and can be accessed like
 * f[k].r and f[k].i
 */
void renamenoise_fft_c(const renamenoise_fft_state *cfg, const renamenoise_fft_cpx *fin, renamenoise_fft_cpx *fout);

void renamenoise_fft_impl(const renamenoise_fft_state *st, renamenoise_fft_cpx *fout);

void renamenoise_fft_free(const renamenoise_fft_state *cfg, int arch);

void renamenoise_fft_free_arch_c(renamenoise_fft_state *st);
int renamenoise_fft_alloc_arch_c(renamenoise_fft_state *st);

#if !defined(OVERRIDE_RENAMENOISE_FFT)

#	define renamenoise_fft_alloc_arch(_st, arch) ((void) (arch), renamenoise_fft_alloc_arch_c(_st))

#	define renamenoise_fft_free_arch(_st, arch) ((void) (arch), renamenoise_fft_free_arch_c(_st))

#	define renamenoise_fft(_cfg, _fin, _fout, arch) ((void) (arch), renamenoise_fft_c(_cfg, _fin, _fout))

#endif /* end if !defined(OVERRIDE_RENAMENOISE_FFT) */

#ifdef __cplusplus
}
#endif

#endif /* RENAMENOISE_FFT_H */
