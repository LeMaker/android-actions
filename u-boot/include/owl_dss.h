/*
 * owl_dss.h - OWL display driver
 *
 * Copyright (C) 2012, Actions Semiconductor Co. LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/list.h>
#include <linux/fb.h>

enum disp_dev_id {
	LCD_DISPLAYER = 0x1,
	HDMI_DISPLAYER = 0x2,
	TV_CVBS_DISPLAYER = 0x4,
	TV_YPbPr_DISPLAYER = 0x8,
	LCD1_DISPLAYER = 0x10,
	DSI_DISPLAYER = 0x20,
};

#define MAX_NUM_DISP_DEV	6

struct display_ops {
	int (*enable)(void);
	int (*disable)(void);
	int (*config)(const struct fb_videomode *mode);
};

/* videomode lib, choose one as display default mode when display register */
extern const struct fb_videomode owl_mode_800_480;
extern const struct fb_videomode owl_mode_1280_720;

int owl_display_register(u32 display_id, const char * name , struct display_ops *ops,
		const struct fb_videomode *def_mode,int data_width,int rotate);

/********************/
enum {
	LCDIC_TYPE_LVDS2EDP = 0,
	LCDIC_TYPE_MAX,
};

struct lcdi_convertion;

struct lcdi_convertion_ops {
	int (*enable)(struct lcdi_convertion *);	
	int (*disable)(struct lcdi_convertion *);
};

struct lcdi_convertion {
	int type;
	struct lcdi_convertion_ops *ops;
};

int lcdi_convertion_register(int type,
		struct lcdi_convertion_ops *ops);
struct lcdi_convertion *lcdi_convertion_get(int type);
int lcdi_convertion_enable(struct lcdi_convertion *lcdic);
int lcdi_convertion_disable(struct lcdi_convertion *lcdic);

/********************/



enum owl_arrange_option {
	/*no scale, preserve aspect*/
	ARR_ORIGINAL_SIZE = 0,

	/*fill the whole screen,ignore aspect*/
	ARR_SCALE_FULL_SIZE = 1,

	/*scale the image to fit the screen, centred , preserve aspect*/
	ARR_SCALE_FIT_SIZE = 2,

	/*if original image is smaller than screen, then original size*/
	ARR_SCALE_ORIGINAL_OR_FIT_SIZE = 3,

	/*user defined scale ratio, preserve aspect*/
	/*ARR_SCALE_CUSTOM_EQUAL_SCALE	= 4,*/

	ARR_SCALE_MAX,
	ARR_2X_MODE = 16,
	ARR_ALL_CUSTOM = 32,
};

/* Prototypes for external board-specific functions */
#define CPU_PWM_VOLT_TABLE_LEN 16
struct gamma_info {
	unsigned int gamma_table[CONFIG_SYS_GAMMA_SIZE / 4];
	int is_valid;
};

struct cpu_pwm_volt_table {
	unsigned int pwm_val;
	unsigned int voltage_mv;
};

struct cpu_pwm_volt_info {
	struct cpu_pwm_volt_table cpu_pwm_volt_tb[CPU_PWM_VOLT_TABLE_LEN];
	int cpu_pwm_volt_tb_len;
};

struct kernel_reserve_info {
	struct gamma_info gamma;
	struct cpu_pwm_volt_info cpu_pwm_volt;
} __attribute__ ((packed));

extern struct kernel_reserve_info *kinfo;

extern void kinfo_init(void);