/*
 * linux/drivers/video/owl/dss/lcdc.c
 *
 * Copyright (C) 2009 Actions Corporation
 * Author: Hui Wang  <wanghui@actions-semi.com>
 *
 * Some code and ideas taken from drivers/video/owl/ driver
 * by leopard.
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

#define DSS_SUBSYS_NAME "LCDC"

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/fb.h>

#include <mach/hardware.h>

#include <video/owldss.h>

#include "../../dss/dss_features.h"
#include "../../dss/dss.h"
#include "lcdchw.h"

#define POWER_REGULATOR_NAME        "lcdvcc"

struct owl_lcdc_gpio {
    int gpio;
    int active_low;
};

struct owl_videomode {
    u32                     refresh;
    u32                     xres;
    u32                     yres;

    /*in pico second, 0.000 000 000 001s*/
    u32                     pixclock;

    u32                     left_margin;
    u32                     right_margin;
    u32                     upper_margin;
    u32                     lower_margin;
    u32                     hsync_len;
    u32                     vsync_len;

    /*0: FB_VMODE_NONINTERLACED, 1:FB_VMODE_INTERLACED*/
    u32                     vmode;
};

struct lcdc_data {
    struct platform_device  *pdev;
    void __iomem            *base;
    struct owl_dss_device   *dssdev;

    struct clk              *lcdc_clk;
    struct regulator        *lcdc_power;

    /* the followings are strict with DTS */
    struct owl_lcdc_gpio    power_gpio;
	struct owl_lcdc_gpio    reset_gpio;
	struct owl_lcdc_gpio    standby_gpio;

    #define LCD_PORT_TYPE_RGB       0
    #define LCD_PORT_TYPE_CPU       1
    #define LCD_PORT_TYPE_LVDS      2
    #define LCD_PORT_TYPE_EDP       3
    u32                     port_type;

    u32                     data_width;
    u32                     vsync_inversion;
    u32                     hsync_inversion;
    u32                     dclk_inversion;
    u32                     lde_inversion;
    u32                      lightness;
    u32                      saturation;
    u32                      contrast;

	u32                     lvds_ctl;
	u32                     lvds_alg_ctl0;
	
    u32                     num_modes;
    struct owl_videomode   *modes;
    /* end of strict with DTS */

    bool                    lcdc_enabled;
    
    struct mutex            lock;
};

static bool boot_lcd0_inited;

static struct platform_device *lcdc_pdev_map[MAX_NUM_LCD];

inline struct lcdc_data *lcdchw_get_lcdcdrv_data(struct platform_device *lcdcdev)
{
    return dev_get_drvdata(&lcdcdev->dev);
}

inline void lcdchw_write_reg(struct platform_device *lcdcdev,const u16 index, u32 val)
{
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);
    DSSDBG("lcdchw_write_reg  ~~~ %p index %d\n",lcdc->base,index);
    writel(val, lcdc->base + index);
}

inline u32 lcdchw_read_reg(struct platform_device *lcdcdev,const u16 index)
{
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);
    DSSDBG("lcdchw_read_reg  ~~~ %p index %d\n",lcdc->base,index);
    return readl(lcdc->base + index);
}

void lcdchw_dump_regs(struct platform_device *lcdcdev){
#define DUMPREG(r) DSSDBG("%08x ~~ %08x\n",r,lcdchw_read_reg(lcdcdev,r))
     DUMPREG(LCDCHW_CTL);
     DUMPREG(LCDCHW_SIZE);
     DUMPREG(LCDCHW_STATUS);
     DUMPREG(LCDCHW_TIM0);
     DUMPREG(LCDCHW_TIM1);
     DUMPREG(LCDCHW_TIM2);
     DUMPREG(LCDCHW_COLOR);
     DUMPREG(LCDCHW_IMG_XPOS);
     DUMPREG(LCDCHW_IMG_YPOS);
#if 1
     DUMPREG(LCDCHW_LVDS_CTL);
     DUMPREG(LCDCHW_LVDS_ALG_CTL0);
     DUMPREG(LCDCHW_LVDS_DEBUG);
#endif
}

/* TODO */
#if 0
static int lcdchw_set_lcdc_clk(struct platform_device *lcdcdev, bool is_tft,
                               unsigned long pck_req, unsigned long *fck,
                               int *lck_div, int *pck_div)
{
    return 0;
}
#endif

static void lcdchw_set_size(struct platform_device *lcdcdev, u16 width, u16 height)
{
    u32 val;
    
    BUG_ON((width > (1 << 12)) || (height > (1 << 12)));
    
    val = REG_VAL(height - 1, 27, 16) | REG_VAL(width - 1, 11, 0);
    
    lcdchw_write_reg(lcdcdev,LCDCHW_SIZE, val);
}

static void lcdchw_set_preline(struct platform_device *lcdcdev,u16 preline)
{
    u32 val;
        
    val = 0;
    
    val = REG_SET_VAL(val, preline, 12, 8);
    
    val = REG_SET_VAL(val, 1, 13, 13);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_TIM0, val);
}

static void lcdchw_set_default_color(struct platform_device *lcdcdev, u32 color)
{    
    lcdchw_write_reg(lcdcdev,LCDCHW_COLOR, color);
}

static void lcdchw_set_vsync_inv(struct platform_device *lcdcdev, u8 vsync_inv)
{    
    u32 val;
        
    val = lcdchw_read_reg(lcdcdev,LCDCHW_TIM0);
    
    val = REG_SET_VAL(val, vsync_inv, 7, 7);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_TIM0, val);
}
static void lcdchw_set_hsync_inv(struct platform_device *lcdcdev, u8 hsync_inv)
{    
    u32 val;
        
    val = lcdchw_read_reg(lcdcdev,LCDCHW_TIM0);
    
    val = REG_SET_VAL(val, hsync_inv, 6, 6);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_TIM0, val);
}

static void lcdchw_set_dclk_inv(struct platform_device *lcdcdev, u8 dclk_inv)
{    
    u32 val;
        
    val = lcdchw_read_reg(lcdcdev,LCDCHW_TIM0);
    
    val = REG_SET_VAL(val, dclk_inv, 5, 5);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_TIM0, val);
}

static void lcdchw_set_lde_inv(struct platform_device *lcdcdev, u8 led_inv)
{    
    u32 val;
        
    val = lcdchw_read_reg(lcdcdev,LCDCHW_TIM0);
    
    val = REG_SET_VAL(val, led_inv, 4, 4);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_TIM0, val);
}
static void lcdchw_set_timings(struct platform_device *lcdcdev, u16 hbp ,u16 hfp, u16 hsw, u16 vbp ,u16 vfp, u16 vsw)
{    
    u32 val;
    
    BUG_ON((hbp > (1 << 9)) || (hfp > (1 << 9)) || (hsw > (1 << 9)));    
    
    BUG_ON((vbp > (1 << 9)) || (vfp > (1 << 9)) || (vsw > (1 << 9)));
       
    val = REG_VAL(hsw - 1, 29, 20) | REG_VAL(hfp - 1, 19, 10) | REG_VAL(hbp - 1, 9, 0) ;
    
    lcdchw_write_reg(lcdcdev,LCDCHW_TIM1, val);
    
    val = REG_VAL(vsw - 1, 29, 20) | REG_VAL(vfp - 1, 19, 10) | REG_VAL(vbp - 1, 9, 0) ;
    
    lcdchw_write_reg(lcdcdev,LCDCHW_TIM2, val);
    
}

static void lcdchw_set_single_fromat(struct platform_device *lcdcdev,u8 format)
{    
    u32 val;
        
    val = lcdchw_read_reg(lcdcdev,LCDCHW_CTL);
    
    val = REG_SET_VAL(val, format, 12, 10);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_CTL, val);
}

static void lcdchw_set_all_pad_pulldown(struct platform_device *lcdcdev,bool pull_down)
{    
    u32 val;
        
    val = lcdchw_read_reg(lcdcdev,LCDCHW_CTL);
    
    val = REG_SET_VAL(val, pull_down, 20, 20);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_CTL, val);
}

static void lcdchw_set_rb_swap(struct platform_device *lcdcdev, bool rb_swap)
{    
    u32 val;
        
    val = lcdchw_read_reg(lcdcdev,LCDCHW_CTL);
    
    val = REG_SET_VAL(val, rb_swap, 1, 1);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_CTL, val);
}

static void lcdchw_set_data_width(struct platform_device *lcdcdev, int data_width)
{    
    u32 val;

    val = lcdchw_read_reg(lcdcdev,LCDCHW_CTL);
    
    val = REG_SET_VAL(val, data_width, 18, 16);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_CTL, val);
}

static void lcdchw_set_single_from(struct platform_device *lcdcdev, u8 single)
{    
    u32 val;
        
    val = lcdchw_read_reg(lcdcdev,LCDCHW_CTL);
    
    val = REG_SET_VAL(val, single, 7, 6);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_CTL, val);
}

static void lcdchw_single_enable(struct platform_device *lcdcdev, bool enable)
{    
    u32 val;
        
    val = lcdchw_read_reg(lcdcdev,LCDCHW_CTL);
    
    val = REG_SET_VAL(val, enable, 0, 0);
        
    lcdchw_write_reg(lcdcdev,LCDCHW_CTL, val);
}

static void lcdchw_lvds_port_enable(struct platform_device *lcdcdev, bool enable)
{    
    u32 val;
	struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);
	
    if(enable){        
        val = lcdchw_read_reg(lcdcdev,LCDCHW_LVDS_ALG_CTL0);
               
        val = REG_SET_VAL(val, 3, 31, 30);
       
        val = REG_SET_VAL(val, 3, 5, 4);
        
        lcdchw_write_reg(lcdcdev,LCDCHW_LVDS_ALG_CTL0, lcdc->lvds_alg_ctl0);
        
        val = lcdchw_read_reg(lcdcdev,LCDCHW_LVDS_CTL);
        
        val = REG_SET_VAL(val, enable, 0, 0);
        
        lcdchw_write_reg(lcdcdev,LCDCHW_LVDS_CTL, lcdc->lvds_ctl);
                    
    }else{
        val = lcdchw_read_reg(lcdcdev,LCDCHW_LVDS_ALG_CTL0);
		        
        val = REG_SET_VAL(val, 0, 31, 30);
		   
        val = REG_SET_VAL(val, 0, 5, 4);
        
        lcdchw_write_reg(lcdcdev,LCDCHW_LVDS_ALG_CTL0, val);
        
        val = lcdchw_read_reg(lcdcdev,LCDCHW_LVDS_CTL);
        
        val = REG_SET_VAL(val, enable, 0, 0);
        
        lcdchw_write_reg(lcdcdev,LCDCHW_LVDS_CTL, val);        
    }
    
}

static void lcdchw_display_init_lcdc(struct owl_dss_device *dssdev)
{
    struct platform_device * lcdcdev = lcdc_pdev_map[0];
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);
    struct owl_video_timings * timings = &(dssdev->timings);
    
    BUG_ON(!timings);
    
    lcdchw_set_size(lcdcdev,timings->x_res,timings->y_res);

    lcdchw_set_timings(lcdcdev,timings->hbp ,timings->hfp, timings->hsw, timings->vbp ,timings->vfp, timings->vsw);
    
    lcdchw_set_preline(lcdcdev,8);
    
    if(lcdc->port_type == LCD_PORT_TYPE_RGB){ 
	
		lcdchw_set_all_pad_pulldown(lcdcdev,false);
	
	}    
    
    lcdchw_set_data_width(lcdcdev,lcdc->data_width);

    lcdchw_set_default_color(lcdcdev,0);
    
    lcdchw_set_single_fromat(lcdcdev,0);
    
    lcdchw_set_rb_swap(lcdcdev,0);
    
    lcdchw_set_vsync_inv(lcdcdev,lcdc->vsync_inversion);
    
    lcdchw_set_hsync_inv(lcdcdev,lcdc->hsync_inversion);    
    
    lcdchw_set_dclk_inv(lcdcdev, lcdc->dclk_inversion);

    lcdchw_set_lde_inv(lcdcdev, lcdc->lde_inversion);
    
    lcdchw_set_single_from(lcdcdev,0x02);

}

static int lcdchw_get_clocks(struct platform_device *lcdcdev)
{
#if 0
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);
    struct clk *clk;
    clk = clk_get(&lcdcdev->dev, "lcd");
    if (IS_ERR(clk)) {
        DSSERR("can't get fck\n");
        return PTR_ERR(clk);
    }

    lcdc->lcdc_clk = clk;
#endif 
    return 0;
}

static void lcdchw_put_clocks(struct platform_device *lcdcdev)
{
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);

    if (lcdc->lcdc_clk)
        clk_put(lcdc->lcdc_clk);

}

static void lcdchw_check_boot_lcd0_inited(struct platform_device *lcdcdev) {
    boot_lcd0_inited = lcdchw_read_reg(lcdcdev, LCDCHW_CTL) & 0x1;

    DSSINFO("LCD INITED FROM UBOOT??  %d\n", boot_lcd0_inited);

    return ;
}

static int lcdchw_power_init(struct platform_device *lcdcdev) {
    int ret = 0;
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);

    if (boot_lcd0_inited) {
        if (gpio_is_valid(lcdc->power_gpio.gpio)) {
            gpio_direction_output(lcdc->power_gpio.gpio,
                                  !lcdc->power_gpio.active_low);
        }

        if (lcdc->lcdc_power) {
            ret = regulator_enable(lcdc->lcdc_power);
        }

        lcdc->lcdc_enabled = true;
    } else {
        if (gpio_is_valid(lcdc->power_gpio.gpio)) {
            gpio_direction_output(lcdc->power_gpio.gpio,
                                  lcdc->power_gpio.active_low);
        }
        lcdc->lcdc_enabled = false;
    }

    return ret;
}

static int lcdchw_power_enable(struct platform_device *lcdcdev, bool enable) {
    int ret = 0;
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);

    if (enable) {
        if (gpio_is_valid(lcdc->power_gpio.gpio)) {
            gpio_direction_output(lcdc->power_gpio.gpio,
                                  !lcdc->power_gpio.active_low);
        }

        if (lcdc->lcdc_power) {
            ret = regulator_enable(lcdc->lcdc_power);
        }
		
		if (gpio_is_valid(lcdc->reset_gpio.gpio)){
			printk("reset_gpio  is ok %d\n", lcdc->reset_gpio.active_low);
	        gpio_direction_output(lcdc->reset_gpio.gpio,
                                  lcdc->reset_gpio.active_low);
			mdelay(10);
	        gpio_direction_output(lcdc->reset_gpio.gpio,
                                  !lcdc->reset_gpio.active_low);

		}
		
        if (gpio_is_valid(lcdc->standby_gpio.gpio)) {
            gpio_direction_output(lcdc->standby_gpio.gpio,
                                  !lcdc->standby_gpio.active_low);
        }
    } else {	
        if (gpio_is_valid(lcdc->standby_gpio.gpio)) {
            gpio_direction_output(lcdc->standby_gpio.gpio,
                                  lcdc->standby_gpio.active_low);
        }
		
		if (gpio_is_valid(lcdc->reset_gpio.gpio)){
	        gpio_direction_output(lcdc->reset_gpio.gpio,
                                  lcdc->reset_gpio.active_low);
		}
								  
        if (gpio_is_valid(lcdc->power_gpio.gpio)) {
            gpio_direction_output(lcdc->power_gpio.gpio,
                                  lcdc->power_gpio.active_low);
        }
		
		if (lcdc->lcdc_power) {
            ret = regulator_disable(lcdc->lcdc_power);
        }
    }

    return ret;
}

int owl_lcdc_display_enable(struct owl_dss_device *dssdev)
{
    int r;

    struct platform_device *lcdcdev = lcdc_pdev_map[0] ;
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);
    
    mutex_lock(&lcdc->lock);
    
    if (dssdev->manager == NULL) {
        DSSERR("failed to enable display: no manager\n");
        return -ENODEV;
    }

    r = owl_dss_start_device(dssdev);
    
    if (r) {
        DSSERR("failed to start device\n");
        goto err_start_dev;
    }

    r = dss_mgr_enable(dssdev->manager);
    if (r)
        goto err_mgr_enable;
    
    if (!lcdc->lcdc_enabled) {
        lcdchw_power_enable(lcdcdev, true);

        lcdchw_display_init_lcdc(dssdev);
        
        if(lcdc->port_type == LCD_PORT_TYPE_LVDS){   
			lcdchw_lvds_port_enable(lcdcdev,true);
		}
        
        lcdchw_single_enable(lcdcdev,true);    

        lcdc->lcdc_enabled = true;
    }

   
    mutex_unlock(&lcdc->lock);
    lcdchw_dump_regs(lcdcdev);
    return 0;
    
err_mgr_enable:
err_start_dev:
    mutex_unlock(&lcdc->lock);
    return r;
}

void owl_lcdc_display_disable(struct owl_dss_device *dssdev)
{
    struct platform_device *lcdcdev = lcdc_pdev_map[0] ;
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);
    
    mutex_lock(&lcdc->lock);
	
	if(lcdc->port_type == LCD_PORT_TYPE_LVDS){ 
	
		lcdchw_lvds_port_enable(lcdcdev,false);
	
    }
    lcdchw_single_enable(lcdcdev,false);    
    
    dss_mgr_disable(dssdev->manager);
	 
	lcdchw_power_enable(lcdcdev, false);
	
	if(lcdc->port_type == LCD_PORT_TYPE_RGB){ 
	
		lcdchw_set_all_pad_pulldown(lcdcdev,true);
	
	}
	
    owl_dss_stop_device(dssdev);
    
    lcdc->lcdc_enabled = false;

    mutex_unlock(&lcdc->lock);

    lcdchw_dump_regs(lcdcdev);
}

/* temp, TODO pls fix me */
void owl_lcdc_select_video_timings(struct owl_dss_device *dssdev, u32 num,
                                   struct owl_video_timings *timings)
{
    struct platform_device *lcdcdev = lcdc_pdev_map[0] ;
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);

    struct owl_videomode *mode = &lcdc->modes[num];

    timings->x_res          = mode->xres;
    timings->y_res          = mode->yres;
    timings->pixel_clock    = PICOS2KHZ(mode->pixclock);
    timings->hfp            = mode->left_margin;
    timings->hbp            = mode->right_margin;
    timings->vfp            = mode->upper_margin;
    timings->vbp            = mode->lower_margin;
    timings->hsw            = mode->hsync_len;
    timings->vsw            = mode->vsync_len;
	
	timings->data_width     = 24;
	switch(lcdc->data_width){
		case 0:
		timings->data_width     = 24;
		break;
		case 1:
		timings->data_width     = 18;
		break;
		case 2:
		timings->data_width     = 16;
		break;
	}
    return;
}

int owl_lcdc_get_effect_parameter(struct owl_dss_device *dssdev, enum owl_plane_effect_parameter parameter_id)
{
    struct platform_device *lcdcdev = lcdc_pdev_map[0] ;
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);
    switch(parameter_id){
    	case OWL_DSS_VIDEO_LIGHTNESS:
    		return lcdc->lightness;
    	case OWL_DSS_VIDEO_SATURATION:
    		return lcdc->saturation;
    	case OWL_DSS_VIDEO_CONSTRAST:
    		return lcdc->contrast;
    	default:
    		printk("invalid plane effect parameter \n");
    		return -1;
    }
}

void owl_lcdc_set_effect_parameter(struct owl_dss_device *dssdev,enum owl_plane_effect_parameter parameter_id ,int value)
{
    struct platform_device *lcdcdev = lcdc_pdev_map[0] ;
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(lcdcdev);
     switch(parameter_id){
    	case OWL_DSS_VIDEO_LIGHTNESS:
    		lcdc->lightness = value;
    		break;
    	case OWL_DSS_VIDEO_SATURATION:
    		lcdc->saturation = value;
    		break;
    	case OWL_DSS_VIDEO_CONSTRAST:
    		lcdc->contrast = value;
    		break;
    	case OWL_DSS_DEF_EFFECT:
    		lcdc->lightness = DEF_LIGHTNESS;
			lcdc->saturation = DEF_SATURATION;
			lcdc->contrast = DEF_CONTRAST;
    		break;			
    	default:
    		printk("invalid plane effect parameter parameter_id %d value %d\n",parameter_id,value);
    		break;
    }

}
void owl_lcdc_display_dump()
{
    struct platform_device *lcdcdev = lcdc_pdev_map[0] ;    
    lcdchw_dump_regs(lcdcdev);
}

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

static u32 string_to_port_type(const char *name)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(port_types); i++) {
        if (!strcmp(port_types[i].name, name))
            return port_types[i].port_type;
    }

    return -EINVAL;
}

static int lcdchw_parse_gpio(struct device_node *of_node,
                             const char *propname,
                             struct owl_lcdc_gpio *gpio)
{
    enum of_gpio_flags flags;
    int                gpio_num;

    gpio_num = of_get_named_gpio_flags(of_node, propname, 0, &flags);
    if (gpio_num >= 0) {
        gpio->gpio = gpio_num;
    } else {
        gpio->gpio = -1;
    }

    gpio->active_low = flags & OF_GPIO_ACTIVE_LOW;

    DSSDBG("%s, gpio = %d\n", __func__, gpio->gpio);
    DSSDBG("%s, active low = %d\n", __func__, gpio->active_low);

    return 0;
}

static int lcdchw_parse_params(struct platform_device *pdev,
                               struct lcdc_data *lcdc)
{
    struct device_node      *of_node;
    char                    propname[20];
    const char              *portname;
    int                     mode_num;
    struct device_node      *mode_node;
    struct owl_videomode    *modes;
    int                     i, ret;

    of_node = pdev->dev.of_node;

    /* 
     * power gpio
     */
    lcdchw_parse_gpio(of_node, "lcd_power_gpios", &lcdc->power_gpio);
    if (lcdc->power_gpio.gpio < 0) {
        DSSERR("%s, fail to get lcd power gpio\n", __func__);
    }
	
	 /* 
     * reset gpio
     */
    lcdchw_parse_gpio(of_node, "lcd_reset_gpios", &lcdc->reset_gpio);
    if (lcdc->reset_gpio.gpio < 0) {
        DSSERR("%s, fail to get lcd reset gpio\n", __func__);
    }
	 /* 
     * standby gpio
     */
    lcdchw_parse_gpio(of_node, "lcd_standby_gpios", &lcdc->standby_gpio);
    if (lcdc->standby_gpio.gpio < 0) {
        DSSERR("%s, fail to get lcd standby gpio\n", __func__);
    }

    /* 
     * interface timing
     */
    if (of_property_read_string(of_node, "port_type", &portname)) {
        return -EINVAL;
    }
    DSSDBG("portname = %s\n", portname);

    lcdc->port_type = string_to_port_type(portname);

    if (of_property_read_u32(of_node, "data_width", &lcdc->data_width)) {
        return -EINVAL;
    }
    DSSDBG("data_width = %d\n", lcdc->data_width);

    if (of_property_read_u32(of_node, "vsync_inversion",
                             &lcdc->vsync_inversion)) {
        return -EINVAL;
    }
    DSSDBG("vsync_inversion = %d\n", lcdc->vsync_inversion);

    if (of_property_read_u32(of_node, "hsync_inversion",
                             &lcdc->hsync_inversion)) {
        return -EINVAL;
    }
    DSSDBG("hsync_inversion = %d\n", lcdc->hsync_inversion);

    if (of_property_read_u32(of_node, "dclk_inversion",
                             &lcdc->dclk_inversion)) {
        return -EINVAL;
    }
    DSSDBG("dclk_inversion = %d\n", lcdc->dclk_inversion);

    if (of_property_read_u32(of_node, "lde_inversion",
                             &lcdc->lde_inversion)) {
        return -EINVAL;
    }
    DSSDBG("lde_inversion = %d\n", lcdc->lde_inversion);
	
	if(lcdc->port_type == LCD_PORT_TYPE_LVDS){
	    if (of_property_read_u32(of_node, "lvds_ctl", &lcdc->lvds_ctl)) {
	        lcdc->lvds_ctl = 0x000a9521;
	    } 
	    
	    if (of_property_read_u32(of_node, "lvds_alg_ctl0", &lcdc->lvds_alg_ctl0)) {
	        lcdc->lvds_alg_ctl0 = 0xc141a030;
	    } 
	    printk("lvds_ctl 0x%x lvds_alg_ctl0 0x%x \n",lcdc->lvds_ctl,lcdc->lvds_alg_ctl0);
	}

    if (of_property_read_u32(of_node, "lightness", &lcdc->lightness)) {
        lcdc->lightness = DEF_LIGHTNESS;
    } 
          
    DSSDBG("lightness = %d\n", lcdc->lightness);
        
    if (of_property_read_u32(of_node, "saturation", &lcdc->saturation)) {
        lcdc->saturation = DEF_SATURATION;
    }
    DSSDBG("saturation = %d\n", lcdc->saturation);
    
    if (of_property_read_u32(of_node, "contrast", &lcdc->contrast)) {
        lcdc->contrast = DEF_CONTRAST; 
    }
    DSSDBG("contrast = %d\n", lcdc->contrast); 
    /* 
     * video mode
     */

    /* get  mode number */
    for (mode_num = 0; ; mode_num++) {
        sprintf(propname, "videomode-%d", mode_num);
        DSSDBG("propname = %s\n", propname);

        mode_node = of_parse_phandle(of_node, propname, 0);
        if (!mode_node) {
            break;
        }
    }
    DSSDBG("mode num = %d\n", mode_num);

    /* alloc memory */
    modes = kzalloc(sizeof(struct owl_videomode) * mode_num, GFP_KERNEL);
    if (!modes) {
        return -EINVAL;
    }

    for (i = 0; i < mode_num; i++) {
        struct owl_videomode *vmode;

        sprintf(propname, "videomode-%d", i);
        mode_node = of_parse_phandle(of_node, propname, 0);

        vmode = &modes[i];
        if (of_property_read_u32(mode_node, "refresh", &vmode->refresh)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("refresh = %d\n", vmode->refresh);

        if (of_property_read_u32(mode_node, "xres", &vmode->xres)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("xres = %d\n", vmode->xres);

        if (of_property_read_u32(mode_node, "yres", &vmode->yres)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("yres = %d\n", vmode->yres);

        if (of_property_read_u32(mode_node, "pixclock", &vmode->pixclock)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("pixclock = %d\n", vmode->pixclock);

        if (of_property_read_u32(mode_node, "left_margin", &vmode->left_margin)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("left_margin = %d\n", vmode->left_margin);

        if (of_property_read_u32(mode_node, "right_margin", &vmode->right_margin)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("right_margin = %d\n", vmode->right_margin);

        if (of_property_read_u32(mode_node, "upper_margin", &vmode->upper_margin)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("upper_margin = %d\n", vmode->upper_margin);

        if (of_property_read_u32(mode_node, "lower_margin", &vmode->lower_margin)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("lower_margin = %d\n", vmode->lower_margin);

        if (of_property_read_u32(mode_node, "hsync_len", &vmode->hsync_len)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("hsync_len = %d\n", vmode->hsync_len);

        if (of_property_read_u32(mode_node, "vsync_len", &vmode->vsync_len)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("vsync_len = %d\n", vmode->vsync_len);

        if (of_property_read_u32(mode_node, "vmode", &vmode->vmode)) {
            ret = -EINVAL;
            goto parse_mode_fail;
        }
        DSSDBG("vmode = %d\n", vmode->vmode);
    }

    lcdc->num_modes     = mode_num;
    lcdc->modes         = modes;

    return 0;

parse_mode_fail:
    kfree(modes);
    return ret;
}

static int owl_lcdchw_probe(struct platform_device *pdev)
{
    int r;
    struct resource     *lcdc_mem;
    struct lcdc_data    *lcdc;

    DSSINFO("%s\n", __func__);

    lcdc = kzalloc(sizeof(*lcdc), GFP_KERNEL);
    if (!lcdc) {
        r = -ENOMEM;
        goto err_alloc;
    }

    lcdc->pdev          = pdev;
    lcdc_pdev_map[0]    = pdev;

    dev_set_drvdata(&pdev->dev, lcdc);

    mutex_init(&lcdc->lock);

    r = lcdchw_get_clocks(pdev);
    if (r)
        goto err_get_clk;

    pm_runtime_enable(&pdev->dev);

    lcdc_mem = platform_get_resource(lcdc->pdev, IORESOURCE_MEM, 0);
    
    if (!lcdc_mem) {
        DSSERR("can't get IORESOURCE_MEM DSI\n");
        r = -EINVAL;
        goto err_ioremap;
    }    
    
    lcdc->base = ioremap(lcdc_mem->start, resource_size(lcdc_mem));
    if (!lcdc->base) {
        DSSERR("can't ioremap lcdc \n");
        r = -ENOMEM;
        goto err_ioremap;
    }
    DSSDBG("lcdc->base  ~~~ %p \n",lcdc->base);    

    lcdchw_check_boot_lcd0_inited(pdev);

    r = lcdchw_parse_params(pdev, lcdc);
    if (r) {
        DSSERR("%s, parse lcdc params error\n", __func__);
        goto err_parse_params;
    }

    /*
     * configure gpio
     */
    if (gpio_is_valid(lcdc->power_gpio.gpio)) {
        r = gpio_request(lcdc->power_gpio.gpio, NULL);
        if (r) {
            DSSERR("%s, request power gpio failed\n", __func__);
            goto err_parse_params;
        }
    } 
    if (gpio_is_valid(lcdc->reset_gpio.gpio)) {
        r = gpio_request(lcdc->reset_gpio.gpio, NULL);
        if (r) {
            DSSERR("%s, request reset_gpio failed\n", __func__);
            goto err_parse_params;
        }
    } 
    if (gpio_is_valid(lcdc->standby_gpio.gpio)) {
        r = gpio_request(lcdc->standby_gpio.gpio, NULL);
        if (r) {
            DSSERR("%s, request standby_gpio failed\n", __func__);
            goto err_parse_params;
        }
    } 
	
    lcdc->lcdc_power = regulator_get(&pdev->dev, POWER_REGULATOR_NAME);
    if (IS_ERR(lcdc->lcdc_power)) {
        lcdc->lcdc_power = NULL;
    }
    DSSDBG("%s, lcdc_power: %p\n", __func__, lcdc->lcdc_power);

    lcdchw_power_init(pdev);

    DSSINFO("owl_lcdchw_probe called  ok ~~~~~~~~~~~~~\n");
    return 0;

err_parse_params:
    iounmap(lcdc->base);
err_ioremap:
    pm_runtime_disable(&pdev->dev);
err_get_clk:
    kfree(lcdc);
err_alloc:
    return r;
}

static int owl_lcdchw_remove(struct platform_device *pdev)
{
    struct lcdc_data *lcdc = lcdchw_get_lcdcdrv_data(pdev);

    pm_runtime_disable(&pdev->dev);

    lcdchw_put_clocks(pdev);

    lcdchw_power_enable(pdev, false);

    if (lcdc->lcdc_power != NULL) {
        regulator_put(lcdc->lcdc_power);
        lcdc->lcdc_power = NULL;
    }

    iounmap(lcdc->base);

    kfree(lcdc);

    lcdc->lcdc_enabled = false;
    return 0;
}

static struct of_device_id owl_lcdchw_of_match[] = {
    { .compatible = "actions,owl-lcd", },
    { },
};

static struct platform_driver owl_lcdc_driver = {
    .driver = {
        .name           = "owl_lcdchw",
        .owner          = THIS_MODULE,
        .of_match_table = owl_lcdchw_of_match,
    },
    .probe              = owl_lcdchw_probe,
    .remove             = owl_lcdchw_remove,
};

int owl_lcdc_init_platform(void)
{
    int ret = 0;
    
    ret = platform_driver_register(&owl_lcdc_driver);
    
    if (ret) {
        DSSERR("Failed to initialize lcdc platform driver\n");
        return ret;
    }
    return 0;
}

int owl_lcdc_uninit_platform(void)
{   
    platform_driver_unregister(&owl_lcdc_driver);
    
    return 0;
}
