/*
 * arch/arm/mach-leopard/include/mach/bootafinfo.h
 *
 * Boot AFInfo interface
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __ASM_ARCH_BOOTAFINFO_H
#define __ASM_ARCH_BOOTAFINFO_H

/*
 * get boot afinfo 
 */
extern unsigned char *owl_get_boot_afinfo(void);
extern int owl_get_boot_afinfo_len(void);

#endif /* __ASM_ARCH_BOOTAFINFO_H */
