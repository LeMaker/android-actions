#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"  

namespace android
{

bool OMXCameraAdapter::needFlashStrobe(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    bool retval = false;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ACT_CONFIG_AGCVALUE gain;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return false;
    }

    if(!mFlashSupport)
    {
        return false;
    }

    if(( OMX_IMAGE_FLASHCONTROLTYPE ) Gen3A.FlashMode == OMX_IMAGE_FlashControlOn )
    {
        retval = true;
    }
    else  if(( OMX_IMAGE_FLASHCONTROLTYPE ) Gen3A.FlashMode == OMX_IMAGE_FlashControlTorch)
    {
        retval = true;
    }
    else if(( OMX_IMAGE_FLASHCONTROLTYPE ) Gen3A.FlashMode == OMX_IMAGE_FlashControlAuto)
    {
        //TODO
        OMX_INIT_STRUCT_PTR (&gain, OMX_ACT_CONFIG_AGCVALUE);
        gain.nPortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;

        eError =  OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
            (OMX_INDEXTYPE) OMX_ACT_IndexConfigAGCExposureValue,
            &gain);
        if ( OMX_ErrorNone != eError )
        {
            CAMHAL_LOGEB("Error while get gain value 0x%x", eError);
        }
        else
        {
            if(gain.nGain > 0)
            {
                retval = true;
            }
            CAMHAL_LOGDB("get gain value successfully %d", gain.nGain);
        }

    }
    else
    {
        retval  = false;
    }

    LOG_FUNCTION_NAME_EXIT;
    return retval;
}

status_t OMXCameraAdapter::startFlashStrobe()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ACT_CONFIG_FlashStrobeParams strobe;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }
    if(!mFlashSupport)
    {
        return NO_ERROR;
    }

    OMX_INIT_STRUCT_PTR (&strobe, OMX_ACT_CONFIG_FlashStrobeParams);
    strobe.nPortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;

    strobe.bStrobeOn = OMX_TRUE;
    strobe.nElapsedTime = 0;

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_ACT_IndexConfigFlashStrobeValue,
                            &strobe);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring strobe 0x%x", eError);
    }
    else
    {
        CAMHAL_LOGDA("Camera strobe configured successfully");
    }

    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);
}
status_t OMXCameraAdapter::stopFlashStrobe()
    
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ACT_CONFIG_FlashStrobeParams strobe;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }
    if(!mFlashSupport)
    {
        return NO_ERROR;
    }

    OMX_INIT_STRUCT_PTR (&strobe, OMX_ACT_CONFIG_FlashStrobeParams);
    strobe.nPortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;

    strobe.bStrobeOn = OMX_FALSE;
    strobe.nElapsedTime = 0;

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_ACT_IndexConfigFlashStrobeValue,
                            &strobe);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring strobe 0x%x", eError);
    }
    else
    {
        CAMHAL_LOGDA("Camera strobe configured successfully");
    }

    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);
}

};

