#include <hardware/gralloc.h>
#include <hardware/hardware.h>

#include <stdint.h>
#include <errno.h>

#include <system/window.h>
#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
@lzou
*****************************************************************************

*****************************************************************************
 Name:		IGralloc_lock

 Purpose:	to get a buffer virtuall address and lock buffer not free

 Inputs:	handle - the buffer handle
              usage - the attrib of the buffer, read or write

 Outputs:	vaddr- the bufffer virtual address

 Returns:	0 if sucess ,others fail
*****************************************************************************/

int IGralloc_lock(void* handle,int usage,void **vaddr);
/*****************************************************************************
 Name:		IGralloc_unlock

 Purpose:	to unlock buffer

 Inputs:	handle - the buffer handle

 Outputs:	no

 Returns:	0 if sucess ,others fail
*****************************************************************************/

int IGralloc_unlock(void* handle);
 /*****************************************************************************
  Name: 	 IGralloc_getPhys
 
  Purpose:	 to get physical address of a  buffer
 
  Inputs:	 handle - the buffer handle
 
  Outputs:	 no
 
  Returns:	 0 if sucess ,others fail
 *****************************************************************************/

 int IGralloc_getPhys(void* handle,void **paddr);
 /*****************************************************************************
  Name: 	 IGralloc_getBufferInfo
 
  Purpose:	 get buffer info like width,height,format,size
 
  Inputs:	 handle - the buffer handle
 
  Outputs:	 width,height,format,size
 
  Returns:	 0 if sucess ,others fail
 *****************************************************************************/
 int IGralloc_getBufferInfo(void* handle, 
						 int *width, int *height, int *format, int *size);
 /*****************************************************************************
  Name: 	 IGralloc_stretchBlit
 
  Purpose:	 copy src buffer to dest buffer
 
  Inputs:	 src- the source buffer handle
               dest - the dest buffer handle
               psSrcRect - source rectangle you want to copy
               psDestRect - dest rectangle you want to copy to
               eRotation - rotate angles frome source to dest
 
  Outputs:	 no
  Returns:	 0 if sucess ,others fail
 *****************************************************************************/
 int IGralloc_stretchBlit(buffer_handle_t src, buffer_handle_t dest,
							  android_native_rect_t* psSrcRect, 
							  android_native_rect_t* psDestRect,
							  int eRotation);
 /*****************************************************************************
  Name: 	 IGralloc_stretchBlit2
 
  Purpose:	 copy src buffer to dest buffer
 
  Inputs:	   src - the buffer handle
               dest - virtual adrress of the dest buffer
               format - dest buffer format
               w - width of the dest buffer
               h - height of the dest buffer
               psSrcRect - source rectangle you want to copy
               psDestRect - dest rectangle you want to copy to
               eRotation - rotate angles frome source to dest
 
  Outputs:	 no
  Returns:	 0 if sucess ,others fail
 *****************************************************************************/
 int IGralloc_stretchBlit2(buffer_handle_t src, void *dest, int format, int w, int h,
								   android_native_rect_t *psSrcRect, 
								   android_native_rect_t *psDestRect,
								   int eRotation);
#ifdef __cplusplus
}
#endif



