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

#ifndef SYNCML_DM_WRITER_H
#define SYNCML_DM_WRITER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "SyncML_DM_FileHandle.H"

class SyncML_DM_Writer
{

  public:

    /* Class constructor */
    SyncML_DM_Writer(DMFileHandler* handle) : fileHandle(handle){ }

        /* Class destructor */
    virtual ~SyncML_DM_Writer(){ }

    /* Accessor for setting the file handle used by this class */
    virtual void setFileHandle(DMFileHandler* fileHandle){ this->fileHandle = fileHandle; }

    /* write a byte into the file 
     * implementations of this class will take advantage of this method
     */
    virtual SYNCML_DM_RET_STATUS_T writeByte(UINT8 bYte) = 0;

    /* write a string into the file 
     * implementations of this class will take advantage of this method
     */
    virtual SYNCML_DM_RET_STATUS_T writeString(CPCHAR string) = 0;

  protected:

    /* file handle used by class' utility methods */
    DMFileHandler* fileHandle;

};

#endif /* SYNCML_DM_WRITER_H */
