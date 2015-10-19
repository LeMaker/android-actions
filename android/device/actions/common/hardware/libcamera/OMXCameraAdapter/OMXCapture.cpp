/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

/**
* @file OMXCapture.cpp
*
* This file contains functionality for handling image capture.
*
*/

#include "CameraATraceTag.h"

#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"

#include "CameraConfigs.h"

namespace android
{
/**
 *
 * BUGFIX:  Fix for NV21 enum value to OMX.
 * BUGFIX:  Fix PIXEL_FORMAT_YUV422I align format OMX_COLOR_FormatYCbYCr; Add support for PIXEL_FORMAT_YUV422SP.
 *
 ************************************
 *
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::setParametersCapture(const CameraParameters &params,
        BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    const char *str = NULL;
    int w, h;
    int bpp;
    //ActionsCode(author:liuyiguang, change_code)
    int pixFormat;
    OMX_IMAGE_CODINGTYPE codingType;
    const char *valstr = NULL;
    int i;
    int cw,ch;
    int cropW,cropH;

    LOG_FUNCTION_NAME;

    OMXCameraPortParameters *cap;
    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    params.getPictureSize(&w, &h);


    for(i =0; i < (int)mCapabilities->mExtendedImageResolution.size(); i++)
    {
        const ExtendedResolution &extImageRes = mCapabilities->mExtendedImageResolution.itemAt(i);
        if(w == (int)extImageRes.width && h == (int)extImageRes.height)
        {
            cw = extImageRes.captureWidth; 
            ch = extImageRes.captureHeight; 
            if(extImageRes.needCrop)
            {
                cropW = extImageRes.cropWidth;
                cropH = extImageRes.cropHeight;
            }
            else
            {
                cropW = extImageRes.captureWidth;
                cropH = extImageRes.captureHeight;
            }
            break;
        }
    }     
    if(i >= (int)mCapabilities->mExtendedImageResolution.size())
    {
        cw = w;
        ch = h;
        cropW = w;
        cropH = h;
    }

    if ( ( cw != ( int ) cap->mWidth ) ||
            ( ch != ( int ) cap->mHeight ) )
    {
        mPendingCaptureSettings |= SetFormat;
        CAMHAL_LOGDB("mPendingCaptureSettings = %d", (int)mPendingCaptureSettings);
    }



    //TODO: Support more pixelformats

    CAMHAL_LOGDB("Image: cap.mWidth = %d", (int)cap->mWidth);
    CAMHAL_LOGDB("Image: cap.mHeight = %d", (int)cap->mHeight);
    CAMHAL_LOGDB("Image: cw = %d", (int)cw);
    CAMHAL_LOGDB("Image: ch = %d", (int)ch);

    if ( (valstr = params.getPictureFormat()) != NULL )
    {
        //ActionsCode(author:liuyiguang, change_code, fix YUV422I align YCbYCr, YUYV)
        if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
        {
            CAMHAL_LOGDA("YCbYCr format selected");
            pixFormat = OMX_COLOR_FormatYCbYCr;
            codingType = OMX_IMAGE_CodingUnused;
            bpp = 16;     
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_YUV422I;
        }
        //ActionsCode(author:liuyiguang, add_code, add support for PIXEL_FORMAT_YUV422SP)
        else if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV422SP) == 0)
        {
            CAMHAL_LOGDA("CbYCrY format selected");
            pixFormat = OMX_COLOR_FormatCbYCrY;
            codingType = OMX_IMAGE_CodingUnused;
            bpp = 16;     
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_YUV422SP;
        }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
        {
            CAMHAL_LOGDA("YUV420SP format selected");
            //ActionsCode(author:liuyiguang, change_code, for omx, NV21 is OMX_COLOR_FormatYVU420SemiPlanar)
            pixFormat = OMX_COLOR_FormatYVU420SemiPlanar;
            codingType = OMX_IMAGE_CodingUnused;
            bpp = 12;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_YUV420SP;
        }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
        {
            CAMHAL_LOGDA("YUV420SP format selected");
            //ActionsCode(author:liuyiguang, change_code, for omx, YV12 is OMX_COLOR_FormatYVU420Planar)
            pixFormat = OMX_COLOR_FormatYVU420Planar;
            codingType = OMX_IMAGE_CodingUnused;
            bpp = 12;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_YUV420P;
        }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_RGB565) == 0)
        {
            CAMHAL_LOGDA("RGB565 format selected");
            pixFormat = OMX_COLOR_Format16bitRGB565;
            codingType = OMX_IMAGE_CodingUnused;
            bpp = 16;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_RGB565;
        }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_JPEG) == 0)
        {
            const char *prevformatstr = NULL;
            CAMHAL_LOGDA("JPEG format selected");
#ifdef CAMERA_RUN_IN_EMULATOR
            pixFormat = OMX_COLOR_Format16bitRGB565;
            bpp = 16;
#else

            //same as preview format, for raw data format should same as preview format
            prevformatstr = params.getPreviewFormat();
            if(prevformatstr!=NULL && strcmp(prevformatstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
            {
                //ActionsCode(author:liuyiguang, change_code, for omx, YUV420P is OMX_COLOR_FormatYVU420Planar)
                pixFormat = OMX_COLOR_FormatYVU420Planar;
		bpp = 12;
            }
            else if (strcmp(prevformatstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
            {
                //ActionsCode(author:liuyiguang, change_code, for omx, YUV420SP is OMX_COLOR_FormatYVU420SemiPlanar)
                pixFormat = OMX_COLOR_FormatYVU420SemiPlanar;
		bpp = 12;
            }
	    else if (strcmp(prevformatstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
            {
                pixFormat = OMX_COLOR_FormatYCbYCr;
		bpp = 16;
            }
            else
            {
                pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
		bpp = 12;
            }
          
#endif
            codingType = OMX_IMAGE_CodingUnused;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_JPEG;
        }
        else
        {
            CAMHAL_LOGEA("Invalid format, yuv420sp format selected as default");
            //ActionsCode(author:liuyiguang, change_code, for omx, NV21 is OMX_COLOR_FormatYVU420SemiPlanar)
            pixFormat = OMX_COLOR_FormatYVU420SemiPlanar;
            codingType = OMX_IMAGE_CodingUnused;
            bpp = 16;
            mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_JPEG;
        }
    }
    else
    {
        CAMHAL_LOGEA("Picture format is NULL, defaulting to yuv420sp");
        //ActionsCode(author:liuyiguang, change_code, for omx, NV21 is OMX_COLOR_FormatYVU420SemiPlanar)
        pixFormat = OMX_COLOR_FormatYVU420SemiPlanar;
        codingType = OMX_IMAGE_CodingUnused;
        bpp = 16;
        mPictureFormatFromClient = CameraParameters::PIXEL_FORMAT_JPEG;
    }

    if ( ( w != ( int ) cap->mEncodeWidth ) ||
            ( h != ( int ) cap->mEncodeHeight ) )
    {
        cap->mPendingSize = true;
    }

    cap->mPendingWidth = cw;
    cap->mPendingHeight = ch;
    cap->mPendingEncodeWidth = w;
    cap->mPendingEncodeHeight = h;
    cap->mPendingCropWidth = cropW;
    cap->mPendingCropHeight = cropH;

    cap->mPendingStride = bpp*cap->mPendingWidth>>3;
    cap->mPendingBufSize = cap->mPendingStride * ch;

    CAMHAL_LOGDB("cap->mColorFormat = %d", (int)cap->mColorFormat);
    CAMHAL_LOGDB("cap->mCompressionFormat = %d", (int)cap->mCompressionFormat);
    CAMHAL_LOGDB("pixFormat = %d", (int)pixFormat);
    CAMHAL_LOGDB("codingType = %d", (int)codingType);

    if ( pixFormat != cap->mColorFormat || codingType != cap->mCompressionFormat)
    {
        mPendingCaptureSettings |= SetFormat;
        //ActionsCode(author:liuyiguang, change_code)
        cap->mColorFormat = (OMX_COLOR_FORMATTYPE)pixFormat;
        cap->mCompressionFormat = codingType;
        CAMHAL_LOGDB("mPendingCaptureSettings = %d", (int)mPendingCaptureSettings);
    }

    if ( ( params.getInt(CameraParameters::KEY_JPEG_QUALITY)  >= MIN_JPEG_QUALITY ) &&
            ( params.getInt(CameraParameters::KEY_JPEG_QUALITY)  <= MAX_JPEG_QUALITY ) )
    {
        if (static_cast<unsigned int>(params.getInt(CameraParameters::KEY_JPEG_QUALITY)) != mPictureQuality)
        {
            mPendingCaptureSettings |= SetQuality;
        }
        mPictureQuality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    }
    else
    {
        if (mPictureQuality != MAX_JPEG_QUALITY) mPendingCaptureSettings |= SetQuality;
        mPictureQuality = MAX_JPEG_QUALITY;
    }

    CAMHAL_LOGVB("Picture Quality set %d", mPictureQuality);

    if ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH)  >= 0 )
    {
        if (static_cast<unsigned int>(params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH)) != mThumbWidth)
        {
            mPendingCaptureSettings |= SetThumb;
        }
        mThumbWidth = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    }
    else
    {
        if (mThumbWidth != DEFAULT_THUMB_WIDTH) mPendingCaptureSettings |= SetThumb;
        mThumbWidth = DEFAULT_THUMB_WIDTH;
    }


    CAMHAL_LOGVB("Picture Thumb width set %d", mThumbWidth);

    if ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT)  >= 0 )
    {
        if (static_cast<unsigned int>(params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT)) != mThumbHeight)
        {
            mPendingCaptureSettings |= SetThumb;
        }
        mThumbHeight = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    }
    else
    {
        if (mThumbHeight != DEFAULT_THUMB_HEIGHT) mPendingCaptureSettings |= SetThumb;
        mThumbHeight = DEFAULT_THUMB_HEIGHT;
    }


    CAMHAL_LOGVB("Picture Thumb height set %d", mThumbHeight);

    if ( ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY)  >= MIN_JPEG_QUALITY ) &&
            ( params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY)  <= MAX_JPEG_QUALITY ) )
    {
        if (static_cast<unsigned int>(params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY)) != mThumbQuality)
        {
            mPendingCaptureSettings |= SetThumb;
        }
        mThumbQuality = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    }
    else
    {
        if (mThumbQuality != MAX_JPEG_QUALITY) mPendingCaptureSettings |= SetThumb;
        mThumbQuality = MAX_JPEG_QUALITY;
    }

    CAMHAL_LOGDB("Thumbnail Quality set %d", mThumbQuality);

    if (mFirstTimeInit)
    {
        mPendingCaptureSettings = ECapturesettingsAll;
    }

    


    LOG_FUNCTION_NAME_EXIT;

    return ret;
}
/**
 * BUGFIX: fix for 5.1 cts testBurstVideoSnapshot fail . 
 *ActionsCode(author:liyuan, change_code)
 */
status_t OMXCameraAdapter::getPictureBufferSize(size_t &length, size_t bufferCount)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters *imgCaptureData = NULL;
    OMXCameraPortParameters *previewData = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;
    CAMERA_SCOPEDTRACE("getPictureBufferSize");

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
    previewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];


    CAMHAL_LOGDB("mPendingCaptureSettings = %d", (int)mPendingCaptureSettings);
    if (mPendingCaptureSettings&SetFormat)
    {
        disableImagePort();
        if ( NULL != mReleaseImageBuffersCallback )
        {
            CAMHAL_LOGDA("mReleaseImageBuffersCallback called");
            mReleaseImageBuffersCallback(mReleaseData);
        }
    }

    imgCaptureData->mNumBufs = (OMX_U8)bufferCount;

    // check if image port is already configured...
    // if it already configured then we don't have to query again
    if (!mCaptureConfigured)
    {
        ret = setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *imgCaptureData);
        mPendingCaptureSettings &= ~SetFormat;
        CAMHAL_LOGDB("mPendingCaptureSettings = %d", (int)mPendingCaptureSettings);
    }
    else
    {
        //just set Pending size
        if(imgCaptureData->mPendingSize == true)
        {
            imgCaptureData->mPendingSize = false;
            imgCaptureData->mWidth = imgCaptureData->mPendingWidth;
            imgCaptureData->mHeight = imgCaptureData->mPendingHeight;
            imgCaptureData->mEncodeWidth = imgCaptureData->mPendingEncodeWidth;
            imgCaptureData->mEncodeHeight = imgCaptureData->mPendingEncodeHeight;
            imgCaptureData->mCropWidth = imgCaptureData->mPendingCropWidth;
            imgCaptureData->mCropHeight = imgCaptureData->mPendingCropHeight;
            imgCaptureData->mStride = imgCaptureData->mPendingStride;
            imgCaptureData->mBufSize = imgCaptureData->mPendingBufSize;
        }

    }

    //should after the mPendingSize updated
#ifdef CAMERA_IMAGE_SNAPSHOT
#ifdef CAMERA_VIDEO_SNAPSHOT
     /**
    * BUGFIX: fix for 5.1 cts testBurstVideoSnapshot fail . 
    *ActionsCode(author:liyuan, change_code)
    */
    if(((imgCaptureData->mWidth == previewData->mWidth && imgCaptureData->mHeight == previewData->mHeight)
       || (mAdapterState&VIDEO_ACTIVE) || mIternalRecordingHint)
#else
    if((imgCaptureData->mWidth == previewData->mWidth && imgCaptureData->mHeight == previewData->mHeight)
#endif

#ifndef CAMERA_ANDROID16
#ifdef CAMERA_HDR
     && !(mHdrSupport && (OMX_EXPOSURECONTROLTYPE)mParameters3A.SceneMode == (OMX_EXPOSURECONTROLTYPE)OMX_ExposureControlActHDR)

#endif
#endif
    )
    {
        //ActionsCode(author:liuyiguang, add_code, get capture frame from omx always for 900A)
#ifdef CAMERA_GS705A
        mCaptureUsePreviewFrame = true;
#elif defined CAMERA_GS900A
        mCaptureUsePreviewFrame = false;
#endif
        //asume color format is same
        //TODO color Format Maybe different
        //disable image port
        disableImagePort();
        if ( NULL != mReleaseImageBuffersCallback )
        {
            CAMHAL_LOGDA("mReleaseImageBuffersCallback called");
            mReleaseImageBuffersCallback(mReleaseData);
        }
        
        length = previewData->mBufSize;
    }

#endif

    if ( ret == NO_ERROR )
    {
        length = imgCaptureData->mBufSize;
    }
    else
    {
        CAMHAL_LOGEB("setFormat() failed 0x%x", ret);
        length = 0;
    }

exit:
    CAMHAL_LOGDB("getPictureBufferSize %d", length);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
 *
 * BUGFIX:  Fix for Singal Focus.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::startImageCapture()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * capData = NULL;
    OMX_CONFIG_PORTBOOLEANTYPE bOMX;
    OMX_CONFIG_CAPTUREMODETYPE captureMode;

    LOG_FUNCTION_NAME;
    CAMERA_SCOPEDTRACE("startImageCapture");

    if ((getNextState() & (CAPTURE_ACTIVE)) == 0)
    {
        CAMHAL_LOGDA("trying starting capture when already canceled");
        return NO_ERROR;
    }


    if(strcmp(mCapabilities->get(CameraProperties::FOCUS_SUPPORTED), "true") == 0)
    {
        /*set quick capture focus mode for taking picture except video snap shot*/
        //ActionsCode(author:liuyiguang, change_code)
        //if ((mParameters3A.Focus != OMX_IMAGE_FocusControlOff)&& !mIternalRecordingHint)
        //{
        //    int query_cnt = 0;
        //    CAMHAL_LOGDA("set quick capture mode");
        //    setFocusMode((OMX_IMAGE_FOCUSCONTROLTYPE)OMX_IMAGE_FocusControlQuickCapture);

        //    CameraHalEvent::FocusStatus focus_status;

        //    ret = getFocusStatus(focus_status);
        //    while ((focus_status == CameraHalEvent::FOCUS_STATUS_PENDING)
        //                &&(mParameters3A.Focus != OMX_IMAGE_FocusControlOff)) {
        //        query_cnt++;
        //        if (query_cnt > 300) {
        //            CAMHAL_LOGEA("wait for quick focus finish timeout!");
        //            break;
        //        }
        //        usleep(10*1000);    //sleep     10ms
        //        getFocusStatus(focus_status);
        //    }
        //}
    }

    // Camera framework doesn't expect face callbacks once capture is triggered
    //pauseFaceDetection(true);

    //make sure that jpeg encode not using this exifs
    freeExifObjects();

    //ActionsCode(author:liuyiguang, add_code, turn on flash light befor get image buffer!Only fix for 705A)
#ifdef CAMERA_GS705A
    if(needFlashStrobe(mParameters3A))
    {
        mFlashStrobed = true;
        mFlashConvergenceFrame = mFlashConvergenceFrameConfig;
        startFlashStrobe();
    }
#endif

#ifdef CAMERA_IMAGE_SNAPSHOT
    if(mCaptureUsePreviewFrame)
    {
        mCaptureUsePreviewFrameStarted =  true;


        return NO_ERROR;
    }
#endif
    if(!mCaptureConfigured)
    {
        ///Image capture was cancelled before we could start
        return NO_ERROR;
    }
    
    if ( 0 != mStartCaptureSem.Count() )
    {
        CAMHAL_LOGEB("Error mStartCaptureSem semaphore count %d", mStartCaptureSem.Count());
        return NO_INIT;
    }

#if 0
    if ( NO_ERROR == ret )
    {
        if (mPendingCaptureSettings & SetRotation)
        {
            mPendingCaptureSettings &= ~SetRotation;
            ret = setPictureRotation(mPictureRotation);
            if ( NO_ERROR != ret )
            {
                CAMHAL_LOGEB("Error configuring image rotation %x", ret);
            }
        }
    }
#endif


    if ( NO_ERROR == ret )
    {
        capData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

        ///Queue all the buffers on capture port
        for ( int index = 0 ; index < capData->mNumBufs ; index++ )
        {
            CAMERA_SCOPEDTRACE("OMX_FillThisBuffer-imageport");
            CAMHAL_LOGDB("Queuing buffer on Capture port header=0x%x, pBuffer= 0x%x",
                          ( unsigned int ) capData->mBufferHeader[index],( unsigned int ) capData->mBufferHeader[index]->pBuffer);
            eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                                        (OMX_BUFFERHEADERTYPE*)capData->mBufferHeader[index]);

            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }

        mCaptureSignalled = false;


        OMX_INIT_STRUCT_PTR (&captureMode, OMX_CONFIG_CAPTUREMODETYPE);
        captureMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
        captureMode.bFrameLimited = OMX_TRUE;
        captureMode.bContinuous = OMX_TRUE;
        captureMode.nFrameLimit = 1;

        /// sending Capture mode
        {
            CAMERA_SCOPEDTRACE("OMX_IndexConfigCaptureMode");
             
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                OMX_IndexConfigCaptureMode,
                &captureMode);
        }

        OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_PORTBOOLEANTYPE);
        bOMX.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
        bOMX.bEnabled = OMX_TRUE;

        /// sending Capturing Command to the component
        {
            CAMERA_SCOPEDTRACE("OMX_IndexConfigCommonPortCapturing");
            eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                (OMX_INDEXTYPE)OMX_IndexConfigCommonPortCapturing,
                &bOMX);
        }

        CAMHAL_LOGDB("Capture set - 0x%x", eError);

        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
    }


    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    mCaptureSignalled = false;
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::stopImageCapture()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_PORTBOOLEANTYPE bOMX;
    OMXCameraPortParameters *imgCaptureData = NULL;

    LOG_FUNCTION_NAME;
    CAMERA_SCOPEDTRACE("stopImageCapture");

#ifdef CAMERA_IMAGE_SNAPSHOT
    if(mCaptureUsePreviewFrame)
    {
        mCaptureUsePreviewFrame = false;
        mCaptureUsePreviewFrameStarted =  false;
        return NO_ERROR;
    }
#endif
    
    if (!mCaptureConfigured)
    {
        //Capture is not ongoing, return from here
        return NO_ERROR;
    }


#if 0
    // After capture, face detection should be disabled
    // and application needs to restart face detection
    stopFaceDetection();
#endif

    //Wait here for the capture to be done, in worst case timeout and proceed with cleanup
    mCaptureSem.WaitTimeout(OMX_CAPTURE_TIMEOUT);

    OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_PORTBOOLEANTYPE);
    bOMX.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
    bOMX.bEnabled = OMX_FALSE;
    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
    eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                           (OMX_INDEXTYPE)OMX_IndexConfigCommonPortCapturing,
                           &bOMX);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error during SetConfig- 0x%x", eError);
        ret = -1;
        goto EXIT;
    }
    CAMHAL_LOGDB("Capture set - 0x%x", eError);

    mCaptureSignalled = true; //set this to true if we exited because of timeout

    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    //Release image buffers
    if ( NULL != mReleaseImageBuffersCallback )
    {
        mReleaseImageBuffersCallback(mReleaseData);
    }

    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::disableImagePort()
{
    status_t ret = NO_ERROR;
    OMX_CONFIG_PORTBOOLEANTYPE bOMX;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters *imgCaptureData = NULL;
    if (!mCaptureConfigured)
    {
        return NO_ERROR;
    }

    CAMERA_SCOPEDTRACE("disableImagePort");


    mCaptureConfigured = false;
    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];



    OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_PORTBOOLEANTYPE);
    bOMX.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
    bOMX.bEnabled = OMX_FALSE;

    {
    CAMERA_SCOPEDTRACE("OMX_IndexConfigCommonPortCapturing-false");
    /// sending Capturing Command to the component
    eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
        (OMX_INDEXTYPE)OMX_IndexConfigCommonPortCapturing,
        &bOMX);
    }


    {
    CAMERA_SCOPEDTRACE("disableImagePort-inner");
        
    ///Register for Image port Disable event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mImagePortIndex,
                           mStopCaptureSem);


    {
        Mutex::Autolock lock(imgCaptureData->mLock);                
        imgCaptureData->mPortEnable = false;
    }
    ///Disable Capture Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortDisable,
                             mCameraAdapterParameters.mImagePortIndex,
                             NULL);
    ///Free all the buffers on capture port
    if (imgCaptureData)
    {
        for ( int index = 0 ; index < imgCaptureData->mNumBufs ; index++)
        {
            CAMERA_SCOPEDTRACE("OMX_FreeBuffer-imageport");
            eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                                    mCameraAdapterParameters.mImagePortIndex,
                                    (OMX_BUFFERHEADERTYPE*)imgCaptureData->mBufferHeader[index]);

            if(eError!=OMX_ErrorNone)
            {
                CAMHAL_LOGEB("OMX_FreeBuffer error=%d", eError);
            }
            GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
        }
    }

    //Wait for the image port enable event
    ret = mStopCaptureSem.WaitTimeout(OMX_CMD_TIMEOUT);
    }

    //should after port disable, otherwise onEmptyBufferDone will reference freed buffer
    clearMetadataBufs(*imgCaptureData);

    //If somethiing bad happened while we wait
    if (mComponentState == 0/*OMX_StateInvalid*/)
    {
        CAMHAL_LOGEA("Invalid State after Disable Image Port Exitting!!!");
        goto EXIT;
    }
    CAMHAL_LOGDB("Debug %s, %d", __func__,__LINE__);
    if ( NO_ERROR == ret )
    {
        CAMHAL_LOGDA("Port disabled");
    }
    else
    {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mImagePortIndex,
                           NULL);
        CAMHAL_LOGEA("Timeout expired on port disable");
        goto EXIT;
    }
EXIT:
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::UseBuffersCapture(void* bufArr, int num)
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * imgCaptureData = NULL;
    //ActionsCode(author:liuyiguang, change_code)
    //uint32_t *buffers = (uint32_t*)bufArr;
    long *buffers = (long *)bufArr;
    OMXCameraPortParameters cap;

    CAMERA_SCOPEDTRACE("UseBuffersCapture");

    imgCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];


#ifdef CAMERA_IMAGE_SNAPSHOT
    if(mCaptureUsePreviewFrame)
    {
        for ( int index = 0 ; index < imgCaptureData->mNumBufs ; index++ )
        { 
            imgCaptureData->mNumBufs = num;
            //ActionsCode(author:liuyiguang, change_code)
            //imgCaptureData->mHostBufaddr[index] = (OMX_U32)buffers[index];
            imgCaptureData->mHostBufaddr[index] = (long)buffers[index];
        }
        imgCaptureData->mCurIndex = 0;
        imgCaptureData->mBufferBitmap = (unsigned int)-1;
        mCapturedFrames = mBurstFrames;
        return NO_ERROR;
    }
#endif
    

    if ( 0 != mUseCaptureSem.Count() )
    {
        CAMHAL_LOGEB("Error mUseCaptureSem semaphore count %d", mUseCaptureSem.Count());
        return BAD_VALUE;
    }

    // capture is already configured...we can skip this step
    if (mCaptureConfigured)
    {

        if ( NO_ERROR == ret )
        {
            //ret = setupEXIF();
            if ( NO_ERROR != ret )
            {
                CAMHAL_LOGEB("Error configuring EXIF Buffer %x", ret);
            }
        }

        mCapturedFrames = mBurstFrames;
        return NO_ERROR;
    }

    imgCaptureData->mNumBufs = num;

    //TODO: Support more pixelformats

    CAMHAL_LOGDB("Params Width = %d", (int)imgCaptureData->mWidth);
    CAMHAL_LOGDB("Params Height = %d", (int)imgCaptureData->mHeight);

    if (mPendingCaptureSettings & SetFormat)
    {
        ret = setFormat(OMX_CAMERA_PORT_IMAGE_OUT_IMAGE, *imgCaptureData);
        if ( ret != NO_ERROR )
        {
            CAMHAL_LOGEB("setFormat() failed %d", ret);
            LOG_FUNCTION_NAME_EXIT;
            return ret;
        }
        mPendingCaptureSettings &= ~SetFormat;
    }


    StoreMetaDataInBuffersParams storeMetaDataParam;
    OMX_INIT_STRUCT_PTR (&storeMetaDataParam, StoreMetaDataInBuffersParams);

    storeMetaDataParam.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
    storeMetaDataParam.bStoreMetaData = OMX_TRUE;



    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                                  (OMX_INDEXTYPE)OMX_IndexParameterStoreMediaData, &storeMetaDataParam);
    if(eError!=OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);


    ///Register for Image port ENABLE event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mImagePortIndex,
                           mUseCaptureSem);

    ///Enable Capture Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortEnable,
                             mCameraAdapterParameters.mImagePortIndex,
                             NULL);

    CAMHAL_LOGDB("OMX_UseBuffer error = 0x%x", eError);
    GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);


    clearMetadataBufs(*imgCaptureData);

    video_metadata_t *metadata;
    OMX_BUFFERHEADERTYPE *pBufferHdr;

    {
        Mutex::Autolock lock(imgCaptureData->mLock);        
        imgCaptureData->mPortEnable = true;
    }

    for ( int index = 0 ; index < imgCaptureData->mNumBufs ; index++ )
    {
        
        CAMERA_SCOPEDTRACE("OMX_UseBuffer-imageport");
        //ActionsCode(author:liuyiguang, change_code)
        //CAMHAL_LOGDB("OMX_UseBuffer Capture address: 0x%x, size = %d",
        //             (unsigned int)buffers[index],
        //             (int)imgCaptureData->mBufSize);
        CAMHAL_LOGDB("OMX_UseBuffer Capture address: 0x%x, size = %d",
                     (long)buffers[index],
                     (int)imgCaptureData->mBufSize);

        metadata = (video_metadata_t *)malloc(sizeof(video_metadata_t));
        if(metadata == NULL)
        {
            CAMHAL_LOGDA("malloc metadata error");
            ret = NO_MEMORY;
            goto EXIT;
        }
        metadata->metadataBufferType = kMetadataBufferTypeCameraSource_act;
        metadata->handle = (void *) buffers[index];

        eError = OMX_UseBuffer(mCameraAdapterParameters.mHandleComp,
                               &pBufferHdr,
                               mCameraAdapterParameters.mImagePortIndex,
                               0,
                               sizeof(video_metadata_t),
                               //mCaptureBuffersLength,
                               (OMX_U8*)metadata);

        CAMHAL_LOGDB("OMX_UseBuffer = 0x%x", eError);
        GOTO_EXIT_IF(( eError != OMX_ErrorNone ), eError);

        CAMHAL_LOGDB("OMX_UseBuffer, pBufferHdr = 0x%p", pBufferHdr);
        CAMHAL_LOGDB("OMX_UseBuffer, pBufferHdr->pBuffer = 0x%p", metadata);
        //ActionsCode(author:liuyiguang, change_code)
        //pBufferHdr->pAppPrivate = (OMX_PTR) index;
        pBufferHdr->pAppPrivate = (OMX_PTR)(long) index;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion.s.nVersionMajor = SPECVERSIONMAJOR ;
        pBufferHdr->nVersion.s.nVersionMinor = SPECVERSIONMINOR ;
        pBufferHdr->nVersion.s.nRevision = SPECREVISION ;
        pBufferHdr->nVersion.s.nStep =  SPECSTEP;
        imgCaptureData->mBufferHeader[index] = pBufferHdr;
        //ActionsCode(author:liuyiguang, change_code)
        //imgCaptureData->mHostBufaddr[index] = (OMX_U32)buffers[index];
        //imgCaptureData->mMetadataBufs[index] = (OMX_U32)metadata;
        imgCaptureData->mHostBufaddr[index] = (long)buffers[index];
        imgCaptureData->mMetadataBufs[index] = (long)metadata;
    }
    imgCaptureData->mMetadataNum = imgCaptureData->mNumBufs;

    //Wait for the image port enable event
    CAMHAL_LOGDA("Waiting for port enable");
    ret = mUseCaptureSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == 0/*OMX_StateInvalid*/)
    {
        CAMHAL_LOGEA("Invalid State after Enable Image Port Exitting!!!");
        goto EXIT;
    }

    if ( ret == NO_ERROR )
    {
        CAMHAL_LOGDA("Port enabled");
    }
    else
    {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mImagePortIndex,
                           NULL);
        CAMHAL_LOGDA("Timeout expired on port enable");
        goto EXIT;
    }

    if ( NO_ERROR == ret )
    {
        //ret = setupEXIF();
        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("Error configuring EXIF Buffer %x", ret);
        }
    }

    mCapturedFrames = mBurstFrames;
    mCaptureConfigured = true;

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    //Release image buffers
    if ( NULL != mReleaseImageBuffersCallback )
    {
        mReleaseImageBuffersCallback(mReleaseData);
    }
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));

}

status_t OMXCameraAdapter::canSetCapture3A(bool &state)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_PORTBOOLEANTYPE bOMX;

    LOG_FUNCTION_NAME;

#ifdef CAMERA_IMAGE_SNAPSHOT
    if(mCaptureUsePreviewFrame && mCaptureUsePreviewFrameStarted)
    {
        state = true;
        goto exit;
    }    
#endif

    OMX_INIT_STRUCT_PTR (&bOMX, OMX_CONFIG_PORTBOOLEANTYPE);
    bOMX.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

    /// sending Capturing Command to the component
    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
        (OMX_INDEXTYPE)OMX_IndexConfigCommonPortCapturing,
        &bOMX);

    state = bOMX.bEnabled == OMX_TRUE ? true:false;

exit:
    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

}


};
