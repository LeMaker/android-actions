#include "pinctrl_data-owl.h"
#include <mach/hardware.h>

enum owl_mux {
	OWL_MUX_NOR,
	OWL_MUX_ETH_RMII,
	OWL_MUX_ETH_SMII,
	OWL_MUX_SPI0,
	OWL_MUX_SPI1,
	OWL_MUX_SPI2,
	OWL_MUX_SPI3,
	OWL_MUX_SENS0,
	OWL_MUX_SENS1,
	OWL_MUX_UART0,
	OWL_MUX_UART1,
	OWL_MUX_UART2,
	OWL_MUX_UART3,
	OWL_MUX_UART4,
	OWL_MUX_UART5,
	OWL_MUX_UART6,
	OWL_MUX_I2S0,
	OWL_MUX_I2S1,
	OWL_MUX_PCM1,
	OWL_MUX_PCM0,
	OWL_MUX_KS,
	OWL_MUX_JTAG,
	OWL_MUX_PWM0,
	OWL_MUX_PWM1,
	OWL_MUX_PWM2,
	OWL_MUX_PWM3,
	OWL_MUX_PWM4,
	OWL_MUX_PWM5,
	OWL_MUX_P0,	/*what the hell is this?*/
	OWL_MUX_SD0,
	OWL_MUX_SD1,
	OWL_MUX_SD2,
	OWL_MUX_I2C0,
	OWL_MUX_I2C1,
	OWL_MUX_I2C2,
	OWL_MUX_I2C3,
	OWL_MUX_DSI,
	OWL_MUX_LVDS,
	OWL_MUX_USB30,
	OWL_MUX_CLKO_25M,
	OWL_MUX_MIPI_CSI,
	OWL_MUX_NAND,	
	OWL_MUX_SPDIF,
	OWL_MUX_SIRQ0,
	OWL_MUX_SIRQ1,
	OWL_MUX_SIRQ2,
	OWL_MUX_TS,
	OWL_MUX_LCD0,
	OWL_MUX_MAX,
	OWL_MUX_RESERVED,
};

/*some basic definitions, we want to use this data file in uboot*/
/*so don't include kernel headfiles*/
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define NULL ((void *)0)

/*
 * Most pins affected by the pinmux can also be GPIOs. Define these first.
 * These must match how the GPIO driver names/numbers its pins.
 */

#define _GPIOA(offset)		  (offset)
#define _GPIOB(offset)		  (32 + (offset))
#define _GPIOC(offset)		  (64 + (offset))
#define _GPIOD(offset)		  (96 + (offset))
#define _GPIOE(offset)		  (128 + (offset))

/* All non-GPIO pins follow */
#define NUM_GPIOS               (_GPIOE(3) + 1)
#define _PIN(offset)            (NUM_GPIOS + (offset))

#define P_RESERA0               _GPIOA(0)
#define P_RESERA1               _GPIOA(1)
#define P_RESERA2               _GPIOA(2)
#define P_RESERA3               _GPIOA(3)
#define P_RESERA4               _GPIOA(4)
#define P_RESERA5               _GPIOA(5)
#define P_RESERA6               _GPIOA(6)
#define P_RESERA7               _GPIOA(7)
#define P_RESERA8               _GPIOA(8)
#define P_RESERA9               _GPIOA(9)
#define P_RESERA10              _GPIOA(10)
#define P_RESERA11              _GPIOA(11)

/* Ethernet MAC */
#define P_ETH_TXD0              _GPIOA(14)
#define P_ETH_TXD1              _GPIOA(15)
#define P_ETH_TXEN              _GPIOA(16)
#define P_ETH_RXER              _GPIOA(17)
#define P_ETH_CRS_DV            _GPIOA(18)
#define P_ETH_RXD1              _GPIOA(19)
#define P_ETH_RXD0              _GPIOA(20)
#define P_ETH_REF_CLK           _GPIOA(21)
#define P_ETH_MDC               _GPIOA(22)
#define P_ETH_MDIO              _GPIOA(23)

/* SIRQ */
#define P_SIRQ0                 _GPIOA(24)
#define P_SIRQ1                 _GPIOA(25)
#define P_SIRQ2                 _GPIOA(26)

/* I2S */
#define P_I2S_D0                _GPIOA(27)
#define P_I2S_BCLK0             _GPIOA(28)
#define P_I2S_LRCLK0            _GPIOA(29)
#define P_I2S_MCLK0             _GPIOA(30)
#define P_I2S_D1                _GPIOA(31)
#define P_I2S_BCLK1             _GPIOB(0)
#define P_I2S_LRCLK1            _GPIOB(1)
#define P_I2S_MCLK1             _GPIOB(2)

/* PCM1 */
#define P_PCM1_IN               _GPIOD(28)
#define P_PCM1_CLK              _GPIOD(29)
#define P_PCM1_SYNC             _GPIOD(30)
#define P_PCM1_OUT              _GPIOD(31)

/* KEY */
#define P_KS_IN0                _GPIOB(3)
#define P_KS_IN1                _GPIOB(4)
#define P_KS_IN2                _GPIOB(5)
#define P_KS_IN3                _GPIOB(6)
#define P_KS_OUT0               _GPIOB(7)
#define P_KS_OUT1               _GPIOB(8)
#define P_KS_OUT2               _GPIOB(9)

/* LVDS */
#define P_LVDS_OEP              _GPIOB(10)
#define P_LVDS_OEN              _GPIOB(11)
#define P_LVDS_ODP              _GPIOB(12)
#define P_LVDS_ODN              _GPIOB(13)
#define P_LVDS_OCP              _GPIOB(14)
#define P_LVDS_OCN              _GPIOB(15)
#define P_LVDS_OBP              _GPIOB(16)
#define P_LVDS_OBN              _GPIOB(17)
#define P_LVDS_OAP              _GPIOB(18)
#define P_LVDS_OAN              _GPIOB(19)
#define P_LVDS_EEP              _GPIOB(20)
#define P_LVDS_EEN              _GPIOB(21)
#define P_LVDS_EDP              _GPIOB(22)
#define P_LVDS_EDN              _GPIOB(23)
#define P_LVDS_ECP              _GPIOB(24)
#define P_LVDS_ECN              _GPIOB(25)
#define P_LVDS_EBP              _GPIOB(26)
#define P_LVDS_EBN              _GPIOB(27)
#define P_LVDS_EAP              _GPIOB(28)
#define P_LVDS_EAN              _GPIOB(29)
#define P_LCD0_D18              _GPIOB(30)
#define P_LCD0_D17              _GPIOB(31)

/* DSI */
#define P_DSI_DP3               _GPIOC(0)
#define P_DSI_DN3               _GPIOC(1)
#define P_DSI_DP1               _GPIOC(2)
#define P_DSI_DN1               _GPIOC(3)
#define P_DSI_CP                _GPIOC(4)
#define P_DSI_CN                _GPIOC(5)
#define P_DSI_DP0               _GPIOC(6)
#define P_DSI_DN0               _GPIOC(7)
#define P_DSI_DP2               _GPIOC(8)
#define P_DSI_DN2               _GPIOC(9)

/* SD */
#define P_SD0_D0                _GPIOC(10)
#define P_SD0_D1                _GPIOC(11)
#define P_SD0_D2                _GPIOC(12)
#define P_SD0_D3                _GPIOC(13)
#define P_SD0_D4                _GPIOC(14)
#define P_SD0_D5                _GPIOC(15)
#define P_SD0_D6                _GPIOC(16)
#define P_SD0_D7                _GPIOC(17)
#define P_SD0_CMD               _GPIOC(18)
#define P_SD0_CLK               _GPIOC(19)
#define P_SD1_CMD               _GPIOC(20)
#define P_SD1_CLK               _GPIOC(21)
#define P_SD1_D0		P_SD0_D4
#define P_SD1_D1		P_SD0_D5
#define P_SD1_D2		P_SD0_D6
#define P_SD1_D3		P_SD0_D7

/* SPI */
#define P_SPI0_SCLK             _GPIOC(22)
#define P_SPI0_SS               _GPIOC(23)
#define P_SPI0_MISO             _GPIOC(24)
#define P_SPI0_MOSI             _GPIOC(25)

/* UART for console */
#define P_UART0_RX              _GPIOC(26)
#define P_UART0_TX              _GPIOC(27)

/* UART for Bluetooth */
#define P_UART2_RX              _GPIOD(18)
#define P_UART2_TX              _GPIOD(19)
#define P_UART2_RTSB            _GPIOD(20)
#define P_UART2_CTSB            _GPIOD(21)

/* UART for 3G */
#define P_UART3_RX              _GPIOD(22)
#define P_UART3_TX              _GPIOD(23)
#define P_UART3_RTSB            _GPIOD(24)
#define P_UART3_CTSB            _GPIOD(25)

/* UART for GPS */
#define P_RESERD26              _GPIOD(26)
#define P_RESERD27              _GPIOD(27)

/* I2C */
#define P_I2C0_SCLK             _GPIOC(28)
#define P_I2C0_SDATA            _GPIOC(29)
#define P_I2C1_SCLK             _GPIOE(0)
#define P_I2C1_SDATA            _GPIOE(1)
#define P_I2C2_SCLK             _GPIOE(2)
#define P_I2C2_SDATA            _GPIOE(3)

/* CSI*/
#define P_CSI_DN0               _PIN(0)
#define P_CSI_DP0               _PIN(1)
#define P_CSI_DN1               _PIN(2)
#define P_CSI_DP1               _PIN(3)
#define P_CSI_CN                _PIN(4)
#define P_CSI_CP                _PIN(5)
#define P_CSI_DN2               _PIN(6)
#define P_CSI_DP2               _PIN(7)
#define P_CSI_DN3               _PIN(8)
#define P_CSI_DP3               _PIN(9)

#define P_RESERC30               _GPIOC(30)
#define P_RESERD0                _GPIOD(0)
#define P_RESERD1                _GPIOD(1)
#define P_RESERD2                _GPIOD(2)
#define P_RESERD3                _GPIOD(3)
#define P_RESERD4                _GPIOD(4)
#define P_RESERD5                _GPIOD(5)
#define P_RESERD6                _GPIOD(6)
#define P_RESERD7                _GPIOD(7)
#define P_RESERD8                _GPIOD(8)
#define P_RESERD9                _GPIOD(9)
#define P_RESERD11               _GPIOD(11)

/* Sensor */
#define P_SENSOR0_PCLK            _GPIOC(31)
#define P_SENSOR0_CKOUT           _GPIOD(10)

/* NAND (1.8v / 3.3v) */
#define P_DNAND_D0              _PIN(10)
#define P_DNAND_D1              _PIN(11)
#define P_DNAND_D2              _PIN(12)
#define P_DNAND_D3              _PIN(13)
#define P_DNAND_D4              _PIN(14)
#define P_DNAND_D5              _PIN(15)
#define P_DNAND_D6              _PIN(16)
#define P_DNAND_D7              _PIN(17)
#define P_DNAND_WRB             _PIN(18)
#define P_DNAND_RDB             _PIN(19)
#define P_DNAND_RDBN            _PIN(20)
#define P_DNAND_DQS             _GPIOA(12)
#define P_DNAND_DQSN            _GPIOA(13)
#define P_DNAND_RB0             _PIN(21)
#define P_DNAND_ALE             _GPIOD(12)
#define P_DNAND_CLE             _GPIOD(13)
#define P_DNAND_CEB0            _GPIOD(14)
#define P_DNAND_CEB1            _GPIOD(15)
#define P_DNAND_CEB2            _GPIOD(16)
#define P_DNAND_CEB3            _GPIOD(17)

/* System */
#define P_PORB			_PIN(22)
#define P_CLKO_25M		_PIN(23)
#define P_BSEL			_PIN(24)
#define P_PKG0			_PIN(25)
#define P_PKG1			_PIN(26)
#define P_PKG2			_PIN(27)
#define P_PKG3			_PIN(28)

#define _FIRSTPAD		P_RESERA0
#define _LASTPAD		P_PKG3
#define NUM_PADS		(_LASTPAD - _FIRSTPAD + 1)

unsigned int atm7059_num_pads = NUM_PADS;

#define PINCTRL_PIN(a, b) { .number = a, .name = b }
/* Pad names for the pinmux subsystem */
const struct owl_pinctrl_pin_desc atm7059_pads[NUM_PADS] = {
	PINCTRL_PIN(P_RESERA0, "P_RESERA0"),
	PINCTRL_PIN(P_RESERA1, "P_RESERA1"),
	PINCTRL_PIN(P_RESERA2, "P_RESERA2"),
	PINCTRL_PIN(P_RESERA3, "P_RESERA3"),
	PINCTRL_PIN(P_RESERA4, "P_RESERA4"),
	PINCTRL_PIN(P_RESERA5, "P_RESERA5"),
	PINCTRL_PIN(P_RESERA6, "P_RESERA6"),
	PINCTRL_PIN(P_RESERA7, "P_RESERA7"),
	PINCTRL_PIN(P_RESERA8, "P_RESERA8"),
	PINCTRL_PIN(P_RESERA9, "P_RESERA9"),
	PINCTRL_PIN(P_RESERA10, "P_RESERA10"),
	PINCTRL_PIN(P_RESERA11, "P_RESERA11"),
	
	PINCTRL_PIN(P_ETH_TXD0, "P_ETH_TXD0"),
	PINCTRL_PIN(P_ETH_TXD1, "P_ETH_TXD1"),
	PINCTRL_PIN(P_ETH_TXEN, "P_ETH_TXEN"),
	PINCTRL_PIN(P_ETH_RXER, "P_ETH_RXER"),
	PINCTRL_PIN(P_ETH_CRS_DV, "P_ETH_CRS_DV"),
	PINCTRL_PIN(P_ETH_RXD1, "P_ETH_RXD1"),
	PINCTRL_PIN(P_ETH_RXD0, "P_ETH_RXD0"),
	PINCTRL_PIN(P_ETH_REF_CLK, "P_ETH_REF_CLK"),
	PINCTRL_PIN(P_ETH_MDC, "P_ETH_MDC"),
	PINCTRL_PIN(P_ETH_MDIO, "P_ETH_MDIO"),
	PINCTRL_PIN(P_SIRQ0, "P_SIRQ0"),
	PINCTRL_PIN(P_SIRQ1, "P_SIRQ1"),
	PINCTRL_PIN(P_SIRQ2, "P_SIRQ2"),
	PINCTRL_PIN(P_I2S_D0, "P_I2S_D0"),
	PINCTRL_PIN(P_I2S_BCLK0, "P_I2S_BCLK0"),
	PINCTRL_PIN(P_I2S_LRCLK0, "P_I2S_LRCLK0"),
	PINCTRL_PIN(P_I2S_MCLK0, "P_I2S_MCLK0"),
	PINCTRL_PIN(P_I2S_D1, "P_I2S_D1"),
	PINCTRL_PIN(P_I2S_BCLK1, "P_I2S_BCLK1"),
	PINCTRL_PIN(P_I2S_LRCLK1, "P_I2S_LRCLK1"),
	PINCTRL_PIN(P_I2S_MCLK1, "P_I2S_MCLK1"),
	PINCTRL_PIN(P_PCM1_IN, "P_PCM1_IN"),
	PINCTRL_PIN(P_PCM1_CLK, "P_PCM1_CLK"),
	PINCTRL_PIN(P_PCM1_SYNC, "P_PCM1_SYNC"),
	PINCTRL_PIN(P_PCM1_OUT, "P_PCM1_OUT"),
	PINCTRL_PIN(P_KS_IN0, "P_KS_IN0"),
	PINCTRL_PIN(P_KS_IN1, "P_KS_IN1"),
	PINCTRL_PIN(P_KS_IN2, "P_KS_IN2"),
	PINCTRL_PIN(P_KS_IN3, "P_KS_IN3"),
	PINCTRL_PIN(P_KS_OUT0, "P_KS_OUT0"),
	PINCTRL_PIN(P_KS_OUT1, "P_KS_OUT1"),
	PINCTRL_PIN(P_KS_OUT2, "P_KS_OUT2"),
	PINCTRL_PIN(P_LVDS_OEP, "P_LVDS_OEP"),
	PINCTRL_PIN(P_LVDS_OEN, "P_LVDS_OEN"),
	PINCTRL_PIN(P_LVDS_ODP, "P_LVDS_ODP"),
	PINCTRL_PIN(P_LVDS_ODN, "P_LVDS_ODN"),
	PINCTRL_PIN(P_LVDS_OCP, "P_LVDS_OCP"),
	PINCTRL_PIN(P_LVDS_OCN, "P_LVDS_OCN"),
	PINCTRL_PIN(P_LVDS_OBP, "P_LVDS_OBP"),
	PINCTRL_PIN(P_LVDS_OBN, "P_LVDS_OBN"),
	PINCTRL_PIN(P_LVDS_OAP, "P_LVDS_OAP"),
	PINCTRL_PIN(P_LVDS_OAN, "P_LVDS_OAN"),
	PINCTRL_PIN(P_LVDS_EEP, "P_LVDS_EEP"),
	PINCTRL_PIN(P_LVDS_EEN, "P_LVDS_EEN"),
	PINCTRL_PIN(P_LVDS_EDP, "P_LVDS_EDP"),
	PINCTRL_PIN(P_LVDS_EDN, "P_LVDS_EDN"),
	PINCTRL_PIN(P_LVDS_ECP, "P_LVDS_ECP"),
	PINCTRL_PIN(P_LVDS_ECN, "P_LVDS_ECN"),
	PINCTRL_PIN(P_LVDS_EBP, "P_LVDS_EBP"),
	PINCTRL_PIN(P_LVDS_EBN, "P_LVDS_EBN"),
	PINCTRL_PIN(P_LVDS_EAP, "P_LVDS_EAP"),
	PINCTRL_PIN(P_LVDS_EAN, "P_LVDS_EAN"),
	PINCTRL_PIN(P_LCD0_D18, "P_LCD0_D18"),
	PINCTRL_PIN(P_LCD0_D17, "P_LCD0_D17"),
	PINCTRL_PIN(P_DSI_DP3, "P_DSI_DP3"),
	PINCTRL_PIN(P_DSI_DN3, "P_DSI_DN3"),
	PINCTRL_PIN(P_DSI_DP1, "P_DSI_DP1"),
	PINCTRL_PIN(P_DSI_DN1, "P_DSI_DN1"),
	PINCTRL_PIN(P_DSI_CP, "P_DSI_CP"),
	PINCTRL_PIN(P_DSI_CN, "P_DSI_CN"),
	PINCTRL_PIN(P_DSI_DP0, "P_DSI_DP0"),
	PINCTRL_PIN(P_DSI_DN0, "P_DSI_DN0"),
	PINCTRL_PIN(P_DSI_DP2, "P_DSI_DP2"),
	PINCTRL_PIN(P_DSI_DN2, "P_DSI_DN2"),
	PINCTRL_PIN(P_SD0_D0, "P_SD0_D0"),
	PINCTRL_PIN(P_SD0_D1, "P_SD0_D1"),
	PINCTRL_PIN(P_SD0_D2, "P_SD0_D2"),
	PINCTRL_PIN(P_SD0_D3, "P_SD0_D3"),
	PINCTRL_PIN(P_SD1_D0, "P_SD1_D0"),
	PINCTRL_PIN(P_SD1_D1, "P_SD1_D1"),
	PINCTRL_PIN(P_SD1_D2, "P_SD1_D2"),
	PINCTRL_PIN(P_SD1_D3, "P_SD1_D3"),
	PINCTRL_PIN(P_SD0_CMD, "P_SD0_CMD"),
	PINCTRL_PIN(P_SD0_CLK, "P_SD0_CLK"),
	PINCTRL_PIN(P_SD1_CMD, "P_SD1_CMD"),
	PINCTRL_PIN(P_SD1_CLK, "P_SD1_CLK"),
	PINCTRL_PIN(P_SPI0_SCLK, "P_SPI0_SCLK"),
	PINCTRL_PIN(P_SPI0_SS, "P_SPI0_SS"),
	PINCTRL_PIN(P_SPI0_MISO, "P_SPI0_MISO"),
	PINCTRL_PIN(P_SPI0_MOSI, "P_SPI0_MOSI"),
	PINCTRL_PIN(P_UART0_RX, "P_UART0_RX"),
	PINCTRL_PIN(P_UART0_TX, "P_UART0_TX"),
	PINCTRL_PIN(P_UART2_RX, "P_UART2_RX"),
	PINCTRL_PIN(P_UART2_TX, "P_UART2_TX"),
	PINCTRL_PIN(P_UART2_RTSB, "P_UART2_RTSB"),
	PINCTRL_PIN(P_UART2_CTSB, "P_UART2_CTSB"),
	PINCTRL_PIN(P_UART3_RX, "P_UART3_RX"),
	PINCTRL_PIN(P_UART3_TX, "P_UART3_TX"),
	PINCTRL_PIN(P_UART3_RTSB, "P_UART3_RTSB"),
	PINCTRL_PIN(P_UART3_CTSB, "P_UART3_CTSB"),
	PINCTRL_PIN(P_RESERD26, "P_RESERD26"),
	PINCTRL_PIN(P_RESERD27, "P_RESERD27"),
	PINCTRL_PIN(P_I2C0_SCLK, "P_I2C0_SCLK"),
	PINCTRL_PIN(P_I2C0_SDATA, "P_I2C0_SDATA"),
	PINCTRL_PIN(P_I2C1_SCLK, "P_I2C1_SCLK"),
	PINCTRL_PIN(P_I2C1_SDATA, "P_I2C1_SDATA"),
	PINCTRL_PIN(P_I2C2_SCLK, "P_I2C2_SCLK"),
	PINCTRL_PIN(P_I2C2_SDATA, "P_I2C2_SDATA"),
	PINCTRL_PIN(P_CSI_DN0, "P_CSI_DN0"),
	PINCTRL_PIN(P_CSI_DP0, "P_CSI_DP0"),
	PINCTRL_PIN(P_CSI_DN1, "P_CSI_DN1"),
	PINCTRL_PIN(P_CSI_DP1, "P_CSI_DP1"),
	PINCTRL_PIN(P_CSI_CN, "P_CSI_CN"),
	PINCTRL_PIN(P_CSI_CP, "P_CSI_CP"),
	PINCTRL_PIN(P_CSI_DN2, "P_CSI_DN2"),
	PINCTRL_PIN(P_CSI_DP2, "P_CSI_DP2"),
	PINCTRL_PIN(P_CSI_DN3, "P_CSI_DN3"),
	PINCTRL_PIN(P_CSI_DP3, "P_CSI_DP3"),

	PINCTRL_PIN(P_RESERC30, "P_RESERC30"),
	PINCTRL_PIN(P_SENSOR0_PCLK, "P_SENSOR0_PCLK"),
	PINCTRL_PIN(P_RESERD0, "P_RESERD0"),
	PINCTRL_PIN(P_RESERD1, "P_RESERD1"),
	PINCTRL_PIN(P_RESERD2, "P_RESERD2"),
	PINCTRL_PIN(P_RESERD3, "P_RESERD3"),
	PINCTRL_PIN(P_RESERD4, "P_RESERD4"),
	PINCTRL_PIN(P_RESERD5, "P_RESERD5"),
	PINCTRL_PIN(P_RESERD6, "P_RESERD6"),
	PINCTRL_PIN(P_RESERD7, "P_RESERD7"),
	PINCTRL_PIN(P_RESERD8, "P_RESERD8"),
	PINCTRL_PIN(P_RESERD9, "P_RESERD9"),

	PINCTRL_PIN(P_SENSOR0_CKOUT, "P_SENSOR0_CKOUT"),
	PINCTRL_PIN(P_RESERD11, "P_RESERD11"),
	
	PINCTRL_PIN(P_DNAND_D0, "P_DNAND_D0"),
	PINCTRL_PIN(P_DNAND_D1, "P_DNAND_D1"),
	PINCTRL_PIN(P_DNAND_D2, "P_DNAND_D2"),
	PINCTRL_PIN(P_DNAND_D3, "P_DNAND_D3"),
	PINCTRL_PIN(P_DNAND_D4, "P_DNAND_D4"),
	PINCTRL_PIN(P_DNAND_D5, "P_DNAND_D5"),
	PINCTRL_PIN(P_DNAND_D6, "P_DNAND_D6"),
	PINCTRL_PIN(P_DNAND_D7, "P_DNAND_D7"),
	PINCTRL_PIN(P_DNAND_WRB, "P_DNAND_WRB"),
	PINCTRL_PIN(P_DNAND_RDB, "P_DNAND_RDB"),
	PINCTRL_PIN(P_DNAND_RDBN, "P_DNAND_RDBN"),
	PINCTRL_PIN(P_DNAND_DQS, "P_DNAND_DQS"),
	PINCTRL_PIN(P_DNAND_DQSN, "P_DNAND_DQSN"),
	PINCTRL_PIN(P_DNAND_RB0, "P_DNAND_RB0"),
	PINCTRL_PIN(P_DNAND_ALE, "P_DNAND_ALE"),
	PINCTRL_PIN(P_DNAND_CLE, "P_DNAND_CLE"),
	PINCTRL_PIN(P_DNAND_CEB0, "P_DNAND_CEB0"),
	PINCTRL_PIN(P_DNAND_CEB1, "P_DNAND_CEB1"),
	PINCTRL_PIN(P_DNAND_CEB2, "P_DNAND_CEB2"),
	PINCTRL_PIN(P_DNAND_CEB3, "P_DNAND_CEB3"),
	PINCTRL_PIN(P_PORB, "P_PORB"),
	PINCTRL_PIN(P_CLKO_25M, "P_CLKO_25M"),
	PINCTRL_PIN(P_BSEL, "P_BSEL"),
	PINCTRL_PIN(P_PKG0, "P_PKG0"),
	PINCTRL_PIN(P_PKG1, "P_PKG1"),
	PINCTRL_PIN(P_PKG2, "P_PKG2"),
	PINCTRL_PIN(P_PKG3, "P_PKG3"),
};

/*****MFP group data****************************/
/*
** mfp0_31_26 reserved
*/

/*
** mfp0_25_23
*/
static unsigned int  owl_mfp0_25_23_pads[] = {
	P_LCD0_D18,
};

static unsigned int  owl_mfp0_25_23_funcs[] = {
	OWL_MUX_NOR,
	OWL_MUX_SENS1,
	OWL_MUX_PWM2,
	OWL_MUX_PWM4,
	OWL_MUX_LCD0,
};

/*
** mfp0_22_20
*/
static unsigned int  owl_mfp0_22_20_pads[] = {
	P_ETH_CRS_DV,
};

static unsigned int  owl_mfp0_22_20_funcs[] = {
	OWL_MUX_ETH_RMII,
	OWL_MUX_ETH_SMII,
	OWL_MUX_SPI2,
	OWL_MUX_UART4,
	OWL_MUX_PWM4,
};


/*
** mfp0_18_16_eth_txd0
*/
static unsigned int  owl_mfp0_18_16_eth_txd0_pads[] = {
	P_ETH_TXD0,
};

static unsigned int  owl_mfp0_18_16_eth_txd0_funcs[] = {
	OWL_MUX_ETH_RMII,
	OWL_MUX_ETH_SMII,
	OWL_MUX_SPI2,
	OWL_MUX_UART6,
	OWL_MUX_PWM4,
};

/*
** mfp0_18_16_eth_txd1
*/
static unsigned int  owl_mfp0_18_16_eth_txd1_pads[] = {
	P_ETH_TXD1,
};

static unsigned int  owl_mfp0_18_16_eth_txd1_funcs[] = {
	OWL_MUX_ETH_RMII,
	OWL_MUX_ETH_SMII,
	OWL_MUX_SPI2,
	OWL_MUX_UART6,
	OWL_MUX_PWM5,
};

/*
** mfp0_15_13_rmii_txen
*/
static unsigned int  owl_mfp0_15_13_rmii_txen_pads[] = {
	P_ETH_TXEN,
};

static unsigned int  owl_mfp0_15_13_rmii_txen_funcs[] = {
	OWL_MUX_ETH_RMII,
	OWL_MUX_UART2,
	OWL_MUX_SPI3,
	OWL_MUX_PWM0,
};

/*
** mfp0_15_13_rmii_rxen
*/
static unsigned int  owl_mfp0_15_13_rmii_rxen_pads[] = {
	P_ETH_RXER,
};

static unsigned int  owl_mfp0_15_13_rmii_rxen_funcs[] = {
	OWL_MUX_ETH_RMII,
	OWL_MUX_UART2,
	OWL_MUX_SPI3,
	OWL_MUX_PWM1,
};


/*
** mfp0_12_11 reserved
*/

/*
** mfp0_10_8_rmii_rxd1
*/
static unsigned int  owl_mfp0_10_8_rmii_rxd1_pads[] = {
	P_ETH_RXD1,
};

static unsigned int  owl_mfp0_10_8_rmii_rxd1_funcs[] = {
	OWL_MUX_ETH_RMII,
	OWL_MUX_UART2,
	OWL_MUX_SPI3,
	OWL_MUX_PWM2,
	OWL_MUX_UART5,
};

/*
** mfp0_10_8_rmii_rxd0
*/
static unsigned int  owl_mfp0_10_8_rmii_rxd0_pads[] = {
	P_ETH_RXD0,
};

static unsigned int  owl_mfp0_10_8_rmii_rxd0_funcs[] = {
	OWL_MUX_ETH_RMII,
	OWL_MUX_UART2,
	OWL_MUX_SPI3,
	OWL_MUX_PWM3,
	OWL_MUX_UART5,
};

/*
** mfp0_7_6
*/
static unsigned int  owl_mfp0_7_6_pads[] = {
	P_ETH_REF_CLK,
};

static unsigned int  owl_mfp0_7_6_funcs[] = {
	OWL_MUX_ETH_RMII,
	OWL_MUX_UART4,
	OWL_MUX_SPI2,
	OWL_MUX_RESERVED,

	OWL_MUX_ETH_SMII,
};

/*
** mfp0_5
*/
static unsigned int  owl_mfp0_5_pads[] = {
	P_I2S_D0,
};

static unsigned int  owl_mfp0_5_funcs[] = {
	OWL_MUX_I2S0,
	OWL_MUX_NOR,
};

/*
** mfp0_4_3
*/
static unsigned int  owl_mfp0_4_3_pads[] = {
	P_I2S_LRCLK0,
	P_I2S_MCLK0,
};

static unsigned int  owl_mfp0_4_3_funcs[] = {
	OWL_MUX_I2S0,
	OWL_MUX_RESERVED,
	OWL_MUX_PCM1,
};

/*
** mfp0_2_1_i2s0
*/
static unsigned int  owl_mfp0_2_1_i2s0_pads[] = {
	P_I2S_BCLK0,
};

static unsigned int  owl_mfp0_2_1_i2s0_funcs[] = {
	OWL_MUX_I2S0,
	OWL_MUX_NOR,
	OWL_MUX_PCM0,
};

/*
** mfp0_2_1_i2s1
*/
static unsigned int  owl_mfp0_2_1_i2s1_pads[] = {
	P_I2S_BCLK1,
	P_I2S_LRCLK1,
	P_I2S_MCLK1,
};

static unsigned int  owl_mfp0_2_1_i2s1_funcs[] = {
	OWL_MUX_I2S1,
	OWL_MUX_NOR,
	OWL_MUX_PCM0,
};

/*
** mfp0_0
*/
static unsigned int  owl_mfp0_0_pads[] = {
	P_I2S_D1,
};

static unsigned int  owl_mfp0_0_funcs[] = {
	OWL_MUX_I2S1,
	OWL_MUX_NOR,
};

/*eth smii dummy group*/
static unsigned int  owl_eth_smi_dummy_pads[] = {
	P_ETH_MDC,
	P_ETH_MDIO,
};

static unsigned int  owl_eth_smi_dummy_funcs[] = {
	OWL_MUX_ETH_SMII,
	OWL_MUX_ETH_RMII,
};

/*sirq0 dummy group*/
static unsigned int  owl_sirq0_dummy_pads[] = {
	P_SIRQ0,
};

static unsigned int  owl_sirq0_dummy_funcs[] = {
	OWL_MUX_SIRQ0,
};

/*sirq1 dummy group*/
static unsigned int  owl_sirq1_dummy_pads[] = {
	P_SIRQ1,
};

static unsigned int  owl_sirq1_dummy_funcs[] = {
	OWL_MUX_SIRQ1,
};

/*sirq2 dummy group*/
static unsigned int  owl_sirq2_dummy_pads[] = {
	P_SIRQ2,
};

static unsigned int  owl_sirq2_dummy_funcs[] = {
	OWL_MUX_SIRQ2,
};

/*
** mfp1_31_29_ks_in2
*/
static unsigned int  owl_mfp1_31_29_ks_in2_pads[] = {
	P_KS_IN2,
};

static unsigned int  owl_mfp1_31_29_ks_in2_funcs[] = {
	OWL_MUX_KS,
	OWL_MUX_JTAG,
	OWL_MUX_NOR,
	OWL_MUX_PWM0,
	OWL_MUX_PWM4,
	OWL_MUX_SENS1,
	OWL_MUX_PWM4,
	OWL_MUX_P0,
};

/*
** mfp1_31_29_ks_in1
*/
static unsigned int  owl_mfp1_31_29_ks_in1_pads[] = {
	P_KS_IN1,
};

static unsigned int  owl_mfp1_31_29_ks_in1_funcs[] = {
	OWL_MUX_KS,
	OWL_MUX_JTAG,
	OWL_MUX_NOR,
	OWL_MUX_PWM1,
	OWL_MUX_PWM5,
	OWL_MUX_SENS1,
	OWL_MUX_PWM1,
	OWL_MUX_USB30,
};

/*
** mfp1_31_29_ks_in0
*/
static unsigned int  owl_mfp1_31_29_ks_in0_pads[] = {
	P_KS_IN0,
};

static unsigned int  owl_mfp1_31_29_ks_in0_funcs[] = {
	OWL_MUX_KS,
	OWL_MUX_JTAG,
	OWL_MUX_NOR,
	OWL_MUX_PWM0,
	OWL_MUX_PWM0,
	OWL_MUX_SENS1,
	OWL_MUX_PWM0,
	OWL_MUX_P0,
};


/*
** mfp1_28_26_ks_in3
*/
static unsigned int  owl_mfp1_28_26_ks_in3_pads[] = {
	P_KS_IN3,
};

static unsigned int  owl_mfp1_28_26_ks_in3_funcs[] = {
	OWL_MUX_KS,
	OWL_MUX_JTAG,
	OWL_MUX_NOR,
	OWL_MUX_PWM1,
	OWL_MUX_RESERVED,
	OWL_MUX_SENS1,
};

/*
** mfp1_28_26_ks_out0
*/
static unsigned int  owl_mfp1_28_26_ks_out0_pads[] = {
	P_KS_OUT0,
};

static unsigned int  owl_mfp1_28_26_ks_out0_funcs[] = {
	OWL_MUX_KS,
	OWL_MUX_UART5,
	OWL_MUX_NOR,
	OWL_MUX_PWM2,
	OWL_MUX_RESERVED,
	OWL_MUX_SENS1,
	OWL_MUX_SD0,
};

/*
** mfp1_28_26_ks_out1
*/
static unsigned int  owl_mfp1_28_26_ks_out1_pads[] = {
	P_KS_OUT1,
};

static unsigned int  owl_mfp1_28_26_ks_out1_funcs[] = {
	OWL_MUX_KS,
	OWL_MUX_JTAG,
	OWL_MUX_NOR,
	OWL_MUX_PWM3,
	OWL_MUX_RESERVED,
	OWL_MUX_SENS1,
	OWL_MUX_SD0,
};

/*
** mfp1_25_23
*/
static unsigned int  owl_mfp1_25_23_pads[] = {
	P_KS_OUT2,
};

static unsigned int  owl_mfp1_25_23_funcs[] = {
	OWL_MUX_SD0,
	OWL_MUX_KS,
	OWL_MUX_NOR,
	OWL_MUX_PWM2,
	OWL_MUX_UART5,
	OWL_MUX_SENS1,
};

/*
** mfp1_22_21
*/
static unsigned int  owl_mfp1_22_21_pads[] = {
	P_LVDS_OEP,
	P_LVDS_OEN,
	P_LVDS_ODP,
	P_LVDS_ODN,
	P_LVDS_OCP,
	P_LVDS_OCN,
	P_LVDS_OBP,
	P_LVDS_OBN,
	P_LVDS_OAP,
	P_LVDS_OAN,
};

static unsigned int  owl_mfp1_22_21_funcs[] = {
	OWL_MUX_LVDS,
	OWL_MUX_TS,
	OWL_MUX_LCD0,
};

/*
** mfp1_20_19
*/
static unsigned int  owl_mfp1_20_19_pads[] = {
	P_DSI_DN0,
};

static unsigned int  owl_mfp1_20_19_funcs[] = {
	OWL_MUX_DSI,
	OWL_MUX_UART2,
	OWL_MUX_SPI0,
};

/*
** mfp1_18_17
*/
static unsigned int  owl_mfp1_18_17_pads[] = {
	P_DSI_DP2,
};

static unsigned int  owl_mfp1_18_17_funcs[] = {
	OWL_MUX_DSI,
	OWL_MUX_UART2,
	OWL_MUX_SPI0,
	OWL_MUX_SD1,
};

/*
** mfp1_16_14
*/
static unsigned int  owl_mfp1_16_14_pads[] = {
	P_LCD0_D17,
};

static unsigned int  owl_mfp1_16_14_funcs[] = {
	OWL_MUX_NOR,
	OWL_MUX_SD0,
	OWL_MUX_SD1,
	OWL_MUX_PWM3,
	OWL_MUX_LCD0,
};

/*
** mfp1_13_12
*/
static unsigned int  owl_mfp1_13_12_pads[] = {
	P_DSI_DP3,
};

static unsigned int  owl_mfp1_13_12_funcs[] = {
	OWL_MUX_DSI,
	OWL_MUX_SD0,
	OWL_MUX_SD1,
	OWL_MUX_LCD0,
};

/*
** mfp1_11_10
*/
static unsigned int  owl_mfp1_11_10_pads[] = {
	P_DSI_DN3,
};

static unsigned int  owl_mfp1_11_10_funcs[] = {
	OWL_MUX_DSI,
	OWL_MUX_RESERVED,
	OWL_MUX_SD1,
	OWL_MUX_LCD0,
};

/*
** mfp1_9_7
*/
static unsigned int  owl_mfp1_9_7_pads[] = {
	P_DSI_DP0,
};

static unsigned int  owl_mfp1_9_7_funcs[] = {
	OWL_MUX_DSI,
	OWL_MUX_RESERVED,
	OWL_MUX_SD0,
	OWL_MUX_UART2,
	OWL_MUX_SPI0,
};

/*
** mfp1_6_5
*/
static unsigned int  owl_mfp1_6_5_pads[] = {
	P_LVDS_EEP,
	P_LVDS_EEN,
};

static unsigned int  owl_mfp1_6_5_funcs[] = {
	OWL_MUX_LVDS,
	OWL_MUX_NOR,
	OWL_MUX_TS,
	OWL_MUX_LCD0,
};

/*
** mfp1_4_3
*/
static unsigned int  owl_mfp1_4_3_pads[] = {
	P_SPI0_SCLK,
	P_SPI0_MOSI,

};

static unsigned int  owl_mfp1_4_3_funcs[] = {
	OWL_MUX_SPI0,
	OWL_MUX_NOR,
	OWL_MUX_I2C3,
	OWL_MUX_PCM0,
};

/*
** mfp1_2_0
*/
static unsigned int  owl_mfp1_2_0_pads[] = {
	P_SPI0_SS,
	P_SPI0_MISO,

};

static unsigned int  owl_mfp1_2_0_funcs[] = {
	OWL_MUX_SPI0,
	OWL_MUX_NOR,
	OWL_MUX_I2S1,
	OWL_MUX_PCM1,
	OWL_MUX_PCM0,
};

/*
** mfp2_31 reserved
*/

/*
** mfp2_30_29
*/
static unsigned int  owl_mfp2_30_29_pads[] = {
	P_DSI_DP1,
	P_DSI_CP,
	P_DSI_CN,
};

static unsigned int  owl_mfp2_30_29_funcs[] = {
	OWL_MUX_DSI,
	OWL_MUX_SD1,
	OWL_MUX_LCD0,
};


/*
** mfp2_28_27
*/
static unsigned int  owl_mfp2_28_27_pads[] = {
	P_LVDS_EDP,
	P_LVDS_EDN,
	P_LVDS_ECP,
	P_LVDS_ECN,
	P_LVDS_EBP,
	P_LVDS_EBN,
	P_LVDS_EAP,
	P_LVDS_EAN,
};

static unsigned int  owl_mfp2_28_27_funcs[] = {
	OWL_MUX_LVDS,
	OWL_MUX_NOR,
	OWL_MUX_LCD0,
};

/*
** mfp2_26_24
*/
static unsigned int  owl_mfp2_26_24_pads[] = {
	P_DSI_DN2,
};

static unsigned int  owl_mfp2_26_24_funcs[] = {
	OWL_MUX_DSI,
	OWL_MUX_RESERVED,
	OWL_MUX_SD1,
	OWL_MUX_UART2,
	OWL_MUX_SPI0,
};

/*
** mfp2_23
*/
static unsigned int  owl_mfp2_23_pads[] = {
	P_UART2_RTSB,
};

static unsigned int  owl_mfp2_23_funcs[] = {
	OWL_MUX_UART2,
	OWL_MUX_UART0,
};

/*
** mfp2_22
*/
static unsigned int  owl_mfp2_22_pads[] = {
	P_UART2_CTSB,
};

static unsigned int  owl_mfp2_22_funcs[] = {
	OWL_MUX_UART2,
	OWL_MUX_UART0,
};

/*
** mfp2_21
*/
static unsigned int  owl_mfp2_21_pads[] = {
	P_UART3_RTSB,
};

static unsigned int  owl_mfp2_21_funcs[] = {
	OWL_MUX_UART3,
	OWL_MUX_UART5,
};

/*
** mfp2_20
*/
static unsigned int  owl_mfp2_20_pads[] = {
	P_UART3_CTSB,
};

static unsigned int  owl_mfp2_20_funcs[] = {
	OWL_MUX_UART3,
	OWL_MUX_UART5,
};

/*
** mfp2_19_17
*/
static unsigned int  owl_mfp2_19_17_pads[] = {
	P_SD0_D0,
};

static unsigned int  owl_mfp2_19_17_funcs[] = {
	OWL_MUX_SD0,
	OWL_MUX_NOR,
	OWL_MUX_RESERVED,
	OWL_MUX_JTAG,
	OWL_MUX_UART2,
	OWL_MUX_UART5,
};

/*
** mfp2_16_14
*/
static unsigned int  owl_mfp2_16_14_pads[] = {
	P_SD0_D1,
};

static unsigned int  owl_mfp2_16_14_funcs[] = {
	OWL_MUX_SD0,
	OWL_MUX_NOR,
	OWL_MUX_RESERVED,
	OWL_MUX_RESERVED,
	OWL_MUX_UART2,
	OWL_MUX_UART5,
};

/*
** mfp2_13_11
*/
static unsigned int  owl_mfp2_13_11_pads[] = {
	P_SD0_D2,
	P_SD0_D3,
};

static unsigned int  owl_mfp2_13_11_funcs[] = {
	OWL_MUX_SD0,
	OWL_MUX_NOR,
	OWL_MUX_RESERVED,
	OWL_MUX_JTAG,
	OWL_MUX_UART2,
	OWL_MUX_UART1,
};

/*
** mfp2_10_9
*/
static unsigned int  owl_mfp2_10_9_pads[] = {
	P_SD1_D0,
	P_SD1_D1,
	P_SD1_D2,
	P_SD1_D3,
};

static unsigned int  owl_mfp2_10_9_funcs[] = {
	OWL_MUX_SD0,
	OWL_MUX_NOR,
	OWL_MUX_RESERVED,
	OWL_MUX_SD1,
};

/*
** mfp2_8_7
*/
static unsigned int  owl_mfp2_8_7_pads[] = {
	P_SD0_CMD,
};

static unsigned int  owl_mfp2_8_7_funcs[] = {
	OWL_MUX_SD0,
	OWL_MUX_NOR,
	OWL_MUX_RESERVED,
	OWL_MUX_JTAG,
};

/*
** mfp2_6_5
*/
static unsigned int  owl_mfp2_6_5_pads[] = {
	P_SD0_CLK,
};

static unsigned int  owl_mfp2_6_5_funcs[] = {
	OWL_MUX_SD0,
	OWL_MUX_RESERVED,
	OWL_MUX_JTAG,
};

/*
** mfp2_4_3
*/
static unsigned int  owl_mfp2_4_3_pads[] = {
	P_SD1_CMD,
};

static unsigned int  owl_mfp2_4_3_funcs[] = {
	OWL_MUX_SD1,
	OWL_MUX_NOR,
};

/*
** mfp2_2_0
*/
static unsigned int  owl_mfp2_2_0_pads[] = {
	P_UART0_RX,
};

static unsigned int  owl_mfp2_2_0_funcs[] = {
	OWL_MUX_UART0,
	OWL_MUX_UART2,
	OWL_MUX_SPI1,
	OWL_MUX_I2C0,
	OWL_MUX_PCM1,
	OWL_MUX_I2S1,
};

/*
** sd1 dummy group
*/
static unsigned int  owl_sd1_dummy_pads[] = {
	P_SD1_CLK,
};

static unsigned int  owl_sd1_dummy_funcs[] = {
	OWL_MUX_SD1,
};

/*
** mfp3_31 reserved
*/

/*
** mfp3_30
*/
static unsigned int  owl_mfp3_30_pads[] = {
	P_CLKO_25M,
};

static unsigned int  owl_mfp3_30_funcs[] = {
	OWL_MUX_RESERVED,
	OWL_MUX_CLKO_25M,
};

/*
** mfp3_29_28
*/
static unsigned int  owl_mfp3_29_28_pads[] = {
	P_CSI_CN,
	P_CSI_CP,
};

static unsigned int  owl_mfp3_29_28_funcs[] = {
	OWL_MUX_MIPI_CSI,
	OWL_MUX_SENS0,
};

/*
** mfp3_27_24 reserved
*/

/*
** mfp3_23_22
*/
static unsigned int  owl_mfp3_23_22_pads[] = {
	P_SENSOR0_CKOUT,
};

static unsigned int  owl_mfp3_23_22_funcs[] = {
	OWL_MUX_SENS0,
	OWL_MUX_NOR,
	OWL_MUX_SENS1,
	OWL_MUX_PWM1,
};

/*
** mfp3_21_19
*/
static unsigned int  owl_mfp3_21_19_pads[] = {
	P_UART0_TX,
};

static unsigned int  owl_mfp3_21_19_funcs[] = {
	OWL_MUX_UART0,
	OWL_MUX_UART2,
	OWL_MUX_SPI1,
	OWL_MUX_I2C0,
	OWL_MUX_SPDIF,
	OWL_MUX_PCM1,
	OWL_MUX_I2S1,
};

/*
** mfp3_18_16
*/
static unsigned int  owl_mfp3_18_16_pads[] = {
	P_I2C0_SCLK,
	P_I2C0_SDATA,
};

static unsigned int  owl_mfp3_18_16_funcs[] = {
	OWL_MUX_I2C0,
	OWL_MUX_UART2,
	OWL_MUX_I2C1,
	OWL_MUX_UART1,
	OWL_MUX_SPI1,
};

/*
** mfp3_15_14
*/
static unsigned int  owl_mfp3_15_14_pads[] = {
	P_CSI_DN0,
	P_CSI_DN1,
	P_CSI_DN2,
	P_CSI_DN3,
	P_CSI_DP0,
	P_CSI_DP1,
	P_CSI_DP2,
	P_CSI_DP3,
};

static unsigned int  owl_mfp3_15_14_funcs[] = {
	OWL_MUX_MIPI_CSI,
	OWL_MUX_SENS0,
};

/*
** mfp3_13_12
*/
static unsigned int  owl_mfp3_13_12_pads[] = {
	P_SENSOR0_PCLK,
};

static unsigned int  owl_mfp3_13_12_funcs[] = {
	OWL_MUX_SENS0,
	OWL_MUX_NOR,
	OWL_MUX_PWM0,
};

/*
** mfp3_11_10
*/
static unsigned int  owl_mfp3_11_10_pads[] = {
	P_PCM1_IN,
};

static unsigned int  owl_mfp3_11_10_funcs[] = {
	OWL_MUX_PCM1,
	OWL_MUX_SENS1,
	OWL_MUX_UART4,
	OWL_MUX_PWM4,
};

/*
** mfp3_9_8
*/
static unsigned int  owl_mfp3_9_8_pads[] = {
	P_PCM1_CLK,
};

static unsigned int  owl_mfp3_9_8_funcs[] = {
	OWL_MUX_PCM1,
	OWL_MUX_SENS1,
	OWL_MUX_UART4,
	OWL_MUX_PWM5,
};

/*
** 	mfp3_7_6
*/
static unsigned int  owl_mfp3_7_6_pads[] = {
	P_PCM1_SYNC,
};

static unsigned int  owl_mfp3_7_6_funcs[] = {
	OWL_MUX_PCM1,
	OWL_MUX_SENS1,
	OWL_MUX_UART6,
	OWL_MUX_I2C3,
};

/*
** mfp3_8
*/
static unsigned int  owl_mfp3_5_4_pads[] = {
	P_PCM1_OUT,
};

static unsigned int  owl_mfp3_5_4_funcs[] = {
	OWL_MUX_PCM1,
	OWL_MUX_SENS1,
	OWL_MUX_UART6,
	OWL_MUX_I2C3,
};

/*
** mfp3_3
*/
static unsigned int  owl_mfp3_3_pads[] = {
	P_DNAND_D0,
	P_DNAND_D1,
	P_DNAND_D2,
	P_DNAND_D3,
	P_DNAND_D4,
	P_DNAND_D5,
	P_DNAND_D6,
	P_DNAND_D7,
	P_DNAND_RDB,
	P_DNAND_RDBN,
};

static unsigned int  owl_mfp3_3_funcs[] = {
	OWL_MUX_NAND,
	OWL_MUX_SD2,
};

/*
** mfp3_2
*/
static unsigned int  owl_mfp3_2_pads[] = {
	P_DNAND_ALE,
	P_DNAND_CLE,
	P_DNAND_CEB0,
	P_DNAND_CEB1,
};

static unsigned int  owl_mfp3_2_funcs[] = {
	OWL_MUX_NAND,
	OWL_MUX_SPI2,
};

/*
** mfp3_1_0_nand_ceb2
*/
static unsigned int  owl_mfp3_1_0_nand_ceb2_pads[] = {
	P_DNAND_CEB2,
};

static unsigned int  owl_mfp3_1_0_nand_ceb2_funcs[] = {
	OWL_MUX_NAND,
	OWL_MUX_PWM5,
};

/*
** mfp3_1_0_nand_ceb3
*/
static unsigned int  owl_mfp3_1_0_nand_ceb3_pads[] = {
	P_DNAND_CEB3,
};

static unsigned int  owl_mfp3_1_0_nand_ceb3_funcs[] = {
	OWL_MUX_NAND,
	OWL_MUX_PWM4,
};

/*
** dsi dummy group
*/
static unsigned int  owl_dsi_dummy_pads[] = {
	P_DSI_DN1,
};

static unsigned int  owl_dsi_dummy_funcs[] = {
	OWL_MUX_DSI,
};


/*
** nand dummy group
*/
static unsigned int  owl_nand_dummy_pads[] = {
	P_DNAND_WRB,
	P_DNAND_RB0,
	P_DNAND_DQS,
	P_DNAND_DQSN,
};

static unsigned int  owl_nand_dummy_funcs[] = {
	OWL_MUX_NAND,
};

/*
** uart2 dummy group
*/
static unsigned int  owl_uart2_dummy_pads[] = {
	P_UART2_RX,
	P_UART2_TX,
};

static unsigned int  owl_uart2_dummy_funcs[] = {
	OWL_MUX_UART2,
};

/*
** uart3 dummy group
*/
static unsigned int  owl_uart3_dummy_pads[] = {
	P_UART3_RX,
	P_UART3_TX,
};

static unsigned int  owl_uart3_dummy_funcs[] = {
	OWL_MUX_UART3,
};


/*
** i2c1 dummy group
*/
static unsigned int  owl_i2c1_dummy_pads[] = {
	P_I2C1_SCLK,
	P_I2C1_SDATA,
};

static unsigned int  owl_i2c1_dummy_funcs[] = {
	OWL_MUX_I2C1,
};

/*
** i2c2 dummy group
*/
static unsigned int  owl_i2c2_dummy_pads[] = {
	P_I2C2_SCLK,
	P_I2C2_SDATA,
};

static unsigned int  owl_i2c2_dummy_funcs[] = {
	OWL_MUX_I2C2,
};


/*****End MFP group data****************************/

/*****PADDRV group data****************************/

/*PAD_DRV0*/
static unsigned int  owl_paddrv0_29_28_pads[] = {
	P_SIRQ0,
	P_SIRQ1,
	P_SIRQ2,
};

static unsigned int  owl_paddrv0_23_22_pads[] = {
	P_ETH_TXD0,
	P_ETH_TXD1,
	P_ETH_TXEN,
};

static unsigned int  owl_paddrv0_21_20_pads[] = {
	P_ETH_RXER,
};

static unsigned int  owl_paddrv0_19_18_pads[] = {
	P_ETH_CRS_DV,
};

static unsigned int  owl_paddrv0_17_16_pads[] = {
	P_ETH_RXD0,
	P_ETH_RXD1,
};

static unsigned int  owl_paddrv0_15_14_pads[] = {
	P_ETH_REF_CLK,
};

static unsigned int  owl_paddrv0_13_12_pads[] = {
	P_ETH_MDC,
	P_ETH_MDIO,
};

static unsigned int  owl_paddrv0_11_10_pads[] = {
	P_I2S_D0,
};

static unsigned int  owl_paddrv0_9_8_pads[] = {
	P_I2S_BCLK0,
};

static unsigned int  owl_paddrv0_7_6_pads[] = {
	P_I2S_LRCLK0,
	P_I2S_MCLK0,
	P_I2S_D1,
};

static unsigned int  owl_paddrv0_5_4_pads[] = {
	P_I2S_BCLK1,
	P_I2S_LRCLK1,
	P_I2S_MCLK1,
};

static unsigned int  owl_paddrv0_3_2_pads[] = {
	P_PCM1_IN,
	P_PCM1_CLK,
	P_PCM1_SYNC,
	P_PCM1_OUT,
};

static unsigned int  owl_paddrv0_1_0_pads[] = {
	P_KS_IN0,
	P_KS_IN1,
	P_KS_IN2,
	P_KS_IN3,
};

/*PAD_DRV1*/
static unsigned int  owl_paddrv1_31_30_pads[] = {
	P_KS_OUT0,
	P_KS_OUT1,
	P_KS_OUT2,
};

static unsigned int  owl_paddrv1_29_28_pads[] = {
	P_LVDS_OEP,
	P_LVDS_OEN,
	P_LVDS_ODP,
	P_LVDS_ODN,
	P_LVDS_OCP,
	P_LVDS_OCN,
	P_LVDS_OBP,
	P_LVDS_OBN,
	P_LVDS_OAP,
	P_LVDS_OAN,
	P_LVDS_EEP,
	P_LVDS_EEN,
	P_LVDS_EDP,
	P_LVDS_EDN,
	P_LVDS_ECP,
	P_LVDS_ECN,
	P_LVDS_EBP,
	P_LVDS_EBN,
	P_LVDS_EAP,
	P_LVDS_EAN,
};

static unsigned int  owl_paddrv1_27_26_pads[] = {
	P_DSI_DP3,
	P_DSI_DN3,
	P_DSI_DP1,
	P_DSI_DN1,
	P_DSI_CP,
	P_DSI_CN,
};

static unsigned int  owl_paddrv1_25_24_pads[] = {
	P_DSI_DP0,
	P_DSI_DN0,
	P_DSI_DP2,
	P_DSI_DN2,
};

static unsigned int  owl_paddrv1_23_22_pads[] = {
	P_SD0_D0,
	P_SD0_D1,
	P_SD0_D2,
	P_SD0_D3,
};

static unsigned int  owl_paddrv1_21_20_pads[] = {
	P_SD1_D0,
	P_SD1_D1,
	P_SD1_D2,
	P_SD1_D3,
};

static unsigned int  owl_paddrv1_19_18_pads[] = {
	P_SD0_CMD,
};

static unsigned int  owl_paddrv1_17_16_pads[] = {
	P_SD0_CLK,
};

static unsigned int  owl_paddrv1_15_14_pads[] = {
	P_SD1_CMD,
};

static unsigned int  owl_paddrv1_13_12_pads[] = {
	P_SD1_CLK,
};

static unsigned int  owl_paddrv1_11_10_pads[] = {
	P_SPI0_SCLK,
	P_SPI0_SS,
	P_SPI0_MISO,
	P_SPI0_MOSI,
};

/*PAD_DRV2*/
static unsigned int  owl_paddrv2_31_30_pads[] = {
	P_UART0_RX,
};

static unsigned int  owl_paddrv2_29_28_pads[] = {
	P_UART0_TX,
};

static unsigned int  owl_paddrv2_27_26_pads[] = {
	P_UART2_RX,
	P_UART2_TX,
	P_UART2_RTSB,
	P_UART2_CTSB,
};

static unsigned int  owl_paddrv2_24_23_pads[] = {
	P_I2C0_SCLK,
	P_I2C0_SDATA,
};

static unsigned int  owl_paddrv2_22_21_pads[] = {
	P_I2C1_SCLK,
	P_I2C1_SDATA,
	P_I2C2_SCLK,
	P_I2C2_SDATA,
};

static unsigned int  owl_paddrv2_19_18_pads[] = {
	P_SENSOR0_PCLK,
};

static unsigned int  owl_paddrv2_13_12_pads[] = {
	P_SENSOR0_CKOUT,
};

static unsigned int  owl_paddrv2_3_2_pads[] = {
	P_UART3_RX,
	P_UART3_TX,
	P_UART3_RTSB,
	P_UART3_CTSB,
};

/*****End PADDRV group data****************************/

#define MUX_PG(group_name, mfpctl_regn, mfpctl_sft, mfpctl_w)		\
	{	\
		.name = #group_name,		\
		.pads = owl_##group_name##_pads,	\
		.padcnt = ARRAY_SIZE(owl_##group_name##_pads),	\
		.funcs = owl_##group_name##_funcs,	\
		.nfuncs = ARRAY_SIZE(owl_##group_name##_funcs),	\
		.mfpctl_regnum = mfpctl_regn,	\
		.mfpctl_shift = mfpctl_sft,	\
		.mfpctl_width = mfpctl_w,	\
		.paddrv_regnum = -1,		\
	}

#define MUX_PG_DUMMY(group_name)		\
	{	\
		.name = #group_name,		\
		.pads = owl_##group_name##_pads,	\
		.padcnt = ARRAY_SIZE(owl_##group_name##_pads),	\
		.funcs = owl_##group_name##_funcs,	\
		.nfuncs = ARRAY_SIZE(owl_##group_name##_funcs),	\
		.mfpctl_regnum = -1,	\
		.paddrv_regnum = -1,	\
	}

#define PADDRV_PG(group_name, paddrv_regn, paddrv_sft)	\
	{	\
		.name = #group_name,		\
		.pads = owl_##group_name##_pads,	\
		.padcnt = ARRAY_SIZE(owl_##group_name##_pads),	\
		.paddrv_regnum = paddrv_regn,		\
		.paddrv_shift = paddrv_sft,	\
		.paddrv_width = 2,		\
		.mfpctl_regnum = -1,	\
	}

/*
**
** all pinctrl groups of atm7059 board
**
*/
const struct owl_group atm7059_groups[] = {
	MUX_PG(mfp0_25_23, 0, 23, 3),
	MUX_PG(mfp0_22_20, 0, 20, 3),
	MUX_PG(mfp0_18_16_eth_txd0, 0, 16, 3),
	MUX_PG(mfp0_18_16_eth_txd1, 0, 16, 3),
	MUX_PG(mfp0_15_13_rmii_txen, 0, 13, 3),
	MUX_PG(mfp0_15_13_rmii_rxen, 0, 13, 3),
	MUX_PG(mfp0_10_8_rmii_rxd1, 0, 8, 3),
	MUX_PG(mfp0_10_8_rmii_rxd0, 0, 8, 3),
	MUX_PG(mfp0_7_6, 0, 6, 2),
	MUX_PG(mfp0_5, 0, 5, 1),
	MUX_PG(mfp0_4_3, 0, 3, 2),
	MUX_PG(mfp0_2_1_i2s0, 0, 1, 2),
	MUX_PG(mfp0_2_1_i2s1, 0, 1, 2),
	MUX_PG(mfp0_0, 0, 0, 1),
	MUX_PG_DUMMY(eth_smi_dummy),
	MUX_PG_DUMMY(sirq0_dummy),
	MUX_PG_DUMMY(sirq1_dummy),
	MUX_PG_DUMMY(sirq2_dummy),

	MUX_PG(mfp1_31_29_ks_in2, 1, 29, 3),
	MUX_PG(mfp1_31_29_ks_in1, 1, 29, 3),
	MUX_PG(mfp1_31_29_ks_in0, 1, 29, 3),
	MUX_PG(mfp1_28_26_ks_in3, 1, 26, 3),
	MUX_PG(mfp1_28_26_ks_out0, 1, 26, 3),
	MUX_PG(mfp1_28_26_ks_out1, 1, 26, 3),
	MUX_PG(mfp1_25_23, 1, 23, 3),
	MUX_PG(mfp1_22_21, 1, 21, 2),
	MUX_PG(mfp1_20_19, 1, 19, 2),
	MUX_PG(mfp1_18_17, 1, 17, 2),
	MUX_PG(mfp1_16_14, 1, 14, 3),
	MUX_PG(mfp1_13_12, 1, 12, 2),
	MUX_PG(mfp1_11_10, 1, 10, 2),
	MUX_PG(mfp1_9_7, 1, 7, 3),
	MUX_PG(mfp1_6_5, 1, 5, 2),
	MUX_PG(mfp1_4_3, 1, 3, 2),
	MUX_PG(mfp1_2_0, 1, 0, 3),

	MUX_PG(mfp2_30_29, 2, 29, 2),
	MUX_PG(mfp2_28_27, 2, 27, 2),
	MUX_PG(mfp2_26_24, 2, 24, 3),
	MUX_PG(mfp2_23, 2, 23, 1),
	MUX_PG(mfp2_22, 2, 22, 1),
	MUX_PG(mfp2_21, 2, 21, 1),
	MUX_PG(mfp2_20, 2, 20, 1),
	MUX_PG(mfp2_19_17, 2, 17, 3),
	MUX_PG(mfp2_16_14, 2, 14, 3),
	MUX_PG(mfp2_13_11, 2, 11, 3),
	MUX_PG(mfp2_10_9, 2, 9, 2),
	MUX_PG(mfp2_8_7, 2, 7, 2),
	MUX_PG(mfp2_6_5, 2, 5, 2),
	MUX_PG(mfp2_4_3, 2, 3, 2),
	MUX_PG(mfp2_2_0, 2, 0, 3),
	MUX_PG_DUMMY(sd1_dummy),

	MUX_PG(mfp3_30, 3, 30, 1),
	MUX_PG(mfp3_29_28, 3, 28, 2),
	MUX_PG(mfp3_23_22, 3, 22, 2),
	MUX_PG(mfp3_21_19, 3, 19, 3),
	MUX_PG(mfp3_18_16, 3, 16, 3),
	MUX_PG(mfp3_15_14, 3, 14, 2),
	MUX_PG(mfp3_13_12, 3, 12, 2),
	MUX_PG(mfp3_11_10, 3, 10, 2),
	MUX_PG(mfp3_9_8, 3, 8, 2),
	MUX_PG(mfp3_7_6, 3, 6, 2),
	MUX_PG(mfp3_5_4, 3, 4, 2),
	MUX_PG(mfp3_3, 3, 3, 1),
	MUX_PG(mfp3_2, 3, 2, 1),
	MUX_PG(mfp3_1_0_nand_ceb2, 3, 0, 2),
	MUX_PG(mfp3_1_0_nand_ceb3, 3, 0, 2),
	
	MUX_PG_DUMMY(dsi_dummy),
	MUX_PG_DUMMY(nand_dummy),
	MUX_PG_DUMMY(uart2_dummy),
	MUX_PG_DUMMY(uart3_dummy),
	MUX_PG_DUMMY(i2c1_dummy),
	MUX_PG_DUMMY(i2c2_dummy),

	PADDRV_PG(paddrv0_29_28, 0, 28),
	PADDRV_PG(paddrv0_23_22, 0, 22),
	PADDRV_PG(paddrv0_21_20, 0, 20),
	PADDRV_PG(paddrv0_19_18, 0, 18),
	PADDRV_PG(paddrv0_17_16, 0, 16),
	PADDRV_PG(paddrv0_15_14, 0, 14),
	PADDRV_PG(paddrv0_13_12, 0, 12),
	PADDRV_PG(paddrv0_11_10, 0, 10),
	PADDRV_PG(paddrv0_9_8, 0, 8),
	PADDRV_PG(paddrv0_7_6, 0, 6),
	PADDRV_PG(paddrv0_5_4, 0, 4),
	PADDRV_PG(paddrv0_3_2, 0, 2),
	PADDRV_PG(paddrv0_1_0, 0, 0),

	PADDRV_PG(paddrv1_31_30, 1, 30),
	PADDRV_PG(paddrv1_29_28, 1, 28),
	PADDRV_PG(paddrv1_27_26, 1, 26),
	PADDRV_PG(paddrv1_25_24, 1, 24),
	PADDRV_PG(paddrv1_23_22, 1, 22),
	PADDRV_PG(paddrv1_21_20, 1, 20),
	PADDRV_PG(paddrv1_19_18, 1, 18),
	PADDRV_PG(paddrv1_17_16, 1, 16),
	PADDRV_PG(paddrv1_15_14, 1, 14),
	PADDRV_PG(paddrv1_13_12, 1, 12),
	PADDRV_PG(paddrv1_11_10, 1, 10),

	PADDRV_PG(paddrv2_31_30, 2, 30),
	PADDRV_PG(paddrv2_29_28, 2, 28),
	PADDRV_PG(paddrv2_27_26, 2, 26),
	PADDRV_PG(paddrv2_24_23, 2, 23),
	PADDRV_PG(paddrv2_22_21, 2, 21),
	PADDRV_PG(paddrv2_19_18, 2, 18),
	PADDRV_PG(paddrv2_13_12, 2, 12),
	PADDRV_PG(paddrv2_3_2, 2, 2),
};

int atm7059_num_groups = ARRAY_SIZE(atm7059_groups);

static const char * const nor_groups[] = {
	"mfp0_25_23",
	"mfp0_5",
	"mfp0_2_1_i2s0",
	"mfp0_2_1_i2s1",
	"mfp0_0",
	"mfp1_31_29_ks_in2",
	"mfp1_31_29_ks_in1",
	"mfp1_31_29_ks_in0",
	"mfp1_28_26_ks_in3",
	"mfp1_28_26_ks_out0",
	"mfp1_28_26_ks_out1",
	"mfp1_25_23",
	"mfp1_16_14",
	"mfp1_6_5",
	"mfp1_4_3",
	"mfp1_2_0",
	"mfp2_28_27",
	"mfp2_19_17",
	"mfp2_16_14",
	"mfp2_13_11",
	"mfp2_10_9",
	"mfp2_8_7",
	"mfp2_4_3",
	"mfp3_23_22",
	"mfp3_13_12",
};

static const char * const eth_rmii_groups[] = {
	"mfp0_22_20",
	"mfp0_18_16_eth_txd0",
	"mfp0_18_16_eth_txd1",
	"mfp0_15_13_rmii_txen",
	"mfp0_15_13_rmii_rxen",
	"mfp0_10_8_rmii_rxd1",
	"mfp0_10_8_rmii_rxd0",
	"mfp0_7_6",
	"eth_smi_dummy",
};

static const char * const eth_smii_groups[] = {
	"mfp0_22_20",
	"mfp0_18_16_eth_txd0",
	"mfp0_18_16_eth_txd1",
	"mfp0_7_6",
	"eth_smi_dummy",
};

static const char * const spi0_groups[] = {
	"mfp1_20_19",
	"mfp1_18_17",
	"mfp1_9_7",
	"mfp1_4_3",
	"mfp1_2_0",
	"mfp2_26_24",

};

static const char * const spi1_groups[] = {
	"mfp2_2_0",
	"mfp3_21_19",
	"mfp3_18_16",
};

static const char * const spi2_groups[] = {
	"mfp0_22_20",
	"mfp0_18_16_eth_txd0",
	"mfp0_18_16_eth_txd1",
	"mfp0_7_6",
	"mfp3_2",
};

static const char * const spi3_groups[] = {
	"mfp0_15_13_rmii_txen",
	"mfp0_15_13_rmii_rxen",
	"mfp0_10_8_rmii_rxd1",
	"mfp0_10_8_rmii_rxd0",
};

static const char * const sens0_groups[] = {
	"mfp3_29_28",
	"mfp3_23_22",
	"mfp3_15_14",
	"mfp3_13_12",
};

static const char * const sens1_groups[] = {
	"mfp0_25_23",
	"mfp1_31_29_ks_in2",
	"mfp1_31_29_ks_in1",
	"mfp1_31_29_ks_in0",
	"mfp1_28_26_ks_in3",
	"mfp1_28_26_ks_out0",
	"mfp1_28_26_ks_out1",
	"mfp1_25_23",
	"mfp3_23_22",
	"mfp3_11_10",
	"mfp3_9_8",
	"mfp3_7_6",
	"mfp3_5_4",
};

static const char * const uart0_groups[] = {
	"mfp2_23",
	"mfp2_22",
	"mfp2_2_0",
	"mfp3_21_19",
};

static const char * const uart1_groups[] = {
	"mfp2_13_11",
	"mfp3_18_16",
};

static const char * const uart2_groups[] = {
	"mfp0_15_13_rmii_txen",
	"mfp0_15_13_rmii_rxen",
	"mfp0_10_8_rmii_rxd1",
	"mfp0_10_8_rmii_rxd0",
	"mfp1_20_19",
	"mfp1_18_17",
	"mfp1_9_7",
	"mfp2_26_24",
	"mfp2_23",
	"mfp2_22",
	"mfp2_19_17",
	"mfp2_16_14",
	"mfp2_13_11",
	"mfp2_2_0",
	"mfp3_21_19",
	"mfp3_18_16",
	"uart2_dummy"
};

static const char * const uart3_groups[] = {
	"mfp2_21",
	"mfp2_20",
	"uart3_dummy"
};

static const char * const uart4_groups[] = {
	"mfp0_22_20",
	"mfp0_7_6",
	"mfp3_11_10",
	"mfp3_9_8",
};

static const char * const uart5_groups[] = {
	"mfp0_10_8_rmii_rxd1",
	"mfp0_10_8_rmii_rxd0",
	"mfp1_28_26_ks_out0",
	"mfp1_25_23",
	"mfp2_21",
	"mfp2_20",
	"mfp2_19_17",
	"mfp2_16_14",
};

static const char * const uart6_groups[] = {
	"mfp0_18_16_eth_txd0",
	"mfp0_18_16_eth_txd1",
	"mfp3_7_6",
	"mfp3_5_4",
};

static const char * const i2s0_groups[] = {
    "mfp0_5",
	"mfp0_4_3",
	"mfp0_2_1_i2s0",
};

static const char * const i2s1_groups[] = {
	"mfp0_2_1_i2s1",
	"mfp0_0",
	"i2s1_dummy",
	"mfp1_2_0",
	"mfp2_2_0",
	"mfp3_21_19",
};

static const char * const pcm1_groups[] = {
	"mfp0_4_3",
	"mfp1_2_0",
	"mfp2_2_0",
	"mfp3_21_19",
	"mfp3_11_10",
	"mfp3_9_8",
	"mfp3_7_6",
	"mfp3_5_4",
};

static const char * const pcm0_groups[] = {
	"mfp0_2_1_i2s0",
	"mfp0_2_1_i2s1",
	"mfp1_4_3",
	"mfp1_2_0",
};

static const char * const ks_groups[] = {
	"mfp1_31_29_ks_in2",
	"mfp1_31_29_ks_in1",
	"mfp1_31_29_ks_in0",
	"mfp1_28_26_ks_in3",
	"mfp1_28_26_ks_out0",
	"mfp1_28_26_ks_out1",
	"mfp1_25_23",
};

static const char * const jtag_groups[] = {
	"mfp1_31_29_ks_in2",
	"mfp1_31_29_ks_in1",
	"mfp1_31_29_ks_in0",
	"mfp1_28_26_ks_in3",
	"mfp1_28_26_ks_out1",
	"mfp2_19_17",
	"mfp2_13_11",
	"mfp2_8_7",
	"mfp2_6_5",
};

static const char * const pwm0_groups[] = {
	"mfp1_31_29_ks_in2",
	"mfp1_31_29_ks_in0",
	"mfp0_15_13_rmii_txen",
	"mfp3_13_12",
};

static const char * const pwm1_groups[] = {
	"mfp0_15_13_rmii_rxen",
	"mfp1_31_29_ks_in1",
	"mfp1_28_26_ks_in3",
	"mfp3_23_22",
};

static const char * const pwm2_groups[] = {
	"mfp0_25_23",
	"mfp0_10_8_rmii_rxd1",
	"mfp1_28_26_ks_out0",
	"mfp1_25_23",
};

static const char * const pwm3_groups[] = {
	"mfp0_10_8_rmii_rxd0",
	"mfp1_28_26_ks_out1",
	"mfp1_16_14",
};

static const char * const pwm4_groups[] = {
	"mfp0_25_23",
	"mfp0_22_20",
	"mfp0_18_16_eth_txd0",
	"mfp1_31_29_ks_in0",
	"mfp3_11_10",
	"mfp3_1_0_nand_ceb3",
};

static const char * const pwm5_groups[] = {
	"mfp0_18_16_eth_txd1",
	"mfp1_31_29_ks_in1",
	"mfp3_9_8",
	"mfp3_1_0_nand_ceb2",
};

static const char * const p0_groups[] = {
	"mfp1_31_29_ks_in2",
	"mfp1_31_29_ks_in0",
};

static const char * const sd0_groups[] = {
	"mfp1_28_26_ks_out0",
	"mfp1_28_26_ks_out1",
	"mfp1_25_23",
	"mfp1_16_14",
	"mfp1_13_12",
	"mfp1_9_7",
	"mfp2_19_17",
	"mfp2_16_14",
	"mfp2_13_11",
	"mfp2_10_9",
	"mfp2_8_7",
	"mfp2_6_5",
};

static const char * const sd1_groups[] = {
	"mfp1_18_17",
	"mfp1_16_14",
	"mfp1_13_12",
	"mfp1_11_10",
	"mfp2_30_29",
	"mfp2_26_24",
	"mfp2_10_9",
	"mfp2_4_3",
	"sd1_dummy",
};

static const char * const sd2_groups[] = {
	"mfp3_3",
};

static const char * const i2c0_groups[] = {
	"mfp2_2_0",
	"mfp3_21_19",
	"mfp3_18_16",
};

static const char * const i2c1_groups[] = {
	"mfp3_18_16",
	"i2c1_dummy"
};

static const char * const i2c2_groups[] = {
	"i2c2_dummy"
};

static const char * const i2c3_groups[] = {
	"mfp1_4_3",
	"mfp3_7_6",
	"mfp3_5_4",
};

static const char * const lvds_groups[] = {
	"mfp1_22_21",
	"mfp1_6_5",
	"mfp2_28_27",
};

static const char * const ts_groups[] = {
	"mfp1_22_21",
	"mfp1_6_5",
};

static const char * const lcd0_groups[] = {
	"mfp0_25_23",
	"mfp1_16_14",
	"mfp1_22_21",
	"mfp1_13_12",
	"mfp1_11_10",
	"mfp1_6_5",
	"mfp2_30_29",
	"mfp2_28_27",
};


static const char * const usb30_groups[] = {
	"mfp1_31_29_ks_in1",
};

static const char * const clko_25m_groups[] = {
	"mfp3_30",
};

static const char * const mipi_csi_groups[] = {
	"mfp3_29_28",
	"mfp3_15_14",
};

static const char * const dsi_groups[] = {
	"mfp1_20_19",
	"mfp1_18_17",
	"mfp1_13_12",
	"mfp1_11_10",
	"mfp1_9_7",
	"mfp2_30_29",
	"dsi_dummy",
};

static const char * const nand_groups[] = {
	"mfp3_3",
	"mfp3_2",
	"mfp3_1_0_nand_ceb2",
	"mfp3_1_0_nand_ceb3",
	"nand_dummy",
};

static const char * const spdif_groups[] = {
	"mfp3_21_19",
};


static const char * const sirq0_groups[] = {
	"sirq0_dummy",
};

static const char * const sirq1_groups[] = {
	"sirq1_dummy",
};

static const char * const sirq2_groups[] = {
	"sirq2_dummy",
};


#define FUNCTION(fname)					\
	{						\
		.name = #fname,				\
		.groups = fname##_groups,		\
		.ngroups = ARRAY_SIZE(fname##_groups),	\
	}

const struct owl_pinmux_func atm7059_functions[] = {
	[OWL_MUX_NOR] = FUNCTION(nor),
	[OWL_MUX_ETH_RMII] = FUNCTION(eth_rmii),
	[OWL_MUX_ETH_SMII] = FUNCTION(eth_smii),
	[OWL_MUX_SPI0] = FUNCTION(spi0),
	[OWL_MUX_SPI1] = FUNCTION(spi1),
	[OWL_MUX_SPI2] = FUNCTION(spi2),
	[OWL_MUX_SPI3] = FUNCTION(spi3),
	[OWL_MUX_SENS0] = FUNCTION(sens0),
	[OWL_MUX_SENS1] = FUNCTION(sens1),
	[OWL_MUX_UART0] = FUNCTION(uart0),
	[OWL_MUX_UART1] = FUNCTION(uart1),
	[OWL_MUX_UART2] = FUNCTION(uart2),
	[OWL_MUX_UART3] = FUNCTION(uart3),
	[OWL_MUX_UART4] = FUNCTION(uart4),
	[OWL_MUX_UART5] = FUNCTION(uart5),
	[OWL_MUX_UART6] = FUNCTION(uart6),
	[OWL_MUX_I2S0] = FUNCTION(i2s0),
	[OWL_MUX_I2S1] = FUNCTION(i2s1),
	[OWL_MUX_PCM1] = FUNCTION(pcm1),
	[OWL_MUX_PCM0] = FUNCTION(pcm0),
	[OWL_MUX_KS] = FUNCTION(ks),
	[OWL_MUX_JTAG] = FUNCTION(jtag),
	[OWL_MUX_PWM0] = FUNCTION(pwm0),
	[OWL_MUX_PWM1] = FUNCTION(pwm1),
	[OWL_MUX_PWM2] = FUNCTION(pwm2),
	[OWL_MUX_PWM3] = FUNCTION(pwm3),
	[OWL_MUX_PWM4] = FUNCTION(pwm4),
	[OWL_MUX_PWM5] = FUNCTION(pwm5),
	[OWL_MUX_P0] = FUNCTION(p0),
	[OWL_MUX_SD0] = FUNCTION(sd0),
	[OWL_MUX_SD1] = FUNCTION(sd1),
	[OWL_MUX_SD2] = FUNCTION(sd2),
	[OWL_MUX_I2C0] = FUNCTION(i2c0),
	[OWL_MUX_I2C1] = FUNCTION(i2c1),
	[OWL_MUX_I2C2] = FUNCTION(i2c2),
	[OWL_MUX_I2C3] = FUNCTION(i2c3),
	[OWL_MUX_DSI] = FUNCTION(dsi),
	[OWL_MUX_LVDS] = FUNCTION(lvds),
	[OWL_MUX_USB30] = FUNCTION(usb30),
	[OWL_MUX_CLKO_25M] = FUNCTION(clko_25m),
	[OWL_MUX_MIPI_CSI] = FUNCTION(mipi_csi),
	[OWL_MUX_DSI] = FUNCTION(dsi),
	[OWL_MUX_NAND] = FUNCTION(nand),
	[OWL_MUX_SPDIF] = FUNCTION(spdif),
	[OWL_MUX_SIRQ0] = FUNCTION(sirq0),
	[OWL_MUX_SIRQ1] = FUNCTION(sirq1),
	[OWL_MUX_SIRQ2] = FUNCTION(sirq2),
	[OWL_MUX_TS] = FUNCTION(ts),
	[OWL_MUX_LCD0] = FUNCTION(lcd0),
};

int atm7059_num_functions = ARRAY_SIZE(atm7059_functions);

/******PAD SCHIMTT CONFIGURES*************************/

/*refer to SPEC, schmitt trigger*/
static unsigned int P_ETH_REF_CLK_schimtt_funcs[] = {
	OWL_MUX_ETH_RMII,
	OWL_MUX_ETH_SMII,
};

static unsigned int P_ETH_TXEN_schimtt_funcs[] = {
	OWL_MUX_SPI3,
};

static unsigned int P_ETH_TXD0_schimtt_funcs[] = {
	OWL_MUX_SPI2,
	OWL_MUX_UART6,
};

static unsigned int P_I2S_LRCLK1_schimtt_funcs[] = {
	OWL_MUX_PCM0,
};

static unsigned int P_UART0_TX_schimtt_funcs[] = {
	OWL_MUX_I2C0,
};

static unsigned int P_SPI0_SCLK_schimtt_funcs[] = {
	OWL_MUX_SPI0,
	OWL_MUX_I2C3,
	OWL_MUX_PCM0,
};

static unsigned int P_SD0_CLK_schimtt_funcs[] = {
	OWL_MUX_JTAG,
};

static unsigned int P_KS_IN0_schimtt_funcs[] = {
	OWL_MUX_JTAG,
};

static unsigned int P_I2C0_SCLK_schimtt_funcs[] = {
	OWL_MUX_I2C0,
	OWL_MUX_I2C1,
	OWL_MUX_SPI1,
};

static unsigned int P_I2S_MCLK1_schimtt_funcs[] = {
	OWL_MUX_PCM0,
};

static unsigned int P_KS_OUT0_schimtt_funcs[] = {
	OWL_MUX_SENS1,
};

static unsigned int P_KS_OUT1_schimtt_funcs[] = {
	OWL_MUX_SENS1,
};

static unsigned int P_KS_OUT2_schimtt_funcs[] = {
	OWL_MUX_SENS1,
};

static unsigned int P_UART0_RX_schimtt_funcs[] = {
	OWL_MUX_I2C0,
};

static unsigned int P_I2C0_SDATA_schimtt_funcs[] = {
	OWL_MUX_I2C0,
	OWL_MUX_I2C1,
};

static unsigned int P_I2S_BCLK0_schimtt_funcs[] = {
	OWL_MUX_PCM0,
};

static unsigned int P_I2S_MCLK0_schimtt_funcs[] = {
	OWL_MUX_PCM1,
};

static unsigned int P_SPI0_SS_schimtt_funcs[] = {
	OWL_MUX_SPI0,
};

static unsigned int P_SPI0_MISO_schimtt_funcs[] = {
	OWL_MUX_SPI0,
	OWL_MUX_PCM1,
	OWL_MUX_PCM0,
};

static unsigned int P_SPI0_MOSI_schimtt_funcs[] = {
	OWL_MUX_SPI0,
	OWL_MUX_I2C3,
	OWL_MUX_PCM0,
};

static unsigned int P_I2C1_SCLK_schimtt_funcs[] = {
	OWL_MUX_I2C1,
};

static unsigned int P_I2C1_SDATA_schimtt_funcs[] = {
	OWL_MUX_I2C1,
};

static unsigned int P_I2C2_SCLK_schimtt_funcs[] = {
	OWL_MUX_I2C2,
};

static unsigned int P_I2C2_SDATA_schimtt_funcs[] = {
	OWL_MUX_I2C2,
};

static unsigned int P_PCM1_IN_schimtt_funcs[] = {
	OWL_MUX_PCM1,
};

static unsigned int P_PCM1_SYNC_schimtt_funcs[] = {
	OWL_MUX_PCM1,
};

static unsigned int P_PCM1_CLK_schimtt_funcs[] = {
	OWL_MUX_PCM1,
};

static unsigned int P_ETH_TXD1_schimtt_funcs[] = {
	OWL_MUX_SPI2,
};

static unsigned int P_ETH_RXER_schimtt_funcs[] = {
	OWL_MUX_SPI3,
};

static unsigned int P_ETH_CRS_DV_schimtt_funcs[] = {
	OWL_MUX_SPI2,
	OWL_MUX_UART4,
};

static unsigned int P_ETH_RXD1_schimtt_funcs[] = {
	OWL_MUX_SPI3,
};

static unsigned int P_ETH_RXD0_schimtt_funcs[] = {
	OWL_MUX_SPI3,
};

static unsigned int P_UART2_RX_schimtt_funcs[] = {
	OWL_MUX_UART2,
};

static unsigned int P_UART2_CTSB_schimtt_funcs[] = {
	OWL_MUX_UART2,
};

static unsigned int P_UART3_RX_schimtt_funcs[] = {
	OWL_MUX_UART3,
};

static unsigned int P_UART3_CTSB_schimtt_funcs[] = {
	OWL_MUX_UART3,
};

static unsigned int P_UART2_RTSB_schimtt_funcs[] = {
	OWL_MUX_UART0,
};

static unsigned int P_UART3_RTSB_schimtt_funcs[] = {
	OWL_MUX_UART5,
};

static unsigned int P_I2S_LRCLK0_schimtt_funcs[] = {
	OWL_MUX_PCM1,
};

static unsigned int P_DSI_DP0_schimtt_funcs[] = {
	OWL_MUX_UART2,
	OWL_MUX_SPI0,
};

static unsigned int P_DSI_DN0_schimtt_funcs[] = {
	OWL_MUX_SPI0,
};

static unsigned int P_DSI_DP2_schimtt_funcs[] = {
	OWL_MUX_SPI0,
};

static unsigned int P_DSI_DN2_schimtt_funcs[] = {
	OWL_MUX_UART2,
	OWL_MUX_SPI0,
};


#if 1
#define SCHIMMT_CONF(pad_name, reg_n, sft)		\
	{	\
		.schimtt_funcs = pad_name##_schimtt_funcs,		\
		.num_schimtt_funcs =	\
			ARRAY_SIZE(pad_name##_schimtt_funcs),	\
		.reg_num = reg_n,	\
		.shift = sft,	\
	}

#else
#define SCHIMMT_CONF(pad_name, reg_n, sft)		\
	{	\
		.schimtt_funcs = pad_name##_schimtt_funcs,		\
		.reg_num = reg_n,	\
		.shift = sft,	\
	}
#endif

#if 0
#define PAD_SCHIMMT_CONF(pad_name, reg_num, shift)		\
	struct owl_pinconf_schimtt pad_name##_schimmt_conf	\
		= SCHIMMT_CONF(pad_name, reg_num, shift)
#endif

#define PAD_SCHIMMT_CONF(pad_name, reg_n, sft)		\
	struct owl_pinconf_schimtt pad_name##_schimmt_conf = {	\
		.schimtt_funcs = pad_name##_schimtt_funcs,		\
		.num_schimtt_funcs =	\
			ARRAY_SIZE(pad_name##_schimtt_funcs),	\
		.reg_num = reg_n,	\
		.shift = sft,	\
	}

/*PAD_ST0*/
static PAD_SCHIMMT_CONF(P_I2C0_SDATA, 0, 30);
static PAD_SCHIMMT_CONF(P_UART0_RX, 0, 29);
static PAD_SCHIMMT_CONF(P_I2S_MCLK1, 0, 23);
static PAD_SCHIMMT_CONF(P_ETH_REF_CLK, 0, 22);
static PAD_SCHIMMT_CONF(P_ETH_TXEN, 0, 21);
static PAD_SCHIMMT_CONF(P_ETH_TXD0, 0, 20);
static PAD_SCHIMMT_CONF(P_I2S_LRCLK1, 0, 19);
static PAD_SCHIMMT_CONF(P_DSI_DP0, 0, 16);
static PAD_SCHIMMT_CONF(P_DSI_DN0, 0, 15);
static PAD_SCHIMMT_CONF(P_UART0_TX, 0, 14);
static PAD_SCHIMMT_CONF(P_SPI0_SCLK, 0, 13);
static PAD_SCHIMMT_CONF(P_SD0_CLK, 0, 12);
static PAD_SCHIMMT_CONF(P_KS_IN0, 0, 11);
static PAD_SCHIMMT_CONF(P_I2C0_SCLK, 0, 7);
static PAD_SCHIMMT_CONF(P_KS_OUT0, 0, 6);
static PAD_SCHIMMT_CONF(P_KS_OUT1, 0, 5);
static PAD_SCHIMMT_CONF(P_KS_OUT2, 0, 4);

/*PAD_ST1*/
static PAD_SCHIMMT_CONF(P_DSI_DP2, 1, 31);
static PAD_SCHIMMT_CONF(P_DSI_DN2, 1, 30);
static PAD_SCHIMMT_CONF(P_I2S_LRCLK0, 1, 29);
static PAD_SCHIMMT_CONF(P_UART3_CTSB, 1, 27);
static PAD_SCHIMMT_CONF(P_UART3_RTSB, 1, 26);
static PAD_SCHIMMT_CONF(P_UART3_RX, 1, 25);
static PAD_SCHIMMT_CONF(P_UART2_RTSB, 1, 24);
static PAD_SCHIMMT_CONF(P_UART2_CTSB, 1, 23);
static PAD_SCHIMMT_CONF(P_UART2_RX, 1, 22);
static PAD_SCHIMMT_CONF(P_ETH_RXD0, 1, 21);
static PAD_SCHIMMT_CONF(P_ETH_RXD1, 1, 20);
static PAD_SCHIMMT_CONF(P_ETH_CRS_DV, 1, 19);
static PAD_SCHIMMT_CONF(P_ETH_RXER, 1, 18);
static PAD_SCHIMMT_CONF(P_ETH_TXD1, 1, 17);
static PAD_SCHIMMT_CONF(P_PCM1_CLK, 1, 11);
static PAD_SCHIMMT_CONF(P_PCM1_IN, 1, 10);
static PAD_SCHIMMT_CONF(P_PCM1_SYNC, 1, 9);
static PAD_SCHIMMT_CONF(P_I2C1_SCLK, 1, 8);
static PAD_SCHIMMT_CONF(P_I2C1_SDATA, 1, 7);
static PAD_SCHIMMT_CONF(P_I2C2_SCLK, 1, 6);
static PAD_SCHIMMT_CONF(P_I2C2_SDATA, 1, 5);
static PAD_SCHIMMT_CONF(P_SPI0_MOSI, 1, 4);
static PAD_SCHIMMT_CONF(P_SPI0_MISO, 1, 3);
static PAD_SCHIMMT_CONF(P_SPI0_SS, 1, 2);
static PAD_SCHIMMT_CONF(P_I2S_BCLK0, 1, 1);
static PAD_SCHIMMT_CONF(P_I2S_MCLK0, 1, 0);


/******PAD PULL UP/DOWN CONFIGURES*************************/
#define PULL_CONF(reg_n, sft, w, pup, pdn)		\
	{	\
		.reg_num = reg_n,	\
		.shift = sft,	\
		.width = w,	\
		.pullup = pup,		\
		.pulldown = pdn,	\
	}

#define PAD_PULL_CONF(pad_name, reg_num,	\
			shift, width, pull_up, pull_down)		\
	struct owl_pinconf_reg_pull pad_name##_pull_conf	\
		= PULL_CONF(reg_num, shift, width, pull_up, pull_down)

/*PAD_PULLCTL0*/
static PAD_PULL_CONF(P_PCM1_SYNC, 0, 30, 1, 0x1, 0);
static PAD_PULL_CONF(P_PCM1_OUT, 0, 29, 1, 0x1, 0);
static PAD_PULL_CONF(P_KS_OUT2, 0, 28, 1, 0x1, 0);
static PAD_PULL_CONF(P_LCD0_D17, 0, 27, 1, 0x1, 0);
static PAD_PULL_CONF(P_DSI_DN3, 0, 26, 1, 0x1, 0);
static PAD_PULL_CONF(P_ETH_RXER, 0, 16, 1, 0, 0x1);
static PAD_PULL_CONF(P_SIRQ0, 0, 14, 2, 0x1, 0x2);
static PAD_PULL_CONF(P_SIRQ1, 0, 12, 2, 0x1, 0x2);
static PAD_PULL_CONF(P_SIRQ2, 0, 10, 2, 0x1, 0x2);
static PAD_PULL_CONF(P_I2C0_SDATA, 0, 9, 1, 0x1, 0);
static PAD_PULL_CONF(P_I2C0_SCLK, 0, 8, 1, 0x1, 0);
static PAD_PULL_CONF(P_KS_IN0, 0, 7, 1, 0x1, 0);
static PAD_PULL_CONF(P_KS_IN1, 0, 6, 1, 0x1, 0);
static PAD_PULL_CONF(P_KS_IN2, 0, 5, 1, 0x1, 0);
static PAD_PULL_CONF(P_KS_IN3, 0, 4, 1, 0x1, 0);
static PAD_PULL_CONF(P_KS_OUT0, 0, 2, 1, 0x1, 0);
static PAD_PULL_CONF(P_KS_OUT1, 0, 1, 1, 0x1, 0);
static PAD_PULL_CONF(P_DSI_DP1, 0, 0, 1, 0x1, 0);

/*PAD_PULLCTL1*/
static PAD_PULL_CONF(P_DSI_CP, 1, 31, 1, 0x1, 0);
static PAD_PULL_CONF(P_DSI_CN, 1, 30, 1, 0x1, 0);
static PAD_PULL_CONF(P_DSI_DN2, 1, 28, 1, 0x1, 0);
static PAD_PULL_CONF(P_DNAND_RDBN, 1, 25, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD0_D0, 1, 17, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD0_D1, 1, 16, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD0_D2, 1, 15, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD0_D3, 1, 14, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD0_CMD, 1, 13, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD0_CLK, 1, 12, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD1_CMD, 1, 11, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD1_D0, 1, 6, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD1_D1, 1, 5, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD1_D2, 1, 4, 1, 0x1, 0);
static PAD_PULL_CONF(P_SD1_D3, 1, 3, 1, 0x1, 0);
static PAD_PULL_CONF(P_UART0_RX, 1, 2, 1, 0x1, 0);
static PAD_PULL_CONF(P_UART0_TX, 1, 1, 1, 0x1, 0);
static PAD_PULL_CONF(P_CLKO_25M, 1, 0, 1, 0, 0x1);

/*PAD_PULLCTL2*/
static PAD_PULL_CONF(P_SPI0_SCLK, 2, 12, 1, 0x1, 0);
static PAD_PULL_CONF(P_SPI0_MOSI, 2, 11, 1, 0x1, 0);
static PAD_PULL_CONF(P_I2C1_SDATA, 2, 10, 1, 0x1, 0);
static PAD_PULL_CONF(P_I2C1_SCLK, 2, 9, 1, 0x1, 0);
static PAD_PULL_CONF(P_I2C2_SDATA, 2, 8, 1, 0x1, 0);
static PAD_PULL_CONF(P_I2C2_SCLK, 2, 7, 1, 0x1, 0);
static PAD_PULL_CONF(P_DNAND_DQSN, 2, 5, 2, 0x2, 0);
static PAD_PULL_CONF(P_DNAND_DQS, 2, 3, 2, 0x1, 0x2);
static PAD_PULL_CONF(P_DNAND_D0, 2, 2, 1, 0x1, 0);
static PAD_PULL_CONF(P_DNAND_D1, 2, 2, 1, 0x1, 0);
static PAD_PULL_CONF(P_DNAND_D2, 2, 2, 1, 0x1, 0);
static PAD_PULL_CONF(P_DNAND_D3, 2, 2, 1, 0x1, 0);
static PAD_PULL_CONF(P_DNAND_D4, 2, 2, 1, 0x1, 0);
static PAD_PULL_CONF(P_DNAND_D5, 2, 2, 1, 0x1, 0);
static PAD_PULL_CONF(P_DNAND_D6, 2, 2, 1, 0x1, 0);
static PAD_PULL_CONF(P_DNAND_D7, 2, 2, 1, 0x1, 0);


/********PAD INFOS*****************************/
#define PAD_TO_GPIO(padnum)		\
		((padnum < NUM_GPIOS) ? padnum : -1)

#define PAD_INFO(name)		\
	{		\
		.pad = name,	\
		.gpio = PAD_TO_GPIO(name),		\
		.schimtt = NULL,		\
		.pull = NULL,	\
	}

#define PAD_INFO_SCHIMTT(name)	\
	{		\
		.pad = name,	\
		.gpio = PAD_TO_GPIO(name),		\
		.schimtt = &name##_schimmt_conf,		\
		.pull = NULL,	\
	}

#define PAD_INFO_PULL(name)	\
	{		\
		.pad = name,	\
		.gpio = PAD_TO_GPIO(name),		\
		.schimtt = NULL,		\
		.pull = &name##_pull_conf,	\
	}

#define PAD_INFO_SCHIMTT_PULL(name)	\
	{		\
		.pad = name,	\
		.gpio = PAD_TO_GPIO(name),		\
		.schimtt = &name##_schimmt_conf,		\
		.pull = &name##_pull_conf,	\
	}

/* Pad info table for the pinmux subsystem */
struct owl_pinconf_pad_info atm7059_pad_tab[NUM_PADS] = {
	[P_RESERA0] = PAD_INFO(P_RESERA0),
	[P_RESERA1] = PAD_INFO(P_RESERA1),
	[P_RESERA2] = PAD_INFO(P_RESERA2),
	[P_RESERA3] = PAD_INFO(P_RESERA3),
	[P_RESERA4] = PAD_INFO(P_RESERA4),
	[P_RESERA5] = PAD_INFO(P_RESERA5),
	[P_RESERA6] = PAD_INFO(P_RESERA6),
	[P_RESERA7] = PAD_INFO(P_RESERA7),
	[P_RESERA8] = PAD_INFO(P_RESERA8),
	[P_RESERA9] = PAD_INFO(P_RESERA9),
	[P_RESERA10] = PAD_INFO(P_RESERA10),
	[P_RESERA11] = PAD_INFO(P_RESERA11),
	[P_DNAND_DQS] = PAD_INFO_PULL(P_DNAND_DQS),
	[P_DNAND_DQSN] = PAD_INFO_PULL(P_DNAND_DQSN),
	[P_ETH_TXD0] = PAD_INFO_SCHIMTT(P_ETH_TXD0),
	[P_ETH_TXD1] = PAD_INFO_SCHIMTT(P_ETH_TXD1),
	[P_ETH_TXEN] = PAD_INFO_SCHIMTT(P_ETH_TXEN),
	[P_ETH_RXER] = PAD_INFO_SCHIMTT_PULL(P_ETH_RXER),
	[P_ETH_CRS_DV] = PAD_INFO_SCHIMTT(P_ETH_CRS_DV),
	[P_ETH_RXD1] = PAD_INFO_SCHIMTT(P_ETH_RXD1),
	[P_ETH_RXD0] = PAD_INFO_SCHIMTT(P_ETH_RXD0),
	[P_ETH_REF_CLK] = PAD_INFO_SCHIMTT(P_ETH_REF_CLK),
	[P_ETH_MDC] = PAD_INFO(P_ETH_MDC),
	[P_ETH_MDIO] = PAD_INFO(P_ETH_MDIO),
	[P_SIRQ0] = PAD_INFO_PULL(P_SIRQ0),
	[P_SIRQ1] = PAD_INFO_PULL(P_SIRQ1),
	[P_SIRQ2] = PAD_INFO_PULL(P_SIRQ2),
	[P_I2S_D0] = PAD_INFO(P_I2S_D0),
	[P_I2S_BCLK0] = PAD_INFO_SCHIMTT(P_I2S_BCLK0),
	[P_I2S_LRCLK0] = PAD_INFO_SCHIMTT(P_I2S_LRCLK0),
	[P_I2S_MCLK0] = PAD_INFO_SCHIMTT(P_I2S_MCLK0),
	[P_I2S_D1] = PAD_INFO(P_I2S_D1),
	[P_I2S_BCLK1] = PAD_INFO(P_I2S_BCLK1),
	[P_I2S_LRCLK1] = PAD_INFO_SCHIMTT(P_I2S_LRCLK1),
	[P_I2S_MCLK1] = PAD_INFO_SCHIMTT(P_I2S_MCLK1),
	[P_KS_IN0] = PAD_INFO_SCHIMTT_PULL(P_KS_IN0),
	[P_KS_IN1] = PAD_INFO_PULL(P_KS_IN1),
	[P_KS_IN2] = PAD_INFO_PULL(P_KS_IN2),
	[P_KS_IN3] = PAD_INFO_PULL(P_KS_IN3),
	[P_KS_OUT0] = PAD_INFO_SCHIMTT_PULL(P_KS_OUT0),
	[P_KS_OUT1] = PAD_INFO_SCHIMTT_PULL(P_KS_OUT1),
	[P_KS_OUT2] = PAD_INFO_SCHIMTT_PULL(P_KS_OUT2),
	[P_LVDS_OEP] = PAD_INFO(P_LVDS_OEP),
	[P_LVDS_OEN] = PAD_INFO(P_LVDS_OEN),
	[P_LVDS_ODP] = PAD_INFO(P_LVDS_ODP),
	[P_LVDS_ODN] = PAD_INFO(P_LVDS_ODN),
	[P_LVDS_OCP] = PAD_INFO(P_LVDS_OCP),
	[P_LVDS_OCN] = PAD_INFO(P_LVDS_OCN),
	[P_LVDS_OBP] = PAD_INFO(P_LVDS_OBP),
	[P_LVDS_OBN] = PAD_INFO(P_LVDS_OBN),
	[P_LVDS_OAP] = PAD_INFO(P_LVDS_OAP),
	[P_LVDS_OAN] = PAD_INFO(P_LVDS_OAN),
	[P_LVDS_EEP] = PAD_INFO(P_LVDS_EEP),
	[P_LVDS_EEN] = PAD_INFO(P_LVDS_EEN),
	[P_LVDS_EDP] = PAD_INFO(P_LVDS_EDP),
	[P_LVDS_EDN] = PAD_INFO(P_LVDS_EDN),
	[P_LVDS_ECP] = PAD_INFO(P_LVDS_ECP),
	[P_LVDS_ECN] = PAD_INFO(P_LVDS_ECN),
	[P_LVDS_EBP] = PAD_INFO(P_LVDS_EBP),
	[P_LVDS_EBN] = PAD_INFO(P_LVDS_EBN),
	[P_LVDS_EAP] = PAD_INFO(P_LVDS_EAP),
	[P_LVDS_EAN] = PAD_INFO(P_LVDS_EAN),
	[P_LCD0_D18] = PAD_INFO(P_LCD0_D18),
	[P_LCD0_D17] = PAD_INFO_PULL(P_LCD0_D17),
	[P_DSI_DP3] = PAD_INFO(P_DSI_DP3),
	[P_DSI_DN3] = PAD_INFO_PULL(P_DSI_DN3),
	[P_DSI_DP1] = PAD_INFO_PULL(P_DSI_DP1),
	[P_DSI_DN1] = PAD_INFO(P_DSI_DN1),
	[P_DSI_CP] = PAD_INFO_PULL(P_DSI_CP),
	[P_DSI_CN] = PAD_INFO_PULL(P_DSI_CN),
	[P_DSI_DP0] = PAD_INFO_SCHIMTT(P_DSI_DP0),
	[P_DSI_DN0] = PAD_INFO_SCHIMTT(P_DSI_DN0),
	[P_DSI_DP2] = PAD_INFO_SCHIMTT(P_DSI_DP2),
	[P_DSI_DN2] = PAD_INFO_SCHIMTT_PULL(P_DSI_DN2),
	[P_SD0_D0] = PAD_INFO_PULL(P_SD0_D0),
	[P_SD0_D1] = PAD_INFO_PULL(P_SD0_D1),
	[P_SD0_D2] = PAD_INFO_PULL(P_SD0_D2),
	[P_SD0_D3] = PAD_INFO_PULL(P_SD0_D3),
	[P_SD1_D0] = PAD_INFO_PULL(P_SD1_D0),
	[P_SD1_D1] = PAD_INFO_PULL(P_SD1_D1),
	[P_SD1_D2] = PAD_INFO_PULL(P_SD1_D2),
	[P_SD1_D3] = PAD_INFO_PULL(P_SD1_D3),
	[P_SD0_CMD] = PAD_INFO_PULL(P_SD0_CMD),
	[P_SD0_CLK] = PAD_INFO_SCHIMTT_PULL(P_SD0_CLK),
	[P_SD1_CMD] = PAD_INFO_PULL(P_SD1_CMD),
	[P_SD1_CLK] = PAD_INFO(P_SD1_CLK),
	[P_SPI0_SCLK] = PAD_INFO_SCHIMTT_PULL(P_SPI0_SCLK),
	[P_SPI0_SS] = PAD_INFO_SCHIMTT(P_SPI0_SS),
	[P_SPI0_MISO] = PAD_INFO_SCHIMTT(P_SPI0_MISO),
	[P_SPI0_MOSI] = PAD_INFO_SCHIMTT_PULL(P_SPI0_MOSI),
	[P_UART0_RX] = PAD_INFO_SCHIMTT_PULL(P_UART0_RX),
	[P_UART0_TX] = PAD_INFO_SCHIMTT_PULL(P_UART0_TX),
	[P_I2C0_SCLK] = PAD_INFO_SCHIMTT_PULL(P_I2C0_SCLK),
	[P_I2C0_SDATA] = PAD_INFO_SCHIMTT_PULL(P_I2C0_SDATA),

	[P_RESERC30] = PAD_INFO(P_RESERC30),
	[P_SENSOR0_PCLK] = PAD_INFO(P_SENSOR0_PCLK),
	[P_RESERD0] = PAD_INFO(P_RESERD0),
	[P_RESERD1] = PAD_INFO(P_RESERD1),
	[P_RESERD2] = PAD_INFO(P_RESERD2),
	[P_RESERD3] = PAD_INFO(P_RESERD3),
	[P_RESERD4] = PAD_INFO(P_RESERD4),
	[P_RESERD5] = PAD_INFO(P_RESERD5),
	[P_RESERD6] = PAD_INFO(P_RESERD6),
	[P_RESERD7] = PAD_INFO(P_RESERD7),
	[P_RESERD8] = PAD_INFO(P_RESERD8),
	[P_RESERD9] = PAD_INFO(P_RESERD9),
	[P_SENSOR0_CKOUT] = PAD_INFO(P_SENSOR0_CKOUT),
	[P_RESERD11] = PAD_INFO(P_RESERD11),
	[P_DNAND_ALE] = PAD_INFO(P_DNAND_ALE),
	[P_DNAND_CLE] = PAD_INFO(P_DNAND_CLE),
	[P_DNAND_CEB0] = PAD_INFO(P_DNAND_CEB0),
	[P_DNAND_CEB1] = PAD_INFO(P_DNAND_CEB1),
	[P_DNAND_CEB2] = PAD_INFO(P_DNAND_CEB2),
	[P_DNAND_CEB3] = PAD_INFO(P_DNAND_CEB3),
	[P_UART2_RX] = PAD_INFO_SCHIMTT(P_UART2_RX),
	[P_UART2_TX] = PAD_INFO(P_UART2_TX),
	[P_UART2_RTSB] = PAD_INFO_SCHIMTT(P_UART2_RTSB),
	[P_UART2_CTSB] = PAD_INFO_SCHIMTT(P_UART2_CTSB),
	[P_UART3_RX] = PAD_INFO_SCHIMTT(P_UART3_RX),
	[P_UART3_TX] = PAD_INFO(P_UART3_TX),
	[P_UART3_RTSB] = PAD_INFO_SCHIMTT(P_UART3_RTSB),
	[P_UART3_CTSB] = PAD_INFO_SCHIMTT(P_UART3_CTSB),
	[P_RESERD26] = PAD_INFO(P_RESERD26),
	[P_RESERD27] = PAD_INFO(P_RESERD27),
	[P_PCM1_IN] = PAD_INFO_SCHIMTT(P_PCM1_IN),
	[P_PCM1_CLK] = PAD_INFO_SCHIMTT(P_PCM1_CLK),
	[P_PCM1_SYNC] = PAD_INFO_SCHIMTT_PULL(P_PCM1_SYNC),
	[P_PCM1_OUT] = PAD_INFO_PULL(P_PCM1_OUT),
	[P_I2C1_SCLK] = PAD_INFO_SCHIMTT_PULL(P_I2C1_SCLK),
	[P_I2C1_SDATA] = PAD_INFO_SCHIMTT_PULL(P_I2C1_SDATA),
	[P_I2C2_SCLK] = PAD_INFO_SCHIMTT_PULL(P_I2C2_SCLK),
	[P_I2C2_SDATA] = PAD_INFO_SCHIMTT_PULL(P_I2C2_SDATA),
	[P_CSI_DN0] = PAD_INFO(P_CSI_DN0),
	[P_CSI_DP0] = PAD_INFO(P_CSI_DP0),
	[P_CSI_DN1] = PAD_INFO(P_CSI_DN1),
	[P_CSI_DP1] = PAD_INFO(P_CSI_DP1),
	[P_CSI_CN] = PAD_INFO(P_CSI_CN),
	[P_CSI_CP] = PAD_INFO(P_CSI_CP),
	[P_CSI_DN2] = PAD_INFO(P_CSI_DN2),
	[P_CSI_DP2] = PAD_INFO(P_CSI_DP2),
	[P_CSI_DN3] = PAD_INFO(P_CSI_DN3),
	[P_CSI_DP3] = PAD_INFO(P_CSI_DP3),
	[P_DNAND_D0] = PAD_INFO_PULL(P_DNAND_D0),
	[P_DNAND_D1] = PAD_INFO_PULL(P_DNAND_D1),
	[P_DNAND_D2] = PAD_INFO_PULL(P_DNAND_D2),
	[P_DNAND_D3] = PAD_INFO_PULL(P_DNAND_D3),
	[P_DNAND_D4] = PAD_INFO_PULL(P_DNAND_D4),
	[P_DNAND_D5] = PAD_INFO_PULL(P_DNAND_D5),
	[P_DNAND_D6] = PAD_INFO_PULL(P_DNAND_D6),
	[P_DNAND_D7] = PAD_INFO_PULL(P_DNAND_D7),
	[P_DNAND_WRB] = PAD_INFO(P_DNAND_WRB),
	[P_DNAND_RDB] = PAD_INFO(P_DNAND_RDB),
	[P_DNAND_RDBN] = PAD_INFO_PULL(P_DNAND_RDBN),
	[P_DNAND_RB0] = PAD_INFO(P_DNAND_RB0),
	[P_PORB] = PAD_INFO(P_PORB),
	[P_CLKO_25M] = PAD_INFO_PULL(P_CLKO_25M),
	[P_BSEL] = PAD_INFO(P_BSEL),
	[P_PKG0] = PAD_INFO(P_PKG0),
	[P_PKG1] = PAD_INFO(P_PKG1),
	[P_PKG2] = PAD_INFO(P_PKG2),
	[P_PKG3] = PAD_INFO(P_PKG3),
};


static struct owl_gpio_pad atm7059_gpio_pads[] = {
	{.gpio = _GPIOD(28), .reg = SPS_PWR_CTL, .mask = 0x00000080, .bit = 7},
	{.gpio = _GPIOD(29), .reg = SPS_PWR_CTL, .mask = 0x00000040, .bit = 6},
	{.gpio = _GPIOD(30), .reg = SPS_PWR_CTL, .mask = 0x00000020, .bit = 5},
	{.gpio = _GPIOD(31), .reg = SPS_PWR_CTL, .mask = 0x00000010, .bit = 4},
};

struct owl_gpio_pad_data atm7059_gpio_pad_data = 
{
	.gpio_pads = atm7059_gpio_pads, 
	.size = sizeof(atm7059_gpio_pads)/sizeof(struct owl_gpio_pad),
};


