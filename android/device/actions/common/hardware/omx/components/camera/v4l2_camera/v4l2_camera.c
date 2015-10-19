#ifdef __cplusplus
extern "C" {
#endif


#include "v4l2_camera.h"
#include "ACT_OMX_Common_V1_2__V1_1.h"
#include "ACT_OMX_IVCommon.h"
#include "OMX_IVCommon.h"
#include "OMX_Image.h"
#include <linux/videodev2.h>
#include "ext_mem.h"
#include "isp_ctl.h"
#include<stdio.h>
#include<sys/time.h>
#include<unistd.h>
#include "perf.h"
#include "log.h"


#ifndef V4L2_CID_EXPOSURE_COMP
#define V4L2_CID_EXPOSURE_COMP      (V4L2_CID_CAMERA_CLASS_BASE+20)
#endif


//static int manual_wb_tempture[10] = {0,0,5500,6000,7000,3300,4000,2700,6000,5000};
//static int manual_color_effect[10] = {0,17,16,3,2,4,18,20,14,19};
static int manual_wb_tempture[10] = { -1, -1, 2, 3, 6, -1, 1, 0, -1, 5};
static int manual_color_effect[11] = {0, -1, 4, 3, 5, -1, -1, -1, -1, -1, -1};
/**
1,700 K Match flame
1,850 K Candle flame, sunset/sunrise
2,700每3,300 K Incandescent lamps
3,000 K Soft White compact fluorescent lamps
3,200 K Studio lamps, photofloods, etc.
3,350 K Studio "CP" light
4,100每4,150 K Moonlight,[2] xenon arc lamp
5,000 K Horizon daylight
5,000 K tubular fluorescent lamps or Cool White/Daylight compact fluorescent lamps (CFL)
5,500每6,000 K Vertical daylight, electronic flash
6,500 K Daylight, overcast
6,500每9,300 K LCD or CRT screen
*/
static int nscene_exp[] = {0, 1, 2, 3, 4, 6, 7, 10, 11, 13, 14, 15};
static int manual_color_effect_ext[] = {0, 1, 2, 6, 7, 8, 9};

static int find_index(int *idxPtr, int num, int idx_target)
{
    int i = 0;

    //  int idx = 0;
    for(i = 0; i < num; i++)
    {
        if(*(idxPtr + i) == idx_target)
        {
            //idx = i;
            return i;
        }
    }

    return -1;
}

static int ev_adt(OMX_CAMERATYPE *camera_module, int ev)
{
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    int ctl_id = 0;
    int ctl_val = 0;
    int exp_time = 0;
    int exp_gain = 0;

    StandModule->mod_get_ctl(StandModule, V4L2_CID_EXPOSURE, &exp_time);
    StandModule->mod_get_ctl(StandModule, V4L2_CID_GAIN, &exp_gain);
    V4L2DBUG(V4L2DBUG_VERB, "last expo info: %d, %d\n", exp_time, exp_gain);

    StandModule->mod_set_ctl(StandModule, V4L2_CID_EXPOSURE_COMP, ev); //
    V4L2DBUG(V4L2DBUG_VERB, "set ev: %d\n", ev);

    return (exp_time * exp_gain);
}

static int ev_adt_tg(OMX_CAMERATYPE *camera_module, int ev, int *exp_time_lc, int *exp_gain_lc, int flag)
{
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    int ctl_id = 0;
    int ctl_val = 0;
    int exp_time = 0;
    int exp_gain = 0;
    int exp_exp = 0;
    int exp_gain_next = 0, exp_time_next = 0;
    int MAX_EXP = camera_module_type->max_exp;
    int Min_EXP = camera_module_type->min_exp;
    int MAX_GAIN = camera_module_type->max_gain;
    int Min_GAIN = camera_module_type->min_gain;

    V4L2DBUG(V4L2DBUG_ERR, "Exposure range: %d-%d; %d-%d", MAX_EXP, Min_EXP, MAX_GAIN, Min_GAIN);

    if(flag == 0)
    {
        StandModule->mod_get_ctl(StandModule, V4L2_CID_EXPOSURE, &exp_time);
        StandModule->mod_get_ctl(StandModule, V4L2_CID_GAIN, &exp_gain);
        V4L2DBUG(V4L2DBUG_ERR, "last expo info: %d, %d", exp_time, exp_gain);
        *exp_time_lc = exp_time;
        *exp_gain_lc = exp_gain;
        exp_exp = (exp_time * exp_gain);
    }
    else
    {
        StandModule->mod_get_ctl(StandModule, V4L2_CID_EXPOSURE, &exp_time);
        StandModule->mod_get_ctl(StandModule, V4L2_CID_GAIN, &exp_gain);
        V4L2DBUG(V4L2DBUG_ERR, "last expo info: %d, %d", exp_time, exp_gain);
        exp_exp = (exp_time * exp_gain);
        exp_time = *exp_time_lc;
        exp_gain = *exp_gain_lc;
    }

#if 0
    if(ev < 0)
    {
        if(exp_gain  <= 350)
        {
            exp_time_next = exp_time / 4;
            V4L2DBUG(V4L2DBUG_ERR, "dark: set time %d, %d,%d", exp_time_next, exp_gain, __LINE__);
            StandModule->mod_set_ctl(StandModule, V4L2_CID_EXPOSURE, exp_time_next);
            StandModule->mod_set_ctl(StandModule, V4L2_CID_GAIN, exp_gain);
        }
        else
        {
            exp_gain_next = exp_gain / 4;
            V4L2DBUG(V4L2DBUG_ERR, "dark: set gain %d, %d, %d", exp_time, exp_gain_next, __LINE__);
            StandModule->mod_set_ctl(StandModule, V4L2_CID_EXPOSURE, exp_time);
            StandModule->mod_set_ctl(StandModule, V4L2_CID_GAIN, exp_gain_next);
        }
    }
    else if(ev > 0)
    {
        if(exp_gain <= 350)
        {
            exp_time_next = exp_time * 2.5;
            V4L2DBUG(V4L2DBUG_ERR, "bright: set time %d, %d, %d", exp_time_next, exp_gain, __LINE__);
            StandModule->mod_set_ctl(StandModule, V4L2_CID_EXPOSURE, exp_time_next);
            StandModule->mod_set_ctl(StandModule, V4L2_CID_GAIN, exp_gain);
        }
        else
        {
            exp_gain_next = exp_gain * 2.5;
            V4L2DBUG(V4L2DBUG_ERR, "bright: set gain %d, %d, %d", exp_time, exp_gain_next, __LINE__);
            StandModule->mod_set_ctl(StandModule, V4L2_CID_EXPOSURE, exp_time);
            StandModule->mod_set_ctl(StandModule, V4L2_CID_GAIN, exp_gain_next);
        }
    }
#else

    if(-4 == ev)
    {
        float ratio;
        ratio = (float)exp_gain / Min_GAIN;

        if(ratio >= 4.0f)
        {
            exp_gain /= 4;
        }
        else
        {
            exp_gain = Min_GAIN;
            ratio = 4.0f / ratio;
            exp_time /= ratio;

            if(exp_time < Min_EXP) { exp_time = Min_EXP; }
        }
    }
    else if(-2 == ev)
    {
        float ratio;
        ratio = (float)exp_gain / Min_GAIN;

        if(ratio >= 2.0f)
        {
            exp_gain /= 2;
        }
        else
        {
            exp_gain = Min_GAIN;
            ratio = 2.0f / ratio;
            exp_time /= ratio;

            if(exp_time < Min_EXP) { exp_time = Min_EXP; }
        }
    }
    else if(4 == ev)
    {
        float ratio;
        ratio = MAX_EXP / (float)exp_time;

        if(ratio >= 4.0f)
        {
            exp_time *= 4;
        }
        else
        {
            int HALF_GAIN = MAX_GAIN / 2;
            exp_time = MAX_EXP;
            ratio = 4.0f / ratio;

            if(exp_gain < HALF_GAIN)
            {
                exp_gain *= ratio;

                if(exp_gain > HALF_GAIN) { exp_gain = HALF_GAIN; }
            }
        }
    }
    else if(2 == ev)
    {
        float ratio;
        ratio = MAX_EXP / (float)exp_time;

        if(ratio >= 2.0f)
        {
            exp_time *= 2;
        }
        else
        {
            int HALF_GAIN = MAX_GAIN / 2;
            exp_time = MAX_EXP;
            ratio = 2.0f / ratio;

            if(exp_gain < HALF_GAIN)
            {
                exp_gain *= ratio;

                if(exp_gain > HALF_GAIN) { exp_gain = HALF_GAIN; }
            }
        }
    }
    else if(0 == ev)
    {
        ;
    }

    V4L2DBUG(V4L2DBUG_ERR, "set expo: %d, %d", exp_time, exp_gain);
    StandModule->mod_set_ctl(StandModule, V4L2_CID_EXPOSURE, exp_time);
    StandModule->mod_set_ctl(StandModule, V4L2_CID_GAIN, exp_gain);
#endif

    return (exp_exp);
}

void set_hdr_bracket(OMX_CAMERATYPE *camera_module, int cur_ev)
{
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    FILE *fd;

#if 0
    fd = fopen("/data/hdrev.txt", "r");
    if(NULL != fd)
    {
        int ev_1, ev_2;
        char ctmp;
        fscanf(fd, "%d%c%d", &ev_1, &ctmp, &ev_2);
        if(ev_1 > 2) { ev_1 = 2; }
        if(ev_1 < -2) { ev_1 = -2; }
        if(ev_2 > 2) { ev_2 = 2; }
        if(ev_2 < -2) { ev_2 = -2; }

        camera_module_type->hdr_ev_param[0] = ev_1 * 2;
        camera_module_type->hdr_ev_param[2] = ev_2 * 2;
        V4L2DBUG(V4L2DBUG_ERR, "CRF ev: %d, %d", camera_module_type->hdr_ev_param[0],
                 camera_module_type->hdr_ev_param[2]);

        fclose(fd);
        return;
    }
#endif

    switch(cur_ev)
    {
    case  0:
        camera_module_type->hdr_ev_param[0] =  0;
        camera_module_type->hdr_ev_param[1] =  4;
        camera_module_type->hdr_ev_param[2] = -4;
        break;

    case -2:
        camera_module_type->hdr_ev_param[0] = -2;
        camera_module_type->hdr_ev_param[1] =  2;
        camera_module_type->hdr_ev_param[2] = -4;
        break;

    case  2:
        camera_module_type->hdr_ev_param[0] =  2;
        camera_module_type->hdr_ev_param[1] =  4;
        camera_module_type->hdr_ev_param[2] = -2;
        break;

    case -4:
        camera_module_type->hdr_ev_param[0] = -4;
        camera_module_type->hdr_ev_param[1] = -2;
        camera_module_type->hdr_ev_param[2] =  0;
        break;

    case  4:
        camera_module_type->hdr_ev_param[0] =  4;
        camera_module_type->hdr_ev_param[1] =  2;
        camera_module_type->hdr_ev_param[2] =  0;
        break;

    default:
        V4L2DBUG(V4L2DBUG_ERR, "not support the current ev in %s\n", __func__);
        break;
    }

    V4L2DBUG(V4L2DBUG_PARAM, "%s: %d, %d, %d\n", __func__, 
        camera_module_type->hdr_ev_param[0], camera_module_type->hdr_ev_param[1], camera_module_type->hdr_ev_param[2]);

    return;
}

static int qtmpBuf(camera_direct_base_PrivateType *camera_module_type)
{
    int ret = 0;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int i = 0, j = 0;
    int module_num = 0;
    int64_t tmp = 0;
    int start_idx = 0;
    int buflen = camera_module_type->mPreviewInfo.nh * camera_module_type->mPreviewInfo.nw * 3 / 2 + 4096;
    tmp = 0x7FFFFFFFFFFFFFFFLL;

    if(buflen < camera_module_type->mPreviewInfo.nPreVSize)
    { buflen = camera_module_type->mPreviewInfo.nPreVSize; }

    for(i = 0; i < camera_module_type->mPreviewInfo.nbufNum; i++)
    {
        if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_CLITOMOD)
        {
            if(tmp > camera_module_type->mPreviewInfo.nTimeStamp[i])
            {
                tmp = camera_module_type->mPreviewInfo.nTimeStamp[i];
                start_idx = i;
            }

            module_num++;
        }
    }

    for(j = 0; j < module_num; j++)
    {
        for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
        {
            if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_CLITOMOD && camera_module_type->mPreviewInfo.nTimeStamp[i] == tmp)
            {
                V4L2DBUG(V4L2DBUG_ERR, "qtmpBuf a %x %x %lld,%lld\n", i, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.nTimeStamp[i], camera_module_type->nPreviewSeq);
                //ret |= StandModule->mod_querybuf(StandModule,i,buflen,camera_module_type->mPreviewInfo.pBufPhy[i],camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
                ret |= StandModule->mod_qbuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
                camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_MODULE;
                i = camera_module_type->mPreviewInfo.nbufNum + 1;
            }
        }

        tmp++;
    }

    return ret;
}


static int qtmpCBuf(camera_direct_base_PrivateType *camera_module_type)
{
    int ret = 0;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int i = 0, j = 0;
    int module_num = 0;
    int64_t tmp = 0;
    int start_idx = 0;
    int buflen = camera_module_type->mCaptureInfo.nh * camera_module_type->mCaptureInfo.nw * 3 / 2 + 4096;
    tmp = 0x7FFFFFFFFFFFFFFFLL;

    if(buflen < camera_module_type->mCaptureInfo.nPreVSize)
    { buflen = camera_module_type->mCaptureInfo.nPreVSize; }

    for(i = 0; i < camera_module_type->mCaptureInfo.nbufNum; i++)
    {
        if(camera_module_type->mCaptureInfo.pBufOwner[i] == OWN_BY_CLITOMOD)
        {
            if(tmp > camera_module_type->mCaptureInfo.nTimeStamp[i])
            {
                tmp = camera_module_type->mCaptureInfo.nTimeStamp[i];
                start_idx = i;
            }

            module_num++;
        }
    }

    for(j = 0; j < module_num; j++)
    {
        for(i = 0 ; i < camera_module_type->mCaptureInfo.nbufNum; i++)
        {
            if(camera_module_type->mCaptureInfo.pBufOwner[i] == OWN_BY_CLITOMOD && camera_module_type->mCaptureInfo.nTimeStamp[i] == tmp)
            {
                V4L2DBUG(V4L2DBUG_VERB, "qtmpCBuf a %x %x %lld,%d\n", i, camera_module_type->mCaptureInfo.pBufPhy[i], camera_module_type->mCaptureInfo.nTimeStamp[i], camera_module_type->nCaptureSeq);
                //ret |= StandModule->mod_querybuf(StandModule,i,buflen,camera_module_type->mCaptureInfo.pBufPhy[i],camera_module_type->mCaptureInfo.pBufPhy_Stat[i]);
                ret |= StandModule->mod_qbuf(StandModule, i, buflen, camera_module_type->mCaptureInfo.pBufPhy[i], camera_module_type->mCaptureInfo.pBufPhy_Stat[i]);
                camera_module_type->mCaptureInfo.pBufOwner[i] = OWN_BY_MODULE;
                i = camera_module_type->mCaptureInfo.nbufNum + 1;
            }
        }

        tmp++;
    }

    return ret;
}

static int restore_direct_preview_bufq(camera_direct_base_PrivateType *camera_module_type)
{
    int ret = 0;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ctl_id = 0;
    int ctl_val = 0;
    int i = 0, j = 0;
    int module_num = 0;
    int64_t tmp = 0;
    int start_idx = 0;
    int buflen = camera_module_type->mPreviewInfo.nh * camera_module_type->mPreviewInfo.nw * 3 / 2 + 4096;

    //int nStartNum = 0;
    if(buflen < camera_module_type->mPreviewInfo.nPreVSize)
    { buflen = camera_module_type->mPreviewInfo.nPreVSize; }

    //ret = StandModule->mod_set_res(StandModule,camera_module_type->preview_width,camera_module_type->preview_height,camera_module_type->preview_fmt);
    //ret |= StandModule->mod_set_crop(StandModule,0,0,camera_module_type->preview_width,camera_module_type->preview_height);
    ret |= StandModule->mod_set_framerate(StandModule, camera_module_type->preview_fps);
    ret |= StandModule->mod_requestbuf(StandModule, (unsigned int *)&camera_module_type->mPreviewInfo.nbufNum);
#if 0

    for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
    {
        if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_MODULE)
        {
            V4L2DBUG(V4L2DBUG_ERR, "reqbuf a %x %x\n", i, camera_module_type->mPreviewInfo.pBufPhy[i]);
            ret |= StandModule->mod_querybuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
        }
    }

    for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
    {
        if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_CLIENT)
        {
            V4L2DBUG(V4L2DBUG_ERR, "reqbuf b %x %x\n", i, camera_module_type->mPreviewInfo.pBufPhy[i]);
            ret |= StandModule->mod_querybuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
            camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_MODULE;
        }
    }

#else
    /*for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
    {
        if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_CLIENT)
        {
            nStartNum = i;
        }
    }*/
#if 0

    if((camera_module_type->nIndex + 1) == camera_module_type->mPreviewInfo.nbufNum)
    { camera_module_type->nIndex = 0; }
    else
    { camera_module_type->nIndex += 1; }

    for(i = camera_module_type->nIndex ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
    {
        if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_MODULE)
        {
            V4L2DBUG(V4L2DBUG_VERB, "reqbuf a %x %x\n", i, camera_module_type->mPreviewInfo.pBufPhy[i]);
        }
        else
        { camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_MODULE; }

        ret |= StandModule->mod_querybuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
    }

    for(i = 0 ; i < camera_module_type->nIndex; i++)
    {
        if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_MODULE)
        {
            V4L2DBUG(V4L2DBUG_VERB, "reqbuf a %x %x\n", i, camera_module_type->mPreviewInfo.pBufPhy[i]);
        }
        else
        { camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_MODULE; }

        ret |= StandModule->mod_querybuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
    }

#endif
    tmp = 0x7FFFFFFFFFFFFFFFLL;

    for(i = 0; i < camera_module_type->mPreviewInfo.nbufNum; i++)
    {
        if(camera_module_type->mPreviewInfo.pBufOwner[i] != OWN_BY_CLIENT)
        {
            if(tmp > camera_module_type->mPreviewInfo.nTimeStamp[i])
            {
                tmp = camera_module_type->mPreviewInfo.nTimeStamp[i];
                start_idx = i;
            }

            module_num++;
        }
    }

    for(j = 0; j < module_num; j++)
    {
        for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
        {
            if(camera_module_type->mPreviewInfo.pBufOwner[i] != OWN_BY_CLIENT && 
                camera_module_type->mPreviewInfo.nTimeStamp[i] == tmp)
            {
                V4L2DBUG(V4L2DBUG_ERR, "reqbuf 196 a %x %x\n", i, camera_module_type->mPreviewInfo.pBufPhy[i]);
                ret |= StandModule->mod_querybuf(StandModule, i, buflen, 
                    camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
                ret |= StandModule->mod_qbuf(StandModule, i, buflen, 
                    camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
                camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_MODULE;
                i = camera_module_type->mPreviewInfo.nbufNum + 1;
            }
        }

        tmp++;
    }
#endif

    if(camera_module_type->awb_type)
    {
        V4L2DBUG(V4L2DBUG_ERR, "AWB Restore \n");

        if(camera_module_type->awb_type == OMX_WhiteBalControlAuto)
        {
            ctl_id = V4L2_CID_AUTO_WHITE_BALANCE;
            ctl_val = 1;
        }
        else if(camera_module_type->awb_type == OMX_WhiteBalControlOff)
        {
            ctl_id = V4L2_CID_AUTO_WHITE_BALANCE;
            ctl_val = 0;
        }
        else
        {
            if(camera_module_type->awb_type == OMX_WhiteBalControlAuto)
            {
                ctl_id = V4L2_CID_AUTO_WHITE_BALANCE;
                ctl_val = 0;
                ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
            }

            ctl_id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
            ctl_val = manual_wb_tempture[camera_module_type->awb_type];
        }

        ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
    }

    if(camera_module_type->ae_type)
    {
        V4L2DBUG(V4L2DBUG_ERR, "AE Restore \n");

        if(camera_module_type->ae_type == OMX_ExposureControlAuto)
        {
            ctl_id = V4L2_CID_EXPOSURE_AUTO;
            ctl_val = V4L2_EXPOSURE_AUTO;
            ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        }
        else if(camera_module_type->ae_type == OMX_ExposureControlOff)
        {
            ctl_id = V4L2_CID_EXPOSURE_AUTO;
            ctl_val = V4L2_EXPOSURE_MANUAL;
            ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        }
        else
        {
            if(camera_module_type->ae_type == OMX_ExposureControlAuto)
            {
                ctl_id = V4L2_CID_EXPOSURE_AUTO;
                ctl_val = V4L2_EXPOSURE_MANUAL;
                ret = StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
            }

            if(camera_module_type->ae_type == OMX_ExposureControlNight)
            {
                ctl_id = V4L2_CID_SCENE_EXPOSURE;
                ctl_val = V4L2_SCENE_MODE_NIGHT;
                ret = StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
            }

            if(camera_module_type->ae_type == OMX_ExposureControlSports)
            {
                ctl_id = V4L2_CID_SCENE_EXPOSURE;
                ctl_val = V4L2_SCENE_MODE_SPORTS;
                ret = StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
            }

            if(camera_module_type->ae_type == OMX_ExposureControlBackLight)
            {
                //Fix here
                //ret |= StandModule->mod_set_ctl(StandModule,ctl_id,ctl_val);
            }
        }
    }

    //if(camera_module_type->cmp_ev)
    {
        V4L2DBUG(V4L2DBUG_PARAM, "ev Restore: %d\n", camera_module_type->cmp_ev);
        ctl_id = V4L2_CID_EXPOSURE_COMP;
        ctl_val = camera_module_type->cmp_ev;
        ret |= StandModule->mod_set_ctl(StandModule,ctl_id,ctl_val);
    }

    if(camera_module_type->bright_level)
    {
        V4L2DBUG(V4L2DBUG_ERR, "brt Restore \n");
        ctl_id = V4L2_CID_BRIGHTNESS;
        ctl_val = camera_module_type->bright_level * 12 / 100;
        ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
    }

    if(camera_module_type->flicker_type)
    {
        V4L2DBUG(V4L2DBUG_ERR, "flic Restore \n");
        ctl_id = V4L2_CID_POWER_LINE_FREQUENCY;

        if(camera_module_type->flicker_type == OMX_FlickerRejectionOff)
        {
            ctl_val = camera_module_type->flicker_type;
            ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        }
        else if(camera_module_type->flicker_type == OMX_FlickerRejection50)
        {
            ctl_val = camera_module_type->flicker_type;
            ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        }
        else if(camera_module_type->flicker_type == OMX_FlickerRejection60)
        {
            ctl_val = camera_module_type->flicker_type;
            ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        }

    }

    if(camera_module_type->color_fixtype)
    {
        V4L2DBUG(V4L2DBUG_ERR, "clor Restore \n");
        ctl_id = V4L2_CID_COLORFX;
        ctl_val = manual_color_effect[camera_module_type->color_fixtype];
        ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
    }

    return ret;
}

static int open_device(void *handle, int id, int mode_type, int uvcmode)
{
    OMX_CAMERATYPE *camera_module_type = (OMX_CAMERATYPE *)handle;
    int ret = 0;
    int flag = 0;
    int step = 0;
    int dft_v = 0;
    int ret_qry = 0;
    camera_direct_base_PrivateType *camera_direct = (camera_direct_base_PrivateType *)camera_module_type->pCameraPrivate;
    CAMERA_MODULETYPE *cam_module = camera_direct->StandModule;
    pthread_mutex_lock(&camera_direct->camera_mutex);
    ret = cam_module->mod_open(cam_module, id, uvcmode);///

    camera_direct->min_gain = 16 * 16;
    camera_direct->max_gain = 128 * 16;
    camera_direct->min_exp = 32;
    camera_direct->max_exp = 32;

    ret_qry = cam_module->mod_queryctl(cam_module, 0, &camera_direct->min_gain, &camera_direct->max_gain, &dft_v, &step, &flag);

    if(ret_qry == -1)
    {
        V4L2DBUG(V4L2DBUG_ERR, "mod_queryctl gain err\n");
    }

    ret_qry = cam_module->mod_queryctl(cam_module, 1, &camera_direct->min_exp, &camera_direct->max_exp, &dft_v, &step, &flag);

    if(ret_qry == -1)
    {
        V4L2DBUG(V4L2DBUG_ERR, "mod_queryctl exposure err\n");
    }

    V4L2DBUG(V4L2DBUG_ERR, "Got range of exposure: %d-%d; %d-%d", camera_direct->max_exp, camera_direct->min_exp, camera_direct->max_gain, camera_direct->min_gain);

    //  camera_direct->mmodle_type = mode_type;
    pthread_mutex_unlock(&camera_direct->camera_mutex);
    return ret;
}

static int close_device(void *handle)
{
    OMX_CAMERATYPE *camera_module_type = (OMX_CAMERATYPE *)handle;
    int ret = 0;
    camera_direct_base_PrivateType *camera_direct = (camera_direct_base_PrivateType *)camera_module_type->pCameraPrivate;
    CAMERA_MODULETYPE *cam_module = camera_direct->StandModule;
    pthread_mutex_lock(&camera_direct->camera_mutex);

    if(camera_direct->ipp_handle)
    {
        V4L2DBUG(V4L2DBUG_ERR, "ipp drect close\n");
        cam_module->mod_close(cam_module);
    }

    pthread_mutex_unlock(&camera_direct->camera_mutex);
    return ret;
}

static int camera_s_fmt(void *handle, int w, int h, int fmt, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;

    V4L2DBUG(V4L2DBUG_VERB, "%dx%d, %d, %d, %d, %d", w, h, work_mode, camera_module_type->eprev_status, 
                camera_module_type->ecap_status, camera_module_type->bStreaming);
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    if(work_mode == OMX_PREVIEW_MODE)
    {
        ret |= StandModule->mod_set_mode(StandModule, OMX_PREVIEW_MODE);

        if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
        {
            if((w != camera_module_type->preview_width) || \
                    (h != camera_module_type->preview_height))
            {
                camera_module_type->preview_width = w;
                camera_module_type->preview_height = h;
                camera_module_type->preview_fmt = fmt;

                if(camera_module_type->bStreaming == 1)
                {
                    ret |= StandModule->mod_streamoff(StandModule);
                    ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
                    ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
                    ret |= restore_direct_preview_bufq(camera_module_type);
                    ret |= StandModule->mod_streamon(StandModule);
                }
                else
                {
                    ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
                    ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
                }
            }
        }
        else
        {
            camera_module_type->preview_width = w;
            camera_module_type->preview_height = h;
            camera_module_type->preview_fmt = fmt;
            ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
            ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
        }

        camera_module_type->mPreviewInfo.nw = w;
        camera_module_type->mPreviewInfo.nh = h;
    }
    else if(work_mode == OMX_CAPTURE_MODE)
    {
        ret |= StandModule->mod_set_mode(StandModule, OMX_CAPTURE_MODE);

        if(camera_module_type->ecap_status == CAM_CAPTURE_RUNNING)
        {
            if(camera_module_type->bStreaming == 1)
            {
                ret |= StandModule->mod_streamoff(StandModule);
                ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
                ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
                ret |= restore_direct_preview_bufq(camera_module_type);
                ret |= StandModule->mod_streamon(StandModule);
            }
            else
            {
                ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
                ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
            }
        }
        else if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
        {
            StandModule->mod_streamoff(StandModule);
            camera_module_type->eprev_status == CAM_PREVIEW_READY;
            ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
            ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
        }
        else if(camera_module_type->eprev_status == CAM_PREVIEW_READY)
        {
            StandModule->mod_streamoff(StandModule);
            //camera_module_type->eprev_status == CAM_PREVIEW_PAUSE;
            ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
            ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
        }
        else
        {
            ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
            ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
        }

        camera_module_type->capture_width = w;
        camera_module_type->capture_height = h;
        camera_module_type->capture_fmt = fmt;
        camera_module_type->mCaptureInfo.nw = w;
        camera_module_type->mCaptureInfo.nh = h;
        //          camera_module_type->max_exp = h + 16;

    }
    else if(work_mode == OMX_CAPTURE_PREP)
    {
        camera_module_type->capture_width = w;
        camera_module_type->capture_height = h;
        camera_module_type->capture_fmt = fmt;
        camera_module_type->mCaptureInfo.nw = w;
        camera_module_type->mCaptureInfo.nh = h;
        //          camera_module_type->max_exp = h + 16;
    }


    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    return ret;
}

static int camera_g_fmt(void *handle, int *w, int *h, int *fmt, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    if(work_mode == OMX_PREVIEW_MODE)
    {
        if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
        {
            ret = StandModule->mod_get_res(StandModule, w, h, fmt);
        }
        else
        {
            *w = camera_module_type->preview_width;
            *h = camera_module_type->preview_height;
            *fmt = camera_module_type->preview_fmt;
        }
    }
    else
    {
        *w = camera_module_type->capture_width;
        *h = camera_module_type->capture_height;
        *fmt = camera_module_type->capture_fmt;
    }

    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    return ret;
}

static int camera_set_crop(void *handle, int x, int y, int w, int h, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    if(work_mode == OMX_PREVIEW_MODE)
    {
        //ipp_videoinfo_t videoinfo;
        //ipp_clt_t ipp_ctl;
        //StandModule->mod_set_crop(StandModule,x,y,width,height);
        if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
        {
            camera_module_type->preview_width = w;
            camera_module_type->preview_height = h;
            //camera_module_type->preview_fmt = fmt;

        }

        camera_module_type->mPreviewInfo.nw = w;
        camera_module_type->mPreviewInfo.nh = h;
    }
    else
    {
        camera_module_type->capture_width = w;
        camera_module_type->capture_height = h;
        camera_module_type->mCaptureInfo.nw = w;
        camera_module_type->mCaptureInfo.nh = h;
        //camera_module_type->capture_fmt = fmt;
    }

    ret = StandModule->mod_set_crop(StandModule, x, y, w, h);
    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    return ret;
}

static int camera_get_crop(void *handle, int *x, int *y, int *w, int *h, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    pthread_mutex_lock(&camera_module_type->camera_mutex);
    ret = StandModule->mod_get_crop(StandModule, x, y, w, h);
    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    return ret;
}

static int camera_setfps(void *handle, int framerate_Q16, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;

    V4L2DBUG(V4L2DBUG_VERB, "set fps %d\n", framerate_Q16);
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);
    if(work_mode == OMX_PREVIEW_MODE)
    {
        camera_module_type->mPreviewInfo.nfps = framerate_Q16;
        ret = StandModule->mod_set_framerate(StandModule, framerate_Q16);
        camera_module_type->preview_fps = framerate_Q16;
    }
    else
    {
        camera_module_type->mCaptureInfo.nfps = framerate_Q16;
        camera_module_type->capture_fps = framerate_Q16;
    }
    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_getfps(void *handle, int *framerate_Q16, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);
    ret = StandModule->mod_get_framerate(StandModule, framerate_Q16);
    pthread_mutex_unlock(&camera_module_type->camera_mutex);

    V4L2DBUG(V4L2DBUG_VERB, "get fps %d\n", *framerate_Q16);
    
    return ret;
}

static int camera_g_ctrl(void *handle,  unsigned int id, unsigned int *value, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    //int ctl_id = 0;
    //int ctl_val = 0;
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    switch(id)
    {
    case CAM_GET_FOCUS_STATUS:
    {
        act_af_status_info_t *af_status = (act_af_status_info_t *)value;

        if(camera_module_type->focus_type == OMX_IMAGE_FocusControlOff)
        {
            af_status->bfocused = 0;
        }
        else
        {
            //af_status->bfocused = 3;
            ret = StandModule->mod_get_ctl(StandModule, V4L2_CID_AF_STATUS, &af_status->bfocused);
            if(af_status->bfocused == OMX_FocusStatusReached)
            {
            	af_status->nfocused_ratio = 100;
            }
            else if(af_status->bfocused == OMX_FocusStatusRequest)
            {
            	af_status->nfocused_ratio = 10;
            }
            else
            {
            	af_status->nfocused_ratio = -1;
            }
        }
    }
    break;

    case CAM_GET_SHUTTER:
    {
        ret = StandModule->mod_get_ctl(StandModule, V4L2_CID_EXPOSURE, value);
        V4L2DBUG(V4L2DBUG_VERB, "V4L2_CID_EXPOSURE %d", *value);
    }
    break;

    case CAM_GET_GAIN:
    {
        ret = StandModule->mod_get_ctl(StandModule, V4L2_CID_GAIN, value);
        V4L2DBUG(V4L2DBUG_VERB, "CAM_GET_GAIN %d", *value);
    }
    break;

    case CAM_GET_FOCUS_DIS:/*OMX_ACT_CONFIG_FOCUSDISTANCETYPE*/
    {

    }
    break;

    case CAM_GET_EVS:
        value[0] = camera_module_type->hdr_ev_info[0];
        value[1] = camera_module_type->hdr_ev_info[1];
        value[2] = camera_module_type->hdr_ev_info[2];
        break;

    default:
        break;
    }

    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    return ret;
}

static int camera_s_ctrl(void *handle, unsigned int id, unsigned long value, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    int ctl_id = 0;
    int ctl_val = 0;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    switch(id)
    {
    case CAM_SET_EV:
    {
        OMX_CONFIG_EXPOSUREVALUETYPE  *pType = (OMX_CONFIG_EXPOSUREVALUETYPE *)value;
        ctl_id = V4L2_CID_EXPOSURE_COMP;
        ctl_val = pType->xEVCompensation * 2 / 65536;
        V4L2DBUG(V4L2DBUG_PARAM, "CAM_SET_EV: %d\n", ctl_val);

        camera_module_type->cmp_ev = ctl_val;
        set_hdr_bracket(camera_module, ctl_val);
    }
    break;

    case CAM_SET_HDR_EV:
    {
        OMX_ACT_CONFIG_HDR_EVParams *pType = (OMX_ACT_CONFIG_HDR_EVParams *)value;
        camera_module_type->ev_auto = pType->bAutoMode;

        if(camera_module_type->ev_auto == OMX_FALSE)
        {
            camera_module_type->hdr_ev_param[0] = pType->nHighLightEV;
            camera_module_type->hdr_ev_param[1] = pType->nNormalLightEV;
            camera_module_type->hdr_ev_param[2] = pType->nLowLightEV;
            V4L2DBUG(V4L2DBUG_PARAM, "CAM_SET_HDR_EV: %d, %d, %d\n", pType->nNormalLightEV, pType->nLowLightEV, pType->nHighLightEV);
        }

        goto RET_UNSPT;
    }
    break;

    case CAM_SET_FLASH_TYPE:
    {
        OMX_IMAGE_PARAM_FLASHCONTROLTYPE *pType = (OMX_IMAGE_PARAM_FLASHCONTROLTYPE *)value;
        /*
        OMX_IMAGE_FlashControlOn = 0,
        OMX_IMAGE_FlashControlOff,
        OMX_IMAGE_FlashControlAuto,
        OMX_IMAGE_FlashControlRedEyeReduction,
        OMX_IMAGE_FlashControlFillin,
        OMX_IMAGE_FlashControlTorch,
        */
        V4L2DBUG(V4L2DBUG_ERR, "CAM_SET_FLASH_TYPE %d", pType->eFlashControl);
        ctl_id = V4L2_CID_FLASH_LED_MODE;

        if(pType->eFlashControl == OMX_IMAGE_FlashControlOn)
        {
            ctl_val = 1;
        }
        else if(pType->eFlashControl == OMX_IMAGE_FlashControlOff)
        {
            ctl_val = 0;
        }
        else if(pType->eFlashControl == OMX_IMAGE_FlashControlAuto)
        {
            ctl_val = 3;
        }
        else if(pType->eFlashControl == OMX_IMAGE_FlashControlTorch)
        {
            ctl_val = 2;
        }
    }
    break;

    case CAM_SET_FLASH_STROBE_MODE:
    {
        OMX_ACT_CONFIG_FlashStrobeParams *pType = (OMX_ACT_CONFIG_FlashStrobeParams *)value;
        V4L2DBUG(V4L2DBUG_ERR, "CAM_SET_FLASH_STROBE_MODE %d", pType->bStrobeOn);

        if(pType->bStrobeOn == OMX_TRUE)
        {
            if(pType->nElapsedTime > 0)
            {
                ctl_id = V4L2_CID_FLASH_TIMEOUT;
                ctl_val = pType->nElapsedTime;
            }
            else
            {
                ctl_id = V4L2_CID_FLASH_STROBE;
                ctl_val = 0;
            }
        }
        else
        {
            ctl_id = V4L2_CID_FLASH_STROBE_STOP;
            ctl_val = 0;
        }
    }
    break;

    case CAM_SET_WB_TYPE:/*OMX_WHITEBALCONTROLTYPE*/
    {
        OMX_CONFIG_WHITEBALCONTROLTYPE  *pType = (OMX_CONFIG_WHITEBALCONTROLTYPE *)value;

        if(pType->eWhiteBalControl == OMX_WhiteBalControlAuto)
        {
            ctl_id = V4L2_CID_AUTO_WHITE_BALANCE;
            ctl_val = 1;
        }
        else if(pType->eWhiteBalControl == OMX_WhiteBalControlOff)
        {
            ctl_id = V4L2_CID_AUTO_WHITE_BALANCE;
            ctl_val = 0;
        }
        else
        {
            if(camera_module_type->awb_type == OMX_WhiteBalControlAuto)
            {
                ctl_id = V4L2_CID_AUTO_WHITE_BALANCE;
                ctl_val = 0;
                ret = StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
            }

            ctl_id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
            ctl_val = manual_wb_tempture[pType->eWhiteBalControl];

            if(ctl_val < 0)
            {
                ret = -1;
                goto RET_UNSPT;
            }
        }

        camera_module_type->awb_type = pType->eWhiteBalControl;
    }
    break;

    case CAM_SET_EXP_TYPE:/*OMX_EXPOSURECONTROLTYPE*/
    {
        OMX_CONFIG_EXPOSURECONTROLTYPE  *pType = (OMX_CONFIG_EXPOSURECONTROLTYPE *)value;

        if(pType->eExposureControl == OMX_ExposureControlActHDR)
        {
            camera_module_type->hdr_mode = 1;
            goto RET_UNSPT;
        }
        else
        {
            camera_module_type->hdr_mode = 0;

            if(pType->eExposureControl == OMX_ExposureControlAuto)
            {
                ctl_id = V4L2_CID_EXPOSURE_AUTO;
                ctl_val = V4L2_EXPOSURE_AUTO;
                V4L2DBUG(V4L2DBUG_ERR, "V4L2_EXPOSURE_AUTO ");
            }
            else if(pType->eExposureControl == OMX_ExposureControlOff)
            {
                ctl_id = V4L2_CID_EXPOSURE_AUTO;
                ctl_val = V4L2_EXPOSURE_MANUAL;
            }
            else
            {
                if(camera_module_type->ae_type == OMX_ExposureControlAuto)
                {
                    ctl_id = V4L2_CID_EXPOSURE_AUTO;
                    ctl_val = V4L2_EXPOSURE_MANUAL;
                    ret = StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
                    V4L2DBUG(V4L2DBUG_ERR, "V4L2_EXPOSURE_MANUAL ");
                }

                /*
                 * enum  v4l2_scene_exposure_type {
                V4L2_SCENE_MODE_HOUSE           = 0,
                V4L2_SCENE_MODE_SUNSET          = 1,
                V4L2_SCENE_MODE_ACTION          = 2,
                V4L2_SCENE_MODE_PORTRAIT        = 3,
                V4L2_SCENE_MODE_LANDSCAPE       = 4,
                V4L2_SCENE_MODE_NIGHT           = 5,
                V4L2_SCENE_MODE_NIGHT_PORTRAIT  = 6,
                V4L2_SCENE_MODE_THEATRE         = 7,
                V4L2_SCENE_MODE_BEACH           = 8,
                V4L2_SCENE_MODE_SNOW            = 9,
                V4L2_SCENE_MODE_STEADYPHOTO     = 10,
                V4L2_SCENE_MODE_FIREWORKS       = 11,
                V4L2_SCENE_MODE_SPORTS          = 12,
                V4L2_SCENE_MODE_PARTY           = 13,
                V4L2_SCENE_MODE_CANDLELIGHT     = 14,
                V4L2_SCENE_MODE_BARCODE         = 15,
                };
                  OMX_ExposureControlOff = 0,
                    OMX_ExposureControlAuto,
                    OMX_ExposureControlNight,
                    OMX_ExposureControlBackLight,
                    OMX_ExposureControlSpotLight,
                    OMX_ExposureControlSports,
                    OMX_ExposureControlSnow,
                    OMX_ExposureControlBeach,
                    OMX_ExposureControlLargeAperture,
                    OMX_ExposureControlSmallApperture,

                    OMX__ExposureControlActHouse = 0x7F000001,0
                                OMX__ExposureControlActSunset,1
                                OMX__ExposureControlActAction,2
                                OMX__ExposureControlActPortrait,3
                                OMX__ExposureControlActLandscape,4
                                OMX__ExposureControlActNight_Portrait,6
                                OMX__ExposureControlActTheatre,7
                                OMX__ExposureControlActStreadyPhoto,10
                                OMX__ExposureControlActFireworks,11
                                OMX__ExposureControlActParty,13
                                OMX__ExposureControlActCandlelight,14
                                OMX__ExposureControlActBarcode,15
                 */

                if(pType->eExposureControl == OMX_ExposureControlNight)
                {
                    ctl_id = V4L2_CID_SCENE_EXPOSURE;
                    ctl_val = V4L2_SCENE_MODE_NIGHT;
                    //StandModule->mod_get_ctl(StandModule,ctl_id,&ctl_val);
                    //ctl_val = ctl_val * 12/10;
                }

                if(pType->eExposureControl == OMX_ExposureControlBackLight)
                {
                    //Fix here
                    goto RET_UNSPT;
                }

                if(pType->eExposureControl == OMX_ExposureControlSports)
                {
                    ctl_id = V4L2_CID_SCENE_EXPOSURE;
                    ctl_val = V4L2_SCENE_MODE_SPORTS;
                }

                if(pType->eExposureControl == OMX_ExposureControlSnow)
                {
                    ctl_id = V4L2_CID_SCENE_EXPOSURE;
                    ctl_val = V4L2_SCENE_MODE_NONE;
                }

                if(pType->eExposureControl > 0x7F000000 && pType->eExposureControl <= 0x7F00000F)
                {
                    //{0,1,2,3,4,6,7,10,11,13,14,15};
                    ctl_id = V4L2_CID_SCENE_EXPOSURE;
                    ctl_val = nscene_exp[(pType->eExposureControl & 0xf) - 1];
                    V4L2DBUG(V4L2DBUG_ERR, "eExposureControl %x,%x\n", ctl_val, pType->eExposureControl);
                }

            }
        }

        camera_module_type->ae_type = pType->eExposureControl;
    }
    break;

    case CAM_SET_HDR:
    {
        camera_module_type->hdr_mode = 0;
        goto RET_UNSPT;
    }
    break;

    case CAM_SET_BRIGHTNESS_LEVEL:
    {
        OMX_CONFIG_BRIGHTNESSTYPE *pType = (OMX_CONFIG_BRIGHTNESSTYPE *)value;
        ctl_id = V4L2_CID_BRIGHTNESS;
        ctl_val = pType->nBrightness;
        camera_module_type->bright_level = pType->nBrightness;
    }
    break;

    case CAM_SET_CONSTRAST_LEVEL:
    {
        OMX_CONFIG_CONTRASTTYPE *pType = (OMX_CONFIG_CONTRASTTYPE *)value;
        ctl_id = V4L2_CID_CONTRAST;
        ctl_val = pType->nContrast;
        camera_module_type->contrast_level = pType->nContrast;
    }
    break;

    case CAM_SET_DNS_LEVEL:
    {
        //OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *pType = (OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE*)value;
        //ctl_id = V4L2_CID_CONTRAST;
        //ctl_val = pType->nContrast*12/100;
        goto RET_UNSPT;
    }
    break;

    case CAM_SET_SHARP_LEVEL:
    {
        OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *pType = (OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *)value;
        ctl_id = V4L2_CID_SHARPNESS;
        ctl_val = pType->nLevel;
        camera_module_type->sharp_level = pType->nLevel;
    }
    break;

    case CAM_SET_FLICKER_TYPE:/*OMX_COMMONFLICKERCANCELTYPE*/
    {
        OMX_CONFIG_FLICKERREJECTIONTYPE *pType = (OMX_CONFIG_FLICKERREJECTIONTYPE *)value;
        ctl_id = V4L2_CID_POWER_LINE_FREQUENCY;

        if(pType->eFlickerRejection == OMX_FlickerRejectionOff)
        { ctl_val = pType->eFlickerRejection; }
        else if(pType->eFlickerRejection == OMX_FlickerRejection50)
        { ctl_val = pType->eFlickerRejection; }
        else if(pType->eFlickerRejection == OMX_FlickerRejection60)
        { ctl_val = pType->eFlickerRejection; }
        else
        {
            pthread_mutex_unlock(&camera_module_type->camera_mutex);
            return -1;
        }

        camera_module_type->flicker_type = pType->eFlickerRejection;
    }
    break;

    case CAM_SET_COLORFIX:/*OMX_IMAGEFILTERTYPE*/
    {
        OMX_CONFIG_IMAGEFILTERTYPE  *pType = (OMX_CONFIG_IMAGEFILTERTYPE *)value;
        ctl_id = V4L2_CID_COLORFX;

        if(pType->eImageFilter < 0x7F000000)
        { ctl_val = manual_color_effect[pType->eImageFilter]; }
        else
        {
            ctl_val = manual_color_effect_ext[pType->eImageFilter & 0xf];
            V4L2DBUG(V4L2DBUG_ERR, "ColorFixExt %x,%x\n", ctl_val, pType->eImageFilter);
        }

        V4L2DBUG(V4L2DBUG_VERB, "ColorFix %x,%x\n", ctl_val, pType->eImageFilter);

        if(ctl_val < 0)
        {
            V4L2DBUG(V4L2DBUG_ERR, "ColorFix %x,%x\n", ctl_val, pType->eImageFilter);
            ret = -1;
            goto RET_UNSPT;
        }

        camera_module_type->color_fixtype = pType->eImageFilter;
    }
    break;

    case CAM_SET_FLIP:/*OMX_MIRRORTYPE*/
    {
        OMX_CONFIG_MIRRORTYPE  *pType = (OMX_CONFIG_MIRRORTYPE *)value;

        if(pType->eMirror == OMX_MirrorHorizontal)
        { ctl_id = V4L2_CID_HFLIP; }
        else if(pType->eMirror == OMX_MirrorVertical)
        { ctl_id = V4L2_CID_VFLIP; }
        else
        {
            pthread_mutex_unlock(&camera_module_type->camera_mutex);
            return -1;
        }

        ctl_val = 1;
        camera_module_type->mirror_type = pType->eMirror;
    }
    break;

    case CAM_SET_COLORMTR:
    {
        goto RET_UNSPT;
    }
    break;

    case CAM_SET_GAMMA:
    {
        goto RET_UNSPT;
    }
    break;

    case CAM_SET_OPTIC_ZOOM:/*OMX_CONFIG_SCALEFACTORTYPE*/
    {
        goto RET_UNSPT;
    }
    break;

    case CAM_SET_BLIT_CMP:
    {
        goto RET_UNSPT;
    }
    break;

    case CAM_SET_MET_AREA:
    {
        //Not Support
        goto RET_UNSPT;
    }
    break;

    case CAM_SET_AF_TYPE: /*OMX_IMAGE_FOCUSCONTROLTYPE*/
    {
        //goto RET_UNSPT;
        OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE  *pType = (OMX_CONFIG_BRIGHTNESSTYPE *)value;
        V4L2DBUG(V4L2DBUG_VERB, "Focus mode %x", pType->eFocusControl);

        if(pType->eFocusControl == OMX_IMAGE_FocusControlOn)
        {
            ctl_id = V4L2_CID_AF_MODE;
            ctl_val = (0x1 << 5);
        }
        else if(pType->eFocusControl == OMX_IMAGE_FocusControlAuto)
        {
            ctl_id = V4L2_CID_AF_MODE;
            ctl_val = (0x1 << 2);
        }
        else if(pType->eFocusControl == OMX_IMAGE_FocusControlOff)
        {
            ctl_id = V4L2_CID_AF_MODE;
            ctl_val = 0;
        }
        else if(pType->eFocusControl == OMX_IMAGE_FocusControlSingle)
        {
            ctl_id = V4L2_CID_AF_MODE;
            ctl_val = (0x1 << 1);
        }
        else if(pType->eFocusControl == OMX_IMAGE_FocusControlZone)
        {
            ctl_id = V4L2_CID_AF_MODE;
            ctl_val = (0x1 << 3);
        }
        else if(pType->eFocusControl == OMX_IMAGE_FocusControlMacro)
        {
            ctl_id = V4L2_CID_AF_MODE;
            ctl_val = (0x1 << 4);
        }
        else if(pType->eFocusControl == OMX_IMAGE_FocusControlFACE)
        {
            ctl_id = V4L2_CID_AF_MODE;
            ctl_val = (0x1 << 6);
        }
        else
        {
            goto RET_UNSPT;
        }

        camera_module_type->focus_type = pType->eFocusControl;
    }
    break;

    case CAM_SET_AF_REGION:
    {
        act_af_region_info_t  *pType = (act_af_region_info_t *)value;
        struct v4l2_afregion centerxy;
        centerxy.position_x = pType->nrect_offset_x[0];
        centerxy.position_y = pType->nrect_offset_y[0];
        centerxy.width = pType->nrect_w;
        centerxy.height = pType->nrect_h;

        ctl_id = V4L2_CID_AF_REGION;
        ctl_val = &centerxy;
    }
    break;

    case CAM_SET_FOCUS_MANUL:
    {
        goto RET_UNSPT;
    }
    break;

    case CAM_GET_FOCUS_DIS:/*OMX_ACT_CONFIG_FOCUSDISTANCETYPE*/
    {
        goto RET_UNSPT;
    }
    break;

    case CAM_SET_AWBLOCK:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)value;

        if(pType->eImageLock == OMX_IMAGE_LockOff)
        {
            ctl_id = V4L2_CID_AUTO_WHITE_BALANCE;
            ctl_val = 1;
        }
        else if(pType->eImageLock == OMX_IMAGE_LockImmediate)
        {
            ctl_id = V4L2_CID_AUTO_WHITE_BALANCE;
            ctl_val = 0;
        }
        else if(pType->eImageLock == OMX_IMAGE_LockAtCapture)
        {
            goto RET_UNSPT;
        }

        camera_module_type->awb_lock = pType->eImageLock;
    }
    break;

    case CAM_SET_AELOCK:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)value;

        if(pType->eImageLock == OMX_IMAGE_LockOff)
        {
            ctl_id = V4L2_CID_EXPOSURE_AUTO;
            ctl_val = V4L2_EXPOSURE_AUTO;
        }
        else if(pType->eImageLock == OMX_IMAGE_LockImmediate)
        {
            ctl_id = V4L2_CID_EXPOSURE_AUTO;
            ctl_val = V4L2_EXPOSURE_MANUAL;
        }
        else if(pType->eImageLock == OMX_IMAGE_LockAtCapture)
        {
            goto RET_UNSPT;
        }

        camera_module_type->ae_lock = pType->eImageLock;
    }
    break;

    case CAM_SET_AFLOCK:
    {
        goto RET_UNSPT;
    }
    break;

    default:
        goto RET_UNSPT;
        break;
    }

    ret = StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
RET_UNSPT:
    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    return ret;
}

static int camera_requestbuf(void *handle, unsigned int *num_bufs, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);
    
    ret = StandModule->mod_requestbuf(StandModule, num_bufs);

    if(work_mode == OMX_PREVIEW_MODE)
    { camera_module_type->mPreviewInfo.nbufNum = *num_bufs; }
    else
    { camera_module_type->mCaptureInfo.nbufNum = *num_bufs; }

    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_streamoff(void *handle, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    int ret = 0;
    int i = 0;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;

    if(camera_module_type->bStreaming == 0)
    {
        V4L2DBUG(V4L2DBUG_ERR, "v4l2camera:Stream is off before now.%d,%d\n", work_mode, camera_module_type->bStreaming);
    }

    pthread_mutex_lock(&camera_module_type->camera_mutex);

    V4L2DBUG(V4L2DBUG_VERB, "%s: %d, %d, %d, %d", __func__, work_mode, camera_module_type->eprev_status, 
                camera_module_type->ecap_status, camera_module_type->bStreaming);

    if(work_mode == OMX_CAPTURE_MODE)
    {
        camera_module_type->ecap_status = CAM_IDLE;
#if 0

        for(i = 0; i < camera_module_type->mCaptureInfo.nbufNum; i++)
        {
            if(camera_module_type->mCaptureInfo.pBufVir[i])
            {
                ext_put_vir_addr(camera_module_type->mCaptureInfo.pBufVir[i]);
                camera_module_type->mCaptureInfo.pBufVir[i] = NULL;
            }
        }

#endif

        for(i = 0; i < camera_module_type->mCaptureInfo.nbufNum; i++)
        {
            camera_module_type->mCaptureInfo.pBufOwner[i] = OWN_BY_CLIENT;
            camera_module_type->mCaptureInfo.pBufPhy[i] = 0;
        }

        camera_module_type->mCaptureInfo.nbufNum = 0;
        camera_module_type->nCaptureSeq = 0;
        camera_module_type->cur_capture_num = 0;
        camera_module_type->cpnum = 0;
    }
    else if(work_mode == OMX_PREVIEW_MODE)
    {

#if 0

        for(i = 0; i < camera_module_type->mPreviewInfo.nbufNum; i++)
        {
            if(camera_module_type->mPreviewInfo.pBufVir[i])
            {
                ext_put_vir_addr(camera_module_type->mPreviewInfo.pBufVir[i]);
                camera_module_type->mPreviewInfo.pBufVir[i] = NULL;
            }
        }

#endif

        for(i = 0; i < camera_module_type->mPreviewInfo.nbufNum; i++)
        {
            camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_CLIENT;
            camera_module_type->mPreviewInfo.pBufPhy[i] = 0;
        }

        camera_module_type->eprev_status = CAM_IDLE;
        camera_module_type->mPreviewInfo.nbufNum = 0;
        camera_module_type->cur_preview_num = 0;
        camera_module_type->nPreviewSeq = 0;
        camera_module_type->bkTimeStamp = 0LL;
        camera_module_type->nOffset_Time = 0LL;
    }
    else if(work_mode == OMX_PREVIEW_CAP)
    {
        if(camera_module_type->eprev_status != CAM_PREVIEW_READY)
        {
            if(camera_module_type->mPreviewInfo.nbufNum > 0)
            {
                camera_module_type->eprev_status = CAM_PREVIEW_PAUSE;
            }
            else
            {
                camera_module_type->eprev_status = CAM_IDLE;
#if 0
                camera_module_type->nPreviewSeq = 0;

                for(i = 0; i < camera_module_type->mPreviewInfo.nbufNum; i++)
                {
                    camera_module_type->mPreviewInfo.pBufOwner[i] != OWN_BY_CLIENT
                }

#endif
            }
        }

        camera_module_type->ecap_status = CAM_IDLE;
        camera_module_type->nCaptureSeq = 0;
        camera_module_type->cpnum = 0;
#if 1

        for(i = 0; i < camera_module_type->mCaptureInfo.nbufNum; i++)
        {
            camera_module_type->mCaptureInfo.pBufOwner[i] = OWN_BY_CLIENT;
            camera_module_type->mCaptureInfo.pBufPhy[i] = 0;
        }

#endif
    }
    else
    {
        camera_module_type->eprev_status = CAM_PREVIEW_PAUSE;
        camera_module_type->bkTimeStamp = get_current_time();
    }

    if(camera_module_type->bStreaming == 1)
    {
        ret = StandModule->mod_streamoff(StandModule);
        V4L2DBUG(V4L2DBUG_ERR, "StreamOff Ok....%d,%d\n", camera_module_type->eprev_status, camera_module_type->nCaptureSeq);
        camera_module_type->bStreaming = 0;
    }

#if 1

    if(camera_module_type->eprev_status == CAM_PREVIEW_PAUSE && work_mode == OMX_PREVIEW_CAP)
    {
        V4L2DBUG(V4L2DBUG_VERB, "");
        
        camera_module_type->eprev_status = CAM_PREVIEW_READY;
#if 0
        //stream on preview again
        int i, j;
        int64_t tmp = 0;
        int start_idx = 0;
        int module_num = 0;
        //      int nStartNum = 0;
        int buflen = camera_module_type->mPreviewInfo.nh * camera_module_type->mPreviewInfo.nw * 3 / 2 + 4096;

        V4L2DBUG(V4L2DBUG_ERR, "E prev.....%d\n", camera_module_type->mPreviewInfo.nbufNum);
        //camera_module_type->eprev_status = CAM_PREVIEW_RUNNING;
        ret = StandModule->mod_set_res(StandModule, camera_module_type->preview_width, camera_module_type->preview_height, camera_module_type->preview_fmt);
        ret |= StandModule->mod_set_crop(StandModule, 0, 0, camera_module_type->preview_width, camera_module_type->preview_height);
        ret |= StandModule->mod_set_framerate(StandModule, camera_module_type->preview_fps);
        //ret |= StandModule->mod_streamon(StandModule);
        //camera_module_type->bStreaming = 1;
        ret |= StandModule->mod_requestbuf(StandModule, (unsigned int *)&camera_module_type->mPreviewInfo.nbufNum);
        V4L2DBUG(V4L2DBUG_ERR, "E prev reqbuf %x\n", camera_module_type->mPreviewInfo.nbufNum);
        /*for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
        {
            if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_CLIENT)
            {
                nStartNum = i;
            }
        }*/
        tmp = 0x7FFFFFFFFFFFFFFFLL;

        for(i = 0; i < camera_module_type->mPreviewInfo.nbufNum; i++)
        {
            if(camera_module_type->mPreviewInfo.pBufOwner[i] != OWN_BY_CLIENT)
            {
                if(tmp > camera_module_type->mPreviewInfo.nTimeStamp[i])
                {
                    tmp = camera_module_type->mPreviewInfo.nTimeStamp[i];
                    start_idx = i;
                }

                V4L2DBUG(V4L2DBUG_ERR, "Type %d,nTimeStamp %lld\n", camera_module_type->mPreviewInfo.pBufOwner[i], camera_module_type->mPreviewInfo.nTimeStamp[i]);
                module_num++;
            }
        }

        V4L2DBUG(V4L2DBUG_ERR, "module_num %d\n", module_num);

        //if((camera_module_type->nIndex + 1) == camera_module_type->mPreviewInfo.nbufNum)
        //  camera_module_type->nIndex = 0;
        //else
        //  camera_module_type->nIndex += 1;
        for(j = 0; j < module_num; j++)
        {
            for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
            {
                if(camera_module_type->mPreviewInfo.pBufOwner[i] != OWN_BY_CLIENT && camera_module_type->mPreviewInfo.nTimeStamp[i] == tmp)
                {
                    V4L2DBUG(V4L2DBUG_ERR, "reqbuf a %x %x,%lld\n", i, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.nTimeStamp[i]);
                    ret |= StandModule->mod_querybuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
                    ret |= StandModule->mod_qbuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
                    camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_MODULE;
                    i = camera_module_type->mPreviewInfo.nbufNum + 1;
                }
            }

            tmp++;
        }

#if 0

        for(i = 0 ; i < camera_module_type->nIndex; i++)
        {
            if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_MODULE)
            {
                V4L2DBUG(V4L2DBUG_VERB, "reqbuf a %x %x\n", i, camera_module_type->mPreviewInfo.pBufPhy[i]);
                ret |= StandModule->mod_querybuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
                ret |= StandModule->mod_qbuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);

            }

            //          else
            //              camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_MODULE;
        }

#endif
        /*for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
        {
            if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_CLIENT)
            {
                V4L2DBUG(V4L2DBUG_VERB,"reqbuf b %x %x\n",i,camera_module_type->mPreviewInfo.pBufPhy[i]);
                StandModule->mod_querybuf(StandModule,i,buflen,camera_module_type->mPreviewInfo.pBufPhy[i],camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
                camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_MODULE;
            }
        }*/
#endif
    }
    else
    {
        int i;

        if(work_mode == OMX_CAPTURE_MODE)
        {
            for(i = 0 ; i < camera_module_type->mCaptureInfo.nbufNum; i++)
            {
                camera_module_type->mCaptureInfo.pBufOwner[i] = OWN_BY_CLIENT;
            }
        }

#if 0
        else if(work_mode == OMX_PREVIEW_MODE)
        {
            for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
            {
                camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_CLIENT;
            }
        }

#endif
    }

#endif
    pthread_mutex_unlock(&camera_module_type->camera_mutex);

    return ret;
}

static int camera_querybuf(void *handle, int index, int buflen, unsigned long userptr, unsigned long resver, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    int ret = 0;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
    {
        V4L2DBUG(V4L2DBUG_ERR, "Qery buffer err now  a\n");

        if(work_mode == OMX_PREVIEW_MODE)
        {
            pthread_mutex_unlock(&camera_module_type->camera_mutex);
            ret = camera_streamoff(handle, OMX_PREVIEW_MODE);
            pthread_mutex_lock(&camera_module_type->camera_mutex);
        }
        else
        {
            pthread_mutex_unlock(&camera_module_type->camera_mutex);
            ret = camera_streamoff(handle, OMX_PREVIEW_MODE);
            pthread_mutex_lock(&camera_module_type->camera_mutex);
            camera_module_type->eprev_status = CAM_PREVIEW_PAUSE;
        }
    }
    else if(camera_module_type->ecap_status == CAM_CAPTURE_RUNNING)
    {
        V4L2DBUG(V4L2DBUG_ERR, "Qery buffer err now\n");
        pthread_mutex_unlock(&camera_module_type->camera_mutex);
        ret = camera_streamoff(handle, OMX_CAPTURE_MODE);
        pthread_mutex_lock(&camera_module_type->camera_mutex);
    }

#if 0
    if(work_mode == OMX_PREVIEW_MODE)
    {
#if 0

        if(camera_module_type->mPreviewInfo.pBufPhy[camera_module_type->cur_preview_num] != userptr)
        {
            if(camera_module_type->mPreviewInfo.pBufVir[camera_module_type->cur_preview_num])
            { ext_put_vir_addr(camera_module_type->mPreviewInfo.pBufVir[camera_module_type->cur_preview_num]); }
        }

#endif
        camera_module_type->mPreviewInfo.pBufPhy[camera_module_type->cur_preview_num] = userptr;
        camera_module_type->mPreviewInfo.pBufVir[camera_module_type->cur_preview_num] = userptr;//ext_get_vir_addr((unsigned int)userptr,buflen);
        camera_module_type->mPreviewInfo.pBufIndex[camera_module_type->cur_preview_num] = index;
        camera_module_type->mPreviewInfo.pBufOwner[camera_module_type->cur_preview_num] = OWN_BY_MODULE;
        camera_module_type->mPreviewInfo.pBufPhy_Stat[camera_module_type->cur_preview_num] = resver;
        camera_module_type->cur_preview_num++;
        V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:camera_qbuf addr prev a %d,%x\n", index, (unsigned int)userptr);
    }
    else
    {
#if 0

        if(camera_module_type->mCaptureInfo.pBufPhy[camera_module_type->cur_capture_num] != userptr)
        {
            if(camera_module_type->mCaptureInfo.pBufVir[camera_module_type->cur_capture_num])
            { ext_put_vir_addr(camera_module_type->mCaptureInfo.pBufVir[camera_module_type->cur_capture_num]); }
        }

#endif
        camera_module_type->mCaptureInfo.pBufPhy[camera_module_type->cur_capture_num] = userptr;
        camera_module_type->mCaptureInfo.pBufVir[camera_module_type->cur_capture_num] = userptr;//ext_get_vir_addr(userptr,buflen);
        camera_module_type->mCaptureInfo.pBufIndex[camera_module_type->cur_capture_num] = index;
        camera_module_type->mCaptureInfo.pBufOwner[camera_module_type->cur_capture_num] = OWN_BY_MODULE;
        camera_module_type->mCaptureInfo.pBufPhy_Stat[camera_module_type->cur_capture_num] = resver;
        camera_module_type->cur_capture_num++;
        V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:camera_qbuf addr cap a %d,%x\n", index, (unsigned int)userptr);
    }

#endif

    ret |= StandModule->mod_querybuf(StandModule, index, buflen, userptr, resver);

    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_qbuf(void *handle, int index, int buflen, unsigned long userptr, unsigned long resver, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    int ret = 0;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    pthread_mutex_lock(&camera_module_type->camera_mutex);
    
    V4L2DBUG(V4L2DBUG_VERB, "%s, %d,%d %d\n", __func__, work_mode, camera_module_type->mwork_mode, camera_module_type->bStreaming);

    if(camera_module_type->bStreaming == 1 && camera_module_type->mwork_mode == work_mode)
    {
        if(work_mode == OMX_PREVIEW_MODE)
        {

            if(camera_module_type->mPreviewInfo.pBufPhy[index] == 0)
            {
                camera_module_type->mPreviewInfo.pBufPhy[index] = userptr;
                camera_module_type->mPreviewInfo.pBufVir[index] = userptr;//ext_get_vir_addr((unsigned int)userptr,buflen);
                camera_module_type->mPreviewInfo.pBufIndex[index] = index;

                //camera_module_type->mPreviewInfo.pBufOwner[index] = OWN_BY_MODULE;
                if(resver)
                {
                    camera_module_type->mPreviewInfo.pBufPhy_Stat[index] = ((cam_stat_addr_t *)(resver))->pPhyAddr;
                    camera_module_type->mPreviewInfo.pBufVir_Stat[index] = ((cam_stat_addr_t *)(resver))->pVirAddr;
                }

                camera_module_type->cur_preview_num++;
                V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:camera_qbuf addr prev a %d,%x\n", index, (unsigned int)userptr);
            }

            if(camera_module_type->mPreviewInfo.pBufOwner[index] == OWN_BY_CLIENT)
            {
                pthread_mutex_unlock(&camera_module_type->camera_mutex);
                ret = StandModule->mod_qbuf(StandModule, index, buflen, userptr, 0);
                pthread_mutex_lock(&camera_module_type->camera_mutex);

                if(ret == 0)
                { V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:camera_qbuf prv addr %d,%x\n", index, (unsigned int)userptr); }
            }

            camera_module_type->mPreviewInfo.pBufOwner[index] = OWN_BY_MODULE;
            //index = camera_module_type->mPreviewInfo.pBufIndex[index];
            camera_module_type->mPreviewInfo.nTimeStamp[index] = camera_module_type->nPreviewSeq;
            camera_module_type->nPreviewSeq++;
            camera_module_type->mPreviewInfo.nPreVSize = buflen;
        }
        else
        {
            camera_module_type->mCaptureInfo.nPreVSize = buflen;

            if(camera_module_type->mCaptureInfo.pBufPhy[index] == 0)
            {
                camera_module_type->mCaptureInfo.pBufPhy[index] = userptr;
                camera_module_type->mCaptureInfo.pBufVir[index] = userptr;//ext_get_vir_addr(userptr,buflen);
                camera_module_type->mCaptureInfo.pBufIndex[index] = index;

                //camera_module_type->mCaptureInfo.pBufOwner[index] = OWN_BY_MODULE;
                if(resver)
                { camera_module_type->mCaptureInfo.pBufPhy_Stat[index] = ((cam_stat_addr_t *)(resver))->pPhyAddr; }

                camera_module_type->cur_capture_num++;
                V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:camera_qbuf addr cap a %d,%x\n", index, (unsigned int)userptr);
            }


            if(camera_module_type->mCaptureInfo.pBufOwner[index] == OWN_BY_CLIENT)
            {
                pthread_mutex_unlock(&camera_module_type->camera_mutex);
                ret = StandModule->mod_qbuf(StandModule, index, buflen, userptr, 0);
                pthread_mutex_lock(&camera_module_type->camera_mutex);

                if(ret == 0)
                { V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:camera_qbuf cap addr %d,%x\n", index, (unsigned int)userptr); }
            }

            camera_module_type->mCaptureInfo.pBufOwner[index] = OWN_BY_MODULE;
            camera_module_type->mCaptureInfo.nTimeStamp[index] = camera_module_type->nCaptureSeq;
            index = camera_module_type->mCaptureInfo.pBufIndex[index];
            camera_module_type->nCaptureSeq++;
        }
    }
    else //if(camera_module_type->bStreaming == 1 && camera_module_type->mwork_mode != work_mode)
    {
        if(work_mode == OMX_PREVIEW_MODE)
        {
            camera_module_type->mPreviewInfo.nPreVSize = buflen;

            if(camera_module_type->mPreviewInfo.pBufPhy[index] == 0)
            {
                camera_module_type->mPreviewInfo.pBufPhy[index] = userptr;
                camera_module_type->mPreviewInfo.pBufVir[index] = userptr;//ext_get_vir_addr((unsigned int)userptr,buflen);
                camera_module_type->mPreviewInfo.pBufIndex[index] = index;

                //camera_module_type->mPreviewInfo.pBufOwner[index] = OWN_BY_MODULE;
                //camera_module_type->mPreviewInfo.pBufPhy_Stat[index] = resver;
                if(resver)
                {
                    camera_module_type->mPreviewInfo.pBufPhy_Stat[index] = ((cam_stat_addr_t *)(resver))->pPhyAddr;
                    camera_module_type->mPreviewInfo.pBufVir_Stat[index] = ((cam_stat_addr_t *)(resver))->pVirAddr;
                }

                camera_module_type->cur_preview_num++;
                V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:camera_qbuf S addr prev a %d,%x\n", index, (unsigned int)userptr);
            }

            camera_module_type->mPreviewInfo.pBufOwner[index] = OWN_BY_CLITOMOD;
            camera_module_type->mPreviewInfo.nTimeStamp[index] = camera_module_type->nPreviewSeq;
            camera_module_type->nPreviewSeq++;
        }
        else if(work_mode == OMX_CAPTURE_MODE)
        {
            camera_module_type->mCaptureInfo.nPreVSize = buflen;

            if(camera_module_type->mCaptureInfo.pBufPhy[index] == 0)
            {
                camera_module_type->mCaptureInfo.pBufPhy[index] = userptr;
                camera_module_type->mCaptureInfo.pBufVir[index] = userptr;//ext_get_vir_addr(userptr,buflen);
                camera_module_type->mCaptureInfo.pBufIndex[index] = index;

                //camera_module_type->mCaptureInfo.pBufOwner[index] = OWN_BY_MODULE;
                //camera_module_type->mCaptureInfo.pBufPhy_Stat[index] = resver;
                if(resver)
                {
                    camera_module_type->mCaptureInfo.pBufPhy_Stat[index] = ((cam_stat_addr_t *)(resver))->pPhyAddr;
                    camera_module_type->mCaptureInfo.pBufVir_Stat[index] = ((cam_stat_addr_t *)(resver))->pVirAddr;
                }

                camera_module_type->cur_capture_num++;
                V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:camera_qbuf S addr cap a %d,%x\n", index, (unsigned int)userptr);
            }

            camera_module_type->mCaptureInfo.pBufOwner[index] = OWN_BY_CLITOMOD;
            camera_module_type->mCaptureInfo.nTimeStamp[index] = camera_module_type->nCaptureSeq;
            camera_module_type->nCaptureSeq++;
        }
    }

    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    return ret;
}

static int camera_dqbuf(void *handle, int *index, unsigned long *userptr, unsigned long *resver, long long *timestamp, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    wdog_cb_t *omx_wdog_cb = &camera_module->omx_wdog_cb;
    char cb_info[128];
    int64_t t1, t2;
    int ret = 0;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    if(camera_module_type->bStreaming == 0)
    {
        pthread_mutex_unlock(&camera_module_type->camera_mutex);
        return -1;
    }

    if(camera_module_type->bStreaming == 1 && camera_module_type->mwork_mode == work_mode)
    {
        pthread_mutex_unlock(&camera_module_type->camera_mutex);
        if(NULL != omx_wdog_cb->handle)
        {
            sprintf(cb_info, "%s %d", __func__, __LINE__);
            omx_wdog_cb->func(omx_wdog_cb->handle, cb_info);
        }
        t1 = get_current_time();
        ret = StandModule->mod_dqbuf(StandModule, index, userptr, resver, timestamp);
        t2 = get_current_time() - t1;
        if(NULL != omx_wdog_cb->handle)
        {
            sprintf(cb_info, "%s %d", __func__, __LINE__);
            omx_wdog_cb->func(omx_wdog_cb->handle, cb_info);
        }
        pthread_mutex_lock(&camera_module_type->camera_mutex);

        if(ret == 0)
        {
            if(work_mode == OMX_PREVIEW_MODE)
            {
                *index = find_index(camera_module_type->mPreviewInfo.pBufIndex, 32, *index);
                camera_module_type->mPreviewInfo.pBufOwner[*index] = OWN_BY_CLIENT;

                if(camera_module_type->bAdjustFlag)
                {
                    camera_module_type->bAdjustFlag = 0;
                    camera_module_type->nOffset_Time = (camera_module_type->nCurrent_Time - *timestamp + 100000);
                    V4L2DBUG(V4L2DBUG_ERR, "Adjust Time %lld,%lld,%lld\n", camera_module_type->nOffset_Time, camera_module_type->nCurrent_Time, *timestamp);
                    *timestamp += camera_module_type->nOffset_Time;
                }
                else
                {
                    *timestamp += camera_module_type->nOffset_Time;

                    if(*timestamp < camera_module_type->nCurrent_Time)
                    {
                        V4L2DBUG(V4L2DBUG_ERR, "Adjust Time err %lld,%lld\n", *timestamp, camera_module_type->nCurrent_Time);
                        *timestamp = camera_module_type->nCurrent_Time + 1000000 / camera_module_type->preview_fps;
                        camera_module_type->nOffset_Time += (1000000 / camera_module_type->preview_fps);
                    }
                }

                camera_module_type->nCurrent_Time = *timestamp;
            }
            else
            {
                unsigned int pPhyAddr = 0;
                unsigned int nBufIdx = 0;
                int buflen = camera_module_type->capture_width *  camera_module_type->capture_height * 3 / 2;
                int exp_time = 0;
                int exp_gain = 0;

                if(camera_module_type->hdr_mode == 1)
                {
                    if(camera_module_type->cpnum == 0)
                    {
                        int i = 0;
                        while(i--)
                        {
                            pPhyAddr = *userptr;
                            nBufIdx = *index;
                            StandModule->mod_qbuf(StandModule, nBufIdx, buflen, pPhyAddr, 0);
                            StandModule->mod_dqbuf(StandModule, index, userptr, resver, timestamp);
                            //V4L2DBUG(V4L2DBUG_ERR, "dqbuf: %d, %d", camera_module_type->cpnum, *index);
                        }

                        V4L2DBUG(V4L2DBUG_PARAM, "hdr_ev_param: %d\n", camera_module_type->hdr_ev_param[1]);
                        camera_module_type->hdr_ev_info[camera_module_type->cpnum%3] = ev_adt(camera_module, camera_module_type->hdr_ev_param[1]);
                        //camera_module_type->hdr_ev_info[camera_module_type->cpnum % 3] = ev_adt_tg(camera_module, camera_module_type->hdr_ev_param[2], &(camera_module_type->exp_time), &(camera_module_type->exp_gain), 0);
                        camera_module_type->cpnum++;
                    }
                    else if(camera_module_type->cpnum == 1)
                    {
                        int i = 3;
                        while(i--)
                        {
                            pPhyAddr = *userptr;
                            nBufIdx = *index;
                            StandModule->mod_qbuf(StandModule, nBufIdx, buflen, pPhyAddr, 0);
                            StandModule->mod_dqbuf(StandModule, index, userptr, resver, timestamp);
                            //V4L2DBUG(V4L2DBUG_ERR, "dqbuf: %d, %d", camera_module_type->cpnum, *index);
                        }

                        V4L2DBUG(V4L2DBUG_PARAM, "hdr_ev_param: %d\n", camera_module_type->hdr_ev_param[2]);
                        camera_module_type->hdr_ev_info[camera_module_type->cpnum%3] = ev_adt(camera_module, camera_module_type->hdr_ev_param[2]);
                        exp_time = camera_module_type->exp_time;
                        exp_gain = camera_module_type->exp_gain;
                        //camera_module_type->hdr_ev_info[camera_module_type->cpnum % 3] = ev_adt_tg(camera_module, camera_module_type->hdr_ev_param[0], &exp_time, &exp_gain, 1);
                        camera_module_type->cpnum++;
                    }
                    else if(camera_module_type->cpnum == 2)
                    {
                        int i = 3;
                        while(i--)
                        {
                            pPhyAddr = *userptr;
                            nBufIdx = *index;
                            StandModule->mod_qbuf(StandModule, nBufIdx, buflen, pPhyAddr, 0);
                            StandModule->mod_dqbuf(StandModule, index, userptr, resver, timestamp);
                            //V4L2DBUG(V4L2DBUG_ERR, "dqbuf: %d, %d", camera_module_type->cpnum, *index);
                        }

                        camera_module_type->hdr_ev_info[camera_module_type->cpnum % 3] = ev_adt(camera_module, camera_module_type->hdr_ev_param[0]);
                        exp_time = camera_module_type->exp_time;
                        exp_gain = camera_module_type->exp_gain;
                        //camera_module_type->hdr_ev_info[camera_module_type->cpnum % 3] = ev_adt_tg(camera_module, 0, &exp_time, &exp_gain, 1);
                        camera_module_type->cpnum++;
                    }
                }
                else
                { ; }

                V4L2DBUG(V4L2DBUG_PARAM, "Capture time: %lld,%d,%d,%d,%d", t2, buflen, camera_module_type->hdr_ev_info[0], camera_module_type->hdr_ev_info[1], camera_module_type->hdr_ev_info[2]);
                *index = find_index(camera_module_type->mCaptureInfo.pBufIndex, 32, *index);
                camera_module_type->mCaptureInfo.pBufOwner[*index] = OWN_BY_CLIENT;
            }
        }

        if(ret == 0)
        {
            V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:OMX VL4 DQ %x,%d,%d\n", (unsigned int)*userptr, work_mode, *index);

            if(work_mode == OMX_PREVIEW_MODE)
            { camera_module_type->nIndex = *index; }
        }
    }
    else
    {
        pthread_mutex_unlock(&camera_module_type->camera_mutex);
        return -1;
    }

    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    return ret;
}

static int camera_streamon(void *handle, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_direct_base_PrivateType *camera_module_type = (camera_direct_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int eprev_status;
    int ret = 0;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    V4L2DBUG(V4L2DBUG_VERB, "%d, %d, %d, %d", work_mode, camera_module_type->eprev_status, 
                camera_module_type->ecap_status, camera_module_type->bStreaming);

    eprev_status = camera_module_type->eprev_status;
    
#if 0
    if(work_mode == OMX_PREVIEW_MODE)
    {
        if(camera_module_type->cur_preview_num < camera_module_type->mPreviewInfo.nbufNum)
        {
            pthread_mutex_unlock(&camera_module_type->camera_mutex);
            return -1;
        }
    }
#endif

    if(work_mode == OMX_PREVIEW_MODE)
    {
        if(camera_module_type->eprev_status == CAM_PREVIEW_READY)
        {
#if 1
            //stream on preview again
            int i, j;
            int64_t tmp = 0;
            int start_idx = 0;
            int module_num = 0;
            //      int nStartNum = 0;
            int buflen = camera_module_type->mPreviewInfo.nh * camera_module_type->mPreviewInfo.nw * 3 / 2 + 4096;

            V4L2DBUG(V4L2DBUG_ERR, "E prev.....%d\n", camera_module_type->mPreviewInfo.nbufNum);
            //camera_module_type->eprev_status = CAM_PREVIEW_RUNNING;
            ret = StandModule->mod_set_mode(StandModule, OMX_PREVIEW_MODE);
            ret = StandModule->mod_set_res(StandModule, camera_module_type->preview_width, camera_module_type->preview_height, camera_module_type->preview_fmt);
            ret |= StandModule->mod_set_crop(StandModule, 0, 0, camera_module_type->preview_width, camera_module_type->preview_height);

            //              ret |= StandModule->mod_set_framerate(StandModule,camera_module_type->preview_fps);
            //              ret |= StandModule->mod_requestbuf(StandModule,(unsigned int*)&camera_module_type->mPreviewInfo.nbufNum);
            //              V4L2DBUG(V4L2DBUG_ERR,"E prev reqbuf %x\n",camera_module_type->mPreviewInfo.nbufNum);
            //              tmp = 0x7FFFFFFFFFFFFFFFLL;
            //
            //              for(i = 0; i < camera_module_type->mPreviewInfo.nbufNum; i++)
            //              {
            //                  if(camera_module_type->mPreviewInfo.pBufOwner[i] != OWN_BY_CLIENT)
            //                  {
            //                      if(tmp > camera_module_type->mPreviewInfo.nTimeStamp[i])
            //                      {
            //                          tmp = camera_module_type->mPreviewInfo.nTimeStamp[i];
            //                          start_idx = i;
            //                      }
            //                      V4L2DBUG(V4L2DBUG_ERR,"Type %d,nTimeStamp %lld\n",camera_module_type->mPreviewInfo.pBufOwner[i],camera_module_type->mPreviewInfo.nTimeStamp[i]);
            //                      module_num++;
            //                  }
            //              }
            //             V4L2DBUG(V4L2DBUG_ERR,"module_num %d\n",module_num);
            //
            //              for(j = 0; j <module_num; j++ ){
            //                  for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
            //                  {
            //                      if(camera_module_type->mPreviewInfo.pBufOwner[i] != OWN_BY_CLIENT && camera_module_type->mPreviewInfo.nTimeStamp[i] == tmp)
            //                      {
            //                          V4L2DBUG(V4L2DBUG_ERR,"reqbuf a %x %x,%lld\n",i,camera_module_type->mPreviewInfo.pBufPhy[i],camera_module_type->mPreviewInfo.nTimeStamp[i]);
            //                          ret |= StandModule->mod_querybuf(StandModule,i,buflen,camera_module_type->mPreviewInfo.pBufPhy[i],camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
            //                          ret |= StandModule->mod_qbuf(StandModule,i,buflen,camera_module_type->mPreviewInfo.pBufPhy[i],camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
            //                          camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_MODULE;
            //                          i = camera_module_type->mPreviewInfo.nbufNum + 1;
            //                      }
            //                  }
            //                  tmp++;
            //              }

            restore_direct_preview_bufq(camera_module_type);
            camera_module_type->eprev_status == CAM_PREVIEW_PAUSE;
#endif
        }

        if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
        {
            //camera_streamoff(handle,OMX_PREVIEW_MODE);
            V4L2DBUG(V4L2DBUG_VERB, "ALready runingg\n");
            pthread_mutex_unlock(&camera_module_type->camera_mutex);
            return 0;
        }
        else if(camera_module_type->ecap_status == CAM_PREVIEW_RUNNING)
        {
            V4L2DBUG(V4L2DBUG_VERB, "camera_streamon %d\n", __LINE__);

            if(camera_module_type->bStreaming == 1)
            {
                pthread_mutex_unlock(&camera_module_type->camera_mutex);
                ret = camera_streamoff(handle, OMX_CAPTURE_MODE);
                pthread_mutex_lock(&camera_module_type->camera_mutex);
                camera_module_type->ecap_status = CAM_IDLE;
            }
        }
        else if(camera_module_type->eprev_status == CAM_PREVIEW_PAUSE)
        {
            camera_module_type->nOffset_Time += (get_current_time() - camera_module_type->bkTimeStamp);
            camera_module_type->bAdjustFlag = 1;
            V4L2DBUG(V4L2DBUG_ERR, "camera_preview time %lld,%d\n", camera_module_type->nOffset_Time, camera_module_type->bAdjustFlag);
        }

        camera_module_type->eprev_status = CAM_PREVIEW_RUNNING;
    }
    else
    {
        //V4L2DBUG(V4L2DBUG_ERR, "camera_streamon %d\n", __LINE__);

        if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
        {
            //camera_streamoff(handle,OMX_PREVIEW_MODE);
            if(camera_module_type->bStreaming == 1)
            {
                pthread_mutex_unlock(&camera_module_type->camera_mutex);
                ret = camera_streamoff(handle, OMX_PREVIEW_MODE);
                pthread_mutex_lock(&camera_module_type->camera_mutex);
                camera_module_type->eprev_status = CAM_PREVIEW_PAUSE;
            }
        }
        else if(camera_module_type->ecap_status == CAM_PREVIEW_RUNNING)
        {
            pthread_mutex_unlock(&camera_module_type->camera_mutex);
            return 0;
        }

        camera_module_type->ecap_status = CAM_CAPTURE_RUNNING;
    }

    if(work_mode == OMX_PREVIEW_MODE)
    {
        int ctl_id = 0;
        int ctl_val = 0;

        //if(eprev_status != CAM_PREVIEW_PAUSE && camera_module_type->nPreviewSeq > 0){
        //  ret|=qtmpBuf(camera_module_type);
        //}
        if(camera_module_type->ae_lock == OMX_IMAGE_LockAtCapture)
        {
            ctl_id = V4L2_CID_EXPOSURE_AUTO;
            ctl_val = V4L2_EXPOSURE_AUTO;
            ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        }

        if(camera_module_type->awb_lock == OMX_IMAGE_LockAtCapture)
        {
            ctl_id = V4L2_CID_AUTO_WHITE_BALANCE;
            ctl_val = 1;
            ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        }

        //ctl_id = V4L2_CID_PRIVATE_PREV_CAPT;
        //ctl_val = OMX_PREVIEW_MODE;
        //ret |= StandModule->mod_set_ctl(StandModule,ctl_id,ctl_val);
    }
    else
    {
        int ctl_id = 0;
        int ctl_val = 0;

        if(camera_module_type->nCaptureSeq)
        {
            ret |= qtmpCBuf(camera_module_type);
        }

        if(camera_module_type->ae_lock == OMX_IMAGE_LockAtCapture)
        {
            ctl_id = V4L2_CID_EXPOSURE_AUTO;
            ctl_val = V4L2_EXPOSURE_MANUAL;
            ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        }

        if(camera_module_type->awb_lock == OMX_IMAGE_LockAtCapture)
        {
            ctl_id = V4L2_CID_AUTO_WHITE_BALANCE;
            ctl_val = 0;
            ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        }

        //ctl_id = V4L2_CID_PRIVATE_PREV_CAPT;
        //ctl_val = OMX_CAPTURE_MODE;
        //ret |= StandModule->mod_set_ctl(StandModule,ctl_id,ctl_val);
    }

    V4L2DBUG(V4L2DBUG_VERB, "Stream On now %d\n", camera_module_type->bStreaming);

    if(camera_module_type->bStreaming == 0)
    {
        ret |= StandModule->mod_streamon(StandModule);
    }

    V4L2DBUG(V4L2DBUG_PARAM, "v4l2camera:Stream On OK! %d\n", work_mode);
    camera_module_type->bStreaming = 1;
    camera_module_type->mwork_mode = work_mode;
    
    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_setcb(void *handle, wdog_cb_t *cb)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_module->omx_wdog_cb.func = cb->func;
    camera_module->omx_wdog_cb.handle = cb->handle;

    return 0;
}


int camera_direct_base_Constructor(OMX_CAMERATYPE *omx_camera)
{
    int ret = 0;
    camera_direct_base_PrivateType *camera_direct = (camera_direct_base_PrivateType *)calloc(1, sizeof(camera_direct_base_PrivateType));
    if(camera_direct == NULL)
    {
        return -1;
    }

    camera_direct->StandModule = (CAMERA_MODULETYPE *)calloc(1, sizeof(CAMERA_MODULETYPE));

    if(camera_direct->StandModule == NULL)
    {
        return -1;
    }

    V4L2DBUG(V4L2DBUG_ERR, " Direct cons. %d\n", __LINE__);
    memcpy(camera_direct->module_name, "yuv-module", 10);
    ret = Camera_Module_constructer(camera_direct->StandModule);
    omx_camera->pCameraPrivate = camera_direct;
    camera_direct->ipp_handle = (ispctl_handle_t *)omx_camera->pIppHnale;

    camera_direct->focus_type = OMX_IMAGE_FocusControlOff;
    omx_camera->omx_camera_open = open_device;
    omx_camera->omx_camera_close = close_device;
    omx_camera->omx_camera_setres = camera_s_fmt;
    omx_camera->omx_camera_getres = camera_g_fmt;
    omx_camera->omx_camera_setzoom = camera_set_crop;
    omx_camera->omx_camera_getzoom = camera_get_crop;
    omx_camera->omx_camera_setfps = camera_setfps;
    omx_camera->omx_camera_getfps = camera_getfps;
    omx_camera->omx_camera_setctl = camera_s_ctrl;
    omx_camera->omx_camera_getctl = camera_g_ctrl;
    omx_camera->omx_camera_requestbuf = camera_requestbuf;
    omx_camera->omx_camera_querybuf = camera_querybuf;
    omx_camera->omx_camera_qbuf = camera_qbuf;
    omx_camera->omx_camera_dqbuf = camera_dqbuf;
    omx_camera->omx_camera_streamon = camera_streamon;
    omx_camera->omx_camera_streamoff = camera_streamoff;
    omx_camera->omx_camera_setcb = camera_setcb;
    
    pthread_mutex_init(&camera_direct->camera_mutex, NULL);
    
    return ret;
}

int camera_direct_base_Destructor(OMX_CAMERATYPE *omx_camera)
{
    int ret = 0;
    camera_direct_base_PrivateType *camera_direct = (camera_direct_base_PrivateType *)omx_camera->pCameraPrivate;

    if(camera_direct)
    {
#if 0
        int i = 0;

        for(i = 0; i < camera_direct->mCaptureInfo.nbufNum; i++)
        {
            if(camera_direct->mCaptureInfo.pBufVir[i])
            {
                ext_put_vir_addr(camera_direct->mCaptureInfo.pBufVir[i]);
                camera_direct->mCaptureInfo.pBufVir[i] = NULL;
            }
        }

        for(i = 0; i < camera_direct->mPreviewInfo.nbufNum; i++)
        {
            if(camera_direct->mPreviewInfo.pBufVir[i])
            {
                ext_put_vir_addr(camera_direct->mPreviewInfo.pBufVir[i]);
                camera_direct->mPreviewInfo.pBufVir[i] = NULL;
            }
        }

#endif
        ret = Camera_Module_destructer(camera_direct->StandModule);

        if(camera_direct->StandModule)
        {
            free(camera_direct->StandModule);
            camera_direct->StandModule = NULL;
        }

        pthread_mutex_destroy(&camera_direct->camera_mutex);
        free(camera_direct);
    }

    return ret;
}

#ifdef __cplusplus
}
#endif
// mCameraHandle = open(device, O_RDWR)
