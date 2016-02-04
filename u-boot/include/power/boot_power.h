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


#ifndef __BOOT_POWER_H_
#define __BOOT_POWER_H_


#define PMU_SYS_CTL1_EN_S1              		(1 << 0)
#define PMU_SYS_CTL3_EN_S3       				(1 << 14)
#define PMU_SYS_CTL3_EN_S2            			(1 << 15)

//
#define     PMU_CHARGER_CTL1_BAT_EXIST_EN		(1 << 5)
#define     PMU_CHARGER_CTL1_BAT_EXIST			(1 << 10)

enum CHARGE_PLUG_STATUS
{
	NO_PLUG = 0,
	WALL_PLUG,
	USB_PLUG
};
enum SUPPORT_ADAPTER_TYPE
{
	UNKNOWN = 0,
	DCIN,
	USB,
	DCIN_USB,
};

int is_battery_exist(void);
void get_bat_capacity(int *cap, int bat_mv);
int get_bat_voltage(int *vol);
void get_cfg_items(void);
int get_adaptor_type(int *type);
int get_charge_plugin_status(int* wall_mv, int* vbus_mv);
void atc260x_shutoff(void);

void check_power(void);
#endif

