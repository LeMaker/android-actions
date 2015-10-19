/*
 * gl520x_mmc.c - GL5203 SD/MMC driver
 *
 * Copyright (C) 2012, Actions Semiconductor Co. LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/cpufreq.h>
#include <linux/genhd.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/clk.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/hdmac-owl.h>
#include <mach/bootdev.h>

#include <linux/mfd/atc260x/atc260x.h>
#include "gl520x_mmc.h"
#include "wlan_plat_data.h"
#include "wlan_device.h"

static int act_check_trs_date_status(
		struct gl520xmmc_host *host , struct mmc_request *mrq);
static void act_dump_reg(struct gl520xmmc_host *host);

#undef pr_debug
/* #define ACTS_MMC_DEBUG */

#ifdef ACTS_MMC_DEBUG
#define pr_debug(format, arg...)	\
	printk(KERN_INFO format, ##arg)
#else
#define pr_debug(format, args...)
#endif
//#define DEBUG_EN 1
#ifdef DEBUG_EN 
#define DEBUG(format, arg...)	\
	printk(KERN_INFO format, ##arg)
#else
#define DEBUG(format, args...)\
	do{}while(0);
#endif

/*
 * SD Controller Linked Card type, one of those:
 * MMC_CARD_DISABLE | MMC_CARD_MEMORY | MMC_CARD_WIFI
 */
static const char * const card_types[] = {
	[MMC_CARD_DISABLE]	= "none",
	[MMC_CARD_MEMORY]	= "memory",
	[MMC_CARD_EMMC]		= "emmc",
	[MMC_CARD_WIFI]		= "wifi",
};


/*
 * Method to detect card Insert/Extract:
 */
static const char * const card_detect_modes[] = {
	[SIRQ_DETECT_CARD]		= "sirq",
	[GPIO_DETECT_CARD]		= "gpio",
	[COMMAND_DETECT_CARD]		= "command",
};


static int detect_use_sirq = 0 ;
static int detect_use_gpio = 0 ;

extern int acts_wlan_set_power(struct wlan_plat_data *pdata, int on,unsigned long msec);
extern struct wlan_plat_data *acts_get_wlan_plat_data(void);


static void acts_dump_mfp(struct gl520xmmc_host *host)
{
	void __iomem *reg_mfp;
	void __iomem *reg_cmu;
	
	reg_mfp = ioremap(GPIO_MFP_PWM_BASE,INTC_GPIOE_TYPE-GPIO_MFP_PWM_BASE+4);
	reg_cmu = ioremap(CMU_BASE,CMU_AUDIOPLL_ETHPLLDEBUG-CMU_BASE+4);
	if(!(((u32)reg_cmu)&&((u32)(reg_mfp)))){
		printk("ERR:%s:ioreamp:reg_cmu=0x%08x,reg_mfp=0x%08x\n",\
			__FUNCTION__,(u32)reg_cmu,(u32)(reg_mfp));
		return;
	}
		
	pr_err("\tGPIO_CINEN:0x%x\n", readl(DUMP_GPIO_CINEN(reg_mfp)));
	pr_err("\tGPIO_COUTEN:0x%x\n", readl(DUMP_GPIO_COUTEN(reg_mfp)));

	pr_err("\tMFP_CTL0:0x%x\n", readl(DUMP_MFP_CTL0(reg_mfp)));
	pr_err("\tMFP_CTL1:0x%x\n", readl(DUMP_MFP_CTL1(reg_mfp)));
	pr_err("\tMFP_CTL2:0x%x\n", readl(DUMP_MFP_CTL2(reg_mfp)));
	pr_err("\tMFP_CTL3:0x%x\n", readl(DUMP_MFP_CTL3(reg_mfp)));
	pr_err("\tPAD_PULLCTL0:0x%x\n", readl(DUMP_PAD_PULLCTL0(reg_mfp)));
	pr_err("\tPAD_PULLCTL1:0x%x\n", readl(DUMP_PAD_PULLCTL1(reg_mfp)));
	pr_err("\tPAD_PULLCTL2:0x%x\n", readl(DUMP_PAD_PULLCTL2(reg_mfp)));

	pr_err("\tPAD_DRV0:0x%x\n", readl(DUMP_PAD_DVR0(reg_mfp)));
	pr_err("\tPAD_DRV1:0x%x\n", readl(DUMP_PAD_DVR1(reg_mfp)));
	pr_err("\tPAD_DRV2:0x%x\n", readl(DUMP_PAD_DVR2(reg_mfp)));

	pr_err("\tCMU_DEVCLKEN0:0x%x\n", readl(DUMP_CMU_DEVCLKEN0(reg_cmu)));
	pr_err("\tCMU_DEVCLKEN1:0x%x\n", readl(DUMP_CMU_DEVCLKEN1(reg_cmu)));

	pr_err("\tCMU_DEVPLL:0x%x\n", readl(DUMP_CMU_DEVPLL(reg_cmu)));
	pr_err("\tCMU_NANDPLL:0x%x\n", readl(DUMP_CMU_NANDPLL(reg_cmu)));

	pr_err("\tCMU_SD0CLK:0x%x\n", readl(DUMP_CMU_CMU_SD0CLK(reg_cmu)));
	pr_err("\tCMU_SD1CLK:0x%x\n", readl(DUMP_CMU_CMU_SD1CLK(reg_cmu)));
	pr_err("\tCMU_SD2CLK:0x%x\n", readl(DUMP_CMU_CMU_SD2CLK(reg_cmu)));
	if(reg_mfp)
		iounmap(reg_mfp);
	if(reg_cmu)
		iounmap(reg_cmu);
}

								



static void acts_dump_sdc(struct gl520xmmc_host *host)
{
	pr_err("\n\tSD_EN:0x%x\n", readl(HOST_EN(host)));
	pr_err("\tSD_CTL:0x%x\n", readl(HOST_CTL(host)));
	pr_err("\tSD_STATE:0x%x\n", readl(HOST_STATE(host)));
	pr_err("\tSD_CMD:0x%x\n", readl(HOST_CMD(host)));
	pr_err("\tSD_ARG:0x%x\n", readl(HOST_ARG(host)));
	pr_err("\tSD_RSPBUF0:0x%x\n", readl(HOST_RSPBUF0(host)));
	pr_err("\tSD_RSPBUF1:0x%x\n", readl(HOST_RSPBUF1(host)));
	pr_err("\tSD_RSPBUF2:0x%x\n", readl(HOST_RSPBUF2(host)));
	pr_err("\tSD_RSPBUF3:0x%x\n", readl(HOST_RSPBUF3(host)));
	pr_err("\tSD_RSPBUF4:0x%x\n", readl(HOST_RSPBUF4(host)));
	pr_err("\tSD_DAT:0x%x\n", readl(HOST_DAT(host)));
	pr_err("\tSD_BLK_SIZE:0x%x\n\n", readl(HOST_BLK_SIZE(host)));
	pr_err("\tSD_BLK_NUM:0x%x\n", readl(HOST_BLK_NUM(host)));
	pr_err("\tSD_BUF_SIZE:0x%x\n", readl(HOST_BUF_SIZE(host)));
}

static void acts_dump_dmac(struct gl520xmmc_host *host)
{
	owl_dma_dump_all(host->dma);
}

static inline int acts_enable_clock(struct gl520xmmc_host *host)
{
	int ret;

	if (!host->clk_on) {
		ret = module_clk_enable(host->module_id);
		if (ret) {
			pr_err("SDC[%d] enable module clock error\n",
				host->id);
			return ret;
		}
		host->clk_on = 1;
	}
	return 0;
}

static inline void acts_disable_clock(struct gl520xmmc_host *host)
{
	if (host->clk_on) {
		module_clk_disable(host->module_id);
		host->clk_on = 0;
	}
}

static int acts_mmc_send_init_clk(struct gl520xmmc_host *host)
{
	u32 mode;
	int ret = 0;

	init_completion(&host->sdc_complete);
	mode = SD_CTL_TS  | SD_CTL_TCN(5) | SD_CTL_TM(8);
	mode |= (readl(HOST_CTL(host)) & (0xff << 16));

	writel(mode, HOST_CTL(host));
	DEBUG("host%d: send acts_mmc_send_init_clk  \n",host->id );
	
	if (!wait_for_completion_timeout(&host->sdc_complete,  HZ)) {
		pr_err("*SDC%d send 80 init clock timeout error\n", host->id);	
		act_dump_reg(host);
		ret = -1;
	}
	DEBUG("host%d: acts_mmc_send_init_clk  OK\n",host->id);
	return ret;
}

static void acts_mmc_set_clk(struct gl520xmmc_host *host, int rate)
{

	
	if (0 == rate) {
		pr_err("SDC%d set clock error\n", host->id);
		return;
		
	}

	/*
	 * Set the RDELAY and WDELAY based on the sd clk.
	 */
	if (rate <= 1000000) {
		
		writel((readl(HOST_CTL(host))  & (~(0xff << 16))) |
			SD_CTL_RDELAY(host->rdelay.delay_lowclk) |
			SD_CTL_WDELAY(host->wdelay.delay_lowclk),
			HOST_CTL(host));

	} else if ((rate > 1000000) && (rate <= 26000000)) {
		writel((readl(HOST_CTL(host))  & (~(0xff << 16))) |
			SD_CTL_RDELAY(host->rdelay.delay_midclk) |
			SD_CTL_WDELAY(host->wdelay.delay_midclk),
			HOST_CTL(host));

	} else if ((rate > 26000000) && (rate <= 52000000)) {

			writel((readl(HOST_CTL(host))  & (~(0xff << 16))) |
			SD_CTL_RDELAY(host->rdelay.delay_highclk) |
			SD_CTL_WDELAY(host->wdelay.delay_highclk),
			HOST_CTL(host));

	} else if ((rate > 52000000) && (rate <= 100000000)) {
		
		writel((readl(HOST_CTL(host))  & (~(0xff << 16))) |
			SD_CTL_RDELAY(6) |
			SD_CTL_WDELAY(4),
			HOST_CTL(host));

	} else {
		pr_err("SD3.0 max clock should not > 100Mhz\n");

	}

	host->read_delay_chain   = (readl(HOST_CTL(host)) & (0xf << 20))>>20;
	host->write_delay_chain  = (readl(HOST_CTL(host)) & (0xf << 16))>>16;	
	host->write_delay_chain_bak = host->write_delay_chain;
	host->read_delay_chain_bak  =  host->read_delay_chain;
	module_clk_set_rate(host, rate);

}

static void act_mmc_opt_regulator(struct gl520xmmc_host *host,bool enable)
{
	int ret = 0;
	if (!(IS_ERR(host->reg)|| NULL==host->reg)) {
		if(enable){
			ret = regulator_enable(host->reg);
			if(ret){
				printk("host%d regulator_enable fail\n",host->id);
			}
		}else{
			ret = regulator_disable(host->reg);
			if(ret){
				printk("host%d regulator_disable fail\n",host->id);
			}			
		}
		
	}
}

static void acts_mmc_power_up(struct gl520xmmc_host *host)
{
	/* enable gl5302 power for card */


	/* power on reset */
	module_reset(host->module_id);
	acts_enable_clock(host);

	writel(SD_ENABLE | SD_EN_RESE, HOST_EN(host));
}

static int owl_switch_uart_pinctr(struct mmc_host *mmc)
{

	int ret = 0;
	struct gl520xmmc_host *host;
	host = mmc_priv(mmc);

	if(mmc_card_expected_mem(host->type_expected)&&\
	(host->sdio_uart_supported)){
	
		if(host->switch_pin_flag == SD_PIN){
			// free sd pin
			if (!IS_ERR(host->pcl)){
				pinctrl_put(host->pcl);
			}
			// requeset uart pin
			host->pcl = pinctrl_get_select(mmc->parent, PINCTRL_UART_PIN);
			if (IS_ERR(host->pcl)){
				pr_err("SDC%d get misc uart pinctrl failed, %ld\n",
				host->id, PTR_ERR(host->pcl));
				ret = (int)PTR_ERR(host->pcl);
				host->switch_pin_flag  = ERR_PIN ;
				goto out;
			}
		}else if(host->switch_pin_flag  == UART_PIN){
				goto out;
		}else{
				printk("err:owl_switch_uart_pinctr\n");
				host->switch_pin_flag  = ERR_PIN ;
				ret = -1 ;
		}
		
		host->switch_pin_flag  = UART_PIN;
	
	}
	
out:
	return ret;
}

static int owl_switch_sd_pinctr(struct mmc_host *mmc)
{

	int ret = 0;
	struct gl520xmmc_host *host;
	host = mmc_priv(mmc);

	if(mmc_card_expected_mem(host->type_expected)&&\
	(host->sdio_uart_supported)){
		if(host->switch_pin_flag  == UART_PIN){
			// free uart pin
			if (!IS_ERR(host->pcl)){
				pinctrl_put(host->pcl);
			}
			// requeset sd pin
			host->pcl = pinctrl_get_select_default(mmc->parent);
			if (IS_ERR(host->pcl)){
				pr_err("SDC%d get sd pinctrl failed, %ld\n",
				host->id, PTR_ERR(host->pcl));
				ret = (int)PTR_ERR(host->pcl);
				host->switch_pin_flag  = ERR_PIN ;
				goto out;
			}
		}else if(host->switch_pin_flag  == SD_PIN){
				goto out;
		}else{
				printk("err:owl_switch_sd_pinctr\n");
				host->switch_pin_flag  = ERR_PIN ;
				ret = -1 ;
		}
		
		host->switch_pin_flag  = SD_PIN;
	
	}
	
out:
	return ret;
}
static int acts_mmc_request_pinctrl(struct gl520xmmc_host *host)
{

	static int req_pinctr_num = 0;  
	// host0 usd for sdcard need not request,it will request when resan sdcard
	if(mmc_card_expected_mem(host->type_expected)&&\
		(host->sdio_uart_supported))
		return 0;

	if (host->sdio_uart_supported) 
		 sdio_uart_pinctrl_free();

	host->pcl = pinctrl_get_select_default(host->mmc->parent);
	
	if (IS_ERR(host->pcl)) {
		pr_err("SDC%d get default pinctrl failed, %ld\n",
			host->id, PTR_ERR(host->pcl));
		return (int)PTR_ERR(host->pcl);
	}
	
	/* first time request pinctr in platrom ,so sd2
	need not request again for the first time
	*/			
	if( (SDC2_SLOT == host->id)&&( !req_pinctr_num ) ){

		if (IS_ERR(host->pcl) ||(NULL==host->pcl)) {
			printk("SDC%d get default pinctrl failed, %ld\n",
			host->id, PTR_ERR(host->pcl));
			
			return (int)PTR_ERR(host->pcl);
		}
		pinctrl_put(host->pcl);
		req_pinctr_num = 1;
		printk("host%d,platfrom request already\n",host->id);
	}
	
	return 0;
}

static int acts_mmc_free_pinctrl(struct gl520xmmc_host *host)
{
	
	int ret;
	// host0 usd for sdcard need not free,it will free when resan fail
	if(mmc_card_expected_mem(host->type_expected)&&\
		(host->sdio_uart_supported))
		return 0;

	if (host->pcl == NULL) {
		pr_err("SDC%d pinctrl has not initialed\n", host->id);
		return 0;
	}

	if (!IS_ERR(host->pcl)) {
			pinctrl_put(host->pcl);
	}
	
	if (host->sdio_uart_supported) {
		ret = sdio_uart_pinctrl_request();
		if (ret < 0) {
			pr_err("SDC%d uart pinctrl request failed\n",
				host->id);
			return ret;
		}
	}

	return 0;
}

static void acts_mmc_power_on(struct gl520xmmc_host *host)
{


	mutex_lock(&host->pin_mutex);
	if (acts_mmc_request_pinctrl(host) < 0)
		pr_err("SDC%d request pinctrl failed\n", host->id);
	mutex_unlock(&host->pin_mutex);

tag:
	/* clocks is provided to eliminate power-up synchronization problems */
	/* enabel cmd irq */
	writel(readl(HOST_STATE(host)) | SD_STATE_TEIE,
			HOST_STATE(host));

	/* module function enable */
	if (mmc_card_expected_wifi(host->type_expected)) {
		writel(readl(HOST_EN(host)) | SD_EN_SDIOEN,
			HOST_EN(host));
	}

	acts_mmc_send_init_clk(host);

}

static void acts_mmc_power_off(struct gl520xmmc_host *host)
{
	mutex_lock(&host->pin_mutex);
	if (acts_mmc_free_pinctrl(host) < 0)
		pr_err("SDC%d free pinctrl failed\n", host->id);
	mutex_unlock(&host->pin_mutex);
}

static void acts_mmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct gl520xmmc_host *host = mmc_priv(mmc);
	u32 ctrl_reg;



	if (ios->power_mode != host->power_state) {
		host->power_state = ios->power_mode;

		switch (ios->power_mode) {
		case MMC_POWER_UP:
			pr_debug("\tMMC_POWER_UP\n");
			acts_mmc_power_up(host);
			break;
		case MMC_POWER_ON:
			pr_debug("\tMMC_POWER_ON\n");
			acts_mmc_power_on(host);
			break;
		case MMC_POWER_OFF:
			pr_debug("\tMMC_POWER_OFF\n");
			acts_mmc_power_off(host);
			break;
		default:
			pr_err("Power mode not supported\n");
		}
	}

	if (ios->clock && ios->clock != host->clock) {
		host->clock = ios->clock;
		pr_debug("\tSet clock: %d\n", host->clock);
		acts_mmc_set_clk(host, ios->clock);
	}

	ctrl_reg = readl(HOST_EN(host));
	{
		host->bus_width = ios->bus_width;
		switch (ios->bus_width) {
		case MMC_BUS_WIDTH_8:
			ctrl_reg &= ~0x3;
			ctrl_reg |= 0x2;
			break;
		case MMC_BUS_WIDTH_4:
			ctrl_reg &= ~0x3;
			ctrl_reg |= 0x1;
			break;
		case MMC_BUS_WIDTH_1:
			ctrl_reg &= ~0x3;
			break;
		}
	}

	if (ios->chip_select != host->chip_select) {
		host->chip_select = ios->chip_select;
		switch (ios->chip_select) {
		case MMC_CS_DONTCARE:
			break;
		case MMC_CS_HIGH:
			ctrl_reg &= ~0x3;
			ctrl_reg |= 0x1;
			break;
		case MMC_CS_LOW:
			ctrl_reg &= ~0x3;
			break;
		}
	}

	if(ios->timing != host->timing)	{
		host->timing = ios->timing;
		if (ios->timing == MMC_TIMING_UHS_DDR50){
			ctrl_reg |= (1 << 2);
		}
	}

	switch (ctrl_reg & 0x3) {
	case 0x2:
		pr_debug("\tMMC_BUS_WIDTH_8\n");
		break;
	case 0x1:
		pr_debug("\tMMC_BUS_WIDTH_4\n");
		break;
	case 0x0:
		pr_debug("\tMMC_BUS_WIDTH_1\n");
		break;
	default:
		pr_debug("\tMMC_BUS_WIDTH NOT known\n");
	}

	writel(ctrl_reg, HOST_EN(host));
}

static void acts_mmc_gpio_check_status(unsigned long data)
{
	struct gl520xmmc_host *host = (struct gl520xmmc_host *)data;
	int card_present;

	if (host->card_detect_reverse)
		card_present = !!(gpio_get_value(host->detect_pin));
	else
		card_present = !(gpio_get_value(host->detect_pin));

	if (card_present ^ host->present) {
		/* debouncer */
		mdelay(20);

		if (host->card_detect_reverse)
			card_present = !!(gpio_get_value(host->detect_pin));
		else
			card_present = !(gpio_get_value(host->detect_pin));

		if (card_present ^ host->present) {
			pr_info("%s: Slot status change detected (%d -> %d)\n",
				mmc_hostname(host->mmc), host->present,
				card_present);

				mmc_detect_change(host->mmc, 0);

			host->present = card_present;
		}
	}
	mod_timer(&host->timer, jiffies + HZ/5);
}

static void acts_mmc_detect_irq_enable(struct gl520xmmc_host *host)
{

}

static irqreturn_t acts_mmc_detect_irq_handler(int irq, void *devid)
{
	return IRQ_NONE;
}


static void acts_mmc_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct gl520xmmc_host *host = mmc_priv(mmc);
	unsigned long flags;
	u32 state;

	spin_lock_irqsave(&host->lock, flags);
	state = readl(HOST_STATE(host));
	if (enable) {
		state |= SD_STATE_SDIOA_EN;
		/* default SDIOA,  protect irq throw away */
		state &= ~SD_STATE_SDIOA_P;
		state &= ~SD_STATE_TEI;
	} else {
		state |= SD_STATE_SDIOA_P;
		state &= ~SD_STATE_SDIOA_EN;
		state &= ~SD_STATE_TEI;
	}

	writel(state, HOST_STATE(host));
	spin_unlock_irqrestore(&host->lock, flags);
}

static irqreturn_t acts_sdc_irq_handler(int irq, void *devid)
{
	struct gl520xmmc_host *host = devid;
	struct mmc_host *mmc = host->mmc;
	unsigned long flags;
	u32 state;
	u32 temp;

	spin_lock_irqsave(&host->lock, flags);
	state = readl(HOST_STATE(host));
		/* check cmd irq */
	if (state & SD_STATE_TEI) {
		temp = readl(HOST_STATE(host));
		temp = temp &(~SD_STATE_SDIOA_P);
		temp |= SD_STATE_TEI;
		writel(temp,HOST_STATE(host));
		complete(&host->sdc_complete);
	}
	spin_unlock_irqrestore(&host->lock, flags);
	/*check sdio date0 irq */
	if(mmc->caps & MMC_CAP_SDIO_IRQ){
		if ((state & SD_STATE_SDIOA_P) && (state & SD_STATE_SDIOA_EN)) {
			DEBUG("wifihost%d:%s %d \n",host->id ,__FUNCTION__,__LINE__);
			mmc_signal_sdio_irq(host->mmc);
		}
	}

	return IRQ_HANDLED;
}

static unsigned int _config_read_dma_mode(unsigned int sdc_id)
{
	unsigned int dma_mode = -1;

	switch (sdc_id) {
	case SDC0_SLOT:
		dma_mode = ATV520X_SDC0RD_DMAMODE;
		break;
	case SDC1_SLOT:
		dma_mode = ATV520X_SDC1RD_DMAMODE;
		break;
	case SDC2_SLOT:
		dma_mode = ATV520X_SDC2RD_DMAMODE;
		break;
	default:
		pr_err("error: MMC/SD slot %d not support\n", sdc_id);
		return -1;
	}

	return dma_mode;
}

static unsigned int _config_write_dma_mode(unsigned int sdc_id)
{
	unsigned int dma_mode;

	switch (sdc_id) {
	case SDC0_SLOT:
		dma_mode = ATV520X_SDC0WT_DMAMODE;
		break;
	case SDC1_SLOT:
		dma_mode = ATV520X_SDC1WT_DMAMODE;
		break;
	case SDC2_SLOT:
		dma_mode = ATV520X_SDC2WT_DMAMODE;
		break;
	default:
		pr_err("error: MMC/SD slot %d not support\n", sdc_id);
		return -1;
	}

	return dma_mode;
}


/* add work to decrese module init mmc for kernel start
or it better to use mmc_add_host directly
*/
static void mmc_host_add_work(struct work_struct *work)
{
	
	struct delayed_work	*phost_add_work = (struct delayed_work*)work;
	struct gl520xmmc_host *host =container_of(phost_add_work, \
									struct gl520xmmc_host,host_add_work);
	mmc_add_host(host->mmc);
}


static void acts_mmc_finish_request(struct gl520xmmc_host *host)
{
	struct mmc_request *mrq;
	struct mmc_data *data;

	WARN_ON(!host->mrq);

	mrq = host->mrq;
	host->mrq = NULL;


	if (mrq->data) {
		data = mrq->data;

		/* Finally finished */
		dma_unmap_sg(host->dma->device->dev, data->sg, data->sg_len,
			host->dma_dir);
	}

	mmc_request_done(host->mmc, mrq);
}

/*
 * Since send_command can be called by data_complete,
 * so it should not "finish the request".
 * acts_mmc_send_command May sleep!
 */

static int acts_mmc_send_command(struct gl520xmmc_host *host,
	struct mmc_command *cmd, struct mmc_data *data)
{

	u32 mode;
	u32 rsp[2];
	unsigned int cmd_rsp_mask = 0;
	u32 status ;
	int err = 0;

	cmd->error = 0;
	
	init_completion(&host->sdc_complete);
	
	switch (mmc_resp_type(cmd)) {
	case MMC_RSP_NONE:
		mode = SD_CTL_TM(0);
		break;

	case MMC_RSP_R1:
		if (data) {
			if (data->flags & MMC_DATA_READ)
				mode = SD_CTL_TM(4);
			else
				mode = SD_CTL_TM(5);
		} else {
			mode = SD_CTL_TM(1);
		}
		cmd_rsp_mask = 	SD_STATE_CLNR 
						|SD_STATE_CRC7ER;
		
		break;

	case MMC_RSP_R1B:
		mode = SD_CTL_TM(3);
		cmd_rsp_mask =   SD_STATE_CLNR 
						|SD_STATE_CRC7ER;
		break;

	case MMC_RSP_R2:
		mode = SD_CTL_TM(2);
		cmd_rsp_mask =  SD_STATE_CLNR 
						|SD_STATE_CRC7ER;
		break;

	case MMC_RSP_R3:
		mode = SD_CTL_TM(1);
		cmd_rsp_mask =  SD_STATE_CLNR ;
		break;

	default:
		pr_err("no math command RSP flag %x\n", cmd->flags);
		cmd->error = -1;
		return MMC_CMD_COMPLETE;
	}
	

	/* keep current RDELAY & WDELAY value */
	mode |= (readl(HOST_CTL(host)) & (0xff << 16));
	
	/* start to send corresponding command type */	
	writel(cmd->arg, HOST_ARG(host));
	writel(cmd->opcode, HOST_CMD(host));
	DEBUG("host%d Transfer mode:0x%x\n\tArg:0x%x\n\tCmd:%u\n",
			host->id,mode, cmd->arg, cmd->opcode);
	
	
	/*set lbe to send clk after busy*/
	if(data){
		mode |= (SD_CTL_TS |SD_CTL_LBE| 0xE4000000);
	}else{
		/*pure cmd disable hw timeout and SD_CTL_LBE*/
		mode  &= ~(SD_CTL_TOUTEN|SD_CTL_LBE);
		mode  |=  SD_CTL_TS ;
	}

 	/* start transfer */
	writel(mode, HOST_CTL(host));
	
	pr_debug("SDC%d send CMD%d, SD_CTL=0x%x\n", host->id,
				cmd->opcode, readl(HOST_CTL(host)));
	
	/* date cmd return */
	if(data){
		return  DATA_CMD;
	}
	
	/*
	*wait for cmd finish 
	* Some bad card dose need more time to complete
	* data transmission and programming.
	*/
	if (!wait_for_completion_timeout(&host->sdc_complete,  30*HZ)) {
		pr_err("!!!host%d:cmd wait ts interrupt timeout\n",host->id);
		cmd->error = CMD_TS_TIMEOUT;
		act_dump_reg(host);
		goto out;
	}

	DEBUG("host%d: wait cmd sdc_complete OK\n",host->id);
	
	status = readl(HOST_STATE(host));
	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd_rsp_mask & status  ){
			if ( status & SD_STATE_CLNR ) {	
				cmd->error = CMD_RSP_ERR;	
				#if 0	
					pr_err("SDC%d send CMD%d: CMD_NO_RSP...\n",		
					host->id, cmd->opcode);			
				#endif	
				goto out;
			}	
			
			if ( status & SD_STATE_CRC7ER) {	
				
				cmd->error = -EILSEQ;

				host->read_delay_chain --;
				if(host->read_delay_chain < 0){
					host->read_delay_chain = 0xf;
				}

				writel((readl(HOST_CTL(host))  & (~(0xff << 16))) |
					SD_CTL_RDELAY(host->read_delay_chain) |
					SD_CTL_WDELAY(host->write_delay_chain),
					HOST_CTL(host));
				
				printk("cmd:try read delay chain:%d\n",
				host->read_delay_chain);
				
				pr_err("cmd:SDC%d send CMD%d, CMD_RSP_CRC_ERR\n",
					host->id, cmd->opcode);	
				
				goto out;
			}	
	
		}
		
		if (cmd->flags & MMC_RSP_136) {	
		/*TODO: MSB first */		
			cmd->resp[3] = readl(HOST_RSPBUF0(host));	
			cmd->resp[2] = readl(HOST_RSPBUF1(host));	
			cmd->resp[1] = readl(HOST_RSPBUF2(host));	
			cmd->resp[0] = readl(HOST_RSPBUF3(host));	
		} else {		
			rsp[0] = readl(HOST_RSPBUF0(host));		
			rsp[1] = readl(HOST_RSPBUF1(host));	
			cmd->resp[0] = rsp[1] << 24 | rsp[0] >> 8;		
			cmd->resp[1] = rsp[1] >> 8;	
		}	
	}	
	
out:
	return  PURE_CMD;
	
}
static  void  acts_mmc_dma_complete(struct gl520xmmc_host *host)
{
	BUG_ON( !host->mrq->data);
	
	//unsigned long flags;
	
	if(host->dma_terminate == true){
		host->dma_terminate = false;
		printk("%s:return for  dmaengine_terminate_all\n",__FUNCTION__);
		return ;
	}
	
	if( host->mrq->data ){
		complete(&host->dma_complete);
		host->dmaflag = true;
	}

}



static int acts_mmc_prepare_data(struct gl520xmmc_host *host,
	struct mmc_data *data)
{
	struct scatterlist *sg;
	enum dma_transfer_direction slave_dirn;
	int i, sglen;
	unsigned total;
	
	host->dmaflag = false;
	host->dma_terminate = false ;

	writel(readl(HOST_EN(host)) | SD_EN_BSEL, HOST_EN(host));

	writel(data->blocks, HOST_BLK_NUM(host));
	writel(data->blksz, HOST_BLK_SIZE(host));

	total = data->blksz * data->blocks;

	if (total < 512)
		writel(total , HOST_BUF_SIZE(host));
	else
		writel(512, HOST_BUF_SIZE(host));

	/*
	 * We don't do DMA on "complex" transfers, i.e. with
	 * non-word-aligned buffers or lengths.
	 */
	for_each_sg(data->sg, sg, data->sg_len, i) {
		if (sg->offset & 3 || sg->length & 3)
			pr_err("SD tag: non-word-aligned buffers or lengths.\n");
	}

	if (data->flags & MMC_DATA_READ) {
		host->dma_dir = DMA_FROM_DEVICE;
		host->dma_conf.direction = slave_dirn = DMA_DEV_TO_MEM;
	} else if (data->flags & MMC_DATA_WRITE) {
		host->dma_dir = DMA_TO_DEVICE;
		host->dma_conf.direction = slave_dirn = DMA_MEM_TO_DEV;
	} else {
		BUG_ON(1);
	}

	sglen = dma_map_sg(host->dma->device->dev, data->sg,
		data->sg_len, host->dma_dir);

	host->dma_slave.dma_dev = host->dma->device->dev;
	host->dma_slave.trans_type = SLAVE;
	if (data->flags & MMC_DATA_READ)
		host->dma_slave.mode = _config_read_dma_mode(host->id);
	else
		host->dma_slave.mode = _config_write_dma_mode(host->id);

	host->dma->private = (void *)&host->dma_slave;
	if (dmaengine_slave_config(host->dma, &host->dma_conf))
		pr_err("Failed to config DMA channel\n");

	host->desc = dmaengine_prep_slave_sg(host->dma,
		data->sg, sglen, slave_dirn,
		DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!host->desc) {
		pr_err("dmaengine_prep_slave_sg() fail\n");
		return -EBUSY;
	}

	host->desc->callback = acts_mmc_dma_complete;
	host->desc->callback_param = host;
	/*
	*init for adjust delay chain 
	*/
	data->error = 0;


	return 0;
}

static int acts_mmc_card_exist(struct mmc_host *mmc)
{
	struct gl520xmmc_host *host = mmc_priv(mmc);
	int present;

	if (mmc_card_expected_mem(host->type_expected)) {
		if (detect_use_gpio) {
			if (host->card_detect_reverse)
				present = !!(gpio_get_value(host->detect_pin));
			else
				present = !(gpio_get_value(host->detect_pin));

			return present;
		}
	}

	if (mmc_card_expected_wifi(host->type_expected))
		return host->sdio_present;

	return -ENOSYS;
}

static void acts_mmc_err_reset(struct gl520xmmc_host *host)
{
	u32 reg_en, reg_ctr, reg_state;

	reg_en = readl(HOST_EN(host));
	reg_ctr = readl(HOST_CTL(host));
	reg_state = readl(HOST_STATE(host));

	module_reset(host->module_id);
	
	writel(SD_ENABLE, HOST_EN(host));
	writel(reg_en, HOST_EN(host));
	reg_ctr &=~SD_CTL_TS;
	writel(reg_ctr, HOST_CTL(host));
	writel(reg_state, HOST_STATE(host));
}

//#define DEBUG_ERR 1

static void acts_mmc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct gl520xmmc_host *host = mmc_priv(mmc);
	int ret = 0;
	int err = 0;
	static unsigned int time = 0;
	static  bool flag = false;
	unsigned long flags;
	// check card is removed
	if ((acts_mmc_card_exist(mmc) == 0)&&\
		(!(mmc->caps&MMC_CAP_NONREMOVABLE))){
		mrq->cmd->error = -ENOMEDIUM;
		mmc_request_done(mmc, mrq);
		return;
	}
	/*
	 * the pointer being not NULL means we are making request on sd/mmc,
	 * which will be reset to NULL in finish_request.
	 */

#if DEBUG_ERR
	if(mrq->data){
		if(time++  >300 ){
			time = 0;
			
			if(mrq->data->flags & MMC_DATA_READ){
				host->read_delay_chain = 0xd;
				pr_err("set error read_delay_chain:0x%x\n",host->read_delay_chain );	
			}else {
				host->write_delay_chain = 0xd;
				pr_err("set error write_delay_chain:0x%x\n",host->write_delay_chain );	
				
			}

			writel((readl(HOST_CTL(host))  & (~(0xff << 16))) |
				SD_CTL_RDELAY(host->read_delay_chain) |
				SD_CTL_WDELAY(host->write_delay_chain),
				HOST_CTL(host));

		}
	}
#endif


	host->mrq = mrq;
	
	if (mrq->data) {
		ret = acts_mmc_prepare_data(host, mrq->data);
		if (ret != 0) {
			pr_err("SD DMA transfer: prepare data error\n");
			mrq->data->error = ret;
			acts_mmc_finish_request(host);
			return;
		} else {
			init_completion(&host->dma_complete);
			dmaengine_submit(host->desc);
			dma_async_issue_pending(host->dma);
		}
	}

	ret = acts_mmc_send_command(host, mrq->cmd, mrq->data);
	
chec_err:	
	if( ret == DATA_CMD){

		if (!wait_for_completion_timeout(&host->sdc_complete,  10*HZ)) {
				pr_err("!!!host%d:wait date transfer ts intrupt timeout\n",host->id);
		}	
		
		DEBUG("host%d: wait date sdc_complete OK\n",host->id);
		
		if(act_check_trs_date_status(host , mrq)){
		 	pr_err("!!!host%d err:act_check_trs_date_status\n",host->id);
			act_dump_reg(host);
			pr_err("Entry SD/MMC module error reset\n");
			
			host->dma_terminate = true ;		
			dmaengine_terminate_all(host->dma);

			acts_mmc_err_reset(host);
			pr_err("Exit SD/MMC module error reset\n");
			goto finish;
		}
	 
		if (!wait_for_completion_timeout(&host->dma_complete,  5*HZ)) {
			pr_err("!!!host%d:dma transfer completion timeout\n",host->id);

			pr_err("!!!host%d:dmaflag:%d\n",host->id ,host->dmaflag);
			mrq->data->error = CMD_DATA_TIMEOUT;
			mrq->cmd->error = -ETIMEDOUT;	
			act_dump_reg(host);
			pr_err("Entry SD/MMC module error reset\n");		
			host->dma_terminate = true ;			
			dmaengine_terminate_all(host->dma);
			acts_mmc_err_reset(host);
			pr_err("Exit SD/MMC module error reset\n");
			goto finish;

		}	
		
		DEBUG("host%d: wait date dma_complete OK\n",host->id);
		
		if (mrq->data->stop){
			/* send stop cmd */
			acts_mmc_send_command(host, mrq->data->stop, NULL);
			if(mrq->data->stop->error){
				act_dump_reg(host);
				pr_err("Entry SD/MMC module error reset\n");	
				acts_mmc_err_reset(host);
				pr_err("Exit SD/MMC module error reset\n");
				goto finish;
			}
		}

		mrq->data->bytes_xfered = mrq->data->blocks *
								  mrq->data->blksz;
		
	}

finish:
	acts_mmc_finish_request(host);
}


static void act_dump_reg(struct gl520xmmc_host *host)
{
	acts_dump_mfp(host);
	acts_dump_sdc(host);
	acts_dump_dmac(host);
}


/* check status reg  is ok */
static int act_check_trs_date_status(
		struct gl520xmmc_host *host , struct mmc_request *mrq)
{
	
	struct mmc_data *data = mrq->data;
	struct mmc_command	*cmd = mrq->cmd;
	u32 status =readl(HOST_STATE(host)) ;
	u32 check_status  = 0;
	u32 cmd_rsp_mask = 0;
	int ret = 0;

	
	if (!host || !host->mrq || !host->mrq->data) {
		pr_err("SDC%d when DMA finish, request is NULL\n",
			host->id);
		return;
	}

	cmd_rsp_mask =   SD_STATE_TOUTE 
					|SD_STATE_CLNR 
					|SD_STATE_WC16ER
					|SD_STATE_RC16ER
					|SD_STATE_CRC7ER;

	check_status = status &cmd_rsp_mask;

	if(check_status){
		if(check_status & SD_STATE_TOUTE){
			pr_err("data:card HW  timeout error\n");
			data->error = HW_TIMEOUT;
			goto out;

		}
		if(check_status & SD_STATE_CLNR){
			pr_err("data:card cmd line no respond error\n");
			data->error = CMD_RSP_ERR;
			goto out;
		}
		if(check_status & SD_STATE_WC16ER){
			
			pr_err("data:card write:crc error\n");
			data->error =  DATA_WR_CRC_ERR;
			cmd->error = -EILSEQ;
			goto out;
		}
		if(check_status & SD_STATE_RC16ER){
			pr_err("data:card read:crc error\n");
			data->error = DATA_RD_CRC_ERR;
			cmd->error = -EILSEQ;
			goto out;
		}
		if(check_status & SD_STATE_CRC7ER){
			pr_err("data: cmd  CMD_RSP_CRC_ERR\n");
			data->error = CMD_RSP_CRC_ERR;
			cmd->error = -EILSEQ;
			goto out;
		}
	}
	
out:
	if((data->error == DATA_RD_CRC_ERR) || 
		(data->error == CMD_RSP_CRC_ERR)){
		host->read_delay_chain --;
		if(host->read_delay_chain < 0){
			host->read_delay_chain = 0xf;
		}
		
		printk("try read delay chain:%d\n",
				host->read_delay_chain);
 		
	}else if(data->error == DATA_WR_CRC_ERR){
	
		host->write_delay_chain --;
		if(host->write_delay_chain < 0){
			host->write_delay_chain = 0xf;
		}
		
		printk("try write delay chain:%d\n",
			host->write_delay_chain);
	}

	if(data->error == DATA_WR_CRC_ERR||
		data->error == DATA_RD_CRC_ERR){
		writel((readl(HOST_CTL(host))  & (~(0xff << 16))) |
			SD_CTL_RDELAY(host->read_delay_chain) |
			SD_CTL_WDELAY(host->write_delay_chain),
			HOST_CTL(host));
	}

	return (data->error);
}

static int acts_mmc_signal_voltage_switch(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct gl520xmmc_host *host = mmc_priv(mmc);
	u32 state;
	int i = 0;
	int ret = 0;

	if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330) {
		goto out;
	} else if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180) {

		/* stop send clk until 5ms later */
		mdelay(5);

		/* switch host voltage to 1.8v and delay 10ms */
		writel(readl(HOST_EN(host)) | SD_EN_S18EN,
			HOST_EN(host));
		mdelay(10);

		/* send continuous clock */
		writel(readl(HOST_CTL(host)) | SD_CTL_SCC,
			HOST_CTL(host));
		for (i = 0; i < 100; i++) {
			state = readl(HOST_STATE(host));
			if ((state & SD_STATE_CMDS) && (state & SD_STATE_DAT0S))
				break;
			udelay(100);
		}
		if (i >= 100) { /* max 10ms */
			pr_err("SDC%d level error, voltage switch failed\n",
				host->id);
			ret = -EINVAL;
		}

		/* stop to send continuous clock */
		writel(readl(HOST_CTL(host)) & ~SD_CTL_SCC,
			HOST_CTL(host));
	}

out:

	return ret;
}

static int acts_mmc_busy(struct mmc_host *mmc)
{
		unsigned int state;
		unsigned int i = 0;
		struct gl520xmmc_host *host = mmc_priv(mmc);
		state = readl(HOST_STATE(host));
		while ((state & SD_STATE_CMDS) || (state & SD_STATE_DAT0S)) {
		
			if (i >= 100) { /* max 10ms */
				pr_err("%s:SDC%d level error, CMD11 send failed\n",
					__FUNCTION__,host->id);
				return 0;   
			}
			i++;
			udelay(100);
			state = readl(HOST_STATE(host));
		}
		return 1;   //not busy
}

static int acts_mmc_get_ro(struct mmc_host *mmc)
{
	struct gl520xmmc_host *host = mmc_priv(mmc);
	int status = -ENOSYS;
	int read_only = -ENOSYS;

	if (gpio_is_valid(host->wpswitch_gpio)) {
		status = gpio_request(host->wpswitch_gpio,
			"wp_switch");
		if (status) {
			pr_err("%s: %s: Failed to request GPIO %d\n",
				mmc_hostname(mmc), __func__,
				host->wpswitch_gpio);
		} else {
			status = gpio_direction_input(host->wpswitch_gpio);
			if (!status) {
				/*
				 * Wait for atleast 300ms as debounce
				 * time for GPIO input to stabilize.
				 */
				msleep(300);

				/*
				 * SD card write protect switch on, high level.
				 */
				read_only = gpio_get_value_cansleep(
					host->wpswitch_gpio);

			}
			gpio_free(host->wpswitch_gpio);
		}
	}

	pr_info("%s: Card read-only status %d\n", __func__, read_only);

	return read_only;
}

static const struct mmc_host_ops acts_mmc_ops = {
	.get_cd	= acts_mmc_card_exist,
	.request = acts_mmc_request,
	.set_ios = acts_mmc_set_ios,
	.enable_sdio_irq = acts_mmc_enable_sdio_irq,
	.start_signal_voltage_switch = acts_mmc_signal_voltage_switch,
	.card_busy = acts_mmc_busy,
	.get_ro	= acts_mmc_get_ro,
	.switch_sd_pinctr = owl_switch_sd_pinctr,
	.switch_uart_pinctr = owl_switch_uart_pinctr,
};

static int acts_mmc_clkfreq_notify(struct notifier_block *nb,
	unsigned long action, void *data)
{
	struct clk_notifier_data *clk_nd = data;
	unsigned long old_rate, new_rate;
	struct gl520xmmc_host *host;

	host = container_of(nb, struct gl520xmmc_host, nblock);
	old_rate = clk_nd->old_rate;
	new_rate = clk_nd->new_rate;

	if (action == PRE_RATE_CHANGE) {
		/* pause host dma transfer */
	} else if (action == POST_RATE_CHANGE) {
		/* acts_mmc_set_clk(host, new_rate); */
		/* resume to start dma transfer */
	}

	return NOTIFY_OK;
}

static inline void acts_mmc_sdc_config(struct gl520xmmc_host *host)
{
	if (host->start & 0x4000) {
		//res->start
		host->id = SDC1_SLOT;
		host->module_id = MOD_ID_SD1;
		host->pad_drv = SDC1_PAD_DRV;
		host->wdelay.delay_lowclk = SDC1_WDELAY_LOW_CLK;
		host->wdelay.delay_midclk = SDC1_WDELAY_MID_CLK;
		host->wdelay.delay_highclk = SDC1_WDELAY_HIGH_CLK;
		host->rdelay.delay_lowclk = SDC1_RDELAY_LOW_CLK;
		host->rdelay.delay_midclk = SDC1_RDELAY_MID_CLK;
		host->rdelay.delay_highclk = SDC1_RDELAY_HIGH_CLK;
	} else if (host->start& 0x8000) {
		host->id = SDC2_SLOT;
		host->module_id = MOD_ID_SD2;
		host->pad_drv = SDC2_PAD_DRV;
		host->wdelay.delay_lowclk = SDC2_WDELAY_LOW_CLK;
		host->wdelay.delay_midclk = SDC2_WDELAY_MID_CLK;
		host->wdelay.delay_highclk = SDC2_WDELAY_HIGH_CLK;
		host->rdelay.delay_lowclk = SDC2_RDELAY_LOW_CLK;
		host->rdelay.delay_midclk = SDC2_RDELAY_MID_CLK;
		host->rdelay.delay_highclk = SDC2_RDELAY_HIGH_CLK;
	} else {
		host->id = SDC0_SLOT;
		host->module_id = MOD_ID_SD0;
		host->pad_drv = SDC0_PAD_DRV;
		host->wdelay.delay_lowclk = SDC0_WDELAY_LOW_CLK;
		host->wdelay.delay_midclk = SDC0_WDELAY_MID_CLK;
		host->wdelay.delay_highclk = SDC0_WDELAY_HIGH_CLK;
		host->rdelay.delay_lowclk = SDC0_RDELAY_LOW_CLK;
		host->rdelay.delay_midclk = SDC0_RDELAY_MID_CLK;
		host->rdelay.delay_highclk = SDC0_RDELAY_HIGH_CLK;
	}
}

const int of_get_card_detect_mode(struct device_node *np)
{
	const char *pm;
	int err, i;

	err = of_property_read_string(np, "card_detect_mode", &pm);
	if (err < 0)
		return err;
	for (i = 0; i < ARRAY_SIZE(card_detect_modes); i++)
		if (!strcasecmp(pm, card_detect_modes[i]))
			return i;
	pr_err("error: please chose card detect method\n");
	return -ENODEV;
}

const int of_get_card_type(struct device_node *np)
{
	const char *pm;
	int err, i;

	err = of_property_read_string(np, "card_type", &pm);
	if (err < 0)
		return err;
	for (i = 0; i < ARRAY_SIZE(card_types); i++)
		if (!strcasecmp(pm, card_types[i]))
			return i;
	pr_err("error: please make sure card type is exist\n");
	return -ENODEV;
}

static int acts_mmc_get_power(struct gl520xmmc_host *host,
	struct device_node *np)
{
	const char *pm;
	int err;
	
	if (of_find_property(np, "sd_vcc", NULL)) {
		err = of_property_read_string(np, "sd_vcc", &pm);
		if (err < 0) {
			pr_err("SDC[%u] can not read SD_VCC power source\n",
				host->id);
			return -1;
		}

		host->reg = regulator_get(NULL, pm);
		if (IS_ERR(host->reg)) {
			pr_err("SDC[%u] failed to get regulator %s\n",
				host->id, "sd_vcc");
			return -1;
		}

		act_mmc_opt_regulator(host,REG_ENABLE);
	}

	return 0;
}
	
static int alloc_mmc_add_host_workqueue(struct gl520xmmc_host *host)
{

	char mmc_add_host_wq_name[OWL_MMC_WORK_QUEUE_NAME]={0};	
	
	snprintf (mmc_add_host_wq_name, OWL_MMC_WORK_QUEUE_NAME,\
		      "host_add_work%d",host->id);

	host->add_host_wq = alloc_workqueue(mmc_add_host_wq_name,
			WQ_MEM_RECLAIM | WQ_NON_REENTRANT, 1);

	if(NULL == host->add_host_wq){
		printk("%s:alloc mmc_host_add workqueue fail\n",__FUNCTION__);
		return  -ENOMEM;
	}
	
	INIT_DELAYED_WORK(&(host->host_add_work), mmc_host_add_work);
	
	return 0;	
	
}
void cancel_mmc_work(struct mmc_host *host );
void start_mmc_work(struct mmc_host *host );

#ifdef CONFIG_EARLYSUSPEND

static void mmc_early_suspend(struct early_suspend *handler)
{

	struct gl520xmmc_host*host =NULL;
	struct mmc_host *mmc;

	host = container_of(handler,struct gl520xmmc_host,mmc_es_handler);
	mmc = host->mmc ;
	if((mmc_card_expected_mem(host->type_expected))&&\
		(mmc->caps& MMC_CAP_NEEDS_POLL)){
		host->mmc_early_suspend = 1;
		printk("hostid:%d,mmc_early_suspend:host->mmc_early_suspend=%d\n",host->id,host->mmc_early_suspend);
	}

}

static void mmc_late_resume(struct early_suspend *handler)
{
	struct gl520xmmc_host*host =NULL;
	struct mmc_host *mmc;
	
	host = container_of(handler,struct gl520xmmc_host,mmc_es_handler);
	mmc = host->mmc ;
	
	if((mmc_card_expected_mem(host->type_expected)&&\
		(mmc->caps& MMC_CAP_NEEDS_POLL))){
		host->mmc_early_suspend = 0;
		start_mmc_work(mmc);
		printk("hostid:%d,mmc_late_resume:host->mmc_early_suspend=%d\n",host->id,host->mmc_early_suspend);
	}
}
#endif

static int owl_upgrade_flag = OWL_NORMAL_BOOT;

static int __init owl_check_upgrade(char *__unused)
{
	owl_upgrade_flag = OWL_UPGRADE;
	printk("%s:owl_upgrade_flag is OWL_UPGRADE\n",__FUNCTION__);
	return 0 ;
}

__setup("owl_upgrade", owl_check_upgrade);

static int owl_mmc_resan(struct gl520xmmc_host * host,
								unsigned long delay)
{
	int bootdev;

	bootdev = owl_get_boot_dev();
	if(mmc_card_expected_mem(host->type_expected)||
		mmc_card_expected_wifi(host->type_expected)	){
		printk("host%d: sure rescan mmc\n",host->id);
		queue_delayed_work(host->add_host_wq, &host->host_add_work, delay);
	}else if(mmc_card_expected_emmc(host->type_expected)){
		if((owl_upgrade_flag == OWL_UPGRADE)||((bootdev !=OWL_BOOTDEV_NAND)&&\
			(bootdev !=OWL_BOOTDEV_SD02NAND))){
			queue_delayed_work(host->add_host_wq, &host->host_add_work, delay);
		}else{
			host->pcl = pinctrl_get_select_default(host->mmc->parent);
			if (IS_ERR(host->pcl)) {
				pr_err("%s:SDC%d get default pinctrl failed, %ld\n",
				__FUNCTION__,host->id, PTR_ERR(host->pcl));
				return (int)PTR_ERR(host->pcl);
			}
			pinctrl_put(host->pcl);
			pinctrl_put(host->pcl);
			printk("host%d:there is no need to resan emmc\n",host->id);
		}
	}else{
		printk("!!!!!error: mmc type is error\n");
		return -1;

	}
	return 0;	
}

/*
* only when sd0 used for sdcard ,
* we set uart ux rx vaild,sd0 clk cmd vaild,
* so it rescan sdcard and uart pin is vaild
*/
static int owl_requeset_share_uart_sd0_pinctr(struct mmc_host *mmc)
{
	struct gl520xmmc_host *host;
	host = mmc_priv(mmc);
	
	if((host->sdio_uart_supported)&&\
		(mmc_card_expected_mem(host->type_expected))){
	
		sdio_uart_pinctrl_free();
		host->pcl = pinctrl_get_select(mmc->parent, PINCTRL_UART_PIN);
		if (IS_ERR(host->pcl)) {
			pr_err("SDC%d get misc uart pinctrl failed, %ld\n",
			host->id, PTR_ERR(host->pcl));
			return (int)PTR_ERR(host->pcl);
			host->switch_pin_flag  = ERR_PIN;
		}
		host->switch_pin_flag  = UART_PIN ;
		
	}
	return 0;
}
static int __init acts_mmc_probe(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct gl520xmmc_host *host;
	struct resource *res;
	dma_cap_mask_t mask;
	struct device_node *dn = pdev->dev.of_node;
	struct wlan_plat_data *pdata;
	int ret = 0;
	mmc = mmc_alloc_host(sizeof(struct gl520xmmc_host), &pdev->dev);
	if (!mmc) {
		dev_err(&pdev->dev, "require memory for mmc_host failed\n");
		ret = -ENOMEM;
		goto out;
	}

	host = mmc_priv(mmc);
	spin_lock_init(&host->lock);
	mutex_init(&host->pin_mutex);
	host->mmc = mmc;
	host->power_state = host->bus_width = host->chip_select = -1;
	host->clock = 0;
	host->mrq = NULL;
	host->switch_pin_flag = ERR_PIN; //for init pin stat
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource\n");
		ret = -ENODEV;
		goto out_free_host;
	}

	if (!request_mem_region(res->start,
				resource_size(res), pdev->name)) {
		dev_err(&pdev->dev, "Unable to request register region\n");
		return -EBUSY;
	}

	host->iobase = ioremap(res->start, resource_size(res));
	if (host->iobase == NULL) {
		dev_err(&pdev->dev, "Unable to ioremap register region\n");
		return -ENXIO;
	}

	
	host->start = res->start;
	

	host->type_expected = of_get_card_type(dn);
	if (host->type_expected < 0)
		goto out_free_host;
	
	acts_mmc_sdc_config(host);

	if(alloc_mmc_add_host_workqueue(host)){
		pr_info("SDC%d request dma queue fail\n", host->id);
		goto err_dma_workqueue; 
	}

	ret = module_clk_get(host);
	if (ret < 0)
		goto err_add_host_workqueue;

	memset(&host->nblock, 0, sizeof(host->nblock));
	host->nblock.notifier_call = acts_mmc_clkfreq_notify;
	/* clk_notifier_register(host->clk, &host->nblock); */

	ret = acts_mmc_get_power(host, dn);
	if (ret < 0)
		goto out_put_clk;

	if (of_find_property(dn, "sdio_uart_supported", NULL)) {
		host->sdio_uart_supported = 1;
		pr_info("SDC%d use sdio uart conversion\n", host->id);
	}

	if (of_find_property(dn, "card_detect_reverse", NULL)) {
		host->card_detect_reverse = 1;
		pr_info("SDC%d detect sd card use reverse power-level\n",
			host->id);
	}

	/* MT5931 SDIO WiFi need to send continuous clock */
	if (mmc_card_expected_wifi(host->type_expected)) {
		if (of_find_property(dn, "send_continuous_clock", NULL)) {
			host->send_continuous_clock = 1;
			pr_info("SDC%d wifi send continuous clock\n", host->id);
		}
	}

	/* Request DMA channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	host->dma = dma_request_channel(mask, NULL, NULL);
	if (!host->dma) {
		dev_err(&pdev->dev, "Failed to request DMA channel\n");
		ret = -ENODEV;
		goto out_put_regulator;
	}

	dev_info(&pdev->dev, "using %s for DMA transfers\n",
		dma_chan_name(host->dma));

	host->dma_conf.src_addr = HOST_DAT_DMA(host);
	host->dma_conf.dst_addr = HOST_DAT_DMA(host);
	host->dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	host->dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	host->dma_conf.device_fc = false;

	mmc->ops = &acts_mmc_ops;

	mmc->f_min = 100000;
	if(SDC0_SLOT == host->id)
	/*5206 sd0 max 50Hz*/
		mmc->f_max = 50000000;  
	else
		mmc->f_max = 100000000; 
	mmc->max_seg_size = 256 * 512;
	mmc->max_segs = 128;
	mmc->max_req_size = 512 * 256;
	mmc->max_blk_size = 512;
	mmc->max_blk_count = 256;

	mmc->ocr_avail = ACTS_MMC_OCR;
	mmc->caps = MMC_CAP_NEEDS_POLL | MMC_CAP_MMC_HIGHSPEED |
		MMC_CAP_SD_HIGHSPEED | MMC_CAP_4_BIT_DATA;

	mmc->caps2 = (MMC_CAP2_BOOTPART_NOACC | MMC_CAP2_DETECT_ON_ERR);

	if (of_find_property(dn, "one_bit_width", NULL))
		mmc->caps &= ~MMC_CAP_4_BIT_DATA;

	if (mmc_card_expected_emmc(host->type_expected))
		mmc->caps |= MMC_CAP_1_8V_DDR | MMC_CAP_UHS_DDR50|MMC_CAP_8_BIT_DATA;
	//emmc and sd card support earse (discard,trim,sediscard)
	if(mmc_card_expected_emmc(host->type_expected)||\
		mmc_card_expected_mem(host->type_expected)){
		mmc->caps |= MMC_CAP_ERASE;
	}

	/* SD3.0 support */
	if (SDC0_SLOT == host->id) {	
		mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
			MMC_CAP_UHS_SDR50 ;

		mmc->caps |= MMC_CAP_SET_XPC_330 | MMC_CAP_SET_XPC_300 |
			MMC_CAP_SET_XPC_180;
		
		mmc->caps |= MMC_CAP_MAX_CURRENT_800;
	}

	
	if(owl_requeset_share_uart_sd0_pinctr(mmc)){
			ret = -1;
			dev_err(&pdev->dev,
			"proble requeset share uart sd0 pinctr fail\n");
			goto out_free_dma;
	}

	if (mmc_card_expected_mem(host->type_expected)) {
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
		if (!res) {
			dev_err(&pdev->dev,
				"can not get sdc transfer end irq resource\n");
			ret = -ENODEV;
			goto out_free_dma;
		}
		host->sdc_irq = res->start;
		ret = request_irq(host->sdc_irq,
			(irq_handler_t)acts_sdc_irq_handler, 0,
			"sdcard", host);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"request SDC transfer end interrupt failed\n");
			goto out_free_dma;
		}

		host->card_detect_mode = of_get_card_detect_mode(dn);

		if (host->card_detect_mode == SIRQ_DETECT_CARD) {
			detect_use_sirq = 1;
			mmc->caps &= ~MMC_CAP_NEEDS_POLL;

			/* SIRQ */
			res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
			if (!res) {
				dev_err(&pdev->dev, "no card detect irq resource\n");
				ret = -ENODEV;
				goto out_free_sdc_irq;
			}

			host->detect = host->detect_sirq = res->start;
			acts_mmc_detect_irq_enable(host);

			ret = request_irq(host->detect,
				(irq_handler_t)acts_mmc_detect_irq_handler,
				IRQF_DISABLED, "card-detect", host);
			if (ret < 0) {
				dev_err(&pdev->dev, "unable to request card detect SIRQ%d\n",
					host->detect);
				goto out_free_sdc_irq;
			}

			host->detect_irq_registered = 1;
		} else if (host->card_detect_mode == GPIO_DETECT_CARD) {
			pr_info("use GPIO to detect SD/MMC card\n");
			detect_use_gpio = 1;
			mmc->caps &= ~MMC_CAP_NEEDS_POLL;

			/* card write protecte switch gpio */
			host->wpswitch_gpio = -ENOENT;

			if (of_find_property(dn, "wp_switch_gpio", NULL)) {
				host->wpswitch_gpio = of_get_named_gpio(dn,
					"wp_switch_gpio", 0);
			}

			/* card detect gpio */
			host->detect_pin = of_get_named_gpio(dn, "card_detect_gpios", 0);
			if (gpio_is_valid(host->detect_pin)) {
				ret = gpio_request(host->detect_pin,
					"card_detect_gpio");
				if (ret < 0) {
					dev_err(&pdev->dev, "couldn't claim card detect gpio pin\n");
					goto out_free_sdc_irq;
				}
				gpio_direction_input(host->detect_pin);
			} else {
				dev_err(&pdev->dev, "card detect gpio pin invalid\n");
				goto out_free_sdc_irq;
			}

			host->present = 0;

			init_timer(&host->timer);
			host->timer.data = (unsigned long)host;
			host->timer.function = acts_mmc_gpio_check_status;
			host->timer.expires = jiffies + HZ;
			add_timer(&host->timer);
		} else if (host->card_detect_mode == COMMAND_DETECT_CARD) {
			#ifdef CONFIG_EARLYSUSPEND
			   	host->mmc_early_suspend = 0;
				host->mmc_es_handler.suspend =mmc_early_suspend;
			   	host->mmc_es_handler.resume = mmc_late_resume;
			   	register_early_suspend(&(host->mmc_es_handler));
			#endif
			pr_info("use COMMAND to detect SD/MMC card\n");
		} else {
			pr_err("please choose card detect method\n");
		}
	} else if (mmc_card_expected_wifi(host->type_expected)) {
		mmc->caps &= ~MMC_CAP_NEEDS_POLL;
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
		if (!res) {
			dev_err(&pdev->dev, "no SDIO irq resource\n");
			ret = -ENODEV;
			goto out_free_dma;
		}

		mmc->caps |= MMC_CAP_SDIO_IRQ;
		host->sdio_irq = res->start;
		ret = request_irq(host->sdio_irq,
			(irq_handler_t)acts_sdc_irq_handler, 0,
			"sdio", host);
		if (ret < 0) {
			dev_err(&pdev->dev, "request SDIO interrupt failed\n");
			goto out_free_dma;
		}

		/* dummy device for power control */
		acts_wlan_status_check_register(host);

		pdata = acts_wlan_device.dev.platform_data;
		pdata->parent = pdev;
		ret = platform_device_register(&acts_wlan_device);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to register dummy wifi device\n");
			goto out_free_sdio_irq;
		}

		/* (wifi & bt) power control init */
		acts_wlan_bt_power_init(pdata);

	} else if (mmc_card_expected_emmc(host->type_expected)) {
		mmc->caps &= ~MMC_CAP_NEEDS_POLL;
		mmc->caps |= MMC_CAP_NONREMOVABLE;
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
		if (!res) {
			dev_err(&pdev->dev,
				"can not get sdc transfer end irq resource\n");
			ret = -ENODEV;
			goto out_free_dma;
		}
		host->sdc_irq = res->start;
		ret = request_irq(host->sdc_irq,
			(irq_handler_t)acts_sdc_irq_handler, 0,
			"emmc", host);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"request eMMC SDC transfer end interrupt failed\n");
			goto out_free_dma;
		}
	} else {
		dev_err(&pdev->dev, "SDC%d not supported %d\n",
			host->id, host->type_expected);
		ret = -ENXIO;
		goto out_free_dma;
	}

	ret = owl_mmc_resan(host,10);
	if(ret){
		goto out_free_sdio_irq;
	}
	platform_set_drvdata(pdev, host);

	return 0;

out_free_sdio_irq:
	/* SDIO WiFi card */
	if (mmc_card_expected_wifi(host->type_expected))
		free_irq(host->sdio_irq, host);

	/* memory card */
	if (mmc_card_expected_mem(host->type_expected)) {
		if (gpio_is_valid(host->detect_pin)) {
			del_timer_sync(&host->timer);
			gpio_free(host->detect_pin);
		}

		if (host->detect_irq_registered)
			free_irq(host->detect, host);
	}

out_free_sdc_irq:
	if (mmc_card_expected_mem(host->type_expected))
		free_irq(host->sdc_irq, host);

	if (mmc_card_expected_emmc(host->type_expected))
		free_irq(host->sdc_irq, host);

out_free_dma:
	if (host->dma)
		dma_release_channel(host->dma);

out_put_regulator:
	if (host->reg) {
		regulator_disable(host->reg);
		regulator_put(host->reg);
	}

out_put_clk:
	if (host->clk) {
		/* clk_notifier_unregister(host->clk, &host->nblock); */
		clk_put(host->clk);
	}
err_add_host_workqueue:
	destroy_workqueue(host->add_host_wq);
	
err_dma_workqueue:
	destroy_workqueue(host->dma_wq);

out_free_host:
	mmc_free_host(mmc);

out:
	return ret;
}

static int __exit acts_mmc_remove(struct platform_device *pdev)
{
	struct gl520xmmc_host *host = platform_get_drvdata(pdev);

	if (host) {
		mmc_remove_host(host->mmc);

		if (mmc_card_expected_wifi(host->type_expected)) {
			acts_wlan_bt_power_release();
			platform_device_unregister(&acts_wlan_device);
			free_irq(host->sdio_irq, host);
		}

		if (mmc_card_expected_mem(host->type_expected)) {
			if (gpio_is_valid(host->detect_pin)) {
				del_timer_sync(&host->timer);
				gpio_free(host->detect_pin);
			}
			if (host->detect_irq_registered) {
				free_irq(host->detect, host);
				host->detect_irq_registered = 0;
			}
			free_irq(host->sdc_irq, host);
		}

		if (mmc_card_expected_emmc(host->type_expected))
			free_irq(host->sdc_irq, host);


		if (host->dma)
			dma_release_channel(host->dma);

		/* when stop host, power is off */
		act_mmc_opt_regulator(host,REG_DISENABLE);

		if (host->clk) {
			/* clk_notifier_unregister(host->clk, &host->nblock); */
			clk_put(host->clk);
		}

		mmc_free_host(host->mmc);
		platform_set_drvdata(pdev, NULL);
	}

	return 0;
}


static void suspend_wait_data_finish(struct gl520xmmc_host *host,int timeout)
{

	while ((readl(HOST_CTL(host)) & SD_CTL_TS)&&(--timeout)){
		udelay(1);
	}

	if(timeout <=0 ){
		pr_err("SDC%d mmc suspend wait card finish data timeout\n",host->id);
	}else{
		printk("SDC%d mmc card finish data then enter suspend\n",host->id);
	}

}
#ifdef CONFIG_PM
static int acts_mmc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct gl520xmmc_host *host = platform_get_drvdata(pdev);
	struct mmc_host *mmc = host->mmc;
	int ret = 0;

	pm_message_t * pstate = &state;
	pstate = pstate;

	int bootdev;

	bootdev = owl_get_boot_dev();

	pr_debug("SD%d host controller driver Enter suspend\n",
		host->id);
	//cancel still runing work
	cancel_delayed_work(&host->host_add_work);
	if(host->add_host_wq){
		flush_workqueue(host->add_host_wq);
	}
	
	cancel_delayed_work(&host->dma_work);
	if(host->dma_wq){
		flush_workqueue(host->dma_wq);
	}
	
	//timout 2 s
	suspend_wait_data_finish(host,2000000);

	ret = mmc_suspend_host(host->mmc);
			
	if (mmc&&(mmc->card)&&(mmc->card->type == MMC_TYPE_SDIO)){
	  	config_inner_charger_current(DEV_CHARGER_PRE_CONFIG,
			DEV_CHARGER_CURRENT_WIFI, 0);
			acts_wlan_set_power(acts_get_wlan_plat_data(), 0, 0);
			config_inner_charger_current(DEV_CHARGER_POST_CONFIG,
			DEV_CHARGER_CURRENT_WIFI, 0);		
	}	
	if(bootdev != OWL_BOOTDEV_SD0){
		act_mmc_opt_regulator(host,REG_DISENABLE);
	}
	
	return ret;
}


static int acts_mmc_resume(struct platform_device *pdev)
{
	struct gl520xmmc_host *host = platform_get_drvdata(pdev);
	struct mmc_host *mmc = host->mmc;
	int bootdev;

	bootdev = owl_get_boot_dev();

	pr_debug("SD%d host controller Enter resume\n", host->id);
	
	if(bootdev != OWL_BOOTDEV_SD0){
		act_mmc_opt_regulator(host,REG_ENABLE);
	}	
	
	if (mmc && (mmc->card)&&(mmc->card->type == MMC_TYPE_SDIO)){
		config_inner_charger_current(DEV_CHARGER_PRE_CONFIG,
		DEV_CHARGER_CURRENT_WIFI, 1);
		acts_wlan_set_power(acts_get_wlan_plat_data(), 1, 0);
		config_inner_charger_current(DEV_CHARGER_POST_CONFIG,
		DEV_CHARGER_CURRENT_WIFI, 1);		
	}	
		
	mmc_resume_host(host->mmc);

	return 0;
}
#else
#define acts_mmc_suspend NULL
#define acts_mmc_resume NULL
#endif

static const struct of_device_id acts_mmc_dt_match[]  = {
	{.compatible = "actions,owl-mmc", },
	{}
};

static struct platform_driver acts_mmc_driver = {
	.probe = acts_mmc_probe,
	.remove = acts_mmc_remove,
	.suspend = acts_mmc_suspend,
	.resume = acts_mmc_resume,
	.driver = {
		.name = "gl520x_mmc",
		.owner = THIS_MODULE,
		.of_match_table = acts_mmc_dt_match,
	},
};

static int __init acts_mmc_init(void)
{
	int ret = 0;
	ret = platform_driver_register(&acts_mmc_driver);
	if (ret) {
		pr_err("SD/MMC controller driver register failed\n");
		ret = -ENOMEM;
		
	}
	return ret;
}

static void __exit acts_mmc_exit(void)
{
	platform_driver_unregister(&acts_mmc_driver);

}

module_init(acts_mmc_init);
module_exit(acts_mmc_exit);

MODULE_AUTHOR("Actions");
MODULE_DESCRIPTION("MMC/SD host controller driver");
MODULE_LICENSE("GPL");
