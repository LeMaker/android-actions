/*
 * linux/drivers/video/owl/dss/edp.c
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

#define DSS_SUBSYS_NAME "EDP"

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/slab.h>

#include "../owldss.h"
#include "../dss/dss_features.h"
#include "../dss/dss.h"
#include "edphw.h"

struct edp_data {
	
	struct platform_device * pdev;
	
	struct regulator *edp_power;
	
	bool edp_power_enabled;
	
	struct mutex lock;
	
	struct owl_dss_device *dssdev;
	
	struct clk *edp_clk;
	
	int (*enable_pads)(int edp_id, unsigned lane_mask);
	
	void (*disable_pads)(int edp_id, unsigned lane_mask);
		
	void __iomem	*base;
	
};

static struct platform_device *edp_pdev_map[MAX_NUM_LCD];

inline struct edp_data *edphw_get_edpdrv_data(struct platform_device *edpdev)
{
	return dev_get_drvdata(&edpdev->dev);
}

inline void edphw_write_reg(struct platform_device *edpdev,const u16 index, u32 val)
{
	struct edp_data *edp = edphw_get_edpdrv_data(edpdev);
	DSSDBG("edphw_write_reg  ~~~ %p index %d\n",edp->base,index);
	writel(val, edp->base + index);
}

inline u32 edphw_read_reg(struct platform_device *edpdev,const u16 index)
{
	struct edp_data *edp = edphw_get_edpdrv_data(edpdev);
	DSSDBG("edphw_read_reg  ~~~ %p index %d\n",edp->base,index);
	return readl(edp->base + index);
}

void edphw_dump_regs(struct platform_device *edpdev){
	
#define DUMPREG(r) DSSDBG("%08x ~~ %08x\n",r,edphw_read_reg(edpdev,r))
     DUMPREG(EDP_LNK_LANE_COUNT);
     DUMPREG(EDP_LNK_ENHANCED);
     DUMPREG(EDP_LNK_TRAP);
     DUMPREG(EDP_LNK_QUAL_PAT);
     DUMPREG(EDP_LNK_SCR_CTRL);
     DUMPREG(EDP_LNK_DSPR_CTRL);
     DUMPREG(EDP_LNK_SCR_RST);
     DUMPREG(EDP_LNK_PANEL_SRF);
     
     DUMPREG(EDP_CORE_TX_EN);
     DUMPREG(EDP_CORE_MSTREAM_EN);
     DUMPREG(EDP_CORE_SSTREAM_EN);
     DUMPREG(EDP_CORE_FSCR_RST);     
     DUMPREG(EDP_CORE_USER_CFG);
     DUMPREG(EDP_CORE_CAPS);
     DUMPREG(EDP_CORE_ID);
     
     DUMPREG(EDP_AUX_COMD);
     DUMPREG(EDP_AUX_WR_FIFO);
     DUMPREG(EDP_AUX_ADDR);
     DUMPREG(EDP_AUX_CLK_DIV);
     DUMPREG(EDP_AUX_STATE);
     DUMPREG(EDP_AUX_RPLY_DAT);
     DUMPREG(EDP_AUX_RPLY_CODE);
     DUMPREG(EDP_AUX_RPLY_COUNT);
     DUMPREG(EDP_AUX_INT_STAT);         
     DUMPREG(EDP_AUX_INT_MASK);
     DUMPREG(EDP_AUX_RPLY_DAT_CNT);
     DUMPREG(EDP_AUX_STATUS);
     DUMPREG(EDP_AUX_RCLK_WIDTH);
     
     DUMPREG(EDP_MSTREAM_HTOTAL);
     DUMPREG(EDP_MSTREAM_VTOTAL);
     DUMPREG(EDP_MSTREAM_POLARITY);
     DUMPREG(EDP_MSTREAM_HSWIDTH);
     DUMPREG(EDP_MSTREAM_VSWIDTH);
     DUMPREG(EDP_MSTREAM_HRES);
     DUMPREG(EDP_MSTREAM_VRES);
     DUMPREG(EDP_MSTREAM_HSTART);     
     DUMPREG(EDP_MSTREAM_VSTART);
     DUMPREG(EDP_MSTREAM_MISC0);
     DUMPREG(EDP_MSTREAM_MISC1);
     DUMPREG(EDP_M_VID);
     DUMPREG(EDP_N_VID);
     DUMPREG(EDP_USER_PIXEL_WIDTH);
     DUMPREG(EDP_USER_DATA_COUNT);
     DUMPREG(EDP_MSTREAM_INTERLACED);
     DUMPREG(EDP_USER_SYNC_POLARITY);
     
     DUMPREG(EDP_PHY_RESET);
     DUMPREG(EDP_PHY_PREEM_L0);
     DUMPREG(EDP_PHY_PREEM_L1);
     DUMPREG(EDP_PHY_PREEM_L2);
     DUMPREG(EDP_PHY_PREEM_L3);
     DUMPREG(EDP_PHY_VSW_L0);
     DUMPREG(EDP_PHY_VSW_L1);
     DUMPREG(EDP_PHY_VSW_L2);
     DUMPREG(EDP_PHY_VSW_L3);
     DUMPREG(EDP_PHY_VSW_AUX);
     DUMPREG(EDP_PHY_PWR_DOWN);     
     DUMPREG(EDP_PHY_CAL_CONFIG);
     DUMPREG(EDP_PHY_CAL_CTRL);
     DUMPREG(EDP_PHY_CTRL);

     DUMPREG(EDP_SDB_LANE_SELECT);
     DUMPREG(EDP_SDB_WRITE_INDEX);     
     DUMPREG(EDP_SDB_DATA_COUNT);
     DUMPREG(EDP_SDB_DATA);
     DUMPREG(EDP_SDB_READY);     
     DUMPREG(EDP_SDB_BUSY);
     
     DUMPREG(EDP_RGB_CTL);     
     DUMPREG(EDP_RGB_STATUS);
     DUMPREG(EDP_RGB_COLOR);
     DUMPREG(EDP_DEBUG);

}

static int edphw_enable_pads(int dsi_id, unsigned lane_mask)
{

	return 0;
}

static void edphw_disable_pads(int dsi_id, unsigned lane_mask)
{

}

static int edphw_set_edp_clk(struct platform_device *edpdev, bool is_tft,unsigned long pck_req, unsigned long *fck, int *lck_div,
		int *pck_div)
{
	return 0;
}

static void edphw_aux_write(struct platform_device *edpdev, u16 addr,u16 data)
{
	u32 temp;
	
	edphw_write_reg(edpdev,EDP_AUX_ADDR,addr);
	
	edphw_write_reg(edpdev,EDP_AUX_WR_FIFO,data);	
	
	edphw_write_reg(edpdev,EDP_AUX_COMD,0x08<<8); //aux write, 1byte
	
	do{
		temp = edphw_read_reg(edpdev,EDP_AUX_STATE);	
		
	}while(temp&(1<<1));		//if 1, request is in progress
	
	temp =edphw_read_reg(edpdev,EDP_AUX_RPLY_CODE);	
}	

static u16 edphw_aux_read(struct platform_device *edpdev,u16 addr)
{
	u32 temp;
	
	edphw_write_reg(edpdev,EDP_AUX_ADDR,addr);
	
	edphw_write_reg(edpdev,EDP_AUX_COMD,0x09<<8); //aux read, 1byte
	
	do{
		
		temp = edphw_read_reg(edpdev,EDP_AUX_STATE);	
		
	}while(!(temp&(1<<2)));		//if 1, reply is in progress
	
	temp = edphw_read_reg(edpdev,EDP_AUX_RPLY_CODE);	
	temp = edphw_read_reg(edpdev,EDP_AUX_RPLY_COUNT);	
	temp = edphw_read_reg(edpdev,EDP_AUX_RPLY_DAT_CNT);	
	temp = edphw_read_reg(edpdev,EDP_AUX_RPLY_DAT);	
	
	return(temp);
}


static void edphw_set_size(struct platform_device *edpdev, u16 width, u16 height)
{
	u32 val;
	
	BUG_ON((width > (1 << 14)) || (height > (1 << 14)));
	
	val = REG_VAL(width, 14, 0);
	
	edphw_write_reg(edpdev,EDP_MSTREAM_HRES, val);
	
	val = REG_VAL(height, 14, 0);
	
	edphw_write_reg(edpdev,EDP_MSTREAM_VRES, val);
}

static void edphw_set_timings(struct platform_device *edpdev,u16 width, u16 height, u16 hbp ,u16 hfp, u16 hsw, u16 vbp ,u16 vfp, u16 vsw)
{	

	BUG_ON((hbp > (1 << 15)) || (hfp > (1 << 15)) || (hsw > (1 << 15)));	
	
	BUG_ON((vbp > (1 << 15)) || (vfp > (1 << 15)) || (vsw > (1 << 15)));
	   
    edphw_write_reg(edpdev,EDP_MSTREAM_HSWIDTH, hsw);    
    edphw_write_reg(edpdev,EDP_MSTREAM_HSTART, hbp);
    edphw_write_reg(edpdev,EDP_MSTREAM_HTOTAL, hsw + hbp + width + hfp);
    	
	edphw_write_reg(edpdev,EDP_MSTREAM_VSWIDTH, vsw);    
    edphw_write_reg(edpdev,EDP_MSTREAM_VSTART, vbp);
    edphw_write_reg(edpdev,EDP_MSTREAM_VTOTAL, vsw + vbp + height + vfp);    
	
}

static void edphw_phy_config(struct platform_device *edpdev)
{	
	   
    edphw_write_reg(edpdev,EDP_PHY_PREEM_L0, 0);    
    edphw_write_reg(edpdev,EDP_PHY_PREEM_L1, 0);
    edphw_write_reg(edpdev,EDP_PHY_PREEM_L2, 0);
    edphw_write_reg(edpdev,EDP_PHY_PREEM_L3, 0);
    
    edphw_write_reg(edpdev,EDP_PHY_VSW_L0, 0);    
    edphw_write_reg(edpdev,EDP_PHY_VSW_L1, 0);
    edphw_write_reg(edpdev,EDP_PHY_VSW_L2, 0);
    edphw_write_reg(edpdev,EDP_PHY_VSW_L3, 0);
    	
	edphw_write_reg(edpdev,EDP_PHY_VSW_AUX, 0);//set the aux channel voltage swing 400mV	
	
	edphw_write_reg(edpdev,EDP_PHY_CTRL, 0); //set no Mirror,polarity not changed
	
}
static void edphw_link_config(struct platform_device *edpdev)
{	
	edphw_write_reg(edpdev,EDP_AUX_CLK_DIV,24);	//set aux clock 1MHz;
	edphw_write_reg(edpdev,EDP_LNK_LANE_COUNT,0x01);	//set 1 lane	
	edphw_write_reg(edpdev,EDP_LNK_SCR_RST,0x00);		//eDP enable, use only for embedded application		
	edphw_write_reg(edpdev,EDP_LNK_SCR_CTRL,0x00);	//enable internal scramble	
}
static void edphw_link_training(struct platform_device *edpdev)
{	
	u32 temp ;   
    edphw_aux_write(edpdev,0x100,0x06);	//1.62G
	mdelay(50);
	temp=edphw_aux_read(edpdev,0x100);
	mdelay(50);
	edphw_aux_write(edpdev,0x101,0x01);	//1 lane
	mdelay(50);
	temp=edphw_aux_read(edpdev,0x101);
	mdelay(50);
	
	edphw_aux_write(edpdev,0x102,0x01 | (1<<5));		//set Sink training pattern, disable scramble
	mdelay(50);
	temp=edphw_aux_read(edpdev,0x102);
		
	edphw_aux_write(edpdev,0x103,0x00);	//lane0 level 3,1.0V
	mdelay(50);
	temp=edphw_aux_read(edpdev,0x103);

   //source config	
	//edphw_aux_write(edpdev,CMU_EDPCLK,0x70300|0);	//0:1.62G; 1:2.7G ; 4:5.4G
	edphw_aux_write(edpdev,EDP_LNK_TRAP,1);					
	edphw_aux_write(edpdev,EDP_LNK_SCR_CTRL,0x01);	//disable internal scramble
	mdelay(50);

	temp=edphw_aux_read(edpdev,0x202);

   //pattern2	
	edphw_aux_write(edpdev,0x102,2|(1<<5));		//set Sink training pattern, disable scramble
	mdelay(50);
	temp=edphw_aux_read(edpdev,0x102);
    edphw_write_reg(edpdev,EDP_LNK_TRAP,2);				//pattern2	

	mdelay(50);

	temp=edphw_aux_read(edpdev,0x202);

	edphw_aux_write(edpdev,0x102,0x00);		//no training pattern now,enable scramble
	mdelay(50);
	temp=edphw_aux_read(edpdev,0x102);
	
	edphw_write_reg(edpdev,EDP_LNK_TRAP,0x00);	//training off
	edphw_write_reg(edpdev,EDP_LNK_SCR_CTRL,0x00);	//enable internal scramble
	
}

static void edphw_tx_init(struct platform_device *edpdev)
{	
	edphw_write_reg(edpdev,EDP_CORE_TX_EN,0x00);		//disable trasmitter output
	edphw_write_reg(edpdev,EDP_PHY_PWR_DOWN,0x00);	//power enable,pull the PHY out of power down		
	edphw_write_reg(edpdev,EDP_PHY_RESET,0x00);	//pull the PHY out of reset
	mdelay(1);			//wait for mdelay complete
	edphw_write_reg(edpdev,EDP_CORE_TX_EN,0x01);	//enab	
}

static void edphw_set_lnk_lane_cnt(struct platform_device *edpdev,u8 lane)
{	

	BUG_ON((lane > (1 << 4)));
	
    edphw_write_reg(edpdev,EDP_LNK_LANE_COUNT, lane);      
	
}

static void edphw_set_lnk_enhanced(struct platform_device *edpdev,bool enable)
{	
    edphw_write_reg(edpdev,EDP_LNK_ENHANCED, enable);     
	
}

static void edphw_set_default_color(struct platform_device *edpdev, u32 color)
{	
	edphw_write_reg(edpdev,EDP_RGB_COLOR, color);
}


static void edphw_set_single_fromat(struct platform_device *edpdev,u8 format)
{	
	u32 val;
		
	val = edphw_read_reg(edpdev,EDP_MSTREAM_MISC0);
	
	val = REG_SET_VAL(val, format, 2, 1);
		
	edphw_write_reg(edpdev,EDP_MSTREAM_MISC0, val);

}

static void edphw_set_date_width(struct platform_device *edpdev,u8 bit_per_pix)
{	
	u32 val;
		
	val = edphw_read_reg(edpdev,EDP_MSTREAM_MISC0);
	
	val = REG_SET_VAL(val, bit_per_pix, 7, 5);
		
	edphw_write_reg(edpdev,EDP_MSTREAM_MISC0, val);	

}
static void edphw_set_yuv_bt(struct platform_device *edpdev,u8 bt_yuv)
{	
	u32 val;
		
	val = edphw_read_reg(edpdev,EDP_MSTREAM_MISC0);
	
	val = REG_SET_VAL(val, bt_yuv, 4, 4);
		
	edphw_write_reg(edpdev,EDP_MSTREAM_MISC0, val);	

}
static void edphw_set_micro_packet_size(struct platform_device *edpdev,u8 size)
{	
	u32 val;
	
	BUG_ON(size > (1 << 6));	
	
	val = edphw_read_reg(edpdev,EDP_MTRANSFER_UNIT);
	
	val = REG_SET_VAL(val, size, 6, 0);
		
	edphw_write_reg(edpdev,EDP_MTRANSFER_UNIT, val);	

}


static void edphw_set_rb_swap(struct platform_device *edpdev, bool rb_swap)
{	
	u32 val;
		
	val = edphw_read_reg(edpdev,EDP_RGB_CTL);
	
	val = REG_SET_VAL(val, rb_swap, 1, 1);
		
	edphw_write_reg(edpdev,EDP_RGB_CTL, val);
}

static void edphw_set_single_from(struct platform_device *edpdev, u8 single)
{	
	u32 val;
		
	val = edphw_read_reg(edpdev,EDP_RGB_CTL);
	
	val = REG_SET_VAL(val, single, 1, 1);
		
	edphw_write_reg(edpdev,EDP_RGB_CTL, val);
}

static void edphw_single_enable(struct platform_device *edpdev, bool enable)
{	
	u32 val;
	//start calibrate	
	val = edphw_read_reg(edpdev,EDP_PHY_CAL_CTRL);
	
	val = REG_SET_VAL(val, enable, 8, 8);
		
	edphw_write_reg(edpdev,EDP_PHY_CAL_CTRL, val);
	
	//Enable the main stream
	val = edphw_read_reg(edpdev,EDP_CORE_MSTREAM_EN);
	
	val = REG_SET_VAL(val, enable, 0, 0);
		
	edphw_write_reg(edpdev,EDP_CORE_MSTREAM_EN, val);
	
	//enable RGB interface	
	val = edphw_read_reg(edpdev,EDP_RGB_CTL);
	
	val = REG_SET_VAL(val, enable, 0, 0);
		
	edphw_write_reg(edpdev,EDP_RGB_CTL, val);	
}

static void edphw_display_init_edp(struct owl_dss_device *dssdev)
{
	struct platform_device * edpdev = edp_pdev_map[0];
	struct owl_video_timings * timings = &(dssdev->timings);
	
	BUG_ON(!timings);
	
	edphw_set_size(edpdev,timings->x_res,timings->y_res);

	edphw_set_timings(edpdev,timings->x_res,timings->y_res,timings->hbp ,timings->hfp, timings->hsw, timings->vbp ,timings->vfp, timings->vsw);

	edphw_set_default_color(edpdev,0);
	
	edphw_set_single_fromat(edpdev,0);
	
	edphw_set_rb_swap(edpdev,0);
	
	edphw_set_single_from(edpdev,0x02);
	
	edphw_phy_config(edpdev);
	
	edphw_tx_init(edpdev);
	
	edphw_link_config(edpdev);

}

static int edphw_get_clocks(struct platform_device *edpdev)
{
	struct edp_data *edp = edphw_get_edpdrv_data(edpdev);
	struct clk *clk;
#if 0
	clk = clk_get(&edpdev->dev, "lcd");
	if (IS_ERR(clk)) {
		DSSERR("can't get fck\n");
		return PTR_ERR(clk);
	}

	edp->edp_clk = clk;
#endif 
	return 0;
}
static void edphw_put_clocks(struct platform_device *edpdev)
{
	struct edp_data *edp = edphw_get_edpdrv_data(edpdev);

	if (edp->edp_clk)
		clk_put(edp->edp_clk);

}
int owl_edp_display_enable(struct owl_dss_device *dssdev)
{
	int r;

    struct platform_device *edpdev = edp_pdev_map[0] ;
	struct edp_data *edp = edphw_get_edpdrv_data(edpdev);
	
	mutex_lock(&edp->lock);
	
	if (dssdev->manager == NULL) {
		DSSERR("failed to enable display: no manager\n");
		return -ENODEV;
	}

	r = owl_dss_start_device(dssdev);
	
	if (r) {
		DSSERR("failed to start device\n");
		goto err_start_dev;
	}
	
	if(edp->edp_power){
		r = regulator_enable(edp->edp_power);
		if (r)
			goto err_reg_enable;
	}

	edphw_display_init_edp(dssdev);
	
	
	r = dss_mgr_enable(dssdev->manager);
	
	if (r)
		goto err_mgr_enable;
		
	edphw_single_enable(edpdev,true);	
	
	mutex_unlock(&edp->lock);
	edphw_dump_regs(edpdev);
	return 0;
	
err_mgr_enable:
err_reg_enable:
err_get_edp:	
err_start_dev:
	mutex_unlock(&edp->lock);
	return r;
}

void owl_edp_display_disable(struct owl_dss_device *dssdev)
{
	int r;
	struct platform_device *edpdev = edp_pdev_map[0] ;
	struct edp_data *edp = edphw_get_edpdrv_data(edpdev);
	
	edphw_single_enable(edpdev,false);	
	
	dss_mgr_disable(dssdev->manager);

	if(edp->edp_power){
		r = regulator_disable(edp->edp_power);
		if (r){
		}
	}

	owl_dss_stop_device(dssdev);
}

/* EDP HW IP initialisation */
static int owl_edphw_probe(struct platform_device *edpdev)
{
	int r;
	struct resource *edp_mem;
	struct edp_data *edp;
	DSSDBG("owl_edphw_probe called \n");
	edp = kzalloc(sizeof(*edp), GFP_KERNEL);
	if (!edp) {
		r = -ENOMEM;
		goto err_alloc;
	}

	edp->pdev = edpdev;
	edp_pdev_map[0] = edpdev;
	dev_set_drvdata(&edpdev->dev, edp);

	edp->enable_pads = edphw_enable_pads;
	edp->disable_pads = edphw_disable_pads;

	mutex_init(&edp->lock);

	r = edphw_get_clocks(edpdev);
	if (r)
		goto err_get_clk;

	pm_runtime_enable(&edpdev->dev);

	edp_mem = platform_get_resource(edp->pdev, IORESOURCE_MEM, 0);
	
	if (!edp_mem) {
		DSSERR("can't get IORESOURCE_MEM DSI\n");
		r = -EINVAL;
		goto err_ioremap;
	}	
	
	edp->base = ioremap(edp_mem->start, resource_size(edp_mem));
	if (!edp->base) {
		DSSERR("can't ioremap edp \n");
		r = -ENOMEM;
		goto err_ioremap;
	}
	DSSDBG("edp->base  ~~~ %p \n",edp->base);	
		
	//dsi_calc_clock_param_ranges(edpdev);
	DSSDBG("owl_edphw_probe called  ok ~~~~~~~~~~~~~\n");
	return 0;

err_get_dsi:
	iounmap(edp->base);
err_ioremap:
	pm_runtime_disable(&edpdev->dev);
err_get_clk:
	kfree(edp);
err_alloc:
	return r;
}

static int owl_edphw_remove(struct platform_device *edpdev)
{
	struct edp_data *edp = edphw_get_edpdrv_data(edpdev);

	pm_runtime_disable(&edpdev->dev);

	edphw_put_clocks(edpdev);

	if (edp->edp_power != NULL) {
		if (edp->edp_power_enabled) {
			regulator_disable(edp->edp_power);
			edp->edp_power_enabled = false;
		}
		regulator_put(edp->edp_power);
		edp->edp_power = NULL;
	}

	iounmap(edp->base);

	kfree(edp);

	return 0;
}

static struct resource asoc_edphw_resources[] = {
	{
		.start = EDPHW_REG_MEM_BASE,
		.end = EDPHW_REG_MEM_END,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device asoc_edphw_devices = {
    .name           = "owl_edphw",
    .num_resources  = ARRAY_SIZE(asoc_edphw_resources),
    .resource       = asoc_edphw_resources
};


static struct platform_driver asoc_edphw_driver = {
	.probe          = owl_edphw_probe,
	.remove         = owl_edphw_remove,
	.driver         = {
		.name   = "owl_edphw",
		.owner  = THIS_MODULE,
	},
};

int owl_edp_init_platform(void)
{
	int r;
	
	r = platform_device_register(&asoc_edphw_devices);
	if (r) {
		DSSERR("Failed to initialize edp platform devices\n");
		goto err_device;
	}
	
	r = platform_driver_register(&asoc_edphw_driver);
	
	if (r) {
		DSSERR("Failed to initialize edp platform driver\n");
		goto err_driver;
	}
	return 0;
err_driver:
	platform_device_unregister(&asoc_edphw_devices);
err_device:
	return r;
}

int owl_edp_uninit_platform(void)
{   
	platform_device_unregister(&asoc_edphw_devices);
	
	platform_driver_unregister(&asoc_edphw_driver);
	
    return 0;
}
