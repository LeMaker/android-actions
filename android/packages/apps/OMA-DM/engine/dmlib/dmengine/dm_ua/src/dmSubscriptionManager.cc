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

    Source Name: dmSubscriptionManager.cc

    General Description: Implementation of the DMSubscriptionManager class

==================================================================================================*/

#include "dm_tree_class.H"
#include "dmSubscriptionManager.h"
#include "dm_uri_utils.h"
#include "xpl_Logger.h"

SYNCML_DM_RET_STATUS_T 
DMSubscriptionManager::Init( CEnv* env, DMTree* tree )
{
    return DMConfigManager::Init(env,tree,SYNCML_DM_FILE_EVENT);
}


SYNCML_DM_RET_STATUS_T
DMSubscriptionManager::UpdateEvents(CPCHAR szPath,
                                    SYNCML_DM_EVENT_ACTION_T nAction,
                                    CPCHAR szNewName)
{
    SYNCML_DM_RET_STATUS_T  dm_stat;
    SyncML_DM_Archiver & m_oArchiver = m_pTree->GetArchiver();
    
    if ( nAction == SYNCML_DM_EVENT_RENAME )
    {
        dm_stat = m_oArchiver.UpdateEvents(szPath,szNewName);
        if ( dm_stat != SYNCML_DM_SUCCESS )
           return dm_stat;
    }    
    return SYNCML_DM_SUCCESS;
    
}    


SYNCML_DM_RET_STATUS_T
DMSubscriptionManager::CleanEvents(CPCHAR szPath)
{
    SyncML_DM_Archiver & m_oArchiver = m_pTree->GetArchiver();
    
    return m_oArchiver.CleanEvents(szPath);
    
}    

CPCHAR
DMSubscriptionManager::ResetName(BOOLEAN bIsEnabledByParent,
                                 DMEventSubscription * pItem, 
                                 CPCHAR szNewName)
{
    if ( bIsEnabledByParent && pItem->GetType() == SYNCML_DM_EVENT_CUMULATIVE )
        return NULL;
    else
        return szNewName;
}         




SYNCML_DM_RET_STATUS_T 
DMSubscriptionManager::ProcessUpdateForSubscription(SyncML_DM_Archive * pArchive,
                                                    CPCHAR szPath,
                                                    SYNCML_DM_EVENT_ACTION_T nAction,
                                                    CPCHAR szNewName)
{


    if ( pArchive == NULL )
        return SYNCML_DM_FAIL;

    DMString strTrackedPath = szPath; 
    
    PDMConfigItem pItem; 
    CPCHAR pNewName = NULL;
    BOOLEAN bIsEnabledByParent = FALSE;

    
    if ( IsEnabled( strTrackedPath,nAction,pItem, bIsEnabledByParent,FALSE) )
    {
          pNewName =  ResetName(bIsEnabledByParent,(DMSubscriptionItem*)(pItem.GetPtr()),szNewName);
          pArchive->GetEventLogger().OnNodeChanged( strTrackedPath, 
                                                    nAction,
                                                    pItem,
                                                    bIsEnabledByParent,
                                                    pNewName);
    }      

    return SYNCML_DM_SUCCESS;

}    



SYNCML_DM_RET_STATUS_T 
DMSubscriptionManager::ProcessUpdateForPlugin(SyncML_DM_Archive * pArchive,
                                              CPCHAR szPath,
                                              SYNCML_DM_EVENT_ACTION_T nAction,
                                              CPCHAR szNewName)
{

    if ( pArchive == NULL )
        return SYNCML_DM_FAIL;

    DMString strTrackedPath = szPath; 
    
    PDMPlugin pPlugin;
    CPCHAR pNewName = NULL;
    BOOLEAN bIsEnabledByParent = FALSE;
    
   
    if ( !IsEnabled(strTrackedPath,nAction,pPlugin, bIsEnabledByParent,FALSE) )
         return SYNCML_DM_SUCCESS;

    pNewName =  ResetName(bIsEnabledByParent,(DMSubscriptionItem*)(pPlugin.GetPtr()),szNewName);
    pArchive->GetEventLogger().OnNodeChanged(strTrackedPath, 
                                             nAction, 
                                             pPlugin,
                                             bIsEnabledByParent,
                                             pNewName);
    return SYNCML_DM_SUCCESS;

}    


SYNCML_DM_RET_STATUS_T 
DMSubscriptionManager::OnNodeChanged(SyncML_DM_Archive * pArchive,
                                     CPCHAR szPath,
                                     SYNCML_DM_EVENT_ACTION_T nAction,
                                     CPCHAR szNewName)
{

    Deserialize();

    
    XPL_LOG_DM_TMN_Debug(("Received update notification for path= %s\n", szPath));
        
    UpdateEvents(szPath,nAction,szNewName);
    if ( !pArchive->GetEventLogger().IsIgnoreSubscriptionEvent(szPath, nAction,szNewName) )
        ProcessUpdateForSubscription(pArchive,szPath,nAction,szNewName);
    if ( !pArchive->GetEventLogger().IsIgnorePluginEvent(szPath, nAction,szNewName) )
        ProcessUpdateForPlugin(pArchive,szPath,nAction,szNewName);
    return SYNCML_DM_SUCCESS;

}    



SYNCML_DM_RET_STATUS_T 
DMSubscriptionManager::ProcessDeleteForSubscription(SyncML_DM_Archive * pArchive,
                                                    CPCHAR szPath,
                                                    BOOLEAN bIsChild )
{
    if ( pArchive == NULL )
        return SYNCML_DM_FAIL;

    DMString strTrackedPath = szPath; 
    PDMConfigItem pItem;
    BOOLEAN bIsEnabledByParent = FALSE;

      
    if ( IsEnabled(strTrackedPath,SYNCML_DM_EVENT_DELETE,pItem, bIsEnabledByParent,bIsChild) )
    {
         pArchive->GetEventLogger().OnNodeChanged(strTrackedPath, 
                                                  SYNCML_DM_EVENT_DELETE, 
                                                  pItem,
                                                  bIsEnabledByParent);
    }     

    return SYNCML_DM_SUCCESS;
}



SYNCML_DM_RET_STATUS_T 
DMSubscriptionManager::ProcessDeleteForPlugin(SyncML_DM_Archive * pArchive,
                                              CPCHAR szPath,
                                              BOOLEAN bIsChild)
{
    if ( pArchive == NULL )
        return SYNCML_DM_FAIL;

    DMString strTrackedPath = szPath; 
    PDMPlugin pPlugin;
    BOOLEAN bIsEnabledByParent = FALSE;
   
    if ( IsEnabled(strTrackedPath,SYNCML_DM_EVENT_DELETE,pPlugin,bIsEnabledByParent,bIsChild) )
    {
        pArchive->GetEventLogger().OnNodeChanged(strTrackedPath, 
                                                 SYNCML_DM_EVENT_DELETE,
                                                 pPlugin,
                                                 bIsEnabledByParent);
    }    
 
    return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T 
DMSubscriptionManager::OnNodeDeleted(SyncML_DM_Archive * pArchive,
                                     CPCHAR szPath,
                                     const DMStringVector & aDeletedChildren )
{

    Deserialize();

    XPL_LOG_DM_TMN_Debug(("Received delete notification for path= %s\n", szPath));

   
    CleanEvents(szPath);
    if ( !pArchive->GetEventLogger().IsIgnoreSubscriptionEvent(szPath,SYNCML_DM_EVENT_DELETE) )
        ProcessDeleteForSubscription(pArchive,szPath,FALSE);  
    if ( !pArchive->GetEventLogger().IsIgnorePluginEvent(szPath,SYNCML_DM_EVENT_DELETE) )
        ProcessDeleteForPlugin(pArchive,szPath,FALSE);  

    for (INT32 index=0; index<aDeletedChildren.size(); index++)
    {
         CPCHAR szChildPath = aDeletedChildren[index].c_str();
         if ( !pArchive->GetEventLogger().IsIgnoreSubscriptionEvent(szChildPath,SYNCML_DM_EVENT_DELETE) )
            ProcessDeleteForSubscription(pArchive,szChildPath,TRUE);  
         if ( !pArchive->GetEventLogger().IsIgnorePluginEvent(szChildPath,SYNCML_DM_EVENT_DELETE) )
            ProcessDeleteForPlugin(pArchive,szChildPath,TRUE);  
    }    
 
    return SYNCML_DM_SUCCESS;
}




BOOLEAN 
DMSubscriptionManager::IsParentEnabled(DMString & strPath, 
                                       CPCHAR szParent,
                                       SYNCML_DM_EVENT_ACTION_T nAction,
                                       DMEventSubscription * pItem) const 
{

    if ( pItem->IsEnabled( nAction, m_pTree ) )
    {
        if ( pItem->GetType() == SYNCML_DM_EVENT_CUMULATIVE )
        {
            strPath = szParent;
            return TRUE;
        }
        else
            if ( pItem->GetType() == SYNCML_DM_EVENT_DETAIL )
                return TRUE;
   } 
   return FALSE;
}



BOOLEAN 
DMSubscriptionManager::IsParentEnabledBySubscription(DMString & strPath, 
                                                     CPCHAR szParent,
                                                     SYNCML_DM_EVENT_ACTION_T nAction,
                                                     PDMConfigItem & pItem) const 
{

    INT32 index = Find(szParent,pItem);
    if ( index == -1 )
        return FALSE;

    DMSubscriptionItem * pSubscriptionItem = (DMSubscriptionItem*)(pItem.GetPtr());

    return IsParentEnabled(strPath,
                           szParent,
                           nAction,
                           (DMEventSubscription*)pSubscriptionItem);
}



BOOLEAN 
DMSubscriptionManager::IsParentEnabledByPlugin(DMString & strPath, 
                                             CPCHAR szParent,
                                             SYNCML_DM_EVENT_ACTION_T nAction,
                                             PDMPlugin & pPlugin) const 
{
    DMPluginManager & oPluginManager = m_pTree->GetPluginManager();

    pPlugin = oPluginManager.FindCommitPlugin(szParent);
    if ( pPlugin == NULL )
        return FALSE;

   return IsParentEnabled(strPath,
                          szParent,
                          nAction,
                          (DMEventSubscription*)(pPlugin.GetPtr()));
}



BOOLEAN 
DMSubscriptionManager::IsEnabledBySubscription(DMString & strPath, 
                                            SYNCML_DM_EVENT_ACTION_T nAction,
                                            PDMConfigItem & pItem,
                                            BOOLEAN & bIsEnabledByParent,
                                            BOOLEAN bIsChild) const 
{

    bIsEnabledByParent = FALSE;
    INT32 index = Find(strPath,pItem);
    if ( index != -1 )
    {
          if ( ((DMSubscriptionItem*)(pItem.GetPtr()))->IsEnabled( nAction,m_pTree) )
              return TRUE;
    }      

    if ( bIsChild )
        return FALSE;

    DMURI oURI(TRUE,strPath.GetBuffer());
    CPCHAR pParent = NULL;
    bIsEnabledByParent = TRUE;
        
    while ( (pParent = oURI.getParentURI()) != NULL ) 
    {
         if ( IsParentEnabledBySubscription(strPath,pParent,nAction, pItem) )
              return TRUE;
    }    

    return FALSE; 
}





BOOLEAN 
DMSubscriptionManager::IsEnabledByPlugin(DMString & strPath, 
                                        SYNCML_DM_EVENT_ACTION_T nAction,
                                        PDMPlugin & pPlugin,
                                        BOOLEAN & bIsEnabledByParent,
                                        BOOLEAN bIsChild) const 
{

    DMPluginManager & oPluginManager = m_pTree->GetPluginManager();

    bIsEnabledByParent = FALSE; 
    pPlugin = oPluginManager.FindCommitPlugin(strPath);
    if ( pPlugin != NULL )
    {
          if ( pPlugin->IsEnabled( nAction,m_pTree) )
               return TRUE;
    }      

    if ( bIsChild )
        return FALSE;

    DMURI oURI(TRUE,strPath.GetBuffer());
    CPCHAR pParent = NULL;
    bIsEnabledByParent = TRUE;
        
    while ( (pParent = oURI.getParentURI()) != NULL ) 
    {
        if ( IsParentEnabledByPlugin(strPath,pParent,nAction, pPlugin) )
            return TRUE;
    }    

    return FALSE; 
}


SYNCML_DM_RET_STATUS_T 
DMSubscriptionManager::GetLogPath(DMString & strPath,
                                  CPCHAR szMDF) const 
{

    UINT32 nSegmentCount = 0;
    {
        DMParser oParser(szMDF);
        nSegmentCount = oParser.getSegmentsCount();
    }    
    char * pEnd = NULL;
    
    {
         DMParser oParser(strPath); 
         if (  nSegmentCount == oParser.getSegmentsCount() )
            return SYNCML_DM_SUCCESS;
         
         for (INT32 count=0; count<=(INT32)nSegmentCount; count++)
            pEnd = (char*)oParser.nextSegment();
    }
    
    if ( pEnd != NULL )
    {
        pEnd--;   
        *pEnd = '\0'; // pEnd can't be NULL
    }    
    return SYNCML_DM_SUCCESS;

}    




BOOLEAN 
DMSubscriptionManager:: IsEnabled(DMString & strPath, 
                                  SYNCML_DM_EVENT_ACTION_T nAction,
                                  PDMConfigItem & pItem,
                                  BOOLEAN & bIsEnabledByParent,
                                  BOOLEAN bIsChild) const 
{

    BOOLEAN bIsEnabled = FALSE;

    if ( !m_aConfig.size() )
        return FALSE;
    
    bIsEnabled = IsEnabledBySubscription(strPath, 
                                         nAction, 
                                         pItem, 
                                         bIsEnabledByParent, 
                                         bIsChild); 
    if ( bIsEnabled )
        return TRUE;

    DMString strMDF;
    SYNCML_DM_RET_STATUS_T ret_status;
    DMMetaDataManager & m_oMDFObj =  m_pTree->GetMetaDataManager();
    ret_status = m_oMDFObj.GetPath(strPath, strMDF);

    if ( ret_status == SYNCML_DM_SUCCESS &&  strMDF != strPath)
    {
        bIsEnabled = IsEnabledBySubscription(strMDF, 
                                             nAction, 
                                             pItem, 
                                             bIsEnabledByParent,  
                                             bIsChild);
           
        if ( bIsEnabled && bIsChild )
        {
            if ( ((DMSubscriptionItem*)(pItem.GetPtr()))->GetType() == SYNCML_DM_EVENT_CUMULATIVE )
                GetLogPath(strPath,(CPCHAR)strMDF);
            
            return TRUE; 
        }    
    }

    return bIsEnabled; 
    
}



BOOLEAN 
DMSubscriptionManager:: IsEnabled(DMString & strPath, 
                                  SYNCML_DM_EVENT_ACTION_T nAction,
                                  PDMPlugin & pPlugin,
                                  BOOLEAN & bIsEnabledByParent,
                                  BOOLEAN bIsChild) const 
{

    BOOLEAN bIsEnabled = FALSE;
    
    bIsEnabled = IsEnabledByPlugin(strPath, 
                                   nAction, 
                                   pPlugin, 
                                   bIsEnabledByParent,
                                   bIsChild); 

    if ( bIsEnabled )
        return TRUE;
   
    DMString strMDF;
    SYNCML_DM_RET_STATUS_T ret_status;
    DMMetaDataManager & m_oMDFObj =  m_pTree->GetMetaDataManager();
    ret_status = m_oMDFObj.GetPath(strPath, strMDF);

    if ( ret_status == SYNCML_DM_SUCCESS &&  strMDF != strPath)
    {
        bIsEnabled = IsEnabledByPlugin(strMDF, 
                                       nAction, 
                                       pPlugin, 
                                       bIsEnabledByParent, 
                                       bIsChild);
           
        if ( bIsEnabled && bIsChild == FALSE )
        {
            if ( (pPlugin.GetPtr())->GetType() == SYNCML_DM_EVENT_CUMULATIVE )
                GetLogPath(strPath,(CPCHAR)strMDF);
            return TRUE; 
        }    
    }

    return bIsEnabled; 
    
}




SYNCML_DM_RET_STATUS_T
DMSubscriptionManager::EnableEvent(CPCHAR szPath, 
                                   const DmtEventSubscription & oEvent)
{

    CheckLocking();

    DMString szMDF;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    DMMetaDataManager & m_oMDFObj =  m_pTree->GetMetaDataManager();
    dm_stat = m_oMDFObj.GetPath(szPath, szMDF);
    if ( dm_stat != SYNCML_DM_SUCCESS )
    { 
        XPL_LOG_DM_TMN_Error(("MDF path isn't found, path = %s\n", szPath));
        return dm_stat; 
    }    

    Delete(szPath);
 
    DMSubscriptionItem * pEventItem = new DMSubscriptionItem(szPath);
    if ( pEventItem == NULL )
    {
        XPL_LOG_DM_TMN_Error(("Not enough memory\n"));
        return SYNCML_DM_DEVICE_FULL;
    }    

    dm_stat = pEventItem->Set(oEvent);
    if ( dm_stat != SYNCML_DM_SUCCESS ) 
    {
        XPL_LOG_DM_TMN_Error(("Event data can't be set for path = %s\n", szPath));
        delete pEventItem;
        return dm_stat;
    }
  
    m_aConfig.push_back(PDMConfigItem((DMConfigItem*)pEventItem));
    m_bChanged = true;
    return dm_stat;
}


void DMSubscriptionManager::GetFileName(DMString & strFileName)
{
    m_pEnv->GetWFSFullPath(DM_DEFAULT_EVENT_FILENAME,strFileName);
}    

DMConfigItem * DMSubscriptionManager::AllocateConfigItem()
{
    return new DMSubscriptionItem();
}


SYNCML_DM_RET_STATUS_T DMSubscriptionManager::GetEvent(CPCHAR szPath, 
        DmtEventSubscription & oEvent)
{
    PDMConfigItem pItem;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    Deserialize();
     
    dm_stat = Get(szPath,pItem);

    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    DMSubscriptionItem * ptr = ((DMSubscriptionItem*)(pItem.GetPtr()));

    return oEvent.Set(ptr->GetAction(),
                       ptr->GetType(),
                       ptr->GetTopic(),
                       ptr->GetPrincipals(TRUE),
                       ptr->GetPrincipals(FALSE));
}
