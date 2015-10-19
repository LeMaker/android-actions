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

#ifndef DMMETADATABUFFER_H
#define DMMETADATABUFFER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmMetaDataBuffer.h

General Description: This file contains declaration declaration 
                            of DMMetaBuffer class used for reading meta information from MDF file

==================================================================================================*/

#include <ctype.h>
#include "syncml_dm_data_types.h" 
#include "dmvector.h"
#include "dmbuffer.h"

typedef const UINT8 * MDF_BUFFER_T;

/**
* DMMetaDataBuffer represent MDF reader 
*/
class DMMetaDataBuffer
{

public:
  /**
 * Default constructor
 */
  DMMetaDataBuffer();

 /**
  * Constructor, that sets pointer on a MDF buffer
  * \param pBuffer [in] - pointer on binary buffer
  */
  DMMetaDataBuffer(MDF_BUFFER_T pBuffer);

  /**
  * Retrieves pointer on a binary MDF buffer
  */
  inline MDF_BUFFER_T GetBuffer() const { return m_pBuffer; } 

  /**
  * Sets pointer on a MDF buffer
  * \param pBuffer [in] - pointer on binary buffer
  */
  inline void SetBuffer(MDF_BUFFER_T pBuffer) 
  {
  	m_pBuffer = pBuffer; 
#ifndef DM_IGNORE_BMDF_VERIFICATION
       m_nOffset = 0;
       m_nFilesSize = ReadUINT32();
#endif  
       m_nOffset = 0;
  }

  /**
  * Sets offset in a buffer
  * \param offset [in] - position in the buffer to be set
  */
  inline void SetOffset(UINT32 offset) 
  { 
    	m_nOffset = offset; 
#ifndef DM_IGNORE_BMDF_VERIFICATION
    	if ( m_nOffset >= m_nFilesSize ) 
	{
      	    m_bCorrupted = 1;
          m_nOffset = 0;
      }
#endif  
  } 

   /**
  * Changes position in a buffer
  * \param offset [in] - number of bytes to move pistion in the buffer
  */
  inline void IncOffset(UINT32 offset) 
  { 
  	m_nOffset += offset; 
#ifndef DM_IGNORE_BMDF_VERIFICATION
    	if ( m_nOffset >= m_nFilesSize ) 
	{
      	    m_bCorrupted = 1;
          m_nOffset = 0;
      }
#endif  
  }

  /**
  * Retrieves current offset in a buffer
  */
  inline UINT32 GetOffset() { return m_nOffset; }

  /**
  * Reads string from a buffer
  */
  CPCHAR ReadString();

   /**
  * Reads one byte from a buffer
  */
  inline UINT8 ReadUINT8()
  {
  	return *(m_pBuffer+m_nOffset++);
  }

  /**
  * Reads two bytes value from a buffer
  */
  UINT16 ReadUINT16();

  /**
  * Reads integer value from a buffer
  */
  UINT32 ReadUINT32();

   /**
  * Reads float value from a buffer
  */
  FLOAT ReadFLOAT();

#ifndef DM_IGNORE_BMDF_VERIFICATION
  /**
  * Checks if  a buffer is corrupted 
  * \return 1 if corrupted
  */ 
  inline int IsCorrupted() const { return m_bCorrupted;}

  /**
  * Resets flag for corruption verification 
  */ 
  inline void ResetCorrupted() {  m_bCorrupted = 0;}
#endif  

private:
  /* Pointer on binary buffer that hold MDF */
  MDF_BUFFER_T  m_pBuffer;
  /* Buffer offset */ 
  UINT32 m_nOffset;
#ifndef DM_IGNORE_BMDF_VERIFICATION
  /* Buffer size */
  UINT32  m_nFilesSize;
  /* Flag to verify if buffer is corrupted */
  static UINT32  m_bCorrupted;  
#endif  
};

#endif
