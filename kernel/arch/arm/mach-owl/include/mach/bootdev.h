/*
 * arch/arm/mach-leopard/include/mach/bootdev.h
 *
 * Boot device
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __ASM_ARCH_BOOTDEV_H
#define __ASM_ARCH_BOOTDEV_H

#define OWL_BOOTDEV_NAND       (0x00)
#define OWL_BOOTDEV_SD0        (0x20)
#define OWL_BOOTDEV_SD1        (0x21)
#define OWL_BOOTDEV_SD2        (0x22)
#define OWL_BOOTDEV_SD02NAND   (0x30)   //nand for cardburn 
#define OWL_BOOTDEV_SD02SD2    (0x31)	 //emmc for cardburn 
#define OWL_BOOTDEV_NOR        (0x40)   //spinor

/*
 * get boot device name
 */
extern int owl_get_boot_dev(void);




#define OWL_BOOT_MODE_NORMAL 	0
#define OWL_BOOT_MODE_UPGRADE 	1
#define OWL_BOOT_MODE_CHARGER	2
#define OWL_BOOT_MODE_RECOVERY	3
/*return boot mode*/
extern int owl_get_boot_mode(void);

#endif /* __ASM_ARCH_BOOTDEV_H */
