#ifndef __SKIMAGE_DECODER_ACT_H
#define __SKIMAGE_DECODER_ACT_H

#ifndef ANDROID_DEFAULT_CODE
#include "SkBitmap.h"

#define SDRAM_SUPPORT 1024
#define SHIFTVAL 5
#define SOFTDEC_MAX_SIZE  4096*4095
	
typedef struct scale_param{
		unsigned char *addr;
		unsigned int stride;
		unsigned int width;
		unsigned int height;
}scale_param_t;
	
typedef unsigned short u_scale;
  
int ACT_getDoEnhance();
void ACT_setDoEnhance(int doEnhance);
int image_enhance_func(int w, int h, SkColorType config, unsigned char *src);
void image_scale(scale_param_t *dst, scale_param_t *src);
void image_scale_rgb8888(scale_param_t *dst, scale_param_t *src);
int ACT_getSdramCapacity();
int ACT_getDecodeStandard();
int ACT_getHWCodecMaxWidth();
int ACT_getHWCodecMaxHeight();

void ACT_setSdramCapacity(int sdramcap);
void ACT_setDecodeStandard(int decodeStandard);
void ACT_setHWCodecMaxWidth(int HWMaxWidth);
void ACT_setHWCodecMaxHeight(int HWMaxHeight);

#endif /*ANDROID_DEFAULT_CODE*/	


#endif //__SKIMAGE_DECODER_ACT_H

