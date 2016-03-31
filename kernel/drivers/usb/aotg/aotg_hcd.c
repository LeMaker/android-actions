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
//#include "aotg_dma.h"
#include "aotg_hcd_debug.h"
#include "aotg_mon.h"

#define	DRIVER_DESC	"AOTG USB Host Controller Driver"
int hcd_ports_en_ctrl = 0;

static int handle_setup_packet(struct aotg_hcd *acthcd, struct aotg_queue *q);
static void handle_hcep0_in(struct aotg_hcd *acthcd);
static void handle_hcep0_out(struct aotg_hcd *acthcd);
//static int aotg_hcd_flush_queue(struct aotg_hcd *acthcd);
#if 0
#ifdef	CONFIG_PM
static void aotg_hcd_register_earlysuspend(struct aotg_hcd *acthcd);
static void aotg_hcd_unregister_earlysuspend(struct aotg_hcd *acthcd);
static void aotg_hcd_early_suspend(struct early_suspend *h);
static void aotg_hcd_late_resume(struct early_suspend *h);

typedef int (* aotg_hcd_reset_device_f)(struct usb_device *udev);
aotg_hcd_reset_device_f aotg_hcd_reset_device = NULL;
void aotg_hcd_reset_and_verify_device(struct aotg_hcd *acthcd, int reset_device);
#endif
#endif

#define MAX_PACKET(x)	((x)&0x7FF)

/* because usb0 and usb1's pll is all controlled together, 
 * we couldn't enable aotg0 and aotg1 seperately. 
 */
//static int hcd_2clk_bits_en = 0;
/* 0 is all enable, 1 -- just usb0 enable, 2 -- usb1 enable, 
 * 3 -- usb0 and usb1 enable,but reversed. 
 */

#if 0
/* forbid to enter suspend when driver is installed. */
//struct wake_lock acts_hcd_wakelock;

#ifdef	CONFIG_PM

static void aotg_hcd_register_earlysuspend(struct aotg_hcd *acthcd)
{
	if (!acthcd) {
		ACT_HCD_ERR
		return;
	}
	
	acthcd->earlysuspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 10;
	acthcd->earlysuspend.suspend = aotg_hcd_early_suspend;
	acthcd->earlysuspend.resume = aotg_hcd_late_resume;
	register_early_suspend(&acthcd->earlysuspend);
	
	acthcd->lr_flag = 0;
	
	return;
}

static void aotg_hcd_unregister_earlysuspend(struct aotg_hcd *acthcd)
{
	if (acthcd->earlysuspend.suspend) 
		unregister_early_suspend(&acthcd->earlysuspend);

	acthcd->earlysuspend.suspend = NULL;
	acthcd->earlysuspend.resume = NULL;
	
	return;
}

static void aotg_hcd_early_suspend(struct early_suspend *h)
{
	printk(KERN_DEBUG"%s do nothing!\n", __FUNCTION__);
	
	return;
}

static void aotg_hcd_late_resume(struct early_suspend *h)
{
	struct aotg_hcd *acthcd = container_of(h, struct aotg_hcd, earlysuspend);
	if (!acthcd) {
		printk(KERN_ERR"%s err, acthcd is NULL pointer!\n", __FUNCTION__);
		return;
	}
	
	if (acthcd->lr_flag) {
		acthcd->lr_flag = 0;
		printk(KERN_DEBUG"%s, %d\n", __FUNCTION__, __LINE__);
//		if(hcd_suspend_en==0)
			aotg_hcd_reset_and_verify_device(acthcd, USB_RESET_AND_VERIFY_DEVICE);
	}
	
	return;
}

#if 0
static void aotg_hcd_resume_disconnect(struct aotg_hcd *acthcd)
{
	//printk("uhost%d, usb device inserted : %d, otg state : 0x%02X\n",	acthcd->id, acthcd->inserted, usb_readb(acthcd->base + OTGSTATE) );
	if ( (acthcd->inserted ==1) && 
	    (usb_readb(acthcd->base + OTGSTATE) != AOTG_STATE_A_HOST)) {
		printk("uhost%d,usb device disconnect in suspend, so,  now disconnect it!\n", acthcd->id);
		acthcd->discon_happened = 1;
		mod_timer(&acthcd->hotplug_timer, jiffies + msecs_to_jiffies(1));
	}
}
#endif

void aotg_hcd_reset_and_verify_device(struct aotg_hcd *acthcd, int reset_device)
{	
	int i, ret = 0;
	unsigned long jiffies_expire = jiffies + HZ;
	struct usb_device *udev = NULL;
	
	printk (KERN_DEBUG"* %s, %d *\n", __FUNCTION__, __LINE__);
		
	if (acthcd == NULL){
		ACT_HCD_ERR
		return;
	}
	
	for (i = 0; i < MAX_EP_NUM; i++) {
			if (acthcd->ep0[i] != NULL) {
				break;
			}
		}
	
	if (i == MAX_EP_NUM) {
		printk(KERN_ERR"%s, usb device is NULL!\n", __FUNCTION__);
	}
	else
		udev = acthcd->ep0[i]->udev;

	if (udev == NULL) {
		printk(KERN_ERR"%s, udev is NULL, can't reset device here!\n", __FUNCTION__);
		return;
	}

	while (!usb_trylock_device(udev)) {
		if (time_after(jiffies, jiffies_expire)){
			ret = -1;
			break;
		}
		msleep(15);
	}
	
	if (ret == 0) {
		if (reset_device){
			printk (KERN_DEBUG"usb_reset_and_verify_device\n");
			aotg_hcd_reset_device = (aotg_hcd_reset_device_f)kallsyms_lookup_name("usb_reset_and_verify_device");
			if (aotg_hcd_reset_device != NULL)
				aotg_hcd_reset_device(udev);
			else 
				printk(KERN_ERR"Get reset_device fail!\n");
		} else {
				printk (KERN_DEBUG"usb_reset_device\n");
				usb_reset_device(udev);		
		}
		mutex_unlock(&(udev->dev).mutex);
	} else {
		printk (KERN_ERR"Fail to reset usb device!\n");
	}
}

EXPORT_SYMBOL_GPL (aotg_hcd_reset_and_verify_device);
#endif
#endif

typedef void (* aotg_hub_symbol_func_t)(int);
aotg_hub_symbol_func_t aotg_hub_notify_func = NULL;

void aotg_power_onoff(int id,int on_off)
{
	if(port_host_plug_detect[id] == 2)
		return;
	if(port_host_plug_detect[id] == 3){
		if(act_hcd_ptr[1-id] != NULL)//if the other port is working;don't change vbus status
			return;
	}
	if (vbus_otg_en_gpio[id][0] >= 0)
		gpio_set_value(vbus_otg_en_gpio[id][0], !(on_off^vbus_otg_en_gpio[id][1]));
}


static void aotg_hub_notify_hcd_exit(int state)
{
	static int is_first_call = 1;

	if (is_first_call) {
		is_first_call = 0;
		aotg_hub_notify_func = (aotg_hub_symbol_func_t)kallsyms_lookup_name("aotg_hub_notify_exit");
	}
	if (aotg_hub_notify_func) {
		aotg_hub_notify_func(state);
	}
	return;
}

static ulong get_fifo_addr(struct aotg_hcd *acthcd, int size)
{
	int i, j;
	ulong addr = 0;
	int mul = size / ALLOC_FIFO_UNIT;
	int max_unit = AOTG_MAX_FIFO_SIZE/ALLOC_FIFO_UNIT;
	int find_next = 0;

	if (mul == 0)
		mul = 1;

	for (i = 2; i < max_unit;) {
		if (acthcd->fifo_map[i] != 0) {
			i++;
			continue; /*find first unused addr*/
		}

		for (j = i; j < max_unit; j++) {
			if ((j - i + 1) == mul)
				break;

			if (acthcd->fifo_map[j]) {
				i = j;
				find_next = 1;
				break;
			}
		}

		if (j == max_unit) {
			break;
		} else if (find_next) {
			find_next = 0;
			continue;
		} else {
			int k;
			for (k = i; k <= j; k++)
				acthcd->fifo_map[k] = (1 << 31) | (i * 64);

			addr = i * ALLOC_FIFO_UNIT;
			break;
		}
	}

	return addr;
}

static void release_fifo_addr(struct aotg_hcd *acthcd, ulong addr)
{
	int i;

	for (i = addr/ALLOC_FIFO_UNIT; i < AOTG_MAX_FIFO_SIZE/ALLOC_FIFO_UNIT ; i++) {
		if ((acthcd->fifo_map[i] & 0x7FFFFFFF) == addr)
			acthcd->fifo_map[i] = 0;
 		else
			break;
	}
	return;
}

static struct aotg_queue * aotg_hcd_get_queue(struct aotg_hcd *acthcd, struct urb *urb, unsigned mem_flags)
{
	int i;
	int empty_idx = -1;
	struct aotg_queue *q = NULL;

	for (i = 0; i < AOTG_QUEUE_POOL_CNT; i++) {
		if (acthcd->queue_pool[i] != NULL) {
			if (acthcd->queue_pool[i]->in_using == 0) {
				q = acthcd->queue_pool[i];
				break;
			}
		} else {
			if (empty_idx < 0) {
				empty_idx = i;
			}
		}
	}
	if (i == AOTG_QUEUE_POOL_CNT) {
		q = kzalloc(sizeof(*q), GFP_ATOMIC);
		if (unlikely(!q)) {
			dev_err(acthcd->dev, "aotg_hcd_get_queue failed\n");
			return NULL;
		}
		if ((empty_idx >= 0) && (empty_idx < AOTG_QUEUE_POOL_CNT)) {
			acthcd->queue_pool[empty_idx] = q;
		}
	}

	memset(q, 0, sizeof(*q));
	q->length = 0;
	q->td.trb_vaddr = NULL;
	INIT_LIST_HEAD(&q->enqueue_list);
	INIT_LIST_HEAD(&q->dequeue_list);
	INIT_LIST_HEAD(&q->finished_list);

	q->in_using = 1;
	return q;
}

static void aotg_hcd_release_queue(struct aotg_hcd *acthcd, struct aotg_queue *q)
{
	int i;

	if (NULL == q)
		return;

	q->td.trb_vaddr = NULL;

	/* release all */
	if (q == NULL) {
		for (i = 0; i < AOTG_QUEUE_POOL_CNT; i++) {
			if (acthcd->queue_pool[i] != NULL) {
				kfree(acthcd->queue_pool[i]);
				acthcd->queue_pool[i] = NULL;
			}
		}
		return;
	}

	for (i = 0; i < AOTG_QUEUE_POOL_CNT; i++) {
		if (acthcd->queue_pool[i] == q) {
			acthcd->queue_pool[i]->in_using = 0;
			return;
		}
	}

	kfree(q);
	return;
}

static __inline__ int is_epfifo_busy(struct aotg_hcep *ep, int is_in)
{
	
	if (is_in) 
		return(EPCS_BUSY & readb(ep->reg_hcepcs)) == 0;	
	else 
		return (EPCS_BUSY & readb(ep->reg_hcepcs)) != 0;
}

static __inline__ void ep_setup(struct aotg_hcep *ep, u8 type, u8 buftype)
{
	ep->buftype = buftype;
	writeb(type | buftype, ep->reg_hcepcon);
}

static __inline__ void pio_irq_disable(struct aotg_hcd *acthcd, u8 mask)
{
	u8 is_out = mask & USB_HCD_OUT_MASK;
	u8 ep_num = mask & 0x0f;

	if (is_out) {
		usb_clearbitsw(1 << ep_num, acthcd->base + HCOUTxIEN0);
	} else {
		usb_clearbitsw(1 << ep_num, acthcd->base + HCINxIEN0);
	}
	return;
}

static __inline__ void pio_irq_enable(struct aotg_hcd *acthcd, u8 mask)
{
	u8 is_out = mask & USB_HCD_OUT_MASK;
	u8 ep_num = mask & 0x0f;

	if (is_out) {
		usb_setbitsw(1 << ep_num, acthcd->base + HCOUTxIEN0);
	} else {
		usb_setbitsw(1 << ep_num, acthcd->base + HCINxIEN0);
	}
	return;
}

static __inline__ void pio_irq_clear(struct aotg_hcd *acthcd, u8 mask)
{
	u8 is_out = mask & USB_HCD_OUT_MASK;
	u8 ep_num = mask & 0x0f;

	if (is_out) {
		writew(1 << ep_num, acthcd->base + HCOUTxIRQ0);
	}
	else {
		writew(1 << ep_num, acthcd->base + HCINxIRQ0);
	}
	return;
}

static __inline__ void ep_enable(struct aotg_hcep *ep)
{
	usb_setbitsb(0x80, ep->reg_hcepcon);
}

static __inline__ void ep_disable(struct aotg_hcep *ep)
{
	usb_clearbitsb(0x80, ep->reg_hcepcon);
}

static __inline__ void aotg_sofirq_on(struct aotg_hcd *acthcd)
{
	usb_setbitsb((1 << 1), acthcd->base + USBIEN);
}

static __inline__ void aotg_sofirq_off(struct aotg_hcd *acthcd)
{
	usb_clearbitsb(1 << 1, acthcd->base + USBIEN);
}

static __inline__ int get_subbuffer_count(u8 buftype)
{
	int count = 0;

	switch (buftype) {
	case EPCON_BUF_SINGLE:
		count = 1;
		break;
	case EPCON_BUF_DOUBLE:
		count = 2;
		break;
	case EPCON_BUF_TRIPLE:
		count = 3;
		break;
	case EPCON_BUF_QUAD:
		count = 4;
		break;
	}

	return count;
}

static inline void aotg_config_hub_addr(struct urb *urb, struct aotg_hcep *ep) 
{
	if (ep->has_hub) {
		if (urb->dev->speed == USB_SPEED_HIGH) {
			writeb(usb_pipedevice(urb->pipe), ep->reg_hcep_dev_addr);
			writeb(urb->dev->portnum, ep->reg_hcep_port);
		} else {
			writeb((0x80 | usb_pipedevice(urb->pipe)), ep->reg_hcep_dev_addr);
			if (urb->dev->speed == USB_SPEED_LOW) {
				writeb(0x80 | urb->dev->portnum, ep->reg_hcep_port);
			} else {
				writeb(urb->dev->portnum, ep->reg_hcep_port);
			}
		}
		//writeb(0, ep->reg_hcep_splitcs);
	} else {
		writeb(usb_pipedevice(urb->pipe), ep->reg_hcep_dev_addr);
		writeb(urb->dev->portnum, ep->reg_hcep_port);
	}
}

#if (1)
static void aotg_start_ring_transfer(struct aotg_hcd *acthcd, struct aotg_hcep *ep,
		struct urb *urb)
{
	u32 addr;
	struct aotg_trb *trb;
	struct aotg_ring *ring = ep->ring;

	aotg_config_hub_addr(urb, ep);
	if (usb_pipetype(urb->pipe) == PIPE_INTERRUPT) {
		writeb(ep->interval, ep->reg_hcep_interval);
		if (ring->is_out) {
			trb = ring->dequeue_trb;
			trb->hw_buf_ptr = urb->transfer_dma;
			trb->hw_buf_len = urb->transfer_buffer_length;
		}

	}
	ep_enable(ep);
	addr = ring_trb_virt_to_dma(ring, ring->dequeue_trb);
	aotg_start_ring(ring, addr);
}

#else
static void aotg_start_ring_transfer(struct aotg_hcd *acthcd, struct aotg_hcep *ep,
							struct urb *urb)
{
	u32 addr;
	struct aotg_ring *ring = ep->ring;

	aotg_config_hub_addr(urb, ep);
	if (usb_pipetype(urb->pipe) == PIPE_INTERRUPT) {
		writeb(ep->interval, ep->reg_hcep_interval);
		addr = ring_trb_virt_to_dma(ring, ring->first_trb);
	} else {
		addr = ring_trb_virt_to_dma(ring, ring->dequeue_trb);
	}
	ep_enable(ep);	
	aotg_start_ring(ring, addr);
}
#endif

static int aotg_hcep_config(struct aotg_hcd *acthcd,
			    struct aotg_hcep *ep,
			    u8 type, u8 buftype, int is_out)
{
	int index = 0;
	ulong addr = 0;
	int get_ep = 0;
	int subbuffer_count;
	//u8 fifo_ctrl;

	if (0 == (subbuffer_count = get_subbuffer_count(buftype))) {
		dev_err(acthcd->dev, "error buftype: %02X, %s, %d\n", buftype, __func__, __LINE__);
		return -EPIPE;
	}

	if (is_out) {
		for (index = 1; index < MAX_EP_NUM; index++) {
			if (acthcd->outep[index] == NULL) {
				ep->is_out = 1;
				ep->index = index;
				ep->mask = (u8) (USB_HCD_OUT_MASK | index);
				acthcd->outep[index] = ep;
				get_ep = 1;
				break;
			}
		}
	} else {
		for (index = 1; index < MAX_EP_NUM; index++) {
			if (acthcd->inep[index] == NULL) {
				ep->is_out = 0;
				ep->index = index;
				ep->mask = (u8) index;
				acthcd->inep[index] = ep;
				get_ep = 1;
				break;
			}
		}
	}

	if (!get_ep) {
		dev_err(acthcd->dev, "%s: no more available space for ep\n", __func__);
		return -ENOSPC;
	}

	addr = get_fifo_addr(acthcd, subbuffer_count * MAX_PACKET(ep->maxpacket));
	if (addr == 0) {
		dev_err(acthcd->dev, "buffer configuration overload!! addr: %08X, subbuffer_count: %d, ep->maxpacket: %u\n",
				(u32)addr, subbuffer_count, MAX_PACKET(ep->maxpacket));
		if (is_out) {
			acthcd->outep[ep->index] = NULL;
		}
		else {
			acthcd->inep[ep->index] = NULL;
		}
		return -ENOSPC;
	}
	else {
		ep->fifo_addr = addr;
	}

	ep->reg_hcepcon = get_hcepcon_reg(is_out, 
							acthcd->base + HCOUT1CON, 
							acthcd->base + HCIN1CON, 
							ep->index);
	ep->reg_hcepcs = get_hcepcs_reg(is_out, 
							acthcd->base + HCOUT1CS, 
							acthcd->base + HCIN1CS, 
							ep->index);
	ep->reg_hcepbc = get_hcepbc_reg(is_out, 
							acthcd->base + HCOUT1BCL, 
							acthcd->base + HCIN1BCL, 
							ep->index);
	ep->reg_hcepctrl = get_hcepctrl_reg(is_out, 
							acthcd->base + HCOUT1CTRL, 
							acthcd->base + HCIN1CTRL, 
							ep->index);
	ep->reg_hcmaxpck = get_hcepmaxpck_reg(is_out, 
							acthcd->base + HCOUT1MAXPCKL, 
							acthcd->base + HCIN1MAXPCKL, 
							ep->index);
	ep->reg_hcepaddr = get_hcepaddr_reg(is_out, 
							acthcd->base + HCOUT1STADDR, 
	    						acthcd->base + HCIN1STADDR, 
	    						ep->index);
	ep->reg_hcep_dev_addr = get_hcep_dev_addr_reg(is_out,
							acthcd->base + HCOUT1ADDR, 
	    						acthcd->base + HCIN1ADDR, 
	    						ep->index);
	ep->reg_hcep_port = get_hcep_port_reg(is_out,
							acthcd->base + HCOUT1PORT, 
	    						acthcd->base + HCIN1PORT, 
	    						ep->index);
	ep->reg_hcep_splitcs = get_hcep_splitcs_reg(is_out,
							acthcd->base + HCOUT1SPILITCS, 
	    						acthcd->base + HCIN1SPILITCS, 
	    						ep->index);

	//ep->reg_hcfifo = get_hcfifo_reg(acthcd->base + FIFO1DATA, ep->index);
	if (!is_out) {
		///* 5202 is just for write, read's HCINXCOUNT address is not the same with write address. */
		//ep->reg_hcincount_wt = acthcd->base + HCIN1_COUNTL + (ep->index - 1) * 4;
		//ep->reg_hcincount_rd = acthcd->base + HCIN1_COUNTL + (ep->index - 1) * 2;
		ep->reg_hcerr = acthcd->base + HCIN0ERR + ep->index * 0x4;
		ep->reg_hcep_interval = acthcd->base + HCEP0BINTERVAL + ep->index * 0x8;
	}
	else {
		ep->reg_hcerr = acthcd->base + HCOUT0ERR + ep->index * 0x4;
		ep->reg_hcep_interval = acthcd->base + HCOUT1BINTERVAL + (ep->index - 1) * 0x8;
	}

#ifdef DEBUG_EP_CONFIG
	dev_info(acthcd->dev, "== ep->index: %d, is_out: %d, fifo addr: %08X\n", ep->index, is_out, (u32)addr);
	dev_info(acthcd->dev, "== reg_hcepcon: %08lX, reg_hcepcs: %08lX, reg_hcepbc: %08lX, reg_hcepctrl: %08lX, reg_hcmaxpck: %08lX, ep->reg_hcepaddr: %08lX\n",
			ep->reg_hcepcon,
			ep->reg_hcepcs,
			ep->reg_hcepbc,
			ep->reg_hcepctrl,
			ep->reg_hcmaxpck,
			ep->reg_hcepaddr);
#endif
	
	pio_irq_disable(acthcd, ep->mask);
	pio_irq_clear(acthcd, ep->mask);
	
	ep_disable(ep);

	/*allocate buffer address of ep fifo */
	writel(addr, ep->reg_hcepaddr);
	writew(ep->maxpacket, ep->reg_hcmaxpck);
	ep_setup(ep, type, buftype);	/*ep setup */
	
	/*reset this ep */
	usb_settoggle(ep->udev, ep->epnum, is_out, 0);
	aotg_hcep_reset(acthcd, ep->mask, ENDPRST_FIFORST | ENDPRST_TOGRST);
	writeb(ep->epnum, ep->reg_hcepctrl);

	//fifo_ctrl = (1<<5) | ((!!is_out) << 4) | ep->index; //set auto fifo
	//writeb(fifo_ctrl, acthcd->base + FIFOCTRL);
	//pio_irq_enable(acthcd, ep->mask);

	return 0;
}

static int aotg_hcep_set_split_micro_frame(struct aotg_hcd *acthcd, struct aotg_hcep *ep)
{
	static const u8 split_val[] = {0x31, 0x42, 0x53, 0x64, 0x75, 0x17, 0x20};
	int i, index;
	u8 set_val, rd_val;

	for (i=0; i<sizeof(split_val); i++) {
		set_val = split_val[i];

		for (index=0; index<MAX_EP_NUM; index++) {
			if (acthcd->inep[index] != NULL) {
				rd_val = acthcd->inep[index]->reg_hcep_splitcs_val;

				if ((0 == rd_val) || (set_val != rd_val)) {
					continue;
				}
				if (set_val == rd_val)
					set_val = 0;
				break;
			}
		}
		if (set_val == 0)
			continue;

		for (index=0; index<MAX_EP_NUM; index++) {
			if (acthcd->outep[index] != NULL) {
				rd_val = acthcd->outep[index]->reg_hcep_splitcs_val;

				if ((0 == rd_val) || (set_val != rd_val)) {
					continue;
				}
				if (set_val == rd_val)
					set_val = 0;
				break;
			}
		}

		if (set_val != 0)
			break;
	}

	if (set_val != 0) {
		ep->reg_hcep_splitcs_val = set_val;
		writeb(set_val, ep->reg_hcep_splitcs);
		printk("====reg_hcep_splitcs_val:%x, index:%d\n", set_val, ep->index);
	}
	return 0;
}

static void finish_request(struct aotg_hcd *acthcd,
			   struct aotg_queue *q,
			   int status)
{
	struct urb *urb = q->urb;

	if (unlikely((acthcd == NULL) || (q == NULL) || (urb == NULL))) {
		WARN_ON(1);
		return;
	}

	q->status = status;
	if (list_empty(&q->finished_list)) {
		list_add_tail(&q->finished_list, &acthcd->hcd_finished_list);
	} else {
		ACT_HCD_ERR
	}
	tasklet_hi_schedule(&acthcd->urb_tasklet);
	return;
}

static void tasklet_finish_request(struct aotg_hcd *acthcd,
			   struct aotg_queue *q,
			   int status)
{
	struct urb *urb = q->urb;
	struct aotg_hcep *ep = q->ep;

	if (unlikely((acthcd == NULL) || (q == NULL) || (urb == NULL))) {
		WARN_ON(1);
		return;
	}

	if ((q != NULL) && (ep != NULL)) {
		if (ep->q == NULL) {
			ACT_HCD_ERR
		} else {
			if (ep->q == q) {
				ep->q = NULL;
			}
		}
	} else {
		ACT_HCD_ERR
		return;
	}

	if (status == 0) {
		q->err_count = 0;
	}

	if (usb_pipetype(urb->pipe) == PIPE_CONTROL) {
		if ((acthcd->active_ep0 != NULL) && (acthcd->active_ep0 == q->ep)) {
			if (acthcd->active_ep0->q == NULL) {
				acthcd->active_ep0 = NULL;
			} else {
				ACT_HCD_ERR
			}
		} else {
			ACT_HCD_ERR
		}
	} 
#if 0
	if (q->td.trb_vaddr && q->td.trb_num) {
		dma_free_coherent(aotg_to_hcd(acthcd)->self.controller,
			q->td.trb_num * sizeof(struct aotg_trb),
			q->td.trb_vaddr, q->td.trb_dma);
	}
#endif
	aotg_dbg_finish_q(q);
	aotg_hcd_release_queue(acthcd, q);
	//usb_hcd_unlink_urb_from_ep(hcd, urb);
	//usb_hcd_giveback_urb(hcd, urb, status);

	ep->urb_endque_cnt++;
	//ep->fifo_busy = 0;
	//if (usb_pipeint(urb->pipe)) 
	return;
}

static __inline__ void handle_status(struct aotg_hcd *acthcd, struct aotg_hcep *ep, int is_out)
{
	/*status always DATA1,set 1 to ep0 toggle */
	writeb(EP0CS_HCSETTOOGLE, acthcd->base + EP0CS);

	if (is_out) {
		writeb(0, acthcd->base + HCIN0BC); //recv 0 packet
	}
	else {
		writeb(0, acthcd->base + HCOUT0BC); //send 0 packet
	}
}

static void write_hcep0_fifo(struct aotg_hcd *acthcd, struct aotg_hcep *ep, struct urb *urb)
{
	u32 *buf;
	int length, count;
	void __iomem *addr = acthcd->base + EP0INDATA_W0;

	if (!(readb(acthcd->base + EP0CS) & EP0CS_HCOUTBSY)) {
		buf = (u32 *) (urb->transfer_buffer + urb->actual_length);
		prefetch(buf);

		/* how big will this packet be? */
		length = min((int)ep->maxpacket, (int)urb->transfer_buffer_length - (int)urb->actual_length);

		count = length >> 2;	/*wirte in DWORD */
		if (length & 0x3) count++;

		while (likely(count--)) {
			writel(*buf, addr);
			buf++;
			addr += 4;
		}

		ep->length = length;
		writeb(length, acthcd->base + HCOUT0BC);
		usb_dotoggle(urb->dev, usb_pipeendpoint(urb->pipe), 1);
	} else {
		dev_err(acthcd->dev, "<CTRL>OUT data is not ready\n");
	}
}

static void read_hcep0_fifo(struct aotg_hcd *acthcd, struct aotg_hcep *ep, struct urb *urb)
{
	u8 *buf;
	unsigned overflag, is_short, shorterr, is_last;
	unsigned length, count;
	struct usb_device *udev;
	void __iomem *addr = acthcd->base + EP0OUTDATA_W0;  //HCEP0INDAT0;
	unsigned bufferspace;

	overflag = 0;
	is_short = 0;
	shorterr = 0;
	is_last = 0;
	udev = ep->udev;

	if (readb(acthcd->base + EP0CS) & EP0CS_HCINBSY) {
		dev_err(acthcd->dev, "<CTRL>IN data is not ready\n");
		return;
	} else {
		usb_dotoggle(udev, ep->epnum, 0);
		buf = urb->transfer_buffer + urb->actual_length;
		bufferspace = urb->transfer_buffer_length - urb->actual_length;
		//prefetch(buf);

		length = count = readb(acthcd->base + HCIN0BC);
		if (length > bufferspace) {
			count = bufferspace;
			urb->status = -EOVERFLOW;
			overflag = 1;
		}

		urb->actual_length += count;
		while (count--) {
			*buf++ = readb(addr);
#if 0
			buf--;
			printk("ep0in:%x, cnt:%d\n", (unsigned int)*buf, count);
			buf++;
#endif
			addr++;
		}

		if (urb->actual_length >= urb->transfer_buffer_length) {
			ep->nextpid = USB_PID_ACK;
			is_last = 1;
			handle_status(acthcd, ep, 0);
		} else if (length < ep->maxpacket) {
			is_short = 1;
			is_last = 1;
			if (urb->transfer_flags & URB_SHORT_NOT_OK) {
				urb->status = -EREMOTEIO;
				shorterr = 1;
			}
			ep->nextpid = USB_PID_ACK;
			handle_status(acthcd, ep, 0);
		}
		else {
			writeb(0, acthcd->base + HCIN0BC);
		}
	}
}

static int handle_setup_packet(struct aotg_hcd *acthcd, struct aotg_queue *q)
{
	struct urb *urb = q->urb;
	struct aotg_hcep *ep = q->ep;
	u32 *buf;
	void __iomem *addr = acthcd->base + EP0INDATA_W0;
	int i = 0;

#ifdef DEBUG_SETUP_DATA
	u16 w_value, w_index, w_length;
	struct usb_ctrlrequest *ctrlreq;

	ctrlreq = (struct usb_ctrlrequest *)urb->setup_packet;
	w_value = le16_to_cpu(ctrlreq->wValue);
	w_index = le16_to_cpu(ctrlreq->wIndex);
	w_length = le16_to_cpu(ctrlreq->wLength);
	dev_info(acthcd->dev, "<CTRL>SETUP stage  %02x.%02x V%04x I%04x L%04x\n ",
		  ctrlreq->bRequestType, ctrlreq->bRequest, w_value, w_index,
		  w_length);
#endif
	if ((q->is_xfer_start) || (ep->q)) {
		ACT_HCD_DBG
		printk("q->is_xfer_start:%d\n", q->is_xfer_start);
		return 0;
	}
	if (unlikely(!HC_IS_RUNNING(aotg_to_hcd(acthcd)->state))) {
		ACT_HCD_DBG
		return -ESHUTDOWN;
	}
	if (acthcd->active_ep0 != NULL) {
		ACT_HCD_ERR
		return -EBUSY;
	}

	writeb(ep->epnum, acthcd->base + HCEP0CTRL);
	writeb((u8)ep->maxpacket, acthcd->base + HCIN0MAXPCK);

	acthcd->active_ep0 = ep;
	ep->q = q;
	q->is_xfer_start = 1;
	usb_settoggle(urb->dev, usb_pipeendpoint(urb->pipe), 1, 1);
	ep->nextpid = USB_PID_SETUP;
	buf = (u32 *) urb->setup_packet;

	/*initialize the setup stage */
	writeb(EP0CS_HCSET, acthcd->base + EP0CS);
	while (readb(acthcd->base + EP0CS) & EP0CS_HCOUTBSY) {
		writeb(EP0CS_HCSET, acthcd->base + EP0CS);
		i++;
		if (i > 2000000) {
			printk("handle_setup timeout!\n");
			break;
		}
	}
	
	if (!(readb(acthcd->base + EP0CS) & EP0CS_HCOUTBSY)) {
		/*fill the setup data in fifo */
		writel(*buf, addr);
		addr += 4;
		buf++;
		writel(*buf, addr);
		writeb(8, acthcd->base + HCOUT0BC);
	}
	else {
		dev_warn(acthcd->dev, "setup ep busy!!!!!!!\n");
	}

	return 0;
}

static void handle_hcep0_out(struct aotg_hcd *acthcd)
{
	struct aotg_hcep *ep;
	struct urb *urb;
	struct usb_device *udev;
	struct aotg_queue *q;
	
	ep = acthcd->active_ep0;
	
	if (unlikely(!ep)) {
		ACT_HCD_ERR
		return;
	}
	q = ep->q;
	if (q == NULL) {
		ACT_HCD_ERR
		return;
	}

	urb = q->urb;
	udev = ep->udev;

	switch (ep->nextpid) {
	case USB_PID_SETUP:
		if (urb->transfer_buffer_length == urb->actual_length) {
			ep->nextpid = USB_PID_ACK;
			handle_status(acthcd, ep, 1);	/*no-data transfer */
		} else if (usb_pipeout(urb->pipe)) {
			usb_settoggle(udev, 0, 1, 1);
			ep->nextpid = USB_PID_OUT;
			write_hcep0_fifo(acthcd, ep, urb);
		} else {
			usb_settoggle(udev, 0, 0, 1);
			ep->nextpid = USB_PID_IN;
			writeb(0, acthcd->base + HCIN0BC);
		}
		break;
	case USB_PID_OUT:
		urb->actual_length += ep->length;
		usb_dotoggle(udev, ep->epnum, 1);
		if (urb->actual_length >= urb->transfer_buffer_length) {
			ep->nextpid = USB_PID_ACK;
			handle_status(acthcd, ep, 1);	/*control write transfer */
		}
		else {
			ep->nextpid = USB_PID_OUT;
			write_hcep0_fifo(acthcd, ep, urb);
		}
		break;
	case USB_PID_ACK:
		finish_request(acthcd, q, 0);
		break;
	default:
		dev_err(acthcd->dev, "<CTRL>ep0 out ,odd pid %d, %s, %d\n", 
				ep->nextpid, __func__, __LINE__);
	}
}

static void handle_hcep0_in(struct aotg_hcd *acthcd)
{
	struct aotg_hcep *ep;
	struct urb *urb;
	struct usb_device *udev;
	struct aotg_queue *q;
	
	ep = acthcd->active_ep0;
	if (unlikely(!ep)) {
		return;
	}
	q = ep->q;
	if (q == NULL) {
		ACT_HCD_ERR
		return;
	}

	urb = q->urb;
	udev = ep->udev;

	switch (ep->nextpid) {
	case USB_PID_IN:
		read_hcep0_fifo(acthcd, ep, urb);
		break;
	case USB_PID_ACK:
		finish_request(acthcd, q, 0);
		break;
	default:
		dev_err(acthcd->dev, "<CTRL>ep0 out ,odd pid %d\n", ep->nextpid);
	}
}

static void aotg_hcd_err_handle(struct aotg_hcd *acthcd, u32 irqvector, 
				int ep_num, int is_in)
{
	struct urb *urb;
	struct aotg_queue *q;
	struct aotg_hcep *ep = NULL;
	struct aotg_ring *ring = NULL;
	struct aotg_td *td = NULL;
	int status = -EOVERFLOW;
	u8 err_val = 0;
	u8 err_type = 0;
	u8 reset = 0;
	struct usb_hcd *hcd = aotg_to_hcd(acthcd);

	printk("hcd ep err ep_num:%d, is_in:%d\n", ep_num, is_in);
		
	if (ep_num == 0) {
		ep = acthcd->active_ep0;
		if (ep == NULL) {
			ACT_HCD_ERR
			return;
		}
		q = ep->q;
		if (is_in) {
			ep->reg_hcerr = acthcd->base + HCIN0ERR;
		} else {
			ep->reg_hcerr = acthcd->base + HCOUT0ERR;
		}
	} else {
		if (is_in) {
			ep = acthcd->inep[ep_num];
		} else {
			ep = acthcd->outep[ep_num];
		}
		if (ep == NULL) {
			ACT_HCD_ERR
			printk("is_in:%d, ep_num:%d\n", is_in, ep_num);
			return;
		}
		ring = ep->ring;
		if (!ring) {
			ACT_HCD_ERR
			return;
		}
		td = list_first_entry_or_null(&ep->enring_td_list, struct aotg_td, enring_list);
		if (!td) {
			aotg_stop_ring(ring);
			ACT_HCD_ERR
			return;
		}
	}
	
	err_val = readb(ep->reg_hcerr);
	if (is_in) {
		writew(1 << ep_num, acthcd->base + HCINxERRIRQ0);
	} else {
		writew(1 << ep_num, acthcd->base + HCOUTxERRIRQ0);
	}

	err_type = err_val & HCINxERR_TYPE_MASK;
	printk("err_type:%x\n",err_type>>2);
	switch (err_type) {
	case HCINxERR_NO_ERR:
	case HCINxERR_OVER_RUN:
		status = -EOVERFLOW;
		break;
	case HCINxERR_UNDER_RUN:
		status = -EREMOTEIO;
		break;
	case HCINxERR_STALL:
		status = -EPIPE;
		break;
	case HCINxERR_TIMEOUT:
		status = -ETIMEDOUT;
		break;
	case HCINxERR_CRC_ERR:
	case HCINxERR_TOG_ERR:
	case HCINxERR_PID_ERR:
		status = -EPROTO;
		break;
	//case HCINxERR_SPLIET:
	default:
		printk("err_val:0x%x, err_type:%d\n", err_val, err_type);
		if (is_in) {
			printk("HCINEP%dSPILITCS:0x%x\n", ep_num, 
					readb(acthcd->base + ep_num * 8 + HCEP0SPILITCS));
		} else { 
			printk("HCOUTEP%dSPILITCS:0x%x\n", ep_num, 
					readb(acthcd->base + (ep_num - 1) * 8 + HCOUT1SPILITCS));
		}
		status = -EPIPE;
		break;

	//default:
	//	printk("err_type:%x\n", err_type);
	//	status = -EOVERFLOW;
	//	reset = ENDPRST_FIFORST | ENDPRST_TOGRST;
	}

	if (!(acthcd->port & USB_PORT_STAT_ENABLE)
			|| (acthcd->port & (USB_PORT_STAT_C_CONNECTION << 16))
			|| (acthcd->hcd_exiting != 0)
			|| (acthcd->inserted == 0)
    		|| !HC_IS_RUNNING(hcd->state)) {
		dev_err(acthcd->dev, "usbport, dead, port:%x, hcd_exiting:%d \n", acthcd->port, acthcd->hcd_exiting);
		status = -ENODEV;
	}

	if (ep->index == 0) {
		q = ep->q;
		urb = q->urb;
		if ((status == -EPIPE) || (status == -ENODEV)) 
			writeb(HCINxERR_RESEND, ep->reg_hcerr);  /* resend. */
		finish_request(acthcd, q, status);
		dev_info(acthcd->dev, "%s ep %d error [0x%02X] error type [0x%02X], reset it...\n",
			    usb_pipeout(urb->pipe)?"HC OUT":"HC IN", ep->index, err_val, (err_val>>2)&0x7);
	} else {
		if ((status != -EPIPE) && (status != -ENODEV)) {
			printk("td->err_count:%d\n", td->err_count);
			td->err_count++;
			
			if (td->err_count < MAX_ERROR_COUNT) {				
				writeb(HCINxERR_RESEND, ep->reg_hcerr);  /* resend. */
				return;
			}		
		}
			if (status == -ETIMEDOUT || status == -EPIPE) {
					ep->error_count++;
		}
		
		reset = ENDPRST_FIFORST | ENDPRST_TOGRST;
		ep_disable(ep);
		if (is_in) {
			aotg_hcep_reset(acthcd, ep->mask, reset);
		} else {
			aotg_hcep_reset(acthcd, ep->mask | USB_HCD_OUT_MASK, reset);
		}
		
		/*if (usb_pipeout(urb->pipe)) {
			aotg_hcep_reset(acthcd, ep->mask | USB_HCD_OUT_MASK, reset);
		} else {
			aotg_hcep_reset(acthcd, ep->mask, reset);
		}*/
		
		aotg_stop_ring(ring);
		urb = td->urb;
		//writel(DMACTRL_DMACC,ep->ring->reg_dmactrl);
		if (ep->type == PIPE_INTERRUPT)
			dequeue_intr_td(ring, td);
		else
			dequeue_td(ring, td, TD_IN_FINISH);
		
		if (urb) {
			usb_hcd_unlink_urb_from_ep(hcd, urb);
			usb_hcd_giveback_urb(hcd, urb, status);
		}
		else
		{
			ERR("urb not exist!\n");
		}

		/*
		 * after, need to rewrite port_num, dev_addr when using hub ?
		 */
		/*if ((urb) && (!list_empty(&ep->enring_td_list)) &&
				!is_ring_running(ring)) {
			ACT_HCD_DBG
			ep_enable(ep);	
			addr = ring_trb_virt_to_dma(ring, ring->dequeue_trb);
			aotg_start_ring(ring, addr);
		}*/
		dev_info(acthcd->dev, "%s ep %d error [0x%02X] error type [0x%02X], reset it...\n",
			    is_in?"HC IN":"HC OUT", ep->index, err_val, (err_val>>2)&0x7);
	}
	
	return;
}
#if 0
static void aotg_hcd_error(struct aotg_hcd *acthcd, u32 irqvector, int ep_num, int is_in)
{
	struct aotg_queue *q;
	struct aotg_hcep *ep = NULL;
	struct urb *urb;
	int status;
	u8 error = 0;
	u8 reset = 0;
	struct usb_hcd *hcd = aotg_to_hcd(acthcd);

	printk("hcd ep err ep_num:%d, is_in:%d\n", ep_num, is_in);

	if (ep_num == 0) {
		ep = acthcd->active_ep0;
		if (ep == NULL) {
			ACT_HCD_ERR
			return;
		}
		q = ep->q;
		if (is_in) {
			ep->reg_hcerr = acthcd->base + HCIN0ERR;
		} else {
			ep->reg_hcerr = acthcd->base + HCOUT0ERR;
		}
	} else {
		if (is_in) {
			ep = acthcd->inep[ep_num];
		} else {
			ep = acthcd->outep[ep_num];
		}
		if (ep == NULL) {
			ACT_HCD_ERR
			printk("is_in:%d, ep_num:%d\n", is_in, ep_num);
			return;
		}
		q = ep->q;
	}

	if (is_in) {
		writew(1 << ep_num, acthcd->base + HCINxERRIRQ0);
	} else {
		writew(1 << ep_num, acthcd->base + HCOUTxERRIRQ0);
	}
	error = readb(ep->reg_hcerr);

	if (q) {
		urb = q->urb;

		switch (error & HCINxERR_TYPE_MASK) {
		case HCINxERR_NO_ERR:
			status = 0;
			break;
		
		case HCINxERR_STALL:
			status = -EPIPE;
			reset = ENDPRST_FIFORST | ENDPRST_TOGRST;
			break;
		
		case HCINxERR_TIMEOUT:
			status = -ETIMEDOUT;
			reset = ENDPRST_FIFORST | ENDPRST_TOGRST;
			break;

		default:
			printk("error:%x\n", ((error & HCINxERR_TYPE_MASK) >> 2));
			status = -EIO;
			reset = ENDPRST_FIFORST | ENDPRST_TOGRST;
		}

		if (!(acthcd->port & USB_PORT_STAT_ENABLE)
			|| (acthcd->port & (USB_PORT_STAT_C_CONNECTION << 16))
			|| (acthcd->hcd_exiting != 0)
			|| (acthcd->inserted == 0)
	    		|| !HC_IS_RUNNING(hcd->state)) {
			dev_err(acthcd->dev, "usbport, dead, port:%x, hcd_exiting:%d \n", acthcd->port, acthcd->hcd_exiting);
			status = -ENODEV;
		}

		if (status < 0) {
			ACT_HCD_DBG
			printk("status:%d\n", status);

			if (ep->index > 0 && (status != -EPIPE) && (status != -ENODEV)) {
				printk("q->err_count:%d\n", q->err_count);
				q->err_count++;
				writeb(HCINxERR_RESEND, ep->reg_hcerr);  /* resend. */

				if ((q->err_count % MAX_ERROR_COUNT) < (MAX_ERROR_COUNT - 1)) {
					return;
				}
				if (q->err_count > MAX_ERROR_COUNT * 3) {
					ACT_HCD_DBG
					q->err_count = 0;
					acthcd->discon_happened = 1;
					mod_timer(&acthcd->hotplug_timer, jiffies + msecs_to_jiffies(1));
					return;
				}
			}

			if ((ep->index == 0) && ((status == -EPIPE) || (status == -ENODEV))) {
				writeb(HCINxERR_RESEND, ep->reg_hcerr);  /* resend. */
			}

			if (ep->index > 0) {
				if (usb_pipeout(urb->pipe)) {
					aotg_hcep_reset(acthcd, ep->mask | USB_HCD_OUT_MASK, reset);
				} else {
					ep_disable(ep);
					aotg_hcep_reset(acthcd, ep->mask, reset);
				}
			}
			dev_info(acthcd->dev, "%s ep %d error [0x%02X] error type [0x%02X], reset it...\n",
				    usb_pipeout(urb->pipe)?"HC OUT":"HC IN", ep->index, error, (error>>2)&0x7);
#if 0
			if (AOTG_GET_DMA_NUM(q->dma_no)) {
				ACT_HCD_DBG
				__clear_dma(acthcd, q);
			}
#endif
			finish_request(acthcd, q, status);
		}
	}
	tasklet_hi_schedule(&acthcd->urb_tasklet);

	return;
}
#endif 

void aotg_hcd_abort_urb(struct aotg_hcd *acthcd)
{
	int cnt;
	struct aotg_hcep *ep;
	struct urb *urb;
	struct aotg_ring *ring;
	struct aotg_td *td;
	unsigned long flags;
//	struct aotg_queue *q;
	struct usb_hcd *hcd = aotg_to_hcd(acthcd);
	
/*	if (HC_IS_SUSPENDED(hcd->state)) {
		usb_hcd_resume_root_hub(hcd);
	}
		//ACT_HCD_DBG
		//aotg_hcd_flush_queue(acthcd);
	usb_hcd_poll_rh_status(hcd);*/
	
	spin_lock_irqsave(&acthcd->lock, flags);
	/*ep = acthcd->active_ep0;
	if (ep && ep->q) {
		q = ep->q;
		urb = q->urb;
		q->status = -ENODEV;
		//printk("%s in ep 0\n",__func__);
		aotg_hcd_release_queue(acthcd, q);
		usb_hcd_unlink_urb_from_ep(hcd, urb);
		spin_unlock(&acthcd->lock);
		usb_hcd_giveback_urb(hcd, urb, -ENODEV);
		spin_lock(&acthcd->lock);
	}*/
	
	for (cnt=1; cnt<MAX_EP_NUM; cnt++) {
		ep = acthcd->inep[cnt];
		if (ep) {
			ring = ep->ring;
			td = list_first_entry_or_null(&ep->enring_td_list, struct aotg_td, enring_list);
			if (!td)
				continue;
			urb = td->urb;
			if (!urb)
				continue;
			if (ep->type == PIPE_INTERRUPT)
				dequeue_intr_td(ring, td);
			else
				dequeue_td(ring, td, TD_IN_FINISH);
			//printk("%s in ep %d\n",__func__,cnt);
			usb_hcd_unlink_urb_from_ep(hcd, urb);
			spin_unlock(&acthcd->lock);
			usb_hcd_giveback_urb(hcd, urb, -ENODEV);
			spin_lock(&acthcd->lock);
		}
	}
	
	for (cnt=1; cnt<MAX_EP_NUM; cnt++) {
		ep = acthcd->outep[cnt];
		if (ep) {
			ring = ep->ring;
			td = list_first_entry_or_null(&ep->enring_td_list, struct aotg_td, enring_list);
			if (!td)
				continue;
			urb = td->urb;
			if (!urb)
				continue;
			if (ep->type == PIPE_INTERRUPT)
				dequeue_intr_td(ring, td);
			else
				dequeue_td(ring, td, TD_IN_FINISH);
			//printk("%s out ep %d\n",__func__,cnt);
			usb_hcd_unlink_urb_from_ep(hcd, urb);
			spin_unlock(&acthcd->lock);
			usb_hcd_giveback_urb(hcd, urb, -ENODEV);
			spin_lock(&acthcd->lock);
		}
	}
	
	spin_unlock_irqrestore(&acthcd->lock, flags);
}

static irqreturn_t aotg_hub_irq(struct usb_hcd *hcd)
{
	struct platform_device *pdev;
	unsigned int port_no;
	u32 irqvector;
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	u8 eirq_mask = readb(acthcd->base + USBEIEN);
	u8 eirq_pending = readb(acthcd->base + USBEIRQ);
	u8 otg_state;

	/* take cate to use lock, because in irq -> dma_handler -> finish_request -> 
	 * usb_hcd_giveback_urb -> urb->complete(), it maybe call enqueue and get spin_lock again.
	 */
	//spin_lock(&acthcd->lock);
	pdev = to_platform_device(hcd->self.controller);
	port_no = pdev->id & 0xff;
#if(0)
	int i = 0;
	irqvector = (u32)readb(acthcd->base + IVECT);
	printk("USBEIEN:0x%x, USBEIRQ:0x%x, ivec:0x%x\n", readb(acthcd->base + USBEIEN),
			readb(acthcd->base + USBEIRQ), irqvector);
	printk("HCINxDMAIRQ0:0x%x, HCOUTxDMAIRQ0:0x%x\n",
		readw(acthcd->base + HCINxDMAIRQ0),
		readw(acthcd->base + HCOUTxDMAIRQ0));
	for (i = 0; i <= 0x1d; i++) {
		printk("0x%p : 0x%x\n", acthcd->base + 0x500 + i,
			readb(acthcd->base + 0x500 + i));
	}
#endif

	if (eirq_pending & USBEIRQ_USBIRQ) {
		irqvector = (u32)readb(acthcd->base + IVECT);
		writeb(eirq_mask | USBEIRQ_USBIRQ, acthcd->base + USBEIRQ);
//		HUB_DEBUG("irqvector:%d, 0x%x\n", irqvector, irqvector);

		switch (irqvector) {
		//case UIV_OTGIRQ:
		case UIV_IDLE:
		case UIV_SRPDET:
		case UIV_LOCSOF:
		case UIV_VBUSERR:
		case UIV_PERIPH:
			if (readb(acthcd->base + OTGIRQ) & (0x1<<2)) {
				writeb(0x1<<2, acthcd->base + OTGIRQ);
				otg_state = readb(acthcd->base + OTGSTATE);

				printk("port_no:%d OTG IRQ, OTGSTATE: 0x%02X, USBIRQ:0x%02X\n", 
					port_no, otg_state,
					readb(acthcd->base + USBIRQ));

				if (otg_state == 0x4) {
					return IRQ_HANDLED;
				}
				
				if ((otg_state == 0x02) && port_host_plug_detect[acthcd->id]) {
					aotg_disable_irq(acthcd);
					acthcd->hcd_exiting = 1;
					aotg_hcd_abort_urb(acthcd);
					aotg_dev_plugout_msg(acthcd->id);
					return IRQ_HANDLED;
				}
				acthcd->put_aout_msg = 0;
				if (otg_state == AOTG_STATE_A_HOST) {
					/*if (acthcd->inserted != 0) {
						acthcd->discon_happened = 1;
					}*/

					//if (acthcd->port & (USB_PORT_STAT_C_CONNECTION << 16)) {
					//	mod_timer(&acthcd->hotplug_timer, jiffies + msecs_to_jiffies(1000));
					//} else {
						if (acthcd->discon_happened == 1) {
							mod_timer(&acthcd->hotplug_timer, jiffies + msecs_to_jiffies(500));
						} else {
							mod_timer(&acthcd->hotplug_timer, jiffies + msecs_to_jiffies(1));
						}
					//}
				} else {
					acthcd->discon_happened = 1;
					mod_timer(&acthcd->hotplug_timer, jiffies + msecs_to_jiffies(1));
				}
			} else {
				printk("port_no:%d error OTG irq! OTGIRQ: 0x%02X\n", 
					port_no, readb(acthcd->base + OTGIRQ));
			}
			break;
		case UIV_SOF:
			writeb(USBIRQ_SOF, acthcd->base + USBIRQ);
#if 0
			{
				u16 index;
				struct aotg_hcep *ep;
	
				index = acthcd->frame;
				ep = acthcd->periodic[index];
				aotg_hcd_period_transfer(acthcd, ep);
	
				acthcd->frame++;
				acthcd->frame = acthcd->frame % PERIODIC_SIZE;
			}
#endif
			break;
		case UIV_USBRESET:
			if (acthcd->port & (USB_PORT_STAT_POWER | USB_PORT_STAT_CONNECTION)) {
				acthcd->speed = USB_SPEED_FULL;	/*FS is the default */
				acthcd->port |= (USB_PORT_STAT_C_RESET << 16);
				acthcd->port &= ~USB_PORT_STAT_RESET;

				/*clear usb reset irq */
				writeb(USBIRQ_URES, acthcd->base + USBIRQ); 
	
				/*reset all ep-in */
				aotg_hcep_reset(acthcd, USB_HCD_IN_MASK,
						ENDPRST_FIFORST | ENDPRST_TOGRST);
				/*reset all ep-out */
				aotg_hcep_reset(acthcd, USB_HCD_OUT_MASK,
						ENDPRST_FIFORST | ENDPRST_TOGRST);
	
				acthcd->port |= USB_PORT_STAT_ENABLE;
				acthcd->rhstate = AOTG_RH_ENABLE;
				/*now root port is enabled fully */
				if (readb(acthcd->base + USBCS) & USBCS_HFMODE) {
					acthcd->speed = USB_SPEED_HIGH;
					acthcd->port |= USB_PORT_STAT_HIGH_SPEED;
					writeb(USBIRQ_HS, acthcd->base + USBIRQ);
					HCD_DEBUG("%s: USB device is  HS\n", __func__);
				} else if (readb(acthcd->base + USBCS) & USBCS_LSMODE) {
					acthcd->speed = USB_SPEED_LOW;
					acthcd->port |= USB_PORT_STAT_LOW_SPEED;
					HCD_DEBUG("%s: USB device is  LS\n", __func__);
				} else {
					acthcd->speed = USB_SPEED_FULL;
					HCD_DEBUG("%s: USB device is  FS\n", __func__);
				}
	
				/*usb_clearbitsb(USBIEN_URES,USBIEN);*/ /*disable reset irq */
				/*khu del for must enable USBIEN_URES again*/
				writew(0xffff, acthcd->base + HCINxERRIRQ0);
				writew(0xffff, acthcd->base + HCOUTxERRIRQ0);

				writew(0xffff, acthcd->base + HCINxIRQ0);
				writew(0xffff, acthcd->base + HCOUTxIRQ0);
	
				writew(0xffff, acthcd->base + HCINxERRIEN0);
				writew(0xffff, acthcd->base + HCOUTxERRIEN0);
	
				HCD_DEBUG("%s: USB reset end\n", __func__);
			}
			break;
	
		case UIV_EP0IN:
			writew(1, acthcd->base + HCOUTxIRQ0);	/*clear hcep0out irq */
			handle_hcep0_out(acthcd);
			break;
		case UIV_EP0OUT:
			writew(1, acthcd->base + HCINxIRQ0);	/*clear hcep0in irq */
			handle_hcep0_in(acthcd);
			break;
		case UIV_EP1IN:
			ACT_HCD_DBG
			writew(1<<1, acthcd->base + HCOUTxIRQ0);	/*clear hcep1out irq */
			break;
		case UIV_EP1OUT:
			ACT_HCD_DBG
			writeb(1<<1, acthcd->base + HCINxIRQ0);	/*clear hcep1in irq */
			break;
		case UIV_EP2IN:
			ACT_HCD_DBG
			writew(1<<2, acthcd->base + HCOUTxIRQ0);	/*clear hcep2out irq */
			break;
		case UIV_EP2OUT:
			ACT_HCD_DBG
			writeb(1<<2, acthcd->base + HCINxIRQ0);	/*clear hcep2in irq */
			break;

		default:
			if ((irqvector >= UIV_HCOUT0ERR) && (irqvector <= UIV_HCOUT15ERR)) {
				printk("irqvector:%d, 0x%x\n", irqvector, irqvector);
				aotg_hcd_err_handle(acthcd, irqvector, (irqvector - UIV_HCOUT0ERR), 0);
				break;
			}
			if ((irqvector >= UIV_HCIN0ERR) && (irqvector <= UIV_HCIN15ERR)) {				
				printk("irqvector:%d, 0x%x\n", irqvector, irqvector);
				aotg_hcd_err_handle(acthcd, irqvector, (irqvector - UIV_HCIN0ERR), 1);
				break;
			}
			dev_err(acthcd->dev, "error interrupt, pls check it! irqvector: 0x%02X\n", (u8)irqvector);
			//spin_unlock(&acthcd->lock);
			return IRQ_NONE;
		}
	}

	//writeb(readb(acthcd->base + 0x518), acthcd->base + 0x518);

	
	aotg_clear_all_overflow_irq(acthcd);
	aotg_clear_all_shortpkt_irq(acthcd);
	aotg_clear_all_zeropkt_irq(acthcd);
	aotg_clear_all_hcoutdma_irq(acthcd);
	aotg_ring_irq_handler(acthcd);

	//ACT_HCD_DBG
	//spin_unlock(&acthcd->lock);
	//printk("%s,(0x%p : 0x%x)\n",__FUNCTION__, acthcd->base + HCINxDMAIRQ0,
	   //readw(acthcd->base + HCINxDMAIRQ0));
	return IRQ_HANDLED;
}

void aotg_hub_hotplug_timer(unsigned long data)
{
	struct aotg_hcd *acthcd = (struct aotg_hcd *)data;
	struct usb_hcd *hcd = aotg_to_hcd(acthcd);
	struct platform_device *pdev;
	unsigned int port_no;
	unsigned long flags;
	int connect_changed = 0;

	//if ((void *)data == (void *)NULL) 
	if (unlikely(IS_ERR_OR_NULL((void *)data))) {
		ACT_HCD_DBG
		return;
	}
	if (acthcd->hcd_exiting != 0) {
		ACT_HCD_DBG
		return;
	}

	//disable_irq_nosync(acthcd->uhc_irq);
	disable_irq(acthcd->uhc_irq);
	spin_lock_irqsave(&acthcd->lock, flags);

	if (acthcd->put_aout_msg != 0) {
		pdev = to_platform_device(hcd->self.controller);
		port_no = pdev->id & 0xff;
		ACT_HCD_DBG
		//update_driver_state(UPDATE_UDEVICE_OUT, port_no);
		acthcd->put_aout_msg = 0;
		spin_unlock_irqrestore(&acthcd->lock, flags);
		enable_irq(acthcd->uhc_irq);
		aotg_hub_notify_hcd_exit(0);
		return;
	}
	
	if ((readb(acthcd->base + OTGSTATE) == AOTG_STATE_A_HOST) && (acthcd->discon_happened == 0)) {
		if (!acthcd->inserted) {
			acthcd->port |= (USB_PORT_STAT_C_CONNECTION << 16);
			/*set port status bit,and indicate the present of  a device */
			acthcd->port |= USB_PORT_STAT_CONNECTION;
			acthcd->rhstate = AOTG_RH_ATTACHED;
			acthcd->inserted = 1;
			connect_changed = 1;
		}
	} else {
		if (acthcd->inserted) {
			acthcd->port &= ~(USB_PORT_STAT_CONNECTION |
					  USB_PORT_STAT_ENABLE |
					  USB_PORT_STAT_LOW_SPEED |
					  USB_PORT_STAT_HIGH_SPEED | USB_PORT_STAT_SUSPEND);
			acthcd->port |= (USB_PORT_STAT_C_CONNECTION << 16);
			acthcd->rhstate = AOTG_RH_NOATTACHED;
			acthcd->inserted = 0;
			connect_changed = 1;
		}
		if (acthcd->discon_happened == 1) {
			acthcd->discon_happened = 0;

			if (readb(acthcd->base + OTGSTATE) == AOTG_STATE_A_HOST) {
				mod_timer(&acthcd->hotplug_timer, jiffies + msecs_to_jiffies(1000));
			}
		}
	}

	dev_info(acthcd->dev, "<USB> %s connection changed: %d, acthcd->inserted: %d\n", 
			dev_name(hcd->self.controller), connect_changed, acthcd->inserted);
	if (connect_changed) {
		if (HC_IS_SUSPENDED(hcd->state)) {
			usb_hcd_resume_root_hub(hcd);
		}
		ACT_HCD_DBG
		//aotg_hcd_flush_queue(acthcd);
		usb_hcd_poll_rh_status(hcd);
	}

	if ((acthcd->inserted == 0) && (connect_changed	== 1) && 
	    (readb(acthcd->base + OTGSTATE) != AOTG_STATE_A_HOST)) {
		acthcd->put_aout_msg = 1;
		mod_timer(&acthcd->hotplug_timer, jiffies + msecs_to_jiffies(2200));
	}
	acthcd->suspend_request_pend = 0;

	spin_unlock_irqrestore(&acthcd->lock, flags);
	enable_irq(acthcd->uhc_irq);
	return;
}

static inline int aotg_print_ep_timeout(struct aotg_hcep *ep)
{
	int ret = 0;

	if (ep == NULL) {
		return ret;
	}
	if (ep->q != NULL) {
		if (ep->q->timeout == 0)
			return ret;

		if (time_after(jiffies, ep->q->timeout)) {
			ret = 1;
			printk("ep->index:%x ep->mask:%x\n", ep->index, ep->mask);
			printk("timeout:0x%x!\n", (unsigned int)ep->q->timeout);
			ep->q->timeout = jiffies + HZ;
		}
	}
	return ret;
}

void aotg_check_trb_timer(unsigned long data)
{
	unsigned long flags;
	struct aotg_hcep *ep;
	int i;
	struct aotg_hcd *acthcd = (struct aotg_hcd *)data;

	if (unlikely(IS_ERR_OR_NULL((void *)data))) {
		ACT_HCD_DBG
		return;
	}
	if (acthcd->hcd_exiting != 0) {
		ACT_HCD_DBG
		return;
	}

	spin_lock_irqsave(&acthcd->lock, flags);
	if (acthcd->check_trb_mutex) {
		mod_timer(&acthcd->check_trb_timer, jiffies + msecs_to_jiffies(1));
		spin_unlock_irqrestore(&acthcd->lock, flags);
		return;
	}

	for (i = 1; i < MAX_EP_NUM; i++) {
		ep = acthcd->inep[i];
		if (ep && (ep->ring) && (ep->ring->type == PIPE_BULK))
				handle_ring_dma_tx(acthcd,i);
	}

	for (i = 1; i < MAX_EP_NUM; i++) {
		ep = acthcd->outep[i];
		if (ep && (ep->ring) && (ep->ring->type == PIPE_BULK))
			handle_ring_dma_tx(acthcd,i | AOTG_DMA_OUT_PREFIX);
	}

	mod_timer(&acthcd->check_trb_timer, jiffies + msecs_to_jiffies(3));

	spin_unlock_irqrestore(&acthcd->lock, flags);
	return;
}

void aotg_hub_trans_wait_timer(unsigned long data)
{
	unsigned long flags;
	struct aotg_hcep *ep;
	int i, ret;
	struct aotg_hcd *acthcd = (struct aotg_hcd *)data;

	if (unlikely(IS_ERR_OR_NULL((void *)data))) {
		ACT_HCD_DBG
		return;
	}
	if (acthcd->hcd_exiting != 0) {
		ACT_HCD_DBG
		return;
	}

	//disable_irq_nosync(acthcd->uhc_irq);
	disable_irq(acthcd->uhc_irq);
	spin_lock_irqsave(&acthcd->lock, flags);

	ep = acthcd->active_ep0;
	ret = aotg_print_ep_timeout(ep);

	for (i = 1; i < MAX_EP_NUM; i++) {
		ep = acthcd->inep[i];
		ret |= aotg_print_ep_timeout(ep);
	}
	for (i = 1; i < MAX_EP_NUM; i++) {
		ep = acthcd->outep[i];
		if (ep == NULL) {
			continue;
		}
		ret |= aotg_print_ep_timeout(ep);

		if (ep->fifo_busy) {
			if ((ep->fifo_busy > 80) && (ep->fifo_busy % 80 == 0))  {
				printk("ep->fifo_busy:%d\n", ep->fifo_busy);
			}
			if (ret == 0) {
				tasklet_hi_schedule(&acthcd->urb_tasklet);
				break;
			}
		}
	}

	if (ret != 0) {
		tasklet_hi_schedule(&acthcd->urb_tasklet);
	}
	mod_timer(&acthcd->trans_wait_timer, jiffies + msecs_to_jiffies(500));

	spin_unlock_irqrestore(&acthcd->lock, flags);
	enable_irq(acthcd->uhc_irq);
	return;
}
//FIXME
static inline int start_transfer(struct aotg_hcd *acthcd, struct aotg_queue *q, struct aotg_hcep *ep)
{
	struct urb *urb = q->urb;
	int retval = 0;

	ep->urb_enque_cnt++;
	q->length = urb->transfer_buffer_length;

	/* do with hub connected. */
	if (ep->has_hub) {
		if (urb->dev->speed == USB_SPEED_HIGH) {
			writeb(usb_pipedevice(urb->pipe), ep->reg_hcep_dev_addr);
			writeb(urb->dev->portnum, ep->reg_hcep_port);
		} else {
			writeb((0x80 | usb_pipedevice(urb->pipe)), ep->reg_hcep_dev_addr);
			if (urb->dev->speed == USB_SPEED_LOW) {
				writeb(0x80 | urb->dev->portnum, ep->reg_hcep_port);
			} else {
				writeb(urb->dev->portnum, ep->reg_hcep_port);
			}
		}
		//writeb(0, ep->reg_hcep_splitcs);
	} else {
		writeb(usb_pipedevice(urb->pipe), ep->reg_hcep_dev_addr);
		writeb(urb->dev->portnum, ep->reg_hcep_port);
	}

	switch (usb_pipetype(urb->pipe)) {
	case PIPE_CONTROL:
		q->timeout = jiffies + HZ/2;
		retval = handle_setup_packet(acthcd, q);
		break;

	default:
		printk(KERN_ERR"%s err, check it pls!\n", __FUNCTION__);
	}

	return retval;
}

#if 0
#define USB_CLASS_PER_INTERFACE		0	/* for DeviceClass */
#define USB_CLASS_AUDIO			1
#define USB_CLASS_COMM			2
#define USB_CLASS_HID			3
#define USB_CLASS_PHYSICAL		5
#define USB_CLASS_STILL_IMAGE		6
#define USB_CLASS_PRINTER		7
#define USB_CLASS_MASS_STORAGE		8
#define USB_CLASS_HUB			9
#define USB_CLASS_CDC_DATA		0x0a
#define USB_CLASS_CSCID			0x0b	/* chip+ smart card */
#define USB_CLASS_CONTENT_SEC		0x0d	/* content security */
#define USB_CLASS_VIDEO			0x0e
#define USB_CLASS_WIRELESS_CONTROLLER	0xe0
#define USB_CLASS_MISC			0xef
#define USB_CLASS_APP_SPEC		0xfe
#define USB_CLASS_VENDOR_SPEC		0xff
#endif
/*
static void aotg_hcd_dump_isoc_packet(struct urb *urb)
{
	int i;
	int number_of_packets;
	u32 start_addr, addr;
	unsigned int len;
	
	number_of_packets = urb->number_of_packets;
	start_addr = (u32)urb->transfer_dma;

	printk("----dump iso_packets( addr--len )----\n");
	for (i = 0; i < number_of_packets; i++) {
		addr = start_addr + urb->iso_frame_desc[i].offset;
		len = urb->iso_frame_desc[i].length;
		printk("packet%d : %u, %d\n", i, addr, len);
	}
	return;
}
*/
static struct aotg_hcep	*aotg_hcep_alloc(struct usb_hcd *hcd, struct urb *urb)
{
	struct aotg_hcep *ep = NULL;
	int pipe = urb->pipe;
	int is_out = usb_pipeout(pipe);
	int type = usb_pipetype(pipe);
	int i, retval = 0;
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	u8 think_time;

	ep = kzalloc(sizeof	*ep, GFP_ATOMIC);
	if (NULL == ep)	{
		dev_err(acthcd->dev, "alloc	ep failed\n");
		retval = -ENOMEM;
		goto exit;
	}

	ep->udev = usb_get_dev(urb->dev);
	ep->epnum = usb_pipeendpoint(pipe);
	ep->maxpacket = usb_maxpacket(ep->udev, urb->pipe, is_out);
	ep->type = type;
	ep->urb_enque_cnt = 0;
	ep->urb_endque_cnt = 0;
	ep->urb_stop_stran_cnt = 0;
	ep->urb_unlinked_cnt = 0;
#ifdef USBH_DEBUG
	dev_info(acthcd->dev, "ep->epnum: %d, ep->maxpacket : %d, ep->type : %d\n", ep->epnum, ep->maxpacket, ep->type);
#endif
	ep->length = 0;
	usb_settoggle(urb->dev, usb_pipeendpoint(urb->pipe), is_out, 0);

	if (urb->dev->parent) {
		if (urb->dev->tt) {
			/* calculate in ns. */
			think_time = (urb->dev->tt->think_time / 666);
			printk("think_time:%d\n", think_time);
			if (think_time <= 0) {
				think_time = 1;
			} else if (think_time > 4) {
				think_time = 4;
			}
			think_time = think_time * 20;
			writeb(think_time, acthcd->base + HCTRAINTERVAL);
			printk("think_time:0x%x\n", readb(acthcd->base + HCTRAINTERVAL));
			//printk("urb->dev->tt->hub:%p \n", urb->dev->tt->hub);
		}

		if ((urb->dev->parent->parent) && (urb->dev->parent != hcd->self.root_hub)) {
			ep->has_hub = 1;
			ep->hub_addr = 0x7f & readb(acthcd->base + FNADDR);
		} else {
			ep->has_hub = 0;
		}
	}

	switch (type) {
	case PIPE_CONTROL:
		ep->reg_hcep_dev_addr = acthcd->base + HCEP0ADDR;
		ep->reg_hcep_port = acthcd->base + HCEP0PORT;
		ep->reg_hcep_splitcs = acthcd->base + HCEP0SPILITCS;

		for (i = 0; i < MAX_EP_NUM; i++) {
			if (acthcd->ep0[i] == NULL) {
				ep->ep0_index = i;
				acthcd->ep0[i] = ep;
				break;
			}
		}
		if (i == MAX_EP_NUM) {
			ACT_HCD_ERR
		}

		ep->index = 0;
		ep->mask = 0;
		usb_settoggle(urb->dev, usb_pipeendpoint(urb->pipe), 1, 0);
		usb_settoggle(urb->dev, usb_pipeendpoint(urb->pipe), 0, 0);

		if (acthcd->active_ep0 == NULL) {
			//writeb(ep->epnum, acthcd->base + HCEP0CTRL);
			//writeb((u8)ep->maxpacket, acthcd->base + HCIN0MAXPCK);
			//writeb((u8)ep->maxpacket, acthcd->base + HCOUT0MAXPCK);
			usb_setbitsw(1, acthcd->base + HCOUTxIEN0);
			usb_setbitsw(1, acthcd->base + HCINxIEN0);
			writew(1, acthcd->base + HCOUTxIRQ0);
			writew(1, acthcd->base + HCINxIRQ0);

			if (ep->has_hub) {
				usb_setbitsb(0x80, acthcd->base + FNADDR);
			} else {
				writeb(usb_pipedevice(urb->pipe), acthcd->base + FNADDR);
			}
			dev_info(acthcd->dev, "device addr : 0x%08x\n", readb(acthcd->base + FNADDR));
		} else {
			ACT_HCD_ERR
		}
		break;

	case PIPE_BULK:
		retval = aotg_hcep_config(acthcd, ep, EPCON_TYPE_BULK, EPCON_BUF_SINGLE, is_out);
		if (retval < 0) {
			dev_err(acthcd->dev, "PIPE_BULK, retval: %d\n", retval);
			kfree(ep);
			goto exit;
		}
		break;

	case PIPE_INTERRUPT:
		retval = aotg_hcep_config(acthcd, ep, EPCON_TYPE_INT, EPCON_BUF_SINGLE, is_out);
		if (retval < 0) {
			dev_err(acthcd->dev, "PIPE_INTERRUPT, retval: %d\n", retval);
			kfree(ep);
			goto exit;
		}
		ep->interval= urb->ep->desc.bInterval;
		writeb(ep->interval, ep->reg_hcep_interval);		
		//printk("urb->interval: %d\n", urb->interval);
		//printk("urb->ep->desc.bInterval: %d, reg_interval:0x%x\n", 
		//		urb->ep->desc.bInterval, readb(ep->reg_hcep_interval));
		
		break;

	case PIPE_ISOCHRONOUS:
		retval = aotg_hcep_config(acthcd, ep, EPCON_TYPE_ISO, EPCON_BUF_SINGLE, is_out);
		ep->iso_packets = (urb->ep->desc.wMaxPacketSize >> 11) & 3;
		ep->interval = urb->ep->desc.bInterval;
		writeb(ep->interval, ep->reg_hcep_interval);
		usb_setb(ep->iso_packets << 4, ep->reg_hcepcon);
		printk("iso_packets:%d, bInterval:%d, urb_interval:%d, reg_con:0x%x\n",
					ep->iso_packets, ep->interval, urb->interval, readb(ep->reg_hcepcon));				
		break;

	default:
		dev_err(acthcd->dev, "not support type, type: %d\n", type);
		retval = -ENODEV;
		kfree(ep);
		goto exit;
	}

	if ((ep->udev->speed != USB_SPEED_HIGH) && ep->has_hub && (type == PIPE_INTERRUPT)) {
		aotg_hcep_set_split_micro_frame(acthcd, ep);
	}
	ep->hep = urb->ep;
	urb->ep->hcpriv = ep;
	return ep;

exit:
	return NULL;
}

static int aotg_hub_urb_enqueue(struct usb_hcd *hcd, struct urb *urb, unsigned mem_flags)
{
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	struct aotg_queue *q=NULL;
	unsigned long flags;
	struct aotg_hcep *ep = NULL;
	struct aotg_td *td, *next; 
	int pipe = urb->pipe;
	int type = usb_pipetype(pipe);
	int retval = 0;
	
	if ((acthcd == NULL) || (act_hcd_ptr[acthcd->id] == NULL)) {
		printk("aotg_hcd device had been removed...\n");
		return -EIO;
	}

	if (acthcd->hcd_exiting != 0) {
		dev_dbg(acthcd->dev, "aotg hcd exiting! type:%d\n", type);
		return -ENODEV;
	}

	if (!(acthcd->port & USB_PORT_STAT_ENABLE)
		|| (acthcd->port & (USB_PORT_STAT_C_CONNECTION << 16))
		|| (acthcd->hcd_exiting != 0)
		|| (acthcd->inserted == 0)
	    	|| !HC_IS_RUNNING(hcd->state)) {
		dev_err(acthcd->dev, "usbport dead or disable\n");
		return -ENODEV;
	}

	spin_lock_irqsave(&acthcd->lock, flags);
	
	ep = urb->ep->hcpriv;
	if ((unlikely(!urb->ep->enabled)) || (likely(ep) && unlikely(ep->error_count > 3))) {
		printk("ep had been stopped!\n");
		//spin_unlock_irqrestore(&acthcd->lock, flags);
		//ep = (struct aotg_hcep *)urb->ep->hcpriv;
		retval = -ENOENT;
		goto exit0;
	}
	retval = usb_hcd_link_urb_to_ep(hcd, urb);
	if (retval) {
		dev_err(acthcd->dev, "<QUEUE>  usb_hcd_link_urb_to_ep error!! retval:0x%x\n",retval);
		goto exit0;
	}

	if (likely(urb->ep->hcpriv)) {
		ep = (struct aotg_hcep *)urb->ep->hcpriv;
	} else {
		ep = aotg_hcep_alloc(hcd, urb);
		if (NULL == ep) {
			dev_err(acthcd->dev, "<QUEUE> alloc ep failed\n");
			retval = -ENOMEM;
			goto exit1;
		}
		if (!usb_pipecontrol(pipe)) {
			if (usb_pipeint(pipe)) 
				ep->ring = aotg_alloc_ring(acthcd, ep, INTR_TRBS, GFP_ATOMIC);
			else
				ep->ring = aotg_alloc_ring(acthcd, ep, NUM_TRBS, GFP_ATOMIC);							
			if (!ep->ring) {
				dev_err(acthcd->dev, "alloc td_ring failed\n");
				retval = -ENOMEM;
				goto exit1;
			}
			INIT_LIST_HEAD(&ep->queue_td_list);
			INIT_LIST_HEAD(&ep->enring_td_list);
			INIT_LIST_HEAD(&ep->dering_td_list);

			//enable_overflow_irq(acthcd, ep);
		}
		urb->ep->hcpriv	= ep;
		if (type == PIPE_BULK)
			mod_timer(&acthcd->check_trb_timer, jiffies + msecs_to_jiffies(100));
	}

	urb->hcpriv = hcd;

	if (type == PIPE_CONTROL) {
		q = aotg_hcd_get_queue(acthcd, urb, mem_flags);
		if (unlikely(!q)) {
			dev_err(acthcd->dev, "<QUEUE>  alloc dma queue failed\n");
			spin_unlock_irqrestore(&acthcd->lock, flags);
			return -ENOMEM;
		}		

		q->ep = ep;
		q->urb = urb;
		list_add_tail(&q->enqueue_list, &acthcd->hcd_enqueue_list);
		aotg_dbg_put_q(q, usb_pipeendpoint(q->urb->pipe), usb_pipein(q->urb->pipe), 
			q->urb->transfer_buffer_length);
	} else if (type == PIPE_BULK) {
		td = aotg_alloc_td(mem_flags);
		if (!td) {
			dev_err(acthcd->dev, "alloc td failed\n");
			retval = -ENOMEM;
			goto exit1;
		}
		td->urb = urb;

		ep->urb_enque_cnt++;

		if (list_empty(&ep->queue_td_list)) {
			//ACT_HCD_DBG
			retval = aotg_ring_enqueue_td(acthcd, ep->ring, td);
			if (retval) {
				list_add_tail(&td->queue_list, &ep->queue_td_list);
				goto out;
			} 

			list_add_tail(&td->enring_list, &ep->enring_td_list);
			ep->ring->enring_cnt++;
		} else {
			//ACT_HCD_DBG
			list_add_tail(&td->queue_list, &ep->queue_td_list);
		}

		if (!list_empty(&ep->enring_td_list) && 
								!is_ring_running(ep->ring)) {
			//ACT_HCD_DBG
			aotg_start_ring_transfer(acthcd, ep, urb);
		}
		
	} else if (type == PIPE_INTERRUPT) {
		if (unlikely(ep->ring->intr_inited == 0)) {
			retval = aotg_ring_enqueue_intr_td(acthcd, ep->ring, ep, urb, GFP_ATOMIC);
			if (retval) {
				printk("%s, intr urb enqueue err!\n", __FUNCTION__);
				goto exit1;
			}
			ep->ring->intr_started = 0;
		}
		ep->urb_enque_cnt++;
		list_for_each_entry_safe(td, next, &ep->enring_td_list, enring_list) {
			if (td->urb) {
				continue;
			} else {
				td->urb = urb;
				break;
			}			
		}
		
		if (unlikely(ep->ring->enqueue_trb->hw_buf_len != urb->transfer_buffer_length)) {
			//printk("ep:%p,hw_buf_len:%d, urb_len:%d .......\n",ep,ep->ring->enqueue_trb->hw_buf_len,urb->transfer_buffer_length);
			aotg_intr_chg_buf_len(acthcd,ep->ring,urb->transfer_buffer_length);
			printk("WARNNING:interrupt urb length changed......\n");
		}

		if (ep->ring->intr_started == 0) {
			ep->ring->intr_started = 1;			
			//printk("%s, start ep%d intr transfer\n", __FUNCTION__, ep->index);
			aotg_start_ring_transfer(acthcd, ep, urb);
		}
		
		if (!is_ring_running(ep->ring)) { /*trb overflow or no urb*/
			if (ep->is_out) {
				aotg_start_ring_transfer(acthcd, ep, urb);
			} else {
				//if (ep->ring->ring_stopped == 0) {
				if (aotg_intr_get_finish_trb(ep->ring) == 0) {
					ep->ring->ring_stopped = 0;
					aotg_reorder_intr_td(ep);
					ep_enable(ep);
					mb();
					writel(DMACTRL_DMACS,ep->ring->reg_dmactrl);
				//	printk("restart intr!\n");
				} else ep->ring->ring_stopped = 1;
			}
		}
	} else {// type == PIPE_ISOCHRONOUS
		td = aotg_alloc_td(mem_flags);
		if (!td) {
			dev_err(acthcd->dev, "alloc td failed\n");
			retval = -ENOMEM;
			goto exit1;
		}
		td->urb = urb;
		
		ep->urb_enque_cnt++;

		if (list_empty(&ep->queue_td_list)) {
			//ACT_HCD_DBG
			retval = aotg_ring_enqueue_isoc_td(acthcd, ep->ring, td);
			if (retval) {
				list_add_tail(&td->queue_list, &ep->queue_td_list);
				goto out;
			} 

			list_add_tail(&td->enring_list, &ep->enring_td_list);
			ep->ring->enring_cnt++;
		} else {
			//ACT_HCD_DBG
			list_add_tail(&td->queue_list, &ep->queue_td_list);
		}

		if (!list_empty(&ep->enring_td_list) && !is_ring_running(ep->ring)) {
			if (ep->ring->dequeue_trb != ep->ring->first_trb)
				aotg_reorder_iso_td(acthcd, ep->ring);
			aotg_start_ring_transfer(acthcd, ep, urb);
		}
	}
out:
	spin_unlock_irqrestore(&acthcd->lock, flags);
	tasklet_hi_schedule(&acthcd->urb_tasklet);
	return retval;
exit1:
	usb_hcd_unlink_urb_from_ep(hcd, urb);
exit0:
	/* FIXME */
	printk("never goto here, need to just\n");
	if (unlikely(retval < 0) && ep) {
		if (type == PIPE_CONTROL)	{
			ACT_HCD_ERR
			if (ep)
				ep->q = NULL;
			if(q)
				aotg_hcd_release_queue(acthcd, q);
		} else {
			writel(DMACTRL_DMACC,ep->ring->reg_dmactrl);
			ep_disable(ep);
			/*if (!list_empty(&ep->queue_td_list)) {
				list_for_each_entry_safe(td, next, &ep->queue_td_list, queue_list) {
					list_del(&td->queue_list);
					if (td)
						kfree(td);
				}
			}
		
			if (!list_empty(&ep->enring_td_list)) {
				list_for_each_entry_safe(td, next, &ep->enring_td_list, enring_list) {
					list_del(&td->enring_list);
					if (td)
						kfree(td);
				}
			}*/
		}
	}
	spin_unlock_irqrestore(&acthcd->lock, flags);
	return retval;
}

static int aotg_hub_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status)
{
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	struct aotg_hcep *ep;
	struct aotg_queue *q = NULL, *next, *tmp;
	struct aotg_ring *ring;
	struct aotg_td *td, *next_td;
	unsigned long flags;
	int retval = 0;
	
	if ((acthcd == NULL) || (act_hcd_ptr[acthcd->id] == NULL)) {
		printk("aotg_hcd device had been removed...\n");
		return -EIO;
	}

	spin_lock_irqsave(&acthcd->lock, flags);

	retval = usb_hcd_check_unlink_urb(hcd, urb, status);
	if (retval) {
		printk("%s, retval:%d, urb not submitted or unlinked\n", __FUNCTION__, 
				retval);
		goto dequeue_out;
	}

	ep = (struct aotg_hcep *)urb->ep->hcpriv;
	if (ep == NULL) {
		ACT_HCD_ERR
		retval = -EINVAL;
		goto dequeue_out;
	}

	if (!usb_pipecontrol(urb->pipe)) {
//		ACT_HCD_DBG
		ep->urb_unlinked_cnt++;
		ring = ep->ring;
		
		if (usb_pipeint(urb->pipe)) {
			list_for_each_entry_safe(td, next_td, &ep->enring_td_list, enring_list) {
				if (urb == td->urb) {
					retval = aotg_ring_dequeue_intr_td(acthcd, ep, ring, td);
					goto de_bulk;				
				}
			}
			printk("%s, intr dequeue err\n", __FUNCTION__);
		}

		list_for_each_entry_safe(td, next_td, &ep->queue_td_list, queue_list) {
			if (urb == td->urb) {
				retval = aotg_ring_dequeue_td(acthcd, ring, td, TD_IN_QUEUE);
				goto de_bulk;
			}
		}

		list_for_each_entry_safe(td, next_td, &ep->enring_td_list, enring_list) {
			mb();
			if (urb == td->urb) {
				retval = aotg_ring_dequeue_td(acthcd, ring, td, TD_IN_RING);
				ep->urb_stop_stran_cnt++;
				goto de_bulk;
			} 
		}

		//pr_err("%s err, never be here, pls check it!\n", __func__);		
		retval = -EINVAL;
		goto dequeue_out;
de_bulk:
		usb_hcd_unlink_urb_from_ep(hcd, urb);
		spin_unlock(&acthcd->lock);
		usb_hcd_giveback_urb(hcd, urb, status);
		spin_lock(&acthcd->lock);

		spin_unlock_irqrestore(&acthcd->lock, flags);
		return retval;
	}
	
	q = ep->q;

	/* ep->mask currently equal to q->dma_no. */
	if (q && (q->urb == urb)) {
		writeb(EP0CS_HCSET,acthcd->base + EP0CS);
		//dev_info(acthcd->dev, 
			//"current dequeue -- ep->index: %d, dir : %s, type: %d, transfer_buffer_length: %d, actual_length:%d\n",
			//ep->index, usb_pipeout(urb->pipe)?"out":"in", usb_pipetype(urb->pipe), 
			//urb->transfer_buffer_length, urb->actual_length);

		/* maybe finished in tasklet_finish_request. */
		if (!list_empty(&q->finished_list)) {
			if (q->finished_list.next != LIST_POISON1) {
				list_del(&q->finished_list);
			}
		}

		if (q->is_xfer_start) {
			ep->urb_stop_stran_cnt++;
			q->is_xfer_start = 0;
		}
	} else {
		q = NULL;
		list_for_each_entry_safe(tmp, next, &acthcd->hcd_enqueue_list, enqueue_list) {
			if (tmp->urb == urb) {
				list_del(&tmp->enqueue_list);
				q = tmp;
				ep = q->ep;
				if (ep->q == q) {
					ACT_HCD_DBG
				}
				break;
			}
		}
	}

	if (likely(q)) {
		//ACT_HCD_DBG
		//if (ep->q == q) 
			//ep->q = NULL;
		q->status = status;
		list_add_tail(&q->dequeue_list, &acthcd->hcd_dequeue_list);
		spin_unlock_irqrestore(&acthcd->lock, flags);
		tasklet_schedule(&acthcd->urb_tasklet);
		return retval;
	} else {
		ACT_HCD_ERR
		printk("dequeue's urb not find in enqueue_list!\n");
	}	

dequeue_out:
	spin_unlock_irqrestore(&acthcd->lock, flags);
	return retval;
}

void urb_tasklet_func(unsigned long data)
{
	struct aotg_hcd *acthcd = (struct aotg_hcd *)data;
	struct aotg_queue *q, *next;
	struct aotg_hcep *ep;
	struct urb *urb;
	struct aotg_ring *ring;
	struct aotg_td *td;
	unsigned long flags;
	int status;
	struct usb_hcd *hcd = aotg_to_hcd(acthcd);
	int cnt = 0;
	//static struct aotg_hcep * hcin_ep = NULL;

	//spin_lock(&acthcd->tasklet_lock);

	do {
		status = (int)spin_is_locked(&acthcd->tasklet_lock);
		if (status) {
			acthcd->tasklet_retry = 1;
			printk("locked, urb retry later!\n");
			return;
		}
		cnt++;
		/* sometimes tasklet_lock is unlocked, but spin_trylock still will be failed, 
		 * maybe caused by the instruction of strexeq in spin_trylock, it will return failed   
		 * if other cpu is accessing the nearby address of &acthcd->tasklet_lock.
		 */
		status = spin_trylock(&acthcd->tasklet_lock);
		if ((!status) && (cnt > 10))  {
			acthcd->tasklet_retry = 1;
			printk("urb retry later!\n");
			return;
		}
	} while (status == 0);

	disable_irq(acthcd->uhc_irq);
	spin_lock_irqsave(&acthcd->lock, flags);

	for (cnt=1; cnt<MAX_EP_NUM; cnt++) {
		ep = acthcd->inep[cnt];
		if (ep && (ep->type == PIPE_INTERRUPT)) {
			ring = ep->ring;
			if (ring->ring_stopped) {
				td = list_first_entry_or_null(&ep->enring_td_list, struct aotg_td, enring_list);
				if (!td)
					continue;
				urb = td->urb;
				if (!urb)
					continue;
				intr_finish_td(acthcd, ring, td);
			}
		}
	}
	/* do dequeue task. */
DO_DEQUEUE_TASK:
	urb = NULL;
	list_for_each_entry_safe(q, next, &acthcd->hcd_dequeue_list, dequeue_list) {
		if (q->status < 0) {
			urb = q->urb;
			ep = q->ep;
			if (ep) {
				ep->urb_unlinked_cnt++;
				//ep->q = NULL;
			}
			list_del(&q->dequeue_list);
			status = q->status;
			tasklet_finish_request(acthcd, q, status);
			hcd = bus_to_hcd(urb->dev->bus);
			break;
		} else {
			ACT_HCD_ERR
		}
	}
	if (urb != NULL) {
		usb_hcd_unlink_urb_from_ep(hcd, urb);
		spin_unlock_irqrestore(&acthcd->lock, flags);
		/* in usb_hcd_giveback_urb, complete function may call new urb_enqueue. */ 
		usb_hcd_giveback_urb(hcd, urb, status);
		spin_lock_irqsave(&acthcd->lock, flags);
		goto DO_DEQUEUE_TASK;
	}

	/* do finished task. */
DO_FINISH_TASK:
	urb = NULL;
	list_for_each_entry_safe(q, next, &acthcd->hcd_finished_list, finished_list) {
		if (q->finished_list.next != LIST_POISON1) {
			list_del(&q->finished_list);
		} else {
			ACT_HCD_ERR
			break;
		}
		status = q->status;
		tasklet_finish_request(acthcd, q, status);

		hcd = aotg_to_hcd(acthcd);
		urb = q->urb;
		ep = q->ep;
		if (urb != NULL) {
			break;
		}
	}
	if (urb != NULL) {
		usb_hcd_unlink_urb_from_ep(hcd, urb);

		spin_unlock_irqrestore(&acthcd->lock, flags);

		/* in usb_hcd_giveback_urb, complete function may call new urb_enqueue. */ 
		usb_hcd_giveback_urb(hcd, urb, status);

		spin_lock_irqsave(&acthcd->lock, flags);
		goto DO_FINISH_TASK;
	}

//DO_ENQUEUE_TASK:
	/* do enqueue task. */
	/* start transfer directly, don't care setup appearing in bulkout. */
	list_for_each_entry_safe(q, next, &acthcd->hcd_enqueue_list, enqueue_list) {
		urb = q->urb;
		ep = q->ep;

		/* deal with controll request. */
		if (usb_pipetype(urb->pipe) == PIPE_CONTROL) {
			if ((acthcd->active_ep0 != NULL) && (acthcd->active_ep0->q != NULL)) {
				acthcd->ep0_block_cnt++;
				//ACT_HCD_DBG
				if ((acthcd->ep0_block_cnt % 10) == 0) {
					ACT_HCD_DBG
					printk("cnt:%d\n", acthcd->ep0_block_cnt);
					acthcd->ep0_block_cnt = 0;
					//aotg_hub_urb_dequeue(hcd, acthcd->active_ep0->q->urb, -ETIMEDOUT);
				}
				continue;
			} else {
				acthcd->ep0_block_cnt = 0;
				goto BEGIN_START_TANSFER;
			}
		} 

		/* deal with new bulk in request. */
		if ((usb_pipetype(urb->pipe) == PIPE_BULK) && (usb_pipein(urb->pipe))) {
			if (ep->q != NULL) {
				continue;
			}
			goto BEGIN_START_TANSFER;
		}

		/* deal with bulk out request. */
		if ((usb_pipetype(urb->pipe) == PIPE_BULK) && (usb_pipeout(urb->pipe))) {
			if (ep->q != NULL) {
				continue;
			}
			if ((EPCS_BUSY & readb(ep->reg_hcepcs)) != 0) {
				ep->fifo_busy++;
				mod_timer(&acthcd->trans_wait_timer, jiffies + msecs_to_jiffies(3));
				continue;
			} else {
				ep->fifo_busy = 0;
			}
			goto BEGIN_START_TANSFER;
		}

BEGIN_START_TANSFER:
		list_del(&q->enqueue_list);
		status = start_transfer(acthcd, q, ep);

		if (unlikely(status < 0)) {
			ACT_HCD_ERR
			hcd = bus_to_hcd(urb->dev->bus);
			aotg_hcd_release_queue(acthcd, q);

			usb_hcd_unlink_urb_from_ep(hcd, urb);
			spin_unlock_irqrestore(&acthcd->lock, flags);
			usb_hcd_giveback_urb(hcd, urb, status);
			spin_lock_irqsave(&acthcd->lock, flags);
		}
		//spin_unlock_irqrestore(&acthcd->lock, flags);
		//enable_irq(acthcd->uhc_irq);
		//spin_unlock(&acthcd->tasklet_lock);
		//return;
	}

	if (acthcd->tasklet_retry != 0) {
		acthcd->tasklet_retry = 0;
		goto DO_DEQUEUE_TASK;
	}
	spin_unlock_irqrestore(&acthcd->lock, flags);
	enable_irq(acthcd->uhc_irq);
	spin_unlock(&acthcd->tasklet_lock);
	return;
}

static void aotg_hub_endpoint_disable(struct usb_hcd *hcd, struct usb_host_endpoint *hep)
{
	int i;
	int index;
	unsigned long flags;
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	struct aotg_hcep *ep = hep->hcpriv;
	
	if (!ep)
		return;

	/* assume we'd just wait for the irq */
//	for (i = 0; i < 100 && !list_empty(&hep->urb_list); i++)
//		msleep(3);

	if (in_irq()) {
		disable_irq_nosync(acthcd->uhc_irq);
	} else {
		disable_irq(acthcd->uhc_irq);
	}
	spin_lock_irqsave(&acthcd->lock, flags);

//	usb_put_dev(ep->udev);
	index = ep->index;
	if (index == 0) {
		acthcd->ep0[ep->ep0_index] = NULL;
		if (acthcd->active_ep0 == ep) {
			acthcd->active_ep0 = NULL;
		}
		for (i = 0; i < MAX_EP_NUM; i++) {
			if (acthcd->ep0[i] != NULL) {
				break;
			}
		}
		if (i == MAX_EP_NUM) {
			usb_clearbitsw(1, acthcd->base + HCOUTxIEN0);
			usb_clearbitsw(1, acthcd->base + HCINxIEN0);
			writew(1, acthcd->base + HCOUTxIRQ0);
			writew(1, acthcd->base + HCINxIRQ0);
		}
	} else {
		ep_disable(ep);
		if (ep->mask & USB_HCD_OUT_MASK) {
			acthcd->outep[index] = NULL;
		} else {
			acthcd->inep[index] = NULL;
		}
//		pio_irq_disable(acthcd, ep->mask);
//		pio_irq_clear(acthcd, ep->mask);
		release_fifo_addr(acthcd, ep->fifo_addr);
	}
	
	hep->hcpriv = NULL;

	if(ep->ring){
		printk("%s\n", __FUNCTION__);
		
		aotg_stop_ring(ep->ring);
		if (ep->ring->type == PIPE_INTERRUPT) {
			printk("%s, ep%d dma buf free\n", __FUNCTION__, ep->index);
			aotg_intr_dma_buf_free(acthcd, ep->ring);
		}
			
		aotg_free_ring(acthcd, ep->ring);
	}
		
	dev_info(acthcd->dev, "<EP DISABLE> ep%d index %d from ep [%s]\n", 
			ep->epnum, index, 
			ep->mask & USB_HCD_OUT_MASK ? "out":"in");

	spin_unlock_irqrestore(&acthcd->lock, flags);
	enable_irq(acthcd->uhc_irq);
	kfree(ep);
	return;
}

static int aotg_hcd_get_frame(struct usb_hcd *hcd)
{
	struct timeval	tv;

	do_gettimeofday(&tv);
	return tv.tv_usec / 1000;
}

static int aotg_hub_status_data(struct usb_hcd *hcd, char *buf)
{
	struct aotg_hcd *acthcd;
	unsigned long flags;
	int retval = 0;

	acthcd = hcd_to_aotg(hcd);
	local_irq_save(flags);
	if (!HC_IS_RUNNING(hcd->state))
		goto done;

	if ((acthcd->port & AOTG_PORT_C_MASK) != 0) {
		*buf = (1 << 1);
		HUB_DEBUG("<HUB STATUS>port status %08x has changes\n", acthcd->port);
		retval = 1;
	}
done:
	local_irq_restore(flags);
	return retval;
}

static __inline__ void port_reset(struct aotg_hcd *acthcd)
{
	HCD_DEBUG("<USB> port reset\n");
	writeb(0x1<<6 | 0x1<<5, acthcd->base + HCPORTCTRL);	/*portrst & 55ms */
}

static void port_power(struct aotg_hcd *acthcd, int is_on)
{
	struct usb_hcd *hcd = aotg_to_hcd(acthcd);
	//struct device *dev = hcd->self.controller;

	/* hub is inactive unless the port is powered */
	if (is_on) {
		hcd->self.controller->power.power_state = PMSG_ON;
		dev_dbg(acthcd->dev, "<USB> power on\n");
	} else {
		hcd->self.controller->power.power_state = PMSG_SUSPEND;
		dev_dbg(acthcd->dev, "<USB> power off\n");
	}
}

static void port_suspend(struct aotg_hcd *acthcd)
{
	usb_clearbitsb(OTGCTRL_BUSREQ, acthcd->base + OTGCTRL);
}

static void port_resume(struct aotg_hcd *acthcd)
{
	usb_setbitsb(OTGCTRL_BUSREQ, acthcd->base + OTGCTRL);
}

static int aotg_hcd_start(struct usb_hcd *hcd)
{
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	//struct device *dev = hcd->self.controller;
	//struct aotg_plat_data *data = dev->platform_data;
	int retval = 0;
	unsigned long flags;
	
	dev_info(acthcd->dev, "<HCD> start\n");
	
	local_irq_save(flags);
	hcd->state = HC_STATE_RUNNING;
	hcd->uses_new_polling = 1;
	local_irq_restore(flags);
	
	return retval;

}

static void aotg_hcd_stop(struct usb_hcd *hcd)
{
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	//struct device *dev = hcd->self.controller;
	//struct aotg_plat_data *data = dev->platform_data;
	unsigned long flags;
	
	dev_info(acthcd->dev, "<HCD> stop\n");
	
	local_irq_save(flags);
	hcd->state = HC_STATE_HALT;
	acthcd->port = 0;
	acthcd->rhstate = AOTG_RH_POWEROFF;
	local_irq_restore(flags);
	return;
}

#ifdef	CONFIG_PM

static int aotg_hub_suspend(struct usb_hcd *hcd)
{
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);

	if ((hcd == NULL) || (acthcd == NULL)) {
		ACT_HCD_ERR
		return 0;
	}
	acthcd->suspend_request_pend = 1;
	port_suspend(acthcd);
	
	return 0;
}

static int
aotg_hub_resume(struct usb_hcd *hcd)
{
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	
	port_resume(acthcd);

	return 0;
}

#else

#define	aotg_hub_suspend	NULL
#define	aotg_hub_resume		NULL

#endif

static __inline__ void aotg_hub_descriptor(struct usb_hub_descriptor *desc)
{
	memset(desc, 0, sizeof *desc);
	desc->bDescriptorType = 0x29;
	desc->bDescLength = 9;
	desc->wHubCharacteristics = (__force __u16)
	    (__constant_cpu_to_le16(0x0001));
	desc->bNbrPorts = 1;
	//desc->bitmap[0] = 1 << 1;
	//desc->bitmap[1] = 0xff;
}

static int aotg_hub_control(struct usb_hcd *hcd,
			    u16 typeReq,
			    u16 wValue, u16 wIndex, char *buf, u16 wLength)
{
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	unsigned long flags;
	int retval = 0;

	if (in_irq()) {
		disable_irq_nosync(acthcd->uhc_irq);
	} else {
		disable_irq(acthcd->uhc_irq);
	}
	spin_lock_irqsave(&acthcd->lock, flags);

	if (!HC_IS_RUNNING(hcd->state)) {
		dev_err(acthcd->dev, "<HUB_CONTROL> hc state is not HC_STATE_RUNNING\n");
		spin_unlock_irqrestore(&acthcd->lock, flags);
		enable_irq(acthcd->uhc_irq);
		return -EPERM;
	}
	HUB_DEBUG("<HUB_CONTROL> typeReq:%x, wValue:%04x, wIndex: %04x, wLength: %04x\n", typeReq, wValue, wIndex, wLength);

	switch (typeReq) {
	case ClearHubFeature:
		HUB_DEBUG("<HUB_CONTROL> ClearHubFeature, wValue:%04x, wIndex: %04x, wLength: %04x\n",
		     wValue, wIndex, wLength);
		break;
	case ClearPortFeature:
		HUB_DEBUG("<HUB_CONTROL> ClearPortFeature, wValue:%04x, wIndex: %04x, wLength: %04x\n",
		     wValue, wIndex, wLength);

		if (wIndex != 1 || wLength != 0)
			goto hub_error;
		HUB_DEBUG("<HUB_CONTROL> before clear operation,the port status is %08x\n", acthcd->port);
		switch (wValue) {
		case USB_PORT_FEAT_ENABLE:
			acthcd->port &= ~(USB_PORT_STAT_ENABLE
					  | USB_PORT_STAT_LOW_SPEED
					  | USB_PORT_STAT_HIGH_SPEED);
			acthcd->rhstate = AOTG_RH_DISABLE;
			if (acthcd->port & USB_PORT_STAT_POWER)
				port_power(acthcd, 0);
			break;
		case USB_PORT_FEAT_SUSPEND:
			HUB_DEBUG("<HUB_CONTROL>clear suspend feathure\n");
			//port_resume(acthcd);
			acthcd->port &= ~(1 << wValue);
			break;
		case USB_PORT_FEAT_POWER:
			acthcd->port = 0;
			acthcd->rhstate = AOTG_RH_POWEROFF;
			port_power(acthcd, 0);
			break;
		case USB_PORT_FEAT_C_ENABLE:
		case USB_PORT_FEAT_C_SUSPEND:
		case USB_PORT_FEAT_C_CONNECTION:
		case USB_PORT_FEAT_C_OVER_CURRENT:
		case USB_PORT_FEAT_C_RESET:
			acthcd->port &= ~(1 << wValue);
			break;
		default:
			goto hub_error;
		}
		HUB_DEBUG("<HUB_CONTROL> after clear operation,the port status is %08x\n", acthcd->port);
		break;
	case GetHubDescriptor:
		HUB_DEBUG("<HUB_CONTROL> GetHubDescriptor, wValue:%04x, wIndex: %04x, wLength: %04x\n",
		     wValue, wIndex, wLength);
		aotg_hub_descriptor((struct usb_hub_descriptor *)buf);
		break;
	case GetHubStatus:
		HUB_DEBUG("<HUB_CONTROL> GetHubStatus, wValue:%04x, wIndex: %04x, wLength: %04x\n",
		     wValue, wIndex, wLength);

		*(__le32 *) buf = __constant_cpu_to_le32(0);
		break;
	case GetPortStatus:
		HUB_DEBUG("<HUB_CONTROL> GetPortStatus, wValue:%04x, wIndex: %04x, wLength: %04x\n",
		     wValue, wIndex, wLength);

		if (wIndex != 1)
			goto hub_error;
		*(__le32 *) buf = cpu_to_le32(acthcd->port);
		HUB_DEBUG("<HUB_CONTROL> the port status is %08x\n ",
			 acthcd->port);
		break;
	case SetHubFeature:
		HUB_DEBUG("<HUB_CONTROL> SetHubFeature, wValue: %04x,wIndex: %04x, wLength: %04x\n",
		     wValue, wIndex, wLength);
		goto hub_error;
		break;
	case SetPortFeature:
		HUB_DEBUG("<HUB_CONTROL> SetPortFeature, wValue:%04x, wIndex: %04x, wLength: %04x\n",
		     wValue, wIndex, wLength);

		switch (wValue) {
		case USB_PORT_FEAT_POWER:
			if (unlikely(acthcd->port & USB_PORT_STAT_POWER))
				break;
			acthcd->port |= (1 << wValue);
			acthcd->rhstate = AOTG_RH_POWERED;
			port_power(acthcd, 1);
			break;
		case USB_PORT_FEAT_RESET:
			port_reset(acthcd);
			/* if it's already enabled, disable */
			acthcd->port &= ~(USB_PORT_STAT_ENABLE
					  | USB_PORT_STAT_LOW_SPEED
					  | USB_PORT_STAT_HIGH_SPEED);
			acthcd->port |= (1 << wValue);
			mdelay(2);
			acthcd->rhstate = AOTG_RH_RESET;
			usb_setbitsb(USBIEN_URES, acthcd->base + USBIEN);
			/*enable reset irq */
			break;
		case USB_PORT_FEAT_SUSPEND:
			/*acthcd->port |= USB_PORT_FEAT_SUSPEND;*/
			acthcd->port |= (1 << wValue);
			acthcd->rhstate = AOTG_RH_SUSPEND;
			//port_suspend(acthcd);
			break;
		default:
			if (acthcd->port & USB_PORT_STAT_POWER)
				acthcd->port |= (1 << wValue);
		}
		break;
	default:
hub_error:
		retval = -EPIPE;
		dev_err(acthcd->dev, "<HUB_CONTROL> hub control error\n");
		break;

	}
	spin_unlock_irqrestore(&acthcd->lock, flags);
	enable_irq(acthcd->uhc_irq);

	return retval;
}

int aotg_hcd_init(struct usb_hcd *hcd, struct platform_device *pdev)
{
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	int retval = 0;
	int i;

	/*init software state */
	spin_lock_init(&acthcd->lock);
	spin_lock_init(&acthcd->tasklet_lock);
	acthcd->tasklet_retry = 0;
	//acthcd->dev = &pdev->dev;
	acthcd->port_specific = pdev->dev.platform_data;
	acthcd->port = 0;
	acthcd->rhstate = AOTG_RH_POWEROFF;
	acthcd->inserted = 0;

	INIT_LIST_HEAD(&acthcd->hcd_enqueue_list);
	INIT_LIST_HEAD(&acthcd->hcd_dequeue_list);
	INIT_LIST_HEAD(&acthcd->hcd_finished_list);
	tasklet_init(&acthcd->urb_tasklet, urb_tasklet_func, (unsigned long)acthcd);

#if 0
	for (i = 0; i < PERIODIC_SIZE; i++) {
		acthcd->load[i] = 0;
		acthcd->periodic[i] = NULL;
	}
#endif

	acthcd->active_ep0 = NULL;
	for (i = 0; i < MAX_EP_NUM; i++) {
		acthcd->ep0[i] = NULL;
		acthcd->inep[i] = NULL;
		acthcd->outep[i] = NULL;
	}

	acthcd->fifo_map[0] = 1<<31;
	acthcd->fifo_map[1] = 1<<31 | 64;
	for (i = 2; i < 64; i++) {
		acthcd->fifo_map[i] = 0;
	}

	acthcd->put_aout_msg = 0;
	acthcd->discon_happened = 0;
	acthcd->uhc_irq = 0;
	acthcd->check_trb_mutex = 0;
	for (i = 0; i < AOTG_QUEUE_POOL_CNT; i++) {
		acthcd->queue_pool[i] = NULL;
	}

	return retval;
}

static const char platform_drv_name[] = "aotg_hcd";
static const char hcd_driver_name[] = "aotg_hub_hcd";

#define AOTG_BUF_NEED_MAP(x, y)	((x != NULL) && (((unsigned long)x & 0x3) == 0))
//#define AOTG_BUF_NEED_MAP(x, y)	((x != NULL) && ((y & 0x3) == 0))

static int aotg_map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb,
				      gfp_t mem_flags)
{
	int ret = 0;

	if (usb_pipetype(urb->pipe) != PIPE_INTERRUPT) {
		ret = usb_hcd_map_urb_for_dma(hcd, urb, mem_flags);
	}
	return ret;
}

static void aotg_unmap_urb_for_dma(struct usb_hcd *hcd, struct urb *urb)
{
	if (usb_pipetype(urb->pipe) != PIPE_INTERRUPT) {

		usb_hcd_unmap_urb_for_dma(hcd, urb);
	}
	return;
}

struct hc_driver act_hc_driver = {
	.description = hcd_driver_name,
	.hcd_priv_size = sizeof(struct aotg_hcd),
	.product_desc = DRIVER_DESC,

	/*
	 * generic hardware linkage
	 */
	.irq = aotg_hub_irq,
	.flags = HCD_USB2 | HCD_MEMORY,

	/* Basic lifecycle operations */
	.start = aotg_hcd_start,
	.stop = aotg_hcd_stop,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue = aotg_hub_urb_enqueue,
	.urb_dequeue = aotg_hub_urb_dequeue,

	.map_urb_for_dma	= aotg_map_urb_for_dma,
	.unmap_urb_for_dma	= aotg_unmap_urb_for_dma,

	.endpoint_disable = aotg_hub_endpoint_disable,

	/*
	 * periodic schedule support
	 */
	.get_frame_number = aotg_hcd_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data = aotg_hub_status_data,
	.hub_control = aotg_hub_control,

	.bus_suspend =        aotg_hub_suspend,
	.bus_resume =         aotg_hub_resume,
};

static int aotg_hub_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	struct aotg_hcep *ep;
	int i;

	usb_remove_hcd(hcd);
	act_hcd_ptr[pdev->id] = NULL;
#if 0    
#ifdef CONFIG_PM
	aotg_hcd_unregister_earlysuspend(acthcd);
#endif
#endif
	aotg_disable_irq(acthcd);
	aotg_clk_enable(acthcd->id, 0);
	acthcd->hcd_exiting = 1;

	tasklet_kill(&acthcd->urb_tasklet);
	del_timer_sync(&acthcd->trans_wait_timer);
	del_timer_sync(&acthcd->check_trb_timer);
	del_timer_sync(&acthcd->hotplug_timer);
	remove_debug_file(acthcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	//aotg_hcd_release_queue(acthcd, NULL);
	owl_powergate_power_off(acthcd->id == 0 ? OWL_POWERGATE_USB2_0 : OWL_POWERGATE_USB2_1);

	for (i = 0; i < MAX_EP_NUM; i++) {
		ep = acthcd->ep0[i];
		if (ep) {
			ACT_HCD_DBG
			if (ep->q)
				ACT_HCD_DBG
			kfree(ep);
		}
	}
//fix here in 64 bit
	for (i = 1; i < MAX_EP_NUM; i++) {
		ep = acthcd->inep[i];
		if (ep) {
			ACT_HCD_DBG
			if (ep->ring) {
				ACT_HCD_DBG
				//aotg_free_ring(acthcd, ep->ring);
			}
			kfree(ep);
		}
	}
	for (i = 1; i < MAX_EP_NUM; i++) {
		ep = acthcd->outep[i];
		if (ep) {
			ACT_HCD_DBG
			if (ep->ring) {
				ACT_HCD_DBG
				//aotg_free_ring(acthcd,ep->ring);
			}
			kfree(ep);
		}
	}
	
	usb_put_hcd(hcd);
	printk("pdev->id remove:%d\n", pdev->id);
	
	if (!port_host_plug_detect[acthcd->id])
		aotg_power_onoff(pdev->id,0);

	return 0;
}

#ifdef	CONFIG_PM
static int aotg_hcd_hub_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);

	if ((hcd == NULL) || (acthcd == NULL)) {
		ACT_HCD_ERR
		return 0;
	}
	dev_info(acthcd->dev, " ==> %s\n", __func__);
	/*do {
		i++;
		msleep(1);
	} while ((acthcd->suspend_request_pend != 0) && (i < 200));*/

	aotg_disable_irq(acthcd);
	//usb_clearbitsb(OTGCTRL_BUSREQ, acthcd->base + OTGCTRL);
	owl_powergate_power_off(acthcd->id == 0 ?
		OWL_POWERGATE_USB2_0 : OWL_POWERGATE_USB2_1);
	aotg_clk_enable(acthcd->id, 0);

	aotg_power_onoff(pdev->id,0);
	acthcd->lr_flag = 1;
	return 0;
}

static int aotg_hcd_hub_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct aotg_hcd *acthcd = hcd_to_aotg(hcd);
	struct usb_device *udev;
	int i;
	
	if ((hcd == NULL) || (acthcd == NULL)) {
		ACT_HCD_ERR
		return 0;
	}

	/* power on. */
/*	if (acthcd->id == 0) {
		aotg0_device_init(1);
	} else {
		aotg1_device_init(1);
	}*/
	//gpio_set_value_cansleep(vbus_otg_en_gpio[pdev->id][0], 1);
	aotg_power_onoff(pdev->id,1);
	msleep(10);

	dev_info(acthcd->dev, " ==> %s\n", __func__);
	aotg_hardware_init(acthcd->id);

	//INIT_LIST_HEAD(&acthcd->hcd_enqueue_list);
	
	for (i = 0; i < MAX_EP_NUM; i++) {
			if (acthcd->ep0[i] != NULL) {
				break;
			}
		}
	
	if (i == MAX_EP_NUM) {
		printk(KERN_ERR"%s, usb device is NULL!\n", __FUNCTION__);
	}
	else {
		udev = acthcd->ep0[i]->udev;
		//if (port_plug_en[acthcd->id]) {
			usb_set_device_state(udev, USB_STATE_NOTATTACHED);
		//}	else {
		//	usb_set_device_state(udev, USB_STATE_CONFIGURED);
		//}
	}
	if (port_host_plug_detect[acthcd->id]) {
		aotg_dev_plugout_msg(acthcd->id);
	} else {
		aotg_enable_irq(acthcd);
	}
	return 0;
}
#endif

void aotg_hcd_shutdown(struct platform_device *pdev)
{	
	struct usb_hcd *hcd	= platform_get_drvdata(pdev);
	struct aotg_hcd	*acthcd	= hcd_to_aotg(hcd);
	printk("usb2-%d shutdown\n", acthcd->id);
	
	//gpio_set_value_cansleep(vbus_otg_en_gpio[pdev->id][0], 0);
	aotg_power_onoff(pdev->id,0);
	return;
}

struct platform_driver aotg_hcd_driver = {
	.probe = aotg_probe,
	.remove = aotg_hub_remove,
#ifdef	CONFIG_PM
	.suspend = aotg_hcd_hub_suspend,
	.resume = aotg_hcd_hub_resume,
#endif
	.shutdown = aotg_hcd_shutdown,
	.driver = {
		.owner = THIS_MODULE,
		.name = platform_drv_name,
		.of_match_table = aotg_of_match,
	},
};
