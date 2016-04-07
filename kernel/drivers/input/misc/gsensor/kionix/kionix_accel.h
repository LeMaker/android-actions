/* include/linux/input/kionix_accel.h - Kionix accelerometer driver
 *
 * Copyright (C) 2012 Kionix, Inc.
 * Written by Kuching Tan <kuchingtan@kionix.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __KIONIX_ACCEL_H__
#define __KIONIX_ACCEL_H__

#define KIONIX_ACCEL_I2C_ADDR		0x0E
#define KIONIX_ACCEL_NAME			"kionix_accel"
#define KIONIX_ACCEL_IRQ			"kionix-irq"
#define KIONIX_ACCEL_RES_12BIT	0
#define KIONIX_ACCEL_RES_8BIT	1
#define KIONIX_ACCEL_RES_6BIT	2
#define KIONIX_ACCEL_G_2G		0
#define KIONIX_ACCEL_G_4G		1
#define KIONIX_ACCEL_G_6G		2
#define KIONIX_ACCEL_G_8G		3
#endif  /* __KIONIX_ACCEL_H__ */
