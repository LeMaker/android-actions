#ifndef  __LINUX_USB_AOTG_UDC_DEBUG_H__ 
#define  __LINUX_USB_AOTG_UDC_DEBUG_H__ 

#include "aotg.h"
#include "aotg_udc.h"

extern unsigned int aotg_trace_onff;
#define	AOTG_TRACE_ERR_PLACE	if (aotg_trace_onff) printk("-%d\n", __LINE__);

void aotg_dbg_put_info(char *info_h, char *info0, unsigned int info1, unsigned int info2, unsigned int info3);
void aotg_dbg_output_info(void);

void create_acts_udc_proc(void);
void remove_acts_udc_proc(void);

void aotg_dbg_ep_queue_list(struct aotg_ep *ep);

#endif /* __LINUX_USB_AOTG_DEBUG_H__ */ 
 
