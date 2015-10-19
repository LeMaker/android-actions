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

#ifndef SYNCML_DM_WBXMLREADER_H
#define SYNCML_DM_WBXMLREADER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "SyncML_DM_Reader.H"
#include "SyncML_DM_FileHandle.H"
#include "syncml_dm_data_types.h"
#include "dm_tree_typedef.h"

class SyncML_DM_WBXMLReader : public SyncML_DM_Reader
{

public:

  /* Class constructor */
  SyncML_DM_WBXMLReader(DMFileHandler* fileHandle) : SyncML_DM_Reader(fileHandle) { }

  /* Reads a byte of data from the file handle */
  virtual SYNCML_DM_RET_STATUS_T readByte(UINT8 *bYte);

  /* Reads an opaque encoded block of data from the file handle
   * returning the length and a buffer holding the data.
       */
  SYNCML_DM_RET_STATUS_T readOpaque(DMBuffer *pBuffer);
  SYNCML_DM_RET_STATUS_T readOpaque(UINT8 *pBuffer, UINT8 nSize);

  /* Reads a node from the file handle 
   *
   * The stopByte is defined by the calling function and its pointer is passed.
   */
  virtual SYNCML_DM_RET_STATUS_T readNode(DMAddNodeProp* node, 
                                          UINT8* stopByte);

  /* Read the WBXML string table header from the file handle */
  SYNCML_DM_RET_STATUS_T readHeader();

  /* Read a log record from the RW plugin log file */
  SYNCML_DM_RET_STATUS_T readOneLogRecord(SYNCML_DM_PLUGIN_COMMAND_T* cmdType,
                                          DMBuffer  *cmdURI, 
                                          SYNCML_DM_PLUGIN_COMMAND_ATTRIBUTE_T * attribute,
                                          DMAddNodeProp* props,
                                          UINT8* stopByte);

  /* Read a log record from the commit log file */
  SYNCML_DM_RET_STATUS_T readOneCommitLogRecord(SYNCML_DM_COMMAND_T* cmdType,
                                          DMBuffer  *sourceFileName, 
                                          DMBuffer  *targetFileName, 
                                          UINT8* stopByte);
    /* Operators to allocate and delete memory for operation */
  void* operator new(size_t sz);
  void operator delete(void* buf);

private:
  SYNCML_DM_RET_STATUS_T skipTag(UINT8* pByte);    
    
  SYNCML_DM_RET_STATUS_T readLen(UINT32 *pLen);    
    
  SYNCML_DM_RET_STATUS_T readOpaqueTag(UINT8* pByte, UINT8 *pBuffer, UINT8 nSize);  
    
  SYNCML_DM_RET_STATUS_T readOpaqueTag(UINT8* pByte, DMBuffer * pBuffer);  
    
  SYNCML_DM_RET_STATUS_T checkNextByteIsEndTag(UINT8* pByte); 
    
  SYNCML_DM_RET_STATUS_T readNodeName(DMAddNodeProp* nodeProps, UINT8* pByte);
#ifdef LOB_SUPPORT  
  SYNCML_DM_RET_STATUS_T readESNFileName(DMAddNodeProp* nodeProps, UINT8* pByte);
#endif
  SYNCML_DM_RET_STATUS_T readAccess(DMAddNodeProp* nodeProps, UINT8* pByte);
    
  SYNCML_DM_RET_STATUS_T readScope(DMAddNodeProp* nodeProps, UINT8* pByte);
    
  SYNCML_DM_RET_STATUS_T readClassID(DMAddNodeProp* nodeProps, UINT8* pByte);
    
  SYNCML_DM_RET_STATUS_T readFormat(DMAddNodeProp* nodeProps, UINT8* pByte);
  
  SYNCML_DM_RET_STATUS_T readFormatOld(DMAddNodeProp* nodeProps, UINT8* pByte);
    
  SYNCML_DM_RET_STATUS_T readMime(DMAddNodeProp* nodeProps, UINT8* pByte);
    
  SYNCML_DM_RET_STATUS_T readData(DMAddNodeProp* nodeProps, UINT8* pByte);
    
  SYNCML_DM_RET_STATUS_T readOPiData(DMAddNodeProp* nodeProps, UINT8* pByte);
    
  SYNCML_DM_RET_STATUS_T readTitle(DMAddNodeProp* nodeProps, UINT8* pByte);
    
  SYNCML_DM_RET_STATUS_T readFlag(DMAddNodeProp* nodeProps, UINT8* pByte);

#ifndef DM_IGNORE_TSTAMP_AND_VERSION
  SYNCML_DM_RET_STATUS_T readTStamp(DMAddNodeProp* nodeProps, UINT8* pByte);

  SYNCML_DM_RET_STATUS_T readIntTStamp(DMAddNodeProp* nodeProps, UINT8* pByte);
  
  SYNCML_DM_RET_STATUS_T readVersion(DMAddNodeProp* nodeProps, UINT8* pByte);
#endif    
};

#endif /* SYNCML_DM_WBXMLREADER_H */
