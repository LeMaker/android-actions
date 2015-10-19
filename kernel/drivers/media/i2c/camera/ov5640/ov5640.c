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


static bool need_freeze_aec = false;


static int camera_i2c_read(struct i2c_adapter *i2c_adap, unsigned int reg, unsigned int *dest)
{
	unsigned char regs_array[4] = {0, 0, 0, 0};
    unsigned char data_array[4] = {0, 0, 0, 0};
	struct i2c_msg msg;
	int ret = 0;
	regs_array[0] = (reg & 0xff00) >> 8;
    regs_array[1] =  reg & 0x00ff;
	
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
	
	
	regs_array[0] = (reg & 0xff00) >> 8;
    regs_array[1] =  reg & 0x00ff;
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
	ret = camera_i2c_write(client->adapter, 0x3103, 0x11);
    ret = camera_i2c_write(client->adapter, 0x3008, 0x82);
    mdelay(50);
	return ret;
}
#if 0

static int module_soft_standby(struct i2c_client *client)
{
	unsigned int reg_0x1a;
	
	int ret = 0;
	struct i2c_adapter *i2c_adap = client->adapter;
	ret = camera_i2c_read(i2c_adap, 0x1a, &reg_0x1a);
	reg_0x1a |= (0x1<<0);
	ret = camera_i2c_write(i2c_adap, 0x1a, reg_0x1a);
	
	ret |= camera_i2c_write(i2c_adap, 0x25, 0x00);
	
	return ret;	
}
static int module_set_exposure_param(struct v4l2_subdev *sd)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	//struct camera_module_priv *priv = to_camera_priv(client);
	int ret = 0;
    
    
	return ret;
}

#endif

#if 0
static int module_normal(struct v4l2_subdev *sd)
{
	unsigned int reg_0x1a;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
	int ret = 0;

	
	struct i2c_adapter *i2c_adap = client->adapter;
	ret  = camera_i2c_write(i2c_adap, 0xfe,0x00);
	ret |= camera_i2c_read(i2c_adap, 0x1a, &reg_0x1a);
	reg_0x1a &= (~(0x1<<0));
	
	ret |= camera_i2c_write(i2c_adap, 0x1a, reg_0x1a);
	
	ret |= camera_i2c_write(i2c_adap, 0x25, 0xff);
	
	return ret;
}
#endif
static int module_start_aec(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	const struct camera_module_win_size *win = priv->win;
	unsigned int reg_0x3503 = 0x00;
	int ret = 0;

	if (win->width == module_win_vga.width && win->height == module_win_vga.height) {
		need_freeze_aec = true;
	} else {
		need_freeze_aec = false;
	}

	ret = camera_i2c_write(client->adapter, 0x3503, reg_0x3503); 	
	return ret;
}

static int module_freeze_aec(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;
	unsigned int reg_0x3503 = 0x07;

    //if (need_freeze_aec) 
		{
	    ret = camera_i2c_write(client->adapter, 0x3503, reg_0x3503); 
	}
	
	return ret;
}





#if 0
static int  module_start_aec(struct v4l2_subdev *sd)
{
	int ret = 0;

	//printk("int the module_start_aec\n ");
	return ret;
}

static int module_freeze_aec(struct v4l2_subdev *sd)
{
	int ret = 0;
	//printk("int the module_freeze_aec\n ");
	return ret;
}

static int module_set_exposure_param(struct v4l2_subdev *sd)
{
	int ret = 0;
	//printk("int the module_set_exposure_param\n ");
	return ret;
}
static int module_normal(struct v4l2_subdev *sd)
{
	int ret = 0;
	//printk("int the module_set_scene_exposure\n ");
	return ret;
}
#endif

static int  module_set_scene_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	int scene_exposure = ctrl->val;
	unsigned int reg_0x3a00;
	int ret = 0;

	switch(scene_exposure) {
	case V4L2_SCENE_MODE_NIGHT:  //室内
		ret = camera_i2c_read(client->adapter, 0x3a00, &reg_0x3a00);
		reg_0x3a00 = reg_0x3a00 | (0x1 << 2);  //开启夜模式
		ret |= camera_i2c_write(client->adapter, 0x3a00, reg_0x3a00);
		break;

	case V4L2_SCENE_MODE_SUNSET:	//室外
		ret = camera_i2c_read(client->adapter, 0x3a00, &reg_0x3a00);
		reg_0x3a00 = reg_0x3a00 & 0xfb;  //关闭夜模式
		ret |= camera_i2c_write(client->adapter, 0x3a00, reg_0x3a00);
		break;

	default:
		return -ERANGE;
	}

	priv->scene_exposure = scene_exposure;
	ctrl->cur.val = scene_exposure;
	
	return ret;
}

static int module_save_exposure_param(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	struct i2c_adapter *i2c_adap = client->adapter;
    int ret;
    unsigned int shutter;
    unsigned int  reg_0x3500 = 0;
    unsigned int  reg_0x3501 = 0;
    unsigned int  reg_0x3502 = 0;

    ret = camera_i2c_read(i2c_adap, 0x3500, &reg_0x3500);
    ret |= camera_i2c_read(i2c_adap, 0x3501, &reg_0x3501);
    ret |= camera_i2c_read(i2c_adap, 0x3502, &reg_0x3502);

    shutter = (reg_0x3500 & 0x0f);
    shutter = (shutter << 8) + reg_0x3501;
    shutter = (shutter << 4) + (reg_0x3502 >> 4);

    shutter = shutter * 5 / 8;   // for 7.5fps 5M capture
    priv->preview_exposure_param.shutter = shutter;
//pr_alert("###############shutter = 0x%x###########\n", shutter);
    return ret;
}


static int module_set_auto_white_balance(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	struct i2c_adapter *i2c_adap = client->adapter;
	//int auto_white_balance = ctrl->val;
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
	int ret = 0;
	
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
		
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
	int ret = 0;

	return ret;
}

static int module_get_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	return ret;
}

static int module_set_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int digital_gain = 0;
	unsigned int total_gain = 0;
	int ret = 0;

	ret = camera_i2c_read(client->adapter, 0x56a1, &digital_gain);
    digital_gain &= 0xFF;
	/*???,??digital_gain >64(???)??????,<64(???)?????,???????????*/
	if (digital_gain < 32) {
		total_gain =  128*32;//on
	} else {
		total_gain =  28*32;//off
	}    
	
	return total_gain;
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


static int module_set_exposure_param(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct camera_module_priv *priv = to_camera_priv(client);
    struct i2c_adapter *i2c_adap = client->adapter;
    int ret = 0;
    unsigned int capture_shutter;
    unsigned int preview_shutter = priv->preview_exposure_param.shutter;
   	unsigned int  temp = 0;

    capture_shutter = preview_shutter * 15 / 16;

    capture_shutter = capture_shutter * 16;
    temp = capture_shutter & 0xff;
    ret |= camera_i2c_write(i2c_adap, 0x3502, temp);

    temp = (capture_shutter & 0xff00) >> 8;

    ret |= camera_i2c_write(i2c_adap, 0x3501, temp);
    temp = (capture_shutter & 0xff0000) >> 16;
    ret |= camera_i2c_write(i2c_adap, 0x3500, temp);

    return ret;
}


#if 0
static int module_set_colorfx(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	//struct i2c_client *client = v4l2_get_subdevdata(sd);
	//struct camera_module_priv *priv = to_camera_priv(client);
   // int colorfx = ctrl->val;
	int ret = 0;
#if 0
	switch (colorfx) {
	case V4L2_COLORFX_NONE: /* normal */
		ret = camera_write_array(client, module_effect_normal_regs, ARRAY_SIZE(module_effect_normal_regs));
		break;
	
	case V4L2_COLORFX_BW: /* black and white */
		ret = camera_write_array(client, module_effect_white_black_regs, ARRAY_SIZE(module_effect_white_black_regs));
		break;
	
	case V4L2_COLORFX_SEPIA: /* antique ,复古*/
		ret = camera_write_array(client, module_effect_antique_regs, ARRAY_SIZE(module_effect_antique_regs));
		break;

	case V4L2_COLORFX_NEGATIVE: /* negative，负片 */
		ret = camera_write_array(client, module_effect_negative_regs, ARRAY_SIZE(module_effect_negative_regs));
		break;

	default:
		return -ERANGE;
	}

	priv->colorfx = colorfx;
	ctrl->cur.val = colorfx;
#endif
	return ret;
}
#endif
static int module_set_exposure_auto(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	struct i2c_adapter *i2c_adap = client->adapter;
   // int exposure_auto = ctrl->val;
	int ret = 0;
	
	int exposure_auto;
		if(ctrl)
			{
			exposure_auto = ctrl->val;
			}
		else 
			{
				exposure_auto = V4L2_EXPOSURE_AUTO;
			}
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






static int module_pause_af(struct i2c_client *client)
{	

	unsigned int reg_0x3022,reg_0x3023;
	int ret = 0;
	
	printk("%s, %d, pause af\n", __func__, __LINE__);
	reg_0x3023 = 0x01;
	reg_0x3022 = 0x06;
	ret  = camera_i2c_write(client->adapter, 0x3023, reg_0x3023); 
	ret |= camera_i2c_write(client->adapter, 0x3022, reg_0x3022); 
	if (ret) {
		return ret;
	}
msleep(50);

	return ret;
}

static int  module_set_af_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_module_priv *priv = to_camera_priv(client);
	int mode = ctrl->val;
	unsigned int reg_0x3022;
	unsigned int reg_0x3023;
	int ret = 0;

	switch(mode) {
	case NONE_AF:
		reg_0x3023 = 0x01;
		reg_0x3022 = 0x08;
		ret  = camera_i2c_write(client->adapter, 0x3023, reg_0x3023); 
		ret |= camera_i2c_write(client->adapter, 0x3022, reg_0x3022); 
		if (ret) {
			return ret;
		}
		
		priv->af_mode = NONE_AF;
		msleep(50);
		break;
	
	case SINGLE_AF:
#ifndef SINGLE_AF_DISABLE
		reg_0x3023 = 0x01;
		reg_0x3022 = 0x03;
		ret  = camera_i2c_write(client->adapter, 0x3023, reg_0x3023); 
		ret |= camera_i2c_write(client->adapter, 0x3022, reg_0x3022);
		if (ret) {
			return ret;
		}
		
		priv->af_mode = SINGLE_AF;
		msleep(5);
#else
		priv->af_mode = SINGLE_AF;
#endif
		break;
	
	case CONTINUE_AF:
		reg_0x3023 = 0x01;
		reg_0x3022 = 0x04;
		ret  = camera_i2c_write(client->adapter, 0x3023, reg_0x3023); 
		ret |= camera_i2c_write(client->adapter, 0x3022, reg_0x3022);
		if (ret) {
			return ret; 
		}
		
		priv->af_mode = CONTINUE_AF;
		msleep(25);
		printk("in the module_set_af_mode CONTINUE_AF\n");
		break;

	default:
		return -ERANGE;
	}

	ctrl->cur.val = mode;
	return ret;
}


static int  module_get_af_status(struct camera_module_priv *priv, struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = &priv->subdev;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;
	unsigned int reg_value = 0;
	
	
	switch (priv->af_mode){
	case NONE_AF:
		ctrl->val = AF_STATUS_DISABLE;
		break;
	
	case SINGLE_AF:			
	case CONTINUE_AF:
		ret |=  camera_i2c_read(client->adapter, 0x3029, &reg_value); 				
		if (!reg_value) {
			ctrl->val = AF_STATUS_UNFINISH;	
//			printk("-----AF_STATUS_UNFINISH----\r\n");
		} else if(reg_value==0x10||reg_value==0x70){
			ctrl->val = AF_STATUS_OK;
//			printk("-----AF_STATUS_OK----\r\n");			
		}
		break;

	default:
		return -ERANGE;
	}
	
	return ret;
}




static int module_set_stream(struct i2c_client *client,int enable)
{
	struct camera_module_priv *priv = to_camera_priv(client);

	
	int ret = 0;
	   if (!enable) {
		   GC_INFO("stream down");
		   printk("in the stream pause \n");
		   module_pause_af(client);
			return ret;
	   }
	
	   if (NULL == priv->win || NULL == priv->cfmt) {
		   GC_ERR("cfmt or win select error");
		   return (-EPERM);
	   }	

	   printk("the width is %d,height is %d\n",priv->win->width,priv->win->height);
	   ret = camera_write_array(client->adapter,module_init_auto_focus);
	   GC_INFO("stream on");
	  return ret;
}



static int module_set_mbusformat(struct i2c_client *client, const struct module_color_format *cfmt)
{
	enum v4l2_mbus_pixelcode code;
	int ret = 0;
	
	code = cfmt->code;
	switch (code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
		
		break;
				
	case V4L2_MBUS_FMT_UYVY8_2X8:
		
		break;
		
	case V4L2_MBUS_FMT_YVYU8_2X8:
		
		break;
		
	case V4L2_MBUS_FMT_VYUY8_2X8:
		
		break;
		
	default:
		return -ERANGE;
	}
	
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


static int  module_s_mirror_flip(struct v4l2_subdev *sd, unsigned int mirror, unsigned int flip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_adapter *i2c_adap = client->adapter;
	int ret = 0;
	 unsigned int reg_0x3820 = 0x40;
    unsigned int reg_0x3821 = 0x00;
	
	if ((!mirror) && (!flip)) {
			printk("%s():no need to set mirror flip \n",__FUNCTION__);
			return 0;
		}
	
	camera_i2c_read(i2c_adap, 0x3821, &reg_0x3821);
   if (mirror) {
        reg_0x3821 |= (0x3 << 1);
    } else {
        reg_0x3821 &= ~(0x3 << 1);
    }
	camera_i2c_write(i2c_adap, 0x3821, reg_0x3821);

	
    camera_i2c_read(i2c_adap, 0x3820, &reg_0x3820);
   if (flip) {
        reg_0x3821 |= (0x3 << 1);
    } else {
        reg_0x3821 &= ~(0x3 << 1);
    }
	camera_i2c_write(i2c_adap, 0x3820, reg_0x3820);
	return ret;
}

static int module_verify_pid(struct i2c_adapter *i2c_adap,struct camera_module_priv 	*priv)
{
	int ret = 0;
	unsigned int pidh,pidl;
    /*
	 * check and show product ID and manufacturer ID
	 */  
	camera_i2c_read(i2c_adap, PIDH, &pidh);
    camera_i2c_read(i2c_adap, PIDL, &pidl);
	switch (VERSION(pidh, pidl)) 
    {
	case CAMERA_MODULE_PID:
		if(priv)
			{
		     priv->model= V4L2_IDENT_OV5640;
			}
		printk("[%s] Product ID verified %x\n",CAMERA_MODULE_NAME, VERSION(pidh, pidl));
		break;
	
	default:
		printk("[%s] Product ID error %x\n",CAMERA_MODULE_NAME, VERSION(pidh, pidl));
		return -ENODEV;
	}
	return ret;
}



