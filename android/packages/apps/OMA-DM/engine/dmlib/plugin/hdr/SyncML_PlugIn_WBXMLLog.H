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

#ifndef SYNCML_PlugIn_WBXMLLog_H
#define SYNCML_PlugIn_WBXMLLog_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "SyncML_DM_Archive.H"
#include "SyncML_DM_WBXMLWriter.H"
#include "SyncML_DM_WBXMLReader.H"
#include "SyncML_Log.H"
#include "dmt.hpp"

#include "dmtRWPlugin.hpp"

class SyncML_PlugIn_WBXMLLog : public SyncML_Log
{

  public:

    /* Class constructor */
    SyncML_PlugIn_WBXMLLog(const DmtRWPluginTree *pluginTree, CPCHAR rootPath);

    /* Class destructor */
    virtual ~SyncML_PlugIn_WBXMLLog();

    // Open log file and create one if the file doesn't exist
     SYNCML_DM_RET_STATUS_T InitLog(CPCHAR logFileName);
    // Uninitialize log
    SYNCML_DM_RET_STATUS_T UnInitLog();
    /* Log one command */
    SYNCML_DM_RET_STATUS_T logCommand(SYNCML_DM_PLUGIN_COMMAND_T commandType,
                                              CPCHAR pbURI, 
                                              SYNCML_DM_PLUGIN_COMMAND_ATTRIBUTE_T attribute,
                                              const DmtNode* inNode);

    /* Playback log file */
   SYNCML_DM_RET_STATUS_T playLog();
   SYNCML_DM_RET_STATUS_T playLog(CPCHAR logFileName);


  private:

        /* Performs the actual recovery function calls */
    SYNCML_DM_RET_STATUS_T  playbackOneRecord(SYNCML_DM_PLUGIN_COMMAND_T commandType,
                                             CPCHAR pbURI, 
                                             SYNCML_DM_PLUGIN_COMMAND_ATTRIBUTE_T attribute,
                                             DMAddNodeProp* props);

    SYNCML_DM_RET_STATUS_T gotoLastRecord(SyncML_DM_WBXMLReader* reader, 
                                              UINT8 *lastByte, 
                                              INT32 *currentPos );

    SYNCML_DM_RET_STATUS_T writeLogHeader(SyncML_DM_WBXMLWriter* writer);
    SYNCML_DM_RET_STATUS_T writeURIInfo(SYNCML_DM_PLUGIN_COMMAND_T commandType, 
                                          CPCHAR pbURI, 
                                          SyncML_DM_WBXMLWriter* writer);

    SYNCML_DM_RET_STATUS_T writePluginNode(CPCHAR pbURI, 
                                              SyncML_DM_WBXMLWriter* writer, 
                                              const DmtNode* ptrNode);

    SYNCML_DM_RET_STATUS_T  updatePosition(SyncML_DM_WBXMLWriter* writer);

    DMString m_strRootPath;
    INT32 prevRecord;
    const DmtRWPluginTree *pluginTree;
    SyncML_DM_WBXMLWriter* writer;
    SyncML_DM_WBXMLReader* reader;

};

#endif /* SYNCML_PlugIn_WBXMLLog_H */
