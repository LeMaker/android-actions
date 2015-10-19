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

#include "dmdefs.h"
#include "SyncML_DM_FileHandle.H"
#include "dm_uri_utils.h"
#include "dm_tree_class.H"

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

/* The .temp file extention is used for the archive being written
 * the .bak file extention is used during the process of replacing
 * the old with the new archive
 */

/* The wchar_t standard and handy L"string"; usage are not supported by the ARM compiler. */
const char DMFileHandler::TEMP_FILE_EXTENSION[] = ".temp";
const char DMFileHandler::BAK_FILE_EXTENSION[]= ".bak";
const char DMFileHandler::COMMIT_LOG_EXTENSION[]= ".cmt";
const char DMFileHandler::LOG_FILE_EXTENSION[]= ".log";
const char DMFileHandler::MIDDLE_FILE_EXTENSION[]= ".mid";

/*==================================================================================================

Function:    DMFileHandler::DMFileHandler

Description: Constructor method for the DMFileHandler class

==================================================================================================*/
DMFileHandler::DMFileHandler(CPCHAR path)
{
    init(path);  
}


DMFileHandler::DMFileHandler(CPCHAR path, BOOLEAN bIsCache)
{
     init(path);
     m_bIsCache = bIsCache;
}    


void DMFileHandler::init(CPCHAR path)
{
  m_pInternalBuf    = NULL;
  m_nInternalBufLen = 0;
  m_nBufPos = 0;
  m_nFilePos = 0;
  m_nBufOffset = 0;
  m_nFileHandle = -1;
  m_strPath = path;
  m_bWrite = FALSE; 
  m_bIsCache = TRUE;
  m_pMemMap = NULL;
  m_nSize = 0;

}

/*==================================================================================================

Function:    DMFileHandler::DMFileHandler

Description: Destructor method for the DMFileHandler class

==================================================================================================*/
DMFileHandler::~DMFileHandler()
{
    close();
    unmmap();
    if ( m_pInternalBuf )
        DmFreeMem(m_pInternalBuf);
}


/*==================================================================================================

Function:    DMFileHandler::open

Description: Opens the file represented by this object for I/O

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMFileHandler::open(INT32 modeFlag)
{
  XPL_FS_RET_STATUS_T result;

  m_nMode = modeFlag; 
  m_nFileHandle = XPL_FS_Open(m_strPath.c_str(), modeFlag, &result);

  /* Reset the internal buffer length since we've opened a new file */
  m_nInternalBufLen = 0;
  m_nBufPos = 0;
  m_nFilePos = 0;
  m_nBufOffset =0;	
  m_bWrite = FALSE;

  if ( !m_pInternalBuf && m_bIsCache )
    m_pInternalBuf = (char *)DmAllocMem(MAX_INTERNAL_BUFFER_LENGTH);
  
  if(m_nFileHandle >= 0 )
    return SYNCML_DM_SUCCESS;

  if (result == XPL_FS_RET_PERM_FAIL)
    return  SYNCML_DM_COMMAND_NOT_ALLOWED;
 
  return SYNCML_DM_IO_FAILURE;
}

/*==================================================================================================

Function:    DMFileHandler::read

Description: Reads count bytes into buffer

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMFileHandler::read(char* buffer, UINT16 count)
{
  XPL_FS_RET_STATUS_T  result;
  
  if ( m_nFileHandle < 0 )
    return SYNCML_DM_IO_FAILURE;

  if ( m_nMode == XPL_FS_FILE_WRITE ||
        m_nMode == XPL_FS_FILE_APPEND )
    return SYNCML_DM_IO_FAILURE;        

  if ( !m_pInternalBuf ){
    int nRead = XPL_FS_Read(m_nFileHandle, buffer, count,&result);
    return nRead == count ? SYNCML_DM_SUCCESS : SYNCML_DM_IO_FAILURE;
  }

  int nRead = min( count, m_nInternalBufLen-m_nBufPos );
  if ( nRead > 0 )
  {
      memcpy( buffer, m_pInternalBuf+m_nBufPos, nRead);
      m_nBufPos += nRead;
  }    
  if ( m_nBufPos == m_nInternalBufLen && count > nRead)
  { // need more data
    m_nBufPos = 0;
    if (m_nMode == XPL_FS_FILE_RDWR )
      flush();
        
    m_nInternalBufLen = XPL_FS_Read(m_nFileHandle, m_pInternalBuf, MAX_INTERNAL_BUFFER_LENGTH, &result);
    if ( m_nInternalBufLen <= 0 )
      return SYNCML_DM_IO_FAILURE;
    
    m_nBufOffset = m_nFilePos;
    m_nFilePos += m_nInternalBufLen;
    return read( buffer + nRead, count -nRead ); 
  }
  return SYNCML_DM_SUCCESS;
}

/*==================================================================================================

Function:    DMFileHandler::write

Description: Writes count bytes into buffer.  This method implements a double buffering scheme.
             Writes less than MAX_INTERNAL_BUFFER_LENGTH are placed in a RAM queue.  Large writes
         are sent to the filesystem in blocks of size MAX_INTERNAL_BUFFER_LENGTH.

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMFileHandler::write(const char* buffer, UINT16 count)
{
   //XPL_LOG_DM_TMN_Debug(("DMFileHandler::write:  handle is %d, name = %s\n",m_nFileHandle, m_strPath.c_str()));

   if ( m_nFileHandle < 0 )
    return SYNCML_DM_IO_FAILURE;

   if ( m_nMode == XPL_FS_FILE_READ )
    return SYNCML_DM_IO_FAILURE;       
   
  if ( !m_pInternalBuf )
    m_pInternalBuf  = (char *)DmAllocMem(MAX_INTERNAL_BUFFER_LENGTH);

  if ( !m_pInternalBuf || m_nFileHandle < 0 )
    return SYNCML_DM_IO_FAILURE;

  int nWriteCount = min( count, MAX_INTERNAL_BUFFER_LENGTH-m_nBufPos);

  if ( nWriteCount > 0 )
  {
    m_bWrite = TRUE;
    memcpy(m_pInternalBuf + m_nBufPos, buffer, nWriteCount);
    m_nBufPos += nWriteCount;
    if ( m_nBufPos > m_nInternalBufLen )
      m_nInternalBufLen = m_nBufPos;
  }  
 
  if ( m_nInternalBufLen == MAX_INTERNAL_BUFFER_LENGTH && m_nBufPos == MAX_INTERNAL_BUFFER_LENGTH )
  {
    if(flush() != SYNCML_DM_SUCCESS)
      return SYNCML_DM_IO_FAILURE;
  
    if ( count > nWriteCount )
      return write(buffer + nWriteCount, count - nWriteCount);
  }
  
  return SYNCML_DM_SUCCESS;
}

/*==================================================================================================

Function:    DMFileHandler::writeBuffer

Description: Writes count bytes to the filesystem.  No double buffering is involved.

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMFileHandler::writeBuffer(const char* buffer, UINT16 count)
{
  XPL_FS_RET_STATUS_T result;
  int nWritten = XPL_FS_Write( m_nFileHandle, (char*)buffer, count, &result );
  m_nFilePos += nWritten;
  m_nBufOffset += nWritten;
  if ( nWritten == count )
  {
    m_nInternalBufLen = 0;
    m_nBufPos = 0;
    m_bWrite = FALSE; 
    return SYNCML_DM_SUCCESS;
  }
  else
  {
      // TODO
    return SYNCML_DM_IO_FAILURE;
  }  
}

/*==================================================================================================

Function:    DMFileHandler::seek

Description: Seek offset bytes relative to seekFrom

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMFileHandler::seek(XPL_FS_SEEK_MODE_T whence, INT32 offset)
{
  if ( m_nFileHandle < 0 )
    return SYNCML_DM_IO_FAILURE;

  
  SYNCML_DM_RET_STATUS_T ret_stat;
  XPL_FS_SEEK_OFFSET_T file_pos;
  XPL_FS_RET_STATUS_T result;
  

  if (m_nMode != XPL_FS_FILE_READ )
    if((ret_stat = this->flush()) != SYNCML_DM_SUCCESS)
      return ret_stat;
   
  if ( (file_pos = XPL_FS_Seek(m_nFileHandle, offset, whence,&result)) >=0  )
  {
    if ( m_nInternalBufLen > 0 )
    {
// Cache should be cleanred when seeking to a new position. 
//       if ( file_pos >= m_nFilePos || file_pos < m_nBufOffset )
//       {
         m_nInternalBufLen = 0;
         m_nBufPos = 0;
         m_nBufOffset = file_pos;
//       }
//       else
//       { 
//         m_nBufPos = file_pos-m_nBufOffset;
//       }  
    } 
    else
       m_nBufOffset =  file_pos;
      
    m_nFilePos = file_pos;
    return SYNCML_DM_SUCCESS;
  }    

  return SYNCML_DM_IO_FAILURE;
}

/*============================================================================================
======

Function:    DMFileHandler::flush

Description: Flushes the internal write buffer if it exists into the filesystem
         component.

Notes:       The file system may not actually flush the data to the file itself
         as the file system component seems to maintain its own internal buffers

==============================================================================================
====*/
SYNCML_DM_RET_STATUS_T
DMFileHandler::flush()
{
    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;
    if(m_nInternalBufLen > 0 && m_bWrite)
    {
      if ( m_nBufOffset != m_nFilePos )
        XPL_FS_Seek(m_nFileHandle, m_nBufOffset, XPL_FS_SEEK_SET,NULL);
      ret_stat = writeBuffer(m_pInternalBuf, m_nInternalBufLen);
    }
    return ret_stat;
}

/*==================================================================================================

Function:    DMFileHandler::close

Description: Close the file handle

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMFileHandler::close()
{
    if ( m_nFileHandle < 0 )
      return SYNCML_DM_FAIL;

    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;

    if ( m_nMode != XPL_FS_FILE_READ )
        ret_stat = flush();
   
    XPL_FS_Close(m_nFileHandle);
    m_nFileHandle = -1;
    if ( ret_stat == XPL_FS_RET_SUCCESS ) 
        return SYNCML_DM_SUCCESS;
    else
        return SYNCML_DM_IO_FAILURE; 
}
/*==================================================================================================

Function:    DMFileHandler::rename

Description: Create an unique External Storage Node file name

         The function argument is the full path name of the file.

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
DMFileHandler::createTempESNFileName(DMString &fileName)
{
    char uniqueStr[20];
    XPL_CLK_LONG_CLOCK_T curTime = XPL_CLK_GetClockMs();
    DmSprintf(uniqueStr, "%lld", curTime);
    if(uniqueStr[0] == '-')
       uniqueStr[0]='L';
   
    dmTreeObj.GetWritableFileSystemFullPath(fileName);
    CPCHAR pT = fileName.c_str();
    if (pT[fileName.length()-1] != '/')
      fileName += "/";

    XPL_FS_MkDir(fileName);
    fileName += uniqueStr;
    fileName += ".lob";
    fileName += MIDDLE_FILE_EXTENSION;
    return SYNCML_DM_SUCCESS;
}
/*==================================================================================================

Function:    DMFileHandler::rename

Description: Rename a file to new name.
             On Win32 systems, the file must be closed to properly rename, however;
             P2K requires that the filehandle be open.  We handle that with preprocessor
             logic here.

         The function argument is the full path name of the file.

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMFileHandler::rename(CPCHAR newFileName)
{
  
  this->close();
  XPL_FS_RET_STATUS_T ret_stat; 

  ret_stat = XPL_FS_Rename(m_strPath, newFileName);
  if ( ret_stat == XPL_FS_RET_SUCCESS ) 
  {
     m_strPath = newFileName;
     return SYNCML_DM_SUCCESS;
  }
  else
     return SYNCML_DM_IO_FAILURE;
}

/*==================================================================================================

Function:    DMFileHandler::deleteFile

Description: Delete a file

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMFileHandler::deleteFile()
{
  XPL_FS_RET_STATUS_T ret_stat; 

  close();
  ret_stat = XPL_FS_Remove(m_strPath);
  if ( ret_stat == XPL_FS_RET_SUCCESS ) 
     return SYNCML_DM_SUCCESS;
  else
     return SYNCML_DM_IO_FAILURE; 
}

/*==================================================================================================

Function:    DMFileHandler::getSize

Description: Returns size of file

==================================================================================================*/
XPL_FS_SIZE_T 
DMFileHandler::size()
{
    if ( m_nFileHandle < 0 )
        return 0;

    if ( m_nMode != XPL_FS_FILE_READ || m_nSize == 0 )
        m_nSize = XPL_FS_GetSize(m_nFileHandle);
 
    return m_nSize;
}


/*==================================================================================================*/

XPL_FS_SEEK_OFFSET_T
DMFileHandler::position()
{
    return m_nFilePos;
}

#ifndef DM_NO_LOCKING
/*==================================================================================================

Function:    DMFileHandler::lock

Description: locks the file

==================================================================================================*/
SYNCML_DM_RET_STATUS_T DMFileHandler::lock(BOOLEAN bLockExclusive )
{
    XPL_FS_RET_STATUS_T result;
    result = XPL_FS_Lock(m_nFileHandle, bLockExclusive);
    if ( result == XPL_FS_RET_SUCCESS )
        return SYNCML_DM_SUCCESS;
    else
        if ( result == XPL_FS_RET_TRYAGAIN )
            return SYNCML_DM_LOCK_TRY_AGAIN;
        else
            return SYNCML_DM_IO_FAILURE;
}

/*==================================================================================================

Function:    DMFileHandler::unlock

Description: unlocks the file

==================================================================================================*/
SYNCML_DM_RET_STATUS_T DMFileHandler::unlock()
{
    XPL_FS_RET_STATUS_T result;
    result = XPL_FS_Unlock(m_nFileHandle);
    if ( result == XPL_FS_RET_SUCCESS )
        return SYNCML_DM_SUCCESS;
    else
        return SYNCML_DM_IO_FAILURE;
}

#endif

/*==================================================================================================

Function:    DMFileHandler::fgets

Description: read one more string from the file and returns or null if error

==================================================================================================*/
char* DMFileHandler::fgets( char* s, INT32 size )
{
  if ( size < 1 || !s )
    return NULL;
  
  s[0] = 0;
  if ( iseof() )
    return NULL;

  char c = 0;
  INT32 nPos = 0;
  BOOLEAN bIsValid = FALSE;
  
  while ( read( &c, 1 ) == SYNCML_DM_SUCCESS && (nPos +1 ) < size ) 
  {
    if ( c == '\n' )
      break;
  
    if ( c == '\r' )
      continue;
  
    if ( c == '\t' )
       c = ' ';

    if ( c != ' ' )
       bIsValid = TRUE;
    else
       if ( !bIsValid )
          continue;
    
    s[nPos] = c;
    nPos++;

  }
 
  while ( nPos > 0 && s[nPos-1] == ' ' )
    nPos--;

  s[nPos] = 0;
  
  return s;
}

/*==================================================================================================

Function:    DMFileHandler::iseof

Description: returns true if end of file reached

==================================================================================================*/
BOOLEAN DMFileHandler::iseof()
{

  size();
  if ( !m_pInternalBuf ) 
  {
     m_nFilePos = XPL_FS_Seek(m_nFileHandle, 0, XPL_FS_SEEK_CUR,NULL);
     if ( m_nFilePos < 0 )
        return TRUE; 
     return (((INT32)m_nSize == m_nFilePos));
  }   
  else
  {
     return ( ((INT32)m_nSize == m_nFilePos) && (m_nBufPos == m_nInternalBufLen) );
  }   
}

UINT8 * DMFileHandler::mmap()
{
    XPL_FS_RET_STATUS_T result;
    if ( m_pMemMap == NULL )
    {
        if ( m_nFileHandle < 0 )
            return NULL;


        if ( m_nMode != XPL_FS_FILE_READ  )
            return NULL;

        size();
        if ( m_nSize == 0 )
            return NULL;

        m_pMemMap = XPL_FS_MemMap(m_nFileHandle,m_nSize,0,&result);
    }     

    return m_pMemMap;    
}

SYNCML_DM_RET_STATUS_T DMFileHandler::unmmap()
{
    XPL_FS_RET_STATUS_T result;
    if ( m_pMemMap )
    {
        result = XPL_FS_MemUnMap(m_pMemMap,m_nSize);
        if ( result == XPL_FS_RET_SUCCESS )
            return SYNCML_DM_SUCCESS;
        else
            return SYNCML_DM_IO_FAILURE;
    }    
    else
        return SYNCML_DM_SUCCESS; 
}
