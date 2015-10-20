#ifndef __IMG_EN_H__
#define __IMG_EN_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum En_DataFormat
{
	EN_FORMAT_YUV420P = 0,
	EN_FORMAT_YUV420SP,
	EN_FORMAT_RGB565,
	EN_FORMAT_ABGR8888,
}En_DataFormat;

typedef enum En_MediaFormat
{
	EN_MEDIA_IMAGE = 0,
	EN_FORMAT_VIDEO,
}En_MediaFormat;

typedef struct En_Input_t
{
	int data_format;       /*En_DataFormat*/
	int media_format;    /*En_MediaFormat*/
	int width;
	int height;
	int reserve[4];
}En_Input_t;

typedef struct En_Parm_t
{
	unsigned char *src;
	unsigned char *dst;
	int reserve[6];
}En_Parm_t;

void* ImgEn_Open(void * input/*En_Input_t*/);
int ImgEn_Run(void* handle,void* param/*En_Parm_t*/);
void ImgEn_Close(void* handle);
#ifdef __cplusplus
}
#endif
#endif
