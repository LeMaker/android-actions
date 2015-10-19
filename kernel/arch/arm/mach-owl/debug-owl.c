/*
 * arch/arm/mach-owl/debug-owl.c
 *
 * special debug support for Actions SOC
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/kallsyms.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/debug.h>


/* FIXME: move symbol from adfus to kernel temporarily */
typedef unsigned int (*func_t)(unsigned int *, void *);

func_t AdfuUpdateMbrFromPhyToUsr;
EXPORT_SYMBOL(AdfuUpdateMbrFromPhyToUsr);

typedef void (*func_t1)(void);
func_t1 adfu_flush_nand_cache;
EXPORT_SYMBOL(adfu_flush_nand_cache);

typedef int (*func_t4)(unsigned long, unsigned long , void *, void *);

func_t4 adfus_nand_read;
func_t4 adfus_nand_write;
EXPORT_SYMBOL(adfus_nand_read);
EXPORT_SYMBOL(adfus_nand_write);

extern void show_state_filter(unsigned long state_filter);
void owl_show_state(void)
{
	show_state_filter(0);
}
EXPORT_SYMBOL(owl_show_state);

typedef struct
{
    unsigned int magic;
    void *caller;
    struct task_struct  *tsk;
    struct timer_list   timer;
}watch_dog_data_t;

static void owl_watchdog_callback(unsigned long data)
{
    watch_dog_data_t *watch_dog_data = (watch_dog_data_t *)data;
    
    console_verbose();
#ifdef CONFIG_KALLSYMS
    printk("watchdog set from [<%08lx>] (%pS)\n", (unsigned long)watch_dog_data->caller, watch_dog_data->caller);
#else
    printk("watchdog set from [<%08lx>]\n", (unsigned long)watch_dog_data->caller);
#endif
    show_stack(watch_dog_data->tsk, NULL);

    printk("dump all task stack:\n");
    owl_show_state();
}

/**
 * owl_watchdog_start - start watchdog until timeout
 * @timeout: timeout value in jiffies
 * @callback: when timeout happened, this function will be called
 *
 * Make the current task run until @timeout jiffies have
 * elapsed, and call owl_watchdog_callback defaultly when it happend.
 *
 * You can set the callback as yours by passing param callback with not NULL.
 *
 * return value is the handle of watchdog
 */
void *owl_watchdog_start(int timeout, void (*callback)(unsigned long data), unsigned long data)
{
    watch_dog_data_t *watch_dog_data = kmalloc(sizeof(watch_dog_data_t), GFP_KERNEL);
    if(!watch_dog_data)
        return NULL;
    
    watch_dog_data->magic = 0x77617463;
    watch_dog_data->tsk = current;
	watch_dog_data->caller = __builtin_return_address(0);
    init_timer(&watch_dog_data->timer);
    if(callback)
    {
        watch_dog_data->timer.function = callback;
        watch_dog_data->timer.data = data;
    }
    else
    {
        watch_dog_data->timer.function = owl_watchdog_callback;
        watch_dog_data->timer.data = (unsigned long)watch_dog_data;
    }
    watch_dog_data->timer.expires = jiffies + timeout;
    add_timer(&watch_dog_data->timer);
    return watch_dog_data;
}

void owl_watchdog_stop(void *owl_watchdog)
{
    watch_dog_data_t *watch_dog_data = owl_watchdog;
	
    if(watch_dog_data && watch_dog_data->magic == 0x77617463)
    {
        watch_dog_data->magic = 0;
        del_timer_sync(&watch_dog_data->timer);
        kfree(watch_dog_data);
    }
}

#ifdef CONFIG_OWL_DEBUG_IRQ_STACK
static void __owl_debug_show_regs(struct pt_regs *regs)
{
	pr_emerg("regs : [<%08lx>]\n", (long)regs);
	pr_emerg("pc : [<%08lx>]    lr : [<%08lx>]    psr: %08lx\n"
		   "sp : %08lx  ip : %08lx  fp : %08lx\n",
		regs->ARM_pc, regs->ARM_lr, regs->ARM_cpsr,
		regs->ARM_sp, regs->ARM_ip, regs->ARM_fp);
	pr_emerg("r10: %08lx  r9 : %08lx  r8 : %08lx\n",
		regs->ARM_r10, regs->ARM_r9,
		regs->ARM_r8);
	pr_emerg("r7 : %08lx  r6 : %08lx  r5 : %08lx  r4 : %08lx\n",
		regs->ARM_r7, regs->ARM_r6,
		regs->ARM_r5, regs->ARM_r4);
	pr_emerg("r3 : %08lx  r2 : %08lx  r1 : %08lx  r0 : %08lx\n",
		regs->ARM_r3, regs->ARM_r2,
		regs->ARM_r1, regs->ARM_r0);
}

static void hbp_handler(struct perf_event *bp,
			       struct perf_sample_data *data,
			       struct pt_regs *regs)
{
	pr_emerg("change here\n");
	__owl_debug_show_regs(regs);
}

void hw_break_set(void **hbp, void *hbp_addr)
{
	struct perf_event_attr attr;
	
	hw_breakpoint_init(&attr);
	attr.bp_addr = (unsigned long)hbp_addr;
	attr.bp_len = HW_BREAKPOINT_LEN_4;
	attr.bp_type = HW_BREAKPOINT_W;// | HW_BREAKPOINT_R;

	*hbp = register_wide_hw_breakpoint(&attr, hbp_handler, NULL);
	if (IS_ERR((void __force *)hbp)) {
		printk("hw_break_init failed\n");
	}
}
EXPORT_SYMBOL(hw_break_set);

void hw_break_unset(void **hbp)
{
	if (!IS_ERR((void __force *)hbp)) {
		unregister_wide_hw_breakpoint(*hbp);
		*hbp = NULL;
	}
}
EXPORT_SYMBOL(hw_break_unset);

static int owl_irqstack_debug = 0;

void owl_debug_save_irqstack(struct pt_regs *regs)
{
	struct thread_info *thread_info;
	if(!owl_irqstack_debug)
		return;

	thread_info = current_thread_info();
	if(thread_info->regs_init == 0x12345678)
		return;
	thread_info->regs_init = 0x12345678;
	
	thread_info->regs = *regs;
}

void owl_debug_check_irqstack(struct pt_regs *regs)
{
	struct thread_info *thread_info;
	struct pt_regs *regs_save;
	if(!owl_irqstack_debug)
		return;

	thread_info = current_thread_info();
	if(thread_info->regs_init != 0x12345678)
		return;
	thread_info->regs_init = 0;
	
	regs_save = &thread_info->regs;
	if(memcmp(regs_save, regs, sizeof(struct pt_regs)) != 0)
	{
		pr_emerg("irq stack diff from %p\n", __builtin_return_address(0));
		pr_emerg("CPU: %d PID: %d Comm: %.20s\n",
			   raw_smp_processor_id(), current->pid, current->comm);
		pr_emerg("irq stack org:\n");
		__owl_debug_show_regs(regs_save);
		pr_emerg("irq stack now:\n");
		__owl_debug_show_regs(regs);
	}
}

void owl_debug_clear_irqstack(void)
{
	struct thread_info *thread_info;
	if(!owl_irqstack_debug)
		return;
	
	thread_info = current_thread_info();
	thread_info->regs_init = 0;
}

static int owl_irqstack_debug_config(char *str)
{
	if (!str)
		return -EINVAL;
	if (strcmp(str, "off") == 0)
	{
		owl_irqstack_debug = 0;
		printk("owl_irqstack_debug off\n");
	}
	else if (strcmp(str, "on") == 0)
	{
		owl_irqstack_debug = 1;
		printk("owl_irqstack_debug on\n");
	}
	else
		return -EINVAL;
	return 0;
}
early_param("owl_irqstack_debug", owl_irqstack_debug_config);

#endif

void owl_dump_mem(void *startaddr, int size, void *showaddr, int show_bytes)
{
    int i, count, count_per_line;
    void *addr = startaddr;

    if ((show_bytes != 1) && (show_bytes != 2) && (show_bytes != 4))
    {
        printk("dump_mem: not support mode\n");
        return;
    }

    if (((int) startaddr & (show_bytes - 1)) || (size & (show_bytes - 1)))
    {
        printk("dump_mem: startaddr must be aligned by %d bytes!\n", show_bytes);
        return;
    }

    count = size / show_bytes;
    count_per_line = 16 / show_bytes; // 16 bytes per line

    printk("startaddr 0x%p, size 0x%x\n",
        startaddr, size);

    i = 0;
    while (i < count)
    {
        if ((i % count_per_line) == 0) {
            if (i != 0)
                printk("\n");

            printk("%08x: ", (unsigned int)showaddr + ((i / count_per_line) * 16));
        }
        switch (show_bytes) {
        case 1:
            printk("%02x ", *((unsigned char *) addr + i));
            break;
        case 2:
            printk("%04x ", *((unsigned short *) addr + i));
            break;
        case 4:
            printk("%08x ", *((unsigned int *) addr + i));
            break;
        default:
            printk("dump_mem: not support mode\n");
            return;
        }

        i++;
    }
    printk("\n");
}

EXPORT_SYMBOL(owl_dump_mem);

void owl_dump_reg(unsigned int addr, int size)
{
    int count, i = 0;

    if ((addr & 3) || (size & 3)) {
        printk("owl_dump_reg: startaddr must be aligned by 4 bytes!\n");
        return;
    }

    count = size / 4;

    printk("addr 0x%08x, size 0x%x\n", addr, size);

    while (i < count) {
        if ((i % 4) == 0) {
            if (i != 0)
                printk("\n");

            printk("%08x: ", (unsigned int)addr + i * 4);
        }

        printk("%08x ", act_readl((unsigned int)addr + i * 4));

        i++;
    }
    printk("\n");
}

EXPORT_SYMBOL(owl_dump_reg);

#ifdef CONFIG_DEBUG_FS

static ssize_t reg_read(struct file *filp, char __user *buffer,
        size_t count, loff_t *ppos)
{
    printk("read/write a register:\n");
    printk("  read : echo 0xb01c0000 > reg\n");
    printk("  write: echo 0xb01c00000=0x12345678 > reg\n");

    return 0;
}

static ssize_t reg_write(struct file *filp, const char __user *buffer,
        size_t count, loff_t *ppos)
{
    unsigned int reg, read_val, reg_val;
    char buf[32];
    char *end_ptr;
    int write = 0;

    if (*ppos != 0)
        return -EINVAL;
    if (count > 32)
        return -EINVAL;
    if (copy_from_user(buf, buffer, count))
        return -EFAULT;

    *ppos += count;

    reg = simple_strtoul(buf, &end_ptr, 16);

    if ((reg & 0x3) || (reg < OWL_PA_REG_BASE) ||
        (reg >= OWL_PA_REG_BASE + 6 * SZ_1M)) {
        printk("invalid register address\n");
        return -EINVAL;
    }

    if ((buf == end_ptr) )
        goto out;

    read_val = act_readl(reg);
    printk("[0x%08x]: 0x%08x\n", reg, read_val);

    if (*end_ptr++ == '=') {
        reg_val = simple_strtoul(end_ptr, NULL, 16);
        write = 1;
    }

    if (write) {
        act_writel(reg_val, reg);
        printk("[0x%08x] <- 0x%08x\n", reg, reg_val);
    }

out:
    return count;
}

static int reg_open(struct inode *inode, struct file *filp)
{
    filp->private_data = inode->i_private;
    return 0;
}

static struct file_operations reg_fops = {
    .open = reg_open,
    .read = reg_read,
    .write = reg_write,
};


static ssize_t regs_read(struct file *filp, char __user *buffer,
        size_t count, loff_t *ppos)
{
    printk("read registers:\n");
    printk("  echo regs_start,regs_len > regs\n");	
    printk("  echo 0xb01c0000,0x100 > regs\n");

    return 0;
}

static ssize_t regs_write(struct file *filp, const char __user *buffer,
        size_t count, loff_t *ppos)
{
    unsigned int reg, reg_len;
    char buf[32];
    char *end_ptr;

    if (*ppos != 0)
        return -EINVAL;
    if (count > 32)
        return -EINVAL;
    if (copy_from_user(buf, buffer, count))
        return -EFAULT;

    *ppos += count;

    reg = simple_strtoul(buf, &end_ptr, 16);

    if ((reg & 0x3) || (reg < OWL_PA_REG_BASE) ||
        (reg >= OWL_PA_REG_BASE + 6 * SZ_1M)) {
        printk("invalid register address\n");
        return -EINVAL;
    }

    if ((buf == end_ptr))
        goto out;

    if (*end_ptr++ == ',') {
        reg_len = simple_strtoul(end_ptr, NULL, 16);
		owl_dump_reg(reg, reg_len);
    }

out:
    return count;
}

static int regs_open(struct inode *inode, struct file *filp)
{
    filp->private_data = inode->i_private;
    return 0;
}

static struct file_operations regs_fops = {
    .open = regs_open,
    .read = regs_read,
    .write = regs_write,
};

int __init owl_debug_init(void)
{
    struct dentry *dir;
    struct dentry *d;

    dir = debugfs_create_dir("owl", NULL);
    if (!dir)
        return -ENOMEM;

    d = debugfs_create_file("reg", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, dir, NULL,
        &reg_fops);
    if (!d)
        return -ENOMEM;

    d = debugfs_create_file("regs", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP, dir, NULL,
        &regs_fops);
    if (!d)
        return -ENOMEM;		
    return 0;
}

arch_initcall(owl_debug_init);

#endif /* CONFIG_DEBUG_FS */
