/*
 * arch/arm/mach-owl/include/mach/irqs.h
 *
 * IRQ definitions
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

#define IRQ_LOCALTIMER              (29)
#define IRQ_LOCALWDT                (30)

#define OWL_IRQ_OFFSET             (32)
#define OWL_IRQ(x)                 ((x) + OWL_IRQ_OFFSET)

#define OWL_IRQ_ETHERNET           OWL_IRQ(0)
#define OWL_IRQ_DE                 OWL_IRQ(1)
#define OWL_IRQ_IMX                OWL_IRQ(2)
#define OWL_IRQ_GPU                OWL_IRQ(3)
#define OWL_IRQ_PC0                OWL_IRQ(4)
#define OWL_IRQ_PC1                OWL_IRQ(5)
#define OWL_IRQ_PC2                OWL_IRQ(6)
#define OWL_IRQ_PC3                OWL_IRQ(7)
#define OWL_IRQ_RESERVED0          OWL_IRQ(8)
#define OWL_IRQ_RESERVED1          OWL_IRQ(9)
#define OWL_IRQ_TIMER0             OWL_IRQ(10)
#define OWL_IRQ_TIMER1             OWL_IRQ(11)
#define OWL_IRQ_BISP               OWL_IRQ(12)
#define OWL_IRQ_SIRQ0              OWL_IRQ(13)
#define OWL_IRQ_SIRQ1              OWL_IRQ(14)
#define OWL_IRQ_SIRQ2              OWL_IRQ(15)
#define OWL_IRQ_GPIOF              OWL_IRQ(16)
#define OWL_IRQ_PCM0               OWL_IRQ(17)
#define OWL_IRQ_PCM1               OWL_IRQ(18)
#define OWL_IRQ_SPI0               OWL_IRQ(19)
#define OWL_IRQ_SPI1               OWL_IRQ(20)
#define OWL_IRQ_SPI2               OWL_IRQ(21)
#define OWL_IRQ_SPI3               OWL_IRQ(22)
#define OWL_IRQ_USB3               OWL_IRQ(23)
#define OWL_IRQ_USB2H0             OWL_IRQ(24)
#define OWL_IRQ_I2C0               OWL_IRQ(25)
#define OWL_IRQ_I2C1               OWL_IRQ(26)
#define OWL_IRQ_I2C2               OWL_IRQ(27)
#define OWL_IRQ_I2C3               OWL_IRQ(28)
#define OWL_IRQ_UART0              OWL_IRQ(29)
#define OWL_IRQ_UART1              OWL_IRQ(30)
#define OWL_IRQ_UART2              OWL_IRQ(31)
#define OWL_IRQ_UART3              OWL_IRQ(32)
#define OWL_IRQ_UART4              OWL_IRQ(33)
#define OWL_IRQ_UART5              OWL_IRQ(34)
#define OWL_IRQ_UART6              OWL_IRQ(35)
#define OWL_IRQ_GPIOA              OWL_IRQ(36)
#define OWL_IRQ_GPIOB              OWL_IRQ(37)
#define OWL_IRQ_GPIOC              OWL_IRQ(38)
#define OWL_IRQ_GPIOD              OWL_IRQ(39)
#define OWL_IRQ_GPIOE              OWL_IRQ(40)
#define OWL_IRQ_NAND0              OWL_IRQ(41)
#define OWL_IRQ_SD0                OWL_IRQ(42)
#define OWL_IRQ_SD1                OWL_IRQ(43)
#define OWL_IRQ_SD2                OWL_IRQ(44)
#define OWL_IRQ_LCD                OWL_IRQ(45)
#define OWL_IRQ_HDMI               OWL_IRQ(46)
#define OWL_IRQ_USB2H1             OWL_IRQ(47)
#define OWL_IRQ_AUDIO_INOUT        OWL_IRQ(48)
#define OWL_IRQ_VCE                OWL_IRQ(49)
#define OWL_IRQ_VDE                OWL_IRQ(50)
#define OWL_IRQ_DSI                OWL_IRQ(51)
#define OWL_IRQ_CSI0               OWL_IRQ(52)
#define OWL_IRQ_NAND1              OWL_IRQ(53)
#define OWL_IRQ_DCU_DEBUG          OWL_IRQ(54)
#define OWL_IRQ_L2                 OWL_IRQ(55)
#define OWL_IRQ_HDCP2TX            OWL_IRQ(56)
#define OWL_IRQ_DMA0               OWL_IRQ(57)
#define OWL_IRQ_DMA1               OWL_IRQ(58)
#define OWL_IRQ_DMA2               OWL_IRQ(59)
#define OWL_IRQ_DMA3               OWL_IRQ(60)
#define OWL_IRQ_CSI1               OWL_IRQ(61)
#define OWL_IRQ_SD3                OWL_IRQ(62)
#define OWL_IRQ_SECURE_TIME2       OWL_IRQ(63)
#define OWL_IRQ_SECURE_TIME3       OWL_IRQ(64)
#define OWL_IRQ_EDP                OWL_IRQ(65)
#define OWL_IRQ_HDE                OWL_IRQ(66)
#define OWL_IRQ_SE                 OWL_IRQ(67)
#define OWL_IRQ_DCU_CH0            OWL_IRQ(68)
#define OWL_IRQ_DCU_CH1            OWL_IRQ(69)

/* Set the default NR_IRQS */
#define NR_OWL_IRQS                (OWL_IRQ(69) + 1)

/* virtual IRQs: external speical IRQs */
#define OWL_EXT_IRQ_SIRQ_BASE               (NR_OWL_IRQS)
#define OWL_EXT_IRQ_SIRQ0                   (OWL_EXT_IRQ_SIRQ_BASE + 0)
#define OWL_EXT_IRQ_SIRQ1                   (OWL_EXT_IRQ_SIRQ_BASE + 1)
#define OWL_EXT_IRQ_SIRQ2                   (OWL_EXT_IRQ_SIRQ_BASE + 2)
#define NR_OWL_SIRQ                (3)

/* virtual IRQs: GPIO */
#define OWL_EXT_IRQ_GPIOA_BASE              (OWL_EXT_IRQ_SIRQ_BASE + NR_OWL_SIRQ)
#define OWL_EXT_IRQ_GPIOA(x)                (OWL_EXT_IRQ_GPIOA_BASE + (x))
#define OWL_EXT_IRQ_GPIOB_BASE              (OWL_EXT_IRQ_GPIOA(31) + 1)
#define OWL_EXT_IRQ_GPIOB(x)                (OWL_EXT_IRQ_GPIOB_BASE + (x))
#define OWL_EXT_IRQ_GPIOC_BASE              (OWL_EXT_IRQ_GPIOB(31) + 1)
#define OWL_EXT_IRQ_GPIOC(x)                (OWL_EXT_IRQ_GPIOC_BASE + (x))
#define OWL_EXT_IRQ_GPIOD_BASE              (OWL_EXT_IRQ_GPIOC(31) + 1)
#define OWL_EXT_IRQ_GPIOD(x)                (OWL_EXT_IRQ_GPIOD_BASE + (x))

#define NR_OWL_EXT_GPIO_INT            (3 * 32 + 22)   /* for ATM7029 */

#define OWL_EXT_GPIO_TO_IRQ(gpio)      ((gpio) + OWL_EXT_IRQ_GPIOA_BASE)
#define OWL_EXT_IRQ_TO_GPIO(irq)       ((irq) - OWL_EXT_IRQ_GPIOA_BASE)

/* virtual IRQs: ATC260x */
#define IRQ_ATC260X_BASE             (OWL_EXT_IRQ_GPIOA_BASE + NR_OWL_EXT_GPIO_INT)
/* reserved 16 interrupt sources for ATC260x */
#define IRQ_ATC260X_MAX_NUM          16

#define NR_IRQS                     (IRQ_ATC260X_BASE + IRQ_ATC260X_MAX_NUM)

#endif  /* __ASM_ARCH_IRQS_H */
