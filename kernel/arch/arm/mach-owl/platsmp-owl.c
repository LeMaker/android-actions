/*
 * arch/arm/mach-owl/platsmp-owl.c
 *
 * Platform file needed for Leopard. This file is based on arm
 * realview smp platform.
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/cpu.h>

#include <asm/cacheflush.h>
#include <asm/localtimer.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>

#include <mach/hardware.h>
#include <mach/powergate.h>
#include <mach/smp.h>
#include <mach/module-owl.h>

#define BOOT_FLAG					(0x55aa)
#define CPU_SHIFT(cpu)	(19 + cpu)

static DEFINE_SPINLOCK(boot_lock);

static void __iomem *scu_base_addr(void)
{
	return (void *)IO_ADDRESS(OWL_PA_SCU);
}

/*
 * control for which core is the next to come out of the secondary
 * boot "holding pen"
 */
//volatile int pen_release = -1;

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}

extern int gic_secondary_init(struct notifier_block *nfb,
					unsigned long action, void *hcpu);

void __cpuinit owl_secondary_init(unsigned int cpu)
{
	trace_hardirqs_off();

	/*
	 * if any interrupts are already enabled for the primary
	 * core (e.g. timer irq), then they will not have been enabled
	 * for us: do so
	 */
	gic_secondary_init(NULL, CPU_STARTING, (void*)0);

	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

static void wakeup_secondary(unsigned int cpu)
{
	enum owl_powergate_id cpuid;

	pr_info("po %d\n", cpu);

	cpuid = owl_cpu_powergate_id(cpu);
	owl_powergate_power_on(cpuid);

	/* wait CPUx run to WFE instruct */
	udelay(200);

	pr_info("wu %d\n", cpu);

	/*
	 * write the address of secondary startup into the boot ram register
	 * at offset 0x204/0x304, then write the flag to the boot ram register
	 * at offset 0x200/0x300, which is what boot rom code is waiting for.
	 * This would wake up the secondary core from WFE
	 */
	switch (cpu) {
	case 1:
		/* ensure cpu1 dbg module reset complete before access dbg register */
		module_reset(MODULE_RST_DBG1RESET);
		udelay(10);
		
		act_writel(virt_to_phys(owl_secondary_startup), CPU1_ADDR);
		act_writel(BOOT_FLAG, CPU1_FLAG);
		break;
	case 2:
		act_writel(virt_to_phys(owl_secondary_startup),
				 CPU2_ADDR);
		act_writel(BOOT_FLAG, CPU2_FLAG);
		break;
	case 3:
		act_writel(virt_to_phys(owl_secondary_startup),
				CPU3_ADDR);
		act_writel(BOOT_FLAG, CPU3_FLAG);
		break;
	default:
		printk(KERN_INFO "%s(): invalid cpu number %d\n",
			__func__, cpu);
		break;
	}

	/*
	 * Send a 'sev' to wake the secondary core from WFE.
	 * Drain the outstanding writes to memory
	 */
	dsb_sev();
	mb();
}

int __cpuinit owl_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	printk(KERN_INFO "cpu:%d\n",  cpu);

	wakeup_secondary(cpu);

	/* wait for CPUx wakeup */
	udelay(10);

	/*
	 * set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * The secondary processor is waiting to be released from
	 * the holding pen - release it, then wait for it to flag
	 * that it has been released by resetting pen_release.
	 */
	write_pen_release(cpu_logical_map(cpu));
	smp_send_reschedule(cpu);

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		if (pen_release == -1)
			break;
	}

	switch (cpu) {
	case 1:
		act_writel(0, CPU1_ADDR);
		act_writel(0, CPU1_FLAG);
		break;
	case 2:
		act_writel(0, CPU2_ADDR);
		act_writel(0, CPU2_FLAG);
		break;
	case 3:
		act_writel(0, CPU3_ADDR);
		act_writel(0, CPU3_FLAG);
		break;
	default:
		printk(KERN_INFO "%s(): invalid cpu number %d\n",
			__func__, cpu);
		break;
	}

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);
	printk(KERN_INFO "pr:%d\n", pen_release);

	return pen_release != -1 ? -ENOSYS : 0;
}

static bool powersave = false;

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init owl_smp_init_cpus(void)
{
	void __iomem *scu_base = scu_base_addr();
	unsigned int i, ncores;

	ncores = scu_base ? scu_get_core_count(scu_base) : 1;

	printk(KERN_INFO "%s(): ncores %d\n", __func__, ncores);

	/* sanity check */
	if (ncores > nr_cpu_ids) {
		printk(KERN_WARNING
			"[PLATSMP] no. of cores (%d) greater than configured "
			"maximum of %d - clipping\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}
	
	if(powersave)
		ncores = 2;
	
	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

	set_smp_cross_call(gic_raise_softirq);
}

void __init owl_smp_prepare_cpus(unsigned int max_cpus)
{
	printk(KERN_INFO "%s(max_cpus:%d)\n", __func__, max_cpus);

	scu_enable(scu_base_addr());
}

static int __init powersave_set(char *__unused)
{
	powersave = true;
	pr_alert("powersave now\n");
	return 0;
}
early_param("powersave", powersave_set);

