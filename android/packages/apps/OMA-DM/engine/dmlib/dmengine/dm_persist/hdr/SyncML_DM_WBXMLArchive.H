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

#ifndef SYNCML_DM_WBXMLARCHIVE_H
#define SYNCML_DM_WBXMLARCHIVE_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: SyncML_DM_WBXMLArchive.H

    General Description: This class provides the implementation of WBXML logging in the file system

==================================================================================================*/

#include "SyncML_DM_Archive.H"
#include "SyncML_DM_WBXMLWriter.H"
#include "SyncML_DM_WBXMLReader.H"
#include "syncml_dm_data_types.h"
#ifdef LOB_SUPPORT
#include "SyncML_Commit_Log.H"
#endif

class DMTree;
class CEnv;

/*==================================================================================================
CLASS DECLARATION
==================================================================================================*/
class SyncML_DM_WBXMLArchive : public SyncML_DM_Archive
{

  public:

    /* The default WBXML header used for string table lookups 
    */
    static const UINT8 DEFAULT_FILE_HEADER[] ;

    static const char FILE_EXTENSION[] ;

    /* The list of tag values for WBXML encoding */
    enum
    {
      WBXML_VERSION = 0x01,  /* WBXML spec ver 1.1 (this is what our xml-to-wbxml tool does) */
      PUBLIC_ID     = 0x01,  /* Unknown or missing identifier; must be <=127 */
      CHARSET       = 0x6A  /* charset is UTF-8; value must be <=127 */
    };

    enum WBXML_TAGS
    { 
      TAG_CONTENT_MASK      = 0x40,
      FILE_START_TAG        = 0x1F,
      TREE_START_TAG        = 0x1E,
      NODE_START_TAG        = 0x1D,
      NAME_START_TAG        = 0x1C,
      ACL_START_TAG         = 0x1B,
      ACCESS_START_TAG      = 0x1A,
      SCOPE_START_TAG       = 0x19,
      CLASSID_START_TAG     = 0x18,
      FORMAT_START_TAG      = 0x17,
      TYPE_START_TAG        = 0x16,
      FLAGS_START_TAG       = 0x15, //DP substitution for SCOPE/CALSSID
      TSTAMP_INT_START_TAG  = 0x14, // DP: integer timestamp representation
      FORMAT_NEW_START_TAG  = 0x13,
      URI_START_TAG         = 0x12,

      DATA_START_TAG        = 0x08,   //leave space 0x20-0x27 for urbana expansion
      PLURAL_START_TAG      = 0x09,
      OPI_DATA_START_TAG    = 0x0A,
      TITLE_START_TAG       = 0x10,
      TSTAMP_START_TAG      = 0x11,
      VERSION_START_TAG     = 0x07,
#ifdef LOB_SUPPORT
      ESN_File_NAME_START_TAG        = 0x20,
#endif
      LOG_START_TAG           = 0x21,
      ENTRY_START_TAG         = 0x22,
      CMDTYPE_START_TAG       = 0x23,
      SOURCE_FILE_TAG       = 0x24,
      TARGET_FILE_TAG       = 0x25,
      END_TAG               = 0x01
    };

    enum
    {   
      OPAQUE_CODE    = 0xC3,
      STRING_TABLE   = 0x00
    };

    /* Class constructor */
    SyncML_DM_WBXMLArchive( CEnv* env, CPCHAR pURI, CPCHAR path) ;
    ~SyncML_DM_WBXMLArchive() ;


	
    /* Deserializes a set of tree files from the file system,
    * creates nodes and passes them to the tree
    */
    virtual SYNCML_DM_RET_STATUS_T deserialize(DMTree* tree,
                                               BOOLEAN bIsReload = FALSE);

    /* Serializes a sub tree of data into the proper mount
    * list referenced WBXML file
    */
    virtual SYNCML_DM_RET_STATUS_T serialize(DMTree* tree) ;
    /* Accessor for the last modification time of the archive 
    */
    virtual XPL_CLK_CLOCK_T getLastModifiedTime() ;

    //Get last persisted time
    virtual XPL_CLK_CLOCK_T getLastSavedTime() ;

    // Check permission (flags from xpl_File.h)
    virtual BOOLEAN verifyPermission(XPL_FS_OPEN_MODE_T permission) const;

    virtual void serializeDone() ;
#ifdef LOB_SUPPORT
    // Rollback operation for ESN
    virtual SYNCML_DM_RET_STATUS_T rollbackESN(DMTree* tree); 
    virtual SYNCML_DM_RET_STATUS_T commitESN(DMTree* tree);
    SyncML_Commit_Log*  GetCommitLogHandler();
    SYNCML_DM_RET_STATUS_T CloseCommitLog();
    SYNCML_DM_RET_STATUS_T  PlayCommitLog();
#endif
  private:
    SYNCML_DM_RET_STATUS_T deserializeFile(DMTree* tree, CPCHAR szFileName, BOOLEAN bWFS);
#ifdef LOB_SUPPORT
    SyncML_Commit_Log *commitLog;
#endif
    XPL_CLK_CLOCK_T lastSavedTime;
    CEnv            *m_pEnv;
};

#endif /* SYNCML_DM_WBXMLARCHIVE_H */
