/*
 * cvbs_ip.h
 *
 * CVBS header definition for OWL IP.
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

#ifndef _CVBS_ACTS_OWL_H_
#define _CVBS_ACTS_OWL_H_

	#define     TVOUT_EN                                                          (0x0000)
	#define     TVOUT_OCR                                                   	    (0x0004)
	#define     TVOUT_STA                                                         (0x0008)
	#define     TVOUT_CCR                                                         (0x000C)
	#define     TVOUT_BCR                                                         (0x0010)
	#define     TVOUT_CSCR                                                        (0x0014)
	#define     TVOUT_PRL                                                         (0x0018)
	#define     TVOUT_VFALD                                                 		  (0x001C)
	#define     CVBS_MSR                                                          (0x0020)
	#define     CVBS_AL_SEPO                                          				    (0x0024)
	#define     CVBS_AL_SEPE                                          					  (0x0028)
	#define     CVBS_AD_SEP                                                       (0x002c)
	#define     CVBS_HUECR                                                        (0x0030)
	#define     CVBS_SCPCR                                                        (0x0034)
	#define     CVBS_SCFCR                                                        (0x0038)
	#define     CVBS_CBACR                                                        (0x003c)
	#define     CVBS_SACR                                                         (0x0040)
	
	/*********************BT*************************/
	#define     BT_MSR0                                                           (0x0100)
	#define     BT_MSR1                                                           (0x0104)
	#define     BT_AL_SEPO                                                        (0x0108)
	#define     BT_AL_SEPE                                                        (0x010c)
	#define     BT_AP_SEP                                                         (0x0110)
	
	#define     TVOUT_DCR                                                         (0x0070)
	#define     TVOUT_DDCR                                                        (0x0074)
	#define     TVOUT_DCORCTL                                                     (0x0078)
	#define     TVOUT_DRCR                                                        (0x007c)
#endif
