/*
 * (C) Copyright www.actions-semi.com 2012-2014
 *     Written by houjingkun. <houjingkun@actions-semi.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kthread.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <mach/bootdev.h>

#include <asm/irq.h>
#include <asm/system.h>
#include <linux/regulator/consumer.h>
//#include <asm/mach-actions/aotg_otg.h>

#include <mach/hardware.h>
#include <linux/clk.h>
//#include <mach/clock.h>
#include <linux/wakelock.h>
#include <linux/suspend.h>
//#include <mach/atc260x.h>
#include <mach/debug.h>
#include <asm/prom.h>
#include <mach/gpio.h>
#include <linux/kallsyms.h>
#include <mach/powergate.h>
#include <mach/module-owl.h>

#include "aotg_hcd.h"
#include "aotg_plat_data.h"
#include "aotg_hcd_debug.h"
#include "aotg_udc_debug.h"
#include "aotg_mon.h"
#include "aotg_udc.h"

static int aotg0_slew_rate = -1;
module_param(aotg0_slew_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(aotg0_slew_rate, "aotg0_slew_rate");
static int aotg0_tx_bias = -1;
module_param(aotg0_tx_bias, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(aotg0_tx_bias, "aotg0_tx_bias");

static int aotg1_slew_rate = -1;
module_param(aotg1_slew_rate, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(aotg1_slew_rate, "aotg1_slew_rate");
static int aotg1_tx_bias = -1;
module_param(aotg1_tx_bias, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(aotg1_tx_bias, "aotg1_tx_bias");

struct aotg_udc *acts_udc_controller;
unsigned int port_device_enable[2];
int vbus_otg_en_gpio[2][2];
enum aotg_mode_e aotg_mode[2];
static struct platform_device *aotg_dev[2][2];
static int aotg_initialized[2][2];
int is_ls_device[2]; /*if detect low speed device plug in,must disable usbh high speed*/
struct mutex aotg_onoff_mutex;
struct aotg_hcd *act_hcd_ptr[2];
int aotg_udc_enable[2];

static u64 hcd_dmamask = DMA_BIT_MASK(32);
static struct aotg_plat_data aotg_data0 = {
	.usbecs = (void __iomem *)IO_ADDRESS(USBH0_ECS),
	.usbpll = (void __iomem *)IO_ADDRESS(CMU_USBPLL),
	.usbpll_bits = CMU_USBPLL_USBPLL0EN,
	.devrst = (void __iomem *)IO_ADDRESS(CMU_DEVRST1),
	.devrst_bits = CMU_DEVRST1_USBH0,
	.no_hs = 0,
};

static struct aotg_plat_data aotg_data1 = {
	.usbecs = (void __iomem *)IO_ADDRESS(USBH1_ECS),
	.usbpll = (void __iomem *)IO_ADDRESS(CMU_USBPLL),
	.usbpll_bits = CMU_USBPLL_USBPLL1EN,
	.devrst = (void __iomem *)IO_ADDRESS(CMU_DEVRST1),
	.devrst_bits = CMU_DEVRST1_USB1,
	.no_hs = 0,
};

static void aotg_DD_set_phy(void __iomem *base, u8 reg, u8 value)
{
	u8 addrlow, addrhigh;
	int time = 1;

	addrlow = reg & 0x0f;
	addrhigh = (reg >> 4) & 0x0f;

	/*write vstatus: */
	writeb(value, base + VDSTATUS);
	mb();

	/*write vcontrol: */
	writeb(addrlow | 0x10, base + VDCTRL);
	udelay(time); //the vload period should > 33.3ns
	writeb(addrlow & 0x0f, base + VDCTRL);
	udelay(time);
	mb();
	writeb(addrlow | 0x10, base + VDCTRL);
	udelay(time);
	writeb(addrhigh | 0x10, base + VDCTRL);
	udelay(time);
	writeb(addrhigh & 0x0f, base + VDCTRL);
	udelay(time);
	writeb(addrhigh | 0x10, base + VDCTRL);
	udelay(time);
	return;
}

static void aotg_set_udc_phy(int id)
{
	int slew_rate;
	void __iomem *base = acts_udc_controller->base;
	if (id)
		slew_rate = aotg1_slew_rate;
	else
		slew_rate = aotg0_slew_rate;

	aotg_DD_set_phy(base, 0xf4,(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0));
	aotg_DD_set_phy(base, 0xe0,(1<<5) |(1<<4)|(0<<3)|(1<<2)|(1<<0));
	
	aotg_DD_set_phy(base, 0xf4,(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0));
	aotg_DD_set_phy(base, 0xe1,(slew_rate<<5) |(0<<4)|(1<<3)|(1<<2)|(3<<0));
	
	aotg_DD_set_phy(base, 0xf4,(1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0));
	aotg_DD_set_phy(base, 0xe6,(1<<7) |(4<<4)|(1<<3)|(0<<2)|(3<<0));
	
	aotg_DD_set_phy(base, 0xf4,(1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0));
	aotg_DD_set_phy(base, 0xe7,(7<<4)|(0<<1)|(1<<0));
	
	aotg_DD_set_phy(base, 0xf4,(1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0));
	aotg_DD_set_phy(base, 0xe4,(9<<4)|(7<<0));
}

static void aotg_set_hcd_phy(int id)
{
	int slew_rate,tx_bias;
	void __iomem *base = act_hcd_ptr[id]->base;
	if (id) {
		slew_rate = aotg1_slew_rate;
		tx_bias = aotg1_tx_bias;
	} else {
		slew_rate = aotg0_slew_rate;
		tx_bias = aotg0_tx_bias;
	}

	aotg_DD_set_phy(base, 0xf4,(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0));
	aotg_DD_set_phy(base, 0xe0,(1<<5) |(1<<4)|(0<<3)|(1<<2)|(1<<0));

	aotg_DD_set_phy(base, 0xf4,(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0));
	if((slew_rate >=0) && (slew_rate <= 7))
		aotg_DD_set_phy(base, 0xe1,(slew_rate<<5)|(0<<4)|(1<<3)|(1<<2)|(3<<0));
	else
		aotg_DD_set_phy(base, 0xe1,(3<<5) |(0<<4)|(1<<3)|(1<<2)|(3<<0));

	aotg_DD_set_phy(base, 0xf4,(1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0));
	aotg_DD_set_phy(base, 0xe6,(1<<7) |(4<<4)|(1<<3)|(0<<2)|(3<<0));

	aotg_DD_set_phy(base, 0xf4,(1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0));
	//aotg_DD_set_phy(base, 0xe7,(7<<4)|(0<<1)|(1<<0));
	aotg_DD_set_phy(base, 0xe7,(9<<4)|(0<<1)|(1<<0));

	aotg_DD_set_phy(base, 0xf4,(1<<7) |(1<<5)|(1<<4)|(2<<2)|(3<<0));
	aotg_DD_set_phy(base, 0xe0,(1<<5) |(1<<4)|(0<<3)|(0<<2)|(1<<0));
	
	
	aotg_DD_set_phy(base, 0xf4,(1<<7) |(0<<5)|(1<<4)|(2<<2)|(3<<0));
	if((tx_bias >=0) && (tx_bias <= 15))
		aotg_DD_set_phy(base, 0xe4, (0xc<<4)|(tx_bias<<0));
	else
		aotg_DD_set_phy(base, 0xe4, (0xc<<4)|(4<<0));
	
	aotg_DD_set_phy(base, 0xf0,(1<<7) |(1<<6) |(1<<5)|(1<<4)|(1<<3)|(1<<2)|(0<<1)|(0<<0));
	return;
}

void aotg_clk_enable(int id, int is_enable)
{
	struct aotg_plat_data *data;
	if (id)
		data = &aotg_data1;
	else
		data = &aotg_data0;

	if (is_enable) {
		usb_setbitsl(data->usbpll_bits, data->usbpll);
	} else {
		usb_clearbitsl(data->usbpll_bits, data->usbpll);
	}

	return;
}

static int aotg_controller_reset(int id)
{
	struct aotg_plat_data *data;
	void __iomem *usb_reset;
	int i = 0;

	if (id)
		data = &aotg_data1;
	else
		data = &aotg_data0;
	usb_clearbitsl(data->devrst_bits, data->devrst);
	udelay(1);
	usb_setbitsl(data->devrst_bits, data->devrst);

	if (aotg_mode[id] == HCD_MODE)
		usb_reset = act_hcd_ptr[id]->base + USBERESET;
	else
		usb_reset = acts_udc_controller->base + USBERESET;
	while (((readb(usb_reset) & USBERES_USBRESET) != 0) && (i < 300000)) {
		i++;
		udelay(1);
	}

	if (!(readb(usb_reset) & USBERES_USBRESET)) {
		pr_info("usb reset OK: %x.\n", readb(usb_reset));
	} else {
		pr_err("usb reset ERROR: %x.\n", readb(usb_reset));
		return -EBUSY;
	}
	return 0;
}

void aotg_hardware_init(int id)
{
	u8 val8;
	unsigned long flags;
	struct aotg_plat_data *data;
	void __iomem *base;
	if (id)
		data = &aotg_data1;
	else
		data = &aotg_data0;

	if (aotg_mode[id] == HCD_MODE)
		base = act_hcd_ptr[id]->base;
	else
		base = acts_udc_controller->base;

	owl_powergate_power_on(id ? OWL_POWERGATE_USB2_1 : OWL_POWERGATE_USB2_0);
	//module_clk_enable(MOD_ID_USB2);
	module_clk_enable(id ? MOD_ID_USB2_1 : MOD_ID_USB2_0);
	aotg_clk_enable(id, 1);

	local_irq_save(flags);
	aotg_controller_reset(id);
	/* fpga : new DMA mode */
	writel(0x1, base + HCDMABCKDOOR);

	if (aotg_mode[id] == HCD_MODE) {
		writel((0x37 << 24) | (0x10 << 13) | (0xb << 4), data->usbecs);
		udelay(100);
		aotg_set_hcd_phy(id);

		/***** TA_BCON_COUNT *****/
		writeb(0x0, base + TA_BCON_COUNT);     		//110ms

		/*set TA_AIDL_BDIS timeout never generate */
		writeb(0xff, base + TAAIDLBDIS);
		/*set TA_WAIT_BCON timeout never generate */
		writeb(0xff, base + TAWAITBCON);
		/*set TB_VBUS_DISCHARGE_PLS timeout value = 40.68ms*/
		writeb(0x28, base + TBVBUSDISPLS);
		/*set TA_WAIT_BCON timeout never generate*/
		usb_setbitsb(1 << 7, base + TAWAITBCON);

		writew(0x1000, base + VBUSDBCTIMERL);
//	writeb(0x40, base + USBCS);

		val8 = readb(base + BKDOOR);
		if (data && data->no_hs) {
			val8 |= (1 << 7);
		} else {
			val8 &= ~(1 << 7);
		}
		if (is_ls_device[id])
			val8 |= (1<<7);
		writeb(val8, base + BKDOOR);
	} else {
		if (acts_udc_controller->inited == 0) { /*in poweron,don't enter device mode*/
			writel((0x37 << 24) | (0x10 << 13) | (0xb << 4), data->usbecs);
			acts_udc_controller->inited = 1;
		} else {
			writel((0x3f << 24) | (0x10 << 13) | (0xb << 4) | (0x0f), data->usbecs);
		}
		udelay(100);
		aotg_set_udc_phy(id);
		usb_setbitsb(1 << 4, base + BKDOOR);  /*clk40m */

		writeb(0xff,base + USBIRQ);
		writeb(0xff,base + OTGIRQ);
		writeb(readb(base + USBEIRQ), base + USBEIRQ);
		writeb(0xff,base + OTGIEN);
		writeb(0x0D,base + USBEIRQ);
	}
	mb();
	local_irq_restore(flags);

	return;
}

struct of_device_id aotg_of_match[] = {
	{.compatible = "actions,owl-usb-2.0-0",.data = &aotg_data0},
	{.compatible = "actions,owl-usb-2.0-1",.data = &aotg_data1},
	{},
};
MODULE_DEVICE_TABLE(of, aotg_of_match);

void aotg_get_dts(void)
{
	struct device_node *of_node;
	enum of_gpio_flags flags;
	
	of_node = of_find_compatible_node(NULL, NULL, "actions,owl-usb-2.0-0");
	if (of_node) {
		if (!of_find_property(of_node, "port0_host_plug_detect", NULL)) {
			pr_info("can't find port0_host_plug_detect config\n");
			port_host_plug_detect[0] = 0;
		}	else {
			port_host_plug_detect[0] = be32_to_cpup((const __be32 *)of_get_property(of_node,  "port0_host_plug_detect",NULL));
		}
		pr_info("port_host_plug_detect[0]:%d\n", port_host_plug_detect[0]);
		
		if (!of_find_property(of_node, "vbus_otg_en_gpio", NULL)) {
			pr_debug("can't find vbus_otg0_en_gpio config\n");
			vbus_otg_en_gpio[0][0] = -1;
		}	else {
			vbus_otg_en_gpio[0][0] = of_get_named_gpio_flags(of_node,  "vbus_otg_en_gpio",0, &flags);
			vbus_otg_en_gpio[0][1] = flags & 0x01;
			if (gpio_request(vbus_otg_en_gpio[0][0], aotg_of_match[0].compatible))
				pr_debug("fail to request vbus gpio [%d]\n", vbus_otg_en_gpio[0][0]);
			if (port_host_plug_detect[0] != 2)
				gpio_direction_output(vbus_otg_en_gpio[0][0], !!port_host_plug_detect[0]);
		}
		pr_info("port0_vubs_en:%d\n",vbus_otg_en_gpio[0][0]);
	}
	else {
		pr_debug("can't find usbh0 dts node\n");
	}
	
	of_node = of_find_compatible_node(NULL, NULL, "actions,owl-usb-2.0-1");
	if (of_node) {
		if (!of_find_property(of_node, "port1_host_plug_detect", NULL)) {
			pr_info("can't find port1_host_plug_detect config\n");
			port_host_plug_detect[1] = 0;
		}	else {
			port_host_plug_detect[1] = be32_to_cpup((const __be32 *)of_get_property(of_node,  "port1_host_plug_detect",NULL));
		}
		pr_info("port_host_plug_detect[1]:%d\n", port_host_plug_detect[1]);
		
		if (!of_find_property(of_node, "vbus_otg_en_gpio", NULL)) {
			printk("can't find vbus_otg1_en_gpio config\n");
			vbus_otg_en_gpio[1][0] = -1;
		}	else {
			vbus_otg_en_gpio[1][0] = of_get_named_gpio_flags(of_node,  "vbus_otg_en_gpio",0, &flags);
			vbus_otg_en_gpio[1][1] = flags & 0x01;
			if (gpio_request(vbus_otg_en_gpio[1][0], aotg_of_match[1].compatible))
				pr_debug("fail to request vbus gpio [%d]\n", vbus_otg_en_gpio[1][0]);
			if (port_host_plug_detect[1] != 2)
				gpio_direction_output(vbus_otg_en_gpio[1][0], !!port_host_plug_detect[1]);
		}
		pr_info("port1_vubs_en:%d\n",vbus_otg_en_gpio[1][0]);
	}
	else {
		pr_debug("can't find usbh1 dts node\n");
	}
}

int aotg_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct resource *res_mem;
	void __iomem		*regs;
	struct aotg_hcd *acthcd;
	struct aotg_udc *udc;
	int irq;
	int retval;

	if (aotg_mode[pdev->id] == HCD_MODE)
		aotg_power_onoff(pdev->id,1);

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_mem) {
		dev_err(&pdev->dev, "<HCD_PROBE>usb has no resource for mem!\n");
		retval = -ENODEV;
		goto err0;
	}
	pr_info("res_mem->start--end = 0x%x--0x%x\n",res_mem->start,res_mem->end);

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(&pdev->dev, "<AOTG_PROBE>usb has no resource for irq!\n");
		retval = -ENODEV;
		goto err0;
	}

	if (!request_mem_region(res_mem->start, res_mem->end - res_mem->start + 1, dev_name(&pdev->dev))) {
		dev_err(&pdev->dev, "<AOTG_PROBE>request_mem_region failed\n");
		retval = -EBUSY;
		goto err0;
	}

	regs = ioremap(res_mem->start, res_mem->end - res_mem->start + 1);
	if (!regs) {
		dev_err(&pdev->dev, "<AOTG_PROBE>ioremap failed\n");
		retval = -ENOMEM;
		goto err1;
	}

	if (aotg_mode[pdev->id] == HCD_MODE) {
		hcd = usb_create_hcd(&act_hc_driver, &pdev->dev, dev_name(&pdev->dev));
		if (!hcd) {
			dev_err(&pdev->dev, "<HCD_PROBE>usb create hcd failed\n");
			retval = -ENOMEM;
			goto err2;
		}

		hcd->regs = regs;
		hcd->rsrc_start = res_mem->start;
		hcd->rsrc_len = res_mem->end - res_mem->start + 1;
		retval = aotg_hcd_init(hcd, pdev);
		if (retval) {
			dev_err(&pdev->dev, "<HCD_PROBE>hcd init failed\n");
			usb_put_hcd(hcd);
			goto err2;
		}

		acthcd = hcd_to_aotg(hcd);
		act_hcd_ptr[pdev->id] = acthcd;
		acthcd->dev = &pdev->dev;
		acthcd->base = hcd->regs;
		acthcd->hcd_exiting = 0;
		acthcd->uhc_irq = irq;
		acthcd->id = pdev->id;
		dev_info(&pdev->dev, "pdev->id probe:%d\n", pdev->id);
		aotg_hardware_init(pdev->id);

#if 0    
#ifdef	CONFIG_PM
		aotg_hcd_register_earlysuspend(acthcd);
#endif
#endif

		hcd->self.sg_tablesize = 32;

		hcd->has_tt = 1;
		hcd->self.uses_pio_for_control = 1;	/* for ep0, using CPU mode only. */

		init_timer(&acthcd->hotplug_timer);
		acthcd->hotplug_timer.function = aotg_hub_hotplug_timer;
		acthcd->hotplug_timer.data = (unsigned long)acthcd;

		init_timer(&acthcd->trans_wait_timer);
		acthcd->trans_wait_timer.function = aotg_hub_trans_wait_timer;
		acthcd->trans_wait_timer.data = (unsigned long)acthcd;
		init_timer(&acthcd->check_trb_timer);
		acthcd->check_trb_timer.function = aotg_check_trb_timer;
		acthcd->check_trb_timer.data = (unsigned long)acthcd;

		retval = usb_add_hcd(hcd, irq, 0);
		if (likely(retval == 0)) {
			aotg_enable_irq(acthcd);
			create_debug_file(acthcd);
			dev_info(&pdev->dev, "hcd controller initialized. OTGIRQ: 0x%02X, OTGSTATE: 0x%02X \n", 
				readb(acthcd->base + OTGIRQ),
				readb(acthcd->base + OTGSTATE));

			writeb(readb(acthcd->base + USBEIRQ), acthcd->base + USBEIRQ);
			printk("USBEIRQ(0x%p): 0x%02X\n", acthcd->base + USBEIRQ, readb(acthcd->base + USBEIRQ));
			return 0;
		} else {
			dev_err(acthcd->dev, "<HCD_PROBE>usb add hcd failed\n");
		}

		del_timer_sync(&acthcd->hotplug_timer);
		del_timer_sync(&acthcd->trans_wait_timer);
		del_timer_sync(&acthcd->check_trb_timer);
	} else {
		//aotg_power_onoff(pdev->id, 0);
		udc = &memory;
		udc->port_specific = pdev->dev.platform_data;
		acts_udc_controller = udc;
		udc->irq = irq;
		udc->dev =  &pdev->dev;
		udc->base = regs;
		udc->rsrc_start = res_mem->start;
		udc->rsrc_len = res_mem->end - res_mem->start + 1;
		udc->id = pdev->id;
		platform_set_drvdata(pdev, udc);
		udc_reinit(udc);

		aotg_hardware_init(pdev->id);
		printk("udc->irq:%d\n", udc->irq);
		retval = request_irq(udc->irq, aotg_udc_irq, 0, udc_driver_name, udc);
		if (retval != 0) {
			dev_err(udc->dev, "%s: can't get irq %i, err %d\n", udc_driver_name, udc->irq, retval);
			goto err2;
		}

		retval = usb_add_gadget_udc(&pdev->dev, &udc->gadget);
		if (retval) {
			free_irq(udc->irq, udc);
			goto err2;
		}
		return 0;
	}

err2:
	aotg_clk_enable(pdev->id, 0);
	owl_powergate_power_off(pdev->id ? OWL_POWERGATE_USB2_1 : OWL_POWERGATE_USB2_0);
	iounmap(regs);
err1:
	release_mem_region(res_mem->start, res_mem->end - res_mem->start + 1);
err0:
	dev_err(&pdev->dev, "%s: usb probe hcd  failed, error is %d", __func__, retval);

	return retval;
}
static inline int aotg_device_calc_id(int dev_id)
{
	int id;

	if (hcd_ports_en_ctrl == 1) {
		id = 0;
	} else if (hcd_ports_en_ctrl == 2) {
		id = 1;
	} else if (hcd_ports_en_ctrl == 3) {
		if (dev_id) {
			id = 0;
		} else {
			id = 1;
		}
	} else {
		id = dev_id;
	}
	return id;
}

int aotg_device_init(int dev_id, enum aotg_mode_e mode)
{
	struct device_node *of_node;
	struct resource res[2];
	int ret = 0;

	mutex_lock(&aotg_onoff_mutex);
	if (aotg_initialized[dev_id][mode]) {
		aotg_initialized[dev_id][mode]++;
		printk("aotg%d initialized already! cnt:%d\n", dev_id, aotg_initialized[dev_id][mode]);
		mutex_unlock(&aotg_onoff_mutex);
		return 0;
	}
	aotg_initialized[dev_id][mode] = 1;
	aotg_mode[dev_id] = mode;
	of_node = of_find_compatible_node(NULL, NULL, aotg_of_match[dev_id].compatible);
	if (NULL == of_node) {
		pr_err("can't find usbh%d dts node\n",dev_id);
		ret = -1;
		goto err1;
	}

	memset(&res, 0, sizeof(res));
	if (of_address_to_resource(of_node, 0, &res[0])) {
		pr_err("can't fetch mem info from dts\n");
		ret = -2;
		goto err1;
	}

	if (of_irq_to_resource(of_node, 0, &res[1]) == NO_IRQ) {
		pr_err("can't fetch IRQ info from dts\n");
		ret = -3;
		goto err1;
	}

	if (mode == HCD_MODE)
		aotg_dev[dev_id][mode] = platform_device_alloc("aotg_hcd", dev_id);
	else
		aotg_dev[dev_id][mode] = platform_device_alloc("aotg_udc", dev_id);
	if (!aotg_dev[dev_id][mode]) {
		pr_err("platform_device_alloc fail\n");
		ret = -ENOMEM;
		goto err1;
	}
	aotg_dev[dev_id][mode]->dev.dma_mask = &hcd_dmamask;
	aotg_dev[dev_id][mode]->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	ret = platform_device_add_resources(aotg_dev[dev_id][mode], res, ARRAY_SIZE(res));
	if (ret) {
		ERR("platform_device_add_resources fail\n");
		goto err;
	}

	ret = platform_device_add_data(aotg_dev[dev_id][mode], aotg_of_match[dev_id].data, sizeof(struct aotg_plat_data));
	if (ret) {
		ERR("platform_device_add_data fail\n");
		goto err;
	}

	ret = platform_device_add(aotg_dev[dev_id][mode]);
	if (ret) {
		ERR("platform_device_add fail\n");
		goto err;
	}
	mutex_unlock(&aotg_onoff_mutex);
	return 0; 
err:
	platform_device_put(aotg_dev[dev_id][mode]);
	aotg_dev[dev_id][mode] = NULL;
err1:
	mutex_unlock(&aotg_onoff_mutex);
	return ret; 
}

void aotg_device_exit(int dev_id, enum aotg_mode_e mode)
{
	mutex_lock(&aotg_onoff_mutex);
	if (!aotg_initialized[dev_id][mode]) {
		printk("aotg%d exit already!\n",dev_id);
		mutex_unlock(&aotg_onoff_mutex);
		return;
	}

	aotg_initialized[dev_id][mode]--;
	if (aotg_initialized[dev_id][mode] > 0) {
		printk("aotg%d_exit cnt:%d\n", dev_id, aotg_initialized[dev_id][mode]);
		mutex_unlock(&aotg_onoff_mutex);
		return;
	}
	aotg_initialized[dev_id][mode] = 0;
	aotg_mode[dev_id] = mode;

	if (aotg_dev[dev_id][mode] != NULL) {
		platform_device_unregister(aotg_dev[dev_id][mode]);
		aotg_dev[dev_id][mode] = NULL;
	}
	mutex_unlock(&aotg_onoff_mutex);
}

void aotg_udc_init(int id)
{
	struct resource *res_mem;
	void __iomem		*regs;
	struct aotg_udc *udc;
	int irq;
	int retval;
	struct platform_device *pdev;

	pdev = aotg_dev[id][UDC_MODE];
	aotg_power_onoff(id, 0);
	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_mem) {
		dev_err(&pdev->dev, "<HCD_PROBE>usb has no resource for mem!\n");
		retval = -ENODEV;
		goto err0;
	}
	printk("res_mem->start--end = 0x%x--0x%x\n",res_mem->start,res_mem->end);

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(&pdev->dev, "<AOTG_PROBE>usb has no resource for irq!\n");
		retval = -ENODEV;
		goto err0;
	}

	if (!request_mem_region(res_mem->start, res_mem->end - res_mem->start + 1, dev_name(&pdev->dev))) {
		dev_err(&pdev->dev, "<AOTG_PROBE>request_mem_region failed\n");
		retval = -EBUSY;
		goto err0;
	}

	regs = ioremap(res_mem->start, res_mem->end - res_mem->start + 1);
	if (!regs) {
		dev_err(&pdev->dev, "<AOTG_PROBE>ioremap failed\n");
		retval = -ENOMEM;
		goto err1;
	}

	udc = &memory;
	udc->port_specific = pdev->dev.platform_data;
	acts_udc_controller = udc;
	udc->irq = irq;
	udc->dev =  &pdev->dev;
	udc->base = regs;
	udc->id = pdev->id;
	platform_set_drvdata(pdev, udc);
	udc_reinit(udc);

	aotg_mode[id] = UDC_MODE;
	aotg_hardware_init(pdev->id);
	printk("udc->irq:%d\n", udc->irq);
	retval = request_irq(udc->irq, aotg_udc_irq, 0, udc_driver_name, udc);
	if (retval != 0) {
		dev_err(udc->dev, "%s: can't get irq %i, err %d\n", udc_driver_name, udc->irq, retval);
		goto err2;
	}
	return;

err2:
	aotg_clk_enable(id, 0);
	owl_powergate_power_off(id ? OWL_POWERGATE_USB2_1 : OWL_POWERGATE_USB2_0);
	iounmap(regs);
err1:
	release_mem_region(res_mem->start, res_mem->end - res_mem->start + 1);
err0:
	dev_err(&pdev->dev, "%s: usb probe hcd  failed, error is %d", __func__, retval);

	return;
}

void aotg_udc_exit(int id)
{
	struct aotg_udc *udc = acts_udc_controller;
	acts_udc_controller = NULL;
	free_irq(udc->irq, udc);
	aotg_clk_enable(id, 0);
	iounmap(udc->base);
	release_mem_region(udc->rsrc_start, udc->rsrc_len);
	owl_powergate_power_off(id ? OWL_POWERGATE_USB2_1 : OWL_POWERGATE_USB2_0);
}

int aotg_udc_register(int id)
{
	int ret = 0;
	if (aotg_dev[id][HCD_MODE]) {
		pr_err("aotg%d is used in hcd mode now!\n",id);
		return -1;
	}

	if (acts_udc_controller) {
		pr_err("aotg%d has been in udc mode now!\n",id);
		return -1;
	}

	if (aotg_initialized[id][HCD_MODE]) {
		pr_err("aotg%d has been in hcd mode now!\n",id);
		return -1;
	}

	if (aotg_dev[id][UDC_MODE]) {
		if (id) {
			if (aotg_uhost_mon1) {
				wake_lock(&aotg_uhost_mon1->aotg_wake_lock);
				aotg_uhost_mon1->aotg_uhost_det = 0;
				usb_clearbitsl(USB2_PLL_EN1,aotg_uhost_mon1->usbpll);
				owl_powergate_power_off(OWL_POWERGATE_USB2_1);
			}
		} else {
			if (aotg_uhost_mon0) {
				wake_lock(&aotg_uhost_mon0->aotg_wake_lock);
				aotg_uhost_mon0->aotg_uhost_det = 0;
				usb_clearbitsl(USB2_PLL_EN0,aotg_uhost_mon0->usbpll);
				owl_powergate_power_off(OWL_POWERGATE_USB2_0);
			}
		}
		aotg_udc_init(id);
		aotg_udc_endpoint_config(acts_udc_controller);
		pullup(acts_udc_controller, 1);
		usb_setbitsb(USBEIRQ_USBIEN, acts_udc_controller->base + USBEIEN);
	} else {
		pr_err("udc%d device not exist !\n",id);
		ret = -1;
	}
	return ret;
}
EXPORT_SYMBOL(aotg_udc_register);

void aotg_udc_unregister(int id)
{
	if (!acts_udc_controller) {
		pr_err("aotg%d hasn't added udc before now\n",id);
		return;
	}

	aotg_udc_exit(id);
	if (id) {
		if (aotg_uhost_mon1) {
			aotg_power_onoff(id, 1);
			wake_lock_timeout(&aotg_uhost_mon1->aotg_wake_lock, 15*HZ);
			owl_powergate_power_on(OWL_POWERGATE_USB2_1);
			usb_setbitsl(USB2_PLL_EN1,aotg_uhost_mon1->usbpll);
			usb_setbitsl(USB2_PHYCLK_EN1,aotg_uhost_mon1->usbpll);
			usb_setbitsl(USB2_ECS_PLL_LDO_EN,aotg_uhost_mon1->usbecs);
			usb2_set_dp_500k_15k(aotg_uhost_mon1, 0, 1);
			is_ls_device[1]=0;
			aotg_uhost_mon1->aotg_uhost_det = 1;
			mod_timer(&aotg_uhost_mon1->hotplug_timer, jiffies + msecs_to_jiffies(1000));
		}
	} else {
		if (aotg_uhost_mon0) {
			aotg_power_onoff(id, 1);
			wake_lock_timeout(&aotg_uhost_mon0->aotg_wake_lock, 15*HZ);
			owl_powergate_power_on(OWL_POWERGATE_USB2_0);
			usb_setbitsl(USB2_PLL_EN0,aotg_uhost_mon0->usbpll);
			usb_setbitsl(USB2_PHYCLK_EN0,aotg_uhost_mon0->usbpll);
			usb_setbitsl(USB2_ECS_PLL_LDO_EN,aotg_uhost_mon0->usbecs);
			usb2_set_dp_500k_15k(aotg_uhost_mon0, 0, 1);
			is_ls_device[0]=0;
			aotg_uhost_mon0->aotg_uhost_det = 1;
			mod_timer(&aotg_uhost_mon0->hotplug_timer, jiffies + msecs_to_jiffies(1000));
		}
	}
}
EXPORT_SYMBOL(aotg_udc_unregister);

int aotg_hub_register(int dev_id)
{
	int proc_id, ret = -1;
	proc_id = aotg_device_calc_id(dev_id);
	ret = aotg_device_init(proc_id, HCD_MODE);
	return ret;
}
EXPORT_SYMBOL(aotg_hub_register);

void aotg_hub_unregister(int dev_id)
{
	int proc_id;
	proc_id = aotg_device_calc_id(dev_id);
	aotg_device_exit(dev_id, HCD_MODE);
}
EXPORT_SYMBOL(aotg_hub_unregister);

int is_udc_enable(int id)
{
	struct device_node *of_node;

	of_node = of_find_compatible_node(NULL, NULL, aotg_of_match[id].compatible);
	if (NULL == of_node) {
		pr_err("can't find usbh%d dts node\n",id);
		return 0;
	}

	if (!of_find_property(of_node, "aotg_udc_enable", NULL)) {
		pr_info("can't find aotg_udc_enable config\n");
		return 0;
	} else {
		aotg_udc_enable[id] = !!(be32_to_cpup((const __be32 *)of_get_property(of_node,  "aotg_udc_enable",NULL)));
		return aotg_udc_enable[id];
	}
}

void aotg_udc_add(void)
{
	int id;
	if(owl_get_boot_mode() == OWL_BOOT_MODE_UPGRADE)
		return;
	if (is_udc_enable(0)) {
		id = 0;
	} else if (is_udc_enable(1)) {
		id = 1;
	} else {
		pr_info("No aotg_udc being enabled!\n");
		return;
	}
	aotg_device_init(id, UDC_MODE);
}

void aotg_udc_remove(void)
{
	int id;
	if(owl_get_boot_mode() == OWL_BOOT_MODE_UPGRADE)
		return;
	if (aotg_udc_enable[0])
		id = 0;
	else if (aotg_udc_enable[1])
		id = 1;
	else
		return;

	aotg_device_exit(id, UDC_MODE);
}

static struct workqueue_struct *start_mon_wq;
static struct delayed_work start_mon_wker;

static int __init aotg_init(void)
{
	mutex_init(&aotg_onoff_mutex);
	platform_driver_register(&aotg_hcd_driver);
	create_acts_hcd_proc();
	platform_driver_register(&aotg_udc_driver);
	create_acts_udc_proc();
	aotg_get_dts();
	aotg_udc_add();
	start_mon_wq = create_singlethread_workqueue("aotg_start_mon_wq");
	INIT_DELAYED_WORK(&start_mon_wker, aotg_uhost_mon_init);
	queue_delayed_work(start_mon_wq, &start_mon_wker, msecs_to_jiffies(10000));
	return 0;	
}

static void __exit aotg_exit(void)
{
	cancel_delayed_work_sync(&start_mon_wker);
	flush_workqueue(start_mon_wq);
	destroy_workqueue(start_mon_wq);
	aotg_udc_remove();
	remove_acts_hcd_proc();
	platform_driver_unregister(&aotg_hcd_driver);	
	remove_acts_udc_proc();
	platform_driver_unregister(&aotg_udc_driver);
	return;
}

module_init(aotg_init);
module_exit(aotg_exit);

MODULE_DESCRIPTION("Actions OTG controller driver");
MODULE_LICENSE("GPL");
