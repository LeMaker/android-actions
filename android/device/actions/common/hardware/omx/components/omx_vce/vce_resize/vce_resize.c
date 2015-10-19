#include <string.h>
#include <sys/ioctl.h>
#include <dlfcn.h>   /* For dynamic loading */
#include <fcntl.h>
#include "log.h"
#include "omx_malloc.h"
#include "drv_resize.h"
#include "vce_resize.h"

#define ALIGN_32(x)  (((x) + 0x1f) & (~0x1f))
#define ALIGN_16(x)  (((x) + 0xf) & (~0xf))
#define ALIGN_8(x)    (((x) + 0x7) & (~0x7))

typedef struct VR_Handle_t
{
    drv_input_t drv_in;
    drv_output_t drv_out;
    vcedrv_freq_t vcedrv_freq;
    unsigned long dst_phy_temp;
    unsigned long dst_vir_temp;
    int dstw_align;
} VR_Handle_t;

#ifndef IC_TYPE_GL5207
static int Get_Ds_Index(int scale)
{
    int lv = 0;
    switch(scale)
    {
    case 1:
        lv = 0;
        break;
    case 2:
        lv = 1;
        break;
    case 4:
        lv = 2;
        break;
    case 8:
        lv = 3;
        break;
    default:
        printf("err!can not support(%d)!%s,%d\n", scale, __FILE__, __LINE__);
        lv = -1;
        break;
    }

    return lv;
}
#endif

void *VceReSize_Open(void *input)
{
    int  scale_len, vcedrv_fd;
    unsigned long scale_phy = 0, scale_vir = 0;
    VR_Input_t *vr_input = (VR_Input_t *)input;
    VR_Handle_t *vr_handle;

    /*vr_handle*/
    vr_handle = (VR_Handle_t *)malloc(sizeof(VR_Handle_t));
    if(vr_handle == NULL)
    {
        printf("err!VceReSize_Open failed!%s,%d\n", __FILE__, __LINE__);
        return NULL;
    }
    memset(vr_handle, 0, sizeof(VR_Handle_t));

    /*vcedrv*/
    vcedrv_fd = open("/dev/vce", O_RDWR | O_NONBLOCK);
    if (vcedrv_fd < 0)
    {
        printf("open vcedrv fail!%s,%d\n", __FILE__, __LINE__);
        free(vr_handle);
        return NULL;
    }

    /*scale buffer*/
#ifndef IC_TYPE_GL5207
    scale_len = vr_input->max_srcw * vr_input->max_srch * 3 / 2;
#else
    scale_len = 160 * 1024;
#endif
    scale_phy = (unsigned long) omx_malloc_phy(scale_len, &scale_vir);
    if (scale_phy == 0)
    {
        printf("err!omx_malloc_phy fail! %s,%d!\n", __FILE__, __LINE__);
        close(vcedrv_fd);
        free(vr_handle);
        return NULL;
    }

    /*初始化*/
    vr_handle->drv_in.vcedrv_fd = vcedrv_fd;
    vr_handle->drv_in.scale_phy = scale_phy;
    vr_handle->drv_in.scale_len = scale_len;
    vr_handle->dstw_align = vr_input->dstw_align;
    vr_handle->dst_phy_temp = 0;
    vr_handle->dst_vir_temp = 0;

    return (void *)vr_handle;
}

int  VceReSize_Run(void *handle, void *param)
{
    int ret = 0;
    int wlv, hlv;
    VR_Handle_t *vr_handle = (VR_Handle_t *)handle;
    VR_Parm_t *vr_param = (VR_Parm_t *)param;
    drv_input_t *drv_in;
    drv_output_t *drv_out;

    /*错误判断*/
    //printf("VceReSize_Run!\n");
    if(vr_handle == NULL)
    {
        printf("err!vr_handle %p!%s,%d\n", vr_handle, __FILE__, __LINE__);
        return -1;
    }

#ifdef IC_TYPE_GL5206
    if(vr_param->srcstride % 32 != 0)
    {
        printf("src stride must be 32-aligned after gl5206!\n");
        return -1;
    }
#endif

    drv_in = &vr_handle->drv_in;
    drv_out = &vr_handle->drv_out;

    /*配置:drv_in*/
    drv_in->src_addr = vr_param->src_addr;
    drv_in->srcstride = ALIGN_16(vr_param->srcstride);
    drv_in->srcw = ALIGN_8(vr_param->cropw);
    drv_in->srch = ALIGN_8(vr_param->croph);
    drv_in->srcy = vr_param->src_addr;
    drv_in->srccb = vr_param->src_addr + vr_param->srcstride * vr_param->srcheight;;
    drv_in->srccr = drv_in->srccb;

#ifndef IC_TYPE_GL5207
    drv_in->scale_w = drv_in->srcw;
    drv_in->scale_h = drv_in->srch;
#endif

    drv_in->issemi = 1;
    drv_in->isyuv420 = 1;
    drv_in->enc_mode = 0; /*ENC_PREVIEW*/
    drv_in->b_ds = 1;
#ifndef IC_TYPE_GL5207
    wlv = Get_Ds_Index(vr_param->wscale);
    hlv = Get_Ds_Index(vr_param->hscale);
    if(wlv < 0 || hlv < 0)
        return -1;
    drv_in->i_ds_lv = (wlv << 2) | hlv;
#else
    if(vr_param->wscale < 1 || vr_param->wscale > 2
            || vr_param->hscale < 1 || vr_param->hscale > 2 )
    {
        printf("err!wscale(%d) or hscale(%d) out of range!%s,%d\n",
               vr_param->wscale, vr_param->hscale, __FILE__, __LINE__);
        return -1;
    }
#endif

    /*配置:drv_out*/
    if(vr_handle->dstw_align == ALIGN_16PIXELS)
        drv_out->dst_width = ALIGN_16(vr_param->cropw / vr_param->wscale);
    else
        drv_out->dst_width = ALIGN_32(vr_param->cropw / vr_param->wscale);
    drv_out->dst_height = ALIGN_16(vr_param->croph / vr_param->hscale);
    drv_out->dst_phyaddr = vr_param->dst_addr;

    /*run*/
    drv_resize_setfreq(&vr_handle->vcedrv_freq, drv_in, drv_out);
    ret = drv_resize_run(drv_in, drv_out);

    return ret;
}

int VceReSize_Cmd(void *handle, int cmd, void *param)
{
    int ret = 0;
    int wlv, hlv;
    float wscale, hscale;
    int i, yoffset, uvoffset, dst_len, srcformat, dstformat;
    unsigned char *y_dst, *cb_dst, *cr_dst;
    unsigned char *y_dst_temp, *cb_dst_temp, *cr_dst_temp;
    VR_Handle_t *vr_handle = (VR_Handle_t *)handle;
    VR_Cmd_t *vr_param = (VR_Cmd_t *)param;
    drv_input_t *drv_in = &vr_handle->drv_in;
    drv_output_t *drv_out = &vr_handle->drv_out;

    /*错误判断*/
    //printf("VceReSize_Cmd!\n");
    if((vr_handle == NULL) || (vr_param->src_addr == 0) || (vr_param->dst_addr == 0))
    {
        printf("err!VceReSize_Cmd(%p,%p,%p)!%s,%d\n",
               vr_handle, (void *)vr_param->src_addr, (void *)vr_param->dst_addr, __FILE__, __LINE__);
        return -1;
    }

    if(vr_param->bvir_addr == 0)
    {
        printf("err!bvir_addr(%d) Can not support phy_addr for dst now,need add this feature by coding(fix bugs)!%s,%d\n",
               vr_param->bvir_addr, __FILE__, __LINE__);
        return -1;
    }

    srcformat = vr_param->srcformat;
    dstformat = vr_param->dstformat;
    if((srcformat != dstformat) && ((dstformat != FORMAT_YVU420SP) || (dstformat != FORMAT_YVU420P)))
    {
        printf("err!Can not support format (%d) to (%d) now,need add this feature by coding(fix bugs)!%s,%d\n",
               srcformat, dstformat, __FILE__, __LINE__);
        return -1;
    }

#ifndef IC_TYPE_GL5207
    printf("err! not support IC type before gl5207!\n");
    return -1;
#endif

    switch(cmd)
    {
    case VR_CROP:
    {
        wscale = vr_param->cropw / (float)vr_param->dstw;
        hscale = vr_param->croph / (float)vr_param->dsth;
        //printf("wscale:%f,hscale:%f\n", wscale, hscale);
        if(wscale > 2 || hscale > 2 || wscale < 1 / 8 || hscale < 1 / 8)
        {
            printf("err!wscale(%f) or hscale(%f) out of range!%s,%d\n",
                   wscale, hscale, __FILE__, __LINE__);
            return -1;
        }

        /*配置:drv_in*/
        if((srcformat == FORMAT_YUV420SP) || (srcformat == FORMAT_YVU420SP))
        {
            /*如是yvu420sp输入，drv_in->srcy,drv_in->srccb,
            drv_in->srccr三个值保持按yuv420sp的顺序配置，
            那么输出也必然是yvu420sp*/
            drv_in->issemi = 1;
            drv_in->isyuv420 = 1;
        }
        else
        {
            /*如是yvu420p输入，drv_in->srcy,drv_in->srccb,
            drv_in->srccr三个值保持按yuv420p的顺序配置，
            那么输出也必然是yvu420sp*/
            drv_in->issemi = 0;
            drv_in->isyuv420 = 1;
        }

        drv_in->enc_mode = 0; /*ENC_PREVIEW*/
        drv_in->b_ds = 1;
        yoffset = vr_param->cropy * vr_param->srcstride + vr_param->cropx;
        uvoffset = (vr_param->cropy >> 1) * (vr_param->srcstride >> 1) + (vr_param->cropx >> 1);
#ifndef IC_TYPE_GL5206
        yoffset = yoffset & (~0x7);
        uvoffset = uvoffset & (~0x7);
        if(vr_param->srcstride % 32 != 0)
        {
            printf("src stride must be 32-aligned after gl5206!\n");
            return -1;
        }
#else
        yoffset = yoffset & (~0x1f);
        uvoffset = uvoffset & (~0x1f);
#endif

        drv_in->src_addr = vr_param->src_addr;
        drv_in->srcstride = ALIGN_16(vr_param->srcstride);
        drv_in->srcw = ALIGN_8(vr_param->cropw);
        drv_in->srch = ALIGN_8(vr_param->croph);
        drv_in->srcy = vr_param->src_addr + yoffset;
        if((srcformat == FORMAT_YUV420SP) || (srcformat == FORMAT_YVU420SP))
        {
            drv_in->srccb = vr_param->src_addr + vr_param->srcstride * vr_param->srcheight + uvoffset * 2;
            drv_in->srccr = drv_in->srccb + 1;
        }
        else
        {
            drv_in->srccb = vr_param->src_addr + vr_param->srcstride * vr_param->srcheight + uvoffset;
            drv_in->srccr = vr_param->src_addr + vr_param->srcstride * vr_param->srcheight * 5 / 4 + uvoffset;
        }

        /*配置:drv_out*/
        if(vr_handle->dstw_align == ALIGN_16PIXELS)
            drv_out->dst_width = ALIGN_16(vr_param->dstw);
        else
            drv_out->dst_width = ALIGN_32(vr_param->dstw);
        drv_out->dst_height = ALIGN_16(vr_param->dsth);
        dst_len = drv_out->dst_width * drv_out->dst_height * 3 / 2;
        if(drv_out->dst_width != vr_param->dstw)
        {
            printf("warning!dst_width(%d) != dstw(%d),maybe need fix bug!\n",
                   drv_out->dst_width, vr_param->dstw);
        }

        /*temp buffer*/
        if(vr_param->bvir_addr == 0)
        {
            drv_out->dst_phyaddr = vr_param->dst_addr;
        }
        else
        {
            if (vr_handle->dst_phy_temp == 0)
            {
                vr_handle->dst_phy_temp = (unsigned long) omx_malloc_phy(dst_len, &vr_handle->dst_vir_temp);
                if(vr_handle->dst_phy_temp == 0)
                {
                    vr_handle->dst_vir_temp = 0;
                    printf("err!omx_malloc_phy fail! %s,%d!\n", __FILE__, __LINE__);
                    return -1;
                }
            }
            drv_out->dst_phyaddr = vr_handle->dst_phy_temp;
        }

        /*run*/
        drv_resize_setfreq(&vr_handle->vcedrv_freq, drv_in, drv_out);
        ret = drv_resize_run(drv_in, drv_out);

        /*需要处理好：format & align(dst height)*/
        if(vr_param->bvir_addr != 0)
        {
            /*多一次内存拷贝*/
            if(srcformat == dstformat)
            {
                if((srcformat == FORMAT_YUV420SP) || (srcformat == FORMAT_YVU420SP))
                {
                    /*yuv420sp输入:vce输出是yuv420sp,需软件转为yuv420sp*/
                    /*yvu420sp输入:vce输出是yvu420sp,需软件转为yvu420sp*/
                    if(drv_out->dst_height == vr_param->dsth)
                    {
                        memcpy((unsigned char*)vr_param->dst_addr, (unsigned char*)vr_handle->dst_vir_temp, dst_len);
                    }
                    else
                    {
                        y_dst = (unsigned char*)vr_param->dst_addr;
                        cb_dst = (unsigned char*)(vr_param->dst_addr + drv_out->dst_width * vr_param->dsth);
                        y_dst_temp = (unsigned char*)vr_handle->dst_vir_temp;
                        cb_dst_temp = (unsigned char*)(vr_handle->dst_vir_temp + drv_out->dst_width * drv_out->dst_height);

                        memcpy(y_dst, y_dst_temp, drv_out->dst_width * vr_param->dsth);
                        memcpy(cb_dst, cb_dst_temp, drv_out->dst_width * vr_param->dsth / 2);
                    }
                }
                else
                {
                    /*yuv420p输入:vce输出是yuv420sp,需软件转为yuv420p*/
                    /*yvu420p输入:vce输出是yvu420sp,需软件转为yvu420p*/
                    unsigned int *cb_dst_temp_int, *cb_dst_int, *cr_dst_int;
                    unsigned int t0, t1;
                    int uvlen;

                    y_dst_temp = (unsigned char*)vr_handle->dst_vir_temp;
                    cb_dst_temp = (unsigned char*)(vr_handle->dst_vir_temp + drv_out->dst_width * drv_out->dst_height);
                    cr_dst_temp = cb_dst_temp + 1;
                    y_dst = (unsigned char*)vr_param->dst_addr;
                    cb_dst = (unsigned char*)(vr_param->dst_addr + drv_out->dst_width * vr_param->dsth);
                    cr_dst = cb_dst + drv_out->dst_width * vr_param->dsth / 4;

                    /*y*/
                    memcpy(y_dst, y_dst_temp, drv_out->dst_width * vr_param->dsth);

                    /*cb,cr*/
                    cb_dst_temp_int = (unsigned int *)cb_dst_temp;
                    cb_dst_int = (unsigned int *)cb_dst;
                    cr_dst_int = (unsigned int *)cr_dst;

                    if( ((((unsigned long)(cb_dst_temp_int)) & 3) == 0) &&
                            ((((unsigned long)(cb_dst_int)) & 3) == 0) &&
                            ((((unsigned long)(cr_dst_int)) & 3) == 0) )
                    {
                    		uvlen = drv_out->dst_width * vr_param->dsth / 4 / 4;
                        for(i = 0; i < uvlen; i++)
                        {
                            t0 = *cb_dst_temp_int++;
                            t1 = *cb_dst_temp_int++;
                            /*注意大小端问题！*/
                            *cb_dst_int++ = (t0 & 0xff) | ((t0 & 0xff0000) >> 8) | ((t1 & 0xff) << 16) | ((t1 & 0xff0000) << 8);
                            *cr_dst_int++ = ((t0 & 0xff00) >> 8) | ((t0 & 0xff000000) >> 16) | ((t1 & 0xff00) << 8) | (t1 & 0xff000000) ;
                        }
                    }
                    else
                    {
                    		uvlen = drv_out->dst_width * vr_param->dsth / 4;
                        for(i = 0; i < uvlen; i++)
                        {
                            *cb_dst = *cb_dst_temp;
                            *cr_dst = *cr_dst_temp;
                            cb_dst++;
                            cr_dst++;
                            cb_dst_temp += 2;
                            cr_dst_temp += 2;
                        }
                    }
                }
            }
            else if(dstformat == FORMAT_YVU420SP)
            {
                if((srcformat == FORMAT_YUV420SP) || (srcformat == FORMAT_YUV420P))
                {
                    /*yuv420sp输入:vce输出是yuv420sp,需软件转为yvu420sp*/
                    /*yuv420p输入:vce输出是yuv420sp,需软件转为yvu420sp*/
                    y_dst_temp = (unsigned char*)vr_handle->dst_vir_temp;
                    cb_dst_temp = (unsigned char*)(vr_handle->dst_vir_temp + drv_out->dst_width * drv_out->dst_height);
                    cr_dst_temp = cb_dst_temp + 1;
                    y_dst = (unsigned char*)vr_param->dst_addr;
                    cr_dst = (unsigned char*)(vr_param->dst_addr + drv_out->dst_width * vr_param->dsth);
                    cb_dst = cr_dst + 1;

                    /*y*/
                    memcpy(y_dst, y_dst_temp, drv_out->dst_width * vr_param->dsth);

                    /*cb,cr*/
                    /*可以参考srcformat == dstformat分支进行优化，
                    由于只有176x144才会跑到srcformat ！= dstformat，性能影响不大*/
                    for(i = 0; i < drv_out->dst_width * vr_param->dsth / 4; i++)
                    {
                        *cb_dst = *cb_dst_temp;
                        *cr_dst = *cr_dst_temp;
                        cb_dst += 2;
                        cr_dst += 2;
                        cb_dst_temp += 2;
                        cr_dst_temp += 2;
                    }
                }
                else/*FORMAT_YVU420P*/
                {
                    /*yvu420p输入:vce输出是yvu420sp,需软件转为yvu420sp*/
                    if(drv_out->dst_height == vr_param->dsth)
                    {
                        memcpy((unsigned char*)vr_param->dst_addr,(unsigned char*)vr_handle->dst_vir_temp, dst_len);
                    }
                    else
                    {
                        y_dst = (unsigned char*)vr_param->dst_addr;
                        cr_dst = (unsigned char*)(vr_param->dst_addr + drv_out->dst_width * vr_param->dsth);
                        y_dst_temp = (unsigned char*)vr_handle->dst_vir_temp;
                        cr_dst_temp = (unsigned char*)(vr_handle->dst_vir_temp + drv_out->dst_width * drv_out->dst_height);

                        memcpy(y_dst, y_dst_temp, drv_out->dst_width * vr_param->dsth);
                        memcpy(cr_dst, cr_dst_temp, drv_out->dst_width * vr_param->dsth / 2);
                    }
                }
            }
            else if(dstformat == FORMAT_YVU420P)
            {
                if((srcformat == FORMAT_YUV420SP) || (srcformat == FORMAT_YUV420P))
                {
                    /*yuv420sp输入:vce输出是yuv420sp,需软件转为yvu420p*/
                    /*yuv420p输入:vce输出是yuv420sp,需软件转为yvu420p*/
                    y_dst_temp = (unsigned char*)vr_handle->dst_vir_temp;
                    cb_dst_temp = (unsigned char*)(vr_handle->dst_vir_temp + drv_out->dst_width * drv_out->dst_height);
                    cr_dst_temp = cb_dst_temp + 1;
                    y_dst = (unsigned char*)vr_param->dst_addr;
                    cr_dst = (unsigned char*)(vr_param->dst_addr + drv_out->dst_width * vr_param->dsth);
                    cb_dst = cr_dst + drv_out->dst_width * vr_param->dsth / 4;

                    /*y*/
                    memcpy(y_dst, y_dst_temp, drv_out->dst_width * vr_param->dsth);

                    /*cb,cr*/
                    /*可以参考srcformat == dstformat分支进行优化，
                    由于只有176x144才会跑到srcformat ！= dstformat，性能影响不大*/
                    for(i = 0; i < drv_out->dst_width * vr_param->dsth / 4; i++)
                    {
                        *cb_dst = *cb_dst_temp;
                        *cr_dst = *cr_dst_temp;
                        cb_dst ++;
                        cr_dst ++;
                        cb_dst_temp += 2;
                        cr_dst_temp += 2;
                    }
                }
                else/*FORMAT_YVU420SP*/
                {
                    /*yvu420sp输入:vce输出是yvu420sp,需软件转为yvu420p*/
                    y_dst_temp = (unsigned char*)vr_handle->dst_vir_temp;
                    cr_dst_temp = (unsigned char*)(vr_handle->dst_vir_temp + drv_out->dst_width * drv_out->dst_height);
                    cb_dst_temp = cr_dst_temp + 1;
                    y_dst = (unsigned char*)vr_param->dst_addr;
                    cr_dst = (unsigned char*)(vr_param->dst_addr + drv_out->dst_width * vr_param->dsth);
                    cb_dst = cr_dst + drv_out->dst_width * vr_param->dsth / 4;

                    /*y*/
                    memcpy(y_dst, y_dst_temp, drv_out->dst_width * vr_param->dsth);

                    /*cb,cr*/
                    /*可以参考srcformat == dstformat分支进行优化，
                    由于只有176x144才会跑到srcformat ！= dstformat，性能影响不大*/
                    for(i = 0; i < drv_out->dst_width * vr_param->dsth / 4; i++)
                    {
                        *cb_dst = *cb_dst_temp;
                        *cr_dst = *cr_dst_temp;
                        cb_dst ++;
                        cr_dst ++;
                        cb_dst_temp += 2;
                        cr_dst_temp += 2;
                    }
                }
            }
            else
            {
                //ToDo:fix for dstformat is other format
            }
        }
        else
        {
            //ToDo:fix for bvir_addr == 0
        }
    }
    break;

    default:
        printf("err! VR_cmd error!%s,%d\n", __FILE__, __LINE__);
    }

    return ret;
}

void VceReSize_Close(void *handle)
{
    VR_Handle_t *vr_handle = (VR_Handle_t *)handle;

    printf("VceReSize_Close!\n");
    if(vr_handle)
    {
        if (vr_handle->drv_in.scale_phy != 0)
        {
            omx_free_phy((void *) (vr_handle->drv_in.scale_phy));
            vr_handle->drv_in.scale_phy = 0;
        }

        if (vr_handle->dst_phy_temp != 0)
        {
            omx_free_phy((void *) (vr_handle->dst_phy_temp));
            vr_handle->dst_phy_temp = 0;
            vr_handle->dst_vir_temp = 0;
        }
        close(vr_handle->drv_in.vcedrv_fd);
        free(vr_handle);
    }
}