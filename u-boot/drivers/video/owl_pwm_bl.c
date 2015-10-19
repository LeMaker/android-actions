/*
 * PWM BACKLIGHT driver for Board based on OWL.
 *
 */
#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#include <common.h>
#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pwm.h>
#include <asm/arch/pwm_backlight.h>
#include <asm/arch/sps.h>

DECLARE_GLOBAL_DATA_PTR;

struct owl_pwm_bl {
	struct pwm_device pwm;
	int total_steps;
	int min_brightness;
	int max_brightness;
	int dft_brightness;

	int brightness;

	struct owl_fdt_gpio_state en_gpio;

	int node;
} pwm_bl;

static int pwm_activate_gpio(struct owl_fdt_gpio_state *gpio)
{
	int active_level;

	active_level = (gpio->flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	owl_gpio_generic_direction_output(gpio->chip, gpio->gpio, active_level);
	return 0;
}

static int pwm_deactivate_gpio(struct owl_fdt_gpio_state *gpio)
{
	int active_level;

	active_level = (gpio->flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	owl_gpio_generic_direction_output(gpio->chip, gpio->gpio, !active_level);
	return 0;
}


int owl_pwm_backlight_update_status(struct pwm_backlight_data *pd)
{
	int brightness = pd->brightness;
	int total_steps = pwm_bl.total_steps;

	struct pwm_device *pwm;

	pwm = &pwm_bl.pwm;
	if (pd->power == 0)
		brightness = 0;

	if (brightness > 0)
		brightness += pwm_bl.min_brightness;

	if (brightness > total_steps)
		return -1;

	if (brightness == 0) {
		pwm_deactivate_gpio(&pwm_bl.en_gpio);
		pwm_config(
			pwm->hwpwm, pwm->period, pwm->period, !pwm->polarity);
		mdelay(10);
		pwm_disable(pwm->hwpwm);
	} else {
		pwm_config(
			pwm->hwpwm, brightness * pwm->period / total_steps,
			pwm->period, pwm->polarity);
		pwm_enable(pwm->hwpwm);
		mdelay(10);
		pwm_activate_gpio(&pwm_bl.en_gpio);
	}

	return 0;
}

/*
	if use gpiod28-31, set 1 to cpu_pwr_ctl bit 7-4 first
	GPIOD 28  -------  CPU_PWR_CTL BIT 7    gpio  124
	GPIOD 29  -------  CPU_PWR_CTL BIT 6	gpio  125
	GPIOD 30  -------  CPU_PWR_CTL BIT 5	gpio  126
	GPIOD 31  -------  CPU_PWR_CTL BIT 4	gpio  127
*/
#define MIN_GPIO_TO_PWR  124    
#define MAX_GPIO_TO_PWR  127	
static int gpio_bit_to_pwr(struct owl_fdt_gpio_state *gpio)
{
	int tmp = 0;
	if(gpio->name){
		if((gpio->gpio>=MIN_GPIO_TO_PWR)&&(gpio->gpio<=MAX_GPIO_TO_PWR)){
			tmp |= (1<<(MAX_GPIO_TO_PWR-gpio->gpio));
			tmp <<= 4;
		}	
		printf("get_pwr_ctl %s  %d\n", gpio->name, tmp);		
	}
	return tmp;
}


int owl_pwm_backlight_init(struct pwm_backlight_data *pd)
{
	int devnode,tmp,reg_val;

	devnode = fdtdec_next_compatible(gd->fdt_blob, 0,
		COMPAT_ACTIONS_OWL_PWM_BACKLIGHT);
	if (devnode < 0) {
		debug("%s: Cannot find device tree node\n", __func__);
		return -1;
	}

	pwm_bl.node = devnode;

	if (fdtdec_pwm_get(gd->fdt_blob, devnode, "pwms", &pwm_bl.pwm))
		return -1;

	pwm_bl.total_steps =
		fdtdec_get_int(gd->fdt_blob, devnode, "total_steps", -1);
	pwm_bl.min_brightness =
		fdtdec_get_int(gd->fdt_blob, devnode, "min_brightness", -1);
	pwm_bl.max_brightness =
		fdtdec_get_int(gd->fdt_blob, devnode, "max_brightness", -1);
	pwm_bl.dft_brightness =
		fdtdec_get_int(gd->fdt_blob, devnode, "dft_brightness", -1);

	owl_fdtdec_decode_gpio(
		gd->fdt_blob, devnode, "backlight_en_gpios", &pwm_bl.en_gpio);
		
		tmp = gpio_bit_to_pwr(&pwm_bl.en_gpio);
		
		if(tmp){		
		reg_val = readl(CMU_PWR_CTL);
		reg_val |= tmp;
		writel(reg_val, CMU_PWR_CTL);		
	}

	pd->power = 0;
	pd->max_brightness = pwm_bl.max_brightness - pwm_bl.min_brightness;
	pd->brightness = pwm_bl.dft_brightness - pwm_bl.min_brightness;

	return 0;
}
