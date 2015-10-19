/*
 * powergate definitions
 *
 * Copyright 2013 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __ASM_ARCH_POWERGATE_H
#define __ASM_ARCH_POWERGATE_H

enum owl_powergate_id {
	OWL_POWERGATE_CPU2 = 0,
	OWL_POWERGATE_CPU3,
	OWL_POWERGATE_GPU3D,
	OWL_POWERGATE_VCE_BISP,
	OWL_POWERGATE_VDE,
	/*atm7059 avaliabe only*/
	OWL_POWERGATE_USB2_0,
	OWL_POWERGATE_USB2_1,
	OWL_POWERGATE_USB3,
	OWL_POWERGATE_DS,
	OWL_POWERGATE_DMA,
	OWL_POWERGATE_MAXID,
};

int  __init owl_powergate_init(void);

int owl_cpu_powergate_id(int cpuid);
int owl_powergate_is_powered(enum owl_powergate_id id);
int owl_powergate_power_on(enum owl_powergate_id id);
int owl_powergate_power_off(enum owl_powergate_id id);

#endif /* __ASM_ARCH_POWERGATE_H */
