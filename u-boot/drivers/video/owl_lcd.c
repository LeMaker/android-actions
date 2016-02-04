/*
 * owl_lcd.c - OWL lcd driver
 *
 * Copyright (C) 2012, Actions Semiconductor Co. LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/owl_lcd.h>
#include <asm/io.h>
#include <asm/arch/pwm_backlight.h>
#include <asm/gpio.h>
#include <asm/arch/sps.h>
#include <asm/arch/sys_proto.h>

#include <common.h>
#include <malloc.h>

#include <video_fb.h>
#include <owl_dss.h>
#include <owl_lcd.h>
#include <linux/list.h>
#include <linux/fb.h>

DECLARE_GLOBAL_DATA_PTR;

#define PICOS2KHZ(a) (1000000000UL/(a))
#define KHZ2PICOS(a) (1000000000UL/(a))

static struct owl_lcd lcd_par;

static int fdtdec_enable_lcd(void);
static int fdtdec_disable_lcd(void);

static bool bl_power = 1;

#define DIV_ROUND(n,d)		(((n) + ((d)/2)) / (d))

static int get_lcd_data_width(struct owl_lcd *par)
{
	switch (par->data_width) {
	case 0:
		return 24;
	case 1:
		return 18;
	case 2:
		return 16;
	default:
		return 24;
	}
	return -1;
}

int __platform_enable_lcd(void)
{
	u32 tmp;

	/* get pin */
	tmp = readl(PAD_CTL);
	tmp |= 0x2;
	writel(tmp, PAD_CTL);

	tmp = readl(MFP_CTL1);
	tmp &= 0xff80003f;
	tmp |= 0x00600000;
	writel(tmp, MFP_CTL1);

	return 0;
}

int platform_enable_lcd(void)
			__attribute__((weak, alias("__platform_enable_lcd")));


void lvds_init(void)
{
	setbits_le32(USB3_P0_CTL, (1 << 28));

	writel(lcd_par.lvds_ctl, LVDS_CTL);
	writel(lcd_par.lvds_alg_ctl0, LVDS_ALG_CTL0);
}

void lvds_close(void)
{
	clrbits_le32(LVDS_ALG_CTL0, (1 << 31)|(1 << 30)|(1 << 4)|(1 << 5));
	clrbits_le32(LVDS_CTL, 0x1);
}

int enable_lcdc(void)
{
	u32 tmp;
	u32 lcd_id = 0;

	/* enable lcdc */
	tmp = readl(LCDx_CTL(lcd_id));
	tmp |= 0x1;
	writel(tmp, LCDx_CTL(lcd_id));
	return 0;
}

#define LCDCLK_SRC_DEVPLL	0
#define LCDCLK_SRC_DISPLAYPLL	1
#define LCDCLK_SRC_TVOUTPLL	2

int lcd_enable(void)
{
	u32 tmp = 0;
	const struct fb_videomode *mode = lcd_par.mode;
	u32 data_width = lcd_par.data_width;
	u32 vsync_inversion = lcd_par.vsync_inversion;
	u32 hsync_inversion = lcd_par.hsync_inversion;
	u32 dclk_inversion = lcd_par.dclk_inversion;
	u32 lde_inversion = lcd_par.lde_inversion;
	u32 port_type = lcd_par.port_type;
	u32 xres = mode->xres;
	u32 yres = mode->yres;
	u32 hbp = mode->left_margin;
	u32 hfp = mode->right_margin;
	u32 hspw = mode->hsync_len;
	u32 vbp = mode->upper_margin;
	u32 vfp = mode->lower_margin;
	u32 vspw = mode->vsync_len;

	int lcd_id = 0;

	int source_hz;
	int clksrc_regval;
	int target_hz;
	u32 lcd_div;
	u32 lcd0_div;

/******************/
	if (lcd_par.enable_state == 1)
		return 0;

	switch (lcd_par.lcd_clk_src) {
	case LCDCLK_SRC_DEVPLL:
		source_hz = owl_get_devpll_rate();
		clksrc_regval = 0x1;
		break;
	case LCDCLK_SRC_DISPLAYPLL:
		source_hz = owl_get_displaypll_rate();
		clksrc_regval = 0x1;
		break;
	default:
		return -1;
	}

	target_hz = PICOS2KHZ(lcd_par.mode->pixclock) * 1000;
	lcd0_div = DIV_ROUND(source_hz, target_hz);
	if (lcd0_div <= 12) {
		lcd_div = 1;
	} else {
		lcd_div = 7;
		lcd0_div = DIV_ROUND(source_hz / lcd_div, target_hz);
	}

	tmp = readl(CMU_LCDCLK);
	tmp &= (~0x31ff);
	if (lcd_div == 7)
		tmp |= (1 << 8);

	lcd0_div -= 1;
	tmp |= (lcd0_div);
	tmp |= (clksrc_regval << 12);
	writel(tmp, CMU_LCDCLK);
/******************/

/***reset lcdc***************************/
	tmp = readl(CMU_DEVRST0);
	tmp &= (~(0x1 << 8));
	writel(tmp, CMU_DEVRST0);

	udelay(50);

	tmp = readl(CMU_DEVRST0);
	tmp |= (0x1 << 8);
	writel(tmp, CMU_DEVRST0);
/*****************************/

/***enable lcdc module***************/
	setbits_le32(CMU_DEVCLKEN0, (1 << 9));
/*****************************/

	/*set size*/
	tmp = readl(LCDx_SIZE(lcd_id));
	tmp = tmp & (~(0x7ff<<16)) & (~(0x7ff));
	tmp = tmp | ((yres - 1) << 16) | (xres - 1);
	writel(tmp, LCDx_SIZE(lcd_id));

	/*set timeing0*/
	tmp = readl(LCDx_TIM0(lcd_id));
	/**tmp for preline*******/
	tmp = 0x2a00;
	/********/
	tmp = tmp & (~(0xf << 4));
	tmp = tmp | (vsync_inversion << 7)
		| (hsync_inversion << 6)
		| (dclk_inversion << 5)
		| (lde_inversion << 4);
	writel(tmp, LCDx_TIM0(lcd_id));

	/*set timeing1*/
	tmp = (hbp - 1) | ((hfp - 1) << 10) | ((hspw - 1) << 20);
	writel(tmp, LCDx_TIM1(lcd_id));

	/*set timeing2*/
	tmp = (vbp - 1) | ((vfp - 1) << 10) | ((vspw - 1) << 20);
	writel(tmp, LCDx_TIM2(lcd_id));

	/***set default color********/
	writel(0x66ff33, LCDx_COLOR(lcd_id));

	/*set lcd ctl*/
	tmp = readl(LCDx_CTL(lcd_id));
	tmp = tmp & (~(0x1 << 31)) & (~(0x7 << 16)) & (~(0x3 << 6));

	if (port_type == LCD_PORT_TYPE_CPU)
		tmp |= (0x1 << 31);

	/* rgb if, data width, video from de */
	tmp = tmp | (data_width << 16) | (0x2 << 6);
	writel(tmp, LCDx_CTL(lcd_id));

	/*we don't enable lcdc, let platform_enable_lcd to do the timing*/
	if (port_type == LCD_PORT_TYPE_LVDS)
		lvds_init();

#ifdef CONFIG_OF_CONTROL
	fdtdec_enable_lcd();
#else
	platform_enable_lcd();
#endif

	lcd_par.enable_state = 1;

	return 0;
}


int lcd_disable(void)
{
	if (lcd_par.enable_state == 0)
		return 0;

	fdtdec_disable_lcd();

	lcd_par.enable_state = 0;
	return 0;
}

static struct display_ops lcd_ops = {
	.enable = lcd_enable,
	.disable = lcd_disable,
};

int __platform_get_lcd_par(struct owl_lcd *par)
{
	return -1;
}

int platform_get_lcd_par(struct owl_lcd *)
__attribute__((weak, alias("__platform_get_lcd_par")));


struct port_type_param {
	const char *name;
	u32 port_type;
};

struct port_type_param port_types[] = {
	{"rgb", LCD_PORT_TYPE_RGB},
	{"cpu", LCD_PORT_TYPE_CPU},
	{"lvds", LCD_PORT_TYPE_LVDS},
	{"edp", LCD_PORT_TYPE_EDP},
};

u32 string_to_port_type(const char *name)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(port_types); i++) {
		if (!strcmp(port_types[i].name, name))
			return port_types[i].port_type;
	}

	return -1;
}

int get_lcdic_type(u32 port_type, u32 lcd_port_type)
{
	if (port_type == LCD_PORT_TYPE_LVDS && lcd_port_type == LCD_PORT_TYPE_EDP)
		return LCDIC_TYPE_LVDS2EDP;
	else
		return -1;
}

static int lcd_activate_gpio(struct owl_fdt_gpio_state *gpio)
{
	int active_level;

	active_level = (gpio->flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	owl_gpio_generic_direction_output(gpio->chip, gpio->gpio, active_level);
	return 0;
}

static int lcd_deactivate_gpio(struct owl_fdt_gpio_state *gpio)
{
	int active_level;

	active_level = (gpio->flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	owl_gpio_generic_direction_output(gpio->chip, gpio->gpio, !active_level);
	return 0;
}

static void open_backlight(void)
{
	struct pwm_backlight_data *pd;	
	
	mdelay(200);
	
	pd = &lcd_par.pwm_bl_data;	
	if(bl_power){
		pd->power = 1;	
	}else{
		pd->power = 0;	
	}
	owl_pwm_backlight_update_status(pd);
	
	debug("open bl %d\n", pd->power);
}

/*
	if use gpiod28-31, set 1 to cpu_pwr_ctl bit 7-4 first
	GPIOD 28  -------  CPU_PWR_CTL BIT 7    gpio  124
	GPIOD 29  -------  CPU_PWR_CTL BIT 6	gpio  125
	GPIOD 30  -------  CPU_PWR_CTL BIT 5	gpio  126
	GPIOD 31  -------  CPU_PWR_CTL BIT 4	gpio  127
*/
#define MIN_GPIO_TO_PWR  124    
#define MAX_GPIO_TO_PWR  127	
static int gpio_bit_to_pwr(struct owl_fdt_gpio_state *gpio)
{
	int tmp = 0;
	if(gpio->name){
		if((gpio->gpio>=MIN_GPIO_TO_PWR)&&(gpio->gpio<=MAX_GPIO_TO_PWR)){
			tmp |= (1<<(MAX_GPIO_TO_PWR-gpio->gpio));
			tmp <<= 4;
		}	
		debug("get_pwr_ctl %s  %d\n", gpio->name, tmp);		
	}
	return tmp;
}

static int fdtdec_enable_lcd(void)
{
	int ret;
	int tmp;
	int reg_val;

	debug("fdtdec enable lcd\n");

	ret = owl_device_fdtdec_set_pinctrl_default(
		lcd_par.dev_node);
	if (ret)
		return -1;

	tmp = gpio_bit_to_pwr(&lcd_par.lcdpower_gpio);
	tmp |= gpio_bit_to_pwr(&lcd_par.lcdpower2_gpio);
	tmp |= gpio_bit_to_pwr(&lcd_par.lcdreset_gpio);
	tmp |= gpio_bit_to_pwr(&lcd_par.lcdstandby_gpio);
	if(tmp){		
		reg_val = readl(CMU_PWR_CTL);
		reg_val |= tmp;
		writel(reg_val, CMU_PWR_CTL);		
	}

	lcd_activate_gpio(&lcd_par.lcdpower_gpio);
	lcd_activate_gpio(&lcd_par.lcdpower2_gpio);
	//owl_regulator_enable(lcd_par.lcdvcc_regulator);

	if(lcd_par.lcdreset_gpio.name){
		lcd_deactivate_gpio(&lcd_par.lcdreset_gpio);
		mdelay(10);
		lcd_activate_gpio(&lcd_par.lcdreset_gpio);
		debug("reset ok %s\n", lcd_par.lcdreset_gpio.name);
	}

	if(lcd_par.lcdstandby_gpio.name){
		lcd_activate_gpio(&lcd_par.lcdstandby_gpio);
		debug("standby ok %s\n", lcd_par.lcdstandby_gpio.name);
	}
	
	enable_lcdc();
	if (lcd_par.lcdic) {
		mdelay(10);
		lcdi_convertion_enable(lcd_par.lcdic);
	}
	
	open_backlight();

	debug("fdtdec enable lcd ok %d\n", bl_power);

	return 0;
}

static int fdtdec_disable_lcd(void)
{
	struct pwm_backlight_data *pd;
	u32 tmp = 0;
	int lcd_id = 0;
	u32 port_type = lcd_par.port_type;

	pd = &lcd_par.pwm_bl_data;
	pd->power = 0;
	owl_pwm_backlight_update_status(pd);

	lcd_deactivate_gpio(&lcd_par.lcdpower_gpio);
	lcd_deactivate_gpio(&lcd_par.lcdpower2_gpio);
	//owl_regulator_disable(lcd_par.lcdvcc_regulator);

	if (port_type == LCD_PORT_TYPE_LVDS)
		lvds_close();

/******pull down vsync /hsync / dclk pads**********/
/*	act_writel(0xc0, LCDx_TIM0(lcd_id));*/
	tmp = readl(LCDx_TIM0(lcd_id));
	tmp &= (~(0xf<<4));
	tmp |= 0xc0;
	writel(tmp, LCDx_TIM0(lcd_id));

/*****************************************/
	tmp = readl(LCDx_CTL(lcd_id));
	tmp &= (~0x01);
	writel(tmp, LCDx_CTL(lcd_id));

	clrbits_le32(CMU_DEVCLKEN0, (1 << 9));

	lcdi_convertion_disable(lcd_par.lcdic);

	return 0;

}

int fdtdec_get_lcd_par(struct owl_lcd *par)
{
	int dev_node;
	int mode_node;
	const char *type_name;
	int len;
	struct fb_videomode *mode;

	debug("fdtdec_get_lcd_par\n");

	dev_node = fdtdec_next_compatible(
		gd->fdt_blob, 0, COMPAT_ACTIONS_OWL_LCD);
	if (dev_node <= 0) {
		debug("Can't get owl-lcd device node\n");
		return -1;
	}

	par->dev_node = dev_node;

	mode_node = fdtdec_lookup_phandle(
			gd->fdt_blob, dev_node, "videomode-0");

	if (mode_node <= 0) {
		debug("Can't get lcd mode node\n");
		return -1;
	}

	mode = malloc(sizeof(struct fb_videomode));

	mode->refresh = fdtdec_get_int(
			gd->fdt_blob, mode_node, "refresh", -1);
	mode->xres = fdtdec_get_int(
			gd->fdt_blob, mode_node, "xres", -1);
	mode->yres = fdtdec_get_int(
			gd->fdt_blob, mode_node, "yres", -1);
	mode->pixclock = fdtdec_get_int(
			gd->fdt_blob, mode_node, "pixclock", -1);
	mode->left_margin = fdtdec_get_int(
			gd->fdt_blob, mode_node, "left_margin", -1);
	mode->right_margin = fdtdec_get_int(
			gd->fdt_blob, mode_node, "right_margin", -1);
	mode->upper_margin = fdtdec_get_int(
			gd->fdt_blob, mode_node, "upper_margin", -1);
	mode->lower_margin = fdtdec_get_int(
			gd->fdt_blob, mode_node, "lower_margin", -1);
	mode->hsync_len = fdtdec_get_int(
			gd->fdt_blob, mode_node, "hsync_len", -1);
	mode->vsync_len = fdtdec_get_int(
			gd->fdt_blob, mode_node, "vsync_len", -1);
	mode->vmode = fdtdec_get_int(
			gd->fdt_blob, mode_node, "vmode", -1);
	mode->sync = 0;
	mode->flag = 0;

	par->mode = mode;

	type_name = fdt_getprop(
			gd->fdt_blob, dev_node, "port_type", &len);
	par->port_type = string_to_port_type(type_name);

	type_name = fdt_getprop(
			gd->fdt_blob, dev_node, "lcd_port_type", &len);
	if (type_name)
		par->lcd_port_type = string_to_port_type(type_name);
	else
		par->lcd_port_type = -1;

	par->data_width = fdtdec_get_int(
			gd->fdt_blob, dev_node, "data_width", -1);
	par->vsync_inversion = fdtdec_get_int(
			gd->fdt_blob, dev_node, "vsync_inversion", -1);
	par->hsync_inversion = fdtdec_get_int(
			gd->fdt_blob, dev_node, "hsync_inversion", -1);
	par->dclk_inversion = fdtdec_get_int(
			gd->fdt_blob, dev_node, "dclk_inversion", -1);
	par->lde_inversion = fdtdec_get_int(
			gd->fdt_blob, dev_node, "lde_inversion", -1);

	if (par->port_type == LCD_PORT_TYPE_LVDS) {
		par->lvds_ctl = fdtdec_get_int(
				gd->fdt_blob, dev_node, "lvds_ctl", -1);
		par->lvds_alg_ctl0 = fdtdec_get_int(
				gd->fdt_blob, dev_node, "lvds_alg_ctl0", -1);
	}

	owl_fdtdec_decode_gpio(
		gd->fdt_blob, dev_node, "lcd_power_gpios", &par->lcdpower_gpio);

	owl_fdtdec_decode_gpio(
		gd->fdt_blob, dev_node, "lcd_power2_gpios",
			&par->lcdpower2_gpio);
			
	owl_fdtdec_decode_gpio(
		gd->fdt_blob, dev_node, "lcd_reset_gpios",
			&par->lcdreset_gpio);
			
	owl_fdtdec_decode_gpio(
		gd->fdt_blob, dev_node, "lcd_standby_gpios",
			&par->lcdstandby_gpio);
			
	//par->lcdvcc_regulator = fdtdec_owl_regulator_get(gd->fdt_blob, dev_node, "lcdvcc-supply");

	return 0;
}

int owl_lcd_init(void)
{
	int lcdic_type;
	int data_width;

	debug("OWL LCD: lcd init\n");

	lcd_par.lcd_clk_src = LCDCLK_SRC_DEVPLL;

#ifdef CONFIG_OF_CONTROL
	if (fdtdec_get_lcd_par(&lcd_par)) {
		printf("OWL LCD: fdt No lcd par\n");
		return -1;
	}
#else
	if (platform_get_lcd_par(&lcd_par)) {
		printf("OWL LCD: No lcd par\n");
		return -1;
	}
#endif

	lcdic_type = get_lcdic_type(lcd_par.port_type, lcd_par.lcd_port_type);
	if (lcdic_type >= 0) {
		lcd_par.lcdic = lcdi_convertion_get(lcdic_type);
		if (!lcd_par.lcdic)
			return -1;
	}

	if (owl_pwm_backlight_init(&lcd_par.pwm_bl_data)) {
		printf("OWL LCD: backlight not found\n");
		return -1;
	}
	data_width = get_lcd_data_width(&lcd_par);
	owl_display_register(LCD_DISPLAYER,"lcd", &lcd_ops, lcd_par.mode, data_width, 0);
	return 0;
}

void set_lcd_bl_power(bool enable)
{
	bl_power = enable;
	debug("set backlight power %d\n", enable);
}
