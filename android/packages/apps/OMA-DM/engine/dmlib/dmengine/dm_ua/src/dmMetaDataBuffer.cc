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

/*==================================================================================================

    Source Name: dmMetaDataBuffer.cc

    General Description: Implementation of the DMMetaDataBuffer class

==================================================================================================*/

#include "dm_uri_utils.h"
#include "dmStringUtil.h"
#include "dmMetaDataBuffer.h" 

UINT32  DMMetaDataBuffer::m_bCorrupted = 0;  // due to multiple passing by value, it's reasonable to use static


DMMetaDataBuffer::DMMetaDataBuffer()
  {
    m_pBuffer = NULL;
    m_nOffset = 0;
#ifndef DM_IGNORE_BMDF_VERIFICATION
    m_nFilesSize = 0;
#endif  
 } 
  
DMMetaDataBuffer::DMMetaDataBuffer(MDF_BUFFER_T pBuffer)
  {
    m_pBuffer = pBuffer;
    m_nOffset = 0;
#ifndef DM_IGNORE_BMDF_VERIFICATION
    m_nFilesSize = ReadUINT32();
    m_nOffset = 0;
#endif  
  } 



CPCHAR DMMetaDataBuffer::ReadString()
{
  UINT32 offset = ReadUINT32();

#ifndef DM_IGNORE_BMDF_VERIFICATION
  if ( offset >= m_nFilesSize ) {
    m_bCorrupted = 1;
    return "";
  }
#endif  
  CPCHAR str = (CPCHAR)(m_pBuffer + offset);
  return str;
}


UINT16 DMMetaDataBuffer::ReadUINT16()
{
    UINT16 value = 0;

    for (UINT32 i=0; i<sizeof(UINT16); i++) 
    {
        value |= (*(m_pBuffer+m_nOffset)) << (i*8);
        m_nOffset++;
    }
    return value;
  }


UINT32 DMMetaDataBuffer::ReadUINT32()
{
  UINT32 value = 0;

  for (int i=0; i<sizeof(UINT32); i++) 
  {
    value |= (*(m_pBuffer + m_nOffset)) << (i*8);
    m_nOffset++;
  }  
  return value; 
}

FLOAT DMMetaDataBuffer::ReadFLOAT()
{
  FLOAT value = 0;
  UINT8 *ptr = (UINT8 *)&value;
  UINT8 len = sizeof(FLOAT) - 1;

  for (int i=0; i<=len; i++) 
  {
    *(ptr+i) = *(m_pBuffer+m_nOffset);
    m_nOffset++;
  }  

  return value; 
}
