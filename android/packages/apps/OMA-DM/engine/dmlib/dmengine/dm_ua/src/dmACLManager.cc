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

    Source Name: dmACLManager.cc

    General Description: Implementation of the DMAclManager class

==================================================================================================*/

#include "dm_uri_utils.h"
#include "dm_tree_class.H"
#include "xpl_Logger.h"
#include "dmACLManager.h"

SYNCML_DM_RET_STATUS_T 
DMAclManager::Init(CEnv* env, 
                          DMTree* tree) 
{
    return DMConfigManager::Init(env,tree,SYNCML_DM_FILE_ACL);
}

void DMAclManager::GetFileName(DMString & strFileName)
{
     m_pEnv->GetWFSFullPath(DM_DEFAULT_ACL_FILENAME,strFileName);
}      


DMConfigItem * DMAclManager::AllocateConfigItem()
{
    return (new DMAclItem());

}




BOOLEAN 
DMAclManager::IsPermitted( CPCHAR szPath,
                                          CPCHAR szServerID, 
                                          SYNCML_DM_ACL_PERMISSIONS_T nPermissions ) const
{
    DMURI oURI(FALSE,szPath);
    CPCHAR pParent = szPath;

    if ( !m_aConfig.size() ) 
       return TRUE;
    
    do  
    {
        PDMConfigItem pItem;
        if ( Find(pParent,pItem) != -1 ) 
        {
            if ( ((DMAclItem*)(pItem.GetPtr()))->IsPermitted( "*", nPermissions ) ||
                  ((DMAclItem*)(pItem.GetPtr()))->IsPermitted( szServerID, nPermissions ) )
                return TRUE;
            XPL_LOG_DM_TMN_Debug(("DMAclManager::IsPermitted(,,): check permission failed... \n"));
            return FALSE;
        }
    } while ( (pParent = oURI.getParentURI()) != NULL ) ;
  
  XPL_LOG_DM_TMN_Debug(("DMAclManager::IsPermitted(,,): check . node, permission not allowed \n"));
  return FALSE; // not . node
}

BOOLEAN 
DMAclManager::IsPermitted( CPCHAR szPath, 
                                      CPCHAR szServerID, 
                                      SYNCML_DM_ACL_PERMISSIONS_T nPermissions,
                                      BOOLEAN bIsCheckLocal ) 
{

    DMMetaDataManager & m_oMDFObj =  m_pTree->GetMetaDataManager();
  
    if ( bIsCheckLocal && m_oMDFObj.IsLocal(szPath) )
    {
         XPL_LOG_DM_TMN_Debug(("DMAclManager::IsPermitted(,,,): check local but path is not local\n")); 
         return FALSE;
    }

    // Deserialize() make expensive XPL_FS_GetModTime call
    // we just trying to minimize them
    if (!m_bLoaded)
		Deserialize();

    if ( IsPermitted(szPath, szServerID, nPermissions) == FALSE )
    {  
         XPL_LOG_DM_TMN_Debug(("DMAclManager::IsPermitted(,,,): permission for node %s is not granted to the server %s\n", szPath,szServerID));
         return FALSE;
    }
        
    DMString szMDF;
    m_oMDFObj.GetPath(szPath, szMDF);
    
    if (szMDF != szPath )
    {
        XPL_LOG_DM_TMN_Debug(("DMAclManager::IsPermitted(,,,): permission mdf check, szMDF:%s, szPath:%s",szMDF.c_str(), szPath)); 
        return IsPermitted(szMDF, szServerID, nPermissions);
    }
  
    return TRUE; 
}


SYNCML_DM_RET_STATUS_T DMAclManager::SetACL( CPCHAR szPath, CPCHAR szACL )
{

    CheckLocking();
    Delete(szPath);
 
    DMAclItem *pItem = new DMAclItem(szPath, szACL);
    if ( pItem == NULL )
        return SYNCML_DM_DEVICE_FULL;  
    
    m_aConfig.push_back(PDMConfigItem(pItem));
    m_bChanged = true;
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T
DMAclManager::GetACL(CPCHAR szPath, 
                                     DMString& strACL) 
{
    strACL = NULL;

    PDMConfigItem pItem;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    // Deserialize() make expensive XPL_FS_GetModTime call
    // we just trying to minimize them
    if (!m_bLoaded)
    	Deserialize();
      
    dm_stat = Get(szPath,pItem);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
   
    strACL = ((DMAclItem*)(pItem.GetPtr()))->toString();
    return SYNCML_DM_SUCCESS;
}
