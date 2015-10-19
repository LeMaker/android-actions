/*
 * arch/arm/mach-leopard/include/mach/debug.h
 *
 * debug stuff
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef __ASM_ARCH_DEBUG_H
#define __ASM_ARCH_DEBUG_H

extern void owl_dump_mem(void *startaddr, int size, void *showaddr, int show_bytes);
extern void owl_dump_reg(unsigned int addr, int size);

#endif /* __ASM_ARCH_DEBUG_H */
