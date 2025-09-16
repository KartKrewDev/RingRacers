#!/bin/sh

gcc -DRENAMENOISE_TRAINING=1 -Wall -W -O3 -g -I../include denoise.c renamenoise_fft.c pitch.c renamenoise_lpc.c rnn.c rnn_data.c -o denoise_training -lm
