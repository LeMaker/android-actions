#ifndef CLOCK_OWL_H
#define CLOCK_OWL_H

enum owl_cmureg_no {
	R_CMU_COREPLL,
	R_CMU_DEVPLL,
	R_CMU_DDRPLL,
	R_CMU_NANDPLL,
	R_CMU_DISPLAYPLL,
	R_CMU_AUDIOPLL,
	R_CMU_TVOUTPLL,
	R_CMU_BUSCLK,
	R_CMU_SENSORCLK,
	R_CMU_LCDCLK,
	R_CMU_DSICLK,
	R_CMU_CSICLK,
	R_CMU_DECLK,
	R_CMU_BISPCLK,

	R_CMU_BUSCLK1,
	R_CMU_RESERVE2,

	R_CMU_VDECLK,
	R_CMU_VCECLK,

	R_CMU_RESERVE3,

	R_CMU_NANDCCLK,
	R_CMU_SD0CLK,
	R_CMU_SD1CLK,
	R_CMU_SD2CLK,
	R_CMU_UART0CLK,
	R_CMU_UART1CLK,
	R_CMU_UART2CLK,

	R_CMU_PWM4CLK,
	R_CMU_PWM5CLK,
	R_CMU_PWM0CLK,
	R_CMU_PWM1CLK,
	R_CMU_PWM2CLK,
	R_CMU_PWM3CLK,
	R_CMU_USBPLL,
	R_CMU_ETHERNETPLL,
	R_CMU_CVBSPLL,

	R_CMU_LENSCLK,
	R_CMU_GPU3DCLK,

	R_CMU_RESERVE6,

	R_CMU_CORECTL,
	R_CMU_DEVCLKEN0,
	R_CMU_DEVCLKEN1,
	R_CMU_DEVRST0,
	R_CMU_DEVRST1,
	R_CMU_UART3CLK,
	R_CMU_UART4CLK,
	R_CMU_UART5CLK,
	R_CMU_UART6CLK,

	R_CMU_RESERVE7,

	R_CMU_SSCLK,
	R_CMU_DIGTALDEBUG,
	R_CMU_ANALOGDEBUG,
	R_CMU_COREPLLDEBUG,
	R_CMU_DEVPLLDEBUG,
	R_CMU_DDRPLLDEBUG,
	R_CMU_NANDPLLDEBUG,
	R_CMU_DISPLAYPLLDEBUG,
	R_CMU_TVOUTPLLDEBUG,

	R_CMU_RESERVE8,

	R_CMU_DEEPCOLORPLLDEBUG,
	R_CMU_AUDIOPLL_ETHPLLDEBUG,
	R_CMU_CVBSPLLDEBUG,
	R_CMUMAX
};


enum owl_clock_e {
	CLOCK__HOSC,
	CLOCK__IC_32K,
	CLOCK__COREPLL,
	CLOCK__DEVPLL,
	CLOCK__DDRPLL,
	CLOCK__NANDPLL,
	CLOCK__DISPLAYPLL,
	CLOCK__AUDIOPLL,
	CLOCK__TVOUTPLL,
	CLOCK__DEEPCOLORPLL,
	CLOCK__ETHERNETPLL,
	CLOCK__CVBSPLL,
	CLOCK__DEV_CLK,
	CLOCK__DDR_CLK_0,
	CLOCK__DDR_CLK_90,
	CLOCK__DDR_CLK_180,
	CLOCK__DDR_CLK_270,
	CLOCK__DDR_CLK_CH0,
	CLOCK__DDR_CLK_CH1,
	CLOCK__DDR_CLK,
	CLOCK__SPDIF_CLK,
	CLOCK__HDMIA_CLK,
	CLOCK__I2SRX_CLK,
	CLOCK__I2STX_CLK,
	CLOCK__PCM0_CLK,
	CLOCK__PCM1_CLK,
	CLOCK__CLK_CVBSX2,
	CLOCK__CLK_CVBS,
	CLOCK__CVBS_CLK108M,
	CLOCK__CLK_PIXEL,
	CLOCK__CLK_TMDS,
	CLOCK__CLK_TMDS_PHY_P,
	CLOCK__CLK_TMDS_PHY_N,
	CLOCK__L2_NIC_CLK,
	CLOCK__APBDBG_CLK,
	CLOCK__L2_CLK,
	CLOCK__ACP_CLK,
	CLOCK__PERIPH_CLK,
	CLOCK__NIC_DIV_CLK,
	CLOCK__NIC_CLK,
	CLOCK__AHBPREDIV_CLK,
	CLOCK__H_CLK,
	CLOCK__APB30_CLK,
	CLOCK__APB20_CLK,
	CLOCK__AHB_CLK,
	CLOCK__CORE_CLK,
	CLOCK__CPU_CLK,
	CLOCK__SENSOR_CLKOUT0,
	CLOCK__SENSOR_CLKOUT1,
	CLOCK__LCD_CLK,
	CLOCK__LVDS_CLK,
	CLOCK__CKA_LCD_H,
	CLOCK__LCD1_CLK,
	CLOCK__LCD0_CLK,
	CLOCK__DSI_HCLK,
	CLOCK__DSI_HCLK90,
	CLOCK__PRO_CLK,
	CLOCK__PHY_CLK,
	CLOCK__CSI_CLK,
	CLOCK__DE1_CLK,
	CLOCK__DE2_CLK,
	CLOCK__BISP_CLK,
	CLOCK__ISPBP_CLK,
	CLOCK__IMG5_CLK,
	CLOCK__VDE_CLK,
	CLOCK__VCE_CLK,
	CLOCK__NANDC_CLK,
	CLOCK__ECC_CLK,
	CLOCK__PRESD0_CLK,
	CLOCK__PRESD1_CLK,
	CLOCK__PRESD2_CLK,
	CLOCK__SD0_CLK_2X,
	CLOCK__SD1_CLK_2X,
	CLOCK__SD2_CLK_2X,
	CLOCK__SD0_CLK,
	CLOCK__SD1_CLK,
	CLOCK__SD2_CLK,
	CLOCK__UART0_CLK,
	CLOCK__UART1_CLK,
	CLOCK__UART2_CLK,
	CLOCK__UART3_CLK,
	CLOCK__UART4_CLK,
	CLOCK__UART5_CLK,
	CLOCK__UART6_CLK,
	CLOCK__PWM0_CLK,
	CLOCK__PWM1_CLK,
	CLOCK__PWM2_CLK,
	CLOCK__PWM3_CLK,
	CLOCK__PWM4_CLK,
	CLOCK__PWM5_CLK,
	CLOCK__RMII_REF_CLK,
	CLOCK__I2C0_CLK,
	CLOCK__I2C1_CLK,
	CLOCK__I2C2_CLK,
	CLOCK__I2C3_CLK,
	CLOCK__25M_CLK,
	CLOCK__LENS_CLK,
	CLOCK__HDMI24M,
	CLOCK__TIMER_CLK,
	CLOCK__SS_CLK,
	CLOCK__SPS_CLK,
	CLOCK__IRC_CLK,
	CLOCK__TV24M,
	CLOCK__MIPI24M,
	CLOCK__LENS24M,
	CLOCK__GPU3D_SYSCLK,
	CLOCK__GPU3D_HYDCLK,
	CLOCK__GPU3D_NIC_MEMCLK,
	CLOCK__GPU3D_CORECLK,
	CLOCK__MAX
};


struct owl_clkreq {
	int reg_no;
	unsigned long *reg_hw;
	unsigned long int mask;
	int offset;
};



enum owl_divtype_e {
	DIV_T_NATURE = 4,
	DIV_T_EXP = 8,
	DIV_T_EXP_D2,
	DIV_T_TABLE = 16,
	DIV_T_SEQ = 32,
	DIV_T_SEQ_D2,
	DIV_T_COMP = 64
};

struct owl_refertab {
	int div[9];
	int flac;
};

struct owl_seqtab {
	int length;
	int seq[9];
};

struct owl_section {
	int type;
	int range_from;
	int range_to;
	union {
		struct owl_seqtab *seq;
		struct owl_refertab *tab;
	} ext;
};

struct owl_compdiv {
	struct owl_section sections[2];
};

struct owl_div {
	int type;
	int range_from;
	int range_to;
	union {
		struct owl_refertab *tab;
		struct owl_seqtab *seq;
		struct owl_compdiv *comp;
	} ext;
	struct owl_clkreq *reg;
};



enum owl_clocktype_e {
	TYPE_STATIC, TYPE_PLL, TYPE_DYNAMIC
};

#define MAX_SRC_SEL 5

struct owl_clocknode {
	char name[32];
	int id;
	int type;

	int source_av[MAX_SRC_SEL];
	int source_lim;
	int source_sel;
	int putaway_enabled;
	int putaway_sel;
	int putaway_divsel;
	int putback_divsel;

	struct owl_clkreq *reg_srcsel;
	int clock_en;

	int multipler;
	int divider;
	int harddivider;
	unsigned long frequency;
	int changed;

	struct owl_div *actdiv;
	int divsel;

	struct owl_clocknode *parent;
	struct owl_clocknode *prev;
	struct owl_clocknode *next;
	struct owl_clocknode *sub;
};





enum owl_plltype_e {
	PLL_T_STEP,
	PLL_T_D4DYN,
	PLL_T_FREQ
};

struct owl_pll {
	int type;
	int range_from;
	int range_to;
	union {
		struct  {
			int step;
			int offset;
		} step;
		int freqtab[8];
	} freq;
	int sel;
	int delay;
	struct owl_clkreq *reg_pllen;
	struct owl_clkreq *reg_pllfreq;
	struct owl_clkreq *reg_plllock;
};

enum owl_pll_e {
	PLL__COREPLL,
	PLL__DEVPLL,
	PLL__DDRPLL,
	PLL__NANDPLL,
	PLL__DISPLAYPLL,
	PLL__AUDIOPLL,
	PLL__TVOUTPLL,
	PLL__DEEPCOLORPLL,
	PLL__ETHERNETPLL,
	PLL__CVBSPLL,
	PLL__MAX
};



struct owl_cmumod {
	char modname[32];
	struct owl_clkreq *reg_devclken;
	struct owl_clkreq *reg_devrst;
};

enum owl_cmumod_e {
	MOD__ROOT,
	MOD__GPU3D,
	MOD__SHARESRAM,
	MOD__HDCP2X,
	MOD__VCE,
	MOD__VDE,
	MOD__PCM0,
	MOD__SPDIF,
	MOD__HDMIA,
	MOD__I2SRX,
	MOD__I2STX,
	MOD__GPIO,
	MOD__KEY,
	MOD__LENS,
	MOD__BISP,
	MOD__CSI,
	MOD__DSI,
	MOD__LVDS,
	MOD__LCD1,
	MOD__LCD0,
	MOD__DE,
	MOD__SD2,
	MOD__SD1,
	MOD__SD0,
	MOD__NANDC,
	MOD__DDRCH0,
	MOD__NOR,
	MOD__DMAC,
	MOD__DDRCH1,
	MOD__I2C3,
	MOD__I2C2,
	MOD__TIMER,
	MOD__PWM5,
	MOD__PWM4,
	MOD__PWM3,
	MOD__PWM2,
	MOD__PWM1,
	MOD__PWM0,
	MOD__ETHERNET,
	MOD__UART5,
	MOD__UART4,
	MOD__UART3,
	MOD__UART6,
	MOD__PCM1,
	MOD__I2C1,
	MOD__I2C0,
	MOD__SPI3,
	MOD__SPI2,
	MOD__SPI1,
	MOD__SPI0,
	MOD__IRC,
	MOD__UART2,
	MOD__UART1,
	MOD__UART0,
	MOD__HDMI,
	MOD__SS,
	MOD__TV24M,
	MOD__CVBS_CLK108M,
	MOD__TVOUT,
	MOD__MAX_IN_CLK,

	MOD__PERIPHRESET = MOD__MAX_IN_CLK,
	MOD__NIC301,
	MOD__AUDIO,
	MOD__LCD,
	MOD__DDR,
	MOD__NORIF,
	MOD__DBG3RESET,
	MOD__DBG2RESET,
	MOD__DBG1RESET,
	MOD__DBG0RESET,
	MOD__WD3RESET,
	MOD__WD2RESET,
	MOD__WD1RESET,
	MOD__WD0RESET,
	MOD__CHIPID,
	MOD__USB3,
	MOD__HDCP2Tx,
	MOD__USB2_0,
	MOD__USB2_1,
	MOD__MAX
};



struct owl_clk_foo {
	struct clk_hw hw;
	int clock;
};


#define KILO	1000
#define MEGA	(1000*KILO)

#define PLLDELAY     50

#define OWL_PLLDELAY(x)	((x)+(x)/4)
#define OWL_COREPLL_DELAY		 OWL_PLLDELAY(120)
#define OWL_DDRPLL_DELAY		 OWL_PLLDELAY(50)
#define OWL_DEVPLL_DELAY		 OWL_PLLDELAY(22)
#define OWL_NANDPLL_DELAY		 OWL_PLLDELAY(35)
#define OWL_AUDIOPLL_DELAY		 OWL_PLLDELAY(80)
#define OWL_DISPLAYPLL_DELAY	 OWL_PLLDELAY(45)
#define OWL_TVOUTPLL_DELAY		 OWL_PLLDELAY(25)
#define OWL_DEEPCOLORPLL_DELAY	 OWL_PLLDELAY(20)
#define OWL_ETHERNETPLL_DELAY	 OWL_PLLDELAY(20)
#define OWL_CVBSPLL_DELAY		 OWL_PLLDELAY(25)

#define to_clk_foo(_hw)		container_of(_hw, struct owl_clk_foo, hw)

#define CMU_BITMAP(reg, _mask, _offset) \
	{\
		.reg_no = R_##reg, \
		.reg_hw = (unsigned long *)reg, \
		.mask = _mask, \
		.offset = _offset,\
	}

int read_clkreg_val(struct owl_clkreq *reg);
void write_clkreg_val(struct owl_clkreq *reg, int val);

int owl_pllsub_set_putaway(int clock, int source);
unsigned long owl_get_putaway_parent_rate(struct clk *clk);
unsigned long owl_getparent_newrate(struct clk *clk);
int owl_getdivider_index(struct clk *clk, int divexp);

int owl_set_putaway_divsel(struct clk *clk, int tmp_divsel, int new_divsel);
void owl_update_notify_newrate(struct clk *clk, unsigned long newrate);
int owl_pll_in_change(void);

#endif
