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

/*=============================================================================

                  Module Name: dmtDataChunk.cc

           General Description: Implementation of DmtDataChunk class.

=============================================================================*/

#include "dmt.hpp"
#include "xpl_dm_Manager.h"
#include "dmMemory.h"

/*=============================================================================
*Function called:   DmtDataChunk
*Parameters :     
*Returns:           
*Synopsis:          The constructor of DmtDataChunk
*Pre-conditions:
*Post-Conditions:

=============================================================================*/
DmtDataChunk::DmtDataChunk()
{
    chunkBuffer = NULL;
    dataSize = 0L;
    returnLen = 0L;
    maxDataSize =  XPL_DM_GetChunkSize();
}

/*=============================================================================
*Function called:   DmtDataChunk::attach
*Parameters :     
*Returns:           
*Synopsis:          Attach chunk buffer 
*Pre-conditions:
*Post-Conditions:

=============================================================================*/
void DmtDataChunk::attach(UINT8    *chunkBuffer, UINT32 dataSize)
{
    this->chunkBuffer = chunkBuffer;
    this->dataSize = dataSize;
    returnLen = 0L;
    maxDataSize =  XPL_DM_GetChunkSize();
}

/*=============================================================================
*Function called:   DmtDataChunk::detach
*Parameters :     
*Returns:           
*Synopsis:          Detach chunk buffer 
*Pre-conditions:
*Post-Conditions:

=============================================================================*/
UINT8 * DmtDataChunk::detach()
{
    UINT8 *pBuffer = this->chunkBuffer;
    this->chunkBuffer = NULL;
    this->dataSize = 0;
    returnLen = 0L;
    return pBuffer;
}

/*=============================================================================
*Function called:   ~DmtDataChunk
*Parameters :     
*Returns:           
*Synopsis:          The destructor of DmtDataChunk
*Pre-conditions:
*Post-Conditions:

=============================================================================*/
DmtDataChunk::~DmtDataChunk()
{
    if(chunkBuffer != NULL)
        DmFreeMem(chunkBuffer);
}

/*=============================================================================
*Function called:  GetChunkSize
*Parameters :    
*Returns:       Return chunk buffer size to acess ESN data
*Synopsis:         
*Pre-conditions: 
*Post-Conditions:

=============================================================================*/
 UINT32 DmtDataChunk::GetChunkSize()
{
     return XPL_DM_GetChunkSize();
}

/*=============================================================================
*Function called:   GetChunkData
*Parameters :  
*            bufp --Pointer to the pointer of  chunk data
*Returns:
*    return SYNCML_DM_SUCCESS if get data successfully
*    return other in case of error

*Synopsis:       

*Pre-conditions:
*Post-Conditions:
=============================================================================*/
 SYNCML_DM_RET_STATUS_T DmtDataChunk::GetChunkData( UINT8 **bufp)
 {
 *bufp = chunkBuffer;
  return SYNCML_DM_SUCCESS;
 }

/*=============================================================================
*Function called:   GetChunkDataSize
*Parameters :  
*             dataSize - return data size
*Returns:
*    return SYNCML_DM_SUCCESS if get data size successfully 
*    return other in case of error

*Synopsis:          

*Pre-conditions:
*Post-Conditions:
=============================================================================*/
 SYNCML_DM_RET_STATUS_T DmtDataChunk::GetChunkDataSize( UINT32& dataSize)
{
 dataSize = this->dataSize;
  return SYNCML_DM_SUCCESS;
}

/*=============================================================================
*Function called:   GetReturnLen
*Parameters :  
*            len --Reference to return data size in chunk buffer
*Returns:
*    return SYNCML_DM_SUCCESS if get  return size successfully
*    return other in case of error

*Synopsis:   

*Pre-conditions:
*Post-Conditions:
=============================================================================*/

 SYNCML_DM_RET_STATUS_T DmtDataChunk::GetReturnLen( UINT32& len ) const
{
    len = returnLen;
      return SYNCML_DM_SUCCESS;
}
/*=============================================================================
*Function called:   SetReturnLen
*Parameters :  
*            len --Reference to return data size in chunk buffer
*Returns:
*    return SYNCML_DM_SUCCESS if set return length successfully
*    return other in case of error

*Synopsis:   

*Pre-conditions:
*Post-Conditions:
=============================================================================*/
 SYNCML_DM_RET_STATUS_T DmtDataChunk::SetReturnLen( UINT32& len )
{
  returnLen = len;
  return SYNCML_DM_SUCCESS;
}

/*=============================================================================
*Function called:   SetChunkData
*Parameters :  
            buf --Pointer to data buffer need to copy to chunk buffer
            dataSize -- Data size in the buf
*Returns:
*    return SYNCML_DM_SUCCESS if if set chunk data successfully
*    return other in case of error

*Synopsis:         

*Pre-conditions:
*Post-Conditions:
=============================================================================*/

SYNCML_DM_RET_STATUS_T DmtDataChunk::SetChunkData(const UINT8 *buf, UINT32 dataSize)
{
SYNCML_DM_RET_STATUS_T resStatus = SYNCML_DM_SUCCESS;
    if( buf != NULL)
    {
        if(dataSize > maxDataSize)
            return  SYNCML_DM_INVALID_PARAMETER;

        if(chunkBuffer == NULL)
        {      chunkBuffer = (UINT8*)DmAllocMem(maxDataSize);
            if(chunkBuffer == NULL)
                return SYNCML_DM_DEVICE_FULL;
        }    
        memcpy(chunkBuffer, buf , dataSize);
    }
    this->dataSize = dataSize;
    return resStatus;
}
/*=============================================================================
*Function called:   AllocateChunkBuffer
*Parameters :  
*Returns:
*    return SYNCML_DM_SUCCESS if if allocate chunk buffer successfully
*    return other in case of error

*Synopsis:         

*Pre-conditions:
*Post-Conditions:
=============================================================================*/
SYNCML_DM_RET_STATUS_T DmtDataChunk::AllocateChunkBuffer()
{
    if (chunkBuffer == NULL)
    {
        chunkBuffer = (UINT8*)DmAllocMem(maxDataSize);
        if (chunkBuffer == NULL)
            return SYNCML_DM_DEVICE_FULL;
    }    
    return SYNCML_DM_SUCCESS;
}
/*=============================================================================
*Function called:   FreeChunkBuffer
*Parameters :  
*Returns:
*    return SYNCML_DM_SUCCESS if if free chunk buffer successfully
*    return other in case of error

*Synopsis:         

*Pre-conditions:
*Post-Conditions:
=============================================================================*/
SYNCML_DM_RET_STATUS_T DmtDataChunk::FreeChunkBuffer()
{
    if (chunkBuffer != NULL)
    {
        DmFreeMem(chunkBuffer);
        chunkBuffer = NULL;
    }
    return SYNCML_DM_SUCCESS;
}
