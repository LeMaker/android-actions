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

#ifndef __FILE_MANAGER__H__
#define __FILE_MANAGER__H__

#include "syncml_dm_data_types.h"        //For DM data type definitions
#include "dmt.hpp"
#include "dmdefs.h"

// assume that we support 32 or less files, so we can pass "file set" as an integer (32 bits)
// every bit means file "selected" if it set. For example, value 6, binary 0110 means files "1" and "2"
// Bit 0 is file0, bit 1 is file1, bit is file 2
typedef UINT32 FILESETTYPE;

#define FILESET_ALLFILES    0xFFFFFFFF

class DMTree;

class CMultipleFileManager
{
public:
  CMultipleFileManager();

  virtual SYNCML_DM_RET_STATUS_T Init( DMTree* tree );
  virtual SYNCML_DM_RET_STATUS_T DeInit();

  virtual ~CMultipleFileManager() ;
     
  virtual int GetFileNumber() const ;

  virtual FILESETTYPE GetFileSetByURI( CPCHAR szURI, BOOLEAN bSharedLock ) const ;

  virtual SYNCML_DM_RET_STATUS_T LockFileSet(CPCHAR szURI, 
                                                                 FILESETTYPE nFileSet, 
                                                                 SYNCML_DM_TREE_LOCK_TYPE_T eLockType) ;

  virtual SYNCML_DM_RET_STATUS_T UnlockFileSet(CPCHAR szURI,  
                                                                 FILESETTYPE nFileSet,
                                                                 SYNCML_DM_TREE_LOCK_TYPE_T eLockType) ;
  
  virtual SYNCML_DM_RET_STATUS_T RollbackFileSet(CPCHAR szURI,  FILESETTYPE nFileSet ) ;
  
  virtual SYNCML_DM_RET_STATUS_T FlushFileSet(CPCHAR szURI, FILESETTYPE nFileSet ,SYNCML_DM_COMMAND_T type);

private:
   DMTree                     *m_pTree;
};

#endif
