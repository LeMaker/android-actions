/*
 * owl_mmc.c - OWL SD/MMC driver
 *
 * Copyright (C) 2012, Actions Semiconductor Co. LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <common.h>
#include <mmc.h>
#include <part.h>
#include <fdtdec.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <asm/arch/owl_dma.h>
#include <asm/arch/owl_mmc.h>


#define DAM_ALIE_BYTE 32
static struct mmc_config * owl_mmc_dev_init(int index);

#undef pr_debug
#ifdef OWL_MMC_DEBUG
#define pr_debug(format, arg...)	\
	printf(format, ##arg)
#else
#define pr_debug(format, args...)
#endif

#define CONFIG_FPGA	1
#define DATA_TRANSFER_TIMEOUT		(3 * (100 * 1000))  /* 2 seconds */

#define MAX_MMC_NUM 3
static struct mmc mmc_dev[MAX_MMC_NUM] ;
struct owl_mmc_host mmc_host[MAX_MMC_NUM];
static char* mmc_name[MAX_MMC_NUM] = {"SD0", "SD1", "eMMC"};

/* start for misc process */
#define EMMC_SECTOR_SHIFT	9
#define EMMC_SECTOR_SIZE	(1 << EMMC_SECTOR_SHIFT)
#define EMMC_RESERVED		(4 * 1024 * 1024)

#define PART_FREE	0x55
#define PART_DUMMY	0xff
#define PART_READONLY	0x85
#define PART_WRITEONLY	0x86
#define PART_NO_ACCESS	0x87
#define PART_RESERVE	0x99

static int module_reset(enum module_id mod_id)
{
	u32 regv, offset;
	u32 timeout;

	switch (mod_id) {
	case MOD_ID_SD0:
		offset = 1 << 4;
		break;
	case MOD_ID_SD1:
		offset = 1 << 5;
		break;
	case MOD_ID_SD2:
		offset = 1 << 9;
		break;
	default:
		printf("error: RST, Mod not supported\n");
		return -1;
	}

	/* clear */
	timeout = 0;
	regv = readl(CMU_DEVRST0);
	regv &= ~offset;
	writel(regv, CMU_DEVRST0);
	do {
		regv = readl(CMU_DEVRST0);
		regv &= offset;
		timeout++;
	} while ((0 != regv) && (timeout < 0x100));

	/* set */
	timeout = 0;
	regv = readl(CMU_DEVRST0);
	regv |= offset;
	writel(regv, CMU_DEVRST0);
	do {
		regv = readl(CMU_DEVRST0);
		regv &= offset;
		timeout++;
	} while ((0 == regv) && (timeout < 0x100));

	return 0;
}

static void module_clk_set(enum module_id mod_id, int enable)
{
	u32 offset;

	switch (mod_id) {
	case MOD_ID_SD0:
		offset = 1 << 5;
		break;
	case MOD_ID_SD1:
		offset = 1 << 6;
		break;
	case MOD_ID_SD2:
		offset = 1 << 7;
		break;
	default:
		printf("error: CLK, Mod not supported\n");
		return;
	}

	if (enable)
		setbits_le32(CMU_DEVCLKEN0, offset);
	else
		clrbits_le32(CMU_DEVCLKEN0, offset);

	readl(CMU_DEVCLKEN0);
}

static int module_clk_enable(enum module_id mod_id)
{
	module_clk_set(mod_id, 1);
	return 0;
}

static int module_clk_disable(enum module_id mod_id)
{
	module_clk_set(mod_id, 0);
	return 0;
}

static int module_clk_set_rate(enum module_id mod_id, unsigned long rate)
{

	unsigned long regv, div, div128;
	unsigned long parent_rate;

	if ((readl(CMU_DEVPLL) & (1 << 12)) && (readl(CMU_DEVPLL) & (1 << 8))) {
		parent_rate = readl(CMU_DEVPLL) & 0x7f;
		parent_rate *= 6000000;
		pr_debug("MMC: source clk CMU_DEVPLL:%luHz\n", parent_rate);

	} else {
		pr_debug("MMC: parent clock not used, CMU_DEVPLL:0x%x\n",
			readl(CMU_DEVPLL));
		return -1;
	}

	rate *= 2;

	if (rate >= parent_rate)
		div = 0;
	else {
		div = parent_rate / rate;
		if (div >= 128) {
			div128 = div;
			div = div / 128;

			if (div128 % 128)
				div++;

			div--;
			div |= 0x100;
		} else {
			if (parent_rate % rate)
				div++;

			div--;
		}

		if(MOD_ID_SD2 == mod_id){
			regv  = readl(CMU_SD2CLK);
			regv &= 0xfffffce0;
			regv |= div;
			writel(regv, CMU_SD2CLK);
		}else if(MOD_ID_SD0 == mod_id){
			regv  = readl(CMU_SD0CLK);
			regv &= 0xfffffce0;
			regv |= div;
			writel(regv, CMU_SD0CLK);
		}else{
			printf("%s:%d:err\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}
	return 0;
}

static void owl_dump_mfp(struct owl_mmc_host *host)
{
	printf("\tMFP_CTL0:0x%x\n", readl(MFP_CTL0));
	printf("\tMFP_CTL1:0x%x\n", readl(MFP_CTL1));
	printf("\tMFP_CTL2:0x%x\n", readl(MFP_CTL2));
	printf("\tMFP_CTL3:0x%x\n", readl(MFP_CTL3));
	printf("\tPAD_PULLCTL0:0x%x\n", readl(PAD_PULLCTL0));
	printf("\tPAD_PULLCTL1:0x%x\n", readl(PAD_PULLCTL1));
	printf("\tPAD_PULLCTL2:0x%x\n", readl(PAD_PULLCTL2));

	printf("\tPAD_DRV0:0x%x\n", readl(PAD_DRV0));
	printf("\tPAD_DRV1:0x%x\n", readl(PAD_DRV1));
	printf("\tPAD_DRV2:0x%x\n", readl(PAD_DRV2));

	printf("\tCMU_DEVCLKEN0:0x%x\n", readl(CMU_DEVCLKEN0));
	printf("\tCMU_DEVCLKEN1:0x%x\n", readl(CMU_DEVCLKEN1));

	printf("\tCMU_DEVPLL:0x%x\n", readl(CMU_DEVPLL));
	printf("\tCMU_NANDPLL:0x%x\n", readl(CMU_NANDPLL));

	printf("\tCMU_SD0CLK:0x%x\n", readl(CMU_SD0CLK));
	printf("\tCMU_SD1CLK:0x%x\n", readl(CMU_SD1CLK));
	printf("\tCMU_SD2CLK:0x%x\n", readl(CMU_SD2CLK));
}

static void owl_dump_sdc(struct owl_mmc_host *host)
{
	printf("\n\tSD_EN:0x%x\n", readl(HOST_EN(host)));
	printf("\tSD_CTL:0x%x\n", readl(HOST_CTL(host)));
	printf("\tSD_STATE:0x%x\n", readl(HOST_STATE(host)));
	printf("\tSD_CMD:0x%x\n", readl(HOST_CMD(host)));
	printf("\tSD_ARG:0x%x\n", readl(HOST_ARG(host)));
	printf("\tSD_RSPBUF0:0x%x\n", readl(HOST_RSPBUF0(host)));
	printf("\tSD_RSPBUF1:0x%x\n", readl(HOST_RSPBUF1(host)));
	printf("\tSD_RSPBUF2:0x%x\n", readl(HOST_RSPBUF2(host)));
	printf("\tSD_RSPBUF3:0x%x\n", readl(HOST_RSPBUF3(host)));
	printf("\tSD_RSPBUF4:0x%x\n", readl(HOST_RSPBUF4(host)));
	printf("\tSD_DAT:0x%x\n", readl(HOST_DAT(host)));
	printf("\tSD_BLK_SIZE:0x%x\n\n", readl(HOST_BLK_SIZE(host)));
	printf("\tSD_BLK_NUM:0x%x\n", readl(HOST_BLK_NUM(host)));
	printf("\tSD_BUF_SIZE:0x%x\n", readl(HOST_BUF_SIZE(host)));
}

static void owl_dump_dmac(struct owl_mmc_host *host)
{
	int dmanr;

	dmanr = host->dma_channel;
	printf("Current DMA Channel is: %d\n", dmanr);

	printf("\tDMA_MODE:0x%x\n", readl(dma_base[dmanr] + DMA_MODE_OFFSET));
	printf("\tDMA_SOURCE:0x%x\n", readl(dma_base[dmanr] +
			DMA_SOURCE_OFFSET));
	printf("\tDMA_DESTINATION:0x%x\n", readl(dma_base[dmanr] +
			DMA_DESTINATION_OFFSET));
	printf("\tDMA_FRAME_LEN:0x%x\n", readl(dma_base[dmanr] +
			DMA_FRAME_LEN_OFFSET));
	printf("\tDMA_FRAME_CNT:0x%x\n", readl(dma_base[dmanr] +
			DMA_FRAME_CNT_OFFSET));
	printf("\tDMA_REMAIN_FRAME_CNT:0x%x\n", readl(dma_base[dmanr] +
			DMA_REMAIN_FRAME_CNT_OFFSET));
	printf("\tDMA_REMAIN_CNT:0x%x\n", readl(dma_base[dmanr] +
			DMA_REMAIN_CNT_OFFSET));
	printf("\tDMA_SOURCE_STRIDE:0x%x\n", readl(dma_base[dmanr] +
			DMA_SOURCE_STRIDE_OFFSET));
	printf("\tDMA_DESTINATION_STRIDE:0x%x\n", readl(dma_base[dmanr] +
			DMA_DESTINATION_STRIDE_OFFSET));
	printf("\tDMA_START:0x%x\n", readl(dma_base[dmanr] +
			DMA_START_OFFSET));

	/* For advanced debug*/
#ifdef CONFIG_USE_ADVANCED_DMA
	printf("\tDMA_IRQ_PD0_BASE:0x%x\n", readl(DMA_IRQ_PD0));
	printf("\tDMA_IRQ_PD1_BASE:0x%x\n", readl(DMA_IRQ_PD1));
	printf("\tDMA_IRQ_PD2_BASE:0x%x\n", readl(DMA_IRQ_PD2));
	printf("\tDMA_IRQ_PD3_BASE:0x%x\n", readl(DMA_IRQ_PD3));

	printf("\tDMA_IRQ_EN0_BASE:0x%x\n", readl(DMA_IRQ_EN0));
	printf("\tDMA_IRQ_EN1_BASE:0x%x\n", readl(DMA_IRQ_EN1));
	printf("\tDMA_IRQ_EN2_BASE:0x%x\n", readl(DMA_IRQ_EN2));
	printf("\tDMA_IRQ_EN3_BASE:0x%x\n", readl(DMA_IRQ_EN3));


	printf("\tDMA_ACP_ATTRIBUTE:0x%x\n", readl(dma_base[dmanr] +
			DMA_ACP_ATTRIBUTE_OFFSET));
	printf("\tDMA_CHAINED_CTL:0x%x\n", readl(dma_base[dmanr] +
			DMA_CHAINED_CTL_OFFSET));
	printf("\tDMA_CONSTANT:0x%x\n", readl(dma_base[dmanr] +
			DMA_CONSTANT_OFFSET));
	printf("\tDMA_LINKLIST_CTL:0x%x\n", readl(dma_base[dmanr] +
			DMA_LINKLIST_CTL_OFFSET));
	printf("\tDMA_NEXT_DESCRIPTOR:0x%x\n", readl(dma_base[dmanr] +
			DMA_NEXT_DESCRIPTOR_OFFSET));
	printf("\tDMA_CURRENT_DESCRIPTOR_NUM:0x%x\n", readl(dma_base[dmanr] +
			DMA_CURRENT_DESCRIPTOR_NUM_OFFSET));
	printf("\tDMA_INT_CTL:0x%x\n", readl(dma_base[dmanr] +
			DMA_INT_CTL_OFFSET));

	printf("\tDMA_INT_STATUS:0x%x\n", readl(dma_base[dmanr] +
			DMA_INT_STATUS_OFFSET));
	printf("\tDMA_CURRENT_SOURCE_POINTER:0x%x\n", readl(dma_base[dmanr] +
			DMA_CURRENT_SOURCE_POINTER_OFFSET));
	printf("\tDMA_CURRENT_DESTINATION:0x%x\n", readl(dma_base[dmanr] +
			DMA_CURRENT_DESTINATION_POINTER_OFFSET));
#endif
}

static inline int owl_enable_clock(struct owl_mmc_host *host)
{
	int ret;

	if (!host->clk_on) {
		ret = module_clk_enable(host->module_id);
		if (ret) {
			printf("error: enable module clock error\n");
			return ret;
		}
		host->clk_on = 1;
	}
	return 0;
}

static inline int owl_disable_clock(struct owl_mmc_host *host)
{
	int ret;

	if (host->clk_on) {
		ret = module_clk_disable(host->module_id);
		if (ret) {
			printf("error: disable module clock error\n");
			return ret;
		}
		host->clk_on = 0;
	}
	return 0;
}


static int owl_mmc_send_init_clk(struct owl_mmc_host *host)
{
	u32 mode;
	int ret = 0;

	mode = SD_CTL_TS | SD_CTL_SCC | SD_CTL_TCN(5) | SD_CTL_TM(8);
	mode |= (readl(HOST_CTL(host)) & (0xff << 16));

	writel(mode, HOST_CTL(host));
	mdelay(10);
	if (readl(HOST_CTL(host)) & SD_CTL_TS) {
		printf("error: Memory card send init clock timeout error\n");
		ret = -1;
	}

	return ret;
}


static void owl_mmc_set_clk(struct owl_mmc_host *host, int rate)
{
	if (0 == rate)
		return;

	/*
	 * Set the RDELAY and WDELAY based on the sd clk.
	 */
	if (rate <= 1000000) {
		writel((readl(HOST_CTL(host)) & (~(0xff << 16))) |
			SD_CTL_RDELAY(host->rdelay.delay_lowclk) |
			SD_CTL_WDELAY(host->wdelay.delay_lowclk),
			HOST_CTL(host));
	} else if ((rate > 1000000) && (rate <= 26000000)) {
		writel((readl(HOST_CTL(host)) & (~(0xff << 16))) |
			SD_CTL_RDELAY(host->rdelay.delay_midclk) |
			SD_CTL_WDELAY(host->wdelay.delay_midclk),
			HOST_CTL(host));
	} else if ((rate > 26000000) && (rate <= 52000000)) {
		writel((readl(HOST_CTL(host)) & (~(0xff << 16))) |
			SD_CTL_RDELAY(host->rdelay.delay_highclk) |
			SD_CTL_WDELAY(host->wdelay.delay_highclk),
			HOST_CTL(host));
	} else {
		printf("error: SD2.0 max clock should < 50Mhz\n");
	}

	module_clk_set_rate(host->module_id, rate);
}

static void owl_mmc_power_up(struct owl_mmc_host *host)
{
	/* power on reset */
	module_reset(host->module_id);
	module_clk_enable(host->module_id);

	/* module function enable */
	writel(SD_ENABLE | SD_EN_RESE, HOST_EN(host));
}

DECLARE_GLOBAL_DATA_PTR;

static void owl_mmc_power_on(struct owl_mmc_host *host)
{
	u32 pad_drv_mask = 0, pad_drv_high = 0, pad_drv_high2 = 0;
	u32 pad_drv_tmp;

	if (MOD_ID_SD0 == host->module_id) {
		/* mfp & pull up control */
		writel(readl(MFP_CTL2) & 0xfff0061f, MFP_CTL2);
		writel((readl(PAD_PULLCTL1) & 0xffffefff) | 0x3e000,
			PAD_PULLCTL1);

		pr_debug("SDC0 cfg:(MFP_CTL2=0x%x\tPAD_PULLCTL1=0x%x)\n",
			readl(MFP_CTL2), readl(PAD_PULLCTL1));

		/* PAD drive capacity config */
		pad_drv_mask = SD0_DRV_HIGH_MASK & SD0_DRV_HIGH_MASK2;
		switch (host->pad_drv) {
		case PAD_DRV_LOW:
			pad_drv_high = SD0_DRV_HIGH_LOW;
			pad_drv_high2 = SD0_DRV_HIGH2_LOW;
			break;
		case PAD_DRV_MID:
			pad_drv_high = SD0_DRV_HIGH_MID;
			pad_drv_high2 = SD0_DRV_HIGH2_MID;
			break;
		case PAD_DRV_HIGH:
			pad_drv_high = SD0_DRV_HIGH_HIGH;
			pad_drv_high2 = SD0_DRV_HIGH2_HIGH;
			break;
		default:
			pad_drv_high = SD0_DRV_HIGH_HIGH;
			pad_drv_high2 = SD0_DRV_HIGH2_HIGH;
			printf("error: host->pad_drv %d\n", host->pad_drv);
		}
	} else if (MOD_ID_SD1 == host->module_id) {
		/* mfp & pull up control */
		writel((readl(MFP_CTL2) & 0xffffffe7) | 0x600, MFP_CTL2);
		writel(readl(PAD_PULLCTL1) | 0x878, PAD_PULLCTL1);

		pr_debug("SDC1 cfg:(MFP_CTL2=0x%x\tPAD_PULLCTL1=0x%x)\n",
			readl(MFP_CTL2), readl(PAD_PULLCTL1));

		/* PAD drive capacity config */
		pad_drv_mask = SD1_DRV_HIGH_MASK & SD1_DRV_HIGH_MASK2;
		switch (host->pad_drv) {
		case PAD_DRV_LOW:
			pad_drv_high = SD1_DRV_HIGH_LOW;
			pad_drv_high2 = SD1_DRV_HIGH2_LOW;
			break;
		case PAD_DRV_MID:
			pad_drv_high = SD1_DRV_HIGH_MID;
			pad_drv_high2 = SD1_DRV_HIGH2_MID;
			break;
		case PAD_DRV_HIGH:
			pad_drv_high = SD1_DRV_HIGH_HIGH;
			pad_drv_high2 = SD1_DRV_HIGH2_HIGH;
			break;
		default:
			pad_drv_high = SD1_DRV_HIGH_HIGH;
			pad_drv_high2 = SD1_DRV_HIGH2_HIGH;
			printf("error: host->pad_drv %d\n", host->pad_drv);
		}
	} else if (MOD_ID_SD2 == host->module_id) {
		/* mfp & pull up control */
		writel((readl(MFP_CTL3) | (1<<3)), MFP_CTL3);
		writel((readl(PAD_PULLCTL1) | (1<<25)), PAD_PULLCTL1);
		writel((readl(PAD_PULLCTL2) | (1<<2)), PAD_PULLCTL2);
		pr_debug("SDC2 cfg:(MFP_CTL3=0x%x\tPAD_PULLCTL1=0x%x\tPAD_PULLCTL2=0X%x)\n",
			readl(MFP_CTL3), readl(PAD_PULLCTL1), readl(PAD_PULLCTL2));

		/* PAD drive capacity config */
		/* SDC2 not supported PAD - DRV config */
	} else {
		printf("error: power on, MOD%d not supported\n",
			host->module_id);
		return;
	}

	pad_drv_tmp = readl(PAD_DRV1);
	pad_drv_tmp &= pad_drv_mask;
	pad_drv_tmp |= pad_drv_high;
	pad_drv_tmp |= pad_drv_high2;
	writel(pad_drv_tmp, PAD_DRV1);

	/* clocks is provided to eliminate power-up synchronization problems */
	owl_mmc_send_init_clk(host);
}


static int owl_mmc_init_setup(struct mmc *mmc)
{
	struct owl_mmc_host *host = mmc->priv;

	owl_mmc_power_up(host);
	mdelay(10);
	owl_mmc_power_on(host);

	return 0;
}

static void owl_mmc_set_ios(struct mmc *mmc)
{
	struct owl_mmc_host *host = mmc->priv;
	u32 ctrl_reg;

	owl_enable_clock(host);

	pr_debug("owl_mmc_set_ios:\n");

	if (mmc->clock && mmc->clock != host->clock) {
		host->clock = mmc->clock;
		pr_debug("\tSet clock: %d\n", host->clock);
		owl_mmc_set_clk(host, mmc->clock);
	}

	ctrl_reg = readl(HOST_EN(host));
	if (mmc->bus_width != host->bus_width) {
		host->bus_width = mmc->bus_width;
		switch (mmc->bus_width) {
		case 8:
			ctrl_reg &= ~0x3;
			ctrl_reg |= 0x2;
			break;
		case 4:
			ctrl_reg &= ~0x3;
			ctrl_reg |= 0x1;
			break;
		case 1:
			ctrl_reg &= ~0x3;
			break;
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

static unsigned int _config_read_dma_mode(unsigned int sdc_id)
{
	unsigned int dma_mode = -1;

	switch (sdc_id) {
	case SDC0_SLOT:
		dma_mode = OWL_SDC0RD_DMAMODE;
		break;
	case SDC1_SLOT:
		dma_mode = OWL_SDC1RD_DMAMODE;
		break;
	case SDC2_SLOT:
		dma_mode = OWL_SDC2RD_DMAMODE;
		break;
	default:
		printf("error: MMC/SD slot %d not support\n", sdc_id);
		return -1;
	}

	return dma_mode;
}

static unsigned int _config_write_dma_mode(unsigned int sdc_id)
{
	unsigned int dma_mode;

	switch (sdc_id) {
	case SDC0_SLOT:
		dma_mode = OWL_SDC0WT_DMAMODE;
		break;
	case SDC1_SLOT:
		dma_mode = OWL_SDC1WT_DMAMODE;
		break;
	case SDC2_SLOT:
		dma_mode = OWL_SDC2WT_DMAMODE;
		break;
	default:
		printf("error: MMC/SD slot %d not support\n", sdc_id);
		return -1;
	}

	return dma_mode;
}


static int owl_mmc_prepare_data(struct owl_mmc_host *host,
	struct mmc_data *data)
{
	int flags = data->flags;
	unsigned int dma_channel;

	dma_channel = host->dma_channel;
	data->temp_aline_buf = NULL;
	data->alin_buf = NULL;

	pr_debug("Acquire dma channel: %d\n", host->dma_channel);
	pr_debug("\tblocksize:0x%x\n\tblocks:%u\n", data->blocksize,
		data->blocks);

	writel(readl(HOST_EN(host)) | SD_EN_BSEL, HOST_EN(host));

	writel(data->blocksize, HOST_BLK_SIZE(host));
	writel(data->blocks, HOST_BLK_NUM(host));
	if (data->blocksize < 512)
		writel(data->blocksize, HOST_BUF_SIZE(host));
	else
		writel(512, HOST_BUF_SIZE(host));

	stop_dma(dma_channel);

	if (flags & MMC_DATA_READ) {

		if(((unsigned int)(data->dest))%DAM_ALIE_BYTE){
			data->temp_aline_buf = malloc((data->blocks* data->blocksize)+DAM_ALIE_BYTE-1);
			if(data->temp_aline_buf == NULL){
				printf("malloc read buf err:length:0x%08x\n",((data->blocks* data->blocksize)+63));
				return -1;
			}	
			data->alin_buf = (char *) (((unsigned int )(data->temp_aline_buf+DAM_ALIE_BYTE-1))\
								&(~(DAM_ALIE_BYTE-1)));			
		}
		set_dma_mode(dma_channel, _config_read_dma_mode(host->id));
		set_dma_src_addr(dma_channel, HOST_DAT(host));
		if(data->alin_buf){
			set_dma_dst_addr(dma_channel, (unsigned int)data->alin_buf);
		}else{
			set_dma_dst_addr(dma_channel, (unsigned int)data->dest);
		}
		set_dma_frame_len(dma_channel, data->blocks);
		set_dma_frame_count(dma_channel,  data->blocksize);
	} else {
		if(((unsigned int)(data->src))%DAM_ALIE_BYTE){
			data->temp_aline_buf = malloc((data->blocks* data->blocksize)+DAM_ALIE_BYTE-1);
			if(data->temp_aline_buf == NULL){
				printf("malloc read buf err:length:0x%08x\n",((data->blocks* data->blocksize)+63));
				return -1;
			}	
			data->alin_buf =  (char *)(((unsigned int )(data->temp_aline_buf+DAM_ALIE_BYTE-1))\
								&(~(DAM_ALIE_BYTE-1)));			
		}
		set_dma_mode(dma_channel, _config_write_dma_mode(host->id));

		if(data->alin_buf){ /* need aline */
			memcpy(data->alin_buf,data->src,data->blocks* data->blocksize);
			
			set_dma_src_addr(dma_channel,
			(unsigned int)data->alin_buf);
			
			flush_dcache_range((u32)data->alin_buf,
			(u32)data->alin_buf + data->blocks* data->blocksize);

		}else{
			set_dma_src_addr(dma_channel,
				(unsigned int)data->src);

			flush_dcache_range((u32)data->src,
				(u32)data->src + data->blocks* data->blocksize);
		}
		set_dma_dst_addr(dma_channel, HOST_DAT(host));
		set_dma_frame_len(dma_channel, data->blocks);
		set_dma_frame_count(dma_channel,  data->blocksize);	
	}
	start_dma(dma_channel);

	return 0;
}

static void owl_mmc_finish_request(struct owl_mmc_host *host)
{
	/* release DMA, etc */
	stop_dma(host->dma_channel);
	while (readl(HOST_CTL(host)) & SD_CTL_TS) {
		writel(readl(HOST_CTL(host)) & (~SD_CTL_TS),
			HOST_CTL(host));
	}
}

static int owl_mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd,
	struct mmc_data *data)
{
	struct owl_mmc_host *host;
	u32 mode;
	unsigned int cmd_rsp_mask = 0;
	int dat0_busy_check = 0;
	int timeout = DATA_TRANSFER_TIMEOUT;
	int ret = 0;

	host = mmc->priv;

	switch (cmd->resp_type) {
	case MMC_RSP_NONE:
		mode = SD_CTL_TM(0);
		mdelay(1);
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
		cmd_rsp_mask = 0x11;
		break;

	case MMC_RSP_R1b:
		mode = SD_CTL_TM(3);
		cmd_rsp_mask = 0x11;
		dat0_busy_check = 1;
		break;

	case MMC_RSP_R2:
		mode = SD_CTL_TM(2);
		cmd_rsp_mask = 0x11;
		break;

	case MMC_RSP_R3:
		cmd->cmdarg = 0x40ff8000;

		mode = SD_CTL_TM(1);
		cmd_rsp_mask = 0x10;
		break;

	default:
		printf("error: no math command RSP flag %x\n", cmd->cmdarg);
		return -1;
	}

	/* keep current RDELAY & WDELAY value */
	mode |= (readl(HOST_CTL(host)) & (0xff << 16));

	/*
	 * start to send corresponding command type
	 */
	writel(cmd->cmdarg, HOST_ARG(host));
	writel(cmd->cmdidx, HOST_CMD(host));

	if (data) {
		ret = owl_mmc_prepare_data(host, data);
		mode |= SD_CTL_LBE;
		if (ret != 0) {
			printf("error: Prepare data error\n");
			owl_mmc_finish_request(host);
			if(data->temp_aline_buf){
				free(data->temp_aline_buf);
				data->temp_aline_buf= NULL;
				data->alin_buf= NULL;
			}
			return -1;
		}
		mode |= (SD_CTL_TS | 0xE4000000);
	} else {
		mode |= SD_CTL_TS;
	}

	pr_debug("Transfer mode:0x%x\n\tArg:0x%x\n\tCmd:%u\n",
		mode, cmd->cmdarg, cmd->cmdidx);

	writel(mode, HOST_CTL(host));	/* start transfer */

	/* wait SDC transfer complete */
	while ((readl(HOST_CTL(host)) & SD_CTL_TS)  && timeout--)
		udelay(20);

	if (timeout <= 0) {
		printf("error: SDC%d transfered data timeout\n",
			host->id);
		owl_dump_mfp(host);
		owl_dump_sdc(host);
		owl_dump_dmac(host);
		owl_mmc_finish_request(host);
			if(data->temp_aline_buf){
				free(data->temp_aline_buf);			
				data->temp_aline_buf= NULL;
				data->alin_buf= NULL;
			}	
		return -1;
	}


	if (data) {
		timeout = DATA_TRANSFER_TIMEOUT;

		/* waite for DMA transfer complete */
		while (dma_started(host->dma_channel) && timeout--)
			udelay(20);

		if (timeout <= 0) {
			printf("error: DMA%d transfered data timeout\n",
				host->dma_channel);
			owl_dump_mfp(host);
			owl_dump_sdc(host);
			owl_dump_dmac(host);
			owl_mmc_finish_request(host);
			if(data->temp_aline_buf){
				free(data->temp_aline_buf);
				data->temp_aline_buf= NULL;
				data->alin_buf= NULL;
			}		
			return -1;
		}

		owl_mmc_finish_request(host);
		if (data->flags & MMC_DATA_READ){
			if(data->alin_buf){
			invalidate_dcache_range((u32)data->alin_buf,
				(u32)data->alin_buf + data->blocks
				* data->blocksize);
			memcpy(data->dest,data->alin_buf,(data->blocks *data->blocksize ));
			}else{
			invalidate_dcache_range((u32)data->dest,
				(u32)data->dest + data->blocks
				* data->blocksize);
			}
		}
		pr_debug("transfer data finish\n");
		if(data->temp_aline_buf){
			free(data->temp_aline_buf);
			data->temp_aline_buf= NULL;
			data->alin_buf= NULL;
		}	
		return 0;
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd_rsp_mask) {
			if (readl(HOST_STATE(host)) & SD_STATE_CLNR) {
				printf("error: SDC%d send CMD%d, No rsp\n",
					host->id, cmd->cmdidx);
#if 0
				owl_dump_mfp(host);
				owl_dump_sdc(host);
#endif
				return TIMEOUT;
			}

			if (readl(HOST_STATE(host)) & (cmd_rsp_mask &
					SD_STATE_CRC7ER)) {
				printf("error: SDC%d send CMD%d, CRC7 error\n",
					host->id, cmd->cmdidx);
				owl_dump_mfp(host);
				owl_dump_sdc(host);
				return -1;
			}
		}


		/* wait for DAT0 busy status over.
		 * when DAT0 is low level, busy
		 */
		if (dat0_busy_check) {
			while ((readl(HOST_STATE(host)) &
					SD_STATE_DAT0S) != SD_STATE_DAT0S)
				;
		}

		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[3] = readl(HOST_RSPBUF0(host));
			cmd->response[2] = readl(HOST_RSPBUF1(host));
			cmd->response[1] = readl(HOST_RSPBUF2(host));
			cmd->response[0] = readl(HOST_RSPBUF3(host));
		} else {
			u32 rsp[2];
			rsp[0] = readl(HOST_RSPBUF0(host));
			rsp[1] = readl(HOST_RSPBUF1(host));
			cmd->response[0] = rsp[1] << 24 | rsp[0] >> 8;
			cmd->response[1] = rsp[1] >> 8;
		}
	}
	return 0;
}


static void owl_host_init(int index, struct owl_mmc_host *host)
{
	memset((char*)host,0,sizeof(struct owl_mmc_host));
	
	host->clock = 0;
	host->dma_channel = ATS_MMC_DMA_CHAN_NUM;

	switch (index) {
	case 0:
		host->module_id = MOD_ID_SD0;
		host->id = SDC0_SLOT;
		host->iobase = OWL_SDC0_BASE;
		host->pad_drv = SDC0_PAD_DRV;
		host->wdelay.delay_lowclk = SDC0_WDELAY_LOW_CLK;
		host->wdelay.delay_midclk = SDC0_WDELAY_MID_CLK;
		host->wdelay.delay_highclk = SDC0_WDELAY_HIGH_CLK;
		host->rdelay.delay_lowclk = SDC0_RDELAY_LOW_CLK;
		host->rdelay.delay_midclk = SDC0_RDELAY_MID_CLK;
		host->rdelay.delay_highclk = SDC0_RDELAY_HIGH_CLK;
		break;
	case 1:
		host->module_id = MOD_ID_SD1;
		host->id = SDC1_SLOT;
		host->iobase = OWL_SDC1_BASE;
		host->pad_drv = SDC1_PAD_DRV;
		host->wdelay.delay_lowclk = SDC1_WDELAY_LOW_CLK;
		host->wdelay.delay_midclk = SDC1_WDELAY_MID_CLK;
		host->wdelay.delay_highclk = SDC1_WDELAY_HIGH_CLK;
		host->rdelay.delay_lowclk = SDC1_RDELAY_LOW_CLK;
		host->rdelay.delay_midclk = SDC1_RDELAY_MID_CLK;
		host->rdelay.delay_highclk = SDC1_RDELAY_HIGH_CLK;
		break;
	case 2:
		host->module_id = MOD_ID_SD2;
		host->id = SDC2_SLOT;
		host->iobase = OWL_SDC2_BASE;
		host->pad_drv = SDC2_PAD_DRV;
		host->wdelay.delay_lowclk = SDC2_WDELAY_LOW_CLK;
		host->wdelay.delay_midclk = SDC2_WDELAY_MID_CLK;
		host->wdelay.delay_highclk = SDC2_WDELAY_HIGH_CLK;
		host->rdelay.delay_lowclk = SDC2_RDELAY_LOW_CLK;
		host->rdelay.delay_midclk = SDC2_RDELAY_MID_CLK;
		host->rdelay.delay_highclk = SDC2_RDELAY_HIGH_CLK;
		break;
	default:
		printf("error: SD host controller not supported: %d\n", index);
	}
}

static struct mmc_config * owl_mmc_dev_init(int index)
{
	struct mmc_config *mmc_config =  NULL;
	struct mmc_ops *mmc_ops =  NULL;

	mmc_config = malloc(sizeof(struct mmc_config));

	if (mmc_config == NULL){
		printf("err:%s:%d:malloc mmc_config fail\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	mmc_ops = malloc(sizeof(struct mmc_ops));

	if (mmc_ops == NULL){
		printf("err:%s:%d:malloc mmc_ops fail\n",__FUNCTION__,__LINE__);
		return NULL;
	}
	
	mmc_config->ops = mmc_ops;
	mmc_config->name = mmc_name[index];

	mmc_ops->send_cmd = owl_mmc_send_cmd;
	mmc_ops->set_ios = owl_mmc_set_ios;
	mmc_ops->init = owl_mmc_init_setup;
	mmc_ops->getcd = NULL;

	mmc_config->voltages = OWL_MMC_OCR;
	mmc_config->host_caps = (MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS |
		MMC_MODE_HC);

	mmc_config->f_min = 187500;
	mmc_config->f_max = 52000000;

	mmc_config->b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;

	return mmc_config;
}

int owl_sd_init(int index, struct owl_mmc_host *host)
{
	int ret = 0;
	struct mmc *mmc = &mmc_dev[index];
	struct mmc_config * mmc_config = NULL ;
	
	mmc_config = owl_mmc_dev_init(index);
	if(NULL == mmc_config){
		printf("err:%s:%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

	mmc = mmc_create(mmc_config,host);
	
	if(mmc_init(mmc)){
		printf("slot:%d, error mmc_init\n",host->id);
		return -1;
	}
	pr_debug("host->id:%d mmc_init OK\n",host->id);

	mmc->block_dev.blksz = MMC_MAX_BLOCK_LEN;
	pr_debug("mmc block_dev.lba:0x%08lx , blksz:%lu\n",
			mmc->block_dev.lba, mmc->block_dev.blksz);
	
	return ret ;
		
}


int owl_mmc_init(int dev_index)
{
	struct owl_mmc_host *host;
	int ret = 0;

	pr_debug("%s  dev_index:%d\n",__FUNCTION__, dev_index);
	host = &mmc_host[dev_index];
	owl_host_init(dev_index, host);
	ret = owl_sd_init(dev_index, host);
	if(ret){
		printf("error:%d,owl_sd_init fail\n",ret);
	}	

	return ret;
}
