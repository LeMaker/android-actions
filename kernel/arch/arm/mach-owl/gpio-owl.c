/*
 * arch/arm/mach-owl/gpio-owl.c
 *
 * GPIO interface for Actions SOC
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_device.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/gpio.h>

#define GPIO_REG_BASE               (GPIO_MFP_PWM_BASE)

#define GPIO_BANK(gpio)             ((gpio) / 32)
#define GPIO_IN_BANK(gpio)          ((gpio) % 32)
#define GPIO_BIT(gpio)              (1 << GPIO_IN_BANK(gpio))

#define GPIO_REG_OUTEN(gpio)	(GPIO_REG_BASE + GPIO_BANK(gpio) * 0xc + 0x0)
#define GPIO_REG_INEN(gpio)	(GPIO_REG_BASE + GPIO_BANK(gpio) * 0xc + 0x4)
#define GPIO_REG_DAT(gpio)	(GPIO_REG_BASE + GPIO_BANK(gpio) * 0xc + 0x8)
#define GPIO_REG_INTC_PD(gpio)	(GPIO_REG_BASE + GPIO_BANK(gpio) * 0x8 + 0x208)
#define GPIO_REG_INTC_MASK(gpio)	(GPIO_REG_BASE  \
						+ GPIO_BANK(gpio) * 0x8 + 0x20c)
#define GPIO_REG_INTC_TYPE(gpio)	(GPIO_REG_BASE   \
						+  GPIO_BANK(gpio)*0x8 + 0x230)

#define GPIO_GAPD			(1<<0)
#define GPIO_GBPD			(1<<5)
#define GPIO_GCPD			(1<<10)
#define GPIO_GDPD			(1<<15)
#define GPIO_GEPD			(1<<20)

/* INTC_EXTCTL */
#define GPIO_INT_TYPE_MASK          0x3
#define GPIO_INT_TYPE_HIGH          0x0
#define GPIO_INT_TYPE_LOW           0x1
#define GPIO_INT_TYPE_RISING        0x2
#define GPIO_INT_TYPE_FALLING       0x3

static struct irq_domain *irq_domain;

#ifdef CONFIG_PINCTRL_OWL
DEFINE_SPINLOCK(owl_gpio_lock);
#else
static DEFINE_SPINLOCK(owl_gpio_lock);
#endif

#define _GPIOA(offset)		  (offset)
#define _GPIOB(offset)		  (32 + (offset))
#define _GPIOC(offset)		  (64 + (offset))
#define _GPIOD(offset)		  (96 + (offset))
#define _GPIOE(offset)		  (128 + (offset))

static int owl_gpio_request(struct gpio_chip *chip, unsigned offset)
{
#ifdef CONFIG_PINCTRL_OWL
	int ret;
	/*
	 * Map back to global GPIO space and request muxing, the direction
	 * parameter does not matter for this controller.
	 */
	int gpio = chip->base + offset;

	ret = pinctrl_request_gpio(gpio);
	return ret;
#else
	return 0;
#endif
}

static void owl_gpio_free(struct gpio_chip *chip, unsigned offset)
{
#ifdef CONFIG_PINCTRL_OWL
	int gpio = chip->base + offset;

	pinctrl_free_gpio(gpio);
#endif
}

static int owl_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	return act_readl(GPIO_REG_DAT(offset)) & GPIO_BIT(offset);
}

static void owl_gpio_set(struct gpio_chip *chip, unsigned offset, int val)
{
	unsigned int dat;

	dat = act_readl(GPIO_REG_DAT(offset));

	if (val)
		dat |= GPIO_BIT(offset);
	else
		dat &= ~GPIO_BIT(offset);

	act_writel(dat, GPIO_REG_DAT(offset));
}

static int owl_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&owl_gpio_lock, irq_flags);

	act_writel(act_readl(GPIO_REG_OUTEN(offset)) & ~GPIO_BIT(offset),
	GPIO_REG_OUTEN(offset));

	act_writel(act_readl(GPIO_REG_INEN(offset)) | GPIO_BIT(offset),
	GPIO_REG_INEN(offset));

	spin_unlock_irqrestore(&owl_gpio_lock, irq_flags);

	return 0;
}

static int owl_gpio_direction_output(struct gpio_chip *chip,
		unsigned offset, int val)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&owl_gpio_lock, irq_flags);

	act_writel(act_readl(GPIO_REG_INEN(offset)) & ~GPIO_BIT(offset),
			GPIO_REG_INEN(offset));

	act_writel(act_readl(GPIO_REG_OUTEN(offset)) | GPIO_BIT(offset),
			GPIO_REG_OUTEN(offset));

	owl_gpio_set(chip, offset, val);

	spin_unlock_irqrestore(&owl_gpio_lock, irq_flags);
	return 0;
}

static int owl_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
//	return OWL_EXT_GPIO_TO_IRQ(chip->base + offset);

	return irq_find_mapping(irq_domain, offset);
}

static struct gpio_chip owl_gpio_chip = {
	.label                = "owl-gpio-chip",
	.base               = 0,
	.ngpio              = NR_OWL_GPIO,
	.request            = owl_gpio_request,
	.free               = owl_gpio_free,
	.direction_input    = owl_gpio_direction_input,
	.direction_output   = owl_gpio_direction_output,
	.get                = owl_gpio_get,
	.set                = owl_gpio_set,
	.to_irq             = owl_gpio_to_irq,
};


static void owl_gpio_irq_mask(struct irq_data *d)
{
	int gpio = d->hwirq;
	unsigned long irq_flags;
	unsigned int val;

	spin_lock_irqsave(&owl_gpio_lock, irq_flags);

	val = act_readl(GPIO_REG_INTC_MASK(gpio));
	val &= ~GPIO_BIT(gpio);
	act_writel(val, GPIO_REG_INTC_MASK(gpio));

	if (val == 0) {
		val = act_readl(INTC_GPIOCTL);
		val &= ~(0x1 << (GPIO_BANK(gpio) * 5 + 1));
		act_writel(val, INTC_GPIOCTL);
	}

	spin_unlock_irqrestore(&owl_gpio_lock, irq_flags);
}


static void owl_gpio_irq_unmask(struct irq_data *d)
{
	int gpio = d->hwirq;
	unsigned long irq_flags;
	unsigned int val;

	spin_lock_irqsave(&owl_gpio_lock, irq_flags);

	val = act_readl(GPIO_REG_INTC_MASK(gpio));
	val |= GPIO_BIT(gpio);
	act_writel(val, GPIO_REG_INTC_MASK(gpio));

	val = act_readl(INTC_GPIOCTL);
	val |= 0x1 << (GPIO_BANK(gpio) * 5 + 1);
	act_writel(val, INTC_GPIOCTL);

	spin_unlock_irqrestore(&owl_gpio_lock, irq_flags);
}

static void owl_gpio_irq_ack(struct irq_data *d)
{
	unsigned long irq_flags, bank;
	unsigned int val;

	bank = d->hwirq >> 5;

	spin_lock_irqsave(&owl_gpio_lock, irq_flags);

	/* clear GPIO* IRQ pending */
	val = act_readl(INTC_GPIOCTL);
	switch (bank) {
	case 0:
		val &= ~(GPIO_GBPD|GPIO_GCPD|GPIO_GDPD|GPIO_GEPD);
		break;
	case 1:
		val &= ~(GPIO_GAPD|GPIO_GCPD|GPIO_GDPD|GPIO_GEPD);
		break;
	case 2:
		val &= ~(GPIO_GAPD|GPIO_GBPD|GPIO_GDPD|GPIO_GEPD);
		break;
	case 3:
		val &= ~(GPIO_GAPD|GPIO_GBPD|GPIO_GCPD|GPIO_GEPD);
		break;
	case 4:
		val &= ~(GPIO_GAPD|GPIO_GBPD|GPIO_GCPD|GPIO_GDPD);
		break;
	default:
		printk(KERN_INFO "[GPIO] %s(): invalid GPIO bank number %lu\n",
			__func__, bank);
		return;
	}

	act_writel(val, INTC_GPIOCTL);

	spin_unlock_irqrestore(&owl_gpio_lock, irq_flags);
}

static int owl_gpio_irq_set_type(struct irq_data *d, unsigned int flow_type)
{
	int gpio = d->hwirq;
	unsigned long irq_flags;
	unsigned int type, val, offset;

	spin_lock_irqsave(&owl_gpio_lock, irq_flags);

	if (flow_type & IRQ_TYPE_EDGE_BOTH)
		__irq_set_handler_locked(d->irq, handle_edge_irq);
	else
		__irq_set_handler_locked(d->irq, handle_level_irq);

	flow_type &= IRQ_TYPE_SENSE_MASK;

	switch (flow_type) {
	case IRQ_TYPE_EDGE_RISING:
		type = GPIO_INT_TYPE_RISING;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		type = GPIO_INT_TYPE_FALLING;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		type = GPIO_INT_TYPE_HIGH;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		type = GPIO_INT_TYPE_LOW;
		break;
	default:
		pr_err("[GPIO] %s: gpio %d, unknow irq type %d\n",
			__func__, gpio, flow_type);
	return -1;
	}

	offset = (GPIO_IN_BANK(gpio) < 16) ? 4 : 0;
	val = act_readl(GPIO_REG_INTC_TYPE(gpio)+offset);
	val &= ~(0x3 << ((gpio%16) * 2));
	val |= type << ((gpio%16) * 2);
	act_writel(val, (GPIO_REG_INTC_TYPE(gpio)+offset));

	spin_unlock_irqrestore(&owl_gpio_lock, irq_flags);

	return 0;
}

/*
 * When the summary IRQ is raised, any number of GPIO lines may be high.
 * It is the job of the summary handler to find all those GPIO lines
 * which have been set as summary IRQ lines and which are triggered,
 * and to call their interrupt handlers.
 */
static void owl_gpio_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	unsigned long bank, gpio_in_bank, pending, gpioctl;
	struct irq_chip *chip = irq_desc_get_chip(desc);

	chained_irq_enter(chip, desc);

	gpioctl = act_readl(INTC_GPIOCTL);

	bank = irq - OWL_IRQ_GPIOA;

	if (bank >= 0 && bank < 5) {
		if (gpioctl & (1 << ((bank * 5)))) {
			/* check pending status in one gpio bank  */
			pending = act_readl(GPIO_REG_INTC_PD(bank * 32));
			while (pending != 0) {
				gpio_in_bank = ffs(pending) - 1;

				generic_handle_irq(owl_gpio_to_irq(
				    &owl_gpio_chip, bank * 32 + gpio_in_bank));

				pending &= ~GPIO_BIT(gpio_in_bank);
			}
		}
	}

	chained_irq_exit(chip, desc);
}

static struct irq_chip owl_gpio_irq_chip = {
	.name           = "owl-gpio-irq",
	.irq_mask       = owl_gpio_irq_mask,
	.irq_unmask     = owl_gpio_irq_unmask,
	.irq_ack        = owl_gpio_irq_ack,
	.irq_set_type   = owl_gpio_irq_set_type,
};

struct owl_gpio_bank {
	int irq;
};

static struct owl_gpio_bank owl_gpio_banks[OWL_GPIO_BANKS];


static struct of_device_id owl_gpio_of_match[] = {
	{ .compatible = "actions,atm7039c-gpio"},
	{ .compatible = "actions,atm7059a-gpio"},
	{ },
};

static int owl_gpio_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	struct resource *res;
	int i, irq;
	int ret;

	printk(KERN_INFO "owl_gpio_probe()\n");

	of_id = of_match_device(owl_gpio_of_match, &pdev->dev);
	if (!of_id)
		return -EINVAL;

	for (i = 0; i < OWL_GPIO_BANKS; i++) {
		res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (res) {
			owl_gpio_banks[i].irq = res->start;
			printk(KERN_INFO "owl gpio bank%d - irq%d\n",
					i, res->start);
		} else {
			printk(KERN_INFO "owl gpio bank%d Missing IRQ resource\n"
					, i);
			owl_gpio_banks[i].irq = (OWL_IRQ_GPIOA + i);
		}
	}
	
	irq_domain = irq_domain_add_linear(pdev->dev.of_node,
					   owl_gpio_chip.ngpio, &irq_domain_simple_ops, NULL);

	if (!irq_domain)
		return -ENODEV;

#ifdef CONFIG_OF_GPIO
	owl_gpio_chip.of_node = pdev->dev.of_node;
#endif

	ret = gpiochip_add(&owl_gpio_chip);
	if (ret < 0)
		return ret;

	for (i = 0; i < owl_gpio_chip.ngpio; ++i) {
		irq = irq_create_mapping(irq_domain, i);
		irq_set_chip_and_handler(irq, &owl_gpio_irq_chip,
				handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	for (i = 0; i < OWL_GPIO_BANKS; i++)
		irq_set_chained_handler(owl_gpio_banks[i].irq,
					owl_gpio_irq_handler);


	return 0;
}

static struct platform_driver owl_gpio_driver = {
	.driver		= {
		.name	= "owl-gpio",
		.owner	= THIS_MODULE,
		.of_match_table = owl_gpio_of_match,
	},
	.probe		= owl_gpio_probe,
};

static int __init owl_gpio_init(void)
{
	printk(KERN_INFO "owl_gpio_init()\n");

	return platform_driver_register(&owl_gpio_driver);
}
postcore_initcall(owl_gpio_init);
