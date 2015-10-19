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

#ifndef DMBUFFERREADER_H
#define DMBUFFERREADER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "dmtDefs.h"
#include "dmMemory.h"

/**
 * DMBufferReader represents a class that reads data from a binary buffer.
 */
class DMBufferReader
{
  public:
  /**
  * Constructor that initilizes object
  * \param pBuffer [in] - pointer on a buffer
  * \param size [in] - size of a buffer
  */
   DMBufferReader(UINT8 * pBuffer, UINT32 size);

  /**
  * Reads line from a buffer
  * \return Return Type (char *) 
  * returns pointer on next line, or NULL if end of a buffer is reached 
  */  
   char * fgets();

   /**
  * Checks if end of a buffer is reached
  * \return TRUE if end is reached 
  */ 
   BOOLEAN iseof();

  /**
  * Operator to allocate memory
  * \param sz [in] - number of bytes to be allocated
  */
  inline void* operator new(size_t sz)
  {
    return (DmAllocMem(sz));
  }

  /**
  * Operator to free memory
  * \param buf [in] - pointer on buffer to be freed
  */
  inline void operator delete(void* buf)
  {
    DmFreeMem(buf);
  }

  /**
  * Reads string from a buffer
  */
  CPCHAR ReadString();

  /**
  * Reads one byte from a buffer
  */
  UINT8 ReadUINT8();

  /**
  * Reads two bytes from a buffer
  */
  UINT16 ReadUINT16();

  /**
  * Reads integer from a buffer
  */
  UINT32 ReadUINT32();

  /**
  * Retrieves size of a buffer
  */
  inline UINT32 GetSize() const { return m_nSize; }

 private:
   /* Pointer on a buffer */
  UINT8 * m_pBuffer;
  /* Size of a buffer */ 
  UINT32  m_nSize;  
  /* Current position in a buffer */
  UINT32  m_nPos;

  /**
  * Reads specified number of bytes from a buffer
  * \param pBuffer [out] - pointer on a buffer to read into
  * \param count [in] - number of bytes to read
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T Read(UINT8 * pBuffer, UINT32 count);
     
};

#endif /* DM_BUFFERREADER_H */
