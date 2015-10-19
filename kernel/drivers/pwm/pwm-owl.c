/* arch/arm/plat-s3c24xx/pwm.c
 *
 * Copyright (c) 2007 Ben Dooks
 * Copyright (c) 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>, <ben-linux@fluff.org>
 *
 * S3C24XX PWM device core
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_device.h>

#include <mach/hardware.h>
#include <mach/module-owl.h>
#include <mach/clkname.h>
#include <mach/pwm-owl.h>

#define MAX_NUM_PWM 6

struct pwm_chip_data {
	int chip_type;	//0-atm5206tc&atm5203; 1-atm5206
	int pwm_num;	//atm5206tc&atm5203 == 4; 1-atm5206 == 6
};

struct gl520x_pwm_dev {
	struct clk *clk;
	u32 module_id;

	unsigned int required_period_ns;
	unsigned int period_ns;
	unsigned int duty_ns;
	unsigned int counter_steps;

	struct device dev;
	struct pinctrl *pinctrl;
};

struct gl520x_pwm_chip {
	struct platform_device *pdev;

	struct pwm_chip chip;
	const struct pwm_chip_data *chip_data;
	struct gl520x_pwm_dev pwm_dev[MAX_NUM_PWM];
};

static const struct pwm_chip_data *g_chip_data;

unsigned int pwm_ctl_reg_array[] = {
	PWM_CTL0, PWM_CTL1, PWM_CTL2, PWM_CTL3, PWM_CTL4, PWM_CTL5
};

#if 0
#define PWM_PRINT(fmt, args...) printk(KERN_INFO fmt, ##args)
#else
#define PWM_PRINT(fmt, args...)
#endif

#define NS_IN_HZ (1000000000UL)

/********************************/
static inline struct gl520x_pwm_chip *to_gl520x_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct gl520x_pwm_chip, chip);
}

static inline int get_pwm_clk_info(struct pwm_chip *chip,
	int hwpwm, struct clk **clk, u32 *module_id)
{
	const char *clk_name;

	switch (hwpwm) {
	case 0:
		clk_name = CLKNAME_PWM0_CLK;
		*module_id = MODULE_CLK_PWM0;
		break;

	case 1:
		clk_name = CLKNAME_PWM1_CLK;
		*module_id = MODULE_CLK_PWM1;
		break;

	case 2:
		clk_name = CLKNAME_PWM2_CLK;
		*module_id = MODULE_CLK_PWM2;
		break;

	case 3:
		clk_name = CLKNAME_PWM3_CLK;
		*module_id = MODULE_CLK_PWM3;
		break;

	case 4:
		clk_name = CLKNAME_PWM4_CLK;
		*module_id = MODULE_CLK_PWM4;
		break;

	case 5:
		clk_name = CLKNAME_PWM5_CLK;
		*module_id = MODULE_CLK_PWM5;
		break;

	default:
			printk(KERN_ALERT "bad pwm id %d\n", hwpwm);
			return -EINVAL;
	}

	*clk = clk_get(chip->dev, clk_name);
	return 0;
}

static int gl520x_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			    int duty_ns, int period_ns)
{
	unsigned int tmp = 0, val, valc;
	s32 rate = 0;
	struct clk *parent_clk;
	struct gl520x_pwm_chip *pc = to_gl520x_pwm_chip(chip);
	struct gl520x_pwm_dev *pwm_dev;
	int hwpwm;
	uint counter_steps, comparator_steps, real_period_ns;
	u32 pwm_ctl_reg;

	/* We currently avoid using 64bit arithmetic by using the
	 * fact that anything faster than 1Hz is easily representable
	 * by 32bits. */

	if (period_ns > NS_IN_HZ || duty_ns > NS_IN_HZ)
		return -ERANGE;

	hwpwm = pwm->hwpwm;

	pwm_dev = &pc->pwm_dev[hwpwm];

	if (period_ns != pwm_dev->required_period_ns) {
		uint parent_clk_freq, whole_div, pre_scale;
		uint rq_pwm_src_clk, real_pwm_src_clk;

		pwm_dev->required_period_ns = period_ns;

		rate = NS_IN_HZ / period_ns; /* PWM output freq in Hz */
		if (rate < 512) {
			parent_clk = clk_get(NULL, CLKNAME_IC_32K);
		} else if (rate <= 375000) {
			/* 为了保证分辨率, 软件限定分母最小为64, 故PWM频率最大为 24MHz/64 */
			parent_clk = clk_get(NULL, CLKNAME_HOSC);
		} else {
			rate = 375000;
			parent_clk = clk_get(NULL, CLKNAME_HOSC);
			PWM_PRINT("pwm freq will be 375kHZ at most!\n");
		}
		parent_clk_freq = clk_get_rate(parent_clk);

		if(pc->chip_data->chip_type == 1) {
			/* 5206 */
			whole_div = (parent_clk_freq + rate / 2U) / rate;
			pre_scale = (whole_div + 1023U) / 1024U;
			counter_steps = (whole_div + pre_scale - 1) / pre_scale; /* 取上整. */
			if (counter_steps > 1024U)
				counter_steps = 1024U;
			rq_pwm_src_clk = parent_clk_freq / pre_scale; /* 必须取下整. */
		} else {
			/* 5203 */
			rq_pwm_src_clk = rate * 64U;
			counter_steps = 64U;
		}
		pwm_dev->counter_steps = counter_steps;

		/* setup clock source */
		clk_set_parent(pwm_dev->clk, parent_clk);
		clk_set_rate(pwm_dev->clk, rq_pwm_src_clk);
		real_pwm_src_clk = clk_get_rate(pwm_dev->clk);

		real_period_ns = div_u64((u64)counter_steps * NS_IN_HZ, real_pwm_src_clk);
		pwm_dev->period_ns = real_period_ns;

		dev_dbg(&(pwm_dev->dev), "set period, rq_period_ns=%d parent_clk=%u "
			"rq_pwm_src_clk=%u real_pwm_src_clk=%u counter_steps=%u "
			"real_period_ns=%u\n",
			period_ns, parent_clk_freq, rq_pwm_src_clk, real_pwm_src_clk,
			counter_steps, real_period_ns);
	} else {
		counter_steps = pwm_dev->counter_steps;
	}

	/* 比例要用duty和period申请值来算, 不能用真实值来算.
	 * 注意用4舍5入来消除舍入误差,
	 * 不这样做的话, pwm-dcdc那边的和这边合起来的step到duty再到step的转换的舍入误差足以造成跳档. */
	comparator_steps = div_u64((u64)duty_ns * counter_steps + period_ns / 2U, period_ns);
	if (comparator_steps > counter_steps)
		comparator_steps = counter_steps;
	pwm_dev->duty_ns = div_u64(pwm_dev->period_ns * (u64)comparator_steps, counter_steps);

	dev_dbg(&(pwm_dev->dev), "set duty, rq_duty_ns=%d rq_period_ns=%d "
		"counter_steps=%u comparator_steps=%u\n",
		duty_ns, period_ns, counter_steps, comparator_steps);

	pwm_ctl_reg = pwm_ctl_reg_array[hwpwm];
	tmp = act_readl(pwm_ctl_reg);

	if(pc->chip_data->chip_type == 1) {
		val = (comparator_steps != 0) ? (comparator_steps - 1) : 0; /* to reg value */
		valc = (counter_steps != 0) ? (counter_steps - 1) : 0;
		tmp &= ~((1U << 20) -1U);
		tmp |= (val << 10) | valc;
	} else {
		val = (comparator_steps != 0) ? comparator_steps - 1 : 0; /* to reg value */
		tmp &= (~0x3f);
		tmp |= (val & 0x3f);
	}
	act_writel(tmp, pwm_ctl_reg);

	return 0;
}

int owl_pwm_get_duty_cfg(struct pwm_device *pwm, uint *comparator_steps, uint *counter_steps)
{
	uint hwpwm;
	u32 pwm_ctl_reg, reg_val;

	if (pwm == NULL || comparator_steps == NULL || counter_steps == NULL)
		return -EINVAL;

	hwpwm = pwm->hwpwm;
	pwm_ctl_reg = pwm_ctl_reg_array[hwpwm];

	reg_val = act_readl(pwm_ctl_reg);
	if(g_chip_data->chip_type == 1) {
		*comparator_steps = ((reg_val >> 10) & ((1U << 10) -1U)) + 1U;
		*counter_steps = (reg_val & ((1U << 10) -1U)) + 1U;
	} else {
		*comparator_steps = (reg_val & 0x3f) + 1U;
		*counter_steps = 64;
	}
	return 0;
}
EXPORT_SYMBOL(owl_pwm_get_duty_cfg);

enum pwm_polarity owl_pwm_get_polarity(struct pwm_device *pwm)
{
	uint hwpwm;
	u32 pwm_ctl_reg;
	u32 val;

	PWM_PRINT("%s\n", __func__);

	hwpwm = pwm->hwpwm;

	pwm_ctl_reg = pwm_ctl_reg_array[hwpwm];

	val = act_readl(pwm_ctl_reg);
	if(g_chip_data->chip_type == 1) {
		if (val & (1U << 20))
			return PWM_POLARITY_NORMAL;
		else
			return PWM_POLARITY_INVERSED;
	} else {
		if (val & (0x1 << 8))
			return PWM_POLARITY_NORMAL;
		else
			return PWM_POLARITY_INVERSED;
	}
}
EXPORT_SYMBOL(owl_pwm_get_polarity);

static int gl520x_pwm_set_polarity(
		struct pwm_chip *chip, struct pwm_device *pwm,
		enum pwm_polarity polarity)
{
	int hwpwm;
	u32 pwm_ctl_reg;
	struct gl520x_pwm_chip *pc = to_gl520x_pwm_chip(chip);

	PWM_PRINT("%s\n", __func__);

	hwpwm = pwm->hwpwm;

	pwm_ctl_reg = pwm_ctl_reg_array[hwpwm];

	if(pc->chip_data->chip_type == 1) {
		if (polarity == PWM_POLARITY_INVERSED) {
			PWM_PRINT("pwm inverse\n");
			act_clearl(1U << 20, pwm_ctl_reg);
		} else {
			PWM_PRINT("pwm not inverse\n");
			/* Duty cycle defines HIGH period of PWM */
			act_setl(1U << 20, pwm_ctl_reg);
		}
	} else {
		if (polarity == PWM_POLARITY_INVERSED) {
			PWM_PRINT("pwm inverse\n");
			act_clearl(0x1 << 8, pwm_ctl_reg);
		} else {
			PWM_PRINT("pwm not inverse\n");
			/* Duty cycle defines HIGH period of PWM */
			act_setl(0x1 << 8, pwm_ctl_reg);
		}
	}
	PWM_PRINT("%s END\n", __func__);

	return 0;
}

static int gl520x_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct gl520x_pwm_chip *pc = to_gl520x_pwm_chip(chip);
	int ret = 0;
	int hwpwm;

	PWM_PRINT("%s\n", __func__);

	hwpwm = pwm->hwpwm;

	module_clk_enable(pc->pwm_dev[hwpwm].module_id);
	ret = clk_prepare_enable(pc->pwm_dev[hwpwm].clk);
	if (ret < 0)
		return ret;

	PWM_PRINT("%s END\n", __func__);

	return 0;
}

static void gl520x_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct gl520x_pwm_chip *pc = to_gl520x_pwm_chip(chip);
	int hwpwm;

	PWM_PRINT("%s\n", __func__);

	hwpwm = pwm->hwpwm;

	clk_disable_unprepare(pc->pwm_dev[hwpwm].clk);
	module_clk_disable(pc->pwm_dev[hwpwm].module_id);

	PWM_PRINT("%s END\n", __func__);

	return ;
}

static int gl520x_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct gl520x_pwm_chip *pc = to_gl520x_pwm_chip(chip);
	int ret = 0;
	int hwpwm;
	struct gl520x_pwm_dev *pwm_dev;

	PWM_PRINT("%s\n", __func__);

	hwpwm = pwm->hwpwm;
	pwm_dev = &pc->pwm_dev[hwpwm];

	PWM_PRINT("%s REQ ID = %d\n", __func__, hwpwm);

	ret = get_pwm_clk_info(chip, hwpwm, &pwm_dev->clk, &pwm_dev->module_id);
	if (ret)
		return ret;

	PWM_PRINT("%s 2\n", __func__);

	pwm_dev->pinctrl = pinctrl_get_select_default(&pwm_dev->dev);
	if (IS_ERR(pwm_dev->pinctrl))
		return PTR_ERR(pwm_dev->pinctrl);

	PWM_PRINT("%s END\n", __func__);

	return 0;
}

static void gl520x_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct gl520x_pwm_chip *pc = to_gl520x_pwm_chip(chip);
	int hwpwm;
	struct gl520x_pwm_dev *pwm_dev;

	PWM_PRINT("%s\n", __func__);

	hwpwm = pwm->hwpwm;
	pwm_dev = &pc->pwm_dev[hwpwm];

	if (test_bit(PWMF_ENABLED, &pwm->flags))
		gl520x_pwm_disable(chip, pwm);

	clk_put(pwm_dev->clk);

	pinctrl_put(pwm_dev->pinctrl);

	PWM_PRINT("%s END\n", __func__);

	return ;

}

static const struct pwm_ops gl520x_pwm_ops = {
	.request = gl520x_pwm_request,
	.free = gl520x_pwm_free,
	.config = gl520x_pwm_config,
	.set_polarity = gl520x_pwm_set_polarity,
	.enable = gl520x_pwm_enable,
	.disable = gl520x_pwm_disable,
	.owner = THIS_MODULE,
};

static int gl520x_of_pwm_devs_init(struct platform_device *pdev)
{
	struct device_node *child;
	int hwpwm;
	struct gl520x_pwm_chip *pc = platform_get_drvdata(pdev);
	struct gl520x_pwm_dev *pwm_dev;

	PWM_PRINT("%s\n", __func__);

	for_each_child_of_node(pdev->dev.of_node, child) {
		if (of_property_read_u32(child, "id", &hwpwm))
			return -EINVAL;

		PWM_PRINT("%s, child id = %d\n", __func__, hwpwm);

		if (hwpwm >= pc->chip_data->pwm_num)
			return -EINVAL;

		pwm_dev = &pc->pwm_dev[hwpwm];
		device_initialize(&pwm_dev->dev);
		dev_set_name(&pwm_dev->dev, "gl5206-pwm%d", hwpwm);
		pwm_dev->dev.of_node = child;
	}
	PWM_PRINT("%s OK\n", __func__);
	return 0;
}

static struct pwm_chip_data pwm_chip_data[] = {
	{ .chip_type = 0, .pwm_num = 4 }, //atm5206tc&atm5203
	{ .chip_type = 1, .pwm_num = 6 }, //atm5206
};

static struct of_device_id gl520x_pwm_of_match[] = {
	{ .compatible = "actions,atm7039c-pwm", .data = &pwm_chip_data[0] },
	{ .compatible = "actions,atm7059a-pwm", .data = &pwm_chip_data[1] },
	{ }
};

static int gl520x_pwm_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	struct device *dev = &pdev->dev;
	struct gl520x_pwm_chip *pwm;
	int ret;

	dev_info(&pdev->dev, "Probing...\n");

	pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
	if (!pwm) {
		dev_err(dev, "failed to allocate pwm_device\n");
		return -ENOMEM;
	}

	pwm->pdev = pdev;

	of_id = of_match_device(gl520x_pwm_of_match, &pdev->dev);
	if (of_id)
		pwm->chip_data = of_id->data;
	else
		return -EINVAL;
	
	g_chip_data = pwm->chip_data;
	
	platform_set_drvdata(pdev, pwm);

	pwm->chip.dev = &pdev->dev;
	pwm->chip.ops = &gl520x_pwm_ops;
	pwm->chip.of_xlate = of_pwm_xlate_with_flags;
	pwm->chip.of_pwm_n_cells = 3;
	pwm->chip.base = -1;
	pwm->chip.npwm = pwm->chip_data->pwm_num;

	if (gl520x_of_pwm_devs_init(pdev))
		return -EINVAL;

	ret = pwmchip_add(&pwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static int gl520x_pwm_remove(struct platform_device *pdev)
{
	struct gl520x_pwm_chip *pc = platform_get_drvdata(pdev);

	if (WARN_ON(!pc))
		return -ENODEV;

	return pwmchip_remove(&pc->chip);
}

static struct platform_driver gl5201_pwm_driver = {
	.driver = {
		.name = "pwm-owl",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(gl520x_pwm_of_match),
	},
	.probe = gl520x_pwm_probe,
	.remove = gl520x_pwm_remove,
};

static int __init pwm_init(void)
{
	PWM_PRINT("platform register pwm driver: %s\n",
		gl5201_pwm_driver.driver.name);
	return platform_driver_register(&gl5201_pwm_driver);
}

arch_initcall(pwm_init);
