#include <malloc.h>
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include "xpl_Memory.h"


#ifdef __cplusplus  
extern "C" {
#endif

void * xplAllocMemEx(UINT32 bufsize, CPCHAR szFile, int nLine )
{
  char* ptr = (char*)malloc(bufsize);
  if (ptr != 0) {
    memset(ptr,0,bufsize);
  }
  return ptr;
}

void xplFreeMemEx(void *ptr, CPCHAR szFile, int nLine)
{
  if (ptr) {
     free( ptr );
  }
}

#ifdef __cplusplus  
}
#endif

