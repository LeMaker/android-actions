
#ifndef __ISP_OWL_H__
#define __ISP_OWL_H__

#include <media/soc_camera.h>
#include <linux/pinctrl/consumer.h>
struct soc_camera_link;


/* for flags */
/* used by dts, will replace sensor's default value */
#define SENSOR_FLAG_CHANNEL1  (1 << 0) /* bit-0, 1 channel_1; 0 channel_2  */
#define SENSOR_FLAG_CHANNEL2  (0 << 0)
#define SENSOR_FLAG_CH_MASK   (SENSOR_FLAG_CHANNEL1)
#define SENSOR_FLAG_DVP       (1 << 1) /* bit-1, 1,  parellal interface, DVP; 0 mipi*/
#define SENSOR_FLAG_MIPI      (0 << 1)
#define SENSOR_FLAG_INTF_MASK (SENSOR_FLAG_DVP)
#define SENSOR_FLAG_YUV       (1 << 2) /* bit-2, 1, output yuv data; 0 raw data */
#define SENSOR_FLAG_RAW       (0 << 2)
#define SENSOR_FLAG_DATA_MASK (SENSOR_FLAG_YUV)
#define SENSOR_FLAG_DTS_MASK  \
    (SENSOR_FLAG_CH_MASK | SENSOR_FLAG_INTF_MASK | SENSOR_FLAG_DATA_MASK)

/* determined by sensor driver, not dts */
#define SENSOR_FLAG_8BIT      (1 << 8)
#define SENSOR_FLAG_10BIT     (1 << 9)
#define SENSOR_FLAG_12BIT     (1 << 10)


#define OUTTO_SENSO_CLOCK 24000000

#define ISP_CHANNEL_0 0
#define ISP_CHANNEL_1 1

#define SENSOR_DATA_TYPE_YUV 0
#define SENSOR_DATA_TYPE_RAW 1


#define OWL_ISP_HOST0  0
#define OWL_ISP_HOST1  1

/* ANSI Color codes */
#define VT(CODES)  "\033[" CODES "m"
#define VT_NORMAL  VT("")
#define VT_RED     VT("0;32;31")
#define VT_GREEN   VT("1;32")
#define VT_YELLOW  VT("1;33")
#define VT_BLUE    VT("1;34")
#define VT_PURPLE  VT("0;35")

#define xprintk(fmt, ...) \
    printk("%s()->%d " fmt, __func__, __LINE__, ## __VA_ARGS__)

#define _DBG(color, fmt, ...)  \
    xprintk(color "" fmt VT_NORMAL, ## __VA_ARGS__)

#define _INFO(color, fmt, ...) \
    xprintk(color "::" fmt ""VT_NORMAL, ## __VA_ARGS__)

/* mainly used in test code */
#define INFO_PURLPLE(fmt, args...) _INFO(VT_PURPLE, fmt, ## args)
#define INFO_RED(fmt, args...)     _INFO(VT_RED, fmt, ## args)
#define INFO_GREEN(fmt, args...)   _INFO(VT_GREEN, fmt, ## args)
#define INFO_BLUE(fmt, args...)    _INFO(VT_BLUE, fmt, ## args)

#define QQVGA_WIDTH  160
#define QQVGA_HEIGHT 120

#define QVGA_WIDTH  320
#define QVGA_HEIGHT 240

#define CIF_WIDTH   352
#define CIF_HEIGHT  288

#define VGA_WIDTH   640
#define VGA_HEIGHT  480

#define SVGA_WIDTH	800
#define SVGA_HEIGHT	600

#define XGA_WIDTH	1024
#define XGA_HEIGHT	768

#define WXGA_WIDTH	1280
#define WXGA_HEIGHT	720

#define V720P_WIDTH (WXGA_WIDTH)
#define V720P_HEIGHT (WXGA_HEIGHT)

#define SXGA_WIDTH	1280
#define SXGA_HEIGHT	960

#define V1080P_WIDTH 1920
#define V1080P_HEIGHT 1080

#define UXGA_WIDTH	1600
#define UXGA_HEIGHT	1200

#define QXGA_WIDTH	2048
#define QXGA_HEIGHT	1536

#define QSXGA_WIDTH	    2592
#define QSXGA_HEIGHT	1944


struct v4l2_ctl_cmd_info {
    unsigned int  id;
    int min;
    int max;
    unsigned int step;
    int def;
};

struct v4l2_ctl_cmd_info_menu {
    unsigned int  id;
    int max;
    int mask;
    int def;
};

/* 
  * for Edge ctrl 
  * 
  * strength also control Auto or Manual Edge Control Mode 
  * see also OV2643_MANUAL_EDGE_CTRL 
  */
struct module_edge_ctrl {	
	unsigned char strength;	
	unsigned char threshold;	
	unsigned char upper;	
	unsigned char lower;
};

/* 
  * module info 
  */
struct module_info 
{	
	unsigned long		flags;	
	struct module_edge_ctrl	edgectrl;	
	unsigned int video_devnum;
};


struct dts_gpio {
    int num;
    int active_level;  /* 1: high level active, 0:low level active */
};

struct dts_regulator {
    struct regulator *regul;
    unsigned int min; /* uV */
    unsigned int max;
};

struct isp_regulators {
    int avdd_use_gpio;  /* 0: regul, 1: use gpio */
    union {
        struct dts_gpio gpio;
        struct dts_regulator regul;
    } avdd;

    int dvdd_use_gpio;  /* 0: regul, 1: use gpio */
    struct dts_gpio dvdd_gpio;
    struct dts_regulator dvdd;
};

struct sensor_pwd_info {
    int flag;  /* sensor supports: front only, rear only, or dual */
    struct dts_gpio gpio_rear;
    struct dts_gpio gpio_front;
    struct dts_gpio gpio_reset;
    struct clk *ch_clk[2];
};

struct dts_sensor_config {
    int rear; /* 1: rear sensor, 0: front sensor */
    int channel; /* 0: channel-1, 1: channel-2 */
    int data_type; /* 0: output YUV data, 1: RAW data */
    int host; /* bus_id of soc_camera_link */
    int i2c_adapter;
    enum v4l2_mbus_type bus_type; /* dvp or mipi */
    struct device *dev; /* sensor's platform device */
    struct device_node *dn;
    struct pinctrl *mfp;
};

#define DECLARE_DTS_SENSOR_CFG(name) \
    static struct dts_sensor_config name = { \
        .rear = 1, .host = 0, .i2c_adapter = 1, \
    }

/**
 * set current sensor mode, preview(or video) and capture
 */
enum {
    ACTS_ISP_PREVIEW_MODE = 0,
    ACTS_ISP_CAPTURE_MODE,
    ACTS_ISP_VIDEO_MODE,
};

/**
 * using sensor as a front/rear camera
 */
enum {
    ACTS_CAM_SENSOR_FRONT = 0,
    ACTS_CAM_SENSOR_REAR,
};


#define V4L2_CID_CAM_CV_MODE _IOW('v', BASE_VIDIOC_PRIVATE + 0, int)


void attach_sensor_pwd_info(struct device *dev, 
    struct sensor_pwd_info *pi, int host_id);
void detach_sensor_pwd_info(struct device *dev, 
    struct sensor_pwd_info *pi, int host_id);
void owl_isp_reset(struct device *dev, int host_id);
int owl_isp_power_on(int channel, int rear, int host_id);
int owl_isp_power_off(int channel, int rear, int host_id);
int parse_config_info(struct soc_camera_link *link,
    struct dts_sensor_config *dsc, const char *name);

extern int owl_isp_reset_pin_state;
#endif  //__ISP_OWL_H__

