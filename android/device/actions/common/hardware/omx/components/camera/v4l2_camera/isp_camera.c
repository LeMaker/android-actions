#ifdef __cplusplus
extern "C" {
#endif

#include "ACT_OMX_Common_V1_2__V1_1.h"
#include <OMX_IVCommon.h>
#include <linux/videodev2.h>
#include "ACT_OMX_IVCommon.h"
#include "omx_camera.h"
#include "OMX_Image.h"
#include "isp_camera.h"
#include "ispctl_adapter.h"
#include "ext_mem.h"
#include "perf.h"
#include "log.h"


static int camera_streamoff(void *handle, int work_mode);

static int ispcam_callback_expo(int msg_type, void *msg_info, void *user)
{
		int ret = 0;
    camera_isp_base_PrivateType *camera_direct = (camera_isp_base_PrivateType *)user;
    CAMERA_MODULETYPE *cam_mod = camera_direct->StandModule;
    act_msg_info_t *data = (act_msg_info_t *)msg_info;

    switch(msg_type)
    {
    case ACT_S_EXP_GAIN:
        ret |= cam_mod->mod_set_ctl(cam_mod, V4L2_CID_EXPOSURE, data->nExpTime);
        ret |= cam_mod->mod_set_ctl(cam_mod, V4L2_CID_GAIN, data->nExpGain);
        break;

    case ACT_G_EXP_GAIN:
        ret |= cam_mod->mod_get_ctl(cam_mod, V4L2_CID_EXPOSURE, (int *)&data->nExpTime);
        ret |= cam_mod->mod_get_ctl(cam_mod, V4L2_CID_GAIN, (int *)&data->nExpGain);
        break;

    case ACT_S_MOTOR_POS:
        ret |= cam_mod->mod_set_ctl(cam_mod, V4L2_CID_MOTOR, data->nMotorPos);
        break;
        
    case ACT_G_MOTOR_POS:
        ret |= cam_mod->mod_get_ctl(cam_mod, V4L2_CID_MOTOR, (int *)&data->nMotorPos);
        break;
        
    case ACT_G_MOTOR_RNG:
        ret |= cam_mod->mod_get_ctl(cam_mod, V4L2_CID_MOTOR_GET_MAX, (int *)&data->nMotorRng);
        break; 
        
    default:
        V4L2DBUG(V4L2DBUG_ERR, "err! not support this msg(%d)!\n", msg_type);
        break;
    }

    return ret;    
}

static int find_index(int *idxPtr, int num, int idx_target)
{
    int i = 0;

    //int idx = 0;
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

static int IPP_DQbuf(camera_isp_base_PrivateType *camera_module_type, int workmode, int index)
{
    int ret = 0;
    int i;
    int64_t c1, c2;
    int do_flag = 0;

    if(workmode == OMX_PREVIEW_MODE)
    {
        if((camera_module_type->preview_idx % 2) == 0)
        {
            do_flag = 1;
            camera_module_type->preview_idx = 0;
        }

        camera_module_type->preview_idx++;
    }
    else
    {
        //if((camera_module_type->capture_idx%2) == 0)
        {
            do_flag = 0;
            camera_module_type->capture_idx = 0;
        }
        camera_module_type->capture_idx++;
    }

    //if(do_flag == 1)
    {
        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_PROCESS, NULL); 
    }

    return ret;
}

static int qtmpBuf(camera_isp_base_PrivateType *camera_module_type)
{
    int ret = 0;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int i = 0, j = 0;
    int module_num = 0;
    int64_t tmp = 0;
    int start_idx = 0;
    int buflen = camera_module_type->mPreviewInfo.nh * camera_module_type->mPreviewInfo.nw * 3 / 2 + 4096;
    tmp = 0x7FFFFFFFFFFFFFFFLL;

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
#if 0
                if(camera_module_type->mPreviewInfo.pBufFlag[i] == 0)
                {
                    ipp_buf_reg_t buf_reg;
                    buf_reg.device_fd = (int)StandModule->pModuleHandle;
                    buf_reg.frm_num = 1;

                    if(camera_module_type->mPreviewInfo.pBufPhy_Stat[i] != 0)
                    {
                        int frm_addr = camera_module_type->mPreviewInfo.pBufVir_Stat[i]; // ext_get_vir_addr(camera_module_type->mPreviewInfo.pBufPhy_Stat[i],4096);
                        camera_module_type->mPreviewInfo.pBufVir_Stat[i] = frm_addr;
                        buf_reg.addr_offset = 0;
                        buf_reg.frm_addr = &camera_module_type->mPreviewInfo.pBufVir_Stat[i];
                        buf_reg.phy_addr = &camera_module_type->mPreviewInfo.pBufPhy_Stat[i];
                    }

                    camera_module_type->mPreviewInfo.pBufFlag[i] = 1;
                }

                camera_module_type->videoinfo_p[i].stat_addr = (void *)camera_module_type->mPreviewInfo.pBufPhy_Stat[i];
#endif
                ret |= StandModule->mod_qbuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], 0); //&camera_module_type->videoinfo_p[i]);
                camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_MODULE;
                i = camera_module_type->mPreviewInfo.nbufNum + 1;
            }
        }

        tmp++;
    }

    return ret;
}


static int qtmpCBuf(camera_isp_base_PrivateType *camera_module_type)
{
    int ret = 0;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int i = 0, j = 0;
    int module_num = 0;
    int64_t tmp = 0;
    int start_idx = 0;
    int buflen = camera_module_type->mCaptureInfo.nh * camera_module_type->mCaptureInfo.nw * 3 / 2 + 4096;
    tmp = 0x7FFFFFFFFFFFFFFFLL;

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
                V4L2DBUG(V4L2DBUG_ERR, "qtmpCBuf a %x,%x, %x %lld,%lld\n", i, camera_module_type->mCaptureInfo.pBufFlag[i], camera_module_type->mCaptureInfo.pBufPhy[i], camera_module_type->mCaptureInfo.nTimeStamp[i], camera_module_type->nCaptureSeq);
                //ret |= StandModule->mod_querybuf(StandModule,i,buflen,camera_module_type->mCaptureInfo.pBufPhy[i],camera_module_type->mCaptureInfo.pBufPhy_Stat[i]);
#if 0
                if(camera_module_type->mCaptureInfo.pBufFlag[i] == 0)
                {
                    ipp_capture_info_t pinfo;
                    ipp_videoinfo_t vid_inf;
                    ipp_buf_reg_t buf_reg;
                    pinfo.buf_frame = &buf_reg;
                    pinfo.videoinfo = &vid_inf;

                    vid_inf.device_fd = (int)StandModule->pModuleHandle;
                    vid_inf.framerate = camera_module_type->mCaptureInfo.nfps;
                    vid_inf.height = camera_module_type->mCaptureInfo.nh;
                    vid_inf.width = camera_module_type->mCaptureInfo.nw;

                    buf_reg.device_fd = (int)StandModule->pModuleHandle;
                    buf_reg.frm_num = 1;

                    if(camera_module_type->mCaptureInfo.pBufPhy_Stat[i] != 0)
                    {
                        int frm_addr = camera_module_type->mCaptureInfo.pBufVir_Stat[i];//ext_get_vir_addr(camera_module_type->mCaptureInfo.pBufPhy_Stat[i],4096);
                        camera_module_type->mCaptureInfo.pBufVir_Stat[i] = frm_addr;
                        buf_reg.addr_offset = 0;
                        buf_reg.frm_addr = &camera_module_type->mCaptureInfo.pBufVir_Stat[i];
                        buf_reg.phy_addr = &camera_module_type->mCaptureInfo.pBufPhy_Stat[i];
                    }

                    camera_module_type->mCaptureInfo.pBufFlag[i] = 1;
                }

                camera_module_type->videoinfo_c[i].stat_addr = (void *)camera_module_type->mCaptureInfo.pBufPhy_Stat[i];
#endif
                ret |= StandModule->mod_qbuf(StandModule, i, buflen, camera_module_type->mCaptureInfo.pBufPhy[i], 0); //&camera_module_type->videoinfo_c[i]);
                camera_module_type->mCaptureInfo.pBufOwner[i] = OWN_BY_MODULE;
                i = camera_module_type->mCaptureInfo.nbufNum + 1;
            }
        }

        tmp++;
    }

    return ret;
}
static int restore_preview_bufq(camera_isp_base_PrivateType *camera_module_type)
{
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int i = 0, j = 0;
    int64_t tmp = 0;
    int start_idx = 0;
    int module_num = 0;
    //int nStartNum = 0;
    int buflen = camera_module_type->mPreviewInfo.nh * camera_module_type->mPreviewInfo.nw * 3 / 2 + 4096;
    int ret = 0;

    //ret = StandModule->mod_set_res(StandModule,camera_module_type->preview_width,camera_module_type->preview_height,camera_module_type->preview_fmt);
    //ret |= StandModule->mod_set_crop(StandModule,0,0,camera_module_type->preview_width,camera_module_type->preview_height);
    ret |= StandModule->mod_set_framerate(StandModule, camera_module_type->preview_fps);
    ret |= StandModule->mod_requestbuf(StandModule, (unsigned int *)&camera_module_type->mPreviewInfo.nbufNum);
#if 0

    for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
    {
        if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_MODULE)
        {
            V4L2DBUG(V4L2DBUG_VERB, "reqbuf a %x %x\n", i, camera_module_type->mPreviewInfo.pBufPhy[i]);
            StandModule->mod_querybuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
        }
    }

    for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
    {
        if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_CLIENT)
        {
            V4L2DBUG(V4L2DBUG_VERB, "reqbuf b %x %x\n", i, camera_module_type->mPreviewInfo.pBufPhy[i]);
            StandModule->mod_querybuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
            camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_MODULE;
        }
    }

#else
//	    for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
//	    {
//	        if(camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_CLIENT)
//	        {
//	            nStartNum = i;
//	        }
//	    }
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
            if(camera_module_type->mPreviewInfo.pBufOwner[i] != OWN_BY_CLIENT && camera_module_type->mPreviewInfo.nTimeStamp[i] == tmp)
            {
                V4L2DBUG(V4L2DBUG_ERR, "reqbuf 196 a %x %x\n", i, camera_module_type->mPreviewInfo.pBufPhy[i]);
                ret |= StandModule->mod_querybuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
#if 0

                if(camera_module_type->mPreviewInfo.pBufFlag[i] == 0)
                {
                    ipp_buf_reg_t buf_reg;
                    buf_reg.device_fd = (int)StandModule->pModuleHandle;
                    buf_reg.frm_num = 1;

                    if(camera_module_type->mPreviewInfo.pBufPhy_Stat[i] != 0)
                    {
                        int frm_addr = camera_module_type->mPreviewInfo.pBufVir_Stat[i];//ext_get_vir_addr(camera_module_type->mPreviewInfo.pBufPhy_Stat[i],4096);
                        camera_module_type->mPreviewInfo.pBufVir_Stat[i] = frm_addr;
                        buf_reg.addr_offset = 0;
                        buf_reg.frm_addr = &camera_module_type->mPreviewInfo.pBufVir_Stat[i];
                        buf_reg.phy_addr = &camera_module_type->mPreviewInfo.pBufPhy_Stat[i];
                    }

                    camera_module_type->mPreviewInfo.pBufFlag[i] = 1;
                }

                camera_module_type->videoinfo_p[i].stat_addr = (void *)camera_module_type->mPreviewInfo.pBufPhy_Stat[i];
#endif
                ret |= StandModule->mod_qbuf(StandModule, i, buflen, camera_module_type->mPreviewInfo.pBufPhy[i], 0); //&camera_module_type->videoinfo_p[i]);
                camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_MODULE;
                i = camera_module_type->mPreviewInfo.nbufNum + 1;
            }
        }

        tmp++;
    }

    if(camera_module_type->awb_type)
    {
        V4L2DBUG(V4L2DBUG_ERR, "AWB Restore %d\n", camera_module_type->awb_type);
        //
    }

    if(camera_module_type->ae_type)
    {
        V4L2DBUG(V4L2DBUG_ERR, "AE Restore \n");
        //
    }

    if(camera_module_type->color_fixtype)
    {
        V4L2DBUG(V4L2DBUG_ERR, "clor Restore \n");
        //
    }
#endif

    return ret;
}

static int open_device(void *handle, int id, int mode_type,int uvcmode)
{
    OMX_CAMERATYPE *camera_module_handle = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_direct = (camera_isp_base_PrivateType *)camera_module_handle->pCameraPrivate;
    CAMERA_MODULETYPE *cam_module = camera_direct->StandModule;
    int ret = 0;
    int i;
    
    pthread_mutex_lock(&camera_direct->camera_mutex);
    cam_module->mod_open(cam_module, id,uvcmode);

    if(camera_direct->ipp_handle)
    {
        act_cb_info_t cb_info;
        act_sensor_info_t sensor_info;

        // open
        V4L2DBUG(V4L2DBUG_VERB, "act_isp_open, %i\n", id);
        ret = camera_direct->ipp_handle->act_isp_open(camera_direct->ipp_handle);
        if(ret != 0)
        {
            V4L2DBUG(V4L2DBUG_ERR, "act_isp_open failed\n");
            cam_module->mod_close(cam_module);
        }

        // shutter-gain callback
        cb_info.nDataCb = ispcam_callback_expo;
        cb_info.pusr = camera_direct;
        ret |= camera_direct->ipp_handle->act_isp_cmd(camera_direct->ipp_handle, ACT_ISP_S_CB, &cb_info);

        // sensor id
        cam_module->mod_get_ctl(cam_module, V4L2_CID_SENSOR_ID, &sensor_info.sensor_id);
        V4L2DBUG(V4L2DBUG_PARAM, "get sensor id: 0x%04x\n", sensor_info.sensor_id);
        camera_direct->ipp_handle->act_isp_cmd(camera_direct->ipp_handle, ACT_ISP_S_SENSOR_ID, &sensor_info);

        // init
        ret |= camera_direct->ipp_handle->act_isp_cmd(camera_direct->ipp_handle, ACT_ISP_S_INIT_PARAM, NULL);
    }
    else
    {
        pthread_mutex_unlock(&camera_direct->camera_mutex);
        return -1;
    }

    for(i = 0; i < MX_BUF_NUM; i++)
    {
        camera_direct->mPreviewInfo.pBufVir_Stat[i] = 0;
        camera_direct->mPreviewInfo.pBufOwner[i] = OWN_BY_CLIENT;
        camera_direct->mPreviewInfo.pBufPhy[i] = 0;
        camera_direct->mPreviewInfo.pBufFlag[i] = 0;

        camera_direct->mCaptureInfo.pBufVir_Stat[i] = 0;
        camera_direct->mCaptureInfo.pBufOwner[i] = OWN_BY_CLIENT;
        camera_direct->mCaptureInfo.pBufPhy[i] = 0;
        camera_direct->mCaptureInfo.pBufFlag[i] = 0;
    }

    //camera_direct->mmodle_type = mode_type;
    pthread_mutex_unlock(&camera_direct->camera_mutex);

    // shutter-gain test
    if(0)
    {
        for(i = 64; i < 521; i++)
        {
            int ii;
            cam_module->mod_set_ctl(cam_module, V4L2_CID_GAIN, i);
            cam_module->mod_get_ctl(cam_module, V4L2_CID_GAIN, &ii);
            V4L2DBUG(V4L2DBUG_PARAM, "gain test: %d, %d\n", i, ii);
        }
    }
    
    return ret;
}

static int close_device(void *handle)
{
    OMX_CAMERATYPE *camera_module_handle = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_direct = (camera_isp_base_PrivateType *)camera_module_handle->pCameraPrivate;
    CAMERA_MODULETYPE *cam_module = camera_direct->StandModule;
    int ret = 0;
    
    pthread_mutex_lock(&camera_direct->camera_mutex);

    if(camera_direct->ipp_handle)
    {
        V4L2DBUG(V4L2DBUG_PARAM, "act_isp_close\n");
        ret = camera_direct->ipp_handle->act_isp_close(camera_direct->ipp_handle);
        cam_module->mod_close(cam_module);
    }

    pthread_mutex_unlock(&camera_direct->camera_mutex);
    
    return ret;
}

static int camera_s_fmt(void *handle, int w, int h, int fmt, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    act_isp_video_info_t isp_video_info;
    int ret = 0;

    V4L2DBUG(V4L2DBUG_VERB, "%dx%d, %d, %d, %d, %d", w, h, work_mode, camera_module_type->eprev_status, 
                camera_module_type->ecap_status, camera_module_type->bStreaming);

    pthread_mutex_lock(&camera_module_type->camera_mutex);

    if(work_mode == OMX_PREVIEW_MODE)
    {
        if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
        {
            if((w != camera_module_type->preview_width) || \
                    (h != camera_module_type->preview_height))
            {
                isp_video_info.width = w;
                isp_video_info.height = h;
                isp_video_info.framerate = camera_module_type->preview_fps;
                isp_video_info.work_mode = work_mode;
                ret = camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_VIDEOINFO, &isp_video_info);
                if(ret != 0)
                {
                    V4L2DBUG(V4L2DBUG_ERR, "err! ACT_ISP_S_VIDEOINFO failed!\n");
                    return ret;
                }
                
                camera_module_type->preview_width = w;
                camera_module_type->preview_height = h;
                camera_module_type->preview_fmt = fmt;

                if(camera_module_type->bStreaming == 1)
                {
                    ret |= StandModule->mod_streamoff(StandModule);
                    ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
                    ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
                    ret |= restore_preview_bufq(camera_module_type);
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

            isp_video_info.width = w;
            isp_video_info.height = h;
            isp_video_info.framerate = camera_module_type->preview_fps;
            isp_video_info.work_mode = work_mode;
            ret = camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_VIDEOINFO, &isp_video_info);
            if(ret != 0)
            {
                V4L2DBUG(V4L2DBUG_ERR, "err! ACT_ISP_S_VIDEOINFO failed!\n");
                return ret;
            }

            ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
            ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
        }

        camera_module_type->mPreviewInfo.nw = w;
        camera_module_type->mPreviewInfo.nh = h;
    }
    else if(work_mode == OMX_CAPTURE_MODE)
    {
        isp_video_info.width = w;
        isp_video_info.height = h;
        isp_video_info.framerate = camera_module_type->capture_fps;
        isp_video_info.work_mode = work_mode;
        ret = camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_VIDEOINFO, &isp_video_info);
        if(ret != 0)
        {
            V4L2DBUG(V4L2DBUG_ERR, "err! ACT_ISP_S_VIDEOINFO failed!\n");
            return ret;
        }
        
        if(camera_module_type->ecap_status == CAM_CAPTURE_RUNNING)
        {
            if(camera_module_type->bStreaming == 1)
            {
                ret |= StandModule->mod_streamoff(StandModule);
                ret |= StandModule->mod_set_res(StandModule, w, h, fmt);
                ret |= StandModule->mod_set_crop(StandModule, 0, 0, w, h);
                ret |= restore_preview_bufq(camera_module_type);
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

    }
    else if(work_mode == OMX_CAPTURE_PREP)
    {
        camera_module_type->capture_width = w;
        camera_module_type->capture_height = h;
        camera_module_type->capture_fmt = fmt;
        camera_module_type->mCaptureInfo.nw = w;
        camera_module_type->mCaptureInfo.nh = h;
    }

    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_g_fmt(void *handle, int *w, int *h, int *fmt, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
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
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    act_isp_video_info_t isp_video_info;
    int ret = 0;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    if(work_mode == OMX_PREVIEW_MODE)
    {
        //StandModule->mod_set_crop(StandModule,x,y,width,height);
        
        if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
        {
            if((w != camera_module_type->preview_width) || \
                    (h != camera_module_type->preview_height))
            {
                isp_video_info.width = w;
                isp_video_info.height = h;
                isp_video_info.framerate = camera_module_type->preview_fps;
                isp_video_info.work_mode = work_mode;
                ret = camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_VIDEOINFO, &isp_video_info);
                if(ret != 0)
                {
                    V4L2DBUG(V4L2DBUG_ERR, "err! ACT_ISP_S_VIDEOINFO failed!\n");
                    return ret;
                }

                camera_module_type->preview_width = w;
                camera_module_type->preview_height = h;
            }

        }
        else
        {
            camera_module_type->preview_width = w;
            camera_module_type->preview_height = h;
            //camera_module_type->preview_fmt = fmt;

            isp_video_info.width = w;
            isp_video_info.height = h;
            isp_video_info.framerate = camera_module_type->preview_fps;
            isp_video_info.work_mode = work_mode;
            ret = camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_VIDEOINFO, &isp_video_info);
            if(ret != 0)
            {
                V4L2DBUG(V4L2DBUG_ERR, "err! ACT_ISP_S_VIDEOINFO failed!\n");
                return ret;
            }
        }

        camera_module_type->mPreviewInfo.nw = w;
        camera_module_type->mPreviewInfo.nh = h;
    }
    else
    {
        if(camera_module_type->ecap_status == CAM_CAPTURE_RUNNING)
        {
            isp_video_info.width = w;
            isp_video_info.height = h;
            isp_video_info.framerate = camera_module_type->preview_fps;
            isp_video_info.work_mode = work_mode;
            ret = camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_VIDEOINFO, &isp_video_info);
            if(ret != 0)
            {
                V4L2DBUG(V4L2DBUG_ERR, "err! ACT_ISP_S_VIDEOINFO failed!\n");
                return ret;
            }
        }

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
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
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
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;

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

    //ret = StandModule->mod_set_framerate(StandModule,framerate_Q16);
    
    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_getfps(void *handle, int *framerate_Q16, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);
    ret = StandModule->mod_get_framerate(StandModule, framerate_Q16);
    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_g_ctrl(void *handle,  unsigned int id, unsigned int *value, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    switch(id)
    {
    case CAM_GET_SENSOR_ID:
    {
        ret = StandModule->mod_get_ctl(StandModule, V4L2_CID_SENSOR_ID, value);
    }
    break;
    
    case CAM_GET_FOCUS_DIS:/*OMX_ACT_CONFIG_FOCUSDISTANCETYPE*/
    {
        ;
    }
    break;
    
    case CAM_GET_FOCUS_STATUS:
    {
        act_af_status_info_t *af_status = (act_af_status_info_t *)value;

        if(camera_module_type->af_type == OMX_IMAGE_FocusControlOff)
        {
            af_status->bfocused = 0;
        }
        else
        {
            camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_G_AF_STATUS, af_status);
            //V4L2DBUG(V4L2DBUG_ERR, "CAM_GET_FOCUS_STATUS %d,%d", af_status->bfocused,af_status->nfocused_ratio);
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
    
    case CAM_GET_EVS:
    {
        value[0] = camera_module_type->hdr_ev_info[0];
        value[1] = camera_module_type->hdr_ev_info[1];
        value[2] = camera_module_type->hdr_ev_info[2];
    }
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
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    int ctl_id = 0;
    int ctl_val = 0;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    V4L2DBUG(V4L2DBUG_VERB, "s_ctrl mode: %i,%i\n", id, work_mode);

    switch(id)
    {
    case CAM_SET_WB_TYPE:/*OMX_WHITEBALCONTROLTYPE*/
    {
        OMX_CONFIG_WHITEBALCONTROLTYPE  *pType = (OMX_CONFIG_WHITEBALCONTROLTYPE *)value;
        V4L2DBUG(V4L2DBUG_PARAM, "CAM_SET_WB_TYPE %d\n", pType->eWhiteBalControl);
        
        camera_module_type->awb_type = pType->eWhiteBalControl;
        ;
        
        break;
    }

    case CAM_SET_FLASH_TYPE:
    {
        OMX_IMAGE_PARAM_FLASHCONTROLTYPE *pType = (OMX_IMAGE_PARAM_FLASHCONTROLTYPE *)value;
        V4L2DBUG(V4L2DBUG_PARAM, "CAM_SET_FLASH_TYPE %d", pType->eFlashControl);
        
//	        OMX_IMAGE_FlashControlOn = 0,
//	        OMX_IMAGE_FlashControlOff,
//	        OMX_IMAGE_FlashControlAuto,
//	        OMX_IMAGE_FlashControlRedEyeReduction,
//	        OMX_IMAGE_FlashControlFillin,
//	        OMX_IMAGE_FlashControlTorch,

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

        ret = StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        break;
    }

    case CAM_SET_FLASH_STROBE_MODE:
    {
        OMX_ACT_CONFIG_FlashStrobeParams *pType = (OMX_ACT_CONFIG_FlashStrobeParams *)value;
        V4L2DBUG(V4L2DBUG_PARAM, "CAM_SET_FLASH_STROBE_MODE %d", pType->bStrobeOn);

        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_FLASH_STROBE, pType->bStrobeOn);

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

        ret = StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
        break;
    }

    case CAM_SET_EXP_TYPE:/*OMX_EXPOSURECONTROLTYPE*/
    {
        OMX_CONFIG_EXPOSURECONTROLTYPE  *pType = (OMX_CONFIG_EXPOSURECONTROLTYPE *)value;
        camera_module_type->ae_type = pType->eExposureControl;

        if(pType->eExposureControl == OMX_ExposureControlActHDR)
        {
            camera_module_type->hdr_mode = 1;
        }
        else
        {
            camera_module_type->hdr_mode = 0;
            ;
        }

        break;
    }

    case CAM_SET_HDR:
    {
        camera_module_type->hdr_mode = 0;
        break;
    }

    case CAM_SET_AF_TYPE:/*OMX_IMAGE_FOCUSCONTROLTYPE*/
    {
        OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE *pFocusCtl = (OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE *)value;
        OMX_IMAGE_FOCUSCONTROLTYPE focusType = pFocusCtl->eFocusControl;

        V4L2DBUG(V4L2DBUG_ERR, "Focus mode %x", pFocusCtl->eFocusControl);
        if(pFocusCtl->eFocusControl == OMX_IMAGE_FocusControlOn)
        {
        		camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_AF_MODE, ACT_AF_SINGLE); 
        }
        else if(pFocusCtl->eFocusControl == OMX_IMAGE_FocusControlAuto)
        {
        		camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_AF_MODE, ACT_AF_CONTINUOUS);
        		camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_AF_RUN, 0); 
        }
        else if(pFocusCtl->eFocusControl == OMX_IMAGE_FocusControlOff)
        {
        		camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_AF_STOP, 0); 
        }
        else if(pFocusCtl->eFocusControl == OMX_IMAGE_FocusControlSingle 
        	      || pFocusCtl->eFocusControl == OMX_IMAGE_FocusControlZone)
        {
						if(camera_module_type->af_type ==OMX_IMAGE_FocusControlOff) 
						{
							V4L2DBUG(V4L2DBUG_ERR,"OMX_IMAGE_FocusControlOff need start again");
							camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_AF_RUN, 0); 
						}
        		camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_AF_MODE, ACT_AF_SINGLE); 
        }
        else if(pFocusCtl->eFocusControl == OMX_IMAGE_FocusControlQuickCapture)
        {
        	  V4L2DBUG(V4L2DBUG_ERR,"OMX_IMAGE_FocusControlQuickCapture\n");
        		camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_AF_MODE, 2);
        		camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_AF_RUN, 0); 
        }
        else if(pFocusCtl->eFocusControl == OMX_IMAGE_FocusControlMacro)
        {
            V4L2DBUG(V4L2DBUG_ERR,"err!OMX_IMAGE_FocusControlMacro\n");
        }
        else if(pFocusCtl->eFocusControl == OMX_IMAGE_FocusControlFACE)
        {
            V4L2DBUG(V4L2DBUG_ERR,"err!OMX_IMAGE_FocusControlFACE\n");
        }

        camera_module_type->af_type = focusType;
        break;
    }

    case CAM_SET_EV:
    {
        OMX_CONFIG_EXPOSUREVALUETYPE  *pType = (OMX_CONFIG_BRIGHTNESSTYPE *)value;
        V4L2DBUG(V4L2DBUG_ERR, "CAM_SET_EV: %i\n", pType->xEVCompensation);
        
        ;

        break;
    }

    case CAM_SET_BRIGHTNESS_LEVEL:
    {
        OMX_CONFIG_BRIGHTNESSTYPE *pType = (OMX_CONFIG_BRIGHTNESSTYPE *)value;
        camera_module_type->bright_level = pType->nBrightness;
        
        ;
        
        break;
    }

    case CAM_SET_CONSTRAST_LEVEL:
    {
        OMX_CONFIG_CONTRASTTYPE *pType = (OMX_CONFIG_CONTRASTTYPE *)value;
        camera_module_type->contrast_level = pType->nContrast;
        
        ;
        
        break;
    }

    case CAM_SET_DNS_LEVEL:
    {
        OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *pType = (OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *)value;
        camera_module_type->dns_level = value;
        
        ;
        
        break;
    }

    case CAM_SET_SHARP_LEVEL:
    {
        OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *pType = (OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *)value;
        camera_module_type->sharp_level = pType->nLevel;
        
        ;
        
        break;
    }

    case CAM_SET_AF_REGION:
    {
        act_af_region_info_t *pType = (act_af_region_info_t *)value;
        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_AF_REGION, pType); 
        break;
    }

    case CAM_SET_FOCUS_MANUL:
    {
        OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE *pFocusCtl = (OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE *)value;
        
        ;
        
        break;
    }

    case CAM_SET_FLICKER_TYPE:/*OMX_COMMONFLICKERCANCELTYPE*/
    {
        OMX_CONFIG_FLICKERREJECTIONTYPE *pType = (OMX_CONFIG_FLICKERREJECTIONTYPE *)value;
        camera_module_type->flicker_type = pType->eFlickerRejection;
        
        if(pType->eFlickerRejection == OMX_FlickerRejectionOff)
        { 
            //camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_BANDING, ACT_BANDING_NONE); 
        }
        else if(pType->eFlickerRejection == OMX_FlickerRejection50)
        { 
            //camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_BANDING, ACT_BANDING_50HZ);
        }
        else if(pType->eFlickerRejection == OMX_FlickerRejection60)
        { 
            //camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_BANDING, ACT_BANDING_60HZ);
        }
        else
        {
            pthread_mutex_unlock(&camera_module_type->camera_mutex);
            return -1;
        }
        
        break;
    }

    case CAM_SET_OPTIC_ZOOM:/*OMX_CONFIG_SCALEFACTORTYPE*/
    {
        break;
    }

    case CAM_SET_BLIT_CMP:
    {
        break;
    }

    case CAM_SET_COLORFIX_MANUAL:
    {
        OMX_CONFIG_COLORENHANCEMENTTYPE  *pType = (OMX_CONFIG_COLORENHANCEMENTTYPE *)value;

        ;

        break;
    }

    case CAM_SET_COLORFIX:/*OMX_IMAGEFILTERTYPE*/
    {
        OMX_CONFIG_IMAGEFILTERTYPE  *pType = (OMX_CONFIG_IMAGEFILTERTYPE *)value;
        
        ;
        
        break;
    }

    case CAM_SET_FLIP:/*OMX_MIRRORTYPE*/
    {
        OMX_CONFIG_MIRRORTYPE  *pType = (OMX_CONFIG_MIRRORTYPE *)value;
        int ctl_id, ctl_val;

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
        //ret = StandModule->mod_set_ctl(StandModule,ctl_id,ctl_val);
        
        ;
        
        break;
    }

    case CAM_SET_MET_AREA:/*OMX_ACT_CONFIG_FOCUSDISTANCETYPE*/
    {
        ;
        
        break;
    }

    case CAM_SET_AWBLOCK:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)value;
        if(pType->eImageLock == OMX_IMAGE_LockOff)
        {
            ;
        }
        else if(pType->eImageLock == OMX_IMAGE_LockImmediate)
        {
            ;
        }
        else if(pType->eImageLock == OMX_IMAGE_LockAtCapture)
        {
            ;
        }

        camera_module_type->awb_lock = pType->eImageLock;

        break;
    }

    case CAM_SET_AELOCK:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)value;
        if(pType->eImageLock == OMX_IMAGE_LockOff)
        {
            ;
        }
        else if(pType->eImageLock == OMX_IMAGE_LockImmediate)
        {
            ;
        }
        else if(pType->eImageLock == OMX_IMAGE_LockAtCapture)
        {
            ;
        }

        camera_module_type->ae_lock = pType->eImageLock;

        break;
    }

    case CAM_SET_AFLOCK:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)value;
        if(pType->eImageLock == OMX_IMAGE_LockOff)
        {
            ;
        }
        else if(pType->eImageLock == OMX_IMAGE_LockImmediate)
        {
            ;
        }
        else if(pType->eImageLock == OMX_IMAGE_LockAtCapture)
        {
            ;
        }

        camera_module_type->af_lock = pType->eImageLock;

        break;
    }

    default:
        V4L2DBUG(V4L2DBUG_ERR, "s_ctrl, undefined command!\n");
        break;
    }

    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_requestbuf(void *handle, unsigned int *num_bufs, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;
    
    V4L2DBUG(V4L2DBUG_VERB, "requestbuf mode %i\n", work_mode);
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);
    
    ret = StandModule->mod_requestbuf(StandModule, num_bufs);
    if(work_mode == OMX_PREVIEW_MODE)
    { camera_module_type->mPreviewInfo.nbufNum = *num_bufs; }
    else
    { camera_module_type->mCaptureInfo.nbufNum = *num_bufs; }

    V4L2DBUG(V4L2DBUG_PARAM, "req bufnum %d,%d\n", camera_module_type->mPreviewInfo.nbufNum, camera_module_type->mCaptureInfo.nbufNum);
    
    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_querybuf(void *handle, int index, int buflen, unsigned long userptr, unsigned long resver, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int ret = 0;

    pthread_mutex_lock(&camera_module_type->camera_mutex);

    if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
    {
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
        V4L2DBUG(V4L2DBUG_PARAM, "Qery buffer err now\n");
        
        pthread_mutex_unlock(&camera_module_type->camera_mutex);
        ret = camera_streamoff(handle, OMX_CAPTURE_MODE);
        pthread_mutex_lock(&camera_module_type->camera_mutex);
    }

    ret |= StandModule->mod_querybuf(StandModule, index, buflen, userptr, resver);
    
    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_qbuf(void *handle, int index, int buflen, unsigned long userptr, unsigned long resver, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int idx = 0;
    int ret = 0;
    
    V4L2DBUG(V4L2DBUG_VERB, "qbuf: %i,%i,%i,%x,%x,%i,%i,%i\n", index, work_mode, camera_module_type->bStreaming, (unsigned int)userptr, resver, camera_module_type->mPreviewInfo.pBufPhy[index], camera_module_type->mPreviewInfo.pBufOwner[index], camera_module_type->mPreviewInfo.pBufFlag[index]);

    pthread_mutex_lock(&camera_module_type->camera_mutex);

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
                //camera_module_type->mPreviewInfo.pBufPhy_Stat[index] = resver;
                if(resver)
                {
                    camera_module_type->mPreviewInfo.pBufPhy_Stat[index] = ((cam_stat_addr_t *)(resver))->pPhyAddr;
                    camera_module_type->mPreviewInfo.pBufVir_Stat[index] = ((cam_stat_addr_t *)(resver))->pVirAddr;
                }

                camera_module_type->cur_preview_num++;
                //V4L2DBUG(V4L2DBUG_ERR,"v4l2camera:camera_qbuf addr prev a %x,%x\n",camera_module_type->mPreviewInfo.pBufPhy_Stat[index],(unsigned int)camera_module_type->mPreviewInfo.pBufVir_Stat[index]);
            }

            if(camera_module_type->mPreviewInfo.pBufOwner[index] == OWN_BY_CLIENT)
            {
                pthread_mutex_unlock(&camera_module_type->camera_mutex);
                ret = StandModule->mod_qbuf(StandModule, index, buflen, userptr, 0); //&camera_module_type->videoinfo_p[index]);
                pthread_mutex_lock(&camera_module_type->camera_mutex);

                if(ret == 0)
                { V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:camera_qbuf prv addr %d,%x\n", index, (unsigned int)userptr); }
            }

            camera_module_type->mPreviewInfo.pBufOwner[index] = OWN_BY_MODULE;
            //index = camera_module_type->mPreviewInfo.pBufIndex[index];
            camera_module_type->mPreviewInfo.nTimeStamp[index] = camera_module_type->nPreviewSeq;
            camera_module_type->nPreviewSeq++;
        }
        else
        {
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
                V4L2DBUG(V4L2DBUG_VERB, "v4l2camera:camera_qbuf addr cap a %d,%x\n", index, (unsigned int)userptr);
            }

            if(camera_module_type->mCaptureInfo.pBufOwner[index] == OWN_BY_CLIENT)
            {
                pthread_mutex_unlock(&camera_module_type->camera_mutex);
                ret = StandModule->mod_qbuf(StandModule, index, buflen, userptr, 0); //&camera_module_type->videoinfo_c[index]);
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
                V4L2DBUG(V4L2DBUG_ERR, "v4l2camera:camera_qbuf S addr cap a %d,%x,%x\n", index, (unsigned int)userptr, camera_module_type->mCaptureInfo.pBufPhy_Stat[index]);
            }

            camera_module_type->mCaptureInfo.pBufOwner[index] = OWN_BY_CLITOMOD;
            camera_module_type->mCaptureInfo.nTimeStamp[index] = camera_module_type->nCaptureSeq;
            camera_module_type->nCaptureSeq++;
        }
    }

    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    V4L2DBUG(V4L2DBUG_VERB, "qbuf out\n");
    return ret;
}

static int camera_dqbuf(void *handle, int *index, unsigned long *userptr, unsigned long *resver, long long *timestamp, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    wdog_cb_t *omx_wdog_cb = &camera_module->omx_wdog_cb;
    char cb_info[128];
    int ret = 0;
    
    V4L2DBUG(V4L2DBUG_VERB, "dqbuf: %d, %d, %d\n", work_mode, camera_module_type->mwork_mode, camera_module_type->bStreaming);
    
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
        ret = StandModule->mod_dqbuf(StandModule, index, userptr, resver, timestamp);
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
                if(NULL != omx_wdog_cb->handle)
                {
                    sprintf(cb_info, "%s %d", __func__, __LINE__);
                    omx_wdog_cb->func(omx_wdog_cb->handle, cb_info);
                }
                IPP_DQbuf(camera_module_type, OMX_PREVIEW_MODE, *index);
                if(NULL != omx_wdog_cb->handle)
                {
                    sprintf(cb_info, "%s %d", __func__, __LINE__);
                    omx_wdog_cb->func(omx_wdog_cb->handle, cb_info);
                }
                
                //*index =find_index(camera_module_type->mPreviewInfo.pBufIndex,32,*index);
                V4L2DBUG(V4L2DBUG_VERB, "dqbuf: %x, %d\n", *userptr, *index);
                camera_module_type->mPreviewInfo.pBufOwner[*index] = OWN_BY_CLIENT;

                if(camera_module_type->bAdjustFlag)
                {
                    camera_module_type->bAdjustFlag = 0;
                    camera_module_type->nOffset_Time = (camera_module_type->nCurrent_Time - *timestamp + 100000);
                    V4L2DBUG(V4L2DBUG_PARAM, "Adjust Time %lld,%lld,%lld\n", camera_module_type->nOffset_Time, camera_module_type->nCurrent_Time, *timestamp);
                    *timestamp += camera_module_type->nOffset_Time;
                }
                else
                {
                    *timestamp += camera_module_type->nOffset_Time;

                    if(*timestamp < camera_module_type->nCurrent_Time)
                    {
                        V4L2DBUG(V4L2DBUG_PARAM, "Adjust Time err %lld,%lld\n", *timestamp, camera_module_type->nCurrent_Time);
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
                int buflen = camera_module_type->mCaptureInfo.nh * camera_module_type->mCaptureInfo.nw * 3 / 2 + 4096;

                if(NULL != omx_wdog_cb->handle)
                {
                    sprintf(cb_info, "%s %d", __func__, __LINE__);
                    omx_wdog_cb->func(omx_wdog_cb->handle, cb_info);
                }
                IPP_DQbuf(camera_module_type, OMX_CAPTURE_MODE, *index);
                if(NULL != omx_wdog_cb->handle)
                {
                    sprintf(cb_info, "%s %d", __func__, __LINE__);
                    omx_wdog_cb->func(omx_wdog_cb->handle, cb_info);
                }

                if(camera_module_type->hdr_mode == 1)
                {
                    V4L2DBUG(V4L2DBUG_PARAM, "cpnum cp %x,%x\n", camera_module_type->cpnum, *userptr);
                    
                    if(camera_module_type->cpnum == 0)
                    {
                        camera_module_type->hdr_exp[camera_module_type->cpnum % 3] = camera_module_type->exp_time;
                        camera_module_type->hdr_gain[camera_module_type->cpnum % 3] = camera_module_type->exp_gain;
                        camera_module_type->hdr_ev_info[camera_module_type->cpnum % 3] = camera_module_type->exp_time * camera_module_type->exp_gain;

                        if((camera_module_type->cpnum % 3) == 0)
                        {
                            ;
                        }
                        else if((camera_module_type->cpnum % 3) == 1)
                        {
                            ;
                        }

                        camera_module_type->cpnum++;
                    }
                    else if(camera_module_type->cpnum == 1)
                    {
                        camera_module_type->hdr_exp[camera_module_type->cpnum % 3] = camera_module_type->exp_time;
                        camera_module_type->hdr_gain[camera_module_type->cpnum % 3] = camera_module_type->exp_gain;
                        camera_module_type->hdr_ev_info[camera_module_type->cpnum % 3] = camera_module_type->exp_time * camera_module_type->exp_gain;

                        if((camera_module_type->cpnum % 3) == 0)
                        {
                            ;
                        }
                        else if((camera_module_type->cpnum % 3) == 1)
                        {
                            ;
                        }

                        pPhyAddr = *userptr;
                        nBufIdx = *index;
                        StandModule->mod_qbuf(StandModule, nBufIdx, buflen, pPhyAddr, 0);
                        StandModule->mod_dqbuf(StandModule, index, userptr, resver, timestamp);
                        pPhyAddr = *userptr;
                        nBufIdx = *index;
                        camera_module_type->cpnum++;
                    }
                    else if(camera_module_type->cpnum == 2)
                    {
                        camera_module_type->hdr_ev_info[camera_module_type->cpnum % 3] = camera_module_type->exp_time * camera_module_type->exp_gain;
                        
                        pPhyAddr = *userptr;
                        nBufIdx = *index;
                        StandModule->mod_qbuf(StandModule, nBufIdx, buflen, pPhyAddr, 0);
                        StandModule->mod_dqbuf(StandModule, index, userptr, resver, timestamp);
                        pPhyAddr = *userptr;
                        nBufIdx = *index;
                        camera_module_type->cpnum++;
                    }
                }

                *index = find_index(camera_module_type->mCaptureInfo.pBufIndex, 32, *index);
                camera_module_type->mCaptureInfo.pBufOwner[*index] = OWN_BY_CLIENT;
            }

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
    
    V4L2DBUG(V4L2DBUG_VERB, "dqbuf return: %i,%x\n", work_mode, (unsigned int)*userptr);
    return ret;
}

static int camera_streamon(void *handle, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
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
            //stream on preview again
            int i, j;
            int64_t tmp = 0;
            int start_idx = 0;
            int module_num = 0;
            //      int nStartNum = 0;
            int buflen = camera_module_type->mPreviewInfo.nh * camera_module_type->mPreviewInfo.nw * 3 / 2 + 4096;
            act_isp_video_info_t isp_video_info;
            
            //camera_module_type->eprev_status = CAM_PREVIEW_RUNNING;
            
            isp_video_info.width = camera_module_type->preview_width;
            isp_video_info.height = camera_module_type->preview_height;
            isp_video_info.framerate = camera_module_type->preview_fps;
            isp_video_info.work_mode = work_mode;
            ret = camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_VIDEOINFO, &isp_video_info);
            if(ret != 0)
            {
                V4L2DBUG(V4L2DBUG_ERR, "err! ACT_ISP_S_VIDEOINFO failed!\n");
                return ret;
            }

            ret = StandModule->mod_set_res(StandModule, camera_module_type->preview_width, camera_module_type->preview_height, camera_module_type->preview_fmt);
            ret |= StandModule->mod_set_crop(StandModule, 0, 0, camera_module_type->preview_width, camera_module_type->preview_height);
            
//	            ret |= StandModule->mod_set_framerate(StandModule,camera_module_type->preview_fps);
//	            ret |= StandModule->mod_requestbuf(StandModule,(unsigned int*)&camera_module_type->mPreviewInfo.nbufNum);
//	            V4L2DBUG(V4L2DBUG_ERR,"E prev reqbuf %x\n",camera_module_type->mPreviewInfo.nbufNum);
//	            tmp = 0x7FFFFFFFFFFFFFFFLL;
//	
//	            for(i = 0; i < camera_module_type->mPreviewInfo.nbufNum; i++)
//	            {
//	                if(camera_module_type->mPreviewInfo.pBufOwner[i] != OWN_BY_CLIENT)
//	                {
//	                    if(tmp > camera_module_type->mPreviewInfo.nTimeStamp[i])
//	                    {
//	                        tmp = camera_module_type->mPreviewInfo.nTimeStamp[i];
//	                        start_idx = i;
//	                    }
//	                    V4L2DBUG(V4L2DBUG_ERR,"Type %d,nTimeStamp %lld\n",camera_module_type->mPreviewInfo.pBufOwner[i],camera_module_type->mPreviewInfo.nTimeStamp[i]);
//	                    module_num++;
//	                }
//	            }
//	            V4L2DBUG(V4L2DBUG_ERR,"module_num %d\n",module_num);
//	
//	            for(j = 0; j <module_num; j++ ){
//	                for(i = 0 ; i < camera_module_type->mPreviewInfo.nbufNum; i++)
//	                {
//	                    if(camera_module_type->mPreviewInfo.pBufOwner[i] != OWN_BY_CLIENT && camera_module_type->mPreviewInfo.nTimeStamp[i] == tmp)
//	                    {
//	                        V4L2DBUG(V4L2DBUG_ERR,"reqbuf a %x %x,%lld\n",i,camera_module_type->mPreviewInfo.pBufPhy[i],camera_module_type->mPreviewInfo.nTimeStamp[i]);
//	                        ret |= StandModule->mod_querybuf(StandModule,i,buflen,camera_module_type->mPreviewInfo.pBufPhy[i],camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
//	                        ret |= StandModule->mod_qbuf(StandModule,i,buflen,camera_module_type->mPreviewInfo.pBufPhy[i],camera_module_type->mPreviewInfo.pBufPhy_Stat[i]);
//	                        camera_module_type->mPreviewInfo.pBufOwner[i] == OWN_BY_MODULE;
//	                        i = camera_module_type->mPreviewInfo.nbufNum + 1;
//	                    }
//	                }
//	                tmp++;
//	            }
            
            restore_preview_bufq(camera_module_type);
            camera_module_type->eprev_status == CAM_PREVIEW_PAUSE;
        }

        if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
        {
            //camera_streamoff(handle,OMX_PREVIEW_MODE);
            V4L2DBUG(V4L2DBUG_PARAM, "already on\n");
            pthread_mutex_unlock(&camera_module_type->camera_mutex);
            return 0;
        }
        else if(camera_module_type->ecap_status == CAM_PREVIEW_RUNNING)
        {
            V4L2DBUG(V4L2DBUG_VERB, "ecap_status, %d\n", __LINE__);

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
            V4L2DBUG(V4L2DBUG_PARAM, "camera_preview time %lld,%d\n", camera_module_type->nOffset_Time, camera_module_type->bAdjustFlag);
        }

//	        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_START, NULL);
        camera_module_type->eprev_status = CAM_PREVIEW_RUNNING;
    }
    else
    {
        if(camera_module_type->eprev_status == CAM_PREVIEW_RUNNING)
        {
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

        ; //set ev
//	        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_START, NULL);
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
            ;
        }

        if(camera_module_type->awb_lock == OMX_IMAGE_LockAtCapture)
        {
            ;
        }

        if(camera_module_type->af_lock == OMX_IMAGE_LockAtCapture)
        {
            ;
        }

//	        V4L2DBUG(V4L2DBUG_ERR, "stream On V4L2_CID_PRIVATE_PREV_CAPT");
//	        ctl_id = V4L2_CID_PRIVATE_PREV_CAPT;
//	        ctl_val = OMX_PREVIEW_MODE;
//	        ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
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
            ;
        }

        if(camera_module_type->awb_lock == OMX_IMAGE_LockAtCapture)
        {
            ;
        }

        if(camera_module_type->af_lock == OMX_IMAGE_LockAtCapture)
        {
            ;
        }
        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_AF_STOP, 0); 

//	        V4L2DBUG(V4L2DBUG_ERR, "stream on V4L2_CID_PRIVATE_PREV_CAPT");
//	        ctl_id = V4L2_CID_PRIVATE_PREV_CAPT;
//	        ctl_val = OMX_CAPTURE_MODE;
//	        ret |= StandModule->mod_set_ctl(StandModule, ctl_id, ctl_val);
    }

    if(camera_module_type->bStreaming == 0)
    {
        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_START, NULL);
        ret |= StandModule->mod_streamon(StandModule);
    }

    V4L2DBUG(V4L2DBUG_PARAM, "stream on done\n");
    camera_module_type->bStreaming = 1;
    camera_module_type->mwork_mode = work_mode;
    
    pthread_mutex_unlock(&camera_module_type->camera_mutex);
    
    return ret;
}

static int camera_streamoff(void *handle, int work_mode)
{
    OMX_CAMERATYPE *camera_module = (OMX_CAMERATYPE *)handle;
    camera_isp_base_PrivateType *camera_module_type = (camera_isp_base_PrivateType *)camera_module->pCameraPrivate;
    CAMERA_MODULETYPE *StandModule = camera_module_type->StandModule;
    int i = 0;
    int ret = 0;
    
    pthread_mutex_lock(&camera_module_type->camera_mutex);

    V4L2DBUG(V4L2DBUG_VERB, "%d, %d, %d, %d", work_mode, camera_module_type->eprev_status, 
                camera_module_type->ecap_status, camera_module_type->bStreaming);

    if(work_mode == OMX_CAPTURE_MODE)
    {
        camera_module_type->ecap_status = CAM_IDLE;
//	        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_STOP, NULL);

        for(i = 0; i < camera_module_type->mCaptureInfo.nbufNum; i++)
        {
            //ext_put_vir_addr(camera_module_type->mCaptureInfo.pBufVir_Stat[i],4096);
            camera_module_type->mCaptureInfo.pBufVir_Stat[i] = 0;
            camera_module_type->mCaptureInfo.pBufOwner[i] = OWN_BY_CLIENT;
            camera_module_type->mCaptureInfo.pBufPhy[i] = 0;
            camera_module_type->mCaptureInfo.pBufFlag[i] = 0;
        }

        camera_module_type->mCaptureInfo.nbufNum = 0;
        camera_module_type->nCaptureSeq = 0;
        camera_module_type->cur_capture_num = 0;
        camera_module_type->cpnum = 0;
    }
    else if(work_mode == OMX_PREVIEW_MODE)
    {
        for(i = 0; i < camera_module_type->mPreviewInfo.nbufNum; i++)
        {
            //ext_put_vir_addr(camera_module_type->mPreviewInfo.pBufVir_Stat[i],4096);
            camera_module_type->mPreviewInfo.pBufVir_Stat[i] = 0;
            camera_module_type->mPreviewInfo.pBufOwner[i] = OWN_BY_CLIENT;
            camera_module_type->mPreviewInfo.pBufPhy[i] = 0;
            camera_module_type->mPreviewInfo.pBufFlag[i] = 0;
        }

        camera_module_type->eprev_status = CAM_IDLE;
//	        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_STOP, NULL);

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
            }
        }

        camera_module_type->ecap_status = CAM_IDLE;
//	        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_STOP, NULL);
        
        camera_module_type->nCaptureSeq = 0;
        camera_module_type->cpnum = 0;

        for(i = 0; i < camera_module_type->mCaptureInfo.nbufNum; i++)
        {
            //ext_put_vir_addr(camera_module_type->mCaptureInfo.pBufVir_Stat[i],4096);
            camera_module_type->mCaptureInfo.pBufVir_Stat[i] = 0;
            camera_module_type->mCaptureInfo.pBufOwner[i] = OWN_BY_CLIENT;
            camera_module_type->mCaptureInfo.pBufPhy[i] = 0;
            camera_module_type->mCaptureInfo.pBufFlag[i] = 0;
        }
    }
    else
    {
        camera_module_type->eprev_status = CAM_PREVIEW_PAUSE;
        camera_module_type->bkTimeStamp = get_current_time();
        
//	        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_STOP, NULL);
    }

    if(camera_module_type->bStreaming == 1)
    {
        ret = StandModule->mod_streamoff(StandModule);
        camera_module_type->ipp_handle->act_isp_cmd(camera_module_type->ipp_handle, ACT_ISP_S_STOP, NULL);
        
        camera_module_type->bStreaming = 0;
        V4L2DBUG(V4L2DBUG_ERR, "raw stream off Ok\n");
    }

#if 1
    if(camera_module_type->eprev_status == CAM_PREVIEW_PAUSE && work_mode == OMX_PREVIEW_CAP)
    {
        camera_module_type->eprev_status = CAM_PREVIEW_READY;
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
    }
#endif

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

int camera_isp_base_Constructor(OMX_CAMERATYPE *omx_camera)
{
    int ret = 0;
    int i;
    camera_isp_base_PrivateType *camera_direct = (camera_isp_base_PrivateType *)malloc(sizeof(camera_isp_base_PrivateType));
    if(camera_direct == NULL)
    {
        return -1;
    }
    memset(camera_direct, 0, sizeof(camera_isp_base_PrivateType));
    
    V4L2DBUG(V4L2DBUG_PARAM, "camera_isp_base_Constructor In\n");
    
    camera_direct->StandModule = (CAMERA_MODULETYPE *)malloc(sizeof(CAMERA_MODULETYPE));
    if(camera_direct->StandModule == NULL)
    {
        return -1;
    }
    memset(camera_direct->StandModule, 0, sizeof(CAMERA_MODULETYPE));
    
    memcpy(camera_direct->module_name, "raw-module", 10);
    
    //isp lib opened by component

    ret = Camera_Module_constructer(camera_direct->StandModule);
    camera_direct->ipp_handle = (ispctl_handle_t *)omx_camera->pIppHnale;

    if(camera_direct->ipp_handle == NULL)
    {
        V4L2DBUG(V4L2DBUG_ERR, "IPP Handle is Null....\n");
    }

    omx_camera->pCameraPrivate = camera_direct;
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

int camera_isp_base_Destructor(OMX_CAMERATYPE *omx_camera)
{
    int ret = 0;
    camera_isp_base_PrivateType *camera_direct = (camera_isp_base_PrivateType *)omx_camera->pCameraPrivate;

    if(camera_direct)
    {
        //isp lib closed by component
#if 1
        int i = 0;

        for(i = 0; i < camera_direct->mCaptureInfo.nbufNum; i++)
        {
            if(camera_direct->mCaptureInfo.pBufVir[i])
            {
                ext_put_vir_addr(camera_direct->mCaptureInfo.pBufVir[i], camera_direct->mCaptureInfo.nPreVSize);
                camera_direct->mCaptureInfo.pBufVir[i] = 0;
            }

            if(camera_direct->mCaptureInfo.pBufVir_Stat[i])
            {
                ext_put_vir_addr(camera_direct->mCaptureInfo.pBufVir_Stat[i], 4096);
                camera_direct->mCaptureInfo.pBufVir_Stat[i] = 0;
            }
        }

        for(i = 0; i < camera_direct->mPreviewInfo.nbufNum; i++)
        {
            if(camera_direct->mPreviewInfo.pBufVir[i])
            {
                ext_put_vir_addr(camera_direct->mPreviewInfo.pBufVir[i], camera_direct->mPreviewInfo.nPreVSize);
                camera_direct->mPreviewInfo.pBufVir[i] = 0;
            }

            if(camera_direct->mPreviewInfo.pBufVir_Stat[i])
            {
                ext_put_vir_addr(camera_direct->mPreviewInfo.pBufVir_Stat[i], 4096);
                camera_direct->mPreviewInfo.pBufVir_Stat[i] = 0;
            }

        }
#endif
        ret = Camera_Module_destructer(camera_direct->StandModule);
        pthread_mutex_destroy(&camera_direct->camera_mutex);
        free(camera_direct);
    }

    return ret;
}

#ifdef __cplusplus
}
#endif
