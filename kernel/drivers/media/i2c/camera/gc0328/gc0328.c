/*
 * gc0328 Camera Driver
 *
 * Copyright (C) 2011 Actions Semiconductor Co.,LTD
 * Wang Xin <wangxin@actions-semi.com>
 *
 * Based on ov227x driver
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <media/v4l2-chip-ident.h>
#include <linux/platform_device.h>
#include <mach/isp-owl.h>
#include "module_diff.h"
#include "../module_comm/module_comm.c"
#ifdef  SELf_DETECT
#include "../module_comm/module_detect.c"
#endif

static int camera_i2c_read(struct i2c_adapter *i2c_adap, unsigned int reg, unsigned int *dest)
{
	unsigned char regs_array[4] = {0, 0, 0, 0};
  unsigned char data_array[4] = {0, 0, 0, 0};
	struct i2c_msg msg;
	int ret = 0;
	regs_array[0] = reg & 0xff;
	
	msg.addr = MODULE_I2C_REAL_ADDRESS;
	msg.flags = 0;
	msg.len   = I2C_REGS_WIDTH;
	msg.buf   = regs_array;
	ret = i2c_transfer(i2c_adap, &msg, 1);
	if (ret < 0) {
		printk("write register %s error %d", CAMERA_MODULE_NAME,ret);
		return ret;
	}


	
	msg.flags = I2C_M_RD;
	msg.len   = I2C_DATA_WIDTH;
	msg.buf   = data_array;	
	ret = i2c_transfer(i2c_adap, &msg, 1);
	if (ret >= 0) {
        ret = 0;
		*dest = data_array[0];
	}
	else {
	    printk("read register%s error %d",CAMERA_MODULE_NAME, ret);
	}
	
	return ret;
}

static int camera_i2c_write(struct i2c_adapter *i2c_adap, unsigned int reg, unsigned int src)
{
	unsigned char regs_array[4] = {0, 0, 0, 0};
    unsigned char data_array[4] = {0, 0, 0, 0};
    unsigned char tran_array[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	struct i2c_msg msg;
	unsigned int data = src;
	int ret,i;
	
	
	regs_array[0] = reg & 0xff;
	data_array[0] = data & 0xff;
	for (i = 0; i < I2C_REGS_WIDTH; i++) {
        tran_array[i] = regs_array[i];
    }

    for (i = I2C_REGS_WIDTH; i < (I2C_REGS_WIDTH + I2C_DATA_WIDTH); i++) {
        tran_array[i] = data_array[i - I2C_REGS_WIDTH];
    }
	
	msg.addr = MODULE_I2C_REAL_ADDRESS;
	msg.flags = 0;
	msg.len   = I2C_REGS_WIDTH + I2C_DATA_WIDTH;
	msg.buf   = tran_array;    
	ret = i2c_transfer(i2c_adap, &msg, 1);
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
	    printk("write register %s error %d",CAMERA_MODULE_NAME, ret);
	}
	
	return ret;	
}

static int camera_write_array(struct i2c_adapter *i2c_adap, const struct regval_list *vals)
{
	while (vals->reg_num != 0xff) {
		int ret = camera_i2c_write(i2c_adap,
							vals->reg_num,
							vals->value);
		if (ret < 0)
			{
			printk("[camera] i2c write error!,i2c address is %x\n",MODULE_I2C_REAL_ADDRESS);
			return ret;
			}
		vals++;
	}
	return 0;
}
static int module_soft_reset(struct i2c_client *client)
{
	int ret = 0;
	unsigned int reg_0xfe;
	printk("in the gc0238 module_soft_reset\n");
	//struct i2c_adapter *i2c_adap = client->adapter;
	ret = camera_i2c_read(client->adapter, 0xfe, &reg_0xfe);
	reg_0xfe |= (0x1<<7);
	ret = camera_i2c_write(client->adapter, 0xfe, reg_0xfe);
	msleep(10);
	return ret;
}
/*

static int module_soft_standby(struct i2c_client *client)
{
	int ret = 0;
#if 0
	unsigned int reg_0xfc;
	unsigned int reg_0xfe;
	unsigned int reg_0xf1;
	unsigned int reg_0xf2;
 
	// softstandy has a problem ,so return
  reg_0xfe = 0x00;
  ret  = camera_i2c_write(client->adapter, 0xfe, reg_0xfe);
    
	ret |= camera_i2c_read(client->adapter, 0xfc, &reg_0xfc);
	reg_0xfc |= (0x1<<0);
	ret = camera_i2c_write(client->adapter, 0xfc, reg_0xfc);
	
	reg_0xf1 = 0x00;
	reg_0xf2 = 0x00;
	ret |= camera_i2c_write(client->adapter, 0xf1, reg_0xf1);
	ret |= camera_i2c_write(client->adapter, 0xf2, reg_0xf2);
#endif
	
	return ret;
}


static int module_normal(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_adapter *i2c_adap = client->adapter;

	unsigned int reg_0xf1;
	unsigned int reg_0xfc;
	unsigned int reg_0xfe;
	int ret = 0;

	reg_0xfe = 0x00;
	ret = camera_i2c_write(i2c_adap, 0xfe, reg_0xfe);
	
	ret = camera_i2c_read(i2c_adap, 0xfc, &reg_0xfc);
	reg_0xfc &= (~(0x1<<0));
	ret  = camera_i2c_write(i2c_adap, 0xfc, reg_0xfc);
	
	reg_0xf1 = 0x07;
	ret |= camera_i2c_write(i2c_adap, 0xf1, reg_0xf1);
	
	return ret;
}
*/
static int module_start_aec(struct v4l2_subdev *sd)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
	int ret = 0;
	
	return ret;
}

static int module_freeze_aec(struct v4l2_subdev *sd)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
	int ret = 0;
	
	return ret;
}

static int module_set_exposure_param(struct v4l2_subdev *sd)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
    int ret = 0;
	
	return ret;
}

int module_save_exposure_param(struct v4l2_subdev *sd)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
	int ret = 0;
	
	return ret;
}

static int module_set_auto_white_balance(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	struct i2c_adapter *i2c_adap = client->adapter;
	int auto_white_balance;
	int ret = 0;
	
	if(ctrl)
		auto_white_balance = ctrl->val;
	else
		auto_white_balance = 1;
  
	if (auto_white_balance < 0 || auto_white_balance > 1) {
		printk("set auto_white_balance over range, auto_white_balance = %d\n", auto_white_balance);
		return -ERANGE;
	}
	
	switch(auto_white_balance)
	{
		case 0:
		    ret = 0;
		    break;
		    
		case 1:	
			ret = camera_write_array(i2c_adap, module_whitebance_auto_regs);
			break;
		
		default:
			break;
	}
	
	priv->auto_white_balance = auto_white_balance;
	if(ctrl)
		ctrl->cur.val = auto_white_balance;  
		

	return ret;
}

static int module_set_white_balance_temperature(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
  struct i2c_adapter *i2c_adap = client->adapter;
  int white_balance_temperature = ctrl->val;
	int ret = 0;

	switch(white_balance_temperature) {
	case V4L2_WHITE_BALANCE_INCANDESCENT: /* 白炽光 */
		ret = camera_write_array(i2c_adap, module_whitebance_incandescent_regs);
		break;
	
	case V4L2_WHITE_BALANCE_FLUORESCENT: /* 荧光灯 */
		ret = camera_write_array(i2c_adap, module_whitebance_fluorescent_regs);
		break;
	
	case V4L2_WHITE_BALANCE_DAYLIGHT: /* 日光 (晴天)*/
		ret = camera_write_array(i2c_adap, module_whitebance_sunny_regs);
		break;
	
	case V4L2_WHITE_BALANCE_CLOUDY: /* 多云 （阴天）*/
		ret = camera_write_array(i2c_adap, module_whitebance_cloudy_regs);
		break;
	
	default:
		return -ERANGE;
	}
	
	priv->auto_white_balance = 0;
	priv->white_balance_temperature = white_balance_temperature;
	ctrl->cur.val = white_balance_temperature;
	
	return ret;
}

#if 0
static int module_set_colorfx(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
    int colorfx = ctrl->val;
	int ret = 0;

	switch (colorfx) {
	case V4L2_COLORFX_NONE: /* normal */
		ret = camera_write_array(client->adapter, module_effect_normal_regs);
		break;
	
	case V4L2_COLORFX_BW: /* black and white */
		ret = camera_write_array(client->adapter, module_effect_white_black_regs);
		break;
	
	case V4L2_COLORFX_SEPIA: /* antique ,复古*/
		ret = camera_write_array(client->adapter, module_effect_antique_regs);
		break;

	case V4L2_COLORFX_NEGATIVE: /* negative，负片 */
		ret = camera_write_array(client->adapter, module_effect_negative_regs);
		break;

	default:
		return -ERANGE;
	}

	priv->colorfx = colorfx;
	ctrl->cur.val = colorfx;

	return 0;
}
#endif
static int module_set_exposure_auto(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	struct i2c_adapter *i2c_adap = client->adapter;
    int exposure_auto;
	int ret = 0;

	if(ctrl)
		exposure_auto = ctrl->val;
	else
		exposure_auto = V4L2_EXPOSURE_AUTO;

	if (exposure_auto < 0 || exposure_auto > 1) {
		return -ERANGE;
	}
  
	switch (exposure_auto) {
	case V4L2_EXPOSURE_AUTO:/*  auto */
        ret = camera_write_array(i2c_adap, module_scene_auto_regs);
		break;
	
	case V4L2_EXPOSURE_MANUAL:/*  auto */
		ret = 0;
		break;
    }

	priv->exposure_auto = exposure_auto;
	if(ctrl)
		ctrl->cur.val = exposure_auto;

	return 0;
}


static int module_set_scene_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	int ret = 0;
#if 0	
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
  int scene_exposure = ctrl->val;
	

	switch(scene_exposure) {
	case V4L2_SCENE_MODE_HOUSE:  //室内
		ret = camera_write_array(client->adapter, module_scene_night_regs);
		break;
	
	case V4L2_SCENE_MODE_SUNLIGHT:  //室外
		ret = camera_write_array(client->adapter, module_scene_auto_regs);
		break;

	default:
		return -ERANGE;
	}

	priv->scene_exposure = scene_exposure;
	ctrl->cur.val = scene_exposure;
#endif
	return ret;

}

/*
static int module_set_prev_capt_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
    int mode = ctrl->val;

	switch(mode) {
	case PREVIEW_MODE:
		priv->prev_capt_mode = mode;
		break;

	case CAPTURE_MODE:
        priv->prev_capt_mode = mode;
		break;
	
	default:
		return -ERANGE;
	}
	
	ctrl->cur.val = mode;
	return 0;
	
}

static int module_pause_af(struct i2c_client *client)
{	
	int ret = 0;
	
	return ret;	
}

static int module_set_af_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
	
    return 0;
}

static int module_get_af_status(struct camera_module_priv *priv, struct v4l2_ctrl *ctrl)
{
//	struct v4l2_subdev *sd = &priv->subdev;
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
    
    return 0;
}




static int module_set_exposure(struct v4l2_subdev *sd)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
	int ret = 0;

	return ret;
}
*/

static int module_set_stream(struct i2c_client *client,int enable)
{
	struct camera_module_priv *priv = to_camera_priv(client);
	int ret = 0;

	
	struct i2c_adapter *i2c_adap = client->adapter;
	GC_INFO("");
	   if (!enable) {
		   GC_INFO("stream down");

		 camera_i2c_write(i2c_adap, 0xfe, 0x00);
	   camera_i2c_write(i2c_adap, 0xf1, 0x70);
	   camera_i2c_write(i2c_adap, 0xf2, 0x00);
		   return ret;
	   }
	
	   if (NULL == priv->win || NULL == priv->cfmt) {
		   GC_ERR("cfmt or win select error");
		   return (-EPERM);
	   }	
	   GC_INFO("stream on");
		
		 camera_i2c_write(i2c_adap, 0xfe, 0x00);
	   camera_i2c_write(i2c_adap, 0xf1, 0x07);
	   camera_i2c_write(i2c_adap, 0xf2, 0x01);
	   
	return 0;
}

static int module_get_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	return ret;
}

static int module_get_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
//  unsigned int analog_gain = 0;
//	unsigned int digital_gain = 0;
	unsigned int total_gain = 0;
//	int ret = 0;
    
	return total_gain;
}

static int module_set_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);

	return 0;
}

static int module_set_mbusformat(struct i2c_client *client, const struct module_color_format *cfmt)
{
	enum v4l2_mbus_pixelcode code;
	unsigned int reg_0x44 = 0;
	
	code = cfmt->code;
	switch (code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
		reg_0x44 = 0x02;
		break;
		
	case V4L2_MBUS_FMT_UYVY8_2X8:
		reg_0x44 = 0x00;
		break;
		
	case V4L2_MBUS_FMT_YVYU8_2X8:
		reg_0x44 = 0x03;
		break;
		
	case V4L2_MBUS_FMT_VYUY8_2X8:
		reg_0x44 = 0x01;
		break;
		
	default:
		printk("[gc0328] %s, %d, mbus code error\n",__FUNCTION__, __LINE__);
		return -1;
	}
	return camera_i2c_write(client->adapter, 0x44, reg_0x44);
}


static int  module_s_mirror_flip(struct v4l2_subdev *sd, unsigned int mirror, unsigned int flip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_adapter *i2c_adap = client->adapter;
	unsigned int reg_0x17 = 0x10;
	unsigned int reg_0xfe = 0x00;
	int ret = 0;
	
	if ((!mirror) && (!flip)) {
		return 0;
	}
	
	ret = camera_i2c_write(i2c_adap, 0xfe, reg_0xfe);

	if (mirror)	{
		reg_0x17 |= 0x1;
	} else {
		reg_0x17 &= (~0x1);
	}
	
	if (flip) {
		reg_0x17 |= (0x1<<0x1);
	} else {
		reg_0x17 &= (~(0x1<<0x1));
	}

	ret |= camera_i2c_write(i2c_adap, 0x17, reg_0x17);
	return ret;
}

static int module_verify_pid(struct i2c_adapter *i2c_adap,struct camera_module_priv 	*priv)
{
	unsigned int  		pid;
	int ret = 0;
	camera_i2c_write(i2c_adap,0xfe,0x00);
    /*
	 * check and show product ID and manufacturer ID
	 */  
	ret = camera_i2c_read(i2c_adap, PID, &pid); 
	switch (pid) 
    {
	case CAMERA_MODULE_PID:
/*		if(priv)
			{
		     priv->model= V4L2_IDENT_GC0328;
			}*/
		printk("[gc0328] Product ID verified %x\n", pid);
		break;
	
	default:
		printk("[gc0328] Product ID error %x\n", pid);
		return -ENODEV;
	}
	
	return ret;
}
#if 0
void module_power_on_sequence(struct device *dev)
{
	enable_module_clk();
	disable_module_power_down(dev);
	enable_module_reset(dev);
}

void module_power_off_sequence(struct device *dev)
{
    enable_module_power_down(dev);
    disable_module_clk();
    disable_module_reset(dev);
}

void module_extra_handle(struct i2c_client *client)
{
//	struct camera_module_priv *priv = to_camera_priv(client);
	
}
#endif

static int  module_set_ev(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	return 0;
}
static int  module_set_af_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	return 0;
}
static int  module_get_af_status(struct camera_module_priv *priv, struct v4l2_ctrl *ctrl)
{
	return 0;
}

static int  module_set_power_line(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	
	return 0;
}

static int  module_get_power_line(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	
	ctrl->val = V4L2_CID_POWER_LINE_FREQUENCY_AUTO;
	return 0;
}