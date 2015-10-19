#ifndef OMX_ACT_IVCommon_H
#define OMX_ACT_IVCommon_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#ifndef _OPENMAX_V1_2_
#include "ACT_OMX_Common_V1_2__V1_1.h"
#endif
#include <OMX_Types.h>
#include <OMX_IVCommon.h>
#include <OMX_Image.h>


/**
 *processing level type
 *  Simultaneously lock focus, white balance and exposure (and relevant other settings).
 *
 * STRUCT MEMBERS:
 *  nSize        : Size of the structure in bytes
 *  nVersion     : OMX specification version information
 *  nPortIndex   : Port that this structure applies to
 *  nLevel :
 *               nLevel hinting processing amount. Range of values is -100 to 100.
 *               0 causes no change to the image.  Increased values cause increased processing to occur, with 100 applying maximum processing.
 *               Negative values have the opposite effect of positive values.
 *  bAuto:
 *		sets if the processing should be applied according to input data.
 		It is allowed to combine the hint level with the auto setting,
 *		i.e. to give a bias to the automatic setting. When set to false, the processing should not take input data into account.
 */

typedef struct OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE {
		OMX_U32 nSize;
		OMX_VERSIONTYPE nVersion;
		OMX_U32 nPortIndex;
		OMX_S32 nLevel;
		OMX_BOOL bAuto;
} OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE;



/**
 * Structure used to configure current OMX_ACT_CONFIG_SHAREDBUFFER
 *
 * STRUCT MEMBERS:
 * nSize            : Size of the structure in bytes
 * nVersion         : OMX specification version information
 * nPortIndex       : Port that this structure applies to
 * nSharedBuffSize  : Size of the pSharedBuff in bytes
 * pSharedBuff      : Pointer to a buffer
 */
typedef struct OMX_ACT_CONFIG_SHAREDBUFFER {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 nSharedBuffSize;
	OMX_U8* pSharedBuff;
} OMX_ACT_CONFIG_SHAREDBUFFER;

/**
 * Structure used to configure current OMX_TI_VARFPSTYPE
 *
 * @param nVarFPSMin    Number of the smallest FPS supported.
 * @param nVarFPSMax    Number of the biggest FPS supported, be equal to nVarFPSMin
 */
typedef struct OMX_ACT_VARFPSTYPE {
    OMX_U32                 nVarFPSMin;
    OMX_U32                 nVarFPSMax;
} OMX_ACT_VARFPSTYPE;

/**
 * Structure used to configure current OMX_TI_VARFPSTYPE
 *
 * @param nWidth    RES Width.
 * @param nWidth    RES Height.
 */
typedef struct OMX_ACT_VARRESTYPE {
    OMX_U32                 nWidth;
    OMX_U32                 nHeight;
    OMX_U32					nMaxFps;
} OMX_ACT_VARRESTYPE;
/**
 * sensor select  types
 */
typedef enum OMX_SENSORSELECT{
	OMX_PrimarySensor = 0,
	OMX_SecondarySensor,
	OMX_TI_StereoSensor,
	OMX_SensorTypeMax = 0x7fffffff
}OMX_SENSORSELECT;

/**
 *
 * Sensor Select
 */
typedef  struct OMX_PARAM_SENSORSELECTTYPE {
	OMX_U32  nSize; /**< Size of the structure in bytes */
	OMX_VERSIONTYPE nVersion; /**< OMX specification version info */
	OMX_U32 nPortIndex; /**< Port that this struct applies to */
	OMX_SENSORSELECT eSensor; /**< sensor select */
} OMX_PARAM_SENSORSELECTTYPE;

/**
 * Structure used to configure current OMX_TI_CAPRESTYPE
 *
 * STRUCT MEMBERS:
 * nSize            : Size of the structure in bytes
 * nVersion         : OMX specification version information
 * nPortIndex       : Port that this structure applies to
 * nWidthMin        : Number of the smallest width supported
 * nHeightMin       : Number of the smallest height supported
 * nWidthMax        : Number of the biggest width supported
 * nHeightMax       : Number of the biggest height supported
 */
typedef struct OMX_ACT_CAPRESTYPE {
	OMX_U32         nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32         nPortIndex;
	OMX_U32         nWidthMin;  // smallest width supported
	OMX_U32         nHeightMin; // smallest height supported
	OMX_U32         nWidthMax;  // biggest width supported
	OMX_U32         nHeightMax; // biggest height supported
} OMX_ACT_CAPRESTYPE;

typedef struct OMX_ACT_SUPPORT_LEVEL{
	OMX_U32         nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32         nPortIndex;
	OMX_S32         nMinVal;  // smallest width supported
	OMX_S32         nMaxVal; // smallest height supported
	OMX_U32         nStep;
}OMX_ACT_SUPPORT_LEVEL;
/**
 * Structure used to configure current OMX_ACT_CAPTYPE
 *
 * STRUCT MEMBERS:
 * nSize                                : Size of the structure in bytes
 * nVersion                             : OMX specification version information
 * nPortIndex                           : Port that this structure applies to
 * ulPreviewFormatCount                 : Number of the supported preview pixelformat count
 * ePreviewFormats                      : Array containing the supported preview pixelformat count
 * ulImageFormatCount                   : Number of the supported image pixelformat count
 * eImageFormats                        : Array containing the supported image pixelformat count
 * tPreviewResRange                     : Supported preview resolution range
 * tImageResRange                       : Supported image resolution range
 * tThumbResRange                       : Supported thumbnail resolution range
 * ulWhiteBalanceCount                  : Supported whitebalance mode count
 * eWhiteBalanceModes                   : Array containing the whitebalance modes
 * ulColorEffectCount                   : Supported effects count
 * eColorEffects                        : Array containing the supported effects
 * xMaxWidthZoom                        : Fixed point value stored as Q16 representing the maximum value for the Zoom allowed on Width
 * xMaxHeightZoom                       : Fixed point value stored as Q16 representing the maximum value for the Zoom allowed on Height
 * ulFlickerCount                       : Number of the supported anti-flicker modes
 * eFlicker                             : Array containing the supported anti-flicker modes
 * ulExposureModeCount                  : Number of the supported exposure modes
 * eExposureModes                       : Array containing the supported exposure modes
 * bLensDistortionCorrectionSupported   : Flag for Lens Distortion Correction Algorithm support
 * bISONoiseFilterSupported             : Flag for Noise Filter Algorithm support
 * xEVCompensationMin                   : Fixed point value stored as Q16 representing the EVCompensation minumum allowed value
 * xEVCompensationMax                   : Fixed point value stored as Q16 representing the EVCompensation maximum allowed value
 * nSensitivityMax                      : nSensitivityMax = 100 implies maximum supported equal to "ISO 100"
 * ulFocusModeCount                     : Number of the supported focus modes
 * eFocusModes                          : Array containing the supported focus modes
 * ulFlashCount                         : Number of the supported flash modes
 * eFlashModes                          : Array containing the supported flash modes
 * bContrastSupported                   : Flag showing if the contrast is supported
 * bSaturationSupported                 : Flag showing if the saturation is supported
 * bBrightnessSupported                 : Flag showing if the brightness is supported
 * bProcessingLevelSupported            : Flag showing if the processing level is supported
 * ulPrvVarFPSModesCount                : Number of preview FPS modes
 * tPrvVarFPSModes                      : Preview FPS modes
 * ulCapVarFPSModesCount                : Number of capture FPS modes
 * tCapVarFPSModes                      : Capture FPS modes
 */
typedef struct OMX_ACT_CAPTYPE {
	OMX_U32                 nSize;
	OMX_VERSIONTYPE         nVersion;
	OMX_U32                 nSensorIndex;
	OMX_U16                 ulPreviewFormatCount;   // supported preview pixelformat count
	OMX_COLOR_FORMATTYPE    ePreviewFormats[8];
	OMX_U16                 ulImageFormatCount;     // supported image pixelformat count
	OMX_COLOR_FORMATTYPE    eImageFormats[8];
	OMX_ACT_VARRESTYPE      tPreviewRes[8];       // supported preview resolution range
	OMX_U16                 ulPreviewResCount;    // supported preview resolution count
	OMX_ACT_VARRESTYPE      tImageRes[64];         // supported image resolution range
	OMX_U16                 ulImageResCount;    // supported image resolutio count
	OMX_ACT_VARRESTYPE      tThumbRes[4];         // supported thumbnail resolution range
	OMX_U16                 ulThumbResCount;    // supported thumbnail resolution count
	OMX_U16                 ulWhiteBalanceCount;    // supported whitebalance mode count
	OMX_WHITEBALCONTROLTYPE eWhiteBalanceModes[16];
	OMX_U16                 ulColorEffectCount;     // supported effects count
	OMX_IMAGEFILTERTYPE     eColorEffects[32];
	OMX_S32                 xMaxWidthZoom;          // Fixed point value stored as Q16
	OMX_S32                 xMaxHeightZoom;         // Fixed point value stored as Q16
	OMX_U16                 ulFlickerCount;         // supported anti-flicker mode count
	OMX_FLICKERREJECTIONTYPE     eFlicker[8];
	OMX_U16                 ulExposureModeCount;    // supported exposure mode count
	OMX_EXPOSURECONTROLTYPE eExposureModes[32];
	OMX_BOOL                bISONoiseFilterSupported;
	OMX_S32                 xEVCompensationMin;     // Fixed point value stored as Q16
	OMX_S32                 xEVCompensationMax;     // Fixed point value stored as Q16
	OMX_U32                 nSensitivityMax;        // nSensitivityMax = 100 implies maximum supported equal to "ISO 100"
	OMX_U16                 ulFocusModeCount;       // supported focus mode count
	OMX_IMAGE_FOCUSCONTROLTYPE      eFocusModes[16];
	OMX_U16                 ulFlashCount;           // supported flash modes count
	OMX_IMAGE_FLASHCONTROLTYPE      eFlashModes[8];
	OMX_BOOL                bContrastSupported;
	OMX_BOOL                bSaturationSupported;
	OMX_BOOL                bBrightnessSupported;
	OMX_BOOL                bWhiteBalanceLockSupported;
	OMX_BOOL                bExposureLockSupported;
	OMX_BOOL                bFocusLockSupported;
	OMX_ACT_SUPPORT_LEVEL   xBrightnessLevel;//should be [-n,n] as [ -6:1:6],[0 6] used by OMX_CONFIG_BRIGHTNESSTYPE,[-6<<16,6<<16] used as EV
	OMX_ACT_SUPPORT_LEVEL   xDeNoiseLevel;//should be [0,n] as [0:1:6]
	OMX_ACT_SUPPORT_LEVEL   xSaturationLevel;//should be [-n,n] as [ -6:1:6]
	OMX_ACT_SUPPORT_LEVEL   xContrastLevel;//should be [-n,n] as [ -6:1:6]
	OMX_U16                 ulPrvVarFPSModesCount;  // supported variable FPS preview modes count
	OMX_ACT_VARFPSTYPE      tPrvVarFPSModes[10];
	OMX_U16                 ulCapVarFPSModesCount;  // supported variable FPS capture modes count
	OMX_ACT_VARFPSTYPE      tCapVarFPSModes[10];
	OMX_U16                 ulAreasFocusCount;    // supported number of AlgoAreas for focus areas
 } OMX_ACT_CAPTYPE;



/**
 * Defines 3A Region priority mode.
 *
 * STRUCT MEMBERS:
 *  nSize               : Size of the structure in bytes
 *  nVersion            : OMX specification version information
 *  nPortIndex          : Port that this structure applies to
 *  bAwbFaceEnable      : Enable Region priority for Auto White Balance
 *  bAeFaceEnable       : Enable Region priority for Auto Exposure
 *  bAfFaceEnable       : Enable Region priority for Auto Focus
 */
typedef struct OMX_ACT_CONFIG_3A_REGION_PRIORITY {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_BOOL bAwbRegionEnable;
	OMX_BOOL bAeRegionEnable;
	OMX_BOOL bAfRegionEnable;
	OMX_CONFIG_RECTTYPE tRect;
} OMX_ACT_CONFIG_3A_REGION_PRIORITY;


/**
 * The OMX_ACT_BRIGHTNESSCONTRASTCRTLTYPE enumeration is used to define the
 * brightness and contrast mode types.
 */
typedef enum OMX_ACT_BRIGHTNESSCONTRASTCRTLTYPE {
	OMX_ACT_BceModeOff = 0,
	OMX_ACT_BceModeOn,
	OMX_ACT_BceModeAuto,
	OMX_ACT_BceModeMax = 0x7FFFFFFF
} OMX_ACT_BRIGHTNESSCONTRASTCRTLTYPE;

/**
 * Local and global brightness contrast type.
 *
 * STRUCT MEMBERS:
 *  nSize             : Size of the structure in bytes
 *  nVersion          : OMX specification version information
 *  nPortIndex        : Port that this structure applies to
 *  eControl          : Control field for GLBCE
 */
typedef struct OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_ACT_BRIGHTNESSCONTRASTCRTLTYPE eControl;
} OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE;


/**
 * Focus distance configuration
 *
 *  STRUCT MEMBERS:
 *  nSize: Size of the structure in bytes
 *  nVersion: OMX specification version information
 *  nPortIndex: Port that this structure applies to
 *  nFocusDistanceNear : Specifies the near focus distance in mm ( 0 equals infinity )
 *  nFocusDistanceOptimal : Specifies the optimal focus distance in mm ( 0 equals infinity )
 *  nFocusDistanceFar : Specifies the far focus distance in mm ( 0 equals infinity )
 *  nLensPosition : Specifies the current lens position in driver units
 */
typedef struct OMX_ACT_CONFIG_FOCUSDISTANCETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFocusDistanceNear;
    OMX_U32 nFocusDistanceOptimal;
    OMX_U32 nFocusDistanceFar;
    OMX_S32 nLensPosition;
} OMX_ACT_CONFIG_FOCUSDISTANCETYPE;

/**
 * Function is enabled?
 *
 * */
typedef struct StoreMetaDataInBuffersParams {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bStoreMetaData;
}StoreMetaDataInBuffersParams;

/**
 * Function is enabled?
 *
 * */
typedef struct OMX_ACTIONS_Params {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
}OMX_ACTIONS_Params;

/**
 * The information of a face from camera face detection.
 */
typedef struct omx_camera_face {
    /**
     * Bounds of the face [left, top, right, bottom]. (-1000, -1000) represents
     * the top-left of the camera field of view, and (1000, 1000) represents the
     * bottom-right of the field of view. The width and height cannot be 0 or
     * negative. This is supported by both hardware and software face detection.
     *
     * The direction is relative to the sensor orientation, that is, what the
     * sensor sees. The direction is not affected by the rotation or mirroring
     * of CAMERA_CMD_SET_DISPLAY_ORIENTATION.
     */
	OMX_S32 rect[4];

    /**
     * The confidence level of the face. The range is 1 to 100. 100 is the
     * highest confidence. This is supported by both hardware and software
     * face detection.
     */
	OMX_S32 score;

    /**
     * An unique id per face while the face is visible to the tracker. If
     * the face leaves the field-of-view and comes back, it will get a new
     * id. If the value is 0, id is not supported.
     */
	OMX_S32 id;

    /**
     * The coordinates of the center of the left eye. The range is -1000 to
     * 1000. -2000, -2000 if this is not supported.
     */
	OMX_S32 left_eye[2];

    /**
     * The coordinates of the center of the right eye. The range is -1000 to
     * 1000. -2000, -2000 if this is not supported.
     */
	OMX_S32 right_eye[2];

    /**
     * The coordinates of the center of the mouth. The range is -1000 to 1000.
     * -2000, -2000 if this is not supported.
     */
	OMX_S32 mouth[2];

} omx_camera_face_t;

/**
 * The metadata of the frame data.
 */
typedef struct omx_camera_frame_metadata {
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
    /**
     * The number of detected faces in the frame.
     */
	OMX_S32 number_of_faces;

    /**
     * An array of the detected faces. The length is number_of_faces.
     */
    omx_camera_face_t *faces;
} omx_camera_frame_metadata_t;


//*
typedef enum OMX_TSPACKET_TYPE {
	OMX_TsPacket_Disable,
	OMX_TsPacket_NoBlu,
	OMX_TsPacket_WithBlu,
} OMX_TSPACKET_TYPE;

//*
typedef struct OMX_ACT_PARAM_TsPacketType{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_TSPACKET_TYPE TsPacketType;
}OMX_ACT_PARAM_TsPacketType;

//*
typedef struct OMX_ACT_PARAM_FaceDetType{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 nAngle;
	OMX_BOOL isFrontCamera;
}OMX_ACT_PARAM_FaceDetType;

typedef struct OMX_ACT_BlendImageType{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_COLOR_FORMATTYPE eColorFormat;
}OMX_ACT_BlendImageType;


typedef struct OMX_ACT_PARAM_EXIFPARAM{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_BOOL bExifEnable;				
	OMX_U32 ImageOri;// 图像的方向，默认为0
	OMX_BOOL bGPS;// GPS 信息是否含有，-1表示无GPS信息
	char *dataTime;// 拍照日期
	char *exifmake;// make and model
	char *exifmodel;
	OMX_U32  focalLengthL;// 焦距
	OMX_U32  focalLengthH;
	OMX_U32  gpsLATL[3];// 纬度与径度
	OMX_U32  gpsLATH[3];
	OMX_U32  gpsLONGL[3];
	OMX_U32  gpsLONGH[3];
	char *gpsprocessMethod;
	OMX_U32  gpsTimeL[3];
	OMX_U32  gpsTimeH[3];
	char *gpsDate;
	void *extendPtr;
	OMX_U32  gpsALTIL[1];
	OMX_U32  gpsALTIH[1];
	OMX_U32  gpsLATREF;//N:0 S:1
	OMX_U32  gpsLONGREF;//E:0 W:1
	OMX_U32  gpsALTIREF;//Sea level:0
}OMX_ACT_PARAM_EXIFPARAM;
				
typedef struct OMX_ACT_PARAM_THUMBPARAM{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 nWidth;
	OMX_U32 nHeight;
	OMX_BOOL bThumbEnable;
}OMX_ACT_PARAM_THUMBPARAM;
				
typedef struct OMX_ACT_CONFIG_VIDEOPARAM{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_U32 nWidth;
	OMX_U32 nHeight;
	OMX_U32 nFramerate;
}OMX_ACT_CONFIG_VIDEOPARAM;


/////add for extern enum
typedef enum OMX_ACT_IMAGEFILTERTYPE {
	OMX_ImageFilterACTBW = 0x7F000001,
	OMX_ImageFilterACTSEPIA,
	OMX_ImageFilterACTSKY_BLUE,
	OMX_ImageFilterACTGRASS_GREEN,
	OMX_ImageFilterACTREDDISH,
	OMX_ImageFilterACTSKIN_WHITEN,
	OMX_ImageFilterACTVIVID,
}OMX_ACT_IMAGEFILTERTYPE;

typedef enum OMX_ACT_EXPOSURECONTROLTYPE {
	OMX_ExposureControlActHouse = 0x7F000001,
	OMX_ExposureControlActSunset,
	OMX_ExposureControlActAction,
	OMX_ExposureControlActPortrait,
	OMX_ExposureControlActLandscape,
	OMX_ExposureControlActNight_Portrait,
	OMX_ExposureControlActTheatre,
	OMX_ExposureControlActStreadyPhoto,
	OMX_ExposureControlActFireworks,
	OMX_ExposureControlActParty,
	OMX_ExposureControlActCandlelight,
	OMX_ExposureControlActBarcode,
} OMX_ACT_EXPOSURECONTROLTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


