#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "log.h"
#include "vce_cfg.h"
#include "omx_malloc.h"
#include "vce_resize.h"

#define ALIGN_32(x)  (((x) + 0x1f) & (~0x1f))
#define ALIGN_16(x)  (((x) + 0xf) & (~0xf))

#define VR_TEST

int main(void)
{
    double cur_ms;
    struct timeval stv0, stv1;
    int i, ret = 0;
    FILE *fin = NULL, *fout = NULL;
    unsigned long phy_in = 0, vir_in = 0;
    unsigned long phy_out = 0, vir_out = 0;
    int len_in = 0, len_out = 0;
    void *handle = NULL;
    VR_Input_t input;
    VR_Parm_t param;
    VR_Cmd_t cmd_param;
    int stride = 1600;
    int width = 1600;
    int height = 1200;
    int wscale = 2;
    int hscale = 2;
    int frames = 3;
    int cropx = 0;
    int cropy = 150;
    int dw = 1600;
    int dh = 900;
    int dstw = 1920;
    int dsth = 1080;

    /*File*/
    fin = fopen("news_vga_2.yuv", "rb");
    if(fin == NULL)
    {
        printf("err!can not open input file!%s,%d!\n", __FILE__, __LINE__);
        goto out;
    }
    fout = fopen("out.yuv", "wb");
    if(fout == NULL)
    {
        printf("err!can not open output file!%s,%d!\n", __FILE__, __LINE__);
        goto out;
    }

    /*buffer*/
    len_in = stride * height * 3 / 2;
    phy_in = (unsigned long) omx_malloc_phy(len_in, &vir_in);
    if(phy_in == 0)
    {
        printf("err!omx_malloc_phy fail! %s,%d!\n", __FILE__, __LINE__);
        goto out;
    }

#ifndef VR_TEST
    len_out = ALIGN_16(width / wscale) * ALIGN_16(height / hscale) * 3 / 2;
#else
    len_out = ALIGN_16(dstw) * ALIGN_16(dsth) * 3 / 2;
#endif
    phy_out = (unsigned long) omx_malloc_phy(len_out, &vir_out);
    if(phy_out == 0)
    {
        printf("err!omx_malloc_phy fail! %s,%d!\n", __FILE__, __LINE__);
        goto out;
    }

    /*Open*/
#ifndef IC_TYPE_GL5207
    input.max_srcw = 4096;
    input.max_srch = 2304;
#endif
    input.dstw_align = ALIGN_16PIXELS;
    handle = VceReSize_Open(&input);

    /*Run*/
#ifndef VR_TEST
    param.src_addr = phy_in;
    param.srcstride = stride;
    param.srcheight = height;
    param.cropw = width;
    param.croph = height;
    param.dst_addr = phy_out;
    param.wscale = wscale;
    param.hscale = hscale;
#else
    cmd_param.src_addr = phy_in;
    cmd_param.srcformat = FORMAT_YUV420SP;
    cmd_param.srcstride = stride;
    cmd_param.srcheight = height;
    cmd_param.cropx = cropx;
    cmd_param.cropy = cropy;
    cmd_param.cropw = dw;
    cmd_param.croph = dh;
    cmd_param.dst_addr = phy_out;
    cmd_param.dstformat = FORMAT_YUV420SP;
    cmd_param.bvir_addr = 0;
    cmd_param.dstw = dstw;
    cmd_param.dsth = dsth;
#endif

    for (i = 0; i < frames; i++)
    {
        //if((i%30 == 0))fseek(fin,0,SEEK_SET);
        fread((void *)vir_in, 1, len_in, fin);

        gettimeofday(&stv0, NULL);
#ifndef VR_TEST
        ret = VceReSize_Run(handle, &param);
#else
        ret = VceReSize_Cmd(handle, 0, &cmd_param);
#endif
        gettimeofday(&stv1, NULL);
        cur_ms = (stv1.tv_sec - stv0.tv_sec) * 1000 +  ((double)(stv1.tv_usec - stv0.tv_usec)) / 1000;
        printf("VceReSize_Run us:%fms\n", cur_ms);

        if(ret < 0)
        {
            printf("err!VceReSize_Run fail(%d)!%s,%d!\n", i, __FILE__, __LINE__);
            goto out;
        }
        fwrite((void *)vir_out, 1, len_out, fout);
    }

out:
    /*Close*/
    if(handle)VceReSize_Close(handle);
    if(phy_in)omx_free_phy((void *) (phy_in));
    if(phy_out)omx_free_phy((void *) (phy_out));
    if(fout)fclose(fout);
    if(fin)fclose(fin);
    return 0;
}