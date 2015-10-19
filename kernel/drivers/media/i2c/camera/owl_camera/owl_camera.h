/*
 * gl5206_camera.h - GL5206 camera driver header file
 */

#ifndef _OWL_CAMERA_H_
#define _OWL_CAMERA_H_

#include <linux/device.h>
#include <linux/videodev2.h>
#include <media/soc_mediabus.h>
#include <mach/isp-owl.h>

#define owl_camera_hw_call(hw,f,args...)  ((!(hw)) ? -ENODEV : (((hw)->ops->f) ? ((hw)->ops->f((hw), ##args)) : -ENOIOCTLCMD))

#define SENSOR_FRONT 0x1
#define SENSOR_REAR 0x2
#define SENSOR_DUAL 0x4

#define DEFAULT_MBUS_PARAM_DVP \
    (OWL_CAMERA_DATA_HIGH | OWL_CAMERA_PCLK_RISING | \
    OWL_CAMERA_HSYNC_HIGH | OWL_CAMERA_VSYNC_HIGH | \
    OWL_CAMERA_DATAWIDTH_8)

#define DEFAULT_MBUS_PARAM_MIPI \
    (V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_2_LANE | \
    V4L2_MBUS_CSI2_CONTINUOUS_CLOCK)


#define OWL_CAMERA_DATA_HIGH		1
#define OWL_CAMERA_PCLK_RISING	2
#define OWL_CAMERA_HSYNC_HIGH	4
#define OWL_CAMERA_VSYNC_HIGH	8
#define OWL_CAMERA_DATAWIDTH_4	0x10
#define OWL_CAMERA_DATAWIDTH_8	0x20
#define OWL_CAMERA_DATAWIDTH_10	0x40
#define OWL_CAMERA_DATAWIDTH_16	0x80

#define CONTEXT_DT_RAW8  (0x2A)
#define CONTEXT_DT_RAW10 (0x2B)
#define CONTEXT_DT_RAW12 (0x2C)



#define CAMERA_CSI2_CHANNEL_0 (1 << 9)
#define CAMERA_CSI2_CHANNEL_1 (1 << 10)
#define CAMERA_CSI2_CONTINUOUS_CLOCK (1 << 11)
#define CAMERA_CSI2_NONCONTINUOUS_CLOCK (1 << 12)
#define CAMERA_CSI2_LANE_1 (1 << 13)
#define CAMERA_CSI2_LANE_2 (1 << 14)
#define CAMERA_CSI2_LANE_3 (1 << 15)
#define CAMERA_CSI2_LANE_4 (1 << 16)


#define CAM_HOST_NAME "camera_host"
#define CAM_HOST2_NAME "camera_host2"

#define CAMERA_DATAWIDTH_MASK (CAMERA_DATAWIDTH_4 | CAMERA_DATAWIDTH_8 | \
				   CAMERA_DATAWIDTH_10 | CAMERA_DATAWIDTH_16)


enum owl_dev_state {
    DEV_STOP = 0,
    DEV_START,
    DEV_SUSPEND,
    DEV_RESUME,
    DEV_OPEN,
    DEV_CLOSE,
};

struct owl_camera_hw_adapter;

struct owl_camera_reg{
	volatile void * reg;
	u32 val;
};
extern volatile void*	si_clk;

extern void*	noc_si_to_ddr;	
extern void*	gpio_dinen;
extern void* si_reset;



struct owl_camera_hw_ops
{
	int (*hw_adapter_init)(struct owl_camera_hw_adapter* hw,struct platform_device *pdev);
    int (*get_channel_state)(struct owl_camera_hw_adapter* hw, int channel);
    int (*set_channel_if)(struct owl_camera_hw_adapter* hw, int channel, int bus_type);
    int (*set_channel_addrY)(struct owl_camera_hw_adapter* hw, int channel, void *addrY);
    int (*set_channel_addrU)(struct owl_camera_hw_adapter* hw, int channel, void *addrU);
    int (*set_channel_addrV)(struct owl_camera_hw_adapter* hw, int channel, void *addrV);
    int (*set_channel_addr1UV)(struct owl_camera_hw_adapter* hw, int channel, void *addr1UV);
    int (*set_channel_input_fmt)(struct owl_camera_hw_adapter* hw, int channel, enum v4l2_mbus_pixelcode code);
    int (*set_channel_output_fmt)(struct owl_camera_hw_adapter* hw, int channel, u32 fourcc);
    int (*set_channel_preline)(struct owl_camera_hw_adapter* hw, int channel, int preline);
	int (*set_channel_int_en)(struct owl_camera_hw_adapter* hw, int channel);
    int (*clear_channel_int_en)(struct owl_camera_hw_adapter* hw, int channel);
    int (*set_channel_preline_int_en)(struct owl_camera_hw_adapter* hw, int channel);
    int (*set_channel_frameend_int_en)(struct owl_camera_hw_adapter* hw, int channel);
	int (*clear_channel_frameend_int_en)(struct owl_camera_hw_adapter* hw, int channel);
	int (*clear_all_pending)(struct owl_camera_hw_adapter* hw);
	int (*clear_channel_preline_int_en)(struct owl_camera_hw_adapter* hw);
    int (*clear_channel_preline_pending)(struct owl_camera_hw_adapter* hw, int channel);
    int (*clear_channel_frameend_pending)(struct owl_camera_hw_adapter* hw, unsigned int isp_int_stat);
	int (*set_signal_polarity)(struct owl_camera_hw_adapter* hw,int channel,unsigned int common_flags);
	int (*set_col_range)(struct owl_camera_hw_adapter* hw,int channel,unsigned int start,unsigned int end);
	int (*set_row_range)(struct owl_camera_hw_adapter* hw,int channel,unsigned int start,unsigned int end); 
	int (*get_channel_overflow)(struct owl_camera_hw_adapter* hw, int channel);
	int (*clear_channel_overflow)(struct owl_camera_hw_adapter* hw, int channel); 
	int (*save_regs)(struct owl_camera_hw_adapter* hw);
	int (*restore_regs)(struct owl_camera_hw_adapter* hw); 
};


struct owl_camera_hw_adapter {
    char *hw_name;
	struct owl_camera_reg *restored_regs;
	int restored_regs_num;
    struct owl_camera_dev *cam_dev;

    int max_channel;
    int has_isp;
    int is_3D_support;
    int crop_x_align;
    int crop_y_align;
    int crop_w_align;
    int crop_h_align;
    int max_width;
    int max_height;
	int preline_int_pd;
	int frameend_int_pd;

    int isp_state;

    int power_ref;
    int enable_ref;

    struct clk *hw_clk;

    struct owl_camera_hw_ops *ops;

    void *priv;

};


/*
 * OWL have tow channel, if support all, should register tow soc camera host
 */
struct owl_camera_dev {

    struct soc_camera_device *icd;

    unsigned int dvp_mbus_flags;
    struct soc_camera_host soc_host;

    struct list_head capture;
    spinlock_t lock;            /* Protects video buffer lists */
    struct vb2_buffer *cur_frm;
    struct vb2_buffer *prev_frm;

    struct vb2_alloc_ctx *alloc_ctx;
    unsigned int sequence;
    struct completion wait_stop;

    int irq;

    int started;
    struct pinctrl *mfp;
    int skip_frames;

    struct sensor_pwd_info spinfo;
    struct isp_regulators ir;
    struct clk *isp_clk;
    struct clk *csi_clk;
    struct clk *ch_clk[2]; /* correspond to ISP_CHANNEL_0/1 (should be 0 or 1) */
	struct owl_camera_hw_adapter *hw_adapter;

};

struct owl_camera_param {
    /* ISP data offsets within croped by the GL5203 camera output */
    unsigned int isp_left;
    unsigned int isp_top;
    /* Client output, as seen by the GL5203 */
    unsigned int width;
    unsigned int height;
    /*
     * User window from S_CROP / G_CROP, produced by client cropping,
     * GL5203 cropping, mapped back onto the client
     * input window
     */
    struct v4l2_rect subrect;
    /* Camera cropping rectangle */
    struct v4l2_rect rect;
    const struct soc_mbus_pixelfmt *extra_fmt;
    enum v4l2_mbus_pixelcode code;
    unsigned long flags;
    unsigned int skip_frames;
    int channel;
    enum v4l2_mbus_type bus_type;
    int lane_num;
    int raw_width;
    int data_type;//0 for yuv sensor,1 for raw-bayer sensor
    int (*ext_cmd)(struct v4l2_subdev *sd,int cmd,void *args);
};



#define debug_print(fmt, arg...) do { if (1) printk(KERN_ERR fmt, ##arg); } while (0)

#define isp_info(fmt, ...) \
    printk(KERN_INFO fmt, ## __VA_ARGS__)
    
#define isp_err(fmt, ...) \
    printk(KERN_ERR "%s(L%d) error: " fmt, __func__, __LINE__, ## __VA_ARGS__)


extern struct owl_camera_hw_adapter atm7059_hw_adapter;
extern struct owl_camera_hw_adapter atm7039_hw_adapter;
#endif
