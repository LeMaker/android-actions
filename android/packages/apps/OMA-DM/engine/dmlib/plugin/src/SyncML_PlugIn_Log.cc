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

    File Name: SyncML_PlugIn_Log.cc

    General Description: This contains function implementations of the Log class.

==================================================================================================*/

#include "SyncML_PlugIn_Log.H"

/*==================================================================================================

Function:    SyncML_PlugIn_Log::SyncML_PlugIn_Log

Description: Constructor for the Log object

==================================================================================================*/
SyncML_PlugIn_Log::SyncML_PlugIn_Log()
{
	this->fileHandle = NULL;
}

/*==================================================================================================

Function:    SyncML_DM_Log::~SyncML_PlugIn_Log

Description: Destructor for the Log object

==================================================================================================*/
SyncML_PlugIn_Log::~SyncML_PlugIn_Log()
{
	/* The fileHandle reference is deleted by the calling class */
}
/*==================================================================================================

Function:    SyncML_PlugIn_Log::setLogFileHandle

Description:  Accessor method for the file reader

==================================================================================================*/
void
SyncML_PlugIn_Log::setLogFileHandle(DMFileHandler* fileHandle)
{
	this->fileHandle = fileHandle;
}

/*==================================================================================================

Function:    SyncML_PlunIn_Log::getLogFileHandle

Description:  Accessor method for the file reader

==================================================================================================*/
DMFileHandler*
SyncML_PlugIn_Log::getLogFileHandle()
{
        return this->fileHandle;
}
