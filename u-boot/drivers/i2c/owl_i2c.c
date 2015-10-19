/*
 * arch/arm/mach-leopard/owl_i2c.c
 *
 * I2C master mode controller driver for Actions SOC
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <common.h>
#include <mmc.h>
#include <part.h>
#include <asm/arch/actions_reg_owl.h>
#include <asm/io.h>
#include <asm/arch/owl_dma.h>
#include <asm/processor.h>
#include <asm/errno.h>
#include <malloc.h>
#include <linux/ctype.h>

#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#define DEVCLKEN_GPIO			(0x1 << 18)

#undef GS5203_I2C_DBG
#ifdef GS5203_I2C_DBG
#define i2c_dbg(fmt, args...)	printf(fmt, ##args)
#else
#define i2c_dbg(fmt, args...)
#endif

#define I2C_BASE				0xB0170000
#define I2C_BANK_WIDTH		0X4000
#define I2C_BUS_BASE(bus)	(I2C_BASE + (bus)*I2C_BANK_WIDTH)

#define I2C0_CLK_EN			(1<<14)
#define I2C1_CLK_EN			(1<<15)
#define I2C2_CLK_EN			(1<<30)
#define I2C3_CLK_EN			(1<<31)

/* I2Cx_CTL */
#define I2C_CTL_GRAS		(0x1 << 0)
/* Generate ACK or NACK Signal */
#define I2C_CTL_GRAS_ACK	0
/* generate the ACK signal at 9th clock of SCL */
#define I2C_CTL_GRAS_NACK	I2C_CTL_GRAS
/* generate the NACK signal at 9th clock of SCL */
#define I2C_CTL_RB			(0x1 << 1)
/* Release Bus */
#define I2C_CTL_GBCC_MASK	(0x3 << 2)
/* Loop Back Enable */
#define I2C_CTL_GBCC(x)			(((x) & 0x3) << 2)
#define I2C_CTL_GBCC_NONE		I2C_CTL_GBCC(0)
#define I2C_CTL_GBCC_START		I2C_CTL_GBCC(1)
#define I2C_CTL_GBCC_STOP		I2C_CTL_GBCC(2)
#define I2C_CTL_GBCC_RESTART	I2C_CTL_GBCC(3)
#define I2C_CTL_IRQE			(0x1 << 5)
/* IRQ Enable */
#define I2C_CTL_PUEN			(0x1 << 6)
/* Internal Pull-Up resistor (1.5k) enable. */
#define I2C_CTL_EN				(0x1 << 7)
/* Enable. When enable, reset the status machine to IDLE */
#define I2C_CTL_AE				(0x1 << 8)
/* Arbitor enable */

/* I2Cx_CLKDIV */
#define I2C_CLKDIV_DIV_MASK	(0xff << 0)
/* Clock Divider Factor (only for master mode). */
#define I2C_CLKDIV_DIV(x)		(((x) & 0xff) << 0)

/* I2Cx_STAT */
#define I2C_STAT_RACK			(0x1 << 0)
/* Receive ACK or NACK when transmit data or address */
#define I2C_STAT_BEB			(0x1 << 1)
/* IRQ Pending Bit, Write ¡°1¡± to clear this bit */
#define I2C_STAT_IRQP			(0x1 << 2)
/* IRQ Pending Bit, Write ¡°1¡± to clear this bit */
#define I2C_STAT_LAB			(0x1 << 3)
/* Lose arbitration bit, Write ¡°1¡± to clear this bit */
#define I2C_STAT_STPD			(0x1 << 4)
/* Stop detect bit, Write ¡°1¡± to clear this bit */
#define I2C_STAT_STAD			(0x1 << 5)
/* Start detect bit, Write ¡°1¡± to clear this bit */
#define I2C_STAT_BBB			(0x1 << 6)
/* Bus busy bit */
#define I2C_STAT_TCB			(0x1 << 7)
/* Transfer complete bit */
#define I2C_STAT_LBST			(0x1 << 8)
/* Last Byte Status Bit, 0: address, 1: data */
#define I2C_STAT_SAMB			(0x1 << 9)
/* Slave address match bit */
#define I2C_STAT_SRGC			(0x1 << 10)
/* Slave receive general call */

#define I2C_BUS_ERR_MSK		(I2C_STAT_LAB | I2C_STAT_BEB)
/* I2Cx_CMD */
#define I2C_CMD_SBE				(0x1 << 0)
/* Start bit enable */
#define I2C_CMD_AS_MASK		(0x7 << 1)
/* Address select */
#define I2C_CMD_AS(x)			(((x) & 0x7) << 1)
#define I2C_CMD_RBE				(0x1 << 4)
/* Restart bit enable */
#define I2C_CMD_SAS_MASK		(0x7 << 5)
/* Second Address select */
#define I2C_CMD_SAS(x)			(((x) & 0x7) << 5)
#define I2C_CMD_DE				(0x1 << 8)
/* Data enable */
#define I2C_CMD_NS				(0x1 << 9)
/* NACK select */
#define I2C_CMD_SE				(0x1 << 10)
/* Stop enable */
#define I2C_CMD_MSS			(0x1 << 11)
/* MSS Master or slave mode select */
#define I2C_CMD_WRS			(0x1 << 12)
/* Write or Read select */
#define I2C_CMD_EXEC			(0x1 << 15)
/* Start to execute the command list */

/*FIFO mode write cmd 0x8d01 */
#define I2C_CMD_X	(\
	I2C_CMD_EXEC | I2C_CMD_MSS | \
	I2C_CMD_SE | I2C_CMD_DE | I2C_CMD_SBE)

/* I2Cx_FIFOCTL */
#define I2C_FIFOCTL_NIB			(0x1 << 0)
/* NACK Ignore Bit */
#define I2C_FIFOCTL_RFR			(0x1 << 1)
/* RX FIFO reset bit, Write 1 to reset RX FIFO */
#define I2C_FIFOCTL_TFR			(0x1 << 2)
/* TX FIFO reset bit, Write 1 to reset TX FIFO */

/* I2Cx_FIFOSTAT */
#define I2C_FIFOSTAT_CECB		(0x1 << 0)
/* command Execute Complete bit */
#define I2C_FIFOSTAT_RNB		(0x1 << 1)
/* Receive NACK Error bit */
#define I2C_FIFOSTAT_RFE		(0x1 << 2)
/* RX FIFO empty bit */
#define I2C_FIFOSTAT_RFF		(0x1 << 3)
/* RX FIFO full bit */
#define I2C_FIFOSTAT_TFE		(0x1 << 4)
/* TX FIFO empty bit */
#define I2C_FIFOSTAT_TFF		(0x1 << 5)
/* TX FIFO full bit */
#define I2C_FIFOSTAT_RFD_MASK	(0xff << 8)
/* Rx FIFO level display */
#define I2C_FIFOSTAT_RFD_SHIFT	(8)
#define I2C_FIFOSTAT_TFD_MASK	(0xff << 16)
/* Tx FIFO level display */
#define I2C_FIFOSTAT_TFD_SHIFT	(16)

#define I2C_CTL_START_CMD	(  \
		I2C_CTL_IRQE |   \
		I2C_CTL_EN |     \
		I2C_CTL_GBCC_START |    \
		I2C_CTL_PUEN |     \
		I2C_CTL_RB   \
	)

#define I2C_CTL_STOP_CMD	(  \
		I2C_CTL_EN |  \
		I2C_CTL_PUEN | \
		I2C_CTL_GBCC_STOP | \
		I2C_CTL_RB | \
		I2C_CTL_IRQE \
	)

#define I2C_CTL_RESTART_CMD	( \
		I2C_CTL_IRQE | \
		I2C_CTL_EN | \
		I2C_CTL_GBCC_RESTART | \
		I2C_CTL_PUEN | \
		I2C_CTL_RB \
	)

#define I2C_WRITE		0
#define I2C_READ		1

#define I2C_OK			0
#define I2C_NOK			1
#define I2C_NACK		2
#define I2C_NOK_TOUT	3

#define I2C_TIMEOUT		1

struct gs5203_i2c {
	u32 ctl;		/* I2C Control Register */
	u32 clkdiv;		/* I2C Clk Divide Register */
	u32 stat;		/* I2C Status Register */
	u32 addr;		/* I2C Address Register */
	u32 txdat;		/* I2C TX Data Register */
	u32 rxdat;		/* I2C RX Data Register */
	u32 cmd;		/* I2C Command Register */
	u32 fifoctl;		/* I2C FIFO control Register */
	u32 fifostat;		/* I2C FIFO status Register */
	u32 datcnt;		/* I2C Data transmit counter */
	u32 rcnt;		/* I2C Data transmit remain counter */
};

static u32 g_current_bus = -1;	/* Stores Current I2C Bus */

#if 0
static void acts_dump_mfp(void)
{
	printf("\tMFP_CTL0:0x%x\n", readl(MFP_CTL0));
	printf("\tMFP_CTL1:0x%x\n", readl(MFP_CTL1));
	printf("\tMFP_CTL2:0x%x\n", readl(MFP_CTL2));
	printf("\tMFP_CTL3:0x%x\n", readl(MFP_CTL3));
	printf("\tPAD_PULLCTL0:0x%x\n", readl(PAD_PULLCTL0));
	printf("\tPAD_PULLCTL1:0x%x\n", readl(PAD_PULLCTL1));
	printf("\tPAD_PULLCTL2:0x%x\n", readl(PAD_PULLCTL2));
	printf("\tPAD_DRV0:0x%x\n", readl(PAD_DRV0));
	printf("\tPAD_DRV1:0x%x\n", readl(PAD_DRV1));
	printf("\tPAD_DRV2:0x%x\n", readl(PAD_DRV2));
	printf("\tCMU_DEVCLKEN0:0x%x\n", readl(CMU_DEVCLKEN0));
	printf("\tCMU_DEVCLKEN1:0x%x\n", readl(CMU_DEVCLKEN1));
}
#endif

static inline void owl_i2c_mfp_config(u32 bus)
{
	int  ret;
	switch (bus) {
	case 0:
		/* MFP_CTL3 bit 18:16 = 000 config p_i2c0_sclk
		p_i2c0_sdata as i2c0 clk data  */
		writel((readl(MFP_CTL3) & 0xfff8ffff), MFP_CTL3);
		writel((readl(MFP_CTL3) & ~0x00380000), MFP_CTL3);
		writel((readl(MFP_CTL2) & ~0x00000007), MFP_CTL2);
#if 0
		/* MFP_CTL3 bit 21:19 = 011 config P_UART0_TX as i2c0 clk  */
		writel((readl(MFP_CTL3) & 0xffdfffff), MFP_CTL3);
		/* MFP_CTL2 bit 2:0 = 011 config P_UART0_RX as i2c0 SDATA  */
		writel((readl(MFP_CTL2) & 0xfffffffb), MFP_CTL2);
#endif
		break;

	case 1:
		/* MFP_CTL3 bit 18:16 = 010
		config p_i2c0_sclk p_i2c0_sdata as i2c1 clk data  */
		writel((readl(MFP_CTL3) & 0xfffaffff), MFP_CTL3);
		break;

	case 2:
		break;

	case 3:
		/*MFP_CTL1 bit 5:4 = 10
		config p_spi0_sclk p_spi0_mosi as i2c3 clk data  */
		ret = readl(MFP_CTL1);
		ret |= 1 << 4;
		ret &= ~(1 << 3);
		writel(ret, MFP_CTL1);
		break;

	default:
		printf("Invalid adap[%d] !\r\n", bus);
		break;
	}
}

static inline void owl_i2c_pullup_enable(u32 bus)
{
	switch (bus) {
	case 0:
		/* PAD_PULLCTL0  bit 9:8 = 11
		enable p_i2c0_sclk p_i2c0_sdata pull up resist  */
		writel((readl(PAD_PULLCTL0) | 0x0300), PAD_PULLCTL0);
		break;

	case 1:
		/* PAD_PULLCTL2  bit 10:9 = 11
		enable p_i2c1_sclk_P p_i2c1_sdata_P pull up resist  */
		writel((readl(PAD_PULLCTL2) | 0x0600), PAD_PULLCTL2);
		break;

	case 2:
		/* PAD_PULLCTL2  bit 8:7 = 11
		enable p_i2c1_sclk_P p_i2c1_sdata_P pull up resist  */
		writel((readl(PAD_PULLCTL2) | 0x0180), PAD_PULLCTL2);
		break;

	case 3:
		writel((readl(PAD_PULLCTL2) | 0x01800), PAD_PULLCTL2);
		break;

	default:
		printf("Invalid adap[%d] !\r\n", bus);
		break;
	}
}

static inline void owl_i2c_pullup_disable(u32 bus)
{
	switch (bus) {
	case 0:
		/* PAD_PULLCTL0  bit 9:8 = 11
		enable p_i2c0_sclk p_i2c0_sdata pull up resist  */
		writel((readl(PAD_PULLCTL0) & 0xfffffcff), PAD_PULLCTL0);
		break;

	case 1:
		/* PAD_PULLCTL2  bit 10:9 = 11
		enable p_i2c1_sclk_P p_i2c1_sdata_P pull up resist  */
		writel((readl(PAD_PULLCTL2) & 0xfffff9ff), PAD_PULLCTL2);
		break;

	case 2:
		/* PAD_PULLCTL2  bit 8:7 = 11
		enable p_i2c1_sclk_P p_i2c1_sdata_P pull up resist  */
		writel((readl(PAD_PULLCTL2) & 0xfffffe7f), PAD_PULLCTL2);
		break;

	case 3:
		break;

	default:
		printf("Invalid adap[%d] !\r\n", bus);
		break;
	}
}

static inline void owl_i2c_clk_enable(u32 bus)
{
	uint reg_val;

	reg_val = readl(CMU_ETHERNETPLL);
	if((reg_val & 1U) == 0) {
		setbits_le32(CMU_ETHERNETPLL, 0x1);	//i2c clock source.
		udelay(300);
	}

	switch (bus) {
	case 0:
		setbits_le32(CMU_DEVCLKEN1, I2C0_CLK_EN);
		break;

	case 1:
		setbits_le32(CMU_DEVCLKEN1, I2C1_CLK_EN);
		break;

	case 2:
		setbits_le32(CMU_DEVCLKEN1, I2C2_CLK_EN);
		break;

	case 3:
		setbits_le32(CMU_DEVCLKEN0, DEVCLKEN_GPIO);
		setbits_le32(CMU_DEVCLKEN1, I2C3_CLK_EN);
		break;

	default:
		printf("Invalid adap[%d] !\r\n", bus);
		break;
	}
}

static inline void owl_i2c_clk_disable(u32 bus)
{
	switch (bus) {
	case 0:
		clrbits_le32(CMU_DEVCLKEN1, I2C0_CLK_EN);
		break;

	case 1:
		clrbits_le32(CMU_DEVCLKEN1, I2C1_CLK_EN);
		break;

	case 2:
		clrbits_le32(CMU_DEVCLKEN1, I2C2_CLK_EN);
		break;

	case 3:
		clrbits_le32(CMU_DEVCLKEN1, I2C3_CLK_EN);
		break;

	default:
		printf("Invalid adap[%d] !\r\n", bus);
		break;
	}
}

static struct gs5203_i2c *get_i2c_base(void)
{
	return (struct gs5203_i2c *)(I2C_BASE +
				      g_current_bus * I2C_BANK_WIDTH);
}

int i2c_set_bus_num(unsigned int bus)
{
	struct gs5203_i2c *i2c;
	static int speed = 0x10;
	/*set i2c clock, speed=0x3f : 100k ; speed=0x10 : 400k*/

	if ((bus < 0) || (bus >= 4)) {
		i2c_dbg("Bad bus: %d\n", bus);
		return -1;
	}
	if(g_current_bus == bus) {
		return 0;
	}
	g_current_bus = bus;

	owl_i2c_mfp_config(bus);
	owl_i2c_clk_enable(bus);
	owl_i2c_pullup_enable(bus);

	i2c = get_i2c_base();

	/*set i2c speed100k*/
	writel(speed, &i2c->clkdiv);

	return 0;
}

u32 i2c_get_bus_num(void)
{
	return g_current_bus;
}

static inline void i2c_writel(u32 reg, u32 val)
{
	writel(val, reg);
	i2c_dbg("-->>write 0x%x to 0x%x\r\n", val, reg);
}

static inline u32 i2c_readl(u32 reg)
{
	return readl(reg);
}

/*
 * cmd_type is 0 for write, 1 for read.
 *
 * addr_len can take any value from 0-255, it is only limited
 * by the char, we could make it larger if needed. If it is
 * 0 we skip the address write cycle.
 */
int i2c_transfer(struct gs5203_i2c *i2c,
		 unsigned char cmd_type,
		 unsigned char chip,
		 unsigned char addr[],
		 unsigned char addr_len,
		 unsigned char data[], unsigned char data_len)
{
	u32 val = 0, i = 0, result = I2C_OK;
	u32 i2c_cmd;
	ulong timebase;

#if 0
	/*The internel register offset from 1 to 7 bytes */
	if ((addr_len < 1) | (addr_len > 7)) {
		i2c_dbg("Accessing register valid\r\n");
		result = I2C_NOK;
		goto out;
	}

	/*Just support 1 byte slave addr ,
	   Max to 7 byte register
	   so the data_len <=120 byte
	 */
	if (data_len > 120) {
		i2c_dbg("Required too much data\r\n");
		result = I2C_NOK;
		goto out;
	}

	/* Check I2C bus idle */
	if (readl(&i2c->stat) & I2C_STAT_BBB) {
		result = I2C_NOK_TOUT;
		goto out;
	}
#endif

	switch (cmd_type) {
	case I2C_WRITE:
		/*1, enable i2c ,not enable interrupt*/
		i2c_writel((u32)&i2c->ctl, 0x80);
		/*2, write data count*/
		i2c_writel((u32)&i2c->datcnt, data_len);
		/*3, write slave addr*/
		i2c_writel((u32)&i2c->txdat, (chip << 1));
		/*4, write register addr*/
		for (i = 0; i < addr_len; i++)
			i2c_writel((u32)&i2c->txdat, addr[i]);
		/*5, write data*/
		for (i = 0; i < data_len; i++)
			i2c_writel((u32)&i2c->txdat, data[i]);
		/*6, write fifo command */
		i2c_cmd = I2C_CMD_X | I2C_CMD_AS((addr_len) + sizeof(chip));
		i2c_writel((u32)&i2c->cmd, i2c_cmd);
		i2c_readl((u32)&i2c->cmd);
		/* wait command complete */
		timebase = get_timer(0);
		while(1) {
			val = i2c_readl((u32)&i2c->fifostat);
			if(val & I2C_FIFOSTAT_RNB) {
				result = I2C_NACK;
				goto label_err;
			}
			if(val & I2C_FIFOSTAT_CECB)
				break;
			if(get_timer(timebase) > (500 * CONFIG_SYS_HZ / 1000)) {
				/* 500ms timeout */
				result = I2C_NOK_TOUT;
				goto label_err;
			}
		}
		result = I2C_OK;
		break;

	case I2C_READ:
		/*1, enable i2c ,not enable interrupt*/
		i2c_writel((u32)&i2c->ctl, 0x80);
		/*2, write data count*/
		i2c_writel((u32)&i2c->datcnt, data_len);
		/*3, write slave addr*/
		i2c_writel((u32)&i2c->txdat, (chip << 1));
		/*4, write register addr*/
		for (i = 0; i < addr_len; i++)
			i2c_writel((u32)&i2c->txdat, addr[i]);
		/*5, write slave addr | read_flag*/
		i2c_writel((u32)&i2c->txdat, (chip << 1) | I2C_READ);
		/*6, write fifo command */
		i2c_cmd = I2C_CMD_X | I2C_CMD_RBE | I2C_CMD_NS \
			| I2C_CMD_SAS(sizeof(chip)) |\
			I2C_CMD_AS(sizeof(chip) + addr_len);
		i2c_writel((u32)&i2c->cmd, i2c_cmd);

		/* wait command complete */
		timebase = get_timer(0);
		while(1) {
			val = i2c_readl((u32)&i2c->fifostat);
			if(val & I2C_FIFOSTAT_RNB) {
				result = I2C_NACK;
				goto label_err;
			}
			if(val & I2C_FIFOSTAT_CECB)
				break;
			if(get_timer(timebase) > (500 * CONFIG_SYS_HZ / 1000)) {
				/* 500ms timeout */
				result = I2C_NOK_TOUT;
				goto label_err;
			}
		}
		result = I2C_OK;

		/*8, Read data from rxdata*/
		for (i = 0; i < data_len; i++) {
			data[i] = readl(&i2c->rxdat);
			i2c_dbg("-->>Read data[%d] = 0x%02x\r\n", i, data[i]);
		}
		break;

	default:
		i2c_dbg("i2c_transfer: bad call\n");
		result = I2C_NOK;
		break;
	}

	return result;

	label_err:
	/* clear err bit */
	i2c_writel((u32)&i2c->fifostat, I2C_FIFOSTAT_RNB);
	/* reset fifo */
	i2c_writel((u32)&i2c->fifoctl, 0x06);
	while(i2c_readl((u32)&i2c->fifoctl) & 0x06);
	return result;
}


int i2c_probe(uchar chip)
{
#if 1
	struct gs5203_i2c *i2c;
	uint val, i2c_cmd;

	i2c = get_i2c_base();

	//change this into fifo mode to probe.
	// 1. enable sm
	i2c_writel((u32)&i2c->ctl, 0x80);	
	// 2. set datacnt
	i2c_writel((u32)&i2c->datcnt, 0);
	// 3. set device address
	i2c_writel((u32)&i2c->txdat, chip << 1);
	/* 4, write fifo command */
	i2c_cmd = I2C_CMD_EXEC | I2C_CMD_MSS | \
		I2C_CMD_SE | I2C_CMD_SAS(0) | I2C_CMD_AS(1) | I2C_CMD_SBE;
	i2c_writel((u32)&i2c->cmd, i2c_cmd);
	i2c_readl((u32)&i2c->cmd);

	while(((val = i2c_readl((u32)&i2c->fifostat)) & I2C_FIFOSTAT_CECB) == 0);

	if (val & I2C_FIFOSTAT_RNB) {
		/* clear err bit */
		i2c_writel((u32)&i2c->fifostat, I2C_FIFOSTAT_RNB);
		/* reset fifo */
		i2c_writel((u32)&i2c->fifoctl, 0x06);
		while(i2c_readl((u32)&i2c->fifoctl) & 0x06);
		return I2C_NACK;
	}
	return 0;

#else	
	struct gs5203_i2c *i2c;
	u32 val = 0;
	i2c = get_i2c_base();

	/*
	 * What is needed is to send the chip address and verify that the
	 * address was <ACK>ed (i.e. there was a chip at that address which
	 * drove the data line low).
	 */
	i2c_writel((u32)&i2c->cmd, 0);
	i2c_writel((u32)&i2c->txdat, chip << 1);
	i2c_writel((u32)&i2c->ctl, I2C_CTL_START_CMD & ~I2C_CTL_IRQE);
	i2c_readl((u32)&i2c->ctl);
	udelay(1000);
	while(((val = i2c_readl((u32)&i2c->stat)) & I2C_STAT_TCB) == 0);
	i2c_readl((u32)&i2c->stat);
	i2c_writel((u32)&i2c->ctl, I2C_CTL_STOP_CMD & ~I2C_CTL_IRQE);
	i2c_readl((u32)&i2c->stat);
	while((i2c_readl((u32)&i2c->stat) & (I2C_STAT_STPD|I2C_STAT_BBB)) != I2C_STAT_STPD);

	/* clear pending */
	i2c_writel((u32)&i2c->stat, I2C_STAT_IRQP);

	/* reset fifo */
	i2c_writel((u32)&i2c->fifoctl, 0x06);
	while(i2c_readl((u32)&i2c->fifoctl) & 0x06);

	if (val & I2C_STAT_RACK) {
		i2c_dbg("Chip is online\r\n");
		return 0;
	} else {
		i2c_dbg("Chip is offline\r\n");
		return -1;
	}
#endif
}

int i2c_read(uchar chip, uchar addr, u32 alen, uchar *buffer, uchar len)
{
	struct gs5203_i2c *i2c;
	int ret = 0;

	i2c = get_i2c_base();
	ret = i2c_transfer(i2c, I2C_READ, chip, &addr, alen, buffer, len);
	if (ret != 0) {
		debug("I2c read: failed %d\n", ret);
		return 1;
	}
	return 0;
}

int i2c_write(uchar chip, uchar addr, u32 alen, uchar *buffer, u32 len)
{
	struct gs5203_i2c *i2c;
	int ret = 0;

	i2c = get_i2c_base();
	ret = i2c_transfer(i2c, I2C_WRITE, chip, &addr, alen, buffer, len);
	if (ret != 0) {
		debug("I2c read: failed %d\n", ret);
		return 1;
	}
	return 0;
}

void acts_dump_i2c_register(void)
{
	struct gs5203_i2c *i2c;
	u32 addr = 0;
	int i = 0;
	u32 val = 0;

	i2c = get_i2c_base();
	addr = (u32)i2c;

	for (i = 0; i < 11; i++) {
		val = readl(addr++);
		printf("[0x%x] = 0x%x\r\n", addr, val);
	}
}

#ifdef CONFIG_OF_CONTROL

#define CONFIG_MAX_I2C_NUM	4

struct owl_i2c_bus {
	int node;	/* device tree node */
	int bus_num;	/* i2c bus number */
};

static int i2c_busses __attribute__((section(".data")));
static struct owl_i2c_bus i2c_bus[CONFIG_MAX_I2C_NUM]
			__attribute__((section(".data")));

void board_i2c_init(const void *blob)
{
	int node_list[CONFIG_MAX_I2C_NUM];
	int count, i;

	count = fdtdec_find_aliases_for_id(blob, "i2c",
		COMPAT_ACTIONS_OWL_I2C, node_list,
		CONFIG_MAX_I2C_NUM);

	for (i = 0; i < count; i++) {
		struct owl_i2c_bus *bus;
		int node = node_list[i];

		if (node <= 0)
			continue;
		bus = &i2c_bus[i];
		bus->node = node;
		bus->bus_num = i2c_busses++;
	}
}

static struct owl_i2c_bus *get_bus(unsigned int bus_idx)
{
	if (bus_idx < i2c_busses)
		return &i2c_bus[bus_idx];

	debug("Undefined bus: %d\n", bus_idx);
	return NULL;
}

int i2c_get_bus_num_fdt(int node)
{
	int i;

	for (i = 0; i < i2c_busses; i++) {
		if (node == i2c_bus[i].node)
			return i;
	}

	debug("%s: Can't find any matched I2C bus\n", __func__);
	return -1;
}

int i2c_reset_port_fdt(const void *blob, int node)
{
	struct owl_i2c_bus *i2c;
	int bus;

	bus = i2c_get_bus_num_fdt(node);
	if (bus < 0) {
		debug("could not get bus for node %d\n", node);
		return -1;
	}

	i2c = get_bus(bus);
	if (!i2c) {
		debug("get_bus() failed for node node %d\n", node);
		return -1;
	}

	//i2c_ch_init(i2c->regs, CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

	return 0;
}
#endif

