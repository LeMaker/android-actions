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
#include <common.h>
#include <asm/io.h>

#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/owl_de.h>
#include <asm/arch/owl_lcd.h>

#include <owl_dss.h>
#include <owl_lcd.h>
#include <owl_dsi.h>
#include <linux/list.h>
#include <linux/fb.h>

int platform_get_lcd_par(struct owl_lcd *lcd_par)
{
	debug("platform lcd: pltfm get lcd par\n");

	lcd_par->port_type = LCD_PORT_TYPE_RGB;
	lcd_par->mode = &owl_mode_800_480;
	lcd_par->data_width = PARALLEL_IF_DATA_WIDTH_24BIT;
	lcd_par->vsync_inversion = 0;
	lcd_par->hsync_inversion = 0;
	lcd_par->dclk_inversion = 0;
	lcd_par->lde_inversion = 0;
	return 0;
}

int platform_enable_lcd(void)
{
	u32 tmp;

	debug("platform lcd: pltfm enable lcd\n");
/****get pin*********************************/
	tmp = readl(PAD_CTL);
	tmp |= 0x2;
	writel(tmp, PAD_CTL);

	tmp = readl(MFP_CTL1);
	tmp &= 0xff80003f;
	tmp |= 0x00600000;
	writel(tmp, MFP_CTL1);

	enable_lcdc();
	return 0;
}

/****dsi platform code, if no dts***************/
int platform_get_dsi_par(struct owl_dsi *dsi_par)
{
	debug("platform dsi: pltfm get dsi par\n");

	dsi_par->mode = &owl_mode_800_480;
	return 0;
}

int platform_enable_dsi(void)
{
	u32 tmp;

	debug("platform dsi: pltfm enable dsi\n");

	tmp = readl(PAD_CTL);
	tmp |= 0x2;
	writel(tmp, PAD_CTL);

	enable_lcdc();
	return 0;
}


