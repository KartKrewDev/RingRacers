/*
   Copyright (c) 2024, The Mumble Developers
   Copyright (c) 2017, Jean-Marc Valin

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

#ifndef RNN_H_
#define RNN_H_

#include "renamenoise.h"
#include "renamenoise_types.h"

#define RENAMENOISE_WEIGHTS_SCALE (1.f / 256)

#define RENAMENOISE_MAX_NEURONS 128

#define RENAMENOISE_ACTIVATION_TANH 0
#define RENAMENOISE_ACTIVATION_SIGMOID 1
#define RENAMENOISE_ACTIVATION_RELU 2

typedef signed char renamenoise_rnn_weight;

typedef struct {
	const renamenoise_rnn_weight *bias;
	const renamenoise_rnn_weight *input_weights;
	int nb_inputs;
	int nb_neurons;
	int activation;
} ReNameNoiseDenseLayer;

typedef struct {
	const renamenoise_rnn_weight *bias;
	const renamenoise_rnn_weight *input_weights;
	const renamenoise_rnn_weight *recurrent_weights;
	int nb_inputs;
	int nb_neurons;
	int activation;
} ReNameNoiseGRULayer;

typedef struct ReNameNoiseRNNState ReNameNoiseRNNState;

void renamenoise_compute_dense(const ReNameNoiseDenseLayer *layer, float *output, const float *input);

void renamenoise_compute_gru(const ReNameNoiseGRULayer *gru, float *state, const float *input);

void renamenoise_compute_rnn(ReNameNoiseRNNState *rnn, float *gains, float *vad, const float *input);

#endif /* RNN_H_ */
