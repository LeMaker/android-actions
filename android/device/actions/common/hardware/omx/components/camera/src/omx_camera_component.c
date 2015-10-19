/**
  src/components/videoscheduler/omx_video_scheduler_component.c

  This component implements a video scheduler

  Copyright (C) 2008-2009 STMicroelectronics
  Copyright (C) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <pthread.h>        /* pthread_cond_*, pthread_mutex_* */
#include <sys/prctl.h>
#include <linux/videodev2.h>
#include <Igralloc.h>
#include <cutils/properties.h>
#include "ext_mem.h"
#include "omx_camera_component.h"
#include "video_mediadata.h"
#include "ispctl_adapter.h"
#include "imxapi_adapter.h"
#include "isp_ctl.h"
#include "actim_hdr.h"
#include "perf.h"
#include "log.h"
#include "watch_dog.h"
#include "omx_callback.h"


#define DEFAULT_WIDTH   640
#define DEFAULT_HEIGHT  480
/** define the max input buffer size */
#define DEFAULT_VIDEO_INPUT_BUF_SIZE DEFAULT_WIDTH*DEFAULT_HEIGHT*3/2
//#define ALLOC_FROM_USR
#define HDR_PROCESS 0

static int omx_coloridx[4] =
{
    OMX_COLOR_FormatYUV420SemiPlanar,
    OMX_COLOR_FormatYUV420Planar,
    OMX_COLOR_FormatYUV422SemiPlanar,
    OMX_COLOR_FormatYUV422Planar,
};

/*
 * 减少一次open/close的操作
 * */
#define GMAX_MODULES 3
static int gomxcam_bIsInited = -1;
static int gomxcam_bDoneQuery[GMAX_MODULES];
static OMX_ACT_CAPTYPE gomxcam_OMX_CAPS[GMAX_MODULES];

//#define DBUG_CAMDATA
static FILE *fraw = NULL;
static FILE *fraw_pv = NULL;
static FILE *fraw_cap = NULL;
static FILE *fraw_info = NULL;

static void inv_memcpy(void *src, void *dst, int len)
{
    memcpy(dst, src, len);
}

static int get_vir_addr(void *pbuffhandle, void **vaddr)
{
    int w, h, fmt, ss;
    int ret = 0;
    Actions_OSAL_GetBufInfo(pbuffhandle, &w, &h, &fmt, &ss);
    ret = Actions_OSAL_LockANBHandleWidthUsage(pbuffhandle, w, h, 2, vaddr);
    OMXDBUG(OMXDBUG_ERR, "get vaddr %x,%d", vaddr, ret);
    Actions_OSAL_UnlockANBHandle(pbuffhandle);
    return ret;
}

static int get_vir_addr_abs(void *pbuffhandle, void **vaddr)
{
    int w, h, fmt, ss;
    int ret = 0;
    Actions_OSAL_GetBufInfo(pbuffhandle, &w, &h, &fmt, &ss);
    ret = Actions_OSAL_LockANBHandleWidthUsage(pbuffhandle, w, h, 1, vaddr);
    OMXDBUG(OMXDBUG_ERR, "get vaddr %x,%d", vaddr, ret);
    return ret;
}

static void *get_vir_addr_mmap(OMX_U8 *pBuffer, int usage)
{
    video_metadata_t *pbuff = (video_metadata_t *)(pBuffer);
    void *vir_addr = NULL;

    if(IGralloc_lock(pbuff->handle, usage, &vir_addr))
    {
        OMXDBUG(OMXDBUG_ERR, "lock buffer failed\n");
        return NULL;
    }

    return vir_addr;
}

static int free_vir_addr_mmap(OMX_U8 *pBuffer)
{
    video_metadata_t *pbuff = (video_metadata_t *)(pBuffer);

    if(IGralloc_unlock(pbuff->handle))
    {
        OMXDBUG(OMXDBUG_ERR, "unlock buffer failed\n");
        return OMX_ErrorUndefined;
    }

    return 0;
}

static int get_input_res(OMX_COMPONENTTYPE *hComponent, int w, int h, int *oW, int *oH, int mode)
{
    OMX_COMPONENTTYPE                 *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_camera_PrivateType          *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    omx_base_camera_video_PortType *pPort = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[0];
    //omx_base_camera_video_PortType *pPort_img = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[1];
    int nPreviewRes = pPort->pCapInfo->ulPreviewResCount - pPort->pCapInfo->tPreviewRes[pPort->pCapInfo->ulPreviewResCount].nWidth;
    int nCaptureRes = pPort->pCapInfo->ulImageResCount - pPort->pCapInfo->tImageRes[pPort->pCapInfo->ulImageResCount].nWidth;
    int bDone = 0, i;

    *oW = 0;
    *oH = 0;
    OMXDBUG(OMXDBUG_PARAM, "in %s: %d,%d,%dx%d\n", __func__, pPort->pCapInfo->ulPreviewResCount, nPreviewRes, w, h);

    if(mode) // capture
    {
        for(i = 0; i < nCaptureRes; i++)
        {
            if(pPort->pCapInfo->tImageRes[i].nWidth == w && pPort->pCapInfo->tImageRes[i].nHeight == h)
            {
                *oW = w;
                *oH = h;
                bDone = 1;
                return 0;
            }
        }
    }

    //check if has
    for(i = 0; i < nPreviewRes; i++)
    {
        if(pPort->pCapInfo->tPreviewRes[i].nWidth == w && pPort->pCapInfo->tPreviewRes[i].nHeight == h)
        {
            *oW = w;
            *oH = h;
            bDone = 1;
            return 0;
        }
    }

    // select the one which w&h are both larger and is the nearest to the target
    for(i = 0; i < nPreviewRes; i++)
    {
        if(pPort->pCapInfo->tPreviewRes[i].nWidth >= w && pPort->pCapInfo->tPreviewRes[i].nHeight >= h && bDone == 0)
        {
            *oW = pPort->pCapInfo->tPreviewRes[i].nWidth;
            *oH = pPort->pCapInfo->tPreviewRes[i].nHeight;
            bDone = 1;
        }
        else if(pPort->pCapInfo->tPreviewRes[i].nWidth >= w && pPort->pCapInfo->tPreviewRes[i].nHeight >= h && bDone == 1)
        {
            if(*oW >= pPort->pCapInfo->tPreviewRes[i].nWidth && *oH >= pPort->pCapInfo->tPreviewRes[i].nHeight)
            {
                *oW = pPort->pCapInfo->tPreviewRes[i].nWidth;
                *oH = pPort->pCapInfo->tPreviewRes[i].nHeight;
            }
        }
    }

    OMXDBUG(OMXDBUG_PARAM, "in %s, nPreviewRes out %d,%d\n", __func__, *oW, *oH);

    if(bDone == 1) { return 1; }

    return -1;
}

static int GetISPLibAndHandle(omx_camera_PrivateType *omx_camera_component_Private)
{
    int ret = 0;

    omx_camera_component_Private->ispctl_handle = ispctl_open();
    if(NULL == omx_camera_component_Private->ispctl_handle) ret = -1;
    
    return ret;
}

static OMX_ERRORTYPE encFreeISPLibAndHandle(omx_camera_PrivateType   *omx_camera_component_Private)
{
    return ispctl_close(omx_camera_component_Private->ispctl_handle);
}

static int GetIMXLibAndHandle(omx_camera_PrivateType *omx_camera_component_Private)
{
    int ret = 0;

    omx_camera_component_Private->imxctl_handle = imxctl_open();
    if(NULL == omx_camera_component_Private->imxctl_handle) ret = -1;
    
    return ret;
}

static OMX_ERRORTYPE encFreeIMXLibAndHandle(omx_camera_PrivateType   *omx_camera_component_Private)
{
    return imxctl_close(omx_camera_component_Private->imxctl_handle);
}

static int GetV4L2LibAndHandle(omx_camera_PrivateType   *omx_filter_Private)
{
    int ret = 0;
    void *pCodaModule = NULL;
    const char *pErr = dlerror();
    char buf[50] = "libACT_V4L2HAL.so";

    pCodaModule = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);

    if(pCodaModule == NULL)
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "dlopen %s failed because %s\n", buf, dlerror());
#endif
        ret = OMX_ErrorComponentNotFound;
        goto UNLOCK_MUTEX;
    }


    omx_filter_Private->camera_direct_base_Constructor = dlsym(pCodaModule, "camera_direct_base_Constructor");

    //pErr = dlerror();
    if((omx_filter_Private->camera_direct_base_Constructor == NULL))
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
#endif
        ret = OMX_ErrorInvalidComponent;
        goto CLEAN_UP;
    }

    omx_filter_Private->camera_direct_base_Destructor = dlsym(pCodaModule, "camera_direct_base_Destructor");

    //pErr = dlerror();
    if((omx_filter_Private->camera_direct_base_Destructor == NULL))
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
#endif
        ret = OMX_ErrorInvalidComponent;
        goto CLEAN_UP;
    }

    ///isp
    omx_filter_Private->camera_isp_base_Constructor = dlsym(pCodaModule, "camera_isp_base_Constructor");

    //pErr = dlerror();
    if((omx_filter_Private->camera_isp_base_Constructor == NULL))
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, omx_filter_Private->camera_isp_base_Constructor);
#endif
        ret = OMX_ErrorInvalidComponent;
        goto CLEAN_UP;
    }

    omx_filter_Private->camera_isp_base_Destructor = dlsym(pCodaModule, "camera_isp_base_Destructor");

    //pErr = dlerror();
    if((omx_filter_Private->camera_isp_base_Destructor == NULL))
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
#endif
        ret = OMX_ErrorInvalidComponent;
        goto CLEAN_UP;
    }

    omx_filter_Private->camera_module_query = dlsym(pCodaModule, "camera_module_query");

    //pErr = dlerror();
    if((omx_filter_Private->camera_module_query == NULL))
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
#endif
        ret = OMX_ErrorInvalidComponent;
        goto CLEAN_UP;
    }

    omx_filter_Private->camera_module_release = dlsym(pCodaModule, "camera_module_release");

    //pErr = dlerror();
    if((omx_filter_Private->camera_module_release == NULL))
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
#endif
        ret = OMX_ErrorInvalidComponent;
        goto CLEAN_UP;
    }

UNLOCK_MUTEX:
    omx_filter_Private->ModuleLib = pCodaModule;
    return ret;

CLEAN_UP:
    OMXDBUG(OMXDBUG_ERR, "Base Filter ISP err\n");
    dlclose(pCodaModule);
    pCodaModule = NULL;
    return ret;

}

static OMX_ERRORTYPE encFreeV4L2LibAndHandle(omx_camera_PrivateType   *omx_filter_Private)
{
    int ret = 0;

    if(omx_filter_Private->ModuleLib)
    {
        dlclose(omx_filter_Private->ModuleLib);
        omx_filter_Private->ModuleLib = NULL;
    }

    return ret;

}

static int GetHDRLibAndHandle(omx_camera_PrivateType   *omx_filter_Private)
{
    omx_base_camera_video_PortType *pPort = (omx_base_camera_video_PortType *)omx_filter_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
    int ret = 0;
    void *pCodaModule = NULL;
    const char *pErr = dlerror();
    char buf[50] = "libhdri.so";

    pCodaModule = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);

    if(pCodaModule == NULL)
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "dlopen %s failed because %s\n", buf, dlerror());
#endif
        ret = OMX_ErrorComponentNotFound;
        goto UNLOCK_MUTEX;
    }

    pPort->openHDRI = dlsym(pCodaModule, "openHDRI");

    //pErr = dlerror();
    if((pPort->openHDRI == NULL))
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "GetHDRLib: dlsym failed for module %p %s\n", pCodaModule, dlerror());
#endif
        ret = OMX_ErrorInvalidComponent;
        goto CLEAN_UP;
    }

UNLOCK_MUTEX:
    omx_filter_Private->HDRILib = pCodaModule;
    return ret;

CLEAN_UP:
    OMXDBUG(OMXDBUG_ERR, "Base Filter HDRI err\n");
    dlclose(pCodaModule);
    pCodaModule = NULL;
    return ret;
}

static OMX_ERRORTYPE encFreeHDRILibAndHandle(omx_camera_PrivateType   *omx_filter_Private)
{
    int ret = 0;

    if(omx_filter_Private->HDRILib)
    {
        dlclose(omx_filter_Private->HDRILib);
        omx_filter_Private->HDRILib = NULL;
    }

    return ret;

}

static int GetMJPGLibAndHandle(omx_camera_PrivateType   *omx_filter_Private)
{
    int ret = 0;
    void *pCodaModule = NULL;
    const char *pErr = dlerror();
    char buf[50] = "vd_mjpg.so";
    void *(*func_handle)(void);
    //videodec_plugin_t *vdec_plugn;

    pCodaModule = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);

    if(pCodaModule == NULL)
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "dlopen %s failed because %s\n", buf, dlerror());
#endif
        ret = OMX_ErrorComponentNotFound;
        goto UNLOCK_MUTEX;
    }

    func_handle = dlsym(pCodaModule, "get_plugin_info");

    //pErr = dlerror();
    if((func_handle == NULL))
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
#endif
        ret = OMX_ErrorInvalidComponent;
        goto CLEAN_UP;
    }

    omx_filter_Private->vdec_plugn = (videodec_plugin_t *)func_handle();

    //pErr = dlerror();
    if((omx_filter_Private->vdec_plugn == NULL))
    {
#if 1//ACTION_OMX_CODA_DEBUG
        OMXDBUG(OMXDBUG_ERR, "GetCodaLibAndHandle: dlsym failed for module %p %s\n", pCodaModule, dlerror());
#endif
        ret = OMX_ErrorInvalidComponent;
        goto CLEAN_UP;
    }

    omx_filter_Private->vdec_handle = omx_filter_Private->vdec_plugn->open(NULL, NULL, NULL);
    OMXDBUG(OMXDBUG_PARAM,"vdec_handle->open %x,%x\n",omx_filter_Private->vdec_plugn,omx_filter_Private->vdec_handle);
    
UNLOCK_MUTEX:
    omx_filter_Private->vdec_lib = pCodaModule;
    return ret;

CLEAN_UP:
    OMXDBUG(OMXDBUG_ERR, "Base Filter ISP err\n");
    dlclose(pCodaModule);
    pCodaModule = NULL;
    return ret;

}

static OMX_ERRORTYPE encFreeMJPGLibAndHandle(omx_camera_PrivateType   *omx_filter_Private)
{
    int ret = 0;

    OMXDBUG(OMXDBUG_PARAM,"vdec_handle->close %x,%x,%x\n",omx_filter_Private->vdec_plugn,omx_filter_Private->vdec_handle,omx_filter_Private->vdec_plugn->dispose);
    
    if(omx_filter_Private->vdec_plugn && omx_filter_Private->vdec_handle)
    {
        omx_filter_Private->vdec_plugn->dispose(omx_filter_Private->vdec_handle);
        omx_filter_Private->vdec_plugn = NULL;
    }

    if(omx_filter_Private->vdec_lib)
    {
        dlclose(omx_filter_Private->vdec_lib);
        omx_filter_Private->vdec_lib = NULL;
    }

    return ret;

}

static void Module_CAPInit(OMX_ACT_CAPTYPE *pCapInfo, int portIndex)
{
    pCapInfo->bBrightnessSupported     = OMX_TRUE;
    pCapInfo->bContrastSupported       = OMX_FALSE;
    pCapInfo->bISONoiseFilterSupported = OMX_FALSE;
    pCapInfo->bSaturationSupported     = OMX_FALSE;
    pCapInfo->bWhiteBalanceLockSupported = OMX_FALSE;
    pCapInfo->bExposureLockSupported     = OMX_FALSE;
    pCapInfo->bFocusLockSupported     = OMX_FALSE;
    pCapInfo->nSensorIndex = portIndex;

    pCapInfo->ulPreviewFormatCount = 1;   // supported preview pixelformat count
    pCapInfo->ePreviewFormats[0] = OMX_COLOR_FormatYUV420SemiPlanar;
    //pCapInfo->ePreviewFormats[1] = OMX_COLOR_FormatYUV420Planar;

    pCapInfo->ulImageFormatCount = 1;     // supported image pixelformat count
    pCapInfo->eImageFormats[0] = OMX_COLOR_FormatYUV420SemiPlanar;
    //pCapInfo->eImageFormats[1] = OMX_COLOR_FormatYUV420Planar;

    pCapInfo->ulPreviewResCount = 0;
    memset(pCapInfo->tPreviewRes, 0, sizeof(OMX_ACT_VARRESTYPE) * 8);
    pCapInfo->ulThumbResCount = 0;
    memset(pCapInfo->tThumbRes, 0, sizeof(OMX_ACT_VARRESTYPE) * 4);
    pCapInfo->ulImageResCount = 0;
    memset(pCapInfo->tImageRes, 0, sizeof(OMX_ACT_VARRESTYPE) * 64);

    pCapInfo->ulWhiteBalanceCount = 0;    // supported whitebalance mode count
    pCapInfo->eWhiteBalanceModes[0] = OMX_WhiteBalControlOff;

    pCapInfo->ulColorEffectCount = 0;     // supported effects count
    pCapInfo->eColorEffects[0] = OMX_ImageFilterNone;
    //omx_filter_Private->pCapInfo[portIndex]->eColorEffects[1] = OMX_ImageFilterNegative;

    pCapInfo->xMaxWidthZoom = 0;          // Fixed point value stored as Q16
    pCapInfo->xMaxHeightZoom = 0;         // Fixed point value stored as Q16

    pCapInfo->ulFlickerCount = 0;         // supported anti-flicker mode count
    pCapInfo->eFlicker[0] = OMX_FlickerRejectionOff;
    //omx_filter_Private->pCapInfo[portIndex]->eFlicker[1] = OMX_FlickerRejection50;
    //omx_filter_Private->pCapInfo[portIndex]->eFlicker[2] = OMX_FlickerRejection60;

    pCapInfo->ulExposureModeCount = 0;    // supported exposure mode count
    pCapInfo->eExposureModes[0] = OMX_ExposureControlOff;
    //omx_filter_Private->pCapInfo[portIndex]->eExposureModes[1] = OMX_ExposureControlAuto;

    pCapInfo->xEVCompensationMin = 0;     // Fixed point value stored as Q16
    pCapInfo->xEVCompensationMax = 0;     // Fixed point value stored as Q16
    pCapInfo->nSensitivityMax = 0;        // nSensitivityMax = 100 implies maximum supported equal to "ISO 100"

    pCapInfo->ulFocusModeCount = 0;       // supported focus mode count
    pCapInfo->eFocusModes[0] = OMX_IMAGE_FocusControlOff;
    //omx_filter_Private->pCapInfo[portIndex]->eFocusModes[1] = OMX_IMAGE_FocusControlAuto;
    //omx_filter_Private->pCapInfo[portIndex]->eFocusModes[2] = OMX_IMAGE_FocusControlOn;

    pCapInfo->ulFlashCount = 0;           // supported flash modes count

    pCapInfo->ulPrvVarFPSModesCount = 0;  // supported variable FPS preview modes count
    pCapInfo->tPrvVarFPSModes[0].nVarFPSMax = 25 << 16;
    pCapInfo->tPrvVarFPSModes[0].nVarFPSMin = 25 << 16;
    pCapInfo->tPrvVarFPSModes[1].nVarFPSMax = 30 << 16;
    pCapInfo->tPrvVarFPSModes[1].nVarFPSMin = 30 << 16;
    pCapInfo->ulCapVarFPSModesCount = 0;  // supported variable FPS capture modes count
    pCapInfo->tCapVarFPSModes[0].nVarFPSMin = 2 << 16;
    pCapInfo->tCapVarFPSModes[0].nVarFPSMax = 8 << 16;
    pCapInfo->ulAreasFocusCount = 1;  // supported number of AlgoAreas for focus areas

}
static void Module_Cap_Init(omx_camera_PrivateType *omx_filter_Private, int portIndex)
{
    omx_filter_Private->pCapInfo[portIndex]->bBrightnessSupported     = OMX_TRUE;
    omx_filter_Private->pCapInfo[portIndex]->bContrastSupported       = OMX_FALSE;
    omx_filter_Private->pCapInfo[portIndex]->bISONoiseFilterSupported = OMX_FALSE;
    omx_filter_Private->pCapInfo[portIndex]->bSaturationSupported     = OMX_FALSE;
    omx_filter_Private->pCapInfo[portIndex]->bWhiteBalanceLockSupported = OMX_FALSE;
    omx_filter_Private->pCapInfo[portIndex]->bExposureLockSupported     = OMX_FALSE;
    omx_filter_Private->pCapInfo[portIndex]->bFocusLockSupported     = OMX_FALSE;
    omx_filter_Private->pCapInfo[portIndex]->nSensorIndex = portIndex;

    omx_filter_Private->pCapInfo[portIndex]->ulPreviewFormatCount = 0;   // supported preview pixelformat count
    omx_filter_Private->pCapInfo[portIndex]->ePreviewFormats[0] = OMX_COLOR_FormatYUV420SemiPlanar;
    //omx_filter_Private->pCapInfo[portIndex]->ePreviewFormats[1] = OMX_COLOR_FormatYUV420Planar;

    omx_filter_Private->pCapInfo[portIndex]->ulImageFormatCount = 0;     // supported image pixelformat count
    omx_filter_Private->pCapInfo[portIndex]->eImageFormats[0] = OMX_COLOR_FormatYUV420SemiPlanar;
    //omx_filter_Private->pCapInfo[portIndex]->eImageFormats[1] = OMX_COLOR_FormatYUV420Planar;

    omx_filter_Private->pCapInfo[portIndex]->ulPreviewResCount = 0;
    memset(omx_filter_Private->pCapInfo[portIndex]->tPreviewRes, 0, sizeof(OMX_ACT_VARRESTYPE) * 8);
    omx_filter_Private->pCapInfo[portIndex]->ulThumbResCount = 0;
    memset(omx_filter_Private->pCapInfo[portIndex]->tThumbRes, 0, sizeof(OMX_ACT_VARRESTYPE) * 4);
    omx_filter_Private->pCapInfo[portIndex]->ulImageResCount = 0;
    memset(omx_filter_Private->pCapInfo[portIndex]->tImageRes, 0, sizeof(OMX_ACT_VARRESTYPE) * 64);

    omx_filter_Private->pCapInfo[portIndex]->ulWhiteBalanceCount = 0;    // supported whitebalance mode count
    omx_filter_Private->pCapInfo[portIndex]->eWhiteBalanceModes[0] = OMX_WhiteBalControlOff;
    //omx_filter_Private->pCapInfo[portIndex]->eWhiteBalanceModes[1] = OMX_WhiteBalControlAuto;
    //omx_filter_Private->pCapInfo[portIndex]->eWhiteBalanceModes[2] = OMX_WhiteBalControlSunLight;
    //omx_filter_Private->pCapInfo[portIndex]->eWhiteBalanceModes[3] = OMX_WhiteBalControlCloudy;
    //omx_filter_Private->pCapInfo[portIndex]->eWhiteBalanceModes[4] = OMX_WhiteBalControlFluorescent;

    omx_filter_Private->pCapInfo[portIndex]->ulColorEffectCount = 0;     // supported effects count
    omx_filter_Private->pCapInfo[portIndex]->eColorEffects[0] = OMX_ImageFilterNone;
    //omx_filter_Private->pCapInfo[portIndex]->eColorEffects[1] = OMX_ImageFilterNegative;

    omx_filter_Private->pCapInfo[portIndex]->xMaxWidthZoom = 0;          // Fixed point value stored as Q16
    omx_filter_Private->pCapInfo[portIndex]->xMaxHeightZoom = 0;         // Fixed point value stored as Q16

    omx_filter_Private->pCapInfo[portIndex]->ulFlickerCount = 0;         // supported anti-flicker mode count
    omx_filter_Private->pCapInfo[portIndex]->eFlicker[0] = OMX_FlickerRejectionOff;
    //omx_filter_Private->pCapInfo[portIndex]->eFlicker[1] = OMX_FlickerRejection50;
    //omx_filter_Private->pCapInfo[portIndex]->eFlicker[2] = OMX_FlickerRejection60;

    omx_filter_Private->pCapInfo[portIndex]->ulExposureModeCount = 0;    // supported exposure mode count
    omx_filter_Private->pCapInfo[portIndex]->eExposureModes[0] = OMX_ExposureControlOff;
    //omx_filter_Private->pCapInfo[portIndex]->eExposureModes[1] = OMX_ExposureControlAuto;

    omx_filter_Private->pCapInfo[portIndex]->xEVCompensationMin = 0;     // Fixed point value stored as Q16
    omx_filter_Private->pCapInfo[portIndex]->xEVCompensationMax = 0;     // Fixed point value stored as Q16
    omx_filter_Private->pCapInfo[portIndex]->nSensitivityMax = 0;        // nSensitivityMax = 100 implies maximum supported equal to "ISO 100"

    omx_filter_Private->pCapInfo[portIndex]->ulFocusModeCount = 0;       // supported focus mode count
    omx_filter_Private->pCapInfo[portIndex]->eFocusModes[0] = OMX_IMAGE_FocusControlOff;
    //omx_filter_Private->pCapInfo[portIndex]->eFocusModes[1] = OMX_IMAGE_FocusControlAuto;
    //omx_filter_Private->pCapInfo[portIndex]->eFocusModes[2] = OMX_IMAGE_FocusControlOn;

    omx_filter_Private->pCapInfo[portIndex]->ulFlashCount = 0;           // supported flash modes count

    omx_filter_Private->pCapInfo[portIndex]->ulPrvVarFPSModesCount = 0;  // supported variable FPS preview modes count
    omx_filter_Private->pCapInfo[portIndex]->tPrvVarFPSModes[0].nVarFPSMax = 25 << 16;
    omx_filter_Private->pCapInfo[portIndex]->tPrvVarFPSModes[0].nVarFPSMin = 25 << 16;
    omx_filter_Private->pCapInfo[portIndex]->tPrvVarFPSModes[1].nVarFPSMax = 30 << 16;
    omx_filter_Private->pCapInfo[portIndex]->tPrvVarFPSModes[1].nVarFPSMin = 30 << 16;
    omx_filter_Private->pCapInfo[portIndex]->ulCapVarFPSModesCount = 0;  // supported variable FPS capture modes count
    omx_filter_Private->pCapInfo[portIndex]->tCapVarFPSModes[0].nVarFPSMin = 2 << 16;
    omx_filter_Private->pCapInfo[portIndex]->tCapVarFPSModes[0].nVarFPSMax = 8 << 16;
    omx_filter_Private->pCapInfo[portIndex]->ulAreasFocusCount = 1;  // supported number of AlgoAreas for focus areas
}

static int SetCamParams(OMX_COMPONENTTYPE *hComponent, int portIndex)
{
    OMX_COMPONENTTYPE                 *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_camera_PrivateType          *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    omx_base_camera_video_PortType *pPort = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[portIndex];
    OMX_IMAGE_CONFIG_LOCKTYPE  pLockType;
    //OMX_INDEXTYPE nParamIndex = OMX_IndexParamPortDefinition;
    //OMX_PTR ComponentParameterStructure = (OMX_PTR)&pPort->sPortParam;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    OMX_CONF_INIT_STRUCT(&pLockType, OMX_IMAGE_CONFIG_LOCKTYPE);
    pLockType.nPortIndex = portIndex;
    pLockType.eImageLock = pPort->bAWB_Lock;

    if(pPort->config_idx & (1 << 2))
    {
        //nParamIndex = OMX_IndexConfigCommonColorEnhancement;
        //ComponentParameterStructure = (OMX_PTR)pPort->pColorEft;
        //err =omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_COLORFIX_MANUAL, (unsigned long)pPort->pColorEft, pPort->pSensorMode->bOneShot);


    }

    if(pPort->config_idx & (1 << 3))
    {
        //nParamIndex = OMX_IndexConfigCommonImageFilter;
        //ComponentParameterStructure = pPort->pImageFilter;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_COLORFIX, (unsigned long)pPort->pImageFilter, pPort->pSensorMode->bOneShot);

    }

    if(pPort->config_idx & (1 << 4))
    {
        //nParamIndex = OMX_IndexConfigCommonMirror;
        //ComponentParameterStructure = pPort->pImageMirror;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLIP, (unsigned long)pPort->pImageMirror, pPort->pSensorMode->bOneShot);

    }

    if(pPort->config_idx & (1 << 5))
    {
        //nParamIndex = OMX_IndexConfigCommonWhiteBalance;
        //ComponentParameterStructure = &pLockType;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_WB_TYPE, (unsigned long)pPort->pWBType, pPort->pSensorMode->bOneShot);

    }

    if(pPort->config_idx & (1 << 6))
    {
        //nParamIndex = OMX_IndexConfigImageWhiteBalanceLock;
        //ComponentParameterStructure = &pLockType;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AWBLOCK, (unsigned long)&pLockType, pPort->pSensorMode->bOneShot);

    }

    if(pPort->config_idx & (1 << 7))
    {
        pLockType.eImageLock = pPort->bAE_Lock;
        //nParamIndex = OMX_IndexConfigImageExposureLock;
        //ComponentParameterStructure = &pLockType;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AELOCK, (unsigned long)&pLockType, pPort->pSensorMode->bOneShot);

    }

    if(pPort->config_idx & (1 << 8))
    {
        //nParamIndex = OMX_IndexConfigCommonExposure;
        //ComponentParameterStructure = pPort->pExpType;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_EXP_TYPE, (unsigned long)pPort->pExpType, pPort->pSensorMode->bOneShot);

    }

    if(pPort->config_idx & (1 << 9))
    {
        //nParamIndex = OMX_IndexConfigCommonExposureValue;
        //ComponentParameterStructure = pPort->pExpVal;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        //err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera,CAM_SET_EXP_TYPE,(unsigned int)pPort->pExpType,pPort->pSensorMode->bOneShot);
        if(pPort->pCapInfo->xEVCompensationMax > 0 && pPort->pCapInfo->xEVCompensationMax != pPort->pCapInfo->xEVCompensationMin)
        {
            //OMX_CONFIG_BRIGHTNESSTYPE  pTypeBright;
            //pTypeBright.nBrightness = pPort->pExpVal->xEVCompensation>>16;
            err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_EV, (unsigned long)&pPort->pExpVal, pPort->pSensorMode->bOneShot);
        }

        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_MET_AREA, (unsigned long)pPort->pExpVal->eMetering, pPort->pSensorMode->bOneShot);

    }

    if(pPort->config_idx & (1 << 10))
    {
        //nParamIndex = OMX_IndexConfigCommonContrast;
        //ComponentParameterStructure = pPort->pContrast;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_CONSTRAST_LEVEL, (unsigned long)pPort->pContrast, pPort->pSensorMode->bOneShot);


    }

    if(pPort->config_idx & (1 << 11))
    {
        OMX_CONFIG_BRIGHTNESSTYPE  pTypeCur;

        //nParamIndex = OMX_IndexConfigCommonBrightness;
        //ComponentParameterStructure = pPort->pBright;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        //err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera,CAM_SET_CONSTRAST_LEVEL,(unsigned int)pType,pPort->pSensorMode->bOneShot);
        if(pPort->pCapInfo->xBrightnessLevel.nMaxVal > 0)
        {
            if(pPort->pCapInfo->xBrightnessLevel.nMinVal < 0)
            { pTypeCur.nBrightness = pPort->pBright->nBrightness * (pPort->pCapInfo->xBrightnessLevel.nMaxVal) / 100; }
            else
            { pTypeCur.nBrightness = pPort->pBright->nBrightness * (pPort->pCapInfo->xBrightnessLevel.nMaxVal - pPort->pCapInfo->xBrightnessLevel.nMinVal) / 100 + pPort->pCapInfo->xBrightnessLevel.nMinVal; }


            err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_BRIGHTNESS_LEVEL, (unsigned long)&pTypeCur, pPort->pSensorMode->bOneShot);
        }
    }

    if(pPort->config_idx & (1 << 12))
    {
        OMX_CONFIG_SATURATIONTYPE  pTypeCur;

        //nParamIndex = OMX_IndexConfigCommonSaturation;
        //ComponentParameterStructure = pPort->pSat;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        if(pPort->pCapInfo->xSaturationLevel.nMaxVal > 0)
        {
            pTypeCur.nSaturation = pPort->pSat->nSaturation * (pPort->pCapInfo->xSaturationLevel.nMaxVal - pPort->pCapInfo->xSaturationLevel.nMinVal) / 100 + pPort->pCapInfo->xSaturationLevel.nMinVal;
            err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_STA, (unsigned long)&pTypeCur, pPort->pSensorMode->bOneShot);
        }

    }

    if(pPort->config_idx & (1 << 13))
    {
        //nParamIndex = OMX_IndexConfigFlickerRejection;
        //ComponentParameterStructure = pPort->pFlicktype;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLICKER_TYPE, (unsigned long)pPort->pFlicktype, pPort->pSensorMode->bOneShot);

    }

    if(pPort->config_idx & (1 << 14))
    {
        //nParamIndex = OMX_ACT_IndexConfigImageDeNoiseLevel;
        //ComponentParameterStructure = pPort->pNs_level;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_DNS_LEVEL, (unsigned long)pPort->pNs_level, pPort->pSensorMode->bOneShot);

    }

    if(pPort->config_idx & (1 << 15))
    {
        //nParamIndex = OMX_IndexConfigSharpness;
        //ComponentParameterStructure = pPort->pSharp_level;
        //err |= omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_SHARP_LEVEL, (unsigned long)pPort->pSharp_level, pPort->pSensorMode->bOneShot);

    }

    OMXDBUG(OMXDBUG_ERR, "set param,%d,%d\n", __LINE__, err);
    return err;
}

static int cam_set_def(omx_base_camera_video_PortType *pPort, int nPortIndex)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    int input_format = 0;

    if(nPortIndex == OMX_BASE_FILTER_VIDEO_INDEX)
    {
        int nW = pPort->sPortParam.format.video.nFrameWidth;
        int nH = pPort->sPortParam.format.video.nFrameHeight;

        if(pPort->bResizeEnable == 1)
        {
            nW = pPort->nInputW;
            nH = pPort->nInputH;
        }

        input_format = pPort->sPortParam.format.video.eColorFormat;

        if(pPort->bMJPEG_Enable == 1 && pPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
        {
            input_format = OMX_COM_FORMAT_MJPG;
        }

        OMXDBUG(OMXDBUG_PARAM, "set v res %d,%d,%d,%d\n", pPort->pSensorMode->bOneShot, nW, nH, pPort->sPortParam.format.video.eColorFormat);
        err |= pPort->omx_camera->omx_camera_setres(pPort->omx_camera, nW, nH, input_format, pPort->pSensorMode->bOneShot);
        pPort->omx_camera->omx_camera_setfps(pPort->omx_camera, pPort->sPortParam.format.video.xFramerate >> 16, pPort->pSensorMode->bOneShot);
    }
    else
    {
        int nW = pPort->sPortParam.format.image.nFrameWidth;
        int nH = pPort->sPortParam.format.image.nFrameHeight;

        if(pPort->bResizeEnable == 1)
        {
            nW = pPort->nInputW;
            nH = pPort->nInputH;
        }
        
        input_format = pPort->sPortParam.format.image.eColorFormat;

        if(pPort->bMJPEG_Enable == 1 && pPort->sPortParam.format.image.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
        {
            input_format = OMX_COM_FORMAT_MJPG;
        }

        OMXDBUG(OMXDBUG_PARAM, "set i res %d,%d,%d,%d\n", pPort->pSensorMode->bOneShot, nW, nH, pPort->sPortParam.format.image.eColorFormat);
        err |= pPort->omx_camera->omx_camera_setres(pPort->omx_camera, nW, nH, input_format, pPort->pSensorMode->bOneShot);
        //err |= pPort->omx_camera->omx_camera_setfps(pPort->omx_camera,pPortDef->format.video.xFramerate>>16,pPort->pSensorMode->bOneShot);
    }

    return err;
}

#if 1
static void UseBuffer_Hdr(omx_base_camera_video_PortType *pPort)
{
    if(pPort->bHdr_Enable == OMX_TRUE)
    {
        OMXDBUG(OMXDBUG_ERR, "UseBuffer_Hdr %d,%d\n", pPort->sPortParam.format.image.nFrameWidth, pPort->sPortParam.format.image.nFrameHeight);
        pPort->pHDR_Buf[0].phyAddr = ext_phycalloc_mem_cm(1, pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 3 / 2, &pPort->pHDR_Buf[0].VirAddr);
        pPort->pHDR_Buf[1].phyAddr = ext_phycalloc_mem_cm(1, pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 3 / 2, &pPort->pHDR_Buf[1].VirAddr);
        pPort->pHDR_Buf[2].phyAddr = ext_phycalloc_mem_cm(1, pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 3 / 2, &pPort->pHDR_Buf[2].VirAddr);
        pPort->pHDR_Buf[3].phyAddr = ext_phycalloc_mem_cm(1, pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 3 / 2, &pPort->pHDR_Buf[3].VirAddr);
        OMXDBUG(OMXDBUG_ERR, "UseBuffer_Hdr %x,%x,%x,%x\n", pPort->pHDR_Buf[0].phyAddr, pPort->pHDR_Buf[1].phyAddr, pPort->pHDR_Buf[2].phyAddr, pPort->pHDR_Buf[3].phyAddr);
    }
    else
    {
        pPort->pHDR_Buf[0].phyAddr = NULL;
        pPort->pHDR_Buf[1].phyAddr = NULL;
        pPort->pHDR_Buf[2].phyAddr = NULL;
        pPort->pHDR_Buf[3].phyAddr = NULL;
    }
}

static int  FillBuffer_Hdr(omx_base_camera_video_PortType *pPort, int buffidx)
{
    int err = 0;

    if(pPort->bHdr_Enable == OMX_TRUE)
    {
        err |= pPort->omx_camera->omx_camera_qbuf(pPort->omx_camera, buffidx, pPort->sPortParam.nBufferSize, pPort->pHDR_Buf[buffidx].phyAddr, NULL, pPort->pSensorMode->bOneShot);
        OMXDBUG(OMXDBUG_VERB, "%s, %d, %d\n", __func__, buffidx, err);
    }

    return err;
}

static int  ReqBuf_Hdr(omx_base_camera_video_PortType *pPort)
{
    int bufcnt = 4;
    int buffidx = 0;
    int err = OMX_ErrorNone;
    
    OMXDBUG(OMXDBUG_PARAM, "ReqBuf_Hdr");
    
    UseBuffer_Hdr(pPort);

    if(pPort->bHdr_Enable == OMX_TRUE)
    {
        err |= pPort->omx_camera->omx_camera_requestbuf(pPort->omx_camera, (unsigned int *)&bufcnt, pPort->pSensorMode->bOneShot);

        if(err != OMX_ErrorNone)
        {
            return OMX_ErrorHardware;
        }

        for(buffidx = 0; buffidx < bufcnt; buffidx++)
        {
            err = pPort->omx_camera->omx_camera_querybuf((void *)pPort->omx_camera, buffidx, \
                    pPort->sPortParam.nBufferSize, \
                    pPort->pHDR_Buf[buffidx].phyAddr, \
                    NULL, \
                    pPort->pSensorMode->bOneShot);
        }

        for(buffidx = 0; buffidx < bufcnt; buffidx++)
        {
//	            FillBuffer_Hdr(pPort, buffidx);
        }
    }

    return err;
}

static void Free_Hdr(omx_base_camera_video_PortType *pPort)
{
    //if(pPort->bHdr_Enable == OMX_TRUE)
    {
        if(pPort->pHDR_Buf[0].phyAddr)
        {
            OMXDBUG(OMXDBUG_ERR, "Free_Hdr");
            ext_phyfree_mem_cm(pPort->pHDR_Buf[0].phyAddr, pPort->pHDR_Buf[0].VirAddr);
            pPort->pHDR_Buf[0].phyAddr = NULL;
        }

        if(pPort->pHDR_Buf[1].phyAddr)
        {
            OMXDBUG(OMXDBUG_ERR, "Free_Hdr");
            ext_phyfree_mem_cm(pPort->pHDR_Buf[1].phyAddr, pPort->pHDR_Buf[1].VirAddr);
            pPort->pHDR_Buf[1].phyAddr = NULL;
        }

        if(pPort->pHDR_Buf[2].phyAddr)
        {
            OMXDBUG(OMXDBUG_ERR, "Free_Hdr");
            ext_phyfree_mem_cm(pPort->pHDR_Buf[2].phyAddr, pPort->pHDR_Buf[2].VirAddr);
            pPort->pHDR_Buf[2].phyAddr = NULL;
        }

        if(pPort->pHDR_Buf[3].phyAddr)
        {
            OMXDBUG(OMXDBUG_ERR, "Free_Hdr");
            ext_phyfree_mem_cm(pPort->pHDR_Buf[3].phyAddr, pPort->pHDR_Buf[3].VirAddr);
            pPort->pHDR_Buf[3].phyAddr = NULL;
        }

        //pPort->bHdr_Enable = OMX_FALSE;
    }
}

int getHDRInfo(const char *prop_name, int default_val, int *ret_val)
{
    char val[255];

    if(property_get(prop_name, val, NULL))
    {
        char *end;
        unsigned long x = strtoul(val, &end, 10);

        //          OMXDBUG(OMXDBUG_ERR, "val: %s", val);
        if(*end == '\0' && end > val && x > 0)
        {
            //              OMXDBUG(OMXDBUG_ERR, "x:%d", x);
            *ret_val =  x;
            return 1;
        }
    }

    *ret_val = default_val;
    return 0;
}

static void Process_Hdr(omx_base_camera_video_PortType *pPort, unsigned char *buf1, unsigned char *buf2, unsigned char *buf3, unsigned char *bufout, int w, int h, int exptime[3])
{
    int i;
    hdri_t *hdri;
    frames_info_t frames_info;
    float exposure_time[3];
    int width_hdr;
    int height_hdr;
    unsigned char *frames[3];
    unsigned char *pic_hdr = NULL;
    int ret = 0;
    FILE *raw_cap;
    FILE *raw_info;
    int doRemoveGhost;

#if 0
    raw_cap = fopen("/data/hdraw.yuv", "wb");
    if(raw_cap)
    {
        OMXDBUG(OMXDBUG_PARAM, "/data/hdraw.yuv exists!");
        fwrite(buf1, 1, w * h * 3 / 2, raw_cap);
        fwrite(buf2, 1, w * h * 3 / 2, raw_cap);
        fwrite(buf3, 1, w * h * 3 / 2, raw_cap);
        fclose(raw_cap);
    }

    raw_info = fopen("/data/hdraw.info", "w");
    if(raw_info)
    {
        OMXDBUG(OMXDBUG_PARAM, "/data/hdraw.info exists!");
        fprintf(raw_info, "%d %d\n", w, h);
        fprintf(raw_info, "%d %d %d", exptime[0], exptime[1], exptime[2]);
        fclose(raw_info);
        return;
    }
#endif

    getHDRInfo("ro.camerahal.hdrghost", 0, &doRemoveGhost);
    OMXDBUG(OMXDBUG_PARAM, "de-ghosting enable: %d\n", doRemoveGhost);

    pPort->width_hdr = w;
    pPort->height_hdr = h;
    memcpy(bufout, buf1, w * h * 3 / 2);

#if 1
    hdri = (hdri_t *)(pPort->openHDRI());
    
    frames_info.width = w;
    frames_info.height = h;
    frames_info.format = 0;
    if(!hdri->init(hdri, &frames_info))
    {
        goto HDR_EXIT;
    }

    frames[0] = buf2;
    frames[1] = buf1;
    frames[2] = buf3;
    exposure_time[0] = exptime[1];
    exposure_time[1] = exptime[0];
    exposure_time[2] = exptime[2];
    if(!hdri->addFrame(hdri, frames[0], exposure_time[0], 1) ||
            !hdri->addFrame(hdri, frames[1], exposure_time[1], 1) ||
            !hdri->addFrame(hdri, frames[2], exposure_time[2], 1))
    {
        goto HDR_EXIT;
    }

    if(!hdri->blendFrames(hdri, &width_hdr, &height_hdr, doRemoveGhost))
    {
        goto HDR_EXIT;
    }

    if(!hdri->runToneMapping(hdri, bufout))
    {
        goto HDR_EXIT;
    }

    pPort->width_hdr = width_hdr;
    pPort->height_hdr = height_hdr;
    OMXDBUG(OMXDBUG_PARAM, "hdr out: %dx%d", width_hdr, height_hdr);

HDR_EXIT:
    hdri->exit(hdri);
    free(hdri);
#endif

    Free_Hdr(pPort);
    return;
}
#endif

OMX_ERRORTYPE omx_camera_component_AllocateBuffer(
    OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes)
{
    OMX_COMPONENTTYPE                          *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_camera_PrivateType                   *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    omx_base_camera_video_PortType                    *pPort = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[nPortIndex];
    OMX_ERRORTYPE err;

    OMXDBUG(OMXDBUG_VERB, "");

    if(pPort->bStoreMediadata)
    {
        OMXDBUG(OMXDBUG_ERR, "bIsStoreMediaData Not Support here.\n");
        return OMX_ErrorUnsupportedSetting;
    }

    err = omx_base_component_AllocateBuffer(hComponent, ppBuffer, nPortIndex, pAppPrivate, nSizeBytes);

    if(err == OMX_ErrorNone)
    {
        int buffidx = omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue;

        /*if(pPort->bStoreMediadata){
            video_metadata_t *pbuff = (video_metadata_t*)((*ppBuffer)->pBuffer);
            video_handle_t *pbuffhandle = pbuff->handle;
            omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx] = pbuffhandle->phys_addr;
        }
        else */
        {
            omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx] = pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr;//(unsigned int)(*ppBuffer)->pBuffer;
        }
        pPort->pBufferHeadAct[buffidx].pConfigParam.bUseBufFlag = OMX_FALSE;
        pPort->pBufferHeadAct[buffidx].pConfigParam.index = buffidx;
        pPort->pBufferHeadAct[buffidx].pConfigParam.buffersize = pPort->sPortParam.nBufferSize;
        pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr = omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx];
        pPort->pBufferHeadAct[buffidx].pConfigParam.bAllocStatFlag = 0;
#ifndef ALLOC_FROM_USR
        pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr = ext_phycalloc_mem(1, 4096, &pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr);
        pPort->pBufferHeadAct[buffidx].pConfigParam.bAllocStatFlag = 1;
#else
        //pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr = pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr+pPort->sPortParam.nBufferSize;
        //pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr = pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr+pPort->sPortParam.nBufferSize;

#endif
        //omx_camera_component_Private->pBufQ[nPortIndex].pBufferStat[buffidx] = pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr;
        omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue++;
        pPort->nDropFrames++;

        if(pPort->nNumAssignedBuffers == pPort->sPortParam.nBufferCountActual && nPortIndex == 0)
        {
            omx_base_camera_video_PortType *pPort_Other = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1];

            if(pPort_Other->bStreamOn == OMX_TRUE)
            {
                //Close ViewFinder Now
                OMXDBUG(OMXDBUG_VERB, "StreamOff....\n");
                pthread_mutex_lock(&omx_camera_component_Private->dq_mutex);
                pPort_Other->omx_camera->omx_camera_streamoff(pPort_Other->omx_camera, pPort_Other->pSensorMode->bOneShot);
                pPort_Other->bStreamOn = OMX_FALSE;
                pPort_Other->isCapture = OMX_FALSE;
                pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);
            }

            if(pPort->bStreamOn == OMX_FALSE && omx_camera_component_Private->bTransFrom_C2V == OMX_FALSE && omx_camera_component_Private->state == OMX_StateExecuting)
            {
                unsigned int buffidx = 0;
                OMXDBUG(OMXDBUG_ERR, "Stream On Set...%d\n", __LINE__);
                //omx_camera_component_Private->filter_setconfig(hComponent,OMX_IndexParamPortDefinition,(OMX_PTR)&pPort->sPortParam);
                err |= cam_set_def(pPort, nPortIndex);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

                if(pPort->config_idx)
                {
                    err |= SetCamParams((OMX_COMPONENTTYPE *)hComponent, 0);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        return OMX_ErrorHardware;
                    }
                }

                pPort->omx_camera->omx_camera_requestbuf(pPort->omx_camera, \
                        (unsigned int *)&pPort->sPortParam.nBufferCountActual, \
                        pPort->pSensorMode->bOneShot);

                for(buffidx = 0; buffidx < pPort->nNumAssignedBuffers; buffidx++)
                {
                    OMXDBUG(OMXDBUG_VERB, "Preview Stream SetBuff now ....%i\n", buffidx);
                    pPort->omx_camera->omx_camera_querybuf((void *)pPort->omx_camera, buffidx, \
                                                           pPort->sPortParam.nBufferSize, \
                                                           omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx], \
                                                           omx_camera_component_Private->pBufQ[nPortIndex].pBufferStat[buffidx], \
                                                           pPort->pSensorMode->bOneShot);
                    omx_camera_component_Private->pBufQ[nPortIndex].bQb[buffidx] = 1;
                }

                pPort->omx_camera->omx_camera_streamon(pPort->omx_camera, pPort->pSensorMode->bOneShot);
                pPort->bStreamOn = OMX_TRUE;
                //if(omx_camera_component_Private->bMgmtSem->semval==0)
                {
                    tsem_up(omx_camera_component_Private->bMgmtSem);
                }
            }
        }
    }

    return err;
}

OMX_ERRORTYPE omx_camera_component_UseBuffer(
    OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes,
    OMX_U8 *pBuffer)
{
    OMX_COMPONENTTYPE                          *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_camera_PrivateType                   *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    omx_base_camera_video_PortType                    *pPort;
    OMX_ERRORTYPE err;
    unsigned int buffidx = omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue;
    pPort = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[nPortIndex];

    OMXDBUG(OMXDBUG_VERB, "in %s, %d\n", __func__, nPortIndex);

    if(pPort->bStoreMediadata)
    {
        if(nSizeBytes != sizeof(video_metadata_t))
        {
            OMXDBUG(OMXDBUG_ERR, "Bad buffer size on storemediadata type %d\n", nSizeBytes);
            return OMX_ErrorBadParameter;
        }
    }

    err = omx_base_component_UseBuffer(hComponent, ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
    if(err == OMX_ErrorNone)
    {
        if(pPort->bStoreMediadata)
        {
            video_metadata_t *pbuff = (video_metadata_t *)(pBuffer);
#if 0
            void *pbuffhandle = pbuff->handle;
            unsigned int *phys_addr;
            void *vaddr = NULL;
            int w, h, fmt, ss;
            Actions_OSAL_GetPhyAddr(pbuffhandle, &phys_addr);
            OMXDBUG(OMXDBUG_ERR, "store addr %x,%x,%x\n", (unsigned int)pbuff, (unsigned int)pbuffhandle, phys_addr);
            omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx] = phys_addr;
            pPort->pBufferHeadAct[buffidx].pConfigParam.handle = pbuffhandle;
#ifdef ALLOC_FROM_USR
            Actions_OSAL_GetBufInfo(pbuffhandle, &w, &h, &fmt, &ss);
            Actions_OSAL_LockANBHandleWidthUsage(pbuffhandle, w, h, 2, &vaddr);
            pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr = vaddr;//omx_mmap_ion_fd(pbuffhandle->ion_share_fd,pbuffhandle->size);
            pPort->pBufferHeadAct[buffidx].pConfigParam.bAllocStatFlag = 2;
            pPort->pBufferHeadAct[buffidx].pConfigParam.nVirSize = ss;
            Actions_OSAL_UnlockANBHandle(pPort->pBufferHeadAct[buffidx].pConfigParam.handle);
            pPort->pBufferHeadAct[buffidx].pConfigParam.handle = NULL;
#endif
#endif
            void *phys_addr = NULL, *vir_addr = NULL;
            int width = 0, height = 0, format = 0, size = 0;

            if(IGralloc_getPhys(pbuff->handle, &phys_addr))
            {
                OMXDBUG(OMXDBUG_ERR, "get buffer physical address failed\n");
                return OMX_ErrorUndefined;
            }

            omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx] = (unsigned long)(phys_addr);

#if 1
            if(IGralloc_getBufferInfo(pbuff->handle, &width, &height, &format, &size))
            {
                OMXDBUG(OMXDBUG_ERR, "UseBuffer store media mode,get buffer infofailed\n");
                return OMX_ErrorUndefined;
            }

            if(IGralloc_lock(pbuff->handle, (GRALLOC_USAGE_SW_READ_OFTEN), &vir_addr))
            {
                OMXDBUG(OMXDBUG_ERR, "UseBuffer store media mode,get buffer virtual address failed\n");
                return OMX_ErrorUndefined;
            }

            pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr = vir_addr;
            pPort->pBufferHeadAct[buffidx].pConfigParam.bAllocStatFlag = 2;
            pPort->pBufferHeadAct[buffidx].pConfigParam.nVirSize = size;

            if(IGralloc_unlock(pbuff->handle))
            {
                OMXDBUG(OMXDBUG_ERR, "UseBuffer store media mode,unlock buffer failed\n");
                return OMX_ErrorUndefined;
            }
#endif

            OMXDBUG(OMXDBUG_PARAM,"%s, video_metadata_t: %d, %d, %d, %x, %x\n",
                __func__, width, height, size, pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr, phys_addr);

            // imx buffer allocated outside; if bResizeEnable, not this size
//	            if(0 == buffidx)
//	            {
//	                OMXDBUG(OMXDBUG_PARAM,"alloc imx nr&ee buffer, %d\n", nPortIndex);
//	                pPort->imx_buf.phyAddr = ext_phycalloc_mem(1, size, &pPort->imx_buf.VirAddr);
//	                if(NULL == pPort->imx_buf.phyAddr)
//	                {
//	                    return OMX_ErrorInsufficientResources;
//	                }
//	            }
        }
        else
        {
            pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr = ext_phycalloc_mem(1, pPort->sPortParam.nBufferSize, \
                    &pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr);

            if(pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr == NULL)
            {
                return OMX_ErrorInsufficientResources;
            }

            pPort->pBufferHeadAct[buffidx].pConfigParam.bAllocByComp = OMX_TRUE;
            pPort->pBufferHeadAct[buffidx].pConfigParam.bAllocStatFlag = 0;
            omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx] = (unsigned int)pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr;
        }

        if(pPort->bResizeEnable == 1 || pPort->bMJPEG_Enable == 1)
        {
            int nFrameSize = pPort->nInputW * pPort->nInputH * 3 / 2 + 4096;
            if(pPort->bMJPEG_Enable)     { nFrameSize   = pPort->nInputW * pPort->nInputH * 2 + 4096; }

            pPort->pBufferHeadAct[buffidx].pConfigParam.bytes_of_resize = nFrameSize;
            pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize = ext_phycalloc_mem_cm(
                        1, nFrameSize, &pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr_of_resize);
            if(pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize == NULL)
            {
                return OMX_ErrorInsufficientResources;
            }
            memset(pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr_of_resize, 0, nFrameSize);
            
            OMXDBUG(OMXDBUG_PARAM, "Preview size is small now %dx%d to %dx%d,%x,%x", pPort->nInputW, pPort->nInputH, 
                pPort->sPortParam.format.image.nFrameWidth, pPort->sPortParam.format.image.nFrameHeight, 
                pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize, pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr_of_resize);
        }

        pPort->pBufferHeadAct[buffidx].pConfigParam.bUseBufFlag = OMX_TRUE;
        pPort->pBufferHeadAct[buffidx].pConfigParam.index = buffidx;
        pPort->pBufferHeadAct[buffidx].pConfigParam.buffersize = pPort->sPortParam.nBufferSize;
        pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr = omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx];
#ifndef ALLOC_FROM_USR
        //pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr = ext_phycalloc_mem(1,4096,(unsigned int*)&pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr);
        //pPort->pBufferHeadAct[buffidx].pConfigParam.bAllocStatFlag = 1;
#else
        pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr = 0;
        //pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr = pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr+pPort->sPortParam.nBufferSize;
        //pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr = pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr+pPort->sPortParam.nBufferSize;
        //pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr[4095] = 0x80;
        //OMXDBUG(OMXDBUG_ERR,"vir val %d",pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr[4095]);
        //memset(pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr,0x0,4096);
#endif
        //omx_camera_component_Private->pBufQ[nPortIndex].pBufferStat[buffidx] = pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr;
        omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue++;
        pPort->nDropFrames++;

        if(pPort->nNumAssignedBuffers == pPort->sPortParam.nBufferCountActual && nPortIndex == 0 && omx_camera_component_Private->state == OMX_StateExecuting)
        {
            omx_base_camera_video_PortType *pPort_Other = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1];
            unsigned int buffidx = 0;
            pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);

            if(pPort_Other->bStreamOn == OMX_TRUE)
            {
                //Close ViewFinder Now
                OMXDBUG(OMXDBUG_VERB, "StreamOff....\n");
                pPort_Other->omx_camera->omx_camera_streamoff(pPort_Other->omx_camera, pPort_Other->pSensorMode->bOneShot);
                pPort_Other->bStreamOn = OMX_FALSE;
                pPort_Other->isCapture = OMX_FALSE;
            }

            if(pPort->bStreamOn == OMX_FALSE && omx_camera_component_Private->bTransFrom_C2V == OMX_FALSE)
            {
                OMXDBUG(OMXDBUG_ERR, "Stream On Set...%d\n", __LINE__);
                //omx_camera_component_Private->filter_setconfig(hComponent,OMX_IndexParamPortDefinition,(OMX_PTR)&pPort->sPortParam);
                err |= cam_set_def(pPort, nPortIndex);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                    return OMX_ErrorHardware;
                }

                if(pPort->config_idx)
                {
                    err |= SetCamParams((OMX_COMPONENTTYPE *)hComponent, 0);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                        return OMX_ErrorHardware;
                    }
                }

                pPort->omx_camera->omx_camera_requestbuf(pPort->omx_camera, \
                        (unsigned int *)&pPort->sPortParam.nBufferCountActual, \
                        pPort->pSensorMode->bOneShot);

                for(buffidx = 0; buffidx < pPort->nNumAssignedBuffers; buffidx++)
                {
                    OMXDBUG(OMXDBUG_VERB, "Preview Stream SetBuff now ....%i\n", buffidx);
                    pPort->omx_camera->omx_camera_querybuf((void *)pPort->omx_camera, buffidx, \
                                                           pPort->sPortParam.nBufferSize, \
                                                           omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx], \
                                                           omx_camera_component_Private->pBufQ[nPortIndex].pBufferStat[buffidx], \
                                                           pPort->pSensorMode->bOneShot);
                    omx_camera_component_Private->pBufQ[nPortIndex].bQb[buffidx] = 1;
                }

                pPort->omx_camera->omx_camera_streamon(pPort->omx_camera, pPort->pSensorMode->bOneShot);
                pPort->bStreamOn = OMX_TRUE;
                pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                //if(omx_camera_component_Private->bMgmtSem->semval==0)
                {
                    tsem_up(omx_camera_component_Private->bMgmtSem);
                }
            }
            else
            { pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex); }
        }
    }

    return err;
}

OMX_ERRORTYPE omx_camera_component_FreeBuffer(
    OMX_HANDLETYPE hComponent,
    OMX_U32 nPortIndex,
    OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_COMPONENTTYPE                          *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_camera_PrivateType                   *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    omx_base_camera_video_PortType  *pPort = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[nPortIndex];

    OMXDBUG(OMXDBUG_PARAM, "in %s, %d\n", __func__, nPortIndex);

#if 0
    if(omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue == pPort->nNumAssignedBuffers)
    {
        pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, pPort->isCapture);
        pPort->bStreamOn = OMX_FALSE;
    }

#endif
    int buffidx = 0;

    if(pPort->bStreamOn == OMX_TRUE)
    {
        pthread_mutex_lock(&omx_camera_component_Private->dq_mutex);

        if(pPort->omx_camera->omx_camera_streamoff)
        {
            pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, pPort->pSensorMode->bOneShot);
        }

        pPort->isCapture = OMX_FALSE;
        pPort->bStreamOn = OMX_FALSE;
        pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);

        if(pPort->pSensorMode->bOneShot == OMX_FALSE)
        {
            omx_camera_component_Private->bTransFrom_C2V = OMX_FALSE;
        }

        tsem_up(omx_camera_component_Private->bMgmtSem);
    }

    omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue--;

    if(omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue < 0)
    {
        omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue = 0;
    }

    omx_camera_component_Private->pBufQ[nPortIndex].bQb[omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue] = 0;
    pPort->nDropFrames--;

    if(pPort->nDropFrames < 0) { pPort->nDropFrames = 0; }

    buffidx = get_base_bufferidx(pPort->pBufferHeadAct, pBuffer, pPort->sPortParam.nBufferCountActual);

    if(buffidx != -1)
    {
        if(pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize)
        {
            pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex_resize);
            ext_phyfree_mem_cm(pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize, pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr_of_resize);
            pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr_of_resize = NULL;
            pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize = 0;
            pPort->pBufferHeadAct[buffidx].pConfigParam.bytes_of_resize = 0;
            pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex_resize);
        }
    }

//	    // 
//	    if(pPort->imx_buf.phyAddr)
//	    {
//	        ext_phyfree_mem(pPort->imx_buf.phyAddr, pPort->imx_buf.VirAddr);
//	        pPort->imx_buf.phyAddr = NULL;
//	    }

#ifdef ALLOC_FROM_USR
    buffidx = get_base_bufferidx(pPort->pBufferHeadAct, pBuffer, pPort->sPortParam.nBufferCountActual);

    if(buffidx != -1)
    {
        if(pPort->pBufferHeadAct[buffidx].pConfigParam.bAllocStatFlag == 2)
        {
            /*if(pPort->pBufferHeadAct[buffidx].pConfigParam.handle){
                Actions_OSAL_UnlockANBHandle(pPort->pBufferHeadAct[buffidx].pConfigParam.handle);
                pPort->pBufferHeadAct[buffidx].pConfigParam.handle = NULL;
            }*/

            //omx_munmap_ion_fd(pPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr,pPort->pBufferHeadAct[buffidx].pConfigParam.nVirSize);
        }

        //else if(pPort->pBufferHeadAct[buffidx].pConfigParam.bAllocStatFlag == 1){
        //ext_phyfree_mem(pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr,pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr);
        //}
    }

#else
    //OMXDBUG(OMXDBUG_ERR,"omx_base_component_FreeBuffer");
#endif
#if 1
    Free_Hdr(pPort);
#endif
    return omx_base_component_FreeBuffer(hComponent, nPortIndex, pBuffer);
}

OMX_ERRORTYPE omx_camera_component_FillThisBuffer(
    OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_COMPONENTTYPE                          *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_camera_PrivateType                   *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    omx_base_camera_video_PortType                    *pPort;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    //OMX_BUFFERHEADERTYPE_ACTEXT *pBufferAct = (OMX_BUFFERHEADERTYPE_ACTEXT*)pBuffer;
    unsigned int nPortIndex = pBuffer->nOutputPortIndex;
    int buffidx = -1;
    
    pPort = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[nPortIndex];
    OMXDBUG(OMXDBUG_VERB, "In %s, FillThisBuffer %d,%d,%d\n", __func__, nPortIndex, PORT_IS_BEING_DISABLED(pPort), PORT_IS_ENABLED(pPort));
    if(PORT_IS_BEING_DISABLED(pPort) || !PORT_IS_ENABLED(pPort))
    {
        return OMX_ErrorNotReady;
    }

#if 0
    pPort = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[nPortIndex];

    //printf("Stream FillThisBuffer ....%i,%i\n",buffidx,nPortIndex);
    if(pPort->bStreamOn == OMX_FALSE)
    {
        int buffidx = omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue;
        pPort_other = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[1 - nPortIndex];
        OMXDBUG(OMXDBUG_VERB, "Stream SetBuff now ....%i\n", buffidx);

        if(pPort->pSensorMode->bOneShot == OMX_TRUE && pPort_other->pSensorMode->bOneShot == OMX_FALSE)
        {
            //preview to capture
            if(buffidx == pPort->nNumAssignedBuffers)
            {
                pPort->nDropFrames = pPort->nNumAssignedBuffers;
                pPort->omx_camera->omx_camera_requestbuf(pPort->omx_camera, (unsigned int *)&pPort->sPortParam.nBufferCountActual, pPort->pSensorMode->bOneShot);
            }

            if(buffidx < pPort->nNumAssignedBuffers)
            {
                if(pPort->bStoreMediadata)
                {
                    video_metadata_t *pbuff = (video_metadata_t *)(pBuffer->pBuffer);
                    void *pbuffhandle = pbuff->handle;
                    unsigned int *phys_addr;
                    Actions_OSAL_GetPhyAddr(pbuffhandle, &phys_addr);
                    omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[pPort->nNumAssignedBuffers - buffidx] = phys_addr;
                }
                else
                {
                    omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[pPort->nNumAssignedBuffers - buffidx] = (unsigned int)pBuffer->pBuffer;
                }

                pBufferAct->pConfigParam.index = pPort->nNumAssignedBuffers - buffidx;
                pBufferAct->pConfigParam.buffersize = pPort->sPortParam.nBufferSize;
                pBufferAct->pConfigParam.phyAddr = omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[pPort->nNumAssignedBuffers - buffidx];
                omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue--;
                pPort->omx_camera->omx_camera_querybuf((void *)pPort->omx_camera, pPort->nNumAssignedBuffers - buffidx, pPort->sPortParam.nBufferSize, omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[pPort->nNumAssignedBuffers - buffidx], pBufferAct->pConfigParam.Stat_phyAddr, pPort->pSensorMode->bOneShot);
                buffidx = omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue;
            }

            if(buffidx == 0)
            {
                omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue = pPort->nNumAssignedBuffers;
                pPort->omx_camera->omx_camera_streamon((void *)pPort->omx_camera, pPort->isCapture);
                pPort->bStreamOn = OMX_TRUE;
            }
        }
        else
        {
            if(buffidx == 0)
            {
                pPort->nDropFrames = pPort->nNumAssignedBuffers;
                pPort->omx_camera->omx_camera_requestbuf(pPort->omx_camera, (unsigned int *)&pPort->sPortParam.nBufferCountActual, pPort->pSensorMode->bOneShot);
            }

            OMXDBUG(OMXDBUG_VERB, "Preview Stream SetBuff now ....%i\n", buffidx);

            if(buffidx < pPort->nNumAssignedBuffers)
            {
                if(pPort->bStoreMediadata)
                {
                    video_metadata_t *pbuff = (video_metadata_t *)(pBuffer->pBuffer);
                    //video_handle_t *pbuffhandle = pbuff->handle;
                    void *pbuffhandle = pbuff->handle;
                    unsigned int *phys_addr;
                    Actions_OSAL_GetPhyAddr(pbuffhandle, &phys_addr);
                    omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx] = phys_addr;
                }
                else
                {
                    omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx] = (unsigned int)pBuffer->pBuffer;
                }

                pBufferAct->pConfigParam.index = buffidx;
                pBufferAct->pConfigParam.buffersize = pPort->sPortParam.nBufferSize;
                pBufferAct->pConfigParam.phyAddr = omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx];
                omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue++;
                pPort->omx_camera->omx_camera_querybuf((void *)pPort->omx_camera, buffidx, pPort->sPortParam.nBufferSize, omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx], pBufferAct->pConfigParam.Stat_phyAddr, pPort->pSensorMode->bOneShot);
                buffidx = omx_camera_component_Private->pBufQ[nPortIndex].nBufferQueue;
            }

            if(buffidx == pPort->nNumAssignedBuffers)
            {
                pPort->omx_camera->omx_camera_streamon((void *)pPort->omx_camera, pPort->pSensorMode->bOneShot);
                pPort->bStreamOn = OMX_TRUE;
                OMXDBUG(OMXDBUG_VERB, "Stream On now....\n");
            }
        }
    }
#endif

    if(pPort->bStoreMediadata)
    {
        video_metadata_t *pbuff = (video_metadata_t *)(pBuffer->pBuffer);
        pbuff->off_x = 0;
        pbuff->off_y = 0;
        pbuff->crop_w = 0;
        pbuff->crop_h = 0;
//             OMXDBUG(OMXDBUG_ERR, "Cropped width & height: %d, %d, %d", pbuff->crop_w, pbuff->crop_h, __LINE__);
    }

    //if(pPort->nDropFrames <= 0)
    // if(omx_camera_component_Private->pBufQ[nPortIndex].bQb[pBufferAct->pConfigParam.index] == 0)
#if 1
    if(nPortIndex != 0)
    {
        // pPort->nDropFrames = 0;
        //if(pPort->bStreamOn == OMX_TRUE)
        buffidx = get_base_bufferidx(pPort->pBufferHeadAct, pBuffer, pPort->sPortParam.nBufferCountActual);
        OMXDBUG(OMXDBUG_PARAM, "v4l2camera:Fill This Buff component  %x,%x,%x,%x\n", (unsigned int)nPortIndex, (unsigned int)pPort->bStreamOn, (unsigned int)buffidx, (unsigned int)pPort->sPortParam.nBufferSize);
        if(buffidx != -1)
        {
            pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);
            OMXDBUG(OMXDBUG_PARAM, "FTBC %x\n", (unsigned int)pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr);
#if 1
            if(pPort->bMJPEG_Enable == 1 || pPort->bResizeEnable == 1)
            {
                err = pPort->omx_camera->omx_camera_qbuf(pPort->omx_camera, buffidx, \
                        pPort->pBufferHeadAct[buffidx].pConfigParam.bytes_of_resize, pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize, \
                        (unsigned long)&pPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo, pPort->pSensorMode->bOneShot);
                //    OMXDBUG(OMXDBUG_ERR,"omx_camera_qbuf image addr %d,%x\n",buffidx,pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize);
            }
            else if(pPort->bMJPEG_Enable == 0)
            {
                pPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo.pPhyAddr = pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr;
                pPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo.pVirAddr = (unsigned long)pPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr;

                if(pPort->bHdr_Enable == OMX_FALSE)
                    err = pPort->omx_camera->omx_camera_qbuf(pPort->omx_camera, buffidx, pPort->sPortParam.nBufferSize,
                            pPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr, (unsigned long)&pPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo, pPort->pSensorMode->bOneShot);
            }

            pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "Err %d\n", __LINE__);
                return OMX_ErrorHardware;
            }
#endif
        }
    }
    else
#endif
    {
        pPort = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[nPortIndex];
        //OMXDBUG(OMXDBUG_ERR,"queue_q %d\n",nPortIndex);
        queue(pPort->queue_dq, (OMX_PTR)pBuffer);
    }
    //else
    // {
    //    omx_camera_component_Private->pBufQ[nPortIndex].bQb[pBufferAct->pConfigParam.index] = 0;
    // }
    //pPort->nDropFrames--;
    pBuffer->nFlags &= (~OMX_BUFFERFLAG_ENDOFFRAME);
    return omx_base_component_FillThisBuffer(hComponent, pBuffer);
}

OMX_ERRORTYPE omx_camera_component_SendCommand(
    OMX_HANDLETYPE hComponent,
    OMX_COMMANDTYPE Cmd,
    OMX_U32 nParam,
    OMX_PTR pCmdData)
{
    OMX_COMPONENTTYPE                          *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_camera_PrivateType                   *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    OMXDBUG(OMXDBUG_VERB, "");

    if(Cmd == OMX_CommandStateSet && nParam == OMX_StateExecuting)
    {
        omx_base_camera_video_PortType *pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[0];
        omx_base_camera_video_PortType *pPort_Other = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1];

        /* if(pPort_Other->bStreamOn == OMX_TRUE)
        {
            //Close ViewFinder Now
            OMXDBUG(OMXDBUG_VERB,"StreamOff....\n");
            pPort_Other->omx_camera->omx_camera_streamoff(pPort_Other->omx_camera,pPort_Other->pSensorMode->bOneShot);
            pPort_Other->bStreamOn = OMX_FALSE;
            pPort_Other->isCapture = OMX_FALSE;
        }*/
        pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);

        if(pPort_Other->bStreamOn == OMX_FALSE && pPort->bStreamOn == OMX_FALSE && omx_camera_component_Private->bTransFrom_C2V == OMX_FALSE && pPort->nNumAssignedBuffers == pPort->sPortParam.nBufferCountActual)
        {
            unsigned int buffidx = 0;
            OMXDBUG(OMXDBUG_ERR, "Stream On ff Set...%d\n", __LINE__);

            //omx_camera_component_Private->filter_setconfig(hComponent,OMX_IndexParamPortDefinition,(OMX_PTR)&pPort->sPortParam);
            err |= cam_set_def(pPort, 0);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                return OMX_ErrorHardware;
            }

            if(pPort->config_idx)
            {
                err |= SetCamParams((OMX_COMPONENTTYPE *)hComponent, 0);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                    return OMX_ErrorHardware;
                }
            }

            pPort->omx_camera->omx_camera_requestbuf(pPort->omx_camera, \
                    (unsigned int *)&pPort->sPortParam.nBufferCountActual, \
                    pPort->pSensorMode->bOneShot);

            for(buffidx = 0; buffidx < pPort->nNumAssignedBuffers; buffidx++)
            {
                OMXDBUG(OMXDBUG_VERB, "Preview Stream SetBuff now ....%i\n", buffidx);
                pPort->omx_camera->omx_camera_querybuf((void *)pPort->omx_camera, buffidx, \
                                                       pPort->sPortParam.nBufferSize, \
                                                       omx_camera_component_Private->pBufQ[0].pBufferPhy[buffidx], \
                                                       omx_camera_component_Private->pBufQ[0].pBufferStat[buffidx], \
                                                       pPort->pSensorMode->bOneShot);
            }

            pPort->omx_camera->omx_camera_streamon(pPort->omx_camera, pPort->pSensorMode->bOneShot);
            pPort->bStreamOn = OMX_TRUE;
            pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
            //if(omx_camera_component_Private->bMgmtSem->semval==0)
            {
                tsem_up(omx_camera_component_Private->bMgmtSem);
            }
        }
        else
        { pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex); }
    }

    return omx_base_component_SendCommand(hComponent, Cmd, nParam, pCmdData);
}

OMX_ERRORTYPE omx_camera_component_SetParameter(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE                     err = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE      *pPortDef;
    OMX_VIDEO_PARAM_PORTFORMATTYPE    *pVideoPortFormat;
    //  OMX_OTHER_PARAM_PORTFORMATTYPE    *pOtherPortFormat;
    OMX_U32                           portIndex;
    OMX_PARAM_COMPONENTROLETYPE       *pComponentRole;

    /* Check which structure we are being fed and make control its header */
    OMX_COMPONENTTYPE                 *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_camera_PrivateType            *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    omx_base_camera_video_PortType           *pPort;

    if(ComponentParameterStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }

    OMXDBUG(OMXDBUG_VERB, "Setting parameter %x\n", nParamIndex);

    switch((unsigned int)nParamIndex)
    {
    case OMX_IndexParamStandardComponentRole:
    {
        pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)ComponentParameterStructure;

        if(omx_camera_component_Private->state != OMX_StateLoaded && omx_camera_component_Private->state != OMX_StateWaitForResources)
        {
            OMXDBUG(OMXDBUG_ERR, "In %s Incorrect State=%x lineno=%d\n", __func__, omx_camera_component_Private->state, __LINE__);
            return OMX_ErrorIncorrectStateOperation;
        }

        if((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone)
        {
            return err;
        }

        if(strcmp((char *) pComponentRole->cRole, ISP_COMP_ROLE))
        {
            return OMX_ErrorBadParameter;
        }
    }
    break;

    case OMX_IndexParamPortDefinition:
    {
        pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *) ComponentParameterStructure;
        portIndex = pPortDef->nPortIndex;
        
        err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
            return err;
        }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];
        pPort->sPortParam.nBufferCountActual = pPortDef->nBufferCountActual;
        
        OMXDBUG(OMXDBUG_PARAM, "OMX_IndexParamPortDefinition: %d,%d,%d,%d\n", portIndex, 
            pPortDef->format.video.nFrameWidth,pPortDef->format.video.nFrameHeight, pPortDef->format.video.nStride);
        
        //  Copy stuff from OMX_VIDEO_PORTDEFINITIONTYPE structure

        // Read-only field by spec
        if(portIndex == OMX_BASE_FILTER_VIDEO_INDEX)
        {
            int err_res = 0;
            
#ifndef _OPENMAX_V1_2_
            if(pPortDef->format.video.cMIMEType != NULL)
            {
                strcpy(pPort->sPortParam.format.video.cMIMEType , pPortDef->format.video.cMIMEType);
            }
#endif
            pPort->sPortParam.format.video.nFrameWidth  = pPortDef->format.video.nFrameWidth;
            pPort->sPortParam.format.video.nFrameHeight = pPortDef->format.video.nFrameHeight;
            pPort->sPortParam.format.video.nBitrate     = pPortDef->format.video.nBitrate;
            pPort->sPortParam.format.video.xFramerate   = pPortDef->format.video.xFramerate;
            pPort->sPortParam.format.video.bFlagErrorConcealment = pPortDef->format.video.bFlagErrorConcealment;
            pPort->sPortParam.format.video.eCompressionFormat = pPortDef->format.video.eCompressionFormat;
            pPort->sPortParam.format.video.eColorFormat = pPortDef->format.video.eColorFormat;
            // stride, slice height, min buffer size
            pPort->sPortParam.format.video.nStride      = pPortDef->format.video.nStride;
            pPort->sPortParam.format.video.nSliceHeight = pPort->sPortParam.format.video.nFrameHeight;  //  No support for slices yet
#ifdef RAW_ALIGN
            pPort->sPortParam.nBufferSize = (OMX_U32) abs(pPort->sPortParam.format.video.nStride) * pPort->sPortParam.format.video.nSliceHeight * 3 / 2 + 4096;
#else
            pPort->sPortParam.nBufferSize = (OMX_U32) abs(pPort->sPortParam.format.video.nStride) * pPort->sPortParam.format.video.nSliceHeight * 3 / 2;
#endif

            pPort->nInputW = pPortDef->format.video.nFrameWidth;
            pPort->nInputH = pPortDef->format.video.nFrameHeight;

            pPort->bMJPEG_Enable  = 0;
            if(pPort->pCapInfo->ulThumbResCount == 1 && pPort->sPortParam.format.video.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
            {
                pPort->bMJPEG_Enable  = 1;
            }

            err_res = get_input_res(hComponent, pPortDef->format.video.nFrameWidth, pPortDef->format.video.nFrameHeight, &pPort->nInputW, &pPort->nInputH, 0);
            if(err_res == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "omx camera not support %d\n", pPortDef->format.video.nFrameWidth);
                return OMX_ErrorBadParameter;
            }
            if(err_res == 1)
            {
                pPort->bResizeEnable = 1;
            }
            else
            {
                pPort->bResizeEnable = 0;
            }

            if((pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) || \
                    (pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar))
            {
#ifdef RAW_ALIGN
                pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 3 / 2 + 4096;
#else
                pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 3 / 2;
#endif

                if(pPort->bMJPEG_Enable)
                { pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 2; }
            }
            else if((pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV422SemiPlanar) || \
                    (pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV422Planar) || \
                    (pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYCbYCr) || \
                    (pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYCrYCb))
            {
#ifdef RAW_ALIGN
                pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 2 + 4096;
#else
                pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 2;
#endif
            }
        }
        else if(OMX_BASE_FILTER_IMAGE_INDEX == portIndex)
        {
            int err_res = 0;
            
#ifndef _OPENMAX_V1_2_
            if(pPortDef->format.image.cMIMEType != NULL)
            {
                strcpy(pPort->sPortParam.format.image.cMIMEType , pPortDef->format.image.cMIMEType);
            }
#endif
            pPort->sPortParam.format.image.nFrameWidth  = pPortDef->format.image.nFrameWidth;
            pPort->sPortParam.format.image.nFrameHeight = pPortDef->format.image.nFrameHeight;
            pPort->sPortParam.format.image.bFlagErrorConcealment = pPortDef->format.image.bFlagErrorConcealment;
            pPort->sPortParam.format.image.eCompressionFormat = pPortDef->format.image.eCompressionFormat;
            pPort->sPortParam.format.image.eColorFormat = pPortDef->format.image.eColorFormat;
            // stride, slice height, min buffer size
            pPort->sPortParam.format.image.nStride      = pPortDef->format.image.nStride;
            pPort->sPortParam.format.image.nSliceHeight = pPort->sPortParam.format.image.nFrameHeight;  //  No support for slices yet
#ifdef RAW_ALIGN
            pPort->sPortParam.nBufferSize = (OMX_U32) abs(pPort->sPortParam.format.image.nStride) * pPort->sPortParam.format.image.nSliceHeight * 3 / 2 + 4096;
#else
            pPort->sPortParam.nBufferSize = (OMX_U32) abs(pPort->sPortParam.format.image.nStride) * pPort->sPortParam.format.image.nSliceHeight * 3 / 2;
#endif

            pPort->nInputW = pPortDef->format.image.nFrameWidth;
            pPort->nInputH = pPortDef->format.image.nFrameHeight;

            pPort->bMJPEG_Enable  = 0;
            if(pPort->pCapInfo->ulThumbResCount == 1 && pPort->sPortParam.format.image.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
            {
                pPort->bMJPEG_Enable  = 1;
            }

            err_res = get_input_res(hComponent, pPortDef->format.image.nFrameWidth, pPortDef->format.image.nFrameHeight, &pPort->nInputW, &pPort->nInputH, 1);
            if(err_res == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "omx camera not support %d\n", pPortDef->format.image.nFrameWidth);
                return OMX_ErrorBadParameter;
            }
            if(err_res == 1)
            {
                pPort->bResizeEnable = 1;
            }
            else
            {
                pPort->bResizeEnable = 0;
            }

            if((pPortDef->format.image.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) || \
                    (pPortDef->format.image.eColorFormat == OMX_COLOR_FormatYUV420Planar))
            {
#ifdef RAW_ALIGN
                pPort->sPortParam.nBufferSize = pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 3 / 2 + 4096;
#else
                pPort->sPortParam.nBufferSize = pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 3 / 2;
#endif

                if(pPort->bMJPEG_Enable)
                { pPort->sPortParam.nBufferSize = pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 2; }
            }
            else if((pPortDef->format.image.eColorFormat == OMX_COLOR_FormatYUV422SemiPlanar) || \
                    (pPortDef->format.image.eColorFormat == OMX_COLOR_FormatYUV422Planar) || \
                    (pPortDef->format.image.eColorFormat == OMX_COLOR_FormatYCbYCr) || \
                    (pPortDef->format.image.eColorFormat == OMX_COLOR_FormatYCrYCb))
            {
#ifdef RAW_ALIGN
                pPort->sPortParam.nBufferSize = pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 2 + 4096;
#else
                pPort->sPortParam.nBufferSize = pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 2;
#endif
            }
        }
        else
        {
            return OMX_ErrorBadParameter;
        }
    }
    break;

    case OMX_IndexParamImagePortFormat:
    {
        //  FIXME: How do we handle the nIndex member?
        OMX_IMAGE_PARAM_PORTFORMATTYPE *pImagePortFormat = (OMX_IMAGE_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        portIndex = pImagePortFormat->nPortIndex;
        err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pImagePortFormat, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));

        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
            return err;
        }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];

        if(pImagePortFormat->eCompressionFormat != OMX_IMAGE_CodingUnused)
        {
            OMXDBUG(OMXDBUG_ERR, "ImagePortFormat Error=%d,%d\n", __LINE__, err);
            return OMX_ErrorBadParameter;
        }

        pPort->sImageParam.eCompressionFormat = pImagePortFormat->eCompressionFormat;
        pPort->sImageParam.eColorFormat       = pImagePortFormat->eColorFormat;
        pPort->sPortParam.format.image.eColorFormat = pPort->sImageParam.eColorFormat;
        pPort->bMJPEG_Enable  = 0;

        if(pPort->pCapInfo->ulThumbResCount == 1 && pPort->sPortParam.format.image.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
        {
            pPort->bMJPEG_Enable  = 1;
        }

        if((pPort->sImageParam.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) || \
                (pPort->sImageParam.eColorFormat == OMX_COLOR_FormatYUV420Planar))
        {
#ifdef RAW_ALIGN
            pPort->sPortParam.nBufferSize = pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 3 / 2 + 4096;
#else
            pPort->sPortParam.nBufferSize = pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 3 / 2;
#endif

            if(pPort->bMJPEG_Enable)
            { pPort->sPortParam.nBufferSize = pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 2; }
        }
        else if((pPort->sImageParam.eColorFormat == OMX_COLOR_FormatYUV422SemiPlanar) || \
                (pPort->sImageParam.eColorFormat == OMX_COLOR_FormatYUV422Planar) || \
                (pPort->sImageParam.eColorFormat == OMX_COLOR_FormatYCbYCr) || \
                (pPort->sImageParam.eColorFormat == OMX_COLOR_FormatYCrYCb))
        {
#ifdef RAW_ALIGN
            pPort->sPortParam.nBufferSize = pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 2 + 4096;
#else
            pPort->sPortParam.nBufferSize = pPort->sPortParam.format.image.nFrameWidth * pPort->sPortParam.format.image.nFrameHeight * 2;
#endif
        }
    }
    break;

    case OMX_IndexParamVideoPortFormat:
    {
        pVideoPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        portIndex = pVideoPortFormat->nPortIndex;
        err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pVideoPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));

        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
            return err;
        }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];
        pPort->sVideoParam.xFramerate         = pVideoPortFormat->xFramerate;
        pPort->sVideoParam.eCompressionFormat = pVideoPortFormat->eCompressionFormat;
        pPort->sVideoParam.eColorFormat       = pVideoPortFormat->eColorFormat;
        pPort->sPortParam.format.video.eColorFormat = pPort->sVideoParam.eColorFormat;
        OMXDBUG(OMXDBUG_ERR, "In color fmt %x", pPort->sVideoParam.eColorFormat);
        pPort->bMJPEG_Enable  = 0;

        if(pPort->pCapInfo->ulThumbResCount == 1 && pPort->sPortParam.format.image.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
        {
            pPort->bMJPEG_Enable  = 1;
        }

        if((pPort->sVideoParam.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) || \
                (pPort->sVideoParam.eColorFormat == OMX_COLOR_FormatYUV420Planar))
        {
#ifdef RAW_ALIGN
            pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 3 / 2 + 4096;
#else
            pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 3 / 2;
#endif

            if(pPort->bMJPEG_Enable)
            { pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 2; }
        }
        else if((pPort->sVideoParam.eColorFormat == OMX_COLOR_FormatYUV422SemiPlanar) || \
                (pPort->sVideoParam.eColorFormat == OMX_COLOR_FormatYUV422Planar) || \
                (pPort->sVideoParam.eColorFormat == OMX_COLOR_FormatYCrYCb) || \
                (pPort->sVideoParam.eColorFormat == OMX_COLOR_FormatYCbYCr))
        {
#ifdef RAW_ALIGN
            pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 2 + 4096;
#else
            pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 2;
#endif
        }
    }
    break;

    case OMX_ACT_IndexParamSensorSelect:
    {
        OMX_PARAM_SENSORSELECTTYPE *pSensorType = (OMX_PARAM_SENSORSELECTTYPE *)ComponentParameterStructure;
        omx_base_camera_video_PortType *pPortOther = NULL;
        int bRaw = 0;
        int b3d = 0;
        int err = 0;
        void *pApplicationPrivate = NULL;
        camera_module_info_t pModeInfo;
        
        OMXDBUG(OMXDBUG_PARAM, "OMX_ACT_IndexParamSensorSelect %d,%d,%d,%d\n", 
            pSensorType->eSensor, gomxcam_bDoneQuery[pSensorType->eSensor], gomxcam_bIsInited, pSensorType->nPortIndex);

        /**
                * 此处按需求是仅仅支持单sensor输入，port端的定义，限制了仅支持
                * Viewfinder 和 image capture
                */
        portIndex = pSensorType->nPortIndex;
        if(portIndex == OMX_ALL)
        {
            pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
            pPortOther->nSensorSelect = pSensorType->eSensor;
            omx_camera_component_Private->nSensorSelect[OMX_BASE_FILTER_IMAGE_INDEX] = pSensorType->eSensor;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
            pPort->nSensorSelect = pSensorType->eSensor;
            omx_camera_component_Private->nSensorSelect[OMX_BASE_FILTER_VIDEO_INDEX] = pSensorType->eSensor;
            pPort->pCapInfo = omx_camera_component_Private->pCapInfo[OMX_BASE_FILTER_VIDEO_INDEX];
            //pSensorType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
            pPortOther->pCapInfo = pPort->pCapInfo;
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];
            pPort->nSensorSelect = pSensorType->eSensor;
            omx_camera_component_Private->nSensorSelect[portIndex] = pSensorType->eSensor;
            pPort->pCapInfo = omx_camera_component_Private->pCapInfo[pSensorType->eSensor];

            pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPortOther->nSensorSelect = pSensorType->eSensor;
            omx_camera_component_Private->nSensorSelect[1 - portIndex] = pSensorType->eSensor;
            pPortOther->pCapInfo = pPort->pCapInfo;
        }

        OMXDBUG(OMXDBUG_PARAM, "Select SensorType %d,%d\n", (int)pPort->nSensorSelect, pPort->pCapInfo->ulWhiteBalanceCount);

        if((omx_camera_component_Private->pCapInfo[0]->ulPreviewResCount == 0) && (pSensorType->eSensor == OMX_PrimarySensor))
        {
            pModeInfo.supportParam[OMX_PrimarySensor] = omx_camera_component_Private->pCapInfo[0];

            if(gomxcam_bIsInited == 1 && gomxcam_bDoneQuery[pSensorType->eSensor] == 1)
            {
                memcpy(omx_camera_component_Private->pCapInfo[0], &gomxcam_OMX_CAPS[pSensorType->eSensor], sizeof(OMX_ACT_CAPTYPE));
                omx_camera_component_Private->nModuleType[0] = pSensorType->eSensor;
                OMXDBUG(OMXDBUG_ERR, "gomxcam_bDoneQuery copy 0\n");
            }
            else
            {
                err = omx_camera_component_Private->camera_module_query(&pModeInfo, OMX_PrimarySensor,pSensorType->uvcmode);

                if(err == -1)
                {
                    OMXDBUG(OMXDBUG_ERR, "open failed by OMX_PrimarySensor !!\n");
                    omx_camera_component_Private->pCapInfo[0]->ulPreviewResCount = 0;
                    //return OMX_ErrorBadParameter;
                }
                else
                {
                    omx_camera_component_Private->Dgain_th[0] = pModeInfo.mDgain_th[0];
                    omx_camera_component_Private->nModuleType[0] = pModeInfo.supportType[0];

                    if(gomxcam_bDoneQuery[pSensorType->eSensor] == 0)
                    {
                        memcpy(&gomxcam_OMX_CAPS[pSensorType->eSensor], omx_camera_component_Private->pCapInfo[0], sizeof(OMX_ACT_CAPTYPE));
                        gomxcam_bDoneQuery[pSensorType->eSensor] = 1;
                        OMXDBUG(OMXDBUG_ERR, "gomxcam_bDoneQuery 0\n");
                    }
                }
            }

            OMXDBUG(OMXDBUG_ERR, "Select OMX_PrimarySensor %d\n", (int)pSensorType->nPortIndex);
        }

        if((omx_camera_component_Private->pCapInfo[1]->ulPreviewResCount == 0) && (pSensorType->eSensor == OMX_SecondarySensor))
        {
            pModeInfo.supportParam[OMX_SecondarySensor] = omx_camera_component_Private->pCapInfo[1];

            if(gomxcam_bIsInited == 1 && gomxcam_bDoneQuery[pSensorType->eSensor] == 1)
            {
                memcpy(omx_camera_component_Private->pCapInfo[1], &gomxcam_OMX_CAPS[pSensorType->eSensor], sizeof(OMX_ACT_CAPTYPE));
                omx_camera_component_Private->nModuleType[1] = pSensorType->eSensor;
                OMXDBUG(OMXDBUG_ERR, "gomxcam_bDoneQuery copy 1\n");
            }
            else
            {
                err = omx_camera_component_Private->camera_module_query(&pModeInfo, OMX_SecondarySensor,pSensorType->uvcmode);

                if(err == -1)
                {
                    OMXDBUG(OMXDBUG_ERR, "open failed by OMX_PrimarySensor !!\n");
                    omx_camera_component_Private->pCapInfo[1]->ulPreviewResCount = 0;
                    //return OMX_ErrorBadParameter;
                }
                else
                {
                    omx_camera_component_Private->Dgain_th[1] = pModeInfo.mDgain_th[1];
                    omx_camera_component_Private->nModuleType[1] = pModeInfo.supportType[1];

                    if(gomxcam_bDoneQuery[pSensorType->eSensor] == 0)
                    {
                        memcpy(&gomxcam_OMX_CAPS[pSensorType->eSensor], omx_camera_component_Private->pCapInfo[1], sizeof(OMX_ACT_CAPTYPE));
                        gomxcam_bDoneQuery[pSensorType->eSensor] = 1;
                        OMXDBUG(OMXDBUG_ERR, "gomxcam_bDoneQuery 1\n");
                    }
                }
            }

            OMXDBUG(OMXDBUG_ERR, "Select OMX_SecondarySensor %d\n", (int)pSensorType->nPortIndex);
        }


        if(omx_camera_component_Private->nModuleType[pSensorType->eSensor] == YUV)
        {
            bRaw = YUV;
        }
        else if(omx_camera_component_Private->nModuleType[pSensorType->eSensor] == RAW8)
        {
            bRaw = RAW8;
        }
        else if(omx_camera_component_Private->nModuleType[pSensorType->eSensor] == RAW_3D_NORMAL || \
                omx_camera_component_Private->nModuleType[pSensorType->eSensor] == RAW_3D_MIPI)
        {
            //3D Raw，目前没有支持
            OMXDBUG(OMXDBUG_ERR, " Warning::RAW 3D in not support Here,maybe exception soon!\n");
            bRaw = omx_camera_component_Private->nModuleType[pSensorType->eSensor];
            b3d = 1;
        }

        OMXDBUG(OMXDBUG_PARAM, "Param-SensorType %i\n", bRaw);

        if((pPortOther->nSensorSelect == (int)pSensorType->eSensor) && pPortOther->omx_camera->pCameraPrivate)
        {
            pApplicationPrivate = pPort->omx_camera->pApplicationPrivate;
            memcpy(pPort->omx_camera, pPortOther->omx_camera, sizeof(OMX_CAMERATYPE));
            pPort->omx_camera->pApplicationPrivate = pApplicationPrivate;
            pPort->bCopy = OMX_TRUE;
        }
        else
        {
            int bfoucus_support = pPort->pCapInfo->ulFocusModeCount > 0;

            //check is the raw or yuv module
            if(bRaw == RAW8)
            {
                if(omx_camera_component_Private->camera_isp_base_Constructor)
                {
                    err |= omx_camera_component_Private->camera_isp_base_Constructor(pPort->omx_camera);
                    pPort->camera_isp_base_Destructor = omx_camera_component_Private->camera_isp_base_Destructor;

                    if(b3d)
                    { err |= pPort->omx_camera->omx_camera_open(pPort->omx_camera, pSensorType->eSensor, RAW_3D_NORMAL | (bfoucus_support << 16),pSensorType->uvcmode); }
                    else
                    {
                        int sensor_id;
                        wdog_cb_t cb_info;
                        err |= pPort->omx_camera->omx_camera_open(pPort->omx_camera, pSensorType->eSensor, RAW8 | (bfoucus_support << 16),pSensorType->uvcmode);

                        cb_info.func = tickle_watch_dog;
                        cb_info.handle = omx_camera_component_Private->wdog_handle;
                        err |= pPort->omx_camera->omx_camera_setcb(pPort->omx_camera, &cb_info);
                        
                        pPort->omx_camera->omx_camera_getctl(pPort->omx_camera, CAM_GET_SENSOR_ID, &sensor_id, 0);
                        err |= imxctl_init(omx_camera_component_Private->imxctl_handle, sensor_id);
                    }

                    pPort->nSensorType = 0;
                }
            }
            else
            {
                if(omx_camera_component_Private->camera_direct_base_Constructor)
                {
                    wdog_cb_t cb_info;
                    err |= omx_camera_component_Private->camera_direct_base_Constructor(pPort->omx_camera);
                    pPort->camera_direct_base_Destructor = omx_camera_component_Private->camera_direct_base_Destructor;
                    err |= pPort->omx_camera->omx_camera_open(pPort->omx_camera, pSensorType->eSensor, YUV,pSensorType->uvcmode);

                    cb_info.func = tickle_watch_dog;
                    cb_info.handle = omx_camera_component_Private->wdog_handle;
                    err |= pPort->omx_camera->omx_camera_setcb(pPort->omx_camera, &cb_info);
                    
                    pPort->nSensorType = 1;
                    pPort->bCopy = OMX_FALSE;
                }
            }

            pPortOther->nSensorType = pPort->nSensorType;

            if(pPortOther->omx_camera->pCameraPrivate == NULL)
            {
                pApplicationPrivate = pPortOther->omx_camera->pApplicationPrivate;
                memcpy(pPortOther->omx_camera, pPort->omx_camera, sizeof(OMX_CAMERATYPE));
                pPortOther->omx_camera->pApplicationPrivate = pApplicationPrivate;
                pPortOther->bCopy = OMX_TRUE;
            }
        }

        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "SensorSelect Error=%d,%d\n", __LINE__, err);
            return err;
        }
    }
    break;

    case OMX_IndexParamCommonSensorMode:
    {
        OMX_PARAM_SENSORMODETYPE  *pType = (OMX_PARAM_SENSORMODETYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
            memcpy(pPort->pSensorMode, pType, sizeof(OMX_PARAM_SENSORMODETYPE));
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
            memcpy(pPort->pSensorMode, pType, sizeof(OMX_PARAM_SENSORMODETYPE));
            OMXDBUG(OMXDBUG_VERB, "OMX_IndexParamCommonSensorMode process Init All!!\n");
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
            OMXDBUG(OMXDBUG_VERB, "OMX_IndexParamCommonSensorMode process now!!\n");

            if((pType->nPortIndex == OMX_BASE_FILTER_VIDEO_INDEX && pType->bOneShot == OMX_TRUE) || \
                    (pType->nPortIndex == OMX_BASE_FILTER_IMAGE_INDEX && pType->bOneShot == OMX_FALSE))
            {
                OMXDBUG(OMXDBUG_VERB, "Not Support This Mode!!Be Careful!! \n");
                //return OMX_ErrorBadParameter;
            }

            memcpy(pPort->pSensorMode, pType, sizeof(OMX_PARAM_SENSORMODETYPE));
        }
    }
    break;

    case OMX_ACT_IndexConfigVideoParam:
    {
        OMX_ACT_CONFIG_VIDEOPARAM *pType = (OMX_ACT_CONFIG_VIDEOPARAM *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
            pPort->pSensorMode->nFrameRate = pType->nFramerate;
            pPort->sPortParam.format.video.xFramerate = pType->nFramerate;
            pPort->config_idx |= (1 << 1);

            if(pPort->omx_camera == NULL)
            {
                OMXDBUG(OMXDBUG_ERR, "ERR:Please set sensor type first!! \n");;
                return OMX_ErrorBadParameter;
            }

            OMXDBUG(OMXDBUG_ERR, "VideoParam nFramerate %d\n", pType->nFramerate);
            //pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
            //err = omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
            err = pPort->omx_camera->omx_camera_setfps(pPort->omx_camera, pType->nFramerate >> 16, pPort->pSensorMode->bOneShot);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                return OMX_ErrorHardware;
            }

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
            pPort->pSensorMode->nFrameRate = pType->nFramerate;
            pPort->sPortParam.format.video.xFramerate = pType->nFramerate;
            pPort->config_idx |= (1 << 1);

            if(pPort->bStreamOn == OMX_TRUE)
            {
#if 0
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#endif
                err = pPort->omx_camera->omx_camera_setfps(pPort->omx_camera, pType->nFramerate >> 16, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return OMX_ErrorHardware;
                }
            }

        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];
#if 0
            err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                return err;
            }

#endif
            err = pPort->omx_camera->omx_camera_setfps(pPort->omx_camera, pType->nFramerate >> 16, pPort->pSensorMode->bOneShot);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                return OMX_ErrorHardware;
            }

            pPort->pSensorMode->nFrameRate = pType->nFramerate;
            pPort->sPortParam.format.video.xFramerate = pType->nFramerate;
            pPort->config_idx |= (1 << 1);
        }

        OMXDBUG(OMXDBUG_ERR, "ERR:OMX_ACT_IndexConfigVideoParam process frame rate now!!\n");

    }
    break;

    case OMX_IndexParameterStoreMediaData:
    {
        //Port端的Buffer管理方式,是结构还是物理地址输入
        StoreMetaDataInBuffersParams *pMediaType = (StoreMetaDataInBuffersParams *)ComponentParameterStructure;
        portIndex = pMediaType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
            pPort->bStoreMediadata = pMediaType->bStoreMetaData;
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
            pPort->bStoreMediadata = pMediaType->bStoreMetaData;
            OMXDBUG(OMXDBUG_VERB, "bStoreMediadata...%d\n", pMediaType->bStoreMetaData);
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];
            pPort->bStoreMediadata = pMediaType->bStoreMetaData;
        }
    }
    break;
    
#if HDR_PROCESS
    case OMX_ACT_IndexConfigHDRParam:
    {
        OMX_ACT_CONFIG_HDR_EVParams  *pType = (OMX_ACT_CONFIG_HDR_EVParams *)ComponentParameterStructure;
        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];
        portIndex = pType->nPortIndex;
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_HDR_EV, (unsigned long)pType, pPort->pSensorMode->bOneShot);

        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
            return err;
        }

        memcpy(pPort->pHdrParam, pType, sizeof(OMX_ACT_CONFIG_HDR_EVParams));
    }
    break;
#endif

    /* 前处理相关函数 */
    case OMX_IndexParamFlashControl:
    case OMX_IndexConfigFlashControl:
    {
        int i;
        int bSupported = -1;
        OMX_IMAGE_PARAM_FLASHCONTROLTYPE  *pType = (OMX_IMAGE_PARAM_FLASHCONTROLTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            for(i = 0; i < pPort->pCapInfo->ulFlashCount; i++)
            {
                if(pPort->pCapInfo->eFlashModes[i] == pType->eFlashControl)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

#if 1
            err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLASH_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                return err;
            }

            memcpy(pPort->pFlashType, pType, sizeof(OMX_IMAGE_PARAM_FLASHCONTROLTYPE));
            pPort->pFlashType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#endif

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            for(i = 0; i < pPort->pCapInfo->ulFlashCount; i++)
            {
                if(pPort->pCapInfo->eFlashModes[i] == pType->eFlashControl)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

#if 1
            err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLASH_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                return err;
            }

            memcpy(pPort->pFlashType, pType, sizeof(OMX_IMAGE_PARAM_FLASHCONTROLTYPE));
            pPort->pFlashType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#endif
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];

            for(i = 0; i < pPort->pCapInfo->ulFlashCount; i++)
            {
                if(pPort->pCapInfo->eFlashModes[i] == pType->eFlashControl)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

#if 1
            //Not Support yet
            err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLASH_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                return err;
            }

            memcpy(pPort->pFlashType, pType, sizeof(OMX_IMAGE_PARAM_FLASHCONTROLTYPE));
#endif
        }

    }
    break;

    case OMX_ACT_IndexConfigAGCExposureValue:/**< reference: OMX_ACT_CONFIG_AGCVALUE */
    {
        OMXDBUG(OMXDBUG_ERR, "Not Support OMX_ACT_IndexConfigAGCExposureValue Set\n");
    }
    break;

    case OMX_ACT_IndexConfigFlashStrobeValue:/**< reference: OMX_ACT_CONFIG_FlashStrobeParams */
    {
        OMX_ACT_CONFIG_FlashStrobeParams  *pType = (OMX_ACT_CONFIG_FlashStrobeParams *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }

        if(portIndex == OMX_ALL)
        {
            return OMX_ErrorBadParameter;
        }
        else
        {
            portIndex = pType->nPortIndex;
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];
            err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLASH_STROBE_MODE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                return err;
            }

            memcpy(pPort->act_flashstrobe, pType, sizeof(OMX_ACT_CONFIG_FlashStrobeParams));
        }
    }
    break;
#if 0

    case OMX_IndexConfigCommonColorFormatConversion://OMX_IndexConfigCommonColorEnhancement:
    {
        OMX_CONFIG_COLORCONVERSIONTYPE  *pType = (OMX_CONFIG_COLORCONVERSIONTYPE *)ComponentParameterStructure;
        omx_base_camera_video_PortType *pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        memcpy(pPort->pColorFix, pType, sizeof(OMX_CONFIG_COLORCONVERSIONTYPE));
        err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);
    }
    break;
#endif

    case OMX_IndexConfigCommonColorEnhancement://OMX_IndexConfigCommonColorEnhancement:
    {
        OMX_CONFIG_COLORENHANCEMENTTYPE  *pType = (OMX_CONFIG_COLORENHANCEMENTTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
            memcpy(pPort->pColorEft, pType, sizeof(OMX_CONFIG_COLORENHANCEMENTTYPE));
            pPort->config_idx |= (1 << 2);
            pPort->pColorEft->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
            memcpy(pPort->pColorEft, pType, sizeof(OMX_CONFIG_COLORENHANCEMENTTYPE));
            pPort->config_idx |= (1 << 2);
            pPort->pColorEft->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pColorEft);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_COLORFIX_MANUAL, (unsigned long)pPort->pColorEft, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "ColorEnhancement Error=%d,%d\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pColorEft);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_COLORFIX_MANUAL, (unsigned long)pPort->pColorEft, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "ColorEnhancement Error=%d,%d\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];
            memcpy(pPort->pColorEft, pType, sizeof(OMX_CONFIG_COLORENHANCEMENTTYPE));
            pPort->config_idx |= (1 << 2);

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPortOther->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_COLORFIX_MANUAL, (unsigned long)pPort->pColorEft, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "ColorEnhancement Error=%d,%d\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }

    }
    break;

    case OMX_IndexConfigCommonImageFilter:
    {
        int i;
        int bSupported = -1;
        OMX_CONFIG_IMAGEFILTERTYPE  *pType = (OMX_CONFIG_IMAGEFILTERTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            for(i = 0; i < pPort->pCapInfo->ulColorEffectCount; i++)
            {
                if(pPort->pCapInfo->eColorEffects[i] == pType->eImageFilter)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            memcpy(pPort->pImageFilter, pType, sizeof(OMX_CONFIG_IMAGEFILTERTYPE));
            pPort->config_idx |= (1 << 3);
            pPort->pImageFilter->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            for(i = 0; i < pPort->pCapInfo->ulColorEffectCount; i++)
            {
                if(pPort->pCapInfo->eColorEffects[i] == pType->eImageFilter)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            memcpy(pPort->pImageFilter, pType, sizeof(OMX_CONFIG_IMAGEFILTERTYPE));
            pPort->config_idx |= (1 << 3);
            pPort->pImageFilter->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pImageFilter);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_COLORFIX, (unsigned long)pPort->pImageFilter, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "ImageFilter Error=%d,%d\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pImageFilter);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_COLORFIX, (unsigned long)pPort->pImageFilter, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "ImageFilter Error=%d,%d\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];

            for(i = 0; i < pPort->pCapInfo->ulColorEffectCount; i++)
            {
                if(pPort->pCapInfo->eColorEffects[i] == pType->eImageFilter)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            memcpy(pPort->pImageFilter, pType, sizeof(OMX_CONFIG_IMAGEFILTERTYPE));
            pPort->config_idx |= (1 << 3);

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPortOther->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_COLORFIX, (unsigned long)pPort->pImageFilter, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "ImageFilter Error=%d,%d\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }

    }
    break;

    case OMX_IndexConfigCommonMirror:
    {
        OMX_CONFIG_MIRRORTYPE  *pType = (OMX_CONFIG_MIRRORTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
            memcpy(pPort->pImageMirror, pType, sizeof(OMX_CONFIG_MIRRORTYPE));
            pPort->config_idx |= (1 << 4);
            pPort->pImageMirror->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
            memcpy(pPort->pImageMirror, pType, sizeof(OMX_CONFIG_MIRRORTYPE));
            pPort->config_idx |= (1 << 4);
            pPort->pImageMirror->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pImageMirror);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLIP, (unsigned long)pPort->pImageMirror, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Mirror Error=%d,%d\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pImageMirror);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLIP, (unsigned long)pPort->pImageMirror, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Mirror Error=%d,%d\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPortOther->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLIP, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Mirror Error=%d,%d\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            memcpy(pPort->pImageMirror, pType, sizeof(OMX_CONFIG_MIRRORTYPE));
            pPort->config_idx |= (1 << 4);
        }
    }
    break;

    case OMX_IndexConfigCommonOpticalZoom:
    {
        OMX_CONFIG_SCALEFACTORTYPE  *pType = (OMX_CONFIG_SCALEFACTORTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            if(pPort->pCapInfo->xMaxWidthZoom == 0) { return OMX_ErrorBadParameter; }

            memcpy(pPort->pOpticZoomType, pType, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
            pPort->pOpticZoomType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            if(pPort->pCapInfo->xMaxWidthZoom == 0) { return OMX_ErrorBadParameter; }

            memcpy(pPort->pOpticZoomType, pType, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
            pPort->pOpticZoomType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pOpticZoomType);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                //err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera,CAM_SET_FLIP,(unsigned long)pPort->pImageMirror,pPort->pSensorMode->bOneShot);
                //if(err!=OMX_ErrorNone) {
                //  OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n",__func__,err);
                //  return OMX_ErrorHardware;
                //  }
#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pOpticZoomType);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[portIndex];

            if(pPort->pCapInfo->xMaxWidthZoom == 0) { return OMX_ErrorBadParameter; }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPortOther->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#endif
            }

            memcpy(pPort->pOpticZoomType, pType, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
        }

    }
    break;

    case OMX_IndexConfigCommonWhiteBalance:
    {
        int i;
        int bSupported = -1;
        OMX_CONFIG_WHITEBALCONTROLTYPE  *pType = (OMX_CONFIG_WHITEBALCONTROLTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;
#if 1
        OMXDBUG(OMXDBUG_VERB, "wb idx %d\n", (unsigned int)portIndex);

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            for(i = 0; i < pPort->pCapInfo->ulWhiteBalanceCount; i++)
            {
                if(pPort->pCapInfo->eWhiteBalanceModes[i] == pType->eWhiteBalControl)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            memcpy(pPort->pWBType, pType, sizeof(OMX_CONFIG_WHITEBALCONTROLTYPE));
            pPort->config_idx |= (1 << 5);
            pPort->pWBType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            for(i = 0; i < pPort->pCapInfo->ulWhiteBalanceCount; i++)
            {
                if(pPort->pCapInfo->eWhiteBalanceModes[i] == pType->eWhiteBalControl)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            memcpy(pPort->pWBType, pType, sizeof(OMX_CONFIG_WHITEBALCONTROLTYPE));
            pPort->config_idx |= (1 << 5);
            pPort->pWBType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pWBType);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_WB_TYPE, (unsigned long)pPort->pWBType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pWBType);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_WB_TYPE, (unsigned long)pPort->pWBType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
            OMXDBUG(OMXDBUG_VERB, "CapInfo %x,%x\n", (unsigned long)pPort->pCapInfo, (unsigned long)pPortOther->pCapInfo);

            for(i = 0; i < pPort->pCapInfo->ulWhiteBalanceCount; i++)
            {
                if(pPort->pCapInfo->eWhiteBalanceModes[i] == pType->eWhiteBalControl)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPort->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_WB_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            memcpy(pPort->pWBType, pType, sizeof(OMX_CONFIG_WHITEBALCONTROLTYPE));
            pPort->config_idx |= (1 << 5);
        }

        OMXDBUG(OMXDBUG_VERB, "set WB....\n");
#endif
    }
    break;

    case OMX_IndexConfigImageWhiteBalanceLock:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            if(pPort->pCapInfo->bWhiteBalanceLockSupported == OMX_FALSE) { return OMX_ErrorBadParameter; }

            pPort->config_idx |= (1 << 6);
            pPort->bAWB_Lock = pType->eImageLock;
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            if(pPort->pCapInfo->bWhiteBalanceLockSupported == OMX_FALSE) { return OMX_ErrorBadParameter; }

            pPort->config_idx |= (1 << 6);
            pPort->bAWB_Lock = pType->eImageLock;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AWBLOCK, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AWBLOCK, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            if(pPort->pCapInfo->bWhiteBalanceLockSupported == OMX_FALSE) { return OMX_ErrorBadParameter; }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPort->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AWBLOCK, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            pPort->config_idx |= (1 << 6);
            pPort->bAWB_Lock = pType->eImageLock;
        }

    }
    break;

    case OMX_IndexConfigImageFocusLock:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            if(pPort->pCapInfo->bFocusLockSupported == OMX_FALSE) { return OMX_ErrorBadParameter; }

            pPort->bAF_Lock = pType->eImageLock;
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            if(pPort->pCapInfo->bFocusLockSupported == OMX_FALSE) { return OMX_ErrorBadParameter; }

            pPort->bAF_Lock = pType->eImageLock;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AFLOCK, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AFLOCK, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            pType->nPortIndex = OMX_ALL;
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            if(pPort->pCapInfo->bFocusLockSupported == OMX_FALSE) { return OMX_ErrorBadParameter; }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPort->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AFLOCK, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            pPort->bAF_Lock = pType->eImageLock;
        }

    }
    break;

    case OMX_IndexConfigImageExposureLock:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            if(pPort->pCapInfo->bExposureLockSupported == OMX_FALSE) { return OMX_ErrorBadParameter; }

            pPort->config_idx |= (1 << 7);
            pPort->bAE_Lock = pType->eImageLock;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            if(pPort->pCapInfo->bExposureLockSupported == OMX_FALSE) { return OMX_ErrorBadParameter; }

            pPort->config_idx |= (1 << 7);
            pPort->bAE_Lock = pType->eImageLock;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AELOCK, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AELOCK, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            pType->nPortIndex = OMX_ALL;
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            if(pPort->pCapInfo->bExposureLockSupported == OMX_FALSE) { return OMX_ErrorBadParameter; }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPort->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AELOCK, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            pPort->config_idx |= (1 << 7);
            pPort->bAE_Lock = pType->eImageLock;
        }

    }
    break;

    case OMX_IndexConfigCommonExposure:
    {
        int i;
        int bSupported = -1;
        OMX_CONFIG_EXPOSURECONTROLTYPE  *pType = (OMX_CONFIG_EXPOSURECONTROLTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;
#if 1

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            for(i = 0; i < pPort->pCapInfo->ulExposureModeCount; i++)
            {
                if(pPort->pCapInfo->eExposureModes[i] == pType->eExposureControl)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            memcpy(pPort->pExpType, pType, sizeof(OMX_CONFIG_EXPOSURECONTROLTYPE));
            pPort->config_idx |= (1 << 8);
            pPort->pExpType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            for(i = 0; i < pPort->pCapInfo->ulExposureModeCount; i++)
            {
                if(pPort->pCapInfo->eExposureModes[i] == pType->eExposureControl)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            if((int)pType->eExposureControl == OMX_ExposureControlActHDR)
            {
                pPort->bHdr_Enable = OMX_TRUE;
                OMXDBUG(OMXDBUG_ERR, "bHdr_Enable \n");
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_EXP_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }
            }
            else
            {
                pPort->bHdr_Enable = OMX_FALSE;
                pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_HDR, (unsigned long)0, pPort->pSensorMode->bOneShot);
            }

            memcpy(pPort->pExpType, pType, sizeof(OMX_CONFIG_EXPOSURECONTROLTYPE));
            pPort->config_idx |= (1 << 8);
            pPort->pExpType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pExpType);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_EXP_TYPE, (unsigned long)pPort->pExpType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pExpType);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_EXP_TYPE, (unsigned long)pPort->pExpType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            for(i = 0; i < pPort->pCapInfo->ulExposureModeCount; i++)
            {
                if(pPort->pCapInfo->eExposureModes[i] == pType->eExposureControl)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPort->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPort->bStreamOn == OMX_TRUE))
            {
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_EXP_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

            }

            memcpy(pPort->pExpType, pType, sizeof(OMX_CONFIG_EXPOSURECONTROLTYPE));
            pPort->config_idx |= (1 << 8);

            if(((int)pType->eExposureControl) == OMX_ExposureControlActHDR)
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
                pPort->bHdr_Enable = OMX_TRUE;
                OMXDBUG(OMXDBUG_ERR, "bHdr_Enable \n");
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_EXP_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }
            }
            else
            {
                pPort->bHdr_Enable = OMX_FALSE;
                pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_HDR, (unsigned long)0, pPort->pSensorMode->bOneShot);
            }

        }

#endif
    }
    break;

    case OMX_IndexConfigCommonExposureValue:
    {
        OMX_CONFIG_EXPOSUREVALUETYPE  *pType = (OMX_CONFIG_EXPOSUREVALUETYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            if(pPort->pCapInfo->xEVCompensationMax < pType->xEVCompensation || pPort->pCapInfo->xEVCompensationMin > pType->xEVCompensation)
            { return OMX_ErrorBadParameter; }

            memcpy(pPort->pExpVal, pType, sizeof(OMX_CONFIG_EXPOSUREVALUETYPE));
            pPort->config_idx |= (1 << 9);
            pPort->pExpVal->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            if(pPort->pCapInfo->xEVCompensationMax < pType->xEVCompensation || pPort->pCapInfo->xEVCompensationMin > pType->xEVCompensation)
            { return OMX_ErrorBadParameter; }

            memcpy(pPort->pExpVal, pType, sizeof(OMX_CONFIG_EXPOSUREVALUETYPE));
            pPort->config_idx |= (1 << 9);
            pPort->pExpVal->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pExpVal);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else

                if(pPort->pCapInfo->xEVCompensationMax > 0 && pPort->pCapInfo->xEVCompensationMax != pPort->pCapInfo->xEVCompensationMin)
                {
                    //OMX_CONFIG_BRIGHTNESSTYPE  pTypeBright;
                    //pTypeBright.nBrightness = pType->xEVCompensation>>16;
                    OMXDBUG(OMXDBUG_ERR, "xEVCompensation set %d,%d", pType->xEVCompensation, __LINE__);
                    err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_EV, (unsigned long)pType, pPort->pSensorMode->bOneShot);
                }

                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_MET_AREA, (unsigned long)pPort->pExpVal->eMetering, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pExpVal);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else

                if(pPort->pCapInfo->xEVCompensationMax > 0 && pPort->pCapInfo->xEVCompensationMax != pPort->pCapInfo->xEVCompensationMin)
                {
                    //OMX_CONFIG_BRIGHTNESSTYPE  pTypeBright;
                    //pTypeBright.nBrightness = pType->xEVCompensation>>16;
                    err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_EV, (unsigned long)pType, pPort->pSensorMode->bOneShot);
                }

                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_MET_AREA, (unsigned long)pPort->pExpVal->eMetering, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            if(pPort->pCapInfo->xEVCompensationMax < pType->xEVCompensation || pPort->pCapInfo->xEVCompensationMin > pType->xEVCompensation)
            { return OMX_ErrorBadParameter; }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPort->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else

                if(pPort->pCapInfo->xEVCompensationMax > 0 && pPort->pCapInfo->xEVCompensationMax != pPort->pCapInfo->xEVCompensationMin)
                {
                    //OMX_CONFIG_BRIGHTNESSTYPE  pTypeBright;
                    //pTypeBright.nBrightness = pType->xEVCompensation>>16;
                    OMXDBUG(OMXDBUG_ERR, "xEVCompensation set %d,%d", pType->xEVCompensation, __LINE__);
                    err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_EV, (unsigned long)pType, pPort->pSensorMode->bOneShot);
                }

                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_MET_AREA, (unsigned long)pType->eMetering, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            memcpy(pPort->pExpVal, pType, sizeof(OMX_CONFIG_EXPOSUREVALUETYPE));
            pPort->config_idx |= (1 << 9);
        }
    }
    break;

    case OMX_IndexConfigCommonContrast:
    {
        OMX_CONFIG_CONTRASTTYPE  *pType = (OMX_CONFIG_CONTRASTTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            if(pPort->pCapInfo->bContrastSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            memcpy(pPort->pContrast, pType, sizeof(OMX_CONFIG_CONTRASTTYPE));
            pPort->config_idx |= (1 << 10);
            pPort->pContrast->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            if(pPort->pCapInfo->bContrastSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            memcpy(pPort->pContrast, pType, sizeof(OMX_CONFIG_CONTRASTTYPE));
            pPort->config_idx |= (1 << 10);
            pPort->pContrast->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pContrast);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_CONSTRAST_LEVEL, (unsigned long)pPort->pContrast, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pContrast);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_CONSTRAST_LEVEL, (unsigned long)pPort->pContrast, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            if(pPort->pCapInfo->bContrastSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPort->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_CONSTRAST_LEVEL, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            memcpy(pPort->pContrast, pType, sizeof(OMX_CONFIG_CONTRASTTYPE));
            pPort->config_idx |= (1 << 10);
        }
    }
    break;

    case OMX_IndexConfigCommonBrightness:
    {
        OMX_CONFIG_BRIGHTNESSTYPE  *pType = (OMX_CONFIG_BRIGHTNESSTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            if(pPort->pCapInfo->bBrightnessSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            memcpy(pPort->pBright, pType, sizeof(OMX_CONFIG_BRIGHTNESSTYPE));
            pPort->config_idx |= (1 << 11);
            pPort->pBright->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            if(pPort->pCapInfo->bBrightnessSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            memcpy(pPort->pBright, pType, sizeof(OMX_CONFIG_BRIGHTNESSTYPE));
            pPort->config_idx |= (1 << 11);
            pPort->pBright->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pBright);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else

                if(pPort->pCapInfo->xBrightnessLevel.nMaxVal > 0)
                {
                    OMX_CONFIG_BRIGHTNESSTYPE  pTypeCur;

                    if(pPort->pCapInfo->xBrightnessLevel.nMinVal < 0)
                    { pTypeCur.nBrightness = pType->nBrightness * (pPort->pCapInfo->xBrightnessLevel.nMaxVal) / 100; }
                    else
                    { pTypeCur.nBrightness = pType->nBrightness * (pPort->pCapInfo->xBrightnessLevel.nMaxVal - pPort->pCapInfo->xBrightnessLevel.nMinVal) / 100 + pPort->pCapInfo->xBrightnessLevel.nMinVal; }


                    err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_BRIGHTNESS_LEVEL, (unsigned long)&pTypeCur, pPort->pSensorMode->bOneShot);
                }

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pBright);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else

                if(pPort->pCapInfo->xBrightnessLevel.nMaxVal > 0)
                {
                    OMX_CONFIG_BRIGHTNESSTYPE  pTypeCur;

                    if(pPort->pCapInfo->xBrightnessLevel.nMinVal < 0)
                    { pTypeCur.nBrightness = pType->nBrightness * (pPort->pCapInfo->xBrightnessLevel.nMaxVal) / 100; }
                    else
                    { pTypeCur.nBrightness = pType->nBrightness * (pPort->pCapInfo->xBrightnessLevel.nMaxVal - pPort->pCapInfo->xBrightnessLevel.nMinVal) / 100 + pPort->pCapInfo->xBrightnessLevel.nMinVal; }


                    err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_BRIGHTNESS_LEVEL, (unsigned long)&pTypeCur, pPort->pSensorMode->bOneShot);
                }

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            if(pPort->pCapInfo->bBrightnessSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPort->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else

                if(pPort->pCapInfo->xBrightnessLevel.nMaxVal > 0)
                {
                    OMX_CONFIG_BRIGHTNESSTYPE  pTypeCur;

                    if(pPort->pCapInfo->xBrightnessLevel.nMinVal < 0)
                    { pTypeCur.nBrightness = pType->nBrightness * (pPort->pCapInfo->xBrightnessLevel.nMaxVal) / 100; }
                    else
                    { pTypeCur.nBrightness = pType->nBrightness * (pPort->pCapInfo->xBrightnessLevel.nMaxVal - pPort->pCapInfo->xBrightnessLevel.nMinVal) / 100 + pPort->pCapInfo->xBrightnessLevel.nMinVal; }


                    err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_BRIGHTNESS_LEVEL, (unsigned long)&pTypeCur, pPort->pSensorMode->bOneShot);
                }

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            memcpy(pPort->pBright, pType, sizeof(OMX_CONFIG_BRIGHTNESSTYPE));
            pPort->config_idx |= (1 << 11);
        }
    }
    break;

    case OMX_IndexConfigCommonGamma:
    {
        //Suggest::Not Support
        OMX_CONFIG_GAMMATYPE  *pType = (OMX_CONFIG_GAMMATYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
            memcpy(pPort->pGamma, pType, sizeof(OMX_CONFIG_GAMMATYPE));
            pPort->pGamma->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
            memcpy(pPort->pGamma, pType, sizeof(OMX_CONFIG_GAMMATYPE));
            pPort->pGamma->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pGamma);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_GAMMA, (unsigned long)pPort->pGamma, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pGamma);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_GAMMA, (unsigned long)pPort->pGamma, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPort->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_GAMMA, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            memcpy(pPort->pGamma, pType, sizeof(OMX_CONFIG_GAMMATYPE));
        }

    }
    break;

    case OMX_IndexConfigCommonSaturation:
    {
        OMX_CONFIG_SATURATIONTYPE  *pType = (OMX_CONFIG_SATURATIONTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            if(pPort->pCapInfo->bSaturationSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            memcpy(pPort->pSat, pType, sizeof(OMX_CONFIG_SATURATIONTYPE));
            pPort->config_idx |= (1 << 12);
            pPort->pSat->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            if(pPort->pCapInfo->bSaturationSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            memcpy(pPort->pSat, pType, sizeof(OMX_CONFIG_SATURATIONTYPE));
            pPort->config_idx |= (1 << 12);
            pPort->pSat->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pSat);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else

                if(pPort->pCapInfo->xSaturationLevel.nMaxVal > 0)
                {
                    OMX_CONFIG_SATURATIONTYPE  pTypeCur;
                    pTypeCur.nSaturation = pType->nSaturation * (pPort->pCapInfo->xSaturationLevel.nMaxVal - pPort->pCapInfo->xSaturationLevel.nMinVal) / 100 + pPort->pCapInfo->xSaturationLevel.nMinVal;
                    err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_STA, (unsigned long)&pTypeCur, pPort->pSensorMode->bOneShot);
                }

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pSat);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else

                if(pPort->pCapInfo->xSaturationLevel.nMaxVal > 0)
                {
                    OMX_CONFIG_SATURATIONTYPE  pTypeCur;
                    pTypeCur.nSaturation = pType->nSaturation * (pPort->pCapInfo->xSaturationLevel.nMaxVal - pPort->pCapInfo->xSaturationLevel.nMinVal) / 100 + pPort->pCapInfo->xSaturationLevel.nMinVal;
                    err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_STA, (unsigned long)&pTypeCur, pPort->pSensorMode->bOneShot);
                }

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            if(pPort->pCapInfo->bSaturationSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPortOther->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else

                if(pPort->pCapInfo->xSaturationLevel.nMaxVal > 0)
                {
                    OMX_CONFIG_SATURATIONTYPE  pTypeCur;
                    pTypeCur.nSaturation = pType->nSaturation * (pPort->pCapInfo->xSaturationLevel.nMaxVal - pPort->pCapInfo->xSaturationLevel.nMinVal) / 100 + pPort->pCapInfo->xSaturationLevel.nMinVal;
                    err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_STA, (unsigned long)&pTypeCur, pPort->pSensorMode->bOneShot);
                }

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            memcpy(pPort->pSat, pType, sizeof(OMX_CONFIG_SATURATIONTYPE));
            pPort->config_idx |= (1 << 12);
        }

    }
    break;

    case OMX_IndexConfigFlickerRejection:
    {
        int i;
        int bSupported = -1;
        OMX_CONFIG_FLICKERREJECTIONTYPE  *pType = (OMX_CONFIG_FLICKERREJECTIONTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            for(i = 0; i < pPort->pCapInfo->ulFlickerCount; i++)
            {
                if(pPort->pCapInfo->eFlicker[i] == pType->eFlickerRejection)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            memcpy(pPort->pFlicktype, pType, sizeof(OMX_CONFIG_FLICKERREJECTIONTYPE));
            pPort->config_idx |= (1 << 13);
            pPort->pFlicktype->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            for(i = 0; i < pPort->pCapInfo->ulFlickerCount; i++)
            {
                if(pPort->pCapInfo->eFlicker[i] == pType->eFlickerRejection)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            memcpy(pPort->pFlicktype, pType, sizeof(OMX_CONFIG_FLICKERREJECTIONTYPE));
            pPort->config_idx |= (1 << 13);
            pPort->pFlicktype->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pFlicktype);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLICKER_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pFlicktype);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLICKER_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            for(i = 0; i < pPort->pCapInfo->ulFlickerCount; i++)
            {
                if(pPort->pCapInfo->eFlicker[i] == pType->eFlickerRejection)
                {
                    bSupported = 1;
                }
            }

            if(bSupported == -1)
            {
                OMXDBUG(OMXDBUG_ERR, "Not Support This PARAM\n");
                return OMX_ErrorBadParameter;
            }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPortOther->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_FLICKER_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            memcpy(pPort->pFlicktype, pType, sizeof(OMX_CONFIG_FLICKERREJECTIONTYPE));
            pPort->config_idx |= (1 << 13);
        }

    }
    break;

    case OMX_ACT_IndexConfigImageDeNoiseLevel:
    {
        //DNS 只支持0-3配置
        OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE  *pType = (OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];

            if(pPort->pCapInfo->bISONoiseFilterSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            memcpy(pPort->pNs_level, pType, sizeof(OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE));
            pPort->config_idx |= (1 << 14);
            pPort->pNs_level->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];

            if(pPort->pCapInfo->bISONoiseFilterSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            memcpy(pPort->pNs_level, pType, sizeof(OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE));
            pPort->config_idx |= (1 << 14);
            pPort->pNs_level->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pType->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pNs_level);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_DNS_LEVEL, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pType->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pNs_level);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_DNS_LEVEL, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

            if(pPort->pCapInfo->bISONoiseFilterSupported == OMX_FALSE)
            { return OMX_ErrorBadParameter; }

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPortOther->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_DNS_LEVEL, (unsigned long)pType, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            memcpy(pPort->pNs_level, pType, sizeof(OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE));
            pPort->config_idx |= (1 << 14);
        }

    }
    break;

    case OMX_IndexConfigSharpness:
    {
        //sharpness 只有0-1,被IC定死,level不可选,效果不明显
        OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE  *pFlicktype = (OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *)ComponentParameterStructure;
        portIndex = pFlicktype->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
            memcpy(pPort->pSharp_level, pFlicktype, sizeof(OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE));
            pPort->config_idx |= (1 << 15);
            pPort->pSharp_level->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
            memcpy(pPort->pSharp_level, pFlicktype, sizeof(OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE));
            pPort->config_idx |= (1 << 15);
            pPort->pSharp_level->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;

            if(pPort->bStreamOn == OMX_TRUE)
            {
                pFlicktype->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pSharp_level);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_SHARP_LEVEL, (unsigned long)pFlicktype, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
            else
            {
                pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
                pFlicktype->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, (OMX_PTR)pPort->pSharp_level);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_SHARP_LEVEL, (unsigned long)pFlicktype, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            omx_base_camera_video_PortType *pPortOther = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - portIndex];
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pFlicktype->nPortIndex];

            if((portIndex == OMX_BASE_FILTER_VIDEO_INDEX && pPortOther->bStreamOn == OMX_FALSE) || \
                    (portIndex == OMX_BASE_FILTER_IMAGE_INDEX && pPortOther->bStreamOn == OMX_TRUE))
            {
#if 0
                err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                    return err;
                }

#else
                err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_SHARP_LEVEL, (unsigned long)pFlicktype, pPort->pSensorMode->bOneShot);

                if(err != OMX_ErrorNone)
                {
                    OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                    return OMX_ErrorHardware;
                }

#endif
            }

            memcpy(pPort->pSharp_level, pFlicktype, sizeof(OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE));
            pPort->config_idx |= (1 << 15);
        }

    }
    break;

    case OMX_ACT_IndexConfigGlobalBlitCompensation:
    {
        //背光补偿,软件方式补偿
        OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE  *pFlicktype = (OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE *)ComponentParameterStructure;
        portIndex = pFlicktype->nPortIndex;

        if(portIndex != OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pFlicktype->nPortIndex];
        memcpy(pPort->pBlitComp, pFlicktype, sizeof(OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE));
#if 0
        err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
            break;
        }

#else
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_BLIT_CMP, (unsigned long)pFlicktype, pPort->pSensorMode->bOneShot);

        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
            return OMX_ErrorHardware;
        }

#endif
    }
    break;
#if 0

    case OMX_IndexConfigCommonFocusRegionStatus:
    {
        OMX_CONFIG_FOCUSREGIONSTATUSTYPE  *pType = (OMX_CONFIG_FOCUSREGIONSTATUSTYPE *)ComponentParameterStructure;
        omx_base_camera_video_PortType *pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[0];///fix
        memcpy(pPort->pAFStatus, pType, sizeof(OMX_CONFIG_FOCUSREGIONSTATUSTYPE));
        omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

    }
    break;

    case OMX_IndexConfigCommonFocusStatus:
    {
        OMX_PARAM_FOCUSSTATUSTYPE  *pType = (OMX_PARAM_FOCUSSTATUSTYPE *)ComponentParameterStructure;
        omx_base_camera_video_PortType *pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[0];///fix
        memcpy(pPort->pAFStatusL, pType, sizeof(OMX_PARAM_FOCUSSTATUSTYPE));
        omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);
    }
    break;

    case OMX_ACT_IndexConfigFocusDistance:
    {
        //提示对焦状态,给出焦距,用于EXIF信息
        OMX_ACT_CONFIG_FOCUSDISTANCETYPE  *pFlicktype = (OMX_ACT_CONFIG_FOCUSDISTANCETYPE *)ComponentParameterStructure;
        omx_base_camera_video_PortType *pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pFlicktype->nPortIndex];
        memcpy(pPort->pAF_Dis, pFlicktype, sizeof(OMX_ACT_CONFIG_FOCUSDISTANCETYPE));
        omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);
    }
    break;
#endif

    case OMX_IndexConfigCommonFocusRegionControl:
    {
        OMX_CONFIG_FOCUSREGIONCONTROLTYPE  *pType = (OMX_CONFIG_FOCUSREGIONCONTROLTYPE *)ComponentParameterStructure;
        omx_base_camera_video_PortType *pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[0];//fixxxx
        act_af_region_info_t af_region;
        int i;
        
        pPort->pCapInfo->ulAreasFocusCount = pType->nFAreas;
        if(pPort->pCapInfo->ulAreasFocusCount)
        {
            memcpy(pPort->pAFRegion, pType, sizeof(OMX_CONFIG_FOCUSREGIONCONTROLTYPE));
#if 0
            err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                break;
            }

#else
            memset(&af_region, 0, sizeof(act_af_region_info_t));
            af_region.nrect_num = pType->nFAreas;
            af_region.nrect_w = pType->sManualFRegions[0].nRectWidth;
            af_region.nrect_h = pType->sManualFRegions[0].nRectHeight;      
            af_region.nstride_w = pPort->sPortParam.format.video.nFrameWidth;
            af_region.nstride_h = pPort->sPortParam.format.video.nFrameHeight;

            for(i = 0; i < af_region.nrect_num; i++)
            {
                af_region.nrect_valid[i] = 1;
                af_region.nrect_offset_x[i] = pType->sManualFRegions[i].nRectY; //注意是反的！
                af_region.nrect_offset_y[i] = pType->sManualFRegions[i].nRectX; 
            }

            if(pPort->bStreamOn)
            { err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AF_REGION, (unsigned long)&af_region, 1); }
            else
            { err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AF_REGION, (unsigned long)&af_region, 0); }

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                return OMX_ErrorHardware;
            }

#endif
        }
    }
    break;

    case OMX_IndexConfigCommonFocusRegion:
    {
        OMX_CONFIG_FOCUSREGIONTYPE  *pType = (OMX_CONFIG_FOCUSREGIONTYPE *)ComponentParameterStructure;
        act_af_region_info_t af_region;
        int i;

        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

        if(pPort->pCapInfo->ulAreasFocusCount)
        {
            memcpy(pPort->pAFRegionL, pType, sizeof(OMX_CONFIG_FOCUSREGIONTYPE));
#if 0
            err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                break;
            }

#else
            memset(&af_region, 0, sizeof(act_af_region_info_t));
            af_region.nrect_valid[0] = pType->bCenter;
            af_region.nrect_valid[1] = pType->bLeft;
            af_region.nrect_valid[2] = pType->bRight;
            af_region.nrect_valid[3] = pType->bTop;
            af_region.nrect_valid[4] = pType->bBottom;
            af_region.nrect_valid[5] = pType->bTopLeft;
            af_region.nrect_valid[6] = pType->bTopRight;
            af_region.nrect_valid[7] = pType->bBottomLeft;
            af_region.nrect_valid[8] = pType->bBottomRight;
            af_region.nrect_w = 32;
            af_region.nstride_w = pPort->sPortParam.format.video.nFrameWidth;
            af_region.nstride_h = pPort->sPortParam.format.video.nFrameHeight;
            af_region.nrect_offset_x[0] = ((pPort->sPortParam.format.video.nFrameWidth - af_region.nrect_w) / 2) & (~32);
            af_region.nrect_offset_y[0] = ((pPort->sPortParam.format.video.nFrameHeight - af_region.nrect_w) / 2) & (~32);
            af_region.nrect_offset_x[1] = af_region.nrect_offset_x[0] - af_region.nrect_w;
            af_region.nrect_offset_y[1] = af_region.nrect_offset_y[0];;
            af_region.nrect_offset_x[2] = af_region.nrect_offset_x[0] + af_region.nrect_w;
            af_region.nrect_offset_y[2] = af_region.nrect_offset_y[0];

            af_region.nrect_offset_x[3] = af_region.nrect_offset_x[0];
            af_region.nrect_offset_y[3] = af_region.nrect_offset_y[0] - af_region.nrect_w;;

            af_region.nrect_offset_x[4] = af_region.nrect_offset_x[0];
            af_region.nrect_offset_y[4] = af_region.nrect_offset_y[0] + af_region.nrect_w;

            af_region.nrect_offset_x[5] = af_region.nrect_offset_x[0] - af_region.nrect_w;
            af_region.nrect_offset_y[5] = af_region.nrect_offset_y[0] - af_region.nrect_w;;

            af_region.nrect_offset_x[6] = af_region.nrect_offset_x[0] + af_region.nrect_w;
            af_region.nrect_offset_y[6] = af_region.nrect_offset_y[0] - af_region.nrect_w;

            af_region.nrect_offset_x[7] = af_region.nrect_offset_x[0] - af_region.nrect_w;
            af_region.nrect_offset_y[7] = af_region.nrect_offset_y[0] + af_region.nrect_w;

            af_region.nrect_offset_x[8] = af_region.nrect_offset_x[0] + af_region.nrect_w;
            af_region.nrect_offset_y[8] = af_region.nrect_offset_y[0] + af_region.nrect_w;

            for(i = 0; i < 9; i++)
            {
                af_region.nrect_num += af_region.nrect_valid[i];
            }


            err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AF_REGION, (unsigned long)&af_region, pPort->pSensorMode->bOneShot);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                return OMX_ErrorHardware;
            }

#endif
        }
    }
    break;

    case OMX_IndexConfigFocusControl:
    {
        OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE  *pType = (OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        OMXDBUG(OMXDBUG_ERR, " Set FocusControl 0x%x,Modecount %d", pType->eFocusControl, pPort->pCapInfo->ulFocusModeCount);

        if(pPort->pCapInfo->ulFocusModeCount)
        {
#if 0
            err = omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "In %s Parameter Check Error=%x\n", __func__, err);
                break;
            }

#else
            err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_SET_AF_TYPE, (unsigned long)pType, pPort->pSensorMode->bOneShot);

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                return OMX_ErrorHardware;
            }

#endif
            memcpy(pPort->pFocusType, pType, sizeof(OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE));
        }
    }
    break;

    /* 拍照 相关函数 */
    case OMX_IndexConfigCaptureMode:
    {
        //使能采集图像的方式,连续采集 或者 限制采集帧数
        OMX_CONFIG_CAPTUREMODETYPE  *pType = (OMX_CONFIG_CAPTUREMODETYPE *)ComponentParameterStructure;
        portIndex = pType->nPortIndex;

        if(portIndex == OMX_ALL)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
            memcpy(pPort->pCapMode, pType, sizeof(OMX_CONFIG_CAPTUREMODETYPE));
            pPort->pCapMode->nPortIndex = OMX_BASE_FILTER_VIDEO_INDEX;

            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
            memcpy(pPort->pCapMode, pType, sizeof(OMX_CONFIG_CAPTUREMODETYPE));
            pPort->pCapMode->nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;
        }
        else if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }
        else
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
            memcpy(pPort->pCapMode, pType, sizeof(OMX_CONFIG_CAPTUREMODETYPE));
        }
    }
    break;

    case OMX_IndexConfigCapturing:
    {
        unsigned int buffidx = 0;
        //开始采集,//使能拍照模式,注意Video模式的死锁
        OMX_CONFIG_BOOLEANTYPE  *pCaptype = (OMX_CONFIG_BOOLEANTYPE *)ComponentParameterStructure;
        omx_base_camera_video_PortType *pPort_Other;
        unsigned int nPortIndex = OMX_BASE_FILTER_IMAGE_INDEX;//we just guess setting on image port.
        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[nPortIndex];
        pPort_Other = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - nPortIndex];
        
        OMXDBUG(OMXDBUG_ERR, "OMX_IndexConfigCapturing\n");

        if(pPort->pSensorMode->bOneShot == OMX_TRUE)
        {
            //Image Capture...
            pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);

            if(pCaptype->bEnabled == OMX_TRUE)
            {
                OMXDBUG(OMXDBUG_ERR, "pCaptype->bEnabled %d\n", pPort_Other->bStreamOn);

                if(pPort_Other->bStreamOn == OMX_TRUE)
                {
                    //Close ViewFinder Now
                    OMXDBUG(OMXDBUG_ERR, "OMX_IndexConfigCapturing StreamOff....\n");
                    pthread_mutex_lock(&omx_camera_component_Private->dq_mutex);
                    pPort_Other->omx_camera->omx_camera_streamoff(pPort_Other->omx_camera, OMX_CAPTURE_PREP);
                    pPort_Other->bStreamOn = OMX_FALSE;
                    pPort_Other->isCapture = OMX_FALSE;
                    pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);

                    //omx_camera_component_Private->bTransFrom_C2V = OMX_FALSE;
                    if(omx_camera_component_Private->bMgmtSem->semval == 0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }

                if(pPort->bStreamOn == OMX_FALSE)// && omx_camera_component_Private->bTransFrom_C2V == OMX_FALSE)
                {
                    //pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);
                    err |= cam_set_def(pPort, nPortIndex);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                        return OMX_ErrorHardware;
                    }

                    //err |= omx_camera_component_Private->filter_setconfig(hComponent,OMX_IndexParamPortDefinition,(OMX_PTR)&pPort->sPortParam);
                    if(pPort->config_idx)
                    {
                        err |= SetCamParams((OMX_COMPONENTTYPE *)hComponent, nPortIndex);

                        if(err != OMX_ErrorNone)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                            pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                            return OMX_ErrorHardware;
                        }
                    }

                    if(pPort->bHdr_Enable == OMX_FALSE)
                    {
                        err |= pPort->omx_camera->omx_camera_requestbuf(pPort->omx_camera, \
                                (unsigned int *)&pPort->sPortParam.nBufferCountActual, \
                                pPort->pSensorMode->bOneShot);

                        if(err != OMX_ErrorNone)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                            pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                            return OMX_ErrorHardware;
                        }

                        for(buffidx = 0; buffidx < pPort->nNumAssignedBuffers; buffidx++)
                        {
                            OMXDBUG(OMXDBUG_VERB, "Preview Stream SetBuff now ....%i\n", buffidx);
                            err = pPort->omx_camera->omx_camera_querybuf((void *)pPort->omx_camera, buffidx, \
                                    pPort->sPortParam.nBufferSize, \
                                    omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx], \
                                    omx_camera_component_Private->pBufQ[nPortIndex].pBufferStat[buffidx], \
                                    pPort->pSensorMode->bOneShot);
                        }
                    }
                    else
                    {
#if 1
                        ReqBuf_Hdr(pPort);
                        FillBuffer_Hdr(pPort,0);
                        FillBuffer_Hdr(pPort,1);
#endif
                    }

                    err |= pPort->omx_camera->omx_camera_streamon(pPort->omx_camera, pPort->pSensorMode->bOneShot);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                        return OMX_ErrorHardware;
                    }

                    pPort->bStreamOn = OMX_TRUE;
                    pPort->isCapture = pCaptype->bEnabled;
                    //pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                    //omx_camera_component_Private->CurStatus = pPort->isCapture;
                    //if(omx_camera_component_Private->bMgmtSem->semval==0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }

            }
            else if(pCaptype->bEnabled == OMX_FALSE)
            {
                //pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);
                if(pPort->bStreamOn == OMX_TRUE)
                {
                    OMXDBUG(OMXDBUG_ERR, "stream off........caping\n");
                    //pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);
                    pthread_mutex_lock(&omx_camera_component_Private->dq_mutex);

                    if(pPort_Other->pSensorMode->bOneShot == OMX_TRUE)
                        err = pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, \
                                pPort->pSensorMode->bOneShot);
                    else
                    {
                        if(omx_camera_component_Private->bTransFrom_C2V == OMX_TRUE)
                        { err = pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, OMX_PREVIEW_CAP); }
                        else
                        { err = pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, pPort->pSensorMode->bOneShot); }
                    }

                    pPort->bStreamOn = OMX_FALSE;
                    pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                        return OMX_ErrorHardware;
                    }

                    pPort->isCapture = pCaptype->bEnabled;

                    //pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                    //  omx_camera_component_Private->CurStatus = pPort->isCapture;
                    if(omx_camera_component_Private->bMgmtSem->semval == 0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }

                //pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
            }

            pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
            OMXDBUG(OMXDBUG_ERR, "OMX_IndexConfigCapturing out\n");
        }
        else
        {
            OMXDBUG(OMXDBUG_ERR, "ERR IN Video Port View....\n");

            if(pCaptype->bEnabled == OMX_TRUE)
            {
                if(pPort_Other->bStreamOn == OMX_TRUE)
                {
                    //Close ViewFinder Now
                    OMXDBUG(OMXDBUG_VERB, "StreamOff....\n");
                    pthread_mutex_lock(&omx_camera_component_Private->dq_mutex);
                    err = pPort_Other->omx_camera->omx_camera_streamoff(pPort_Other->omx_camera, pPort_Other->pSensorMode->bOneShot);
                    pPort_Other->bStreamOn = OMX_FALSE;
                    pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        return OMX_ErrorHardware;
                    }

                    pPort_Other->isCapture = OMX_FALSE;

                    if(omx_camera_component_Private->bMgmtSem->semval == 0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }

                if(pPort->bStreamOn == OMX_FALSE)
                {
                    //pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);
                    err |= cam_set_def(pPort, nPortIndex);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        //pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                        return OMX_ErrorHardware;
                    }

                    //err |= omx_camera_component_Private->filter_setconfig(hComponent,OMX_IndexParamPortDefinition,(OMX_PTR)&pPort->sPortParam);
                    if(pPort->config_idx)
                    {
                        err = SetCamParams((OMX_COMPONENTTYPE *)hComponent, nPortIndex);

                        if(err != OMX_ErrorNone)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                            //pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                            return OMX_ErrorHardware;
                        }
                    }

                    err |= pPort->omx_camera->omx_camera_requestbuf(pPort->omx_camera, \
                            (unsigned int *)&pPort->sPortParam.nBufferCountActual, \
                            pPort->pSensorMode->bOneShot);

                    for(buffidx = 0; buffidx < pPort->nNumAssignedBuffers; buffidx++)
                    {
                        OMXDBUG(OMXDBUG_VERB, "Preview Stream SetBuff now ....%i\n", buffidx);
                        err = pPort->omx_camera->omx_camera_querybuf((void *)pPort->omx_camera, buffidx, \
                                pPort->sPortParam.nBufferSize, \
                                omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx], \
                                omx_camera_component_Private->pBufQ[nPortIndex].pBufferStat[buffidx], \
                                pPort->pSensorMode->bOneShot);
                    }

                    err |= pPort->omx_camera->omx_camera_streamon(pPort->omx_camera, pPort->pSensorMode->bOneShot);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        //pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                        return OMX_ErrorHardware;
                    }

                    pPort->bStreamOn = OMX_TRUE;
                    pPort->isCapture = pCaptype->bEnabled;
                    //pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                    //omx_camera_component_Private->CurStatus = pPort->isCapture;
                    //if(omx_camera_component_Private->bMgmtSem->semval==0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }

            }
            else if(pCaptype->bEnabled == OMX_FALSE)
            {
                if(pPort->bStreamOn == OMX_TRUE)
                {
                    //Close ViewFinder or Capture Now
                    OMXDBUG(OMXDBUG_ERR, "stream off %d\n", __LINE__);
                    pthread_mutex_lock(&omx_camera_component_Private->dq_mutex);
                    err = pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, pPort->pSensorMode->bOneShot);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);
                        return OMX_ErrorHardware;
                    }

                    pPort->bStreamOn = OMX_FALSE;
                    pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);
                    pPort->isCapture = pCaptype->bEnabled;
                    //omx_camera_component_Private->CurStatus = pPort->isCapture;
                    omx_camera_component_Private->bTransFrom_C2V = OMX_FALSE;

                    if(omx_camera_component_Private->bMgmtSem->semval == 0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }
            }
        }

        //pPort->isCapture = pCaptype->bEnabled;
        // omx_camera_component_Private->CurStatus = pPort->isCapture;
        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
            return OMX_ErrorHardware;
        }
    }
    break;

    case OMX_IndexConfigCommonPortCapturing:
    {
        unsigned int buffidx = 0;
        //开始采集,//使能拍照模式,注意Video模式的死锁
        OMX_CONFIG_PORTBOOLEANTYPE  *pCaptype = (OMX_CONFIG_PORTBOOLEANTYPE *)ComponentParameterStructure;
        omx_base_camera_video_PortType *pPort_Other;
        unsigned int nPortIndex = pCaptype->nPortIndex;

        OMXDBUG(OMXDBUG_VERB, "OMX_IndexConfigCommonPortCapturing\n");

        if(nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pCaptype->nPortIndex];
        pPort_Other = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1 - pCaptype->nPortIndex];

        if(pPort->pSensorMode->bOneShot == OMX_TRUE)
        {
            //Image Capture...

            if(pCaptype->bEnabled == OMX_TRUE)
            {
                OMXDBUG(OMXDBUG_ERR, "bStreamOn IndexConfig lock....%d\n", (int)pPort_Other->bStreamOn);

                pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);

                if(pPort_Other->bStreamOn == OMX_TRUE)
                {
                    //Close ViewFinder Now
                    OMXDBUG(OMXDBUG_ERR, "StreamOff IndexConfig....%d\n", (int)pCaptype->nPortIndex);
                    pthread_mutex_lock(&omx_camera_component_Private->dq_mutex);
                    pPort_Other->omx_camera->omx_camera_streamoff(pPort_Other->omx_camera, OMX_CAPTURE_PREP);
                    pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);
                    pPort_Other->bStreamOn = OMX_FALSE;
                    pPort_Other->isCapture = OMX_FALSE;

                    //omx_camera_component_Private->bTransFrom_C2V = OMX_FALSE;
                    if(omx_camera_component_Private->bMgmtSem->semval == 0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }


                //pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);
                if(pPort->bStreamOn == OMX_FALSE)// && omx_camera_component_Private->bTransFrom_C2V == OMX_FALSE)
                {
                    err |= cam_set_def(pPort, pCaptype->nPortIndex);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                        return OMX_ErrorHardware;
                    }

                    //err |= omx_camera_component_Private->filter_setconfig(hComponent,OMX_IndexParamPortDefinition,(OMX_PTR)&pPort->sPortParam);
                    if(pPort->config_idx)
                    {
                        err |= SetCamParams((OMX_COMPONENTTYPE *)hComponent, pCaptype->nPortIndex);

                        if(err != OMX_ErrorNone)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                            pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                            return OMX_ErrorHardware;
                        }
                    }

                    if(pPort->bHdr_Enable == OMX_FALSE)
                    {
                        err |= pPort->omx_camera->omx_camera_requestbuf(pPort->omx_camera, \
                                (unsigned int *)&pPort->sPortParam.nBufferCountActual, \
                                pPort->pSensorMode->bOneShot);

                        if(err != OMX_ErrorNone)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                            pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                            return OMX_ErrorHardware;
                        }

                        for(buffidx = 0; buffidx < pPort->nNumAssignedBuffers; buffidx++)
                        {
                            OMXDBUG(OMXDBUG_VERB, "Preview Stream SetBuff now ....%i\n", buffidx);
                            err = pPort->omx_camera->omx_camera_querybuf((void *)pPort->omx_camera, buffidx, \
                                    pPort->sPortParam.nBufferSize, \
                                    omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx], \
                                    omx_camera_component_Private->pBufQ[nPortIndex].pBufferStat[buffidx], \
                                    pPort->pSensorMode->bOneShot);
                        }
                    }
                    else
                    {
#if 1
                        ReqBuf_Hdr(pPort);
                        FillBuffer_Hdr(pPort,0);
                        FillBuffer_Hdr(pPort,1);
#endif
                    }

                    err |= pPort->omx_camera->omx_camera_streamon(pPort->omx_camera, pPort->pSensorMode->bOneShot);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                        return OMX_ErrorHardware;
                    }

                    pPort->bStreamOn = OMX_TRUE;
                    pPort->isCapture = pCaptype->bEnabled;
                    pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                    //omx_camera_component_Private->CurStatus = pPort->isCapture;
                    //printf("stream on now %d\n",omx_camera_component_Private->bMgmtSem->semval);
                    //if(omx_camera_component_Private->bMgmtSem->semval==0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }
                else
                { pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex); }

                OMXDBUG(OMXDBUG_ERR, "OMX_IndexConfigCapturing on out\n");

            }
            else if(pCaptype->bEnabled == OMX_FALSE)
            {
                OMXDBUG(OMXDBUG_ERR, "OMX_IndexConfigCapturing off in\n");
                pthread_mutex_lock(&omx_camera_component_Private->cmd_mutex);

                if(pPort->bStreamOn == OMX_TRUE)
                {
                    OMXDBUG(OMXDBUG_ERR, "stream off........caping\n");
                    pthread_mutex_lock(&omx_camera_component_Private->dq_mutex);

                    if(pPort_Other->pSensorMode->bOneShot == OMX_TRUE)
                        err = pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, \
                                pPort->pSensorMode->bOneShot);
                    else
                    {
                        if(omx_camera_component_Private->bTransFrom_C2V == OMX_TRUE)
                        { err = pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, OMX_PREVIEW_CAP); }
                        else
                        { err = pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, pPort->pSensorMode->bOneShot); }
                    }

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);
                        pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                        return OMX_ErrorHardware;
                    }

                    pPort->bStreamOn = OMX_FALSE;
                    pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);
                    pPort->isCapture = pCaptype->bEnabled;
                    pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);

                    //pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex);
                    //omx_camera_component_Private->CurStatus = pPort->isCapture;
                    if(omx_camera_component_Private->bMgmtSem->semval == 0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }
                else
                { pthread_mutex_unlock(&omx_camera_component_Private->cmd_mutex); }

                OMXDBUG(OMXDBUG_ERR, "OMX_IndexConfigCapturing off out\n");
#if 0

                if(pPort->bStreamOn == OMX_TRUE)
                {
                    //Close ViewFinder or Capture Now
                    pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, pPort->pSensorMode->bOneShot);
                    pPort->bStreamOn == OMX_FALSE;
                }

                if(pPort->pCapMode->bFrameLimited == OMX_FALSE)
                {
                    pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, pPort->pSensorMode->bOneShot);
                    pPort->bStreamOn = OMX_FALSE;

                    if(pPort_Other->config_idx)
                    {
                        SetCamParams(hComponent, 1 - pCaptype->nPortIndex);
                    }

                    pPort_Other->omx_camera->omx_camera_streamon(pPort_Other->omx_camera, pPort_Other->pSensorMode->bOneShot);
                    pPort_Other->bStreamOn = OMX_TRUE;
                }

#endif
            }

        }
        else
        {
            OMXDBUG(OMXDBUG_ERR, "ERR IN Video Port View....\n");

            if(pCaptype->bEnabled == OMX_TRUE)
            {
                if(pPort_Other->bStreamOn == OMX_TRUE)
                {
                    //Close ViewFinder Now
                    OMXDBUG(OMXDBUG_VERB, "StreamOff....\n");
                    pthread_mutex_lock(&omx_camera_component_Private->dq_mutex);
                    err = pPort_Other->omx_camera->omx_camera_streamoff(pPort_Other->omx_camera, pPort_Other->pSensorMode->bOneShot);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);
                        return OMX_ErrorHardware;
                    }

                    pPort_Other->bStreamOn = OMX_FALSE;
                    pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);
                    pPort_Other->isCapture = OMX_FALSE;

                    if(omx_camera_component_Private->bMgmtSem->semval == 0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }

                if(pPort->bStreamOn == OMX_FALSE)
                {
                    err |= cam_set_def(pPort, pCaptype->nPortIndex);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        return OMX_ErrorHardware;
                    }

                    //err |= omx_camera_component_Private->filter_setconfig(hComponent,OMX_IndexParamPortDefinition,(OMX_PTR)&pPort->sPortParam);
                    if(pPort->config_idx)
                    {
                        err = SetCamParams((OMX_COMPONENTTYPE *)hComponent, pCaptype->nPortIndex);

                        if(err != OMX_ErrorNone)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                            return OMX_ErrorHardware;
                        }
                    }

                    err |= pPort->omx_camera->omx_camera_requestbuf(pPort->omx_camera, \
                            (unsigned int *)&pPort->sPortParam.nBufferCountActual, \
                            pPort->pSensorMode->bOneShot);

                    for(buffidx = 0; buffidx < pPort->nNumAssignedBuffers; buffidx++)
                    {
                        OMXDBUG(OMXDBUG_VERB, "Preview Stream SetBuff now ....%i\n", buffidx);
                        err = pPort->omx_camera->omx_camera_querybuf((void *)pPort->omx_camera, buffidx, \
                                pPort->sPortParam.nBufferSize, \
                                omx_camera_component_Private->pBufQ[nPortIndex].pBufferPhy[buffidx], \
                                omx_camera_component_Private->pBufQ[nPortIndex].pBufferStat[buffidx], \
                                pPort->pSensorMode->bOneShot);
                    }

                    err |= pPort->omx_camera->omx_camera_streamon(pPort->omx_camera, pPort->pSensorMode->bOneShot);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        return OMX_ErrorHardware;
                    }

                    pPort->bStreamOn = OMX_TRUE;
                    pPort->isCapture = pCaptype->bEnabled;
                    //omx_camera_component_Private->CurStatus = pPort->isCapture;
                    //if(omx_camera_component_Private->bMgmtSem->semval==0)
                    {

                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }

            }
            else if(pCaptype->bEnabled == OMX_FALSE)
            {
                if(pPort->bStreamOn == OMX_TRUE)
                {
                    //Close ViewFinder or Capture Now
                    OMXDBUG(OMXDBUG_ERR, "stream off %d\n", __LINE__);
                    pthread_mutex_lock(&omx_camera_component_Private->dq_mutex);
                    err = pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, pPort->pSensorMode->bOneShot);

                    if(err != OMX_ErrorNone)
                    {
                        OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                        pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);
                        return OMX_ErrorHardware;
                    }

                    pPort->bStreamOn = OMX_FALSE;
                    pthread_mutex_unlock(&omx_camera_component_Private->dq_mutex);
                    pPort->isCapture = pCaptype->bEnabled;
                    //omx_camera_component_Private->CurStatus = pPort->isCapture;
                    omx_camera_component_Private->bTransFrom_C2V = OMX_FALSE;

                    if(omx_camera_component_Private->bMgmtSem->semval == 0)
                    {
                        tsem_up(omx_camera_component_Private->bMgmtSem);
                    }
                }

#if 0

                if(pPort->pCapMode->bFrameLimited == OMX_FALSE)
                {
                    pPort->omx_camera->omx_camera_streamoff(pPort->omx_camera, pPort->pSensorMode->bOneShot);
                    pPort->bStreamOn = OMX_FALSE;

                    if(pPort_Other->config_idx)
                    {
                        SetCamParams(hComponent, 1 - pCaptype->nPortIndex);
                    }

                    pPort_Other->omx_camera->omx_camera_streamon(pPort_Other->omx_camera, pPort_Other->pSensorMode->bOneShot);
                    pPort_Other->bStreamOn = OMX_TRUE;
                }

#endif
            }
        }

        //pPort->isCapture = pCaptype->bEnabled;
        //omx_camera_component_Private->CurStatus = pPort->isCapture;
        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
            return OMX_ErrorHardware;
        }
    }
    break;

    case OMX_IndexConfigCommonExtCaptureMode:
    {
        OMX_CONFIG_EXTCAPTUREMODETYPE *pExtMode = (OMX_CONFIG_EXTCAPTUREMODETYPE *)ComponentParameterStructure;
        unsigned int nPortIndex = pExtMode->nPortIndex;

        if(nPortIndex != OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pExtMode->nPortIndex];
        memcpy(pPort->pCapExtMode, pExtMode, sizeof(OMX_CONFIG_EXTCAPTUREMODETYPE));
        OMXDBUG(OMXDBUG_ERR, "OMX_IndexConfigCommonExtCaptureMode\n");
    }
    break;

    case OMX_IndexAutoPauseAfterCapture:
    {
        OMX_CONFIG_BOOLEANTYPE  *pCaptype = (OMX_CONFIG_BOOLEANTYPE *)ComponentParameterStructure;
        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[0];
        pPort->bCapturePause = pCaptype->bEnabled;
        ((omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1])->bCapturePause = pCaptype->bEnabled;
    }
    break;

    case OMX_ACT_IndexConfigFlip:
    {
        OMXDBUG(OMXDBUG_ERR, "Parameter ID not supported\n");
    }
    break;

    default: /*Call the base component function*/
        return omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
    }

    OMXDBUG(OMXDBUG_VERB, "Setting parameter out %x\n", nParamIndex);
    return err;
}

OMX_ERRORTYPE omx_camera_component_GetParameter(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR ComponentParameterStructure)
{

    OMX_VIDEO_PARAM_PORTFORMATTYPE             *pVideoPortFormat;
    //  OMX_OTHER_PARAM_PORTFORMATTYPE             *pOtherPortFormat;
    OMX_ERRORTYPE                              err = OMX_ErrorNone;
    OMX_COMPONENTTYPE                          *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_camera_PrivateType                   *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    omx_base_camera_video_PortType                    *pPort;
    OMX_PARAM_COMPONENTROLETYPE                *pComponentRole;

    if(ComponentParameterStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }

    OMXDBUG(OMXDBUG_VERB, "  cam Getting parameter %x\n", nParamIndex);

    /* Check which structure we are being fed and fill its header */
    switch((unsigned int)nParamIndex)
    {
        /*case OMX_IndexParamVideoInit:
          if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone) {
            break;
          }
          memcpy(ComponentParameterStructure, &omx_camera_component_Private->sPortTypesParam[OMX_PortDomainVideo], sizeof(OMX_PORT_PARAM_TYPE));
          break;*/
    case OMX_IndexParamVideoInit:
    {
        OMX_PORT_PARAM_TYPE *pPortDomains = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;

        if((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone)
        {
            break;
        }

        pPortDomains->nPorts = 1;
        pPortDomains->nStartPortNumber = 0;
    }
    break;

    case OMX_IndexParamImageInit:
    {
        OMX_PORT_PARAM_TYPE *pPortDomains = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;

        if((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone)
        {
            break;
        }

        pPortDomains->nPorts = 1;
        pPortDomains->nStartPortNumber = 1;
    }
    break;

    case OMX_IndexParamVideoPortFormat:
    {
        int formatIdx = 0;
        pVideoPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pVideoPortFormat->nPortIndex];

        if((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone)
        {
            break;
        }

        formatIdx = pVideoPortFormat->nIndex;
        OMXDBUG(OMXDBUG_ERR, " Getting VideoPortFormat %i,%d\n", pVideoPortFormat->nPortIndex, formatIdx);

        if(pVideoPortFormat->nPortIndex == OMX_BASE_FILTER_VIDEO_INDEX)
        {
            if(formatIdx > 1) { return OMX_ErrorBadParameter; }

            pVideoPortFormat->eColorFormat = omx_coloridx[formatIdx];
            pVideoPortFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
        }
        else
        {
            return OMX_ErrorBadParameter;
        }
    }
    break;

    case OMX_IndexParamImagePortFormat:
    {
        int formatIdx = 0;
        OMX_IMAGE_PARAM_PORTFORMATTYPE *pImagePortFormat = (OMX_IMAGE_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pImagePortFormat->nPortIndex];

        if((err = checkHeader(ComponentParameterStructure, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone)
        {
            break;
        }

        formatIdx = pImagePortFormat->nIndex;
        OMXDBUG(OMXDBUG_ERR, " Getting VideoPortFormat %i,%d\n", pImagePortFormat->nPortIndex, formatIdx);

        if(pImagePortFormat->nPortIndex == OMX_BASE_FILTER_IMAGE_INDEX)
        {
            if(formatIdx > 3) { return OMX_ErrorBadParameter; }

            pImagePortFormat->eColorFormat = omx_coloridx[formatIdx];
            pImagePortFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
        }
        else
        {
            return OMX_ErrorBadParameter;
        }
    }
    break;

    case OMX_IndexParamStandardComponentRole:
        pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)ComponentParameterStructure;

        if((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone)
        {
            break;
        }

        strcpy((char *) pComponentRole->cRole, ISP_COMP_ROLE);
        break;

    case OMX_ACT_IndexConfigCamCapabilities:
    {
        OMX_ACT_CAPTYPE *pCap = (OMX_ACT_CAPTYPE *)ComponentParameterStructure;
        camera_module_info_t pModeInfo;
        int err = 0;
        OMXDBUG(OMXDBUG_ERR, "OMX_ACT_IndexConfigCamCapabilities %d, %d, %d\n", pCap->nSensorIndex, gomxcam_bIsInited, gomxcam_bDoneQuery[pCap->nSensorIndex]);
        /**
         * 此处按需求是仅仅支持单sensor输入，port端的定义，限制了仅支持Viewfinder 和 image capture
         */

        if(pCap->nSensorIndex > 2 || (int)pCap->nSensorIndex < 0)
        {

            return OMX_ErrorBadParameter;
        }

        if(pCap->nSensorIndex == 0)
        {
            if(omx_camera_component_Private->pCapInfo[0]->ulPreviewResCount == 0)
            {
                if(gomxcam_bIsInited == 1 && gomxcam_bDoneQuery[pCap->nSensorIndex] == 1)
                {
                    memcpy(omx_camera_component_Private->pCapInfo[0], &gomxcam_OMX_CAPS[pCap->nSensorIndex], sizeof(OMX_ACT_CAPTYPE));
                    omx_camera_component_Private->nModuleType[0] = pCap->nSensorIndex;
                }
                else
                {
                    pModeInfo.supportParam[OMX_PrimarySensor] = omx_camera_component_Private->pCapInfo[0];
                    err = omx_camera_component_Private->camera_module_query(&pModeInfo, OMX_PrimarySensor,pCap->uvcmode);///------

                    if(err == -1)
                    {
                        OMXDBUG(OMXDBUG_ERR, "open failed by OMX_PrimarySensor !!\n");
                        omx_camera_component_Private->pCapInfo[0]->ulPreviewResCount = 0;
                    }
                    else
                    {
                        omx_camera_component_Private->Dgain_th[0] = pModeInfo.mDgain_th[0];
                        omx_camera_component_Private->nModuleType[0] = pModeInfo.supportType[0];

                        if(gomxcam_bDoneQuery[pCap->nSensorIndex] == 0)
                        {
                            memcpy(&gomxcam_OMX_CAPS[pCap->nSensorIndex], omx_camera_component_Private->pCapInfo[0], sizeof(OMX_ACT_CAPTYPE));
                            gomxcam_bDoneQuery[pCap->nSensorIndex] = 1;
                        }
                    }

                    OMXDBUG(OMXDBUG_ERR, "Select SensorType %d\n", (int)pCap->nSensorIndex);
                }
            }
        }

        if(pCap->nSensorIndex == 1)
        {
            if(omx_camera_component_Private->pCapInfo[1]->ulPreviewResCount == 0)
            {
                if(gomxcam_bIsInited == 1 && gomxcam_bDoneQuery[pCap->nSensorIndex] == 1)
                {
                    memcpy(omx_camera_component_Private->pCapInfo[1], &gomxcam_OMX_CAPS[pCap->nSensorIndex], sizeof(OMX_ACT_CAPTYPE));
                    omx_camera_component_Private->nModuleType[1] = pCap->nSensorIndex;
                }
                else
                {
                    pModeInfo.supportParam[OMX_SecondarySensor] = omx_camera_component_Private->pCapInfo[1];
					err = omx_camera_component_Private->camera_module_query(&pModeInfo, OMX_SecondarySensor,pCap->uvcmode);

                    if(err == -1)
                    {
                        OMXDBUG(OMXDBUG_ERR, "open failed by OMX_PrimarySensor !!\n");
                        omx_camera_component_Private->pCapInfo[1]->ulPreviewResCount = 0;
                    }
                    else
                    {
                        if(gomxcam_bDoneQuery[pCap->nSensorIndex] == 0)
                        {
                            memcpy(&gomxcam_OMX_CAPS[pCap->nSensorIndex], omx_camera_component_Private->pCapInfo[1], sizeof(OMX_ACT_CAPTYPE));
                            gomxcam_bDoneQuery[pCap->nSensorIndex] = 1;
                        }

                        omx_camera_component_Private->Dgain_th[1] = pModeInfo.mDgain_th[1];
                        omx_camera_component_Private->nModuleType[1] = pModeInfo.supportType[1];
                    }
                }

                OMXDBUG(OMXDBUG_ERR, "Select SensorType %d\n", (int)pCap->nSensorIndex);
            }
        }

        memcpy(pCap, omx_camera_component_Private->pCapInfo[pCap->nSensorIndex], sizeof(OMX_ACT_CAPTYPE));
        OMXDBUG(OMXDBUG_VERB, "Get SensorNum %d\n", (int)pCap->nSensorIndex);
        OMXDBUG(OMXDBUG_VERB, "Res Count %d\n", (unsigned int)omx_camera_component_Private->pCapInfo[pCap->nSensorIndex]->ulPreviewResCount);
    }
    break;

    case OMX_ACT_IndexConfigVideoParam:
    {
        OMX_ACT_CONFIG_VIDEOPARAM *pType = (OMX_ACT_CONFIG_VIDEOPARAM *)ComponentParameterStructure;
        pPort  = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        pType->nFramerate = pPort->sPortParam.format.video.xFramerate;
        pType->nWidth = pPort->sPortParam.format.video.nFrameWidth;
        pType->nHeight = pPort->sPortParam.format.video.nFrameHeight;
    }
    break;

#if HDR_PROCESS
    case OMX_ACT_IndexConfigHDRParam:
    {
        OMX_ACT_CONFIG_HDR_EVParams  *pType = (OMX_ACT_CONFIG_HDR_EVParams *)ComponentParameterStructure;
        pPort  = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pHdrParam, pType, sizeof(OMX_ACT_CONFIG_HDR_EVParams));
    }
    break;
#endif

    case OMX_IndexConfigCommonFocusRegionStatus:
    {
        OMX_CONFIG_FOCUSREGIONSTATUSTYPE  *pType = (OMX_CONFIG_FOCUSREGIONSTATUSTYPE *)ComponentParameterStructure;
        act_af_status_info_t af_status;
        int nfocusNum = 0;
        int i;

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[1];///fix

        if(pPort->bStreamOn == OMX_FALSE)
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[0];///fix
        }

        if(pPort->pCapInfo->ulFocusModeCount)
        {
            if(pPort->isCapture == OMX_FALSE)
            { err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_GET_FOCUS_STATUS, (unsigned long)&af_status, OMX_PREVIEW_MODE); }
            else
            { err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_GET_FOCUS_STATUS, (unsigned long)&af_status, OMX_CAPTURE_MODE); }

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                return OMX_ErrorHardware;
            }

            pPort->pAF_Dis->nLensPosition = af_status.blens_dis;
            pPort->pAFStatusL->bCenterStatus = af_status.brect_focused[0];
            pPort->pAFStatusL->bLeftStatus = af_status.brect_focused[1];
            pPort->pAFStatusL->bRightStatus = af_status.brect_focused[2];
            pPort->pAFStatusL->bTopStatus = af_status.brect_focused[3];
            pPort->pAFStatusL->bBottomStatus = af_status.brect_focused[4];
            pPort->pAFStatusL->bTopLeftStatus = af_status.brect_focused[5];
            pPort->pAFStatusL->bTopRightStatus = af_status.brect_focused[6];
            pPort->pAFStatusL->bBottomLeftStatus = af_status.brect_focused[7];
            pPort->pAFStatusL->bBottomRightStatus = af_status.brect_focused[8];

            for(i = 0; i < 9; i++)
            { nfocusNum += af_status.brect_focused[i]; }

            pPort->pAFStatusL->eFocusStatus = af_status.bfocused;
            pPort->pAFStatusL->nFocusRatio = af_status.nfocused_ratio;
            
            pPort->pAFStatus->nFocusRatio = af_status.nfocused_ratio;
            pPort->pAFStatus->bFocused = af_status.bfocused;
            pPort->pAFStatus->nMaxFAreas = pPort->pAFRegion->nFAreas;
            pPort->pAFStatus->nFAreas = nfocusNum;

            for(i = 0; i < (int)pPort->pAFStatus->nMaxFAreas; i++)
            {
                pPort->pAFStatus->sFROIs[i].eFocusStatus = af_status.brect_focused[i];
                pPort->pAFStatus->sFROIs[i].xFocusDistance = af_status.blens_dis;
                pPort->pAFStatus->sFROIs[i].nRectX = pPort->pAFRegion->sManualFRegions[i].nRectX;
                pPort->pAFStatus->sFROIs[i].nRectY = pPort->pAFRegion->sManualFRegions[i].nRectY;
                pPort->pAFStatus->sFROIs[i].nRectWidth = pPort->pAFRegion->sManualFRegions[i].nRectWidth;
                pPort->pAFStatus->sFROIs[i].nRectHeight = pPort->pAFRegion->sManualFRegions[i].nRectHeight;
            }

            memcpy(pType, pPort->pAFStatus, sizeof(OMX_CONFIG_FOCUSREGIONSTATUSTYPE));
        }
    }
    break;

    case OMX_IndexConfigCommonFocusStatus:
    {
        OMX_PARAM_FOCUSSTATUSTYPE  *pType = (OMX_PARAM_FOCUSSTATUSTYPE *)ComponentParameterStructure;
        act_af_status_info_t af_status;
        int nfocusNum = 0;
        int i;

        if(pType->nPortIndex == OMX_ALL)
        {
            OMXDBUG(OMXDBUG_ERR, "portindex is ALL\n");
            return OMX_ErrorBadParameter;
        }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];///fix
#if 0
        omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);
#else

        if(pPort->pCapInfo->ulFocusModeCount)
        {
            if(pPort->isCapture == OMX_FALSE)
            { err |= pPort->omx_camera->omx_camera_getctl(pPort->omx_camera, CAM_GET_FOCUS_STATUS, &af_status, OMX_PREVIEW_MODE); }
            else
            { err |= pPort->omx_camera->omx_camera_getctl(pPort->omx_camera, CAM_GET_FOCUS_STATUS, &af_status, OMX_CAPTURE_MODE); }

            if(err != OMX_ErrorNone)
            {
                OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
                return OMX_ErrorHardware;
            }

            pPort->pAF_Dis->nLensPosition = af_status.blens_dis;
            pPort->pAFStatusL->bCenterStatus = af_status.brect_focused[0];
            pPort->pAFStatusL->bLeftStatus = af_status.brect_focused[1];
            pPort->pAFStatusL->bRightStatus = af_status.brect_focused[2];
            pPort->pAFStatusL->bTopStatus = af_status.brect_focused[3];
            pPort->pAFStatusL->bBottomStatus = af_status.brect_focused[4];
            pPort->pAFStatusL->bTopLeftStatus = af_status.brect_focused[5];
            pPort->pAFStatusL->bTopRightStatus = af_status.brect_focused[6];
            pPort->pAFStatusL->bBottomLeftStatus = af_status.brect_focused[7];
            pPort->pAFStatusL->bBottomRightStatus = af_status.brect_focused[8];

            for(i = 0; i < 9; i++)
            { nfocusNum += af_status.brect_focused[i]; }

            pPort->pAFStatusL->eFocusStatus = af_status.bfocused;
            pPort->pAFStatusL->nFocusRatio = af_status.nfocused_ratio;
            
            pPort->pAFStatus->nFocusRatio = af_status.nfocused_ratio;
            pPort->pAFStatus->bFocused = af_status.bfocused;
            pPort->pAFStatus->nMaxFAreas = pPort->pAFRegion->nFAreas;
            pPort->pAFStatus->nFAreas = nfocusNum;

            for(i = 0; i < (int)pPort->pAFStatus->nMaxFAreas; i++)
            {
                pPort->pAFStatus->sFROIs[i].eFocusStatus = af_status.brect_focused[i];
                pPort->pAFStatus->sFROIs[i].xFocusDistance = af_status.blens_dis;
                pPort->pAFStatus->sFROIs[i].nRectX = pPort->pAFRegion->sManualFRegions[i].nRectX;
                pPort->pAFStatus->sFROIs[i].nRectY = pPort->pAFRegion->sManualFRegions[i].nRectY;
                pPort->pAFStatus->sFROIs[i].nRectWidth = pPort->pAFRegion->sManualFRegions[i].nRectWidth;
                pPort->pAFStatus->sFROIs[i].nRectHeight = pPort->pAFRegion->sManualFRegions[i].nRectHeight;
            }
        }
        else
        {
            OMXDBUG(OMXDBUG_ERR, "ulFocusModeCount is zero\n");
        }

#endif
        memcpy(pType, pPort->pAFStatusL, sizeof(OMX_PARAM_FOCUSSTATUSTYPE));
    }
    break;

    case OMX_ACT_IndexConfigFocusDistance:
    {
        //提示对焦状态,给出焦距,用于EXIF信息
        OMX_ACT_CONFIG_FOCUSDISTANCETYPE  *pFlicktype = (OMX_ACT_CONFIG_FOCUSDISTANCETYPE *)ComponentParameterStructure;

        if(pFlicktype->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pFlicktype->nPortIndex];
        //omx_camera_component_Private->filter_setconfig(hComponent,nParamIndex,ComponentParameterStructure);
        err |= pPort->omx_camera->omx_camera_setctl(pPort->omx_camera, CAM_GET_FOCUS_DIS, (unsigned long)pFlicktype, pPort->pSensorMode->bOneShot);

        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "Err Parameter  Error=%d,%x\n", __LINE__, err);
            return OMX_ErrorHardware;
        }

        memcpy(pFlicktype, pPort->pAF_Dis, sizeof(OMX_ACT_CONFIG_FOCUSDISTANCETYPE));

    }
    break;

    case OMX_IndexConfigCommonPortCapturing:
    {
        OMX_CONFIG_PORTBOOLEANTYPE *pCaptype = (OMX_CONFIG_PORTBOOLEANTYPE *)ComponentParameterStructure;
        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pCaptype->nPortIndex];
        pCaptype->bEnabled = pPort->isCapture;
    }
    break;

    case OMX_IndexConfigCapturing:
    {
        OMX_CONFIG_BOOLEANTYPE  *pCaptype = (OMX_CONFIG_BOOLEANTYPE *)ComponentParameterStructure;
        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
        pCaptype->bEnabled = pPort->isCapture;
    }
    break;

    case OMX_IndexConfigCaptureMode:
    {
        OMX_CONFIG_CAPTUREMODETYPE  *pType = (OMX_CONFIG_CAPTUREMODETYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pCapMode, pType, sizeof(OMX_CONFIG_CAPTUREMODETYPE));
    }
    break;

    case OMX_IndexParamCommonSensorMode:
    {
        OMX_PARAM_SENSORMODETYPE  *pType = (OMX_PARAM_SENSORMODETYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pSensorMode, pType, sizeof(OMX_PARAM_SENSORMODETYPE));
    }
    break;

    /* 前处理相关函数 */
    case OMX_IndexParamFlashControl:
    case OMX_IndexConfigFlashControl:
    {
        OMX_IMAGE_PARAM_FLASHCONTROLTYPE  *pType = (OMX_IMAGE_PARAM_FLASHCONTROLTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pFlashType, pType, sizeof(OMX_IMAGE_PARAM_FLASHCONTROLTYPE));
    }
    break;

    case OMX_ACT_IndexConfigAGCExposureValue:/**< reference: OMX_ACT_CONFIG_AGCVALUE */
    {
        OMX_ACT_CONFIG_AGCVALUE *pType = (OMX_ACT_CONFIG_AGCVALUE *)ComponentParameterStructure;
        int nagc_gain_q8 = 0;

        if(pType->nPortIndex == OMX_ALL)
        {
            return OMX_ErrorBadParameter;
        }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        pPort->omx_camera->omx_camera_getctl(pPort->omx_camera, CAM_GET_GAIN, &nagc_gain_q8, OMX_PREVIEW_MODE);
        pType->nGain = 0;

        if(omx_camera_component_Private->Dgain_th[pPort->nSensorSelect] != -1)
        {
            if(nagc_gain_q8 > omx_camera_component_Private->Dgain_th[pPort->nSensorSelect])
            { pType->nGain = 1; }
            else
            { pType->nGain = 0; }
        }

        OMXDBUG(OMXDBUG_ERR, "OMX_ACT_IndexConfigAGCExposureValue=%d,%d,%d,%d\n", nagc_gain_q8, pType->nGain, pPort->nSensorSelect, omx_camera_component_Private->Dgain_th[pPort->nSensorSelect]);
    }
    break;

    case OMX_ACT_IndexConfigFlashStrobeValue:/**< reference: OMX_ACT_CONFIG_FlashStrobeParams */
    {
        OMX_ACT_CONFIG_FlashStrobeParams  *pType = (OMX_ACT_CONFIG_FlashStrobeParams *)ComponentParameterStructure;
        int portIndex = pType->nPortIndex;

        if(portIndex > OMX_BASE_FILTER_IMAGE_INDEX) { return OMX_ErrorBadParameter; }

        if(portIndex == (int)OMX_ALL)
        {
            return OMX_ErrorBadParameter;
        }
        else
        {
            pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
            inv_memcpy(pPort->act_flashstrobe, pType, sizeof(OMX_ACT_CONFIG_FlashStrobeParams));
        }
    }
    break;

    case OMX_IndexConfigCommonColorFormatConversion:
    {
        OMX_CONFIG_COLORCONVERSIONTYPE  *pType = (OMX_CONFIG_COLORCONVERSIONTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pColorFix, pType, sizeof(OMX_CONFIG_COLORCONVERSIONTYPE));
    }
    break;

    case OMX_IndexConfigCommonColorEnhancement://OMX_IndexConfigCommonColorEnhancement:
    {
        OMX_CONFIG_COLORENHANCEMENTTYPE  *pType = (OMX_CONFIG_COLORENHANCEMENTTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pColorEft, pType, sizeof(OMX_CONFIG_COLORENHANCEMENTTYPE));
    }
    break;

    case OMX_IndexConfigCommonImageFilter:
    {
        OMX_CONFIG_IMAGEFILTERTYPE  *pType = (OMX_CONFIG_IMAGEFILTERTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pImageFilter, pType, sizeof(OMX_CONFIG_IMAGEFILTERTYPE));
    }
    break;

    case OMX_IndexConfigCommonMirror:
    {
        OMX_CONFIG_MIRRORTYPE  *pType = (OMX_CONFIG_MIRRORTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pImageMirror, pType, sizeof(OMX_CONFIG_MIRRORTYPE));
    }
    break;

    case OMX_IndexConfigCommonOpticalZoom:
    {
        OMX_CONFIG_SCALEFACTORTYPE  *pType = (OMX_CONFIG_SCALEFACTORTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pOpticZoomType, pType, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
    }
    break;

    case OMX_IndexConfigCommonWhiteBalance:
    {
        OMX_CONFIG_WHITEBALCONTROLTYPE  *pType = (OMX_CONFIG_WHITEBALCONTROLTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pWBType, pType, sizeof(OMX_CONFIG_WHITEBALCONTROLTYPE));
    }
    break;

    case OMX_IndexConfigCommonExposure:
    {
        OMX_CONFIG_EXPOSURECONTROLTYPE  *pType = (OMX_CONFIG_EXPOSURECONTROLTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pExpType, pType, sizeof(OMX_CONFIG_EXPOSURECONTROLTYPE));
    }
    break;

    case OMX_IndexConfigCommonExposureValue:
    {
        OMX_CONFIG_EXPOSUREVALUETYPE  *pType = (OMX_CONFIG_EXPOSUREVALUETYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pExpVal, pType, sizeof(OMX_CONFIG_EXPOSUREVALUETYPE));
    }
    break;

    case OMX_IndexConfigCommonContrast:
    {
        OMX_CONFIG_CONTRASTTYPE  *pType = (OMX_CONFIG_CONTRASTTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pContrast, pType, sizeof(OMX_CONFIG_CONTRASTTYPE));
    }
    break;

    case OMX_IndexConfigCommonBrightness:
    {
        OMX_CONFIG_BRIGHTNESSTYPE  *pType = (OMX_CONFIG_BRIGHTNESSTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pBright, pType, sizeof(OMX_CONFIG_BRIGHTNESSTYPE));
    }
    break;

    case OMX_IndexConfigCommonGamma:
    {
        //Suggest::Not Support
        OMX_CONFIG_GAMMATYPE  *pType = (OMX_CONFIG_GAMMATYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pGamma, pType, sizeof(OMX_CONFIG_GAMMATYPE));
    }
    break;

    case OMX_IndexConfigCommonSaturation:
    {
        OMX_CONFIG_SATURATIONTYPE  *pType = (OMX_CONFIG_SATURATIONTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pSat, pType, sizeof(OMX_CONFIG_SATURATIONTYPE));
    }
    break;

    case OMX_IndexConfigFlickerRejection:
    {
        OMX_CONFIG_FLICKERREJECTIONTYPE  *pFlicktype = (OMX_CONFIG_FLICKERREJECTIONTYPE *)ComponentParameterStructure;

        if(pFlicktype->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pFlicktype->nPortIndex];
        inv_memcpy(pPort->pFlicktype, pFlicktype, sizeof(OMX_CONFIG_FLICKERREJECTIONTYPE));
    }
    break;

    case OMX_ACT_IndexConfigImageDeNoiseLevel:
    {
        //DNS 只支持0-3配置
        OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE  *pFlicktype = (OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *)ComponentParameterStructure;

        if(pFlicktype->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pFlicktype->nPortIndex];
        inv_memcpy(pPort->pNs_level, pFlicktype, sizeof(OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE));
    }
    break;

    case OMX_IndexConfigSharpness:
    {
        //sharpness 只有0-1,被IC定死,level不可选,效果不明显
        OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE  *pFlicktype = (OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *)ComponentParameterStructure;

        if(pFlicktype->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pFlicktype->nPortIndex];
        inv_memcpy(pPort->pSharp_level, pFlicktype, sizeof(OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE));
    }
    break;

    case OMX_ACT_IndexConfigGlobalBlitCompensation:
    {
        //背光补偿,软件方式补偿
        OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE  *pFlicktype = (OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE *)ComponentParameterStructure;

        if(pFlicktype->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pFlicktype->nPortIndex];
        inv_memcpy(pPort->pBlitComp, pFlicktype, sizeof(OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE));
    }
    break;
#if 0

    case OMX_IndexConfigCommonFocusStatus:
    {
        OMX_PARAM_FOCUSSTATUSTYPE  *pType = (OMX_PARAM_FOCUSSTATUSTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pAFStatus, pType, sizeof(OMX_PARAM_FOCUSSTATUSTYPE));
        omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);
    }
    break;

    case OMX_ACT_IndexConfigFocusDistance:
    {
        //提示对焦状态,给出焦距,用于EXIF信息
        OMX_ACT_CONFIG_FOCUSDISTANCETYPE  *pFlicktype = (OMX_ACT_CONFIG_FOCUSDISTANCETYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pFlicktype->nPortIndex];
        inv_memcpy(pPort->pAF_Dis, pFlicktype, sizeof(OMX_ACT_CONFIG_FOCUSDISTANCETYPE));
        omx_camera_component_Private->filter_setconfig(hComponent, nParamIndex, ComponentParameterStructure);
    }
    break;
#endif

    case OMX_IndexConfigCommonFocusRegion:
    {
        OMX_CONFIG_FOCUSREGIONTYPE  *pType = (OMX_CONFIG_FOCUSREGIONTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pAFRegion, pType, sizeof(OMX_CONFIG_FOCUSREGIONTYPE));
    }
    break;

    case OMX_IndexConfigFocusControl:
    {
        OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE  *pType = (OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        inv_memcpy(pPort->pFocusType, pType, sizeof(OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE));
    }
    break;

    case OMX_IndexConfigImageWhiteBalanceLock:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

        pType->eImageLock = pPort->bAWB_Lock;
    }
    break;

    case OMX_IndexConfigImageFocusLock:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];

        pType->eImageLock = pPort->bAF_Lock;
    }
    break;

    case OMX_IndexConfigImageExposureLock:
    {
        OMX_IMAGE_CONFIG_LOCKTYPE  *pType = (OMX_IMAGE_CONFIG_LOCKTYPE *)ComponentParameterStructure;

        if(pType->nPortIndex == OMX_ALL) { return OMX_ErrorBadParameter; }

        pPort = (omx_base_camera_video_PortType *) omx_camera_component_Private->ports[pType->nPortIndex];
        pType->eImageLock = pPort->bAE_Lock;
    }
    break;

    default: /*Call the base component function*/
        return omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
    }

    OMXDBUG(OMXDBUG_VERB, "  cam Getting parameter out %x\n", nParamIndex);
    return err;
}

OMX_ERRORTYPE omx_camera_component_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR ComponentParameterStructure)
{

    //  OMX_VIDEO_PARAM_PORTFORMATTYPE             *pVideoPortFormat;
    //  OMX_OTHER_PARAM_PORTFORMATTYPE             *pOtherPortFormat;
    //  OMX_ERRORTYPE                              err = OMX_ErrorNone;
    //  OMX_COMPONENTTYPE                          *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    // omx_camera_PrivateType                   *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    //  omx_base_camera_video_PortType                    *pPort;

    if(ComponentParameterStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }

    OMXDBUG(OMXDBUG_VERB, "   Getting Config %x\n", nParamIndex);

    return omx_camera_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
}

OMX_ERRORTYPE omx_camera_component_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR ComponentParameterStructure)
{

    //OMX_VIDEO_PARAM_PORTFORMATTYPE             *pVideoPortFormat;
    // OMX_OTHER_PARAM_PORTFORMATTYPE             *pOtherPortFormat;
    // OMX_ERRORTYPE                              err = OMX_ErrorNone;
    //OMX_COMPONENTTYPE                          *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    // omx_camera_PrivateType                   *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    // omx_base_camera_video_PortType                    *pPort;
    //  OMX_PARAM_COMPONENTROLETYPE                *pComponentRole;

    if(ComponentParameterStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }

    OMXDBUG(OMXDBUG_VERB, "   Getting parameter %i\n", nParamIndex);
    /* Check which structure we are being fed and fill its header */

    return omx_camera_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
}

/** This is the central function for component processing. It
  * is executed in a separate thread, is synchronized with
  * semaphores at each port, those are released each time a new buffer
  * is available on the given port.
  */
static void *omx_base_camera_filter_BufferMgmtFunction(void *param)
{
    OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)param;
    omx_camera_PrivateType *omx_base_filter_Private = (omx_camera_PrivateType *)openmaxStandComp->pComponentPrivate;
    omx_base_camera_video_PortType *pVidPort = (omx_base_camera_video_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
    omx_base_camera_video_PortType *pImgPort = (omx_base_camera_video_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
    tsem_t *pInputSem = pVidPort->pBufferSem;
    tsem_t *pOutputSem = pImgPort->pBufferSem;
    OMX_BUFFERHEADERTYPE *pOutputBuffer = NULL;
    OMX_BUFFERHEADERTYPE *pInputBuffer = NULL;
    OMX_BUFFERHEADERTYPE *pInputBuffer_Que = NULL;
    OMX_BOOL isStreamOn_Video = OMX_FALSE;
    OMX_CAMERATYPE *pCamType = NULL;
    int nPrePare_Num = 0;
    int nPreq_Num = 0;
    int try_ret = 0;
    unsigned int idx = 0;
    unsigned int buffphy = 0;
    unsigned int nCapturedNum = 0;
    unsigned int nCapPrePNum = 0;
    unsigned long phyAddr_of_Stat[4] = {0};
    long long ntimestamp = 0;
    int buffidx = 0;
    int nFrmCnt = 0;
    long long nframeEclapse = 0;
    int qready = 0;
    OMX_COMPONENTTYPE *target_component = NULL;
    int64_t lfisrtTime = 0;
    dec_buf_t vo_frm;
    char wdb_info[128];
    
    prctl(PR_SET_NAME, (unsigned long)"OMXCAM_BUFMNG", 0, 0, 0);
    OMXDBUG(OMXDBUG_VERB, "Inxxxx %s of component %p\n", __func__, openmaxStandComp);
    //OMXDBUG(OMXDBUG_VERB, "In %s the thread ID is %i\n", __func__, (int)omx_base_filter_Private->actionsThreads->nThreadBufferMngtID);
    OMXDBUG(OMXDBUG_VERB, "Inaaaa %s,%p,%p\n", __func__, pVidPort, pImgPort);

    omx_base_filter_Private->bellagioThreads->nThreadBufferMngtID = 1;//(long int)syscall(__NR_gettid);

    // watch dog
    if(NULL != omx_base_filter_Private->wdog_handle)
    { start_watch_dog(omx_base_filter_Private->wdog_handle); }

    /* checks if the component is in a state able to receive buffers */
    while(omx_base_filter_Private->state == OMX_StateIdle || omx_base_filter_Private->state == OMX_StateExecuting ||  omx_base_filter_Private->state == OMX_StatePause ||
            omx_base_filter_Private->transientState == OMX_TransStateLoadedToIdle)
    {

        /*Wait till the ports are being flushed*/
        pthread_mutex_lock(&omx_base_filter_Private->flush_mutex);

        while(PORT_IS_BEING_FLUSHED(pVidPort) || PORT_IS_BEING_FLUSHED(pImgPort))
        {
            pthread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

            // watch dog
            if(NULL != omx_base_filter_Private->wdog_handle)
            { stop_watch_dog(omx_base_filter_Private->wdog_handle); }

            if(PORT_IS_BEING_FLUSHED(pVidPort))
            {
                OMXDBUG(OMXDBUG_ERR, "Flush Video Port \n");

                if(pVidPort->bStreamOn == OMX_TRUE)
                {
                    pthread_mutex_lock(&omx_base_filter_Private->dq_mutex);
                    pVidPort->omx_camera->omx_camera_streamoff(pVidPort->omx_camera, OMX_PREVIEW_MODE);
                    pVidPort->bStreamOn = OMX_FALSE;
                    pthread_mutex_unlock(&omx_base_filter_Private->dq_mutex);
                }

                isStreamOn_Video = OMX_FALSE;
                omx_base_filter_Private->bTransFrom_C2V = OMX_FALSE;
                nPrePare_Num = getquenelem(pVidPort->pBufferQueue);

                while(nPrePare_Num > 0)
                {
                    tsem_down(pInputSem);
                    pInputBuffer = dequeue(pVidPort->pBufferQueue);
                    OMXDBUG(OMXDBUG_VERB, "In %s 1 signaling flush all cond iSemVal=%d,oSemval=%d\n",
                          __func__, pInputSem->semval, pOutputSem->semval);

                    if(pInputBuffer)
                    {
                        pVidPort->ReturnBufferFunction((omx_base_PortType *)pVidPort, (OMX_BUFFERHEADERTYPE *)pInputBuffer);
                        pInputBuffer = NULL;
                    }

                    nPrePare_Num = getquenelem(pVidPort->pBufferQueue);
                }

                nPrePare_Num = getquenelem(pVidPort->queue_dq);

                while(nPrePare_Num > 0)
                {
                    pInputBuffer = dequeue(pVidPort->queue_dq);

                    if(pInputBuffer)
                    {
                        //pVidPort->ReturnBufferFunction((omx_base_PortType*)pVidPort,(OMX_BUFFERHEADERTYPE*)pInputBuffer);
                        pInputBuffer = NULL;
                    }

                    nPrePare_Num = getquenelem(pVidPort->queue_dq);
                }
            }

            if(PORT_IS_BEING_FLUSHED(pImgPort))
            {
                OMXDBUG(OMXDBUG_ERR, "Flush Image Port \n");

                if(pImgPort->bStreamOn == OMX_TRUE)
                {
                    OMXDBUG(OMXDBUG_ERR, "Flush Image Port \n");
                    pCamType = pImgPort->omx_camera;

                    if(pImgPort->omx_camera->pCameraPrivate == NULL)
                    {
                        pCamType = pVidPort->omx_camera;
                    }

                    pthread_mutex_lock(&omx_base_filter_Private->dq_mutex);

                    if(omx_base_filter_Private->bTransFrom_C2V)
                    { pCamType->omx_camera_streamoff(pCamType, OMX_PREVIEW_CAP); }
                    else
                    { pCamType->omx_camera_streamoff(pCamType, pImgPort->pSensorMode->bOneShot); }

                    pImgPort->bStreamOn = OMX_FALSE;
                    pImgPort->isCapture = OMX_FALSE;
                    pthread_mutex_unlock(&omx_base_filter_Private->dq_mutex);
                    /*}else {
                      pthread_mutex_lock(&omx_base_filter_Private->dq_mutex);
                      pCamType->omx_camera_streamoff(pCamType,OMX_CAPTURE_MODE);
                      pImgPort->bStreamOn = OMX_FALSE;
                      pImgPort->isCapture = OMX_FALSE;
                    pthread_mutex_unlock(&omx_base_filter_Private->dq_mutex);*/
                }

                nPrePare_Num = getquenelem(pImgPort->pBufferQueue);

                while(nPrePare_Num > 0)
                {
                    tsem_down(pOutputSem);
                    pOutputBuffer = dequeue(pImgPort->pBufferQueue);
                    OMXDBUG(OMXDBUG_VERB, "In %s 1 signaling flush all cond iSemVal=%d,oSemval=%d\n",
                          __func__, pInputSem->semval, pOutputSem->semval);

                    //stream off and get buffer here.... fix...
                    if(pOutputBuffer)
                    {
                        pImgPort->ReturnBufferFunction((omx_base_PortType *)pImgPort, (OMX_BUFFERHEADERTYPE *)pOutputBuffer);
                        pOutputBuffer = NULL;
                    }

                    nPrePare_Num = getquenelem(pImgPort->pBufferQueue);
                }

                nPrePare_Num = getquenelem(pImgPort->queue_dq);

                while(nPrePare_Num > 0)
                {
                    pOutputBuffer = dequeue(pImgPort->queue_dq);

                    if(pOutputBuffer)
                    {
                        //pImgPort->ReturnBufferFunction((omx_base_PortType*)pImgPort,(OMX_BUFFERHEADERTYPE*)pOutputBuffer);
                        pOutputBuffer = NULL;
                    }

                    nPrePare_Num = getquenelem(pImgPort->queue_dq);
                }
            }

            //omx_base_filter_Private->isFlushed = OMX_TRUE;
            OMXDBUG(OMXDBUG_ERR, "In %s 2 signaling flush all cond iSemVal=%d,oSemval=%d\n",
                  __func__, pInputSem->semval, pOutputSem->semval);

            tsem_up(omx_base_filter_Private->flush_all_condition);
            tsem_down(omx_base_filter_Private->flush_condition);
            pthread_mutex_lock(&omx_base_filter_Private->flush_mutex);
        }

        pthread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

        if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid)
        {
            //omx_base_filter_Private->MNG_EXIT = OMX_TRUE;
            OMXDBUG(OMXDBUG_ERR, "In %s Buffer Management Thread is exiting\n", __func__);
            break;
        }

#if 0

        if(omx_base_filter_Private->MNG_STOP == OMX_TRUE)
        {
            OMXDBUG(OMXDBUG_ERR, "In %s Buffer Management Thread is exiting\n", __func__);
            omx_base_filter_Private->MNG_EXIT = OMX_TRUE;
            break;
        }

#endif

        pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex);

        if(pImgPort->isCapture == OMX_FALSE)
        {
            nCapturedNum = 0;

            if(pImgPort->bStreamOn == OMX_FALSE && pVidPort->bStreamOn == OMX_FALSE && omx_base_filter_Private->bTransFrom_C2V == OMX_TRUE && omx_base_filter_Private->state == OMX_StateExecuting) //omx_base_filter_Private->state==OMX_StatePause
            {
                pthread_mutex_lock(&omx_base_filter_Private->dq_mutex);
                try_ret = pVidPort->omx_camera->omx_camera_streamon(pVidPort->omx_camera, OMX_PREVIEW_MODE);

                if(try_ret == 0)
                {
                    omx_base_filter_Private->bTransFrom_C2V = OMX_FALSE;
                    pVidPort->bStreamOn = OMX_TRUE;
                }

                pthread_mutex_unlock(&omx_base_filter_Private->dq_mutex);
            }

            if(pVidPort->bStreamOn && pVidPort->sPortParam.bEnabled == OMX_TRUE)
            {
                isStreamOn_Video = OMX_TRUE;

                if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Buffer Management Thread is exiting\n", __func__);
                    pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                    break;
                }

                /*qready = getquenelem(pVidPort->queue_dq);
                while(qready > 0){
                    int buffidx = 0;
                    OMXDBUG(OMXDBUG_ERR,"qready %d\n",qready);
                    pInputBuffer_Que = dequeue(pVidPort->queue_dq);
                    buffidx = get_base_bufferidx(pVidPort->pBufferHeadAct,pInputBuffer_Que,pVidPort->sPortParam.nBufferCountActual);

                  if(buffidx != -1)
                  {
                      int err = pVidPort->omx_camera->omx_camera_qbuf(pVidPort->omx_camera,buffidx,pVidPort->sPortParam.nBufferSize,pVidPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr,pVidPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr,pVidPort->pSensorMode->bOneShot);
                      if(err != OMX_ErrorNone)
                      {
                          OMXDBUG(OMXDBUG_ERR,"Err %d\n",__LINE__);
                      }
                    }
                    qready--;
                }*/

                nPrePare_Num = getquenelem(pVidPort->pBufferQueue);

                /*No buffer to process. So wait here*/
                if((pVidPort->bStreamOn && nPrePare_Num <= 1) &&
                        (omx_base_filter_Private->state != OMX_StateLoaded && omx_base_filter_Private->state != OMX_StateInvalid))
                {
                    //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
                    OMXDBUG(OMXDBUG_VERB, "Waiting for next video input/output buffer %d,%d\n", pVidPort->bStreamOn, nPrePare_Num);
                    pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                    tsem_down(omx_base_filter_Private->bMgmtSem);
                    pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex);
                    OMXDBUG(OMXDBUG_VERB, "Wakeup for next video input/output buffer\n");
                }

                nPrePare_Num = getquenelem(pVidPort->pBufferQueue);
                qready = getquenelem(pVidPort->queue_dq);

                while(qready > 0 && pVidPort->bStreamOn == OMX_TRUE)
                {
                    int buffidx = 0;
                    //OMXDBUG(OMXDBUG_ERR,"qready %d\n",qready);
                    pInputBuffer_Que = dequeue(pVidPort->queue_dq);
                    buffidx = get_base_bufferidx(pVidPort->pBufferHeadAct, pInputBuffer_Que, pVidPort->sPortParam.nBufferCountActual);
                    pthread_mutex_lock(&omx_base_filter_Private->dq_mutex);

                    if(buffidx != -1 && pVidPort->bStreamOn == OMX_TRUE)
                    {
                        int err = OMX_ErrorNone;
                        pVidPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo.pPhyAddr = pVidPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr;
                        pVidPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo.pVirAddr = (unsigned long)pVidPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr;

                        if(pVidPort->bResizeEnable == 1 || pVidPort->bMJPEG_Enable == 1)
                        {
                            err = pVidPort->omx_camera->omx_camera_qbuf(pVidPort->omx_camera, buffidx, \
                                    pVidPort->pBufferHeadAct[buffidx].pConfigParam.bytes_of_resize, pVidPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize, \
                                    &pVidPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo, pVidPort->pSensorMode->bOneShot);
                        }
                        else
                        {
                            err = pVidPort->omx_camera->omx_camera_qbuf(pVidPort->omx_camera, buffidx, pVidPort->sPortParam.nBufferSize, pVidPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr, &pVidPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo, pVidPort->pSensorMode->bOneShot);
                        }

                        if(err != OMX_ErrorNone)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Err %d\n", __LINE__);
                        }
                    }

                    pthread_mutex_unlock(&omx_base_filter_Private->dq_mutex);
                    qready--;
                }



                if(nPrePare_Num > 1)
                {
                    long long cT1, cT2;
#if 1
                    ntimestamp = 0x0LL;
                    pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                    pthread_mutex_lock(&omx_base_filter_Private->dq_mutex);
                    //OMXDBUG(OMXDBUG_PARAM, "omx_camera_dqbuf vid in\n");
//	                    cT1 = get_current_time();
//	                    if(NULL != omx_base_filter_Private->wdog_handle)
//	                    {
//	                        sprintf(wdb_info, "%s %d", __func__, __LINE__);
//	                        tickle_watch_dog(omx_base_filter_Private->wdog_handle, wdb_info); 
//	                    }
                    phyAddr_of_Stat[0] = 0;
                    phyAddr_of_Stat[1] = 0;
                    try_ret = pVidPort->omx_camera->omx_camera_dqbuf(pVidPort->omx_camera, (int *)&idx, (unsigned long *)&buffphy, (unsigned long *)&phyAddr_of_Stat, &ntimestamp, OMX_PREVIEW_MODE);
//	                    if(NULL != omx_base_filter_Private->wdog_handle)
//	                    {
//	                        sprintf(wdb_info, "%s %d", __func__, __LINE__);
//	                        tickle_watch_dog(omx_base_filter_Private->wdog_handle, wdb_info); 
//	                    }
//	                    cT2 = get_current_time();
//	                    if((cT2 - cT1) < 500000)
//	                    { /*OMXDBUG(OMXDBUG_PARAM, "omx_camera_dqbuf vid out\n");*/ }
//	                    else
//	                    { OMXDBUG(OMXDBUG_ERR, "omx_camera_dqbuf vid out %lld\n", cT2 - cT1); }
                    pthread_mutex_unlock(&omx_base_filter_Private->dq_mutex);
                    pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex);

                    if(try_ret == -1)
                    {
                        OMXDBUG(OMXDBUG_VERB, "Error for omx_camera_dqbuf\n");
                        pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                        usleep(4000);
                        continue;
                    }

                    if(pVidPort->bBufferStateAllocated[idx] == BUFFER_FREE)
                    {
                        OMXDBUG(OMXDBUG_ERR, "BUFFER_FREED %d", idx);
                        pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                        continue;

                    }

                    //openmaxStandPort->bBufferStateAllocated[i]
                    //pVidPort->omx_camera->omx_camera_qbuf(pVidPort->omx_camera,idx,1280*720*3/2+4096,buffphy,phyAddr_of_Stat,PREVIEW_MODE);

#endif
                    tsem_down(pInputSem);
                    pInputBuffer = dequeue(pVidPort->pBufferQueue);
                    nPrePare_Num = getquenelem(pVidPort->pBufferQueue);

                    OMXDBUG(OMXDBUG_VERB, "v4l2camera:Left Buff Queue %d,%p\n", nPrePare_Num, pInputBuffer->pBuffer);

#if 1
                    buffidx = get_base_bufferidx(pVidPort->pBufferHeadAct, pInputBuffer, pVidPort->sPortParam.nBufferCountActual);

                    while(buffidx != (int)idx && pVidPort->pBufferQueue->nelem > 0 && pVidPort->bIsFullOfBuffers == OMX_TRUE)
                    {
                        OMXDBUG(OMXDBUG_ERR, "err in queue buffer now %d,%x,%p\n", idx, buffphy, pInputBuffer);
                        queue(pVidPort->pBufferQueue, pInputBuffer);
                        tsem_up(pInputSem);
                        tsem_down(pInputSem);
                        pInputBuffer = dequeue(pVidPort->pBufferQueue);
                        buffidx = get_base_bufferidx(pVidPort->pBufferHeadAct, pInputBuffer, pVidPort->sPortParam.nBufferCountActual);
                        nPrePare_Num = getquenelem(pVidPort->pBufferQueue);
                        OMXDBUG(OMXDBUG_ERR, "Err Seqs !!!! %p %d\n", pInputBuffer->pBuffer, nPrePare_Num);
                        usleep(40000);
                    }

#endif

                    if(omx_base_filter_Private->pMark.hMarkTargetComponent != NULL)
                    {
                        pInputBuffer->hMarkTargetComponent = omx_base_filter_Private->pMark.hMarkTargetComponent;
                        pInputBuffer->pMarkData            = omx_base_filter_Private->pMark.pMarkData;
                        omx_base_filter_Private->pMark.hMarkTargetComponent = NULL;
                        omx_base_filter_Private->pMark.pMarkData            = NULL;
                    }

                    target_component = (OMX_COMPONENTTYPE *)pInputBuffer->hMarkTargetComponent;

                    if(target_component == (OMX_COMPONENTTYPE *)openmaxStandComp)
                    {
                        /*Clear the mark and generate an event*/
                        (*(omx_base_filter_Private->callbacks->EventHandler))
                        (openmaxStandComp,
                         omx_base_filter_Private->callbackData,
                         OMX_EventMark, /* The command was completed */
                         1, /* The commands was a OMX_CommandStateSet */
                         0, /* The state has been changed in message->messageParam2 */
                         pInputBuffer->pMarkData);
                    }
                    else if(pInputBuffer->hMarkTargetComponent != NULL)
                    {
                        /*If this is not the target component then pass the mark*/
                        OMXDBUG(OMXDBUG_ERR, "Pass Mark. This is a Source!!\n");
                    }

                    if(pInputBuffer->nFilledLen == 0 && pVidPort->bBufferStateAllocated[buffidx] != BUFFER_FREE)
                    {
                        pInputBuffer->nFilledLen = pVidPort->pBufferHeadAct[buffidx].pConfigParam.buffersize;
                    }

                    pInputBuffer->nTimeStamp = (OMX_TICKS)ntimestamp;

                    if(lfisrtTime == 0) { lfisrtTime = ntimestamp; }

                    if(ntimestamp < lfisrtTime)
                    {
                        OMXDBUG(OMXDBUG_ERR, "TimeStamp ERR %lld,%lld,0x%x,0x%x,0x%x,0x%x\n", ntimestamp, ntimestamp - lfisrtTime, pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr[16], pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr[17], pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr[256], pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr[128]);
                    }

                    if((ntimestamp - lfisrtTime) > 500000)
                    {
                        OMXDBUG(OMXDBUG_ERR, "TimeStamp ERR ERR %lld,%lld,%d\n", ntimestamp, ntimestamp - lfisrtTime, nPrePare_Num);
                    }

                    if(nFrmCnt == 500)
                    {
                        OMXDBUG(OMXDBUG_ERR, "TimeStamp %lld,%lld,%lld\n", ntimestamp, ntimestamp - lfisrtTime, nframeEclapse / 500);
                        nFrmCnt = 0;
                        nframeEclapse = 0;
                    }

                    nFrmCnt++;
                    nframeEclapse += ntimestamp - lfisrtTime;
                    lfisrtTime = ntimestamp;

                    if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid)
                    {
                        //omx_base_filter_Private->MNG_EXIT = OMX_TRUE;
                        OMXDBUG(OMXDBUG_ERR, "In %s Buffer Management Thread is exiting\n", __func__);
                        pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                        break;
                    }
#if 0
                    if(omx_base_filter_Private->MNG_STOP == OMX_TRUE)
                    {
                        OMXDBUG(OMXDBUG_ERR, "In %s Buffer Management Thread is exiting\n", __func__);
                        omx_base_filter_Private->MNG_EXIT = OMX_TRUE;
                        break;
                    }
#endif
                    if(omx_base_filter_Private->state == OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pVidPort)))
                    {
                        /*Waiting at paused state*/
                        OMXDBUG(OMXDBUG_ERR, "In %s Waiting at paused state\n", __func__);
                        pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                        tsem_wait(omx_base_filter_Private->bStateSem);
                        pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex);
                    }

                    /*Input Buffer has been completely consumed. So, return input buffer*/
                    if(pInputBuffer && pInputBuffer->nFilledLen > 0)
                    {
                        if(pVidPort->bIsFullOfBuffers == OMX_TRUE)
                        { pInputBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME; }

                        if(phyAddr_of_Stat[0]&V4L2_BUF_FLAG_ERROR)
                        {
                            pInputBuffer->nFlags |= OMX_BUFFERFLAG_DATACORRUPT;
                        }

                        if(pVidPort->bStoreMediadata == OMX_FALSE && pVidPort->pBufferHeadAct[buffidx].pConfigParam.bUseBufFlag == OMX_TRUE && pVidPort->bBufferStateAllocated[buffidx] != BUFFER_FREE)
                        {
                            memcpy(pInputBuffer->pBuffer, pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr, pVidPort->sPortParam.nBufferSize);
                        }
#ifdef DBUG_CAMDATA
                        if(fraw_pv)
                        {
                            int w = pVidPort->nInputW; //pVidPort->sPortParam.format.video.nFrameWidth;
                            int h = pVidPort->nInputH; //pVidPort->sPortParam.format.video.nFrameHeight;
                            OMXDBUG(OMXDBUG_PARAM, "camera data, preview %dx%d\n", w, h);
                            if(!pVidPort->bResizeEnable)
                                fwrite(pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr, 1, w * h * 3 / 2, fraw_pv);
                        }
#endif
                        if(0 == pVidPort->nSensorType)
                        {
                            int shutter, gain;
                            void *src_phy = pVidPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr;
                            void *src_vir = pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr;
                            int format = IMX_YUV420SP;

                            if(OMX_COLOR_FormatYUV420Planar == pVidPort->sPortParam.format.video.eColorFormat
                                || OMX_COLOR_FormatYVU420Planar == pVidPort->sPortParam.format.video.eColorFormat)
                            {
                                format = IMX_YUV420P;
                            }
                            
                            pVidPort->omx_camera->omx_camera_getctl(pVidPort->omx_camera, CAM_GET_SHUTTER, &shutter, 0);
                            pVidPort->omx_camera->omx_camera_getctl(pVidPort->omx_camera, CAM_GET_GAIN, &gain, 0);
                            if(pVidPort->bResizeEnable)
                            {
                                src_phy = pVidPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize;
                                src_vir = pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr_of_resize;
                            }
                            
                            OMXDBUG(OMXDBUG_VERB, "nr&ee, preview %dx%d %d, %d, %d\n", pVidPort->nInputW, pVidPort->nInputH, format, shutter, gain);
                            imxctl_process(omx_base_filter_Private->imxctl_handle, src_phy, src_vir, pVidPort->nInputW, pVidPort->nInputH, 0, 0, format, shutter, gain, 0);
                        }

                        if(pVidPort->bResizeEnable == 1)
                        {
                            unsigned char *pOutBuffer = NULL;
                            unsigned char *pInBuffer = NULL;
                            int nInW = pVidPort->nInputW;
                            int nInH = pVidPort->nInputH;
                            int nOW = pVidPort->sPortParam.format.video.nFrameWidth;
                            int nOH = pVidPort->sPortParam.format.video.nFrameHeight;

                            pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex_resize);

                            if(pVidPort->nNumAssignedBuffers > 0)
                            {
                                pInBuffer = pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr_of_resize;
                                pOutBuffer = pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr;
                            }
                            else
                            { pInBuffer = NULL; }

                            if(pVidPort->bStoreMediadata && pInBuffer)
                            {
                                //video_metadata_t *pbuff = (video_metadata_t*)(pInputBuffer->pBuffer);
                                //void *pbuffhandle = pbuff->handle;
                                //void *vaddr = NULL;
                                //get_vir_addr_abs(pbuffhandle,&vaddr);
                                void *vaddr = get_vir_addr_mmap(pInputBuffer->pBuffer, GRALLOC_USAGE_SW_WRITE_OFTEN);

                                if(vaddr)
                                {
                                    pOutBuffer = (unsigned char *)vaddr;
                                    
                                    OMXDBUG(OMXDBUG_VERB, "SubSample images %d,%d,%p,%p\n", nInW, nOW, pInBuffer, pOutBuffer);
                                    //subsample_image(pInBuffer,nInW,nInH,pOutBuffer,nOW,nOH);
                                    downsample_image(pInBuffer, nInW, nInH, pOutBuffer, nOW, nOH, pVidPort->sPortParam.format.video.eColorFormat);
                                    //actal_cache_flush(pOutBuffer,nOW*nOH*3/2);
                                }

                                //if(pbuffhandle)   Actions_OSAL_UnlockANBHandle(pbuffhandle);
                                if(pInputBuffer->pBuffer)
                                {
                                    free_vir_addr_mmap(pInputBuffer->pBuffer);
                                }
                            }

                            pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex_resize);

                        }
                        else if(pVidPort->bMJPEG_Enable == 1)
                        {
                            unsigned char *pOutBuffer = NULL;
                            unsigned char *pInBuffer = NULL;
                            int nInW = pVidPort->nInputW;
                            int nInH = pVidPort->nInputH;
                            int err_mjpg = 0;
                            int nOW = pVidPort->sPortParam.format.video.nFrameWidth;
                            int nOH = pVidPort->sPortParam.format.video.nFrameHeight;
                            pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex_resize);

                            if(pVidPort->nNumAssignedBuffers > 0)
                            {
                                pInBuffer = pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr_of_resize;
                                pOutBuffer = pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr;
                            }
                            else
                            { pInBuffer = NULL; }

                            if(pVidPort->bStoreMediadata && pInBuffer)
                            {
                                //video_metadata_t *pbuff = (video_metadata_t*)(pInputBuffer->pBuffer);
                                //void *pbuffhandle = pbuff->handle;
                                //void *vaddr = NULL;
                                //get_vir_addr_abs(pbuffhandle,&vaddr);
                                void *vaddr = get_vir_addr_mmap(pInputBuffer->pBuffer, GRALLOC_USAGE_SW_WRITE_OFTEN);

                                if(vaddr)
                                {
                                    pOutBuffer = (unsigned char *)vaddr;
                                }

                                //if(pbuffhandle)   Actions_OSAL_UnlockANBHandle(pbuffhandle);
                            }

                            if(pInBuffer && pOutBuffer && omx_base_filter_Private->vdec_handle)
                            {
                                stream_buf_handle bitstream_buf;
                                frame_buf_handle outbuf;
#if 0
                                FILE *fp = fopen("/data/mjpeg.jpg","rb");
                                phyAddr_of_Stat[1] = 126960;
                                fread(pInBuffer,1,phyAddr_of_Stat[1],fp);
                                fclose(fp);
#endif
                                //OMXDBUG(OMXDBUG_ERR,"bMJPEG_data %d,%d,%p,%p",pInBuffer[0],pInBuffer[1],pInBuffer,pOutBuffer);
                                
                                outbuf.vo_frame_info = &vo_frm;
                                outbuf.vir_addr = pOutBuffer;
                                outbuf.phy_addr = pVidPort->pBufferHeadAct[idx].pConfigParam.phyAddr;

                                bitstream_buf.vir_addr = pInBuffer;
                                bitstream_buf.phy_addr = pVidPort->pBufferHeadAct[idx].pConfigParam.phyAddr_of_resize;
                                bitstream_buf.data_len = nInW * nInH * 3 / 2;
                                bitstream_buf.reserved = &outbuf;

                                if(phyAddr_of_Stat[1] > 0 && phyAddr_of_Stat[1] < bitstream_buf.data_len)
                                { bitstream_buf.data_len = phyAddr_of_Stat[1]; }
#if 0
                                fp = fopen("/data/IMAG0210_raw.yuv","wb");
                                fwrite(pOutBuffer,1,800*600*3/2,fp);
                                fclose(fp);
#endif
                                if(phyAddr_of_Stat[0]&V4L2_BUF_FLAG_ERROR)
                                {
                                    OMXDBUG(OMXDBUG_ERR, "V4L2_BUF_FLAG_ERROR");
                                }
                                else
                                {
                                    //double t0 = now_ms();
                                    actal_cache_flush(pInBuffer);
                                    omx_base_filter_Private->vdec_plugn->ex_ops(omx_base_filter_Private->vdec_handle, EX_RESERVED1, 1);
                                    err_mjpg = omx_base_filter_Private->vdec_plugn->decode_data(omx_base_filter_Private->vdec_handle, &bitstream_buf);
                                    //OMXDBUG(OMXDBUG_PARAM, "mjpeg decoding consumption: %gms\n", now_ms() - t0);
#if 0
                                    {
//	                                        FILE *fd;
//	                                        fd = fopen("/data/mjpeg.jpg", "wb");
//	                                        if(fd)
//	                                        {
//	                                            fwrite(pInBuffer, 1, bitstream_buf.data_len, fd);
//	                                            fclose(fd);
//	                                        }
//	                                        fd = fopen("/data/mjpeg.yuv", "wb");
//	                                        if(fd)
//	                                        {
//	                                            fwrite(pOutBuffer, 1, nInW * nInH * 3 / 2, fd);
//	                                            fclose(fd);
//	                                        }
                                        
                                        if(fraw)
                                        {
                                            fwrite(pInBuffer, 1, bitstream_buf.data_len, fraw);
                                        }
                                        if(fraw_info)
                                        {
                                            fprintf(fraw_info, "%d\n", bitstream_buf.data_len);
                                        }
//	                                        if(fraw_pv)
//	                                        {
//	                                            fwrite(pOutBuffer, 1, nInW * nInH * 3 / 2, fraw_pv);
//	                                        }
                                    }
#endif
                                    if(err_mjpg != 0)
                                    {
                                        pInputBuffer->nFlags |= OMX_BUFFERFLAG_DATACORRUPT;
                                    }
                                }
                            }

                            if(pVidPort->bStoreMediadata && pInBuffer)
                            {
                                //video_metadata_t *pbuff = (video_metadata_t*)(pInputBuffer->pBuffer);
                                //void *pbuffhandle = pbuff->handle;
                                //if(pbuffhandle)   Actions_OSAL_UnlockANBHandle(pbuffhandle);
                                if(pInputBuffer->pBuffer)
                                {
                                    free_vir_addr_mmap(pInputBuffer->pBuffer);
                                }
                            }

                            pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex_resize);
                        }

                        if(NULL != omx_base_filter_Private->wdog_handle)
                        {
                            sprintf(wdb_info, "%s %d", __func__, __LINE__);
                            tickle_watch_dog(omx_base_filter_Private->wdog_handle, wdb_info); 
                        }
                        pVidPort->ReturnBufferFunction((omx_base_PortType *)pVidPort, pInputBuffer);
                        if(NULL != omx_base_filter_Private->wdog_handle)
                        {
                            sprintf(wdb_info, "%s %d", __func__, __LINE__);
                            tickle_watch_dog(omx_base_filter_Private->wdog_handle, wdb_info); 
                        }
                        pInputBuffer = NULL;

                    }

                    //usleep(10000);
                }
            }
            else
            {
                //OMXDBUG(OMXDBUG_ERR,"In %s Waiting in \n",__func__);
                pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                tsem_down(omx_base_filter_Private->bMgmtSem);
                pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex);
                //OMXDBUG(OMXDBUG_ERR,"In %s Waiting out \n",__func__);
                //usleep(40000);
            }
        }
        else if(pImgPort->isCapture == OMX_TRUE)
        {
            unsigned char *pImgBufs[4] = {0};
            unsigned int nExpEv[4] = {0};
            
            OMXDBUG(OMXDBUG_PARAM, "CPI %d,%d\n", omx_base_filter_Private->bTransFrom_C2V, pImgPort->bStreamOn);
            
            pCamType = pImgPort->omx_camera;
            if(pImgPort->omx_camera->pCameraPrivate == NULL)
            {
                pCamType = pVidPort->omx_camera;
            }

            if(isStreamOn_Video == OMX_TRUE)
            {
                omx_base_filter_Private->bTransFrom_C2V = OMX_TRUE;
            }

            if(pImgPort->bStreamOn == OMX_TRUE)
            {
                if(pImgPort->bStreamOn == OMX_FALSE)
                {
                    OMXDBUG(OMXDBUG_VERB, "Waiting for next input/output bufferS \n");
                    pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                    tsem_down(omx_base_filter_Private->bMgmtSem);
                    pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex);
                }

                //          isStreamOn_Image = OMX_TRUE;
                if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid)
                {
                    // omx_base_filter_Private->MNG_EXIT = OMX_TRUE;
                    OMXDBUG(OMXDBUG_ERR, "In %s Buffer Management Thread is exiting\n", __func__);
                    pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                    break;
                }

#if 0

                if(omx_base_filter_Private->MNG_STOP == OMX_TRUE)
                {
                    OMXDBUG(OMXDBUG_ERR, "In %s Buffer Management Thread is exiting\n", __func__);
                    omx_base_filter_Private->MNG_EXIT = OMX_TRUE;
                    break;
                }

#endif
                nPrePare_Num = getquenelem(pImgPort->pBufferQueue);

                /*No buffer to process. So wait here*/
                if(((pImgPort->bStreamOn == OMX_TRUE) && nPrePare_Num <= 1) &&
                        (omx_base_filter_Private->state != OMX_StateLoaded && omx_base_filter_Private->state != OMX_StateInvalid))
                {
                    //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
                    OMXDBUG(OMXDBUG_VERB, "Waiting for next image input/output buffer\n");
                    pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                    tsem_down(omx_base_filter_Private->bMgmtSem);
                    pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex);
                    OMXDBUG(OMXDBUG_VERB, "Wakeup next image input/output buffer\n");

                }

                nPrePare_Num = getquenelem(pImgPort->pBufferQueue);
                qready = getquenelem(pImgPort->queue_dq);

                while(qready > 0 && (pImgPort->bHdr_Enable == OMX_FALSE))
                {
                    int buffidx = 0;
                    //OMXDBUG(OMXDBUG_ERR,"qready img %d,%d,%d\n",qready,nPrePare_Num,pImgPort->pSensorMode->bOneShot);
                    pInputBuffer_Que = dequeue(pImgPort->queue_dq);
                    buffidx = get_base_bufferidx(pImgPort->pBufferHeadAct, pInputBuffer_Que, pImgPort->sPortParam.nBufferCountActual);

                    if(buffidx != -1)
                    {
                        int err = OMX_ErrorNone;
                        pImgPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo.pPhyAddr = pImgPort->pBufferHeadAct[buffidx].pConfigParam.Stat_phyAddr;
                        pImgPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo.pVirAddr = (unsigned long)pImgPort->pBufferHeadAct[buffidx].pConfigParam.Stat_VirAddr;

                        err = pImgPort->omx_camera->omx_camera_qbuf(pImgPort->omx_camera, buffidx, pImgPort->sPortParam.nBufferSize, pImgPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr, &pImgPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo, pImgPort->pSensorMode->bOneShot);

                        if(err != OMX_ErrorNone)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Err %d\n", __LINE__);
                        }
                    }

                    qready--;
                }

                if(nPrePare_Num > 1)
                {
#if 1
                    if(pImgPort->bHdr_Enable == OMX_TRUE)
                    {
                        OMXDBUG(OMXDBUG_PARAM, "hdr dqbuf in, %dx%d\n", 
                            pImgPort->sPortParam.format.image.nFrameWidth, pImgPort->sPortParam.format.image.nFrameHeight);

                        phyAddr_of_Stat[0] = 0;
                        phyAddr_of_Stat[1] = 0;
                        try_ret = pCamType->omx_camera_dqbuf(pCamType, (int *)&idx, (unsigned long *)&buffphy, (unsigned long *)&phyAddr_of_Stat, &ntimestamp, pImgPort->isCapture);
                        if(try_ret == -1)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Error for capture omx_camera_dqbuf\n");
                            pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                            usleep(4000);
                            continue;
                        }
                        FillBuffer_Hdr(pImgPort,2);
                        pImgBufs[0] = (unsigned char *)pImgPort->pHDR_Buf[idx].VirAddr;
                        actal_cache_flush(pImgBufs[0]);
                        OMXDBUG(OMXDBUG_PARAM, "DQ idx %d,%x\n", idx, pImgBufs[0]);

                        phyAddr_of_Stat[0] = 0;
                        phyAddr_of_Stat[1] = 0;
                        try_ret = pCamType->omx_camera_dqbuf(pCamType, (int *)&idx, (unsigned long *)&buffphy, (unsigned long *)&phyAddr_of_Stat, &ntimestamp, pImgPort->isCapture);
                        if(try_ret == -1)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Error for capture omx_camera_dqbuf\n");
                            pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                            usleep(4000);
                            continue;
                        }
                        FillBuffer_Hdr(pImgPort,3);
                        pImgBufs[1] = (unsigned char *)pImgPort->pHDR_Buf[idx].VirAddr;
                        actal_cache_flush(pImgBufs[1]);
                        OMXDBUG(OMXDBUG_PARAM, "DQ idx %d,%x\n", idx, pImgBufs[1]);

                        phyAddr_of_Stat[0] = 0;
                        phyAddr_of_Stat[1] = 0;
                        try_ret = pCamType->omx_camera_dqbuf(pCamType, (int *)&idx, (unsigned long *)&buffphy, (unsigned long *)&phyAddr_of_Stat, &ntimestamp, pImgPort->isCapture);
                        if(try_ret == -1)
                        {
                            OMXDBUG(OMXDBUG_ERR, "Error for capture omx_camera_dqbuf\n");
                            pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                            usleep(4000);
                            continue;
                        }
                        pImgBufs[2] = (unsigned char *)pImgPort->pHDR_Buf[idx].VirAddr;
                        actal_cache_flush(pImgBufs[2]);
                        OMXDBUG(OMXDBUG_PARAM, "DQ idx %d,%x\n", idx, pImgBufs[2]);
                        
                        idx = 0;
                        
                        pCamType->omx_camera_getctl(pCamType, CAM_GET_EVS, nExpEv, 1);
                        OMXDBUG(OMXDBUG_PARAM, "hdr expo time&gain:%d,%d,%d\n", nExpEv[0], nExpEv[1], nExpEv[2]);
                        OMXDBUG(OMXDBUG_PARAM, "hdr dqbuf out\n");
                    }
                    else
#endif
                    {
//	                        if(NULL != omx_base_filter_Private->wdog_handle)
//	                        {
//	                            sprintf(wdb_info, "%s %d", __func__, __LINE__);
//	                            tickle_watch_dog(omx_base_filter_Private->wdog_handle, wdb_info); 
//	                        }
                        phyAddr_of_Stat[0] = 0;
                        try_ret = pCamType->omx_camera_dqbuf(pCamType, (int *)&idx, (unsigned long *)&buffphy, (unsigned long *)&phyAddr_of_Stat, &ntimestamp, pImgPort->isCapture);
//	                        if(NULL != omx_base_filter_Private->wdog_handle)
//	                        {
//	                            sprintf(wdb_info, "%s %d", __func__, __LINE__);
//	                            tickle_watch_dog(omx_base_filter_Private->wdog_handle, wdb_info); 
//	                        }
                        /*while(buffphy != pImgPort->pBufferHeadAct[0].pConfigParam.phyAddr && buffphy != pImgPort->pBufferHeadAct[1].pConfigParam.phyAddr){
                                pImgPort->omx_camera->omx_camera_qbuf(pImgPort->omx_camera,idx,pImgPort->sPortParam.nBufferSize,buffphy,(unsigned int)&pImgPort->pBufferHeadAct[buffidx].pConfigParam.mStatInfo,pImgPort->pSensorMode->bOneShot);
                                OMXDBUG(OMXDBUG_ERR,"buffphy %x,%x,%x\n",buffphy,pImgPort->pBufferHeadAct[0].pConfigParam.phyAddr,pImgPort->pBufferHeadAct[1].pConfigParam.phyAddr);
                                try_ret = pCamType->omx_camera_dqbuf(pCamType,(int*)&idx,(unsigned long*)&buffphy,(unsigned long*)&phyAddr_of_Stat,&ntimestamp,pImgPort->isCapture);
                        }*/

                        if(try_ret == -1)
                        {
                            OMXDBUG(OMXDBUG_VERB, "Error for Capture omx_camera_dqbuf\n");
                            pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                            usleep(8000);
                            continue;
                        }
                    }

                    tsem_down(pOutputSem);
                    pOutputBuffer = dequeue(pImgPort->pBufferQueue);

                    if(omx_base_filter_Private->pMark.hMarkTargetComponent != NULL)
                    {
                        pOutputBuffer->hMarkTargetComponent = omx_base_filter_Private->pMark.hMarkTargetComponent;
                        pOutputBuffer->pMarkData            = omx_base_filter_Private->pMark.pMarkData;
                        omx_base_filter_Private->pMark.hMarkTargetComponent = NULL;
                        omx_base_filter_Private->pMark.pMarkData            = NULL;
                    }

                    target_component = (OMX_COMPONENTTYPE *)pOutputBuffer->hMarkTargetComponent;

                    if(target_component == (OMX_COMPONENTTYPE *)openmaxStandComp)
                    {
                        /*Clear the mark and generate an event*/
                        (*(omx_base_filter_Private->callbacks->EventHandler))
                        (openmaxStandComp,
                         omx_base_filter_Private->callbackData,
                         OMX_EventMark, /* The command was completed */
                         1, /* The commands was a OMX_CommandStateSet */
                         0, /* The state has been changed in message->messageParam2 */
                         pOutputBuffer->pMarkData);
                    }
                    else if(pOutputBuffer->hMarkTargetComponent != NULL)
                    {
                        /*If this is not the target component then pass the mark*/
                        OMXDBUG(OMXDBUG_ERR, "Pass Mark. This is a Source!!\n");
                    }
                    
#ifdef DBUG_CAMDATA
                    if(fraw_cap)
                    {
                        int w = pImgPort->sPortParam.format.image.nFrameWidth;
                        int h = pImgPort->sPortParam.format.image.nFrameHeight;
                        void *pdata = pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr;
                        if(pImgPort->bResizeEnable)
                        {
                            w = pImgPort->nInputW;
                            h = pImgPort->nInputH;
                            pdata = pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr_of_resize;
                        }
                        OMXDBUG(OMXDBUG_PARAM, "camera data, capture, %dx%d\n", w, h);
                        fwrite(pdata, 1, w * h * 1.5, fraw_cap);
                    }
#endif

                    if(0 == pImgPort->nSensorType)
                    {
                        int shutter, gain;
                        void *src_phy = pImgPort->pBufferHeadAct[idx].pConfigParam.phyAddr;
                        void *src_vir = pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr;
                        int format = IMX_YUV420SP;

                        if(OMX_COLOR_FormatYUV420Planar == pImgPort->sPortParam.format.image.eColorFormat
                                || OMX_COLOR_FormatYVU420Planar == pImgPort->sPortParam.format.image.eColorFormat)
                        {
                            format = IMX_YUV420P;
                        }
                        
                        pImgPort->omx_camera->omx_camera_getctl(pImgPort->omx_camera, CAM_GET_SHUTTER, &shutter, 0);
                        pImgPort->omx_camera->omx_camera_getctl(pImgPort->omx_camera, CAM_GET_GAIN, &gain, 0);

                        if(pImgPort->bResizeEnable)
                        {
                            src_phy = pImgPort->pBufferHeadAct[idx].pConfigParam.phyAddr_of_resize;
                            src_vir = pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr_of_resize;
                        }
                        
                        OMXDBUG(OMXDBUG_PARAM, "nr&ee, capture %dx%d %d, %d, %d\n", pImgPort->nInputW, pImgPort->nInputH, format, shutter, gain);
                        imxctl_process(omx_base_filter_Private->imxctl_handle, src_phy, src_vir, pImgPort->nInputW, pImgPort->nInputH, 0, 0, format, shutter, gain, 1);
                    }

                    if(pImgPort->bResizeEnable == 1)
                    {
                        unsigned char *pOutBuffer = NULL;
                        unsigned char *pInBuffer = NULL;
                        int nInW = pImgPort->nInputW;
                        int nInH = pImgPort->nInputH;
                        int nOW = pImgPort->sPortParam.format.image.nFrameWidth;
                        int nOH = pImgPort->sPortParam.format.image.nFrameHeight;

                        pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex_resize);

                        if(pImgPort->nNumAssignedBuffers > 0)
                        {
                            pInBuffer = pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr_of_resize;
                            pOutBuffer = pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr;
                        }
                        else
                        { pInBuffer = NULL; }

                        if(pImgPort->bStoreMediadata && pInBuffer)
                        {
                            OMXDBUG(OMXDBUG_PARAM, "SubSample images %d,%d,%p,%p\n", nInW, nOW, pInBuffer, pOutBuffer);
                            downsample_image(pInBuffer, nInW, nInH, pOutBuffer, nOW, nOH, pImgPort->sPortParam.format.image.eColorFormat);
                            //actal_cache_flush(pOutBuffer,nOW*nOH*3/2);
                        }

                        pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex_resize);
                    }
                    else if(pImgPort->bMJPEG_Enable == 1)
                    {
                        unsigned char *pOutBuffer = NULL;
                        unsigned char *pInBuffer = NULL;
                        int nInW = pImgPort->nInputW;
                        int nInH = pImgPort->nInputH;
                        int err_mjpg = 0;
                        int nOW = pImgPort->sPortParam.format.image.nFrameWidth;
                        int nOH = pImgPort->sPortParam.format.image.nFrameHeight;
                        pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex_resize);

                        //pInBuffer = pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr_of_resize;
                        //pOutBuffer = pVidPort->pBufferHeadAct[buffidx].pConfigParam.VirAddr;
                        if(pImgPort->nNumAssignedBuffers > 0)
                        {
                            pInBuffer = pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr_of_resize;
                            pOutBuffer = pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr;
                        }
                        else
                        { pInBuffer = NULL; }

                        //OMXDBUG(OMXDBUG_ERR,"omx_camera_MJPG IMAGE addr %d,%x\n",buffidx,pImgPort->pBufferHeadAct[buffidx].pConfigParam.phyAddr_of_resize);
                        //subsample_image(pInBuffer,nInW,nInH,pOutBuffer,nOW,nOH);
                        //    OMXDBUG(OMXDBUG_ERR,"bMJPEG_Enable %d,%p,%p",idx,pInBuffer,pOutBuffer);
                        if(pImgPort->bStoreMediadata && pInBuffer)
                        {
                            //video_metadata_t *pbuff = (video_metadata_t*)(pOutputBuffer->pBuffer);
                            //void *pbuffhandle = pbuff->handle;
                            //void *vaddr = NULL;
                            //get_vir_addr_abs(pbuffhandle,&vaddr);
                            void *vaddr = get_vir_addr_mmap(pOutputBuffer->pBuffer, GRALLOC_USAGE_SW_WRITE_OFTEN);

                            if(vaddr)
                            {
                                pOutBuffer = (unsigned char *)vaddr;
                            }

                            //if(pbuffhandle)   Actions_OSAL_UnlockANBHandle(pbuffhandle);
                        }

                        if(pInBuffer && pOutBuffer && omx_base_filter_Private->vdec_handle)
                        {
                            stream_buf_handle bitstream_buf;
                            frame_buf_handle outbuf;

                            //FILE *fp = fopen("/data/IMAG0210_raw.jpg","rb");
                            //fread(pInBuffer,1,343212,fp);
                            //fclose(fp);

                            outbuf.vo_frame_info = &vo_frm;
                            outbuf.vir_addr = pOutBuffer;
                            outbuf.phy_addr = pImgPort->pBufferHeadAct[idx].pConfigParam.phyAddr;

                            bitstream_buf.vir_addr = pInBuffer;
                            bitstream_buf.phy_addr = pImgPort->pBufferHeadAct[idx].pConfigParam.phyAddr_of_resize;
                            bitstream_buf.data_len = nInW * nInH * 3 / 2;
                            bitstream_buf.reserved = &outbuf;

                            if(phyAddr_of_Stat[1] > 0 && phyAddr_of_Stat[1] < bitstream_buf.data_len)
                            { bitstream_buf.data_len = phyAddr_of_Stat[1]; }

                            if(bitstream_buf.data_len > 2000000)
                            {
                                bitstream_buf.data_len = 2000000 * 2 / 3 - 1;
                            }
                            OMXDBUG(OMXDBUG_VERB, "mjpeg data len: %dbytes\n", bitstream_buf.data_len);

                            //fp = fopen("/data/IMAG0210_raw.yuv","wb");
                            //fwrite(pOutBuffer,1,800*600*3/2,fp);
                            //fclose(fp);

                            if(phyAddr_of_Stat[0]&V4L2_BUF_FLAG_ERROR)
                            {
                                OMXDBUG(OMXDBUG_ERR, "V4L2_BUF_FLAG_ERROR");
                            }
                            else
                            {
                                actal_cache_flush(pInBuffer);
                                omx_base_filter_Private->vdec_plugn->ex_ops(omx_base_filter_Private->vdec_handle, EX_RESERVED1, 1);
                                err_mjpg = omx_base_filter_Private->vdec_plugn->decode_data(omx_base_filter_Private->vdec_handle, &bitstream_buf);
#if 0
                                {
                                      FILE *fd;
                                      fd = fopen("/data/mjpeg_cap.jpg", "wb");
                                      if(fd)
                                      {
                                          fwrite(pInBuffer, 1, bitstream_buf.data_len, fd);
                                          fclose(fd);
                                      }
                                      fd = fopen("/data/mjpeg_cap.yuv", "wb");
                                      if(fd)
                                      {
                                          fwrite(pOutBuffer, 1, nInW * nInH * 3 / 2, fd);
                                          fclose(fd);
                                      }
                                }
#endif
                                if(err_mjpg != 0)
                                {
                                    pOutputBuffer->nFlags |= OMX_BUFFERFLAG_DATACORRUPT;
                                }
                            }
                        }

                        if(pImgPort->bStoreMediadata && pInBuffer)
                        {
                            //video_metadata_t *pbuff = (video_metadata_t*)(pOutputBuffer->pBuffer);
                            //void *pbuffhandle = pbuff->handle;
                            //if(pbuffhandle)   Actions_OSAL_UnlockANBHandle(pbuffhandle);
                            if(pOutputBuffer->pBuffer)
                            {
                                free_vir_addr_mmap(pOutputBuffer->pBuffer);
                            }
                        }

                        pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex_resize);
                    }

                    if(pOutputBuffer->nFilledLen == 0)
                    {
                        pOutputBuffer->nFilledLen = pImgPort->pBufferHeadAct[idx].pConfigParam.buffersize;
                    }

                    if(phyAddr_of_Stat[0]&V4L2_BUF_FLAG_ERROR)
                    {
                        pOutputBuffer->nFlags |= OMX_BUFFERFLAG_DATACORRUPT;
                    }

                    pOutputBuffer->nTimeStamp = (OMX_TICKS)ntimestamp;

                    if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid)
                    {
                        //omx_base_filter_Private->MNG_EXIT = OMX_TRUE;
                        OMXDBUG(OMXDBUG_ERR, "In %s Buffer Management Thread is exiting\n", __func__);
                        pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                        break;
                    }

                    if(omx_base_filter_Private->state == OMX_StatePause && PORT_IS_BEING_FLUSHED(pImgPort))
                    {
                        /*Waiting at paused state*/
                        pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                        tsem_wait(omx_base_filter_Private->bStateSem);
                        pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex);
                    }

                    /*Input Buffer has been completely consumed. So, return input buffer*/
                    nCapturedNum++;
                    {
                        if(pImgPort->pCapMode->bContinuous == OMX_FALSE)
                        {
                            if(pImgPort->pCapMode->bFrameLimited == OMX_TRUE)
                            {
                                if(pImgPort->pCapMode->nFrameLimit >= (nCapturedNum - nCapPrePNum))
                                {
                                    OMXDBUG(OMXDBUG_VERB, "LOFF %i\n", __LINE__);

                                    if(pOutputBuffer && pOutputBuffer->nFilledLen > 0 && pImgPort->pCapExtMode->bPrepareCapture == OMX_FALSE)
                                    {
                                        if(pImgPort->bIsFullOfBuffers == OMX_TRUE)
                                        { pOutputBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME; }

                                        if(pImgPort->bStoreMediadata == OMX_FALSE && pImgPort->pBufferHeadAct[idx].pConfigParam.bUseBufFlag == OMX_TRUE)
                                        {
                                            memcpy(pOutputBuffer->pBuffer, pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr, pImgPort->sPortParam.nBufferSize);
                                        }

#if 1
                                        if(pImgPort->bHdr_Enable == OMX_TRUE)
                                        {
                                            OMXDBUG(OMXDBUG_ERR, "bHdr_Enable %i\n", __LINE__);

                                            if(pImgBufs[0] && pImgBufs[1] && pImgBufs[2])
                                            {
                                                if(pImgPort->bStoreMediadata)
                                                {
                                                    video_metadata_t *pbuff = (video_metadata_t *)(pOutputBuffer->pBuffer);
                                                    //void *pbuffhandle = pbuff->handle;
                                                    //void *vaddr = NULL;
                                                    //get_vir_addr(pbuffhandle,&vaddr);
                                                    void *vaddr = get_vir_addr_mmap(pbuff, GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);

                                                    if(vaddr)
                                                    { Process_Hdr(pImgPort, pImgBufs[0], pImgBufs[1], pImgBufs[2], vaddr, pImgPort->sPortParam.format.image.nFrameWidth, pImgPort->sPortParam.format.image.nFrameHeight, nExpEv); }

                                                    pbuff->off_x = 0;
                                                    pbuff->off_y = 0;
                                                    pbuff->crop_w = pImgPort->width_hdr;//CROP_W,H
                                                    pbuff->crop_h = pImgPort->height_hdr;
                                                    OMXDBUG(OMXDBUG_ERR, "Cropped width & height: %d, %d", pbuff->crop_w, pbuff->crop_h);
                                                    free_vir_addr_mmap(pbuff);
                                                }
                                            }
                                        }

#endif
                                        pImgPort->ReturnBufferFunction((omx_base_PortType *)pImgPort, pOutputBuffer);
                                        pOutputBuffer = NULL;
                                    }

                                    pImgPort->bStreamOn = OMX_FALSE;
                                    nPrePare_Num = getquenelem(pImgPort->pBufferQueue);
                                    nPreq_Num = getquenelem(pImgPort->queue_dq);
                                    pthread_mutex_lock(&omx_base_filter_Private->dq_mutex);

                                    if(omx_base_filter_Private->bTransFrom_C2V)
                                    { pCamType->omx_camera_streamoff(pCamType, OMX_PREVIEW_CAP); }
                                    else
                                    { pCamType->omx_camera_streamoff(pCamType, pImgPort->isCapture); }

                                    pthread_mutex_unlock(&omx_base_filter_Private->dq_mutex);
                                    pImgPort->isCapture = OMX_FALSE;

                                    while(nPreq_Num > 0)
                                    {
                                        pInputBuffer_Que = dequeue(pImgPort->queue_dq);
                                        pInputBuffer_Que = NULL;
                                        nPreq_Num--;// = getquenelem(pImgPort->queue_dq);
                                    }

                                    //omx_base_filter_Private->CurStatus  = OMX_FALSE;
                                    if(pImgPort->bCapturePause == OMX_TRUE)
                                    {
                                        omx_base_filter_Private->state = OMX_StatePause;
                                        OMXDBUG(OMXDBUG_VERB, "OMX_StatePause \n ");
                                    }

                                    //if(omx_base_filter_Private->bTransFrom_C2V)
                                    //omx_base_filter_Private->ComSetParam(openmaxStandComp,OMX_BASE_FILTER_VIDEO_INDEX);

                                    nCapturedNum = 0;
                                    nCapPrePNum = 0;
                                }
                                else
                                {
                                    OMXDBUG(OMXDBUG_ERR, "LOFF %i,%d,%d,%d\n", __LINE__, pImgPort->pCapMode->nFrameLimit, nCapturedNum, nCapPrePNum);
                                }
                            }
                            else
                            {
                                OMXDBUG(OMXDBUG_VERB, "LOFF %i,%d\n", __LINE__, omx_base_filter_Private->bTransFrom_C2V);

                                if(pOutputBuffer && pOutputBuffer->nFilledLen > 0 && pImgPort->pCapExtMode->bPrepareCapture == OMX_FALSE)
                                {
                                    if(pImgPort->bIsFullOfBuffers == OMX_TRUE)
                                    { pOutputBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME; }

                                    if(pImgPort->bStoreMediadata == OMX_FALSE && pImgPort->pBufferHeadAct[idx].pConfigParam.bUseBufFlag == OMX_TRUE)
                                    {
                                        memcpy(pOutputBuffer->pBuffer, pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr, pImgPort->sPortParam.nBufferSize);
                                    }

#if 1
                                    if(pImgPort->bHdr_Enable == OMX_TRUE)
                                    {
                                        OMXDBUG(OMXDBUG_ERR, "bHdr_Enable %i\n", __LINE__);

                                        if(pImgBufs[0] && pImgBufs[1] && pImgBufs[2])
                                        {
                                            if(pImgPort->bStoreMediadata)
                                            {
                                                video_metadata_t *pbuff = (video_metadata_t *)(pOutputBuffer->pBuffer);
                                                //void *pbuffhandle = pbuff->handle;
                                                //void *vaddr = NULL;
                                                //get_vir_addr(pbuffhandle,&vaddr);
                                                void *vaddr = get_vir_addr_mmap(pbuff, GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);

                                                if(vaddr)
                                                { Process_Hdr(pImgPort, pImgBufs[0], pImgBufs[1], pImgBufs[2], vaddr, pImgPort->sPortParam.format.image.nFrameWidth, pImgPort->sPortParam.format.image.nFrameHeight, nExpEv); }

                                                pbuff->off_x = 0;
                                                pbuff->off_y = 0;
                                                pbuff->crop_w = pImgPort->width_hdr;//CROP_W,H
                                                pbuff->crop_h = pImgPort->height_hdr;
                                                OMXDBUG(OMXDBUG_ERR, "Cropped width & height: %d, %d", pbuff->crop_w, pbuff->crop_h);
                                                free_vir_addr_mmap(pbuff);
                                            }

                                        }
                                    }

#endif
                                    pImgPort->ReturnBufferFunction((omx_base_PortType *)pImgPort, pOutputBuffer);
                                    pOutputBuffer = NULL;
                                }

                                pImgPort->bStreamOn = OMX_FALSE;
                                nPrePare_Num = getquenelem(pImgPort->pBufferQueue);
                                nPreq_Num = getquenelem(pImgPort->queue_dq);
                                pthread_mutex_lock(&omx_base_filter_Private->dq_mutex);

                                if(omx_base_filter_Private->bTransFrom_C2V)
                                { pCamType->omx_camera_streamoff(pCamType, OMX_PREVIEW_CAP); }
                                else
                                { pCamType->omx_camera_streamoff(pCamType, pImgPort->pSensorMode->bOneShot); }

                                pthread_mutex_unlock(&omx_base_filter_Private->dq_mutex);

                                while(nPreq_Num > 0)
                                {
                                    pInputBuffer_Que = dequeue(pImgPort->queue_dq);
                                    pInputBuffer_Que = NULL;
                                    nPreq_Num--;// = getquenelem(pImgPort->queue_dq);
                                }

                                pImgPort->isCapture = OMX_FALSE;

                                //omx_base_filter_Private->CurStatus  = OMX_FALSE;
                                if(pImgPort->bCapturePause == OMX_TRUE)
                                {
                                    omx_base_filter_Private->state = OMX_StatePause;
                                    OMXDBUG(OMXDBUG_VERB, "OMX_StatePause \n ");
                                }

                                //if(omx_base_filter_Private->bTransFrom_C2V)
                                //omx_base_filter_Private->ComSetParam(openmaxStandComp,OMX_BASE_FILTER_VIDEO_INDEX);

                                nCapturedNum = 0;
                                nCapPrePNum = 0;
                            }
                        }
                        else
                        {
                            if(pImgPort->pCapMode->bFrameLimited == OMX_TRUE)
                            {
                                if(pImgPort->pCapMode->nFrameLimit >= (nCapturedNum - nCapPrePNum))
                                {
                                    OMXDBUG(OMXDBUG_VERB, "LOFF %i\n", __LINE__);

                                    if(pOutputBuffer && pOutputBuffer->nFilledLen > 0 && pImgPort->pCapExtMode->bPrepareCapture == OMX_FALSE)
                                    {
                                        if(pImgPort->bIsFullOfBuffers == OMX_TRUE)
                                        { pOutputBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME; }

                                        if(pImgPort->bStoreMediadata == OMX_FALSE && pImgPort->pBufferHeadAct[idx].pConfigParam.bUseBufFlag == OMX_TRUE)
                                        {
                                            memcpy(pOutputBuffer->pBuffer, pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr, pImgPort->sPortParam.nBufferSize);
                                        }

                                        pImgPort->bStreamOn = OMX_FALSE;
                                        nPrePare_Num = getquenelem(pImgPort->pBufferQueue);
                                        nPreq_Num = getquenelem(pImgPort->queue_dq);
                                        pthread_mutex_lock(&omx_base_filter_Private->dq_mutex);

                                        if(omx_base_filter_Private->bTransFrom_C2V)
                                        { pCamType->omx_camera_streamoff(pCamType, OMX_PREVIEW_CAP); }
                                        else
                                        { pCamType->omx_camera_streamoff(pCamType, pImgPort->pSensorMode->bOneShot); }

                                        pthread_mutex_unlock(&omx_base_filter_Private->dq_mutex);
                                        pImgPort->isCapture = OMX_FALSE;
#if 1
                                        if(pImgPort->bHdr_Enable == OMX_TRUE)
                                        {
                                            OMXDBUG(OMXDBUG_ERR, "bHdr_Enable %i\n", __LINE__);

                                            if(pImgBufs[0] && pImgBufs[1] && pImgBufs[2])
                                            {
                                                if(pImgPort->bStoreMediadata)
                                                {
                                                    video_metadata_t *pbuff = (video_metadata_t *)(pOutputBuffer->pBuffer);
                                                    //void *pbuffhandle = pbuff->handle;
                                                    //void *vaddr = NULL;
                                                    //get_vir_addr(pbuffhandle,&vaddr);
                                                    void *vaddr = get_vir_addr_mmap(pbuff, GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);

                                                    if(vaddr)
                                                    { Process_Hdr(pImgPort, pImgBufs[0], pImgBufs[1], pImgBufs[2], vaddr, pImgPort->sPortParam.format.image.nFrameWidth, pImgPort->sPortParam.format.image.nFrameHeight, nExpEv); }

                                                    pbuff->off_x = 0;
                                                    pbuff->off_y = 0;
                                                    pbuff->crop_w = pImgPort->width_hdr;//CROP_W,H
                                                    pbuff->crop_h = pImgPort->height_hdr;
                                                    OMXDBUG(OMXDBUG_ERR, "Cropped width & height: %d, %d", pbuff->crop_w, pbuff->crop_h);
                                                    free_vir_addr_mmap(pbuff);
                                                }
                                            }

                                        }

#endif
                                        if(NULL != omx_base_filter_Private->wdog_handle)
                                        {
                                            sprintf(wdb_info, "%s %d", __func__, __LINE__);
                                            tickle_watch_dog(omx_base_filter_Private->wdog_handle, wdb_info); 
                                        }
                                        pImgPort->ReturnBufferFunction((omx_base_PortType *)pImgPort, pOutputBuffer);
                                        if(NULL != omx_base_filter_Private->wdog_handle)
                                        {
                                            sprintf(wdb_info, "%s %d", __func__, __LINE__);
                                            tickle_watch_dog(omx_base_filter_Private->wdog_handle, wdb_info); 
                                        }
                                        pOutputBuffer = NULL;
                                    }

                                    while(nPreq_Num > 0)
                                    {
                                        pInputBuffer_Que = dequeue(pImgPort->queue_dq);
                                        pInputBuffer_Que = NULL;
                                        nPreq_Num--;// = getquenelem(pImgPort->queue_dq);
                                    }

                                    //omx_base_filter_Private->CurStatus  = OMX_FALSE;
                                    if(pImgPort->bCapturePause == OMX_TRUE)
                                    {
                                        omx_base_filter_Private->state = OMX_StatePause;
                                        OMXDBUG(OMXDBUG_VERB, "OMX_StatePause \n ");
                                    }

                                    //if(omx_base_filter_Private->bTransFrom_C2V)
                                    //omx_base_filter_Private->ComSetParam(openmaxStandComp,OMX_BASE_FILTER_VIDEO_INDEX);

                                    nCapturedNum = 0;
                                    nCapPrePNum = 0;
                                }
                                else
                                {
                                    OMXDBUG(OMXDBUG_ERR, "LOFF %i,%d,%d,%d\n", __LINE__, pImgPort->pCapMode->nFrameLimit, nCapturedNum, nCapPrePNum);
                                }
                            }
                            else
                            {
                                //Capture frame one by one now
                                OMXDBUG(OMXDBUG_ERR, "LOFF %i\n", __LINE__);
//	                                pCamType->omx_camera_streamoff(pCamType,pImgPort->isCapture);
//	                                pImgPort->isCapture = OMX_FALSE;
//	                                omx_base_filter_Private->CurStatus  = OMX_FALSE;
//	                                if(pImgPort->bCapturePause == OMX_TRUE)
//	                                {
//	                                    omx_base_filter_Private->state = OMX_StatePause;
//	                                }
#if 0
                                if(omx_base_filter_Private->bTransFrom_C2V)
                                { pCamType->omx_camera_streamoff(pCamType, OMX_PREVIEW_CAP); }
                                else
                                { pCamType->omx_camera_streamoff(pCamType, pImgPort->pSensorMode->bOneShot); }

                                nPrePare_Num = getquenelem(pImgPort->queue_dq);

                                while(nPrePare_Num > 0)
                                {
                                    pInputBuffer_Que = dequeue(pImgPort->queue_dq);
                                    pInputBuffer_Que = NULL;
                                    nPrePare_Num = getquenelem(pImgPort->queue_dq);
                                }

                                pImgPort->isCapture = OMX_FALSE;

                                //omx_base_filter_Private->CurStatus  = OMX_FALSE;
                                if(pImgPort->bCapturePause == OMX_TRUE)
                                {
                                    omx_base_filter_Private->state = OMX_StatePause;
                                    OMXDBUG(OMXDBUG_VERB, "OMX_StatePause \n ");
                                }

                                //if(omx_base_filter_Private->bTransFrom_C2V)
                                //omx_base_filter_Private->ComSetParam(openmaxStandComp,OMX_BASE_FILTER_VIDEO_INDEX);
                                pImgPort->bStreamOn = OMX_FALSE;
                                nCapturedNum = 0;
                                nCapPrePNum = 0;
#endif

                                if(pOutputBuffer && pOutputBuffer->nFilledLen > 0 && pImgPort->pCapExtMode->bPrepareCapture == OMX_FALSE)
                                {
                                    if(pImgPort->bIsFullOfBuffers == OMX_TRUE)
                                    { pOutputBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME; }

                                    if(pImgPort->bStoreMediadata == OMX_FALSE && pImgPort->pBufferHeadAct[idx].pConfigParam.bUseBufFlag == OMX_TRUE)
                                    {
                                        memcpy(pOutputBuffer->pBuffer, pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr, pImgPort->sPortParam.nBufferSize);
                                    }

                                    pImgPort->ReturnBufferFunction((omx_base_PortType *)pImgPort, pOutputBuffer);
                                    pOutputBuffer = NULL;
                                }

                            }
                        }
                    }


                    if(pImgPort->pCapExtMode->bPrepareCapture == OMX_TRUE && \
                            nCapturedNum < pImgPort->pCapExtMode->nFrameBefore && pImgPort->bHdr_Enable == OMX_FALSE)
                    {
                        //按标准应该全部缓冲起来发出去，考虑memory 此处暂时全部丢弃
                        nCapPrePNum++;
                        pImgPort->pBufferHeadAct[idx].pConfigParam.mStatInfo.pPhyAddr = pImgPort->pBufferHeadAct[idx].pConfigParam.Stat_phyAddr;
                        pImgPort->pBufferHeadAct[idx].pConfigParam.mStatInfo.pVirAddr = (unsigned long)pImgPort->pBufferHeadAct[idx].pConfigParam.Stat_VirAddr;
                        pImgPort->omx_camera->omx_camera_qbuf(pImgPort->omx_camera, idx, \
                                                              pImgPort->sPortParam.nBufferSize, &pImgPort->pBufferHeadAct[idx].pConfigParam.phyAddr, \
                                                              (unsigned long)&pImgPort->pBufferHeadAct[idx].pConfigParam.mStatInfo, pImgPort->pSensorMode->bOneShot);
                        queue(pImgPort->pBufferQueue, pOutputBuffer);
                        tsem_up(pOutputSem);
                    }
                    else
                    {
                        if(pOutputBuffer && pOutputBuffer->nFilledLen > 0)
                        {
                            if(pImgPort->bIsFullOfBuffers == OMX_TRUE)
                            { pOutputBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME; }

                            if(pImgPort->bStoreMediadata == OMX_FALSE && pImgPort->pBufferHeadAct[idx].pConfigParam.bUseBufFlag == OMX_TRUE)
                            {
                                memcpy(pOutputBuffer->pBuffer, pImgPort->pBufferHeadAct[idx].pConfigParam.VirAddr, pImgPort->sPortParam.nBufferSize);
                            }
                            
                            pImgPort->ReturnBufferFunction((omx_base_PortType *)pImgPort, pOutputBuffer);
                            pOutputBuffer = NULL;
                        }

                        if(pImgPort->bStreamOn == OMX_FALSE)
                        {
                            if(pImgPort->pCapMode->bContinuous == OMX_TRUE && pImgPort->pCapMode->bFrameLimited == OMX_FALSE)
                            { nPrePare_Num = getquenelem(pImgPort->pBufferQueue); }

                            while(nPrePare_Num)
                            {
                                OMXDBUG(OMXDBUG_ERR, "LBufNum %i\n", nPrePare_Num);
                                tsem_down(pOutputSem);
                                pOutputBuffer = dequeue(pImgPort->pBufferQueue);

                                if(pOutputBuffer)
                                {
                                    pImgPort->ReturnBufferFunction((omx_base_PortType *)pImgPort, pOutputBuffer);
                                    pOutputBuffer = NULL;
                                }

                                nPrePare_Num--;// = getquenelem(pImgPort->pBufferQueue);
                            }

                        }
                    }
                }
                else
                {
                    if(pImgPort->bStreamOn == OMX_FALSE)
                    {
                        nCapturedNum = 0;
                        nCapPrePNum = 0;
                    }
                }
            }
            else
            {
                nCapturedNum = 0;
                nCapPrePNum = 0;
                //usleep(40000);
                //printf("stream image %d\n",omx_base_filter_Private->bMgmtSem->semval);
                pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
                tsem_down(omx_base_filter_Private->bMgmtSem);
                pthread_mutex_lock(&omx_base_filter_Private->cmd_mutex);
            }
        }

        pthread_mutex_unlock(&omx_base_filter_Private->cmd_mutex);
    }

    // omx_base_filter_Private->MNG_EXIT = OMX_TRUE;
    OMXDBUG(OMXDBUG_VERB, "Out of %s of component %p\n", __func__, openmaxStandComp);
    return NULL;
}

/** The Constructor
  * @param openmaxStandComp the component handle to be constructed
  * @param cComponentName is the name of the constructed component
  */
OMX_ERRORTYPE OMX_ComponentInit(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName)
{
    OMX_ERRORTYPE                                err = OMX_ErrorNone;
    omx_camera_PrivateType   *omx_camera_component_Private;
    omx_base_camera_video_PortType       *inPort, *outPort;
    OMX_U32                                      i;
    int nPortNum = 0;
    camera_module_info_t pModeInfo;

    if(!openmaxStandComp->pComponentPrivate)
    {

        openmaxStandComp->pComponentPrivate = calloc(1, sizeof(omx_camera_PrivateType));

        if(openmaxStandComp->pComponentPrivate == NULL)
        {
            return OMX_ErrorInsufficientResources;
        }

        OMXDBUG(OMXDBUG_ERR, "In %s, allocating component,%p\n", __func__, openmaxStandComp->pComponentPrivate);
    }
    else
    {
        OMXDBUG(OMXDBUG_VERB, "In %s, Error Component %p Already Allocated\n", __func__, openmaxStandComp->pComponentPrivate);
    }

    omx_camera_component_Private        = openmaxStandComp->pComponentPrivate;
    omx_camera_component_Private->ports = NULL;
    omx_camera_component_Private->openmaxStandComp = openmaxStandComp;
    OMXDBUG(OMXDBUG_ERR, "gomxcam_bIsInited %d,%d,%d,%d\n", gomxcam_bIsInited, gomxcam_bDoneQuery[0], gomxcam_bDoneQuery[1], gomxcam_bDoneQuery[2]);

    if(gomxcam_bIsInited == -1)
    {
        gomxcam_bDoneQuery[0] = gomxcam_bDoneQuery[1] = gomxcam_bDoneQuery[2] = 0;
        OMX_CONF_INIT_STRUCT(&gomxcam_OMX_CAPS[0], OMX_ACT_CAPTYPE);
        gomxcam_OMX_CAPS[0].nSensorIndex = 0;
        Module_CAPInit(&gomxcam_OMX_CAPS[0], 0);

        OMX_CONF_INIT_STRUCT(&gomxcam_OMX_CAPS[1], OMX_ACT_CAPTYPE);
        gomxcam_OMX_CAPS[1].nSensorIndex = 1;
        Module_CAPInit(&gomxcam_OMX_CAPS[1], 1);

        OMX_CONF_INIT_STRUCT(&gomxcam_OMX_CAPS[2], OMX_ACT_CAPTYPE);
        gomxcam_OMX_CAPS[2].nSensorIndex = 2;
        Module_CAPInit(&gomxcam_OMX_CAPS[2], 2);
        gomxcam_bIsInited = 1;
        OMXDBUG(OMXDBUG_ERR, "gomxcam_bIsInited is ok\n");
    }

    /** we could create our own port structures here
      * fixme maybe the base class could use a "port factory" function pointer?
      */
    // err = omx_base_camera_filter_Constructor(openmaxStandComp);
    {
        /* Call the base class constructor */
        err = omx_base_source_Constructor(openmaxStandComp, cComponentName);

        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "The base constructor failed in %s\n", __func__);
            return err;
        }

        GetV4L2LibAndHandle(omx_camera_component_Private);
        //omx_camera_component_Private->isFlushed = OMX_FALSE;

        /* Get Parma,If OMX_GetParamters */
        omx_camera_component_Private->pCapInfo[0] = calloc(1, sizeof(OMX_ACT_CAPTYPE));
        OMX_CONF_INIT_STRUCT(omx_camera_component_Private->pCapInfo[0], OMX_ACT_CAPTYPE);
        omx_camera_component_Private->pCapInfo[0]->nSensorIndex = 0;
        Module_Cap_Init(omx_camera_component_Private, 0);
        pModeInfo.supportParam[OMX_PrimarySensor] = omx_camera_component_Private->pCapInfo[0];
#if 0
        err = omx_camera_component_Private->camera_module_query(&pModeInfo, OMX_PrimarySensor);

        if(err == -1)
        {
            OMXDBUG(OMXDBUG_ERR, "open failed by OMX_PrimarySensor !!\n");
            omx_camera_component_Private->pCapInfo[0]->ulPreviewResCount = 0;
        }
        else
        { omx_camera_component_Private->nModuleType[0] = pModeInfo.supportType[0]; }

        OMXDBUG(OMXDBUG_ERR, "Pres Count %d\n", omx_camera_component_Private->pCapInfo[0]->ulPreviewResCount);
#if 1
        omx_camera_component_Private->pCapInfo[1] = calloc(1, sizeof(OMX_ACT_CAPTYPE));
        OMX_CONF_INIT_STRUCT(omx_camera_component_Private->pCapInfo[1], OMX_ACT_CAPTYPE);
        omx_camera_component_Private->pCapInfo[1]->nSensorIndex = 1;
        Module_Cap_Init(omx_camera_component_Private, 1);
        pModeInfo.supportParam[OMX_SecondarySensor] = omx_camera_component_Private->pCapInfo[1];
        err = omx_camera_component_Private->camera_module_query(&pModeInfo, OMX_SecondarySensor);

        if(err == -1)
        {
            OMXDBUG(OMXDBUG_ERR, "open failed by OMX_SecondarySensor !!\n");
            omx_camera_component_Private->pCapInfo[1]->ulPreviewResCount = 0;
        }
        else
        { omx_camera_component_Private->nModuleType[1] = pModeInfo.supportType[1]; }

        omx_camera_component_Private->pCapInfo[2] = calloc(1, sizeof(OMX_ACT_CAPTYPE));
        OMX_CONF_INIT_STRUCT(omx_camera_component_Private->pCapInfo[2], OMX_ACT_CAPTYPE);
        omx_camera_component_Private->pCapInfo[2]->nSensorIndex = 2;
        Module_Cap_Init(omx_camera_component_Private, 2);
        pModeInfo.supportParam[OMX_TI_StereoSensor] = omx_camera_component_Private->pCapInfo[2];
        err = omx_camera_component_Private->camera_module_query(&pModeInfo, OMX_TI_StereoSensor);

        if(err == -1)
        {
            OMXDBUG(OMXDBUG_ERR, "open failed by OMX_TI_StereoSensor !!\n");
            omx_camera_component_Private->pCapInfo[2]->ulPreviewResCount = 0;
        }
        else
        { omx_camera_component_Private->nModuleType[2] = pModeInfo.supportType[2]; }

#endif
#else
        omx_camera_component_Private->pCapInfo[1] = calloc(1, sizeof(OMX_ACT_CAPTYPE));
        OMX_CONF_INIT_STRUCT(omx_camera_component_Private->pCapInfo[1], OMX_ACT_CAPTYPE);
        omx_camera_component_Private->pCapInfo[1]->nSensorIndex = 1;
        Module_Cap_Init(omx_camera_component_Private, 1);
        omx_camera_component_Private->pCapInfo[2] = calloc(1, sizeof(OMX_ACT_CAPTYPE));
        OMX_CONF_INIT_STRUCT(omx_camera_component_Private->pCapInfo[2], OMX_ACT_CAPTYPE);
        omx_camera_component_Private->pCapInfo[2]->nSensorIndex = 2;
        Module_Cap_Init(omx_camera_component_Private, 2);
#endif


        omx_camera_component_Private->BufferMgmtFunction = omx_base_camera_filter_BufferMgmtFunction;
    }

    openmaxStandComp = omx_camera_component_Private->openmaxStandComp;
    omx_camera_component_Private->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber = 0;
    omx_camera_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts = 1;
    omx_camera_component_Private->sPortTypesParam[OMX_PortDomainImage].nStartPortNumber = 1;
    omx_camera_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts = 1;
    nPortNum = omx_camera_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts + omx_camera_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts;

    /** Allocate Ports and call port constructor. */
    if(nPortNum && !omx_camera_component_Private->ports)
    {
        omx_camera_component_Private->ports = calloc(nPortNum, sizeof(omx_base_camera_video_PortType *));

        if(!omx_camera_component_Private->ports)
        {
            return OMX_ErrorInsufficientResources;
        }

        for(i = omx_camera_component_Private->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber; i < omx_camera_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++)
        {
            omx_camera_component_Private->ports[i] = calloc(1, sizeof(omx_base_camera_video_PortType));

            if(!omx_camera_component_Private->ports[i])
            {
                return OMX_ErrorInsufficientResources;
            }
        }

        for(i = omx_camera_component_Private->sPortTypesParam[OMX_PortDomainImage].nStartPortNumber; i < omx_camera_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts; i++)
        {
            omx_camera_component_Private->ports[i] = calloc(1, sizeof(omx_base_camera_video_PortType));

            if(!omx_camera_component_Private->ports[i])
            {
                return OMX_ErrorInsufficientResources;
            }
        }

        base_camera_video_port_Constructor(openmaxStandComp, &omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX], 0, OMX_FALSE);
        base_camera_video_port_Constructor(openmaxStandComp, &omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX], 1, OMX_FALSE);
        OMXDBUG(OMXDBUG_VERB, "Port Init OK\n");
    }

    inPort = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[OMX_BASE_FILTER_VIDEO_INDEX];
    outPort = (omx_base_camera_video_PortType *)omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX];
    omx_camera_component_Private->destructor         = omx_camera_component_Destructor;

    inPort->sPortParam.bEnabled = OMX_FALSE;
    outPort->sPortParam.bEnabled = OMX_FALSE;
    openmaxStandComp->SetParameter  = omx_camera_component_SetParameter;
    openmaxStandComp->GetParameter  = omx_camera_component_GetParameter;
    openmaxStandComp->SetConfig  = omx_camera_component_SetConfig;
    openmaxStandComp->GetConfig  = omx_camera_component_GetConfig;
    openmaxStandComp->AllocateBuffer = omx_camera_component_AllocateBuffer;
    openmaxStandComp->UseBuffer = omx_camera_component_UseBuffer;
    openmaxStandComp->FreeBuffer = omx_camera_component_FreeBuffer;
    openmaxStandComp->FillThisBuffer = omx_camera_component_FillThisBuffer;
    openmaxStandComp->SendCommand = omx_camera_component_SendCommand;

    err = GetISPLibAndHandle(omx_camera_component_Private);
    OMXDBUG(OMXDBUG_VERB, "GetLib return %d,%p,%p\n", err, inPort, outPort);
    if(err == 0)
    {
        inPort->device_handle = omx_camera_component_Private->ispctl_handle;
        outPort->device_handle = omx_camera_component_Private->ispctl_handle;
        OMXDBUG(OMXDBUG_VERB, "Device Handle %x\n", (unsigned long)omx_camera_component_Private->ispctl_handle);
        inPort->omx_camera->pIppHnale = inPort->device_handle;
        outPort->omx_camera->pIppHnale = inPort->device_handle;
    }

    GetIMXLibAndHandle(omx_camera_component_Private);

    GetMJPGLibAndHandle(omx_camera_component_Private);
    //tsem_init(&omx_camera_component_Private->cmd_mutex,0);
    //tsem_init(&omx_camera_component_Private->dq_mutex,0);
    pthread_mutex_init(&omx_camera_component_Private->cmd_mutex, NULL);
    pthread_mutex_init(&omx_camera_component_Private->cmd_mutex_resize, NULL);
    pthread_mutex_init(&omx_camera_component_Private->dq_mutex, NULL);
    //omx_camera_component_Private->ComSetParam = SetCamParams;
    omx_camera_component_Private->nSensorSelect[0] = -100;
    omx_camera_component_Private->nSensorSelect[1] = -100;
    OMXDBUG(OMXDBUG_VERB, "Out %s\n", __func__);

    if(omx_camera_component_Private->ports[OMX_BASE_FILTER_IMAGE_INDEX])
    {
        GetHDRLibAndHandle(omx_camera_component_Private);
    }

    // watch dog
    omx_camera_component_Private->wdog_handle = open_watch_dog();
    if(NULL == omx_camera_component_Private->wdog_handle) 
    { OMXDBUG(OMXDBUG_ERR, "failed to open watch dog\n"); }

#ifdef DBUG_CAMDATA
    fraw = fopen("/data/cam.yuv", "wb");
    fraw_pv = fopen("/data/cam_pv.yuv", "wb");
    fraw_cap = fopen("/data/cam_cap.yuv", "wb");
    fraw_info = fopen("/data/cam.info", "wb");
#else
    fraw = NULL;
    fraw_pv = NULL;
    fraw_cap = NULL;
    fraw_info = NULL;
#endif

    return err;
}

/** The destructor
 */
OMX_ERRORTYPE omx_camera_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp)
{
    omx_camera_PrivateType   *omx_camera_component_Private = openmaxStandComp->pComponentPrivate;
    OMX_U32 i;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    OMXDBUG(OMXDBUG_VERB, "In %s\n", __func__);

    if(fraw)
    {
        fclose(fraw);
        fraw = NULL;
    }

    if(fraw_cap)
    {
        fclose(fraw_cap);
        fraw_cap = NULL;
    }

    if(fraw_pv)
    {
        fclose(fraw_pv);
        fraw_pv = NULL;
    }

    if(fraw_info)
    {
        fclose(fraw_info);
        fraw_info = NULL;
    }

    /* frees port/s */
    if(omx_camera_component_Private->ports)
    {
        for(i = omx_camera_component_Private->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber; i < omx_camera_component_Private->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber + omx_camera_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts ; i++)
        {
            if(omx_camera_component_Private->ports[i])
            {

                omx_camera_component_Private->ports[i]->PortDestructor(omx_camera_component_Private->ports[i]);
            }
        }

        for(i = omx_camera_component_Private->sPortTypesParam[OMX_PortDomainImage].nStartPortNumber; i < omx_camera_component_Private->sPortTypesParam[OMX_PortDomainImage].nStartPortNumber + omx_camera_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts ; i++)
        {
            if(omx_camera_component_Private->ports[i])
            {

                omx_camera_component_Private->ports[i]->PortDestructor(omx_camera_component_Private->ports[i]);
            }
        }

        free(omx_camera_component_Private->ports);
        omx_camera_component_Private->ports = NULL;
    }

    if(omx_camera_component_Private->bufferMgmtThreadID == 0)
    {
        tsem_signal(omx_camera_component_Private->bStateSem);
        /*Signal Buffer Management Thread to Exit*/
        tsem_up(omx_camera_component_Private->bMgmtSem);
        err = pthread_join(omx_camera_component_Private->bufferMgmtThread, NULL);
        OMXDBUG(OMXDBUG_VERB, "In %s after pthread_detach bufferMgmtThread\n", __func__);
        omx_camera_component_Private->bufferMgmtThreadID = -1;

        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_VERB, "In %s pthread_join returned err=%d\n", __func__, err);
        }
    }

    encFreeISPLibAndHandle(omx_camera_component_Private);
    encFreeIMXLibAndHandle(omx_camera_component_Private);
    encFreeMJPGLibAndHandle(omx_camera_component_Private);
    omx_camera_component_Private->camera_module_release();
    //omx_base_camera_filter_Destructor(openmaxStandComp);
    {
        encFreeV4L2LibAndHandle(omx_camera_component_Private);
        encFreeHDRILibAndHandle(omx_camera_component_Private);

        // watch dog
        if(NULL != omx_camera_component_Private->wdog_handle)
        {
            close_watch_dog(omx_camera_component_Private->wdog_handle);
            omx_camera_component_Private->wdog_handle = NULL;
        }

        if(omx_camera_component_Private->pCapInfo[0])
        {
            free(omx_camera_component_Private->pCapInfo[0]);
            omx_camera_component_Private->pCapInfo[0] = NULL;
        }

        if(omx_camera_component_Private->pCapInfo[1])
        {
            free(omx_camera_component_Private->pCapInfo[1]);
            omx_camera_component_Private->pCapInfo[1] = NULL;
        }

        if(omx_camera_component_Private->pCapInfo[2])
        {
            free(omx_camera_component_Private->pCapInfo[2]);
            omx_camera_component_Private->pCapInfo[2] = NULL;
        }

        //tsem_deinit(&omx_camera_component_Private->cmd_mutex);
        //tsem_deinit(&omx_camera_component_Private->dq_mutex);
        pthread_mutex_destroy(&omx_camera_component_Private->cmd_mutex);
        pthread_mutex_destroy(&omx_camera_component_Private->dq_mutex);
        pthread_mutex_destroy(&omx_camera_component_Private->cmd_mutex_resize);
        err = omx_base_source_Destructor(openmaxStandComp);

        if(err != OMX_ErrorNone)
        {
            OMXDBUG(OMXDBUG_ERR, "The base component destructor failed\n");
            return err;
        }
    }
    OMXDBUG(OMXDBUG_VERB, "Out of %s\n", __func__);

    return OMX_ErrorNone;
}

