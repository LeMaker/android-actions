/*
 * arch/arm/mach-gl5202/include/mach/hardware.h
 *
 * hardware support
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <linux/types.h>
#include <mach/regs_map-atm7039.h>

#define OWL_SDRAM_BASE			UL(0x00000000)
#define OWL_IO_DEVICE_BASE		UL(0xB0000000)

#define OWL_IO_ADDR_BASE		0xF8000000

/* macro to get at IO space when running virtually */
#ifdef CONFIG_MMU
/*
 * Statically mapped addresses:
 *
 * b0xx xxxx -> f8xx xxxx
 * b1xx xxxx -> f9xx xxxx
 * b2xx xxxx -> faxx xxxx
 * b3xx xxxx -> fbxx xxxx
 */
/*
#define IO_ADDRESS(x)	(((void __iomem __force *)OWL_IO_ADDR_BASE \
			+ ((x) & 0x03ffffff))
*/
#define IO_ADDRESS(x)	(OWL_IO_ADDR_BASE + ((x) & 0x03ffffff))
#else
#define IO_ADDRESS(x)	(x)
#endif

#define __io_address(n)	 __io(IO_ADDRESS(n))

/*
 * Peripheral physical addresses
 */
#define OWL_PA_REG_BASE	(0xB0000000)
#define OWL_PA_REG_SIZE	(6 * SZ_1M)

#define OWL_PA_CORESIGHT	(0xB0000000)
#define OWL_PA_EXTDEBUG	(0xB0008000)

#define OWL_PA_COREPERI	(0xB0020000)
#define OWL_PA_SCU		(0xB0020000)
#define OWL_PA_GIC_CPU		(0xB0020100)
#define OWL_PA_GIC_GP		(0xB0020200)
#define OWL_PA_TWD		(0xB0020600)
#define OWL_PA_GIC_DIST	(0xB0021000)
#define OWL_IO_COREPERI_SIZE   (SZ_8K)

#define OWL_PA_L2CC		(0xB0022000)
#define OWL_IO_L2CC_SIZE	(SZ_4K)

#define OWL_PA_BOOT_RAM	(0xFFFF8000)
#define OWL_VA_BOOT_RAM	(0xFFFF8000)

#ifndef __ASSEMBLY__
/******************************************************************************/
static inline void act_writeb(u8 val, u32 reg)
{
	*(volatile u8 *)(IO_ADDRESS(reg)) = val;
}

static inline void act_writew(u16 val, u32 reg)
{
	*(volatile u16 *)(IO_ADDRESS(reg)) = val;
}

static inline void act_writel(u32 val, u32 reg)
{
	*(volatile u32 *)(IO_ADDRESS(reg)) = val;
}
/******************************************************************************/
static inline u8 act_readb(u32 reg)
{
	return *(volatile u8 *)IO_ADDRESS(reg);
}

static inline u16 act_readw(u32 reg)
{
	return *(volatile u16 *)IO_ADDRESS(reg);
}

static inline u32 act_readl(u32 reg)
{
	return *(volatile u32 *)IO_ADDRESS(reg);
}
/******************************************************************************/
static inline void act_setb(u8 val, u32 reg)
{
	*(volatile u8 *)IO_ADDRESS(reg) |= val;
}

static inline void act_setw(u16 val, u32 reg)
{
	*(volatile u16 *)IO_ADDRESS(reg) |= val;
}

static inline void act_setl(u32 val, u32 reg)
{
	*(volatile u32 *)IO_ADDRESS(reg) |= val;
}
/******************************************************************************/
static inline void act_clearb(u8 val, u32 reg)
{
	*(volatile u8 *)IO_ADDRESS(reg) &= ~val;
}

static inline void act_clearw(u16 val, u32 reg)
{
	*(volatile u16 *)IO_ADDRESS(reg) &= ~val;
}

static inline void act_clearl(u32 val, u32 reg)
{
	*(volatile u32 *)IO_ADDRESS(reg) &= ~val;
}
/******************************************************************************/
#endif

#endif /* __ASM_ARCH_HARDWARE_H */
