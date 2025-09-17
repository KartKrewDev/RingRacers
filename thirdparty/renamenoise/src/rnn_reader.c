/*
   Copyright (c) 2024, The Mumble Developers
   Copyright (c) 2018, Gregor Richards

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

#include "renamenoise.h"
#include "rnn.h"
#include "rnn_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

// Although these values are the same as in rnn.h, we make them separate to
// avoid accidentally burning internal values into a file format
#define F_RENAMENOISE_ACTIVATION_TANH 0
#define F_RENAMENOISE_ACTIVATION_SIGMOID 1
#define F_RENAMENOISE_ACTIVATION_RELU 2

ReNameNoiseModel *renamenoise_model_from_file(FILE *f) {
	int i, in;

	if (fscanf(f, "renamenoise-nu model file version %d\n", &in) != 1 || in != 1) {
		return NULL;
	}

	ReNameNoiseModel *ret = calloc(1, sizeof(ReNameNoiseModel));
	if (!ret) {
		return NULL;
	}

#define RENAMENOISE_ALLOC_LAYER(type, name) \
	type *name;                             \
	name = calloc(1, sizeof(type));         \
	if (!name) {                            \
		renamenoise_model_free(ret);        \
		return NULL;                        \
	}                                       \
	ret->name = name

	RENAMENOISE_ALLOC_LAYER(ReNameNoiseDenseLayer, input_dense);
	RENAMENOISE_ALLOC_LAYER(ReNameNoiseGRULayer, vad_gru);
	RENAMENOISE_ALLOC_LAYER(ReNameNoiseGRULayer, noise_gru);
	RENAMENOISE_ALLOC_LAYER(ReNameNoiseGRULayer, denoise_gru);
	RENAMENOISE_ALLOC_LAYER(ReNameNoiseDenseLayer, denoise_output);
	RENAMENOISE_ALLOC_LAYER(ReNameNoiseDenseLayer, vad_output);

#define RENAMENOISE_INPUT_VAL(name)                            \
	do {                                                       \
		if (fscanf(f, "%d", &in) != 1 || in < 0 || in > 128) { \
			renamenoise_model_free(ret);                       \
			return NULL;                                       \
		}                                                      \
		name = in;                                             \
	} while (0)

#define RENAMENOISE_INPUT_ACTIVATION(name)                                                       \
	do {                                                                                         \
		int activation;                                                                          \
		RENAMENOISE_INPUT_VAL(activation);                                                       \
		switch (activation) {                                                                    \
			case F_RENAMENOISE_ACTIVATION_SIGMOID: name = RENAMENOISE_ACTIVATION_SIGMOID; break; \
			case F_RENAMENOISE_ACTIVATION_RELU: name = RENAMENOISE_ACTIVATION_RELU; break;       \
			default: name = RENAMENOISE_ACTIVATION_TANH;                                         \
		}                                                                                        \
	} while (0)

#define RENAMENOISE_INPUT_ARRAY(name, len)                                               \
	do {                                                                                 \
		renamenoise_rnn_weight *values = malloc((len) * sizeof(renamenoise_rnn_weight)); \
		if (!values) {                                                                   \
			renamenoise_model_free(ret);                                                 \
			return NULL;                                                                 \
		}                                                                                \
		name = values;                                                                   \
		for (i = 0; i < (len); i++) {                                                    \
			if (fscanf(f, "%d", &in) != 1) {                                             \
				renamenoise_model_free(ret);                                             \
				return NULL;                                                             \
			}                                                                            \
			values[i] = in;                                                              \
		}                                                                                \
	} while (0)

#define RENAMENOISE_INPUT_DENSE(name)                                                     \
	do {                                                                                  \
		RENAMENOISE_INPUT_VAL(name->nb_inputs);                                           \
		RENAMENOISE_INPUT_VAL(name->nb_neurons);                                          \
		ret->name##_size = name->nb_neurons;                                              \
		RENAMENOISE_INPUT_ACTIVATION(name->activation);                                   \
		RENAMENOISE_INPUT_ARRAY(name->input_weights, name->nb_inputs * name->nb_neurons); \
		RENAMENOISE_INPUT_ARRAY(name->bias, name->nb_neurons);                            \
	} while (0)

#define RENAMENOISE_INPUT_GRU(name)                                                                \
	do {                                                                                           \
		RENAMENOISE_INPUT_VAL(name->nb_inputs);                                                    \
		RENAMENOISE_INPUT_VAL(name->nb_neurons);                                                   \
		ret->name##_size = name->nb_neurons;                                                       \
		RENAMENOISE_INPUT_ACTIVATION(name->activation);                                            \
		RENAMENOISE_INPUT_ARRAY(name->input_weights, name->nb_inputs * name->nb_neurons * 3);      \
		RENAMENOISE_INPUT_ARRAY(name->recurrent_weights, name->nb_neurons * name->nb_neurons * 3); \
		RENAMENOISE_INPUT_ARRAY(name->bias, name->nb_neurons * 3);                                 \
	} while (0)

	RENAMENOISE_INPUT_DENSE(input_dense);
	RENAMENOISE_INPUT_GRU(vad_gru);
	RENAMENOISE_INPUT_GRU(noise_gru);
	RENAMENOISE_INPUT_GRU(denoise_gru);
	RENAMENOISE_INPUT_DENSE(denoise_output);
	RENAMENOISE_INPUT_DENSE(vad_output);

	return ret;
}

void renamenoise_model_free(ReNameNoiseModel *model) {
#define RENAMENOISE_FREE_MAYBE(ptr) \
	do {                            \
		if (ptr)                    \
			free(ptr);              \
	} while (0)
#define RENAMENOISE_FREE_DENSE(name)                   \
	do {                                               \
		if (model->name) {                             \
			free((void *) model->name->input_weights); \
			free((void *) model->name->bias);          \
			free((void *) model->name);                \
		}                                              \
	} while (0)
#define RENAMENOISE_FREE_GRU(name)                         \
	do {                                                   \
		if (model->name) {                                 \
			free((void *) model->name->input_weights);     \
			free((void *) model->name->recurrent_weights); \
			free((void *) model->name->bias);              \
			free((void *) model->name);                    \
		}                                                  \
	} while (0)

	if (!model) {
		return;
	}
	RENAMENOISE_FREE_DENSE(input_dense);
	RENAMENOISE_FREE_GRU(vad_gru);
	RENAMENOISE_FREE_GRU(noise_gru);
	RENAMENOISE_FREE_GRU(denoise_gru);
	RENAMENOISE_FREE_DENSE(denoise_output);
	RENAMENOISE_FREE_DENSE(vad_output);
	free(model);
}
