
/*
 * module common macro
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MODULE_COMM_H__
#define __MODULE_COMM_H__


/* for flags */
#define MODULE_FLAG_VFLIP	(1 << 0) /* Vertical flip image */
#define MODULE_FLAG_HFLIP	(1 << 1) /* Horizontal flip image */
#define MODULE_FLAG_V_H_FLIP (MODULE_FLAG_VFLIP | MODULE_FLAG_HFLIP)

#define MODULE_FLAG_8BIT	(1 << 2) /* default 8 bit */
#define MODULE_FLAG_10BIT	(1 << 3) /*  10 bit interface*/
#define MODULE_FLAG_12BIT	(1 << 4) /* 12 bit interface*/
#define MODULE_FLAG_FRONT	(1 << 5) /* posization front */
#define MODULE_FLAG_BACK	(1 << 6) /* posization back */
#define MODULE_FLAG_PALL	(1 << 7) /* parellal interface */
#define MODULE_FLAG_MIPI	(1 << 8) /* mipi interface */
#define MODULE_FLAG_CHANNEL1	(1 << 9) /* use isp channel1 */
#define MODULE_FLAG_CHANNEL2	(1 << 10) /* use isp channel2 */
#define MODULE_FLAG_3D	(1 << 11) /* 3d mode */
#define MODULE_FLAG_HOST1	(1 << 12) /* on host1 */
#define MODULE_FLAG_HOST2	(1 << 13) /* on host2 */
#define MODULE_FLAG_NODVDD	(1 << 14) /* no use dvdd */
#define MODULE_FLAG_AF	(1 << 15) /* AUTO FOCUS */
#define MODULE_FLAG_ALWAYS_POWER	(1 << 16) /* always power on */
#define MODULE_FLAG_NO_AVDD	(1 << 17) /* no need to operate avdd */


#define SENSOR_FRONT 0x1
#define SENSOR_REAR 0x2
#define SENSOR_DUAL 0x4

#define ENDMARKER { 0xff, 0xff }

//module interface type
//#define INTERFACE_PALL "pall"
//#define INTERFACE_MIPI "mipi"


#define WIDTH_QQVGA		160
#define HEIGHT_QQVGA	120
#define WIDTH_QVGA		320
#define HEIGHT_QVGA		240 
#define WIDTH_VGA		640
#define HEIGHT_VGA		480 
#define WIDTH_SVGA		800
#define HEIGHT_SVGA		600
#define WIDTH_720P		1280
#define HEIGHT_720P		720
#define WIDTH_1080P		1920
#define HEIGHT_1080P	1080
#define WIDTH_UXGA		1600
#define HEIGHT_UXGA		1200
#define WIDTH_QSXGA		2592
#define HEIGHT_QSXGA	1944
#define WIDTH_QXGA      2048
#define HEIGHT_QXGA      1536

//#define MODULE_DBG

#ifdef MODULE_DBG
#define GC_INFO(fmt, args...)  printk(KERN_INFO"[" CAMERA_MODULE_NAME "] line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)
#else
#define GC_INFO(fmt, args...)  
#endif

#define GC_ERR(fmt, args...)   printk(KERN_ERR"[" CAMERA_MODULE_NAME "] line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)


struct regval_list {
	unsigned short reg_num;
	unsigned short value;
};




struct camera_module_win_size {
	char								*name;
	__u32								width;
	__u32								height;
	const struct regval_list			*win_regs;
	unsigned int        				*frame_rate_array;
	unsigned int capture_only;
};

typedef struct
{
	 unsigned int max_shutter;
    unsigned int shutter;
    unsigned int gain;
    unsigned int dummy_line;
    unsigned int dummy_pixel;
    unsigned int extra_line;
} exposure_param_t;

struct module_color_format {
	enum v4l2_mbus_pixelcode code;
    enum v4l2_colorspace colorspace;
};

struct camera_module_priv {
	struct v4l2_subdev subdev;
    struct v4l2_ctrl_handler hdl;
    struct module_info *info;
    const struct module_color_format *cfmt;
    const struct camera_module_win_size *win;
    int model;
    int pcv_mode;
    int flip_flag;

    unsigned short auto_white_balance;
    unsigned short exposure;
    unsigned short power_line_frequency;
    unsigned short white_balance_temperature;
    unsigned short colorfx;
    unsigned short exposure_auto;
    unsigned short scene_exposure;

    exposure_param_t preview_exposure_param;
    exposure_param_t capture_exposure_param;
	enum v4l2_flash_led_mode 			flash_led_mode;
	enum af_status						af_status;	
	enum af_mode						af_mode;	
};

/*
 * supported color format list.
 * see definition in
 *     http://thread.gmane.org/gmane.linux.drivers.video-input-infrastructure/12830/focus=13394
 * YUYV8_2X8_LE == YUYV with LE packing
 * YUYV8_2X8_BE == UYVY with LE packing
 * YVYU8_2X8_LE == YVYU with LE packing
 * YVYU8_2X8_BE == VYUY with LE packing
 */
static const struct module_color_format module_cfmts[] = {
	{
        .code = V4L2_MBUS_FMT_YUYV8_2X8,
        .colorspace = V4L2_COLORSPACE_JPEG,
    },
    {
        .code = V4L2_MBUS_FMT_UYVY8_2X8,
        .colorspace = V4L2_COLORSPACE_JPEG,
    },
    {
        .code = V4L2_MBUS_FMT_YVYU8_2X8,
        .colorspace = V4L2_COLORSPACE_JPEG,
    },
    {
        .code = V4L2_MBUS_FMT_VYUY8_2X8,
        .colorspace = V4L2_COLORSPACE_JPEG,
    },
};

#ifdef  SELf_DETECT
static int detect_work(void);
static int detect_init(void);
static void detect_deinit(void);
#endif



static int camera_i2c_read(struct i2c_adapter *i2c_adap, unsigned int reg, unsigned int *dest);
static int camera_i2c_write(struct i2c_adapter *i2c_adap, unsigned int reg, unsigned int src);
static int camera_write_array(struct i2c_adapter *i2c_adap, const struct regval_list *vals);

static struct camera_module_priv *to_camera_priv(const struct i2c_client *client);

static int  module_soft_reset(struct i2c_client *client);
static int module_save_exposure_param(struct v4l2_subdev *sd);


static int  module_set_auto_white_balance(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_set_white_balance_temperature(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_set_exposure_auto(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_set_scene_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);



static int  module_set_mbusformat(struct i2c_client *client, const struct module_color_format *cfmt);
//static int  module_s_mirror_flip(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_s_mirror_flip(struct v4l2_subdev *sd, unsigned int mirror, unsigned int flip);

static int module_verify_pid(struct i2c_adapter *i2c_adap,struct camera_module_priv 	*priv);
static int module_set_stream(struct i2c_client *client,int enable);
static int  module_set_ev(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_set_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_get_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_get_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_set_af_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_get_af_status(struct camera_module_priv *priv, struct v4l2_ctrl *ctrl);
static int  module_set_power_line(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_get_power_line(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);



static int  module_start_aec(struct v4l2_subdev *sd);
static int  module_freeze_aec(struct v4l2_subdev *sd);
//static int module_normal(struct v4l2_subdev *sd);
static int  module_set_exposure_param(struct v4l2_subdev *sd);
static int module_save_exposure_param(struct v4l2_subdev *sd);

#if 0
static int  module_soft_standby(struct i2c_client *client);

static int  module_set_exposure(struct v4l2_subdev *sd);

static int  module_start_aec(struct v4l2_subdev *sd);
static int  module_freeze_aec(struct v4l2_subdev *sd);

static int  module_set_scene_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_set_prev_capt_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_pause_af(struct i2c_client *client);
static int  module_set_af_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_get_af_status(struct camera_module_priv *priv, struct v4l2_ctrl *ctrl);
static int  module_get_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_get_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
static int  module_set_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);

static int  module_set_colorfx(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl);
#endif

#endif  //__MODULE_COMM_H__
