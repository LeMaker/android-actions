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

#include "dmtAsyncData.h"
#include "dm_tree_util.h" 
#include "dmt.hpp"
#include "dmMemory.h"
#include "xpl_Logger.h"

static SYNCML_DM_RET_STATUS_T dmtBuildData(const DMT_DATA_T * pData, DmtData & oDmtData)
{

    SYNCML_DM_RET_STATUS_T res = SYNCML_DM_SUCCESS;
    if (  pData == NULL )
    {
        oDmtData = DmtData();
    }   
    else
        switch ( pData->meta_format )
        {
          case SYNCML_DM_DATAFORMAT_STRING:
          case SYNCML_DM_DATAFORMAT_FLOAT:
          case SYNCML_DM_DATAFORMAT_TIME:
          case SYNCML_DM_DATAFORMAT_DATE:
              res = oDmtData.SetString(pData->data.str_value, pData->meta_format);
              break;

          case SYNCML_DM_DATAFORMAT_INT:
              res = oDmtData.SetInt(pData->data.int_value);
              break;
            
          case SYNCML_DM_DATAFORMAT_BOOL:
              res = oDmtData.SetBoolean((BOOLEAN)pData->data.int_value);
              break;
            
          case SYNCML_DM_DATAFORMAT_BIN:
              res = oDmtData.SetBinary(pData->data.bin.bin_value,pData->data.bin.len_bin_data);
              break;

          default:
              oDmtData = DmtData();
              break;
       }
    return res;
    
}


static SYNCML_DM_RET_STATUS_T dmtBuildMap(const DMT_LEAF_CHILDREN_DATA_T* pData, DMMap<DMString, DmtData> & oMapNodes)
{
    SYNCML_DM_RET_STATUS_T res = SYNCML_DM_SUCCESS;

    if ( pData )
    {
        for (int index = 0; index<pData->num_children; index++)
        {
            DmtData oData;
            res = dmtBuildData((DMT_DATA_T*)&pData->pData[index],oData);
            if ( res == SYNCML_DM_SUCCESS )
                oMapNodes.put(DMString(pData->ppChildren[index]),oData);
            else
                break;
        }
    }
    else
        return SYNCML_DM_FAIL;

    return res;
}        



static void dmtFreeCharPtr(CPCHAR str)
{
    char * ptr = (char*)str;
    if ( ptr )
        DmFreeMem(ptr);
}


static void dmtFreeBytePtr(const UINT8 * byte)
{
    UINT8 * ptr = (UINT8*)byte;
    if ( ptr )
        DmFreeMem(ptr);
}

static void dmtFreeDataStruct(DMT_DATA_T * pData)
{
    if ( pData == NULL )
        return;

    switch ( pData->meta_format )
    {
        case SYNCML_DM_DATAFORMAT_STRING:
        case SYNCML_DM_DATAFORMAT_FLOAT:
        case SYNCML_DM_DATAFORMAT_TIME:
        case SYNCML_DM_DATAFORMAT_DATE:
            dmtFreeCharPtr(pData->data.str_value);
            break;

        case SYNCML_DM_DATAFORMAT_BIN:
            dmtFreeBytePtr(pData->data.bin.bin_value);
            break;
    }
    memset(pData,0,sizeof(DMT_DATA_T));
}


SYNCML_DM_RET_STATUS_T DMPrincipalMessage::set(CPCHAR szPrincipal,
                                                UINT32 messageID, 
                                                UINT32 pUserData)
{
    DMAsyncMessage::set(messageID,pUserData);
    principal.assign(szPrincipal);
    if ( principal.getName() == NULL )
    {
        return SYNCML_DM_DEVICE_FULL; 
    }
    return SYNCML_DM_SUCCESS;
}



SYNCML_DM_RET_STATUS_T DMGetSubTreeMessage::set(CPCHAR szPrincipal, 
                                                CPCHAR subtreeRoot,
                                                SYNCML_DM_TREE_LOCK_TYPE_T nLockType,
                                                DMT_CallbackGetTree callback,
                                                UINT32 messageID, 
                                                UINT32 pUserData)
{
    SYNCML_DM_RET_STATUS_T res;
    res = DMPrincipalMessage::set(szPrincipal,messageID,pUserData);
    if ( res == SYNCML_DM_SUCCESS )
    {
        if ( subtreeRoot )
        {
            this->subtreeRoot = subtreeRoot;
            if ( subtreeRoot[0] && this->subtreeRoot == NULL )
            {
                return SYNCML_DM_DEVICE_FULL; 
            }
        }    
    }
    this->nLockType = nLockType;
    this->callback = callback;
    return res;
}



SYNCML_DM_RET_STATUS_T DMScriptMessage::set(CPCHAR szPrincipal, 
                                            const UINT8 * buf, 
                                            INT32 len, 
                                            BOOLEAN isWBXML,
                                            UINT32 messageID, 
                                            UINT32 pUserData)
{
    SYNCML_DM_RET_STATUS_T res;
    res = DMPrincipalMessage::set(szPrincipal,messageID,pUserData);
    if ( res == SYNCML_DM_SUCCESS )
    {
        this->buf.assign(buf,len);
        if ( this->buf.getBuffer() == NULL )
        {
            return SYNCML_DM_DEVICE_FULL; 
        }
    }
    this->isWBXML = isWBXML;
    return res;
}


SYNCML_DM_RET_STATUS_T DMProcessScriptMessage::set(CPCHAR szPrincipal, 
                                                   const UINT8 * buf, 
                                                   INT32 len, 
                                                   BOOLEAN isWBXML,
                                                   DMT_CallbackProcessScript callback,
                                                   UINT32 messageID, 
                                                   UINT32 pUserData)
{
    this->callback = callback;
    return DMScriptMessage::set(szPrincipal,buf,len,isWBXML,messageID,pUserData);
}


SYNCML_DM_RET_STATUS_T DMBootstrapMessage::set(CPCHAR szPrincipal, 
                                               const UINT8 * buf, 
                                               INT32 len, 
                                               BOOLEAN isWBXML,
                                               BOOLEAN isProcess,
                                               DMT_CallbackBootstrap callback,
                                               UINT32 messageID, 
                                               UINT32 pUserData)
{
    this->callback = callback;
    this->isProcess = isProcess;
    return DMScriptMessage::set(szPrincipal,buf,len,isWBXML,messageID,pUserData);
}



SYNCML_DM_RET_STATUS_T DMStartServerSessionMessage::set(CPCHAR szPrincipal, 
                                                        const DMT_SESSION_PROP_T * pSessionProp,
                                                        DMT_CallbackStatusCode callback,
                                                        UINT32 messageID, 
                                                        UINT32 pUserData)
{
    sessionProp.setWBXML(pSessionProp->isWBXML);
      
    if ( pSessionProp->direction == SYNCML_DM_SERVER_INITIATED_SESSION )
       sessionProp.setSessionID(pSessionProp->sessionID);
    if ( pSessionProp->num_alerts )
    {
        if ( !pSessionProp->alerts ) 
            return SYNCML_DM_INVALID_PARAMETER;
        for (INT32 i=0; i<pSessionProp->num_alerts; i++)
        {
            DmtFirmAlert alert;
            SYNCML_DM_RET_STATUS_T res;
            INT32 count;

            res = alert.setAlertType(((pSessionProp->alerts)+i)->strAlertType);
            if ( res != SYNCML_DM_SUCCESS )
                return res;
            
            res = alert.setAlertFormat(((pSessionProp->alerts)+i)->strAlertFormat);
            if ( res != SYNCML_DM_SUCCESS )
                return res;

            res = alert.setAlertMark(((pSessionProp->alerts)+i)->strAlertMark);
            if ( res != SYNCML_DM_SUCCESS )
                return res;

            res = alert.setResultData(((pSessionProp->alerts)+i)->strResultData);
            if ( res != SYNCML_DM_SUCCESS )
                return res;

            res = alert.setCorrelator(((pSessionProp->alerts)+i)->strCorrelator);
            if ( res != SYNCML_DM_SUCCESS )
                return res;

            if ( ((pSessionProp->alerts)+i)->strPackageURI != NULL )
            {
	            res = alert.setPackageURI(((pSessionProp->alerts)+i)->strPackageURI);
	            if ( res != SYNCML_DM_SUCCESS )
	                return res;
            	}			

            count = sessionProp.addFirmAlert(alert);
            if ( count != i+1 )
                return SYNCML_DM_DEVICE_FULL;
        }
    }
    this->callback = callback;
    return DMPrincipalMessage::set(szPrincipal,messageID,pUserData);
}




SYNCML_DM_RET_STATUS_T DMProcessNotificationMessage::set(CPCHAR szPrincipal, 
                                                         const UINT8 * buf, 
                                                         INT32 len,
                                                         DMT_CallbackProcessNotification callback,
                                                         UINT32 messageID, 
                                                         UINT32 pUserData)
{
    SYNCML_DM_RET_STATUS_T res;
    res = DMPrincipalMessage::set(szPrincipal,messageID,pUserData);
    if ( res == SYNCML_DM_SUCCESS )
    {
        this->buf.assign(buf,len);
        if ( this->buf.getBuffer() == NULL )
        {
            return SYNCML_DM_DEVICE_FULL; 
        }
    }
    this->callback = callback;
    return res;
}


SYNCML_DM_RET_STATUS_T DMTreeMessage::set(DMT_H_TREE htree, 
                                           CPCHAR path,
                                           UINT32 messageID, 
                                           UINT32 pUserData)
{
    DMAsyncMessage::set(messageID,pUserData);
    if ( path )
    {
        this->path = path;
        if ( path[0] && this->path == NULL )
        {
            return SYNCML_DM_DEVICE_FULL; 
        }
    }    
    this->htree = htree;
    return SYNCML_DM_SUCCESS;
}



SYNCML_DM_RET_STATUS_T DMGetNodeMessage::set(DMT_H_TREE htree, 
                                             CPCHAR path, 
                                             DMT_CallbackGetNode callback,
                                             UINT32 messageID, 
                                             UINT32 pUserData)
{
    this->callback = callback;
    return DMTreeMessage::set(htree,path,messageID,pUserData);
}  


SYNCML_DM_RET_STATUS_T DMTreeNodeMessage::set(DMT_H_TREE htree, 
                                                CPCHAR path, 
                                                CPCHAR str, 
                                                DMT_CallbackStatusCode callback,
                                                UINT32 messageID, 
                                                UINT32 pUserData)
{
    SYNCML_DM_RET_STATUS_T res;
    res = DMTreeMessage::set(htree,path,messageID,pUserData);
    if ( res == SYNCML_DM_SUCCESS )
    {
        if ( str )
        {
            this->str = str;
            if ( str[0] && this->str == NULL )
            {
                return SYNCML_DM_DEVICE_FULL; 
            }
        }    
        this->callback = callback;
    }  
    return res;
}



SYNCML_DM_RET_STATUS_T DMCreateLeafNodeMessage::set(DMT_H_TREE htree, 
                                                    CPCHAR path, 
                                                    const DMT_DATA_T* data, 
                                                    DMT_CallbackStatusCode callback,
                                                    UINT32 messageID, 
                                                    UINT32 pUserData)
{
    SYNCML_DM_RET_STATUS_T res;
    this->callback = callback;
    res = DMTreeMessage::set(htree,path,messageID,pUserData);
    if ( res == SYNCML_DM_SUCCESS )
        res = dmtBuildData(data, this->data);
    return res;
}


SYNCML_DM_RET_STATUS_T DMGetChildNodeNamesMessage::set(DMT_H_TREE htree, 
                                                       CPCHAR path,
                                                       DMT_CallbackGetChildNodeNames callback,
                                                       UINT32 messageID, 
                                                       UINT32 pUserData)
{
    this->callback = callback;
    return DMTreeMessage::set(htree,path,messageID,pUserData);
}


SYNCML_DM_RET_STATUS_T  DMGetChildValuesMapMessage::set(DMT_H_TREE htree, 
                                                        CPCHAR path, 
                                                        DMT_CallbackGetChildValuesMap callback,
                                                        UINT32 messageID, 
                                                        UINT32 pUserData)
{
    this->callback = callback;
    return DMTreeMessage::set(htree,path,messageID,pUserData);
}


SYNCML_DM_RET_STATUS_T DMSetChildValuesMapMessage::set(DMT_H_TREE htree, 
                                                       CPCHAR path, 
                                                       const DMT_LEAF_CHILDREN_DATA_T*  data,
                                                       DMT_CallbackStatusCode callback,
                                                       UINT32 messageID, 
                                                       UINT32 pUserData)
{
    SYNCML_DM_RET_STATUS_T res;
    this->callback = callback;
    res = DMTreeMessage::set(htree,path,messageID,pUserData);
    if ( res == SYNCML_DM_SUCCESS )
      res = dmtBuildMap(data, this->data);
    return res;
}


SYNCML_DM_RET_STATUS_T DMSetValueMessage::set(DMT_H_NODE hnode, 
                                              const DMT_DATA_T* data, 
                                              DMT_CallbackStatusCode callback,
                                              UINT32 messageID, 
                                              UINT32 pUserData)
{
      DMAsyncMessage::set(messageID,pUserData);
      this->hnode = hnode;
      this->callback = callback;
      return dmtBuildData(data, this->data);
}



SYNCML_DM_RET_STATUS_T DMNodeMessage::set(DMT_H_NODE hnode, 
                                          CPCHAR str, 
                                          DMT_CallbackStatusCode callback,
                                          UINT32 messageID, 
                                          UINT32 pUserData)
{
    DMAsyncMessage::set(messageID,pUserData);
    this->hnode = hnode;
    this->callback = callback;
    if ( str )
    {
        this->str = str;
        if ( str[0] && this->str == NULL )
        {
            return SYNCML_DM_DEVICE_FULL; 
        }
    }    
    return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T DMExecuteMessage::set(DMT_H_NODE hnode, 
                                              CPCHAR params, 
                                              DMT_CallbackExecute callback,
                                              UINT32 messageID, 
                                              UINT32 pUserData)
{
    DMAsyncMessage::set(messageID,pUserData);
    this->hnode = hnode;
    this->callback = callback;
    this->params = params;
    if ( params )
    {
        if ( params[0] && this->params == NULL )
        {
            return SYNCML_DM_DEVICE_FULL; 
        }
    }    
    return SYNCML_DM_SUCCESS;
}


void DMT_Free_GetChildNodeNamesStruct(DMT_CALLBACK_STRUCT_GET_CHILDNODE_NAMES_T *pStruct)
{
    if ( pStruct == NULL )
        return;

    if ( pStruct->ppChildren )
    {
        for (INT32 index=0; index<pStruct->num_children; index++)
        {
            if ( pStruct->ppChildren[index] )
                dmtFreeCharPtr(pStruct->ppChildren[index]);
        }   
        char ** ptr = (char**)pStruct->ppChildren;
        DmFreeMem(ptr);   
    }    
    memset(pStruct,0,sizeof(DMT_CALLBACK_STRUCT_GET_CHILDNODE_NAMES_T));
}


void DMT_Free_GetChildValuesStruct(DMT_CALLBACK_STRUCT_GET_CHILDVALUES_MAP_T *pStruct)
{
    
    if ( pStruct == NULL )
        return;

    if ( pStruct->data.ppChildren )
    {
        for (INT32 index=0; index<pStruct->data.num_children; index++)
        {
            if ( pStruct->data.ppChildren[index] )
                dmtFreeCharPtr(pStruct->data.ppChildren[index]);
        }
        char ** ptr = (char**)pStruct->data.ppChildren;
        DmFreeMem(ptr);
    }        
             
    if ( pStruct->data.pData )
    {
       for (INT32 index=0; index<pStruct->data.num_children; index++) 
       {
            dmtFreeDataStruct((DMT_DATA_T*)&pStruct->data.pData[index]);
       }
       DMT_DATA_T * ptr = (DMT_DATA_T*)(pStruct->data.pData); 
       DmFreeMem(ptr);
    }   
    memset(pStruct,0,sizeof(DMT_CALLBACK_STRUCT_GET_CHILDVALUES_MAP_T));


}

void DMT_Free_GetAttributesStruct(DMT_CALLBACK_STRUCT_GET_ATTRIBUTES_T *pStruct)
{
    if ( pStruct == NULL )
        return;

    dmtFreeCharPtr(pStruct->attributes.name);
    dmtFreeCharPtr(pStruct->attributes.format);
    dmtFreeCharPtr(pStruct->attributes.title);
    dmtFreeCharPtr(pStruct->attributes.type);
    dmtFreeCharPtr(pStruct->attributes.acl);

    memset(pStruct,0,sizeof(DMT_CALLBACK_STRUCT_GET_ATTRIBUTES_T));
}


void DMT_Free_GetValueStruct(DMT_CALLBACK_STRUCT_GET_VALUE_T *pStruct)
{
    if ( pStruct == NULL )
        return;

    dmtFreeDataStruct(&pStruct->data);
    memset(pStruct,0,sizeof(DMT_CALLBACK_STRUCT_GET_VALUE_T));
    
}

void DMT_Free_ExecuteStruct(DMT_CALLBACK_STRUCT_EXECUTE_T *pStruct)
{
    if ( pStruct == NULL )
        return;

    dmtFreeCharPtr(pStruct->result);
    memset(pStruct,0,sizeof(DMT_CALLBACK_STRUCT_EXECUTE_T));

}


void DMT_Free_ProcessScriptStruct(DMT_CALLBACK_STRUCT_PROCESS_SCRIPT_T *pStruct)
{
    if ( pStruct == NULL )
        return;

    dmtFreeCharPtr(pStruct->result);
    memset(pStruct,0,sizeof(DMT_CALLBACK_STRUCT_PROCESS_SCRIPT_T));

}

void DMT_Free_BootstrapStruct(DMT_CALLBACK_STRUCT_BOOTSTRAP_T *pStruct)
{
    if ( pStruct == NULL )
        return;

    dmtFreeCharPtr(pStruct->serverID);
    memset(pStruct,0,sizeof(DMT_CALLBACK_STRUCT_BOOTSTRAP_T));

}

void DMT_Free_ProcessNotificationStruct(DMT_CALLBACK_STRUCT_PROCESS_NOTIFICATION_T *pStruct)
{
    if ( pStruct == NULL )
        return;

    dmtFreeCharPtr(pStruct->notification.serverID);
    memset(pStruct,0,sizeof(DMT_NOTIFICATION_T));
}
