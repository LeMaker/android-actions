#ifdef LOB_SUPPORT
//------------------------------------------------------------------------
//
//   Module Name: dm_tree_default_ESN_class.cc
//
//   General Description:Contains the implementations of the methods of
//                       DMDefaultESN class.
//------------------------------------------------------------------------
// Revision History:
//                     Modification   Tracking
// Author (core ID)       Date         Number    Description of Changes
//c23495               11/29/2006    libgg67059             LOB support
// cdp180              03/16/2007    LIBll55345   Removing ACL check for internal calls                                  
// -----------------  ------------   ----------  -------------------------
// Portability: This module is portable to other compilers. 
//------------------------------------------------------------------------
//                          INCLUDE FILES
//------------------------------------------------------------------------
#include "dmdefs.h"
#include "dm_tree_default_ESN_class.H" //header file for class defn
#include "dm_tree_util.h"                    //FillgetRetData
#include "xpl_File.h"
#include "dm_uri_utils.h"

//------------------------------------------------------------------------
//                     LOCAL FUNCTION PROTOTYPES
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                          LOCAL CONSTANTS
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//               LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//                            LOCAL MACROS
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                           LOCAL VARIABLES
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//                          GLOBAL VARIABLES
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                           LOCAL FUNCTIONS
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                          GLOBAL FUNCTIONS
//------------------------------------------------------------------------
//------------------------------------------------------------------------

//default constructor
DMDefaultESN::DMDefaultESN(CPCHAR pbFileName):DMDefaultLeafNode()
{
   abStorageName = NULL;
   abOriginalName =  (pbFileName != NULL) ? pbFileName : NULL;

   totalSize = 0L;
   m_bSetComplete = TRUE;
   m_bDirty = FALSE;
   m_bNeedLogging = FALSE;

   fileHandle = NULL;
   offset = 0L;
   // Mark as ESN
   SetESN();
}
// Destructor for ESN
DMDefaultESN::~DMDefaultESN()
{
	CloseInternalFile();
	abStorageName = NULL;
   	abOriginalName = NULL;
}
// Get internal storage file name
CPCHAR DMDefaultESN::GetInternalStorageFileName(void) const
{
	return (abStorageName != NULL) ? abStorageName.c_str() : NULL;

}
CPCHAR DMDefaultESN::GetOriginalInternalFileName(void) const
{
	return (abOriginalName != NULL) ? abOriginalName.c_str() : NULL;
}
/*==================================================================================================
  * Function: : Close intenal storage file
  * param:
  * return : Return SYNCML_DM_SUCCESS if file open successfull and others in case of error 
*==================================================================================================
*/
SYNCML_DM_RET_STATUS_T DMDefaultESN::CloseInternalFile(void)
{
  SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
  if (fileHandle != NULL)
 { 
     fileHandle->close();
     delete fileHandle;
     fileHandle = NULL;
  }
  return retStatus;
}
/*==================================================================================================
  * Function: : Open intenal storage file
  * param:
  * return : Return SYNCML_DM_SUCCESS if file open successfull and others in case of error 
*==================================================================================================
*/

SYNCML_DM_RET_STATUS_T DMDefaultESN::OpenInternalStorageFile()  
{
  SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
  // If the file is not opened before
  if(fileHandle == NULL) 
  {
   DMString abESNFileName;
 
   if(abStorageName.length() != 0)
   	abESNFileName = abStorageName;
   else
	  abESNFileName = abOriginalName;

   if(abESNFileName.length() != 0) {
	
    INT32 modeFlag = XPL_FS_FILE_RDWR;
    // If file does not exist use write mode instead of read/write to prevent file I/O error
    if (!XPL_FS_Exist(abESNFileName.c_str()))
    {
		modeFlag = XPL_FS_FILE_WRITE;
    }
    fileHandle = new DMFileHandler(abESNFileName.c_str(), FALSE);
    if (fileHandle == NULL)
       return SYNCML_DM_IO_FAILURE;
    if (fileHandle->open(modeFlag) != SYNCML_DM_SUCCESS)
    {
       fileHandle->deleteFile();
       delete fileHandle;
       fileHandle = NULL;
       return SYNCML_DM_IO_FAILURE;
    }
    totalSize = fileHandle->size();
   }
   else
   	totalSize = 0;	
  }
   return retStatus;
}
//------------------------------------------------------------------------
// FUNCTION        : Add
// DESCRIPTION     : This function sets the ACCESS type and format
//                   properties for an Interior node added in the tree.
//                   No data involved ,so synchronous Non-blocking call
//                   callback function pointers will be ignored
// ARGUMENTS PASSED: *psAdd,
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : status code
// PRE-CONDITIONS  : 1.URI has already been validated
//                   2.The node object of the plug-in class has been
//                     created and a reference has been given to DMTNM
// POST-CONDITIONS : Following property values are set by the function
//                   Format = SYNCML_DM_FORMAT_NODE
// IMPORTANT NOTES : The default Leaf node class grants all Access
//                   rights to the Node in Add
//                   If a Plug-in needs to implement a class for interior
//                   nodes in which the access type needs to be plug-in
//                   specific Ex: that plug-in does not want to give
//                   delete access type, then the plug-in SHALL implement
//                   it's own Add accordingly.
// REQUIREMENT #   : ESR-DMTNM0042-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultESN::Add(DMAddData & oAddData)
{
   return set(&oAddData);
}

//------------------------------------------------------------------------
// FUNCTION        : Delete
// DESCRIPTION     : This function should not actually not get called.
//                   since DMTNM deletes the node object.It is a pure
//                   virtual function in DMNode,hence is implemented and
//                   returns SYNCML_DM_SUCCESS
// ARGUMENTS PASSED: waitMsgForStatus
//                   replyStatusCback
//                   dwCommandId,
//                   bItemNumber,
//                   *pbUri,
//                   oIsThisAtomic,
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : ALWAYS returns
//                   SYNCML_DM_COMMAND_NOT_ALLOWED
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS : DMTNM actually deletes the node object
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM0025-m to ESR-DMTNM0027-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultESN::Delete(CPCHAR pbUri)
{
  SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;

  // Remoeve temporary file
  if(m_bDirty)
  {
  retStatus = OpenInternalStorageFile();
  if(retStatus != SYNCML_DM_SUCCESS)
	  return retStatus;
  
  retStatus = fileHandle->deleteFile();
  if(retStatus != SYNCML_DM_SUCCESS)
	  return retStatus;
  delete fileHandle;
  fileHandle = NULL;
  }
  return CloseInternalFile();
}


//------------------------------------------------------------------------
// FUNCTION        : Get
// DESCRIPTION     : 
// ARGUMENTS PASSED: waitMsgForGetdata
//                   waitMsgForGetdata
//                   dwCommandId,
//                   bItemNumber,
//                   *pbUri,
//                   dwStartByte,
//                   dwNBytes
//                   **ppsReturnData
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : ALWAYS returns
//                   SYNCML_DM_COMMAND_NOT_ALLOWED
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS : 
// IMPORTANT NOTES : The DMTNM will return the list of child names(if 
//                   the interior node has no children it will return 
//                   an empty list
// REQUIREMENT #   : ESR-DMTNM0028-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultESN::Get(CPCHAR pbUri, DMGetData & oReturnData)
{
  	SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
	// Set ESN flag
	oReturnData.SetESN(TRUE);

	if(oReturnData.chunkData != NULL)
	{
		if(m_bSetComplete == FALSE)
			return SYNCML_DM_ESN_SET_NOT_COMPLETE;
		
		offset = oReturnData.m_chunkOffset;
		if(oReturnData.m_chunkOffset ==0)
			retStatus = GetFirstChunk(*oReturnData.chunkData);
		else
			retStatus = GetNextChunk(*oReturnData.chunkData);
	}
	else
	{	
		retStatus = OpenInternalStorageFile();
		if(retStatus != SYNCML_DM_SUCCESS)
			return retStatus;
		retStatus = DMDefaultLeafNode::Get(pbUri, oReturnData);
		oReturnData.m_TotalSize = totalSize;
	}
	return retStatus;
}
	

//------------------------------------------------------------------------
// FUNCTION        : GetFormat
//
// DESCRIPTION     : This function returns FORMAT of the node
// ARGUMENTS PASSED: waitMsgForGetFormat,
//                   replyGetFormatCback,
//                   *pbUri,
//                   **pRetPropertyData
// RETURN VALUE    : ALWAYS returns SYNCML_DM_FORMAT_NODE
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM0033-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultESN::GetFormat(CPCHAR pbUri,
                                                       SYNCML_DM_FORMAT_T *dwpRetPropertyData)
{
   *dwpRetPropertyData = this->bFormat;
   return(SYNCML_DM_SUCCESS);
}

//------------------------------------------------------------------------
// FUNCTION        : GetType
// DESCRIPTION     : This function returns MIME type
// ARGUMENTS PASSED: waitMsgForGetFormat,
//                   replyGetFormatCback,
//                   *pbUri,
//                   **pRetPropertyData
// RETURN VALUE    : status code,type is NULL for interior nodes 
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES : 
// REQUIREMENT #   : ESR-DMTNM0033-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultESN::GetType(CPCHAR pbUri,
                                                     DMString& strType)
{
   strType = this->getType();
   return SYNCML_DM_SUCCESS;
}

//------------------------------------------------------------------------
// FUNCTION        : GetSize
// DESCRIPTION     : This function returns SIZE of the node.
// ARGUMENTS PASSED: waitMsgForGetFormat,
//                   replyGetFormatCback,
//                   *pbUri,
//                   **pRetPropertyData
// RETURN VALUE    : 
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : 
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultESN ::GetSize(CPCHAR pbUri,
                                                     UINT32 *dwpRetPropertyData)
{
  SYNCML_DM_RET_STATUS_T retStatus = OpenInternalStorageFile();
  if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;
   *dwpRetPropertyData= totalSize;
   return(SYNCML_DM_SUCCESS);
}

//------------------------------------------------------------------------
// FUNCTION        : Rename
// DESCRIPTION     : This function is called when a node's name has
//                   been changed
// ARGUMENTS PASSED: *pbUri,
//                   pNewNodeName
// RETURN VALUE    : ALWAYS returns SYNCML_DM_SUCCESS
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES : The node Name will be renamed by the DMTNM.It informs
//                   the plug-in about this to allow the plug-in to change
//                   the name in the data-base correspondingly.Since
//                   Interior nodes in this class have no database this
//                   method simply returns SYNCML_DM_SUCCESS.This means that
//                   name of node will be renamed by DMTNM,and plug-in
//                   does not have anything specific to do.
// REQUIREMENT #   : 
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultESN::Rename(CPCHAR pbUri,
                                                    CPCHAR pNewNodeName)
{
   return(SYNCML_DM_SUCCESS);
}

//------------------------------------------------------------------------
// FUNCTION        : Replace
//
// DESCRIPTION     : This function is called when a node's value has
//                   to be replaced.
// ARGUMENTS PASSED: waitMsgForStatus,
//                   replyStatusCback,
//                   dwCommandId,
//                   bItemNumber,
//                   *pReplace,
//                   oMoreData
//                   oIsThisAtomic
// RETURN VALUE    : 
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM0024-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultESN::Replace(DMAddData & oReplace)
{
	SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
	if(oReplace.chunkData != NULL)
	{
		offset = oReplace.m_chunkOffset;

		if(oReplace.m_chunkOffset ==0)
		{ 
			retStatus = CloseInternalFile();
			if(retStatus != SYNCML_DM_SUCCESS)
			  	return retStatus;

			retStatus = SetFirstChunk(*oReplace.chunkData);
			if(retStatus != SYNCML_DM_SUCCESS)
			  	return retStatus;
		 	if(oReplace.IsLastChunk())
			{  m_bSetComplete = TRUE;
			   retStatus = CloseInternalFile();
		 	}
		}
		else
		{
		    if(m_bDirty)
		    {  if(oReplace.IsLastChunk())
				retStatus = SetLastChunk(*oReplace.chunkData);
			else
				retStatus = SetNextChunk(*oReplace.chunkData);
		    }
		    else
		      retStatus =  SYNCML_DM_INVALID_PARAMETER;
		}
	}
	else
	   retStatus = DMDefaultLeafNode::Replace(oReplace);
	return retStatus;
}
//------------------------------------------------------------------------
// FUNCTION        : Commit
//
// DESCRIPTION     : This function The method will commit  operations on the node
// ARGUMENTS PASSED: waitMsgForStatus,
//
// RETURN VALUE    : 
// POST-CONDITIONS :
// IMPORTANT NOTES :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultESN::Commit()
{
 SYNCML_DM_RET_STATUS_T retStatus = CloseInternalFile();

 if(retStatus != SYNCML_DM_SUCCESS)
 	return retStatus;

  // Is temporary file created?
  if(m_bDirty)
  {
	 // Check if operations to the node is complete
	 if(m_bSetComplete)
	 {
	   // Newly created node
	   if(abOriginalName == NULL)
	  	 abOriginalName.RemoveSufix(abStorageName.c_str(), SYNCML_DM_DOT);

	   abStorageName = NULL;
	   m_bDirty = FALSE;
	   m_bSetComplete = TRUE;
	 }
	 else
	 {
		retStatus	 = SYNCML_DM_INCOMPLETE_COMMAND;
	 }
 }
 return retStatus;

}

//------------------------------------------------------------------------
// FUNCTION        : Rollback
//
// DESCRIPTION     : This function is called when a node's value has
//                   needs to be rolledback. NOT SUPPORTED FOR PHASE 1
// ARGUMENTS PASSED: waitMsgForStatus,
//                   replyStatusCback,
//                   dwCommandId,
//                   bItemNumber,
//                   dmCommand
//                   *pbUri,
// RETURN VALUE    : returns SYNCML_DM_FEATURE_NOT_SUPPORTED
// PRE-CONDITIONS  : 1.URI has already been validated
// POST-CONDITIONS :
// IMPORTANT NOTES :
// REQUIREMENT #   : ESR-DMTNM0037-m
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DMDefaultESN::Rollback()
{
   SYNCML_DM_RET_STATUS_T retStatus = CloseInternalFile();
   if(retStatus != SYNCML_DM_SUCCESS)
	   return retStatus;

  if(m_bDirty)
  {	// Are there any temoprary file?
	 if( abStorageName.length() != 0)
	 {  	
	 	SYNCML_DM_RET_STATUS_T retStatus = OpenInternalStorageFile();
		 if(retStatus != SYNCML_DM_SUCCESS)
		 return retStatus;

 	   	// Always delete the temporary file
	    	if(fileHandle != NULL)
	   	{ 	  retStatus = fileHandle->deleteFile();
			  delete fileHandle;
			  fileHandle = NULL;
	   	}
	}
	// Clear the temporary file name
	abStorageName = NULL;
   	m_bDirty = FALSE;
   	m_bSetComplete = TRUE;
  }
 return retStatus;
}
/*==================================================================================================
  * Function: :Get the first chunk 
  * param dmtChunkData  -- reference to DmtDataChunk
  * return status code
*==================================================================================================
*/
SYNCML_DM_RET_STATUS_T DMDefaultESN::GetFirstChunk(DmtDataChunk&  chunkData)
{
  	SYNCML_DM_RET_STATUS_T retStatus = OpenInternalStorageFile();
	UINT32 remainlLen = totalSize- offset;
	UINT32 getLen = 0L;
	UINT8 *bufp;
	if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;

	if(fileHandle == NULL)
	{
		chunkData.SetChunkData(NULL, 0L);
		chunkData.SetReturnLen(getLen);
		return retStatus;
	}
	chunkData.GetChunkData(&bufp);
	if(remainlLen <0|| bufp == NULL)
		return SYNCML_DM_INVALID_PARAMETER;

	if(remainlLen == 0)
	{
		chunkData.SetChunkData(NULL, 0L);
		chunkData.SetReturnLen(remainlLen);
		return SYNCML_DM_SUCCESS;
	}

	getLen = chunkData.GetChunkSize();
	if(getLen > remainlLen)
		getLen = remainlLen;

	chunkData.GetChunkData(&bufp); 	// the chunk data is available 	
	if(bufp == NULL)
		return SYNCML_DM_INVALID_PARAMETER;
    	if(fileHandle->seek(XPL_FS_SEEK_SET, offset) != SYNCML_DM_SUCCESS) 
	        return  SYNCML_DM_IO_FAILURE;
    	if(fileHandle->read(bufp, getLen) != SYNCML_DM_SUCCESS) 
	        return  SYNCML_DM_IO_FAILURE;
	// Set data size		
	chunkData.SetChunkData(NULL, getLen);
	// Set return data size
	chunkData.SetReturnLen(getLen);
	return retStatus;
}

/*==================================================================================================
  * Function: :Get the first chunk 
  * param dmtChunkData  -- reference to DmtDataChunk
  * return status code
*==================================================================================================
*/

SYNCML_DM_RET_STATUS_T DMDefaultESN::GetNextChunk(DmtDataChunk& chunkData)
{
	return GetFirstChunk(chunkData);
}
/*==================================================================================================
  * Function: :set the first chunk 
  * param dmtChunkData  -- reference to DmtDataChunk
  * return status code
*==================================================================================================
*/
SYNCML_DM_RET_STATUS_T DMDefaultESN::SetFirstChunk(DmtDataChunk& chunkData)  
{
	SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
	UINT32 dataLen;
	UINT8 *bufp;
    LOGD("DMDefaultESN::SetFirstChunk");
    	
	// No internal file created yet
	if(abStorageName.length() == 0) {
	   	retStatus = DMFileHandler::createTempESNFileName(abStorageName);
		if(retStatus != SYNCML_DM_SUCCESS)
			return retStatus;
		if(offset == 0L)
		{	m_bNeedLogging = TRUE;
			m_bSetComplete = FALSE;
		}

	}
	else// Replace previous data
		{
			// Set first trunk
			if(offset == 0L)
			{	totalSize = 0L;
   				m_bSetComplete = FALSE;
				m_bNeedLogging = TRUE;

				// Remove the current data file
  				retStatus = OpenInternalStorageFile();
				if(retStatus != SYNCML_DM_SUCCESS)
					return retStatus;

				retStatus = fileHandle->deleteFile();
				if(retStatus != SYNCML_DM_SUCCESS)
					return retStatus;
				delete fileHandle;
				fileHandle = NULL;
			}
			else
			{	m_bNeedLogging = FALSE;
			}
	}
	chunkData.GetChunkDataSize(dataLen); 
	if(dataLen != 0)
	{	retStatus = OpenInternalStorageFile();
		if(retStatus != SYNCML_DM_SUCCESS)
			return retStatus;

		chunkData.GetChunkData(&bufp); 	// the chunk data is available 	
		if(fileHandle == NULL  ||bufp == NULL)
			return SYNCML_DM_INVALID_PARAMETER;
		
		XPL_FS_SIZE_T fsize = fileHandle->size();
        LOGD("fileHandle file size=%d", fsize);

        // ehb005: no need to seek since we're writing and always appending to end of file
    	//if(fileHandle->seek(XPL_FS_SEEK_SET, offset) != SYNCML_DM_SUCCESS) 
	    //    return  SYNCML_DM_IO_FAILURE;
	    	if(fileHandle->write(bufp, dataLen) != SYNCML_DM_SUCCESS) 
		        return  SYNCML_DM_IO_FAILURE;
	}

	totalSize = offset + dataLen;
	chunkData.SetReturnLen(dataLen);
	m_bDirty = TRUE;
	LOGD("dataLen=%d, totalSize=%d, offset=%d", dataLen, totalSize, offset);
    LOGD("DMDefaultESN::SetFirstChunk abStorageName=%s", abStorageName.c_str());

    return retStatus;
}
/*==================================================================================================
  * Function: :set the next chunk 
  * param dmtChunkData  -- reference to DmtDataChunk
  * return status code
*==================================================================================================
*/
SYNCML_DM_RET_STATUS_T DMDefaultESN::SetNextChunk(DmtDataChunk& chunkData)
{
    LOGD("DMDefaultESN::SetNextChunk");
 	return SetFirstChunk(chunkData);
}

/*==================================================================================================
  * Function: :set the last chunk 
  * param dmtChunkData  -- reference to DmtDataChunk
  * return status code
*==================================================================================================
*/
SYNCML_DM_RET_STATUS_T DMDefaultESN::SetLastChunk(DmtDataChunk& chunkData)
{
    LOGD("DMDefaultESN::SetLastChunk");
	SYNCML_DM_RET_STATUS_T retStatus = SetFirstChunk(chunkData);;
	if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;

	m_bSetComplete = TRUE;
	retStatus = CloseInternalFile();
	return retStatus;
}
#endif

