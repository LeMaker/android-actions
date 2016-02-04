/*
 * cec.h
 *
 * HDMI OWL IP driver Library
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: HaiYu Huang  <huanghaiyu@actions-semi.com>
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


#ifndef _OWL_HDMI_CEC_H_
#define _OWL_HDMI_CEC_H_ __FILE__

/*****************************************************************************
 * This file includes declarations for external functions of
 * Samsung TVOUT-related hardware. So only external functions
 * to be used by higher layer must exist in this file.
 *
 * Higher layer must use only the declarations included in this file.
 ****************************************************************************/

#define to_tvout_plat(d) (to_platform_device(d)->dev.platform_data)


#ifndef hdmi_cec_dbg
#ifdef CONFIG_TV_DEBUG
#define hdmi_cec_dbg(fmt, ...)					\
		printk(KERN_DEBUG "[%s] %s(): " fmt,		\
			"HDMI-CEC", __func__, ##__VA_ARGS__)
#else
#define hdmi_cec_dbg(fmt, ...)
#endif
#endif

enum s5p_tvout_endian {
	TVOUT_LITTLE_ENDIAN = 0,
	TVOUT_BIG_ENDIAN = 1
};

enum cec_state {
	STATE_RX,
	STATE_TX,
	STATE_DONE,
	STATE_ERROR
};

struct cec_rx_struct {
	spinlock_t lock;
	wait_queue_head_t waitq;
	atomic_t state;
	u8 *buffer;
	unsigned int size;
};

struct cec_tx_struct {
	wait_queue_head_t waitq;
	atomic_t state;
};

extern struct cec_rx_struct cec_rx_struct;
extern struct cec_tx_struct cec_tx_struct;

void hdmi_cec_hw_init(void);
void hdmi_cec_set_divider(void);
void hdmi_cec_enable_rx(void);
void hdmi_cec_disable_rx(void);
void hdmi_cec_enable_tx(void);
void hdmi_cec_disable_tx(void);
void hdmi_cec_mask_rx_interrupts(void);
void hdmi_cec_unmask_rx_interrupts(void);
void hdmi_cec_mask_tx_interrupts(void);
void hdmi_cec_unmask_tx_interrupts(void);
void hdmi_cec_reset(void);
void hdmi_cec_tx_reset(void);
void hdmi_cec_rx_reset(void);
void hdmi_cec_threshold(void);
void hdmi_cec_set_tx_state(enum cec_state state);
void hdmi_cec_set_rx_state(enum cec_state state);
void hdmi_cec_copy_packet(char *data, size_t count);
void hdmi_cec_set_addr(u32 addr);
u32  hdmi_cec_get_status(void);
void hdmi_cec_clr_pending_tx(void);
void hdmi_cec_clr_pending_rx(void);
u8 hdmi_cec_get_rx_header(void);
void hdmi_cec_get_rx_buf(u32 size, u8 *buffer);
int __init hdmi_cec_mem_probe(struct platform_device *pdev);

#endif /* _OWL_HDMI_CEC_H_ */
