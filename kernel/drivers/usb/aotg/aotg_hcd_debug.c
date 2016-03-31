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

#include "aotg_hcd.h"
#include "aotg_regs.h"
#include "aotg_plat_data.h"
//#include "aotg_dma.h"
#include "aotg_hcd_debug.h"


char aotg_hcd_proc_sign = 'n';
unsigned int aotg_trace_onff = 0;

void aotg_dbg_proc_output_ep(void)
{
	return;
}

void aotg_dbg_output_info(void)
{
	return;
}

void aotg_dbg_put_q(struct aotg_queue *q, unsigned int num, unsigned int type, unsigned int len)
{
	return;
}

void aotg_dbg_finish_q(struct aotg_queue *q)
{
	return;
}

void aotg_dump_ep_reg(struct aotg_hcd *acthcd, int ep_index, int is_out)
{
	int index_multi = ep_index - 1;
	if (NULL == acthcd) {
		ACT_HCD_ERR
		return;
	}

	if (ep_index > 15) {
		printk("ep_index : %d too big, err!\n", ep_index);
		return;
	}	
	
	printk("=== dump hc-%s ep%d reg info ===\n",
		is_out ? "out" : "in", ep_index);

	if (ep_index == 0) {
		printk(" HCIN0BC(0x%p) : 0x%X\n",
	            acthcd->base + HCIN0BC, usb_readb(acthcd->base + HCIN0BC));
		printk(" EP0CS(0x%p) : 0x%X\n",
	            acthcd->base + EP0CS, usb_readb(acthcd->base + EP0CS));
		printk(" HCOUT0BC(0x%p) : 0x%X\n",
	            acthcd->base + HCOUT0BC, usb_readb(acthcd->base + HCOUT0BC));
		printk(" HCEP0CTRL(0x%p) : 0x%X\n",
	            acthcd->base + HCEP0CTRL, usb_readb(acthcd->base + HCEP0CTRL));
		printk(" HCIN0ERR(0x%p) : 0x%X\n",
				acthcd->base + HCIN0ERR, usb_readb(acthcd->base + HCIN0ERR));
		return;
	}
	
	if (is_out) {
		printk(" HCOUT%dBC(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1BC + index_multi * 0x8,
			usb_readw(acthcd->base + HCOUT1BC+ index_multi *0x8));	
		printk(" HCOUT%dCON(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1CON + index_multi * 0x8,
			usb_readb(acthcd->base + HCOUT1CON + index_multi *0x8));
		printk(" HCOUT%dCS(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1CS + index_multi * 0x8,
			usb_readb(acthcd->base + HCOUT1CS + index_multi *0x8));
		printk(" HCOUT%dCTRL(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1CTRL + index_multi * 0x4,
			usb_readb(acthcd->base + HCOUT1CTRL + index_multi *0x4));
		printk(" HCOUT%dERR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1ERR + index_multi * 0x4,
			usb_readb(acthcd->base + HCOUT1ERR + index_multi *0x4));
		printk(" HCOUT%dSTADDR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1STADDR + index_multi * 0x4,
			usb_readl(acthcd->base + HCOUT1STADDR + index_multi * 0x4));
		printk(" HCOUT%dMAXPCK(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1MAXPCK + index_multi * 0x2,
			usb_readw(acthcd->base + HCOUT1MAXPCK + index_multi * 0x2));
		
		printk(" HCOUT%dDMASTADDR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1DMASTADDR + index_multi * 0x8,
			usb_readl(acthcd->base + HCOUT1DMASTADDR + index_multi * 0x8));
		printk(" HCOUT%dDMACOUNTER(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCOUT1DMACOUNTER + index_multi * 0x8,
			usb_readl(acthcd->base + HCOUT1DMACOUNTER + index_multi * 0x8));
	} else {
		printk(" HCIN%dBC(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1BC + index_multi * 0x8,
			usb_readw(acthcd->base + HCIN1BC + index_multi *0x8));
		printk(" HCIN%dCON(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1CON+ index_multi * 0x8,
			usb_readb(acthcd->base + HCIN1CON+ index_multi *0x8));
		printk(" HCIN%dCS(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1CS + index_multi * 0x8,
			usb_readb(acthcd->base + HCIN1CS + index_multi *0x8));
		printk(" HCIN%dCS(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1CTRL + index_multi * 0x4,
			usb_readb(acthcd->base + HCIN1CTRL+ index_multi *0x4));
		printk(" HCIN%dERR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1ERR + index_multi * 0x4,
			usb_readb(acthcd->base + HCIN1ERR + index_multi *0x4));
		printk(" HCIN%dSTADDR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1STADDR + index_multi * 0x4,
			usb_readl(acthcd->base + HCIN1STADDR + index_multi *0x4));
		printk(" HCIN%dMAXPCK(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1MAXPCK + index_multi * 0x2,
			usb_readw(acthcd->base + HCIN1MAXPCK + index_multi * 0x2));

		printk(" HCIN%dDMASTADDR(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1DMASTADDR + index_multi * 0x8,
			usb_readl(acthcd->base + HCIN1DMASTADDR + index_multi * 0x8));
		printk(" HCIN%dDMACOUNTER(0x%p) : 0x%X\n", ep_index,
			acthcd->base + HCIN1DMACOUNTER + index_multi * 0x8,
			usb_readl(acthcd->base + HCIN1DMACOUNTER + index_multi * 0x8));
	}

}

#ifdef AOTG_REG_DUMP
void aotg_dbg_regs(struct aotg_hcd *acthcd)
{
    struct aotg_plat_data *data = acthcd->port_specific;
#if 1
	int i = 0;

	do {
		printk(" USB reg(0x%p):0x%X  ", acthcd->base + i, usb_readl(acthcd->base + i));
		i += 4;
		printk(":0x%X ", usb_readl(acthcd->base + i));
		i += 4;
		printk(":0x%X ", usb_readl(acthcd->base + i));
		i += 4;
		printk(":0x%X ", usb_readl(acthcd->base + i));
		i += 4;
		printk("\n");
	} while (i < 0x600);
#endif
	dev_info(acthcd->dev, "============== aotg regs ==================\n");

  printk("usbecs:0x%X ", usb_readl(data->usbecs));
#if 1
	printk(" USBEIRQ(0x%p) : 0x%X\n",
            acthcd->base + USBEIRQ, usb_readb(acthcd->base + USBEIRQ));
	printk(" USBEIEN(0x%p) : 0x%X\n",
            acthcd->base + USBEIEN, usb_readb(acthcd->base + USBEIEN));
	printk(" SRPCTRL(0x%p) : 0x%X\n",
            acthcd->base + SRPCTRL, usb_readb(acthcd->base + SRPCTRL));

	printk("HCINxSHORTPCKIRQ0(0x%p) : 0x%X\n",
            acthcd->base + HCINxSHORTPCKIRQ0 , usb_readb(acthcd->base + HCINxSHORTPCKIRQ0 ));
	printk("HCINxSHORTPCKIRQ1 (0x%p) : 0x%X\n",
            acthcd->base + HCINxSHORTPCKIRQ1 , usb_readb(acthcd->base + HCINxSHORTPCKIRQ1 ));
	printk("HCINxSHORTPCKIEN0 (0x%p) : 0x%X\n",
            acthcd->base + HCINxSHORTPCKIEN0 , usb_readb(acthcd->base + HCINxSHORTPCKIEN0 ));
	printk("HCINxSHORTPCKIEN1 (0x%p) : 0x%X\n",
            acthcd->base + HCINxSHORTPCKIEN1 , usb_readb(acthcd->base + HCINxSHORTPCKIEN1 ));

	printk("HCINxERRIRQ0(0x%p) : 0x%X\n",
            acthcd->base + HCINxERRIRQ0, usb_readw(acthcd->base + HCINxERRIRQ0));

	printk(" OTGIRQ(0x%p) : 0x%X\n",
            acthcd->base + OTGIRQ, usb_readb(acthcd->base + OTGIRQ));
	printk(" OTGSTATE(0x%p) : 0x%X\n",
            acthcd->base + OTGSTATE, usb_readb(acthcd->base + OTGSTATE));
	printk(" OTGCTRL(0x%p) : 0x%X\n",
            acthcd->base + OTGCTRL, usb_readb(acthcd->base + OTGCTRL));
	printk(" OTGSTATUS(0x%p) : 0x%X\n",
            acthcd->base + OTGSTATUS, usb_readb(acthcd->base + OTGSTATUS));
	printk(" OTGIEN(0x%p) : 0x%X\n",
            acthcd->base + OTGIEN, usb_readb(acthcd->base + OTGIEN));
	printk("\n");
	printk(" BKDOOR(0x%p) : 0x%X\n",
            acthcd->base + BKDOOR, usb_readb(acthcd->base + BKDOOR));
	printk(" USBIRQ(0x%p) : 0x%X\n",
            acthcd->base + USBIRQ, usb_readb(acthcd->base + USBIRQ));
	printk(" USBIEN(0x%p) : 0x%X\n",
            acthcd->base + USBIEN, usb_readb(acthcd->base + USBIEN));
	printk("\n");
#endif

	printk("HCINxPNGIEN0:%x\n", (u32)usb_readb(acthcd->base + HCINxPNGIEN0));
	printk(" HCIN1DMACOUNTER(0x%p) : 0x%X\n",
            acthcd->base + HCIN1DMACOUNTER, usb_readb(acthcd->base + HCIN1DMACOUNTER));
	printk(" HCIN2DMASTADDR(0x%p) : 0x%X\n",
            acthcd->base + HCIN2DMASTADDR, usb_readb(acthcd->base + HCIN2DMASTADDR));

	printk(" HCOUTxIRQ0(0x%p) : 0x%X\n",
            acthcd->base + HCOUTxIRQ0, usb_readb(acthcd->base + HCOUTxIRQ0));
	printk(" HCOUTxIEN0(0x%p) : 0x%X\n",
            acthcd->base + HCOUTxIEN0, usb_readb(acthcd->base + HCOUTxIEN0));
	printk(" HCINxIRQ0(0x%p) : 0x%X\n",
            acthcd->base + HCINxIRQ0, usb_readb(acthcd->base + HCINxIRQ0));
	printk(" HCINxIEN0(0x%p) : 0x%X\n",
            acthcd->base + HCINxIEN0, usb_readb(acthcd->base + HCINxIEN0));
	printk("\n");
#if 1
	printk(" HCIN0BC(0x%p) : 0x%X\n",
            acthcd->base + HCIN0BC, usb_readb(acthcd->base + HCIN0BC));
	printk(" EP0CS(0x%p) : 0x%X\n",
            acthcd->base + EP0CS, usb_readb(acthcd->base + EP0CS));
	printk(" HCOUT0BC(0x%p) : 0x%X\n",
            acthcd->base + HCOUT0BC, usb_readb(acthcd->base + HCOUT0BC));
	printk(" HCEP0CTRL(0x%p) : 0x%X\n",
            acthcd->base + HCEP0CTRL, usb_readb(acthcd->base + HCEP0CTRL));
	printk("\n");
	printk(" HCIN1BC(0x%p) : 0x%X\n",
            acthcd->base + HCIN1BCL, usb_readw(acthcd->base + HCIN1BCL));
	printk(" HCIN1CON(0x%p) : 0x%X\n",
            acthcd->base + HCIN1CON, usb_readb(acthcd->base + HCIN1CON));
	printk(" HCIN1CS(0x%p) : 0x%X\n",
            acthcd->base + HCIN1CS, usb_readb(acthcd->base + HCIN1CS));
	printk(" HCIN1CTRL(0x%p) : 0x%X\n",
            acthcd->base + HCIN1CTRL, usb_readb(acthcd->base + HCIN1CTRL));
	printk(" HCIN2BC(0x%p) : 0x%X\n",
            acthcd->base + HCIN2BCL, usb_readw(acthcd->base + HCIN2BCL));
	printk(" HCIN2CON(0x%p) : 0x%X\n",
            acthcd->base + HCIN2CON, usb_readb(acthcd->base + HCIN2CON));
	printk(" HCIN2CS(0x%p) : 0x%X\n",
            acthcd->base + HCIN2CS, usb_readb(acthcd->base + HCIN2CS));
	printk(" HCIN2CTRL(0x%p) : 0x%X\n",
            acthcd->base + HCIN2CTRL, usb_readb(acthcd->base + HCIN2CTRL));
	printk("\n");
	printk(" HCIN4DMASTADDR(0x%p) : 0x%X\n",
            acthcd->base + HCIN4DMASTADDR, usb_readw(acthcd->base + HCIN4DMASTADDR));
	printk(" HCIN4DMACOUNTER(0x%p) : 0x%X\n",
            acthcd->base + HCIN4DMACOUNTER, usb_readw(acthcd->base + HCIN4DMACOUNTER));
	//printk(" HCINCTRL(0x%p) : 0x%X\n", acthcd->base + HCINCTRL, usb_readb(acthcd->base + HCINCTRL));
	printk("\n");
#endif
	printk(" HCOUT1BC(0x%p) : 0x%X\n",
            acthcd->base + HCOUT1BCL, usb_readw(acthcd->base + HCOUT1BCL));
	printk(" HCOUT1CON(0x%p) : 0x%X\n",
            acthcd->base + HCOUT1CON, usb_readb(acthcd->base + HCOUT1CON));
	printk(" HCOUT1CS(0x%p) : 0x%X\n",
            acthcd->base + HCOUT1CS, usb_readb(acthcd->base + HCOUT1CS));
	printk(" HCOUT1CTRL(0x%p) : 0x%X\n",
            acthcd->base + HCOUT1CTRL, usb_readb(acthcd->base + HCOUT1CTRL));
	printk(" HCOUT2BC(0x%p) : 0x%X\n",
            acthcd->base + HCOUT2BCL, usb_readw(acthcd->base + HCOUT2BCL));
	printk(" HCOUT2CON(0x%p) : 0x%X\n",
            acthcd->base + HCOUT2CON, usb_readb(acthcd->base + HCOUT2CON));
	printk(" HCOUT2CS(0x%p) : 0x%X\n",
            acthcd->base + HCOUT2CS, usb_readb(acthcd->base + HCOUT2CS));
	printk(" HCOUT2CTRL(0x%p) : 0x%X\n",
            acthcd->base + HCOUT2CTRL, usb_readb(acthcd->base + HCOUT2CTRL));
	printk("\n");
	printk("\n");
	return;
}

#else	/* AOTG_REG_DUMP */

void aotg_dbg_regs(struct aotg_hcd *acthcd)
{
	/* fpga5209 dump */
	printk("dump gl5209 reg\n");

/*
	printk("CMU_USBPLL(0x%p) : 0x%X\n",
		acthcd->base + CMU_USBPLL, usb_readl(acthcd->base + CMU_USBPLL));
	printk("CMU_DEVRST1(0x%p) : 0x%X\n",
		acthcd->base + CMU_DEVRST1, usb_readl(acthcd->base + CMU_DEVRST1));
	printk("HCDMABCKDOOR(0x%p) : 0x%X\n",
		acthcd->base + HCDMABCKDOOR, usb_readl(acthcd->base + HCDMABCKDOOR));
	printk("USBH_0ECS(0x%p) : 0x%X\n",
		acthcd->base + USBH_0ECS, usb_readl(acthcd->base + USBH_0ECS));	
*/
	printk(" USBEIEN(0x%p) : 0x%X\n",
            acthcd->base + USBEIEN, usb_readb(acthcd->base + USBEIEN));
	printk(" OTGIRQ(0x%p) : 0x%X\n",
            acthcd->base + OTGIRQ, usb_readb(acthcd->base + OTGIRQ));
	printk(" OTGSTATE(0x%p) : 0x%X\n",
            acthcd->base + OTGSTATE, usb_readb(acthcd->base + OTGSTATE));
	printk(" OTGCTRL(0x%p) : 0x%X\n",
            acthcd->base + OTGCTRL, usb_readb(acthcd->base + OTGCTRL));
	printk(" OTGSTATUS(0x%p) : 0x%X\n",
            acthcd->base + OTGSTATUS, usb_readb(acthcd->base + OTGSTATUS));
	printk(" OTGIEN(0x%p) : 0x%X\n",
            acthcd->base + OTGIEN, usb_readb(acthcd->base + OTGIEN));

    return;
}

#endif	/* AOTG_REG_DUMP */


#ifdef AOTG_DEBUG_FILE

int aotg_dbg_proc_output_ep_state1(struct aotg_hcd *acthcd)
{
	struct aotg_hcep *tmp_ep;
	int i;
	struct aotg_queue *q, *next;
	struct aotg_hcep *ep;
	struct urb *urb;

	ep = acthcd->active_ep0;
	if (ep) {
		printk("------------- active ep0 queue: \n");
		printk("urb_enque_cnt:%d\n", ep->urb_enque_cnt);
		printk("urb_endque_cnt:%d\n", ep->urb_endque_cnt);
		printk("urb_stop_stran_cnt:%d\n", ep->urb_stop_stran_cnt);
		printk("urb_unlinked_cnt:%d\n", ep->urb_unlinked_cnt);

		if (ep->q != NULL) {
			q = ep->q;
			printk("dma[0]: ep->index: %d, type: %d, dir : %s, transfer_buffer_length: %d, actual_length:%d\n",
				q->ep->index,
				usb_pipetype(q->urb->pipe), usb_pipeout(q->urb->pipe)?"out":"in",
				q->urb->transfer_buffer_length, q->urb->actual_length);
		}
	}

	for (i = 0; i < MAX_EP_NUM; i++) {
		ep = acthcd->ep0[i];
		if (ep) {
			printk("------------- ep0 list index:%d queue: \n", i);
			printk("urb_enque_cnt:%d\n", ep->urb_enque_cnt);
			printk("urb_endque_cnt:%d\n", ep->urb_endque_cnt);
			printk("urb_stop_stran_cnt:%d\n", ep->urb_stop_stran_cnt);
			printk("urb_unlinked_cnt:%d\n", ep->urb_unlinked_cnt);
			printk("ep->epnum:%d\n", ep->epnum);

			if (ep->q != NULL) {
				q = ep->q;
				printk("ep->index: %d, type: %d, dir : %s, transfer_buffer_length: %d, actual_length:%d\n",
					q->ep->index,
					usb_pipetype(q->urb->pipe), usb_pipeout(q->urb->pipe)?"out":"in",
					q->urb->transfer_buffer_length, q->urb->actual_length);
			}
		}
	}

	for (i = 1; i < MAX_EP_NUM; i++) {
		tmp_ep = acthcd->inep[i];
		if (tmp_ep) {
			//if (tmp_ep->urb_enque_cnt > (tmp_ep->urb_endque_cnt + tmp_ep->urb_stop_stran_cnt)) 
			{
				printk("inep:%d\n", i);
				printk("urb_enque_cnt:%d\n", tmp_ep->urb_enque_cnt);
				printk("urb_endque_cnt:%d\n", tmp_ep->urb_endque_cnt);
				printk("urb_stop_stran_cnt:%d\n", tmp_ep->urb_stop_stran_cnt);
				printk("urb_unlinked_cnt:%d\n", tmp_ep->urb_unlinked_cnt);

				printk("index:%d\n", tmp_ep->index);
				printk("maxpacket:%d\n", tmp_ep->maxpacket);
				printk("epnum:%d\n", tmp_ep->epnum);
			}
		}
	}
	for (i = 1; i < MAX_EP_NUM; i++) {
		tmp_ep = acthcd->outep[i];
		if (tmp_ep) {
			//if (tmp_ep->urb_enque_cnt > (tmp_ep->urb_endque_cnt + tmp_ep->urb_stop_stran_cnt)) 
			{
				printk("outep:%d\n", i);
				printk("urb_enque_cnt:%d\n", tmp_ep->urb_enque_cnt);
				printk("urb_endque_cnt:%d\n", tmp_ep->urb_endque_cnt);
				printk("urb_stop_stran_cnt:%d\n", tmp_ep->urb_stop_stran_cnt);
				printk("urb_unlinked_cnt:%d\n", tmp_ep->urb_unlinked_cnt);

				printk("index:%d\n", tmp_ep->index);
				printk("maxpacket:%d\n", tmp_ep->maxpacket);
				printk("epnum:%d\n", tmp_ep->epnum);
			}
		}
	}

	printk("in hcd enqueue list: \n");
	list_for_each_entry_safe(q, next, &acthcd->hcd_enqueue_list, enqueue_list) {
		urb = q->urb;
		ep = q->ep;
		printk("ep->epnum:%d ", ep->epnum);
		printk("urb->transfer_buffer_length:%d ", urb->transfer_buffer_length);
		printk("usb_pipein(urb->pipe):%x\n", usb_pipein(urb->pipe));
		printk("usb_pipetype(urb->pipe):%x\n", usb_pipetype(urb->pipe));
	}
	return 0;
}

int aotg_dbg_proc_output_ep_state(struct aotg_hcd *acthcd)
{
	struct aotg_queue *q, *next;
	struct aotg_hcep *ep;
	struct urb *urb;
	int i = 0;

	list_for_each_entry_safe(q, next, &acthcd->hcd_enqueue_list, enqueue_list) {
		urb = q->urb;
		ep = q->ep;
		i++;
	}

	if (i>2) {
		printk("error, more enque.\n");
		//aotg_dbg_output_info();
		//aotg_dbg_regs(acthcd);
		//BUG_ON(1);
	}
	if (i <= 1) {
		i = 0;
	}
	aotg_dbg_proc_output_ep_state1(acthcd);
	return i;
}

int aotg_dump_regs(struct aotg_hcd *acthcd)
{
	int i;
	struct aotg_hcep *ep;

	for (i = 0; i < MAX_EP_NUM; i++) {
		ep = acthcd->inep[i];
		if (ep) {
			aotg_dump_linklist_reg_2(acthcd, ep->mask);
		}
	}

	for (i = 0; i < MAX_EP_NUM; i++) {
		ep = acthcd->outep[i];
		if (ep) {
			aotg_dump_linklist_reg_2(acthcd, ep->mask);	
		}
	}

	return 0;

}

void __dump_ring_info(struct aotg_hcep *ep)
{
	int i;
	struct aotg_ring *ring = NULL;
	struct aotg_td *td, *next;
	struct urb *urb;
	struct aotg_trb *trb;

	//struct aotg_trb trb_val;

	if (!ep)
		return;
	printk("\n------------- current %s ep%d ring : \n", ep->is_out ? "OUT" : "IN",
					ep->index);
	printk("urb_enque_cnt:%d\n", ep->urb_enque_cnt);
	printk("urb_endque_cnt:%d\n", ep->urb_endque_cnt);
	printk("urb_stop_stran_cnt:%d\n", ep->urb_stop_stran_cnt);
	printk("urb_unlinked_cnt:%d\n", ep->urb_unlinked_cnt);
	printk("ep_num:%d\n", ep->epnum);
	printk("ep_type:%d\n", ep->type);

	ring = ep->ring;
	
	i=0;
	if (ring) {
		trb = ring->first_trb;
		while(trb <= ring->last_trb) {
			printk("%d hw_buf_ptr:%x,hw_buf_len:%x,hw_buf_remain:%x,hw_token:%x\n",i, \
			       trb->hw_buf_ptr,trb->hw_buf_len,trb->hw_buf_remain,trb->hw_token);
			trb++;
			i++;
		}
		
	}
	
	if (ring) {		
		printk("-----\n");
		printk("enring_cnt:%d\n", ring->enring_cnt);
		printk("dering_cnt:%d\n", ring->dering_cnt);
		printk("num_trbs_free:%d\n", (u32)atomic_read(&ring->num_trbs_free));
		printk("first_trb:0x%p, dma:0x%x\n", ring->first_trb, 
					ring_trb_virt_to_dma(ring, ring->first_trb));
		printk("last_trb:0x%x, dma:0x%x\n", (u32)(ring->last_trb),
					ring_trb_virt_to_dma(ring, ring->last_trb));
		printk("ring_enqueue:0x%x(%d)\n", (u32)(ring->enqueue_trb),
					ring->enqueue_trb - ring->first_trb);
		printk("ring_dequeue:0x%x(%d)\n", (u32)(ring->dequeue_trb),
					ring->dequeue_trb - ring->first_trb);
		printk("reg_linkaddr(0x%p):0x%x\n", ring->reg_dmalinkaddr,
					usb_readl(ring->reg_dmalinkaddr));
		printk("reg_curradr(0x%p):0x%x\n", ring->reg_curaddr,
					usb_readl(ring->reg_curaddr));
		printk("reg_dmactrl(0x%p):0x%x\n", ring->reg_dmactrl,
					usb_readl(ring->reg_dmactrl));

		printk( "in eq_enqueue_td list: \n");
		i = 0;
		list_for_each_entry_safe(td, next, &ep->queue_td_list, queue_list) {
			urb = td->urb;	
			i++;
			printk("-----\n");
			printk("urb->transfer_buffer_length:%d\n", urb->transfer_buffer_length);
			printk("usb_pipein(urb->pipe):%x\n", usb_pipein(urb->pipe));
			printk("usb_pipetype(urb->pipe):%x\n", usb_pipetype(urb->pipe));
		}
		if (i) {
			i = 0;
			printk("======td in queue num : %d\n", i);
		}

		printk( "in eq_enring_td list: \n");
		list_for_each_entry_safe(td, next, &ep->enring_td_list, enring_list) {
			//trb_val = *(td->trb_vaddr);
			printk("-----\n");
			i++;
			trb = td->trb_vaddr;
			if (td->urb)
				printk("urb:%p\n",td->urb);
			printk("hw_buf_ptr:%x,hw_buf_len:%x,hw_buf_remain:%x,hw_token:%x\n", \
			       trb->hw_buf_ptr,trb->hw_buf_len,trb->hw_buf_remain,trb->hw_token);
			printk("num_trbs:%d\n", td->num_trbs);
			//printk("trb_dma:0x%x\n", td->trb_dma);
			//printk("trb_vaddr:0x%x\n", td->trb_vaddr);
			//printk("trb_dma:0x%x\n", td->trb_dma);
			//printk("cross_ring:0x%d\n", td->cross_ring);

			//aotg_hcd_dump_td(ring, td);
			//printk("hw_buf_ptr : 0x%x\n", trb_val.hw_buf_ptr);
			//printk("hw_buf_len : %d\n", trb_val.hw_buf_len);
			//printk("hw_buf_remain : %d\n", trb_val.hw_buf_remain);
			//printk("hw_token : 0x%x\n", trb_val.hw_token);	
			//printk("in_dma_irq:0x%x\n",usb_readw(USBH_BASE0 + HCINxDMAIRQ0));
		}
		if (i) {
			printk("======td in ring num : %d\n", i);
		}
	}
 
/*
	seq_printf(s, "------------- current IN ep%d ring : \n", i);
	seq_printf(s, "urb_enque_cnt:%d\n", ep->urb_enque_cnt);
	seq_printf(s, "urb_endque_cnt:%d\n", ep->urb_endque_cnt);
	seq_printf(s, "urb_stop_stran_cnt:%d\n", ep->urb_stop_stran_cnt);
	seq_printf(s, "urb_unlinked_cnt:%d\n", ep->urb_unlinked_cnt);
	seq_printf(s, "ep->epnum:%d\n", ep->epnum);

	ring = ep->ring;
	if (ring) {
		seq_printf(s, "enring_cnt:%d\n", ring->enring_cnt);
		seq_printf(s, "dering_cnt:%d\n", ring->dering_cnt);
		seq_printf(s, "num_trbs_free:%d\n", ring->num_trbs_free);
		seq_printf(s, "ring_enqueue_ptr:0x%x", ring->enqueue_trb);
		seq_printf(s, "ring_dequeue_ptr:0x%x", ring->dequeue_trb);

		seq_printf(s, "in eq_enqueue_td list: \n");
		list_for_each_entry_safe(td, next, &ep->queue_td_list, queue_td_list) {
			urb = td->urb;
			seq_printf(s, "urb->transfer_buffer_length:%d ", urb->transfer_buffer_length);
			seq_printf(s, "usb_pipein(urb->pipe):%x\n", usb_pipein(urb->pipe));
			seq_printf(s, "usb_pipetype(urb->pipe):%x\n", usb_pipetype(urb->pipe));
		}

		seq_printf(s, "in eq_enring_td list: \n");
		list_for_each_entry_safe(td, next, &ep->enring_td_list, enring_list) {
			seq_printf(s, "num_trbs:%d\n", td->num_trbs);
			seq_printf(s, "trb_vaddr:0x%x\n", td->trb_vaddr);
			seq_printf(s, "trb_dma:0x%x\n", td->trb_dma);
			seq_printf(s, "cross_ring:0x%d\n", td->cross_ring);
		}
	}
*/
}

static int aotg_hcd_show_ring_info(struct aotg_hcd *acthcd)
{
	int i;
	struct aotg_hcep *ep;
	struct aotg_queue *q;

	ep = acthcd->active_ep0;
	if (ep) {
		printk("------------- active ep0 queue: \n");
		printk("urb_enque_cnt:%d\n", ep->urb_enque_cnt);
		printk("urb_endque_cnt:%d\n", ep->urb_endque_cnt);
		printk("urb_stop_stran_cnt:%d\n", ep->urb_stop_stran_cnt);
		printk("urb_unlinked_cnt:%d\n", ep->urb_unlinked_cnt);

		if (ep->q != NULL) {
			q = ep->q;
			printk("dma[0]: ep->index: %d, type: %d, dir : %s, transfer_buffer_length: %d, actual_length:%d\n",
				q->ep->index,
				usb_pipetype(q->urb->pipe), usb_pipeout(q->urb->pipe)?"out":"in",
				q->urb->transfer_buffer_length, q->urb->actual_length);
		}
	}

	for (i = 0; i < MAX_EP_NUM; i++) {
		ep = acthcd->inep[i];
		__dump_ring_info(ep);
	}

	for (i = 0; i < MAX_EP_NUM; i++) {
		ep = acthcd->outep[i];
		__dump_ring_info(ep);
	}

	return 0;
}
#if 0
static int aotg_hcd_show_enque_info(struct seq_file *s, struct aotg_hcd	*acthcd)
{
	int i;
	struct aotg_queue *q, *next;
	struct aotg_hcep *ep;

	for (i = 0; i < AOTG_QUEUE_POOL_CNT; i++) {
		if (acthcd->queue_pool[i] != NULL) {
			seq_printf(s, "queue_pool[%d]->in_using: %d\n", 
			      	i, acthcd->queue_pool[i]->in_using);
		} 
	}

	seq_printf(s, "current dma queue: \n");

	ep = acthcd->active_ep0;
	if (ep) {
		seq_printf(s, "------------- active ep0 queue: \n");
		seq_printf(s, "urb_enque_cnt:%d\n", ep->urb_enque_cnt);
		seq_printf(s, "urb_endque_cnt:%d\n", ep->urb_endque_cnt);
		seq_printf(s, "urb_stop_stran_cnt:%d\n", ep->urb_stop_stran_cnt);
		seq_printf(s, "urb_unlinked_cnt:%d\n", ep->urb_unlinked_cnt);

		if (ep->q != NULL) {
			q = ep->q;
			seq_printf(s, "dma[0]: ep->index: %d, type: %d, dir : %s, transfer_buffer_length: %d, actual_length:%d\n",
				q->ep->index,
				usb_pipetype(q->urb->pipe), usb_pipeout(q->urb->pipe)?"out":"in",
				q->urb->transfer_buffer_length, q->urb->actual_length);
		}
	}

	for (i = 0; i < MAX_EP_NUM; i++) {
		ep = acthcd->ep0[i];
		if (ep) {
			seq_printf(s, "------------- ep0 list index:%d queue: \n", i);
			seq_printf(s, "urb_enque_cnt:%d\n", ep->urb_enque_cnt);
			seq_printf(s, "urb_endque_cnt:%d\n", ep->urb_endque_cnt);
			seq_printf(s, "urb_stop_stran_cnt:%d\n", ep->urb_stop_stran_cnt);
			seq_printf(s, "urb_unlinked_cnt:%d\n", ep->urb_unlinked_cnt);
			seq_printf(s, "ep->epnum:%d\n", ep->epnum);

			if (ep->q != NULL) {
				q = ep->q;
				seq_printf(s, "ep->index: %d, type: %d, dir : %s, transfer_buffer_length: %d, actual_length:%d\n",
					q->ep->index,
					usb_pipetype(q->urb->pipe), usb_pipeout(q->urb->pipe)?"out":"in",
					q->urb->transfer_buffer_length, q->urb->actual_length);
			}
		}
	}
	
	for (i = 1; i < MAX_EP_NUM; i++) {
		ep = acthcd->inep[i];
		if (ep) {
			seq_printf(s, "------------- current IN ep%d queue: \n", i);
			seq_printf(s, "urb_enque_cnt:%d\n", ep->urb_enque_cnt);
			seq_printf(s, "urb_endque_cnt:%d\n", ep->urb_endque_cnt);
			seq_printf(s, "urb_stop_stran_cnt:%d\n", ep->urb_stop_stran_cnt);
			seq_printf(s, "urb_unlinked_cnt:%d\n", ep->urb_unlinked_cnt);
			seq_printf(s, "ep->epnum:%d\n", ep->epnum);

			if (ep->q != NULL) {
				q = ep->q;
				seq_printf(s, "ep->index: %d, type: %d, dir : %s, transfer_buffer_length: %d, actual_length:%d\n",
					q->ep->index,
					usb_pipetype(q->urb->pipe), usb_pipeout(q->urb->pipe)?"out":"in",
					q->urb->transfer_buffer_length, q->urb->actual_length);
			}
		}
	}
	
	for (i = 1; i < MAX_EP_NUM; i++) {
		ep = acthcd->outep[i];
		if (ep) {
			seq_printf(s, "------------- current OUT ep%d queue: \n", i);
			seq_printf(s, "urb_enque_cnt:%d\n", ep->urb_enque_cnt);
			seq_printf(s, "urb_endque_cnt:%d\n", ep->urb_endque_cnt);
			seq_printf(s, "urb_stop_stran_cnt:%d\n", ep->urb_stop_stran_cnt);
			seq_printf(s, "urb_unlinked_cnt:%d\n", ep->urb_unlinked_cnt);
			seq_printf(s, "ep->epnum:%d\n", ep->epnum);

			if (ep->q != NULL) {
				q = ep->q;
				seq_printf(s, "ep->index: %d, type: %d, dir : %s, transfer_buffer_length: %d, actual_length:%d\n",
					q->ep->index,
					usb_pipetype(q->urb->pipe), usb_pipeout(q->urb->pipe)?"out":"in",
					q->urb->transfer_buffer_length, q->urb->actual_length);
			}
		}
	}

	seq_printf(s, "\n");
	seq_printf(s, "in hcd enqueue list: \n");
	list_for_each_entry_safe(q, next, &acthcd->hcd_enqueue_list, enqueue_list) {
		ep = q->ep;
		seq_printf(s, "ep->epnum:%d ", ep->epnum);
		seq_printf(s, "urb->transfer_buffer_length:%d ", q->urb->transfer_buffer_length);
		seq_printf(s, "usb_pipein(urb->pipe):%x\n", usb_pipein(q->urb->pipe));
		seq_printf(s, "usb_pipetype(urb->pipe):%x\n", usb_pipetype(q->urb->pipe));
	}
	return 0;
}
#endif
/* 
 * echo a value to controll the cat /proc/aotg_hcd output content.
 * echo h>/proc/aotg_hcd.0 to see help info.
 */
ssize_t aotg_hcd_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char c = 'n';

	if (count) {
		if (get_user(c, buf))
			return -EFAULT;
		aotg_hcd_proc_sign = c;
	}
	if (c == 'h') {
		printk(" a ---- all.  \n");
		printk(" b ---- backup info.  \n");
		printk(" d ---- dma related.  \n");
		printk(" e ---- enque and outque info.  \n");
		printk(" f ---- trace in info.  \n");
		printk(" h ---- help info.  \n");
		printk(" n ---- normal.  \n");
		printk(" r ---- register info.  \n");
		printk(" s ---- aotg state.  \n");
		printk(" t ---- trace out info.  \n");
		printk(" z ---- stop stace.  \n");
	}
        return count;
}

int aotg_hcd_proc_show(struct seq_file *s, void *unused)
{
	struct aotg_hcd	*acthcd = s->private;
	struct usb_hcd *hcd = aotg_to_hcd(acthcd);
	//struct aotg_plat_data *data = acthcd->port_specific;

	if (aotg_hcd_proc_sign == 'd') {
		// todo.
	}

	if (aotg_hcd_proc_sign == 's') {
		aotg_dbg_proc_output_ep_state(acthcd);
		seq_printf(s, "hcd state : 0x%08X\n", hcd->state);
	}

	if (aotg_hcd_proc_sign == 'r') {
		//aotg_dbg_regs(acthcd);
		aotg_dump_regs(acthcd);
	}

	if (aotg_hcd_proc_sign == 'e') {
		//aotg_hcd_show_enque_info(s, acthcd);
		aotg_hcd_show_ring_info(acthcd);
	}

	if (aotg_hcd_proc_sign == 'b') {
		aotg_dbg_proc_output_ep();
		aotg_dbg_output_info();
	}

	if (aotg_hcd_proc_sign == 'a') {
	}

	seq_printf(s, "\n");
	return 0;
}


static int aotg_hcd_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, aotg_hcd_proc_show, PDE(inode)->data);
}

static const struct file_operations proc_ops = {
	.open		= aotg_hcd_proc_open,
	.read		= seq_read,
	.write		= aotg_hcd_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void create_debug_file(struct aotg_hcd *acthcd)
{
	struct device *dev = aotg_to_hcd(acthcd)->self.controller;

	acthcd->pde = proc_create_data(dev_name(dev), 0, NULL, &proc_ops, acthcd);
	return;
}

void remove_debug_file(struct aotg_hcd *acthcd)
{
	struct device *dev = aotg_to_hcd(acthcd)->self.controller;
	
	if (acthcd->pde)
		remove_proc_entry(dev_name(dev), NULL);
	return;
}

#else	/* AOTG_DEBUG_FILE */

void create_debug_file(struct aotg_hcd *acthcd)
{
	return;
}

void remove_debug_file(struct aotg_hcd *acthcd)
{
	return;
}

#endif	/* AOTG_DEBUG_FILE */


void aotg_print_xmit_cnt(char * info, int cnt)
{
	if (aotg_hcd_proc_sign == 'e') {
		printk("%s cnt:%d\n", info, cnt);
	}
	//printk("\n");
	//aotg_dbg_proc_output_ep();
	//aotg_dbg_regs(p_aotg_hcd0);
	//aotg_dbg_output_info();

	return;
}
//EXPORT_SYMBOL(aotg_print_xmit_cnt);


static struct proc_dir_entry *acts_hub_pde = NULL; 

int acts_hcd_proc_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "hcd_ports_en_ctrl: %d\n", hcd_ports_en_ctrl);
	return 0;
}

static int acts_hub_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, acts_hcd_proc_show, PDE(inode)->data);
}

static ssize_t acts_hub_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char c = 'n';

	if (count) {
		if (get_user(c, buf))
			return -EFAULT;
	}
	if ((c >= '0') && (c <= '3')) { 
		hcd_ports_en_ctrl = c - '0';
		printk("hcd_hub en:%d\n", hcd_ports_en_ctrl);
	}
	if (c == 'h') {
		printk(" num ---- 0-all enable, 1-usb0 enable, 2-usb1 enable, 3-reversed. \n");
		printk("o ---- hcd_hub power on\n");
		printk("f ---- hcd_hub power off\n");
		printk("a ---- hcd_hub aotg0 add\n");
		printk("b ---- hcd_hub aotg0 remove\n");
		printk("c ---- hcd_hub aotg1 add\n");
		printk("d ---- hcd_hub aotg1 remove\n");
	}

	if (c == 'a') {
		printk("hcd_hub aotg0 add\n");
		//aotg0_device_init(0);
		aotg_hub_register(0);
	}
	if (c == 'b') {
		printk("hcd_hub aotg0 remove\n");
		//aotg0_device_exit(0);
		aotg_hub_unregister(0);
	}

	if (c == 'c') {
		printk("hcd_hub aotg1 add\n");
		//aotg1_device_init(0);
		aotg_hub_register(1);
	}
	if (c == 'd') {
		printk("hcd_hub aotg1 remove\n");
		//aotg1_device_exit(0);
		aotg_hub_unregister(1);
	}

	if (c == 'e') {
		aotg_trace_onff = 1;
	}
	if (c == 'f') {
		aotg_trace_onff = 0;
	}
	
	if (c == 'g') {
		aotg_dbg_regs(act_hcd_ptr[0]); 
	}
	
	if (c == 'i') {
		aotg_dbg_regs(act_hcd_ptr[1]); 
	}
		return count;
}

static const struct file_operations acts_hub_proc_ops = {
	.open		= acts_hub_proc_open,
	.read		= seq_read,
	.write		= acts_hub_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void create_acts_hcd_proc(void)
{
	acts_hub_pde = proc_create_data("acts_hub", S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH, NULL, &acts_hub_proc_ops, acts_hub_pde);
	return;
}

void remove_acts_hcd_proc(void)
{
	if (acts_hub_pde) {
		remove_proc_entry("acts_hub", NULL);
		acts_hub_pde = NULL;
	}
	return;
}

