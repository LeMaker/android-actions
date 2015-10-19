/*
 * Driver for the Atmel AHB DMA Controller (aka HDMA or DMAC on AT91 systems)
 *
 * Copyright (C) 2008 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 * This supports the Atmel AHB DMA Controller,
 *
 * The driver has currently been tested with the Atmel AT91SAM9RL
 * and AT91SAM9G45 series.
 */


#include <linux/clk.h>
#include "dmaengine.h"
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/cpu.h>
#include <linux/of_device.h>
#include <mach/powergate.h>
#include <mach/clkname.h>
#include <mach/module-owl.h>

#if 1
#define DMA_CACHE_ADDR
#endif

#include "owl_hdmac_regs.h"

#define STOP_TIMEOUT 200000
static unsigned long max_timeout;

#define debug_nand_timeout
#ifdef debug_nand_timeout
static unsigned int nand_desc;
static unsigned long time_dostart;
static unsigned long time_interrupt;
static unsigned long time_callback;
static unsigned long time_submit;
#endif

//#define enable_one_normal
#ifdef enable_one_normal
static unsigned char *dma_normal_buf;
static dma_addr_t dma_normal_phy;
#endif

/*
 * Initial number of descriptors to allocate for each channel. This could
 * be increased during dma usage.
 */
static unsigned int init_nr_desc_per_channel = 64;
module_param(init_nr_desc_per_channel, uint, 0644);
MODULE_PARM_DESC(init_nr_desc_per_channel,
		 "initial descriptors per channel (default: 64)");

/* prototypes */
static dma_cookie_t acts_tx_submit(struct dma_async_tx_descriptor *tx);

static struct acts_desc *acts_first_active(struct owl_dma_chan *atchan)
{
	return list_first_entry(&atchan->active_list,
				struct acts_desc, desc_node);
}

static struct acts_desc *acts_first_queued(struct owl_dma_chan *atchan)
{
	return list_first_entry(&atchan->queue,
				struct acts_desc, desc_node);
}

#ifdef DMA_CACHE_ADDR
static void acts_sync_desc_for_cpu(
	struct owl_dma_chan *atchan,
	struct acts_desc *desc)
{
	struct acts_desc	*child;

	list_for_each_entry(child, &desc->tx_list, desc_node)
		dma_sync_single_for_cpu(chan2parent(&atchan->chan_common),
				child->txd.phys, sizeof(child->lli),
				DMA_TO_DEVICE);
	dma_sync_single_for_cpu(chan2parent(&atchan->chan_common),
			desc->txd.phys, sizeof(desc->lli),
			DMA_TO_DEVICE);
}
#endif

/**
 * acts_alloc_descriptor - allocate and return an initilized descriptor
 * @chan: the channel to allocate descriptors for
 * @gfp_flags: GFP allocation flags
 *
 * Note: The ack-bit is positioned in the descriptor flag at creation time
 *       to make initial allocation more convenient. This bit will be cleared
 *       and control will be given to client at usage time (during
 *       preparation functions).
 */
static struct acts_desc *acts_alloc_descriptor(struct dma_chan *chan,
					    gfp_t gfp_flags)
{
	struct acts_desc *desc = NULL;
#ifndef DMA_CACHE_ADDR
	struct owl_dma	*owl_dma = to_owl_dma(chan->device);
	dma_addr_t phys;
#endif

#ifdef DMA_CACHE_ADDR

		desc = kzalloc(sizeof(struct acts_desc), GFP_KERNEL);
		if (desc) {
			INIT_LIST_HEAD(&desc->tx_list);
			dma_async_tx_descriptor_init(&desc->txd, chan);
			desc->txd.tx_submit = acts_tx_submit;
			desc->txd.flags = DMA_CTRL_ACK;
			desc->txd.phys = dma_map_single(chan2parent(chan),
				&desc->lli, sizeof(desc->lli), DMA_TO_DEVICE);
			/*atchan_desc_put(atchan, desc);*/
		}
#else
	desc = dma_pool_alloc(owl_dma->dma_desc_pool, gfp_flags, &phys);
	if (desc) {
		memset(desc, 0, sizeof(struct acts_desc));
		INIT_LIST_HEAD(&desc->tx_list);
		dma_async_tx_descriptor_init(&desc->txd, chan);
		/* txd.flags will be overwritten in prep functions */
		desc->txd.flags = DMA_CTRL_ACK;
		desc->txd.tx_submit = acts_tx_submit;
		desc->txd.phys = phys;
	}
#endif

	return desc;
}

/**
 * acts_desc_get - get an unused descriptor from free_list
 * @atchan: channel we want a new descriptor for
 */
static struct acts_desc *acts_desc_get(struct owl_dma_chan *atchan)
{
	struct acts_desc *desc, *_desc;
	struct acts_desc *ret = NULL;
	unsigned int i = 0;
	LIST_HEAD(tmp_list);
	unsigned long flags;

	spin_lock_irqsave(&atchan->lock, flags);
	list_for_each_entry_safe(desc, _desc, &atchan->free_list, desc_node) {
		i++;
		if (async_tx_test_ack(&desc->txd)) {
			list_del(&desc->desc_node);
			ret = desc;
			break;
		}
		dev_dbg(chan2dev(&atchan->chan_common),
				"desc %p not ACKed\n", desc);
	}
	spin_unlock_irqrestore(&atchan->lock, flags);
	dev_vdbg(chan2dev(&atchan->chan_common),
		"scanned %u descriptors on freelist\n", i);

	/* no more descriptor available in initial pool: create one more */
	if (!ret) {
		ret = acts_alloc_descriptor(&atchan->chan_common, GFP_ATOMIC);
		if (ret) {
			spin_lock_irqsave(&atchan->lock, flags);
			atchan->descs_allocated++;
			spin_unlock_irqrestore(&atchan->lock, flags);
		} else {
			dev_err(chan2dev(&atchan->chan_common),
					"not enough descriptors available\n");
		}
	}

	return ret;
}

/**
 * acts_desc_put - move a descriptor, including any children, to the free list
 * @atchan: channel we work on
 * @desc: descriptor, at the head of a chain, to move to free list
 */
static void acts_desc_put(struct owl_dma_chan *atchan, struct acts_desc *desc)
{

	if (desc) {
		unsigned long flags;

#ifdef	DMA_CACHE_ADDR
		acts_sync_desc_for_cpu(atchan, desc);
#endif

		spin_lock_irqsave(&atchan->lock, flags);
		list_splice_init(&desc->tx_list, &atchan->free_list);
		list_add(&desc->desc_node, &atchan->free_list);
		spin_unlock_irqrestore(&atchan->lock, flags);
	}
}

/**
 * atc_desc_chain - build chain adding a descripor
 * @first: address of first descripor of the chain
 * @prev: address of previous descripor of the chain
 * @desc: descriptor to queue
 *
 * Called from prep_* functions
 */

static void acts_desc_chain(struct dma_chan *chan, struct acts_desc **first,
			struct acts_desc **prev, struct acts_desc *desc)
{
	if (!(*first)) {
		*first = desc;
	} else {
		/* inform the HW lli about chaining */
		(*prev)->lli.dscr = desc->txd.phys;
#ifdef DMA_CACHE_ADDR
		dma_sync_single_for_device(chan2parent(chan),
				(*prev)->txd.phys, sizeof((*prev)->lli),
				DMA_TO_DEVICE);
#endif
		/* insert the link descriptor to the LD ring */
		list_add_tail(&desc->desc_node,
				&(*first)->tx_list);
	}
	*prev = desc;
}

/**
 * acts_dostart - starts the DMA engine for real
 * @atchan: the channel we want to start
 * @first: first descriptor in the list we want to begin with
 *
 * Called with atchan->lock held and bh disabled
 */
static void acts_dostart(struct owl_dma_chan *atchan, struct acts_desc *first)
{
	/*struct owl_dma	*owl_dma =
	to_owl_dma(atchan->chan_common.device);*/
	u32	int_ctl;

	if (acts_chan_is_enabled_dump(atchan)) {
		dev_err(chan2dev(&atchan->chan_common),
			"BUG: Attempted to start non-idle channel!\n");
		return;
	}

#if defined(VERBOSE_DEBUG)
	vdbg_dump_regs(atchan);
#endif

#ifdef	DMA_CACHE_ADDR
		acts_sync_desc_for_cpu(atchan, first);
#endif

	int_ctl = ACTS_INT_CTL_SUPERBLOCK_INT
		| ACTS_INT_STATUS_SECURE_ERROR | ACTS_INT_STATUS_ALAINED_ERROR;
	if (acts_chan_is_cyclic(atchan))
		int_ctl |= ACTS_INT_STATUS_END_BLOCK_INT;

	channel_writel(atchan, MODE, first->mode);
	channel_writel(atchan, LINKLIST, ACTS_LINKLIST_SRC_DST_VLD);
	channel_writel(atchan, NEXT_DESC, first->txd.phys);

	channel_writel(atchan, INT_CTL, int_ctl);
	channel_writel(atchan, INT_STAT, 0x7f);

	channel_writel(atchan, START, 0x1);

#ifdef debug_nand_timeout
	if (atchan->descs_allocated == nand_desc)
			time_dostart = jiffies;
#endif

//	vdbg_dump_regs(atchan);
}

/**
 * acts_chain_complete - finish work for one transaction chain
 * @atchan: channel we work on
 * @desc: descriptor at the head of the chain we want do complete
 *
 * Called with atchan->lock held and bh disabled */
static void
acts_chain_complete(struct owl_dma_chan *atchan, struct acts_desc *desc)
{
	dma_async_tx_callback		callback;
	void				*param;
	struct dma_async_tx_descriptor	*txd = &desc->txd;
	struct owl_dma_slave *slave = atchan->chan_common.private;
	struct device *parent;

	dev_vdbg(chan2dev(&atchan->chan_common),
		"descriptor %u complete\n", txd->cookie);

	if (!acts_chan_is_cyclic(atchan))
		dma_cookie_complete(txd);


	callback = txd->callback;
	param = txd->callback_param;

#ifdef	DMA_CACHE_ADDR
	acts_sync_desc_for_cpu(atchan, desc);
#endif

	/* move children to free_list */
	list_splice_init(&desc->tx_list, &atchan->free_list);
	/* move myself to free_list */
	list_move(&desc->desc_node, &atchan->free_list);

	/* unmap dma addresses */
	/*if (!atchan->chan_common.private) {*/

	if (slave->trans_type == MEMCPY) {
		dev_vdbg(chan2dev(&atchan->chan_common), "unmap  the memory\n");
		parent = chan2parent(&atchan->chan_common);
		if (!(txd->flags & DMA_COMPL_SKIP_DEST_UNMAP)) {
			if (txd->flags & DMA_COMPL_DEST_UNMAP_SINGLE)
				dma_unmap_single(parent,
						desc->lli.daddr,
						desc->len, DMA_FROM_DEVICE);
			else
				dma_unmap_page(parent,
						desc->lli.daddr,
						desc->len, DMA_FROM_DEVICE);
		}
		if (!(txd->flags & DMA_COMPL_SKIP_SRC_UNMAP)) {
			if (txd->flags & DMA_COMPL_SRC_UNMAP_SINGLE)
				dma_unmap_single(parent,
						desc->lli.saddr,
						desc->len, DMA_TO_DEVICE);
			else
				dma_unmap_page(parent,
						desc->lli.saddr,
						desc->len, DMA_TO_DEVICE);
		}
	}

	/*
	 * The API requires that no submissions are done from a
	 * callback, so we don't need to drop the lock here
	 */
	if (!acts_chan_is_cyclic(atchan)) {
		dma_async_tx_callback	callback = txd->callback;
		void			*param = txd->callback_param;

		/*
		 * The API requires that no submissions are done from a
		 * callback, so we don't need to drop the lock here
		 */

#ifdef debug_nand_timeout
		if (atchan->descs_allocated == nand_desc)
			time_callback = jiffies;

		if ((atchan->descs_allocated == nand_desc) &&
				(jiffies_to_msecs(time_callback - time_submit)
				> 1000)) {
			pr_alert("\n\nIF you see this, report it to CaiYu(SH),please!!!\n\n");
			pr_alert("[NAND-DMA]time elapsed from submit to dostart: %d ms\n",
					jiffies_to_msecs(time_dostart -
						time_submit));
			pr_alert("[NAND-DMA]time elapsed from dostart to interrupt: %d ms\n",
					jiffies_to_msecs(time_interrupt -
						time_dostart));
			pr_alert("[NAND-DMA]time elapsed from interrupt to callback: %d ms\n\n\n",
					jiffies_to_msecs(time_callback -
						time_interrupt));
		}
#endif

		if (callback)
			callback(param);
	}

	dma_run_dependencies(txd);
}

/**
 * acts_complete_all - finish work for all transactions
 * @atchan: channel to complete transactions for
 *
 * Eventually submit queued descriptors if any
 *
 * Assume channel is idle while calling this function
 * Called with atchan->lock held and bh disabled
 */
static void acts_complete_all(struct owl_dma_chan *atchan)
{
	struct acts_desc *desc, *_desc;
	LIST_HEAD(list);

	dev_vdbg(chan2dev(&atchan->chan_common), "complete all\n");

	BUG_ON(acts_chan_is_enabled_dump(atchan));

	/*
	 * Submit queued descriptors ASAP, i.e. before we go through
	 * the completed ones.
	 */
	if (!list_empty(&atchan->queue))
		acts_dostart(atchan, acts_first_queued(atchan));
	/* empty active_list now it is completed */
	list_splice_init(&atchan->active_list, &list);
	/* empty queue list by moving descriptors (if any) to active_list */
	list_splice_init(&atchan->queue, &atchan->active_list);

	list_for_each_entry_safe(desc, _desc, &list, desc_node)
		acts_chain_complete(atchan, desc);
}

/**
 * acts_cleanup_descriptors - cleanup up finished descriptors in active_list
 * @atchan: channel to be cleaned up
 *
 * Called with atchan->lock held and bh disabled
 */
static void acts_cleanup_descriptors(struct owl_dma_chan *atchan)
{
	/*struct acts_desc	*desc, *_desc;*/
	/*struct acts_desc	*child;*/

	dev_err(chan2dev(&atchan->chan_common), "cleanup descriptors\n");
	return;
}

/**
 * acts_advance_work - at the end of a transaction, move forward
 * @atchan: channel where the transaction ended
 *
 * Called with atchan->lock held and bh disabled
 */
static void acts_advance_work(struct owl_dma_chan *atchan)
{
	dev_vdbg(chan2dev(&atchan->chan_common), "advance_work\n");

	if (list_empty(&atchan->active_list) ||
	    list_is_singular(&atchan->active_list)) {
		acts_complete_all(atchan);
	} else {
		acts_chain_complete(atchan, acts_first_active(atchan));
		/* advance work */
		acts_dostart(atchan, acts_first_active(atchan));
	}
}

/**
 * acts_handle_error - handle errors reported by DMA controller
 * @atchan: channel where error occurs
 *
 * Called with atchan->lock held and bh disabled
 */
static void acts_handle_error(struct owl_dma_chan *atchan)
{
	struct acts_desc *bad_desc;
	struct acts_desc *child;

	/*
	 * The descriptor currently at the head of the active list is
	 * broked. Since we don't have any way to report errors, we'll
	 * just have to scream loudly and try to carry on.
	 */
	bad_desc = acts_first_active(atchan);
	list_del_init(&bad_desc->desc_node);

	/* As we are stopped, take advantage to push queued descriptors
	 * in active_list */
	list_splice_init(&atchan->queue, atchan->active_list.prev);

	/* Try to restart the controller */
	if (!list_empty(&atchan->active_list))
		acts_dostart(atchan, acts_first_active(atchan));

	/*
	 * KERN_CRITICAL may seem harsh, but since this only happens
	 * when someone submits a bad physical address in a
	 * descriptor, we should consider ourselves lucky that the
	 * controller flagged an error instead of scribbling over
	 * random memory locations.
	 */
	dev_crit(chan2dev(&atchan->chan_common),
			"Bad descriptor submitted for DMA!\n");
	dev_crit(chan2dev(&atchan->chan_common),
			"  cookie: %d\n", bad_desc->txd.cookie);
	acts_dump_lli(atchan, &bad_desc->lli);
	list_for_each_entry(child, &bad_desc->tx_list, desc_node)
		acts_dump_lli(atchan, &child->lli);

	/* Pretend the descriptor completed successfully */
	acts_chain_complete(atchan, bad_desc);
}

static void acts_handle_cyclic(struct owl_dma_chan *atchan)
{
	struct acts_desc		*first = acts_first_active(atchan);
	struct dma_async_tx_descriptor	*txd = &first->txd;
	dma_async_tx_callback		callback = txd->callback;
	void				*param = txd->callback_param;

	if (test_and_clear_bit(ACTS_IS_INTERRUPT, &atchan->status)) {
		dev_vdbg(chan2dev(&atchan->chan_common),
			"new cyclic period llp 0x%08x\n",
			channel_readl(atchan, NEXT_DESC));

		if (NULL != callback)
			callback(param);
	}
}

/*--  IRQ & Tasklet  ---------------------------------------------------*/

static void acts_tasklet(unsigned long data)
{
	struct owl_dma *owl_dma = (struct owl_dma *)data;
	struct owl_dma_chan *atchan;
	int i;

	for (i = 0; i < owl_dma->dma_common.chancnt; i++) {
		atchan = &owl_dma->chan[i];
		if (test_bit(ACTS_IS_ERROR, &atchan->status))
			acts_handle_error(atchan);
		else if (test_bit(ACTS_IS_CYCLIC, &atchan->status)) {
			if (list_empty(&atchan->active_list)) {
				pr_warning("WARNING: %s, active_list is empty!\n",
						__func__);
				return;
			}
			acts_handle_cyclic(atchan);
		} else if (test_and_clear_bit(ACTS_IS_INTERRUPT,
				&atchan->status)) {
			if (acts_chan_is_enabled_dump(atchan)) {
				dev_err(chan2dev(&atchan->chan_common),
					"BUG: channel enabled in tasklet\n");
				return;
			}
			acts_advance_work(atchan);
		}
	}
}

static irqreturn_t owl_dma_interrupt(int irq, void *dev_id)
{
	struct owl_dma		*owl_dma = (struct owl_dma *)dev_id;
	struct owl_dma_chan	*atchan;
	int			i;
	u32			status, imr, pending, p0, p1, p2, p3;
	u32			int_ctl, int_status, channel_pending;
	int			ret = IRQ_NONE;

	imr = dma_readl(owl_dma, IRQEN_0);
	status = dma_readl(owl_dma, IRQPD_0);
	p0 = status  & imr;
	imr = dma_readl(owl_dma, IRQEN_1);
	status = dma_readl(owl_dma, IRQPD_1);
	p1 = status  & imr;
	imr = dma_readl(owl_dma, IRQEN_2);
	status = dma_readl(owl_dma, IRQPD_2);
	p2 = status  & imr;
	imr = dma_readl(owl_dma, IRQEN_3);
	status = dma_readl(owl_dma, IRQPD_3);
	p3 = status  & imr;

	pending = p0 | p1 | p2 | p3;

	dev_vdbg(owl_dma->dma_common.dev,
		"interrupt: pending =  0x%08x\n",
		pending);

	pending &= 0x0fff;
	if (pending) {
		for (i = 0; i < owl_dma->dma_common.chancnt; i++) {
			atchan = &owl_dma->chan[i];
#ifdef debug_nand_timeout
			if (atchan->descs_allocated == nand_desc)
				time_interrupt = jiffies;
#endif
			if (pending & ACTS_DMA_PENDING_MASK(i)) {
				spin_lock(&atchan->lock);
				int_ctl = channel_readl(atchan, INT_CTL);
				int_status = channel_readl(atchan, INT_STAT);

				/*clear pending bits ASAP*/
				channel_writel(atchan, INT_STAT, 0x7f);
				dma_writel(owl_dma, IRQPD_0, (1 << i));
				dma_writel(owl_dma, IRQPD_1, (1 << i));
				dma_writel(owl_dma, IRQPD_2, (1 << i));
				dma_writel(owl_dma, IRQPD_3, (1 << i));
				spin_unlock(&atchan->lock);

				channel_pending = int_ctl & int_status;
				if (channel_pending & 0x60) {
					set_bit(ACTS_IS_ERROR, &atchan->status);
					pr_err("dma%d transfer error",
						atchan->chan_common.chan_id);
					owl_dma_dump_all(&atchan->chan_common);
				} else if (channel_pending & 0x1f) {
					set_bit(ACTS_IS_INTERRUPT,
						&atchan->status);
				}
			}
		}

		/*owl_dma->pending = pending;*/
		tasklet_schedule(&owl_dma->tasklet);
		ret = IRQ_HANDLED;
	}
	return ret;
}

/*--  DMA Engine API  --------------------------------------------------*/

/**
 * acts_tx_submit - set the prepared descriptor(s) to be executed by the engine
 * @desc: descriptor at the head of the transaction chain
 *
 * Queue chain if DMA engine is working already
 *
 * Cookie increment and adding to active_list or queue must be atomic
 */
static dma_cookie_t acts_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct acts_desc		*desc = txd_to_acts_desc(tx);
	struct owl_dma_chan	*atchan = to_owl_dma_chan(tx->chan);
	dma_cookie_t		cookie;
	unsigned long       flags;

#ifdef debug_nand_timeout
		if (atchan->descs_allocated == nand_desc)
			time_submit = jiffies;
#endif

	spin_lock_irqsave(&atchan->lock, flags);
	cookie = dma_cookie_assign(tx);

	if (list_empty(&atchan->active_list)) {
		dev_vdbg(chan2dev(tx->chan), "tx_submit: started %u\n",
				desc->txd.cookie);
		acts_dostart(atchan, desc);
		list_add_tail(&desc->desc_node, &atchan->active_list);
	} else {
		dev_vdbg(chan2dev(tx->chan), "tx_submit: queued %u\n",
				desc->txd.cookie);
		list_add_tail(&desc->desc_node, &atchan->queue);
	}

	spin_unlock_irqrestore(&atchan->lock, flags);

	return cookie;
}

static void acts_set_ctrl(struct dma_chan *chan, struct acts_desc *desc,
		struct owl_dma_slave *atslave, u32 mode, u32 ctrlb)
{
	unsigned long acp;
	u32	int_ctl;
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);

	acp  = atslave->trans_type >> 2;
	if ((mode & ACTS_SRC_DST_STRIDE) == 0) {  /*no stride mode*/
		/* frame cnt */
		desc->lli.ctrla |=  ACTS_FRAMECNT(0x1);
		desc->lli.src_stride = 0;
		desc->lli.dst_stride = 0;
	} else if ((mode & ACTS_SRC_DST_STRIDE) == ACTS_DST_ADDR_MODE_STRIDE) {
		/* frame cnt */
		desc->lli.ctrla |=  ACTS_FRAMECNT(atslave->frame_cnt);
		desc->lli.src_stride = 0;
		desc->lli.dst_stride = atslave->dst_stride;
	} else if ((mode & ACTS_SRC_DST_STRIDE) == ACTS_SRC_ADDR_MODE_STRIDE) {
		/* frame cnt */
		desc->lli.ctrla |=  ACTS_FRAMECNT(atslave->frame_cnt);
		desc->lli.src_stride = atslave->src_stride;
		desc->lli.dst_stride = 0;
	}

	desc->lli.ctrlb = 0;
	desc->lli.ctrlb |= ctrlb;
	desc->lli.ctrlb |= (ACTS_LINKLIST_SRC_VLD
			|ACTS_LINKLIST_DST_VLD);

	int_ctl = ACTS_INT_CTL_SECURE_INT
	| ACTS_INT_CTL_SUPERBLOCK_INT | ACTS_INT_CTL_ALAINED_INT;
	if (acts_chan_is_cyclic(atchan))
		int_ctl |= ACTS_INT_STATUS_END_BLOCK_INT;

	desc->lli.ctrlc = ACTS_DMAINTCTL(int_ctl);
	if (acp) {
		desc->lli.ctrlc |=  ACTS_DMAINTCTL(atslave->intctl)
		|ACTS_ACPCTL_1(atslave->acp_attr)
		|ACTS_ACPCTL_2(atslave->acp_attr)
		|ACTS_ACPCTL_3(atslave->acp_attr)
		|ACTS_ACPCTL_4(atslave->acp_attr);
	 } else
		desc->lli.ctrlb |= ACTS_LINKLIST_DIS_TYPE1;

	dev_vdbg(chan2dev(chan),
		"desrricptor: d0x%x s0x%x ctrla0x%x ctrlb0x%x ctrlc0x%x\n",
		desc->lli.daddr, desc->lli.saddr, desc->lli.ctrla,
		desc->lli.ctrlb, desc->lli.ctrlc);

	dev_vdbg(chan2dev(chan),
		"desrricptor: dtr0x%x str0x%x num0x%x next0x%x\n",
		desc->lli.dst_stride, desc->lli.src_stride, desc->lli.const_num,
		desc->lli.dscr);
}

/**
 * acts_prep_dma_memcpy - prepare a memcpy operation
 * @chan: the channel to prepare operation on
 * @dest: operation virtual destination address
 * @src: operation virtual source address
 * @len: operation length
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
acts_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
		size_t len, unsigned long flags)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
	struct owl_dma_slave	*atslave = chan->private;
	struct acts_desc		*desc = NULL;
	struct acts_desc		*first = NULL;
	struct acts_desc		*prev = NULL;
	size_t			xfer_count;
	size_t			offset;
	unsigned int		src_width;
	unsigned int		dst_width;
	u32			ctrlb;
	u32			mode;

	dev_vdbg(chan2dev(chan), "prep_dma_memcpy: d0x%x s0x%x l0x%zx f0x%lx\n",
			dest, src, len, flags);

	if (unlikely(!len)) {
		dev_dbg(chan2dev(chan), "prep_dma_memcpy: length is zero!\n");
		return NULL;
	}

	if (ACTS_SRAM_ADDR(dest) || ACTS_SRAM_ADDR(src)) {
		if (len > ACTS_SRAM_SIZE) {
			dev_err(chan2dev(chan),
				"txfer len exceed the share ram 0x%x\n", len);
			return NULL;
		}
	}

	flags |= DMA_CTRL_ACK;
	mode = atslave->mode;


	if (!((src | dest  | len) & 3))
		src_width = dst_width = 2;
	else if (!((src | dest | len) & 1))
		src_width = dst_width = 1;
	else
		src_width = dst_width = 0;


	ctrlb =  ACTS_LINKLIST_CTRLB
		| ACTS_SRC_ADDR_MODE_INCR
		| ACTS_DST_ADDR_MODE_INCR
		| ACTS_DMAMODE_1(mode)
		| ACTS_DMAMODE_2(mode)
		| ACTS_DMAMODE_4(mode)
		| ACTS_DMAMODE_5(mode);

	/*
	 * We can be a lot more clever here, but this should take care
	 * of the most common optimization.
	 */

     /* set the dma mode and  src */
	for (offset = 0; offset < len; offset += xfer_count << src_width) {
		xfer_count = min_t(size_t, (len - offset) >> src_width,
		ACTS_BTSIZE_MAX);

		desc = acts_desc_get(atchan);
		if (!desc)
			goto err_desc_get;

		desc->lli.saddr = src + offset;
		desc->lli.daddr = dest + offset;
		desc->lli.ctrla = 0;
		desc->lli.ctrla |= xfer_count << src_width;  /*frame_len */

		acts_set_ctrl(chan, desc, atslave, mode, ctrlb);

		desc->txd.cookie = 0;
		/* async_tx_ack(&desc->txd);*/

		acts_desc_chain(chan, &first, &prev, desc);

	}

	first->mode = mode
		|ACTS_LINKLIST_CTRLB
		| SRC_INCR
		| DST_INCR;

	/* First descriptor of the chain embedds additional information */
	first->txd.cookie = -EBUSY;
	first->len = len;

	/* set end-of-link to the last link descriptor of list*/
	set_desc_eol(desc);

#ifdef DMA_CACHE_ADDR
		dma_sync_single_for_device(chan2parent(chan),
			prev->txd.phys, sizeof(prev->lli),
			DMA_TO_DEVICE);
#endif

	/*desc->txd.flags = flags; *//* client is in control of this ack */
	first->txd.flags = flags;

	return &first->txd;

err_desc_get:
	acts_desc_put(atchan, first);
	return NULL;
}

static struct dma_async_tx_descriptor *acts_prep_dma_memset
(struct dma_chan *chan, dma_addr_t dest, int value,
	size_t len, unsigned long flags)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
	struct owl_dma_slave	*atslave = chan->private;
	struct acts_desc		*desc = NULL;
	struct acts_desc		*first = NULL;
	struct acts_desc		*prev = NULL;
	size_t			xfer_count;
	size_t			offset;
	unsigned int		src_width;
	unsigned int		dst_width;
	u32			ctrlb;
	u32			mode;

	dev_vdbg(chan2dev(chan), "prep_dma_memset: d0x%x val0x%x l0x%zx flg0x%lx\n",
			dest, value, len, flags);
	if (unlikely(!len)) {
		dev_dbg(chan2dev(chan), "prep_dma_memset: length is zero!\n");
		return NULL;
	}

	flags |= DMA_CTRL_ACK;
	mode = atslave->mode;
	ctrlb =  ACTS_LINKLIST_CTRLB
		| ACTS_CONSTFILL_CTRLB
		| ACTS_SRC_ADDR_MODE_INCR
		| ACTS_DST_ADDR_MODE_INCR
		| ACTS_DMAMODE_1(mode)
		| ACTS_DMAMODE_2(mode)
		| ACTS_DMAMODE_4(mode)
		| ACTS_DMAMODE_5(mode);

	/*
	 * We can be a lot more clever here, but this should take care
	 * of the most common optimization.
	 */

	if (!((dest  | len) & 3))
		src_width = dst_width = 2;
	else if (!((dest | len) & 1))
		src_width = dst_width = 1;
	else
		src_width = dst_width = 0;

	for (offset = 0; offset < len; offset += xfer_count << src_width) {
		xfer_count = min_t(size_t, (len - offset) >> src_width,
		ACTS_BTSIZE_MAX);

		desc = acts_desc_get(atchan);
		if (!desc)
			goto err_desc_get;

		desc->lli.saddr = 0;
		desc->lli.daddr = dest + offset;
		desc->lli.ctrla = 0;
		desc->lli.ctrla |= xfer_count << src_width;  /*frame_len*/
		atslave->trans_type |= ACP;
		desc->lli.const_num = value;

		acts_set_ctrl(chan, desc, atslave, mode, ctrlb);

		desc->txd.cookie = 0;
		async_tx_ack(&desc->txd);

		acts_desc_chain(chan, &first, &prev, desc);
	}

	/* First descriptor of the chain embedds additional information */
	first->txd.cookie = -EBUSY;
	first->len = len;

	/* set end-of-link to the last link descriptor of list*/
	set_desc_eol(desc);

#ifdef DMA_CACHE_ADDR
			dma_sync_single_for_device(chan2parent(chan),
			prev->txd.phys, sizeof(prev->lli), DMA_TO_DEVICE);
#endif

	first->mode = mode
		|ACTS_LINKLIST_CTRLB
		| ACTS_CONSTFILL_CTRLB
		| SRC_INCR
		| DST_INCR;

	first->txd.flags = flags;

	return &first->txd;

err_desc_get:
	acts_desc_put(atchan, first);
	return NULL;
}

/**
 * acts_prep_slave_sg - prepare descriptors for a DMA_SLAVE transaction
 * @chan: DMA channel
 * @sgl: scatterlist to transfer to/from
 * @sg_len: number of entries in @scatterlist
 * @direction: DMA direction
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
acts_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
	struct owl_dma_slave	*atslave = chan->private;
	struct dma_slave_config	*sconfig = &atchan->dma_sconfig;

	struct acts_desc		*first = NULL;
	struct acts_desc		*prev = NULL;
	u32			ctrlb;
	dma_addr_t		reg;
	/*unsigned int		reg_width;*/
	unsigned int		mem_width;
	unsigned int		i;
	struct scatterlist	*sg;
	u32			mode;
	size_t			total_len = 0;

	dev_vdbg(chan2dev(chan), "prep_slave_sg: %s f0x%lx\n",
			direction == DMA_MEM_TO_DEV ?
			"TO DEVICE" : "FROM DEVICE",
			flags);

	if (unlikely(!atslave || !sg_len)) {
		dev_dbg(chan2dev(chan), "prep_dma_slave_sg: length is zero!\n");
		return NULL;
	}

	flags |= DMA_CTRL_ACK;
	mode = atslave->mode;
	/*reg_width = atslave->reg_width;*/

	ctrlb =  ACTS_LINKLIST_CTRLB
		| ACTS_DMAMODE_1(mode)
		| ACTS_DMAMODE_2(mode)
		| ACTS_DMAMODE_3(mode)
		| ACTS_DMAMODE_4(mode)
		| ACTS_DMAMODE_5(mode);


	switch (direction) {
	case DMA_MEM_TO_DEV:

		reg = sconfig->dst_addr;
		for_each_sg(sgl, sg, sg_len, i) {
			struct acts_desc	*desc;
			u32		len;
			u32		mem;

			desc = acts_desc_get(atchan);
			if (!desc)
				goto err_desc_get;

			mem = sg_dma_address(sg);
			len = sg_dma_len(sg);
			mem_width = 2;
			if (unlikely(mem & 3 || len & 3))
				mem_width = 0;

			desc->lli.saddr = mem;
			desc->lli.daddr = reg;

#ifdef debug_nand_timeout
			if (reg > 0xb0210000 && reg < 0xb0220000)
				nand_desc = atchan->descs_allocated;
#endif

			desc->lli.ctrla = 0;
			desc->lli.ctrla |= len;

			acts_set_ctrl(chan, desc, atslave, mode, ctrlb);
			acts_desc_chain(chan, &first, &prev, desc);
			total_len += len;
		}
		break;
	case DMA_DEV_TO_MEM:

		reg = sconfig->src_addr;
		for_each_sg(sgl, sg, sg_len, i) {
			struct acts_desc	*desc;
			u32		len;
			u32		mem;

			desc = acts_desc_get(atchan);
			if (!desc)
				goto err_desc_get;

			mem = sg_dma_address(sg);
			len = sg_dma_len(sg);
			mem_width = 2;
			if (unlikely(mem & 3 || len & 3))
				mem_width = 0;

			desc->lli.saddr = reg;
			desc->lli.daddr = mem;

#ifdef debug_nand_timeout
			if (reg > 0xb0210000 && reg < 0xb0220000)
				nand_desc = atchan->descs_allocated;
#endif

			desc->lli.ctrla = 0;
			desc->lli.ctrla |= len;
			acts_set_ctrl(chan, desc, atslave, mode, ctrlb);
			acts_desc_chain(chan, &first, &prev, desc);
			total_len += len;
		}
		break;
	default:
		return NULL;
	}

	/* set end-of-link to the last link descriptor of list*/
	set_desc_eol(prev);

#ifdef DMA_CACHE_ADDR
	dma_sync_single_for_device(chan2parent(chan),
		prev->txd.phys, sizeof(prev->lli),
		DMA_TO_DEVICE);
#endif

	/* First descriptor of the chain embedds additional information */
	first->txd.cookie = -EBUSY;
	first->len = total_len;
	first->mode = mode | ACTS_LINKLIST_CTRLB;

	/* last link descriptor of list is responsible of flags */
	/*prev->txd.flags = flags;*/ /* client is in control of this ack */

	return &first->txd;

err_desc_get:
	dev_err(chan2dev(chan), "not enough descriptors available\n");
	acts_desc_put(atchan, first);
	return NULL;
}

/**
 * owl_dma_cyclic_check_values
 * Check for too big/unaligned periods and unaligned DMA buffer
 */
static int
owl_dma_cyclic_check_values(unsigned int reg_width, dma_addr_t buf_addr,
		size_t period_len, enum dma_transfer_direction direction)
{
	if (period_len > (ACTS_BTSIZE_MAX << reg_width))
		goto err_out;
	if (unlikely(period_len & ((1 << reg_width) - 1)))
		goto err_out;
	if (unlikely(buf_addr & ((1 << reg_width) - 1)))
		goto err_out;
	if (unlikely(!(direction & (DMA_DEV_TO_MEM | DMA_MEM_TO_DEV))))
		goto err_out;

	return 0;

err_out:
	return -EINVAL;
}

/**
 * acts_prep_dma_cyclic - prepare the cyclic DMA transfer
 * @chan: the DMA channel to prepare
 * @buf_addr: physical DMA address where the buffer starts
 * @buf_len: total number of bytes for the entire buffer
 * @period_len: number of bytes for each period
 * @direction: transfer direction, to or from device
 * @context: transfer context (ignored)
 */
static struct dma_async_tx_descriptor *
acts_prep_dma_cyclic(struct dma_chan *chan, dma_addr_t buf_addr, size_t buf_len,
		size_t period_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
	struct owl_dma_slave	*atslave = chan->private;
	struct dma_slave_config	*sconfig = &atchan->dma_sconfig;
	struct acts_desc		*first = NULL;
	struct acts_desc		*prev = NULL;
	unsigned long		was_cyclic;
	unsigned int		reg_width;
	unsigned int		periods = buf_len / period_len;
	unsigned int		i;
	u32		mode, ctrlb;

	dev_vdbg(chan2dev(chan), "prep_dma_cyclic: %s buf@0x%08x - %d (%d/%d)\n",
		direction == DMA_MEM_TO_DEV ? "TO DEVICE" : "FROM DEVICE",
			buf_addr,
			periods, buf_len, period_len);

	if (unlikely(!atslave || !buf_len || !period_len)) {
		dev_dbg(chan2dev(chan), "prep_dma_cyclic: length is zero!\n");
		return NULL;
	}

	was_cyclic = test_and_set_bit(ACTS_IS_CYCLIC, &atchan->status);
	if (was_cyclic) {
		dev_dbg(chan2dev(chan), "prep_dma_cyclic: channel in use!\n");
		return NULL;
	}

	mode = atslave->mode;
	ctrlb =  ACTS_LINKLIST_CTRLB
		| ACTS_DMAMODE_1(mode)
		| ACTS_DMAMODE_2(mode)
		| ACTS_DMAMODE_3(mode)
		| ACTS_DMAMODE_4(mode)
		| ACTS_DMAMODE_5(mode);

	reg_width = (mode & ACTS_MODE_BUS_WIDTH) ? 0 : 2;

	/* Check for too big/unaligned periods and unaligned DMA buffer */
	if (owl_dma_cyclic_check_values(reg_width, buf_addr,
					period_len, direction))
		goto err_out;

	/* build cyclic linked list */
	for (i = 0; i < periods; i++) {
		struct acts_desc	*desc;

		desc = acts_desc_get(atchan);
		if (!desc)
			goto err_desc_get;

	switch (direction) {
	case DMA_MEM_TO_DEV:
		desc->lli.saddr = buf_addr + (period_len * i);
		desc->lli.daddr = sconfig->dst_addr;
		break;

	case DMA_DEV_TO_MEM:
		desc->lli.saddr = sconfig->src_addr;
		desc->lli.daddr = buf_addr + (period_len * i);

		break;

	default:
		goto err_trans_type;
	}
		desc->lli.ctrla = 0;
		/*desc->lli.ctrla |= period_len >> reg_width; */
		desc->lli.ctrla |= period_len;
		acts_set_ctrl(chan, desc, atslave, mode, ctrlb);
		acts_desc_chain(chan, &first, &prev, desc);
	}

	/* lets make a cyclic list */
	prev->lli.dscr = first->txd.phys;

#ifdef DMA_CACHE_ADDR
	dma_sync_single_for_device(chan2parent(chan),
		prev->txd.phys, sizeof(prev->lli),
		DMA_TO_DEVICE);
#endif

	/* First descriptor of the chain embedds additional information */
	first->txd.cookie = -EBUSY;
	first->len = buf_len;
	first->mode = mode | ACTS_LINKLIST_CTRLB;

	return &first->txd;

err_trans_type:
	dev_err(chan2dev(chan), "wrong transfer directions\n");
err_desc_get:
	dev_err(chan2dev(chan), "not enough descriptors available\n");
	acts_desc_put(atchan, first);
err_out:
	clear_bit(ACTS_IS_CYCLIC, &atchan->status);
	return NULL;
}

static int set_runtime_config(struct dma_chan *chan,
			      struct dma_slave_config *sconfig)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);

	/* Check if it is chan is configured for slave transfers */
	if (!chan->private)
		return -EINVAL;

	memcpy(&atchan->dma_sconfig, sconfig, sizeof(*sconfig));

	return 0;
}

static int acts_pause(struct dma_chan *chan)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
	unsigned long flags, tmp;
	unsigned long id = chan->chan_id;
	struct owl_dma	*owl_dma = to_owl_dma(atchan->chan_common.device);

	if (acts_chan_is_paused(atchan))
		return 0;

	spin_lock_irqsave(&atchan->lock, flags);
	tmp = dma_readl(owl_dma, DBG_SEL);
	tmp = tmp | (0x1 << (id + 16));
	dma_writel(owl_dma, DBG_SEL, tmp);
	set_bit(ACTS_IS_PAUSED, &atchan->status);
	spin_unlock_irqrestore(&atchan->lock, flags);

	return 0;
}

static int acts_resume(struct dma_chan *chan)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
	unsigned long flags, tmp;
	unsigned long id = chan->chan_id;
	struct owl_dma	*owl_dma = to_owl_dma(atchan->chan_common.device);

	if (!acts_chan_is_paused(atchan))
		return 0;

	spin_lock_irqsave(&atchan->lock, flags);
	tmp = dma_readl(owl_dma, DBG_SEL);
	tmp = tmp & ~(0x1 << (id + 16));
	dma_writel(owl_dma, DBG_SEL, tmp);
	clear_bit(ACTS_IS_PAUSED, &atchan->status);
	spin_unlock_irqrestore(&atchan->lock, flags);

	return 0;
}

static int acts_stop(struct dma_chan *chan)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
	struct acts_desc		*desc, *_desc;
	unsigned long	flags, tmp, timeout = STOP_TIMEOUT;
	unsigned long id = chan->chan_id;
	struct owl_dma	*owl_dma = to_owl_dma(atchan->chan_common.device);

	LIST_HEAD(list);

	/*
	 * This is only called when something went wrong elsewhere, so
	 * we don't really care about the data. Just disable the
	 * channel. We still have to poll the channel enable bit due
	 * to AHB/HSB limitations.
	 */
	spin_lock_irqsave(&atchan->lock, flags);
	channel_writel(atchan, START, 0);
	spin_unlock_irqrestore(&atchan->lock, flags);
	while (!(dma_readl(owl_dma, IDLE_STAT) & (0x1 << id))) {
		udelay(5);
		timeout--;
		if (timeout == 0)
			goto err;
	}
	if (STOP_TIMEOUT - timeout > max_timeout) {
		max_timeout = STOP_TIMEOUT - timeout;
		pr_err("dma%d stop wating time is %ld, max timeout %ld\n",
			chan->chan_id, STOP_TIMEOUT - timeout, max_timeout);
	}

	dma_readl(owl_dma, IDLE_STAT);
	dma_readl(owl_dma, IDLE_STAT);
	dma_readl(owl_dma, IDLE_STAT);

	spin_lock_irqsave(&atchan->lock, flags);
	channel_writel(atchan, INT_STAT, 0x7f);
	dma_writel(owl_dma, IRQPD_0, (1 << id));
	dma_writel(owl_dma, IRQPD_1, (1 << id));
	dma_writel(owl_dma, IRQPD_2, (1 << id));
	dma_writel(owl_dma, IRQPD_3, (1 << id));

	/* stop the transfer  */
	/* active_list entries will :end up before queued entries */
	list_splice_init(&atchan->queue, &list);
	list_splice_init(&atchan->active_list, &list);

	/* Flush all pending and queued descriptors */
	list_for_each_entry_safe(desc, _desc, &list, desc_node)
		acts_chain_complete(atchan, desc);

	tmp = dma_readl(owl_dma, DBG_SEL);
	tmp = tmp & ~(0x1 << (id + 16));
	dma_writel(owl_dma, DBG_SEL, tmp);
	clear_bit(ACTS_IS_PAUSED, &atchan->status);
	clear_bit(ACTS_IS_CYCLIC, &atchan->status);
	clear_bit(ACTS_IS_INTERRUPT, &atchan->status);
	spin_unlock_irqrestore(&atchan->lock, flags);

	return 0;

err:
	pr_err("dma id %d: channel not stop\n", chan->chan_id);
	pr_err("%s, calling stack is:\n", __func__);
	pr_err("\t%pF\n", __builtin_return_address(0));

	owl_dma_dump_all(chan);
	return -EINVAL;
}

/*This function should be rewirte when using chained mode to implement cyclic*/
static int acts_read_remain_frame_cnt(struct owl_dma_chan *atchan)
{
	struct acts_desc *first;
	dma_addr_t cur_saddr, saddr;
	size_t period_len;
	int periods;
	int remain;
	struct acts_desc *tmp;

    if(list_empty(&atchan->active_list))
        return 0;
        
    first = acts_first_active(atchan);
    saddr = first->lli.saddr;
    
    periods = 1;/*the only desc in active list also counts*/
	list_for_each_entry(tmp, &first->tx_list, desc_node)
		periods++;

	period_len = first->lli.ctrla & 0xfffff;
	
	/*wurui: avoid atm7059a dma cur_src_ptr fast away from src problem*/
	while(channel_readl(atchan, CUR_SRC_PTR) - channel_readl(atchan, SRC) > period_len
		|| channel_readl(atchan, CUR_SRC_PTR) - channel_readl(atchan, SRC) < 0);
		
	cur_saddr = channel_readl(atchan, CUR_SRC_PTR);

	remain = periods - (cur_saddr - saddr) / period_len;

	return remain;
}

static int acts_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
		       unsigned long arg)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);

	dev_vdbg(chan2dev(chan), "call the acts control\n");

	if (cmd == DMA_TERMINATE_ALL)
		return acts_stop(chan);
	else if (cmd == DMA_PAUSE)
		return acts_pause(chan);
	else if (cmd == DMA_RESUME)
		return acts_resume(chan);
	else if (cmd == DMA_SLAVE_CONFIG)
		return set_runtime_config(chan, (struct dma_slave_config *)arg);
	else if ((cmd == FSLDMA_EXTERNAL_START) && (arg == 0))
		return vdbg_dump_regs(atchan);
	else if ((cmd == FSLDMA_EXTERNAL_START) && (arg == 1))
		return acts_read_remain_cnt(atchan);
	else if ((cmd == FSLDMA_EXTERNAL_START) && (arg == 2))
		return acts_read_remain_frame_cnt(atchan);
	/*else return -ENIXO;*/
	else
		return -EINVAL;
}

/**
 * acts_tx_status - poll for transaction completion
 * @chan: DMA channel
 * @cookie: transaction identifier to check status of
 * @txstate: if not %NULL updated with transaction state
 *
 * If @txstate is passed in, upon return it reflect the driver
 * internal state and can be used with dma_async_is_complete() to check
 * the status of multiple cookies without re-checking hardware state.
 */
static enum dma_status
acts_tx_status(struct dma_chan *chan,
		dma_cookie_t cookie,
		struct dma_tx_state *txstate)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
	dma_cookie_t		last_used;
	dma_cookie_t		last_complete;
	unsigned long       flags;
	enum dma_status		ret;

	spin_lock_irqsave(&atchan->lock, flags);

	ret = dma_cookie_status(chan, cookie, txstate);
	if (ret != DMA_SUCCESS) {
		acts_cleanup_descriptors(atchan);

		ret = dma_cookie_status(chan, cookie, txstate);
	}

	last_complete = chan->completed_cookie;
	last_used = chan->cookie;

	spin_unlock_irqrestore(&atchan->lock, flags);

	if (ret != DMA_SUCCESS)
		dma_set_residue(txstate, acts_first_active(atchan)->len);

	if (acts_chan_is_paused(atchan))
		ret = DMA_PAUSED;

	dev_vdbg(chan2dev(chan), "tx_status: %d (d%d, u%d)\n",
		 cookie, last_complete ? last_complete : 0,
		 last_used ? last_used : 0);

	return ret;
}

/**
 * acts_issue_pending - try to finish work
 * @chan: target DMA channel
 */
static void acts_issue_pending(struct dma_chan *chan)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
	unsigned long flags;

	dev_vdbg(chan2dev(chan), "issue_pending\n");

	if (acts_chan_is_cyclic(atchan))
		return;

	spin_lock_irqsave(&atchan->lock, flags);
	if (!acts_chan_is_enabled(atchan))
		acts_advance_work(atchan);
	spin_unlock_irqrestore(&atchan->lock, flags);
}

/**
 * acts_alloc_chan_resources - allocate resources for DMA channel
 * @chan: allocate descriptor resources for this channel
 * @client: current client requesting the channel be ready for requests
 *
 * return - the number of allocated descriptors
 */
static int acts_alloc_chan_resources(struct dma_chan *chan)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
	struct owl_dma		*owl_dma = to_owl_dma(chan->device);
	struct acts_desc		*desc;
	struct owl_dma_slave	*atslave;
	int			i;
	unsigned long flags;
	u32			cfg;
	LIST_HEAD(tmp_list);

	dev_vdbg(chan2dev(chan), "alloc_chan_resources\n");

	/* ASSERT:  channel is idle */
	if (acts_chan_is_enabled(atchan)) {
		dev_dbg(chan2dev(chan), "DMA channel not idle ?\n");
		return -EIO;
	}

	atslave = chan->private;
	if (atslave) {
		/*
		 * We need controller-specific data to set up slave
		 * transfers.
		 */
		BUG_ON(!atslave->dma_dev
			|| atslave->dma_dev != owl_dma->dma_common.dev);

		/* if cfg configuration specified take it instad of default */
		if (atslave->mode)
			cfg = atslave->mode;
	}

	/* have we already been set up?
	 * reconfigure channel but no need to reallocate descriptors */
	if (!list_empty(&atchan->free_list))
		return atchan->descs_allocated;

	/* Allocate initial pool of descriptors */
	for (i = 0; i < init_nr_desc_per_channel; i++) {
		desc = acts_alloc_descriptor(chan, GFP_KERNEL);
		if (!desc) {
			dev_err(owl_dma->dma_common.dev,
				"Only %d initial descriptors\n", i);
			break;
		}
#ifdef DMA_CACHE_ADDR
		acts_desc_put(atchan, desc);
#else
		list_add_tail(&desc->desc_node, &tmp_list);
#endif
	}

	spin_lock_irqsave(&atchan->lock, flags);
	atchan->descs_allocated = i;
#ifndef DMA_CACHE_ADDR
	list_splice(&tmp_list, &atchan->free_list);
#endif

	dma_cookie_init(chan);
	channel_writel(atchan, MODE, 0x50a00); /*avoid dev to dev */
	spin_unlock_irqrestore(&atchan->lock, flags);
	/* channel parameters */

	dev_dbg(chan2dev(chan),
		"alloc_chan_resources: allocated %d descriptors\n",
		atchan->descs_allocated);

	return atchan->descs_allocated;
}

/**
 * acts_free_chan_resources - free all channel resources
 * @chan: DMA channel
 */
static void acts_free_chan_resources(struct dma_chan *chan)
{
	struct owl_dma_chan	*atchan = to_owl_dma_chan(chan);
#ifndef DMA_CACHE_ADDR
	struct owl_dma		*owl_dma = to_owl_dma(chan->device);
#endif
	struct acts_desc	*desc, *_desc;

	LIST_HEAD(list);

	dev_dbg(chan2dev(chan), "free_chan_resources: (descs allocated=%u)\n",
		atchan->descs_allocated);

	if (!list_empty(&atchan->active_list))
		pr_err("dma:%d free the dirty channel\n", chan->chan_id);
	/* ASSERT:  channel is idle */
	BUG_ON(acts_chan_is_enabled_dump(atchan));
	BUG_ON(!list_empty(&atchan->active_list));
	BUG_ON(!list_empty(&atchan->queue));

	list_for_each_entry_safe(desc, _desc, &atchan->free_list, desc_node) {
		dev_vdbg(chan2dev(chan), "  freeing descriptor %p\n", desc);
#ifdef DMA_CACHE_ADDR
		dma_unmap_single(chan2parent(chan), desc->txd.phys,
				sizeof(desc->lli), DMA_TO_DEVICE);
		list_del(&desc->desc_node);
		kfree(desc);
#else
		list_del(&desc->desc_node);
		/* free link descriptor */
		dma_pool_free(owl_dma->dma_desc_pool, desc, desc->txd.phys);
#endif
	}
	chan->private = NULL;

	list_splice_init(&atchan->free_list, &list);
	atchan->descs_allocated = 0;

	dev_vdbg(chan2dev(chan), "free_chan_resources: done\n");
}

/*--  Module Management  -----------------------------------------------*/

/**
 * owl_dma_on - /enable/disable DMA controller
 * @owl_dma: the Acts HDAMC device
 */
static void owl_dma_on(struct owl_dma *owl_dma, bool on)
{

	if (on) {
		dma_writel(owl_dma, IRQEN_0, 0xfff);
		dma_writel(owl_dma, IRQEN_1, 0xfff);
		dma_writel(owl_dma, IRQEN_2, 0xfff);
		dma_writel(owl_dma, IRQEN_3, 0xfff);
		dma_writel(owl_dma, NIC_QOS, 0xf0);
	} else {
		dma_writel(owl_dma, IRQEN_0, 0);
		dma_writel(owl_dma, IRQEN_1, 0);
		dma_writel(owl_dma, IRQEN_2, 0);
		dma_writel(owl_dma, IRQEN_3, 0);
		dma_writel(owl_dma, NIC_QOS, 0);
	}
	dma_writel(owl_dma, IRQPD_0, 0xfff);
	dma_writel(owl_dma, IRQPD_1, 0xfff);
	dma_writel(owl_dma, IRQPD_2, 0xfff);
	dma_writel(owl_dma, IRQPD_3, 0xfff);
}

#ifdef enable_one_normal
static void enable_one_normal_channel(struct owl_dma_chan *atchan)
{
	/*
	 *1. fake device -> DCU
	 *2. DRQ trig source set as an undefined value
	 *3. use DMA channel 11
	 */
	unsigned int mode = 0x1 << 18 | 0x0 << 16 |
		0x2 << 10 | 0x0 << 8 | 0x3f << 0;
	dma_normal_buf = kzalloc(4, GFP_KERNEL);
	dma_normal_phy = dma_map_single(NULL, dma_normal_buf,
			4, DMA_FROM_DEVICE);

	channel_writel(atchan, MODE, mode);
	channel_writel(atchan, SRC, 0x0);
	channel_writel(atchan, DST, dma_normal_phy);
	channel_writel(atchan, FRAMELEN, 0x4);
	channel_writel(atchan, FRAMECNT, 0x1);
	channel_writel(atchan, INT_CTL, 0x1);
	channel_writel(atchan, START, 0x1);
}
#endif

/*
static void dma_set_affinity(int cpu, int irq)
{
	struct cpumask cpumask;

	cpumask_clear(&cpumask);
	cpumask_set_cpu(cpu, &cpumask);

	irq_set_affinity(irq, &cpumask);
	irq_set_affinity(irq + 1, &cpumask);
	irq_set_affinity(irq + 2, &cpumask);
	irq_set_affinity(irq + 3, &cpumask);
}

static int dma_irq_temp;
static int __cpuinit setdmairqaffinity_cpu_callback(struct notifier_block *nfb,
				 unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long) hcpu;

	switch (action) {
	case CPU_ONLINE:
		if (1 == cpu)
			dma_set_affinity(1, dma_irq_temp);
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block setdmairqaffinity_cpu_notifier = {
	.notifier_call = setdmairqaffinity_cpu_callback,
};
*/

static struct owl_dma_platform_data owl_dma_platform_data[] = {
	[0] = {
		.nr_channels = 12,
		.cap_mask = {
			.bits = { 0x621}
		},
	},
};

static const struct of_device_id owl_dma_of_match[] = {
    {.compatible = "actions,owl-dma", .data = &owl_dma_platform_data[0]},
    {}
};
MODULE_DEVICE_TABLE(of, owl_dma_of_match);


static int __init owl_dma_probe(struct platform_device *pdev)
{
	const struct owl_dma_platform_data *pdata;
    const struct of_device_id *id;
    struct resource *iores;
	struct owl_dma	*owl_dma;
	size_t			size;
	int			irq;
	int			err;
	int			i;

    printk("owl_dma_probe\n");
	max_timeout = 1;

	/* get DMA Controller parameters from platform */
    id = of_match_device(owl_dma_of_match, &pdev->dev);
    if(id == NULL)
    {
        printk("owl dma id is null\n");
        return -EINVAL;
    }
    pdata = id->data;
	if (!pdata || pdata->nr_channels > ACTS_DMA_MAX_NR_CHANNELS)
    {
        printk("owl dma pdata error\n");
		return -EINVAL;
	}

    iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(iores == NULL)
    {
        printk("owl dma iores is null\n");
        return -EINVAL;
    }

    irq = platform_get_irq(pdev, 0);
    if (irq < 0)
    {
        printk("owl dma irq get failed\n");
        return irq;
    }

	size = sizeof(struct owl_dma);
#ifdef enable_one_normal
	size += (pdata->nr_channels - 1) * sizeof(struct owl_dma_chan);
#else
	size += pdata->nr_channels * sizeof(struct owl_dma_chan);
#endif
	owl_dma = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);
	if (!owl_dma)
		return -ENOMEM;

	/* discover transaction capabilites from the platform data */
	owl_dma->dma_common.cap_mask = pdata->cap_mask;
#ifdef enable_one_normal
	owl_dma->all_chan_mask = (1 << (pdata->nr_channels - 1)) - 1;
#else
	owl_dma->all_chan_mask = (1 << pdata->nr_channels) - 1;
#endif
	owl_dma->id = pdev->id;

    iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(iores == NULL)
    {
        printk("owl dma iores is null\n");
        return -EINVAL;
    }

    owl_dma->regs = devm_ioremap_resource(&pdev->dev, iores);
    if (IS_ERR(owl_dma->regs))
    {
        printk("owl dma iobase remap failed\n");
        return PTR_ERR(owl_dma->regs);
    }

	owl_powergate_power_on(OWL_POWERGATE_DMA);
	
	owl_dma->clk = clk_get(&pdev->dev, "CMUMOD_DMAC");
	if (IS_ERR(owl_dma->clk)) {
		err = PTR_ERR(owl_dma->clk);
		goto err_clk;
	}
	clk_enable(owl_dma->clk);

	/* force dma off, just in case */
	owl_dma_on(owl_dma, 0);


	err = request_irq(irq, owl_dma_interrupt, 0, "owl_dma0", owl_dma);
	if (err)
		goto err_irq;
	err = request_irq(irq + 1, owl_dma_interrupt, 0, "owl_dma1", owl_dma);
	if (err)
		goto err_irq;
	err = request_irq(irq + 2, owl_dma_interrupt, 0, "owl_dma2", owl_dma);
	if (err)
		goto err_irq;
	err = request_irq(irq + 3, owl_dma_interrupt, 0, "owl_dma3", owl_dma);
	if (err)
		goto err_irq;

	/* dma_set_affinity(1, irq); */ 
	owl_dma->irq = irq;

	platform_set_drvdata(pdev, owl_dma);

#ifndef DMA_CACHE_ADDR
	/* create a pool of consistent memory blocks for hardware descriptors */
	owl_dma->dma_desc_pool = dma_pool_create("acts_hdmac_desc_pool",
			&pdev->dev, sizeof(struct acts_desc),
			4 /* word alignment */, 0);
	if (!owl_dma->dma_desc_pool) {
		dev_err(&pdev->dev, "No memory for descriptors dma pool\n");
		err = -ENOMEM;
		goto err_pool_create;
	}
#endif
	/* clear any pending interrupt */

	/* initialize channels related values */
	INIT_LIST_HEAD(&owl_dma->dma_common.channels);
#ifdef enable_one_normal
	for (i = 0; i < (pdata->nr_channels - 1);
		i++, owl_dma->dma_common.chancnt++)
#else
	for (i = 0; i < pdata->nr_channels;
		i++, owl_dma->dma_common.chancnt++)
#endif
	{
		struct owl_dma_chan	*atchan = &owl_dma->chan[i];

		atchan->chan_common.device = &owl_dma->dma_common;
		dma_cookie_init(&atchan->chan_common);
		atchan->chan_common.chan_id = i;
		list_add_tail(&atchan->chan_common.device_node,
				&owl_dma->dma_common.channels);

		atchan->ch_regs = owl_dma->regs + ch_regs(i);
		spin_lock_init(&atchan->lock);
		atchan->mask = 1 << i;

		INIT_LIST_HEAD(&atchan->active_list);
		INIT_LIST_HEAD(&atchan->queue);
		INIT_LIST_HEAD(&atchan->free_list);

		acts_enable_irq(atchan);
	}

	tasklet_init(&owl_dma->tasklet, acts_tasklet,
			(unsigned long)owl_dma);

	/* set base routines */
	owl_dma->dma_common.device_alloc_chan_resources =
		acts_alloc_chan_resources;
	owl_dma->dma_common.device_free_chan_resources =
		acts_free_chan_resources;
	owl_dma->dma_common.device_tx_status = acts_tx_status;
	owl_dma->dma_common.device_issue_pending = acts_issue_pending;
	owl_dma->dma_common.dev = &pdev->dev;

	/* set prep routines based on capability */
	if (dma_has_cap(DMA_MEMCPY, owl_dma->dma_common.cap_mask))
		owl_dma->dma_common.device_prep_dma_memcpy =
			acts_prep_dma_memcpy;

	if (dma_has_cap(DMA_SLAVE, owl_dma->dma_common.cap_mask)) {
		owl_dma->dma_common.device_prep_slave_sg =
			acts_prep_slave_sg;
		dma_cap_set(DMA_CYCLIC, owl_dma->dma_common.cap_mask);
		owl_dma->dma_common.device_prep_dma_cyclic =
			acts_prep_dma_cyclic;
		owl_dma->dma_common.device_control = acts_control;
	}
	if (dma_has_cap(DMA_MEMSET, owl_dma->dma_common.cap_mask))
		owl_dma->dma_common.device_prep_dma_memset =
			acts_prep_dma_memset;


	owl_dma_on(owl_dma, 1); /* enable dma*/

	dev_info(&pdev->dev,
		"actions AHB DMA Controller ( %s%s%s), %d channels\n",
		dma_has_cap(DMA_MEMCPY,
			owl_dma->dma_common.cap_mask) ? "cpy " : "",
		dma_has_cap(DMA_SLAVE,
			owl_dma->dma_common.cap_mask)  ? "slave " : "",
		dma_has_cap(DMA_MEMSET,
			owl_dma->dma_common.cap_mask)  ? "memset " : "",
		owl_dma->dma_common.chancnt);

	dma_async_device_register(&owl_dma->dma_common);

#ifdef enable_one_normal
	enable_one_normal_channel(&owl_dma->chan[owl_dma->dma_common.chancnt-1]);
#endif

	return 0;

#ifndef DMA_CACHE_ADDR
err_pool_create:
#endif
	platform_set_drvdata(pdev, NULL);
	free_irq(platform_get_irq(pdev, 0), owl_dma);

err_irq:
	clk_disable(owl_dma->clk);
	clk_put(owl_dma->clk);
err_clk:
	return err;
}

static int __exit owl_dma_remove(struct platform_device *pdev)
{
	struct owl_dma		*owl_dma = platform_get_drvdata(pdev);
	struct dma_chan		*chan, *_chan;
    struct owl_dma_chan	*atchan;
    
	owl_dma_on(owl_dma, 0);
	dma_async_device_unregister(&owl_dma->dma_common);

#ifndef DMA_CACHE_ADDR
	dma_pool_destroy(owl_dma->dma_desc_pool);
#endif
	platform_set_drvdata(pdev, NULL);
	free_irq(platform_get_irq(pdev, 0), owl_dma);

	list_for_each_entry_safe(chan, _chan, &owl_dma->dma_common.channels,
			device_node) {
		atchan = to_owl_dma_chan(chan);

		/* Disable interrupts */
		acts_disable_irq(atchan);
		list_del(&chan->device_node);
	}

	tasklet_disable(&owl_dma->tasklet);
	tasklet_kill(&owl_dma->tasklet);

	clk_disable(owl_dma->clk);
	clk_put(owl_dma->clk);


#ifdef enable_one_normal
    atchan = &owl_dma->chan[owl_dma->dma_common.chancnt-1];
	channel_writel(atchan, START, 0x0);
	dma_unmap_single(NULL, dma_normal_phy, 4, DMA_FROM_DEVICE);
	kfree(dma_normal_buf);
#endif

	kfree(owl_dma);
	return 0;
}

//static void owl_dma_shutdown(struct platform_device *pdev)
//{
//	struct owl_dma	*owl_dma = platform_get_drvdata(pdev);
//
//	owl_dma_on(platform_get_drvdata(pdev), 0);
//	clk_disable(owl_dma->clk);
//}

static void acts_suspend_cyclic(struct owl_dma_chan *atchan)
{
	struct dma_chan	*chan = &atchan->chan_common;

	/* Channel should be paused by user
	 * do it anyway even if it is not done already */
	if (!acts_chan_is_paused(atchan)) {
		dev_warn(chan2dev(chan),
		"cyclic channel not paused, should be done by channel user\n");
		acts_control(chan, DMA_PAUSE, 0);
	}

	/* now preserve additional data for cyclic operations */
	/* next descriptor address in the cyclic list */
	atchan->save_dscr = channel_readl(atchan, NEXT_DESC);

	/* vdbg_dump_regs(atchan); */
}

static void acts_resume_cyclic(struct owl_dma_chan *atchan)
{
	int int_ctl;

	/* restore channel status for cyclic descriptors list:
	 * next descriptor in the cyclic list at the time of suspend */
	channel_writel(atchan, SRC, 0);
	channel_writel(atchan, DST, 0);
	channel_writel(atchan, NEXT_DESC, atchan->save_dscr);
	int_ctl = channel_readl(atchan, INT_CTL);
	channel_writel(atchan, INT_CTL, int_ctl | ACTS_INT_CTL_END_BLOCK_INT);

	/* channel pause status should be removed by channel user
	 * We cannot take the initiative to do it here */

	/* vdbg_dump_regs(atchan); */
}

static int owl_dma_suspend_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct owl_dma *owl_dma = platform_get_drvdata(pdev);
	struct dma_chan *chan, *_chan;

	/* preserve data */
	list_for_each_entry_safe(chan, _chan, &owl_dma->dma_common.channels,
			device_node) {
		struct owl_dma_chan *atchan = to_owl_dma_chan(chan);

		/* here, all channels should be idled already
		 * (cyclic channel is paused)
		 */
		if (acts_chan_is_enabled(atchan)
			&& !acts_chan_is_cyclic(atchan)) {
			vdbg_dump_regs(atchan);
			pr_alert("%s:DMA channel%d is not idled yet!!!\n",
					__func__, atchan->chan_common.chan_id);
		}

		if (acts_chan_is_cyclic(atchan))
			acts_suspend_cyclic(atchan);
		atchan->save_mode = channel_readl(atchan, MODE);
		atchan->save_ll = channel_readl(atchan,	LINKLIST);
		acts_disable_irq(atchan);
	}
		owl_dma->save_nicqos = dma_readl(owl_dma, NIC_QOS);

	/* disable DMA controller */
	owl_dma_on(owl_dma, 0);
	clk_disable(owl_dma->clk);
	return 0;
}

static int owl_dma_resume_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct owl_dma *owl_dma = platform_get_drvdata(pdev);
	struct dma_chan *chan, *_chan;

    module_reset(MODULE_RST_DMAC);

	/* bring back DMA controller */
	clk_enable(owl_dma->clk);
	owl_dma_on(owl_dma, 1);

	/* restore saved data */
	dma_writel(owl_dma, NIC_QOS, owl_dma->save_nicqos);
	list_for_each_entry_safe(chan, _chan, &owl_dma->dma_common.channels,
			device_node) {
		struct owl_dma_chan *atchan = to_owl_dma_chan(chan);

		acts_enable_irq(atchan);
		channel_writel(atchan, MODE, atchan->save_mode);
		channel_writel(atchan, LINKLIST, atchan->save_ll);
		if (acts_chan_is_cyclic(atchan))
			acts_resume_cyclic(atchan);
	}
	return 0;
}

static const struct dev_pm_ops owl_dma_dev_pm_ops = {
	.suspend_noirq = owl_dma_suspend_noirq,
	.resume_noirq = owl_dma_resume_noirq,
};

static struct platform_driver owl_dma_driver = {
    .driver = {
        .name = "owl_dma",
		.pm	= &owl_dma_dev_pm_ops,
        .owner = THIS_MODULE,
        .of_match_table = owl_dma_of_match,
    },
    .probe = owl_dma_probe,
    .remove = __exit_p(owl_dma_remove),
//	.shutdown	= owl_dma_shutdown,
};

static int __init owl_dma_init(void)
{
	int err;
    
    printk("owl_dma_init\n");
    err = platform_driver_register(&owl_dma_driver);
    if (err != 0) {
        printk("register owl dma platform driver error!\n");
        return err;
    } 

	return err;
}
subsys_initcall(owl_dma_init);

static void __exit owl_dma_exit(void)
{
    platform_driver_unregister(&owl_dma_driver);
}
module_exit(owl_dma_exit);

MODULE_DESCRIPTION("actions AHB DMA Controller driver");
MODULE_AUTHOR("mdchen");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:acts_hdmac");
