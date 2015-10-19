/*
 * owl_dsi.c - OWL display driver
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
#include <asm/cache.h>
#include <asm/arch/sps.h>

#include <common.h>
#include <malloc.h>

#include <video_fb.h>
#include <owl_dss.h>
#include <owl_dsi.h>
#include <linux/list.h>
#include <linux/fb.h>

DECLARE_GLOBAL_DATA_PTR;

#define PICOS2KHZ(a) (1000000000UL/(a))
#define KHZ2PICOS(a) (1000000000UL/(a))

//#undef debug
//#define debug printf

static struct owl_dsi dsi_par;

static int fdtdec_enable_dsi(void);
static int fdtdec_disable_dsi(void);

struct dma_addr {
	void *vaddr;		/* Virtual address */
	u32 paddr;		/* 32-bit physical address */
	unsigned int offset;	/* Alignment offset */
};
struct dma_addr dsi_addr;
int get_dsi_data_width(struct owl_dsi *par)
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

static int wait_lanes_stop(void)
{
	u32 tmp;
	int cnt = 1000;

	do {
		tmp = readl(DSI_LANE_STA);
		if ((tmp & (1 << 12)) && (tmp & (1 << 5)))
			break;

		udelay(1);
	} while (--cnt);

	if (cnt <= 0) {
		debug("lanes cannot STOP\n");
		return -1;
	}

	return 0;
}

int __platform_enable_dsi(void)
{
	u32 tmp;

	/* get pin */
	tmp = readl(PAD_CTL);
	tmp |= 0x2;
	writel(tmp, PAD_CTL);

	return 0;
}

int platform_enable_dsi(void)
			__attribute__((weak, alias("__platform_enable_dsi")));


int enable_dsi(void)
{
	u32 tmp;

	/* enable dsi */
	tmp = readl(DSI_VIDEO_CFG);
	tmp |= 0x1;
	writel(tmp, DSI_VIDEO_CFG);
	return 0;
}

static int dsi_activate_gpio(struct owl_fdt_gpio_state *gpio)
{
	int active_level;

	active_level = (gpio->flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	owl_gpio_generic_direction_output(gpio->chip, gpio->gpio, active_level);
	return 0;
}

static int dsi_deactivate_gpio(struct owl_fdt_gpio_state *gpio)
{
	int active_level;

	active_level = (gpio->flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	owl_gpio_generic_direction_output(gpio->chip, gpio->gpio, !active_level);
	return 0;
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
		printf("get_pwr_ctl %s  %d\n", gpio->name, tmp);		
	}
	return tmp;
}

void power_enable(void)
{
	
	unsigned int tmp, reg_val;
	debug("start power_enable\n");	

	tmp = readl(SPS_LDO_CTL);
	tmp |= (1<<11);
	writel(tmp, SPS_LDO_CTL);	

	tmp = readl(MFP_CTL1);
	tmp &= 0xffe1c07f;	//
	writel(tmp, MFP_CTL1);//[11:10]=b'11, select LCD_D9	[6:5]=b'11, select LCD_D14,D13
	
	tmp = readl(MFP_CTL2);
	tmp &= 0x98ffffff;	//
	writel(tmp, MFP_CTL2);//		
	
	tmp = gpio_bit_to_pwr(&dsi_par.dsipower_gpio);
	tmp |= gpio_bit_to_pwr(&dsi_par.dsipower2_gpio);
	tmp |= gpio_bit_to_pwr(&dsi_par.dsireset_gpio);
	if(tmp){		
		reg_val = readl(CMU_PWR_CTL);
		reg_val |= tmp;
		writel(reg_val, CMU_PWR_CTL);		
	}

	dsi_activate_gpio(&dsi_par.dsipower_gpio);
	dsi_activate_gpio(&dsi_par.dsipower2_gpio);

	mdelay(250);
	
	if(dsi_par.dsireset_gpio.name){
		dsi_activate_gpio(&dsi_par.dsireset_gpio);
		mdelay(30);
		dsi_deactivate_gpio(&dsi_par.dsireset_gpio);
		mdelay(30);
		dsi_activate_gpio(&dsi_par.dsireset_gpio);
		printf("reset ok %s\n", dsi_par.dsireset_gpio.name);
	}
	
}


static int dsihw_set_dsi_clk(void)
{
	u32 reg_val=0;

	writel(dsi_par.reg_val.cmu_dsipll_clk, CMU_DSICLK);	

	return 0;
}

static void dsihw_phy_config(void)
{	
	int tmp;

	debug("phy set start\n");
	tmp = 0x1300;
	writel(tmp, DSI_CTRL);	

	tmp = dsi_par.reg_val.dsi_phy_t0;
	writel(tmp, DSI_PHY_T0);	

	tmp = dsi_par.reg_val.dsi_phy_t1;
	writel(tmp, DSI_PHY_T1);	

	tmp = dsi_par.reg_val.dsi_phy_t2;
	writel(tmp, DSI_PHY_T2);	

	tmp = dsi_par.reg_val.dsi_phy_ctrl;
	writel(tmp, DSI_PHY_CTRL);	
	
	tmp = 0x688;
	writel(tmp, DSI_PIN_MAP);	

	
	tmp = readl(DSI_PHY_CTRL);
	tmp |= (1<<24);
	writel(tmp, DSI_PHY_CTRL);
	
	mdelay(10);
	tmp = readl(DSI_PHY_CTRL);
	tmp |= ((1<<25)|(1<<28));
	writel(tmp, DSI_PHY_CTRL);
	
	do{
	  tmp=readl(DSI_PHY_CTRL);
	 }while(!(tmp&(1<<31)));
	 
	udelay(100);
	tmp=readl(DSI_PHY_CTRL);
	if(tmp&0x02000000){
		
		printf("ERR : dsi cal fail!!\n");

		tmp = readl(DSI_PHY_CTRL);//disable calibrate
		tmp &= (~(1<<25));
		writel(tmp,DSI_PHY_CTRL);
		
		tmp = readl(DSI_LANE_CTRL); //force clock lane 
		tmp |= ((1<<1)|(1<<4));
		writel(tmp,DSI_LANE_CTRL);
		
		tmp = readl(DSI_PHY_CTRL);   //Select output node 
		tmp |= (3<<2);
		writel(tmp,DSI_PHY_CTRL);
		
		tmp = readl(DSI_PHY_CTRL);  //Disable (D-PHY is 
		tmp &= (~(1<<24));
		writel(tmp,DSI_PHY_CTRL);
		
		tmp = readl(DSI_PHY_CTRL);//Enable (D-PHY is 
		tmp |= (1<<24);
		writel(tmp,DSI_PHY_CTRL);

	}
	
	
	wait_lanes_stop();
	
	mdelay(1);
	writel(0x03, DSI_TR_STA);//Clear LP1 & LP0 Error
	
	tmp = dsi_par.reg_val.dsi_pin_map;
	writel(tmp, DSI_PIN_MAP);
	
	debug("phy set end~~~~\n");   
	
	/*data0 line ,clk line in stop state */
	wait_lanes_stop();

	tmp = readl(DSI_CTRL);
	tmp |= 0x40;
	writel(tmp, DSI_CTRL);	
	/*data0 line ,clk line in stop state */	
	wait_lanes_stop();
}


static void dsihw_video_config(void)
{	
	int tmp;

	tmp = dsi_par.reg_val.dsi_ctrl;
	writel(tmp, DSI_CTRL);	
	
	tmp = dsi_par.reg_val.dsi_size;
	writel(tmp, DSI_SIZE);	
	
	tmp = dsi_par.reg_val.dsi_color;
	writel(tmp, DSI_COLOR);	
	
	tmp = dsi_par.reg_val.dsi_rgbht0;
	writel(tmp, DSI_RGBHT0);	
	
	tmp = dsi_par.reg_val.dsi_rgbht1;
	writel(tmp, DSI_RGBHT1);	
	
	tmp = dsi_par.reg_val.dsi_rgbvt0;
	writel(tmp, DSI_RGBVT0);	
	
	tmp = dsi_par.reg_val.dsi_rgbvt1;
	writel(tmp, DSI_RGBVT1);	
	
	tmp = dsi_par.reg_val.dsi_pack_cfg;
	writel(tmp, DSI_PACK_CFG);	
	
	tmp = dsi_par.reg_val.dsi_pack_header;
	writel(tmp, DSI_PACK_HEADER);	

	tmp = dsi_par.reg_val.dsi_vedio_cfg;
	writel(tmp, DSI_VIDEO_CFG);	
}


void dsihw_send_short_packet(int data_type, int sp_data, int trans_mode)
{	
	int tmp;	
	int cnt = 100;
	debug("send short start\n");

	tmp = readl(DSI_CTRL);
	tmp &= 0xffffefff;
	writel(tmp, DSI_CTRL);	
	
	writel(sp_data, DSI_PACK_HEADER);	
	
	tmp = (data_type << 8) | (trans_mode << 14);
	writel(tmp, DSI_PACK_CFG);	
	mdelay(1);
	
	tmp = readl(DSI_PACK_CFG);
	tmp |= 1 ;
	writel(tmp, DSI_PACK_CFG);
	
	while ((!(readl(DSI_TR_STA) & (1 << 19))) && --cnt)
		udelay(1);
	
	writel(0x80000, DSI_TR_STA);
	debug("send short end\n");

}

static int allocate_buf(struct dma_addr *buf, u32 size, u32 bytes_align)
{
	u32 offset, ssize;
	u32 mask;

	ssize = size + bytes_align;
	buf->vaddr = malloc(ssize);
	if (!buf->vaddr)
		return -1;

	memset(buf->vaddr, 0, ssize);
	mask = bytes_align - 1;
	offset = (u32)buf->vaddr & mask;
	if (offset) {
		buf->offset = bytes_align - offset;
		buf->vaddr += offset;
	} else {
		buf->offset = 0;
	}
	buf->paddr = virt_to_phys(buf->vaddr);
	
	printf("buf->vaddr  0x%p buf->paddr  0x%x\n", buf->vaddr , buf->paddr );
	return 0;
}

void dsihw_send_long_packet(int data_type, int word_cnt, int * send_data, int trans_mode)
{	
	int tmp;
	unsigned long *src_addr;
	int  i;
	int cnt = 100;
	printf("send long start\n");
	tmp = readl(DSI_CTRL);
	tmp &= 0xffffefff;
	writel(tmp, DSI_CTRL);	
#if 1	
	src_addr = dsi_addr.vaddr;			
	for(i = 0; i <= word_cnt / 4; i++)
	{
		*(src_addr + i) = *(send_data + i);
	}
#endif
	flush_dcache_all();
	
	writel(0x00010224, DMA0_MODE);
	writel(0x0, DMA0_CHAINED_CTL);
	writel(0x3, DMA_IRQ_PD0);
	writel(dsi_addr.paddr, DMA0_SOURCE);
	writel(DSI_FIFO_ODAT, DMA0_DESTINATION);
	writel(((word_cnt+3)/4)*4, DMA0_FRAME_LEN);
	writel(0x1, DMA0_FRAME_CNT);
	writel(0x1, DMA0_INT_CTL);
	writel(0x1, DMA0_START);
	
	tmp = word_cnt;
	writel(tmp, DSI_PACK_HEADER);	
	tmp = ((data_type << 8) | 0x40000 | (trans_mode << 14));
	writel(tmp, DSI_PACK_CFG);	
	
	tmp = readl(DSI_PACK_CFG);
	tmp |= 1 ;
	writel(tmp, DSI_PACK_CFG);
	
	while ((!(readl(DSI_TR_STA) & (1 << 19))) && --cnt)
		udelay(1);

	writel(0x80000, DSI_TR_STA);
	printf("send long end\n");

		
}


int dsi_enable(void)
{
	if (dsi_par.enable_state == 1)
		return 0;
	
	power_enable();
	
	dsihw_set_dsi_clk();
	dsihw_phy_config();
	send_cmd();
	dsihw_video_config();


#ifdef CONFIG_OF_CONTROL
	fdtdec_enable_dsi();
#else
	platform_enable_dsi();
#endif

	dsi_par.enable_state = 1;

	debug("%s 2\n", __func__);

	return 0;
}

int dsi_disable(void)
{
	if (dsi_par.enable_state == 0)
		return 0;

#ifdef CONFIG_OF_CONTROL
	fdtdec_disable_dsi();
#endif

	dsi_par.enable_state = 0;

	return 0;
}

static struct display_ops dsi_ops = {
	.enable = dsi_enable,
	.disable = dsi_disable,
};

int __platform_get_dsi_par(struct owl_dsi *par)
{
	return -1;
}

int platform_get_dsi_par(struct owl_dsi *)
__attribute__((weak, alias("__platform_get_dsi_par")));


static int fdtdec_enable_dsi(void)
{
	int ret;
	struct pwm_backlight_data *pd;

	debug("fdtdec enable dsi\n");

	ret = owl_device_fdtdec_set_pinctrl_default(
		dsi_par.dev_node);
	if (ret){
		debug("err:owl_device_fdtdec_set_pinctrl_default\n");
	//	return -1;
	}
	enable_dsi();
	mdelay(300);

	pd = &dsi_par.pwm_bl_data;
	pd->power = 1;
	owl_pwm_backlight_update_status(pd);

	debug("fdtdec enable dsi ok\n");

	return 0;
}

static int fdtdec_disable_dsi(void)
{
	int ret;
	struct pwm_backlight_data *pd;

	pd = &dsi_par.pwm_bl_data;
	pd->power = 0;
	owl_pwm_backlight_update_status(pd);

	dsi_deactivate_gpio(&dsi_par.dsipower_gpio);
	dsi_deactivate_gpio(&dsi_par.dsipower2_gpio);

	clrbits_le32(DSI_PHY_CTRL, (1 << 24));
	clrbits_le32(CMU_DEVCLKEN0, (1 << 12));

	return 0;
}

int fdtdec_get_dsi_par(struct owl_dsi *par)
{
	int dev_node;
	int mode_node;
	int len;
	const char *data_fmt_name;
	struct fb_videomode *mode;

	debug("fdtdec_get_dsi_par0\n");

	dev_node = fdtdec_next_compatible(
		gd->fdt_blob, 0, COMPAT_ACTIONS_OWL_DSI);
	if (dev_node <= 0) {
		debug("Can't get owl-dsi device node\n");
		return -1;
	}

	par->dev_node = dev_node;

	mode_node = fdtdec_lookup_phandle(
			gd->fdt_blob, dev_node, "videomode-0");
	if (mode_node <= 0) {
		debug("Can't get dsi mode node\n");
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
	
	par->data_width = fdtdec_get_int(
		gd->fdt_blob, dev_node, "data_width", -1);

	owl_fdtdec_decode_gpio(
		gd->fdt_blob, dev_node, "dsi_power_gpios",
			&par->dsipower_gpio);
	debug("fdtdec_get_dsi_par 4\n");

	owl_fdtdec_decode_gpio(
		gd->fdt_blob, dev_node, "dsi_power2_gpios",
			&par->dsipower2_gpio);
	debug("fdtdec_get_dsi_par 5\n");
	
	owl_fdtdec_decode_gpio(
		gd->fdt_blob, dev_node, "dsi_reset_gpios",
			&par->dsireset_gpio);
	debug("fdtdec_get_dsi_par 6\n");
	
	mode_node = fdtdec_lookup_phandle(
			gd->fdt_blob, dev_node, "hw-set");
	if (mode_node <= 0) {
		debug("Can't get dsi mode node\n");
		return -1;
	}	
	
	par->reg_val.dsi_ctrl = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_ctrl", -1);
	debug("fdtdec_get_dsi_par dsi_ctrl %x\n", par->reg_val.dsi_ctrl);

	par->reg_val.dsi_size = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_size", -1);
	debug("fdtdec_get_dsi_par dsi_size %x\n", par->reg_val.dsi_size);
	
	par->reg_val.dsi_color = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_color", -1);
	debug("fdtdec_get_dsi_par dsi_color %x\n", par->reg_val.dsi_color);
	
	par->reg_val.dsi_rgbht0 = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_rgbht0", -1);
	debug("fdtdec_get_dsi_par dsi_rgbht0 %x\n", par->reg_val.dsi_rgbht0);
	
	par->reg_val.dsi_rgbht1 = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_rgbht1", -1);
	debug("fdtdec_get_dsi_par dsi_rgbht1 %x\n", par->reg_val.dsi_rgbht1);
	
	par->reg_val.dsi_rgbvt0 = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_rgbvt0", -1);
	debug("fdtdec_get_dsi_par dsi_rgbvt0 %x\n", par->reg_val.dsi_rgbvt0);
	
	par->reg_val.dsi_rgbvt1 = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_rgbvt1", -1);
	debug("fdtdec_get_dsi_par dsi_rgbvt1 %x\n", par->reg_val.dsi_rgbvt1);
	
	par->reg_val.dsi_pack_cfg = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_pack_cfg", -1);
	debug("fdtdec_get_dsi_par dsi_pack_cfg %x\n", par->reg_val.dsi_pack_cfg);
	
	par->reg_val.dsi_pack_header = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_pack_header", -1);
	debug("fdtdec_get_dsi_par dsi_pack_header %x\n", par->reg_val.dsi_pack_header);
	
	par->reg_val.dsi_vedio_cfg = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_vedio_cfg", -1);
	debug("fdtdec_get_dsi_par dsi_vedio_cfg %x\n", par->reg_val.dsi_vedio_cfg);
	
	par->reg_val.dsi_phy_t0 = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_phy_t0", -1);
	debug("fdtdec_get_dsi_par dsi_phy_t0 %x\n", par->reg_val.dsi_phy_t0);
	
	par->reg_val.dsi_phy_t1 = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_phy_t1", -1);
	debug("fdtdec_get_dsi_par dsi_phy_t1 %x\n", par->reg_val.dsi_phy_t1);
	
	par->reg_val.dsi_phy_t2 = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_phy_t2", -1);
	debug("fdtdec_get_dsi_par dsi_phy_t2 %x\n", par->reg_val.dsi_phy_t2);
	
	par->reg_val.dsi_phy_ctrl = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_phy_ctrl", -1);
	debug("fdtdec_get_dsi_par dsi_phy_ctrl %x\n", par->reg_val.dsi_phy_ctrl);
	
	par->reg_val.dsi_pin_map = fdtdec_get_int(
			gd->fdt_blob, mode_node, "dsi_pin_map", -1);
	debug("fdtdec_get_dsi_par dsi_pin_map %x\n", par->reg_val.dsi_pin_map);
	
	par->reg_val.cmu_dsipll_clk = fdtdec_get_int(
			gd->fdt_blob, mode_node, "cmu_dsipll_clk", -1);
	debug("fdtdec_get_dsi_par cmu_dsipll_clk %x\n", par->reg_val.cmu_dsipll_clk);
	
	printf("fdtdec_get_dsi_par ok\n");

	return 0;
}

int owl_dsi_init(void)
{
	int data_width;
	debug("OWL dsi: dsi init\n");
	
	allocate_buf(&dsi_addr, 100, 32);

#ifdef CONFIG_OF_CONTROL
	if (fdtdec_get_dsi_par(&dsi_par)) {
		printf("OWL dsi: fdt No dsi par\n");
		return -1;
	}
#else
	if (platform_get_lcd_par(&dsi_par)) {
		printf("OWL dsi: No dsi par\n");
		return -1;
	}
#endif

	if (owl_pwm_backlight_init(&dsi_par.pwm_bl_data)) {
		printf("OWL dsi: backlight not found\n");
		return -1;
	}
	data_width = get_dsi_data_width(&dsi_par);
	owl_display_register(DSI_DISPLAYER, &dsi_ops, dsi_par.mode, data_width, 0);

	return 0;
}
