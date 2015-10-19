/*
 * atc260x_onoff.c  --  On/Off key driver for ATC260X
 *
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/mfd/atc260x/atc260x.h>

#include <mach/power.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>

extern int owl_pm_wakeup_flag(void);



struct atc260x_onoff_dev;
typedef int (*atc260x_onoff_init_func_t)(struct atc260x_onoff_dev *atc260x_onoff);

struct atc260x_onoff_regdef {
	atc260x_onoff_init_func_t init_func;
	atc260x_onoff_init_func_t exit_func;
	u32     reg_int_ctl;
	u32     reg_int_pnd;
	u32     int_reg_w1c_bm;  /* 一些"写1清0"功能位的位码. 用于应付不同的写副作用寄存器位混编的brain-damaged设计. */
	u32     pnd_reg_w1c_bm;  /* 一些"写1清0"功能位的位码. */
	u32     kdwn_state_bitm;
	u32     long_int_pnd_bitm;
	u32     short_int_pnd_bitm;
	u32     kdwn_int_pnd_bitm;
	u32     press_int_en_bitm;
	u32     kdwn_int_en_bitm;
	u32     reg_reset_pnd;
	u32     reset_pnd_bitm;
};

/* On/Off module structure */
struct atc260x_onoff_dev {
	struct device *dev;
	struct input_dev *idev;
	struct atc260x_dev *atc260x;
	const struct atc260x_onoff_regdef *regdef;
	struct delayed_work work;
	uint irq;
	uint key_trigger;
	uint suspend_state;
};



/* for atc2603a ------------------------------------------------------------- */

static int _atc2603a_onoff_init(struct atc260x_onoff_dev *atc260x_onoff)
{
	uint onoff_time = 0; /* more quick response of the on/off key */
	return atc260x_reg_setbits(atc260x_onoff->atc260x, ATC2603A_PMU_SYS_CTL2,
		(1U << 12) | (3U << 10) | (1<<14)|(1<<13),
		(1U << 12) | (onoff_time << 10) | (1<<14)|(1<<13));
	/* Don't touch the INTS_MSK register here, it is own by regmap now.
	 * Register interrupt also enable the source in INTS_MSK register. */
}
static int _atc2603a_onoff_exit(struct atc260x_onoff_dev *atc260x_onoff)
{
	/* set long_key to 2s */
	return atc260x_reg_setbits(atc260x_onoff->atc260x, ATC2603A_PMU_SYS_CTL2,
		(3U << 10), (2 << 10));
}
static const struct atc260x_onoff_regdef sc_atc2603a_onoff_regdef = {
	.init_func          = _atc2603a_onoff_init,
	.exit_func          = _atc2603a_onoff_exit,
	.reg_int_ctl        = ATC2603A_PMU_SYS_CTL2,
	.reg_int_pnd        = ATC2603A_PMU_SYS_CTL2,
	.int_reg_w1c_bm     = (1<<14)|(1<<13),
	.pnd_reg_w1c_bm     = (1<<14)|(1<<13),
	.kdwn_state_bitm    = (1 << 15),
	.long_int_pnd_bitm  = (1 << 13),
	.short_int_pnd_bitm = (1 << 14),
	.kdwn_int_pnd_bitm  = 0,
	.press_int_en_bitm  = (1 << 12),
	.kdwn_int_en_bitm   = 0,
	.reg_reset_pnd		= 0,
	.reset_pnd_bitm		= 0,
};


/* for atc2603c ------------------------------------------------------------- */

static int _atc2603c_onoff_init(struct atc260x_onoff_dev *atc260x_onoff)
{
	uint onoff_time = 2; /* <2s for short_key, >2s for long_key */
	return atc260x_reg_setbits(atc260x_onoff->atc260x, ATC2603A_PMU_SYS_CTL2,
		(1U << 12) | (1 << 1) | (3U << 10) | (1<<14)|(1<<13)|(1<<2),
		(1U << 12) | (1 << 1) | (onoff_time << 10) | (1<<14)|(1<<13)|(1<<2));
	/* Don't touch the INTS_MSK register here, it is own by regmap now.
	 * Register interrupt also enable the source in INTS_MSK register. */
}
static const struct atc260x_onoff_regdef sc_atc2603c_onoff_regdef = {
	.init_func          = _atc2603c_onoff_init,
	.reg_int_ctl        = ATC2603A_PMU_SYS_CTL2,
	.reg_int_pnd        = ATC2603A_PMU_SYS_CTL2,
	.int_reg_w1c_bm     = (1<<14)|(1<<13)|(1<<2),
	.pnd_reg_w1c_bm     = (1<<14)|(1<<13)|(1<<2),
	.kdwn_state_bitm    = (1 << 15),
	.long_int_pnd_bitm  = (1 << 13),
	.short_int_pnd_bitm = (1 << 14),
	.kdwn_int_pnd_bitm  = (1 << 2),
	.press_int_en_bitm  = (1 << 12),
	.kdwn_int_en_bitm   = (1 << 1),
	.reg_reset_pnd		= ATC2603A_PMU_SYS_CTL1,
	.reset_pnd_bitm		= (1 << 10),
};


/* for atc2609a ------------------------------------------------------------- */

static int _atc2609a_onoff_init(struct atc260x_onoff_dev *atc260x_onoff)
{
	uint onoff_time = 2; /* <2s for short_key, >2s for long_key */
	return atc260x_reg_setbits(atc260x_onoff->atc260x, ATC2603A_PMU_SYS_CTL2,
		(1U << 12) | (1 << 1) | (3U << 10) | (1<<14)|(1<<13)|(1<<2),
		(1U << 12) | (1 << 1) | (onoff_time << 10) | (1<<14)|(1<<13)|(1<<2));
	/* Don't touch the INTS_MSK register here, it is own by regmap now.
	 * Register interrupt also enable the source in INTS_MSK register. */
}
static const struct atc260x_onoff_regdef sc_atc2609a_onoff_regdef = {
	.init_func          = _atc2609a_onoff_init,
	.reg_int_ctl        = ATC2603A_PMU_SYS_CTL2,
	.reg_int_pnd        = ATC2603A_PMU_SYS_CTL2,
	.int_reg_w1c_bm     = (1<<14)|(1<<13)|(1<<2),
	.pnd_reg_w1c_bm     = (1<<14)|(1<<13)|(1<<2),
	.kdwn_state_bitm    = (1 << 15),
	.long_int_pnd_bitm  = (1 << 13),
	.short_int_pnd_bitm = (1 << 14),
	.kdwn_int_pnd_bitm  = (1 << 2),
	.press_int_en_bitm  = (1 << 12),
	.kdwn_int_en_bitm   = (1 << 1),
	.reg_reset_pnd		= 0,
	.reset_pnd_bitm		= 0,
};

static const struct atc260x_onoff_regdef * const sc_atc260x_onoff_regdef_tbl[ATC260X_ICTYPE_CNT] = {
	[ATC260X_ICTYPE_2603A] = &sc_atc2603a_onoff_regdef,
	[ATC260X_ICTYPE_2603C] = &sc_atc2603c_onoff_regdef,
	[ATC260X_ICTYPE_2609A] = &sc_atc2609a_onoff_regdef,
};


/* Read-Modify-Write operation */
static void _atc260x_onoff_rmw_intreg(struct atc260x_dev *atc260x,
	const struct atc260x_onoff_regdef *regdef, uint mask, uint value)
{
	int ret;
	ret = atc260x_reg_wp_setbits(atc260x, regdef->reg_int_ctl,
		regdef->int_reg_w1c_bm, mask, value);
	if (ret) {
		pr_err("%s() failed to r/w onoff int reg\n", __func__);
	}
}

/* Clear-Pend operation */
static void _atc260x_onoff_w1clr_pndreg(struct atc260x_dev *atc260x,
	const struct atc260x_onoff_regdef *regdef, uint mask)
{
	int ret;
	ret = atc260x_reg_wp_clrpnd(atc260x, regdef->reg_int_pnd,
		regdef->pnd_reg_w1c_bm, mask);
	if (ret) {
		pr_err("%s() failed to w onoff pnd reg\n", __func__);
	}
}

/**
 * The chip gives us an interrupt when the ON/OFF pin is asserted but we
 * then need to poll to see when the pin is deasserted.
 */
static void atc260x_poll_onoff(struct work_struct *work)
{
	struct atc260x_onoff_dev *atc260x_onoff = container_of(work, struct atc260x_onoff_dev,
						   work.work);
	struct atc260x_dev *atc260x = atc260x_onoff->atc260x;
	const struct atc260x_onoff_regdef *regdef = atc260x_onoff->regdef;
	int poll, ret;

	ret = atc260x_reg_read(atc260x, regdef->reg_int_pnd);
	if (ret >= 0) {
		poll = atc260x_onoff->key_trigger || (ret & regdef->kdwn_state_bitm);
		atc260x_onoff->key_trigger = 0;
		dev_dbg(atc260x_onoff->dev, "On/Off key CTL2=%x, poll=%d\n", ret, poll);

		/* report key pressed */
		input_report_key(atc260x_onoff->idev, KEY_POWER, poll);
		input_sync(atc260x_onoff->idev);
	} else {
		dev_err(atc260x_onoff->dev, "Failed to read ON/OFF status: %d\n", ret);
		poll = 1;
	}

	/* if key is pressed, check key status after 200 ms */
	if (poll) {
		schedule_delayed_work(&atc260x_onoff->work, msecs_to_jiffies(200));
	} else {
		/* cleare On/Off press pending */
		_atc260x_onoff_w1clr_pndreg(atc260x, regdef,
			(regdef->long_int_pnd_bitm | regdef->short_int_pnd_bitm |
			regdef->kdwn_int_pnd_bitm));
		/* enable the On/Off IRQ */
		_atc260x_onoff_rmw_intreg(atc260x, regdef,
			(regdef->press_int_en_bitm | regdef->kdwn_int_en_bitm),
			(regdef->press_int_en_bitm | regdef->kdwn_int_en_bitm));
	}
}

static void _atc260x_onoff_trigger_key_process(struct atc260x_onoff_dev *atc260x_onoff, uint delay)
{
	struct atc260x_dev *atc260x = atc260x_onoff->atc260x;
	const struct atc260x_onoff_regdef *regdef = atc260x_onoff->regdef;

	/* disable On/Off interrupt, but not clear the On/Off IRQ pending bits */
	_atc260x_onoff_rmw_intreg(atc260x, regdef,
		(regdef->press_int_en_bitm | regdef->kdwn_int_en_bitm), 0);

	atc260x_onoff->key_trigger = 1;
	schedule_delayed_work(&atc260x_onoff->work, delay);
}

static void atc260x_onoff_irq_handle(struct atc260x_onoff_dev *atc260x_onoff);

/**
 * On/Off IRQ hander, run in kernel thread of ATC260X core IRQ dispatcher
 */
static irqreturn_t atc260x_onoff_irq(int irq, void *data)
{
	struct atc260x_onoff_dev *atc260x_onoff = data;

	atc260x_onoff_irq_handle(atc260x_onoff);
	/*dev_dbg(atc260x_onoff->dev, "on/off interrupt!\n"); */

	_atc260x_onoff_trigger_key_process(atc260x_onoff, 0);
	return IRQ_HANDLED;
}

static int atc260x_onoff_suspend(struct device *dev)
{
	struct atc260x_onoff_dev *atc260x_onoff = dev_get_drvdata(dev);
	struct atc260x_dev *atc260x = atc260x_onoff->atc260x;
	const struct atc260x_onoff_regdef *regdef = atc260x_onoff->regdef;
	dev_info(atc260x_onoff->dev, "%s() enter\n", __func__);

	/* disable On/Off interrupt */
	_atc260x_onoff_rmw_intreg(atc260x, regdef,
		(regdef->press_int_en_bitm | regdef->kdwn_int_en_bitm), 0);

	cancel_delayed_work_sync(&atc260x_onoff->work);

	atc260x_onoff->suspend_state = 1;
	/*wake up in 0.5s,not 2s*/
	atc260x_reg_setbits(atc260x,ATC2603C_PMU_SYS_CTL2,0x0c00,0);

	return 0;
}

static int atc260x_onoff_resume(struct device *dev)
{
	struct atc260x_onoff_dev *atc260x_onoff = dev_get_drvdata(dev);
	struct atc260x_dev *atc260x = atc260x_onoff->atc260x;
	
	int ret;

	dev_info(atc260x_onoff->dev, "%s() enter\n", __func__);

	ret = (atc260x_onoff->regdef->init_func)(atc260x_onoff);
	if (ret) {
		dev_info(atc260x_onoff->dev, "%s() hw re-init failed\n", __func__);
		return -EIO;
	}
	/*long time press back to 2s*/
	atc260x_reg_setbits(atc260x,ATC2603C_PMU_SYS_CTL2,0x0c00,(2 << 10));

	return 0;
}

static int atc260x_onoff_suspend_prepare(struct device *dev)
{
	struct atc260x_onoff_dev *atc260x_onoff = dev_get_drvdata(dev);
	dev_info(atc260x_onoff->dev, "%s() enter\n", __func__);
	atc260x_onoff->suspend_state = 0;
	return 0;
}

static void atc260x_onoff_resume_complete(struct device *dev)
{
	struct atc260x_onoff_dev *atc260x_onoff = dev_get_drvdata(dev);

	dev_info(atc260x_onoff->dev, "%s() enter\n", __func__);

	/* resend the long/short press key sequence, according to the active wakeup source. */
	if (atc260x_onoff->suspend_state != 0) {
		uint wakeup_flag = owl_pm_wakeup_flag();
		uint sim_wake_srcs = OWL_PMIC_WAKEUP_SRC_WALL_IN | OWL_PMIC_WAKEUP_SRC_VBUS_IN;
#if 1
		/* distinguish long/short press, resend the corresponding sequence. */
		if (wakeup_flag & (OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT | sim_wake_srcs)) {
			dev_info(atc260x_onoff->dev, "resend/simulate On/Off short press...\n");
			input_report_key(atc260x_onoff->idev, KEY_POWER, 1);
			input_report_key(atc260x_onoff->idev, KEY_POWER, 0);
			input_sync(atc260x_onoff->idev);
		} else if (wakeup_flag & OWL_PMIC_WAKEUP_SRC_ONOFF_LONG) {
			dev_info(atc260x_onoff->dev, "resend/simulate On/Off long press...\n");
			input_report_key(atc260x_onoff->idev, KEY_POWER, 1);
			input_sync(atc260x_onoff->idev);
			_atc260x_onoff_trigger_key_process(atc260x_onoff, msecs_to_jiffies(2500));
		}
#else
		/* Only resend short press. */
		if (wakeup_flag & (OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT | OWL_PMIC_WAKEUP_SRC_ONOFF_LONG | sim_wake_srcs)) {
			dev_info(atc260x_onoff->dev, "resend/simulate On/Off short press...\n");
			input_report_key(atc260x_onoff->idev, KEY_POWER, 1);
			input_report_key(atc260x_onoff->idev, KEY_POWER, 0);
			input_sync(atc260x_onoff->idev);
		}
#endif
	}
}

static const struct dev_pm_ops s_atc260x_onoff_pm_ops = {
	.suspend       = atc260x_onoff_suspend,
	.resume        = atc260x_onoff_resume,
	.prepare       = atc260x_onoff_suspend_prepare,
	.complete      = atc260x_onoff_resume_complete,
};

void atc260x_onoff_shutdown(struct platform_device *pdev)
{
	struct atc260x_onoff_dev *atc260x_onoff = platform_get_drvdata(pdev);
	struct atc260x_dev *atc260x = atc260x_onoff->atc260x;
	const struct atc260x_onoff_regdef *regdef = atc260x_onoff->regdef;

	dev_info(atc260x_onoff->dev, "%s() enter\n", __func__);

	/* call exit_func */
	if (regdef->exit_func != NULL) {
		(regdef->exit_func)(atc260x_onoff);
	}

	/* disable On/Off interrupt */
	_atc260x_onoff_rmw_intreg(atc260x, regdef,
		(regdef->press_int_en_bitm | regdef->kdwn_int_en_bitm), 0);
}

static int onoff_reset_irq = 0;
static struct irq_domain *onoff_reset_domain;

static void atc260x_onoff_irq_handle(struct atc260x_onoff_dev *atc260x_onoff)
{
	if(atc260x_onoff->regdef->reg_reset_pnd != 0 && onoff_reset_irq > 0)
	{
		int ret;
		struct atc260x_dev *atc260x = atc260x_onoff->atc260x;
		const struct atc260x_onoff_regdef *regdef = atc260x_onoff->regdef;

		ret = atc260x_reg_read(atc260x, regdef->reg_reset_pnd);
		if(ret > 0 && (ret & regdef->reset_pnd_bitm) != 0)
		{
			handle_nested_irq(onoff_reset_irq);
			pr_err("[onoff_reset_irq] %s: send irq!\n", __func__);
		}
	}
}

static void onoff_reset_irq_disable(struct irq_data *data)
{
}

static void onoff_reset_irq_enable(struct irq_data *data)
{
}

static struct irq_chip owl_sirq_irq = {
	.name = "onoff_reset_irq",
	.irq_ack = onoff_reset_irq_disable,
	.irq_mask = onoff_reset_irq_disable,
	.irq_mask_ack = onoff_reset_irq_disable,
	.irq_unmask = onoff_reset_irq_enable,
};

static int onoff_reset_irq_map(struct irq_domain *d, unsigned int virq,
				irq_hw_number_t hwirq)
{
	irq_set_chip_and_handler(virq, &owl_sirq_irq, handle_simple_irq);
	set_irq_flags(virq, IRQF_VALID);
	return 0;
}


static struct irq_domain_ops onoff_reset_irq_ops = {
	.map    = onoff_reset_irq_map,
	.xlate  = irq_domain_xlate_onecell,
};

static int atc260x_onoff_irq_chip_init(struct platform_device *pdev)
{
	onoff_reset_domain = irq_domain_add_linear(pdev->dev.of_node,
					   1, &onoff_reset_irq_ops, NULL);

	if (!onoff_reset_domain)
	{
		pr_err("[onoff_reset_irq] %s: irq_domain_add_linear failed!\n", __func__);
		return -ENODEV;
	}

	onoff_reset_irq = irq_create_mapping(onoff_reset_domain, 0);
	if(onoff_reset_irq <= 0)
	{
		pr_err("[onoff_reset_irq] %s: irq_create_mapping failed!\n", __func__);
		return -ENODEV;
	}
	
	return 0;
}

static int atc260x_onoff_probe(struct platform_device *pdev)
{
	struct atc260x_dev *atc260x;
	struct atc260x_onoff_dev *atc260x_onoff;
	uint ic_type;
	int irq, ret;

	dev_info(&pdev->dev, "Probing %s\n", pdev->name);

	atc260x = atc260x_get_parent_dev(&pdev->dev);

	atc260x_onoff = devm_kzalloc(&pdev->dev, sizeof(struct atc260x_onoff_dev),
				GFP_KERNEL);
	if (!atc260x_onoff) {
		dev_err(&pdev->dev, "Can't allocate data\n");
		return -ENOMEM;
	}
	atc260x_onoff->dev = &pdev->dev;
	atc260x_onoff->atc260x = atc260x;
	platform_set_drvdata(pdev, atc260x_onoff);

	INIT_DELAYED_WORK(&atc260x_onoff->work, atc260x_poll_onoff);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "No IRQ resource for On/Off key\n");
		ret = -ENODEV;
		goto err;
	}
	dev_info(&pdev->dev, "atc260x_onoff IRQ num : %u\n", irq);
	atc260x_onoff->irq = irq;

	atc260x_onoff->idev = input_allocate_device();
	if (!atc260x_onoff->idev) {
		dev_err(&pdev->dev, "Can't allocate input dev\n");
		ret = -ENOMEM;
		goto err;
	}

	atc260x_onoff->idev->evbit[0] = BIT_MASK(EV_KEY);
	atc260x_onoff->idev->keybit[BIT_WORD(KEY_POWER)] = BIT_MASK(KEY_POWER);
	atc260x_onoff->idev->name = "atc260x_onoff";
	atc260x_onoff->idev->phys = "atc260x_onoff/input0";
	atc260x_onoff->idev->dev.parent = &pdev->dev;

	ret = input_register_device(atc260x_onoff->idev);
	if (ret) {
		dev_dbg(&pdev->dev, "Can't register input device: %d\n", ret);
		goto err_input_alloc_dev;
	}

	/* init & enable hardware. */
	ic_type = atc260x_get_ic_type(atc260x);
	BUG_ON(ic_type >= ARRAY_SIZE(sc_atc260x_onoff_regdef_tbl));
	atc260x_onoff->regdef = sc_atc260x_onoff_regdef_tbl[ic_type];
	BUG_ON(atc260x_onoff->regdef == NULL);
	ret = (atc260x_onoff->regdef->init_func)(atc260x_onoff);
	if (ret) {
		dev_dbg(&pdev->dev, "%s() hardware init err, ret=%d\n", __func__, ret);
		goto err_input_reg_dev;
	}

	/*
	 *use default primary handle, and atc260x_onoff_irq run in ATC260X core irq kernel thread
	 */
	ret = devm_request_threaded_irq(&pdev->dev, atc260x_onoff->irq, NULL,
			atc260x_onoff_irq, IRQF_TRIGGER_HIGH, "atc260x_onoff",
			atc260x_onoff);
	if (ret < 0) {
		dev_err(&pdev->dev, "Unable to request IRQ: %d\n", ret);
		goto err_input_reg_dev;
	}

	atc260x_onoff_irq_chip_init(pdev);
	
	return 0;

err_input_reg_dev:
	input_unregister_device(atc260x_onoff->idev);
err_input_alloc_dev:
	input_free_device(atc260x_onoff->idev);
err:
	return ret;
}

static int atc260x_onoff_remove(struct platform_device *pdev)
{
	struct atc260x_onoff_dev *atc260x_onoff = platform_get_drvdata(pdev);
	cancel_delayed_work_sync(&atc260x_onoff->work);
	input_unregister_device(atc260x_onoff->idev);
	return 0;
}

static const struct of_device_id atc260x_onoff_match[] = {
	{ .compatible = "actions,atc2603a-onoff", },
	{ .compatible = "actions,atc2603c-onoff", },
	{ .compatible = "actions,atc2609a-onoff", },
	{},
};
MODULE_DEVICE_TABLE(of, atc260x_onoff_match);

static struct platform_driver atc260x_onoff_driver = {
	.driver     = {
		.name   = "atc260x-onoff",
		.owner  = THIS_MODULE,
		.pm     = &s_atc260x_onoff_pm_ops,
		.of_match_table = of_match_ptr(atc260x_onoff_match),
	},
	.probe      = atc260x_onoff_probe,
	.remove     = atc260x_onoff_remove,
	.shutdown   = atc260x_onoff_shutdown,
};

module_platform_driver(atc260x_onoff_driver);

MODULE_ALIAS("platform:atc260x-onff");
MODULE_DESCRIPTION("ATC260X ON/OFF pin");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Actions Semi, Inc");
