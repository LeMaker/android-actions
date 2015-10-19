#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "Resize.h"
#include "Converters.h"

#include <OMX_IVCommon.h>

#define ALIGN16(n) (((n)+0xf)&(~0xf))

#define SIMPLE_YUV420_CONV 1

namespace android
{

const char* getPixFormatConstant(const char* parameters_format)
{
    const char* pixFormat = NULL;

    if ( parameters_format != NULL )
    {
        if (strcmp(parameters_format, (const char *) CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
        {
            pixFormat = (const char *) CameraParameters::PIXEL_FORMAT_YUV422I;
        }
        //ActionsCode(author:liuyiguang, add_code, add support for PIXEL_FORMAT_YUV422SP)
        else if (strcmp(parameters_format, (const char *) CameraParameters::PIXEL_FORMAT_YUV422SP) == 0)
        {
            pixFormat = (const char *) CameraParameters::PIXEL_FORMAT_YUV422SP;
        }
        else if( strcmp(parameters_format, (const char *) CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
        {
            pixFormat = (const char *) CameraParameters::PIXEL_FORMAT_YUV420P;
        }
        else if(strcmp(parameters_format, (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
        {
            pixFormat = (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP;

        }
        else if( strcmp(parameters_format, (const char *) ActCameraParameters::PIXEL_FORMAT_YUV420P_YU12) == 0)
        {
            pixFormat = (const char *) ActCameraParameters::PIXEL_FORMAT_YUV420P_YU12;
        }
        else if(strcmp(parameters_format, (const char *) ActCameraParameters::PIXEL_FORMAT_YUV420SP_NV12) == 0)
        {
            pixFormat = (const char *) ActCameraParameters::PIXEL_FORMAT_YUV420SP_NV12;

        }
        else if(strcmp(parameters_format, (const char *) CameraParameters::PIXEL_FORMAT_RGB565) == 0)
        {
            pixFormat = (const char *) CameraParameters::PIXEL_FORMAT_RGB565;
        }
        else if(strcmp(parameters_format, (const char *) CameraParameters::PIXEL_FORMAT_JPEG) == 0)
        {
            pixFormat = (const char *) CameraParameters::PIXEL_FORMAT_JPEG;
        }
        else
        {
            ALOGE("format is NULL");
            pixFormat = NULL;
        }
    }
    else
    {
        ALOGE("format is NULL");
        pixFormat = NULL;
    }

    return pixFormat;
}

void saveFile(unsigned char   *buff, int width, int height, int bpp)
{
    static int      counter = 1;
    int             fd = -1;
    char            fn[256];

    LOG_FUNCTION_NAME;

    fn[0] = 0;
    sprintf(fn, "/data/camera_data/preview_%d_%d_%03d.yuv", width, height, counter);
    fd = open(fn, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0611);
    if(fd < 0)
    {
        ALOGE("Unable to open file %s: %s", fn, strerror(fd));
        return;
    }

    CAMHAL_LOGVB("Copying from 0x%p, size=%d x %d", buff, width, height);

    //method currently supports only nv12 dumping
    int stride = width*bpp>>3;
    uint8_t *bf = (uint8_t*) buff;
    for(int i=0; i<height; i++)
    {
        write(fd, bf, stride);
        bf += stride;
    }

    close(fd);

    counter++;

    LOG_FUNCTION_NAME_EXIT;
}

bool cameraFrameResizeYUYV(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;

    out.y = dst_buffer;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    if((resize.inputW == resize.inputCrop.w)
        &&(resize.inputH == resize.inputCrop.h)
        &&(resize.inputCrop.x == 0)
        &&(resize.inputCrop.y==0)
        &&(resize.outputW == resize.inputW)
        &&(resize.outputH == resize.inputH))
    {
        memcpy(dst_buffer, src_buffer, (resize.inputW*resize.inputH*16)>>3);
    }
    else
    {
        YUYVResize(&out, &in, &resize);
    }
    return true;
}

bool cameraFrameResizeYUYVToYV12_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;

    out.y = dst_buffer;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    
    YUYVToYV12ResizeA(&out, &in, &resize);
    return true;
}
bool cameraFrameResizeYUYVToNV21_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;

    out.y = dst_buffer;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    
    YUYVToNV21ResizeA(&out, &in, &resize);
    return true;
}
bool cameraFrameResizeYUYVToNV21(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;

    out.y = dst_buffer;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    
    YUYVToNV21Resize(&out, &in, &resize);
    return true;
}
bool cameraFrameResizeYUYVToNV12(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;

    out.y = dst_buffer;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    
    YUYVToNV12Resize(&out, &in, &resize);
    return true;
}
bool cameraFrameResizeRGB565(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.rgb = src_buffer;

    out.rgb = dst_buffer;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

   
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    if((resize.inputW == resize.inputCrop.w)
        &&(resize.inputH == resize.inputCrop.h)
        &&(resize.inputCrop.x == 0)
        &&(resize.inputCrop.y == 0)
        &&(resize.outputW == resize.inputW)
        &&(resize.outputH == resize.inputH))
    {
        memcpy(dst_buffer, src_buffer, (resize.inputW*resize.inputH*16)>>3);
    }
    else
    {
        RGB565Resize(&out, &in, &resize); 
    }
    return true;
}

bool cameraFrameResizeNV12ToNV12(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  


    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + 1;

    out.y = dst_buffer;
    out.u = dst_buffer +  cameraframe.mEncodeWidth * cameraframe.mEncodeHeight;
    out.v = out.u + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;

    if((resize.inputW == resize.inputCrop.w)
        &&(resize.inputH == resize.inputCrop.h)
        &&(resize.inputCrop.x == 0)
        &&(resize.inputCrop.y==0)
        &&(resize.outputW == resize.inputW)
        &&(resize.outputH == resize.inputH)
        &&!(resize.inputW%16))
    {
        memcpy(dst_buffer, src_buffer, 
            (ALIGN16(resize.inputW)*resize.inputH +ALIGN16(resize.inputW)*(resize.inputH>>1)));
    }
    else
    {
        NVXXResizeIA(&out, &in, &resize);
    }

    return true;
}
bool cameraFrameResizeNV21ToNV21(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  


    in.y = src_buffer;
    in.v = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.u = in.v + 1;

    out.y = dst_buffer;
    out.v = dst_buffer +  cameraframe.mEncodeWidth * cameraframe.mEncodeHeight;
    out.u = out.v + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;

    if((resize.inputW == resize.inputCrop.w)
        &&(resize.inputH == resize.inputCrop.h)
        &&(resize.inputCrop.x == 0)
        &&(resize.inputCrop.y==0)
        &&(resize.outputW == resize.inputW)
        &&(resize.outputH == resize.inputH)
        &&!(resize.inputW%16))
    {
        memcpy(dst_buffer, src_buffer,  
            (ALIGN16(resize.inputW)*resize.inputH +ALIGN16(resize.inputW)*(resize.inputH>>1)));
    }
    else
    {
        NVXXResizeIA(&out, &in, &resize);
    }

    return true;
}


bool cameraFrameResizeNV12ToNV21(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + 1;

    out.y = dst_buffer;
    out.v = dst_buffer +  cameraframe.mEncodeWidth * cameraframe.mEncodeHeight;
    out.u = out.v + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

   
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    NVXXResizeIA(&out, &in, &resize);       
    return true;
}

bool cameraFrameResizeYU12ToYU12(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.u = dst_buffer +  (cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.v = out.u + ((cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

   
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    if((resize.inputW == resize.inputCrop.w)
        &&(resize.inputH == resize.inputCrop.h)
        &&(resize.inputCrop.x == 0)
        &&(resize.inputCrop.y==0)
        &&(resize.outputW == resize.inputW)
        &&(resize.outputH == resize.inputH)
        &&!(resize.inputW%32))
    {
        memcpy(dst_buffer, src_buffer, 
            ALIGN16(resize.inputW)*resize.inputH + ALIGN16(resize.inputW>>1)*resize.inputH);
    }
    else
    {
        YX12ResizeIA(&out, &in, &resize); 
    }
    return true;
}

bool cameraFrameResizeYV12ToYV12(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.v = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.u = in.v + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.v = dst_buffer +  (cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + ((cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

   
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    if((resize.inputW == resize.inputCrop.w)
        &&(resize.inputH == resize.inputCrop.h)
        &&(resize.inputCrop.x == 0)
        &&(resize.inputCrop.y==0)
        &&(resize.outputW == resize.inputW)
        &&(resize.outputH == resize.inputH)
        &&!(resize.inputW%32))
    {
        memcpy(dst_buffer, src_buffer,
            ALIGN16(resize.inputW)*resize.inputH + ALIGN16(resize.inputW>>1)*resize.inputH);
    }
    else
    {
        YX12ResizeIA(&out, &in, &resize); 
    }
    return true;
}

bool cameraFrameResizeYU12ToYV12(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.v = dst_buffer +  (cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + ((cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    YX12ResizeIA(&out, &in, &resize);   
    return true;
}

bool cameraFrameResizeNV12ToYV12(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + 1;

    out.y = dst_buffer;
    out.v = dst_buffer +  (cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + ((cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    NVXXToYX12ResizeIA(&out, &in, &resize);   
    return true;
}
bool cameraFrameResizeNV21ToYV12(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.v = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.u = in.v + 1;

    out.y = dst_buffer;
    out.v = dst_buffer +  (cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + ((cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    NVXXToYX12ResizeIA(&out, &in, &resize);   
    return true;
}


bool cameraFrameResizeYV12ToNV21(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.v = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.u = in.v + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.v = dst_buffer +  (cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    YX12ToNVXXResizeIA(&out, &in, &resize);   
    return true;
}
bool cameraFrameResizeYU12ToNV21(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.v = dst_buffer +  (cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    YX12ToNVXXResizeIA(&out, &in, &resize);   
    return true;
}

bool cameraFrameResizeYU12ToNV12(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.u = dst_buffer +  (cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.v = out.u + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    YX12ToNVXXResizeIA(&out, &in, &resize);   
    return true;
}
bool cameraFrameResizeNV12ToYU12(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + 1;

    out.y = dst_buffer;
    out.u = dst_buffer +  (cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.v = out.u + ((cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    NVXXToYX12ResizeIA(&out, &in, &resize);   
    return true;
}


bool cameraFrameResizeNV12ToNV12_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  


    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + 1;

    out.y = dst_buffer;
    out.u = dst_buffer +  ALIGN16(cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.v = out.u + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;

    if((resize.inputW == resize.inputCrop.w)
        &&(resize.inputH == resize.inputCrop.h)
        &&(resize.inputCrop.x == 0)
        &&(resize.inputCrop.y==0)
        &&(resize.outputW == resize.inputW)
        &&(resize.outputH == resize.inputH)
        &&!(resize.inputW%16))
    {
        memcpy(dst_buffer, src_buffer, 
            ALIGN16(resize.inputW)*resize.inputH + ALIGN16(resize.inputW)*(resize.inputH>>1));
    }
    else
    {
        NVXXResizeA(&out, &in, &resize);
    }

    return true;
}
bool cameraFrameResizeNV21ToNV21_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  


    in.y = src_buffer;
    in.v = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.u = in.v + 1;

    out.y = dst_buffer;
    out.v = dst_buffer +  ALIGN16(cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;

    if((resize.inputW == resize.inputCrop.w)
        &&(resize.inputH == resize.inputCrop.h)
        &&(resize.inputCrop.x == 0)
        &&(resize.inputCrop.y==0)
        &&(resize.outputW == resize.inputW)
        &&(resize.outputH == resize.inputH)
        )
    {
        memcpy(dst_buffer, src_buffer, 
            ALIGN16(resize.inputW)*resize.inputH + ALIGN16(resize.inputW)*(resize.inputH>>1));
    }
    else
    {
        NVXXResizeA(&out, &in, &resize);
    }

    return true;
}


bool cameraFrameResizeNV12ToNV21_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + 1;

    out.y = dst_buffer;//
    out.v = dst_buffer +  ALIGN16(cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

   
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    NVXXResizeA(&out, &in, &resize);//       
    return true;
}

bool cameraFrameResizeYU12ToYU12_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.u = dst_buffer +  ALIGN16(cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.v = out.u + (ALIGN16(cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

   
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    if((resize.inputW == resize.inputCrop.w)
        &&(resize.inputH == resize.inputCrop.h)
        &&(resize.inputCrop.x == 0)
        &&(resize.inputCrop.y==0)
        &&(resize.outputW == resize.inputW)
        &&(resize.outputH == resize.inputH))
    {
        memcpy(dst_buffer, src_buffer, 
            ALIGN16(resize.inputW)*resize.inputH + ALIGN16(resize.inputW>>1)*(resize.inputH));
    }
    else
    {
        YX12ResizeA(&out, &in, &resize); 
    }
    return true;
}

bool cameraFrameResizeYV12ToYV12_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.v = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.u = in.v + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.v = dst_buffer +  ALIGN16(cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + (ALIGN16(cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

   
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    if((resize.inputW == resize.inputCrop.w)
        &&(resize.inputH == resize.inputCrop.h)
        &&(resize.inputCrop.x == 0)
        &&(resize.inputCrop.y==0)
        &&(resize.outputW == resize.inputW)
        &&(resize.outputH == resize.inputH))
    {
        memcpy(dst_buffer, src_buffer, ALIGN16(resize.inputW)*resize.inputH+ALIGN16(resize.inputW>>1)*resize.inputH);
    }
    else
    {
        YX12ResizeA(&out, &in, &resize); 
    }
    return true;
}

bool cameraFrameResizeYU12ToYV12_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.v = dst_buffer +  ALIGN16(cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + (ALIGN16(cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    YX12ResizeA(&out, &in, &resize);   
    return true;
}

bool cameraFrameResizeNV12ToYV12_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + 1;

    out.y = dst_buffer;
    out.v = dst_buffer +  ALIGN16(cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + (ALIGN16(cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    NVXXToYX12ResizeA(&out, &in, &resize);   
    return true;
}
bool cameraFrameResizeNV21ToYV12_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.v = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.u = in.v + 1;

    out.y = dst_buffer;
    out.v = dst_buffer +  ALIGN16(cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + (ALIGN16(cameraframe.mEncodeWidth>>1) * (cameraframe.mEncodeHeight>>1));

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    NVXXToYX12ResizeA(&out, &in, &resize);   
    return true;
}


bool cameraFrameResizeYV12ToNV21_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.v = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.u = in.v + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.v = dst_buffer +  ALIGN16(cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    YX12ToNVXXResizeA(&out, &in, &resize);   
    return true;
}
bool cameraFrameResizeYU12ToNV21_A16(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer)
{
    RGBOrYUV_t in, out;
    ImageResize_t resize;

    if (!src_buffer || !dst_buffer)
    {
        return false;
    }  

    in.y = src_buffer;
    in.u = src_buffer +  ALIGN16(cameraframe.mOrigWidth) * cameraframe.mOrigHeight;
    in.v = in.u + ALIGN16(cameraframe.mOrigWidth>>1) * (cameraframe.mOrigHeight>>1);

    out.y = dst_buffer;
    out.v = dst_buffer +  ALIGN16(cameraframe.mEncodeWidth) * cameraframe.mEncodeHeight;
    out.u = out.v + 1;

    resize.inputW = cameraframe.mOrigWidth;
    resize.inputH = cameraframe.mOrigHeight;
    resize.inputCrop.x = cameraframe.mXOff;
    resize.inputCrop.y = cameraframe.mYOff;
    resize.inputCrop.w =  cameraframe.mWidth;
    resize.inputCrop.h = cameraframe.mHeight;

    
    resize.outputW = cameraframe.mEncodeWidth;
    resize.outputH = cameraframe.mEncodeHeight;
    

    YX12ToNVXXResizeA(&out, &in, &resize);   
    return true;
}

/**
 *
 * BUGFIX:  Add YUV420P convert to YUV420P deal.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
bool cameraFrameResize(CameraFrame &cameraframe, unsigned char* src, unsigned char* dst, bool align)
{
    bool (*resize_func)(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer);
    resize_func = NULL;
    switch(cameraframe.mFormat)
    {
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            resize_func = align?cameraFrameResizeNV21ToYV12_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            resize_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            resize_func = align?cameraFrameResizeNV21ToNV21_A16:cameraFrameResizeNV21ToNV21;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            resize_func = NULL;
            break;
            default:
            resize_func = NULL;
            break;

        }
        break;

        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            //ActionsCode(author:liuyiguang, change_code)
            resize_func = align?cameraFrameResizeYV12ToYV12_A16:cameraFrameResizeYV12ToYV12;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            resize_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            resize_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            resize_func = NULL;
            break;
            default:
            resize_func = NULL;
            break;
        }
        break;

        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            resize_func = align?cameraFrameResizeNV12ToYV12_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            resize_func = NULL; 
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            resize_func = align?cameraFrameResizeNV12ToNV21_A16:NULL;//
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            resize_func = align?NULL:cameraFrameResizeNV12ToNV12;
            break;
            default:
            resize_func = NULL;
            break;
        }
        break;

        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            resize_func = align?cameraFrameResizeYU12ToYV12_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            resize_func = align?NULL:cameraFrameResizeYU12ToYU12;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            resize_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            resize_func = NULL;
            break;
            default:
            resize_func = NULL;
            break;
        }
        break;

        case CameraFrame::CAMERA_FRAME_FORMAT_YUV422I:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            resize_func = align?cameraFrameResizeYUYVToYV12_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            resize_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            resize_func = align?cameraFrameResizeYUYVToNV21_A16:cameraFrameResizeYUYVToNV21;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            resize_func = align?NULL:cameraFrameResizeYUYVToNV12;
            break;
            default:
            resize_func = NULL;
            break;
        }
        break;

        default:
        resize_func = NULL;
        break;
    }

    if(resize_func != NULL)
    {
        resize_func(cameraframe, src, dst);//
    }
    else
    {
        ALOGD("CameraFrameResize not support from %d to %d ", cameraframe.mFormat, cameraframe.mConvFormat);
    }

    return true;
}

/**
 *
 * BUGFIX:  Add YUV420P convert to YUV420P deal.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
bool cameraFrameConvert(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer, int src_len, bool align)
{
    int outw, outh;
    int w, h;
    unsigned char *src,*dst;

    outw = cameraframe.mEncodeWidth;
    outh = cameraframe.mEncodeHeight;

    w = outw;
    h = outh;

    src = src_buffer ;
    dst = dst_buffer;
    //ALOGE("cameraFrameConvert:fmt=%d, convfmt=%d, w=%d,h=%d,outw=%d,outh=%d",cameraframe.mFormat,cameraframe.mConvFormat,w,h,outw,outh);

    void (*convert_func)(void* src, void* dst,  int width, int height, int out_width, int out_height);

    convert_func = NULL;

    switch(cameraframe.mFormat)
    {
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            convert_func = align?NV21ToYV12_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            convert_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            convert_func = align?NV21ToNV21_A16:NV21ToNV21_IA16;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            convert_func = NULL;
            break;
            default:
            convert_func = NULL;
            break;

        }
        break;

        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            //ActionsCode(author:liuyiguang, change_code)
            convert_func = align?YV12ToYV12_A16:YV12ToYV12_IA16;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            convert_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            convert_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            convert_func = NULL;
            break;
            default:
            convert_func = NULL;
            break;
        }
        break;

        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            convert_func = align?NV12ToYV12_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            convert_func = NULL; 
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            convert_func = align?NV12ToNV21_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            convert_func = align?NULL:NV12ToNV12_IA16;
            break;
            default:
            convert_func = NULL;
            break;
        }
        break;

        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            convert_func = align?YU12ToYV12_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            convert_func = align?NULL:YU12ToYU12_IA16;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            convert_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            convert_func = NULL;
            break;
            default:
            convert_func = NULL;
            break;
        }
        break;

        case CameraFrame::CAMERA_FRAME_FORMAT_YUV422I:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            convert_func = align?YUYVToYV12_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            convert_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            convert_func = align?YUYVToNV21_A16:YUYVToNV21_IA16;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            convert_func = align?NULL:YUYVToNV12_IA16;
            break;
            default:
            convert_func = NULL;
            break;
        }
        break;

        default:
        convert_func = NULL;
        break;
    }

    if(convert_func != NULL)
    {
        convert_func(src, dst, w, h, outw, outh);
    }
    else
    {
        ALOGE("%s not implemented, convert from %d to %d)\n", __func__,cameraframe.mFormat, cameraframe.mConvFormat);
    }

    return true;

}     
bool cameraFrameConvertForVce(CameraFrame &cameraframe, unsigned char* src_buffer, unsigned char* dst_buffer, int src_len, bool align)
{
    int outw, outh;
    int w, h;
    unsigned char *src,*dst;

    outw = cameraframe.mEncodeWidth;
    outh = cameraframe.mEncodeHeight;

    w = (outw+0xf)&(~0xf);
    h = (outh+0xf)&(~0xf);

    src = src_buffer ;
    dst = dst_buffer;
    //ALOGE("cameraFrameConvertForVce:fmt=%d, convfmt=%d, w=%d,h=%d,outw=%d,outh=%d",cameraframe.mFormat,cameraframe.mConvFormat,w,h,outw,outh);
    void (*convert_func)(void* src, void* dst,  int width, int height, int out_width, int out_height);

    convert_func = NULL;

    switch(cameraframe.mFormat)
    {
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            convert_func = align?NV21ToYV12_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            convert_func = NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            convert_func = align?NV21ToNV21_A16:NV21ToNV21_IA16;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            convert_func = NULL;
            break;
            default:
            convert_func = NULL;
            break;

        }
        break;

        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
        switch(cameraframe.mConvFormat)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            convert_func = align?NV12ToYV12_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            convert_func = NULL; 
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            convert_func = align?NV12ToNV21_A16:NULL;
            break;
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            convert_func = align?NULL:NV12ToNV12_IA16;
            break;
            default:
            convert_func = NULL;
            break;
        }
        break;

        default:
        convert_func = NULL;
        break;
    }

    if(convert_func != NULL)
    {
        convert_func(src, dst, w, h, outw, outh);
    }
    else
    {
        ALOGE("%s not implemented, convert from %d to %d)\n", __func__,cameraframe.mFormat, cameraframe.mConvFormat);
    }

    return true;

}     

void cameraStartTime(struct timeval *start)
{
    gettimeofday(start, NULL);
}

void cameraDeltaTime(const char *str, struct timeval *start)
{
    struct timeval end;

    unsigned long long elapsed;
    gettimeofday(&end, NULL);
    elapsed = end.tv_sec - start->tv_sec;
    elapsed *= 1000000;
    elapsed += end.tv_usec - start->tv_usec;

    CAMHAL_LOGDB("deltaTime: %s :%llu.%llu ms", str, ( elapsed /1000 ), ( elapsed % 1000 ));
}

unsigned long long cameraGetMs()
{
    struct timeval time;
    unsigned long long ms;

    gettimeofday(&time, NULL);
    ms = time.tv_sec;
    ms *= 1000;
    ms += time.tv_usec /1000;

    return ms;
}        

void clearUnalignedBuf(unsigned char *buf, int w, int h, int length,int f)
{
    int i=0;
    unsigned char *p = NULL;
    int alignedlne = 0;
    int unalignedlne = 0;

    if(w%32 == 0)
    {
        return;
    }
    unalignedlne = w%32;
    alignedlne = w-unalignedlne;
    if(f == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP)
    {
        //y
        p= buf+alignedlne;
        for(i = 0; i < h; i++)
        {
            memset(p,0,unalignedlne); 
            p+=w;
        }
        //uv
        p= buf+w*h+alignedlne;
        for(i = 0; i < (h>>1); i++)
        {
            memset(p,128,unalignedlne); 
            p+=w;
        }

    }
    else if(f == CameraFrame::CAMERA_FRAME_FORMAT_YUV420P)
    {
        //y
        p= buf+alignedlne;
        for(i = 0; i < h; i++)
        {
            memset(p,0,unalignedlne); 
            p+=w;
        }
        //u
        p= buf+w*h+(alignedlne>>1);
        for(i = 0; i < (h>>1); i++)
        {
            memset(p,128,(unalignedlne>>1)); 
            p+=(w>>1);
        }
        //v
        p= buf+w*h*5/4+(alignedlne>>1);
        for(i = 0; i < (h>>1); i++)
        {
            memset(p,128,(unalignedlne>>1)); 
            p+=(w>>1);
        }
    }
    else
    {
        memset(buf, 0, length);
    }

}

};

