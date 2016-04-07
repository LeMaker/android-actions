#ifndef _ATV5203_DMA_H_
#define _ATV5203_DMA_H_

#include <asm/arch/actions_reg_owl.h>

/*
 * Simple DMA transfer operations defines for MMC/SD card
 */
#if 0
#define DMA_GLOBAL_BASE				0xB0260000
#define DMA_IRQ_PD0_BASE			0xB0260000
#define DMA_IRQ_PD1_BASE			0xB0260004
#define DMA_IRQ_PD2_BASE			0xB0260008
#define DMA_IRQ_PD3_BASE			0xB026000c
#define DMA_IRQ_EN0_BASE			0xB0260010
#define DMA_IRQ_EN1_BASE			0xB0260014
#define DMA_IRQ_EN2_BASE			0xB0260018
#define DMA_IRQ_EN3_BASE			0xB026001c

#define DMA0_BASE				0xB0260100
#define DMA1_BASE				0xB0260200
#define DMA2_BASE				0xB0260300
#define DMA3_BASE				0xB0260400
#define DMA4_BASE				0xB0260500
#define DMA5_BASE				0xB0260600
#define DMA6_BASE				0xB0260700
#define DMA7_BASE				0xB0260800
#define DMA8_BASE				0xB0260900
#define DMA9_BASE				0xB0260A00
#define DMA10_BASE				0xB0260B00
#define DMA11_BASE				0xB0260C00
#define DMA12_BASE				0xB0260D00
#define DMA13_BASE				0xB0260E00
#define DMA14_BASE				0xB0260F00
#define DMA15_BASE				0xB0261000
#endif

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

static unsigned int dma_base[] = {
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
	writel(mode, dma_base[dmanr] + DMA_MODE_OFFSET);
}

/**
 * get_dma_mode() - get the dma transfer mode
 * @dmanr: dma channel number
 */
static __inline__ unsigned int get_dma_mode(unsigned int dmanr)
{
	return readl(dma_base[dmanr] + DMA_MODE_OFFSET);
}

/**
 * set_dma_src_addr() - set the dma transfer source address
 * @dmanr: dma channel number
 * @src: source address
 */
static __inline__ void set_dma_src_addr(unsigned int dmanr, unsigned int src)
{
	writel(src, dma_base[dmanr] + DMA_SOURCE_OFFSET);
}

/**
 * set_dma_dst_addr() - set the dma transfer destination address
 * @dmanr: dma channel number
 * @dst: destination address
 */
static __inline__ void set_dma_dst_addr(unsigned int dmanr, unsigned int dst)
{
	writel(dst, dma_base[dmanr] + DMA_DESTINATION_OFFSET);
}

/**
 * set_dma_frame_len() - set the dma transfer frame length
 * @dmanr: dma channel number
 * @len: dma transfer frame length
 */
static __inline__ void set_dma_frame_len(unsigned int dmanr, unsigned int len)
{
	len &= DMA_FRAME_LEN_MASK;
	writel(len, dma_base[dmanr] + DMA_FRAME_LEN_OFFSET);
}

/**
 * set_dma_frame_count() - set the dma transfer frame number
 * @dmanr: dma channel number
 * @count: dma transfer frame number
 */
static __inline__ void set_dma_frame_count(unsigned int dmanr, unsigned int cnt)
{
	cnt &= DMA_FRAME_CNT_MASK;
	writel(cnt, dma_base[dmanr] + DMA_FRAME_CNT_OFFSET);
}

/**
 * start_dma() - start the dma transfer
 * @dmanr: dma channel number
 */
static __inline__ void start_dma(unsigned int dmanr)
{
	writel(DMA_START_DSE, dma_base[dmanr] + DMA_START_OFFSET);
}

/**
 * dma_started() - test if the dma channel started
 * @dmanr: dma channel number
 */
static __inline__ int dma_started(unsigned int dmanr)
{
	return (readl(dma_base[dmanr] + DMA_START_OFFSET) & DMA_START_DSE) ? 1 : 0;
}

/**
 * pause_dma() - pause the dma transfer
 * @dmanr: dma channel number
 */
static __inline__ void pause_dma(unsigned int dmanr)
{
	writel(DMA_START_DPE, dma_base[dmanr] + DMA_START_OFFSET);
}

/**
 * stop_dma() - stop the dma transfer
 * @dmanr: dma channel number
 */
static __inline__ void stop_dma(unsigned int dmanr)
{
	writel(readl(dma_base[dmanr] + DMA_START_OFFSET) & (~DMA_START_DSE),
		dma_base[dmanr] + DMA_START_OFFSET);
}

#if 0
/**
 * get_dma_tcirq_pend() - short function description of foobar
 * @dmanr: dma channel number
 **/
static __inline__ int get_dma_tcirq_pend(unsigned int dmanr)
{
	return (readl(DMA_IRQPD) & (1 << (dmanr*2))) ? 1 : 0;
}

/**
 * clear_dma_tcirq_pend() - short function description of foobar
 * @dmanr: dma channel number
 **/
static __inline__ int clear_dma_tcirq_pend(unsigned int dmanr)
{
	writel(1 << (dmanr*2), DMA_IRQPD);

	return 0;
}
#endif

#endif /* end of _ATV5203_DMA_H_ */
