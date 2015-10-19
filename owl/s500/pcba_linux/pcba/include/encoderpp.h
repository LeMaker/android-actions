
#ifndef ENCODERPP_H
#define ENCODERPP_H

// #include "audiopp_dev.h"
#include "vad_detect.h"
#include "denoise.h"

#ifdef _WIN32
#include <stdlib.h>
#include <string.h>
#define actal_malloc   malloc
#define actal_free     free
#define actal_memset   memset

typedef __int64 int_64;

#define  max_energy    (0x80000000000000)  

#else

typedef long long int_64;
extern void *actal_malloc(int);
extern void actal_free(void *);
extern void *actal_memset(void *, int, int);

#define  max_energy    (0x80000000000000LL)  

#endif

#ifndef NULL
#define NULL 0
#endif

typedef struct
{
	int   energy_enable;        //是否支持能量输出，
	int   adc_enable;           //是否支持adc纠正

	int   avr_enable;           //是否支持声控， 
	int   fenqu_enable;         //是否支持自动分曲，
	
	int   agc_enable;           //是否支持自动音量增益， 
	int   denoise_enable;       //是否支持降噪， 

	int   gain_enable;          //是否支持录音增益

	int   sample_rate;          //当前采样率
	int   adc_channels;         //实际输入数据声道数
	int   encoder_channels;     //编码器需要的声道数
    int   encoder_frame_len;    //当前编码器要求输入数据帧长
    int   adc_frame_len;        //输入数据帧长          
	
    int   energy;               //取输入数据的前512个采样点计算得到的能量

    int   vad_mode;             //录音模式, 0:正常录音, 1:声控模式, 2:自动分曲
    int   vad_level;            //静音检测级别，用于确定门限值，为0表示采用初始录音的噪声平均值，不为0则-0.5dB一级   
	int   vad_delay;            //检测到静音后延迟一段时间才处理，单位ms

    int   agc_level;            //自动音量增益级别, 为0表示关闭 
    int   denoise_level;        //降噪级别，为0表示关闭

   	int   input_gain;           //录音增益设置，移位之后的调整
	int   input_shift;          //录音增益设置，需要移位的个数

	int   need_adc_flag;         //是否需要新的ADC数据  
    
	VAD_State * vad_st;
	int   last_vad_flag;

	int  *adcin_left;            //encpp用于缓存adc左声道输入数据的buffer
	int  *adcin_right;           //encpp用于缓存adc右声道输入数据的buffer

	int  *denoiseout_left;       //encpp用于缓存denoise后左声道输出数据的buffer
	int  *denoiseout_right;      //encpp用于缓存denoise后右声道输出数据的buffer

	DenoiseInterface *denoise_IF;
	int  adc_new_flag;
} Encoderpp_state;

int  sat16(int pcm_in);
void  sat16_4(int * in_ptr, short * out_ptr, int length);
int  cal_energy(short *pcm_in, int pcm_num, int channel);
int  set_energy_out(int energy_in);


#endif



