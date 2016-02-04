/**
 * core.c - DesignWare USB3 DRD Controller Core file
 *
 * Copyright (C) 2010-2011 Texas Instruments Incorporated - http://www.ti.com
 *
 * Authors: Felipe Balbi <balbi@ti.com>,
 *	    Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>

#include <linux/usb/otg.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include <mach/hardware.h>
#include <mach/powergate.h>
#include "core.h"
#include "gadget.h"
#include "io.h"
#include "usb3_regs.h"
#include "debug.h"

static char *maximum_speed = "high";
module_param(maximum_speed, charp, 0);
MODULE_PARM_DESC(maximum_speed, "Maximum supported speed.");
static int dwc3_slew_rate =-1;
module_param(dwc3_slew_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dwc3_slew_rate, "dwc3_slew_rate");
static int dwc3_tx_bias=-1;
module_param(dwc3_tx_bias, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dwc3_tx_bias, "dwc3_tx_bias");
/* -------------------------------------------------------------------------- */
#define DWC3_DEVS_POSSIBLE	32
struct dwc3_port_info {
	void __iomem *usbecs;
	void __iomem *devrst;
	void __iomem *usbpll;
};
struct dwc3_actions {
	struct platform_device	*dwc3;
	struct device		*dev;

	struct dwc3_port_info port_info;	
	void __iomem        *base;
       int 				ic_type;
};
enum{
   IC_ATM7039C = 0,
   IC_ATM7059A
};
static int g_ic_type = IC_ATM7039C;

static DECLARE_BITMAP(dwc3_devs, DWC3_DEVS_POSSIBLE);

#if SUPPORT_NOT_RMMOD_USBDRV
static struct platform_device *pdev_dwc;
extern  int __dwc3_set_plugstate(struct dwc3	*dwc,int s);
int dwc3_set_plugstate(int s)
{
	struct dwc3	*dwc;
    
    	if(pdev_dwc ==NULL){
		printk("------can't get dwc3 platform device (structure)!!---\n");
        	return -ENODEV;
	}            
            
    	dwc = platform_get_drvdata(pdev_dwc);
	 __dwc3_set_plugstate(dwc,s);
     	return 0;
}
EXPORT_SYMBOL_GPL(dwc3_set_plugstate);

void (*dwc3_clk_open)(void );
void (*dwc3_clk_close)(void );
 void dwc3_set_usb_clk_ops(void (*clk_open)(void ),void (*clk_close)(void ))
{
     dwc3_clk_open = clk_open;
     dwc3_clk_close = clk_close;
}
EXPORT_SYMBOL_GPL(dwc3_set_usb_clk_ops);
#endif

int dwc3_get_device_id(void)
{
	int		id;

again:
	id = find_first_zero_bit(dwc3_devs, DWC3_DEVS_POSSIBLE);
	if (id < DWC3_DEVS_POSSIBLE) {
		int old;

		old = test_and_set_bit(id, dwc3_devs);
		if (old)
			goto again;
	} else {
		pr_err("dwc3: no space for new device\n");
		id = -ENOMEM;
	}

	return id;
}
EXPORT_SYMBOL_GPL(dwc3_get_device_id);

void dwc3_put_device_id(int id)
{
	int			ret;

	if (id < 0)
		return;

	ret = test_bit(id, dwc3_devs);
	WARN(!ret, "dwc3: ID %d not in use\n", id);
	smp_mb__before_clear_bit();
	clear_bit(id, dwc3_devs);
}
EXPORT_SYMBOL_GPL(dwc3_put_device_id);

void dwc3_set_mode(struct dwc3 *dwc, u32 mode)
{
	u32 reg;

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg &= ~(DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG));
	reg |= DWC3_GCTL_PRTCAPDIR(mode);
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);
}

/**
 * dwc3_core_soft_reset - Issues core soft reset and PHY reset
 * @dwc: pointer to our context structure
 */
static void dwc3_core_soft_reset(struct dwc3 *dwc,int phy_reset_en)
{
	u32		reg;

	/* Before Resetting PHY, put Core in Reset */
	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg |= DWC3_GCTL_CORESOFTRESET;
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);
	if(phy_reset_en){
		/* Assert USB3 PHY reset */
		reg = dwc3_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));
		reg |= DWC3_GUSB3PIPECTL_PHYSOFTRST;
		dwc3_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);

		/* Assert USB2 PHY reset */
		/* port 0 */
		reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));
		reg |= DWC3_GUSB2PHYCFG_PHYSOFTRST;
		dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);

	       if(g_ic_type == IC_ATM7059A){
			/* port 1 */
			reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(1));
			reg |= DWC3_GUSB2PHYCFG_PHYSOFTRST;
			dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(1), reg);
		}
#if 0
		usb_phy_init(dwc->usb2_phy);
		usb_phy_init(dwc->usb3_phy);
#endif
		mdelay(100);

		/* Clear USB3 PHY reset */
		reg = dwc3_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));
		reg &= ~DWC3_GUSB3PIPECTL_PHYSOFTRST;
		dwc3_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);

		/* Clear USB2 PHY reset */
		/* port 0 */
		reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));
		reg &= ~DWC3_GUSB2PHYCFG_PHYSOFTRST;
		dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);

		 if(g_ic_type == IC_ATM7059A){
			/* port 1 */
			reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(1));
			reg &= ~DWC3_GUSB2PHYCFG_PHYSOFTRST;
			dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(1), reg);
		}
		mdelay(100);
	}
	/* After PHYs are stable we can take Core out of reset state */
	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg &= ~DWC3_GCTL_CORESOFTRESET;
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);
}

/**
 * dwc3_free_one_event_buffer - Frees one event buffer
 * @dwc: Pointer to our controller context structure
 * @evt: Pointer to event buffer to be freed
 */
static void dwc3_free_one_event_buffer(struct dwc3 *dwc,
		struct dwc3_event_buffer *evt)
{
	dma_free_coherent(dwc->dev, evt->length, evt->buf, evt->dma);
}

/**
 * dwc3_alloc_one_event_buffer - Allocates one event buffer structure
 * @dwc: Pointer to our controller context structure
 * @length: size of the event buffer
 *
 * Returns a pointer to the allocated event buffer structure on success
 * otherwise ERR_PTR(errno).
 */
static struct dwc3_event_buffer *dwc3_alloc_one_event_buffer(struct dwc3 *dwc,
		unsigned length)
{
	struct dwc3_event_buffer	*evt;

	evt = devm_kzalloc(dwc->dev, sizeof(*evt), GFP_KERNEL);
	if (!evt)
		return ERR_PTR(-ENOMEM);

	evt->dwc	= dwc;
	evt->length	= length;
	/*   just to avoid bus reset bug*/
			
	evt->buf	= dma_alloc_coherent(dwc->dev, length,
			&evt->dma, GFP_KERNEL);
	if (!evt->buf)
		return ERR_PTR(-ENOMEM);

	return evt;
}

/**
 * dwc3_free_event_buffers - frees all allocated event buffers
 * @dwc: Pointer to our controller context structure
 */
static void dwc3_free_event_buffers(struct dwc3 *dwc)
{
	struct dwc3_event_buffer	*evt;
	int i;

	for (i = 0; i < dwc->num_event_buffers; i++) {
		evt = dwc->ev_buffs[i];
		if (evt)
			dwc3_free_one_event_buffer(dwc, evt);
	}
}

/**
 * dwc3_alloc_event_buffers - Allocates @num event buffers of size @length
 * @dwc: pointer to our controller context structure
 * @length: size of event buffer
 *
 * Returns 0 on success otherwise negative errno. In the error case, dwc
 * may contain some buffers allocated but not all which were requested.
 */
static int dwc3_alloc_event_buffers(struct dwc3 *dwc, unsigned length)
{
	int			num;
	int			i;

	num = DWC3_NUM_INT(dwc->hwparams.hwparams1);
	dwc->num_event_buffers = num;

	dwc->ev_buffs = devm_kzalloc(dwc->dev, sizeof(*dwc->ev_buffs) * num,
			GFP_KERNEL);
	if (!dwc->ev_buffs) {
		dev_err(dwc->dev, "can't allocate event buffers array\n");
		return -ENOMEM;
	}

	for (i = 0; i < num; i++) {
		struct dwc3_event_buffer	*evt;

		evt = dwc3_alloc_one_event_buffer(dwc, length);
		if (IS_ERR(evt)) {
			dev_err(dwc->dev, "can't allocate event buffer\n");
			return PTR_ERR(evt);
		}
		dwc->ev_buffs[i] = evt;
	}

	return 0;
}

/**
 * dwc3_event_buffers_setup - setup our allocated event buffers
 * @dwc: pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
//static int dwc3_event_buffers_setup(struct dwc3 *dwc)
int dwc3_event_buffers_setup(struct dwc3 *dwc)
{
	struct dwc3_event_buffer	*evt;
	int				n;

	for (n = 0; n < dwc->num_event_buffers; n++) {
		evt = dwc->ev_buffs[n];
		dev_dbg(dwc->dev, "Event buf %p dma %08llx length %d\n",
				evt->buf, (unsigned long long) evt->dma,
				evt->length);

		evt->lpos = 0;

		dwc3_writel(dwc->regs, DWC3_GEVNTADRLO(n),
				lower_32_bits(evt->dma));
		dwc3_writel(dwc->regs, DWC3_GEVNTADRHI(n),
				upper_32_bits(evt->dma));
		dwc3_writel(dwc->regs, DWC3_GEVNTSIZ(n),
				evt->length & 0xffff);
		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(n), 0);
	}

	return 0;
}

static void dwc3_event_buffers_cleanup(struct dwc3 *dwc)
{
	struct dwc3_event_buffer	*evt;
	int				n;

	for (n = 0; n < dwc->num_event_buffers; n++) {
		evt = dwc->ev_buffs[n];

		evt->lpos = 0;

		dwc3_writel(dwc->regs, DWC3_GEVNTADRLO(n), 0);
		dwc3_writel(dwc->regs, DWC3_GEVNTADRHI(n), 0);
		dwc3_writel(dwc->regs, DWC3_GEVNTSIZ(n), 0);
		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(n), 0);
	}
}

static void dwc3_core_num_eps(struct dwc3 *dwc)
{
	struct dwc3_hwparams	*parms = &dwc->hwparams;

	dwc->num_in_eps = DWC3_NUM_IN_EPS(parms);
	dwc->num_out_eps = DWC3_NUM_EPS(parms) - dwc->num_in_eps;

	dev_vdbg(dwc->dev, "found %d IN and %d OUT endpoints\n",
			dwc->num_in_eps, dwc->num_out_eps);
}

static void dwc3_cache_hwparams(struct dwc3 *dwc)
{
	struct dwc3_hwparams	*parms = &dwc->hwparams;

	parms->hwparams0 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS0);
	parms->hwparams1 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS1);
	parms->hwparams2 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS2);
	parms->hwparams3 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS3);
	parms->hwparams4 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS4);
	parms->hwparams5 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS5);
	parms->hwparams6 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS6);
	parms->hwparams7 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS7);
	parms->hwparams8 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS8);
}

static void setphy(unsigned char reg_add, unsigned char value, unsigned char port_num)
{
	void __iomem *usb3_usb_vcon=	NULL;
	volatile unsigned char addr_low;
	volatile unsigned char addr_high;
	volatile unsigned int vstate;

	if(g_ic_type == IC_ATM7059A){
		if (port_num == 0)
		usb3_usb_vcon = (void __iomem *)IO_ADDRESS((0xB0400000 + 0xCe00));
		else if (port_num == 1)
		usb3_usb_vcon = (void __iomem *)IO_ADDRESS((0xB0400000 + 0xCe10));
		else {
			printk("port_num must is 0 or 1!!\n");
			return;
		}
	}		
	else if(g_ic_type == IC_ATM7039C){
        	usb3_usb_vcon = (void __iomem *)IO_ADDRESS(USB3_USB2_P0_VDCTRL);
	}
	addr_low =  reg_add & 0x0f;
	addr_high =  (reg_add >> 4) & 0x0f;

	vstate = value;
	vstate = vstate << 8;

	addr_low |= 0x10;
	writel(vstate | addr_low, usb3_usb_vcon);
	mb();

	addr_low &= 0x0f; 
	writel(vstate | addr_low, usb3_usb_vcon);
	mb();

	addr_low |= 0x10;
	writel(vstate | addr_low, usb3_usb_vcon);
	mb();

	addr_high |= 0x10;
	writel(vstate | addr_high, usb3_usb_vcon);
	mb();

	addr_high &= 0x0f; 
	writel(vstate | addr_high, usb3_usb_vcon);
	mb();

	addr_high |= 0x10;
	writel(vstate | addr_high, usb3_usb_vcon);  
	mb();
	return;
}
#define USB3_UMON_FDT_COMPATIBLE_ATM7039C    "actions,atm7039c-usb"
#define USB3_UMON_FDT_COMPATIBLE_ATM7059TC  "actions,atm7059tc-usb"
#define USB3_UMON_FDT_COMPATIBLE_ATM7059A    "actions,atm7059a-usb"
static int dwc3_get_slewrate_config(void)
{
	struct device_node *fdt_node;
	const __be32 *property;
       int value;
       
	fdt_node = of_find_compatible_node(NULL, NULL, USB3_UMON_FDT_COMPATIBLE_ATM7039C);
	if (NULL == fdt_node) {
		fdt_node = of_find_compatible_node(NULL, NULL, USB3_UMON_FDT_COMPATIBLE_ATM7059TC);
		if (NULL == fdt_node) 
			fdt_node = of_find_compatible_node(NULL, NULL, USB3_UMON_FDT_COMPATIBLE_ATM7059A);
			if (NULL == fdt_node)
			{
				printk("<dwc3>err: no usb3-fdt-compatible\n");
				return -EINVAL;
			}
	}		
	property = of_get_property(fdt_node, "usb_hs_output_strength", NULL);
	value = be32_to_cpup(property);
	return value;
}
void dwc3_phy_init(u8 mode, unsigned char port_num)
{
	unsigned char val_u8,slew_rate;
    
       slew_rate= dwc3_get_slewrate_config();
       if((port_num!= 0)&&(port_num!= 1)){
		port_num = 0;
        }        
	if(g_ic_type == IC_ATM7039C){
		setphy(0xe7,0x0b, port_num);
		udelay(100);
		setphy(0xe7,0x0f, port_num);
		udelay(100);
		setphy(0xe2,0x74, port_num); 
		udelay(100);
	}
	else if(g_ic_type == IC_ATM7059A){

		if(mode == DWC3_MODE_DEVICE) {
			printk(" GS705A phy init for dwc3 gadget %s\n", __TIME__ );
			val_u8 =(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0);//select page1
			setphy(0xf4, val_u8, port_num);
			val_u8 =(1<<5) |(1<<4)|(0<<3)|(1<<2)|(1<<0);//negative sample
			setphy(0xe0, val_u8, port_num);
            
			val_u8 =(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0);//select page1
			setphy(0xf4, val_u8, port_num);
			val_u8 =(slew_rate<<5) |(0<<4)|(1<<3)|(1<<2)|(3<<0);//slewRate
			setphy(0xe1, val_u8, port_num);
            
			val_u8 =(1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0);//bit[6:5]=select page0
			setphy(0xf4, val_u8, port_num);
			val_u8 =(1<<7) |(4<<4)|(1<<3)|(0<<2)|(3<<0); //bit[3]= Enablepowerdown mode
			setphy(0xe6, val_u8, port_num);
            
			val_u8 = (1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0);
			setphy(0xf4, val_u8, port_num);
			val_u8 = (7<<4)|(0<<1)|(1<<0);                 //sensitivity lower
			setphy(0xe7, val_u8, port_num);
            
			val_u8 = (1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0);
			setphy(0xf4, val_u8, port_num);
			val_u8 = (9<<4)|(7<<0);                               //hstx current lower
			setphy(0xe4, val_u8, port_num);
		}
		else {
			printk(" GS705A phy init for xhci %s\n", __TIME__ );
			val_u8 =(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0);//select page1
			setphy(0xf4, val_u8, port_num);
			val_u8 =(1<<5) |(1<<4)|(0<<3)|(1<<2)|(1<<0);//negative sample
			setphy(0xe0, val_u8, port_num);
            
			val_u8 =(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0);//select page1
			setphy(0xf4, val_u8, port_num);
			if((dwc3_slew_rate >=0) &&(dwc3_slew_rate <= 7))
				val_u8 =(dwc3_slew_rate<<5) |(0<<4)|(1<<3)|(1<<2)|(3<<0);//slewRate
			else
				val_u8 =(3<<5) |(0<<4)|(1<<3)|(1<<2)|(3<<0);//slewRate
			setphy(0xe1, val_u8, port_num);
                    
			val_u8 =(1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0);//bit[6:5]=select page0
			setphy(0xf4, val_u8, port_num);
			val_u8 =(1<<7) |(4<<4)|(1<<3)|(0<<2)|(3<<0); //bit[3]= Enablepowerdown mode
			setphy(0xe6, val_u8, port_num);
            
			val_u8 = (1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0);
			setphy(0xf4, val_u8, port_num);
			val_u8 = (9<<4)|(0<<1)|(1<<0);                 //sensitivity lower
			setphy(0xe7, val_u8, port_num);
            
			val_u8 =(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0);//select page1
			setphy(0xf4, val_u8, port_num);
			val_u8 =(1<<5) |(1<<4)|(0<<3)|(0<<2)|(1<<0);// REG_CAL=0
			setphy(0xe0, val_u8, port_num);
            
			val_u8 = (1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0); // elect page0
			setphy(0xf4, val_u8, port_num);
			if((dwc3_tx_bias >=0) &&(dwc3_tx_bias <= 15))
				val_u8 = (0xf<<4)|(dwc3_tx_bias<<0);                                  //adjust hshd threshold and hstx current
			else
				val_u8 = (0xf<<4)|(4<<0);                                  //adjust hshd threshold and hstx current
			setphy(0xe4, val_u8, port_num);

			val_u8 = (1<<7) |(1<<6) |(1<<5)|(1<<4)|(1<<3)|(1<<2)|(0<<1)|(0<<0);
			setphy(0xf0, val_u8, port_num);   //disconnect enable
		}

	}

	return;
}
EXPORT_SYMBOL_GPL(dwc3_phy_init);
/*
 *fix bug:when usb3 bias enable ,reset machine will leak current by gpio 
 *if call this func will also disable super speed.
 */
void disable_bias(void)
{	
	struct dwc3 *dwc;
	u32 reg;
    
	dwc = platform_get_drvdata(pdev_dwc);
	reg = dwc3_readl(dwc->regs, 0xce04);
	reg |=(1<<27);
	dwc3_writel(dwc->regs, 0xce04, reg);
	reg = dwc3_readl(dwc->regs, ANA0F);
	reg &=~(1<<14);
	dwc3_writel(dwc->regs, ANA0F, reg);

}
EXPORT_SYMBOL_GPL(disable_bias);
void enable_bias(void)
{	
	struct dwc3 *dwc;
	u32 reg;
    
	dwc = platform_get_drvdata(pdev_dwc);
	
	reg = dwc3_readl(dwc->regs, ANA0F);
	reg |=(1<<14);
	dwc3_writel(dwc->regs, ANA0F, reg);

}
EXPORT_SYMBOL_GPL(enable_bias);

/**
 * dwc3_core_init - Low-level initialization of DWC3 Core
 * @dwc: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
//static int dwc3_core_init(struct dwc3 *dwc)
int dwc3_core_init(struct dwc3 *dwc,int phy_reset_en)
{
	unsigned long		timeout;
	u32			reg;
	int			ret;

	reg = dwc3_readl(dwc->regs, DWC3_GSNPSID);
	/* This should read as U3 followed by revision number */
	if ((reg & DWC3_GSNPSID_MASK) != 0x55330000) {
		dev_err(dwc->dev, "this is not a DesignWare USB3 DRD Core\n");
		ret = -ENODEV;
		goto err0;
	}
	dwc->revision = reg;

	/* issue device SoftReset too */
	timeout = jiffies + msecs_to_jiffies(500);
	dwc3_writel(dwc->regs, DWC3_DCTL, DWC3_DCTL_CSFTRST);
	do {
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		if (!(reg & DWC3_DCTL_CSFTRST))
			break;

		if (time_after(jiffies, timeout)) {
			dev_err(dwc->dev, "Reset Timed Out\n");
			ret = -ETIMEDOUT;
			goto err0;
		}

		cpu_relax();
	} while (true);

	if(g_ic_type == IC_ATM7059A){
		dwc3_writel(dwc->regs, ANA02, 0x6046);
		dwc3_writel(dwc->regs, ANA0E, 0x2010);
		dwc3_writel(dwc->regs, ANA0F, 0x8000);
		dwc3_writel(dwc->regs, REV1, 0x0);
		dwc3_writel(dwc->regs, PAGE1_REG02, 0x0013);
		dwc3_writel(dwc->regs, PAGE1_REG06, 0x0004);
		dwc3_writel(dwc->regs, PAGE1_REG07, 0x22ed);
		dwc3_writel(dwc->regs, PAGE1_REG08, 0xf802);
		dwc3_writel(dwc->regs, PAGE1_REG09, 0x3080);
		dwc3_writel(dwc->regs, PAGE1_REG0B, 0x2030);
		dwc3_writel(dwc->regs, ANA0F, (1<<14));

		dwc3_core_soft_reset(dwc,phy_reset_en);

		dwc3_cache_hwparams(dwc);
            //====force to high speed====
		reg = dwc3_readl(dwc->regs, DWC3_DCFG);
		reg &= ~(DWC3_DCFG_SPEED_MASK);
		reg |= DWC3_DCFG_HIGHSPEED;
		dwc3_writel(dwc->regs, DWC3_DCFG, reg);
	}
	else if(g_ic_type == IC_ATM7039C){
		/*
		 * donot need set this DWC3_BACKDOOR reg,
		 * this may cause some masstorge device transfer error!!, 
		 * add by hwliu 2013.07.22 .
		 */
		dwc3_core_soft_reset(dwc,phy_reset_en);

		reg = dwc3_readl(dwc->regs, DWC3_BACKDOOR);
		reg &= ~DWC3_FLADJ_30MHZ_MASK;	
		reg |= DWC3_FLADJ_30MHZ(0x20);
		dwc3_writel(dwc->regs, DWC3_BACKDOOR, reg);
	}

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg &= ~DWC3_GCTL_SCALEDOWN_MASK;
	reg &= ~DWC3_GCTL_DISSCRAMBLE;

	switch (DWC3_GHWPARAMS1_EN_PWROPT(dwc->hwparams.hwparams1)) {
	case DWC3_GHWPARAMS1_EN_PWROPT_CLK:
		reg &= ~DWC3_GCTL_DSBLCLKGTNG;
		break;
	default:
		dev_dbg(dwc->dev, "No power optimization available\n");
	}

	/*
	 * WORKAROUND: DWC3 revisions <1.90a have a bug
	 * where the device can fail to connect at SuperSpeed
	 * and falls back to high-speed mode which causes
	 * the device to enter a Connect/Disconnect loop
	 */
	if (dwc->revision < DWC3_REVISION_190A)
		reg |= DWC3_GCTL_U2RSTECN;

	dwc3_core_num_eps(dwc);

	dwc3_writel(dwc->regs, DWC3_GCTL, reg);

	return 0;

err0:
	return ret;
}

static void dwc3_core_exit(struct dwc3 *dwc)
{
#if 0
	usb_phy_shutdown(dwc->usb2_phy);
	usb_phy_shutdown(dwc->usb3_phy);
#endif
}

#define DWC3_ALIGN_MASK		(16 - 1)

static int dwc3_probe(struct platform_device *pdev)
{
	struct device_node	*node = pdev->dev.of_node;
	struct resource		*res;
	struct dwc3		*dwc;
	struct device		*dev = &pdev->dev;

	int			ret = -ENOMEM;

	void __iomem		*regs;
	void			*mem;

	u8			mode;

#if SUPPORT_NOT_RMMOD_USBDRV
	pdev_dwc = pdev;
#endif
	mem = devm_kzalloc(dev, sizeof(*dwc) + DWC3_ALIGN_MASK, GFP_KERNEL);
	if (!mem) {
		dev_err(dev, "not enough memory\n");
		return -ENOMEM;
	}
	dwc = PTR_ALIGN(mem, DWC3_ALIGN_MASK + 1);
	dwc->mem = mem;
       //to get ic type from dwc3-actions.ko,use resource method
       res = platform_get_resource(pdev, IORESOURCE_REG, 0);
	if (!res) {
		dev_err(dev, "missing IRQ\n");
		return -ENODEV;
	}
      g_ic_type = res->flags&0xff;
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "missing IRQ\n");
		return -ENODEV;
	}
	dwc->xhci_resources[1].start = res->start;
	dwc->xhci_resources[1].end = res->end;
	dwc->xhci_resources[1].flags = res->flags;
	dwc->xhci_resources[1].name = res->name;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "missing memory resource\n");
		return -ENODEV;
	}
	dwc->xhci_resources[0].start = res->start;
	dwc->xhci_resources[0].end = dwc->xhci_resources[0].start +
					DWC3_XHCI_REGS_END;
	dwc->xhci_resources[0].flags = res->flags;
	dwc->xhci_resources[0].name = res->name;

	 /*
	  * Request memory region but exclude xHCI regs,
	  * since it will be requested by the xhci-plat driver.
	  */
	res = devm_request_mem_region(dev, res->start + DWC3_GLOBALS_REGS_START,
			resource_size(res) - DWC3_GLOBALS_REGS_START,
			dev_name(dev));
	if (!res) {
		dev_err(dev, "can't request mem region\n");
		return -ENOMEM;
	}

	regs = devm_ioremap_nocache(dev, res->start, resource_size(res));
	if (!regs) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}

#if 0
	if (node) {
		dwc->usb2_phy = devm_usb_get_phy_by_phandle(dev, "usb-phy", 0);
		dwc->usb3_phy = devm_usb_get_phy_by_phandle(dev, "usb-phy", 1);
	} else {
		dwc->usb2_phy = devm_usb_get_phy(dev, USB_PHY_TYPE_USB2);
		dwc->usb3_phy = devm_usb_get_phy(dev, USB_PHY_TYPE_USB3);
	}

	if (IS_ERR(dwc->usb2_phy)) {
		ret = PTR_ERR(dwc->usb2_phy);

		/*
		 * if -ENXIO is returned, it means PHY layer wasn't
		 * enabled, so it makes no sense to return -EPROBE_DEFER
		 * in that case, since no PHY driver will ever probe.
		 */
		if (ret == -ENXIO)
			return ret;

		dev_err(dev, "no usb2 phy configured\n");
		return -EPROBE_DEFER;
	}

	if (IS_ERR(dwc->usb3_phy)) {
		ret = PTR_ERR(dwc->usb3_phy);

		/*
		 * if -ENXIO is returned, it means PHY layer wasn't
		 * enabled, so it makes no sense to return -EPROBE_DEFER
		 * in that case, since no PHY driver will ever probe.
		 */
		if (ret == -ENXIO)
			return ret;

		dev_err(dev, "no usb3 phy configured\n");
		return -EPROBE_DEFER;
	}

	usb_phy_set_suspend(dwc->usb2_phy, 0);
	usb_phy_set_suspend(dwc->usb3_phy, 0);
#endif

	owl_powergate_power_on(OWL_POWERGATE_USB3);
	spin_lock_init(&dwc->lock);
	platform_set_drvdata(pdev, dwc);

	dwc->regs	= regs;
	dwc->regs_size	= resource_size(res);
	dwc->dev	= dev;

	dev->dma_mask	= dev->parent->dma_mask;
	dev->dma_parms	= dev->parent->dma_parms;
	dma_set_coherent_mask(dev, dev->parent->coherent_dma_mask);

	if (!strncmp("super", maximum_speed, 5))
		dwc->maximum_speed = DWC3_DCFG_SUPERSPEED;
	else if (!strncmp("high", maximum_speed, 4))
		dwc->maximum_speed = DWC3_DCFG_HIGHSPEED;
	else if (!strncmp("full", maximum_speed, 4))
		dwc->maximum_speed = DWC3_DCFG_FULLSPEED1;
	else if (!strncmp("low", maximum_speed, 3))
		dwc->maximum_speed = DWC3_DCFG_LOWSPEED;
	else
		dwc->maximum_speed = DWC3_DCFG_SUPERSPEED;

	dwc->needs_fifo_resize = of_property_read_bool(node, "tx-fifo-resize");

	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
	pm_runtime_forbid(dev);

	dwc3_cache_hwparams(dwc);

	ret = dwc3_alloc_event_buffers(dwc, DWC3_EVENT_BUFFERS_SIZE);
	if (ret) {
		dev_err(dwc->dev, "failed to allocate event buffers\n");
		ret = -ENOMEM;
		goto err0;
	}

	ret = dwc3_core_init(dwc,0);
	if (ret) {
		dev_err(dev, "failed to initialize core\n");
		goto err0;
	}

	ret = dwc3_event_buffers_setup(dwc);
	if (ret) {
		dev_err(dwc->dev, "failed to setup event buffers\n");
		goto err1;
	}

	if (IS_ENABLED(CONFIG_USB_DWC3_HOST))
		mode = DWC3_MODE_HOST;
	else if (IS_ENABLED(CONFIG_USB_DWC3_GADGET))
		mode = DWC3_MODE_DEVICE;
	else
		mode = DWC3_MODE_DRD;
	mode = DWC3_MODE_DEVICE;	
	if(g_ic_type == IC_ATM7039C){
		dwc3_phy_init(mode, 0);
	}
	else if(g_ic_type == IC_ATM7059A){
		dwc3_phy_init(mode, 0);
		dwc3_phy_init(mode, 1);
	}
	switch (mode) {
	case DWC3_MODE_DEVICE:
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_DEVICE);
		ret = dwc3_gadget_init(dwc);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			goto err2;
		}
		break;
	case DWC3_MODE_HOST:
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_HOST);
		ret = dwc3_host_init(dwc);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			goto err2;
		}
		break;
	case DWC3_MODE_DRD:
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_OTG);
		ret = dwc3_host_init(dwc);
		if (ret) {
			dev_err(dev, "failed to initialize host\n");
			goto err2;
		}

		ret = dwc3_gadget_init(dwc);
		if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
			goto err2;
		}
		break;
	default:
		dev_err(dev, "Unsupported mode of operation %d\n", mode);
		goto err2;
	}
	dwc->mode = mode;

	ret = dwc3_debugfs_init(dwc);
	if (ret) {
		dev_err(dev, "failed to initialize debugfs\n");
		goto err3;
	}

	pm_runtime_allow(dev);

	return 0;

err3:
	switch (mode) {
	case DWC3_MODE_DEVICE:
		dwc3_gadget_exit(dwc);
		break;
	case DWC3_MODE_HOST:
		dwc3_host_exit(dwc);
		break;
	case DWC3_MODE_DRD:
		dwc3_host_exit(dwc);
		dwc3_gadget_exit(dwc);
		break;
	default:
		/* do nothing */
		break;
	}

err2:
	dwc3_event_buffers_cleanup(dwc);

err1:
	dwc3_core_exit(dwc);

err0:
	dwc3_free_event_buffers(dwc);

	return ret;
}

static int dwc3_remove(struct platform_device *pdev)
{
	struct dwc3	*dwc = platform_get_drvdata(pdev);

#if 0
	usb_phy_set_suspend(dwc->usb2_phy, 1);
	usb_phy_set_suspend(dwc->usb3_phy, 1);
#endif

	pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	dwc3_debugfs_exit(dwc);

	switch (dwc->mode) {
	case DWC3_MODE_DEVICE:
		dwc3_gadget_exit(dwc);
		break;
	case DWC3_MODE_HOST:
		dwc3_host_exit(dwc);
		break;
	case DWC3_MODE_DRD:
		dwc3_host_exit(dwc);
		dwc3_gadget_exit(dwc);
		break;
	default:
		/* do nothing */
		break;
	}

	dwc3_event_buffers_cleanup(dwc);
	dwc3_free_event_buffers(dwc);
	dwc3_core_exit(dwc);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
#if 0
static int dwc3_prepare(struct device *dev)
{
	struct dwc3	*dwc = dev_get_drvdata(dev);
	unsigned long	flags;

	spin_lock_irqsave(&dwc->lock, flags);

	switch (dwc->mode) {
	case DWC3_MODE_DEVICE:
	case DWC3_MODE_DRD:
		dwc3_gadget_prepare(dwc);
		/* FALLTHROUGH */
	case DWC3_MODE_HOST:
	default:
		dwc3_event_buffers_cleanup(dwc);
		break;
	}

	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;
}

static void dwc3_complete(struct device *dev)
{
	struct dwc3	*dwc = dev_get_drvdata(dev);
	unsigned long	flags;

	spin_lock_irqsave(&dwc->lock, flags);

	switch (dwc->mode) {
	case DWC3_MODE_DEVICE:
	case DWC3_MODE_DRD:
		dwc3_gadget_complete(dwc);
		/* FALLTHROUGH */
	case DWC3_MODE_HOST:
	default:
		dwc3_event_buffers_setup(dwc);
		break;
	}

	spin_unlock_irqrestore(&dwc->lock, flags);
}

static int dwc3_suspend(struct device *dev)
{
	struct dwc3	*dwc = dev_get_drvdata(dev);
	unsigned long	flags;

	spin_lock_irqsave(&dwc->lock, flags);

	switch (dwc->mode) {
	case DWC3_MODE_DEVICE:
	case DWC3_MODE_DRD:
		dwc3_gadget_suspend(dwc);
		/* FALLTHROUGH */
	case DWC3_MODE_HOST:
	default:
		/* do nothing */
		break;
	}

	dwc->gctl = dwc3_readl(dwc->regs, DWC3_GCTL);
	spin_unlock_irqrestore(&dwc->lock, flags);

	usb_phy_shutdown(dwc->usb3_phy);
	usb_phy_shutdown(dwc->usb2_phy);

	return 0;
}

static int dwc3_resume(struct device *dev)
{
	struct dwc3	*dwc = dev_get_drvdata(dev);
	unsigned long	flags;

	usb_phy_init(dwc->usb3_phy);
	usb_phy_init(dwc->usb2_phy);
	msleep(100);

	spin_lock_irqsave(&dwc->lock, flags);

	dwc3_writel(dwc->regs, DWC3_GCTL, dwc->gctl);

	switch (dwc->mode) {
	case DWC3_MODE_DEVICE:
	case DWC3_MODE_DRD:
		dwc3_gadget_resume(dwc);
		/* FALLTHROUGH */
	case DWC3_MODE_HOST:
	default:
		/* do nothing */
		break;
	}

	spin_unlock_irqrestore(&dwc->lock, flags);

	pm_runtime_disable(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	return 0;
}
#endif
static const struct dev_pm_ops dwc3_dev_pm_ops = {
	//.prepare	= dwc3_prepare,
	//.complete	= dwc3_complete,

	/*SET_SYSTEM_SLEEP_PM_OPS(dwc3_suspend, dwc3_resume)*/
};

#define DWC3_PM_OPS	&(dwc3_dev_pm_ops)
#else
#define DWC3_PM_OPS	NULL
#endif

#ifdef CONFIG_OF
static const struct of_device_id of_dwc3_match[] = {
	{
		.compatible = "synopsys,dwc3"
	},
	{ },
};
MODULE_DEVICE_TABLE(of, of_dwc3_match);
#endif

static struct platform_driver dwc3_driver = {
	.probe		= dwc3_probe,
	.remove		= dwc3_remove,
	.driver		= {
		.name	= "dwc3",
		.of_match_table	= of_match_ptr(of_dwc3_match),
		.pm	= DWC3_PM_OPS,
	},
};

module_platform_driver(dwc3_driver);

MODULE_ALIAS("platform:dwc3");
MODULE_AUTHOR("Felipe Balbi <balbi@ti.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("DesignWare USB3 DRD Controller Driver");
