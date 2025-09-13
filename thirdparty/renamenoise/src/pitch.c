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
   @file pitch.c
   @brief Pitch analysis
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "pitch.h"

#include "common.h"
#include "math.h"
#include "renamenoise_lpc.h"

static void renamenoise_find_best_pitch(renamenoise_val32 *xcorr, renamenoise_val16 *y, int len, int max_pitch, int *best_pitch) {
	int i, j;
	renamenoise_val32 Syy = 1;
	renamenoise_val16 best_num[2];
	renamenoise_val32 best_den[2];

	best_num[0] = -1;
	best_num[1] = -1;
	best_den[0] = 0;
	best_den[1] = 0;
	best_pitch[0] = 0;
	best_pitch[1] = 1;
	for (j = 0; j < len; j++) {
		Syy = RENAMENOISE_ADD(Syy, RENAMENOISE_MULT(y[j], y[j]));
	}
	for (i = 0; i < max_pitch; i++) {
		if (xcorr[i] > 0) {
			renamenoise_val16 num;
			renamenoise_val32 xcorr16;
			xcorr16 = xcorr[i];
			// Considering the range of xcorr16, this should avoid both underflows and overflows (inf) when squaring xcorr16
			xcorr16 *= 1e-12f;
			num = RENAMENOISE_MULT(xcorr16, xcorr16);
			if (RENAMENOISE_MULT(num, best_den[1]) > RENAMENOISE_MULT(best_num[1], Syy)) {
				if (RENAMENOISE_MULT(num, best_den[0]) > RENAMENOISE_MULT(best_num[0], Syy)) {
					best_num[1] = best_num[0];
					best_den[1] = best_den[0];
					best_pitch[1] = best_pitch[0];
					best_num[0] = num;
					best_den[0] = Syy;
					best_pitch[0] = i;
				} else {
					best_num[1] = num;
					best_den[1] = Syy;
					best_pitch[1] = i;
				}
			}
		}
		Syy += RENAMENOISE_MULT(y[i + len], y[i + len]) - RENAMENOISE_MULT(y[i], y[i]);
		Syy = RENAMENOISE_MAX32(1, Syy);
	}
}

static void renamenoise_fir5(const renamenoise_val16 *x, const renamenoise_val16 *num, renamenoise_val16 *y, int N, renamenoise_val16 *mem) {
	int i;
	renamenoise_val16 num0, num1, num2, num3, num4;
	renamenoise_val32 mem0, mem1, mem2, mem3, mem4;
	num0 = num[0];
	num1 = num[1];
	num2 = num[2];
	num3 = num[3];
	num4 = num[4];
	mem0 = mem[0];
	mem1 = mem[1];
	mem2 = mem[2];
	mem3 = mem[3];
	mem4 = mem[4];
	for (i = 0; i < N; i++) {
		renamenoise_val32 sum = x[i];
		sum = RENAMENOISE_MAC(sum, num0, mem0);
		sum = RENAMENOISE_MAC(sum, num1, mem1);
		sum = RENAMENOISE_MAC(sum, num2, mem2);
		sum = RENAMENOISE_MAC(sum, num3, mem3);
		sum = RENAMENOISE_MAC(sum, num4, mem4);
		mem4 = mem3;
		mem3 = mem2;
		mem2 = mem1;
		mem1 = mem0;
		mem0 = x[i];
		y[i] = sum;
	}
	mem[0] = mem0;
	mem[1] = mem1;
	mem[2] = mem2;
	mem[3] = mem3;
	mem[4] = mem4;
}

void renamenoise_pitch_downsample(renamenoise_sig *x[], renamenoise_val16 *x_lp, int len, int C) {
	int i;
	renamenoise_val32 ac[5];
	renamenoise_val16 tmp = RENAMENOISE_Q15ONE;
	renamenoise_val16 lpc[4], mem[5] = {0, 0, 0, 0, 0};
	renamenoise_val16 lpc2[5];
	renamenoise_val16 c1 = .8f;
	for (i = 1; i < (len >> 1); i++) {
		x_lp[i] = RENAMENOISE_HALF(RENAMENOISE_HALF(x[0][(2 * i - 1)] + x[0][(2 * i + 1)]) + x[0][2 * i]);
	}
	x_lp[0] = RENAMENOISE_HALF(RENAMENOISE_HALF(x[0][1]) + x[0][0]);
	if (C == 2) {
		for (i = 1; i < (len >> 1); i++) {
			x_lp[i] += RENAMENOISE_HALF(RENAMENOISE_HALF(x[1][(2 * i - 1)] + x[1][(2 * i + 1)]) + x[1][2 * i]);
		}
		x_lp[0] += RENAMENOISE_HALF(RENAMENOISE_HALF(x[1][1]) + x[1][0]);
	}

	_renamenoise_autocorr(x_lp, ac, NULL, 0, 4, len >> 1);

	// Noise floor -40 dB
	ac[0] *= 1.0001f;
	// Lag windowing
	for (i = 1; i <= 4; i++) {
		// ac[i] *= exp(-.5*(2*M_PI*.002*i)*(2*M_PI*.002*i));
		ac[i] -= ac[i] * (.008f * i) * (.008f * i);
	}

	_renamenoise_lpc(lpc, ac, 4);
	for (i = 0; i < 4; i++) {
		tmp = RENAMENOISE_MULT(.9f, tmp);
		lpc[i] = RENAMENOISE_MULT(lpc[i], tmp);
	}
	// Add a zero
	lpc2[0] = lpc[0] + .8f;
	lpc2[1] = lpc[1] + RENAMENOISE_MULT(c1, lpc[0]);
	lpc2[2] = lpc[2] + RENAMENOISE_MULT(c1, lpc[1]);
	lpc2[3] = lpc[3] + RENAMENOISE_MULT(c1, lpc[2]);
	lpc2[4] = RENAMENOISE_MULT(c1, lpc[3]);
	renamenoise_fir5(x_lp, lpc2, x_lp, len >> 1, mem);
}

void renamenoise_pitch_xcorr(const renamenoise_val16 *_x, const renamenoise_val16 *_y, renamenoise_val32 *xcorr, int len, int max_pitch) {
	// Unrolled version of the pitch correlation -- runs faster on x86 and ARM

	int i;
	// The EDSP version requires that max_pitch is at least 1,
	// and that _x is 32-bit aligned.
	// Since it's hard to put asserts in assembly, put them here.
	renamenoise_assert(max_pitch > 0);
	renamenoise_assert((((unsigned char *) _x - (unsigned char *) NULL) & 3) == 0);
	for (i = 0; i < max_pitch - 3; i += 4) {
		renamenoise_val32 sum[4] = {0, 0, 0, 0};
		renamenoise_xcorr_kernel(_x, _y + i, sum, len);
		xcorr[i] = sum[0];
		xcorr[i + 1] = sum[1];
		xcorr[i + 2] = sum[2];
		xcorr[i + 3] = sum[3];
	}
	// In case max_pitch isn't a multiple of 4, do non-unrolled version.
	for (; i < max_pitch; i++) {
		renamenoise_val32 sum;
		sum = renamenoise_inner_prod(_x, _y + i, len);
		xcorr[i] = sum;
	}
}

void renamenoise_pitch_search(const renamenoise_val16 *x_lp, renamenoise_val16 *y, int len, int max_pitch, int *pitch) {
	int i, j;
	int lag;
	int best_pitch[2] = {0, 0};
	int offset;

	renamenoise_assert(len > 0);
	renamenoise_assert(max_pitch > 0);
	lag = len + max_pitch;

	renamenoise_stackalloc(renamenoise_val16, x_lp4, len >> 2);
	renamenoise_stackalloc(renamenoise_val16, y_lp4, lag >> 2);
	renamenoise_stackalloc(renamenoise_val32, xcorr, max_pitch >> 1);

	// Downsample by 2 again
	for (j = 0; j < (len >> 2); j++) {
		x_lp4[j] = x_lp[2 * j];
	}
	for (j = 0; j < (lag >> 2); j++) {
		y_lp4[j] = y[2 * j];
	}

	// Coarse search with 4x decimation

	renamenoise_pitch_xcorr(x_lp4, y_lp4, xcorr, len >> 2, max_pitch >> 2);

	renamenoise_find_best_pitch(xcorr, y_lp4, len >> 2, max_pitch >> 2, best_pitch);

	// Finer search with 2x decimation
	for (i = 0; i < (max_pitch >> 1); i++) {
		renamenoise_val32 sum;
		xcorr[i] = 0;
		if (abs(i - 2 * best_pitch[0]) > 2 && abs(i - 2 * best_pitch[1]) > 2) {
			continue;
		}
		sum = renamenoise_inner_prod(x_lp, y + i, len >> 1);
		xcorr[i] = RENAMENOISE_MAX32(-1, sum);
	}
	renamenoise_find_best_pitch(xcorr, y, len >> 1, max_pitch >> 1, best_pitch);

	// Refine by pseudo-interpolation
	if (best_pitch[0] > 0 && best_pitch[0] < (max_pitch >> 1) - 1) {
		renamenoise_val32 a, b, c;
		a = xcorr[best_pitch[0] - 1];
		b = xcorr[best_pitch[0]];
		c = xcorr[best_pitch[0] + 1];
		if ((c - a) > RENAMENOISE_MULT(.7f, b - a)) {
			offset = 1;
		} else if ((a - c) > RENAMENOISE_MULT(.7f, b - c)) {
			offset = -1;
		} else {
			offset = 0;
		}
	} else {
		offset = 0;
	}
	*pitch = 2 * best_pitch[0] - offset;
}

static renamenoise_val16 renamenoise_compute_pitch_gain(renamenoise_val32 xy, renamenoise_val32 xx, renamenoise_val32 yy) {
	return xy / sqrt(1 + xx * yy);
}

static const int renamenoise_second_check[16] = {0, 0, 3, 2, 3, 2, 5, 2, 3, 2, 3, 2, 5, 2, 3, 2};
renamenoise_val16 renamenoise_remove_doubling(renamenoise_val16 *x, int maxperiod, int minperiod, int N, int *T0_, int prev_period,
											  renamenoise_val16 prev_gain) {
	int k, i, T, T0;
	renamenoise_val16 g, g0;
	renamenoise_val16 pg;
	renamenoise_val32 xy, xx, yy, xy2;
	renamenoise_val32 xcorr[3];
	renamenoise_val32 best_xy, best_yy;
	int offset;
	int minperiod0;

	minperiod0 = minperiod;
	maxperiod /= 2;
	minperiod /= 2;
	*T0_ /= 2;
	prev_period /= 2;
	N /= 2;
	x += maxperiod;
	if (*T0_ >= maxperiod) {
		*T0_ = maxperiod - 1;
	}

	T = T0 = *T0_;
	renamenoise_stackalloc(renamenoise_val32, yy_lookup, maxperiod + 1);
	renamenoise_dual_inner_prod(x, x, x - T0, N, &xx, &xy);
	yy_lookup[0] = xx;
	yy = xx;
	for (i = 1; i <= maxperiod; i++) {
		yy = yy + RENAMENOISE_MULT(x[-i], x[-i]) - RENAMENOISE_MULT(x[N - i], x[N - i]);
		yy_lookup[i] = RENAMENOISE_MAX32(0, yy);
	}
	yy = yy_lookup[T0];
	best_xy = xy;
	best_yy = yy;
	g = g0 = renamenoise_compute_pitch_gain(xy, xx, yy);
	// Look for any pitch at T/k
	for (k = 2; k <= 15; k++) {
		int T1, T1b;
		renamenoise_val16 g1;
		renamenoise_val16 cont = 0;
		renamenoise_val16 thresh;
		T1 = (2 * T0 + k) / (2 * k);
		if (T1 < minperiod) {
			break;
		}
		// Look for another strong correlation at T1b
		if (k == 2) {
			if (T1 + T0 > maxperiod) {
				T1b = T0;
			} else {
				T1b = T0 + T1;
			}
		} else {
			T1b = (2 * renamenoise_second_check[k] * T0 + k) / (2 * k);
		}
		renamenoise_dual_inner_prod(x, &x[-T1], &x[-T1b], N, &xy, &xy2);
		xy = RENAMENOISE_HALF(xy + xy2);
		yy = RENAMENOISE_HALF(yy_lookup[T1] + yy_lookup[T1b]);
		g1 = renamenoise_compute_pitch_gain(xy, xx, yy);
		if (abs(T1 - prev_period) <= 1) {
			cont = prev_gain;
		} else if (abs(T1 - prev_period) <= 2 && 5 * k * k < T0) {
			cont = RENAMENOISE_HALF(prev_gain);
		} else {
			cont = 0;
		}
		thresh = RENAMENOISE_MAX16(.3f, RENAMENOISE_MULT(.7f, g0) - cont);
		// Bias against very high pitch (very short period) to avoid false-positives due to short-term correlation
		if (T1 < 3 * minperiod) {
			thresh = RENAMENOISE_MAX16(.4f, RENAMENOISE_MULT(.85f, g0) - cont);
		} else if (T1 < 2 * minperiod) {
			thresh = RENAMENOISE_MAX16(.5f, RENAMENOISE_MULT(.9f, g0) - cont);
		}
		if (g1 > thresh) {
			best_xy = xy;
			best_yy = yy;
			T = T1;
			g = g1;
		}
	}
	best_xy = RENAMENOISE_MAX32(0, best_xy);
	if (best_yy <= best_xy) {
		pg = RENAMENOISE_Q15ONE;
	} else {
		pg = best_xy / (best_yy + 1);
	}

	for (k = 0; k < 3; k++) {
		xcorr[k] = renamenoise_inner_prod(x, x - (T + k - 1), N);
	}
	if ((xcorr[2] - xcorr[0]) > RENAMENOISE_MULT(.7f, xcorr[1] - xcorr[0])) {
		offset = 1;
	} else if ((xcorr[0] - xcorr[2]) > RENAMENOISE_MULT(.7f, xcorr[1] - xcorr[2])) {
		offset = -1;
	} else {
		offset = 0;
	}
	if (pg > g) {
		pg = g;
	}
	*T0_ = 2 * T + offset;

	if (*T0_ < minperiod0) {
		*T0_ = minperiod0;
	}
	return pg;
}
