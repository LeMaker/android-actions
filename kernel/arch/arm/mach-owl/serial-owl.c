/*
 * arch/arm/mach-owl/serial-owl.c
 *
 * serial driver for Actions SOC
 *
 * Copyright 2013 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
 
#if defined(CONFIG_SERIAL_OWL_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kref.h>

#include <linux/pinctrl/consumer.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dma-direction.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/hdmac-owl.h>
#include <mach/clkname.h>
#include <mach/module-owl.h>

#define OWL_MAX_UART		7/*updated from 6 to 7*/
#define OWL_SERIAL_MAJOR	204
#define OWL_SERIAL_MINOR	0

#define UART_TO_OWL_UART_PORT(uart_port) ((struct owl_uart_port *) uart_port)

#define UART_CTL		0x0000
#define UART_RXDAT		0x0004
#define UART_TXDAT		0x0008
#define UART_STAT		0x000C

/* UART0_CTL */
/*bit 31~23 reserved*/
#define UART_CTL_DTCR		(0x1 << 22)	/* DMA TX counter reset */
#define UART_CTL_DRCR		(0x1 << 21)	/* DMA RX counter reset */
#define UART_CTL_LBEN		(0x1 << 20)	/* Loop Back Enable */
#define UART_CTL_TXIE		(0x1 << 19)	/* TX IRQ Enable */
#define UART_CTL_RXIE		(0x1 << 18)	/* RX IRQ Enable */
#define UART_CTL_TXDE		(0x1 << 17)	/* TX DRQ Enable */
#define UART_CTL_RXDE		(0x1 << 16)	/* RX DRQ Enable */
#define UART_CTL_EN			(0x1 << 15)	/* UART0 Enable */
#define UART_CTL_TRFS		(0x1 << 14)	/* UART0 TX/RX FIFO Enable */
#define	 UART_CTL_TRFS_RX	(0x0 << 14)	/* select RX FIFO */
#define	 UART_CTL_TRFS_TX	(0x1 << 14)	/* select TX FIFO */
/*bit 13 reserved*/
#define UART_CTL_AFE		(0x1 << 12)	/* Autoflow Enable */
/*bit 11~7 reserved*/
#define UART_CTL_PRS_MASK	(0x7 << 4)
#define UART_CTL_PRS(x)		(((x) & 0x7) << 4)
#define	 UART_CTL_PRS_NONE	UART_CTL_PRS(0)
#define	 UART_CTL_PRS_ODD	UART_CTL_PRS(4)
#define	 UART_CTL_PRS_MARK	UART_CTL_PRS(5)
#define	 UART_CTL_PRS_EVEN	UART_CTL_PRS(6)
#define	 UART_CTL_PRS_SPACE	UART_CTL_PRS(7)
/*bit 3 reserved*/
#define UART_CTL_STPS			(0x1 << 2)
#define	 UART_CTL_STPS_1BITS	(0x0 << 2)
#define	 UART_CTL_STPS_2BITS	(0x1 << 2)
#define UART_CTL_DWLS_MASK		(0x3 << 0)
#define UART_CTL_DWLS(x)		(((x) & 0x3) << 0)
#define	 UART_CTL_DWLS_5BITS	UART_CTL_DWLS(0)
#define	 UART_CTL_DWLS_6BITS	UART_CTL_DWLS(1)
#define	 UART_CTL_DWLS_7BITS	UART_CTL_DWLS(2)
#define	 UART_CTL_DWLS_8BITS	UART_CTL_DWLS(3)

/******************************************************************************/
/* UART0_RXDAT */
/*bit 31~8 reserved*/
#define UART_RXDAT_MASK		(0xFF << 0)	  /* Received Data */

/******************************************************************************/
/* UART0_TXDAT */
/*bit 31~8 reserved*/
#define UART_TXDAT_MASK		(0xFF << 0)	  /* Sending Data*/

/******************************************************************************/
/* UART_STAT */
/*bit 31~17 reserved*/
#define UART_STAT_UTBB		(0x1 << 16)	/* UART0 TX busy bit */
#define UART_STAT_TRFL_MASK	(0x1F << 11)	/* TX/RX FIFO Level */
#define UART_STAT_TRFL_SET(x)	(((x) & 0x1F) << 11)
#define UART_STAT_TFES		(0x1 << 10)	/* TX FIFO Empty Status */
#define UART_STAT_RFFS		(0x1 << 9)	/* RX FIFO full Status */
#define UART_STAT_RTSS		(0x1 << 8)	/* RTS status */
#define UART_STAT_CTSS		(0x1 << 7)	/* CTS status */
#define UART_STAT_TFFU		(0x1 << 6)	/* TX FIFO full Status */
#define UART_STAT_RFEM		(0x1 << 5)	/* RX FIFO Empty Status */
#define UART_STAT_RXST		(0x1 << 4)	/* Receive Status */
#define UART_STAT_TFER		(0x1 << 3)	/* TX FIFO Erro */
#define UART_STAT_RXER		(0x1 << 2)	/* RX FIFO Erro */
#define UART_STAT_TIP		(0x1 << 1)	/* TX IRQ Pending Bit */
#define UART_STAT_RIP		(0x1 << 0)	/* RX IRQ Pending Bit */

#define RX_FIFO_DEPTH  32
/******************************************************************************/
/*DMA*/
#define OWL_UART_RX_DMA_BUFFER_SIZE (4096*4)
#define DMA_RX_FLUSH_JIFFIES (HZ/50)
#define STOP_TIMEOUT 1000000

struct owl_uart_port {
	struct uart_port uart;
	char name[16];
	struct clk *clk;
	unsigned int saved_ctl;

	/*dma*/
	bool enable_dma_rx;
	bool enable_dma_tx;
	struct dma_chan *rx_dma_chan;
	struct dma_chan *tx_dma_chan;
	dma_addr_t rx_dma_buf_phys;
	dma_addr_t tx_dma_buf_phys;
	unsigned char *rx_dma_buf_virt;
	unsigned char *tx_dma_buf_virt;
	struct dma_async_tx_descriptor *rx_dma_desc;
	struct dma_async_tx_descriptor *tx_dma_desc;
	dma_cookie_t rx_cookie;
	dma_cookie_t tx_cookie;

	struct timer_list rx_dma_timer;
	unsigned int dma_buf_tail;

	int tx_dma_in_progress;
	unsigned int tx_bytes;
	int status;
};

struct owl_serial_state {
	struct pinctrl *p;
	int refcount;
};

static struct owl_uart_port owl_uart_ports[OWL_MAX_UART];
static struct owl_serial_state *sdio_serial_state;
static struct platform_device *g_pdev;
static int owl_console_port = OWL_MAX_UART;

static int owl_uart_start_dma(struct owl_uart_port *owl_uart_port, bool dma_to_memory);

static inline struct uart_port *get_port_from_line(unsigned int line)
{
	return &owl_uart_ports[line].uart;
}

static inline void owl_write(struct uart_port *port, unsigned int val, unsigned int off)
{
	__raw_writel(val, port->membase + off);
}

static inline unsigned int owl_read(struct uart_port *port, unsigned int off)
{
	return __raw_readl(port->membase + off);
}

static void owl_uart_next_tx(struct owl_uart_port *owl_uart_port)
{
	struct uart_port *uport = &(owl_uart_port->uart);
	struct circ_buf *xmit = &uport->state->xmit;

	owl_uart_port->tx_bytes = CIRC_CNT_TO_END(xmit->head, xmit->tail,
			UART_XMIT_SIZE);
	if (!owl_uart_port->tx_bytes)
		return;

	owl_uart_start_dma(owl_uart_port, false);
}

/* Start transmitting */
static void owl_start_tx(struct uart_port *port)
{
	unsigned int data;
	struct owl_uart_port *owl_uart_port = UART_TO_OWL_UART_PORT(port);

	if (owl_uart_port->enable_dma_tx) {
		if (owl_uart_port->tx_dma_in_progress)
			return;

		owl_uart_next_tx(owl_uart_port);
	} else{
		data = owl_read(port, UART_STAT);
		data |= UART_STAT_TIP;
		owl_write(port, data, UART_STAT);

		data = owl_read(port, UART_CTL);
		data |= UART_CTL_TXIE;
		owl_write(port, data, UART_CTL);
	}
}

/*
 * Stop transmitting.
 */
static void owl_stop_tx(struct uart_port *port)
{
	struct owl_uart_port *owl_uart_port = UART_TO_OWL_UART_PORT(port);
	unsigned int data;

	owl_uart_port->tx_dma_in_progress = 0;
	if (owl_uart_port->enable_dma_tx)
		dmaengine_terminate_all(owl_uart_port->tx_dma_chan);

	data = owl_read(port, UART_CTL);
	data &= ~(UART_CTL_TXIE | UART_CTL_TXDE);
	owl_write(port, data, UART_CTL);

	data = owl_read(port, UART_STAT);
	data |= UART_STAT_TIP;
	owl_write(port, data, UART_STAT);
}

/*
 * Stop receiving - port is in process of being closed.
 */
static void owl_stop_rx(struct uart_port *port)
{
	struct owl_uart_port *owl_uart_port = UART_TO_OWL_UART_PORT(port);
	unsigned int data;

	if(port->line == owl_console_port)
		return;
	del_timer_sync(&(owl_uart_port->rx_dma_timer));

	if (owl_uart_port->enable_dma_rx)
		dmaengine_terminate_all(owl_uart_port->rx_dma_chan);

	owl_uart_port->dma_buf_tail = 0;
	data = owl_read(port, UART_CTL);
	data &= ~(UART_CTL_RXIE | UART_CTL_RXDE);
	owl_write(port, data, UART_CTL);

	data = owl_read(port, UART_STAT);
	data |= UART_STAT_RIP;
	owl_write(port, data, UART_STAT);
}

/*
 * Enable modem status interrupts
 */
static void owl_enable_ms(struct uart_port *port)
{
}

static int owl_is_break_button_down(char c)
{
	static char breakbuf[] = {0x02, 0x12, 0x05, 0x01, 0x0b};	//ctrl + "break"
	static int index = 0;
	
	if(c == breakbuf[index])
	{
		index++;
		if(index == sizeof(breakbuf))
		{
			index = 0;
			return 1;
		}
	}
	else
		index = 0;
	
	return 0;
}

#ifdef CONFIG_ARM_OWL_CPUFREQ
void owl_cpufreq_frequency_lock(void);
void owl_cpufreq_frequency_unlock(void);
#endif

static int owl_uart_handle_sysrq_char(struct uart_port *port, char c)
{
	if(port->line != owl_console_port)
		return 0;

	if(owl_is_break_button_down(c))
		uart_handle_break(port);
	else if(port->sysrq)
	{
		int console_loglevel_save = console_loglevel;
		
		spin_unlock(&port->lock);
#ifdef CONFIG_ARM_OWL_CPUFREQ
		owl_cpufreq_frequency_lock();
#endif

		console_loglevel = 7;
		uart_handle_sysrq_char(port, c);
		console_loglevel = console_loglevel_save;
		
#ifdef CONFIG_ARM_OWL_CPUFREQ
		owl_cpufreq_frequency_unlock();
#endif
		spin_lock(&port->lock);
		return 1;
	}
	
	return 0;
}
/*
 * Characters received (called from interrupt handler)
 */
static void handle_rx(struct uart_port *port)
{
	unsigned int stat, data;

	/* select RX FIFO */
	data = owl_read(port, UART_CTL);
	data &= ~UART_CTL_TRFS;
	data |= UART_CTL_TRFS_RX;
	owl_write(port, data, UART_CTL);

	/* and now the main RX loop */
	while (!((stat = owl_read(port, UART_STAT)) & UART_STAT_RFEM)) {
		unsigned int c;
		char flag = TTY_NORMAL;

		c = owl_read(port, UART_RXDAT);

		if (stat & UART_STAT_RXER)
			port->icount.overrun++;

		if (stat & UART_STAT_RXST) {
			/* we are not able to distinguish the error type */
			port->icount.brk++;
			port->icount.frame++;

			/* Mask conditions we're ignorning. */
			stat &= port->read_status_mask;
			if (stat & UART_STAT_RXST)
				flag = TTY_PARITY;
		} else
			port->icount.rx++;
		
		if(owl_uart_handle_sysrq_char(port, c))
			continue;

		uart_insert_char(port, stat, stat & UART_STAT_RXER, c, flag);
	}

	tty_flip_buffer_push(&port->state->port);
}

/*
 * transmit interrupt handler
 */
static void handle_tx(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	unsigned int data;

	if (port->x_char) {
		/* wait TX FIFO not full */
		while (owl_read(port, UART_STAT) & UART_STAT_TFFU)
			;
		owl_write(port, port->x_char, UART_TXDAT);
		port->icount.tx++;
		port->x_char = 0;
	}

	/* select TX FIFO */
	data = owl_read(port, UART_CTL);
	data &= ~UART_CTL_TRFS;
	data |= UART_CTL_TRFS_TX;
	owl_write(port, data, UART_CTL);

	while (!(owl_read(port, UART_STAT) & UART_STAT_TFFU)) {
		if (uart_circ_empty(xmit))
			break;

		owl_write(port, xmit->buf[xmit->tail], UART_TXDAT);

		/* wait FIFO empty? */
/*		while (owl_read(port, UART_STAT) & UART_STAT_TRFL_MASK)
			;
*/
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		owl_stop_tx(port);
}

/*
 * Interrupt handler
 */
static irqreturn_t owl_uart_irq(int irq, void *dev_id)
{
	unsigned long flags;
	struct uart_port *port = dev_id;
	unsigned int stat;

	spin_lock_irqsave(&port->lock, flags);
	stat = owl_read(port, UART_STAT);

	/*when using DMA, handle_rx will never be called*/
	if (stat & UART_STAT_RIP)
		handle_rx(port);

	if (stat & UART_STAT_TIP)
		handle_tx(port);

	stat = owl_read(port, UART_STAT);
	stat |= UART_STAT_RIP | UART_STAT_TIP;
	owl_write(port, stat, UART_STAT);

	spin_unlock_irqrestore(&port->lock, flags);

	return IRQ_HANDLED;
}

/*
 * Return TIOCSER_TEMT when transmitter FIFO and Shift register is empty.
 */
static unsigned int owl_tx_empty(struct uart_port *port)
{
	unsigned int data, ret;
	unsigned long flags = 0;

	spin_lock_irqsave(&port->lock, flags);
	/* select TX FIFO */
	data = owl_read(port, UART_CTL);
	data &= ~UART_CTL_TRFS;
	data |= UART_CTL_TRFS_TX;
	owl_write(port, data, UART_CTL);

	/* check FIFO level */
	data = owl_read(port, UART_STAT);
	ret = (data & UART_STAT_TRFL_MASK) ? 0 : TIOCSER_TEMT;

	spin_unlock_irqrestore(&port->lock, flags);

	return ret;
}

/*modem control is not implemented yet*/
static unsigned int owl_get_mctrl(struct uart_port *port)
{
	return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
}

static void owl_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
}

static void owl_break_ctl(struct uart_port *port, int break_ctl)
{
}

static int owl_set_baud_rate(struct uart_port *port, unsigned int baud)
{
	struct owl_uart_port *owl_uart_port = UART_TO_OWL_UART_PORT(port);
	unsigned int target_freq, lookup_freq, freq;
	int ret;

	if (!owl_uart_port->clk) {
		printk("ERROR: uart%d did not get clk\n", port->line);
		return -1;
	}

	target_freq = baud * 8;
	lookup_freq = clk_round_rate(owl_uart_port->clk, target_freq);

#if 1
	/*provide an accuracy < 1%*/
	lookup_freq = target_freq + target_freq/128;
	freq = clk_round_rate(owl_uart_port->clk, lookup_freq);
	while (freq > target_freq) {
		lookup_freq = freq;
		freq = clk_round_rate(owl_uart_port->clk, lookup_freq - 1);
	}
#endif

	ret = clk_set_rate(owl_uart_port->clk, lookup_freq);
	if (ret < 0) {
		dev_err(port->dev, "clk_set_rate() failed for rate %u\n",
				lookup_freq);
		return ret;
	}

	return 0;
}

/*
 * RX DMA callback function -- do noting
 * note:it will be called by dma_terminate_all()
 */
static void owl_uart_rx_dma_callback(void *param)
{
	return;
}

static void owl_uart_rx_to_tty(struct owl_uart_port *owl_uart_port,
		unsigned int tail, unsigned int count)
{
	struct uart_port *uport = &(owl_uart_port->uart);
	int copied;
	unsigned int status;

	status = owl_read(uport, UART_STAT);
	if (status & (0x1 << 4 | 0x1 << 2)) {
		pr_err("\n\nRX FIFO error, UART_STAT = 0x%x\n\n", status);
		owl_dma_dump_all(owl_uart_port->rx_dma_chan);
	}

	uport->icount.rx += count;

	dma_sync_single_for_cpu(uport->dev, owl_uart_port->rx_dma_buf_phys,
			OWL_UART_RX_DMA_BUFFER_SIZE, DMA_FROM_DEVICE);

	copied = tty_insert_flip_string(&owl_uart_port->uart.state->port,
			(unsigned char *)(owl_uart_port->rx_dma_buf_virt + tail),
			count);
	if (copied != count)
		pr_err("rx_dma_callback:RxData copy to tty layer failed\n");

	dma_sync_single_for_device(uport->dev, owl_uart_port->rx_dma_buf_phys,
			OWL_UART_RX_DMA_BUFFER_SIZE, DMA_FROM_DEVICE);
	tty_flip_buffer_push(&owl_uart_port->uart.state->port);
}

static void owl_uart_rx_dma_timeout(struct owl_uart_port *owl_uart_port)
{
	struct uart_port *uport = &(owl_uart_port->uart);
	unsigned int dma_remain = 0, count;
	unsigned int head, tail = owl_uart_port->dma_buf_tail;
	unsigned long flags;

	spin_lock_irqsave(&uport->lock, flags);

	dma_remain = read_remain_cnt(owl_uart_port->rx_dma_chan);
	head = OWL_UART_RX_DMA_BUFFER_SIZE - dma_remain;
	if (head < tail) {
		count = OWL_UART_RX_DMA_BUFFER_SIZE - tail;
		owl_uart_rx_to_tty(owl_uart_port, tail, count);
		tail = 0;
	}

	count = head - tail;
	if (count > 0)
		owl_uart_rx_to_tty(owl_uart_port, tail, count);

	owl_uart_port->dma_buf_tail = head;

	spin_unlock_irqrestore(&uport->lock, flags);

	mod_timer(&(owl_uart_port->rx_dma_timer), jiffies + DMA_RX_FLUSH_JIFFIES);
}

/*TX DMA callback function*/
static void owl_uart_tx_dma_callback(void *param)
{
	struct owl_uart_port *owl_uart_port = (struct owl_uart_port *)param;
	struct uart_port *uport = &(owl_uart_port->uart);
	struct circ_buf *xmit = &uport->state->xmit;
	unsigned long flags, timeout = STOP_TIMEOUT;
	unsigned int status;

	if (0 == owl_uart_port->tx_dma_in_progress)
		return;

	spin_lock_irqsave(&uport->lock, flags);

	/*
	 * we must wait until the TX FIFO is empty before
	 * we start next dma transfer, otherwise we will
	 * lost some bytes.
	 */
	while (owl_read(uport, UART_STAT) & UART_STAT_TRFL_MASK) {
		udelay(10);
		timeout--;
		if (0 == timeout) {
			pr_err("TX DMA timeout...");
			return;
		}
	}

	status = owl_read(uport, UART_STAT);
	if (status & UART_STAT_TFER) {
		pr_err("\n\nTX FIFO error, UART_STAT = 0x%x\n\n", status);
		owl_dma_dump_all(owl_uart_port->tx_dma_chan);
	}

	dma_sync_single_for_cpu(owl_uart_port->uart.dev,
			owl_uart_port->tx_dma_buf_phys,
			UART_XMIT_SIZE, DMA_TO_DEVICE);

	xmit->tail += owl_uart_port->tx_bytes;
	xmit->tail &= UART_XMIT_SIZE - 1;
	uport->icount.tx += owl_uart_port->tx_bytes;

	if (uart_circ_empty(xmit))
		owl_uart_port->tx_dma_in_progress = 0;
	else
		owl_uart_next_tx(owl_uart_port);

	spin_unlock_irqrestore(&uport->lock, flags);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(uport);
}

static unsigned int owl_uart_dma_mode_config(unsigned int line,
		bool dma_to_memory)
{
	unsigned int mode = 0;

	if (dma_to_memory)
		mode = PRIORITY_ZERO | SRC_CONSTANT |
			DST_INCR | DST_DCU |
			SRC_DEV | (0x10+line*2+1) | BUS_WIDTH_8BIT;
	else
		mode = PRIORITY_ZERO | SRC_INCR |
			DST_CONSTANT | DST_DEV |
			SRC_DCU | (0x10+line*2) | BUS_WIDTH_8BIT;

	return mode;
}

static int owl_uart_dma_channel_allocate(struct owl_uart_port *owl_uart_port,
		bool dma_to_memory)
{
	struct dma_chan *dma_chan;
	struct owl_dma_slave *acts_slave;
	dma_cap_mask_t mask;
	unsigned char *dma_buf;
	dma_addr_t dma_phys;
	struct dma_slave_config *dma_sconf;
	int ret;
	struct uart_port *uport = &(owl_uart_port->uart);

	/*request dma channel*/
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_chan = dma_request_channel(mask, NULL, NULL);
	if (!dma_chan) {
		dev_err(uport->dev, "Failed to request DMA channel\n");
		return -ENODEV;
	}

	/*do DMA mapping*/
	/*ALERT: use dma_alloc_coherent() will not work!*/
	if (dma_to_memory) { /*RX*/
		dma_buf = kzalloc(OWL_UART_RX_DMA_BUFFER_SIZE, GFP_KERNEL);
		if (!dma_buf) {
			dev_err(uport->dev, "Not able to allocate the dma buffer for RX\n");
			dma_release_channel(dma_chan);
			return -ENOMEM;
		}
		dma_phys = dma_map_single(uport->dev, dma_buf,
				OWL_UART_RX_DMA_BUFFER_SIZE, DMA_FROM_DEVICE);

	} else{ /*TX*/
		dma_buf = uport->state->xmit.buf;
		dma_phys = dma_map_single(uport->dev, dma_buf, UART_XMIT_SIZE,
				DMA_TO_DEVICE);
	}

	/*alloc and fill struct dma_slave_config*/
	dma_sconf = devm_kzalloc(uport->dev, sizeof(*dma_sconf), GFP_KERNEL);
	if (!dma_sconf) {
		dev_err(uport->dev, "Not able to allocate struct dma_slave_config\n");
		return -ENOMEM;
	}
	if (dma_to_memory)
		dma_sconf->src_addr = (unsigned int)uport->mapbase + UART_RXDAT;
	else
		dma_sconf->dst_addr = (unsigned int)uport->mapbase + UART_TXDAT;

	/*alloc and fill struct owl_dma_slave*/
	acts_slave = devm_kzalloc(uport->dev, sizeof(*acts_slave), GFP_KERNEL);
	if (!acts_slave) {
		dev_err(uport->dev, "Not able to alloc struct owl_dma_slave\n");
		return -ENOMEM;
	}
	acts_slave->dma_dev = dma_chan->device->dev;
	acts_slave->trans_type = SLAVE;
	acts_slave->mode = owl_uart_dma_mode_config(uport->line,
			dma_to_memory);

	if (dma_to_memory) {
		owl_uart_port->rx_dma_chan = dma_chan;
		owl_uart_port->rx_dma_buf_virt = dma_buf;
		owl_uart_port->rx_dma_buf_phys = dma_phys;
		owl_uart_port->rx_dma_chan->private = acts_slave;
	} else{
		owl_uart_port->tx_dma_chan = dma_chan;
		owl_uart_port->tx_dma_buf_virt = dma_buf;
		owl_uart_port->tx_dma_buf_phys = dma_phys;
		owl_uart_port->tx_dma_chan->private = acts_slave;
	}

	/*finish DMA config*/
	ret = dmaengine_slave_config(dma_chan, dma_sconf);
	if (ret < 0) {
		dev_err(uport->dev, "DMA slave config failed, err = %d\n", ret);
		dma_release_channel(dma_chan);
		return ret;
	}

	return 0;
}

static void owl_uart_dma_channel_free(struct owl_uart_port *owl_uart_port,
		bool dma_to_memory)
{
	struct dma_chan *dma_chan;
	struct uart_port *uport = &(owl_uart_port->uart);

	if (dma_to_memory) {
		dma_unmap_single(uport->dev, owl_uart_port->rx_dma_buf_phys,
				OWL_UART_RX_DMA_BUFFER_SIZE, DMA_FROM_DEVICE);
		kfree(owl_uart_port->rx_dma_buf_virt);
		dma_chan = owl_uart_port->rx_dma_chan;
		owl_uart_port->rx_dma_chan = NULL;
		owl_uart_port->rx_dma_buf_phys = 0;
		owl_uart_port->rx_dma_buf_virt = NULL;
	} else {
		dma_unmap_single(uport->dev, owl_uart_port->tx_dma_buf_phys,
				UART_XMIT_SIZE, DMA_TO_DEVICE);
		dma_chan = owl_uart_port->tx_dma_chan;
		owl_uart_port->tx_dma_chan = NULL;
		owl_uart_port->tx_dma_buf_phys = 0;
		owl_uart_port->tx_dma_buf_virt = NULL;
	}

	dma_release_channel(dma_chan);
}

static void owl_uart_module_enable(unsigned int line)
{
	switch (line) {
	case 0:
		module_clk_enable(MOD_ID_UART0);
		break;

	case 1:
		module_clk_enable(MOD_ID_UART1);
		break;

	case 2:
		module_clk_enable(MOD_ID_UART2);
		break;

	case 3:
		module_clk_enable(MOD_ID_UART3);
		break;

	case 4:
		module_clk_enable(MOD_ID_UART4);
		break;

	case 5:
		module_clk_enable(MOD_ID_UART5);
		break;

	case 6:
		module_clk_enable(MOD_ID_UART6);
		break;

	default:
		break;
	}
}

static void owl_uart_module_disable(unsigned int line)
{
	switch (line) {
	case 0:
		module_clk_disable(MOD_ID_UART0);
		break;

	case 1:
		module_clk_disable(MOD_ID_UART1);
		break;

	case 2:
		module_clk_disable(MOD_ID_UART2);
		break;

	case 3:
		module_clk_disable(MOD_ID_UART3);
		break;

	case 4:
		module_clk_disable(MOD_ID_UART4);
		break;

	case 5:
		module_clk_disable(MOD_ID_UART5);
		break;

	case 6:
		module_clk_disable(MOD_ID_UART6);
		break;

	default:
		break;
	}
}

static void owl_uart_hw_init(struct owl_uart_port *owl_uart_port)
{
	struct uart_port *uport = &(owl_uart_port->uart);
	unsigned int data;
	unsigned long flags;

	spin_lock_irqsave(&uport->lock, flags);
	
	data = owl_read(uport, UART_STAT);
	/* clear IRQ pending and reset RX/TX FIFO*/
	data |= UART_STAT_RIP | UART_STAT_TIP |
		UART_STAT_RXER | UART_STAT_TFER | UART_STAT_RXST;
	owl_write(uport, data, UART_STAT);

	data = owl_read(uport, UART_CTL);
	if (owl_uart_port->enable_dma_rx)
		/*enable DRQ for RX*/
		data |= UART_CTL_DRCR | UART_CTL_RXDE;
	else
		data |= UART_CTL_RXIE;

	if (owl_uart_port->enable_dma_tx)
		/*enable DRQ for TX*/
		data |= UART_CTL_DTCR | UART_CTL_TXDE;
	else
		data |= UART_CTL_TXIE;

	/* enable module*/
	data |= UART_CTL_EN;
	owl_write(uport, data, UART_CTL);
	
	spin_unlock_irqrestore(&uport->lock, flags);
	/*no need to set trigger level*/
}

static int owl_uart_start_dma(struct owl_uart_port *owl_uart_port, bool dma_to_memory)
{
	struct uart_port *uport = &(owl_uart_port->uart);
	struct circ_buf *xmit = &uport->state->xmit;

	if (dma_to_memory) {
		owl_uart_port->rx_dma_desc = dmaengine_prep_dma_cyclic(
				owl_uart_port->rx_dma_chan,
				owl_uart_port->rx_dma_buf_phys,
				OWL_UART_RX_DMA_BUFFER_SIZE,
				OWL_UART_RX_DMA_BUFFER_SIZE,
				DMA_DEV_TO_MEM, 0);
		if (!owl_uart_port->rx_dma_desc) {
			dev_err(uport->dev, "Not able to get desc for RX\n");
			return -EIO;
		}

		/*set callback function*/
		owl_uart_port->rx_dma_desc->callback = owl_uart_rx_dma_callback;
		owl_uart_port->rx_dma_desc->callback_param = owl_uart_port;

		/*submit request*/
		owl_uart_port->rx_cookie = dmaengine_submit(owl_uart_port->rx_dma_desc);

		dma_sync_single_for_device(owl_uart_port->uart.dev,
				owl_uart_port->rx_dma_buf_phys,
				OWL_UART_RX_DMA_BUFFER_SIZE, DMA_FROM_DEVICE);
		/*activate transactions in the pending queue*/
		dma_async_issue_pending(owl_uart_port->rx_dma_chan);

		/*init timer*/
		owl_uart_port->rx_dma_timer.data = (unsigned long)owl_uart_port;
		owl_uart_port->rx_dma_timer.function =
			(void *)owl_uart_rx_dma_timeout;
		owl_uart_port->rx_dma_timer.expires =
			jiffies + DMA_RX_FLUSH_JIFFIES;
		add_timer(&(owl_uart_port->rx_dma_timer));

	} else{ /*TX*/
		dma_sync_single_for_device(owl_uart_port->uart.dev,
				owl_uart_port->tx_dma_buf_phys,
				UART_XMIT_SIZE, DMA_TO_DEVICE);

		owl_uart_port->tx_dma_in_progress = 1;

		owl_uart_port->tx_dma_desc = dmaengine_prep_slave_single(
				owl_uart_port->tx_dma_chan,
				owl_uart_port->tx_dma_buf_phys + xmit->tail,
				owl_uart_port->tx_bytes, DMA_MEM_TO_DEV, 0);
		if (!owl_uart_port->tx_dma_desc) {
			dev_err(uport->dev, "Not able to get desc for TX\n");
			return -EIO;
		}

		owl_uart_port->tx_dma_desc->callback = owl_uart_tx_dma_callback;
		owl_uart_port->tx_dma_desc->callback_param = owl_uart_port;

		owl_uart_port->tx_cookie = dmaengine_submit(owl_uart_port->tx_dma_desc);
		/*
		 * do not issue pending according to
		 * our dmaengine implementation
		 */
		/*dma_async_issue_pending(owl_uart_port->tx_dma_chan);*/
	}
	return 0;
}


static int keepuartonflag;
static int __init keepuarton_setup(char *__str)
{
	keepuartonflag = 1;
	return 1;
}
__setup("keepuarton", keepuarton_setup);

/*
 * Disable the port
 */
static void __owl_uart_shutdown(struct uart_port *port)
{
	struct owl_uart_port *owl_uart_port = UART_TO_OWL_UART_PORT(port);
	unsigned int data;
	unsigned long flags;

	if(owl_uart_port->status == 0)
		return;
		
	spin_lock_irqsave(&port->lock, flags);
	
	owl_uart_port->status = 0;
		
	/* disable module/IRQs/DRQs */
	data = owl_read(port, UART_CTL);
	data &= ~(UART_CTL_RXIE | UART_CTL_TXIE
			| UART_CTL_RXDE | UART_CTL_TXDE);
	if (1 != keepuartonflag)
		data &= ~UART_CTL_EN;

	owl_write(port, data, UART_CTL);
	
	spin_unlock_irqrestore(&port->lock, flags);

	if (owl_uart_port->enable_dma_rx)
		owl_uart_dma_channel_free(owl_uart_port, true);

	if (owl_uart_port->enable_dma_tx)
		owl_uart_dma_channel_free(owl_uart_port, false);

	if (!owl_uart_port->enable_dma_rx || !owl_uart_port->enable_dma_tx)
		free_irq(port->irq, port);
		
}
 
static void owl_uart_shutdown(struct uart_port *port)
{
	if(port->line == owl_console_port)	//console always on
		return;

	__owl_uart_shutdown(port);
}

/*
 * use DMA for RX and TX if we set "enable-dma-xx" in dts
 */
static int owl_uart_dma_startup(struct uart_port *uport)
{
	int ret;
	struct owl_uart_port *owl_uart_port = UART_TO_OWL_UART_PORT(uport);

	if(uport->line == owl_console_port)		//console only closed when another open happened
		__owl_uart_shutdown(uport);
		
	if(owl_uart_port->status == 1)
		return 0;

	if (owl_uart_port->enable_dma_rx) {
		ret = owl_uart_dma_channel_allocate(owl_uart_port, true);/*RX*/
		if (ret < 0) {
			dev_err(uport->dev,
					"UART Rx Dma allocation failed, "
					"err = %d\n", ret);
			return ret;
		}
	}

	if (owl_uart_port->enable_dma_tx) {
		ret = owl_uart_dma_channel_allocate(owl_uart_port, false);/*TX*/
		if (ret < 0) {
			dev_err(uport->dev,
					"UART Tx Dma allocation failed, "
					"err = %d\n", ret);
			if (owl_uart_port->enable_dma_rx)
				owl_uart_dma_channel_free(owl_uart_port, true);
			else
				return ret;
		}
	}

	if (!owl_uart_port->enable_dma_rx || !owl_uart_port->enable_dma_tx) {
		ret = request_irq(uport->irq, owl_uart_irq, IRQF_TRIGGER_HIGH,
				owl_uart_port->name, uport);
		if (ret) {
			if (owl_uart_port->enable_dma_rx)
				owl_uart_dma_channel_free(owl_uart_port, true);
			else if (owl_uart_port->enable_dma_tx)
				owl_uart_dma_channel_free(owl_uart_port, false);
			else
				return ret;

			return ret;
		}
	}

	owl_uart_hw_init(owl_uart_port);

	/*start RX DMA*/
	if (owl_uart_port->enable_dma_rx) {
		ret = owl_uart_start_dma(owl_uart_port, true);
		if (ret < 0)
			dev_err(uport->dev,
					"RX DMA start failed, err = %d\n", ret);
	}

	pr_info("open ttyS%x %s %s\n", uport->line,
			owl_uart_port->enable_dma_rx ? "DMA RX" : " ",
			owl_uart_port->enable_dma_tx ? "DMA TX" : " ");

	owl_uart_port->status = 1;
	
	return 0;
}

/*
 * Change the port parameters
 */
static void owl_set_termios(struct uart_port *port, struct ktermios *termios,
		struct ktermios *old)
{
	unsigned long flags;
	unsigned int ctl, baud;
	int ret;

	/*For BT, the max bit rate is 3M*/
	baud = uart_get_baud_rate(port, termios, old, 0, 3200000);

	/*it's not able to set baudrate on fpga*/
	ret = owl_set_baud_rate(port, baud);
	if (ret < 0) {
		printk("owl_set_baud_rate failed\n");
		return;
	}

	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);

	spin_lock_irqsave(&port->lock, flags);

	/* We don't support modem control lines. */
	termios->c_cflag &= ~(HUPCL | CMSPAR);
	termios->c_cflag |= CLOCAL;

	/* We don't support BREAK character recognition. */
	termios->c_iflag &= ~(IGNBRK | BRKINT);

	ctl = owl_read(port, UART_CTL);
	ctl &= ~(UART_CTL_DWLS_MASK | UART_CTL_STPS
			| UART_CTL_PRS_MASK | UART_CTL_AFE);

	/* byte size */
	ctl &= ~UART_CTL_DWLS_MASK;
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		ctl |= UART_CTL_DWLS(0);
		break;
	case CS6:
		ctl |= UART_CTL_DWLS(1);
		break;
	case CS7:
		ctl |= UART_CTL_DWLS(2);
		break;
	case CS8:
	default:
		ctl |= UART_CTL_DWLS(3);
		break;
	}

	/* stop bits */
	if (termios->c_cflag & CSTOPB)
		ctl |= UART_CTL_STPS_2BITS;
	else
		ctl |= UART_CTL_STPS_1BITS;

	/* parity */
	if (termios->c_cflag & PARENB) {
		/* Mark or Space parity */
		if (termios->c_cflag & CMSPAR) {
			if (termios->c_cflag & PARODD)
				ctl |= UART_CTL_PRS_MARK;
			else
				ctl |= UART_CTL_PRS_SPACE;
		} else if (termios->c_cflag & PARODD)
			ctl |= UART_CTL_PRS_ODD;
		else
			ctl |= UART_CTL_PRS_EVEN;
	} else
		ctl |= UART_CTL_PRS_NONE;

	/* Only uart2/3 support RTS/CTS Automatic Hardware Flow Control. */
	if ((termios->c_cflag & CRTSCTS) &&
			(2 == port->line || 3 == port->line))
		ctl |= UART_CTL_AFE;

	owl_write(port, ctl, UART_CTL);

	/* Configure status bits to ignore based on termio flags. */

	/*
	 * Normally we need to mask the bits we do care about
	 * as there is no hardware support for
	 * (termios->c_iflag & INPACK/BRKINT/PARMRK)
	 * and it seems the interrupt happened only for tx/rx
	 * we do nothing about the port.read_status_mask
	 */
	port->read_status_mask |= UART_STAT_RXER;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= UART_STAT_RXST;

	/* update the per-port timeout */
	uart_update_timeout(port, termios->c_cflag, baud);

	spin_unlock_irqrestore(&port->lock, flags);
}

/*
 * Return string describing the specified port
 */
static const char *owl_uart_type(struct uart_port *port)
{
	return "OWL_SERIAL";
}

/*
 * Release the memory region(s) being used by 'port'.
 */
static void owl_release_port(struct uart_port *port)
{
	struct platform_device *pdev = to_platform_device(port->dev);
	struct resource *resource;
	resource_size_t size;

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!resource))
		return;
	size = resource->end - resource->start + 1;

	release_mem_region(port->mapbase, size);

	if (port->flags & UPF_IOREMAP) {
		iounmap(port->membase);
		port->membase = NULL;
	}
}

/*
 * Request the memory region(s) being used by 'port'.
 */
static int owl_request_port(struct uart_port *port)
{
	struct platform_device *pdev = to_platform_device(port->dev);
	struct resource *resource;
	resource_size_t size;

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!resource))
		return -ENXIO;
	size = resource->end - resource->start + 1;

	if (!request_mem_region(port->mapbase, size, "owl_serial"))
		return -EBUSY;

	if (port->flags & UPF_IOREMAP) {
		port->membase = ioremap(port->mapbase, size);
		if (port->membase == NULL) {
			release_mem_region(port->mapbase, size);
			return -ENOMEM;
		}
	}

	return 0;
}


/*
 * Configure/autoconfigure the port.
 */
static void owl_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_OWL;
		owl_request_port(port);
	}
}

static int owl_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	if (unlikely(ser->type != PORT_UNKNOWN && ser->type != PORT_OWL))
		return -EINVAL;
	if (unlikely(port->irq != ser->irq))
		return -EINVAL;
	if ((void *)port->membase != ser->iomem_base)
		return -EINVAL;
	return 0;
}

#ifdef CONFIG_CONSOLE_POLL
static int owl_poll_get_char(struct uart_port *port)
{
	unsigned int old_ctl, data;
	unsigned long flags;
	unsigned int ch = NO_POLL_CHAR;

	spin_lock_irqsave(&port->lock, flags);

	/* backup old control register */
	old_ctl = owl_read(port, UART_CTL);

	/* select RX FIFO */
	data = old_ctl & (~(UART_CTL_TRFS));
	data = data | UART_CTL_TRFS_RX;

	/* disable IRQ */
	/*
	   data &= ~(UART_CTL_TXIE | UART_CTL_RXIE);
	   */
	owl_write(port, data, UART_CTL);

	/* wait RX FIFO not emtpy */
	do {
		cpu_relax();
		/* Get the interrupts */
		data = owl_read(port, UART_STAT);
	} while ((data & UART_STAT_RIP) == 0);

	while (!(data & UART_STAT_RFEM)) {
		ch = owl_read(port, UART_RXDAT);
		data = owl_read(port, UART_STAT);
	}

	/* clear IRQ pending */
	data = owl_read(port, UART_STAT);
	/*
	   data |= UART_STAT_TIP | UART_STAT_RIP;
	   */
	data |= UART_STAT_RIP;
	owl_write(port, data, UART_STAT);

	/* restore old ctl */
	owl_write(port, old_ctl, UART_CTL);

	spin_unlock_irqrestore(&port->lock, flags);

	return ch;
}

static void owl_poll_put_char(struct uart_port *port, unsigned char ch)
{
	unsigned int old_ctl, data;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	/* backup old control register */
	old_ctl = owl_read(port, UART_CTL);

	/* select TX FIFO */
	data = old_ctl & (~(UART_CTL_TRFS));
	data = data | UART_CTL_TRFS_TX;

	/* disable IRQ */
	data &= ~(UART_CTL_TXIE | UART_CTL_RXIE);
	owl_write(port, data, UART_CTL);

	/* wait TX FIFO not full */
	while (owl_read(port, UART_STAT) & UART_STAT_TFFU)
		cpu_relax();

	owl_write(port, ch, UART_TXDAT);

	/* wait until all content have been sent out
	 * TODO:
	 */
	while (owl_read(port, UART_STAT) & UART_STAT_TRFL_MASK)
		cpu_relax();

	/* clear IRQ pending */
	data = owl_read(port, UART_STAT);
	/*
	   data |= UART_STAT_TIP | UART_STAT_RIP;
	   */
	data |= UART_STAT_TIP;
	owl_write(port, data, UART_STAT);
	data = owl_read(port, UART_STAT);

	/* restore old ctl */
	owl_write(port, old_ctl, UART_CTL);

	spin_unlock_irqrestore(&port->lock, flags);
	return;
}
#endif


/*
 * Power / Clock management.
 */
static void owl_uart_pm(struct uart_port *port, unsigned int state,
			    unsigned int oldstate)
{
	switch (state) {
	case 0:
		/*
		 * Enable clock for this serial port.
		 * This is called on uart_open() or a resume event.
		 */
		owl_uart_module_enable(port->line);
		break;
	case 3:
		/*
		 * Disable clock for this serial port.
		 * This is called on uart_close() or a suspend event.
		 */
		owl_uart_module_disable(port->line);
		break;
	default:
		pr_err("owl_serial: unknown pm state: %d\n", state);
	}
}

static struct uart_ops owl_uart_pops = {
	.tx_empty = owl_tx_empty,
	.set_mctrl = owl_set_mctrl,
	.get_mctrl = owl_get_mctrl,
	.stop_tx = owl_stop_tx,
	.start_tx = owl_start_tx,
	.stop_rx = owl_stop_rx,
	.enable_ms = owl_enable_ms,
	.break_ctl = owl_break_ctl,
	.startup = owl_uart_dma_startup,
	.shutdown = owl_uart_shutdown,
	.set_termios = owl_set_termios,
	.type = owl_uart_type,
	.release_port = owl_release_port,
	.request_port = owl_request_port,
	.config_port = owl_config_port,
	.verify_port = owl_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char  = owl_poll_get_char,
	.poll_put_char  = owl_poll_put_char,
#endif
	.pm = owl_uart_pm,
};

/*
 * Configure the port from the platform device resource info.
 */
static int owl_init_uport(struct uart_port *uport,
		struct platform_device *pdev)
{
	struct owl_uart_port *owl_uart_port = UART_TO_OWL_UART_PORT(uport);
	struct resource *resource;

	uport->type		= PORT_OWL;
	uport->iotype   = UPIO_MEM;
	uport->ops      = &owl_uart_pops;
	uport->fifosize = 16;
	uport->dev      = &pdev->dev;

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!resource)) {
		dev_err(&pdev->dev, "No IO memory resource\n");
		return -ENXIO;
	}

	uport->mapbase = resource->start;
	uport->membase = (void __iomem *)IO_ADDRESS(uport->mapbase);
	if (!uport->membase) {
		dev_err(&pdev->dev, "memregion/iomap address req failed\n");
		return -EADDRNOTAVAIL;
	}

	/*devm_clk_get() is not implemented yet in 3.4*/
	/*    owl_uart_port->clk = devm_clk_get(&pdev->dev, NULL);*/
	switch (uport->line) {
	case 0:
		owl_uart_port->clk = clk_get(NULL, CLKNAME_UART0_CLK);
		break;
	case 1:
		owl_uart_port->clk = clk_get(NULL, CLKNAME_UART1_CLK);
		break;
	case 2:
		owl_uart_port->clk = clk_get(NULL, CLKNAME_UART2_CLK);
		break;
	case 3:
		owl_uart_port->clk = clk_get(NULL, CLKNAME_UART3_CLK);
		break;
	case 4:
		owl_uart_port->clk = clk_get(NULL, CLKNAME_UART4_CLK);
		break;
	case 5:
		owl_uart_port->clk = clk_get(NULL, CLKNAME_UART5_CLK);
		break;
	case 6:
		owl_uart_port->clk = clk_get(NULL, CLKNAME_UART6_CLK);
	default:
		break;
	}

	if (IS_ERR(owl_uart_port->clk)) {
		printk(KERN_ERR "%s(): cannot get clk for port %d\n",
				__func__, pdev->id);
		owl_uart_port->clk = NULL;
		return -1;
	}

	uport->irq = platform_get_irq(pdev, 0);
	if (unlikely(uport->irq < 0))
		return -ENXIO;

	init_timer(&(owl_uart_port->rx_dma_timer));

	return 0;
}

#ifdef CONFIG_SERIAL_OWL_CONSOLE
static void owl_console_putchar(struct uart_port *port, int c)
{
	while (owl_read(port, UART_STAT) & UART_STAT_TFFU)
		cpu_relax();
	owl_write(port, c, UART_TXDAT);
}

static void owl_console_write(struct console *co, const char *s,
		unsigned int count)
{
	struct uart_port *port;
	unsigned long flags;
	unsigned int old_ctl, data;

	BUG_ON(co->index < 0 || co->index >= OWL_MAX_UART);

	port = get_port_from_line(co->index);

	spin_lock_irqsave(&port->lock, flags);

	/* backup old control register */
	old_ctl = owl_read(port, UART_CTL);

	/* disable IRQ */
	data = old_ctl | UART_CTL_TRFS_TX;
	data &= ~(UART_CTL_TXIE | UART_CTL_RXIE);
	owl_write(port, data, UART_CTL);

	uart_console_write(port, s, count, owl_console_putchar);

	/* wait until all content have been sent out */
	while (owl_read(port, UART_STAT) & UART_STAT_TRFL_MASK)
		;

	/* clear IRQ pending */
	data = owl_read(port, UART_STAT);
	data |= UART_STAT_TIP | UART_STAT_RIP;
	owl_write(port, data, UART_STAT);

	/* restore old ctl */
	owl_write(port, old_ctl, UART_CTL);

	spin_unlock_irqrestore(&port->lock, flags);
}

static int __init owl_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (unlikely(co->index >= OWL_MAX_UART || co->index < 0))
		return -ENXIO;

	port = get_port_from_line(co->index);

	if (unlikely(!port->membase))
		return -ENXIO;

	port->cons = co;
    
	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

#ifdef OWL_CONSOLE_KEEP_ON
	owl_console_port = port->line;
	printk(KERN_INFO "owl_serial: console setup on port #%d\n",
			owl_console_port);
#endif

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver owl_uart_driver;

static struct console owl_console = {
	.name = "ttyS",
	.write = owl_console_write,
	.device = uart_console_device,
	.setup = owl_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &owl_uart_driver,
};
/*
   static int __init owl_uart_console_init(void)
   {
   register_console(&owl_console);
   return 0;
   }
   console_initcall(owl_uart_console_init);
   */
#define OWL_CONSOLE	(&owl_console)
#else
#define OWL_CONSOLE	NULL
#endif

static struct uart_driver owl_uart_driver = {
	.owner = THIS_MODULE,
	.driver_name = "owl_serial",
	.dev_name = "ttyS",
	.nr = OWL_MAX_UART,
	.cons = OWL_CONSOLE,
	.major = OWL_SERIAL_MAJOR,
	.minor = OWL_SERIAL_MINOR,
};

static struct of_device_id owl_uart_of_match[] = {
	{.compatible = "actions,owl-uart",},
	{},
};
MODULE_DEVICE_TABLE(of, owl_uart_of_match);

static struct uart_port *owl_serial_parse_dt(struct platform_device *pdev)
{
	struct owl_uart_port *owl_uart_port;
	struct uart_port *uport;
	struct device_node *np = pdev->dev.of_node;
	int line;

	if (!of_device_is_compatible(np, "actions,owl-uart")) {
		printk("owl_uart: no device is found\n");
		return NULL;
	}

	line = of_alias_get_id(np, "serial");
	if (line < 0) {
		printk("failed to get alias id, err = %d\n", line);
		return NULL;
	}

	owl_uart_port = &owl_uart_ports[line];
	owl_uart_port->uart.line = line;

	owl_uart_port->enable_dma_rx = of_property_read_bool(np,
			"actions,enable-dma-rx");
	owl_uart_port->enable_dma_tx = of_property_read_bool(np,
			"actions,enable-dma-tx");

	uport = &(owl_uart_port->uart);
	return uport;
}


int uart_pinctrl_request(struct owl_serial_state *state,
		struct platform_device *pdev)
{
	state->p = pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(state->p)) {
		printk("failed to get pinctrl\n");
		return PTR_ERR(state->p);
	}
	state->refcount++;
	return 0;
}

int sdio_uart_pinctrl_request(void)
{
	return uart_pinctrl_request(sdio_serial_state, g_pdev);
}
EXPORT_SYMBOL(sdio_uart_pinctrl_request);

void sdio_uart_pinctrl_free(void)
{
	if(sdio_serial_state->refcount > 0) {
		sdio_serial_state->refcount--;
		pinctrl_put(sdio_serial_state->p);
	}
}
EXPORT_SYMBOL(sdio_uart_pinctrl_free);

static int owl_serial_pinctrl(struct platform_device *pdev)
{
	struct owl_serial_state *serial_state;
	int ret;

	if (of_find_property(pdev->dev.of_node, "sdio_uart_supported", NULL)) {
		g_pdev = pdev;

		sdio_serial_state = kzalloc(sizeof(struct owl_serial_state),
				GFP_KERNEL);
		if (!sdio_serial_state) {
			printk("failed to alloc owl_serial_state\n");
			return -ENOMEM;
		}

		//platform_driver_probe已经调了一次pinctrl_get，这里调用仅是获取pinctrl的指针
		ret = uart_pinctrl_request(sdio_serial_state, g_pdev);
		if (ret < 0) {
			printk("failed to request sdio uart pinctrl\n");
			kfree(sdio_serial_state);
			return -ENOMEM;
		}
		//这里主动释放掉多余的一次引用
		pinctrl_put(sdio_serial_state->p);
	} else {
		serial_state =
			kzalloc(sizeof(struct owl_serial_state), GFP_KERNEL);
		if (!serial_state) {
			printk("failed to alloc owl_serial_state\n");
			return -ENOMEM;
		}

		//platform_driver_probe已经调了一次pinctrl_get，这里调用仅是判断是否能正确获取pin
		ret = uart_pinctrl_request(serial_state, pdev);
		if (ret < 0) {
			kfree(serial_state);
			return -ENOMEM;
		}
		//这里主动调用pinctrl_put释放掉多余的一次引用
		pinctrl_put(serial_state->p);
		kfree(serial_state);
	}

	return 0;
}

static int __init owl_serial_probe(struct platform_device *pdev)
{
	struct uart_port *uport;
	int ret;

	uport = owl_serial_parse_dt(pdev);
	if (NULL == uport)
		return -ENODEV;

	/*pinctrl may be fialed, but will not stop the probe*/
	ret = owl_serial_pinctrl(pdev);
	if (ret < 0)
		printk("failed to set serial pin mux\n");

	ret = owl_init_uport(uport, pdev);
	if (ret) {
		printk("owl_init_uport failed\n");
		return ret;
	}

	platform_set_drvdata(pdev, uport);

	ret = uart_add_one_port(&owl_uart_driver, uport);
	if (ret) {
		printk("failed to add uart port, err = %d\n", ret);
		return ret;
	}

	return 0;
}

static int owl_serial_remove(struct platform_device *pdev)
{
	struct uart_port *port = platform_get_drvdata(pdev);
	int ret = 0;

	device_init_wakeup(&pdev->dev, 0);
	platform_set_drvdata(pdev, NULL);

	ret = uart_remove_one_port(&owl_uart_driver, port);

	/* "port" is allocated statically, so we shouldn't free it */

	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int owl_uart_suspend(struct device *dev)
{
	struct owl_uart_port *owl_uart_port = dev_get_drvdata(dev);
	struct uart_port *uport = &owl_uart_port->uart;

	if(uport->line == owl_console_port)	//console always on
		return 0;

	disable_irq(owl_uart_port->uart.irq);
	return uart_suspend_port(&owl_uart_driver, uport);
}

static int owl_uart_resume(struct device *dev)
{
	struct owl_uart_port *owl_uart_port = dev_get_drvdata(dev);
	struct uart_port *uport = &owl_uart_port->uart;

	if(uport->line == owl_console_port)	//console always on
		return 0;

	enable_irq(owl_uart_port->uart.irq);
	return uart_resume_port(&owl_uart_driver, uport);
}
#endif

static const struct dev_pm_ops owl_uart_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(owl_uart_suspend, owl_uart_resume)
};

static struct platform_driver owl_platform_driver = {
	.remove = owl_serial_remove,
	.driver = {
		.name   = "owl-serial",
		.owner  = THIS_MODULE,
		.of_match_table = owl_uart_of_match,
		.pm = &owl_uart_pm_ops,
	},
};

static int __init owl_serial_init(void)
{
	int ret;

	ret = uart_register_driver(&owl_uart_driver);
	if (unlikely(ret)) {
		printk("Could not register %s driver\n",
				owl_uart_driver.driver_name);
		return ret;
	}

	ret = platform_driver_probe(&owl_platform_driver, owl_serial_probe);
	if (unlikely(ret)) {
		printk("Uart platform driver register failed, err=%d\n",
				ret);
		uart_unregister_driver(&owl_uart_driver);
		return ret;
	}

	return 0;
}

static void __exit owl_serial_exit(void)
{
	platform_driver_unregister(&owl_platform_driver);
	uart_unregister_driver(&owl_uart_driver);
}

module_init(owl_serial_init);
module_exit(owl_serial_exit);

MODULE_AUTHOR("Actions Semi Inc.");
MODULE_DESCRIPTION("serial driver for Actions SOC");
MODULE_LICENSE("GPL");
