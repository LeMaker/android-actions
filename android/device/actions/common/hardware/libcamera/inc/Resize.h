#ifndef CAMERA_RESIZE_H
#define CAMERA_RESIZE_H

namespace android
{
typedef union{
    uint8_t *rgb;
    struct{
        uint8_t *y;
        uint8_t *u;
        uint8_t *v;
        uint8_t du;
        uint8_t dv;
        uint32_t yStride;
        uint32_t uStride;
        uint32_t vStride;
    };

}RGBOrYUV_t;

typedef struct{
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
}ImageCrop_t;

typedef struct{
    uint32_t inputW;
    uint32_t inputH;
    ImageCrop_t inputCrop;
    uint32_t outputW;
    uint32_t outputH;
}ImageResize_t;


void _YUV420Resize(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void NVXXResize(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YX12Resize(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void NVXXResizeIA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YX12ResizeIA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void NVXXToYX12ResizeIA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YX12ToNVXXResizeIA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void NVXXToYX12Resize(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YX12ToNVXXResize(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void NVXXResizeOA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YX12ResizeOA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void NVXXToYX12ResizeOA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YX12ToNVXXResizeOA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void NVXXResizeA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YX12ResizeA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void RGB565Resize(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);

void YUYVResize(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void NVXXToYX12ResizeA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YX12ToNVXXResizeA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);

void YUYVToYUV420Resize(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YUYVToYV12ResizeA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YUYVToNV21ResizeA(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YUYVToNV21Resize(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
void YUYVToNV12Resize(RGBOrYUV_t *out,
                    RGBOrYUV_t *in,
                    ImageResize_t *resize);
};
#endif //#define CAMERA_RESIZE_H_
