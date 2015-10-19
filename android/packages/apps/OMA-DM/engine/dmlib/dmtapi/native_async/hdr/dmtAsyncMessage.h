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

    Header Name: dmtAsyncMessage.h

    General Description: This file contains declaration of structures for 
    async APIs

==============================================================================*/

#ifndef DMT_ASYNC_MESSAGE_H
#define DMT_ASYNC_MESSAGE_H

/************** HEADER FILE INCLUDES *****************************************/

#include "dmtAsyncData.h"

/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/

typedef struct
{
    DMT_CallbackStatusCode callback;
    UINT32 messageID;
    UINT32 pUserData;
} SYNCML_DM_ENGINE_MESSAGE_T;


typedef struct
{
    DMT_CallbackNotifyOnIdle callback;
    UINT32 messageID;
    UINT32 pUserData;
} SYNCML_DM_NOTIFY_MESSAGE_T;
 
typedef struct
{
    DMGetSubTreeMessage * pMsg;
} SYNCML_DM_GET_SUB_TREE_MESSAGE_T;


typedef struct
{
    DMProcessScriptMessage * pMsg;
} SYNCML_DM_PROCESS_SCRIPT_MESSAGE_T;

typedef struct
{
    DMBootstrapMessage * pMsg;
} SYNCML_DM_BOOTSTRAP_MESSAGE_T;


typedef struct
{
    DMStartServerSessionMessage * pMsg;
} SYNCML_DM_START_SERVER_SESSION_MESSAGE_T;

typedef struct
{
    DMProcessNotificationMessage * pMsg;
} SYNCML_DM_PROCESS_NOTIFICATION_MESSAGE_T;

typedef struct
{
    DMGetNodeMessage * pMsg;
} SYNCML_DM_GET_NODE_MESSAGE_T;


typedef struct
{
    DMTreeNodeMessage * pMsg;
} SYNCML_DM_TREENODE_MESSAGE_T;


typedef struct
{
    DMCreateLeafNodeMessage * pMsg;
} SYNCML_DM_CREATE_LEAF_NODE_MESSAGE_T;


typedef struct
{
    DMGetChildNodeNamesMessage * pMsg;
} SYNCML_DM_GET_CHILD_NODE_NAMES_MESSAGE_T;


typedef struct
{
    DMT_H_TREE htree;
    DMT_CallbackStatusCode callback;
    UINT32 messageID;
    UINT32 pUserData; 
} SYNCML_DM_TREE_MESSAGE_T;



typedef struct
{
    DMGetChildValuesMapMessage * pMsg;
} SYNCML_DM_GET_CHILD_VALUES_MAP_MESSAGE_T;


typedef struct
{
    DMSetChildValuesMapMessage * pMsg;
} SYNCML_DM_SET_CHILD_VALUES_MAP_MESSAGE_T;


typedef struct
{
    DMT_H_NODE hnode;
    DMT_CallbackGetAttributes callback;
    UINT32 messageID;
    UINT32 pUserData; 
} SYNCML_DM_GET_ATTRIBUTES_MESSAGE_T;


typedef struct
{
    DMT_H_NODE hnode;
    DMT_CallbackGetValue callback;
    UINT32 messageID;
    UINT32 pUserData; 
} SYNCML_DM_GET_VALUE_MESSAGE_T;

typedef struct
{
    DMSetValueMessage * pMsg;
} SYNCML_DM_SET_VALUE_MESSAGE_T;

typedef struct
{
    DMNodeMessage * pMsg;
} SYNCML_DM_NODE_MESSAGE_T;


typedef struct
{
    DMExecuteMessage * pMsg;
} SYNCML_DM_EXECUTE_MESSAGE_T;

#ifdef LOB_SUPPORT
typedef struct
{
    DMT_H_NODE hnode;
    DMT_DATACHUNK_T *datachunk;
    DMT_CallbackDataChunk callback;
    UINT32 messageID;
    UINT32 pUserData; 
} SYNCML_DM_DATA_CHUNK_MESSAGE_T;
#endif

#endif
