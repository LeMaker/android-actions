#ifndef __OMX_Common_V1_2__V1_1_H__
#define __OMX_Common_V1_2__V1_1_H__
#include "OMX_Types.h"
#include "OMX_IVCommon.h"
#define OMX_IndexConfigCommonPortCapturing (0x1000000 + 25)     /**< reference: OMX_CONFIG_PORTBOOLEANTYPE */
#define OMX_IndexConfigFlickerRejection        (0x5000000 + 8)/**< reference: OMX_CONFIG_FLICKERREJECTIONTYPE */
#define OMX_IndexConfigImageHistogram          (0x5000000 + 9)/**< reference: OMX_IMAGE_HISTOGRAMTYPE */
#define OMX_IndexConfigImageHistogramData      (0x5000000 + 10)/**< reference: OMX_IMAGE_HISTOGRAMDATATYPE */
#define OMX_IndexConfigImageHistogramInfo      (0x5000000 + 11)/**< reference: OMX_IMAGE_HISTOGRAMINFOTYPE */
#define OMX_IndexConfigImageCaptureStarted     (0x5000000 + 12)/**< reference: OMX_PARAM_U32TYPE */
#define OMX_IndexConfigImageCaptureEnded       (0x5000000 + 13)/**< reference: OMX_PARAM_U32TYPE */
    
    
#define OMX_IndexConfigSharpness 								(0x7000000 + 35)
#define OMX_IndexConfigCommonExtDigitalZoom 		(0x7000000 + 36)    /**< reference: OMX_CONFIG_ZOOMFACTORTYPE */
#define OMX_IndexConfigCommonExtOpticalZoom 		(0x7000000 + 37)    /**< reference: OMX_CONFIG_ZOOMFACTORTYPE */
#define OMX_IndexConfigCommonCenterFieldOfView 	(0x7000000 + 38) /**< reference: OMX_CONFIG_POINTTYPE */
#define OMX_IndexConfigImageExposureLock 				(0x7000000 + 39)      /**< reference: OMX_IMAGE_CONFIG_LOCKTYPE */
#define OMX_IndexConfigImageWhiteBalanceLock 		(0x7000000 + 40)   /**< reference: OMX_IMAGE_CONFIG_LOCKTYPE */
#define OMX_IndexConfigImageFocusLock 					(0x7000000 + 41)          /**< reference: OMX_IMAGE_CONFIG_LOCKTYPE */
#define OMX_IndexConfigCommonFocusRange 				(0x7000000 + 42)        /**< reference: OMX_CONFIG_FOCUSRANGETYPE */
#define OMX_IndexConfigImageFlashStatus 				(0x7000000 + 43)        /**< reference: OMX_FLASHSTATUSTYPE */
#define OMX_IndexConfigCommonExtCaptureMode 		(0x7000000 + 44)   /**< reference: OMX_CONFIG_EXTCAPTUREMODETYPE */
#define OMX_IndexConfigCommonNDFilterControl 		(0x7000000 + 45)   /**< reference: OMX_CONFIG_NDFILTERCONTROLTYPE */
#define OMX_IndexConfigCommonAFAssistantLight 	(0x7000000 + 46)  /**< reference: OMX_CONFIG_AFASSISTANTLIGHTTYPE */
#define OMX_IndexConfigCommonFocusRegionStatus 	(0x7000000 + 47) /**< reference: OMX_CONFIG_FOCUSREGIONSTATUSTYPE */
#define OMX_IndexConfigCommonFocusRegionControl (0x7000000 + 48)/**< reference: OMX_CONFIG_FOCUSREGIONCONTROLTYPE */
#define OMX_IndexParamInterlaceFormat 					(0x7000000 + 49)          /**< reference: OMX_INTERLACEFORMATTYPE */
#define OMX_IndexConfigDeInterlace 							(0x7000000 + 50)             /**< reference: OMX_DEINTERLACETYPE */
#define OMX_IndexConfigStreamInterlaceFormats 	(0x7000000 + 51)  /**< reference: OMX_STREAMINTERLACEFORMATTYPE */

enum{
	OMX_COLOR_Format32bitABGR8888 = 44,
	OMX_COLOR_FormatYVU420Planar,
	OMX_COLOR_FormatYVU420PackedPlanar,
	OMX_COLOR_FormatYVU420SemiPlanar,
	OMX_COLOR_FormatYVU420PackedSemiPlanar,
	OMX_COLOR_FormatYVU422Planar,
	OMX_COLOR_FormatYVU422PackedPlanar,
	OMX_COLOR_FormatYVU422SemiPlanar,
	OMX_COLOR_FormatYVU422PackedSemiPlanar,
	OMX_COLOR_Format8bitBGR233,
	OMX_COLOR_Format12bitBGR444,
	OMX_COLOR_Format16bitBGRA4444,
	OMX_COLOR_Format16bitBGRA5551,
	OMX_COLOR_Format18bitBGRA5661,
	OMX_COLOR_Format19bitBGRA6661,
	OMX_COLOR_Format24bitBGRA7881,
	OMX_COLOR_Format25bitBGRA8881,
	OMX_COLOR_Format24BitBGRA6666,
	OMX_COLOR_Format24BitRGBA6666,
};

typedef enum OMX_FLICKERREJECTIONTYPE {
    OMX_FlickerRejectionOff = 0,
    OMX_FlickerRejectionAuto,
    OMX_FlickerRejection50,
    OMX_FlickerRejection60,
    OMX_FlickerRejectionKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_FlickerRejectionVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_FlickerRejectionMax = 0x7FFFFFFF
}OMX_FLICKERREJECTIONTYPE;

typedef struct OMX_CONFIG_FLICKERREJECTIONTYPE{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_FLICKERREJECTIONTYPE eFlickerRejection;
} OMX_CONFIG_FLICKERREJECTIONTYPE;

typedef struct OMX_SHARPNESSTYPE{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_S32 nSharpness;
} OMX_SHARPNESSTYPE;

typedef struct OMX_CONFIG_ZOOMFACTORTYPE { 
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion; 
    OMX_U32 nPortIndex; 
    OMX_BU32 xZoomFactor; 
}OMX_CONFIG_ZOOMFACTORTYPE;

typedef enum OMX_IMAGE_LOCKTYPE {
    OMX_IMAGE_LockOff = 0, 
    OMX_IMAGE_LockImmediate,
    OMX_IMAGE_LockAtCapture,
    OMX_IMAGE_LockKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_IMAGE_LockVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_IMAGE_LockMax = 0x7FFFFFFF
} OMX_IMAGE_LOCKTYPE;

typedef struct OMX_IMAGE_CONFIG_LOCKTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_IMAGE_LOCKTYPE eImageLock;
} OMX_IMAGE_CONFIG_LOCKTYPE;

typedef enum OMX_FOCUSRANGETYPE {
    OMX_FocusRangeAuto = 0, 
    OMX_FocusRangeHyperfocal,
    OMX_FocusRangeNormal,
    OMX_FocusRangeSuperMacro,
    OMX_FocusRangeMacro,
    OMX_FocusRangeInfinity,
    OMX_FocusRangeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_FocusRangeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_FocusRangeMax = 0x7FFFFFFF
} OMX_FOCUSRANGETYPE;

typedef struct OMX_CONFIG_FOCUSRANGETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_FOCUSRANGETYPE eFocusRange;
} OMX_CONFIG_FOCUSRANGETYPE;

typedef enum OMX_IMAGE_FLASHSTATUSTYPE
{
    OMX_IMAGE_FlashUnknown 	= 0,
    OMX_IMAGE_FlashOff,
    OMX_IMAGE_FlashCharging,
    OMX_IMAGE_FlashReady,
    OMX_IMAGE_FlashNotAvailable,
    OMX_IMAGE_FlashInsufficientCharge,
    OMX_IMAGE_FlashKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */ 
    OMX_IMAGE_FlashVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_IMAGE_FlashMax = 0x7FFFFFFF
} OMX_IMAGE_FLASHSTATUSTYPE;
              
typedef struct OMX_IMAGE_CONFIG_FLASHSTATUSTYPE { 
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_IMAGE_FLASHSTATUSTYPE eFlashStatus;
} OMX_IMAGE_CONFIG_FLASHSTATUSTYPE;

typedef struct OMX_CONFIG_EXTCAPTUREMODETYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nFrameBefore;
    OMX_BOOL bPrepareCapture;
} OMX_CONFIG_EXTCAPTUREMODETYPE;

typedef struct OMX_FROITYPE {
    OMX_S32 nRectX;
    OMX_S32 nRectY;
    OMX_S32 nRectWidth;
    OMX_S32 nRectHeight;
    OMX_S32 xFocusDistance;
    OMX_FOCUSSTATUSTYPE eFocusStatus;
} OMX_FROITYPE;

typedef struct OMX_CONFIG_FOCUSREGIONSTATUSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL bFocused;
    OMX_U32 nMaxFAreas;
    OMX_U32 nFAreas;
    OMX_FROITYPE sFROIs[1];
    OMX_U32 nFocusRatio;
} OMX_CONFIG_FOCUSREGIONSTATUSTYPE;

typedef struct OMX_MANUALFOCUSRECTTYPE {
    OMX_S32 nRectX;
    OMX_S32 nRectY;
    OMX_S32 nRectWidth;
    OMX_S32 nRectHeight;
} OMX_MANUALFOCUSRECTTYPE;

typedef enum OMX_FOCUSREGIONCONTROLTYPE {
    OMX_FocusRegionControlAuto = 0,
    OMX_FocusRegionControlManual,
    OMX_FocusRegionControlFacePriority,
    OMX_FocusRegionControlObjectPriority,
    OMX_FocusRegionControlKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    OMX_FocusRegionControlVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    OMX_FocusRegionControlMax = 0x7FFFFFFF
} OMX_FOCUSREGIONCONTROLTYPE;

typedef struct OMX_CONFIG_FOCUSREGIONCONTROLTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nFAreas;
    OMX_FOCUSREGIONCONTROLTYPE eFocusRegionsControl;
    OMX_MANUALFOCUSRECTTYPE sManualFRegions[1];
} OMX_CONFIG_FOCUSREGIONCONTROLTYPE;

typedef struct OMX_CONFIG_PORTBOOLEANTYPE{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnabled;
} OMX_CONFIG_PORTBOOLEANTYPE;
#endif
