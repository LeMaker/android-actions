/*
 * board-owl.c  --  Board support file for Actions gs703a
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/memblock.h>
#include <linux/spi/spi.h>
//#include <linux/ion.h>
#include <linux/pinctrl/machine.h>
#include <linux/i2c.h>
#include <linux/of_platform.h>
#include <linux/highmem.h>
#include <asm/system_info.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/hdmac-owl.h>
#include <mach/gpio.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
//#include <asm/hardware/gic.h>
#include <asm/setup.h>

#include "board-owl.h"

void owl_switch_jtag(void)
{
    act_writel(act_readl(MFP_CTL1) & (~((0x7<<29) | (0x7<<26))), MFP_CTL1);
    act_writel((act_readl(MFP_CTL2) & (~((0x3<<5) | (0x3<<7) | (0x7<<11) | (0x7<<17))))
        | ((0x2<<5) | (0x3<<7) | (0x3<<11) | (0x3<<17)), MFP_CTL2);
}


#ifndef CONFIG_OF
static struct resource owl_res_ethernet[] = {
	{
		.start = OWL_IRQ_ETHERNET,
		.end = OWL_IRQ_ETHERNET,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device owl_ethernet_device = {
	.name = "owl-ethernet",
	.id = 0,
	.resource = owl_res_ethernet,
	.num_resources = ARRAY_SIZE(owl_res_ethernet),
};
#endif

static struct resource owl_res_uart0[] = {
	{
		.start = UART0_BASE,
		.end = UART0_BASE + 0xfff,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = OWL_IRQ_UART0,
		.end = OWL_IRQ_UART0,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device owl_uart_device0 = {
	.name = "owl-serial",
	.id = 0,
	.resource = owl_res_uart0,
	.num_resources = ARRAY_SIZE(owl_res_uart0),
};



/************************************************/

/************************************************/

/************************/
static struct platform_device owl_vout_device = {
	.name   = "gl5201_vout",
	.id		= 0,
};
/************************/


static struct platform_device *owl_platform_devices[] __initdata = {
#ifndef CONFIG_OF
	&owl_ethernet_device,
#endif
	&owl_uart_device0,
	&owl_vout_device,
};

#ifdef CONFIG_OF
static struct of_device_id owl_dt_match_table[] __initdata = {
	{ .compatible = "simple-bus", },
	{}
};
#endif /* CONFIG_OF */

static void __init owl_board_init(void)
{
	int ret;

	ret = platform_add_devices(owl_platform_devices,
		ARRAY_SIZE(owl_platform_devices));
	if (ret)
		pr_warn("platform_add_devices() fail\n");

#ifdef CONFIG_OF
	ret = of_platform_populate(NULL, owl_dt_match_table, NULL, NULL);
	if (ret)
		pr_warn("of_platform_populate() fail\n");
#endif

	pr_info("%s()\n", __func__);
}

#ifdef CONFIG_OF
static const char *owl_dt_match[] __initconst = {
	"actions,atm7039c",
	"actions,atm7059tc",
	"actions,atm7059a",
	NULL,
};
#endif

extern void owl_powergate_earlyinit(void);

void __init owl_check_revision(void)
{
	char *vddr = kmap_atomic(pfn_to_page(PFN_DOWN(0)));
	memcpy(&system_serial_low, vddr+0x800, sizeof(system_serial_low));
	memcpy(&system_serial_high, vddr+0x804, sizeof(system_serial_high));
	kunmap_atomic(vddr);
}

void __init owl_init_early(void)
{
	owl_check_revision();

	owl_powergate_earlyinit();
	
	owl_init_clocks();
}


extern int owl_cpu_disable(unsigned int cpu);
extern void owl_cpu_die(unsigned int cpu);
extern int owl_cpu_kill(unsigned int cpu);
extern void owl_smp_prepare_cpus(unsigned int max_cpus);
extern void owl_smp_init_cpus(void);
extern int owl_boot_secondary(unsigned int cpu, struct task_struct *idle);
extern void owl_secondary_init(unsigned int cpu);

static struct smp_operations owl_smp_ops =
{
#ifdef CONFIG_SMP
    .smp_init_cpus = owl_smp_init_cpus,
    .smp_prepare_cpus = owl_smp_prepare_cpus,
    .smp_secondary_init = owl_secondary_init,
    .smp_boot_secondary = owl_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_kill = owl_cpu_kill,
	.cpu_die = owl_cpu_die,
	.cpu_disable = owl_cpu_disable,
#endif
#endif
};


bool __init owl_smp_init(void)
{
    smp_set_ops(&owl_smp_ops);
    return true;
}

extern void __init owl_timer_init(void);
extern void gic_handle_irq(struct pt_regs *regs);

MACHINE_START(OWL, "gs705a")
#ifdef CONFIG_OF
	.dt_compat	= owl_dt_match,
#endif
	.atag_offset	= 0x00000100,
	.smp_init       = owl_smp_init,
	.init_early	= owl_init_early,
	.map_io		= owl_map_io,
	.reserve	= owl_reserve,
	.init_irq	= owl_init_irq,
	.handle_irq	= gic_handle_irq,
	.init_machine	= owl_board_init,
	.init_time  = &owl_timer_init,
MACHINE_END
