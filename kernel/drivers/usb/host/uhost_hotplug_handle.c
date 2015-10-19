#define SUPPORT_NOT_RMMOD_USBDRV 1
#if SUPPORT_NOT_RMMOD_USBDRV

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/slab.h>

enum plugstate{
	PLUGSTATE_A_OUT=0,
  PLUGSTATE_B_OUT,
	PLUGSTATE_A_IN,
	PLUGSTATE_B_IN,
	PLUGSTATE_A_SUSPEND,
	PLUGSTATE_A_RESUME,	
};

extern int xhci_resume_host(void);
extern int xhci_suspend_host(void);

 int xhci_set_plugstate(int state)
{
		if((state==PLUGSTATE_A_SUSPEND)){	
			    printk("\n----xhci_set_plugstate--HOST_SUSPEND--\n");				  		
				xhci_suspend_host();
		}
		else if((state==PLUGSTATE_A_RESUME)){  
			    printk("\n----xhci_set_plugstate--HOST_RESUME--\n");				  		
				xhci_resume_host();
		}
		return 0;
}
EXPORT_SYMBOL_GPL(xhci_set_plugstate);

#endif