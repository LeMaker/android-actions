/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2012 Actions Semi Inc.
*/
/******************************************************************************/

#ifndef __CHIPID_REG_ATM7039_H__
#define __CHIPID_REG_ATM7039_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* CHIPID_CTL0 */
#define CHIPID_CTL0_EFUSE_CLKDIV_MASK			(0x7 << 5)
#define CHIPID_CTL0_EFUSE_CLKDIV_SET(x)			(((x) & 0x7) << 5)
#define CHIPID_CTL0_DATA_READY				(0x1 << 0)

/******************************************************************************/
/* CHIPID_DAT_1 */
#define CHIPID_DAT_1_ID_MASK				(0xFF << 0)
#define CHIPID_DAT_1_ID_SET(x)				(((x) & 0xFF) << 0)

/******************************************************************************/
/* CHIPID_CTL1 */
#define CHIPID_CTL1_VDDQ_EN				(0x1 << 17)
#define CHIPID_CTL1_VDDQ_SET_MASK			(0x7 << 14)
#define CHIPID_CTL1_VDDQ_SET_SET(x)			(((x) & 0x7) << 14)
#define CHIPID_CTL1_B2P_FLAG				(0x1 << 13)
#define CHIPID_CTL1_B1P_FLAG				(0x1 << 12)
#define CHIPID_CTL1_PGENB				(0x1 << 11)
#define CHIPID_CTL1_STROBE				(0x1 << 10)
#define CHIPID_CTL1_LOAD				(0x1 << 9)
#define CHIPID_CTL1_CSB					(0x1 << 8)
#define CHIPID_CTL1_P_SEL				(0x1 << 7)
#define CHIPID_CTL1_B_SEL				(0x1 << 6)
#define CHIPID_CTL1_A_MASK				(0x3F << 0)
#define CHIPID_CTL1_A_SET(x)				(((x) & 0x3F) << 0)

/******************************************************************************/
/* CHIPID_CTL2 */
#define CHIPID_CTL2_PASSWORD_MASK			(0xFFFFFFFF << 0)
#define CHIPID_CTL2_PASSWORD_SET(x)		(((x) & 0xFFFFFFFF) << 0)

#ifdef __cplusplus
}
#endif

#endif  /* ifndef __CHIPID_REG_ATM7039_H__ */
