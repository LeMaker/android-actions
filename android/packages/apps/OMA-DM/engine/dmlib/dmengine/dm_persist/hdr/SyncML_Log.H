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

#ifndef SYNCML_Log_H
#define SYNCML_Log_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: SyncML_Log.H

    General Description: This file contains the declaration for the SyncML_Log class.
==================================================================================================*/

#include "syncml_dm_data_types.h"
#include "dmMemory.h"
#include "SyncML_DM_FileHandle.H"

/*==================================================================================================
CLASS DECLARATION
==================================================================================================*/
class SyncML_Log
{
  public:
  /* Class constructor */
  SyncML_Log();

  /* Class destructor */
  virtual ~SyncML_Log();

 // Open log file and create one if the file doesn't exist
  virtual SYNCML_DM_RET_STATUS_T InitLog(CPCHAR logFileName);

 // Uninitialize log
  virtual SYNCML_DM_RET_STATUS_T UnInitLog();

  /* Take each entry in the log file and play it on the tree (by passing to
   * the tree and node manager) according to logic based on flags in the log entry
   */
  virtual SYNCML_DM_RET_STATUS_T playLog() = 0;
  virtual SYNCML_DM_RET_STATUS_T playLog(CPCHAR logFileName);

  virtual SYNCML_DM_RET_STATUS_T RemoveLog();

   virtual SYNCML_DM_RET_STATUS_T CloseLog();

  /* Accessors for the file handle associated with this log */
  virtual DMFileHandler* getLogFileHandle();
  virtual SYNCML_DM_RET_STATUS_T setLogFileHandle(DMFileHandler *fileHandle);

  inline void* operator new(size_t sz)
  {
       return (DmAllocMem(sz));
  }
        
  inline void operator delete(void* buf)
  {
      DmFreeMem(buf);
  }

  protected:
  DMFileHandler *fileHandle;

};

#endif /* SYNCML_Log_H */
