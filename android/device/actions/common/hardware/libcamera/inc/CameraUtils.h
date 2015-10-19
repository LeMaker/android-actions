
#include "CameraHal.h"

#include <OMX_IVCommon.h>

#ifndef CAMERA_UTILS_H
#define CAMERA_UTILS_H



namespace android
{

class CameraFrame;

const char* getPixFormatConstant(const char* parameters_format);

void saveFile(unsigned char   *buff, int width, int height, int bpp);
bool cameraFrameResize(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer, bool align);
bool cameraFrameConvert(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer, int src_len, bool align);
bool cameraFrameConvertForVce(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer, int src_len, bool align);
void cameraDeltaTime(const char *str, struct timeval *start);
void cameraStartTime(struct timeval *start);

unsigned long long cameraGetMs();

void clearUnalignedBuf(unsigned char *buf, int w, int h, int length,int f);
}

#endif
