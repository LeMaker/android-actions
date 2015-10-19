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

#ifndef __ASM_GMP_H
#define __ASM_GMP_H

#define GPIO_MFP_PWM_BASE		0xB01B0000
#define GPIO_AOUTEN			(GPIO_MFP_PWM_BASE + 0x0000)
#define GPIO_AINEN			(GPIO_MFP_PWM_BASE + 0x0004)
#define GPIO_ADAT			(GPIO_MFP_PWM_BASE + 0x0008)
#define GPIO_BOUTEN			(GPIO_MFP_PWM_BASE + 0x000C)
#define GPIO_BINEN			(GPIO_MFP_PWM_BASE + 0x0010)
#define GPIO_BDAT			(GPIO_MFP_PWM_BASE + 0x0014)
#define GPIO_COUTEN			(GPIO_MFP_PWM_BASE + 0x0018)
#define GPIO_CINEN			(GPIO_MFP_PWM_BASE + 0x001C)
#define GPIO_CDAT			(GPIO_MFP_PWM_BASE + 0x0020)
#define GPIO_DOUTEN			(GPIO_MFP_PWM_BASE + 0x0024)
#define GPIO_DINEN			(GPIO_MFP_PWM_BASE + 0x0028)
#define GPIO_DDAT			(GPIO_MFP_PWM_BASE + 0x002C)
#define GPIO_EOUTEN			(GPIO_MFP_PWM_BASE + 0x0030)
#define GPIO_EINEN			(GPIO_MFP_PWM_BASE + 0x0034)
#define GPIO_EDAT			(GPIO_MFP_PWM_BASE + 0x0038)
#define MFP_CTL0			(GPIO_MFP_PWM_BASE + 0x0040)
#define MFP_CTL1			(GPIO_MFP_PWM_BASE + 0x0044)
#define MFP_CTL2			(GPIO_MFP_PWM_BASE + 0x0048)
#define MFP_CTL3			(GPIO_MFP_PWM_BASE + 0x004C)
#define PWM_CTL0			(GPIO_MFP_PWM_BASE + 0X50)
#define PWM_CTL1			(GPIO_MFP_PWM_BASE + 0X54)
#define PWM_CTL2			(GPIO_MFP_PWM_BASE + 0X58)
#define PWM_CTL3			(GPIO_MFP_PWM_BASE + 0X5C)
#define PWM_CTL4			(GPIO_MFP_PWM_BASE + 0X78)
#define PWM_CTL5			(GPIO_MFP_PWM_BASE + 0X7C)
#define PAD_PULLCTL0			(GPIO_MFP_PWM_BASE + 0x0060)
#define PAD_PULLCTL1			(GPIO_MFP_PWM_BASE + 0x0064)
#define PAD_PULLCTL2			(GPIO_MFP_PWM_BASE + 0x0068)
#define PAD_ST0			(GPIO_MFP_PWM_BASE + 0x006C)
#define PAD_ST1			(GPIO_MFP_PWM_BASE + 0x0070)
#define PAD_CTL			(GPIO_MFP_PWM_BASE + 0x0074)
#define PAD_DRV0			(GPIO_MFP_PWM_BASE + 0x0080)
#define PAD_DRV1			(GPIO_MFP_PWM_BASE + 0x0084)
#define PAD_DRV2			(GPIO_MFP_PWM_BASE + 0x0088)
#define DEBUG_SEL			(GPIO_MFP_PWM_BASE + 0x0090)
#define DEBUG_OEN0			(GPIO_MFP_PWM_BASE + 0x0094)
#define DEBUG_OEN1			(GPIO_MFP_PWM_BASE + 0x0098)
#define DEBUG_IEN0			(GPIO_MFP_PWM_BASE + 0x009C)
#define DEBUG_IEN1			(GPIO_MFP_PWM_BASE + 0x00A0)
#define BIST_START0			(GPIO_MFP_PWM_BASE + 0x00C0)
#define BIST_START1			(GPIO_MFP_PWM_BASE + 0x00C4)
#define BIST_DONE0			(GPIO_MFP_PWM_BASE + 0x00C8)
#define BIST_DONE1			(GPIO_MFP_PWM_BASE + 0x00CC)
#define BIST_DONE2			(GPIO_MFP_PWM_BASE + 0x00D0)
#define BIST_FAIL0			(GPIO_MFP_PWM_BASE + 0x00D4)
#define BIST_FAIL1			(GPIO_MFP_PWM_BASE + 0x00D8)
#define BIST_FAIL2			(GPIO_MFP_PWM_BASE + 0x00DC)
#define L2_BIST_INSTR1			(GPIO_MFP_PWM_BASE + 0x00F0)
#define L2_BIST_INSTR2			(GPIO_MFP_PWM_BASE + 0x00F4)
#define L2_BIST_INSTR3			(GPIO_MFP_PWM_BASE + 0x00F8)
#define INTC_EXTCTL			(GPIO_MFP_PWM_BASE + 0x0200)
#define INTC_GPIOCTL			(GPIO_MFP_PWM_BASE + 0x0204)
#define INTC_GPIOA_PD			(GPIO_MFP_PWM_BASE + 0x0208)
#define INTC_GPIOA_MSK			(GPIO_MFP_PWM_BASE + 0x020c)
#define INTC_GPIOB_PD			(GPIO_MFP_PWM_BASE + 0x0210)
#define INTC_GPIOB_MSK			(GPIO_MFP_PWM_BASE + 0x0214)
#define INTC_GPIOC_PD			(GPIO_MFP_PWM_BASE + 0x0218)
#define INTC_GPIOC_MSK			(GPIO_MFP_PWM_BASE + 0x021c)
#define INTC_GPIOD_PD			(GPIO_MFP_PWM_BASE + 0x0220)
#define INTC_GPIOD_MSK			(GPIO_MFP_PWM_BASE + 0x0224)
#define INTC_GPIOE_PD			(GPIO_MFP_PWM_BASE + 0x0228)
#define INTC_GPIOE_MSK			(GPIO_MFP_PWM_BASE + 0x022c)

#endif

