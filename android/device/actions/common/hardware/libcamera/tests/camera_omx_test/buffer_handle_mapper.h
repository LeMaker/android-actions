#ifndef __BUFFER_HANDLE_MAPPER__
#define __BUFFER_HANDLE_MAPPER__

#include <cutils/native_handle.h>
#include <system/window.h>



#ifdef __cplusplus
extern "C" {
#endif

void *bufferHandleLock(buffer_handle_t handle, int w, int h);

void bufferHandleUnlock(buffer_handle_t handle);
 
void *getPhys(buffer_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
    
