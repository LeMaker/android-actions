/*
 * Camera module common Driver
 *
 * Copyright (C) 2011 Actions Semiconductor Co.,LTD
 * Wang Xin <wangxin@actions-semi.com>
 *
 * Based on gc2035 driver
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <media/v4l2-subdev.h>
#include <media/soc_camera.h>
#include <media/v4l2-chip-ident.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <mach/isp-owl.h>
#include "../flashlight/flashlight.h"

static int flash_flag = 0;
#define GPIO_NAME_FLASHLIGHT    "flashlight_gpio"
static int  rear  = 1;
static int dual = 0;
static bool gpio_flash_cfg_exist      = false;


static bool init_regs_writed   = false;


static struct camera_module_priv *to_camera_priv(const struct i2c_client *client)
{
    return container_of(i2c_get_clientdata(client), struct camera_module_priv, subdev);
}


static struct camera_module_win_size *camera_module_select_win(u32 width, u32 height)
{
    
    __u32 diff;
    struct camera_module_win_size *win = NULL;
    int win_num = 0;
    int i = 0;
    int j = 0;

	
	GC_INFO("width:%d, height:%d", width, height);

	
    win_num = ARRAY_SIZE(module_win_list);
    if (win_num == 1) {
        win = module_win_list[0];
        j = 0;
    } else {
        diff = abs(width - module_win_list[0]->width) + abs(height - module_win_list[0]->height);
        win = module_win_list[0];
        j = 0;
        for (i=1; i<win_num; i++) {
            if (diff > abs(width  - module_win_list[i]->width) + abs(height - module_win_list[i]->height)) {
                win = module_win_list[i];
                j = i;
                diff = abs(width  - module_win_list[i]->width) + abs(height - module_win_list[i]->height);
            }
        }
    }

    return win;
}


static int camera_module_get_params(struct i2c_client *client, u32 *width, u32 *height, enum v4l2_mbus_pixelcode code)
{
	 
    struct camera_module_priv *priv = to_camera_priv(client);
    int ret = 0;
    int i = 0;

	GC_INFO("");
    /*
     * select format
     */
    priv->cfmt = NULL;
    for (i = 0; i < ARRAY_SIZE(module_cfmts); i++) {
        if (code == module_cfmts[i].code) {
            priv->cfmt = module_cfmts + i;
            break;
        }
    }

    if (!priv->cfmt) {
        printk("[camera] Unsupported sensor format.\n");
        goto module_get_fmt_error;
    }
	 priv->win = camera_module_select_win(*width, *height);
	*width = priv->win->width;
    *height = priv->win->height;
	GC_INFO("current params: %s %dX%d\n", priv->win->name, *width, *height);
	return ret;
module_get_fmt_error:
module_soft_reset(client);
priv->win = NULL;
priv->cfmt = NULL;
return ret;

}

static int camera_module_set_params(struct i2c_client *client, u32 *width, u32 *height, enum v4l2_mbus_pixelcode code)
{

    struct camera_module_priv *priv= to_camera_priv(client);
    int ret = 0;
    int i = 0;

	GC_INFO("");

    /*
     * select format
     */
    priv->cfmt = NULL;
    for (i = 0; i < ARRAY_SIZE(module_cfmts); i++) {
        if (code == module_cfmts[i].code) {
            priv->cfmt = module_cfmts + i;
            break;
        }
    }

    if (!priv->cfmt) {
        printk("[camera] Unsupported sensor format.\n");
        goto module_set_fmt_error;
    }

    /*
     * select win
     */
    
     
    priv->win = camera_module_select_win(*width, *height);
	//mdelay(20);
	
   // GC_INFO("the window name is %s,size is %d",priv->win->name,priv->win->win_regs_size);
	camera_write_array(client->adapter, priv->win->win_regs);

    ret = module_set_mbusformat(client, priv->cfmt);

    if (ret < 0) {
        printk("[camera] module set mbus format error.\n");
        goto module_set_fmt_error;
    }

    *width = priv->win->width;
    *height = priv->win->height;

    return ret;

module_set_fmt_error:
    module_soft_reset(client);
    priv->win = NULL;
    priv->cfmt = NULL;

    return ret;
}


static int camera_module_set_flash_led_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    //DEBUG_PRINT("");    
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct camera_module_priv *priv = to_camera_priv(client);
    int mode = ctrl->val;

    switch(mode) {
        case V4L2_FLASH_LED_MODE_NONE:
            priv->flash_led_mode = mode;
            flashlight_control(FLASHLIGHT_OFF);
            break;
        
        case V4L2_FLASH_LED_MODE_TORCH:
            priv->flash_led_mode = mode;
            flashlight_control(FLASHLIGHT_TORCH);
            break;
        
        case V4L2_FLASH_LED_MODE_FLASH:
            priv->flash_led_mode = mode;
            break;

        case V4L2_FLASH_LED_MODE_AUTO:
            priv->flash_led_mode = mode;
            break;

        default :
            return -ERANGE;
    }
 
    ctrl->cur.val = mode;

    return 0;
}
/*
 * v4l2_subdev_core_ops function
 */
static int camera_module_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *id)
{
    
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct camera_module_priv *priv = to_camera_priv(client);
	GC_INFO("");
    id->ident    = priv->model;
    id->revision = 0;

    return 0;
}

/*
 * v4l2_ctrl_ops function
 */
static int camera_module_s_ctrl(struct v4l2_ctrl *ctrl)
{
   
    struct camera_module_priv *priv = container_of(ctrl->handler, struct camera_module_priv , hdl);
    struct v4l2_subdev *sd = &priv->subdev;
    int ret = 0;

	if (NULL == priv->win) {
        GC_INFO("skip such ctrl -s %s\n", v4l2_ctrl_get_name(ctrl->id));
        return (0);     // hasn't chose win, no need process anymore
    }
    GC_INFO("s_ctrl- %s\n", v4l2_ctrl_get_name(ctrl->id));
    switch (ctrl->id) {
    case V4L2_CID_GAIN:
        module_set_gain(sd, ctrl);
        break;
    case V4L2_CID_AUTO_WHITE_BALANCE:
        module_set_auto_white_balance(sd, ctrl);
        break;
 
    case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
        module_set_white_balance_temperature(sd, ctrl);
        break;
    case V4L2_CID_FLASH_LED_MODE:
        if(flash_flag)
        	{
        		camera_module_set_flash_led_mode(sd, ctrl);
        	}		
        break;
    case V4L2_CID_FLASH_STROBE:
         if(flash_flag)
        	{
        		flashlight_control(FLASHLIGHT_FLASH);
        	}
        break;

    case V4L2_CID_FLASH_STROBE_STOP:
	   if(flash_flag)
        	{
        		flashlight_control(FLASHLIGHT_OFF);
        	}
        break;
	case V4L2_CID_HFLIP:
		 if(ctrl->val ==1)    	  
						priv->info->flags |=MODULE_FLAG_HFLIP;
				else
						priv->info->flags &= (~MODULE_FLAG_HFLIP);	
				module_s_mirror_flip(sd, (priv->info->flags) & MODULE_FLAG_HFLIP, (priv->info->flags) & MODULE_FLAG_VFLIP);
		break;
    case V4L2_CID_VFLIP:
       if(ctrl->val ==1)     	
    				priv->info->flags |= MODULE_FLAG_VFLIP;
				else
						priv->info->flags &= (~MODULE_FLAG_VFLIP);    
				module_s_mirror_flip(sd, (priv->info->flags) & MODULE_FLAG_HFLIP, (priv->info->flags) & MODULE_FLAG_VFLIP);
        break;
	//hal层初始化时，会写HFlip和Vflip；使用此控制字同时写；避免先后连续操作两次寄存器，而导致模组输出的数据有问题；
	case V4L2_CID_MIRRORFLIP:
	  if((ctrl->val & HFLIP) >0)     	
				priv->info->flags |= MODULE_FLAG_HFLIP;
			else
					priv->info->flags &= (~MODULE_FLAG_HFLIP);
	  if((ctrl->val & VFLIP) >0)     	
				priv->info->flags |= MODULE_FLAG_VFLIP;
			else
					priv->info->flags &= (~MODULE_FLAG_VFLIP);		 
			module_s_mirror_flip(sd, (priv->info->flags) & MODULE_FLAG_HFLIP, (priv->info->flags) & MODULE_FLAG_VFLIP);
			break;
		
	case V4L2_CID_EXPOSURE_AUTO:
		module_set_exposure_auto(sd, ctrl);
		break;
	 case V4L2_CID_SCENE_MODE:
        module_set_scene_exposure(sd, ctrl);
        break;
		
	case V4L2_CID_AF_MODE:
		module_set_af_mode(sd, ctrl);
		break;
		
	case V4L2_CID_AF_STATUS:
		break;	//上层不应该调用此命令字。
	
	case V4L2_CID_COLORFX:
			break;

    case V4L2_CID_EXPOSURE_COMP:
        module_set_ev(sd,ctrl);
        break;
       
	case V4L2_CID_POWER_LINE_FREQUENCY:
		module_set_power_line(sd,ctrl);
		break;
    default:
        return -EINVAL;
    }

    return ret;
}

static int camera_module_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	 struct camera_module_priv *priv = container_of(ctrl->handler, struct camera_module_priv , hdl);
    struct v4l2_subdev *sd = &priv->subdev;

    int ret = 0;

    switch (ctrl->id) {	
	case V4L2_CID_AF_STATUS:				   
		ret = module_get_af_status(priv, ctrl);
		break;								   
    case V4L2_CID_GAIN:
        ret = module_get_gain(sd, ctrl);
        break;

    case V4L2_CID_EXPOSURE:
        ret = module_get_exposure(sd,ctrl);
        break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
    ret = module_get_power_line(sd,ctrl);
    break;

    default:
        return -EINVAL;
    }
    return 0;
}
/*
 * v4l2_subdev_video_ops function
 */
static int camera_module_s_stream(struct v4l2_subdev *sd, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct camera_module_priv *priv = to_camera_priv(client);
    int ret = 0;
	ret = module_set_stream(client,enable);
   if(ACTS_ISP_PREVIEW_MODE == priv->pcv_mode)
   	{
   		GC_INFO("");
			ret |= module_save_exposure_param(sd);
		GC_INFO("");
		
   	}
   else if(ACTS_ISP_CAPTURE_MODE == priv->pcv_mode)
   	{
   		GC_INFO("");
   		 ret |= module_freeze_aec(sd);
		 ret |= module_set_exposure_param(sd);
		 ret |= module_start_aec(sd);
		 msleep(5);
		 GC_INFO("");
   	}
	   printk("format %d, win %s", priv->cfmt->code, priv->win->name);
	return ret;

}

static int camera_module_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
    GC_INFO("");
    a->bounds.left              = 0;
    a->bounds.top               = 0;
    a->bounds.width             = MODULE_MAX_WIDTH;
    a->bounds.height            = MODULE_MAX_HEIGHT;
    a->defrect                  = a->bounds;
    a->type                     = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    a->pixelaspect.numerator    = 1;
    a->pixelaspect.denominator  = 30;

    return 0;
}

static int camera_module_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *vc)
{
    GC_INFO("");

    vc->c.left = 0;
    vc->c.top = 0;
    vc->c.width = MODULE_MAX_WIDTH;
    vc->c.height = MODULE_MAX_HEIGHT;
    vc->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    return 0;
}

static int camera_module_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    GC_INFO("");
    return 0;
}

static int camera_module_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    GC_INFO("");
    return 0;
}

static int camera_module_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
    GC_INFO("index:%d", fsize->index);
//    struct i2c_client *client = v4l2_get_subdevdata(sd);

    if (fsize->index >= ARRAY_SIZE(module_win_list)) {
        return -EINVAL;
    }

    switch (fsize->pixel_format) {
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_YUYV:
        fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
        fsize->discrete.width = module_win_list[fsize->index]->width;
        fsize->discrete.height = module_win_list[fsize->index]->height;
		fsize->reserved[0]     = module_win_list[fsize->index]->capture_only;
        break;

    default:
        printk("[camera] pixel_format(%d) is Unsupported\n", fsize->pixel_format);
        return -EINVAL;
    }

    return 0;
}

static int camera_module_enum_frameintervals(struct v4l2_subdev *sd, struct v4l2_frmivalenum *fival)
{
   
    const struct camera_module_win_size *win_size = camera_module_select_win(fival->width, fival->height);
    unsigned int array_size = sizeof(win_size->frame_rate_array)/sizeof(unsigned int);
	GC_INFO("");
    if (fival->index >= array_size) {
        return -EINVAL;
    }

    if ((win_size->width != fival->width) || (win_size->height != fival->height)) {
        printk("[camera] width(%d) height(%d) is over range.\n", fival->width, fival->height);
        return -EINVAL;
    }

    fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
    fival->discrete.numerator = 1;
    fival->discrete.denominator = (win_size->frame_rate_array)[fival->index];

    return 0;
}

static int camera_module_enum_fmt(struct v4l2_subdev *sd, unsigned int index, enum v4l2_mbus_pixelcode *code)
{
    GC_INFO("");
    if (index >= ARRAY_SIZE(module_cfmts)) {
        return -EINVAL;
    }

    *code = module_cfmts[index].code;
    return 0;
}

static int camera_module_g_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct camera_module_priv *priv = to_camera_priv(client);
    int ret = 0;
    GC_INFO("");
    if (!priv->win || !priv->cfmt) {
        u32 width = MODULE_DEFAULT_WIDTH;
        u32 height = MODULE_DEFAULT_HEIGHT;

        ret = camera_module_get_params(client, &width, &height, V4L2_MBUS_FMT_UYVY8_2X8);
        if (ret < 0) {
            return ret;
        }
    }

    mf->width       = priv->win->width;
    mf->height      = priv->win->height;
    mf->code        = priv->cfmt->code;
    mf->colorspace  = priv->cfmt->colorspace;
    mf->field       = V4L2_FIELD_NONE;
    return ret;
}

static int camera_module_try_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct camera_module_priv *priv = to_camera_priv(client);
    const struct camera_module_win_size *win;
    int i = 0;
	GC_INFO("");
    // select suitable win
    win = camera_module_select_win(mf->width, mf->height);

    mf->width   = win->width;
    mf->height  = win->height;
    mf->field   = V4L2_FIELD_NONE;

    for (i = 0; i < ARRAY_SIZE(module_cfmts); i++) {
        if (mf->code == module_cfmts[i].code) {
            break;
        }
    }

    if (i == ARRAY_SIZE(module_cfmts)) {
        /* Unsupported format requested. Propose either */
        if (priv->cfmt) {
            /* the current one or */
            mf->colorspace = priv->cfmt->colorspace;
            mf->code = priv->cfmt->code;
        } else {
            /* the default one */
            mf->colorspace = module_cfmts[0].colorspace;
            mf->code = module_cfmts[0].code;
        }
    } else {
        /* Also return the colorspace */
        mf->colorspace    = module_cfmts[i].colorspace;
    }

    return 0;
}

static int camera_module_s_fmt(struct v4l2_subdev *sd,    struct v4l2_mbus_framefmt *mf)
{
   
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct camera_module_priv *priv = to_camera_priv(client);
    int ret = 0;
	GC_INFO("");
    ret = camera_module_set_params(client, &mf->width, &mf->height, mf->code);
    if (!ret) {
        mf->colorspace = priv->cfmt->colorspace;
    }
  
    return ret;
}

static int camera_module_g_mbus_config(struct v4l2_subdev *sd, struct v4l2_mbus_config *cfg)
{
 
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct soc_camera_subdev_desc *desc = soc_camera_i2c_to_desc(client);

    GC_INFO("");
    cfg->flags =  DEFAULT_PCLK_SAMPLE_EDGE | V4L2_MBUS_MASTER |
        V4L2_MBUS_HSYNC_ACTIVE_HIGH | DEFAULT_VSYNC_ACTIVE_LEVEL |
        V4L2_MBUS_DATA_ACTIVE_HIGH;

    cfg->type = V4L2_MBUS_PARALLEL;

    cfg->flags = soc_camera_apply_board_flags(desc, cfg);
    return (0);
}

static int camera_module_s_mbus_config(struct v4l2_subdev *sd, const struct v4l2_mbus_config *cfg)
{
    return 0;
}

static long camera_module_ioctrl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct camera_module_priv *priv = to_camera_priv(client);

    GC_INFO("cmd:%d", cmd);
    switch (cmd) {
    case V4L2_CID_CAM_CV_MODE: {
            int mode = *(int *) arg;

            if (mode < ACTS_ISP_PREVIEW_MODE || mode > ACTS_ISP_VIDEO_MODE) {
                return (-EINVAL);
            }
            priv->pcv_mode = mode;
            break;
        }
    default:
        GC_ERR("Don't support current cmd:0x%x", cmd);
        return (-EINVAL);
    }

    switch (priv->pcv_mode) {
    case ACTS_ISP_PREVIEW_MODE:
        GC_INFO("Preview");
        break;
    case ACTS_ISP_CAPTURE_MODE:
        GC_INFO("Capture");
        break;
    case ACTS_ISP_VIDEO_MODE:
        GC_INFO("Video");
        break;
    default:
        GC_ERR("out of range");
        break;
    }

    return 0;
}

static int camera_module_s_power(struct v4l2_subdev *sd, int on)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct soc_camera_subdev_desc *desc = soc_camera_i2c_to_desc(client);
    int ret = 0;

    GC_INFO("%s", on ? "on" : "off");

    if (!on) {
        return soc_camera_power_off(&client->dev, desc);
    }

    ret = soc_camera_power_on(&client->dev, desc);
    if (ret < 0)
        return ret;
	mdelay(10);
	module_soft_reset(client);
	if(!init_regs_writed)
		{
		
    	ret = camera_write_array(client->adapter, module_init_regs);
		}
	init_regs_writed = false;
    return (ret);
}

static int camera_module_g_skip_frames(struct v4l2_subdev *sd, u32 *frames)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct camera_module_priv *priv = to_camera_priv(client);

    if (ACTS_ISP_CAPTURE_MODE == priv->pcv_mode) {
        *frames = DROP_NUM_CAPTURE;
    } else {
        *frames = DROP_NUM_PREVIEW;
    }
    GC_INFO("skip %d frames\n", *frames);
    return 0;
}


static const struct v4l2_ctrl_ops camera_module_ctrl_ops = {
	.g_volatile_ctrl = camera_module_g_volatile_ctrl,
    .s_ctrl  = camera_module_s_ctrl,
};

static struct v4l2_subdev_sensor_ops module_subdev_sensor_ops = {
	.g_skip_frames = camera_module_g_skip_frames,
};


static struct v4l2_subdev_core_ops camera_module_subdev_core_ops = {
    .g_chip_ident    = camera_module_g_chip_ident,
	.ioctl           = camera_module_ioctrl,
	.s_power         = camera_module_s_power,	
};

static struct v4l2_subdev_video_ops camera_module_subdev_video_ops = {
    .s_stream               = camera_module_s_stream,
    .cropcap                = camera_module_cropcap,
    .g_crop                 = camera_module_g_crop,
    .g_parm                 = camera_module_g_parm,
    .s_parm                 = camera_module_s_parm,
    .enum_framesizes        = camera_module_enum_framesizes,
    .enum_frameintervals    = camera_module_enum_frameintervals,
    .enum_mbus_fmt          = camera_module_enum_fmt,
    .g_mbus_fmt             = camera_module_g_fmt,
    .try_mbus_fmt           = camera_module_try_fmt,
    .s_mbus_fmt             = camera_module_s_fmt,
    .g_mbus_config          = camera_module_g_mbus_config,
    .s_mbus_config          = camera_module_s_mbus_config,
};

static struct v4l2_subdev_ops module_subdev_ops = {
    .core   = &camera_module_subdev_core_ops,
    .video  = &camera_module_subdev_video_ops,
    .sensor = &module_subdev_sensor_ops,
};

static void camera_module_priv_init(struct camera_module_priv * priv)
{
    priv->pcv_mode           = ACTS_ISP_PREVIEW_MODE;
    priv->exposure_auto            = 1;
    priv->auto_white_balance       = 1;
	priv->power_line_frequency     = DEFAULT_POWER_LINE_FREQUENCY;
   	priv->power_line_frequency = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
    priv->win = NULL;
	priv->af_status 			   = AF_STATUS_DISABLE;   
	priv->af_mode				   = CONTINUE_AF;		  
	return;
}

static void camera_module_init_ops(struct v4l2_ctrl_handler *hdl, const struct v4l2_ctrl_ops *ops)
{
   
    unsigned int cmd_array_size = ARRAY_SIZE(v4l2_ctl_array);
    unsigned int cmd_menu_array_size = ARRAY_SIZE(v4l2_ctl_array_menu);
	
    struct v4l2_ctrl *ret = NULL;
    unsigned int i = 0;
	unsigned int err = 0;
	GC_INFO("");

    err = v4l2_ctrl_handler_init(hdl, cmd_array_size + cmd_menu_array_size);
    
	for (i = 0; i < cmd_array_size; i++) {
			const struct v4l2_ctl_cmd_info *pctl = v4l2_ctl_array + i;
	
			ret = v4l2_ctrl_new_std(hdl, ops, pctl->id, pctl->min, pctl->max, pctl->step, pctl->def);
	
			if (NULL == ret) {
				GC_ERR("ctr[%d] - id:%d, min:%d, max:%d, step:%d, def:%d", i,
					   pctl->id, pctl->min, pctl->max, pctl->step, pctl->def);
			}
			if ((pctl->id == V4L2_CID_GAIN)	
			 || (pctl->id == V4L2_CID_POWER_LINE_FREQUENCY)
			 || (pctl->id == V4L2_CID_AF_STATUS)		 
			 || (pctl->id == V4L2_CID_EXPOSURE)){ 	 
				if (ret!= NULL){								 
					ret->flags |= V4L2_CTRL_FLAG_VOLATILE;		 
					ret = NULL; 								 
				}													 
			}														 
			
			hdl->error = 0;
		}
	  for (i = 0; i < cmd_menu_array_size; i++) {
        const struct v4l2_ctl_cmd_info_menu *pmenu = v4l2_ctl_array_menu + i;
		if((pmenu->id == V4L2_CID_FLASH_LED_MODE) && (!gpio_flash_cfg_exist))
			{
				continue;
			}

        ret = v4l2_ctrl_new_std_menu(hdl, ops, pmenu->id, pmenu->max, pmenu->mask, pmenu->def);

        if (NULL == ret) {
            GC_ERR("menu[%d] - id:%d, max:%d, mask:%d, def:%d", i,
                   pmenu->id, pmenu->max, pmenu->mask, pmenu->def);
        }
        hdl->error = 0;
    }
	  GC_INFO("");
    return ;
}


static int camera_module_probe(struct i2c_client *client, const struct i2c_device_id *did)
{
	int ret = 0;
    
    struct camera_module_priv  *priv;
    struct soc_camera_subdev_desc *desc = soc_camera_i2c_to_desc(client);
    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	
	struct v4l2_subdev *subdev;

	
	GC_INFO("%s probe start...",CAMERA_MODULE_NAME);
    GC_INFO("flags:0x%x, addr:0x%x, name:%s, irq:0x%x",
            client->flags, client->addr, client->name, client->irq);

    if (NULL == desc) {
        GC_ERR("error: camera module missing soc camera link");
        return -EINVAL;
    }
    if (NULL == desc->drv_priv) {
        GC_ERR("error: no init module_info of camera module");
        return -EINVAL;
    }

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        printk("[camera] I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE_DATA\n");
        return -EIO;
    }

    priv = kzalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        return -ENOMEM;
    }

    camera_module_priv_init(priv);
    priv->info = desc->drv_priv;

    v4l2_i2c_subdev_init(&priv->subdev, client, &module_subdev_ops);
	camera_module_init_ops(&priv->hdl, &camera_module_ctrl_ops);
    priv->subdev.ctrl_handler = &priv->hdl;
	priv->hdl.error = 0;
    if (priv->hdl.error) {
        ret = priv->hdl.error;
        kfree(priv);
        printk("[camera]%s %d, module init error!\n",__FUNCTION__, __LINE__);
        return ret;
    }
	
    subdev = i2c_get_clientdata(client);
	init_regs_writed = true;
	ret = camera_module_s_power(subdev, 1);
	if(ret < 0)
		return ret;
	ret = module_verify_pid(client->adapter,priv);
	ret = camera_module_s_power(subdev, 0);
	if(0 > ret)
		return ret;
    priv->pcv_mode = ACTS_ISP_PREVIEW_MODE;
	ret = v4l2_ctrl_handler_setup(&priv->hdl);
	
    return ret;
}

static int camera_module_remove(struct i2c_client *client)
{
    
    struct camera_module_priv *priv = to_camera_priv(client);
	GC_INFO("");
    v4l2_device_unregister_subdev(&priv->subdev);
    v4l2_ctrl_handler_free(&priv->hdl);
    kfree(priv);


    return 0;
}


DECLARE_DTS_SENSOR_CFG(g_sensor_cfg);

#define CAMERA_COMMON_NAME           "sensor_common"

/* soc_camer_link's hooks */
static int camera_module_power(struct device *dev, int mode)
{
    struct dts_sensor_config *dsc = &g_sensor_cfg;
    int channel = !!dsc->channel;
    int rear_cam = !!dsc->rear;
    int host_id = !!dsc->host;
    int err;
    
    printk(KERN_INFO"isp_sensor_power, mode:%d\n", mode);
    
    if (mode) {
       // dsc->mfp = pinctrl_get_select_default(dsc->dev);
        printk(KERN_INFO"owl_isp_power_on - channel:%d, rear_cam:%d, host_id:%d\n",
                channel, rear_cam, host_id);
        err = owl_isp_power_on(channel, rear_cam, host_id);
		//flash_flag = rear_cam;
		if(dual)
		{
			flash_flag = !rear_cam;
		}
		else
		{
	   		flash_flag = rear_cam;
		}
    } else {
        if (!IS_ERR(dsc->mfp)) {
         //   pinctrl_put(dsc->mfp);
        }
        printk(KERN_INFO"owl_isp_power_off - channel:%d, rear_cam:%d, host_id:%d\n",
                channel, rear_cam, host_id);
        err = owl_isp_power_off(channel, rear_cam, host_id);
    }
    return err;
}

static int camera_module_reset(struct device *dev)
{
    //printk(KERN_INFO"isp_sensor_reset\n");
    int host_id = !!g_sensor_cfg.host;
    owl_isp_reset(dev, host_id);
    return 0;
}


static struct i2c_board_info owl_i2c_camera = {
    I2C_BOARD_INFO(CAMERA_MODULE_NAME, MODULE_I2C_REG_ADDRESS),
};

static struct module_info camera_module_info = {
    .flags            = SENSOR_FLAG_8BIT | SENSOR_FLAG_DVP | SENSOR_FLAG_CHANNEL2,
};

static const unsigned short camera_module_addrs[] = {
    MODULE_I2C_REG_ADDRESS,
    I2C_CLIENT_END,
};


static struct soc_camera_link camera_module_link = {
    .bus_id          = 0,
    .power           = camera_module_power,
    .reset           = camera_module_reset,
    .board_info      = &owl_i2c_camera,
    .i2c_adapter_id  = 1,  //id编号从0开始
    .module_name     = CAMERA_MODULE_NAME,
    .priv            = &camera_module_info,
};

static struct platform_device owl_camera_device = {
    .name           = "soc-camera-pdrv",
    .id             = 0,
    .dev = {
        .platform_data = &camera_module_link,
    },
};

static struct platform_device  two_module_flag_device = {
    .name           = "camera_flag_device",
    .id             = 0,
    .dev = {
        //.platform_data = &camera_module_link,
        //.init_name     = "sens1",
    },
};




static const struct i2c_device_id camera_module_id[] = {
    { CAMERA_MODULE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, camera_module_id);



static struct i2c_driver camera_i2c_driver = {
    .driver = {
        .name         = CAMERA_MODULE_NAME,
    },
    .probe            = camera_module_probe,
    .remove           = camera_module_remove,
    .id_table         = camera_module_id,
};

static ssize_t dualmod_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return (sprintf(buf, "%d [0: front, 1: rear]\n", !!g_sensor_cfg.rear));
}

/* if call it, should before opening the video */
static ssize_t dualmod_store(struct kobject *kobj, struct kobj_attribute *attr,
                             const char *buf, size_t n)
{
    int err = 0;

    if ('1' == buf[0]) {
        if (!g_sensor_cfg.rear) {
            g_sensor_cfg.rear = 1;
        }
    } else if ('0' == buf[0]) {
        if (g_sensor_cfg.rear) {
            g_sensor_cfg.rear = 0;
        }
    } else {
        err = -EINVAL;
    }
    return (err ? err : n);
}
static struct kobj_attribute dualmod_attr = __ATTR(dualmod, 0666, dualmod_show, dualmod_store);

static struct kobject *dual_kobj = NULL;

static int dual_sysfs_init(void)
{
    dual_kobj = kobject_create_and_add("dualcam", NULL);
    if (NULL == dual_kobj) {
        return (-ENOMEM);
    }
    if (sysfs_create_file(dual_kobj, &dualmod_attr.attr)) {
        kobject_put(dual_kobj);
        return (-ENOMEM);
    }
    return (0);
}

static void dual_sysfs_cleanup(void)
{
    if (NULL == dual_kobj) {
        return;
    }
    sysfs_remove_file(dual_kobj, &dualmod_attr.attr);
    kobject_put(dual_kobj);
    return;
}


static int sensor_mod_init(struct soc_camera_link *link, struct platform_device *pdev,
                            struct i2c_driver *idrv)
{
    struct dts_sensor_config *dsc = &g_sensor_cfg;
    int ret = 0;
	struct device_node *fdt_node;
	fdt_node = of_find_compatible_node(NULL, NULL, "flashlight");
	 if (NULL == fdt_node) {
        printk("err: no [flashlight] in dts\n");
		gpio_flash_cfg_exist =false;
        }
	 else
	 	{
	 		gpio_flash_cfg_exist = true;
	 	}

    ret = parse_config_info(link, dsc, CAMERA_COMMON_NAME);
    if (ret) {
        printk(KERN_ERR "fail go get config\n");
        goto err;
    }
	
    pdev->dev.of_node = dsc->dn;
    dsc->dev = &pdev->dev;
	#ifdef  SELf_DETECT
	ret = detect_init();
    if (ret) {
        GC_INFO("module detect init error.");
       goto err;
    }
	
	camera_id = detect_work();

	detect_deinit();
	if(SENSOR_REAR == camera_id)
		{
		dsc->rear = 1;
		camera_module_info.video_devnum = 0;
		}
	else if(SENSOR_FRONT == camera_id)
		{
		dsc->rear = 0;
		camera_module_info.video_devnum = 1;
		}
	else
		return camera_id;
	#endif
	camera_module_info.video_devnum = (rear == 0);
	dsc->rear = rear;
    GC_INFO("install as [%s] camera\n", (dsc->rear ? "REAR" : "FRONT"));
    GC_INFO("i2c adapter[%d], host[%d], channel[%d], bus[%s], output data[%s]\n", 
              dsc->i2c_adapter, dsc->host, dsc->channel,
              V4L2_MBUS_PARALLEL == dsc->bus_type ? "DVP" : "MIPI",
              SENSOR_DATA_TYPE_YUV == dsc->data_type ? "YUV" : "RAW");

    pdev->id = !!dsc->rear;

    printk("sensor_mod_init():platform_device_register: %s.%d,the modules name is %s\n",
            pdev->name, pdev->id,CAMERA_MODULE_NAME);
	 if (dual && (ret = dual_sysfs_init())) {
        printk(KERN_ERR "sysfs init failed\n");
        goto err;
	   ret |= platform_device_register(&two_module_flag_device);
      if (ret) {
               goto err;
        }
    }
    ret = platform_device_register(pdev);
    if (ret) {
        printk(KERN_ERR "fail to register platform\n");
        goto regdev_err;
    }

    printk("sensor_mod_init():i2c_add_driver\n");
    ret = i2c_add_driver(idrv);
    if (ret) {
        printk(KERN_ERR "fail to add i2c driver\n");
        goto regdrv_err;
    }
    return (ret);

  regdrv_err:
    platform_device_unregister(pdev);
  regdev_err:
  err:

    return (ret);
}


/* module function */
static int __init camera_module_init(void)
{
    
	unsigned int ret = 0;
	GC_INFO("");
    ret = sensor_mod_init(&camera_module_link, &owl_camera_device, &camera_i2c_driver);
    return ret;
}
module_init(camera_module_init);

static void  __exit camera_module_exit(void)
{
    GC_INFO("");
	i2c_del_driver(&camera_i2c_driver);
    platform_device_unregister(&owl_camera_device);
	 if (dual) {
        dual_sysfs_cleanup();
    }
}

module_exit(camera_module_exit);
module_param(rear, int, S_IRUSR);
module_param(dual, int, 0);

MODULE_DESCRIPTION("Camera module driver");
MODULE_AUTHOR("Actions-semi");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");

