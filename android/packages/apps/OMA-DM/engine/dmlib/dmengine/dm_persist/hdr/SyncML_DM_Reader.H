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

#ifndef SYNCML_DM_READER_H
#define SYNCML_DM_READER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: SyncML_DM_Reader.H

    General Description: ABC class for all readers of tree data.  This class does not 
    possess a .cc file since there are two functions with little content.

==================================================================================================*/

#include "SyncML_DM_FileHandle.H"

class DMAddNodeProp;

class SyncML_DM_Reader
{

public:

  /* Class constructor */
  SyncML_DM_Reader(DMFileHandler* handle) : fileHandle(handle){ }

  /* Class destructor */
  virtual ~SyncML_DM_Reader(){ }

  /* Accessor for setting the file handle used by the class' utilities */
  virtual void setFileHandle(DMFileHandler* fileHandle){ this->fileHandle = fileHandle; }

  /* Read a byte into the argument of this function 
   * pure virtual method.  Implementations of Reader must override 
   */
  virtual SYNCML_DM_RET_STATUS_T readByte(UINT8* bYte) = 0;

  /* Read a tree node into the data structure pointer argument and 
   * return the next byte read 
   *
   * This function allocates the memory returned in its node argument
   * The stopByte is defined by the calling function and its pointer is passed.
   */
  virtual SYNCML_DM_RET_STATUS_T readNode(DMAddNodeProp* node, UINT8* stopByte) = 0;

protected:

  /* The file handle used by this object. 
   * As implemented, this class requires the user to open a file "on the side"
   * and set this variable so then the writer can work on the file. The caller
   * is responsinble for closing the file and freeing the handle.
   */
  DMFileHandler* fileHandle;

};

#endif /* SYNCML_DM_READER_H */
