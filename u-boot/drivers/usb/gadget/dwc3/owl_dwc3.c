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

#if 0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>

#include <linux/usb/otg.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include <mach/hardware.h>
#include "core.h"
#include "gadget.h"
#include "io.h"
#include "usb3_regs.h"
#include "debug.h"

#endif


#include <common.h>
#include <config.h>
//#include <ubi_uboot.h>
#include <linux/err.h>
#include <asm/io.h>
#include <linux/usb/ch9.h>
#include <linux/compat.h>
#include <usb/lin_gadget_compat.h>
#include <malloc.h>
#include <asm/arch/actions_reg_owl.h>

#include "owl_dwc3.h"
#include "owl_ep0.c"
#include "owl_gadget.c"


/* Init values for USB endpoints. */
static const struct usb_ep dwc3_ep_init[2] = {
	[0] = {	/* EP 0 */
		.maxpacket	= 64,
		.name		= "ep0",
		.ops		= &dwc3_gadget_ep0_ops,
	},
	[1] = {	/* EP 1..n */
		.maxpacket	= 512,
		.name		= "ep-",
		.ops		= &dwc3_gadget_ep_ops,
	},
};

static struct dwc3 act_dwc3= {
	.gadget	= {
		.name	= "owl-dwc3",
		.ops	= &dwc3_gadget_ops,
		//.is_dualspeed = 1,
	},
};

#define DWC3_DEVS_POSSIBLE	32
#if 0
static char *maximum_speed = "high";
module_param(maximum_speed, charp, 0);
MODULE_PARM_DESC(maximum_speed, "Maximum supported speed.");

static bool host_config = 0;
module_param(host_config, bool, S_IRUGO);
MODULE_PARM_DESC(host_config, "device or host mode select");
/* -------------------------------------------------------------------------- */
#define DWC3_DEVS_POSSIBLE	32
struct dwc3_port_info {
	void __iomem *usbecs;
	void __iomem *devrst;
	void __iomem *usbpll;
};
struct dwc3_owl {
	struct platform_device	*dwc3;
	struct device		*dev;

	struct dwc3_port_info port_info;	
	void __iomem        *base;
       int 				ic_type;
};
#endif
enum{
   IC_ATM7059A
};
static int g_ic_type = IC_ATM7059A;
//static DECLARE_BITMAP(dwc3_devs, DWC3_DEVS_POSSIBLE);
#if 0
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
#endif
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
static void dwc3_core_soft_reset(struct dwc3 *dwc)
{
	u32		reg;

	/* Before Resetting PHY, put Core in Reset */
	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg |= DWC3_GCTL_CORESOFTRESET;
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);

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

	evt->buf	= dma_alloc_coherent(dwc->dev, length,
			&evt->dma, GFP_KERNEL);
	printf("\n------dwc3_alloc_one_event_buffer---%p--%x-------\n",evt->buf,(u32)evt->dma);

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
static int dwc3_event_buffers_setup(struct dwc3 *dwc)
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
	void __iomem *usb3_usb_vcon;
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

void dwc3_phy_init(u8 mode, unsigned char port_num)
{
	unsigned char val_u8;
    
       if((port_num!= 0)&&(port_num!= 1)){
		port_num = 0;
        }        
	if(g_ic_type == IC_ATM7059A){

		if(mode == DWC3_MODE_DEVICE) {
			printk(" GS705A phy init for dwc3 gadget %s\n", __TIME__ );
			val_u8 =(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0);//select page1
			setphy(0xf4, val_u8, port_num);
			val_u8 =(1<<5) |(1<<4)|(0<<3)|(1<<2)|(1<<0);//negative sample
			setphy(0xe0, val_u8, port_num);
			val_u8 =(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0);//select page1
			setphy(0xf4, val_u8, port_num);
			val_u8 =(3<<5) |(0<<4)|(1<<3)|(1<<2)|(3<<0);//slewRate
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
			val_u8 =(3<<5) |(0<<4)|(1<<3)|(1<<2)|(3<<0);//slewRate
			setphy(0xe1, val_u8, port_num);
			val_u8 =(1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0);//bit[6:5]=select page0
			setphy(0xf4, val_u8, port_num);
			val_u8 =(1<<7) |(4<<4)|(1<<3)|(0<<2)|(3<<0); //bit[3]= Enablepowerdown mode
			setphy(0xe6, val_u8, port_num);
			val_u8 = (1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0);
			setphy(0xf4, val_u8, port_num);
			val_u8 = (7<<4)|(0<<1)|(1<<0);                 //sensitivity lower
			setphy(0xe7, val_u8, port_num);
			val_u8 =(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0);//select page1
			setphy(0xf4, val_u8, port_num);
			val_u8 =(1<<5) |(1<<4)|(0<<3)|(0<<2)|(1<<0);// REG_CAL=0
			setphy(0xe0, val_u8, port_num);
			val_u8 = (1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0); // elect page0
			setphy(0xf4, val_u8, port_num);
			val_u8 = (0xc<<4)|(4<<0);                                  //adjust hshd threshold and hstx current
			setphy(0xe4, val_u8, port_num);
			val_u8 = (1<<7) |(1<<6) |(1<<5)|(1<<4)|(1<<3)|(1<<2)|(0<<1)|(0<<0);
			setphy(0xf0, val_u8, port_num);   //disconnect enable
		}
	}
	return;
}
EXPORT_SYMBOL_GPL(dwc3_phy_init);

/**
 * dwc3_core_init - Low-level initialization of DWC3 Core
 * @dwc: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
static int dwc3_core_init(struct dwc3 *dwc)
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
	timeout = 500;
	dwc3_writel(dwc->regs, DWC3_DCTL, DWC3_DCTL_CSFTRST);
	do {
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		if (!(reg & DWC3_DCTL_CSFTRST))
			break;
		mdelay(1);
            	if((timeout--) == 0){
			dev_err(dwc->dev, "Reset Timed Out\n");
			ret = -ETIMEDOUT;
			goto err0;
		}
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

		dwc3_core_soft_reset(dwc);

		dwc3_cache_hwparams(dwc);
            //====force to high speed====
		reg = dwc3_readl(dwc->regs, DWC3_DCFG);
		reg &= ~(DWC3_DCFG_SPEED_MASK);
		reg |= DWC3_DCFG_HIGHSPEED;
		dwc3_writel(dwc->regs, DWC3_DCFG, reg);
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
#if 0    
  	 /*************disable usb irq****************************/
     	reg = dwc3_readl(dwc->regs, 0xcd50);
     	reg &=~(1<<12);
	dwc3_writel(dwc->regs, 0xcd50, reg);
  
    	irq_free_handler(USB_IRQ_ID);
#endif          
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

void owl_usb3_open_powergate(void)
{
	u32 tmp, i;

	printf("OWL USB3: power on\n");

	/* 
	* 1. assert reset
	*/  
	tmp = readl(CMU_DEVRST1);
	tmp &= ~(USB3_MOD_RST);       /* USB3 */
	writel(tmp, CMU_DEVRST1);

	/* 
	* 2. power on
	*/
	tmp = readl(SPS_PG_CTL);
	tmp |= (0x1 << 10);      /* USB3 */
	writel(tmp, SPS_PG_CTL);

	/* wait power on */
	i = 1000;
	while (i-- && (readl(SPS_PG_CTL) & (0x1 << 14)) == 0) {
		/* bit 13 is DS ACK */
		mdelay(1);
	}
	printf("OWL USB3: wait res %d, %x\n", i, readl(SPS_PG_CTL));


	/* 
	* 3. clk enable :USB3 PLL enable 
	*/
	tmp = readl(CMU_USBPLL);
	tmp |= (0x1f);
	writel(tmp, CMU_USBPLL);
	udelay(1000);

	/* 
	* 4. deassert reset
	*/
	tmp = readl(CMU_DEVRST1);
	tmp |= ( USB3_MOD_RST);      /* USB3 */
	writel(tmp, CMU_DEVRST1);
}
static int dwc3_probe(void)
{
	int			ret = -ENOMEM;
	u8			mode;
    
	owl_usb3_open_powergate();
    
 	act_dwc3.regs	= (void __iomem		*)DWC3_BASE;
    	act_dwc3.maximum_speed = DWC3_DCFG_HIGHSPEED;
	dwc3_cache_hwparams(&act_dwc3);

	ret = dwc3_alloc_event_buffers(&act_dwc3, DWC3_EVENT_BUFFERS_SIZE);
	if (ret) {
		printk( "failed to allocate event buffers\n");
		ret = -ENOMEM;
		goto err0;
	}
	ret = dwc3_core_init(&act_dwc3);
	if (ret) {
		dev_err(dev, "failed to initialize core\n");
		goto err0;
	}
	ret = dwc3_event_buffers_setup(&act_dwc3);
	if (ret) {
		dev_err(dwc->dev, "failed to setup event buffers\n");
		goto err1;
	}
	mode = DWC3_MODE_DEVICE;
	if(g_ic_type == IC_ATM7059A){
		dwc3_phy_init(mode, 0);
		dwc3_phy_init(mode, 1);
	}
	dwc3_set_mode(&act_dwc3, DWC3_GCTL_PRTCAP_DEVICE);
	ret = dwc3_gadget_init(&act_dwc3);
	if (ret) {
			dev_err(dev, "failed to initialize gadget\n");
		goto err2;
	}
	act_dwc3.mode = mode;
	return 0;
err2:
	dwc3_event_buffers_cleanup(&act_dwc3);

err1:
	dwc3_core_exit(&act_dwc3);

err0:
	dwc3_free_event_buffers(&act_dwc3);

	return ret;
}

	
static int dwc3_remove(void)
{
 	int i;
    
	dwc3_gadget_exit(&act_dwc3);
	dwc3_event_buffers_cleanup(&act_dwc3);
	dwc3_free_event_buffers(&act_dwc3);
	dwc3_core_exit(&act_dwc3);

	//add for devm_kzalloc ,to free memory
    	for (i = 0; i < act_dwc3.num_event_buffers ; i++){
		if(act_dwc3.ev_buffs[i] !=NULL)
            		free(act_dwc3.ev_buffs[i]);
       }
	free(act_dwc3.ev_buffs);
    
	return 0;
}
#if 0
#ifdef CONFIG_PM_SLEEP
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

static const struct dev_pm_ops dwc3_dev_pm_ops = {
	.prepare	= dwc3_prepare,
	.complete	= dwc3_complete,

	//SET_SYSTEM_SLEEP_PM_OPS(dwc3_suspend, dwc3_resume)
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

#endif


////////////////////////////////////////////////////////////////////////////////
void dwc3_clk_init(void)
{
	u32		reg;
     
	/*USB3 PLL enable*/    
	reg = readl(CMU_USBPLL);
       if(g_ic_type == IC_ATM7059A){
	    reg |= (0x7f);
       }
	writel(reg, CMU_USBPLL);

	udelay(1000);

	/*USB3 Cmu Reset */
	reg = readl(CMU_DEVRST1);
	reg &= ~(USB3_MOD_RST);
	writel(reg, CMU_DEVRST1);

	udelay(100);
	
	reg = readl(CMU_DEVRST1);
	reg |= (USB3_MOD_RST);
	writel(reg, CMU_DEVRST1);

	udelay(100);

 	if(g_ic_type == IC_ATM7059A){
		writel(0x6046, DWC3_BASE+ANA02 );
		writel(0x2010,DWC3_BASE+ANA0E );
		writel(0x8000,DWC3_BASE+ANA0F );
		writel(0x0, DWC3_BASE+ REV1 );
		writel(0x0013,DWC3_BASE+PAGE1_REG02 );
		writel(0x0004,DWC3_BASE+PAGE1_REG06 );
		writel(0x22ed,DWC3_BASE+PAGE1_REG07 );
		writel(0xf802, DWC3_BASE+PAGE1_REG08 );
		writel(0x3080,DWC3_BASE+PAGE1_REG09 );
		writel(0x2030, DWC3_BASE+PAGE1_REG0B );
		writel((1<<14), DWC3_BASE+ANA0F );        
		reg = readl(DWC3_BASE + DWC3_CMU_DEBUG_LDO);
		reg |= CMU_BIAS_EN;
		writel(reg, DWC3_BASE+ DWC3_CMU_DEBUG_LDO);
		/*USB2 LDO enable*/
		reg = readl(USB3_P0_CTL);
		reg |= (1 << USB3_P0_CTL_PLLLDOEN_IC1 )|(2 << USB3_P0_CTL_LDOVREFSEL_SHIFT_IC1);
		writel(reg, USB3_P0_CTL );
	}
	udelay(100);

//	reg = readl(USB3_P0_CTL);
//	reg &=  ~((0x1 << USB3_P0_CTL_DPPUEN_P0)|(0x1 << USB3_P0_CTL_DMPUEN_P0)); 
//	writel(reg,USB3_P0_CTL );
    
       udelay(1000);
	return;
}

void dwc3_clk_exit(void)
{
	u32		reg;
	
	/*USB3 PLL disable*/
	reg = readl(CMU_USBPLL);
	
       if(g_ic_type == IC_ATM7059A){
	    reg &= ~(0x7f);
       }
	writel(reg, CMU_USBPLL);
}
int usb_gadget_handle_interrupts(void)
{
	u32 value=0;
#if 0    
    	static int cnt =0 ;
	static int flag =0 ;
        cnt++;
        if(cnt%100 ==0)
		printf("\n-----%d-----%x-----%d---DCFG=0x%x--DEVTEN=0x%x---DCTL=0x%x--DSTS=0x%x--EXT_CTL=0x%x----n",\
		flag,readl(USB3_P0_CTL),dwc3_readl(act_dwc3.regs, DWC3_GEVNTCOUNT(value)),\
		dwc3_readl(act_dwc3.regs, DWC3_DCFG),dwc3_readl(act_dwc3.regs, DWC3_DEVTEN),\
		dwc3_readl(act_dwc3.regs, DWC3_DCTL),dwc3_readl(act_dwc3.regs, DWC3_DSTS),dwc3_readl(act_dwc3.regs, 0xcd50));
#endif        
    	value=dwc3_interrupt(0, &act_dwc3);
	if(value==IRQ_WAKE_THREAD){
    		value=dwc3_thread_interrupt(0, &act_dwc3);
        }

	return value;
}


int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	int ret;

	if (!driver)
		return -EINVAL;
	if (!driver->bind || !driver->setup || !driver->disconnect)
		return -EINVAL;
	if (driver->speed != USB_SPEED_FULL && driver->speed != USB_SPEED_HIGH)
		return -EINVAL;

	dwc3_clk_init();

	dwc3_probe();
	memcpy(&act_dwc3.eps[0]->endpoint, &dwc3_ep_init[0], sizeof(*dwc3_ep_init));
	memcpy(&act_dwc3.eps[1]->endpoint, &dwc3_ep_init[1], sizeof(*dwc3_ep_init));
	ret = driver->bind(&act_dwc3.gadget);
	if (ret) {
		printk("driver->bind() returned %d\n", ret);
		return ret;
	}

	//act_dwc3.gadget_driver = driver;
	dwc3_gadget_start(&act_dwc3.gadget,driver);
	 
	return 0;
}
extern  int dwc3_gadget_suspend(struct dwc3 *dwc);

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
       dwc3_gadget_run_stop(&act_dwc3,0);
       writel(DWC3_DEVTEN, 0x00);   
       act_dwc3.gadget.speed = USB_SPEED_UNKNOWN;
	act_dwc3.gadget_driver->disconnect(&act_dwc3.gadget);
       dwc3_gadget_stop(&act_dwc3.gadget,driver);
       act_dwc3.gadget.speed = USB_SPEED_UNKNOWN;
	dwc3_gadget_suspend(&act_dwc3);
	driver->unbind(&act_dwc3.gadget);	
    
	dwc3_remove();
	dwc3_clk_exit();
	act_dwc3.gadget_driver = NULL;
	return 0;
}

#include <usb.h>
int board_usb_init(int index, enum usb_init_type init)
{
	return 0;
}

