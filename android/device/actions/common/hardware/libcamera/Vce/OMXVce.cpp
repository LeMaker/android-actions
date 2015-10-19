#include "CameraHalDebug.h"

#include "OMXVce.h"
#include "video_mediadata.h"
#include "ErrorUtils.h"
#include "CameraHal.h"
#include "MemoryManager.h"

#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBuffer.h>

namespace android
{



#define OMXVCE_GOTO_EXIT_IF(_CONDITION,_ERROR) {    \
    if ((_CONDITION)) {                             \
        ret = (_ERROR);                             \
        goto EXIT;                                  \
    }                                               \
}

// static
OMX_ERRORTYPE OnEvent(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1,
    OMX_IN OMX_U32 nData2,
    OMX_IN OMX_PTR pEventData)
{
    OMXVce *instance = static_cast<OMXVce *>(pAppData);

    CAMHAL_LOGDB("OnEvent, pAppData=%p,eEvent=%d, nData1=%d,nData2=%d",(void *)pAppData,(int)eEvent,(int)nData1,(int)nData2);
    instance->onEvent( eEvent, nData1, nData2, pEventData);
    return OMX_ErrorNone;
}

// static
OMX_ERRORTYPE OnEmptyBufferDone(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMXVce *instance = static_cast<OMXVce *>(pAppData);
    instance->onEmptyBufferDone(pBuffer);
    return OMX_ErrorNone;
}

// static
OMX_ERRORTYPE OnFillBufferDone(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMXVce *instance = static_cast<OMXVce *>(pAppData);
    instance->onFillBufferDone(pBuffer);
    return OMX_ErrorNone;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
OMX_ERRORTYPE OMXEventSyncList::SignalEvent(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    Mutex::Autolock lock(mEventLock);
    ActUtils::Message *msg;
    LOG_FUNCTION_NAME


    if ( !mEventSignalQ.isEmpty() )
    {
        CAMHAL_LOGDA("Event queue not empty");

        for ( unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
        {
            msg = mEventSignalQ.itemAt(i);
            if ( NULL != msg )
            {
                //ActionsCode(author:liuyiguang, change_code)
                //if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                //        && ( !msg->arg1 || ( OMX_U32 ) msg->arg1 == nData1 )
                //        && ( !msg->arg2 || ( OMX_U32 ) msg->arg2 == nData2 )
                //        && msg->arg3)
                if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                        && ( !msg->arg1 || ( long ) msg->arg1 == nData1 )
                        && ( !msg->arg2 || ( long ) msg->arg2 == nData2 )
                        && msg->arg3)
                {
                    Semaphore *sem  = (Semaphore*) msg->arg3;
                    CAMHAL_LOGDA("Event matched, signalling sem");
                    mEventSignalQ.removeAt(i);
                    //Signal the semaphore provided
                    sem->Signal();
                    free(msg);
                    break;
                }
            }
        }
    }
    else
    {
        CAMHAL_LOGDA("Event queue empty!!!");
    }
    LOG_FUNCTION_NAME_EXIT

    return OMX_ErrorNone;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
OMX_ERRORTYPE OMXEventSyncList::RemoveEvent(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    Mutex::Autolock lock(mEventLock);
    ActUtils::Message *msg;

    LOG_FUNCTION_NAME

    if ( !mEventSignalQ.isEmpty() )
    {

        for ( unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
        {
            msg = mEventSignalQ.itemAt(i);
            if ( NULL != msg )
            {
                //ActionsCode(author:liuyiguang, change_code)
                //if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                //        && ( !msg->arg1 || ( OMX_U32 ) msg->arg1 == nData1 )
                //        && ( !msg->arg2 || ( OMX_U32 ) msg->arg2 == nData2 )
                //        && msg->arg3)
                if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                        && ( !msg->arg1 || ( long ) msg->arg1 == nData1 )
                        && ( !msg->arg2 || ( long ) msg->arg2 == nData2 )
                        && msg->arg3)
                {
                    Semaphore *sem  = (Semaphore*) msg->arg3;
                    CAMHAL_LOGDA("Event matched, signalling sem");
                    mEventSignalQ.removeAt(i);
                    free(msg);
                    break;
                }
            }
        }
    }
    else
    {
        CAMHAL_LOGDA("Event queue empty!!!");
    }

    LOG_FUNCTION_NAME_EXIT

    return OMX_ErrorNone;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXEventSyncList::RegisterForEvent(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN Semaphore &semaphore)
{
    status_t ret = NO_ERROR;
    ssize_t res;
    LOG_FUNCTION_NAME

    Mutex::Autolock lock(mEventLock);

    ActUtils::Message * msg = ( struct ActUtils::Message * ) malloc(sizeof(struct ActUtils::Message));
    if ( NULL != msg )
    {
        msg->command = ( unsigned int ) eEvent;
        //ActionsCode(author:liuyiguang, change_code)
        //msg->arg1 = ( void * ) nData1;
        //msg->arg2 = ( void * ) nData2;
        msg->arg1 = ( void * )(long) nData1;
        msg->arg2 = ( void * )(long) nData2;
        msg->arg3 = ( void * ) &semaphore;
        msg->arg4 =  ( void * ) hComponent;
        res = mEventSignalQ.add(msg);
        if ( NO_MEMORY == res )
        {
            CAMHAL_LOGDA("No ressources for inserting OMX events");
            free(msg);
            ret = -ENOMEM;
        }
    }

    LOG_FUNCTION_NAME_EXIT


    return ret;
}


status_t OMXEventSyncList::FlushEvent()
{
    LOG_FUNCTION_NAME

    if ( !mEventSignalQ.isEmpty() )
    {
        for (unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
        {
            ActUtils::Message *msg = mEventSignalQ.itemAt(i);
            //remove from queue and free msg
            if ( NULL != msg )
            {
                Semaphore *sem  = (Semaphore*) msg->arg3;
                sem->Signal();
                free(msg);
            }
        }
        mEventSignalQ.clear();
    }

    LOG_FUNCTION_NAME_EXIT
    return NO_ERROR;
}

OMXEventSyncList::~OMXEventSyncList()
{
    LOG_FUNCTION_NAME

    FlushEvent();

    LOG_FUNCTION_NAME_EXIT
}

OMX_CALLBACKTYPE OMXVce::mCallbacks =
{
    &OnEvent,
    &OnEmptyBufferDone,
    &OnFillBufferDone,
};



char OMXVce::OMX_VCE_NAME[] = "OMX.Action.Video.Encoder";

OMXVce::OMXVce()
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    LOG_FUNCTION_NAME

    mSync = true;

    mType = OMXVceEncoderType;
    mInputPortEnable = false;
    mOutputPortEnable = false;

    mImageProcessing = false;

    mOmxHandle = NULL;
    mComponentState = (OMX_STATETYPE)0;

    mOutputBuf = NULL;
    mOutputBufSize = 0;

    mObserver = NULL;

    mSwitchToExecSem.Create(0);
    mSwitchToIdleSem.Create(0);
    mPortEnableSem.Create(0);
    mPortDisableSem.Create(0);
    mSwitchToLoadedSem.Create(0);


    mFillBufferSyncAvilable = false;



    mInputBufferCount = 0;
    mOutputBufferCount = 0;

    mOutputBufferHandleCount = 0;

    LOG_FUNCTION_NAME_EXIT

}
OMXVce::~OMXVce()
{
    unsigned int i;

    LOG_FUNCTION_NAME

    switchToLoaded();

    freeOmxHandle();


    for(i = 0; i< mInputBufferCount; i++)
    {
        free(mInputBuffers[i]);
    }
    mInputBufferSize = 0;
    mInputBufferCount= 0;
    for(i = 0; i< mOutputBufferCount; i++)
    {
        free(mOutputBuffers[i]);
    }

    mOutputBufferSize = 0;
    mOutputBufferCount=0;


#ifdef OMX_VCE_OUTPUT_USEBUFFER
    for(i = 0; i< mOutputBufferHandleCount; i++)
    {
        GraphicBufferAllocator &GrallocAlloc = GraphicBufferAllocator::get();
        GrallocAlloc.free((buffer_handle_t)mOutputBufferHandles[i]);
    }
#endif
    mOutputBufferHandleCount = 0;



    mInputBufferMap.clear();
    mOutputBufferMap.clear();

    /// Free the memory manager
    mMemoryManager.clear();

    LOG_FUNCTION_NAME_EXIT

}

status_t OMXVce::init(int type, bool sync)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;


    LOG_FUNCTION_NAME

    mSync = sync;

    mType = type;

    mImageCrop.cropx = 0;
    mImageCrop.cropy = 0;
    mImageCrop.cropw = 0;
    mImageCrop.croph = 0;

    mImageWidth = 0;
    mImageHeight = 0;
    mImageFormat = 0;
    mImageInputCnt = 0;
    mImageOutFormat = 0;
    mImageCoding = 0;

    mOutputImageWidth = 0;
    mOutputImageHeight = 0;

    if(!mMemoryManager.get())
    {
        /// Create Memory Manager
        mMemoryManager = new MemoryManager();
        if( ( NULL == mMemoryManager.get() ) || ( mMemoryManager->initialize() != NO_ERROR))
        {
            CAMHAL_LOGEA("Unable to create or initialize MemoryManager");
            goto EXIT;
        }
    }

    ret = getOmxHandle();
    if(ret != NO_ERROR)
    {
        goto EXIT;
    }
    if(mType == OMXVceEncoderType)
    {
        mInputPortIndex = OMXVceImageInputPort;
        mOutputPortIndex = OMXVceEncoderOutputPort;
    }
    else
    {
        mInputPortIndex = OMXVceImageInputPort;
        mOutputPortIndex = OMXVceFDOutputPort;
    }

    eError = OMX_SendCommand(mOmxHandle,
                             OMX_CommandPortDisable,
                             OMX_ALL,
                             NULL);

    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

    eError = OMX_SendCommand(mOmxHandle,
                             OMX_CommandPortEnable,
                             mInputPortIndex,
                             NULL);

    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

    eError = OMX_SendCommand(mOmxHandle,
                             OMX_CommandPortEnable,
                             mOutputPortIndex,
                             NULL);

    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

    mInputPortEnable = true;
    mOutputPortEnable = true;

    ret = setStoreMetaData(mInputPortIndex, OMX_TRUE);
    if(ret != NO_ERROR)
    {
        goto EXIT;
    }

    if(mType == OMXVceEncoderType)
    {
#ifdef OMX_VCE_OUTPUT_USEBUFFER
        ret = setStoreMetaData(mOutputPortIndex, OMX_TRUE);
#else
        ret = setStoreMetaData(mOutputPortIndex, OMX_FALSE);
#endif
    }
    if(ret != NO_ERROR)
    {
        CAMHAL_LOGEB("setStoreMetaData error(%d)", ret);
        goto EXIT;
    }


    LOG_FUNCTION_NAME_EXIT

    return ret;

EXIT:
    CAMHAL_LOGEA("OMXVce::init error");
    freeOmxHandle();

    LOG_FUNCTION_NAME_EXIT
    return ret;
}


status_t OMXVce::getOmxHandle()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    LOG_FUNCTION_NAME

    //get omx handle
    eError = OMX_GetHandle(&mOmxHandle, OMX_VCE_NAME, this, &mCallbacks);
    if (eError != OMX_ErrorNone)
    {
        ret = ErrorUtils::omxToAndroidError(eError);
    }
    mComponentState = OMX_StateLoaded;

    LOG_FUNCTION_NAME_EXIT
    return ret;

}

status_t OMXVce::freeOmxHandle()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    if(mOmxHandle)
    {
        switchToLoaded();
        OMX_FreeHandle(mOmxHandle);
        mOmxHandle = NULL;
    }
    mComponentState = (OMX_STATETYPE)0;

    LOG_FUNCTION_NAME_EXIT
    return ret;

}

status_t OMXVce::switchToExec()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    LOG_FUNCTION_NAME

    if(mOmxHandle == NULL)
    {
        goto EXIT;
    }
    if(mComponentState == OMX_StateExecuting)
    {
        LOG_FUNCTION_NAME_EXIT
        return ret;
    }


    if(mComponentState == OMX_StateLoaded)
    {
        //disable all ports
        disableAllPorts();

        //change to idle
        ret = mEventSyncList.RegisterForEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateIdle, mSwitchToIdleSem);
        if(ret != NO_ERROR)
        {
            goto EXIT;
        }
        eError = OMX_SendCommand(mOmxHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
        OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
        ret = mSwitchToIdleSem.WaitTimeout(OMX_CMD_TIMEOUT);
        if(ret != NO_ERROR)
        {
            ret |= mEventSyncList.RemoveEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateIdle, NULL);
            goto EXIT;
        }
        mComponentState = OMX_StateIdle;
    }

    //change to exec
    if( mComponentState == OMX_StateIdle)
    {
        ret = mEventSyncList.RegisterForEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateExecuting, mSwitchToExecSem);
        if(ret != NO_ERROR)
        {
            goto EXIT;
        }

        eError = OMX_SendCommand(mOmxHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

        OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
        ret = mSwitchToExecSem.WaitTimeout(OMX_CMD_TIMEOUT);
        if(ret != NO_ERROR)
        {
            ret |= mEventSyncList.RemoveEvent(mOmxHandle, OMX_EventCmdComplete, OMX_StateExecuting, OMX_StateIdle, NULL);
            goto EXIT;
        }
        mComponentState = OMX_StateExecuting;
    }

    LOG_FUNCTION_NAME_EXIT
    return ret;

EXIT:
    freeOmxHandle();
    LOG_FUNCTION_NAME_EXIT
    return ret;
}


status_t OMXVce::execToIdle()
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    OMX_U32 inputPortIndex, outputPortIndex;
    int i;

    LOG_FUNCTION_NAME

    if(mOmxHandle == NULL)
    {
        goto EXIT;
    }

    if(mComponentState == OMX_StateExecuting)
    {
        //change to idle
        ret = mEventSyncList.RegisterForEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateIdle, mSwitchToIdleSem);
        if(ret != NO_ERROR)
        {
            goto EXIT;
        }
        eError = OMX_SendCommand(mOmxHandle,
                                 OMX_CommandStateSet,
                                 OMX_StateIdle,
                                 NULL);

        OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
        ret = mSwitchToIdleSem.WaitTimeout(OMX_CMD_TIMEOUT);
        if(ret != NO_ERROR)
        {
            ret |= mEventSyncList.RemoveEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateIdle, NULL);
            goto EXIT;
        }

        mComponentState = OMX_StateIdle;

    }

    LOG_FUNCTION_NAME_EXIT

    return ret;

EXIT:
    freeOmxHandle();
    LOG_FUNCTION_NAME_EXIT

    return ret;

}


status_t OMXVce::switchToLoaded()
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    OMX_U32 inputPortIndex, outputPortIndex;
    int i;

    LOG_FUNCTION_NAME

    if(mOmxHandle == NULL)
    {
        goto EXIT;
    }
    {


        if(mComponentState == OMX_StateExecuting)
        {
            //change to idle
            ret = mEventSyncList.RegisterForEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateIdle, mSwitchToIdleSem);
            if(ret != NO_ERROR)
            {
                goto EXIT;
            }
            eError = OMX_SendCommand(mOmxHandle,
                                     OMX_CommandStateSet,
                                     OMX_StateIdle,
                                     NULL);

            OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
            ret = mSwitchToIdleSem.WaitTimeout(OMX_CMD_TIMEOUT);
            if(ret != NO_ERROR)
            {
                ret |= mEventSyncList.RemoveEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateIdle, NULL);
                goto EXIT;
            }

            mComponentState = OMX_StateIdle;

        }



        if(mComponentState == OMX_StateIdle)
        {
            //disablePort
            disableAllPorts();

            //change to unloaded
            ret = mEventSyncList.RegisterForEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateLoaded, mSwitchToLoadedSem);
            if(ret != NO_ERROR)
            {
                goto EXIT;
            }
            eError = OMX_SendCommand(mOmxHandle,
                                     OMX_CommandStateSet,
                                     OMX_StateLoaded,
                                     NULL);

            OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
            ret = mSwitchToLoadedSem.WaitTimeout(OMX_CMD_TIMEOUT);
            if(ret != NO_ERROR)
            {
                ret |= mEventSyncList.RemoveEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateLoaded, NULL);
                goto EXIT;
            }

            mComponentState = OMX_StateLoaded;

            eError = OMX_SendCommand(mOmxHandle,
                                     OMX_CommandPortEnable,
                                     mInputPortIndex,
                                     NULL);

            OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

            eError = OMX_SendCommand(mOmxHandle,
                                     OMX_CommandPortEnable,
                                     mOutputPortIndex,
                                     NULL);

            OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

            mInputPortEnable = true;
            mOutputPortEnable = true;
        }

    }
    LOG_FUNCTION_NAME_EXIT

    return ret;

EXIT:
    CAMHAL_LOGEA("OMXVce::switchToLoaded error");
    mObserver->onOmxVceError(VCE_ERROR_FATAL);

    LOG_FUNCTION_NAME_EXIT

    return ret;

}


status_t OMXVce::getPortDefinition(OMX_U32 portIndex,OMX_PARAM_PORTDEFINITIONTYPE &portDefinition)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    LOG_FUNCTION_NAME

    OMX_INIT_STRUCT_PTR (&portDefinition, OMX_PARAM_PORTDEFINITIONTYPE);

    portDefinition.nPortIndex = portIndex;
    eError = OMX_GetParameter (mOmxHandle,
                               OMX_IndexParamPortDefinition, &portDefinition);
    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

EXIT:

    LOG_FUNCTION_NAME_EXIT
    return ret;

}


status_t OMXVce::setPortDefinition(OMX_U32 portIndex,OMX_PARAM_PORTDEFINITIONTYPE &portDefinition)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    LOG_FUNCTION_NAME
    portDefinition.nPortIndex = portIndex;

    eError = OMX_SetParameter (mOmxHandle,
                               OMX_IndexParamPortDefinition, &portDefinition);
    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

EXIT:

    LOG_FUNCTION_NAME_EXIT
    return ret;
}


status_t OMXVce::setImageCropSize(OMX_U32 x, OMX_U32 y, OMX_U32 w, OMX_U32 h)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    OMX_CONFIG_RECTTYPE cropRect;

    LOG_FUNCTION_NAME

    OMX_INIT_STRUCT_PTR (&cropRect, OMX_CONFIG_RECTTYPE);

    if( x == (OMX_U32)mImageCrop.cropx
            && y == (OMX_U32)mImageCrop.cropy
            && w == (OMX_U32)mImageCrop.cropw
            && h == (OMX_U32)mImageCrop.croph)
    {
        goto EXIT;
    }
    cropRect.nLeft = x;
    cropRect.nTop = y;
    cropRect.nWidth = w;
    cropRect.nHeight = h;

    cropRect.nPortIndex = OMXVceImageInputPort;

    eError = OMX_SetConfig(mOmxHandle,
                           OMX_IndexConfigCommonInputCrop, &cropRect);
    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
    mImageCrop.cropx = x;
    mImageCrop.cropy = y;
    mImageCrop.cropw = w;
    mImageCrop.croph = h;
EXIT:

    LOG_FUNCTION_NAME_EXIT
    return ret;

}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXVce::useBuffers( OMX_U32 portIndex, OMX_U32 bufSize,void* bufs[],OMX_U32 bufCount)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    video_metadata_t *metadata;
    unsigned int i;

    LOG_FUNCTION_NAME

    if(mComponentState != OMX_StateLoaded)
    {
        CAMHAL_LOGEB("component state(%d) error", mComponentState);
        ret = INVALID_OPERATION;
        goto EXIT;
    }
    if(portIndex == mInputPortIndex)
    {
        for(i = 0; i< mInputBufferCount; i++)
        {
            free(mInputBuffers[i]);
        }
        mInputBufferSize = bufSize;
        mInputBufferCount= 0;
        
        for(i = 0; i<bufCount ; i++)
        {
            metadata = (video_metadata_t *)malloc(sizeof(video_metadata_t));
            if(metadata == NULL)
            {
                goto EXIT;
            }
            metadata->metadataBufferType = kMetadataBufferTypeCameraSource_act;
            //ActionsCode(author:liuyiguang, change_code)
            //metadata->handle = (void *)(((uint32_t *)bufs)[i]);
            metadata->handle = (void *)(((long *)bufs)[i]);
            mInputBuffers[i] = metadata;
            mInputBufferCount++;
        }
    }
    else if(portIndex == mOutputPortIndex)
    {

#ifdef OMX_VCE_OUTPUT_USEBUFFER
        unsigned int phy;
        const char dummy[] = "";

        for(i = 0; i< mOutputBufferCount; i++)
        {
            free(mOutputBuffers[i]);
        }
        mOutputBufferCount = 0;

        for(i = 0; i< mOutputBufferHandleCount; i++)
        {
            GraphicBufferAllocator &GrallocAlloc = GraphicBufferAllocator::get();
            GrallocAlloc.free((buffer_handle_t)mOutputBufferHandles[i]);
        }
        mOutputBufferHandleCount = 0;
        mOutputBufferSize = bufSize;



        for(i = 0; i<bufCount ; i++)
        {
            metadata = (video_metadata_t *)malloc(sizeof(video_metadata_t));
            if(metadata == NULL)
            {
                goto EXIT;
            }
            metadata->metadataBufferType = kMetadataBufferTypeCameraSource_act;


            mOutputBuffers[i] = metadata;
            mOutputBufferCount++;

            GraphicBufferAllocator &GrallocAlloc = GraphicBufferAllocator::get();
            buffer_handle_t buf;
            CAMHAL_LOGDB("mOutputBufferSize = %d",mOutputBufferSize);
            int32_t stride;
            ret = GrallocAlloc.alloc((mOutputBufferSize)*3/2/32, 32, HAL_PIXEL_FORMAT_ACT_NV12, GRALLOC_USAGE_HW_CAMERA, &buf, &stride);
            if (ret != NO_ERROR)
            {
                CAMHAL_LOGEB("Couldn't allocate video buffers using Gralloc,ret=%d",ret);
                ret = -NO_MEMORY;
                goto EXIT;
            }
            metadata->handle = (void *)(buf);
            mOutputBufferHandleCount++;


        }
#else
        mOutputBufferHandleCount = 0;
        mOutputBufferCount = bufCount;
        mOutputBufferSize = bufSize;
#endif

    }
    else
    {
        CAMHAL_LOGEB("portIndex(%d) error", (int)portIndex);
    
        ret = BAD_VALUE;
        goto EXIT;
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;

EXIT:
    if(portIndex == mInputPortIndex)
    {
        for(i = 0; i< mInputBufferCount; i++)
        {
            free(mInputBuffers[i]);
        }
        mInputBufferSize = 0;
        mInputBufferCount= 0;
    }
    else
    {
#ifdef OMX_VCE_OUTPUT_USEBUFFER
        for(i = 0; i< mOutputBufferCount; i++)
        {
            free(mOutputBuffers[i]);
        }
#endif
        mOutputBufferSize = 0;
        mOutputBufferCount=0;
    }

#ifdef OMX_VCE_OUTPUT_USEBUFFER
    for(i = 0; i< mOutputBufferHandleCount; i++)
    {
        GraphicBufferAllocator &GrallocAlloc = GraphicBufferAllocator::get();
        GrallocAlloc.free((buffer_handle_t)mOutputBufferHandles[i]);
    }
#endif
    mOutputBufferHandleCount = 0;
    LOG_FUNCTION_NAME_EXIT

    return ret;

}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXVce::loadedToIdle( )
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    unsigned int i;

    unsigned int virt;
    unsigned int phy;
    OMX_BUFFERHEADERTYPE *pBufferHdr;

    LOG_FUNCTION_NAME


    if(mComponentState != OMX_StateLoaded)
    {
        LOG_FUNCTION_NAME_EXIT
        return ret;
    }
    

    //switch to idle
    ret = mEventSyncList.RegisterForEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateIdle, mSwitchToIdleSem);
    if(ret != NO_ERROR)
    {
        goto EXIT;
    }
    eError = OMX_SendCommand(mOmxHandle,
                             OMX_CommandStateSet,
                             OMX_StateIdle,
                             NULL);

    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));


    //use Buffer
    mInputBufferMap.clear();

    CAMHAL_LOGDB("mInputBufferCount = %d\n",mInputBufferCount);
    for(i = 0; i< mInputBufferCount; i++)
    {
        CAMHAL_LOGDB("OMX_UseBuffer = %p\n", reinterpret_cast<OMX_U8 *>(mInputBuffers[i]));
        CAMHAL_LOGDB("OMX_UseBuffer handle= %p\n", (void *)((video_metadata_t *)mInputBuffers[i])->handle);
        eError = OMX_UseBuffer(mOmxHandle,&pBufferHdr, mInputPortIndex, 0, sizeof(video_metadata_t), reinterpret_cast<OMX_U8 *>(mInputBuffers[i]));
        OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
        
        //ActionsCode(author:liuyiguang, change_code)
        //mInputBufferMap.add((unsigned int)((video_metadata_t *)mInputBuffers[i])->handle, (unsigned int)pBufferHdr);
        mInputBufferMap.add((long)((video_metadata_t *)mInputBuffers[i])->handle, (long)pBufferHdr);
        CAMHAL_LOGDB("OMX_UseBuffer over (%p)\n", reinterpret_cast<OMX_U8 *>(mInputBuffers[i]));
    }

    CAMHAL_LOGDA("out of use input buffer\n");
    mOutputBufferMap.clear();
    CAMHAL_LOGDB("mOutputBufferCount = %d\n",(int)mOutputBufferCount);
#ifdef OMX_VCE_OUTPUT_USEBUFFER
    
    for(i = 0; i< mOutputBufferCount; i++)
    {
        eError = OMX_UseBuffer(mOmxHandle,&pBufferHdr, mOutputPortIndex, 0, sizeof(video_metadata_t),(OMX_U8 *)mOutputBuffers[i]);


        video_metadata_t *metadata;

        metadata = (video_metadata_t *)mOutputBuffers[i];
    //fixe me: private_handle_t no longer open to  gralloc module user, it's not accessible by user
    //gralloc module provide extra interfaces for user to get virtual and physical address
    // should modify later
        private_handle_t *handle = (private_handle_t*)metadata->handle;

        virt = (unsigned int)(handle->base);

        mOutputBufferMap.add(virt, (unsigned int)pBufferHdr);
        OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

    }
#else
    for(i = 0; i< mOutputBufferCount; i++)
    {
        eError = OMX_AllocateBuffer(mOmxHandle,&pBufferHdr, mOutputPortIndex, 0, mOutputBufferSize);

        //ActionsCode(author:liuyiguang, change_code)
        //mOutputBufferMap.add((unsigned int)pBufferHdr->pBuffer, (unsigned int)pBufferHdr);
        mOutputBufferMap.add((long)pBufferHdr->pBuffer, (long)pBufferHdr);
        OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

    }
#endif

    CAMHAL_LOGDA("out of use output buffer\n");

    ret = mSwitchToIdleSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if(ret != NO_ERROR)
    {

        ret |= mEventSyncList.RemoveEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandStateSet, OMX_StateIdle, NULL);
        goto EXIT;
    }

    CAMHAL_LOGDA("switch to idle successfully\n");
    mComponentState = OMX_StateIdle;

    LOG_FUNCTION_NAME_EXIT

    return ret;

EXIT:
    CAMHAL_LOGDA("use buffer error\n");
    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXVce::disableAllPorts()
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    LOG_FUNCTION_NAME

    ret = disablePort(mInputPortIndex);
    ret |= disablePort(mOutputPortIndex);

    LOG_FUNCTION_NAME_EXIT

    return ret;

}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXVce::disablePort(OMX_U32 portIndex)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    unsigned int i;

    LOG_FUNCTION_NAME

    if(portIndex==mInputPortIndex && mInputPortEnable == false)
    {
        LOG_FUNCTION_NAME_EXIT
        return ret;
    }
    else if(portIndex==mOutputPortIndex && mOutputPortEnable == false)
    {
        LOG_FUNCTION_NAME_EXIT
        return ret;
    }

    //disable port
    ret = mEventSyncList.RegisterForEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandPortDisable, portIndex, mPortDisableSem);
    if(ret != NO_ERROR)
    {
        goto EXIT;
    }
    eError = OMX_SendCommand(mOmxHandle,
                             OMX_CommandPortDisable,
                             portIndex,
                             NULL);

    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));


    //free Buffer
    if(portIndex == mInputPortIndex)
    {
        for(i = 0; i< mInputBufferMap.size(); i++)
        {
            //ActionsCode(author:liuyiguang, change_code)
            //eError = OMX_FreeBuffer(mOmxHandle,
            //                        portIndex,
            //                        (OMX_BUFFERHEADERTYPE*)mInputBufferMap.valueAt(i));
            eError = OMX_FreeBuffer(mOmxHandle,
                                    portIndex,
                                    (OMX_BUFFERHEADERTYPE*)(long)mInputBufferMap.valueAt(i));

            OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

        }
    }
    else
    {
        for(i = 0; i< mOutputBufferMap.size(); i++)
        {
            //ActionsCode(author:liuyiguang, change_code)
            //eError = OMX_FreeBuffer(mOmxHandle,
            //                        portIndex,
            //                        (OMX_BUFFERHEADERTYPE*)mOutputBufferMap.valueAt(i));
            eError = OMX_FreeBuffer(mOmxHandle,
                                    portIndex,
                                    (OMX_BUFFERHEADERTYPE*)(long)mOutputBufferMap.valueAt(i));

            OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

        }
    }
    ret = mPortDisableSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if(ret != NO_ERROR)
    {

        ret |= mEventSyncList.RemoveEvent(mOmxHandle, OMX_EventCmdComplete, OMX_CommandPortDisable, portIndex, NULL);
        goto EXIT;
    }

    if(portIndex==mInputPortIndex)
    {
        mInputPortEnable = false;
    }
    else
    {
        mOutputPortEnable = false;
    }

    LOG_FUNCTION_NAME_EXIT
    return ret;

EXIT:
    LOG_FUNCTION_NAME_EXIT
    return ret;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXVce::processImageAsync(void * buf, VceCropRect *crop)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    OMX_BUFFERHEADERTYPE *pBufferHdr;
    video_metadata_t *metadata;


    LOG_FUNCTION_NAME
    if(mComponentState != OMX_StateExecuting)
    {
        LOG_FUNCTION_NAME_EXIT
        return ret;
    }

    if(mSync == true)
    {
        ret = BAD_VALUE;
        goto EXIT;
    }
    {
        Mutex::Autolock lock(mImageLock);
        //ActionsCode(author:liuyiguang, change_code)
        //pBufferHdr = reinterpret_cast<OMX_BUFFERHEADERTYPE *>(mInputBufferMap.valueFor((unsigned int )buf));
        pBufferHdr = reinterpret_cast<OMX_BUFFERHEADERTYPE *>(mInputBufferMap.valueFor((long)buf));

        if(pBufferHdr == NULL)
        {
            ret = BAD_VALUE;
            goto EXIT;
        }

        metadata = (video_metadata_t *)pBufferHdr->pBuffer;
        metadata->off_x = crop->cropx;
        metadata->off_y = crop->cropy;
        metadata->crop_w= crop->cropw;
        metadata->crop_h= crop->croph;
        eError = OMX_EmptyThisBuffer(mOmxHandle, pBufferHdr);
        OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;

EXIT:
    LOG_FUNCTION_NAME_EXIT

    return ret;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXVce::processImageSync(void * buf, VceCropRect *crop,  void **outbuf, unsigned int *offset, unsigned int *size)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    OMX_BUFFERHEADERTYPE *pBufferHdr;
    video_metadata_t *metadata;

    LOG_FUNCTION_NAME

    *outbuf = NULL;
    *size = 0;

    if(mComponentState != OMX_StateExecuting)
    {
        LOG_FUNCTION_NAME_EXIT
        return ret;
    }

    if(mSync == false)
    {
        ret = BAD_VALUE;
        goto EXIT;
    }
    {
        Mutex::Autolock lock(mImageLock);

        while(mImageProcessing == true)
        {
            mImageCond.wait(mImageLock);
        }
        for(unsigned int i = 0; i < mInputBufferMap.size(); i++)
        {
            CAMHAL_LOGDB("key = 0x%x, v=0x%x\n", mInputBufferMap.keyAt(i), mInputBufferMap.valueAt(i));
        }
        CAMHAL_LOGDB("buf=%p\n",buf);
        //ActionsCode(author:liuyiguang, change_code)
        //pBufferHdr =reinterpret_cast<OMX_BUFFERHEADERTYPE *>(mInputBufferMap.valueFor((unsigned int )buf));
        pBufferHdr =reinterpret_cast<OMX_BUFFERHEADERTYPE *>(mInputBufferMap.valueFor((long)buf));
        CAMHAL_LOGDB("pBufferHdr=%p\n",pBufferHdr);
        if(pBufferHdr == NULL)
        {
            ret = BAD_VALUE;
            goto EXIT;
        }
        metadata = (video_metadata_t *)pBufferHdr->pBuffer;
        metadata->off_x = crop->cropx;
        metadata->off_y = crop->cropy;
        metadata->crop_w= crop->cropw;
        metadata->crop_h= crop->croph;

        mImageProcessing = true;
        mOutputBuf = NULL;
        mOutputBufSize = 0;
        mOutputBufOffset= 0;

        {
            Mutex::Autolock lock(mFillBufferSyncLock);
            mFillBufferSyncAvilable = false;
            eError = OMX_EmptyThisBuffer(mOmxHandle, pBufferHdr);
            OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

            ret = NO_ERROR;

            while( (ret == NO_ERROR) && (mFillBufferSyncAvilable != true))
            {
                ret = mFillBufferSyncCond.waitRelative(mFillBufferSyncLock, (nsecs_t)(VCE_ENCODE_SYNC_TIMEOUT)*1000);
            }
            if(ret != NO_ERROR)
            {
                CAMHAL_LOGEA("Error, Vce Encode Timeout!");
                mImageProcessing = false;
                mObserver->onOmxVceError(VCE_ERROR_FATAL);
                LOG_FUNCTION_NAME_EXIT
                return ret;
            }
        }

        *outbuf = mOutputBuf;
        *size = mOutputBufSize;
        *offset = mOutputBufOffset;
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;

EXIT:
    LOG_FUNCTION_NAME_EXIT
    return ret;
}

status_t OMXVce::freeImageData(void * buf)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME
    if(mComponentState != OMX_StateExecuting)
    {
        LOG_FUNCTION_NAME_EXIT
        return ret;
    }

    if(mSync == false)
    {
        cycleOutputBuffer(buf);;
    }
    else
    {
        Mutex::Autolock lock(mImageLock);

        if(mImageProcessing == true)
        {
            cycleOutputBuffer(buf);
            mImageProcessing = false;
        }
        mImageCond.signal();
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;

    EXIT:
    LOG_FUNCTION_NAME_EXIT
    return ret;
}


status_t OMXVce::fillAllOutputBuffers()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_BUFFERHEADERTYPE *pBufferHdr;
    unsigned int i = 0;

    LOG_FUNCTION_NAME

    for(i = 0; i< mOutputBufferMap.size(); i++)
    {
        pBufferHdr =reinterpret_cast<OMX_BUFFERHEADERTYPE *>(mOutputBufferMap.valueAt(i));

        if(pBufferHdr == NULL)
        {
            ret = BAD_VALUE;
            continue;
        }

        //pBufferHdr->nFilledLen = 0;
        eError = OMX_FillThisBuffer(mOmxHandle, pBufferHdr);
        OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;

EXIT:

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t OMXVce::onEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    CAMHAL_LOGDB("+OMX_Event %x, %d %d", eEvent, (int)nData1, (int)nData2);
    LOG_FUNCTION_NAME

    switch (eEvent)
    {
    case OMX_EventCmdComplete:
        CAMHAL_LOGDB("+OMX_EventCmdComplete %d %d", (int)nData1, (int)nData2);

        if (OMX_CommandStateSet == nData1)
        {
            mComponentState = (OMX_STATETYPE) nData2;
        }
        else if (OMX_CommandFlush == nData1)
        {
            CAMHAL_LOGDB("OMX_CommandFlush received for port %d", (int)nData2);

        }
        else if (OMX_CommandPortDisable == nData1)
        {
            CAMHAL_LOGDB("OMX_CommandPortDisable received for port %d", (int)nData2);

        }
        else if (OMX_CommandPortEnable == nData1)
        {
            CAMHAL_LOGDB("OMX_CommandPortEnable received for port %d", (int)nData2);

        }
        else if (OMX_CommandMarkBuffer == nData1)
        {
            ///This is not used currently
        }

        CAMHAL_LOGDA("-OMX_EventCmdComplete");
        break;

    case OMX_EventError:
        CAMHAL_LOGDB("OMX interface failed to execute OMX command %d", (int)nData1);
        CAMHAL_LOGDA("See OMX_INDEXTYPE for reference");
        if ( NULL != mObserver && ( ( OMX_U32 ) OMX_ErrorHardware == nData1 ) && mComponentState != 0/*OMX_StateInvalid*/)
        {
            CAMHAL_LOGDA("***Got Fatal Error Notification***\n");
            mComponentState = (OMX_STATETYPE)0/*OMX_StateInvalid*/;
            /*
               Remove any unhandled events and
               unblock any waiting semaphores
               */
            mEventSyncList.FlushEvent();

            ///Report Error to App
            mObserver->onOmxVceError(VCE_ERROR_FATAL);
        }


        break;

    case OMX_EventMark:
        break;

    case OMX_EventPortSettingsChanged:
        break;

    case OMX_EventBufferFlag:
        break;

    case OMX_EventResourcesAcquired:
        break;

    case OMX_EventComponentResumed:
        break;

    case OMX_EventDynamicResourcesAvailable:
        break;

    case OMX_EventPortFormatDetected:
        break;

    default:
        break;
    }

    ///Signal to the thread(s) waiting that the event has occured
    mEventSyncList.SignalEvent(mOmxHandle, eEvent, nData1, nData2, pEventData);

    LOG_FUNCTION_NAME_EXIT;
    return eError;

EXIT:

    CAMHAL_LOGDB("Exiting function %s because of eError=%x", __FUNCTION__, eError);
    LOG_FUNCTION_NAME_EXIT;
    return eError;

}

status_t OMXVce::onEmptyBufferDone(OMX_BUFFERHEADERTYPE *pBuffer)
{
    LOG_FUNCTION_NAME


    if(pBuffer->nInputPortIndex == OMXVceImageInputPort)
    {
        if(mSync != true)
        {
            returnInputBuffer(((video_metadata_t *)pBuffer->pBuffer)->handle);
        }
    }

    LOG_FUNCTION_NAME_EXIT
    return NO_ERROR;
}

status_t OMXVce::onFillBufferDone(OMX_BUFFERHEADERTYPE *pBuffer)
{
    LOG_FUNCTION_NAME

#ifdef OMX_VCE_OUTPUT_USEBUFFER
    video_metadata_t *metadata;

    metadata = (video_metadata_t *)pBuffer->pBuffer;
    //fixe me: private_handle_t no longer open to  gralloc module user, it's not accessible by user
    //gralloc module provide extra interfaces for user to get virtual and physical address
    // should modify later
    private_handle_t *handle = (private_handle_t*)metadata->handle;

    {

        if(mComponentState != OMX_StateExecuting)
        {
            CAMHAL_LOGEA("component state is not stateexecuting");
            return UNKNOWN_ERROR;
        }

        if(mSync == true)
        {
            //TODO, should lock buffer_handle_t
            mOutputBuf = (unsigned char *)handle->base;
            CAMHAL_LOGDB("vce jpeg encoder output viraddr=%p", mOutputBuf);
            CAMHAL_LOGDB("pBuffer->nOffset=%p", (void *)pBuffer->nOffset);
            //mOutputBufSize = pBuffer->nFilledLen;
            //mOutputBufOffset = pBuffer->nOffset;
            mOutputBufSize = metadata->vce_attribute.nfilledlen;
            mOutputBufOffset = metadata->vce_attribute.noffset;

            {
                Mutex::Autolock lock(mFillBufferSyncLock);
                mFillBufferSyncAvilable = true;
                mFillBufferSyncCond.signal();
            }
        }
        else
        {
            //sendOutputBuffer((void *)handle->base, pBuffer->nOffset, pBuffer->nFilledLen);
            sendOutputBuffer((void *)handle->base, metadata->vce_attribute.noffset, metadata->vce_attribute.nfilledlen);
        }

    }
#else
    if(mComponentState != OMX_StateExecuting)
    {
        CAMHAL_LOGEA("component state is not stateexecuting");
        return UNKNOWN_ERROR;
    }

    if(mSync == true)
    {
        mOutputBuf = (unsigned char *)pBuffer->pBuffer;
        CAMHAL_LOGDB("vce jpeg encoder output viraddr=%p", mOutputBuf);
        CAMHAL_LOGDB("pBuffer->nOffset=%p", (void *)pBuffer->nOffset);
        mOutputBufSize = pBuffer->nFilledLen;
        mOutputBufOffset = pBuffer->nOffset;

        {
            Mutex::Autolock lock(mFillBufferSyncLock);
            mFillBufferSyncAvilable = true;
            mFillBufferSyncCond.signal();
        }
    }
    else
    {
        sendOutputBuffer((void *)pBuffer->pBuffer, pBuffer->nOffset, pBuffer->nFilledLen);
    }    

#endif
    LOG_FUNCTION_NAME_EXIT
    return NO_ERROR;


}

void OMXVce::returnInputBuffer(void * buffer)
{

    LOG_FUNCTION_NAME

    if(mObserver)
    {
        mObserver->returnInputBuffer(buffer);
    }

    LOG_FUNCTION_NAME_EXIT
}

void OMXVce::sendOutputBuffer(void *outbuf, unsigned int offset, unsigned int size)
{

    LOG_FUNCTION_NAME

    if(mObserver)
    {
        mObserver->sendOutputBuffer(outbuf, offset, size);
    }

    LOG_FUNCTION_NAME_EXIT
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXVce::cycleOutputBuffer(void *outbuf)
{

    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    OMX_BUFFERHEADERTYPE *pBufferHdr;

    LOG_FUNCTION_NAME

    //ActionsCode(author:liuyiguang, change_code)
    //int index = mOutputBufferMap.indexOfKey((unsigned int )outbuf);
    int index = mOutputBufferMap.indexOfKey((long)outbuf);
    if(index < 0)
    {
        return NO_ERROR;
    }
    pBufferHdr = reinterpret_cast<OMX_BUFFERHEADERTYPE *>(mOutputBufferMap.valueAt(index));

    if(pBufferHdr == NULL)
    {
        ret = BAD_VALUE;
        goto EXIT;
    }

    //pBufferHdr->nFilledLen = 0;
    eError = OMX_FillThisBuffer(mOmxHandle, pBufferHdr);
    if ( eError != OMX_ErrorNone )
    {
        goto EXIT;
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;

EXIT:

    LOG_FUNCTION_NAME_EXIT

    return ret;

}

status_t OMXVce::setStoreMetaData(OMX_U32 portIndex, OMX_BOOL enable)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    LOG_FUNCTION_NAME

    StoreMetaDataInBuffersParams storeMetaDataParam;
    OMX_INIT_STRUCT_PTR (&storeMetaDataParam, StoreMetaDataInBuffersParams);


    storeMetaDataParam.nPortIndex = portIndex;
    storeMetaDataParam.bStoreMetaData = enable;

    eError = OMX_SetParameter(mOmxHandle,static_cast<OMX_INDEXTYPE>(OMX_IndexParameterStoreMediaData), &storeMetaDataParam);
    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

EXIT:

    LOG_FUNCTION_NAME_EXIT
    return ret;

}

status_t OMXVce::setVceObserver(OMXVceObserver *observer)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    if ( NULL == observer )
    {
        ret = -EINVAL;
    }

    if ( NO_ERROR == ret )
    {
        mObserver = observer;
    }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}


status_t OMXVce::setImageSize(OMX_U32 w, OMX_U32 h)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    if(w == mImageWidth
            && h == mImageHeight)
    {
        return NO_ERROR;
    }

    mImageWidth = w;
    mImageHeight = h;

    LOG_FUNCTION_NAME_EXIT
    return ret;
}
status_t OMXVce::setOutputImageSize(OMX_U32 w, OMX_U32 h)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    if(w == mOutputImageWidth
            && h == mOutputImageHeight)
    {
        return NO_ERROR;
    }

    mOutputImageWidth = w;
    mOutputImageHeight = h;

    LOG_FUNCTION_NAME_EXIT
    return ret;
}

status_t OMXVce::setImageFormat(OMX_U32 f)
{
    status_t ret = NO_ERROR;
    
    mImageFormat = f;
    return ret;
}


status_t OMXVce::setImageInputCnt(OMX_U32 n)
{

    status_t ret = NO_ERROR;
    
    mImageInputCnt = n;
    return ret;
}


//OMXVceJpegEncoder
status_t OMXVceJpegEncoder::init()
{
    status_t ret;
    ret = OMXVce::init(OMXVceEncoderType, true);

    memset(&mLastParam, 0,sizeof(mLastParam));
    return ret;
}


status_t OMXVceJpegEncoder::setImageQuality(OMX_U32 quality)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    OMX_IMAGE_PARAM_QFACTORTYPE qfactor;

    LOG_FUNCTION_NAME


    OMX_INIT_STRUCT_PTR (&qfactor, OMX_IMAGE_PARAM_QFACTORTYPE);

    qfactor.nPortIndex = OMXVceEncoderOutputPort;
    qfactor.nQFactor = quality;

    eError = OMX_SetParameter(mOmxHandle, OMX_IndexParamQFactor, &qfactor);
    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

EXIT:

    LOG_FUNCTION_NAME_EXIT
    return ret;
}

status_t OMXVceJpegEncoder::setThumbnailSize(OMX_U32 w, OMX_U32 h)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    OMX_ACT_PARAM_THUMBPARAM thumbParam;

    LOG_FUNCTION_NAME

    OMX_INIT_STRUCT_PTR (&thumbParam, OMX_ACT_PARAM_THUMBPARAM);

    thumbParam.nPortIndex = mOutputPortIndex;

    if(w == 0 || h==0)
    {
        thumbParam.bThumbEnable = OMX_FALSE;
    }
    else
    {
        thumbParam.nWidth = w;
        thumbParam.nHeight = h;
        thumbParam.bThumbEnable = OMX_TRUE;
    }

    eError = OMX_SetParameter(mOmxHandle,
                           static_cast<OMX_INDEXTYPE>(OMX_ACT_IndexParamThumbControl), &thumbParam);
    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

EXIT:

    LOG_FUNCTION_NAME_EXIT

    return ret;
}



status_t OMXVceJpegEncoder::setExif(CameraHalExif *exifParam)
{

    OMX_ACT_PARAM_EXIFPARAM exif;
    LOG_FUNCTION_NAME

    //set exif
    OMX_INIT_STRUCT_PTR (&exif, OMX_ACT_PARAM_EXIFPARAM);
    exif.nPortIndex = mOutputPortIndex;

    if(exifParam == NULL)
    {
        exif.bExifEnable = OMX_FALSE;
    }
    else
    {
        exif.bExifEnable = OMX_TRUE;
        exif.bGPS = (exifParam->bGPS == true) ? OMX_TRUE : OMX_FALSE;
        exif.dataTime = (char *)&(exifParam->dateTime);
        exif.exifmake= (char *)&(exifParam->exifmake);
        exif.exifmodel= (char *)&(exifParam->exifmodel);
        exif.focalLengthL = static_cast<OMX_U32>(exifParam->focalLengthL);
        exif.focalLengthH = static_cast<OMX_U32>(exifParam->focalLengthH);
        exif.gpsDate = (char *)&(exifParam->gpsDatestamp);
        exif.gpsLATL[0] = static_cast<OMX_U32>(exifParam->gpsLATL[0]);
        exif.gpsLATH[0] = static_cast<OMX_U32>(exifParam->gpsLATH[0]);
        exif.gpsLATL[1] = static_cast<OMX_U32>(exifParam->gpsLATL[1]);
        exif.gpsLATH[1] = static_cast<OMX_U32>(exifParam->gpsLATH[1]);
        exif.gpsLATL[2] = static_cast<OMX_U32>(exifParam->gpsLATL[2]);
        exif.gpsLATH[2] = static_cast<OMX_U32>(exifParam->gpsLATH[2]);

        exif.gpsLATREF = static_cast<OMX_U32>(exifParam->gpsLATREF);

        exif.gpsLONGL[0] = static_cast<OMX_U32>(exifParam->gpsLONGL[0]);
        exif.gpsLONGH[0] = static_cast<OMX_U32>(exifParam->gpsLONGH[0]);
        exif.gpsLONGL[1] = static_cast<OMX_U32>(exifParam->gpsLONGL[1]);
        exif.gpsLONGH[1] = static_cast<OMX_U32>(exifParam->gpsLONGH[1]);
        exif.gpsLONGL[2] = static_cast<OMX_U32>(exifParam->gpsLONGL[2]);
        exif.gpsLONGH[2] = static_cast<OMX_U32>(exifParam->gpsLONGH[2]);

        exif.gpsLONGREF = static_cast<OMX_U32>(exifParam->gpsLONGREF);

        exif.gpsALTIL[0] = static_cast<OMX_U32>(exifParam->gpsALTL);
        exif.gpsALTIH[0] = static_cast<OMX_U32>(exifParam->gpsALTH);
        exif.gpsALTIREF = static_cast<OMX_U32>(exifParam->gpsALTREF);

        exif.gpsprocessMethod = (char *)&(exifParam->gpsProcessMethod);

        exif.gpsTimeL[0] = static_cast<OMX_U32>(exifParam->gpsTimeL[0]);
        exif.gpsTimeH[0] = static_cast<OMX_U32>(exifParam->gpsTimeH[0]);
        exif.gpsTimeL[1] = static_cast<OMX_U32>(exifParam->gpsTimeL[1]);
        exif.gpsTimeH[1] = static_cast<OMX_U32>(exifParam->gpsTimeH[1]);
        exif.gpsTimeL[2] = static_cast<OMX_U32>(exifParam->gpsTimeL[2]);
        exif.gpsTimeH[2] = static_cast<OMX_U32>(exifParam->gpsTimeH[2]);

        exif.ImageOri = 1;
        switch(exifParam->imageOri)
        {
        case 0:
            exif.ImageOri = 1;
            break;
        case 90:
            exif.ImageOri = 6;
            break;
        case 180:
            exif.ImageOri = 3;
            break;
        case 270:
            exif.ImageOri = 8;
            break;
        }

    }

    LOG_FUNCTION_NAME_EXIT

    return setExif(&exif);
}

status_t OMXVceJpegEncoder::setExif(OMX_ACT_PARAM_EXIFPARAM *exif)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    LOG_FUNCTION_NAME

#ifdef CAMERA_VCEJPEG_KEEPIDLE
    eError = OMX_SetParameter(mOmxHandle,
                           static_cast<OMX_INDEXTYPE>(OMX_ACT_IndexParamExifControl), exif);
#else
    eError = OMX_SetConfig(mOmxHandle,
                           static_cast<OMX_INDEXTYPE>(OMX_ACT_IndexParamExifControl), exif);
#endif
    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));

EXIT:

    LOG_FUNCTION_NAME_EXIT
    return ret;
}

bool OMXVceJpegEncoder::isNeedSetParam(ImageJpegEncoderParam *newp, ImageJpegEncoderParam *oldp)
{
#ifdef CAMERA_VCEJPEG_KEEPIDLE
    return true;
#endif

    if(newp->buffersCount != oldp->buffersCount)
    {
        return true;
    }
    else
    {
        for(uint32_t i = 0; i < newp->buffersCount;i++)
        {
            if(newp->buffers[i] != oldp->buffers[i])
            {
                return true;
            }
        }
    }

    if(newp->bufferSize!= oldp->bufferSize)
    {
        return true;
    }
    if(newp->format!= oldp->format)
    {
        return true;
    }
    if(newp->coding!= oldp->coding)
    {
        return true;
    }
    if(newp->origWidth!= oldp->origWidth)
    {
        return true;
    }
    if(newp->origHeight!= oldp->origHeight)
    {
        return true;
    }

    if(newp->outputWidth!= oldp->outputWidth)
    {
        return true;
    }
    if(newp->outputHeight!= oldp->outputHeight)
    {
        return true;
    }
    if(newp->thumbWidth!= oldp->thumbWidth)
    {
        return true;
    }
    if(newp->thumbHeight!= oldp->thumbHeight)
    {
        return true;
    }
    if(newp->quality!= oldp->quality)
    {
        return true;
    }
    return false;

}
status_t OMXVceJpegEncoder::encode(ImageJpegEncoderParam *param, CameraHalExif *exifParam)
{
    status_t ret = NO_ERROR;


    void * outBuf = NULL;
    unsigned int outSize;
    unsigned int outOffset;

    VceCropRect cropRect;

    LOG_FUNCTION_NAME

    CAMHAL_LOGDB("param->buffersCount=%d", (int)param->buffersCount);
    CAMHAL_LOGDB("param->bufferSize=%d", (int)param->bufferSize);
    CAMHAL_LOGDB("param->format=%d", (int)param->format);
    CAMHAL_LOGDB("param->coding =%d", (int)param->coding);
    CAMHAL_LOGDB("param->inData=%p", param->inData);
    CAMHAL_LOGDB("param->origWidth=%d", (int)param->origWidth);
    CAMHAL_LOGDB("param->origHeight=%d", (int)param->origHeight);
    CAMHAL_LOGDB("param->outputWidth=%d", (int)param->outputWidth);
    CAMHAL_LOGDB("param->outputHeight=%d", (int)param->outputHeight);
    CAMHAL_LOGDB("param->xoff=%d", (int)param->xoff);
    CAMHAL_LOGDB("param->yoff=%d", (int)param->yoff);
    CAMHAL_LOGDB("param->width=%d", (int)param->width);
    CAMHAL_LOGDB("param->height=%d", (int)param->height);
    CAMHAL_LOGDB("param->thumbWidth=%d", (int)param->thumbWidth);
    CAMHAL_LOGDB("param->thumbHeight=%d", (int)param->thumbHeight);

    {
        Mutex::Autolock lock(mJpegLock);

        if((mComponentState != OMX_StateExecuting) || isNeedSetParam(&mLastParam, param) )
        {
            switchToLoaded();

#ifdef CAMERA_VCEJPEG_KEEPIDLE
            if(exifParam != NULL)
            {
                //set exif
                ret = setExif(exifParam);
                if(ret != NO_ERROR)
                {
                    CAMHAL_LOGEA("setExif  error!");
                    goto EXIT;
                }

            }
#endif
            

            //set setThumbnailSize
            ret = setThumbnailSize(param->thumbWidth, param->thumbHeight);
            if(ret != NO_ERROR)
            {
                CAMHAL_LOGEB("setThumbnailSize:%dx%d  error!", param->thumbWidth, param->thumbHeight);
                goto EXIT;
            }

        //set quality
        ret = setImageQuality(param->quality);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEB("setImageQuality:%d  error!", param->quality);
            goto EXIT;
        }
        //set port definition
        ret = getPortDefinition(mInputPortIndex, mInputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("getPortDefinition  error!");
            goto EXIT;
        }

            mInputPortParam.nBufferCountActual = param->buffersCount;
            mInputPortParam.format.video.nFrameWidth = param->origWidth;
            mInputPortParam.format.video.nFrameHeight = param->origHeight;
            mInputPortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)param->format;
            mInputPortParam.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingUnused;

        CAMHAL_LOGDB("inputPortParam.nBufferCountActual = %d", (int)mInputPortParam.nBufferCountActual);

        ret = setPortDefinition(mInputPortIndex, mInputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("setPortDefinition  error!");
            goto EXIT;
        }

        ret = getPortDefinition(mOutputPortIndex, mOutputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("getPortDefinition  error!");
            goto EXIT;
        }
        mOutputPortParam.nBufferCountActual = 1;
        mOutputPortParam.format.video.nFrameWidth = param->outputWidth;
        mOutputPortParam.format.video.nFrameHeight = param->outputHeight;
        mOutputPortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)param->format;
        mOutputPortParam.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)param->coding;


        ret = setPortDefinition(mOutputPortIndex, mOutputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("setPortDefinition  error!");
            goto EXIT;
        }
        //get the real buffer size for output buffer
        ret = getPortDefinition(mOutputPortIndex, mOutputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("getPortDefinition  error!");
            goto EXIT;
        }


        //use buffers
        ret = useBuffers(mInputPortIndex, param->bufferSize, param->buffers, param->buffersCount);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("useBuffers error!");
            goto EXIT;
        }

        //use buffers
        ret = useBuffers(mOutputPortIndex, mOutputPortParam.nBufferSize, NULL, mOutputPortParam.nBufferCountActual);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("useBuffers error!");
            goto EXIT;
        }

        //to Idle
        ret = loadedToIdle();
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("loadedToIdle error!");
            goto EXIT;
        }

            //to Idle
            ret = switchToExec();
            if(ret != NO_ERROR)
            {
                CAMHAL_LOGEA("switchToExec error!");
                goto EXIT;
            }
            ret = fillAllOutputBuffers();
            if(ret != NO_ERROR)
            {
                CAMHAL_LOGEA("fillAllOutputBuffers error!");
                goto EXIT;
            }
        }

#ifndef CAMERA_VCEJPEG_KEEPIDLE
        if(exifParam != NULL)
        {
            //set exif
            ret = setExif(exifParam);
            if(ret != NO_ERROR)
            {
                CAMHAL_LOGEA("setExif  error!");
                goto EXIT;
            }

        }
#endif

        cropRect.cropx = param->xoff;
        cropRect.cropy = param->yoff;
        cropRect.cropw = param->width;//&(~0xf);
        cropRect.croph = param->height;//&(~0xf);
        ret = processImageSync(param->inData,&cropRect, &(outBuf),&outOffset, &(outSize));
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("processImageSync error!");
            goto EXIT;
        }
        param->outData = outBuf;
        param->outDataSize = outSize;
        param->outDataOffset = outOffset;
        if(outSize==0)
        {
            freeEncodeData(outBuf);
            CAMHAL_LOGEA("processImageSync outSize is 0, error!");
            ret = UNKNOWN_ERROR;
            goto EXIT;
        }
        memcpy(&mLastParam, param,sizeof(mLastParam));
        return ret;
    EXIT:
        memcpy(&mLastParam, param,sizeof(mLastParam));
        param->outData = NULL;
        param->outDataSize = 0;
        param->outDataOffset=0;

        LOG_FUNCTION_NAME_EXIT

        return ret;
    }
}


status_t OMXVceJpegEncoder::encode(ImageJpegEncoderParam *param)
{
    return encode(param, NULL);
}


status_t OMXVceJpegEncoder::freeEncodeDataLocked(void *buf)
{
    LOG_FUNCTION_NAME
    if(buf != NULL)
    {
        freeImageData(buf);
    }
#ifdef CAMERA_VCEJPEG_KEEPIDLE
    switchToLoaded();
#endif

    LOG_FUNCTION_NAME_EXIT
    return NO_ERROR;
}
status_t OMXVceJpegEncoder::freeEncodeData(void *buf)
{
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mJpegLock);
    freeEncodeDataLocked(buf);

    LOG_FUNCTION_NAME_EXIT
    return NO_ERROR;
}

status_t  OMXVceJpegEncoder::stopEncode()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME
        
    Mutex::Autolock lock(mJpegLock);
    ret = switchToLoaded();

    LOG_FUNCTION_NAME_EXIT
        
return ret;
}

status_t OMXVceImageResize::init()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME
    ret = OMXVce::init(OMXVceEncoderType, true);
    LOG_FUNCTION_NAME_EXIT
    return ret;
}

status_t OMXVceImageResize::setPortParameters()
{

    status_t ret = NO_ERROR;
    {
        Mutex::Autolock lock(mResizeLock);

        //set port definition
        ret = getPortDefinition(mInputPortIndex, mInputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("getPortDefinition  error!");
            goto EXIT;
        }

        mInputPortParam.nBufferCountActual = mImageInputCnt;
        mInputPortParam.format.video.nFrameWidth = mImageWidth;
        mInputPortParam.format.video.nFrameHeight = mImageHeight;
        mInputPortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)mImageFormat;
        mInputPortParam.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingUnused;

        CAMHAL_LOGDB("inputPortParam.nBufferCountActual = %d", (int)mInputPortParam.nBufferCountActual);

        ret = setPortDefinition(mInputPortIndex, mInputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("setPortDefinition  error!");
            goto EXIT;
        }

        ret = getPortDefinition(mOutputPortIndex, mOutputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("getPortDefinition  error!");
            goto EXIT;
        }
        mOutputPortParam.nBufferCountActual = 2;
        mOutputPortParam.format.video.nFrameWidth = mOutputImageWidth;
        mOutputPortParam.format.video.nFrameHeight = mOutputImageHeight;
        mOutputPortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)mImageFormat;
        mOutputPortParam.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingUnused;

        ret = setPortDefinition(mOutputPortIndex, mOutputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("setPortDefinition  error!");
            goto EXIT;
        }
        //get the real buffer size for output buffer
        ret = getPortDefinition(mOutputPortIndex, mOutputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("getPortDefinition  error!");
            goto EXIT;
        }

        //use buffers
        ret = useBuffers(mOutputPortIndex, mOutputPortParam.nBufferSize, NULL, mOutputPortParam.nBufferCountActual);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("useBuffers error!");
            goto EXIT;
        }
    EXIT:
        return ret;
    }
}
status_t  OMXVceImageResize::prepareEncode()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME
    {
        Mutex::Autolock lock(mResizeLock);
        //to Idle
        ret = loadedToIdle();
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("loadedToIdle  error!");
            goto EXIT;
        }

        //to Idle
        ret = switchToExec();
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("switchToExec  error!");
            goto EXIT;
        }

        ret = fillAllOutputBuffers();
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("fillAllOutputBuffers  error!");
            goto EXIT;
        }
        LOG_FUNCTION_NAME_EXIT

        return ret;

        EXIT:
        LOG_FUNCTION_NAME_EXIT
        return ret;
    }
}
status_t  OMXVceImageResize::stopEncode()
{
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mResizeLock);
    return switchToLoaded();
}

status_t OMXVceImageResize::encode(ImageResizeParam *param)
{
    status_t ret = NO_ERROR;
    VceCropRect crop;

    LOG_FUNCTION_NAME
    {
        Mutex::Autolock lock(mResizeLock);
        crop.cropx = param->xoff;
        crop.cropy = param->yoff;
        crop.cropw = param->width;
        crop.croph = param->height;
        ret = processImageSync(param->inData, &crop, &param->outData,&param->outDataOffset, &param->outDataSize);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("processImageSync error!");
            goto EXIT;
        }
    }

    LOG_FUNCTION_NAME_EXIT
    return ret;

EXIT:

    LOG_FUNCTION_NAME_EXIT
    return ret;
}

status_t OMXVceImageResize::freeEncodeData(void *buf)
{
    LOG_FUNCTION_NAME

    Mutex::Autolock lock(mResizeLock);
    if(buf != NULL)
    {
        freeImageData(buf);
    }
    LOG_FUNCTION_NAME_EXIT
    return NO_ERROR;
}


status_t OMXVceFaceDetect::init()
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME
    mImageFormat = OMX_COLOR_FormatUnused;
    ret = OMXVce::init(OMXVceFaceDetectType, false);
    ret |= setFaceDetect(OMX_TRUE);
    LOG_FUNCTION_NAME_EXIT
    return ret;
}
status_t OMXVceFaceDetect::setFaceDetect(OMX_BOOL enable)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    LOG_FUNCTION_NAME


    OMX_ACTIONS_Params  fdParam;
    OMX_INIT_STRUCT_PTR (&fdParam, OMX_ACTIONS_Params);

    fdParam.nPortIndex = OMXVceFDOutputPort;
    fdParam.bEnable = enable;
    CAMHAL_LOGIB("mOmxHandle=%p!",mOmxHandle);
    CAMHAL_LOGIB("fdParam=%p!",&fdParam);
    eError = OMX_SetConfig(mOmxHandle, static_cast<OMX_INDEXTYPE>(OMX_ACT_IndexConfig_FACEDETECTION), &fdParam);
    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
EXIT:
    LOG_FUNCTION_NAME_EXIT
    return ret;
}
status_t OMXVceFaceDetect::setFaceDetectInfo(OMX_U32  sensor,OMX_U32  orientation)
{
    status_t ret = NO_ERROR;

    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    LOG_FUNCTION_NAME

    Mutex::Autolock lock(mFDLock);
    OMX_ACT_PARAM_FaceDetType  fdInfo;
    OMX_INIT_STRUCT_PTR (&fdInfo, OMX_ACT_PARAM_FaceDetType);

    fdInfo.nPortIndex = OMXVceFDOutputPort;
    fdInfo.nAngle = orientation;
    fdInfo.isFrontCamera = sensor==0 ? OMX_FALSE:OMX_TRUE;
    CAMHAL_LOGVB("setFaceDetectInfo  %d, %d!", sensor, orientation);

    eError = OMX_SetConfig(mOmxHandle, static_cast<OMX_INDEXTYPE>(OMX_ACT_IndexParmaFaceDet), &fdInfo);
    OMXVCE_GOTO_EXIT_IF((eError!=OMX_ErrorNone), ErrorUtils::omxToAndroidError(eError));
EXIT:
    LOG_FUNCTION_NAME_EXIT
    return ret;
}


status_t OMXVceFaceDetect::startFaceDetect()
{
    //
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME
    {
        Mutex::Autolock lock(mFDLock);
        ret = switchToLoaded();
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("switchToLoaded  error!");
            goto EXIT;
        }

        //set port definition
        ret = getPortDefinition(mInputPortIndex, mInputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("getPortDefinition  error!");
            goto EXIT;
        }

        mInputPortParam.nBufferCountActual = mImageInputCnt;
        mInputPortParam.format.video.nFrameWidth = mImageWidth;
        mInputPortParam.format.video.nFrameHeight = mImageHeight;
        mInputPortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)mImageFormat;
        mInputPortParam.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingUnused;

        ret = setPortDefinition(mInputPortIndex, mInputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("setPortDefinition  error!");
            goto EXIT;
        }

        ret = getPortDefinition(mOutputPortIndex, mOutputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("getPortDefinition  error!");
            goto EXIT;
        }

        mOutputPortParam.nBufferCountActual = 2;
        mOutputPortParam.format.video.nFrameWidth = mImageWidth;
        mOutputPortParam.format.video.nFrameHeight = mImageHeight;
        mOutputPortParam.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)mImageFormat;
        mOutputPortParam.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingUnused;

        ret = setPortDefinition(mOutputPortIndex, mOutputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("setPortDefinition  error!");
            goto EXIT;
        }


        ret = getPortDefinition(mOutputPortIndex, mOutputPortParam);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("getPortDefinition  error!");
            goto EXIT;
        }

        //use buffers
        ret = useBuffers(mOutputPortIndex, mOutputPortParam.nBufferSize, NULL, mOutputPortParam.nBufferCountActual);
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("useBuffers error!");
            goto EXIT;
        }



        //to Idle
        ret = loadedToIdle();
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("loadedToIdle  error!");
            goto EXIT;
        }

        //to Idle
        ret = switchToExec();
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("switchToExec  error!");
            goto EXIT;
        }

        ret = fillAllOutputBuffers();
        if(ret != NO_ERROR)
        {
            CAMHAL_LOGEA("fillAllOutputBuffers  error!");
            goto EXIT;
        }

        LOG_FUNCTION_NAME_EXIT
        return ret;

    EXIT:

        switchToLoaded();

        LOG_FUNCTION_NAME_EXIT
        return ret;
    }

}

status_t OMXVceFaceDetect::stopFaceDetect()
{
    Mutex::Autolock lock(mFDLock);
    return switchToLoaded();
}

status_t OMXVceFaceDetect::faceDetect(void *data, VceCropRect *crop,void **outData, unsigned int *offset, unsigned int *outSize)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME
    {
        Mutex::Autolock lock(mFDLock);
        if(mSync)
        {

            ret = processImageSync(data, crop, outData, offset, outSize);
            if(ret != NO_ERROR)
            {
                CAMHAL_LOGEA("processImageSync error!");
                goto EXIT;
            }
        }
        else
        {
            ret = processImageAsync(data, crop);
            if(ret != NO_ERROR)
            {
                CAMHAL_LOGEA("processImageSync error!");
                goto EXIT;
            }

        }
    }

    LOG_FUNCTION_NAME_EXIT
    return ret;

EXIT:

    LOG_FUNCTION_NAME_EXIT
    return ret;

}

status_t OMXVceFaceDetect::freeFaceDetectData(void *outData)
{
    /*Deadlock with stopFaceDetect, when switch omx to idle state, the omx will call onFillBufferDone, and onFillBufferDone was blocked by mFDLock in here.
    */
    //Mutex::Autolock lock(mFDLock);
    return freeImageData(outData);
}

};

