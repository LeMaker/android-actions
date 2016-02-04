/**
 * hotplug_handle.c - actions-semi DWC3 Specific Glue layer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>
#include <linux/kallsyms.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <mach/hardware.h>
#include <mach/powergate.h>
#include "core.h"
#include "gadget.h"
#include "io.h"
#if SUPPORT_NOT_RMMOD_USBDRV
extern int dwc3_core_init(struct dwc3 *dwc,int phy_reset_en);
extern  int dwc3_event_buffers_setup(struct dwc3 *dwc);
extern void dwc3_phy_init(u8 mode, unsigned char port_num);
extern void (*dwc3_clk_open)(void );
extern void (*dwc3_clk_close)(void );
int dwc3_gadget_suspend(struct dwc3 *dwc);
int dwc3_gadget_resume(struct dwc3 *dwc);
static irqreturn_t dwc3_interrupt(int irq, void *_dwc);
static irqreturn_t dwc3_thread_interrupt(int irq, void *_dwc);
static int dwc3_gadget_run_stop(struct dwc3 *dwc, int is_on);
static void dwc3_gadget_usb2_phy_suspend(struct dwc3 *dwc, int suspend);
static void dwc3_gadget_usb3_phy_suspend(struct dwc3 *dwc, int suspend);
extern u32 reset_interrupt_occured ;
extern void disable_bias(void);

static int gadget_is_plugin =1;
static int dwc3_gadget_is_plugin(void)
{
	return gadget_is_plugin;
}


static int dwc3_gadget_plugout(struct dwc3 *dwc,int s)
{
       int cnt =500;
       int			irq;
       unsigned long	flags;
       
       spin_lock_irqsave(&dwc->lock, flags);
       gadget_is_plugin =0;
       //--step1------Waits until the Core Idle bit in DSTS is set------
       while(--cnt>0)
       {
           if(dwc3_readl(dwc->regs, DWC3_DSTS)&DWC3_DSTS_COREIDLE)
               break;
           udelay(100);
       }
       //-----step2--------stop core  ------------------------------------   
       if(s==PLUGSTATE_B_OUT)
		dwc->gadget_driver->disconnect(&dwc->gadget);
       dwc3_gadget_run_stop(dwc,0);
       dwc3_writel(dwc->regs, DWC3_DEVTEN, 0x00);   
       dwc->gadget.speed = USB_SPEED_UNKNOWN;

       //-----step3--------disable ep0  ------------------------------------ 
	dwc3_gadget_suspend(dwc);
       
       spin_unlock_irqrestore(&dwc->lock, flags);  
       irq = platform_get_irq(to_platform_device(dwc->dev), 0);
	free_irq(irq, dwc);

	return 0;
}

static int dwc3_gadget_plugin(struct dwc3 *dwc)
{
	int			ret;
       u32 reg;
       int			irq;
       unsigned long	flags;
       
	irq = platform_get_irq(to_platform_device(dwc->dev), 0);
	ret = request_threaded_irq(irq, dwc3_interrupt, dwc3_thread_interrupt,
		IRQF_SHARED | IRQF_ONESHOT, "dwc3", dwc);
	if (ret) {
		dev_err(dwc->dev, "failed to request irq #%d --> %d\n",
				irq, ret);
		return -EAGAIN ;
	}
       spin_lock_irqsave(&dwc->lock, flags);
       
#ifdef USB_CHARGE_DETECT       
       reset_interrupt_occured = 0;/*reset this flag every time plug in */
#endif
       //-----step1--------do dwc3_gadget_exit() things --------------
       reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg |= DWC3_DCFG_LPM_CAP;
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

	/* Enable all but Start and End of Frame IRQs */
	reg = (DWC3_DEVTEN_VNDRDEVTSTRCVEDEN |
			DWC3_DEVTEN_EVNTOVERFLOWEN |
			DWC3_DEVTEN_CMDCMPLTEN |
			DWC3_DEVTEN_ERRTICERREN |
			DWC3_DEVTEN_WKUPEVTEN |
			DWC3_DEVTEN_ULSTCNGEN |
			DWC3_DEVTEN_CONNECTDONEEN |
			DWC3_DEVTEN_USBRSTEN |
			DWC3_DEVTEN_DISCONNEVTEN);
	dwc3_writel(dwc->regs, DWC3_DEVTEN, reg);

	/* Enable USB2 LPM and automatic phy suspend only on recent versions */
	if (dwc->revision >= DWC3_REVISION_194A) {
		reg = dwc3_readl(dwc->regs, DWC3_DCFG);
		reg |= DWC3_DCFG_LPM_CAP;
		dwc3_writel(dwc->regs, DWC3_DCFG, reg);

		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~(DWC3_DCTL_HIRD_THRES_MASK | DWC3_DCTL_L1_HIBER_EN);

		/* TODO: This should be configurable */
		reg |= DWC3_DCTL_HIRD_THRES(28);

		dwc3_writel(dwc->regs, DWC3_DCTL, reg);

		dwc3_gadget_usb2_phy_suspend(dwc, false);
		dwc3_gadget_usb3_phy_suspend(dwc, false);
	}
    
      //-----step2--------set core run ------------------------------------      
       dwc3_gadget_run_stop(dwc,1);
      
       //-----step3-------enable&config ep0 -----------------------------
       reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_SPEED_MASK);
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

	dwc->start_config_issued = false;

	dwc3_gadget_resume(dwc);
    
	dwc->gadget.speed = USB_SPEED_UNKNOWN;
	gadget_is_plugin =1;
    
       spin_unlock_irqrestore(&dwc->lock, flags);  
	return 0;
}
int dwc3_plug_out(struct dwc3	*dwc,int s)
{
	if(((s == PLUGSTATE_B_OUT)||(s == PLUGSTATE_B_SUSPEND))&&(dwc->gadget_driver ==NULL))
		return 0;
	if((s == PLUGSTATE_B_OUT)||(s == PLUGSTATE_B_SUSPEND)){
		dwc3_gadget_plugout(dwc,s);
	}

	if(s == PLUGSTATE_A_OUT){
		dwc3_host_exit(dwc);
	}
	return 0;
}

 int dwc3_plug_in(struct dwc3	*dwc,int s)
{
	int ret=0;


	if((s == PLUGSTATE_B_IN)&&(dwc->gadget_driver == NULL))
		return 0;
    
       owl_powergate_power_on(OWL_POWERGATE_USB3);
	ret = dwc3_core_init(dwc,1);
	if (ret) {
		dev_err(dwc->dev, "failed to initialize core\n");
		return ret;
	}
	dwc3_event_buffers_setup(dwc);
	dwc3_phy_init(dwc->mode,0);
	dwc3_phy_init(dwc->mode,1);
	if(s == PLUGSTATE_A_IN){
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_HOST);
        	ret = dwc3_host_init(dwc);
		if (ret) {
			//dev_err(dev, "failed to initialize host\n");
			//goto err2;
			return 0;
		}
	}
	else if(s == PLUGSTATE_B_IN){
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_DEVICE);    
		ret =dwc3_gadget_plugin(dwc);
		disable_bias();
	}     
	pm_runtime_disable(dwc->dev);
	pm_runtime_set_active(dwc->dev);
	pm_runtime_enable(dwc->dev);

	return ret;	
    
}

/*
*plugstate:
*	0:	idle, nothing plugin or connect
*	1:	host, A_IN,  plug in udisk via otg wire
*	2:	device, B_IN, connect to pc
*/
int __dwc3_set_plugstate(struct dwc3	*dwc,int s)
{
   	if(dwc ==NULL){
		printk("---dwc3  device (structure) not exist!!---\n");
        	return -ENODEV;
	}        
	if((s==PLUGSTATE_A_OUT)||(s==PLUGSTATE_B_OUT)||(s==PLUGSTATE_B_SUSPEND)){    
		printk("\n----udc_set_plugstate--PLUGSTATE_OUT--\n");
        	dwc3_plug_out(dwc,s);
		if(owl_powergate_is_powered(OWL_POWERGATE_USB3))
			owl_powergate_power_off(OWL_POWERGATE_USB3);    
		if(dwc3_clk_close)
			dwc3_clk_close();
		
	}
	else if((s==PLUGSTATE_A_IN)||(s==PLUGSTATE_B_IN)){
		printk("\n----udc_set_plugstate--PLUGSTATE_IN-%d--\n",s);
		if(dwc3_clk_open)
			dwc3_clk_open();
		dwc3_plug_in(dwc,s);
	}
	return 0;
}


#endif

