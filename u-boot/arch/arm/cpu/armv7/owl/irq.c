/*
 * (C) Copyright 2012
 * Actions Semi .Inc
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
 
#define N_IRQS				(21)
#define OWL_PERIPH_BASE (0xB0020000)

#define IRQ_ICCICR_ADDR		(OWL_PERIPH_BASE + 0x0100)
#define IRQ_ICCPMR_ADDR		(OWL_PERIPH_BASE + 0x0104)
#define IRQ_ICCIAD_ADDR		(OWL_PERIPH_BASE + 0x010C)
#define IRQ_ICCEOIR_ADDR	(OWL_PERIPH_BASE + 0x0110)
#define IRQ_ICDDCR_ADDR		(OWL_PERIPH_BASE + 0x1000)
#define IRQ_ICDISER_ADDR	(OWL_PERIPH_BASE + 0x1100)
#define IRQ_ICDICER_ADDR	(OWL_PERIPH_BASE + 0x1180)
#define IRQ_ICDIPR_ADDR		(OWL_PERIPH_BASE + 0x1400)
#define IRQ_ICDITARGET_ADDR	(OWL_PERIPH_BASE + 0x1800)

#define IRQ_TARGET_MASK		(0xFF << 0)
#define IRQ_TARGET_CPU0		(0x1 << 0)
#define IRQ_TARGET_CPU1		(0x1 << 1)
#define IRQ_TARGET_CPU2		(0x1 << 2)
#define IRQ_TARGET_CPU3		(0x1 << 3)

#define IRQ_ID_END		(N_IRQS)
struct _irq_handler {
	void *m_data;
	void (*m_func)(void *data);
	unsigned int count;
	unsigned int id;
};

static struct _irq_handler irq_handler[N_IRQS];

static u32 find_irq_id(int irq)
{
	u32 i;
	for (i = 0; i < N_IRQS; i++)
		if (irq == irq_handler[i].id)
			return i;
	return IRQ_ID_END;
}

static void default_isr(void *data)
{
	printf("default_isr():  called for IRQ %d, Interrupt Status=%x PR=%x\n",
	       (int)data, 0, 0);
}

static u32 alloc_irq_id(int irq, interrupt_handler_t handle_irq)
{
	u32 i;

	if (handle_irq == default_isr) {
		irq_handler[irq].id = 0;
		return irq;
	}

	for (i = 0; i < N_IRQS; i++) {
		if (!irq_handler[i].id) {
			irq_handler[i].id = irq;
			return i;
		}
	}
	return IRQ_ID_END;
}

/*  Read the Interrupt Acknowledge Register  (ICCIAR) */
static inline int next_irq(void)
{
	return readl(IRQ_ICCIAD_ADDR);
}

static inline void irq_enable_id(int id)
{
	u32 bit, offset;

	offset = ((id >> 5) << 2);
	bit = 0x1 << (id & 0x1F);
	writel(bit, IRQ_ICDISER_ADDR + offset);

	debug("bit(0x%x), off(0x%x) add(0x%x), val(0x%x)\n", bit, offset,
		IRQ_ICDISER_ADDR + offset,
		readl(IRQ_ICDISER_ADDR + offset));
}

static inline void irq_disable_id(int id)
{
	u32 bit, offset;

	offset = ((id >> 5) << 2);
	bit = 0x1 << (id & 0x1F);
	writel(bit, IRQ_ICDICER_ADDR + offset);
}

static inline void irq_set_priority(int id, int priority)
{
	int addr, shift, clr, set;

	shift = (id & 0x3) << 3;
	set = (priority & 0x1F) << (3 + shift);
	clr = 0xFF << shift;
	addr = (id & ~0x3) + IRQ_ICDIPR_ADDR;

	clrsetbits_le32(addr, clr, set);
}

static inline void irq_set_target(int id)
{
	int addr, shift, clr, set;

	shift = (id & 0x3) << 3;
	set = IRQ_TARGET_CPU0 << shift;
	clr = 0xFF << shift;
	addr = ((id >> 2) << 2) + IRQ_ICDITARGET_ADDR;

	debug("set target addr(0x%x), clr(0x%x), set(0x%x)\n",
			addr, clr, set);

	clrsetbits_le32(addr, clr, set);
}

/* Acknowlege the Interrupt, (ICCEOIR) */
static inline void irq_ack(int irq)
{
	writel(irq, IRQ_ICCEOIR_ADDR);
}

void do_irq(struct pt_regs *pt_regs)
{
	int irq = next_irq();

	u32 id = find_irq_id(irq);
	if (id < IRQ_ID_END && irq_handler[id].m_func) {
		irq_handler[id].m_func(irq_handler[id].m_data);
		irq_handler[id].count++;
		irq_ack(irq);
	}
}

void irq_install_handler(int irq, interrupt_handler_t handle_irq, void *data)
{
	u32 id;
	if (!handle_irq) {
		printf("error: no irq handle\n");
		return;
	}

	/* irq_handler[id].id will be set */
	id = alloc_irq_id(irq, handle_irq);
	if (id == IRQ_ID_END) {
		printf("error: no available irq id\n");
		return;
	}

	/* overwrite if default isr, fist init */
	irq_handler[id].m_data = data;
	irq_handler[id].m_func = handle_irq;
	irq_handler[id].count = 0;

	/* not default isr, so handle real hardware */
	if (handle_irq != default_isr) {
		irq_enable_id(irq);
		/* set priority */
		irq_set_priority(irq, 0x0);
		irq_set_target(irq);
	}
}

void irq_free_handler(int irq)
{
	u32 id = find_irq_id(irq);

	if (id == IRQ_ID_END) {
		printf("irq_free_handler: bad irq number %d\n", irq);
		return;
	}

	irq_disable_id(irq);

	irq_handler[id].m_data = NULL;
	irq_handler[id].m_func = NULL;
	irq_handler[id].count = 0;
	irq_handler[id].id = 0;
}


int arch_interrupt_init(void)
{
	uint i, reg_val;

	/* install default interrupt handlers */
	for (i = 0; i < N_IRQS; i++)
		irq_install_handler(i, default_isr, (void *)i);

	/* disable all src from forwarding */
	for(i=0; i<32; i++) {
		reg_val = readl(IRQ_ICDICER_ADDR + (i<<2));
		if(reg_val != 0) {
			writel(reg_val, IRQ_ICDICER_ADDR + (i<<2));
		}
	}

	/* enable global irq gic */
	setbits_le32(IRQ_ICDDCR_ADDR, 0x1);

	/* enable processor interface */
#define SECURE_BIT		(0x1 << 0)
#define NON_SECURE_BIT		(0x1 << 1)
	setbits_le32(IRQ_ICCICR_ADDR, SECURE_BIT | NON_SECURE_BIT);
	writel(0x1F, IRQ_ICCPMR_ADDR);

	return 0;
}

int irq_printf(const char *fmt, ...)
{
       va_list args;
       uint i;
       char printbuffer[64];

       va_start(args, fmt);

       /* For this to work, printbuffer must be larger than
        * anything we ever want to print.
        */
       i = vscnprintf(printbuffer, sizeof(printbuffer), fmt, args);
       va_end(args);

       /* Print the string */
       serial_puts(printbuffer);
       return i;
}


#if defined(CONFIG_CMD_IRQ)
int do_irqinfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i;

	printf("\nInterrupt-Information:\n\n");
	printf("Nr  Routine   Arg       Count\n");
	printf("-----------------------------\n");

	for (i = 0; i < N_IRQS; i++) {
		if (irq_handler[i].m_func) {
			printf("%02d  %08lx  %08lx  %d\n",
				i,
				(ulong)irq_handler[i].m_func,
				(ulong)irq_handler[i].m_data,
				irq_handler[i].count);
		}
	}
	printf("\n");

	return 0;
}
#endif
