#include <sys/mman.h>
#include "omx_malloc.h"
#include "omx_comp_debug_levels.h"
#include "log.h"

void *omx_malloc_phy(int len, unsigned long *vir_addr)
{
	unsigned long phy_addr = 0;
	int newlen = (len + 4095) & (~4095);

	*vir_addr = (unsigned long) actal_malloc_uncache(newlen, (unsigned long *) (&phy_addr));
	if (*vir_addr == 0)
	{
		DEBUG(DEB_LEV_ERR, "malloc ion addr err %lx\n", phy_addr);
		return NULL;
	}
	memset((void*) (*vir_addr), 0, newlen);

	DEBUG(DEB_LEV_ERR, "malloc ion addr! phy:%lx,vir:%lx,len:%x\n", phy_addr, *vir_addr, len);
	return (void*) phy_addr;
}

void omx_free_phy(void *phy_addr)
{
	void *vir_addr = (void*) actal_get_virtaddr((long) phy_addr);
	if (vir_addr != 0)
		actal_free_uncache(vir_addr);
	DEBUG(DEB_LEV_ERR, "free ion phyaddr!phy:%p vir:%p\n", phy_addr, vir_addr);
}

void* omx_mmap_ion_fd(int ion_fd, int length)
{
	void* vir_addr = NULL;
	vir_addr = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, ion_fd, 0);
	DEBUG(DEB_LEV_ERR, "get vir from ion fd!fd:%x vir:%p,len:%x\n", ion_fd, vir_addr, length);
	return vir_addr;
}

int omx_munmap_ion_fd(void * vir_addr, int length)
{
	int ret = 0;
	DEBUG(DEB_LEV_ERR, "munmap ion fd!vir:%p,len:%x\n", vir_addr, length);
	ret = munmap(vir_addr, length);
	if(ret != 0)	DEBUG(DEB_LEV_ERR, "err!munmap fail,ret:%d!%s,%d\n", ret, __FILE__, __LINE__);
	return ret;
}

#ifdef enable_gralloc
OMX_ERRORTYPE VCE_OSAL_GetPhyAddr(OMX_PTR handle,OMX_PTR *paddr)
{
	int ret = 0;
#ifdef IC_TYPE_GL5206
	ret = IGralloc_getPhys((void *)handle,paddr); /*GL5203*/
#else
#ifdef IC_TYPE_GL5207
	ret = Actions_OSAL_GetPhyAddr(handle,paddr);
#else/*IC_TYPE_GL5203*/
	ret = IGralloc_getPhys((void *)handle,paddr); /*GL5203*/
#endif/*IC_TYPE_GL5207*/
#endif/*IC_TYPE_GL5206*/

	if(ret != 0)
	{
		DEBUG(DEB_LEV_ERR,"err!VCE_OSAL_GetPhyAddr fail!handle:%p!%s,%d\n",(void *)handle,__FILE__,__LINE__);
		return OMX_ErrorUndefined;
	}

	return ret;
}

OMX_ERRORTYPE VCE_OSAL_GetBufInfo(OMX_PTR handle,OMX_PTR width,OMX_PTR height,
		OMX_PTR format,OMX_PTR size)
{
	int ret = 0;
#ifdef IC_TYPE_GL5206
	ret = IGralloc_getBufferInfo((void *)handle,(int *)width,(int *)height,(int *)format,(int *)size);  /*GL5203*/
#else
#ifdef IC_TYPE_GL5207
	ret = Actions_OSAL_GetBufInfo(handle,width,height,format,size);
#else/*IC_TYPE_GL5203*/
	ret = IGralloc_getBufferInfo((void *)handle,(int *)width,(int *)height,(int *)format,(int *)size);  /*GL5203*/
#endif/*IC_TYPE_GL5207*/
#endif/*IC_TYPE_GL5206*/

	if(ret != 0)
	{
		DEBUG(DEB_LEV_ERR,"err!VCE_OSAL_GetBufInfo fail!handle:%p!%s,%d\n",(void *)handle,__FILE__,__LINE__);
		return OMX_ErrorUndefined;
	}

	return ret;
}

/*
//≤‚ ‘
static int lock_cnt = 0;
static int unlock_cnt = 0;
*/

OMX_ERRORTYPE VCE_OSAL_LockANBHandleWidthUsage(OMX_PTR handle,OMX_U32 width,
		OMX_U32 height,OMX_U32 usage,OMX_PTR *vaddr)
{
	int ret = 0;
#ifdef IC_TYPE_GL5206
	ret = IGralloc_lock((void *)handle,usage,vaddr);  /*GL5203*/
#else
#ifdef IC_TYPE_GL5207
	ret = Actions_OSAL_LockANBHandleWidthUsage(handle,width,height,usage,vaddr);
#else/*IC_TYPE_GL5203*/
	ret = IGralloc_lock((void *)handle,usage,vaddr);  /*GL5203*/
#endif/*IC_TYPE_GL5207*/
#endif/*IC_TYPE_GL5206*/

	/*ock_cnt++;*/

	if(ret != 0)
	{
		DEBUG(DEB_LEV_ERR,"err!VCE_OSAL_LockANBHandleWidthUsage fail!handle:%p!%s,%d\n",(void *)handle,__FILE__,__LINE__);
		return OMX_ErrorUndefined;
	}

	return ret;
}

OMX_ERRORTYPE VCE_OSAL_UnlockANBHandle(OMX_PTR handle)
{
	int ret = 0;
#ifdef IC_TYPE_GL5206
	ret = IGralloc_unlock((void *)handle);  /*GL5203*/
#else
#ifdef IC_TYPE_GL5207
	ret = Actions_OSAL_UnlockANBHandle(handle);
#else/*IC_TYPE_GL5203*/
	ret = IGralloc_unlock((void *)handle);  /*GL5203*/
#endif/*IC_TYPE_GL5207*/
#endif/*IC_TYPE_GL5206*/

	/*
	unlock_cnt++;
	DEBUG(DEB_LEV_ERR,"lock_cnt:%d,unlock_cnt:%d\n",lock_cnt,unlock_cnt);
	*/

	if(ret != 0)
	{
		DEBUG(DEB_LEV_ERR,"err!VCE_OSAL_UnlockANBHandle fail!handle:%p!%s,%d\n",(void *)handle,__FILE__,__LINE__);
		return OMX_ErrorUndefined;
	}

	return ret;
}
#endif
