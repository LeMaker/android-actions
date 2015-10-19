/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#if 0
#define CPUFREQ_DBG(format, ...) \
	printk(KERN_NOTICE "owl cpufreq: " format, ## __VA_ARGS__)

#else /* DEBUG */
#define CPUFREQ_DBG(format, ...)
#endif

#define CPUFREQ_ERR(format, ...) \
	printk(KERN_ERR "owl cpufreq: " format, ## __VA_ARGS__)

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/opp.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <asm/cpu.h>

#include <mach/module-owl.h>
#include <mach/clkname.h>
#include <mach/clkname_priv.h>
#include <mach/cpu_map-owl.h>

extern int owl_cpu0_cpufreq_driver_init(void);
extern void owl_cpu0_cpufreq_driver_exit(void);

static int cpu0_cpufreq_driver_init(void)
{
	return owl_cpu0_cpufreq_driver_init();
}

static void __exit cpu0_cpufreq_driver_exit(void)
{
	return cpu0_cpufreq_driver_exit();
}

late_initcall(cpu0_cpufreq_driver_init);
module_exit(cpu0_cpufreq_driver_exit);

MODULE_AUTHOR("Actions semi");
MODULE_DESCRIPTION("owl CPU0 cpufreq driver");
MODULE_LICENSE("GPL");
