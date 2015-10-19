/*
 * arch/arm/mach-owl/powergate-owl.c
 *
 * powergate support for Actions SOC
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#if 0
#define DEBUG
#endif

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/of_platform.h>

#include <mach/hardware.h>
#include <mach/powergate.h>
#include <mach/module-owl.h>

#define MAX_RESET_ID_SIZE 6
#define NO_ACK_ID 0xffffffff

static struct mutex powergate_mutex;        /* mutex of one powergate for multi muser */
static int owl_powergate_maxid;
static unsigned int sps_pg_ctl;

struct owl_powergate_info {
	char name[16];             /* name of powergate */
	unsigned int reset_id[MAX_RESET_ID_SIZE];
	unsigned int pwr_id;
	unsigned int ack_id;
	int count;                 /* power on count */
	int init_power_off;        /* power off at boot init stage */
	int use_mutex;             /* need use mutex? */
};

static struct owl_powergate_info powergate_info[] = {
	[OWL_POWERGATE_CPU2] = {
		.name = "cpu2",
		.pwr_id = 5,
		.ack_id = 21,
		/* for CPU2, init_power_off shoud be 0 */
		.init_power_off = 0,
		.use_mutex = 0,
		.count = 0,
	},

	[OWL_POWERGATE_CPU3] = {
		.name = "cpu3",
		.pwr_id = 6,
		.ack_id = 22,
		/* for CPU3, init_power_off shoud be 0 */
		.init_power_off = 0,
		.use_mutex = 0,
		.count = 0,
	},

	[OWL_POWERGATE_GPU3D] = {
		.name = "gpu3d",
		.reset_id = {MOD_ID_GPU3D},
		.pwr_id = 3,
		.ack_id = NO_ACK_ID,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},

	[OWL_POWERGATE_VCE_BISP] = {
		.name = "vce/bisp",
		.reset_id = {MOD_ID_VCE, MOD_ID_BISP},
		.pwr_id = 1,
		.ack_id = 17,
		/* for VCE_BISP, init_power_off shoud be 1 */
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},

	[OWL_POWERGATE_VDE] = {
		.name = "vde",
		.reset_id = {MOD_ID_VDE},
		.pwr_id = 0,
		.ack_id = 16,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},

	/*atm7059 avaliabe only*/
	[OWL_POWERGATE_USB2_0] = {
		.name = "usb2_0",
		.reset_id = {MOD_ID_USB2_0},
		.pwr_id = 11,
		.ack_id = 15,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},
	
	[OWL_POWERGATE_USB2_1] = {
		.name = "usb2_1",
		.reset_id = {MOD_ID_USB2_1},
		.pwr_id = 2,
		.ack_id = 18,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},
	
	[OWL_POWERGATE_USB3] = {
		.name = "usb3",
		.reset_id = {MOD_ID_USB3},
		.pwr_id = 10,
		.ack_id = 14,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},
	
	[OWL_POWERGATE_DS] = {
		.name = "ds",
		.reset_id = {MOD_ID_DE, MOD_ID_LCD, MOD_ID_HDMI, MOD_ID_TVOUT, MOD_ID_DSI},
		.pwr_id = 9,
		.ack_id = 13,
		.init_power_off = 0,
		.use_mutex = 1,
		.count = 0,
	},
	
	[OWL_POWERGATE_DMA] = {
		.name = "dma",
		.reset_id = {MOD_ID_DMAC},
		.pwr_id = 8,
		.ack_id = 12,
		.init_power_off = 1,
		.use_mutex = 1,
		.count = 0,
	},
	
};

static unsigned int owl_cpu_domains[] = {
	0xffffffff,
	0xffffffff,
	OWL_POWERGATE_CPU2,
	OWL_POWERGATE_CPU3,
};

static DEFINE_SPINLOCK(owl_powergate_lock);

static int owl_powergate_set(enum owl_powergate_id id, bool on)
{
	struct owl_powergate_info *pgi = &powergate_info[id];
	bool ack_is_on;
	unsigned long val, flags;
	int timeout, i, reset_id;
	
	if(id < OWL_POWERGATE_USB2_0)
		pr_debug("[PowerGate] name: '%s', on: %d, before SPS_PG_CTL: 0x%x\n",
			pgi->name, on, act_readl(SPS_PG_CTL));
	else
		printk("[PowerGate] name: '%s', on: %d, before SPS_PG_CTL: 0x%x\n",
			pgi->name, on, act_readl(SPS_PG_CTL));

	spin_lock_irqsave(&owl_powergate_lock, flags);
		
	if (pgi->ack_id != NO_ACK_ID) {
		ack_is_on = (act_readl(SPS_PG_CTL) & (1 << pgi->ack_id));

		if (ack_is_on == on) {
			spin_unlock_irqrestore(&owl_powergate_lock, flags);
			return 0;
		}
	}

	/* assert modules reset before poweron */
	if (on) {
		if (id == OWL_POWERGATE_CPU2) {
			/* core reset */
			val = act_readl(CMU_CORECTL);
			val &= ~(1 << 6);
			act_writel(val, CMU_CORECTL);
		} else if (id == OWL_POWERGATE_CPU3) {
			/* core reset */
			val = act_readl(CMU_CORECTL);
			val &= ~(1 << 7);
			act_writel(val, CMU_CORECTL);
		} else {
			for (i = 0; i < MAX_RESET_ID_SIZE; i++) {
				reset_id = pgi->reset_id[i];
				if (reset_id != MOD_ID_ROOT)
					owl_module_reset_assert(reset_id);
			}
		}
	}

	val = act_readl(SPS_PG_CTL);
	if (on)
		val |= (1 << pgi->pwr_id);
	else
		val &= ~(1 << pgi->pwr_id);
	act_writel(val, SPS_PG_CTL);

	if (on) {
		timeout = 5000;  /* 5ms */
		while (timeout > 0 && !owl_powergate_is_powered(id)) {
			udelay(50);
			timeout -= 50;
		}
		if (timeout <= 0) {
			pr_err("[PowerGate] enable power for id %d timeout\n",
			       id);
		}
		udelay(10);
		
		/* deasert modules reset after poweron */
		if (id == OWL_POWERGATE_CPU2) {
			/* clk en */
			val = act_readl(CMU_CORECTL);
			val |= (1 << 2);
			act_writel(val, CMU_CORECTL);
			/* core reset */
			val = act_readl(CMU_CORECTL);
			val |= (1 << 6);
			act_writel(val, CMU_CORECTL);
		} else if (id == OWL_POWERGATE_CPU3) {
			/* clk en */
			val = act_readl(CMU_CORECTL);
			val |= (1 << 3);
			act_writel(val, CMU_CORECTL);
			/* core reset */
			val = act_readl(CMU_CORECTL);
			val |= (1 << 7);
			act_writel(val, CMU_CORECTL);
		} else {
			for (i = 0; i < MAX_RESET_ID_SIZE; i++) {
				reset_id = pgi->reset_id[i];
				if (reset_id != MOD_ID_ROOT) {
					module_clk_enable(reset_id);
					owl_module_reset_deassert(reset_id);
				}
			}
		}
	}

	spin_unlock_irqrestore(&owl_powergate_lock, flags);

	if(id < OWL_POWERGATE_USB2_0)
		pr_debug("[PowerGate] name: '%s', on: %d, after SPS_PG_CTL: 0x%x\n",
			pgi->name, on, act_readl(SPS_PG_CTL));
	else
		printk("[PowerGate] name: '%s', on: %d, after SPS_PG_CTL: 0x%x\n",
			pgi->name, on, act_readl(SPS_PG_CTL));

	return 0;
}

int owl_powergate_power_on(enum owl_powergate_id id)
{
	struct owl_powergate_info *pgi;
	int ret;

	if (id < 0 || id >= owl_powergate_maxid)
		return -EINVAL;

	pgi = &powergate_info[id];

	pr_debug("[PowerGate] %s(): '%s', count %d\n",
		__func__, pgi->name, pgi->count);

	if (pgi->use_mutex)
		mutex_lock(&powergate_mutex);
	pgi->count++;

	if (pgi->ack_id != NO_ACK_ID &&
		owl_powergate_is_powered(id) > 0) {
		if (pgi->use_mutex) {
			pr_err("[PowerGate] '%s', skip power on, count %d\n",
				pgi->name, pgi->count);

			mutex_unlock(&powergate_mutex);
		}
		return 0;
	}

	ret = owl_powergate_set(id, true);

	if (pgi->use_mutex)
		mutex_unlock(&powergate_mutex);

	return ret;
}
EXPORT_SYMBOL(owl_powergate_power_on);

int owl_powergate_power_off(enum owl_powergate_id id)
{
	struct owl_powergate_info *pgi;
	int ret = 0;

	if (id < 0 || id >= owl_powergate_maxid)
		return -EINVAL;

	pgi = &powergate_info[id];

	pr_debug("[PowerGate] %s(): '%s', count %d\n",
		__func__, pgi->name, pgi->count);

	if (pgi->use_mutex)
		mutex_lock(&powergate_mutex);

	if (WARN(pgi->count <= 0,
		"unbalanced power off for %s\n", pgi->name)) {
		if (pgi->use_mutex)
			mutex_unlock(&powergate_mutex);
		return -EIO;
	}

	pgi->count--;
	if (pgi->count == 0) {
		pr_debug("[PowerGate] '%s', count is 0, real power off\n",
			pgi->name);

		ret = owl_powergate_set(id, false);
	}

	if (pgi->use_mutex)
		mutex_unlock(&powergate_mutex);

	return ret;
}
EXPORT_SYMBOL(owl_powergate_power_off);

int owl_powergate_is_powered(enum owl_powergate_id id)
{
	struct owl_powergate_info *pgi;
	u32 status;

	if (id < 0 || id >= owl_powergate_maxid)
		return -EINVAL;

	pgi = &powergate_info[id];
	if (pgi->ack_id == NO_ACK_ID)
		return 1;

	status = act_readl(SPS_PG_CTL) & (1 << pgi->ack_id);

	pr_debug("[PowerGate] %s(): '%s', status %d\n",
		__func__, pgi->name, !!status);

	return !!status;
}
EXPORT_SYMBOL(owl_powergate_is_powered);

int owl_powergate_suspend(void)
{
	struct owl_powergate_info *pgi;
	int i;

	pr_debug("[PowerGate] suspend\n");
	
	sps_pg_ctl = act_readl(SPS_PG_CTL);
	
	for (i = 0; i < owl_powergate_maxid; i++) {
		pgi = &powergate_info[i];

		if (owl_powergate_is_powered(i) > 0) {
			pgi->init_power_off = 0;
		} else {
			pgi->init_power_off = 1;
		}
	}

	return 0;
}

int owl_powergate_resume(void)
{
	struct owl_powergate_info *pgi;
	int i;

	pr_debug("[PowerGate] resume\n");

	for (i = 0; i < owl_powergate_maxid; i++) {
		pgi = &powergate_info[i];

		if (pgi->init_power_off == 0 && owl_powergate_is_powered(i) == 0) {
			owl_powergate_set(i, true);
		}
	}
	
	act_writel(sps_pg_ctl, SPS_PG_CTL);

	return 0;
}

int owl_cpu_powergate_id(int cpuid)
{
	if (cpuid > 1 && cpuid < ARRAY_SIZE(owl_cpu_domains))
		return owl_cpu_domains[cpuid];

	return -EINVAL;
}

void owl_powergate_earlyinit(void)
{
	if (of_machine_is_compatible("actions,atm7059a"))
		owl_powergate_maxid = OWL_POWERGATE_MAXID;
	else
		owl_powergate_maxid = OWL_POWERGATE_VDE + 1;
}

#ifdef CONFIG_DEBUG_FS

static int powergate_show(struct seq_file *s, void *data)
{
	struct owl_powergate_info *pgi;
	int i;

	seq_printf(s, " powergate powered\n");
	seq_printf(s, "------------------\n");

	seq_printf(s, "     name     status    count\n");

	for (i = 0; i < owl_powergate_maxid; i++) {
		pgi = &powergate_info[i];

		seq_printf(s, " %9s %7s %7d\n",
		pgi->name,
			owl_powergate_is_powered(i) ? "on" : "off",
		pgi->count);
	}

	return 0;
}

static int powergate_open(struct inode *inode, struct file *file)
{
	return single_open(file, powergate_show, inode->i_private);
}

static const struct file_operations powergate_fops = {
	.open		= powergate_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init powergate_debugfs_init(void)
{
	struct dentry *dir = NULL;
	struct dentry *d;

	d = debugfs_create_file("powergate", S_IRUGO, dir, NULL,
		&powergate_fops);
	if (!d)
		return -ENOMEM;

	return 0;
}

#else
static int powergate_debugfs_init(void)
{
	return 0;
}
#endif


int owl_powergate_init(void)
{
	struct owl_powergate_info *pgi;
	int i;

	pr_debug("[PowerGate] init\n");
	
	for (i = 0; i < owl_powergate_maxid; i++) {
		pgi = &powergate_info[i];

		if (owl_powergate_is_powered(i) > 0) {
			if (pgi->init_power_off) {
				pr_debug("[PowerGate] %s(): '%s', init off\n",
					__func__, pgi->name);
				/* power off */
				owl_powergate_set(i, false);
				pgi->count = 0;
			} else {
				pgi->count = 1;
			}
		} else {
			pgi->count = 0;
		}

		pr_debug("[PowerGate] %s(): '%s', init count %d\n",
			__func__, pgi->name, pgi->count);
	}
	
	mutex_init(&powergate_mutex);
	powergate_debugfs_init();

	return 0;
}

arch_initcall(owl_powergate_init);
