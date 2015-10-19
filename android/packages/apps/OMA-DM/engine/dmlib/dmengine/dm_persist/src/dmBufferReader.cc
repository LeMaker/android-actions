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

#include "dmBufferReader.h"

DMBufferReader::DMBufferReader(UINT8 * pBuffer, UINT32 size)
{
    m_pBuffer = pBuffer;
    m_nSize = size;
    m_nPos = 0;
}

char* 
DMBufferReader::fgets()
{
  if ( m_nSize < 1 || !m_pBuffer )
    return NULL;
  
  if ( iseof() )
    return NULL;

  UINT8 * str = &m_pBuffer[m_nPos];
  
  while ( m_pBuffer[m_nPos] != 0 && m_nPos + 1 < m_nSize )
  {
    m_nPos++;
  }

  m_nPos++; // next line
  return (char*)str;
}

BOOLEAN 
DMBufferReader::iseof()
{
  if ( m_nPos >= m_nSize )
    return TRUE;
  else
    return FALSE;
}

CPCHAR 
DMBufferReader::ReadString()
{
     return fgets();
}


UINT8 
DMBufferReader::ReadUINT8()
{
     UINT8 value = 0;
     
     Read((UINT8*)&value,sizeof(UINT8));
     
     return value;

}

UINT16 
DMBufferReader::ReadUINT16()
{
      UINT16 value = 0;
     
     Read((UINT8*)&value,sizeof(UINT16));
     
     return value;
}

UINT32 
DMBufferReader::ReadUINT32()
{
     UINT32 value = 0;
     
     Read((UINT8*)&value,sizeof(UINT32));
     
     return value;
     
}

SYNCML_DM_RET_STATUS_T 
DMBufferReader::Read(UINT8 * pBuffer, UINT32 count)
{

     if ( m_nSize < 1 || !m_pBuffer )
        return SYNCML_DM_FAIL;
 
     if ( m_nPos + count  >= m_nSize ) 
        return SYNCML_DM_FAIL;

     memcpy(pBuffer,m_pBuffer+m_nPos,count) ;
     m_nPos += count;   
     return SYNCML_DM_SUCCESS; 
}
