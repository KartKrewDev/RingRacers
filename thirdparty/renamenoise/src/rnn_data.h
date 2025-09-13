/*
   Copyright (c) 2024, The Mumble Developers
   Copyright (c) 2017, Xiph.Org Foundation, Jean-Marc Valin

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

#ifndef RNN_DATA_H
#define RNN_DATA_H

#include "rnn.h"

struct ReNameNoiseModel {
	int input_dense_size;
	const ReNameNoiseDenseLayer *input_dense;

	int vad_gru_size;
	const ReNameNoiseGRULayer *vad_gru;

	int noise_gru_size;
	const ReNameNoiseGRULayer *noise_gru;

	int denoise_gru_size;
	const ReNameNoiseGRULayer *denoise_gru;

	int denoise_output_size;
	const ReNameNoiseDenseLayer *denoise_output;

	int vad_output_size;
	const ReNameNoiseDenseLayer *vad_output;
};

struct ReNameNoiseRNNState {
	const ReNameNoiseModel *model;
	float *vad_gru_state;
	float *noise_gru_state;
	float *denoise_gru_state;
};

#endif /* RNN_DATA_H */
