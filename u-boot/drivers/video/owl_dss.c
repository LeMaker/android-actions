/*
 * owl_dss.c - OWL display driver
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

#include <common.h>
#include <malloc.h>

#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/owl_de.h>
#include <asm/io.h>
#include <asm/arch/sps.h>

#include "videomodes.h"
#include <video_fb.h>
#include <owl_dss.h>
#include <owl_lcd.h>
#include <linux/list.h>
#include <linux/fb.h>
#include <asm/arch/owl_afi.h>

#define DISPLAY_LAYER_ID    0

#define LAYER_FAKE_DISABLE  1
DECLARE_GLOBAL_DATA_PTR;

struct fb_addr {
    void *vaddr;        /* Virtual address */
    u32 paddr;      /* 32-bit physical address */
    unsigned int offset;    /* Alignment offset */
};

struct owl_display {
    u32 display_id;
	const char * name;
    int data_width;
    int rotate;
    struct display_ops *ops;
    const struct fb_videomode *mode;
};

static struct owl_display display_list[MAX_NUM_DISP_DEV];
static u32 registered_display_ids;

static int get_memory_size(void)
{
	int index = (readl(0xb0290028) & (0x7 << 8)) >> 8;
	return (((index+1)*256)<<20);
}


/*******************/

static struct lcdi_convertion cur_lcdic = {-1, NULL};
/**********************/

struct owl_dss {
    struct owl_display *display;        /* SEE disp_dev_id */

    u32 bytespp;
    u32 pixfmt;

    struct fb_addr addr;
    u32 mem_size;

    int img_par_required;
    int img_w;
    int img_h;
    int arrange;

    int de_clk_source;

    int enabled;
#ifdef CONFIG_OF_CONTROL
    int node;
#endif
};

static struct owl_dss dss;

#define MAX_OVERLAY_NUM 4

/********display engine hardware functions************************/
#include <asm/arch/owl_de.h>

#define DECLK_SRC_DEVPLL    0
#define DECLK_SRC_DISPLAYPLL    1

#define DECLK1_MAX  300000000
#define DECLK2_MAX  150000000
#define DECLK_TARGET    300000000

static int equal_scale(int image_width, int image_height,
            int scaled_width, int scaled_height,
            int output_width_max,
            int output_height_max,
            int screen_width,   int screen_height,
            u32 *xcor, u32 *ycor,
            u32 *output_w, u32 *output_h,
            u32 *x_off, u32 *y_off,
            u32 *input_w, u32 *input_h)
{
    if (scaled_width > output_width_max) {
        *output_w = output_width_max;

        *input_w = *output_w * image_width / scaled_width;
        *input_w &= 0xfffffffc;
        *x_off = ((image_width - *input_w) >> 1);
        *xcor = 0;

    } else {
        *output_w = scaled_width;
        *input_w = image_width;
        *x_off = 0;
        *xcor = ((screen_width - *output_w) >> 1);
    }

    if (scaled_height > output_height_max) {

        *output_h = output_height_max;

        *input_h = *output_h * image_height / scaled_height;
        *input_h &= 0xfffffffe;
        *y_off = ((image_height - *input_h) >> 1);
        *ycor = 0;

    } else {
        *output_h = scaled_height;
        *input_h = image_height;
        *y_off = 0;
        *ycor = ((screen_height - *output_h) >> 1);
    }

    return 0;
}

static int do_arrange(struct owl_dss * dss,u32 screen_width, u32 screen_height,
            u32 *xcor, u32 *ycor,
            u32 *output_w, u32 *output_h,
            u32 *x_off, u32 *y_off,
            u32 *input_w, u32 *input_h)
{
    int scaled_width;
    int scaled_height;
    int scale_rate;
    int image_width;
    int image_height;
    int output_width_max;
    int output_height_max;
    u32 arrange;

    scale_rate = 100;

    image_width = dss->img_w;
    image_height = dss->img_h;
    arrange = dss->arrange;

    output_width_max = screen_width * scale_rate / 100;
    output_height_max = screen_height * scale_rate / 100;

    if (arrange == ARR_SCALE_ORIGINAL_OR_FIT_SIZE) {
        if (
            (image_width > output_width_max) ||
            (image_height > output_height_max)) {

            arrange = ARR_SCALE_FIT_SIZE;
        } else {
            arrange = ARR_ORIGINAL_SIZE;
        }
    }

    switch (arrange) {
    case ARR_ORIGINAL_SIZE:
        scaled_width = image_width;
        scaled_height = image_height;
        equal_scale(
            image_width, image_height,
            scaled_width, scaled_height,
            output_width_max,
            output_height_max,
            screen_width, screen_height,
            xcor, ycor,
            output_w, output_h,
            x_off, y_off,
            input_w, input_h);
        break;
    case ARR_SCALE_FULL_SIZE:
        *x_off = 0;
        *y_off = 0;
        *input_w =  image_width;
        *input_h = image_height;
        *output_w = output_width_max;
        *output_h = output_height_max;
        *xcor = ((screen_width-output_width_max)>>1);
        *ycor = ((screen_height-output_height_max)>>1);
        break;
    case ARR_SCALE_FIT_SIZE:
        scaled_width = output_width_max;
        scaled_height = scaled_width * image_height / image_width;

        if (scaled_height > output_height_max) {
            scaled_height = output_height_max;
            scaled_width =
                scaled_height * image_width / image_height;
        }

        equal_scale(
            image_width, image_height,
            scaled_width, scaled_height,
            output_width_max,
            output_height_max,
            screen_width, screen_height,
            xcor, ycor,
            output_w, output_h,
            x_off, y_off,
            input_w, input_h);
        break;

    case ARR_ALL_CUSTOM:
    default:
        return -1;
    }

    return 0;
}

void de_hw_reset(void)
{
    u32 tmp;

/****module reset*****************/
    tmp = readl(CMU_DEVRST0);
    tmp &= (~(0x1 << 7));
    writel(tmp, CMU_DEVRST0);

    udelay(50);

    tmp = readl(CMU_DEVRST0);
    tmp |= (0x1 << 7);
    writel(tmp, CMU_DEVRST0);
/***********************/

}

/* 
 * declk livider is not linear for ATM7059TC, ATM7059A and ATM9009A,
 * so we need a table to get divider value,
 * while the index is divder*2, the value is register value,
 * the convertion is:
 *  index   2   3   4   5   6   8   12   16   24
 *  value   0   1   2   3   4   5   6    7    8
 *  div 1  1.5 2  2.5   3   4   6    8    12
 */
#if defined(CONFIG_ATM7059A) || defined(CONFIG_ATM7059TC)
const static int declk_div_table[25] = {
    0, 0, 0, 1, 2, 3, 4,    /* 0 ~ 6 */
    4, 5, 5, 5, 6, 6, 6,    /* 7 ~ 13 */
    6, 7, 7, 7, 7, 7, 7,    /* 14~ 20 */
    8, 8, 8, 8,     /* 21 ~24 */
};
#endif

static void de_hw_clk_config(void)
{
    u32 tmp;
    int target_declk1_hz;
    int target_declk2_hz;
    int nic_rate_hz;
    int source_hz;
    int clk_src_regval;
    u32 declk1_div;
    u32 declk2_div;
    /****de clock set*******************/
    /*de clk1 will be a little lower than nic clk*/
    nic_rate_hz = owl_get_nic_clk_rate();
    debug("nic clk rate = %d\n", target_declk1_hz);

#ifdef DECLK_TARGET
    if (nic_rate_hz < DECLK_TARGET) {
        debug("DE clk > nic clk!!\n");
        debug("DE clk target %dhz\n", DECLK_TARGET);
        debug("nic clk %d\n", nic_rate_hz);
    }
    target_declk1_hz = DECLK_TARGET;
#else

    if (nic_rate_hz > DECLK1_MAX)
        target_declk1_hz = DECLK1_MAX;
    else
        target_declk1_hz = nic_rate_hz - 1;
#endif
    debug("de hw init2\n");

    switch (dss.de_clk_source) {
    case DECLK_SRC_DEVPLL:
        source_hz = owl_get_devpll_rate();
        clk_src_regval = 0x1;
        break;
    case DECLK_SRC_DISPLAYPLL:
        source_hz = owl_get_displaypll_rate();
        clk_src_regval = 0x0;
        break;
    default:
        return -1;
    }
#if defined(CONFIG_ATM7059A) || defined(CONFIG_ATM7059TC)
    /* div is multied by 2 */
    declk1_div = (source_hz * 2 + target_declk1_hz - 1) / target_declk1_hz;
    target_declk1_hz = source_hz * 2 / declk1_div;
#else
    declk1_div = (source_hz + target_declk1_hz - 1) / target_declk1_hz;
    target_declk1_hz = source_hz / declk1_div;
#endif

    target_declk2_hz = target_declk1_hz;
    if (target_declk2_hz > DECLK2_MAX)
        target_declk2_hz = DECLK2_MAX;

#if defined(CONFIG_ATM7059A) || defined(CONFIG_ATM7059TC)
    /* div is multied by 2 */
    declk2_div = (source_hz * 2 + target_declk2_hz - 1) / target_declk2_hz;
    target_declk2_hz = source_hz * 2 / declk2_div;
#else
    declk2_div = (source_hz + target_declk2_hz - 1) / target_declk2_hz;
    target_declk2_hz = source_hz / declk2_div;
#endif

    debug("de hw init3, declk1: %d, declk2: %d\n",
        target_declk1_hz, target_declk2_hz);

#if defined(CONFIG_ATM7059A) || defined(CONFIG_ATM7059TC)
    declk1_div = declk_div_table[declk1_div];
    declk2_div = declk_div_table[declk2_div];
#else
    declk1_div -= 1;
    declk2_div -= 1;
#endif

    debug("declk1 div: %d, declk2 div: %d\n", declk1_div, declk2_div);

    tmp = readl(CMU_DECLK);
    tmp &= (~0x10ff);
    tmp |= ((declk2_div << 4) | (declk1_div) | (clk_src_regval << 12));
    writel(tmp, CMU_DECLK);
}

static void de_hw_layer_config(struct owl_dss * dss,struct owl_display * display ,int layer_id)
{
    u32 tmp;
    u32 addr = 0;
    u32 xres, yres;
    u32 xcor, ycor, output_w, output_h;
    u32 x_off, y_off, input_w, input_h;
    u32 w_factor, h_factor;

    xres = display->mode->xres;
    yres = display->mode->yres;
    
    do_arrange(dss,
        xres, yres, &xcor, &ycor, &output_w, &output_h,
        &x_off, &y_off, &input_w, &input_h);
        
    /***config de const scale parameter regist ***************/
    writel(0x00400000, VIDEOx_SCOEF0(layer_id));
    writel(0xFC3E07FF, VIDEOx_SCOEF1(layer_id));
    writel(0xFA3810FE, VIDEOx_SCOEF2(layer_id));
    writel(0xF9301BFC, VIDEOx_SCOEF3(layer_id));
    writel(0xFA2626FA, VIDEOx_SCOEF4(layer_id));
    writel(0xFC1B30F9, VIDEOx_SCOEF5(layer_id));
    writel(0xFE1038FA, VIDEOx_SCOEF6(layer_id));
    writel(0xFF073EFC, VIDEOx_SCOEF7(layer_id));
    
    /***begin config video_cfg register ***************/
    tmp = readl(VIDEOx_CFG(layer_id));
    
    //config de crittical all ways off 
    tmp &= ~(0x3 << 26);
    tmp |= (0x2 << 26);
    
    //YUV 0~255 format
    tmp |= (1 << 29);
    
    if(display->display_id == TV_CVBS_DISPLAYER || display->display_id == HDMI_DISPLAYER)
    {
		if(display->rotate == 3)
    	{
    		tmp |= (0x3 << 20);
    	}
    }
    
    //config video layer pixfmt
    tmp &= (~VIDEO_CFG_FMT_MASK);
    
    switch (dss->pixfmt) {
         case GDF_16BIT_565RGB:
            tmp |= VIDEO_CFG_FMT(0);
            break;
         case GDF_32BIT_X888RGB:
            tmp |= VIDEO_CFG_FMT(1);
            break;
         default:
             printf("invalid pixel_format\n");
    }
    writel(tmp, VIDEOx_CFG(layer_id));

    /******config image w & h & addr & stride***************/
    addr = dss->addr.paddr + dss->img_w * dss->bytespp * y_off + x_off;
        
    writel(addr, VIDEOx_FB_0(layer_id));
    
    writel(((dss->img_w * dss->bytespp) >> 3), VIDEOx_STR(layer_id));
    
    tmp = ((input_h - 1) << 16) | (input_w - 1);
    writel(tmp, VIDEOx_ISIZE(layer_id));
    
    /******config window w & h coor and factor***************/
    tmp = (ycor << 16) | xcor;
    writel(tmp, VIDEOx_COOR(layer_id));
    
    tmp = ((output_h - 1) << 16) | (output_w - 1);
    writel(tmp, VIDEOx_OSIZE(layer_id));
    
    w_factor = input_w * 8192 / output_w;
    h_factor = input_h * 8192 / output_h;   
    tmp = (h_factor << 16) | w_factor;   
    writel(tmp, VIDEOx_SR(layer_id));
    
    /********config colorkey & global alpha**********************/
    tmp = readl(VIDEOx_BLEND(layer_id));
    tmp |= (VIDEO_BLEND_ALPHA_EN);
    writel(tmp, VIDEOx_BLEND(layer_id));
    writel(VIDEO_ALPHA_A(255), VIDEOx_ALPHA(layer_id));

}

static void de_hw_path_config(struct owl_display * display,int channel_id)
{
    const struct fb_videomode *display_mode;
    u32 tmp;
    u32 is_interlace;
    
    /********config patch size **********************/
    display_mode = display->mode;

    tmp = PATH_SIZE_HEIGHT(display_mode->yres - 1)
        |PATH_SIZE_WIDTH(display_mode->xres - 1);
        
    writel(tmp, PATHx_SIZE(channel_id));

	is_interlace =
		((display_mode->vmode & FB_VMODE_INTERLACED) ? 1 : 0);

    
    /********config patch ctl **********************/
    
    tmp = readl(PATHx_CTL(channel_id));

    tmp &= (~PATH_CTL_RGB_YUV_EN);  

    if (is_interlace)
        tmp |= PATH_CTL_INTERLACE_ENABLE;
    else
        tmp &= (~PATH_CTL_INTERLACE_ENABLE);
    
    if(display->display_id == TV_CVBS_DISPLAYER)
    {
         tmp |= PATH_CTL_RGB_YUV_EN;
    }
        
    writel(tmp, PATHx_CTL(channel_id));

    /***********config dither**************************************/

    if((display->display_id == LCD_DISPLAYER 
        || display->display_id == DSI_DISPLAYER)
        &&  channel_id == 0){
        tmp = readl(PATH0_DITHER);
#if defined(CONFIG_ATM7059A)
        tmp &= (~(3));
        if(display->data_width==18){
            tmp |= 0x01;
        }else if(display->data_width==16){
            tmp |= 0x03;
        }
#else if defined(CONFIG_ATM7059TC)
        tmp &= (~(7<<12));
        tmp &= (~(1<<8));
        if(display->data_width==18){
            tmp |= (7<<12);
            tmp |= (1<<8);
        }else if(display->data_width==16){
            tmp |= (2<<12);
            tmp |= (1<<8);
        }
#endif 
        writel(tmp, PATH0_DITHER);
    }

}

static void de_hw_output_config(struct owl_display * display,int * channel_id,int * layer_id)
{
    u32 tmp;
    u32 path_output_sel;
    switch (display->display_id) {
    case HDMI_DISPLAYER:        
        path_output_sel = 0x2; 
        *channel_id = 1;
        *layer_id  = 3;      
        break;
    case TV_CVBS_DISPLAYER:
        path_output_sel = 0x0;
        *channel_id = 1;
        *layer_id  = 3;
        break;
    case LCD_DISPLAYER:
        path_output_sel = 0x3;
        *channel_id = 0;
        *layer_id  = 0;
        break;
    case LCD1_DISPLAYER:
        path_output_sel = 0x4;
        *channel_id = 1;
        break;
    case DSI_DISPLAYER:
        path_output_sel = 0x1;
        *channel_id = 0;
        *layer_id  = 0;
        break;
    default:
        printf("de: not support yet\n");
        return -1;
    }
	/* if this is default display */
	if(dss.display && dss.display->display_id == display->display_id&&display->display_id!=TV_CVBS_DISPLAYER){
		printf("display->name %s is a default display ,used channel 0 and layer 0 \n",display->name);
		*channel_id = 0;
        *layer_id  = 0;
	}
    tmp = readl(OUTPUT_CTL);
    if (*channel_id == 0) {
        tmp &= (~OUTPUT_CTL_PATH0_SEL_MASK);
        tmp |= (path_output_sel << OUTPUT_CTL_PATH0_SEL_SHIFT);
    } else {
        tmp &= (~OUTPUT_CTL_PATH1_SEL_MASK);
        tmp |= (path_output_sel << OUTPUT_CTL_PATH1_SEL_SHIFT);
    }
    writel(tmp, OUTPUT_CTL);
}
int de_hw_init(void)
{
	int i = 0;
    u32 tmp;
    u32 channel_id = 0;
    u32 layer_id = 0;

    debug("de hw init\n");
    
    /********config de clk ***************/
    de_hw_clk_config();
    
    /***enable de module***************/
    setbits_le32(CMU_DEVCLKEN0, (1 << 8));
    
    for (i = 0; i < MAX_NUM_DISP_DEV; i++) {
        
        if (display_list[i].display_id != 0) {
            
            struct owl_display * display = &display_list[i];
            
            de_hw_output_config(display,&channel_id,&layer_id);
    
            de_hw_layer_config(&dss,display,layer_id);
        
            de_hw_path_config(display,channel_id);
        
            /* enable layer on path */
            tmp = readl(PATHx_CTL(channel_id));
            tmp |= (0x1 << (20 + layer_id));
            writel(tmp, PATHx_CTL(channel_id));
        
            
            /* enable path and set FCR  */
            tmp = readl(PATHx_CTL(channel_id));
            tmp |= (PATH_CTL_PATH_ENABLE | PATH_CTL_FCR);
            writel(tmp, PATHx_CTL(channel_id));   
           
        }
        else
        {
            break;
        }
    }
    /*set noc priority */
    writel(0x0f, 0xb0500108);
    /*DE out standing to 0x1f*/
    writel(0x1f0000, 0xb02e000c);  
    
    /*main display always on path 0 ,we need force open path 0 */
    if((readl(PATHx_CTL(0)) & PATH_CTL_PATH_ENABLE) == 0)
    {
    	writel(readl(PATHx_CTL(0)) | PATH_CTL_PATH_ENABLE |PATH_CTL_FCR, PATHx_CTL(0)); 
    	writel(0x02cf04ff, PATHx_SIZE(0));  
    }
    

    return 0;
}

int de_hw_close(void)
{
    u32 tmp;
    u32 channel_id = 0;

    tmp = readl(PATHx_CTL(channel_id));
    tmp &= (~PATH_CTL_PATH_ENABLE);
    writel(tmp, PATHx_CTL(channel_id));

    clrbits_le32(CMU_DEVCLKEN0, (1 << 8));
    return 0;
}

static struct owl_display *get_display(u32 display_id)
{
    int i;
    for (i = 0; i < MAX_NUM_DISP_DEV; i++) {
        if (display_list[i].display_id == display_id)
            return &display_list[i];
    }

    return NULL;
}
static struct owl_display *get_display_by_name(const char * name)
{
    int i;
    for (i = 0; i < MAX_NUM_DISP_DEV; i++) {
        if (strcmp(display_list[i].name,name) == 0)
            return &display_list[i];
    }

    return NULL;
}

/********display drivers use  ******************************/
int owl_display_register(u32 display_id,const char * name,
        struct display_ops *ops,
        const struct fb_videomode *def_mode,int data_width,int rotate)
{
    int i;
    int find_slot = 0;
    struct owl_display *display;

    printf("OWL VIDEO: display 0x%x register def_mode %p\n", display_id,def_mode);

    for (i = 0; i < MAX_NUM_DISP_DEV; i++) {
        if (display_list[i].display_id == 0) {
            find_slot = 1;
            break;
        }
    }

    if (find_slot) {
        registered_display_ids |= display_id;
        display = &display_list[i];
        display->display_id = display_id;
        display->ops = ops;
        display->mode = def_mode;
        display->data_width = data_width;
        display->rotate = rotate;
		display->name = name;
    } else {
        return -1;
    }
    printf("OWL VIDEO: display 0x%x register ok\n", display_id);

    return 0;
}

/********dss functions operating display******************************/
int disp_enable(void)
{
   int r;  
   int i = 0;
   for (i = 0; i < MAX_NUM_DISP_DEV; i++) {
        if (display_list[i].display_id != 0) {
            struct owl_display * display = &display_list[i];
            if(display->ops->config){
                r = display->ops->config(display->mode);
                if (r){
                   printf(" owl display config mode error %d \n", display->display_id);
                   return -1;
                }
            }
            if(display->ops->enable){
                r = display->ops->enable();
                if (r){
                   printf(" owl display enable error %d \n", display->display_id);
                   return -1;
                }
            }
        }else{
            break;
        }
    } 
    return 0;
}

int disp_disable(void)
{
    int r; 
    int i = 0; 
    for (i = 0; i < MAX_NUM_DISP_DEV; i++) {
        if (display_list[i].display_id != 0) {
            struct owl_display * display = &display_list[i];
           if(display->ops->disable){
                r = display->ops->disable();
                if (r){
                   printf(" owl display disable error %d \n", display->display_id);
                   return -1;
                }
            }
        }else{
            break;
        }
    } 
    return 0;
}

/*************************/

/*************************/
int lcdi_convertion_register(int type,
        struct lcdi_convertion_ops *ops)
{
    debug("OWL VIDEO: lcdic %d register\n", type);

    if (!cur_lcdic.ops) {
        cur_lcdic.type = type;
        cur_lcdic.ops = ops;
    } else {
        return -1;
    }
    debug("OWL VIDEO: lcdic %d register ok\n", type);

    return 0;
}

struct lcdi_convertion *lcdi_convertion_get(int type)
{
    if (cur_lcdic.type == type)
        return &cur_lcdic;
    else
        return NULL;
}


int lcdi_convertion_enable(struct lcdi_convertion *lcdic)
{
    int ret = 0;
    if (lcdic && lcdic->ops && lcdic->ops->enable)
        ret = lcdic->ops->enable(lcdic);

    return ret;
}

int lcdi_convertion_disable(struct lcdi_convertion *lcdic)
{
    int ret = 0;
    if (lcdic && lcdic->ops && lcdic->ops->disable)
        ret = lcdic->ops->disable(lcdic);

    return ret;
}
/*****************************************************/

#if DSS_ALLOC_BUF
/*
 * Align to 64-bit(8-byte), 32-byte, etc.
 */
static int allocate_buf(struct fb_addr *buf, u32 size, u32 bytes_align)
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
    return 0;
}
#else
unsigned int get_fb_heap_size(void)
{
    int node;
    unsigned int size;

    if(get_memory_size() > 512 * 1024 * 1024) {
        node = fdt_path_offset(gd->fdt_blob, "/reserved/normal");
        size = fdtdec_get_int(gd->fdt_blob, node, "fb_heap_size", 0);
    } else {
        node = fdt_path_offset(gd->fdt_blob, "/reserved/tiny");
        size = fdtdec_get_int(gd->fdt_blob, node, "fb_heap_size", 0);
    }
    
    return size * 1024 * 1024;
}

static int allocate_buf(struct fb_addr *buf, u32 size, u32 bytes_align)
{
    uint64_t addr, rsv_size;
    unsigned int  memory_size = get_memory_size();
    
    rsv_size = get_fb_heap_size();
    
    if(memory_size >1024*1024*1024)
    	{
    		memory_size = 768 * 1024 * 1024 ;
    	}
    else if(memory_size > 512 * 1024 * 1024)
    	{
    		memory_size = 760 * 1024 * 1024 ;
    	} 
    addr = memory_size - rsv_size;
    
    debug(
        "allocate buf 1, addr = 0x%llx, rsv_size = 0x%llx\n",
        addr, rsv_size);

    if (size > rsv_size)
        return -1;

    debug("allocate buf 2\n");

    mmu_set_region_dcache_behaviour(addr, size, DCACHE_WRITETHROUGH);

    buf->paddr = (u32)addr;
    buf->vaddr = (void *)buf->paddr;
    buf->offset = 0;

    memset(buf->vaddr, 0, size);
    return 0;
}
#endif
/*
 * Allocate a framebuffer and an Area Descriptor that points to it.  Both
 * are created in the same memory block.  The Area Descriptor is updated to
 * point to the framebuffer memory. Memory is aligned as needed.
 */
static int allocate_fb(struct fb_addr *addr, u32 size)
{
    /*
     * Allocate a memory block that holds the Area Descriptor and the
     * frame buffer right behind it.  To keep the code simple, everything
     * is aligned on a 32-byte address.
     */
    if (allocate_buf(addr, size, 32) < 0)
        return -1;

    return 0;
}

struct arrange_name {
    int id;
    const char *name;
};

static struct arrange_name arrange_name_list[] = {
    {ARR_ORIGINAL_SIZE, "original"},
    {ARR_SCALE_FULL_SIZE, "full"},
    {ARR_SCALE_FIT_SIZE, "fit"},
    {ARR_SCALE_ORIGINAL_OR_FIT_SIZE, "original-or-fit"},
    {ARR_2X_MODE, "2x"},
    {ARR_ALL_CUSTOM, "all-custom"},
};

int string_to_arrange_id(const char *name)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(arrange_name_list); i++) {
        if (!strcmp(arrange_name_list[i].name, name))
            return arrange_name_list[i].id;
    }

    return ARR_SCALE_FULL_SIZE;
}

int owl_dss_enable(void)
{
    debug("DDD owl_dss enable\n");
	if(owl_get_boot_mode() == BOOT_MODE_PRODUCE)
		return 0;
    if((gd->flags & GD_FLG_BL_LOWPOWER))
		set_lcd_bl_power(0);
		
    if (dss.enabled)
        return 0;

    de_hw_init();

    disp_enable();

    dss.enabled = 1;
	
    return 0;
}

int owl_dss_remove(void)
{
    debug("DDD owl_dss remove\n");

    if (!dss.enabled)
        return 0;


    disp_disable();
    de_hw_close();

    dss.enabled = 0;

    return 0;
}

int fdtdec_get_fb_par(struct owl_dss * dss)
{
    int dev_node;
    int subnode;
    const char *arrange_name;
	const char *def_display_name;
    int len;
    int bpp;
    int temp;

    subnode = fdtdec_next_compatible(
        gd->fdt_blob, 0, COMPAT_ACTIONS_OWL_FRAMEBUFFER);
    if (subnode <= 0) {
        printf("Can't get framebuffer device node\n");
        return -1;
    }

    dss->node = subnode; 

	def_display_name = fdt_getprop(
            gd->fdt_blob, subnode, "def_display", &len);
    if (!def_display_name)
        return -1;
    printf("def_display_name %s \n", def_display_name);
	
	dss->display = get_display_by_name(def_display_name);
	
	if(dss->display == NULL)
	{
	    printf("def_display_name  %s not found \n", def_display_name);
	}
	
    bpp = fdtdec_get_int(
            gd->fdt_blob, subnode, "bpp", -1);			
    if (bpp <= 0)
        dss->bytespp = 0;
	
	dss->bytespp = bpp;
	
	printf("bpp %d \n", bpp);
    dss->img_w = fdtdec_get_int(
            gd->fdt_blob, subnode, "xres", -1);
    if (dss->img_w <= 0 &&  dss->display != NULL){
		dss->img_w = dss->display->mode->xres;
	}
	
    printf("get fb dss, img_w = %d\n", dss->img_w);

    dss->img_h = fdtdec_get_int(
            gd->fdt_blob, subnode, "yres", -1);
    if (dss->img_h <= 0 &&  dss->display != NULL)
        dss->img_h =  dss->display->mode->yres;
	
	printf("get fb dss, img_h = %d\n", dss->img_h);
	dss->img_par_required = 1;
	
    arrange_name = fdt_getprop(
            gd->fdt_blob, subnode, "arrange", &len);
    if (arrange_name){
		dss->arrange = string_to_arrange_id(arrange_name);
	}
    if (dss->arrange <= 0){
        dss->arrange = ARR_SCALE_FULL_SIZE; 
	}
    return 0;
}

void phy_write_gamma_ram_idx(int channel_id, unsigned int idx, unsigned int val)
{

    debug("write gamma, idx = 0x%x, val = 0x%x\n", idx, val);

    writel(PATH_GAMMA_IDX_RAM_OP(1) | idx, PATHx_GAMMA_IDX(channel_id));
    writel(val, PATHx_GAMMA_RAM(channel_id));

    while (readl(PATHx_GAMMA_IDX(channel_id)) & PATH_GAMMA_IDX_BUSY)
        ;

}

int write_gamma_ram(int channel_id,
        unsigned int len, unsigned int *gamma_addr)
{
    unsigned int i;

    for(i = 0; i < (len / 4); i++)
         phy_write_gamma_ram_idx(channel_id, i, gamma_addr[i]);

    writel(0x0, PATHx_GAMMA_IDX(channel_id));
    return 0;
}

int owl_dss_gamma_init(int channel_id)
{
    __maybe_unused ulong addr;
    __maybe_unused char *s;

    s = getenv("lcdgamma");

    if (s != NULL) {
        struct gamma_info *info;

        addr = simple_strtoul(s, NULL, 16);
        info = (struct gamma_info *)addr;

        setbits_le32(CMU_DEVCLKEN0, (1 << 8));
        setbits_le32(PATHx_CTL(channel_id), PATH_CTL_GAMMA_ENABLE);
        write_gamma_ram(channel_id,
            CONFIG_SYS_GAMMA_SIZE, info->gamma_table);
        clrbits_le32(CMU_DEVCLKEN0, (1 << 8));
    }

    return 0;
}

int owl_dss_init(struct owl_dss * dss)
{
    int r;
    u32 display_id;

    dss->de_clk_source = DECLK_SRC_DEVPLL;
	
	if(!dss->display){
		if (registered_display_ids & LCD_DISPLAYER)
			display_id = LCD_DISPLAYER;
		else if (registered_display_ids & DSI_DISPLAYER)
			display_id = DSI_DISPLAYER;
		else if (registered_display_ids & HDMI_DISPLAYER)
			display_id = HDMI_DISPLAYER;
	  else if (registered_display_ids & TV_CVBS_DISPLAYER)
			display_id = TV_CVBS_DISPLAYER;
		else
			return -1;
		
		printf("OWL VIDEO: dss init display id = 0x%x\n", display_id);
		
		dss->display = get_display(display_id);
		if (!dss->display)
			return -1;
	}

    switch (dss->bytespp) {
    case 2:
        dss->pixfmt = GDF_16BIT_565RGB;
        break;
    case 4:
        dss->pixfmt = GDF_32BIT_X888RGB;
        break;
    default:		
		dss->pixfmt = GDF_32BIT_X888RGB;
		dss->bytespp = 4;
        break;
    }
	
    dss->mem_size = dss->img_w * dss->img_h * dss->bytespp;

    /* Memory allocation for framebuffer */
    r = allocate_fb(&dss->addr, dss->mem_size);
    if (r) {
        printf("OWL VIDEO: Out of memory\n");
        return -1;
    }
	
    de_hw_reset();

    if ((dss->display->display_id == LCD_DISPLAYER) ||
        (dss->display->display_id == DSI_DISPLAYER)) {
        owl_dss_gamma_init(0);
    }
    return 0;
}

#if defined(CONFIG_ATM7059A)
static void owl_dss_power_on(void)
{
    u32 tmp, i;
    
    printf("OWL VIDEO: power on\n");
    
    /* 
    * 1. assert reset
    */
    tmp = readl(CMU_DEVRST0);
    tmp &= (~(0x1 << 7));       /* DE */
    tmp &= (~(0x1 << 8));       /* LCD */
    tmp &= (~(0x1 << 10));      /* DSI */
    writel(tmp, CMU_DEVRST0);
    
    tmp = readl(CMU_DEVRST1);
    tmp &= (~(0x1 << 2));       /* HDMI */
    tmp &= (~(0x1 << 1));       /* CVBS */
    writel(tmp, CMU_DEVRST1);
    
    /* 
    * 2. power on
    */
    tmp = readl(SPS_PG_CTL);
    tmp |= (0x1 << 9);      /* PWR_DS */
    writel(tmp, SPS_PG_CTL);
    
    /* wait power on */
    i = 1000;
    while (i-- && (readl(SPS_PG_CTL) & (0x1 << 13)) == 0) {
        /* bit 13 is DS ACK */
        mdelay(1);
    }
    printf("OWL VIDEO: wait res %d, %x\n", i, readl(SPS_PG_CTL));
    
    
    /* 
    * 3. clk enable
    */
    tmp = readl(CMU_DEVCLKEN0);
    tmp |= (0x1 << 9);      /* LCD */
    tmp |= (0x1 << 8);      /* DE */
    tmp |= (0x1 << 12);     /* DSI */
    writel(tmp, CMU_DEVCLKEN0);
    
    tmp = readl(CMU_DEVCLKEN1);
    tmp |= (0x1 << 3);      /* HDMI */
    tmp |= (0x1 << 0);      /* cvbs */
    writel(tmp, CMU_DEVCLKEN1);
    
    /* 
    * 4. deassert reset
    */
    tmp = readl(CMU_DEVRST0);
    tmp |= (0x1 << 7);      /* DE */
    tmp |= (0x1 << 8);      /* LCD */
    tmp |= (0x1 << 10);     /* DSI */
    writel(tmp, CMU_DEVRST0);
    
    tmp = readl(CMU_DEVRST1);
    tmp |= (0x1 << 2);      /* HDMI */
    tmp |= (0x1 << 1);      /* cvbs */
    writel(tmp, CMU_DEVRST1);


}
#endif

int board_video_skip(void)
{
	if(owl_get_boot_mode() == BOOT_MODE_PRODUCE)
		return 1;   
	return 0;
}

void *video_hw_init(void)
{
#ifdef CONFIG_UPGRADE_SPL
	return NULL;
#else
    static GraphicDevice ctfb;
    const char *options;
    unsigned int depth, freq, xres, yres;

    printf("OWL VIDEO: video hw init\n");

#if defined(CONFIG_ATM7059A)
    owl_dss_power_on();
#endif
	
#ifdef CONFIG_OWL_DISPLAY_LCD
    owl_lcd_init();
#endif

#ifdef CONFIG_OWL_DISPLAY_DSI
    owl_dsi_init();
#endif

#ifdef CONFIG_OWL_DISPLAY_HDMI
     hdmi_init();
#endif
#ifdef CONFIG_OWL_DISPLAY_CVBS
     cvbs_init();
#endif
	
	fdtdec_get_fb_par(&dss);
	
    if (owl_dss_init(&dss))
        return NULL;

    printf("owl_dss_init ok ,xres %d\n yres %d\n",xres,yres);

    /* fill in Graphic device struct */

    ctfb.winSizeX = dss.img_w;
    ctfb.winSizeY = dss.img_h;

    ctfb.frameAdrs = (unsigned int)dss.addr.vaddr;
    ctfb.plnSizeX = ctfb.winSizeX;
    ctfb.plnSizeY = ctfb.winSizeY;

    ctfb.gdfBytesPP = dss.bytespp;
    ctfb.gdfIndex = dss.pixfmt;

    ctfb.isaBase = 0;
    ctfb.pciBase = 0;
    ctfb.memSize = dss.mem_size;

    /* Cursor Start Address */
    ctfb.dprBase = 0;
    ctfb.vprBase = 0;
    ctfb.cprBase = 0;

    sprintf(ctfb.modeIdent, "%ix%ix%i",
        ctfb.winSizeX, ctfb.winSizeY, depth);

    debug("OWL VIDEO: video hw init OK\n");

    return &ctfb;
#endif
}

/**********************/
struct kernel_reserve_info *kinfo;
unsigned int get_owl_reserved_size(void)
{
    int node;
    unsigned int size;

    if(gd->ram_size > 512 * 1024 * 1024) {
        node = fdt_path_offset(gd->fdt_blob, "/reserved/normal");
    } else {
        node = fdt_path_offset(gd->fdt_blob, "/reserved/tiny");
    }
    
    size = fdtdec_get_int(gd->fdt_blob, node, "fb_heap_size", 0);
    size += fdtdec_get_int(gd->fdt_blob, node, "kinfo_heap_size", 0);

    return size * 1024 * 1024;
}

void kinfo_init(void)
{
    uint64_t addr, size, rsv_size;
    
    rsv_size = get_owl_reserved_size();
    kinfo = (struct kernel_reserve_info *)(gd->ram_size - rsv_size);
    printf("set kinfo addr = %p\n", kinfo);
}
/**********************/