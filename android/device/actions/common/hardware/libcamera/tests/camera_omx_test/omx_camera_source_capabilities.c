#include "omx_camera_source_capabilities.h"


#define ID_TO_ARRAY_NAME(a,id)   (a##_##id)
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0])) 

#define SET_ARRAY_CAPS(count,name,array) do{ \
    caps->count = ARRAY_SIZE(array);         \
    if(caps->count>0){                                         \
        memcpy((char *)&(caps->name), (char *)&array, sizeof(array));   \
    }                                                                                                                  \
}while(0);

OMX_COLOR_FORMATTYPE    ID_TO_ARRAY_NAME(previewFormatsSupported,0)[]={
    OMX_COLOR_Format16bitRGB565,
    OMX_COLOR_FormatYUV420SemiPlanar,
    OMX_COLOR_FormatYUV420Planar
};

OMX_COLOR_FORMATTYPE    ID_TO_ARRAY_NAME(imageFormatsSupported,0)[]={
    OMX_COLOR_Format16bitRGB565,
    OMX_COLOR_FormatYUV420SemiPlanar,
    OMX_COLOR_FormatYUV420Planar
};

OMX_ACT_VARRESTYPE      ID_TO_ARRAY_NAME(previewResSupported,0)[]={
    {800,600,30},
    {1280,720,30},
    /*
    {640,480,30},
    {800,600,30},
    */
};

OMX_ACT_VARRESTYPE      ID_TO_ARRAY_NAME(imageResSupported,0)[]={
    {800,600,30},
    {1280,720,30},
    /*
    {640,480,30},
    {800,600,30},
    {1280,720,30},
    */
};
OMX_ACT_VARRESTYPE      ID_TO_ARRAY_NAME(thumbResSupported,0)[]={
    {64,64,30},
    {128,128,30},
};

OMX_WHITEBALCONTROLTYPE ID_TO_ARRAY_NAME(whiteBalanceModesSupported,0)[]={
    OMX_WhiteBalControlAuto,
    OMX_WhiteBalControlSunLight,
    OMX_WhiteBalControlCloudy,
    OMX_WhiteBalControlFluorescent,
    OMX_WhiteBalControlIncandescent,
};

OMX_IMAGEFILTERTYPE     ID_TO_ARRAY_NAME(colorEffectsSupported,0)[]={
    OMX_ImageFilterNone
};

OMX_FLICKERREJECTIONTYPE     ID_TO_ARRAY_NAME(flickerSupported,0)[]={
    OMX_FlickerRejectionOff,
    OMX_FlickerRejectionAuto,
    OMX_FlickerRejection50,
    OMX_FlickerRejection60
};

OMX_EXPOSURECONTROLTYPE ID_TO_ARRAY_NAME(exposureModesSupported,0)[]={
    OMX_ExposureControlAuto,
    OMX_ExposureControlNight,
    OMX_ExposureControlBeach,
    OMX_ExposureControlSnow,
    OMX_ExposureControlSports,
};

OMX_IMAGE_FOCUSCONTROLTYPE      ID_TO_ARRAY_NAME(focusModesSupported,0)[]={
    OMX_IMAGE_FocusControlOff
};

OMX_IMAGE_FLASHCONTROLTYPE      ID_TO_ARRAY_NAME(flashModesSupported,0)[]={
    OMX_IMAGE_FlashControlOn,
    OMX_IMAGE_FlashControlOff,
    OMX_IMAGE_FlashControlAuto
};

OMX_ACT_VARFPSTYPE      ID_TO_ARRAY_NAME(prvVarFPSModesSupported,0)[]={
    {5, 5},
    {15, 15},
    {20, 20},
    {25, 25},
    {30, 30},
};

OMX_ACT_VARFPSTYPE      ID_TO_ARRAY_NAME(capVarFPSModesSupported,0)[]={
    {5, 5},
    {15, 15},
    {20, 20},
    {25, 25},
    {30, 30},
};


/****************************************************************************/
OMX_COLOR_FORMATTYPE    ID_TO_ARRAY_NAME(previewFormatsSupported,1)[]={
    OMX_COLOR_Format16bitRGB565,
    OMX_COLOR_FormatYUV420SemiPlanar,
    OMX_COLOR_FormatYUV420Planar
};

OMX_COLOR_FORMATTYPE    ID_TO_ARRAY_NAME(imageFormatsSupported,1)[]={
    OMX_COLOR_Format16bitRGB565,
    OMX_COLOR_FormatYUV420SemiPlanar,
    OMX_COLOR_FormatYUV420Planar
};

OMX_ACT_VARRESTYPE      ID_TO_ARRAY_NAME(previewResSupported,1)[]={
    {320,240,30},
    {640,480,30},
};

OMX_ACT_VARRESTYPE      ID_TO_ARRAY_NAME(imageResSupported,1)[]={
    {320,240,30},
    {640,480,30},
    /*
    {640,480,30},
    */
};
OMX_ACT_VARRESTYPE      ID_TO_ARRAY_NAME(thumbResSupported,1)[]={
    {64,64,30},
    {128,128,30},
};

OMX_WHITEBALCONTROLTYPE ID_TO_ARRAY_NAME(whiteBalanceModesSupported,1)[]={
    OMX_WhiteBalControlAuto,
    OMX_WhiteBalControlSunLight,
    OMX_WhiteBalControlCloudy,
    OMX_WhiteBalControlFluorescent,
    OMX_WhiteBalControlIncandescent,
};

OMX_IMAGEFILTERTYPE     ID_TO_ARRAY_NAME(colorEffectsSupported,1)[]={
    OMX_ImageFilterNone
};

OMX_FLICKERREJECTIONTYPE     ID_TO_ARRAY_NAME(flickerSupported,1)[]={
    OMX_FlickerRejectionOff,
    OMX_FlickerRejectionAuto,
    OMX_FlickerRejection50,
    OMX_FlickerRejection60
};

OMX_EXPOSURECONTROLTYPE ID_TO_ARRAY_NAME(exposureModesSupported,1)[]={
    OMX_ExposureControlAuto,
    OMX_ExposureControlNight,
    OMX_ExposureControlBeach,
    OMX_ExposureControlSnow,
    OMX_ExposureControlSports,
};

OMX_IMAGE_FOCUSCONTROLTYPE      ID_TO_ARRAY_NAME(focusModesSupported,1)[]={
    OMX_IMAGE_FocusControlAuto
};

OMX_IMAGE_FLASHCONTROLTYPE      ID_TO_ARRAY_NAME(flashModesSupported,1)[]={
    OMX_IMAGE_FlashControlOn,
    OMX_IMAGE_FlashControlOff,
    OMX_IMAGE_FlashControlAuto
};

OMX_ACT_VARFPSTYPE      ID_TO_ARRAY_NAME(prvVarFPSModesSupported,1)[]={
    {5, 5},
    {15, 15},
    {20, 20},
    {25, 25},
    {30, 30},
};

OMX_ACT_VARFPSTYPE      ID_TO_ARRAY_NAME(capVarFPSModesSupported,1)[]={
    {5, 5},
    {15, 15},
    {20, 20},
    {25, 25},
    {30, 30},
};



OMX_ERRORTYPE getCamCapabilities(OMX_ACT_CAPTYPE *caps, OMX_U32 id)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    memset(caps, 0, sizeof(OMX_ERRORTYPE));
    setHeader(caps, sizeof(OMX_ERRORTYPE));
    switch(id)
    {
        case 0:
        {
            SET_ARRAY_CAPS(ulPreviewFormatCount, ePreviewFormats, ID_TO_ARRAY_NAME(previewFormatsSupported,0));

            SET_ARRAY_CAPS(ulImageFormatCount, eImageFormats, ID_TO_ARRAY_NAME(imageFormatsSupported,0));

            SET_ARRAY_CAPS(ulPreviewResCount, tPreviewRes, ID_TO_ARRAY_NAME(previewResSupported,0));
            
            SET_ARRAY_CAPS(ulImageResCount, tImageRes, ID_TO_ARRAY_NAME(imageResSupported,0));
           
            SET_ARRAY_CAPS(ulThumbResCount, tThumbRes, ID_TO_ARRAY_NAME(thumbResSupported,0));

            SET_ARRAY_CAPS(ulWhiteBalanceCount, eWhiteBalanceModes, ID_TO_ARRAY_NAME(whiteBalanceModesSupported,0));
           
            SET_ARRAY_CAPS(ulColorEffectCount, eColorEffects, ID_TO_ARRAY_NAME(colorEffectsSupported,0));
            
            caps-> xMaxWidthZoom = 0;
            caps-> xMaxHeightZoom = 0;

            SET_ARRAY_CAPS(ulFlickerCount, eFlicker, ID_TO_ARRAY_NAME(flickerSupported,0));

            SET_ARRAY_CAPS(ulExposureModeCount, eExposureModes, ID_TO_ARRAY_NAME(exposureModesSupported,0));

            caps->bISONoiseFilterSupported = OMX_TRUE;
            caps->nSensitivityMax = 1600;
            

            caps->xEVCompensationMin = 0;
            caps->xEVCompensationMax = 0;

            caps->bBrightnessSupported = OMX_TRUE;
            caps->bContrastSupported = OMX_TRUE;
            caps->bSaturationSupported = OMX_TRUE;

            caps->bExposureLockSupported = OMX_FALSE;
            caps->bFocusLockSupported = OMX_FALSE;
            caps->bWhiteBalanceLockSupported = OMX_FALSE;

            setHeader(&caps->xBrightnessLevel, sizeof(OMX_ACT_SUPPORT_LEVEL));
            setHeader(&caps->xContrastLevel, sizeof(OMX_ACT_SUPPORT_LEVEL));
            setHeader(&caps->xSaturationLevel, sizeof(OMX_ACT_SUPPORT_LEVEL));
            setHeader(&caps->xDeNoiseLevel, sizeof(OMX_ACT_SUPPORT_LEVEL));
            
            

            SET_ARRAY_CAPS(ulFocusModeCount, eFocusModes, ID_TO_ARRAY_NAME(focusModesSupported,0));

            SET_ARRAY_CAPS(ulFlashCount, eFlashModes, ID_TO_ARRAY_NAME(flashModesSupported,0));


            SET_ARRAY_CAPS(ulPrvVarFPSModesCount, tPrvVarFPSModes, ID_TO_ARRAY_NAME(prvVarFPSModesSupported,0));
            SET_ARRAY_CAPS(ulCapVarFPSModesCount, tCapVarFPSModes, ID_TO_ARRAY_NAME(capVarFPSModesSupported,0));

            caps->ulAreasFocusCount = 0;
        }
        break;

        case 1:
        {
            SET_ARRAY_CAPS(ulPreviewFormatCount, ePreviewFormats, ID_TO_ARRAY_NAME(previewFormatsSupported,1));

            SET_ARRAY_CAPS(ulImageFormatCount, eImageFormats, ID_TO_ARRAY_NAME(imageFormatsSupported,1));

            SET_ARRAY_CAPS(ulPreviewResCount, tPreviewRes, ID_TO_ARRAY_NAME(previewResSupported,1));
            
            SET_ARRAY_CAPS(ulImageResCount, tImageRes, ID_TO_ARRAY_NAME(imageResSupported,1));
           
            SET_ARRAY_CAPS(ulThumbResCount, tThumbRes, ID_TO_ARRAY_NAME(thumbResSupported,1));

            SET_ARRAY_CAPS(ulWhiteBalanceCount, eWhiteBalanceModes, ID_TO_ARRAY_NAME(whiteBalanceModesSupported,1));
           
            SET_ARRAY_CAPS(ulColorEffectCount, eColorEffects, ID_TO_ARRAY_NAME(colorEffectsSupported,1));
            
            caps-> xMaxWidthZoom = 0;
            caps-> xMaxHeightZoom = 0;

            SET_ARRAY_CAPS(ulFlickerCount, eFlicker, ID_TO_ARRAY_NAME(flickerSupported,1));

            SET_ARRAY_CAPS(ulExposureModeCount, eExposureModes, ID_TO_ARRAY_NAME(exposureModesSupported,1));

            caps->bISONoiseFilterSupported = OMX_TRUE;
            caps->nSensitivityMax = 800;
            

            caps->xEVCompensationMin = 0;
            caps->xEVCompensationMax = 0;

            caps->bBrightnessSupported = OMX_TRUE;
            caps->bContrastSupported = OMX_TRUE;
            caps->bSaturationSupported = OMX_TRUE;

            caps->bExposureLockSupported = OMX_FALSE;
            caps->bFocusLockSupported = OMX_FALSE;
            caps->bWhiteBalanceLockSupported = OMX_FALSE;

            setHeader(&caps->xBrightnessLevel, sizeof(OMX_ACT_SUPPORT_LEVEL));
            setHeader(&caps->xContrastLevel, sizeof(OMX_ACT_SUPPORT_LEVEL));
            setHeader(&caps->xSaturationLevel, sizeof(OMX_ACT_SUPPORT_LEVEL));
            setHeader(&caps->xDeNoiseLevel, sizeof(OMX_ACT_SUPPORT_LEVEL));
            

            SET_ARRAY_CAPS(ulFocusModeCount, eFocusModes, ID_TO_ARRAY_NAME(focusModesSupported,1));

            SET_ARRAY_CAPS(ulFlashCount, eFlashModes, ID_TO_ARRAY_NAME(flashModesSupported,1));


            SET_ARRAY_CAPS(ulPrvVarFPSModesCount, tPrvVarFPSModes, ID_TO_ARRAY_NAME(prvVarFPSModesSupported,1));
            SET_ARRAY_CAPS(ulCapVarFPSModesCount, tCapVarFPSModes, ID_TO_ARRAY_NAME(capVarFPSModesSupported,1));

            caps->ulAreasFocusCount = 0;
        }
        break;

    default:
        err = OMX_ErrorNoMore;
        break;
    }
    return err;
}
