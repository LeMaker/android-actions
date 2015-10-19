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

#ifndef __ASM_SPI_H
#define __ASM_SPI_H

/* SPI0 */
#define SPI0_BASE			0xB0200000
#define SPI0_CTL			(SPI0_BASE + 0x0000)
#define SPI0_CLKDIV			(SPI0_BASE + 0x0004)
#define SPI0_STAT			(SPI0_BASE + 0x0008)
#define SPI0_RXDAT			(SPI0_BASE + 0x000c)
#define SPI0_TXDAT			(SPI0_BASE + 0x0010)
#define SPI0_TCNT			(SPI0_BASE + 0x0014)
#define SPI0_SEED			(SPI0_BASE + 0x0018)
#define SPI0_TXCR			(SPI0_BASE + 0x001c)
#define SPI0_RXCR			(SPI0_BASE + 0x0020)

/* SPI1 */
#define SPI1_BASE			0xB0204000
#define SPI1_CTL			(SPI1_BASE + 0x0000)
#define SPI1_CLKDIV			(SPI1_BASE + 0x0004)
#define SPI1_STAT			(SPI1_BASE + 0x0008)
#define SPI1_RXDAT			(SPI1_BASE + 0x000c)
#define SPI1_TXDAT			(SPI1_BASE + 0x0010)
#define SPI1_TCNT			(SPI1_BASE + 0x0014)
#define SPI1_SEED			(SPI1_BASE + 0x0018)
#define SPI1_TXCR			(SPI1_BASE + 0x001c)
#define SPI1_RXCR			(SPI1_BASE + 0x0020)

/* SPI2 */
#define SPI2_BASE			0xB0208000
#define SPI2_CTL			(SPI2_BASE + 0x0000)
#define SPI2_CLKDIV			(SPI2_BASE + 0x0004)
#define SPI2_STAT			(SPI2_BASE + 0x0008)
#define SPI2_RXDAT			(SPI2_BASE + 0x000c)
#define SPI2_TXDAT			(SPI2_BASE + 0x0010)
#define SPI2_TCNT			(SPI2_BASE + 0x0014)
#define SPI2_SEED			(SPI2_BASE + 0x0018)
#define SPI2_TXCR			(SPI2_BASE + 0x001c)
#define SPI2_RXCR			(SPI2_BASE + 0x0020)

/* SPI3 */
#define SPI3_BASE			0xB020C000
#define SPI3_CTL			(SPI3_BASE + 0x0000)
#define SPI3_CLKDIV			(SPI3_BASE + 0x0004)
#define SPI3_STAT			(SPI3_BASE + 0x0008)
#define SPI3_RXDAT			(SPI3_BASE + 0x000c)
#define SPI3_TXDAT			(SPI3_BASE + 0x0010)
#define SPI3_TCNT			(SPI3_BASE + 0x0014)
#define SPI3_SEED			(SPI3_BASE + 0x0018)
#define SPI3_TXCR			(SPI3_BASE + 0x001c)
#define SPI3_RXCR			(SPI3_BASE + 0x0020)

#endif

