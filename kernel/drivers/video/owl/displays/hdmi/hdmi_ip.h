/*
 * hdmi_ip.h
 *
 * HDMI header definition for OWL IP.
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: Guo Long  <guolong@actions-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _HDMI_ACTS_OWL_H_
#define _HDMI_ACTS_OWL_H_

	#define     HDMI_VICTL                                                        (0x0000)
	#define     HDMI_VIVSYNC                                                      (0x0004)
	#define     HDMI_VIVHSYNC                                                     (0x0008)
	#define     HDMI_VIALSEOF                                                     (0x000C)
	#define     HDMI_VIALSEEF                                                     (0x0010)
	#define     HDMI_VIADLSE                                                      (0x0014)
	#define     HDMI_AIFRAMEC                                                     (0x0020)
	#define     HDMI_AICHSTABYTE0TO3                                              (0x0024)
	#define     HDMI_AICHSTABYTE4TO7                                              (0x0028)
	#define     HDMI_AICHSTABYTE8TO11                                             (0x002C)
	#define     HDMI_AICHSTABYTE12TO15                                            (0x0030)
	#define     HDMI_AICHSTABYTE16TO19                                            (0x0034)
	#define     HDMI_AICHSTABYTE20TO23                                            (0x0038)
	#define     HDMI_AICHSTASCN                                                   (0x003C)
	#define     HDMI_VR                                                           (0x0050)
	#define     HDMI_CR                                                           (0x0054)
	#define     HDMI_SCHCR                                                        (0x0058)
	#define     HDMI_ICR                                                          (0x005C)
	#define     HDMI_SCR                                                          (0x0060)
	#define     HDMI_LPCR                                                         (0x0064)
	#define     HDCP_CR                                                           (0x0068)
	#define     HDCP_SR                                                           (0x006C)
	#define     HDCP_ANLR                                                         (0x0070)
	#define     HDCP_ANMR                                                         (0x0074)
	#define     HDCP_ANILR                                                        (0x0078)
	#define     HDCP_ANIMR                                                        (0x007C)
	#define     HDCP_DPKLR                                                        (0x0080)
	#define     HDCP_DPKMR                                                        (0x0084)
	#define     HDCP_LIR                                                          (0x0088)
	#define     HDCP_SHACR                                                        (0x008C)
	#define     HDCP_SHADR                                                        (0x0090)
	#define     HDCP_ICR                                                          (0x0094)
	#define     HDCP_KMMR                                                         (0x0098)
	#define     HDCP_KMLR                                                         (0x009C)
	#define     HDCP_MILR                                                         (0x00A0)
	#define     HDCP_MIMR                                                         (0x00A4)
	#define     HDCP_KOWR                                                         (0x00A8)
	#define     HDCP_OWR                                                          (0x00AC)
	#define     TMDS_STR0                                                         (0x00B8)
	#define     TMDS_STR1                                                         (0x00BC)
	#define     TMDS_EODR0                                                        (0x00C0)
	#define     TMDS_EODR1                                                        (0x00C4)
	#define     HDMI_ASPCR                                                        (0x00D0)
	#define     HDMI_ACACR                                                        (0x00D4)
	#define     HDMI_ACRPCR                                                       (0x00D8)
	#define     HDMI_ACRPCTSR                                                     (0x00DC)
	#define     HDMI_ACRPPR                                                       (0x00E0)
	#define     HDMI_GCPCR                                                        (0x00E4)
	#define     HDMI_RPCR                                                         (0x00E8)
	#define     HDMI_RPRBDR                                                       (0x00EC)
	#define     HDMI_OPCR                                                         (0x00F0)
	#define     HDMI_DIPCCR                                                       (0x00F4)
	#define     HDMI_ORP6PH                                                       (0x00F8)
	#define     HDMI_ORSP6W0                                                      (0x00FC)
	#define     HDMI_ORSP6W1                                                      (0x0100)
	#define     HDMI_ORSP6W2                                                      (0x0104)
	#define     HDMI_ORSP6W3                                                      (0x0108)
	#define     HDMI_ORSP6W4                                                      (0x010C)
	#define     HDMI_ORSP6W5                                                      (0x0110)
	#define     HDMI_ORSP6W6                                                      (0x0114)
	#define     HDMI_ORSP6W7                                                      (0x0118)
	#define     HDMI_CECCR                                                        (0x011C)
	#define     HDMI_CECRTCR                                                      (0x0120)
	#define     HDMI_CECRXCR                                                      (0x0124)
	#define     HDMI_CECTXCR                                                      (0x0128)
	#define     HDMI_CECTXDR                                                      (0x012C)
	#define     HDMI_CECRXDR                                                      (0x0130)
	#define     HDMI_CECRXTCR                                                     (0x0134)
	#define     HDMI_CECTXTCR0                                                    (0x0138)
	#define     HDMI_CECTXTCR1                                                    (0x013C)
	#define     HDMI_CRCCR                                                        (0x0140)
	#define     HDMI_CRCDOR                                                       (0x0144)
	#define     HDMI_TX_1                                                         (0x0154)
	#define     HDMI_TX_2                                                         (0x0158)
	#define     CEC_DDC_HPD                                                       (0x015C)
	#define     MHL_PHYCTL1                                                       (0x0160)
	#define     MHL_PHYCTL2                                                       (0x0164)
	#define     MHL_PHYCTL3                                                       (0x0168)
	#define     MHL_CR                                                            (0x0180)
	#define     MHL_INTMSK                                                        (0x0184)
	#define     MHL_INTPD                                                         (0x0188)
	#define     MHL_INTSR                                                         (0x018c)
	#define     MSC_REQMSGR                                                       (0x0190)
	#define     MSC_REQRMSGR                                                      (0x0194)
	#define     MSC_RSPRMR                                                        (0x0198)
	#define     MSC_RSPRFIFO                                                      (0x019c)
	#define     MSC_RSPRRMR                                                       (0x01a0)
	#define     MHL_DDCCSR                                                        (0x01a4)
	#define     MHL_DDCPR                                                         (0x01a8)
	#define     CBUS_DCR0TO3                                                      (0x01b0)
	#define     CBUS_DCR4TO7                                                      (0x01b4)
	#define     CBUS_DCR8TOB                                                      (0x01b8)
	#define     CBUS_DCRCTOF                                                      (0x01bc)
	#define     CBUS_DEVINTR                                                      (0x01c0)
	#define     CBUS_DEVSR                                                        (0x01c4)
	#define     CBUS_SPR0TO3                                                      (0x01c8)
	#define     CBUS_SPR4TO7                                                      (0x01cc)
	#define     CBUS_SPR8TOB                                                      (0x01d0)
	#define     CBUS_SPRCTOF                                                      (0x01d4)
	#define     CBUS_VID                                                          (0x01d8)
	#define     CBUS_LLTCR                                                        (0x01e0)
	#define     CBUS_TLTCR                                                        (0x01e4)
	#define     MHL_DEBUG                                                         (0x1f0)

	/********************************************************/
	/* HDMI_LPCR */
	#define HDMI_LPCR_CURLINECNTR                       (0xFFF << 16)
	#define HDMI_LPCR_CURPIXELCNTR                      (0xFFF << 0)

	/********************************************************/
	/* HDCP_CR */
	#define HDCP_CR_EN1DOT1_FEATURE                     (1 << 31)
	#define HDCP_CR_DOWNSTRISREPEATER                   (1 << 30)
	#define HDCP_CR_ANINFREQ                            (1 << 25)
	#define HDCP_CR_SEEDLOAD                            (1 << 24)
	#define HDCP_CR_ENRIUPDINT                          (1 << 9)
	#define HDCP_CR_ENPJUPDINT                          (1 << 8)
	#define HDCP_CR_ANINFLUENCEMODE                     (1 << 7)
	#define HDCP_CR_HDCP_ENCRYPTENABLE                  (1 << 6)
	#define HDCP_CR_RESETKMACC                          (1 << 4)
	#define HDCP_CR_FORCETOUNAUTHENTICATED              (1 << 3)
	#define HDCP_CR_DEVICEAUTHENTICATED                 (1 << 2)
	#define HDCP_CR_AUTHCOMPUTE                         (1 << 1)
	#define HDCP_CR_AUTHREQUEST                         (1 << 0)

	/********************************************************/
	/* HDCP_SR */
	#define HDCP_SR_HDCPCIPHERSTATE(x)                  (((x) & 0xFF) << 24)
	#define HDCP_SR_RIUPDATED                           (1 << 5)
	#define HDCP_SR_PJUPDATED                           (1 << 4)
	#define HDCP_SR_CURDPKACCDONE                       (1 << 3)
	#define HDCP_SR_HDCP_ENCRYPT_STATUS                 (1 << 2)
	#define HDCP_SR_ANTHENTICATEDOK                     (1 << 1)
	#define HDCP_SR_ANREADY                             (1 << 0)


	/********************************************************/
	/* HDCP_LIR */
	#define HDCP_LIR_RI(x)                              (((x) & 0xFFFF) << 16)
	#define HDCP_LIR_PJ(x)                              (((x) & 0xFF) << 8)

	/********************************************************/
	/* HDCP_SHACR */
	#define HDCP_SHACR_VMATCH                           (1 << 4)
	#define HDCP_SHACR_SHAREADY                         (1 << 3)
	#define HDCP_SHACR_SHASTART                         (1 << 2)
	#define HDCP_SHACR_SHAFIRST                         (1 << 1)
	#define HDCP_SHACR_RSTSHAPTR                        (1 << 0)

	/********************************************************/
	/* HDCP_ICR */
	#define HDCP_ICR_RIRATE(x)                          (((x) & 0xFF) << 8)
	#define HDCP_ICR_PJRATE(x)                          (((x) & 0xFF) << 0)

	/* HDCP_KOWR */
	#define HDCP_KOWR_HDCPREKEYKEEPOUTWIN(x)            (((x) & 0xFF) << 24)
	#define HDCP_KOWR_HDCPVERKEEPOUTWINEND(x)           (((x) & 0xFFF) << 12)
	#define HDCP_KOWR_HDCPVERTKEEPOUTWINSTART(x)        (((x) & 0xFFF) << 0)

	/********************************************************/
	/* HDCP_OWR */
	#define HDCP_OWR_HDCPOPPWINEND(x)                   (((x) & 0xFFF) << 12)
	#define HDCP_OWR_HDCPOPPWINSTART(x)                 (((x) & 0xFFF) << 0)

#endif
