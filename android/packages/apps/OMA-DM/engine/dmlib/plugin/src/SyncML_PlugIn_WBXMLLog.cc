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

    File Name: SyncML_PlugIn_WBXMLLog.cc 

    General Description: This contains function implementations of the WBXMLLog class.

==================================================================================================*/

#include "dmStringUtil.h"
#include "SyncML_PlugIn_WBXMLLog.H"
#include "SyncML_DM_WBXMLArchive.H"
#include "dmtRWPlugin.hpp"
#include "dm_tree_plugin_util.H"
#include "dm_uri_utils.h"
#include "dm_tree_util.h"

#ifdef TEST_DM_RECOVERY
extern char power_fail_point[];
#endif

/*==================================================================================================

Function:    SyncML_PlugIn_WBXMLLog::SyncML_PlugIn_WBXMLLog

Description: Constructor for the Log object

==================================================================================================*/

SyncML_PlugIn_WBXMLLog::SyncML_PlugIn_WBXMLLog(const DmtRWPluginTree *pluginTree, const char * rootPath):SyncML_Log()
{
    m_strRootPath=rootPath;
    prevRecord = 0;
    this->pluginTree = pluginTree;
}
/*==================================================================================================

Function:    SyncML_PlugIn_WBXMLLog::~SyncML_PlugIn_WBXMLLog

Description: Destructor for the Log object

==================================================================================================*/
SyncML_PlugIn_WBXMLLog::~SyncML_PlugIn_WBXMLLog()
{
	UnInitLog();
}
/*==================================================================================================

Function:    SyncML_PlugIn_WBXMLLog::UnInitLog

Description: Uninitialize log

==================================================================================================*/
 SYNCML_DM_RET_STATUS_T SyncML_PlugIn_WBXMLLog::UnInitLog()
{
    if(reader != NULL)
    {
        delete reader;
        reader = NULL;
    }

    if(writer != NULL)
    {
        delete writer;
        writer = NULL;
    }
   return	 SYNCML_DM_SUCCESS;
}
/*==================================================================================================

Function:    SyncML_PlugIn_WBXMLLog::InitLog

Description: Open/Create log file

==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_PlugIn_WBXMLLog::InitLog(CPCHAR logFileName)
{
  SYNCML_DM_RET_STATUS_T dm_stat =  UnInitLog();;
  BOOLEAN  m_WriteHeader = FALSE;

   if ( dm_stat != SYNCML_DM_SUCCESS)
		 return dm_stat;

    if (!XPL_FS_Exist(logFileName))
        m_WriteHeader = TRUE;

   dm_stat = SyncML_Log::InitLog(logFileName);
   if ( dm_stat != SYNCML_DM_SUCCESS)
	   return dm_stat;

	// Write log file file header
    if(m_WriteHeader)
   	{
        // Go to end of log file
        if(this->fileHandle->seek(XPL_FS_SEEK_END, 0) != SYNCML_DM_SUCCESS) 
         {
		CloseLog();
         	dm_stat = SYNCML_DM_IO_FAILURE;
        }
        // Write log header
        prevRecord = this->fileHandle->position();
        if(prevRecord == 0)
        {
		if(writer == NULL)
		{  writer = new SyncML_DM_WBXMLWriter(this->fileHandle);
			   if( writer == NULL)
			   return SYNCML_DM_DEVICE_FULL;
		}
            if(this->writeLogHeader(writer) != SYNCML_DM_SUCCESS)
		{
			CloseLog();
			dm_stat = SYNCML_DM_IO_FAILURE;
		}
        }
    	}
	return dm_stat;
}

/*============================================================================n

Function:    SyncML_PlugIn_WBXMLLog::updatePosition

Description: Update position information

Memory:      The caller is responsible for freeing the SyncML_DM_Command object

Notes:       

================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_PlugIn_WBXMLLog::updatePosition(SyncML_DM_WBXMLWriter* writer)
{
    INT32 tempPos = 0;
    INT32 currentPos = 0;

    
    // Write previous record position
    if(writer->writeData((const UINT8*)&this->prevRecord, sizeof(this->prevRecord)) != SYNCML_DM_SUCCESS)
        return  SYNCML_DM_IO_FAILURE;

    // Place holder for next record  pointer 
    if(writer->writeData((const UINT8*)&tempPos, sizeof(tempPos)) != SYNCML_DM_SUCCESS)
        return  SYNCML_DM_IO_FAILURE;

    // Go to end of log file
    if(this->fileHandle->seek(XPL_FS_SEEK_END, 0) != SYNCML_DM_SUCCESS) 
        return  SYNCML_DM_IO_FAILURE;

    currentPos = this->fileHandle->position();

    // Go to the previous place holder
    if(this->fileHandle->seek(XPL_FS_SEEK_SET, this->prevRecord - sizeof(INT32)) != SYNCML_DM_SUCCESS) 
        return  SYNCML_DM_IO_FAILURE;

    tempPos = this->fileHandle->position();

    if(writer->writeData((const UINT8*)&currentPos, sizeof(currentPos)) != SYNCML_DM_SUCCESS)
        return  SYNCML_DM_IO_FAILURE;

    // Go to end of log file
    if(this->fileHandle->seek(XPL_FS_SEEK_END, 0) != SYNCML_DM_SUCCESS) 
        return  SYNCML_DM_IO_FAILURE;

    this->prevRecord = currentPos;

    return SYNCML_DM_SUCCESS;

}
/*============================================================================n

Function:    SyncML_PlugIn_WBXMLLog::writeLogHeader

Description: Write log header information

Notes:       

================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_PlugIn_WBXMLLog::writeLogHeader(SyncML_DM_WBXMLWriter* writer)
{
    INT32 tempPos = 0;
    prevRecord    = 3 * sizeof(INT32) + m_strRootPath.length() + 4;
        
    if(writer->writeData((const UINT8*)&prevRecord, sizeof(prevRecord)) != SYNCML_DM_SUCCESS)
       return  SYNCML_DM_IO_FAILURE;

    if(writer->writeByte(SyncML_DM_WBXMLArchive::URI_START_TAG 
                         | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS)
        return  SYNCML_DM_IO_FAILURE;

    if(writer->writeString(m_strRootPath.c_str()) != SYNCML_DM_SUCCESS)
        return  SYNCML_DM_IO_FAILURE;

    if(this->fileHandle->seek(XPL_FS_SEEK_END, 0) != SYNCML_DM_SUCCESS) 
        return  SYNCML_DM_IO_FAILURE;

    if(writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
        return  SYNCML_DM_IO_FAILURE;

    if(writer->writeData((const UINT8*)&tempPos, sizeof(tempPos)) != SYNCML_DM_SUCCESS)
        return  SYNCML_DM_IO_FAILURE;
    
    if(writer->writeData((const UINT8*)&tempPos, sizeof(tempPos)) != SYNCML_DM_SUCCESS) 
        return  SYNCML_DM_IO_FAILURE;

    return SYNCML_DM_SUCCESS;
}
/*============================================================================n

Function:    SyncML_PlugIn_WBXMLLog::writeLogHeader

Description: Write log header information

Notes:       

================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_PlugIn_WBXMLLog::writePluginNode(CPCHAR pbURI, SyncML_DM_WBXMLWriter* writer, const DmtNode* ptrNode)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    DMString strVal;
    DMGetData oGetData;
    BOOLEAN  m_bESN = FALSE;
    CPCHAR tmpStr = NULL;


    DmtAttributes oAttr;
    dm_stat = ptrNode->GetAttributes( oAttr );
    if( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat; 

    if (ptrNode->IsLeaf())
    {    
        DmtData data;
        dm_stat = ptrNode->GetValue(data);

        if (dm_stat == SYNCML_DM_SUCCESS)
            dm_stat = oGetData.set(data,oAttr.GetType());
#ifdef LOB_SUPPORT
	if(ptrNode->IsExternalStorageNode())
      {
		PDmtRWPluginNode pRWNode = (DmtRWPluginNode *) ((DmtNode *)ptrNode);
		m_bESN = TRUE;
		tmpStr = pRWNode->GetESNBackupFileName();
	}
#endif
    }
    else
    {    
        DMStringVector mapNodeNames;

        dm_stat = ((DmtPluginTree *)this->pluginTree)->GetChildNodeNames( pbURI, mapNodeNames );

        if ( dm_stat != SYNCML_DM_SUCCESS )
           return dm_stat;
            
        for ( int i = 0; i < mapNodeNames.size(); i++ )
        {
           if (i >0)
              strVal += "/";
               
           strVal += mapNodeNames[i];
        }
        dm_stat = oGetData.set(SYNCML_DM_FORMAT_NODE,strVal,strVal.length(),"text/plain");
    }
    
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

    DMNode * psNodeObject = dmTreeObj.CreateNodeObj(oGetData.m_nFormat, m_bESN, tmpStr);
    if(psNodeObject == NULL)
       return SYNCML_DM_DEVICE_FULL;

    DMString nodeName;

    dm_stat = ptrNode->GetNodeName(nodeName);
    if ( dm_stat == SYNCML_DM_SUCCESS )
    {
        dm_stat = psNodeObject->set(nodeName,oAttr.GetTitle(),&oGetData);
        if ( dm_stat == SYNCML_DM_SUCCESS )
        {    dm_stat = writer->writeNode((const DMNode*)psNodeObject);
        	if ( dm_stat == SYNCML_DM_SUCCESS )
        	{
			// Write interior node information
			if (!ptrNode->IsLeaf())
			{
				if(strVal.length()!= 0)
				{
					if(writer->writeByte(SyncML_DM_WBXMLArchive::DATA_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
						writer->writeOpaque((const UINT8*)strVal.c_str(), strVal.length()) != SYNCML_DM_SUCCESS ||
						writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
						dm_stat = SYNCML_DM_IO_FAILURE;

				}
			}

        	}

        }
    }    

    delete psNodeObject;

    return dm_stat;
}
/*============================================================================n

Function:    SyncML_PlugIn_WBXMLLog::writeURIInfo

Description:Log URI information

Memory:  

Notes:       

================================================================================*/

SYNCML_DM_RET_STATUS_T 
SyncML_PlugIn_WBXMLLog::writeURIInfo(SYNCML_DM_PLUGIN_COMMAND_T commandType, CPCHAR pbURI, SyncML_DM_WBXMLWriter* writer)
{
    SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_SUCCESS;
    UINT8 ioArray[5] = { SyncML_DM_WBXMLArchive::ENTRY_START_TAG | 
                         SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK,
                         SyncML_DM_WBXMLArchive::CMDTYPE_START_TAG | 
                         SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK,
                         commandType,
                         SyncML_DM_WBXMLArchive::END_TAG,
                         SyncML_DM_WBXMLArchive::URI_START_TAG | 
                         SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK};
    
    if(writer == NULL || fileHandle == NULL)
                return SYNCML_DM_FAIL;

    // Go to end of log file
    if(this->fileHandle->seek(XPL_FS_SEEK_END, 0) != SYNCML_DM_SUCCESS) 
    {
        ret_code = SYNCML_DM_IO_FAILURE;
        goto URIWriteFailed;
    }


    if(writer->writeData(ioArray, sizeof(ioArray)) != SYNCML_DM_SUCCESS){
        ret_code = SYNCML_DM_IO_FAILURE;
        goto URIWriteFailed;
    }

    if(writer->writeString(pbURI) != SYNCML_DM_SUCCESS){
        ret_code = SYNCML_DM_IO_FAILURE;
        goto URIWriteFailed;
    }

    /* End of the URI field */
    if(writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS){
        ret_code = SYNCML_DM_IO_FAILURE;
        goto URIWriteFailed;
    }
     return ret_code;

    URIWriteFailed:
        delete writer;
        writer = NULL;
        this->fileHandle->close();
        return ret_code;
}

/*============================================================================n

Function:    SyncML_PlugIn_WBXMLLog::logCommand

Description: logs the command type, URI, node properties and recovery field for 
         a command in the log file.

Memory:      The caller is responsible for freeing the SyncML_DM_Command object

Notes:       

================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_PlugIn_WBXMLLog::logCommand(SYNCML_DM_PLUGIN_COMMAND_T commandType,
                                        CPCHAR pbURI, 
                                        SYNCML_DM_PLUGIN_COMMAND_ATTRIBUTE_T attribute,
                                        const DmtNode* node)
{
    SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_SUCCESS;
    INT32 tempPos = 0;
    INT32 currentPos = 0;
    DMVector<PDmtNode> oChildren;
    PDmtRWPluginNode ptrTempNode = (DmtRWPluginNode *) ((DmtPluginNode *)node);

    if(commandType != SYNCML_DM_PLUGIN_ADD &&
       commandType != SYNCML_DM_PLUGIN_ADD_CHILD &&
       commandType != SYNCML_DM_PLUGIN_DELETE &&
       commandType != SYNCML_DM_PLUGIN_REPLACE)
    {
        return SYNCML_DM_FAIL;
    }
  if(writer == NULL)
  {  writer = new SyncML_DM_WBXMLWriter(this->fileHandle);
	   if( writer == NULL)
	   return SYNCML_DM_DEVICE_FULL;
  }
    
    if(commandType != SYNCML_DM_PLUGIN_ADD)
    { 
        ret_code = writeURIInfo(commandType, pbURI, writer);
        if(ret_code != SYNCML_DM_SUCCESS)
            goto WriteFailed;
    }
    
    INT32 i;
    switch ( commandType ) 
    {
        case SYNCML_DM_PLUGIN_ADD:  
        {
            if(node == NULL)
                return  SYNCML_DM_FAIL;

             // Serialize the subtree
            ret_code = ptrTempNode->GetChildNodes( oChildren );
            DMString nodePath; 
            for (i = 0; i < oChildren.size(); i++ )
            {
                ret_code = oChildren[i]->GetPath(nodePath);
                if(ret_code != SYNCML_DM_SUCCESS)
                    goto WriteFailed;
                
                ret_code = logCommand(SYNCML_DM_PLUGIN_ADD,
                                      nodePath.c_str(),
                                      attribute,
                                     (const DmtNode*)oChildren[i]);
                if(ret_code != SYNCML_DM_SUCCESS)
                    goto WriteFailed;
            }                
            ret_code = writeURIInfo(commandType, pbURI, writer);
            if(ret_code != SYNCML_DM_SUCCESS)
                goto WriteFailed;

            if(writePluginNode(pbURI, writer, node) != SYNCML_DM_SUCCESS)
            {
                ret_code = SYNCML_DM_IO_FAILURE;
                goto WriteFailed;
            }
            if(writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
            {
                ret_code = SYNCML_DM_IO_FAILURE;
                goto WriteFailed;
            }
            
            if(updatePosition(writer) != SYNCML_DM_SUCCESS) 
            {
                ret_code = SYNCML_DM_IO_FAILURE;
                goto WriteFailed;
            }
        }    
            break;
        
        case SYNCML_DM_PLUGIN_REPLACE:
            if(writer->writeByte(attribute) != SYNCML_DM_SUCCESS)
            {
                ret_code = SYNCML_DM_IO_FAILURE;
                goto WriteFailed;
            }

            switch(attribute) 
            {
                case SYNCML_DM_PLUGIN_COMMAND_ON_NODE:
                    if (node->IsLeaf())
                    {
                        DmtData data;
                        DMString strVal;
                        ret_code = node->GetValue(data);
                        if(ret_code == SYNCML_DM_SUCCESS)
                        {
                            if ( data.GetType() != SYNCML_DM_DATAFORMAT_NULL )
                                ret_code = data.GetString(strVal);
                            if (ret_code == SYNCML_DM_SUCCESS && strVal.length() != 0)
                            {
                                 if(writer->writeByte(SyncML_DM_WBXMLArchive::DATA_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
                                    writer->writeOpaque((UINT8*)strVal.c_str(),strVal.length()) != SYNCML_DM_SUCCESS ||
                                    writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS) 
                                 {
                                     ret_code = SYNCML_DM_IO_FAILURE;
                                     goto WriteFailed;
                                 }
                             }

                         }
                    }
                    else 
                    {
                        ret_code = SYNCML_DM_FAIL;
                        goto WriteFailed;
                    }
                    break;
                    
                case SYNCML_DM_PLUGIN_COMMAND_ON_NAME_PROPERTY:
                {
                    DMString nodeName;
                    
                    ret_code = node->GetNodeName(nodeName);
                    if ( ret_code == SYNCML_DM_SUCCESS )
                    {
                        if(writer->writeByte(SyncML_DM_WBXMLArchive::NAME_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
                           writer->writeString(nodeName.c_str()) != SYNCML_DM_SUCCESS ||
                           writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS) 
                        {
                            ret_code = SYNCML_DM_IO_FAILURE;
                            goto WriteFailed;
                        }
                    }    
                }    
                    break;
                
                case SYNCML_DM_PLUGIN_COMMAND_ON_TITLE_PROPERTY:
                {
                    DmtAttributes oAttr;
                    ret_code = node->GetAttributes( oAttr );
                    if ( ret_code == SYNCML_DM_SUCCESS ) 
                    {
                        const DMString & title = oAttr.GetTitle();
                        if(writer->writeByte(SyncML_DM_WBXMLArchive::TITLE_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
                           writer->writeString(title.c_str()) != SYNCML_DM_SUCCESS ||
                           writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS) 
                        {
                            ret_code = SYNCML_DM_IO_FAILURE;
                            goto WriteFailed;
                        }
                    }
                    else
                    {
                        ret_code = SYNCML_DM_FAIL;
                        goto WriteFailed;
                    }
                }    
                    break;
#ifdef LOB_SUPPORT
		   case SYNCML_DM_PLUGIN_COMMAND_ON_LOB_PROPERTY:
		   {
				CPCHAR  tmpStr = ptrTempNode->GetESNBackupFileName();

				if ( tmpStr != NULL) 
				{
					if(writer->writeByte(SyncML_DM_WBXMLArchive::ESN_File_NAME_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
						writer->writeString(tmpStr) != SYNCML_DM_SUCCESS ||
						writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS) 
						{
								ret_code = SYNCML_DM_IO_FAILURE;
								goto WriteFailed;
						}
				}
				else
				{
					if(writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
					{
						ret_code = SYNCML_DM_IO_FAILURE;
						goto WriteFailed;
					}
				}
			}	 
			break;
#endif
		default:
			ret_code = SYNCML_DM_INVALID_PARAMETER;
			goto WriteFailed;
			
            }
                
            if(updatePosition(writer) != SYNCML_DM_SUCCESS) 
            {
                ret_code = SYNCML_DM_IO_FAILURE;
                goto WriteFailed;
            }
            break;
        
        case SYNCML_DM_PLUGIN_DELETE:
            if(writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
            {
                ret_code = SYNCML_DM_IO_FAILURE;
                goto WriteFailed;
            }
            if(updatePosition(writer) != SYNCML_DM_SUCCESS) 
            {
                ret_code = SYNCML_DM_IO_FAILURE;
                goto WriteFailed;
            }
            break;
                
        case SYNCML_DM_PLUGIN_ADD_CHILD:
            if(writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
            {
                ret_code = SYNCML_DM_IO_FAILURE;
                goto WriteFailed;
            }
            if(updatePosition(writer) != SYNCML_DM_SUCCESS) 
            {
                ret_code = SYNCML_DM_IO_FAILURE;
                goto WriteFailed;
            }
            break;
    }

#ifdef TEST_DM_RECOVERY
    if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "PLUGIN_PF4") == 0)) 
    {
        printf("Type Ctrl-C to simulate Power Fail ...\n");
        sleep(30);
    }
#endif

    return ret_code;
    
    
    WriteFailed:
        delete writer;
        writer = NULL;
        this->fileHandle->close();
        return ret_code;

}

/*==============================================================================
====================

Function:    SyncML_PlugIn_WBXMLLog::gotoLastRecord

Description: Goto last log entry position

Returns:     SYNCML_DM_IO_FAILURE     - could not perform an I/O operation
         SYNCML_DM_LOG_CORRUPT    - there was an invalid byte read
         SYNCML_DM_SUCCESS
         SYNCML_DM_FILE_NOT_FOUND - could not find the log file

================================================================================
==================*/
SYNCML_DM_RET_STATUS_T
SyncML_PlugIn_WBXMLLog::gotoLastRecord(SyncML_DM_WBXMLReader* reader, 
                                           UINT8 *lastByte, 
                                           INT32 *currentPos )
{
    SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_SUCCESS;
    INT32 tempPos = 0;
    INT32 currentRecordPos = 0;
    INT32 nextRecordPos = 0;

    *currentPos = 0;

    // Go to end of log file
    if(this->fileHandle->seek(XPL_FS_SEEK_SET, prevRecord - 2*sizeof(INT32)) != SYNCML_DM_SUCCESS) 
        return  SYNCML_DM_IO_FAILURE;

    if(this->fileHandle->read((UINT8 *)currentPos, sizeof(INT32)) != SYNCML_DM_SUCCESS) 
        return  SYNCML_DM_IO_FAILURE;

    // No log entry
    if(*currentPos == 0)
        return SYNCML_DM_SUCCESS;
    
    // Point to last log entry
    ret_code = this->fileHandle->seek(XPL_FS_SEEK_SET, (int)*currentPos);
    
    // If it is valid log entry
    if(reader->readByte(lastByte) != SYNCML_DM_SUCCESS ||
       *lastByte != (SyncML_DM_WBXMLArchive::ENTRY_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK)) 
    {
        // Point to last log entry
        
        if(this->fileHandle->seek(XPL_FS_SEEK_SET, 0) != SYNCML_DM_SUCCESS) 
            return  SYNCML_DM_IO_FAILURE;

        if ((ret_code = this->fileHandle->read((UINT8 *)&currentRecordPos, sizeof(INT32))) != SYNCML_DM_SUCCESS) 
            return  SYNCML_DM_IO_FAILURE;

        nextRecordPos = currentRecordPos;
        
        while((ret_code == SYNCML_DM_SUCCESS))
        {
            // Read next entry until the pointer is 0
            tempPos = currentRecordPos;
            this->fileHandle->seek(XPL_FS_SEEK_SET, tempPos-sizeof(INT32));

            ret_code=  this->fileHandle->read((UINT8 *)&currentRecordPos, sizeof(INT32));
            if((currentRecordPos == 0) || (ret_code != SYNCML_DM_SUCCESS))
                break;
            nextRecordPos = tempPos;
        }
        *currentPos = nextRecordPos;
         // Read last entry
        this->fileHandle->seek(XPL_FS_SEEK_SET, (int)*currentPos);

        // If it is valid log entry
        if(reader->readByte(lastByte) != SYNCML_DM_SUCCESS ||
           *lastByte != (SyncML_DM_WBXMLArchive::ENTRY_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
        {
            //Read previous entry
            if(this->fileHandle->seek(XPL_FS_SEEK_SET, nextRecordPos-2*sizeof(INT32)) != SYNCML_DM_SUCCESS) 
                return  SYNCML_DM_IO_FAILURE;
            if(this->fileHandle->read((UINT8 *)currentPos, sizeof(INT32)) != SYNCML_DM_SUCCESS) 
                return  SYNCML_DM_IO_FAILURE;

            // No log entry
            if(*currentPos == 0)
                return SYNCML_DM_SUCCESS;
            // Point to last log entry
            this->fileHandle->seek(XPL_FS_SEEK_SET, (int)*currentPos);
            ret_code = reader->readByte(lastByte);
        }
    }
    return ret_code;
}
/*==================================================================================================

Function:	 SyncML_PlugIn_WBXMLLog::playLog

Description:  Log file playback and remove

==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_PlugIn_WBXMLLog::playLog(CPCHAR logFileName)
{
	return SyncML_Log::playLog(logFileName);
}

/*===============================================================================================

Function:    SyncML_PlugIn_WBXMLLog::playLog

Description: This function provides log looping for log playback and recovery
         purposes.  If the isRecovery field is set the function performs
         calls to recovery functions for the log entries in the log file.
         Otherwise, the function does log playback to the Tree and node 
         manager.

Returns:     SYNCML_DM_IO_FAILURE     - could not perform an I/O operation
         SYNCML_DM_LOG_CORRUPT    - there was an invalid byte read
         SYNCML_DM_SUCCESS
         SYNCML_DM_FILE_NOT_FOUND - could not find the log file

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_PlugIn_WBXMLLog::playLog()
{
    SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_SUCCESS;
    SYNCML_DM_COMMAND_T cmdType = SYNCML_DM_NO_COMMAND;
    DMBuffer cmdURI;
    BOOLEAN isLast = TRUE;
    DMAddNodeProp props;
    INT32 currentRecordPos = 0;

    UINT8 bYte;
    SYNCML_DM_URI_RESULT_T attribute;

    if((fileHandle == NULL))
         return SYNCML_DM_FAIL;

    if(reader == NULL)
   {		 reader = new SyncML_DM_WBXMLReader(this->fileHandle);
	
		 if(reader == NULL)
			 return SYNCML_DM_DEVICE_FULL;
   }
  if(this->fileHandle->seek(XPL_FS_SEEK_END, 0) != SYNCML_DM_SUCCESS) 
	  ret_code = SYNCML_DM_IO_FAILURE;

   prevRecord = this->fileHandle->position();

    if((ret_code=gotoLastRecord(reader,&bYte, &currentRecordPos)) != SYNCML_DM_SUCCESS)
       return ret_code;
   
    // No log entry
    if(currentRecordPos == 0)
        return SYNCML_DM_SUCCESS;
    
    while(1)
    {
        if(bYte == SyncML_DM_WBXMLArchive::END_TAG)
        {
            /* Signifies the end of the log file */
            break;    
        }
        else 
            if(bYte == (SyncML_DM_WBXMLArchive::ENTRY_START_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
            {
                if(reader->readOneLogRecord(&cmdType, &cmdURI, &attribute,&props,&bYte) != SYNCML_DM_SUCCESS)
                {
                    ret_code = SYNCML_DM_IO_FAILURE;
                    goto PlayFinished;
                }

                ret_code = playbackOneRecord(cmdType,(const char *)cmdURI.getBuffer(),attribute,&props);

#ifdef TEST_DM_RECOVERY
                if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "PLUGIN_PF5") == 0)) 
                {
                    printf("Type Ctrl-C to simulate Power Fail ...\n");
                    sleep(30);
                }
#endif
       
                if(ret_code == SYNCML_DM_IO_FAILURE)
                {
                    if(!isLast)
                        break; 
                }
                isLast = FALSE;
                // Go to previous entry
                if(this->fileHandle->seek(XPL_FS_SEEK_SET, currentRecordPos-2 * sizeof(INT32)) != SYNCML_DM_SUCCESS) 
                {
                    ret_code = SYNCML_DM_IO_FAILURE;
                    goto PlayFinished;
                }
                // Last log entry position
                if(this->fileHandle->read((UINT8 *)&currentRecordPos, sizeof(INT32)) != SYNCML_DM_SUCCESS) 
                    return  SYNCML_DM_IO_FAILURE;

                // No more log entry
                if(currentRecordPos == 0)
                    break;

                // Goto the entry
                if(this->fileHandle->seek(XPL_FS_SEEK_SET, currentRecordPos) != SYNCML_DM_SUCCESS) 
                {
                    ret_code = SYNCML_DM_IO_FAILURE;
                    goto PlayFinished;
                }
                // Read the first byte of data looking for an ENTRY_START_TAG 
                if(reader->readByte(&bYte) != SYNCML_DM_SUCCESS)
                {   
                    ret_code = SYNCML_DM_IO_FAILURE;
                    goto PlayFinished;
                }
    
            }
    }
    
    PlayFinished:

    UnInitLog();
    return ret_code;
}
/*=============================================================================================

Function:    SyncML_DM_WBXMLLog::performRecovery

Description: The function arguments are the command type and the full URI
         including the query and special case extensions (e.g. http://hi.com?ACL=*).

Returns:    SYNCML_DM_SUCCES
            SYNCML_DM_FAIL
            SYNCML_DM_IO_FAILURE

Notes:

=============================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_PlugIn_WBXMLLog::playbackOneRecord(SYNCML_DM_PLUGIN_COMMAND_T commandType,
                                           CPCHAR pbURI, 
                                           SYNCML_DM_PLUGIN_COMMAND_ATTRIBUTE_T attribute,
                                           DMAddNodeProp* props)

{
    PDmtRWPluginNode ptrNode = NULL;
    PDmtNode ptrDmtNode;
    DmtData oData;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    if(pluginTree == NULL)
         return SYNCML_DM_FAIL;

    /* Call the recovery functions */
    switch(commandType)
    {
        case SYNCML_DM_PLUGIN_ADD:
        {    
		dm_stat =  dmBuildData(props->m_nFormat, props->m_oData, oData);
		if ( dm_stat != SYNCML_DM_SUCCESS )
				return dm_stat;

            if (props->m_nFormat == SYNCML_DM_FORMAT_NODE) 
            {
                DMStringVector oChildren;
                DmStrToStringVector((CPCHAR)props->m_oData.getBuffer(),props->m_oData.getSize(), oChildren, '/');
      
                dm_stat = ((DmtRWPluginTree *)this->pluginTree)->CreateInteriorNodeInternal(pbURI, ptrDmtNode, oChildren);
            }
            else 
            {
#ifdef LOB_SUPPORT
		boolean isESN = (props->m_nFlags & DMNode::enum_NodeESN) != 0;
                dm_stat = ((DmtRWPluginTree *)this->pluginTree)->CreateLeafNodeInternal(pbURI, ptrDmtNode, oData, isESN);
		 if(dm_stat == SYNCML_DM_SUCCESS)
		 {
			// Restore ESN data
			if(isESN)
			{
	                ptrNode = (DmtRWPluginNode *) ((DmtNode *)ptrDmtNode);
			   dm_stat = ptrNode->RestoreESNData(props->getESNFileName());
			}
		 }
#else
                dm_stat = ((DmtRWPluginTree *)this->pluginTree)->CreateLeafNodeInternal(pbURI, ptrDmtNode, oData);
#endif
            }
           break;
        }
        
        case SYNCML_DM_PLUGIN_ADD_CHILD:
            dm_stat = ((DmtRWPluginTree *)this->pluginTree)->LinkToParentNode(pbURI);
            break;

        case SYNCML_DM_PLUGIN_DELETE:
            dm_stat = ((DmtRWPluginTree *)this->pluginTree)->DeleteNode(pbURI);
            break;

        case SYNCML_DM_PLUGIN_REPLACE:
            dm_stat = ((DmtRWPluginTree *)this->pluginTree)->GetNode(pbURI, ptrDmtNode);
            if ( dm_stat == SYNCML_DM_SUCCESS )
            {
                ptrNode = (DmtRWPluginNode *) ((DmtNode *)ptrDmtNode);
                if(ptrNode != NULL) 
                {

                    switch( attribute) 
                    {
                        case SYNCML_DM_PLUGIN_COMMAND_ON_NODE:
				  {
				  	DmtData curData;
				  	 dm_stat = ptrNode->GetValue(curData);
					 dm_stat =	dmBuildData(curData.GetType(), props->m_oData, oData);
					 if ( dm_stat != SYNCML_DM_SUCCESS )
						 return dm_stat;

                            	dm_stat = ptrNode->SetValue(oData);
                        	 } 
                           break;

                        case SYNCML_DM_PLUGIN_COMMAND_ON_NAME_PROPERTY:
                            dm_stat = ptrNode->Rename(props->getName());
                            break;

                        case SYNCML_DM_PLUGIN_COMMAND_ON_TITLE_PROPERTY:
                            dm_stat = ptrNode->SetTitle(props->getTitle());
                            break;
#ifdef LOB_SUPPORT
			     case	SYNCML_DM_PLUGIN_COMMAND_ON_LOB_PROPERTY:
			 	 dm_stat = ptrNode->RestoreESNData(props->getESNFileName());
				 break;
#endif
				default:
					break;
                    }
                }
             }
             break;

         default: 
         /* recovery_status has already been initialized to fail */
         break;
   }

   return dm_stat; 

}
