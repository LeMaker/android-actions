/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <malloc.h>
#include <string.h>
#include <pthread.h>

#include "RenderScript.h"
#include "rsCppStructs.h"
#include "rsCppInternal.h"

#include <dlfcn.h>
#include <unistd.h>

#if !defined(RS_SERVER) && !defined(RS_COMPATIBILITY_LIB) && defined(HAVE_ANDROID_OS)
#include <cutils/properties.h>
#else
#include "rsCompatibilityLib.h"
#endif


using namespace android;
using namespace RSC;

bool RS::gInitialized = false;
bool RS::usingNative = false;
pthread_mutex_t RS::gInitMutex = PTHREAD_MUTEX_INITIALIZER;
dispatchTable* RS::dispatch = NULL;
static int gInitError = 0;

RS::RS() {
    mDev = NULL;
    mContext = NULL;
    mErrorFunc = NULL;
    mMessageFunc = NULL;
    mMessageRun = false;
    mInit = false;
    mCurrentError = RS_SUCCESS;

    memset(&mElements, 0, sizeof(mElements));
    memset(&mSamplers, 0, sizeof(mSamplers));
}

RS::~RS() {
    if (mInit == true) {
        mMessageRun = false;

        if (mContext) {
            RS::dispatch->ContextDeinitToClient(mContext);

            void *res = NULL;
            int status = pthread_join(mMessageThreadId, &res);

            RS::dispatch->ContextDestroy(mContext);
            mContext = NULL;
        }
        if (mDev) {
            RS::dispatch->DeviceDestroy(mDev);
            mDev = NULL;
        }
    }
}

bool RS::init(std::string name, uint32_t flags) {
    return RS::init(name, RS_VERSION, flags);
}

static bool loadSymbols(void* handle) {

    RS::dispatch->AllocationGetType = (AllocationGetTypeFnPtr)dlsym(handle, "rsaAllocationGetType");
    if (RS::dispatch->AllocationGetType == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationGetType");
        return false;
    }
    RS::dispatch->TypeGetNativeData = (TypeGetNativeDataFnPtr)dlsym(handle, "rsaTypeGetNativeData");
    if (RS::dispatch->TypeGetNativeData == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->TypeGetNativeData");
        return false;
    }
    RS::dispatch->ElementGetNativeData = (ElementGetNativeDataFnPtr)dlsym(handle, "rsaElementGetNativeData");
    if (RS::dispatch->ElementGetNativeData == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ElementGetNativeData");
        return false;
    }
    RS::dispatch->ElementGetSubElements = (ElementGetSubElementsFnPtr)dlsym(handle, "rsaElementGetSubElements");
    if (RS::dispatch->ElementGetSubElements == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ElementGetSubElements");
        return false;
    }
    RS::dispatch->DeviceCreate = (DeviceCreateFnPtr)dlsym(handle, "rsDeviceCreate");
    if (RS::dispatch->DeviceCreate == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->DeviceCreate");
        return false;
    }
    RS::dispatch->DeviceDestroy = (DeviceDestroyFnPtr)dlsym(handle, "rsDeviceDestroy");
    if (RS::dispatch->DeviceDestroy == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->DeviceDestroy");
        return false;
    }
    RS::dispatch->DeviceSetConfig = (DeviceSetConfigFnPtr)dlsym(handle, "rsDeviceSetConfig");
    if (RS::dispatch->DeviceSetConfig == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->DeviceSetConfig");
        return false;
    }
    RS::dispatch->ContextCreate = (ContextCreateFnPtr)dlsym(handle, "rsContextCreate");;
    if (RS::dispatch->ContextCreate == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ContextCreate");
        return false;
    }
    RS::dispatch->GetName = (GetNameFnPtr)dlsym(handle, "rsaGetName");;
    if (RS::dispatch->GetName == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->GetName");
        return false;
    }
    RS::dispatch->ContextDestroy = (ContextDestroyFnPtr)dlsym(handle, "rsContextDestroy");
    if (RS::dispatch->ContextDestroy == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ContextDestroy");
        return false;
    }
    RS::dispatch->ContextGetMessage = (ContextGetMessageFnPtr)dlsym(handle, "rsContextGetMessage");
    if (RS::dispatch->ContextGetMessage == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ContextGetMessage");
        return false;
    }
    RS::dispatch->ContextPeekMessage = (ContextPeekMessageFnPtr)dlsym(handle, "rsContextPeekMessage");
    if (RS::dispatch->ContextPeekMessage == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ContextPeekMessage");
        return false;
    }
    RS::dispatch->ContextSendMessage = (ContextSendMessageFnPtr)dlsym(handle, "rsContextSendMessage");
    if (RS::dispatch->ContextSendMessage == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ContextSendMessage");
        return false;
    }
    RS::dispatch->ContextInitToClient = (ContextInitToClientFnPtr)dlsym(handle, "rsContextInitToClient");
    if (RS::dispatch->ContextInitToClient == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ContextInitToClient");
        return false;
    }
    RS::dispatch->ContextDeinitToClient = (ContextDeinitToClientFnPtr)dlsym(handle, "rsContextDeinitToClient");
    if (RS::dispatch->ContextDeinitToClient == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ContextDeinitToClient");
        return false;
    }
    RS::dispatch->TypeCreate = (TypeCreateFnPtr)dlsym(handle, "rsTypeCreate");
    if (RS::dispatch->TypeCreate == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->TypeCreate");
        return false;
    }
    RS::dispatch->AllocationCreateTyped = (AllocationCreateTypedFnPtr)dlsym(handle, "rsAllocationCreateTyped");
    if (RS::dispatch->AllocationCreateTyped == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationCreateTyped");
        return false;
    }
    RS::dispatch->AllocationCreateFromBitmap = (AllocationCreateFromBitmapFnPtr)dlsym(handle, "rsAllocationCreateFromBitmap");
    if (RS::dispatch->AllocationCreateFromBitmap == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationCreateFromBitmap");
        return false;
    }
    RS::dispatch->AllocationCubeCreateFromBitmap = (AllocationCubeCreateFromBitmapFnPtr)dlsym(handle, "rsAllocationCubeCreateFromBitmap");
    if (RS::dispatch->AllocationCubeCreateFromBitmap == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationCubeCreateFromBitmap");
        return false;
    }
    RS::dispatch->AllocationGetSurface = (AllocationGetSurfaceFnPtr)dlsym(handle, "rsAllocationGetSurface");
    if (RS::dispatch->AllocationGetSurface == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationGetSurface");
        return false;
    }
    RS::dispatch->AllocationSetSurface = (AllocationSetSurfaceFnPtr)dlsym(handle, "rsAllocationSetSurface");
    if (RS::dispatch->AllocationSetSurface == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationSetSurface");
        return false;
    }
    RS::dispatch->ContextFinish = (ContextFinishFnPtr)dlsym(handle, "rsContextFinish");
    if (RS::dispatch->ContextFinish == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ContextFinish");
        return false;
    }
    RS::dispatch->ContextDump = (ContextDumpFnPtr)dlsym(handle, "rsContextDump");
    if (RS::dispatch->ContextDump == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ContextDump");
        return false;
    }
    RS::dispatch->ContextSetPriority = (ContextSetPriorityFnPtr)dlsym(handle, "rsContextSetPriority");
    if (RS::dispatch->ContextSetPriority == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ContextSetPriority");
        return false;
    }
    RS::dispatch->AssignName = (AssignNameFnPtr)dlsym(handle, "rsAssignName");
    if (RS::dispatch->AssignName == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AssignName");
        return false;
    }
    RS::dispatch->ObjDestroy = (ObjDestroyFnPtr)dlsym(handle, "rsObjDestroy");
    if (RS::dispatch->ObjDestroy == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ObjDestroy");
        return false;
    }
    RS::dispatch->ElementCreate = (ElementCreateFnPtr)dlsym(handle, "rsElementCreate");
    if (RS::dispatch->ElementCreate == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ElementCreate");
        return false;
    }
    RS::dispatch->ElementCreate2 = (ElementCreate2FnPtr)dlsym(handle, "rsElementCreate2");
    if (RS::dispatch->ElementCreate2 == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ElementCreate2");
        return false;
    }
    RS::dispatch->AllocationCopyToBitmap = (AllocationCopyToBitmapFnPtr)dlsym(handle, "rsAllocationCopyToBitmap");
    if (RS::dispatch->AllocationCopyToBitmap == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationCopyToBitmap");
        return false;
    }
    RS::dispatch->Allocation1DData = (Allocation1DDataFnPtr)dlsym(handle, "rsAllocation1DData");
    if (RS::dispatch->Allocation1DData == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->Allocation1DData");
        return false;
    }
    RS::dispatch->Allocation1DElementData = (Allocation1DElementDataFnPtr)dlsym(handle, "rsAllocation1DElementData");
    if (RS::dispatch->Allocation1DElementData == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->Allocation1DElementData");
        return false;
    }
    RS::dispatch->Allocation2DData = (Allocation2DDataFnPtr)dlsym(handle, "rsAllocation2DData");
    if (RS::dispatch->Allocation2DData == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->Allocation2DData");
        return false;
    }
    RS::dispatch->Allocation3DData = (Allocation3DDataFnPtr)dlsym(handle, "rsAllocation3DData");
    if (RS::dispatch->Allocation3DData == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->Allocation3DData");
        return false;
    }
    RS::dispatch->AllocationGenerateMipmaps = (AllocationGenerateMipmapsFnPtr)dlsym(handle, "rsAllocationGenerateMipmaps");
    if (RS::dispatch->AllocationGenerateMipmaps == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationGenerateMipmaps");
        return false;
    }
    RS::dispatch->AllocationRead = (AllocationReadFnPtr)dlsym(handle, "rsAllocationRead");
    if (RS::dispatch->AllocationRead == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationRead");
        return false;
    }
    RS::dispatch->Allocation1DRead = (Allocation1DReadFnPtr)dlsym(handle, "rsAllocation1DRead");
    if (RS::dispatch->Allocation1DRead == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->Allocation1DRead");
        return false;
    }
    RS::dispatch->Allocation2DRead = (Allocation2DReadFnPtr)dlsym(handle, "rsAllocation2DRead");
    if (RS::dispatch->Allocation2DRead == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->Allocation2DRead");
        return false;
    }
    RS::dispatch->AllocationSyncAll = (AllocationSyncAllFnPtr)dlsym(handle, "rsAllocationSyncAll");
    if (RS::dispatch->AllocationSyncAll == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationSyncAll");
        return false;
    }
    RS::dispatch->AllocationResize1D = (AllocationResize1DFnPtr)dlsym(handle, "rsAllocationResize1D");
    if (RS::dispatch->AllocationResize1D == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationResize1D");
        return false;
    }
    RS::dispatch->AllocationCopy2DRange = (AllocationCopy2DRangeFnPtr)dlsym(handle, "rsAllocationCopy2DRange");
    if (RS::dispatch->AllocationCopy2DRange == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationCopy2DRange");
        return false;
    }
    RS::dispatch->AllocationCopy3DRange = (AllocationCopy3DRangeFnPtr)dlsym(handle, "rsAllocationCopy3DRange");
    if (RS::dispatch->AllocationCopy3DRange == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationCopy3DRange");
        return false;
    }
    RS::dispatch->SamplerCreate = (SamplerCreateFnPtr)dlsym(handle, "rsSamplerCreate");
    if (RS::dispatch->SamplerCreate == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->SamplerCreate");
        return false;
    }
    RS::dispatch->ScriptBindAllocation = (ScriptBindAllocationFnPtr)dlsym(handle, "rsScriptBindAllocation");
    if (RS::dispatch->ScriptBindAllocation == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptBindAllocation");
        return false;
    }
    RS::dispatch->ScriptSetTimeZone = (ScriptSetTimeZoneFnPtr)dlsym(handle, "rsScriptSetTimeZone");
    if (RS::dispatch->ScriptSetTimeZone == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptSetTimeZone");
        return false;
    }
    RS::dispatch->ScriptInvoke = (ScriptInvokeFnPtr)dlsym(handle, "rsScriptInvoke");
    if (RS::dispatch->ScriptInvoke == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptInvoke");
        return false;
    }
    RS::dispatch->ScriptInvokeV = (ScriptInvokeVFnPtr)dlsym(handle, "rsScriptInvokeV");
    if (RS::dispatch->ScriptInvokeV == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptInvokeV");
        return false;
    }
    RS::dispatch->ScriptForEach = (ScriptForEachFnPtr)dlsym(handle, "rsScriptForEach");
    if (RS::dispatch->ScriptForEach == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptForEach");
        return false;
    }
    RS::dispatch->ScriptSetVarI = (ScriptSetVarIFnPtr)dlsym(handle, "rsScriptSetVarI");
    if (RS::dispatch->ScriptSetVarI == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptSetVarI");
        return false;
    }
    RS::dispatch->ScriptSetVarObj = (ScriptSetVarObjFnPtr)dlsym(handle, "rsScriptSetVarObj");
    if (RS::dispatch->ScriptSetVarObj == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptSetVarObj");
        return false;
    }
    RS::dispatch->ScriptSetVarJ = (ScriptSetVarJFnPtr)dlsym(handle, "rsScriptSetVarJ");
    if (RS::dispatch->ScriptSetVarJ == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptSetVarJ");
        return false;
    }
    RS::dispatch->ScriptSetVarF = (ScriptSetVarFFnPtr)dlsym(handle, "rsScriptSetVarF");
    if (RS::dispatch->ScriptSetVarF == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptSetVarF");
        return false;
    }
    RS::dispatch->ScriptSetVarD = (ScriptSetVarDFnPtr)dlsym(handle, "rsScriptSetVarD");
    if (RS::dispatch->ScriptSetVarD == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptSetVarD");
        return false;
    }
    RS::dispatch->ScriptSetVarV = (ScriptSetVarVFnPtr)dlsym(handle, "rsScriptSetVarV");
    if (RS::dispatch->ScriptSetVarV == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptSetVarV");
        return false;
    }
    RS::dispatch->ScriptGetVarV = (ScriptGetVarVFnPtr)dlsym(handle, "rsScriptGetVarV");
    if (RS::dispatch->ScriptGetVarV == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptGetVarV");
        return false;
    }
    RS::dispatch->ScriptSetVarVE = (ScriptSetVarVEFnPtr)dlsym(handle, "rsScriptSetVarVE");
    if (RS::dispatch->ScriptSetVarVE == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptSetVarVE");
        return false;
    }
    RS::dispatch->ScriptCCreate = (ScriptCCreateFnPtr)dlsym(handle, "rsScriptCCreate");
    if (RS::dispatch->ScriptCCreate == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptCCreate");
        return false;
    }
    RS::dispatch->ScriptIntrinsicCreate = (ScriptIntrinsicCreateFnPtr)dlsym(handle, "rsScriptIntrinsicCreate");
    if (RS::dispatch->ScriptIntrinsicCreate == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptIntrinsicCreate");
        return false;
    }
    RS::dispatch->ScriptKernelIDCreate = (ScriptKernelIDCreateFnPtr)dlsym(handle, "rsScriptKernelIDCreate");
    if (RS::dispatch->ScriptKernelIDCreate == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptKernelIDCreate");
        return false;
    }
    RS::dispatch->ScriptFieldIDCreate = (ScriptFieldIDCreateFnPtr)dlsym(handle, "rsScriptFieldIDCreate");
    if (RS::dispatch->ScriptFieldIDCreate == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptFieldIDCreate");
        return false;
    }
    RS::dispatch->ScriptGroupCreate = (ScriptGroupCreateFnPtr)dlsym(handle, "rsScriptGroupCreate");
    if (RS::dispatch->ScriptGroupCreate == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptGroupCreate");
        return false;
    }
    RS::dispatch->ScriptGroupSetOutput = (ScriptGroupSetOutputFnPtr)dlsym(handle, "rsScriptGroupSetOutput");
    if (RS::dispatch->ScriptGroupSetOutput == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptGroupSetOutput");
        return false;
    }
    RS::dispatch->ScriptGroupSetInput = (ScriptGroupSetInputFnPtr)dlsym(handle, "rsScriptGroupSetInput");
    if (RS::dispatch->ScriptGroupSetInput == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptGroupSetInput");
        return false;
    }
    RS::dispatch->ScriptGroupExecute = (ScriptGroupExecuteFnPtr)dlsym(handle, "rsScriptGroupExecute");
    if (RS::dispatch->ScriptGroupExecute == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->ScriptGroupExecute");
        return false;
    }
    RS::dispatch->AllocationIoSend = (AllocationIoSendFnPtr)dlsym(handle, "rsAllocationIoSend");
    if (RS::dispatch->AllocationIoSend == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationIoSend");
        return false;
    }
    RS::dispatch->AllocationIoReceive = (AllocationIoReceiveFnPtr)dlsym(handle, "rsAllocationIoReceive");
    if (RS::dispatch->AllocationIoReceive == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationIoReceive");
        return false;
    }
    RS::dispatch->AllocationGetPointer = (AllocationGetPointerFnPtr)dlsym(handle, "rsAllocationGetPointer");
    if (RS::dispatch->AllocationGetPointer == NULL) {
        ALOGV("Couldn't initialize RS::dispatch->AllocationGetPointer");
        //return false;
    }

    return true;
}

// this will only open API 19+ libRS
// because that's when we changed libRS to extern "C" entry points
static bool loadSO(const char* filename) {
    void* handle = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    if (handle == NULL) {
        ALOGV("couldn't dlopen %s, %s", filename, dlerror());
        return false;
    }

    if (loadSymbols(handle) == false) {
        ALOGV("%s init failed!", filename);
        return false;
    }
    //ALOGE("Successfully loaded %s", filename);
    return true;
}

static uint32_t getProp(const char *str) {
#if !defined(__LP64__) && !defined(RS_SERVER) && defined(HAVE_ANDROID_OS)
    char buf[256];
    property_get(str, buf, "0");
    return atoi(buf);
#else
    return 0;
#endif
}

bool RS::initDispatch(int targetApi) {
    pthread_mutex_lock(&gInitMutex);
    if (gInitError) {
        goto error;
    } else if (gInitialized) {
        pthread_mutex_unlock(&gInitMutex);
        return true;
    }

    RS::dispatch = new dispatchTable;

    // attempt to load libRS, load libRSSupport on failure
    // if property is set, proceed directly to libRSSupport
    if (getProp("debug.rs.forcecompat") == 0) {
        usingNative = loadSO("libRS.so");
    }
    if (usingNative == false) {
        if (loadSO("libRSSupport.so") == false) {
            ALOGE("Failed to load libRS.so and libRSSupport.so");
            goto error;
        }
    }

    gInitialized = true;

    pthread_mutex_unlock(&gInitMutex);
    return true;

 error:
    gInitError = 1;
    pthread_mutex_unlock(&gInitMutex);
    return false;
}

bool RS::init(std::string &name, int targetApi, uint32_t flags) {
    if (mInit) {
        return true;
    }

    if (initDispatch(targetApi) == false) {
        ALOGE("Couldn't initialize dispatch table");
        return false;
    }

    mCacheDir = name;

    mDev = RS::dispatch->DeviceCreate();
    if (mDev == 0) {
        ALOGE("Device creation failed");
        return false;
    }

    if (flags & ~(RS_CONTEXT_SYNCHRONOUS | RS_CONTEXT_LOW_LATENCY |
                  RS_CONTEXT_LOW_POWER)) {
        ALOGE("Invalid flags passed");
        return false;
    }

    mContext = RS::dispatch->ContextCreate(mDev, 0, targetApi, RS_CONTEXT_TYPE_NORMAL, flags);
    if (mContext == 0) {
        ALOGE("Context creation failed");
        return false;
    }

    pid_t mNativeMessageThreadId;

    int status = pthread_create(&mMessageThreadId, NULL, threadProc, this);
    if (status) {
        ALOGE("Failed to start RS message thread.");
        return false;
    }
    // Wait for the message thread to be active.
    while (!mMessageRun) {
        usleep(1000);
    }

    mInit = true;

    return true;
}

void RS::throwError(RSError error, const char *errMsg) {
    if (mCurrentError == RS_SUCCESS) {
        mCurrentError = error;
        ALOGE("RS CPP error: %s", errMsg);
    } else {
        ALOGE("RS CPP error (masked by previous error): %s", errMsg);
    }
}

RSError RS::getError() {
    return mCurrentError;
}


void * RS::threadProc(void *vrsc) {
    RS *rs = static_cast<RS *>(vrsc);
    size_t rbuf_size = 256;
    void * rbuf = malloc(rbuf_size);

    RS::dispatch->ContextInitToClient(rs->mContext);
    rs->mMessageRun = true;

    while (rs->mMessageRun) {
        size_t receiveLen = 0;
        uint32_t usrID = 0;
        uint32_t subID = 0;
        RsMessageToClientType r = RS::dispatch->ContextPeekMessage(rs->mContext,
                                                                   &receiveLen, sizeof(receiveLen),
                                                                   &usrID, sizeof(usrID));

        if (receiveLen >= rbuf_size) {
            rbuf_size = receiveLen + 32;
            rbuf = realloc(rbuf, rbuf_size);
        }
        if (!rbuf) {
            ALOGE("RS::message handler realloc error %zu", rbuf_size);
            // No clean way to recover now?
        }
        RS::dispatch->ContextGetMessage(rs->mContext, rbuf, rbuf_size, &receiveLen, sizeof(receiveLen),
                            &subID, sizeof(subID));

        switch(r) {
        case RS_MESSAGE_TO_CLIENT_ERROR:
            ALOGE("RS Error %s", (const char *)rbuf);
            rs->throwError(RS_ERROR_RUNTIME_ERROR, "Error returned from runtime");
            if(rs->mMessageFunc != NULL) {
                rs->mErrorFunc(usrID, (const char *)rbuf);
            }
            break;
        case RS_MESSAGE_TO_CLIENT_NONE:
        case RS_MESSAGE_TO_CLIENT_EXCEPTION:
        case RS_MESSAGE_TO_CLIENT_RESIZE:
            // teardown. But we want to avoid starving other threads during
            // teardown by yielding until the next line in the destructor can
            // execute to set mRun = false. Note that the FIFO sends an
            // empty NONE message when it reaches its destructor.
            usleep(1000);
            break;
        case RS_MESSAGE_TO_CLIENT_USER:
            if(rs->mMessageFunc != NULL) {
                rs->mMessageFunc(usrID, rbuf, receiveLen);
            } else {
                ALOGE("Received a message from the script with no message handler installed.");
            }
            break;

        default:
            ALOGE("RS unknown message type %i", r);
        }
    }

    if (rbuf) {
        free(rbuf);
    }
    ALOGV("RS Message thread exiting.");
    return NULL;
}

void RS::setErrorHandler(ErrorHandlerFunc_t func) {
    mErrorFunc = func;
}

void RS::setMessageHandler(MessageHandlerFunc_t func) {
    mMessageFunc  = func;
}

void RS::finish() {
    RS::dispatch->ContextFinish(mContext);
}
