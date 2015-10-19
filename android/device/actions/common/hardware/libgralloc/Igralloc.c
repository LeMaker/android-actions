#include"Igralloc.h"

#include <utils/Log.h>

static gralloc_module_t const* mAllocMod=NULL;
void GetModule()
{
  
	hw_module_t const* module;
	int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);

	if (err == 0) {
		mAllocMod = (gralloc_module_t const *)module;
	}else {
		ALOGE("FATAL: can't find the %s module", GRALLOC_HARDWARE_MODULE_ID);
	}
}

int  IGralloc_lock(void* handle,int usage,void **vaddr)
{
	//ALOGD("IGralloc_lock, handle %p",handle);
  if(mAllocMod==NULL) 
  	GetModule();
  	return mAllocMod->lock(mAllocMod, (buffer_handle_t)handle,usage,0,0,0,0,vaddr);
}

int  IGralloc_unlock(void * handle)
{
   if(mAllocMod==NULL) 
  	GetModule();
  	return mAllocMod->unlock(mAllocMod,(buffer_handle_t)handle);
}

int  IGralloc_getPhys(void * handle,void **paddr)
{
	if(mAllocMod==NULL) 
	  GetModule();
	  return mAllocMod->getPhysAddr(mAllocMod,(buffer_handle_t)handle,paddr);
}

int  IGralloc_getBufferInfo(void * handle, //buffer_handle_t
					 int *width, int *height, int *format, int *size)
{
	 if(mAllocMod==NULL) 
	GetModule();
	return mAllocMod->getBufferInfo(mAllocMod,(buffer_handle_t)handle,width,height,format,size, 0);
}

int  IGralloc_stretchBlit(buffer_handle_t src, buffer_handle_t dest,
						  android_native_rect_t* psSrcRect, 
						  android_native_rect_t* psDestRect,
						  int eRotation)
{
	if(mAllocMod==NULL) 
	 GetModule();
	return mAllocMod->stretchBlit(mAllocMod,src,dest,psSrcRect,psDestRect,eRotation);
}

int  IGralloc_stretchBlit2( buffer_handle_t src, void *dest, int format, int w, int h,
							   android_native_rect_t *psSrcRect, 
							   android_native_rect_t *psDestRect,
							   int eRotation)
{
	if(mAllocMod==NULL) 
	 GetModule();
	return mAllocMod->stretchBlit2(mAllocMod,src,dest,format,w,h,psSrcRect,psDestRect,eRotation);

}
  



