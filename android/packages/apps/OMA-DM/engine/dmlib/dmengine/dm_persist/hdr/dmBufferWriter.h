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

#ifndef DMBUFFERWRITER_H
#define DMBUFFERWITER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: dmBufferReader.h

    General Description: This file contains the declaration of the DMBufferReader class.

==================================================================================================*/

#include "dmtDefs.h"
#include "dmMemory.h"
#include "dmstring.h"

/**
 * DMBufferWriter represents a class that writes into a binary buffer.
 */
class DMBufferWriter
{
  public:
    /**
  * Default constructor that initilizes object
  */
    DMBufferWriter();

   /**
   * Destructor 
   */
    ~DMBufferWriter();
   
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
  * Retrieves pointer on a internal buffer
  */
  inline UINT8 * GetBuffer() const { return m_pBuffer; } 

  /**
  * Retrieves size of a buffer
  */
  inline UINT32 GetSize() const { return m_nSize; }

   /**
  * Allocates internal buffer
  * \param size [in] - number of bytes to allocate
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T Allocate(UINT32 size);

   /**
  * Writes string into a buffer
  * \param str [in] - string to write
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T WriteString(const DMString & str);

   /**
  * Writes string into a buffer
  * \param str [in] - string to write
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T WriteString(CPCHAR str);

   /**
  * Writes integer into a buffer
  * \param data [in] - integer to write
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T WriteUINT32(UINT32 data);

   /**
  * Writes byte into a buffer
  * \param data [in] - byte to write
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T WriteUINT8(UINT8 data); 

   /**
  * Writes two bytes into a buffer
  * \param data [in] - short to write
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T WriteUINT16(UINT16 data);

 private:
  /* Pointer on a buffer */
  UINT8 * m_pBuffer;
  /* Size of a buffer */ 
  UINT32  m_nSize;  
  /* Current position in a buffer */
  UINT32  m_nPos;

   /**
  * Writes specified number of bytes into a buffer
  * \param pBuffer [out] - pointer on a buffer to write from
  * \param count [in] - count of bytes to write
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
    SYNCML_DM_RET_STATUS_T Write(UINT8 * pBuffer, UINT32 count);	
     
};

#endif /* DM_BUFFERREADER_H */
