/*
 * (C) Copyright 2012
 * Actions Semi .Inc
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __ASM_CLOCKS_H
#define __ASM_CLOCKS_H

#define CMU_BASE			0xB0160000

#define CMU_COREPLL			(CMU_BASE + 0x0000)
#define CMU_DEVPLL			(CMU_BASE + 0x0004)
#define CMU_DDRPLL			(CMU_BASE + 0x0008)
#define CMU_NANDPLL			(CMU_BASE + 0x000C)
#define CMU_DISPLAYPLL			(CMU_BASE + 0x0010)
#define CMU_AUDIOPLL			(CMU_BASE + 0x0014)
#define CMU_TVOUTPLL			(CMU_BASE + 0x0018)
#define CMU_BUSCLK			(CMU_BASE + 0x001C)
#define CMU_SENSORCLK			(CMU_BASE + 0x0020)
#define CMU_LCDCLK			(CMU_BASE + 0x0024)
#define CMU_DSICLK			(CMU_BASE + 0x0028)
#define CMU_CSICLK			(CMU_BASE + 0x002C)
#define CMU_DECLK			(CMU_BASE + 0x0030)
#define CMU_BISPCLK			(CMU_BASE + 0x0034)
#define CMU_BUSCLK1			(CMU_BASE + 0x0038)
#define CMU_VDECLK			(CMU_BASE + 0x0040)
#define CMU_VCECLK			(CMU_BASE + 0x0044)
#define CMU_NANDCCLK			(CMU_BASE + 0x004C)
#define CMU_SD0CLK			(CMU_BASE + 0x0050)
#define CMU_SD1CLK			(CMU_BASE + 0x0054)
#define CMU_SD2CLK			(CMU_BASE + 0x0058)
#define CMU_UART0CLK			(CMU_BASE + 0x005C)
#define CMU_UART1CLK			(CMU_BASE + 0x0060)
#define CMU_UART2CLK			(CMU_BASE + 0x0064)
#define CMU_PWM4CLK			(CMU_BASE + 0x0068)
#define CMU_PWM5CLK			(CMU_BASE + 0x006C)
#define CMU_PWM0CLK			(CMU_BASE + 0x0070)
#define CMU_PWM1CLK			(CMU_BASE + 0x0074)
#define CMU_PWM2CLK			(CMU_BASE + 0x0078)
#define CMU_PWM3CLK			(CMU_BASE + 0x007C)
#define CMU_USBPLL			(CMU_BASE + 0x0080)
#define CMU_ETHERNETPLL			(CMU_BASE + 0x0084)
#define CMU_LENSCLK			(CMU_BASE + 0x008C)
#define CMU_GPU3DCLK			(CMU_BASE + 0x0090)
#define CMU_DEVCLKEN0			(CMU_BASE + 0x00A0)
#define CMU_DEVCLKEN1			(CMU_BASE + 0x00A4)
#define CMU_DEVRST0			(CMU_BASE + 0x00A8)
#define CMU_DEVRST1			(CMU_BASE + 0x00AC)
#define CMU_UART3CLK			(CMU_BASE + 0x00B0)
#define CMU_UART4CLK			(CMU_BASE + 0x00B4)
#define CMU_UART5CLK			(CMU_BASE + 0x00B8)
#define CMU_UART6CLK			(CMU_BASE + 0x00BC)
#define CMU_DIGITALDEBUG		(CMU_BASE + 0x00D0)
#define CMU_ANALOGDEBUG			(CMU_BASE + 0x00D4)
#define CMU_COREPLLDEBUG		(CMU_BASE + 0x00D8)
#define CMU_DEVPLLDEBUG			(CMU_BASE + 0x00DC)
#define CMU_DDRPLLDEBUG			(CMU_BASE + 0x00E0)
#define CMU_NANDPLLDEBUG		(CMU_BASE + 0x00E4)
#define CMU_DISPLAYPLLDEBUG		(CMU_BASE + 0x00E8)
#define CMU_TVOUTPLLDEBUG		(CMU_BASE + 0x00EC)
#define CMU_DEEPCOLORPLLDEBUG		(CMU_BASE + 0x00F4)
#define CMU_AUDIOPLL_ETHPLLDEBUG	(CMU_BASE + 0x00F8)

/* reset0 definition */
#define CMU_RST_DMAC			(0x1 << 0)
#define CMU_RST_SRAMI			(0x1 << 1)
#define CMU_RST_DDR			(0x1 << 2)
#define CMU_RST_NANDC			(0x1 << 3)
#define CMU_RST_SD0			(0x1 << 4)
#define CMU_RST_SD1			(0x1 << 5)
#define CMU_RST_DE			(0x1 << 7)
#define CMU_RST_LCD			(0x1 << 8)

/* reset1 definition */
#define CMU_RST_USB2			(0x1 << 0)
#define CMU_RST_UART0			(0x1 << 5)
#define CMU_RST_UART1			(0x1 << 6)
#define CMU_RST_UART2			(0x1 << 7)
#define CMU_RST_UART3			(0x1 << 15)
#define CMU_RST_UART4			(0x1 << 16)
#define CMU_RST_UART5			(0x1 << 17)
#define CMU_RST_UART6			(0x1 << 4)
#define CMU_RST_SPI0			(0x1 << 8)
#define CMU_RST_SPI1			(0x1 << 9)
#define CMU_RST_SPI2			(0x1 << 10)
#define CMU_RST_SPI3			(0x1 << 11)
#define CMU_RST_I2C0			(0x1 << 12)
#define CMU_RST_I2C1			(0x1 << 13)
#define CMU_RST_ETHERNET		(0x1 << 20)

#define CMU_RST_WDREST(x)		(0x1 << (24 + (x)))
#define CMU_RST_WD0REST			CMU_RST_WDREST(0)
#define CMU_RST_WD1REST			CMU_RST_WDREST(1)
#define CMU_RST_WD2REST			CMU_RST_WDREST(2)
#define CMU_RST_WD3REST			CMU_RST_WDREST(3)

#define CMU_RST_DBGREST(x)		(0x1 << (28 + (x)))
#define CMU_RST_DBG0REST		CMU_RST_DBGREST(0)
#define CMU_RST_DBG1REST		CMU_RST_DBGREST(1)
#define CMU_RST_DBG2REST		CMU_RST_DBGREST(2)
#define CMU_RST_DBG3REST		CMU_RST_DBGREST(3)

/* DEVCLKEN0 */
#define DEVCLKEN_GPU3D			(0x1 << 30)
#define DEVCLKEN_SHARERAM		(0x1 << 28)
#define DEVCLKEN_HDCP2X			(0x1 << 27)
#define DEVCLKEN_GPIO			(0x1 << 18)
#define DEVCLKEN_KEY			(0x1 << 17)
#define DEVCLKEN_BISP			(0x1 << 14)
#define DEVCLKEN_LCD1			(0x1 << 10)
#define DEVCLKEN_LCD0			(0x1 << 9)
#define DEVCLKEN_DE			(0x1 << 8)
#define DEVCLKEN_SD2			(0x1 << 7)
#define DEVCLKEN_SD1			(0x1 << 6)
#define DEVCLKEN_SD0			(0x1 << 5)
#define DEVCLKEN_NANDC			(0x1 << 4)
#define DEVCLKEN_DDRCH0			(0x1 << 3)
#define DEVCLKEN_SRAMI			(0x1 << 2)
#define DEVCLKEN_DMAC			(0x1 << 1)
#define DEVCLKEN_DDRCH1			(0x1 << 0)

/* DEVCLKEN1 */
#define DEVCLKEN_I2C3			(0x1 << 31)
#define DEVCLKEN_I2C2			(0x1 << 30)
#define DEVCLKEN_NOC1			(0x1 << 28)
#define DEVCLKEN_I2C1			(0x1 << 15)
#define DEVCLKEN_I2C0			(0x1 << 14)
#define DEVCLKEN_TIMER			(0x1 << 27)
#define DEVCLKEN_PWM3			(0x1 << 26)
#define DEVCLKEN_PWM2			(0x1 << 25)
#define DEVCLKEN_PWM1			(0x1 << 24)
#define DEVCLKEN_PWM0			(0x1 << 23)
#define DEVCLKEN_ETHERNET		(0x1 << 22)
#define DEVCLKEN_UART6			(0x1 << 18)
#define DEVCLKEN_UART5			(0x1 << 21)
#define DEVCLKEN_UART4			(0x1 << 20)
#define DEVCLKEN_UART3			(0x1 << 19)
#define DEVCLKEN_UART2			(0x1 << 8)
#define DEVCLKEN_UART1			(0x1 << 7)
#define DEVCLKEN_UART0			(0x1 << 6)

#define DEVCLKEN_PCM1			(0x1 << 16)
#define DEVCLKEN_IRC			(0x1 << 9)
#define DEVCLKEN_SPI3			(0x1 << 13)
#define DEVCLKEN_SPI2			(0x1 << 12)
#define DEVCLKEN_SPI1			(0x1 << 11)
#define DEVCLKEN_SPI0			(0x1 << 10)

#endif

