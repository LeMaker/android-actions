/* For ACTIONS android platform.
 *
 * mir3da.h - Linux kernel modules for 3-Axis Accelerometer
 *
 * Copyright (C) 2011-2013 MiraMEMS Sensing Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MIR3DA_STANDARD_H__
#define __MIR3DA_STANDARD_H__
	 
#include <linux/ioctl.h>

#define MIR3DA_I2C_ADDR		                    0x27

#define DRI_VER                  		        "1.0"
#define MIR3DA_DRV_NAME                 			"mir3da"
#define MIR3DA_INPUT_DEV_NAME      			MIR3DA_DRV_NAME
#define MIR3DA_MISC_NAME                			MIR3DA_DRV_NAME


#define DELAY_INTERVAL_MAX              			200
#define INPUT_FUZZ                      				0
#define INPUT_FLAT                      				0
#define LSG                                  				1024

#define CFG_GSENSOR_USE_CONFIG   			1


#endif /* !__MIR3DA_STANDARD_H__ */


