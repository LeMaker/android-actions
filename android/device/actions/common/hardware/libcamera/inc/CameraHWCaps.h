#ifndef __CAMERA_HW_CAPS__
#define __CAMERA_HW_CAPS__

namespace android
{

enum
{
    FORMAT_NV12YU12_TYPE = 0,
    FORMAT_NV21YV12_TYPE,
};

int getHwVceFormatType();
int getHwVideoFormatType();
int getHwJpegFormatType();
int getHwZoomFormatType();

int getHwVideoFormatAligned();

int getHwVideoEncodeFormatPreferred(int format);
//ActionsCode(author:liuyiguang, change_code)
//int isHwVceSupport(int sw, int sh, int sf, int dw, int dh, int df);
//int isHwZoomSupport(int sw, int sh, int sf, int dw, int dh, int df);
//int isHwJpegEncodeSupport(int sw, int sh, int sf, int dw, int dh, int df);
int isHwVceSupport(int sw, int sh, int sf, int dw, int dh, int df, int stride);
int isHwZoomSupport(int sw, int sh, int sf, int dw, int dh, int df, int stride);
int isHwJpegEncodeSupport(int sw, int sh, int sf, int dw, int dh, int df, int stride);

int getHwVceOutFormat(int format);


};

#endif
