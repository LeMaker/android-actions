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

static volatile void*	atm_7059_cmu_sensor_clk = 0;	
static volatile void*	ATM_7059_SI_BASE = 0;

#define     ATM_7059_SI_ENABLE             (ATM_7059_SI_BASE+0x00)
#define     ATM_7059_SI_INT_STAT           (ATM_7059_SI_BASE+0x04)

#define     ATM_7059_SI_CH0_CTRL           (ATM_7059_SI_BASE+0x08)
#define     ATM_7059_SI_CH0_ROW_RANGE      (ATM_7059_SI_BASE+0x0c)
#define     ATM_7059_SI_CH0_COL_RANGE      (ATM_7059_SI_BASE+0x10)
#define     ATM_7059_SI_CH0_ADDRY          (ATM_7059_SI_BASE+0x14)
#define     ATM_7059_SI_CH0_ADDRU          (ATM_7059_SI_BASE+0x18)
#define     ATM_7059_SI_CH0_ADDRV          (ATM_7059_SI_BASE+0x1c)

#define     ATM_7059_SI_CH1_CTRL           (ATM_7059_SI_BASE+0x20)
#define     ATM_7059_SI_CH1_ROW_RANGE      (ATM_7059_SI_BASE+0x24)
#define     ATM_7059_SI_CH1_COL_RANGE      (ATM_7059_SI_BASE+0x28)
#define     ATM_7059_SI_CH1_ADDRY          (ATM_7059_SI_BASE+0x2c)
#define     ATM_7059_SI_CH1_ADDRU          (ATM_7059_SI_BASE+0x30)
#define     ATM_7059_SI_CH1_ADDRV          (ATM_7059_SI_BASE+0x34)

#define ATM_7059_CMU_SENSORCLK_INVT0 (0x1 << 12)
#define ATM_7059_CMU_SENSORCLK_INVT1 (0x1 << 13)

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
#define YUV_OUTPUT_FORMAT_UV_REVERSE  (0x1<<10)    //10

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

static struct owl_camera_reg restored_regs[8];




#define RESTORED_REG_NUM  8


void*	noc_si_to_ddr = NULL;	
void*	gpio_dinen = NULL;	
void*	si_reset = NULL;	





static int atm_7059_hw_adapter_init(struct owl_camera_hw_adapter* hw,struct platform_device *pdev)
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

	ATM_7059_SI_BASE = devm_ioremap(&pdev->dev,res->start,resource_size(res));
	if(!ATM_7059_SI_BASE)
		return -ENXIO;
	printk("SI paddr is %x",(int)ATM_7059_SI_BASE);

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
	atm_7059_cmu_sensor_clk = devm_ioremap(&pdev->dev,res->start,resource_size(res));
	if(!atm_7059_cmu_sensor_clk)
		return -ENXIO;
	noc_si_to_ddr = ioremap(0xb0500508,4);
	restored_regs[0].reg = ATM_7059_SI_ENABLE;
	restored_regs[1].reg = ATM_7059_SI_CH0_CTRL;
	restored_regs[2].reg = ATM_7059_SI_INT_STAT;
	restored_regs[3].reg = ATM_7059_SI_CH0_ROW_RANGE;
	restored_regs[4].reg = ATM_7059_SI_CH0_COL_RANGE;
	restored_regs[5].reg = ATM_7059_SI_CH0_ADDRY;
	restored_regs[6].reg = ATM_7059_SI_CH0_ADDRU;
	restored_regs[7].reg = ATM_7059_SI_CH0_ADDRV;
	gpio_dinen = ioremap(0xb01b0028,4);
	si_reset = ioremap(0xb01b001c,4);
	return 0;
	
}



static int atm_7059_get_channel_state(struct owl_camera_hw_adapter* hw, int channel)
{
    
   return readl(ATM_7059_SI_INT_STAT);
}

int atm_7059_get_channel_overflow(struct owl_camera_hw_adapter* hw, int channel)
{
	u32 si_con_stat;
    si_con_stat = readl(ATM_7059_SI_INT_STAT);

    return si_con_stat&(0x3<<10);
}

int atm_7059_clear_channel_overflow(struct owl_camera_hw_adapter* hw, int channel)
{
	 u32 si_con_stat;
    si_con_stat = readl(ATM_7059_SI_INT_STAT);
    //si_con_stat &= ~(0xf<<8);
   // si_con_stat |= (0x3<<10);
    writel(si_con_stat, ATM_7059_SI_INT_STAT);
    return 0;

}



static int atm_7059_set_channel_if(struct owl_camera_hw_adapter* hw, int channel, int bus_type)
{
	unsigned long intf;
    volatile void * reg = 0;
    unsigned long value;
	 // intf
    if (ISP_CHANNEL_0 == channel) {
        reg = ATM_7059_SI_CH0_CTRL;
    } else {
        reg = ATM_7059_SI_CH1_CTRL;
    }
    value = readl(reg);

    if (V4L2_MBUS_PARALLEL == bus_type) {
        intf = SRC_INTF_SENSOR;
    } else {
        intf = SRC_INTF_CSI;
    }
    value &= ~SRC_INTF;
    value |= intf;
    writel(value, reg);
    return 0;
}

static int atm_7059_set_channel_addrY(struct owl_camera_hw_adapter* hw, int channel, void *addrY)
{
    if(ISP_CHANNEL_0 == channel)
    {
        writel((u32)addrY, ATM_7059_SI_CH0_ADDRY); 
    }
    else
    {
        writel((u32)addrY, ATM_7059_SI_CH1_ADDRY);
    }
    return 0;
}
static int atm_7059_set_channel_addrU(struct owl_camera_hw_adapter* hw, int channel, void *addrU)
{
    
    if(ISP_CHANNEL_0 == channel)
    {
        writel((u32)addrU,  ATM_7059_SI_CH0_ADDRU ); 
    }
    else
    {
        writel((u32)addrU, ATM_7059_SI_CH1_ADDRU);
    }
    return 0;
}
static int atm_7059_set_channel_addrV(struct owl_camera_hw_adapter* hw, int channel, void *addrV)
{
    if(ISP_CHANNEL_0 == channel)
    {
        writel((u32)addrV,  ATM_7059_SI_CH0_ADDRV); 
    }
    else
    {
        writel((u32)addrV, ATM_7059_SI_CH1_ADDRV);
    }
    return 0;
}
static int atm_7059_set_channel_addr1UV(struct owl_camera_hw_adapter* hw, int channel, void *addr1UV)
{
	 if(ISP_CHANNEL_0 == channel)
    {
        writel((u32)addr1UV,  ATM_7059_SI_CH0_ADDRU ); 
    }
    else
    {
        writel((u32)addr1UV, ATM_7059_SI_CH1_ADDRU);
    }
    return 0;
}

static int atm_7059_set_channel_input_fmt(struct owl_camera_hw_adapter* hw, int channel, enum v4l2_mbus_pixelcode code)
{
	 unsigned long value;
	 volatile void * reg = 0;
	 if (ISP_CHANNEL_0 == channel) {
        reg = ATM_7059_SI_CH0_CTRL;
    } else {
        reg = ATM_7059_SI_CH1_CTRL;
    }
    value = readl(reg);

    value &= ~YUV_INPUT_FORMAT_MASK;
    value &= ~IN_FMT_MASK;
    switch (code) {
    case V4L2_MBUS_FMT_UYVY8_2X8:  /* UYVY */
        // should be the same as senor's output format
        value |= YUV_INPUT_FMT_UYVY;
        value |= INT_FMT_YUV;
        printk("input format UYVY, 1pix/clk\n");
        break;

    default:
        printk("input data error (pixel code:0x%x)\n", code);
        break;

    }

    writel(value, reg);
	return 0;
}

static int atm_7059_set_channel_output_fmt(struct owl_camera_hw_adapter* hw, int channel, u32 fourcc)
{
	volatile void * reg = 0;
    unsigned int  value;
	 if (ISP_CHANNEL_0 == channel) {
        reg = ATM_7059_SI_CH0_CTRL;
    } else {
        reg = ATM_7059_SI_CH1_CTRL;
    }
    value = readl(reg);

    value &= ~(YUV_OUTPUT_FORMAT_MASK | YUV_OUTPUT_FORMAT_UV_REVERSE);
    switch (fourcc) {
    case V4L2_PIX_FMT_YUV420:  //420 planar
        value |= YUV_OUTPUT_FMT_YUV420;
        printk("output format YUV420\n");
        break;

    case V4L2_PIX_FMT_YUV422P: //422 semi planar
        value |= YUV_OUTPUT_FMT_YUV422_SEMI;
        printk("output format YUV422P\n");
        break;

    case V4L2_PIX_FMT_NV12:    //420 semi-planar
        value |= YUV_OUTPUT_FMT_YUV420_SEMI;
        printk("output format NV12.the value :%x\n",value);
        break;
	 case V4L2_PIX_FMT_NV21:    //420 semi-planar
        value |= YUV_OUTPUT_FMT_YUV420_SEMI|YUV_OUTPUT_FORMAT_UV_REVERSE;
        printk("output format NV21\n");
        break;
	 case V4L2_PIX_FMT_YVU420:    //420 semi-planar
		value |= YUV_OUTPUT_FMT_YUV420;
		printk("output V4L2_PIX_FMT_YVU420\n");
		break;

    case V4L2_PIX_FMT_YUYV:    //interleaved
    case V4L2_PIX_FMT_UYVY:
        value |= YUV_OUTPUT_FMT_YUV422;
        printk("output format YUYV\n");
        break;

    default:   /* Raw RGB */
        printk("set isp output format failed, fourcc = 0x%x", fourcc);
        return -EINVAL;
    }

    writel(value, reg);
    return 0;

}


static int atm_7059_set_channel_preline(struct owl_camera_hw_adapter* hw, int channel, int preline)
{
	  unsigned int state;
	 if (ISP_CHANNEL_0 == channel) {
        state = readl(ATM_7059_SI_ENABLE);
		
        state &= ~CH0_PRELINE_NUM_MASK;
        state |= CH0_PRELINE_NUM(preline + ISP_PRELINE_NUM);
        writel(state, ATM_7059_SI_ENABLE);
    } else {
        state = readl(ATM_7059_SI_ENABLE);
		printk("channel:%d, state:%d\n", channel,state);
        state &= ~CH1_PRELINE_NUM_MASK;
        state |= CH1_PRELINE_NUM(channel + ISP_PRELINE_NUM);
		printk("channel:%d, state:%d\n", channel,state);
        writel(state, ATM_7059_SI_ENABLE);
    }
    return 0;
}

static int atm_7059_clear_channel_int_en(struct owl_camera_hw_adapter* hw, int channel)
{
	 unsigned int isp_enable;
	isp_enable = readl(ATM_7059_SI_ENABLE);

    if (ISP_CHANNEL_0 == channel) {
        isp_enable &= ~(CH0_ENABLE);
    } else {
        isp_enable &= ~(CH1_ENABLE);
    }
    writel(isp_enable, ATM_7059_SI_ENABLE);
    return 0;
}

static int atm_7059_set_channel_preline_int_en(struct owl_camera_hw_adapter* hw, int channel)
{
	
    // int stat
    unsigned long isp_int_stat;
    isp_int_stat = readl(ATM_7059_SI_INT_STAT);
	isp_int_stat &= ~CH0_FRAME_END_IRQ_EN;
    if (ISP_CHANNEL_0 == channel) {
        isp_int_stat |= CH0_PRELINE_IRQ_EN;
    } else {
        isp_int_stat |= CH1_PRELINE_IRQ_EN;
    }
    writel(isp_int_stat, ATM_7059_SI_INT_STAT);
    return 0;
}

static int atm_7059_set_channel_frameend_int_en(struct owl_camera_hw_adapter* hw, int channel)
{

    u32 int_stat;
    int_stat = readl(ATM_7059_SI_INT_STAT);
    int_stat &= ~(CH0_PRELINE_IRQ_EN | CH1_PRELINE_IRQ_EN);
	if(ISP_CHANNEL_0 == channel)
	{
		writel(int_stat | CH0_FRAME_END_IRQ_EN, ATM_7059_SI_INT_STAT);
	}
	else 
	{
		writel(int_stat | CH1_FRAME_END_IRQ_EN, ATM_7059_SI_INT_STAT);
	}
    return 0;
    
}

static int atm_7059_clear_channel_frameend_int_en(struct owl_camera_hw_adapter* hw, int channel)
{

    u32 int_stat;
    int_stat = readl(ATM_7059_SI_INT_STAT);
  //  int_stat &= ~(CH0_PRELINE_IRQ_EN | CH1_PRELINE_IRQ_EN);
	if(ISP_CHANNEL_0 == channel)
	{
		writel(int_stat & (~CH0_FRAME_END_IRQ_EN), ATM_7059_SI_INT_STAT);
	}
	else 
	{
		writel(int_stat & (~CH1_FRAME_END_IRQ_EN), ATM_7059_SI_INT_STAT);
	}
    return 0;
    
}


static int atm_7059_clear_channel_preline_pending(struct owl_camera_hw_adapter* hw, int channel)
{
	u32 isp_int_stat;
    isp_int_stat = readl(ATM_7059_SI_INT_STAT);
	//isp_int_stat &= ~CH0_FRAME_END_IRQ_EN;
   // isp_int_stat |= (0x3<<8);
    writel(isp_int_stat, ATM_7059_SI_INT_STAT);
    return 0;
}
static int atm_7059_clear_channel_preline_int_en(struct owl_camera_hw_adapter* hw)
{
	u32 isp_int_stat;
	isp_int_stat = readl(ATM_7059_SI_INT_STAT);
	isp_int_stat &= ~CH0_PRELINE_IRQ_EN;
    writel(isp_int_stat, ATM_7059_SI_INT_STAT);
    return 0;
}

static int atm_7059_clear_channel_frameend_pending(struct owl_camera_hw_adapter* hw,  unsigned int isp_int_stat)
{
	//isp_int_stat &= ~CH0_FRAME_END_IRQ_EN;
	writel(isp_int_stat, ATM_7059_SI_INT_STAT);
    return 0;
}


static int atm_7059_set_signal_polarity(struct owl_camera_hw_adapter* hw, int channel,unsigned int common_flags)
{
	u32 cmu_sensorclk;
	volatile void * reg = 0;
    unsigned long value;
	cmu_sensorclk = readl(atm_7059_cmu_sensor_clk);
	 if (ISP_CHANNEL_0 == channel) {
        reg = ATM_7059_SI_CH0_CTRL;
        if (common_flags & V4L2_MBUS_PCLK_SAMPLE_FALLING) {
            cmu_sensorclk |= ATM_7059_CMU_SENSORCLK_INVT0;
        } else {
            cmu_sensorclk &= ~ATM_7059_CMU_SENSORCLK_INVT0;
         //  cmu_sensorclk |= ATM_7059_CMU_SENSORCLK_INVT0;
        }
    } else {
        reg = ATM_7059_SI_CH1_CTRL;
        if (common_flags & V4L2_MBUS_PCLK_SAMPLE_FALLING) {
            cmu_sensorclk |= ATM_7059_CMU_SENSORCLK_INVT0;
        } else {
            cmu_sensorclk &= ~ATM_7059_CMU_SENSORCLK_INVT0;
        }
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
    writel(value, reg);
    writel(cmu_sensorclk, atm_7059_cmu_sensor_clk);
	return 0;
}


static int atm_7059_set_col_range(struct owl_camera_hw_adapter* hw,int channel,unsigned int start,unsigned int end)
{
	int ret = 0;
	 if (ISP_CHANNEL_0 == channel) {
        writel(COL_START(start) | COL_END(end), ATM_7059_SI_CH0_COL_RANGE);
    } else {
        writel(COL_START(start) | COL_END(end),  ATM_7059_SI_CH1_COL_RANGE);
    }
	return ret;
}

static int atm_7059_set_row_range(struct owl_camera_hw_adapter* hw,int channel,unsigned int start,unsigned int end)
{
	int ret = 0;
	 if (ISP_CHANNEL_0 == channel) {
        writel(COL_START(start) | COL_END(end), ATM_7059_SI_CH0_ROW_RANGE);
    } else {
        writel(COL_START(start) | COL_END(end), ATM_7059_SI_CH1_ROW_RANGE);
    }
	return ret;
}


static int atm_7059_set_channel_int_en(struct owl_camera_hw_adapter* hw, int channel)
{
	
	//channel enable
	int ret = 0;
	unsigned long isp_enable;
   isp_enable = readl(ATM_7059_SI_ENABLE);
   if (ISP_CHANNEL_0 == channel) {
	   isp_enable |= CH0_ENABLE;
   } else {
	   isp_enable |= CH1_ENABLE;
   }
   writel(isp_enable, ATM_7059_SI_ENABLE);
   return ret;
}
static void atm_7059_save_reg_array(struct owl_camera_reg *regs, int num)
{
	unsigned int reg_i;
	//printk(KERN_ERR"in the atm_7059_save_reg_array \n");
    for(reg_i = 0; reg_i < num; reg_i++)
    {
        regs[reg_i].val = readl(regs[reg_i].reg);
		//printk(KERN_ERR"the reg is 0x%x,value is 0x%x  \n",*((long *)regs[reg_i].reg),regs[reg_i].val);
    }
}

static int atm_7059_save_regs(struct owl_camera_hw_adapter* hw)
{
    atm_7059_save_reg_array(hw->restored_regs, hw->restored_regs_num);
    return 0;
}


static void atm_7059_restore_reg_array(struct owl_camera_reg *regs, int num)
{
	unsigned int reg_i;

    for (reg_i = 0; reg_i < num; reg_i++)
    {
    	//printk(KERN_ERR"the reg is 0x%x,value is 0x%x\n",*((long *)regs[reg_i].reg),regs[reg_i].val);
        writel(regs[reg_i].val, regs[reg_i].reg);
    }
}

int atm_7059_restore_regs(struct owl_camera_hw_adapter* hw)
{
	atm_7059_restore_reg_array(hw->restored_regs, hw->restored_regs_num);
    return 0;	
}

int atm_7059_clear_all_pending(struct owl_camera_hw_adapter* hw)
{
	 u32 isp_int_stat;
    isp_int_stat = readl(ATM_7059_SI_INT_STAT);
    writel(isp_int_stat, ATM_7059_SI_INT_STAT);
	return 0;
}

struct owl_camera_hw_ops atm7059_hw_ops = {
	.hw_adapter_init = atm_7059_hw_adapter_init,
    .get_channel_state = atm_7059_get_channel_state,
    .set_channel_if = atm_7059_set_channel_if,
    .set_channel_addrY = atm_7059_set_channel_addrY,
    .set_channel_addrU = atm_7059_set_channel_addrU,
    .set_channel_addrV = atm_7059_set_channel_addrV,
    .set_channel_addr1UV = atm_7059_set_channel_addr1UV,
    .set_channel_input_fmt = atm_7059_set_channel_input_fmt,
    .set_channel_output_fmt = atm_7059_set_channel_output_fmt,
    .set_channel_preline = atm_7059_set_channel_preline,
    .clear_channel_int_en = atm_7059_clear_channel_int_en,
    .set_channel_preline_int_en = atm_7059_set_channel_preline_int_en,
    .clear_channel_preline_int_en = atm_7059_clear_channel_preline_int_en,
    .set_channel_frameend_int_en = atm_7059_set_channel_frameend_int_en,
    .clear_channel_frameend_int_en = atm_7059_clear_channel_frameend_int_en,
    .clear_channel_preline_pending = atm_7059_clear_channel_preline_pending,
    .clear_channel_frameend_pending = atm_7059_clear_channel_frameend_pending,
    .set_signal_polarity = atm_7059_set_signal_polarity,
    .clear_all_pending = atm_7059_clear_all_pending,
    .set_col_range = atm_7059_set_col_range,
    .set_row_range = atm_7059_set_row_range,
    .set_channel_int_en = atm_7059_set_channel_int_en,
    .get_channel_overflow = atm_7059_get_channel_overflow,
    .clear_channel_overflow = atm_7059_clear_channel_overflow,
    .save_regs = atm_7059_save_regs,
    .restore_regs = atm_7059_restore_regs,
};

struct owl_camera_hw_adapter atm7059_hw_adapter = {
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
	.preline_int_pd = CH0_PRELINE_PEND,
	.frameend_int_pd = CH0_FRAME_PEND,
	.restored_regs_num = RESTORED_REG_NUM,
	.restored_regs = restored_regs,


    .power_ref = 0,
    .enable_ref = 0,
    .hw_clk = NULL,

    .ops = &atm7059_hw_ops,
};

