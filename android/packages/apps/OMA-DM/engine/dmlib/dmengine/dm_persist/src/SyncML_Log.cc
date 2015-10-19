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

#include "SyncML_Log.H"

/*==================================================================================================

Function:    SyncML_Log::SyncML_Log

Description: Constructor for the Log object

==================================================================================================*/
SyncML_Log::SyncML_Log()
{
    this->fileHandle = NULL;
}

/*==================================================================================================

Function:    SyncML_DM_Log::InitLog

Description: Open log

==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_Log::InitLog(CPCHAR logFileName)
{
    if (fileHandle != NULL)
        CloseLog();

    INT32 modeFlag = XPL_FS_FILE_RDWR;
    // If file does not exist use write mode instead of read/write to prevent file I/O error
    if (!XPL_FS_Exist(logFileName))
        modeFlag = XPL_FS_FILE_WRITE;

    fileHandle = new DMFileHandler(logFileName);
    if(fileHandle == NULL)
       return SYNCML_DM_IO_FAILURE;
    if (fileHandle->open(modeFlag) != SYNCML_DM_SUCCESS)
    {
       fileHandle->deleteFile();
       delete fileHandle;
       fileHandle = NULL;
       return SYNCML_DM_IO_FAILURE;
    }

    return setLogFileHandle(fileHandle);
}

/*==================================================================================================

Function:    UnInitLog

Description: Uninitialize log

==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_Log::UnInitLog()
{
    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================

Function:   :~SyncML_Log

Description: Destructor for the Log object

==================================================================================================*/
SyncML_Log::~SyncML_Log()
{
    /* The fileHandle reference is deleted by the calling class */
}

/*==================================================================================================

Function:    setLogFileHandle

Description:  Accessor method for the file reader

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_Log::setLogFileHandle(DMFileHandler* fileHandle)
{
    this->fileHandle = fileHandle;
    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================

Function:    getLogFileHandle

Description:  Accessor method for the file reader

==================================================================================================*/
DMFileHandler*
SyncML_Log::getLogFileHandle()
{
    return this->fileHandle;
}

/*==================================================================================================

Function:    RemoveLog

Description:  Remove  log file

==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_Log::RemoveLog()
{
    UnInitLog();
    if(fileHandle != NULL)
    {
        fileHandle->deleteFile();
        delete fileHandle;
        fileHandle = NULL;
    }
    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================

Function:    CloseLog

Description:  Close log file

==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_Log::CloseLog()
{
    UnInitLog();
    if (fileHandle != NULL)
    {
        fileHandle->close();
        delete fileHandle;
        fileHandle = NULL;
    }
    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================

Function:    SyncML_Log::playLog

Description:  Log file playback

==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_Log::playLog()
{
    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================

Function:    SyncML_Log::playLog

Description:  Log file playback and remove

==================================================================================================*/
SYNCML_DM_RET_STATUS_T SyncML_Log::playLog(CPCHAR logFileName)
{
    SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
    retStatus = InitLog(logFileName);
    if (retStatus == SYNCML_DM_SUCCESS)
    {
        retStatus = playLog();      
    }
    RemoveLog();
    return retStatus;
}
