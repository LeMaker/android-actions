/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Contains implemenation of framebuffer conversion routines.
 */
#include "CameraHalDebug.h"

#include "Converters.h"

#define ALIGN16(n) (((n)+0xf)&(~0xf))

#define NVXX_PLANE_ADDR(src, w, h, y, c1, c2) do{     \
    uint8_t* s = (uint8_t*)(src);                     \
    *((uint8_t**)(y))=s;                              \
    *((uint8_t**)(c1))=s+ (w) * (h);           \
    *((uint8_t**)(c2))=*(c1)+1;                       \
}while(0);

#define YX12_PLANE_ADDR(src, w, h, y, c1, c2) do{      \
    uint8_t* s = (uint8_t*)(src);                      \
    *(uint8_t**)(y)=s;                                 \
    *(uint8_t**)(c1)=s+ (w) * (h);              \
    *(uint8_t**)(c2)=*(c1)+((w)>>1) * ((h)>>1); \
}while(0);

#define NVXX_PLANE_ADDR_A16(src, w, h, y, c1, c2) do{  \
    uint8_t* s = (uint8_t*)(src);                      \
    *(uint8_t**)(y)=s;                                 \
    *(uint8_t**)(c1)=s+ ALIGN16(w) * (h);              \
    *(uint8_t**)(c2)=*(c1)+1;                          \
}while(0);

#define YX12_PLANE_ADDR_A16(src, w, h, y, c1, c2) do{  \
    uint8_t* s = (uint8_t*)(src);                      \
    *(uint8_t**)(y)=s;                                 \
    *(uint8_t**)(c1)=s+ ALIGN16(w) * (h);              \
    *(uint8_t**)(c2)=*(c1)+ALIGN16((w)>>1) * ((h)>>1); \
}while(0);

#define NVXX_SIZE(w, h)     ((w)*(h)*3/2)
#define YX12_SIZE(w, h)     ((w)*(h)*3/2)
#define NVXX_SIZE_A16(w, h)     (ALIGN16(w)*(h)*3/2)
#define YX12_SIZE_A16(w, h)     (ALIGN16(w)*(h) + ALIGN16((w)>>1)*(h))


namespace android
{
static void _YUV420SToYUV420S(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint8_t* oY,
                             uint8_t* oU,
                             uint8_t* oV,
                             int odUV,
                             int width,
                             int height)
{
    memcpy((void *)oY,Y,width*height);

    for (int h = 0; h < (height>>1); h++)
    {
        for (int w = 0; (w < (width>>1)); w++,U += dUV, V += dUV,oU +=odUV, oV += odUV)
        {
            *oU = *(U);
            *oV = *(V);
        }
    }
}

static void _YUV420SToYUV420S_OA16(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint8_t* oY,
                             uint8_t* oU,
                             uint8_t* oV,
                             int odUV,
                             int width,
                             int height,
                             int out_width,
                             int out_height)
{
    const uint8_t* U_pos;
    const uint8_t* V_pos;
    uint8_t* oU_pos;
    uint8_t* oV_pos;

    if(out_height > height || out_width > width)
    {
        return;
    }
    //copy y
    for(int h = 0; h < out_height; h++)
    {
        memcpy(oY, Y, out_width);
        Y+=width;
        oY+=ALIGN16(out_width);
    }

    for (int h = 0; h < (out_height>>1); h++)
    {
        U_pos = U;
        V_pos = V;
        oU_pos = oU;
        oV_pos = oV;
        for (int w = 0; (w < (width>>1)); w++,U += dUV, V += dUV,oU +=odUV, oV += odUV)
        {
            *oU = *(U);
            *oV = *(V);
        }

        U = U_pos + (width>>1)*dUV;
        V = V_pos + (width>>1)*dUV;

        oU = oU_pos + ALIGN16((out_width*odUV)>>1);
        oV = oV_pos + ALIGN16((out_width*odUV)>>1);
    } 
}

//YV12 to YU12, YV12 to YV12, YU12 to YU12, YU12 to YV12
static void _YX12ToYX12_OA16(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint8_t* oY,
                             uint8_t* oU,
                             uint8_t* oV,
                             int odUV,
                             int width,
                             int height,
                             int out_width,
                             int out_height)
{
    if(out_height > height || out_width > width)
    {
        return;
    }
    //y
    for(int h = 0; h < out_height; h++)
    {
        memcpy(oY, Y, out_width);
        Y+=width;
        oY+=ALIGN16(out_width);
    }
    //uv
    for(int h = 0; h < (out_height>>1); h++)
    {
        memcpy(oV, V, (out_width>>1));
        V+=width>>1;
        oV+=ALIGN16((out_width>>1));

        memcpy(oU, U, (out_width>>1));
        U+=width>>1;
        oU+=ALIGN16((out_width>>1));
    }
}

//NV12 to NV12 or NV21 to NV21
static void _NVXXToNVXX_OA16(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint8_t* oY,
                             uint8_t* oU,
                             uint8_t* oV,
                             int odUV,
                             int width,
                             int height,
                             int out_width,
                             int out_height)
{
    if(out_height > height || out_width > width)
    {
        return;
    }
    //y
    for(int h = 0; h < out_height; h++)
    {
        memcpy(oY, Y, out_width);
        Y+=width;
        oY+=ALIGN16(out_width);
    }
    //uv
    for(int h = 0; h < (out_height>>1); h++)
    {
        memcpy(oU, U, (out_width));
        U+=width;
        oU+=ALIGN16((out_width));
    }
}

//**
static void _YUV420SToYUV420S_A16(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint8_t* oY,
                             uint8_t* oU,
                             uint8_t* oV,
                             int odUV,
                             int width,
                             int height,
                             int out_width,
                             int out_height)
{
    const uint8_t* U_pos;
    const uint8_t* V_pos;
    uint8_t* oU_pos;
    uint8_t* oV_pos;

    if(out_height > height || out_width > width)
    {
        return;
    }
    //copy y
    for(int h = 0; h < out_height; h++)
    {
        memcpy(oY, Y, out_width);
        Y+=ALIGN16(width);
        oY+=ALIGN16(out_width);
    }

    for (int h = 0; h < (out_height>>1); h++)
    {
        U_pos = U;
        V_pos = V;
        oU_pos = oU;
        oV_pos = oV;
        for (int w = 0; (w < (width>>1)); w++,U += dUV, V += dUV,oU +=odUV, oV += odUV)
        {
            *oU = *(U);
            *oV = *(V);
        }

        U = U_pos + ALIGN16((width*dUV)>>1);
        V = V_pos + ALIGN16((width*dUV)>>1);

        oU = oU_pos + ALIGN16((out_width*odUV)>>1);
        oV = oV_pos + ALIGN16((out_width*odUV)>>1);
    } 
}

//YV12 to YU12, YV12 to YV12, YU12 to YU12, YU12 to YV12
static void _YX12ToYX12_A16(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint8_t* oY,
                             uint8_t* oU,
                             uint8_t* oV,
                             int odUV,
                             int width,
                             int height,
                             int out_width,
                             int out_height)
{
    if(out_height > height || out_width > width)
    {
        return;
    }
    //y
    for(int h = 0; h < out_height; h++)
    {
        memcpy(oY, Y, out_width);
        Y+=ALIGN16(width);
        oY+=ALIGN16(out_width);
    }
    //uv
    for(int h = 0; h < (out_height>>1); h++)
    {
        memcpy(oV, V, (out_width>>1));
        V+=ALIGN16(width>>1);
        oV+=ALIGN16((out_width>>1));

        memcpy(oU, U, (out_width>>1));
        U+=ALIGN16(width>>1);
        oU+=ALIGN16((out_width>>1));
    }
}

//NV12 to NV12 or NV21 to NV21
static void _NVXXToNVXX_A16(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint8_t* oY,
                             uint8_t* oU,
                             uint8_t* oV,
                             int odUV,
                             int width,
                             int height,
                             int out_width,
                             int out_height)
{
    if(out_height > height || out_width > width)
    {
        return;
    }
    //y
    for(int h = 0; h < out_height; h++)
    {
        memcpy(oY, Y, out_width);
        Y+=ALIGN16(width);
        oY+=ALIGN16(out_width);
    }
    //uv
    for(int h = 0; h < (out_height>>1); h++)
    {
        memcpy(oU, U, (out_width));
        U+=ALIGN16(width);
        oU+=ALIGN16((out_width));
    }
}

//**
static void _YUV420SToYUV420S_IA16(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint8_t* oY,
                             uint8_t* oU,
                             uint8_t* oV,
                             int odUV,
                             int width,
                             int height,
                             int out_width,
                             int out_height)
{
    const uint8_t* U_pos;
    const uint8_t* V_pos;
    uint8_t* oU_pos;
    uint8_t* oV_pos;

    if(out_height > height || out_width > width)
    {
        return;
    }
    //copy y
    for(int h = 0; h < out_height; h++)
    {
        memcpy(oY, Y, out_width);
        Y+=ALIGN16(width);
        oY+=(out_width);
    }

    for (int h = 0; h < (out_height>>1); h++)
    {
        U_pos = U;
        V_pos = V;
        oU_pos = oU;
        oV_pos = oV;
        for (int w = 0; (w < (width>>1)); w++,U += dUV, V += dUV,oU +=odUV, oV += odUV)
        {
            *oU = *(U);
            *oV = *(V);
        }

        U = U_pos + ALIGN16((width*dUV)>>1);
        V = V_pos + ALIGN16((width*dUV)>>1);

        oU = oU_pos + (out_width>>1)*odUV;
        oV = oV_pos + (out_width>>1)*odUV;
    } 
}

//YV12 to YU12, YV12 to YV12, YU12 to YU12, YU12 to YV12
static void _YX12ToYX12_IA16(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint8_t* oY,
                             uint8_t* oU,
                             uint8_t* oV,
                             int odUV,
                             int width,
                             int height,
                             int out_width,
                             int out_height)
{
    if(out_height > height || out_width > width)
    {
        return;
    }
    //y
    for(int h = 0; h < out_height; h++)
    {
        memcpy(oY, Y, out_width);
        Y+=ALIGN16(width);
        oY+=(out_width);
    }
    //uv
    for(int h = 0; h < (out_height>>1); h++)
    {
        memcpy(oV, V, (out_width>>1));
        V+=ALIGN16(width>>1);
        oV+=((out_width>>1));

        memcpy(oU, U, (out_width>>1));
        U+=ALIGN16(width>>1);
        oU+=((out_width>>1));
    }
}

//NV12 to NV12 or NV21 to NV21
static void _NVXXToNVXX_IA16(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint8_t* oY,
                             uint8_t* oU,
                             uint8_t* oV,
                             int odUV,
                             int width,
                             int height,
                             int out_width,
                             int out_height)
{
    if(out_height > height || out_width > width)
    {
        return;
    }
    //y
    for(int h = 0; h < out_height; h++)
    {
        memcpy(oY, Y, out_width);
        Y+=ALIGN16(width);
        oY+=(out_width);
    }
    //uv
    for(int h = 0; h < (out_height>>1); h++)
    {
        memcpy(oU, U, (out_width));
        U+=ALIGN16(width);
        oU+=((out_width));
    }
}



static void _YUV420SToRGB565(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint16_t* rgb,
                             int width,
                             int height)
{
    const uint8_t* U_pos = U;
    const uint8_t* V_pos = V;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x += 2, U += dUV, V += dUV)
        {
            const uint8_t nU = *U;
            const uint8_t nV = *V;
            *rgb = YUVToRGB565(*Y, nU, nV);
            Y++;
            rgb++;
            *rgb = YUVToRGB565(*Y, nU, nV);
            Y++;
            rgb++;
        }
        if (y & 0x1)
        {
            U_pos = U;
            V_pos = V;
        }
        else
        {
            U = U_pos;
            V = V_pos;
        }
    }
}


static void _RGB565ToYUV420S(const uint16_t* rgb,
                             uint8_t* Y,
                             uint8_t* U,
                             uint8_t* V,
                             int dUV,
                             int width,
                             int height)
{
    uint8_t y, u0, u1,u2,u3, v0,v1,v2,v3;

    for (int h = 0; h < (height>>1); h++)
    {
        for (int w = 0; (w < (width>>1)); w++,rgb+=2, Y+=2,U += dUV, V += dUV)
        {

            RGB565ToYUV(*rgb, &y, &u0, &v0);
            *Y = y;
            RGB565ToYUV(*(rgb+1), &y, &u1, &v1);
            *(Y+1) = y;
            RGB565ToYUV(*(rgb+width), &y, &u2, &v2);
            *(Y+width) = y;
            RGB565ToYUV(*(rgb+width+1), &y, &u3, &v3);
            *(Y+width+1) = y;

            *U = (u1+u1+u2+u3)>>2;
            *V = (v1+v1+v2+v3)>>2;
        }
        Y+=width;
    }
}


static void _YUV420SToRGB32(const uint8_t* Y,
                            const uint8_t* U,
                            const uint8_t* V,
                            int dUV,
                            uint32_t* rgb,
                            int width,
                            int height)
{
    const uint8_t* U_pos = U;
    const uint8_t* V_pos = V;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x += 2, U += dUV, V += dUV)
        {
            const uint8_t nU = *U;
            const uint8_t nV = *V;
            *rgb = YUVToRGB32(*Y, nU, nV);
            Y++;
            rgb++;
            *rgb = YUVToRGB32(*Y, nU, nV);
            Y++;
            rgb++;
        }
        if (y & 0x1)
        {
            U_pos = U;
            V_pos = V;
        }
        else
        {
            U = U_pos;
            V = V_pos;
        }
    }
}

void YV12ToRGB565(void* yv12, void* rgb, int width, int height)
{
    uint8_t* Y;
    uint8_t* V;
    uint8_t* U;

    YX12_PLANE_ADDR(yv12, width, height, &Y, &V, &U);

    _YUV420SToRGB565(Y, U, V, 1, reinterpret_cast<uint16_t*>(rgb), width, height);
}

void YV12ToRGB32(void* yv12, void* rgb, int width, int height)
{
    uint8_t* Y;
    uint8_t* V;
    uint8_t* U;

    YX12_PLANE_ADDR(yv12, width, height, &Y, &V, &U);

    _YUV420SToRGB32(Y, U, V, 1, reinterpret_cast<uint32_t*>(rgb), width, height);
}


void YU12ToRGB565(void* yv12, void* rgb, int width, int height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;

    YX12_PLANE_ADDR(yv12, width, height, &Y, &U, &V);
    
    _YUV420SToRGB565(Y, U, V, 1, reinterpret_cast<uint16_t*>(rgb), width, height);
}


void YU12ToRGB32(void* yu12, void* rgb, int width, int height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;

    YX12_PLANE_ADDR(yu12, width, height, &Y, &U, &V);

    _YUV420SToRGB32(Y, U, V, 1, reinterpret_cast<uint32_t*>(rgb), width, height);
}

/* Common converter for YUV 4:2:0 interleaved to RGB565.
 * y, u, and v point to Y,U, and V panes, where U and V values are interleaved.
 */
static void _NVXXToRGB565(const uint8_t* Y,
                          const uint8_t* U,
                          const uint8_t* V,
                          uint16_t* rgb,
                          int width,
                          int height)
{
    _YUV420SToRGB565(Y, U, V, 2, rgb, width, height);
}

/* Common converter for YUV 4:2:0 interleaved to RGB565.
 * y, u, and v point to Y,U, and V panes, where U and V values are interleaved.
 */

static void _RGB565ToNVXX(const uint16_t* rgb,
                          uint8_t* Y,
                          uint8_t* U,
                          uint8_t* V,
                          int width,
                          int height)
{
    _RGB565ToYUV420S(rgb, Y, U, V, 2, width, height);
}

/* Common converter for YUV 4:2:0 interleaved to RGB32.
 * y, u, and v point to Y,U, and V panes, where U and V values are interleaved.
 */
static void _NVXXToRGB32(const uint8_t* Y,
                         const uint8_t* U,
                         const uint8_t* V,
                         uint32_t* rgb,
                         int width,
                         int height)
{
    _YUV420SToRGB32(Y, U, V, 2, rgb, width, height);
}

void NV12ToRGB565(void* nv12, void* rgb, int width, int height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;

    NVXX_PLANE_ADDR(nv12, width, height, &Y, &U, &V);    

    _NVXXToRGB565(Y, U, V,
                  reinterpret_cast<uint16_t*>(rgb), width, height);
}

void NV12ToRGB32(void* nv12, void* rgb, int width, int height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;

    NVXX_PLANE_ADDR(nv12, width, height, &Y, &U, &V);    
    
    _NVXXToRGB32(Y, U, V,
                 reinterpret_cast<uint32_t*>(rgb), width, height);
}

void NV21ToRGB565(void* nv21, void* rgb, int width, int height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;

    NVXX_PLANE_ADDR(nv21, width, height, &Y, &V, &U);    
    
    _NVXXToRGB565(Y, U, V,
                  reinterpret_cast<uint16_t*>(rgb), width, height);
}

void NV21ToRGB32(void* nv21, void* rgb, int width, int height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;

    NVXX_PLANE_ADDR(nv21, width, height, &Y, &V, &U);    

    _NVXXToRGB32(Y, U, V,
                 reinterpret_cast<uint32_t*>(rgb), width, height);
}


void RGB565ToNV12(void* rgb, void* nv21,  int width, int height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;

    NVXX_PLANE_ADDR(nv21, width, height, &Y, &U, &V);    
    _RGB565ToNVXX(reinterpret_cast<uint16_t*>(rgb), Y, U, V,
                  width, height);
}

void NV12ToNV12(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y ;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;

    if(width == out_width
        && height == out_height)
    {
        memcpy(dst, src, NVXX_SIZE(width, height));
        return;
    }
    NVXX_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    NVXX_PLANE_ADDR(dst, out_width, out_height, &oY, &oU, &oV);    
    _YUV420SToYUV420S(Y, U, V, 2, oY, oU, oV, 2,  width,  height);
}

void NV12ToYU12(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oU, &oV);    
    _YUV420SToYUV420S(Y, U, V, 2, oY, oU, oV, 1,  width,  height);
}

void YU12ToNV12(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    NVXX_PLANE_ADDR(dst, out_width, out_height, &oY, &oU, &oV);    

    _YUV420SToYUV420S(Y, U, V, 1, oY, oU, oV, 2,  width,  height);
}

void YU12ToYU12(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oU, &oV);    

    if(width == out_width
        && height == out_height)
    {
        memcpy(dst, src, YX12_SIZE(width, height));
        return;
    }
    _YUV420SToYUV420S(Y, U, V, 1, oY, oU, oV, 1,  width,  height);
}

void NV21ToNV21(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR(src, width, height, &Y, &V, &U);    
    NVXX_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    
    
    if(width == out_width
        && height == out_height)
    {
        memcpy(dst, src, NVXX_SIZE(width, height));
        return;
    }
    _YUV420SToYUV420S(Y, U, V, 2, oY, oU, oV, 2,  width,  height);
}

void NV21ToYV12(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR(src, width, height, &Y, &V, &U);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YUV420SToYUV420S(Y, U, V, 2, oY, oU, oV, 1,  width,  height);
}

void YV12ToNV21(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR(src, width, height, &Y, &V, &U);    
    NVXX_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    

    _YUV420SToYUV420S(Y, U, V, 1, oY, oU, oV, 2,  width,  height);
}

void YV12ToYV12(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR(src, width, height, &Y, &V, &U);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    
    
    if(width == out_width
        && height == out_height)
    {
        memcpy(dst, src, YX12_SIZE(width, height));
        return;
    }
    _YUV420SToYUV420S(Y, U, V, 1, oY, oU, oV, 1,  width,  height);
}

void NV12ToNV21(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    NVXX_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    
   
    _YUV420SToYUV420S(Y, U, V, 2, oY, oU, oV, 2,  width,  height);
}

void NV12ToYV12(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YUV420SToYUV420S(Y, U, V, 2, oY, oU, oV, 1,  width,  height);
}

void YU12ToYV12(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YUV420SToYUV420S(Y, U, V, 1, oY, oU, oV, 1,  width,  height);
}

void NV12ToNV21_OA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    NVXX_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YUV420SToYUV420S_OA16(Y, U, V, 2, oY, oU, oV, 2,  width,  height,  out_width,  out_height);
}
void NV12ToYV12_OA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YUV420SToYUV420S_OA16(Y, U, V, 2, oY, oU, oV, 1,  width,  height,  out_width,  out_height);
}
void NV21ToYV12_OA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR(src, width, height, &Y, &V, &U);    
    YX12_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YUV420SToYUV420S_OA16(Y, U, V, 2, oY, oU, oV, 1,  width,  height,  out_width,  out_height);
}
void YU12ToYV12_OA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    

    _YX12ToYX12_OA16(Y, U, V, 1, oY, oU, oV, 1,  width,  height, out_width,  out_height);
}
void YV12ToYV12_OA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR(src, width, height, &Y, &V, &U);    
    YX12_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    

    if(width == out_width
        && height == out_height
        && !(out_width%32)
        )
    {
        memcpy(dst, src, YX12_SIZE_A16(out_width , out_height));
        return;
    }
    _YX12ToYX12_OA16(Y, U, V, 1, oY, oU, oV, 1,  width,  height, out_width,  out_height);

}

void YU12ToNV21_OA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR(src, width, height, &Y, &U, &V);    
    NVXX_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    

    _YUV420SToYUV420S_OA16(Y, U, V, 1, oY, oU, oV, 2,  width,  height, out_width,  out_height);
}

void YV12ToNV21_OA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR(src, width, height, &Y, &V, &U);    
    NVXX_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    

    _YUV420SToYUV420S_OA16(Y, U, V, 1, oY, oU, oV, 2,  width,  height, out_width,  out_height);
}

void NV21ToNV21_OA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{   
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR(src, width, height, &Y, &V, &U);    
    NVXX_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    

    if(width == out_width
        && height == out_height
        && !(out_width%16)
        )
    {
        memcpy(dst, src, NVXX_SIZE_A16(out_width,out_height));
        return;
    }

    _NVXXToNVXX_OA16(Y, U, V, 2, oY, oU, oV, 2,  width,  height, out_width,  out_height);
}

//****
void NV12ToNV12_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    NVXX_PLANE_ADDR(dst, out_width, out_height, &oY, &oU, &oV);    

    if(width == out_width
        && height == out_height
        && !(width%16))
    {
        memcpy(dst, src, NVXX_SIZE_A16(out_width,out_height));
        return;
    }
    _YUV420SToYUV420S_IA16(Y, U, V, 2, oY, oU, oV, 2,  width,  height,  out_width,  out_height);
}

void NV12ToYU12_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oU, &oV);    

    _YUV420SToYUV420S_IA16(Y, U, V, 2, oY, oU, oV, 1,  width,  height,  out_width,  out_height);
}

void YU12ToNV12_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    NVXX_PLANE_ADDR(dst, out_width, out_height, &oY, &oU, &oV);    

    _YUV420SToYUV420S_IA16(Y, U, V, 1, oY, oU, oV, 2,  width,  height,  out_width,  out_height);
}

void YU12ToYU12_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oU, &oV);    

    if(width == out_width
        && height == out_height
        && !(width%32))
    {
        memcpy(dst, src, YX12_SIZE_A16(out_width,out_height));
        return;
    }
    _YUV420SToYUV420S_IA16(Y, U, V, 1, oY, oU, oV, 1,  width,  height,  out_width,  out_height);
}

void NV21ToNV21_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR_A16(src, width, height, &Y, &V, &U);    
    NVXX_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    

    if(width == out_width
        && height == out_height
        && !(width%16))
    {
        memcpy(dst, src, NVXX_SIZE_A16(out_width,out_height));
        return;
    }
    _YUV420SToYUV420S_IA16(Y, U, V, 2, oY, oU, oV, 2,  width,  height,  out_width,  out_height);
}

void NV21ToYV12_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR_A16(src, width, height, &Y, &V, &U);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    

    _YUV420SToYUV420S_IA16(Y, U, V, 2, oY, oU, oV, 1,  width,  height,  out_width,  out_height);
}

void YV12ToNV21_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR_A16(src, width, height, &Y, &V, &U);    
    NVXX_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    

    _YUV420SToYUV420S_IA16(Y, U, V, 1, oY, oU, oV, 2,  width,  height,  out_width,  out_height);
}

void YV12ToYV12_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR_A16(src, width, height, &Y, &V, &U);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    

    if(width == out_width
        && height == out_height
        && !(width%32))
    {
        memcpy(dst, src, YX12_SIZE_A16(out_width,out_height));
        return;
    }
    _YUV420SToYUV420S_IA16(Y, U, V, 1, oY, oU, oV, 1,  width,  height,  out_width,  out_height);
}

void NV12ToNV21_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    NVXX_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    

    _YUV420SToYUV420S_IA16(Y, U, V, 2, oY, oU, oV, 2,  width,  height,  out_width,  out_height);
}

void NV12ToYV12_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YUV420SToYUV420S_IA16(Y, U, V, 2, oY, oU, oV, 1,  width,  height,  out_width,  out_height);
}

void YU12ToYV12_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR(dst, out_width, out_height, &oY, &oV, &oU);    

    _YUV420SToYUV420S_IA16(Y, U, V, 1, oY, oU, oV, 1,  width,  height,  out_width,  out_height);
}

//****
void NV12ToNV21_A16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    NVXX_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    

    _YUV420SToYUV420S_A16(Y, U, V, 2, oY, oU, oV, 2,  width,  height,  out_width,  out_height);
}
void NV12ToYV12_A16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    

    _YUV420SToYUV420S_A16(Y, U, V, 2, oY, oU, oV, 1,  width,  height,  out_width,  out_height);
}
void NV21ToYV12_A16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR_A16(src, width, height, &Y, &V, &U);    
    YX12_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YUV420SToYUV420S_A16(Y, U, V, 2, oY, oU, oV, 1,  width,  height,  out_width,  out_height);
}
void YU12ToYV12_A16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    YX12_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YX12ToYX12_A16(Y, U, V, 1, oY, oU, oV, 1,  width,  height, out_width,  out_height);
}
void YV12ToYV12_A16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR_A16(src, width, height, &Y, &V, &U);    
    YX12_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    
    
    if(width == out_width
        && height == out_height
        )
    {
        memcpy(dst, src, YX12_SIZE_A16(width,height));
        return;
    }
    _YX12ToYX12_A16(Y, U, V, 1, oY, oU, oV, 1,  width,  height, out_width,  out_height);

}

void YU12ToNV21_A16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR_A16(src, width, height, &Y, &U, &V);    
    NVXX_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YUV420SToYUV420S_A16(Y, U, V, 1, oY, oU, oV, 2,  width,  height, out_width,  out_height);
}

void YV12ToNV21_A16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    YX12_PLANE_ADDR_A16(src, width, height, &Y, &V, &U);    
    NVXX_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    
    
    _YUV420SToYUV420S_A16(Y, U, V, 1, oY, oU, oV, 2,  width,  height, out_width,  out_height);
}

void NV21ToNV21_A16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{   
    uint8_t* Y;
    uint8_t* U;
    uint8_t* V;
    uint8_t* oY;
    uint8_t* oU;
    uint8_t* oV;
    NVXX_PLANE_ADDR_A16(src, width, height, &Y, &V, &U);    
    NVXX_PLANE_ADDR_A16(dst, out_width, out_height, &oY, &oV, &oU);    
    
    if(width == out_width
        && height == out_height
        )
    {
        memcpy(dst, src, NVXX_SIZE_A16(width,height));
        return;
    }

    _NVXXToNVXX_A16(Y, U, V, 2, oY, oU, oV, 2,  width,  height, out_width,  out_height);
}

void YUYVToYV12_A16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    int r,c;

    int src_stride;
    int dst_stride_y;
    int dst_stride_uv;
    unsigned char *dst_y, *dst_u, *dst_v;
    unsigned char *s;

    if (!src || !dst  
        || width <= 0 || height <= 0
        || out_width <= 0 || out_height <= 0
        || width < out_width || height < out_height
        ) 
    {
        return ;
    }

    src_stride = width*2;
    dst_stride_y = ALIGN16(width);
    dst_stride_uv = ALIGN16((width>>1));
    s = (unsigned char *)src;

    YX12_PLANE_ADDR_A16(dst, out_width, out_height, &dst_y, &dst_v, &dst_u);

    for(r=0; r<out_height; r++)
    {
        for(c=0; c<out_width; c++)
        {
            //y
            dst_y[r*dst_stride_y + c] = s[r*src_stride+c*2];
            //vu
            if((r&0x1) == 0 && (c&0x1) == 0)
            {
                //v
                dst_v[(r>>1)*dst_stride_uv+(c>>1)]=s[r*src_stride+(c-c%2)*2+3];
                //u
                dst_u[(r>>1)*dst_stride_uv+(c>>1)]=s[r*src_stride+(c-c%2)*2 +1];
            }
        }
    }

}
void YUYVToNV21_A16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    int r,c;

    int src_stride;
    int dst_stride_y;
    int dst_stride_uv;
    unsigned char *dst_y, *dst_u, *dst_v;
    unsigned char *s;

    if (!src || !dst  
        || width <= 0 || height <= 0
        || out_width <= 0 || out_height <= 0
        || width < out_width || height < out_height
        ) 
    {
        return ;
    }

    src_stride = width*2;
    dst_stride_y = ALIGN16(width);
    dst_stride_uv = ALIGN16((width));
    s = (unsigned char *)src;

    NVXX_PLANE_ADDR_A16(dst, out_width, out_height, &dst_y, &dst_v, &dst_u);

    for(r=0; r<out_height; r++)
    {
        for(c=0; c<out_width; c++)
        {
            //y
            dst_y[r*dst_stride_y + c] = s[r*src_stride+c*2];
            //vu
            if((r&0x1) == 0 && (c&0x1) == 0)
            {
                //v
                dst_v[(r>>1)*dst_stride_uv+(c>>1)*2]=s[r*src_stride+(c-c%2)*2+3];
                //u
                dst_u[(r>>1)*dst_stride_uv+(c>>1)*2]=s[r*src_stride+(c-c%2)*2 +1];
            }
        }
    }

}
void YUYVToNV12_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    int r,c;

    int src_stride;
    int dst_stride_y;
    int dst_stride_uv;
    unsigned char *dst_y, *dst_u, *dst_v;
    unsigned char *s;

    if (!src || !dst  
        || width <= 0 || height <= 0
        || out_width <= 0 || out_height <= 0
        || width < out_width || height < out_height
        ) 
    {
        return ;
    }

    src_stride = width*2;
    dst_stride_y = (width);
    dst_stride_uv = ((width));
    s = (unsigned char *)src;

    NVXX_PLANE_ADDR(dst, out_width, out_height, &dst_y, &dst_u, &dst_v);

    for(r=0; r<out_height; r++)
    {
        for(c=0; c<out_width; c++)
        {
            //y
            dst_y[r*dst_stride_y + c] = s[r*src_stride+c*2];
            //vu
            if((r&0x1) == 0 && (c&0x1) == 0)
            {
                //v
                dst_v[(r>>1)*dst_stride_uv+(c>>1)*2]=s[r*src_stride+(c-c%2)*2+3];
                //u
                dst_u[(r>>1)*dst_stride_uv+(c>>1)*2]=s[r*src_stride+(c-c%2)*2 +1];
            }
        }
    }

}
void YUYVToNV21_IA16(void* src, void* dst,  int width, int height, int out_width, int out_height)
{
    int r,c;

    int src_stride;
    int dst_stride_y;
    int dst_stride_uv;
    unsigned char *dst_y, *dst_u, *dst_v;
    unsigned char *s;

    if (!src || !dst  
        || width <= 0 || height <= 0
        || out_width <= 0 || out_height <= 0
        || width < out_width || height < out_height
        ) 
    {
        return ;
    }

    src_stride = width*2;
    dst_stride_y = (width);
    dst_stride_uv = ((width));
    s = (unsigned char *)src;

    NVXX_PLANE_ADDR(dst, out_width, out_height, &dst_y, &dst_v, &dst_u);

    for(r=0; r<out_height; r++)
    {
        for(c=0; c<out_width; c++)
        {
            //y
            dst_y[r*dst_stride_y + c] = s[r*src_stride+c*2];
            //vu
            if((r&0x1) == 0 && (c&0x1) == 0)
            {
                //v
                dst_v[(r>>1)*dst_stride_uv+(c>>1)*2]=s[r*src_stride+(c-c%2)*2+3];
                //u
                dst_u[(r>>1)*dst_stride_uv+(c>>1)*2]=s[r*src_stride+(c-c%2)*2 +1];
            }
        }
    }

}

}; /* namespace android */
