/*
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

#define IRQ_LOCALTIMER				(29)
#define IRQ_LOCALWDT				(30)

#define OWL_IRQ_OFFSET				(32)
#define OWL_IRQ(x)					((x) + OWL_IRQ_OFFSET)

#define OWL_IRQ_ETHERNET			OWL_IRQ(0)
#define OWL_IRQ_DE					OWL_IRQ(1)
#define OWL_IRQ_xxx					OWL_IRQ(2)	/* reserved */
#define OWL_IRQ_GPU_3D				OWL_IRQ(3)
#define OWL_IRQ_PC0					OWL_IRQ(4)
#define OWL_IRQ_PC1					OWL_IRQ(5)
#define OWL_IRQ_PC2					OWL_IRQ(6)
#define OWL_IRQ_PC3					OWL_IRQ(7)
#define OWL_IRQ_2HZ0				OWL_IRQ(8)
#define OWL_IRQ_2HZ1				OWL_IRQ(9)
#define OWL_IRQ_TIMER0				OWL_IRQ(10)
#define OWL_IRQ_TIMER1				OWL_IRQ(11)
#define OWL_IRQ_BISP				OWL_IRQ(12)
#define OWL_IRQ_SIRQ0				OWL_IRQ(13)
#define OWL_IRQ_SIRQ1				OWL_IRQ(14)
#define OWL_IRQ_SIRQ2				OWL_IRQ(15)
#define OWL_IRQ_KEY					OWL_IRQ(16)
#define OWL_IRQ_PCM0				OWL_IRQ(17)
#define OWL_IRQ_PCM1				OWL_IRQ(18)
#define OWL_IRQ_SPI0				OWL_IRQ(19)
#define OWL_IRQ_SPI1				OWL_IRQ(20)
#define OWL_IRQ_SPI2				OWL_IRQ(21)
#define OWL_IRQ_SPI3				OWL_IRQ(22)
#define OWL_IRQ_USB3				OWL_IRQ(23)
#define OWL_IRQ_USBH0				OWL_IRQ(24)
#define OWL_IRQ_I2C0				OWL_IRQ(25)
#define OWL_IRQ_I2C1				OWL_IRQ(26)
#define OWL_IRQ_I2C2				OWL_IRQ(27)
#define OWL_IRQ_I2C3				OWL_IRQ(28)
#define OWL_IRQ_UART0				OWL_IRQ(29)
#define OWL_IRQ_UART1				OWL_IRQ(30)
#define OWL_IRQ_UART2				OWL_IRQ(31)
#define OWL_IRQ_UART3				OWL_IRQ(32)
#define OWL_IRQ_UART4				OWL_IRQ(33)
#define OWL_IRQ_UART5				OWL_IRQ(34)
#define OWL_IRQ_UART6				OWL_IRQ(35)
#define OWL_IRQ_GPIOA				OWL_IRQ(36)
#define OWL_IRQ_GPIOB				OWL_IRQ(37)
#define OWL_IRQ_GPIOC				OWL_IRQ(38)
#define OWL_IRQ_GPIOD				OWL_IRQ(39)
#define OWL_IRQ_GPIOE				OWL_IRQ(40)
#define OWL_IRQ_NAND				OWL_IRQ(41)
#define OWL_IRQ_SD0					OWL_IRQ(42)
#define OWL_IRQ_SD1					OWL_IRQ(43)
#define OWL_IRQ_SD2					OWL_IRQ(44)
#define OWL_IRQ_LCD					OWL_IRQ(45)
#define OWL_IRQ_HDMI				OWL_IRQ(46)
#define OWL_IRQ_TVOUT				OWL_IRQ(47)
#define OWL_IRQ_AUDIO_INOUT			OWL_IRQ(48)
#define OWL_IRQ_H264_JPEG_ENCODER	OWL_IRQ(49)
#define OWL_IRQ_HIVDE_DECODER		OWL_IRQ(50)
#define OWL_IRQ_DSI					OWL_IRQ(51)
#define OWL_IRQ_CSI					OWL_IRQ(52)
/* OWL_IRQ(53) */
#define OWL_IRQ_DCU_DEBUG			OWL_IRQ(54)
#define OWL_IRQ_L2					OWL_IRQ(55)
#define OWL_IRQ_HDCP2TX				OWL_IRQ(56)
#define OWL_IRQ_DMA0				OWL_IRQ(57)
#define OWL_IRQ_DMA1				OWL_IRQ(58)
#define OWL_IRQ_DMA2				OWL_IRQ(59)
#define OWL_IRQ_DMA3				OWL_IRQ(60)
#define OWL_IRQ_USBH1				OWL_IRQ(61)

#define OWL_IRQ_RESERVE1			OWL_IRQ(62)
#define OWL_IRQ_RESERVE2			OWL_IRQ(63)

//owl phy irqs num
#define NR_OWL_PHY_IRQS				(OWL_IRQ(63) + 1)

/*------------------------virtual irq used linear irq map---------------------*/
/* virtual sirq interrupt num used by sirq interrupt controler */
#define NR_OWL_SIRQ_IRQS			(3)

/* virtual gpio interrupt num used by gpio interrupt controler */
#define NR_OWL_GPIO_IRQS			(4 * 32 + 4)

/* virtual atc260x interrupt num used by atc260x interrupt controler */
#define NR_OWL_ATC260X_IRQS		    16

//owl virtual irqs num
#define NR_OWL_VIR_IRQS		        (NR_OWL_SIRQ_IRQS + NR_OWL_GPIO_IRQS + NR_OWL_ATC260X_IRQS)

//interrupt table size:
#define NR_IRQS						(NR_OWL_PHY_IRQS + NR_OWL_VIR_IRQS)

#endif  /* __ASM_ARCH_IRQS_H */
