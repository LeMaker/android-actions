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

#include "SyncML_DM_WBXMLWriter.H"
#include "SyncML_DM_WBXMLArchive.H"

#ifdef LOB_SUPPORT
#include "dm_tree_default_ESN_class.H" //header file for class defn
#endif

#include "dm_uri_utils.h"
const int SyncML_DM_WBXMLWriter::MAX_OPAQUE_STRING_LENGTH = 255;

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLWriter::writeByte
 
Description: Write a byte of data to the file handle
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLWriter::writeByte(UINT8 byte) {
    return this->fileHandle->write(&byte, 1);
}

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLWriter::writeString
 
Description: Writes a string of data as opaque data. The null terminator will
             not be written, as opaque data has a length stored with it. If the
             string is NULL, that will be written as an opaque data with length 0.
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLWriter::writeString(CPCHAR string) {
    UINT32 len = 0;

    if (string != NULL) 
        len = DmStrlen(string);
    return writeOpaque((UINT8*)string, len);
}

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLWriter::writeData
 
Description: Writes a given amount of raw data (without WBXML formatting or special encodings)
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLWriter::writeData(const UINT8* data, UINT8 len) {
    return this->fileHandle->write(data, len);
}

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLWriter::writeOpaque
 
Description:
            Write opaque data of a given length.
            The data better be less than 128 bytes in length,
            as this service does not handle encoding the mb_u_int32 format.
 
            The service returns the success/fail code from the lower level I/O, or
            may also return SYNCML_DM_IO_FAILURE if the data was too long.
 
            A length of 0 and/or a NULL data buffer is handled.
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLWriter::writeOpaque(const UINT8* data, UINT32 len) {
    SYNCML_DM_RET_STATUS_T ret_stat;

    ret_stat = writeByte(SyncML_DM_WBXMLArchive::OPAQUE_CODE);

    if (ret_stat != SYNCML_DM_SUCCESS)
        return ret_stat;

    if((data != NULL) && (len != 0)) {

        UINT8 result, bits;
        int continuation=0;
        for(int shift = 28; shift > 0; shift -= 7) {
            bits = (len >> shift) & 0x7F;
            if ((bits!=0) ||(continuation != 0)) {
                result = 0x80 | bits;
                ret_stat = writeByte((UINT8)result);
                if (ret_stat != SYNCML_DM_SUCCESS)
                    return SYNCML_DM_IO_FAILURE;
            }
            if (bits != 0)
                continuation = 1;
        }
        result = len & 0x7F;
        ret_stat = writeByte((UINT8)result);
        if (ret_stat != SYNCML_DM_SUCCESS)
            return SYNCML_DM_IO_FAILURE;

        ret_stat = this->fileHandle->write(data, (UINT16)len);

    } else {
        /* Since the pointer to the byte is NULL, then there is no opaque data
         * for this node to be written, so the length field is zero. Zero encoded
         * in the mb_u_int32 format takes one byte, 0.
         */
        ret_stat = writeByte((UINT8)0);
    }

    return ret_stat;
}


/*==================================================================================================
 
Function:    SyncML_DM_WBXMLWriter::writeNode
 
Description: Write a SYNCML_DM_NODE_PROPERTIES_T type node of data to the WBXML file
    Note! This service does not write the END_TAG for the whole node; the next byte that can occur
    is a NODE_START_TAG (indicating the start of a new sub-node) or an END_TAG, (indicating the end
    of the current node, and parhaps the end of the current branch of the tree). The caller must
    decide what to write next.
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLWriter::writeNode(const DMNode* node) {
    /* Write the start tag */
    if(writeByte(SyncML_DM_WBXMLArchive::NODE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS)
        return SYNCML_DM_IO_FAILURE;

    /* Write node properties */
    CPCHAR tmpStr;
    tmpStr = node->getName();

    if ( DmStrcmp(tmpStr,"") != 0 )  /* If there a NodeName to write... */
    {
        if(writeByte(SyncML_DM_WBXMLArchive::NAME_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
                writeString(tmpStr) != SYNCML_DM_SUCCESS ||
                writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
            return SYNCML_DM_IO_FAILURE;
    }
    UINT8 nodeFormat = (const UINT8) node->getFormat();

    if ( node->IsOverlayPIData() && nodeFormat == SYNCML_DM_FORMAT_NODE )
      nodeFormat = SYNCML_DM_FORMAT_NODE_PDATA;
    
    if ( nodeFormat != SYNCML_DM_FORMAT_NODE ) {
      if(writeByte(SyncML_DM_WBXMLArchive::FORMAT_NEW_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
              writeOpaque((const UINT8*)&nodeFormat, sizeof(nodeFormat)) != SYNCML_DM_SUCCESS ||
              writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
          return SYNCML_DM_IO_FAILURE;
    }
#ifdef LOB_SUPPORT  
    if(node->IsESN())
    {
	 DMString tempESNName;

  	 // Get ESN pointer
	 const DMDefaultESN *tempESN = reinterpret_cast<const DMDefaultESN *>(node);
	// Get original storage file name
	 tmpStr = tempESN->GetOriginalInternalFileName();

	// Storage file changed ?
	if(tempESN->IsDirty())
	{
		if(tempESN->IsSetComplete())
		{
			// Newly created node
			if(tmpStr == NULL)
			{	tempESNName.RemoveSufix(tempESN->GetInternalStorageFileName(), SYNCML_DM_DOT);
				tmpStr = tempESNName.c_str();
			}
		}
		else
			 return SYNCML_DM_INCOMPLETE_COMMAND;
	}
	if(DmStrlen(tmpStr) != 0)   /* If there a file name  to write... */
	 {
	        if(writeByte(SyncML_DM_WBXMLArchive::ESN_File_NAME_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
	                writeString(tmpStr) != SYNCML_DM_SUCCESS ||
	                writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
	            return SYNCML_DM_IO_FAILURE;
	  }
    }
#endif
    if ( node->IsOverlayPIData() && node->getOverlayPIData() && node->getOverlayPIData()->size() > 0 )
    {
      if(writeByte(SyncML_DM_WBXMLArchive::OPI_DATA_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
              writeOpaque(node->getOverlayPIData()->get_data(), node->getOverlayPIData()->size()) != SYNCML_DM_SUCCESS ||
              writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
          return SYNCML_DM_IO_FAILURE;
    }
    UINT16 flag = node->getFlags(); flag &= ~DMNode::enum_NodeNotPersisted;
        /* Guarantee Big Endian storage of a UINT16 */
    UINT8 flagNo[2] = { static_cast<UINT8>(flag >> 8),
                        static_cast<UINT8>(flag & 0xFF) };
    if ( flag ){
      if(writeByte(SyncML_DM_WBXMLArchive::FLAGS_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
              writeOpaque(flagNo, sizeof(flagNo)) != SYNCML_DM_SUCCESS ||
              writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
          return SYNCML_DM_IO_FAILURE;
    }

    tmpStr = node->getType();
    if (DmStrcmp(tmpStr,"") != 0)  /* If there are any mimetype to write... */
    {
        if(writeByte(SyncML_DM_WBXMLArchive::TYPE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
                writeString(tmpStr) != SYNCML_DM_SUCCESS ||
                writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
            return SYNCML_DM_IO_FAILURE;
    }

    const DMBuffer * psData = node->getData();

    if ( psData && psData->getSize() )  /* If there are any ACLs to write... */
    {
        if(writeByte(SyncML_DM_WBXMLArchive::DATA_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
                writeOpaque(psData->getBuffer(), psData->getSize()) != SYNCML_DM_SUCCESS ||
                writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
            return SYNCML_DM_IO_FAILURE;
    }

    tmpStr = node->getTitle();
    if (DmStrcmp(tmpStr,"") != 0) {
        if(writeByte(SyncML_DM_WBXMLArchive::TITLE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
                writeString(tmpStr) != SYNCML_DM_SUCCESS ||
                writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
            return SYNCML_DM_IO_FAILURE;
    }

#ifndef DM_IGNORE_TSTAMP_AND_VERSION
    {
        DMNode * pNode = (DMNode*)node;
    
        UINT8 timeStamp[ sizeof(XPL_CLK_CLOCK_T) ];
        
        XPL_CLK_CLOCK_T tStamp = pNode->GetTStamp(NULL);
        
        for ( UINT32 i = 0; i < sizeof(XPL_CLK_CLOCK_T); i++ )
          timeStamp[i] = (UINT8)(tStamp >> (i*8));
        
        if(writeByte(SyncML_DM_WBXMLArchive::TSTAMP_INT_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
                writeOpaque(timeStamp, sizeof(timeStamp) ) != SYNCML_DM_SUCCESS ||
                writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
            return SYNCML_DM_IO_FAILURE;
   
        if(writeByte(SyncML_DM_WBXMLArchive::VERSION_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS)
           return SYNCML_DM_IO_FAILURE;
        /* Guarantee Big Endian storage of a UINT16 */
        UINT16 wVerNo = pNode->GetVerNo(NULL);
        UINT8 verNo[2] = { static_cast<UINT8>(wVerNo >> 8),
                           static_cast<UINT8>(wVerNo & 0xFF) };
        if(writeOpaque(verNo, sizeof(verNo)) != SYNCML_DM_SUCCESS ||
           writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
           return SYNCML_DM_IO_FAILURE;
   }      
    
#endif

    return SYNCML_DM_SUCCESS;
}
/*==================================================================================================
 
Function:    SyncML_DM_WBXMLWriter::operator new
 
Description: Allocate memory for this object
 
Memory policy: The caller is responsible to delete (free) the new object (alloc'd memory)
 
==================================================================================================*/
void *
SyncML_DM_WBXMLWriter::operator new(size_t sz) {
    return (DmAllocMem(sz));
}

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLWriter::operator delete
 
Description: De-allocate memory for this object
 
==================================================================================================*/
void
SyncML_DM_WBXMLWriter::operator delete (void *buf) {
    DmFreeMem(buf);
}
