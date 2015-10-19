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

#include "dm_tree_class.H"
#include "dm_uri_utils.h"
#include "SyncML_DM_Archive.H"
#include "xpl_Logger.h"
#include "dm_uri_utils.h"

/*==================================================================================================
 
Function:    SyncML_DM_Archive::SyncML_DM_Archive
 
Description: The SyncML_DM_Archive class constructor.
 
==================================================================================================*/
SyncML_DM_Archive::SyncML_DM_Archive(CEnv* env, CPCHAR pURI, CPCHAR path)
  : m_pURI(pURI),
  m_path(path)
{
  rootTreeNode=NULL;
  parent=NULL;
  dirty = FALSE;
  m_bWritableExist = FALSE;
  m_permission = -1;

  env->GetWFSFullPath(path,m_strWFSFileName);
}

/*==================================================================================================
 
Function:    SyncML_DM_Archive::~SyncML_DM_Archive
 
Description: The SyncML_DM_Archive class deconstructor.
 
==================================================================================================*/
SyncML_DM_Archive::~SyncML_DM_Archive() 
{
    if ( rootTreeNode )
        rootTreeNode->pArchive = NULL;
}

void 
SyncML_DM_Archive::getFilePath(char * path, CPCHAR ext) 
{
    DmStrcpy(path, m_strWFSFileName);
    DmStrcat(path, ext);
}

BOOLEAN 
SyncML_DM_Archive::LoadSkeleton(DMTree* pTree) 
{
    if ( rootTreeNode ) 
        return TRUE;
  
    rootTreeNode = pTree->CreateSkeletonNode( getURI() );
    
    if ( rootTreeNode )
        rootTreeNode->pArchive = this;
    else 
        return FALSE;
  
    return TRUE;
}


SYNCML_DM_RET_STATUS_T 
SyncML_DM_Archive::Init(DMTree* pTree)
{
    if ( !pTree ) 
        return SYNCML_DM_FAIL;

    m_pTree = pTree;
    return oEventLogger.Init(pTree);

}

void 
SyncML_DM_Archive::setLastAccessedTime(XPL_CLK_CLOCK_T lastAccessedTime)
{
    m_lastAccessedTime = lastAccessedTime;
}

XPL_CLK_CLOCK_T
SyncML_DM_Archive::getLastAccessedTime()
{
    return m_lastAccessedTime;
}
