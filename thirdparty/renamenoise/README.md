# ReNameNoise

ReNameNoise - a fork of [RNNoise](https://gitlab.xiph.org/xiph/rnnoise) - is a noise suppression library based on a recurrent neural network.

A description of the algorithm is provided in the following paper:

```
J.-M. Valin, A Hybrid DSP/Deep Learning Approach to Real-Time Full-Band Speech
Enhancement, Proceedings of IEEE Multimedia Signal Processing (MMSP) Workshop,
arXiv:1709.08243, 2018.
https://arxiv.org/pdf/1709.08243.pdf
```

An interactive demo is available at: https://jmvalin.ca/demo/rnnoise/

## Prerequisites

To build the library with the existing, pre-trained network data you will need to install the following packages:

* build-essential
* cmake

## Build

To compile, open a terminal in the repository root directory and type:

```bash
cmake .
make
```

To compile the library only, without the demo executable:

```bash
cmake -DRENAMENOISE_DEMO_EXECUTABLE=OFF .
make
```

## Usage

While it is meant to be used as a library, a simple command-line tool is
provided as an example. It operates on RAW 16-bit (machine endian) mono
PCM files sampled at 48 kHz. It can be used as:

``./examples/renamenoise_demo <noisy speech> <output denoised>``

The output is also a 16-bit raw PCM file.d

## Training

Training is not necessary to use the library as presented in this repository.

However, if you want to train the network on your own samples you need to follow these steps:

```bash
cd src ; ./compile.sh

./denoise_training signal.raw noise.raw count > training.f32

# (note the matrix size and replace 500000 87 below)

cd training ; ./bin2hdf5.py ../src/training.f32 500000 87 training.h5

./rnn_train.py

./dump_rnn.py weights.hdf5 ../src/rnn_data.c ../src/rnn_data.h
```
