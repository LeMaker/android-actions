#include <asm/arch/gmp.h>
#include <asm/arch/clocks.h>
#include <asm/io.h>
#include <common.h>
#include <asm/arch/pmu.h>
#include <asm/arch/spi.h>
#include <spi.h>

#define SPI_BASE(num) ((SPI0_BASE) + (num) * 0x4000)
#define MAX_SPI_POLL_LOOPS 5000
#define SPIx_STAT_TCOM  (1 << 2)

struct spi_slave g_nor_spi_slave = {
	.bus = 2,
	.cs = 0,
	.max_write_size = 32,
};

struct owl_spi_regs {
	u32 ctl;
	u32 clkdiv;
	u32 stat;
	u32 rxdat;
	u32 txdat;
};

void spi_clk_init(int num)
{
	if (num != 0) {
		setbits_le32(CMU_DEVCLKEN1, (1 << (10 + num)));
		clrbits_le32(CMU_DEVRST1, (1 << (8 + num)));
		setbits_le32(CMU_DEVRST1, (1 << (8 + num)));
	}
}

void spi_mfp_init(int num)
{
	if (num == 1) {
		clrsetbits_le32(MFP_CTL2, 0x7, 0x2); /*MISO over uart0_rx */
		clrsetbits_le32(MFP_CTL3, (0x7 << 19), (0x2 << 19)); /*SS over uart0_tx */
		clrsetbits_le32(MFP_CTL3, (0x7 << 16), (0x4 << 16)); /*MOSI,CLK over i2c0 */
	} else if(num == 2){
		clrsetbits_le32(MFP_CTL3, (0x1 << 2), (0x1 << 2));
	}
}

void spi_init(void)
{
	int num = g_nor_spi_slave.bus;
	struct owl_spi_regs *s =
		(struct owl_spi_regs *)SPI_BASE(num);

	spi_clk_init(num);
	spi_mfp_init(num);

	writel(readl(CMU_BUSCLK1) & 0xfffffff3 , CMU_BUSCLK1);
	debug("%x %x\n", readl(CMU_COREPLL), readl(CMU_BUSCLK1));
	
	writel(3, &s->clkdiv);           /*set SPI_CLK=H_clk/(CLKDIV*2)*/

	writel(0x00c0, &s->ctl);         /* select SPI 8bit mode 3 */
	writel(0xFFFFFFFF, &s->stat);    /* clear SPI status register */
	setbits_le32(&s->ctl, 0x1 << 18);
}

void spi_cs_activate(struct spi_slave *slave)
{
	int num = slave->bus;
	struct owl_spi_regs *s =
		(struct owl_spi_regs *)SPI_BASE(num);

	clrbits_le32(&s->ctl, 0x10);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	int num = slave->bus;
	struct owl_spi_regs *s =
		(struct owl_spi_regs *)SPI_BASE(num);

	setbits_le32(&s->ctl, 0x10);
}

int owl_spi_wait_till_ready(struct owl_spi_regs *s)
{
	int i;

	for (i = 0; i < MAX_SPI_POLL_LOOPS; i++) {
		if (readl(&s->stat) & SPIx_STAT_TCOM) {
			writel(readl(&s->stat) | SPIx_STAT_TCOM, &s->stat);
			return 1;
		}
	}
	
	return -1;
}

int owl_spi_write_read_8bit(struct owl_spi_regs *s, unsigned int bitlen, const void *dout,
		void *din)
{
	const u8 *tx_buf = dout;
	u8 *rx_buf = din;
	unsigned int count = bitlen / 8;
	
	clrbits_le32(&s->ctl, 0x300);
	debug("%d s->ctl: %x %x %d\n", __LINE__, readl(&s->ctl), readl(&s->stat), count);
	writel(0x30, &s->stat);    /* clear SPI FIFO */

	do {
		if (tx_buf)
			writel(*tx_buf++, &s->txdat);
		else
			writel(0, &s->txdat);

		if (owl_spi_wait_till_ready(s) < 0) {
			printf("TXS timed out\n");
			return count;
		}

		if (rx_buf)
			*rx_buf++ = readl(&s->rxdat);

		count--;
	} while (count);

    return count;		
}

int owl_spi_write_read_32bit(struct owl_spi_regs *s, unsigned int bitlen, const void *dout,
		void *din)
{
	const u32 *tx_buf = dout;
	u32 *rx_buf = din;
	unsigned int count = bitlen / 8;
	
	setbits_le32(&s->ctl, 0x200);
	debug("%d s->ctl: %x %x %d\n", __LINE__, readl(&s->ctl), readl(&s->stat), count);
	writel(0x30, &s->stat);    /* clear SPI FIFO */

	do {
		if (tx_buf)
			writel(*tx_buf++, &s->txdat);
		else
			writel(0, &s->txdat);

		if (owl_spi_wait_till_ready(s) < 0) {
			printf("TXS timed out\n");
			return count;
		}

		if (rx_buf){
			*rx_buf++ = readl(&s->rxdat);
		}

		count -= 4;
	} while (count);
	
    return count;	
}

int  spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
		void *din, unsigned long flags)
{
	int num = slave->bus, ret = 0;
	unsigned int bytes = bitlen / 8;
	struct owl_spi_regs *s =
		(struct owl_spi_regs *)SPI_BASE(num);

	debug("%d %d %lx\n", __LINE__, bitlen, flags);
	if(bitlen <= 0)
		goto out;

	if (flags & SPI_XFER_BEGIN){
		spi_cs_activate(slave);
	}

	if(bytes >= 32 && bytes % 32 == 0){
		ret = owl_spi_write_read_32bit(s, bitlen, dout, din);
	}else{
		ret = owl_spi_write_read_8bit(s, bitlen, dout, din);
	}
	debug("spi_xfer ret: %d\n", ret);

out:	
	if (flags & SPI_XFER_END){
		spi_cs_deactivate(slave);
	}
	return ret;
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
			unsigned int max_hz, unsigned int mode)
{
	spi_init();
	return &g_nor_spi_slave;
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	return;
}

void spi_free_slave(struct spi_slave *slave)
{
	return;
}
