#ifndef FFT_H
#define FFT_H

// Based on fft.cc from audacious project
/*
 * fft.c
 * Copyright 2011 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#define N 512  /* size of the DFT */

/* Input is N=512 PCM samples.
 * Output is intensity of frequencies from 1 to N/2=256. */
void calc_freq(const float data[N], float freq[N / 2]);

#endif // FFT_H
