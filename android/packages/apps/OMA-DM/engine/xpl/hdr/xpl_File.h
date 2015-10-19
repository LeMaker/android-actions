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

/*
 *  DESCRIPTION:
 *      The xpl_File.h header file contains constants and function prototypes
 *      for filesystem operations      
 */

#ifndef XPL_FILE_H
#define XPL_FILE_H
 
#include "xpl_Time.h"

#ifdef __cplusplus  
extern "C" {
#endif

/************** CONSTANTS ****************************************************/

#define XPL_FS_MAX_FILE_NAME_LENGTH  255
#define XPL_FS_SIZE_INVALID (0)
#define XPL_FS_HANDLE_INVALID (-1)
#define XPL_FS_SHANDLE_INVALID (-1)

/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/

typedef INT32 XPL_FS_HANDLE_T;           
typedef INT32  XPL_FS_SHANDLE_T;        
typedef INT32 XPL_FS_COUNT_T;    
typedef INT32 XPL_FS_SEEK_OFFSET_T;
typedef UINT32 XPL_FS_SIZE_T;
typedef char XPL_FS_SEARCH_FILE[XPL_FS_MAX_FILE_NAME_LENGTH];

/* This are internal flags */
enum
{
     XPL_FS_RDONLY_MODE = 1,  /* open for reading only */
     XPL_FS_WRONLY_MODE = 2,  /* open for writing only */
     XPL_FS_TRUNC_MODE = 4,   /* length is truncated to 0 */  
     XPL_FS_CREAT_MODE = 8,   /* create if file doesn't exist */
     XPL_FS_RDWR_MODE = 16,   /* open for reading and writing */ 
     XPL_FS_APPEND_MODE= 32   /* offeset is set to the end of the file 
                               *   prior each write */
};
typedef UINT8 XPL_FS_OPEN_MODE_T;

enum
{
     XPL_FS_SEEK_SET =0,  /* set offset to offset */
     XPL_FS_SEEK_CUR,     /* set offset to current plus offset */
     XPL_FS_SEEK_END      /* set fileoffset to EOF plus offset */
 };
typedef UINT8 XPL_FS_SEEK_MODE_T;

enum
{
     XPL_FS_RET_SUCCESS  = 0,     /* operation successfully completed */
     XPL_FS_RET_FAIL = 1,         /* operation failed */
     XPL_FS_RET_TRYAGAIN = 2,     /* try later */ 
     XPL_FS_RET_NOT_FOUND = 3,    /* not found */
     XPL_FS_RET_BADARGUMENT = 4,  /* bad argument */
     XPL_FS_RET_NOT_SUPPORTED = 5,/* not supported */
     XPL_FS_RET_PERM_FAIL = 6     /* permission failed */
     
};
typedef UINT8  XPL_FS_RET_STATUS_T;

/************** CONSTANTS ****************************************************/

/*Thise are public used flags passed to XPL_FS_OpenFile */
#define XPL_FS_FILE_READ XPL_FS_RDONLY_MODE
#define XPL_FS_FILE_WRITE (XPL_FS_WRONLY_MODE|XPL_FS_TRUNC_MODE|XPL_FS_CREAT_MODE)
#define XPL_FS_FILE_RDWR  (XPL_FS_RDWR_MODE|XPL_FS_CREAT_MODE)
#define XPL_FS_FILE_APPEND (XPL_FS_WRONLY_MODE|XPL_FS_APPEND_MODE|XPL_FS_CREAT_MODE)

/*=================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================*/
/* Function returns an integer value, which is used to refer to the file. 
 * If un- successful, it returns -1, and sets the result to 
 * indicate the error type. */
XPL_FS_HANDLE_T XPL_FS_Open(CPCHAR file_uri,
                                const XPL_FS_OPEN_MODE_T open_mode,
                                XPL_FS_RET_STATUS_T * result);


/* Function closes the file associated with handle. 
* The function returns 0 if successful, or error code set appropriately. */
XPL_FS_RET_STATUS_T XPL_FS_Close(XPL_FS_HANDLE_T file_handle);

/* Function deletes file */
XPL_FS_RET_STATUS_T XPL_FS_Remove(CPCHAR path);

/* Attempts to change name of a file */
XPL_FS_RET_STATUS_T XPL_FS_Rename(CPCHAR old_name, CPCHAR new_name);

/* Returns TRUE if file with specified name exist */
BOOLEAN XPL_FS_Exist(CPCHAR match_uri);

/* Returns TRUE if file has permission */
BOOLEAN XPL_FS_CheckPermission(CPCHAR match_uri, XPL_FS_OPEN_MODE_T permission);

/* Returns file size */
XPL_FS_SIZE_T XPL_FS_GetSize(XPL_FS_HANDLE_T file_handle);

/* Returns time of last modifiction */ 
XPL_CLK_CLOCK_T XPL_FS_GetModTime(CPCHAR file_uri);

/*Function attempts to read count from the file associated with file_handle, 
* and places the characters read into buffer. 
* The function returns the number of bytes read. On end-of-file, 0 is returned, on error it returns -1
* setting result to indicate the type of error that occurred */
XPL_FS_COUNT_T XPL_FS_Read(XPL_FS_HANDLE_T file_handle,
                               void* buffer,
                               const XPL_FS_COUNT_T count,
                               XPL_FS_RET_STATUS_T * result);

/* Function attempts to write count of byte from buffer to the file associated with handle.
* The function returns the count of bytes written to the file.
* A return value of -1 indicates an error, with result set appropriately. */
XPL_FS_COUNT_T XPL_FS_Write(XPL_FS_HANDLE_T file_handle,
                               void* buffer,
                               const XPL_FS_COUNT_T count,
                               XPL_FS_RET_STATUS_T * result);

/* Function shall set the file offset for the open file description associated with file_handle
* Upon successful completion, the resulting offset, as measured in bytes from the beginning of
* the file, shall be returned. Otherwise, -1 shall be returned, result shall be set to indicate the error. */
XPL_FS_SEEK_OFFSET_T XPL_FS_Seek(XPL_FS_HANDLE_T file_handle,
                                XPL_FS_SEEK_OFFSET_T offset,
                                XPL_FS_SEEK_MODE_T whence,
                                XPL_FS_RET_STATUS_T * result);


/* Functionon search files in the directory and returns handler of search results */
XPL_FS_SHANDLE_T XPL_FS_StartSearch(CPCHAR dir_uri, 
                                    CPCHAR extension, 
                                    BOOLEAN bFullName, 
                                    XPL_FS_RET_STATUS_T * result);

/* Returns result of search. */
XPL_FS_RET_STATUS_T XPL_FS_GetSearchResult(XPL_FS_SHANDLE_T search_handle, 
                                           XPL_FS_SEARCH_FILE file_name);

/* Closes search */ 
XPL_FS_RET_STATUS_T XPL_FS_EndSearch(XPL_FS_SHANDLE_T dir_handle);

/* Attempts to create the directory. */ 
XPL_FS_RET_STATUS_T XPL_FS_MkDir(CPCHAR dir_uri);

/* locks the open file */
XPL_FS_RET_STATUS_T XPL_FS_Lock(XPL_FS_HANDLE_T file_handle, BOOLEAN bLockExclusive );

/* unlocks file */
XPL_FS_RET_STATUS_T XPL_FS_Unlock(XPL_FS_HANDLE_T file_handle);

/* unlinks file */
XPL_FS_RET_STATUS_T XPL_FS_Unlink(CPCHAR name);


/* establishes a mapping between a process address space and a file for read only operation. */
UINT8 * XPL_FS_MemMap(XPL_FS_HANDLE_T file_handle, UINT32 size, UINT32 offset, XPL_FS_RET_STATUS_T * result);

/* cancel mapping between a process address space and a file */
XPL_FS_RET_STATUS_T XPL_FS_MemUnMap(UINT8 * pBuffer, UINT32 size);
#ifdef LOB_SUPPORT
/* Return the temporary ESN file directory */
CPCHAR XPL_FS_TempEsnDir();
/* Return free disk space */
XPL_FS_SIZE_T XPL_FS_FreeDiskSpace(CPCHAR pEsnDir);
#endif

#ifdef __cplusplus
}
#endif

#endif /* XPL_FILE_H */
