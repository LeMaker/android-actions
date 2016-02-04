/**
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

#include "omx_videoenc_component.h"
#include <sys/prctl.h>
#include <sys/syscall.h>
#include "vce_cfg.h"
#include "log.h"

/* define the default  buffer size */
#define DEFAULT_WIDTH   352
#define DEFAULT_HEIGHT  288
#define DEFAULT_VIDEO_INPUT_BUF_SIZE   DEFAULT_WIDTH*DEFAULT_HEIGHT*2
#define DEFAULT_VIDEO_OUTPUT_BUF_SIZE   DEFAULT_WIDTH*DEFAULT_HEIGHT*2
#define DEFAULT_VIDEO_SYNCPUT_BUF_SIZE   1024

/* OMX_BASE_FILTER_SYNCPORT_INDEX is the index of any sync port for the derived components */
#define OMX_BASE_FILTER_SYNCPORT_INDEX 2

/* AVC Supported Levels & profiles */
typedef struct VIDEO_PROFILE_LEVEL
{
	OMX_S32  nProfile;
	OMX_S32  nLevel;
} VIDEO_PROFILE_LEVEL_TYPE;

VIDEO_PROFILE_LEVEL_TYPE SupportedAVCProfileLevels[] ={
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel5},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel5},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51},
	{-1,-1}
};

typedef struct VIDEO_ENC_PORT_FORMAT_TYPE
{
	OMX_VIDEO_CODINGTYPE eCompressionFormat;
	OMX_COLOR_FORMATTYPE eColorFormat;
    OMX_U32 xFramerate;
} VIDEO_ENC_PORT_FORMAT_TYPE;

VIDEO_ENC_PORT_FORMAT_TYPE SupportedVideoEncParamFormatType_OutPort[] =
{
	{OMX_VIDEO_CodingAVC,OMX_COLOR_FormatUnused,30<<16},
	{OMX_VIDEO_CodingMJPEG,OMX_COLOR_FormatUnused,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYUV420Planar,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYUV420SemiPlanar,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYUV422Planar,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYUV422SemiPlanar,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYVU420SemiPlanar,1<<16},
	{-1,-1,-1},
};

VIDEO_ENC_PORT_FORMAT_TYPE SupportedVideoEncParamFormatType_InPort[] =
{
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYUV420Planar,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYUV420SemiPlanar,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYUV422Planar,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYUV422SemiPlanar,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatAndroidOpaque,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYVU420Planar,1<<16},
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatYVU420SemiPlanar,1<<16},
	{-1,-1,-1},
};

VIDEO_ENC_PORT_FORMAT_TYPE SupportedVideoEncParamFormatType_SyncPort[] =
{
	{OMX_VIDEO_CodingUnused,OMX_COLOR_FormatUnused,1<<16},
	{-1,-1,-1},
};

/** The Constructor
* @param openmaxStandComp the component handle to be constructed
* @param cComponentName is the name of the constructed component
*/
OMX_ERRORTYPE OMX_ComponentInit(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName)
{
	OMX_U32 i;
	int ret;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	omx_videoenc_PrivateType *omx_videoenc_component_Private;
	omx_videoenc_PortType *inPort,*outPort;
	omx_videoenc_PortType *syncPort;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	DEBUG(DEB_LEV_FUNCTION_NAME, "OMX_ComponentInit----------\n");

	if (!openmaxStandComp->pComponentPrivate)
	{
		openmaxStandComp->pComponentPrivate = calloc(1, sizeof(omx_videoenc_PrivateType));
		if(openmaxStandComp->pComponentPrivate == NULL)
		{
			DEBUG(DEB_LEV_ERR, "err!The videoenc constructor failed in %s,%d\n", __func__, __LINE__);
			DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
			return OMX_ErrorInsufficientResources;
		}
		DEBUG(DEB_LEV_ERR,"In %s, allocating component,%p\n", __func__, openmaxStandComp->pComponentPrivate);
	}
	else
	{
		DEBUG(DEB_LEV_FUNCTION_NAME, "In %s, Error Component %p Already Allocated\n", __func__, openmaxStandComp->pComponentPrivate);
	}

	omx_videoenc_component_Private = openmaxStandComp->pComponentPrivate;
	omx_videoenc_component_Private->ports = NULL;

	/** we could create our own port structures here
	*/
	err = omx_base_filter_Constructor(openmaxStandComp, cComponentName);
	if(err != OMX_ErrorNone)
	{
		DEBUG(DEB_LEV_ERR, "err!The videoenc constructor failed in %s,%d\n", __func__, __LINE__);
		goto omx_err0;
	}

	omx_videoenc_component_Private->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber = 0;
	omx_videoenc_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts = 3;

	/** Allocate Ports and call port constructor. */
	if ((omx_videoenc_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts) \
		&& !omx_videoenc_component_Private->ports)
	{
			omx_videoenc_component_Private->ports = calloc(omx_videoenc_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts,
				sizeof(omx_videoenc_PortType *));
			if (!omx_videoenc_component_Private->ports)
			{
				DEBUG(DEB_LEV_ERR, "err!The videoenc constructor failed in %s,%d\n", __func__,__LINE__);
				err =  OMX_ErrorInsufficientResources;
				goto omx_err1;
			}

			memset(omx_videoenc_component_Private->ports, 0, omx_videoenc_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts * sizeof(omx_videoenc_PortType *));

			for (i = 0; i < omx_videoenc_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++)
			{
				omx_videoenc_component_Private->ports[i] = calloc(1, sizeof(omx_videoenc_PortType));
				if (!omx_videoenc_component_Private->ports[i])
				{
					DEBUG(DEB_LEV_ERR, "err!The videoenc constructor failed in %s,%d\n", __func__,__LINE__);
					err = OMX_ErrorInsufficientResources;
					goto omx_err2;
				}
			}

			err = videoenc_port_Constructor(openmaxStandComp, &omx_videoenc_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX], 0, OMX_TRUE);
			if(err != OMX_ErrorNone)
			{
				DEBUG(DEB_LEV_ERR, "err!The videoenc constructor failed in %s,%d\n", __func__, __LINE__);
				goto omx_err2;
			}

			err = videoenc_port_Constructor(openmaxStandComp, &omx_videoenc_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX], 1, OMX_FALSE);
			if(err != OMX_ErrorNone)
			{
				DEBUG(DEB_LEV_ERR, "err!The videoenc constructor failed in %s,%d\n", __func__, __LINE__);
				goto omx_err3;
			}

			err = videoenc_port_Constructor(openmaxStandComp, &omx_videoenc_component_Private->ports[OMX_BASE_FILTER_SYNCPORT_INDEX], 2, OMX_FALSE);
			if(err != OMX_ErrorNone)
			{
				DEBUG(DEB_LEV_ERR, "err!The videoenc constructor failed in %s,%d\n", __func__, __LINE__);
				goto omx_err4;
			}
	}

	ret = mng_open(&omx_videoenc_component_Private->mng_info);
	if(ret == -1)
	{
		DEBUG(DEB_LEV_ERR, "err!The videoenc constructor failed in %s,%d\n", __func__, __LINE__);
		err = OMX_ErrorInsufficientResources;
		goto omx_err5;
	}

	inPort = (omx_videoenc_PortType *) omx_videoenc_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort = (omx_videoenc_PortType *) omx_videoenc_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	syncPort = (omx_videoenc_PortType *) omx_videoenc_component_Private->ports[OMX_BASE_FILTER_SYNCPORT_INDEX];

	/** Domain specific section for the ports. */
	/* port parameter settings */
	inPort->sPortParam.format.video.nFrameWidth = DEFAULT_WIDTH;
	inPort->sPortParam.format.video.nFrameHeight = DEFAULT_HEIGHT;
	inPort->sPortParam.nBufferSize = DEFAULT_VIDEO_INPUT_BUF_SIZE;
	inPort->sPortParam.bEnabled = OMX_TRUE;
	inPort->sPortParam.nBufferCountActual = 5;

	outPort->sPortParam.format.video.nFrameWidth = DEFAULT_WIDTH;
	outPort->sPortParam.format.video.nFrameHeight = DEFAULT_HEIGHT;
	outPort->sPortParam.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUF_SIZE;
	outPort->sPortParam.bEnabled = OMX_TRUE;
	outPort->sPortParam.nBufferCountActual = 5;

	syncPort->sPortParam.format.video.nFrameWidth = 0;
	syncPort->sPortParam.format.video.nFrameHeight = 0;
	syncPort->sPortParam.nBufferSize = DEFAULT_VIDEO_SYNCPUT_BUF_SIZE;
	syncPort->sPortParam.bEnabled = OMX_FALSE;
	syncPort->sPortParam.nBufferCountActual = 5;

	omx_videoenc_component_Private->mng_info.fd_isFront = 0;
	omx_videoenc_component_Private->mng_info.fd_nAngle = 0;

	omx_videoenc_component_Private->FD_Enable = OMX_FALSE;
	omx_videoenc_component_Private->isFlushed = OMX_FALSE;
	omx_videoenc_component_Private->MNG_EXIT = OMX_FALSE;
	omx_videoenc_component_Private->MSG_EXIT = OMX_FALSE;

	omx_videoenc_component_Private->act_avc_param.i_pic_width = 0;
	omx_videoenc_component_Private->act_avc_param.i_pic_height = 0;
	omx_videoenc_component_Private->act_avc_param.i_bitrate = 0;
	omx_videoenc_component_Private->act_avc_param.i_framerate = 30;
	omx_videoenc_component_Private->act_avc_param.i_profile = 2;  //0 baseline, 1 main, 2 high
	omx_videoenc_component_Private->act_avc_param.i_level_idc = 51;
	omx_videoenc_component_Private->act_avc_param.i_init_qp = 26;
	omx_videoenc_component_Private->act_avc_param.i_bframes = 1; //caution
	omx_videoenc_component_Private->act_avc_param.b_cabac = 1;
	omx_videoenc_component_Private->act_avc_param.i_pic_fmt = 2; //2 YUV411, 5 YUV422
	omx_videoenc_component_Private->act_avc_param.b_semi = 1;
	omx_videoenc_component_Private->act_avc_param.b_mvc = 0;
	omx_videoenc_component_Private->act_avc_param.kIntraPeroid = DEFAULT_IDRPERIOD_AVC;
	omx_videoenc_component_Private->act_avc_param.b_uv_reversal = 0;
	omx_videoenc_component_Private->act_avc_param.b_wfd_mode = 0;

	omx_videoenc_component_Private->act_jpg_param.jpg_quality = 75;
	omx_videoenc_component_Private->act_jpg_param.b_use_new_tbl = 1;
	omx_videoenc_component_Private->act_jpg_param.i_pic_width = 0;
	omx_videoenc_component_Private->act_jpg_param.i_pic_height = 0;
	omx_videoenc_component_Private->act_jpg_param.i_pic_fmt = 2; //2 YUV411, 5 YUV422
	omx_videoenc_component_Private->act_jpg_param.b_semi = 1;
	omx_videoenc_component_Private->act_jpg_param.b_exif = 0;
	omx_videoenc_component_Private->act_jpg_param.b_thumb = 0;
	omx_videoenc_component_Private->act_jpg_param.i_thumb_w = 64;
	omx_videoenc_component_Private->act_jpg_param.i_thumb_h = 64;
	omx_videoenc_component_Private->act_jpg_param.mJpegExif.bExif = 0;
	omx_videoenc_component_Private->act_jpg_param.b_uv_reversal = 0;

	omx_videoenc_component_Private->act_prp_param.b_bld = 0;
	omx_videoenc_component_Private->act_prp_param.i_bld_fmt = 0;
	omx_videoenc_component_Private->act_prp_param.b_downscale = 0;
	omx_videoenc_component_Private->act_prp_param.i_downscale_level = 0;
	omx_videoenc_component_Private->act_prp_param.i_ts_en = 0; //0 no, 1 mpeg2ts, 2 bluts
	omx_videoenc_component_Private->act_prp_param.d_width = 0;
	omx_videoenc_component_Private->act_prp_param.d_height = 0;
	omx_videoenc_component_Private->act_prp_param.b_semi = 1;
	omx_videoenc_component_Private->act_prp_param.i_fmt = 2; //2 YUV411, 5 YUV422
	omx_videoenc_component_Private->act_prp_param.b_uv_reversal = 0;

	omx_videoenc_component_Private->enc_mode = ENC_PREVIEW;
	omx_videoenc_component_Private->enc_codec = ENC_H264;
	omx_videoenc_component_Private->video_encoding_type = OMX_VIDEO_EnCoding_AVC;

	omx_videoenc_component_Private->BufferMgmtFunction = omx_videoenc_BufferMgmtFunction;
	omx_videoenc_component_Private->destructor = omx_videoenc_component_Destructor;
	openmaxStandComp->SendCommand = omx_videoenc_component_SendCommand;
	openmaxStandComp->GetExtensionIndex = omx_videoenc_component_GetExtensionIndex;
	openmaxStandComp->SetConfig = omx_videoenc_component_SetConfig;
	openmaxStandComp->GetConfig = omx_videoenc_component_GetConfig;
	openmaxStandComp->SetParameter = omx_videoenc_component_SetParameter;
	openmaxStandComp->GetParameter = omx_videoenc_component_GetParameter;

	omx_videoenc_component_Private->isIDRBefore = FALSE; //WFD SET
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return err;

omx_err5:
	videoenc_port_Destructor(omx_videoenc_component_Private->ports[OMX_BASE_FILTER_SYNCPORT_INDEX]);

omx_err4:
	videoenc_port_Destructor(omx_videoenc_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX]);

omx_err3:
	videoenc_port_Destructor(omx_videoenc_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX]);

omx_err2:
	for (i=0; i < omx_videoenc_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++)
	{
		if (omx_videoenc_component_Private->ports[i])
		{
			free(omx_videoenc_component_Private->ports[i]);
			omx_videoenc_component_Private->ports[i] = NULL;
		}
	}

omx_err1:
	omx_base_filter_Destructor(openmaxStandComp);

omx_err0:
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);
	return err;
}

/** The destructor
 */
OMX_ERRORTYPE omx_videoenc_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp)
{
	omx_videoenc_PrivateType* omx_videoenc_component_Private = openmaxStandComp->pComponentPrivate;
	OMX_U32 i;
	OMX_ERRORTYPE err = OMX_ErrorNone;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);

	if(omx_videoenc_component_Private->bufferMgmtThreadID == 0)
	{
		tsem_signal(omx_videoenc_component_Private->bStateSem);
		/* Signal Buffer Management Thread to Exit */
		tsem_up(omx_videoenc_component_Private->bMgmtSem);
		err = pthread_join(omx_videoenc_component_Private->bufferMgmtThread, NULL);
		DEBUG(DEB_LEV_FUNCTION_NAME, "In %s after pthread_detach bufferMgmtThread\n", __func__);
		omx_videoenc_component_Private->bufferMgmtThreadID = -1;
		if(err != OMX_ErrorNone) {
			DEBUG(DEB_LEV_FUNCTION_NAME,"In %s pthread_join returned err=%d\n", __func__, err);
		}
	}

	mng_free(&omx_videoenc_component_Private->mng_info);

	/* frees component */
	omx_base_filter_Destructor(openmaxStandComp);

	/* frees ports */
	if (omx_videoenc_component_Private->ports)
	{
		for(i = 0; i < omx_videoenc_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++)
		{
			if(omx_videoenc_component_Private->ports[i])
				omx_videoenc_component_Private->ports[i]->PortDestructor(omx_videoenc_component_Private->ports[i]);
		}
		free(omx_videoenc_component_Private->ports);
		omx_videoenc_component_Private->ports = NULL;
	}

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s\n", __func__);

	return err;
}

/** internal function to set codec related parameters in the private type structure
 */
void SetInternalVideoParameters(OMX_COMPONENTTYPE *openmaxStandComp)
{
	omx_videoenc_PrivateType *omx_videoenc_component_Private = openmaxStandComp->pComponentPrivate;
	omx_videoenc_PortType *outPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX] ;

	if (omx_videoenc_component_Private->video_encoding_type == OMX_VIDEO_EnCoding_AVC)
	{
#ifndef _OPENMAX_V1_2_
		strcpy(outPort->sPortParam.format.video.cMIMEType, CMIMEType_Video_Avc);
#endif
		outPort->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
		outPort->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingAVC;
	}
	else if (omx_videoenc_component_Private->video_encoding_type == OMX_VIDEO_EnCoding_MJPEG)
	{
#ifndef _OPENMAX_V1_2_
		strcpy(outPort->sPortParam.format.video.cMIMEType, CMIMEType_Video_Mjpep);
#endif
		outPort->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingMJPEG;
		outPort->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingMJPEG;
	}
	else if (omx_videoenc_component_Private->video_encoding_type == OMX_VIDEO_EnCoding_PREVIEW)
	{
#ifndef _OPENMAX_V1_2_
		strcpy(outPort->sPortParam.format.video.cMIMEType, CMIMEType_Video_PreView);
#endif
		outPort->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
		outPort->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingUnused;
	}
}

/** @brief standard SendCommand function
 *
 * In general this function does not need a overwrite, but
 * a special derived component could do it.
 */
OMX_ERRORTYPE omx_videoenc_component_SendCommand(
	OMX_HANDLETYPE hComponent,
	OMX_COMMANDTYPE Cmd,
	OMX_U32 nParam,
	OMX_PTR pCmdData)
{
	OMX_COMPONENTTYPE *omxComponent = (OMX_COMPONENTTYPE *)hComponent;
	omx_base_component_PrivateType *omx_base_component_Private = (omx_base_component_PrivateType *)omxComponent->pComponentPrivate;
	internalRequestMessageType *message;
	queue_t *messageQueue;
	tsem_t *messageSem;
	OMX_U32 i,j,k;
	omx_base_PortType *pPort;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	int errQue;
	unsigned int ports_num = 0;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, hComponent);

	messageQueue = omx_base_component_Private->messageQueue;
	messageSem = omx_base_component_Private->messageSem;

	if (omx_base_component_Private->state == OMX_StateInvalid)
	{
		DEBUG(DEB_LEV_ERR, "err!The videoenc failed in %s,%d\n", __FILE__, __LINE__);
		return OMX_ErrorInvalidState;
	}

	message = calloc(1, sizeof(internalRequestMessageType));
	message->messageParam = nParam;
	message->pCmdData=pCmdData;
	/** Fill in the message */
	switch (Cmd)
	{
	case OMX_CommandStateSet:
		 message->messageType = OMX_CommandStateSet;
		 if ((nParam == OMX_StateIdle) && (omx_base_component_Private->state == OMX_StateLoaded))
		 {
			/* Allocate Internal Buffer Storage and Buffer Allocation State flags*/
			/* for all ports */
			for(j = 0; j < NUM_DOMAINS; j++)
			{
				ports_num = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
					omx_base_component_Private->sPortTypesParam[j].nPorts;

				for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber; i < ports_num; i++)
				{
					pPort = omx_base_component_Private->ports[i];
					
					if(pPort->pInternalBufferStorage == NULL)
					{
						pPort->pInternalBufferStorage = calloc(pPort->sPortParam.nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE *));
					}

					if(pPort->bBufferStateAllocated == NULL)
					{
						pPort->bBufferStateAllocated = calloc(pPort->sPortParam.nBufferCountActual, sizeof(BUFFER_STATUS_FLAG));
					}

					for(k=0; k < pPort->sPortParam.nBufferCountActual; k++)
					{
						pPort->bBufferStateAllocated[k] = BUFFER_FREE;
					}
				}
			}

			omx_base_component_Private->transientState = OMX_TransStateLoadedToIdle;
		}
		else if ((nParam == OMX_StateLoaded) && (omx_base_component_Private->state == OMX_StateIdle))
		{
			omx_base_component_Private->transientState = OMX_TransStateIdleToLoaded;
		}
		else if ((nParam == OMX_StateIdle) && (omx_base_component_Private->state == OMX_StateExecuting))
		{
			omx_base_component_Private->transientState = OMX_TransStateExecutingToIdle;
		}
		else if ((nParam == OMX_StateIdle) && (omx_base_component_Private->state == OMX_StatePause))
		{
			omx_base_component_Private->transientState = OMX_TransStatePauseToIdle;
		}
		break;

	case OMX_CommandFlush:
		if (nParam >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) && nParam != OMX_ALL) 
		{
			DEBUG(DEB_LEV_ERR, "err!The videoenc failed in %s,%d\n", __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}
		message->messageType = OMX_CommandFlush;
		break;

	case OMX_CommandPortDisable:
		if (nParam >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) && nParam != OMX_ALL)
		{
			DEBUG(DEB_LEV_ERR, "err!The videoenc failed in %s,%d\n", __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}
		message->messageType = OMX_CommandPortDisable;
		if(((OMX_U32)message->messageParam) == OMX_ALL)
		{
			/* for all ports */
			for(j = 0; j < NUM_DOMAINS; j++)
			{
				ports_num = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
					omx_base_component_Private->sPortTypesParam[j].nPorts;

				for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber; i < ports_num; i++)
				{
					omx_base_component_Private->ports[i]->bIsTransientToDisabled = OMX_TRUE;
				}
			}
		}
		else
		{
			omx_base_component_Private->ports[message->messageParam]->bIsTransientToDisabled = OMX_TRUE;
		}
		break;

	case OMX_CommandPortEnable:
		if (nParam >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) && nParam != OMX_ALL)
		{
			DEBUG(DEB_LEV_ERR, "err!The videoenc failed in %s,%d\n", __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		message->messageType = OMX_CommandPortEnable;
		if( ((OMX_U32)message->messageParam) == OMX_ALL)
		{
			/* for all ports */
			for(j = 0; j < NUM_DOMAINS; j++)
			{
				ports_num = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
					omx_base_component_Private->sPortTypesParam[j].nPorts;

				for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber; i < ports_num; i++)
				{
					omx_base_component_Private->ports[i]->bIsTransientToEnabled = OMX_TRUE;
				}
			}
		}
		else
		{
			omx_base_component_Private->ports[message->messageParam]->bIsTransientToEnabled = OMX_TRUE;
		}
		break;

	case OMX_CommandMarkBuffer:
		if (nParam >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
			omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) && nParam != OMX_ALL) 
		{
			DEBUG(DEB_LEV_ERR, "err!The videoenc failed in %s,%d\n", __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}
		 message->messageType = OMX_CommandMarkBuffer;
		 break;

	 default:
		err = OMX_ErrorUnsupportedIndex;
		break;
	}

	if (err == OMX_ErrorNone)
	{
		errQue = queue(messageQueue, message);
		if (errQue)
		{
			/* /TODO the queue is full. This can be handled in a fine way with
			* some retrials, or other checking. For the moment this is a critical error
			* and simply causes the failure of this call
			*/
			DEBUG(DEB_LEV_ERR, "err!The videoenc failed in %s,%d\n", __FILE__, __LINE__);
			return OMX_ErrorInsufficientResources;
		}
		tsem_up(messageSem);
	}

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p\n", __func__, hComponent);
	return err;
}


/** @brief base function not implemented
 *
 * This function can be eventually implemented by a
 * derived component if needed
 */
OMX_ERRORTYPE omx_videoenc_component_GetExtensionIndex(
	OMX_HANDLETYPE hComponent,
	OMX_STRING cParameterName,
	OMX_INDEXTYPE *pIndexType)
{
	OMX_ERRORTYPE  err = OMX_ErrorNone;
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, hComponent);

	if(strcmp(cParameterName,"OMX.google.android.index.storeMetaDataInBuffers") == 0)
	{
		*pIndexType = OMX_IndexParameterStoreMediaData;
	}
	else if(strcmp(cParameterName,"OMX.actions.index.facedetection") == 0)
	{
		*pIndexType = OMX_ACT_IndexConfig_FACEDETECTION;
	}
	else if(strcmp(cParameterName,"OMX.actions.index.MVC") == 0)
	{
		*pIndexType = OMX_ACT_IndexParamMVCTYPE;
	}
	else if(strcmp(cParameterName,"OMX.actions.index.thumbcontrol") == 0)
	{
		*pIndexType = OMX_ACT_IndexParamThumbControl;
	}
	else if(strcmp(cParameterName,"OMX.actions.index.exifcontrol") == 0)
	{
		*pIndexType = OMX_ACT_IndexParamExifControl;
	}
	else if(strcmp(cParameterName,"OMX.actions.index.tspacket") == 0)
	{
		*pIndexType = OMX_ACT_IndexParmaTsPacket;
	}
	else if(strcmp(cParameterName,"OMX.actions.android.index.MediaFormatConvert") == 0)
	{
		*pIndexType = OMX_ACT_IndexConfigMediaFormatConvert;
	}
	else if(strcmp(cParameterName,"OMX.actions.index.ringbuffer") == 0)
	{
		*pIndexType = OMX_IndexParamRingBuff;
	}
	else if(strcmp(cParameterName,"OMX.google.android.index.prependSPSPPSToIDRFrames") == 0)
	{
		*pIndexType = OMX_IndexParamIDRBefore;
	}
	else if(strcmp(cParameterName,"OMX.google.android.index.useAndroidNativeBuffer2") == 0)
	{
		*pIndexType = OMX_IndexParamNative;
	}
	else
	{
		err = omx_base_component_GetExtensionIndex(hComponent,cParameterName,pIndexType);
	}

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p\n", __func__, hComponent);
	return err;
}

OMX_ERRORTYPE omx_videoenc_component_SetConfig(
	OMX_HANDLETYPE hComponent,
	OMX_INDEXTYPE nParamIndex,
	OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
	omx_videoenc_PrivateType *omx_videoenc_component_Private = openmaxStandComp->pComponentPrivate;
	omx_videoenc_PortType *pPort;
	OMX_U32 portIndex;

	if (ComponentParameterStructure == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!The videoenc failed in %s,%d\n", __FILE__, __LINE__);
		return OMX_ErrorBadParameter;
	}
	DEBUG(DEB_LEV_SIMPLE_SEQ, "Getting parameter %i\n", nParamIndex);

	/* Check which structure we are being fed and fill its header */
	switch((int)nParamIndex)
	{
	case OMX_IndexConfigVideoIntraVOPRefresh:
	{
		/* checkHeader */
		OMX_CONFIG_INTRAREFRESHVOPTYPE *pIDR_Refresh = (OMX_CONFIG_INTRAREFRESHVOPTYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_CONFIG_INTRAREFRESHVOPTYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pIDR_Refresh->nPortIndex;
		pPort = (omx_videoenc_PortType *) omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		omx_videoenc_component_Private->mng_info.IDR_Refresh = pIDR_Refresh->IntraRefreshVOP;

		pPort->pconfig_vopfresh.IntraRefreshVOP = pIDR_Refresh->IntraRefreshVOP;
		break;
	}

	case OMX_IndexConfigCommonInputCrop:
	{
		OMX_CONFIG_RECTTYPE *pconfig_rect=  (OMX_CONFIG_RECTTYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_CONFIG_RECTTYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pconfig_rect->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_INPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		omx_videoenc_component_Private->mng_info.bchanged = 1;
		omx_videoenc_component_Private->mng_info.rect_x = ALIGN_SRC_OFFX(pconfig_rect->nLeft);
		omx_videoenc_component_Private->mng_info.rect_y = ALIGN_SRC_OFFY(pconfig_rect->nTop);
		omx_videoenc_component_Private->mng_info.rect_w = ALIGN_SRC_WIDTH(pconfig_rect->nWidth);
		omx_videoenc_component_Private->mng_info.rect_h = ALIGN_SRC_HEIGHT(pconfig_rect->nHeight);

		memcpy(&pPort->pconfig_crop, pconfig_rect, sizeof(OMX_CONFIG_RECTTYPE));
		break;
	}

	case OMX_IndexConfigVideoAVCIntraPeriod:
	{
		OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pActPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n",err, __FILE__, __LINE__);
			break;
		}

		portIndex = pActPeriod->nPortIndex;
		pPort = (omx_videoenc_PortType *) omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(&pPort->pconfig_avcperiod, pActPeriod, sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD));

		if(pActPeriod->nIDRPeriod > MAX_IDRPERIOD_AVC || pActPeriod->nIDRPeriod < MIN_IDRPERIOD_AVC)
		{
			pPort->pconfig_avcperiod.nIDRPeriod = DEFAULT_IDRPERIOD_AVC;
			DEBUG(DEB_LEV_ERR, "Warning!kIntraPeroid:%d  is too small or too big!,force to %d \n", pActPeriod->nIDRPeriod, DEFAULT_IDRPERIOD_AVC);
		}
		else
		{
			pPort->pconfig_avcperiod.nIDRPeriod = pActPeriod->nIDRPeriod;
		}

		if(omx_videoenc_component_Private->act_avc_param.b_wfd_mode == 0)
			omx_videoenc_component_Private->act_avc_param.kIntraPeroid = pPort->pconfig_avcperiod.nIDRPeriod;
		else
			omx_videoenc_component_Private->act_avc_param.kIntraPeroid = WFD_IDRPERIOD_AVC;
		break;
	}

	case OMX_IndexConfigVideoFramerate:
	{
		OMX_CONFIG_FRAMERATETYPE *pframerate= (OMX_CONFIG_FRAMERATETYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_CONFIG_FRAMERATETYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n",err, __FILE__, __LINE__);
			break;
		}

		portIndex = pframerate->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];

		if( portIndex != OMX_BASE_FILTER_INPUTPORT_INDEX )//set frame rate at input port
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		int framerate = (pframerate->xEncodeFramerate)>>16;
		if((MAX_FRAMERATE_AVC < framerate) || (framerate < MIN_FRAMERATE_AVC))
		{
			omx_videoenc_component_Private->act_avc_param.i_framerate = DEFAULT_FRAMERATE_AVC;
			omx_videoenc_component_Private->mng_info.frame_rate = DEFAULT_FRAMERATE_AVC;
			omx_videoenc_component_Private->mng_info.enc_frame->frmtime = 1000000 / DEFAULT_FRAMERATE_AVC;
			DEBUG(DEB_LEV_ERR,"Warning!framerate:%d fps is too small or too big!,force to %d fps\n", framerate, DEFAULT_FRAMERATE_AVC);

			pPort->sPortParam.format.video.xFramerate = DEFAULT_FRAMERATE_AVC<<16;
			pPort->sVideoParam.xFramerate = DEFAULT_FRAMERATE_AVC<<16;
			pPort->pconfig_framerate.xEncodeFramerate = DEFAULT_FRAMERATE_AVC<<16;
		}
		else
		{
			omx_videoenc_component_Private->act_avc_param.i_framerate = framerate;
			omx_videoenc_component_Private->mng_info.frame_rate = framerate;
			omx_videoenc_component_Private->mng_info.enc_frame->frmtime = 1000000 / framerate;

			pPort->sPortParam.format.video.xFramerate = pframerate->xEncodeFramerate;
			pPort->sVideoParam.xFramerate = pframerate->xEncodeFramerate;
			pPort->pconfig_framerate.xEncodeFramerate = pframerate->xEncodeFramerate;
		}
		break;
	}

	case OMX_IndexConfigVideoBitrate:
	{
		OMX_VIDEO_CONFIG_BITRATETYPE *pbitrate = (OMX_VIDEO_CONFIG_BITRATETYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n",err, __FILE__,__LINE__);
			break;
		}

		portIndex = pbitrate->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(  (portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)  )
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n",portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		omx_videoenc_component_Private->act_avc_param.i_bitrate = pbitrate->nEncodeBitrate / 8;

		pPort->sPortParam.format.video.nBitrate = pbitrate->nEncodeBitrate;
		pPort->pbitrate.nTargetBitrate = pbitrate->nEncodeBitrate;

		memcpy(&pPort->pconfig_bitrate, pbitrate, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE));
		break;
	}

	case OMX_ACT_IndexConfig_FACEDETECTION:
	{
		OMX_ACTIONS_Params *pActParam = (OMX_ACTIONS_Params *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_ACTIONS_Params))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n",err, __FILE__, __LINE__);
			break;
		}

		portIndex = pActParam->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_SYNCPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n",portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		int bEnable = pActParam->bEnable;
		int fd_ret = mng_cmd(&omx_videoenc_component_Private->mng_info, SET_FD, (void*)&bEnable);
		if(fd_ret < 0)
		{
			DEBUG(DEB_LEV_ERR, "err!SET_FD fail!%s,%d\n", __FILE__, __LINE__);
			return OMX_ErrorUndefined;
		}

		omx_videoenc_component_Private->FD_Enable = bEnable;
		break;
	}

	case OMX_ACT_IndexParmaFaceDet:
	{
		OMX_ACT_PARAM_FaceDetType *pActParamFD = (OMX_ACT_PARAM_FaceDetType *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_ACT_PARAM_FaceDetType))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pActParamFD->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_SYNCPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		int nAngle = pActParamFD->nAngle;
		int isFrontCamera = pActParamFD->isFrontCamera;
		if(nAngle != 0 && nAngle != 90 && nAngle != 180 && nAngle != 270)
		{
			DEBUG(DEB_LEV_ERR,"err!facedet nAngle:%d!  %s,%d\n", nAngle, __FILE__, __LINE__);
			return OMX_ErrorBadParameter;
		}

		omx_videoenc_component_Private->mng_info.fd_nAngle = nAngle;
		omx_videoenc_component_Private->mng_info.fd_isFront = isFrontCamera;
		break;
	}

	case OMX_ACT_IndexParamExifControl: /**< reference: OMX_ACT_PARAM_EXIFPARAM */
	{
		OMX_ACT_PARAM_EXIFPARAM *pVideoExif = (OMX_ACT_PARAM_EXIFPARAM *)ComponentParameterStructure;
		int bVersionFlag = -1;
		portIndex = pVideoExif->nPortIndex;
		if(pVideoExif->nSize == 128)
			bVersionFlag = 0;
		else if(pVideoExif->nSize == 148)
			bVersionFlag = 1;

		err = checkHeader(ComponentParameterStructure, sizeof(OMX_ACT_PARAM_EXIFPARAM));
		if(err != OMX_ErrorNone && bVersionFlag == -1)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		int i;
		JpegExif_t *Jexif = &(omx_videoenc_component_Private->act_jpg_param.mJpegExif);
		Jexif->bExif = pVideoExif->bExifEnable;
		if(pVideoExif->bExifEnable == OMX_TRUE)
		{
			Jexif->ImageOri = pVideoExif->ImageOri;
			if(!strcmp(pVideoExif->dataTime,""))
			{
				strcpy(Jexif->dataTime, "20121212");
			} else 
			{
				strcpy(Jexif->dataTime, pVideoExif->dataTime);
			}
			strcpy(Jexif->exifmake, pVideoExif->exifmake);
			strcpy(Jexif->exifmodel, pVideoExif->exifmodel);
			Jexif->focalLengthH = pVideoExif->focalLengthH;
			Jexif->focalLengthL = pVideoExif->focalLengthL;

			if(pVideoExif->bGPS == OMX_TRUE)
				Jexif->exifofGPS = 1;
			else
				Jexif->exifofGPS = -1;

			if(pVideoExif->bGPS == OMX_TRUE)
			{
				for (i = 0; i < 3; i++)
				{
					Jexif->gpsLATH[i] = pVideoExif->gpsLATH[i];
					Jexif->gpsLATL[i] = pVideoExif->gpsLATL[i];
					Jexif->gpsLONGH[i] = pVideoExif->gpsLONGH[i];
					Jexif->gpsLONGL[i] = pVideoExif->gpsLONGL[i];
					Jexif->gpsTimeH[i] = pVideoExif->gpsTimeH[i];
					Jexif->gpsTimeL[i] = pVideoExif->gpsTimeL[i];
				}
				if(bVersionFlag == 1) 
				{
					Jexif->gpsALTIL[0] = pVideoExif->gpsALTIL[0];
					Jexif->gpsALTIH[0] = pVideoExif->gpsALTIH[0];
					Jexif->gpsLATREF = pVideoExif->gpsLATREF; //N:0 S:1
					Jexif->gpsLONGREF = pVideoExif->gpsLONGREF; //E:0 W:1
					Jexif->gpsALTIREF = pVideoExif->gpsALTIREF; //Sea level:0
				}
				else 
				{
					Jexif->gpsALTIL[0] = 0;
					Jexif->gpsALTIH[0] = 0;
					Jexif->gpsLATREF = 0; //N:0 S:1
					Jexif->gpsLONGREF = 0; //E:0 W:1
					Jexif->gpsALTIREF = 0; //Sea level:0
				}

				DEBUG(DEB_LEV_ERR,"GPS Info %d,%d,%d,%d,%d\n", Jexif->gpsALTIL[0], Jexif->gpsALTIH[0], Jexif->gpsLATREF, Jexif->gpsLONGREF, Jexif->gpsALTIREF);
				strcpy(Jexif->gpsData,pVideoExif->gpsDate);
				strcpy(Jexif->gpsprocessMethod,pVideoExif->gpsprocessMethod);
			}

			omx_videoenc_component_Private->act_jpg_param.b_exif = 1;
		}
		else
		{
			omx_videoenc_component_Private->act_jpg_param.b_exif = 0;
		}

		pPort->pExifInfo.nSize = pVideoExif->nSize;
		pPort->pExifInfo.nVersion = pVideoExif->nVersion;
		pPort->pExifInfo.nPortIndex = pVideoExif->nPortIndex;
		pPort->pExifInfo.bExifEnable = pVideoExif->bExifEnable;
		if(pPort->pExifInfo.bExifEnable == OMX_TRUE)
		{
			pPort->pExifInfo.ImageOri = pVideoExif->ImageOri;
			strcpy(pPort->pExifInfo.dataTime,pVideoExif->dataTime);
			strcpy(pPort->pExifInfo.exifmake,pVideoExif->exifmake);
			strcpy(pPort->pExifInfo.exifmodel,pVideoExif->exifmodel);
			pPort->pExifInfo.focalLengthH = pVideoExif->focalLengthH;
			pPort->pExifInfo.focalLengthL = pVideoExif->focalLengthL;

			pPort->pExifInfo.bGPS = pVideoExif->bGPS;
			if(pPort->pExifInfo.bGPS == OMX_TRUE )
			{
				for (i = 0; i < 3; i++)
				{
					pPort->pExifInfo.gpsLATH[i] = pVideoExif->gpsLATH[i];
					pPort->pExifInfo.gpsLATL[i] = pVideoExif->gpsLATL[i];
					pPort->pExifInfo.gpsLONGH[i] = pVideoExif->gpsLONGH[i];
					pPort->pExifInfo.gpsLONGL[i] = pVideoExif->gpsLONGL[i];
					pPort->pExifInfo.gpsTimeH[i] = pVideoExif->gpsTimeH[i];
					pPort->pExifInfo.gpsTimeL[i] = pVideoExif->gpsTimeL[i];
				}

				strcpy(pPort->pExifInfo.gpsDate,pVideoExif->gpsDate);
				strcpy(pPort->pExifInfo.gpsprocessMethod,pVideoExif->gpsprocessMethod);
			}
		}
		break;
	}

	/* RGB->YUV BT601 */
	case OMX_ACT_IndexConfigMediaFormatConvert:
	{
		OMX_ACT_CONFIG_CommonParams *pCommonParams = (OMX_ACT_CONFIG_CommonParams *)ComponentParameterStructure;
		portIndex = pCommonParams->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pCommonParams, sizeof(OMX_ACT_CONFIG_CommonParams));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n",err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *) omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_INPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		DEBUG(DEB_LEV_ERR, "MediaFormatConvert!%d\n", (int)pCommonParams->bEnable);

		if(pCommonParams->bEnable == OMX_FALSE)
			omx_videoenc_component_Private->mng_info.is_formatconvert = 0;
		else
			omx_videoenc_component_Private->mng_info.is_formatconvert = 1;

		memcpy(&pPort->pMediaFormatConvert, pCommonParams, sizeof(OMX_ACT_CONFIG_CommonParams));
		break;
	}

	default:
#if 0
		/*Call the base component function*/
		return omx_base_component_SetConfig(hComponent, nParamIndex, ComponentParameterStructure);
#else
		return OMX_ErrorUnsupportedIndex;
#endif
}

	return err;
}

OMX_ERRORTYPE omx_videoenc_component_GetConfig(
	OMX_HANDLETYPE hComponent,
	OMX_INDEXTYPE nParamIndex,
	OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
	omx_videoenc_PrivateType *omx_videoenc_component_Private = openmaxStandComp->pComponentPrivate;
	omx_videoenc_PortType *pPort;
	OMX_U32 portIndex;

	if (ComponentParameterStructure == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!The videoenc failed in %s,%d\n", __FILE__, __LINE__);
		return OMX_ErrorBadParameter;
	}

	DEBUG(DEB_LEV_SIMPLE_SEQ, "Getting parameter %i\n", nParamIndex);
	/* Check which structure we are being fed and fill its header */
	switch((int)nParamIndex)
	{
	case OMX_IndexConfigVideoIntraVOPRefresh:
	{
		/* checkHeader */
		OMX_CONFIG_INTRAREFRESHVOPTYPE* pIDR_Refresh= (OMX_CONFIG_INTRAREFRESHVOPTYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_CONFIG_INTRAREFRESHVOPTYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n",err, __FILE__, __LINE__);
			break;
		}

		portIndex = pIDR_Refresh->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pIDR_Refresh->IntraRefreshVOP = pPort->pconfig_vopfresh.IntraRefreshVOP;
		break;
	}

	case OMX_IndexConfigCommonInputCrop:
	{
		OMX_CONFIG_RECTTYPE *pconfig_rect= (OMX_CONFIG_RECTTYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_CONFIG_RECTTYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pconfig_rect->nPortIndex;
		pPort = (omx_videoenc_PortType *) omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_INPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(pconfig_rect, &pPort->pconfig_crop, sizeof(OMX_CONFIG_RECTTYPE));
		break;
	}

	case OMX_IndexConfigVideoAVCIntraPeriod:
	{
		OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pActPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n",err, __FILE__, __LINE__);
			break;
		}

		portIndex = pActPeriod->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(pActPeriod, &pPort->pconfig_avcperiod, sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD));
		break;
	}

	case OMX_IndexConfigVideoFramerate:
	{
		OMX_CONFIG_FRAMERATETYPE *pframerate= (OMX_CONFIG_FRAMERATETYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_CONFIG_FRAMERATETYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pframerate->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];

		if(portIndex != OMX_BASE_FILTER_INPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(pframerate, &pPort->pconfig_framerate, sizeof(OMX_CONFIG_FRAMERATETYPE));
		break;
	}

	case OMX_IndexConfigVideoBitrate:
	{
		OMX_VIDEO_CONFIG_BITRATETYPE *pbitrate = (OMX_VIDEO_CONFIG_BITRATETYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pbitrate->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(  (portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)  )
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(pbitrate, &pPort->pconfig_bitrate, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE));
		break;
	}

	case OMX_ACT_IndexConfig_FACEDETECTION:
	{
		OMX_ACTIONS_Params *pActParam = (OMX_ACTIONS_Params *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_ACTIONS_Params))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pActParam->nPortIndex;
		pPort = (omx_videoenc_PortType *) omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_SYNCPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pActParam->bEnable = omx_videoenc_component_Private->FD_Enable;
		break;
	}

	case OMX_ACT_IndexParmaFaceDet:
	{
		OMX_ACT_PARAM_FaceDetType *pActParamFD = (OMX_ACT_PARAM_FaceDetType *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_ACT_PARAM_FaceDetType))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pActParamFD->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_SYNCPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pActParamFD->nAngle = omx_videoenc_component_Private->mng_info.fd_nAngle;
		pActParamFD->isFrontCamera = omx_videoenc_component_Private->mng_info.fd_isFront;
		break;
	}

	case OMX_ACT_IndexParamExifControl: /**< reference: OMX_ACT_CONFIG_EXIFPARAM */
	{
		OMX_ACT_PARAM_EXIFPARAM *pVideoExif = (OMX_ACT_PARAM_EXIFPARAM *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_ACT_PARAM_EXIFPARAM))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pVideoExif->nPortIndex;
		pPort = (omx_videoenc_PortType *) omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pVideoExif->bExifEnable = pPort->pExifInfo.bExifEnable;
		{
			pVideoExif->ImageOri = pPort->pExifInfo.ImageOri;
			strcpy(pVideoExif->dataTime,pPort->pExifInfo.dataTime);
			strcpy(pVideoExif->exifmake,pPort->pExifInfo.exifmake);
			strcpy(pVideoExif->exifmodel,pPort->pExifInfo.exifmodel);
			pVideoExif->focalLengthH = pPort->pExifInfo.focalLengthH;
			pVideoExif->focalLengthL = pPort->pExifInfo.focalLengthL;

			pVideoExif->bGPS = pPort->pExifInfo.bGPS;
			{
				int i;
				for (i = 0; i < 3; i++)
				{
					pVideoExif->gpsLATH[i] = pPort->pExifInfo.gpsLATH[i] ;
					pVideoExif->gpsLATL[i] = pPort->pExifInfo.gpsLATL[i];
					pVideoExif->gpsLONGH[i] = pPort->pExifInfo.gpsLONGH[i];
					pVideoExif->gpsLONGL[i] = pPort->pExifInfo.gpsLONGL[i];
					pVideoExif->gpsTimeH[i] = pPort->pExifInfo.gpsTimeH[i];
					pVideoExif->gpsTimeL[i] = pPort->pExifInfo.gpsTimeL[i];
				}

				strcpy(pVideoExif->gpsDate,pPort->pExifInfo.gpsDate);
				strcpy(pVideoExif->gpsprocessMethod,pPort->pExifInfo.gpsprocessMethod);
			}
		}
		break;
	}

	case OMX_ACT_IndexConfigMediaFormatConvert:
	{
		OMX_ACT_CONFIG_CommonParams *pCommonParams = (OMX_ACT_CONFIG_CommonParams *)ComponentParameterStructure;
		portIndex = pCommonParams->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pCommonParams, sizeof(OMX_ACT_CONFIG_CommonParams));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_INPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pCommonParams->bEnable = pPort->pMediaFormatConvert.bEnable;
		break;
	}

	default:
#if 0
		/*Call the base component function*/
		return omx_base_component_GetConfig(hComponent, nParamIndex, ComponentParameterStructure);
#else
		return OMX_ErrorUnsupportedIndex;
#endif
}

	return err;
}

OMX_ERRORTYPE omx_videoenc_component_SetParameter(
	OMX_HANDLETYPE hComponent,
	OMX_INDEXTYPE nParamIndex,
	OMX_PTR ComponentParameterStructure)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
	OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat;
	OMX_U32 portIndex;
	OMX_PARAM_COMPONENTROLETYPE *pComponentRole;

	/* Check which structure we are being fed and make control its header */
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
	omx_videoenc_PrivateType *omx_videoenc_component_Private = openmaxStandComp->pComponentPrivate;
	omx_videoenc_PortType *pPort;


	if (ComponentParameterStructure == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!The videoenc failed in %s,%d\n", __FILE__, __LINE__);
		return OMX_ErrorBadParameter;
	}

	DEBUG(DEB_LEV_SIMPLE_SEQ, "Setting parameter %i\n", nParamIndex);
	switch((int)nParamIndex)
	{
	case OMX_IndexParamIDRBefore:
	{
		DEBUG(DEB_LEV_ERR, " Video Encoder OMX_IndexParamIDRBefore \n");
		omx_videoenc_component_Private->isIDRBefore = TRUE;
		break;
	}

	case OMX_IndexParamPortDefinition:
	{
		pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
		portIndex = pPortDef->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
		if(err!=OMX_ErrorNone) 
		{
			DEBUG(DEB_LEV_ERR, "err!Parameter Check Error=%x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex > omx_videoenc_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts) 
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		if(pPortDef->eDomain != OMX_PortDomainVideo)
		{
			DEBUG(DEB_LEV_ERR, "err!eDomain %x is not Video!%s,%d\n",pPortDef->eDomain, __FILE__, __LINE__);
			return OMX_ErrorBadParameter;
		}

		if(portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX)
		{
			if(pPortDef->format.video.eCompressionFormat != OMX_VIDEO_CodingUnused)
			{
				DEBUG(DEB_LEV_ERR, "eCompressionFormat is not OMX_VIDEO_CodingUnused!%s,%d", __FILE__, __LINE__);
				return OMX_ErrorBadParameter;
			}

			if((pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) || \
				(pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar) || \
				((int)pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYVU420SemiPlanar) || \
				((int)pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYVU420Planar) || \
				(pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV420Flexible) )
			{

				pPort->sPortParam.nBufferSize = pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight * 3/2;

				if( ((int)pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) || \
					((int)pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYVU420SemiPlanar) )
				{
					omx_videoenc_component_Private->act_avc_param.b_semi = 1;
					omx_videoenc_component_Private->act_jpg_param.b_semi = 1;
					omx_videoenc_component_Private->act_prp_param.b_semi = 1;
				}
				else
				{
					omx_videoenc_component_Private->act_avc_param.b_semi = 0;
					omx_videoenc_component_Private->act_jpg_param.b_semi = 0;
					omx_videoenc_component_Private->act_prp_param.b_semi = 0;
				}

				if( ((int)pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYVU420SemiPlanar) || \
					((int)pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYVU420Planar) )
				{
					omx_videoenc_component_Private->act_avc_param.b_uv_reversal = 1;
					omx_videoenc_component_Private->act_jpg_param.b_uv_reversal = 1;
					omx_videoenc_component_Private->act_prp_param.b_uv_reversal = 1;
				}
				else
				{
					omx_videoenc_component_Private->act_avc_param.b_uv_reversal = 0;
					omx_videoenc_component_Private->act_jpg_param.b_uv_reversal = 0;
					omx_videoenc_component_Private->act_prp_param.b_uv_reversal = 0;
				}

				if( pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV420Flexible )
				{
					DEBUG(DEB_LEV_ERR, "OMX_COLOR_FormatYUV420Flexible, YUV420P, %s, %d\n", __FILE__, __LINE__);
				}

				omx_videoenc_component_Private->act_avc_param.i_pic_fmt = 2;
				omx_videoenc_component_Private->act_jpg_param.i_pic_fmt = 2;
				omx_videoenc_component_Private->act_prp_param.i_fmt = 2;

				omx_videoenc_component_Private->mng_info.is_argb8888 = 0;
				omx_videoenc_component_Private->act_avc_param.b_wfd_mode = 0;
			}
			else if((pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV422SemiPlanar) || \
				(pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV422Planar))
			{
				pPort->sPortParam.nBufferSize = pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight * 2;

				if(pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV422SemiPlanar)
				{
					omx_videoenc_component_Private->act_avc_param.b_semi = 1;
					omx_videoenc_component_Private->act_jpg_param.b_semi = 1;
					omx_videoenc_component_Private->act_prp_param.b_semi = 1;
				}
				else
				{
					omx_videoenc_component_Private->act_avc_param.b_semi = 0;
					omx_videoenc_component_Private->act_jpg_param.b_semi = 0;
					omx_videoenc_component_Private->act_prp_param.b_semi = 0;
				}
				omx_videoenc_component_Private->act_avc_param.i_pic_fmt = 5;  //2 411 5 422
				omx_videoenc_component_Private->act_jpg_param.i_pic_fmt = 5;
				omx_videoenc_component_Private->act_prp_param.i_fmt = 5;

				omx_videoenc_component_Private->mng_info.is_argb8888 = 0;
				omx_videoenc_component_Private->act_avc_param.b_wfd_mode = 0;
			}
			else if((pPortDef->format.video.eColorFormat == OMX_COLOR_Format32bitARGB8888)|| \
				(pPortDef->format.video.eColorFormat == OMX_COLOR_FormatAndroidOpaque))
			{
				pPort->sPortParam.nBufferSize = pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight * 4;

				omx_videoenc_component_Private->mng_info.is_argb8888 = 1;
				omx_videoenc_component_Private->act_avc_param.b_wfd_mode = 1;
				omx_videoenc_component_Private->act_avc_param.kIntraPeroid = WFD_IDRPERIOD_AVC;//wifi-display
				omx_videoenc_component_Private->act_avc_param.b_semi = 0;
				omx_videoenc_component_Private->act_jpg_param.b_semi = 0;
				omx_videoenc_component_Private->act_prp_param.b_semi = 0;

				omx_videoenc_component_Private->act_avc_param.i_pic_fmt = ENC_ARGB8888;
				omx_videoenc_component_Private->act_jpg_param.i_pic_fmt = ENC_ARGB8888;
				omx_videoenc_component_Private->act_prp_param.i_fmt = ENC_ARGB8888;
			}
			else
			{
				DEBUG(DEB_LEV_ERR, "err!eColorFormat:%x!%s,%d\n", pPortDef->format.video.eColorFormat, __FILE__, __LINE__);
				return OMX_ErrorBadParameter;
			}

			int framerate = (pPortDef->format.video.xFramerate)>>16;
			if((MAX_FRAMERATE_AVC < framerate) || (framerate < MIN_FRAMERATE_AVC ))
			{
				omx_videoenc_component_Private->act_avc_param.i_framerate = DEFAULT_FRAMERATE_AVC;
				omx_videoenc_component_Private->mng_info.frame_rate = DEFAULT_FRAMERATE_AVC;
				omx_videoenc_component_Private->mng_info.enc_frame->frmtime = 1000000 / DEFAULT_FRAMERATE_AVC;
				DEBUG(DEB_LEV_ERR,"Warning!framerate:%d fps is too small or too big!,force to %d fps\n", framerate, DEFAULT_FRAMERATE_AVC);

				pPort->sVideoParam.xFramerate = DEFAULT_FRAMERATE_AVC<<16;
				pPort->pconfig_framerate.xEncodeFramerate = DEFAULT_FRAMERATE_AVC<<16;
				pPort->sPortParam.format.video.xFramerate = DEFAULT_FRAMERATE_AVC<<16;
			}
			else
			{
				omx_videoenc_component_Private->act_avc_param.i_framerate = framerate;
				omx_videoenc_component_Private->mng_info.frame_rate = framerate;
				omx_videoenc_component_Private->mng_info.enc_frame->frmtime = 1000000 / framerate;

				pPort->sVideoParam.xFramerate = pPortDef->format.video.xFramerate;
				pPort->pconfig_framerate.xEncodeFramerate = pPortDef->format.video.xFramerate;
				pPort->sPortParam.format.video.xFramerate = pPortDef->format.video.xFramerate;
			}

			omx_videoenc_component_Private->mng_info.i_video_fmt = omx_videoenc_component_Private->act_avc_param.i_pic_fmt;
			omx_videoenc_component_Private->mng_info.b_semi = omx_videoenc_component_Private->act_avc_param.b_semi;
			
			omx_videoenc_component_Private->mng_info.i_source_width = ALIGN_SRC_WIDTH(pPortDef->format.video.nFrameWidth);
			omx_videoenc_component_Private->mng_info.i_source_height = ALIGN_SRC_HEIGHT(pPortDef->format.video.nFrameHeight);
			if(pPortDef->format.video.nStride > 0)
				omx_videoenc_component_Private->mng_info.i_source_stride = ALIGN_SRC_STRIDE(pPortDef->format.video.nStride);
			else  if(pPortDef->format.video.nStride < 0)
				omx_videoenc_component_Private->mng_info.i_source_stride = ALIGN_SRC_STRIDE(-pPortDef->format.video.nStride);
			else
				omx_videoenc_component_Private->mng_info.i_source_stride = ALIGN_SRC_STRIDE(pPortDef->format.video.nFrameWidth);
		}
		else if( portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			if(pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)
			{
				pPort->sPortParam.nBufferSize = ((pPortDef->format.video.nFrameWidth + 15)&(~15)) * ((pPortDef->format.video.nFrameHeight + 15)&(~15)) / 2;

				omx_videoenc_component_Private->enc_codec = ENC_H264;
				omx_videoenc_component_Private->enc_mode = ENC_ENCODE;
				omx_videoenc_component_Private->act_avc_param.i_bitrate = pPortDef->format.video.nBitrate / 8; //note: bits/s -> Bytes/s
				omx_videoenc_component_Private->video_encoding_type = OMX_VIDEO_EnCoding_AVC;

				int framerate = (pPortDef->format.video.xFramerate)>>16;
				DEBUG(DEB_LEV_PARAMS,"outport framerate:%d fps\n", framerate);
				if(framerate != 0)
				{
					if((MAX_FRAMERATE_AVC < framerate) || (framerate < MIN_FRAMERATE_AVC))
					{
						omx_videoenc_component_Private->act_avc_param.i_framerate = DEFAULT_FRAMERATE_AVC;
						omx_videoenc_component_Private->mng_info.frame_rate = DEFAULT_FRAMERATE_AVC;
						omx_videoenc_component_Private->mng_info.enc_frame->frmtime = 1000000 / DEFAULT_FRAMERATE_AVC;
						DEBUG(DEB_LEV_ERR,"Warning!framerate:%d fps is too small or too big!,force to %d fps!%s,%d\n",
							framerate, DEFAULT_FRAMERATE_AVC, __FILE__, __LINE__);

						pPort->sVideoParam.xFramerate = DEFAULT_FRAMERATE_AVC<<16;
						pPort->pconfig_framerate.xEncodeFramerate = DEFAULT_FRAMERATE_AVC<<16;
						pPort->sPortParam.format.video.xFramerate = DEFAULT_FRAMERATE_AVC<<16;
					}
					else
					{
						omx_videoenc_component_Private->act_avc_param.i_framerate = framerate;
						omx_videoenc_component_Private->mng_info.frame_rate = framerate;
						omx_videoenc_component_Private->mng_info.enc_frame->frmtime = 1000000 / framerate;

						pPort->sVideoParam.xFramerate = pPortDef->format.video.xFramerate;
						pPort->pconfig_framerate.xEncodeFramerate = pPortDef->format.video.xFramerate;
						pPort->sPortParam.format.video.xFramerate = pPortDef->format.video.xFramerate;
					}
				}
			}
			else if(pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingMJPEG)
			{
				pPort->sPortParam.nBufferSize = ((pPortDef->format.video.nFrameWidth + 15)&(~15)) * ((pPortDef->format.video.nFrameHeight + 15)&(~15)) * 2;

				omx_videoenc_component_Private->enc_codec = ENC_JPEG;
				omx_videoenc_component_Private->enc_mode = ENC_ENCODE;
				omx_videoenc_component_Private->mng_info.frame_rate = 1;
				omx_videoenc_component_Private->act_avc_param.i_framerate =1;
				omx_videoenc_component_Private->mng_info.i_bframes = 0;
				omx_videoenc_component_Private->video_encoding_type = OMX_VIDEO_EnCoding_MJPEG;
			}
			else if(pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingUnused)
			{
				pPort->sPortParam.nBufferSize = ((pPortDef->format.video.nFrameWidth + 15)&(~15)) * ((pPortDef->format.video.nFrameHeight + 15)&(~15)) * 2;

				omx_videoenc_component_Private->enc_mode = ENC_PREVIEW;
				omx_videoenc_component_Private->mng_info.frame_rate = 1;
				omx_videoenc_component_Private->act_avc_param.i_framerate =1;
				omx_videoenc_component_Private->mng_info.i_bframes = 0;
				omx_videoenc_component_Private->video_encoding_type = OMX_VIDEO_EnCoding_PREVIEW;
			}
			else
			{
				DEBUG(DEB_LEV_ERR,"err!eCompressionFormat:%x!%s,%d\n",pPortDef->format.video.eCompressionFormat, __FILE__, __LINE__);
				return OMX_ErrorBadParameter;
			}

			omx_videoenc_component_Private->act_avc_param.i_pic_width = pPortDef->format.video.nFrameWidth;
			omx_videoenc_component_Private->act_avc_param.i_pic_height = pPortDef->format.video.nFrameHeight;
			omx_videoenc_component_Private->act_jpg_param.i_pic_width = pPortDef->format.video.nFrameWidth;
			omx_videoenc_component_Private->act_jpg_param.i_pic_height =  pPortDef->format.video.nFrameHeight;
			omx_videoenc_component_Private->act_prp_param.d_width = ALIGN_16(pPortDef->format.video.nFrameWidth);
			omx_videoenc_component_Private->act_prp_param.d_height = ALIGN_16(pPortDef->format.video.nFrameHeight);
		}
		else if( portIndex == OMX_BASE_FILTER_SYNCPORT_INDEX )
		{
			pPort->sPortParam.nBufferSize = 1024;
		}
		else
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pPort->pbitrate.nTargetBitrate = pPortDef->format.video.nBitrate;
		pPort->pconfig_bitrate.nEncodeBitrate = pPortDef->format.video.nBitrate;
		pPort->sVideoParam.eCompressionFormat = pPortDef->format.video.eCompressionFormat;
		pPort->sVideoParam.eColorFormat = pPortDef->format.video.eColorFormat;

		if( (portIndex != OMX_BASE_FILTER_INPUTPORT_INDEX) && (pPortDef->format.video.eCompressionFormat != OMX_VIDEO_CodingAVC) )
		{
			pPort->sVideoParam.xFramerate = pPortDef->format.video.xFramerate;
			pPort->pconfig_framerate.xEncodeFramerate = pPortDef->format.video.xFramerate;
			pPort->sPortParam.format.video.xFramerate = pPortDef->format.video.xFramerate;
		}

		OMX_U32 old_nBufferCountActual = 0, j;
		old_nBufferCountActual = pPort->sPortParam.nBufferCountActual;
		DEBUG(DEB_LEV_PARAMS,"portIndex:%d,old_nBufferCountActual:%d ,new_nBufferCountActual:%d \n", portIndex, old_nBufferCountActual, pPortDef->nBufferCountActual);

		//ringbuffer usually not work!
		if(pPort->ringbuffer == OMX_TRUE && portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			if(pPortDef->nBufferCountActual > MAX_RingBuffers_NO || pPortDef->nBufferCountActual < MIN_RingBuffers_NO)
			{
				pPort->sPortParam.nBufferCountActual = DEFAULT_RingBuffers_NO;
				DEBUG(DEB_LEV_ERR,"Warning!nBufferCountActual:%d  is too small or too big!,force to %d!%s,%d\n",
					pPortDef->nBufferCountActual, DEFAULT_RingBuffers_NO, __FILE__, __LINE__);
			}
			else
			{
				pPort->sPortParam.nBufferCountActual = pPortDef->nBufferCountActual;
			}

			pPort->ringbuf_framesize = pPort->sPortParam.nBufferSize;
			if(omx_videoenc_component_Private->video_encoding_type == OMX_VIDEO_EnCoding_AVC)
			{
				int buffersize = pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight;
				if(buffersize > 1920*1088)
					pPort->ringbuf_framesize = 200000; //1920x2160:48Mb/30
				else if(buffersize > 1280*720)
					pPort->ringbuf_framesize = 100000; //1080P:24Mb/30
				else if(buffersize > 640*480)
					pPort->ringbuf_framesize = 67000; //720P:16Mb/30
				else
					pPort->ringbuf_framesize = 25000; //VGA:6Mb/30
			}
		}
		else
		{
			pPort->sPortParam.nBufferCountActual = pPortDef->nBufferCountActual;
		}

		pPort->sPortParam.format.video.nFrameWidth = pPortDef->format.video.nFrameWidth;
		pPort->sPortParam.format.video.nFrameHeight = pPortDef->format.video.nFrameHeight;
		pPort->sPortParam.format.video.nBitrate = pPortDef->format.video.nBitrate;
		pPort->sPortParam.format.video.bFlagErrorConcealment = pPortDef->format.video.bFlagErrorConcealment;
		pPort->sPortParam.format.video.eCompressionFormat = pPortDef->format.video.eCompressionFormat;
		pPort->sPortParam.format.video.eColorFormat = pPortDef->format.video.eColorFormat;
		pPort->sPortParam.format.video.pNativeWindow = pPortDef->format.video.pNativeWindow;
		pPort->sPortParam.format.video.pNativeRender = pPortDef->format.video.pNativeRender;
		pPort->sPortParam.format.video.nStride = pPortDef->format.video.nStride;

#ifndef _OPENMAX_V1_2_
		if(pPortDef->format.video.cMIMEType != NULL)
		{
			strcpy(pPort->sPortParam.format.video.cMIMEType, pPortDef->format.video.cMIMEType);
		}
#endif

		/* If component state Idle/Pause/Executing and re-alloc the following private variables */
		if ((omx_videoenc_component_Private->state == OMX_StateIdle ||
			omx_videoenc_component_Private->state == OMX_StatePause  ||
			omx_videoenc_component_Private->state == OMX_StateExecuting) &&
			(pPortDef->nBufferCountActual > old_nBufferCountActual) )
		{
			if(pPort->pInternalBufferStorage)
			{
				pPort->pInternalBufferStorage = realloc(pPort->pInternalBufferStorage, pPort->sPortParam.nBufferCountActual * sizeof(OMX_BUFFERHEADERTYPE *));
			}

			if(pPort->bBufferStateAllocated)
			{
				pPort->bBufferStateAllocated = realloc(pPort->bBufferStateAllocated, pPort->sPortParam.nBufferCountActual * sizeof(BUFFER_STATUS_FLAG));
				for(j=0; j < pPort->sPortParam.nBufferCountActual; j++)
				{
					pPort->bBufferStateAllocated[j] = BUFFER_FREE;
				}
			}
		}

		DEBUG(DEB_LEV_PARAMS, "Set eCompressionFormat:%d\n", pPort->sPortParam.format.video.eCompressionFormat);
		break;
	}

	case OMX_IndexParamVideoPortFormat:
	{
		pVideoPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
		portIndex = pVideoPortFormat->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pVideoPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error=%x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];

		if(portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX)
		{
			if(pVideoPortFormat->eCompressionFormat != OMX_VIDEO_CodingUnused) 
			{
				DEBUG(DEB_LEV_ERR, "err!eCompressionFormat != OMX_VIDEO_CodingUnused!%s,%d\n", __FILE__, __LINE__);
				return OMX_ErrorBadParameter;
			}

			if((pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) || \
				(pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYUV420Planar) || \
				((int)pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYVU420SemiPlanar) || \
				((int)pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYVU420Planar) )
			{
				pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 3/2;

				if( ((int)pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) || \
					((int)pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYVU420SemiPlanar) )
				{
					omx_videoenc_component_Private->act_avc_param.b_semi = 1;
					omx_videoenc_component_Private->act_jpg_param.b_semi = 1;
					omx_videoenc_component_Private->act_prp_param.b_semi = 1;
				}
				else
				{
					omx_videoenc_component_Private->act_avc_param.b_semi = 0;
					omx_videoenc_component_Private->act_jpg_param.b_semi = 0;
					omx_videoenc_component_Private->act_prp_param.b_semi = 0;
				}

				if( ((int)pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYVU420SemiPlanar) || \
					((int)pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYVU420Planar) )
				{
					omx_videoenc_component_Private->act_avc_param.b_uv_reversal = 1;
					omx_videoenc_component_Private->act_jpg_param.b_uv_reversal = 1;
					omx_videoenc_component_Private->act_prp_param.b_uv_reversal = 1;
				}
				else
				{
					omx_videoenc_component_Private->act_avc_param.b_uv_reversal = 0;
					omx_videoenc_component_Private->act_jpg_param.b_uv_reversal = 0;
					omx_videoenc_component_Private->act_prp_param.b_uv_reversal = 0;
				}

				omx_videoenc_component_Private->act_avc_param.i_pic_fmt = 2;  //2 411 5 422
				omx_videoenc_component_Private->act_jpg_param.i_pic_fmt = 2;
				omx_videoenc_component_Private->act_prp_param.i_fmt = 2;

				omx_videoenc_component_Private->mng_info.is_argb8888 = 0;
				omx_videoenc_component_Private->act_avc_param.b_wfd_mode = 0;
			}
			else if((pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYUV422SemiPlanar) || \
				(pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYUV422Planar))
			{
				pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 2;
				if(pVideoPortFormat->eColorFormat == OMX_COLOR_FormatYUV422SemiPlanar)
				{
					omx_videoenc_component_Private->act_avc_param.b_semi = 1;
					omx_videoenc_component_Private->act_jpg_param.b_semi = 1;
					omx_videoenc_component_Private->act_prp_param.b_semi = 1;
				}
				else
				{
					omx_videoenc_component_Private->act_avc_param.b_semi = 0;
					omx_videoenc_component_Private->act_jpg_param.b_semi = 0;
					omx_videoenc_component_Private->act_prp_param.b_semi = 0;
				}
				omx_videoenc_component_Private->act_avc_param.i_pic_fmt = 5;  //2 411 5 422
				omx_videoenc_component_Private->act_jpg_param.i_pic_fmt = 5;
				omx_videoenc_component_Private->act_prp_param.i_fmt = 5;

				omx_videoenc_component_Private->mng_info.is_argb8888 = 0;
				omx_videoenc_component_Private->act_avc_param.b_wfd_mode = 0;
			}
			else if((pVideoPortFormat->eColorFormat == OMX_COLOR_Format32bitARGB8888) || (pVideoPortFormat->eColorFormat == OMX_COLOR_FormatAndroidOpaque))
			{
				pPort->sPortParam.nBufferSize = pPort->sPortParam.format.video.nFrameWidth * pPort->sPortParam.format.video.nFrameHeight * 4;
				omx_videoenc_component_Private->mng_info.is_argb8888 = 1;
				omx_videoenc_component_Private->act_avc_param.b_wfd_mode = 1;
				omx_videoenc_component_Private->act_avc_param.kIntraPeroid = WFD_IDRPERIOD_AVC; //wifi-display
				omx_videoenc_component_Private->act_avc_param.b_semi = 0;
				omx_videoenc_component_Private->act_jpg_param.b_semi = 0;
				omx_videoenc_component_Private->act_prp_param.b_semi = 0;

				omx_videoenc_component_Private->act_avc_param.i_pic_fmt = ENC_ARGB8888;
				omx_videoenc_component_Private->act_jpg_param.i_pic_fmt = ENC_ARGB8888;
				omx_videoenc_component_Private->act_prp_param.i_fmt = ENC_ARGB8888;
			}
			else
			{
				DEBUG(DEB_LEV_ERR, "err!eColorFormat: %x!%s,%d\n", pVideoPortFormat->eColorFormat, __FILE__, __LINE__);
				return OMX_ErrorBadParameter;
			}

			int framerate = (pVideoPortFormat->xFramerate)>>16;
			if((MAX_FRAMERATE_AVC < framerate) || (framerate < MIN_FRAMERATE_AVC))
			{
				omx_videoenc_component_Private->act_avc_param.i_framerate = DEFAULT_FRAMERATE_AVC;
				omx_videoenc_component_Private->mng_info.frame_rate = DEFAULT_FRAMERATE_AVC;
				omx_videoenc_component_Private->mng_info.enc_frame->frmtime = 1000000 / DEFAULT_FRAMERATE_AVC;
				DEBUG(DEB_LEV_ERR,"Warning!framerate:%d fps is too small or too big!,force to %d fps\n", framerate, DEFAULT_FRAMERATE_AVC);

				pPort->sPortParam.format.video.xFramerate = DEFAULT_FRAMERATE_AVC<<16;
				pPort->pconfig_framerate.xEncodeFramerate = DEFAULT_FRAMERATE_AVC<<16;
				pPort->sVideoParam.xFramerate = DEFAULT_FRAMERATE_AVC<<16;
			}
			else
			{
				omx_videoenc_component_Private->act_avc_param.i_framerate = framerate;
				omx_videoenc_component_Private->mng_info.frame_rate = framerate;
				omx_videoenc_component_Private->mng_info.enc_frame->frmtime = 1000000 / framerate;

				pPort->sPortParam.format.video.xFramerate = pVideoPortFormat->xFramerate;
				pPort->pconfig_framerate.xEncodeFramerate = pVideoPortFormat->xFramerate;
				pPort->sVideoParam.xFramerate = pVideoPortFormat->xFramerate;
			}
			
			omx_videoenc_component_Private->mng_info.b_semi = omx_videoenc_component_Private->act_avc_param.b_semi;
		}
		else if( portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			if(pVideoPortFormat->eCompressionFormat == OMX_VIDEO_CodingAVC)
			{
				omx_videoenc_component_Private->enc_codec = ENC_H264;
				omx_videoenc_component_Private->enc_mode = ENC_ENCODE;
				omx_videoenc_component_Private->video_encoding_type = OMX_VIDEO_EnCoding_AVC;
			}
			else if(pVideoPortFormat->eCompressionFormat == OMX_VIDEO_CodingMJPEG)
			{
				omx_videoenc_component_Private->enc_codec = ENC_JPEG;
				omx_videoenc_component_Private->enc_mode = ENC_ENCODE;
				omx_videoenc_component_Private->mng_info.frame_rate = 1;
				omx_videoenc_component_Private->mng_info.i_bframes = 0;
				omx_videoenc_component_Private->video_encoding_type = OMX_VIDEO_EnCoding_MJPEG;
			}
			else if(pVideoPortFormat->eCompressionFormat == OMX_VIDEO_CodingUnused)
			{
				omx_videoenc_component_Private->enc_mode = ENC_PREVIEW;
				omx_videoenc_component_Private->mng_info.frame_rate = 1;
				omx_videoenc_component_Private->mng_info.i_bframes = 0;
				omx_videoenc_component_Private->video_encoding_type = OMX_VIDEO_EnCoding_PREVIEW;
			}
			else
			{
				DEBUG(DEB_LEV_ERR, "err!eCompressionFormat Error:%x!%s,%d\n", pVideoPortFormat->eCompressionFormat, __FILE__, __LINE__);
				return OMX_ErrorBadParameter;
			}
		}
		else if(portIndex == OMX_BASE_FILTER_SYNCPORT_INDEX )
		{
			if( pVideoPortFormat->eCompressionFormat != OMX_VIDEO_CodingUnused  ||  \
				pVideoPortFormat->eColorFormat != OMX_COLOR_FormatUnused)
			{
				DEBUG(DEB_LEV_ERR, "err!eCompressionFormat Error:%x!%s,%d\n", pVideoPortFormat->eCompressionFormat, __FILE__, __LINE__);
				return OMX_ErrorBadParameter;
			}
		}
		else
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pPort->sPortParam.format.video.eCompressionFormat = pVideoPortFormat->eCompressionFormat;
		pPort->sPortParam.format.video.eColorFormat = pVideoPortFormat->eColorFormat;
		pPort->sVideoParam.eCompressionFormat = pVideoPortFormat->eCompressionFormat;
		pPort->sVideoParam.eColorFormat = pVideoPortFormat->eColorFormat;

		if((portIndex != OMX_BASE_FILTER_INPUTPORT_INDEX) && (pVideoPortFormat->eCompressionFormat != OMX_VIDEO_CodingAVC))
		{
			pPort->sPortParam.format.video.xFramerate = pVideoPortFormat->xFramerate;
			pPort->pconfig_framerate.xEncodeFramerate = pVideoPortFormat->xFramerate;
			pPort->sVideoParam.xFramerate = pVideoPortFormat->xFramerate;
		}
		break;
	}

	case OMX_IndexParamStandardComponentRole:
	{
		pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		if (omx_videoenc_component_Private->state != OMX_StateLoaded && omx_videoenc_component_Private->state != OMX_StateWaitForResources) 
		{
			DEBUG(DEB_LEV_ERR, "err!Incorrect State:%x!%s,%d\n", omx_videoenc_component_Private->state, __FILE__, __LINE__);
			return OMX_ErrorIncorrectStateOperation;
		}

		if (!strcmp((char *)pComponentRole->cRole, VIDEO_ENC_COMP_ROLE))
		{
			omx_videoenc_component_Private->video_encoding_type = OMX_VIDEO_EnCoding_AVC;
		}
		else if (!strcmp((char *)pComponentRole->cRole, JPEG_ENC_COMP_ROLE))
		{
			omx_videoenc_component_Private->video_encoding_type = OMX_VIDEO_EnCoding_MJPEG;
		}
		else if (!strcmp((char *)pComponentRole->cRole, IPP_ENC_COMP_ROLE))
		{
			omx_videoenc_component_Private->video_encoding_type = OMX_VIDEO_EnCoding_PREVIEW;
		}
		else
		{
			DEBUG(DEB_LEV_ERR, "err!Incorrect cRole:%s!%s,%d\n", (char *)pComponentRole->cRole, __FILE__, __LINE__);
			return OMX_ErrorUnsupportedSetting;
		}

		SetInternalVideoParameters(openmaxStandComp);
		break;
	}

	case OMX_IndexParamVideoBitrate:
	{
		OMX_VIDEO_PARAM_BITRATETYPE *pVideoBitrate = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParameterStructure;
		portIndex = pVideoBitrate->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pVideoBitrate, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
		if(err!=OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		if(pVideoBitrate->eControlRate == OMX_Video_ControlRateConstant || pVideoBitrate->eControlRate == OMX_Video_ControlRateVariable)
			omx_videoenc_component_Private->act_avc_param.i_bitrate = pVideoBitrate->nTargetBitrate / 8;
		else if(pVideoBitrate->eControlRate == OMX_Video_ControlRateDisable)
			omx_videoenc_component_Private->act_avc_param.i_bitrate = 0;
		else
		{
			DEBUG(DEB_LEV_ERR, "err!eControlRate Error:%x!%s,%d\n", pVideoBitrate->eControlRate, __FILE__, __LINE__);
			return OMX_ErrorBadParameter; 
		}

		DEBUG(DEB_LEV_PARAMS, "act_avc_param.i_bitrate:%d\n", omx_videoenc_component_Private->act_avc_param.i_bitrate);

		pPort->sPortParam.format.video.nBitrate = pVideoBitrate->nTargetBitrate;
		pPort->pconfig_bitrate.nEncodeBitrate = pVideoBitrate->nTargetBitrate;

		memcpy(&pPort->pbitrate,pVideoBitrate, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
		break;
	}

	case OMX_IndexParamVideoAvc:
	{
		OMX_VIDEO_PARAM_AVCTYPE *pAvcType = (OMX_VIDEO_PARAM_AVCTYPE *)ComponentParameterStructure;
		portIndex = pAvcType->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pAvcType, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		int i_profile = 0,i_level = 0;
		switch(pAvcType->eProfile)
		{
		case OMX_VIDEO_AVCProfileBaseline:
			i_profile = 0;
			break;
		case OMX_VIDEO_AVCProfileMain:
			i_profile = 1;
			break;
		case	OMX_VIDEO_AVCProfileHigh:
			i_profile = 2;
			break;
		default :
			DEBUG(DEB_LEV_ERR, "err!eProfile Error:%x!%s,%d\n", pAvcType->eProfile, __FILE__, __LINE__);
			return OMX_ErrorBadParameter;
			break;
		};

		switch(pAvcType->eLevel)
		{
		case OMX_VIDEO_AVCLevel1:
			i_level = 10; break;
		case OMX_VIDEO_AVCLevel1b:
			i_level = 9; break;
		case OMX_VIDEO_AVCLevel11:
			i_level = 11; break;
		case OMX_VIDEO_AVCLevel12:
			i_level = 12; break;
		case OMX_VIDEO_AVCLevel13:
			i_level = 13; break;
		case OMX_VIDEO_AVCLevel2:
			i_level = 20; break;
		case OMX_VIDEO_AVCLevel21:
			i_level = 21; break;
		case OMX_VIDEO_AVCLevel22:
			i_level = 22; break;
		case OMX_VIDEO_AVCLevel3:
			i_level = 23; break;
		case OMX_VIDEO_AVCLevel31:
			i_level = 31; break;
		case OMX_VIDEO_AVCLevel32:
			i_level = 32; break;
		case OMX_VIDEO_AVCLevel4:
			i_level = 40; break;
		case OMX_VIDEO_AVCLevel41:
			i_level = 41; break;
		case OMX_VIDEO_AVCLevel42:
			i_level = 42; break;
		case OMX_VIDEO_AVCLevel5:
			i_level = 50; break;
		case OMX_VIDEO_AVCLevel51:
			i_level = 51; break;
		default :
			DEBUG(DEB_LEV_ERR, "err!eLevel Error:%x!%s,%d\n", pAvcType->eLevel, __FILE__, __LINE__);
			return OMX_ErrorBadParameter;
			break;
		};

		omx_videoenc_component_Private->act_avc_param.i_profile = i_profile;
		omx_videoenc_component_Private->act_avc_param.i_level_idc = i_level;
		omx_videoenc_component_Private->act_avc_param.i_bframes = pAvcType->nBFrames>2 ? 2: pAvcType->nBFrames;//caution
		if(omx_videoenc_component_Private->act_avc_param.b_wfd_mode == 0)
		{
			omx_videoenc_component_Private->act_avc_param.kIntraPeroid = pAvcType->nBFrames + pAvcType->nPFrames + 1;
			if(omx_videoenc_component_Private->act_avc_param.kIntraPeroid > MAX_IDRPERIOD_AVC)
			{
				DEBUG(DEB_LEV_ERR,"Warning! nBFrames+nPFrames:%d is too big!,force to %d \n",
					omx_videoenc_component_Private->act_avc_param.kIntraPeroid, MAX_IDRPERIOD_AVC);
				omx_videoenc_component_Private->act_avc_param.kIntraPeroid = MAX_IDRPERIOD_AVC;
			}
			if(omx_videoenc_component_Private->act_avc_param.kIntraPeroid < MIN_IDRPERIOD_AVC)
			{
				DEBUG(DEB_LEV_ERR,"Warning! nBFrames+nPFrames:%d is too small!,force to %d \n",
					omx_videoenc_component_Private->act_avc_param.kIntraPeroid, MIN_IDRPERIOD_AVC);
				omx_videoenc_component_Private->act_avc_param.kIntraPeroid = MIN_IDRPERIOD_AVC;
			}
		}
		else
			omx_videoenc_component_Private->act_avc_param.kIntraPeroid = WFD_IDRPERIOD_AVC;
		omx_videoenc_component_Private->act_avc_param.b_cabac = pAvcType->bEntropyCodingCABAC;
		omx_videoenc_component_Private->mng_info.i_bframes = omx_videoenc_component_Private->act_avc_param.i_bframes;

		pPort->pprofile.eProfile = pAvcType->eProfile;
		pPort->pprofile.eLevel = pAvcType->eLevel;

		memcpy(&pPort->pavctype,pAvcType,sizeof(OMX_VIDEO_PARAM_AVCTYPE));
		break;
	}

	case OMX_IndexParamVideoProfileLevelCurrent:
	{
		OMX_VIDEO_PARAM_PROFILELEVELTYPE *pprofile = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)ComponentParameterStructure;
		portIndex = pprofile->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pprofile, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		int i_profile = 0,i_level = 0;
		switch(pprofile->eProfile)
		{
		case OMX_VIDEO_AVCProfileBaseline:
			i_profile = 0;
			break;
		case OMX_VIDEO_AVCProfileMain:
			i_profile = 1;
			break;
		case OMX_VIDEO_AVCProfileHigh:
			i_profile = 2;
			break;
		default :
			return OMX_ErrorBadParameter;
			break;
		};

		switch(pprofile->eLevel)
		{
		case OMX_VIDEO_AVCLevel1:
			i_level = 10; break;
		case OMX_VIDEO_AVCLevel1b:
			i_level = 9; break;
		case OMX_VIDEO_AVCLevel11:
			i_level = 11; break;
		case OMX_VIDEO_AVCLevel12:
			i_level = 12; break;
		case OMX_VIDEO_AVCLevel13:
			i_level = 13; break;
		case OMX_VIDEO_AVCLevel2:
			i_level = 20; break;
		case OMX_VIDEO_AVCLevel21:
			i_level = 21; break;
		case OMX_VIDEO_AVCLevel22:
			i_level = 22; break;
		case OMX_VIDEO_AVCLevel3:
			i_level = 23; break;
		case OMX_VIDEO_AVCLevel31:
			i_level = 31; break;
		case OMX_VIDEO_AVCLevel32:
			i_level = 32; break;
		case OMX_VIDEO_AVCLevel4:
			i_level = 40; break;
		case OMX_VIDEO_AVCLevel41:
			i_level = 41; break;
		case OMX_VIDEO_AVCLevel42:
			i_level = 42; break;
		case OMX_VIDEO_AVCLevel5:
			i_level = 50; break;
		case OMX_VIDEO_AVCLevel51:
			i_level = 51; break;
		default :
			return OMX_ErrorBadParameter;
			break;
		};

		omx_videoenc_component_Private->act_avc_param.i_profile = i_profile;
		omx_videoenc_component_Private->act_avc_param.i_level_idc = i_level;

		pPort->pavctype.eProfile = pprofile->eProfile;
		pPort->pavctype.eLevel = pprofile->eLevel;

		memcpy(&pPort->pprofile, pprofile, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
		break;
	}

	case OMX_IndexParamVideoQuantization:
	{
		OMX_VIDEO_PARAM_QUANTIZATIONTYPE *pquanty = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)(ComponentParameterStructure);
		portIndex = pquanty->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pquanty, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(&pPort->pquanty, pquanty, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
		break;
	}

	case OMX_IndexParamQFactor:
	{
		OMX_IMAGE_PARAM_QFACTORTYPE *Iquanty = (OMX_IMAGE_PARAM_QFACTORTYPE *)(ComponentParameterStructure);
		portIndex = Iquanty->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, Iquanty, sizeof(OMX_IMAGE_PARAM_QFACTORTYPE));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		if( /*(Iquanty->nQFactor < 0) || */(Iquanty->nQFactor > 100))
		{
			DEBUG(DEB_LEV_ERR, "err!nQFactor Error:%d!%s,%d\n", Iquanty->nQFactor, __FILE__, __LINE__);
			return OMX_ErrorBadParameter;
		}

		omx_videoenc_component_Private->act_jpg_param.jpg_quality = Iquanty->nQFactor;

		memcpy(&pPort->pIquanty, Iquanty, sizeof(OMX_IMAGE_PARAM_QFACTORTYPE));
		break;
	}

	case OMX_IndexParameterStoreMediaData:
	{
		StoreMetaDataInBuffersParams *pMediaStoreType = (StoreMetaDataInBuffersParams *)ComponentParameterStructure;
		portIndex = pMediaStoreType->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pMediaStoreType, sizeof(StoreMetaDataInBuffersParams));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];

		if(portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX)
		{
			omx_videoenc_component_Private->mng_info.b_store_in_video[0] = pMediaStoreType->bStoreMetaData;
		}

		else if(portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
#ifdef enable_gralloc
			/* output port not support StoreMetaData */
			if(pMediaStoreType->bStoreMetaData != OMX_FALSE)
			{
				DEBUG(DEB_LEV_ERR,"err!the output port can not support StoreMetaData!%s,%d\n", __FILE__, __LINE__);
				return OMX_ErrorBadParameter;
			}
#endif
			omx_videoenc_component_Private->mng_info.b_store_in_video[1] = pMediaStoreType->bStoreMetaData;

			if(pMediaStoreType->bStoreMetaData)
			{
				omx_videoenc_component_Private->mng_info.ringbuf = OMX_FALSE;
				pPort->ringbuffer = OMX_FALSE;
			}
		}
		else
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pPort->bIsStoreMediaData = pMediaStoreType->bStoreMetaData;
		break;
	}

	case OMX_ACT_IndexParamMVCTYPE:
	{
		OMX_ACTIONS_Params *pActParam = (OMX_ACTIONS_Params *)ComponentParameterStructure;
		portIndex = pActParam->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pActParam, sizeof(OMX_ACTIONS_Params));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if( portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		omx_videoenc_component_Private->act_avc_param.b_mvc = pActParam->bEnable;
		omx_videoenc_component_Private->mng_info.bmvc = pActParam->bEnable;

		pPort->pMVC = pActParam->bEnable;
		break;
	}

	case OMX_ACT_IndexParmaTsPacket:
	{
		OMX_ACT_PARAM_TsPacketType *pActTsPacket = (OMX_ACT_PARAM_TsPacketType *)ComponentParameterStructure;
		portIndex = pActTsPacket->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pActTsPacket, sizeof(OMX_ACT_PARAM_TsPacketType));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if( portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		if(pActTsPacket->TsPacketType == OMX_TsPacket_Disable)
		{
			omx_videoenc_component_Private->act_prp_param.i_ts_en = 0;
		}
		else if (pActTsPacket->TsPacketType == OMX_TsPacket_NoBlu)
		{
			omx_videoenc_component_Private->act_prp_param.i_ts_en = 1;
		}
		else if(pActTsPacket->TsPacketType == OMX_TsPacket_WithBlu)
		{
			omx_videoenc_component_Private->act_prp_param.i_ts_en = 2;
		}
		else
		{
			DEBUG(DEB_LEV_ERR, "err!TsPacketType Error:%x!%s,%d\n", pActTsPacket->TsPacketType, __FILE__, __LINE__);
			return OMX_ErrorBadParameter;
		}

		DEBUG(DEB_LEV_PARAMS,"i_ts_en:%d\n", omx_videoenc_component_Private->act_prp_param.i_ts_en);

		memcpy(&pPort->pActTsPacket, pActTsPacket, sizeof(OMX_ACT_PARAM_TsPacketType));
		break;
	}

	case OMX_ACT_IndexParamThumbControl:
	{
		OMX_ACT_PARAM_THUMBPARAM *pVideoTumb = (OMX_ACT_PARAM_THUMBPARAM *)ComponentParameterStructure;
		portIndex = pVideoTumb->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pVideoTumb, sizeof(OMX_ACT_PARAM_THUMBPARAM));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		if((pVideoTumb->nWidth > 128) || (pVideoTumb->nHeight > 128))
		{
			DEBUG(DEB_LEV_ERR, "err!Tumb Size Error,w:%d,h:%d!%s,%d\n", pVideoTumb->nWidth, pVideoTumb->nHeight, __FILE__, __LINE__);
			return OMX_ErrorBadParameter;
		}

		omx_videoenc_component_Private->act_jpg_param.i_thumb_w = pVideoTumb->nWidth;
		omx_videoenc_component_Private->act_jpg_param.i_thumb_h = pVideoTumb->nHeight;
		omx_videoenc_component_Private->act_jpg_param.b_thumb = pVideoTumb->bThumbEnable;

		memcpy(&pPort->pConfigTumb, pVideoTumb, sizeof(OMX_ACT_PARAM_THUMBPARAM));
		break;
	}

	case OMX_IndexParamRingBuff:
	{
		OMX_ACTIONS_Params *pActParam = (OMX_ACTIONS_Params *)ComponentParameterStructure;
		portIndex = pActParam->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pActParam, sizeof(OMX_ACTIONS_Params));
		if(err != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!ParameterSanityCheck Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if( portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		if(pActParam->bEnable)
		{
			omx_videoenc_component_Private->mng_info.b_store_in_video[1] = OMX_FALSE;
			pPort->bIsStoreMediaData = OMX_FALSE;

			omx_videoenc_component_Private->mng_info.ringbuf = OMX_TRUE;
			pPort->ringbuffer = OMX_TRUE;

			OMX_U32 old_nBufferCountActual = 0, j;
			old_nBufferCountActual = pPort->sPortParam.nBufferCountActual;
			DEBUG(DEB_LEV_ERR,"portIndex is 1,old_nBufferCountActual:%d ,new_nBufferCountActual:%d \n", old_nBufferCountActual, DEFAULT_RingBuffers_NO);
			pPort->sPortParam.nBufferCountActual = DEFAULT_RingBuffers_NO;
			if ((omx_videoenc_component_Private->state == OMX_StateIdle ||
				omx_videoenc_component_Private->state == OMX_StatePause  ||
				omx_videoenc_component_Private->state == OMX_StateExecuting) &&
				(pPort->sPortParam.nBufferCountActual > old_nBufferCountActual))
			{
				if(pPort->pInternalBufferStorage)
				{
					pPort->pInternalBufferStorage = realloc(pPort->pInternalBufferStorage,pPort->sPortParam.nBufferCountActual * sizeof(OMX_BUFFERHEADERTYPE *));
				}

				if(pPort->bBufferStateAllocated)
				{
					pPort->bBufferStateAllocated = realloc(pPort->bBufferStateAllocated,pPort->sPortParam.nBufferCountActual * sizeof(BUFFER_STATUS_FLAG));
					for(j=0; j < pPort->sPortParam.nBufferCountActual; j++)
					{
						pPort->bBufferStateAllocated[j] = BUFFER_FREE;
					}
				}
			}
		}
		else
		{
			omx_videoenc_component_Private->mng_info.ringbuf = OMX_FALSE;
			pPort->ringbuffer = OMX_FALSE;
		}
		break;
	}

	case OMX_IndexConfigVideoIntraVOPRefresh:
	case OMX_IndexConfigCommonInputCrop:
	case OMX_IndexConfigVideoAVCIntraPeriod:
	case OMX_IndexConfigVideoFramerate:
	case OMX_IndexConfigVideoBitrate:
	case OMX_ACT_IndexConfig_FACEDETECTION:
	case OMX_ACT_IndexParmaFaceDet:
	case OMX_ACT_IndexParamExifControl:
	case OMX_ACT_IndexConfigMediaFormatConvert:
		return omx_videoenc_component_SetConfig(hComponent, nParamIndex, ComponentParameterStructure);

	default: /*Call the base component function*/
		return omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
	}

	return err;
}

OMX_ERRORTYPE omx_videoenc_component_GetParameter(
	OMX_HANDLETYPE hComponent,
	OMX_INDEXTYPE nParamIndex,
	OMX_PTR ComponentParameterStructure)
{
	OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
	omx_videoenc_PrivateType *omx_videoenc_component_Private = openmaxStandComp->pComponentPrivate;
	omx_videoenc_PortType *pPort;
	OMX_U32  portIndex;
	OMX_PARAM_COMPONENTROLETYPE *pComponentRole;

	if (ComponentParameterStructure == NULL) {
		return OMX_ErrorBadParameter;
	}
	DEBUG(DEB_LEV_SIMPLE_SEQ, "Getting parameter %i\n", nParamIndex);
	/* Check which structure we are being fed and fill its header */

	switch((int)nParamIndex)
	{
	case OMX_IndexParamVideoInit:
	{
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone) 
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n",err, __FILE__,__LINE__);
			break;
		}

		memcpy(ComponentParameterStructure, &omx_videoenc_component_Private->sPortTypesParam[OMX_PortDomainVideo], sizeof(OMX_PORT_PARAM_TYPE));
		break;
	}

	case OMX_IndexParamVideoPortFormat:
	{
		pVideoPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		VIDEO_ENC_PORT_FORMAT_TYPE *pSupportedVideoEncParamFormatType;
		OMX_U32 nNumberOfSupported = 0;
		portIndex = pVideoPortFormat->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];

		if(portIndex == OMX_BASE_FILTER_INPUTPORT_INDEX)
		{
			pSupportedVideoEncParamFormatType = SupportedVideoEncParamFormatType_InPort;
			nNumberOfSupported = sizeof(SupportedVideoEncParamFormatType_InPort) / sizeof(VIDEO_ENC_PORT_FORMAT_TYPE);
		}
		else if(portIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			pSupportedVideoEncParamFormatType = SupportedVideoEncParamFormatType_OutPort;
			nNumberOfSupported = sizeof(SupportedVideoEncParamFormatType_OutPort) / sizeof(VIDEO_ENC_PORT_FORMAT_TYPE);
		}
		else if(portIndex == OMX_BASE_FILTER_SYNCPORT_INDEX)
		{
			pSupportedVideoEncParamFormatType = SupportedVideoEncParamFormatType_SyncPort;
			nNumberOfSupported = sizeof(SupportedVideoEncParamFormatType_SyncPort) / sizeof(VIDEO_ENC_PORT_FORMAT_TYPE);
		}
		else
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		if(pVideoPortFormat->nIndex >= nNumberOfSupported)
		{
			DEBUG(DEB_LEV_ERR, "Warning!VideoPortFormat is no more!%s,%d\n", __FILE__, __LINE__);
			return OMX_ErrorNoMore;
		}
		pSupportedVideoEncParamFormatType += pVideoPortFormat->nIndex ;

		if(pSupportedVideoEncParamFormatType->eCompressionFormat != (OMX_U32)(-1))
		{
			pVideoPortFormat->eCompressionFormat = pSupportedVideoEncParamFormatType->eCompressionFormat;
			pVideoPortFormat->eColorFormat = pSupportedVideoEncParamFormatType->eColorFormat;
			pVideoPortFormat->xFramerate = pSupportedVideoEncParamFormatType->xFramerate;
		}
		else
		{
			DEBUG(DEB_LEV_ERR, "Warning!eCompressionFormat is no more!%s,%d\n", __FILE__, __LINE__);
			err = OMX_ErrorNoMore;
		}
		break;
	}

	case OMX_IndexParamStandardComponentRole:
	{
		pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n",err, __FILE__,__LINE__);
			break;
		}

		if (omx_videoenc_component_Private->video_encoding_type == OMX_VIDEO_EnCoding_AVC)
		{
			strcpy((char *) pComponentRole->cRole, VIDEO_ENC_COMP_ROLE);
		}
		else if (omx_videoenc_component_Private->video_encoding_type == OMX_VIDEO_EnCoding_MJPEG)
		{
			strcpy((char *) pComponentRole->cRole, JPEG_ENC_COMP_ROLE);
		}
		else if (omx_videoenc_component_Private->video_encoding_type == OMX_VIDEO_EnCoding_PREVIEW)
		{
			strcpy((char *) pComponentRole->cRole, IPP_ENC_COMP_ROLE);
		}
		else
		{
			strcpy((char *) pComponentRole->cRole, "\0");;
		}
		break;
	}

	case OMX_IndexParamVideoBitrate:
	{
		OMX_VIDEO_PARAM_BITRATETYPE *pVideoBitrate = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_PARAM_BITRATETYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n",err, __FILE__,__LINE__);
			break;
		}

		portIndex = pVideoBitrate->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n",portIndex,__FILE__,__LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(pVideoBitrate, &pPort->pbitrate, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
		break;
	}

	case OMX_IndexParamVideoAvc:
	{
		OMX_VIDEO_PARAM_AVCTYPE *pAvcType = (OMX_VIDEO_PARAM_AVCTYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_PARAM_AVCTYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n",err, __FILE__,__LINE__);
			break;
		}

		portIndex = pAvcType->nPortIndex;
		pPort = (omx_videoenc_PortType *) omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(pAvcType, &pPort->pavctype, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
		break;
	}

	case OMX_IndexParamVideoProfileLevelCurrent:
	{
		OMX_VIDEO_PARAM_PROFILELEVELTYPE *pprofile = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pprofile->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(pprofile, &pPort->pprofile, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
		break;
	}

	case OMX_IndexParamVideoQuantization:
	{
		OMX_VIDEO_PARAM_QUANTIZATIONTYPE *pquanty = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pquanty->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(pquanty, &pPort->pquanty, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
		break;
	}

	case OMX_IndexParamVideoProfileLevelQuerySupported:
	{
		VIDEO_PROFILE_LEVEL_TYPE *pProfileLevel = SupportedAVCProfileLevels;
		OMX_VIDEO_PARAM_PROFILELEVELTYPE *pParamProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}
		OMX_U32 nNumberOfProfiles = sizeof(SupportedAVCProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);

		if(pParamProfileLevel->nPortIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", pParamProfileLevel->nPortIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

#ifndef _OPENMAX_V1_2_
		if(pParamProfileLevel->nProfileIndex >= nNumberOfProfiles)
		{
			DEBUG(DEB_LEV_ERR, "Warning!nProfileIndex is no more!%s,%d\n", __FILE__, __LINE__);
			return OMX_ErrorNoMore;
		}
		pProfileLevel += pParamProfileLevel->nProfileIndex;
#else
		if(pParamProfileLevel->nIndex >= nNumberOfProfiles)
		{
			return OMX_ErrorNoMore;
		}
		pProfileLevel += pParamProfileLevel->nIndex;
#endif

		/* -1 indicates end of table */
		if(pProfileLevel->nProfile != -1)
		{
			pParamProfileLevel->eProfile = pProfileLevel->nProfile;
			pParamProfileLevel->eLevel = pProfileLevel->nLevel;
			err = OMX_ErrorNone;
		}
		else
		{
			DEBUG(DEB_LEV_ERR, "Warning!nProfile is no more!%s,%d\n", __FILE__, __LINE__);
			err = OMX_ErrorNoMore;
		}
		break;
	}

	case OMX_IndexParamQFactor:
	{
		OMX_IMAGE_PARAM_QFACTORTYPE *Iquanty = (OMX_IMAGE_PARAM_QFACTORTYPE *)(ComponentParameterStructure);
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_IMAGE_PARAM_QFACTORTYPE))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = Iquanty->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(Iquanty, &pPort->pIquanty, sizeof(OMX_IMAGE_PARAM_QFACTORTYPE));
		break;
	}

	case OMX_IndexParameterStoreMediaData:
	{
		StoreMetaDataInBuffersParams *pMediaStoreType  = (StoreMetaDataInBuffersParams *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(StoreMetaDataInBuffersParams))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pMediaStoreType->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];		
		if((portIndex != OMX_BASE_FILTER_INPUTPORT_INDEX) ||  
			(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX))
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pMediaStoreType->bStoreMetaData = pPort->bIsStoreMediaData;
		break;
	}

	case OMX_ACT_IndexParamMVCTYPE:
	{
		OMX_ACTIONS_Params *pActParam = (OMX_ACTIONS_Params *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_ACTIONS_Params))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pActParam->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pActParam->bEnable = pPort->pMVC;
		break;
	}

	case OMX_ACT_IndexParmaTsPacket:
	{
		OMX_ACT_PARAM_TsPacketType *pActTsPacket = (OMX_ACT_PARAM_TsPacketType *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_ACT_PARAM_TsPacketType))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pActTsPacket->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(pActTsPacket, &pPort->pActTsPacket, sizeof(OMX_ACT_PARAM_TsPacketType));
		break;
	}

	case OMX_ACT_IndexParamThumbControl: /**< reference: OMX_ACT_CONFIG_THUMBPARAM */
	{
		OMX_ACT_PARAM_THUMBPARAM *pVideoTumb = (OMX_ACT_PARAM_THUMBPARAM *)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_ACT_PARAM_THUMBPARAM))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		portIndex = pVideoTumb->nPortIndex;
		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if(portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		memcpy(pVideoTumb, &pPort->pConfigTumb, sizeof(OMX_ACT_PARAM_THUMBPARAM));
		break;
	}

	case OMX_IndexParamRingBuff:
	{
		OMX_ACTIONS_Params *pActParam = (OMX_ACTIONS_Params *)ComponentParameterStructure;
		portIndex = pActParam->nPortIndex;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_ACT_PARAM_EXIFPARAM))) != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR, "err!checkHeader Error %x!%s,%d\n", err, __FILE__, __LINE__);
			break;
		}

		pPort = (omx_videoenc_PortType *)omx_videoenc_component_Private->ports[portIndex];
		if( portIndex != OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			DEBUG(DEB_LEV_ERR, "err!Port index Error:%d!%s,%d\n", portIndex, __FILE__, __LINE__);
			return OMX_ErrorBadPortIndex;
		}

		pActParam->bEnable = pPort->ringbuffer;
		break;
	}

	case OMX_IndexConfigVideoIntraVOPRefresh:
	case OMX_IndexConfigCommonInputCrop:
	case OMX_IndexConfigVideoAVCIntraPeriod:
	case OMX_IndexConfigVideoFramerate:
	case OMX_IndexConfigVideoBitrate:
	case OMX_ACT_IndexConfig_FACEDETECTION:
	case OMX_ACT_IndexParmaFaceDet:
	case OMX_ACT_IndexParamExifControl:
	case OMX_ACT_IndexConfigMediaFormatConvert:
		return omx_videoenc_component_GetConfig(hComponent, nParamIndex, ComponentParameterStructure);

	default: /*Call the base component function*/
		return omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
	}

	return err;
}

/* fix for android CTS.4.4, CTS4.4 need last-1 frame as EOS like last one, so we need buffer a frame at least. */
static void MyOutputReturn(omx_videoenc_PortType *pOutPort, queue_t *pOutputReturnQueue, OMX_BUFFERHEADERTYPE *pOutputBuffer)
{
	OMX_BUFFERHEADERTYPE *pOutputReturnTmp;

	if(getquenelem(pOutputReturnQueue) >= 1)
	{
		queue(pOutputReturnQueue, pOutputBuffer);
		pOutputReturnTmp = dequeue(pOutputReturnQueue);
		
		if((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
		{
			if(pOutputBuffer->nFilledLen == 0)
				pOutputReturnTmp->nFlags |= OMX_BUFFERFLAG_EOS;

			pOutputReturnTmp->nFlags |= OMX_BUFFERFLAG_ACTZEROOFFSET;
			pOutPort->ReturnBufferFunction((omx_base_PortType *)pOutPort, pOutputReturnTmp);
			pOutputReturnTmp = NULL;

			pOutputReturnTmp = dequeue(pOutputReturnQueue);
			if(pOutputReturnTmp != pOutputBuffer) 
			{
				DEBUG(DEB_LEV_ERR, "Warning!Unreasonable(%p,%p,%d)!%s,%d\n",
					pOutputReturnTmp, pOutputBuffer, getquenelem(pOutputReturnQueue), __FILE__, __LINE__);
			}
			pOutputBuffer->nFlags |= OMX_BUFFERFLAG_ACTZEROOFFSET;
			pOutPort->ReturnBufferFunction((omx_base_PortType *)pOutPort, pOutputBuffer);
		}
		else
		{
			pOutputReturnTmp->nFlags |= OMX_BUFFERFLAG_ACTZEROOFFSET;
			pOutPort->ReturnBufferFunction((omx_base_PortType *)pOutPort, pOutputReturnTmp);
		}
	}
	else
	{
		if((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
		{
			pOutputBuffer->nFlags |= OMX_BUFFERFLAG_ACTZEROOFFSET;
			pOutPort->ReturnBufferFunction((omx_base_PortType *)pOutPort, pOutputBuffer);
		}
		else
		{
			queue(pOutputReturnQueue, pOutputBuffer);
		}
	}
}

/* same as MyOutputReturn() */
static void MyOutputReturn_Flush(omx_videoenc_PortType *pOutPort, queue_t *pOutputReturnQueue)
{
	int i;
	OMX_BUFFERHEADERTYPE *pOutputReturnTmp;
	int n = getquenelem(pOutputReturnQueue);

	if(n >= 1)
	{
		for(i = 0; i < n; i++)
		{
			pOutputReturnTmp = dequeue(pOutputReturnQueue);
			if(pOutputReturnTmp != NULL)
			{
				pOutputReturnTmp->nFlags |= OMX_BUFFERFLAG_ACTZEROOFFSET;
				pOutPort->ReturnBufferFunction((omx_base_PortType*)pOutPort, pOutputReturnTmp);
				pOutputReturnTmp = NULL;
			}
			else
			{
				DEBUG(DEB_LEV_ERR, "Warning!Unreasonable(%p,%d)!%s,%d\n", pOutputReturnTmp, n, __FILE__, __LINE__);
			}
		}
	}
}

/** This is the central function for component processing. It
  * is executed in a separate thread, is synchronized with
  * semaphores at each port, those are released each time a new buffer
  * is available on the given port.
  */
void *omx_videoenc_BufferMgmtFunction(void *param)
{
	OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)param;
	omx_videoenc_PrivateType *omx_videoenc_Private = (omx_videoenc_PrivateType *)openmaxStandComp->pComponentPrivate;
	omx_videoenc_PortType *pInPort=(omx_videoenc_PortType *)omx_videoenc_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	omx_videoenc_PortType *pOutPort=(omx_videoenc_PortType *)omx_videoenc_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	tsem_t *pInputSem = pInPort->pBufferSem;
	tsem_t *pOutputSem = pOutPort->pBufferSem;
	queue_t *pInputQueue = pInPort->pBufferQueue;
	queue_t *pOutputQueue = pOutPort->pBufferQueue;	
	queue_t *pOutputReturnQueue = omx_videoenc_Private->mng_info.queue_strm_return;
	
	int enable_returnquece = 0;
	OMX_BUFFERHEADERTYPE *pOutputBuffer = NULL;
	OMX_BUFFERHEADERTYPE *pInputBuffer = NULL;
	OMX_VCE_Buffers_List *pInBuffersMng_List = &(pInPort->BuffersMng_List);
	OMX_VCE_Buffers_List *pOutBuffersMng_List = &(pOutPort->BuffersMng_List);

	omx_videoenc_PortType *pSynPort=(omx_videoenc_PortType *)omx_videoenc_Private->ports[OMX_BASE_FILTER_SYNCPORT_INDEX];
	tsem_t *pSyncSem = pSynPort->pBufferSem;
	OMX_BUFFERHEADERTYPE *pOutputBuffer_process = NULL;
	OMX_VCE_Buffers_List *pSyncBuffersMng_List = &(pSynPort->BuffersMng_List);

	OMX_BOOL isEncodePrepared = OMX_FALSE;
	OMX_BOOL isFaceDetPrepared = OMX_FALSE;
	omx_camera_frame_metadata_t *fd_info;
	enc_param_t enc_param;
	int nPrePare_Num = 0;
	int try_ret = 0;
	int init_ret;

	omx_videoenc_Private->bellagioThreads->nThreadBufferMngtID = (long int)syscall(__NR_gettid);
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s of component %p\n", __func__, openmaxStandComp);
	DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s the thread ID is %i\n", __func__, (int)omx_videoenc_Private->bellagioThreads->nThreadBufferMngtID);
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
	prctl(PR_SET_NAME, (unsigned long)"OMXVCE_BUFMNG", 0, 0, 0); //thread name
	omx_videoenc_Private->mng_info.pInputSem = pInputSem;
	omx_videoenc_Private->mng_info.pOutputSem = pOutputSem;
	omx_videoenc_Private->mng_info.queue_video = pInputQueue;
	omx_videoenc_Private->mng_info.queue_out = pOutputQueue;
	omx_videoenc_Private->mng_info.pInBuffersMng_List = pInBuffersMng_List;
	omx_videoenc_Private->mng_info.pOutBuffersMng_List = pOutBuffersMng_List;
	omx_videoenc_Private->mng_info.ringbuf = pOutPort->ringbuffer;
	omx_videoenc_Private->mng_info.bufferpool = pOutPort->bufferpool;
	omx_videoenc_Private->mng_info.ringbuf_size = pOutPort->sPortParam.nBufferSize;

	/* init */
	DEBUG(DEB_LEV_SIMPLE_SEQ, "mng_init b4!%s %d\n", __func__, __LINE__);
	enc_param.enc_codec = omx_videoenc_Private->enc_codec;
	enc_param.enc_mode = omx_videoenc_Private->enc_mode;
	enc_param.h264_param = &omx_videoenc_Private->act_avc_param;
	enc_param.jpg_param = &omx_videoenc_Private->act_jpg_param;
	enc_param.prp_param = &omx_videoenc_Private->act_prp_param;
	init_ret = mng_init(&omx_videoenc_Private->mng_info,(void *)&enc_param);
	if(init_ret == -1)
	{
		DEBUG(DEB_LEV_ERR, "err!mng_init fail!%s,%d\n", __FILE__, __LINE__);
		DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s of component %p\n", __func__, openmaxStandComp);

		(*(omx_videoenc_Private->callbacks->EventHandler))
			(openmaxStandComp,
			omx_videoenc_Private->callbackData,
			OMX_EventError, 
			OMX_ErrorUndefined,
			0,
			NULL);

		return NULL;
	}
	DEBUG(DEB_LEV_SIMPLE_SEQ,"mng_init aft!%s %d\n", __func__, __LINE__);

	/* fix for CTS4.4 of H264 code (need buffer a frame) */
	enable_returnquece = (omx_videoenc_Private->enc_codec == ENC_H264) && (omx_videoenc_Private->enc_mode == ENC_ENCODE);
	if(enable_returnquece)
		DEBUG(DEB_LEV_ERR,"info!openmaxStandComp:%p, enable_returnquece:%d\n", openmaxStandComp, enable_returnquece);
    
	/* checks if the component is in a state able to receive buffers */
	while(omx_videoenc_Private->state == OMX_StateIdle || omx_videoenc_Private->state == OMX_StateExecuting ||  omx_videoenc_Private->state == OMX_StatePause ||
		omx_videoenc_Private->transientState == OMX_TransStateLoadedToIdle)
	{
		/* Wait till the ports are being flushed */
		pthread_mutex_lock(&omx_videoenc_Private->flush_mutex);

		/* flush Port! */
		while(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort) || PORT_IS_BEING_FLUSHED(pSynPort))
		{
				pthread_mutex_unlock(&omx_videoenc_Private->flush_mutex);

				DEBUG(DEB_LEV_FULL_SEQ, "In %s 1 signaling flush all cond iSemVal=%d,oSemval=%d\n",
					__func__, pInputSem->semval, pOutputSem->semval);

				DEBUG(DEB_LEV_SIMPLE_SEQ,"PORT_IS_BEING_FLUSHED!%d,%d\n",PORT_IS_BEING_FLUSHED(pInPort), PORT_IS_BEING_FLUSHED(pSynPort));
				if(omx_videoenc_Private->mng_info.bexit == 0)
					omx_videoenc_Private->mng_info.bexit = 1;
					
				if(enable_returnquece)
					MyOutputReturn_Flush(pOutPort, pOutputReturnQueue);

				while(omx_videoenc_Private->mng_info.bexit != MMM_FNG_EXIT)
				{
					DEBUG(DEB_LEV_SIMPLE_SEQ, "mng_get!int to loop\n");
					pOutputBuffer = NULL;
					try_ret = mng_get(&omx_videoenc_Private->mng_info, (OMX_PTR *)&pInputBuffer, (OMX_PTR *)&pOutputBuffer);
					if(try_ret == -1)
					{
						DEBUG(DEB_LEV_ERR, "Warning!mng_get fail!%s,%d\n", __FILE__, __LINE__);
					}

					if(pOutputBuffer)
					{
						pOutPort->ReturnBufferFunction((omx_base_PortType *)pOutPort, pOutputBuffer);
						pOutputBuffer = NULL;
					}

					if(pInputBuffer)
					{
						pInPort->ReturnBufferFunction((omx_base_PortType *)pInPort, pInputBuffer);
						pInputBuffer = NULL;
					}
				}

				omx_videoenc_Private->mng_info.bexit = 0;
				omx_videoenc_Private->isFlushed = OMX_TRUE;
				DEBUG(DEB_LEV_FULL_SEQ, "In %s 2 signaling flush all cond iSemVal=%d,oSemval=%d\n",
					__func__, pInputSem->semval, pOutputSem->semval);

				DEBUG(DEB_LEV_SIMPLE_SEQ,"b4 base_port_FlushProcessingBuffers:%d\n", omx_videoenc_Private->flush_all_condition->semval);
				tsem_up(omx_videoenc_Private->flush_all_condition);
				DEBUG(DEB_LEV_SIMPLE_SEQ,"aft base_port_FlushProcessingBuffers:%d\n", omx_videoenc_Private->flush_all_condition->semval);
				DEBUG(DEB_LEV_SIMPLE_SEQ, "b42 base_port_FlushProcessingBuffers:%d\n", omx_videoenc_Private->flush_condition->semval);
				tsem_down(omx_videoenc_Private->flush_condition);
				DEBUG(DEB_LEV_SIMPLE_SEQ, "aft2 base_port_FlushProcessingBuffers:%d\n", omx_videoenc_Private->flush_condition->semval);
				pthread_mutex_lock(&omx_videoenc_Private->flush_mutex);
		}
		pthread_mutex_unlock(&omx_videoenc_Private->flush_mutex);

		nPrePare_Num = 0;
		mng_cmd(&omx_videoenc_Private->mng_info, GET_PREPARE_FRAMES, (void *)&nPrePare_Num);

		/* No buffer to process. So wait here */
		if((isEncodePrepared == OMX_FALSE && (pInputSem->semval == 0 || pOutputSem->semval == 0) && nPrePare_Num == 0) &&
			(omx_videoenc_Private->state != OMX_StateLoaded && omx_videoenc_Private->state != OMX_StateInvalid))
		{
			tsem_down(omx_videoenc_Private->bMgmtSem);
		}

		/* exit bufferMng thread */
		if(omx_videoenc_Private->state == OMX_StateLoaded || omx_videoenc_Private->state == OMX_StateInvalid)
		{
			omx_videoenc_Private->MNG_EXIT = OMX_TRUE;
			DEBUG(DEB_LEV_ERR, "In %s 3  Buffer Management Thread is exiting\n", __func__);
			break;
		}

		/* pOutPort is enable? */
		if(PORT_IS_ENABLED(pOutPort))
			omx_videoenc_Private->mng_info.outport_enable = 1;
		else
			omx_videoenc_Private->mng_info.outport_enable = 0;
		DEBUG(DEB_LEV_PARAMS, "outport_enable:%d\n", omx_videoenc_Private->mng_info.outport_enable);

		/* try start */
		if(omx_videoenc_Private->mng_info.b_encoded == 0)
		{
			/* H264 */
			if((omx_videoenc_Private->enc_codec == ENC_H264) && (omx_videoenc_Private->enc_mode == ENC_ENCODE))
			{
				if(omx_videoenc_Private->mng_info.get_avc_info_ready == 0)
				{
					omx_videoenc_Private->mng_info.enc_frame->avc_info_mode = MOD_ONLY_PPS_SPS;
					try_ret = mng_get_avc_info(&omx_videoenc_Private->mng_info, (OMX_PTR *)&pOutputBuffer);
					if(try_ret == 0)
					{
						omx_videoenc_Private->mng_info.get_avc_info_ready = 1;
						if(omx_videoenc_Private->isIDRBefore == TRUE)
							omx_videoenc_Private->mng_info.enc_frame->avc_info_mode = MOD_INCLUDE_PPS_SPS;
						else
							omx_videoenc_Private->mng_info.enc_frame->avc_info_mode = MOD_EXECEPT_PPS_SPS;

						if(pOutputBuffer)
						{
							if(enable_returnquece)
								MyOutputReturn(pOutPort,pOutputReturnQueue,pOutputBuffer);
							else
								pOutPort->ReturnBufferFunction((omx_base_PortType *)pOutPort, pOutputBuffer);
							pOutputBuffer = NULL;
						}
					}
					else
					{
						DEBUG(DEB_LEV_ERR, "Warning!mng_get_avc_info fail!%s,%d\n", __FILE__, __LINE__);
					}
				}
				else
				{
					try_ret = mng_try_encode(&omx_videoenc_Private->mng_info, (OMX_PTR *)&pInputBuffer, (OMX_PTR *)&pOutputBuffer);
					if(try_ret == 0)
					{
						isEncodePrepared = OMX_TRUE;
						isFaceDetPrepared = OMX_TRUE;
						if(omx_videoenc_Private->isIDRBefore == FALSE)
							omx_videoenc_Private->mng_info.enc_frame->avc_info_mode = MOD_EXECEPT_PPS_SPS;
						else
							omx_videoenc_Private->mng_info.enc_frame->avc_info_mode = MOD_INCLUDE_PPS_SPS;
					}
					else
					{
						if(pOutputBuffer) 
						{
							pOutputBuffer->nFilledLen = 0;
						}
					}
					DEBUG(DEB_LEV_FULL_SEQ,"%s,%d,mng_try_encode1:%d,%p,%p\n", __func__, __LINE__, try_ret, pInputBuffer, pOutputBuffer);
				}
			}
			/* JPEG & preview */
			else
			{
				omx_videoenc_Private->mng_info.enc_frame->avc_info_mode = MOD_INCLUDE_PPS_SPS;
				try_ret = mng_try_encode(&omx_videoenc_Private->mng_info, (OMX_PTR *)&pInputBuffer, (OMX_PTR *)&pOutputBuffer);
				if(try_ret == 0)
				{
					isEncodePrepared = OMX_TRUE;
					isFaceDetPrepared = OMX_TRUE;
				}
				else
				{
					if(pOutputBuffer) 
					{
						pOutputBuffer->nFilledLen = 0;
					}
				}
				DEBUG(DEB_LEV_FULL_SEQ,"%s,%d,mng_try_encode2:%d,%p,%p\n", __func__, __LINE__, try_ret, pInputBuffer, pOutputBuffer);
			}
		}

		if(isEncodePrepared == OMX_TRUE && omx_videoenc_Private->bIsEOSReached == OMX_FALSE)
		{
			if(pInputBuffer->hMarkTargetComponent != NULL)
			{
				DEBUG(DEB_LEV_ERR,"%s,%d\n", __func__, __LINE__);
				if((OMX_COMPONENTTYPE *)pInputBuffer->hMarkTargetComponent == (OMX_COMPONENTTYPE *)openmaxStandComp)
				{
					/* Clear the mark and generate an event */
					(*(omx_videoenc_Private->callbacks->EventHandler))
						(openmaxStandComp,
						omx_videoenc_Private->callbackData,
						OMX_EventMark, 
						1,
						0,
						pInputBuffer->pMarkData);
				}
				else
				{
					/* If this is not the target component then pass the mark */
					omx_videoenc_Private->pMark.hMarkTargetComponent = pInputBuffer->hMarkTargetComponent;
					omx_videoenc_Private->pMark.pMarkData = pInputBuffer->pMarkData;
				}
				pInputBuffer->hMarkTargetComponent = NULL;
			}
		}

		if(isEncodePrepared == OMX_TRUE && omx_videoenc_Private->bIsEOSReached == OMX_FALSE)
		{
			if(omx_videoenc_Private->mng_info.outport_enable)
			{
				if(omx_videoenc_Private->pMark.hMarkTargetComponent != NULL)
				{
					DEBUG(DEB_LEV_ERR,"%s,%d\n",__func__,__LINE__);
					pOutputBuffer->hMarkTargetComponent = omx_videoenc_Private->pMark.hMarkTargetComponent;
					pOutputBuffer->pMarkData = omx_videoenc_Private->pMark.pMarkData;
					omx_videoenc_Private->pMark.hMarkTargetComponent = NULL;
					omx_videoenc_Private->pMark.pMarkData = NULL;
				}
			}

			if(1)//if(omx_videoenc_Private->state == OMX_StateExecuting)
			{
				pOutputBuffer = NULL;
				DEBUG(DEB_LEV_SIMPLE_SEQ, "b4 mng_get\n");
				try_ret = mng_get(&omx_videoenc_Private->mng_info, (OMX_PTR *)&pInputBuffer, (OMX_PTR *)&pOutputBuffer);
				DEBUG(DEB_LEV_SIMPLE_SEQ, "aft mng_get\n");
				if(try_ret == -1)
				{
					DEBUG(DEB_LEV_ERR, "Warning!mng_get fail!%s,%d\n", __FILE__, __LINE__);

					if(pInputBuffer)
						pInputBuffer->nFilledLen = 0;
					if(pOutputBuffer)
						pOutputBuffer->nTimeStamp = pInputBuffer->nTimeStamp;
					isEncodePrepared = OMX_FALSE;
				}
				else
				{
					pInputBuffer->nFilledLen = 0;
					if(pOutputBuffer)
						pOutputBuffer->nTimeStamp = pInputBuffer->nTimeStamp;
					isEncodePrepared = OMX_FALSE;
				}
			}
			else if(!(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort)))
			{
				DEBUG(DEB_LEV_ERR, "In %s Received Buffer in non-Executing State(%x)\n", __func__, (int)omx_videoenc_Private->state);
			}
			else
			{
				pInputBuffer->nFilledLen = 0;
			}

			if(pInputBuffer && ((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS && pInputBuffer->nFilledLen == 0))
			{
				DEBUG(DEB_LEV_ERR, "Detected EOS flags in input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);

				if(pOutputBuffer)pOutputBuffer->nFlags = pInputBuffer->nFlags;
				OMX_U32 nFlags = pInputBuffer->nFlags;
				(*(omx_videoenc_Private->callbacks->EventHandler))
					(openmaxStandComp,
					omx_videoenc_Private->callbackData,
					OMX_EventBufferFlag,
					1,
					nFlags,
					NULL);

				omx_videoenc_Private->bIsEOSReached = OMX_TRUE;
			}

			/* get face detection result, sync_port(out port 2) */
			if( PORT_IS_ENABLED(pSynPort) && omx_videoenc_Private->FD_Enable == OMX_TRUE  \
				&& isFaceDetPrepared == OMX_TRUE && pSynPort->pBufferQueue->nelem > 0)
			{
				int fd_ret = mng_cmd(&omx_videoenc_Private->mng_info, GET_FD_INFO, (void *)&fd_info);
				if(fd_ret == 0)
				{
					pOutputBuffer_process = dequeue(pSynPort->pBufferQueue);
					if(pOutputBuffer_process)
					{
						tsem_down(pSyncSem);

						DEBUG(DEB_LEV_PARAMS, "return face info buffer now....%d\n", pSyncSem->semval);
						void *pSync_VirAddr =  Get_VirAddr_BuffersMng(pSyncBuffersMng_List, pOutputBuffer_process, OMX_FALSE);
						if( pSync_VirAddr == NULL)
						{
							DEBUG(DEB_LEV_ERR,"err!Get Addr of pOutputBuffer_process fail!%s,%d\n", __FILE__, __LINE__);
						}
						else
						{
							/* buffer like this: | omx_camera_frame_metadata_t | omx_camera_face_t(1) | omx_camera_face_t(2) | ... | ... | */
							memcpy(pSync_VirAddr, fd_info, sizeof(omx_camera_frame_metadata_t));
							*(unsigned int *)((unsigned long)pSync_VirAddr + sizeof(omx_camera_frame_metadata_t) - sizeof(void *)) = (unsigned long)pSync_VirAddr + sizeof(omx_camera_frame_metadata_t);
							memcpy(((unsigned char *)pSync_VirAddr) + sizeof(omx_camera_frame_metadata_t), fd_info->faces, sizeof(omx_camera_face_t) * fd_info->number_of_faces);
							pOutputBuffer_process->nFilledLen = sizeof(omx_camera_frame_metadata_t) + sizeof(omx_camera_face_t) * fd_info->number_of_faces;
							pOutputBuffer_process->nOffset = 0;
						}

						pSynPort->ReturnBufferFunction((omx_base_PortType *)pSynPort, pOutputBuffer_process);
						pOutputBuffer_process = NULL;
					}
				}

				isFaceDetPrepared = OMX_FALSE;
			}

			if(omx_videoenc_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort)))
			{
				/* Waiting at paused state */
				tsem_wait(omx_videoenc_Private->bStateSem);
			}

			/* If EOS and Input buffer Filled Len Zero then Return output buffer immediately */
			if(omx_videoenc_Private->mng_info.outport_enable)
			{
				DEBUG(DEB_LEV_PARAMS, "pOutputBuffer:%p nFilledLen:%d\n", pOutputBuffer, pOutputBuffer->nFilledLen);
				if(pOutputBuffer)
				{
					if(enable_returnquece)
						MyOutputReturn(pOutPort, pOutputReturnQueue, pOutputBuffer);
					else
						pOutPort->ReturnBufferFunction((omx_base_PortType *)pOutPort, pOutputBuffer);
					pOutputBuffer = NULL;
				}
			}
		}

		if(omx_videoenc_Private->state == OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort)))
		{
			/* Waiting at paused state */
			tsem_wait(omx_videoenc_Private->bStateSem);
		}
		
		if(pInputBuffer && ((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS && pInputBuffer->nFilledLen == 0))
		{
			DEBUG(DEB_LEV_ERR, "Detected EOS flags in input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);

			 /* send callback */
			if(pOutputBuffer)
				pOutputBuffer->nFlags = pInputBuffer->nFlags;
			OMX_U32 nFlags = pInputBuffer->nFlags;
			(*(omx_videoenc_Private->callbacks->EventHandler))
				(openmaxStandComp,
				omx_videoenc_Private->callbackData,
				OMX_EventBufferFlag,
				1,
				nFlags,
				NULL);

			omx_videoenc_Private->bIsEOSReached = OMX_TRUE;
			if(pOutputBuffer)
			{
				pOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
				if(enable_returnquece)
					MyOutputReturn(pOutPort,pOutputReturnQueue, pOutputBuffer);
				else
					pOutPort->ReturnBufferFunction((omx_base_PortType *)pOutPort, pOutputBuffer);
				pOutputBuffer = NULL;
			}
		}
		
		/* Input Buffer has been completely consumed. So, return input buffer */
		if(pInputBuffer && pInputBuffer->nFilledLen==0)
		{
			pInPort->ReturnBufferFunction((omx_base_PortType *)pInPort, pInputBuffer);
			pInputBuffer = NULL;
		}
	}

	/* deinit */
	DEBUG(DEB_LEV_SIMPLE_SEQ, "mng_deinit b4!%s %d\n", __func__, __LINE__);
	mng_deinit(&omx_videoenc_Private->mng_info);
	DEBUG(DEB_LEV_SIMPLE_SEQ, "mng_deinit aft!%s %d\n", __func__, __LINE__);

	omx_videoenc_Private->MNG_EXIT = OMX_TRUE;
	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s of component %p\n", __func__, openmaxStandComp);
	return NULL;
}

