/*
 * (C) Copyright 2012
 * Actions Semi .Inc
 */

#ifndef __OWL_CLK_H
#define __OWL_CLK_H


/* reset0 definition */
#define CMU_RST_DMAC			(0)
#define CMU_RST_SRAMI			(1)
#define CMU_RST_DDR				(2)
#define CMU_RST_NANDC			(3)
#define CMU_RST_SD0				(4)
#define CMU_RST_SD1				(5)
#define CMU_RST_DE				(7)
#define CMU_RST_LCD				(8)

/* reset1 definition */
#define CMU_RST_USB2			(32)
#define CMU_RST_UART6			(36)
#define CMU_RST_UART0			(37)
#define CMU_RST_UART1			(38)
#define CMU_RST_UART2			(39)
#define CMU_RST_SPI0			(40)
#define CMU_RST_SPI1			(41)
#define CMU_RST_SPI2			(42)
#define CMU_RST_SPI3			(43)
#define CMU_RST_I2C0			(44)
#define CMU_RST_I2C1			(45)
#define CMU_RST_UART3			(47)
#define CMU_RST_UART4			(48)
#define CMU_RST_UART5			(49)
#define CMU_RST_ETHERNET		(52)



/* DEVCLKEN0 */

#define DEVCLKEN_DDRCH1			(0)
#define DEVCLKEN_DMAC			(1)
#define DEVCLKEN_SRAMI			(2)
#define DEVCLKEN_DDRCH0			(3)
#define DEVCLKEN_NANDC			(4)
#define DEVCLKEN_SD0			(5)
#define DEVCLKEN_SD1			(6)
#define DEVCLKEN_SD2			(7)
#define DEVCLKEN_DE				(8)
#define DEVCLKEN_LCD0			(9)
#define DEVCLKEN_LCD1			(10)
#define DEVCLKEN_BISP			(14)
#define DEVCLKEN_KEY			(17)
#define DEVCLKEN_GPIO			(18)
#define DEVCLKEN_HDCP2X			(27)
#define DEVCLKEN_SHARERAM		(28)
#define DEVCLKEN_GPU3D			(30)
/* DEVCLKEN1 */
#define DEVCLKEN_UART0			(38)
#define DEVCLKEN_UART1			(39)
#define DEVCLKEN_UART2			(40)
#define DEVCLKEN_IRC			(41)
#define DEVCLKEN_SPI0			(42)
#define DEVCLKEN_SPI1			(43)
#define DEVCLKEN_SPI2			(44)
#define DEVCLKEN_SPI3			(45)
#define DEVCLKEN_I2C0			(46)
#define DEVCLKEN_I2C1			(47)
#define DEVCLKEN_PCM1			(48)
#define DEVCLKEN_UART6			(50)
#define DEVCLKEN_UART3			(51)
#define DEVCLKEN_UART4			(52)
#define DEVCLKEN_UART5			(53)
#define DEVCLKEN_ETHERNET		(54)
#define DEVCLKEN_PWM0			(55)
#define DEVCLKEN_PWM1			(56)
#define DEVCLKEN_PWM2			(57)
#define DEVCLKEN_PWM3			(58)
#define DEVCLKEN_TIMER			(59)
#define DEVCLKEN_NOC1			(60)
#define DEVCLKEN_I2C2			(62)
#define DEVCLKEN_I2C3			(63)



void owl_clk_enable(int clk_id);
void owl_clk_disable(int clk_id);

void owl_reset_assert(int rst_id);
void owl_reset_deassert(int rst_id);
void owl_reset(int rst_id);
void owl_reset_and_enable_clk(int rst_id, int clk_id);
int owl_clk_init(void);
#endif