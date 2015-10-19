#include <unistd.h> 
#include <cutils/log.h> 
#include <ui/GraphicBufferMapper.h> 
#include <ui/GraphicBufferAllocator.h> 
#include <ui/Rect.h> 

#include <hardware/gralloc_priv.h>
#include <hardware/gralloc.h>

#include "buffer_handle.h"


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


int bufferHandleAlloc(uint32_t w, uint32_t h,int format, int usage, buffer_handle_t* handle, int32_t* stride)
{
    int ret = 0;
    android::GraphicBufferAllocator &GrallocAlloc = android::GraphicBufferAllocator::get();
    buffer_handle_t buf;
    ret = GrallocAlloc.alloc(w, h, format, usage, handle, stride);
    return ret;
}

int bufferHandleAllocFree(buffer_handle_t handle)
{
    int ret = 0;
    android::GraphicBufferAllocator &GrallocAlloc = android::GraphicBufferAllocator::get();
    ret =GrallocAlloc.free(handle);
    return ret;
    
}



#ifdef __cplusplus
}
#endif
    
