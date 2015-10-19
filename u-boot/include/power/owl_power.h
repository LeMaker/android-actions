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


#ifndef __OWL_POWER_H_
#define __OWL_POWER_H_

#define WAKEUP_SRC_RESET			(1 << 1)
#define WAKEUP_SRC_HDSW			(1 << 2)
#define WAKEUP_SRC_ALARM			(1 << 3)
#define WAKEUP_SRC_ONOFF_SHORT		(1 << 7)
#define WAKEUP_SRC_ONOFF_LONG		(1 << 8)
#define WAKEUP_SRC_WALL_IN			(1 << 9)
#define WAKEUP_SRC_VBUS_IN			(1 << 10)
#define PMU_SYS_CTL1_EN_S1              					     (1 << 0)
#define PMU_SYS_CTL3_EN_S3       					             (1 << 14)
#define PMU_SYS_CTL3_EN_S2            					     (1 << 15)
#define PMU_SYS_CTL3_FW_FLAG_S2					     (1 << 4)
#define PMU_SYS_CTL3_FW_FLAG_S2					     (1 << 4)
#define PMU_DC2_CTL0_DC2_EN					         (1 << 15)
enum {
	BOOT_IMAGE_NO_PICTURE = 1,
	BOOT_IMAGE_BATTERY_LOW,
	BOOT_IMAGE_NORMAL,
	BOOT_IMAGE_RECOVERY,
};


struct owl_regulator_ops {
	int (*enable)(int id);
	int (*disable)(int id);
	int (*set_voltage)(int id, unsigned voltage_mv);
};

struct owl_regulator {
	int id;
	int dev_node;
	struct owl_regulator_ops *ops;
};


extern int owl_regulator_enable(struct owl_regulator *regulator);
extern int owl_regulator_disable(struct owl_regulator *regulator);
extern struct owl_regulator *fdtdec_owl_regulator_get(const void *blob, int node,
		const char *prop_name);

#endif
