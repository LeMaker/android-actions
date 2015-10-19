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

#define DRIVER_NAME			"asoc_spi"

#if 0
#define SPI_PRINT(fmt, args...) printk(KERN_ALERT fmt, ##args)
#else
#define SPI_PRINT(fmt, args...)
#endif
/*
u32 asoc_get_sclk(void)
{
	return 24000000;
}
*/

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

#define BYTES_4_DMA_XFER		64
#define BYTES_4_DMA_XFER_MAX        4096

struct asoc_spi {
/*	struct spi_master	*master;*/
/*	unsigned int irq; */
/*	struct owl_gpio *owl_spi_cs_table; */
	unsigned int irq;
	struct dma_slave_config *dma_config;
	struct owl_dma_slave *atslave;
	struct pinctrl *ppc;
	spinlock_t		lock;
	struct clk *clk;

	u32 base; /*phy addr*/

	struct workqueue_struct *workqueue;
	struct work_struct	work;

	/* Lock access to transfer list.	*/
	struct list_head	msg_queue;

	struct completion done;
	u8 busy;
	u8 enable_dma;
	void * dma_buf; //contiguous buf for dma

};

static inline void dump_spi_registers(struct asoc_spi *asoc_spi)
{
	SPI_PRINT("asoc_spi: SPI0_CTL(0x%x) = 0x%x\n", asoc_spi->base + SPI_CTL,
	       act_readl(asoc_spi->base + SPI_CTL));
	SPI_PRINT("asoc_spi: SPI0_STAT(0x%x) = 0x%x\n", asoc_spi->base + SPI_STAT,
	       act_readl(asoc_spi->base + SPI_STAT));
	SPI_PRINT("asoc_spi: SPI0_CLKDIV(0x%x) = 0x%x\n", asoc_spi->base + SPI_CLKDIV,
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

	SPI_PRINT("asoc_spi: required speed = %d\n", speed);
	SPI_PRINT("asoc_spi:spi clock = %d KHz(hclk = %d,clk_div = %d)\n",
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
			SPI_PRINT("wait num = %d\n", i);
			return 1;
		}
	}

	dump_spi_registers(asoc_spi);

	return -1;
}


static void spi_callback(void *completion)
{
	SPI_PRINT("spi callback ok\n");
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

	SPI_PRINT("%s(len:0x%x)\n", __func__, xfer->len);
	if (rx_buf != NULL)
		SPI_PRINT("  rx_buf:0x%x)\n", ((const u8 *)(xfer->rx_buf))[0]);
	if (tx_buf != NULL)
		SPI_PRINT("  tx_buf:0x%x)\n", ((const u8 *)(xfer->tx_buf))[0]);


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

	if (rx_buf != NULL)
		SPI_PRINT("  rx_buf:0x%x)\n", ((const u8 *)(xfer->rx_buf))[0]);
	if (tx_buf != NULL)
		SPI_PRINT("  tx_buf:0x%x)\n", ((const u8 *)(xfer->tx_buf))[0]);

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

	if (tx_buf)
		SPI_PRINT("    tx_buf 0x%x  0x%x\n", tx_buf[0], tx_buf[1]);

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
			SPI_PRINT("rx_buf 0x%x\n", rx_buf[-1]);
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

static unsigned int asoc_spi_config_write_dma_mode(unsigned int base)
{
	unsigned int dma_mode = 0;

	switch (base) {
		case SPI0_BASE:
			dma_mode = PRIORITY_ZERO | SRC_INCR | DST_CONSTANT |
		SRC_DCU | DST_DEV | SPI0_T;
			break;
		case SPI1_BASE:
			dma_mode =PRIORITY_ZERO | SRC_INCR | DST_CONSTANT |
		SRC_DCU | DST_DEV | SPI1_T;
			break;
		case SPI2_BASE:
			dma_mode = PRIORITY_ZERO | SRC_INCR | DST_CONSTANT |
		SRC_DCU | DST_DEV | SPI2_T;
			break;	
		case SPI3_BASE:
			dma_mode = PRIORITY_ZERO | SRC_INCR | DST_CONSTANT |
		SRC_DCU | DST_DEV | SPI3_T;
			break;
		default:
			pr_err("error: 0x%x.spi do not support\n", base);
			return -1;
	}

	return dma_mode;
}

static unsigned int asoc_spi_config_read_dma_mode(unsigned int base)
{
	unsigned int dma_mode = 0;

	switch (base) {
		case SPI0_BASE:
			dma_mode = PRIORITY_ZERO | DST_INCR |
		SRC_CONSTANT | DST_DCU | SRC_DEV | SPI0_R;
			break;
		case SPI1_BASE:
			dma_mode = PRIORITY_ZERO | DST_INCR |
		SRC_CONSTANT | DST_DCU | SRC_DEV | SPI1_R;
			break;
		case SPI2_BASE:
			dma_mode = PRIORITY_ZERO | DST_INCR |
		SRC_CONSTANT | DST_DCU | SRC_DEV | SPI2_R;
			break;	
		case SPI3_BASE:
			dma_mode = PRIORITY_ZERO | DST_INCR |
		SRC_CONSTANT | DST_DCU | SRC_DEV | SPI3_R;
			break;
		default:
			pr_err("error: 0x%x.spi do not support\n", base);
			return -1;
	}

	return dma_mode;
}


static int asoc_spi_write_by_dma(struct spi_device *spi,
	struct spi_transfer *xfer)
{
	u32 tx_reg, rx_reg, ctl_reg, stat_reg, txcr_reg;
	struct asoc_spi *asoc_spi;
	u32 tmp;
	unsigned int count = xfer->len;
	dma_cap_mask_t mask;
	int retval = 0;

	struct completion cmp;
	struct dma_chan *chan;
	dma_cookie_t		cookie;
	enum dma_status		status;
	struct dma_slave_config *dma_config;
	struct owl_dma_slave *atslave;
	struct dma_async_tx_descriptor *tx;
	int err;

	SPI_PRINT("start write\n");
	SPI_PRINT("count = %d\n", count);

	asoc_spi = spi_master_get_devdata(spi->master);
	ctl_reg = spi_reg(asoc_spi, SPI_CTL);
	tx_reg = spi_reg(asoc_spi, SPI_TXDAT);
	rx_reg = spi_reg(asoc_spi, SPI_RXDAT);
	stat_reg = spi_reg(asoc_spi, SPI_STAT);
	txcr_reg = spi_reg(asoc_spi, SPI_TXCR);

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	chan = dma_request_channel(mask, NULL, NULL);

	if (!chan) {
		SPI_PRINT("SPI: write request channel fail!\n");
		retval = -EINVAL;
		goto dma_write_err;
	}

	SPI_PRINT("request dma ok\n");


	dma_config = asoc_spi->dma_config;
	atslave = asoc_spi->atslave;
	atslave->mode =  asoc_spi_config_write_dma_mode(asoc_spi->base);
	if(atslave->mode == -1){		
		retval = -EINVAL;
		goto dma_write_err;
	}
	atslave->dma_dev  =  chan->device->dev;

	chan->private = (void *) atslave;

	dma_config->dst_addr = tx_reg;
	dma_config->direction = DMA_MEM_TO_DEV;

	err = dmaengine_slave_config(chan, dma_config);

	if (err) {
		SPI_PRINT("call the write slave config error\n");
		retval = -EINVAL;
		goto dma_write_err;
	}

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
	tmp |= (SPIx_CTL_RWC(1) |
		SPIx_CTL_RDIC(2) |
		SPIx_CTL_TDIC(2) |
		SPIx_CTL_TDEN);
	act_writel(tmp, ctl_reg);

	tx = dmaengine_prep_slave_single(chan, xfer->tx_dma,
		count, DMA_MEM_TO_DEV, 0);

	if (!tx) {
		SPI_PRINT("prep write error!\n");
		retval = -EINVAL;
		goto dma_write_err;
	}
	
	tmp = count / 4;
	act_writel(tmp, txcr_reg);

	init_completion(&cmp);
	tx->callback = spi_callback;
	tx->callback_param = &cmp;
	cookie = dmaengine_submit(tx);

	if (dma_submit_error(cookie)) {
		SPI_PRINT("submit write error!\n");
		retval = -EINVAL;
		goto dma_write_err;
	}


	dma_async_issue_pending(chan);


	/* setup the dma */
	SPI_PRINT("write start dma\n");
	if (!wait_for_completion_timeout(&cmp, 5 * HZ)) {
		dev_err(&spi->dev, "wait_for_completion timeout while send by dma\n");
		dump_spi_registers(asoc_spi);
		owl_dma_dump_all(chan);
		retval = -EINVAL;
		goto dma_write_err;
	}

	status = dma_async_is_tx_complete(chan, cookie, NULL, NULL);

	if (status != DMA_SUCCESS) {
		dev_err(&spi->dev, "transfer not succeed\n");
		retval = -EINVAL;
		goto dma_write_err;
	}

	SPI_PRINT("wake\n");

	dump_spi_registers(asoc_spi);

	if (asoc_spi_wait_till_ready(asoc_spi) < 0) {
		dev_err(&spi->dev, "TXS timed out\n");
		retval = -EINVAL;
		goto dma_write_err;
	}

	SPI_PRINT("check txs complete ok\n");

	if (act_readl(stat_reg) &
	    (SPIx_STAT_RFER | SPIx_STAT_TFER | SPIx_STAT_BEB)) {
		dev_err(&spi->dev, "spi state error while send by dma\n");
		dump_spi_registers(asoc_spi);
		retval = -EINVAL;
		goto dma_write_err;
	}

	SPI_PRINT("check bus ok\n");

	

	dma_release_channel(chan);
	
	return retval;

dma_write_err:
	
	dma_release_channel(chan);
	return retval;
}

static int asoc_spi_read_by_dma(struct spi_device *spi,
			   struct spi_transfer *xfer)
{
	u32 tx_reg, rx_reg, ctl_reg, stat_reg, tcnt_reg, rxcr_reg;
	struct asoc_spi *asoc_spi;
	u32 tmp;
	unsigned int count = xfer->len;
	int retval = 0;
	struct completion cmp;
	struct dma_chan *chan;
	dma_cookie_t		cookie;
	enum dma_status		status;
	struct dma_slave_config *dma_config;
	struct owl_dma_slave *atslave;
	struct dma_async_tx_descriptor *tx;
	dma_cap_mask_t mask;
	int err;

	asoc_spi = spi_master_get_devdata(spi->master);
	ctl_reg = spi_reg(asoc_spi, SPI_CTL);
	tx_reg = spi_reg(asoc_spi, SPI_TXDAT);
	rx_reg = spi_reg(asoc_spi, SPI_RXDAT);
	stat_reg = spi_reg(asoc_spi, SPI_STAT);
	rxcr_reg = spi_reg(asoc_spi, SPI_RXCR);
	tcnt_reg = spi_reg(asoc_spi, SPI_TCNT);

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	chan = dma_request_channel(mask, NULL, NULL);
	if (!chan) {
		SPI_PRINT("SPI: read request channel fail!\n");		
		retval = -EINVAL;
		goto dma_read_err;
	}

	SPI_PRINT("request dma ok\n");

	dma_config = asoc_spi->dma_config;
	atslave = asoc_spi->atslave;

	atslave->mode =  asoc_spi_config_read_dma_mode(asoc_spi->base);
	if(atslave->mode == -1){
		retval = -EINVAL;
		goto dma_read_err;
	}
	atslave->dma_dev  =  chan->device->dev;

	chan->private = (void *) atslave;

	dma_config->src_addr = rx_reg;
	dma_config->direction = DMA_DEV_TO_MEM;

	err = dmaengine_slave_config(chan, dma_config);

	if (err) {
		SPI_PRINT("call the read slave config error\n");
		retval = -EINVAL;
		goto dma_read_err;
	}

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
	tmp |= (SPIx_CTL_RWC(2) |
		SPIx_CTL_RDIC(2) |
		SPIx_CTL_TDIC(2) |
		SPIx_CTL_SDT(1) |
		SPIx_CTL_DTS |
		SPIx_CTL_RDEN);
	act_writel(tmp, ctl_reg);

	tx = dmaengine_prep_slave_single
		(chan, xfer->rx_dma, count, DMA_DEV_TO_MEM, 0);

	if (!tx) {
		SPI_PRINT("prep read error!\n");
		retval = -EINVAL;
		goto dma_read_err;
	}

	tmp = count / 4;
	act_writel(tmp, tcnt_reg);

	tmp = count / 4;
	act_writel(tmp, rxcr_reg);

	init_completion(&cmp);
	tx->callback = spi_callback;
	tx->callback_param = &cmp;
	cookie = dmaengine_submit(tx);

	if (dma_submit_error(cookie)) {
		SPI_PRINT("submit read error!\n");
		retval = -EINVAL;
		goto dma_read_err;
	}

	dma_async_issue_pending(chan);

	if (!wait_for_completion_timeout(&cmp, 5 * HZ)) {
		dev_err(&spi->dev, "read wait_for_completion timeout while receive by dma\n");
		owl_dma_dump_all(chan);
		retval = -EINVAL;
		goto dma_read_err;
	}

	status = dma_async_is_tx_complete(chan, cookie, NULL, NULL);

	if (status != DMA_SUCCESS) {
		dev_err(&spi->dev, "transfer not succeed\n");
		retval = -EINVAL;
		goto dma_read_err;
	}

	if (asoc_spi_wait_till_ready(asoc_spi) < 0) {
		dev_err(&spi->dev, "RXS timed out\n");
		retval = -EINVAL;
		goto dma_read_err;
	}

	if (act_readl(stat_reg) &
	    (SPIx_STAT_RFER | SPIx_STAT_TFER | SPIx_STAT_BEB)) {
		dev_err(&spi->dev, "spi state error while send by dma\n");
		dump_spi_registers(asoc_spi);
		retval = -EINVAL;
		goto dma_read_err;
	}

	dma_release_channel(chan);

	return retval;
dma_read_err:
	dma_release_channel(chan);
	return retval;
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
		(len > BYTES_4_DMA_XFER_MAX) ||
		(asoc_spi->enable_dma == 0) ||
		(xfer->bits_per_word != 32)) {
		unsigned int count = 0;

		SPI_PRINT("cpu wr\n");

		if (word_len == 8)
			count = asoc_spi_write_read_8bit(spi, xfer);
		else if (word_len == 16)
			count = asoc_spi_write_read_16bit(spi, xfer);
		else if (word_len == 32)
			count = asoc_spi_write_read_32bit(spi, xfer);

		return len - count;

	} else {
		int retval = 0;
		const void *tx = xfer->tx_buf;
		void *rx = xfer->rx_buf;

		SPI_PRINT("dma wr\n");
		SPI_PRINT("tx addr = 0x%x\n", tx);

		if (tx && (!rx)) {
			memcpy(asoc_spi->dma_buf, tx, len);
			xfer->tx_dma = dma_map_single
				(NULL, asoc_spi->dma_buf, len, DMA_TO_DEVICE);

			retval = asoc_spi_write_by_dma(spi, xfer);

			dma_unmap_single(NULL,
				xfer->tx_dma, len, DMA_TO_DEVICE);
		} else if ((!tx) && rx) {
			memcpy(asoc_spi->dma_buf, rx, len);
			xfer->rx_dma = dma_map_single
				(NULL, (void *)rx, len, DMA_FROM_DEVICE);

			retval = asoc_spi_read_by_dma(spi, xfer);

			dma_unmap_single(NULL, xfer->rx_dma,
				len, DMA_FROM_DEVICE);

		} else {
			dev_err(&spi->dev, "cannot support full duplex xfer by dma yet!\n");
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
	//printk("#######%d %d\n", t->len,  bits_per_word);
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

	SPI_PRINT("ASOC SPI: enter spi work\n");


	spin_lock_irq(&asoc_spi->lock);
	while (!list_empty(&asoc_spi->msg_queue)) {
		struct spi_message *m;
		struct spi_device *spi;
		struct spi_transfer *t = NULL;
		int par_override = 0;
		int status = 0;
		int cs_active = 0;

		SPI_PRINT("asoc_spi: start one message\n");

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
			SPI_PRINT("asoc_spi: start one transfer\n");

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
			SPI_PRINT("asoc_spi: end one transfer\n");

		}

msg_done:
		if (cs_active)
			asoc_spi_deactivate_cs(spi);

		m->status = status;
		m->complete(m->context);

		SPI_PRINT("asoc_spi: end one message\n");
		spin_lock_irq(&asoc_spi->lock);
	}

	spin_unlock_irq(&asoc_spi->lock);

	SPI_PRINT("ASOC SPI: quit spi work\n");

}

static int asoc_spi_setup(struct spi_device *spi)
{
	struct asoc_spi *asoc_spi;
	unsigned int spi_source_clk_hz;

	SPI_PRINT("ASOC SPI: enter spi setup\n");

	asoc_spi = spi_master_get_devdata(spi->master);

	if (spi->bits_per_word > 32)
		return -EINVAL;

	if (spi->bits_per_word == 0)
		spi->bits_per_word = 8;

	spi_source_clk_hz = clk_get_rate(asoc_spi->clk);

	pr_info("ahb freq is %d\n", spi_source_clk_hz);


	if ((spi->max_speed_hz == 0)
			|| (spi->max_speed_hz > spi_source_clk_hz))
		spi->max_speed_hz = spi_source_clk_hz;

	SPI_PRINT("ASOC SPI: ok spi setup\n");

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


//static int spi_set_pin_mux(struct platform_device *pdev,
//	struct pinctrl *ppc)
//{
//	int ret = 0;
//
//	ppc =  pinctrl_get_select_default(&pdev->dev);
//	ret = IS_ERR(ppc);
//	if (ret) {
//		printk("spi pinctrl get failed, ret = %d\n", ret);
//		return ret;
//	}
//}

static void spi_put_pin_mux(struct asoc_spi *spi)
{
	if (spi->ppc)
		pinctrl_put(spi->ppc);
}

static int __init asoc_spi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct asoc_spi *asoc_spi;
	struct owl_spi_pdata *spi_pdata;
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *match;
	int ret = 0;

	printk("ASOC SPI: enter spi probe\n");

	SPI_PRINT("pdev->name: %s\n", pdev->name ? pdev->name : "<null>");


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
	asoc_spi->irq = platform_get_irq(pdev, 0);
	asoc_spi->base =
		(platform_get_resource(pdev, IORESOURCE_MEM, 0)->start);


	asoc_spi->clk = clk_get(&pdev->dev, "H_CLK");
	if (IS_ERR(asoc_spi->clk)) {
		ret = PTR_ERR(asoc_spi->clk);
		goto out0;
	}
/*
	ret = spi_set_pin_mux(pdev, asoc_spi->ppc);
	if (ret)
		goto out0;
*/
	asoc_spi->enable_dma = spi_pdata->enable_dma;

	if (asoc_spi->enable_dma) {
		asoc_spi->dma_config = kzalloc(sizeof(struct dma_slave_config), GFP_KERNEL);
		asoc_spi->atslave = kzalloc(sizeof(struct owl_dma_slave), GFP_KERNEL);
		asoc_spi->atslave->trans_type  =  SLAVE;
		asoc_spi->dma_buf = kmalloc(4096, GFP_KERNEL);
		memset(asoc_spi->dma_buf, 0, 4096);
	}

	SPI_PRINT("%s_%d:\n", __FUNCTION__, __LINE__);

	spin_lock_init(&asoc_spi->lock);
	init_completion(&asoc_spi->done);
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

	SPI_PRINT("ASOC SPI: spi probe ok\n");

    if(asoc_spi->base == 0xb0208000) {
		act_writel(act_readl(0xb01600a4) | (0x1 << 12), 0xb01600a4);
		act_writel(act_readl(0xb01600ac) | (0x1 << 10), 0xb01600ac);
		act_writel(act_readl(0xb01b004c) | (0x1 << 2) , 0xb01b004c);
    }

	return ret;

out1:
	destroy_workqueue(asoc_spi->workqueue);
out0:
	spi_master_put(master);
	return ret;
}


static int __exit asoc_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master;
	struct asoc_spi *asoc_spi;

	printk("spi remove\n");

	master = dev_get_drvdata(&pdev->dev);
	asoc_spi = spi_master_get_devdata(master);
	spi_put_pin_mux(asoc_spi);
	clk_put(asoc_spi->clk);

	if (asoc_spi->enable_dma) {
		kfree(asoc_spi->dma_config);
		kfree(asoc_spi->atslave);
		kfree(asoc_spi->dma_buf);
	}

	cancel_work_sync(&asoc_spi->work);

	spi_unregister_master(master);

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
