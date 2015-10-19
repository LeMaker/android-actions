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

    Name: dmtNode.cc

    General Description: Implementation of the DmtTreeImpl class

==================================================================================================*/

#include "dmt.hpp"
#include "dmtTreeImpl.hpp"
#include "dmtNodeImpl.hpp"
#include "dm_tree_util.h"   
#include "dm_tree_plugin_util.H"   
#include "dmLockingHelper.h"

DmtNodeImpl::~DmtNodeImpl()
{
}
#ifdef LOB_SUPPORT
DmtNodeImpl::DmtNodeImpl( BOOLEAN bLeaf,BOOLEAN bESN, DmtTree *ptrTree, CPCHAR oPath, CPCHAR strName )
{
  m_bLeaf = bLeaf;
  m_ptrTree = ptrTree;
  m_oPath = DmtGetSafeStrPtr( oPath );
  m_strName = strName;
 m_bESN = bESN;
 m_chunkOffset = 0L;         // offset
 chunkData = NULL;
}
#else
DmtNodeImpl::DmtNodeImpl( BOOLEAN bLeaf, DmtTree *ptrTree, CPCHAR oPath, CPCHAR strName )
{
  m_bLeaf = bLeaf;
  m_ptrTree = ptrTree;
  m_oPath = DmtGetSafeStrPtr( oPath );
  m_strName = strName;
}
#endif
SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetTree( PDmtTree& ptrTree ) const
{
  ptrTree = m_ptrTree;
  return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetPath(DMString & path) const
{
    path = m_oPath;
    if ( m_oPath != NULL && path == NULL )
        return SYNCML_DM_DEVICE_FULL;
    return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetNodeName(DMString & name) const
{
    name = m_strName;
    if ( m_strName != NULL && name == NULL )
        return SYNCML_DM_DEVICE_FULL;
    return SYNCML_DM_SUCCESS;
}
    
SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetAttributes( DmtAttributes& oAttr ) const
{
   DMLockingHelper oLock( GetTree(), GetTree()->m_nLockType );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();

  return dmTreeObj.GetAttributes(m_oPath.c_str(), oAttr, SYNCML_DM_REQUEST_TYPE_API);

}

SYNCML_DM_RET_STATUS_T DmtNodeImpl::SetTitle( CPCHAR szTitle )
{
  if ( GetTree()->m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  DMLockingHelper oLock( GetTree(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();

  return GetTree()->SetNodeStringProp( m_oPath.c_str(), "Title", szTitle );
}

SYNCML_DM_RET_STATUS_T DmtNodeImpl::SetAcl( const DmtAcl& oAcl )
{
  if ( GetTree()->m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  DMLockingHelper oLock( GetTree(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();

  return GetTree()->SetNodeStringProp( m_oPath.c_str(), "ACL", oAcl.toString().c_str());
}

SYNCML_DM_RET_STATUS_T DmtNodeImpl::Rename( CPCHAR szNewName )
{
  if ( GetTree()->m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  DMLockingHelper oLock( GetTree(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();
  
  return GetTree()->SetNodeStringProp( m_oPath.c_str(), "Name", szNewName );
}

SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetValue( DmtData& oData ) const
{
#ifdef LOB_SUPPORT
if (m_bESN)
       return SYNCML_DM_RESULTS_TOO_LARGE;
#endif

  DMLockingHelper oLock( GetTree(), GetTree()->m_nLockType );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();

  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
  DMGetData getData;

  dm_stat = dmTreeObj.Get(m_oPath.c_str(),getData,SYNCML_DM_REQUEST_TYPE_API);

  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;

  if (getData.m_nFormat == SYNCML_DM_FORMAT_NODE ) 
  {
     DMStringVector aChildren;
     DMURI ptr(FALSE,getData.getCharData());
     CPCHAR pSegment = NULL;

     oData = DmtData( aChildren );  // set type to "node" for case of empty parent
     
     // we got list of nodes, divided by '/'
     while ( (pSegment = ptr.nextSegment()) != NULL ) 
     {
       dm_stat = oData.AddNodeValue(pSegment);
       if ( dm_stat != SYNCML_DM_SUCCESS )
           return dm_stat;
     }
  }
  else
      dm_stat =  dmBuildData(getData.m_nFormat, getData.m_oData, oData);

  return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtNodeImpl::SetValue( const DmtData& value )
{
#ifdef LOB_SUPPORT
if (m_bESN)
       return SYNCML_DM_RESULTS_TOO_LARGE;
#endif

  if ( GetTree()->m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  DMLockingHelper oLock( GetTree(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();

  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
  DMAddData oReplace;

  dm_stat = oReplace.set(m_oPath.c_str(),value,"text/plain");

  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;
  
  return dmTreeObj.Replace(oReplace,SYNCML_DM_REQUEST_TYPE_API);
}

SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetChildNodes( DMVector<PDmtNode>& oChildren ) const
{
  DMStringVector  aChildren;
  return GetChildNodes( oChildren, aChildren );
}

SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetChildNodes( DMVector<PDmtNode>& oChildren, DMStringVector& aChildren ) const
{
  // cache current time for last access time updates
  DMTreeCacheCurrentTime cacheTime(&dmTreeObj);

  oChildren.clear(); // remove all previous items from array

  if ( m_bLeaf )
    return SYNCML_DM_FAIL;

  DmtData oData;

  SYNCML_DM_RET_STATUS_T dm_stat = GetValue( oData );

  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;

  dm_stat = oData.GetNodeValue( aChildren );

  if ( dm_stat != SYNCML_DM_SUCCESS )
    return dm_stat;

  for ( int i = 0; i < aChildren.size(); i++ )
  {
    DMString oPath = m_oPath;

    oPath += "/";
    oPath += aChildren[i];

    PDmtNode ptrNode;

    dm_stat = GetTree()->GetNodeByFullPath( oPath.c_str(), ptrNode );
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    oChildren.push_back( ptrNode );
  }

  return SYNCML_DM_SUCCESS;
}



BOOLEAN DmtNodeImpl::IsLeaf() const
{
  return m_bLeaf;
}

SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetChildNode( CPCHAR szPath, PDmtNode& ptrNode  )
{
  DMString oPath = m_oPath;

  oPath += "/";
  oPath += DmtGetSafeStrPtr( szPath );

  return GetTree()->GetNodeByFullPath( oPath.c_str(), ptrNode );
}


SYNCML_DM_RET_STATUS_T DmtNodeImpl::Execute( CPCHAR strData, DMString& result )
{

    if ( GetTree()->m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
        return SYNCML_DM_TREE_READONLY;
   
    return dmTreeObj.Exec(m_oPath.c_str(),strData,result);
}

// implementation helpers
DmtTreeImpl* DmtNodeImpl::GetTree() const
{
  return (DmtTreeImpl*)m_ptrTree.GetPtr();
}
#ifdef LOB_SUPPORT
 SYNCML_DM_RET_STATUS_T DmtNodeImpl:: GetEngineChunkData(void)
{
   SYNCML_DM_RET_STATUS_T retStatus =  SYNCML_DM_SUCCESS;
  DMGetData getData;

  retStatus = getData.set(chunkData, m_chunkOffset);

  if ( retStatus != SYNCML_DM_SUCCESS )
    return retStatus;

  retStatus = dmTreeObj.Get(m_oPath.c_str(),getData,SYNCML_DM_REQUEST_TYPE_API);

  if ( retStatus != SYNCML_DM_SUCCESS )
    return retStatus;

  UINT32 returnLen;
  retStatus = chunkData->GetReturnLen(returnLen);
  m_chunkOffset += returnLen;
  
  return retStatus;
}
  SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetFirstChunk(DmtDataChunk&  dmtChunkData)
  {
   DMLockingHelper oLock( GetTree(), GetTree()->m_nLockType );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();

   SYNCML_DM_RET_STATUS_T retStatus =  SYNCML_DM_SUCCESS;
   if (!m_bESN)
       return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;

  if( chunkData != NULL)
  {    
      if(chunkData != &dmtChunkData)
      {    retStatus = chunkData->FreeChunkBuffer();
        if(retStatus!= SYNCML_DM_SUCCESS)
                return retStatus;
      }
  }
  retStatus = dmtChunkData.AllocateChunkBuffer();
  if(retStatus!= SYNCML_DM_SUCCESS)
        return retStatus;
  chunkData = &dmtChunkData;
  m_chunkOffset = 0L;

  retStatus =  GetEngineChunkData();
  return retStatus;
 }
  SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetNextChunk(DmtDataChunk& dmtChunkData)
  {
   DMLockingHelper oLock( GetTree(), GetTree()->m_nLockType );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();

   SYNCML_DM_RET_STATUS_T retStatus =  SYNCML_DM_SUCCESS;
   if (!m_bESN)
       return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;

     if(( chunkData == NULL) ||(chunkData != &dmtChunkData))
    return SYNCML_DM_INVALID_PARAMETER;

  retStatus =  GetEngineChunkData();
  return retStatus;
 }

  SYNCML_DM_RET_STATUS_T DmtNodeImpl:: SetEngineChunkData(BOOLEAN isLastChunk)
  {
   SYNCML_DM_RET_STATUS_T retStatus =  SYNCML_DM_SUCCESS;
  DMAddData oReplace;

  retStatus = oReplace.set(m_oPath.c_str(), chunkData, m_chunkOffset, isLastChunk);

  if ( retStatus != SYNCML_DM_SUCCESS )
    return retStatus;
  
  retStatus =  dmTreeObj.Replace(oReplace,SYNCML_DM_REQUEST_TYPE_API);
  if ( retStatus != SYNCML_DM_SUCCESS )
    return retStatus;

  UINT32 dataLen;
  retStatus = chunkData->GetChunkDataSize(dataLen);
  m_chunkOffset += dataLen;
   return retStatus;
  }
  
  SYNCML_DM_RET_STATUS_T DmtNodeImpl:: SetFirstChunk(DmtDataChunk& dmtChunkData)
  {
   if (!m_bESN)
       return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;

  if ( GetTree()->m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  DMLockingHelper oLock( GetTree(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();

   SYNCML_DM_RET_STATUS_T retStatus =  SYNCML_DM_SUCCESS;
   if (!m_bESN)
       return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;


  if (chunkData != NULL)
  {    
      if(chunkData != &dmtChunkData)
      {    retStatus = chunkData->FreeChunkBuffer();
        if(retStatus!= SYNCML_DM_SUCCESS)
                return retStatus;
      }
  }
  retStatus = dmtChunkData.AllocateChunkBuffer();
  if(retStatus!= SYNCML_DM_SUCCESS)
        return retStatus;
  chunkData = &dmtChunkData;
  m_chunkOffset = 0L;

   retStatus =  SetEngineChunkData(FALSE);
   return retStatus;

  }

  SYNCML_DM_RET_STATUS_T DmtNodeImpl::SetNextChunk(DmtDataChunk& dmtChunkData)
  {
   if (!m_bESN)
       return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;

  if ( GetTree()->m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  DMLockingHelper oLock( GetTree(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();
   SYNCML_DM_RET_STATUS_T retStatus =  SYNCML_DM_SUCCESS;
   if (!m_bESN)
       return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
     if(( chunkData == NULL) ||(chunkData != &dmtChunkData))
    return SYNCML_DM_INVALID_PARAMETER;

   retStatus =  SetEngineChunkData(FALSE);

   return retStatus;

  }
 SYNCML_DM_RET_STATUS_T DmtNodeImpl::SetLastChunk(DmtDataChunk& dmtChunkData)
 {
   if (!m_bESN)
       return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;

  if ( GetTree()->m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    return SYNCML_DM_TREE_READONLY;

  DMLockingHelper oLock( GetTree(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

  if ( !oLock.IsLockedSuccessfully() )
    return oLock.GetError();
   SYNCML_DM_RET_STATUS_T retStatus =  SYNCML_DM_SUCCESS;
   if (!m_bESN)
       return SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT;
     if(( chunkData == NULL) ||(chunkData != &dmtChunkData))
    return SYNCML_DM_INVALID_PARAMETER;

   retStatus =  SetEngineChunkData(TRUE);
   return retStatus;

}
boolean DmtNodeImpl::IsExternalStorageNode(void) const 
{ return m_bESN;
}

#else
SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetFirstChunk(DmtDataChunk&  dmtChunkData)
{
 return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}
SYNCML_DM_RET_STATUS_T DmtNodeImpl::GetNextChunk(DmtDataChunk& dmtChunkData)
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}
SYNCML_DM_RET_STATUS_T DmtNodeImpl::SetFirstChunk(DmtDataChunk& dmtChunkData)
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}
SYNCML_DM_RET_STATUS_T DmtNodeImpl::SetNextChunk(DmtDataChunk& dmtChunkData)
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}
SYNCML_DM_RET_STATUS_T DmtNodeImpl::SetLastChunk(DmtDataChunk& dmtChunkData)
{
    return SYNCML_DM_FEATURE_NOT_SUPPORTED;
}
boolean DmtNodeImpl::IsExternalStorageNode(void) const { return FALSE;}

#endif
