/*
 * gs5604 Camera Driver
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
// extern inline void set_gpio_level(struct dts_gpio *gpio, bool active);

static int camera_i2c_read(struct i2c_adapter *i2c_adap,unsigned int data_width, 
unsigned int reg, unsigned int *dest)
{
	unsigned char regs_array[4] = {0, 0, 0, 0};
    unsigned char data_array[4] = {0, 0, 0, 0};
	struct i2c_msg msg;
	int ret = 0;
	
	if (I2C_REGS_WIDTH == 1)		
		regs_array[0] = reg & 0xff;
	if (I2C_REGS_WIDTH == 2) {
		regs_array[0] = (reg >> 8) & 0xff;
		regs_array[1] = reg & 0xff;
	}
	msg.addr = MODULE_I2C_REAL_ADDRESS;
	msg.flags = 0;
	msg.len   = I2C_REGS_WIDTH;
	msg.buf   = regs_array;
	ret = i2c_transfer(i2c_adap, &msg, 1);
	if (ret < 0) {
		printk("write register %s error %d",CAMERA_MODULE_NAME, ret);
		return ret;
	}

	msg.flags = I2C_M_RD;
	msg.len   = data_width;
	msg.buf   = data_array;	
	ret = i2c_transfer(i2c_adap, &msg, 1);
	if (ret >= 0) {
        ret = 0;
        if (data_width == 1)
			*dest = data_array[0];
		if (data_width == 2)
			*dest = data_array[0]<<8 | data_array[1];
	}
	else {
	    printk("read register %s error %d",CAMERA_MODULE_NAME, ret);
	}
	
	return ret;
}
static int camera_i2c_write(struct i2c_adapter *i2c_adap, unsigned int data_width, 
unsigned int reg, unsigned int data)
{
	unsigned char regs_array[4] = {0, 0, 0, 0};
    unsigned char data_array[4] = {0, 0, 0, 0};
    unsigned char tran_array[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	struct i2c_msg msg;
	int ret,i;
	
//	printk("data_width = %d,reg = 0x%04x, data = 0x%x \n",data_width,reg,data);
	if (I2C_REGS_WIDTH == 1)		
		regs_array[0] = reg & 0xff;
	if (I2C_REGS_WIDTH == 2) {
		regs_array[0] = (reg >> 8) & 0xff;
		regs_array[1] = reg & 0xff;
	}
	if (data_width == 1)
		data_array[0] = data & 0xff;
	if (data_width == 2) {
		data_array[0] = data & 0xff;
		data_array[1] = (data >> 8) & 0xff;
	}
	for (i = 0; i < I2C_REGS_WIDTH; i++) {
        tran_array[i] = regs_array[i];
    }

    for (i = I2C_REGS_WIDTH; i < (I2C_REGS_WIDTH + data_width); i++) {
        tran_array[i] = data_array[i - I2C_REGS_WIDTH];
    }
	
	msg.addr = MODULE_I2C_REAL_ADDRESS;
	msg.flags = 0;
	msg.len   = I2C_REGS_WIDTH + data_width;
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
							vals->data_width,
							vals->reg_num,
							vals->value);
		if (ret < 0){
			printk("[camera] i2c write error!,i2c address is %x\n",MODULE_I2C_REAL_ADDRESS);
			return ret;
		}
		vals++;
	}
	return 0;
}
/*************************************************************************
* FUNCTION
*   GS5604MIPI_WAIT_STATUS
*
* DESCRIPTION
*   This function wait the 0x000E bit 0 is 1;then clear the bit 0;
*   The salve address is 0x34
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void gs5604mipi_wait_status(struct i2c_adapter *i2c_adap)

{
    unsigned int tmp = 0,count = 0;
    DBG_INFO("");
	
    do{
    	count ++;
		if (count > 50)
			break;
    	camera_i2c_read(i2c_adap, 1, 0x000E, &tmp);
    	tmp &= 0x01;
    	DBG_INFO("gs5604mipi_wait_status while1!\r\n");		
	}while(!tmp);
	
	mdelay(10);
	
	camera_i2c_write(i2c_adap, 1, 0x0012, 0x01);
	
	mdelay(10);
	 
    do{
    	camera_i2c_read(i2c_adap, 1, 0x000E, &tmp);
    	tmp &= 0x01;
    	DBG_INFO("gs5604mipi_wait_status while2!\r\n");
    }while(tmp);
    
    mdelay(100);
	
    DBG_INFO("gs5604mipi_wait_status exit\n ");
}

/*************************************************************************
* FUNCTION
*   GS5604MIPI_WAIT_STATUS1
*
* DESCRIPTION
*   This function wait the 0x000E bit 1 is 1;then clear the bit 1;
*   The salve address is 0x78
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void gs5604mipi_wait_status1(struct i2c_adapter *i2c_adap)

{
    unsigned int tmp = 0,count = 0;
    DBG_INFO("[GS5604MIPI]enter GS5604MIPI_WAIT_STATUS1 function:\n ");

    do{
    	count ++;
		if (count > 50)
			break;
     	camera_i2c_read(i2c_adap, 1, 0x000E, &tmp);
     	tmp &= 0x02;
     	DBG_INFO("gs5604mipi_wait_status while1!\r\n");	
    }while(tmp !=0x02);
    mdelay(10);

    camera_i2c_write(i2c_adap, 1, 0x0012,0x02);

    do{
    	camera_i2c_read(i2c_adap, 1, 0x000E, &tmp);
    	tmp &= 0x02;
    	DBG_INFO("gs5604mipi_wait_status while2!\r\n");	
    }while(tmp);
    
    mdelay(10);
    DBG_INFO("[GS5604MIPI]exit GS5604MIPI_WAIT_STATUS1 function:\n ");
}

/*************************************************************************
* FUNCTION
*   GS5604MIPI_WAIT_STATUS2
*
* DESCRIPTION
*   This function wait the 0x000E bit 0 is 1;then clear the bit 0;
*   The salve address is 0x78
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void gs5604mipi_wait_status2(struct i2c_adapter *i2c_adap)

{
    unsigned int tmp = 0,count = 0;

    DBG_INFO("[GS5604MIPI]enter GS5604MIPI_WAIT_STATUS2 function:\n ");

    do{
    	count ++;
		if (count > 50)
			break;
    	camera_i2c_read(i2c_adap, 1, 0x000E, &tmp);
    	tmp &= 0x01;
    	DBG_INFO("gs5604mipi_wait_status while1!\r\n");	
    }while(!tmp);
    mdelay(10);

    camera_i2c_write(i2c_adap, 1, 0x0012,0x01);

    do{
    	camera_i2c_read(i2c_adap, 1, 0x000E, &tmp);
    	tmp &= 0x01;
    	DBG_INFO("gs5604mipi_wait_status while2!\r\n");	
    }while(tmp);
    
    mdelay(10);
    DBG_INFO("[GS5604MIPI]exit GS5604MIPI_WAIT_STATUS2 function:\n ");
}

/*************************************************************************
* FUNCTION
*   GS5604MIPI_WAIT_STATUS3
*
* DESCRIPTION
*   This function wait the 0x000E bit 4 is 1;then clear the bit 1;
*   The salve address is 0x78
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void gs5604mipi_wait_status3(struct i2c_adapter *i2c_adap)
{
    unsigned int tmp = 0;
    int count = 0;
    DBG_INFO("[GS5604MIPI]enter GS5604MIPI_WAIT_STATUS3 function:\n ");
    do{
    	camera_i2c_read(i2c_adap, 1, 0x000E, &tmp);
    	tmp &= 0x10;
    	DBG_INFO("gs5604mipi_wait_status while1!\r\n");	
    }while(tmp !=0x10);
    
    mdelay(10);
    camera_i2c_write(i2c_adap, 1, 0x0012,0x10);
    
    do{
    	count ++;
		if (count > 50)
			break;
    	camera_i2c_read(i2c_adap, 1, 0x000E, &tmp);
    	tmp &= 0x10;
    	DBG_INFO("gs5604mipi_wait_status while1!\r\n");	
	}while(tmp);
	
    mdelay(10);
    DBG_INFO("[GS5604MIPI]exit GS5604MIPI_WAIT_STATUS3 function:\n ");
}
static void update_after_init(struct i2c_adapter *i2c_adap)
{

	/*change iic address form 0x1a to 0x3c*/
	MODULE_I2C_REAL_ADDRESS = 0x3c;
	
	mdelay(50);  //don't delete
    gs5604mipi_wait_status2(i2c_adap);
	camera_i2c_write(i2c_adap, 1, 0x5008,0x00); 
    mdelay(50);	
}
static void start_af(struct i2c_adapter *i2c_adap)
{
	int af_state = 0;
	int count = 0;
	
     /***** AF manual start******/
    camera_i2c_write(i2c_adap, 2, 0x6648,0x00);
	camera_i2c_write(i2c_adap, 1, 0x00B2,0x02);
	camera_i2c_write(i2c_adap, 1, 0x00B3,0x02);//halfrelease mode: manual af
	camera_i2c_write(i2c_adap, 1, 0x00B4,0x02);
	camera_i2c_write(i2c_adap, 1, 0x00B1,0x01);//restart
	mdelay(100);
	do
	{	
		count ++;
		if (count > 50)
			break;
		camera_i2c_read(i2c_adap, 1, 0x8b8a, &af_state);
	}while(af_state !=0x03);
	/***** AF manual end******/	
}
static void enter_preview_mode(struct i2c_adapter *i2c_adap)
{
	gs5604mipi_wait_status1(i2c_adap);
	mdelay(20);
}
static void enter_capture_mode(struct i2c_adapter *i2c_adap)
{
	start_af(i2c_adap);
	gs5604mipi_wait_status1(i2c_adap);
	mdelay(100);	
}
static void enter_video_mode(struct i2c_adapter *i2c_adap)
{
	gs5604mipi_wait_status1(i2c_adap);
	mdelay(20);		
}
static int module_soft_reset(struct i2c_client *client)
{
	int ret;
	unsigned int reg_0xfe;
	struct i2c_adapter *i2c_adap = client->adapter;
    DBG_INFO("");
    
	gs5604mipi_wait_status(i2c_adap);
    return ret;
}

static int module_start_aec(struct v4l2_subdev *sd)
{
  int ret = 0;
  
//  struct i2c_client *client = v4l2_get_subdevdata(sd);
////  struct camera_module_priv *priv = to_camera_priv(client);
//  struct i2c_adapter *i2c_adap = client->adapter;
//  
//  unsigned int reg_0xfe = 0x00;
//  unsigned int reg_0xb6 = 0x01;
//
//  ret = camera_i2c_write(i2c_adap, 0xfe, reg_0xfe); //page 0
//  ret |= camera_i2c_write(i2c_adap, 0xb6, reg_0xb6); 

  return ret;
}

static int module_freeze_aec(struct v4l2_subdev *sd)
{
 int ret = 0;
 
//  struct i2c_client *client = v4l2_get_subdevdata(sd);
////  struct camera_module_priv *priv = to_camera_priv(client);
//  struct i2c_adapter *i2c_adap = client->adapter;
// 
//  unsigned int reg_0xfe = 0x00;
//  unsigned int reg_0xb6 = 0x00;
//
//  ret = camera_i2c_write(i2c_adap, 0xfe, reg_0xfe); //page 0
//  ret |= camera_i2c_write(i2c_adap, 0xb6, reg_0xb6); 
 
  return ret;
}

static int module_save_exposure_param(struct v4l2_subdev *sd)
{
	int ret = 0;
	
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
//	struct i2c_adapter *i2c_adap = client->adapter;
//	
//	unsigned int reg_0xfe = 0x00;
//	unsigned int reg_0x03;
//	unsigned int reg_0x04;
//
//	ret = camera_i2c_write(i2c_adap, 0xfe, reg_0xfe); //page 0
//	ret |= camera_i2c_read(i2c_adap, 0x03, &reg_0x03);
//	ret |= camera_i2c_read(i2c_adap, 0x04, &reg_0x04);
//		
//	priv->preview_exposure_param.shutter = (reg_0x03 << 8) | reg_0x04;
//	priv->capture_exposure_param.shutter = (priv->preview_exposure_param.shutter)/2;
//	
//	//printk("GC2155 module_save_exposure_param, win->name:%s\n", priv->win->name);
	return ret;
}

static int module_set_exposure_param(struct v4l2_subdev *sd)
{
 	int ret = 0;
  
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
//	struct i2c_adapter *i2c_adap = client->adapter;
//	
//	unsigned int reg_0xfe = 0x00;
//	unsigned char reg_0x03;
//	unsigned char reg_0x04;
//
//	if(priv->capture_exposure_param.shutter < 1) {
//		priv->capture_exposure_param.shutter = 1;
//	}
//
//	reg_0x03 = ((priv->capture_exposure_param.shutter)>>8) & 0x1F ;
//	reg_0x04 = (priv->capture_exposure_param.shutter) & 0xFF;
//
//	ret = camera_i2c_write(i2c_adap, 0xfe, reg_0xfe); //page 0
//	ret |= camera_i2c_write(i2c_adap, 0x03, reg_0x03);
//	ret |= camera_i2c_write(i2c_adap, 0x04, reg_0x04);
	
	return ret;
}

static int module_set_auto_white_balance(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	int ret = 0;
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
//	struct i2c_adapter *i2c_adap = client->adapter;
//	int auto_white_balance;
//
//
//	if(ctrl)
//		auto_white_balance = ctrl->val;
//	else
//		auto_white_balance = 1;
//  
//	if (auto_white_balance < 0 || auto_white_balance > 1) {
//		printk("[gs5604] set auto_white_balance over range, auto_white_balance = %d\n", auto_white_balance);
//		return -ERANGE;
//	}
//	
//	switch(auto_white_balance)
//	{
//	case 0:
//		ret = 0;
//		goto change_val;
//		
//	case 1:	
//		ret |=camera_write_array(i2c_adap, module_whitebance_auto_regs);
//		break;
//	}
//	
//change_val:
//	priv->auto_white_balance = auto_white_balance;
//	if(ctrl)
//		ctrl->cur.val = auto_white_balance;  
//		

	return ret;
}

static int module_set_white_balance_temperature(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
//	int white_balance_temperature = ctrl->val;
//	struct i2c_adapter *i2c_adap = client->adapter;
	int ret = 0;
	
//	switch(white_balance_temperature)
//	{
//	case V4L2_WHITE_BALANCE_INCANDESCENT: /* 白炽光 */
//		ret = camera_write_array(i2c_adap, module_whitebance_incandescent_regs);
//		break;
//	
//	case V4L2_WHITE_BALANCE_FLUORESCENT: /* 荧光灯 */
//		ret = camera_write_array(i2c_adap, module_whitebance_fluorescent_regs);
//		break;
//	
//	case V4L2_WHITE_BALANCE_DAYLIGHT: /* 日光 (晴天)*/
//		ret = camera_write_array(i2c_adap, module_whitebance_sunny_regs);
//		break;
//	
//	case V4L2_WHITE_BALANCE_CLOUDY: /* 多云 （阴天）*/
//		ret = camera_write_array(i2c_adap, module_whitebance_cloudy_regs);
//		break;
//	
//	default:
//		printk("[gs5604] set white_balance_temperature over range, white_balance_temperature = %d\n", white_balance_temperature);
//		return -ERANGE;	
//	}
//	
//	priv->auto_white_balance = 0;
//	priv->white_balance_temperature = white_balance_temperature;
//	ctrl->cur.val = white_balance_temperature;
//	
	return ret;
}
static int module_set_colorfx(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
//	struct i2c_adapter *i2c_adap = client->adapter;
//  int colorfx = ctrl->val;
//	int ret = 0;
//
//
//	switch (colorfx) {
//	case V4L2_COLORFX_NONE: /* normal */
//		ret = camera_write_array(i2c_adap, module_effect_normal_regs);
//		break;
//	
//	case V4L2_COLORFX_BW: /* black and white */
//		ret = camera_write_array(i2c_adap, module_effect_white_black_regs);
//		break;
//	
//	case V4L2_COLORFX_SEPIA: /* antique ,复古*/
//		ret = camera_write_array(i2c_adap, module_effect_antique_regs);
//		break;
//
//	case V4L2_COLORFX_NEGATIVE: /* negative，负片 */
//		ret = camera_write_array(i2c_adap, module_effect_negative_regs);
//		break;
//
//    default:
//        printk("[gs5604] set colorfx over range, colorfx = %d\n", colorfx);
//        return -ERANGE;	
//    }
//
//    priv->colorfx = colorfx;
//    ctrl->cur.val = colorfx;

    return 0;
}
static int module_set_exposure_auto(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
//	struct i2c_adapter *i2c_adap = client->adapter;
//    int exposure_auto;
	int ret = 0;

//	if(ctrl)
//		exposure_auto = ctrl->val;
//	else
//		exposure_auto = V4L2_EXPOSURE_AUTO;
//
//	if (exposure_auto < 0 || exposure_auto > 1) {
//		return -ERANGE;
//	}
//  
//	switch (exposure_auto) {
//	case V4L2_EXPOSURE_AUTO:/*  auto */
//        ret = camera_write_array(i2c_adap, module_scene_auto_regs);
//		break;
//
//    case V4L2_EXPOSURE_MANUAL: // non auto
//        ret = 0;
//        break;
//    }
//
//	priv->exposure_auto = exposure_auto;
//	if(ctrl)
//		ctrl->cur.val = exposure_auto;

    return ret;
}

static int module_set_scene_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	int ret = 0;
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct camera_module_priv *priv = to_camera_priv(client);
//	struct i2c_adapter *i2c_adap = client->adapter;
//  	int scene_exposure = ctrl->val;
//  	unsigned char reg_3a00 = 0;
//
//	
//	switch(scene_exposure) {
//	case 0:  //室内
//
//		break;
//	
//	case 1:  //室外
//
//		break;
//
//    default:
//        printk("[gs5604] set scene_exposure over range, scene_exposure = %d\n", scene_exposure);
//        return -ERANGE;
//    }
//
//    priv->scene_exposure = scene_exposure;
//    ctrl->cur.val = scene_exposure;

	return ret;
}

static int module_set_stream(struct i2c_client *client,int enable)
{
	struct camera_module_priv *priv = to_camera_priv(client);
	struct i2c_adapter *i2c_adap = client->adapter;
	int ret = 0;


	DBG_INFO("");
	   if (!enable) {
		   DBG_INFO("stream down");
//		   camera_i2c_write(i2c_adap, 0x4202, 0x0f);
		   return ret;
	   }
	
	   if (NULL == priv->win || NULL == priv->cfmt) {
		   DBG_ERR("cfmt or win select error");
		   return (-EPERM);
	   }	
	   DBG_INFO("stream on");
//	   camera_i2c_write(i2c_adap, 0x4202, 0x00);   
	 return 0;   
}
static int  module_set_exposure(struct v4l2_subdev *sd, int val)
{
	// struct i2c_adapter *i2c_adap = client->adapter;
	// unsigned int exposure_3500 = 0;
	// unsigned int exposure_3501 = 0;
	// unsigned int exposure_3502 = 0;
	int ret = 0;
	// DBG_INFO(" val = 0x%04x \n", val);
	// exposure_3500 = (val >> 16) & 0x0f;
	// exposure_3501 = (val >> 8) & 0xff;
	// exposure_3502 = val & 0xf0;
	 	
//	ret = camera_i2c_write(i2c_adap, 0x3500, exposure_3500);
//	ret |= camera_i2c_write(i2c_adap, 0x3501, exposure_3501);
//	ret |= camera_i2c_write(i2c_adap, 0x3502, exposure_3502);
	return ret;
}
static int module_get_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	// struct i2c_adapter *i2c_adap = client->adapter;
	// unsigned int exposure_3500 = 0;
	// unsigned int exposure_3501 = 0;
	// unsigned int exposure_3502 = 0;
	int ret = 0;
	
	// ret = camera_i2c_read(i2c_adap, 0x3500, &exposure_3500);
	// ret |= camera_i2c_read(i2c_adap, 0x3501, &exposure_3501);
	// ret |= camera_i2c_read(i2c_adap, 0x3502, &exposure_3502);
	// val = (exposure_3500 & 0x0f) << 16 + (exposure_3501 & 0xff) << 8 + exposure_3502 & 0xf0;
	
	// DBG_INFO(" val = 0x%04x \n", val);
    return ret;
}

static int module_get_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	// struct i2c_adapter *i2c_adap = client->adapter;
	// unsigned int gain_350a = 0;
	// unsigned int gain_350b = 0;
	int ret = 0;
	
//	ret = camera_i2c_read(i2c_adap, 0x350a, &gain_350a);
//	ret |= camera_i2c_read(i2c_adap, 0x350b, &gain_350b);
//	val = (gain_350a & 0x03) << 8 + gain_350b & 0xff;
//	
	DBG_INFO(" val = 0x%04x \n", val);
    return ret;
}

static int module_set_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	// struct i2c_adapter *i2c_adap = client->adapter;
	// unsigned int gain_350a = 0;
	// unsigned int gain_350b = 0;
	int ret = 0;
	
	DBG_INFO(" val = 0x%04x \n", val);
//	gain_350a = (val >> 8) &0x03; 
//	gain_350b = val & 0xff;
//	ret = camera_i2c_write(i2c_adap, 0x350a, gain_350a);
//	ret |= camera_i2c_write(i2c_adap, 0x350b, gain_350b);

    return ret;
}

static int module_set_ev(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
 	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_adapter *i2c_adap = client->adapter;
	
	switch(ctrl->val) {
		case 3:

			break;
		case 2:
		
			break;
		case 1:
			
			break;
		case 0:
			
			break;
		case -1:
			
			break;
		case -2:
		
			break;
		case -3:
		
			break;
		default:
			printk("error:gs5604 env setting,val out of rang\n");
			return -ERANGE;	
	}
    return 0;
}
static int module_set_mbusformat(struct i2c_client *client, const struct module_color_format *cfmt)
{
	return 0;
}

static int module_s_mirror_flip(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_adapter *i2c_adap = client->adapter;
	int ret = 0;

    DBG_INFO("");

    switch (ctrl->id) {
    case V4L2_CID_HFLIP:
        if (ctrl->val) {
// 			ret = camera_i2c_write(i2c_adap, 0x3820, 0x47);
        } else {
//			ret |= camera_i2c_write(i2c_adap, 0x3820, 0x41);
        }
        break;
    case MODULE_FLAG_VFLIP:
        if (ctrl->val) {
//            ret |= camera_i2c_write(i2c_adap, 0x3821, 0x07);
        } else {
//            ret |= camera_i2c_write(i2c_adap, 0x3821, 0x01);
        }
        break;
    default:
        DBG_ERR("set flip out of range\n");
        return (-ERANGE);
    }
  
	return ret;
}

static int module_verify_pid(struct i2c_adapter *i2c_adap,struct camera_module_priv 	*priv)
{
	unsigned int pid = 0;
    int ret;

	
	DBG_INFO("");

    /*
	 * check and show product ID and manufacturer ID
	 */  
	ret = camera_i2c_read(i2c_adap, 1, PID, &pid); 
	
	switch (pid) 
    {
		case CAMERA_MODULE_PID:

			printk("[%s] Product ID verified %x\n",CAMERA_MODULE_NAME, pid);
		break;
	
		default:
			printk("[%s] Product ID error %x\n",CAMERA_MODULE_NAME, pid);
		return -ENODEV;
	}
	return ret;
}

static int  module_set_af_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct i2c_adapter *i2c_adap = client->adapter;
//	
//	ret |= camera_i2c_write(i2c_adap, 0x3022, 0x03);
	return 0;
}

static int module_get_af_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
  int ret = 0;
	ctrl->val = NONE_AF;
	//DBG_INFO("module_get_af_mode: %d\n", ctrl->val);
	return ret;
}

static int  module_get_af_status(struct camera_module_priv *priv, struct v4l2_ctrl *ctrl)
{
	return 0;
}
// void sensor_power_on(bool rear, struct sensor_pwd_info *spinfo, bool hardware)
// {
   
	// if (hardware) {
		
		// if (rear) { 
			// set_gpio_level(&spinfo->gpio_power, GPIO_LOW);
			// mdelay(50);
			// set_gpio_level(&spinfo->gpio_power, GPIO_HIGH);
			// mdelay(50);
			// set_gpio_level(&spinfo->gpio_rear, GPIO_LOW);
			// set_gpio_level(&spinfo->gpio_rear_reset, GPIO_LOW);
			// mdelay(50);
			// set_gpio_level(&spinfo->gpio_rear_reset, GPIO_HIGH);
			// mdelay(50);
	        // set_gpio_level(&spinfo->gpio_rear, GPIO_HIGH);
	        // mdelay(50);
	    // } else {
			// set_gpio_level(spinfo->gpio_power, GPIO_LOW);
			// mdelay(500);
			// set_gpio_level(spinfo->gpio_power, GPIO_HIGH);
			// mdelay(500);	    	
			// set_gpio_level(&spinfo->gpio_front, GPIO_LOW);
			// set_gpio_level(&spinfo->gpio_front_reset, GPIO_LOW);
			// mdelay(500);
			// set_gpio_level(&spinfo->gpio_front_reset, GPIO_HIGH);
			// mdelay(500);
	        // set_gpio_level(&spinfo->gpio_front, GPIO_HIGH);
	        // mdelay(500);    
	    // }

	// } else {
		// if (rear) {
			// set_gpio_level(&spinfo->gpio_rear, GPIO_HIGH);
			// mdelay(20);
		// } else {
			// set_gpio_level(&spinfo->gpio_front, GPIO_HIGH);
			// mdelay(20);
		// }
	// }
// }

// void sensor_power_off(bool rear, struct sensor_pwd_info *spinfo, bool hardware)
// {
	// if (hardware) {
		
	    // if (rear) {
	    	// set_gpio_level(&spinfo->gpio_rear, GPIO_LOW);
	    	// mdelay(5);
	    	// set_gpio_level(&spinfo->gpio_rear_reset, GPIO_LOW);
	    	// mdelay(10);
	    // } else {
	    	// set_gpio_level(&spinfo->gpio_front, GPIO_LOW);
	    	// mdelay(5);
	    	// set_gpio_level(&spinfo->gpio_front_reset, GPIO_LOW);
	        
	    // }
	// } else {
		// if (rear) {
			// set_gpio_level(&spinfo->gpio_rear, GPIO_LOW);
			// mdelay(10);
		// }
		// else
			// set_gpio_level(&spinfo->gpio_front, GPIO_LOW);
	// }
// }
// static int get_sensor_id(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
// {
	// int ret = 0;
	
	// ctrl->val = CAMERA_MODULE_PID;
	
	// return ret;
// }

static int  module_set_power_line(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	
	return 0;
}
static int  module_get_power_line(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
	
	ctrl->val = V4L2_CID_POWER_LINE_FREQUENCY_AUTO;
	return 0;
}