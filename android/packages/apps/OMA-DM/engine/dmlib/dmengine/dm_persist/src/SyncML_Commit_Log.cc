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
#include "SyncML_Commit_Log.H"
#include "dm_uri_utils.h"

/*==================================================================================================

Function:    SyncML_Commit_Log::SyncML_Commit_Log

Description: Constructor for the Log object

==================================================================================================*/
SyncML_Commit_Log::SyncML_Commit_Log():SyncML_Log()
{
    reader = NULL;
    writer = NULL;
}

/*==================================================================================================

Function:    SyncML_Commit_Log::~SyncML_Commit_Log

Description: Destructor for the Log object

==================================================================================================*/
SyncML_Commit_Log::~SyncML_Commit_Log()
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
}
 /*==================================================================================================
 
 Function:     UnInitLog
 
 Description: Uninitialize log
 
 ==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_Commit_Log::UnInitLog()
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
     return SyncML_Log::UnInitLog();
 }
 /*============================================================================n
 
 Function:      SyncML_Commit_Log::CloseLog
 
 Description: Write end of log tag log file and close it.
 
 
 Notes:       
 
 ================================================================================*/
 SYNCML_DM_RET_STATUS_T  SyncML_Commit_Log::CloseLog()
{
     /* Write the end tag for the root node, then end tag for the MgmtTree */
     UINT8 endTags[2] = { SyncML_DM_WBXMLArchive::END_TAG, SyncML_DM_WBXMLArchive::END_TAG };
     
     if( fileHandle == NULL)
           return SYNCML_DM_SUCCESS;

     if(writer == NULL)
     {
        writer = new SyncML_DM_WBXMLWriter(this->fileHandle);
        if(writer == NULL)
              return SYNCML_DM_DEVICE_FULL;
     }
     // Go to end of log file
      if(fileHandle->seek(XPL_FS_SEEK_END, 0) != SYNCML_DM_SUCCESS)
        {
        SyncML_Log::CloseLog();
          return SYNCML_DM_IO_FAILURE;
        }

     if(writer->writeData(&endTags[0], 2) != SYNCML_DM_SUCCESS) 
     {
         SyncML_Log::CloseLog();
         return SYNCML_DM_IO_FAILURE;
     }
    return     SyncML_Log::CloseLog();
}
 /*============================================================================n
 
 Function:      SyncML_Commit_Log::logCommand
 
 Description: logs the command type, URI, node properties and recovery field for 
          a command in the log file.
 
 Memory:      The caller is responsible for freeing the SyncML_DM_Command object
 
 Notes:       
 
 ================================================================================*/
 SYNCML_DM_RET_STATUS_T
 SyncML_Commit_Log::logCommand(SYNCML_DM_COMMAND_T commandType,
                                        CPCHAR pSourceFileName, 
                                        CPCHAR pTargetFileName)
{
  SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_SUCCESS;
  UINT8 ioArray[5] = { SyncML_DM_WBXMLArchive::ENTRY_START_TAG | 
                     SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK,
                     SyncML_DM_WBXMLArchive::CMDTYPE_START_TAG | 
                     SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK,
                     commandType,
                     SyncML_DM_WBXMLArchive::END_TAG,
                     SyncML_DM_WBXMLArchive::TARGET_FILE_TAG | 
                     SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK};

  // Check if the command is valid
  if(       commandType != SYNCML_DM_DELETE &&
       commandType != SYNCML_DM_REPLACE)
  {
        return SYNCML_DM_FAIL;
  }
  if(writer == NULL)
  {
      writer = new SyncML_DM_WBXMLWriter(this->fileHandle);
     if(writer == NULL)
     {
           ret_code = SYNCML_DM_DEVICE_FULL;
           goto commitlogfailed;
     } 
  }
  if( fileHandle == NULL || pTargetFileName == NULL)
  {
        ret_code = SYNCML_DM_IO_FAILURE;
        goto commitlogfailed;
  } 
 // Go to end of log file
  if(this->fileHandle->seek(XPL_FS_SEEK_END, 0) != SYNCML_DM_SUCCESS) 
  {
    ret_code = SYNCML_DM_IO_FAILURE;
    goto commitlogfailed;
  } 

 if(writer->writeData(ioArray, sizeof(ioArray)) != SYNCML_DM_SUCCESS)
 {
    ret_code = SYNCML_DM_IO_FAILURE;
    goto commitlogfailed;
}

if(writer->writeString(pTargetFileName) != SYNCML_DM_SUCCESS)
{
    ret_code = SYNCML_DM_IO_FAILURE;
    goto commitlogfailed;
}

/* End of the URI field */
if(writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
{
    ret_code = SYNCML_DM_IO_FAILURE;
    goto commitlogfailed;
}
// Special case for SYNCML_DM_REPLACE command
if( commandType == SYNCML_DM_REPLACE)
{
    // Write original file name
    if(DmStrlen(pSourceFileName) ==0)
    {
        /* End of the URI field */
        if(writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS)
        {
            ret_code = SYNCML_DM_IO_FAILURE;
            goto commitlogfailed;
        }
    }
    else
    {
    if(writer->writeByte(SyncML_DM_WBXMLArchive::SOURCE_FILE_TAG | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK) != SYNCML_DM_SUCCESS ||
       writer->writeString(pSourceFileName) != SYNCML_DM_SUCCESS ||
       writer->writeByte(SyncML_DM_WBXMLArchive::END_TAG) != SYNCML_DM_SUCCESS) 
    {
        ret_code = SYNCML_DM_IO_FAILURE;
        goto commitlogfailed;
    }
  }
}
 return ret_code;

commitlogfailed:
 if(writer != NULL)
 {    delete writer;
        writer = NULL;
 }
 if(fileHandle)
 {
        this->fileHandle->close();
 }
 return ret_code;
 }
 /*===============================================================================================
 
 Function:      SyncML_Commit_Log::renameESNfile
 
 Description: Remove the .cmt suffix from ESN file
 
 Returns:      SYNCML_DM_IO_FAILURE       - could not perform an I/O operation
          SYNCML_DM_LOG_CORRUPT    - there was an invalid byte read
          SYNCML_DM_SUCCESS
          SYNCML_DM_FILE_NOT_FOUND - could not find the log file
 
 ==================================================================================================*/
 SYNCML_DM_RET_STATUS_T  SyncML_Commit_Log::renameESNfile(CPCHAR pTargetFileName,    DMFileHandler &handler)
{
  SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_SUCCESS;
  DMString fileName;
  fileName.RemoveSufix(pTargetFileName, SYNCML_DM_DOT);
  if (handler.rename(fileName.c_str()) != SYNCML_DM_SUCCESS)
     ret_code = SYNCML_DM_IO_FAILURE;
  
  handler.close();
  return ret_code;
}
 /*===============================================================================================
 
 Function:     playbackOneRecord
 
 Description: Playback one commit log record
 
 Returns:      SYNCML_DM_IO_FAILURE       - could not perform an I/O operation
          SYNCML_DM_LOG_CORRUPT    - there was an invalid byte read
          SYNCML_DM_SUCCESS
          SYNCML_DM_FILE_NOT_FOUND - could not find the log file
 
 ==================================================================================================*/
 SYNCML_DM_RET_STATUS_T  SyncML_Commit_Log::playbackOneRecord(SYNCML_DM_COMMAND_T commandType,
                                                  CPCHAR pSourceFileName,
                                                   CPCHAR pTargetFileName)
{
  SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_SUCCESS;
  // No any log entry
  if(commandType == SYNCML_DM_NO_COMMAND)
    return  SYNCML_DM_SUCCESS;

  if (!XPL_FS_Exist(pTargetFileName))
  {
    return  SYNCML_DM_SUCCESS;
  }
  
  DMFileHandler targetHandle(pTargetFileName);
  ret_code = targetHandle.open(XPL_FS_FILE_RDWR);
  if(ret_code != SYNCML_DM_SUCCESS)
      return  SYNCML_DM_SUCCESS;
      
  switch(commandType)
  {
    // Delete the lob file
    case SYNCML_DM_DELETE:
        ret_code = targetHandle.deleteFile();
        break;
            
    // Delete the lob file
    case SYNCML_DM_REPLACE:
            // Delete the target file first
            if(pSourceFileName != NULL)
            {
                DMFileHandler sourceHandle(pSourceFileName);
                ret_code = sourceHandle.open(XPL_FS_FILE_RDWR);
                if(ret_code == SYNCML_DM_SUCCESS)
                    sourceHandle.deleteFile();

                if (targetHandle.rename(pSourceFileName) != SYNCML_DM_SUCCESS)
                    ret_code = SYNCML_DM_IO_FAILURE;
                targetHandle.close();
            }
            else
            ret_code    = renameESNfile(pTargetFileName, targetHandle);
            break;
        default:
            targetHandle.close();
            ret_code = SYNCML_DM_FAIL;
            break;
  }
  return ret_code;
}

 /*===============================================================================================
 
 Function:      SyncML_Commit_Log::playLog
 
 Description: This function provides log looping for log playback and recovery
          purposes.  If the isRecovery field is set the function performs
          calls to recovery functions for the log entries in the log file.
          Otherwise, the function does log playback to the Tree and node 
          manager.
 
 Returns:      SYNCML_DM_IO_FAILURE       - could not perform an I/O operation
          SYNCML_DM_LOG_CORRUPT    - there was an invalid byte read
          SYNCML_DM_SUCCESS
          SYNCML_DM_FILE_NOT_FOUND - could not find the log file
 
 ==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_Commit_Log::playLog()
{
    SYNCML_DM_RET_STATUS_T ret_code = SYNCML_DM_SUCCESS;
    DMBuffer cmdURI;
    DMAddNodeProp props;
 
    UINT8 bYte;
 
    if (reader == NULL)
    {
        reader = new SyncML_DM_WBXMLReader(this->fileHandle);
        if (reader == NULL)
        {
            ret_code = SYNCML_DM_DEVICE_FULL;
            goto commitlogplayfinished;
        } 
    }
    if (fileHandle == NULL)
    {
        ret_code = SYNCML_DM_FAIL;
        goto commitlogplayfinished;
    } 
 
    // Goto the entry
    if (this->fileHandle->seek(XPL_FS_SEEK_SET, 0) != SYNCML_DM_SUCCESS) 
    {
        ret_code = SYNCML_DM_IO_FAILURE;
        goto commitlogplayfinished;
    }
    // Read the first byte of data looking for an ENTRY_START_TAG 
    if (reader->readByte(&bYte) != SYNCML_DM_SUCCESS)
    {     
        ret_code = SYNCML_DM_IO_FAILURE;
        goto commitlogplayfinished;
    }
    while (1)
    {
        if (bYte == SyncML_DM_WBXMLArchive::END_TAG)
        {
            /* Signifies the end of the log file */
            break;    
        }
        else 
        {
            if (bYte == (SyncML_DM_WBXMLArchive::ENTRY_START_TAG
                    | SyncML_DM_WBXMLArchive::TAG_CONTENT_MASK))
            {
                SYNCML_DM_COMMAND_T cmdType;
                DMBuffer  sourceFileName;
                DMBuffer  targetFileName;
                if (reader->readOneCommitLogRecord(&cmdType, &sourceFileName, &targetFileName,
                        &bYte) != SYNCML_DM_SUCCESS)
                {
                    ret_code = SYNCML_DM_IO_FAILURE;
                    goto commitlogplayfinished;
                }
 
                ret_code = playbackOneRecord(cmdType, (CPCHAR)sourceFileName.getBuffer(),
                        (CPCHAR)targetFileName.getBuffer());
 
                // Continue playing back the log even if the command fails        
                if (ret_code == SYNCML_DM_IO_FAILURE)
                {
                    break; 
                }
                // Read the first byte of data looking for an ENTRY_START_TAG 
                if (reader->readByte(&bYte) != SYNCML_DM_SUCCESS)
                {     
                    ret_code = SYNCML_DM_IO_FAILURE;
                    goto commitlogplayfinished;
                }
            }
        }
    }
     
    commitlogplayfinished:
 
    if (reader != NULL)
    {
        delete reader;
        reader = NULL;
    }
 
    if (writer != NULL)
    {
        delete writer;
        writer = NULL;
    }
    return ret_code;
}

/*==================================================================================================
 
Function:      SyncML_Commit_Log::playLog
 
Description:  Log file playback and remove
 
==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_Commit_Log::playLog(CPCHAR logFileName)
{
    return SyncML_Log::playLog(logFileName);
}
