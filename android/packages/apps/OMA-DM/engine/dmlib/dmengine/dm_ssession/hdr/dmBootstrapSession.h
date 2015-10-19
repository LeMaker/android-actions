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

#ifndef DM_BOOTSTRAPSESSION_H
#define DM_BOOTSTRAPSESSION_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: dmExtractServerIDSession.h

    General Description: This file contains declaration of DMExractServerIDSession calls.

==================================================================================================*/

#include "dmSession.h"

/*==================================================================================================
                                 CLASSES DECLARATION
==================================================================================================*/

class DMBootstrapSession : public DMSession
{

  public:
 
    virtual SYNCML_DM_RET_STATUS_T Start(const UINT8 *docInputBuffer , 
                                          UINT32 inDocSize, 
                                          BOOLEAN isWBXML, 
                                          DMString & serverID);

  private:
    /* This function will be called to register the DM Engine with the SyncML Toolkit. */
    virtual SYNCML_DM_RET_STATUS_T SetToolkitCallbacks(SmlCallbacks_t * pSmlCallbacks);

 
 
   
};

#endif /* DMProcessScriptSession */
