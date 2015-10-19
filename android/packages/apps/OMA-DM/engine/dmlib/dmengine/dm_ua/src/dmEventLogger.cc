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

/*==================================================================================================

    Source Name: dmEventLogger.cc

    General Description: Implementation of the DMEventLogger class

==================================================================================================*/

#include "dmEventLogger.h"
#include "dm_uri_utils.h"
#include "dmtoken.h"
#include "xpl_dm_Notifications.h"
#include "dmPluginManager.h"
#include "dm_tree_class.H" 
#include "dmEvent.h"
#include "xpl_Logger.h"

DMEventLogger::DMEventLogger()
{
     m_pTree = NULL;
}


DMEventLogger::~DMEventLogger()
{
}

SYNCML_DM_RET_STATUS_T DMEventLogger::Init( DMTree* tree )
{
    if( !tree ) return SYNCML_DM_FAIL;
  
    m_pTree = tree;
    return SYNCML_DM_SUCCESS;
}


PDmtEventData 
DMEventLogger::Find(const DmtEventDataVector & aVector,
                             SYNCML_DM_EVENT_ACTION_T nAction, 
                             BOOLEAN bIsCumulative,
                             BOOLEAN bIsEnabledByParent,
                             CPCHAR szName,
                             CPCHAR szNewName)
{

    if ( bIsCumulative )
    {
        // no need to search if new node added in it is tracked by node itself
        if ( nAction == SYNCML_DM_EVENT_ADD && !bIsEnabledByParent )
            return NULL;
    }
    else
    {
        if ( nAction != SYNCML_DM_EVENT_INDIRECT && nAction != SYNCML_DM_EVENT_REPLACE ) 
           return NULL;   
    }

    for (INT32 index=aVector.size()-1; index>=0; index--)
    {
        PDmtEventData & pEventData = (PDmtEventData&)(aVector[index]);
        if ( pEventData->GetName() == szName || pEventData->GetNewName() == szName )
        {
            return pEventData;
        }
    }
    return NULL;
}



SYNCML_DM_RET_STATUS_T 
DMEventLogger::AddEvent(PDmtEventData & pEventData,
                        SYNCML_DM_EVENT_ACTION_T nAction, 
                        CPCHAR szNewName)
{

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    ((DMEventData*)pEventData.GetPtr())->AddAction(nAction);
    if ( nAction == SYNCML_DM_EVENT_RENAME && szNewName )
    {
        dm_stat = ((DMEventData*)pEventData.GetPtr())->SetNewName(szNewName);
    }

    XPL_LOG_DM_TMN_Debug(("New action is added to logger, name = %s\n", (pEventData->GetName().c_str())));
    return SYNCML_DM_SUCCESS;   
}   



SYNCML_DM_RET_STATUS_T 
DMEventLogger::AddEvent(DmtEventDataVector & aVector,
                        SYNCML_DM_EVENT_ACTION_T nAction, 
                        BOOLEAN bIsLeaf,
                        BOOLEAN bIsEnabledByParent,
                        CPCHAR szName,
                        CPCHAR szNewName)
{

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    DMEventData* pDataPtr = NULL;

    pDataPtr = new DMEventData();
    if ( pDataPtr == NULL )
        return SYNCML_DM_DEVICE_FULL;

    pDataPtr->SetAction(nAction);
    pDataPtr->SetLeaf(bIsLeaf);
    pDataPtr->SetEnabledByParent(bIsEnabledByParent);

    dm_stat =   pDataPtr->SetName(szName);
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
        delete pDataPtr;
        return dm_stat;
    }      

    if ( nAction == SYNCML_DM_EVENT_RENAME && szNewName )
    {
        dm_stat = pDataPtr->SetNewName(szNewName);
        if ( dm_stat != SYNCML_DM_SUCCESS )
        {
            delete pDataPtr;
            return dm_stat;
        }
    }     

    aVector.push_back(PDmtEventData(pDataPtr));
    XPL_LOG_DM_TMN_Debug(("New event is added to logger, name = %s\n", szName));

    return SYNCML_DM_SUCCESS;   
}   



BOOLEAN 
DMEventLogger::Find(CPCHAR szPath, 
                              const DMEventMap & aEventMap,
                              PDMEventPath & pEventPath,
                              DmtEventDataVector & aVector)
{
                              
    for ( DmtEventMap::POS nPos = 0; nPos < aEventMap.end(); nPos++ )
    {
        pEventPath =  aEventMap.get_key(nPos); 
        if ( pEventPath ->GetPath() == szPath )
        { 
            aVector =  aEventMap.get_value(nPos);   
            return TRUE;
            break;
        }
    }
    return FALSE;

}    



SYNCML_DM_RET_STATUS_T 
DMEventLogger::OnNodeChanged(CPCHAR szPath ,
                             SYNCML_DM_EVENT_ACTION_T nAction, 
                             DMEventMap & aEventMap,
                             BOOLEAN bIsCumulative,
                             BOOLEAN bIsEnabledByParent,
                             CPCHAR szNewName )
{

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    
    DMURI oURI(FALSE, szPath);

    CPCHAR pParent = oURI.getParentURI();
    CPCHAR pNode = oURI.getTailSegments();

    DmtEventDataVector aVector;
    PDMEventPath pEventPath;
    BOOLEAN bFound = Find(pParent,aEventMap, pEventPath,aVector);

    if ( bFound == FALSE )
    {
        pEventPath = PDMEventPath(new DMEventPath(pParent));

        if ( pEventPath  == NULL )
        {
            XPL_LOG_DM_TMN_Error(("Memory allocation error \n"));
            return SYNCML_DM_DEVICE_FULL;
        }    
    }

    PDmtEventData pEventData = NULL;
    pEventData = Find(aVector,nAction, bIsCumulative,bIsEnabledByParent,pNode,szNewName);
    
    if ( pEventData != NULL )
    {
          dm_stat = AddEvent(pEventData, nAction, szNewName);
    }
    else
    {
         BOOLEAN bIsLeaf = FALSE;

         if ( nAction == SYNCML_DM_EVENT_REPLACE )
            bIsLeaf = TRUE;
        else
        {
            DMMetaDataManager & m_oMDFObj =  m_pTree->GetMetaDataManager();
            bIsLeaf = m_oMDFObj.IsLeaf(szPath);
        }    
        dm_stat = AddEvent(aVector, nAction, bIsLeaf, bIsEnabledByParent, pNode, szNewName);
    }
    
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
        XPL_LOG_DM_TMN_Error(("Cannot add new event, dm_stat = %d\n", dm_stat));
        return dm_stat;
    }    
  
    aEventMap.put( pEventPath , aVector );
  
    return SYNCML_DM_SUCCESS;
  
}



SYNCML_DM_RET_STATUS_T 
DMEventLogger::OnNodeChanged(CPCHAR szPath,
                             SYNCML_DM_EVENT_ACTION_T nAction, 
                             const PDMPlugin & pPlugin,
                             BOOLEAN bIsEnabledByParent,
                             CPCHAR szNewName )
{

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    BOOLEAN bIsCumulative = FALSE;

    DMEventMap aEventMap;

    XPL_LOG_DM_TMN_Debug(("OnNodeChanged called for commit plug-in %s\n", szPath));
    m_aPluginEvents.lookup(pPlugin,aEventMap);
  
    if ( pPlugin->GetType() == SYNCML_DM_EVENT_CUMULATIVE )
         bIsCumulative = TRUE;
  
    dm_stat =  OnNodeChanged(szPath,nAction,aEventMap,bIsCumulative,bIsEnabledByParent,szNewName);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
   
    m_aPluginEvents.put(pPlugin,aEventMap);
    return SYNCML_DM_SUCCESS;
  
}


SYNCML_DM_RET_STATUS_T 
DMEventLogger::OnNodeChanged(CPCHAR szPath,
                             SYNCML_DM_EVENT_ACTION_T nAction, 
                             const PDMConfigItem & pItem,
                             BOOLEAN bIsEnabledByParent,
                             CPCHAR szNewName )
{

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    BOOLEAN bIsCumulative = FALSE;

    DMEventMap aEventMap;
    XPL_LOG_DM_TMN_Debug(("OnNodeChanged called for ES %s\n", szPath));
    m_aPostEvents.lookup(pItem,aEventMap);
  
    if ( ((DMSubscriptionItem*)(pItem.GetPtr()))->GetType() == SYNCML_DM_EVENT_CUMULATIVE )
         bIsCumulative = TRUE;
  
    dm_stat =  OnNodeChanged(szPath,nAction,aEventMap,bIsCumulative,bIsEnabledByParent,szNewName);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
   
    m_aPostEvents.put(pItem,aEventMap);
    return SYNCML_DM_SUCCESS;
  
}


UINT32 DMEventLogger::GetSize(const DMEventMap & aEventMap)
{
    UINT32 size = sizeof(UINT32) * 2;

    for ( DMEventMap::POS nPos = 0; nPos < aEventMap.end(); nPos++ )
    {
         const PDMEventPath & pPath = aEventMap.get_key(nPos);
         const DmtEventDataVector & aVector = aEventMap.get_value(nPos);

         size += pPath->GetSize();
         size += sizeof(UINT32);
         for (INT32 index=0; index<aVector.size(); index++)
         {
            const PDmtEventData & pData = aVector[index];
            DMEventData * pDataPtr  = (DMEventData*)pData.GetPtr();
            size += pDataPtr->GetSize();
         }
    }     

    return size;
}




SYNCML_DM_RET_STATUS_T 
DMEventLogger::Serialize(const DmtEventDataVector & aVector, 
                         DMBufferWriter & oBuffer)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS; 

    oBuffer.WriteUINT32(aVector.size());
    
    for (INT32 index=0; index<aVector.size(); index++)
    {
         const PDmtEventData & pData = aVector[index];
         dm_stat = ((DMEventData*)(pData.GetPtr()))->Serialize(oBuffer);
         if ( dm_stat != SYNCML_DM_SUCCESS )
             return dm_stat;
    }     

    return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T 
DMEventLogger::Serialize(const DMEventMap & aEventMap, 
                         DMBufferWriter & oBuffer)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS; 

    INT32 size = GetSize(aEventMap);
    dm_stat = oBuffer.Allocate(size);

    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
  
    oBuffer.WriteUINT32(size);

    oBuffer.WriteUINT32(aEventMap.end());
    
    for ( DMEventMap::POS nPos = 0; nPos < aEventMap.end(); nPos++ )
    {
         const PDMEventPath & pPath = aEventMap.get_key(nPos);
         const DmtEventDataVector & aVector = aEventMap.get_value(nPos);

         dm_stat = pPath->Serialize(oBuffer);
         if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;

         dm_stat = Serialize(aVector,oBuffer); 
         if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;
    }     

    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
DMEventLogger::Deserialize(DMBufferReader & oBuffer,
                           DMString & strParent,
                           PDmtEventData & aData)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS; 

    UINT32 size = oBuffer.ReadUINT32();
    if ( size != oBuffer.GetSize() )
        return SYNCML_DM_FAIL;
  
    INT32 nPathsCount = (INT32)oBuffer.ReadUINT32();
    if ( nPathsCount != 1 )
        return SYNCML_DM_FAIL;
    
    DMEventPath oPath;
    dm_stat = oPath.Deserialize(oBuffer);

    strParent = oPath.GetPath();

    INT32 nNodesCount =  (INT32)oBuffer.ReadUINT32();   
    if ( nNodesCount != 1 )
       return SYNCML_DM_FAIL;
       
    DMEventData * pDataPtr = new DMEventData();
    if ( pDataPtr == NULL )
         return SYNCML_DM_DEVICE_FULL; 

    dm_stat = pDataPtr->Deserialize(oBuffer);
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
          delete pDataPtr;
          return dm_stat;
    }

    aData = PDmtEventData(pDataPtr);

    return SYNCML_DM_SUCCESS;

}
  


SYNCML_DM_RET_STATUS_T 
DMEventLogger::Deserialize(DMBufferReader & oBuffer,
                           DmtEventDataVector & aVector) 
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS; 

    INT32 nNodesCount =  (INT32)oBuffer.ReadUINT32();   
    if ( nNodesCount == 0 )
        return SYNCML_DM_FAIL;
        
    for (INT32 index =0; index<nNodesCount; index++)
    {
        DMEventData * pDataPtr = new DMEventData();
        if ( pDataPtr == NULL )
            return SYNCML_DM_DEVICE_FULL; 

        dm_stat = pDataPtr->Deserialize(oBuffer);
        if ( dm_stat != SYNCML_DM_SUCCESS )
        {
            delete pDataPtr;
            return dm_stat;
        }
        aVector.push_back(PDmtEventData(pDataPtr));
    }
    
    return SYNCML_DM_SUCCESS;
}




SYNCML_DM_RET_STATUS_T 
DMEventLogger::Deserialize(DMBufferReader & oBuffer,
                           DmtEventMap & aEventMap) 
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS; 

    UINT32 size = oBuffer.ReadUINT32();
    if ( size != oBuffer.GetSize() )
        return SYNCML_DM_FAIL;
  
    INT32 nPathsCount = (INT32)oBuffer.ReadUINT32();
    if ( nPathsCount == 0 )
        return SYNCML_DM_FAIL;
    
    for (INT32 index = 0; index<nPathsCount; index++)
    {
        DMEventPath oPath;
        dm_stat = oPath.Deserialize(oBuffer);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;

        DmtEventDataVector aVector;

        dm_stat = Deserialize(oBuffer,aVector);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;

        aEventMap.put(oPath.GetPath(), aVector);

    }     

    return SYNCML_DM_SUCCESS;
}

void DMEventLogger::OnTreeSaved()
{

    for ( DMPostEventMap::POS nPos = 0; nPos < m_aPostEvents.end(); nPos++ )
    {
        const PDMConfigItem & pItem = m_aPostEvents.get_key(nPos);
        const DMEventMap & aEventMap = m_aPostEvents.get_value(nPos); 
        SYNCML_DM_EVENT_TYPE_T nType =  ((DMSubscriptionItem*)(pItem.GetPtr()))->GetType();
        DMString strTopic = ((DMSubscriptionItem*)(pItem.GetPtr()))->GetTopic();

        DMBufferWriter oBuffer; 
        if ( Serialize(aEventMap,oBuffer) == SYNCML_DM_SUCCESS )
        {
             if ( strTopic == NULL ) 
             {
                strTopic = pItem->GetPath(); 
                XPL_LOG_DM_TMN_Debug(("Sending event for %s\n", strTopic.c_str()));
                XPL_DM_NotifyTreeUpdate(strTopic, NULL, nType, oBuffer.GetBuffer(), oBuffer.GetSize());
             }
             else
             {
                XPL_LOG_DM_TMN_Debug(("Sending event for %s\n", strTopic.c_str()));
                XPL_DM_NotifyTreeUpdate(strTopic, pItem->GetPath(), nType, oBuffer.GetBuffer(), oBuffer.GetSize());
             }   
        }     
        else
        {
            XPL_LOG_DM_TMN_Error(("Event Serialization failed\n"));
        }  
    }  

    Reset();
}

void DMEventLogger::Reset()
{
    m_aPostEvents.clear();
    m_aPluginEvents.clear();
}

void DMEventLogger::GetCommitPluginEvents(const PDMPlugin & pPlugin,
                                          DmtEventMap  & aUpdatedNodes ) const
{
    DMEventMap aStoredMap;
    
    m_aPluginEvents.lookup(pPlugin,aStoredMap);
    for ( DmtEventMap::POS nPos = 0; nPos < aStoredMap.end(); nPos++ )
    {
         const PDMEventPath & pPath = aStoredMap.get_key(nPos);
         const DmtEventDataVector & aVector = aStoredMap.get_value(nPos);
         aUpdatedNodes.put(pPath->GetPath(),aVector);
    }
    
}



SYNCML_DM_RET_STATUS_T 
DMEventLogger::CleanEvents(DmtEventDataVector & aVector)
{

    for ( INT32 index = aVector.size()-1; index >= 0; index-- )
    {
        const PDmtEventData & pData = aVector[index];
        if (  ((DMEventData*)(pData.GetPtr()))->IsEnabledByParent() )
            aVector.remove(index);  
    }  
    return SYNCML_DM_SUCCESS;

}


SYNCML_DM_RET_STATUS_T 
DMEventLogger::CleanEvents(DMEventMap & aEventMap, CPCHAR szPath)
{

    for ( DMEventMap::POS nPos = aEventMap.end()-1; nPos >= 0; nPos-- )
    {
        const PDMEventPath & pPath = aEventMap.get_key(nPos);
     
        BOOLEAN bIsParent = DmIsParentURI(szPath,pPath->GetPath());
        if ( !bIsParent &&  !DmIsParentURI(pPath->GetPath(),szPath) )
        {
            continue;
        }     

        if ( bIsParent )
        {
            DmtEventDataVector & aVector = aEventMap.get_value(nPos);
            CleanEvents(aVector);
            if ( !aVector.size() )
                aEventMap.remove(pPath); 
        }    
    }  
    return SYNCML_DM_SUCCESS;

}



SYNCML_DM_RET_STATUS_T 
DMEventLogger::CleanConfigEvents(CPCHAR szPath)
{

    for ( DMPostEventMap::POS nPos = m_aPostEvents.end()-1; nPos >= 0; nPos-- )
    {
        const PDMConfigItem & pConfigItem = m_aPostEvents.get_key(nPos); 

        if ( !DmIsParentURI(szPath,pConfigItem->GetPath()) && 
             !DmIsParentURI(pConfigItem->GetPath(),szPath) )
        {
            continue;
        }    

        DMParser oStored(pConfigItem->GetPath());
        DMParser oRemoved(szPath);

        if ( oRemoved.getSegmentsCount() < oStored.getSegmentsCount() )
        {
            m_aPostEvents.remove(pConfigItem);
            continue;
        }
            
        DMEventMap & aEventMap = m_aPostEvents.get_value(nPos); 
        CleanEvents(aEventMap,szPath);  
        if ( !aEventMap.size() )
            m_aPostEvents.remove(pConfigItem);
    }  

    return SYNCML_DM_SUCCESS;

}

SYNCML_DM_RET_STATUS_T 
DMEventLogger::CleanPluginEvents(CPCHAR szPath)
{

    for ( DMPostEventMap::POS nPos = m_aPluginEvents.end()-1; nPos >= 0; nPos-- )
    {
        const PDMPlugin & pPlugin = m_aPluginEvents.get_key(nPos); 

         if ( !DmIsParentURI(szPath,pPlugin->GetPath()) && 
             !DmIsParentURI(pPlugin->GetPath(),szPath) )
        {
            continue;
        }    

        DMParser oStored(pPlugin->GetPath());
        DMParser oRemoved(szPath);

        if ( oRemoved.getSegmentsCount() < oStored.getSegmentsCount() )
        {
            m_aPluginEvents.remove(pPlugin);
            continue;
        }
        
        DMEventMap & aEventMap = m_aPluginEvents.get_value(nPos); 
        CleanEvents(aEventMap,szPath);  
        if ( !aEventMap.size() )
            m_aPluginEvents.remove(pPlugin);
    }  
    return SYNCML_DM_SUCCESS;

}


SYNCML_DM_RET_STATUS_T 
DMEventLogger::CleanEvents(CPCHAR szPath)
{
    XPL_LOG_DM_TMN_Debug(("Clean events for %s\n", szPath));
    CleanConfigEvents(szPath);
    CleanPluginEvents(szPath);
    return SYNCML_DM_SUCCESS;

}



SYNCML_DM_RET_STATUS_T 
DMEventLogger::UpdateEvents(DMEventMap & aEventMap, 
                            CPCHAR szPath, 
                            CPCHAR szNewName)
{

    for ( DMEventMap::POS nPos = aEventMap.end()-1; nPos >= 0; nPos-- )
    {
        PDMEventPath & pPath = (PDMEventPath &)aEventMap.get_key(nPos);
     
        if ( !DmIsParentURI(szPath,pPath->GetPath()) )
            continue;

        DMBuffer oNewPath;
        if ( !oNewPath.allocate((pPath->GetPath()).length() + DmStrlen(szNewName)) )
            return SYNCML_DM_DEVICE_FULL;

        DMURI oPath(FALSE,szPath);
        CPCHAR pParent = oPath.getParentURI();

        CPCHAR pStoredPath = (CPCHAR)(pPath->GetPath());
        DMURI oStoredTail(FALSE, pStoredPath + DmStrlen(szPath) + 1);

        oStoredTail.nextSegment();
        CPCHAR pTailSegments = oStoredTail.getTailSegments();
        

        oNewPath.assign(pParent);
        oNewPath.append((UINT8*)"/",1);
        oNewPath.append((UINT8*)szNewName,DmStrlen(szNewName));
        
        if ( pTailSegments )
        {
            oNewPath.append((UINT8*)"/",1);
            oNewPath.append((UINT8*)pTailSegments,DmStrlen(pTailSegments));     
        }    

        pPath = PDMEventPath(new DMEventPath((CPCHAR)oNewPath.getBuffer()));

        if ( pPath  == NULL )
            return SYNCML_DM_DEVICE_FULL;
    }  
    return SYNCML_DM_SUCCESS;

}




SYNCML_DM_RET_STATUS_T 
DMEventLogger::UpdateSubscriptionEvents(CPCHAR szPath,
                                        CPCHAR szNewName)
{

    for ( DMPostEventMap::POS nPos = m_aPostEvents.end()-1; nPos >= 0; nPos-- )
    {
        const PDMConfigItem & pConfigItem = m_aPostEvents.get_key(nPos); 

        if ( !DmIsParentURI(szPath,pConfigItem->GetPath()) && 
             !DmIsParentURI(pConfigItem->GetPath(),szPath) )
        {
            continue;
        }    

        DMEventMap & aEventMap = m_aPostEvents.get_value(nPos); 
        UpdateEvents(aEventMap,szPath,szNewName);  
    }  

    return SYNCML_DM_SUCCESS;

}


SYNCML_DM_RET_STATUS_T 
DMEventLogger::UpdatePluginEvents(CPCHAR szPath, 
                                  CPCHAR szNewName)
{

    for ( DMPluginEventMap::POS nPos = m_aPluginEvents.end()-1; nPos >= 0; nPos-- )
    {
        const PDMPlugin & pPlugin = m_aPluginEvents.get_key(nPos); 

        if ( !DmIsParentURI(szPath,pPlugin->GetPath()) && 
             !DmIsParentURI(pPlugin->GetPath(),szPath) )
        {
            continue;
        }    

        DMEventMap & aEventMap = m_aPluginEvents.get_value(nPos); 
        UpdateEvents(aEventMap,szPath,szNewName);  
    }  

    return SYNCML_DM_SUCCESS;

}


SYNCML_DM_RET_STATUS_T 
DMEventLogger::UpdateEvents(CPCHAR szPath, 
                            CPCHAR szNewName)
{
    XPL_LOG_DM_TMN_Debug(("Update events for %s\n", szPath));
    UpdateSubscriptionEvents(szPath,szNewName);
    UpdatePluginEvents(szPath,szNewName);
    return SYNCML_DM_SUCCESS;

}



BOOLEAN
DMEventLogger::FindRecordForAdd(const DmtEventDataVector & aVector,
                                    CPCHAR szName)
{

    for (INT32 index=aVector.size()-1; index>=0; index--)
    {
        const PDmtEventData & pEventData = aVector[index];
        
        if ( !pEventData->IsLeaf() )
            continue;  // we need only leaf nodes 
            
        if ( pEventData->GetName() == szName ) // No Rename on leaf nodes, so compare only name
        {
            if (pEventData->GetAction() == SYNCML_DM_EVENT_DELETE) // node was deleted
            {
                ((DMEventData*)pEventData.GetPtr())->SetAction(SYNCML_DM_EVENT_REPLACE); // delete+add on leaf node = replace
                return TRUE; 
            }    
            break;
        } 
    }
    return FALSE;
}


BOOLEAN 
DMEventLogger::FindRecordForDelete(DmtEventDataVector & aVector, 
                                   CPCHAR szName) 
{
    BOOLEAN bIsIgnore = FALSE;
    for (INT32 index=aVector.size()-1; index>=0; index--)
    {
         const PDmtEventData & pEventData = aVector[index];
        if ( pEventData->GetName() == szName || pEventData->GetNewName() == szName)
        {
             if ( (pEventData->GetAction() & SYNCML_DM_EVENT_ADD) == SYNCML_DM_EVENT_ADD ) 
            {
                bIsIgnore = TRUE;  
                aVector.remove(index);
            }    

            return bIsIgnore;
        }
    }

    return bIsIgnore;

}


BOOLEAN
DMEventLogger::FindRecordForReplace(const DmtEventDataVector & aVector,
                                    CPCHAR szName)
{

    for (INT32 index=aVector.size()-1; index>=0; index--)
    {
        const PDmtEventData & pEventData = aVector[index];
        if ( pEventData->GetName() == szName ) // Replace is only on leaf nodes , no need to check new name
        {
            if ((pEventData->GetAction() & SYNCML_DM_EVENT_REPLACE) == SYNCML_DM_EVENT_REPLACE)
                return TRUE; 

            if ((pEventData->GetAction() & SYNCML_DM_EVENT_ADD) == SYNCML_DM_EVENT_ADD)
                return TRUE; 

            break;
        } 
    }
    return FALSE;
}


BOOLEAN
DMEventLogger::FindRecordForRename(DmtEventDataVector & aVector,
                                   CPCHAR szName,
                                   CPCHAR szNewName)
{
    BOOLEAN bIsIgnore = FALSE;
    
    for (INT32 index=aVector.size()-1; index>=0; index--)
    {
        PDmtEventData & pEventData = aVector[index];
        if (  ((pEventData->GetAction() & SYNCML_DM_EVENT_ADD) == SYNCML_DM_EVENT_ADD) &&
               pEventData->GetName() == szName )
        {
            ((DMEventData*)pEventData.GetPtr())->SetName(szNewName);
            bIsIgnore = TRUE;
            break;
        }    
 
        if (  ((pEventData->GetAction() & SYNCML_DM_EVENT_RENAME) == SYNCML_DM_EVENT_RENAME) &&
               pEventData->GetNewName() == szName )
        {
            ((DMEventData*)pEventData.GetPtr())->SetNewName(szNewName);
            bIsIgnore = TRUE;
            break;
        }    
    }
    return bIsIgnore;
}



BOOLEAN 
DMEventLogger::CheckEventOnSameNode(DmtEventDataVector & aVector,
                                    SYNCML_DM_EVENT_ACTION_T nAction, 
                                    CPCHAR szName,
                                    BOOLEAN bIsCumulative,
                                    CPCHAR szNewName)
{


    if ( bIsCumulative )
    {
        if ( nAction == SYNCML_DM_EVENT_RENAME )
            return FindRecordForRename(aVector,szName,szNewName); 
    }
    else
    {
        switch ( nAction )
        {    
            case SYNCML_DM_EVENT_ADD:
                return FindRecordForAdd(aVector,szName);
      
            case SYNCML_DM_EVENT_REPLACE:
                return FindRecordForReplace(aVector,szName);
      
            case SYNCML_DM_EVENT_DELETE:
                return FindRecordForDelete(aVector, szName); 

           case  SYNCML_DM_EVENT_RENAME: 
                return FindRecordForRename(aVector,szName,szNewName); 
         }
     }    
            
     return  FALSE;

}




BOOLEAN 
DMEventLogger::IsIgnoreEvent(CPCHAR szPath ,
                             SYNCML_DM_EVENT_ACTION_T nAction, 
                             DMEventMap & aEventMap,
                             BOOLEAN bIsCumulative,
                             CPCHAR szNewName )
{

    
    DMURI oURI(FALSE, szPath);

    CPCHAR pParent = oURI.getParentURI();
    CPCHAR pNode = oURI.getTailSegments();

    DmtEventDataVector aVector;
    PDMEventPath pEventPath;
    BOOLEAN bIsFound = Find(pParent,aEventMap, pEventPath,aVector);

    if ( !bIsFound )
        return FALSE;

    BOOLEAN bIsIgnore = CheckEventOnSameNode(aVector,
                                             nAction,
                                             pNode,
                                             bIsCumulative,
                                             szNewName); 
    if ( !aVector.size() )
        aEventMap.remove(pEventPath);
    else
        aEventMap.put(pEventPath,aVector);

    return bIsIgnore;
  
}



BOOLEAN 
DMEventLogger::IsIgnoreSubscriptionEvent(CPCHAR szPath,
                                         SYNCML_DM_EVENT_ACTION_T nAction, 
                                         CPCHAR szNewName)
{

    BOOLEAN bIsIgnore = TRUE;
    BOOLEAN bIsCumulative = FALSE;

    PDMConfigItem pFoundItem; 
    DMEventMap aEventMap;
    UINT32 nSegmentCount = 0;

    for ( DMPostEventMap::POS nPos = m_aPostEvents.end()-1; nPos >= 0; nPos--  )
    {
        const PDMConfigItem & pConfigItem = m_aPostEvents.get_key(nPos); 

        if (  !DmIsParentURI(pConfigItem->GetPath(),szPath) )
            continue;

        DMParser oParser(szPath);
        if ( nSegmentCount < oParser.getSegmentsCount() )
        {
            nSegmentCount = oParser.getSegmentsCount();
            pFoundItem = pConfigItem;
            aEventMap = m_aPostEvents.get_value(nPos); 
        }
    }    

    if ( pFoundItem == NULL )
       return FALSE;

    if ( ((DMSubscriptionItem*)( pFoundItem.GetPtr()))->GetType() == SYNCML_DM_EVENT_CUMULATIVE )
        bIsCumulative = TRUE;
  
    bIsIgnore = IsIgnoreEvent(szPath,
                              nAction,
                              aEventMap,
                              bIsCumulative,
                              szNewName);
   
    if ( !aEventMap.size() )
        m_aPostEvents.remove( pFoundItem);
    else
        m_aPostEvents.put(pFoundItem,aEventMap);
   
    return bIsIgnore;
}





BOOLEAN 
DMEventLogger::IsIgnorePluginEvent(CPCHAR szPath,
                                   SYNCML_DM_EVENT_ACTION_T nAction, 
                                   CPCHAR szNewName)
{

    BOOLEAN bIsIgnore = TRUE;
    BOOLEAN bIsCumulative = FALSE;

    PDMPlugin pFoundPlugin; 
    DMEventMap aEventMap;
    UINT32 nSegmentCount = 0;

    for ( DMPluginEventMap::POS nPos = m_aPluginEvents.end()-1; nPos >= 0; nPos-- )
    {
        const PDMPlugin & pPlugin = m_aPluginEvents.get_key(nPos); 

        if (  !DmIsParentURI(pPlugin->GetPath(),szPath) )
            continue;

        DMParser oParser(szPath);
        if ( nSegmentCount < oParser.getSegmentsCount() )
        {
            nSegmentCount = oParser.getSegmentsCount();
            pFoundPlugin = pPlugin;
            aEventMap = m_aPluginEvents.get_value(nPos); 
        }
    }    


    if ( pFoundPlugin == NULL )
        return FALSE;
    
    if ( pFoundPlugin->GetType() == SYNCML_DM_EVENT_CUMULATIVE )
         bIsCumulative = TRUE;
  
    bIsIgnore = IsIgnoreEvent(szPath,
                              nAction,
                              aEventMap,
                              bIsCumulative, 
                              szNewName);
   
    if ( !aEventMap.size() )
        m_aPluginEvents.remove(pFoundPlugin);
    else
        m_aPluginEvents.put(pFoundPlugin,aEventMap);
   
    return bIsIgnore;
}
