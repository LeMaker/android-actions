/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Author: Donghwa Lee <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _PWM_BACKLIGHT_H_
#define _PWM_BACKLIGHT_H_

#include <asm/arch/pwm.h>

struct pwm_backlight_data {
	/*read only*/
	int max_brightness;

	/*can be updated*/
	int brightness;
	/*0: power off, 1: power on*/
	int power;
};

#ifdef CONFIG_OWL_PWM_BACKLIGHT
/*param pd: pwm backlight state the func fills for caller*/
extern int owl_pwm_backlight_init(struct pwm_backlight_data *pd);

extern int owl_pwm_backlight_update_status(struct pwm_backlight_data *pd);
#else
static inline int owl_pwm_backlight_init(struct pwm_backlight_data *pd)
{
	return -1;
}

static inline int owl_pwm_backlight_update_status(
		struct pwm_backlight_data *pd)
{
	return -1;
}
#endif

#endif /* _PWM_BACKLIGHT_H_ */
