#ifndef _ATV5203_DMA_H_
#define _ATV5203_DMA_H_





/*
 * Simple DMA transfer operations defines for MMC/SD card
 */
#define DMA_MODE_OFFSET				0x0000
#define DMA_SOURCE_OFFSET			0x0004
#define DMA_DESTINATION_OFFSET	 		0x0008
#define DMA_FRAME_LEN_OFFSET			0x000C
#define DMA_FRAME_CNT_OFFSET			0x0010
#define DMA_REMAIN_FRAME_CNT_OFFSET		0x0014
#define DMA_REMAIN_CNT_OFFSET			0x0018
#define DMA_SOURCE_STRIDE_OFFSET		0x001C
#define DMA_DESTINATION_STRIDE_OFFSET		0x0020
#define DMA_START_OFFSET			0x0024
#define DMA_ACP_ATTRIBUTE_OFFSET		0x0028
#define DMA_CHAINED_CTL_OFFSET			0x002C
#define DMA_CONSTANT_OFFSET			0x0030
#define DMA_LINKLIST_CTL_OFFSET			0x0034
#define DMA_NEXT_DESCRIPTOR_OFFSET		0x0038
#define DMA_CURRENT_DESCRIPTOR_NUM_OFFSET	0x003C
#define DMA_INT_CTL_OFFSET			0x0040
#define DMA_INT_STATUS_OFFSET			0x0044
#define DMA_CURRENT_SOURCE_POINTER_OFFSET	0x0048
#define DMA_CURRENT_DESTINATION_POINTER_OFFSET	0x004C

/* Bit defines */
#define DMA_FRAME_LEN_MASK			0xFFFFF
#define DMA_FRAME_CNT_MASK			0xFFF

#define DMA_START_DPE				(0X1 << 31)
#define DMA_START_DSE				(0X1 << 0)

unsigned int dma_base[] = {
	DMA0_BASE, DMA1_BASE, DMA2_BASE, DMA3_BASE, DMA4_BASE,
	DMA5_BASE, DMA6_BASE, DMA7_BASE, DMA8_BASE, DMA9_BASE,
	DMA10_BASE, DMA11_BASE
};

/**
 * set_dma_mode() - set the dma transfer mode
 * @dmanr: dma channel number
 * @mode: dma transfer mode
 */
static __inline__ void set_dma_mode(unsigned int dmanr, unsigned int mode)
{
	act_writel(mode, dma_base[dmanr] + DMA_MODE_OFFSET);
}

/**
 * get_dma_mode() - get the dma transfer mode
 * @dmanr: dma channel number
 */
static __inline__ unsigned int get_dma_mode(unsigned int dmanr)
{
	return act_readl(dma_base[dmanr] + DMA_MODE_OFFSET);
}

/**
 * set_dma_src_addr() - set the dma transfer source address
 * @dmanr: dma channel number
 * @src: source address
 */
static __inline__ void set_dma_src_addr(unsigned int dmanr, unsigned int src)
{
	act_writel(src, dma_base[dmanr] + DMA_SOURCE_OFFSET);
}

/**
 * set_dma_dst_addr() - set the dma transfer destination address
 * @dmanr: dma channel number
 * @dst: destination address
 */
static __inline__ void set_dma_dst_addr(unsigned int dmanr, unsigned int dst)
{
	act_writel(dst, dma_base[dmanr] + DMA_DESTINATION_OFFSET);
}

/**
 * set_dma_frame_len() - set the dma transfer frame length
 * @dmanr: dma channel number
 * @len: dma transfer frame length
 */
static __inline__ void set_dma_frame_len(unsigned int dmanr, unsigned int len)
{
	len &= DMA_FRAME_LEN_MASK;
	act_writel(len, dma_base[dmanr] + DMA_FRAME_LEN_OFFSET);
}

/**
 * set_dma_frame_count() - set the dma transfer frame number
 * @dmanr: dma channel number
 * @count: dma transfer frame number
 */
static __inline__ void set_dma_frame_count(unsigned int dmanr, unsigned int cnt)
{
	cnt &= DMA_FRAME_CNT_MASK;
	act_writel(cnt, dma_base[dmanr] + DMA_FRAME_CNT_OFFSET);
}

/**
 * start_dma() - start the dma transfer
 * @dmanr: dma channel number
 */
static __inline__ void start_dma(unsigned int dmanr)
{
	act_writel(DMA_START_DSE, dma_base[dmanr] + DMA_START_OFFSET);
}

/**
 * dma_started() - test if the dma channel started
 * @dmanr: dma channel number
 */
static __inline__ int dma_started(unsigned int dmanr)
{
	return (act_readl(dma_base[dmanr] + DMA_START_OFFSET) & DMA_START_DSE) ? 1 : 0;
}

/**
 * pause_dma() - pause the dma transfer
 * @dmanr: dma channel number
 */
static __inline__ void pause_dma(unsigned int dmanr)
{
	act_writel(DMA_START_DPE, dma_base[dmanr] + DMA_START_OFFSET);
}

/**
 * stop_dma() - stop the dma transfer
 * @dmanr: dma channel number
 */
static __inline__ void stop_dma(unsigned int dmanr)
{
	act_writel(act_readl(dma_base[dmanr] + DMA_START_OFFSET) & (~DMA_START_DSE),
		dma_base[dmanr] + DMA_START_OFFSET);
}

#endif /* end of _ATV5203_DMA_H_ */
