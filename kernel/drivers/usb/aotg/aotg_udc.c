/*
 * Actions OWL SoCs usb2.0 controller driver
 *
 * Copyright (c) 2015 Actions Semiconductor Co., ltd.
 * dengtaiping <dengtaiping@actions-semi.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License v2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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

#include <mach/hardware.h>
#include <linux/clk.h>
#include <linux/wakelock.h>
#include <linux/suspend.h>
#include <mach/debug.h>
#include <asm/prom.h>
#include <mach/gpio.h>
#include <linux/kallsyms.h>
#include <mach/powergate.h>
#include <mach/module-owl.h>

#include "aotg_plat_data.h"
#include "aotg_udc.h"
#include "aotg_regs.h"
#include "aotg_udc_debug.h"
#include "aotg.h"

#define DRIVER_DESC "Actions USB Device Controller Driver"

static int aotg_udc_kick_dma(struct aotg_ep *ep, struct aotg_request *req);
extern void aotg_udc_dma_handler(struct aotg_udc *udc, u8 dma_number);

static void aotg_ep_reset(struct aotg_udc *udc, u8 ep_mask, u8 type_mask);
static int aotg_ep_disable(struct usb_ep *_ep);
static void nuke(struct aotg_ep *ep, int status);
static inline void udc_handle_status(struct aotg_udc *udc);
extern void done(struct aotg_ep *ep, struct aotg_request *req, int status);
static int write_ep0_fifo(struct aotg_ep *ep, struct aotg_request *req);
int pullup(struct aotg_udc *udc, int is_active);
static void udc_enable(struct aotg_udc *dev);
static void udc_disable(struct aotg_udc *dev);

#ifndef NO_IRQ
#define NO_IRQ  ((unsigned int)(-1))
#endif
#define MAX_PACKET(x)	((x)&0x7FF)

const char udc_driver_name[] = "aotg_udc";
static const char ep0name[] = "ep0";

static inline int is_room_on_udc_ring(struct udc_ring *ring, unsigned int num_trbs)
{
	return (num_trbs > atomic_read(&ring->num_trbs_free)) ? 0 : 1;
}

static inline unsigned int	count_need_trbs(struct usb_request *req)
{
	if (req->num_mapped_sgs > 0)
		return req->num_mapped_sgs;
	else
		return 1;
}

void aotg_epin_dma_reset(struct aotg_udc *udc)
{
	usb_setbitsb(0xa0, udc->base + HCINDMAERROR);
	udelay(1);
	usb_clearbitsb(0x80, udc->base + HCINDMAERROR);
}

int aotg_dma_enable_irq(struct aotg_udc *udc, int dma_nr, int enable)
{
	int index;

	index = dma_nr & 0xf;
	if (enable) {
		if (AOTG_IS_DMA_DEVICE_IN_HOUT(dma_nr)) {
			usb_setbitsw((0x1 << index), (udc->base + HCOUTxDMAIEN0));
		} else {
			usb_setbitsw((0x1 << index), (udc->base + HCINxDMAIEN0));
		}
	} else {
		if (AOTG_IS_DMA_DEVICE_IN_HOUT(dma_nr)) {
			usb_clearbitsw((0x1 << index), (udc->base + HCOUTxDMAIEN0));
		} else {
			usb_clearbitsw((0x1 << index), (udc->base + HCINxDMAIEN0));
		}
	}
	return 0;
}

/* return the dma irq's dma number. */
unsigned int aotg_udc_get_irq(struct aotg_udc *udc)
{
	unsigned int data;
	unsigned int i;
	unsigned int pending = 0;

	data = usb_readw(udc->base + HCOUTxBUFEMPTYIRQ0);
	if (data) {
		for (i = 1; i < 16; i++) {
			if (data & (0x1 << i)) {
				pending = i | AOTG_DMA_OUT_PREFIX;
				return pending;
			}
		}
	}

	data = usb_readw(udc->base + HCINxDMAIRQ0);
	if (data) {
		for (i = 1; i < 16; i++) {
			if (data & (0x1 << i)) {
				pending = i;
				return pending;
			}
		}
	}

	return pending;
}

int aotg_dma_clear_pend(struct aotg_udc *udc, int dma_nr)
{
	int index;

	index = dma_nr & 0xf;
	if (AOTG_IS_DMA_DEVICE_IN_HOUT(dma_nr)) {
		usb_writew((0x1 << index), (udc->base + HCOUTxBUFEMPTYIRQ0));
	} else {
		usb_writew((0x1 << index), (udc->base + HCINxDMAIRQ0));
	}
	return 0;
}

int aotg_dma_clear_pend_all(struct aotg_udc *udc)
{
	usb_writew(0xffff, udc->base + HCINxDMAIRQ0);
	usb_writew(0xffff, udc->base + HCOUTxDMAIRQ0);
	usb_writew(0xffff, udc->base + HCOUTxBUFEMPTYIRQ0);
	return 0;
}

int is_udc_ring_running(struct aotg_ep *ep)
{
	struct aotg_udc *udc = ep->dev;
	return (readl(udc->base + ep->reg_dmactrl) & 0x1);
}

void aotg_start_udc_ring(struct aotg_ep *ep)
{
	struct aotg_udc *udc = ep->dev;
	mb();
	writel(DMACTRL_DMACS, udc->base + ep->reg_dmactrl);
}

void aotg_stop_udc_ring(struct aotg_ep *ep)
{
	struct aotg_udc *udc = ep->dev;
	writel(DMACTRL_DMACC,  udc->base + ep->reg_dmactrl);
}

void aotg_set_udc_ring_linkaddr(struct aotg_ep *ep, u32 addr)
{
	struct aotg_udc *udc = ep->dev;
	writel(addr, udc->base + ep->reg_dmalinkaddr);
}

u32 udc_ring_trb_virt_to_dma(struct udc_ring *ring, struct aotg_trb *trb)
{
	u32 addr;
	unsigned long offset;

	if (!ring || !trb || (trb > ring->last_trb)) {
		UDC_DBG_ERR;
		return 0;
	}

	offset = trb - ring->first_trb;
	addr = ring->trb_dma + (offset * sizeof(*trb));

	return addr;
}

static void pio_udc_irq_disable(struct aotg_ep *ep)
{
	struct aotg_udc *udc = ep->dev;
	u8 is_in = ep->mask & USB_UDC_IN_MASK;
	u8 ep_num = ep->mask & EP_ADDR_MASK;

	if (is_in)
		usb_clearbitsb(1 << ep_num, udc->base + INxIEN);
	else
		usb_clearbitsb(1 << ep_num, udc->base + OUTxIEN);
}
 
void udc_ep_packet_config(enum usb_device_speed usb_speed, struct aotg_udc *udc)
{
	int i;
	u16 packsize;
	struct aotg_ep *ep;

	for (i = 1; i < AOTG_UDC_NUM_ENDPOINTS; i++) {
		ep = &udc->ep[i];
		if (ep->bmAttributes == USB_ENDPOINT_XFER_BULK) {
			packsize = (usb_speed == USB_SPEED_FULL) ? BULK_FS_PACKET_SIZE : BULK_HS_PACKET_SIZE;
		} else if (ep->bmAttributes == USB_ENDPOINT_XFER_INT) {
			packsize = (usb_speed == USB_SPEED_FULL) ? INT_FS_PACKET_SIZE : INT_HS_PACKET_SIZE;
		} else if (ep->bmAttributes == USB_ENDPOINT_XFER_ISOC) {
			packsize = (usb_speed == USB_SPEED_FULL) ? ISO_FS_PACKET_SIZE : ISO_HS_PACKET_SIZE;
		} else {
			continue;
		}
		ep->ep.maxpacket = packsize;
		ep->maxpacket = packsize;
		writew(packsize, udc->base + ep->reg_maxckl);
	}
	return;
}

#if (0)
void aotg_udc_dump_td(struct aotg_ep *ep)
{
	struct aotg_request *req;
	struct aotg_trb *trb;
	struct udc_ring *ring = ep->ring;
	struct aotg_udc *udc = ep->dev;
	printk("reg_dmalinkaddr:(%x)%x, reg_curaddr:(%x)%x, reg_dmactrl:(%x)%x, reg_dmacomplete_cnt:(%x)%x\n",
		ep->reg_dmalinkaddr,readl(udc->base + ep->reg_dmalinkaddr),
		ep->reg_curaddr,readl(udc->base + ep->reg_curaddr),
		ep->reg_dmactrl,readl(udc->base + ep->reg_dmactrl),
		ep->reg_dmacomplete_cnt,readl(udc->base + ep->reg_dmacomplete_cnt));

	printk("===========ring infos==============:\n");
	printk("first_trb:%p, last_trb:%p,enqueue_trb:%p, cur_trb:%p, trb_dma:%p, num_trbs_free:%d\n",
		ring->first_trb, ring->last_trb, ring->enqueue_trb, ring->cur_trb, ring->trb_dma, ring->num_trbs_free);
	trb = ring->first_trb;
	for(trb=ring->first_trb; trb < ring->last_trb; trb++)	{
		printk("hw_buf_ptr:%x, hw_buf_len:%x, hw_buf_remain:%x, hw_token:%x\n",
			trb->hw_buf_ptr, trb->hw_buf_len, trb->hw_buf_remain, trb->hw_token);
	}
	list_for_each_entry(req, &ep->queue, queue) {
		printk("\t req %p len %d/%d buf %p\n",
		&req->req, req->req.actual, req->req.length, req->req.buf);
	}
}
#endif

static int aotg_ep_enable(struct usb_ep *_ep, const struct usb_endpoint_descriptor *desc)
{
	struct aotg_ep *ep;
	struct aotg_udc *udc;
	unsigned long flags;

	ep = container_of(_ep, struct aotg_ep, ep);
	/*sanity check */
	if (!_ep || !desc || _ep->name == ep0name
	    || desc->bDescriptorType != USB_DT_ENDPOINT
	    || ep->bEndpointAddress != desc->bEndpointAddress 
	    || ep->maxpacket < le16_to_cpu(desc->wMaxPacketSize)) {
		UDC_ERR("<AOTG_EP_ENABLE>%s, bad ep or descriptor\n", __func__);
		return -EINVAL;
	}

	/*xfer types must match, except that interrupt ~= bulk */
	if (ep->bmAttributes != desc->bmAttributes
	    && ep->bmAttributes != USB_ENDPOINT_XFER_BULK
	    && ep->bmAttributes != USB_ENDPOINT_XFER_ISOC 
	    && ep->bmAttributes != USB_ENDPOINT_XFER_INT) {
		UDC_ERR("<AOTG_EP_ENABLE>%s: %s type mismatch\n", __func__, _ep->name);
		return -EINVAL;
	}

	if ((desc->bmAttributes == USB_ENDPOINT_XFER_BULK && le16_to_cpu(desc->wMaxPacketSize)
	     != ep->maxpacket)
	    || !desc->wMaxPacketSize) {
		UDC_ERR("<AOTG_EP_ENABLE>%s: bad %s maxpacket\n", __func__, _ep->name);
		return -ERANGE;
	}

	udc = ep->dev;
	if (!udc->driver || udc->gadget.speed == USB_SPEED_UNKNOWN) {
		if (!udc->driver)
			UDC_DEBUG("no driver\n");
		if (udc->gadget.speed == USB_SPEED_UNKNOWN)
			UDC_DEBUG("UNKNOW speed\n");
		UDC_DEBUG("%s: bogus device state\n", __func__);
		return -ESHUTDOWN;
	}
	spin_lock_irqsave(&udc->lock, flags);
	ep->ep.desc = desc;
	ep->stopped = 0;
	ep->udc_irqs = 0;
	ep->read.bytes = 0;
	ep->read.ops = 0;
	ep->write.bytes = 0;
	ep->write.ops = 0;
	ep->ep.maxpacket = le16_to_cpu(desc->wMaxPacketSize);

	ep->dma_no = ep->bEndpointAddress & EP_ADDR_MASK;
	if (((ep->mask & USB_UDC_IN_MASK) != 0) && (ep->dma_no != 0)) {
		ep->dma_no = AOTG_DMA_OUT_PREFIX | ep->dma_no;
	}

	aotg_ep_reset(udc, ep->mask, ENDPRST_FIFORST | ENDPRST_TOGRST);
	usb_setbitsb(EPCON_VAL, udc->base + ep->reg_udccon);

	spin_unlock_irqrestore(&udc->lock, flags);
	UDC_DEBUG("<EP ENABLE>%s enable, reg_udccon is %02x\n", _ep->name, readb(udc->base + ep->reg_udccon));

	if (ep->dma_no) 
		aotg_dma_clear_pend(udc, ep->dma_no);

	if (ep->bmAttributes == USB_ENDPOINT_XFER_CONTROL)
		return 0;

	if (ep->ring != NULL) {
		UDC_DBG_ERR;
		return 0;
	}

	ep->ring = kmalloc(sizeof(struct udc_ring), GFP_ATOMIC);
	if (!ep->ring) {
		printk("no ring mem!\n");
		return -ENOMEM;
	}
	
	ep->ring->is_running = 0;
	ep->ring->num_trbs = NUM_TRBS;
	atomic_set(&ep->ring->num_trbs_free, NUM_TRBS);

	ep->ring->first_trb = (struct aotg_trb *)
		dma_alloc_coherent(udc->dev, NUM_TRBS * sizeof(struct aotg_trb),
		(dma_addr_t *)&ep->ring->trb_dma, GFP_ATOMIC);
	memset(ep->ring->first_trb, 0, RING_SIZE);
	ep->ring->last_trb = ep->ring->first_trb + (NUM_TRBS - 1);
	ep->ring->enqueue_trb = ep->ring->first_trb;
	ep->ring->cur_trb = ep->ring->first_trb;
	return 0;
}

#if (0)
void aotg_dump_ring(void)
{
	struct aotg_ep *ep;
	struct aotg_request *req;
	struct usb_endpoint_descriptor *desc;
	struct aotg_trb *trb;
	struct udc_ring *ring;
	struct aotg_udc *udc;
	int i;

	for (i = 0; i < AOTG_UDC_NUM_ENDPOINTS; i++) {
		ep = &acts_udc_controller->ep[i];
		udc = ep->dev;

		if (i != 0) {
			desc = ep->ep.desc;
			if (!desc)
				continue;
			
			printk("index:%d\n",i);
			printk("%s max %d irqs %d\n",
				ep->ep.name, le16_to_cpu(desc->wMaxPacketSize), ep->udc_irqs);

		} else		/* ep0 should only have one transfer queued */
			printk("ep0 max 16 pio irqs %x\n", ep->udc_irqs);

		if (list_empty(&ep->queue)) {
			printk("(nothing queued)\n");
			continue;
		}
		printk("reg_dmalinkaddr:(%x)%x, reg_curaddr:(%x)%x, reg_dmactrl:(%x)%x, reg_dmacomplete_cnt:(%x)%x\n",
			ep->reg_dmalinkaddr,readl(udc->base + ep->reg_dmalinkaddr),
			ep->reg_curaddr,readl(udc->base + ep->reg_curaddr),
			ep->reg_dmactrl,readl(udc->base + ep->reg_dmactrl),
			ep->reg_dmacomplete_cnt, readl(udc->base + ep->reg_dmacomplete_cnt));
		
		ring = ep->ring;
		if (!ring)
			continue;
		printk("===========ring infos==============:\n");
		printk("first_trb:%p, last_trb:%p, enqueue_trb:%p, cur_trb:%p, trb_dma:%d, num_trbs_free:%d\n",
				ring->first_trb,ring->last_trb, ring->enqueue_trb, ring->cur_trb,ring->trb_dma, ring->num_trbs_free);
		trb = ring->first_trb;
		for(trb=ring->first_trb; trb < ring->last_trb; trb++)	{
			printk("hw_buf_ptr:%x, hw_buf_len:%x, hw_buf_remain:%x, hw_token:%x\n",
				trb->hw_buf_ptr, trb->hw_buf_len, trb->hw_buf_remain, trb->hw_token);
		}

		list_for_each_entry(req, &ep->queue, queue) {
			printk("req %p len %d/%d buf %p\n",
				&req->req, req->req.actual, req->req.length, req->req.buf);
		}
	}
	return;
}
#endif

static int aotg_ep_disable(struct usb_ep *_ep)
{
	struct aotg_ep *ep;
	struct aotg_udc *udc;
	unsigned long flags;

	ep = container_of(_ep, struct aotg_ep, ep);
	if (!_ep || !ep->ep.desc) {
		UDC_DEBUG("<EP DISABLE> %s not enabled\n", _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	udc = ep->dev;
	spin_lock_irqsave(&udc->lock, flags);
	nuke(ep, -ESHUTDOWN);

	aotg_stop_udc_ring(ep);
	usb_clearbitsb(EPCON_VAL, udc->base + ep->reg_udccon);
	pio_udc_irq_disable(ep);
	if (ep->dma_no) {
		aotg_dma_enable_irq(udc, ep->dma_no, 0);
		aotg_dma_clear_pend(udc, ep->dma_no);
	}
	ep->ep.driver_data = NULL;
	ep->desc = NULL;
	ep->stopped = 1;

	if (ep->ring != NULL) {
		dma_free_coherent(udc->dev, 
			NUM_TRBS * sizeof(struct aotg_trb), ep->ring->first_trb, ep->ring->trb_dma);
		kfree(ep->ring);
		ep->ring = NULL;
	}

	spin_unlock_irqrestore(&udc->lock, flags);\
	UDC_DEBUG("<EP DISABLE>%s disable\n", _ep->name);
	return 0;
}

static struct usb_request *aotg_ep_alloc_request(struct usb_ep *_ep, unsigned gfp_flags)
{
	struct aotg_request *req;

	UDC_DEBUG("<EP ALLOC REQ>%s, flags is %d\n", _ep->name, gfp_flags);
	req = kmalloc(sizeof(*req), gfp_flags);
	if (!req)
		return NULL;
	memset(req, 0, sizeof(*req));
	INIT_LIST_HEAD(&req->queue);
	return &req->req;
}

static void aotg_ep_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct aotg_request *req;

	UDC_DEBUG("<EP FREE REQ>%s, %p\n", _ep->name, _req);
	req = container_of(_req, struct aotg_request, req);
	if (!list_empty(&req->queue))
		UDC_DEBUG("<EP FREE REQ>ep's queue is not empty\n");
	kfree(req);

}

void udc_inc_enqueue_safe(struct udc_ring *ring)
{
	atomic_dec(&ring->num_trbs_free);
	if (ring->enqueue_trb == ring->last_trb) {
		ring->enqueue_trb->hw_token &= ~(TRB_CHN|TRB_CSP);
		ring->enqueue_trb->hw_token |= TRB_LT;
		ring->enqueue_trb = ring->first_trb;
	} else {
		ring->enqueue_trb += 1;
	}
}

void udc_inc_dequeue_safe(struct udc_ring *ring)
{
	memset(ring->cur_trb, 0, sizeof(struct aotg_trb));
	atomic_inc(&ring->num_trbs_free);
	if (ring->cur_trb == ring->last_trb) {
		ring->cur_trb = ring->first_trb;
	} else {
		ring->cur_trb++;
	}
	return;
}

void aotg_dma_enqueue_trb(struct udc_ring *ring, u32 buf_ptr, u32 buf_len, u32 token)
{
	struct aotg_trb *trb;
	trb = ring->enqueue_trb;

	trb->hw_buf_ptr = buf_ptr;
	trb->hw_buf_len = buf_len;
	trb->hw_token = token;
	trb->hw_buf_remain = 0;
	udc_inc_enqueue_safe(ring);	
}

static void aotg_start_udc_ring_transfer(struct aotg_ep *ep, struct aotg_trb *trb)
{
	u32 addr;
	struct udc_ring *ring = ep->ring;
	addr = udc_ring_trb_virt_to_dma(ring, trb);
	if (addr) {
		aotg_set_udc_ring_linkaddr(ep, addr);
		aotg_start_udc_ring(ep);
	}
}

static int aotg_ep_queue(struct usb_ep *_ep, struct usb_request *_req, unsigned gfp_flags)
{
	struct aotg_ep *ep;
	struct aotg_udc *udc;
	struct aotg_request *req;
	unsigned long flags;

	UDC_DEBUG("<EP QUEUE> %s queues one req %p\n", _ep->name, _req);
	/*sanity check */
	req = container_of(_req, struct aotg_request, req);
	if (unlikely(!_req || !_req->complete || !_req->buf || !list_empty(&req->queue))) {
		UDC_ERR("<EP QUEUE>bad params\n");
		return -EINVAL;
	}

	ep = container_of(_ep, struct aotg_ep, ep);
	if (unlikely(!_ep || (!ep->ep.desc && ep->ep.name != ep0name))) {
		UDC_ERR("<EP QUEUE> bad ep\n");
		return -EINVAL;
	}

	udc = ep->dev;
	if (unlikely(!udc->driver || udc->gadget.speed == USB_SPEED_UNKNOWN)) {
		UDC_ERR("<EP QUEUE> bogus device state\n");
		return -ESHUTDOWN;
	}

	//aotg_dbg_put_info("ep_que", ep->ep.name, _req->length, _req->buf, _req->actual);
	UDC_DEBUG("<EP QUEUE>%s queue req %p, len %d buf %p\n", _ep->name, _req, _req->length, _req->buf);
	/*it may be very noisy */

	spin_lock_irqsave(&udc->lock, flags);
	_req->status = -EINPROGRESS;
	_req->actual = 0;
	UDC_DEBUG("<EP QUEUE>%s queue req %p, len %d buf %p\n", _ep->name, _req, _req->length, _req->buf);

	/*only if the req queue of ep is empty and ep is working ,
	 we kick start the queue */
	if (!ep->stopped) {
		if (ep->bEndpointAddress == 0) {
			if (!udc->req_pending) {
				UDC_ERR("<EP QUEUE> something wrong with Control Xfers, req_pending is missing\n");
				spin_unlock_irqrestore(&udc->lock, flags);
				return -EL2HLT;
			}

			switch (udc->ep0state) {
			case EP0_OUT_DATA_PHASE:
				udc->stats.read.ops++;
				/*No-Data Control Xfer */
				if (!req->req.length) {
					 /*ACK*/ 
					udc_handle_status(udc);
					/*cleanup */
					udc->req_pending = 0;
					udc->ep0state = EP0_WAIT_FOR_SETUP;
					done(ep, req, 0);
					req = NULL;
					spin_unlock_irqrestore(&udc->lock, flags);
					return 0;

					/*Control Write Xfer */
				} else {
					/*in this case, we just arm the OUT EP0
					   for first OUT transaction during
					   the data stage
					   hang this req at the tail of
					   queue aossciated with EP0
					   expect OUT EP0 interrupt
					   to advance the i/o queue
					 */

					writeb(0, udc->base + OUT0BC);
				}
				break;
			case EP0_IN_DATA_PHASE:
				udc->stats.write.ops++;
				/*Control Read Xfer */
				if (write_ep0_fifo(ep, req))
					udc->ep0state = EP0_END_XFER;	//!!!!handle_ep0_in will call done(ep, req, 0);
				break;
			default:
				UDC_DEBUG("<EP QUEUE> ep0 i/o, odd state %d\n", udc->ep0state);
				spin_unlock_irqrestore(&udc->lock, flags);
				return -EL2HLT;
			}

		}	else {
			aotg_udc_kick_dma(ep, req);
		}
	}

	/*pio or dma irq handler advances the queue. */
	if (likely(req != NULL)) {
		list_add_tail(&req->queue, &ep->queue);
		UDC_DEBUG("<EP QUEUE>the req of %s is not be done completely,queueing and wait irq kickstart\n", ep->ep.name);
	}
	
	if (ep->bEndpointAddress != 0) {
		if (!is_udc_ring_running(ep)) {
			aotg_start_udc_ring_transfer(ep, ep->ring->cur_trb);
		}
	}
	
	spin_unlock_irqrestore(&udc->lock, flags);
	return 0;
}

static int aotg_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct aotg_ep *ep;
	struct aotg_request *req;
	struct aotg_udc *udc;
	struct aotg_trb *trb;
	struct udc_ring *ring;
	unsigned long flags;
	int i;

	UDC_DEBUG("<EP DEQUEUE> %s dequeues one req %p\n", _ep->name, _req);
	ep = container_of(_ep, struct aotg_ep, ep);

	if (!_ep || ep->ep.name == ep0name)
		return -EINVAL;

	udc = ep->dev;
	spin_lock_irqsave(&udc->lock, flags);
	/*make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req)
			break;
	}
	if (&req->req != _req) {
		UDC_DBG_ERR;
		spin_unlock_irqrestore(&udc->lock, flags);
		return -EINVAL;
	}

	//aotg_dbg_put_info("ep_dequeue", ep->ep.name, _req->length, _req->buf, _req->actual);

	aotg_stop_udc_ring(ep);
	done(ep, req, -ECONNRESET);
	trb = req->start_trb;
	ring = ep->ring;
	for (i = 0; i < req->num_trbs; i++) {
		memset(req->start_trb,0,sizeof(struct aotg_trb));
		udc_inc_dequeue_safe(ring);
		if (trb == ring->last_trb)
			trb = ring->first_trb;
		else
			trb += 1;
	}
	
	if (!list_empty(&ep->queue)) {
		UDC_DBG_ERR;
		aotg_start_udc_ring_transfer(ep,ring->cur_trb);
	}

	spin_unlock_irqrestore(&udc->lock, flags);
	return 0;
}

// ========================================== hjk_checked  
static int aotg_ep_set_halt(struct usb_ep *_ep, int value)
{
	struct aotg_ep *ep;
	struct aotg_udc *udc;
	int retval = -EOPNOTSUPP;
	unsigned long flags;

	ep = container_of(_ep, struct aotg_ep, ep);
	if (!_ep || (!ep->ep.desc && ep->ep.name != ep0name)) {
		UDC_DEBUG("<EP HALT>, bad ep\n");
		return -EINVAL;
	}

	udc = ep->dev;
	spin_lock_irqsave(&udc->lock, flags);

	/*EP0 */
	if (ep->bEndpointAddress == 0) {
		if (value) {
			usb_setbitsb(EP0CS_STALL, udc->base + EP0CS);
			udc->req_pending = 0;
			udc->ep0state = EP0_STALL;
			retval = 0;
		} else {
			usb_clearbitsb(EP0CS_STALL, udc->base + EP0CS);
			udc->ep0state = EP0_WAIT_FOR_SETUP;
			retval = 0;
		}

		/*otherwise, all active non-ISO endpoints can halt */
	} else if (ep->bmAttributes != USB_ENDPOINT_XFER_ISOC && ep->desc) {
		/*IN endpoints must already be idle */
		if ((ep->bEndpointAddress & USB_DIR_IN)
		    && !list_empty(&ep->queue)) {
			UDC_ERR("<EP HALT>dangrous,epin queue is not empty\n");
			retval = 0;
			goto done;
		}
		/*IN endpoint FIFO must be empty */
		if (ep->bEndpointAddress & USB_DIR_IN) {
			u8 not_empty;
			not_empty = (readb(udc->base + ep->reg_udccs) & EPCS_BUSY)
			    || (ep->buftype - ((readb(udc->base + ep->reg_udccs) >> 2) & 0x03));
			if (not_empty) {
				UDC_ERR("<EP HALT>dangrous, epin fifo is not empty\n");
				retval = 0;
				goto done;
			}
		}

		if (value) {
			/*set the stall bit */
			usb_setbitsb(EPCON_STALL, udc->base + ep->reg_udccon);
			ep->stopped = 1;
		} else {
			/*clear the stall bit */
			usb_clearbitsb(EPCON_STALL, udc->base + ep->reg_udccon);
			ep->stopped = 0;
		}
		retval = 0;
	}

      done:
	UDC_DEBUG("<EP HALT>%s %s halt stat %d\n", ep->ep.name, value ? "set" : "clear", retval);
	spin_unlock_irqrestore(&udc->lock, flags);

	return retval;
}

static void aotg_ep_fifo_flush(struct usb_ep *_ep)
{
	struct aotg_udc *udc;
	struct aotg_ep *ep;
	ep = container_of(_ep, struct aotg_ep, ep);
	if (!_ep || ep->ep.name == ep0name || !list_empty(&ep->queue)) {
		UDC_DEBUG("<EP FIFO FLUSH>bad ep\n");
		return;
	}
	UDC_DEBUG("<EP FIFO FLUSH>%s fifo flush\n", ep->ep.name);
	udc = ep->dev;
	aotg_ep_reset(udc, ep->mask, ENDPRST_FIFORST);
}

static struct usb_ep_ops aotg_ep_ops = {
	.enable = aotg_ep_enable,
	.disable = aotg_ep_disable,

	.alloc_request = aotg_ep_alloc_request,
	.free_request = aotg_ep_free_request,

	.queue = aotg_ep_queue,
	.dequeue = aotg_ep_dequeue,

	.set_halt = aotg_ep_set_halt,
	//.fifo_status  = aotg_ep_fifo_status,
	.fifo_flush = aotg_ep_fifo_flush,	/*not sure */

};

/*---------------------------------------------------------------------------
 *  device operations  related parts of
 *  the api to the usb controller hardware,
 *  which don't involve endpoints (or i/o), used by gadget driver;
 *  and the inner talker-to-hardware core.
 *---------------------------------------------------------------------------
 */
static int aotg_udc_get_frame(struct usb_gadget *_gadget)
{
	struct aotg_udc *udc;
	u16 frmnum = 0;

	udc = container_of(_gadget, struct aotg_udc, gadget);

	UDC_DEBUG("<UDC_GET_FRAME>Frame No.: %d\n", frmnum);
	return frmnum;

}

static int aotg_udc_wakeup(struct usb_gadget *_gadget)
{
	struct aotg_udc *udc;
	int retval = -EHOSTUNREACH;

	udc = container_of(_gadget, struct aotg_udc, gadget);

	return retval;
}

static int aotg_udc_vbus_session(struct usb_gadget *_gadget, int is_active)
{
	struct aotg_udc *udc;
	unsigned long flags;

	UDC_DEBUG("<UDC_VBUS_SESSION> VBUS %s\n", is_active ? "on" : "off");
	udc = container_of(_gadget, struct aotg_udc, gadget);
	spin_lock_irqsave(&udc->lock, flags);
	pullup(udc, is_active);
	spin_unlock_irqrestore(&udc->lock, flags);

	return 0;

}

static int aotg_udc_pullup(struct usb_gadget *_gadget, int is_on)
{
	struct aotg_udc *udc;
	unsigned long flags;

	udc = container_of(_gadget, struct aotg_udc, gadget);
	spin_lock_irqsave(&udc->lock, flags);
	udc->softconnect = (is_on != 0);
	pullup(udc, is_on);
	spin_unlock_irqrestore(&udc->lock, flags);

	return 0;
}

static int aotg_udc_vbus_draw(struct usb_gadget *_gadget, unsigned mA)
{
	struct aotg_udc *udc;

	udc = container_of(_gadget, struct aotg_udc, gadget);
	if (udc->transceiver)
		return usb_phy_set_power(udc->transceiver, mA);
	return -EOPNOTSUPP;
}

static int aotg_udc_start(struct usb_gadget *g, struct usb_gadget_driver *driver)
{
	int retval;
	struct aotg_udc *udc = container_of(g, struct aotg_udc, gadget);
	udc->driver = driver;
	extern_irq_enable(udc);
	udc_enable(udc);
	aotg_udc_endpoint_config(udc);

	if (udc->transceiver) {
		retval = otg_set_peripheral(udc->transceiver->otg, &udc->gadget);
		if (retval) {
			dev_err(udc->dev, "can't bind to transceiver\n");
			return retval;
		}
	}

	return 0;
}

static int aotg_udc_stop(struct usb_gadget *g, struct usb_gadget_driver *driver)
{
	struct aotg_udc *udc = container_of(g, struct aotg_udc, gadget);
	udc->driver = NULL;
	extern_irq_disable(udc);
	udc_disable(udc);
	if (udc->transceiver)
		return otg_set_peripheral(udc->transceiver->otg, NULL);
	return 0;
}

static const struct usb_gadget_ops aotg_udc_ops = {
	.get_frame = aotg_udc_get_frame,
	.wakeup = aotg_udc_wakeup,
	.pullup = aotg_udc_pullup,
	.vbus_session = aotg_udc_vbus_session,
	.vbus_draw = aotg_udc_vbus_draw,
	.udc_start = aotg_udc_start,
	.udc_stop = aotg_udc_stop,
};

int aotg_enqueue_req(struct aotg_ep *ep, struct aotg_request *aotg_req)
{
	unsigned	length;
	dma_addr_t	dma;
	struct usb_request *req = &aotg_req->req;
	struct udc_ring *ring = ep->ring;
	u32 token = 0,num_trbs;
	int is_in = !!(ep->bEndpointAddress & USB_DIR_IN);
	
	num_trbs = count_need_trbs(req);
	if (!is_room_on_udc_ring(ring, num_trbs)) {
		UDC_DBG_ERR;
		return -1;
	}
	
	aotg_req->start_trb = ring->enqueue_trb;
	aotg_req->cross_req = 0;
	aotg_req->num_trbs = num_trbs;
	
	if (req->num_mapped_sgs > 0) {
		struct scatterlist *sg = req->sg;
		struct scatterlist *s;
		int	i;
		for_each_sg(sg, s, req->num_mapped_sgs, i) {
			length = sg_dma_len(s);
			dma = sg_dma_address(s);
			token = TRB_OF;
			if (!is_in)
				token |= TRB_CSP;
			
			if (i == (req->num_mapped_sgs - 1) || sg_is_last(s)) {
				break;
			}	else {
				if (ring->enqueue_trb == ring->last_trb) {
					aotg_req->cross_req = 1;
					if (is_in) {
						token |= TRB_ITE;
					} else {
						token |= TRB_ICE;
					}
				} else {
					token |= TRB_CHN;
				}
				aotg_dma_enqueue_trb(ring, (u32)dma, length, token);
			}
		}
	}
	else {

		dma = req->dma;
		length = req->length;
	}
	
	token = TRB_OF;
	if (is_in) {
			token |= TRB_ITE;
	} else {
		token |= TRB_CSP|TRB_ICE;
	}
	aotg_req->end_trb = ring->enqueue_trb;
	aotg_dma_enqueue_trb(ring, (u32)dma, length, token);
	return 0;
}

static int aotg_udc_kick_dma(struct aotg_ep *ep, struct aotg_request *aotg_req)
{
	int is_in = !!(ep->bEndpointAddress & USB_DIR_IN);
	struct aotg_udc *udc = ep->dev;

	usb_gadget_map_request(&udc->gadget, &aotg_req->req, is_in);
	return aotg_enqueue_req(ep, aotg_req);
}

static int aotg_udc_dma_irq_handler(struct aotg_udc *udc, u8 irqshare)
{
	unsigned int dma_pend;
	int i = 0;

	do {
		dma_pend = (unsigned int) aotg_udc_get_irq(udc);
		if (dma_pend == 0)
			return i;
		aotg_dma_clear_pend(udc, dma_pend);

		if (i++ >= 30) {
			UDC_DBG_INFO;
			return i;
		}

		aotg_udc_dma_handler(udc, dma_pend);
	} while (dma_pend);

	return i;
}

int aotg_udc_finish_req(struct aotg_ep *ep, struct aotg_request *aotg_req)
{
	struct aotg_trb *trb;
	int i, trb_tx_len, length = 0;
	struct udc_ring *ring = ep->ring;
	if (aotg_req->cross_req) {
		aotg_req->cross_req = 0;
		aotg_start_udc_ring_transfer(ep,ring->first_trb);
		return 1;
	}

	trb = aotg_req->start_trb;
	for (i = 0; i < aotg_req->num_trbs; i++) {
		if (trb->hw_token & (AOTG_TRB_IOS | AOTG_TRB_IOZ)){
			trb_tx_len = trb->hw_buf_len - trb->hw_buf_remain;
			length += trb_tx_len;
			udc_inc_dequeue_safe(ring);
			break;
		} else if (trb->hw_token & AOTG_TRB_IOC) {
			length += trb->hw_buf_len;
		} else {
			if (!is_udc_ring_running(ep)) {
				aotg_start_udc_ring_transfer(ep,ring->cur_trb);
			}
			return -1;
		}

		udc_inc_dequeue_safe(ring);
		if (trb == ring->last_trb)
			trb = ring->first_trb;
		else
			trb += 1;
	}

	aotg_req->req.actual = length;
	done(ep, aotg_req, 0);

	return 0;
}

void aotg_udc_dma_handler(struct aotg_udc *udc, u8 dma_number)
{
	struct aotg_ep *ep = (struct aotg_ep *) (&(udc->ep));
	struct aotg_request *aotg_req = NULL, *next;
	unsigned int is_in = 0;
	unsigned int i;
	struct udc_ring *ring;

	for (i = 0; i < AOTG_UDC_NUM_ENDPOINTS; i++) {
		if (ep[i].dma_no == dma_number) {
			ep = &ep[i];
			break;
		}
	}
	if (i == AOTG_UDC_NUM_ENDPOINTS) {
		printk("can't find correct ep no in dma irq! dma_number:%d\n", dma_number);
		return;
	}

	ring = ep->ring;
	is_in = !!(ep->bEndpointAddress & USB_DIR_IN);
	aotg_req = list_first_entry_or_null(&ep->queue,struct aotg_request, queue);
	if (aotg_req == NULL)
		return;

	UDC_DEBUG("<dma_handler>:ep address =%d \n", ep->bEndpointAddress);
	ep->udc_irqs++;
	ep->dev->stats.irqs++;

	if (list_empty(&ep->queue)) {
		UDC_DBG_ERR;
		return;
	}
	list_for_each_entry_safe(aotg_req, next, &ep->queue, queue) {
		if (aotg_udc_finish_req(ep, aotg_req))
			break;
	}

	if (list_empty(&ep->queue) && is_udc_ring_running(ep)) {
		aotg_stop_udc_ring(ep);
		UDC_DBG_ERR;
	} else if (!list_empty(&ep->queue) && !(is_udc_ring_running(ep))) {
		UDC_DBG_ERR;
		aotg_start_udc_ring_transfer(ep, ep->ring->cur_trb);
	}

	return;
}

/*---------------------------------------------------------------------------
 *  handle  interrupt
 *---------------------------------------------------------------------------
*/
void done(struct aotg_ep *ep, struct aotg_request *req, int status)
{
	//unsigned stopped = ep->stopped;
	u8 direction;
	struct aotg_udc *udc = ep->dev;

	list_del_init(&req->queue);

	if (likely(req->req.status == -EINPROGRESS)) {
		req->req.status = status;
	} else {
		printk("status:%x req.status:%x\n",status,req->req.status);
		status = req->req.status;
	}



	if (status && status != -ESHUTDOWN) {
		UDC_DEBUG("<REQ RELEASE>complete %s req %p stat %d len %u/%u\n",
			  ep->ep.name, &req->req, status, req->req.actual, req->req.length);
	}
	if (ep->bEndpointAddress != 0) {
		direction = !!(ep->mask & USB_UDC_IN_MASK);
		usb_gadget_unmap_request(&udc->gadget, &req->req, direction);
	}

	spin_unlock(&ep->dev->lock);
	req->req.complete(&ep->ep, &req->req);
	spin_lock(&ep->dev->lock);
	return;
}

static inline void aotg_ep_setup(struct aotg_udc *udc, u8 index, char *name, u8 addr, u8 type, u8 buftype)
{
	struct aotg_ep *ep;
	ep = &udc->ep[index];

	strlcpy(ep->name, name, sizeof ep->name);
	ep->ep.name = ep->name;
	ep->bmAttributes = type;
	ep->bEndpointAddress = addr;

	if (type == USB_ENDPOINT_XFER_BULK) {
		if (ep->bEndpointAddress & USB_DIR_IN) {
			if ((ep->bEndpointAddress & EP_ADDR_MASK) == 1)
				writel(EP1_BULK_IN_STARTADD, udc->base + ep->reg_fifostaddr);
			else
				writel(EP2_BULK_IN_STARTADD, udc->base + ep->reg_fifostaddr);
		} else {
			if ((ep->bEndpointAddress & EP_ADDR_MASK) == 1)
				writel(EP1_BULK_OUT_STARTADD, udc->base + ep->reg_fifostaddr);
			else
				writel(EP2_BULK_OUT_STARTADD, udc->base + ep->reg_fifostaddr);
		}

	} else if (type == USB_ENDPOINT_XFER_INT) {
		writel(EP_INT_IN_STARTADD, udc->base + ep->reg_fifostaddr);
	} else if (type == USB_ENDPOINT_XFER_ISOC) {
		writel(EP_ISO_IN_STARTADD, udc->base + ep->reg_fifostaddr);
	} else {
		return;
	}

	ep->buftype = buftype;
	if (type == USB_ENDPOINT_XFER_ISOC)
		writeb((type << 2) | buftype | (1 << 5), udc->base + ep->reg_udccon);
	else
		writeb((type << 2) | buftype, udc->base + ep->reg_udccon);
	return;
}

static void aotg_ep_reset(struct aotg_udc *udc, u8 ep_mask, u8 type_mask)
{
	u8 val;

	writeb(ep_mask, udc->base + ENDPRST);	/*select ep */
	val = ep_mask | type_mask;
	writeb(val, udc->base + ENDPRST);	/*reset ep */
}

static inline void udc_handle_status(struct aotg_udc *udc)
{
	usb_setbitsb(EP0CS_HSNAK, udc->base + EP0CS);
	UDC_DEBUG("<CTRL>ACK the status stage\n");
}

static void nuke(struct aotg_ep *ep, int status)
{
	struct aotg_request *req;

	if ((ep->bEndpointAddress != 0) && !ep->stopped)
		aotg_stop_udc_ring(ep);
	UDC_DEBUG("<EP NUKE> %s is nuked with status %d\n", ep->ep.name, status);

	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next, struct aotg_request, queue);
		done(ep, req, status);
	}
/*	if (ep->desc)
		pio_udc_irq_disable(ep);*/
}

static void handle_setup(struct aotg_udc *dev, unsigned long *pflag)
{
	u16 w_value, w_length, w_index;
	u32 ackval = 0;
	int reciptype;
	int ep_num;
	struct aotg_ep *ep;
	struct aotg_udc *udc = dev;
	struct aotg_ep *ep0 = &udc->ep[0];
	union {
		struct usb_ctrlrequest r;
		u8 raw[8];
	} u;
	int i, status = 0;
	unsigned long addr = SETUPDATA_W0;
	unsigned long flags = *pflag;

	if (udc->ep0state != EP0_WAIT_FOR_SETUP) {
		nuke(ep0, -ESHUTDOWN);
		udc->ep0state = EP0_WAIT_FOR_SETUP;
	} else
		nuke(ep0, -EPROTO);
	for (i = 0; i < 8; i++) {
		u.raw[i] = readb(udc->base + addr);
		addr++;
	}
	w_value = le16_to_cpup(&u.r.wValue);
	w_length = le16_to_cpup(&u.r.wLength);
	w_index = le16_to_cpup(&u.r.wIndex);
	UDC_DEBUG("<CTRL> SETUP %02x.%02x v%04x i%04x l%04x\n", u.r.bRequestType, u.r.bRequest, w_value, w_index, w_length);

	/*Delegate almost all control requests to the gadget driver,
	 *except for a handful of ch9 status/feature requests that
	 *hardware doesn't autodecode and the gadget API hides.
	 */
	udc->req_std = (u.r.bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD;
	udc->req_config = 0;
	udc->req_pending = 1;
	ep0->stopped = 0;
	reciptype = (u.r.bRequestType & USB_RECIP_MASK);
#if 1
	if ((u.r.bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR)
		goto delegate;
#endif

	switch (u.r.bRequest) {
	case USB_REQ_GET_STATUS:
		UDC_DEBUG("<CTRL>USB_REQ_GET_STATUS\n");
		if (reciptype == USB_RECIP_INTERFACE) {
			/*according to USB spec,
			   this does nothing but return 0 */
			break;
		} else if (reciptype == USB_RECIP_DEVICE) {
			/*return self powered and remote wakeup status
			 *we are self powered , so just check wakeup character
			 */
			if (udc->rwk)
				ackval = 0x02;
			else
				ackval = 0x00;
		} else if (reciptype == USB_RECIP_ENDPOINT) {
			ep_num = u.r.wIndex & EP_ADDR_MASK;

			if (ep_num > (AOTG_UDC_NUM_ENDPOINTS - 1)
			    || u.r.wLength > 2)
				goto stall;
			ep = &udc->ep[ep_num];
			if ((ep_num != 0)
			    && (ep->bEndpointAddress != u.r.wIndex))
				goto stall;
			if (ep->bEndpointAddress == 0)
				ackval |= ((readb(udc->base + EP0CS)
					    & EP0CS_STALL) >> 1);
			/* weird is there should do right twist? */
			else
				ackval |= ((readb(udc->base + ep->reg_udccon)
					    & EPCON_STALL) >> 6);
		} else
			goto stall;

		/*back the status */
		/*FIXME not check whether ep0 fifo is empty */
		writel(ackval, udc->base + EP0INDATA_W0);
		writeb(2, udc->base + IN0BC);
		udc->ep0state = EP0_END_XFER;
		return;
	case USB_REQ_CLEAR_FEATURE:
		if ((u.r.bRequestType & 0x60) == 0x20) {
			UDC_DEBUG("hgl: is class request = 0x%x 0x%x\n", u.r.bRequestType, u.r.bRequest);
			break;
		}
		if ((u.r.bRequestType == USB_RECIP_DEVICE)
		    && (u.r.wValue == USB_DEVICE_REMOTE_WAKEUP)) {
			UDC_DEBUG("<CTRL> clear remote wakeup feature\n");
			udc->rwk = 0;	/*clear the remote wakeup character */
		} else if ((u.r.bRequestType == USB_RECIP_ENDPOINT)
			   && (u.r.wValue == USB_ENDPOINT_HALT)) {
			ep_num = u.r.wIndex & EP_ADDR_MASK;

			if (ep_num > (AOTG_UDC_NUM_ENDPOINTS - 1)
			    || u.r.wLength > 2)
				goto stall;
			ep = &udc->ep[ep_num];
			if ((ep_num != 0)
			    && (ep->bEndpointAddress != u.r.wIndex))
				goto stall;

			if (ep->bEndpointAddress == 0) {
				usb_clearbitsb(EP0CS_STALL, udc->base + EP0CS);
			} else {
				usb_clearbitsb(EPCON_STALL, udc->base + ep->reg_udccon);
				ep->stopped = 0;
				aotg_ep_reset(udc, ep->mask, ENDPRST_TOGRST);
				/*reset the ep toggle */
			}
			UDC_DEBUG("<CTRL> clear %s halt feature\n", ep->ep.name);
		} else
			goto stall;
		/*ACK the status stage */
		udc_handle_status(udc);
		return;
	case USB_REQ_SET_FEATURE:
		if ((u.r.bRequestType == USB_RECIP_DEVICE)) {
			switch (u.r.wValue) {
			case USB_DEVICE_REMOTE_WAKEUP:
				udc->rwk = 1;
				/*clear the remmote wakeup character */
				break;
			case USB_DEVICE_B_HNP_ENABLE:
				UDC_DEBUG("<UDC:()>:b_hnp_enable = 1\n");
				udc->gadget.b_hnp_enable = 1;
				//set_b_hnp_en();
				break;
			case USB_DEVICE_A_HNP_SUPPORT:
				UDC_DEBUG("<UDC:()>:a_hnp_support = 1\n");
				udc->gadget.a_hnp_support = 1;
				break;
			case USB_DEVICE_A_ALT_HNP_SUPPORT:
				UDC_DEBUG("<UDC:()>:a_alt_hnp_support = 1\n");
				udc->gadget.a_alt_hnp_support = 1;
				break;
			default:
				goto stall;
			}
		} else if ((u.r.bRequestType == USB_RECIP_ENDPOINT)
			   && (u.r.wValue == USB_ENDPOINT_HALT)) {
			ep_num = u.r.wIndex & EP_ADDR_MASK;

			if (ep_num > (AOTG_UDC_NUM_ENDPOINTS - 1)
			    || u.r.wLength > 2)
				goto stall;
			ep = &udc->ep[ep_num];
			if ((ep_num != 0)
			    && (ep->bEndpointAddress != u.r.wIndex))
				goto stall;
			if (ep->bEndpointAddress == 0) {
				usb_setbitsb(EP0CS_STALL, udc->base + EP0CS);
				udc->ep0state = EP0_STALL;
			} else
				usb_setbitsb(EPCON_STALL, udc->base + ep->reg_udccon);
		} else
			goto stall;

		/*ACK the status stage */
		udc_handle_status(udc);
		return;
	case USB_REQ_SET_ADDRESS:
		UDC_DEBUG("<CTRL>USB_REQ_SET_ADDRESS\n");
		udc->req_pending = 0;
		/*automatically reponse this request by hardware,
		   so hide it to software */
		return;
	case USB_REQ_SET_INTERFACE:
		UDC_DEBUG("<CTRL>USB_REQ_SET_INTERFACE\n");
		udc->req_config = 1;
		if (w_length != 0)
			goto stall;
		goto delegate;
		/*delegate to the upper gadget driver */
	case USB_REQ_SET_CONFIGURATION:
		UDC_DEBUG("<CTRL>USB_REQ_SET_CONFIGURATION\n");
		if (u.r.bRequestType == USB_RECIP_DEVICE) {
			if (w_length != 0)
				goto stall;
			udc->req_config = 1;
			if (w_value == 0) {
				/*enter address state and all endpoint
				   should be disabled except for endpoint0 */
				UDC_DEBUG("<CTRL>disable all ep\n");
				for (i = 1; i < AOTG_UDC_NUM_ENDPOINTS; i++)
					usb_clearbitsb(EPCON_VAL, udc->base + udc->ep[i].reg_udccon);
			} else {	/*enter configured state */
				UDC_DEBUG("<CTRL>enter configured state\n");
				for (i = 1; i < AOTG_UDC_NUM_ENDPOINTS; i++)
					usb_setbitsb(EPCON_VAL, udc->base + udc->ep[i].reg_udccon);
			}
		} else
			goto stall;
		/*delegate to the upper gadget driver */
		break;
	default:
		/*delegate to the upper gadget driver */
		break;
	}
	/*gadget drivers see class/vendor specific requests,
	 *{SET, GET}_{INTERFACE, DESCRIPTOR, CONFIGURATION},
	 *and more
	 *The gadget driver may return an error here,
	 *causing an immediate protocol stall.
	 *
	 *Else it must issue a response, either queueing a
	 *response buffer for the DATA stage, or halting ep0
	 *(causing a protocol stall, not a real halt).  A
	 *zero length buffer means no DATA stage.
	 *
	 *It's fine to issue that response after the setup()
	 *call returns.
	 */

      delegate:
	UDC_DEBUG("<CTRL>delegate\n");
	if (u.r.bRequestType & USB_DIR_IN)
		udc->ep0state = EP0_IN_DATA_PHASE;
	else
		udc->ep0state = EP0_OUT_DATA_PHASE;

	spin_unlock_irqrestore(&udc->lock, flags);
	status = udc->driver->setup(&udc->gadget, &u.r);	/*delegate */
	spin_lock_irqsave(&udc->lock, flags);
	*pflag=flags;
	if (status < 0) {
stall:
		printk("%p\n",udc->driver->setup);
		printk("<CTRL> req %02x.%02x protocol STALL, err  %d\n", u.r.bRequestType, u.r.bRequest, status);
		if (udc->req_config) {
			UDC_DEBUG("<CTRL>config change erro\n");
		}
		usb_setbitsb(EP0CS_STALL, udc->base + EP0CS);
		udc->req_pending = 0;
		udc->ep0state = EP0_STALL;
    }
    return;
}

static void handle_ep0_in(struct aotg_udc *dev)
{
	struct aotg_udc *udc = dev;
	struct aotg_ep *ep = &udc->ep[0];
	struct aotg_request *req;
	ep->udc_irqs++;
	if (list_empty(&ep->queue))
		req = NULL;
	else
		req = list_entry(ep->queue.next, struct aotg_request, queue);

	UDC_DEBUG("<CTRL>ep0in irq handler, state is %d, queue is %s\n",
		  udc->ep0state, (req == NULL) ? "empty" : "not empty");

	switch (udc->ep0state) {
	case EP0_IN_DATA_PHASE:
		if (req) {
			if (write_ep0_fifo(ep, req))
				udc->ep0state = EP0_END_XFER;
		}
		break;
	case EP0_END_XFER:
		/*ACK*/ 
		udc_handle_status(udc);
		/*cleanup */
		udc->req_pending = 0;
		udc->ep0state = EP0_WAIT_FOR_SETUP;
		if (req) {
			done(ep, req, 0);
			req = NULL;
		}
		break;
	case EP0_STALL:
		if (req) {
			done(ep, req, -ESHUTDOWN);
			req = NULL;
		}
		break;
	default:
		UDC_DEBUG("<CTRL>ep0in irq error, odd state %d\n", udc->ep0state);
	}
	usb_clearbitsb(1, udc->base + INxIEN);
}

/*
 *  EP0 related operations
 */
static inline int write_ep0_packet(struct aotg_udc *udc, struct aotg_request *req, unsigned max)
{
	u32 *buf;
	unsigned length, count;
	unsigned long addr = EP0INDATA_W0;

	buf = (u32 *) (req->req.buf + req->req.actual);
	prefetch(buf);
	/*how big will this packet be? */
	length = min(req->req.length - req->req.actual, max);
	req->req.actual += length;

	count = length / 4;	/*wirte in DWORD */
	if ((length % 4) != 0)
		count++;

	while (likely(count--)) {
		writel(*buf, udc->base + addr);
		buf++;
		addr += 4;
	}
	writeb(length, udc->base + IN0BC);
	/*arm IN EP0 for the next IN transaction */
	return length;
}

static int read_ep0_fifo(struct aotg_ep *ep, struct aotg_request *req)
{
	u8 *buf, byte;
	unsigned bufferspace, count, length;
	unsigned long addr;
	struct aotg_udc *udc;

	udc = ep->dev;
	if (readb(udc->base + EP0CS) & EP0CS_OUTBSY)	/*data is not ready */
		return 0;
	/*fifo can be accessed validly */
	else {
		buf = req->req.buf + req->req.actual;
		bufferspace = req->req.length - req->req.actual;

		length = count = readb(udc->base + OUT0BC);
		addr = EP0OUTDATA_W0;
		for (; count != 0; count--) {
			byte = readb(udc->base + addr);
			if (unlikely(bufferspace == 0)) {
				/*this happens when the driver's buffer
				 *is smaller than what the host sent.
				 *discard the extra data.
				 */
				if (req->req.status != -EOVERFLOW)
					UDC_DEBUG("%s overflow\n", ep->ep.name);
				req->req.status = -EOVERFLOW;
				break;
			} else {
				*buf++ = byte;
				req->req.actual++;
				bufferspace--;
				addr++;
				ep->dev->stats.read.bytes++;
			}
		}
	}

	UDC_DEBUG("ep0out %d bytes %s %d left %p\n", length,
		  (req->req.actual >= req->req.length) ? "/L" : "", req->req.length - req->req.actual, req);
	if ((req->req.actual >= req->req.length))
		return 1;
	return 0;
}

static int write_ep0_fifo(struct aotg_ep *ep, struct aotg_request *req)
{
	unsigned count;
	int is_last;
	struct aotg_udc *udc = ep->dev;

	count = write_ep0_packet(udc, req, EP0_PACKET_SIZE);
	ep->dev->stats.write.bytes += count;

	/*last packet must be a short packet or zlp */
	if (count != EP0_PACKET_SIZE)
		is_last = 1;
	else {
		if ((req->req.length != req->req.actual) || req->req.zero)
			is_last = 0;
		else
			is_last = 1;
	}
	UDC_DEBUG("ep0in %d bytes %s %d left %p\n", count, is_last ? "/L" : "", req->req.length - req->req.actual, req);
	usb_setbitsb(1, udc->base + INxIEN);
	return is_last;
}

static void handle_ep0_out(struct aotg_udc *dev)
{
	struct aotg_udc *udc = dev;
	struct aotg_ep *ep = &udc->ep[0];
	struct aotg_request *req;
	ep->udc_irqs++;

	if (list_empty(&ep->queue)) {
		/* empty queue */
		req = NULL;
	} else
		req = list_entry(ep->queue.next, struct aotg_request, queue);

	UDC_DEBUG("<CTRL>ep0out irq handler, state is %d, queue is %s\n",
		  udc->ep0state, (req == NULL) ? "empty" : "not empty");

	switch (udc->ep0state) {
	case EP0_OUT_DATA_PHASE:
		if (req) {
			if (read_ep0_fifo(ep, req)) {
				/*ACK*/ 
				udc_handle_status(udc);
				/*cleanup */
				udc->req_pending = 0;
				udc->ep0state = EP0_WAIT_FOR_SETUP;
				done(ep, req, 0);
				req = NULL;
			} else
				writeb(0, udc->base + OUT0BC);
			/*write OUT0BC with any value to
			   enable next OUT transaction */
		} else
			UDC_DEBUG("<CTRL>ep0out irq error, queue is empty but state is EP0_OUT_DATA_PHASE\n");
		/*never enter this branch */
		break;
	case EP0_STALL:
		if (req) {
			done(ep, req, -ESHUTDOWN);
			req = NULL;
		}
		break;
	default:
		UDC_DEBUG("<CTRL>ep0out irq error, odd state %d\n", udc->ep0state);
	}
}

static void udc_enable(struct aotg_udc *dev)
{
	struct aotg_udc *udc = dev;

	dev->ep0state = EP0_WAIT_FOR_SETUP;
	dev->stats.irqs = 0;
	dev->state = UDC_IDLE;
	dev->gadget.dev.parent->power.power_state = PMSG_ON;
	dev->gadget.dev.power.power_state = PMSG_ON;
	UDC_DEBUG("<UDC_ENABLE> %p, AOTG enters :%d state\n", dev, readb(udc->base + OTGSTATE));

	UDC_DEBUG("Pull up D+\n");
	/*pull up D+ to let  host  see us */
	dplus_up(udc);
}

static void udc_disable(struct aotg_udc *dev)
{
	struct aotg_udc *udc = dev;

	UDC_DEBUG("<UDC_DISABLE> %p,%d state\n", dev,readb(udc->base + OTGSTATE));
	/*FIX ME: clear some irqs */
	UDC_DEBUG("Pull down D+\n");
	/*Pull down D+ */
	dplus_down(udc);

	/*Clear software state */
	dev->ep0state = EP0_WAIT_FOR_SETUP;
	dev->gadget.speed = USB_SPEED_UNKNOWN;
	printk("%d,===============set speed unknown=============\n",__LINE__);
	dev->state = UDC_DISABLE;
}

int pullup(struct aotg_udc *udc, int is_active)
{
	is_active = is_active && udc->softconnect;
	UDC_DEBUG("<PULL_UP> %s\n", is_active ? "active" : "inactive");
	if (is_active)
		udc_enable(udc);
	else
		udc_disable(udc);

	return 0;
}

irqreturn_t aotg_udc_irq(int irq, void *data)
{
	struct aotg_udc *udc = (struct aotg_udc *)data;
	irqreturn_t retval = IRQ_HANDLED;
	u8 irqvector, external_irq, dma_irq;
	u8 otgint, otg_state;
	int dma_pend_cnt = 0;
	unsigned long flags = 0;
	unsigned long tmp_flag;

	external_irq = readb(udc->base + USBEIRQ);

	spin_lock_irqsave(&udc->lock, flags);
	if (!(external_irq & USBEIRQ_USBIRQ)) {
		dma_pend_cnt = aotg_udc_dma_irq_handler(udc, dma_irq);
		spin_unlock_irqrestore(&udc->lock, flags);
		if (dma_pend_cnt == 0)
			return IRQ_NONE;
		else
			return IRQ_HANDLED;
	}

	writeb(external_irq, udc->base + USBEIRQ);

	/* connect disconnect happened */
	if ((external_irq & 0x50) != 0) {
		printk("<UDC>external irq: %x\n", external_irq);

		if (external_irq & 0x40) {
			aotg_udc_pullup(&udc->gadget, 0);
			udc->state = UDC_UNKNOWN;
			if (udc->driver && udc->driver->disconnect) {
				udc->driver->disconnect(&udc->gadget);
			}
			udc->gadget.speed = USB_SPEED_UNKNOWN;
		}
		spin_unlock_irqrestore(&udc->lock, flags);
		return retval;
	}

	irqvector = readb(udc->base + IVECT);

	switch (irqvector) {
	case UIV_IDLE:
		if (  udc->gadget.speed != USB_SPEED_UNKNOWN)
		{
			printk("%p %p\n",udc->driver, udc->driver->disconnect);
			if (udc->driver && udc->driver->disconnect) {
				spin_unlock_irqrestore(&udc->lock, flags);
				udc->driver->disconnect(&udc->gadget);
				spin_lock_irqsave(&udc->lock, flags);
				//aotg_dev_plugout_msg(udc->id);
			}
		}
	case UIV_SRPDET:
	case UIV_LOCSOF:
	case UIV_VBUSERR:
	case UIV_PERIPH:
		otgint = readb(udc->base + OTGIEN) & readb(udc->base + OTGIRQ);
		writeb(otgint, udc->base + OTGIRQ);
		otg_state = readb(udc->base + OTGSTATE);
		UDC_DEBUG("OTG_STATE is %x\n", otg_state);
		switch (otgint) {
		case OTGIRQ_PERIPH:
UDC_DBG_ERR;
			usb_setbitsb(USBIEN_URES | USBIEN_HS | USBIEN_SUDAV | USBIEN_SUSP, udc->base + USBIEN);
			udc->state = UDC_ACTIVE;
			udc->disconnect = 0;
			break;
		case OTGIRQ_IDLE:
UDC_DBG_ERR;
			UDC_DEBUG("Enter B-IDLE\n");
			udc->state = UDC_IDLE;
			break;
		default:
UDC_DBG_ERR;
			break;

		}
		UDC_DEBUG("UIV_OTG_IRQ 0x%x\n", otgint);
		break;
	case UIV_USBRESET:
		if (  udc->gadget.speed != USB_SPEED_UNKNOWN)
		{
			if (udc->driver && udc->driver->disconnect) {
				spin_unlock_irqrestore(&udc->lock, flags);
				udc->driver->disconnect(&udc->gadget);
				spin_lock_irqsave(&udc->lock, flags);
			}
		}
		writeb(0xdf, udc->base + USBIRQ);
		writeb(0xff, udc->base + OTGIRQ);
		writeb(0xff, udc->base + INxIRQ);
		writeb(0xff, udc->base + OUTxIRQ);

		/* reset dma channel */
		aotg_epin_dma_reset(udc);
		UDC_DEBUG("gadget %s, USB reset done\n", udc->driver->driver.name);

		/*when bus reset, we assume the current speed is FS */
		udc->gadget.speed = USB_SPEED_FULL;
		udc->gadget.b_hnp_enable = 0;
		udc->gadget.a_hnp_support = 0;
		udc->gadget.a_alt_hnp_support = 0;
		udc->highspeed = 0;
		udc_ep_packet_config(USB_SPEED_FULL, udc);
		mdelay(4);
		udc->reset_cnt++;
		break;
	case UIV_HSPEED:
		writeb(USBIRQ_HS, udc->base + USBIRQ);
		udc->gadget.speed = USB_SPEED_HIGH;
		udc->highspeed = 1;
		udc_ep_packet_config(USB_SPEED_HIGH, udc);
#if 0
		if (udc->reset_cnt > 2) {
			usb_setbitsb(0x80, udc->base + BKDOOR);
		}
#endif
		break;
	case UIV_SUSPEND:
UDC_DBG_ERR;
		writeb(USBIRQ_SUSP, udc->base + USBIRQ);
		udc->state = UDC_SUSPEND;
		udc->reset_cnt = 0;
#if 0
		aotg_udc_pullup(&udc->gadget, 0);
		udc->state = UDC_UNKNOWN;
		if (udc->driver && udc->driver->disconnect) {
			udc->driver->disconnect(&udc->gadget);
		}
		udc->gadget.speed = USB_SPEED_UNKNOWN;
		printk("%d,===============set speed unknown=============\n",__LINE__);
#endif
		break;
	case UIV_SUDAV:
		writeb(USBIRQ_SUDAV, udc->base + USBIRQ);
		tmp_flag = flags;
		handle_setup(udc,&tmp_flag);
		flags = tmp_flag;
		break;
	case UIV_EP0IN:
		writeb(1, udc->base + INxIRQ);
		handle_ep0_in(udc);
		break;
	case UIV_EP0OUT:
		writeb(1, udc + OUTxIRQ);
		handle_ep0_out(udc);
		break;
	case UIV_EP1IN:
	case UIV_EP2IN:
	case UIV_EP3IN:
	case UIV_EP4IN:
	case UIV_EP5IN:
		printk("no ep_in irq any more!\n");
		break;
	case UIV_EP1OUT:
	case UIV_EP2OUT:
	case UIV_EP3OUT:
	case UIV_EP4OUT:
	case UIV_EP5OUT:
		printk("no ep_out irq any more!\n");
		break;
	case UIV_SOF:
		UDC_DEBUG("<UDC> sof is come\n");
		break;
	default:
		UDC_DEBUG("The ivent is 0x%x.\n", irqvector);
		retval = IRQ_NONE;
	}

	spin_unlock_irqrestore(&udc->lock, flags);
	return retval;
}

void udc_reinit(struct aotg_udc *dev)
{
	unsigned i;

	/*device/ep0 records init */
	INIT_LIST_HEAD(&dev->gadget.ep_list);
	INIT_LIST_HEAD(&dev->gadget.ep0->ep_list);

	dev->ep0state = EP0_WAIT_FOR_SETUP;
	dev->gadget.speed = USB_SPEED_UNKNOWN;
	printk("%d,===============set speed unknown=============\n",__LINE__);
	dev->state = UDC_UNKNOWN;
	memset(&dev->stats, 0, sizeof(struct udc_stats));

	/*basic endpoint records init */
	for (i = 0; i < AOTG_UDC_NUM_ENDPOINTS; i++) {
		struct aotg_ep *ep = &dev->ep[i];

		if (i != 0)
			list_add_tail(&ep->ep.ep_list, &dev->gadget.ep_list);

		ep->ep.desc = NULL;
		ep->dev = dev;
		ep->stopped = 0;
		INIT_LIST_HEAD(&ep->queue);
		ep->udc_irqs = 0;
	}
}

void aotg_udc_endpoint_config(struct aotg_udc *udc)
{
	UDC_BULK_EP(1, "ep1out", USB_DIR_OUT | 1, EPCON_BUF_QUAD);
	UDC_BULK_EP(2, "ep1in", USB_DIR_IN | 1, EPCON_BUF_QUAD);
	UDC_BULK_EP(3, "ep2out", USB_DIR_OUT | 2, EPCON_BUF_QUAD);
	UDC_BULK_EP(4, "ep2in", USB_DIR_IN | 2, EPCON_BUF_QUAD);
	UDC_BULK_EP(5, "ep3out", USB_DIR_OUT | 3, EPCON_BUF_QUAD);
	/*UDC_INT_EP(6, "ep3in", USB_DIR_IN | 3, EPCON_BUF_QUAD);*/
	UDC_BULK_EP(6, "ep3in", USB_DIR_IN | 3, EPCON_BUF_QUAD);
	UDC_ISO_EP(7, "ep4in", USB_DIR_IN | 4, EPCON_BUF_QUAD);

	udc_ep_packet_config(USB_SPEED_FULL, udc);
}

struct aotg_udc memory = {
	.lock = __SPIN_LOCK_UNLOCKED(memory.lock),
	.gadget = {
		   .ops = &aotg_udc_ops,
		   .ep0 = &memory.ep[0].ep,
		   .max_speed = USB_SPEED_HIGH,
		   .speed		= USB_SPEED_UNKNOWN,
		   .name = udc_driver_name,
		   .dev = {
			   .init_name = "gadget",
			   },
		   },

	/*control endpoint */
	.ep[0] = {
		  .ep = {
			 .name = ep0name,
			 .ops = &aotg_ep_ops,
			 .maxpacket = EP0_PACKET_SIZE,
			 },
		  .dev = &memory,
		  .maxpacket = EP0_PACKET_SIZE,

		  .ring = NULL,
		  },

	/*bulk out endpoint */
	.ep[1] = {
		  .ep = {
			 .ops = &aotg_ep_ops,
			 },
		  .dev = &memory,
		  .bEndpointAddress = 1,
		  .mask = 1,
		  .reg_udccs = OUT1CS,
		  .reg_udccon = OUT1CON,
		  .reg_udcbc = OUT1BCL,
		  .reg_udcfifo = 0,
		  .reg_maxckl = HCIN1MAXPCKL,
		  .reg_fifostaddr = OUT1STARTADDRESS,

		  .reg_dmalinkaddr = HCIN1DMALINKADDR,
		  .reg_curaddr = HCIN1DMACURADDR,
		  .reg_dmactrl = HCIN1DMACTRL,
		  .reg_dmacomplete_cnt = HCIN1DMACOMPLETECNT,
		  .ring = NULL,

		  },

	/*bulk in endpoint */
	.ep[2] = {
		  .ep = {
			 .ops = &aotg_ep_ops,
			 },
		  .dev = &memory,
		  .bEndpointAddress = USB_DIR_IN | 1,
		  .mask = USB_UDC_IN_MASK | 1,
		  .reg_udccs = IN1CS,
		  .reg_udccon = IN1CON,
		  .reg_udcbc = IN1BCL,
		  .reg_udcfifo = 0,
		  .reg_maxckl = HCOUT1MAXPCKL,
		  .reg_fifostaddr = IN1STARTADDRESS,

		  .reg_dmalinkaddr = HCOUT1DMALINKADDR,
		  .reg_curaddr = HCOUT1DMACURADDR,
		  .reg_dmactrl = HCOUT1DMACTRL,
		  .reg_dmacomplete_cnt = HCOUT1DMACOMPLETECNT,
		  .ring = NULL,

		  },

	/*bulk out endpoint */
	.ep[3] = {
		  .ep = {
			 .ops = &aotg_ep_ops,
			 },
		  .dev = &memory,
		  .bEndpointAddress = 2,
		  .mask = 2,
		  .reg_udccs = OUT2CS,
		  .reg_udccon = OUT2CON,
		  .reg_udcbc = OUT2BCL,
		  .reg_udcfifo = 0,
		  .reg_maxckl = HCIN2MAXPCKL,
		  .reg_fifostaddr = OUT2STARTADDRESS,

		  .reg_dmalinkaddr = HCIN2DMALINKADDR,
		  .reg_curaddr = HCIN2DMACURADDR,
		  .reg_dmactrl = HCIN2DMACTRL,
		  .reg_dmacomplete_cnt = HCIN2DMACOMPLETECNT,
		  .ring = NULL,

		  },

	/*bulk in endpoint */
	.ep[4] = {
		  .ep = {
			 .ops = &aotg_ep_ops,
			 },
		  .dev = &memory,
		  .bEndpointAddress = USB_DIR_IN | 2,
		  .mask = USB_UDC_IN_MASK | 2,
		  .reg_udccs = IN2CS,
		  .reg_udccon = IN2CON,
		  .reg_udcbc = IN2BCL,
		  .reg_udcfifo = 0,
		  .reg_maxckl = HCOUT2MAXPCKL,
		  .reg_fifostaddr = IN2STARTADDRESS,

		  .reg_dmalinkaddr = HCOUT2DMALINKADDR,
		  .reg_curaddr = HCOUT2DMACURADDR,
		  .reg_dmactrl = HCOUT2DMACTRL,
		  .reg_dmacomplete_cnt = HCOUT2DMACOMPLETECNT,
		  .ring = NULL,

		  },

	/*bulk out endpoint */
	.ep[5] = {
		  .ep = {
			 .ops = &aotg_ep_ops,
			 },
		  .dev = &memory,
		  .bEndpointAddress = 3,
		  .mask = 3,
		  .reg_udccs = OUT3CS,
		  .reg_udccon = OUT3CON,
		  .reg_udcbc = OUT3BCL,
		  .reg_udcfifo = 0,
		  .reg_maxckl = HCIN3MAXPCKL,
		  .reg_fifostaddr = OUT3STADDR,

		  .reg_dmalinkaddr = HCIN3DMALINKADDR,
		  .reg_curaddr = HCIN3DMACURADDR,
		  .reg_dmactrl = HCIN3DMACTRL,
		  .reg_dmacomplete_cnt = HCIN3DMACOMPLETECNT,
		  .ring = NULL,

		  },

	/* interupt in */
	.ep[6] = {
		  .ep = {
			 .ops = &aotg_ep_ops,
			 },
		  .dev = &memory,
		  .bEndpointAddress = USB_DIR_IN | 3,
		  .mask = USB_UDC_IN_MASK | 3,
		  .reg_udccs = IN3CS,
		  .reg_udccon = IN3CON,
		  .reg_udcbc = IN3BCL,
		  .reg_udcfifo = 0,
		  .reg_maxckl = HCOUT3MAXPCKL,
		  .reg_fifostaddr = IN3STADDR,

		  .reg_dmalinkaddr = HCOUT3DMALINKADDR,
		  .reg_curaddr = HCOUT3DMACURADDR,
		  .reg_dmactrl = HCOUT3DMACTRL,
		  .reg_dmacomplete_cnt = HCOUT3DMACOMPLETECNT,
		  .ring = NULL,

		  },

	/*iso in endpoint */
	.ep[7] = {
		  .ep = {
			 .ops = &aotg_ep_ops,
			 },
		  .dev = &memory,
		  .bEndpointAddress = USB_DIR_IN | 4,
		  .mask = USB_UDC_IN_MASK | 4,
		  .reg_udccs = IN4CS,
		  .reg_udccon = IN4CON,
		  .reg_udcbc = IN4BCL,
		  .reg_udcfifo = 0,
		  .reg_maxckl = HCOUT4MAXPCKL,
		  .reg_fifostaddr = IN4STADDR,

		  .reg_dmalinkaddr = HCOUT4DMALINKADDR,
		  .reg_curaddr = HCOUT4DMACURADDR,
		  .reg_dmactrl = HCOUT4DMACTRL,
		  .reg_dmacomplete_cnt = HCOUT4DMACOMPLETECNT,
		  .ring = NULL,

		  },

	/*bulk out endpoint */
	.ep[8] = {
		  .ep = {
			 .ops = &aotg_ep_ops,
			 },
		  .dev = &memory,
		  .bEndpointAddress = 4,
		  .mask = 4,
		  .reg_udccs = OUT4CS,
		  .reg_udccon = OUT4CON,
		  .reg_udcbc = OUT4BCL,
		  .reg_udcfifo = 0,
		  .reg_maxckl = HCIN4MAXPCKL,
		  .reg_fifostaddr = OUT4STADDR,

		  .reg_dmalinkaddr = HCIN4DMALINKADDR,
		  .reg_curaddr = HCIN4DMACURADDR,
		  .reg_dmactrl = HCIN4DMACTRL,
		  .reg_dmacomplete_cnt = HCIN4DMACOMPLETECNT,
		  .ring = NULL,

		  },
	/*bulk in endpoint */
	.ep[9] = {
		  .ep = {
			 .ops = &aotg_ep_ops,
			 },
		  .dev = &memory,
		  .bEndpointAddress = USB_DIR_IN | 5,
		  .mask = USB_UDC_IN_MASK | 5,
		  .reg_udccs = IN5CS,
		  .reg_udccon = IN5CON,
		  .reg_udcbc = IN5BCL,
		  .reg_udcfifo = 0,
		  .reg_maxckl = HCOUT5MAXPCKL,
		  .reg_fifostaddr = IN5STADDR,

		  .reg_dmalinkaddr = HCOUT5DMALINKADDR,
		  .reg_curaddr = HCOUT5DMACURADDR,
		  .reg_dmactrl = HCOUT5DMACTRL,
		  .reg_dmacomplete_cnt = HCOUT5DMACOMPLETECNT,
		  .ring = NULL,

		  },

	/*bulk out endpoint */
	.ep[10] = {
		   .ep = {
			  .ops = &aotg_ep_ops,
			  },
		   .dev = &memory,
		   .bEndpointAddress = 5,
		   .mask = 5,
		   .reg_udccs = OUT5CS,
		   .reg_udccon = OUT5CON,
		   .reg_udcbc = OUT5BCL,
		   .reg_udcfifo = 0,
		   .reg_maxckl = HCIN5MAXPCKL,
		   .reg_fifostaddr = OUT5STADDR,

		  .reg_dmalinkaddr = HCIN5DMALINKADDR,
		  .reg_curaddr = HCIN5DMACURADDR,
		  .reg_dmactrl = HCIN5DMACTRL,
		  .reg_dmacomplete_cnt = HCIN5DMACOMPLETECNT,
		  .ring = NULL,

		  },
};

static int __exit aotg_udc_remove(struct platform_device *pdev)
{
	struct aotg_udc *udc = platform_get_drvdata(pdev);

	usb_del_gadget_udc(&udc->gadget);
	usb_gadget_unregister_driver(udc->driver);
	free_irq(udc->irq, udc);
	udc->transceiver = NULL;
	platform_set_drvdata(pdev, NULL);
	acts_udc_controller = NULL;
	aotg_clk_enable(pdev->id, 0);
	iounmap(udc->base);
	release_mem_region(udc->rsrc_start, udc->rsrc_len);
	owl_powergate_power_off(pdev->id == 0 ? OWL_POWERGATE_USB2_0 : OWL_POWERGATE_USB2_1);

	return 0;
}

struct platform_driver aotg_udc_driver = {
	.driver = {
		.name = "aotg_udc",
		.owner = THIS_MODULE,
		.of_match_table = aotg_of_match,
	},
	.probe = aotg_probe,
	.remove = __exit_p(aotg_udc_remove),
//	.suspend = aotg_udc_suspend,
//	.resume = aotg_udc_resume,
};
