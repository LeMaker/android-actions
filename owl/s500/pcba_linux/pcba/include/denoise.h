//ty 20090511

#ifndef DENOISE_H
#define DENOISE_H

#include "fft.h"

#ifndef NULL
#define NULL 0
#endif

#ifdef _WIN32

#define  sqrt_int_const 0x4000000000000000

#else

#define  sqrt_int_const 0x4000000000000000LL

#endif

extern int HmWnd129[129];
extern int HmWnd257[257];

extern int Wn_fft256_x[128];
//extern int Wn_ifft256_x[128];          //与Wn_fft256_x一致
extern int Wn_ifft256_y[128];
extern int Wn_fft256_y[128];

extern int Wn_fft512_x[256];
//extern int Wn_ifft512_x[256];          //与Wn_fft512_x一致
extern int Wn_ifft512_y[256];
extern int Wn_fft512_y[256];

typedef struct
{
	int       speech_sum;
	int       noise_sum;
	
	int       frame_count;
	int       speech_count;
	int       noise_init_count;   //左右声道各自保持

	int       noise_count;
	int       vad_flag;
	int       vad_time;

	int       Level;

	int       noise_change; 
	int       soft_noise;                   
	int       soft_speech; 

	int       alpha;
	int       beta;

	int*      power;
	int*      noise_power;
	int*      gain;
    int*      gain_before;

	int*      overlapin;          //输入叠加
	int*      overlapout;         //输出叠加

}DenoiseState;

typedef struct
{
    int            L;
	int            M;
	int*           HMwindow;           //汉明窗指针

	int*           Wn_fft_x;
	int*           Wn_fft_y;

	int*           Wn_ifft_x;
	int*           Wn_ifft_y;

	int*           fftout_x;
	int*           fftout_y;

	int*           fftin_x;
	int*           fftin_y;

	int*           fft_L_x;
	int*           fft_L_y;

	int*           fft_R_x;
	int*           fft_R_y;
    
	int            Data_out_ready;
	int            Need_data_in;
	int            DMA_frame_size;
	int            Enc_frame_size;

	int            channel;
	int*           left_frame_out;
	int*           right_frame_out;

	int            remain_size;
	int*           denoise_left_remain;
	int*           denoise_right_remain;

	DenoiseState*  left_st;
	DenoiseState*  right_st;

	int            denoise_run_count;

}DenoiseInterface;

//denoise接口
DenoiseInterface* denoise_init(int SampleRate, int channel, int framesize, int level);

void denoise_run(DenoiseInterface * st, int *left_datain, int *right_datain, int *left_dataout, int *right_dataout,int channel);

void denoise_close(DenoiseInterface * st);

//内部调用
DenoiseState* DenoiseState_creat(int SampleRate, int L, int level);

void DenoiseState_destroy(DenoiseState * st);

void get_spectrum(DenoiseInterface * st, int* overin_L, int* overin_R, int *left_datain, int *right_datain, int channel);

void frame_denoise(int * fftinout_x, int * fftinout_y, DenoiseState * st, int L);

void speech_rebuild(DenoiseInterface * st, int* overout_L, int* overout_R, int channel);

int *enc_memcpy(int *dst, const int*sst, int length);

#endif
