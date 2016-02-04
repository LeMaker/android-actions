/*
 * asoc_spi.c -- Actions soc SPI controller driver
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/spi/spi.h>
#include <mach/hardware.h>
#include <mach/hdmac-owl.h>
#include <mach/spi-owl.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <mach/module-owl.h>

#define DRIVER_NAME			"asoc_spi"

/**********************************************************/

#define SPI_CTL			0x0
#define SPI_CLKDIV		0x4
#define SPI_STAT		0x8
#define SPI_RXDAT		0xc
#define SPI_TXDAT		0x10
#define SPI_TCNT		0x14
#define SPI_SEED		0x18
#define SPI_TXCR		0x1c
#define SPI_RXCR		0x20

#define MAX_SPI_POLL_LOOPS		5000
#define MAX_SPI_DMA_LEN			8192
#define BYTES_4_DMA_XFER		64

struct asoc_spi {
/*	struct spi_master	*master;*/
/*	unsigned int irq; */
/*	struct owl_gpio *owl_spi_cs_table; */
	struct device *dev;

	unsigned int irq;
	spinlock_t		lock;
	struct clk *clk;

	u32 base; /*phy addr*/

	struct workqueue_struct *workqueue;
	struct work_struct	work;

	/* Lock access to transfer list.	*/
	struct list_head	msg_queue;

	u8 enable_dma;

#ifdef CONFIG_DMA_ENGINE
	struct dma_chan			*dma_rx_channel;
	struct dma_chan			*dma_tx_channel;
	struct sg_table			sgt_rx;
	struct sg_table			sgt_tx;
	bool				dma_running;
#endif

};

static inline void dump_spi_registers(struct asoc_spi *asoc_spi)
{
	dev_dbg(asoc_spi->dev, "asoc_spi: SPI0_CTL(0x%x) = 0x%x\n", asoc_spi->base + SPI_CTL,
	       act_readl(asoc_spi->base + SPI_CTL));
	dev_dbg(asoc_spi->dev, "asoc_spi: SPI0_STAT(0x%x) = 0x%x\n", asoc_spi->base + SPI_STAT,
	       act_readl(asoc_spi->base + SPI_STAT));
	dev_dbg(asoc_spi->dev, "asoc_spi: SPI0_CLKDIV(0x%x) = 0x%x\n", asoc_spi->base + SPI_CLKDIV,
	       act_readl(asoc_spi->base + SPI_CLKDIV));
}

static inline u32 spi_reg(struct asoc_spi *asoc_spi, u32 reg)
{
	return asoc_spi->base + reg;
}


static inline void spi_clear_stat(struct asoc_spi *asoc_spi)
{

	act_writel(SPIx_STAT_TFER	/* clear the rx FIFO */
		| SPIx_STAT_RFER	/* clear the tx FIFO */
		| SPIx_STAT_BEB	/* clear the Bus error bit */
		| SPIx_STAT_TCOM	/* clear the transfer complete bit */
		| SPIx_STAT_TIP	/* clear the tx IRQ pending bit */
		| SPIx_STAT_PIP,	/* clear the rx IRQ pending bit */
		spi_reg(asoc_spi, SPI_STAT)
	);
}

static int asoc_spi_mode_set(struct spi_device *spi)
{
	u32 ctl_reg;
	struct asoc_spi *asoc_spi;
	u32 tmp;
	u8 mode = spi->mode;

	asoc_spi = spi_master_get_devdata(spi->master);
	ctl_reg = spi_reg(asoc_spi, SPI_CTL);

	tmp = act_readl(ctl_reg);

	tmp |= SPIx_CTL_EN;

	if (mode & SPI_CPHA)
		tmp |= (0x1 << 6);
	else
		tmp &= (~(0x1 << 6));

	if (mode & SPI_CPOL)
		tmp |= (0x1 << 7);
	else
		tmp &= (~(0x1 << 7));

	if (mode & SPI_LSB_FIRST)
		tmp |= SPIx_CTL_LMFS;
	else
		tmp &= (~SPIx_CTL_LMFS);

	if (mode & SPI_LOOP)
		tmp |= SPIx_CTL_LBT;
	else
		tmp &= (~SPIx_CTL_LBT);

	act_writel(tmp, ctl_reg);

	return 0;
}

static int asoc_spi_set_bus_param(struct spi_device *spi,
	int bits_per_word, u8 convert_endian)
{
	u32 ctl_reg;
	struct asoc_spi *asoc_spi;
	u32 tmp;

	asoc_spi = spi_master_get_devdata(spi->master);
	ctl_reg = spi_reg(asoc_spi, SPI_CTL);

	tmp = act_readl(ctl_reg);
	tmp &= (~SPIx_CTL_DAWS(3));

	if (bits_per_word == 8) {
		tmp |= SPIx_CTL_DAWS(0);
	} else if (bits_per_word == 16) {
		tmp |= SPIx_CTL_DAWS(1);
	} else if (bits_per_word == 32) {
		tmp |= SPIx_CTL_DAWS(2);
	} else {
		pr_debug("Bad bits per word value %d (8, 16 or 32 are "
			 "allowed).\n", bits_per_word);
		return -EINVAL;
	}

	if (convert_endian)
		tmp |= SPIx_CTL_CEB;
	else
		tmp &= (~SPIx_CTL_CEB);

	spi->bits_per_word = bits_per_word;
	act_writel(tmp, ctl_reg);

	return 0;
}

static int asoc_spi_baudrate_set(struct spi_device *spi, unsigned int speed)
{
	u32 spi_source_clk_hz;
	u32 clk_div;
	u32 clkdiv_reg;
	struct asoc_spi *asoc_spi;

	asoc_spi = spi_master_get_devdata(spi->master);

	clkdiv_reg = spi_reg(asoc_spi, SPI_CLKDIV);

	spi_source_clk_hz = clk_get_rate(asoc_spi->clk);

	/* setup SPI clock register */
	/* SPICLK = HCLK/(CLKDIV*2) */
	clk_div = (spi_source_clk_hz + (2 * speed) - 1) / (speed) / 2;
	if (clk_div == 0)
		clk_div = 1;

	dev_dbg(&spi->dev, "asoc_spi: required speed = %d\n", speed);
	dev_dbg(&spi->dev, "asoc_spi:spi clock = %d KHz(hclk = %d,clk_div = %d)\n",
	       spi_source_clk_hz / (clk_div * 2) / 1000, spi_source_clk_hz, clk_div);

	act_writel(SPIx_CLKDIV_CLKDIV(clk_div), clkdiv_reg);

	return 0;
}


static void asoc_spi_activate_cs(struct spi_device *spi)
{
	struct asoc_spi *asoc_spi;
	u32 ctl_reg;

	asoc_spi = spi_master_get_devdata(spi->master);
	ctl_reg = spi_reg(asoc_spi, SPI_CTL);

	act_clearl(SPIx_CTL_SSCO, ctl_reg);

}

static void asoc_spi_deactivate_cs(struct spi_device *spi)
{
	struct asoc_spi *asoc_spi;
	u32 ctl_reg;

	asoc_spi = spi_master_get_devdata(spi->master);
	ctl_reg = spi_reg(asoc_spi, SPI_CTL);

	act_setl(SPIx_CTL_SSCO, ctl_reg);
}

int asoc_spi_wait_till_ready(struct asoc_spi *asoc_spi)
{
	int i;
	u32 stat_reg = spi_reg(asoc_spi, SPI_STAT);

	for (i = 0; i < MAX_SPI_POLL_LOOPS; i++) {
		if (act_readl(stat_reg) & SPIx_STAT_TCOM) {
			act_writel(act_readl(stat_reg) | SPIx_STAT_TCOM, stat_reg);
			//dump_spi_registers(asoc_spi);
			dev_dbg(asoc_spi->dev, "wait num = %d\n", i);
			return 1;
		}
	}

	dump_spi_registers(asoc_spi);

	return -1;
}


static void spi_callback(void *completion)
{
	complete(completion);
}


static inline int
asoc_spi_write_read_8bit(struct spi_device *spi,
			   struct spi_transfer *xfer)
{
	u32 tx_reg, rx_reg, ctl_reg;
	struct asoc_spi *asoc_spi;
	u32 tmp;
	const u8 *tx_buf = xfer->tx_buf;
	u8 *rx_buf = xfer->rx_buf;
	unsigned int count = xfer->len;

	dev_dbg(&spi->dev, "%s(len:0x%x)\n", __func__, xfer->len);
	if (rx_buf != NULL) {
		dev_dbg(&spi->dev, "  rx_buf:0x%x)\n", ((const u8 *)(xfer->rx_buf))[0]);
	}
	if (tx_buf != NULL) {
		dev_dbg(&spi->dev, "  tx_buf:0x%x)\n", ((const u8 *)(xfer->tx_buf))[0]);
	}


	asoc_spi = spi_master_get_devdata(spi->master);
	ctl_reg = spi_reg(asoc_spi, SPI_CTL);
	tx_reg = spi_reg(asoc_spi, SPI_TXDAT);
	rx_reg = spi_reg(asoc_spi, SPI_RXDAT);

	tmp = act_readl(ctl_reg);
	tmp &= (~(SPIx_CTL_RWC(3) |
		SPIx_CTL_RDIC(3) |
		SPIx_CTL_TDIC(3) |
		SPIx_CTL_SDT(7) |
		SPIx_CTL_DTS |
		SPIx_CTL_TIEN |
		SPIx_CTL_RIEN |
		SPIx_CTL_TDEN |
		SPIx_CTL_RDEN));
	tmp |= SPIx_CTL_RWC(0);
	act_writel(tmp, ctl_reg);

	do {
		if (tx_buf)
			act_writel(*tx_buf++, tx_reg);
		else
			act_writel(0, tx_reg);

		if (asoc_spi_wait_till_ready(asoc_spi) < 0) {
			dev_err(&spi->dev, "TXS timed out\n");
			return count;
		}

		if (rx_buf)
			*rx_buf++ = act_readl(rx_reg);

		count--;

	} while (count);

	if (rx_buf != NULL) {
		dev_dbg(&spi->dev, "  rx_buf:0x%x)\n", ((const u8 *)(xfer->rx_buf))[0]);
	}
	if (tx_buf != NULL) {
		dev_dbg(&spi->dev, "  tx_buf:0x%x)\n", ((const u8 *)(xfer->tx_buf))[0]);
	}

	return count;
}

static inline int
asoc_spi_write_read_16bit(struct spi_device *spi,
			   struct spi_transfer *xfer)
{
	u32 tx_reg, rx_reg, ctl_reg;
	struct asoc_spi *asoc_spi;
	u32 tmp;
	const u16 *tx_buf = xfer->tx_buf;
	u16 *rx_buf = xfer->rx_buf;
	unsigned int count = xfer->len;

	if (tx_buf) {
		dev_dbg(&spi->dev, "    tx_buf 0x%x  0x%x\n", tx_buf[0], tx_buf[1]);
	}

	asoc_spi = spi_master_get_devdata(spi->master);
	ctl_reg = spi_reg(asoc_spi, SPI_CTL);
	tx_reg = spi_reg(asoc_spi, SPI_TXDAT);
	rx_reg = spi_reg(asoc_spi, SPI_RXDAT);

	tmp = act_readl(ctl_reg);
	tmp &= (~(SPIx_CTL_RWC(3) |
		SPIx_CTL_RDIC(3) |
		SPIx_CTL_TDIC(3) |
		SPIx_CTL_SDT(7) |
		SPIx_CTL_DTS |
		SPIx_CTL_TIEN |
		SPIx_CTL_RIEN |
		SPIx_CTL_TDEN |
		SPIx_CTL_RDEN));
	tmp |= SPIx_CTL_RWC(0);
	act_writel(tmp, ctl_reg);

	do {
		if (tx_buf)
			act_writel(*tx_buf++, tx_reg);
		else
			act_writel(0, tx_reg);

		if (asoc_spi_wait_till_ready(asoc_spi) < 0) {
			dev_err(&spi->dev, "TXS timed out\n");
			return count;
		}

		if (rx_buf) {
			*rx_buf++ = act_readl(rx_reg);
			dev_dbg(&spi->dev, "rx_buf 0x%x\n", rx_buf[-1]);
		}

		count -= 2;
	} while (count);

	return count;
}


static inline int
asoc_spi_write_read_32bit(struct spi_device *spi,
			   struct spi_transfer *xfer)
{
	u32 tx_reg, rx_reg, ctl_reg;
	struct asoc_spi *asoc_spi;
	u32 tmp;
	const u32 *tx_buf = xfer->tx_buf;
	u32 *rx_buf = xfer->rx_buf;
	unsigned int count = xfer->len;

	asoc_spi = spi_master_get_devdata(spi->master);
	ctl_reg = spi_reg(asoc_spi, SPI_CTL);
	tx_reg = spi_reg(asoc_spi, SPI_TXDAT);
	rx_reg = spi_reg(asoc_spi, SPI_RXDAT);

	tmp = act_readl(ctl_reg);
	tmp &= (~(SPIx_CTL_RWC(3) |
		SPIx_CTL_RDIC(3) |
		SPIx_CTL_TDIC(3) |
		SPIx_CTL_SDT(7) |
		SPIx_CTL_DTS |
		SPIx_CTL_TIEN |
		SPIx_CTL_RIEN |
		SPIx_CTL_TDEN |
		SPIx_CTL_RDEN));
	tmp |= SPIx_CTL_RWC(0);
	act_writel(tmp, ctl_reg);

	do {
		if (tx_buf)
			act_writel(*tx_buf++, tx_reg);
		else
			act_writel(0, tx_reg);

		if (asoc_spi_wait_till_ready(asoc_spi) < 0) {
			dev_err(&spi->dev, "TXS timed out\n");
			return count;
		}

		if (rx_buf)
			*rx_buf++ = act_readl(rx_reg);

		count -= 4;

	} while (count);

	return count;
}

static int asoc_spi_get_channel_no(unsigned int base)
{
	switch (base) {
		case SPI0_BASE:
			return 0;
		case SPI1_BASE:
			return 1;
		case SPI2_BASE:
			return 2;
		case SPI3_BASE:
			return 3;
	}
	return -1;
}

static unsigned int asoc_spi_get_write_dma_trig(unsigned int base)
{
	static unsigned int trigs[] = {SPI0_T, SPI1_T, SPI2_T, SPI3_T};
	
	int spi_no = asoc_spi_get_channel_no(base);
	if(spi_no < 0) {
		pr_err("error: 0x%x.spi do not support\n", base);
		return -1;
	}
	
	return trigs[spi_no];
}

static inline unsigned int asoc_spi_get_read_dma_trig(unsigned int base)
{
	static unsigned int trigs[] = {SPI0_R, SPI1_R, SPI2_R, SPI3_R};
	int spi_no = asoc_spi_get_channel_no(base);
	if(spi_no < 0) {
		pr_err("error: 0x%x.spi do not support\n", base);
		return -1;
	}
	
	return trigs[spi_no];
}

static struct page *asoc_spi_virt_to_page(const void *addr)
{
	if (is_vmalloc_addr(addr))
		return vmalloc_to_page(addr);
	else
		return virt_to_page(addr);
}

static void asoc_spi_setup_dma_scatter(struct asoc_spi *asoc_spi,
			      void *buffer,
			      unsigned int length,
			      struct sg_table *sgtab)
{
	struct scatterlist *sg;
	int bytesleft = length;
	void *bufp = buffer;
	int mapbytes;
	int i;

	if (buffer) {
		for_each_sg(sgtab->sgl, sg, sgtab->nents, i) {
			if(bytesleft == 0) {
				sg_mark_end(sg);
				sgtab->nents = i;
				break;
			}
			/*
			 * If there are less bytes left than what fits
			 * in the current page (plus page alignment offset)
			 * we just feed in this, else we stuff in as much
			 * as we can.
			 */
			if (bytesleft < (PAGE_SIZE - offset_in_page(bufp)))
				mapbytes = bytesleft;
			else
				mapbytes = PAGE_SIZE - offset_in_page(bufp);
			sg_set_page(sg, asoc_spi_virt_to_page(bufp),
				    mapbytes, offset_in_page(bufp));
			bufp += mapbytes;
			bytesleft -= mapbytes;
			dev_dbg(asoc_spi->dev,
				"set RX/TX target page @ %p, %d bytes, %d left\n",
				bufp, mapbytes, bytesleft);
		}
	}
	
	BUG_ON(bytesleft);
}

static int asoc_spi_write_by_dma(struct asoc_spi *asoc_spi,
	struct spi_transfer *xfer)
{
	struct dma_slave_config tx_conf = {
		.dst_addr = spi_reg(asoc_spi, SPI_TXDAT),
		.direction = DMA_MEM_TO_DEV,
	};
	struct owl_dma_slave tx_atslave = {
		.mode = PRIORITY_ZERO | SRC_INCR | DST_CONSTANT
					| SRC_DCU | DST_DEV 
					| asoc_spi_get_write_dma_trig(asoc_spi->base),
		.dma_dev = asoc_spi->dma_tx_channel->device->dev,
		.trans_type = SLAVE,
	};
	u32 ctl_reg = spi_reg(asoc_spi, SPI_CTL);
	u32 stat_reg = spi_reg(asoc_spi, SPI_STAT);
	u32 txcr_reg = spi_reg(asoc_spi, SPI_TXCR);

	struct dma_chan *txchan = asoc_spi->dma_tx_channel;
	unsigned int pages;
	int len, left;
	void *tx_buf;
	int tx_sglen;
	struct dma_async_tx_descriptor *txdesc;
	u32 val;
	int retval;

	struct completion tx_cmp;
	dma_cookie_t		cookie;
	enum dma_status		status;

	/* Create sglists for the transfers */
	left = xfer->len;
	tx_buf = (void*)xfer->tx_buf;
	len = left > MAX_SPI_DMA_LEN ? MAX_SPI_DMA_LEN : left;
	
	pages = DIV_ROUND_UP(len, PAGE_SIZE);
	retval = sg_alloc_table(&asoc_spi->sgt_tx, pages, GFP_ATOMIC);
	if (retval)
		goto err_slave;

	while(left > 0) {
		len = left > MAX_SPI_DMA_LEN ? MAX_SPI_DMA_LEN : left;
		left -= len;
		
		val = act_readl(ctl_reg);
		val &= (~(SPIx_CTL_RWC(3) |
			SPIx_CTL_RDIC(3) |
			SPIx_CTL_TDIC(3) |
			SPIx_CTL_SDT(7) |
			SPIx_CTL_DTS |
			SPIx_CTL_TIEN |
			SPIx_CTL_RIEN |
			SPIx_CTL_TDEN |
			SPIx_CTL_RDEN));
		val |= (SPIx_CTL_RWC(1) |
			SPIx_CTL_RDIC(2) |
			SPIx_CTL_TDIC(2) |
			SPIx_CTL_TDEN);
		act_writel(val, ctl_reg);
	
		act_writel(len/4, txcr_reg);

		txchan->private = (void *)&tx_atslave;
		retval = dmaengine_slave_config(txchan, &tx_conf);
		if (retval) {
			dev_err(asoc_spi->dev, "call the write slave config error\n");
			goto err_slave;
		}

		/* Fill in the scatterlists for the TX buffers */
		asoc_spi_setup_dma_scatter(asoc_spi, tx_buf, len, &asoc_spi->sgt_tx);
		tx_sglen = dma_map_sg(txchan->device->dev, asoc_spi->sgt_tx.sgl,
				   asoc_spi->sgt_tx.nents, DMA_TO_DEVICE);
		if (!tx_sglen)
			goto err_sgmap;

		tx_buf += len;

		/* Send scatterlists */
		txdesc = dmaengine_prep_slave_sg(txchan,
					      asoc_spi->sgt_tx.sgl,
					      tx_sglen,
					      DMA_MEM_TO_DEV,
					      0);
		if (!txdesc)
			goto err_desc;

		init_completion(&tx_cmp);
	
		txdesc->callback = spi_callback;
		txdesc->callback_param = &tx_cmp;

		cookie = dmaengine_submit(txdesc);
		if (dma_submit_error(cookie)) {
			dev_err(asoc_spi->dev, "submit write error!\n");
			goto err_desc;
		}

		dma_async_issue_pending(txchan);

		dev_dbg(asoc_spi->dev, "write start dma\n");
		if (!wait_for_completion_timeout(&tx_cmp, 5 * HZ)) {
			dev_err(asoc_spi->dev, "wait_for_completion timeout while send by dma\n");
			owl_dma_dump_all(txchan);
			goto err_desc;
		}

		status = dma_async_is_tx_complete(txchan, cookie, NULL, NULL);
		if (status != DMA_SUCCESS) {
			dev_err(asoc_spi->dev, "transfer not succeed\n");
			goto err_desc;
		}

		if (asoc_spi_wait_till_ready(asoc_spi) < 0) {
			dev_err(asoc_spi->dev, "TXS timed out\n");
			goto err_desc;
		}

		if (act_readl(stat_reg) &
		    (SPIx_STAT_RFER | SPIx_STAT_TFER | SPIx_STAT_BEB)) {
			dev_err(asoc_spi->dev, "spi state error while send by dma\n");
			dump_spi_registers(asoc_spi);
			goto err_desc;
		}

		dma_unmap_sg(txchan->device->dev, asoc_spi->sgt_tx.sgl,
			     asoc_spi->sgt_tx.nents, DMA_TO_DEVICE);
	}
	sg_free_table(&asoc_spi->sgt_tx);
	return 0;

err_desc:
	dmaengine_terminate_all(txchan);
err_sgmap:
	sg_free_table(&asoc_spi->sgt_tx);
err_slave:
	return -EINVAL;
}

static int asoc_spi_read_by_dma(struct asoc_spi *asoc_spi,
			   struct spi_transfer *xfer)
{
	struct dma_slave_config rx_conf = {
		.src_addr = spi_reg(asoc_spi, SPI_RXDAT),
		.direction = DMA_DEV_TO_MEM,
	};
	struct owl_dma_slave rx_atslave = {
		.mode = PRIORITY_ZERO | DST_INCR | SRC_CONSTANT 
					| DST_DCU | SRC_DEV 
					| asoc_spi_get_read_dma_trig(asoc_spi->base),
		.dma_dev = asoc_spi->dma_rx_channel->device->dev,
		.trans_type = SLAVE,
	};
	u32 ctl_reg = spi_reg(asoc_spi, SPI_CTL);
	u32 stat_reg = spi_reg(asoc_spi, SPI_STAT);
	u32 rxcr_reg = spi_reg(asoc_spi, SPI_RXCR);
	u32 tcnt_reg = spi_reg(asoc_spi, SPI_TCNT);
	struct dma_chan *rxchan = asoc_spi->dma_rx_channel;
	unsigned int pages;
	int len, left;
	void *rx_buf;
	int rx_sglen;
	struct dma_async_tx_descriptor *rxdesc;
	
	u32 val;
	int retval;

	struct completion rx_cmp;
	dma_cookie_t		cookie;
	enum dma_status		status;
	
	/* Create sglists for the transfers */
	left = xfer->len;
	rx_buf = xfer->rx_buf;
	len = left > MAX_SPI_DMA_LEN ? MAX_SPI_DMA_LEN : left;
	
	pages = DIV_ROUND_UP(len, PAGE_SIZE);
	retval = sg_alloc_table(&asoc_spi->sgt_rx, pages, GFP_ATOMIC);
	if (retval)
		goto err_slave;

	while(left > 0) {
		len = left > MAX_SPI_DMA_LEN ? MAX_SPI_DMA_LEN : left;
		left -= len;
		
		val = act_readl(ctl_reg);
		val &= (~(SPIx_CTL_RWC(3) |
			SPIx_CTL_RDIC(3) |
			SPIx_CTL_TDIC(3) |
			SPIx_CTL_SDT(7) |
			SPIx_CTL_DTS |
			SPIx_CTL_TIEN |
			SPIx_CTL_RIEN |
			SPIx_CTL_TDEN |
			SPIx_CTL_RDEN));
		val |= (SPIx_CTL_RWC(2) |
			SPIx_CTL_RDIC(2) |
			SPIx_CTL_TDIC(2) |
			SPIx_CTL_SDT(1) |
			SPIx_CTL_DTS |
			SPIx_CTL_RDEN);
		act_writel(val, ctl_reg);

		act_writel(len/4, tcnt_reg);
		act_writel(len/4, rxcr_reg);
		
		rxchan->private = (void *)&rx_atslave;
		retval = dmaengine_slave_config(rxchan, &rx_conf);
		if (retval) {
			dev_err(asoc_spi->dev, "call the read slave config error\n");
			goto err_slave;
		}

		/* Fill in the scatterlists for the RX buffers */
		asoc_spi_setup_dma_scatter(asoc_spi, rx_buf, len, &asoc_spi->sgt_rx);
		rx_sglen = dma_map_sg(rxchan->device->dev, asoc_spi->sgt_rx.sgl,
				   asoc_spi->sgt_rx.nents, DMA_FROM_DEVICE);
		if (!rx_sglen)
			goto err_sgmap;

		rx_buf += len;
		
		/* Send scatterlists */
		rxdesc = dmaengine_prep_slave_sg(rxchan,
					      asoc_spi->sgt_rx.sgl,
					      rx_sglen,
					      DMA_DEV_TO_MEM,
					      0);
		if (!rxdesc)
			goto err_desc;

		init_completion(&rx_cmp);
	
		rxdesc->callback = spi_callback;
		rxdesc->callback_param = &rx_cmp;
	
		cookie = dmaengine_submit(rxdesc);
		if (dma_submit_error(cookie)) {
			dev_err(asoc_spi->dev, "submit read error!\n");
			goto err_desc;
		}

		dma_async_issue_pending(rxchan);

		dev_dbg(asoc_spi->dev, "read start dma\n");
		if (!wait_for_completion_timeout(&rx_cmp, 5 * HZ)) {
			dev_err(asoc_spi->dev, "read wait_for_completion timeout while receive by dma\n");
			owl_dma_dump_all(rxchan);
			goto err_desc;
		}
	
		status = dma_async_is_tx_complete(rxchan, cookie, NULL, NULL);
		if (status != DMA_SUCCESS) {
			dev_err(asoc_spi->dev, "transfer not succeed\n");
			goto err_desc;
		}
	
		if (asoc_spi_wait_till_ready(asoc_spi) < 0) {
			dev_err(asoc_spi->dev, "RXS timed out\n");
			goto err_desc;
		}
	
		if (act_readl(stat_reg) &
		    (SPIx_STAT_RFER | SPIx_STAT_TFER | SPIx_STAT_BEB)) {
			dev_err(asoc_spi->dev, "spi state error while send by dma\n");
			dump_spi_registers(asoc_spi);
			goto err_desc;
		}
	
		dma_unmap_sg(rxchan->device->dev, asoc_spi->sgt_rx.sgl,
			     asoc_spi->sgt_rx.nents, DMA_FROM_DEVICE);
	}
	sg_free_table(&asoc_spi->sgt_rx);
	return 0;
	
err_desc:
	dmaengine_terminate_all(rxchan);
err_sgmap:
	sg_free_table(&asoc_spi->sgt_rx);
err_slave:
	return -EINVAL;
}


static int asoc_spi_write_read_by_dma(struct asoc_spi *asoc_spi,
			   struct spi_transfer *xfer)
{
	struct dma_slave_config tx_conf = {
		.dst_addr = spi_reg(asoc_spi, SPI_TXDAT),
		.direction = DMA_MEM_TO_DEV,
	};
	struct owl_dma_slave tx_atslave = {
		.mode = PRIORITY_ZERO | SRC_INCR | DST_CONSTANT
					| SRC_DCU | DST_DEV 
					| asoc_spi_get_write_dma_trig(asoc_spi->base),
		.dma_dev = asoc_spi->dma_tx_channel->device->dev,
		.trans_type = SLAVE,
	};
	struct dma_slave_config rx_conf = {
		.src_addr = spi_reg(asoc_spi, SPI_RXDAT),
		.direction = DMA_DEV_TO_MEM,
	};
	struct owl_dma_slave rx_atslave = {
		.mode = PRIORITY_ZERO | DST_INCR | SRC_CONSTANT 
					| DST_DCU | SRC_DEV 
					| asoc_spi_get_read_dma_trig(asoc_spi->base),
		.dma_dev = asoc_spi->dma_rx_channel->device->dev,
		.trans_type = SLAVE,
	};
	u32 ctl_reg = spi_reg(asoc_spi, SPI_CTL);
	u32 stat_reg = spi_reg(asoc_spi, SPI_STAT);
	u32 txcr_reg = spi_reg(asoc_spi, SPI_TXCR);
	u32 rxcr_reg = spi_reg(asoc_spi, SPI_RXCR);
	struct dma_chan *txchan = asoc_spi->dma_tx_channel;
	struct dma_chan *rxchan = asoc_spi->dma_rx_channel;
	unsigned int pages;
	int len, left;
	void *tx_buf, *rx_buf;
	int rx_sglen, tx_sglen;
	struct dma_async_tx_descriptor *rxdesc;
	struct dma_async_tx_descriptor *txdesc;
	
	u32 val;
	int retval;

	struct completion rx_cmp, tx_cmp;
	dma_cookie_t		cookie;
	enum dma_status		status;

	/* Create sglists for the transfers */
	left = xfer->len;
	tx_buf = (void*)xfer->tx_buf;
	rx_buf = xfer->rx_buf;
	len = left > MAX_SPI_DMA_LEN ? MAX_SPI_DMA_LEN : left;

	pages = DIV_ROUND_UP(len, PAGE_SIZE);
	retval = sg_alloc_table(&asoc_spi->sgt_tx, pages, GFP_ATOMIC);
	if (retval)
		goto err_slave;
	retval = sg_alloc_table(&asoc_spi->sgt_rx, pages, GFP_ATOMIC);
	if (retval)
		goto err_slave;

	while(left > 0) {
		len = left > MAX_SPI_DMA_LEN ? MAX_SPI_DMA_LEN : left;
		left -= len;

		val = act_readl(ctl_reg);
		val &= (~(SPIx_CTL_RWC(3) |
			SPIx_CTL_RDIC(3) |
			SPIx_CTL_TDIC(3) |
			SPIx_CTL_SDT(7) |
			SPIx_CTL_DTS |
			SPIx_CTL_TIEN |
			SPIx_CTL_RIEN |
			SPIx_CTL_TDEN |
			SPIx_CTL_RDEN));
		val |= (SPIx_CTL_RWC(0) |
			SPIx_CTL_RDIC(2) |
			SPIx_CTL_TDIC(2) |
			SPIx_CTL_SDT(1) |
			SPIx_CTL_DTS |
			SPIx_CTL_RDEN |
			SPIx_CTL_TDEN);
		act_writel(val, ctl_reg);
	
		act_writel(len/4, txcr_reg);
		act_writel(len/4, rxcr_reg);

		txchan->private = (void *)&tx_atslave;
		retval = dmaengine_slave_config(txchan, &tx_conf);
		if (retval) {
			dev_err(asoc_spi->dev, "call the write slave config error\n");
			goto err_slave;
		}
		rxchan->private = (void *)&rx_atslave;
		retval = dmaengine_slave_config(rxchan, &rx_conf);
		if (retval) {
			dev_err(asoc_spi->dev, "call the read slave config error\n");
			goto err_slave;
		}

		/* Fill in the scatterlists for the TX buffers */
		asoc_spi_setup_dma_scatter(asoc_spi, tx_buf, len, &asoc_spi->sgt_tx);
		tx_sglen = dma_map_sg(txchan->device->dev, asoc_spi->sgt_tx.sgl,
				   asoc_spi->sgt_tx.nents, DMA_TO_DEVICE);
		if (!tx_sglen)
			goto err_sgmap;
		asoc_spi_setup_dma_scatter(asoc_spi, rx_buf, len, &asoc_spi->sgt_rx);
		rx_sglen = dma_map_sg(rxchan->device->dev, asoc_spi->sgt_rx.sgl,
				   asoc_spi->sgt_rx.nents, DMA_FROM_DEVICE);
		if (!rx_sglen)
			goto err_sgmap;

		tx_buf += len;
		rx_buf += len;

		/* Send scatterlists */
		txdesc = dmaengine_prep_slave_sg(txchan,
					      asoc_spi->sgt_tx.sgl,
					      tx_sglen,
					      DMA_MEM_TO_DEV,
					      0);
		if (!txdesc)
			goto err_desc;	
		rxdesc = dmaengine_prep_slave_sg(rxchan,
					      asoc_spi->sgt_rx.sgl,
					      rx_sglen,
					      DMA_DEV_TO_MEM,
					      0);
		if (!rxdesc)
			goto err_desc;

		init_completion(&tx_cmp);
		txdesc->callback = spi_callback;
		txdesc->callback_param = &tx_cmp;
		cookie = dmaengine_submit(txdesc);
		if (dma_submit_error(cookie)) {
			dev_err(asoc_spi->dev, "submit write error!\n");
			goto err_desc;
		}

		init_completion(&rx_cmp);
		rxdesc->callback = spi_callback;
		rxdesc->callback_param = &rx_cmp;
		cookie = dmaengine_submit(rxdesc);
		if (dma_submit_error(cookie)) {
			dev_err(asoc_spi->dev, "submit read error!\n");
			goto err_desc;
		}

		dma_async_issue_pending(txchan);
		dma_async_issue_pending(rxchan);

		dev_dbg(asoc_spi->dev, "write&read start dma\n");
		if (!wait_for_completion_timeout(&tx_cmp, 5 * HZ)) {
			dev_err(asoc_spi->dev, "write wait_for_completion timeout while send by dma\n");
			owl_dma_dump_all(txchan);
			goto err_desc;
		}
		if (!wait_for_completion_timeout(&rx_cmp, 1 * HZ)) {
			dev_err(asoc_spi->dev, "read wait_for_completion timeout while receive by dma\n");
			owl_dma_dump_all(rxchan);
			goto err_desc;
		}
	
		status = dma_async_is_tx_complete(txchan, cookie, NULL, NULL);
		if (status != DMA_SUCCESS) {
			dev_err(asoc_spi->dev, "transfer not succeed\n");
			goto err_desc;
		}
		status = dma_async_is_tx_complete(rxchan, cookie, NULL, NULL);
		if (status != DMA_SUCCESS) {
			dev_err(asoc_spi->dev, "transfer not succeed\n");
			goto err_desc;
		}
	
		if (asoc_spi_wait_till_ready(asoc_spi) < 0) {
			dev_err(asoc_spi->dev, "TXS&RXS timed out\n");
			goto err_desc;
		}

		if (act_readl(stat_reg) &
		    (SPIx_STAT_RFER | SPIx_STAT_TFER | SPIx_STAT_BEB)) {
			dev_err(asoc_spi->dev, "spi state error while send by dma\n");
			dump_spi_registers(asoc_spi);
			goto err_desc;
		}

		dma_unmap_sg(txchan->device->dev, asoc_spi->sgt_tx.sgl,
			     asoc_spi->sgt_tx.nents, DMA_TO_DEVICE);
		dma_unmap_sg(rxchan->device->dev, asoc_spi->sgt_rx.sgl,
			     asoc_spi->sgt_rx.nents, DMA_FROM_DEVICE);
	}
	sg_free_table(&asoc_spi->sgt_tx);
	sg_free_table(&asoc_spi->sgt_rx);
	return 0;
	
err_desc:
	dmaengine_terminate_all(rxchan);
	dmaengine_terminate_all(txchan);
err_sgmap:
	sg_free_table(&asoc_spi->sgt_rx);
	sg_free_table(&asoc_spi->sgt_tx);
err_slave:
	return -EINVAL;
}

static unsigned int
asoc_spi_write_read(struct spi_device *spi, struct spi_transfer *xfer)
{
	struct asoc_spi *asoc_spi;
	unsigned int len;
	int word_len;

	asoc_spi = spi_master_get_devdata(spi->master);
	word_len = spi->bits_per_word;
	len = xfer->len;

	spi_clear_stat(asoc_spi);
	
	if ((len < BYTES_4_DMA_XFER) ||
		(asoc_spi->enable_dma == 0) ||
		(xfer->bits_per_word != 32)) {
		unsigned int count = 0;

		dev_dbg(asoc_spi->dev, "cpu wr\n");

		if (word_len == 8)
			count = asoc_spi_write_read_8bit(spi, xfer);
		else if (word_len == 16)
			count = asoc_spi_write_read_16bit(spi, xfer);
		else if (word_len == 32)
			count = asoc_spi_write_read_32bit(spi, xfer);

		return len - count;

	} else {
		int retval = 0;

		if (xfer->tx_buf && (!xfer->rx_buf)) {
			dev_dbg(asoc_spi->dev, "dma w%d\n", xfer->len);
			retval = asoc_spi_write_by_dma(asoc_spi, xfer);
		} else if ((!xfer->tx_buf) && xfer->rx_buf) {
			dev_dbg(asoc_spi->dev, "dma r%d\n", xfer->len);
			retval = asoc_spi_read_by_dma(asoc_spi, xfer);
		} else if((xfer->tx_buf) && (xfer->rx_buf)) {
			dev_dbg(asoc_spi->dev, "dma w&r%d\n", xfer->len);
			retval = asoc_spi_write_read_by_dma(asoc_spi, xfer);
  		} else {
			dev_err(&spi->dev, "cannot find valid xfer buffer\n");
			return 0;
		}

		if (retval)
			return 0;
		else
			return len;
	}

}

/*
 * called only when no transfer is active on the bus
 */
static int asoc_spi_device_setup(struct spi_device *spi)
{
	struct asoc_spi *asoc_spi;
	int retval;
	int bits_per_word, speed_hz;

	asoc_spi = spi_master_get_devdata(spi->master);

	speed_hz = spi->max_speed_hz;
	bits_per_word = spi->bits_per_word;

	if (bits_per_word <= 8)
		bits_per_word = 8;
	else if (bits_per_word <= 16)
		bits_per_word = 16;
	else if (bits_per_word <= 32)
		bits_per_word = 32;
	else
		return -EINVAL;

	retval = asoc_spi_baudrate_set(spi, speed_hz);
	if (retval)
		return retval;

	return asoc_spi_set_bus_param(spi, bits_per_word, 0);
}


static int asoc_spi_transfer_setup(struct spi_device *spi,
		struct spi_transfer *t)
{
	struct asoc_spi *asoc_spi;
	int retval;
	int bits_per_word, speed_hz;

	if (!t)
		return -EINVAL;

	asoc_spi = spi_master_get_devdata(spi->master);

	speed_hz = (t->speed_hz)
			? t->speed_hz : spi->max_speed_hz;
	bits_per_word = (t->bits_per_word)
			? t->bits_per_word : spi->bits_per_word;

	if (bits_per_word <= 8 || t->len <= 8)
		bits_per_word = 8;
	else if (bits_per_word <= 16)
		bits_per_word = 16;
	else if (bits_per_word <= 32)
		bits_per_word = 32;
	else
		return -EINVAL;


	retval = asoc_spi_baudrate_set(spi, speed_hz);
	if (retval)
		return retval;

	return asoc_spi_set_bus_param(spi, bits_per_word, 0);
}


static int asoc_spi_transfer_check(struct spi_device *spi,
		struct spi_transfer *t)
{
	int bits_per_word = 0;

	if ((!t) || (t->len == 0))
		return -EINVAL;

	bits_per_word = (t->bits_per_word)
			? t->bits_per_word : spi->bits_per_word;
	if (bits_per_word <= 8 || t->len <= 8)
		bits_per_word = 8;
	else if (bits_per_word <= 16)
		bits_per_word = 16;
	else if (bits_per_word <= 32)
		bits_per_word = 32;
	else
		return -EINVAL;

	/*transfer length should be alignd according to bits_per_word*/
	if (t->len & ((bits_per_word >> 3) - 1)) {
		dev_err(&spi->dev, "bad transfer length!!\n");
		return -EINVAL;
	}

	return 0;
}


static void asoc_spi_work(struct work_struct *work)
{
	struct asoc_spi *asoc_spi =
		container_of(work, struct asoc_spi, work);

	dev_dbg(asoc_spi->dev, "ASOC SPI: enter spi work\n");

	spin_lock_irq(&asoc_spi->lock);
	while (!list_empty(&asoc_spi->msg_queue)) {
		struct spi_message *m;
		struct spi_device *spi;
		struct spi_transfer *t = NULL;
		int par_override = 0;
		int status = 0;
		int cs_active = 0;

		dev_dbg(asoc_spi->dev, "asoc_spi: start one message\n");

		m = container_of(asoc_spi->msg_queue.next, struct spi_message,
				 queue);

		list_del_init(&m->queue);
		spin_unlock_irq(&asoc_spi->lock);

		spi = m->spi;

		/* Load defaults */
		status = asoc_spi_mode_set(spi);
		status = asoc_spi_device_setup(spi);

		if (status < 0)
			goto msg_done;

		list_for_each_entry(t, &m->transfers, transfer_list) {
			dev_dbg(asoc_spi->dev, "asoc_spi: start one transfer\n");

			status = asoc_spi_transfer_check(spi, t);
			if (status < 0)
				break;

			if (par_override || t->bits_per_word || t->speed_hz) {
				par_override = 1;
				status = asoc_spi_transfer_setup(spi, t);
				if (status < 0)
					break;
				if (!t->speed_hz && !t->bits_per_word)
					par_override = 0;
			}

			if (!cs_active) {
				asoc_spi_activate_cs(spi);
				cs_active = 1;
			}

			m->actual_length +=
				asoc_spi_write_read(spi, t);

			if (t->delay_usecs)
				udelay(t->delay_usecs);

			if (t->cs_change) {
				asoc_spi_deactivate_cs(spi);
				cs_active = 0;
			}
			dev_dbg(asoc_spi->dev, "asoc_spi: end one transfer\n");
		}

msg_done:
		if (cs_active)
			asoc_spi_deactivate_cs(spi);

		m->status = status;
		m->complete(m->context);

		dev_dbg(asoc_spi->dev, "asoc_spi: end one message\n");
		spin_lock_irq(&asoc_spi->lock);
	}

	spin_unlock_irq(&asoc_spi->lock);

	dev_dbg(asoc_spi->dev, "ASOC SPI: quit spi work\n");
}

static int asoc_spi_setup(struct spi_device *spi)
{
	struct asoc_spi *asoc_spi;
	unsigned int spi_source_clk_hz;

	dev_dbg(&spi->dev, "ASOC SPI: enter spi setup\n");

	asoc_spi = spi_master_get_devdata(spi->master);

	if (spi->bits_per_word > 32)
		return -EINVAL;

	if (spi->bits_per_word == 0)
		spi->bits_per_word = 8;

	spi_source_clk_hz = clk_get_rate(asoc_spi->clk);

	dev_dbg(&spi->dev, "ahb freq is %d\n", spi_source_clk_hz);

	if ((spi->max_speed_hz == 0)
			|| (spi->max_speed_hz > spi_source_clk_hz))
		spi->max_speed_hz = spi_source_clk_hz;

	dev_dbg(&spi->dev, "ASOC SPI: ok spi setup\n");

	/*
	 * baudrate & width will be set asoc_spi_setup_transfer
	 */
	return 0;
}


static int asoc_spi_transfer(struct spi_device *spi, struct spi_message *m)
{
	struct asoc_spi *asoc_spi;
	unsigned long flags;

	/* reject invalid messages and transfers */
	if (list_empty(&m->transfers) || !m->complete) {
		m->status = -EINVAL;
		return -EINVAL;
	}

	asoc_spi = spi_master_get_devdata(spi->master);

	m->actual_length = 0;
	m->status = -EINPROGRESS;

	spin_lock_irqsave(&asoc_spi->lock, flags);
	list_add_tail(&m->queue, &asoc_spi->msg_queue);
	queue_work(asoc_spi->workqueue, &asoc_spi->work);
	spin_unlock_irqrestore(&asoc_spi->lock, flags);

	return 0;

}

static void asoc_spi_cleanup(struct spi_device *spi)
{
}

static struct owl_spi_pdata owl_spi_pdata = {
	.max_chipselect = 4,
	.enable_dma = 1,
};

static const struct of_device_id acts_spi_dt_ids[] = {
	{
		.compatible = "actions,owl-spi",
		.data = &owl_spi_pdata,
	},
	{},
};

MODULE_DEVICE_TABLE(of, acts_spi_dt_ids);



static int asoc_spi_dma_probe(struct asoc_spi *asoc_spi)
{
	dma_cap_mask_t mask;

	if (!asoc_spi->enable_dma) {
		dev_dbg(asoc_spi->dev, "spi dma is disabled\n");
		return 0;
	}

	/* Try to acquire a generic DMA engine slave channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	/*
	 * We need both RX and TX channels to do DMA, else do none
	 * of them.
	 */
	asoc_spi->dma_rx_channel = dma_request_channel(mask, NULL, NULL);
	if (!asoc_spi->dma_rx_channel) {
		dev_err(asoc_spi->dev, "no RX DMA channel!\n");
		goto err_no_rxchan;
	}

	asoc_spi->dma_tx_channel = dma_request_channel(mask, NULL, NULL);
	if (!asoc_spi->dma_tx_channel) {
		dev_err(asoc_spi->dev, "no TX DMA channel!\n");
		goto err_no_txchan;
	}

	dev_dbg(asoc_spi->dev, "setup for DMA on RX %s, TX %s\n",
		 dma_chan_name(asoc_spi->dma_rx_channel),
		 dma_chan_name(asoc_spi->dma_tx_channel));

	return 0;

err_no_txchan:
	dma_release_channel(asoc_spi->dma_rx_channel);
	asoc_spi->dma_rx_channel = NULL;
err_no_rxchan:
	dev_err(asoc_spi->dev, "Failed to work in dma mode, work without dma!\n");
	return -ENODEV;
}

static void asoc_spi_unmap_free_dma_scatter(struct asoc_spi *asoc_spi)
{
	/* Unmap and free the SG tables */
	dma_unmap_sg(asoc_spi->dma_tx_channel->device->dev, asoc_spi->sgt_tx.sgl,
		     asoc_spi->sgt_tx.nents, DMA_TO_DEVICE);
	dma_unmap_sg(asoc_spi->dma_rx_channel->device->dev, asoc_spi->sgt_rx.sgl,
		     asoc_spi->sgt_rx.nents, DMA_FROM_DEVICE);
	sg_free_table(&asoc_spi->sgt_rx);
	sg_free_table(&asoc_spi->sgt_tx);
}

static void asoc_spi_terminate_dma(struct asoc_spi *asoc_spi)
{
	struct dma_chan *rxchan = asoc_spi->dma_rx_channel;
	struct dma_chan *txchan = asoc_spi->dma_tx_channel;

	dmaengine_terminate_all(rxchan);
	dmaengine_terminate_all(txchan);
	asoc_spi_unmap_free_dma_scatter(asoc_spi);
	asoc_spi->dma_running = false;
}

static void asoc_spi_dma_remove(struct asoc_spi *asoc_spi)
{
	if (asoc_spi->dma_running)
		asoc_spi_terminate_dma(asoc_spi);
	if (asoc_spi->dma_tx_channel)
		dma_release_channel(asoc_spi->dma_tx_channel);
	if (asoc_spi->dma_rx_channel)
		dma_release_channel(asoc_spi->dma_rx_channel);
}

static int asoc_spi_clk_enable(struct asoc_spi *asoc_spi)
{
	static int mod_ids[] = {MOD_ID_SPI0, MOD_ID_SPI1, MOD_ID_SPI2, MOD_ID_SPI3};
	
	int mod_id;
	int spi_no = asoc_spi_get_channel_no(asoc_spi->base);
	if(spi_no < 0)
		return -1;
		
	asoc_spi->clk = clk_get(asoc_spi->dev, "H_CLK");
	if (IS_ERR(asoc_spi->clk)) {
		return PTR_ERR(asoc_spi->clk);
	}

	mod_id = mod_ids[spi_no];
	
	module_clk_enable(mod_id);
	module_reset(mod_id);
	return 0;
}

static int asoc_spi_clk_disable(struct asoc_spi *asoc_spi)
{
	static int mod_ids[] = {MOD_ID_SPI0, MOD_ID_SPI1, MOD_ID_SPI2, MOD_ID_SPI3};
	
	int mod_id;
	int spi_no = asoc_spi_get_channel_no(asoc_spi->base);
	if(spi_no < 0)
		return -1;
		
	if (!IS_ERR(asoc_spi->clk))
		clk_put(asoc_spi->clk);

	mod_id = mod_ids[spi_no];
	
	module_clk_disable(mod_id);
	return 0;
}

static int __init asoc_spi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct asoc_spi *asoc_spi;
	struct owl_spi_pdata *spi_pdata;
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *match;
	int ret = 0;

	dev_dbg(&pdev->dev, "ASOC SPI: enter spi probe\n");
	dev_dbg(&pdev->dev, "pdev->name: %s\n", pdev->name ? pdev->name : "<null>");

	if (np == NULL)
		return -ENODEV;
	match = of_match_node(acts_spi_dt_ids, np);
	if (match == NULL)
		return -ENODEV;
	spi_pdata = (struct owl_spi_pdata *)match->data;

	master = spi_alloc_master(&pdev->dev, sizeof *asoc_spi);
	if (master == NULL) {
		dev_dbg(&pdev->dev, "master allocation failed\n");
		return -ENOMEM;
	}

	master->bus_num = of_alias_get_id(np, "spi");

	/* the spi->mode bits understood by this driver: */
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;

	master->setup = asoc_spi_setup;
	master->transfer = asoc_spi_transfer;
	master->cleanup = asoc_spi_cleanup;

	master->num_chipselect = spi_pdata->max_chipselect;
	master->dev.of_node = np;

	dev_set_drvdata(&pdev->dev, master);

	asoc_spi = spi_master_get_devdata(master);
	asoc_spi->dev = &pdev->dev;
	asoc_spi->irq = platform_get_irq(pdev, 0);
	asoc_spi->base =
		(platform_get_resource(pdev, IORESOURCE_MEM, 0)->start);

	asoc_spi_clk_enable(asoc_spi);
	asoc_spi->enable_dma = spi_pdata->enable_dma;
	if(asoc_spi_dma_probe(asoc_spi) < 0)
		goto out0;

	spin_lock_init(&asoc_spi->lock);
	INIT_WORK(&asoc_spi->work, asoc_spi_work);
	INIT_LIST_HEAD(&asoc_spi->msg_queue);
	asoc_spi->workqueue = create_singlethread_workqueue(
		dev_name(master->dev.parent));

	if (asoc_spi->workqueue == NULL) {
		ret = -EBUSY;
		goto out0;
	}
	ret = spi_register_master(master);
	if (ret < 0)
		goto out1;

	dev_dbg(&pdev->dev, "ASOC SPI: spi probe ok\n");

	return ret;

out1:
	destroy_workqueue(asoc_spi->workqueue);
out0:
	asoc_spi_dma_remove(asoc_spi);
	spi_master_put(master);
	
	dev_dbg(&pdev->dev, "ASOC SPI: spi probe failed\n");
	return ret;
}


static int __exit asoc_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master;
	struct asoc_spi *asoc_spi;

	dev_dbg(&pdev->dev, "spi remove\n");

	master = dev_get_drvdata(&pdev->dev);
	asoc_spi = spi_master_get_devdata(master);

	cancel_work_sync(&asoc_spi->work);
	asoc_spi_dma_remove(asoc_spi);
	asoc_spi_clk_disable(asoc_spi);

	spi_unregister_master(master);

	dev_dbg(&pdev->dev, "spi remove ok\n");
	
	return 0;
}

MODULE_ALIAS("platform: asoc_spi");


static struct platform_driver asoc_spi_driver = {
	.driver = {
		.name	= "asoc_spi0",
		.owner	= THIS_MODULE,
		.of_match_table = acts_spi_dt_ids,
		},
	.remove		= asoc_spi_remove,

};

static int __init asoc_spi_init(void)
{
	int err;
	err =  platform_driver_probe(&asoc_spi_driver, asoc_spi_probe);
	return err;
}
subsys_initcall(asoc_spi_init);

static void __exit asoc_spi_exit(void)
{
	platform_driver_unregister(&asoc_spi_driver);
}
module_exit(asoc_spi_exit);

MODULE_DESCRIPTION("Asoc SPI driver");
MODULE_LICENSE("GPL");
