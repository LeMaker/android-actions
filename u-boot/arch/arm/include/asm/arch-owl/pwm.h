#ifndef __ASM_OWL_PWM_H
#define __ASM_OWL_PWM_H

enum pwm_polarity {
	PWM_POLARITY_NORMAL,
	PWM_POLARITY_INVERSED,
};

/* This is a single PWM channel */
struct pwm_device {
	unsigned int hwpwm;
	unsigned int period;
	enum pwm_polarity polarity;
};

#define PWM_NUM_CHANNELS	6

extern int pwm_enable(int pwm_id);
extern void pwm_disable(int pwm_id);
extern int pwm_config(int pwm_id, int duty_ns,
	int period_ns, enum pwm_polarity polarity);

/**
 * Request a pwm channel as referenced by a device tree node.
 *
 * This channel can then be passed to pwm_enable().
 *
 * @param blob		Device tree blob
 * @param node		Node containing reference to pwm
 * @param prop_name	Property name of pwm reference
 * @param pwm		pwm propertys the func fills for caller
 * @return 0 if ok, else -1
 */
extern int fdtdec_pwm_get(const void *blob, int node,
		const char *prop_name, struct pwm_device *pwm);

/**
 * Set up the pwm controller, by looking it up in the fdt.
 *
 * @return 0 if ok, -1 if the device tree node was not found or invalid.
 */
int pwm_init(const void *blob);

#endif	/* __ASM_OWL_PWM_H */

