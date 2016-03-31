/*
 * (C) Copyright 2012
 * Actions Semi .Inc
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#include <common.h>
#include <malloc.h>

#include <asm/arch/actions_reg_owl.h>
#include <asm/io.h>

#include "pinctrl_data-owl.h"

DECLARE_GLOBAL_DATA_PTR;


#define MFP_CTL_REG(i)    (MFP_CTL0 + (MFP_CTL1 - MFP_CTL0) * (i))
#define PAD_ST_REG(i)     (PAD_ST0 + (PAD_ST1 - PAD_ST0) * (i))
#define PAD_PULLCTL_REG(i) (PAD_PULLCTL0 + (PAD_PULLCTL1 - PAD_PULLCTL0) * (i))
#define PAD_DRV_REG(i)    (PAD_DRV0 + (PAD_DRV1 - PAD_DRV0) * (i))
#define GPIO_OUTEN_REG(i) (GPIO_AOUTEN + (GPIO_BOUTEN - GPIO_AOUTEN) * (i))
#define GPIO_INEN_REG(i)  (GPIO_AINEN + (GPIO_BINEN - GPIO_AINEN) * (i))
#define GPIO_DAT_REG(i)   (GPIO_ADAT + (GPIO_BDAT - GPIO_ADAT) * (i))

#define MFP_CTL0_BT_D0_D3_MASK                  (7 << 29)
#define MFP_CTL0_BT_D4_D7_MASK                  (7 << 26)
#define MFP_CTL0_BT_PCLK_MASK                   (3 << 24)
#define MFP_CTL0_BT_VSYNC_MASK                  (3 << 22)
#define MFP_CTL0_BT_HSYNC_MASK                  (3 << 20)
#define MFP_CTL0_BT_TS_ERROR_MASK               (1 << 19)
#define MFP_CTL0_RMII_TXD01_MASK                (7 << 16)
#define MFP_CTL0_RMII_TXEN_RXER_MASK            (7 << 13)
#define MFP_CTL0_RMII_CRS_DV_MASK               (3 << 11)
#define MFP_CTL0_RMII_RXD10_MASK                (7 << 8)
#define MFP_CTL0_RMII_REF_CLK_MASK              (3 << 6)
#define MFP_CTL0_I2S_PCM1_MASK                  (3 << 3)
#define MFP_CTL0_I2S_PCM0_MASK                  (1 << 2)

/*MFP_CTL1 MASK*/
#define MFP_CTL1_KS_IN0_2_MASK                  (7 << 29)
#define MFP_CTL1_KS_IN3_OUT0_1_MASK             (7 << 26)
#define MFP_CTL1_KS_OUT2_MASK                   (7 << 23)
#define MFP_CTL1_LVDS_O_PN_MASK                 (1 << 22)
#define MFP_CTL1_LVDS_EO_PN_MASK                (1 << 21)
#define MFP_CTL1_LCD0_HSYNC1_MASK               (3 << 19)
#define MFP_CTL1_LCD0_VSYNC1_MASK               (3 << 17)
#define MFP_CTL1_LCD0_D17_MASK                  (3 << 15)
#define MFP_CTL1_LCD0_D16_MASK                  (3 << 13)
#define MFP_CTL1_LCD0_D9_MASK                   (3 << 11)
#define MFP_CTL1_LCD0_D8_D0_1_MASK              (1 << 10)
#define MFP_CTL1_LCD0_DCLK1_MASK                (7 << 7)
#define MFP_CTL1_SPI0_I2C_PCM_MASK              (3 << 4)
#define MFP_CTL1_SPI0_I2S_PCM_MASK              (7 << 1)
#define MFP_CTL1__10_EXX_MFP_MASK               (1 << 0)

/*MFP_CTL2 MASK*/
#define MFP_CTL2_LCD0_LDE1_MASK                 (7 << 24)
#define MFP_CTL2_UART2_RTSB_MASK                (1 << 23)
#define MFP_CTL2_UART2_CTSB_MASK                (1 << 22)
#define MFP_CTL2_UART3_RTSB_MASK                (1 << 21)
#define MFP_CTL2_UART3_CTSB_MASK                (1 << 20)
#define MFP_CTL2_SD0_D0_MASK                    (7 << 17)
#define MFP_CTL2_SD0_D1_MASK                    (7 << 14)
#define MFP_CTL2_SD0_D2_D3_MASK                 (7 << 11)
#define MFP_CTL2_SD0_D4_D7_MASK                 (3 << 9)
#define MFP_CTL2_SD0_CMD_MASK                   (3 << 7)
#define MFP_CTL2_SD0_CLK_MASK                   (3 << 5)
#define MFP_CTL2_SD1_CMD_MASK                   (3 << 3)
#define MFP_CTL2_UART0_RX_MASK                  (7 << 0)

/*MFP_CTL3 MASK*/
#define MFP_CTL3_SENS0_CKOUT_MASK               (1 << 31)
#define MFP_CTL3_CSI_CN_CP_MASK                 (3 << 28)
#define MFP_CTL3_DNAND_CEB3_MASK                (1 << 27)
#define MFP_CTL3_DNAND_CEB2_MASK                (1 << 26)
#define MFP_CTL3_DNAND_CEB0_MASK                (1 << 25)
#define MFP_CTL3_DNAND_RDBN_QDS_DQSN_MASK       (1 << 24)
#define MFP_CTL3_UART0_TX_MASK                  (7 << 19)
#define MFP_CTL3_I2C0_MFP_MASK                  (7 << 16)
#define MFP_CTL3_CSI_DN_DP_MASK                 (3 << 14)
#define MFP_CTL3_SENS0_PCLK_MASK                (3 << 12)
#define MFP_CTL3_SENS1_PCLK_MASK                (3 << 10)
#define MFP_CTL3_SENS1_VSYNC_MASK               (7 << 7)
#define MFP_CTL3_SENS1_HSYNC_MASK               (7 << 4)
#define MFP_CTL3_SENS1_CKOUT_MASK               (3 << 2)
#define MFP_CTL3_SENS1_D0_7_MASK                (3 << 0)

/******************************************************************************/
/*MFP_CTL0*/
#define MFP_CTL0_BT_D0_D3(x)                    (((x) & 7) << 29)
#define MFP_CTL0_BT_D4_D7(x)                    (((x) & 7) << 26)
#define MFP_CTL0_BT_PCLK(x)                     (((x) & 3) << 24)
#define MFP_CTL0_BT_VSYNC(x)                    (((x) & 3) << 22)
#define MFP_CTL0_BT_HSYNC(x)                    (((x) & 3) << 20)
#define MFP_CTL0_BT_TS_ERROR                    (1 << 19)
#define MFP_CTL0_RMII_TXD01(x)                  (((x) & 7) << 16)
#define MFP_CTL0_RMII_TXEN_RXER(x)              (((x) & 7) << 13)
#define MFP_CTL0_RMII_CRS_DV(x)                 (((x) & 3) << 11)
#define MFP_CTL0_RMII_RXD10(x)                  (((x) & 7) << 8)
#define MFP_CTL0_RMII_REF_CLK(x)                (((x) & 3) << 6)
#define MFP_CTL0_I2S_PCM1(x)                    (((x) & 3) << 3)
#define MFP_CTL0_I2S_PCM0                       (1 << 2)

/*MFP_CTL1*/
#define MFP_CTL1_KS_IN0_2(x)                    (((x) & 7) << 29)
#define MFP_CTL1_KS_IN3_OUT0_1(x)               (((x) & 7) << 26)
#define MFP_CTL1_KS_OUT2(x)                     (((x) & 7) << 23)
#define MFP_CTL1_LVDS_O_PN                      (1 << 22)
#define MFP_CTL1_LVDS_EO_PN                     (1 << 21)
#define MFP_CTL1_LCD0_HSYNC1(x)                 (((x) & 3) << 19)
#define MFP_CTL1_LCD0_VSYNC1(x)                 (((x) & 3) << 17)
#define MFP_CTL1_LCD0_D17(x)                    (((x) & 3) << 15)
#define MFP_CTL1_LCD0_D16(x)                    (((x) & 3) << 13)
#define MFP_CTL1_LCD0_D9(x)                     (((x) & 3) << 11)
#define MFP_CTL1_LCD0_D8_D0_1                   (1 << 10)
#define MFP_CTL1_LCD0_DCLK1(x)                  (((x) & 7) << 7)
#define MFP_CTL1_SPI0_I2C_PCM(x)                (((x) & 3) << 4)
#define MFP_CTL1_SPI0_I2S_PCM(x)                (((x) & 7) << 1)
#define MFP_CTL1__10_EXX_MFP                    (1 << 0)

/*MFP_CTL2*/
#define MFP_CTL2_LCD0_LDE1(x)                   (((x) & 7) << 24)
#define MFP_CTL2_UART2_RTSB                     (1 << 23)
#define MFP_CTL2_UART2_CTSB                     (1 << 22)
#define MFP_CTL2_UART3_RTSB                     (1 << 21)
#define MFP_CTL2_UART3_CTSB                     (1 << 20)
#define MFP_CTL2_SD0_D0(x)                      (((x) & 7) << 17)
#define MFP_CTL2_SD0_D1(x)                      (((x) & 7) << 14)
#define MFP_CTL2_SD0_D2_D3(x)                   (((x) & 7) << 11)
#define MFP_CTL2_SD0_D4_D7(x)                   (((x) & 3) << 9)
#define MFP_CTL2_SD0_CMD(x)                     (((x) & 3) << 7)
#define MFP_CTL2_SD0_CLK(x)                     (((x) & 3) << 5)
#define MFP_CTL2_SD1_CMD(x)                     (((x) & 3) << 3)
#define MFP_CTL2_UART0_RX(x)                    (((x) & 7) << 0)

/*MFP_CTL3*/
#define MFP_CTL3_SENS0_CKOUT                    (1 << 31)
#define MFP_CTL3_CSI_CN_CP(x)                   (((x) & 3) << 28)
#define MFP_CTL3_DNAND_CEB3                     (1 << 27)
#define MFP_CTL3_DNAND_CEB2                     (1 << 26)
#define MFP_CTL3_DNAND_CEB0                     (1 << 25)
#define MFP_CTL3_DNAND_RDBN_QDS_DQSN            (1 << 24)
#define MFP_CTL3_UART0_TX(x)                    (((x) & 7) << 19)
#define MFP_CTL3_I2C0_MFP(x)                    (((x) & 7) << 16)
#define MFP_CTL3_CSI_DN_DP(x)                   (((x) & 3) << 14)
#define MFP_CTL3_SENS0_PCLK(x)                  (((x) & 3) << 12)
#define MFP_CTL3_SENS1_PCLK(x)                  (((x) & 3) << 10)
#define MFP_CTL3_SENS1_VSYNC(x)                 (((x) & 7) << 7)
#define MFP_CTL3_SENS1_HSYNC(x)                 (((x) & 7) << 4)
#define MFP_CTL3_SENS1_CKOUT(x)                 (((x) & 3) << 2)
#define MFP_CTL3_SENS1_D0_7(x)                  (((x) & 3) << 0)

/******************************************************************************/
/*PWMx_CTL*/
/*bit 9-31 Reserved*/
#define PWMx_CTL_POL_SEL                        (1 << 8)
/*bit 6-7 Reserved*/
#define PWMx_CTL_DUTY(x)                        (((x) & 0x3F) << 0)
/******************************************************************************/
/*PAD_PULLCTL0*/
/******************************************************************************/
/*PAD_PULLCTL1*/
/******************************************************************************/
/*PAD_PULLCTL2*/
/******************************************************************************/
/*PAD_ST0*/
#define PAD_ST0_P_I2C0_SDATA_ST                 (1 << 30)
#define PAD_ST0_P_UART0_RX_ST                   (1 << 29)
#define PAD_ST0_P_BT_HSYNC_ST                   (1 << 28)
#define PAD_ST0_P_BT_VSYNC_ST                   (1 << 27)
#define PAD_ST0_P_BT_D0_ST                      (1 << 26)
#define PAD_ST0_P_BT_D1_ST                      (1 << 25)
#define PAD_ST0_P_BT_D5_ST                      (1 << 24)
#define PAD_ST0_P_I2S_MCLK1_ST                  (1 << 23)
#define PAD_ST0_P_ETH_REF_CLK_ST                (1 << 22)
#define PAD_ST0_P_ETH_TXEN_ST                   (1 << 21)
#define PAD_ST0_P_ETH_TXD0_ST                   (1 << 20)
#define PAD_ST0_P_I2S_LRCLK1_ST                 (1 << 19)
#define PAD_ST0_P_SENSOR1_VSYNC_ST              (1 << 18)
#define PAD_ST0_P_SENSOR1_HSYNC_ST              (1 << 17)
#define PAD_ST0_P_LCD0_DCLK1_ST                 (1 << 16)
#define PAD_ST0_P_LCD0_HSYNC1_ST                (1 << 15)
#define PAD_ST0_P_UART0_TX_ST                   (1 << 14)
#define PAD_ST0_P_SPI0_SCLK_ST                  (1 << 13)
#define PAD_ST0_P_SD0_CLK_ST                    (1 << 12)
#define PAD_ST0_P_KS_IN0_ST                     (1 << 11)
#define PAD_ST0_P_BT_PCLK_ST                    (1 << 10)
#define PAD_ST0_P_SENSOR0_PCLK_ST               (1 << 9)
#define PAD_ST0_P_SENSOR1_PCLK_ST               (1 << 8)
#define PAD_ST0_P_I2C0_SCLK_ST                  (1 << 7)
#define PAD_ST0_P_KS_OUT0_ST                    (1 << 6)
#define PAD_ST0_P_KS_OUT1_ST                    (1 << 5)
#define PAD_ST0_P_KS_OUT2_ST                    (1 << 4)
#define PAD_ST0_P_SENSOR1_D1_ST                 (1 << 3)
#define PAD_ST0_P_SENSOR1_D2_ST                 (1 << 2)
#define PAD_ST0_P_SENSOR1_D3_ST                 (1 << 1)
#define PAD_ST0_P_SENSOR1_D7_ST                 (1 << 0)
/******************************************************************************/
/*PAD_ST1*/
#define PAD_ST1_P_LCD0_VSYNC1_ST                (1 << 31)
#define PAD_ST1_P_LCD0_LDE1_ST                  (1 << 30)
#define PAD_ST1_P_I2S_LRCLK0_ST                 (1 << 29)
#define PAD_ST1_P_UART4_RX_ST                   (1 << 28)
#define PAD_ST1_P_UART3_CTSB_ST                 (1 << 27)
#define PAD_ST1_P_UART3_RTSB_ST                 (1 << 26)
#define PAD_ST1_P_UART3_RX_ST                   (1 << 25)
#define PAD_ST1_P_UART2_RTSB_ST                 (1 << 24)
#define PAD_ST1_P_UART2_CTSB_ST                 (1 << 23)
#define PAD_ST1_P_UART2_RX_ST                   (1 << 22)
#define PAD_ST1_P_ETH_RXD0_ST                   (1 << 21)
#define PAD_ST1_P_ETH_RXD1_ST                   (1 << 20)
#define PAD_ST1_P_ETH_CRS_DV_ST                 (1 << 19)
#define PAD_ST1_P_ETH_RXER_ST                   (1 << 18)
#define PAD_ST1_P_ETH_TXD1_ST                   (1 << 17)
#define PAD_ST1_P_BT_D7_ST                      (1 << 16)
#define PAD_ST1_P_BT_D6_ST                      (1 << 15)
#define PAD_ST1_P_BT_D4_ST                      (1 << 14)
#define PAD_ST1_P_BT_D3_ST                      (1 << 13)
#define PAD_ST1_P_BT_D2_ST                      (1 << 12)
#define PAD_ST1_P_PCM1_CLK_ST                   (1 << 11)
#define PAD_ST1_P_PCM1_IN_ST                    (1 << 10)
#define PAD_ST1_P_PCM1_SYNC_ST                  (1 << 9)
#define PAD_ST1_P_I2C1_SCLK_ST                  (1 << 8)
#define PAD_ST1_P_I2C1_SDATA_ST                 (1 << 7)
#define PAD_ST1_P_I2C2_SCLK_ST                  (1 << 6)
#define PAD_ST1_P_I2C2_SDATA_ST                 (1 << 5)
#define PAD_ST1_P_SPI0_MOSI_ST                  (1 << 4)
#define PAD_ST1_P_SPI0_MISO_ST                  (1 << 3)
#define PAD_ST1_P_SPI0_SS_ST                    (1 << 2)
#define PAD_ST1_P_I2S_BCLK0_ST                  (1 << 1)
#define PAD_ST1_P_I2S_MCLK0_ST                  (1 << 0)
/******************************************************************************/
/*PAD_CTL*/
#define PAD_CTL_JTAG_CLR                        (0x1 << 3)
#define PAD_CTL_SPI0_CLR                        (0x1 << 2)
#define PAD_CTL_PADEN                           (0x1 << 1)
#define PAD_CTL_BTSEL                           (0x1 << 0)

struct pinctrl_dev_info {
	char name[30];
	int dev_node;
	bool is_valid;

	const struct owl_pinconf_pad_info *padinfo;
	const struct owl_pinctrl_pin_desc *pins;
	unsigned npins;
	const struct owl_pinmux_func *functions;
	unsigned nfunctions;
	const struct owl_group *groups;
	unsigned ngroups;
};

#define MAX_PINCTRL_DEV		2
static struct pinctrl_dev_info pinctrl_info[MAX_PINCTRL_DEV] = {
	[0] = {
		.is_valid = -1,
	},
	[1] = {
		.is_valid = -1,
	},
};

static int pin_name_to_number(
		struct pinctrl_dev_info *pinctrl,
		const char *name)
{
	unsigned pin_selector = 0;
	unsigned npins = pinctrl->npins;
	const struct owl_pinctrl_pin_desc *pins = pinctrl->pins;

	for (pin_selector = 0; pin_selector < npins; pin_selector++) {
		const char *pname = pins[pin_selector].name;
		if (!pname)
			continue;

		if (!strcmp(pname, name))
			return pins[pin_selector].number;

	}
	return -1;
}


static int group_name_to_selector(
		struct pinctrl_dev_info *pinctrl,
		const char *name)
{
	unsigned group_selector = 0;
	unsigned ngroups = pinctrl->ngroups;
	const struct owl_group *groups = pinctrl->groups;

	while (group_selector < ngroups) {
		const char *gname = groups[group_selector].name;
		if (!strcmp(gname, name)) {
			/*debug("%s found group selector %u for %s\n",
				pinctrl->name,
				group_selector,
				name);
			*/
			return group_selector;
		}

		group_selector++;
	}
	return -1;
}

static int function_name_to_selector(
		struct pinctrl_dev_info *pinctrl,
		const char *name)
{
	unsigned function_selector = 0;
	unsigned nfuncs = pinctrl->nfunctions;
	const struct owl_pinmux_func *functions = pinctrl->functions;

	while (function_selector < nfuncs) {
		const char *fname = functions[function_selector].name;
		if (!strcmp(fname, name)) {
			/*debug("%s found function selector %u for %s\n",
				pinctrl->name,
				function_selector,
				name);
			*/
			return function_selector;
		}

		function_selector++;
	}
	return -1;
}


static inline int get_group_mfp_mask_val(
		const struct owl_group *g,
		int function, u32 *mask, u32 *val)
{
	int i;
	u32 option_num;
	u32 option_mask;

	for (i = 0; i < g->nfuncs; i++) {
		if (g->funcs[i] == function)
			break;
	}
	if (i == g->nfuncs)
		return -1;

	option_num = (1 << g->mfpctl_width);
	if (i > option_num)
		i -= option_num;

	option_mask = option_num - 1;
	*mask = (option_mask  << g->mfpctl_shift);
	*val = (i << g->mfpctl_shift);

	return 0;

}

static int pinctrl_set_function(
		struct pinctrl_dev_info *pinctrl, const char *group_name,
		const char *function_name)
{
	int group;
	int function;
	const struct owl_group *g;
	int i;
	int need_schimtt = 0;
	u32 group_schimtt_val[PINCTRL_STREGS] = {0};
	u32 group_schimtt_mask[PINCTRL_STREGS] = {0};

	u32 g_val;
	u32 g_mask;

	debug(
		"pinctrl_set_function0, group %s, function %s\n",
		group_name, function_name);

	group = group_name_to_selector(pinctrl, group_name);
	if (group < 0)
		return -1;

	debug("pinctrl_set_function 1\n");

	function = function_name_to_selector(pinctrl, function_name);
	if (function < 0)
		return -1;

	debug("pinctrl_set_function 2\n");

	g = &pinctrl->groups[group];

	if (g->mfpctl_regnum >= 0) {
		u32 mfpval;

		debug("pinctrl_set_function 21\n");

		if (get_group_mfp_mask_val(g, function, &g_mask, &g_val))
			return -1;

		debug("pinctrl_set_function 22\n");

/*we've done all the checkings. From now on ,we will set hardware.***/
/*No more errors should happen, otherwise it will be hard to roll back*/
		mfpval = readl(MFP_CTL_REG(g->mfpctl_regnum));
		mfpval &= (~g_mask);
		mfpval |= g_val;
		writel(mfpval, MFP_CTL_REG(g->mfpctl_regnum));
	}

	debug("pinctrl_set_function 3\n");

	/*check each pad of this group for schimtt info, */
	/*fill the group_schimtt_mask & group_schimtt_val*/
	for (i = 0; i < g->padcnt; i++) {
		int pad_num;
		const struct owl_pinconf_pad_info *pad_info;
		struct owl_pinconf_schimtt *schimtt_info;

		pad_num = g->pads[i];
		pad_info = pinctrl->padinfo;

		schimtt_info = pad_info[pad_num].schimtt;

		if (schimtt_info && schimtt_info->reg_num >= 0) {
			int j;

			need_schimtt = 1;

			group_schimtt_mask[schimtt_info->reg_num] |=
				(1 << schimtt_info->shift);

			for (j = 0; j < schimtt_info->num_schimtt_funcs; j++) {
				if (schimtt_info->schimtt_funcs[j] == function) {
					group_schimtt_val[schimtt_info->reg_num] |=
						(1 << schimtt_info->shift);
					break;
				}
			}
		}
	}

	debug("pinctrl_set_function 4\n");

	/*set schimtt val*/
	if (need_schimtt) {
		u32 val;
		u32 reg;
		for (i = 0; i < PINCTRL_STREGS; i++) {
			if (group_schimtt_mask[i] != 0) {
				reg = PAD_ST_REG(i);
				val = readl(reg);
				val &= (~group_schimtt_mask[i]);
				val |= group_schimtt_val[i];
				writel(val, reg);
			}
		}
	}

	debug("pinctrl_set_function 5\n");

	return 0;
}

static int owl_group_pinconf_reg(
		const struct owl_group *g,
		enum owl_pinconf_param param,
		u32 *reg, u32 *bit, u32 *width)
{
	switch (param) {
	case OWL_PINCONF_PARAM_PADDRV:
		if (g->paddrv_regnum < 0)
			return -1;

		*reg = PAD_DRV_REG(g->paddrv_regnum);
		*bit = g->paddrv_shift;
		*width = g->paddrv_width;
		break;
	default:
		return -1;
	}

	return 0;

}

static int owl_group_pinconf_arg2val(
		const struct owl_group *g,
		enum owl_pinconf_param param,
		u32 arg, u32 *val)
{
	switch (param) {
	case OWL_PINCONF_PARAM_PADDRV:
		*val = arg;
		break;

	default:
		return -1;
	}

	return 0;
}

int pinctrl_group_set_config(struct pinctrl_dev_info *pinctrl,
		const char *group_name, unsigned long config)
{
	int ret = 0;
	const struct owl_group *g;
	u32 reg = 0, bit = 0, width = 0;
	u32 val = 0, mask = 0;
	u32 tmp;
	enum owl_pinconf_param param = OWL_PINCONF_UNPACK_PARAM(config);
	u32 arg = OWL_PINCONF_UNPACK_ARG(config);

	int group;

	debug(
		"pinctrl_group_set_config, group %s, config 0x%lx\n",
		group_name, config);

	group = group_name_to_selector(pinctrl, group_name);
	if (group < 0)
		return -1;

	g = &pinctrl->groups[group];
	ret = owl_group_pinconf_reg(g, param, &reg, &bit, &width);
	if (ret)
		return ret;

	ret = owl_group_pinconf_arg2val(g, param, arg, &val);
	if (ret)
		return ret;

	/* Update register */
	mask = (1 << width) - 1;
	mask = mask << bit;
	tmp = readl(reg);
	tmp &= ~mask;
	tmp |= val << bit;
	writel(tmp, reg);

	return ret;
}

static int owl_pad_pinconf_reg(
		const struct owl_pinconf_pad_info *pad,
		enum owl_pinconf_param param,
		u32 *reg, u32 *bit, u32 *width)
{
	switch (param) {
	case OWL_PINCONF_PARAM_PULL:
		if ((!pad->pull) || (pad->pull->reg_num < 0))
			return -1;

		*reg = PAD_PULLCTL_REG(pad->pull->reg_num);
		*bit = pad->pull->shift;
		*width = pad->pull->width;
		break;
	case OWL_PINCONF_PARAM_SCHMITT:
		debug("Cannot configure pad schmitt yet!\n");
		break;
	default:
		return -1;
	}

	return 0;
}

static int owl_pad_pinconf_arg2val(
		const struct owl_pinconf_pad_info *pad,
		enum owl_pinconf_param param,
		u32 arg, u32 *val)
{
	switch (param) {
	case OWL_PINCONF_PARAM_PULL:
		switch (arg) {
		case OWL_PINCONF_PULL_NONE:
			*val = 0;
			break;
		case OWL_PINCONF_PULL_DOWN:
			if (pad->pull->pulldown)
				*val = pad->pull->pulldown;
			else
				return -1;
			break;
		case OWL_PINCONF_PULL_UP:
			if (pad->pull->pullup)
				*val = pad->pull->pullup;
			else
				return -1;
			break;
		default:
			return -1;
		}

		break;

	case OWL_PINCONF_PARAM_SCHMITT:
		debug("Cannot configure pad schmitt yet!\n");
		break;
	default:
		return -1;
	}

	return 0;
}



int pinctrl_pin_set_config(struct pinctrl_dev_info *pinctrl,
		const char *pin_name, unsigned long config)
{
	int ret = 0;
	const struct owl_pinconf_pad_info *pad_tab;
	u32 reg = 0, bit = 0, width = 0;
	u32 val = 0, mask = 0;
	u32 tmp;
	enum owl_pinconf_param param = OWL_PINCONF_UNPACK_PARAM(config);
	u32 arg = OWL_PINCONF_UNPACK_ARG(config);
	int pin;

	debug(
		"pinctrl_pin_set_config, pin %s, config 0x%lx\n",
		pin_name, config);

	pin = pin_name_to_number(pinctrl, pin_name);
	if (pin < 0)
		return -1;

	pad_tab = &pinctrl->padinfo[pin];

	ret = owl_pad_pinconf_reg(pad_tab, param, &reg, &bit, &width);
	if (ret)
		return ret;

	ret = owl_pad_pinconf_arg2val(pad_tab, param, arg, &val);
	if (ret)
		return ret;

	/* Update register */
	mask = (1 << width) - 1;
	mask = mask << bit;
	tmp = readl(reg);
	tmp &= ~mask;
	tmp |= val << bit;
	writel(tmp, reg);

	return ret;
}

int pinctrl_pin_set_configs(
		struct pinctrl_dev_info *pinctrl, const char *pin_name,
		unsigned long *configs, int num_configs)
{
	int i;
	int ret;
	for (i = 0; i < num_configs; i++) {
		ret = pinctrl_pin_set_config(pinctrl, pin_name, configs[i]);
		if (ret)
			return ret;
	}
	return 0;
}

int pinctrl_group_set_configs(
		struct pinctrl_dev_info *pinctrl, const char *group_name,
		unsigned long *configs, int num_configs)
{
	int i;
	int ret;
	for (i = 0; i < num_configs; i++) {
		ret = pinctrl_group_set_config(pinctrl, group_name, configs[i]);
		if (ret)
			return ret;
	}
	return 0;
}


static struct pinctrl_dev_info *fdtdec_find_pinctrl_dev(int node)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(pinctrl_info); i++) {
		if (
			(pinctrl_info[i].is_valid == -1) ||
			(pinctrl_info[i].dev_node <= 0))
			continue ;

		if (pinctrl_info[i].dev_node == node)
			return &pinctrl_info[i];
	}

	return NULL;
}

static struct pinctrl_dev_info *find_pinctrl_dev_by_name(const char *name)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(pinctrl_info); i++) {
		if (pinctrl_info[i].is_valid == -1)
			continue ;

		if (!strcmp(pinctrl_info[i].name, name))
			return &pinctrl_info[i];
	}

	return NULL;
}

/*
static int pinctrl_dev_add_dev_node(const char *name, unsigned int dev_node)
{
	struct pinctrl_dev_info *pinctrl;

	pinctrl = find_pinctrl_dev_by_name(name);
	if (pinctrl) {
		pinctrl->dev_node = dev_node;
		return 0;
	} else {
		return -1;
	}
}
*/
static int pinctrl_dev_register(const char *name,
		int dev_node,
		const struct owl_pinconf_pad_info *padinfo,
		const const struct owl_pinctrl_pin_desc *pins,
		int npins,
		const struct owl_pinmux_func *functions,
		int nfunctions,
		const struct owl_group *groups,
		int ngroups)
{
	struct pinctrl_dev_info *pinctrl;
	int i;

	debug("pinctrl dev register 1\n");
	if (!name && (dev_node <= 0))
		return -1;

	debug("pinctrl dev register 2\n");
	for (i = 0; i < ARRAY_SIZE(pinctrl_info); i++) {
		debug("pinctrl dev register 3\n");

		if (pinctrl_info[i].is_valid == 1) {
			debug(
				"pininfo %d: valid %d\n",
				i, pinctrl_info[i].is_valid);
			continue ;
		}
		debug("pinctrl dev register 4\n");

		pinctrl = &pinctrl_info[i];

		if (name)
			strcpy(pinctrl->name, name);

		if (dev_node > 0)
			pinctrl->dev_node = dev_node;

		pinctrl->padinfo = padinfo;
		pinctrl->pins = pins;
		pinctrl->npins = npins;
		pinctrl->functions = functions;
		pinctrl->nfunctions = nfunctions;
		pinctrl->groups = groups;
		pinctrl->ngroups = ngroups;
		pinctrl->is_valid = 1;
		return 0;
	}

	return -1;

}

struct cfg_param {
	const char *property;
	enum owl_pinconf_param param;
};

static const struct cfg_param cfg_params[] = {
	{"actions,pull",			OWL_PINCONF_PARAM_PULL},
	{"actions,paddrv",		OWL_PINCONF_PARAM_PADDRV},
/*	{"actions,schmitt",		OWL_PINCONF_PARAM_SCHMITT},*/
};

static const char *fdtdec_next_string(const char *cur,
	const char *base, int prop_len)
{
	const void *curv = cur;

	curv += strlen(cur) + 1;
	if (curv >= (const void *)base + prop_len)
		return NULL;

	return curv;
}

#define fdtdec_for_each_string(base, len, s)	\
	for (s = base;		\
		s;						\
		s = fdtdec_next_string(s, base, len))


static int fdtdec_set_node(struct pinctrl_dev_info *pinctrl, int offset)
{
	int i, ret;
	const char *gname, *func, *pname;
	int glen, plen;

	unsigned long configs[ARRAY_SIZE(cfg_params)];
	int num_configs = 0;
	int config_val;

	gname = fdt_getprop(gd->fdt_blob, offset, "actions,groups", &glen);
	pname = fdt_getprop(gd->fdt_blob, offset, "actions,pins", &plen);

	if (!gname && !pname)
		return -1;

	for (i = 0; i < ARRAY_SIZE(cfg_params); i++) {
		config_val = fdtdec_get_int(gd->fdt_blob,
			offset, cfg_params[i].property, -1);
		if (config_val >= 0) {
			configs[num_configs] =
				OWL_PINCONF_PACK(
					cfg_params[i].param,
					config_val);
			num_configs++;
		}
	}

	func = fdt_getprop(gd->fdt_blob, offset, "actions,function", NULL);

	if (gname) {
		const char *group;
		fdtdec_for_each_string(gname, glen, group) {
			if (func) {
				ret = pinctrl_set_function(pinctrl,
					group, func);
				if (ret)
					return ret;
			}

			if (num_configs) {
				ret = pinctrl_group_set_configs(pinctrl,
					group, configs, num_configs);
				if (ret)
					return ret;
			}

		}
	}

	if (pname) {
		const char *pin;
		fdtdec_for_each_string(pname, plen, pin) {
			if (num_configs) {
				ret = pinctrl_pin_set_configs(pinctrl,
					pin, configs, num_configs);
				if (ret)
					return ret;
			}
		}
	}

	return 0;

}

int owl_fdtdec_set_pinctrl(int offset)
{
	int ret;
	int depth;
	struct pinctrl_dev_info *pinctrl;

	int parent;


	parent = fdt_parent_offset(gd->fdt_blob, offset);
	if (parent < 0)
		return -1;


	pinctrl = fdtdec_find_pinctrl_dev(parent);
	if(pinctrl == NULL)
		return -1;

	for (depth = 0; (offset >= 0) && (depth >= 0);
		offset = fdt_next_node(gd->fdt_blob, offset, &depth)) {
		if ((depth == 1) && offset > 0) {
			ret = fdtdec_set_node(pinctrl, offset);
			if (ret)
				return ret;
		}
	}

	return 0;
}

int owl_device_fdtdec_set_pinctrl_default(int dev_offset)
{
	const fdt32_t *list;
	int size;
	const fdt32_t *phandle;
	int i;
	int ret;
	int state_node;

	list = fdt_getprop(gd->fdt_blob, dev_offset, "pinctrl-0", &size);
	size /= sizeof(*list);
	for (i = 0; i < size; i++) {
		phandle = list++;
		state_node = fdt_node_offset_by_phandle(
			gd->fdt_blob, fdt32_to_cpu(*phandle));
		if (state_node > 0) {
			ret = owl_fdtdec_set_pinctrl(state_node);
			if (ret)
				return ret;
		}
	}

	return 0;
}

int owl_pinctrl_set_function(
	const char *pinctrl_dev_name,
	const char *group_name,
	const char *function_name)
{
	struct pinctrl_dev_info *pinctrl;
	int ret;

	pinctrl = find_pinctrl_dev_by_name(pinctrl_dev_name);
	if (pinctrl) {
		ret = pinctrl_set_function(pinctrl, group_name, function_name);
		return ret;
	} else {
		return -1;
	}

}

int owl_pinctrl_pin_set_configs(
		const char *pinctrl_dev_name,
		const char *pin_name,
		unsigned long *configs,
		int num_configs)
{
	struct pinctrl_dev_info *pinctrl;
	int ret;

	pinctrl = find_pinctrl_dev_by_name(pinctrl_dev_name);
	if (pinctrl) {
		ret = pinctrl_pin_set_configs(pinctrl,
			pin_name, configs, num_configs);
		return ret;
	} else {
		return -1;
	}

}

int owl_pinctrl_group_set_configs(
		const char *pinctrl_dev_name,
		const char *group_name,
		unsigned long *configs,
		int num_configs)
{
	struct pinctrl_dev_info *pinctrl;
	int ret;

	pinctrl = find_pinctrl_dev_by_name(pinctrl_dev_name);
	if (pinctrl) {
		ret = pinctrl_group_set_configs(pinctrl,
			group_name, configs, num_configs);
		return ret;
	} else {
		return -1;
	}

}


int pinctrl_init_f(void)
{
	int dev_node;
	int hog_node;
	int ret;
	debug("pinctrl init f 1\n");
	memset(pinctrl_info, 0, sizeof(pinctrl_info));

	dev_node = fdtdec_next_compatible(gd->fdt_blob,
		0, COMPAT_ACTIONS_OWL_PINCTRL);

	if (dev_node <= 0) {
		debug("Can't get pinctrl device node\n");
		return -1;
	}
	debug("pinctrl init f 2\n");

	/*do the register again, cause we don't keep bss data*/
#if defined(CONFIG_ATM7059A)
	ret = pinctrl_dev_register("atm7059a-pinctrl",
						dev_node,
						atm7059_pad_tab,
						atm7059_pads,
						atm7059_num_pads,
						atm7059_functions,
						atm7059_num_functions,
						atm7059_groups,
						atm7059_num_groups);
#elif defined(CONFIG_ATM7059TC)
	ret = pinctrl_dev_register("atm7059tc-pinctrl",
						dev_node,
						atm7059tc_pad_tab,
						atm7059tc_pads,
						atm7059tc_num_pads,
						atm7059tc_functions,
						atm7059tc_num_functions,
						atm7059tc_groups,
						atm7059tc_num_groups);
#elif defined(CONFIG_ATM7039C)
	ret = pinctrl_dev_register("atm7039c-pinctrl",
						dev_node,
						atm7039c_pad_tab,
						atm7039c_pads,
						atm7039c_num_pads,
						atm7039c_functions,
						atm7039c_num_functions,
						atm7039c_groups,
						atm7039c_num_groups);
#endif

	if (ret)
		return ret;
	debug("pinctrl init f 3\n");

	hog_node = fdtdec_lookup_phandle(gd->fdt_blob, dev_node, "pinctrl-0");
	if (hog_node > 0) {
		ret = owl_fdtdec_set_pinctrl(hog_node);
		if (ret)
			return ret;
	}
	debug("pinctrl init f 4\n");

	return 0;
}

int pinctrl_init_r(void)
{
	int dev_node;
	int ret = 0;

	/*clear the registered pinctrl_info, case we will do them again*/
	memset(pinctrl_info, 0, sizeof(pinctrl_info));

	dev_node = fdtdec_next_compatible(gd->fdt_blob,
		0, COMPAT_ACTIONS_OWL_PINCTRL);

	if (dev_node <= 0) {
		debug("Can't get pinctrl device node\n");
		return -1;
	}

	/*do the register again, cause we don't keep bss data*/
#if defined(CONFIG_ATM7059A)
	ret = pinctrl_dev_register("atm7059a-pinctrl",
						dev_node,
						atm7059_pad_tab,
						atm7059_pads,
						atm7059_num_pads,
						atm7059_functions,
						atm7059_num_functions,
						atm7059_groups,
						atm7059_num_groups);
#elif defined(CONFIG_ATM7059TC)
	ret = pinctrl_dev_register("atm7059tc-pinctrl",
						dev_node,
						atm7059tc_pad_tab,
						atm7059tc_pads,
						atm7059tc_num_pads,
						atm7059tc_functions,
						atm7059tc_num_functions,
						atm7059tc_groups,
						atm7059tc_num_groups);
#elif defined(CONFIG_ATM7039C)
	ret = pinctrl_dev_register("atm7039c-pinctrl",
						dev_node,
						atm7039c_pad_tab,
						atm7039c_pads,
						atm7039c_num_pads,
						atm7039c_functions,
						atm7039c_num_functions,
						atm7039c_groups,
						atm7039c_num_groups);
#endif
	return ret;
}
