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

    Header Name: dmtAsyncAPI.cc
    
    General Description: This file contains implementation of the DMT async APIs

==============================================================================*/

#include "dmtAsyncMessage.h"
#include "xpl_Message.h"
#include "xpl_Time.h"
#include "dmAsyncMessageID.h"
#include "dm_tree_util.h" 
#include "dmt.hpp"
#include "dmMemory.h"
#include "xpl_Logger.h"

static DMVector<PDmtTree> treeHandlerVector;
static DMVector<PDmtNode> nodeHandlerVector;

void dmtBuildCallbackData(DMT_DATA_T * pCallbackData, const DmtData & oDmtData)
{
    SYNCML_DM_DATAFORMAT_T format;
    DMString & strData =  (DMString&)oDmtData.GetStringValue();
    DMVector<UINT8> & binData = (DMVector<UINT8>&)oDmtData.GetBinaryValue();
  
    pCallbackData->meta_format = SYNCML_DM_DATAFORMAT_UNDEFINED;
    format = oDmtData.GetType();
    switch ( format )
    {
        case SYNCML_DM_DATAFORMAT_STRING:
        case SYNCML_DM_DATAFORMAT_FLOAT:
        case SYNCML_DM_DATAFORMAT_DATE:
        case SYNCML_DM_DATAFORMAT_TIME:
            pCallbackData->meta_format = format;
            pCallbackData->data.str_value = strData.detach();
            break;

        case SYNCML_DM_DATAFORMAT_INT:
            pCallbackData->meta_format = format;
            oDmtData.GetInt(pCallbackData->data.int_value);
            break;
                
        case SYNCML_DM_DATAFORMAT_BOOL:
            pCallbackData->meta_format = format;
            oDmtData.GetBoolean((BOOLEAN&)pCallbackData->data.int_value);
            break;
                
        case SYNCML_DM_DATAFORMAT_BIN:
            pCallbackData->meta_format = format;
            pCallbackData->data.bin.bin_value = binData.get_data();
            pCallbackData->data.bin.len_bin_data = binData.size();
            binData.detach();
            break;
   }
   
}


void dmtBuildStatusStruct(DMT_CALLBACK_STRUCT_STATUS_T *pStruct, 
                              UINT32 pUserData, 
                              DMT_OPERATION_TYPE_T nCallbackType, 
                              SYNCML_DM_RET_STATUS_T nStatusCode)
{
    pStruct->pUserData = (void*)pUserData;
    pStruct->nCallbackType = nCallbackType;
    pStruct->nStatusCode = nStatusCode;
}


void dmtSendStatusStruct(DMT_CALLBACK_STRUCT_STATUS_T *pStruct,
                    DMT_CallbackStatusCode callback,
                    UINT32 messageID)
{
    if ( callback )
        callback(pStruct);
    else
        if ( messageID )
            XPL_MSG_Send(XPL_PORT_DM,messageID,pStruct,sizeof(DMT_CALLBACK_STRUCT_STATUS_T));
}



INT32 dmtFindTreeHandler(DMT_H_TREE htree)
{
    INT32 size = treeHandlerVector.size();
    for (INT32 index=0; index<size; index++)
    {
        PDmtTree & tree = treeHandlerVector[index];
        if ( tree == (DmtTree*)htree )
            return index;
    }
    return -1;
}


INT32 dmtFindNodeHandler(DMT_H_NODE hnode)
{
    INT32 size = nodeHandlerVector.size();
    for (INT32 index=0; index<size; index++)
    {
        PDmtNode & node = nodeHandlerVector[index];
        if ( node == (DmtNode*)hnode )
            return index;
    }
    return -1;
}
                    
DmtTree * dmtGetTreeHandler(DMT_H_TREE htree)
{
    if ( dmtFindTreeHandler(htree) == -1 )
        return NULL;
    else
        return (DmtTree*)htree;
}


DmtNode * dmtGetNodeHandler(DMT_H_NODE hnode)
{
    if ( dmtFindNodeHandler(hnode) == -1 )
        return NULL;
    else
        return (DmtNode*)hnode;
}


void dmtNotifyHandler(void *data)
{
    SYNCML_DM_NOTIFY_MESSAGE_T *pMessage = (SYNCML_DM_NOTIFY_MESSAGE_T *)data;
  
     if ( pMessage->callback )
        pMessage->callback();
    else
        if ( pMessage->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->messageID,NULL,0);
}


#ifdef DM_NO_LOCKING            
void dmtHandleTimer(void * data)
{
    SYNCML_DM_TIMER_MSG_T *pMessage = (SYNCML_DM_TIMER_MSG_T *)data;

    //XPL_CLK_StopTimer(pMessage->timerHandle);
    pMessage->callback();
}
#endif

 
void dmtEngineHandler(SYNCML_DM_TASK_MESSAGE_ID operation, void *data)
{
    SYNCML_DM_ENGINE_MESSAGE_T *pMessage = (SYNCML_DM_ENGINE_MESSAGE_T *)data;
    DMT_CALLBACK_STRUCT_STATUS_T statusStruct;
    SYNCML_DM_RET_STATUS_T res;

    treeHandlerVector.clear();
    nodeHandlerVector.clear();
    
    if ( operation == SYNCML_DM_INIT_MSG_ID )
        res = DmtTreeFactory::Initialize();
    else
        res = DmtTreeFactory::Uninitialize();

    dmtBuildStatusStruct(&statusStruct,pMessage->pUserData,operation,res);
    dmtSendStatusStruct(&statusStruct,pMessage->callback,pMessage->messageID);
}



void dmtGetSubtreeExHandler(void * data)
{

    SYNCML_DM_GET_SUB_TREE_MESSAGE_T *pMessage = (SYNCML_DM_GET_SUB_TREE_MESSAGE_T *)data;
    PDmtTree ptrTree;
 
    DMT_CALLBACK_STRUCT_GETTREE_T treeStruct;

    treeStruct.pUserData = (void*)pMessage->pMsg->pUserData;
    treeStruct.htree = 0;

    treeStruct.nStatusCode = DmtTreeFactory::GetSubtreeEx(pMessage->pMsg->principal,     
                                                          pMessage->pMsg->subtreeRoot,
                                                          pMessage->pMsg->nLockType,  
                                                          ptrTree);

    if ( treeStruct.nStatusCode == SYNCML_DM_SUCCESS )
    {
        treeHandlerVector.push_back(ptrTree);
        treeStruct.htree = (DMT_H_TREE)ptrTree.GetPtr();
    }    
    if ( pMessage->pMsg->callback )
        pMessage->pMsg->callback(&treeStruct);
    else
        if ( pMessage->pMsg->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->pMsg->messageID,&treeStruct,sizeof(DMT_CALLBACK_STRUCT_GETTREE_T));
    delete pMessage->pMsg;    
 
}
                  

void dmtTreeReleaseHandler(void * data)
{
   
    SYNCML_DM_TREE_MESSAGE_T *pMessage = (SYNCML_DM_TREE_MESSAGE_T*)data;
    INT32 pos;
    DMT_CALLBACK_STRUCT_STATUS_T statusStruct;

    pos = dmtFindTreeHandler(pMessage->htree);

    if ( pos != -1 ) 
    {
        treeHandlerVector.remove(pos);   
        dmtBuildStatusStruct(&statusStruct,pMessage->pUserData,DMT_OPERATION_RELEASE_TREE,SYNCML_DM_SUCCESS);
    }    
    else
        dmtBuildStatusStruct(&statusStruct,pMessage->pUserData,DMT_OPERATION_RELEASE_TREE,SYNCML_DM_FAIL);

    dmtSendStatusStruct(&statusStruct,pMessage->callback,pMessage->messageID);
}
                 


void dmtProcessScriptHandler(void * data)
{

    SYNCML_DM_PROCESS_SCRIPT_MESSAGE_T *pMessage = (SYNCML_DM_PROCESS_SCRIPT_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_PROCESS_SCRIPT_T scriptStruct;
    DMString oResult;

    scriptStruct.nStatusCode = DmtTreeFactory::ProcessScript(pMessage->pMsg->principal, 
                                                            pMessage->pMsg->buf.getBuffer(), 
                                                            pMessage->pMsg->buf.getSize(), 
                                                            pMessage->pMsg->isWBXML, 
                                                            oResult);

    scriptStruct.pUserData = (void*)pMessage->pMsg->pUserData;
    scriptStruct.result_len = oResult.length();
    scriptStruct.result = oResult.detach();
    
    if ( pMessage->pMsg->callback )
    {
        pMessage->pMsg->callback(&scriptStruct);
        DMT_Free_ProcessScriptStruct(&scriptStruct);
    }    
    else
        if ( pMessage->pMsg->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->pMsg->messageID,&scriptStruct,sizeof(DMT_CALLBACK_STRUCT_PROCESS_SCRIPT_T));
    delete pMessage->pMsg; 

}



void dmtBootstrapHandler(void * data)
{

    SYNCML_DM_BOOTSTRAP_MESSAGE_T *pMessage = (SYNCML_DM_BOOTSTRAP_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_BOOTSTRAP_T scriptStruct;
    DMString serverID;
  
    scriptStruct.nStatusCode = DmtTreeFactory::Bootstrap(pMessage->pMsg->principal, 
                                        pMessage->pMsg->buf.getBuffer(), 
                                        pMessage->pMsg->buf.getSize(), 
                                        pMessage->pMsg->isWBXML, 
                                        pMessage->pMsg->isProcess,
                                        serverID);

    scriptStruct.pUserData = (void*)pMessage->pMsg->pUserData;
    scriptStruct.serverID = serverID.detach();
    if ( pMessage->pMsg->callback )
    {
        pMessage->pMsg->callback(&scriptStruct);
        DMT_Free_BootstrapStruct(&scriptStruct);
    }    
    else
        if ( pMessage->pMsg->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->pMsg->messageID,&scriptStruct,sizeof(DMT_CALLBACK_STRUCT_BOOTSTRAP_T));
    delete pMessage->pMsg;    

}


#ifdef DM_NOTIFICATION_AGENT
void dmtProcessNotificationHandler(void * data)
{

    SYNCML_DM_PROCESS_NOTIFICATION_MESSAGE_T *pMessage = (SYNCML_DM_PROCESS_NOTIFICATION_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_PROCESS_NOTIFICATION_T notifStruct;
    DmtNotification notification;
  
    notifStruct.nStatusCode = DmtTreeFactory::ProcessNotification(pMessage->pMsg->principal, 
                                        pMessage->pMsg->buf.getBuffer(), 
                                        pMessage->pMsg->buf.getSize(),  
                                        notification);

    notifStruct.pUserData = (void*)pMessage->pMsg->pUserData;
    
    notifStruct.notification.uiMode = notification.getUIMode();
    notifStruct.notification.initiator = notification.getInitiator();
    notifStruct.notification.sessionID = notification.getSessionID();
    notifStruct.notification.authFlag = notification.getAuthFlag();
    DMString & serverID = (DMString&)notification.getServerID();
    
    notifStruct.notification.serverID = serverID.detach();
    
    if ( pMessage->pMsg->callback )
    {
        pMessage->pMsg->callback(&notifStruct);
        DMT_Free_ProcessNotificationStruct(&notifStruct);
    }    
    else
        if ( pMessage->pMsg->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->pMsg->messageID,&notifStruct,sizeof(DMT_CALLBACK_STRUCT_PROCESS_NOTIFICATION_T));
    delete pMessage->pMsg;    

}      
#endif

void dmtStartServerSessionHandler(void * data)
{
    SYNCML_DM_START_SERVER_SESSION_MESSAGE_T *pMessage = (SYNCML_DM_START_SERVER_SESSION_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_STATUS_T statusStruct;
    SYNCML_DM_RET_STATUS_T res;
   
    res = DmtTreeFactory::StartServerSession(pMessage->pMsg->principal,pMessage->pMsg->sessionProp); 
    dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,DMT_OPERATION_START_SRV_SESSION,res);
    dmtSendStatusStruct(&statusStruct,pMessage->pMsg->callback,pMessage->pMsg->messageID);
    delete pMessage->pMsg;
}

void dmtGetNodeHandler(void * data)
{

    SYNCML_DM_GET_NODE_MESSAGE_T *pMessage = (SYNCML_DM_GET_NODE_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_GETNODE_T nodeStruct;
    DmtTree * pTree = dmtGetTreeHandler(pMessage->pMsg->htree);
    PDmtNode ptrNode;

    nodeStruct.hnode = 0;
    nodeStruct.pUserData = (void*)pMessage->pMsg->pUserData;
    if ( pTree == NULL )
        nodeStruct.nStatusCode = SYNCML_DM_INVALID_PARAMETER;
    else
    {

        nodeStruct.nStatusCode = pTree->GetNode(pMessage->pMsg->path, ptrNode);
        if ( nodeStruct.nStatusCode == SYNCML_DM_SUCCESS )
        {
            nodeHandlerVector.push_back(ptrNode);
            nodeStruct.hnode = (DMT_H_NODE)ptrNode.GetPtr();
            nodeStruct.leaf_node = ptrNode->IsLeaf();
            nodeStruct.external_storage_node = ptrNode->IsExternalStorageNode();
        }
    }    
    if ( pMessage->pMsg->callback )
        pMessage->pMsg->callback(&nodeStruct);
    else
        if ( pMessage->pMsg->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->pMsg->messageID,&nodeStruct,sizeof(DMT_CALLBACK_STRUCT_GETNODE_T));
    delete pMessage->pMsg;    
}

void dmtNodeReleaseHandler(void * data)
{
    SYNCML_DM_NODE_MESSAGE_T *pMessage = (SYNCML_DM_NODE_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_STATUS_T statusStruct;
    INT32 pos;

    pos = dmtFindNodeHandler(pMessage->pMsg->hnode);
    if ( pos != -1 ) 
    {
        nodeHandlerVector.remove(pos);   
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,DMT_OPERATION_RELEASE_NODE,SYNCML_DM_SUCCESS);
    }    
    else
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,DMT_OPERATION_RELEASE_NODE,SYNCML_DM_FAIL);

    dmtSendStatusStruct(&statusStruct,pMessage->pMsg->callback,pMessage->pMsg->messageID);
    delete pMessage->pMsg;
 
}
                 

void dmtTreeNodeHandler(SYNCML_DM_TASK_MESSAGE_ID operation, void * data)
{
    SYNCML_DM_TREENODE_MESSAGE_T *pMessage = (SYNCML_DM_TREENODE_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_STATUS_T statusStruct;
    DmtTree * pTree = dmtGetTreeHandler(pMessage->pMsg->htree);

    if ( pTree == NULL )
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,operation,SYNCML_DM_INVALID_PARAMETER);
    else
    {
        SYNCML_DM_RET_STATUS_T res;
        switch ( operation )
        {
            case SYNCML_DM_DELETE_NODE_MSG_ID:
                res = pTree->DeleteNode(pMessage->pMsg->path);
                break;
            case SYNCML_DM_RENAME_NODE_MSG_ID:   
                res = pTree->RenameNode(pMessage->pMsg->path,pMessage->pMsg->str);
                break;
            case SYNCML_DM_CREATE_INTERIOR_NODE_MSG_ID:
                {
                    PDmtNode ptrNode;
                    res = pTree->CreateInteriorNode(pMessage->pMsg->path, ptrNode);
                }
                break;
        }        
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,operation,res);
    }    
    dmtSendStatusStruct(&statusStruct,pMessage->pMsg->callback,pMessage->pMsg->messageID);
    delete pMessage->pMsg;
}
                
                        

void dmtCreateLeafNodeHandler(void * data)
{

    SYNCML_DM_CREATE_LEAF_NODE_MESSAGE_T *pMessage = (SYNCML_DM_CREATE_LEAF_NODE_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_STATUS_T statusStruct;
   
    DmtTree * pTree = dmtGetTreeHandler(pMessage->pMsg->htree);

    if ( pTree == NULL )
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,DMT_OPERATION_CREATEL_NODE,SYNCML_DM_INVALID_PARAMETER);
    else
    {

        SYNCML_DM_RET_STATUS_T res;
        PDmtNode ptrNode;
        
        res = pTree->CreateLeafNode(pMessage->pMsg->path, ptrNode,pMessage->pMsg->data);
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,DMT_OPERATION_CREATEL_NODE,res);
    }    
    dmtSendStatusStruct(&statusStruct,pMessage->pMsg->callback,pMessage->pMsg->messageID);
    delete pMessage->pMsg;

}
                    

void dmtGetChildNodeNamesHandler(void * data)
{

    SYNCML_DM_GET_CHILD_NODE_NAMES_MESSAGE_T *pMessage = (SYNCML_DM_GET_CHILD_NODE_NAMES_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_GET_CHILDNODE_NAMES_T nodeStruct;
    DMStringVector mapNodes;

    memset(&nodeStruct,0,sizeof(DMT_CALLBACK_STRUCT_GET_CHILDNODE_NAMES_T));
    nodeStruct.pUserData = (void*)pMessage->pMsg->pUserData; 

   
    DmtTree * pTree = dmtGetTreeHandler(pMessage->pMsg->htree);

    if ( pTree == NULL )
        nodeStruct.nStatusCode = SYNCML_DM_INVALID_PARAMETER;
    else
    {
       
        nodeStruct.nStatusCode = pTree->GetChildNodeNames(pMessage->pMsg->path, mapNodes); 
        if ( nodeStruct.nStatusCode == SYNCML_DM_SUCCESS )
        {
            INT32 size = mapNodes.size();
            nodeStruct.ppChildren = (CPCHAR*)DmAllocMem(size*sizeof(CPCHAR));
            if ( nodeStruct.ppChildren == NULL )
                nodeStruct.nStatusCode = SYNCML_DM_DEVICE_FULL;
            else
            {
                nodeStruct.num_children = size;
                for (int index=0; index<size; index++)
                {
                    DMString & str = mapNodes[index];
                    nodeStruct.ppChildren[index] = str.detach();
                }    
            }    
        }
    }    
    if ( pMessage->pMsg->callback )
    {
        pMessage->pMsg->callback(&nodeStruct);
        DMT_Free_GetChildNodeNamesStruct(&nodeStruct);
    }    
    else
        if ( pMessage->pMsg->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->pMsg->messageID,&nodeStruct,sizeof(DMT_CALLBACK_STRUCT_GET_CHILDNODE_NAMES_T));
    delete pMessage->pMsg;    


}
                       

void dmtTreeHandler(SYNCML_DM_TASK_MESSAGE_ID operation, void * data)
{
    SYNCML_DM_TREE_MESSAGE_T *pMessage = (SYNCML_DM_TREE_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_STATUS_T statusStruct;
   
    DmtTree * pTree = dmtGetTreeHandler(pMessage->htree);

    if ( pTree == NULL )
        dmtBuildStatusStruct(&statusStruct,pMessage->pUserData,operation,SYNCML_DM_INVALID_PARAMETER);
    else
    {
        SYNCML_DM_RET_STATUS_T res;
        switch ( operation )
        {
            case SYNCML_DM_FLUSH_MSG_ID: 
                res = pTree->Flush();
                break;
                
            case SYNCML_DM_COMMIT_MSG_ID:             
                res = pTree->Commit();
                break;
                
            case SYNCML_DM_ROLLBACK_MSG_ID:           
                res = pTree->Rollback();
                break;
                
            case SYNCML_DM_BEGIN_MSG_ID:
                res = pTree->Begin();
                break;
        }        
        dmtBuildStatusStruct(&statusStruct,pMessage->pUserData,operation,res);
    }    
    dmtSendStatusStruct(&statusStruct,pMessage->callback,pMessage->messageID);

}
           


void dmtGetChildValuesMapHandler(void * data)
{

    SYNCML_DM_GET_CHILD_VALUES_MAP_MESSAGE_T *pMessage = (SYNCML_DM_GET_CHILD_VALUES_MAP_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_GET_CHILDVALUES_MAP_T nodeStruct;
    DMMap<DMString, DmtData> mapNodes;
       

    memset(&nodeStruct,0,sizeof(DMT_CALLBACK_STRUCT_GET_CHILDVALUES_MAP_T));
    nodeStruct.pUserData = (void*)pMessage->pMsg->pUserData;
   
    DmtTree * pTree = dmtGetTreeHandler(pMessage->pMsg->htree);

    if ( pTree == NULL )
        nodeStruct.nStatusCode = SYNCML_DM_INVALID_PARAMETER;
    else
    {
        nodeStruct.nStatusCode = pTree->GetChildValuesMap(pMessage->pMsg->path, mapNodes);
        if ( nodeStruct.nStatusCode == SYNCML_DM_SUCCESS )
        {
            BOOLEAN isMemFailed = FALSE;
            INT32 size = mapNodes.size();
            
            nodeStruct.data.ppChildren = (CPCHAR*)DmAllocMem(size*sizeof(CPCHAR));
            if ( nodeStruct.data.ppChildren == NULL )
                isMemFailed = TRUE;
            else
            {
                nodeStruct.data.pData = (DMT_DATA_T*)DmAllocMem(size*sizeof(DMT_DATA_T));
                if ( nodeStruct.data.pData == NULL )
                    isMemFailed = TRUE;
            }

            if ( isMemFailed == FALSE )
            {
                nodeStruct.data.num_children = mapNodes.size();
                for (int POS=mapNodes.begin(); POS<mapNodes.end(); POS++)
                {
                    nodeStruct.data.ppChildren[POS] = (CPCHAR)mapNodes.get_key(POS);
                    const DmtData & oData = mapNodes.get_value(POS);
                    dmtBuildCallbackData((DMT_DATA_T*)&nodeStruct.data.pData[POS], oData);
                }

            }       
            else
            {
                nodeStruct.nStatusCode = SYNCML_DM_DEVICE_FULL;
                XPL_LOG_DM_API_Error(("dmtGetChildValuesMapHandler : unable allocate memory\n"));
            }    
        }    
  
   }    

   if ( pMessage->pMsg->callback )
   {
        pMessage->pMsg->callback(&nodeStruct);
        DMT_Free_GetChildValuesStruct(&nodeStruct);
   }     
    else
        if ( pMessage->pMsg->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->pMsg->messageID,&nodeStruct,sizeof(DMT_CALLBACK_STRUCT_GET_CHILDVALUES_MAP_T));
   delete pMessage->pMsg;     
  
}
                       

void dmtSetChildValuesMapHandler(void * data)
{

    SYNCML_DM_SET_CHILD_VALUES_MAP_MESSAGE_T *pMessage = (SYNCML_DM_SET_CHILD_VALUES_MAP_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_STATUS_T statusStruct;
    DmtTree * pTree = dmtGetTreeHandler(pMessage->pMsg->htree);

    if ( pTree == NULL )
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,DMT_OPERATION_SET_CHILDVALUES_MAP,SYNCML_DM_INVALID_PARAMETER);
    else
    {

        SYNCML_DM_RET_STATUS_T res;

        res = pTree->SetChildValuesMap(pMessage->pMsg->path, pMessage->pMsg->data);
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,DMT_OPERATION_SET_CHILDVALUES_MAP,res);
    }    
    dmtSendStatusStruct(&statusStruct,pMessage->pMsg->callback,pMessage->pMsg->messageID);
    delete pMessage->pMsg;
}




void dmtGetAttributesHandler(void * data)
{

    SYNCML_DM_GET_ATTRIBUTES_MESSAGE_T *pMessage = (SYNCML_DM_GET_ATTRIBUTES_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_GET_ATTRIBUTES_T attrStruct;
    DmtAttributes oAttr; 
  
    memset(&attrStruct,0,sizeof(DMT_CALLBACK_STRUCT_GET_ATTRIBUTES_T));    
    attrStruct.pUserData = (void*)pMessage->pUserData;

    DmtNode * pNode = dmtGetNodeHandler(pMessage->hnode);

    if ( pNode == NULL )
        attrStruct.nStatusCode = SYNCML_DM_INVALID_PARAMETER;
    else
    {
        attrStruct.nStatusCode = pNode->GetAttributes(oAttr);
        if ( attrStruct.nStatusCode == SYNCML_DM_SUCCESS )
        {
            DMString aclStr;
            DMString & name = (DMString&)oAttr.GetName();
            DMString & format = (DMString&)oAttr.GetFormat();
            DMString & title = (DMString&)oAttr.GetTitle();
            DMString & type = (DMString&)oAttr.GetType();
       
            attrStruct.attributes.name = name.detach();
            attrStruct.attributes.format = format.detach();
            attrStruct.attributes.title = title.detach();
            attrStruct.attributes.type = type.detach();
            const DmtAcl & oAcl = oAttr.GetAcl();
            aclStr = oAcl.toString();
            attrStruct.attributes.acl = aclStr.detach();
            attrStruct.attributes.version = oAttr.GetVersion();
            attrStruct.attributes.size = oAttr.GetSize();
            attrStruct.attributes.timestamp = oAttr.GetTimestamp();
       }
    }   

    if ( pMessage->callback )
    {
        pMessage->callback(&attrStruct);
        DMT_Free_GetAttributesStruct(&attrStruct);
    }        
    else
        if ( pMessage->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->messageID,&attrStruct,sizeof(DMT_CALLBACK_STRUCT_GET_ATTRIBUTES_T));
  
}
                   
                   

void dmtGetValueHandler(void * data)
{

    SYNCML_DM_GET_VALUE_MESSAGE_T *pMessage = (SYNCML_DM_GET_VALUE_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_GET_VALUE_T valueStruct;
    static DmtData oData; 
 
    memset(&valueStruct,0,sizeof(DMT_CALLBACK_STRUCT_GET_VALUE_T));
    valueStruct.pUserData = (void*)pMessage->pUserData;
   
    DmtNode * pNode = dmtGetNodeHandler(pMessage->hnode);

    if ( pNode == NULL )
        valueStruct.nStatusCode = SYNCML_DM_INVALID_PARAMETER;
    else
    {

        valueStruct.nStatusCode = pNode->GetValue(oData);
        if ( valueStruct.nStatusCode == SYNCML_DM_SUCCESS )
            dmtBuildCallbackData(&valueStruct.data, oData);
    }    

    if ( pMessage->callback )
    {
        pMessage->callback(&valueStruct);
        DMT_Free_GetValueStruct(&valueStruct);
    }    
    else
        if ( pMessage->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->messageID,&valueStruct,sizeof(DMT_CALLBACK_STRUCT_GET_VALUE_T));
   

}

void dmtSetValueHandler(void * data)
{
    SYNCML_DM_SET_VALUE_MESSAGE_T *pMessage = (SYNCML_DM_SET_VALUE_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_STATUS_T statusStruct;
    
    DmtNode * pNode = dmtGetNodeHandler(pMessage->pMsg->hnode);

    if ( pNode == NULL )
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,DMT_OPERATION_SET_VALUE,SYNCML_DM_INVALID_PARAMETER);
    else
    {
        SYNCML_DM_RET_STATUS_T res;
     
        res = pNode->SetValue(pMessage->pMsg->data);
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,DMT_OPERATION_SET_VALUE,res);
    }    
    dmtSendStatusStruct(&statusStruct,pMessage->pMsg->callback,pMessage->pMsg->messageID);
    delete pMessage->pMsg;
    
}
              

void dmtNodeHandler(SYNCML_DM_TASK_MESSAGE_ID operation, void * data)
{
    SYNCML_DM_NODE_MESSAGE_T *pMessage = (SYNCML_DM_NODE_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_STATUS_T statusStruct;
    
    DmtNode * pNode = dmtGetNodeHandler(pMessage->pMsg->hnode);

    if ( pNode == NULL )
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,operation,SYNCML_DM_INVALID_PARAMETER);
    else
    {
        SYNCML_DM_RET_STATUS_T res;
        switch ( operation )
        {
            case SYNCML_DM_SET_TITLE_MSG_ID:
                res = pNode->SetTitle(pMessage->pMsg->str);
                break;

            case SYNCML_DM_SET_ACL_MSG_ID:
                {
                    DmtAcl oAcl(pMessage->pMsg->str);
                    res = pNode->SetAcl(oAcl);
                }    
                break;
        }
        dmtBuildStatusStruct(&statusStruct,pMessage->pMsg->pUserData,operation,res);
    }    
    dmtSendStatusStruct(&statusStruct,pMessage->pMsg->callback,pMessage->pMsg->messageID);
    delete pMessage->pMsg;
}
              

void dmtExecuteHandler(void * data)
{
    SYNCML_DM_EXECUTE_MESSAGE_T *pMessage = (SYNCML_DM_EXECUTE_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_EXECUTE_T execStruct;
    DMString result;
 
    execStruct.result = NULL;
    execStruct.pUserData = (void*)pMessage->pMsg->pUserData;
    
    DmtNode * pNode = dmtGetNodeHandler(pMessage->pMsg->hnode);

    if ( pNode == NULL )
        execStruct.nStatusCode = SYNCML_DM_INVALID_PARAMETER;
    else
    {
        execStruct.nStatusCode = pNode->Execute(pMessage->pMsg->params,result);
        if ( execStruct.nStatusCode == SYNCML_DM_SUCCESS )
            execStruct.result = result.detach();
    }   

    if ( pMessage->pMsg->callback )
    {
        pMessage->pMsg->callback(&execStruct);
        DMT_Free_ExecuteStruct(&execStruct);
    }    
    else
        if ( pMessage->pMsg->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->pMsg->messageID,&execStruct,sizeof(DMT_CALLBACK_STRUCT_EXECUTE_T));
    delete pMessage->pMsg;    
}
             
#ifdef LOB_SUPPORT
void dmtDataChunkHandler(void * data, UINT32 msgtype)
{
    SYNCML_DM_DATA_CHUNK_MESSAGE_T *pMessage = ( SYNCML_DM_DATA_CHUNK_MESSAGE_T*)data;
    DMT_CALLBACK_STRUCT_DATA_CHUNK_T chunkStruct;

    memset(&chunkStruct,0,sizeof(DMT_CALLBACK_STRUCT_DATA_CHUNK_T));
    chunkStruct.pUserData = (void*)pMessage->pUserData;
    chunkStruct.datachunk = pMessage->datachunk;
    DmtNode * pNode = dmtGetNodeHandler(pMessage->hnode);

    if ( pNode == NULL )
        chunkStruct.nStatusCode = SYNCML_DM_INVALID_PARAMETER;
    else
    {
    if(pMessage->datachunk != NULL  && pMessage->datachunk->chunkBuffer != NULL )
    {
        DmtDataChunk chunkData;
        chunkData.attach(pMessage->datachunk->chunkBuffer, pMessage->datachunk->dataSize);
        switch ( msgtype )
        {
            case SYNCML_DM_GET_FIRST_CHUNK_MSG_ID:
                    chunkStruct.nStatusCode = pNode->GetFirstChunk(chunkData);
                    break;
            case  SYNCML_DM_GET_NEXT_CHUNK_MSG_ID:
                    chunkStruct.nStatusCode = pNode->GetNextChunk(chunkData);
                    break;
            case  SYNCML_DM_SET_FIRST_CHUNK_MSG_ID:
                    chunkStruct.nStatusCode = pNode->SetFirstChunk(chunkData);
                    break;
                    
            case  SYNCML_DM_SET_NEXT_CHUNK_MSG_ID:
                    chunkStruct.nStatusCode = pNode->SetNextChunk(chunkData);
                    break;
            case  SYNCML_DM_SET_LAST_CHUNK_MSG_ID:
                    chunkStruct.nStatusCode = pNode->SetLastChunk(chunkData);
                    break;
        }
            if ( chunkStruct.nStatusCode == SYNCML_DM_SUCCESS )
          {
            UINT32 dataLen;
            // Get return len
             chunkStruct.nStatusCode = chunkData.GetReturnLen(dataLen); 
            if( chunkStruct.nStatusCode == SYNCML_DM_SUCCESS)
                    chunkStruct.datachunk->returnLen = dataLen;
            }
        chunkData.detach();
    }
    else
            chunkStruct.nStatusCode = SYNCML_DM_INVALID_PARAMETER;
    }    
    if ( pMessage->callback )
    {
        pMessage->callback(&chunkStruct);
    }        
    else
        if ( pMessage->messageID )
            XPL_MSG_Send(XPL_PORT_DM,pMessage->messageID,&chunkStruct,sizeof(DMT_CALLBACK_STRUCT_DATA_CHUNK_T));
}
#endif

void DMT_MessageHandler(UINT32 msgtype, void *data) 
{

    if ( data == NULL )
        return;
  
    switch ( msgtype )
    {  
        case SYNCML_DM_TIMER_MSG_ID:
#ifdef DM_NO_LOCKING            
            dmtHandleTimer(data);
#endif
            break;

    
        case SYNCML_DM_NOTIFY_ON_IDLE_MSG_ID:
            dmtNotifyHandler(data);
            break;
    
        case SYNCML_DM_INIT_MSG_ID:
        case SYNCML_DM_UNINIT_MSG_ID:
            dmtEngineHandler(msgtype,data);
            break;   
       
        case SYNCML_DM_GET_SUBTREE_MSG_ID: 
            dmtGetSubtreeExHandler(data);
            break; 

        case SYNCML_DM_RELEASE_TREE_ID:
            dmtTreeReleaseHandler(data);
            break;  

        case SYNCML_DM_START_SERVER_SESSION_MSG_ID:
            dmtStartServerSessionHandler(data); 
            break;

        case SYNCML_DM_PROCESS_SCRIPT_MSG_ID:
            dmtProcessScriptHandler(data);
            break;
       
        case SYNCML_DM_BOOTSTRAP_MSG_ID:
            dmtBootstrapHandler(data);
            break;

#ifdef DM_NOTIFICATION_AGENT
        case SYNCML_DM_PROCESS_NOTIFICATION_MSG_ID:   
            dmtProcessNotificationHandler(data);
            break;
#endif            
       
        case SYNCML_DM_GET_NODE_MSG_ID:
            dmtGetNodeHandler(data);
            break;
       
        case SYNCML_DM_RELEASE_NODE_MSG_ID:
            dmtNodeReleaseHandler(data);
            break;
       
        case SYNCML_DM_DELETE_NODE_MSG_ID:
        case SYNCML_DM_RENAME_NODE_MSG_ID:
        case SYNCML_DM_CREATE_INTERIOR_NODE_MSG_ID:
            dmtTreeNodeHandler(msgtype,data);
            break;
       
        case SYNCML_DM_CREATE_LEAF_NODE_MSG_ID:
            dmtCreateLeafNodeHandler(data);
            break;
       
        case SYNCML_DM_GET_CHULD_NODE_NAMES_MSG_ID:
            dmtGetChildNodeNamesHandler(data);
            break;
       
        case SYNCML_DM_FLUSH_MSG_ID:
        case SYNCML_DM_COMMIT_MSG_ID:    
        case SYNCML_DM_ROLLBACK_MSG_ID:
        case SYNCML_DM_BEGIN_MSG_ID:
            dmtTreeHandler(msgtype,data);
            break;
       
        case SYNCML_DM_GET_CHILD_VALUES_MAP_MSG_ID:
            dmtGetChildValuesMapHandler(data);
            break;
       
        case SYNCML_DM_SET_CHILD_VALUES_MAP_MSG_ID:
            dmtSetChildValuesMapHandler(data);
            break;
       
        case SYNCML_DM_GET_ATTRIBUTES_MSG_ID:
            dmtGetAttributesHandler(data);
            break;
       
        case SYNCML_DM_GET_VALUE_MSG_ID:
            dmtGetValueHandler(data);
            break;
       
        case SYNCML_DM_SET_VALUE_MSG_ID:
            dmtSetValueHandler(data);
            break;
       
        case SYNCML_DM_SET_TITLE_MSG_ID:
        case SYNCML_DM_SET_ACL_MSG_ID:
            dmtNodeHandler(msgtype,data);
            break;
       
        case SYNCML_DM_EXECUTE_MSG_ID:
            dmtExecuteHandler(data); 
            break; 
#ifdef LOB_SUPPORT
     case SYNCML_DM_GET_FIRST_CHUNK_MSG_ID:
     case  SYNCML_DM_GET_NEXT_CHUNK_MSG_ID:
     case  SYNCML_DM_SET_FIRST_CHUNK_MSG_ID:
     case  SYNCML_DM_SET_NEXT_CHUNK_MSG_ID:
     case  SYNCML_DM_SET_LAST_CHUNK_MSG_ID:
         dmtDataChunkHandler(data, msgtype);
         break;
#endif
  }   

}    


#ifdef __cplusplus
extern "C" {
#endif



SYNCML_DM_RET_STATUS_T
dmtNotifyOnIdle_Post(DMT_CallbackNotifyOnIdle callback, 
                      void* pUserData, 
                      UINT32 messageID)
{

    SYNCML_DM_NOTIFY_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;

    message.callback = callback;
    message.messageID = messageID;
    message.pUserData = (UINT32)pUserData;
   
    res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_NOTIFY_ON_IDLE_MSG_ID,&message,sizeof(SYNCML_DM_NOTIFY_MESSAGE_T));
    if ( res == XPL_MSG_RET_SUCCESS )
        return SYNCML_DM_SUCCESS;
    else
        return SYNCML_DM_FAIL;

}



SYNCML_DM_RET_STATUS_T
dmtEngine_Post(DMT_CallbackStatusCode callback, 
               void* pUserData, 
               UINT32 messageID,
               UINT32 apiMessageID)
{

    SYNCML_DM_ENGINE_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;

    message.callback = callback;
    message.messageID = messageID;
    message.pUserData = (UINT32)pUserData;
   
    res = XPL_MSG_Send(XPL_PORT_DM_TASK,apiMessageID,&message,sizeof(SYNCML_DM_ENGINE_MESSAGE_T));
    if ( res == XPL_MSG_RET_SUCCESS )
        return SYNCML_DM_SUCCESS;
    else
        return SYNCML_DM_FAIL;

}



SYNCML_DM_RET_STATUS_T
dmtGetSubtreeEx_Post(CPCHAR szPrincipal, 
                  CPCHAR szSubtreeRoot,
                  SYNCML_DM_TREE_LOCK_TYPE_T nLockType, 
                  DMT_CallbackGetTree callback,
                  void* pUserData, 
                  UINT32 messageID )
{

    if ( szPrincipal == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    SYNCML_DM_GET_SUB_TREE_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;
    
    message.pMsg = (DMGetSubTreeMessage*) new DMGetSubTreeMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtGetSubtreeEx_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(szPrincipal, szSubtreeRoot, nLockType, callback,messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS ) 
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_GET_SUBTREE_MSG_ID,&message,sizeof( SYNCML_DM_GET_SUB_TREE_MESSAGE_T));
    
    if ( res != XPL_MSG_RET_SUCCESS || status != SYNCML_DM_SUCCESS )
    {
        delete message.pMsg;
        return SYNCML_DM_FAIL;
    }    
    return status;
}
                  

SYNCML_DM_RET_STATUS_T
dmtTree_Post(DMT_H_TREE htree,
                 DMT_CallbackStatusCode callback,
                 void* pUserData, 
                 UINT32 messageID,
                 UINT32 apiMessageID)
{
    if ( htree == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    SYNCML_DM_TREE_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;

    message.htree = htree;
    message.callback = callback;
    message.messageID = messageID;
    message.pUserData = (UINT32)pUserData;
   
    res = XPL_MSG_Send(XPL_PORT_DM_TASK,apiMessageID,&message,sizeof(SYNCML_DM_TREE_MESSAGE_T));
    if ( res == XPL_MSG_RET_SUCCESS )
        return SYNCML_DM_SUCCESS;
    else
        return SYNCML_DM_FAIL;

}
                 


SYNCML_DM_RET_STATUS_T
dmtProcessScript_Post(CPCHAR szPrincipal, 
                   const UINT8 * buf, 
                   INT32 len, 
                   BOOLEAN isWBXML,
                   DMT_CallbackProcessScript callback,
                   void* pUserData, 
                   UINT32 messageID )
{

    if ( szPrincipal == 0 || buf  == NULL || len == 0 )
        return SYNCML_DM_INVALID_PARAMETER;
   
    SYNCML_DM_PROCESS_SCRIPT_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMProcessScriptMessage*)new DMProcessScriptMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtProcessScript_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(szPrincipal, buf, len, isWBXML, callback,messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_PROCESS_SCRIPT_MSG_ID,&message,sizeof(SYNCML_DM_PROCESS_SCRIPT_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
}
                   


SYNCML_DM_RET_STATUS_T
dmtStartServerSession_Post(CPCHAR szPrincipal, 
                        const DMT_SESSION_PROP_T * pSessionProp,
                        DMT_CallbackStatusCode callback,
                        void* pUserData, 
                        UINT32 messageID )
{

    if ( szPrincipal == 0 || pSessionProp == 0 )
        return SYNCML_DM_INVALID_PARAMETER;
   
    SYNCML_DM_START_SERVER_SESSION_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMStartServerSessionMessage*)new DMStartServerSessionMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtStartServerSession_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(szPrincipal, pSessionProp, callback,messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_START_SERVER_SESSION_MSG_ID,&message,sizeof(SYNCML_DM_START_SERVER_SESSION_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
}     


SYNCML_DM_RET_STATUS_T 
dmtBootstrap_Post(CPCHAR szPrincipal,
                    const UINT8 * buf,
                    INT32 len,
                    BOOLEAN isWBXML,
                    BOOLEAN isProcess,
                    DMT_CallbackBootstrap callback,
                    void* pUserData, 
                    UINT32 messageID )
{
    
   if ( szPrincipal == 0 || buf  == NULL || len == 0 )
        return SYNCML_DM_INVALID_PARAMETER;
   
    SYNCML_DM_BOOTSTRAP_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMBootstrapMessage*)new DMBootstrapMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtExtractServerID_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(szPrincipal, buf, len, isWBXML, isProcess, callback,messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_BOOTSTRAP_MSG_ID,&message,sizeof(SYNCML_DM_BOOTSTRAP_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
}


SYNCML_DM_RET_STATUS_T 
dmtProcessNotification_Post(CPCHAR szPrincipal,
                        const UINT8 *buf, 
                        INT32 len,
                        DMT_CallbackProcessNotification callback,
                        void* pUserData, 
                        UINT32 messageID )
{

#ifdef DM_NOTIFICATION_AGENT
    if ( szPrincipal == 0 || buf == NULL || len == 0 )
        return SYNCML_DM_INVALID_PARAMETER;
   
    SYNCML_DM_PROCESS_NOTIFICATION_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMProcessNotificationMessage*)new DMProcessNotificationMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtProcessNotification_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(szPrincipal, buf, len, callback, messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    { 
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_PROCESS_NOTIFICATION_MSG_ID,&message,sizeof(SYNCML_DM_PROCESS_NOTIFICATION_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
#else
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
#endif

}


SYNCML_DM_RET_STATUS_T 
dmtGetNode_Post(DMT_H_TREE htree,
             CPCHAR  path,
             DMT_CallbackGetNode callback,
             void* pUserData, UINT32 messageID )
{

    if ( htree == 0 || path == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
   
   
    SYNCML_DM_GET_NODE_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMGetNodeMessage*)new DMGetNodeMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtGetNode_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(htree, path, callback, messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_GET_NODE_MSG_ID,&message,sizeof(SYNCML_DM_GET_NODE_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
}             


SYNCML_DM_RET_STATUS_T 
dmtTreeNode_Post(DMT_H_TREE htree,
                CPCHAR  path,
                CPCHAR  str,
                DMT_CallbackStatusCode callback,
                void* pUserData, 
                UINT32 messageID,
                UINT32 apiMessageID)
{
    if ( htree == 0 || path == NULL  )
        return SYNCML_DM_INVALID_PARAMETER;
   
    SYNCML_DM_TREENODE_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMTreeNodeMessage*)new DMTreeNodeMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtDeleteNode_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(htree, path,str,callback,messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,apiMessageID,&message,sizeof(SYNCML_DM_TREENODE_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
}
                

SYNCML_DM_RET_STATUS_T 
dmtRenameNode_Post(DMT_H_TREE htree,
                CPCHAR  path,
                CPCHAR  new_node_name,
                DMT_CallbackStatusCode callback,
                void* pUserData, 
                UINT32 messageID )
{
    if ( new_node_name == NULL )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtTreeNode_Post(htree, path, new_node_name, callback, pUserData, messageID,SYNCML_DM_RENAME_NODE_MSG_ID); 
}
                
                        

SYNCML_DM_RET_STATUS_T 
dmtCreateLeafNode_Post(DMT_H_TREE htree,
                    CPCHAR path,
                    const DMT_DATA_T* data,
                    DMT_CallbackStatusCode callback,
                    void* pUserData, 
                    UINT32 messageID )
{

    if ( htree == 0 || path == NULL )
        return SYNCML_DM_INVALID_PARAMETER;

    if ( data && data->meta_format == SYNCML_DM_DATAFORMAT_NODE )
        return SYNCML_DM_INVALID_PARAMETER;

    SYNCML_DM_CREATE_LEAF_NODE_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMCreateLeafNodeMessage*)new DMCreateLeafNodeMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtCreateLeafNode_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(htree, path, data, callback,messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_CREATE_LEAF_NODE_MSG_ID,&message,sizeof(SYNCML_DM_CREATE_LEAF_NODE_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
}
                    

SYNCML_DM_RET_STATUS_T 
dmtGetChildNodeNames_Post(DMT_H_TREE htree,
                       CPCHAR  path,
                       DMT_CallbackGetChildNodeNames callback,
                       void* pUserData, 
                       UINT32 messageID)
{

    if ( htree == 0 || path == NULL )
        return SYNCML_DM_INVALID_PARAMETER;


    SYNCML_DM_GET_CHILD_NODE_NAMES_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMGetChildNodeNamesMessage*)new DMGetChildNodeNamesMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtGetChildNodeNames_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(htree, path, callback,messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_GET_CHULD_NODE_NAMES_MSG_ID,&message,sizeof(SYNCML_DM_GET_CHILD_NODE_NAMES_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
}

                                  


SYNCML_DM_RET_STATUS_T
dmtGetChildValuesMap_Post(DMT_H_TREE htree,
                       CPCHAR  path,
                       DMT_CallbackGetChildValuesMap callback,
                       void* pUserData, 
                       UINT32 messageID)
{
    if ( htree == 0 || path == NULL )
        return SYNCML_DM_INVALID_PARAMETER;

    SYNCML_DM_GET_CHILD_VALUES_MAP_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMGetChildValuesMapMessage*)new DMGetChildValuesMapMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtGetChildValuesMap_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(htree, path, callback, messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_GET_CHILD_VALUES_MAP_MSG_ID,&message,sizeof(SYNCML_DM_GET_CHILD_VALUES_MAP_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
}


SYNCML_DM_RET_STATUS_T 
dmtSetChildValuesMap_Post(DMT_H_TREE htree,
                       CPCHAR path,
                       const DMT_LEAF_CHILDREN_DATA_T*  data,
                       DMT_CallbackStatusCode callback,
                       void* pUserData, UINT32 messageID)
{
    if ( htree == 0 || path == NULL || data == NULL )
        return SYNCML_DM_INVALID_PARAMETER;

    if ( data->ppChildren == NULL )
        return SYNCML_DM_INVALID_PARAMETER;

    SYNCML_DM_SET_CHILD_VALUES_MAP_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMSetChildValuesMapMessage*)new DMSetChildValuesMapMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtSetChildValuesMap_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(htree, path, data, callback, messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_SET_CHILD_VALUES_MAP_MSG_ID,&message,sizeof(SYNCML_DM_SET_CHILD_VALUES_MAP_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
}
                       


SYNCML_DM_RET_STATUS_T 
dmtGetAttributes_Post(DMT_H_NODE hnode,
                   DMT_CallbackGetAttributes callback,
                   void* pUserData, UINT32 messageID)
{
    if ( hnode == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    SYNCML_DM_GET_ATTRIBUTES_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;

    message.hnode = hnode;
    message.callback = callback;
    message.messageID = messageID;
    message.pUserData = (UINT32)pUserData;
   
    res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_GET_ATTRIBUTES_MSG_ID,&message,sizeof(SYNCML_DM_GET_ATTRIBUTES_MESSAGE_T));
    if ( res == XPL_MSG_RET_SUCCESS )
        return SYNCML_DM_SUCCESS;
    else
        return SYNCML_DM_FAIL;

}
                   
                   

SYNCML_DM_RET_STATUS_T 
dmtGetValue_Post(DMT_H_NODE hnode,
              DMT_CallbackGetValue callback,
              void* pUserData, UINT32 messageID)
{
    if ( hnode == 0 )
        return SYNCML_DM_INVALID_PARAMETER;


    SYNCML_DM_GET_VALUE_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;

    message.hnode = hnode;
    message.callback = callback;
    message.messageID = messageID;
    message.pUserData = (UINT32)pUserData;
   
    res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_GET_VALUE_MSG_ID,&message,sizeof(SYNCML_DM_GET_VALUE_MESSAGE_T));
    if ( res == XPL_MSG_RET_SUCCESS )
        return SYNCML_DM_SUCCESS;
    else
        return SYNCML_DM_FAIL;

}


SYNCML_DM_RET_STATUS_T 
dmtSetValue_Post(DMT_H_NODE hnode,
              const DMT_DATA_T* data,
              DMT_CallbackStatusCode callback,
              void* pUserData, UINT32 messageID)
{
    if ( hnode == 0 )
        return SYNCML_DM_INVALID_PARAMETER;


    SYNCML_DM_SET_VALUE_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMSetValueMessage*)new DMSetValueMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtSetValue_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(hnode, data, callback, messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_SET_VALUE_MSG_ID,&message,sizeof(SYNCML_DM_SET_VALUE_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;
}
              

SYNCML_DM_RET_STATUS_T
dmtNode_Post(DMT_H_NODE  hnode,
              CPCHAR str,
              DMT_CallbackStatusCode callback,
              void* pUserData, 
              UINT32 messageID,
              UINT32 apiMessageID)
{
    if ( hnode == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    SYNCML_DM_NODE_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMNodeMessage*)new DMNodeMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtNode_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(hnode, str, callback,messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,apiMessageID,&message,sizeof(SYNCML_DM_NODE_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;

}


SYNCML_DM_RET_STATUS_T
dmtNodeRelease_Post(DMT_H_NODE  hnode,
                          DMT_CallbackStatusCode callback,
                          void* pUserData, 
                          UINT32 messageID,
                          UINT32 apiMessageID)
{
    return dmtNode_Post(hnode,NULL,callback,pUserData,messageID,apiMessageID);
}              
            

SYNCML_DM_RET_STATUS_T 
dmtExecute_Post(DMT_H_NODE hnode,
             CPCHAR params,
             DMT_CallbackExecute callback,
             void* pUserData, UINT32 messageID)
{
    if ( hnode == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    SYNCML_DM_EXECUTE_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;
    SYNCML_DM_RET_STATUS_T status;

    message.pMsg = (DMExecuteMessage*)new DMExecuteMessage();
    if ( message.pMsg == NULL )
    {
        XPL_LOG_DM_API_Error(("dmtExecute_Post : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }

    status = message.pMsg->set(hnode, params, callback,messageID,(UINT32)pUserData);

    if ( status == SYNCML_DM_SUCCESS )
    {
        res = XPL_MSG_Send(XPL_PORT_DM_TASK,SYNCML_DM_EXECUTE_MSG_ID,&message,sizeof(SYNCML_DM_EXECUTE_MESSAGE_T));
        if ( res != XPL_MSG_RET_SUCCESS )
        {
            delete message.pMsg;
            return SYNCML_DM_FAIL;
        }
    }    
    return status;

}
#ifdef LOB_SUPPORT
SYNCML_DM_RET_STATUS_T 
dmtDataChunk_Post(DMT_H_NODE hnode,
         DMT_DATACHUNK_T *datachunk,
              DMT_CallbackDataChunk callback,
              void* pUserData, UINT32 messageID,UINT32 msgtype)
{
    if ( hnode == 0 )
        return SYNCML_DM_INVALID_PARAMETER;


    SYNCML_DM_DATA_CHUNK_MESSAGE_T message;
    XPL_MSG_RET_STATUS_T res;

    message.hnode = hnode;
    message.datachunk = datachunk;
    message.callback = callback;
    message.messageID = messageID;
    message.pUserData = (UINT32)pUserData;
   
    res = XPL_MSG_Send(XPL_PORT_DM_TASK,msgtype, &message,sizeof(SYNCML_DM_DATA_CHUNK_MESSAGE_T));
    if ( res == XPL_MSG_RET_SUCCESS )
        return SYNCML_DM_SUCCESS;
    else
        return SYNCML_DM_FAIL;
}
SYNCML_DM_RET_STATUS_T 
dmtGetFirstChunk_Post(DMT_H_NODE hnode,
         DMT_DATACHUNK_T *datachunk,
              DMT_CallbackDataChunk callback,
              void* pUserData, UINT32 messageID)
{
 return dmtDataChunk_Post(hnode, datachunk,callback, pUserData, messageID, SYNCML_DM_GET_FIRST_CHUNK_MSG_ID);
}

SYNCML_DM_RET_STATUS_T 
dmtGetNextChunk_Post(DMT_H_NODE hnode,
         DMT_DATACHUNK_T *datachunk,
              DMT_CallbackDataChunk callback,
              void* pUserData, UINT32 messageID)
{
    return dmtDataChunk_Post(hnode, datachunk,callback, pUserData, messageID, SYNCML_DM_GET_NEXT_CHUNK_MSG_ID);
}

SYNCML_DM_RET_STATUS_T 
dmtSetFirstChunk_Post(DMT_H_NODE hnode,
          DMT_DATACHUNK_T *datachunk,
              DMT_CallbackDataChunk callback,
              void* pUserData, UINT32 messageID)
{
    return dmtDataChunk_Post(hnode, datachunk,callback, pUserData, messageID, SYNCML_DM_SET_FIRST_CHUNK_MSG_ID);
}

SYNCML_DM_RET_STATUS_T 
dmtSetNextChunk_Post(DMT_H_NODE hnode,
         DMT_DATACHUNK_T *datachunk,
              DMT_CallbackDataChunk callback,
              void* pUserData, UINT32 messageID)
{
    return dmtDataChunk_Post(hnode, datachunk,callback, pUserData, messageID, SYNCML_DM_SET_NEXT_CHUNK_MSG_ID);
}

SYNCML_DM_RET_STATUS_T 
dmtSetLastChunk_Post(DMT_H_NODE hnode,
         DMT_DATACHUNK_T *datachunk,
              DMT_CallbackDataChunk callback,
              void* pUserData, UINT32 messageID)
{
    return dmtDataChunk_Post(hnode, datachunk,callback, pUserData, messageID, SYNCML_DM_SET_LAST_CHUNK_MSG_ID);
}
#endif

SYNCML_DM_ACL_PERMISSIONS_T  
DMT_AclGetPermissions(CPCHAR acl,CPCHAR principal )
{
    if ( acl == NULL || principal == NULL )
        return 0;
   
    DmtAcl oAcl(acl);

    return oAcl.GetPermissions(DmtPrincipal(principal));
}

SYNCML_DM_RET_STATUS_T
DMT_AclSetPermissions(char* acl,
                       INT32 * buf_len,
                       CPCHAR principal,
                       SYNCML_DM_ACL_PERMISSIONS_T permissions )
{
    if ( buf_len == NULL || acl == NULL || principal == NULL )
        return SYNCML_DM_INVALID_PARAMETER;

    if ( *buf_len == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    DmtAcl oAcl(acl);
    DMString result;
    oAcl.SetPermission(DmtPrincipal(principal),permissions);

    result = oAcl.toString();
    if ( result == NULL )
    {
        return SYNCML_DM_DEVICE_FULL;
    }
   
    INT32 len = result.length();
    if ( len < *buf_len ) 
    {
        DmStrcpy(acl,result.c_str());
        return SYNCML_DM_SUCCESS;
    }
    else
    {
        *buf_len = len;
        return SYNCML_DM_FAIL; 
    }    

}


SYNCML_DM_RET_STATUS_T  
DMT_NotifyOnIdle(DMT_CallbackNotifyOnIdle callback, void* pUserData )
{
    
    return dmtNotifyOnIdle_Post(callback, pUserData, 0);
}


SYNCML_DM_RET_STATUS_T
DMT_Init(DMT_CallbackStatusCode callback, void* pUserData )
{
    return dmtEngine_Post(callback, pUserData, 0,SYNCML_DM_INIT_MSG_ID);

}

SYNCML_DM_RET_STATUS_T  
DMT_Uninit(DMT_CallbackStatusCode callback, void* pUserData)
{
    return dmtEngine_Post(callback, pUserData, 0,SYNCML_DM_UNINIT_MSG_ID);

}


SYNCML_DM_RET_STATUS_T
DMT_GetTree(CPCHAR szPrincipal, 
                DMT_CallbackGetTree callback,
                void* pUserData )
{
    return DMT_GetSubtree(szPrincipal,NULL,callback,pUserData);

}                

SYNCML_DM_RET_STATUS_T
DMT_GetSubtree(CPCHAR szPrincipal, 
                  CPCHAR szSubtreeRoot,
                  DMT_CallbackGetTree callback,
                  void* pUserData )
{

    return DMT_GetSubtreeEx(szPrincipal,szSubtreeRoot,SYNCML_DM_LOCK_TYPE_AUTOMATIC,callback,pUserData);

}



SYNCML_DM_RET_STATUS_T
DMT_GetSubtreeEx(CPCHAR szPrincipal, 
                  CPCHAR szSubtreeRoot,
                  SYNCML_DM_TREE_LOCK_TYPE_T nLockType, 
                  DMT_CallbackGetTree callback,
                  void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
 
    return dmtGetSubtreeEx_Post(szPrincipal,szSubtreeRoot,nLockType,callback,pUserData,0);

}
                  

SYNCML_DM_RET_STATUS_T
DMT_TreeRelease(DMT_H_TREE htree,
                 DMT_CallbackStatusCode callback,
                 void* pUserData )
{
    return dmtTree_Post(htree,callback,pUserData,0,SYNCML_DM_RELEASE_TREE_ID);  
}
                 


SYNCML_DM_RET_STATUS_T
DMT_ProcessScript(CPCHAR szPrincipal, 
                   const UINT8 * buf, 
                   INT32 len, 
                   BOOLEAN isWBXML,
                   DMT_CallbackProcessScript callback,
                   void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtProcessScript_Post(szPrincipal,buf,len,isWBXML,callback,pUserData,0);
}
                   


SYNCML_DM_RET_STATUS_T
DMT_StartServerSession(CPCHAR szPrincipal, 
                        const DMT_SESSION_PROP_T * pSessionProp,
                        DMT_CallbackStatusCode callback,
                        void* pUserData )
{
     if ( callback == NULL )
         return SYNCML_DM_INVALID_PARAMETER;
    return dmtStartServerSession_Post(szPrincipal,pSessionProp,callback,pUserData,0);
}     


SYNCML_DM_RET_STATUS_T 
DMT_Bootstrap(CPCHAR szPrincipal,
                    const UINT8 * buf,
                    INT32 len,
                    BOOLEAN isWBXML,
                    BOOLEAN isProcess,
                    DMT_CallbackBootstrap callback,
                    void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtBootstrap_Post(szPrincipal,buf,len,isWBXML,isProcess,callback,pUserData,0);
}


SYNCML_DM_RET_STATUS_T 
DMT_ProcessNotification(CPCHAR szPrincipal,
                        const UINT8 *buf, 
                        INT32 len,
                        DMT_CallbackProcessNotification callback,
                        void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtProcessNotification_Post(szPrincipal,buf,len,callback,pUserData,0);
}


SYNCML_DM_RET_STATUS_T 
DMT_GetNode(DMT_H_TREE htree,
             CPCHAR  path,
             DMT_CallbackGetNode callback,
             void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtGetNode_Post(htree,path,callback,pUserData,0);
}             


SYNCML_DM_RET_STATUS_T
DMT_NodeRelease(DMT_H_NODE hnode,
                 DMT_CallbackStatusCode callback,
                 void* pUserData )
{
    return dmtNodeRelease_Post(hnode,callback,pUserData,0,SYNCML_DM_RELEASE_NODE_MSG_ID);   
}
                 

SYNCML_DM_RET_STATUS_T 
DMT_DeleteNode(DMT_H_TREE htree,
                CPCHAR  path,
                DMT_CallbackStatusCode callback,
                void* pUserData )
{
    return dmtTreeNode_Post(htree,path,NULL,callback,pUserData,0,SYNCML_DM_DELETE_NODE_MSG_ID);    
}
                

SYNCML_DM_RET_STATUS_T 
DMT_RenameNode(DMT_H_TREE htree,
                CPCHAR  path,
                CPCHAR  new_node_name,
                DMT_CallbackStatusCode callback,
                void* pUserData )
{
    return dmtRenameNode_Post(htree,path,new_node_name,callback,pUserData,0);
}
                

SYNCML_DM_RET_STATUS_T 
DMT_CreateInteriorNode(DMT_H_TREE htree,
                        CPCHAR  path,
                        DMT_CallbackStatusCode callback,
                        void* pUserData )
{
    return dmtTreeNode_Post(htree,path,NULL,callback,pUserData,0,SYNCML_DM_CREATE_INTERIOR_NODE_MSG_ID);
}
                        

SYNCML_DM_RET_STATUS_T 
DMT_CreateLeafNode(DMT_H_TREE htree,
                    CPCHAR path,
                    const DMT_DATA_T* data,
                    DMT_CallbackStatusCode callback,
                    void* pUserData )
{
    return dmtCreateLeafNode_Post(htree,path,data,callback,pUserData,0);

    
}
                    

SYNCML_DM_RET_STATUS_T 
DMT_GetChildNodeNames(DMT_H_TREE htree,
                       CPCHAR  path,
                       DMT_CallbackGetChildNodeNames callback,
                       void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtGetChildNodeNames_Post(htree,path,callback,pUserData,0);
}
                       

SYNCML_DM_RET_STATUS_T 
DMT_Flush(DMT_H_TREE htree, DMT_CallbackStatusCode callback, void* pUserData )
{
    return dmtTree_Post(htree,callback,pUserData,0,SYNCML_DM_FLUSH_MSG_ID);
}
           

SYNCML_DM_RET_STATUS_T 
DMT_Commit(DMT_H_TREE htree, DMT_CallbackStatusCode callback, void* pUserData )
{
    return dmtTree_Post(htree,callback,pUserData,0,SYNCML_DM_COMMIT_MSG_ID);
}
            

SYNCML_DM_RET_STATUS_T
DMT_Rollback(DMT_H_TREE htree, DMT_CallbackStatusCode callback, void* pUserData )
{
    return dmtTree_Post(htree,callback,pUserData,0,SYNCML_DM_ROLLBACK_MSG_ID);
}
              

SYNCML_DM_RET_STATUS_T 
DMT_Begin(DMT_H_TREE htree, DMT_CallbackStatusCode callback, void* pUserData )
{
    return dmtTree_Post(htree,callback,pUserData,0,SYNCML_DM_BEGIN_MSG_ID);
}
           


SYNCML_DM_RET_STATUS_T
DMT_GetChildValuesMap(DMT_H_TREE htree,
                       CPCHAR  path,
                       DMT_CallbackGetChildValuesMap callback,
                       void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtGetChildValuesMap_Post(htree,path,callback,pUserData,0);
}


SYNCML_DM_RET_STATUS_T 
DMT_SetChildValuesMap(DMT_H_TREE htree,
                       CPCHAR path,
                       const DMT_LEAF_CHILDREN_DATA_T*  data,
                       DMT_CallbackStatusCode callback,
                       void* pUserData )
{
    return dmtSetChildValuesMap_Post(htree,path,data,callback,pUserData,0);
}
                       


SYNCML_DM_RET_STATUS_T 
DMT_GetAttributes(DMT_H_NODE hnode,
                   DMT_CallbackGetAttributes callback,
                   void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtGetAttributes_Post(hnode,callback,pUserData,0);    
}
                   
                   

SYNCML_DM_RET_STATUS_T 
DMT_GetValue(DMT_H_NODE hnode,
              DMT_CallbackGetValue callback,
              void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtGetValue_Post(hnode,callback,pUserData,0);
}


SYNCML_DM_RET_STATUS_T 
DMT_SetValue(DMT_H_NODE hnode,
              const DMT_DATA_T* data,
              DMT_CallbackStatusCode callback,
              void* pUserData )
{
    return dmtSetValue_Post(hnode,data,callback,pUserData,0);
}
              

SYNCML_DM_RET_STATUS_T
DMT_SetTitle(DMT_H_NODE  hnode,
              CPCHAR title,
              DMT_CallbackStatusCode callback,
              void* pUserData )
{
    return dmtNode_Post(hnode,title,callback,pUserData,0,SYNCML_DM_SET_TITLE_MSG_ID);
}
              

SYNCML_DM_RET_STATUS_T
DMT_SetACL(DMT_H_NODE hnode,
            CPCHAR acl,
            DMT_CallbackStatusCode callback,
            void* pUserData )
{
    return dmtNode_Post(hnode,acl,callback,pUserData,0,SYNCML_DM_SET_ACL_MSG_ID);
}
            

SYNCML_DM_RET_STATUS_T 
DMT_Execute(DMT_H_NODE hnode,
             CPCHAR params,
             DMT_CallbackExecute callback,
             void* pUserData )
{
   if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
   return dmtExecute_Post(hnode,params,callback,pUserData,0);
}
#ifdef LOB_SUPPORT
SYNCML_DM_RET_STATUS_T 
DMT_GetFirstChunk(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             DMT_CallbackDataChunk  callback,
             void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtGetFirstChunk_Post(hnode, datachunk, callback,pUserData,0);
}
SYNCML_DM_RET_STATUS_T 
DMT_GetNextChunk(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
        DMT_CallbackDataChunk    callback,
             void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtGetNextChunk_Post(hnode, datachunk, callback,pUserData,0);
}

SYNCML_DM_RET_STATUS_T 
DMT_SetFirstChunk(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
        DMT_CallbackDataChunk    callback,
             void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtSetFirstChunk_Post(hnode, datachunk, callback,pUserData,0);
}

SYNCML_DM_RET_STATUS_T 
DMT_SetNextChunk(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
        DMT_CallbackDataChunk    callback,
             void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtSetNextChunk_Post(hnode, datachunk ,callback,pUserData,0);
}

SYNCML_DM_RET_STATUS_T 
DMT_SetLastChunk(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
         DMT_CallbackDataChunk    callback,
             void* pUserData )
{
    if ( callback == NULL )
        return SYNCML_DM_INVALID_PARAMETER;
    return dmtSetLastChunk_Post(hnode, datachunk, callback,pUserData,0);
}

INT32 DMT_GetChunkSize()
{
  return DmtDataChunk::GetChunkSize();
}
#else
SYNCML_DM_RET_STATUS_T 
DMT_GetFirstChunk(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             DMT_CallbackDataChunk  callback,
             void* pUserData )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}
SYNCML_DM_RET_STATUS_T 
DMT_GetNextChunk(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
        DMT_CallbackDataChunk    callback,
             void* pUserData )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T 
DMT_SetFirstChunk(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
        DMT_CallbackDataChunk    callback,
             void* pUserData )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T 
DMT_SetNextChunk(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
        DMT_CallbackDataChunk    callback,
             void* pUserData )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T 
DMT_SetLastChunk(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
         DMT_CallbackDataChunk    callback,
             void* pUserData )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

INT32 DMT_GetChunkSize()
{
  return 0;
}
#endif

SYNCML_DM_RET_STATUS_T  
DMT_NotifyOnIdle_Msg(UINT32 messageID, void* pUserData )
{

    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;
    
    return dmtNotifyOnIdle_Post(NULL, pUserData, messageID);
}
                           

/* Async APIs used message posting instead of callback */
SYNCML_DM_RET_STATUS_T
DMT_Init_Msg(UINT32 messageID, void* pUserData)
{

    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;
    
    return dmtEngine_Post(NULL, pUserData, messageID,SYNCML_DM_INIT_MSG_ID);
}

SYNCML_DM_RET_STATUS_T  
DMT_Uninit_Msg(UINT32 messageID, void* pUserData)
{
   if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtEngine_Post(NULL, pUserData, messageID,SYNCML_DM_UNINIT_MSG_ID);
}

SYNCML_DM_RET_STATUS_T
DMT_GetTree_Msg(CPCHAR szPrincipal, 
                UINT32 messageID,
                void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return DMT_GetSubtree_Msg(szPrincipal,NULL,messageID,pUserData);

}                

SYNCML_DM_RET_STATUS_T
DMT_GetSubtree_Msg(CPCHAR szPrincipal, 
                  CPCHAR szSubtreeRoot,
                  UINT32 messageID,
                  void* pUserData )
{

    return DMT_GetSubtreeEx_Msg(szPrincipal,szSubtreeRoot,SYNCML_DM_LOCK_TYPE_AUTOMATIC,messageID,pUserData);

}


SYNCML_DM_RET_STATUS_T
DMT_GetSubtreeEx_Msg(CPCHAR szPrincipal, 
                  CPCHAR szSubtreeRoot,
                  SYNCML_DM_TREE_LOCK_TYPE_T nLockType, 
                  UINT32 messageID,
                  void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtGetSubtreeEx_Post(szPrincipal,szSubtreeRoot,nLockType,NULL,pUserData,messageID);

}

SYNCML_DM_RET_STATUS_T
DMT_TreeRelease_Msg(DMT_H_TREE htree,
                 UINT32 messageID,
                 void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtTree_Post(htree,NULL,pUserData,messageID,SYNCML_DM_RELEASE_TREE_ID);  
}


SYNCML_DM_RET_STATUS_T
DMT_ProcessScript_Msg(CPCHAR szPrincipal, 
                   const UINT8 * buf, 
                   INT32 len, 
                   BOOLEAN isWBXML,
                   UINT32 messageID,
                   void* pUserData )
{

    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtProcessScript_Post(szPrincipal,buf,len,isWBXML,NULL,pUserData,messageID);

}


SYNCML_DM_RET_STATUS_T
DMT_StartServerSession_Msg(CPCHAR szPrincipal, 
                        const DMT_SESSION_PROP_T * pSessionProp,
                        UINT32 messageID,
                        void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtStartServerSession_Post(szPrincipal,pSessionProp,NULL,pUserData,messageID);
}

SYNCML_DM_RET_STATUS_T
DMT_ProcessNotification_Msg(CPCHAR szPrincipal,
                        const UINT8 *buf, 
                        INT32 len,
                        UINT32 messageID,
                        void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtProcessNotification_Post(szPrincipal,buf,len,NULL,pUserData,messageID);
}


SYNCML_DM_RET_STATUS_T
DMT_Bootstrap_Msg(CPCHAR szPrincipal,
                    const UINT8 * buf,
                    INT32 len,
                    BOOLEAN isWBXML,
                    BOOLEAN isProcess,
                    UINT32 messageID,
                    void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtBootstrap_Post(szPrincipal,buf,len,isWBXML,isProcess, NULL,pUserData,messageID);
}
                                                         

SYNCML_DM_RET_STATUS_T 
DMT_GetNode_Msg(DMT_H_TREE htree,
             CPCHAR  path,
             UINT32 messageID,
             void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtGetNode_Post(htree,path,NULL,pUserData,messageID);

}

SYNCML_DM_RET_STATUS_T
DMT_NodeRelease_Msg(DMT_H_NODE hnode,
                 UINT32 messageID,
                 void* pUserData )
{
     if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

     return dmtNodeRelease_Post(hnode,NULL,pUserData,messageID,SYNCML_DM_RELEASE_NODE_MSG_ID);
}

SYNCML_DM_RET_STATUS_T 
DMT_DeleteNode_Msg(DMT_H_TREE htree,
                CPCHAR  path,
                UINT32 messageID,
                void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtTreeNode_Post(htree,path,NULL,NULL,pUserData,messageID,SYNCML_DM_DELETE_NODE_MSG_ID);


}

SYNCML_DM_RET_STATUS_T 
DMT_RenameNode_Msg(DMT_H_TREE htree,
                CPCHAR  path,
                CPCHAR  new_node_name,
                UINT32 messageID,
                void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtRenameNode_Post(htree,path,new_node_name,NULL,pUserData,messageID);

}

SYNCML_DM_RET_STATUS_T 
DMT_CreateInteriorNode_Msg(DMT_H_TREE htree,
                        CPCHAR  path,
                        UINT32 messageID,
                        void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtTreeNode_Post(htree,path,NULL,NULL,pUserData,messageID,SYNCML_DM_CREATE_INTERIOR_NODE_MSG_ID);

}

SYNCML_DM_RET_STATUS_T 
DMT_CreateLeafNode_Msg(DMT_H_TREE htree,
                    CPCHAR path,
                    const DMT_DATA_T* data,
                    UINT32 messageID,
                    void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtCreateLeafNode_Post(htree,path,data,NULL,pUserData,messageID);

}

SYNCML_DM_RET_STATUS_T 
DMT_GetChildNodeNames_Msg(DMT_H_TREE htree,
                       CPCHAR  path,
                       UINT32 messageID,
                       void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtGetChildNodeNames_Post(htree,path,NULL,pUserData,messageID);

}

SYNCML_DM_RET_STATUS_T 
DMT_Flush_Msg(DMT_H_TREE htree, UINT32 messageID, void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtTree_Post(htree,NULL,pUserData,messageID,SYNCML_DM_FLUSH_MSG_ID);
}

SYNCML_DM_RET_STATUS_T 
DMT_Commit_Msg(DMT_H_TREE htree, UINT32 messageID, void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtTree_Post(htree,NULL,pUserData,messageID,SYNCML_DM_COMMIT_MSG_ID);
}

SYNCML_DM_RET_STATUS_T
DMT_Rollback_Msg(DMT_H_TREE htree,  UINT32 messageID, void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtTree_Post(htree,NULL,pUserData,messageID,SYNCML_DM_ROLLBACK_MSG_ID);
}

SYNCML_DM_RET_STATUS_T 
DMT_Begin_Msg(DMT_H_TREE htree,  UINT32 messageID,  void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtTree_Post(htree,NULL,pUserData,messageID,SYNCML_DM_BEGIN_MSG_ID);
}


SYNCML_DM_RET_STATUS_T
DMT_GetChildValuesMap_Msg(DMT_H_TREE htree,
                       CPCHAR  path,
                       UINT32 messageID,
                       void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtGetChildValuesMap_Post(htree,path,NULL,pUserData,messageID);
}
                       

SYNCML_DM_RET_STATUS_T 
DMT_SetChildValuesMap_Msg(DMT_H_TREE htree,
                       CPCHAR path,
                       const DMT_LEAF_CHILDREN_DATA_T*  data,
                       UINT32 messageID,
                       void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtSetChildValuesMap_Post(htree,path,data,NULL,pUserData,messageID);
}


SYNCML_DM_RET_STATUS_T 
DMT_GetAttributes_Msg(DMT_H_NODE hnode, UINT32 messageID, void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtGetAttributes_Post(hnode,NULL,pUserData,messageID);   

}

SYNCML_DM_RET_STATUS_T 
DMT_GetValue_Msg(DMT_H_NODE hnode, UINT32 messageID, void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtGetValue_Post(hnode,NULL,pUserData,messageID);   
}

SYNCML_DM_RET_STATUS_T 
DMT_SetValue_Msg(DMT_H_NODE hnode,
              const DMT_DATA_T* data,
              UINT32 messageID,
              void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtSetValue_Post(hnode,data,NULL,pUserData,messageID);
}

SYNCML_DM_RET_STATUS_T
DMT_SetTitle_Msg(DMT_H_NODE  hnode,
              CPCHAR title,
              UINT32 messageID,
              void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtNode_Post(hnode,title,NULL,pUserData,messageID,SYNCML_DM_SET_TITLE_MSG_ID);

}

SYNCML_DM_RET_STATUS_T
DMT_SetACL_Msg(DMT_H_NODE hnode,
            CPCHAR acl,
            UINT32 messageID,
            void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtNode_Post(hnode,acl,NULL,pUserData,messageID,SYNCML_DM_SET_ACL_MSG_ID);

}

SYNCML_DM_RET_STATUS_T 
DMT_Execute_Msg(DMT_H_NODE hnode,
             CPCHAR params,
             UINT32 messageID,
             void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtExecute_Post(hnode,params,NULL,pUserData,messageID);
}

#ifdef LOB_SUPPORT

SYNCML_DM_RET_STATUS_T 
DMT_GetFirstChunk_Msg(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtGetFirstChunk_Post(hnode, datachunk, NULL,pUserData,messageID);
}

SYNCML_DM_RET_STATUS_T 
DMT_GetNextChunk_Msg(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtGetNextChunk_Post(hnode, datachunk, NULL,pUserData,messageID);
}

SYNCML_DM_RET_STATUS_T 
DMT_SetFirstChunk_Msg(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtSetFirstChunk_Post(hnode, datachunk, NULL,pUserData,messageID);
}

SYNCML_DM_RET_STATUS_T 
DMT_SetNextChunk_Msg(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtSetNextChunk_Post(hnode, datachunk, NULL,pUserData,messageID);
}

SYNCML_DM_RET_STATUS_T 
DMT_SetLastChunk_Msg(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData )
{
    if ( messageID == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    return dmtSetLastChunk_Post(hnode, datachunk, NULL,pUserData,messageID);
}

#else   // LOB_SUPPORT

SYNCML_DM_RET_STATUS_T 
DMT_GetFirstChunk_Msg(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData )
{
 return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T 
DMT_GetNextChunk_Msg(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T 
DMT_SetFirstChunk_Msg(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T 
DMT_SetNextChunk_Msg(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

SYNCML_DM_RET_STATUS_T 
DMT_SetLastChunk_Msg(DMT_H_NODE hnode,
        DMT_DATACHUNK_T *datachunk,
             UINT32 messageID,
             void* pUserData )
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}

#endif  // LOB_SUPPORT

#ifdef __cplusplus
}
#endif
