/*
 * arch/arm/mach-owl/cpu-owl.c
 *
 * cpu peripheral init for Actions SOC
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/of_irq.h>
#include <linux/module.h>

#include <asm/hardware/cache-l2x0.h>
//#include <asm/hardware/gic.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/localtimer.h>
#include <asm/smp_twd.h>

#include <mach/hardware.h>
#include <mach/irqs.h>

extern void __init owl_gp_timer_init(void);


static struct map_desc owl_io_desc[] __initdata = {
	{
		.virtual	= IO_ADDRESS(OWL_PA_REG_BASE),
		.pfn		= __phys_to_pfn(OWL_PA_REG_BASE),
		.length		= OWL_PA_REG_SIZE,
		.type		= MT_DEVICE,
	},

   /*for suspend , to load ddr ops code. by jlingzhang*/
#if 1
	{
		.virtual	= OWL_VA_BOOT_RAM,
		.pfn		= __phys_to_pfn(OWL_PA_BOOT_RAM),
		.length		= SZ_4K,
		.type		= MT_MEMORY,
	},
#endif
};

/*
 return ATM7033: 0xf, ATM7039: 0xe
 */
int cpu_package(void)
{
	return (act_readl(0xb01b00e0) & 0xf);
}
EXPORT_SYMBOL(cpu_package);

void __init owl_map_io(void)
{
	/* dma coherent allocate buffer: 2 ~ 14MB */
	init_dma_coherent_pool_size(14 << 20);
	iotable_init(owl_io_desc, ARRAY_SIZE(owl_io_desc));
}

#ifdef CONFIG_OF
extern int gic_of_init(struct device_node *node, struct device_node *parent);
static const struct of_device_id owl_dt_irq_match[] __initconst = {
	{ .compatible = "arm,cortex-a9-gic", .data = gic_of_init },
	{ }
};
#endif

extern void of_irq_init(const struct of_device_id *matches);

void __init owl_init_irq(void)
{
#ifdef CONFIG_OF
	of_irq_init(owl_dt_irq_match);
#else
	gic_init(0, 29, IO_ADDRESS(OWL_PA_GIC_DIST),
		IO_ADDRESS(OWL_PA_GIC_CPU));
#endif
}

#ifdef CONFIG_CACHE_L2X0
static int __init owl_l2x0_init(void)
{
	void __iomem *l2x0_base;
	u32 val;

	printk(KERN_INFO "%s()\n", __func__);

	l2x0_base = (void *)IO_ADDRESS(OWL_PA_L2CC);

	/* config l2c310 */
	act_writel(0x78800002, (unsigned int)OWL_PA_L2CC + L2X0_PREFETCH_CTRL);
	act_writel(L2X0_DYNAMIC_CLK_GATING_EN | L2X0_STNDBY_MODE_EN,
		(unsigned int)OWL_PA_L2CC + L2X0_POWER_CTRL);

	/* Instruction prefetch enable
	   Data prefetch enable
	   Round-robin replacement
	   Use AWCACHE attributes for WA
	   32kB way size, 16 way associativity
	   disable exclusive cache
	*/
	val = (1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT)
	| (1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT)
	| (1 << L2X0_AUX_CTRL_NS_INT_CTRL_SHIFT)
	| (1 << L2X0_AUX_CTRL_NS_LOCKDOWN_SHIFT)
	| (1 << 25) /* round robin*/
	| (0x2 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT)
	| (1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT);

#ifdef CONFIG_OF
	l2x0_of_init(val, L2X0_AUX_CTRL_MASK);
#else
	l2x0_init(l2x0_base, val, L2X0_AUX_CTRL_MASK);
#endif

	return 0;
}
early_initcall(owl_l2x0_init);
#endif

#ifdef CONFIG_HAVE_ARM_TWD
static DEFINE_TWD_LOCAL_TIMER(twd_local_timer,
				  OWL_PA_TWD,
				  IRQ_LOCALTIMER);
static void __init owl_twd_init(void)
{
	int err = twd_local_timer_register(&twd_local_timer);
	if (err)
		pr_err("twd_local_timer_register failed %d\n", err);
}
#else
#define owl_twd_init()	do { } while (0)
#endif


void __init owl_timer_init(void)
{
	owl_gp_timer_init();

	owl_twd_init();
}

