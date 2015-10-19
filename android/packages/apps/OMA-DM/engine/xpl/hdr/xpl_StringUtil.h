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

#ifndef XPL_STRINGUTIL_H
#define XPL_STRINGUTIL_H

/**
    \file xpl_StringUtil.h
    \brief The xpl_StringUtil.h header file contains function prototypes for basic string operations.
    <b>Warning:</b>   All functions, structures, and classes from this header file are for internal usage only!!!
*/

/************** HEADER FILE INCLUDES *****************************************/
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "xpl_Types.h"

/************** FUNCTIONS ****************************************************/

/** Definition function xplStrlen for cross platform support*/
#define xplStrlen(str) ((str!=XPL_NULL)? strlen(str):0)

/** Definition function xplStrcpy for cross platform support*/
#define xplStrcpy(target, source) strcpy(target, source)

/** Definition function xplStrncpy for cross platform support*/
#define xplStrncpy(target, source, count) strncpy(target, source, count)

/** Definition function xplStrcat for cross platform support*/
#define xplStrcat(target, source) strcat(target, source)

/** Definition function xplStrncat for cross platform support*/
#define xplStrncat(target, source, count) strncat(target, source, count)

/** Definition function xplStrcmp for cross platform support*/
#define xplStrcmp(target, source) strcmp(target, source)

/** Definition function xplStrncmp for cross platform support*/
#define xplStrncmp(target, source, count) strncmp(target, source, count)

/** Definition function xplStrchr for cross platform support*/
#define xplStrchr(source, target) strchr(source, target)

/** Definition function xplStrrchr for cross platform support*/
#define xplStrrchr(source, target) strrchr(source, target)

/** Definition function xplStrstr for cross platform support*/
#define xplStrstr(source, target) strstr(source, target)

/** Definition function xplTolower for cross platform support*/
#define xplTolower(source) tolower(source)

/** Definition function xplAtoi for cross platform support*/
#define xplAtoi(source) atoi(source)

/** Definition function xplAtol for cross platform support*/
#define xplAtol(source) atol(source)

/** Definition function xplAtoll for cross platform support*/
#define xplAtoll(source) atoll(source)

/** Definition function xplSprintf for cross platform support*/
#define xplSprintf sprintf

/** Definition function xplSnprintf for cross platform support*/
#define xplSnprintf snprintf

/** Definition function xplSscanf for cross platform support*/
#define xplSscanf sscanf

#endif /* XPL_STRINGUTIL_H */
