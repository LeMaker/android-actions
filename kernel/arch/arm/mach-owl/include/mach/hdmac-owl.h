/*
 * Header file for the actions AHB DMA Controller driver
 *
 * Copyright (C) 2012 actions Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef HDMAC_OWL_H
#define HDMAC_OWL_H

#include <linux/dmaengine.h>

/**
 * struct owl_dma_platform_data - Controller configuration parameters
 * @nr_channels: Number of channels supported by hardware (max 8)
 * @cap_mask: dma_capability flags supported by the platform
 */
struct owl_dma_platform_data {
	unsigned int	nr_channels;
	dma_cap_mask_t  cap_mask;
};

/**
 * enum owl_dma_slave_width - DMA slave register access width.
 * @AT_DMA_SLAVE_WIDTH_8BIT: Do 8-bit slave register accesses
 * @AT_DMA_SLAVE_WIDTH_16BIT: Do 16-bit slave register accesses
 * @AT_DMA_SLAVE_WIDTH_32BIT: Do 32-bit slave register accesses
 */
enum owl_dma_slave_width {
	ACT_DMA_SLAVE_WIDTH_8BIT = 0,
	ACT_DMA_SLAVE_WIDTH_16BIT,
	ACT_DMA_SLAVE_WIDTH_32BIT,
};

enum owl_trans_type {
	MEMCPY = 0,
	SLAVE  = 1,
	MEMSET = 2,
	ACP    = 4,
	CHAINED = 8,
};

static inline struct dma_async_tx_descriptor *dmaengine_prep_memcpy
	(struct dma_chan *chan, dma_addr_t dst, dma_addr_t src,
size_t len, unsigned long flags)
{
	return chan->device->device_prep_dma_memcpy(chan, dst, src, len, flags);
}

static inline struct dma_async_tx_descriptor *dmaengine_prep_memset
	(struct dma_chan *chan, dma_addr_t dst, int value,
	size_t len, unsigned long flags)
{
	return chan->device->device_prep_dma_memset(chan, dst, value, len, flags);
}

static inline int owl_dma_dump_all(struct dma_chan *chan)
{
	return chan->device->device_control(chan, FSLDMA_EXTERNAL_START, 0);
}

static inline int read_remain_cnt(struct dma_chan *chan)
{
	return chan->device->device_control(chan, FSLDMA_EXTERNAL_START, 1);
}

static inline int read_remain_frame_cnt(struct dma_chan *chan)
{
	return chan->device->device_control(chan, FSLDMA_EXTERNAL_START, 2);
}

/**
 * struct owl_dma_slave - Controller-specific information about a slave
 * @dma_dev: required DMA master device
 * @tx_reg: physical address of data register used for
 *	memory-to-peripheral transfers
 * @rx_reg: physical address of data register used for
 *	peripheral-to-memory transfers
 * @reg_width: peripheral register width
 * @cfg: Platform-specific initializer for the CFG register
 * @ctrla: Platform-specific initializer for the CTRLA register
 */
struct owl_dma_slave {
	struct device		*dma_dev;
	enum owl_trans_type trans_type;
#ifdef DMA_GL5201
	u32			cfg;
	u32			ctrla;
#else
	u32			mode;
	u32			frame_len;
	u32			frame_cnt;
	u32			src_stride;
	u32			dst_stride;
	u32			acp_attr;
	u32			chained_ctl;
	u32			intctl;
	u32			secure_ctl;
	u32			nic_qos;
#endif
};

enum priority_weight {
	PRIORITY_ZERO = 0x0,
	PRIORITY_ONE = (0x1 << 20),
	PRIORITY_TWO = (0x2 << 20),
	PRIORITY_THREE = (0x3 << 20),
	PRIORITY_FOUR = (0x4 << 20),
	PRIORITY_FIVE = (0x5 << 20),
	PRIORITY_SIX = (0x6 << 20),
	PRIORITY_SEVEN = (0x7 << 20),
};

enum src_srcmode {
	SRC_CONSTANT = 0x0,
	SRC_INCR = (0x1 << 16),
	SRC_STRIDE = (0x2 << 16),
};

enum dst_dstmode {
	DST_CONSTANT = 0x0,
	DST_INCR = (0x1 << 18),
	DST_STRIDE = (0x2 << 18),
};

enum dst_type {
	DST_DEV = 0x0,
	DST_ACP = (0x1 << 10),
	DST_DCU = (0x2 << 10),
	DST_SRAM = (0x3 << 10),
};

enum src_type {
	SRC_DEV = 0x0,
	SRC_ACP = (0x1 << 8),
	SRC_DCU = (0x2 << 8),
	SRC_SRAM = (0x3 << 8),
};

enum trigger_source {
	DCU = 0x0,
	SRAM,
	SD0,
	SD1,
	SD2,
	NANDCMD,
	NANDDATA,
	I2S_T,
	I2S_R,
	PCM0_T,
	PCM0_R,
	PCM1_T,
	PCM1_R,
	SPDIF,
	HDMIAUDIO,
	SPDIF_HDMI,
	UART0_T,
	UART0_R,
	UART1_T,
	UART1_R,
	UART2_T,
	UART2_R,
	UART3_T,
	UART3_R,
	UART4_T,
	UART4_R,
	UART5_T,
	UART5_R,
	SPI0_T,
	SPI0_R,
	SPI1_T,
	SPI1_R,
	SPI2_T,
	SPI2_R,
	SPI3_T,
	SPI3_R,
	DSI_T,
	DSI_R,
	SRAMI,
	AUDIP,
	HDCP_T,
	HDCP_R,
	UART6_T,
	UART6_R,
};


#define BUS_WIDTH_8BIT 0x10000000
#define CRITICAL_BIT 0x800000



#endif /* HDMAC_OWL_H */
