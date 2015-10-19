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

/*============================================================================

    Header Name: dmtAsyncData.h

    General Description: This file contains declaration of structures for 
    async APIs

==============================================================================*/

#ifndef DMT_ASYNC_STRUCT_H
#define DMT_ASYNC_STRUCT_H

/************** HEADER FILE INCLUDES *****************************************/

#include "dmtAsyncAPI.h"
#include "dmbuffer.h"
#include "dmMemory.h"
#include "dmt.hpp"


/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/

class DMAsyncMessage
{

public:
  
    UINT32 messageID;
    UINT32 pUserData;

    DMAsyncMessage()
    {
      messageID = 0;
      pUserData = 0;  

    }  

    inline void* operator new(size_t sz)
    {
      return (DmAllocMem(sz));
    }
        
    inline void operator delete(void* buf)
    {
      DmFreeMem(buf);
    }


    SYNCML_DM_RET_STATUS_T set(UINT32 messageID, UINT32 pUserData)
    {
      this->messageID = messageID;
      this->pUserData = pUserData;
      return SYNCML_DM_SUCCESS;
    } 

};




class DMPrincipalMessage : public DMAsyncMessage
{

public:
     
    DmtPrincipal principal;
 
    SYNCML_DM_RET_STATUS_T set(CPCHAR szPrincipal,
                               UINT32 messageID, 
                               UINT32 pUserData);
};


class DMGetSubTreeMessage : public DMPrincipalMessage
{

public:

   
    DMString subtreeRoot;
    SYNCML_DM_TREE_LOCK_TYPE_T nLockType; 
    DMT_CallbackGetTree callback;

  
    SYNCML_DM_RET_STATUS_T set(CPCHAR szPrincipal, 
                                CPCHAR subtreeRoot,
                                SYNCML_DM_TREE_LOCK_TYPE_T nLockType,
                                DMT_CallbackGetTree callback,
                                UINT32 messageID, 
                                UINT32 pUserData);

};



class DMScriptMessage : public DMPrincipalMessage
{

public:
    DMBuffer buf;
    BOOLEAN isWBXML;

  
    SYNCML_DM_RET_STATUS_T set(CPCHAR szPrincipal,
                               const UINT8 * buf, 
                               INT32 len, 
                               BOOLEAN isWBXML,
                               UINT32 messageID, 
                               UINT32 pUserData);
    
};


class DMProcessScriptMessage : public DMScriptMessage
{

public:
    DMT_CallbackProcessScript callback;

    SYNCML_DM_RET_STATUS_T set(CPCHAR szPrincipal, 
                               const UINT8 * buf, 
                               INT32 len, 
                               BOOLEAN isWBXML,
                               DMT_CallbackProcessScript callback,
                               UINT32 messageID, 
                               UINT32 pUserData);
   
};

class DMBootstrapMessage : public DMScriptMessage
{
public:
    DMT_CallbackBootstrap callback;
    BOOLEAN isProcess;
    
    SYNCML_DM_RET_STATUS_T set(CPCHAR szPrincipal, 
                               const UINT8 * buf, 
                               INT32 len, 
                               BOOLEAN isWBXML,
                               BOOLEAN isProcess,
                               DMT_CallbackBootstrap callback,
                               UINT32 messageID, 
                               UINT32 pUserData);

};


class DMStartServerSessionMessage : public DMPrincipalMessage
{
public:
    DmtSessionProp sessionProp;
    DMT_CallbackStatusCode callback;

    SYNCML_DM_RET_STATUS_T set(CPCHAR szPrincipal, 
                               const DMT_SESSION_PROP_T * pSessionProp,
                               DMT_CallbackStatusCode callback,
                               UINT32 messageID, 
                               UINT32 pUserData);
 
};

class DMProcessNotificationMessage : public DMPrincipalMessage
{
public:
    DMBuffer buf; 
    DMT_CallbackProcessNotification callback;

    SYNCML_DM_RET_STATUS_T set(CPCHAR szPrincipal, 
                               const UINT8 * buf, 
                               INT32 len,
                               DMT_CallbackProcessNotification callback,
                               UINT32 messageID, 
                               UINT32 pUserData);
    
};


class DMTreeMessage : public DMAsyncMessage
{
public:
    DMT_H_TREE htree;
    DMString path;

    SYNCML_DM_RET_STATUS_T set(DMT_H_TREE htree, 
                               CPCHAR path,
                               UINT32 messageID, 
                               UINT32 pUserData);
    
};


class DMGetNodeMessage : public DMTreeMessage
{
public:
    DMT_CallbackGetNode callback;

    SYNCML_DM_RET_STATUS_T set(DMT_H_TREE htree, 
                               CPCHAR path, 
                               DMT_CallbackGetNode callback,
                               UINT32 messageID, 
                               UINT32 pUserData);
    
};


class DMTreeNodeMessage : public DMTreeMessage
{
public:
    DMT_CallbackStatusCode callback;
    DMString str;
    
    SYNCML_DM_RET_STATUS_T set(DMT_H_TREE htree, 
                               CPCHAR path,
                               CPCHAR str,
                               DMT_CallbackStatusCode callback,
                               UINT32 messageID, 
                               UINT32 pUserData);
    
};



class DMCreateLeafNodeMessage : public DMTreeMessage
{
public:
    DmtData data;
    DMT_CallbackStatusCode callback;

    SYNCML_DM_RET_STATUS_T set(DMT_H_TREE htree, 
                               CPCHAR path, 
                               const DMT_DATA_T* data, 
                               DMT_CallbackStatusCode callback,
                               UINT32 messageID, 
                               UINT32 pUserData);
    
};


class DMGetChildNodeNamesMessage : public DMTreeMessage
{
public:
    DMT_CallbackGetChildNodeNames callback;

    SYNCML_DM_RET_STATUS_T set(DMT_H_TREE htree, 
                               CPCHAR path, 
                               DMT_CallbackGetChildNodeNames callback,
                               UINT32 messageID, 
                               UINT32 pUserData);
    
    
};

class DMGetChildValuesMapMessage : public DMTreeMessage
{
public:
    DMT_CallbackGetChildValuesMap callback;

    SYNCML_DM_RET_STATUS_T set(DMT_H_TREE htree, 
                                CPCHAR path, 
                                DMT_CallbackGetChildValuesMap callback,
                                UINT32 messageID, 
                                UINT32 pUserData);
   
};


class DMSetChildValuesMapMessage : public DMTreeMessage
{
public:
    DMMap<DMString, DmtData> data;
    DMT_CallbackStatusCode callback;

    SYNCML_DM_RET_STATUS_T set(DMT_H_TREE htree, 
                               CPCHAR path, 
                               const DMT_LEAF_CHILDREN_DATA_T*  data,
                               DMT_CallbackStatusCode callback,
                               UINT32 messageID, 
                               UINT32 pUserData);
    
};


class DMSetValueMessage : public DMAsyncMessage
{
public:
    DMT_H_NODE hnode;
    DmtData data;
    DMT_CallbackStatusCode callback;

    SYNCML_DM_RET_STATUS_T set(DMT_H_NODE hnode, 
                                const DMT_DATA_T* data, 
                                DMT_CallbackStatusCode callback,
                                UINT32 messageID, 
                                UINT32 pUserData);
    
};


class DMNodeMessage : public DMAsyncMessage
{
public:
    DMString str;
    DMT_H_NODE hnode;
    DMT_CallbackStatusCode callback;

    SYNCML_DM_RET_STATUS_T set(DMT_H_NODE hnode, 
                               CPCHAR str, 
                               DMT_CallbackStatusCode callback,
                               UINT32 messageID, 
                               UINT32 pUserData);
   
};


class DMExecuteMessage :  public DMNodeMessage
{
public:
    DMString params;
    DMT_CallbackExecute callback;

    SYNCML_DM_RET_STATUS_T set(DMT_H_NODE hnode, 
                               CPCHAR params, 
                               DMT_CallbackExecute callback,
                               UINT32 messageID, 
                               UINT32 pUserData);
    
};

#endif
