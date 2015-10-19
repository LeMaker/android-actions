/*
 * V4L2 Driver for OWL camera host
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-common.h>
#include <media/v4l2-dev.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/slab.h>
#include <media/soc_camera.h>
#include <media/soc_mediabus.h>
#include <linux/v4l2-mediabus.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/time.h>
#include <mach/hardware.h>
#include <asm/delay.h>
#include <mach/module-owl.h>
#include <mach/clkname.h>
#include <mach/powergate.h>
#include <linux/pinctrl/consumer.h>
#include <linux/mfd/atc260x/atc260x.h>
#include <media/videobuf2-memops.h>



#include "owl_camera.h"
//#include"atm7039c.c"
#include"atm7059.c"


static struct owl_camera_hw_adapter* hw_adapter = NULL;



#define ISP_FDT_COMPATIBLE "actions,owl-isp"

#define	DEBUG_FRAME_RATE 0

//#define DBG_INFO(fmt, args...)  printk(KERN_INFO"[owl_camera] (INFO) line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)
#define DBG_ERR(fmt, args...)   printk(KERN_ERR"[owl_camera] (ERR) line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)
#ifdef OWL_DBG
#define DBG_INFO(fmt, args...)  printk(KERN_INFO"[owl_camera] (ERR) line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args) 
#else 
#define DBG_INFO(fmt, args...)  do{}while(0)
#endif

//#define DBG_ERR(fmt, args...)   printk(KERN_ERR"[owl_camera] (ERR) line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)



#define ISP_MAX_WIDTH	4288
#define ISP_MAX_HEIGHT	3000
#define ISP_WORK_CLOCK	60000000        //Hz
//#define ISP_WORK_CLOCK        120000000
#define CSI_WORK_CLOCK  150000000

#define OWL_CAM_HOST_NAME "owl-camera-host"
#define CAM_DRV_NAME OWL_CAM_HOST_NAME

#define MAX_VIDEO_MEM 32
#define ISP_REG_NUM_WORD    189
#define DROP_FRAM_INIT_VAL  0xff



#define ISP_MIN_FRAME_RATE 2
#define ISP_FRAME_INTVL_MSEC	(1000 / ISP_MIN_FRAME_RATE)



struct owl_camera_buffer {
    /* common v4l buffer stuff -- must be first */
    struct vb2_buffer vb;
    struct list_head queue;
    enum v4l2_mbus_pixelcode code;
};



static inline void isp_set_preline(struct soc_camera_device *icd)
{
    struct owl_camera_param *param = icd->host_priv;
  	int ret = 0;
	
	//DBG_INFO("channel:%d, top:%d", param->channel, param->isp_top);
	
	ret = owl_camera_hw_call(hw_adapter, set_channel_preline, param->channel, param->isp_top);
}


static inline int isp_set_col_range(struct soc_camera_device *icd, unsigned int start,
                                    unsigned int end)
{
    struct owl_camera_param *cam_param = icd->host_priv;
    
    int width;
 	int ret = 0;
    start = ((start + 1) / 2) * 2;
    width = end - start + 1;
    width = ((width + 31) / 32) * 32;
    end = width + start - 1;
	
	//DBG_INFO("width:%d, start:%d, end:%d", width, start, end);

    if (width >= ISP_MAX_WIDTH) {
        DBG_ERR("width is over range, width:%d, start:%d, end:%d", width, start, end);
        return -EINVAL;
    }
	ret = owl_camera_hw_call(hw_adapter, set_col_range,cam_param->channel, start, end);
	
    return 0;
}

static inline int isp_set_row_range(struct soc_camera_device *icd, unsigned int start,
                                    unsigned int end)
{
    struct owl_camera_param *cam_param = icd->host_priv;
    int height = end - start + 1;
	int ret = 0;

    height = ((height + 1) / 2) * 2;
    end = height + start - 1;

   // DBG_INFO("height:%d, start:%d, end:%d", height, start, end);
    if (height > ISP_MAX_HEIGHT) {
        DBG_ERR("height is over range, height:%d, start:%d, end:%d", height, start, end);
        return -EINVAL;
    }
	ret = owl_camera_hw_call(hw_adapter, set_row_range,cam_param->channel, start, end);
	
    return 0;
}

/* rect is guaranteed to not exceed the camera rectangle */
static inline void isp_set_rect(struct soc_camera_device *icd)
{
    struct owl_camera_param *cam_param = icd->host_priv;

    isp_set_col_range(icd, cam_param->isp_left, cam_param->isp_left + icd->user_width - 1);
    isp_set_row_range(icd, cam_param->isp_top, cam_param->isp_top + icd->user_height - 1);
}


static int isp_set_input_fmt(struct soc_camera_device *icd, enum v4l2_mbus_pixelcode code)
{
    struct owl_camera_param *cam_param = icd->host_priv;
   
	int ret = 0;

	//DBG_INFO("");
	ret = owl_camera_hw_call(hw_adapter, set_channel_input_fmt, cam_param->channel, code);
    return ret;
}

static int isp_set_output_fmt(struct soc_camera_device *icd, u32 fourcc)
{
    struct owl_camera_param *cam_param = icd->host_priv;
   	int ret = 0; 

	//DBG_INFO("");

    ret = owl_camera_hw_call(hw_adapter, set_channel_output_fmt, cam_param->channel, fourcc);
    return ret;
}

static void isp_set_frame(struct owl_camera_dev *cam_dev, struct vb2_buffer *vb)
{
    struct soc_camera_device *icd = cam_dev->icd;
    struct owl_camera_param *cam_param = icd->host_priv;
    u32 fourcc = icd->current_fmt->host_fmt->fourcc;

    dma_addr_t isp_addr;
    dma_addr_t isp_addr_Y;
    dma_addr_t isp_addr_U;
    dma_addr_t isp_addr_V;
    dma_addr_t isp_addr_UV;


    if (NULL == vb) {
        DBG_ERR( "cannot get video buffer.");
        return;
    }
	DBG_INFO("");
    switch (vb->v4l2_buf.memory) {
    case V4L2_MEMORY_MMAP:
        isp_addr = vb2_dma_contig_plane_dma_addr(vb, 0);
        break;
    case V4L2_MEMORY_USERPTR:
        isp_addr = vb->v4l2_planes[0].m.userptr;
        break;
    default:
        DBG_ERR("wrong memory type.");
        return;
    }

    DBG_INFO("frame paddr:0x%x", isp_addr);

    switch (fourcc) {
    case V4L2_PIX_FMT_YUV420:      //420 planar
        DBG_INFO("420p");
        isp_addr_Y = ALIGN(isp_addr, 2);
        isp_addr_U = ALIGN(isp_addr_Y + icd->user_width * icd->user_height, 2);
        isp_addr_V = ALIGN(isp_addr_U + icd->user_width * icd->user_height / 4, 2);
		owl_camera_hw_call(hw_adapter, set_channel_addrY, cam_param->channel, (void*)isp_addr_Y);
		owl_camera_hw_call(hw_adapter, set_channel_addrU, cam_param->channel, (void*)isp_addr_U);
		owl_camera_hw_call(hw_adapter, set_channel_addrV, cam_param->channel, (void*)isp_addr_V);
     
        break;
	 case V4L2_PIX_FMT_YVU420:      //420 planar
        DBG_INFO("420p");
        isp_addr_Y = ALIGN(isp_addr, 2);
        isp_addr_U = ALIGN(isp_addr_Y + icd->user_width * icd->user_height, 2);
        isp_addr_V = ALIGN(isp_addr_U + icd->user_width * icd->user_height / 4, 2);
		owl_camera_hw_call(hw_adapter, set_channel_addrY, cam_param->channel, (void*)isp_addr_Y);
		owl_camera_hw_call(hw_adapter, set_channel_addrU, cam_param->channel, (void*)isp_addr_V);
		owl_camera_hw_call(hw_adapter, set_channel_addrV, cam_param->channel, (void*)isp_addr_U);
     
        break;	

    case V4L2_PIX_FMT_YUV422P:     //422 semi planar
        DBG_INFO("422p");
        isp_addr_Y = ALIGN(isp_addr, 2);
        isp_addr_U = ALIGN(isp_addr_Y + icd->user_width * icd->user_height, 2);
        isp_addr_V = ALIGN(isp_addr_U + icd->user_width * icd->user_height / 4, 2);
        owl_camera_hw_call(hw_adapter, set_channel_addrY, cam_param->channel, (void*)isp_addr_Y);
        owl_camera_hw_call(hw_adapter, set_channel_addrU, cam_param->channel, (void*)isp_addr_V);
        owl_camera_hw_call(hw_adapter, set_channel_addrV, cam_param->channel, (void*)isp_addr_U);

        break;

    case V4L2_PIX_FMT_NV12:        //420 semi-planar
    case V4L2_PIX_FMT_NV21:        //420 semi-planar
        DBG_INFO("420sp");
        isp_addr_Y = ALIGN(isp_addr, 2);
        isp_addr_UV = ALIGN(isp_addr_Y + icd->user_width * icd->user_height, 2);
		owl_camera_hw_call(hw_adapter, set_channel_addrY, cam_param->channel, (void*)isp_addr_Y);
		owl_camera_hw_call(hw_adapter, set_channel_addrU, cam_param->channel, (void*)isp_addr_UV);
      
        break;

    case V4L2_PIX_FMT_YUYV:        //interleaved
	case V4L2_PIX_FMT_UYVY:
        DBG_INFO("422P");
        isp_addr_Y = ALIGN(isp_addr, 2);
        isp_addr_U = ALIGN(isp_addr_Y + icd->user_width * icd->user_height, 2);
        isp_addr_V = ALIGN(isp_addr_U + icd->user_width * icd->user_height / 4, 2);
        owl_camera_hw_call(hw_adapter, set_channel_addrY, cam_param->channel, (void*)isp_addr_Y);
        owl_camera_hw_call(hw_adapter, set_channel_addrU, cam_param->channel, (void*)isp_addr_V);
        owl_camera_hw_call(hw_adapter, set_channel_addrV, cam_param->channel, (void*)isp_addr_U);
        break;

    default:       /* Raw RGB */
        DBG_ERR("Set isp output format failed, fourcc = 0x%x", fourcc);
        return;
    }
}


static int isp_set_signal_polarity(struct soc_camera_device *icd, unsigned int common_flags)
{
    struct owl_camera_param *cam_param = icd->host_priv;

	DBG_INFO("channel:%d, pclk:%s, hsync:%s, vsync:%s",
		cam_param->channel, 
		common_flags & V4L2_MBUS_PCLK_SAMPLE_FALLING ? "falling" : "rising",
		common_flags & V4L2_MBUS_HSYNC_ACTIVE_LOW ? "low" : "high",
		common_flags & V4L2_MBUS_VSYNC_ACTIVE_LOW ? "low" : "high");

        if(!owl_camera_hw_call(hw_adapter,set_signal_polarity,cam_param->channel,common_flags))
        	{
        	
			return 0;
        	}
	return 0;
  
}


static void isp_capture_start(struct owl_camera_dev *cam_dev)
{
	int ret = 0;
    struct soc_camera_device *icd = cam_dev->icd;
    struct owl_camera_param *cam_param = icd->host_priv;
	
    if (cam_dev->started == DEV_START) {
        DBG_INFO("already start");
        return;
    }

    if (V4L2_MBUS_CSI2 == cam_param->bus_type) {
        //mipi_set_mbus(cam_dev);
    }

    cam_dev->skip_frames = DROP_FRAM_INIT_VAL;
    isp_set_preline(icd);
	ret |= owl_camera_hw_call(cam_dev->hw_adapter, set_channel_if, cam_param->channel,cam_param->bus_type);
	ret |= owl_camera_hw_call(hw_adapter, set_channel_preline_int_en, cam_param->channel);
	ret |= owl_camera_hw_call(hw_adapter, clear_all_pending);
	
	ret |= owl_camera_hw_call(hw_adapter, set_channel_int_en, cam_param->channel);
	
    cam_dev->started = DEV_START;

    DBG_INFO("chnnel-%d, %s", cam_param->channel,
             V4L2_MBUS_PARALLEL == cam_param->bus_type ? "dvp" : "mipi");

    
}

static void isp_capture_stop(struct owl_camera_dev *cam_dev)
{
    struct soc_camera_device *icd = cam_dev->icd;
    struct owl_camera_param *cam_param = icd->host_priv;
    unsigned long flags;
    int ret;
	
	//DBG_INFO("");

    if (cam_dev->started == DEV_STOP) {
        DBG_INFO("already stop");
        return;
    }

    spin_lock_irqsave(&cam_dev->lock, flags);
	
	ret |= owl_camera_hw_call(hw_adapter, clear_channel_int_en, cam_param->channel);
	ret |= owl_camera_hw_call(hw_adapter, clear_channel_preline_pending, cam_param->channel);
	ret |= owl_camera_hw_call(hw_adapter, set_channel_frameend_int_en, cam_param->channel);
	
    cam_dev->started = DEV_STOP;
    spin_unlock_irqrestore(&cam_dev->lock, flags);

    ret = wait_for_completion_timeout(&cam_dev->wait_stop, msecs_to_jiffies(ISP_FRAME_INTVL_MSEC));
    if (ret <= 0) {
        DBG_ERR("%s wake up before frame complete", __func__);
    } else {
        DBG_INFO("%s ret is %d remain %dms to wait for stop", __func__, ret,jiffies_to_msecs(ret));
    }
    if (V4L2_MBUS_CSI2 == cam_param->bus_type) {
     //   mipi_disable(cam_dev);
    }
}


static inline struct owl_camera_buffer *to_isp_buf(struct vb2_buffer *vb)
{
    return (container_of(vb, struct owl_camera_buffer, vb));
}

/* make sure capture list is not empty */
static inline struct vb2_buffer *next_vb2_buf(struct owl_camera_dev *ispdev)
{
    struct owl_camera_buffer *cb = NULL;

    cb = list_entry(ispdev->capture.next, struct owl_camera_buffer, queue);
    list_del_init(&cb->queue);

    return (&cb->vb);
}

static inline int get_bytes_per_line(struct soc_camera_device *icd)
{
    return (soc_mbus_bytes_per_line(icd->user_width, icd->current_fmt->host_fmt));
}

static inline int get_frame_size(struct soc_camera_device *icd)
{
    int bytes_per_line = get_bytes_per_line(icd);
    return (icd->user_height * bytes_per_line);
}


static int owl_isp_videobuf_setup(struct vb2_queue *vq,
    const struct v4l2_format *fmt, unsigned int *count,
    unsigned int *num_planes, unsigned int sizes[], void *alloc_ctxs[])
{
    struct soc_camera_device *icd = soc_camera_from_vb2q(vq);
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct owl_camera_dev *ispdev = ici->priv;
    unsigned long frame_size = get_frame_size(icd);

	//DBG_INFO("");

    if (frame_size < 0)
        return (frame_size);

    *num_planes = 1;

    ispdev->sequence = 0;
    sizes[0] = frame_size;
    alloc_ctxs[0] = ispdev->alloc_ctx;

    if (*count < 2) {
        *count = 2;
    }

    if (sizes[0] * *count > MAX_VIDEO_MEM * 1024 * 1024)
        *count = MAX_VIDEO_MEM * 1024 * 1024 / sizes[0];

    dev_dbg(icd->parent, "count=%d, size=%d", *count, sizes[0]);
	//DBG_INFO("Exit");
    return (0);

}

static int owl_isp_videobuf_prepare(struct vb2_buffer *vb)
{
    struct soc_camera_device *icd = soc_camera_from_vb2q(vb->vb2_queue);
    struct owl_camera_buffer *buf = NULL;
    unsigned long frame_size = get_frame_size(icd);;

	//DBG_INFO("");

    if (frame_size < 0) {
        return (frame_size);
    }

    buf = to_isp_buf(vb);

    dev_dbg(icd->parent, "%s (vb=0x%p) 0x%p %lu", __func__,
            vb, vb2_plane_vaddr(vb, 0), vb2_get_plane_payload(vb, 0));

    /* Added list head initialization on alloc */
    if (!list_empty(&buf->queue)) {
        dev_err(icd->parent, "Buffer %p on queue already!", vb);
    }
#ifdef DEBUG
    /*
     * This can be useful if you want to see if we actually fill
     * the buffer with something
     */
    if (vb2_plane_vaddr(vb, 0)) {
        memset(vb2_plane_vaddr(vb, 0), 0xaa, vb2_get_plane_payload(vb, 0));
    }
#endif

    BUG_ON(NULL == icd->current_fmt);

    if (vb2_plane_size(vb, 0) < frame_size) {
        dev_err(icd->parent, "Buffer too small (%lu < %lu)", vb2_plane_size(vb, 0), frame_size);
        return (-ENOBUFS);
    }

    vb2_set_plane_payload(vb, 0, frame_size);
	//DBG_INFO("Exit");
    return (0);

}


static void owl_isp_videobuf_queue(struct vb2_buffer *vb)
{
    struct soc_camera_device *icd = soc_camera_from_vb2q(vb->vb2_queue);
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct owl_camera_dev *ispdev = ici->priv;
    struct owl_camera_buffer *buf = to_isp_buf(vb);
    unsigned long flag;

	//DBG_INFO("");

    dev_dbg(icd->parent, "%s (vb=0x%p) 0x%p %lu", __func__,
            vb, vb2_plane_vaddr(vb, 0), vb2_get_plane_payload(vb, 0));

    spin_lock_irqsave(&ispdev->lock, flag);
    list_add_tail(&buf->queue, &ispdev->capture);
    if (NULL == ispdev->cur_frm) {      // first one to enable isp
        /* Is better to move into start_streaming, but HAL start stream before qbuf */
        ispdev->cur_frm = next_vb2_buf(ispdev);
        ispdev->prev_frm = NULL;
    //    updata_bDBG_INFO(sd, cam_param);
        spin_unlock_irqrestore(&ispdev->lock, flag);

        isp_set_rect(icd);
        isp_set_frame(ispdev, ispdev->cur_frm);
        isp_capture_start(ispdev);
    } else {
        spin_unlock_irqrestore(&ispdev->lock, flag);
    }
	//DBG_INFO("Exit  ");

}


static void owl_isp_videobuf_release(struct vb2_buffer *vb)
{
    struct soc_camera_device *icd = soc_camera_from_vb2q(vb->vb2_queue);
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct owl_camera_buffer *buf = to_isp_buf(vb);
    struct owl_camera_dev *ispdev = ici->priv;
    unsigned long flag;

	//DBG_INFO("");

    spin_lock_irqsave(&ispdev->lock, flag);

    if (ispdev->cur_frm == vb) {
        ispdev->cur_frm = NULL;
    }

    if (ispdev->prev_frm == vb) {
        ispdev->prev_frm = NULL;
    }

    if(buf != NULL && buf->queue.next != NULL) {
        list_del_init(&buf->queue);
    }

    spin_unlock_irqrestore(&ispdev->lock, flag);
	//DBG_INFO("Exit");
}


static int owl_isp_videobuf_init(struct vb2_buffer *vb)
{
    struct owl_camera_buffer *buf = to_isp_buf(vb);

	

    INIT_LIST_HEAD(&buf->queue);
	//DBG_INFO("Exit");
    return (0);
}

static int owl_isp_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	DBG_INFO("");

#if 0
    struct soc_camera_device *icd = soc_camera_from_vb2q(vq);
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct owl_camera_dev *ispdev = ici->priv;
    unsigned long flag;

    dev_dbg(icd->parent, "%s()", __func__);
    BUG_ON(NULL != ispdev->cur_frm);
    BUG_ON(NULL != ispdev->prev_frm);

    spin_lock_irqsave(&ispdev->lock, flag);


    if (count > 1 && !list_empty(&ispdev->capture)) {   // at least 2 bufs
        ispdev->cur_frm = next_vb2_buf(ispdev);
        ispdev->prev_frm = NULL;

        isp_set_rect(icd);
        isp_set_frame(ispdev, ispdev->cur_frm);
        isp_capture_start(ispdev, icd);

    }

    spin_unlock_irqrestore(&ispdev->lock, flag);
#endif
    return (0);
}


static int owl_isp_stop_streaming(struct vb2_queue *vq)
{
    struct soc_camera_device *icd = soc_camera_from_vb2q(vq);
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct owl_camera_dev *ispdev = ici->priv;
    struct owl_camera_buffer *buf, *node;
    unsigned long flag;

	DBG_INFO("");

    spin_lock_irqsave(&ispdev->lock, flag);

    ispdev->cur_frm = NULL;
    list_for_each_entry_safe(buf, node, &ispdev->capture, queue) {
        list_del_init(&buf->queue);
        vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
    }

    spin_unlock_irqrestore(&ispdev->lock, flag);

    isp_capture_stop(ispdev);

    return (0);
}


static struct vb2_ops owl_isp_videobuf_ops = {
    .queue_setup = owl_isp_videobuf_setup,
    .buf_prepare = owl_isp_videobuf_prepare,
    .buf_queue = owl_isp_videobuf_queue,
    .buf_cleanup = owl_isp_videobuf_release,
    .buf_init = owl_isp_videobuf_init,
    .wait_prepare = soc_camera_unlock,
    .wait_finish = soc_camera_lock,
    .start_streaming = owl_isp_start_streaming,
    .stop_streaming = owl_isp_stop_streaming,
};


/** 
 * using preline interrupt of current active buffer to indicate that previous buffer
 * have received all data from sensor, unlike old manner to indicate current buffer's.
 * so ISP_PRELINE_NUM should be as small as possible.
 * to do this taking tow reasons into account:
 * 1. no need to check or wait for frame end interrupt pending which is less useful for ISR.
 * 2. max time of  interrupt response delay shoud be less than about 30ms for 30 fps.
 *     if interval time between preline-interrupt and vsync is less than 30ms, 
 *     there is a risk that ISR receive tow preline interrupt between 
 *     tow adjacent vsync, a buffer will be skipped leaving old data in it.
 */

struct vb2_buffer *tmp_vbbuffer = NULL;


static irqreturn_t owl_camera_host_isp_isr(int irq, void *data)
{
	
    struct owl_camera_dev *cam_dev = data;
    struct soc_camera_device *icd = cam_dev->icd;
    struct owl_camera_param *cam_param = icd->host_priv;
	int reset = 0;
   // struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
    struct vb2_buffer *vb = NULL;
    int ret;
    unsigned long flags;
    unsigned int preline_int_pd,frameend_int_pd;
	unsigned int isp_int_stat,tmp_int_stat;
	//ret = owl_camera_hw_call(hw_adapter, clear_channel_frameend_int_en,cam_param->channel);
	DBG_INFO("Enter isr");
	reset = 0;
	preline_int_pd = hw_adapter->preline_int_pd;
	frameend_int_pd = hw_adapter->frameend_int_pd;
	spin_lock_irqsave(&cam_dev->lock, flags);
	isp_int_stat = owl_camera_hw_call(hw_adapter, get_channel_state,cam_param->channel);
	tmp_int_stat = isp_int_stat;
	 if (DROP_FRAM_INIT_VAL == cam_dev->skip_frames) {
        cam_dev->skip_frames = cam_param->skip_frames;
    }

    if ((cam_dev->skip_frames) && (isp_int_stat & preline_int_pd)) {
        cam_dev->skip_frames--;
        DBG_INFO("skip frame: %d", cam_dev->skip_frames);
        goto out;
    }
	
	if(owl_camera_hw_call(hw_adapter, get_channel_overflow,cam_param->channel))
		{
			printk(KERN_ERR"si is over flow , reset si\n");
		
			owl_camera_hw_call(hw_adapter, clear_channel_overflow, cam_param->channel);
			writel(readl(si_reset)|(0x1<<31),si_reset);
			ret |= owl_camera_hw_call(hw_adapter, clear_channel_int_en, cam_param->channel);
			mdelay(1);
			owl_camera_hw_call(hw_adapter, save_regs);
			
			module_reset(MODULE_RST_BISP);
			
			owl_camera_hw_call(hw_adapter, restore_regs);
			isp_set_rect(icd);
			isp_set_frame(cam_dev, cam_dev->prev_frm);
			writel(readl(si_reset)&(~(0x1<<31)),si_reset);
			ret |= owl_camera_hw_call(hw_adapter, set_channel_int_en, cam_param->channel);
			tmp_vbbuffer = cam_dev->cur_frm;
			cam_dev->cur_frm = cam_dev->prev_frm;
			cam_dev->prev_frm = NULL;
			reset = 1;
			
		}
	if(reset)
	{
		DBG_INFO("in the reset\n");
		goto out;
	}
	 if (cam_dev->started == DEV_STOP&&(tmp_int_stat&frameend_int_pd)) {
        printk(KERN_INFO"found stop in isp_isr");
		//mdelay(1);
		ret |= owl_camera_hw_call(hw_adapter, clear_channel_frameend_int_en, cam_param->channel);
        complete(&cam_dev->wait_stop);
        goto out;
    }
	if(tmp_int_stat & preline_int_pd)
	{
	ret = owl_camera_hw_call(hw_adapter, clear_channel_preline_pending,isp_int_stat);
	}
  	if(tmp_int_stat & frameend_int_pd)
	{
	ret = owl_camera_hw_call(hw_adapter, clear_channel_frameend_pending,isp_int_stat);
	}
	//DBG_INFO("channel:%d, isr S intstat 0x%08x,preline_int_pd  0x%08x", cam_param->channel, isp_int_stat,preline_int_pd);
    if (isp_int_stat & preline_int_pd) {
        // send out a packet already recevied all data
        if (cam_dev->prev_frm != NULL) {
            vb = cam_dev->prev_frm;
            do_gettimeofday(&vb->v4l2_buf.timestamp);
            vb->v4l2_buf.sequence = cam_dev->sequence++;
            vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

           // DBG_INFO("prev_frm");

        }

        if (!list_empty(&cam_dev->capture)) {
            
			
			cam_dev->prev_frm = cam_dev->cur_frm;
			if(tmp_vbbuffer)
				{
				cam_dev->cur_frm = tmp_vbbuffer;
				}
			else
				{
				cam_dev->cur_frm = next_vb2_buf(cam_dev);
				}
				
            isp_set_rect(icd);
            //updata_bisp_info(sd, cam_param);
            isp_set_frame(cam_dev, cam_dev->cur_frm);
         //   DBG_INFO("list not empty, isp_set_frame");

           // BUG_ON(cam_dev->prev_frm == cam_dev->cur_frm);

        } else {
            cam_dev->prev_frm = NULL;
            isp_set_rect(icd);
        //    updata_bisp_info(sd, cam_param);
            isp_set_frame(cam_dev, cam_dev->cur_frm);
          // DBG_INFO("list empty, isp_set_frame");
        }

    }
	tmp_vbbuffer = NULL;
  out:
  	if(tmp_int_stat & preline_int_pd)
		{
		ret = owl_camera_hw_call(hw_adapter, clear_channel_preline_pending,isp_int_stat);
		}
  	if((tmp_int_stat & frameend_int_pd))
  		{
		ret = owl_camera_hw_call(hw_adapter, clear_channel_frameend_pending,isp_int_stat);
  		}
    spin_unlock_irqrestore(&cam_dev->lock, flags);

    return IRQ_HANDLED;
}




static void isp_regulator_enable(struct isp_regulators *ir);
static void isp_regulator_disable(struct isp_regulators *ir);


/* Called with .video_lock held */
static int owl_camera_add_device(struct soc_camera_device *icd)
{
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct owl_camera_dev *cam_dev = ici->priv;
    struct clk *isp_clk = cam_dev->isp_clk;
    int err;

    if (cam_dev->icd) {
        DBG_ERR("devices has already exists.");
        return -EBUSY;
    }
    config_inner_charger_current(DEV_CHARGER_PRE_CONFIG, DEV_CHARGER_CURRENT_CAMERA, 1);
	err = owl_powergate_power_on(OWL_POWERGATE_VCE_BISP);
	if (err) {
		DBG_ERR("owl powergate power-on error %d", err);
		return err;
	}
	module_reset(MODULE_RST_BISP);
	module_clk_enable(MOD_ID_BISP);

    clk_prepare(isp_clk);
    err = clk_enable(isp_clk);
    if (err) {
        DBG_ERR("enable isp clock failed %d", err);
        return err;
    }
  
    err = clk_set_rate(isp_clk, ISP_WORK_CLOCK);
    if (err) {
        DBG_ERR("set isp clock rate failed %d", err);
        return err;
    }
    DBG_INFO("isp clock is %dM", (int) (clk_get_rate(isp_clk) / 1000000));

   
    isp_regulator_enable(&cam_dev->ir);

    cam_dev->started = DEV_OPEN;
    owl_isp_reset_pin_state = 1;
    cam_dev->icd = icd;
    config_inner_charger_current(DEV_CHARGER_POST_CONFIG, DEV_CHARGER_CURRENT_CAMERA, 1);
    pm_runtime_get_sync(ici->v4l2_dev.dev);
	writel(0xf,noc_si_to_ddr);
    DBG_INFO("[add_device] isp attached to sensor %d", icd->devnum);

    return 0;
}

/* Called with .video_lock held */
static void owl_camera_remove_device(struct soc_camera_device *icd)
{
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct owl_camera_dev *cam_dev = ici->priv;
    struct clk *isp_clk = cam_dev->isp_clk;
    unsigned long flags;
    int err = 0;

    BUG_ON(icd != cam_dev->icd);

    /* make sure active buffer is canceled */

    BUG_ON(NULL != cam_dev->cur_frm);
    BUG_ON(NULL != cam_dev->prev_frm);

    spin_lock_irqsave(&cam_dev->lock, flags);
    cam_dev->started = DEV_CLOSE;
    owl_isp_reset_pin_state = 0;
    spin_unlock_irqrestore(&cam_dev->lock, flags);

    pm_runtime_put_sync(ici->v4l2_dev.dev);
    config_inner_charger_current(DEV_CHARGER_PRE_CONFIG, DEV_CHARGER_CURRENT_CAMERA, 0);
    isp_regulator_disable(&cam_dev->ir);

    clk_disable(isp_clk);
    clk_unprepare(isp_clk);
    clk_put(isp_clk);
    module_clk_disable(MOD_ID_BISP);

    err = owl_powergate_power_off(OWL_POWERGATE_VCE_BISP);
    if (err) {
        DBG_ERR("owl powergate power-off err %d", err);
    }

  
    config_inner_charger_current(DEV_CHARGER_POST_CONFIG, DEV_CHARGER_CURRENT_CAMERA, 0);
    cam_dev->icd = NULL;
    DBG_INFO("[remove_device] isp detached from sensor %d", icd->devnum);

}

static int get_supported_mbus_param(struct owl_camera_param *cam_param)
{
    int flags = 0;
    /*
     * OWL camera interface only works in master mode
     * and only support 8bits currently
     */
    if (V4L2_MBUS_PARALLEL == cam_param->bus_type) {
        flags = V4L2_MBUS_MASTER |
            V4L2_MBUS_HSYNC_ACTIVE_HIGH |
            V4L2_MBUS_HSYNC_ACTIVE_LOW |
            V4L2_MBUS_VSYNC_ACTIVE_HIGH |
            V4L2_MBUS_VSYNC_ACTIVE_LOW |
            V4L2_MBUS_PCLK_SAMPLE_RISING |
            V4L2_MBUS_PCLK_SAMPLE_FALLING |
            V4L2_MBUS_DATA_ACTIVE_HIGH |
            SOCAM_DATAWIDTH_8;

    } else if (V4L2_MBUS_CSI2 == cam_param->bus_type) {
        flags = V4L2_MBUS_CSI2_CHANNEL_0 |
            V4L2_MBUS_CSI2_CHANNEL_1 |
            V4L2_MBUS_CSI2_CONTINUOUS_CLOCK |
            V4L2_MBUS_CSI2_NONCONTINUOUS_CLOCK |
            V4L2_MBUS_CSI2_LANES;
    } else {
        DBG_ERR("bus_type is not supported");
    }

    return flags;
}


static int select_dvp_mbus_param(struct owl_camera_dev *cdev, unsigned int common_flags)
{
    if ((common_flags & V4L2_MBUS_HSYNC_ACTIVE_HIGH)
        && (common_flags & V4L2_MBUS_HSYNC_ACTIVE_LOW)) {
        if (cdev->dvp_mbus_flags & OWL_CAMERA_HSYNC_HIGH) {
            common_flags &= ~V4L2_MBUS_HSYNC_ACTIVE_LOW;
        } else {
            common_flags &= ~V4L2_MBUS_HSYNC_ACTIVE_HIGH;
        }
    }

    if ((common_flags & V4L2_MBUS_VSYNC_ACTIVE_HIGH)
        && (common_flags & V4L2_MBUS_VSYNC_ACTIVE_LOW)) {
        if (cdev->dvp_mbus_flags & OWL_CAMERA_VSYNC_HIGH) {
            common_flags &= ~V4L2_MBUS_VSYNC_ACTIVE_LOW;
        } else {
            common_flags &= ~V4L2_MBUS_VSYNC_ACTIVE_HIGH;
        }
    }

    if ((common_flags & V4L2_MBUS_DATA_ACTIVE_HIGH)
        && (common_flags & V4L2_MBUS_DATA_ACTIVE_LOW)) {
        if (cdev->dvp_mbus_flags & OWL_CAMERA_DATA_HIGH) {
            common_flags &= ~V4L2_MBUS_DATA_ACTIVE_LOW;
        } else {
            common_flags &= ~V4L2_MBUS_DATA_ACTIVE_HIGH;
        }
    }
	
    if ((common_flags & V4L2_MBUS_PCLK_SAMPLE_RISING)
        && (common_flags & V4L2_MBUS_PCLK_SAMPLE_FALLING)) {
        if (cdev->dvp_mbus_flags & OWL_CAMERA_PCLK_RISING) {
            common_flags &= ~V4L2_MBUS_PCLK_SAMPLE_FALLING;
        } else {
            common_flags &= ~V4L2_MBUS_PCLK_SAMPLE_RISING;
        }
    }
	//common_flags |= V4L2_MBUS_PCLK_SAMPLE_RISING;
	DBG_INFO("OWL,common_flags is %x",common_flags);
    return common_flags;
}


static int select_mipi_mbus_param(struct owl_camera_dev *cdev, unsigned int common_flags)
{
    struct soc_camera_device *icd = cdev->icd;
    struct owl_camera_param *param = icd->host_priv;

    if (common_flags & V4L2_MBUS_CSI2_1_LANE) {
        param->lane_num = 0;    /* correspond to cis  CSI_CTRL_LANE_NUM's defined */
    } else if (common_flags & V4L2_MBUS_CSI2_2_LANE) {
        param->lane_num = 1;
    } else if (common_flags & V4L2_MBUS_CSI2_3_LANE) {
        param->lane_num = 2;
    } else {
        param->lane_num = 3;
    }
    // determined by CSI, ignore pclk polarity
    return V4L2_MBUS_HSYNC_ACTIVE_HIGH | V4L2_MBUS_VSYNC_ACTIVE_LOW;
}

static int owl_camera_set_bus_param(struct soc_camera_device *icd)
{
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
    struct owl_camera_dev *cam_dev = ici->priv;
    struct owl_camera_param *cam_param = icd->host_priv;
    struct v4l2_mbus_config cfg;

    unsigned int bus_flags;
    unsigned int common_flags;
    //u32 cmu_sensorclk, isp_ctl, isp2_ctl;
    int ret;


    bus_flags = get_supported_mbus_param(cam_param);

    v4l2_subdev_call(sd, video, g_mbus_config, &cfg);
    DBG_INFO("get mbus flags host[0x%x] sensor[0x%x]", bus_flags, cfg.flags);

    common_flags = soc_mbus_config_compatible(&cfg, bus_flags);
    if (!common_flags) {
        DBG_ERR("flags incompatible: host 0x%x, sensor 0x%x", bus_flags, cfg.flags);
        return -EINVAL;
    }

    if (V4L2_MBUS_PARALLEL == cam_param->bus_type) {
        common_flags = select_dvp_mbus_param(cam_dev, common_flags);
    } else {
        common_flags = select_mipi_mbus_param(cam_dev, common_flags);
    }

    cfg.flags = common_flags;
    ret = v4l2_subdev_call(sd, video, s_mbus_config, &cfg);
    if (ret < 0 && ret != -ENOIOCTLCMD) {
        DBG_ERR("camera s_mbus_config(0x%x) returned %d", common_flags, ret);
        return ret;
    }

    DBG_INFO("mbus flags host[0x%x] sensor[0x%x] comm[x%x]", bus_flags, cfg.flags, common_flags);

    isp_set_signal_polarity(icd, common_flags);
    isp_set_input_fmt(icd, cam_param->code);

    return 0;
}

static int check_mbus_param(struct soc_camera_device *icd)
{
    struct owl_camera_param *cam_param = icd->host_priv;

    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
    struct v4l2_mbus_config cfg;
    unsigned long bus_flags, common_flags;
    int ret;

    bus_flags = get_supported_mbus_param(cam_param);

    ret = v4l2_subdev_call(sd, video, g_mbus_config, &cfg);
    if (!ret) {
        common_flags = soc_mbus_config_compatible(&cfg, bus_flags);
        if (!common_flags) {
            DBG_ERR("flags incompatible: sensor[0x%x], host[0x%lx]", cfg.flags, bus_flags);
            return -EINVAL;
        }
    } else if (ret == -ENOIOCTLCMD) {
        ret = 0;
    }

    return ret;
}

static const struct soc_mbus_pixelfmt owl_camera_formats[] = {
    {
        .fourcc = V4L2_PIX_FMT_YUV420,
        .name = "YUV 4:2:0 planar 12 bit",
        .bits_per_sample = 12,
        .packing = SOC_MBUS_PACKING_NONE,
        .order = SOC_MBUS_ORDER_LE,
    },
	{    .fourcc = V4L2_PIX_FMT_YVU420, 
		 .name = "YVU 4:2:0 planar 12 bit",        
		 .bits_per_sample = 12,        
		 .packing = SOC_MBUS_PACKING_NONE,        
		 .order = SOC_MBUS_ORDER_LE,    
    },
    {
        .fourcc = V4L2_PIX_FMT_YUV422P,
        .name = "YUV 4:2:2 planar 16 bit",
        .bits_per_sample = 16,
        .packing = SOC_MBUS_PACKING_NONE,
        .order = SOC_MBUS_ORDER_LE,
    },
    {
        .fourcc = V4L2_PIX_FMT_YUYV,
        .name = "YUYV 4:2:2 interleaved 16bit",
        .bits_per_sample = 16,
        .packing = SOC_MBUS_PACKING_NONE,
        .order = SOC_MBUS_ORDER_LE,
    },
    {
        .fourcc = V4L2_PIX_FMT_NV12,
        .name = "YUV 4:2:0 semi-planar 12 bit",
        .bits_per_sample = 12,
        .packing = SOC_MBUS_PACKING_NONE,
        .order = SOC_MBUS_ORDER_LE,
    },
{        .fourcc = V4L2_PIX_FMT_NV21,        
		 .name = "YUV 4:2:0 semi-planar 12 bit",       
		 .bits_per_sample = 12,        
		 .packing = SOC_MBUS_PACKING_NONE,        
		 .order = SOC_MBUS_ORDER_LE,    
    },  
};


static int client_g_rect(struct v4l2_subdev *sd, struct v4l2_rect *rect)
{
    struct v4l2_crop crop;
    struct v4l2_cropcap cap;
    int ret;

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = v4l2_subdev_call(sd, video, g_crop, &crop);
    if (!ret) {
        *rect = crop.c;
        return ret;
    }

    /* Camera driver doesn't support .g_crop(), assume default rectangle */
    cap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = v4l2_subdev_call(sd, video, cropcap, &cap);
    if (!ret)
        *rect = cap.defrect;

    return ret;
}

/*
 * The common for both scaling and cropping iterative approach is:
 * 1. try if the client can produce exactly what requested by the user
 * 2. if (1) failed, try to double the client image until we get one big enough
 * 3. if (2) failed, try to request the maximum image
 */
static int client_s_crop(struct soc_camera_device *icd, const struct v4l2_crop *crop,
                         struct v4l2_crop *cam_crop)
{
    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
    const  struct v4l2_rect *rect = &crop->c;
	struct v4l2_rect *cam_rect = &cam_crop->c;
    //      struct owl_camera_param *cam = icd->host_priv;
    struct v4l2_cropcap cap;
    int ret;
    unsigned int width, height;
	unsigned int rect_left = rect->left;
	unsigned int rect_width = rect->width;
	unsigned int rect_top = rect->top;
	unsigned int rect_height = rect->height;
    //      v4l2_subdev_call(sd, video, s_crop, crop);
    ret = client_g_rect(sd, cam_rect);
    if (ret < 0) {
        DBG_ERR("get sensor rect failed %d", ret);
        return ret;
    }

    /* Try to fix cropping, that camera hasn't managed to set */
    DBG_INFO("fix camera S_CROP for %dx%d@%d:%d to %dx%d@%d:%d",
             cam_rect->width, cam_rect->height, cam_rect->left, cam_rect->top, 
             rect->width, rect->height, rect->left, rect->top);

    /* We need sensor maximum rectangle */
    ret = v4l2_subdev_call(sd, video, cropcap, &cap);
    if (ret < 0) {
        DBG_ERR("get sensor cropcap failed %d", ret);
        return ret;
    }

    /* Put user requested rectangle within sensor and isp bounds */
	
    soc_camera_limit_side(&rect_left, &rect_width, cap.bounds.left, 32,
                          min(ISP_MAX_WIDTH, cap.bounds.width));
    /* TO FIXUP: must be 32B-aligned if not support stride */
    //rect->width = rect->width - (rect->width % 32);

    soc_camera_limit_side(&rect_top, &rect_height, cap.bounds.top, 1,
                          min(ISP_MAX_HEIGHT, cap.bounds.height));

    /*
     * Popular special case - some cameras can only handle fixed sizes like
     * QVGA, VGA,... Take care to avoid infinite loop.
     */
    width = max(cam_rect->width, 2);
    height = max(cam_rect->height, 2);

    if (!ret && (cap.bounds.width >= width || cap.bounds.height >= height)
        && ((rect->left >= cam_rect->left) && (rect->top >= cam_rect->top)
            && (rect->left + rect->width <= cam_rect->left + cam_rect->width)
            && (rect->top + rect->height <= cam_rect->top + cam_rect->height))) {
        return 0;
    } else {
        DBG_ERR("crop rect must be within sensor rect.");
        return -ERANGE;
    }
}

extern void bisp_updata(unsigned int *pHyaddr, int isp_ch);
extern int af_updata(int isp_ch);
static int ext_cmd(struct v4l2_subdev *sd, int cmd, void *args)
{
    int ret = 0;

    switch (cmd) {
    //case V4L2_CID_BISP_UPDATE:
    //    bisp_updata(0, 0);      // note channel's num
    //    break;
    //case V4L2_CID_AF_UPDATE:
    //    af_updata(0);
    //    break;
    default:
        printk("getctrl invalid control id %d", cmd);
        return -EINVAL;
    }
    //DBG_INFO("%s, cmd:%#x",__func__, cmd);

    return ret;
}

static int parse_sensor_flags(struct owl_camera_param *param, unsigned long flags)
{
    if (flags & SENSOR_FLAG_CH_MASK) {
        param->channel = ISP_CHANNEL_0;
    } else {
        param->channel = ISP_CHANNEL_1;
    }

    if (flags & SENSOR_FLAG_INTF_MASK) {
        param->ext_cmd = NULL;
        param->bus_type = V4L2_MBUS_PARALLEL;
    } else {
        param->ext_cmd = ext_cmd;
        param->bus_type = V4L2_MBUS_CSI2;
    }

    if (flags & SENSOR_FLAG_DATA_MASK) {
        param->data_type = SENSOR_DATA_TYPE_YUV;
    } else {
        param->data_type = SENSOR_DATA_TYPE_RAW;
    }
    DBG_INFO("%s: channel[%d], bus[%s], data[%s]", __func__, param->channel,
             V4L2_MBUS_PARALLEL == param->bus_type ? "dvp" : "mipi",
             SENSOR_DATA_TYPE_YUV == param->data_type ? "yuv" : "raw");

   // if (flags & SENSOR_FLAG_8BIT) {
    //    param->raw_width = CONTEXT_DT_RAW8;
    //} else if (flags & SENSOR_FLAG_10BIT) {
        param->raw_width = CONTEXT_DT_RAW10;
    //} else if (flags & SENSOR_FLAG_12BIT) {
    //    param->raw_width = CONTEXT_DT_RAW12;
    //} else {
    //    param->raw_width = 0;
    //}
    return 0;
}

static int owl_camera_get_formats(struct soc_camera_device *icd, unsigned int idx,
                                     struct soc_camera_format_xlate *xlate)
{
    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
    struct owl_camera_param *cam_param;
    int formats = 0, ret, k, n;
    enum v4l2_mbus_pixelcode code;
    const struct soc_mbus_pixelfmt *fmt;
    struct device *pdev = icd->pdev;
    struct soc_camera_link *module_link = pdev->platform_data;
    struct module_info *module_info = module_link->priv;


    ret = v4l2_subdev_call(sd, video, enum_mbus_fmt, idx, &code);
    if (ret < 0) {
        /* No more formats */
        DBG_ERR("No more formats, %s, %d", __func__, __LINE__);
        return ret;
    }

    fmt = soc_mbus_get_fmtdesc(code);
    if (!fmt) {
        DBG_ERR("invalid format code #%u: %d", idx, code);
        return -EINVAL;
    }
    //DBG_INFO("request bus width %d bit", fmt->bits_per_sample);

    if (!icd->host_priv) {
        struct v4l2_mbus_framefmt mf;
        struct v4l2_rect rect;

        /* Cache current client geometry */
        ret = client_g_rect(sd, &rect);
        if (ret < 0)
            return ret;

        /* First time */
        ret = v4l2_subdev_call(sd, video, g_mbus_fmt, &mf);
        if (ret < 0) {
            DBG_ERR("failed to g_mbus_fmt.");
            return ret;
        }

        DBG_INFO("isp get sensor's default fmt %ux%u", mf.width, mf.height);

        cam_param = kzalloc(sizeof(*cam_param), GFP_KERNEL);
        if (NULL == cam_param) {
            DBG_ERR("alloc camera_param struct failed");
            return -ENOMEM;
        }

        /* We are called with current camera crop, initialise subrect with it */
        cam_param->rect = rect;
        cam_param->subrect = rect;

        cam_param->width = mf.width;
        cam_param->height = mf.height;
        cam_param->isp_left = 0;
        cam_param->isp_top = 0;
        cam_param->flags = module_info->flags;
        cam_param->skip_frames = 0;
        icd->host_priv = cam_param;

        parse_sensor_flags(cam_param, module_info->flags);

        /* This also checks support for the requested bits-per-sample */
        ret = check_mbus_param(icd);
        if (ret < 0) {
            kfree(cam_param);
            icd->host_priv = NULL;
            DBG_ERR("no right formats, %s, %d", __func__, __LINE__);
            return ret;
        }
    } else {
        cam_param = icd->host_priv;
    }

    /* Beginning of a pass */
    if (!idx) {
        cam_param->extra_fmt = NULL;
    }

    switch (code) {
    case V4L2_MBUS_FMT_UYVY8_2X8:
    case V4L2_MBUS_FMT_SGBRG8_1X8:
    case V4L2_MBUS_FMT_SBGGR8_1X8:
    case V4L2_MBUS_FMT_SRGGB8_1X8:
    case V4L2_MBUS_FMT_SGRBG8_1X8:{
            if (cam_param->extra_fmt)
                break;

            /*
             * Our case is simple so far: for any of the above four camera
             * formats we add all our four synthesized NV* formats, so,
             * just marking the device with a single flag suffices. If
             * the format generation rules are more complex, you would have
             * to actually hang your already added / counted formats onto
             * the host_priv pointer and check whether the format you're
             * going to add now is already there.
             */
            cam_param->extra_fmt = owl_camera_formats;

            n = ARRAY_SIZE(owl_camera_formats);
            formats += n;
            DBG_INFO("isp provide output format:");
            for (k = 0; xlate && k < n; k++) {
                xlate->host_fmt = &owl_camera_formats[k];
                xlate->code = code;
                xlate++;
                DBG_INFO("[%d].code-%#x, %s", k, code, owl_camera_formats[k].name);
            }
            break;
        }
    default:
        //DBG_INFO("code:0x%x is not supported", code);
        return 0;

    }

    /* Generic pass-through */
    formats++;
    if (xlate) {
        xlate->host_fmt = fmt;
        xlate->code = code;
        DBG_INFO("xlate->code = %#x", xlate->code);
        xlate++;
    }

    return formats;
}

static void owl_camera_put_formats(struct soc_camera_device *icd)
{
    if (icd->host_priv) {
        kfree(icd->host_priv);
    }
    icd->host_priv = NULL;
}


static int check_frame_range(u32 width, u32 height)
{
    /* limit to owl_camera hardware capabilities */
    return height < 1 || height > ISP_MAX_HEIGHT || width < 32
        || width > ISP_MAX_WIDTH /* || (width % 32) */ ;
}

static int owl_camera_cropcap(struct soc_camera_device *icd, struct v4l2_cropcap *a)
{
    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
    struct v4l2_rect *rect = &a->defrect;
    int ret;

    ret = v4l2_subdev_call(sd, video, cropcap, a);
    if (ret < 0) {
        DBG_ERR("failed to get camera cropcap");
        return ret;
    }

    /* Put user requested rectangle within sensor and isp bounds */
    if (rect->width > ISP_MAX_WIDTH) {
        rect->width = ISP_MAX_WIDTH;
    }
    rect->width = rect->width - (rect->width % 32);

    if (rect->height > ISP_MAX_HEIGHT) {
        rect->height = ISP_MAX_HEIGHT;
    }

    return 0;
}

/*
 * OWL_CAMERA can not crop or scale for YUV,but can crop for RawData.
 * And we don't want to waste bandwidth and kill the
 * framerate by always requesting the maximum image from the client.
 */
static int owl_camera_set_crop(struct soc_camera_device *icd, const struct v4l2_crop *a)
{
    const struct v4l2_rect *rect = &a->c;
    struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    struct owl_camera_dev *cam_dev = ici->priv;
    struct owl_camera_param *cam_param = icd->host_priv;
    struct v4l2_crop cam_crop;
    struct v4l2_rect *cam_rect = &cam_crop.c;
    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
    struct v4l2_mbus_framefmt mf;
    unsigned long flags;
    int ret;

    DBG_INFO("S_CROP(%ux%u@%u:%u)", rect->width, rect->height, rect->left, rect->top);

    //      /* During camera cropping its output window can change too, stop OWL_CAMERA */
    //      ISP_ENABLE = owl_camera_capture_save_reset(cam_dev);

    /* For UYVY/Bayer Raw input data */
    /* 1. - 2. Apply iterative camera S_CROP for new input window. */
    ret = client_s_crop(icd, a, &cam_crop);
    if (ret < 0)
        return ret;
    DBG_INFO("1-2: camera cropped to %ux%u@%u:%u",
             cam_rect->width, cam_rect->height, cam_rect->left, cam_rect->top);

    /* On success cam_crop contains current camera crop */

    /* 3. Retrieve camera output window */
    ret = v4l2_subdev_call(sd, video, g_mbus_fmt, &mf);
    if (ret < 0) {
        DBG_ERR("failed to g_mbus_fmt.");
        return ret;
    }

    if (check_frame_range(mf.width, mf.height)) {
        DBG_ERR("inconsistent state. Use S_FMT to repair");
        return -EINVAL;
    }

    spin_lock_irqsave(&cam_dev->lock, flags);

    /* Cache camera output window */
    cam_param->width = mf.width;
    cam_param->height = mf.height;

    icd->user_width = rect->width;
    icd->user_height = rect->height;
    cam_param->isp_left = rect->left - cam_rect->left;
    cam_param->isp_top = rect->top - cam_rect->top;

    /* 4. Use OWL_CAMERA cropping to crop to the new window. */

    cam_param->rect = *cam_rect;
    cam_param->subrect = *rect;

    spin_unlock_irqrestore(&cam_dev->lock, flags);

    DBG_INFO("4: isp cropped to %ux%u@%u:%u", icd->user_width, 
             icd->user_height, cam_param->isp_left, cam_param->isp_top);

    return ret;
}

static int owl_camera_get_crop(struct soc_camera_device *icd, struct v4l2_crop *a)
{
    struct owl_camera_param *cam_param = icd->host_priv;

    a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    a->c = cam_param->subrect;
    return 0;
}


static int owl_camera_set_fmt(struct soc_camera_device *icd, struct v4l2_format *f)
{
    //struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
    //struct owl_camera_dev *cam_dev = ici->priv;
    struct owl_camera_param *cam_param = icd->host_priv;
    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
    const struct soc_camera_format_xlate *xlate = NULL;
    struct v4l2_pix_format *pix = &f->fmt.pix;
    struct v4l2_mbus_framefmt mf;
    __u32 pixfmt = pix->pixelformat;
    unsigned int skip_frames_num;
    int ret;


    DBG_INFO("%s, S_FMT(pix=0x%x, %ux%u)", __func__, pixfmt, pix->width, pix->height);

    /* sensor may skip different some frames */
    ret = v4l2_subdev_call(sd, sensor, g_skip_frames, &skip_frames_num);
    if (ret < 0) {
        skip_frames_num = 0;
    }
    cam_param->skip_frames = skip_frames_num;

    xlate = soc_camera_xlate_by_fourcc(icd, pixfmt);
    if (!xlate) {
        DBG_INFO("format %x not found", pixfmt);
        return -EINVAL;
    }

    DBG_INFO("set format %dx%d", pix->width, pix->height);

    mf.width = pix->width;
    mf.height = pix->height;
    mf.field = pix->field;
    mf.colorspace = pix->colorspace;
    mf.code = xlate->code;

    ret = v4l2_subdev_call(sd, video, s_mbus_fmt, &mf);
    if (ret < 0) {
        DBG_ERR("failed to configure for format %x", pix->pixelformat);
        return ret;
    }

    if (mf.code != xlate->code) {
        DBG_ERR("wrong code: mf.code = 0x%x, xlate->code = 0x%x", mf.code, xlate->code);
        return -EINVAL;
    }


    if (check_frame_range(mf.width, mf.height)) {
        DBG_ERR("sensor produced an unsupported frame %dx%d", mf.width, mf.height);
        return -EINVAL;
    }

    cam_param->isp_left = 0;
    cam_param->isp_top = 0;
    cam_param->width = mf.width;
    cam_param->height = mf.height;
    cam_param->code = xlate->code;

    pix->bytesperline = soc_mbus_bytes_per_line(pix->width, xlate->host_fmt);
    if (pix->bytesperline < 0) {
        DBG_ERR("bytesperline %d not correct.", pix->bytesperline);
        return pix->bytesperline;
    }
    pix->sizeimage = pix->height * pix->bytesperline;
    pix->width = mf.width;
    pix->height = mf.height;
    pix->field = mf.field;
    pix->colorspace = mf.colorspace;

    icd->current_fmt = xlate;
    icd->user_width = cam_param->width;
    icd->user_height = cam_param->height;

    DBG_INFO("sensor set %dx%d, code = %#x", pix->width, pix->height, cam_param->code);

    isp_set_rect(icd);
    ret = isp_set_output_fmt(icd, icd->current_fmt->host_fmt->fourcc);
    if(ret)
        return ret;

    DBG_INFO("set output data format %s", icd->current_fmt->host_fmt->name);

    return 0;
}

static int owl_camera_try_fmt(struct soc_camera_device *icd, struct v4l2_format *f)
{
    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
    const struct soc_camera_format_xlate *xlate;
    struct v4l2_pix_format *pix = &f->fmt.pix;
    struct v4l2_mbus_framefmt mf;
    __u32 pixfmt = pix->pixelformat;
    int ret;
    xlate = soc_camera_xlate_by_fourcc(icd, pixfmt);
    if (pixfmt && !xlate) {
        DBG_ERR("format %x not found", pixfmt);
        return -EINVAL;
    }

    /*
     * Limit to OWL_CAMERA hardware capabilities.  YUV422P planar format requires
     * images size to be a multiple of 16 bytes.  If not, zeros will be
     * inserted between Y and U planes, and U and V planes, which violates
     * the YUV422P standard.
     */
    v4l_bound_align_image(&pix->width, 32, ISP_MAX_WIDTH, 5,
                          &pix->height, 1, ISP_MAX_HEIGHT, 0,
                          pixfmt == V4L2_PIX_FMT_YUV422P ? 4 : 0);

    pix->bytesperline = soc_mbus_bytes_per_line(pix->width, xlate->host_fmt);
    if (pix->bytesperline < 0) {
        DBG_ERR("bytesperline %d not correct.", pix->bytesperline);
        return pix->bytesperline;
    }
    pix->sizeimage = pix->height * pix->bytesperline;

    /* limit to sensor capabilities */
    mf.width = pix->width;
    mf.height = pix->height;
    mf.field = pix->field;
    mf.colorspace = pix->colorspace;
    mf.code = xlate->code;

    DBG_INFO("try %dx%d", mf.width, mf.height);

    ret = v4l2_subdev_call(sd, video, try_mbus_fmt, &mf);
    if (ret < 0) {
        return ret;
    }

    pix->width = mf.width;
    pix->height = mf.height;
    //      pix->field      = mf.field;
    pix->colorspace = mf.colorspace;

    switch (mf.field) {
    case V4L2_FIELD_ANY:
    case V4L2_FIELD_NONE:
        pix->field = V4L2_FIELD_NONE;
        break;
    default:
        DBG_ERR("field type %d unsupported.", mf.field);
        ret = -EINVAL;
    }

    return ret;
}

static int owl_camera_reqbufs(struct soc_camera_device *icd, struct v4l2_requestbuffers *p)
{
    int i;

    /*
     * This is for locking debugging only. I removed spinlocks and now I
     * check whether .prepare is ever called on a linked buffer, or whether
     * a dma IRQ can occur for an in-work or unlinked buffer. Until now
     * it hadn't triggered
     */
    for (i = 0; i < p->count; i++) {
        struct owl_camera_buffer *buf;

        buf = to_isp_buf(icd->vb2_vidq.bufs[i]);
        INIT_LIST_HEAD(&buf->queue);
    }

    return 0;
}

static unsigned int owl_camera_poll(struct file *file, poll_table * pt)
{
    struct soc_camera_device *icd = file->private_data;

    return vb2_poll(&icd->vb2_vidq, file, pt);
}

static int owl_camera_querycap(struct soc_camera_host *ici, struct v4l2_capability *cap)
{
    /* cap->name is set by the firendly caller:-> */
    strlcpy(cap->card, "OWL Camera", sizeof(cap->card));
    cap->version = KERNEL_VERSION(0, 0, 1);
    cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

    return 0;
}


struct vb2_dc_buf {
	struct device			*dev;
	void				*vaddr;
	unsigned long			size;
	dma_addr_t			dma_addr;
	enum dma_data_direction		dma_dir;
	struct sg_table			*dma_sgt;

	/* MMAP related */
	struct vb2_vmarea_handler	handler;
	atomic_t			refcount;
	struct sg_table			*sgt_base;

	/* USERPTR related */
	struct vm_area_struct		*vma;

	/* DMABUF related */
	struct dma_buf_attachment	*db_attach;
};
static void *owl_vb2_get_userptr(void *alloc_ctx, unsigned long vaddr,
	unsigned long size, int write)
{
	struct vb2_dc_buf  *buf;
	int ret = 0;
	unsigned long pfn;
	void* now_vaddr=NULL;
	
	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	pfn =vaddr>>PAGE_SHIFT;
	if (pfn_valid(pfn)) {
		
		now_vaddr = phys_to_virt(pfn << PAGE_SHIFT);
	} else {
		
		now_vaddr = ioremap(pfn << PAGE_SHIFT, PAGE_SIZE);
	}
	if (!now_vaddr)
		ret = -ENOMEM;
	if (ret) {
		printk(KERN_ERR "Failed acquiring vaddr 0x%08lx\n",
				(unsigned long)vaddr);
		kfree(buf);
		return ERR_PTR(ret);
	}
	buf->size = size;
	buf->vaddr = now_vaddr;
	buf->dma_addr = vaddr;
	buf->vma = NULL;

	return buf;
}

static void owl_vb2_put_userptr(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;
	void* vaddr;
	unsigned long pfn;

	if (!buf)
		return;
	pfn = buf->dma_addr>>PAGE_SHIFT;
	vaddr = buf->vaddr;
	if (!pfn_valid(pfn))
		iounmap(vaddr);
	kfree(buf);
}

struct vb2_mem_ops owl_dma_contig_memops;


static int owl_camera_init_videobuf(struct vb2_queue *q, struct soc_camera_device *icd)
{
    DBG_INFO("%s, %d", __func__, __LINE__);

    q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    q->io_modes = VB2_MMAP | VB2_USERPTR;
    q->drv_priv = icd;
    q->ops = &owl_isp_videobuf_ops;
	memcpy(&owl_dma_contig_memops,&vb2_dma_contig_memops,sizeof(struct vb2_mem_ops));
	owl_dma_contig_memops.get_userptr	= owl_vb2_get_userptr;
	owl_dma_contig_memops.put_userptr   = owl_vb2_put_userptr;
    q->mem_ops = &owl_dma_contig_memops;
    q->buf_struct_size = sizeof(struct owl_camera_buffer);
    q->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;

    return (vb2_queue_init(q));
}


static int owl_camera_get_parm(struct soc_camera_device *icd, struct v4l2_streamparm *parm)
{
    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);

    return v4l2_subdev_call(sd, video, g_parm, parm);
}

static int owl_camera_set_parm(struct soc_camera_device *icd, struct v4l2_streamparm *parm)
{
    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);

    return v4l2_subdev_call(sd, video, s_parm, parm);
}

static int owl_camera_suspend(struct device *dev)
{
    // usually when system suspend, it will close camera, so no need process more
    struct soc_camera_host *ici = to_soc_camera_host(dev);
    struct owl_camera_dev *cam_dev = container_of(ici, struct owl_camera_dev,soc_host);

    disable_irq(cam_dev->irq);
    //writel(0, ISP_ENABLE);
    module_clk_disable(MOD_ID_BISP);


	isp_regulator_disable(&cam_dev->ir);
	
    DBG_INFO("enter camera suspend...");
    return 0;
}
static void isp_regulator_enable_resume(struct isp_regulators *ir)
{
	int ret = 0;
	if (ir->dvdd.regul) {
		ret = regulator_enable(ir->dvdd.regul);
	}

	if (ir->avdd_use_gpio) {
		struct dts_gpio *dg = &ir->avdd.gpio;
		gpio_direction_output(dg->num, dg->active_level);
	} else {
		struct dts_regulator *dr = &ir->avdd.regul;
		if (dr->regul) {
			ret = regulator_enable(dr->regul);
		}
	}

	if (ir->dvdd_use_gpio) {
		struct dts_gpio *dvdd_dg = &ir->dvdd_gpio;
		gpio_direction_output(dvdd_dg->num, dvdd_dg->active_level);
	}
}

static int owl_camera_resume(struct device *dev)
{
    struct soc_camera_host *ici = to_soc_camera_host(dev);
    struct owl_camera_dev *cam_dev = container_of(ici, struct owl_camera_dev,soc_host);
	isp_regulator_enable_resume(&cam_dev->ir);
    module_reset(MODULE_RST_BISP);
    module_clk_enable(MOD_ID_BISP);
    //writel(readl(ISP_INT_STAT), ISP_INT_STAT);
    enable_irq(cam_dev->irq);
    DBG_INFO("enter camera resume...");
    return 0;
}

int owl_camera_enum_fsizes(struct soc_camera_device *icd, struct v4l2_frmsizeenum *fsize)
{
    struct v4l2_subdev *sd = soc_camera_to_subdev(icd);

    return v4l2_subdev_call(sd, video, enum_framesizes, fsize);
}


static struct soc_camera_host_ops owl_soc_camera_host_ops = {
    .owner = THIS_MODULE,
    .add = owl_camera_add_device,
    .remove = owl_camera_remove_device,
    //      .suspend = owl_camera_suspend,
    //      .resume = owl_camera_resume,
    .get_formats = owl_camera_get_formats,
    .put_formats = owl_camera_put_formats,
    .cropcap = owl_camera_cropcap,
    .get_crop = owl_camera_get_crop,
    .set_crop = owl_camera_set_crop,
    .set_livecrop = owl_camera_set_crop,
    .set_fmt = owl_camera_set_fmt,
    .try_fmt = owl_camera_try_fmt,
    //      .set_ctrl = owl_camera_set_ctrl,
    //      .get_ctrl = owl_camera_get_ctrl,
    .set_parm = owl_camera_set_parm,
    .get_parm = owl_camera_get_parm,
    .reqbufs = owl_camera_reqbufs,
    .poll = owl_camera_poll,
    .querycap = owl_camera_querycap,
    .set_bus_param = owl_camera_set_bus_param,
    .init_videobuf2 = owl_camera_init_videobuf,
    .enum_framesizes = owl_camera_enum_fsizes,
};


static inline void sensor_pwd_info_init(struct sensor_pwd_info *spinfo)
{
    spinfo->flag = 0;
    spinfo->gpio_rear.num = -1;
    spinfo->gpio_front.num = -1;
    spinfo->gpio_reset.num = -1;
    spinfo->ch_clk[ISP_CHANNEL_0] = NULL;
    spinfo->ch_clk[ISP_CHANNEL_1] = NULL;
}

static inline void isp_regulators_init(struct isp_regulators *ir)
{
    struct dts_regulator *dr = &ir->avdd.regul;

    dr->regul = NULL;
    ir->avdd_use_gpio = 0;
    ir->dvdd_use_gpio = 0;
    ir->dvdd.regul = NULL;
}

static int camera_clock_init(struct owl_camera_dev *cdev)
{
    struct sensor_pwd_info *spinfo = &cdev->spinfo;
    struct clk *tmp;
    int err;

    tmp = clk_get(NULL, CLKNAME_BISP_CLK);
    if (IS_ERR(tmp)) {
        err = PTR_ERR(tmp);
        DBG_ERR("get isp clock error%d", err);
        goto isp_clk_err;
    }
    cdev->isp_clk = tmp;

    tmp = clk_get(NULL, CLKNAME_CSI_CLK);
    if (IS_ERR(tmp)) {
        err = PTR_ERR(tmp);
        DBG_ERR("get csi clock error%d", err);
        goto csi_err;
    }
    cdev->csi_clk = tmp;

    tmp = clk_get(NULL, CLKNAME_SENSOR_CLKOUT0);
    if (IS_ERR(tmp)) {
        err = PTR_ERR(tmp);
        DBG_ERR("get isp-channel-0 clock error%d", err);
        goto ch0_err;
    }
    cdev->ch_clk[ISP_CHANNEL_0] = tmp;

    tmp = clk_get(NULL, CLKNAME_SENSOR_CLKOUT1);
    if (IS_ERR(tmp)) {
        err = PTR_ERR(tmp);
        DBG_ERR("get isp-channel-1 clock error%d", err);
        goto ch1_err;
    }
    cdev->ch_clk[ISP_CHANNEL_1] = tmp;
    spinfo->ch_clk[ISP_CHANNEL_0] = cdev->ch_clk[ISP_CHANNEL_0];
    spinfo->ch_clk[ISP_CHANNEL_1] = cdev->ch_clk[ISP_CHANNEL_1];

    return 0;

  ch1_err:
    clk_put(cdev->ch_clk[ISP_CHANNEL_0]);
  ch0_err:
    clk_put(cdev->csi_clk);
  csi_err:
    clk_put(cdev->isp_clk);
  isp_clk_err:
    return err;
}


static void camera_clock_exit(struct owl_camera_dev *cdev)
{
    clk_put(cdev->isp_clk);
    clk_put(cdev->csi_clk);
    clk_put(cdev->ch_clk[ISP_CHANNEL_0]);
    clk_put(cdev->ch_clk[ISP_CHANNEL_1]);
}

static int gpio_init(struct device_node *fdt_node,
                     const char *gpio_name, struct dts_gpio *gpio, bool active)
{
    enum of_gpio_flags flags;

    if (!of_find_property(fdt_node, gpio_name, NULL)) {
        DBG_ERR("<isp>no config gpios");
        goto fail;
    }
    gpio->num = of_get_named_gpio_flags(fdt_node, gpio_name, 0, &flags);
    gpio->active_level = !(flags & OF_GPIO_ACTIVE_LOW);

    DBG_INFO("%s: num-%d, active-%s", gpio_name, gpio->num, gpio->active_level ? "high" : "low");

    if (gpio_request(gpio->num, gpio_name)) {
        DBG_ERR("<isp>fail to request gpio [%d]", gpio->num);
        gpio->num = -1;
        goto fail;
    }
    if (active) {
        gpio_direction_output(gpio->num, gpio->active_level);
    } else {
        gpio_direction_output(gpio->num, !gpio->active_level);
    }

    DBG_INFO("gpio value: 0x%x", gpio_get_value(gpio->num));

    return 0;
  fail:
    return -1;
}

static void gpio_exit(struct dts_gpio *gpio, bool active)
{
    if (gpio->num >= 0) {
        if (active) {
            gpio_set_value(gpio->num, gpio->active_level);
        } else {
            gpio_set_value(gpio->num, !gpio->active_level);
        }

        gpio_free(gpio->num);
    }
}

static int regulator_init(struct device_node *fdt_node,
                          const char *regul_name, const char *scope_name,
                          struct dts_regulator *dts_regul)
{
    unsigned int scope[2];
    const char *regul = NULL;

    if (of_property_read_string(fdt_node, regul_name, &regul)) {
        DBG_ERR("<isp> don't config %s", regul_name);
        goto fail;
    }
    DBG_INFO("%s", regul ? regul : "NULL");

    if (of_property_read_u32_array(fdt_node, scope_name, scope, 2)) {
        printk("<isp> fail to get %s", scope_name);
        goto fail;
    }
    DBG_INFO("<isp>: min-%d, max-%d", scope[0], scope[1]);
    dts_regul->min = scope[0];
    dts_regul->max = scope[1];

    dts_regul->regul = regulator_get(NULL, regul);
    if (IS_ERR(dts_regul->regul)) {
        dts_regul->regul = NULL;
        DBG_ERR("<isp> get regulator failed");
        goto fail;
    }

    regulator_set_voltage(dts_regul->regul, dts_regul->min, dts_regul->max);
    //regulator_enable(dts_regul->regul);
    //mdelay(5);
    return 0;

  fail:
    return -1;

}

static inline void regulator_exit(struct dts_regulator *dr)
{
    //regulator_disable(dr->regul);
    regulator_put(dr->regul);
    dr->regul = NULL;
}

static int isp_regulator_init(struct device_node *fdt_node, struct isp_regulators *ir)
{
	const char *avdd_src = NULL;

/*DVDD*/
	struct dts_gpio *dvdd_gpio = &ir->dvdd_gpio;
	if (!gpio_init(fdt_node, "dvdd-gpios", dvdd_gpio, 0))/* poweroff */
		ir->dvdd_use_gpio = 1;
	else
		ir->dvdd_use_gpio = 0;

	if (regulator_init(fdt_node, "dvdd-regulator",
				"dvdd-regulator-scope", &ir->dvdd))
		goto fail;

/*AVDD*/
	if (of_property_read_string(fdt_node, "avdd-src", &avdd_src)) {
		DBG_ERR("<isp> get avdd-src faild");
		goto fail;
	}

	if (!strcmp(avdd_src, "regulator")) {
		DBG_INFO("avdd using regulator: ");
		ir->avdd_use_gpio = 0;

		if (regulator_init(fdt_node, "avdd-regulator",
					"avdd-regulator-scope", &ir->avdd.regul))
			goto free_dvdd;
	} else if (!strcmp(avdd_src, "gpio")) {
		struct dts_gpio *gpio = &ir->avdd.gpio;
		ir->avdd_use_gpio = 1;


	gpio_init(fdt_node, "avdd-gpios", gpio, 0);
	
		DBG_INFO("set - avdd gpio value: 0x%x", gpio_get_value(gpio->num));
	} else
		DBG_INFO("needn't operate avdd manually");

	return 0;

free_dvdd:
	regulator_exit(&ir->dvdd);
fail:
	return -1;
}

static void isp_regulator_exit(struct isp_regulators *ir)
{
    if (ir->dvdd_use_gpio)
        gpio_exit(&ir->dvdd_gpio, 0);

    if (ir->dvdd.regul) {
        regulator_exit(&ir->dvdd);
    }

    if (ir->avdd_use_gpio) {
        gpio_exit(&ir->avdd.gpio, 0);
    } else {
        struct dts_regulator *dr = &ir->avdd.regul;

        if (dr->regul) {
            regulator_exit(dr);
        }
    }
}

static void isp_regulator_enable(struct isp_regulators *ir)
{
	int ret = 0;
	if (ir->dvdd.regul) {
		ret = regulator_enable(ir->dvdd.regul);
		mdelay(5);
	}

	if (ir->avdd_use_gpio) {
		struct dts_gpio *dg = &ir->avdd.gpio;
		gpio_direction_output(dg->num, dg->active_level);
	} else {
		struct dts_regulator *dr = &ir->avdd.regul;
		if (dr->regul) {
			ret = regulator_enable(dr->regul);
			mdelay(5);
		}
	}

	if (ir->dvdd_use_gpio) {
		struct dts_gpio *dvdd_dg = &ir->dvdd_gpio;
		gpio_direction_output(dvdd_dg->num, dvdd_dg->active_level);
	}
}

static void isp_regulator_disable(struct isp_regulators *ir)
{
    if (ir->dvdd_use_gpio) {
        struct dts_gpio *dvdd_dg = &ir->dvdd_gpio;
        gpio_direction_output(dvdd_dg->num, !dvdd_dg->active_level);
    }
	
    if (ir->dvdd.regul) {
        regulator_disable(ir->dvdd.regul);
    }

    if (ir->avdd_use_gpio) {
        struct dts_gpio *dg = &ir->avdd.gpio;
        gpio_direction_output(dg->num, !dg->active_level);
    } else {
        struct dts_regulator *dr = &ir->avdd.regul;
        if (dr->regul) {
            regulator_disable(dr->regul);
        }
    }
}

static int isp_gpio_init(struct device_node *fdt_node, struct sensor_pwd_info *spinfo)
{
    const char *sensors = NULL;

	
    if (gpio_init(fdt_node, "reset-gpios", &spinfo->gpio_reset, 0)) {
        goto fail;
    }

    if (of_property_read_string(fdt_node, "sensors", &sensors)) {
        DBG_ERR("<isp> get sensors faild");
        goto free_reset;
    }

    if (!strcmp(sensors, "front")) {
        // default is power-down
        if (gpio_init(fdt_node, "pwdn-front-gpios", &spinfo->gpio_front, 1)) {
            goto free_reset;
        }
        spinfo->flag = SENSOR_FRONT;
    } else if (!strcmp(sensors, "rear")) {
        if (gpio_init(fdt_node, "pwdn-rear-gpios", &spinfo->gpio_rear, 1)) {
            goto free_reset;
        }
        spinfo->flag = SENSOR_REAR;
    } else if (!strcmp(sensors, "dual")) {
        if (gpio_init(fdt_node, "pwdn-front-gpios", &spinfo->gpio_front, 1)) {
            goto free_reset;
        }
        if (gpio_init(fdt_node, "pwdn-rear-gpios", &spinfo->gpio_rear, 1)) {
            gpio_exit(&spinfo->gpio_front, 1);
            goto free_reset;
        }
        spinfo->flag = SENSOR_DUAL;
    } else {
        DBG_ERR("sensors of dts is wrong");
        goto free_reset;
    }
    return 0;

  free_reset:
    gpio_exit(&spinfo->gpio_reset, 0);
  fail:
    return -1;
}

static void isp_gpio_exit(struct sensor_pwd_info *spinfo)
{
    // only free valid gpio, so no need to check its existence.
    gpio_exit(&spinfo->gpio_front, 1);
    gpio_exit(&spinfo->gpio_rear, 1);
    gpio_exit(&spinfo->gpio_reset, 0);
}


static struct owl_camera_dev *cam_dev_alloc(struct device *dev, struct device_node *dn)
{
    struct owl_camera_dev *cdev;


    cdev = devm_kzalloc(dev,sizeof(struct owl_camera_dev), GFP_ATOMIC);
    if (NULL == cdev) {
        DBG_ERR("alloc owl camera device failed");
        goto ealloc;
    }

    cdev->alloc_ctx = vb2_dma_contig_init_ctx(dev);
    if (IS_ERR(cdev->alloc_ctx)) {
        goto ealloc;
    }
    cdev->started = DEV_CLOSE;

    INIT_LIST_HEAD(&cdev->capture);
    spin_lock_init(&cdev->lock);
    init_completion(&cdev->wait_stop);
    cdev->dvp_mbus_flags = DEFAULT_MBUS_PARAM_DVP;
    cdev->cur_frm = NULL;
    cdev->prev_frm = NULL;
    cdev->mfp = NULL;
    cdev->isp_clk = NULL;
    cdev->csi_clk = NULL;
    cdev->ch_clk[ISP_CHANNEL_0] = NULL;
    cdev->ch_clk[ISP_CHANNEL_1] = NULL;

    sensor_pwd_info_init(&cdev->spinfo);
    isp_regulators_init(&cdev->ir);
    if (camera_clock_init(cdev)) {
        DBG_ERR("camera clocks init error");
        goto clk_err;
    }

    if (isp_gpio_init(dn, &cdev->spinfo)) {
        DBG_ERR("gpio init error");
        goto egpio;
    }

    /* set digital core and analog voltage for sensor */
    if (isp_regulator_init(dn, &cdev->ir)) {
        DBG_ERR("regulator init error");
        goto eregul;
    }


    return cdev;

  eregul:
    isp_gpio_exit(&cdev->spinfo);
  egpio:
    camera_clock_exit(cdev);
  clk_err:
    vb2_dma_contig_cleanup_ctx(cdev->alloc_ctx);
  ealloc:
    return NULL;
}

static void cam_dev_free(struct owl_camera_dev *cdev)
{
    isp_regulator_exit(&cdev->ir);
    isp_gpio_exit(&cdev->spinfo);
    camera_clock_exit(cdev);
    vb2_dma_contig_cleanup_ctx(cdev->alloc_ctx);

}

static int owl_camera_host_probe(struct platform_device *pdev)
{
	
    struct device_node *dn = pdev->dev.of_node;
    struct owl_camera_dev *cam_dev;
    struct soc_camera_host *soc_host;
    unsigned int irq;
    int err = 0;
    const char *owl_camera_status;

    err = of_property_read_string(dn, "status", &owl_camera_status);
    if (err == 0 && strcmp(owl_camera_status, "okay") != 0) {
        printk(KERN_DEBUG"%s owl_camera_status disabled by DTS in %s\n",__func__,dn->full_name);
        return -ENODEV;
    }
	
	err = owl_camera_hw_call(hw_adapter, hw_adapter_init,pdev);
	
    pdev->id = of_alias_get_id(dn, "isp");
    if (pdev->id < 0) {
        dev_err(&pdev->dev, "failed to get alias id, errno %d", pdev->id);
        goto eid;
    }

    DBG_INFO("pdev name: %s", pdev->name);

    cam_dev = cam_dev_alloc(&pdev->dev, dn);
    if (NULL == cam_dev) {
        dev_err(&pdev->dev, "owl_camera_dev alloc failed");
        goto eid;
    }

    attach_sensor_pwd_info(&pdev->dev, &cam_dev->spinfo, pdev->id);

    irq = platform_get_irq(pdev, 0);
    if (!irq) {
        dev_err(&pdev->dev, "no isp irq resource?");
        err = -ENODEV;
        goto egetirq;
    }

    cam_dev->irq = irq;
    DBG_INFO("irq num : %d", cam_dev->irq);

    err = devm_request_irq(&pdev->dev,cam_dev->irq, owl_camera_host_isp_isr,
                      IRQF_DISABLED, OWL_CAM_HOST_NAME, cam_dev);
    if (err) {
        dev_err(&pdev->dev, "Unable to register isp %d interrupt.", cam_dev->irq);
        err = -EBUSY;
        goto egetirq;
    }
	cam_dev->hw_adapter = hw_adapter;
    pm_suspend_ignore_children(&pdev->dev, true);
    pm_runtime_enable(&pdev->dev);
    pm_runtime_resume(&pdev->dev);
    soc_host = &cam_dev->soc_host;
    soc_host->ops = &owl_soc_camera_host_ops;
    soc_host->priv = cam_dev;
    soc_host->v4l2_dev.dev = &pdev->dev;
    soc_host->nr = pdev->id;
    switch (soc_host->nr) {
    case 0:
        soc_host->drv_name = CAM_HOST_NAME;
        break;
    case 1:
        soc_host->drv_name = CAM_HOST2_NAME;
        break;
    default:
        DBG_ERR("host num error");
    }

    err = soc_camera_host_register(soc_host);
    if (err) {
        dev_err(&pdev->dev, "Unable to register owl soc camera host.");
        goto echreg;
    }
	isp_regulator_enable(&cam_dev->ir);
    DBG_INFO("isp driver probe success...");
    return 0;

  echreg:
    pm_runtime_disable(&pdev->dev);

  egetirq:
    detach_sensor_pwd_info(&pdev->dev, &cam_dev->spinfo, pdev->id);
    cam_dev_free(cam_dev);

  eid:
    DBG_ERR("owl camera driver probe fail...");

    return err;
}

static int owl_camera_remove(struct platform_device *pdev)
{
    struct soc_camera_host *soc_host = to_soc_camera_host(&pdev->dev);
    struct owl_camera_dev *cam_dev = soc_host->priv;
		struct resource *res = NULL;
	disable_irq(cam_dev->irq);
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res)
		release_mem_region(res->start, resource_size(res));
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res)
		release_mem_region(res->start, resource_size(res));
    soc_camera_host_unregister(soc_host);
    pm_runtime_disable(&pdev->dev);
    detach_sensor_pwd_info(&pdev->dev, &cam_dev->spinfo, pdev->id);
    cam_dev_free(cam_dev);
	if(noc_si_to_ddr)
		iounmap(noc_si_to_ddr);
	if(gpio_dinen)
		iounmap(gpio_dinen);
	if(si_reset)
		iounmap(si_reset);
	
    DBG_INFO("owl camera is shutdown...");

    return 0;
}

static void owl_camera_shutdown(struct platform_device *pdev)
{
	
	struct soc_camera_host *soc_host = to_soc_camera_host(&pdev->dev);
    struct owl_camera_dev *cam_dev = soc_host->priv;
	isp_regulator_exit(&cam_dev->ir);
	
	DBG_ERR("owl camera is shutdown...");
}



static const struct dev_pm_ops owl_camera_dev_pm_ops = {
    .suspend = owl_camera_suspend,
    .resume = owl_camera_resume,
};

static const struct of_device_id owl_camera_of_match[] = {
    {.compatible = ISP_FDT_COMPATIBLE,},
    {},
};
MODULE_DEVICE_TABLE(of, owl_camera_of_match);

static struct platform_driver owl_camera_host_driver = {
    .driver = {
        .name = OWL_CAM_HOST_NAME,
        .owner = THIS_MODULE,
        .pm = &owl_camera_dev_pm_ops,
        .of_match_table = owl_camera_of_match,
    },
    .probe = owl_camera_host_probe,
    .remove = owl_camera_remove,
    .shutdown = owl_camera_shutdown,
};

struct owl_camera_hw_adapter *get_owl_camera_hw_adapter(void)
{
	if(of_find_compatible_node(NULL, NULL, "actions,atm7059tc") || of_find_compatible_node(NULL, NULL, "actions,atm7059a"))
	{
			printk("actions,atm7059\n");
    	return &atm7059_hw_adapter;
	}
	else if(of_find_compatible_node(NULL, NULL, "actions,atm7039c"))
	{
//		 return &atm7039_hw_adapter;
	}

	return NULL;
}

// platform device register by dts
static int __init owl_camera_init(void)
{
    int ret;

    DBG_INFO("code(2013.09.15): compile time - %s %s", __DATE__, __TIME__);
	hw_adapter = get_owl_camera_hw_adapter();
    ret = platform_driver_register(&owl_camera_host_driver);
    if (ret) {
        DBG_ERR(CAM_DRV_NAME":Could not register isp driver");
    }
    return ret;
}

static void __exit owl_camera_exit(void)
{
    platform_driver_unregister(&owl_camera_host_driver);
}

late_initcall(owl_camera_init);
module_exit(owl_camera_exit);

MODULE_DESCRIPTION("OWL SoC Camera Host driver");
MODULE_AUTHOR("lichi <lichi@actions-semi.com>");
MODULE_LICENSE("GPL v2");
