/*
 * arch/arm/mach-leopard/include/mach/iversion.h
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef __ASM_ARCH_IVERSION_H
#define __ASM_ARCH_IVERSION_H

enum {
	MNO_1 = 1,
	MNO_2,
	MNO_3,
	MNO_4,
	MNO_5,
	MNO_6,
	MNO_MAX,
};

#extern unsigned char icversion_get(void);
#extern unsigned int mno_get(void);

#endif /* __ASM_ARCH_IVERSION_H */
