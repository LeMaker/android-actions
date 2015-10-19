#ifndef __BUFFER_HANDLE__
#define __BUFFER_HANDLE__

#include <cutils/native_handle.h>
#include <system/window.h>



#ifdef __cplusplus
extern "C" {
#endif

void *bufferHandleLock(buffer_handle_t handle, int w, int h);

void bufferHandleUnlock(buffer_handle_t handle);
 
int bufferHandleAlloc(uint32_t w, uint32_t h,int format, int usage, buffer_handle_t* handle, int32_t* stride);

int bufferHandleAllocFree(buffer_handle_t handle);
#ifdef __cplusplus
}
#endif

#endif
    
