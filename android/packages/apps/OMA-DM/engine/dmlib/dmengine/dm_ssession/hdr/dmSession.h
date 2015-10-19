/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef DMSESSION_H
#define DMSESSION_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: DMSession.h

    General Description: This file contains declaration of of base DMSession class.

==================================================================================================*/

#include "dmtDefs.h"
#include "dmMemory.h"
#include "dmSessionDefs.h"

extern "C" {
#include "sml.h"
#include "smlerr.h"
}

/*==================================================================================================
                                 CLASSES DECLARATION
==================================================================================================*/

class DMSession
{

  public:
    DMSession();

    virtual ~DMSession();

    inline void* operator new(size_t sz)
    {
       return (DmAllocMem(sz));
    }
        
    inline void operator delete(void* buf)
    {
      DmFreeMem(buf);
    }
        
 protected:
    virtual SYNCML_DM_RET_STATUS_T Init(BOOLEAN isWBXML);
 
    /* This function will be called to register the DM Engine with the SyncML Toolkit. */
    SYNCML_DM_RET_STATUS_T RegisterDmEngineWithSyncmlToolkit(VoidPtr_t pUserData);
       
    /* This function will be called to register the DM Engine with the SyncML Toolkit. */
    virtual SYNCML_DM_RET_STATUS_T SetToolkitCallbacks(SmlCallbacks_t * pSmlCallbacks);

    /* This function will be called to un-register the DM Engine with the SyncML Toolkit. */
    virtual void UnRegisterDmEngineWithSyncmlToolkit();


 protected:

    InstanceID_t  sendInstanceId;     /* Reference for Send DM doc SyncML instance */      
    InstanceID_t  recvInstanceId;     /* Reference for Recv DM doc SyncML instance */
      
    MemPtr_t      pReadPos;           /* Workspace Pointer from which data can be read */
    MemPtr_t      pWritePos;          /* Workspace Pointer to which data can be written */

    MemSize_t     workspaceFreeSize;  /* Max free Size of available space for data */
    MemSize_t     workspaceUsedSize;  /* Size of used data in workspace which may be
                                                       read */  
    UINT8         *smlContentType;
    SmlEncoding_t  smlEncodingType;

    SYNCML_DM_INDIRECT_BUFFER_T   sendSmlDoc;   /* The SyncML document to be send to the 
                                                       server */ 
    SYNCML_DM_INDIRECT_BUFFER_T   recvSmlDoc;   /* The SycnML document to be received from
                                                       the server */
};

#endif /* SYNCML_DM_MGMTSESSION_H */
