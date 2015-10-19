/*
 * arch/arm/mach-owl/hotplug_base-owl.c
 *
 * cpu hotplug stuff for Actions SOC
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/smp.h>

#include <asm/cp15.h>
#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>

#include <mach/hardware.h>

extern volatile int pen_release;

static inline void cpu_enter_lowpower(void)
{
	unsigned int v;

	/*flush_cache_all();*/
	/*
	Invalidate all instruction caches to PoU. Also flushes branch target cache.
	Clean data cache line to PoC by VA.
	Disable data coherency with other cores in the Cortex-A9 MPCore processor.(ACTLR)
	Data caching disabled at all levels.(SCTLR)
	*/
	asm volatile(
	"	mcr	p15, 0, %1, c7, c5, 0\n"
	"	mcr	p15, 0, %1, c7, c10, 4\n"
	/*
	 * Turn off coherency
	 */
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	bic	%0, %0, %3\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	bic	%0, %0, %2\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	  : "=&r" (v)
	  : "r" (0), "Ir" (CR_C), "Ir" (0x40)
	  : "cc");
}

static inline void cpu_leave_lowpower(void)
{
	unsigned int v;
	asm volatile(
	"mrc	p15, 0, %0, c1, c0, 0\n"
	"	orr	%0, %0, %1\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	orr	%0, %0, %2\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	  : "=&r" (v)
	  : "Ir" (CR_C), "Ir" (0x40)
	  : "cc");
}

static void __iomem *scu_base_addr(void)
{
    return (void *)IO_ADDRESS(OWL_PA_SCU);
}

extern void cpu_reset_to_brom( void );
static inline void platform_do_lowpower(unsigned int cpu)
{
	void __iomem *scu_base = scu_base_addr();

	flush_cache_all();
	cpu_enter_lowpower();

	/* we put the platform to just WFI */
	for (;;) {
		
		if ((cpu >= 1) && (cpu < NR_CPUS))
		{
			if (cpu == 1)
			{
				cpu_reset_to_brom();
			}
			else
			{
				scu_power_mode(scu_base, 0x3);
			}
		}
		__asm__ __volatile__("dsb\n\t" "wfi\n\t"
				: : : "memory");
		if (pen_release == cpu_logical_map(cpu)) {
			/*
			 * OK, proper wakeup, we're done
			 */
			break;
		}
	}
	cpu_leave_lowpower();
}

int owl_cpu_kill(unsigned int cpu)
{
	return 1;
}

/*
 * platform-specific code to shutdown a CPU
 *
 * Called with IRQs disabled
 */
void owl_cpu_die(unsigned int cpu)
{
	/* directly enter low power state, skipping secure registers */
	platform_do_lowpower(cpu);
}

int owl_cpu_disable(unsigned int cpu)
{
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */
	return cpu == 0 ? -EPERM : 0;
}
