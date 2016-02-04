/*
 * linux/drivers/video/owl/dss/de.h
 *
 * Copyright (C) 2014 Actions
 * Author: Lipeng<lipeng@actions-semi.com>
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

#ifndef _OWL_DSS_DE_H_
#define _OWL_DSS_DE_H_

#include <video/owldss.h>

#define DE_MAX_NR_ISRS				5

#define DE_SCLCOEF_ZOOMIN			0
#define DE_SCLCOEF_HALF_ZOOMOUT			1
#define DE_SCLCOEF_SMALLER_ZOOMOUT		2

#define DE_SCALE_CONST_VALUE			8192

enum de_hw_id {
	DE_HW_ID_ATM7059TC,
	DE_HW_ID_ATM7059A,
	DE_HW_ID_ATM9009A,
};

enum de_irq_type {
	DE_IRQ_CVBS_PRE = 0x01,
	DE_IRQ_DSI_PRE = 0x02, 
	DE_IRQ_HDMI_PRE =0x04,
	DE_IRQ_LCD_PRE = 0x08,
	DE_IRQ_LCD2_PRE,
	DE_IRQ_EDP_PRE,
	DE_IRQ_MAX,
};

enum de_video_fb {
        DE_VIDEO_FB0 = 0,
        DE_VIDEO_FB1,
        DE_VIDEO_FB2,
        DE_VIDEO_FB_R0,
        DE_VIDEO_FB_R1,
        DE_VIDEO_FB_R2,
};

struct owl_de_hwops {
	/* global operations */
	u32 (*irq_status_get)(void);
	void (*irq_status_set)(u32 status);

	u32 (*irq_enable_get)(void);
	void (*irq_enable_set)(u32 enable);

	u32 (*irq_to_mask)(enum de_irq_type irq);
	u32 (*irq_mask_to_enable)(u32 mask);
	u32 (*irq_mask_to_vb_mask)(u32 mask);

	int (*mmu_config)(u32 base_addr);
	int (*mmu_enable)(enum owl_plane video, bool enable);

	void (*special_init)(void);

	int (*suspend)(struct platform_device *pdev, pm_message_t state);
	int (*resume)(struct platform_device *pdev);

	/* path operations */
	void (*path_enable)(enum owl_de_path_id path, bool enbale);
	bool (*path_is_enabled)(enum owl_de_path_id path);

	void (*path_size_set)(enum owl_de_path_id path, u32 width, u32 height);
	
	void (*display_type_set)(enum owl_de_path_id path, enum owl_display_type type);

	void (*video_enable)(enum owl_de_path_id path,
			     enum owl_plane video, bool enbale);
	bool (*video_is_enabled)(enum owl_de_path_id path, enum owl_plane video);

	void (*fcr_set)(enum owl_de_path_id path);
	bool (*fcr_get)(enum owl_de_path_id path);

	void (*set_gamma_table)(enum owl_de_path_id path, u32 *gamma);
	void (*get_gamma_table)(enum owl_de_path_id path, u32 *gamma);
	void (*enable_gamma_table)(enum owl_de_path_id path, bool enable);

	void (*dither_set)(enum owl_de_path_id path, enum owl_dither_mode mode);
	void (*dither_enable)(enum owl_de_path_id path, bool enable);

	/* video layer operations */
	bool (*format_set)(enum owl_plane video, enum owl_color_mode color_mode);

	void (*bypass_enable)(enum owl_plane video, bool enable);

	void (*fb_addr_set)(enum owl_plane video, enum de_video_fb fb, void *paddr);

	void (*isize_set)(enum owl_plane video, u32 width, u32 height);
	void (*osize_set)(enum owl_plane video, u32 width, u32 height);
	void (*position_set)(enum owl_de_path_id path, enum owl_plane video,
				u32 x_pos, u32 u_pos);

	void (*scaling_set)(enum owl_plane video, u32 org_width, u32 org_height,
				u32 out_width, u32 out_height, bool ilace);
	void (*rotation_set)(enum owl_plane video, u8 rotation);
	void (*alpha_set)(enum owl_de_path_id path, enum owl_plane video,
				u8 alpha_value, bool alpha_en,  bool pre_mult_en);

	void (*critical_set)(enum owl_de_path_id path,enum owl_plane video);
	void (*lightness_set)(enum owl_plane video, u8 lightness);
	void (*saturation_set)(enum owl_plane video, u8 saturation);
	void (*contrast_set)(enum owl_plane video, u8 contrast);
			
	void (*str_set)(enum owl_plane video,
			enum owl_color_mode color_mode, u32 width);
			
	void (*curosr_enable)(enum owl_de_path_id path, bool enable);
	
	void (*curosr_set_position)(enum owl_de_path_id path, int pos_x, int pos_y);
	
	void (*curosr_set_addr)(enum owl_de_path_id path, void *paddr);
	
	void (*curosr_set_str)(enum owl_de_path_id path, u32 str);

	void (*dump_regs)(void);
	
	ssize_t (*write_regs)(const char *buf, size_t len);
};

struct owl_de_path_pdata {
	const enum owl_de_path_id		id;

	const enum owl_display_type	supported_displays;
};

struct owl_de_video_pdata {
	const enum owl_plane		id;

	const enum owl_color_mode	supported_colors;
};

typedef void (*owl_de_isr_t) (enum de_irq_type irq, void *arg);

struct de_isr_data {
	enum de_irq_type 		irq;
	owl_de_isr_t			isr;
	void				*arg;
	u32				mask;
};

struct de_regs_t {
	int reg;
	u32 value;
};

/*
 * The private data of OWL display engine.
 * num_paths,	the number of DE paths
 * num_videos,	the numbers of DE video layers
 */
struct owl_de_pdata {
	enum de_hw_id			id;

	/*
	 * constant information
	 */
	const u8			num_paths;
	const u8			num_videos;

	const struct owl_de_path_pdata	*path_pdata;
	const struct owl_de_video_pdata	*video_pdata;

	const struct owl_de_hwops	*hwops;

	/*
	 * platform data
	 */
	struct platform_device 		*pdev;
	void __iomem    		*base;

	int 				irq;
        spinlock_t 			irq_lock;
        u32 				irq_mask;
        struct de_isr_data 		registered_isr[DE_MAX_NR_ISRS];

	/* wait VB timeout times */
	u32				vb_timeout_cnt;
};

int __init owl_de_init(void);
void __exit owl_de_exit(void);

void de_mgr_set_path_size(enum owl_de_path_id channel, u16 width, u16 height);
void de_mgr_set_device_type(enum owl_de_path_id channel,enum owl_display_type type);
void de_mgr_enable(enum owl_de_path_id channel, bool enable);
void de_mgr_set_dither(enum owl_de_path_id channel, enum owl_dither_mode mode);
void de_mgr_enable_dither(enum owl_de_path_id channel, bool enable);
bool de_mgr_is_enabled(enum owl_de_path_id channel);
void de_mgr_go(enum owl_de_path_id channel);
void de_mgr_setup(enum owl_de_path_id channel, struct owl_overlay_manager_info *info);
void de_mgr_cursor_setup(enum owl_de_path_id channel, struct owl_cursor_info *info);
enum de_irq_type de_mgr_get_vsync_irq(enum owl_display_type type);

int owl_de_register_isr(enum de_irq_type irq, owl_de_isr_t isr, void *arg);
int owl_de_unregister_isr(enum de_irq_type irq, owl_de_isr_t isr, void *arg);

int de_wait_for_irq_timeout(u32 irqmask, unsigned long timeout);
int de_wait_for_irq_interruptible_timeout(enum de_irq_type irq,enum owl_de_path_id path, unsigned long timeout);



int de_ovl_setup(enum owl_de_path_id channel ,enum owl_plane plane, struct owl_overlay_info *oi, bool ilace);
int de_ovl_enable(enum owl_de_path_id channel ,enum owl_plane plane, bool enable);

int owl_de_mmu_config(u32 base_addr);
int owl_de_mmu_enable(enum owl_plane plane, bool enable);

void de_set_gamma_table(enum owl_de_path_id path, u32 *gamma);
void de_get_gamma_table(enum owl_de_path_id path, u32 *gamma);
void de_enable_gamma_table(enum owl_de_path_id path, bool enable);
bool de_channel_check_boot_inited(enum owl_de_path_id path);


int hdmi_en(void);

int cvbs_en(void);

void io_remap(void);

bool de_is_vb_valid(enum owl_de_path_id path, enum owl_display_type type);

#ifdef CONFIG_VIDEO_OWL_DE_ATM7059
extern struct owl_de_pdata 		owl_de_atm7059;
#endif
#ifdef CONFIG_VIDEO_OWL_DE_ATM9009
extern struct owl_de_pdata 		owl_de_atm9009;
#endif

#endif
