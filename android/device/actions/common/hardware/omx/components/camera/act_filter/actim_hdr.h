/*
 * Copyright (c) 2013 Actions Semiconductor Co., Ltd.
 * All rights reserved.
 *
 *
*/

////////////////////////////////////////////////////////
// HDRI.h
// Author(s): Zhi-quan Deng, Wen-hai Kong, Shi-heng Tan
// Version: v 0.2 2013/7/27

#ifndef ACTIMG_HDR_H
#define ACTIMG_HDR_H

typedef struct
{
    int width;
    int height;
    int format;
} frames_info_t;


typedef struct
{
    // only support yuv\yvu420 semi-planar by now
    int (*init)(void *handle, frames_info_t *frames_info);

    // exposure_time = real exposure time * ISO gain
    // align the added frame if doAlign != 0
    // must follow the sequence: over-exposure - normal exposure - under-exposure
    int (*addFrame)(void *handle, unsigned char *pixel_data, float exposure_time, int doAlign);

    // return the hdr image width & height by pointers
    // remove ghosts if doRemoveGhost != 0
    int (*blendFrames)(void *handle, int *width_hdr, int *height_hdr, int doRemoveGhost);

    // img_hdr points to the memory allocated according to width_hdr & height_hdr above
    int (*runToneMapping)(void *handle, unsigned char *img_hdr);

    int (*exit)(void *handle);
} hdri_t;

void *openHDRI();


#endif


