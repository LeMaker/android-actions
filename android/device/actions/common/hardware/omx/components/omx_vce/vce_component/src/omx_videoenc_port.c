/**
  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

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

#include <string.h>
#include <unistd.h>
#include <omxcore.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include "omx_videoenc_port.h"
#include "omx_malloc.h"
#include "video_mediadata.h"
#include "vce_cfg.h"
#include "log.h"

/**
  * @brief The base contructor for the generic OpenMAX ST Video port
  *
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the component.
  * It takes care of constructing the instance of the port and
  * every object needed by the base port.
  *
  * @param openmaxStandComp pointer to the Handle of the component
  * @param openmaxStandPort the ST port to be initialized
  * @param nPortIndex Index of the port to be constructed
  * @param isInput specifices if the port is an input or an output
  *
  * @return OMX_ErrorInsufficientResources if a memory allocation fails
  */

OSCL_EXPORT_REF OMX_ERRORTYPE videoenc_port_Constructor(
OMX_COMPONENTTYPE *openmaxStandComp,
omx_base_PortType **openmaxStandPort,
OMX_U32 nPortIndex,
OMX_BOOL isInput)
{
	unsigned int i;
	OMX_ERRORTYPE   err = OMX_ErrorNone;
	omx_videoenc_PortType *omx_videoenc_Port;

	if (!(*openmaxStandPort))
	{
		*openmaxStandPort = calloc(1,sizeof (omx_videoenc_PortType));
	}

	if (!(*openmaxStandPort)) {
		return OMX_ErrorInsufficientResources;
	}
	memset(*openmaxStandPort,0,sizeof (omx_videoenc_PortType));

	err = base_video_port_Constructor(openmaxStandComp,openmaxStandPort,nPortIndex, isInput);
	if(err != OMX_ErrorNone){
	   DEBUG(DEB_LEV_ERR, "err!The video port constructor failed in %s,%d\n", __func__,__LINE__);
		return err;
	}

	omx_videoenc_Port = (omx_videoenc_PortType *)(*openmaxStandPort);
	
	if(isInput == OMX_TRUE)
	{
		//intport
		omx_videoenc_Port->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingUnused;
		omx_videoenc_Port->sVideoParam.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
		omx_videoenc_Port->sVideoParam.xFramerate = (DEFAULT_FRAMERATE_AVC<<16);

		omx_videoenc_Port->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
		omx_videoenc_Port->sPortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
		omx_videoenc_Port->sPortParam.format.video.xFramerate = (DEFAULT_FRAMERATE_AVC<<16);
#ifndef _OPENMAX_V1_2_
		strcpy(omx_videoenc_Port->sPortParam.format.video.cMIMEType, CMIMEType_Video_Raw);
#endif
	}
	else if(isInput == OMX_FALSE && nPortIndex == 1)
	{
		//outport
		omx_videoenc_Port->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingAVC;
		omx_videoenc_Port->sVideoParam.eColorFormat = OMX_COLOR_FormatUnused;
		omx_videoenc_Port->sVideoParam.xFramerate = (DEFAULT_FRAMERATE_AVC<<16);

		omx_videoenc_Port->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
		omx_videoenc_Port->sPortParam.format.video.eColorFormat = OMX_COLOR_FormatUnused;
		omx_videoenc_Port->sPortParam.format.video.xFramerate = (DEFAULT_FRAMERATE_AVC<<16);
#ifndef _OPENMAX_V1_2_
		strcpy(omx_videoenc_Port->sPortParam.format.video.cMIMEType, CMIMEType_Video_Avc);
#endif
	}
	else
	{
		//sync port
		omx_videoenc_Port->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingUnused;
		omx_videoenc_Port->sVideoParam.eColorFormat = OMX_COLOR_FormatUnused;
		omx_videoenc_Port->sVideoParam.xFramerate = (DEFAULT_FRAMERATE_AVC<<16);

		omx_videoenc_Port->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
		omx_videoenc_Port->sPortParam.format.video.eColorFormat = OMX_COLOR_FormatUnused;
		omx_videoenc_Port->sPortParam.format.video.xFramerate = (DEFAULT_FRAMERATE_AVC<<16);
#ifndef _OPENMAX_V1_2_
		strcpy(omx_videoenc_Port->sPortParam.format.video.cMIMEType, CMIMEType_Video_FaceDet);
#endif
	}

	omx_videoenc_Port->bIsStoreMediaData = OMX_FALSE;
	omx_videoenc_Port->pMVC = OMX_FALSE;
	omx_videoenc_Port->bconfig_changed = OMX_FALSE;

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pquanty,OMX_VIDEO_PARAM_QUANTIZATIONTYPE,nPortIndex);
	omx_videoenc_Port->pquanty.nQpI = 24;
	omx_videoenc_Port->pquanty.nQpP = 24;
	omx_videoenc_Port->pquanty.nQpB = 24;

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pconfig_avcperiod, OMX_VIDEO_CONFIG_AVCINTRAPERIOD,nPortIndex);
	omx_videoenc_Port->pconfig_avcperiod.nPFrames = 30;

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pbitrate, OMX_VIDEO_PARAM_BITRATETYPE,nPortIndex);
	omx_videoenc_Port->pbitrate.eControlRate = OMX_Video_ControlRateDisable;
	omx_videoenc_Port->pbitrate.nTargetBitrate = 0;

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pprofile, OMX_VIDEO_PARAM_PROFILELEVELTYPE,nPortIndex);
	omx_videoenc_Port->pprofile.eLevel = OMX_VIDEO_AVCLevel31;
	omx_videoenc_Port->pprofile.eProfile = OMX_VIDEO_AVCProfileBaseline;
#ifndef _OPENMAX_V1_2_
	omx_videoenc_Port->pprofile.nProfileIndex = 0;
#else
	omx_videoenc_Port->pprofile.nIndex = 0;
#endif

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pconfig_framerate, OMX_CONFIG_FRAMERATETYPE,nPortIndex);
	omx_videoenc_Port->pconfig_framerate.xEncodeFramerate = (DEFAULT_FRAMERATE_AVC<<16);

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pconfig_bitrate, OMX_VIDEO_CONFIG_BITRATETYPE,nPortIndex);
	omx_videoenc_Port->pconfig_bitrate.nEncodeBitrate = 0;

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pconfig_crop, OMX_CONFIG_RECTTYPE,nPortIndex);

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pconfig_vopfresh, OMX_CONFIG_INTRAREFRESHVOPTYPE,nPortIndex);
	omx_videoenc_Port->pconfig_vopfresh.IntraRefreshVOP = OMX_FALSE;

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pavctype, OMX_VIDEO_PARAM_AVCTYPE,nPortIndex);
	omx_videoenc_Port->pavctype.nBFrames = 0;
	omx_videoenc_Port->pavctype.bEntropyCodingCABAC = OMX_TRUE;
	omx_videoenc_Port->pavctype.bUseHadamard = OMX_TRUE;
	omx_videoenc_Port->pavctype.nRefFrames = 1;
	omx_videoenc_Port->pavctype.eProfile = OMX_VIDEO_AVCProfileBaseline;
	omx_videoenc_Port->pavctype.eLevel = OMX_VIDEO_AVCLevel31;
	omx_videoenc_Port->pavctype.nAllowedPictureTypes = (OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP);


	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pConfigTumb, OMX_ACT_PARAM_THUMBPARAM,nPortIndex);
	omx_videoenc_Port->pConfigTumb.nWidth = 64;
	omx_videoenc_Port->pConfigTumb.nHeight = 64;
	omx_videoenc_Port->pConfigTumb.bThumbEnable = OMX_FALSE;

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pExifInfo, OMX_ACT_PARAM_EXIFPARAM,nPortIndex);
	{
		int mgpsLATH[3] = {22,18,0}; /*纬度*/
		int mgpsLATL[3] = {1,1,1};
		int mgpsLONGH[3] = {113,31,0};
		int mgpsLONGL[3] = {1,1,1};
		int mgpsTimeH[3] = {16,45,12};
		int mgpsTimeL[3] = {1,1,1};

		omx_videoenc_Port->pExifInfo.bExifEnable = OMX_FALSE;
		omx_videoenc_Port->pExifInfo.ImageOri = 1;
		omx_videoenc_Port->pExifInfo.dataTime = (char*)malloc(128);
		strcpy(omx_videoenc_Port->pExifInfo.dataTime,"2012.9.11");
		omx_videoenc_Port->pExifInfo.exifmake = (char*)malloc(128);
		strcpy(omx_videoenc_Port->pExifInfo.exifmake,"tsh");
		omx_videoenc_Port->pExifInfo.exifmodel = (char*)malloc(128);
		strcpy(omx_videoenc_Port->pExifInfo.exifmodel,"act_exif_model");
		omx_videoenc_Port->pExifInfo.gpsDate = (char*)malloc(128);
		strcpy(omx_videoenc_Port->pExifInfo.gpsDate,"2012.9.12");
		omx_videoenc_Port->pExifInfo.gpsprocessMethod = (char*)malloc(128);
		strcpy(omx_videoenc_Port->pExifInfo.gpsprocessMethod,"act_gps_model");
		//omx_videoenc_Port->pExifInfo.dataTime = "2012.9.11";
		//omx_videoenc_Port->pExifInfo.exifmake = "tsh";
		//omx_videoenc_Port->pExifInfo.exifmodel = "act_exif_model";
		//omx_videoenc_Port->pExifInfo.gpsDate = "2012.9.12";
		//omx_videoenc_Port->pExifInfo.gpsprocessMethod = "act_gps_model";
		omx_videoenc_Port->pExifInfo.bGPS = OMX_TRUE;
		omx_videoenc_Port->pExifInfo.focalLengthH = 720;
		omx_videoenc_Port->pExifInfo.focalLengthL = 1;

		for (i = 0; i < 3; i++)
		{
			omx_videoenc_Port->pExifInfo.gpsLATH[i] = mgpsLATH[i];
			omx_videoenc_Port->pExifInfo.gpsLATL[i] = mgpsLATL[i];
			omx_videoenc_Port->pExifInfo.gpsLONGH[i] = mgpsLONGH[i];
			omx_videoenc_Port->pExifInfo.gpsLONGL[i] = mgpsLONGL[i];
			omx_videoenc_Port->pExifInfo.gpsTimeH[i] = mgpsTimeH[i];
			omx_videoenc_Port->pExifInfo.gpsTimeL[i] = mgpsTimeL[i];
		}
	}

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pActTsPacket, OMX_ACT_PARAM_TsPacketType,nPortIndex);
	omx_videoenc_Port->pActTsPacket.TsPacketType = OMX_TsPacket_Disable;

	OMX_PARAMETER_INIT_STRUCT(&omx_videoenc_Port->pIquanty, OMX_IMAGE_PARAM_QFACTORTYPE,nPortIndex);
	omx_videoenc_Port->pIquanty.nQFactor = 80;

	OMX_VCE_Buffer_Info* pList = omx_videoenc_Port->BuffersMng_List.pList;
	omx_videoenc_Port->BuffersMng_List.Actual_Buffers = 0;
	for (i = 0; i < MAX_VCE_MNG_Buffers; i++)
	{
		pList[i].IsValid = OMX_FALSE;
		pList[i].InPutAddr  = NULL;
		pList[i].PhyAddr  = NULL;
		pList[i].VirAddr  = NULL;
		pList[i].Ion_Size = 0;
	}

#if Enable_RingBuffer
	omx_videoenc_Port->bufferpool = NULL;
	omx_videoenc_Port->ringbuffer = OMX_FALSE;
	omx_videoenc_Port->ringbuf_framesize = 100000; /*单位Bytes*/
#endif

	omx_videoenc_Port->PortDestructor = &videoenc_port_Destructor;
	omx_videoenc_Port->FlushProcessingBuffers = &videoenc_port_FlushProcessingBuffers;
	omx_videoenc_Port->Port_AllocateBuffer = &videoenc_port_AllocateBuffer;
	omx_videoenc_Port->Port_UseBuffer = &videoenc_port_UseBuffer;
	omx_videoenc_Port->Port_FreeBuffer = &videoenc_port_FreeBuffer;
	omx_videoenc_Port->Port_SendBufferFunction = &videoenc_port_SendBufferFunction;
	omx_videoenc_Port->ReturnBufferFunction = &videoenc_port_ReturnBufferFunction;

	return OMX_ErrorNone;
}

/**
  * @brief The base video port destructor for the generic OpenMAX ST Video port
  *
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the port.
  * It takes care of destructing the instance of the port
  *
  * @param openmaxStandPort the ST port to be destructed
  *
  * @return OMX_ErrorNone
  */

OSCL_EXPORT_REF OMX_ERRORTYPE videoenc_port_Destructor(omx_base_PortType *openmaxStandPort)
{
	omx_videoenc_PortType *omx_videoenc_Port = (omx_videoenc_PortType *)(openmaxStandPort);

#ifndef _OPENMAX_V1_2_
  if(openmaxStandPort->sPortParam.format.video.cMIMEType) {
    free(openmaxStandPort->sPortParam.format.video.cMIMEType);
    openmaxStandPort->sPortParam.format.video.cMIMEType = NULL;
  }
#endif

  if(omx_videoenc_Port->pExifInfo.dataTime)
  {
	  free(omx_videoenc_Port->pExifInfo.dataTime);
	  omx_videoenc_Port->pExifInfo.dataTime = NULL;
  }
  if(omx_videoenc_Port->pExifInfo.exifmake)
  {
	  free(omx_videoenc_Port->pExifInfo.exifmake);
	  omx_videoenc_Port->pExifInfo.exifmake = NULL;
  }
  if(omx_videoenc_Port->pExifInfo.exifmodel)
  {
	  free(omx_videoenc_Port->pExifInfo.exifmodel);
	  omx_videoenc_Port->pExifInfo.exifmodel = NULL;
  }
  if(omx_videoenc_Port->pExifInfo.gpsDate)
  {
	  free(omx_videoenc_Port->pExifInfo.gpsDate);
	  omx_videoenc_Port->pExifInfo.gpsDate = NULL;
  }
  if(omx_videoenc_Port->pExifInfo.gpsprocessMethod)
  {
	  free(omx_videoenc_Port->pExifInfo.gpsprocessMethod);
	  omx_videoenc_Port->pExifInfo.gpsprocessMethod = NULL;
  }
  
  base_video_port_Destructor(openmaxStandPort);

  return OMX_ErrorNone;
}

/** @brief Releases buffers under processing.
 * This function must be implemented in the derived classes, for the
 * specific processing
 */
OMX_ERRORTYPE videoenc_port_FlushProcessingBuffers(omx_base_PortType *openmaxStandPort)
{
	omx_videoenc_PortType *omx_videoenc_Port = (omx_videoenc_PortType*)openmaxStandPort;
    OMX_VCE_Buffers_List* pBuffersMng_List= &(omx_videoenc_Port->BuffersMng_List);
	omx_base_component_PrivateType* omx_base_component_Private;
	OMX_BUFFERHEADERTYPE* pBuffer;
	int errQue;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, openmaxStandPort);
	omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandPort->standCompContainer->pComponentPrivate;

#if Enable_RingBuffer
	if(omx_videoenc_Port->ringbuffer == OMX_TRUE &&  omx_videoenc_Port->bufferpool)
	{
		clearBufferPool(omx_videoenc_Port->bufferpool);
	}
#endif

	if(openmaxStandPort->sPortParam.eDomain!=OMX_PortDomainOther)
	{
		/* clock buffers not used in the clients buffer managment function */
		pthread_mutex_lock(&omx_base_component_Private->flush_mutex);
		openmaxStandPort->bIsPortFlushed=OMX_TRUE;

		/*Signal the buffer management thread of port flush,if it is waiting for buffers*/
		if(omx_base_component_Private->bMgmtSem->semval==0) {
			tsem_up(omx_base_component_Private->bMgmtSem);
		}

		if(omx_base_component_Private->state != OMX_StateExecuting ) {
			/*Waiting at paused state*/
			tsem_signal(omx_base_component_Private->bStateSem);
		}
		DEBUG(DEB_LEV_FULL_SEQ, "In %s waiting for flush all condition port index =%d\n", __func__,(int)openmaxStandPort->sPortParam.nPortIndex);

		/* Wait until flush is completed */
		pthread_mutex_unlock(&omx_base_component_Private->flush_mutex);
		tsem_down(omx_base_component_Private->flush_all_condition);
	}
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s flushed all the buffers under processing\n", __func__);

	tsem_reset(omx_base_component_Private->bMgmtSem);

	/* Flush all the buffers not under processing */
	while (openmaxStandPort->pBufferSem->semval > 0)
	{
		DEBUG(DEB_LEV_FULL_SEQ, "In %s TFlag=%x Flusing Port=%d,Semval=%d Qelem=%d\n",
			__func__,(int)openmaxStandPort->nTunnelFlags,(int)openmaxStandPort->sPortParam.nPortIndex,
			(int)openmaxStandPort->pBufferSem->semval,(int)openmaxStandPort->pBufferQueue->nelem);

		tsem_down(openmaxStandPort->pBufferSem);
		pBuffer = dequeue(openmaxStandPort->pBufferQueue);

		//FlushProcessingBuffers_BuffersMng(pBuffersMng_List,pBuffer,omx_videoenc_Port->bIsStoreMediaData);

		if (PORT_IS_TUNNELED(openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER(openmaxStandPort))
		{
			DEBUG(DEB_LEV_FULL_SEQ, "In %s: Comp %s is returning io:%d buffer\n",
				__func__,omx_base_component_Private->name,(int)openmaxStandPort->sPortParam.nPortIndex);

			if (openmaxStandPort->sPortParam.eDir == OMX_DirInput)
			{
				((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
			}
			else
			{
				((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
			}
		}
		else if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort))
		{
			errQue = queue(openmaxStandPort->pBufferQueue,pBuffer);
			if (errQue)
			{
				/* /TODO the queue is full. This can be handled in a fine way with
				* some retrials, or other checking. For the moment this is a critical error
				* and simply causes the failure of this call
				*/
				return OMX_ErrorInsufficientResources;
			}
		}
		else
		{
			//here?
			(*(openmaxStandPort->BufferProcessedCallback))(
				openmaxStandPort->standCompContainer,
				omx_base_component_Private->callbackData,
				pBuffer);
		}
	}

	/*Port is tunneled and supplier and didn't received all it's buffer then wait for the buffers*/
	if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort))
	{
		while(openmaxStandPort->pBufferQueue->nelem != (int)openmaxStandPort->nNumAssignedBuffers)
		{
			tsem_down(openmaxStandPort->pBufferSem);
			DEBUG(DEB_LEV_PARAMS, "In %s Got a buffer qelem=%d\n",__func__,openmaxStandPort->pBufferQueue->nelem);
		}
		tsem_reset(openmaxStandPort->pBufferSem);
	}

	pthread_mutex_lock(&omx_base_component_Private->flush_mutex);
	openmaxStandPort->bIsPortFlushed = OMX_FALSE;
	pthread_mutex_unlock(&omx_base_component_Private->flush_mutex);

	tsem_up(omx_base_component_Private->flush_condition);

	DEBUG(DEB_LEV_FULL_SEQ, "Out %s Port Index=%d bIsPortFlushed=%d Component %s\n", __func__,
		(int)openmaxStandPort->sPortParam.nPortIndex,(int)openmaxStandPort->bIsPortFlushed,omx_base_component_Private->name);

	DEBUG(DEB_LEV_PARAMS, "In %s TFlag=%x Qelem=%d BSem=%d bMgmtsem=%d component=%s\n", __func__,
		(int)openmaxStandPort->nTunnelFlags,
		(int)openmaxStandPort->pBufferQueue->nelem,
		(int)openmaxStandPort->pBufferSem->semval,
		(int)omx_base_component_Private->bMgmtSem->semval,
		omx_base_component_Private->name);

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out %s Port %p Index=%d\n", __func__, openmaxStandPort, (int)openmaxStandPort->sPortParam.nPortIndex);
	return OMX_ErrorNone;
}



/** @brief Called by the standard allocate buffer, it implements a base functionality.
 *
 * This function can be overriden if the allocation of the buffer is not a simply alloc call.
 * The parameters are the same as the standard function, except for the handle of the port
 * instead of the handler of the component
 * When the buffers needed by this port are all assigned or allocated, the variable
 * bIsFullOfBuffers becomes equal to OMX_TRUE
 */
OMX_ERRORTYPE videoenc_port_AllocateBuffer(
  omx_base_PortType *openmaxStandPort,
  OMX_BUFFERHEADERTYPE** pBuffer,
  OMX_U32 nPortIndex,
  OMX_PTR pAppPrivate,
  OMX_U32 nSizeBytes)
{
  unsigned int i;
  OMX_ERRORTYPE  err = OMX_ErrorNone;
  OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
  omx_videoenc_PortType *omx_videoenc_Port = (omx_videoenc_PortType*)openmaxStandPort;
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
  OMX_BUFFERHEADERTYPE* pBufferStorage_ACTEXT = NULL;
  OMX_VCE_Buffers_List* pBuffersMng_List= &(omx_videoenc_Port->BuffersMng_List);
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, omx_videoenc_Port);

  if (nPortIndex != omx_videoenc_Port->sPortParam.nPortIndex) {
    return OMX_ErrorBadPortIndex;
  }

  if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(omx_videoenc_Port)) {
    return OMX_ErrorBadPortIndex;
  }

  if (omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle) {
    if (!omx_videoenc_Port->bIsTransientToEnabled) {
      DEBUG(DEB_LEV_ERR, "In %s: The port is not allowed to receive buffers\n", __func__);
      return OMX_ErrorIncorrectStateTransition;
    }
  }

  if(nSizeBytes < omx_videoenc_Port->sPortParam.nBufferSize) {
    DEBUG(DEB_LEV_ERR, "In %s: Requested Buffer Size %lu is less than Minimum Buffer Size %lu\n", __func__, nSizeBytes, omx_videoenc_Port->sPortParam.nBufferSize);
    return OMX_ErrorIncorrectStateTransition;
  }

#if Enable_RingBuffer
  if(omx_videoenc_Port->ringbuffer == OMX_TRUE && nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
  {
	  /*open ringbuffer pool*/
	  int buffsize = omx_videoenc_Port->ringbuf_framesize * omx_videoenc_Port->sPortParam.nBufferCountActual;
	  if(omx_videoenc_Port->bufferpool != NULL)
	  {
		  if(buffsize > get_poolsize(omx_videoenc_Port->bufferpool))
		  {
			  pool_dispose(omx_videoenc_Port->bufferpool);
			  omx_videoenc_Port->bufferpool = NULL;
			  if( pool_open(buffsize,&omx_videoenc_Port->bufferpool) )
			  {
				  DEBUG(DEB_LEV_ERR, "In %s: The port is not allowed to receive buffers %x\n", __func__,buffsize);
				  return OMX_ErrorInsufficientResources;
			  }
		  }
	  }
	  else
	  {
		  if( pool_open(buffsize,&omx_videoenc_Port->bufferpool) )
		  {
			  DEBUG(DEB_LEV_ERR, "In %s: The port is not allowed to receive buffers %x\n", __func__,buffsize);
			  return OMX_ErrorInsufficientResources;
		  }
	  }
  }
#endif

  for(i=0; i < omx_videoenc_Port->sPortParam.nBufferCountActual; i++)
  {
    if (omx_videoenc_Port->bBufferStateAllocated[i] == BUFFER_FREE)
	{
      omx_videoenc_Port->pInternalBufferStorage[i] = (OMX_BUFFERHEADERTYPE*)calloc(1,sizeof(OMX_BUFFERHEADERTYPE));
      if (!omx_videoenc_Port->pInternalBufferStorage[i])
	  {
		  return OMX_ErrorInsufficientResources;
      }

      setHeader(omx_videoenc_Port->pInternalBufferStorage[i], sizeof(OMX_BUFFERHEADERTYPE));

	  /* allocate the buffer */
	  pBufferStorage_ACTEXT = (OMX_BUFFERHEADERTYPE*)(omx_videoenc_Port->pInternalBufferStorage[i]);
	  pBufferStorage_ACTEXT->nAllocLen = nSizeBytes;
	  pBufferStorage_ACTEXT->pPlatformPrivate = omx_videoenc_Port;
	  pBufferStorage_ACTEXT->pAppPrivate = pAppPrivate;

#if Enable_RingBuffer
	  if(nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX &&  omx_videoenc_Port->ringbuffer == OMX_TRUE)
	  {
		  pBufferStorage_ACTEXT->pBuffer = get_poolbase(omx_videoenc_Port->bufferpool);
		  if(pBufferStorage_ACTEXT->pBuffer==NULL)
		  {
			  DEBUG(DEB_LEV_ERR, "err!get_poolbase is NULL!\n");
			  free(pBufferStorage_ACTEXT);
			  omx_videoenc_Port->pInternalBufferStorage[i] = NULL;
			  return OMX_ErrorInsufficientResources;
		  }
	  }
	  else
#endif
	  {
		  DEBUG(DEB_LEV_PARAMS, "nSizeBytes:%lu\n",nSizeBytes);
		  err = Add_AllocateBuffer_BuffersMng(pBuffersMng_List,pBufferStorage_ACTEXT,omx_videoenc_Port->bIsStoreMediaData,nSizeBytes);
		  if( err != OMX_ErrorNone)
		  {
			  DEBUG(DEB_LEV_ERR,"err!Add_AllocateBuffer_BuffersMng fail!%s,%d\n",__FILE__,__LINE__);
			  free(omx_videoenc_Port->pInternalBufferStorage[i]);
			  omx_videoenc_Port->pInternalBufferStorage[i] = NULL;
			  return err;
		  }
	  }

      *pBuffer = (OMX_BUFFERHEADERTYPE*)pBufferStorage_ACTEXT;
      omx_videoenc_Port->bBufferStateAllocated[i] = BUFFER_ALLOCATED;
      omx_videoenc_Port->bBufferStateAllocated[i] |= HEADER_ALLOCATED;
	  if (omx_videoenc_Port->sPortParam.eDir == OMX_DirInput)
	  {
		  pBufferStorage_ACTEXT->nInputPortIndex = omx_videoenc_Port->sPortParam.nPortIndex;
	  }
	  else
	  {
		  pBufferStorage_ACTEXT->nOutputPortIndex = omx_videoenc_Port->sPortParam.nPortIndex;
	  }
      omx_videoenc_Port->nNumAssignedBuffers++;
      DEBUG(DEB_LEV_PARAMS, "omx_videoenc_Port->nNumAssignedBuffers %i\n", (int)omx_videoenc_Port->nNumAssignedBuffers);

      if (omx_videoenc_Port->sPortParam.nBufferCountActual == omx_videoenc_Port->nNumAssignedBuffers)
	  {
        omx_videoenc_Port->sPortParam.bPopulated = OMX_TRUE;
        omx_videoenc_Port->bIsFullOfBuffers = OMX_TRUE;
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s nPortIndex=%d\n",__func__,(int)nPortIndex);
	    DEBUG(DEB_LEV_PARAMS, "pAllocSem!%s,%d,idx:%lu,semval:%d\n",__func__,__LINE__,omx_videoenc_Port->sPortParam.nPortIndex,omx_videoenc_Port->pAllocSem->semval);
		tsem_up(omx_videoenc_Port->pAllocSem);
      }
      DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for port %p\n", __func__, omx_videoenc_Port);
      return OMX_ErrorNone;
    }
  }

  DEBUG(DEB_LEV_ERR, "Out of %s for port %p. Error: no available buffers\n",__func__, omx_videoenc_Port);
  return OMX_ErrorInsufficientResources;
}

/** @brief Called by the standard use buffer, it implements a base functionality.
 *
 * This function can be overriden if the use buffer implicate more complicated operations.
 * The parameters are the same as the standard function, except for the handle of the port
 * instead of the handler of the component.
 * When the buffers needed by this port are all assigned or allocated, the variable
 * bIsFullOfBuffers becomes equal to OMX_TRUE
 */
OMX_ERRORTYPE videoenc_port_UseBuffer(
  omx_base_PortType *openmaxStandPort,
  OMX_BUFFERHEADERTYPE** ppBufferHdr,
  OMX_U32 nPortIndex,
  OMX_PTR pAppPrivate,
  OMX_U32 nSizeBytes,
  OMX_U8* pBuffer)
{
  unsigned int i;
  OMX_ERRORTYPE  err = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE* returnBufferHeader;
  OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
  omx_videoenc_PortType *omx_videoenc_Port = (omx_videoenc_PortType*)openmaxStandPort;
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
  OMX_BUFFERHEADERTYPE* pBufferStorage_ACTEXT = NULL;
  OMX_VCE_Buffers_List* pBuffersMng_List= &(omx_videoenc_Port->BuffersMng_List);
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, omx_videoenc_Port);

  if (nPortIndex != omx_videoenc_Port->sPortParam.nPortIndex) {
    return OMX_ErrorBadPortIndex;
  }

  if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(omx_videoenc_Port)) {
    return OMX_ErrorBadPortIndex;
  }

  if (omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle) {
    if (!omx_videoenc_Port->bIsTransientToEnabled) {
      DEBUG(DEB_LEV_ERR, "In %s: The port of Comp %s is not allowed to receive buffers\n", __func__,omx_base_component_Private->name);
      return OMX_ErrorIncorrectStateTransition;
    }
  }

  if(omx_videoenc_Port->bIsStoreMediaData == OMX_FALSE)
  {
	  if(nSizeBytes < omx_videoenc_Port->sPortParam.nBufferSize) {
		  DEBUG(DEB_LEV_ERR, "In %s: Port %d Given Buffer Size %u is less than Minimum Buffer Size %u\n", __func__, (int)nPortIndex, (int)nSizeBytes, (int)omx_videoenc_Port->sPortParam.nBufferSize);
		  return OMX_ErrorBadParameter;
	  }
  }
  else
  {
	  if(nSizeBytes < sizeof(video_metadata_t))
	  {
		  DEBUG(DEB_LEV_ERR,"err!Bad buffer sizes on StoreMediaData type%lu\n",nSizeBytes);
		  return OMX_ErrorBadParameter;
	  }
  }

  for(i=0; i < omx_videoenc_Port->sPortParam.nBufferCountActual; i++)
  {
    if (omx_videoenc_Port->bBufferStateAllocated[i] == BUFFER_FREE)
	{
      omx_videoenc_Port->pInternalBufferStorage[i] = calloc(1,sizeof(OMX_BUFFERHEADERTYPE));
      if (!omx_videoenc_Port->pInternalBufferStorage[i])
	  {
		  return OMX_ErrorInsufficientResources;
      }

      omx_videoenc_Port->bIsEmptyOfBuffers = OMX_FALSE;
	  pBufferStorage_ACTEXT = (OMX_BUFFERHEADERTYPE*)(omx_videoenc_Port->pInternalBufferStorage[i]);
      setHeader(pBufferStorage_ACTEXT, sizeof(OMX_BUFFERHEADERTYPE));
      pBufferStorage_ACTEXT->nAllocLen = nSizeBytes;
      pBufferStorage_ACTEXT->pPlatformPrivate = omx_videoenc_Port;
      pBufferStorage_ACTEXT->pAppPrivate = pAppPrivate;
      omx_videoenc_Port->bBufferStateAllocated[i] = BUFFER_ASSIGNED;
      omx_videoenc_Port->bBufferStateAllocated[i] |= HEADER_ALLOCATED;

	  err = Add_UseBuffer_BuffersMng(pBuffersMng_List,pBufferStorage_ACTEXT,omx_videoenc_Port->bIsStoreMediaData,nSizeBytes,pBuffer);
	  if( err != OMX_ErrorNone)
	  {
		  DEBUG(DEB_LEV_ERR,"err!Add_UseBuffer_BuffersMng fail!%s,%d\n",__FILE__,__LINE__);
		  free(omx_videoenc_Port->pInternalBufferStorage[i]);
		  omx_videoenc_Port->pInternalBufferStorage[i] = NULL;
		  return err;
	  }

	  returnBufferHeader = calloc(1,sizeof(OMX_BUFFERHEADERTYPE));
	  if (!returnBufferHeader)
	  {
		  return OMX_ErrorInsufficientResources;
	  }
	  setHeader(returnBufferHeader, sizeof(OMX_BUFFERHEADERTYPE));
      returnBufferHeader->pBuffer = pBuffer;
	  returnBufferHeader->nAllocLen = nSizeBytes;
      returnBufferHeader->pPlatformPrivate = omx_videoenc_Port;
      returnBufferHeader->pAppPrivate = pAppPrivate;

      if (omx_videoenc_Port->sPortParam.eDir == OMX_DirInput)
	  {
		  pBufferStorage_ACTEXT->nInputPortIndex = omx_videoenc_Port->sPortParam.nPortIndex;
		  returnBufferHeader->nInputPortIndex = omx_videoenc_Port->sPortParam.nPortIndex;
      }
	  else
	  {
		  pBufferStorage_ACTEXT->nOutputPortIndex = omx_videoenc_Port->sPortParam.nPortIndex;
		  returnBufferHeader->nOutputPortIndex = omx_videoenc_Port->sPortParam.nPortIndex;
      }

      *ppBufferHdr = returnBufferHeader;
      omx_videoenc_Port->nNumAssignedBuffers++;
      DEBUG(DEB_LEV_PARAMS, "omx_videoenc_Port->nNumAssignedBuffers %lu,%d\n", omx_videoenc_Port->sPortParam.nBufferCountActual,(int)omx_videoenc_Port->nNumAssignedBuffers);

      if (omx_videoenc_Port->sPortParam.nBufferCountActual == omx_videoenc_Port->nNumAssignedBuffers)
	  {
		  omx_videoenc_Port->sPortParam.bPopulated = OMX_TRUE;
		  omx_videoenc_Port->bIsFullOfBuffers = OMX_TRUE;

		  DEBUG(DEB_LEV_PARAMS, "pAllocSem!%s,%d,idx:%lu,semval:%d\n",__func__,__LINE__,omx_videoenc_Port->sPortParam.nPortIndex,omx_videoenc_Port->pAllocSem->semval);
		  tsem_up(omx_videoenc_Port->pAllocSem);
      }

      DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for port %p\n", __func__, omx_videoenc_Port);
      return OMX_ErrorNone;
    }
  }

  DEBUG(DEB_LEV_ERR, "In %s Error: no available buffers CompName=%s\n",__func__,omx_base_component_Private->name);
  return OMX_ErrorInsufficientResources;
}

/** @brief Called by the standard function.
 *
 * It frees the buffer header and in case also the buffer itself, if needed.
 * When all the buffers are done, the variable bIsEmptyOfBuffers is set to OMX_TRUE
 */
OMX_ERRORTYPE videoenc_port_FreeBuffer(
  omx_base_PortType *openmaxStandPort,
  OMX_U32 nPortIndex,
  OMX_BUFFERHEADERTYPE* pBuffer)
{
	unsigned int i;
	OMX_ERRORTYPE  err = OMX_ErrorNone;
	OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
	omx_videoenc_PortType *omx_videoenc_Port = (omx_videoenc_PortType*)openmaxStandPort;
	omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
	OMX_BUFFERHEADERTYPE* pBufferStorage_ACTEXT = NULL;
	OMX_VCE_Buffers_List* pBuffersMng_List= &(omx_videoenc_Port->BuffersMng_List);
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, omx_videoenc_Port);

	if (nPortIndex != omx_videoenc_Port->sPortParam.nPortIndex) {
		return OMX_ErrorBadPortIndex;
	}

	if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(omx_videoenc_Port)) {
		return OMX_ErrorBadPortIndex;
	}

	DEBUG(DEB_LEV_SIMPLE_SEQ, "cur state %x,%x\n",omx_base_component_Private->transientState,omx_videoenc_Port->bIsTransientToDisabled);

	/*此判断为了限制free的条件,要么是idle->Loaded，要么是disable port*/
	if (omx_base_component_Private->transientState != OMX_TransStateIdleToLoaded)
	{
		if (!omx_videoenc_Port->bIsTransientToDisabled)
		{
			DEBUG(DEB_LEV_FULL_SEQ, "In %s: The port is not allowed to free the buffers\n", __func__);
			(*(omx_base_component_Private->callbacks->EventHandler))
				(omxComponent,
				omx_base_component_Private->callbackData,
				OMX_EventError, /* The command was completed */
				OMX_ErrorPortUnpopulated, /* The commands was a OMX_CommandStateSet */
				nPortIndex, /* The state has been changed in message->messageParam2 */
				NULL);
		}
	}

	for(i=0; i < omx_videoenc_Port->sPortParam.nBufferCountActual; i++)
	{
		//DEBUG(DEB_LEV_PARAMS, "omx_videoenc_Port->bBufferStateAllocated[i]:%d,%x,%d\n",i,omx_videoenc_Port->bBufferStateAllocated[i],omx_videoenc_Port->sPortParam.nBufferCountActual);
		pBufferStorage_ACTEXT = (OMX_BUFFERHEADERTYPE*)(omx_videoenc_Port->pInternalBufferStorage[i]);
		if (omx_videoenc_Port->bBufferStateAllocated[i] & (BUFFER_ASSIGNED | BUFFER_ALLOCATED))
		{
			omx_videoenc_Port->bIsFullOfBuffers = OMX_FALSE;

			if((omx_videoenc_Port->bBufferStateAllocated[i] & BUFFER_ALLOCATED))
			{
				if(omx_videoenc_Port->pInternalBufferStorage[i] != pBuffer)
				{
					DEBUG(DEB_LEV_PARAMS,"ringbuf %d,%p,%p\n",omx_videoenc_Port->ringbuffer,omx_videoenc_Port->pInternalBufferStorage[i],pBuffer);
					continue;
				}
			}

#if Enable_RingBuffer
			if(omx_videoenc_Port->ringbuffer == OMX_FALSE )
#endif
			{
				if (omx_videoenc_Port->bBufferStateAllocated[i] & BUFFER_ALLOCATED)
				{
					err = Free_AllocateBuffer_BuffersMng(pBuffersMng_List,pBufferStorage_ACTEXT,omx_videoenc_Port->bIsStoreMediaData);
					if( err != OMX_ErrorNone)
					{
						DEBUG(DEB_LEV_ERR,"err!Free_AllocateBuffer_BuffersMng fail!%s,%d\n",__FILE__,__LINE__);
						return err;
					}
				}
				else if (omx_videoenc_Port->bBufferStateAllocated[i] & BUFFER_ASSIGNED)
				{
					err = Free_UseBuffer_BuffersMng(pBuffersMng_List,pBufferStorage_ACTEXT,omx_videoenc_Port->bIsStoreMediaData);
					if( err != OMX_ErrorNone)
					{
						DEBUG(DEB_LEV_ERR,"err!Free_UseBuffer_BuffersMng fail!%s,%d\n",__FILE__,__LINE__);
						return err;
					}
					if(pBuffer)free(pBuffer);
				}
			}

			if(omx_videoenc_Port->bBufferStateAllocated[i] & HEADER_ALLOCATED)
			{
				free(omx_videoenc_Port->pInternalBufferStorage[i]);
				omx_videoenc_Port->pInternalBufferStorage[i] = NULL;
			}

			omx_videoenc_Port->bBufferStateAllocated[i] = BUFFER_FREE;
			omx_videoenc_Port->nNumAssignedBuffers--;
			DEBUG(DEB_LEV_PARAMS, "omx_videoenc_Port->nNumAssignedBuffers %i\n", (int)omx_videoenc_Port->nNumAssignedBuffers);

			if (omx_videoenc_Port->nNumAssignedBuffers == 0)
			{
				if(omx_videoenc_Port->bIsStoreMediaData == OMX_TRUE)
				{
					err = Clear_StoreMedia_BuffersMng(pBuffersMng_List);
					if( err != OMX_ErrorNone)
					{
						DEBUG(DEB_LEV_ERR,"err!Clear_StoreMedia_BuffersMng fail!%s,%d\n",__FILE__,__LINE__);
						return err;
					}
				}

#if Enable_RingBuffer
				if(omx_videoenc_Port->ringbuffer == OMX_TRUE && omx_videoenc_Port->bufferpool)
				{
					pool_dispose(omx_videoenc_Port->bufferpool);
					omx_videoenc_Port->bufferpool = NULL;
				}
#endif

				omx_videoenc_Port->sPortParam.bPopulated = OMX_FALSE;
				omx_videoenc_Port->bIsEmptyOfBuffers = OMX_TRUE;
				DEBUG(DEB_LEV_PARAMS, "pAllocSem!%s,%d,idx:%lu,semval:%d\n",__func__,__LINE__,omx_videoenc_Port->sPortParam.nPortIndex,omx_videoenc_Port->pAllocSem->semval);
				tsem_up(omx_videoenc_Port->pAllocSem);
			}
			DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for port %p\n", __func__, omx_videoenc_Port);
			return OMX_ErrorNone;
		}
	}

	DEBUG(DEB_LEV_ERR, "Out of %s for port %p with OMX_ErrorInsufficientResources\n", __func__, omx_videoenc_Port);
	return OMX_ErrorInsufficientResources;
}


/** @brief the entry point for sending buffers to the port
 *
 * This function can be called by the EmptyThisBuffer or FillThisBuffer. It depends on
 * the nature of the port, that can be an input or output port.
 */
OMX_ERRORTYPE videoenc_port_SendBufferFunction(
  omx_base_PortType *openmaxStandPort,
  OMX_BUFFERHEADERTYPE* pBuffer)
{
	int errQue;
	OMX_U32 portIndex;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
	omx_videoenc_PortType *omx_videoenc_Port = (omx_videoenc_PortType*)openmaxStandPort;
	omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
	OMX_VCE_Buffers_List* pBuffersMng_List= &(omx_videoenc_Port->BuffersMng_List);
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, omx_videoenc_Port);
#if NO_GST_OMX_PATCH
	unsigned int i;
#endif
	portIndex = (omx_videoenc_Port->sPortParam.eDir == OMX_DirInput)?pBuffer->nInputPortIndex:pBuffer->nOutputPortIndex;
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s portIndex %lu\n", __func__, portIndex);

	if (portIndex != omx_videoenc_Port->sPortParam.nPortIndex)
	{
		DEBUG(DEB_LEV_ERR, "In %s: wrong port for this operation portIndex=%d port->portIndex=%d\n", __func__, (int)portIndex, (int)omx_videoenc_Port->sPortParam.nPortIndex);
		return OMX_ErrorBadPortIndex;
	}

	if(omx_base_component_Private->state == OMX_StateInvalid)
	{
		DEBUG(DEB_LEV_ERR, "In %s: we are in OMX_StateInvalid\n", __func__);
		return OMX_ErrorInvalidState;
	}

	if(omx_base_component_Private->state != OMX_StateExecuting &&
		omx_base_component_Private->state != OMX_StatePause &&
		omx_base_component_Private->state != OMX_StateIdle)
	{
		DEBUG(DEB_LEV_ERR, "In %s: we are not in executing/paused/idle state, but in %d\n", __func__, omx_base_component_Private->state);
		return OMX_ErrorIncorrectStateOperation;
	}

	if (!PORT_IS_ENABLED(omx_videoenc_Port) || (PORT_IS_BEING_DISABLED(omx_videoenc_Port) && !PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(omx_videoenc_Port)) ||
		((omx_base_component_Private->transientState == OMX_TransStateExecutingToIdle ||
		omx_base_component_Private->transientState == OMX_TransStatePauseToIdle) &&
		(PORT_IS_TUNNELED(omx_videoenc_Port) && !PORT_IS_BUFFER_SUPPLIER(omx_videoenc_Port))))
	{
		DEBUG(DEB_LEV_ERR, "In %s: Port %d is disabled comp = %s \n", __func__, (int)portIndex,omx_base_component_Private->name);
		return OMX_ErrorIncorrectStateOperation;
	}

	/* Temporarily disable this check for gst-openmax */
#if NO_GST_OMX_PATCH
	{
		OMX_BOOL foundBuffer = OMX_FALSE;
		if(pBuffer!=NULL && pBuffer->pBuffer!=NULL) {
			for(i=0; i < omx_videoenc_Port->sPortParam.nBufferCountActual; i++){
				if (pBuffer->pBuffer == omx_videoenc_Port->pInternalBufferStorage[i]->pBuffer) {
					foundBuffer = OMX_TRUE;
					break;
				}
			}
		}
		if (!foundBuffer) {
			return OMX_ErrorBadParameter;
		}
	}
#endif

	if ((err = checkHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE))) != OMX_ErrorNone)
	{
		DEBUG(DEB_LEV_ERR, "In %s: received wrong buffer header on input port\n", __func__);
		return err;
	}

	/* And notify the buffer management thread we have a fresh new buffer to manage */
	if(!PORT_IS_BEING_FLUSHED(omx_videoenc_Port) && \
		!(PORT_IS_BEING_DISABLED(omx_videoenc_Port) && PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(omx_videoenc_Port)))
	{
		//here
#if 1
		if(omx_videoenc_Port->bIsStoreMediaData == OMX_TRUE)
		{
			if((*(OMX_U32*)pBuffer->pBuffer != kMetadataBufferTypeCameraSource_act) && \
					(*(OMX_U32*)pBuffer->pBuffer != kMetadataBufferTypeGrallocSource_act))
			{
				DEBUG(DEB_LEV_ERR, "Warning!in StoreMediaData mode,but the buffer (%x) is not kMetadataBufferTypeCameraSource_act!\n",(int)*(OMX_U32*)pBuffer->pBuffer);
			}
		}
#endif

#if Enable_RingBuffer
		if(omx_videoenc_Port->ringbuffer == OMX_TRUE && omx_videoenc_Port->sPortParam.nPortIndex == OMX_BASE_FILTER_OUTPUTPORT_INDEX)
		{
			err = Free_UseRingBuffer_BuffersMng(&omx_videoenc_Port->BuffersMng_List,omx_videoenc_Port->bufferpool,pBuffer,omx_videoenc_Port->sPortParam.nBufferSize);
			if( err != OMX_ErrorNone)
			{
				DEBUG(DEB_LEV_ERR, "err!In %s: SendBuffer_BuffersMng\n", __func__);
				return err;
			}
		}
		else
#endif
		{
			err = SendBuffer_BuffersMng(pBuffersMng_List,pBuffer,omx_videoenc_Port->bIsStoreMediaData,omx_videoenc_Port->sPortParam.eDir);
			if( err != OMX_ErrorNone)
			{
				DEBUG(DEB_LEV_ERR, "err!In %s: SendBuffer_BuffersMng\n", __func__);
				return err;
			}
		}

		errQue = queue(omx_videoenc_Port->pBufferQueue, pBuffer);
		if (errQue)
		{
			/* /TODO the queue is full. This can be handled in a fine way with
			* some retrials, or other checking. For the moment this is a critical error
			* and simply causes the failure of this call
			*/
			return OMX_ErrorInsufficientResources;
		}

		tsem_up(omx_videoenc_Port->pBufferSem);
		DEBUG(DEB_LEV_PARAMS, "In %s Signalling bMgmtSem Port Index=%d\n",__func__, (int)portIndex);
		tsem_up(omx_base_component_Private->bMgmtSem);
	}
	else if(PORT_IS_BUFFER_SUPPLIER(omx_videoenc_Port))
	{
		DEBUG(DEB_LEV_FULL_SEQ, "In %s: Comp %s received io:%d buffer\n",
			__func__,omx_base_component_Private->name,(int)omx_videoenc_Port->sPortParam.nPortIndex);

		errQue = queue(omx_videoenc_Port->pBufferQueue, pBuffer);
		if (errQue)
		{
			/* /TODO the queue is full. This can be handled in a fine way with
			* some retrials, or other checking. For the moment this is a critical error
			* and simply causes the failure of this call
			*/
			return OMX_ErrorInsufficientResources;
		}
		tsem_up(omx_videoenc_Port->pBufferSem);
	}
	else
	{
		// If port being flushed and not tunneled then return error
		DEBUG(DEB_LEV_FULL_SEQ, "In %s \n", __func__);
		return OMX_ErrorIncorrectStateOperation;
	}

	DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for port %p\n", __func__, omx_videoenc_Port);
	return OMX_ErrorNone;
}

/**
 * Returns Input/Output Buffer to the IL client or Tunneled Component
 */
OMX_ERRORTYPE videoenc_port_ReturnBufferFunction(
omx_base_PortType* openmaxStandPort,
OMX_BUFFERHEADERTYPE* pBuffer)
{
  int errQue;
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  omx_base_component_PrivateType* omx_base_component_Private = openmaxStandPort->standCompContainer->pComponentPrivate;
  omx_videoenc_PortType *omx_videoenc_Port = (omx_videoenc_PortType*)openmaxStandPort;
  queue_t* pQueue = omx_videoenc_Port->pBufferQueue;
  tsem_t* pSem = omx_videoenc_Port->pBufferSem;
  OMX_VCE_Buffers_List* pBuffersMng_List= &(omx_videoenc_Port->BuffersMng_List);

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, omx_videoenc_Port);

#if 1
  OMX_BUFFERHEADERTYPE *pBuffer_ACTEXT =  (OMX_BUFFERHEADERTYPE*)pBuffer;
  if( omx_videoenc_Port->sPortParam.eDir == OMX_DirOutput )
  {
	  eError = ReturnBuffer_BuffersMng(pBuffersMng_List,pBuffer_ACTEXT,omx_videoenc_Port->bIsStoreMediaData);
	  if(eError != OMX_ErrorNone)
	  {
		  DEBUG(DEB_LEV_ERR, "err!ReturnBuffer_BuffersMng fail!%s,%d\n",__FILE__,__LINE__);
		  return eError;
	  }
  }

#ifdef enable_gralloc
  if(pBuffer->nFilledLen == 0 && ((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) )
  {
	  DEBUG(DEB_LEV_ERR, "ReturnBufferFunction,input buffer is eos now!");
  }
  else 
  {
	  eError = UnLock_VirAddr_BuffersMng(pBuffersMng_List,pBuffer_ACTEXT,omx_videoenc_Port->bIsStoreMediaData);
	  if(eError != OMX_ErrorNone)
	  {
		  DEBUG(DEB_LEV_ERR, "err!UnLock_VirAddr_BuffersMng fail!%s,%d\n",__FILE__,__LINE__);
		  return eError;
	  }
  }
#endif

#endif

  if (PORT_IS_TUNNELED(omx_videoenc_Port) &&
	  ! PORT_IS_BUFFER_SUPPLIER(omx_videoenc_Port))
  {
	  if (omx_videoenc_Port->sPortParam.eDir == OMX_DirInput)
	  {
		  pBuffer->nOutputPortIndex = omx_videoenc_Port->nTunneledPort;
		  pBuffer->nInputPortIndex = omx_videoenc_Port->sPortParam.nPortIndex;
		  eError = ((OMX_COMPONENTTYPE*)(omx_videoenc_Port->hTunneledComponent))->FillThisBuffer(omx_videoenc_Port->hTunneledComponent, pBuffer);
		  if(eError != OMX_ErrorNone)
		  {
			  DEBUG(DEB_LEV_ERR, "In %s eError %08x in FillThis Buffer from Component %s Non-Supplier\n",
				  __func__, eError,omx_base_component_Private->name);
		  }
	  }
	  else
	  {
		  pBuffer->nInputPortIndex = omx_videoenc_Port->nTunneledPort;
		  pBuffer->nOutputPortIndex = omx_videoenc_Port->sPortParam.nPortIndex;
		  eError = ((OMX_COMPONENTTYPE*)(omx_videoenc_Port->hTunneledComponent))->EmptyThisBuffer(omx_videoenc_Port->hTunneledComponent, pBuffer);
		  if(eError != OMX_ErrorNone) {
			  DEBUG(DEB_LEV_ERR, "In %s eError %08x in EmptyThis Buffer from Component %s Non-Supplier\n",
				  __func__, eError,omx_base_component_Private->name);
		  }
	  }
  }
  else if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(omx_videoenc_Port) &&
	  !PORT_IS_BEING_FLUSHED(omx_videoenc_Port))
  {
	  if (omx_videoenc_Port->sPortParam.eDir == OMX_DirInput)
	  {
		  eError = ((OMX_COMPONENTTYPE*)(omx_videoenc_Port->hTunneledComponent))->FillThisBuffer(omx_videoenc_Port->hTunneledComponent, pBuffer);
		  if(eError != OMX_ErrorNone)
		  {
			  DEBUG(DEB_LEV_FULL_SEQ, "In %s eError %08x in FillThis Buffer from Component %s Supplier\n",
				  __func__, eError,omx_base_component_Private->name);
			  /*If Error Occured then queue the buffer*/
			  errQue = queue(pQueue, pBuffer);
			  if (errQue)
			  {
				  /* /TODO the queue is full. This can be handled in a fine way with
				  * some retrials, or other checking. For the moment this is a critical error
				  * and simply causes the failure of this call
				  */
				  return OMX_ErrorInsufficientResources;
			  }

			  tsem_up(pSem);
		  }
	  }
	  else
	  {
		  eError = ((OMX_COMPONENTTYPE*)(omx_videoenc_Port->hTunneledComponent))->EmptyThisBuffer(omx_videoenc_Port->hTunneledComponent, pBuffer);
		  if(eError != OMX_ErrorNone) {
			  DEBUG(DEB_LEV_FULL_SEQ, "In %s eError %08x in EmptyThis Buffer from Component %s Supplier\n",
				  __func__, eError,omx_base_component_Private->name);
			  /*If Error Occured then queue the buffer*/
			  errQue = queue(pQueue, pBuffer);
			  if (errQue) {
				  /* /TODO the queue is full. This can be handled in a fine way with
				  * some retrials, or other checking. For the moment this is a critical error
				  * and simply causes the failure of this call
				  */
				  return OMX_ErrorInsufficientResources;
			  }
			  tsem_up(pSem);
		  }
	  }
  }
  else if (!PORT_IS_TUNNELED(omx_videoenc_Port))
  {
	  //here
	  (*(omx_videoenc_Port->BufferProcessedCallback))(
		  omx_videoenc_Port->standCompContainer,
		  omx_base_component_Private->callbackData,
		  pBuffer);
  }
  else
  {
	  errQue = queue(pQueue, pBuffer);
	  if (errQue)
	  {
		  /* /TODO the queue is full. This can be handled in a fine way with
		  * some retrials, or other checking. For the moment this is a critical error
		  * and simply causes the failure of this call
		  */
		  return OMX_ErrorInsufficientResources;
	  }
	  omx_videoenc_Port->nNumBufferFlushed++;
  }

  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for port %p\n", __func__, omx_videoenc_Port);
  return OMX_ErrorNone;
}