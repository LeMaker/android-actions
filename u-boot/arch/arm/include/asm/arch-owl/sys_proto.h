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

#ifndef __OWL_SYS_PROTO_H
#define __OWL_SYS_PROTO_H

extern int pinctrl_init_r(void);
extern int owl_gpio_init(void);
extern void check_recovery_mode(void);
extern void setup_recovery_env(void);
extern int get_boot_dev_num(void);
extern int owl_get_displaypll_rate(void);
extern int owl_get_devpll_rate(void);
extern int splash_image_init(void);
extern int owl_dss_enable(void);
extern int owl_get_nic_clk_rate(void);

#endif
