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

#ifndef SYNCML_Commit_Log_H
#define SYNCML_Commit_Log_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "syncml_dm_data_types.h"
#include "dmMemory.h"
#include "SyncML_DM_WBXMLWriter.H"
#include "SyncML_DM_WBXMLReader.H"
#include "SyncML_Log.H"

/*==================================================================================================
CLASS DECLARATION
==================================================================================================*/
class SyncML_Commit_Log:public SyncML_Log
{
  public:
  /* Class constructor */
  SyncML_Commit_Log();

  /* Class destructor */
  virtual ~SyncML_Commit_Log();
 // Uninitialize log
  SYNCML_DM_RET_STATUS_T UnInitLog();

  SYNCML_DM_RET_STATUS_T logCommand(SYNCML_DM_COMMAND_T commandType,
						                        	    CPCHAR pSourceFileName, 
						                        	    CPCHAR pTargetFileName);

  /* Take each entry in the log file and play it on the tree (by passing to
   * the tree and node manager) according to logic based on flags in the log entry
   */
   SYNCML_DM_RET_STATUS_T playLog() ;
   SYNCML_DM_RET_STATUS_T playLog(CPCHAR logFileName);
   SYNCML_DM_RET_STATUS_T CloseLog();

  private:
        /* Performs the actual recovery function calls */
    SYNCML_DM_RET_STATUS_T  playbackOneRecord(SYNCML_DM_COMMAND_T commandType,
													 CPCHAR pSourceFileName,
						                        	    			  CPCHAR pTargetFileName);
    SYNCML_DM_RET_STATUS_T  renameESNfile(CPCHAR pTargetFileName,  DMFileHandler &handler);
    SyncML_DM_WBXMLWriter* writer;
    SyncML_DM_WBXMLReader* reader;

};

#endif /* SYNCML_Commit_Log_H */
