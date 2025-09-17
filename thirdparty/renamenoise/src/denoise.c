/*
   Copyright (c) 2024, The Mumble Developers
   Copyright (c) 2018, Gregor Richards
   Copyright (c) 2017, Mozilla

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

#include "arch.h"
#include "common.h"
#include "pitch.h"
#include "renamenoise.h"
#include "renamenoise_fft.h"
#include "rnn.h"
#include "rnn_data.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RENAMENOISE_FRAME_SIZE_SHIFT 2
#define RENAMENOISE_FRAME_SIZE (120 << RENAMENOISE_FRAME_SIZE_SHIFT)
#define RENAMENOISE_WINDOW_SIZE (2 * RENAMENOISE_FRAME_SIZE)
#define RENAMENOISE_FREQ_SIZE (RENAMENOISE_FRAME_SIZE + 1)

#define RENAMENOISE_PITCH_MIN_PERIOD 60
#define RENAMENOISE_PITCH_MAX_PERIOD 768
#define RENAMENOISE_PITCH_FRAME_SIZE 960
#define RENAMENOISE_PITCH_BUF_SIZE (RENAMENOISE_PITCH_MAX_PERIOD + RENAMENOISE_PITCH_FRAME_SIZE)

#define RENAMENOISE_SQUARE(x) ((x) * (x))

#define RENAMENOISE_NB_BANDS 22

#define RENAMENOISE_CEPS_MEM 8
#define RENAMENOISE_NB_DELTA_CEPS 6

#define RENAMENOISE_NB_FEATURES (RENAMENOISE_NB_BANDS + 3 * RENAMENOISE_NB_DELTA_CEPS + 2)

#ifndef RENAMENOISE_TRAINING
#	define RENAMENOISE_TRAINING 0
#endif

/* The built-in model, used if no file is given as input */
extern const struct ReNameNoiseModel renamenoise_model_orig;

static const renamenoise_int16 renamenoise_eband5ms[] = {
	// 0  200 400 600 800  1k 1.2 1.4 1.6  2k 2.4
	0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12,
	// 2.8 3.2  4k 4.8 5.6 6.8  8k 9.6 12k 15.6 20k
	14, 16, 20, 24, 28, 34, 40, 48, 60, 78, 100};

typedef struct {
	int init;
	renamenoise_fft_state *kfft;
	float half_window[RENAMENOISE_FRAME_SIZE];
	float dct_table[RENAMENOISE_NB_BANDS * RENAMENOISE_NB_BANDS];
} ReNameNoiseCommonState;

struct ReNameNoiseDenoiseState {
	float analysis_mem[RENAMENOISE_FRAME_SIZE];
	float cepstral_mem[RENAMENOISE_CEPS_MEM][RENAMENOISE_NB_BANDS];
	int memid;
	float synthesis_mem[RENAMENOISE_FRAME_SIZE];
	float pitch_buf[RENAMENOISE_PITCH_BUF_SIZE];
	float pitch_enh_buf[RENAMENOISE_PITCH_BUF_SIZE];
	float last_gain;
	int last_period;
	float mem_hp_x[2];
	float lastg[RENAMENOISE_NB_BANDS];
	ReNameNoiseRNNState rnn;
};

void renamenoise_compute_band_energy(float *bandE, const renamenoise_fft_cpx *X) {
	int i;
	float sum[RENAMENOISE_NB_BANDS] = {0};
	for (i = 0; i < RENAMENOISE_NB_BANDS - 1; i++) {
		int j;
		int band_size;
		band_size = (renamenoise_eband5ms[i + 1] - renamenoise_eband5ms[i]) << RENAMENOISE_FRAME_SIZE_SHIFT;
		for (j = 0; j < band_size; j++) {
			float tmp;
			float frac = (float) j / band_size;
			tmp = RENAMENOISE_SQUARE(X[(renamenoise_eband5ms[i] << RENAMENOISE_FRAME_SIZE_SHIFT) + j].r);
			tmp += RENAMENOISE_SQUARE(X[(renamenoise_eband5ms[i] << RENAMENOISE_FRAME_SIZE_SHIFT) + j].i);
			sum[i] = sum[i] + (1 - frac) * tmp;
			sum[i + 1] = sum[i + 1] + frac * tmp;
		}
	}
	sum[0] *= 2;
	sum[RENAMENOISE_NB_BANDS - 1] *= 2;
	for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
		bandE[i] = sum[i];
	}
}

void renamenoise_compute_band_corr(float *bandE, const renamenoise_fft_cpx *X, const renamenoise_fft_cpx *P) {
	int i;
	float sum[RENAMENOISE_NB_BANDS] = {0};
	for (i = 0; i < RENAMENOISE_NB_BANDS - 1; i++) {
		int j;
		int band_size;
		band_size = (renamenoise_eband5ms[i + 1] - renamenoise_eband5ms[i]) << RENAMENOISE_FRAME_SIZE_SHIFT;
		for (j = 0; j < band_size; j++) {
			float tmp;
			float frac = (float) j / band_size;
			tmp = X[(renamenoise_eband5ms[i] << RENAMENOISE_FRAME_SIZE_SHIFT) + j].r
				* P[(renamenoise_eband5ms[i] << RENAMENOISE_FRAME_SIZE_SHIFT) + j].r;
			tmp += X[(renamenoise_eband5ms[i] << RENAMENOISE_FRAME_SIZE_SHIFT) + j].i
				 * P[(renamenoise_eband5ms[i] << RENAMENOISE_FRAME_SIZE_SHIFT) + j].i;
			sum[i] += (1 - frac) * tmp;
			sum[i + 1] += frac * tmp;
		}
	}
	sum[0] *= 2;
	sum[RENAMENOISE_NB_BANDS - 1] *= 2;
	for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
		bandE[i] = sum[i];
	}
}

void renamenoise_interp_band_gain(float *g, const float *bandE) {
	int i;
	memset(g, 0, RENAMENOISE_FREQ_SIZE);
	for (i = 0; i < RENAMENOISE_NB_BANDS - 1; i++) {
		int j;
		int band_size;
		band_size = (renamenoise_eband5ms[i + 1] - renamenoise_eband5ms[i]) << RENAMENOISE_FRAME_SIZE_SHIFT;
		for (j = 0; j < band_size; j++) {
			float frac = (float) j / band_size;
			g[(renamenoise_eband5ms[i] << RENAMENOISE_FRAME_SIZE_SHIFT) + j] = (1 - frac) * bandE[i] + frac * bandE[i + 1];
		}
	}
}

ReNameNoiseCommonState renamenoise_common;

static void renamenoise_check_init(void) {
	int i;
	if (renamenoise_common.init) {
		return;
	}
	renamenoise_common.kfft = renamenoise_fft_alloc_twiddles(2 * RENAMENOISE_FRAME_SIZE, NULL, NULL, NULL, 0);
	for (i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
		renamenoise_common.half_window[i] =
			sin(.5 * M_PI * sin(.5 * M_PI * (i + .5) / RENAMENOISE_FRAME_SIZE) * sin(.5 * M_PI * (i + .5) / RENAMENOISE_FRAME_SIZE));
	}
	for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
		int j;
		for (j = 0; j < RENAMENOISE_NB_BANDS; j++) {
			renamenoise_common.dct_table[i * RENAMENOISE_NB_BANDS + j] = cos((i + .5) * j * M_PI / RENAMENOISE_NB_BANDS);
			if (j == 0) {
				renamenoise_common.dct_table[i * RENAMENOISE_NB_BANDS + j] *= sqrt(.5);
			}
		}
	}
	renamenoise_common.init = 1;
}

static void renamenoise_dct(float *out, const float *in) {
	int i;
	renamenoise_check_init();
	for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
		int j;
		float sum = 0;
		for (j = 0; j < RENAMENOISE_NB_BANDS; j++) {
			sum += in[j] * renamenoise_common.dct_table[j * RENAMENOISE_NB_BANDS + i];
		}
		out[i] = sum * sqrt(2. / 22);
	}
}

static void renamenoise_forward_transform(renamenoise_fft_cpx *out, const float *in) {
	int i;
	renamenoise_fft_cpx x[RENAMENOISE_WINDOW_SIZE];
	renamenoise_fft_cpx y[RENAMENOISE_WINDOW_SIZE];
	renamenoise_check_init();
	for (i = 0; i < RENAMENOISE_WINDOW_SIZE; i++) {
		x[i].r = in[i];
		x[i].i = 0;
	}
	renamenoise_fft(renamenoise_common.kfft, x, y, 0);
	for (i = 0; i < RENAMENOISE_FREQ_SIZE; i++) {
		out[i] = y[i];
	}
}

static void renamenoise_inverse_transform(float *out, const renamenoise_fft_cpx *in) {
	int i;
	renamenoise_fft_cpx x[RENAMENOISE_WINDOW_SIZE];
	renamenoise_fft_cpx y[RENAMENOISE_WINDOW_SIZE];
	renamenoise_check_init();
	for (i = 0; i < RENAMENOISE_FREQ_SIZE; i++) {
		x[i] = in[i];
	}
	for (; i < RENAMENOISE_WINDOW_SIZE; i++) {
		x[i].r = x[RENAMENOISE_WINDOW_SIZE - i].r;
		x[i].i = -x[RENAMENOISE_WINDOW_SIZE - i].i;
	}
	renamenoise_fft(renamenoise_common.kfft, x, y, 0);
	// output in reverse order for IFFT.
	out[0] = RENAMENOISE_WINDOW_SIZE * y[0].r;
	for (i = 1; i < RENAMENOISE_WINDOW_SIZE; i++) {
		out[i] = RENAMENOISE_WINDOW_SIZE * y[RENAMENOISE_WINDOW_SIZE - i].r;
	}
}

static void renamenoise_apply_window(float *x) {
	int i;
	renamenoise_check_init();
	for (i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
		x[i] *= renamenoise_common.half_window[i];
		x[RENAMENOISE_WINDOW_SIZE - 1 - i] *= renamenoise_common.half_window[i];
	}
}

int renamenoise_get_size(void) {
	return sizeof(ReNameNoiseDenoiseState);
}

int renamenoise_get_frame_size(void) {
	return RENAMENOISE_FRAME_SIZE;
}

int renamenoise_init(ReNameNoiseDenoiseState *st, ReNameNoiseModel *model) {
	memset(st, 0, sizeof(*st));
	if (model) {
		st->rnn.model = model;
	} else {
		st->rnn.model = &renamenoise_model_orig;
	}
	st->rnn.vad_gru_state = calloc(sizeof(float), st->rnn.model->vad_gru_size);
	st->rnn.noise_gru_state = calloc(sizeof(float), st->rnn.model->noise_gru_size);
	st->rnn.denoise_gru_state = calloc(sizeof(float), st->rnn.model->denoise_gru_size);
	return 0;
}

ReNameNoiseDenoiseState *renamenoise_create(ReNameNoiseModel *model) {
	ReNameNoiseDenoiseState *st;
	st = malloc(renamenoise_get_size());
	renamenoise_init(st, model);
	return st;
}

void renamenoise_destroy(ReNameNoiseDenoiseState *st) {
	free(st->rnn.vad_gru_state);
	free(st->rnn.noise_gru_state);
	free(st->rnn.denoise_gru_state);
	free(st);
}

#if RENAMENOISE_TRAINING
int lowpass = RENAMENOISE_FREQ_SIZE;
int band_lp = RENAMENOISE_NB_BANDS;
#endif

static void renamenoise_frame_analysis(ReNameNoiseDenoiseState *st, renamenoise_fft_cpx *X, float *Ex, const float *in) {
	int i;
	float x[RENAMENOISE_WINDOW_SIZE];
	RENAMENOISE_COPY(x, st->analysis_mem, RENAMENOISE_FRAME_SIZE);
	for (i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
		x[RENAMENOISE_FRAME_SIZE + i] = in[i];
	}
	RENAMENOISE_COPY(st->analysis_mem, in, RENAMENOISE_FRAME_SIZE);
	renamenoise_apply_window(x);
	renamenoise_forward_transform(X, x);
#if RENAMENOISE_TRAINING
	for (i = lowpass; i < RENAMENOISE_FREQ_SIZE; i++) {
		X[i].r = X[i].i = 0;
	}
#endif
	renamenoise_compute_band_energy(Ex, X);
}

static int renamenoise_compute_frame_features(ReNameNoiseDenoiseState *st, renamenoise_fft_cpx *X, renamenoise_fft_cpx *P, float *Ex, float *Ep,
											  float *Exp, float *features, const float *in) {
	int i;
	float E = 0;
	float *ceps_0, *ceps_1, *ceps_2;
	float spec_variability = 0;
	float Ly[RENAMENOISE_NB_BANDS];
	float p[RENAMENOISE_WINDOW_SIZE];
	float pitch_buf[RENAMENOISE_PITCH_BUF_SIZE >> 1];
	int pitch_index;
	float gain;
	float *(pre[1]);
	float tmp[RENAMENOISE_NB_BANDS];
	float follow, logMax;
	renamenoise_frame_analysis(st, X, Ex, in);
	RENAMENOISE_MOVE(st->pitch_buf, &st->pitch_buf[RENAMENOISE_FRAME_SIZE], RENAMENOISE_PITCH_BUF_SIZE - RENAMENOISE_FRAME_SIZE);
	RENAMENOISE_COPY(&st->pitch_buf[RENAMENOISE_PITCH_BUF_SIZE - RENAMENOISE_FRAME_SIZE], in, RENAMENOISE_FRAME_SIZE);
	pre[0] = &st->pitch_buf[0];
	renamenoise_pitch_downsample(pre, pitch_buf, RENAMENOISE_PITCH_BUF_SIZE, 1);
	renamenoise_pitch_search(pitch_buf + (RENAMENOISE_PITCH_MAX_PERIOD >> 1), pitch_buf, RENAMENOISE_PITCH_FRAME_SIZE,
							 RENAMENOISE_PITCH_MAX_PERIOD - 3 * RENAMENOISE_PITCH_MIN_PERIOD, &pitch_index);
	pitch_index = RENAMENOISE_PITCH_MAX_PERIOD - pitch_index;

	gain = renamenoise_remove_doubling(pitch_buf, RENAMENOISE_PITCH_MAX_PERIOD, RENAMENOISE_PITCH_MIN_PERIOD, RENAMENOISE_PITCH_FRAME_SIZE,
									   &pitch_index, st->last_period, st->last_gain);
	st->last_period = pitch_index;
	st->last_gain = gain;
	for (i = 0; i < RENAMENOISE_WINDOW_SIZE; i++) {
		p[i] = st->pitch_buf[RENAMENOISE_PITCH_BUF_SIZE - RENAMENOISE_WINDOW_SIZE - pitch_index + i];
	}
	renamenoise_apply_window(p);
	renamenoise_forward_transform(P, p);
	renamenoise_compute_band_energy(Ep, P);
	renamenoise_compute_band_corr(Exp, X, P);
	for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
		Exp[i] = Exp[i] / sqrt(.001 + Ex[i] * Ep[i]);
	}
	renamenoise_dct(tmp, Exp);
	for (i = 0; i < RENAMENOISE_NB_DELTA_CEPS; i++) {
		features[RENAMENOISE_NB_BANDS + 2 * RENAMENOISE_NB_DELTA_CEPS + i] = tmp[i];
	}
	features[RENAMENOISE_NB_BANDS + 2 * RENAMENOISE_NB_DELTA_CEPS] -= 1.3;
	features[RENAMENOISE_NB_BANDS + 2 * RENAMENOISE_NB_DELTA_CEPS + 1] -= 0.9;
	features[RENAMENOISE_NB_BANDS + 3 * RENAMENOISE_NB_DELTA_CEPS] = .01 * (pitch_index - 300);
	logMax = -2;
	follow = -2;
	for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
		Ly[i] = log10(1e-2 + Ex[i]);
		Ly[i] = RENAMENOISE_MAX16(logMax - 7, RENAMENOISE_MAX16(follow - 1.5, Ly[i]));
		logMax = RENAMENOISE_MAX16(logMax, Ly[i]);
		follow = RENAMENOISE_MAX16(follow - 1.5, Ly[i]);
		E += Ex[i];
	}
	if (!RENAMENOISE_TRAINING && E < 0.04) {
		// If there's no audio, avoid messing up the state.
		RENAMENOISE_CLEAR(features, RENAMENOISE_NB_FEATURES);
		return 1;
	}
	renamenoise_dct(features, Ly);
	features[0] -= 12;
	features[1] -= 4;
	ceps_0 = st->cepstral_mem[st->memid];
	ceps_1 = (st->memid < 1) ? st->cepstral_mem[RENAMENOISE_CEPS_MEM + st->memid - 1] : st->cepstral_mem[st->memid - 1];
	ceps_2 = (st->memid < 2) ? st->cepstral_mem[RENAMENOISE_CEPS_MEM + st->memid - 2] : st->cepstral_mem[st->memid - 2];
	for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
		ceps_0[i] = features[i];
	}
	st->memid++;
	for (i = 0; i < RENAMENOISE_NB_DELTA_CEPS; i++) {
		features[i] = ceps_0[i] + ceps_1[i] + ceps_2[i];
		features[RENAMENOISE_NB_BANDS + i] = ceps_0[i] - ceps_2[i];
		features[RENAMENOISE_NB_BANDS + RENAMENOISE_NB_DELTA_CEPS + i] = ceps_0[i] - 2 * ceps_1[i] + ceps_2[i];
	}
	// Spectral variability features.
	if (st->memid == RENAMENOISE_CEPS_MEM) {
		st->memid = 0;
	}
	for (i = 0; i < RENAMENOISE_CEPS_MEM; i++) {
		int j;
		float mindist = 1e15f;
		for (j = 0; j < RENAMENOISE_CEPS_MEM; j++) {
			int k;
			float dist = 0;
			for (k = 0; k < RENAMENOISE_NB_BANDS; k++) {
				float tmp;
				tmp = st->cepstral_mem[i][k] - st->cepstral_mem[j][k];
				dist += tmp * tmp;
			}
			if (j != i) {
				mindist = RENAMENOISE_MIN32(mindist, dist);
			}
		}
		spec_variability += mindist;
	}
	features[RENAMENOISE_NB_BANDS + 3 * RENAMENOISE_NB_DELTA_CEPS + 1] = spec_variability / RENAMENOISE_CEPS_MEM - 2.1;
	return RENAMENOISE_TRAINING && E < 0.1;
}

static void renamenoise_frame_synthesis(ReNameNoiseDenoiseState *st, float *out, const renamenoise_fft_cpx *y) {
	float x[RENAMENOISE_WINDOW_SIZE];
	int i;
	renamenoise_inverse_transform(x, y);
	renamenoise_apply_window(x);
	for (i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
		out[i] = x[i] + st->synthesis_mem[i];
	}
	RENAMENOISE_COPY(st->synthesis_mem, &x[RENAMENOISE_FRAME_SIZE], RENAMENOISE_FRAME_SIZE);
}

static void renamenoise_biquad(float *y, float mem[2], const float *x, const float *b, const float *a, int N) {
	int i;
	for (i = 0; i < N; i++) {
		float xi, yi;
		xi = x[i];
		yi = x[i] + mem[0];
		mem[0] = mem[1] + (b[0] * (double) xi - a[0] * (double) yi);
		mem[1] = (b[1] * (double) xi - a[1] * (double) yi);
		y[i] = yi;
	}
}

void renamenoise_pitch_filter(renamenoise_fft_cpx *X, const renamenoise_fft_cpx *P, const float *Ex, const float *Ep, const float *Exp,
							  const float *g) {
	int i;
	float r[RENAMENOISE_NB_BANDS];
	float rf[RENAMENOISE_FREQ_SIZE] = {0};
	for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
		if (Exp[i] > g[i]) {
			r[i] = 1;
		} else {
			r[i] = RENAMENOISE_SQUARE(Exp[i]) * (1 - RENAMENOISE_SQUARE(g[i])) / (.001 + RENAMENOISE_SQUARE(g[i]) * (1 - RENAMENOISE_SQUARE(Exp[i])));
		}
		r[i] = sqrt(RENAMENOISE_MIN16(1, RENAMENOISE_MAX16(0, r[i])));
		r[i] *= sqrt(Ex[i] / (1e-8 + Ep[i]));
	}
	renamenoise_interp_band_gain(rf, r);
	for (i = 0; i < RENAMENOISE_FREQ_SIZE; i++) {
		X[i].r += rf[i] * P[i].r;
		X[i].i += rf[i] * P[i].i;
	}
	float newE[RENAMENOISE_NB_BANDS];
	renamenoise_compute_band_energy(newE, X);
	float norm[RENAMENOISE_NB_BANDS];
	float normf[RENAMENOISE_FREQ_SIZE] = {0};
	for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
		norm[i] = sqrt(Ex[i] / (1e-8 + newE[i]));
	}
	renamenoise_interp_band_gain(normf, norm);
	for (i = 0; i < RENAMENOISE_FREQ_SIZE; i++) {
		X[i].r *= normf[i];
		X[i].i *= normf[i];
	}
}

float renamenoise_process_frame(ReNameNoiseDenoiseState *st, float *out, const float *in) {
	int i;
	renamenoise_fft_cpx X[RENAMENOISE_FREQ_SIZE];
	renamenoise_fft_cpx P[RENAMENOISE_WINDOW_SIZE];
	float x[RENAMENOISE_FRAME_SIZE];
	float Ex[RENAMENOISE_NB_BANDS], Ep[RENAMENOISE_NB_BANDS];
	float Exp[RENAMENOISE_NB_BANDS];
	float features[RENAMENOISE_NB_FEATURES];
	float g[RENAMENOISE_NB_BANDS];
	float gf[RENAMENOISE_FREQ_SIZE] = {1};
	float vad_prob = 0;
	int silence;
	static const float a_hp[2] = {-1.99599, 0.99600};
	static const float b_hp[2] = {-2, 1};
	renamenoise_biquad(x, st->mem_hp_x, in, b_hp, a_hp, RENAMENOISE_FRAME_SIZE);
	silence = renamenoise_compute_frame_features(st, X, P, Ex, Ep, Exp, features, x);

	if (!silence) {
		renamenoise_compute_rnn(&st->rnn, g, &vad_prob, features);
		renamenoise_pitch_filter(X, P, Ex, Ep, Exp, g);
		for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
			float alpha = .6f;
			g[i] = RENAMENOISE_MAX16(g[i], alpha * st->lastg[i]);
			st->lastg[i] = g[i];
		}
		renamenoise_interp_band_gain(gf, g);
		for (i = 0; i < RENAMENOISE_FREQ_SIZE; i++) {
			X[i].r *= gf[i];
			X[i].i *= gf[i];
		}
	}

	renamenoise_frame_synthesis(st, out, X);
	return vad_prob;
}

float renamenoise_process_frame_clamped(ReNameNoiseDenoiseState *st, short *out, const float *in) {
	float denoise_frames[RENAMENOISE_FRAME_SIZE];

	float vad_prob = renamenoise_process_frame(st, denoise_frames, in);

	for (unsigned int i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
		out[i] = (short) RENAMENOISE_MIN16(RENAMENOISE_MAX16(denoise_frames[i], SHRT_MIN), SHRT_MAX);
	}

	return vad_prob;
}

#if RENAMENOISE_TRAINING

static float renamenoise_uni_rand() {
	return rand() / (double) RAND_MAX - .5;
}

static void renamenoise_rand_resp(float *a, float *b) {
	a[0] = .75 * renamenoise_uni_rand();
	a[1] = .75 * renamenoise_uni_rand();
	b[0] = .75 * renamenoise_uni_rand();
	b[1] = .75 * renamenoise_uni_rand();
}

int main(int argc, char **argv) {
	int i;
	int count = 0;
	static const float a_hp[2] = {-1.99599, 0.99600};
	static const float b_hp[2] = {-2, 1};
	float a_noise[2] = {0};
	float b_noise[2] = {0};
	float a_sig[2] = {0};
	float b_sig[2] = {0};
	float mem_hp_x[2] = {0};
	float mem_hp_n[2] = {0};
	float mem_resp_x[2] = {0};
	float mem_resp_n[2] = {0};
	float x[RENAMENOISE_FRAME_SIZE];
	float n[RENAMENOISE_FRAME_SIZE];
	float xn[RENAMENOISE_FRAME_SIZE];
	int vad_cnt = 0;
	int gain_change_count = 0;
	float speech_gain = 1, noise_gain = 1;
	FILE *f1, *f2;
	int maxCount;
	ReNameNoiseDenoiseState *st;
	ReNameNoiseDenoiseState *noise_state;
	ReNameNoiseDenoiseState *noisy;
	st = renamenoise_create(NULL);
	noise_state = renamenoise_create(NULL);
	noisy = renamenoise_create(NULL);
	if (argc != 4) {
		fprintf(stderr, "usage: %s <speech> <noise> <count>\n", argv[0]);
		return 1;
	}
	f1 = fopen(argv[1], "r");
	f2 = fopen(argv[2], "r");
	maxCount = atoi(argv[3]);
	for (i = 0; i < 150; i++) {
		short tmp[RENAMENOISE_FRAME_SIZE];
		fread(tmp, sizeof(short), RENAMENOISE_FRAME_SIZE, f2);
	}
	while (1) {
		renamenoise_fft_cpx X[RENAMENOISE_FREQ_SIZE], Y[RENAMENOISE_FREQ_SIZE], N[RENAMENOISE_FREQ_SIZE], P[RENAMENOISE_WINDOW_SIZE];
		float Ex[RENAMENOISE_NB_BANDS], Ey[RENAMENOISE_NB_BANDS], En[RENAMENOISE_NB_BANDS], Ep[RENAMENOISE_NB_BANDS];
		float Exp[RENAMENOISE_NB_BANDS];
		float Ln[RENAMENOISE_NB_BANDS];
		float features[RENAMENOISE_NB_FEATURES];
		float g[RENAMENOISE_NB_BANDS];
		short tmp[RENAMENOISE_FRAME_SIZE];
		float vad = 0;
		float E = 0;
		if (count == maxCount) {
			break;
		}
		if ((count % 1000) == 0) {
			fprintf(stderr, "%d\r", count);
		}
		if (++gain_change_count > 2821) {
			speech_gain = pow(10., (-40 + (rand() % 60)) / 20.);
			noise_gain = pow(10., (-30 + (rand() % 50)) / 20.);
			if (rand() % 10 == 0) {
				noise_gain = 0;
			}
			noise_gain *= speech_gain;
			if (rand() % 10 == 0) {
				speech_gain = 0;
			}
			gain_change_count = 0;
			renamenoise_rand_resp(a_noise, b_noise);
			renamenoise_rand_resp(a_sig, b_sig);
			lowpass = RENAMENOISE_FREQ_SIZE * 3000. / 24000. * pow(50., rand() / (double) RAND_MAX);
			for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
				if (renamenoise_eband5ms[i] << RENAMENOISE_FRAME_SIZE_SHIFT > lowpass) {
					band_lp = i;
					break;
				}
			}
		}
		if (speech_gain != 0) {
			fread(tmp, sizeof(short), RENAMENOISE_FRAME_SIZE, f1);
			if (feof(f1)) {
				rewind(f1);
				fread(tmp, sizeof(short), RENAMENOISE_FRAME_SIZE, f1);
			}
			for (i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
				x[i] = speech_gain * tmp[i];
			}
			for (i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
				E += tmp[i] * (float) tmp[i];
			}
		} else {
			for (i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
				x[i] = 0;
			}
			E = 0;
		}
		if (noise_gain != 0) {
			fread(tmp, sizeof(short), RENAMENOISE_FRAME_SIZE, f2);
			if (feof(f2)) {
				rewind(f2);
				fread(tmp, sizeof(short), RENAMENOISE_FRAME_SIZE, f2);
			}
			for (i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
				n[i] = noise_gain * tmp[i];
			}
		} else {
			for (i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
				n[i] = 0;
			}
		}
		renamenoise_biquad(x, mem_hp_x, x, b_hp, a_hp, RENAMENOISE_FRAME_SIZE);
		renamenoise_biquad(x, mem_resp_x, x, b_sig, a_sig, RENAMENOISE_FRAME_SIZE);
		renamenoise_biquad(n, mem_hp_n, n, b_hp, a_hp, RENAMENOISE_FRAME_SIZE);
		renamenoise_biquad(n, mem_resp_n, n, b_noise, a_noise, RENAMENOISE_FRAME_SIZE);
		for (i = 0; i < RENAMENOISE_FRAME_SIZE; i++) {
			xn[i] = x[i] + n[i];
		}
		if (E > 1e9f) {
			vad_cnt = 0;
		} else if (E > 1e8f) {
			vad_cnt -= 5;
		} else if (E > 1e7f) {
			vad_cnt++;
		} else {
			vad_cnt += 2;
		}
		if (vad_cnt < 0) {
			vad_cnt = 0;
		}
		if (vad_cnt > 15) {
			vad_cnt = 15;
		}

		if (vad_cnt >= 10) {
			vad = 0;
		} else if (vad_cnt > 0) {
			vad = 0.5f;
		} else {
			vad = 1.f;
		}

		renamenoise_frame_analysis(st, Y, Ey, x);
		renamenoise_frame_analysis(noise_state, N, En, n);
		for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
			Ln[i] = log10(1e-2 + En[i]);
		}
		int silence = renamenoise_compute_frame_features(noisy, X, P, Ex, Ep, Exp, features, xn);
		renamenoise_pitch_filter(X, P, Ex, Ep, Exp, g);
		// printf("%f %d\n", noisy->last_gain, noisy->last_period);
		for (i = 0; i < RENAMENOISE_NB_BANDS; i++) {
			g[i] = sqrt((Ey[i] + 1e-3) / (Ex[i] + 1e-3));
			if (g[i] > 1) {
				g[i] = 1;
			}
			if (silence || i > band_lp) {
				g[i] = -1;
			}
			if (Ey[i] < 5e-2 && Ex[i] < 5e-2) {
				g[i] = -1;
			}
			if (vad == 0 && noise_gain == 0) {
				g[i] = -1;
			}
		}
		count++;
		fwrite(features, sizeof(float), RENAMENOISE_NB_FEATURES, stdout);
		fwrite(g, sizeof(float), RENAMENOISE_NB_BANDS, stdout);
		fwrite(Ln, sizeof(float), RENAMENOISE_NB_BANDS, stdout);
		fwrite(&vad, sizeof(float), 1, stdout);
	}
	fprintf(stderr, "matrix size: %d x %d\n", count, RENAMENOISE_NB_FEATURES + 2 * RENAMENOISE_NB_BANDS + 1);
	fclose(f1);
	fclose(f2);
	return 0;
}

#endif
