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

#ifndef SYNCML_PlugIn_Log_H
#define SYNCML_PlugIn_Log_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: SyncML_PlugIn_Log.H

    General Description: This class provides an ABC (Abstract Base Class) for concrete 
  implementations of a SyncML_PlugIn_Log.

==================================================================================================*/

#include "syncml_dm_data_types.h"
#include "dmMemory.h"
#include "SyncML_DM_Reader.H"
#include "SyncML_DM_Writer.H"
#include "SyncML_DM_FileHandle.H"

class DmtNode;
class DMFileHandler;

/*==================================================================================================
CLASS DECLARATION
==================================================================================================*/
class SyncML_PlugIn_Log
{
  public:
  /* Class constructor */
  SyncML_PlugIn_Log();

  /* Class destructor */
  virtual ~SyncML_PlugIn_Log();

  /* Method for logging a command into the log entity represented by the
   * concrete implementation of this class.
   */
  virtual SYNCML_DM_RET_STATUS_T logCommand(SYNCML_DM_PLUGIN_COMMAND_T commandType,
                        CPCHAR pbURI, 
                        SYNCML_DM_PLUGIN_COMMAND_ATTRIBUTE_T attribute,
                        const DmtNode* inNode) = 0;

  /* Take each entry in the log file and play it on the tree (by passing to
   * the tree and node manager) according to logic based on flags in the log entry
   */
  virtual SYNCML_DM_RET_STATUS_T playLog() = 0;

  /* Accessors for the file handle associated with this log */
  virtual DMFileHandler* getLogFileHandle();
  virtual void setLogFileHandle(DMFileHandler *fileHandle);

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

#endif /* SYNCML_PlugIn_Log_H */
