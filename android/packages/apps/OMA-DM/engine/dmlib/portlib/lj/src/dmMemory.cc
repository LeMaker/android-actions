#include "dmAllocatedPointersPool.h"
#include <malloc.h>
#include <stdio.h>  
#include "xpl_Logger.h"
#include "dmMemory.h"

#ifdef DEBUG
DMAllocatedPointersPool *s_pAllocatedPool = new DMAllocatedPointersPool;

// SU API functions - only used in SyncML DM code
const int c_nMemPrefix = 3*sizeof(const char*) + DMAllocatedPointersPool::c_nExtraBytes;
const int c_nMemPostfix = DMAllocatedPointersPool::c_nExtraBytes;
#endif

//#define MEASURE_MEMORY
#ifdef DM_MEMORY_STATISTICS_ENABLED
int g_nMaxMem = 0;
int g_nCurMem = 0;
int g_nNumber = 0;

void DmMemoryStatisticsWrite(const char* header)
{
}

void DmMemoryStatisticsReportLeaks( void )
{
}

#endif

int DmGetMemFailedFlag( void )
{
  return 0;
}
void DmResetMemFailedFlag( void )
{
}

#ifdef DEBUG

int s_nBlocks = 0, s_nSize = 0;
int s_nCnt = 0;



void* DmAllocMemEx(UINT32 bufsize, CPCHAR szFile, int nLine )
{
  s_nBlocks ++;
  s_nSize += (bufsize + 3) / 4 * 4;
  
  // emulate low memory case
  static int s_nFirstBlocks = 4*1024 +1;

  s_nCnt++;

  if ( !s_nFirstBlocks ) {
    if ( (s_nCnt % 1024) == 0)
      ; //return NULL;
  }
  else
    s_nFirstBlocks--;
  

  if ( (s_nCnt % 102400) == 0 )
  {
    XPL_LOG_DM_TMN_Debug(("Total blocks %d, Blocks %d, size %d\n\n", s_nCnt, s_nBlocks, s_nSize ));
  }

 
  char* ptr = (char*)malloc(bufsize + c_nMemPrefix + c_nMemPostfix);
  if (ptr == 0) 
  {
    XPL_LOG_DM_TMN_Debug(("Malloc failed. Memory not available. %d asked by %s In %d. \n",
                       bufsize,szFile, nLine));
  }
  else 
  {
#ifdef DM_MEMORY_STATISTICS_ENABLED
    g_nCurMem += (bufsize + 3) / 4 * 4;
    g_nMaxMem = (g_nMaxMem > g_nCurMem ? g_nMaxMem : g_nCurMem);
    g_nNumber++;
#endif
   
    s_pAllocatedPool->append(ptr);
    memset(ptr,0xcd,bufsize + c_nMemPrefix + c_nMemPostfix);

    CPCHAR* ppStr = (CPCHAR*)ptr;
    ppStr[0] = strdup( szFile );
    ppStr[1] = (CPCHAR)nLine;
    ppStr[2] = (CPCHAR)(long)bufsize;

    ptr += c_nMemPrefix;
    memset(ptr,0,bufsize);
    XPL_LOG_DM_TMN_Debug(("Allocating %d memory at %p in file %s In %d. \n",
                           bufsize, ptr, szFile, nLine));
  }
  return(ptr);
}

void DmFreeMemEx(void *ptr, CPCHAR szFile, int nLine)
{
  unsigned char* bufptr = (unsigned char*)ptr;
  if(bufptr != 0 ){
    bufptr -= c_nMemPrefix;
    
    if ( s_pAllocatedPool->remove(bufptr)) {
      // check over/under run of the buffer
      bool bCorrupted = false;
      CPCHAR* ppStr = (CPCHAR*)bufptr;
      for ( int i = 1; i <= DMAllocatedPointersPool::c_nExtraBytes; i++ )
        {
          if ( bufptr[c_nMemPrefix-i] != 0xcd ||
               bufptr[c_nMemPrefix + (int)ppStr[2] + i - 1] != 0xcd )
            bCorrupted = true;
        }

  s_nBlocks --;
  s_nSize -= ((int)ppStr[2] + 3) / 4 * 4;
#ifdef DM_MEMORY_STATISTICS_ENABLED
      g_nCurMem -= ((int)ppStr[2] + 3) / 4 * 4;
      g_nNumber--;
#endif
      
      if ( bCorrupted )
      {
        XPL_LOG_DM_TMN_Debug(("Block has been corrupted with writing before/after the allocated block; pointer %p; file %c  ln. %d size %d \n", 
        						(void*) (bufptr + c_nMemPrefix), ppStr[0], (int) ppStr[1], (int) ppStr[2]));
      }
      free( (void*)ppStr[0] );
      memset(ptr, 0xcd, (int)ppStr[2] ); 
      free(bufptr);
    }
    else 
    {
      XPL_LOG_DM_TMN_Debug(("Invalid or duplicate freeing %p memory. infile %s ln. %d \n",  
      						(void*) (bufptr + c_nMemPrefix), szFile, nLine));
    } 
  }
}

void* DmReallocMem( void* ptr, int nSize )
{
  // debug version:

  if ( nSize <= 0 ){
    DmFreeMem( ptr );
    return NULL;
  }

  if ( !ptr )
    return DmAllocMem( nSize );

  // check for buf size - if it registered...
  unsigned char* bufptr = (unsigned char*)ptr;
  if(bufptr != 0 )
    bufptr -= c_nMemPrefix;
    
  if ( !s_pAllocatedPool->exists(bufptr)) 
  {
    // invalid input parameter!!!
      XPL_LOG_DM_TMN_Debug(("Realloc is called with invalid input parameter: block %p  was not allocated or already was freed \n",  
      						(void*) (bufptr + c_nMemPrefix)));
    return DmAllocMem( nSize );
  }

  CPCHAR* ppStr = (CPCHAR*)bufptr;
  int nBlockSize =  (int)ppStr[2];

  if ( nBlockSize >=  nSize )
    return ptr; // nothing to do - current block is large enough

  void* pNewBlock = DmAllocMem( nSize );

  if ( pNewBlock ) 
    memcpy( pNewBlock, ptr,(nBlockSize < nSize ? nBlockSize : nSize) );
  
  DmFreeMem(ptr);
  return pNewBlock;
}

#else //!DEBUG

void* DmReallocMem(void *ptr, int nSize)
{
  return realloc(ptr,nSize);
}     

void * DmAllocMemEx(UINT32 bufsize, CPCHAR szFile, int nLine )
{
  char* ptr = (char*)malloc(bufsize);
  if (ptr != 0) {
    memset(ptr,0,bufsize);
  }
  return ptr;
}

void DmFreeMemEx(void *ptr, CPCHAR szFile, int nLine)
{
  if (ptr) {
     free( ptr );
  }
}

#endif //DEBUG


#ifdef DM_MEMORY_STATISTICS_ENABLED
extern "C" void suPrintMemoryUsage() 
{
  XPL_LOG_DM_TMN_Debug(("Memory usage summary: allocated %d block(s) with total %d bytes. Max usage was %d \n",   
      						g_nNumber, g_nCurMem, g_nMaxMem));
}
#endif

#ifdef DEBUG
struct CMemoryStatusPrint
{
  ~CMemoryStatusPrint() {
    s_pAllocatedPool->PrintUnreleased();
  }
}s_oMemoryStatus;
#endif

