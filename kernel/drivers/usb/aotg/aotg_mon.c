/*
 * (C) Copyright www.actions-semi.com 2012-2014
 *	   Written by houjingkun. <houjingkun@actions-semi.com>
 * 
 * This	program	is free	software; you can redistribute it and/or modify	it
 * under the terms of the GNU General Public License as	published by the
 * Free	Software Foundation; either	version	2 of the License, or (at your
 * option) any later version.
 *
 * This	program	is distributed in the hope that	it will	be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A	PARTICULAR PURPOSE.	 See the GNU General Public	License
 * for more	details.
 *
 * You should have received	a copy of the GNU General Public License
 * along with this program;	if not,	write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139,	USA.
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
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/otg.h>
#include <linux/usb/hcd.h>

#include <asm/irq.h>
#include <asm/system.h>
#include <linux/regulator/consumer.h>
#include <mach/hardware.h>
#include <linux/clk.h>
#include <linux/wakelock.h>
#include <linux/suspend.h>
#include <mach/debug.h>
#include <asm/prom.h>
#include <mach/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <mach/powergate.h>

#include "aotg_hcd.h"
#include "aotg_regs.h"
#include "aotg_plat_data.h"
#include "aotg_hcd_debug.h"

//int aotg_device_init(int dev_id);
int aotg_hub_register(int dev_id);
void aotg_hub_unregister(int dev_id);
//void aotg_device_exit(int dev_id);
void aotg_power_onoff(int id,int on_off);


/* usbecs register. */
#define	USB2_ECS_VBUS_P0		10
#define	USB2_ECS_ID_P0			12
#define USB2_ECS_LS_P0_SHIFT	8
#define USB2_ECS_LS_P0_MASK		(0x3<<8)
#define USB2_ECS_DPPUEN_P0     3
#define USB2_ECS_DMPUEN_P0     2
//#define USB2_ECS_DMPDDIS_P0    1
//#define USB2_ECS_DPPDDIS_P0    0
#define USB2_ECS_SOFTIDEN_P0   26
#define USB2_ECS_SOFTID_P0     27
#define USB2_ECS_SOFTVBUSEN_P0 24
#define USB2_ECS_SOFTVBUS_P0   25


int port_host_plug_detect[2] = {0};
extern int is_ls_device[2];
extern int vbus_otg_en_gpio[2][2];
extern struct of_device_id aotg_of_match[];
struct aotg_uhost_mon_t * aotg_uhost_mon0 = NULL;
struct aotg_uhost_mon_t * aotg_uhost_mon1 = NULL;

int usb2_set_dp_500k_15k(struct aotg_uhost_mon_t * umon, int enable_500k_up, int enable_15k_down)
{
	unsigned int val;
	
	val = readl(umon->usbecs) & (~((1 << USB2_ECS_DPPUEN_P0) |
			(1 << USB2_ECS_DMPUEN_P0)));
			
	if (enable_500k_up != 0) {
		val |= (1 << USB2_ECS_DPPUEN_P0)|(1 << USB2_ECS_DMPUEN_P0);
	}
	/*if (enable_15k_down == 0) {
		val |= (1 << USB2_ECS_DPPDDIS_P0)|(1 << USB2_ECS_DMPDDIS_P0);
	}*/
	
	writel(val, umon->usbecs);	/* 500k up enable, 15k down disable; */
	return 0;		
}

/* return dp, dm state. */
static inline unsigned int usb_get_linestates(struct aotg_uhost_mon_t * umon)
{
	unsigned int state;

	state = ((readl(umon->usbecs) & USB2_ECS_LS_P0_MASK) >> USB2_ECS_LS_P0_SHIFT);
	return state;
}

static void aotg_uhost_mon_timer(unsigned long data)
{
	static int cnt = 0;
	struct aotg_uhost_mon_t * umon = (struct aotg_uhost_mon_t *)data;

	if ((!umon) || (!umon->aotg_uhost_det)) {
		return;
	}
	umon->state = usb_get_linestates(umon);

	cnt++;
	if ((cnt % 16) == 0) {
		//printk("umon%d->state:%x\n",umon->id, umon->state);
		//printk("(usbces%d:%x)=%x\n",umon->id,umon->usbecs,readl(umon->usbecs));
	}

	if (umon->state != 0) {
		//printk("umon%d->state:%x\n", umon->id, umon->state);
		//printk("(usbces%d:%x)=%x\n",umon->id,umon->usbecs,readl(umon->usbecs));
		if ((umon->state == umon->old_state) && (umon->state != 0x3)) {
			umon->aotg_uhost_det = 0;
			umon->old_state = 0;
			if(umon->state==2) 
				is_ls_device[umon->id]=1;
			queue_delayed_work(umon->aotg_dev_onoff, &umon->aotg_dev_init, msecs_to_jiffies(1));
			return;
		}
	}

	umon->old_state = umon->state;
	mod_timer(&umon->hotplug_timer, jiffies + msecs_to_jiffies(500));
	return;
}

static void aotg_dev_register(struct work_struct *w)
{
	struct aotg_uhost_mon_t *umon = container_of(w, struct aotg_uhost_mon_t, aotg_dev_init.work);

	/*if (umon->id == 0) {
		aotg0_device_init(0);
	} else {
		aotg1_device_init(0);
	}*/
	if (umon->id) {
		usb_clearbitsl(USB2_PLL_EN1,aotg_uhost_mon1->usbpll);
		owl_powergate_power_off(OWL_POWERGATE_USB2_1);
	} else {
		usb_clearbitsl(USB2_PLL_EN0,aotg_uhost_mon0->usbpll);
		owl_powergate_power_off(OWL_POWERGATE_USB2_0);
	}
	wake_lock_timeout(&umon->aotg_wake_lock, 15*HZ);
	aotg_hub_register(umon->id);
	return;
}

static void aotg_dev_unregister(struct work_struct *w)
{
	struct aotg_uhost_mon_t *umon = container_of(w, struct aotg_uhost_mon_t, aotg_dev_exit.work);

	lock_system_sleep();
	wake_lock_timeout(&umon->aotg_wake_lock, 15*HZ);
	unlock_system_sleep();

	aotg_hub_unregister(umon->id);
	umon->aotg_uhost_det = 1;
	if (umon->id) {
		owl_powergate_power_on(OWL_POWERGATE_USB2_1);
		usb_setbitsl(USB2_PLL_EN1,aotg_uhost_mon1->usbpll);
		usb_setbitsl(USB2_PHYCLK_EN1,aotg_uhost_mon1->usbpll);
		usb_setbitsl(USB2_ECS_PLL_LDO_EN,aotg_uhost_mon1->usbecs);
		usb2_set_dp_500k_15k(aotg_uhost_mon1, 0, 1);
		is_ls_device[1]=0;
	} else {
		owl_powergate_power_on(OWL_POWERGATE_USB2_0);
		usb_setbitsl(USB2_PLL_EN0,aotg_uhost_mon0->usbpll);
		usb_setbitsl(USB2_PHYCLK_EN0,aotg_uhost_mon0->usbpll);
		usb_setbitsl(USB2_ECS_PLL_LDO_EN,aotg_uhost_mon0->usbecs);
		usb2_set_dp_500k_15k(aotg_uhost_mon0, 0, 1);
		is_ls_device[0]=0;
	}

	mod_timer(&umon->hotplug_timer, jiffies + msecs_to_jiffies(1000));
	return;
}

void aotg_dev_plugout_msg(int id)
{
	struct aotg_uhost_mon_t *umon = NULL;

	printk("usb%d had been plugged out!\n",id);
	if ((id == 0) && aotg_uhost_mon0) {
		umon = aotg_uhost_mon0;
	} else if ((id == 1) && aotg_uhost_mon1) {
		umon = aotg_uhost_mon1;
	} else {
		ACT_HCD_DBG
		return;
	}

	umon->old_state = 0;
	queue_delayed_work(umon->aotg_dev_onoff, &umon->aotg_dev_exit, msecs_to_jiffies(1000));
	return;
}

static struct aotg_uhost_mon_t * aotg_uhost_mon_alloc(void)
{
	struct aotg_uhost_mon_t *umon = NULL;

	umon = kzalloc(sizeof(*umon), GFP_KERNEL);
	if (!umon)
		return NULL;

	init_timer(&umon->hotplug_timer);
	umon->hotplug_timer.function = aotg_uhost_mon_timer;
	umon->hotplug_timer.data = (unsigned long)umon;

	INIT_DELAYED_WORK(&umon->aotg_dev_init, aotg_dev_register);
	INIT_DELAYED_WORK(&umon->aotg_dev_exit, aotg_dev_unregister);

	umon->aotg_uhost_det = 1;

	return umon;
}

void aotg_uhost_mon_init(struct work_struct *w)
{
	if (port_host_plug_detect[0]) {
		aotg_uhost_mon0 = aotg_uhost_mon_alloc();
		aotg_uhost_mon0->id = 0;
		aotg_uhost_mon0->aotg_dev_onoff = create_singlethread_workqueue("aotg_dev0_onoff");
		aotg_uhost_mon0->usbecs = (void __iomem *)IO_ADDRESS(USBH0_ECS);
		aotg_uhost_mon0->usbpll = (void __iomem *)IO_ADDRESS(CMU_USBPLL);

		if (aotg_udc_enable[0])
			aotg_udc_exit(0);
		owl_powergate_power_on(OWL_POWERGATE_USB2_0);
		usb_setbitsl(USB2_PLL_EN0,aotg_uhost_mon0->usbpll);
		usb_setbitsl(USB2_PHYCLK_EN0,aotg_uhost_mon0->usbpll);
		usb_setbitsl(USB2_ECS_PLL_LDO_EN,aotg_uhost_mon0->usbecs);
		usb2_set_dp_500k_15k(aotg_uhost_mon0, 0, 1);
		wake_lock_init(&aotg_uhost_mon0->aotg_wake_lock, WAKE_LOCK_SUSPEND, "aotg_wake_lock0");
		printk("start mon 0 ......\n");
		mod_timer(&aotg_uhost_mon0->hotplug_timer, jiffies + msecs_to_jiffies(10000));
        	if (aotg_udc_enable[0]){
			aotg_udc_register(0);
		}
	}
	if (port_host_plug_detect[1]) {
		aotg_uhost_mon1 = aotg_uhost_mon_alloc();
		aotg_uhost_mon1->id = 1;
		aotg_uhost_mon1->aotg_dev_onoff = create_singlethread_workqueue("aotg_dev1_onoff");
		aotg_uhost_mon1->usbecs = (void __iomem *)IO_ADDRESS(USBH1_ECS);
		aotg_uhost_mon1->usbpll = (void __iomem *)IO_ADDRESS(CMU_USBPLL);

		if (aotg_udc_enable[1])
			aotg_udc_exit(1);
		owl_powergate_power_on(OWL_POWERGATE_USB2_1);
		usb_setbitsl(USB2_PLL_EN1,aotg_uhost_mon1->usbpll);
		usb_setbitsl(USB2_PHYCLK_EN1,aotg_uhost_mon1->usbpll);
		usb_setbitsl(USB2_ECS_PLL_LDO_EN,aotg_uhost_mon1->usbecs);
		usb2_set_dp_500k_15k(aotg_uhost_mon1, 0, 1);
		wake_lock_init(&aotg_uhost_mon1->aotg_wake_lock, WAKE_LOCK_SUSPEND, "aotg_wake_lock1");
		printk("start mon 1 ......\n");
		mod_timer(&aotg_uhost_mon1->hotplug_timer, jiffies + msecs_to_jiffies(1000));
    		if (aotg_udc_enable[1]){
			aotg_udc_register(1);
		}
	}

	return;
}

static int inline aotg_uhost_mon_free(struct aotg_uhost_mon_t *umon)
{
	if (!umon)
		return -1;

	if (umon->id) {
		usb_clearbitsl(USB2_PLL_EN1,aotg_uhost_mon1->usbpll);
		owl_powergate_power_off(OWL_POWERGATE_USB2_1);
	} else {
		usb_clearbitsl(USB2_PLL_EN0,aotg_uhost_mon0->usbpll);
		owl_powergate_power_off(OWL_POWERGATE_USB2_0);
	}
	
	if (umon->aotg_dev_onoff) {
		cancel_delayed_work_sync(&umon->aotg_dev_init);
		cancel_delayed_work_sync(&umon->aotg_dev_exit);
		flush_workqueue(umon->aotg_dev_onoff);
		destroy_workqueue(umon->aotg_dev_onoff);
	}
	wake_unlock(&umon->aotg_wake_lock);
	del_timer_sync(&umon->hotplug_timer);
	kfree(umon);
	return 0;
}

void aotg_uhost_mon_exit(void)
{
	aotg_power_onoff(0,0);
	aotg_power_onoff(1,0);

	aotg_uhost_mon_free(aotg_uhost_mon0);
	aotg_uhost_mon_free(aotg_uhost_mon1);
	aotg_uhost_mon0 = NULL;
	aotg_uhost_mon1 = NULL;
	return;
}
