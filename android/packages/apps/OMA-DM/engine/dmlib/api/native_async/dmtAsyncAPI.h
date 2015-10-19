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

    Header Name: dmtAsyncAPI.h

    General Description: This file contains declaration of the DMT async APIs

==============================================================================*/

#ifndef DMT_ASYNC_API_H
#define DMT_ASYNC_API_H

#include "dmtDefs.h"

/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/

typedef UINT32 DMT_H_TREE;
typedef UINT32 DMT_H_NODE;

typedef struct 
{
  SYNCML_DM_DATAFORMAT_T  meta_format;
  union 
  {
    CPCHAR    str_value;
    INT32     int_value;  /* int/bool values */
    struct 
    {
      const UINT8* bin_value;
      INT32   len_bin_data;
    } bin;
  } data;

} DMT_DATA_T;

typedef struct {
	UINT8 *chunkBuffer;	/* chunked data buffer */
	INT32 dataSize; 		/* input data size */
	INT32 returnLen; 		/* actual size of data get/set */
} DMT_DATACHUNK_T;

/* helper struct used by bulk operation functions like xxxValuesMap */
typedef struct 
{
  CPCHAR*            ppChildren;   /* array of names (leaf children) */
  const DMT_DATA_T*  pData;        /* array of data (leaf children) */
  INT32              num_children; /* number of elements in data/name arrays */
} DMT_LEAF_CHILDREN_DATA_T;

typedef struct 
{
  CPCHAR  name;
  CPCHAR  format;
  CPCHAR  title;
  CPCHAR  type;
  CPCHAR  acl;
  INT32   version;
  INT32   size;
  INT64   timestamp;
} DMT_ATTRIBUTE_T;


typedef struct 
{
  UINT8  uiMode;
  UINT8  initiator;
  UINT16 sessionID;
  CPCHAR serverID;
  BOOLEAN authFlag;
} DMT_NOTIFICATION_T;


typedef struct
{ 
  CPCHAR strPackageURI;                // URI of update package
  CPCHAR strCorrelator;
  CPCHAR strResultData;
  CPCHAR strAlertType;
  CPCHAR strAlertFormat;
  CPCHAR strAlertMark;
} DMT_FRM_ALERT_T; 


typedef struct 
{
  SYNCML_DM_SESSION_DIRECTION_T direction;
  UINT32 sessionID;
  BOOLEAN isWBXML;
  DMT_FRM_ALERT_T * alerts;
  INT32             num_alerts;

} DMT_SESSION_PROP_T;

/* callback type definition */
typedef enum { 
  DMT_OPERATION_INIT,               
  DMT_OPERATION_UNINIT,             
  DMT_OPERATION_RELEASE_TREE,       
  DMT_OPERATION_START_SRV_SESSION,  
  DMT_OPERATION_RELEASE_NODE,       
  DMT_OPERATION_DELETE_NODE,        
  DMT_OPERATION_RENAME_NODE,        
  DMT_OPERATION_CREATEI_NODE,       
  DMT_OPERATION_CREATEL_NODE,       
  DMT_OPERATION_SET_CHILDVALUES_MAP,
  DMT_OPERATION_FLUSH,              
  DMT_OPERATION_COMMIT,             
  DMT_OPERATION_ROLLBACK,           
  DMT_OPERATION_BEGIN,              
  DMT_OPERATION_SET_VALUE,          
  DMT_OPERATION_SET_TITLE,          
  DMT_OPERATION_SET_ACL,            

};
typedef UINT8 DMT_OPERATION_TYPE_T;



/* callback parameters */
typedef struct 
{
    void* pUserData; 
    DMT_OPERATION_TYPE_T nCallbackType;
    SYNCML_DM_RET_STATUS_T nStatusCode;
} DMT_CALLBACK_STRUCT_STATUS_T;


typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode; 
  DMT_H_TREE  htree;  /* tree handle if succeeded */
} DMT_CALLBACK_STRUCT_GETTREE_T;

typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode;
  CPCHAR  result;     /* result script if succeeded; */
  INT32   result_len; /* length of the result (useful if binary)*/
} DMT_CALLBACK_STRUCT_PROCESS_SCRIPT_T;

typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode; 
  CPCHAR  serverID;   /* server Id extracted from package 0 */
} DMT_CALLBACK_STRUCT_BOOTSTRAP_T;


typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode; 
  DMT_NOTIFICATION_T notification;
} DMT_CALLBACK_STRUCT_PROCESS_NOTIFICATION_T;

typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode; 
  DMT_H_NODE  hnode;    /* node handle if succeeded */
  BOOLEAN     leaf_node;/* true if leaf node */  
  BOOLEAN     external_storage_node;/* true if External Storage node */  
} DMT_CALLBACK_STRUCT_GETNODE_T;

typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode; 
  CPCHAR* ppChildren; /* list of children */
  INT32   num_children;
} DMT_CALLBACK_STRUCT_GET_CHILDNODE_NAMES_T;

typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode; 
  DMT_LEAF_CHILDREN_DATA_T data;         /* name->data list */
} DMT_CALLBACK_STRUCT_GET_CHILDVALUES_MAP_T;

typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode; 
  DMT_ATTRIBUTE_T attributes;   /* node's attributes */
} DMT_CALLBACK_STRUCT_GET_ATTRIBUTES_T;

typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode; 
  DMT_DATA_T data;         /* node's value */
} DMT_CALLBACK_STRUCT_GET_VALUE_T;

typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode; 
  CPCHAR result;       /* Exec's result */
} DMT_CALLBACK_STRUCT_EXECUTE_T;

typedef struct 
{
  void* pUserData; 
  SYNCML_DM_RET_STATUS_T nStatusCode; 
  DMT_DATACHUNK_T *datachunk;
} DMT_CALLBACK_STRUCT_DATA_CHUNK_T;



/* Callback function */

/* Callback function */
typedef void (*DMT_CallbackNotifyOnIdle)(void); 

typedef void (*DMT_CallbackStatusCode)(DMT_CALLBACK_STRUCT_STATUS_T *pStruct); 

typedef void (*DMT_CallbackGetTree)(DMT_CALLBACK_STRUCT_GETTREE_T *pGetTreeStruct);

typedef void (*DMT_CallbackProcessScript)(
     DMT_CALLBACK_STRUCT_PROCESS_SCRIPT_T *pProcessScriptStruct);

typedef void (*DMT_CallbackBootstrap)(
     DMT_CALLBACK_STRUCT_BOOTSTRAP_T *pBootstrapStruct );


typedef void (*DMT_CallbackProcessNotification)( 
    DMT_CALLBACK_STRUCT_PROCESS_NOTIFICATION_T *pProcessNotificationStruct );

typedef void (*DMT_CallbackGetNode)(DMT_CALLBACK_STRUCT_GETNODE_T *pStruct );

typedef void (*DMT_CallbackGetChildNodeNames)(DMT_CALLBACK_STRUCT_GET_CHILDNODE_NAMES_T *pStruct );

typedef void (*DMT_CallbackGetChildValuesMap)(DMT_CALLBACK_STRUCT_GET_CHILDVALUES_MAP_T *pStruct );

typedef void (*DMT_CallbackGetAttributes)(DMT_CALLBACK_STRUCT_GET_ATTRIBUTES_T *pStruct );

typedef void (*DMT_CallbackGetValue)(DMT_CALLBACK_STRUCT_GET_VALUE_T *pStruct );

typedef void (*DMT_CallbackExecute)(DMT_CALLBACK_STRUCT_EXECUTE_T *pStruct );

typedef void (*DMT_CallbackDataChunk)(DMT_CALLBACK_STRUCT_DATA_CHUNK_T *pStruct );


#ifdef __cplusplus
extern "C" {
#endif

/*=================================================================================
                                     FUNCTION PROTOTYPES
                                      APIs with CALLBACK 
==================================================================================*/

SYNCML_DM_RET_STATUS_T  
DMT_NotifyOnIdle(DMT_CallbackNotifyOnIdle callback,
                 void* pUserData );

/* *****  Functions from DmtTreeFactory class ***** */
/* initializes engine, no other functions should be called until it's done */
SYNCML_DM_RET_STATUS_T
DMT_Init(DMT_CallbackStatusCode callback, void* pUserData );

SYNCML_DM_RET_STATUS_T  
DMT_Uninit(DMT_CallbackStatusCode callback, void* pUserData);

SYNCML_DM_RET_STATUS_T
DMT_GetTree(CPCHAR szPrincipal, 
                DMT_CallbackGetTree callback,
                void* pUserData );

SYNCML_DM_RET_STATUS_T
DMT_GetSubtree(CPCHAR szPrincipal, 
                  CPCHAR szSubtreeRoot,
                  DMT_CallbackGetTree callback,
                  void* pUserData );

SYNCML_DM_RET_STATUS_T
DMT_GetSubtreeEx(CPCHAR szPrincipal, 
                  CPCHAR szSubtreeRoot,
                  SYNCML_DM_TREE_LOCK_TYPE_T nLockType, 
                  DMT_CallbackGetTree callback,
                  void* pUserData );

/* caller is responsible for manual deletion of tree object later */
/* this function also returns tree serialization result */
SYNCML_DM_RET_STATUS_T
DMT_TreeRelease(DMT_H_TREE htree,
                 DMT_CallbackStatusCode callback,
                 void* pUserData );


SYNCML_DM_RET_STATUS_T
DMT_ProcessScript(CPCHAR szPrincipal, 
                   const UINT8 * buf, 
                   INT32 len, 
                   BOOLEAN isWBXML,
                   DMT_CallbackProcessScript callback,
                   void* pUserData );


SYNCML_DM_RET_STATUS_T
DMT_StartServerSession(CPCHAR szPrincipal, 
                        const DMT_SESSION_PROP_T * pSessionProp,
                        DMT_CallbackStatusCode callback,
                        void* pUserData );



SYNCML_DM_RET_STATUS_T
DMT_ProcessNotification(CPCHAR szPrincipal,
                        const UINT8 *buf, 
                        INT32 len,
                        DMT_CallbackProcessNotification callback,
                        void* pUserData ); 


SYNCML_DM_RET_STATUS_T
DMT_Bootstrap(CPCHAR szPrincipal,
                  const UINT8 * buf,
                  INT32 len,
                  BOOLEAN isWBXML,
                  BOOLEAN isProcess,
                  DMT_CallbackBootstrap callback,
                  void* pUserData ); 
                                                         

/* *****  Functions from DmtTree class ***** */
SYNCML_DM_RET_STATUS_T 
DMT_GetNode(DMT_H_TREE htree,
             CPCHAR  path,
             DMT_CallbackGetNode callback,
             void* pUserData );

/* release node when it no longer needed */
SYNCML_DM_RET_STATUS_T
DMT_NodeRelease(DMT_H_NODE hnode,
                 DMT_CallbackStatusCode callback,
                 void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_DeleteNode(DMT_H_TREE htree,
                CPCHAR  path,
                DMT_CallbackStatusCode callback,
                void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_RenameNode(DMT_H_TREE htree,
                CPCHAR  path,
                CPCHAR  new_node_name,
                DMT_CallbackStatusCode callback,
                void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_CreateInteriorNode(DMT_H_TREE htree,
                        CPCHAR  path,
                        DMT_CallbackStatusCode callback,
                        void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_CreateLeafNode(DMT_H_TREE htree,
                    CPCHAR path,
                    const DMT_DATA_T* data,
                    DMT_CallbackStatusCode callback,
                    void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_GetChildNodeNames(DMT_H_TREE htree,
                       CPCHAR  path,
                       DMT_CallbackGetChildNodeNames callback,
                       void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_Flush(DMT_H_TREE htree,
           DMT_CallbackStatusCode callback,
           void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_Commit(DMT_H_TREE htree,
            DMT_CallbackStatusCode callback,
            void* pUserData );

SYNCML_DM_RET_STATUS_T
DMT_Rollback(DMT_H_TREE htree, 
              DMT_CallbackStatusCode callback,
              void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_Begin(DMT_H_TREE htree,
           DMT_CallbackStatusCode callback,
           void* pUserData );


/* bulk operations support */
SYNCML_DM_RET_STATUS_T
DMT_GetChildValuesMap(DMT_H_TREE htree,
                       CPCHAR  path,
                       DMT_CallbackGetChildValuesMap callback,
                       void* pUserData );

/**
A helper method, deletes all leaf nodes and creates new ones, provided in the map
The table key is the child node name, and value is the node value  
It changes leaf nodes only
*/
SYNCML_DM_RET_STATUS_T 
DMT_SetChildValuesMap(DMT_H_TREE htree,
                       CPCHAR path,
                       const DMT_LEAF_CHILDREN_DATA_T*  data,
                       DMT_CallbackStatusCode callback,
                       void* pUserData );


/* *****  Functions from DmtNode class ***** */
SYNCML_DM_RET_STATUS_T 
DMT_GetAttributes(DMT_H_NODE hnode,
                   DMT_CallbackGetAttributes callback,
                   void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_GetValue(DMT_H_NODE hnode,
              DMT_CallbackGetValue callback,
              void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_SetValue(DMT_H_NODE hnode,
              const DMT_DATA_T* data,
              DMT_CallbackStatusCode callback,
              void* pUserData );

SYNCML_DM_RET_STATUS_T
DMT_SetTitle(DMT_H_NODE  hnode,
              CPCHAR title,
              DMT_CallbackStatusCode callback,
              void* pUserData );

SYNCML_DM_RET_STATUS_T
DMT_SetACL(DMT_H_NODE hnode,
            CPCHAR acl,
            DMT_CallbackStatusCode callback,
            void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_Execute(DMT_H_NODE hnode,
             CPCHAR params,
             DMT_CallbackExecute callback,
             void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_GetFirstChunk(DMT_H_NODE hnode,
		DMT_DATACHUNK_T *datachunk,
             DMT_CallbackDataChunk  callback,
             void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_GetNextChunk(DMT_H_NODE hnode,
		DMT_DATACHUNK_T *datachunk,
             DMT_CallbackDataChunk  callback,
             void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_SetFirstChunk(DMT_H_NODE hnode,
		DMT_DATACHUNK_T *datachunk,
             DMT_CallbackDataChunk  callback,
             void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_SetNextChunk(DMT_H_NODE hnode,
		DMT_DATACHUNK_T *datachunk,
             DMT_CallbackDataChunk  callback,
             void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_SetLastChunk(DMT_H_NODE hnode,
		DMT_DATACHUNK_T *datachunk,
             DMT_CallbackDataChunk  callback,
             void* pUserData );

INT32 DMT_GetChunkSize();
/*=================================================================================
                                     FUNCTION PROTOTYPES
                                      APIs with messaging 
==================================================================================*/
SYNCML_DM_RET_STATUS_T  
DMT_NotifyOnIdle_Msg(UINT32 messageID,
                           void* pUserData );


SYNCML_DM_RET_STATUS_T
DMT_Init_Msg(UINT32 messageID, void* pUserData);

SYNCML_DM_RET_STATUS_T  
DMT_Uninit_Msg(UINT32 messageID, void* pUserData);

SYNCML_DM_RET_STATUS_T
DMT_GetTree_Msg(CPCHAR szPrincipal, 
                UINT32 messageID,
                void* pUserData );

SYNCML_DM_RET_STATUS_T
DMT_GetSubtree_Msg(CPCHAR szPrincipal, 
                  CPCHAR szSubtreeRoot,
                  UINT32 messageID,
                  void* pUserData );

SYNCML_DM_RET_STATUS_T
DMT_GetSubtreeEx_Msg(CPCHAR szPrincipal, 
                  CPCHAR szSubtreeRoot,
                  SYNCML_DM_TREE_LOCK_TYPE_T nLockType, 
                  UINT32 messageID,
                  void* pUserData );

/* caller is responsible for manual deletion of tree object later */
/* this function also returns tree serialization result */
SYNCML_DM_RET_STATUS_T
DMT_TreeRelease_Msg(DMT_H_TREE htree,
                 UINT32 messageID,
                 void* pUserData );


SYNCML_DM_RET_STATUS_T
DMT_ProcessScript_Msg(CPCHAR szPrincipal, 
                   const UINT8 * buf, 
                   INT32 len, 
                   BOOLEAN isWBXML,
                   UINT32 messageID,
                   void* pUserData );


SYNCML_DM_RET_STATUS_T
DMT_StartServerSession_Msg(CPCHAR szPrincipal, 
                        const DMT_SESSION_PROP_T * pSessionProp,
                        UINT32 messageID,
                        void* pUserData );



SYNCML_DM_RET_STATUS_T
DMT_ProcessNotification_Msg(CPCHAR szPrincipal,
                        const UINT8 *buf, 
                        INT32 len,
                        UINT32 messageID,
                        void* pUserData ); 


SYNCML_DM_RET_STATUS_T
DMT_Bootstrap_Msg(CPCHAR szPrincipal,
                    const UINT8 * buf,
                    INT32 len,
                    BOOLEAN isWBXML,
                    BOOLEAN isProcess,
                    UINT32 messageID,
                    void* pUserData ); 
                                                         

/* *****  Functions from DmtTree class ***** */
SYNCML_DM_RET_STATUS_T 
DMT_GetNode_Msg(DMT_H_TREE htree,
             CPCHAR  path,
             UINT32 messageID,
             void* pUserData );

/* release node when it no longer needed */
SYNCML_DM_RET_STATUS_T
DMT_NodeRelease_Msg(DMT_H_NODE hnode,
                 UINT32 messageID,
                 void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_DeleteNode_Msg(DMT_H_TREE htree,
                CPCHAR  path,
                UINT32 messageID,
                void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_RenameNode_Msg(DMT_H_TREE htree,
                CPCHAR  path,
                CPCHAR  new_node_name,
                UINT32 messageID,
                void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_CreateInteriorNode_Msg(DMT_H_TREE htree,
                        CPCHAR  path,
                        UINT32 messageID,
                        void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_CreateLeafNode_Msg(DMT_H_TREE htree,
                    CPCHAR path,
                    const DMT_DATA_T* data,
                    UINT32 messageID,
                    void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_GetChildNodeNames_Msg(DMT_H_TREE htree,
                       CPCHAR  path,
                       UINT32 messageID,
                       void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_Flush_Msg(DMT_H_TREE htree, UINT32 messageID, void* pUserData);

SYNCML_DM_RET_STATUS_T 
DMT_Commit_Msg(DMT_H_TREE htree, UINT32 messageID,  void* pUserData);

SYNCML_DM_RET_STATUS_T
DMT_Rollback_Msg(DMT_H_TREE htree, UINT32 messageID, void* pUserData);

SYNCML_DM_RET_STATUS_T 
DMT_Begin_Msg(DMT_H_TREE htree, UINT32 messageID, void* pUserData);


SYNCML_DM_RET_STATUS_T
DMT_GetChildValuesMap_Msg(DMT_H_TREE htree,
                       CPCHAR  path,
                       UINT32 messageID,
                       void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_SetChildValuesMap_Msg(DMT_H_TREE htree,
                       CPCHAR path,
                       const DMT_LEAF_CHILDREN_DATA_T*  data,
                       UINT32 messageID,
                       void* pUserData );


/* *****  Functions from DmtNode class ***** */
SYNCML_DM_RET_STATUS_T 
DMT_GetAttributes_Msg(DMT_H_NODE hnode,
                   UINT32 messageID,
                   void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_GetValue_Msg(DMT_H_NODE hnode,
              UINT32 messageID,
              void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_SetValue_Msg(DMT_H_NODE hnode,
              const DMT_DATA_T* data,
              UINT32 messageID,
              void* pUserData );

SYNCML_DM_RET_STATUS_T
DMT_SetTitle_Msg(DMT_H_NODE  hnode,
              CPCHAR title,
              UINT32 messageID,
              void* pUserData );

SYNCML_DM_RET_STATUS_T
DMT_SetACL_Msg(DMT_H_NODE hnode,
            CPCHAR acl,
            UINT32 messageID,
            void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_Execute_Msg(DMT_H_NODE hnode,
             CPCHAR params,
             UINT32 messageID,
             void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_GetFirstChunk_Msg(DMT_H_NODE hnode,
		DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_GetNextChunk_Msg(DMT_H_NODE hnode,
		DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_SetFirstChunk_Msg(DMT_H_NODE hnode,
		DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_SetNextChunk_Msg(DMT_H_NODE hnode,
		DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData );

SYNCML_DM_RET_STATUS_T 
DMT_SetLastChunk_Msg(DMT_H_NODE hnode,
		DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData );

/*=================================================================================
                                     FUNCTION PROTOTYPES
                                    Synchronous helper APIs  
==================================================================================*/


SYNCML_DM_ACL_PERMISSIONS_T
DMT_AclGetPermissions(CPCHAR acl,CPCHAR principal);

/* sets required permission for specified principal 
   if result isn't successful set buf_len into required acl len */
SYNCML_DM_RET_STATUS_T
DMT_AclSetPermissions(char * acl,
                       INT32 * buf_len,
                       CPCHAR principal,
                       SYNCML_DM_ACL_PERMISSIONS_T permissions );


/* Handler of DMT task messages, should be called from DM task */
void DMT_MessageHandler(UINT32 msgtype, void *data); 

void DMT_Free_ProcessScriptStruct(DMT_CALLBACK_STRUCT_PROCESS_SCRIPT_T *pStruct);

void DMT_Free_BootstrapStruct(DMT_CALLBACK_STRUCT_BOOTSTRAP_T *pStruct);

void DMT_Free_ProcessNotificationStruct(DMT_CALLBACK_STRUCT_PROCESS_NOTIFICATION_T *pStruct);

void DMT_Free_GetChildNodeNamesStruct(DMT_CALLBACK_STRUCT_GET_CHILDNODE_NAMES_T *pStruct);

void DMT_Free_GetChildValuesStruct(DMT_CALLBACK_STRUCT_GET_CHILDVALUES_MAP_T *pStruct);

void DMT_Free_GetAttributesStruct(DMT_CALLBACK_STRUCT_GET_ATTRIBUTES_T *pStruct);

void DMT_Free_GetValueStruct(DMT_CALLBACK_STRUCT_GET_VALUE_T *pStruct);  

void DMT_Free_ExecuteStruct(DMT_CALLBACK_STRUCT_EXECUTE_T *pStruct);     

#ifdef __cplusplus
}
#endif

#endif
