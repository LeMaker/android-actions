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

    Name: dmtTree.cc

    General Description: Implementation of the DmtTreeImpl class

==================================================================================================*/

#include "dmt.hpp"
#include "xpl_Logger.h"
#include "dmtTreeImpl.hpp"
#include "dmtNodeImpl.hpp"
#include "dmLockingHelper.h"
#include "dmprofile.h"
#include "dm_tree_util.h"

DmtTreeImpl::DmtTreeImpl()
{
  m_nLockType = SYNCML_DM_LOCK_TYPE_AUTOMATIC; // automatic
  m_nLockID = 0;  // not acquired
  m_isAtmoic = FALSE;
}

DmtTreeImpl::DmtTreeImpl( BOOLEAN bReadonly )
{
  m_strServerID = dmTreeObj.GetServerId();
  m_nLockType = bReadonly ? SYNCML_DM_LOCK_TYPE_SHARED : SYNCML_DM_LOCK_TYPE_EXCLUSIVE; 
  m_nLockID = SYNCML_DM_LOCKID_IGNORE;  
  m_isAtmoic = FALSE;
}

DmtTreeImpl::~DmtTreeImpl()
{
  dmTreeObj.ReleaseLock( m_nLockID );
#ifdef DM_NO_LOCKING  
  dmTreeObj.GetLockContextManager().UnLock();
#endif

  DM_MEMORY_STATISTICS_WRITE("tree is released\n");

}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::GetFullPath( CPCHAR path, DMString & fullPath ) const
{
    path = DmtGetSafeStrPtr( path );
    fullPath = m_strRootPath; 

    // KC: Backward GetSubTree(Ex) behavior compatiable
    INT32 nLen = fullPath.length();
    if ( fullPath[nLen-1] == '/' )
    {
        fullPath.SetAt(nLen-1,0);
    }

    DMString strTmpPath;
    {
        if ( path != NULL && path[0] != '.' && path[0] != '/')
        {
            strTmpPath = "/";
            strTmpPath += path; 
        }
        else
        {
            strTmpPath = path; 
        }
    }
    fullPath += strTmpPath.c_str();

    XPL_LOG_DM_TMN_Debug(("GetFullPath: rootpath:%s, path:%s, fullPath:%s, len:%d\n", m_strRootPath.c_str(),  path, fullPath.c_str(), fullPath.length()));
    if ( path[0] != 0 )
    {
        return SYNCML_DM_SUCCESS;
    }

    nLen = fullPath.length();
    if ( nLen == 0 )
    {
        fullPath = ".";
        return SYNCML_DM_SUCCESS;
    }  

    if ( fullPath[nLen-1] == '/' )
    {
        fullPath.SetAt(nLen-1,0);
    }

    XPL_LOG_DM_TMN_Debug(("GetFullPath: rootpath:%s, path:%s, fullPath:%s, len:%d\n", m_strRootPath.c_str(),  path, fullPath.c_str(), fullPath.length()));

    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::GetNode( CPCHAR path, PDmtNode& ptrNode )
{
    DM_PERFORMANCE(DM_GET_NODE_ENTER); 

    path = DmtGetSafeStrPtr( path );
    DMString strFullPath;

    SYNCML_DM_RET_STATUS_T dm_stat;
    dm_stat = GetFullPath( path, strFullPath );
    if ( dm_stat != SYNCML_DM_SUCCESS ) 
    {
        XPL_LOG_DM_TMN_Debug(("GetFullPath failed: path:%s, full path:%s\n", path, strFullPath.c_str()));
        return dm_stat;
    }
    
#ifdef DM_PROFILER_ENABLED
    char str[1024];
    sprintf(str, "GetNode:Native - %s", strFullPath.c_str());
    DM_PROFILE(str);
#endif

    dm_stat = GetNodeByFullPath( strFullPath.c_str(), ptrNode );

    DM_PERFORMANCE(DM_GET_NODE_EXIT); 
    return dm_stat;

}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::GetNodeByFullPath( CPCHAR path, PDmtNode& ptrNode )
{
  // cache current time for last access time updates
  DMTreeCacheCurrentTime cacheTime(&dmTreeObj);

  ptrNode = NULL; // make sure to free prev tree/node if any

  DMLockingHelper oLock( this, m_nLockType );
  
  if ( !oLock.IsLockedSuccessfully() )
  {
    XPL_LOG_DM_TMN_Error(("GetNodeByFullPath failded, cannot apply lock\n"));
    return oLock.GetError();
  }

  path = DmtGetSafeStrPtr( path );

  DMGetData oNameData;
  DMGetData oFormatData;
  SYNCML_DM_RET_STATUS_T dm_stat;

  dm_stat = GetNodeProp( path, "Name", oNameData, TRUE );
  if ( dm_stat == SYNCML_DM_SUCCESS )
     dm_stat = GetNodeProp( path, "Format", oFormatData, TRUE );

  if (dm_stat != SYNCML_DM_SUCCESS)
  {
        XPL_LOG_DM_TMN_Debug(("GetNode Property failed :%s\n", path));
        return dm_stat;
  }
  
  BOOLEAN bIsLeaf =  (oFormatData.m_oData.compare("node") ? FALSE : TRUE);
#ifdef LOB_SUPPORT
  dm_stat = GetNodeProp( path, "ESN", oFormatData, TRUE );
  if (dm_stat != SYNCML_DM_SUCCESS)
  {
        XPL_LOG_DM_TMN_Debug(("GetNode Property failed :%s\n", path));
        return dm_stat;
  }    

  BOOLEAN bESN =  (oFormatData.m_oData.compare("no") ? FALSE : TRUE);
  // An ESN should be a leaf node
  if(!bIsLeaf && bESN)
      return SYNCML_DM_INVALID_PARAMETER;
  ptrNode = new DmtNodeImpl( bIsLeaf, bESN, this, path, oNameData.getCharData() );
#else
  ptrNode = new DmtNodeImpl( bIsLeaf, this, path, oNameData.getCharData() );
#endif

  if ( ptrNode == NULL )
  {
        XPL_LOG_DM_TMN_Error(("low memory\n"));
            return dm_stat;
        return SYNCML_DM_DEVICE_FULL;
  }  

  XPL_LOG_DM_TMN_Debug(("GetNodeByFullPath succeeded :%s\n", path));
  
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::RenameNode( CPCHAR path, CPCHAR szNewNodeName )
{
  DM_PROFILE("RenameNode:Native");    
  PDmtNode ptrNode;
  SYNCML_DM_RET_STATUS_T dm_stat = GetNode( path, ptrNode );

  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;

  return ((DmtNodeImpl*)ptrNode.GetPtr())->Rename( szNewNodeName );
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::DeleteNode( CPCHAR path )
{
  if ( m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  DM_PROFILE("DeleteNode:Native");
  DMLockingHelper oLock( this, SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();

  path = DmtGetSafeStrPtr( path );
  
  DMString strFullPath;
  SYNCML_DM_RET_STATUS_T dm_stat;
  
  dm_stat = GetFullPath( path, strFullPath );
  if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
  
  return dmTreeObj.Delete ( strFullPath, SYNCML_DM_REQUEST_TYPE_API );
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::CreateLeafNode(CPCHAR path, PDmtNode& ptrCreatedNode, const DmtData& value , BOOLEAN isESN)
{
    return SYNCML_DM_COMMAND_NOT_IMPLEMENTED;
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::CreateLeafNode( CPCHAR path, PDmtNode& ptrCreatedNode, const DmtData& value )
{
  if ( m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  DM_PROFILE("CreateLeafNode:Native");      
  {
    DMLockingHelper oLock( this, SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

    if ( !oLock.IsLockedSuccessfully() )
      return oLock.GetError();

    path = DmtGetSafeStrPtr( path );
    DMString strFullPath;
    SYNCML_DM_RET_STATUS_T dm_stat;
  
    dm_stat = GetFullPath( path, strFullPath );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    
    dm_stat = HelperCreateLeafNode( strFullPath.c_str(), value );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
  }
  return GetNode( path, ptrCreatedNode );
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::CreateInteriorNode( CPCHAR path, PDmtNode& ptrCreatedNode )
{
  if ( m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
  {
    return SYNCML_DM_TREE_READONLY;
  }

  DM_PROFILE("CreateInteriorNode:Native");
  {
    DMLockingHelper oLock( this, SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

    if ( !oLock.IsLockedSuccessfully() )
    {
      XPL_LOG_DM_TMN_Debug(("CreateInteriorNode !oLock.IsLockedSuccessfully\n"));
      return oLock.GetError();
    }

    path = DmtGetSafeStrPtr( path );
    DMString strFullPath;
    SYNCML_DM_RET_STATUS_T dm_stat;
    dm_stat = GetFullPath( path, strFullPath );
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
        XPL_LOG_DM_TMN_Debug(("CreateInteriorNode GetFullPath, path:%s, full path:%s\n", path, strFullPath.c_str()));
        return dm_stat;
    }
    
    XPL_LOG_DM_TMN_Debug(("W21034:CreateInteriorNode GetFullPath, path:%s, full path:%s\n", path, strFullPath.c_str()));
    dm_stat = SYNCML_DM_INCOMPLETE_COMMAND;
    DMAddData oAdd;

    dm_stat = oAdd.set(strFullPath,SYNCML_DM_FORMAT_NODE,NULL,0,"text/plain");

    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
      XPL_LOG_DM_TMN_Debug(("CreateInteriorNode Set: path:%s, full path:%s\n", path, strFullPath.c_str()));
      return dm_stat;
    }

    XPL_LOG_DM_TMN_Debug(("Calling Add API to add the node:%s\n",strFullPath.c_str()));
    dm_stat=dmTreeObj.Add( oAdd, SYNCML_DM_REQUEST_TYPE_API );

    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
      XPL_LOG_DM_TMN_Debug(("Failed: CreateInteriorNode Add: path:%s, full path:%s\n", path, strFullPath.c_str()));
      return dm_stat;
    }
  }
  return GetNode( path, ptrCreatedNode );
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::GetChildValuesMap( CPCHAR path, DMMap<DMString, DmtData>& mapNodes )
{
  mapNodes.clear();
  PDmtNode ptrNode;

  // cache current time for last access time updates
  DMTreeCacheCurrentTime cacheTime(&dmTreeObj);

  SYNCML_DM_RET_STATUS_T dm_stat = GetNode( path, ptrNode );

  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;
  
  DMVector<PDmtNode> oChildren;

  dm_stat = ptrNode->GetChildNodes( oChildren );
  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;

  for ( INT32 i = 0; i < oChildren.size(); i++ )
  {
      if ( !oChildren[i]->IsLeaf() )
        continue; // skip interior nodes

#ifdef LOB_SUPPORT
    // No support for External Storage Node
      if (oChildren[i]->IsExternalStorageNode() )
      return SYNCML_DM_RESULTS_TOO_LARGE;
#endif
      DmtData data;
      DMString nodeName;

      dm_stat = oChildren[i]->GetValue( data );
      if ( dm_stat == SYNCML_DM_COMMAND_NOT_ALLOWED )  
      continue;    
          
      if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

      dm_stat = oChildren[i]->GetNodeName(nodeName);
      if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
      
      mapNodes.put( nodeName, data );
    }

  return SYNCML_DM_SUCCESS;
}

/**
 * Set the leaf children with the new set of leaf children.  If the children already 
 * exist, replace them.  If the children are new, add them.  If the children were not
 * in the new set of leaf children, remove them.  Since this is a atomic operation,
 * it will roll back to the original state if any of above operations failed, 
 * 
 * @author Andy
 * @param path the path to a node E.g. ./DevInfo
 * @param mapNodes the new set of leaf children
 * @return the status of the operation
 */
SYNCML_DM_RET_STATUS_T DmtTreeImpl::SetChildValuesMap( CPCHAR path, const DMMap<DMString, DmtData>& mapNodes )
{
  DM_PROFILE("SetChildValuesMap:Native");

  // cache current time for last access time updates
  DMTreeCacheCurrentTime cacheTime(&dmTreeObj);

  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

  if ( m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  DMLockingHelper oLock( this, SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();

  path = DmtGetSafeStrPtr( path );
  DMString strFullPath;
  
  dm_stat = GetFullPath( path, strFullPath );
  if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

  // convert data type and call TNM to setLeafChildren 
  DMMap<DMString, UINT32> newChildrenMap;
  
  dm_stat = dmConvertDataMap(strFullPath.c_str(), mapNodes, newChildrenMap);  
  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;
    
  dm_stat = dmTreeObj.setLeafChildren(strFullPath.c_str(), newChildrenMap);

  dmFreeAddMap(newChildrenMap);

  return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::GetChildNodeNames( CPCHAR path, DMStringVector& aNodes )
{
  DM_PROFILE("GetChildNodeNames:Native");
  PDmtNode ptrNode;
  SYNCML_DM_RET_STATUS_T dm_stat = GetNode( path, ptrNode );

  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;

  DMVector<PDmtNode> oChildren;

  return ((DmtNodeImpl*)ptrNode.GetPtr())->GetChildNodes(oChildren, aNodes );
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::Flush()
{
  DM_PROFILE("Flush:Native");
  
  if ( m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  return ReleaseTree(SYNCML_DM_RELEASE);
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::Commit()
{
    DM_PROFILE("Commit:Native");    
    if ( m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
        return SYNCML_DM_TREE_READONLY;
  
#ifdef DM_ATOMIC_SUPPORTED
    return ReleaseTree(SYNCML_DM_COMMIT);
#else
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
#endif
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::Rollback()
{
    DM_PROFILE("Rollback:Native");   
    if ( m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
        return SYNCML_DM_TREE_READONLY;
  
#ifdef DM_ATOMIC_SUPPORTED
    return ReleaseTree(SYNCML_DM_ROLLBACK);
#else
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
#endif
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::Begin()
{
     DM_PROFILE("Begin:Native");

    if ( m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
        return SYNCML_DM_TREE_READONLY;
  
#ifdef DM_ATOMIC_SUPPORTED
    if ( m_isAtmoic )
        return SYNCML_DM_COMMAND_FAILED;

    {
        DMLockingHelper oLock( this, SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

        if ( !oLock.IsLockedSuccessfully() )
            return oLock.GetError();
    }
  
    SYNCML_DM_RET_STATUS_T dm_stat = ReleaseTree(SYNCML_DM_ATOMIC);

    if ( dm_stat == SYNCML_DM_SUCCESS )
        m_isAtmoic = TRUE;

    return dm_stat;
#else
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
#endif
}

BOOLEAN DmtTreeImpl::IsAtomic() const
{
  return m_isAtmoic;
}

/**
 * @return the DmtPrincipal object that the session was created with.
 */
DmtPrincipal DmtTreeImpl::GetPrincipal() const
{
  return DmtPrincipal(m_strServerID.c_str());
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::StartSession( const DmtPrincipal& oPrincipal, 
                                                 CPCHAR szSubtreeRoot, 
                                                 SYNCML_DM_TREE_LOCK_TYPE_T nLockType )
{
  m_strServerID = oPrincipal.getName();
  m_strRootPath = DmtGetSafeStrPtr( szSubtreeRoot );
  m_nLockType = nLockType;

  XPL_LOG_DM_TMN_Debug(("StartSession, principal:  %s, root:%s\n", m_strServerID.c_str(), m_strRootPath.c_str()));

  if ( !m_strRootPath.empty() )
        m_strRootPath += "/";

  {
    DMLockingHelper oLock( this, m_nLockType );

    if ( !oLock.IsLockedSuccessfully() )
    {
      XPL_LOG_DM_TMN_Error(("StartSession failed, cannot apply lock\n"));
      return oLock.GetError();
    }
  }
 
  // if gets subtree only, make sure the path is correct!
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

  if ( !m_strRootPath.empty() )
  {
    PDmtNode ptrNode;
    dm_stat = GetNode( NULL, ptrNode );
  }

  return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::GetNodeProp(
           CPCHAR path,
           CPCHAR propName,
           DMGetData & oData,
           BOOLEAN isNotCheckACL)
{
  path = DmtGetSafeStrPtr( path );

  DMString propPath(path);
  propPath += "?prop=";
  propPath += propName;

  return dmTreeObj.Get(propPath, oData,SYNCML_DM_REQUEST_TYPE_API);

}


SYNCML_DM_RET_STATUS_T DmtTreeImpl::SetNodeStringProp(
                  CPCHAR path,
                  CPCHAR propName,
                  CPCHAR szValue )
{
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

   if (szValue==NULL)
    return dm_stat;

  path = DmtGetSafeStrPtr( path );
  propName = DmtGetSafeStrPtr( propName );

  DMAddData oReplace;
  DMString propPath(path);

  propPath += "?prop=";
  propPath += propName;

  dm_stat = oReplace.set(propPath,SYNCML_DM_FORMAT_INVALID,szValue,DmStrlen(szValue),"text/plain");
  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;

  return dmTreeObj.Replace( oReplace,SYNCML_DM_REQUEST_TYPE_API );
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::HelperCreateLeafNode( CPCHAR szFullpath, const DmtData& value )
{
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_INCOMPLETE_COMMAND;
  DMAddData oAdd;

  dm_stat = oAdd.set(szFullpath,value,"text/plain");
  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;
  
  return dmTreeObj.Add( oAdd, SYNCML_DM_REQUEST_TYPE_API );
}
#ifdef LOB_SUPPORT
SYNCML_DM_RET_STATUS_T DmtTreeImpl::CloneESN(PDmtNode origNode, PDmtNode newNode)
{
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
  DmtDataChunk chunkData;       
  UINT32 getLen;
  dm_stat = origNode->GetFirstChunk(chunkData);  
  if( dm_stat != SYNCML_DM_SUCCESS)
    return dm_stat;
    
  dm_stat = chunkData.GetReturnLen(getLen); 
  if( dm_stat != SYNCML_DM_SUCCESS)
    return dm_stat;

  dm_stat = chunkData.SetChunkData(NULL, getLen); 
  if( dm_stat != SYNCML_DM_SUCCESS)
    return dm_stat;
    
 dm_stat = newNode->SetFirstChunk(chunkData);  
 if( dm_stat != SYNCML_DM_SUCCESS)
     return dm_stat;
    
 while (true) 
 {
     if (getLen == 0)
    {
        dm_stat = newNode->SetLastChunk(chunkData);  
        break;
    }
    else
     {
        dm_stat = origNode->GetNextChunk(chunkData);
        if( dm_stat != SYNCML_DM_SUCCESS)
            return dm_stat;
    
         dm_stat = chunkData.GetReturnLen(getLen); 
         if( dm_stat != SYNCML_DM_SUCCESS)
             return dm_stat;

         if(getLen == 0)
             continue;

        dm_stat = chunkData.SetChunkData(NULL, getLen); 
        if( dm_stat != SYNCML_DM_SUCCESS)
          return dm_stat;

        dm_stat = newNode->SetNextChunk(chunkData);    
        if( dm_stat != SYNCML_DM_SUCCESS)
            return dm_stat;
    }
    
  }
 return dm_stat;
}
#endif
SYNCML_DM_RET_STATUS_T DmtTreeImpl::Clone( CPCHAR path, CPCHAR szNewNodeName )
{
    PDmtNode origNode;
    PDmtNode newNode;
    SYNCML_DM_RET_STATUS_T dm_stat;
    
    if (szNewNodeName == NULL) 
        return SYNCML_DM_INVALID_PARAMETER;
      
    dm_stat = GetNode(path, origNode);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
        
    path = DmtGetSafeStrPtr( path );

    DMString strFullPath;
  
    dm_stat = GetFullPath( path, strFullPath );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;
    
    char* npath;
    char* newnodeuri = NULL;
    char* parentnodeuri;
    char* ec;
    
    npath = (char*)DmAllocMem(strFullPath.length() + 1);
    if (npath == NULL) 
        return SYNCML_DM_DEVICE_FULL;
    
    DmStrcpy(npath, strFullPath.c_str());
    
    if ((ec = DmStrrchr(npath, '/')) == NULL) 
    {
        DmFreeMem(npath);
        return SYNCML_DM_INVALID_PARAMETER;
    }
    
    *ec = '\0';
    parentnodeuri = (char*)npath;
    
    newnodeuri = (char*) DmAllocMem(DmStrlen(parentnodeuri) + DmStrlen(szNewNodeName) + 2);
    if (newnodeuri == NULL) 
    {
        DmFreeMem(npath);
        return SYNCML_DM_DEVICE_FULL;
    }
    
    DmStrcpy(newnodeuri, parentnodeuri);
    DmStrcat(newnodeuri, "/");
    DmStrcat(newnodeuri, szNewNodeName);    
    *ec = '/';
    
    if (IsValidNode(newnodeuri)) 
    {
        DmFreeMem(npath);
        DmFreeMem(newnodeuri);
        return SYNCML_DM_TARGET_ALREADY_EXISTS;
    }
    
    if (origNode->IsLeaf()) 
    {
        DmtData data;
    // Get node data
    if (!origNode->IsExternalStorageNode()) 
                dm_stat = origNode->GetValue(data);
    // Create leaf node
        if ( dm_stat == SYNCML_DM_SUCCESS )
        {    dm_stat = CreateLeafNode(newnodeuri, newNode, data);
          if (dm_stat != SYNCML_DM_SUCCESS)
          {
                DmFreeMem(npath);
                DmFreeMem(newnodeuri);
              return dm_stat;
          }
        }

#ifdef LOB_SUPPORT
    // Clone ESN
    if(origNode->IsExternalStorageNode())
    {
        if (newNode->IsExternalStorageNode()) 
            dm_stat = CloneESN(origNode, newNode);
        else
            dm_stat = SYNCML_DM_INVALID_PARAMETER;
    }
#endif
    }
    else 
    {
        dm_stat = CreateInteriorNode(newnodeuri, newNode);
        if ( dm_stat == SYNCML_DM_SUCCESS )
            dm_stat = CloneRecurseInteriorNode(origNode, newnodeuri);
    }
    DmFreeMem(npath);
    DmFreeMem(newnodeuri);
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::CloneRecurseInteriorNode(PDmtNode origNode, CPCHAR newNodeUri)
{
    DMVector<PDmtNode> childNodes;
    SYNCML_DM_RET_STATUS_T dm_stat;
    PDmtNode newNode;

    dm_stat = origNode->GetChildNodes(childNodes);
    if (dm_stat != SYNCML_DM_SUCCESS)
        return dm_stat;
        
    for (int i = 0; i < childNodes.size(); i++) 
    {
        PDmtNode node = childNodes[i];
        DMString nodeName;
        dm_stat = node->GetNodeName(nodeName);
        if ( dm_stat != SYNCML_DM_SUCCESS )
            return dm_stat;
        
        DMString newName = newNodeUri;
        
        newName += "/";
        newName += nodeName;        
        
        if (node->IsLeaf()) 
        {
            DmtData data;
            
        if (!node->IsExternalStorageNode()) 
                dm_stat = node->GetValue(data);
            if ( dm_stat == SYNCML_DM_SUCCESS )
                dm_stat = CreateLeafNode(newName.c_str(), newNode, data);
#ifdef LOB_SUPPORT
        // Clone ESN
        if(node->IsExternalStorageNode())
        {
            if (newNode->IsExternalStorageNode()) 
                dm_stat = CloneESN(node, newNode);
            else
                dm_stat = SYNCML_DM_INVALID_PARAMETER;
        }
#endif
        } 
        else 
        {
            dm_stat = CreateInteriorNode(newName.c_str(), newNode);
            if ( dm_stat == SYNCML_DM_SUCCESS )
                dm_stat = CloneRecurseInteriorNode(node, newName.c_str());
        }
        if ( dm_stat != SYNCML_DM_SUCCESS )
                return dm_stat;
            
    }
    
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtTreeImpl::ReleaseTree( SYNCML_DM_COMMAND_T command )
{
 SYNCML_DM_RET_STATUS_T dm_stat = dmTreeObj.ReleaseLock( m_nLockID, command );  

  m_isAtmoic = FALSE;

  return dm_stat;
}
