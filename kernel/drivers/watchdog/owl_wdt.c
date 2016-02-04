/*
 * watchdog driver for OWL SOC
 *
 * Copyright 2013 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/fs.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <asm/div64.h>
#include <linux/of_irq.h>

#include <asm/smp_twd.h>
#include <mach/powergate.h>
#include <mach/clkname.h>
#include <asm/system_misc.h>

struct owl_wdt {
	void __iomem	*wd_base;
	void __iomem	*cmu_base;
	int		irq;
	struct clk *periph_clk;
	struct notifier_block nb;
	int wd_type;	//0-hard watchdog; 1-soft watchdog
};

static DEFINE_SPINLOCK(wdt_lock);

#define TIMER_MARGIN	60		/* Default is 60 seconds */
static unsigned int owl_margin = TIMER_MARGIN;	/* in seconds */
module_param(owl_margin, uint, 0);
MODULE_PARM_DESC(owl_margin,
	"OWL watchdog owl_margin in seconds. (0 < owl_margin < 3600, default="
					__MODULE_STRING(TIMER_MARGIN) ")");

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
		"Watchdog cannot be stopped once started (default="
				__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static int owl_noboot = 0;
module_param(owl_noboot, int, 0);
MODULE_PARM_DESC(owl_noboot,
	"OWL watchdog action, set to 1 to ignore reboots, 0 to reboot (default=0)");


static unsigned int owl_get_periphclk(struct owl_wdt *wdt)
{
	unsigned int corepll, div;
	
	corepll = (readl_relaxed(wdt->cmu_base + 0x0) & 0xff) * (12 * 1000000);
	div =((readl_relaxed(wdt->cmu_base + 0x1c) >> 20) & 0x7) + 1;
	
	return (corepll / div);
}

/*
 *	owl_wdt_keepalive - reload the timer
 *
 *	Note that the spec says a DIFFERENT value must be written to the reload
 *	register each time.  The "perturb" variable deals with this by adding 1
 *	to the count every other time the function is called.
 */
static void __owl_wdt_keepalive(struct watchdog_device *wdd)
{
	unsigned int twd_clk_hz;
	struct owl_wdt *wdt = watchdog_get_drvdata(wdd);
	unsigned long count;

	spin_lock(&wdt_lock);
	twd_clk_hz = owl_get_periphclk(wdt);
	count = wdd->timeout * (twd_clk_hz / 256);
	/* Reload the counter */
	writel_relaxed(count, wdt->wd_base + TWD_WDOG_LOAD);
	spin_unlock(&wdt_lock);
}

static void __owl_wdt_start(struct watchdog_device *wdd)
{
	struct owl_wdt *wdt = watchdog_get_drvdata(wdd);

	dev_info(wdd->dev, "enabling watchdog timeout=%ds\n", wdd->timeout);

	/* This loads the count register but does NOT start the count yet */
	__owl_wdt_keepalive(wdd);

	if (owl_noboot || wdt->wd_type == 1) {
		/* Enable watchdog - prescale=256, watchdog mode=0, enable=1 */
		writel_relaxed(0x0000FF05, wdt->wd_base + TWD_WDOG_CONTROL);
		enable_percpu_irq(wdt->irq, 0);
		if(wdt->wd_type == 1 && panic_timeout == 0)
			panic_timeout = 5;
	} else {
		/* Enable watchdog - prescale=256, watchdog mode=1, enable=1 */
		writel_relaxed(0x0000FF09, wdt->wd_base + TWD_WDOG_CONTROL);
	}
}

static void __owl_wdt_stop(struct watchdog_device *wdd)
{
	unsigned int val;
	struct owl_wdt *wdt = watchdog_get_drvdata(wdd);
	
	dev_info(wdd->dev, "disabling watchdog\n");

	val = readl_relaxed(wdt->wd_base + TWD_WDOG_CONTROL);
	if(val & 0x1) {
		if(val & 0x4) {
			disable_percpu_irq(wdt->irq);
		}
		writel_relaxed(0x12345678, wdt->wd_base + TWD_WDOG_DISABLE);
		writel_relaxed(0x87654321, wdt->wd_base + TWD_WDOG_DISABLE);
		writel_relaxed(0x0, wdt->wd_base + TWD_WDOG_CONTROL);
	}
}

static int owl_wdt_start(struct watchdog_device *wdd)
{
	if(smp_call_function_single(0, (void*)__owl_wdt_start, wdd, 1) != 0)
		BUG();
		
	return 0;
}

static int owl_wdt_ping(struct watchdog_device *wdd)
{
	if(smp_call_function_single(0, (void*)__owl_wdt_keepalive, wdd, 1) != 0)
		BUG();
		
	return 0;
}

static int owl_wdt_stop(struct watchdog_device *wdd)
{
	if(smp_call_function_single(0, (void*)__owl_wdt_stop, wdd, 1) != 0)
		BUG();
		
	return 0;
}

static int owl_wdt_set_timeout(struct watchdog_device *wdd, unsigned int t)
{
	if (t < 1 || t > 3600) {
		dev_warn(wdd->dev,
			"owl_margin value must be 1<=x<=3600, using %d\n",
			owl_margin);
		return -EINVAL;
	}
	
	dev_info(wdd->dev, "change watchdog timeout from %ds to %ds\n", wdd->timeout, t);
	wdd->timeout = t;
	return 0;
}

static struct watchdog_info owl_wdt_info = {
	.identity = "OWL Watchdog",
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
};

static struct watchdog_ops owl_wdt_ops = {
	.owner = THIS_MODULE,
	.start = owl_wdt_start,
	.stop = owl_wdt_stop,
	.ping = owl_wdt_ping,
	.set_timeout = owl_wdt_set_timeout,
};

static struct watchdog_device owl_wdt_wdd= {
	.info = &owl_wdt_info,
	.ops = &owl_wdt_ops,
	.min_timeout = 1,
	.max_timeout = 3600,
};

static void periph_clk_notify_on_cpu0(struct clk_notifier_data *cnd)
{
	unsigned long long newcount;
	unsigned int oldcount;
	struct owl_wdt *wdt = watchdog_get_drvdata(&owl_wdt_wdd);

	spin_lock(&wdt_lock);
	if(readl_relaxed(wdt->wd_base + TWD_WDOG_CONTROL) & 0x1) {
		oldcount =  readl_relaxed(wdt->wd_base + TWD_WDOG_COUNTER);
		newcount = (unsigned long long)oldcount * cnd->new_rate;
		do_div(newcount, cnd->old_rate);
		writel_relaxed((unsigned int)newcount, wdt->wd_base + TWD_WDOG_LOAD);
	}
	spin_unlock(&wdt_lock);
}

static int periph_clk_notify(struct notifier_block *nb, unsigned long action, void *data)
{
	struct clk_notifier_data *cnd = data;
	
	if (action == POST_RATE_CHANGE && cnd->new_rate < cnd->old_rate) {
		if(smp_call_function_single(0, (void*)periph_clk_notify_on_cpu0, data, 1) != 0)
			BUG();
	} else if(action == PRE_RATE_CHANGE && cnd->new_rate > cnd->old_rate) {
		if(smp_call_function_single(0, (void*)periph_clk_notify_on_cpu0, data, 1) != 0)
			BUG();
	}
	
	return NOTIFY_OK;
}

/*
 *	This is the interrupt handler.  Note that we only use this
 *	in testing mode, so don't actually do a reboot here.
 */
static irqreturn_t owl_wdt_fire(int irq, void *arg)
{
	struct owl_wdt *wdt = watchdog_get_drvdata(&owl_wdt_wdd);

	/* Check it really was our interrupt */
	if (readl_relaxed(wdt->wd_base + TWD_WDOG_INTSTAT)) {
		if(wdt->wd_type == 1) {
			pr_emerg("watchdog - Rebooting now\n");
			arm_pm_restart('h', NULL);
		} else {
			pr_emerg("watchdog - Reboot ignored\n");
		}
		/* Clear the interrupt on the watchdog */
		__owl_wdt_stop(&owl_wdt_wdd);
		writel_relaxed(1, wdt->wd_base + TWD_WDOG_INTSTAT);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

static int _get_watchdog_type(struct platform_device *pdev)
{
	const char *type;
	
	if (of_property_read_string(pdev->dev.of_node, "wd_type", &type) == 0) {
		if(strcmp(type, "soft") == 0) {
			dev_info(&pdev->dev, "watchdog soft mode\n");
			return 1;
		}
	}

	dev_info(&pdev->dev, "watchdog hard mode\n");
	return 0;
}

static int owl_wdt_probe(struct platform_device *pdev)
{
	struct owl_wdt *wdt;
	struct resource *res;
	int ret;

	wdt = devm_kzalloc(&pdev->dev, sizeof(struct owl_wdt), GFP_KERNEL);
	if (!wdt) {
		dev_err(&pdev->dev, "alloc owl_wdt failed\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "get resource failed\n");
		return -ENODEV;
	}
	wdt->wd_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!wdt->wd_base) {
		dev_err(&pdev->dev, "ioremap failed\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(&pdev->dev, "get resource failed\n");
		return -ENODEV;
	}
	wdt->cmu_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!wdt->cmu_base) {
		dev_err(&pdev->dev, "ioremap failed\n");
		return -ENOMEM;
	}
		
	wdt->irq = platform_get_irq(pdev, 0);
	if (wdt->irq >= 0) {
		ret = request_percpu_irq(wdt->irq, owl_wdt_fire, "owl_wdt", wdt);
		if (ret) {
			dev_err(&pdev->dev,
				"cannot register IRQ%d for watchdog\n",
				wdt->irq);
			return ret;
		}
	}
	
	if (owl_margin < 1 || owl_margin > 3600) {
		owl_margin = TIMER_MARGIN;
		dev_warn(&pdev->dev,
			"owl_margin value must be 1<=x<=3600, using %d\n",
			owl_margin);
	}

	owl_wdt_wdd.timeout = owl_margin;
	watchdog_set_drvdata(&owl_wdt_wdd, wdt);
	watchdog_set_nowayout(&owl_wdt_wdd, nowayout);

	owl_powergate_power_on(OWL_POWERGATE_VDE);
	
	wdt->periph_clk = clk_get_sys(NULL, CLKNAME_PERIPH_CLK); 
	if (IS_ERR(wdt->periph_clk)){
		free_percpu_irq(wdt->irq, wdt);
		dev_err(&pdev->dev,
			"clk_get_sys(%s) failed\n",	CLKNAME_PERIPH_CLK);
		return PTR_ERR(wdt->periph_clk); 		 
	} 
	wdt->nb.notifier_call = periph_clk_notify;
	clk_notifier_register(wdt->periph_clk, &wdt->nb);

	wdt->wd_type = _get_watchdog_type(pdev);
	
	ret = watchdog_register_device(&owl_wdt_wdd);
	if (ret) {
		clk_notifier_unregister(wdt->periph_clk, &wdt->nb);
		clk_put(wdt->periph_clk);
		free_percpu_irq(wdt->irq, wdt);
		dev_err(&pdev->dev, "register failed\n");
		return ret;
	}

	return 0;
}

static int owl_wdt_remove(struct platform_device *pdev)
{
	struct owl_wdt *wdt = watchdog_get_drvdata(&owl_wdt_wdd);

	clk_notifier_unregister(wdt->periph_clk, &wdt->nb);
	clk_put(wdt->periph_clk);
	free_percpu_irq(wdt->irq, wdt);
	watchdog_unregister_device(&owl_wdt_wdd);
	owl_powergate_power_off(OWL_POWERGATE_VDE);
	return 0;
}

static struct of_device_id owl_wdt_id_table[] = {
	{ .compatible = "actions,owl-wdt" },
	{ .compatible = "actions,atm7059-wdt" },
	{ },
};
MODULE_DEVICE_TABLE(of, owl_wdt_id_table);

static struct platform_driver owl_wdt_driver = {
	.probe		= owl_wdt_probe,
	.remove		= owl_wdt_remove,
	.driver		= {
		.name	= "owl_wdt",
		.owner	= THIS_MODULE,
		.of_match_table	= owl_wdt_id_table,
	},
};

module_platform_driver(owl_wdt_driver);

MODULE_AUTHOR("Actions Semi Inc.");
MODULE_DESCRIPTION("watchdog driver for OWL SOC");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
