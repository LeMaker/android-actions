/*
 * XXX Camera Driver
 *
 * Copyright (C) 2013 Actions Semiconductor Co.,LTD
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
#include"../module_comm/camera_chip_ident.h"
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
	
	
	//printk("the i2c address is 0x%x value is 0x%x\n",reg,src);
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
    GC_INFO("");
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
	unsigned int reg_0x01;
	GC_INFO("in the hi708 module_soft_reset\n");
	msleep(10);
	camera_i2c_write(client->adapter,0x03,0x00);
    ret = camera_i2c_read(client->adapter, 0x01, &reg_0x01);
    reg_0x01 |= (0x01 << 0x1);   
    ret |= camera_i2c_write(client->adapter, 0x01, reg_0x01);
    mdelay(1);
    reg_0x01  &= (~(0x01 << 0x1));
    ret |= camera_i2c_write(client->adapter, 0x01, reg_0x01);
	msleep(10);
	return ret;
}

#if 0
static int module_normal(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int reg_0x03 = 0x00;
	unsigned int reg_0x01;
	int ret;
	
	GC_INFO("");
	ret = camera_i2c_write(client->adapter, 0x03, reg_0x03);
	ret |= camera_i2c_read(client->adapter, 0x01, &reg_0x01);
	reg_0x01 &= 0xfe;   //0位置0
	ret |= camera_i2c_write(client->adapter, 0x01, reg_0x01);
	
    return ret;
}
#endif

static int  module_start_aec(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_adapter *i2c_adap = client->adapter;
	int ret = 0;

	unsigned int reg_0x03 = 0x20;								 
	unsigned int reg_0x10 = 0x00;	

		GC_INFO("");
	ret = camera_i2c_write(i2c_adap, 0x03, reg_0x03); //page 20	 
	ret |= camera_i2c_read(i2c_adap, 0x10, &reg_0x10);			 
	reg_0x10 |= 0xc0;											 
	ret |= camera_i2c_write(i2c_adap, 0x10, reg_0x10);			 
	ret |= camera_i2c_read(i2c_adap, 0x10, &reg_0x10);	

	return ret;
}

static int module_freeze_aec(struct v4l2_subdev *sd)
{

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_adapter *i2c_adap = client->adapter;		
	int ret = 0;												
	unsigned int reg_0x03 = 0x20;								
	unsigned int reg_0x10 = 0x00;
	GC_INFO("");
	ret = camera_i2c_write(i2c_adap, 0x03, reg_0x03); //page 20	
	ret |= camera_i2c_read(i2c_adap, 0x10, &reg_0x10);			
	reg_0x10 &= 0x7f;											
	ret |= camera_i2c_write(i2c_adap, 0x10, reg_0x10);			
	ret |= camera_i2c_read(i2c_adap, 0x10, &reg_0x10);	

																
	return ret; 												
}


#if 0

static int module_set_exposure_param(struct v4l2_subdev *sd)
{				  
	int ret = 0;	
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	struct i2c_adapter *i2c_adap = client->adapter;	
	unsigned int reg_0x03 = 0x20;												  
	unsigned int reg_0x83 = 0;													  
	unsigned int reg_0x84 = 0;													  
	unsigned int reg_0x85 = 0;	
	GC_INFO("");
	if (priv->capture_exposure_param.shutter < 1)								  
		priv->capture_exposure_param.shutter = 1;								  
																				  
	reg_0x83 = (priv->capture_exposure_param.shutter)>>16;						  
	reg_0x84 = ((priv->capture_exposure_param.shutter)>>8) & 0x000000FF;		  
	reg_0x85 = (priv->capture_exposure_param.shutter) & 0x000000FF; 			  
																				  
	ret  = camera_i2c_write(i2c_adap, 0x03, reg_0x03); //page 20					  
	ret |= camera_i2c_write(i2c_adap, 0x83, reg_0x83);							  
	ret |= camera_i2c_write(i2c_adap, 0x84, reg_0x84);							  
	ret |= camera_i2c_write(i2c_adap, 0x85, reg_0x85);							  
	return ret; 																  
}

#endif
static int module_save_exposure_param(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	int ret = 0;
	unsigned int reg_0x03 = 0x20;
	unsigned int reg_0x80 = 0;
	unsigned int reg_0x81 = 0;
	unsigned int reg_0x82 = 0;
	
	GC_INFO("");
	ret = camera_i2c_write(client->adapter, 0x03, reg_0x03); //page 20
	ret |= camera_i2c_read(client->adapter, 0x80, &reg_0x80);
	ret |= camera_i2c_read(client->adapter, 0x81, &reg_0x81);
	ret |= camera_i2c_read(client->adapter, 0x82, &reg_0x82);
			
	priv->preview_exposure_param.shutter = (reg_0x80 << 16)|(reg_0x81 << 8)|reg_0x82;
    priv->capture_exposure_param.shutter = (priv->preview_exposure_param.shutter);

	return ret;
}



static int module_set_auto_white_balance(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	struct i2c_adapter *i2c_adap = client->adapter;
	int ret = 0;
	int auto_white_balance;
	if(ctrl)
		{
		auto_white_balance = ctrl->val;
	}
	else 
		{
			auto_white_balance = 1;
		}

	GC_INFO("");
  
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
		{
	ctrl->cur.val = auto_white_balance;
		}

	return ret;
}

static int  module_set_ev(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	switch(ctrl->val){
		case 4:
			camera_write_array(client->adapter,module_exp_comp_pos4_regs);
			break;
		case 3:
			camera_write_array(client->adapter,module_exp_comp_pos3_regs);
			break;
		case 2:
			camera_write_array(client->adapter,module_exp_comp_pos2_regs);
			break;
		case 1:
			camera_write_array(client->adapter,module_exp_comp_pos1_regs);
			break;
		case 0:
			camera_write_array(client->adapter,module_exp_comp_zero_regs);
			break;
		case -1:
			camera_write_array(client->adapter,module_exp_comp_neg1_regs);
			break;
		case -2:
			camera_write_array(client->adapter,module_exp_comp_neg2_regs);
			break;
		case -3:
			camera_write_array(client->adapter,module_exp_comp_neg3_regs);
			break;
		case -4:
			camera_write_array(client->adapter,module_exp_comp_neg4_regs);
			break;
		default:
			break;		
	}
	
	return ret;
}

static int module_get_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int reg_0x10, reg_0x03;
	unsigned int reg_0x83,reg_0x84,reg_0x85;
	int ret = 0;
	reg_0x03 = 0x20;
	ret = camera_i2c_write(client->adapter, 0x03, reg_0x03);
	
	reg_0x10 = 0;
	ret = camera_i2c_read(client->adapter, 0x10, &reg_0x10);
	if(reg_0x10 >> 7){
		ret |= camera_i2c_read(client->adapter, 0x80, &reg_0x83);
		ret |= camera_i2c_read(client->adapter, 0x81, &reg_0x84);
		ret |= camera_i2c_read(client->adapter, 0x82, &reg_0x85);
	}
	else {
		ret |= camera_i2c_read(client->adapter, 0x83, &reg_0x83);
		ret |= camera_i2c_read(client->adapter, 0x84, &reg_0x84);
		ret |= camera_i2c_read(client->adapter, 0x85, &reg_0x85);
	}
	ctrl->val = ((reg_0x83&0xff)<<16) + ((reg_0x84&0xff) << 8) + (reg_0x85&0xff);
	ctrl->val /= 246;

	return ret;
}

static int module_get_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int reg_0x03;
	unsigned int reg_0xb0;
	int ret = 0;
	reg_0x03 = 0x20;
	ret = camera_i2c_write(client->adapter, 0x03, reg_0x03);
	
	reg_0xb0 = 0;
	ret = camera_i2c_read(client->adapter, 0xb0, &reg_0xb0);	
	ctrl->val = (reg_0xb0 + 16) * 16;

	return ret;
}

static int module_set_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int reg_0x03;
	unsigned int reg_0xb0;
	int ret = 0;
	int gain; 
	gain = ctrl->val / 16 - 16;
	if (gain < 0) {
		gain = 0;
	}
		
	reg_0x03 = 0x20;
	ret = camera_i2c_write(client->adapter, 0x03, reg_0x03);
	
	reg_0xb0 = gain;
	ret = camera_i2c_write(client->adapter, 0xb0, reg_0xb0);	

	return 0;
}

static int module_set_exposure_param(struct v4l2_subdev *sd)
{
	int ret = 0;
	//printk("int the module_set_exposure_param\n ");
	return ret;
}


static int module_set_white_balance_temperature(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
    struct i2c_adapter *i2c_adap = client->adapter;
    int white_balance_temperature = ctrl->val;
	int ret = 0;
	GC_INFO("");

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


static int module_set_exposure_auto(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	struct i2c_adapter *i2c_adap = client->adapter;
		int ret = 0;
	unsigned int reg_0x03 = 0x10;
	unsigned int reg_0x12;
	int exposure_auto;
	if(ctrl)
		{
		exposure_auto = ctrl->val;
		}
	else 
		{
			exposure_auto = V4L2_EXPOSURE_AUTO;
		}

	GC_INFO("");
	if (exposure_auto < 0 || exposure_auto > 1) {
		dev_err(&client->dev, "set exposure_auto over range, exposure_auto = %d\n", exposure_auto);
		return -ERANGE;
	}
	
	ret = camera_i2c_write(i2c_adap, 0x03, reg_0x03);
	ret |= camera_i2c_read(i2c_adap, 0x12, &reg_0x12); 
	reg_0x12 |= 0x10;  //make sure the Yoffset control is opened.
	ret |= camera_i2c_write(i2c_adap, 0x12, reg_0x12);

	if (exposure_auto < 0 || exposure_auto > 1) {
		return -ERANGE;
	}
  
	switch (exposure_auto) {
	case V4L2_EXPOSURE_AUTO:/*  auto */
        ret = camera_write_array(i2c_adap, module_scene_auto_regs);
		break;

	case V4L2_EXPOSURE_MANUAL: // non auto
		ret = 0;
		break;
	}

	priv->exposure_auto = exposure_auto;
	if(ctrl)
		{
		ctrl->cur.val = exposure_auto;
		}

	return 0;
}



static int  module_set_scene_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	struct i2c_adapter *i2c_adap = client->adapter;
    int scene_exposure = ctrl->val;

	unsigned int reg_0x03 = 0x10;
	unsigned int reg_0x12;
	int ret;
	GC_INFO("");
	ret = camera_i2c_write(i2c_adap, 0x03, reg_0x03);
	ret |= camera_i2c_read(i2c_adap, 0x12, &reg_0x12); 
	reg_0x12 |= 0x10;  //make sure the Yoffset control is opened.
	ret |= camera_i2c_write(i2c_adap, 0x12, reg_0x12);
	
	switch(scene_exposure)
	{
		case V4L2_SCENE_MODE_NIGHT:  //室内
		{
			ret = camera_write_array(i2c_adap, module_scene_night_regs);
			break;
		}
		case V4L2_SCENE_MODE_SUNSET:  //室外
		{
			ret = camera_write_array(i2c_adap, module_scene_auto_regs);
			break;
		}
		default:
		{
			dev_err(&client->dev, "set scene_exposure over range, scene_exposure = %d\n", scene_exposure);
			return -ERANGE;
		}
	}

	priv->scene_exposure = scene_exposure;
	ctrl->cur.val = scene_exposure;

	return 0;
}


static int module_set_stream(struct i2c_client *client,int enable)
{
	struct camera_module_priv *priv = to_camera_priv(client);
	int ret = 0;
	msleep(10);
	GC_INFO("");
	   if (!enable) {
		   GC_INFO("stream down");
		  
		   return ret;
	   }
	
	   if (NULL == priv->win || NULL == priv->cfmt) {
		   GC_ERR("cfmt or win select error");
		   return (-EPERM);
	   }	
	   GC_INFO("stream on");
	   return ret;
}

static int  module_set_af_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	return 0;
}

static int  module_get_af_status(struct camera_module_priv *priv, struct v4l2_ctrl *ctrl)
{
	return 0;
}




static int module_set_mbusformat(struct i2c_client *client, const struct module_color_format *cfmt)
{
	enum v4l2_mbus_pixelcode code;
	int ret = 0;
	unsigned int reg_0x10;
	unsigned int reg_0x03 = 0x10;  //pgae10
	GC_INFO("");

	code = cfmt->code;
	switch (code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
		reg_0x10 = 0x03;
		break;
				
	case V4L2_MBUS_FMT_UYVY8_2X8:
		reg_0x10 = 0x01;
		break;
		
	case V4L2_MBUS_FMT_YVYU8_2X8:
		reg_0x10 = 0x02;
		break;
		
	case V4L2_MBUS_FMT_VYUY8_2X8:
		reg_0x10 = 0x00;
		break;
		
	default:
		return -ERANGE;
	}
	ret = camera_i2c_write(client->adapter, 0x03, reg_0x03);
	ret |= camera_i2c_write(client->adapter, 0x10, reg_0x10);
	
	return ret;
}

static int  module_s_mirror_flip(struct v4l2_subdev *sd, unsigned int mirror, unsigned int flip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_adapter *i2c_adap = client->adapter;
	int ret = 0;
	unsigned int reg_0x03 = 0x00;
	unsigned int reg_0x11 = 0x90;
	
	GC_INFO("");
	
	if ((!mirror) && (!flip)) {
		return 0;
	}

	ret = camera_i2c_write(i2c_adap, 0x03, reg_0x03);

	ret |= camera_i2c_read(i2c_adap, 0x11, &reg_0x11);
	
	if (mirror)	{
		reg_0x11 |= 0x1;
	} else {
		reg_0x11 &= (~0x1);
	}
	
	if (flip) {
		reg_0x11 |= (0x1<<0x1);
	} else {
		reg_0x11 &= (~(0x1<<0x1));	
	}
	
	ret |= camera_i2c_write(i2c_adap, 0x11, reg_0x11);
	return ret;
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

static int module_verify_pid(struct i2c_adapter *i2c_adap,struct camera_module_priv 	*priv)
{
	unsigned int  		pid;
	int ret = 0;
	GC_INFO("");
	
    /*
	 * check and show product ID and manufacturer ID
	 */  
	ret = camera_i2c_read(i2c_adap, PID, &pid); 
	switch (pid) 
    {
	case CAMERA_MODULE_PID:
		if(priv)
			{
		     priv->model= V4L2_IDENT_HI708;
			}
		printk("[%s] Product ID verified %x\n",CAMERA_MODULE_NAME, pid);
		break;
	
	default:
		printk("[%s] Product ID error %x\n",CAMERA_MODULE_NAME, pid);
		return -ENODEV;
	}
	return ret;
}



