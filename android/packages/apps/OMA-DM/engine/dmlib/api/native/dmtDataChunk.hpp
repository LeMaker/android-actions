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

#ifndef __DMTDATACHUNK_H__
#define __DMTDATACHUNK_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "jem_defs.hpp"
#include "dmtDefs.h"

/**
 * DmtDataChunk encapsulates various methods to access External Storage Node (ESN) data.
 * \par Category: General
 * \par Persistence: Transient
 * \par Security: Non-Secure
 * \par Migration State: FINAL
 */
class DmtDataChunk 
{
public:
  /**
  * Default constructor - no memory allocation performed.
  */
	DmtDataChunk();
  /** 
  * Destructor - freeing all dynamic resources 
  */
	~DmtDataChunk();
  /**
  *  Gets chunk buffer size
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return chunk buffer size
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
	static UINT32 GetChunkSize();
  
  /**
  * Gets ESN data in chunk buffer
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param bufp -[out] Pointer to chunk data 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    SYNCML_DM_RET_STATUS_T GetChunkData( UINT8 **bufp);

  /**
  * Gets ESN data in chunk buffer
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dataSize -[out] Actual length of return  data in chunk buffer
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    SYNCML_DM_RET_STATUS_T GetChunkDataSize( UINT32& dataSize);

  /**
  * Gets ESN data length in chunk buffer
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param len -[out] Actual length of return  data in chunk buffer
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
     SYNCML_DM_RET_STATUS_T GetReturnLen( UINT32& len ) const;

  /**
  * Sets return data length in chunk buffer
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param len -[In] Actual length of return  data in chunk buffer
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    SYNCML_DM_RET_STATUS_T SetReturnLen( UINT32& len );

  /**
  * Sets input data (chunk)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
	SYNCML_DM_RET_STATUS_T SetChunkData(const UINT8 *buf, UINT32 dataSize);  

  /**
  * Allocates chunk buffer
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
	SYNCML_DM_RET_STATUS_T AllocateChunkBuffer();  

  /**
  * Frees chunk buffer
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
	SYNCML_DM_RET_STATUS_T FreeChunkBuffer();  

  /**
  * Detaches chunk buffer
  * \warning This functions is for internal usage only!!! 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return pointer to chunk buffer
  */
   UINT8 * detach();

  /**
  * Attaches chunk buffer
  * \warning This functions is for internal usage only!!!  
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param chunkBuffer -[In]  chunk buffer with data which should be attached
  * \param dataSize -[In] Actual length of data in chunk buffer
  */   
   void attach(UINT8  *chunkBuffer, UINT32 dataSize);
 	
 private:
  	UINT8	*chunkBuffer;		// chunk data buffer
  	UINT32 dataSize;			// actual size of input data		
  	UINT32 returnLen; 			// valid content length returned	
  	UINT32 maxDataSize; 		// Chunk buffer size
  
};

#endif
