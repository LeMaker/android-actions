/*
 * arch/arm/mach-owl/board-owl.h
 *
 * Copyright (C) 2013 Actions, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_BOARD_OWL_H
#define __MACH_BOARD_OWL_H

#include <linux/types.h>

extern void __init owl_init_clocks(void);
extern void __init owl_map_io(void);
extern void __init owl_reserve(void);
extern void __init owl_init_irq(void);

#endif
