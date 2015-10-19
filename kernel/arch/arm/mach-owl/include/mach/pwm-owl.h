#ifndef __PWM_OWL_H__
#define __PWM_OWL_H__

/*=================================================================*/
/* rgb lcd stuff */
#include <linux/delay.h>
#include <linux/pwm.h>

#include <mach/hardware.h>

enum pwm_polarity owl_pwm_get_polarity(struct pwm_device *pwm);


#endif
