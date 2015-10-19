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

#include "SyncML_DM_WBXMLArchive.H"
#include "SyncML_DM_Reader.H"
#include "xpl_Logger.h"
#include "dm_tree_class.H"
#include "dmprofile.h"
#include "dm_uri_utils.h"

/* The DEFAULT_FILE_HEADER field represents the start of a standard WBXML document
 * for standards compliance and for the purpose of file size efficiency.
 *
 * The first 3 bytes are WBXML standard values:
 *     version is always 1 byte.
 *     publicid - We assume the public ID is <=127, which only takes 1 byte
 *                for its mb_uint32 encoding.
 *     charset - 0x6A is the code for UTF-8; 1 byte for mb_uint32 encoding
 * The 4th byte is the mb_uint32 length of the optional string table that follows;
 * we will not have a string table, so the length is zero and no table follows.
 *
 * The purpose of the string table is to store common strings that occur multiple
 * times in the document, and thus save space by referring to them. This is a
 * possible future enhancement.
 */
const UINT8 SyncML_DM_WBXMLArchive::DEFAULT_FILE_HEADER[] = {
      WBXML_VERSION, PUBLIC_ID, CHARSET, 0x00
  };

/* The wchar_t standard and handy L"string"; usage are not supported by the ARM compiler.*/
const char SyncML_DM_WBXMLArchive::FILE_EXTENSION[] = ".wbxml";

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLArchive::SyncML_DM_WBXMLArchive 
 
Description: Constructor method that creates a log and sets up the path and uri of the archive
 
==================================================================================================*/
SyncML_DM_WBXMLArchive::SyncML_DM_WBXMLArchive(CEnv* env, CPCHAR pURI, CPCHAR path)
        : SyncML_DM_Archive( env, pURI, path),
          m_pEnv( env )
{
    lastSavedTime = 0;
#ifdef LOB_SUPPORT  
    commitLog = NULL;
#endif

    if (!m_strWFSFileName.empty() && m_strWFSFileName.length() > 0) {
        m_permission = 0;
        // initialize permission mask
        if (XPL_FS_CheckPermission(m_strWFSFileName, XPL_FS_RDONLY_MODE)) {
            m_permission = XPL_FS_RDONLY_MODE;
        }
        if (XPL_FS_CheckPermission(m_strWFSFileName, XPL_FS_RDWR_MODE)) {
            m_permission |= (XPL_FS_RDWR_MODE|XPL_FS_WRONLY_MODE|XPL_FS_RDWR_MODE|
                             XPL_FS_CREAT_MODE|XPL_FS_TRUNC_MODE|XPL_FS_APPEND_MODE);
        }                    
    }
    XPL_LOG_DM_TMN_Debug(("m_permission initialized to %d, path %s, uri=%s\n",  m_permission, m_strWFSFileName.c_str(), pURI ));
}

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLArchive::~SyncML_DM_WBXMLArchive
 
Description: The SyncML_DM_WBXMLArchive class deconstructor.
 
==================================================================================================*/
SyncML_DM_WBXMLArchive::~SyncML_DM_WBXMLArchive() 
{
#ifdef LOB_SUPPORT  
  // Remove  commit log handler
  if(commitLog != NULL)
  {
	commitLog->CloseLog();
	delete commitLog;
	commitLog = NULL;
  }
#endif
}

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLArchive::serialize
 
Description: Serialization method takes a tree as an argument serializes to the file system 
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_WBXMLArchive::serialize(DMTree* tree) 
{
    SYNCML_DM_SERIALIZATION_STATUS_T ser_ret_stat;

#ifdef DM_PROFILER_ENABLED
    DMString strCaption = "serialize "; 
    strCaption += getURI(); strCaption += ", \""; 
    strCaption += m_path; strCaption += "\"";
    DM_PROFILE( strCaption );
#endif

    /* Create the path string for the temp file */
    if (this->rootTreeNode==NULL)
        return SYNCML_DM_SKIP_SUBTREE;

    tree->InitSerializationList(this->rootTreeNode);

    /* Path + Extension + null */
    DMString strTempFilePath = m_strWFSFileName;
    strTempFilePath += DMFileHandler::TEMP_FILE_EXTENSION;

    /* Acquire a handle representation of the file */
    DMFileHandler fileHandle(strTempFilePath.c_str());

    /* Open the file for writing */
    
    SYNCML_DM_RET_STATUS_T retstat = fileHandle.open(XPL_FS_FILE_WRITE);

    if(retstat == SYNCML_DM_COMMAND_NOT_ALLOWED) {
        return retstat;
    }
    else if (retstat != SYNCML_DM_SUCCESS) {
        return SYNCML_DM_IO_FAILURE;
    }

    /* Create a WBXMLWriter utility class for handling the data */
    SyncML_DM_WBXMLWriter writer(&fileHandle);

    /* Write the default file header, which contains the WBXML version, public ID,
       charset (UTF-8), lookup table length, and lookup table */
    if(writer.writeData((UINT8 *)DEFAULT_FILE_HEADER,
                        (UINT8)sizeof(DEFAULT_FILE_HEADER)) != SYNCML_DM_SUCCESS) {
        return SYNCML_DM_IO_FAILURE;
    }

    /* Write the MgmtTree start tag */
    if(writer.writeByte(SyncML_DM_WBXMLArchive::TREE_START_TAG |
                        SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS) {
        return SYNCML_DM_IO_FAILURE;
    }

    /* Create a URI object for the currURI
      * For the purposes of conserving memory allocation cycles, we assume the worst
      * case URI length + 1 for the nil terminator, and we will reuse this same
      * buffer as the loop runs, putting in a nil terminator where its needed.
      */
    DMNode * pRetNode = NULL;
    /* Loop on the nodes returned by the tree and node manager */
    INT32 nEndTagsNumber = 0;
    while((ser_ret_stat = tree->GetSerializationListNextItem(&pRetNode, nEndTagsNumber)) == SYNCML_DM_SERIALIZATION_SUCCESS) 
    {

        // write "end" tags
        while( nEndTagsNumber > 0 ) {
          nEndTagsNumber--;
          
          if(writer.writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS) 
              return  SYNCML_DM_IO_FAILURE;
        }
#ifdef LOB_SUPPORT  
	// Special case for ESN
	if(pRetNode->IsESN())
	{
		SYNCML_DM_RET_STATUS_T retStatus;
		 // Convert to	ESN pointer
		DMDefaultESN *tempESN = reinterpret_cast< DMDefaultESN *>(pRetNode);
		// Close internal files
		retStatus = tempESN->CloseInternalFile();
		if(retStatus != SYNCML_DM_SUCCESS)
				return retStatus;
	}
#endif

        if(writer.writeNode(pRetNode) != SYNCML_DM_SUCCESS) {
            XPL_LOG_DM_TMN_Error((" SYNCML_DM_IO_FAILURE on %s\n",  (const char *)m_pURI ));
            return  SYNCML_DM_IO_FAILURE;
        }

        if ( pRetNode->opiInSync() ) 
        {
             UINT8 flag = pRetNode->getFlags(); 
             flag &= ~( DMNode::enum_NodeOPISyncUptodate );
             pRetNode->setFlags(flag);   
        }     

    }/* End while */

    /* Ensure the while() terminated as expected */
    if (ser_ret_stat == SYNCML_DM_TREE_TRAVERSING_OVER) {
        /* We have to write the end tag of the last node, as well as any
         * parent nodes that also ended. currURI tells up how many (one
         * for each / still in it)
         */
        while( nEndTagsNumber > 0 ) {
          nEndTagsNumber--;
        
          if(writer.writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS) 
              return SYNCML_DM_IO_FAILURE;
        }
    } else /* ser_ret_stat == SYNCML_DM_SERIALIZATION_FAIL */
    {
        fileHandle.close();
        return SYNCML_DM_FAIL;
    }
    /* Write the end tag for the root node, then end tag for the MgmtTree */
    UINT8 endTags[2] = { SyncML_DM_WBXMLArchive::END_TAG, SyncML_DM_WBXMLArchive::END_TAG };
    
    if(writer.writeData(&endTags[0], 2) != SYNCML_DM_SUCCESS) 
    {
        fileHandle.close();
        return SYNCML_DM_IO_FAILURE;
    }
    /* Close the file */
    if (fileHandle.close() != SYNCML_DM_SUCCESS)
        return SYNCML_DM_IO_FAILURE;

    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================
 
  
Function:    SyncML_DM_WBXMLArchive::deserialize
 
Description: Deserializes a tree in the filesystem to the memory tree
 
==================================================================================================*/

SYNCML_DM_RET_STATUS_T 
SyncML_DM_WBXMLArchive::deserialize(DMTree * pTree,
                                    BOOLEAN bIsReload) 
{
    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;

#ifdef DM_PROFILER_ENABLED
    DMString strCaption = "deserialize "; 
    strCaption += getURI(); strCaption += ", \""; 
    strCaption += m_path; strCaption += "\"";
    DM_PROFILE( strCaption );
#endif

    if ( !rootTreeNode )
        return SYNCML_DM_FAIL;

    oEventLogger.Reset();
  
    if ( !rootTreeNode->IsSkeletonNode() ) 
    {
        if ( bIsReload )  
	  m_pTree->UnloadArchive(rootTreeNode);
        else 
	  return SYNCML_DM_SUCCESS;  
    }

    DMPluginManager & oPluginManager = m_pTree->GetPluginManager();

    ret_stat = deserializeFile( pTree, m_strWFSFileName, true );

    if ( ret_stat == SYNCML_DM_FILE_NOT_FOUND )
        setWritableExist( FALSE );
    else 
    {
        setWritableExist( TRUE );
        return oPluginManager.UpdatePluginNodes(m_pURI);
    }

    for ( int nFS = 0; nFS < m_pEnv->GetRFSCount(); nFS++ ) 
    {
        DMString sFile;
        m_pEnv->GetRFSFullPath(nFS,m_path,sFile);
        if ( deserializeFile( pTree, sFile, false ) == SYNCML_DM_SUCCESS )
            ret_stat = SYNCML_DM_SUCCESS; // at least one file was successfully loaded
    }

   return oPluginManager.UpdatePluginNodes(m_pURI);
}

SYNCML_DM_RET_STATUS_T SyncML_DM_WBXMLArchive::deserializeFile(
    DMTree* pTree, 
    CPCHAR szFileName,
    BOOLEAN bWFS)
{
    if (!XPL_FS_Exist(szFileName))
        return SYNCML_DM_FILE_NOT_FOUND; 
  
    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;

    /* Create a file handle for the file to be deserialized */
    DMFileHandler fileHandle(szFileName);
    SYNCML_DM_RET_STATUS_T openstat = fileHandle.open(XPL_FS_FILE_READ);

    /* If file cannot be opened, leave now */
    if(openstat == SYNCML_DM_COMMAND_NOT_ALLOWED) {
        return openstat;
    }
    else if (openstat != SYNCML_DM_SUCCESS) {
        return SYNCML_DM_FILE_NOT_FOUND;    /* Assume this is the underlying reason */
    }

    m_permission |= XPL_FS_RDONLY_MODE;
    
    /* Create a reader utility class for the reading of tree data */
    SyncML_DM_WBXMLReader reader(&fileHandle);

    /* Working variable */
    UINT8 bYte;

    /* Read a WBXML header from the file */
    if(reader.readHeader() != SYNCML_DM_SUCCESS) 
    {
        fileHandle.close();
        return SYNCML_DM_IO_FAILURE;
    }

    /* Read and verify the MgmtTree start tag */
    if(reader.readByte(&bYte) != SYNCML_DM_SUCCESS) 
    {
        fileHandle.close();
        return SYNCML_DM_IO_FAILURE;
    }
    if(bYte != (TREE_START_TAG | TAG_CONTENT_MASK)) 
    {
        fileHandle.close();
        return SYNCML_DM_IO_FAILURE;
    }

    /* Read the first byte of data, expected to be END_TAG or NODE_START_TAG.
    * The byte read will be parsed in the upcoming loop. 
    * If the byte is not the NODE_START_TAG with content, there is no 
    * tree to parse or there is an error (we assume error here).
    */
    if(reader.readByte(&bYte) != SYNCML_DM_SUCCESS
          || bYte != (SyncML_DM_WBXMLArchive::NODE_START_TAG
                      | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK)) 
    {
        fileHandle.close();
        return SYNCML_DM_IO_FAILURE;
    }
    
    /* The properties data structure */
    DMAddNodeProp props;

    DMNode* pNode = rootTreeNode->GetParent(); // can be null for '.'; otherwise is valid parent

    /* Loop until an internal break occurs (hopefully, not until the last END_TAG */
    while(1) 
    {
        /* If the read byte is an END_TAG... */
        if(bYte == END_TAG) 
        {
          
            if ( pNode == rootTreeNode )
                break; // end of subtree
            
            if ( !pNode || !pNode->GetParent() ){
                XPL_LOG_DM_TMN_Error((" ! Unexpected condition (node is null) inside deserialize\n"));
                fileHandle.close();
                return SYNCML_DM_TREE_CORRUPT;  // unexpected condition - looks like extra "end tag"
            }
            
            pNode = pNode->GetParent();

            /* Get the next byte after the END_TAG */
            ret_stat=reader.readByte(&bYte);
            if(ret_stat != SYNCML_DM_SUCCESS) {
                XPL_LOG_DM_TMN_Error((" ! Error ret_stat=%d\n", ret_stat));
                fileHandle.close();
                return SYNCML_DM_IO_FAILURE;
            }
        } 
        else 
            if(bYte == (NODE_START_TAG | TAG_CONTENT_MASK)) 
            {

                /* This method reads in the node property data as well as
                * the byte that signaled the end of the node.
                * It allocates a props structure we are responsible for,
                * It also sets props->pbURI to NULL.
                */
                if(reader.readNode(&props, &bYte) != SYNCML_DM_SUCCESS) 
                {
                    fileHandle.close();
                    return SYNCML_DM_IO_FAILURE;
                }

                /* Add the node to the tree */
           
                ret_stat = pTree->AddNode(&pNode, props);

                if (ret_stat != SYNCML_DM_SUCCESS) 
                    break;

            } 
            else
            {
                /* Invalid byte read */
                ret_stat = SYNCML_DM_TREE_CORRUPT;
                break;
            }
    }/* End while */

    fileHandle.close();
    XPL_LOG_DM_TMN_Debug(("End of deserialize ret_stat=%d\n", ret_stat));

    //set time stamp to emmory file
    if ( bWFS )
        serializeDone();

    return ret_stat;
}

/*==================================================================================================
 
Function:    SyncML_DM_WBXMLArchive::getLastModifiedTime
 
Description: Retrieves the last modification time of the archive
 
==================================================================================================*/

XPL_CLK_CLOCK_T
SyncML_DM_WBXMLArchive::getLastModifiedTime() 
{
    XPL_CLK_CLOCK_T lastModified=XPL_FS_GetModTime(m_strWFSFileName);
    return lastModified;
}

XPL_CLK_CLOCK_T SyncML_DM_WBXMLArchive::getLastSavedTime() 
{
    return lastSavedTime;
}

void SyncML_DM_WBXMLArchive::serializeDone() 
{
    lastSavedTime=XPL_FS_GetModTime(m_strWFSFileName);
    oEventLogger.OnTreeSaved();
}

// returns TRUE if permission is allowed
BOOLEAN 
SyncML_DM_WBXMLArchive::verifyPermission(XPL_FS_OPEN_MODE_T permission) const
{
    // XPL_LOG_DM_TMN_Debug(("m_permission=%d, filename=%s\n", m_permission, m_strWFSFileName.c_str()));
    return (m_permission & permission);
}


#ifdef LOB_SUPPORT
/*==================================================================================================
 
Function:    SyncML_DM_WBXMLArchive::commitESN
 
Description: Commit all changes for ESN   
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SyncML_DM_WBXMLArchive::commitESN(DMTree* tree)
{
    SYNCML_DM_SERIALIZATION_STATUS_T ser_ret_stat;
    SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;

   //Remove commit log file
   if(commitLog != NULL)
   {
    //Commit log file name =  Path + "commit" + null 
    DMString strTempFilePath = m_strWFSFileName;
    strTempFilePath += DMFileHandler::COMMIT_LOG_EXTENSION;
    commitLog->playLog(strTempFilePath.c_str());
    delete  commitLog;
    commitLog  = NULL;

   /* Create the path string for the temp file */
    if (this->rootTreeNode==NULL)
        return SYNCML_DM_SKIP_SUBTREE;

    tree->InitSerializationList(this->rootTreeNode);

    DMNode * pRetNode = NULL;
    /* Loop on the nodes returned by the tree and node manager */
    INT32 nEndTagsNumber = 0;
    while((ser_ret_stat = tree->GetSerializationListNextItem(&pRetNode, nEndTagsNumber)) == SYNCML_DM_SERIALIZATION_SUCCESS) 
    {
	// Special case for ESN
	if(pRetNode->IsESN())
	{
		 // Convert to  ESN pointer
		 DMDefaultESN *tempESN = reinterpret_cast< DMDefaultESN *>(pRetNode);
		// Call commit command handler
		retStatus = tempESN->Commit();
		if(retStatus != SYNCML_DM_SUCCESS)
			return retStatus;
	}
    }/* End while */
   }
   return SYNCML_DM_SUCCESS;
}
/*==================================================================================================
 
Function:    SyncML_DM_WBXMLArchive::rollback
 
Description: Rollback all changes for ESN   
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SyncML_DM_WBXMLArchive::rollbackESN(DMTree* tree)
{
    SYNCML_DM_SERIALIZATION_STATUS_T ser_ret_stat;
    SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;

   //Remove commit log file
   if(commitLog != NULL)
   {
	commitLog->RemoveLog();
	  delete  commitLog;
	  commitLog  = NULL;

  /* Create the path string for the temp file */
    if (this->rootTreeNode==NULL)
        return SYNCML_DM_SKIP_SUBTREE;

    tree->InitSerializationList(this->rootTreeNode);

    DMNode * pRetNode = NULL;
    /* Loop on the nodes returned by the tree and node manager */
    INT32 nEndTagsNumber = 0;
    while((ser_ret_stat = tree->GetSerializationListNextItem(&pRetNode, nEndTagsNumber)) == SYNCML_DM_SERIALIZATION_SUCCESS) 
    {
	// Special case for ESN
	if(pRetNode->IsESN())
	{
		 // Convert to  ESN pointer
		 DMDefaultESN *tempESN = reinterpret_cast< DMDefaultESN *>(pRetNode);
		// Call rollback command handler
		retStatus = tempESN->Rollback();
		if(retStatus != SYNCML_DM_SUCCESS)
			return retStatus;
	}
    }/* End while */
   }
   return SYNCML_DM_SUCCESS;
}
/*==================================================================================================
 
Function:    SyncML_DM_WBXMLArchive::GetCommitLogHandler
 
Description: Retrieves the commit log handler
 
==================================================================================================*/
SyncML_Commit_Log*  SyncML_DM_WBXMLArchive::GetCommitLogHandler()
{
  	if(commitLog == NULL)
	{	commitLog = new SyncML_Commit_Log();
		if(this->commitLog != NULL)
		{
			//Commit log file name =  Path + "commit" + null 
			DMString strTempFilePath = m_strWFSFileName;
			strTempFilePath += DMFileHandler::COMMIT_LOG_EXTENSION;
			if(this->commitLog->InitLog(strTempFilePath.c_str())  != SYNCML_DM_SUCCESS)
			{
				delete commitLog;
				commitLog = NULL;
			}
		}
  	}	
	return commitLog;
}
/*==================================================================================================
 
Function:    SyncML_DM_WBXMLArchive::CloseCommitLog
 
Description: Close the commit log file
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T  SyncML_DM_WBXMLArchive::CloseCommitLog()
{
  SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;
  if(commitLog != NULL)
 	ret_stat = commitLog->CloseLog();
  return ret_stat;
}
/*==================================================================================================
 
Function:    SyncML_DM_WBXMLArchive::PlayCommitLog
 
Description: Free the commit log handler
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T  SyncML_DM_WBXMLArchive::PlayCommitLog()
{
  SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;
  if(commitLog == NULL)
	  GetCommitLogHandler();
  if(commitLog != NULL)
  {
	commitLog->playLog();
	commitLog->RemoveLog();
	delete commitLog;
	commitLog = NULL;
  }
  return ret_stat;
}
#endif
