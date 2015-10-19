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
#include <linux/io.h>




#include "owl_camera.h"


#define HW_NAME "owl_camera"
#define MAX_WIDTH	4288
#define MAX_HEIGHT	3000
#define WORK_CLOCK	60000000     
#define CROP_X_ALIGN	2
#define CROP_Y_ALIGN	1
#define CROP_W_ALIGN	32
#define CROP_H_ALIGN	4
#define ISP_PRELINE_NUM 16U


static volatile void*	atm_7039_cmu_sensor_clk = 0;	
static volatile void*	ATM_7039_SI_BASE = 0;

#define     SI_ENABLE             (ATM_7039_SI_BASE+0x00)
#define     SI_INT_STAT           (ATM_7039_SI_BASE+0x04)

#define     SI_CH0_CTRL           (ATM_7039_SI_BASE+0x08)
#define     SI_CH0_ROW_RANGE      (ATM_7039_SI_BASE+0x0c)
#define     SI_CH0_COL_RANGE      (ATM_7039_SI_BASE+0x10)
#define     SI_CH0_ADDRY          (ATM_7039_SI_BASE+0x14)
#define     SI_CH0_ADDRU          (ATM_7039_SI_BASE+0x18)
#define     SI_CH0_ADDRV          (ATM_7039_SI_BASE+0x1c)

#define     SI_CH1_CTRL           (ATM_7039_SI_BASE+0x20)
#define     SI_CH1_ROW_RANGE      (ATM_7039_SI_BASE+0x24)
#define     SI_CH1_COL_RANGE      (ATM_7039_SI_BASE+0x28)
#define     SI_CH1_ADDRY          (ATM_7039_SI_BASE+0x2c)
#define     SI_CH1_ADDRU          (ATM_7039_SI_BASE+0x30)
#define     SI_CH1_ADDRV          (ATM_7039_SI_BASE+0x34)

#define CMU_SENSORCLK_INVT0 (0x1 << 12)
#define CMU_SENSORCLK_INVT1 (0x1 << 13)

// SI_ENABLE
#define CH1_ENABLE              (0x1<<31)
#define CH1_PRELINE_NUM_MASK    (0xFFF << 16)
#define CH1_PRELINE_NUM(x)      ((0xFFF & (x)) << 16)
#define CH0_ENABLE              (0x1<<15)
#define CH0_PRELINE_NUM_MASK    (0xFFF)
#define CH0_PRELINE_NUM(x)      (0xFFF & (x))

#define ROW_START(x)    (0xFFF & (x))
#define ROW_END(x)      ((0xFFF & (x)) << 16)

#define COL_START(x)    (0x1FFF & (x))
#define COL_END(x)      ((0x1FFF & (x)) << 16)

//SI_CHx_CTRL
#define SYNC_POL_HSYNC          (0x1<<13)
#define SYNC_POL_VSYNC          (0x1<<12)
#define SEMI_UV_INV             (0x1<<10)
#define YUV_OUTPUT_FORMAT_MASK  (0x3<<8)    //9:8
#define YUV_OUTPUT_FORMAT(v)    ((0x3&(v))<<8)
#define YUV_INPUT_FORMAT_MASK  (0x3<<4)    //5:4
#define YUV_INPUT_FORMAT(v)    ((0x3&(v))<<4)
#define SRC_INTF                (0x1<<3)
#define IN_FMT_MASK             (0)         //2:0
#define IN_FMT(v)               (v)

// yuv output format
#define YUV_OUTPUT_FMT_YUV422       YUV_OUTPUT_FORMAT(0)
#define YUV_OUTPUT_FMT_YUV420       YUV_OUTPUT_FORMAT(1)
#define YUV_OUTPUT_FMT_YUV422_SEMI  YUV_OUTPUT_FORMAT(2)
#define YUV_OUTPUT_FMT_YUV420_SEMI  YUV_OUTPUT_FORMAT(3)

// yuv input format(is input format is yuv)
#define YUV_INPUT_FMT_UYVY  YUV_INPUT_FORMAT(0)
#define YUV_INPUT_FMT_VYUY  YUV_INPUT_FORMAT(1)
#define YUV_INPUT_FMT_YUYV  YUV_INPUT_FORMAT(2)
#define YUV_INPUT_FMT_YVYU  YUV_INPUT_FORMAT(3)

// input format
#define INT_FMT_RAW8        IN_FMT(0)
#define INT_FMT_RAW10       IN_FMT(1)
#define INT_FMT_RAW12       IN_FMT(2)
#define INT_FMT_YUV         IN_FMT(3)
#define INT_FMT_RGB565      IN_FMT(4)
#define INT_FMT_RGB888      IN_FMT(5)


//src intf
#define SRC_INTF_SENSOR     (0x0<<3)   // dvp
#define SRC_INTF_CSI        (0x1<<3)   // mipi


// SI_INT_STAT
#define CH1_IN_OVERFLOW_PEND    (0x1<<15)
#define CH1_OUT_OVERFLOW_PEND   (0x1<<14)
#define CH1_PRELINE_PEND        (0x1<<13)
#define CH1_FRAME_PEND          (0x1<<12)
#define CH0_IN_OVERFLOW_PEND    (0x1<<11)
#define CH0_OUT_OVERFLOW_PEND   (0x1<<10)
#define CH0_PRELINE_PEND        (0x1<<9)
#define CH0_FRAME_PEND          (0x1<<8)

#define CH1_PRELINE_IRQ_EN      (0x1<<3)
#define CH1_FRAME_END_IRQ_EN    (0x1<<2)
#define CH0_PRELINE_IRQ_EN      (0x1<<1)
#define CH0_FRAME_END_IRQ_EN    (0x1<<0)

#define CSI_CTRL_EN  (0x1 << 0)
#define CSI_CTRL_D_PHY_EN    (0x1 << 2)
#define CSI_CTRL_PHY_INIT    (0x1 << 3)
#define CSI_CTRL_LANE_NUM(x) (((x) & 0x3) << 4)
#define CSI_CTRL_ECE (0x1 << 6)
#define CSI_CTRL_CCE (0x1 << 7)
#define CSI_CTRL_CLK_LANE_HS  (0x1 << 8)
#define CSI_CTRL_PHY_INIT_SEL (0x1 << 9)

#define MIPI_PHY_1LANE 0x3
#define MIPI_PHY_2LANE 0x7
#define MIPI_PHY_3LANE 0xf
#define MIPI_PHY_4LANE 0x1f

#define CSI_CONTEXT_EN (0x1 << 0)
#define CSI_CONTEXT_DT(x) (((x) & 0x3f) << 1)





//atm7039 camera si 
#define ISP_ENABLE                                  (ATM_7039_SI_BASE+0x10)
#define ISP_CTL                                     (ATM_7039_SI_BASE+0x20)
#define ISP_CHANNEL_1_ROW_RANGE                     (ATM_7039_SI_BASE+0x2C)
#define ISP_CHANNEL_1_COL_RANGE                     (ATM_7039_SI_BASE+0x30)
#define ISP_CHANNEL_1_STATE                         (ATM_7039_SI_BASE+0x24)

#define ISP_OUT_FMT                                 (ATM_7039_SI_BASE+0x148)
#define ISP_OUT_ADDRY                               (ATM_7039_SI_BASE+0x14C)
#define ISP_OUT_ADDRU                               (ATM_7039_SI_BASE+0x150)
#define ISP_OUT_ADDRV                               (ATM_7039_SI_BASE+0x154)
#define ISP_OUT_ADDR1UV                             (ATM_7039_SI_BASE+0x158)


#define ISP_CHANNEL_2_STATE                         (ATM_7039_SI_BASE+0x1F0)
#define ISP2_CTL                                    (ATM_7039_SI_BASE+0x1EC)
#define ISP_CHANNEL_2_ROW_RANGE                     (ATM_7039_SI_BASE+0x1F8)
#define ISP_CHANNEL_2_COL_RANGE                     (ATM_7039_SI_BASE+0x1FC)


#define ISP2_OUT_FMT                                (ATM_7039_SI_BASE+0x314)
#define ISP2_OUT_ADDRY                              (ATM_7039_SI_BASE+0x318)
#define ISP2_OUT_ADDRU                              (ATM_7039_SI_BASE+0x31C)
#define ISP2_OUT_ADDRV                              (ATM_7039_SI_BASE+0x320)
#define ISP2_OUT_ADDR1UV                            (ATM_7039_SI_BASE+0x324)



#define ISP_INT_STAT                                (ATM_7039_SI_BASE+0x400)

#define CMU_SENSORCLK_INVT0 (0x1 << 12)
#define CMU_SENSORCLK_INVT1 (0x1 << 13)

#define ISP_CH1_PRELINE_NUM_MASK (0xFFF << 8)
#define ISP_CH1_PRELINE_NUM(x) ((0xFFF & (x)) << 8)

#define ISP_CH1_COL_START(x)  (0x1FFF & (x))
#define ISP_CH1_COL_END(x)  ((0x1FFF & (x)) << 16)

#define ISP_CH1_ROW_START(x)  (0xFFF & (x))
#define ISP_CH1_ROW_END(x)  ((0xFFF & (x)) << 16)


#define ISP_CH2_PRELINE_NUM_MASK (0xFFF << 8)
#define ISP_CH2_PRELINE_NUM(x) ((0xFFF & (x)) << 8)

#define ISP_CH2_COL_START(x)  (0x1FFF & (x))
#define ISP_CH2_COL_END(x)  ((0x1FFF & (x)) << 16)

#define ISP_CH2_ROW_START(x)  (0xFFF & (x))
#define ISP_CH2_ROW_END(x)  ((0xFFF & (x)) << 16)

#define ISP_ENABLE_EN (0x1)
#define ISP_ENABLE_MODE_MASK (0x7 << 1)
#define ISP_ENABLE_MODE(x)   ((0x7 & (x)) << 1)
#define ISP_ENABLE_CH1_EN    (0x1 << 4)
#define ISP_ENABLE_CH2_EN    (0x1 << 5)
#define ISP_ENABLE_CH1_MODE  (0x1 << 6)
#define ISP_ENABLE_CH2_MODE  (0x1 << 7)
#define ISP_ENABLE_CH2_SYNC  (0x1 << 8)
#define ISP_ENABLE_MASK      (0x1FF)


#define ISP_INT_STAT_ISP1_PL_INT_EN (0x1 << 2)
#define ISP_INT_STAT_ISP2_PL_INT_EN (0x1 << 3)
#define ISP_INT_STAT_CH1_PL_INT_EN (0x1 << 5)
#define ISP_INT_STAT_CH2_PL_INT_EN (0x1 << 6)
#define ISP_INT_STAT_FRAME_END_INT_EN (0x1 << 7)
#define ISP_INT_STAT_ISP1_PL_PD (0x1 << 10)
#define ISP_INT_STAT_ISP2_PL_PD (0x1 << 11)
#define ISP_INT_STAT_AF_OK_PD  (0x1 << 12)
#define ISP_INT_STAT_CH1_PL_PD (0x1 << 13)
#define ISP_INT_STAT_CH2_PL_PD (0x1 << 14)



// isp_out_fmt reg
#define ISP_OUT_FMT_STRIDE1_MASK  (0x1fff << 16)
#define ISP_OUT_FMT_STRIDE1(x)    ((0x1fff & (x)) << 16)
#define ISP_OUT_FMT_SEMI_UV_INV   (0x1 << 15)
#define ISP_OUT_FMT_STRIDE2_MASK  (0x1ffff << 2)
#define ISP_OUT_FMT_STRIDE2(x)    ((0x1fff & (x)) << 2)
#define ISP_OUT_FMT_MASK    0x3
#define ISP_OUT_FMT_YUV420  0x0
#define ISP_OUT_FMT_YUV422P 0x1
#define ISP_OUT_FMT_NV12    0x2
#define ISP_OUT_FMT_YUYV    0x3

#define ISP_CTL_CHANNEL1_INTF_MIPI (0x1 << 6)
#define ISP_CTL_CHANNEL1_INTF_PARAL (0x0 << 6)
#define ISP_CTL_MODE_MASK (0x3 << 4)
#define ISP_CTL_MODE_RGB8 (0x0 << 4)
#define ISP_CTL_MODE_RGB10 (0x1 << 4)
#define ISP_CTL_MODE_RGB12 (0x2 << 4)
#define ISP_CTL_MODE_YUYV  (0x3 << 4)
#define ISP_CTL_HSYNC_ACTIVE_HIGH (0x1 << 3)
#define ISP_CTL_VSYNC_ACTIVE_HIGH (0x1 << 2)
#define ISP_CTL_COLOR_SEQ_MASK 0x3
#define ISP_CTL_COLOR_SEQ_UYVY 0x0
#define ISP_CTL_COLOR_SEQ_VYUY 0x1
#define ISP_CTL_COLOR_SEQ_YUYV 0x2
#define ISP_CTL_COLOR_SEQ_YVYU 0x3
#define ISP_CTL_COLOR_SEQ_SBGGR 0x0
#define ISP_CTL_COLOR_SEQ_SGRBG 0x1
#define ISP_CTL_COLOR_SEQ_SGBRG 0x2
#define ISP_CTL_COLOR_SEQ_SRGGB 0x3

#define ISP2_CTL_CHANNEL_INTF_MIPI (0x1 << 5)
#define ISP2_CTL_CHANNEL_INTF_PARAL (0x0 << 5)
#define ISP2_CTL_MODE_MASK (0x1 << 4)
#define ISP2_CTL_MODE_RGB (0x0 << 4)
#define ISP2_CTL_MODE_YUYV  (0x1 << 4)
#define ISP2_CTL_HSYNC_ACTIVE_HIGH (0x1 << 3)
#define ISP2_CTL_VSYNC_ACTIVE_HIGH (0x1 << 2)
#define ISP2_CTL_COLOR_SEQ_MASK 0x3
#define ISP2_CTL_COLOR_SEQ_UYVY 0x0
#define ISP2_CTL_COLOR_SEQ_VYUY 0x1
#define ISP2_CTL_COLOR_SEQ_YUYV 0x2
#define ISP2_CTL_COLOR_SEQ_YVYU 0x3
#define ISP2_CTL_COLOR_SEQ_SBGGR 0x0
#define ISP2_CTL_COLOR_SEQ_SGRBG 0x1
#define ISP2_CTL_COLOR_SEQ_SGBRG 0x2
#define ISP2_CTL_COLOR_SEQ_SRGGB 0x3


#define CSI_CTRL_EN  (0x1 << 0)
#define CSI_CTRL_D_PHY_EN    (0x1 << 2)
#define CSI_CTRL_PHY_INIT    (0x1 << 3)
#define CSI_CTRL_LANE_NUM(x) (((x) & 0x3) << 4)
#define CSI_CTRL_ECE (0x1 << 6)
#define CSI_CTRL_CCE (0x1 << 7)
#define CSI_CTRL_CLK_LANE_HS  (0x1 << 8)
#define CSI_CTRL_PHY_INIT_SEL (0x1 << 9)

#define MIPI_PHY_1LANE 0x3
#define MIPI_PHY_2LANE 0x7
#define MIPI_PHY_3LANE 0xf
#define MIPI_PHY_4LANE 0x1f

#define CSI_CONTEXT_EN (0x1 << 0)
#define CSI_CONTEXT_DT(x) (((x) & 0x3f) << 1)

#define CONTEXT_DT_RAW8  (0x2A)
#define CONTEXT_DT_RAW10 (0x2B)
#define CONTEXT_DT_RAW12 (0x2C)










static int atm_7039_hw_adapter_init(struct owl_camera_hw_adapter* hw,struct platform_device *pdev)
{
	struct resource *res;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource\n");
		return -ENODEV;
	}
	//DBG_ERR("res->start is  : 0x%x,resource size is 0x%x",res->start,resource_size(res));
	
	if (!request_mem_region(res->start,
					resource_size(res), "isp")) {
			dev_err(&pdev->dev, "Unable to request register region\n");
			return -EBUSY;
		}	

	ATM_7039_SI_BASE = devm_ioremap(&pdev->dev,res->start,resource_size(res));
	if(!ATM_7039_SI_BASE)
		return -ENXIO;
	printk("SI paddr is %x",(int)ATM_7039_SI_BASE);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource\n");
		return -ENODEV;
	}
	printk("res->start is  : 0x%x,resource size is 0x%x",res->start,resource_size(res));
	
	if (!request_mem_region(res->start,
					resource_size(res), "isp")) {
			dev_err(&pdev->dev, "Unable to request register region\n");
			return -EBUSY;
		}	
	atm_7039_cmu_sensor_clk = devm_ioremap(&pdev->dev,res->start,resource_size(res));
	if(!atm_7039_cmu_sensor_clk)
		return -ENXIO;
	return 0;
	
}







static int atm_7039_get_channel_state(struct owl_camera_hw_adapter* hw, int channel)
{
    
   return readl(ISP_INT_STAT);
}


static int atm_7039_set_channel_if(struct owl_camera_hw_adapter* hw, int channel, int bus_type)
{
	unsigned long ch1_intf,ch2_intf;
    volatile void * reg = 0;
    unsigned long value;

	if (V4L2_MBUS_PARALLEL == bus_type) {
        ch1_intf = ISP_CTL_CHANNEL1_INTF_PARAL;
		ch2_intf = ISP2_CTL_CHANNEL_INTF_PARAL;
    } else {
        ch1_intf = ISP_CTL_CHANNEL1_INTF_MIPI;
		ch2_intf = ISP2_CTL_CHANNEL_INTF_MIPI;
    }
	 // intf
    if (ISP_CHANNEL_0 == channel) {
        reg = ISP_CTL;
		value = readl(ISP_CTL);
	    value &= ~ISP_CTL_CHANNEL1_INTF_MIPI;
		value  |= (ch1_intf);
    } else {
        reg = ISP2_CTL;
		value = readl(ISP2_CTL);
		value  &= ~ISP2_CTL_CHANNEL_INTF_MIPI;
		value  |= (ch2_intf);
    }
    writel(value, reg);
    return 0;
}


static int atm_7039_set_channel_int_en(struct owl_camera_hw_adapter* hw, int channel)
{
	
	//channel enable
	int ret = 0;
	unsigned long isp_enable,isp_int_stat;
   isp_enable = readl(ISP_ENABLE);
   isp_enable &= ~(ISP_ENABLE_MASK);
   if (ISP_CHANNEL_0 == channel) {
   		isp_int_stat = readl(ISP_INT_STAT);
		isp_int_stat |= ISP_INT_STAT_ISP1_PL_INT_EN;
		writel(isp_int_stat, ISP_INT_STAT);
		isp_enable |= (ISP_ENABLE_EN | ISP_ENABLE_MODE(1) | ISP_ENABLE_CH1_EN | ISP_ENABLE_CH1_MODE);
   } else {
   		isp_int_stat = readl(ISP_INT_STAT);
        isp_int_stat |= ISP_INT_STAT_ISP2_PL_INT_EN;
        writel(isp_int_stat, ISP_INT_STAT);
		isp_enable |= (ISP_ENABLE_EN | ISP_ENABLE_MODE(2) | ISP_ENABLE_CH2_EN | ISP_ENABLE_CH2_MODE);
   }
   writel(isp_enable, SI_ENABLE);

   
   return ret;
}


static int atm_7039_set_channel_addrY(struct owl_camera_hw_adapter* hw, int channel, void *addrY)
{
    if(ISP_CHANNEL_0 == channel)
    {
        writel((u32)addrY, ISP_OUT_ADDRY); 
    }
    else
    {
        writel((u32)addrY, ISP2_OUT_ADDRY);
    }
    return 0;
}
static int atm_7039_set_channel_addrU(struct owl_camera_hw_adapter* hw, int channel, void *addrU)
{
    
    if(ISP_CHANNEL_0 == channel)
    {
        writel((u32)addrU, ISP_OUT_ADDRU); 
    }
    else
    {
        writel((u32)addrU, ISP2_OUT_ADDRU);
    }
    return 0;
}
static int atm_7039_set_channel_addrV(struct owl_camera_hw_adapter* hw, int channel, void *addrV)
{
    if(ISP_CHANNEL_0 == channel)
    {
        writel((u32)addrV, ISP_OUT_ADDRV); 
    }
    else
    {
        writel((u32)addrV, ISP2_OUT_ADDRV);
    }
    return 0;
}
static int atm_7039_set_channel_addr1UV(struct owl_camera_hw_adapter* hw, int channel, void *addr1UV)
{
	 if(ISP_CHANNEL_0 == channel)
    {
        writel((u32)addr1UV, ISP_OUT_ADDRU); 
    }
    else
    {
        writel((u32)addr1UV, ISP2_OUT_ADDRU);
    }
    return 0;
}

static int atm_7039_set_channel_input_fmt(struct owl_camera_hw_adapter* hw, int channel, enum v4l2_mbus_pixelcode code)
{
	unsigned int isp_ctl = readl(ISP_CTL);
    unsigned int isp2_ctl = readl(ISP2_CTL);


    isp_ctl &= ~(ISP_CTL_MODE_MASK | ISP_CTL_COLOR_SEQ_MASK);
    isp2_ctl &= ~(ISP2_CTL_MODE_MASK | ISP2_CTL_COLOR_SEQ_MASK);

    switch (code) {
    case V4L2_MBUS_FMT_UYVY8_2X8:{  /* UYVY */
            // should be the same as senor's output format
            isp_ctl |= (ISP_CTL_MODE_YUYV | ISP_CTL_COLOR_SEQ_UYVY);
            isp2_ctl |= (ISP2_CTL_MODE_YUYV | ISP2_CTL_COLOR_SEQ_UYVY);
            isp_info("input format UYVY, 1pix/clk\n");
            break;
        }
    case V4L2_MBUS_FMT_SGRBG8_1X8:{
            isp_info("input format RAW SGRBG8 - ");
            
                isp_ctl |= ISP_CTL_MODE_RGB10 | ISP_CTL_COLOR_SEQ_SBGGR;
                isp_info("10bit\n");
            
            isp2_ctl |= ISP2_CTL_MODE_RGB | ISP2_CTL_COLOR_SEQ_SGRBG;

            break;
        }
    case V4L2_MBUS_FMT_SRGGB8_1X8:
    	{
    		isp_info("input format RAW SRGGB - ");
            
                isp_ctl |= ISP_CTL_MODE_RGB10 | ISP_CTL_COLOR_SEQ_SGBRG;//ISP_CTL_COLOR_SEQ_SRGGB;
                isp_info("10bit\n");
            
            isp2_ctl |= ISP2_CTL_MODE_RGB | ISP2_CTL_COLOR_SEQ_SGRBG;

            break;
    	}
    	case V4L2_MBUS_FMT_SGBRG8_1X8:
    	{
    		isp_info("input format RAW GBRG - ");
           
                isp_ctl |= ISP_CTL_MODE_RGB10 | ISP_CTL_COLOR_SEQ_SGBRG;
                isp_info("10bit\n");
           
            isp2_ctl |= ISP2_CTL_MODE_RGB | ISP2_CTL_COLOR_SEQ_SGRBG;

            break;
    	}
    	case V4L2_MBUS_FMT_SBGGR8_1X8:
    	{
    		isp_info("input format RAW BGGR - ");
           
                isp_ctl |= ISP_CTL_MODE_RGB10 | ISP_CTL_COLOR_SEQ_SBGGR;
                isp_info("10bit\n");
          
            isp2_ctl |= ISP2_CTL_MODE_RGB | ISP2_CTL_COLOR_SEQ_SGRBG;

            break;
    	}
    default:
        isp_err("input data error (pixel code:0x%x)\n", code);
        break;

    }

    if (ISP_CHANNEL_0 == channel) {
        writel(isp_ctl, ISP_CTL);
    } else {
        writel(isp2_ctl, ISP2_CTL);
    }
	return 0;
}

static int atm_7039_set_channel_output_fmt(struct owl_camera_hw_adapter* hw, int channel, u32 fourcc)
{
	unsigned int isp_out_fmt = readl(ISP_OUT_FMT);
    unsigned int isp2_out_fmt = readl(ISP2_OUT_FMT);

    isp_out_fmt &= ~ISP_OUT_FMT_MASK;
    isp2_out_fmt &= ~ISP_OUT_FMT_MASK;

    switch (fourcc) {
    case V4L2_PIX_FMT_YUV420: {  //420 planar
            isp_out_fmt |= ISP_OUT_FMT_YUV420;
            isp2_out_fmt |= ISP_OUT_FMT_YUV420;
            printk("output format YUV420\n");
            break;
        }
    case V4L2_PIX_FMT_YUV422P: { //422 semi planar
            isp_out_fmt |= ISP_OUT_FMT_YUV422P;
            isp2_out_fmt |= ISP_OUT_FMT_YUV422P;
            printk("output format YUV422P\n");
            break;
        }
    case V4L2_PIX_FMT_NV12: {    //420 semi-planar
            isp_out_fmt |= ISP_OUT_FMT_NV12;
            isp2_out_fmt |= ISP_OUT_FMT_NV12;
            printk("output format NV12\n");
            break;
        }
    case V4L2_PIX_FMT_YUYV: {   //interleaved
            isp_out_fmt |= ISP_OUT_FMT_YUYV;
            isp2_out_fmt |= ISP_OUT_FMT_YUYV;
            printk("output format YUYV\n");
            break;
        }
    default:   /* Raw RGB */
        printk("set isp output format failed, fourcc = 0x%x\n", fourcc);
        return -EINVAL;
    }

    if (ISP_CHANNEL_0 == channel) {
        writel(isp_out_fmt, ISP_OUT_FMT);
    } else {
        writel(isp2_out_fmt, ISP2_OUT_FMT);
    }
    return 0;

}


static int atm_7039_set_channel_preline(struct owl_camera_hw_adapter* hw, int channel, int preline)
{
	 unsigned int state;


    if (ISP_CHANNEL_0 == channel) {
        state = readl(ISP_CHANNEL_1_STATE);
        state &= ~ISP_CH1_PRELINE_NUM_MASK;
        state |= ISP_CH1_PRELINE_NUM(channel + ISP_PRELINE_NUM);
        writel(state, ISP_CHANNEL_1_STATE);
    } else {
        state =readl(ISP_CHANNEL_2_STATE);
        state &= ~ISP_CH2_PRELINE_NUM_MASK;
        state |= ISP_CH2_PRELINE_NUM(channel + ISP_PRELINE_NUM);
        writel(state, ISP_CHANNEL_2_STATE);
    }
    return 0;
}

static int atm_7039_clear_channel_int_en(struct owl_camera_hw_adapter* hw, int channel)
{
	unsigned int isp_enable;
	 if (ISP_CHANNEL_0 == channel) {
        writel(isp_enable & (~(ISP_ENABLE_EN | ISP_ENABLE_CH1_EN)), ISP_ENABLE);
    } else {
        writel(isp_enable & (~(ISP_ENABLE_EN | ISP_ENABLE_CH2_EN)), ISP_ENABLE);
    }
    return 0;
}

static int atm_7039_set_channel_preline_int_en(struct owl_camera_hw_adapter* hw, int channel)
{
    return 0;
}

static int atm_7039_set_channel_frameend_int_en(struct owl_camera_hw_adapter* hw, int channel)
{

    u32 int_stat;
    int_stat = readl(SI_INT_STAT);
    int_stat &= ~(CH0_PRELINE_IRQ_EN | CH1_PRELINE_IRQ_EN);
	if(ISP_CHANNEL_0 == channel)
	{
		writel(int_stat | CH0_FRAME_END_IRQ_EN, SI_INT_STAT);
	}
	else 
	{
		writel(int_stat | CH1_FRAME_END_IRQ_EN, SI_INT_STAT);
	}
    return 0;
    
}

static int atm_7039_clear_channel_preline_pending(struct owl_camera_hw_adapter* hw, unsigned int isp_int_stat)
{
	isp_int_stat &= ~CH0_FRAME_END_IRQ_EN;
    isp_int_stat |= (0x7<<8);
    writel(isp_int_stat, SI_INT_STAT);
    return 0;
}

static int atm_7039_clear_channel_frameend_pending(struct owl_camera_hw_adapter* hw,  unsigned int isp_int_stat)
{
	
    return 0;
}


static int atm_7039_set_signal_polarity(struct owl_camera_hw_adapter* hw, int channel,unsigned int common_flags)
{
	u32 cmu_sensorclk;
	volatile void * reg = 0;
    unsigned long value;

	cmu_sensorclk = readl(atm_7039_cmu_sensor_clk);

    if (ISP_CHANNEL_0 == channel) {
       // isp_ctl = readl(ISP_CTL);
		reg = ISP_CTL;
        if (common_flags & V4L2_MBUS_PCLK_SAMPLE_FALLING) {
            cmu_sensorclk |= CMU_SENSORCLK_INVT0;
        } else {
            cmu_sensorclk &= ~CMU_SENSORCLK_INVT0;
        }
        /*if (common_flags & V4L2_MBUS_HSYNC_ACTIVE_LOW) {
            isp_ctl &= ~ISP_CTL_HSYNC_ACTIVE_HIGH;
        } else {
            isp_ctl |= ISP_CTL_HSYNC_ACTIVE_HIGH;
        }
        if (common_flags & V4L2_MBUS_VSYNC_ACTIVE_LOW) {
            isp_ctl &= ~ISP_CTL_VSYNC_ACTIVE_HIGH;
        } else {
            isp_ctl |= ISP_CTL_VSYNC_ACTIVE_HIGH;
        }
        writel(isp_ctl, ISP_CTL);*/
    } else {
        //isp2_ctl = readl(ISP2_CTL);
		
		reg = ISP2_CTL;
        if (common_flags & V4L2_MBUS_PCLK_SAMPLE_FALLING) {
            cmu_sensorclk |= CMU_SENSORCLK_INVT1;
        } else {
            cmu_sensorclk &= ~CMU_SENSORCLK_INVT1;
        }

		/*if (common_flags & V4L2_MBUS_HSYNC_ACTIVE_LOW) {
            isp2_ctl &= ~ISP2_CTL_HSYNC_ACTIVE_HIGH;
        } else {
            isp2_ctl |= ISP2_CTL_HSYNC_ACTIVE_HIGH;
        }
        if (common_flags & V4L2_MBUS_VSYNC_ACTIVE_LOW) {
            isp2_ctl &= ~ISP2_CTL_VSYNC_ACTIVE_HIGH;
        } else {
            isp2_ctl |= ISP2_CTL_VSYNC_ACTIVE_HIGH;
        }
        writel(isp2_ctl, ISP2_CTL);*/
    }
	value = readl(reg);
	 if (common_flags & V4L2_MBUS_HSYNC_ACTIVE_LOW) {
        value &= ~SYNC_POL_HSYNC;
    } else {
        value |= SYNC_POL_HSYNC;
    }
    if (common_flags & V4L2_MBUS_VSYNC_ACTIVE_LOW) {
        value &= ~SYNC_POL_VSYNC;
    } else {
        value |= SYNC_POL_VSYNC;
    }
	printk("before writel\n");
    writel(value, reg);
    writel(cmu_sensorclk, atm_7039_cmu_sensor_clk);
	return 0;
}



static int atm_7039_set_col_range(struct owl_camera_hw_adapter* hw,int channel,unsigned int start,unsigned int end)
{
	int ret = 0;
	  if (ISP_CHANNEL_0 == channel) {
        writel(ISP_CH1_COL_START(start) | ISP_CH1_COL_END(end), ISP_CHANNEL_1_COL_RANGE);
    } else {
        writel(ISP_CH2_COL_START(start) | ISP_CH2_COL_END(end), ISP_CHANNEL_2_COL_RANGE);
    }
	return ret;
}

static int atm_7039_set_row_range(struct owl_camera_hw_adapter* hw,int channel,unsigned int start,unsigned int end)
{
	int ret = 0;
	 if (ISP_CHANNEL_0 == channel) {
        writel(ISP_CH1_ROW_START(start) | ISP_CH1_ROW_END(end), ISP_CHANNEL_1_ROW_RANGE);
    } else {
        writel(ISP_CH2_ROW_START(start) | ISP_CH2_ROW_END(end), ISP_CHANNEL_2_ROW_RANGE);
    }
	return ret;
}





static struct owl_camera_hw_ops atm7039_hw_ops = {
	.hw_adapter_init = atm_7039_hw_adapter_init,
    .get_channel_state = atm_7039_get_channel_state,
    .set_channel_if = atm_7039_set_channel_if,
    .set_channel_addrY = atm_7039_set_channel_addrY,
    .set_channel_addrU = atm_7039_set_channel_addrU,
    .set_channel_addrV = atm_7039_set_channel_addrV,
    .set_channel_addr1UV = atm_7039_set_channel_addr1UV,
    .set_channel_input_fmt = atm_7039_set_channel_input_fmt,
    .set_channel_output_fmt = atm_7039_set_channel_output_fmt,
    .set_channel_preline = atm_7039_set_channel_preline,
    .clear_channel_int_en = atm_7039_clear_channel_int_en,
    .set_channel_preline_int_en = atm_7039_set_channel_preline_int_en,
    .set_channel_frameend_int_en = atm_7039_set_channel_frameend_int_en,
    .clear_channel_preline_pending = atm_7039_clear_channel_preline_pending,
    .clear_channel_frameend_pending = atm_7039_clear_channel_frameend_pending,
    .set_signal_polarity = atm_7039_set_signal_polarity,
    .set_col_range = atm_7039_set_col_range,
    .set_row_range = atm_7039_set_row_range,
    .set_channel_int_en = atm_7039_set_channel_int_en,
};
struct owl_camera_hw_adapter atm7039_hw_adapter = {
    .hw_name = HW_NAME,

    .cam_dev = NULL,

    .max_channel = 1,
    .has_isp = 0,
    .is_3D_support = 0,
    .crop_x_align = CROP_X_ALIGN,
    .crop_y_align = CROP_Y_ALIGN,
    .crop_w_align = CROP_W_ALIGN,
    .crop_h_align = CROP_H_ALIGN,
    .max_width = MAX_WIDTH,
    .max_height = MAX_HEIGHT,
	.preline_int_pd = ISP_INT_STAT_ISP1_PL_PD,


    .power_ref = 0,
    .enable_ref = 0,
    .hw_clk = NULL,

    .ops = &atm7039_hw_ops,
};
