/*
 * linux/drivers/video/owl/dss/dsi.c
 *
 * Copyright (C) 2009 Actions Corporation
 * Author: Hui Wang  <wanghui@actions-semi.com>
 *
 * Some code and ideas taken from drivers/video/owl/ driver
 * by leopard.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define DSS_SUBSYS_NAME "DSI"
#include <mach/bootdev.h>
#include <linux/kernel.h> 
#include <linux/delay.h>
#include <linux/clk.h>
#include <mach/clkname.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/dmaengine.h> 
#include <linux/dma-mapping.h> 
#include <linux/dma-direction.h> 
#include <asm/cacheflush.h>
#include <asm/outercache.h>
#include <mach/hdmac-owl.h>


#include <video/owldss.h>

#include "../../dss/dss_features.h"
#include "../../dss/dss.h"
#include "dsihw.h"
#include "dsi.h"

#define POWER_REGULATOR_NAME        "dsivcc"

void  *reg_sps_ldo_ctl = 0;	
void  *cmu_dsipll_clk = 0;
	
struct dma_test
{
	unsigned char *dma_buf_a;
	unsigned char *dma_buf_b;
	dma_addr_t dma_phys_a;
	dma_addr_t dma_phys_b;
	struct dma_chan *chan;
	struct dma_device *dev;
	enum dma_ctrl_flags flags;
	struct dma_async_tx_descriptor *dma_desc;

	int callback_control;
	unsigned int total;
	
	unsigned int slave_id_fifo0_tx;
	struct completion	dma_complete;
};

struct dma_test *dma_gl;

struct owl_dsi_gpio {
    int gpio;
    int active_low;
};

struct owl_videomode {
    u32                     refresh;
    u32                     xres;
    u32                     yres;

    /*in pico second, 0.000 000 000 001s*/
    u32                     pixclock;

    u32                     left_margin;
    u32                     right_margin;
    u32                     upper_margin;
    u32                     lower_margin;
    u32                     hsync_len;
    u32                     vsync_len;

    /*0: FB_VMODE_NONINTERLACED, 1:FB_VMODE_INTERLACED*/
    u32                     vmode;
};
	
struct owl_dsi_reg {
    u32                     dsi_ctrl;
    u32                     dsi_size;
    u32                     dsi_color;
    u32                     dsi_rgbht0;
    u32                     dsi_rgbht1;		
	u32                     dsi_rgbvt0;
    u32                     dsi_rgbvt1;
    u32                     dsi_pack_cfg;
    u32                     dsi_pack_header;
	u32						dsi_vedio_cfg;
    u32                     dsi_phy_t0;		
	u32                     dsi_phy_t1;
    u32                     dsi_phy_t2;
    u32                     dsi_phy_ctrl;
    u32                     dsi_pin_map;
	u32 					cmu_dsipll_clk;
};

struct dsi_data {
	
    struct platform_device  *pdev;
    void __iomem            *base;
    struct owl_dss_device   *dssdev;

    struct clk              *dsi_clk;
    struct regulator        *dsi_power;

    /* the followings are strict with DTS */
    struct owl_dsi_gpio    power_gpio;
	struct owl_dsi_gpio    reset_gpio;
	
    u32                     port_type;

    u32                     data_width;

    u32                     num_modes;
    struct owl_videomode   *modes;
	
	struct owl_dsi_reg     *regs;
    /* end of strict with DTS */

    bool                    dsi_enabled;
    
    struct mutex            lock;
};

static bool boot_dsi_inited;
static struct platform_device *dsi_pdev_map[MAX_NUM_LCD];

static void dma_callback(void *param)
{
	struct dma_test *dmatest = param;

	complete(&dmatest->dma_complete);		
	DEBUG_DSI("dma_callback~~~~~dmatest  %p\n", dmatest);
	return;
}

static int start_dma(struct dma_test *dmatest, int *pbuf, int len)
{

	dma_addr_t dma_addr;
	dma_cookie_t cookie = -EINVAL;
	DEBUG_DSI("start dma dmatest->chan %p   pbuf  %p dma_gl->dma_buf_a %p!!!!\n", dmatest->chan, pbuf, dma_gl->dma_buf_a);
	dma_addr = dma_map_single(dmatest->chan->device->dev, pbuf, len, DMA_MEM_TO_DEV);
	DEBUG_DSI("dma_map_single end!!!!\n");
	if (dma_addr)
		dmatest->dma_desc = dmaengine_prep_slave_single(dmatest->chan, dma_addr, len,
			DMA_MEM_TO_DEV, DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
			
	DEBUG_DSI("device_prep_dma_memcpy end!!!!\n");
	if (!dmatest->dma_desc) {
		pr_err("Not able to get desc for TX\n");
		if (dmatest->chan) {
			dma_release_channel(dmatest->chan);
			dmatest->chan = NULL;
		}		
		goto out;
	}

	dmatest->dma_desc->callback = dma_callback;
	dmatest->dma_desc->callback_param = dmatest;
	DEBUG_DSI("dmaengine_submit !!!!\n");
	cookie = dmaengine_submit(dmatest->dma_desc);
	DEBUG_DSI("dma_async_issue_pending !!!!\n");
	dma_async_issue_pending(dmatest->chan);
	DEBUG_DSI("dma_async_issue_pending end !!!!\n");

	DEBUG_DSI("dma end!!!!\n");

out:
	dma_unmap_single(dmatest->chan->device->dev, dma_addr, len, DMA_MEM_TO_DEV);
	return -ENODEV;	
}

int dma_init(void)
{
	int ret;
	dma_cap_mask_t mask;
	struct owl_dma_slave *acts_slave;
	struct dma_slave_config cfg;	
	
	DEBUG_DSI("gl++++dmatest_init start \n");
	dma_gl = kzalloc(sizeof(struct dma_test), GFP_KERNEL);
	DEBUG_DSI("dma_gl  kzalloc end \n");
	
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_gl->chan = dma_request_channel(mask, NULL, NULL);
	if (!dma_gl->chan) {
		pr_err("Failed to request DMA channel\n");
		goto err;
	}
	dma_gl->dev = dma_gl->chan->device;
	DEBUG_DSI("%s: TX: got channel %p   id %d\n", __func__, dma_gl->chan, dma_gl->chan->chan_id);
		
	acts_slave = devm_kzalloc(dma_gl->dev->dev, sizeof(*acts_slave), GFP_KERNEL);
	DEBUG_DSI("acts_slave %p \n", acts_slave );	
	if (!acts_slave) {
		pr_err("Not able to alloc struct acts_dma_slave\n");
		goto err;
	}
	acts_slave->dma_dev = dma_gl->dev->dev;
	acts_slave->trans_type = SLAVE;
	acts_slave->mode = 0x00010224;
	dma_gl->chan->private = acts_slave;
	
	memset(&cfg, 0, sizeof(cfg));
	cfg.direction = DMA_MEM_TO_DEV;
	cfg.dst_addr = 0xb0220030;
	cfg.src_addr = 0xb0220030;
	cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	cfg.device_fc = false;
	ret = dmaengine_slave_config(dma_gl->chan, &cfg);
	if (ret < 0)
	{
		DEBUG_DSI("dmaengine_slave_config ret %x\n", ret);
		goto err;
	}	
	
	init_completion(&dma_gl->dma_complete);
	DEBUG_DSI("gl++++dma_init end\n");
	return 0;

err:
	if (dma_gl->chan) {
		dma_release_channel(dma_gl->chan);
		dma_gl->chan = NULL;
	}		
	return -ENODEV;		

}

inline struct dsi_data *dsihw_get_dsidrv_data(struct platform_device *pdev)
{
	return dev_get_drvdata(&pdev->dev);
}

inline void dsihw_write_reg(struct platform_device *pdev,const u16 index, u32 val)
{
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);
	//DEBUG_DSI("dsihw_write_reg  ~~~ %p index %d\n",dsi->base,index);
	writel(val, dsi->base + index);
}

inline u32 dsihw_read_reg(struct platform_device *pdev,const u16 index)
{
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);
	//DEBUG_DSI("dsihw_read_reg  ~~~ %p index %d\n",dsi->base,index);
	return readl(dsi->base + index);
}

void dsihw_dump_regs(struct platform_device *pdev){
	
#define DUMPREG(name,r) printk("%s %08x\n",name,dsihw_read_reg(pdev, r))

     DUMPREG("DSI_CTRL  	value is ", DSI_CTRL);
     DUMPREG("DSI_SIZE  	value is ", DSI_SIZE);
     DUMPREG("DSI_COLOR  	value is ", DSI_COLOR);
	 DUMPREG("DSI_VIDEO_CFG value is ", DSI_VIDEO_CFG);
     DUMPREG("DSI_RGBHT0  	value is ", DSI_RGBHT0);
     DUMPREG("DSI_RGBHT1  	value is ", DSI_RGBHT1);
     DUMPREG("DSI_RGBVT0  	value is ", DSI_RGBVT0);
     DUMPREG("DSI_RGBVT1  	value is ", DSI_RGBVT1);
     DUMPREG("DSI_TIMEOUT 	value is ", DSI_TIMEOUT);     
     DUMPREG("DSI_TR_STA  	value is ", DSI_TR_STA);
     DUMPREG("DSI_INT_EN  	value is ", DSI_INT_EN);
     DUMPREG("DSI_ERROR_REPORT  value is ", DSI_ERROR_REPORT);
     DUMPREG("DSI_FIFO_ODAT  value is ", DSI_FIFO_ODAT);     
     DUMPREG("DSI_FIFO_IDAT  value is ", DSI_FIFO_IDAT);
     DUMPREG("DSI_IPACK  value is ", DSI_IPACK);
     DUMPREG("DSI_PACK_CFG  value is ", DSI_PACK_CFG);     
     DUMPREG("DSI_PACK_HEADER  value is ", DSI_PACK_HEADER);
     DUMPREG("DSI_TX_TRIGGER  value is ", DSI_TX_TRIGGER);
     DUMPREG("DSI_RX_TRIGGER  value is ", DSI_RX_TRIGGER);
     DUMPREG("DSI_LANE_CTRL  value is ", DSI_LANE_CTRL);
     DUMPREG("DSI_LANE_STA  value is ", DSI_LANE_STA);
     DUMPREG("DSI_PHY_T0  value is ", DSI_PHY_T0);
     DUMPREG("DSI_PHY_T1  value is ", DSI_PHY_T1);
     DUMPREG("DSI_PHY_T2  value is ", DSI_PHY_T2);
     DUMPREG("DSI_APHY_DEBUG0  value is ", DSI_APHY_DEBUG0);         
     DUMPREG("DSI_APHY_DEBUG1  value is ", DSI_APHY_DEBUG1);
     DUMPREG("DSI_SELF_TEST  value is ", DSI_SELF_TEST);
     DUMPREG("DSI_PIN_MAP  value is ", DSI_PIN_MAP);
     DUMPREG("DSI_PHY_CTRL  value is ", DSI_PHY_CTRL);     
     DUMPREG("DSI_FT_TEST  value is ", DSI_FT_TEST); 

}

static int wait_lanes_stop(void)
{
	u32 tmp;
	int cnt = 1000;
	
	struct platform_device *pdev = dsi_pdev_map[0] ;
	do {
		tmp = dsihw_read_reg(pdev, DSI_LANE_STA);
		if ((tmp & (1 << 12)) || (tmp & (1 << 5)))
			break;

		udelay(2);
	} while (--cnt);

	if (cnt <= 0) {
		printk("lanes cannot STOP\n");
		return -1;
	}

	return 0;
}

static int dsihw_set_dsi_clk(void)
{
	u32 reg_val=0;
    struct platform_device *pdev = dsi_pdev_map[0] ;
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);
	reg_val = dsi->regs->cmu_dsipll_clk;
	writel(reg_val, cmu_dsipll_clk);	

	return 0;
}

static void dsihw_phy_config(struct platform_device *pdev)
{	
	int tmp,cmt=1000;
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);

	DEBUG_DSI("phy set start %p \n", pdev);
	tmp = 0x1300;
	dsihw_write_reg(pdev,DSI_CTRL, tmp);	


	tmp = dsi->regs->dsi_phy_t0;
	dsihw_write_reg(pdev,DSI_PHY_T0, tmp);	
	

	tmp = dsi->regs->dsi_phy_t1;
	dsihw_write_reg(pdev,DSI_PHY_T1, tmp);	

	tmp = dsi->regs->dsi_phy_t2;
	dsihw_write_reg(pdev,DSI_PHY_T2, tmp);	
	
	tmp = dsi->regs->dsi_phy_ctrl;
	dsihw_write_reg(pdev,DSI_PHY_CTRL, tmp);	
	
	tmp = 0x688;
	dsihw_write_reg(pdev,DSI_PIN_MAP, tmp);	

	
	tmp = dsihw_read_reg(pdev,DSI_PHY_CTRL);
	tmp |= (1<<24);
	dsihw_write_reg(pdev,DSI_PHY_CTRL, tmp);
	
	mdelay(20);
	tmp = dsihw_read_reg(pdev,DSI_PHY_CTRL);
	tmp |= ((1<<25)|(1<<28));
	dsihw_write_reg(pdev,DSI_PHY_CTRL, tmp);
	
	while ((!(dsihw_read_reg(pdev,DSI_PHY_CTRL) & (1 << 31))) && --cmt)
	udelay(2);
	
	
	udelay(100);
	tmp=dsihw_read_reg(pdev,DSI_PHY_CTRL);
	if(tmp&0x02000000){
		
		printk("ERR : dsi cal fail!!\n");

		tmp = dsihw_read_reg(pdev,DSI_PHY_CTRL);//disable calibrate
		tmp &= (~(1<<25));
		dsihw_write_reg(pdev,DSI_PHY_CTRL, tmp);
		
		tmp = dsihw_read_reg(pdev,DSI_LANE_CTRL); //force clock lane 
		tmp |= ((1<<1)|(1<<4));
		dsihw_write_reg(pdev,DSI_LANE_CTRL, tmp);
		
		tmp = dsihw_read_reg(pdev,DSI_PHY_CTRL);   //Select output node 
		tmp |= (3<<2);
		dsihw_write_reg(pdev,DSI_PHY_CTRL, tmp);
		
		tmp = dsihw_read_reg(pdev,DSI_PHY_CTRL);  //Disable (D-PHY is 
		tmp &= (~(1<<24));
		dsihw_write_reg(pdev,DSI_PHY_CTRL, tmp);
		
		tmp = dsihw_read_reg(pdev,DSI_PHY_CTRL);//Enable (D-PHY is 
		tmp |= (1<<24);
		dsihw_write_reg(pdev,DSI_PHY_CTRL, tmp);

	}
	
	wait_lanes_stop();
	
	mdelay(2);
	dsihw_write_reg(pdev,DSI_TR_STA,0x3);//Clear LP1 & LP0 Error
	
	tmp = dsi->regs->dsi_pin_map;
	dsihw_write_reg(pdev,DSI_PIN_MAP, tmp);
	
	DEBUG_DSI("phy set end~~~~\n");   
	
	/*data0 line ,clk line in stop state */
	wait_lanes_stop();
	
	tmp = dsihw_read_reg(pdev,DSI_CTRL);
	tmp |= 0x40;
	dsihw_write_reg(pdev,DSI_CTRL, tmp);	
	/*data0 line ,clk line in stop state */	
	wait_lanes_stop();
}

static void dsihw_video_config(struct platform_device *pdev)
{	
	int tmp;
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);

	//tmp = 0x13d0; //default color
	tmp = dsi->regs->dsi_ctrl;
	dsihw_write_reg(pdev,DSI_CTRL, tmp);	

	tmp = dsi->regs->dsi_size;
	dsihw_write_reg(pdev,DSI_SIZE, tmp);	

	tmp = dsi->regs->dsi_color;
	dsihw_write_reg(pdev,DSI_COLOR, tmp);	

	tmp = dsi->regs->dsi_rgbht0;
	dsihw_write_reg(pdev,DSI_RGBHT0, tmp);	

	tmp = dsi->regs->dsi_rgbht1;
	dsihw_write_reg(pdev,DSI_RGBHT1, tmp);	

	tmp = dsi->regs->dsi_rgbvt0;
	dsihw_write_reg(pdev,DSI_RGBVT0, tmp);	

	tmp = dsi->regs->dsi_rgbvt1;
	dsihw_write_reg(pdev,DSI_RGBVT1, tmp);	

	tmp = dsi->regs->dsi_pack_cfg;
	dsihw_write_reg(pdev,DSI_PACK_CFG, tmp);	

	tmp = dsi->regs->dsi_pack_header;
	dsihw_write_reg(pdev,DSI_PACK_HEADER, tmp);	

	tmp = dsi->regs->dsi_vedio_cfg;
	dsihw_write_reg(pdev,DSI_VIDEO_CFG, tmp);	
}

static int dsihw_power_enable(struct platform_device *dsidev, bool enable) {
    int ret = 0;
	int temp;
    struct dsi_data *dsi = dsihw_get_dsidrv_data(dsidev);

    if (enable) {
        if (gpio_is_valid(dsi->power_gpio.gpio)) {
            gpio_direction_output(dsi->power_gpio.gpio,
                                  !dsi->power_gpio.active_low);
        }

        if (dsi->dsi_power) {
            ret = regulator_enable(dsi->dsi_power);
        }
		
		temp = readl(reg_sps_ldo_ctl);
		temp |= (1<<11);
		writel(temp, reg_sps_ldo_ctl);	
		mdelay(50);			
		
		if (gpio_is_valid(dsi->reset_gpio.gpio)){
			printk("reset_gpio  is ok  %d\n", dsi->reset_gpio.active_low);
		//	gpio_direction_output(dsi->reset_gpio.gpio,
      //                            !dsi->reset_gpio.active_low);
			mdelay(30);					  
	        gpio_direction_output(dsi->reset_gpio.gpio,
                                  dsi->reset_gpio.active_low);
			mdelay(60);
	        gpio_direction_output(dsi->reset_gpio.gpio,
                                  !dsi->reset_gpio.active_low);
		}
    } else {
        if (gpio_is_valid(dsi->power_gpio.gpio)) {
            gpio_direction_output(dsi->power_gpio.gpio,
                                  dsi->power_gpio.active_low);
        }
         mdelay(50);
      /*  if (gpio_is_valid(dsi->reset_gpio.gpio)){
			
		        gpio_direction_output(dsi->reset_gpio.gpio,
                                  dsi->reset_gpio.active_low);
				mdelay(20);
	       */
	//       printk("reset is no control######################\n");
		//}
    }

    return ret;
}

void dsihw_send_short_packet(struct platform_device *pdev,int data_type, int sp_data, int trans_mode)
{
	int tmp;	
	int cnt = 100;
	DEBUG_DSI("send short start\n");
	tmp = dsihw_read_reg(pdev,DSI_CTRL);
	tmp &= 0xffffefff;
	dsihw_write_reg(pdev,DSI_CTRL, tmp);	
	
	dsihw_write_reg(pdev,DSI_PACK_HEADER, sp_data);	
	
	tmp = (data_type << 8) | (trans_mode << 14);
	dsihw_write_reg(pdev,DSI_PACK_CFG, tmp);	
	mdelay(2);
	
	tmp = dsihw_read_reg(pdev,DSI_PACK_CFG);
	tmp |= 1 ;
	dsihw_write_reg(pdev,DSI_PACK_CFG, tmp);	
		
	while ((!(dsihw_read_reg(pdev, DSI_TR_STA) & (1 << 19))) && --cnt)
		udelay(2);
	
	dsihw_write_reg(pdev,DSI_TR_STA, 0x80000);
	DEBUG_DSI("send short end\n");

}

void dsihw_send_long_packet(struct platform_device *pdev, int data_type, int word_cnt, int * send_data, int trans_mode)
{	

	u32 val=0;
	int tmp,word_cnt_tmp;
	int cnt = 100;
	int timeout;
	DEBUG_DSI("dsihw_send_long_packet pdev %p\n", pdev);
	if((dma_gl==NULL)||(dma_gl->chan==NULL)){
		if(dma_init()){
			pr_err("dma_gl->chan==NULL!!!!\n");
			return;		
		}	
	}
	
	tmp = dsihw_read_reg(pdev,DSI_CTRL);
	tmp &= 0xffffefff;
	dsihw_write_reg(pdev,DSI_CTRL, tmp);	
	trans_mode = 0x1;

	word_cnt_tmp=word_cnt%4==0? word_cnt:((word_cnt-word_cnt%4)+4);//ÒªÇóword¶ÔÆë	
	DEBUG_DSI("start dma\n");
	start_dma(dma_gl, send_data, word_cnt_tmp);
	DEBUG_DSI("end dma\n");
		
	dsihw_write_reg(pdev, DSI_PACK_HEADER,word_cnt);
	dsihw_write_reg(pdev, DSI_PACK_CFG,(data_type << 8) | 0x40000 | (trans_mode << 14));
	udelay(15);
	//msleep(1);
	tmp = dsihw_read_reg(pdev,DSI_PACK_CFG);
	DEBUG_DSI("tmp = %x\n", tmp);
	dsihw_write_reg(pdev,DSI_PACK_CFG, tmp+1);

	//Transmit Complete
	while ((!(dsihw_read_reg(pdev, DSI_TR_STA) & (1 << 19))) && --cnt)
		udelay(2);
	
	timeout = wait_for_completion_timeout(&dma_gl->dma_complete,msecs_to_jiffies(500));
    if(timeout==0){
		printk("err: dma  not done\n");
	}
	
	DEBUG_DSI("Transmit Complete\n");
	
	//Clear TCIP
	val = REG_SET_VAL(val, 1, 19, 19);	
	
	dsihw_write_reg(pdev,DSI_TR_STA, val);	
	
	dsihw_write_reg(pdev,DSI_TR_STA, 0xfff);

}

static void dsihw_single_enable(struct platform_device *pdev, bool enable)
{
	int tmp;
	if(enable){
		tmp = dsihw_read_reg(pdev,DSI_VIDEO_CFG);	
		tmp |= 0x01;
		dsihw_write_reg(pdev,DSI_VIDEO_CFG, tmp);		
	}else{
		tmp = dsihw_read_reg(pdev,DSI_VIDEO_CFG);	
		tmp &= (~0x01);
		dsihw_write_reg(pdev,DSI_VIDEO_CFG, tmp);		
		
		tmp = dsihw_read_reg(pdev,DSI_PHY_CTRL);
		tmp &= (~0x01<<24);
		dsihw_write_reg(pdev,DSI_PHY_CTRL, tmp);
	}

}

static void dsihw_display_init_dsi(void)
{
	struct platform_device *pdev = dsi_pdev_map[0] ;
	DEBUG_DSI("test dsi pdev %p\n", pdev);

	dsihw_power_enable(pdev, true);
	dsihw_set_dsi_clk();	
	dsihw_phy_config(pdev);
	send_cmd(pdev);
	dsihw_video_config(pdev);	
	dsihw_single_enable(pdev, true);
	DEBUG_DSI("dsihw_display_init_dsi end\n");
}

static int dsihw_get_clocks(struct platform_device *pdev)
{
#if 0	
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);

	clk = clk_get(&pdev->dev, "lcd");
	if (IS_ERR(clk)) {
		DSSERR("can't get fck\n");
		return PTR_ERR(clk);
	}

	dsi->dsi_clk = clk;
#endif 
	return 0;
}

static void dsihw_put_clocks(struct platform_device *pdev)
{
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);

	if (dsi->dsi_clk)
		clk_put(dsi->dsi_clk);

}

static int dsihw_parse_gpio(struct device_node *of_node,
                             const char *propname,
                             struct owl_dsi_gpio *gpio)
{
    enum of_gpio_flags flags;
    int                gpio_num;

    gpio_num = of_get_named_gpio_flags(of_node, propname, 0, &flags);
    if (gpio_num >= 0) {
        gpio->gpio = gpio_num;
    } else {
        gpio->gpio = -1;
    }

    gpio->active_low = flags & OF_GPIO_ACTIVE_LOW;

    DEBUG_DSI("%s, gpio = %d\n", __func__, gpio->gpio);
    DEBUG_DSI("%s, active low = %d\n", __func__, gpio->active_low);

    return 0;
}


static int dsihw_parse_params(struct platform_device *pdev,
                               struct dsi_data *dsi)
{
    struct device_node      *of_node;
    char                    propname[20];
    struct device_node      *mode_node;
	struct owl_dsi_reg    	*reg_val;
	struct owl_videomode 	*vmode;
    int                      ret;

    of_node = pdev->dev.of_node;

    /* 
     * power gpio
     */
    dsihw_parse_gpio(of_node, "dsi_power_gpios", &dsi->power_gpio);
    if (dsi->power_gpio.gpio < 0) {
        DSSERR("%s, fail to get dsi power gpio\n", __func__);
    }
		
	 /* 
     * reset gpio
     */
    dsihw_parse_gpio(of_node, "dsi_reset_gpios", &dsi->reset_gpio);
    if (dsi->reset_gpio.gpio < 0) {
        DSSERR("%s, fail to get lcd reset gpio\n", __func__);
    }


    /* 
     * interface timing
     */
	 

    dsi->port_type = 5;

    if (of_property_read_u32(of_node, "data_width", &dsi->data_width)) {
        return -EINVAL;
    }
    DEBUG_DSI("data_width = %d\n", dsi->data_width);
	
    /* 
     * video mode
     */
	sprintf(propname, "hw-set");
    DEBUG_DSI("propname = %s\n", propname);
	mode_node = of_parse_phandle(of_node, propname, 0);
	if (!mode_node) {
		return -EINVAL;
	}
	
	reg_val = kzalloc(sizeof(struct owl_dsi_reg), GFP_KERNEL);
	if (!reg_val) {
        return -EINVAL;
    }

    if (of_property_read_u32(mode_node, "dsi_ctrl", &reg_val->dsi_ctrl)) {
		ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_ctrl = %x\n", reg_val->dsi_ctrl);
	
	if (of_property_read_u32(mode_node, "dsi_size", &reg_val->dsi_size)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_size = %x\n", reg_val->dsi_size);
	
	if (of_property_read_u32(mode_node, "dsi_color", &reg_val->dsi_color)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_color = %x\n", reg_val->dsi_color);
	
	if (of_property_read_u32(mode_node, "dsi_rgbht0", &reg_val->dsi_rgbht0)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_rgbht0 = %x\n", reg_val->dsi_rgbht0);
	
	if (of_property_read_u32(mode_node, "dsi_rgbht1", &reg_val->dsi_rgbht1)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_rgbht1 = %x\n", reg_val->dsi_rgbht1);
	
	if (of_property_read_u32(mode_node, "dsi_rgbvt0", &reg_val->dsi_rgbvt0)) {
		ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_rgbvt0 = %x\n", reg_val->dsi_rgbvt0);
	
	if (of_property_read_u32(mode_node, "dsi_rgbvt1", &reg_val->dsi_rgbvt1)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_rgbvt1 = %x\n", reg_val->dsi_rgbvt1);
	
	if (of_property_read_u32(mode_node, "dsi_pack_cfg", &reg_val->dsi_pack_cfg)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_pack_cfg = %x\n", reg_val->dsi_pack_cfg);
	
	if (of_property_read_u32(mode_node, "dsi_pack_header", &reg_val->dsi_pack_header)) {
		ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_pack_header = %x\n", reg_val->dsi_pack_header);
	
	if (of_property_read_u32(mode_node, "dsi_vedio_cfg", &reg_val->dsi_vedio_cfg)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_vedio_cfg = %x\n", reg_val->dsi_vedio_cfg);
	
	if (of_property_read_u32(mode_node, "dsi_phy_t0", &reg_val->dsi_phy_t0)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_phy_t0 = %x\n", reg_val->dsi_phy_t0);
	
	if (of_property_read_u32(mode_node, "dsi_phy_t1", &reg_val->dsi_phy_t1)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_phy_t1 = %x\n", reg_val->dsi_phy_t1);
	
	if (of_property_read_u32(mode_node, "dsi_phy_t2", &reg_val->dsi_phy_t2)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_phy_t2 = %x\n", reg_val->dsi_phy_t2);
	
	if (of_property_read_u32(mode_node, "dsi_phy_ctrl", &reg_val->dsi_phy_ctrl)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_phy_ctrl = %x\n", reg_val->dsi_phy_ctrl);
	
	if (of_property_read_u32(mode_node, "dsi_pin_map", &reg_val->dsi_pin_map)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("dsi_pin_map = %x\n", reg_val->dsi_pin_map);
	
	if (of_property_read_u32(mode_node, "cmu_dsipll_clk", &reg_val->cmu_dsipll_clk)) {
        ret = -EINVAL;
        goto parse_reg_fail;
    }
    DEBUG_DSI("cmu_dsipll_clk = %x\n", reg_val->cmu_dsipll_clk);
	
	dsi->regs         = reg_val;
	
    /* get  mode */
	sprintf(propname, "videomode-0");
    DEBUG_DSI("propname = %s\n", propname);
	mode_node = of_parse_phandle(of_node, propname, 0);
	if (!mode_node) {
		return -EINVAL;
	}
	
    /* alloc memory */
    vmode = kzalloc(sizeof(struct owl_videomode), GFP_KERNEL);
    if (!vmode) {
        return -EINVAL;
    }

	if (of_property_read_u32(mode_node, "refresh", &vmode->refresh)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("refresh = %d\n", vmode->refresh);

	if (of_property_read_u32(mode_node, "xres", &vmode->xres)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("xres = %d\n", vmode->xres);

	if (of_property_read_u32(mode_node, "yres", &vmode->yres)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("yres = %d\n", vmode->yres);

	if (of_property_read_u32(mode_node, "pixclock", &vmode->pixclock)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("pixclock = %d\n", vmode->pixclock);

	if (of_property_read_u32(mode_node, "left_margin", &vmode->left_margin)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("left_margin = %d\n", vmode->left_margin);

	if (of_property_read_u32(mode_node, "right_margin", &vmode->right_margin)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("right_margin = %d\n", vmode->right_margin);

	if (of_property_read_u32(mode_node, "upper_margin", &vmode->upper_margin)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("upper_margin = %d\n", vmode->upper_margin);

	if (of_property_read_u32(mode_node, "lower_margin", &vmode->lower_margin)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("lower_margin = %d\n", vmode->lower_margin);

	if (of_property_read_u32(mode_node, "hsync_len", &vmode->hsync_len)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("hsync_len = %d\n", vmode->hsync_len);

	if (of_property_read_u32(mode_node, "vsync_len", &vmode->vsync_len)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("vsync_len = %d\n", vmode->vsync_len);

	if (of_property_read_u32(mode_node, "vmode", &vmode->vmode)) {
		ret = -EINVAL;
		goto parse_mode_fail;
	}
	DEBUG_DSI("vmode = %d\n", vmode->vmode);
  
    dsi->modes         = vmode;

    return 0;

parse_mode_fail:
    kfree(vmode);
parse_reg_fail:
	kfree(reg_val);
    return ret;
}

void owl_dsi_select_video_timings(struct owl_dss_device *dssdev, u32 num,
                                   struct owl_video_timings *timings)
{

    struct platform_device *pdev = dsi_pdev_map[0] ;
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);

    struct owl_videomode *mode = dsi->modes;

    timings->x_res          = mode->xres;
    timings->y_res          = mode->yres;
    timings->pixel_clock    = PICOS2KHZ(mode->pixclock);
    timings->hfp            = mode->left_margin;
    timings->hbp            = mode->right_margin;
    timings->vfp            = mode->upper_margin;
    timings->vbp            = mode->lower_margin;
    timings->hsw            = mode->hsync_len;
    timings->vsw            = mode->vsync_len;
	
	timings->data_width     = 24;
	switch(dsi->data_width){
		case 0:
		timings->data_width     = 24;
		break;
		case 1:
		timings->data_width     = 18;
		break;
		case 2:
		timings->data_width     = 16;
		break;
	}
	
    return;
}

static bool dsihw_check_boot_dsi_inited(struct platform_device *pdev) {

	boot_dsi_inited = dsihw_read_reg(pdev,DSI_VIDEO_CFG)&0x01;	
	
    return boot_dsi_inited;
}


int owl_dsi_display_enable(struct owl_dss_device *dssdev)
{
	int r=0;
    struct platform_device *pdev = dsi_pdev_map[0] ;
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);

	mutex_lock(&dsi->lock);

	if (dssdev->manager == NULL) {
		DSSERR("failed to enable display: no manager\n");
		goto err_start_dev;
	}
	
	r = owl_dss_start_device(dssdev);	
	if (r) {
		DSSERR("failed to start device\n");
		goto err_start_dev;
	}
	
	r = dss_mgr_enable(dssdev->manager);
	
	if (r)
		goto err_mgr_enable;
		
	
	if(!dsihw_check_boot_dsi_inited(pdev)){
		dsihw_display_init_dsi();	
		dsihw_single_enable(pdev,true);	
		
	}
	
	mutex_unlock(&dsi->lock);

	DEBUG_DSI("gl++++owl_dsi_display_enable end\n");	
	return 0;
	
err_mgr_enable:
err_reg_enable:	
err_start_dev:
	mutex_unlock(&dsi->lock);
	return r;
}

void owl_dsi_display_disable(struct owl_dss_device *dssdev)
{
	int r;
	struct platform_device *pdev = dsi_pdev_map[0] ;
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);
	
	dsihw_single_enable(pdev,false);	
	
	dss_mgr_disable(dssdev->manager);

	if(dsi->dsi_power){
		r = regulator_disable(dsi->dsi_power);
		if (r){
		}
	}
	
	dsihw_power_enable(pdev, false);

	owl_dss_stop_device(dssdev);
}

void io_map(void)
{
	cmu_dsipll_clk = ioremap(0xb0160028, 4);	
	reg_sps_ldo_ctl  = ioremap(0xb01b0108, 4);
}

/* DSI HW IP initialisation */
static int owl_dsihw_probe(struct platform_device *pdev)
{
	int r;
	struct resource *dsi_mem;
	struct dsi_data *dsi;
	printk("owl_dsihw_probe\n");
	

	DEBUG_DSI("owl_dsihw_probe called \n");
	dsi = kzalloc(sizeof(*dsi), GFP_KERNEL);
	if (!dsi) {
		r = -ENOMEM;
		goto err_alloc;
	}

	dsi->pdev = pdev;
	dsi_pdev_map[0] = pdev;
	dev_set_drvdata(&pdev->dev, dsi);
	
	io_map();

	mutex_init(&dsi->lock);

	r = dsihw_get_clocks(pdev);
	if (r)
		goto err_get_clk;

	pm_runtime_enable(&pdev->dev);

	dsi_mem = platform_get_resource(dsi->pdev, IORESOURCE_MEM, 0);
	
	if (!dsi_mem) {
		DSSERR("can't get IORESOURCE_MEM DSI\n");
		r = -EINVAL;
		goto err_ioremap;
	}	
	
	dsi->base = ioremap(dsi_mem->start, resource_size(dsi_mem));
	if (!dsi->base) {
		DSSERR("can't ioremap dsi \n");
		r = -ENOMEM;
		goto err_ioremap;
	}
	DEBUG_DSI("dsi_mem->start  ~~~ %x  resource_size %x\n",dsi_mem->start, resource_size(dsi_mem));	
	DEBUG_DSI("dsi->base  ~~~ %p \n",dsi->base);	
	
	owl_dsi_create_sysfs(&pdev->dev);	

    r = dsihw_parse_params(pdev, dsi);
    if (r) {
        DSSERR("%s, parse dsi params error\n", __func__);
        goto err_parse_params;
    }

    /*
     * configure gpio
     */
    if (gpio_is_valid(dsi->power_gpio.gpio)) {
        r = gpio_request(dsi->power_gpio.gpio, NULL);
        if (r) {
            DSSERR("%s, request power gpio failed\n", __func__);
            goto err_parse_params;
        }
    } 
    if (gpio_is_valid(dsi->reset_gpio.gpio)) {
        r = gpio_request(dsi->reset_gpio.gpio, NULL);
        if (r) {
            DSSERR("%s, request reset_gpio failed\n", __func__);
            goto err_parse_params;
        }
    } 
	
    dsi->dsi_power = regulator_get(&pdev->dev, POWER_REGULATOR_NAME);
    if (IS_ERR(dsi->dsi_power)) {
        dsi->dsi_power = NULL;
    }
    DEBUG_DSI("%s, dsi_power: %p\n", __func__, dsi->dsi_power);

	if(!dsihw_check_boot_dsi_inited(pdev)){
		dsihw_display_init_dsi();
	}
	
    DSSINFO("owl_dsihw_probe called  ok ~~~~~~~~~~~~~\n");
    return 0;

err_parse_params:
	iounmap(dsi->base);
err_ioremap:
	pm_runtime_disable(&pdev->dev);
err_get_clk:
	kfree(dsi);
err_alloc:
	return r;
}

static int owl_dsihw_remove(struct platform_device *pdev)
{
	struct dsi_data *dsi = dsihw_get_dsidrv_data(pdev);

	pm_runtime_disable(&pdev->dev);

	dsihw_put_clocks(pdev);

	if (dsi->dsi_power != NULL) {
		regulator_put(dsi->dsi_power);
		dsi->dsi_power = NULL;
	}

	iounmap(dsi->base);

	kfree(dsi);

    dsi->dsi_enabled = false;
    return 0;
}

static struct of_device_id owl_dsihw_of_match[] = {
    { .compatible = "actions,owl-dsi", },
    { },
};


static struct platform_driver owl_dsi_driver = {
    .driver = {
        .name           = "owl_dsihw",
        .owner          = THIS_MODULE,
        .of_match_table = owl_dsihw_of_match,
    },
    .probe              = owl_dsihw_probe,
    .remove             = owl_dsihw_remove,
};

int owl_dsi_init_platform(void)
{
    int ret = 0;
	
   if (owl_get_boot_mode() == OWL_BOOT_MODE_UPGRADE) {
		printk("product process  not need to DSI modules!\n");
		return -1;
	}
    ret = platform_driver_register(&owl_dsi_driver);
    
    if (ret) {
        DSSERR("Failed to initialize dsi platform driver\n");
        return ret;
    }
    return 0;
}

int owl_dsi_uninit_platform(void)
{   
    platform_driver_unregister(&owl_dsi_driver);
    
    return 0;
}

void dsihw_fs_dump_regs(struct device *dev)
{
	
	struct dsi_data * data = dev_get_drvdata(dev);
	dsihw_dump_regs(data->pdev);

}

void test_fs_dsi(struct device *dev)
{
	struct platform_device *pdev = dsi_pdev_map[0] ;
	dsihw_single_enable(pdev,false);	
	dsihw_display_init_dsi();
}

void test_fs_longcmd(struct device *dev)
{
	struct dsi_data * data = dev_get_drvdata(dev);
	send_cmd_test(data->pdev);
}

