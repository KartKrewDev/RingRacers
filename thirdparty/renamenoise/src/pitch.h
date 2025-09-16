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
   @file pitch.h
   @brief Pitch analysis
 */

#ifndef PITCH_H
#define PITCH_H

#include "arch.h"

void renamenoise_pitch_downsample(renamenoise_sig *x[], renamenoise_val16 *x_lp, int len, int C);

void renamenoise_pitch_search(const renamenoise_val16 *x_lp, renamenoise_val16 *y, int len, int max_pitch, int *pitch);

renamenoise_val16 renamenoise_remove_doubling(renamenoise_val16 *x, int maxperiod, int minperiod, int N, int *T0, int prev_period,
											  renamenoise_val16 prev_gain);

// OPT: This is the kernel you really want to optimize. It gets used a lot by the prefilter and by the PLC.
static RENAMENOISE_INLINE void renamenoise_xcorr_kernel(const renamenoise_val16 *x, const renamenoise_val16 *y, renamenoise_val32 sum[4], int len) {
	int j;
	renamenoise_val16 y_0, y_1, y_2, y_3;
	renamenoise_assert(len >= 3);
	y_3 = 0; /* gcc doesn't realize that y_3 can't be used uninitialized */
	y_0 = *y++;
	y_1 = *y++;
	y_2 = *y++;
	for (j = 0; j < len - 3; j += 4) {
		renamenoise_val16 tmp;
		tmp = *x++;
		y_3 = *y++;
		sum[0] = RENAMENOISE_MAC(sum[0], tmp, y_0);
		sum[1] = RENAMENOISE_MAC(sum[1], tmp, y_1);
		sum[2] = RENAMENOISE_MAC(sum[2], tmp, y_2);
		sum[3] = RENAMENOISE_MAC(sum[3], tmp, y_3);
		tmp = *x++;
		y_0 = *y++;
		sum[0] = RENAMENOISE_MAC(sum[0], tmp, y_1);
		sum[1] = RENAMENOISE_MAC(sum[1], tmp, y_2);
		sum[2] = RENAMENOISE_MAC(sum[2], tmp, y_3);
		sum[3] = RENAMENOISE_MAC(sum[3], tmp, y_0);
		tmp = *x++;
		y_1 = *y++;
		sum[0] = RENAMENOISE_MAC(sum[0], tmp, y_2);
		sum[1] = RENAMENOISE_MAC(sum[1], tmp, y_3);
		sum[2] = RENAMENOISE_MAC(sum[2], tmp, y_0);
		sum[3] = RENAMENOISE_MAC(sum[3], tmp, y_1);
		tmp = *x++;
		y_2 = *y++;
		sum[0] = RENAMENOISE_MAC(sum[0], tmp, y_3);
		sum[1] = RENAMENOISE_MAC(sum[1], tmp, y_0);
		sum[2] = RENAMENOISE_MAC(sum[2], tmp, y_1);
		sum[3] = RENAMENOISE_MAC(sum[3], tmp, y_2);
	}
	if (j++ < len) {
		renamenoise_val16 tmp = *x++;
		y_3 = *y++;
		sum[0] = RENAMENOISE_MAC(sum[0], tmp, y_0);
		sum[1] = RENAMENOISE_MAC(sum[1], tmp, y_1);
		sum[2] = RENAMENOISE_MAC(sum[2], tmp, y_2);
		sum[3] = RENAMENOISE_MAC(sum[3], tmp, y_3);
	}
	if (j++ < len) {
		renamenoise_val16 tmp = *x++;
		y_0 = *y++;
		sum[0] = RENAMENOISE_MAC(sum[0], tmp, y_1);
		sum[1] = RENAMENOISE_MAC(sum[1], tmp, y_2);
		sum[2] = RENAMENOISE_MAC(sum[2], tmp, y_3);
		sum[3] = RENAMENOISE_MAC(sum[3], tmp, y_0);
	}
	if (j < len) {
		renamenoise_val16 tmp = *x++;
		y_1 = *y++;
		sum[0] = RENAMENOISE_MAC(sum[0], tmp, y_2);
		sum[1] = RENAMENOISE_MAC(sum[1], tmp, y_3);
		sum[2] = RENAMENOISE_MAC(sum[2], tmp, y_0);
		sum[3] = RENAMENOISE_MAC(sum[3], tmp, y_1);
	}
}

static RENAMENOISE_INLINE void renamenoise_dual_inner_prod(const renamenoise_val16 *x, const renamenoise_val16 *y01, const renamenoise_val16 *y02,
														   int N, renamenoise_val32 *xy1, renamenoise_val32 *xy2) {
	int i;
	renamenoise_val32 xy01 = 0;
	renamenoise_val32 xy02 = 0;
	for (i = 0; i < N; i++) {
		xy01 = RENAMENOISE_MAC(xy01, x[i], y01[i]);
		xy02 = RENAMENOISE_MAC(xy02, x[i], y02[i]);
	}
	*xy1 = xy01;
	*xy2 = xy02;
}

// We make sure a C version is always available for cases where the overhead of vectorization and passing around an arch flag aren't worth it.
static RENAMENOISE_INLINE renamenoise_val32 renamenoise_inner_prod(const renamenoise_val16 *x, const renamenoise_val16 *y, int N) {
	int i;
	renamenoise_val32 xy = 0;
	for (i = 0; i < N; i++) {
		xy = RENAMENOISE_MAC(xy, x[i], y[i]);
	}
	return xy;
}

void renamenoise_pitch_xcorr(const renamenoise_val16 *_x, const renamenoise_val16 *_y, renamenoise_val32 *xcorr, int len, int max_pitch);

#endif /* PITCH_H */
