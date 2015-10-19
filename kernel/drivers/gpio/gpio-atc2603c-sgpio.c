/*
 * gpio-atc2603c-sgpio.c  --  gpiolib support for ATC260X
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <mach/power.h>
#include <linux/mfd/atc260x/atc260x.h>

/* SGPIO 专指atc2603c内的具有中断&唤醒功能的几个特殊GPIO,
 * atc2603a内也有叫SGPIO的pin, 是完全不同的功能, 不是这里的支持范围. */


#define ATC2603C_SGPIO_NR	7
#define ATC2603C_SGPIO_OBJ_TYPE_ID 0xfc3b25ceU
#define ATC2603C_SGPIO_CACHE_REG_CHANGED_MSK (1U << (sizeof(uint) * 8 -1))

/* for debug */
#define ATC2603C_SGPIO_ASSERT_VALID_DEV(ADEV) \
	BUG_ON(IS_ERR_OR_NULL(ADEV) || (ADEV)->_obj_type_id != ATC2603C_SGPIO_OBJ_TYPE_ID)


struct atc2603c_sgpio_mfp_map {
	u16 mask;
	u16 value;
};

struct atc2603c_sgpio_dev {
	struct device *dev;
	struct atc260x_dev *atc260x;
	struct gpio_chip gpio_chip;

	/* for IRQ function */
	struct mutex irq_bus_lock;
	struct irq_chip irq_chip;
	struct irq_domain *irq_domain;
	uint root_irq;
	uint cached_irq_en_mask;   /* MSB is the change bit */
	uint cached_irq_trig_type; /* MSB is the change bit */
	uint cached_irq_wake_mask; /* MSB is the change bit */
	int wake_count;
	uint parent_irq_no_wake;
	uint irq_map_tbl[ATC2603C_SGPIO_NR]; /* hwirq -> virq */

	/* for debug */
	u32 _obj_type_id; /* set to ATC2603C_SGPIO_OBJ_TYPE_ID */
};


/* Interrupt function ------------------------------------------------------- */

/* 原本这里可以直接用regmap_irq框架的, 但是因为SGPIO的中断mask和pending是同一个寄存器,
 * 不符合regmap_irq的要求, 硬用会出问题的, 所以这里只能自己实现中断分发了. */

static void _atc2603c_sgpio_irq_lock(struct irq_data *data)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;

	atc2603c_sgpio = irq_data_get_irq_chip_data(data);
	ATC2603C_SGPIO_ASSERT_VALID_DEV(atc2603c_sgpio);

	mutex_lock(&(atc2603c_sgpio->irq_bus_lock));
}

static void _atc2603c_sgpio_sync_cached_regs(struct atc2603c_sgpio_dev *atc2603c_sgpio)
{
	uint changed_mask;
	int ret;

	changed_mask = ATC2603C_SGPIO_CACHE_REG_CHANGED_MSK;

	/* write back cached registers */
	if (atc2603c_sgpio->cached_irq_wake_mask & changed_mask) {
		atc2603c_sgpio->cached_irq_wake_mask &= ~changed_mask;
		ret = atc260x_reg_write(atc2603c_sgpio->atc260x,
				ATC2603C_PMU_SGPIO_CTL2,
				atc2603c_sgpio->cached_irq_wake_mask);
		if (ret == 0) {
			/* setup root wakeup source */
			ret = owl_pmic_setup_aux_wakeup_src(
					OWL_PMIC_WAKEUP_SRC_SGPIOIRQ,
					(atc2603c_sgpio->cached_irq_wake_mask != 0));
			if (ret)
				dev_err(atc2603c_sgpio->dev,
					"%s: failed to setup SGPIOIRQ wakeup source\n", __func__);
		} else {
			dev_err(atc2603c_sgpio->dev,
				"%s: failed to write back cached irq_wake_mask\n", __func__);
		}
	}
	if (atc2603c_sgpio->cached_irq_trig_type & changed_mask) {
		atc2603c_sgpio->cached_irq_trig_type &= ~changed_mask;
		ret = atc260x_reg_write(atc2603c_sgpio->atc260x,
				ATC2603C_PMU_SGPIO_CTL1,
				atc2603c_sgpio->cached_irq_trig_type);
		if (ret)
			dev_err(atc2603c_sgpio->dev,
				"%s: failed to write back cached irq_trig_type\n", __func__);
	}
	/* en_mask MUST be handled at last */
	if (atc2603c_sgpio->cached_irq_en_mask & changed_mask) {
		atc2603c_sgpio->cached_irq_en_mask &= ~changed_mask;
		ret = atc260x_reg_wp_setbits(atc2603c_sgpio->atc260x,
				ATC2603C_PMU_SGPIO_CTL0,
				(0x7fU << 9),
				(0x7fU << 2),
				atc2603c_sgpio->cached_irq_en_mask);
		if (ret)
			dev_err(atc2603c_sgpio->dev,
				"%s: failed to write back cached irq_en_mask\n", __func__);
	}
}

static void _atc2603c_sgpio_irq_sync_unlock(struct irq_data *data)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	int i, ret;

	atc2603c_sgpio = irq_data_get_irq_chip_data(data);
	ATC2603C_SGPIO_ASSERT_VALID_DEV(atc2603c_sgpio);

	_atc2603c_sgpio_sync_cached_regs(atc2603c_sgpio);

	/* If we've changed our wakeup count propagate it to the parent */
	if (atc2603c_sgpio->wake_count) {
		if (!atc2603c_sgpio->parent_irq_no_wake) {
			if (atc2603c_sgpio->wake_count < 0)
				for (i = atc2603c_sgpio->wake_count; i < 0; i++) {
					ret = irq_set_irq_wake(atc2603c_sgpio->root_irq, 0);
					if (ret)
						break;
				}
			else
				for (i = 0; i < atc2603c_sgpio->wake_count; i++) {
					ret = irq_set_irq_wake(atc2603c_sgpio->root_irq, 1);
					if (ret && i == 0) {
						atc2603c_sgpio->parent_irq_no_wake = 1;
						break;
					}
				}
		}
		atc2603c_sgpio->wake_count = 0;
	}

	mutex_unlock(&(atc2603c_sgpio->irq_bus_lock));
}

static void _atc2603c_sgpio_irq_enable(struct irq_data *data)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	uint changed_mask;

	atc2603c_sgpio = irq_data_get_irq_chip_data(data);
	ATC2603C_SGPIO_ASSERT_VALID_DEV(atc2603c_sgpio);
	BUG_ON(data->hwirq >= ATC2603C_SGPIO_NR);

	changed_mask = ATC2603C_SGPIO_CACHE_REG_CHANGED_MSK;
	atc2603c_sgpio->cached_irq_en_mask |= changed_mask | (1U << (data->hwirq + 2));
}

static void _atc2603c_sgpio_irq_disable(struct irq_data *data)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	uint changed_mask;

	atc2603c_sgpio = irq_data_get_irq_chip_data(data);
	ATC2603C_SGPIO_ASSERT_VALID_DEV(atc2603c_sgpio);
	BUG_ON(data->hwirq >= ATC2603C_SGPIO_NR);

	changed_mask = ATC2603C_SGPIO_CACHE_REG_CHANGED_MSK;
	atc2603c_sgpio->cached_irq_en_mask |= changed_mask;
	atc2603c_sgpio->cached_irq_en_mask &= ~(1U << (data->hwirq + 2));
}

static int _atc2603c_sgpio_irq_set_type(struct irq_data *data, unsigned int flow_type)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	uint trig_type, changed_mask, cfg_value, shift_pos;

	atc2603c_sgpio = irq_data_get_irq_chip_data(data);
	ATC2603C_SGPIO_ASSERT_VALID_DEV(atc2603c_sgpio);
	BUG_ON(data->hwirq >= ATC2603C_SGPIO_NR);

	trig_type = flow_type & IRQ_TYPE_SENSE_MASK;
	if ((trig_type & (trig_type - 1)) != 0) {
		/* multi bits */
		dev_err(atc2603c_sgpio->dev, "not support IRQ flow_type 0x%x\n", trig_type);
		return -EINVAL;
	}

	cfg_value = 0;
	switch (trig_type) {
	case IRQ_TYPE_EDGE_RISING:
		cfg_value = 2;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		cfg_value = 3;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		cfg_value = 0;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		cfg_value = 1;
		break;
	}
	shift_pos = 2U + data->hwirq * 2U;
	changed_mask = ATC2603C_SGPIO_CACHE_REG_CHANGED_MSK;
	atc2603c_sgpio->cached_irq_trig_type =
		(atc2603c_sgpio->cached_irq_trig_type & ~(3U << shift_pos)) |
		(cfg_value << shift_pos) | changed_mask;

	return 0;
}

static int _atc2603c_sgpio_irq_set_wake(struct irq_data *data, unsigned int on)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	uint changed_mask;

	atc2603c_sgpio = irq_data_get_irq_chip_data(data);
	ATC2603C_SGPIO_ASSERT_VALID_DEV(atc2603c_sgpio);
	BUG_ON(data->hwirq >= ATC2603C_SGPIO_NR);

	on = !!on;

	changed_mask = ATC2603C_SGPIO_CACHE_REG_CHANGED_MSK;
	atc2603c_sgpio->cached_irq_wake_mask =
		(atc2603c_sgpio->cached_irq_wake_mask & ~(1U << (data->hwirq + 2))) |
		(on << (data->hwirq + 2)) | changed_mask;

	if (on)
		atc2603c_sgpio->wake_count++;
	else
		atc2603c_sgpio->wake_count--;
	return 0;
}

static const struct irq_chip sc_atc2603c_sgpio_irq_chip_template = {
	.name                   = "atc2603c-sgpio-irqc",
	.irq_bus_lock           = _atc2603c_sgpio_irq_lock,
	.irq_bus_sync_unlock    = _atc2603c_sgpio_irq_sync_unlock,
	.irq_disable            = _atc2603c_sgpio_irq_disable,
	.irq_enable             = _atc2603c_sgpio_irq_enable,
	.irq_set_type           = _atc2603c_sgpio_irq_set_type,
	.irq_set_wake           = _atc2603c_sgpio_irq_set_wake,
};

static int _atc2603c_sgpio_irq_map(struct irq_domain *h, unsigned int virq,
			  irq_hw_number_t hw)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio = h->host_data;

	ATC2603C_SGPIO_ASSERT_VALID_DEV(atc2603c_sgpio);

	irq_set_chip_data(virq, atc2603c_sgpio);
	irq_set_chip(virq, &atc2603c_sgpio->irq_chip);
	irq_set_nested_thread(virq, 1);

	/* ARM needs us to explicitly flag the IRQ as valid
	 * and will set them noprobe when we do so. */
#ifdef CONFIG_ARM
	set_irq_flags(virq, IRQF_VALID);
#else
	irq_set_noprobe(virq);
#endif

	return 0;
}

static const struct irq_domain_ops sc_atc2603c_sgpio_domain_ops = {
	.map	= _atc2603c_sgpio_irq_map,
	.xlate	= irq_domain_xlate_twocell,
};

static irqreturn_t _atc2603c_sgpio_irq_thread(int irq, void *udata)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio = udata;
	uint irq_status, irq_mask, irq_mask_real;
	uint hwirq;
	int ret;

	ATC2603C_SGPIO_ASSERT_VALID_DEV(atc2603c_sgpio);

	ret = atc260x_reg_read(atc2603c_sgpio->atc260x, ATC2603C_PMU_SGPIO_CTL0);
	if (ret < 0) {
		dev_err(atc2603c_sgpio->dev, "%s: read status reg err, ret=%d\n",
			__func__, ret);
		return IRQ_NONE;
	}
	irq_status = ((uint)ret >> 9) & 0x7f;
	irq_mask_real = ((uint)ret >> 2) & 0x7f;

	irq_mask = (atc2603c_sgpio->cached_irq_en_mask &
			~ATC2603C_SGPIO_CACHE_REG_CHANGED_MSK) >> 2;
	BUG_ON(irq_mask_real != irq_mask);

	if ((irq_status & irq_mask) == 0) {
		dev_err(atc2603c_sgpio->dev, "%s: empty IRQ, irq_status=0x%x irq_mask=0x%x\n",
			__func__, irq_status, irq_mask);
		return IRQ_NONE;
	}
	irq_status = irq_status & irq_mask;

	/* clear all pending */
	mutex_lock(&(atc2603c_sgpio->irq_bus_lock));
	ret = atc260x_reg_wp_clrpnd(atc2603c_sgpio->atc260x,
			ATC2603C_PMU_SGPIO_CTL0,
			(0x7fU << 9),
			(irq_status << 9));
	mutex_unlock(&(atc2603c_sgpio->irq_bus_lock));
	if (ret) {
		dev_err(atc2603c_sgpio->dev, "%s: failed to clr pending, ret=%d\n",
			__func__, ret);
		return IRQ_NONE;
	}

	do {
		hwirq = __ffs(irq_status);
		handle_nested_irq(irq_find_mapping(atc2603c_sgpio->irq_domain, hwirq));
		irq_status &= ~(1U << hwirq);
	} while (irq_status != 0);

	return IRQ_HANDLED;
}

static void _atc2603c_sgpio_dispose_all_irq_mappings(struct atc2603c_sgpio_dev *atc2603c_sgpio)
{
	uint hwirq, virq;

	for (hwirq = 0; hwirq < ATC2603C_SGPIO_NR; hwirq++) {
		virq = atc2603c_sgpio->irq_map_tbl[hwirq];
		if (virq > 0) {
			atc2603c_sgpio->irq_map_tbl[hwirq] = 0;
			irq_dispose_mapping(virq);
		}
	}
}

static int _atc2603c_sgpio_create_all_irq_mappings(struct atc2603c_sgpio_dev *atc2603c_sgpio)
{
	uint hwirq, virq;

	memset(atc2603c_sgpio->irq_map_tbl, 0, sizeof(atc2603c_sgpio->irq_map_tbl));

	for (hwirq = 0; hwirq < ATC2603C_SGPIO_NR; hwirq++) {
		virq = irq_create_mapping(atc2603c_sgpio->irq_domain, hwirq);
		if (virq == 0) {
			_atc2603c_sgpio_dispose_all_irq_mappings(atc2603c_sgpio);
			dev_err(atc2603c_sgpio->dev,
				"%s: failed to create IRQ mapping for hwirq %u\n",
				__func__, hwirq);
			return -ENXIO;
		}
		atc2603c_sgpio->irq_map_tbl[hwirq] = virq;
		dev_info(atc2603c_sgpio->dev, "new IRQ mapping: hwirq%u -> virq%u\n",
			hwirq, virq);
	}

	return 0;
}

static int _atc2603c_sgpio_irq_init(struct atc2603c_sgpio_dev *atc2603c_sgpio)
{
	struct atc260x_dev *atc260x;
	int ret;

	atc260x = atc2603c_sgpio->atc260x;

	/* Mask all the interrupts by default */
	/* Wake is disabled by default */
	ret = atc260x_reg_write(atc260x, ATC2603C_PMU_SGPIO_CTL0, (0x7fU<<9) | 0);
	ret |= atc260x_reg_write(atc260x, ATC2603C_PMU_SGPIO_CTL1, 0);
	ret |= atc260x_reg_write(atc260x, ATC2603C_PMU_SGPIO_CTL2, 0);
	if (ret) {
		dev_err(atc2603c_sgpio->dev, "failed to init interrupt hardware\n");
		return -EIO;
	}
	atc2603c_sgpio->cached_irq_en_mask = 0;
	atc2603c_sgpio->cached_irq_trig_type = 0;
	atc2603c_sgpio->cached_irq_wake_mask = 0;
	atc2603c_sgpio->wake_count = 0;
	atc2603c_sgpio->parent_irq_no_wake = 0; /* assume 0, will check later */

	mutex_init(&(atc2603c_sgpio->irq_bus_lock));

	atc2603c_sgpio->irq_chip = sc_atc2603c_sgpio_irq_chip_template;

	atc2603c_sgpio->irq_domain = irq_domain_add_linear(
			atc2603c_sgpio->dev->of_node,
			ATC2603C_SGPIO_NR,
			&sc_atc2603c_sgpio_domain_ops, atc2603c_sgpio);
	if (!atc2603c_sgpio->irq_domain) {
		dev_err(atc2603c_sgpio->dev, "failed to create IRQ domain\n");
		return -ENOMEM;
	}

	ret = _atc2603c_sgpio_create_all_irq_mappings(atc2603c_sgpio);
	if (ret) {
		irq_domain_remove(atc2603c_sgpio->irq_domain);
		dev_err(atc2603c_sgpio->dev, "failed to create IRQ mappings\n");
	}

	dev_info(atc2603c_sgpio->dev, "root virq num : %u\n", atc2603c_sgpio->root_irq);
	ret = devm_request_threaded_irq(
			atc2603c_sgpio->dev,
			atc2603c_sgpio->root_irq,
			NULL, _atc2603c_sgpio_irq_thread,
			IRQF_ONESHOT,
			atc2603c_sgpio->irq_chip.name, atc2603c_sgpio);
	if (ret != 0) {
		_atc2603c_sgpio_dispose_all_irq_mappings(atc2603c_sgpio);
		irq_domain_remove(atc2603c_sgpio->irq_domain);
		dev_err(atc2603c_sgpio->dev, "failed to request root IRQ %d, ret=%d\n",
			atc2603c_sgpio->root_irq, ret);
		return ret;
	}

	return 0;
}

static void _atc2603c_sgpio_irq_exit(struct atc2603c_sgpio_dev *atc2603c_sgpio)
{
	devm_free_irq(atc2603c_sgpio->dev, atc2603c_sgpio->root_irq, atc2603c_sgpio);
	_atc2603c_sgpio_dispose_all_irq_mappings(atc2603c_sgpio);
	irq_domain_remove(atc2603c_sgpio->irq_domain);
}


/* GPIO function ------------------------------------------------------------ */

static const struct atc2603c_sgpio_mfp_map sc_atc2603c_sgpio_mfp_map[] = {
	/* mask        val */
	{0x0003,     0x0001},    /* GPIO0 */
	{0x001c,     0x0004},    /* GPIO1 */
	{0x00e0,     0x0020},    /* GPIO2 */
	{0x0300,     0x0100},    /* GPIO3 */
	{0x0c00,     0x0000},    /* GPIO4 */
	{0x3000,     0x0000},    /* GPIO5 */
	{0xc000,     0x0000},    /* GPIO6 */
};

static inline struct atc2603c_sgpio_dev *_get_atc2603c_sgpio_dev(struct gpio_chip *chip)
{
	return dev_get_drvdata(chip->dev);
}

static int _atc2603c_sgpio_request(struct gpio_chip *chip, unsigned offset)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	struct atc2603c_sgpio_mfp_map const *mfp_map;
	uint mfp_reg_msk, mfp_reg_val;
	int ret;

	if (offset >= ATC2603C_SGPIO_NR)
		return -EINVAL;

	atc2603c_sgpio = _get_atc2603c_sgpio_dev(chip);
	mfp_map = sc_atc2603c_sgpio_mfp_map;

	mfp_reg_msk = (mfp_map[offset]).mask;
	mfp_reg_val = (mfp_map[offset]).value;

	/* once GPIO MFP set, no way (no need) to revert */
	ret = atc260x_reg_setbits(atc2603c_sgpio->atc260x, ATC2603C_PMU_MUX_CTL0, mfp_reg_msk, mfp_reg_val);
	if (ret)
		return ret;

	dev_info(atc2603c_sgpio->dev, "requested SGPIO #%u\n", offset);
	return 0;
}

static int _atc2603c_sgpio_get_direction(struct gpio_chip *chip, unsigned offset)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	int ret;

	if (offset >= ATC2603C_SGPIO_NR)
		return -EINVAL;

	atc2603c_sgpio = _get_atc2603c_sgpio_dev(chip);

	ret = atc260x_reg_read(atc2603c_sgpio->atc260x, ATC2603C_PMU_SGPIO_CTL3);
	if (ret < 0)
		return ret;
	if (ret & (1U << (offset + 2U + ATC2603C_SGPIO_NR)))
		return GPIOF_DIR_OUT;
	if (ret & (1U << (offset + 2U)))
		return GPIOF_DIR_IN;

	return -ENXIO;
}

static int _atc2603c_sgpio_direction_in(struct gpio_chip *chip, unsigned offset)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	int ret;

	if (offset >= ATC2603C_SGPIO_NR)
		return -EINVAL;

	atc2603c_sgpio = _get_atc2603c_sgpio_dev(chip);

	ret = atc260x_reg_setbits(atc2603c_sgpio->atc260x,
		ATC2603C_PMU_SGPIO_CTL3,
		(1U << (offset + 2U + ATC2603C_SGPIO_NR)) | (1U << (offset + 2U)),
		(1U << (offset + 2U)));
	return ret;
}

static int _atc2603c_sgpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	int ret;

	if (offset >= ATC2603C_SGPIO_NR)
		return -EINVAL;

	atc2603c_sgpio = _get_atc2603c_sgpio_dev(chip);

	ret = atc260x_reg_read(atc2603c_sgpio->atc260x, ATC2603C_PMU_SGPIO_CTL4);
	if (ret < 0)
		return ret;

	return (((uint)ret & (1U << offset)) != 0);
}

static int _atc2603c_sgpio_direction_out(struct gpio_chip *chip, unsigned offset, int value)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	int ret;

	if (offset >= ATC2603C_SGPIO_NR)
		return -EINVAL;

	atc2603c_sgpio = _get_atc2603c_sgpio_dev(chip);

	ret = atc260x_reg_setbits(atc2603c_sgpio->atc260x,
		ATC2603C_PMU_SGPIO_CTL4,
		(1U << offset),
		((value != 0) ? (1U << offset) : 0));
	if (ret == 0) {
		ret = atc260x_reg_setbits(atc2603c_sgpio->atc260x,
			ATC2603C_PMU_SGPIO_CTL3,
			(1U << (offset + 2U + ATC2603C_SGPIO_NR)) | (1U << (offset + 2U)),
			(1U << (offset + 2U + ATC2603C_SGPIO_NR)));
	}
	return ret;
}

static void _atc2603c_sgpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	int ret;

	if (offset >= ATC2603C_SGPIO_NR)
		return;

	atc2603c_sgpio = _get_atc2603c_sgpio_dev(chip);

	ret = atc260x_reg_setbits(atc2603c_sgpio->atc260x,
		ATC2603C_PMU_SGPIO_CTL4,
		(1U << offset),
		((value != 0) ? (1U << offset) : 0));
	if (ret) {
		dev_err(atc2603c_sgpio->dev, "failed to set GPIO%u state to %d, ret=%d\n",
			offset, value, ret);
	}
}

static int _atc2603c_sgpio_find_irq_nr(struct gpio_chip *chip, unsigned offset)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	int ret;

	if (offset >= ATC2603C_SGPIO_NR)
		return -EINVAL;

	atc2603c_sgpio = _get_atc2603c_sgpio_dev(chip);
	ret = atc2603c_sgpio->irq_map_tbl[offset]; /* hw_gpio number is the hwirq number. */
	if (ret <= 0)
		return -ENOENT;
	return ret;
}

static const struct gpio_chip sc_atc2603c_sgpio_chip_template = {
	.label              = "atc2603c-sgpio-chip",
	.owner              = THIS_MODULE,
	.request            = _atc2603c_sgpio_request,
	.free               = NULL,
	.get_direction      = _atc2603c_sgpio_get_direction,
	.direction_input    = _atc2603c_sgpio_direction_in,
	.get                = _atc2603c_sgpio_get,
	.direction_output   = _atc2603c_sgpio_direction_out,
	.set                = _atc2603c_sgpio_set,
	.to_irq             = _atc2603c_sgpio_find_irq_nr,
	.set_debounce       = NULL,
	.dbg_show           = NULL,
	.can_sleep          = 1,
};


/* Driver ------------------------------------------------------------------- */

static int atc2603c_sgpio_probe(struct platform_device *pdev)
{
	struct atc260x_dev *atc260x;
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	int ret;

	dev_info(&pdev->dev, "Probing...\n");

	atc260x = atc260x_get_parent_dev(&pdev->dev);

	atc2603c_sgpio = devm_kzalloc(&pdev->dev, sizeof(*atc2603c_sgpio), GFP_KERNEL);
	if (atc2603c_sgpio == NULL) {
		dev_err(&pdev->dev, "no mem\n");
		return -ENOMEM;
	}
	atc2603c_sgpio->dev = &pdev->dev;
	atc2603c_sgpio->atc260x = atc260x;
	atc2603c_sgpio->_obj_type_id = ATC2603C_SGPIO_OBJ_TYPE_ID;
	platform_set_drvdata(pdev, atc2603c_sgpio);

	ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		dev_err(&pdev->dev, "No IRQ resource\n");
		return -ENODEV;
	}
	atc2603c_sgpio->root_irq = ret;

	ret = _atc2603c_sgpio_irq_init(atc2603c_sgpio);
	if (ret) {
		dev_err(&pdev->dev, "failed to init SGPIO interrupt controller, ret=%d\n", ret);
		return ret;
	}

	atc2603c_sgpio->gpio_chip = sc_atc2603c_sgpio_chip_template;
	atc2603c_sgpio->gpio_chip.dev = &pdev->dev;
	atc2603c_sgpio->gpio_chip.base = -1; /* use a dynamic range, user get GPIO via DTS bindings */
	atc2603c_sgpio->gpio_chip.ngpio = ATC2603C_SGPIO_NR;

	ret = gpiochip_add(&atc2603c_sgpio->gpio_chip);
	if (ret < 0) {
		_atc2603c_sgpio_irq_exit(atc2603c_sgpio);
		dev_err(&pdev->dev, "Failed to register gpiochip, ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int atc2603c_sgpio_remove(struct platform_device *pdev)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	int ret;

	atc2603c_sgpio = platform_get_drvdata(pdev);

	ret = gpiochip_remove(&atc2603c_sgpio->gpio_chip);
	if (ret == 0)
		dev_err(&pdev->dev, "Failed to unregister gpiochip, ret=%d\n", ret);

	_atc2603c_sgpio_irq_exit(atc2603c_sgpio);

	atc2603c_sgpio->_obj_type_id = 0;
	return ret;
}

static int atc2603c_sgpio_suspend_late(struct device *dev)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;
	uint changed_mask;
	int ret;

	atc2603c_sgpio = dev_get_drvdata(dev);
	dev_info(atc2603c_sgpio->dev, "enter suspend\n");

	mutex_lock(&(atc2603c_sgpio->irq_bus_lock));

	changed_mask = ATC2603C_SGPIO_CACHE_REG_CHANGED_MSK;
	atc2603c_sgpio->cached_irq_wake_mask |= changed_mask; /* mark as changed */
	atc2603c_sgpio->cached_irq_trig_type |= changed_mask;
	atc2603c_sgpio->cached_irq_en_mask   |= changed_mask;

	ret = atc260x_reg_wp_setbits(atc2603c_sgpio->atc260x,
			ATC2603C_PMU_SGPIO_CTL0,
			(0x7fU << 9),
			(0x7fU << 2),
			0);
	if (ret)
		dev_err(atc2603c_sgpio->dev, "%s: failed to disable all IRQ\n", __func__);
	return ret;
}

static int atc2603c_sgpio_resume_early(struct device *dev)
{
	struct atc2603c_sgpio_dev *atc2603c_sgpio;

	atc2603c_sgpio = dev_get_drvdata(dev);
	dev_info(atc2603c_sgpio->dev, "resume\n");

	_atc2603c_sgpio_sync_cached_regs(atc2603c_sgpio);

	mutex_unlock(&(atc2603c_sgpio->irq_bus_lock));
	return 0;
}

static const struct dev_pm_ops sc_atc2603c_sgpio_pm_ops = {
	.suspend_late  = atc2603c_sgpio_suspend_late,
	.resume_early  = atc2603c_sgpio_resume_early,
	.freeze_late   = atc2603c_sgpio_suspend_late,
	.thaw_early    = atc2603c_sgpio_resume_early,
	.poweroff_late = atc2603c_sgpio_suspend_late,
	.restore_early = atc2603c_sgpio_resume_early,
};

static const struct of_device_id atc2603c_sgpio_match[] = {
	{ .compatible = "actions,atc2603c-sgpio", },
	{},
};
MODULE_DEVICE_TABLE(of, atc2603c_sgpio_match);

static struct platform_driver atc2603c_sgpio_driver = {
	.probe      = atc2603c_sgpio_probe,
	.remove     = atc2603c_sgpio_remove,
	.driver = {
		.name = "atc2603c-sgpio",
		.owner = THIS_MODULE,
		.pm = &sc_atc2603c_sgpio_pm_ops,
		.of_match_table = atc2603c_sgpio_match,
	},
};

static int __init atc2603c_sgpio_init(void)
{
	return platform_driver_register(&atc2603c_sgpio_driver);
}
subsys_initcall(atc2603c_sgpio_init);

static void __exit atc2603c_sgpio_exit(void)
{
	platform_driver_unregister(&atc2603c_sgpio_driver);
}
module_exit(atc2603c_sgpio_exit);

MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("SGPIO interface for ATC260X PMIC");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc2603c-sgpio");
