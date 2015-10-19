#include <stdio.h>
#include <stdlib.h>

#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <media/hardware/HardwareAPI.h>
#include <hardware/hardware.h>
#include <media/hardware/OMXPluginBase.h>
#include <hardware/gralloc.h>
#include "Actions_OSAL_Android.h"

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif

//#define GRALLOC_HARDWARE_MUDULE_ID 0

OMX_ERRORTYPE Actions_OSAL_GetPhyAddr(
    OMX_IN  OMX_PTR handle,
    OMX_OUT OMX_PTR *paddr)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;   
    buffer_handle_t bufferHandle = (buffer_handle_t)handle;
    hw_module_t const * module;
    struct gralloc_module_t const *gralloc_module;     
		hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);		
		gralloc_module = reinterpret_cast<struct gralloc_module_t const*>(module);
		if(gralloc_module->getPhysAddr(gralloc_module,bufferHandle,(void**)paddr)){
			ret = OMX_ErrorUndefined;
		}

    return ret;
}

OMX_ERRORTYPE Actions_OSAL_GetBufInfo(					  
    OMX_IN  OMX_PTR handle,
    OMX_OUT OMX_PTR width,
    OMX_OUT OMX_PTR height,
    OMX_OUT OMX_PTR format,
    OMX_OUT OMX_PTR size)
{

    OMX_ERRORTYPE ret = OMX_ErrorNone;   
     
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;
    hw_module_t const * module;
    struct gralloc_module_t const *gralloc_module;    
		hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);		
		gralloc_module = reinterpret_cast<struct gralloc_module_t const*>(module);
		if(gralloc_module->getBufferInfo(gralloc_module,bufferHandle,(int*)width,(int*)height,(int*)format,(int*)size, NULL)){
			ret = OMX_ErrorUndefined;
		}

    return ret;
}
OMX_ERRORTYPE Actions_OSAL_LockANBHandleWidthUsage(
    OMX_IN OMX_PTR handle/*buffer_handle_t*/,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_U32 usage,
    OMX_OUT OMX_PTR *vaddr) {
    	
		OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;
    Rect bounds(width, height);

    int usages = 0;
    
//#define OMX_ACT_OSAL_USAGE_RDONLY 0x0
//#define OMX_ACT_OSAL_USAGE_WDONLY 0x1
//#define OMX_ACT_OSAL_USAGE_RW 0x2
    
    if(usage == OMX_ACT_OSAL_USAGE_RW){
    	usages = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
    }else if(usage == OMX_ACT_OSAL_USAGE_RDONLY){
    	usages = GRALLOC_USAGE_SW_READ_OFTEN;
    }else if(usage == OMX_ACT_OSAL_USAGE_WDONLY){
    	usages = GRALLOC_USAGE_SW_WRITE_OFTEN;
    }

    if (mapper.lock(bufferHandle, usages, bounds, vaddr) != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }    

EXIT:
    return ret;    	
    	
  }
  
OMX_ERRORTYPE Actions_OSAL_LockANBHandle(
    OMX_IN OMX_PTR handle,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_OUT OMX_PTR *vaddr)
{

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;
    Rect bounds(width, height);

    int usage = 0;
    usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;

    if (mapper.lock(bufferHandle, usage, bounds, vaddr) != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }    

EXIT:
    return ret;
}

OMX_ERRORTYPE Actions_OSAL_UnlockANBHandle(OMX_IN OMX_PTR handle)
{

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;

     if (mapper.unlock(bufferHandle) != 0) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }


EXIT:

    return ret;
}

#ifdef __cplusplus
}
#endif
