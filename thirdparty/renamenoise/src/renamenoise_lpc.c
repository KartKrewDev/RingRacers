/*
   Copyright (c) 2024,      The Mumble Developers
   Copyright (c) 2009-2010, Xiph.Org Foundation, Written by Jean-Marc Valin

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

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "renamenoise_lpc.h"

#include "arch.h"
#include "common.h"
#include "pitch.h"

void _renamenoise_lpc(renamenoise_val16 *_lpc,     // out: [0...p-1] LPC coefficients
					  const renamenoise_val32 *ac, // in:  [0...p] autocorrelation values
					  int p) {
	int i, j;
	renamenoise_val32 r;
	renamenoise_val32 error = ac[0];
	float *lpc = _lpc;

	RENAMENOISE_CLEAR(lpc, p);
	if (ac[0] != 0) {
		for (i = 0; i < p; i++) {
			// Sum up this iteration's reflection coefficient
			renamenoise_val32 rr = 0;
			for (j = 0; j < i; j++) {
				rr += RENAMENOISE_MULT(lpc[j], ac[i - j]);
			}
			rr += ac[i + 1];
			r = -rr / error;
			//  Update LPC coefficients and total error
			lpc[i] = r;
			for (j = 0; j < ((i + 1) >> 1); j++) {
				renamenoise_val32 tmp1, tmp2;
				tmp1 = lpc[j];
				tmp2 = lpc[i - 1 - j];
				lpc[j] = tmp1 + RENAMENOISE_MULT(r, tmp2);
				lpc[i - 1 - j] = tmp2 + RENAMENOISE_MULT(r, tmp1);
			}

			error = error - RENAMENOISE_MULT(RENAMENOISE_MULT(r, r), error);
			// Bail out once we get 30 dB gain
			if (error < .001f * ac[0]) {
				break;
			}
		}
	}
}

int _renamenoise_autocorr(const renamenoise_val16 *x, //  in: [0...n-1] samples x
						  renamenoise_val32 *ac,      // out: [0...lag-1] ac values
						  const renamenoise_val16 *window, int overlap, int lag, int n) {
	renamenoise_val32 d;
	int i, k;
	int fastN = n - lag;
	int shift;
	const renamenoise_val16 *xptr;
	renamenoise_stackalloc(renamenoise_val16, xx, n);
	renamenoise_assert(n > 0);
	renamenoise_assert(overlap >= 0);
	if (overlap == 0) {
		xptr = x;
	} else {
		for (i = 0; i < n; i++) {
			xx[i] = x[i];
		}
		for (i = 0; i < overlap; i++) {
			xx[i] = RENAMENOISE_MULT(x[i], window[i]);
			xx[n - i - 1] = RENAMENOISE_MULT(x[n - i - 1], window[i]);
		}
		xptr = xx;
	}
	shift = 0;
	renamenoise_pitch_xcorr(xptr, xptr, ac, fastN, lag + 1);
	for (k = 0; k <= lag; k++) {
		for (i = k + fastN, d = 0; i < n; i++) {
			d = RENAMENOISE_MAC(d, xptr[i], xptr[i - k]);
		}
		ac[k] += d;
	}

	return shift;
}
