/*
 * arch/arm/mach-owl/sirq.c
 *
 * special external IRQs support for Actions SOC
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
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
/*#include <mach/gl5202_cmu.h>*/

/* INTC_EXTCTL */
#define INTC_EXTCTL_E0EN            (0x1 << 21)
#define INTC_EXTCTL_E0PD            (0x1 << 16)
#define INTC_EXTCTL_E1EN            (0x1 << 13)
#define INTC_EXTCTL_E1PD            (0x1 << 8)
#define INTC_EXTCTL_E2EN            (0x1 << 5)
#define INTC_EXTCTL_E2PD            (0x1 << 0)

#define INTC_EXTCTL_ETYPE(x)    (22-x*8)

#define EXT_INT_TYPE_MASK           0x3
#define EXT_INT_TYPE_HIGH           0x0
#define EXT_INT_TYPE_LOW            0x1
#define EXT_INT_TYPE_RISING         0x2
#define EXT_INT_TYPE_FALLING        0x3

#define PAD_PULLCTL0_PSIRQP(x)     (14-x*2)

static struct irq_domain *owl_sirq_irq_domain;

static void owl_sirq_enable(struct irq_data *data)
{
	unsigned int hwirq = data->hwirq;
    
	switch (hwirq) {
	case 0:
		act_writel(act_readl(INTC_EXTCTL) | INTC_EXTCTL_E0EN,
			INTC_EXTCTL);
		break;
	case 1:
		act_writel(act_readl(INTC_EXTCTL) | INTC_EXTCTL_E1EN,
			INTC_EXTCTL);
		break;
	case 2:
		act_writel(act_readl(INTC_EXTCTL) | INTC_EXTCTL_E2EN,
			INTC_EXTCTL);
		break;
	default:
		break;
	}
}

static void owl_sirq_disable(struct irq_data *data)
{
	unsigned int hwirq = data->hwirq;

	switch (hwirq) {
	case 0:
		act_writel(act_readl(INTC_EXTCTL) & (~INTC_EXTCTL_E0EN),
			INTC_EXTCTL);
		break;
	case 1:
		act_writel(act_readl(INTC_EXTCTL) & (~INTC_EXTCTL_E1EN),
			INTC_EXTCTL);
		break;
	case 2:
		act_writel(act_readl(INTC_EXTCTL) & (~INTC_EXTCTL_E2EN),
			INTC_EXTCTL);
		break;
	default:
		break;
	}
}

static int owl_sirq_set_type(struct irq_data *data, unsigned int flow_type)
{
	unsigned int regv, type, offset;

	unsigned int hwirq = data->hwirq;
	flow_type &= IRQ_TYPE_SENSE_MASK;
	offset = hwirq;

	switch (flow_type) {
	case IRQ_TYPE_EDGE_RISING:
		type = EXT_INT_TYPE_RISING;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		type = EXT_INT_TYPE_FALLING;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		type = EXT_INT_TYPE_HIGH;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		type = EXT_INT_TYPE_LOW;
		break;
	default:
	pr_err("[SIRQ] %s: hwirq %d, unknow type %d\n",
		__func__, hwirq, flow_type);
	return -1;
	}

	regv = act_readl(INTC_EXTCTL);
	regv &= ~(0x3<<INTC_EXTCTL_ETYPE(offset));
	regv |= type<<INTC_EXTCTL_ETYPE(offset);
	act_writel(regv, INTC_EXTCTL);

	regv = act_readl(PAD_PULLCTL0);
	regv &= ~(0x3<<PAD_PULLCTL0_PSIRQP(offset));
	if ((flow_type == IRQ_TYPE_LEVEL_HIGH) ||
			(flow_type == IRQ_TYPE_EDGE_RISING))
		/* 100K pull-up disable and 100K pull-down enable */
		regv |= 0x2<<PAD_PULLCTL0_PSIRQP(offset);
	if ((flow_type == IRQ_TYPE_LEVEL_LOW) ||
			(flow_type == IRQ_TYPE_EDGE_FALLING))
		/* 100K pull-up enable and 100K pull-down disable */
		regv |= 0x1<<PAD_PULLCTL0_PSIRQP(offset);
	act_writel(regv, PAD_PULLCTL0);

	return 0;
}

static struct irq_chip owl_sirq_irq = {
	.name = "owl_sirq_irq",
	.irq_ack = owl_sirq_disable,
	.irq_mask = owl_sirq_disable,
	.irq_mask_ack = owl_sirq_disable,
	.irq_unmask = owl_sirq_enable,
	.irq_set_type = owl_sirq_set_type,
};

static void owl_sirq_handler(unsigned int irq, struct irq_desc *desc)
{
	unsigned int pending, casc_irq;
	struct irq_chip *chip = irq_get_chip(irq);

	pending = act_readl(INTC_EXTCTL);
	casc_irq = -1;

	switch (irq) {
	case OWL_IRQ_SIRQ0:
		if (pending & INTC_EXTCTL_E0PD) {
		    casc_irq = irq_find_mapping(owl_sirq_irq_domain, 0);
			/* don't clear other sirq pending */
			pending &= ~(INTC_EXTCTL_E1PD | INTC_EXTCTL_E2PD);
		}
		break;
	case OWL_IRQ_SIRQ1:
		if (pending & INTC_EXTCTL_E1PD) {
		    casc_irq = irq_find_mapping(owl_sirq_irq_domain, 1);
			/* don't clear other sirq pending */
			pending &= ~(INTC_EXTCTL_E0PD | INTC_EXTCTL_E2PD);
		}
		break;
	case OWL_IRQ_SIRQ2:
		if (pending & INTC_EXTCTL_E2PD) {
		    casc_irq = irq_find_mapping(owl_sirq_irq_domain, 2);
			/* don't clear other sirq pending */
			pending &= ~(INTC_EXTCTL_E0PD | INTC_EXTCTL_E1PD);
		}
		break;
	default:
		printk(KERN_INFO "[SIRQ] %s(): invalid virtual sirq number %d\n",
			__func__, irq);
	return;
	}

	/* clear pending */
	act_writel(pending, INTC_EXTCTL);
	if(casc_irq >= 0)
		generic_handle_irq(casc_irq);

	if (chip->irq_ack)
		chip->irq_ack(&desc->irq_data);
	if (chip->irq_eoi)
		chip->irq_eoi(&desc->irq_data);

	chip->irq_unmask(&desc->irq_data);
}

struct owl_sirq_bank {
	int irq;
};

static struct owl_sirq_bank owl_sirq_bank[NR_OWL_SIRQ_IRQS];

static int owl_sirq_irq_map(struct irq_domain *d, unsigned int virq,
				irq_hw_number_t hwirq)
{
	irq_set_chip_and_handler(virq, &owl_sirq_irq, handle_simple_irq);
	set_irq_flags(virq, IRQF_VALID);
	irq_set_chained_handler(owl_sirq_bank[hwirq].irq, owl_sirq_handler);
    printk("sirq map %d -> %d\n", owl_sirq_bank[hwirq].irq, virq);
	return 0;
}

static struct irq_domain_ops owl_sirq_irq_ops = {
	.map    = owl_sirq_irq_map,
	.xlate  = irq_domain_xlate_twocell,
};


static struct of_device_id owl_sirq_of_match[] = {
	{ .compatible = "actions,owl-sirq" },
	{ },
};

static int owl_sirq_probe(struct platform_device *pdev)
{
	struct resource *res;
	int i, irq;

	printk(KERN_INFO "owl_sirq_probe()\n");

	for (i = 0; i < NR_OWL_SIRQ_IRQS; i++) {
		res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (res) {
			owl_sirq_bank[i].irq = res->start;
			printk(KERN_INFO "owl sirq bank%d - irq%d\n",
					i, res->start);
		} else {
			printk(KERN_INFO "owl gpio bank%d Missing IRQ resource\n"
					, i);
			owl_sirq_bank[i].irq = (OWL_IRQ_SIRQ0 + i);
		}
	}
	
	owl_sirq_irq_domain = irq_domain_add_linear(pdev->dev.of_node,
					   NR_OWL_SIRQ_IRQS, &owl_sirq_irq_ops, NULL);

	if (!owl_sirq_irq_domain)
	{
    	pr_err("[SIRQ] %s: irq_domain_add_linear failed!\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < NR_OWL_SIRQ_IRQS; ++i) {
		irq = irq_create_mapping(owl_sirq_irq_domain, i);
	}

	return 0;
}


static struct platform_driver owl_sirq_driver = {
	.driver		= {
		.name	= "owl-sirq",
		.owner	= THIS_MODULE,
		.of_match_table = owl_sirq_of_match,
	},
	.probe		= owl_sirq_probe,
};


int __init owl_sirq_init(void)
{
	printk(KERN_INFO "init sirqs\n");

	return platform_driver_register(&owl_sirq_driver);

}

postcore_initcall(owl_sirq_init);
