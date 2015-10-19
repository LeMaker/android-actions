/*
 * Header file for the Atmel AHB DMA Controller driver
 *
 * Copyright (C) 2008 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef ACTS_HDMAC_REGS_H
#define	ACTS_HDMAC_REGS_H

#include <mach/hardware.h>
#include <mach/hdmac-owl.h>

#define	ACTS_DMA_MAX_NR_CHANNELS	12


#define	ACTS_DMA_IRQPD_0		0x00
#define	ACTS_DMA_IRQPD_1		0x04
#define	ACTS_DMA_IRQPD_2		0x08
#define	ACTS_DMA_IRQPD_3		0x0c
#define	ACTS_DMA_IRQEN_0		0x10
#define	ACTS_DMA_IRQEN_1		0x14
#define	ACTS_DMA_IRQEN_2		0x18
#define	ACTS_DMA_IRQEN_3		0x1c
#define	ACTS_DMA_SECURE_ACCESS_CTL	0x20
#define ACTS_DMA_NIC_QOS		0x24
#define ACTS_DMA_DBG_SEL		0x28
#define ACTS_DMA_IDLE_STAT		0x2c

#define	ACTS_DMA_CH_REGS_BASE		0x100

#define ACTS_DMA_PENDING_MASK(x)	(0x1 << (x))

#define ACTS_BTSIZE_MAX			0x8000

#define	ch_regs(x)	(ACTS_DMA_CH_REGS_BASE + (x) * 0x100)
/* Hardware register offset for each channel */
#define	ACTS_MODE_OFFSET		0x00
#define	ACTS_SRC_OFFSET			0x04
#define	ACTS_DST_OFFSET			0x08
#define	ACTS_FRAMELEN_OFFSET		0x0C
#define	ACTS_FRAMECNT_OFFSET		0x10
#define	ACTS_REMAIN_FRAME_OFFSET	0x14
#define	ACTS_REMAIN_CNT_OFFSET		0x18
#define	ACTS_SRC_STRIDE_OFFSET		0x1C
#define	ACTS_DST_STRIDE_OFFSET		0x20
#define	ACTS_START_OFFSET		0x24
#define	ACTS_ACP_ATTR_OFFSET		0x28
#define	ACTS_CHAINED_CTL_OFFSET		0x2c
#define	ACTS_CONSTANT_OFFSET		0x30
#define	ACTS_LINKLIST_OFFSET		0x34
#define	ACTS_NEXT_DESC_OFFSET		0x38
#define	ACTS_CUR_DESC_NUM_OFFSET	0x3C
#define	ACTS_INT_CTL_OFFSET		0x40
#define	ACTS_INT_STAT_OFFSET		0x44
#define	ACTS_CUR_SRC_PTR_OFFSET		0x48
#define	ACTS_CUR_DST_PTR_OFFSET		0x4c

#define ACTS_MODE_BUS_WIDTH		0x10000000

#define ACTS_SRAM_ADDR(x)   (((x) & 0xfffe0000) == 0xb4060000)
#define ACTS_SRAM_SIZE			0x10000

#define DMA_ERROR			0x60

#define ACTS_CHAINED_CTRLB		0x80000000
#define ACTS_LINKLIST_CTRLB		0x40000000
#define ACTS_CONSTFILL_CTRLB		0x20000000
#define ACTS_SRC_ADDR_MODE_INCR		0x00100000
#define ACTS_SRC_ADDR_MODE_STRIDE	0x00200000
#define ACTS_DST_ADDR_MODE_INCR		0x00400000
#define ACTS_DST_ADDR_MODE_STRIDE	0x00800000
#define ACTS_FC_MEM2MEM			0x00050000

#define ACTS_LINKLIST_SRC_DST_VLD	0x500

#define ACTS_LINKLIST_SRC_CONST		0x00000080
#define ACTS_LINKLIST_SRC_VLD		0x00000040
#define ACTS_LINKLIST_DST_VLD		0x00000100
#define ACTS_LINKLIST_DST_CONST		0x00000200
#define ACTS_LINKLIST_DIS_TYPE1		0x00000001
#define ACTS_LINKLIST_DIS_TYPE2		0x00000002

#define ACTS_INT_CTL_SECURE_INT		0x00000040
#define ACTS_INT_CTL_ALAINED_INT	0x00000020
#define ACTS_INT_CTL_LAST_FRAME_INT	0x00000010
#define ACTS_INT_CTL_HALF_FRAME_INT	0x00000008
#define ACTS_INT_CTL_END_FRAME_INT	0x00000004
#define ACTS_INT_CTL_SUPERBLOCK_INT	0x00000002
#define ACTS_INT_CTL_END_BLOCK_INT	0x00000001


#define ACTS_INT_STATUS_SECURE_ERROR	0x00000040
#define ACTS_INT_STATUS_ALAINED_ERROR	0x00000020
#define ACTS_INT_STATUS_LAST_FRAME	0x00000010
#define ACTS_INT_STATUS_HALF_FRAME	0x00000008
#define ACTS_INT_STATUS_END_FRAME	0x00000004
#define ACTS_INT_STATUS_SUPERBLOCK	0x00000002
#define ACTS_INT_STATUS_END_BLOCK_INT	0x00000001

#define PAUSE_CHANNEL			0x80000000
#define ACTS_SRC_DST_STRIDE		0xa0000

/* Bitfield definitions */
#define ACTS_FRAMECNT(x)  (((x) << 20) & 0xfff00000)

/* ctrlb */
#define ACTS_DMAMODE_1(x) ((x) & 0xF0000000)		/*[31:28]*/
#define ACTS_DMAMODE_2(x) (((x) << 4) & 0xf000000)	/*[23:20]*/
#define ACTS_DMAMODE_3(x) (((x) << 4) & 0x0f00000)	/*[20:16]*/
#define ACTS_DMAMODE_4(x) (((x) << 8) & 0xf0000)	/*[11:8]*/
#define ACTS_DMAMODE_5(x) (((x) << 10) & 0xfc00)	/*[5:0]*/

#define ACTS_CHAINEDCTL_2(x) (((x) << 2) & 0x3c)

/*ctrlc*/
#define ACTS_DMAINTCTL(x) (((x) << 18) & 0x1fc0000)
#define ACTS_ACPCTL_1(x) (((x) << 13)  & 0x3e000)	/*acp attr[4:0]*/
#define ACTS_ACPCTL_2(x) ((x) & 0x1f00)			/*acp attr[12:8]*/
#define ACTS_ACPCTL_3(x) (((x) >> 12) & 0xf0)		/*acp attr[19:16]*/
#define ACTS_ACPCTL_4(x) (((x) >> 24) & 0xf)		/*acp attr[19:16]*/

/*--  descriptors  -----------------------------------------------------*/

struct  acts_lli {
	dma_addr_t dscr;
	dma_addr_t	saddr;
	dma_addr_t	daddr;
	u32		ctrla;  /*frame len and cnt*/
	u32		src_stride;
	u32		dst_stride;
	u32		ctrlb; /*dma_mode and linklist ctrl */
	u32		ctrlc;  /*acp attribute;*/
	u32		const_num;
};
/**
 * struct acts_desc - software descriptor
 * @acts_lli: hardware lli structure
 * @txd: support for the async_tx api
 * @desc_node: node on the channed descriptors list
 * @len: total transaction bytecount
 */

struct acts_desc {
	/* FIRST values the hardware uses */

	struct acts_lli			lli;

	/* THEN values for driver housekeeping */
	struct list_head		tx_list;
	struct dma_async_tx_descriptor	txd;
	struct list_head		desc_node;
	size_t				len;
	u32				mode;
};

static inline struct acts_desc *
txd_to_acts_desc(struct dma_async_tx_descriptor *txd)
{
	return container_of(txd, struct acts_desc, txd);
}


enum acts_status {
		ACTS_IS_ERROR = 0,
		ACTS_IS_PAUSED = (1 << 1),
		ACTS_IS_CYCLIC = (1 << 2),
		ACTS_IS_INTERRUPT = (1 << 4),
};

/*--  Channels  --------------------------------------------------------*/

/**
 * struct owl_dma_chan - internal representation of an Atmel HDMAC channel
 * @chan_common: common dmaengine channel object members
 * @device: parent device
 * @ch_regs: memory mapped register base
 * @mask: channel index in a mask
 * @error_status: transmit error status information from irq handler
 *                to tasklet (use atomic operations)
 * @tasklet: bottom half to finish transaction work
 * @lock: serializes enqueue/dequeue operations to descriptors lists
 * @completed_cookie: identifier for the most recently completed operation
 * @active_list: list of descriptors dmaengine is being running on
 * @queue: list of descriptors ready to be submitted to engine
 * @free_list: list of descriptors usable by the channel
 * @descs_allocated: records the actual size of the descriptor pool
 */
struct owl_dma_chan {
	struct dma_chan		chan_common;
	struct owl_dma		*device;
	void __iomem		*ch_regs;
	u32			mask;
	unsigned long		status;
#ifdef COMPLEX_INT
	struct tasklet_struct	tasklet;
#endif
	atomic_t		channel_pending;
	spinlock_t		lock;
	struct dma_slave_config dma_sconfig;
	u32			save_dscr;
	u32			save_mode;
	u32			save_ll;

	/* these other elements are all protected by lock */
	dma_cookie_t		completed_cookie;
	struct list_head	active_list;
	struct list_head	queue;
	struct list_head	free_list;
	unsigned int		descs_allocated;
};

#define	channel_readl(atchan, name) \
	readl_relaxed((atchan)->ch_regs + ACTS_##name##_OFFSET)

#define	channel_writel(atchan, name, val) \
	writel_relaxed((val), (atchan)->ch_regs + ACTS_##name##_OFFSET)


static inline struct owl_dma_chan *to_owl_dma_chan(struct dma_chan *dchan)
{
	return container_of(dchan, struct owl_dma_chan, chan_common);
}

/*--  Controller  ------------------------------------------------------*/

/**
 * struct owl_dma - internal representation of an Atmel HDMA Controller
 * @chan_common: common dmaengine dma_device object members
 * @ch_regs: memory mapped register base
 * @clk: dma controller clock
 * @all_chan_mask: all channels availlable in a mask
 * @dma_desc_pool: base of DMA descriptor region (DMA address)
 * @chan: channels table to store owl_dma_chan structures
 */
struct owl_dma {
	struct dma_device	dma_common;
	void __iomem		*regs;
	struct clk		*clk;
	int irq;

	u32			all_chan_mask;
	u32			id;
#ifndef COMPLEX_INT
	struct tasklet_struct	tasklet;
	atomic_t		pending;
#endif
	u32			save_nicqos;
	struct dma_pool		*dma_desc_pool;
	/* AT THE END channels table */
	struct owl_dma_chan	chan[0];
};

#define	dma_readl(atdma, name) \
	readl_relaxed((atdma)->regs + ACTS_DMA_##name)
#define	dma_writel(atdma, name, val) \
	writel_relaxed((val), (atdma)->regs + ACTS_DMA_##name)

static inline struct owl_dma *to_owl_dma(struct dma_device *ddev)
{
	return container_of(ddev, struct owl_dma, dma_common);
}

/*--  Helper functions  ------------------------------------------------*/

static struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}
static struct device *chan2parent(struct dma_chan *chan)
{
	return chan->dev->device.parent;
}

static int vdbg_dump_regs(struct owl_dma_chan *atchan)
{
	struct owl_dma	*atdma = to_owl_dma(atchan->chan_common.device);

	dev_err(chan2dev(&atchan->chan_common),
		"  channel %d : irqpd = 0x%x, irqen = 0x%x, dbg_sel:0x%x"
		"  idle stat:0x%x\n",
		atchan->chan_common.chan_id,
		dma_readl(atdma, IRQPD_0),
		dma_readl(atdma, IRQEN_0),
		dma_readl(atdma, DBG_SEL),
		dma_readl(atdma, IDLE_STAT));

	dev_err(chan2dev(&atchan->chan_common),
		"  channel: s0x%x d0x%x mode0x%x:0x%x next_des0x%x framelen0x%x intctl0x%x intstat0x%x\n",
		channel_readl(atchan, SRC),
		channel_readl(atchan, DST),
		channel_readl(atchan, MODE),
		channel_readl(atchan, LINKLIST),
		channel_readl(atchan, NEXT_DESC),
		channel_readl(atchan, FRAMELEN),
		channel_readl(atchan, INT_CTL),
		channel_readl(atchan, INT_STAT));
	dev_err(chan2dev(&atchan->chan_common),
		"  channel: framecnt0x%x desc num0x%x cur sptr0x%x cur dptr0x%x remaincnt0x%x remainframe0x%x\n"
		" start:0x%x\n",
		channel_readl(atchan, FRAMECNT),
		channel_readl(atchan, CUR_DESC_NUM),
		channel_readl(atchan, CUR_SRC_PTR),
		channel_readl(atchan, CUR_DST_PTR),
		channel_readl(atchan, REMAIN_CNT),
		channel_readl(atchan, REMAIN_FRAME),
		channel_readl(atchan, START));

	return 0;
}

static void acts_dump_lli(struct owl_dma_chan *atchan, struct acts_lli *lli)
{
	dev_printk(KERN_CRIT, chan2dev(&atchan->chan_common),
		"  desc: s0x%x d0x%x ctrl0x%x:0x%x ctrlc0x%x l0x%x\n",
		lli->saddr, lli->daddr,
		lli->ctrla, lli->ctrlb, lli->ctrlc, lli->dscr);
}

static void acts_setup_irq(struct owl_dma_chan *atchan, int on)
{
	int int_ctl;

	int_ctl = ACTS_INT_CTL_SECURE_INT
		| ACTS_INT_CTL_ALAINED_INT
		| ACTS_INT_CTL_SUPERBLOCK_INT;

	if (on)
		channel_writel(atchan, INT_CTL, int_ctl);
	else
		channel_writel(atchan, INT_CTL, 0);
	channel_writel(atchan, INT_STAT, 0xffff);
}

static inline void acts_enable_irq(struct owl_dma_chan *atchan)
{
	acts_setup_irq(atchan, 1);
}

static inline void acts_disable_irq(struct owl_dma_chan *atchan)
{
	acts_setup_irq(atchan, 0);
}

/**
 * atc_chan_is_paused - test channel pause/resume status
 * @atchan: channel we want to test status
 */
static inline int acts_chan_is_paused(struct owl_dma_chan *atchan)
{
	return test_bit(ACTS_IS_PAUSED, &atchan->status);
}

/**
 * atc_chan_is_cyclic - test if given channel has cyclic property set
 * @atchan: channel we want to test status
 */
static inline int acts_chan_is_cyclic(struct owl_dma_chan *atchan)
{
	return test_bit(ACTS_IS_CYCLIC, &atchan->status);
}

/**
 * acts_chan_is_enabled - test if given channel is enabled
 * @atchan: channel we want to test status
 */
static inline int acts_chan_is_enabled(struct owl_dma_chan *atchan)
{
	struct owl_dma	*asoc_dma = to_owl_dma(atchan->chan_common.device);
	struct dma_chan	*chan = &atchan->chan_common;
	unsigned long id = chan->chan_id;
	unsigned int idle, pause;

	idle = dma_readl(asoc_dma, IDLE_STAT);
	idle = idle & (0x1 << id);
	pause = dma_readl(asoc_dma, DBG_SEL);
	pause = pause & (0x1 << (id + 16));

	if (idle && !pause)
		return 0;
	else
		return 1;
}

static inline int acts_chan_is_enabled_dump(struct owl_dma_chan *atchan)
{
	struct owl_dma	*atdma = to_owl_dma(atchan->chan_common.device);

	if (acts_chan_is_enabled(atchan)) {
		dev_err(chan2dev(&atchan->chan_common),
				"  channel %d : dbg_sel:0x%x idle stat:0x%x\n",
				atchan->chan_common.chan_id,
				dma_readl(atdma, DBG_SEL),
				dma_readl(atdma, IDLE_STAT));

		dev_err(chan2dev(&atchan->chan_common),
				"  channel: s0x%x d0x%x mode:0x%x frame_len:0x%x frame_cnt0x%x\n",
				channel_readl(atchan, SRC),
				channel_readl(atchan, DST),
				channel_readl(atchan, MODE),
				channel_readl(atchan, FRAMELEN),
				channel_readl(atchan, FRAMECNT));

		dev_err(chan2dev(&atchan->chan_common),
				"  channel: acp:0x%x,chain ctl:0x%x,llist:0x%x,int_ctl:0x%x,int_stat:0x%x\n",
				channel_readl(atchan, ACP_ATTR),
				channel_readl(atchan, CHAINED_CTL),
				channel_readl(atchan, LINKLIST),
				channel_readl(atchan, INT_CTL),
				channel_readl(atchan, INT_STAT));
		return 1;
	} else
		return 0;
}

/**
 * set_desc_eol - set end-of-link to descriptor so it will end transfer
 * @desc: descriptor, signle or at the end of a chain, to end chain on
 */
static void set_desc_eol(struct acts_desc *desc)
{
	desc->lli.dscr = 0;
	desc->lli.ctrlb &= ~ACTS_LINKLIST_CTRLB;
}

static int acts_read_remain_cnt(struct owl_dma_chan *atchan)
{
	return channel_readl(atchan, REMAIN_CNT);
}
#endif /* ACTS_HDMAC_REGS_H */
