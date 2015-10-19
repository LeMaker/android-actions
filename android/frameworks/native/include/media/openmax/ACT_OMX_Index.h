#ifndef _OMX_ACT_INDEX_H_
#define _OMX_ACT_INDEX_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/******************************************************************
 *   INCLUDE FILES
 ******************************************************************/
#include <OMX_Types.h>

/*******************************************************************
 * EXTERNAL REFERENCE NOTE: only use if not found in header file
 *******************************************************************/
/*----------         function prototypes      ------------------- */
/*----------         data declarations        ------------------- */
/*******************************************************************
 * PUBLIC DECLARATIONS: defined here, used elsewhere
 *******************************************************************/
/*----------         function prototypes      ------------------- */
/*----------         data declarations        ------------------- */

typedef enum OMX_ACT_INDEXTYPE {
	/* Vendor specific area for storing indices */
    /* Camera Indices */
    OMX_ACT_IndexParamSensorSelect = ((OMX_INDEXTYPE)OMX_IndexVendorStartUnused + 100),   /**< reference: OMX_PARAM_SENSORSELECTTYPE */
    OMX_ACT_IndexConfigImageDeNoiseLevel,           /**< reference: OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE */
    OMX_ACT_IndexConfigCamCapabilities,          /**< reference: OMX_ACT_CAPTYPE */
    OMX_ACT_IndexConfigGlobalBlitCompensation, /**< reference: OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE */
    OMX_ACT_IndexConfigFocusDistance,              /**< reference: OMX_ACT_CONFIG_FOCUSDISTANCETYPE */
    OMX_ACT_IndexConfig_FACEDETECTION,           /**< reference: OMX_ACTIONS_Params */
    OMX_ACT_IndexParamMVCTYPE,           /**< reference: OMX_ACTIONS_Params */
    OMX_IndexParameterStoreMediaData,           /**< reference: StoreMetaDataInBuffersParams */
    OMX_IndexParameterBlendImageType, /**< reference: OMX_ACT_BlendImageType */
    OMX_ACT_IndexParamThumbControl, /**< reference: OMX_ACT_PARAM_THUMBPARAM */
    OMX_ACT_IndexParamExifControl, /**< reference: OMX_ACT_PARAM_EXIFPARAM */
		OMX_ACT_IndexConfigVideoParam,/**< reference: OMX_ACT_CONFIG_VIDEOPARAM */
		OMX_ACT_IndexParmaTsPacket,/**< reference: OMX_ACT_PARAM_TsPacketType */
		OMX_ACT_IndexParmaFaceDet,/**< reference: OMX_ACT_PARAM_FaceDetType */
		OMX_ACT_IndexConfigAGCExposureValue,/**< reference: OMX_ACT_CONFIG_AGCVALUE */
		OMX_ACT_IndexConfigFlashStrobeValue,/**< reference: OMX_ACT_CONFIG_FlashStrobeParams */
		OMX_ACT_IndexConfigHDRParam,/**< reference: OMX_ACT_CONFIG_HDR_EVParams */
		OMX_ACT_IndexConfigMediaFormatConvert,/**< reference: OMX_ACT_CONFIG_CommonParams */
		OMX_ACT_IndexConfigFlip,/**< reference: OMX_ACT_CONFIG_CommonParams */
	/*---for videodeocder---*/
	OMX_IndexStreamBufferUnused = ((OMX_INDEXTYPE)OMX_IndexVendorStartUnused + 1000),
	OMX_IndexParamStreamBufferMode,
	OMX_IndexThumbnail,
	OMX_IndexMinUndequeueBuffer,
	OMX_IndexVideoDecInit,
	OMX_GoogleAndroidIndexEnableAndroidNativeBuffers,
	OMX_GoogleAndroidIndexUseAndroidNativeBuffer,
	OMX_GoogleAndroidIndexUseAndroidNativeBuffer2,
	OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage,
} OMX_TI_INDEXTYPE;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _OMX_ACT_INDEX_H_ */

