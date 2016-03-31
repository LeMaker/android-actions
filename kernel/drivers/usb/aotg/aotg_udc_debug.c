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
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/otg.h>
#include <linux/usb/hcd.h>
#include <asm/system.h>
#include <linux/timer.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/../../fs/proc/internal.h>

#include <mach/hardware.h>
#include <linux/gpio.h>

#include "aotg_udc_debug.h"
#include "aotg.h"
#include "aotg_regs.h"
#include "aotg_plat_data.h"


unsigned int aotg_udc_trace_onff = 0;

#define AOTG_DEBUG_INFO_CNT	30
static int aotg_dbg_idx = 0;
static char aotg_dbg_strings[AOTG_DEBUG_INFO_CNT][8];
static char* aotg_dbg_infoh[AOTG_DEBUG_INFO_CNT];
static char* aotg_dbg_info0[AOTG_DEBUG_INFO_CNT];
static unsigned int aotg_dbg_info1[AOTG_DEBUG_INFO_CNT];
static unsigned int aotg_dbg_info2[AOTG_DEBUG_INFO_CNT];
static unsigned int aotg_dbg_info3[AOTG_DEBUG_INFO_CNT];

void aotg_dbg_put_info(char *info_h, char *info0, unsigned int info1, unsigned int info2, unsigned int info3)
{
	unsigned long flags;

	local_irq_save(flags);
	strncpy(&aotg_dbg_strings[aotg_dbg_idx][0], info0, 7);
	aotg_dbg_strings[aotg_dbg_idx][7] = 0;
	aotg_dbg_infoh[aotg_dbg_idx] = info_h;
	aotg_dbg_info0[aotg_dbg_idx] = info0;
	aotg_dbg_info1[aotg_dbg_idx] = info1;
	aotg_dbg_info2[aotg_dbg_idx] = info2;
	aotg_dbg_info3[aotg_dbg_idx] = info3;
	aotg_dbg_idx++;
	if (aotg_dbg_idx >= AOTG_DEBUG_INFO_CNT)
		aotg_dbg_idx = 0;
	local_irq_restore(flags);
	return;
}

void aotg_udc_dbg_output_info(void)
{
	int i;
	unsigned long flags;

	local_irq_save(flags);

	for (i = 0; i < AOTG_DEBUG_INFO_CNT; i++) {
		printk("i:%d  ", i);
		//printk("strins:%s: ", &aotg_dbg_strings[aotg_dbg_idx][0]);
		printk("info_h:%s: ", aotg_dbg_infoh[aotg_dbg_idx]);
		printk("info0:%s: ", aotg_dbg_info0[aotg_dbg_idx]);
		printk("info1:%d, 0x%x; ", aotg_dbg_info1[aotg_dbg_idx], aotg_dbg_info1[aotg_dbg_idx]);
		printk("info2:%d, 0x%x; ", aotg_dbg_info2[aotg_dbg_idx], aotg_dbg_info2[aotg_dbg_idx]);
		printk("info3:%d, 0x%x; \n", aotg_dbg_info3[aotg_dbg_idx], aotg_dbg_info3[aotg_dbg_idx]);
		aotg_dbg_idx++;
		if (aotg_dbg_idx >= AOTG_DEBUG_INFO_CNT)
			aotg_dbg_idx = 0;
	}
	local_irq_restore(flags);
	return;
}

void aotg_output_current_ep_status(struct seq_file *m)
{
	struct aotg_ep *ep;
	struct aotg_request *req;
	struct usb_endpoint_descriptor *desc;
	struct aotg_trb *trb;
	struct udc_ring *ring;
	struct aotg_udc *udc;
	int i;

	printk("%s %d\n",__func__,__LINE__);
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
		printk("reg_dmalinkaddr:(%x)%x,reg_curaddr:(%x)%x,reg_dmactrl:(%x)%x,reg_dmacomplete_cnt:(%x)%x\n",
			   ep->reg_dmalinkaddr,readl(udc->base + ep->reg_dmalinkaddr),
			   ep->reg_curaddr,readl(udc->base + ep->reg_curaddr),
			   ep->reg_dmactrl,readl(udc->base + ep->reg_dmactrl),
			   ep->reg_dmacomplete_cnt,readl(udc->base + ep->reg_dmacomplete_cnt));
		
		ring = ep->ring;
		if (!ring)
			continue;
		printk("===========ring infos==============:\nfirst_trb:%p,last_trb:%p,enqueue_trb:%p,cur_trb:%p,trb_dma:%d,num_trbs_free:%d\n",
				ring->first_trb,ring->last_trb,ring->enqueue_trb,ring->cur_trb,ring->trb_dma,ring->num_trbs_free);
		trb = ring->first_trb;
		for(trb=ring->first_trb; trb < ring->last_trb; trb++)	{
			printk("hw_buf_ptr:%x,hw_buf_len:%x,hw_buf_remain:%x,hw_token:%x\n",
					trb->hw_buf_ptr,trb->hw_buf_len,trb->hw_buf_remain,trb->hw_token);
		}
						
		list_for_each_entry(req, &ep->queue, queue) {
			printk("req %p len %d/%d buf %p\n",
				   &req->req, req->req.actual, req->req.length, req->req.buf);
		}
	}

	return;
}

//void acts_udc_test_output(char * info, int cnt)
//{
	//return;
//}
//EXPORT_SYMBOL(acts_udc_test_output);


static struct proc_dir_entry *acts_udc_pde = NULL; 

int acts_udc_proc_show(struct seq_file *s, void *unused)
{
	//seq_printf(s, "aotg_udc_trace_onff: %d\n", aotg_udc_trace_onff);
	aotg_output_current_ep_status(s);
	return 0;
}

static int acts_udc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, acts_udc_proc_show, PDE_DATA(inode));
}

void demp_regs(struct aotg_udc *udc)
{
	printk(" USBEIRQ(0x%p) : 0x%X\n",
		udc->base + USBEIRQ, readb(udc->base + USBEIRQ));
	printk(" USBEIEN(0x%p) : 0x%X\n",
		udc->base + USBEIEN, readb(udc->base + USBEIEN));
	printk(" USBIRQ(0x%p) : 0x%X\n",
		udc->base + USBIRQ, readb(udc->base + USBIRQ));
	printk(" USBIEN(0x%p) : 0x%X\n",
		udc->base + USBIEN, readb(udc->base + USBIEN));
}

void aotg_dbg_udc_regs(struct aotg_udc *udc)
{
	struct aotg_plat_data *data = udc->port_specific;
	/*int i = 0;*/
	printk("usbpll:%x,devrst:%x\n",usb_readl(data->usbpll), usb_readl(data->devrst));
#if 0

	do {
		printk(" USB reg(0x%p):0x%X  ", udc->base + i, usb_readl(udc->base + i));
		i += 4;
		printk(":0x%X ", readl(udc->base + i));
		i += 4;
		printk(":0x%X ", readl(udc->base + i));
		i += 4;
		printk(":0x%X ", readl(udc->base + i));
		i += 4;
		printk("\n");
	} while (i < 0x600);
#endif
	dev_info(udc->dev, "============== aotg regs ==================\n");

  printk("usbecs:0x%X ", readl(data->usbecs));
#if 1
	printk(" USBEIRQ(0x%p) : 0x%X\n",
            udc->base + USBEIRQ, readb(udc->base + USBEIRQ));
	printk(" USBEIEN(0x%p) : 0x%X\n",
            udc->base + USBEIEN, readb(udc->base + USBEIEN));
	printk(" SRPCTRL(0x%p) : 0x%X\n",
            udc->base + SRPCTRL, readb(udc->base + SRPCTRL));

	printk("HCINxSHORTPCKIRQ0(0x%p) : 0x%X\n",
            udc->base + HCINxSHORTPCKIRQ0 , readb(udc->base + HCINxSHORTPCKIRQ0 ));
	printk("HCINxSHORTPCKIRQ1 (0x%p) : 0x%X\n",
            udc->base + HCINxSHORTPCKIRQ1 , readb(udc->base + HCINxSHORTPCKIRQ1 ));
	printk("HCINxSHORTPCKIEN0 (0x%p) : 0x%X\n",
            udc->base + HCINxSHORTPCKIEN0 , readb(udc->base + HCINxSHORTPCKIEN0 ));
	printk("HCINxSHORTPCKIEN1 (0x%p) : 0x%X\n",
            udc->base + HCINxSHORTPCKIEN1 , readb(udc->base + HCINxSHORTPCKIEN1 ));

	printk("HCINxERRIRQ0(0x%p) : 0x%X\n",
            udc->base + HCINxERRIRQ0, readw(udc->base + HCINxERRIRQ0));

	printk(" OTGIRQ(0x%p) : 0x%X\n",
            udc->base + OTGIRQ, readb(udc->base + OTGIRQ));
	printk(" OTGSTATE(0x%p) : 0x%X\n",
            udc->base + OTGSTATE, readb(udc->base + OTGSTATE));
	printk(" OTGCTRL(0x%p) : 0x%X\n",
            udc->base + OTGCTRL, readb(udc->base + OTGCTRL));
	printk(" OTGSTATUS(0x%p) : 0x%X\n",
            udc->base + OTGSTATUS, readb(udc->base + OTGSTATUS));
	printk(" OTGIEN(0x%p) : 0x%X\n",
            udc->base + OTGIEN, readb(udc->base + OTGIEN));
	printk("\n");
	printk(" BKDOOR(0x%p) : 0x%X\n",
            udc->base + BKDOOR, readb(udc->base + BKDOOR));
	printk(" USBIRQ(0x%p) : 0x%X\n",
            udc->base + USBIRQ, readb(udc->base + USBIRQ));
	printk(" USBIEN(0x%p) : 0x%X\n",
            udc->base + USBIEN, readb(udc->base + USBIEN));
	printk("\n");
#endif

	printk("HCINxPNGIEN0:%x\n", (u32)readb(udc->base + HCINxPNGIEN0));
	printk(" HCIN1DMACOUNTER(0x%p) : 0x%X\n",
            udc->base + HCIN1DMACOUNTER, readb(udc->base + HCIN1DMACOUNTER));
	printk(" HCIN2DMASTADDR(0x%p) : 0x%X\n",
            udc->base + HCIN2DMASTADDR, readb(udc->base + HCIN2DMASTADDR));

	printk(" HCOUTxIRQ0(0x%p) : 0x%X\n",
            udc->base + HCOUTxIRQ0, readb(udc->base + HCOUTxIRQ0));
	printk(" HCOUTxIEN0(0x%p) : 0x%X\n",
            udc->base + HCOUTxIEN0, readb(udc->base + HCOUTxIEN0));
	printk(" HCINxIRQ0(0x%p) : 0x%X\n",
            udc->base + HCINxIRQ0, readb(udc->base + HCINxIRQ0));
	printk(" HCINxIEN0(0x%p) : 0x%X\n",
            udc->base + HCINxIEN0, readb(udc->base + HCINxIEN0));
	printk("\n");
#if 1
	printk(" HCIN0BC(0x%p) : 0x%X\n",
            udc->base + HCIN0BC, readb(udc->base + HCIN0BC));
	printk(" EP0CS(0x%p) : 0x%X\n",
            udc->base + EP0CS, readb(udc->base + EP0CS));
	printk(" HCOUT0BC(0x%p) : 0x%X\n",
            udc->base + HCOUT0BC, readb(udc->base + HCOUT0BC));
	printk(" HCEP0CTRL(0x%p) : 0x%X\n",
            udc->base + HCEP0CTRL, readb(udc->base + HCEP0CTRL));
	printk("\n");
	printk(" HCIN1BC(0x%p) : 0x%X\n",
            udc->base + HCIN1BCL, readw(udc->base + HCIN1BCL));
	printk(" HCIN1CON(0x%p) : 0x%X\n",
            udc->base + HCIN1CON, readb(udc->base + HCIN1CON));
	printk(" HCIN1CS(0x%p) : 0x%X\n",
            udc->base + HCIN1CS, readb(udc->base + HCIN1CS));
	printk(" HCIN1CTRL(0x%p) : 0x%X\n",
            udc->base + HCIN1CTRL, readb(udc->base + HCIN1CTRL));
	printk(" HCIN2BC(0x%p) : 0x%X\n",
            udc->base + HCIN2BCL, readw(udc->base + HCIN2BCL));
	printk(" HCIN2CON(0x%p) : 0x%X\n",
            udc->base + HCIN2CON, readb(udc->base + HCIN2CON));
	printk(" HCIN2CS(0x%p) : 0x%X\n",
            udc->base + HCIN2CS, readb(udc->base + HCIN2CS));
	printk(" HCIN2CTRL(0x%p) : 0x%X\n",
            udc->base + HCIN2CTRL, readb(udc->base + HCIN2CTRL));
	printk("\n");
	printk(" HCIN4DMASTADDR(0x%p) : 0x%X\n",
            udc->base + HCIN4DMASTADDR, readw(udc->base + HCIN4DMASTADDR));
	printk(" HCIN4DMACOUNTER(0x%p) : 0x%X\n",
            udc->base + HCIN4DMACOUNTER, readw(udc->base + HCIN4DMACOUNTER));
	//printk(" HCINCTRL(0x%p) : 0x%X\n", udc->base + HCINCTRL, usb_readb(udc->base + HCINCTRL));
	printk("\n");
#endif
	printk(" HCOUT1BC(0x%p) : 0x%X\n",
            udc->base + HCOUT1BCL, readw(udc->base + HCOUT1BCL));
	printk(" HCOUT1CON(0x%p) : 0x%X\n",
            udc->base + HCOUT1CON, readb(udc->base + HCOUT1CON));
	printk(" HCOUT1CS(0x%p) : 0x%X\n",
            udc->base + HCOUT1CS, readb(udc->base + HCOUT1CS));
	printk(" HCOUT1CTRL(0x%p) : 0x%X\n",
            udc->base + HCOUT1CTRL, readb(udc->base + HCOUT1CTRL));
	printk(" HCOUT2BC(0x%p) : 0x%X\n",
            udc->base + HCOUT2BCL, readw(udc->base + HCOUT2BCL));
	printk(" HCOUT2CON(0x%p) : 0x%X\n",
            udc->base + HCOUT2CON, readb(udc->base + HCOUT2CON));
	printk(" HCOUT2CS(0x%p) : 0x%X\n",
            udc->base + HCOUT2CS, readb(udc->base + HCOUT2CS));
	printk(" HCOUT2CTRL(0x%p) : 0x%X\n",
            udc->base + HCOUT2CTRL, readb(udc->base + HCOUT2CTRL));
	printk("\n");
	printk("\n");
	return;
}

static ssize_t acts_udc_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char c = 'n';

	if (count) 
		if (get_user(c, buf))
			return -EFAULT;

	if ((c >= '0') && (c <= '3'))
		aotg_udc_trace_onff = c - '0';

	if (c == 'a') {
		printk("aotg udc 0 add\n");
		aotg_udc_register(0);
	}

	if (c == 'b') {
		printk("aotg udc 0 remove\n");
		aotg_udc_unregister(0);
	}

	if (c == 'c') {
		printk("aotg udc 1 add\n");
		aotg_udc_register(1);
	}

	if (c == 'd') {
		printk("aotg udc 1 remove\n");
		aotg_udc_unregister(1);
	}

	if (c == 'e') 
		aotg_udc_dbg_output_info();

	if (c == 'f') 
		aotg_udc_trace_onff = 0;

	if (c == 'g')
		if (acts_udc_controller)
			aotg_dbg_udc_regs(acts_udc_controller); 


	return count;
}

static const struct file_operations acts_udc_proc_ops = {
	.open		= acts_udc_proc_open,
	.read		= seq_read,
	.write		= acts_udc_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void create_acts_udc_proc(void)
{
	acts_udc_pde = proc_create_data("acts_udc", 0, NULL, &acts_udc_proc_ops, acts_udc_pde);
	return;
}

void remove_acts_udc_proc(void)
{
	if (acts_udc_pde) {
		remove_proc_entry("acts_udc", NULL);
		acts_udc_pde = NULL;
	}
	return;
}

void aotg_dbg_ep_queue_list(struct aotg_ep *ep)
{
	struct aotg_request *req;

	printk("ep %s queue list:\n", ep->name);
	list_for_each_entry(req, &ep->queue, queue) {
		printk("req:%p\n", req);
	}
	return;
}
