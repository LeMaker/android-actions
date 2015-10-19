/**
 @file test/components/camera/omxcameratest.c

 Test application that uses two OpenMAX components, a camera and a fbsink.
 The preview port of the camera is tunneled with the fbsink component;
 The output video/image data of the capture port and thumbnail port of the
 camera are saved in disk files, respectively.

 Copyright (C) 2007-2008  Motorola and STMicroelectronics

 This code is licensed under LGPL see README for full LGPL notice.

 Date                           Author                Comment
 Fri, 06 Jul 2007               Motorola              File created
 Fri, 15 Feb 2008               Motorola              Update: The current implementation for this
 test app can only support one color format and
 image size on each port. To convert color formats
 and image sizes to other choices on some port,
 that port must be tunneled with a color conversion
 component.
 Tue, 06 Apr 2008               STM                   Update: Adding support for the color converter

 This Program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 $Date$
 Revision $Rev$
 Author $Author$

 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "omxcameratest.h"
#include "buffer_handle.h"
#include "hardware/gralloc.h"
#include "ACT_OMX_Index.h"
#include "ACT_OMX_IVCommon.h"

/** Callback prototypes for camera component */
static OMX_ERRORTYPE camera_sourceEventHandler(
  OMX_OUT OMX_HANDLETYPE hComponent,
  OMX_OUT OMX_PTR pAppData,
  OMX_OUT OMX_EVENTTYPE eEvent,
  OMX_OUT OMX_U32 Data1,
  OMX_OUT OMX_U32 Data2,
  OMX_OUT OMX_PTR pEventData);

static OMX_ERRORTYPE camera_sourceFillBufferDone(
  OMX_OUT OMX_HANDLETYPE hComponent,
  OMX_OUT OMX_PTR pAppData,
  OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);


/** Set parameters for camera/fbsink components */
static OMX_ERRORTYPE setCameraParameters(OMX_BOOL bCameraStillImageMode);

int surface_display_main_show(const void *buf, int width, int height,
    int format);
int surface_display_main_init(int w, int h);

/* Callbacks for camera component */
static OMX_CALLBACKTYPE camera_source_callbacks =
{
    .EventHandler = camera_sourceEventHandler,
    .EmptyBufferDone = NULL,
    .FillBufferDone = camera_sourceFillBufferDone
};

static appPrivateType* appPriv = NULL;

static OMX_PORTBUFFERCTXT sCameraPortBufferList[NUM_CAMERAPORTS];

/** callbacks implementation of camera component */
static OMX_ERRORTYPE camera_sourceEventHandler(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_EVENTTYPE eEvent,
        OMX_OUT OMX_U32 Data1,
        OMX_OUT OMX_U32 Data2,
        OMX_OUT OMX_PTR pEventData)
{   

    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for camera component\n",__func__);

    DEBUG(DEB_LEV_SIMPLE_SEQ, "Hi there, I am in the %s callback\n", __func__);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "%s: event type code (eEvent)=%d\n", __func__, eEvent);
    if(eEvent == OMX_EventCmdComplete)
    {
        if (Data1 == OMX_CommandStateSet)
        {
            DEBUG(DEB_LEV_SIMPLE_SEQ, "Set state to ");
            switch ((int)Data2)
            {   

                case OMX_StateLoaded:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
                break;
                case OMX_StateIdle:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
                break;
                case OMX_StateExecuting:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
                break;
                case OMX_StatePause:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
                break;
                case OMX_StateWaitForResources:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
                break;
                default:
                DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
                break;
            }
            tsem_up(appPriv->cameraSourceEventSem);
        }
        else
        {
            if(Data1 == OMX_CommandPortEnable)
            {
                tsem_up(appPriv->cameraSourceEventSem);
            }
        }
    }

    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for camera component, return code: 0x%X\n",__func__, OMX_ErrorNone);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE camera_sourceFillBufferDone(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{   

    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for camera component\n",__func__);

    DEBUG(DEB_LEV_FULL_SEQ, "%s: Get returned buffer (0x%lX) from port[%ld], nFilledLen=%ld\n", __func__, (OMX_U32)pBuffer, pBuffer->nOutputPortIndex, pBuffer->nFilledLen);

    if (pBuffer->nOutputPortIndex == OMX_CAMPORT_INDEX_CP)
    {
        DEBUG(DEB_LEV_FULL_SEQ, "%s: writing to file",__func__);
        //fwrite(pBuffer->pBuffer + pBuffer->nOffset, 1, pBuffer->nFilledLen, fCapture);
        pBuffer->nFilledLen = 0;
        pBuffer->nOffset = 0;
        OMX_FillThisBuffer(appPriv->camerahandle, pBuffer);
        fprintf(stdout, "capture buffer done!!!\n");
    }
    else if(pBuffer->nOutputPortIndex == OMX_CAMPORT_INDEX_VF)
    {
        if(pBuffer->nFilledLen >0)
        {
            unsigned char *vaddr = NULL;
            vaddr = bufferHandleLock((buffer_handle_t)pBuffer->pBuffer,0,0);
            if(vaddr){
            surface_display_main_show(vaddr+pBuffer->nOffset, DEFAULT_FRAME_WIDTH, DEFAULT_FRAME_HEIGHT,DEFAULT_CAMERA_COLOR_FORMAT);
            
            }
            bufferHandleUnlock((buffer_handle_t)pBuffer->pBuffer);
        }
        if ((pBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) != 0)
        {
            DEBUG(DEB_LEV_ERR, "%s: Get buffer flag OMX_BUFFERFLAG_STARTTIME!\n", __func__);
        }

        /*
#ifndef OMX_SKIP64BIT
        DEBUG(DEB_LEV_ERR, "%s: buffer[0x%lX] time stamp: 0x%016llX\n", __func__, (OMX_U32)pBuffer, pBuffer->nTimeStamp);
#else
        DEBUG(DEB_LEV_ERR, "%s: buffer[0x%lX] time stamp: 0x%08lX%08lX\n", __func__, (OMX_U32)pBuffer, pBuffer->nTimeStamp.nHighPart, pBuffer->nTimeStamp.nLowPart);
#endif
*/

        pBuffer->nFilledLen = 0;
        pBuffer->nOffset = 0;

        OMX_FillThisBuffer(appPriv->camerahandle, pBuffer);

    }

    DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for camera component, return code: 0x%X\n",__func__, OMX_ErrorNone);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE setCameraParameters(OMX_BOOL bCameraStillImageMode)
{
    OMX_ERRORTYPE errRet = OMX_ErrorNone;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sOmxPortDefinition;
    
    /* set preview port */
    setHeader(&sOmxPortDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    sOmxPortDefinition.nPortIndex = OMX_CAMPORT_INDEX_VF;
    if ((err = OMX_GetParameter(appPriv->camerahandle,
            OMX_IndexParamPortDefinition, &sOmxPortDefinition))
            != OMX_ErrorNone)
    {
        errRet = err;
    }
    else
    {
        sOmxPortDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
        sOmxPortDefinition.format.video.nFrameHeight = DEFAULT_FRAME_HEIGHT;
        sOmxPortDefinition.format.video.nStride = DEFAULT_FRAME_WIDTH;
        sOmxPortDefinition.format.video.nSliceHeight = DEFAULT_FRAME_HEIGHT;
        sOmxPortDefinition.format.video.eCompressionFormat
                = OMX_VIDEO_CodingUnused;
        sOmxPortDefinition.format.video.eColorFormat
                = DEFAULT_CAMERA_COLOR_FORMAT;
        sOmxPortDefinition.nBufferSize
                = sOmxPortDefinition.format.video.nStride
                        * sOmxPortDefinition.format.video.nFrameHeight * 2;
        if ((err = OMX_SetParameter(appPriv->camerahandle,
                OMX_IndexParamPortDefinition, &sOmxPortDefinition))
                != OMX_ErrorNone)
        {
            errRet = err;
        }
    }

    /* set capture port */
    setHeader(&sOmxPortDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    sOmxPortDefinition.nPortIndex = OMX_CAMPORT_INDEX_CP;
    if ((err = OMX_GetParameter(appPriv->camerahandle,
            OMX_IndexParamPortDefinition, &sOmxPortDefinition))
            != OMX_ErrorNone)
    {
        errRet = err;
    }
    else
    {
        sOmxPortDefinition.format.image.nFrameWidth = DEFAULT_FRAME_WIDTH;
        sOmxPortDefinition.format.image.nFrameHeight = DEFAULT_FRAME_HEIGHT;
        sOmxPortDefinition.format.image.nStride = DEFAULT_FRAME_WIDTH;
        sOmxPortDefinition.format.image.nSliceHeight = DEFAULT_FRAME_HEIGHT;
        sOmxPortDefinition.format.image.eCompressionFormat
                = OMX_VIDEO_CodingUnused;
        sOmxPortDefinition.format.image.eColorFormat
                = DEFAULT_CAMERA_COLOR_FORMAT;
        sOmxPortDefinition.nBufferSize
                = sOmxPortDefinition.format.image.nStride
                        * sOmxPortDefinition.format.image.nFrameHeight * 2;
        if ((err = OMX_SetParameter(appPriv->camerahandle,
                OMX_IndexParamPortDefinition, &sOmxPortDefinition))
                != OMX_ErrorNone)
        {
            errRet = err;
        }
    }

    
    return errRet;
}


int bufHandleTest()
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_BOOL bOmxInitialized = OMX_FALSE;
    OMX_PARAM_PORTDEFINITIONTYPE sOmxPortDefinition;
    OMX_CONFIG_BOOLEANTYPE sOmxCapturing;
    OMX_CONFIG_BOOLEANTYPE sOmxAutoPause;
    OMX_STATETYPE sOmxState;
    OMX_U32 nBufferCount;
    OMX_U32 nBufferSize;
    OMX_U32 nPortIndex;
    OMX_U32 i;
    unsigned int nPreviewTime = 5;/* By default, running for 5 sec for preview */
    unsigned int nCaptureTime = 5;/* By default, running for 5 sec for video capture */
    OMX_BOOL bCameraStillImageMode = OMX_FALSE; /* By default, the camera is running in video capture mode */
    OMX_BOOL bCameraAutoPause = OMX_FALSE; /* By default, the camera is not running in autopause mode */
    unsigned int nMaxRunCount = 1;/* By default, running once */
    unsigned int nRunCount = 0;

    buffer_handle_t bufHandle;
    OMX_S32 bufStride;
    int ret = 0;
    

    surface_display_main_init(DEFAULT_FRAME_WIDTH, DEFAULT_FRAME_HEIGHT);
    
    /* Init the Omx core */
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Init the OMX core\n");
    if ((err = OMX_Init()) != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,
                "The OpenMAX core can not be initialized. Exiting...\n");
        goto EXIT;
    }
    bOmxInitialized = OMX_TRUE;
    
    /* Initialize application private data */
    appPriv = malloc(sizeof(appPrivateType));
    if (appPriv == NULL)
    {
        DEBUG(DEB_LEV_ERR, "Allocate app private data failed!Exiting...\n");
        err = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    memset(appPriv, 0, sizeof(appPrivateType));
    
    memset(&sCameraPortBufferList, 0,
            NUM_CAMERAPORTS * sizeof(OMX_PORTBUFFERCTXT));
    
    
    /* Getting camera component handle */
    if ((err = OMX_GetHandle(&appPriv->camerahandle,
            "OMX.Action.Camera.Yuv", appPriv, &camera_source_callbacks))
            != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,
                "Getting camera component handle failed!Exiting...\n");
        goto EXIT;
    }
    //sensor select
    {
        OMX_PARAM_SENSORSELECTTYPE sensorType;
        setHeader(&sensorType, sizeof(OMX_PARAM_SENSORSELECTTYPE));
        sensorType.eSensor = OMX_PrimarySensor;

        if ((err = OMX_SetParameter(appPriv->camerahandle,
                OMX_ACT_IndexParamSensorSelect, &sensorType))
                != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,
                "Set sensor type failed!Exiting...\n");
            goto EXIT;
        }   

    }


    /* Setting parameters for camera component */
    if ((err = setCameraParameters(bCameraStillImageMode)) != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,
                "Set camera parameters failed! Use default settings...\n");
        /* Do not exit! */
    }

    //set store metadata
    {
        for (nPortIndex = OMX_CAMPORT_INDEX_VF; nPortIndex <= OMX_CAMPORT_INDEX_CP; nPortIndex++)
        { 
            StoreMetaDataInBuffersParams storeMetaData;
            setHeader(&storeMetaData, sizeof(StoreMetaDataInBuffersParams));
            storeMetaData.bStoreMetaData = OMX_TRUE;
            storeMetaData.nPortIndex = nPortIndex;

            if ((err = OMX_SetParameter(appPriv->camerahandle,
                        OMX_IndexParameterStoreMediaData, &storeMetaData))
                != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR,
                    "Set store metadata failed!Exiting...\n");
                goto EXIT;
            }   
        }

    }


    /* Allocate and init semaphores */
    appPriv->cameraSourceEventSem = malloc(sizeof(tsem_t));
    if (appPriv->cameraSourceEventSem == NULL)
    {
        DEBUG(DEB_LEV_ERR,
                "Allocate camera event semaphore failed!Exiting...\n");
        err = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    tsem_init(appPriv->cameraSourceEventSem, 0);
    
    RUN_AGAIN:

    for (nPortIndex = OMX_CAMPORT_INDEX_VF; nPortIndex <= OMX_CAMPORT_INDEX_CP; nPortIndex++)
    {
        
        if ((err = OMX_SendCommand(appPriv->camerahandle,
                OMX_CommandPortEnable, nPortIndex, NULL)) != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Camera Idle-->Exec failed!Exiting...\n");
            goto EXIT;
        }
        tsem_down(appPriv->cameraSourceEventSem);
    }
    /* Transition camera component Loaded-->Idle */
    if ((err = OMX_SendCommand(appPriv->camerahandle, OMX_CommandStateSet,
            OMX_StateIdle, NULL)) != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Camera Loaded-->Idle failed!Exiting...\n");
        goto EXIT;
    }

    /* Allocate port buffers for camera component */
    for (nPortIndex = OMX_CAMPORT_INDEX_VF; nPortIndex <= OMX_CAMPORT_INDEX_CP; nPortIndex++)
    {
        
        
        setHeader(&sOmxPortDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        sOmxPortDefinition.nPortIndex = nPortIndex;
        if ((err = OMX_GetParameter(appPriv->camerahandle,
                OMX_IndexParamPortDefinition, &sOmxPortDefinition))
                != OMX_ErrorNone)
        {
            DEBUG(
                    DEB_LEV_ERR,
                    "OMX_GetParameter for camera on OMX_IndexParamPortDefinition index failed!Exiting...\n");
            goto EXIT;
        }
        nBufferCount = sOmxPortDefinition.nBufferCountActual;
        nBufferSize = sOmxPortDefinition.nBufferSize;
        DEBUG(
                DEB_LEV_SIMPLE_SEQ,
                "Camera port[%ld] needs %ld buffers each of which is %ld bytes\n",
                nPortIndex, nBufferCount, nBufferSize);
        
        for (i = 0; i < nBufferCount; i++)
        {
            ret = bufferHandleAlloc(nBufferSize, 1, HAL_PIXEL_FORMAT_RGB_565,GRALLOC_USAGE_HW_CAMERA, &bufHandle,(int32_t *)&bufStride);
            if(ret != 0)
            {
                DEBUG(DEB_LEV_ERR,
                        "Allocate port handle buffer for camera failed!Exiting...\n");
                goto EXIT;
            }
            if ((err = OMX_UseBuffer(appPriv->camerahandle,
                    &sCameraPortBufferList[nPortIndex].pBufHeaderList[i],
                    nPortIndex, NULL, nBufferSize, (OMX_U8 *)bufHandle)) != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR,
                        "Allocate port buffer for camera failed!Exiting...\n");
                goto EXIT;
            }
            sCameraPortBufferList[nPortIndex].nBufferCountActual++;
        }
    }

    /* Wait camera (Loaded-->Idle) to complete */
    tsem_down(appPriv->cameraSourceEventSem);
    
    /* Transition camera component Idle-->Exec */
    if ((err = OMX_SendCommand(appPriv->camerahandle, OMX_CommandStateSet,
            OMX_StateExecuting, NULL)) != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Camera Idle-->Exec failed!Exiting...\n");
        goto EXIT;
    }

    /* Wait camera (Idle-->Exec) to complete */
    tsem_down(appPriv->cameraSourceEventSem);
    
    /* Fill buffers to camera preview port */
    for (i = 0; i
            < sCameraPortBufferList[OMX_CAMPORT_INDEX_VF].nBufferCountActual; i++)
    {
        sCameraPortBufferList[OMX_CAMPORT_INDEX_VF].pBufHeaderList[i]->nFilledLen
                = 0;
        sCameraPortBufferList[OMX_CAMPORT_INDEX_VF].pBufHeaderList[i]->nOffset
                = 0;
        if ((err = OMX_FillThisBuffer(appPriv->camerahandle,
                sCameraPortBufferList[OMX_CAMPORT_INDEX_VF].pBufHeaderList[i]))
                != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,
                    "Fill buffer to camera capture port failed!Exiting...,%d\n",__LINE__);
            goto EXIT;
        }
        DEBUG(
                DEB_LEV_SIMPLE_SEQ,
                "%s: Fill buffer[%ld] (0x%lX) to camera capture port\n",
                __func__,
                i,
                (OMX_U32) sCameraPortBufferList[OMX_CAMPORT_INDEX_VF].pBufHeaderList[i]);
    }

    /* Fill buffers to camera capture port */
    for (i = 0; i
            < sCameraPortBufferList[OMX_CAMPORT_INDEX_CP].nBufferCountActual; i++)
    {
        sCameraPortBufferList[OMX_CAMPORT_INDEX_CP].pBufHeaderList[i]->nFilledLen
                = 0;
        sCameraPortBufferList[OMX_CAMPORT_INDEX_CP].pBufHeaderList[i]->nOffset
                = 0;
        if ((err = OMX_FillThisBuffer(appPriv->camerahandle,
                sCameraPortBufferList[OMX_CAMPORT_INDEX_CP].pBufHeaderList[i]))
                != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,
                    "Fill buffer to camera capture port failed!Exiting...%d\n",__LINE__);
            goto EXIT;
        }
        DEBUG(
                DEB_LEV_SIMPLE_SEQ,
                "%s: Fill buffer[%ld] (0x%lX) to camera capture port\n",
                __func__,
                i,
                (OMX_U32) sCameraPortBufferList[OMX_CAMPORT_INDEX_CP].pBufHeaderList[i]);
    }

    fprintf(stdout, "Start preview, for %d sec...\n", nPreviewTime);
    sleep(nPreviewTime);
    
    /* Set up autopause mode */
    setHeader(&sOmxAutoPause, sizeof(OMX_CONFIG_BOOLEANTYPE));
    sOmxAutoPause.bEnabled = bCameraAutoPause;
    if ((err = OMX_SetConfig(appPriv->camerahandle,
            OMX_IndexAutoPauseAfterCapture, &sOmxAutoPause)) != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR,
                "Set autopause mode failed!Use default settings...\n");
        /* Do not exit */
    }

    /*  Start capturing */
    setHeader(&sOmxCapturing, sizeof(OMX_CONFIG_BOOLEANTYPE));
    sOmxCapturing.bEnabled = OMX_TRUE;
    if ((err = OMX_SetConfig(appPriv->camerahandle, OMX_IndexConfigCapturing,
            &sOmxCapturing)) != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Start capturing failed!Exiting...\n");
        goto EXIT;
    }

   fprintf(stdout, "Start capturing, for %d sec...\n", nCaptureTime);
    DEBUG(DEB_LEV_ERR, "Start capturing, for %d sec...\n", nCaptureTime);
    sleep(nCaptureTime);
    
    /*  Stop capturing */
    if (!bCameraStillImageMode)
    {
        setHeader(&sOmxCapturing, sizeof(OMX_CONFIG_BOOLEANTYPE));
        sOmxCapturing.bEnabled = OMX_FALSE;
        if ((err = OMX_SetConfig(appPriv->camerahandle,
                OMX_IndexConfigCapturing, &sOmxCapturing)) != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Stop capturing failed!Exiting...\n");
            goto EXIT;
        }
        fprintf(stdout, "Stop capturing...\n");
    }

    /* If in autopause mode, stay for a while before exit */
    if (bCameraAutoPause)
    {
        fprintf( stdout,"pause state for capture, sleep(%d)\n",5);
        sleep(5);
        /* Stop autopause mode */
        if ((err = OMX_GetState(appPriv->camerahandle, &sOmxState))
                != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR, "Get camera state failed!Exiting...\n");
            goto EXIT;
        }
        if (sOmxState == OMX_StatePause)
        {
            if ((err = OMX_SendCommand(appPriv->camerahandle,
                    OMX_CommandStateSet, OMX_StateExecuting, 0))
                    != OMX_ErrorNone)
            {
                DEBUG(DEB_LEV_ERR, "Pause-->Exec failed!Exiting...\n");
                goto EXIT;
            }
            /* Wait camera (Pause-->Exec) to complete */
            tsem_down(appPriv->cameraSourceEventSem);
            fprintf(
                    stdout,
                    "Now the camera is out of autopause mode, wait for %d sec before exit...\n",
                    5);
            sleep(5);
        }
        else
        {
            DEBUG(DEB_LEV_ERR,
                    "The camera is not in Pause state in autopause mode, ignore...\n");
        }
    }

#if 1
#if 0
    /* Transition camera component Exec-->Idle */
    if ((err = OMX_SendCommand(appPriv->camerahandle, OMX_CommandStateSet,
            OMX_StateIdle, NULL)) != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Camera Exec-->Idle failed!Exiting...\n");
        goto EXIT;
    }

    /* Wait camera (Exec-->Idle) to complete */
    tsem_down(appPriv->cameraSourceEventSem);
    /* Transition camera component Idle-->Exec */
    if ((err = OMX_SendCommand(appPriv->camerahandle, OMX_CommandStateSet,
            OMX_StateExecuting, NULL)) != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Camera Idle-->Exec failed!Exiting...\n");
        goto EXIT;
    }

    /* Wait camera (Idle-->Exec) to complete */
    tsem_down(appPriv->cameraSourceEventSem);
    
    /* Fill buffers to camera preview port */
    for (i = 0; i
            < sCameraPortBufferList[OMX_CAMPORT_INDEX_VF].nBufferCountActual; i++)
    {
        sCameraPortBufferList[OMX_CAMPORT_INDEX_VF].pBufHeaderList[i]->nFilledLen
                = 0;
        sCameraPortBufferList[OMX_CAMPORT_INDEX_VF].pBufHeaderList[i]->nOffset
                = 0;
        if ((err = OMX_FillThisBuffer(appPriv->camerahandle,
                sCameraPortBufferList[OMX_CAMPORT_INDEX_VF].pBufHeaderList[i]))
                != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,
                    "Fill buffer to camera capture port failed!Exiting...%d\n",__LINE__);
            goto EXIT;
        }
        DEBUG(
                DEB_LEV_SIMPLE_SEQ,
                "%s: Fill buffer[%ld] (0x%lX) to camera capture port\n",
                __func__,
                i,
                (OMX_U32) sCameraPortBufferList[OMX_CAMPORT_INDEX_VF].pBufHeaderList[i]);
    }

    /* Fill buffers to camera capture port */
    for (i = 0; i
            < sCameraPortBufferList[OMX_CAMPORT_INDEX_CP].nBufferCountActual; i++)
    {
        sCameraPortBufferList[OMX_CAMPORT_INDEX_CP].pBufHeaderList[i]->nFilledLen
                = 0;
        sCameraPortBufferList[OMX_CAMPORT_INDEX_CP].pBufHeaderList[i]->nOffset
                = 0;
        if ((err = OMX_FillThisBuffer(appPriv->camerahandle,
                sCameraPortBufferList[OMX_CAMPORT_INDEX_CP].pBufHeaderList[i]))
                != OMX_ErrorNone)
        {
            DEBUG(DEB_LEV_ERR,
                    "Fill buffer to camera capture port failed!Exiting...%d\n",__LINE__);
            goto EXIT;
        }
        DEBUG(
                DEB_LEV_SIMPLE_SEQ,
                "%s: Fill buffer[%ld] (0x%lX) to camera capture port\n",
                __func__,
                i,
                (OMX_U32) sCameraPortBufferList[OMX_CAMPORT_INDEX_CP].pBufHeaderList[i]);
    }
#endif    

    fprintf(stdout, "Continue to preview, for %d sec...\n", nPreviewTime);
    sleep(nPreviewTime);
    
#endif
    /* Transition camera component Exec-->Idle */
    if ((err = OMX_SendCommand(appPriv->camerahandle, OMX_CommandStateSet,
            OMX_StateIdle, NULL)) != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Camera Exec-->Idle failed!Exiting...\n");
        goto EXIT;
    }

    /* Wait camera (Exec-->Idle) to complete */
    tsem_down(appPriv->cameraSourceEventSem);
    
    /* Transition camera component Idle-->Loaded */
    if ((err = OMX_SendCommand(appPriv->camerahandle, OMX_CommandStateSet,
            OMX_StateLoaded, NULL)) != OMX_ErrorNone)
    {
        DEBUG(DEB_LEV_ERR, "Camera Idle-->Loaded failed!Exiting...\n");
        goto EXIT;
    }

#if 1
    /* Free bufers for each non-tunneled port of camera component */
    for (nPortIndex = OMX_CAMPORT_INDEX_VF; nPortIndex <= OMX_CAMPORT_INDEX_CP; nPortIndex++)
    {
        for (i = 0; i < sCameraPortBufferList[nPortIndex].nBufferCountActual; i++)
        {
            if (sCameraPortBufferList[nPortIndex].pBufHeaderList[i] != NULL)
            {
                bufHandle = (buffer_handle_t)sCameraPortBufferList[nPortIndex].pBufHeaderList[i]->pBuffer;
                OMX_FreeBuffer(appPriv->camerahandle, nPortIndex,
                        sCameraPortBufferList[nPortIndex].pBufHeaderList[i]);
                bufferHandleAllocFree(bufHandle);
            }
        }
        sCameraPortBufferList[nPortIndex].nBufferCountActual = 0;
    }
#endif
    
    /* Wait camera (Idle-->Loaded) to complete */
    tsem_down(appPriv->cameraSourceEventSem);
    
    nRunCount++;
    if (nRunCount < nMaxRunCount)
    {
        goto RUN_AGAIN;
    }

    
    EXIT: 
    
    for (nPortIndex = OMX_CAMPORT_INDEX_VF; nPortIndex <= OMX_CAMPORT_INDEX_CP; nPortIndex++)
    {
        for (i = 0; i < sCameraPortBufferList[nPortIndex].nBufferCountActual; i++)
        {
            if (sCameraPortBufferList[nPortIndex].pBufHeaderList[i] != NULL)
            {
                bufHandle = (buffer_handle_t)sCameraPortBufferList[nPortIndex].pBufHeaderList[i]->pBuffer;
                OMX_FreeBuffer(appPriv->camerahandle, nPortIndex,
                        sCameraPortBufferList[nPortIndex].pBufHeaderList[i]);
                DEBUG(DEB_LEV_ERR,
                    "bufferHandleAllocFree...%d\n",__LINE__);
                bufferHandleAllocFree(bufHandle);
            }
        }
        sCameraPortBufferList[nPortIndex].nBufferCountActual = 0;
    }       

    /* Free app private data */
    if (appPriv != NULL)
    {
        /* Free semaphores */
        if (appPriv->cameraSourceEventSem != NULL)
        {
            tsem_deinit(appPriv->cameraSourceEventSem);
            free(appPriv->cameraSourceEventSem);
        }

        /* Free camera component handle */
        if (appPriv->camerahandle != NULL)
        {
            OMX_FreeHandle(appPriv->camerahandle);
        }
        
        free(appPriv);
    }

    /* Deinit the Omx core */
    if (bOmxInitialized)
    {
        OMX_Deinit();
    }
    
    return (int) err;
}
