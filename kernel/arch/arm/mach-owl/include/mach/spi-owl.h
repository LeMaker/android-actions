#ifndef __SPI_OWL_H__
#define __SPI_OWL_H__

struct owl_gpio{
	int gpio_group;
	unsigned int gpio_pin;
};

struct owl_spi_pdata {
	u32	source_clk;		/* source clk type */

	void (*init_hw)(void);
	void (*free_hw)(void);
	struct owl_gpio *owl_spi_cs_table;
	u16 max_chipselect;

	u8 enable_dma;
};

#define SPIx_CTL_SDT(x)                    (((x) & 0x7) << 29)
#define SPIx_CTL_BM                        (1 << 28)
#define SPIx_CTL_GM                        (1 << 27)

#define SPIx_CTL_CEB                        (1 << 26)
#define SPIx_CTL_RANEN                     (1 << 24)
#define SPIx_CTL_RDIC(x)                    (((x) & 0x3) << 22)
#define SPIx_CTL_TDIC(x)                    (((x) & 0x3) << 20)
#define SPIx_CTL_TWME                       (1 << 19)
#define SPIx_CTL_EN                         (1 << 18)
#define SPIx_CTL_RWC(x)                     (((x) & 0x3) << 16)
#define SPIx_CTL_DTS                        (1 << 15)
#define SPIx_CTL_SSATEN                     (1 << 14)
#define SPIx_CTL_DM(x)                      (((x) & 0x3) << 12)
#define SPIx_CTL_LBT                        (1 << 11)
#define SPIx_CTL_MS                         (1 << 10)
#define SPIx_CTL_DAWS(x)                    (((x) & 0x3) << 8)
#define SPIx_CTL_CPOS(x)                    (((x) & 0x3) << 6)
#define SPIx_CTL_LMFS                       (1 << 5)
#define SPIx_CTL_SSCO                       (1 << 4)
#define SPIx_CTL_TIEN                       (1 << 3)
#define SPIx_CTL_RIEN                       (1 << 2)
#define SPIx_CTL_TDEN                       (1 << 1)
#define SPIx_CTL_RDEN                       (1 << 0)
/******************************************************************************/
/*SPIx_CLKDIV*/
/*bit 10-31 Reserved*/
#define SPIx_CLKDIV_CLKDIV(x)               (((x) & 0x3FF) << 0)
/******************************************************************************/
/*SPIx_STAT*/
/*bit 10-31 Reserved*/
#define SPIx_STAT_TFEM                      (1 << 9)
#define SPIx_STAT_RFFU                      (1 << 8)
#define SPIx_STAT_TFFU                      (1 << 7)
#define SPIx_STAT_RFEM                      (1 << 6)
#define SPIx_STAT_TFER                      (1 << 5)
#define SPIx_STAT_RFER                      (1 << 4)
#define SPIx_STAT_BEB                       (1 << 3)
#define SPIx_STAT_TCOM                      (1 << 2)
#define SPIx_STAT_TIP                       (1 << 1)
#define SPIx_STAT_PIP                       (1 << 0)
#endif
