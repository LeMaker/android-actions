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

#include "SyncML_DM_WBXMLReader.H"
#include "SyncML_DM_WBXMLArchive.H"
#include "SyncML_Log.H"
#include "SyncML_PlugIn_WBXMLLog.H"
#include "dm_tree_util.h"
#include "xpl_Logger.h"
#include "dmprofile.h"

/*==================================================================================================

Function:    SyncML_DM_WBXMLReader::readByte

Description: Reads a byte of data from the file handle

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readByte(UINT8* byte) {
    return this->fileHandle->read(byte, 1);
}

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLReader::readHeader
 
Description: Reads the WBXML header from the file start.
             We expect the exact 4 bytes that we serialize back out.
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readHeader() {
    UINT8 bYte;

    /* Read in version */
    if (this->readByte(&bYte) != SYNCML_DM_SUCCESS
            || bYte != SyncML_DM_WBXMLArchive::WBXML_VERSION) {
        return SYNCML_DM_IO_FAILURE;
    }

    /* Read in public identifier */
    if (this->readByte(&bYte) != SYNCML_DM_SUCCESS
            || bYte != SyncML_DM_WBXMLArchive::PUBLIC_ID) {
        return SYNCML_DM_IO_FAILURE;
    }

    /* read charset (check for UTF-8 for all files) */
    if (this->readByte(&bYte) != SYNCML_DM_SUCCESS
            || bYte != SyncML_DM_WBXMLArchive::CHARSET) {
        return SYNCML_DM_IO_FAILURE;
    }

    /* Read in string table length; better be zero, since not supported */
    if (this->readByte(&bYte) != SYNCML_DM_SUCCESS
            || bYte != 0) {
        return SYNCML_DM_IO_FAILURE;
    }

    return SYNCML_DM_SUCCESS;
}


/*==================================================================================================
 
Function:    SyncML_DM_WBXMLReader::readLen
        
Description: 
        
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readLen(UINT32 *pLen) {
    SYNCML_DM_RET_STATUS_T ret;
    UINT8   bYte;

    /* Read and decode the data length.
     * Note! The first byte of the length not expected to have the continue bit set (in msb).
     * If it is, the length is >=128 and this code does not know how to decode it.
     */

    if ((ret = this->readByte(&bYte)) != SYNCML_DM_SUCCESS) {
        return ret;
    }
    *pLen = bYte & 0x7F;
    while(bYte & 0x80) {
        if ((ret = this->readByte(&bYte)) != SYNCML_DM_SUCCESS)
            return ret;
        *pLen = ( (*pLen)<<7) |(bYte & 0x7F);
    }

    /* If there is data, get a buffer, read the data, append '\0' */
    if (*pLen>127)
    {
        XPL_LOG_DM_TMN_Debug(("*pLen=%d\n", *pLen));
    }    
    return ret;
}

 /*==================================================================================================
 
Function:    SyncML_DM_WBXMLReader::readOpaque
        
Description: Reads opaquely encoded data expected to immediately follow the OPAQUE_CODE byte in the
             WBXML file. This function expects a data length field, encoded as mb_u_int32
             32 bit unsigned integer, encoded in multi-byte integer format, to precede the data.
             The length is decoded and returned. The data may or may not be a string, and so no
             assumptions about null termination are made while reading it. However, as a courtesy to
             the caller, a null character is appended to the data read. The returned length does not
             include this character.
 
             Note! The implementation only supports encoded lengths <= 127.
             An I/O error (SYNCML_DM_IO_FAILURE) is returned otherwise, and no data buffer is allocated
             (NULL is returned).
 
             If the encoded length is zero, no data buffer is allocated
             (NULL is returned). The result is SYNCML_DM_SUCCESS.
 
             The file pointer is automatically advanced by the underlying file reader service. Any
             underlying file I/O error code is expected to be SYNCML_DM_IO_FAILURE.
 
             The caller must free the data buffer.
             
             Future enhancement: If incoming *pDATA is NULL, this function should allocate memory;
             else, the caller has provided the memory, and *pLen indicates how large it is. This
             would reduce memory churn.
        
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readOpaque(UINT8 *pBuffer, UINT8 nSize) {
    SYNCML_DM_RET_STATUS_T ret;
    UINT32 nLen;

    *pBuffer = 0;
    if ((ret = readLen(&nLen)) != SYNCML_DM_SUCCESS) 
        return ret;
    if ( nLen > nSize )
        return SYNCML_DM_FAIL;
    if (nLen > 0) 
        ret = fileHandle->read(pBuffer, nLen);
    return ret;
}

 /*==================================================================================================
 
Function:    SyncML_DM_WBXMLReader::readOpaque
        
Description: Reads opaquely encoded data expected to immediately follow the OPAQUE_CODE byte in the
             WBXML file. This function expects a data length field, encoded as mb_u_int32
             32 bit unsigned integer, encoded in multi-byte integer format, to precede the data.
             The length is decoded and returned. The data may or may not be a string, and so no
             assumptions about null termination are made while reading it. However, as a courtesy to
             the caller, a null character is appended to the data read. The returned length does not
             include this character.
 
             Note! The implementation only supports encoded lengths <= 127.
             An I/O error (SYNCML_DM_IO_FAILURE) is returned otherwise, and no data buffer is allocated
             (NULL is returned).
 
             If the encoded length is zero, no data buffer is allocated
             (NULL is returned). The result is SYNCML_DM_SUCCESS.
 
             The file pointer is automatically advanced by the underlying file reader service. Any
             underlying file I/O error code is expected to be SYNCML_DM_IO_FAILURE.
 
             The caller must free the data buffer.
             
             Future enhancement: If incoming *pDATA is NULL, this function should allocate memory;
             else, the caller has provided the memory, and *pLen indicates how large it is. This
             would reduce memory churn.
        
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readOpaque(DMBuffer *pBuffer) {
    SYNCML_DM_RET_STATUS_T ret;
    UINT32 nLen;

    pBuffer->clear();
    if ((ret = readLen(&nLen)) != SYNCML_DM_SUCCESS) {
        return ret;
    }
    if (nLen > 0) 
    {
        pBuffer->allocate(nLen);
        if ( pBuffer->getBuffer() != NULL )
        {
           ret = fileHandle->read(pBuffer->getBuffer(), nLen);
           pBuffer->setSize(nLen);    
        }
        else
           return SYNCML_DM_DEVICE_FULL;
    }

    return ret;
}


/*==================================================================================================
Function:    SyncML_DM_WBXMLReader::readNode
 
Description: Reads a node and its properties from a WBXML file.  The NODE_START_TAG
     is assumed to already have been read.  Exits after all properties have 
     been read, signaled by an END_TAG or the NODE_START_TAG of the next node.
 
Returns: - The byte that caused the node to stop (END_TAG or NODE_START_TAG w/content bit)
             - A node props structure, with:
                node->pbURI set to NULL
                node->pbACL possibly set to an Alloc'd string
                The caller is responsible for freeing these structs (on error or success).
 
              SYNCML_DM_TREE_CORRUPT is returned if any I/O error occurs,
                    or an unexpected opcode is encountered, or an invalid length is encountered.
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readNode(DMAddNodeProp* node, UINT8* stopByte) {

    UINT8 bYte;
    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_FAIL;

    
    /* Initialize the property data */
    node->clear();

    /* While a byte is read correctly */
    while((ret_stat = this->readByte(&bYte)) == SYNCML_DM_SUCCESS) {

        /* Switch on that byte as an indicator of the data to follow
         * and copy that data.
         */
        switch(bYte) {
        case (SyncML_DM_WBXMLArchive::END_TAG):
        case (SyncML_DM_WBXMLArchive::NODE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            *stopByte = bYte;
            return SYNCML_DM_SUCCESS;

#ifdef LOB_SUPPORT  
        case (SyncML_DM_WBXMLArchive::ESN_File_NAME_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readESNFileName(node, &bYte)) != SYNCML_DM_SUCCESS)
               break;
            continue;   /* Skip to top of while() */
#endif
        case (SyncML_DM_WBXMLArchive::NAME_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readNodeName(node, &bYte)) != SYNCML_DM_SUCCESS)
               break;
            continue;   /* Skip to top of while() */

        case (SyncML_DM_WBXMLArchive::ACL_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
        case (SyncML_DM_WBXMLArchive::PLURAL_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
        case (SyncML_DM_WBXMLArchive::URI_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->skipTag(&bYte)) != SYNCML_DM_SUCCESS)
                break;
            continue;   /* Skip to top of while() */

        case (SyncML_DM_WBXMLArchive::ACCESS_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readAccess(node, &bYte)) != SYNCML_DM_SUCCESS)
                break;
            continue;   /* Skip to top of while() */

        case (SyncML_DM_WBXMLArchive::SCOPE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readScope(node, &bYte)) != SYNCML_DM_SUCCESS)
                break;
            continue;   /* Skip to top of while() */

        case (SyncML_DM_WBXMLArchive::CLASSID_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readClassID(node, &bYte)) != SYNCML_DM_SUCCESS)
                break;
            continue;   /* Skip to top of while() */

        case (SyncML_DM_WBXMLArchive::FORMAT_NEW_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readFormat(node, &bYte)) != SYNCML_DM_SUCCESS)
                break;
            continue;   /* Skip to top of while() */

        case (SyncML_DM_WBXMLArchive::FORMAT_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readFormatOld(node, &bYte)) != SYNCML_DM_SUCCESS)
                break;
            continue;   /* Skip to top of while() */
#ifdef LOB_SUPPORT  
            /* Handles an Empty NodeName */
        case SyncML_DM_WBXMLArchive::ESN_File_NAME_START_TAG:
            continue;   /* Skip to top of while() */
#endif
            /* Handles an Empty NodeName */
        case SyncML_DM_WBXMLArchive::NAME_START_TAG:
            continue;   /* Skip to top of while() */

        case (SyncML_DM_WBXMLArchive::TYPE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readMime(node, &bYte)) != SYNCML_DM_SUCCESS)
                break;
            continue;   /* Skip to top of while() */

        case (SyncML_DM_WBXMLArchive::DATA_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readData(node, &bYte)) != SYNCML_DM_SUCCESS)
                 break;
            continue;   /* Skip to top of while() */

        case (SyncML_DM_WBXMLArchive::TITLE_START_TAG |SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readTitle(node, &bYte)) != SYNCML_DM_SUCCESS)
                 break;
            continue;   /* Skip to top of while() */

#ifndef DM_IGNORE_TSTAMP_AND_VERSION

        case (SyncML_DM_WBXMLArchive::VERSION_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readVersion(node, &bYte)) != SYNCML_DM_SUCCESS)
                break;
            continue;   /* Skip to top of while() */
          // dp: compact format 
        case (SyncML_DM_WBXMLArchive::TSTAMP_INT_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
          if((ret_stat = this->readIntTStamp(node, &bYte)) != SYNCML_DM_SUCCESS)
               break;
           continue;

#endif

        case (SyncML_DM_WBXMLArchive::FLAGS_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readFlag(node, &bYte)) != SYNCML_DM_SUCCESS)
                break;
            
          continue;
          
        case (SyncML_DM_WBXMLArchive::OPI_DATA_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK):
            if((ret_stat = this->readOPiData(node, &bYte)) != SYNCML_DM_SUCCESS)
                break;
            
          continue;
          
        default:    /* Do not expect to get here */
          ret_stat = SYNCML_DM_FAIL;
            break;
        }

        /* If we get here, then either none of the continue statements in the switch happened,
         * or else an unexpected byte value was read (and we exited the switch via a break).
         * This next break will leave the while() loop, adn the fcn exits with an error.
         */
        break;
    }

    return ret_stat;
}
/*==================================================================================================
       
Function:    SyncML_DM_WBXMLReader::readOneLogRecord
                 
Description: The DMAddNodeProp and URI fields are completed by this function 
         on successful return.  This function calls SyncML_DM_WBXMLReader::readNode() in order 
             to fill in the property data for the node.
                 
Memory policy: The caller is responsible for freeing the created objects (on error or success).

Notes:  The DMAddNodeProp.pbURI field is not filled by this function
        
==================================================================================================*/

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readOneLogRecord(SYNCML_DM_PLUGIN_COMMAND_T* cmdType,
                                        DMBuffer  *cmdURI, 
                                        SYNCML_DM_PLUGIN_COMMAND_ATTRIBUTE_T * attribute,
                                        DMAddNodeProp* props,
                                        UINT8* stopByte)
{

    UINT8 bYte;
    DMString  tmpStr;

    /* Default values */
    *cmdType = SYNCML_DM_PLUGIN_NO_COMMAND;
    props->clear();
    SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_FAIL;

    //Read command type start tag
    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
        || bYte != (SyncML_DM_WBXMLArchive::CMDTYPE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
        return SYNCML_DM_IO_FAILURE;
    
    //Read command type 
    if((ret_code = this->readByte(cmdType)) != SYNCML_DM_SUCCESS)
      return SYNCML_DM_IO_FAILURE;
    
    //Read END_TAG
    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS ||
        bYte != SyncML_DM_WBXMLArchive::END_TAG)
        return SYNCML_DM_IO_FAILURE;
    
    //Read command type start tag
    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
        || bYte != (SyncML_DM_WBXMLArchive::URI_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
        return SYNCML_DM_IO_FAILURE;
    
    //Read command type start tag
    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
        || bYte != SyncML_DM_WBXMLArchive::OPAQUE_CODE)
        return SYNCML_DM_IO_FAILURE;
    
    // Read URI
    if((ret_code = this->readOpaque(cmdURI)) != SYNCML_DM_SUCCESS)
        return SYNCML_DM_IO_FAILURE;
    
    //Read END_TAG
    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
        || bYte != SyncML_DM_WBXMLArchive::END_TAG)
        return SYNCML_DM_IO_FAILURE;

    switch ( *cmdType ) 
    {
        case SYNCML_DM_PLUGIN_ADD:    
            //Read command type start tag
            if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
                || bYte != (SyncML_DM_WBXMLArchive::NODE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
                return SYNCML_DM_IO_FAILURE;
            if((ret_code = this->readNode(props, stopByte)) != SYNCML_DM_SUCCESS)
                return SYNCML_DM_IO_FAILURE;
            break;

        case SYNCML_DM_PLUGIN_DELETE:
        case SYNCML_DM_PLUGIN_ADD_CHILD:    
            if((ret_code = this->readByte(stopByte)) != SYNCML_DM_SUCCESS
                || *stopByte != SyncML_DM_WBXMLArchive::END_TAG)
                return SYNCML_DM_IO_FAILURE;
            break;
        
       case SYNCML_DM_REPLACE:
            if((ret_code = this->readByte(attribute)) != SYNCML_DM_SUCCESS)
                return SYNCML_DM_IO_FAILURE;    
            switch(*attribute) 
            {
                case SYNCML_DM_PLUGIN_COMMAND_ON_NODE:
                    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
                        || bYte != (SyncML_DM_WBXMLArchive::DATA_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
                        return SYNCML_DM_IO_FAILURE;
                    
                    if((ret_code = this->readData(props, stopByte)) != SYNCML_DM_SUCCESS)
                        return SYNCML_DM_IO_FAILURE;
                    break;


                case SYNCML_DM_PLUGIN_COMMAND_ON_NAME_PROPERTY:
                    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
                        || bYte != (SyncML_DM_WBXMLArchive::NAME_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
                        return SYNCML_DM_IO_FAILURE;
                    
                    if((ret_code = this->readNodeName(props, stopByte)) != SYNCML_DM_SUCCESS)
                        return SYNCML_DM_IO_FAILURE;
                    break;
                
                case SYNCML_DM_PLUGIN_COMMAND_ON_TITLE_PROPERTY:
                    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
                        || bYte != (SyncML_DM_WBXMLArchive::TITLE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
                        return SYNCML_DM_IO_FAILURE;
                    if((ret_code = this->readTitle(props, stopByte)) != SYNCML_DM_SUCCESS)
                        return SYNCML_DM_IO_FAILURE;
                    break;
					
#ifdef LOB_SUPPORT  
		    case SYNCML_DM_PLUGIN_COMMAND_ON_LOB_PROPERTY:
			if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS)
				return SYNCML_DM_IO_FAILURE;

			if(bYte == (SyncML_DM_WBXMLArchive::ESN_File_NAME_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
			{	if((ret_code = this->readESNFileName(props, stopByte)) != SYNCML_DM_SUCCESS)
					return SYNCML_DM_IO_FAILURE;
			}
			else
				if(bYte != SyncML_DM_WBXMLArchive::END_TAG)
					return SYNCML_DM_IO_FAILURE;
			
			break;
#endif				
                default:
                    return SYNCML_DM_IO_FAILURE;
            }
            break;


        default:
            return SYNCML_DM_IO_FAILURE;
    }
            
    return SYNCML_DM_SUCCESS;
   
}
/*==================================================================================================
       
Function:    SyncML_DM_WBXMLReader::readOneCommitLogRecord
                 
Description: Read one record from commit log file

==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_DM_WBXMLReader::readOneCommitLogRecord(SYNCML_DM_COMMAND_T* cmdType,
											DMBuffer  *sourceFileName, 
											DMBuffer  *targetFileName, 
											UINT8* /*stopByte */)
{

    UINT8 bYte;
    DMString  tmpStr;

    /* Default values */
    *cmdType = SYNCML_DM_NO_COMMAND;

    SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_FAIL;

    //Read command type start tag
    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
        || bYte != (SyncML_DM_WBXMLArchive::CMDTYPE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
		        return SYNCML_DM_IO_FAILURE;
    
    //Read command type 
    if((ret_code = this->readByte(cmdType)) != SYNCML_DM_SUCCESS)
      return SYNCML_DM_IO_FAILURE;
    
    //Read END_TAG
    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS ||
        bYte != SyncML_DM_WBXMLArchive::END_TAG)
        return SYNCML_DM_IO_FAILURE;
    
    //Read command type start tag
    if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
        || bYte != (SyncML_DM_WBXMLArchive::TARGET_FILE_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
        return SYNCML_DM_IO_FAILURE;
    
    //Read target file name
    if((ret_code = this->readOpaqueTag(&bYte, targetFileName)) != SYNCML_DM_SUCCESS)
        return SYNCML_DM_IO_FAILURE;
    
    if( *cmdType == SYNCML_DM_REPLACE) 
    {
		if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS)
			return SYNCML_DM_IO_FAILURE;
		// Is the original file name exist?
		if(bYte == (SyncML_DM_WBXMLArchive::SOURCE_FILE_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
		{
			// Read source  file name
			if((ret_code = this->readOpaqueTag(&bYte, sourceFileName)) != SYNCML_DM_SUCCESS)
					return SYNCML_DM_IO_FAILURE;

			//Read END_TAG
			if((ret_code = this->readByte(&bYte)) != SYNCML_DM_SUCCESS
				|| bYte != SyncML_DM_WBXMLArchive::END_TAG)
				return SYNCML_DM_IO_FAILURE;
		}
    }
    return SYNCML_DM_SUCCESS;
   
}

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLReader::operator new
 
Description: Allocate memory for this object
 
Memory policy: The caller is responsible to delete (free) the new object (alloc'd memory)
 
==================================================================================================*/
void *
SyncML_DM_WBXMLReader::operator new(size_t sz) {
    return (DmAllocMem(sz));
}

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLReader::operator delete
 
Description: De-allocate memory for this object
 
==================================================================================================*/
void
SyncML_DM_WBXMLReader::operator delete (void *buf) {
    DmFreeMem(buf);
}


SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readOpaqueTag(UINT8* pByte, UINT8 *pBuffer, UINT8 nSize) {

    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;

    if((ret_stat = this->readByte(pByte)) != SYNCML_DM_SUCCESS)
        return ret_stat;

    if(*pByte != SyncML_DM_WBXMLArchive::OPAQUE_CODE)
        return SYNCML_DM_TREE_CORRUPT;

    ret_stat = this->readOpaque(pBuffer, nSize);
    if( ret_stat != SYNCML_DM_SUCCESS )
        return ret_stat;

    return checkNextByteIsEndTag(pByte);

}

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readOpaqueTag(UINT8* pByte, DMBuffer *pBuffer) {

    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;

    if((ret_stat = this->readByte(pByte)) != SYNCML_DM_SUCCESS)
        return ret_stat;

    if(*pByte != SyncML_DM_WBXMLArchive::OPAQUE_CODE)
        return SYNCML_DM_TREE_CORRUPT;

    ret_stat = this->readOpaque(pBuffer); 
    if ( ret_stat != SYNCML_DM_SUCCESS )
        return ret_stat;

    return checkNextByteIsEndTag(pByte);
}


SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::checkNextByteIsEndTag(UINT8* pByte)
{
    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;

    if((ret_stat = this->readByte(pByte)) != SYNCML_DM_SUCCESS)
        return ret_stat;
    if(*pByte != SyncML_DM_WBXMLArchive::END_TAG)
        return  SYNCML_DM_TREE_CORRUPT;

    return ret_stat;
}


SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::skipTag(UINT8* pByte)
{
    DMBuffer property;

    return this->readOpaqueTag(pByte,&property);
}

#ifdef LOB_SUPPORT  
SYNCML_DM_RET_STATUS_T 
SyncML_DM_WBXMLReader::readESNFileName(DMAddNodeProp* nodeProps, UINT8* pByte)
{
    return this->readOpaqueTag(pByte, &nodeProps->m_oESNFileName);
}
#endif

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readNodeName(DMAddNodeProp* nodeProps, UINT8* pByte) 
{
   
    return this->readOpaqueTag(pByte, &nodeProps->m_oName);

}


SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readAccess(DMAddNodeProp* /*nodeProps*/, UINT8* pByte) 
{
    UINT8 property[2];
    
    return this->readOpaqueTag(pByte, property,2);

}

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readScope(DMAddNodeProp* nodeProps, UINT8* pByte) 
{
    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;
    UINT8 bScope = 0;
    
    if((ret_stat = this->readOpaqueTag(pByte, &bScope,1)) != SYNCML_DM_SUCCESS)
        return ret_stat;

    if ( bScope == DMTNM_NODE_PERMANENT )
      nodeProps->m_nFlags |= DMNode::enum_NodePermanent;

    return ret_stat;
}


SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readClassID(DMAddNodeProp* /*nodeProps*/, UINT8* pByte) 
{
    UINT8 property[2];
    
    return this->readOpaqueTag(pByte, property,2);
}

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readFormat(DMAddNodeProp* nodeProps, UINT8* pByte) 
{
    return readOpaqueTag(pByte, &nodeProps->m_nFormat,1);
}

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readFormatOld(DMAddNodeProp* nodeProps, UINT8* pByte) 
{
    SYNCML_DM_FORMAT_T nOldFormat = 0;
    SYNCML_DM_RET_STATUS_T nRes = readOpaqueTag(pByte, &nOldFormat,1);

    static const SYNCML_DM_FORMAT_T aFormats[] = {
        SYNCML_DM_FORMAT_BIN,
        SYNCML_DM_FORMAT_BOOL,
        SYNCML_DM_FORMAT_B64,
        SYNCML_DM_FORMAT_CHR,
        SYNCML_DM_FORMAT_INT,
        SYNCML_DM_FORMAT_NODE,
        SYNCML_DM_FORMAT_NULL,
        SYNCML_DM_FORMAT_XML,
        SYNCML_DM_FORMAT_INVALID,
        SYNCML_DM_FORMAT_TEST,
        SYNCML_DM_FORMAT_FLOAT,  
        SYNCML_DM_FORMAT_DATE,
        SYNCML_DM_FORMAT_TIME
       
      };

    if ( nRes == SYNCML_DM_SUCCESS && nOldFormat < DIM(aFormats) )
        nodeProps->m_nFormat = aFormats[nOldFormat];
    else
      nodeProps->m_nFormat = SYNCML_DM_FORMAT_NODE;
    
    return nRes;
}

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readMime(DMAddNodeProp* nodeProps, UINT8* pByte) 
{
    return this->readOpaqueTag(pByte, &nodeProps->m_oMimeType);
}

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readData(DMAddNodeProp* nodeProps, UINT8* pByte)
{
    return this->readOpaqueTag(pByte, &nodeProps->m_oData);
}

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readOPiData(DMAddNodeProp* nodeProps, UINT8* pByte)
{
    return this->readOpaqueTag(pByte, &nodeProps->m_oOPiData);
}

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readTitle(DMAddNodeProp* nodeProps, UINT8* pByte) 
{
    return this->readOpaqueTag(pByte, &nodeProps->m_oTitle);
}


SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readFlag(DMAddNodeProp* nodeProps, UINT8* pByte) 
{
    UINT8 property[2];
    SYNCML_DM_RET_STATUS_T ret_stat;

    if((ret_stat = this->readOpaqueTag(pByte, (UINT8*)&property,2)) != SYNCML_DM_SUCCESS)
        return ret_stat;

    nodeProps->m_nFlags = (property[0] << 8) | property[1]; /* stored in Big Endian order */
   nodeProps->m_nFlags &= ~DMNode::enum_NodeNotPersisted;
   return ret_stat;
}

#ifndef DM_IGNORE_TSTAMP_AND_VERSION

SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readIntTStamp(DMAddNodeProp* nodeProps, UINT8* pByte) 
{
    UINT8 timeStamp[ sizeof(XPL_CLK_CLOCK_T) ];
    SYNCML_DM_RET_STATUS_T ret_stat;

    if ( (ret_stat = this->readOpaqueTag(pByte,  timeStamp, sizeof(timeStamp) )) != SYNCML_DM_SUCCESS)
        return ret_stat;

    nodeProps->m_nTStamp=0;
    for ( UINT32 i = 0; i < sizeof(XPL_CLK_CLOCK_T); i++ ) {
      //NOT USE nodeProps->wTStamp |= ((XPL_CLK_CLOCK_T)timeStamp[sizeof(XPL_CLK_CLOCK_T) - i -1]) << (i*8);
      nodeProps->m_nTStamp |= ((XPL_CLK_CLOCK_T)timeStamp[i]) << (i*8);
    }

    return ret_stat;
}


SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLReader::readVersion(DMAddNodeProp* nodeProps, UINT8* pByte) {
  
    UINT8 property[2];
    SYNCML_DM_RET_STATUS_T ret_stat;

    if((ret_stat = this->readOpaqueTag(pByte, (UINT8*)&property,2)) != SYNCML_DM_SUCCESS)
        return ret_stat;

    nodeProps->m_nVerNo = (property[0] << 8) | property[1]; /* stored in Big Endian order */
    return ret_stat;
}

#endif
