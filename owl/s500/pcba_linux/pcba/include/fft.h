//ty
#ifndef FFT_H
#define FFT_H

#define WndCoeff(x)         (int)((x)*0x7fffffff)

#define int_mul(x1,x2)      (int)(((int_64)(x1)*(x2) + 0x40000000)>>31)

#define dsp_mul(x1,x2)      (int)(((int_64)(x1)*(x2) + 0x400000)>>23)

#define int16_mul(x1,x2)     (int)(((int_64)(x1)*(x2) + 0x4000)>>15)

void fft(int* fftin_x, int* fftin_y, int* fftout_x, int* fftout_y, int* Wn_x, int* Wn_y, int m);

#endif

