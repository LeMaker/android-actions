#ifndef Actions_OSAL_ANDROID
#define Actions_OSAL_ANDROID



#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Index.h"

typedef enum{
	OMX_ACT_OSAL_USAGE_RDONLY = 0,
	OMX_ACT_OSAL_USAGE_WDONLY,
	OMX_ACT_OSAL_USAGE_RW
}act_osal_usage_t;

#ifdef __cplusplus
extern "C" {
#endif



//Get phyAddr from Gralloc handle
OMX_ERRORTYPE Actions_OSAL_GetPhyAddr(
    OMX_IN  OMX_PTR handle,
    OMX_OUT OMX_PTR *paddr);
    
//Get buffer info from Gralloc handle
OMX_ERRORTYPE Actions_OSAL_GetBufInfo(					  
    OMX_IN OMX_PTR handle,
    OMX_OUT OMX_PTR width,
    OMX_OUT OMX_PTR height,
    OMX_OUT OMX_PTR format,
    OMX_OUT OMX_PTR size);

//Get virAddr by buffer_handle_t lock in
OMX_ERRORTYPE Actions_OSAL_LockANBHandle(
    OMX_IN OMX_PTR handle/*buffer_handle_t*/,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_OUT OMX_PTR *vaddr);

//Get virAddr by buffer_handle_t lock by usage
OMX_ERRORTYPE Actions_OSAL_LockANBHandleWidthUsage(
    OMX_IN OMX_PTR handle/*buffer_handle_t*/,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_U32 usage,
    OMX_OUT OMX_PTR *vaddr);
        
//Get virAddr by buffer_handle_t lock out
OMX_ERRORTYPE Actions_OSAL_UnlockANBHandle(OMX_IN OMX_PTR handle);

#ifdef __cplusplus
}
#endif

#endif
