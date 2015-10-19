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

#ifndef SYNCML_DM_FILEHANDLE_H
#define SYNCML_DM_FILEHANDLE_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "syncml_dm_data_types.h"
#include "xpl_File.h"
#include "dmMemory.h"
#include "dmstring.h"

class DMFileHandler
{
  public:

    /* File name extensions for serialization */
    static const char TEMP_FILE_EXTENSION[];
    static const char BAK_FILE_EXTENSION[];
    static const char COMMIT_LOG_EXTENSION[];
    static const char LOG_FILE_EXTENSION[];
    static const char MIDDLE_FILE_EXTENSION[];
    enum { MAX_INTERNAL_BUFFER_LENGTH = 1024};

    
    /* Constructor for the class takes absolute path of the file */
    DMFileHandler(CPCHAR path);

    DMFileHandler(CPCHAR path, BOOLEAN bIsCache);

    /* Frees the internal buffer */
    ~DMFileHandler();

    /* Opens a file for I/O */
    SYNCML_DM_RET_STATUS_T open(INT32 mode);

    /* Reads count bytes into buffer */
    inline SYNCML_DM_RET_STATUS_T read(UINT8* buffer, UINT16 count) {return read((char*)( buffer ), count);}
    SYNCML_DM_RET_STATUS_T read(char* buffer, UINT16 count);

    /* Writes count bytes into the buffer/file 
     * For the purposes of performance, this function 
     * adds small writes to a buffer before sending to the 
     * filesystem.*/
    inline SYNCML_DM_RET_STATUS_T write(const UINT8* buffer, UINT16 count) {return write((const char*)( buffer ), count);}
    SYNCML_DM_RET_STATUS_T write(const char* buffer, UINT16 count);

    /* Seek offset bytes relative to seekFrom */
    SYNCML_DM_RET_STATUS_T seek(XPL_FS_SEEK_MODE_T whence, INT32 offset);

    /* Close the file handle */
    SYNCML_DM_RET_STATUS_T close();

    /* Rename the file to name */
    SYNCML_DM_RET_STATUS_T rename(CPCHAR newFileName);

    /* Delete the file */
    SYNCML_DM_RET_STATUS_T deleteFile();

    /* Create an unique External Storage Node file name */
    static SYNCML_DM_RET_STATUS_T createTempESNFileName(DMString &fileName);

    /* Returns file size */
    XPL_FS_SIZE_T size();
    
    // FILE* api functions
    char*  fgets( char* s, INT32 size );

    BOOLEAN iseof();
    
    /* Flushes the internal write buffer if it exists */
    SYNCML_DM_RET_STATUS_T flush();

    UINT8 * mmap();

    SYNCML_DM_RET_STATUS_T unmmap();

    XPL_FS_SEEK_OFFSET_T position();

 #ifndef DM_NO_LOCKING
    SYNCML_DM_RET_STATUS_T lock(BOOLEAN bLockExclusive );
 
    SYNCML_DM_RET_STATUS_T unlock();
#endif    
 
    inline const char* getFullPath() const { return m_strPath.c_str(); }

    inline void* operator new(size_t sz)
    {
       return (DmAllocMem(sz));
    }
        
    inline void operator delete(void* buf)
    {
      DmFreeMem(buf);
    }

  private:

    /* The file handle reference */
    XPL_FS_HANDLE_T m_nFileHandle;

    /* Absolute path of the file pointed to by the file handle */
    DMString m_strPath;

    /* Internal buffer data members */
    char * m_pInternalBuf;
    UINT8 * m_pMemMap;  
    UINT16 m_nInternalBufLen;
    UINT16 m_nBufPos;  // reading operations required 2 int: buf size and pos
    XPL_FS_SIZE_T m_nSize;
  
    XPL_FS_SEEK_OFFSET_T m_nFilePos;
    XPL_FS_SEEK_OFFSET_T m_nBufOffset;    
    BOOLEAN m_bWrite; 
    BOOLEAN m_bIsCache;
    BOOLEAN m_bIsMap;
    XPL_FS_OPEN_MODE_T m_nMode; 
  
    /* Writes count bytes into the file.
     * This method does the actual writes to the filesystem  */
    SYNCML_DM_RET_STATUS_T writeBuffer(const char* buffer, UINT16 count); 
    
    void init(CPCHAR path);
};

#endif /* SYNCML_DM_FILEHANDLE_H */
