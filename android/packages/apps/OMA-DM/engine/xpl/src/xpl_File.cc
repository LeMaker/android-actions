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

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/mman.h>
#include "xpl_Memory.h"
#include "xpl_File.h"
#include "xpl_Logger.h"
#include "dmvector.h"

class DMDirHandler
{
public :
  DMDirHandler() 
  {
    m_nIndex = 0;
    m_nDirLen = 0;
    m_pDir = XPL_NULL;
    
  }
 ~DMDirHandler() 
  {
    if (m_pDir)
      closedir(m_pDir);
  }

  void* operator new(size_t dwSize)
  {
      return (xplAllocMem(dwSize));
  }

  void operator delete(void *pvBuf)
  {
      xplFreeMem(pvBuf);
  }

  DMString m_szDirectory;
  DMString m_szExtension;
  INT32 m_nIndex;
  INT32 m_nDirLen;
  DIR *m_pDir;
};


#ifdef __cplusplus  
extern "C" {
#endif


XPL_FS_HANDLE_T XPL_FS_Open(CPCHAR file_uri,
                            const XPL_FS_OPEN_MODE_T open_mode,
                            XPL_FS_RET_STATUS_T * result)
{
    int flags = 0;
    XPL_FS_HANDLE_T nFileHandle;

    if ( result )
         *result = XPL_FS_RET_SUCCESS;    

    switch ( open_mode) {
    case XPL_FS_FILE_READ: 
      flags = O_RDONLY; /* read-only at current position */
      break;
                
    case XPL_FS_FILE_WRITE:
      flags = O_WRONLY | O_TRUNC | O_CREAT;  /* write-only truncate file */        
      break;
            
    case XPL_FS_FILE_RDWR:
      flags = O_RDWR| O_CREAT;  /* read and write at current position */
      break;
            
    case XPL_FS_FILE_APPEND:
      flags = O_WRONLY | O_APPEND | O_CREAT; /* read and write at current position */
      break;
    }        

    nFileHandle = (XPL_FS_HANDLE_T)open(file_uri, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    // XPL_LOG_DM_XPL_Debug(("DMFileHandler::open: opening file %s with mode flag %d, errno is %d, handle = %d\n",file_uri,(int)open_mode, errno, nFileHandle));
    if ( result && nFileHandle == -1 )
    {
      if (errno == EPERM) {   // permission failure
        *result = XPL_FS_RET_PERM_FAIL;
      }
      else {
        *result = XPL_FS_RET_FAIL;
      }
      XPL_LOG_DM_XPL_Error(("DMFileHandler::open: can't open file %s with mode flag %d errno is %d\n",file_uri,(int)open_mode,errno));
    } 
  
    if ( nFileHandle > 0 && open_mode != XPL_FS_FILE_READ )
    { 
      chmod(file_uri,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP); 
    }
    return nFileHandle;   
  
}


XPL_FS_RET_STATUS_T XPL_FS_Close(XPL_FS_HANDLE_T file_handle)
{
    int result =  close((int)file_handle);
    if ( result < 0 )
        return XPL_FS_RET_FAIL;
    else 
        return XPL_FS_RET_SUCCESS;
}


XPL_FS_COUNT_T XPL_FS_Read(XPL_FS_HANDLE_T file_handle,
                           void* buffer,
                           const XPL_FS_COUNT_T count,
                           XPL_FS_RET_STATUS_T * result)
{
    if ( result )
           *result = XPL_FS_RET_SUCCESS;
    XPL_FS_COUNT_T nRead = (XPL_FS_COUNT_T)read((int)file_handle,buffer,(int)count);
    if ( result && nRead == -1 )
           *result = XPL_FS_RET_FAIL;
    return nRead;
}    


XPL_FS_COUNT_T XPL_FS_Write(XPL_FS_HANDLE_T file_handle,
                            void* buffer,
                            const XPL_FS_COUNT_T count,
                            XPL_FS_RET_STATUS_T * result)
{
    if ( result )
          *result = XPL_FS_RET_SUCCESS;

    XPL_FS_COUNT_T nWrite = (XPL_FS_COUNT_T)write((int)file_handle,buffer,(int)count);
    if ( result && nWrite == -1 ) {
          XPL_LOG_DM_XPL_Error(("XPL_FS_Write: file_handle %d, nWrite = %d errno is %d\n",file_handle, nWrite, errno));
          *result = XPL_FS_RET_FAIL;
    }
    return nWrite;
}    


XPL_FS_SEEK_OFFSET_T XPL_FS_Seek(XPL_FS_HANDLE_T file_handle,
                                 XPL_FS_SEEK_OFFSET_T offset,
                                 XPL_FS_SEEK_MODE_T whence,
                                 XPL_FS_RET_STATUS_T * result)
{
    int whence_posix;
    XPL_FS_SEEK_OFFSET_T result_offset;

    if ( result )
        *result = XPL_FS_RET_SUCCESS;
    switch(whence)
    {
        case XPL_FS_SEEK_SET:
              whence_posix = SEEK_SET;
              break;
        case XPL_FS_SEEK_CUR:
              whence_posix = SEEK_CUR;
              break;
        case XPL_FS_SEEK_END:
              whence_posix = SEEK_END;
              break;
     }
     result_offset = lseek(file_handle,offset,whence_posix);
     if ( result && result_offset == -1)
         *result = XPL_FS_RET_FAIL;
     return result_offset;

}


XPL_FS_RET_STATUS_T XPL_FS_Remove(CPCHAR path)
{
    int result = remove(path);
    if ( result < 0 )
        return XPL_FS_RET_FAIL;
    else 
        return XPL_FS_RET_SUCCESS;
}

XPL_FS_RET_STATUS_T XPL_FS_Rename(CPCHAR old_name, CPCHAR new_name)
{
    int result = rename(old_name, new_name);

    // XPL_LOG_DM_XPL_Debug(("XPL_FS_Rename: old name %s  new name %s result = %d, errno is %d%d\n",old_name,new_name, errno, result));

    if ( result < 0 ) {
    	XPL_LOG_DM_XPL_Error(("XPL_FS_Rename error: old name %s  new name %s result = %d, errno is %d\n",old_name,new_name, errno, result));
        return XPL_FS_RET_FAIL;
    }
    else 
        return XPL_FS_RET_SUCCESS;
}

BOOLEAN XPL_FS_Exist(CPCHAR match_uri)
{
    struct  stat st;
    bool result;


    errno = 0;  // reset to 0
  
    if(stat(match_uri, &st) < 0)
      result = FALSE;
    else 
      result = TRUE;

    // XPL_LOG_DM_XPL_Debug(("XPL_FS_Exists: match uri %s, errno is %d result = %d\n",match_uri, errno, result));

    return result;
}

BOOLEAN XPL_FS_CheckPermission(CPCHAR match_uri, XPL_FS_OPEN_MODE_T permission)
{
    bool result = FALSE;
    int mode = 0;
    
    switch (permission) {
    case XPL_FS_RDONLY_MODE:
      mode = R_OK;
      break;
    case XPL_FS_WRONLY_MODE:
    case XPL_FS_RDWR_MODE:
    case XPL_FS_CREAT_MODE:
    case XPL_FS_TRUNC_MODE:
    case XPL_FS_APPEND_MODE:
      mode = R_OK | W_OK;
      break;
    default:
      break;
    }
     
    errno = 0;  // reset to 0
    if (access(match_uri, mode) == 0) {
      result = TRUE;
    }
    //    XPL_LOG_DM_XPL_Debug(("XPL_FS_CheckPermission: match uri %s, errno is %d result = %d\n",match_uri, errno, result));

    return result;
}


XPL_CLK_CLOCK_T XPL_FS_GetModTime(CPCHAR file_uri)
{
    struct stat st;
    
    if(stat( file_uri, &st) < 0)
      return 0;   // like it was never modified
    else
      return (XPL_CLK_CLOCK_T)st.st_mtime;
 }

XPL_FS_SIZE_T XPL_FS_GetSize(XPL_FS_HANDLE_T file_handle)
{
    struct stat st;

    if(fstat(file_handle, &st) < 0)
       return XPL_FS_SIZE_INVALID;
    return st.st_size;
}


XPL_FS_RET_STATUS_T XPL_FS_MkDir(CPCHAR dir_uri)
{
    mkdir(dir_uri, S_IRWXU|S_IRWXG|S_IRWXO);
    return XPL_FS_RET_SUCCESS;
}    

XPL_FS_RET_STATUS_T XPL_FS_Lock(XPL_FS_HANDLE_T file_handle, BOOLEAN bLockExclusive )
{
    struct flock fl;

    fl.l_type = bLockExclusive ? F_WRLCK : F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    if (fcntl(file_handle, F_SETLK, &fl) != -1) 
        return XPL_FS_RET_SUCCESS;
    else
        return (errno == EAGAIN ? XPL_FS_RET_TRYAGAIN : XPL_FS_RET_FAIL);
}

XPL_FS_RET_STATUS_T XPL_FS_Unlock(XPL_FS_HANDLE_T file_handle)
{
    struct flock fl;

    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    if (fcntl(file_handle, F_SETLK, &fl) != -1)
        return XPL_FS_RET_SUCCESS;
    else
        return XPL_FS_RET_FAIL;
}


XPL_FS_RET_STATUS_T XPL_FS_Unlink(CPCHAR file_name)
{
    if ( unlink(file_name) == 0 )
        return XPL_FS_RET_SUCCESS;
    else
        return XPL_FS_RET_FAIL;    
}



XPL_FS_SHANDLE_T XPL_FS_StartSearch(CPCHAR dir_uri, CPCHAR extension, BOOLEAN bFullName, XPL_FS_RET_STATUS_T * result)
{
     DIR *dir = XPL_NULL;
    
     if ( result )
         *result = XPL_FS_RET_SUCCESS;

     dir = opendir(dir_uri);
     if ( dir == NULL )
     {
        if ( result )
            *result = XPL_FS_RET_FAIL;

		return XPL_FS_SHANDLE_INVALID;
     } 

     DMDirHandler * pDirHandler = NULL; 
     pDirHandler = new DMDirHandler;

     if ( pDirHandler == NULL )
     {
        if ( result )
            *result = XPL_FS_RET_FAIL;

		closedir( dir );
        return XPL_FS_SHANDLE_INVALID; 
     }   

     if ( bFullName )
     {
        int nDirLen = strlen(dir_uri);
        if ( nDirLen >= XPL_FS_MAX_FILE_NAME_LENGTH )
        {
           return XPL_FS_SHANDLE_INVALID;
        }
        if (nDirLen>0)
        {
           pDirHandler->m_szDirectory = dir_uri;
           if ( dir_uri[nDirLen-1] != '/' )
           {
              pDirHandler->m_szDirectory += "/";
              ++nDirLen;
           }
           pDirHandler->m_nDirLen = nDirLen;
        }
     }   

     pDirHandler->m_szExtension = extension;
     pDirHandler->m_pDir = dir;

     return (XPL_FS_SHANDLE_T)(pDirHandler);        
}


XPL_FS_RET_STATUS_T XPL_FS_GetSearchResult(XPL_FS_SHANDLE_T search_handle, XPL_FS_SEARCH_FILE file_name)
{ 
    DMDirHandler * pDirHandler = (DMDirHandler*)search_handle;
    struct dirent *de = NULL;

	if ( !pDirHandler || search_handle == XPL_FS_SHANDLE_INVALID )
	  return XPL_FS_RET_FAIL;

    int nNameLen = 0;
    int nExtensionLen = strlen(pDirHandler->m_szExtension);
    de = readdir(pDirHandler->m_pDir);
    while ( de )
    {
        nNameLen = strlen(de->d_name);
        if (nNameLen > nExtensionLen && !strncmp((de->d_name + nNameLen - nExtensionLen), pDirHandler->m_szExtension, nExtensionLen))
        {
          if ( pDirHandler->m_nDirLen+nNameLen >= XPL_FS_MAX_FILE_NAME_LENGTH )
          {
	    return XPL_FS_RET_FAIL;
          }
          if ( pDirHandler->m_nDirLen>0 )
          {
            strcpy(file_name,pDirHandler->m_szDirectory);
          }
          strcpy(file_name+pDirHandler->m_nDirLen,(CPCHAR)de->d_name);
          return XPL_FS_RET_SUCCESS;          
        }
        else
        {
          de = readdir(pDirHandler->m_pDir);
        }
    }
    
    return XPL_FS_RET_NOT_FOUND;
}


XPL_FS_RET_STATUS_T XPL_FS_EndSearch(XPL_FS_SHANDLE_T search_handle)
{
    DMDirHandler * pDirHandler = (DMDirHandler*)search_handle;
	// DO not call closedir(...); since it's called from DirHandle destructor...
    delete pDirHandler;
    return XPL_FS_RET_SUCCESS;
}


UINT8 * XPL_FS_MemMap(XPL_FS_HANDLE_T file_handle, UINT32 size, UINT32 offset, XPL_FS_RET_STATUS_T * result)
{
    UINT8 * pBuffer = NULL; 

    if ( result )
        *result = XPL_FS_RET_SUCCESS;
   
    pBuffer = (UINT8*)mmap(0,size,PROT_READ,MAP_PRIVATE,file_handle,offset);
    if ( pBuffer == MAP_FAILED )
    {    
         if ( result) 
             *result = XPL_FS_RET_FAIL;
         return NULL;
    }

    return pBuffer;

}


XPL_FS_RET_STATUS_T XPL_FS_MemUnMap(UINT8 * pBuffer, UINT32 size)
{
    int res = munmap(pBuffer,size);
    if ( res == 0 )
        return XPL_FS_RET_SUCCESS;
    else
        return XPL_FS_RET_FAIL;
}
#ifdef LOB_SUPPORT
static const char  TEMP_ESN_DIR_NAME[]= "/data/data/com.android.omadm.service/tmp/";
CPCHAR XPL_FS_TempEsnDir()
{
 return TEMP_ESN_DIR_NAME;
}
XPL_FS_SIZE_T XPL_FS_FreeDiskSpace(CPCHAR pEsnDir)
{
  struct statfs sf;
  XPL_FS_SIZE_T retSize = 0;

  if(statfs(pEsnDir, &sf) == 0)
  {
    retSize = sf.f_bfree * sf.f_bsize;
  }
  return retSize;
}
#endif

#ifdef __cplusplus  
}
#endif
