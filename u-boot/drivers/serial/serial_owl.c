/*
 * Copyright (C) 2014 Actions Semi Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <serial.h>
#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/owl_afi.h>
#include <asm/arch/owl_io.h>
#include <asm/arch/owl_clk.h>


DECLARE_GLOBAL_DATA_PTR;



#define OWL_MAX_UART			(7)
#define UART_REG_CONFIG_NUM		(3)

/* UART registers */
#define UART_CTL			(0x0000)
#define UART_RXDAT			(0x0004)
#define UART_TXDAT			(0x0008)
#define UART_STAT			(0x000C)

#define UART_STAT_TFES          (0x1 << 10)
#define UART_STAT_TFFU			(0x1 << 6)
#define UART_STAT_RFEM			(0x1 << 5)

struct uart_reg_config
{
	unsigned int reg;
	unsigned int mask;
	unsigned int val;
};

struct owl_uart_dev
{
	unsigned char chan;
	unsigned char mfp;
	unsigned char clk_id;
	unsigned char rst_id;

	unsigned long base;
	unsigned long uart_clk;
	struct uart_reg_config reg_cfg[UART_REG_CONFIG_NUM];
};

static struct owl_uart_dev owl_uart_ports[] = {
	/*uart0*/
	{0, 0, DEVCLKEN_UART0, CMU_RST_UART0, UART0_BASE, CMU_UART0CLK,
		{{MFP_CTL2, 0x7, 0x0},{MFP_CTL3, (0x7 << 19), 0x0}, },},
	{0, 1, DEVCLKEN_UART0, CMU_RST_UART0, UART0_BASE, CMU_UART0CLK,
		{{MFP_CTL2, 0x3 << 22, 0x3 << 22},},},
	/*uart 1*/
	{1, 0, DEVCLKEN_UART1, CMU_RST_UART1, UART1_BASE, CMU_UART1CLK,
		{{MFP_CTL2, 0x7 << 11, 0x5 << 11},}},
	{1, 1, DEVCLKEN_UART1, CMU_RST_UART1, UART1_BASE, CMU_UART1CLK,
		{{MFP_CTL3, 0x7 << 16, 0x3 << 16},}},
	/*uart 2*/
	{2, 0, DEVCLKEN_UART2, CMU_RST_UART2, UART2_BASE, CMU_UART2CLK,
		{{MFP_CTL0, (0x7 << 13), (0x1 << 13)},},},
	{2, 1, DEVCLKEN_UART2, CMU_RST_UART2, UART2_BASE, CMU_UART2CLK,
		{{MFP_CTL0, (0x7 << 8), (0x1 << 8)}, },},
	{2, 2, DEVCLKEN_UART2, CMU_RST_UART2, UART2_BASE, CMU_UART2CLK,
		{{MFP_CTL1, (0x3 << 19), (0x1 << 19)},{MFP_CTL2, (0x7 << 17), (0x1 << 17)}, },},	
	{2, 3, DEVCLKEN_UART2, CMU_RST_UART2, UART2_BASE, CMU_UART2CLK,
		{{MFP_CTL1, (0x7 << 7), (0x3 << 7)},{MFP_CTL2, (0x7 << 24), (0x3 << 24)}, },},	
	{2, 4, DEVCLKEN_UART2, CMU_RST_UART2, UART2_BASE, CMU_UART2CLK,
		{{MFP_CTL2, (0x3 << 22), (0x0 << 22)}, },},
	{2, 5, DEVCLKEN_UART2, CMU_RST_UART2, UART2_BASE, CMU_UART2CLK,
		{{MFP_CTL2, (0x7 << 17), (0x4 << 17)},{MFP_CTL2, (0x7 << 14), (0x7 << 14)}, },},
	{2, 6, DEVCLKEN_UART2, CMU_RST_UART2, UART2_BASE, CMU_UART2CLK,
		{{MFP_CTL1, (0x7 << 7), (0x3 << 7)},{MFP_CTL2, (0x7 << 24), (0x3 << 24)}, },},
	{2, 7, DEVCLKEN_UART2, CMU_RST_UART2, UART2_BASE, CMU_UART2CLK,
		{{MFP_CTL2, (0x7 << 11), (0x4 << 11)},{MFP_CTL2, (0x7 << 0), (0x1 << 0)}, },},
	{2, 8, DEVCLKEN_UART2, CMU_RST_UART2, UART2_BASE, CMU_UART2CLK,
		{{MFP_CTL3, (0x7 << 7), (0x1 << 19)},{MFP_CTL3, (0x7 << 16), (0x1 << 16)}, },},	
	{2, 9, DEVCLKEN_UART2, CMU_RST_UART2, UART2_BASE, CMU_UART2CLK,
		{{0, 0, 0},{0, 0, 0}, },},				
	/*uart 3*/
	{3, 0, DEVCLKEN_UART3, CMU_RST_UART3, UART3_BASE, CMU_UART3CLK,},
	{3, 1, DEVCLKEN_UART3, CMU_RST_UART3, UART3_BASE, CMU_UART3CLK,
		{{0, 0, 0},{0, 0, 0}, },},	
	/*uart 4*/
	{4, 0, DEVCLKEN_UART4, CMU_RST_UART4, UART4_BASE, CMU_UART4CLK,
		{{MFP_CTL0, (0x3 << 20)|(0x3 << 6), (0x3 << 20)|(0x1 << 6) },},},
	{4, 1, DEVCLKEN_UART4, CMU_RST_UART4, UART4_BASE, CMU_UART4CLK,
		{{MFP_CTL0, (0x3 << 11)|(0x3 << 6), (0x3 << 11)|(0x1 << 6) },},},
		
	{5, 0, DEVCLKEN_UART5, CMU_RST_UART5, UART5_BASE, CMU_UART5CLK,
		{{MFP_CTL0, (0x7 << 8), 4 << 8},},},
	{5, 1, DEVCLKEN_UART5, CMU_RST_UART5, UART5_BASE, CMU_UART5CLK,
		{{MFP_CTL1, (0x3 << 26), 1 << 26},{MFP_CTL1, (0x7 << 23), 4 << 23},},},
	{5, 2, DEVCLKEN_UART5, CMU_RST_UART5, UART5_BASE, CMU_UART5CLK,
		{{MFP_CTL2, (0x3 << 20), 3 << 20},},},
	{5, 3, DEVCLKEN_UART5, CMU_RST_UART5, UART5_BASE, CMU_UART5CLK,
		{{MFP_CTL2, (0x7 << 17)|(0x7 << 14), (0x5 << 17)|(0x5 << 14)},},},
	{6, 0, DEVCLKEN_UART6, CMU_RST_UART6, UART6_BASE, CMU_UART6CLK,
		{{MFP_CTL0, (0x7 << 16), 3 << 16},},},
	{6, 1, DEVCLKEN_UART6, CMU_RST_UART6, UART6_BASE, CMU_UART6CLK,
		{{MFP_CTL3, (0xffff << 4), 0xa << 16},},},
};
#define MAX_NUM_UART (sizeof(owl_uart_ports) / sizeof(owl_uart_ports[0]))


static struct owl_uart_dev * cur_uart_dev = NULL;


static struct owl_uart_dev * match_uart_port(int id, int mfp)
{
	struct owl_uart_dev *dev;
	int i, num;
	num = MAX_NUM_UART;
	for (i = 0; i < num; i++) {
		dev = &owl_uart_ports[i];
		if (dev->chan == id && dev->mfp == mfp) {
			/* found */
			return dev;
		}
	}
	return NULL;
}

static int owl_serial_set_baudrate(struct owl_uart_dev * dev, unsigned int baud)
{
	unsigned int div;

	if (baud == 0)
		return -1;
		
	div = (24 * 1000000) / (8 * baud);
	if (div > 0)
		div--;

	writel(div, dev->uart_clk);

	return 0;
}

static int init_uart_port(struct owl_uart_dev *dev, unsigned int baud)
{
	struct uart_reg_config *cfg;
	int i;

	/* init mfp config */
	for (i = 0; i < UART_REG_CONFIG_NUM; i++) {
		cfg = &dev->reg_cfg[i];

		if (cfg->mask != 0)
			owl_clrsetbits(cfg->reg, cfg->mask, cfg->val);
	}

	writel(0x8003, dev->base + UART_CTL);

	owl_serial_set_baudrate(dev, baud);

	return 0;
}

static struct owl_uart_dev * owl_serial_init(int id, int baud, int mfp)
{
	struct owl_uart_dev *dev ;
	
	dev = match_uart_port(id, mfp);
	if (dev == NULL)
		return NULL;

	owl_clk_enable(dev->clk_id);
	owl_reset(dev->rst_id);

	init_uart_port(dev, baud);

	return dev;
}

static int owl_serial_putc(struct owl_uart_dev *dev, char c)
{
	/* wait TX FIFO not full */
	while (readl(dev->base + UART_STAT) & UART_STAT_TFFU)
		;
	writel(c, dev->base + UART_TXDAT);

	/*  drain out of TX FIFO */
	while (!(readl(dev->base + UART_STAT) & UART_STAT_TFES))
		;

	return 0;
}

static int owl_serial_tstc(struct owl_uart_dev *dev)
{
	return !(readl(dev->base + UART_STAT) & UART_STAT_RFEM);
}

static int owl_serial_getc(struct owl_uart_dev *dev)
{
	return readl(dev->base + UART_RXDAT);
}

int owl_serial_getc_wait(struct owl_uart_dev *dev)
{

	/* wait RX FIFO not empty */
	while (owl_serial_tstc(dev))
		;

	return owl_serial_getc(dev);
}


/*------------------------------------------------------------------
 * UART the serial port
 *-----------------------------------------------------------------*/
static void owl_uboot_serial_setbrg(void)
{
	int buad, index, mfp;
	index = owl_afi_get_serial_number();
	buad = owl_afi_get_serial_baudrate();
	mfp =owl_afi_get_serial_pad();
	cur_uart_dev = owl_serial_init(index, buad, mfp);
}


static int owl_uboot_serial_init(void)
{
	owl_uboot_serial_setbrg();
	return 0;
}

/*-----------------------------------------------------------------------
 * UART CONSOLE
 *---------------------------------------------------------------------*/
static void owl_uboot_serial_putc(char c)
{
	if ( cur_uart_dev == NULL)
		return;
	
	if (c == '\n')
		owl_serial_putc (cur_uart_dev, '\r');

	owl_serial_putc(cur_uart_dev, c);

}

static int owl_uboot_serial_tstc(void)
{
	if ( cur_uart_dev == NULL)
		return -1;
	return owl_serial_tstc(cur_uart_dev);

}

static int owl_uboot_serial_getc(void)
{
	if ( cur_uart_dev == NULL)
		return -1;
	return owl_serial_getc(cur_uart_dev);
}

static struct serial_device owl_uboot_serial_drv = {
	.name	= "owl_serial",
	.start	= owl_uboot_serial_init,
	.stop	= NULL,
	.setbrg	= owl_uboot_serial_setbrg,
	.putc	= owl_uboot_serial_putc,
	.puts	= default_serial_puts,
	.getc	= owl_uboot_serial_getc,
	.tstc	= owl_uboot_serial_tstc,
};

void owl_serial_initialize(void)
{
	serial_register(&owl_uboot_serial_drv);
}

__weak struct serial_device *default_serial_console(void)
{
	return &owl_uboot_serial_drv;
}
