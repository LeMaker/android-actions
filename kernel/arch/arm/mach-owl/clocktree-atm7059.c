#include "clocktree-owl.h"

static unsigned long rvregs[R_CMUMAX] = {
    [R_CMU_COREPLL]                 =	0x00000064,
    [R_CMU_DEVPLL]                  =	0x00001064,
    [R_CMU_DDRPLL]                  =	0x00000019,
    [R_CMU_NANDPLL]                 =	0x00000032,
    [R_CMU_DISPLAYPLL]              =	0x00000064,
    [R_CMU_AUDIOPLL]                =	0x10000001,
    [R_CMU_TVOUTPLL]                =	0x00250000,
    [R_CMU_BUSCLK]                  =	0x3df60012,
    [R_CMU_SENSORCLK]               =	0x00000000,
    [R_CMU_LCDCLK]                  =	0x00000100,
    [R_CMU_DSICLK]                  =	0x00000000,
    [R_CMU_CSICLK]                  =	0x00000000,
    [R_CMU_DECLK]                   =	0x00000000,
    [R_CMU_BISPCLK]                 =	0x00000000,
    [R_CMU_VDECLK]                  =	0x00000000,
    [R_CMU_VCECLK]                  =	0x00000000,
    [R_CMU_NANDCCLK]                =	0x00000001,
    [R_CMU_SD0CLK]                  =	0x00000000,
    [R_CMU_SD1CLK]                  =	0x00000000,
    [R_CMU_SD2CLK]                  =	0x00000000,
    [R_CMU_UART0CLK]                =	0x00000000,
    [R_CMU_UART1CLK]                =	0x00000000,
    [R_CMU_UART2CLK]                =	0x00000000,
    [R_CMU_PWM0CLK]                 =	0x00000000,
    [R_CMU_PWM1CLK]                 =	0x00000000,
    [R_CMU_PWM2CLK]                 =	0x00000000,
    [R_CMU_PWM3CLK]                 =	0x00000000,
    [R_CMU_ETHERNETPLL]             =	0x00000000,
    [R_CMU_CVBSPLL]             =	0x00000000,
    [R_CMU_LENSCLK]                 =	0x00000000,
    [R_CMU_GPU3DCLK]                =	0x00000000,
    [R_CMU_SSCLK]                   =	0x00000000,
    [R_CMU_UART3CLK]                =	0x00000000,
    [R_CMU_UART4CLK]                =	0x00000000,
    [R_CMU_UART5CLK]                =	0x00000000,
    [R_CMU_UART6CLK]                =	0x00000000,
    [R_CMU_COREPLLDEBUG]            =	0x00000000,
    [R_CMU_DEVPLLDEBUG]             =	0x00000000,
    [R_CMU_DDRPLLDEBUG]             =	0x00000000,
    [R_CMU_NANDPLLDEBUG]            =	0x00000000,
    [R_CMU_DISPLAYPLLDEBUG]         =	0x00000000,
    [R_CMU_TVOUTPLLDEBUG]           =	0x00000000,
    [R_CMU_DEEPCOLORPLLDEBUG]       =	0x00000000,
    [R_CMU_AUDIOPLL_ETHPLLDEBUG]    =	0x00000000,
};

static struct owl_clkreq busbit_DIVEN               = BITMAP(CMU_BUSCLK,                0x80000000, 31);

static struct owl_clkreq divbit_SPDIF_CLK           = BITMAP(CMU_AUDIOPLL,              0xf0000000, 28);
static struct owl_clkreq divbit_HDMIA_CLK           = BITMAP(CMU_AUDIOPLL,              0x0f000000, 24);
static struct owl_clkreq divbit_I2SRX_CLK           = BITMAP(CMU_AUDIOPLL,              0x00f00000, 20);
static struct owl_clkreq divbit_I2STX_CLK           = BITMAP(CMU_AUDIOPLL,              0x000f0000, 16);
static struct owl_clkreq divbit_APBDBG_CLK          = BITMAP(CMU_BUSCLK,                0x1c000000, 26);
static struct owl_clkreq divbit_ACP_CLK             = BITMAP(CMU_BUSCLK,                0x01800000, 23);
static struct owl_clkreq divbit_PERIPH_CLK          = BITMAP(CMU_BUSCLK,                0x00700000, 20);
static struct owl_clkreq divbit_NIC_DIV_CLK         = BITMAP(CMU_BUSCLK,                0x000c0000, 18);
static struct owl_clkreq divbit_NIC_CLK             = BITMAP(CMU_BUSCLK,                0x00030000, 16);
static struct owl_clkreq divbit_APB30_CLK           = BITMAP(CMU_BUSCLK,                0x0000c000, 14);
static struct owl_clkreq divbit_APB20_CLK           = BITMAP(CMU_BUSCLK,                0x00000700, 8);
static struct owl_clkreq divbit_AHBPREDIV_CLK       = BITMAP(CMU_BUSCLK1,               0x00003000, 12);
static struct owl_clkreq divbit_H_CLK               = BITMAP(CMU_BUSCLK1,               0x0000000c, 2);
static struct owl_clkreq divbit_SENSOR_CLKOUT1      = BITMAP(CMU_SENSORCLK,             0x00000f00, 8);
static struct owl_clkreq divbit_SENSOR_CLKOUT0      = BITMAP(CMU_SENSORCLK,             0x0000000f, 0);
static struct owl_clkreq divbit_LCD_CLK             = BITMAP(CMU_LCDCLK,                0x00000100, 8);
static struct owl_clkreq divbit_LCD1_CLK            = BITMAP(CMU_LCDCLK,                0x000000f0, 4);
static struct owl_clkreq divbit_LCD0_CLK            = BITMAP(CMU_LCDCLK,                0x0000000f, 0);
static struct owl_clkreq divbit_PRO_CLK             = BITMAP(CMU_DSICLK,                0x00000030, 4);
static struct owl_clkreq divbit_DSI_HCLK            = BITMAP(CMU_DSICLK,                0x00000003, 0);
static struct owl_clkreq divbit_CSI_CLK             = BITMAP(CMU_CSICLK,                0x0000000f, 0);
static struct owl_clkreq divbit_DE2_CLK             = BITMAP(CMU_DECLK,                 0x000000f0, 4);
static struct owl_clkreq divbit_DE1_CLK             = BITMAP(CMU_DECLK,                 0x0000000f, 0);
static struct owl_clkreq divbit_BISP_CLK            = BITMAP(CMU_BISPCLK,               0x0000000f, 0);
static struct owl_clkreq divbit_VDE_CLK             = BITMAP(CMU_VDECLK,                0x00000007, 0);
static struct owl_clkreq divbit_VCE_CLK             = BITMAP(CMU_VCECLK,                0x00000007, 0);
static struct owl_clkreq divbit_ECC_CLK             = BITMAP(CMU_NANDCCLK,              0x00000070, 4);
static struct owl_clkreq divbit_NANDC_CLK           = BITMAP(CMU_NANDCCLK,              0x00000007, 0);
static struct owl_clkreq divbit_PRESD0_CLK          = BITMAP(CMU_SD0CLK,                0x0000001f, 0);
static struct owl_clkreq divbit_SD0_CLK_2X          = BITMAP(CMU_SD0CLK,                0x00000100, 8);
static struct owl_clkreq divbit_PRESD1_CLK          = BITMAP(CMU_SD1CLK,                0x0000001f, 0);
static struct owl_clkreq divbit_SD1_CLK_2X          = BITMAP(CMU_SD1CLK,                0x00000100, 8);
static struct owl_clkreq divbit_PRESD2_CLK          = BITMAP(CMU_SD2CLK,                0x0000001f, 0);
static struct owl_clkreq divbit_SD2_CLK_2X          = BITMAP(CMU_SD2CLK,                0x00000100, 8);
static struct owl_clkreq divbit_UART0_CLK           = BITMAP(CMU_UART0CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_UART1_CLK           = BITMAP(CMU_UART1CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_UART2_CLK           = BITMAP(CMU_UART2CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_PWM0_CLK            = BITMAP(CMU_PWM0CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_PWM1_CLK            = BITMAP(CMU_PWM1CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_PWM2_CLK            = BITMAP(CMU_PWM2CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_PWM3_CLK            = BITMAP(CMU_PWM3CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_PWM4_CLK            = BITMAP(CMU_PWM4CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_PWM5_CLK            = BITMAP(CMU_PWM5CLK,               0x000003ff, 0);
static struct owl_clkreq divbit_RMII_REF_CLK        = BITMAP(CMU_ETHERNETPLL,           0x00000002, 1);
static struct owl_clkreq divbit_LENS_CLK            = BITMAP(CMU_LENSCLK,               0x00000007, 0);
static struct owl_clkreq divbit_GPU3D_SYSCLK        = BITMAP(CMU_GPU3DCLK,              0x07000000, 24);
static struct owl_clkreq divbit_GPU3D_NIC_MEMCLK    = BITMAP(CMU_GPU3DCLK,              0x00070000, 16);
static struct owl_clkreq divbit_GPU3D_HYDCLK        = BITMAP(CMU_GPU3DCLK,              0x00000700, 8);
static struct owl_clkreq divbit_GPU3D_CORECLK       = BITMAP(CMU_GPU3DCLK,              0x00000007, 0);
static struct owl_clkreq divbit_SS_CLK              = BITMAP(CMU_SSCLK,                 0x000003ff, 0);
static struct owl_clkreq divbit_UART3_CLK           = BITMAP(CMU_UART3CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_UART4_CLK           = BITMAP(CMU_UART4CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_UART5_CLK           = BITMAP(CMU_UART5CLK,              0x000001ff, 0);
static struct owl_clkreq divbit_UART6_CLK           = BITMAP(CMU_UART6CLK,              0x000001ff, 0);
                                                                                        
                                                                                        
static struct owl_clkreq selbit_NIC_CLK             = BITMAP(CMU_BUSCLK,                0x00000070, 4);
static struct owl_clkreq selbit_AHBPREDIV_CLK       = BITMAP(CMU_BUSCLK1,               0x00000700, 8);
static struct owl_clkreq selbit_DEV_CLK             = BITMAP(CMU_DEVPLL,                0x00001000, 12);
static struct owl_clkreq selbit_DDR_CLK_CH1         = BITMAP(CMU_DDRPLL,                0x00000600, 9);
static struct owl_clkreq selbit_CORE_CLK            = BITMAP(CMU_BUSCLK,                0x00000003, 0);
static struct owl_clkreq selbit_SENSOR_CLKOUT0      = BITMAP(CMU_SENSORCLK,             0x00000010, 4);
static struct owl_clkreq selbit_SENSOR_CLKOUT1      = BITMAP(CMU_SENSORCLK,             0x00000010, 4);
static struct owl_clkreq selbit_LCD_CLK             = BITMAP(CMU_LCDCLK,                0x00003000, 12);
static struct owl_clkreq selbit_CSI_CLK             = BITMAP(CMU_CSICLK,                0x00000010, 4);
static struct owl_clkreq selbit_IMG5_CLK            = BITMAP(CMU_DECLK,                 0x00020000, 17);
static struct owl_clkreq selbit_DE1_CLK             = BITMAP(CMU_DECLK,                 0x00001000, 12);
static struct owl_clkreq selbit_DE2_CLK             = BITMAP(CMU_DECLK,                 0x00001000, 12);
static struct owl_clkreq selbit_BISP_CLK            = BITMAP(CMU_BISPCLK,               0x00000010, 4);
static struct owl_clkreq selbit_VDE_CLK             = BITMAP(CMU_VDECLK,                0x00000030, 4);
static struct owl_clkreq selbit_VCE_CLK             = BITMAP(CMU_VCECLK,                0x00000030, 4);
static struct owl_clkreq selbit_NANDC_CLK           = BITMAP(CMU_NANDCCLK,              0x00000300, 8);
static struct owl_clkreq selbit_ECC_CLK             = BITMAP(CMU_NANDCCLK,              0x00000300, 8);
static struct owl_clkreq selbit_PRESD0_CLK          = BITMAP(CMU_SD0CLK,                0x00000200, 9);
static struct owl_clkreq selbit_PRESD1_CLK          = BITMAP(CMU_SD1CLK,                0x00000200, 9);
static struct owl_clkreq selbit_PRESD2_CLK          = BITMAP(CMU_SD2CLK,                0x00000200, 9);
static struct owl_clkreq selbit_UART0_CLK           = BITMAP(CMU_UART0CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART1_CLK           = BITMAP(CMU_UART1CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART2_CLK           = BITMAP(CMU_UART2CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART3_CLK           = BITMAP(CMU_UART3CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART4_CLK           = BITMAP(CMU_UART4CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART5_CLK           = BITMAP(CMU_UART5CLK,              0x00010000, 16);
static struct owl_clkreq selbit_UART6_CLK           = BITMAP(CMU_UART6CLK,              0x00010000, 16);
static struct owl_clkreq selbit_PWM0_CLK            = BITMAP(CMU_PWM0CLK,               0x00001000, 12);
static struct owl_clkreq selbit_PWM1_CLK            = BITMAP(CMU_PWM1CLK,               0x00001000, 12);
static struct owl_clkreq selbit_PWM2_CLK            = BITMAP(CMU_PWM2CLK,               0x00001000, 12);
static struct owl_clkreq selbit_PWM3_CLK            = BITMAP(CMU_PWM3CLK,               0x00001000, 12);
static struct owl_clkreq selbit_PWM4_CLK            = BITMAP(CMU_PWM4CLK,               0x00001000, 12);
static struct owl_clkreq selbit_PWM5_CLK            = BITMAP(CMU_PWM5CLK,               0x00001000, 12);
static struct owl_clkreq selbit_GPU3D_SYSCLK        = BITMAP(CMU_GPU3DCLK,              0x70000000, 28);
static struct owl_clkreq selbit_GPU3D_NIC_MEMCLK    = BITMAP(CMU_GPU3DCLK,              0x00700000, 20);
static struct owl_clkreq selbit_GPU3D_HYDCLK        = BITMAP(CMU_GPU3DCLK,              0x00007000, 12);
static struct owl_clkreq selbit_GPU3D_CORECLK       = BITMAP(CMU_GPU3DCLK,              0x00000070, 4);
                                                                                        
                                                                                        
static struct owl_clkreq pllbit_COREPLLEN           = BITMAP(CMU_COREPLL,               0x00000200, 9);
static struct owl_clkreq pllbit_COREPLLFREQ         = BITMAP(CMU_COREPLL,               0x000000ff, 0);
static struct owl_clkreq pllbit_DEVPLLEN            = BITMAP(CMU_DEVPLL,                0x00000100, 8);
static struct owl_clkreq pllbit_DEVPLLFREQ          = BITMAP(CMU_DEVPLL,                0x0000007f, 0);
static struct owl_clkreq pllbit_DDRPLLEN            = BITMAP(CMU_DDRPLL,                0x00000100, 8);
static struct owl_clkreq pllbit_DDRPLLFREQ          = BITMAP(CMU_DDRPLL,                0x000000ff, 0);
static struct owl_clkreq pllbit_NANDPLLEN           = BITMAP(CMU_NANDPLL,               0x00000100, 8);
static struct owl_clkreq pllbit_NANDPLLFREQ         = BITMAP(CMU_NANDPLL,               0x0000007f, 0);
static struct owl_clkreq pllbit_DISPLAYPLLEN        = BITMAP(CMU_DISPLAYPLL,            0x00000100, 8);
static struct owl_clkreq pllbit_DISPALYPLLFREQ      = BITMAP(CMU_DISPLAYPLL,            0x000000ff, 0);
static struct owl_clkreq pllbit_AUDIOPLLEN          = BITMAP(CMU_AUDIOPLL,              0x00000010, 4);
static struct owl_clkreq pllbit_AUDIOPLLFREQ_SEL    = BITMAP(CMU_AUDIOPLL,              0x00000001, 0);
static struct owl_clkreq pllbit_TVOUTPLLEN          = BITMAP(CMU_TVOUTPLL,              0x00000008, 3);
static struct owl_clkreq pllbit_TVOUTPLLFREQ_SEL    = BITMAP(CMU_TVOUTPLL,              0x00070000, 16);
static struct owl_clkreq pllbit_ENTRNETPLL_EN       = BITMAP(CMU_ETHERNETPLL,           0x00000001, 0);
static struct owl_clkreq pllbit_CVBSPLL_EN       = BITMAP(CMU_CVBSPLL,           0x00000100, 8);
static struct owl_clkreq pllbit_CVBSPLLREQ      = BITMAP(CMU_CVBSPLL,            0x000000ff, 0);
static struct owl_clkreq pllbit_COREPLLLOCK         = BITMAP(CMU_COREPLLDEBUG,          0x00000800, 11);
static struct owl_clkreq pllbit_DEVPLLLOCK          = BITMAP(CMU_DEVPLLDEBUG,           0x00000800, 11);
static struct owl_clkreq pllbit_DDRPLLLOCK          = BITMAP(CMU_DDRPLLDEBUG,           0x80000000, 31);
static struct owl_clkreq pllbit_NANDPLLLOCK         = BITMAP(CMU_NANDPLLDEBUG,          0x80000000, 31);
static struct owl_clkreq pllbit_DISPLAYPLLLOCK      = BITMAP(CMU_DISPLAYPLLDEBUG,       0x80000000, 31);
static struct owl_clkreq pllbit_TVOUTPLLLOCK        = BITMAP(CMU_TVOUTPLLDEBUG,         0x00004000, 14);
static struct owl_clkreq pllbit_ETHERNETPLLLOCK     = BITMAP(CMU_AUDIOPLL_ETHPLLDEBUG,  0x00800000, 23);
static struct owl_clkreq pllbit_AUDIOPLLLOCK        = BITMAP(CMU_AUDIOPLL_ETHPLLDEBUG,  0x00000200, 9);
static struct owl_clkreq pllbit_CVBSPLLLOCK        = BITMAP(CMU_CVBSPLLDEBUG,  0x00000200, 9);


static struct owl_clkreq enablebit_HOSC             = BITMAP(CMU_COREPLL,               0x00000100, 8);
static struct owl_clkreq enablebit_MODULE_GPU3D     = BITMAP(CMU_DEVCLKEN0,             0x40000000, 30);
static struct owl_clkreq enablebit_MODULE_SHARESRAM = BITMAP(CMU_DEVCLKEN0,             0x10000000, 28);
static struct owl_clkreq enablebit_MODULE_HDCP2X    = BITMAP(CMU_DEVCLKEN0,             0x08000000, 27);
static struct owl_clkreq enablebit_MODULE_VCE       = BITMAP(CMU_DEVCLKEN0,             0x04000000, 26);
static struct owl_clkreq enablebit_MODULE_VDE       = BITMAP(CMU_DEVCLKEN0,             0x02000000, 25);
static struct owl_clkreq enablebit_MODULE_PCM0      = BITMAP(CMU_DEVCLKEN0,             0x01000000, 24);
static struct owl_clkreq enablebit_MODULE_SPDIF     = BITMAP(CMU_DEVCLKEN0,             0x00800000, 23);
static struct owl_clkreq enablebit_MODULE_HDMIA     = BITMAP(CMU_DEVCLKEN0,             0x00400000, 22);
static struct owl_clkreq enablebit_MODULE_I2SRX     = BITMAP(CMU_DEVCLKEN0,             0x00200000, 21);
static struct owl_clkreq enablebit_MODULE_I2STX     = BITMAP(CMU_DEVCLKEN0,             0x00100000, 20);
static struct owl_clkreq enablebit_MODULE_GPIO      = BITMAP(CMU_DEVCLKEN0,             0x00040000, 18);
static struct owl_clkreq enablebit_MODULE_KEY       = BITMAP(CMU_DEVCLKEN0,             0x00020000, 17);
static struct owl_clkreq enablebit_MODULE_LENS      = BITMAP(CMU_DEVCLKEN0,             0x00010000, 16);
static struct owl_clkreq enablebit_MODULE_BISP      = BITMAP(CMU_DEVCLKEN0,             0x00004000, 14);
static struct owl_clkreq enablebit_MODULE_CSI       = BITMAP(CMU_DEVCLKEN0,             0x00002000, 13);
static struct owl_clkreq enablebit_MODULE_DSI       = BITMAP(CMU_DEVCLKEN0,             0x00001000, 12);
static struct owl_clkreq enablebit_MODULE_LVDS      = BITMAP(CMU_DEVCLKEN0,             0x00000800, 11);
static struct owl_clkreq enablebit_MODULE_LCD1      = BITMAP(CMU_DEVCLKEN0,             0x00000400, 10);
static struct owl_clkreq enablebit_MODULE_LCD0      = BITMAP(CMU_DEVCLKEN0,             0x00000200, 9);
static struct owl_clkreq enablebit_MODULE_DE        = BITMAP(CMU_DEVCLKEN0,             0x00000100, 8);
static struct owl_clkreq enablebit_MODULE_SD2       = BITMAP(CMU_DEVCLKEN0,             0x00000080, 7);
static struct owl_clkreq enablebit_MODULE_SD1       = BITMAP(CMU_DEVCLKEN0,             0x00000040, 6);
static struct owl_clkreq enablebit_MODULE_SD0       = BITMAP(CMU_DEVCLKEN0,             0x00000020, 5);
static struct owl_clkreq enablebit_MODULE_NANDC     = BITMAP(CMU_DEVCLKEN0,             0x00000010, 4);
static struct owl_clkreq enablebit_MODULE_DDRCH0    = BITMAP(CMU_DEVCLKEN0,             0x00000008, 3);
static struct owl_clkreq enablebit_MODULE_NOR       = BITMAP(CMU_DEVCLKEN0,             0x00000004, 2);
static struct owl_clkreq enablebit_MODULE_DMAC      = BITMAP(CMU_DEVCLKEN0,             0x00000002, 1);
static struct owl_clkreq enablebit_MODULE_DDRCH1    = BITMAP(CMU_DEVCLKEN0,             0x00000001, 0);
static struct owl_clkreq enablebit_MODULE_I2C3      = BITMAP(CMU_DEVCLKEN1,             0x80000000, 31);
static struct owl_clkreq enablebit_MODULE_I2C2      = BITMAP(CMU_DEVCLKEN1,             0x40000000, 30);
static struct owl_clkreq enablebit_MODULE_TIMER     = BITMAP(CMU_DEVCLKEN1,             0x08000000, 27);
static struct owl_clkreq enablebit_MODULE_PWM5      = BITMAP(CMU_DEVCLKEN0,             0x00000001, 0);
static struct owl_clkreq enablebit_MODULE_PWM4      = BITMAP(CMU_DEVCLKEN0,             0x00000800, 11);
static struct owl_clkreq enablebit_MODULE_PWM3      = BITMAP(CMU_DEVCLKEN1,             0x04000000, 26);
static struct owl_clkreq enablebit_MODULE_PWM2      = BITMAP(CMU_DEVCLKEN1,             0x02000000, 25);
static struct owl_clkreq enablebit_MODULE_PWM1      = BITMAP(CMU_DEVCLKEN1,             0x01000000, 24);
static struct owl_clkreq enablebit_MODULE_PWM0      = BITMAP(CMU_DEVCLKEN1,             0x00800000, 23);
static struct owl_clkreq enablebit_MODULE_ETHERNET  = BITMAP(CMU_DEVCLKEN1,             0x00400000, 22);
static struct owl_clkreq enablebit_MODULE_UART5     = BITMAP(CMU_DEVCLKEN1,             0x00200000, 21);
static struct owl_clkreq enablebit_MODULE_UART4     = BITMAP(CMU_DEVCLKEN1,             0x00100000, 20);
static struct owl_clkreq enablebit_MODULE_UART3     = BITMAP(CMU_DEVCLKEN1,             0x00080000, 19);
static struct owl_clkreq enablebit_MODULE_UART6     = BITMAP(CMU_DEVCLKEN1,             0x00040000, 18);
static struct owl_clkreq enablebit_MODULE_PCM1      = BITMAP(CMU_DEVCLKEN1,             0x00010000, 16);
static struct owl_clkreq enablebit_MODULE_I2C1      = BITMAP(CMU_DEVCLKEN1,             0x00008000, 15);
static struct owl_clkreq enablebit_MODULE_I2C0      = BITMAP(CMU_DEVCLKEN1,             0x00004000, 14);
static struct owl_clkreq enablebit_MODULE_SPI3      = BITMAP(CMU_DEVCLKEN1,             0x00002000, 13);
static struct owl_clkreq enablebit_MODULE_SPI2      = BITMAP(CMU_DEVCLKEN1,             0x00001000, 12);
static struct owl_clkreq enablebit_MODULE_SPI1      = BITMAP(CMU_DEVCLKEN1,             0x00000800, 11);
static struct owl_clkreq enablebit_MODULE_SPI0      = BITMAP(CMU_DEVCLKEN1,             0x00000400, 10);
static struct owl_clkreq enablebit_MODULE_IRC       = BITMAP(CMU_DEVCLKEN1,             0x00000200, 9);
static struct owl_clkreq enablebit_MODULE_UART2     = BITMAP(CMU_DEVCLKEN1,             0x00000100, 8);
static struct owl_clkreq enablebit_MODULE_UART1     = BITMAP(CMU_DEVCLKEN1,             0x00000080, 7);
static struct owl_clkreq enablebit_MODULE_UART0     = BITMAP(CMU_DEVCLKEN1,             0x00000040, 6);
static struct owl_clkreq enablebit_MODULE_HDMI      = BITMAP(CMU_DEVCLKEN1,             0x00000008, 3);
static struct owl_clkreq enablebit_MODULE_SS        = BITMAP(CMU_DEVCLKEN1,             0x00000004, 2);
static struct owl_clkreq enablebit_MODULE_TV24M     = BITMAP(CMU_TVOUTPLL,              0x00800000, 23);
static struct owl_clkreq enablebit_MODULE_CVBS_CLK108M     = BITMAP(CMU_CVBSPLL,              0x00000200, 9);
static struct owl_clkreq enablebit_MODULE_TVOUT     = BITMAP(CMU_DEVCLKEN1,             0x00000001, 0);

static struct owl_clkreq resetbit_MODULE_PERIPHRESET       = BITMAP(CMU_DEVRST0,               0x08000000, 27);
static struct owl_clkreq resetbit_MODULE_LENS              = BITMAP(CMU_DEVRST0,               0x04000000, 26);
static struct owl_clkreq resetbit_MODULE_NIC301            = BITMAP(CMU_DEVRST0,               0x00800000, 23);
static struct owl_clkreq resetbit_MODULE_GPU3D             = BITMAP(CMU_DEVRST0,               0x00400000, 22);
static struct owl_clkreq resetbit_MODULE_VCE               = BITMAP(CMU_DEVRST0,               0x00100000, 20);
static struct owl_clkreq resetbit_MODULE_VDE               = BITMAP(CMU_DEVRST0,               0x00080000, 19);
static struct owl_clkreq resetbit_MODULE_PCM0              = BITMAP(CMU_DEVRST0,               0x00040000, 18);
static struct owl_clkreq resetbit_MODULE_AUDIO             = BITMAP(CMU_DEVRST0,               0x00020000, 17);
static struct owl_clkreq resetbit_MODULE_GPIO              = BITMAP(CMU_DEVRST0,               0x00008000, 15);
static struct owl_clkreq resetbit_MODULE_KEY               = BITMAP(CMU_DEVRST0,               0x00004000, 14);
static struct owl_clkreq resetbit_MODULE_BISP              = BITMAP(CMU_DEVRST0,               0x00001000, 12);
static struct owl_clkreq resetbit_MODULE_CSI               = BITMAP(CMU_DEVRST0,               0x00000800, 11);
static struct owl_clkreq resetbit_MODULE_DSI               = BITMAP(CMU_DEVRST0,               0x00000400, 10);
static struct owl_clkreq resetbit_MODULE_SD2               = BITMAP(CMU_DEVRST0,               0x00000200, 9);
static struct owl_clkreq resetbit_MODULE_LCD               = BITMAP(CMU_DEVRST0,               0x00000100, 8);
static struct owl_clkreq resetbit_MODULE_DE                = BITMAP(CMU_DEVRST0,               0x00000080, 7);
static struct owl_clkreq resetbit_MODULE_PCM1              = BITMAP(CMU_DEVRST0,               0x00000040, 6);
static struct owl_clkreq resetbit_MODULE_SD1               = BITMAP(CMU_DEVRST0,               0x00000020, 5);
static struct owl_clkreq resetbit_MODULE_SD0               = BITMAP(CMU_DEVRST0,               0x00000010, 4);
static struct owl_clkreq resetbit_MODULE_NANDC             = BITMAP(CMU_DEVRST0,               0x00000008, 3);
static struct owl_clkreq resetbit_MODULE_DDR               = BITMAP(CMU_DEVRST0,               0x00000004, 2);
static struct owl_clkreq resetbit_MODULE_NORIF             = BITMAP(CMU_DEVRST0,               0x00000002, 1);
static struct owl_clkreq resetbit_MODULE_DMAC              = BITMAP(CMU_DEVRST0,               0x00000001, 0);
static struct owl_clkreq resetbit_MODULE_DBG3RESET         = BITMAP(CMU_DEVRST1,               0x80000000, 31);
static struct owl_clkreq resetbit_MODULE_DBG2RESET         = BITMAP(CMU_DEVRST1,               0x40000000, 30);
static struct owl_clkreq resetbit_MODULE_DBG1RESET         = BITMAP(CMU_DEVRST1,               0x20000000, 29);
static struct owl_clkreq resetbit_MODULE_DBG0RESET         = BITMAP(CMU_DEVRST1,               0x10000000, 28);
static struct owl_clkreq resetbit_MODULE_WD3RESET          = BITMAP(CMU_DEVRST1,               0x08000000, 27);
static struct owl_clkreq resetbit_MODULE_WD2RESET          = BITMAP(CMU_DEVRST1,               0x04000000, 26);
static struct owl_clkreq resetbit_MODULE_WD1RESET          = BITMAP(CMU_DEVRST1,               0x02000000, 25);
static struct owl_clkreq resetbit_MODULE_WD0RESET          = BITMAP(CMU_DEVRST1,               0x01000000, 24);
static struct owl_clkreq resetbit_MODULE_USB2_1            = BITMAP(CMU_DEVRST1,               0x00400000, 22);
static struct owl_clkreq resetbit_MODULE_CHIPID            = BITMAP(CMU_DEVRST1,               0x00200000, 21);
static struct owl_clkreq resetbit_MODULE_ETHERNET          = BITMAP(CMU_DEVRST1,               0x00100000, 20);
static struct owl_clkreq resetbit_MODULE_I2C3              = BITMAP(CMU_DEVRST1,               0x00080000, 19);
static struct owl_clkreq resetbit_MODULE_I2C2              = BITMAP(CMU_DEVRST1,               0x00040000, 18);
static struct owl_clkreq resetbit_MODULE_UART5             = BITMAP(CMU_DEVRST1,               0x00020000, 17);
static struct owl_clkreq resetbit_MODULE_UART4             = BITMAP(CMU_DEVRST1,               0x00010000, 16);
static struct owl_clkreq resetbit_MODULE_UART3             = BITMAP(CMU_DEVRST1,               0x00008000, 15);
static struct owl_clkreq resetbit_MODULE_USB3              = BITMAP(CMU_DEVRST1,               0x00004000, 14);
static struct owl_clkreq resetbit_MODULE_I2C1              = BITMAP(CMU_DEVRST1,               0x00002000, 13);
static struct owl_clkreq resetbit_MODULE_I2C0              = BITMAP(CMU_DEVRST1,               0x00001000, 12);
static struct owl_clkreq resetbit_MODULE_SPI3              = BITMAP(CMU_DEVRST1,               0x00000800, 11);
static struct owl_clkreq resetbit_MODULE_SPI2              = BITMAP(CMU_DEVRST1,               0x00000400, 10);
static struct owl_clkreq resetbit_MODULE_SPI1              = BITMAP(CMU_DEVRST1,               0x00000200, 9);
static struct owl_clkreq resetbit_MODULE_SPI0              = BITMAP(CMU_DEVRST1,               0x00000100, 8);
static struct owl_clkreq resetbit_MODULE_UART2             = BITMAP(CMU_DEVRST1,               0x00000080, 7);
static struct owl_clkreq resetbit_MODULE_UART1             = BITMAP(CMU_DEVRST1,               0x00000040, 6);
static struct owl_clkreq resetbit_MODULE_UART0             = BITMAP(CMU_DEVRST1,               0x00000020, 5);
static struct owl_clkreq resetbit_MODULE_UART6             = BITMAP(CMU_DEVRST1,               0x00000010, 4);
static struct owl_clkreq resetbit_MODULE_HDCP2X              = BITMAP(CMU_DEVRST1,               0x00000008, 3);
static struct owl_clkreq resetbit_MODULE_HDMI              = BITMAP(CMU_DEVRST1,               0x00000004, 2);
static struct owl_clkreq resetbit_MODULE_TVOUT             = BITMAP(CMU_DEVRST1,               0x00000002, 1);
static struct owl_clkreq resetbit_MODULE_USB2_0            = BITMAP(CMU_DEVRST1,               0x00000001, 0);


static struct owl_clocknode clocks[CLOCK__MAX] = {
	CLOCKNODE_ROOT(HOSC, FREQUENCY_24M),
	CLOCKNODE_ROOT(IC_32K, FREQUENCY_32K),
	PLLNODE(COREPLL, HOSC),
	PLLNODE(DEVPLL, HOSC),
	PLLNODE(DDRPLL, HOSC),
	PLLNODE(NANDPLL, HOSC),
	PLLNODE(DISPLAYPLL, HOSC),
	PLLNODE(AUDIOPLL, HOSC),
	PLLNODE(TVOUTPLL, HOSC),
	CLOCKNODE_UNUSED(DEEPCOLORPLL),
	PLLNODE(ETHERNETPLL, HOSC),
	PLLNODE(CVBSPLL, HOSC),
	CLOCKNODE_S2(DEV_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S1(DDR_CLK_0, DDRPLL),
	CLOCKNODE_S1(DDR_CLK_90, DDRPLL),
	CLOCKNODE_S1(DDR_CLK_180, DDRPLL),
	CLOCKNODE_S1(DDR_CLK_270, DDRPLL),
	CLOCKNODE_S1(DDR_CLK_CH0, DDR_CLK_0),
	CLOCKNODE_S4(DDR_CLK_CH1, DDR_CLK_0, DDR_CLK_90, DDR_CLK_180, DDR_CLK_270, 0),
	CLOCKNODE_S1(DDR_CLK, DDR_CLK_0),
	CLOCKNODE_S1(SPDIF_CLK, AUDIOPLL),
	CLOCKNODE_S1(HDMIA_CLK, AUDIOPLL),
	CLOCKNODE_S1(I2SRX_CLK, AUDIOPLL),
	CLOCKNODE_S1(I2STX_CLK, AUDIOPLL),
	CLOCKNODE_S1(PCM0_CLK, AUDIOPLL),
	CLOCKNODE_S1(PCM1_CLK, AUDIOPLL),
	CLOCKNODE_UNUSED(CLK_CVBSX2),
	CLOCKNODE_UNUSED(CLK_CVBS),
	CLOCKNODE_S1(CVBS_CLK108M, HOSC),
	CLOCKNODE_S1(CLK_PIXEL, TVOUTPLL),
	CLOCKNODE_S1(CLK_TMDS, TVOUTPLL),
	CLOCKNODE_S1(CLK_TMDS_PHY_P, CLK_TMDS),
	CLOCKNODE_UNUSED(CLK_TMDS_PHY_N),
	CLOCKNODE_S1(L2_NIC_CLK, CORE_CLK),
	CLOCKNODE_S1(APBDBG_CLK, CPU_CLK),
	CLOCKNODE_S1(L2_CLK, CPU_CLK),
	CLOCKNODE_S1(ACP_CLK, CPU_CLK),
	CLOCKNODE_S1(PERIPH_CLK, CPU_CLK),
	CLOCKNODE_S1(NIC_DIV_CLK, NIC_CLK),
	CLOCKNODE_S4(NIC_CLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, 0),
	CLOCKNODE_S4(AHBPREDIV_CLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, 0),
	CLOCKNODE_S1(H_CLK, AHBPREDIV_CLK),
	CLOCKNODE_S1(APB30_CLK, NIC_CLK),
	CLOCKNODE_S1(APB20_CLK, NIC_CLK),
	CLOCKNODE_S1(AHB_CLK, H_CLK),
	CLOCKNODE_S4(CORE_CLK, IC_32K, HOSC, COREPLL, VCE_CLK, 1),
	CLOCKNODE_S1(CPU_CLK, CORE_CLK),
	CLOCKNODE_S2(SENSOR_CLKOUT0, HOSC, ISPBP_CLK, 0),
	CLOCKNODE_S2(SENSOR_CLKOUT1, HOSC, ISPBP_CLK, 0),
	CLOCKNODE_S2(LCD_CLK, DISPLAYPLL, DEV_CLK, 0),
	CLOCKNODE_S1(LVDS_CLK, DISPLAYPLL),
	CLOCKNODE_S1(CKA_LCD_H, LVDS_CLK),
	CLOCKNODE_S1(LCD1_CLK, LCD_CLK),
	CLOCKNODE_S1(LCD0_CLK, LCD_CLK),
	CLOCKNODE_S1(DSI_HCLK, DISPLAYPLL),
	CLOCKNODE_S1(DSI_HCLK90, DSI_HCLK),
	CLOCKNODE_S1(PRO_CLK, DSI_HCLK90),
	CLOCKNODE_S1(PHY_CLK, DSI_HCLK90),
	CLOCKNODE_S2(CSI_CLK, DISPLAYPLL, DEV_CLK, 0),
	CLOCKNODE_S2(DE1_CLK, DISPLAYPLL, DEV_CLK, 0),
	CLOCKNODE_S2(DE2_CLK, DISPLAYPLL, DEV_CLK, 0),
	CLOCKNODE_S2(BISP_CLK, DISPLAYPLL, DEV_CLK, 0),
	CLOCKNODE_S1(ISPBP_CLK, BISP_CLK),
	CLOCKNODE_S2(IMG5_CLK, LCD1_CLK, LCD0_CLK, 0),
	CLOCKNODE_S4(VDE_CLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, 0),
	CLOCKNODE_S4(VCE_CLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, 0),
	CLOCKNODE_S4(NANDC_CLK, NANDPLL, DISPLAYPLL, DEV_CLK, DDRPLL, 2),
	CLOCKNODE_S4(ECC_CLK, NANDPLL, DISPLAYPLL, DEV_CLK, DDRPLL, 2),
	CLOCKNODE_S2(PRESD0_CLK, DEV_CLK, NANDPLL, 0),
	CLOCKNODE_S2(PRESD1_CLK, DEV_CLK, NANDPLL, 0),
	CLOCKNODE_S2(PRESD2_CLK, DEV_CLK, NANDPLL, 0),
	CLOCKNODE_S1(SD0_CLK_2X, PRESD0_CLK),
	CLOCKNODE_S1(SD1_CLK_2X, PRESD1_CLK),
	CLOCKNODE_S1(SD2_CLK_2X, PRESD2_CLK),
	CLOCKNODE_S1(SD0_CLK, SD0_CLK_2X),
	CLOCKNODE_S1(SD1_CLK, SD1_CLK_2X),
	CLOCKNODE_S1(SD2_CLK, SD2_CLK_2X),
	CLOCKNODE_S2(UART0_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART1_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART2_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART3_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART4_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART5_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(UART6_CLK, HOSC, DEVPLL, 0),
	CLOCKNODE_S2(PWM0_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S2(PWM1_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S2(PWM2_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S2(PWM3_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S2(PWM4_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S2(PWM5_CLK, IC_32K, HOSC, 0),
	CLOCKNODE_S1(RMII_REF_CLK, ETHERNETPLL),
	CLOCKNODE_S1(I2C0_CLK, ETHERNETPLL),
	CLOCKNODE_S1(I2C1_CLK, ETHERNETPLL),
	CLOCKNODE_S1(I2C2_CLK, ETHERNETPLL),
	CLOCKNODE_S1(I2C3_CLK, ETHERNETPLL),
	CLOCKNODE_S1(25M_CLK, ETHERNETPLL),
	CLOCKNODE_S1(LENS_CLK, HOSC),
	CLOCKNODE_S1(HDMI24M, HOSC),
	CLOCKNODE_S1(TIMER_CLK, HOSC),
	CLOCKNODE_S1(SS_CLK, HOSC),
	CLOCKNODE_S1(SPS_CLK, HOSC),
	CLOCKNODE_S1(IRC_CLK, HOSC),
	CLOCKNODE_S1(TV24M, HOSC),
	CLOCKNODE_S1(MIPI24M, HOSC),
	CLOCKNODE_S1(LENS24M, HOSC),
	CLOCKNODE_S5(GPU3D_SYSCLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, CVBSPLL, 0),
	CLOCKNODE_S5(GPU3D_HYDCLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, CVBSPLL, 0),
	CLOCKNODE_S5(GPU3D_NIC_MEMCLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, CVBSPLL, 0),
	CLOCKNODE_S5(GPU3D_CORECLK, DEV_CLK, DISPLAYPLL, NANDPLL, DDRPLL, CVBSPLL, 0),
};

static struct owl_pll pllnode[PLL__MAX] = {
    [PLL__COREPLL] = {
        .type = PLL_T_STEP,
        .range_from = 48/12,
        .range_to = 1608/12,
        .freq.step.step = 12 * MEGA,
        .sel = -1,
        .delay = OWL_COREPLL_DELAY,
        .reg_pllen = &pllbit_COREPLLEN,
        .reg_pllfreq = &pllbit_COREPLLFREQ,
        .reg_plllock = &pllbit_COREPLLLOCK,
    },
    [PLL__DEVPLL] = {
        .type = PLL_T_STEP,
        .range_from = 48/6,
        .range_to = 756/6,
        .freq.step.step = 6 * MEGA,
        .sel = -1,
        .delay = OWL_DEVPLL_DELAY,
        .reg_pllen = &pllbit_DEVPLLEN,
        .reg_pllfreq = &pllbit_DEVPLLFREQ,
        .reg_plllock = &pllbit_DEVPLLLOCK,
    },
    [PLL__DDRPLL] = {
        .type = PLL_T_STEP,
        .range_from = 12/12,
        .range_to = 804/12,
        .freq.step.step = 12 * MEGA,
        .sel = -1,
        .delay = OWL_DDRPLL_DELAY,
        .reg_pllen = &pllbit_DDRPLLEN,
        .reg_pllfreq = &pllbit_DDRPLLFREQ,
        .reg_plllock = &pllbit_DDRPLLLOCK,
    },
    [PLL__NANDPLL] = {
        .type = PLL_T_STEP,
        .range_from = 12/6,
        .range_to = 516/6,
        .freq.step.step = 6 * MEGA,
        .sel = -1,
        .delay = OWL_NANDPLL_DELAY,
        .reg_pllen = &pllbit_NANDPLLEN,
        .reg_pllfreq = &pllbit_NANDPLLFREQ,
        .reg_plllock = &pllbit_NANDPLLLOCK,
    },
    [PLL__DISPLAYPLL] = {
        .type = PLL_T_STEP,
        .range_from = 12/6,
        .range_to = 756/6,
        .freq.step.step = 6 * MEGA,
        .sel = -1,
        .delay = OWL_DISPLAYPLL_DELAY,
        .reg_pllen = &pllbit_DISPLAYPLLEN,
        .reg_pllfreq = &pllbit_DISPALYPLLFREQ,
        .reg_plllock = &pllbit_DISPLAYPLLLOCK,
    },

    [PLL__AUDIOPLL] = {
        .type = PLL_T_FREQ,
        .range_from = 0,
        .range_to = 1,
        .freq.freqtab = {45158400, 49152*KILO},
        .sel = -1,
        .delay = OWL_AUDIOPLL_DELAY,
        .reg_pllen = &pllbit_AUDIOPLLEN,
        .reg_pllfreq = &pllbit_AUDIOPLLFREQ_SEL,
        .reg_plllock = &pllbit_AUDIOPLLLOCK,
    },
    [PLL__TVOUTPLL] = {
        .type = PLL_T_FREQ,
        .range_from = 0,
        .range_to = 7,
        .freq.freqtab = {25200*KILO, 27*MEGA, 50400*KILO, 54*MEGA, 74250*KILO, 108*MEGA, 148500*KILO, 297*MEGA},
        .sel = -1,
        .delay = OWL_TVOUTPLL_DELAY,
        .reg_pllen = &pllbit_TVOUTPLLEN,
        .reg_pllfreq = &pllbit_TVOUTPLLFREQ_SEL,
        .reg_plllock = &pllbit_TVOUTPLLLOCK,
    },
    [PLL__ETHERNETPLL] = {
        .type = PLL_T_FREQ,
        .range_from = 0,
        .range_to = 0,
        .freq.freqtab = {500 * MEGA},
        .sel = -1,
        .delay = OWL_ETHERNETPLL_DELAY,
        .reg_pllen = &pllbit_ENTRNETPLL_EN,
        .reg_plllock = &pllbit_ETHERNETPLLLOCK,
    },
    [PLL__CVBSPLL] = {
        .type = PLL_T_STEP,
        .range_from = (348/12)-2,
        .range_to = (540/12)-2,
        .freq.step.step = 12 * MEGA,
        .freq.step.offset = 2,
        .sel = -1,
        .delay = OWL_CVBSPLL_DELAY,
        .reg_pllen = &pllbit_CVBSPLL_EN,
        .reg_pllfreq = &pllbit_CVBSPLLREQ,
        .reg_plllock = &pllbit_CVBSPLLLOCK,
    },
};

static struct owl_cmumod modnode[MOD__MAX] = {
    [MOD__ROOT]         = {"CMUMOD_DEVCLKS",    NULL,                           NULL                        },
    [MOD__GPU3D]        = {"CMUMOD_GPU3D",      &enablebit_MODULE_GPU3D,        &resetbit_MODULE_GPU3D      },
    [MOD__SHARESRAM]    = {"CMUMOD_SHARESRAM",  &enablebit_MODULE_SHARESRAM,    NULL                        },
    [MOD__HDCP2X]       = {"CMUMOD_HDCP2X",     &enablebit_MODULE_HDCP2X,       &resetbit_MODULE_HDCP2X                        },
    [MOD__VCE]          = {"CMUMOD_VCE",        &enablebit_MODULE_VCE,          &resetbit_MODULE_VCE        },
    [MOD__VDE]          = {"CMUMOD_VDE",        &enablebit_MODULE_VDE,          &resetbit_MODULE_VDE        },
    [MOD__PCM0]         = {"CMUMOD_PCM0",       &enablebit_MODULE_PCM0,         &resetbit_MODULE_PCM0       },
    [MOD__SPDIF]        = {"CMUMOD_SPDIF",      &enablebit_MODULE_SPDIF,        NULL                        },
    [MOD__HDMIA]        = {"CMUMOD_HDMIA",      &enablebit_MODULE_HDMIA,        NULL                        },
    [MOD__I2SRX]        = {"CMUMOD_I2SRX",      &enablebit_MODULE_I2SRX,        NULL                        },
    [MOD__I2STX]        = {"CMUMOD_I2STX",      &enablebit_MODULE_I2STX,        NULL                        },
    [MOD__GPIO]         = {"CMUMOD_GPIO",       &enablebit_MODULE_GPIO,         &resetbit_MODULE_GPIO       },
    [MOD__KEY]          = {"CMUMOD_KEY",        &enablebit_MODULE_KEY,          &resetbit_MODULE_KEY        },
    [MOD__LENS]         = {"CMUMOD_LENS",       &enablebit_MODULE_LENS,         &resetbit_MODULE_LENS       },
    [MOD__BISP]         = {"CMUMOD_BISP",       &enablebit_MODULE_BISP,         &resetbit_MODULE_BISP       },
    [MOD__CSI]          = {"CMUMOD_CSI",        &enablebit_MODULE_CSI,          &resetbit_MODULE_CSI        },
    [MOD__DSI]          = {"CMUMOD_DSI",        &enablebit_MODULE_DSI,          &resetbit_MODULE_DSI        },
    [MOD__LVDS]         = {"CMUMOD_LVDS",       &enablebit_MODULE_LVDS,         NULL                        },
    [MOD__LCD1]         = {"CMUMOD_LCD1",       &enablebit_MODULE_LCD1,         NULL                        },
    [MOD__LCD0]         = {"CMUMOD_LCD0",       &enablebit_MODULE_LCD0,         NULL                        },
    [MOD__DE]           = {"CMUMOD_DE",         &enablebit_MODULE_DE,           &resetbit_MODULE_DE         },
    [MOD__SD2]          = {"CMUMOD_SD2",        &enablebit_MODULE_SD2,          &resetbit_MODULE_SD2        },
    [MOD__SD1]          = {"CMUMOD_SD1",        &enablebit_MODULE_SD1,          &resetbit_MODULE_SD1        },
    [MOD__SD0]          = {"CMUMOD_SD0",        &enablebit_MODULE_SD0,          &resetbit_MODULE_SD0        },
    [MOD__NANDC]        = {"CMUMOD_NANDC",      &enablebit_MODULE_NANDC,        &resetbit_MODULE_NANDC      },
    [MOD__DDRCH0]       = {"CMUMOD_DDRCH0",     &enablebit_MODULE_DDRCH0,       NULL                        },
    [MOD__NOR]          = {"CMUMOD_NOR",        &enablebit_MODULE_NOR,          NULL                        },
    [MOD__DMAC]         = {"CMUMOD_DMAC",       &enablebit_MODULE_DMAC,         &resetbit_MODULE_DMAC       },
    [MOD__DDRCH1]       = {"CMUMOD_DDRCH1",     &enablebit_MODULE_DDRCH1,       NULL                        },
    [MOD__I2C3]         = {"CMUMOD_I2C3",       &enablebit_MODULE_I2C3,         &resetbit_MODULE_I2C3       },
    [MOD__I2C2]         = {"CMUMOD_I2C2",       &enablebit_MODULE_I2C2,         &resetbit_MODULE_I2C2       },
    [MOD__TIMER]        = {"CMUMOD_TIMER",      &enablebit_MODULE_TIMER,        NULL                        },
    [MOD__PWM5]         = {"CMUMOD_PWM5",       &enablebit_MODULE_PWM5,         NULL                        },
    [MOD__PWM4]         = {"CMUMOD_PWM4",       &enablebit_MODULE_PWM4,         NULL                        },
    [MOD__PWM3]         = {"CMUMOD_PWM3",       &enablebit_MODULE_PWM3,         NULL                        },
    [MOD__PWM2]         = {"CMUMOD_PWM2",       &enablebit_MODULE_PWM2,         NULL                        },
    [MOD__PWM1]         = {"CMUMOD_PWM1",       &enablebit_MODULE_PWM1,         NULL                        },
    [MOD__PWM0]         = {"CMUMOD_PWM0",       &enablebit_MODULE_PWM0,         NULL                        },
    [MOD__ETHERNET]     = {"CMUMOD_ETHERNET",   &enablebit_MODULE_ETHERNET,     &resetbit_MODULE_ETHERNET   },
    [MOD__UART5]        = {"CMUMOD_UART5",      &enablebit_MODULE_UART5,        &resetbit_MODULE_UART5      },
    [MOD__UART4]        = {"CMUMOD_UART4",      &enablebit_MODULE_UART4,        &resetbit_MODULE_UART4      },
    [MOD__UART3]        = {"CMUMOD_UART3",      &enablebit_MODULE_UART3,        &resetbit_MODULE_UART3      },
    [MOD__UART6]        = {"CMUMOD_UART6",      &enablebit_MODULE_UART6,        &resetbit_MODULE_UART6      },
    [MOD__PCM1]         = {"CMUMOD_PCM1",       &enablebit_MODULE_PCM1,         &resetbit_MODULE_PCM1       },
    [MOD__I2C1]         = {"CMUMOD_I2C1",       &enablebit_MODULE_I2C1,         &resetbit_MODULE_I2C1       },
    [MOD__I2C0]         = {"CMUMOD_I2C0",       &enablebit_MODULE_I2C0,         &resetbit_MODULE_I2C0       },
    [MOD__SPI3]         = {"CMUMOD_SPI3",       &enablebit_MODULE_SPI3,         &resetbit_MODULE_SPI3       },
    [MOD__SPI2]         = {"CMUMOD_SPI2",       &enablebit_MODULE_SPI2,         &resetbit_MODULE_SPI2       },
    [MOD__SPI1]         = {"CMUMOD_SPI1",       &enablebit_MODULE_SPI1,         &resetbit_MODULE_SPI1       },
    [MOD__SPI0]         = {"CMUMOD_SPI0",       &enablebit_MODULE_SPI0,         &resetbit_MODULE_SPI0       },
    [MOD__IRC]          = {"CMUMOD_IRC",        &enablebit_MODULE_IRC,          NULL                        },
    [MOD__UART2]        = {"CMUMOD_UART2",      &enablebit_MODULE_UART2,        &resetbit_MODULE_UART2      },
    [MOD__UART1]        = {"CMUMOD_UART1",      &enablebit_MODULE_UART1,        &resetbit_MODULE_UART1      },
    [MOD__UART0]        = {"CMUMOD_UART0",      &enablebit_MODULE_UART0,        &resetbit_MODULE_UART0      },
    [MOD__HDMI]         = {"CMUMOD_HDMI",       &enablebit_MODULE_HDMI,         &resetbit_MODULE_HDMI       },
    [MOD__SS]           = {"CMUMOD_SS",         &enablebit_MODULE_SS,           NULL                        },
    [MOD__TV24M]        = {"CMUMOD_TV24M",      &enablebit_MODULE_TV24M,        NULL                        },
    [MOD__CVBS_CLK108M]        = {"CMUMOD_CVBS_CLK108M",      &enablebit_MODULE_CVBS_CLK108M,        NULL                        },
    [MOD__TVOUT]        = {"CMUMOD_TVOUT",      &enablebit_MODULE_TVOUT,                           &resetbit_MODULE_TVOUT                        },

    [MOD__PERIPHRESET]  = {"CMUMOD_PERIPHRESET",NULL,                           &resetbit_MODULE_PERIPHRESET},
    [MOD__NIC301]       = {"CMUMOD_NIC301",     NULL,                           &resetbit_MODULE_NIC301     },
    [MOD__AUDIO]        = {"CMUMOD_AUDIO",      NULL,                           &resetbit_MODULE_AUDIO      },
    [MOD__LCD]          = {"CMUMOD_LCD",        NULL,                           &resetbit_MODULE_LCD        },
    [MOD__DDR]          = {"CMUMOD_DDR",        NULL,                           &resetbit_MODULE_DDR        },
    [MOD__NORIF]        = {"CMUMOD_NORIF",      NULL,                           &resetbit_MODULE_NORIF      },
    [MOD__DBG3RESET]    = {"CMUMOD_DBG3RESET",  NULL,                           &resetbit_MODULE_DBG3RESET  },
    [MOD__DBG2RESET]    = {"CMUMOD_DBG2RESET",  NULL,                           &resetbit_MODULE_DBG2RESET  },
    [MOD__DBG1RESET]    = {"CMUMOD_DBG1RESET",  NULL,                           &resetbit_MODULE_DBG1RESET  },
    [MOD__DBG0RESET]    = {"CMUMOD_DBG0RESET",  NULL,                           &resetbit_MODULE_DBG0RESET  },
    [MOD__WD3RESET]     = {"CMUMOD_WD3RESET",   NULL,                           &resetbit_MODULE_WD3RESET   },
    [MOD__WD2RESET]     = {"CMUMOD_WD2RESET",   NULL,                           &resetbit_MODULE_WD2RESET   },
    [MOD__WD1RESET]     = {"CMUMOD_WD1RESET",   NULL,                           &resetbit_MODULE_WD1RESET   },
    [MOD__WD0RESET]     = {"CMUMOD_WD0RESET",   NULL,                           &resetbit_MODULE_WD0RESET   },
    [MOD__CHIPID]       = {"CMUMOD_CHIPID",     NULL,                           &resetbit_MODULE_CHIPID     },
    [MOD__USB3]         = {"CMUMOD_USB3",       NULL,                           &resetbit_MODULE_USB3       },
    [MOD__USB2_0]       = {"CMUMOD_USB2_0",     NULL,                           &resetbit_MODULE_USB2_0     },
    [MOD__USB2_1]       = {"CMUMOD_USB2_1",     NULL,                           &resetbit_MODULE_USB2_1     },
};

static struct owl_refertab T_lcd    = {{1, 7, -1}, 0};
static struct owl_refertab T_dsipro = {{3 << 16 | 4, 4, -1}, 1};
static struct owl_refertab T_sdx2   = {{1, 128, -1}, 0};
static struct owl_refertab T_uart   = {{624, -1}, 0};
static struct owl_refertab T_rmii   = {{4, 10, -1}, 0};
static struct owl_refertab T_de    = {{1, 2<<16 | 3, 2, 2<<16 | 5, 3, 4, 6, 8, 12}, 1};
static struct owl_refertab T_vde    = {{1, 2<<16 | 3, 2, 2<<16 | 5, 3, 4, 6, 8, -1}, 1};
static struct owl_refertab T_vce    = {{1, 2<<16 | 3, 2, 2<<16 | 5, 3, 4, 6, 8, -1}, 1};
static struct owl_refertab T_gpu    = {{1, 2<<16 | 3, 2, 2<<16 | 5, 3, 4, 6, 8, -1}, 1};

static struct owl_seqtab S_default  = {9, {1, 2, 3, 4, 6, 8, 12, 16, 24} };

static struct owl_compdiv C_dsipro = {
    .sections[0] = {DIV_T_NATURE, 0, 1, {0} },
    .sections[1] = {
        .type = DIV_T_TABLE,
        .range_from = 2,
        .range_to = 3,
        .ext = { .tab = &T_dsipro, },
    },
};


static struct owl_compdiv C_uart = {
    .sections[0] = {DIV_T_NATURE, 0, 312, {0} },
    .sections[1] = {
        .type = DIV_T_TABLE,
        .range_from = 313,
        .range_to = 313,
        .ext = { .tab = &T_uart, },
    },
};


static struct owl_div divider_SPDIF_CLK = {
    .type = DIV_T_SEQ,
    .range_from = 0,
    .range_to = 8,
    .ext = {.seq = &S_default,},
    .reg = &divbit_SPDIF_CLK,
};
static struct owl_div divider_HDMIA_CLK = {
    .type = DIV_T_SEQ,
    .range_from = 0,
    .range_to = 8,
    .ext = {.seq = &S_default,},
    .reg = &divbit_HDMIA_CLK,
};
static struct owl_div divider_I2SRX_CLK = {
    .type = DIV_T_SEQ,
    .range_from = 0,
    .range_to = 8,
    .ext = {.seq = &S_default,},
    .reg = &divbit_I2SRX_CLK,
};
static struct owl_div divider_I2STX_CLK = {
    .type = DIV_T_SEQ,
    .range_from = 0,
    .range_to = 8,
    .ext = {.seq = &S_default,},
    .reg = &divbit_I2STX_CLK,
};
static struct owl_div divider_APBDBG_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 1,
    .range_to = 7,
    .reg = &divbit_APBDBG_CLK,
};
static struct owl_div divider_ACP_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 1,
    .range_to = 3,
    .reg = &divbit_ACP_CLK,
};
static struct owl_div divider_PERIPH_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 1,
    .range_to = 7,
    .reg = &divbit_PERIPH_CLK,
};
static struct owl_div divider_NIC_DIV_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 1, /* reserve NIC_DIV_CLK divisor 1 */
    .range_to = 3,
    .reg = &divbit_NIC_DIV_CLK,
};
static struct owl_div divider_NIC_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 3,
    .reg = &divbit_NIC_CLK,
};
static struct owl_div divider_AHBPREDIV_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 3,
    .reg = &divbit_AHBPREDIV_CLK,
};
static struct owl_div divider_H_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 1,  /* reserve H_CLK divsor 1 */
    .range_to = 3,
    .reg = &divbit_H_CLK,
};
static struct owl_div divider_APB30_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 3,
    .reg = &divbit_APB30_CLK,
};
static struct owl_div divider_APB20_CLK = {
    .type = DIV_T_SEQ_D2,
    .range_from = 0,
    .range_to = 5,
    .ext = {.seq = &S_default,},
    .reg = &divbit_APB20_CLK,
};
static struct owl_div divider_SENSOR_CLKOUT0 = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_SENSOR_CLKOUT0,
};
static struct owl_div divider_SENSOR_CLKOUT1 = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_SENSOR_CLKOUT1,
};
static struct owl_div divider_LCD_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 1,
    .ext = {.tab = &T_lcd,},
    .reg = &divbit_LCD_CLK,
};
static struct owl_div divider_LCD1_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_LCD1_CLK,
};
static struct owl_div divider_LCD0_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_LCD0_CLK,
};
static struct owl_div divider_DSI_HCLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 3,
    .reg = &divbit_DSI_HCLK,
};
static struct owl_div divider_PRO_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 3,
    .ext = {.comp = &C_dsipro,},
    .reg = &divbit_PRO_CLK,
};
static struct owl_div divider_CSI_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_CSI_CLK,
};
static struct owl_div divider_DE1_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 8,
    .ext = {.tab = &T_de,},
    .reg = &divbit_DE1_CLK,
};
static struct owl_div divider_DE2_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 8,
    .ext = {.tab = &T_de,},
    .reg = &divbit_DE2_CLK,
};
static struct owl_div divider_BISP_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 11,
    .reg = &divbit_BISP_CLK,
};
static struct owl_div divider_VDE_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_vde,},
    .reg = &divbit_VDE_CLK,
};
static struct owl_div divider_VCE_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_vce,},
    .reg = &divbit_VCE_CLK,
};
static struct owl_div divider_NANDC_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 7,
    .reg = &divbit_NANDC_CLK,
};
static struct owl_div divider_ECC_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 7,
    .reg = &divbit_ECC_CLK,
};
static struct owl_div divider_PRESD0_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 24,
    .reg = &divbit_PRESD0_CLK,
};
static struct owl_div divider_PRESD1_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 24,
    .reg = &divbit_PRESD1_CLK,
};
static struct owl_div divider_PRESD2_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 24,
    .reg = &divbit_PRESD2_CLK,
};
static struct owl_div divider_SD0_CLK_2X = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 1,
    .ext = {.tab = &T_sdx2,},
    .reg = &divbit_SD0_CLK_2X,
};
static struct owl_div divider_SD1_CLK_2X = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 1,
    .ext = {.tab = &T_sdx2,},
    .reg = &divbit_SD1_CLK_2X,
};
static struct owl_div divider_SD2_CLK_2X = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 1,
    .ext = {.tab = &T_sdx2,},
    .reg = &divbit_SD2_CLK_2X,
};
static struct owl_div divider_UART0_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART0_CLK,
};
static struct owl_div divider_UART1_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART1_CLK,
};
static struct owl_div divider_UART2_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART2_CLK,
};
static struct owl_div divider_UART3_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART3_CLK,
};
static struct owl_div divider_UART4_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART4_CLK,
};
static struct owl_div divider_UART5_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART5_CLK,
};
static struct owl_div divider_UART6_CLK = {
    .type = DIV_T_COMP,
    .range_from = 0,
    .range_to = 313,
    .ext = {.comp = &C_uart,},
    .reg = &divbit_UART6_CLK,
};
static struct owl_div divider_PWM0_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM0_CLK,
};
static struct owl_div divider_PWM1_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM1_CLK,
};
static struct owl_div divider_PWM2_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM2_CLK,
};
static struct owl_div divider_PWM3_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM3_CLK,
};
static struct owl_div divider_PWM4_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM4_CLK,
};
static struct owl_div divider_PWM5_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_PWM5_CLK,
};
static struct owl_div divider_RMII_REF_CLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 1,
    .ext = {.tab = &T_rmii,},
    .reg = &divbit_RMII_REF_CLK,
};
static struct owl_div divider_LENS_CLK = {
    .type = DIV_T_EXP_D2,
    .range_from = 0,
    .range_to = 7,
    .reg = &divbit_LENS_CLK,
};
static struct owl_div divider_GPU3D_SYSCLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_gpu,},
    .reg = &divbit_GPU3D_SYSCLK,
};
static struct owl_div divider_GPU3D_HYDCLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_gpu,},
    .reg = &divbit_GPU3D_HYDCLK,
};
static struct owl_div divider_GPU3D_NIC_MEMCLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_gpu,},
    .reg = &divbit_GPU3D_NIC_MEMCLK,
};
static struct owl_div divider_GPU3D_CORECLK = {
    .type = DIV_T_TABLE,
    .range_from = 0,
    .range_to = 7,
    .ext = {.tab = &T_gpu,},
    .reg = &divbit_GPU3D_CORECLK,
};
static struct owl_div divider_SS_CLK = {
    .type = DIV_T_NATURE,
    .range_from = 0,
    .range_to = 1023,
    .reg = &divbit_SS_CLK,
};


/*
 * parent names init data for common clocks
 */
static const char *parent_names_COREPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_DEVPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_DDRPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_NANDPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_DISPLAYPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_AUDIOPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_TVOUTPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_ETHERNETPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_CVBSPLL[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_DEV_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_DDR_CLK_0[] = {
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_DDR_CLK_90[] = {
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_DDR_CLK_180[] = {
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_DDR_CLK_270[] = {
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_DDR_CLK_CH0[] = {
	clocks[CLOCK__DDR_CLK_0].name,
};
static const char *parent_names_DDR_CLK_CH1[] = {
	clocks[CLOCK__DDR_CLK_0].name,
	clocks[CLOCK__DDR_CLK_90].name,
	clocks[CLOCK__DDR_CLK_180].name,
	clocks[CLOCK__DDR_CLK_270].name,
};
static const char *parent_names_DDR_CLK[] = {
	clocks[CLOCK__DDR_CLK_0].name,
};
static const char *parent_names_SPDIF_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_HDMIA_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_I2SRX_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_I2STX_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_PCM0_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_PCM1_CLK[] = {
	clocks[CLOCK__AUDIOPLL].name,
};
static const char *parent_names_CLK_PIXEL[] = {
	clocks[CLOCK__TVOUTPLL].name,
};
static const char *parent_names_CLK_TMDS[] = {
	clocks[CLOCK__TVOUTPLL].name,
};
static const char *parent_names_CLK_TMDS_PHY_P[] = {
	clocks[CLOCK__CLK_TMDS].name,
};
static const char *parent_names_L2_NIC_CLK[] = {
	clocks[CLOCK__CORE_CLK].name,
};
static const char *parent_names_APBDBG_CLK[] = {
	clocks[CLOCK__CPU_CLK].name,
};
static const char *parent_names_L2_CLK[] = {
	clocks[CLOCK__CPU_CLK].name,
};
static const char *parent_names_ACP_CLK[] = {
	clocks[CLOCK__CPU_CLK].name,
};
static const char *parent_names_PERIPH_CLK[] = {
	clocks[CLOCK__CPU_CLK].name,
};
static const char *parent_names_NIC_DIV_CLK[] = {
	clocks[CLOCK__NIC_CLK].name,
};
static const char *parent_names_NIC_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_AHBPREDIV_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
};

static const char *parent_names_H_CLK[] = {
	clocks[CLOCK__AHBPREDIV_CLK].name,
};
static const char *parent_names_APB30_CLK[] = {
	clocks[CLOCK__NIC_CLK].name,
};
static const char *parent_names_APB20_CLK[] = {
	clocks[CLOCK__NIC_CLK].name,
};
static const char *parent_names_AHB_CLK[] = {
	clocks[CLOCK__H_CLK].name,
};
static const char *parent_names_CORE_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__COREPLL].name,
	clocks[CLOCK__VCE_CLK].name,
};
static const char *parent_names_CPU_CLK[] = {
	clocks[CLOCK__CORE_CLK].name,
};
static const char *parent_names_SENSOR_CLKOUT0[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__ISPBP_CLK].name,
};
static const char *parent_names_SENSOR_CLKOUT1[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__ISPBP_CLK].name,
};
static const char *parent_names_LCD_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
};
static const char *parent_names_LVDS_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
};
static const char *parent_names_CKA_LCD_H[] = {
	clocks[CLOCK__LVDS_CLK].name,
};
static const char *parent_names_LCD1_CLK[] = {
	clocks[CLOCK__LCD_CLK].name,
};
static const char *parent_names_LCD0_CLK[] = {
	clocks[CLOCK__LCD_CLK].name,
};
static const char *parent_names_DSI_HCLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
};
static const char *parent_names_DSI_HCLK90[] = {
	clocks[CLOCK__DSI_HCLK].name,
};
static const char *parent_names_PRO_CLK[] = {
	clocks[CLOCK__DSI_HCLK90].name,
};
static const char *parent_names_PHY_CLK[] = {
	clocks[CLOCK__DSI_HCLK90].name,
};
static const char *parent_names_CSI_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
};
static const char *parent_names_DE1_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
};
static const char *parent_names_DE2_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
};
static const char *parent_names_BISP_CLK[] = {
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
};
static const char *parent_names_ISPBP_CLK[] = {
	clocks[CLOCK__BISP_CLK].name,
};
static const char *parent_names_IMG5_CLK[] = {
	clocks[CLOCK__LCD1_CLK].name,
	clocks[CLOCK__LCD0_CLK].name,
};
static const char *parent_names_VDE_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_VCE_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_NANDC_CLK[] = {
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_ECC_CLK[] = {
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DDRPLL].name,
};
static const char *parent_names_PRESD0_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__NANDPLL].name,
};
static const char *parent_names_PRESD1_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__NANDPLL].name,
};
static const char *parent_names_PRESD2_CLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__NANDPLL].name,
};
static const char *parent_names_SD0_CLK_2X[] = {
	clocks[CLOCK__PRESD0_CLK].name,
};
static const char *parent_names_SD1_CLK_2X[] = {
	clocks[CLOCK__PRESD1_CLK].name,
};
static const char *parent_names_SD2_CLK_2X[] = {
	clocks[CLOCK__PRESD2_CLK].name,
};
static const char *parent_names_SD0_CLK[] = {
	clocks[CLOCK__SD0_CLK_2X].name,
};
static const char *parent_names_SD1_CLK[] = {
	clocks[CLOCK__SD1_CLK_2X].name,
};
static const char *parent_names_SD2_CLK[] = {
	clocks[CLOCK__SD2_CLK_2X].name,
};
static const char *parent_names_UART0_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART1_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART2_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART3_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART4_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART5_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_UART6_CLK[] = {
	clocks[CLOCK__HOSC].name,
	clocks[CLOCK__DEVPLL].name,
};
static const char *parent_names_PWM0_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_PWM1_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_PWM2_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_PWM3_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_PWM4_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_PWM5_CLK[] = {
	clocks[CLOCK__IC_32K].name,
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_RMII_REF_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_I2C0_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_I2C1_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_I2C2_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_I2C3_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_25M_CLK[] = {
	clocks[CLOCK__ETHERNETPLL].name,
};
static const char *parent_names_LENS_CLK[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_HDMI24M[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_TIMER_CLK[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_SS_CLK[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_SPS_CLK[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_IRC_CLK[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_TV24M[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_CVBS_CLK108M[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_MIPI24M[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_LENS24M[] = {
	clocks[CLOCK__HOSC].name,
};
static const char *parent_names_GPU3D_SYSCLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
	clocks[CLOCK__CVBSPLL].name,
};
static const char *parent_names_GPU3D_HYDCLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
	clocks[CLOCK__CVBSPLL].name,
};
static const char *parent_names_GPU3D_NIC_MEMCLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
	clocks[CLOCK__CVBSPLL].name,
};
static const char *parent_names_GPU3D_CORECLK[] = {
	clocks[CLOCK__DEV_CLK].name,
	clocks[CLOCK__DISPLAYPLL].name,
	clocks[CLOCK__NANDPLL].name,
	clocks[CLOCK__DDRPLL].name,
	clocks[CLOCK__CVBSPLL].name,
};

static const char *parent_name_CMUMOD_DEVCLKS[] = {
	modnode[MOD__ROOT].modname,
};

static struct clk owl_clks[];

/*
 * clock imp: "real" clocks
 */
static struct owl_clk_foo clk_foo_clocks[CLOCK__MAX] = {
    [CLOCK__HOSC] = {
        .hw.clk = &owl_clks[CLOCK__HOSC],
        .clock = CLOCK__HOSC,
    },
    [CLOCK__IC_32K] = {
        .hw.clk = &owl_clks[CLOCK__IC_32K],
        .clock = CLOCK__IC_32K,
    },
    [CLOCK__COREPLL] = {
        .hw.clk = &owl_clks[CLOCK__COREPLL],
        .clock = CLOCK__COREPLL,
    },
    [CLOCK__DEVPLL] = {
        .hw.clk = &owl_clks[CLOCK__DEVPLL],
        .clock = CLOCK__DEVPLL,
    },
    [CLOCK__DDRPLL] = {
        .hw.clk = &owl_clks[CLOCK__DDRPLL],
        .clock = CLOCK__DDRPLL,
    },
    [CLOCK__NANDPLL] = {
        .hw.clk = &owl_clks[CLOCK__NANDPLL],
        .clock = CLOCK__NANDPLL,
    },
    [CLOCK__DISPLAYPLL] = {
        .hw.clk = &owl_clks[CLOCK__DISPLAYPLL],
        .clock = CLOCK__DISPLAYPLL,
    },
    [CLOCK__AUDIOPLL] = {
        .hw.clk = &owl_clks[CLOCK__AUDIOPLL],
        .clock = CLOCK__AUDIOPLL,
    },
    [CLOCK__TVOUTPLL] = {
        .hw.clk = &owl_clks[CLOCK__TVOUTPLL],
        .clock = CLOCK__TVOUTPLL,
    },
    [CLOCK__ETHERNETPLL] = {
        .hw.clk = &owl_clks[CLOCK__ETHERNETPLL],
        .clock = CLOCK__ETHERNETPLL,
    },
    [CLOCK__CVBSPLL] = {
        .hw.clk = &owl_clks[CLOCK__CVBSPLL],
        .clock = CLOCK__CVBSPLL,
    },
    [CLOCK__DEV_CLK] = {
        .hw.clk = &owl_clks[CLOCK__DEV_CLK],
        .clock = CLOCK__DEV_CLK,
    },
    [CLOCK__DDR_CLK_0] = {
        .hw.clk = &owl_clks[CLOCK__DDR_CLK_0],
        .clock = CLOCK__DDR_CLK_0,
    },
    [CLOCK__DDR_CLK_90] = {
        .hw.clk = &owl_clks[CLOCK__DDR_CLK_90],
        .clock = CLOCK__DDR_CLK_90,
    },
    [CLOCK__DDR_CLK_180] = {
        .hw.clk = &owl_clks[CLOCK__DDR_CLK_180],
        .clock = CLOCK__DDR_CLK_180,
    },
    [CLOCK__DDR_CLK_270] = {
        .hw.clk = &owl_clks[CLOCK__DDR_CLK_270],
        .clock = CLOCK__DDR_CLK_270,
    },
    [CLOCK__DDR_CLK_CH0] = {
        .hw.clk = &owl_clks[CLOCK__DDR_CLK_CH0],
        .clock = CLOCK__DDR_CLK_CH0,
    },
    [CLOCK__DDR_CLK_CH1] = {
        .hw.clk = &owl_clks[CLOCK__DDR_CLK_CH1],
        .clock = CLOCK__DDR_CLK_CH1,
    },
    [CLOCK__DDR_CLK] = {
        .hw.clk = &owl_clks[CLOCK__DDR_CLK],
        .clock = CLOCK__DDR_CLK,
    },
    [CLOCK__SPDIF_CLK] = {
        .hw.clk = &owl_clks[CLOCK__SPDIF_CLK],
        .clock = CLOCK__SPDIF_CLK,
    },
    [CLOCK__HDMIA_CLK] = {
        .hw.clk = &owl_clks[CLOCK__HDMIA_CLK],
        .clock = CLOCK__HDMIA_CLK,
    },
    [CLOCK__I2SRX_CLK] = {
        .hw.clk = &owl_clks[CLOCK__I2SRX_CLK],
        .clock = CLOCK__I2SRX_CLK,
    },
    [CLOCK__I2STX_CLK] = {
        .hw.clk = &owl_clks[CLOCK__I2STX_CLK],
        .clock = CLOCK__I2STX_CLK,
    },
    [CLOCK__PCM0_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PCM0_CLK],
        .clock = CLOCK__PCM0_CLK,
    },
    [CLOCK__PCM1_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PCM1_CLK],
        .clock = CLOCK__PCM1_CLK,
    },
    [CLOCK__CLK_PIXEL] = {
        .hw.clk = &owl_clks[CLOCK__CLK_PIXEL],
        .clock = CLOCK__CLK_PIXEL,
    },
    [CLOCK__CLK_TMDS] = {
        .hw.clk = &owl_clks[CLOCK__CLK_TMDS],
        .clock = CLOCK__CLK_TMDS,
    },
    [CLOCK__CLK_TMDS_PHY_P] = {
        .hw.clk = &owl_clks[CLOCK__CLK_TMDS_PHY_P],
        .clock = CLOCK__CLK_TMDS_PHY_P,
    },
    [CLOCK__L2_NIC_CLK] = {
        .hw.clk = &owl_clks[CLOCK__L2_NIC_CLK],
        .clock = CLOCK__L2_NIC_CLK,
    },
    [CLOCK__APBDBG_CLK] = {
        .hw.clk = &owl_clks[CLOCK__APBDBG_CLK],
        .clock = CLOCK__APBDBG_CLK,
    },
    [CLOCK__L2_CLK] = {
        .hw.clk = &owl_clks[CLOCK__L2_CLK],
        .clock = CLOCK__L2_CLK,
    },
    [CLOCK__ACP_CLK] = {
        .hw.clk = &owl_clks[CLOCK__ACP_CLK],
        .clock = CLOCK__ACP_CLK,
    },
    [CLOCK__PERIPH_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PERIPH_CLK],
        .clock = CLOCK__PERIPH_CLK,
    },
    [CLOCK__NIC_DIV_CLK] = {
        .hw.clk = &owl_clks[CLOCK__NIC_DIV_CLK],
        .clock = CLOCK__NIC_DIV_CLK,
    },
    [CLOCK__NIC_CLK] = {
        .hw.clk = &owl_clks[CLOCK__NIC_CLK],
        .clock = CLOCK__NIC_CLK,
    },
    [CLOCK__AHBPREDIV_CLK] = {
        .hw.clk = &owl_clks[CLOCK__AHBPREDIV_CLK],
        .clock = CLOCK__AHBPREDIV_CLK,
    },
    [CLOCK__H_CLK] = {
        .hw.clk = &owl_clks[CLOCK__H_CLK],
        .clock = CLOCK__H_CLK,
    },
    [CLOCK__APB30_CLK] = {
        .hw.clk = &owl_clks[CLOCK__APB30_CLK],
        .clock = CLOCK__APB30_CLK,
    },
    [CLOCK__APB20_CLK] = {
        .hw.clk = &owl_clks[CLOCK__APB20_CLK],
        .clock = CLOCK__APB20_CLK,
    },
    [CLOCK__AHB_CLK] = {
        .hw.clk = &owl_clks[CLOCK__AHB_CLK],
        .clock = CLOCK__AHB_CLK,
    },
    [CLOCK__CORE_CLK] = {
        .hw.clk = &owl_clks[CLOCK__CORE_CLK],
        .clock = CLOCK__CORE_CLK,
    },
    [CLOCK__CPU_CLK] = {
        .hw.clk = &owl_clks[CLOCK__CPU_CLK],
        .clock = CLOCK__CPU_CLK,
    },
    [CLOCK__SENSOR_CLKOUT0] = {
        .hw.clk = &owl_clks[CLOCK__SENSOR_CLKOUT0],
        .clock = CLOCK__SENSOR_CLKOUT0,
    },
    [CLOCK__SENSOR_CLKOUT1] = {
        .hw.clk = &owl_clks[CLOCK__SENSOR_CLKOUT1],
        .clock = CLOCK__SENSOR_CLKOUT1,
    },
    [CLOCK__LCD_CLK] = {
        .hw.clk = &owl_clks[CLOCK__LCD_CLK],
        .clock = CLOCK__LCD_CLK,
    },
    [CLOCK__LVDS_CLK] = {
        .hw.clk = &owl_clks[CLOCK__LVDS_CLK],
        .clock = CLOCK__LVDS_CLK,
    },
    [CLOCK__CKA_LCD_H] = {
        .hw.clk = &owl_clks[CLOCK__CKA_LCD_H],
        .clock = CLOCK__CKA_LCD_H,
    },
    [CLOCK__LCD1_CLK] = {
        .hw.clk = &owl_clks[CLOCK__LCD1_CLK],
        .clock = CLOCK__LCD1_CLK,
    },
    [CLOCK__LCD0_CLK] = {
        .hw.clk = &owl_clks[CLOCK__LCD0_CLK],
        .clock = CLOCK__LCD0_CLK,
    },
    [CLOCK__DSI_HCLK] = {
        .hw.clk = &owl_clks[CLOCK__DSI_HCLK],
        .clock = CLOCK__DSI_HCLK,
    },
    [CLOCK__DSI_HCLK90] = {
        .hw.clk = &owl_clks[CLOCK__DSI_HCLK90],
        .clock = CLOCK__DSI_HCLK90,
    },
    [CLOCK__PRO_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PRO_CLK],
        .clock = CLOCK__PRO_CLK,
    },
    [CLOCK__PHY_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PHY_CLK],
        .clock = CLOCK__PHY_CLK,
    },
    [CLOCK__CSI_CLK] = {
        .hw.clk = &owl_clks[CLOCK__CSI_CLK],
        .clock = CLOCK__CSI_CLK,
    },
    [CLOCK__DE1_CLK] = {
        .hw.clk = &owl_clks[CLOCK__DE1_CLK],
        .clock = CLOCK__DE1_CLK,
    },
    [CLOCK__DE2_CLK] = {
        .hw.clk = &owl_clks[CLOCK__DE2_CLK],
        .clock = CLOCK__DE2_CLK,
    },
    [CLOCK__BISP_CLK] = {
        .hw.clk = &owl_clks[CLOCK__BISP_CLK],
        .clock = CLOCK__BISP_CLK,
    },
    [CLOCK__ISPBP_CLK] = {
        .hw.clk = &owl_clks[CLOCK__ISPBP_CLK],
        .clock = CLOCK__ISPBP_CLK,
    },
    [CLOCK__IMG5_CLK] = {
        .hw.clk = &owl_clks[CLOCK__IMG5_CLK],
        .clock = CLOCK__IMG5_CLK,
    },
    [CLOCK__VDE_CLK] = {
        .hw.clk = &owl_clks[CLOCK__VDE_CLK],
        .clock = CLOCK__VDE_CLK,
    },
    [CLOCK__VCE_CLK] = {
        .hw.clk = &owl_clks[CLOCK__VCE_CLK],
        .clock = CLOCK__VCE_CLK,
    },
    [CLOCK__NANDC_CLK] = {
        .hw.clk = &owl_clks[CLOCK__NANDC_CLK],
        .clock = CLOCK__NANDC_CLK,
    },
    [CLOCK__ECC_CLK] = {
        .hw.clk = &owl_clks[CLOCK__ECC_CLK],
        .clock = CLOCK__ECC_CLK,
    },
    [CLOCK__PRESD0_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PRESD0_CLK],
        .clock = CLOCK__PRESD0_CLK,
    },
    [CLOCK__PRESD1_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PRESD1_CLK],
        .clock = CLOCK__PRESD1_CLK,
    },
    [CLOCK__PRESD2_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PRESD2_CLK],
        .clock = CLOCK__PRESD2_CLK,
    },
    [CLOCK__SD0_CLK_2X] = {
        .hw.clk = &owl_clks[CLOCK__SD0_CLK_2X],
        .clock = CLOCK__SD0_CLK_2X,
    },
    [CLOCK__SD1_CLK_2X] = {
        .hw.clk = &owl_clks[CLOCK__SD1_CLK_2X],
        .clock = CLOCK__SD1_CLK_2X,
    },
    [CLOCK__SD2_CLK_2X] = {
        .hw.clk = &owl_clks[CLOCK__SD2_CLK_2X],
        .clock = CLOCK__SD2_CLK_2X,
    },
    [CLOCK__SD0_CLK] = {
        .hw.clk = &owl_clks[CLOCK__SD0_CLK],
        .clock = CLOCK__SD0_CLK,
    },
    [CLOCK__SD1_CLK] = {
        .hw.clk = &owl_clks[CLOCK__SD1_CLK],
        .clock = CLOCK__SD1_CLK,
    },
    [CLOCK__SD2_CLK] = {
        .hw.clk = &owl_clks[CLOCK__SD2_CLK],
        .clock = CLOCK__SD2_CLK,
    },
    [CLOCK__UART0_CLK] = {
        .hw.clk = &owl_clks[CLOCK__UART0_CLK],
        .clock = CLOCK__UART0_CLK,
    },
    [CLOCK__UART1_CLK] = {
        .hw.clk = &owl_clks[CLOCK__UART1_CLK],
        .clock = CLOCK__UART1_CLK,
    },
    [CLOCK__UART2_CLK] = {
        .hw.clk = &owl_clks[CLOCK__UART2_CLK],
        .clock = CLOCK__UART2_CLK,
    },
    [CLOCK__UART3_CLK] = {
        .hw.clk = &owl_clks[CLOCK__UART3_CLK],
        .clock = CLOCK__UART3_CLK,
    },
    [CLOCK__UART4_CLK] = {
        .hw.clk = &owl_clks[CLOCK__UART4_CLK],
        .clock = CLOCK__UART4_CLK,
    },
    [CLOCK__UART5_CLK] = {
        .hw.clk = &owl_clks[CLOCK__UART5_CLK],
        .clock = CLOCK__UART5_CLK,
    },
    [CLOCK__UART6_CLK] = {
        .hw.clk = &owl_clks[CLOCK__UART6_CLK],
        .clock = CLOCK__UART6_CLK,
    },
    [CLOCK__PWM0_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PWM0_CLK],
        .clock = CLOCK__PWM0_CLK,
    },
    [CLOCK__PWM1_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PWM1_CLK],
        .clock = CLOCK__PWM1_CLK,
    },
    [CLOCK__PWM2_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PWM2_CLK],
        .clock = CLOCK__PWM2_CLK,
    },
    [CLOCK__PWM3_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PWM3_CLK],
        .clock = CLOCK__PWM3_CLK,
    },
    [CLOCK__PWM4_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PWM4_CLK],
        .clock = CLOCK__PWM4_CLK,
    },
    [CLOCK__PWM5_CLK] = {
        .hw.clk = &owl_clks[CLOCK__PWM5_CLK],
        .clock = CLOCK__PWM5_CLK,
    },
    [CLOCK__RMII_REF_CLK] = {
        .hw.clk = &owl_clks[CLOCK__RMII_REF_CLK],
        .clock = CLOCK__RMII_REF_CLK,
    },
    [CLOCK__I2C0_CLK] = {
        .hw.clk = &owl_clks[CLOCK__I2C0_CLK],
        .clock = CLOCK__I2C0_CLK,
    },
    [CLOCK__I2C1_CLK] = {
        .hw.clk = &owl_clks[CLOCK__I2C1_CLK],
        .clock = CLOCK__I2C1_CLK,
    },
    [CLOCK__I2C2_CLK] = {
        .hw.clk = &owl_clks[CLOCK__I2C2_CLK],
        .clock = CLOCK__I2C2_CLK,
    },
    [CLOCK__I2C3_CLK] = {
        .hw.clk = &owl_clks[CLOCK__I2C3_CLK],
        .clock = CLOCK__I2C3_CLK,
    },
    [CLOCK__25M_CLK] = {
        .hw.clk = &owl_clks[CLOCK__25M_CLK],
        .clock = CLOCK__25M_CLK,
    },
    [CLOCK__LENS_CLK] = {
        .hw.clk = &owl_clks[CLOCK__LENS_CLK],
        .clock = CLOCK__LENS_CLK,
    },
    [CLOCK__HDMI24M] = {
        .hw.clk = &owl_clks[CLOCK__HDMI24M],
        .clock = CLOCK__HDMI24M,
    },
    [CLOCK__TIMER_CLK] = {
        .hw.clk = &owl_clks[CLOCK__TIMER_CLK],
        .clock = CLOCK__TIMER_CLK,
    },
    [CLOCK__SS_CLK] = {
        .hw.clk = &owl_clks[CLOCK__SS_CLK],
        .clock = CLOCK__SS_CLK,
    },
    [CLOCK__SPS_CLK] = {
        .hw.clk = &owl_clks[CLOCK__SPS_CLK],
        .clock = CLOCK__SPS_CLK,
    },
    [CLOCK__IRC_CLK] = {
        .hw.clk = &owl_clks[CLOCK__IRC_CLK],
        .clock = CLOCK__IRC_CLK,
    },
    [CLOCK__TV24M] = {
        .hw.clk = &owl_clks[CLOCK__TV24M],
        .clock = CLOCK__TV24M,
    },
    [CLOCK__CVBS_CLK108M] = {
        .hw.clk = &owl_clks[CLOCK__CVBS_CLK108M],
        .clock = CLOCK__CVBS_CLK108M,
    },
    [CLOCK__MIPI24M] = {
        .hw.clk = &owl_clks[CLOCK__MIPI24M],
        .clock = CLOCK__MIPI24M,
    },
    [CLOCK__LENS24M] = {
        .hw.clk = &owl_clks[CLOCK__LENS24M],
        .clock = CLOCK__LENS24M,
    },
    [CLOCK__GPU3D_SYSCLK] = {
        .hw.clk = &owl_clks[CLOCK__GPU3D_SYSCLK],
        .clock = CLOCK__GPU3D_SYSCLK,
    },
    [CLOCK__GPU3D_HYDCLK] = {
        .hw.clk = &owl_clks[CLOCK__GPU3D_HYDCLK],
        .clock = CLOCK__GPU3D_HYDCLK,
    },
    [CLOCK__GPU3D_NIC_MEMCLK] = {
        .hw.clk = &owl_clks[CLOCK__GPU3D_NIC_MEMCLK],
        .clock = CLOCK__GPU3D_NIC_MEMCLK,
    },
    [CLOCK__GPU3D_CORECLK] = {
        .hw.clk = &owl_clks[CLOCK__GPU3D_CORECLK],
        .clock = CLOCK__GPU3D_CORECLK,
    },
};

/*
 * clock imp: module level clocks, treat as a "virtual" clock
 */
static struct  owl_clk_foo clk_foo_modules[MOD__MAX_IN_CLK] = {
    [MOD__ROOT] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__ROOT],
        .clock = MOD__ROOT,
    },
    [MOD__GPU3D] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__GPU3D],
        .clock = MOD__GPU3D,
    },
    [MOD__SHARESRAM] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__SHARESRAM],
        .clock = MOD__SHARESRAM,
    },
    [MOD__HDCP2X] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__HDCP2X],
        .clock = MOD__HDCP2X,
    },
    [MOD__VCE] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__VCE],
        .clock = MOD__VCE,
    },
    [MOD__VDE] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__VDE],
        .clock = MOD__VDE,
    },
    [MOD__PCM0] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__PCM0],
        .clock = MOD__PCM0,
    },
    [MOD__SPDIF] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__SPDIF],
        .clock = MOD__SPDIF,
    },
    [MOD__HDMIA] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__HDMIA],
        .clock = MOD__HDMIA,
    },
    [MOD__I2SRX] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__I2SRX],
        .clock = MOD__I2SRX,
    },
    [MOD__I2STX] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__I2STX],
        .clock = MOD__I2STX,
    },
    [MOD__GPIO] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__GPIO],
        .clock = MOD__GPIO,
    },
    [MOD__KEY] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__KEY],
        .clock = MOD__KEY,
    },
    [MOD__LENS] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__LENS],
        .clock = MOD__LENS,
    },
    [MOD__BISP] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__BISP],
        .clock = MOD__BISP,
    },
    [MOD__CSI] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__CSI],
        .clock = MOD__CSI,
    },
    [MOD__DSI] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__DSI],
        .clock = MOD__DSI,
    },
    [MOD__LVDS] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__LVDS],
        .clock = MOD__LVDS,
    },
    [MOD__LCD1] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__LCD1],
        .clock = MOD__LCD1,
    },
    [MOD__LCD0] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__LCD0],
        .clock = MOD__LCD0,
    },
    [MOD__DE] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__DE],
        .clock = MOD__DE,
    },
    [MOD__SD2] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__SD2],
        .clock = MOD__SD2,
    },
    [MOD__SD1] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__SD1],
        .clock = MOD__SD1,
    },
    [MOD__SD0] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__SD0],
        .clock = MOD__SD0,
    },
    [MOD__NANDC] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__NANDC],
        .clock = MOD__NANDC,
    },
    [MOD__DDRCH0] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__DDRCH0],
        .clock = MOD__DDRCH0,
    },
    [MOD__NOR] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__NOR],
        .clock = MOD__NOR,
    },
    [MOD__DMAC] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__DMAC],
        .clock = MOD__DMAC,
    },
    [MOD__DDRCH1] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__DDRCH1],
        .clock = MOD__DDRCH1,
    },
    [MOD__I2C3] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__I2C3],
        .clock = MOD__I2C3,
    },
    [MOD__I2C2] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__I2C2],
        .clock = MOD__I2C2,
    },
    [MOD__TIMER] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__TIMER],
        .clock = MOD__TIMER,
    },
    [MOD__PWM5] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__PWM5],
        .clock = MOD__PWM5,
    },
    [MOD__PWM4] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__PWM4],
        .clock = MOD__PWM4,
    },
    [MOD__PWM3] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__PWM3],
        .clock = MOD__PWM3,
    },
    [MOD__PWM2] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__PWM2],
        .clock = MOD__PWM2,
    },
    [MOD__PWM1] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__PWM1],
        .clock = MOD__PWM1,
    },
    [MOD__PWM0] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__PWM0],
        .clock = MOD__PWM0,
    },
    [MOD__ETHERNET] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__ETHERNET],
        .clock = MOD__ETHERNET,
    },
    [MOD__UART5] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__UART5],
        .clock = MOD__UART5,
    },
    [MOD__UART4] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__UART4],
        .clock = MOD__UART4,
    },
    [MOD__UART3] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__UART3],
        .clock = MOD__UART3,
    },
    [MOD__UART6] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__UART6],
        .clock = MOD__UART6,
    },
    [MOD__PCM1] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__PCM1],
        .clock = MOD__PCM1,
    },
    [MOD__I2C1] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__I2C1],
        .clock = MOD__I2C1,
    },
    [MOD__I2C0] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__I2C0],
        .clock = MOD__I2C0,
    },
    [MOD__SPI3] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__SPI3],
        .clock = MOD__SPI3,
    },
    [MOD__SPI2] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__SPI2],
        .clock = MOD__SPI2,
    },
    [MOD__SPI1] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__SPI1],
        .clock = MOD__SPI1,
    },
    [MOD__SPI0] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__SPI0],
        .clock = MOD__SPI0,
    },
    [MOD__IRC] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__IRC],
        .clock = MOD__IRC,
    },
    [MOD__UART2] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__UART2],
        .clock = MOD__UART2,
    },
    [MOD__UART1] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__UART1],
        .clock = MOD__UART1,
    },
    [MOD__UART0] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__UART0],
        .clock = MOD__UART0,
    },
    [MOD__HDMI] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__HDMI],
        .clock = MOD__HDMI,
    },
    [MOD__SS] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__SS],
        .clock = MOD__SS,
    },
    [MOD__TV24M] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__TV24M],
        .clock = MOD__TV24M,
    },
    [MOD__CVBS_CLK108M] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__CVBS_CLK108M],
        .clock = MOD__CVBS_CLK108M,
    },
    [MOD__TVOUT] = {
        .hw.clk = &owl_clks[CLOCK__MAX + MOD__TVOUT],
        .clock = MOD__TVOUT,
    },
};

static int hoscops_enable(struct clk_hw *hw)
{
	write_clkreg_val(&enablebit_HOSC, 1);
	return 0;
}

static void hoscops_disable(struct clk_hw *hw)
{
	write_clkreg_val(&enablebit_HOSC, 1);
}

static int  hoscops_is_enabled(struct clk_hw *hw)
{
	int ret;
	ret = read_clkreg_val(&enablebit_HOSC);
	return ret;
}

static struct clk_ops clk_ops_gate_hosc = {
	.enable = hoscops_enable,
	.disable = hoscops_disable,
	.is_enabled = hoscops_is_enabled,
};

/*
 * common clk static init.
 */
static struct clk owl_clks[CLOCK__MAX + MOD__MAX_IN_CLK] = {
    [CLOCK__HOSC] = {
        .name = clocks[CLOCK__HOSC].name,
        .ops = &clk_ops_gate_hosc,
        .hw = &clk_foo_clocks[CLOCK__HOSC].hw,
        .parent_names = NULL,
        .num_parents = 0,
        .flags = CLK_IS_ROOT,
    },
    [CLOCK__IC_32K] = {
        .name = clocks[CLOCK__IC_32K].name,
        .ops = &clk_ops_foo,
        .hw = &clk_foo_clocks[CLOCK__IC_32K].hw,
        .parent_names = NULL,
        .num_parents = 0,
        .flags = CLK_IS_ROOT,
    },
    [CLOCK__COREPLL] = {
        .name = clocks[CLOCK__COREPLL].name,
        .ops = &clk_ops_corepll,
        .hw = &clk_foo_clocks[CLOCK__COREPLL].hw,
        .parent_names = parent_names_COREPLL,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__DEVPLL] = {
        .name = clocks[CLOCK__DEVPLL].name,
        .ops = &clk_ops_pll,
        .hw = &clk_foo_clocks[CLOCK__DEVPLL].hw,
        .parent_names = parent_names_DEVPLL,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__DDRPLL] = {
        .name = clocks[CLOCK__DDRPLL].name,
        .ops = &clk_ops_pll,
        .hw = &clk_foo_clocks[CLOCK__DDRPLL].hw,
        .parent_names = parent_names_DDRPLL,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__NANDPLL] = {
        .name = clocks[CLOCK__NANDPLL].name,
        .ops = &clk_ops_pll,
        .hw = &clk_foo_clocks[CLOCK__NANDPLL].hw,
        .parent_names = parent_names_NANDPLL,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__DISPLAYPLL] = {
        .name = clocks[CLOCK__DISPLAYPLL].name,
        .ops = &clk_ops_pll,
        .hw = &clk_foo_clocks[CLOCK__DISPLAYPLL].hw,
        .parent_names = parent_names_DISPLAYPLL,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__AUDIOPLL] = {
        .name = clocks[CLOCK__AUDIOPLL].name,
        .ops = &clk_ops_pll,
        .hw = &clk_foo_clocks[CLOCK__AUDIOPLL].hw,
        .parent_names = parent_names_AUDIOPLL,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__TVOUTPLL] = {
        .name = clocks[CLOCK__TVOUTPLL].name,
        .ops = &clk_ops_pll,
        .hw = &clk_foo_clocks[CLOCK__TVOUTPLL].hw,
        .parent_names = parent_names_TVOUTPLL,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__ETHERNETPLL] = {
        .name = clocks[CLOCK__ETHERNETPLL].name,
        .ops = &clk_ops_pll,
        .hw = &clk_foo_clocks[CLOCK__ETHERNETPLL].hw,
        .parent_names = parent_names_ETHERNETPLL,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__CVBSPLL] = {
        .name = clocks[CLOCK__CVBSPLL].name,
        .ops = &clk_ops_pll,
        .hw = &clk_foo_clocks[CLOCK__CVBSPLL].hw,
        .parent_names = parent_names_CVBSPLL,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__DEV_CLK] = {
        .name = clocks[CLOCK__DEV_CLK].name,
        .ops = &clk_ops_direct_m_parent,
        .hw = &clk_foo_clocks[CLOCK__DEV_CLK].hw,
        .parent_names = parent_names_DEV_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__DDR_CLK_0] = {
        .name = clocks[CLOCK__DDR_CLK_0].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__DDR_CLK_0].hw,
        .parent_names = parent_names_DDR_CLK_0,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__DDR_CLK_90] = {
        .name = clocks[CLOCK__DDR_CLK_90].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__DDR_CLK_90].hw,
        .parent_names = parent_names_DDR_CLK_90,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__DDR_CLK_180] = {
        .name = clocks[CLOCK__DDR_CLK_180].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__DDR_CLK_180].hw,
        .parent_names = parent_names_DDR_CLK_180,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__DDR_CLK_270] = {
        .name = clocks[CLOCK__DDR_CLK_270].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__DDR_CLK_270].hw,
        .parent_names = parent_names_DDR_CLK_270,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__DDR_CLK_CH0] = {
        .name = clocks[CLOCK__DDR_CLK_CH0].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__DDR_CLK_CH0].hw,
        .parent_names = parent_names_DDR_CLK_CH0,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__DDR_CLK_CH1] = {
        .name = clocks[CLOCK__DDR_CLK_CH1].name,
        .ops = &clk_ops_direct_m_parent,
        .hw = &clk_foo_clocks[CLOCK__DDR_CLK_CH1].hw,
        .parent_names = parent_names_DDR_CLK_CH1,
        .num_parents = 4,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__DDR_CLK] = {
        .name = clocks[CLOCK__DDR_CLK].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__DDR_CLK].hw,
        .parent_names = parent_names_DDR_CLK,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__SPDIF_CLK] = {
        .name = clocks[CLOCK__SPDIF_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__SPDIF_CLK].hw,
        .parent_names = parent_names_SPDIF_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__HDMIA_CLK] = {
        .name = clocks[CLOCK__HDMIA_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__HDMIA_CLK].hw,
        .parent_names = parent_names_HDMIA_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__I2SRX_CLK] = {
        .name = clocks[CLOCK__I2SRX_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__I2SRX_CLK].hw,
        .parent_names = parent_names_I2SRX_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__I2STX_CLK] = {
        .name = clocks[CLOCK__I2STX_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__I2STX_CLK].hw,
        .parent_names = parent_names_I2STX_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__PCM0_CLK] = {
        .name = clocks[CLOCK__PCM0_CLK].name,
        .ops = &clk_ops_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__PCM0_CLK].hw,
        .parent_names = parent_names_PCM0_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__PCM1_CLK] = {
        .name = clocks[CLOCK__PCM1_CLK].name,
        .ops = &clk_ops_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__PCM1_CLK].hw,
        .parent_names = parent_names_PCM1_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__CLK_PIXEL] = {
        .name = clocks[CLOCK__CLK_PIXEL].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__CLK_PIXEL].hw,
        .parent_names = parent_names_CLK_PIXEL,
        .num_parents = 1,
        .flags = CLK_GET_RATE_NOCACHE,
    },
    [CLOCK__CLK_TMDS] = {
        .name = clocks[CLOCK__CLK_TMDS].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__CLK_TMDS].hw,
        .parent_names = parent_names_CLK_TMDS,
        .num_parents = 1,
        .flags = CLK_GET_RATE_NOCACHE,
    },
    [CLOCK__CLK_TMDS_PHY_P] = {
        .name = clocks[CLOCK__CLK_TMDS_PHY_P].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__CLK_TMDS_PHY_P].hw,
        .parent_names = parent_names_CLK_TMDS_PHY_P,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__L2_NIC_CLK] = {
        .name = clocks[CLOCK__L2_NIC_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__L2_NIC_CLK].hw,
        .parent_names = parent_names_L2_NIC_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__APBDBG_CLK] = {
        .name = clocks[CLOCK__APBDBG_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__APBDBG_CLK].hw,
        .parent_names = parent_names_APBDBG_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__L2_CLK] = {
        .name = clocks[CLOCK__L2_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__L2_CLK].hw,
        .parent_names = parent_names_L2_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__ACP_CLK] = {
        .name = clocks[CLOCK__ACP_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__ACP_CLK].hw,
        .parent_names = parent_names_ACP_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__PERIPH_CLK] = {
        .name = clocks[CLOCK__PERIPH_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__PERIPH_CLK].hw,
        .parent_names = parent_names_PERIPH_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__NIC_DIV_CLK] = {
        .name = clocks[CLOCK__NIC_DIV_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__NIC_DIV_CLK].hw,
        .parent_names = parent_names_NIC_DIV_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__NIC_CLK] = {
        .name = clocks[CLOCK__NIC_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__NIC_CLK].hw,
        .parent_names = parent_names_NIC_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__AHBPREDIV_CLK] = {
        .name = clocks[CLOCK__AHBPREDIV_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__AHBPREDIV_CLK].hw,
        .parent_names = parent_names_AHBPREDIV_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__H_CLK] = {
        .name = clocks[CLOCK__H_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__H_CLK].hw,
        .parent_names = parent_names_H_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__APB30_CLK] = {
        .name = clocks[CLOCK__APB30_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__APB30_CLK].hw,
        .parent_names = parent_names_APB30_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__APB20_CLK] = {
        .name = clocks[CLOCK__APB20_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__APB20_CLK].hw,
        .parent_names = parent_names_APB20_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__AHB_CLK] = {
        .name = clocks[CLOCK__AHB_CLK].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__AHB_CLK].hw,
        .parent_names = parent_names_AHB_CLK,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__CORE_CLK] = {
        .name = clocks[CLOCK__CORE_CLK].name,
        .ops = &clk_ops_direct_m_parent,
        .hw = &clk_foo_clocks[CLOCK__CORE_CLK].hw,
        .parent_names = parent_names_CORE_CLK,
        .num_parents = 4,
        .flags = 0,
    },
    [CLOCK__CPU_CLK] = {
        .name = clocks[CLOCK__CPU_CLK].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__CPU_CLK].hw,
        .parent_names = parent_names_CPU_CLK,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__SENSOR_CLKOUT0] = {
        .name = clocks[CLOCK__SENSOR_CLKOUT0].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__SENSOR_CLKOUT0].hw,
        .parent_names = parent_names_SENSOR_CLKOUT0,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__SENSOR_CLKOUT1] = {
        .name = clocks[CLOCK__SENSOR_CLKOUT1].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__SENSOR_CLKOUT1].hw,
        .parent_names = parent_names_SENSOR_CLKOUT1,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__LCD_CLK] = {
        .name = clocks[CLOCK__LCD_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__LCD_CLK].hw,
        .parent_names = parent_names_LCD_CLK,
        .num_parents = 3,
        .flags = 0,
    },
    [CLOCK__LVDS_CLK] = {
        .name = clocks[CLOCK__LVDS_CLK].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__LVDS_CLK].hw,
        .parent_names = parent_names_LVDS_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__CKA_LCD_H] = {
        .name = clocks[CLOCK__CKA_LCD_H].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__CKA_LCD_H].hw,
        .parent_names = parent_names_CKA_LCD_H,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__LCD1_CLK] = {
        .name = clocks[CLOCK__LCD1_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__LCD1_CLK].hw,
        .parent_names = parent_names_LCD1_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__LCD0_CLK] = {
        .name = clocks[CLOCK__LCD0_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__LCD0_CLK].hw,
        .parent_names = parent_names_LCD0_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__DSI_HCLK] = {
        .name = clocks[CLOCK__DSI_HCLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__DSI_HCLK].hw,
        .parent_names = parent_names_DSI_HCLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__DSI_HCLK90] = {
        .name = clocks[CLOCK__DSI_HCLK90].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__DSI_HCLK90].hw,
        .parent_names = parent_names_DSI_HCLK90,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__PRO_CLK] = {
        .name = clocks[CLOCK__PRO_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__PRO_CLK].hw,
        .parent_names = parent_names_PRO_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__PHY_CLK] = {
        .name = clocks[CLOCK__PHY_CLK].name,
        .ops = &clk_ops_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__PHY_CLK].hw,
        .parent_names = parent_names_PHY_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__CSI_CLK] = {
        .name = clocks[CLOCK__CSI_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__CSI_CLK].hw,
        .parent_names = parent_names_CSI_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__DE1_CLK] = {
        .name = clocks[CLOCK__DE1_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__DE1_CLK].hw,
        .parent_names = parent_names_DE1_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__DE2_CLK] = {
        .name = clocks[CLOCK__DE2_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__DE2_CLK].hw,
        .parent_names = parent_names_DE2_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__BISP_CLK] = {
        .name = clocks[CLOCK__BISP_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__BISP_CLK].hw,
        .parent_names = parent_names_BISP_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__ISPBP_CLK] = {
        .name = clocks[CLOCK__ISPBP_CLK].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__ISPBP_CLK].hw,
        .parent_names = parent_names_ISPBP_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__IMG5_CLK] = {
        .name = clocks[CLOCK__IMG5_CLK].name,
        .ops = &clk_ops_direct_m_parent,
        .hw = &clk_foo_clocks[CLOCK__IMG5_CLK].hw,
        .parent_names = parent_names_IMG5_CLK,
        .num_parents = 2,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__VDE_CLK] = {
        .name = clocks[CLOCK__VDE_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__VDE_CLK].hw,
        .parent_names = parent_names_VDE_CLK,
        .num_parents = 4,
        .flags = 0,
    },
    [CLOCK__VCE_CLK] = {
        .name = clocks[CLOCK__VCE_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__VCE_CLK].hw,
        .parent_names = parent_names_VCE_CLK,
        .num_parents = 4,
        .flags = 0,
    },
    [CLOCK__NANDC_CLK] = {
        .name = clocks[CLOCK__NANDC_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__NANDC_CLK].hw,
        .parent_names = parent_names_NANDC_CLK,
        .num_parents = 4,
        .flags = 0,
    },
    [CLOCK__ECC_CLK] = {
        .name = clocks[CLOCK__ECC_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__ECC_CLK].hw,
        .parent_names = parent_names_ECC_CLK,
        .num_parents = 4,
        .flags = 0,
    },
    [CLOCK__PRESD0_CLK] = {
        .name = clocks[CLOCK__PRESD0_CLK].name,
        .ops = &clk_ops_h_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__PRESD0_CLK].hw,
        .parent_names = parent_names_PRESD0_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__PRESD1_CLK] = {
        .name = clocks[CLOCK__PRESD1_CLK].name,
        .ops = &clk_ops_h_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__PRESD1_CLK].hw,
        .parent_names = parent_names_PRESD1_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__PRESD2_CLK] = {
        .name = clocks[CLOCK__PRESD2_CLK].name,
        .ops = &clk_ops_h_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__PRESD2_CLK].hw,
        .parent_names = parent_names_PRESD2_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__SD0_CLK_2X] = {
        .name = clocks[CLOCK__SD0_CLK_2X].name,
        .ops = &clk_ops_b_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__SD0_CLK_2X].hw,
        .parent_names = parent_names_SD0_CLK_2X,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__SD1_CLK_2X] = {
        .name = clocks[CLOCK__SD1_CLK_2X].name,
        .ops = &clk_ops_b_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__SD1_CLK_2X].hw,
        .parent_names = parent_names_SD1_CLK_2X,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__SD2_CLK_2X] = {
        .name = clocks[CLOCK__SD2_CLK_2X].name,
        .ops = &clk_ops_b_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__SD2_CLK_2X].hw,
        .parent_names = parent_names_SD2_CLK_2X,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__SD0_CLK] = {
        .name = clocks[CLOCK__SD0_CLK].name,
        .ops = &clk_ops_b_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__SD0_CLK].hw,
        .parent_names = parent_names_SD0_CLK,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__SD1_CLK] = {
        .name = clocks[CLOCK__SD1_CLK].name,
        .ops = &clk_ops_b_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__SD1_CLK].hw,
        .parent_names = parent_names_SD1_CLK,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__SD2_CLK] = {
        .name = clocks[CLOCK__SD2_CLK].name,
        .ops = &clk_ops_b_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__SD2_CLK].hw,
        .parent_names = parent_names_SD2_CLK,
        .num_parents = 1,
        .flags = CLK_SET_RATE_PARENT,
    },
    [CLOCK__UART0_CLK] = {
        .name = clocks[CLOCK__UART0_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__UART0_CLK].hw,
        .parent_names = parent_names_UART0_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__UART1_CLK] = {
        .name = clocks[CLOCK__UART1_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__UART1_CLK].hw,
        .parent_names = parent_names_UART1_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__UART2_CLK] = {
        .name = clocks[CLOCK__UART2_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__UART2_CLK].hw,
        .parent_names = parent_names_UART2_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__UART3_CLK] = {
        .name = clocks[CLOCK__UART3_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__UART3_CLK].hw,
        .parent_names = parent_names_UART3_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__UART4_CLK] = {
        .name = clocks[CLOCK__UART4_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__UART4_CLK].hw,
        .parent_names = parent_names_UART4_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__UART5_CLK] = {
        .name = clocks[CLOCK__UART5_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__UART5_CLK].hw,
        .parent_names = parent_names_UART5_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__UART6_CLK] = {
        .name = clocks[CLOCK__UART6_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__UART6_CLK].hw,
        .parent_names = parent_names_UART6_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__PWM0_CLK] = {
        .name = clocks[CLOCK__PWM0_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__PWM0_CLK].hw,
        .parent_names = parent_names_PWM0_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__PWM1_CLK] = {
        .name = clocks[CLOCK__PWM1_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__PWM1_CLK].hw,
        .parent_names = parent_names_PWM1_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__PWM2_CLK] = {
        .name = clocks[CLOCK__PWM2_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__PWM2_CLK].hw,
        .parent_names = parent_names_PWM2_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__PWM3_CLK] = {
        .name = clocks[CLOCK__PWM3_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__PWM3_CLK].hw,
        .parent_names = parent_names_PWM3_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__PWM4_CLK] = {
        .name = clocks[CLOCK__PWM4_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__PWM4_CLK].hw,
        .parent_names = parent_names_PWM4_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__PWM5_CLK] = {
        .name = clocks[CLOCK__PWM5_CLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__PWM5_CLK].hw,
        .parent_names = parent_names_PWM5_CLK,
        .num_parents = 2,
        .flags = 0,
    },
    [CLOCK__RMII_REF_CLK] = {
        .name = clocks[CLOCK__RMII_REF_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__RMII_REF_CLK].hw,
        .parent_names = parent_names_RMII_REF_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__I2C0_CLK] = {
        .name = clocks[CLOCK__I2C0_CLK].name,
        .ops = &clk_ops_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__I2C0_CLK].hw,
        .parent_names = parent_names_I2C0_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__I2C1_CLK] = {
        .name = clocks[CLOCK__I2C1_CLK].name,
        .ops = &clk_ops_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__I2C1_CLK].hw,
        .parent_names = parent_names_I2C1_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__I2C2_CLK] = {
        .name = clocks[CLOCK__I2C2_CLK].name,
        .ops = &clk_ops_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__I2C2_CLK].hw,
        .parent_names = parent_names_I2C2_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__I2C3_CLK] = {
        .name = clocks[CLOCK__I2C3_CLK].name,
        .ops = &clk_ops_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__I2C3_CLK].hw,
        .parent_names = parent_names_I2C3_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__25M_CLK] = {
        .name = clocks[CLOCK__25M_CLK].name,
        .ops = &clk_ops_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__25M_CLK].hw,
        .parent_names = parent_names_25M_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__LENS_CLK] = {
        .name = clocks[CLOCK__LENS_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__LENS_CLK].hw,
        .parent_names = parent_names_LENS_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__HDMI24M] = {
        .name = clocks[CLOCK__HDMI24M].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__HDMI24M].hw,
        .parent_names = parent_names_HDMI24M,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__TIMER_CLK] = {
        .name = clocks[CLOCK__TIMER_CLK].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__TIMER_CLK].hw,
        .parent_names = parent_names_TIMER_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__SS_CLK] = {
        .name = clocks[CLOCK__SS_CLK].name,
        .ops = &clk_ops_m_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__SS_CLK].hw,
        .parent_names = parent_names_SS_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__SPS_CLK] = {
        .name = clocks[CLOCK__SPS_CLK].name,
        .ops = &clk_ops_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__SPS_CLK].hw,
        .parent_names = parent_names_SPS_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__IRC_CLK] = {
        .name = clocks[CLOCK__IRC_CLK].name,
        .ops = &clk_ops_s_divider_s_parent,
        .hw = &clk_foo_clocks[CLOCK__IRC_CLK].hw,
        .parent_names = parent_names_IRC_CLK,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__TV24M] = {
        .name = clocks[CLOCK__TV24M].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__TV24M].hw,
        .parent_names = parent_names_TV24M,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__CVBS_CLK108M] = {
        .name = clocks[CLOCK__CVBS_CLK108M].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__CVBS_CLK108M].hw,
        .parent_names = parent_names_CVBS_CLK108M,
        .num_parents = 1,
        .flags = 0,
    },	
    [CLOCK__MIPI24M] = {
        .name = clocks[CLOCK__MIPI24M].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__MIPI24M].hw,
        .parent_names = parent_names_MIPI24M,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__LENS24M] = {
        .name = clocks[CLOCK__LENS24M].name,
        .ops = &clk_ops_direct_s_parent,
        .hw = &clk_foo_clocks[CLOCK__LENS24M].hw,
        .parent_names = parent_names_LENS24M,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__GPU3D_SYSCLK] = {
        .name = clocks[CLOCK__GPU3D_SYSCLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__GPU3D_SYSCLK].hw,
        .parent_names = parent_names_GPU3D_SYSCLK,
        .num_parents = 4,
        .flags = 0,
    },
    [CLOCK__GPU3D_HYDCLK] = {
        .name = clocks[CLOCK__GPU3D_HYDCLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__GPU3D_HYDCLK].hw,
        .parent_names = parent_names_GPU3D_HYDCLK,
        .num_parents = 4,
        .flags = 0,
    },
    [CLOCK__GPU3D_NIC_MEMCLK] = {
        .name = clocks[CLOCK__GPU3D_NIC_MEMCLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__GPU3D_NIC_MEMCLK].hw,
        .parent_names = parent_names_GPU3D_NIC_MEMCLK,
        .num_parents = 4,
        .flags = 0,
    },
    [CLOCK__GPU3D_CORECLK] = {
        .name = clocks[CLOCK__GPU3D_CORECLK].name,
        .ops = &clk_ops_m_divider_m_parent,
        .hw = &clk_foo_clocks[CLOCK__GPU3D_CORECLK].hw,
        .parent_names = parent_names_GPU3D_CORECLK,
        .num_parents = 4,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__ROOT] = {
        .name = modnode[MOD__ROOT].modname,
        .ops = &clk_ops_foo,
        .hw = &clk_foo_modules[MOD__ROOT].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 0,
        .flags = CLK_IS_ROOT,
    },
    [CLOCK__MAX + MOD__GPU3D] = {
        .name = modnode[MOD__GPU3D].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__GPU3D].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__SHARESRAM] = {
        .name = modnode[MOD__SHARESRAM].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__SHARESRAM].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__HDCP2X] = {
        .name = modnode[MOD__HDCP2X].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__HDCP2X].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__VCE] = {
        .name = modnode[MOD__VCE].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__VCE].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__VDE] = {
        .name = modnode[MOD__VDE].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__VDE].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__PCM0] = {
        .name = modnode[MOD__PCM0].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__PCM0].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__SPDIF] = {
        .name = modnode[MOD__SPDIF].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__SPDIF].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__HDMIA] = {
        .name = modnode[MOD__HDMIA].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__HDMIA].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__I2SRX] = {
        .name = modnode[MOD__I2SRX].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__I2SRX].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__I2STX] = {
        .name = modnode[MOD__I2STX].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__I2STX].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__GPIO] = {
        .name = modnode[MOD__GPIO].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__GPIO].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__KEY] = {
        .name = modnode[MOD__KEY].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__KEY].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__LENS] = {
        .name = modnode[MOD__LENS].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__LENS].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__BISP] = {
        .name = modnode[MOD__BISP].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__BISP].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__CSI] = {
        .name = modnode[MOD__CSI].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__CSI].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__DSI] = {
        .name = modnode[MOD__DSI].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__DSI].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__LVDS] = {
        .name = modnode[MOD__LVDS].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__LVDS].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__LCD1] = {
        .name = modnode[MOD__LCD1].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__LCD1].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__LCD0] = {
        .name = modnode[MOD__LCD0].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__LCD0].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__DE] = {
        .name = modnode[MOD__DE].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__DE].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__SD2] = {
        .name = modnode[MOD__SD2].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__SD2].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__SD1] = {
        .name = modnode[MOD__SD1].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__SD1].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__SD0] = {
        .name = modnode[MOD__SD0].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__SD0].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__NANDC] = {
        .name = modnode[MOD__NANDC].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__NANDC].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__DDRCH0] = {
        .name = modnode[MOD__DDRCH0].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__DDRCH0].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__NOR] = {
        .name = modnode[MOD__NOR].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__NOR].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__DMAC] = {
        .name = modnode[MOD__DMAC].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__DMAC].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__DDRCH1] = {
        .name = modnode[MOD__DDRCH1].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__DDRCH1].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__I2C3] = {
        .name = modnode[MOD__I2C3].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__I2C3].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__I2C2] = {
        .name = modnode[MOD__I2C2].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__I2C2].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__TIMER] = {
        .name = modnode[MOD__TIMER].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__TIMER].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__PWM5] = {
        .name = modnode[MOD__PWM5].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__PWM5].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__PWM4] = {
        .name = modnode[MOD__PWM4].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__PWM4].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__PWM3] = {
        .name = modnode[MOD__PWM3].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__PWM3].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__PWM2] = {
        .name = modnode[MOD__PWM2].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__PWM2].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__PWM1] = {
        .name = modnode[MOD__PWM1].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__PWM1].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__PWM0] = {
        .name = modnode[MOD__PWM0].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__PWM0].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__ETHERNET] = {
        .name = modnode[MOD__ETHERNET].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__ETHERNET].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__UART5] = {
        .name = modnode[MOD__UART5].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__UART5].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__UART4] = {
        .name = modnode[MOD__UART4].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__UART4].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__UART3] = {
        .name = modnode[MOD__UART3].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__UART3].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__UART6] = {
        .name = modnode[MOD__UART6].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__UART6].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__PCM1] = {
        .name = modnode[MOD__PCM1].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__PCM1].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__I2C1] = {
        .name = modnode[MOD__I2C1].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__I2C1].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__I2C0] = {
        .name = modnode[MOD__I2C0].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__I2C0].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__SPI3] = {
        .name = modnode[MOD__SPI3].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__SPI3].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__SPI2] = {
        .name = modnode[MOD__SPI2].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__SPI2].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__SPI1] = {
        .name = modnode[MOD__SPI1].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__SPI1].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__SPI0] = {
        .name = modnode[MOD__SPI0].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__SPI0].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__IRC] = {
        .name = modnode[MOD__IRC].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__IRC].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__UART2] = {
        .name = modnode[MOD__UART2].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__UART2].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__UART1] = {
        .name = modnode[MOD__UART1].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__UART1].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__UART0] = {
        .name = modnode[MOD__UART0].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__UART0].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__HDMI] = {
        .name = modnode[MOD__HDMI].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__HDMI].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__SS] = {
        .name = modnode[MOD__SS].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__SS].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__TV24M] = {
        .name = modnode[MOD__TV24M].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__TV24M].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__CVBS_CLK108M] = {
        .name = modnode[MOD__CVBS_CLK108M].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__CVBS_CLK108M].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
    [CLOCK__MAX + MOD__TVOUT] = {
        .name = modnode[MOD__TVOUT].modname,
        .ops = &clk_ops_gate_module,
        .hw = &clk_foo_modules[MOD__TVOUT].hw,
        .parent_names = parent_name_CMUMOD_DEVCLKS,
        .num_parents = 1,
        .flags = 0,
    },
};

static struct clk_lookup lookup_table[CLOCK__MAX + MOD__MAX_IN_CLK] = {
    [CLOCK__HOSC] = {
        .con_id = clocks[CLOCK__HOSC].name,
        .clk = &owl_clks[CLOCK__HOSC],
    },
    [CLOCK__IC_32K] = {
        .con_id = clocks[CLOCK__IC_32K].name,
        .clk = &owl_clks[CLOCK__IC_32K],
    },
    [CLOCK__COREPLL] = {
        .con_id = clocks[CLOCK__COREPLL].name,
        .clk = &owl_clks[CLOCK__COREPLL],
    },
    [CLOCK__DEVPLL] = {
        .con_id = clocks[CLOCK__DEVPLL].name,
        .clk = &owl_clks[CLOCK__DEVPLL],
    },
    [CLOCK__DDRPLL] = {
        .con_id = clocks[CLOCK__DDRPLL].name,
        .clk = &owl_clks[CLOCK__DDRPLL],
    },
    [CLOCK__NANDPLL] = {
        .con_id = NULL, /* hide nandpll to ease debug work */
        .clk = &owl_clks[CLOCK__NANDPLL],
    },
    [CLOCK__DISPLAYPLL] = {
        .con_id = clocks[CLOCK__DISPLAYPLL].name,
        .clk = &owl_clks[CLOCK__DISPLAYPLL],
    },
    [CLOCK__AUDIOPLL] = {
        .con_id = clocks[CLOCK__AUDIOPLL].name,
        .clk = &owl_clks[CLOCK__AUDIOPLL],
    },
    [CLOCK__TVOUTPLL] = {
        .con_id = clocks[CLOCK__TVOUTPLL].name,
        .clk = &owl_clks[CLOCK__TVOUTPLL],
    },
    [CLOCK__ETHERNETPLL] = {
        .con_id = clocks[CLOCK__ETHERNETPLL].name,
        .clk = &owl_clks[CLOCK__ETHERNETPLL],
    },
    [CLOCK__CVBSPLL] = {
        .con_id = clocks[CLOCK__CVBSPLL].name,
        .clk = &owl_clks[CLOCK__CVBSPLL],
    },
    [CLOCK__DEV_CLK] = {
        .con_id = clocks[CLOCK__DEV_CLK].name,
        .clk = &owl_clks[CLOCK__DEV_CLK],
    },
    [CLOCK__DDR_CLK_0] = {
        .con_id = clocks[CLOCK__DDR_CLK_0].name,
        .clk = &owl_clks[CLOCK__DDR_CLK_0],
    },
    [CLOCK__DDR_CLK_90] = {
        .con_id = clocks[CLOCK__DDR_CLK_90].name,
        .clk = &owl_clks[CLOCK__DDR_CLK_90],
    },
    [CLOCK__DDR_CLK_180] = {
        .con_id = clocks[CLOCK__DDR_CLK_180].name,
        .clk = &owl_clks[CLOCK__DDR_CLK_180],
    },
    [CLOCK__DDR_CLK_270] = {
        .con_id = clocks[CLOCK__DDR_CLK_270].name,
        .clk = &owl_clks[CLOCK__DDR_CLK_270],
    },
    [CLOCK__DDR_CLK_CH0] = {
        .con_id = clocks[CLOCK__DDR_CLK_CH0].name,
        .clk = &owl_clks[CLOCK__DDR_CLK_CH0],
    },
    [CLOCK__DDR_CLK_CH1] = {
        .con_id = clocks[CLOCK__DDR_CLK_CH1].name,
        .clk = &owl_clks[CLOCK__DDR_CLK_CH1],
    },
    [CLOCK__DDR_CLK] = {
        .con_id = clocks[CLOCK__DDR_CLK].name,
        .clk = &owl_clks[CLOCK__DDR_CLK],
    },
    [CLOCK__SPDIF_CLK] = {
        .con_id = clocks[CLOCK__SPDIF_CLK].name,
        .clk = &owl_clks[CLOCK__SPDIF_CLK],
    },
    [CLOCK__HDMIA_CLK] = {
        .con_id = clocks[CLOCK__HDMIA_CLK].name,
        .clk = &owl_clks[CLOCK__HDMIA_CLK],
    },
    [CLOCK__I2SRX_CLK] = {
        .con_id = clocks[CLOCK__I2SRX_CLK].name,
        .clk = &owl_clks[CLOCK__I2SRX_CLK],
    },
    [CLOCK__I2STX_CLK] = {
        .con_id = clocks[CLOCK__I2STX_CLK].name,
        .clk = &owl_clks[CLOCK__I2STX_CLK],
    },
    [CLOCK__PCM0_CLK] = {
        .con_id = clocks[CLOCK__PCM0_CLK].name,
        .clk = &owl_clks[CLOCK__PCM0_CLK],
    },
    [CLOCK__PCM1_CLK] = {
        .con_id = clocks[CLOCK__PCM1_CLK].name,
        .clk = &owl_clks[CLOCK__PCM1_CLK],
    },
    [CLOCK__CLK_PIXEL] = {
        .con_id = clocks[CLOCK__CLK_PIXEL].name,
        .clk = &owl_clks[CLOCK__CLK_PIXEL],
    },
    [CLOCK__CLK_TMDS] = {
        .con_id = clocks[CLOCK__CLK_TMDS].name,
        .clk = &owl_clks[CLOCK__CLK_TMDS],
    },
    [CLOCK__CLK_TMDS_PHY_P] = {
        .con_id = clocks[CLOCK__CLK_TMDS_PHY_P].name,
        .clk = &owl_clks[CLOCK__CLK_TMDS_PHY_P],
    },
    [CLOCK__L2_NIC_CLK] = {
        .con_id = clocks[CLOCK__L2_NIC_CLK].name,
        .clk = &owl_clks[CLOCK__L2_NIC_CLK],
    },
    [CLOCK__APBDBG_CLK] = {
        .con_id = clocks[CLOCK__APBDBG_CLK].name,
        .clk = &owl_clks[CLOCK__APBDBG_CLK],
    },
    [CLOCK__L2_CLK] = {
        .con_id = clocks[CLOCK__L2_CLK].name,
        .clk = &owl_clks[CLOCK__L2_CLK],
    },
    [CLOCK__ACP_CLK] = {
        .con_id = clocks[CLOCK__ACP_CLK].name,
        .clk = &owl_clks[CLOCK__ACP_CLK],
    },
    [CLOCK__PERIPH_CLK] = {
        .con_id = clocks[CLOCK__PERIPH_CLK].name,
        .clk = &owl_clks[CLOCK__PERIPH_CLK],
    },
    [CLOCK__NIC_DIV_CLK] = {
        .con_id = clocks[CLOCK__NIC_DIV_CLK].name,
        .clk = &owl_clks[CLOCK__NIC_DIV_CLK],
    },
    [CLOCK__NIC_CLK] = {
        .con_id = clocks[CLOCK__NIC_CLK].name,
        .clk = &owl_clks[CLOCK__NIC_CLK],
    },
    [CLOCK__AHBPREDIV_CLK] = {
        .con_id = clocks[CLOCK__AHBPREDIV_CLK].name,
        .clk = &owl_clks[CLOCK__AHBPREDIV_CLK],
    },
    [CLOCK__H_CLK] = {
        .con_id = clocks[CLOCK__H_CLK].name,
        .clk = &owl_clks[CLOCK__H_CLK],
    },
    [CLOCK__APB30_CLK] = {
        .con_id = clocks[CLOCK__APB30_CLK].name,
        .clk = &owl_clks[CLOCK__APB30_CLK],
    },
    [CLOCK__APB20_CLK] = {
        .con_id = clocks[CLOCK__APB20_CLK].name,
        .clk = &owl_clks[CLOCK__APB20_CLK],
    },
    [CLOCK__AHB_CLK] = {
        .con_id = clocks[CLOCK__AHB_CLK].name,
        .clk = &owl_clks[CLOCK__AHB_CLK],
    },
    [CLOCK__CORE_CLK] = {
        .con_id = clocks[CLOCK__CORE_CLK].name,
        .clk = &owl_clks[CLOCK__CORE_CLK],
    },
    [CLOCK__CPU_CLK] = {
        .con_id = clocks[CLOCK__CPU_CLK].name,
        .clk = &owl_clks[CLOCK__CPU_CLK],
    },
    [CLOCK__SENSOR_CLKOUT0] = {
        .con_id = clocks[CLOCK__SENSOR_CLKOUT0].name,
        .clk = &owl_clks[CLOCK__SENSOR_CLKOUT0],
    },
    [CLOCK__SENSOR_CLKOUT1] = {
        .con_id = clocks[CLOCK__SENSOR_CLKOUT1].name,
        .clk = &owl_clks[CLOCK__SENSOR_CLKOUT1],
    },
    [CLOCK__LCD_CLK] = {
        .con_id = clocks[CLOCK__LCD_CLK].name,
        .clk = &owl_clks[CLOCK__LCD_CLK],
    },
    [CLOCK__LVDS_CLK] = {
        .con_id = clocks[CLOCK__LVDS_CLK].name,
        .clk = &owl_clks[CLOCK__LVDS_CLK],
    },
    [CLOCK__CKA_LCD_H] = {
        .con_id = clocks[CLOCK__CKA_LCD_H].name,
        .clk = &owl_clks[CLOCK__CKA_LCD_H],
    },
    [CLOCK__LCD1_CLK] = {
        .con_id = clocks[CLOCK__LCD1_CLK].name,
        .clk = &owl_clks[CLOCK__LCD1_CLK],
    },
    [CLOCK__LCD0_CLK] = {
        .con_id = clocks[CLOCK__LCD0_CLK].name,
        .clk = &owl_clks[CLOCK__LCD0_CLK],
    },
    [CLOCK__DSI_HCLK] = {
        .con_id = clocks[CLOCK__DSI_HCLK].name,
        .clk = &owl_clks[CLOCK__DSI_HCLK],
    },
    [CLOCK__DSI_HCLK90] = {
        .con_id = clocks[CLOCK__DSI_HCLK90].name,
        .clk = &owl_clks[CLOCK__DSI_HCLK90],
    },
    [CLOCK__PRO_CLK] = {
        .con_id = clocks[CLOCK__PRO_CLK].name,
        .clk = &owl_clks[CLOCK__PRO_CLK],
    },
    [CLOCK__PHY_CLK] = {
        .con_id = clocks[CLOCK__PHY_CLK].name,
        .clk = &owl_clks[CLOCK__PHY_CLK],
    },
    [CLOCK__CSI_CLK] = {
        .con_id = clocks[CLOCK__CSI_CLK].name,
        .clk = &owl_clks[CLOCK__CSI_CLK],
    },
    [CLOCK__DE1_CLK] = {
        .con_id = clocks[CLOCK__DE1_CLK].name,
        .clk = &owl_clks[CLOCK__DE1_CLK],
    },
    [CLOCK__DE2_CLK] = {
        .con_id = clocks[CLOCK__DE2_CLK].name,
        .clk = &owl_clks[CLOCK__DE2_CLK],
    },
    [CLOCK__BISP_CLK] = {
        .con_id = clocks[CLOCK__BISP_CLK].name,
        .clk = &owl_clks[CLOCK__BISP_CLK],
    },
    [CLOCK__ISPBP_CLK] = {
        .con_id = clocks[CLOCK__ISPBP_CLK].name,
        .clk = &owl_clks[CLOCK__ISPBP_CLK],
    },
    [CLOCK__IMG5_CLK] = {
        .con_id = clocks[CLOCK__IMG5_CLK].name,
        .clk = &owl_clks[CLOCK__IMG5_CLK],
    },
    [CLOCK__VDE_CLK] = {
        .con_id = clocks[CLOCK__VDE_CLK].name,
        .clk = &owl_clks[CLOCK__VDE_CLK],
    },
    [CLOCK__VCE_CLK] = {
        .con_id = clocks[CLOCK__VCE_CLK].name,
        .clk = &owl_clks[CLOCK__VCE_CLK],
    },
    [CLOCK__NANDC_CLK] = {
        .con_id = clocks[CLOCK__NANDC_CLK].name,
        .clk = &owl_clks[CLOCK__NANDC_CLK],
    },
    [CLOCK__ECC_CLK] = {
        .con_id = clocks[CLOCK__ECC_CLK].name,
        .clk = &owl_clks[CLOCK__ECC_CLK],
    },
    [CLOCK__PRESD0_CLK] = {
        .con_id = clocks[CLOCK__PRESD0_CLK].name,
        .clk = &owl_clks[CLOCK__PRESD0_CLK],
    },
    [CLOCK__PRESD1_CLK] = {
        .con_id = clocks[CLOCK__PRESD1_CLK].name,
        .clk = &owl_clks[CLOCK__PRESD1_CLK],
    },
    [CLOCK__PRESD2_CLK] = {
        .con_id = clocks[CLOCK__PRESD2_CLK].name,
        .clk = &owl_clks[CLOCK__PRESD2_CLK],
    },
    [CLOCK__SD0_CLK_2X] = {
        .con_id = clocks[CLOCK__SD0_CLK_2X].name,
        .clk = &owl_clks[CLOCK__SD0_CLK_2X],
    },
    [CLOCK__SD1_CLK_2X] = {
        .con_id = clocks[CLOCK__SD1_CLK_2X].name,
        .clk = &owl_clks[CLOCK__SD1_CLK_2X],
    },
    [CLOCK__SD2_CLK_2X] = {
        .con_id = clocks[CLOCK__SD2_CLK_2X].name,
        .clk = &owl_clks[CLOCK__SD2_CLK_2X],
    },
    [CLOCK__SD0_CLK] = {
        .con_id = clocks[CLOCK__SD0_CLK].name,
        .clk = &owl_clks[CLOCK__SD0_CLK],
    },
    [CLOCK__SD1_CLK] = {
        .con_id = clocks[CLOCK__SD1_CLK].name,
        .clk = &owl_clks[CLOCK__SD1_CLK],
    },
    [CLOCK__SD2_CLK] = {
        .con_id = clocks[CLOCK__SD2_CLK].name,
        .clk = &owl_clks[CLOCK__SD2_CLK],
    },
    [CLOCK__UART0_CLK] = {
        .con_id = clocks[CLOCK__UART0_CLK].name,
        .clk = &owl_clks[CLOCK__UART0_CLK],
    },
    [CLOCK__UART1_CLK] = {
        .con_id = clocks[CLOCK__UART1_CLK].name,
        .clk = &owl_clks[CLOCK__UART1_CLK],
    },
    [CLOCK__UART2_CLK] = {
        .con_id = clocks[CLOCK__UART2_CLK].name,
        .clk = &owl_clks[CLOCK__UART2_CLK],
    },
    [CLOCK__UART3_CLK] = {
        .con_id = clocks[CLOCK__UART3_CLK].name,
        .clk = &owl_clks[CLOCK__UART3_CLK],
    },
    [CLOCK__UART4_CLK] = {
        .con_id = clocks[CLOCK__UART4_CLK].name,
        .clk = &owl_clks[CLOCK__UART4_CLK],
    },
    [CLOCK__UART5_CLK] = {
        .con_id = clocks[CLOCK__UART5_CLK].name,
        .clk = &owl_clks[CLOCK__UART5_CLK],
    },
    [CLOCK__UART6_CLK] = {
        .con_id = clocks[CLOCK__UART6_CLK].name,
        .clk = &owl_clks[CLOCK__UART6_CLK],
    },
    [CLOCK__PWM0_CLK] = {
        .con_id = clocks[CLOCK__PWM0_CLK].name,
        .clk = &owl_clks[CLOCK__PWM0_CLK],
    },
    [CLOCK__PWM1_CLK] = {
        .con_id = clocks[CLOCK__PWM1_CLK].name,
        .clk = &owl_clks[CLOCK__PWM1_CLK],
    },
    [CLOCK__PWM2_CLK] = {
        .con_id = clocks[CLOCK__PWM2_CLK].name,
        .clk = &owl_clks[CLOCK__PWM2_CLK],
    },
    [CLOCK__PWM3_CLK] = {
        .con_id = clocks[CLOCK__PWM3_CLK].name,
        .clk = &owl_clks[CLOCK__PWM3_CLK],
    },
    [CLOCK__PWM4_CLK] = {
        .con_id = clocks[CLOCK__PWM4_CLK].name,
        .clk = &owl_clks[CLOCK__PWM4_CLK],
    },
    [CLOCK__PWM5_CLK] = {
        .con_id = clocks[CLOCK__PWM5_CLK].name,
        .clk = &owl_clks[CLOCK__PWM5_CLK],
    },
    [CLOCK__RMII_REF_CLK] = {
        .con_id = clocks[CLOCK__RMII_REF_CLK].name,
        .clk = &owl_clks[CLOCK__RMII_REF_CLK],
    },
    [CLOCK__I2C0_CLK] = {
        .con_id = clocks[CLOCK__I2C0_CLK].name,
        .clk = &owl_clks[CLOCK__I2C0_CLK],
    },
    [CLOCK__I2C1_CLK] = {
        .con_id = clocks[CLOCK__I2C1_CLK].name,
        .clk = &owl_clks[CLOCK__I2C1_CLK],
    },
    [CLOCK__I2C2_CLK] = {
        .con_id = clocks[CLOCK__I2C2_CLK].name,
        .clk = &owl_clks[CLOCK__I2C2_CLK],
    },
    [CLOCK__I2C3_CLK] = {
        .con_id = clocks[CLOCK__I2C3_CLK].name,
        .clk = &owl_clks[CLOCK__I2C3_CLK],
    },
    [CLOCK__25M_CLK] = {
        .con_id = clocks[CLOCK__25M_CLK].name,
        .clk = &owl_clks[CLOCK__25M_CLK],
    },
    [CLOCK__LENS_CLK] = {
        .con_id = clocks[CLOCK__LENS_CLK].name,
        .clk = &owl_clks[CLOCK__LENS_CLK],
    },
    [CLOCK__HDMI24M] = {
        .con_id = clocks[CLOCK__HDMI24M].name,
        .clk = &owl_clks[CLOCK__HDMI24M],
    },
    [CLOCK__TIMER_CLK] = {
        .con_id = clocks[CLOCK__TIMER_CLK].name,
        .clk = &owl_clks[CLOCK__TIMER_CLK],
    },
    [CLOCK__SS_CLK] = {
        .con_id = clocks[CLOCK__SS_CLK].name,
        .clk = &owl_clks[CLOCK__SS_CLK],
    },
    [CLOCK__SPS_CLK] = {
        .con_id = clocks[CLOCK__SPS_CLK].name,
        .clk = &owl_clks[CLOCK__SPS_CLK],
    },
    [CLOCK__IRC_CLK] = {
        .con_id = clocks[CLOCK__IRC_CLK].name,
        .clk = &owl_clks[CLOCK__IRC_CLK],
    },
    [CLOCK__TV24M] = {
        .con_id = clocks[CLOCK__TV24M].name,
        .clk = &owl_clks[CLOCK__TV24M],
    },
    [CLOCK__CVBS_CLK108M] = {
        .con_id = clocks[CLOCK__CVBS_CLK108M].name,
        .clk = &owl_clks[CLOCK__CVBS_CLK108M],
    },
    [CLOCK__MIPI24M] = {
        .con_id = clocks[CLOCK__MIPI24M].name,
        .clk = &owl_clks[CLOCK__MIPI24M],
    },
    [CLOCK__LENS24M] = {
        .con_id = clocks[CLOCK__LENS24M].name,
        .clk = &owl_clks[CLOCK__LENS24M],
    },
    [CLOCK__GPU3D_SYSCLK] = {
        .con_id = clocks[CLOCK__GPU3D_SYSCLK].name,
        .clk = &owl_clks[CLOCK__GPU3D_SYSCLK],
    },
    [CLOCK__GPU3D_HYDCLK] = {
        .con_id = clocks[CLOCK__GPU3D_HYDCLK].name,
        .clk = &owl_clks[CLOCK__GPU3D_HYDCLK],
    },
    [CLOCK__GPU3D_NIC_MEMCLK] = {
        .con_id = clocks[CLOCK__GPU3D_NIC_MEMCLK].name,
        .clk = &owl_clks[CLOCK__GPU3D_NIC_MEMCLK],
    },
    [CLOCK__GPU3D_CORECLK] = {
        .con_id = clocks[CLOCK__GPU3D_CORECLK].name,
        .clk = &owl_clks[CLOCK__GPU3D_CORECLK],
    },
    [CLOCK__MAX + MOD__ROOT] = {
        .con_id = modnode[MOD__ROOT].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__ROOT],
    },
    [CLOCK__MAX + MOD__GPU3D] = {
        .con_id = modnode[MOD__GPU3D].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__GPU3D],
    },
    [CLOCK__MAX + MOD__SHARESRAM] = {
        .con_id = modnode[MOD__SHARESRAM].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__SHARESRAM],
    },
    [CLOCK__MAX + MOD__HDCP2X] = {
        .con_id = modnode[MOD__HDCP2X].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__HDCP2X],
    },
    [CLOCK__MAX + MOD__VCE] = {
        .con_id = modnode[MOD__VCE].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__VCE],
    },
    [CLOCK__MAX + MOD__VDE] = {
        .con_id = modnode[MOD__VDE].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__VDE],
    },
    [CLOCK__MAX + MOD__PCM0] = {
        .con_id = modnode[MOD__PCM0].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__PCM0],
    },
    [CLOCK__MAX + MOD__SPDIF] = {
        .con_id = modnode[MOD__SPDIF].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__SPDIF],
    },
    [CLOCK__MAX + MOD__HDMIA] = {
        .con_id = modnode[MOD__HDMIA].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__HDMIA],
    },
    [CLOCK__MAX + MOD__I2SRX] = {
        .con_id = modnode[MOD__I2SRX].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__I2SRX],
    },
    [CLOCK__MAX + MOD__I2STX] = {
        .con_id = modnode[MOD__I2STX].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__I2STX],
    },
    [CLOCK__MAX + MOD__GPIO] = {
        .con_id = modnode[MOD__GPIO].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__GPIO],
    },
    [CLOCK__MAX + MOD__KEY] = {
        .con_id = modnode[MOD__KEY].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__KEY],
    },
    [CLOCK__MAX + MOD__LENS] = {
        .con_id = modnode[MOD__LENS].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__LENS],
    },
    [CLOCK__MAX + MOD__BISP] = {
        .con_id = modnode[MOD__BISP].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__BISP],
    },
    [CLOCK__MAX + MOD__CSI] = {
        .con_id = modnode[MOD__CSI].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__CSI],
    },
    [CLOCK__MAX + MOD__DSI] = {
        .con_id = modnode[MOD__DSI].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__DSI],
    },
    [CLOCK__MAX + MOD__LVDS] = {
        .con_id = modnode[MOD__LVDS].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__LVDS],
    },
    [CLOCK__MAX + MOD__LCD1] = {
        .con_id = modnode[MOD__LCD1].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__LCD1],
    },
    [CLOCK__MAX + MOD__LCD0] = {
        .con_id = modnode[MOD__LCD0].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__LCD0],
    },
    [CLOCK__MAX + MOD__DE] = {
        .con_id = modnode[MOD__DE].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__DE],
    },
    [CLOCK__MAX + MOD__SD2] = {
        .con_id = modnode[MOD__SD2].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__SD2],
    },
    [CLOCK__MAX + MOD__SD1] = {
        .con_id = modnode[MOD__SD1].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__SD1],
    },
    [CLOCK__MAX + MOD__SD0] = {
        .con_id = modnode[MOD__SD0].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__SD0],
    },
    [CLOCK__MAX + MOD__NANDC] = {
        .con_id = modnode[MOD__NANDC].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__NANDC],
    },
    [CLOCK__MAX + MOD__DDRCH0] = {
        .con_id = modnode[MOD__DDRCH0].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__DDRCH0],
    },
    [CLOCK__MAX + MOD__NOR] = {
        .con_id = modnode[MOD__NOR].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__NOR],
    },
    [CLOCK__MAX + MOD__DMAC] = {
        .con_id = modnode[MOD__DMAC].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__DMAC],
    },
    [CLOCK__MAX + MOD__DDRCH1] = {
        .con_id = modnode[MOD__DDRCH1].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__DDRCH1],
    },
    [CLOCK__MAX + MOD__I2C3] = {
        .con_id = modnode[MOD__I2C3].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__I2C3],
    },
    [CLOCK__MAX + MOD__I2C2] = {
        .con_id = modnode[MOD__I2C2].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__I2C2],
    },
    [CLOCK__MAX + MOD__TIMER] = {
        .con_id = modnode[MOD__TIMER].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__TIMER],
    },
    [CLOCK__MAX + MOD__PWM5] = {
        .con_id = modnode[MOD__PWM5].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__PWM5],
    },
    [CLOCK__MAX + MOD__PWM4] = {
        .con_id = modnode[MOD__PWM4].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__PWM4],
    },
    [CLOCK__MAX + MOD__PWM3] = {
        .con_id = modnode[MOD__PWM3].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__PWM3],
    },
    [CLOCK__MAX + MOD__PWM2] = {
        .con_id = modnode[MOD__PWM2].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__PWM2],
    },
    [CLOCK__MAX + MOD__PWM1] = {
        .con_id = modnode[MOD__PWM1].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__PWM1],
    },
    [CLOCK__MAX + MOD__PWM0] = {
        .con_id = modnode[MOD__PWM0].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__PWM0],
    },
    [CLOCK__MAX + MOD__ETHERNET] = {
        .con_id = modnode[MOD__ETHERNET].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__ETHERNET],
    },
    [CLOCK__MAX + MOD__UART5] = {
        .con_id = modnode[MOD__UART5].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__UART5],
    },
    [CLOCK__MAX + MOD__UART4] = {
        .con_id = modnode[MOD__UART4].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__UART4],
    },
    [CLOCK__MAX + MOD__UART3] = {
        .con_id = modnode[MOD__UART3].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__UART3],
    },
    [CLOCK__MAX + MOD__UART6] = {
        .con_id = modnode[MOD__UART6].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__UART6],
    },
    [CLOCK__MAX + MOD__PCM1] = {
        .con_id = modnode[MOD__PCM1].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__PCM1],
    },
    [CLOCK__MAX + MOD__I2C1] = {
        .con_id = modnode[MOD__I2C1].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__I2C1],
    },
    [CLOCK__MAX + MOD__I2C0] = {
        .con_id = modnode[MOD__I2C0].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__I2C0],
    },
    [CLOCK__MAX + MOD__SPI3] = {
        .con_id = modnode[MOD__SPI3].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__SPI3],
    },
    [CLOCK__MAX + MOD__SPI2] = {
        .con_id = modnode[MOD__SPI2].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__SPI2],
    },
    [CLOCK__MAX + MOD__SPI1] = {
        .con_id = modnode[MOD__SPI1].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__SPI1],
    },
    [CLOCK__MAX + MOD__SPI0] = {
        .con_id = modnode[MOD__SPI0].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__SPI0],
    },
    [CLOCK__MAX + MOD__IRC] = {
        .con_id = modnode[MOD__IRC].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__IRC],
    },
    [CLOCK__MAX + MOD__UART2] = {
        .con_id = modnode[MOD__UART2].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__UART2],
    },
    [CLOCK__MAX + MOD__UART1] = {
        .con_id = modnode[MOD__UART1].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__UART1],
    },
    [CLOCK__MAX + MOD__UART0] = {
        .con_id = modnode[MOD__UART0].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__UART0],
    },
    [CLOCK__MAX + MOD__HDMI] = {
        .con_id = modnode[MOD__HDMI].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__HDMI],
    },
    [CLOCK__MAX + MOD__SS] = {
        .con_id = modnode[MOD__SS].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__SS],
    },
    [CLOCK__MAX + MOD__TV24M] = {
        .con_id = modnode[MOD__TV24M].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__TV24M],
    },
    [CLOCK__MAX + MOD__CVBS_CLK108M] = {
        .con_id = modnode[MOD__CVBS_CLK108M].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__CVBS_CLK108M],
    },
    [CLOCK__MAX + MOD__TVOUT] = {
        .con_id = modnode[MOD__TVOUT].modname,
        .clk = &owl_clks[CLOCK__MAX + MOD__TVOUT],
    },
};

static struct clocks_table clks_table = 
{
    .clocks = clocks,
    .rvregs = rvregs,
    .pllnode = pllnode,
    .owl_clks = owl_clks,
    .modnode = modnode,
    .clk_foo_clocks = clk_foo_clocks,
    .lookup_table = lookup_table,
};

struct clocks_table * atm7059_get_clocktree(void)
{
	return &clks_table;
}

void atm7059_init_clocktree(void)
{
	int i;
	int pll;
	int frequency = 0;

	owl_clk_config_recursion(CLOCK__LCD0_CLK, 1);
	owl_clk_config_recursion(CLOCK__LCD1_CLK, 1);
	owl_clk_config_recursion(CLOCK__IMG5_CLK, 1);
	owl_clk_config_recursion(CLOCK__DSI_HCLK90, 1);
	owl_clk_config_recursion(CLOCK__PRO_CLK, 1);
	owl_clk_config_recursion(CLOCK__PHY_CLK, 1);
	owl_clk_config_recursion(CLOCK__LCD_CLK, 2);
	owl_clk_config_recursion(CLOCK__DSI_HCLK, 2);

    clocks[CLOCK__L2_NIC_CLK].actdiv            = NULL;
    clocks[CLOCK__L2_NIC_CLK].divider           = 1;
    clocks[CLOCK__L2_NIC_CLK].harddivider       = 1;
    clocks[CLOCK__L2_CLK].actdiv                = NULL;
    clocks[CLOCK__L2_CLK].divider               = 1;
    clocks[CLOCK__L2_CLK].harddivider           = 1;
    
    clocks[CLOCK__CLK_PIXEL].actdiv             = NULL;
    clocks[CLOCK__CLK_PIXEL].divider            = 1;
	clocks[CLOCK__CLK_PIXEL].harddivider        = 1;
    clocks[CLOCK__CLK_TMDS].actdiv              = NULL;
	clocks[CLOCK__CLK_TMDS].divider             = 1;
	clocks[CLOCK__CLK_TMDS].harddivider         = 1;
	
	clocks[CLOCK__PCM0_CLK].divider             = 2;
	clocks[CLOCK__PCM1_CLK].divider             = 2;
	clocks[CLOCK__PHY_CLK].divider              = 4;
	clocks[CLOCK__SD0_CLK].divider              = 2;
	clocks[CLOCK__SD1_CLK].divider              = 2;
	clocks[CLOCK__SD2_CLK].divider              = 2;
	clocks[CLOCK__I2C0_CLK].divider             = 5;
	clocks[CLOCK__I2C1_CLK].divider             = 5;
	clocks[CLOCK__I2C2_CLK].divider             = 5;
	clocks[CLOCK__I2C3_CLK].divider             = 5;
	clocks[CLOCK__25M_CLK].divider              = 20;
	clocks[CLOCK__IRC_CLK].divider              = 120;
	clocks[CLOCK__SPS_CLK].divider              = 12;
                                                
	clocks[CLOCK__PCM0_CLK].harddivider         = 2;
	clocks[CLOCK__PCM1_CLK].harddivider         = 2;
	clocks[CLOCK__PHY_CLK].harddivider          = 4;
	clocks[CLOCK__SD0_CLK].harddivider          = 2;
	clocks[CLOCK__SD1_CLK].harddivider          = 2;
	clocks[CLOCK__SD2_CLK].harddivider          = 2;
	clocks[CLOCK__I2C0_CLK].harddivider         = 5;
	clocks[CLOCK__I2C1_CLK].harddivider         = 5;
	clocks[CLOCK__I2C2_CLK].harddivider         = 5;
	clocks[CLOCK__I2C3_CLK].harddivider         = 5;
	clocks[CLOCK__25M_CLK].harddivider          = 20;
	clocks[CLOCK__IRC_CLK].harddivider          = 120;
	clocks[CLOCK__SPS_CLK].harddivider          = 12;
                                                
	clocks[CLOCK__HOSC].harddivider             = 1;
	clocks[CLOCK__IC_32K].harddivider           = 1;
	clocks[CLOCK__DEV_CLK].harddivider          = 1;
	clocks[CLOCK__DDR_CLK_0].harddivider        = 1;
	clocks[CLOCK__DDR_CLK_90].harddivider       = 1;
	clocks[CLOCK__DDR_CLK_180].harddivider      = 1;
	clocks[CLOCK__DDR_CLK_270].harddivider      = 1;
	clocks[CLOCK__DDR_CLK_CH0].harddivider      = 1;
	clocks[CLOCK__DDR_CLK_CH1].harddivider      = 1;
	clocks[CLOCK__DDR_CLK].harddivider          = 1;
	clocks[CLOCK__CLK_TMDS_PHY_P].harddivider   = 1;
	clocks[CLOCK__AHB_CLK].harddivider          = 1;
	clocks[CLOCK__CORE_CLK].harddivider         = 1;
	clocks[CLOCK__CPU_CLK].harddivider          = 1;
	clocks[CLOCK__LVDS_CLK].harddivider         = 1;
	clocks[CLOCK__CKA_LCD_H].harddivider        = 1;
	clocks[CLOCK__ISPBP_CLK].harddivider        = 1;
	clocks[CLOCK__IMG5_CLK].harddivider         = 1;
	clocks[CLOCK__HDMI24M].harddivider          = 1;
	clocks[CLOCK__TIMER_CLK].harddivider        = 1;
	clocks[CLOCK__TV24M].harddivider            = 1;
	clocks[CLOCK__CVBS_CLK108M].harddivider            = 1;
	clocks[CLOCK__MIPI24M].harddivider          = 1;
	clocks[CLOCK__LENS24M].harddivider          = 1;
	clocks[CLOCK__DSI_HCLK90].harddivider       = 1;

	clocks[CLOCK__COREPLL].harddivider          = 1;
	clocks[CLOCK__DEVPLL].harddivider           = 1;
	clocks[CLOCK__DDRPLL].harddivider           = 1;
	clocks[CLOCK__NANDPLL].harddivider          = 1;
	clocks[CLOCK__DISPLAYPLL].harddivider       = 1;
	clocks[CLOCK__AUDIOPLL].harddivider         = 1;
	clocks[CLOCK__TVOUTPLL].harddivider         = 1;
	clocks[CLOCK__ETHERNETPLL].harddivider      = 1;
	clocks[CLOCK__CVBSPLL].harddivider      = 1;

	clocks[CLOCK__SPDIF_CLK].actdiv             = &divider_SPDIF_CLK;
	clocks[CLOCK__HDMIA_CLK].actdiv             = &divider_HDMIA_CLK;
	clocks[CLOCK__I2SRX_CLK].actdiv             = &divider_I2SRX_CLK;
	clocks[CLOCK__I2STX_CLK].actdiv             = &divider_I2STX_CLK;
	clocks[CLOCK__APBDBG_CLK].actdiv            = &divider_APBDBG_CLK;
	clocks[CLOCK__ACP_CLK].actdiv               = &divider_ACP_CLK;
	clocks[CLOCK__PERIPH_CLK].actdiv            = &divider_PERIPH_CLK;
	clocks[CLOCK__NIC_DIV_CLK].actdiv           = &divider_NIC_DIV_CLK;
	clocks[CLOCK__NIC_CLK].actdiv               = &divider_NIC_CLK;
	clocks[CLOCK__AHBPREDIV_CLK].actdiv         = &divider_AHBPREDIV_CLK;
	clocks[CLOCK__H_CLK].actdiv                 = &divider_H_CLK;
	clocks[CLOCK__APB30_CLK].actdiv             = &divider_APB30_CLK;
	clocks[CLOCK__APB20_CLK].actdiv             = &divider_APB20_CLK;
	clocks[CLOCK__SENSOR_CLKOUT0].actdiv        = &divider_SENSOR_CLKOUT0;
	clocks[CLOCK__SENSOR_CLKOUT1].actdiv        = &divider_SENSOR_CLKOUT1;
	clocks[CLOCK__LCD_CLK].actdiv               = &divider_LCD_CLK;
	clocks[CLOCK__LCD1_CLK].actdiv              = &divider_LCD1_CLK;
	clocks[CLOCK__LCD0_CLK].actdiv              = &divider_LCD0_CLK;
	clocks[CLOCK__DSI_HCLK].actdiv              = &divider_DSI_HCLK;
	clocks[CLOCK__PRO_CLK].actdiv               = &divider_PRO_CLK;
	clocks[CLOCK__CSI_CLK].actdiv               = &divider_CSI_CLK;
	clocks[CLOCK__DE1_CLK].actdiv               = &divider_DE1_CLK;
	clocks[CLOCK__DE2_CLK].actdiv               = &divider_DE2_CLK;
	clocks[CLOCK__BISP_CLK].actdiv              = &divider_BISP_CLK;
	clocks[CLOCK__VDE_CLK].actdiv               = &divider_VDE_CLK;
	clocks[CLOCK__VCE_CLK].actdiv               = &divider_VCE_CLK;
	clocks[CLOCK__NANDC_CLK].actdiv             = &divider_NANDC_CLK;
	clocks[CLOCK__ECC_CLK].actdiv               = &divider_ECC_CLK;
	clocks[CLOCK__PRESD0_CLK].actdiv            = &divider_PRESD0_CLK;
	clocks[CLOCK__PRESD1_CLK].actdiv            = &divider_PRESD1_CLK;
	clocks[CLOCK__PRESD2_CLK].actdiv            = &divider_PRESD2_CLK;
	clocks[CLOCK__SD0_CLK_2X].actdiv            = &divider_SD0_CLK_2X;
	clocks[CLOCK__SD1_CLK_2X].actdiv            = &divider_SD1_CLK_2X;
	clocks[CLOCK__SD2_CLK_2X].actdiv            = &divider_SD2_CLK_2X;
	clocks[CLOCK__UART0_CLK].actdiv             = &divider_UART0_CLK;
	clocks[CLOCK__UART1_CLK].actdiv             = &divider_UART1_CLK;
	clocks[CLOCK__UART2_CLK].actdiv             = &divider_UART2_CLK;
	clocks[CLOCK__UART3_CLK].actdiv             = &divider_UART3_CLK;
	clocks[CLOCK__UART4_CLK].actdiv             = &divider_UART4_CLK;
	clocks[CLOCK__UART5_CLK].actdiv             = &divider_UART5_CLK;
	clocks[CLOCK__UART6_CLK].actdiv             = &divider_UART6_CLK;
	clocks[CLOCK__PWM0_CLK].actdiv              = &divider_PWM0_CLK;
	clocks[CLOCK__PWM1_CLK].actdiv              = &divider_PWM1_CLK;
	clocks[CLOCK__PWM2_CLK].actdiv              = &divider_PWM2_CLK;
	clocks[CLOCK__PWM3_CLK].actdiv              = &divider_PWM3_CLK;
	clocks[CLOCK__PWM4_CLK].actdiv              = &divider_PWM4_CLK;
	clocks[CLOCK__PWM5_CLK].actdiv              = &divider_PWM5_CLK;
	clocks[CLOCK__RMII_REF_CLK].actdiv          = &divider_RMII_REF_CLK;
	clocks[CLOCK__LENS_CLK].actdiv              = &divider_LENS_CLK;
	clocks[CLOCK__GPU3D_SYSCLK].actdiv          = &divider_GPU3D_SYSCLK;
	clocks[CLOCK__GPU3D_HYDCLK].actdiv          = &divider_GPU3D_HYDCLK;
	clocks[CLOCK__GPU3D_NIC_MEMCLK].actdiv      = &divider_GPU3D_NIC_MEMCLK;
	clocks[CLOCK__GPU3D_CORECLK].actdiv         = &divider_GPU3D_CORECLK;
	clocks[CLOCK__SS_CLK].actdiv                = &divider_SS_CLK;

	for (i = 0; i < CLOCK__MAX; i++) {
		/*
		 * read pll frequency from cmu regs
		 * TVOUTPLL must read before DEEPCOLORPLL
		 */
		if (clocks[i].type == TYPE_PLL) {
			pll = i - CLOCK__COREPLL;
			if (pllnode[pll].reg_pllfreq) {
				pllnode[pll].sel = read_clkreg_val(pllnode[pll].reg_pllfreq);
			} else
				pllnode[pll].sel = 0;

			switch (pllnode[pll].type) {
			case PLL_T_D4DYN:
				pllnode[pll].sel |= 4;
			case PLL_T_STEP:
				frequency = (pllnode[pll].sel+pllnode[pll].freq.step.offset) * pllnode[pll].freq.step.step;
				break;
			case PLL_T_FREQ:
				frequency = pllnode[pll].freq.freqtab[pllnode[pll].sel];
				break;
			default:
				break;
			}
			if (pll == PLL__TVOUTPLL && frequency > 4) {
				pllnode[PLL__DEEPCOLORPLL].freq.step.step = frequency / 4;
			}
			clocks[i].frequency = frequency;
		}

		/*
		 * read clock divider from cmu regs
		 */
		write_clkreg_val(&busbit_DIVEN, 0);
		if (clocks[i].actdiv) {
			clocks[i].divsel = read_clkreg_val(clocks[i].actdiv->reg);
			clocks[i].divider = getdivider(clocks[i].actdiv, clocks[i].divsel);
			if (clocks[i].divider < 0) {
				clocks[i].divsel = getdivider_resetval(clocks[i].actdiv);
				write_clkreg_val(clocks[i].actdiv->reg, clocks[i].divsel);
				clocks[i].divider = getdivider(clocks[i].actdiv, clocks[i].divsel);
			}

			if (i == CLOCK__PRO_CLK && (clocks[i].divider & 0xffff0000)) {
				clocks[i].multipler = 3;
				clocks[i].divider = 4;
			}
		}
		write_clkreg_val(&busbit_DIVEN, 1);

		/*
		 * read clock dependence legacy
		 */
		if (clocks[i].reg_srcsel) {
			clocks[i].source_sel = read_clkreg_val(clocks[i].reg_srcsel);
			/* of cause source_sel must < source_lim */
			if (clocks[i].source_sel >= clocks[i].source_lim) {
				printk("error: clock %s parent index error\n",
					clocks[i].name);
				clocks[i].source_sel = 0;
			}
		}

		addclock(i);
		if (clocks[i].type == TYPE_DYNAMIC) {
			clocks[i].changed = 1;
		}
	}
}

void atm7059_prepare_clocktree(void)
{
    int i;

	/* recover refer count from cmu registers */
	owl_clks[CLOCK__IC_32K].prepare_count++;
	owl_clks[CLOCK__IC_32K].enable_count++;

	if (read_clkreg_val(&enablebit_HOSC) == 1) {
		owl_clks[CLOCK__HOSC].prepare_count++;
		owl_clks[CLOCK__HOSC].enable_count++;
	}

	if (owl_clks[CLOCK__HOSC].enable_count) {
		for (i = 0; i < PLL__MAX; i++) {
			if (pllnode[i].reg_pllen && read_clkreg_val(pllnode[i].reg_pllen) == 1) {
				owl_clks[CLOCK__HOSC].prepare_count++;
				owl_clks[CLOCK__HOSC].enable_count++;
				owl_clks[CLOCK__COREPLL + i].prepare_count++;
				owl_clks[CLOCK__COREPLL + i].enable_count++;
			}
		}
	}

	owl_clks[CLOCK__MAX + MOD__ROOT].prepare_count++;
	owl_clks[CLOCK__MAX + MOD__ROOT].enable_count++;
	for (i = 1; i < MOD__MAX_IN_CLK; i++) {
		if (modnode[i].reg_devclken && read_clkreg_val(modnode[i].reg_devclken) == 1) {
			owl_clks[CLOCK__MAX + MOD__ROOT].prepare_count++;
			owl_clks[CLOCK__MAX + MOD__ROOT].enable_count++;
			owl_clks[CLOCK__MAX + i].prepare_count++;
			owl_clks[CLOCK__MAX + i].enable_count++;
		}
	}
}
