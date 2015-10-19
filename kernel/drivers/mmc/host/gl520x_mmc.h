#ifndef _GL520X_MMC_H_
#define _GL520X_MMC_H_

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dma-direction.h>
#include <linux/clk.h>
#include <mach/hdmac-owl.h>
#include <mach/module-owl.h>
#include <mach/clkname.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define PINCTRL_UART_PIN "share_uart2_5" 
#define OWL_UPGRADE		0
#define OWL_NORMAL_BOOT	1

#define REG_ENABLE 					1
#define REG_DISENABLE 				0
#define MMC_CMD_COMPLETE		1
//#define TSD_SUPPORT 1
/*
 * command response code
 */
#define CMD_OK						BIT(0)
#define CMD_RSP_ERR				BIT(1)
#define CMD_RSP_BUSY				BIT(2)
#define CMD_RSP_CRC_ERR			BIT(3)
#define CMD_TS_TIMEOUT			BIT(4)
#define CMD_DATA_TIMEOUT			BIT(5)
#define HW_TIMEOUT					BIT(6)
#define DATA_WR_CRC_ERR			BIT(7)
#define DATA_RD_CRC_ERR			BIT(8)
#define DATA0_BUSY_ERR				BIT(9)

#define OWL_MMC_WORK_QUEUE_NAME 32
#define OWL_RETRY_DELAY_CHAIN_TIME 2
enum {
	PURE_CMD,
	DATA_CMD ,
};
/*
 * card type
 */
enum {
	MMC_CARD_DISABLE,
	MMC_CARD_MEMORY,
	MMC_CARD_EMMC,
	MMC_CARD_WIFI,
};
#define mmc_card_expected_mem(type)	((type) == MMC_CARD_MEMORY)
#define mmc_card_expected_emmc(type)	((type) == MMC_CARD_EMMC)
#define mmc_card_expected_wifi(type)	((type) == MMC_CARD_WIFI)

/*
 * card detect method
 */
enum {
	SIRQ_DETECT_CARD,
	GPIO_DETECT_CARD,
	COMMAND_DETECT_CARD,
};

#define SDC0_SLOT	0
#define SDC1_SLOT	1
#define SDC2_SLOT	2

#define UART_PIN 	0 
#define SD_PIN   	1
#define ERR_PIN  	-1

#define ACTS_MMC_OCR (MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30  | \
	MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33  | \
	MMC_VDD_33_34 | MMC_VDD_34_35 | MMC_VDD_35_36)

struct mmc_con_delay {
	unsigned char delay_lowclk;
	unsigned char delay_midclk;
	unsigned char delay_highclk;
};

struct gl520xmmc_host {
	spinlock_t lock;
	struct mutex pin_mutex;
	u32 id;				/* SD Controller number */
	u32 module_id;			/* global module ID */
	void __iomem *iobase;
	u32 start;
	u32 type_expected;		/* MEMORY Card or SDIO Card */

	int card_detect_mode;		/* which method used to detect card */

	u32 detect;			/* irq line for mmc/sd card detect */
	u32 detect_sirq;		/* Which SIRQx used to detect card */
	int detect_irq_registered;	/* card detect irq is registered */

	u32 sdc_irq;			/* irq line for SDC transfer end */
	struct completion sdc_complete;

	u32 sdio_irq;			/* irq for SDIO wifi data transfer */
	u32 eject;			/* card status */

	int power_state;		/* card status */
	int bus_width;			/* data bus width */
	int chip_select;
	int timing;
	u32 clock;			/* current clock frequency */
	u32 clk_on;			/* card module clock status */
	struct clk *clk;		/* SDC clock source */
	struct notifier_block nblock;	/* clkfreq notifier block */
	struct regulator *reg;		/* supply regulator */

	struct timer_list timer;	/* used for gpio card detect */
	u32 detect_pin;			/* gpio card detect pin number */
	int wpswitch_gpio;		/* card write protect gpio */
	int present;			/* card is inserted or extracted ? */
	int sdio_present;		/* Wi-Fi is open or not ? */
	char dma_terminate;	
	char switch_pin_flag;	/*UART_PIN: uart mode and host0 cmd sd0 clk vail
							* SD_PIN: cmd clk sd0-sd3
							* ERR_PIN: init status
							*/						
	struct pinctrl *pcl;
	bool dmaflag;						
	unsigned char write_delay_chain;
	unsigned char read_delay_chain;
	unsigned char write_delay_chain_bak;
	unsigned char read_delay_chain_bak;
	unsigned char adjust_write_delay_chain;
	unsigned char adjust_read_delay_chain;
	int sdio_uart_supported;
	int card_detect_reverse;
	int send_continuous_clock;	/* WiFi need to send continuous clock */

	struct mmc_host *mmc;
	struct mmc_request *mrq;

	enum dma_data_direction		dma_dir;
	struct dma_chan			*dma;
	struct dma_async_tx_descriptor	*desc;
	struct dma_slave_config		dma_conf;
	struct owl_dma_slave		dma_slave;

	struct completion		dma_complete;
	struct workqueue_struct *dma_wq;
	struct workqueue_struct *add_host_wq;
	struct delayed_work			dma_work;
	struct delayed_work			host_add_work;

	struct mmc_con_delay		wdelay;
	struct mmc_con_delay		rdelay;
	unsigned char			pad_drv;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend mmc_es_handler; // for elay suspend
	unsigned int mmc_early_suspend;	
#endif
};

/*
 * PAD Drive Capacity config
 */
#define PAD_DRV_LOW		(0)
#define PAD_DRV_MID		(1)
#define PAD_DRV_HIGH		(3)

#define SDC0_WDELAY_LOW_CLK	(0xf)
#define SDC0_WDELAY_MID_CLK	(0xa)
#define SDC0_WDELAY_HIGH_CLK	(0x9)

#define SDC0_RDELAY_LOW_CLK	(0xf)
#define SDC0_RDELAY_MID_CLK	(0xa)
#define SDC0_RDELAY_HIGH_CLK	(0x8)
#define SDC0_RDELAY_DDR50	(0x9)
#define SDC0_WDELAY_DDR50	(0x8)
#define DDR50_CLK			(40000000)

#define SDC0_PAD_DRV		PAD_DRV_MID

#define SDC1_WDELAY_LOW_CLK	(0xf)
#define SDC1_WDELAY_MID_CLK	(0xa)
#define SDC1_WDELAY_HIGH_CLK	(0x8)

#define SDC1_RDELAY_LOW_CLK	(0xf)
#define SDC1_RDELAY_MID_CLK	(0xa)
#define SDC1_RDELAY_HIGH_CLK	(0x8)

#define SDC1_PAD_DRV		PAD_DRV_MID

#define SDC2_WDELAY_LOW_CLK	(0xf)
#define SDC2_WDELAY_MID_CLK	(0xa)
#define SDC2_WDELAY_HIGH_CLK	(0x8)

#define SDC2_RDELAY_LOW_CLK	(0xf)
#define SDC2_RDELAY_MID_CLK	(0xa)
#define SDC2_RDELAY_HIGH_CLK	(0x8)

#define SDC2_PAD_DRV		PAD_DRV_MID


/*
 * SDC registers
 */
#define SD_EN_OFFSET			0x0000
#define SD_CTL_OFFSET			0x0004
#define SD_STATE_OFFSET			0x0008
#define SD_CMD_OFFSET			0x000c
#define SD_ARG_OFFSET			0x0010
#define SD_RSPBUF0_OFFSET		0x0014
#define SD_RSPBUF1_OFFSET		0x0018
#define SD_RSPBUF2_OFFSET		0x001c
#define SD_RSPBUF3_OFFSET		0x0020
#define SD_RSPBUF4_OFFSET		0x0024
#define SD_DAT_OFFSET			0x0028
#define SD_BLK_SIZE_OFFSET		0x002c
#define SD_BLK_NUM_OFFSET		0x0030
#define SD_BUF_SIZE_OFFSET		0x0034

#define HOST_EN(h)			((h)->iobase + SD_EN_OFFSET)
#define HOST_CTL(h)			((h)->iobase + SD_CTL_OFFSET)
#define HOST_STATE(h)			((h)->iobase + SD_STATE_OFFSET)
#define HOST_CMD(h)			((h)->iobase + SD_CMD_OFFSET)
#define HOST_ARG(h)			((h)->iobase + SD_ARG_OFFSET)
#define HOST_RSPBUF0(h)			((h)->iobase + SD_RSPBUF0_OFFSET)
#define HOST_RSPBUF1(h)			((h)->iobase + SD_RSPBUF1_OFFSET)
#define HOST_RSPBUF2(h)			((h)->iobase + SD_RSPBUF2_OFFSET)
#define HOST_RSPBUF3(h)			((h)->iobase + SD_RSPBUF3_OFFSET)
#define HOST_RSPBUF4(h)			((h)->iobase + SD_RSPBUF4_OFFSET)
#define HOST_DAT(h)				((h)->iobase + SD_DAT_OFFSET)
#define HOST_DAT_DMA(h)			((h)->start + SD_DAT_OFFSET)
#define HOST_BLK_SIZE(h)		((h)->iobase + SD_BLK_SIZE_OFFSET)
#define HOST_BLK_NUM(h)			((h)->iobase + SD_BLK_NUM_OFFSET)
#define HOST_BUF_SIZE(h)		((h)->iobase + SD_BUF_SIZE_OFFSET)

/*
 * Register Bit defines
 */

/*
 * Register SD_EN
 */
#define SD_EN_RANE			(1 << 31)
/* bit 30 reserved */
#define SD_EN_RAN_SEED(x)		(((x) & 0x3f) << 24)
/* bit 23~13 reserved */
#define SD_EN_S18EN			(1 << 12)
/* bit 11 reserved */
#define SD_EN_RESE			(1 << 10)
#define SD_EN_DAT1_S			(1 << 9)
#define SD_EN_CLK_S			(1 << 8)
#define SD_ENABLE			(1 << 7)
#define SD_EN_BSEL			(1 << 6)
/* bit 5~4 reserved */
#define SD_EN_SDIOEN			(1 << 3)
#define SD_EN_DDREN			(1 << 2)
#define SD_EN_DATAWID(x)		(((x) & 0x3) << 0)

/*
 * Register SD_CTL
 */
#define SD_CTL_TOUTEN			(1 << 31)
#define SD_CTL_TOUTCNT(x)		(((x) & 0x7f) << 24)
#define SD_CTL_RDELAY(x)		(((x) & 0xf) << 20)
#define SD_CTL_WDELAY(x)		(((x) & 0xf) << 16)
/* bit 15~14 reserved */
#define SD_CTL_CMDLEN			(1 << 13)
#define SD_CTL_SCC			(1 << 12)
#define SD_CTL_TCN(x)			(((x) & 0xf) << 8)
#define SD_CTL_TS			(1 << 7)
#define SD_CTL_LBE			(1 << 6)
#define SD_CTL_C7EN			(1 << 5)
/* bit 4 reserved */
#define SD_CTL_TM(x)			(((x) & 0xf) << 0)

/*
 * Register SD_STATE
 */
/* bit 31~19 reserved */
#define SD_STATE_DAT1BS			(1 << 18)
#define SD_STATE_SDIOB_P		(1 << 17)
#define SD_STATE_SDIOB_EN		(1 << 16)
#define SD_STATE_TOUTE			(1 << 15)
#define SD_STATE_BAEP			(1 << 14)
/* bit 13 reserved */
#define SD_STATE_MEMRDY			(1 << 12)
#define SD_STATE_CMDS			(1 << 11)
#define SD_STATE_DAT1AS			(1 << 10)
#define SD_STATE_SDIOA_P		(1 << 9)
#define SD_STATE_SDIOA_EN		(1 << 8)
#define SD_STATE_DAT0S			(1 << 7)
#define SD_STATE_TEIE			(1 << 6)
#define SD_STATE_TEI			(1 << 5)
#define SD_STATE_CLNR			(1 << 4)
#define SD_STATE_CLC			(1 << 3)
#define SD_STATE_WC16ER			(1 << 2)
#define SD_STATE_RC16ER			(1 << 1)
#define SD_STATE_CRC7ER			(1 << 0)


/*
 * DMA mode config
 */
#define ATV520X_SDC0WT_DMAMODE		(0x00010202)	/* DDR->FIFO */
#define ATV520X_SDC1WT_DMAMODE		(0x00010203)
#define ATV520X_SDC2WT_DMAMODE		(0x00010204)

#define ATV520X_SDC0RD_DMAMODE		(0x00040802)	/* FIFO->DDR */
#define ATV520X_SDC1RD_DMAMODE		(0x00040803)
#define ATV520X_SDC2RD_DMAMODE		(0x00040804)

/*
 * PAD drive capacity config
 */
#define SD1_DRV_HIGH_MASK		(~(0x3 << 20))	/* sd1 data */
#define SD1_DRV_HIGH_LOW		(0x0 << 20)
#define SD1_DRV_HIGH_MID		(0x1 << 20)
#define SD1_DRV_HIGH_HIGH		(0x2 << 20)

#define SD1_DRV_HIGH_MASK2		(~(0xF) << 12)	/* sd1 cmd, clk */
#define SD1_DRV_HIGH2_LOW		(0x0 << 12)
#define SD1_DRV_HIGH2_MID		(0x5 << 12)
#define SD1_DRV_HIGH2_HIGH		(0xa << 12)

#define SD0_DRV_HIGH_MASK		(~(0x3 << 22))	/* sd0 data */
#define SD0_DRV_HIGH_LOW		(0x0 << 22)
#define SD0_DRV_HIGH_MID		(0x1 << 22)
#define SD0_DRV_HIGH_HIGH		(0x3 << 22)

#define SD0_DRV_HIGH_MASK2		(~(0xF << 16))	/* sd0 cmd, clk */
#define SD0_DRV_HIGH2_LOW		(0x0 << 16)
#define SD0_DRV_HIGH2_MID		(0x5 << 16)
#define SD0_DRV_HIGH2_HIGH		(0xf << 16)

//dump_iomap addr
/*GPIO_MFP_PWM_BASE*/
#define DUMP_MFP_CTL0(mapbase) 		(mapbase+MFP_CTL0-GPIO_MFP_PWM_BASE) 
#define DUMP_MFP_CTL1(mapbase) 		(mapbase+MFP_CTL1-GPIO_MFP_PWM_BASE)
#define DUMP_MFP_CTL2(mapbase) 		(mapbase+MFP_CTL2-GPIO_MFP_PWM_BASE)
#define DUMP_MFP_CTL3(mapbase) 		(mapbase+MFP_CTL3-GPIO_MFP_PWM_BASE)
#define DUMP_PAD_DVR0(mapbase) 		(mapbase+PAD_DRV0-GPIO_MFP_PWM_BASE)
#define DUMP_PAD_DVR1(mapbase) 		(mapbase+PAD_DRV1-GPIO_MFP_PWM_BASE)
#define DUMP_PAD_DVR2(mapbase) 		(mapbase+PAD_DRV2-GPIO_MFP_PWM_BASE)
#define DUMP_PAD_PULLCTL0(mapbase)  (mapbase+PAD_PULLCTL0-GPIO_MFP_PWM_BASE)
#define DUMP_PAD_PULLCTL1(mapbase)  (mapbase+PAD_PULLCTL1-GPIO_MFP_PWM_BASE)
#define DUMP_PAD_PULLCTL2(mapbase)  (mapbase+PAD_PULLCTL2-GPIO_MFP_PWM_BASE)
#define DUMP_GPIO_CINEN(mapbase) 	(mapbase+GPIO_CINEN-GPIO_MFP_PWM_BASE)
#define DUMP_GPIO_COUTEN(mapbase) 	(mapbase+GPIO_COUTEN-GPIO_MFP_PWM_BASE)
/*CMU_BASE*/
#define DUMP_CMU_DEVCLKEN0(mapbase) (mapbase+CMU_DEVCLKEN0-CMU_BASE)
#define DUMP_CMU_DEVCLKEN1(mapbase) (mapbase+CMU_DEVCLKEN1-CMU_BASE)
#define DUMP_CMU_DEVPLL(mapbase) 	(mapbase+CMU_DEVPLL-CMU_BASE)
#define DUMP_CMU_NANDPLL(mapbase) 	(mapbase+CMU_NANDPLL-CMU_BASE)
#define DUMP_CMU_CMU_SD0CLK(mapbase) 	(mapbase+CMU_SD0CLK-CMU_BASE)
#define DUMP_CMU_CMU_SD1CLK(mapbase) 	(mapbase+CMU_SD1CLK-CMU_BASE)
#define DUMP_CMU_CMU_SD2CLK(mapbase) 	(mapbase+CMU_SD2CLK-CMU_BASE)


static inline int module_clk_get(struct gl520xmmc_host *host)
{
	switch (host->module_id) {
	case MOD_ID_SD0:
		host->clk = clk_get(NULL, CLKNAME_SD0_CLK);
		break;
	case MOD_ID_SD1:
		host->clk = clk_get(NULL, CLKNAME_SD1_CLK);
		break;
	case MOD_ID_SD2:
		host->clk = clk_get(NULL, CLKNAME_SD2_CLK);
		break;
	default:
		pr_err("error: CLK, Mod not supported\n");
		return -1;
	}

	if (IS_ERR(host->clk)) {
		pr_err("error: SDC[%u], Can not get host clock\n", host->id);
		return -1;
	}

	return 0;
}

static inline int module_clk_set_rate(struct gl520xmmc_host *host,
	unsigned long freq)
{
	unsigned long rate;
	int ret;

	rate = clk_round_rate(host->clk, freq);
	if (rate < 0) {
		pr_err("SDC%d cannot get suitable rate:%lu\n", host->id, rate);
		return -ENXIO;
	}

	ret = clk_set_rate(host->clk, rate);
	if (ret < 0) {
		pr_err("SDC%d Cannot set rate %ld: %d\n", host->id, rate, ret);
		return ret;
	}

	return 0;
}

/* symbols exported from asoc_serial.c */
extern int sdio_uart_pinctrl_request(void);
extern void sdio_uart_pinctrl_free(void);

#endif /* end of _GL520X_MMC_H_ */
