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

#ifndef __DMTNODEIMPL_H__
#define __DMTNODEIMPL_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "dmt.hpp"

class DmtTreeImpl;

class DmtNodeImpl : public DmtNode
{ 
protected:
  virtual ~DmtNodeImpl();

public:
#ifdef LOB_SUPPORT
  DmtNodeImpl( BOOLEAN bLeaf, BOOLEAN bESN, DmtTree * pTree, CPCHAR oPath, CPCHAR strName );
#else
  DmtNodeImpl( BOOLEAN bLeaf, DmtTree * pTree, CPCHAR oPath, CPCHAR strName );
#endif
  virtual SYNCML_DM_RET_STATUS_T GetTree( PDmtTree& ptrTree ) const;
  
  virtual SYNCML_DM_RET_STATUS_T GetPath(DMString & path) const;
  
  virtual SYNCML_DM_RET_STATUS_T GetAttributes( DmtAttributes& oAttr ) const;
  
  virtual SYNCML_DM_RET_STATUS_T SetTitle( CPCHAR szTitle );
  
  virtual SYNCML_DM_RET_STATUS_T SetAcl( const DmtAcl& oAcl );
  
  virtual SYNCML_DM_RET_STATUS_T GetValue( DmtData& oData ) const;
  
  virtual SYNCML_DM_RET_STATUS_T SetValue( const DmtData& value );
  
  virtual SYNCML_DM_RET_STATUS_T GetChildNodes( DMVector<PDmtNode>& oChildren ) const;

  virtual BOOLEAN IsLeaf() const;
  
  virtual SYNCML_DM_RET_STATUS_T GetChildNode( CPCHAR szPath, PDmtNode& ptrNode );
  
  virtual SYNCML_DM_RET_STATUS_T Execute( CPCHAR strData, DMString& result );
  
  virtual SYNCML_DM_RET_STATUS_T GetNodeName(DMString & name) const;

  virtual DMString GetNodeName() const { return m_strName;};
  
  SYNCML_DM_RET_STATUS_T GetChildNodes( DMVector<PDmtNode>& oChildren, DMStringVector& oChildNames ) const;
  
  SYNCML_DM_RET_STATUS_T Rename( CPCHAR szNewName );
  SYNCML_DM_RET_STATUS_T GetFirstChunk(DmtDataChunk&  dmtChunkData); 

  SYNCML_DM_RET_STATUS_T GetNextChunk(DmtDataChunk& dmtChunkData); 

  SYNCML_DM_RET_STATUS_T SetFirstChunk(DmtDataChunk& dmtChunkData);  

  SYNCML_DM_RET_STATUS_T SetNextChunk(DmtDataChunk& dmtChunkData);  

  SYNCML_DM_RET_STATUS_T SetLastChunk(DmtDataChunk& dmtChunkData);  

  boolean IsExternalStorageNode(void) const;
  
private:
#ifdef LOB_SUPPORT
  SYNCML_DM_RET_STATUS_T  SetEngineChunkData(BOOLEAN isLastChunk);
  SYNCML_DM_RET_STATUS_T  GetEngineChunkData(void);
#endif
  DmtTreeImpl* GetTree() const;
  BOOLEAN  m_bLeaf;
  DMString m_oPath;
  PDmtTree m_ptrTree;
  DMString m_strName;
#ifdef LOB_SUPPORT
  BOOLEAN  m_bESN;
  BOOLEAN  m_LobSetComplete;
  UINT32 m_chunkOffset; 		// offset
  DmtDataChunk*chunkData;
#endif
};

#endif
