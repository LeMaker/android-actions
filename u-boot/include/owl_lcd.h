/*
 * owl_lcd.h - OWL lcd driver
 *
 * Copyright (C) 2012, Actions Semiconductor Co. LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

 #ifndef __OWL_VIDEO_LCD_H__
 #define __OWL_VIDEO_LCD_H__
 
#include <linux/list.h>
#include <linux/fb.h>
#include <asm/arch/pwm_backlight.h>
#include <asm/gpio.h>

enum LCD_PORT_TYPE {
	LCD_PORT_TYPE_RGB		= 0x0,
	LCD_PORT_TYPE_CPU			= 0x1,
	LCD_PORT_TYPE_LVDS		= 0x2,

	LCD_PORT_TYPE_EDP		= 0x3,
};

#define  PARALLEL_IF_DATA_WIDTH_24BIT   0x0
#define  PARALLEL_IF_DATA_WIDTH_18BIT   0x1
#define  PARALLEL_IF_DATA_WIDTH_16BIT   0x2
#define  PARALLEL_IF_DATA_WIDTH_8BIT    0x3

#define  SERIAL_IF_DATA_WIDTH_24BIT     0x4
#define  SERIAL_IF_DATA_WIDTH_18BIT     0x5

struct owl_lcd {
	u32 port_type;
	const struct fb_videomode *mode;
	u32 data_width;
	u32 vsync_inversion;
	u32 hsync_inversion;
	u32 dclk_inversion;
	u32 lde_inversion;

	u32 lvds_ctl;
	u32 lvds_alg_ctl0;

	int lcd_clk_src;

	struct owl_fdt_gpio_state lcdpower_gpio;
	struct owl_fdt_gpio_state lcdpower2_gpio;
	struct owl_fdt_gpio_state lcdreset_gpio;
	struct owl_fdt_gpio_state lcdstandby_gpio;
	struct owl_regulator *lcdvcc_regulator;

	struct pwm_backlight_data pwm_bl_data;

	int lcd_port_type;
	struct lcdi_convertion *lcdic; 

	int enable_state;

#ifdef CONFIG_OF_CONTROL
	int dev_node;
#endif
};

/*board specific functions for lcd driver*/
extern int platform_get_lcd_par(struct owl_lcd *);
extern int platform_enable_lcd(void);


/*driver help functions for board specifical use*/
extern int enable_lcdc(void);
extern int owl_lcd_init(void);

#ifdef CONFIG_OWL_DISPLAY_LCD
extern void set_lcd_bl_power(bool enable);
#else
static inline void set_lcd_bl_power(bool enable) {}
#endif

#endif /* __OWL_VIDEO_LCD_H__ */
