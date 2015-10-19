#ifndef DMMEMORY_H
#define DMMEMORY_H
#ifdef __cplusplus
extern "C" {
#endif

/************** HEADER FILE INCLUDES *****************************************/

#include "xpl_Types.h"
#include "dmStringUtil.h"


#define DmAllocMem(bufsize) DmAllocMemEx(bufsize,__FILE__,__LINE__)
#define DmFreeMem(buf) { DmFreeMemEx(buf,__FILE__,__LINE__); (buf) = NULL; }

void * DmAllocMemEx(UINT32 bufsize, CPCHAR szFile, int nLine );
void DmFreeMemEx(void *ptr, CPCHAR szFile, int nLine);
void* DmReallocMem( void* ptr, int nSize );

/* Low memory helpers */
int DmGetMemFailedFlag( void );
void DmResetMemFailedFlag( void );

#ifdef DM_MEMORY_STATISTICS_ENABLED

#define DM_MEMORY_STATISTICS_WRITE(header)  DmMemoryStatisticsWrite(header)
void DmMemoryStatisticsWrite(const char* header);

#define DM_MEMORY_STATISTICS_REPORT_LEAKS  DmMemoryStatisticsReportLeaks();
void DmMemoryStatisticsReportLeaks( void );

#else /*DM_MEMORY_STATISTICS_ENABLED*/

#define DM_MEMORY_STATISTICS_WRITE(header)  
#define DM_MEMORY_STATISTICS_REPORT_LEAKS  

#endif /*DM_MEMORY_STATISTICS_ENABLED*/

#ifdef __cplusplus
}
#endif
#endif /* DMMEMORY_H */

