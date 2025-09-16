/*
   Copyright (c) 2024,      The Mumble Developers
   Copyright (c) 2012-2017, Jean-Marc Valin
   Copyright (c) 2008-2011, Octasic Inc

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

#include "rnn.h"

#include "arch.h"
#include "common.h"
#include "renamenoise_types.h"
#include "rnn_data.h"
#include "tansig_table.h"

#include <math.h>
#include <stdio.h>

static RENAMENOISE_INLINE float renamenoise_tansig_approx(float x) {
	int i;
	float y, dy;
	float sign = 1;
	// Tests are reversed to catch NaNs
	if (!(x < 8)) {
		return 1;
	}
	if (!(x > -8)) {
		return -1;
	}
	// Another check in case of -ffast-math
	if (renamenoise_isnan(x)) {
		return 0;
	}
	if (x < 0) {
		x = -x;
		sign = -1;
	}
	i = (int) floor(.5f + 25 * x);
	x -= .04f * i;
	y = renamenoise_tansig_table[i];
	dy = 1 - y * y;
	y = y + x * dy * (1 - y * x);
	return sign * y;
}

static RENAMENOISE_INLINE float renamenoise_sigmoid_approx(float x) {
	return .5 + .5 * renamenoise_tansig_approx(.5 * x);
}

static RENAMENOISE_INLINE float renamenoise_relu(float x) {
	return x < 0 ? 0 : x;
}

static void renamenoise_faxpy(float *restrict a, const renamenoise_rnn_weight *restrict b, const int k, const float u) {
	if (u == 0.0) {
		return;
	}
	for (int idx = 0; idx < k; idx++) {
		a[idx] += b[idx] * u;
	}
}

static void renamenoise_faxpy2(float *restrict a, const renamenoise_rnn_weight *restrict b, const int k, const float u, const float u2) {
	if (u == 0.0 || u2 == 0.0) {
		return;
	}
	for (int idx = 0; idx < k; idx++) {
		a[idx] += (b[idx] * u) * u2;
	}
}

void renamenoise_compute_dense(const ReNameNoiseDenseLayer *layer, float *output, const float *input) {
	int i, j;
	int N, M;
	M = layer->nb_inputs;
	N = layer->nb_neurons;
	const renamenoise_rnn_weight *ip = layer->input_weights;

	// Compute update gate.
	for (i = 0; i < N; i++) {
		output[i] = layer->bias[i];
	}
	for (j = 0; j < M; j++, ip += N) {
		renamenoise_faxpy(output, ip, N, input[j]);
	}
	if (layer->activation == RENAMENOISE_ACTIVATION_SIGMOID) {
		for (i = 0; i < N; i++) {
			output[i] = renamenoise_sigmoid_approx(RENAMENOISE_WEIGHTS_SCALE * output[i]);
		}
	} else if (layer->activation == RENAMENOISE_ACTIVATION_TANH) {
		for (i = 0; i < N; i++) {
			output[i] = renamenoise_tansig_approx(RENAMENOISE_WEIGHTS_SCALE * output[i]);
		}
	} else if (layer->activation == RENAMENOISE_ACTIVATION_RELU) {
		for (i = 0; i < N; i++) {
			output[i] = renamenoise_relu(RENAMENOISE_WEIGHTS_SCALE * output[i]);
		}
	} else {
		renamenoise_unreachable();
	}
}

void renamenoise_compute_gru(const ReNameNoiseGRULayer *gru, float *state, const float *input) {
	int i, j;
	int N, M;
	int stride;
	float z[RENAMENOISE_MAX_NEURONS];
	float r[RENAMENOISE_MAX_NEURONS];
	float h[RENAMENOISE_MAX_NEURONS];
	M = gru->nb_inputs;
	N = gru->nb_neurons;
	stride = 3 * N;
	const renamenoise_rnn_weight *ip = gru->input_weights;
	const renamenoise_rnn_weight *rp = gru->recurrent_weights;

	// Compute update gate.
	for (i = 0; i < N; i++) {
		z[i] = gru->bias[i];
	}
	for (j = 0; j < M; j++, ip += stride) {
		renamenoise_faxpy(z, ip, N, input[j]);
	}
	for (j = 0; j < N; j++, rp += stride) {
		renamenoise_faxpy(z, rp, N, state[j]);
	}
	for (i = 0; i < N; i++) {
		z[i] = renamenoise_sigmoid_approx(RENAMENOISE_WEIGHTS_SCALE * z[i]);
	}

	// Compute reset gate.
	for (i = 0; i < N; i++) {
		r[i] = gru->bias[N + i];
	}
	ip = gru->input_weights + N;
	rp = gru->recurrent_weights + N;
	for (j = 0; j < M; j++, ip += stride) {
		renamenoise_faxpy(r, ip, N, input[j]);
	}
	for (j = 0; j < N; j++, rp += stride) {
		renamenoise_faxpy(r, rp, N, state[j]);
	}
	for (i = 0; i < N; i++) {
		r[i] = renamenoise_sigmoid_approx(RENAMENOISE_WEIGHTS_SCALE * r[i]);
	}

	// Compute output.
	for (i = 0; i < N; i++) {
		h[i] = gru->bias[2 * N + i];
	}

	ip = gru->input_weights + 2 * N;
	rp = gru->recurrent_weights + 2 * N;

	for (j = 0; j < M; j++, ip += stride) {
		renamenoise_faxpy(h, ip, N, input[j]);
	}

	for (j = 0; j < N; j++, rp += stride) {
		renamenoise_faxpy2(h, rp, N, state[j], r[j]);
	}

	for (i = 0; i < N; i++) {
		if (gru->activation == RENAMENOISE_ACTIVATION_SIGMOID) {
			h[i] = renamenoise_sigmoid_approx(RENAMENOISE_WEIGHTS_SCALE * h[i]);
		} else if (gru->activation == RENAMENOISE_ACTIVATION_TANH) {
			h[i] = renamenoise_tansig_approx(RENAMENOISE_WEIGHTS_SCALE * h[i]);
		} else if (gru->activation == RENAMENOISE_ACTIVATION_RELU) {
			h[i] = renamenoise_relu(RENAMENOISE_WEIGHTS_SCALE * h[i]);
		} else {
			renamenoise_unreachable();
		}
		h[i] = z[i] * state[i] + (1 - z[i]) * h[i];
	}

	memcpy((void *) state, (void *) h, N * sizeof(float));
}

#define RENAMENOISE_INPUT_SIZE 42

void renamenoise_compute_rnn(ReNameNoiseRNNState *rnn, float *gains, float *vad, const float *input) {
	int i;
	float dense_out[RENAMENOISE_MAX_NEURONS];
	float noise_input[RENAMENOISE_MAX_NEURONS * 3];
	float denoise_input[RENAMENOISE_MAX_NEURONS * 3];
	renamenoise_compute_dense(rnn->model->input_dense, dense_out, input);
	renamenoise_compute_gru(rnn->model->vad_gru, rnn->vad_gru_state, dense_out);
	renamenoise_compute_dense(rnn->model->vad_output, vad, rnn->vad_gru_state);
	for (i = 0; i < rnn->model->input_dense_size; i++) {
		noise_input[i] = dense_out[i];
	}
	for (i = 0; i < rnn->model->vad_gru_size; i++) {
		noise_input[i + rnn->model->input_dense_size] = rnn->vad_gru_state[i];
	}
	for (i = 0; i < RENAMENOISE_INPUT_SIZE; i++) {
		noise_input[i + rnn->model->input_dense_size + rnn->model->vad_gru_size] = input[i];
	}
	renamenoise_compute_gru(rnn->model->noise_gru, rnn->noise_gru_state, noise_input);

	for (i = 0; i < rnn->model->vad_gru_size; i++) {
		denoise_input[i] = rnn->vad_gru_state[i];
	}
	for (i = 0; i < rnn->model->noise_gru_size; i++) {
		denoise_input[i + rnn->model->vad_gru_size] = rnn->noise_gru_state[i];
	}
	for (i = 0; i < RENAMENOISE_INPUT_SIZE; i++) {
		denoise_input[i + rnn->model->vad_gru_size + rnn->model->noise_gru_size] = input[i];
	}
	renamenoise_compute_gru(rnn->model->denoise_gru, rnn->denoise_gru_state, denoise_input);
	renamenoise_compute_dense(rnn->model->denoise_output, gains, rnn->denoise_gru_state);
}
