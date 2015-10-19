#include <unistd.h> 
#include <cutils/log.h> 
#include <ui/GraphicBufferMapper.h> 
#include <ui/Rect.h> 

#include <hardware/gralloc_priv.h>
#include <hardware/gralloc.h>

#include "buffer_handle_mapper.h"


#ifdef __cplusplus
extern "C" {
#endif

void *bufferHandleLock(buffer_handle_t handle, int w, int h)
{
    unsigned char *vaddr = NULL;

    android::Rect bounds;
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = w;
    bounds.bottom = h;
    
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
    mapper.lock(handle, 0, bounds, (void **)&vaddr);
    return vaddr;
    
}

void bufferHandleUnlock(buffer_handle_t handle)
{
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();
    mapper.unlock(handle);
}

void *getPhys(buffer_handle_t handle){
    if(handle == NULL){
        return NULL;
    }
    //fixe me: private_handle_t no longer open to  gralloc module user, it's not accessible by user
    //gralloc module provide extra interfaces for user to get virtual and physical address
    // should modify later
    return (void *)((private_handle_t *)handle)->phys_addr;
}

#ifdef __cplusplus
}
#endif
    
