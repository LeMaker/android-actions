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

#include "dmBufferWriter.h"
#include "dmMemory.h"

DMBufferWriter::DMBufferWriter()
{
    m_pBuffer = NULL;
    m_nSize = 0;
    m_nPos = 0;
}

DMBufferWriter::~DMBufferWriter()
{
    if ( m_pBuffer != NULL )
        DmFreeMem(m_pBuffer);
}

SYNCML_DM_RET_STATUS_T 
DMBufferWriter::Allocate(UINT32 size)
{
    m_pBuffer = (UINT8*)DmAllocMem(size);
    if ( m_pBuffer == NULL )
        return SYNCML_DM_DEVICE_FULL;

    memset(m_pBuffer,0,size);
    m_nSize = size;

    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
DMBufferWriter::WriteString(const DMString & str)
{
    if ( m_pBuffer == NULL )
        return SYNCML_DM_FAIL;

    INT32 length = str.length();

     if ( m_nPos + length + 1  > m_nSize ) 
        return SYNCML_DM_FAIL;

    memcpy(m_pBuffer+m_nPos,str.c_str(),length+1);  
    m_nPos += length+1;

    return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T 
DMBufferWriter::WriteString(CPCHAR str)
{

    if ( str == NULL )
        return SYNCML_DM_INVALID_PARAMETER; 
    
    INT32 length = DmStrlen(str);

    return Write((UINT8*)str,length+1); 

}


SYNCML_DM_RET_STATUS_T 
DMBufferWriter::WriteUINT32(UINT32 data)
{
     return Write((UINT8*)&data,sizeof(UINT32)); 
}


SYNCML_DM_RET_STATUS_T 
DMBufferWriter::WriteUINT8(UINT8 data)
{
    return Write((UINT8*)&data,sizeof(UINT8)); 
}


SYNCML_DM_RET_STATUS_T
DMBufferWriter::WriteUINT16(UINT16 data)
{
    return Write((UINT8*)&data,sizeof(UINT16)); 
}


SYNCML_DM_RET_STATUS_T 
DMBufferWriter::Write(UINT8 * pBuffer, UINT32 count)
{
    if ( m_pBuffer == NULL )
        return SYNCML_DM_FAIL;

    if ( m_nPos + count  > m_nSize ) 
        return SYNCML_DM_FAIL;

    memcpy(m_pBuffer+m_nPos,pBuffer,count);  
    m_nPos += count;

    return SYNCML_DM_SUCCESS;
}
