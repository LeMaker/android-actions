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

#ifndef SYNCML_DM_WBXMLWRITER_H
#define SYNCML_DM_WBXMLWRITER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "SyncML_DM_Writer.H"
#include "dm_tree_node_class.H"

class SyncML_DM_WBXMLWriter : public SyncML_DM_Writer
{
  public:

    /* Class constructor */
        SyncML_DM_WBXMLWriter(DMFileHandler* fileHandle) : SyncML_DM_Writer(fileHandle) { }

    /* Write a byte of data into the file handle */
    virtual SYNCML_DM_RET_STATUS_T writeByte(UINT8 bYte);

    /* Write a string into the file handle using opaque */
        virtual SYNCML_DM_RET_STATUS_T writeString(CPCHAR string);

    /* Write opaque data into the file handle */
    SYNCML_DM_RET_STATUS_T writeOpaque(const UINT8* data, UINT32 len);

    /* Write raw data string without WBXML encodings */
    SYNCML_DM_RET_STATUS_T writeData(const UINT8* data, UINT8 len);

    /* Write a tree node of type DMNode into the file handle */    
    //SYNCML_DM_RET_STATUS_T writeNode(const DMNode* pNode);

    /* Write a tree node of type DMAddNodeProp into the file handle */
    SYNCML_DM_RET_STATUS_T writeNode(const DMNode* node);

    /* Operators to allocate and delete memory for operation */
    void* operator new(size_t sz);
    void operator delete(void* buf);

    static const int MAX_OPAQUE_STRING_LENGTH;

};

#endif /* SYNCML_DM_WBXMLWRITER_H */
