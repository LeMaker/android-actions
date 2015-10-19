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

#ifndef __DMTTREEIMPL_H__
#define __DMTTREEIMPL_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "jem_defs.hpp"
#include "dmt.hpp"
#include "dm_tree_typedef.h"

class DMLockingHelper;

class DmtTreeImpl : public DmtTree
{ 
protected:
  virtual ~DmtTreeImpl();

public:
  DmtTreeImpl();
  DmtTreeImpl( BOOLEAN bReadonly ); // creates "nolocking" object for internal use only

  virtual SYNCML_DM_RET_STATUS_T  GetNode( CPCHAR path, PDmtNode& ptrNode );
  
  virtual SYNCML_DM_RET_STATUS_T DeleteNode( CPCHAR path );

  virtual SYNCML_DM_RET_STATUS_T Clone( CPCHAR path, CPCHAR szNewNodeName );

  virtual SYNCML_DM_RET_STATUS_T RenameNode( CPCHAR path, CPCHAR szNewNodeName );
  
  virtual SYNCML_DM_RET_STATUS_T CreateLeafNode( CPCHAR path, PDmtNode& ptrCreatedNode, const DmtData& value );
  virtual SYNCML_DM_RET_STATUS_T CreateLeafNode(CPCHAR path, PDmtNode& ptrCreatedNode, const DmtData& value , BOOLEAN isESN);

  virtual SYNCML_DM_RET_STATUS_T CreateInteriorNode( CPCHAR path, PDmtNode& ptrCreatedNode );
  
  virtual SYNCML_DM_RET_STATUS_T GetChildValuesMap( CPCHAR path, DMMap<DMString, DmtData>& mapNodes ) ; 
  
  virtual SYNCML_DM_RET_STATUS_T SetChildValuesMap( CPCHAR path, const DMMap<DMString, DmtData>& mapNodes ); 
  
  virtual SYNCML_DM_RET_STATUS_T GetChildNodeNames( CPCHAR path, DMStringVector& mapNodes ) ;

  virtual SYNCML_DM_RET_STATUS_T Flush();
  
  virtual SYNCML_DM_RET_STATUS_T Commit(); 
  
  virtual SYNCML_DM_RET_STATUS_T Rollback();
  
  virtual SYNCML_DM_RET_STATUS_T Begin();
  
  virtual BOOLEAN IsAtomic() const ;
  
  virtual DmtPrincipal GetPrincipal() const ;

  SYNCML_DM_RET_STATUS_T StartSession( const DmtPrincipal& oPrincipal, 
                                      CPCHAR szSubtreeRoot, 
                                      SYNCML_DM_TREE_LOCK_TYPE_T nLockType );

  SYNCML_DM_RET_STATUS_T SetNodeStringProp(CPCHAR path,
                                                CPCHAR propName,
                                                CPCHAR szValue );
            
  SYNCML_DM_RET_STATUS_T  GetNodeByFullPath( CPCHAR path, PDmtNode& ptrNode );

private:
  SYNCML_DM_RET_STATUS_T GetNodeProp(CPCHAR path,
                                         CPCHAR propName,
                                         DMGetData & oData,
                                         BOOLEAN isNotCheckACL = FALSE);

  SYNCML_DM_RET_STATUS_T GetFullPath( CPCHAR path, DMString & fullPath ) const;
  
  SYNCML_DM_RET_STATUS_T HelperCreateLeafNode( CPCHAR szFullpath, const DmtData& value );
  
  SYNCML_DM_RET_STATUS_T CloneRecurseInteriorNode( PDmtNode origNode, CPCHAR newnodeuri );
  
  SYNCML_DM_RET_STATUS_T ReleaseTree( SYNCML_DM_COMMAND_T command );
#ifdef LOB_SUPPORT
  SYNCML_DM_RET_STATUS_T CloneESN(PDmtNode origNode, PDmtNode newNode);
#endif
  friend class DmtNodeImpl;
  friend class DMLockingHelper;
  
private:
  DMString  m_strServerID;
  DMString  m_strRootPath;
  SYNCML_DM_TREE_LOCK_TYPE_T  m_nLockType;
  INT32  m_nLockID;
  BOOLEAN m_isAtmoic;
};

#endif
