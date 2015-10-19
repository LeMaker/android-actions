/* kernel/power/earlysuspend.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/earlysuspend.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/rtc.h>
#include <linux/syscalls.h> /* sys_sync */
#include <linux/wakelock.h>
#include <linux/workqueue.h>

#include "power.h"

enum {
	DEBUG_USER_STATE = 1U << 0,
	DEBUG_SUSPEND = 1U << 2,
	DEBUG_VERBOSE = 1U << 3,
};
static int debug_mask = DEBUG_USER_STATE;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static DEFINE_MUTEX(early_suspend_lock);
static LIST_HEAD(early_suspend_handlers);
static void sync_system(struct work_struct *work);
static void early_suspend(struct work_struct *work);
static void late_resume(struct work_struct *work);
static DECLARE_WORK(sync_system_work, sync_system);
static DECLARE_WORK(early_suspend_work, early_suspend);
static DECLARE_WORK(late_resume_work, late_resume);
struct wake_lock sys_sync_wake_lock;
struct workqueue_struct *suspend_work_queue;
struct workqueue_struct *sys_sync_work_queue;
suspend_state_t requested_suspend_state = PM_SUSPEND_MEM;
enum {
	SUSPEND_REQUESTED = 0x1,
	SUSPENDED = 0x2,
	SUSPEND_REQUESTED_AND_SUSPENDED = SUSPEND_REQUESTED | SUSPENDED,
};
static int state;

void register_early_suspend(struct early_suspend *handler)
{
	struct list_head *pos;

	mutex_lock(&early_suspend_lock);
	list_for_each(pos, &early_suspend_handlers) {
		struct early_suspend *e;
		e = list_entry(pos, struct early_suspend, link);
		if (e->level > handler->level)
			break;
	}
	list_add_tail(&handler->link, pos);
	if ((state & SUSPENDED) && handler->suspend)
		handler->suspend(handler);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(register_early_suspend);

void unregister_early_suspend(struct early_suspend *handler)
{
	mutex_lock(&early_suspend_lock);
	list_del(&handler->link);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(unregister_early_suspend);

static void sync_system(struct work_struct *work)
{
	printk("%s +\n", __func__);
    wake_lock(&sys_sync_wake_lock);
	sys_sync();
    wake_unlock(&sys_sync_wake_lock);
	printk("%s -\n", __func__);
}
static void early_suspend(struct work_struct *work)
{
	struct early_suspend *pos;
	int abort = 0;

	mutex_lock(&early_suspend_lock);
	if (state == SUSPEND_REQUESTED)
		state |= SUSPENDED;
	else
		abort = 1;

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			printk("early_suspend: abort, state %d\n", state);
		mutex_unlock(&early_suspend_lock);
		goto abort;
	}

	if (debug_mask & DEBUG_SUSPEND)
		printk("early_suspend: call handlers\n");
	list_for_each_entry(pos, &early_suspend_handlers, link) {
		if (pos->suspend != NULL) {
			if (debug_mask & DEBUG_VERBOSE)
				printk("early_suspend: calling %pf\n", pos->suspend);
			pos->suspend(pos);
		}
	}
	mutex_unlock(&early_suspend_lock);

	if (debug_mask & DEBUG_SUSPEND)
		printk("early_suspend: sync\n");

	/* sys_sync(); */
	queue_work(sys_sync_work_queue, &sync_system_work);

abort:
#ifdef CONFIG_PLATFORM_UBUNTU
	pm_suspend(PM_SUSPEND_MEM);
	request_suspend_state(PM_SUSPEND_ON);
#else
	if (state == SUSPEND_REQUESTED_AND_SUSPENDED)
		pm_autosleep_set_state(PM_SUSPEND_MEM);
#endif
}

static void late_resume(struct work_struct *work)
{
	struct early_suspend *pos;
	int abort = 0;

	mutex_lock(&early_suspend_lock);
	if (state == SUSPENDED)
		state &= ~SUSPENDED;
	else
		abort = 1;

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			printk("late_resume: abort, state %d\n", state);
		goto abort;
	}
	if (debug_mask & DEBUG_SUSPEND)
		printk("late_resume: call handlers\n");
	list_for_each_entry_reverse(pos, &early_suspend_handlers, link) {
		if (pos->resume != NULL) {
			if (debug_mask & DEBUG_VERBOSE)
				printk("late_resume: calling %pf\n", pos->resume);

			pos->resume(pos);
		}
	}
	if (debug_mask & DEBUG_SUSPEND)
		printk("late_resume: done\n");
abort:
	mutex_unlock(&early_suspend_lock);
}

void request_suspend_state(suspend_state_t new_state)
{
/* 	unsigned long irqflags; */
	int old_sleep;

	mutex_lock(&early_suspend_lock);
	old_sleep = state & SUSPEND_REQUESTED;
	if (debug_mask & DEBUG_USER_STATE) {
		struct timespec ts;
		struct rtc_time tm;
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &tm);
		printk("request_suspend_state: %s (%d->%d) at %lld "
			"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n",
			new_state != PM_SUSPEND_ON ? "sleep" : "wakeup",
			requested_suspend_state, new_state,
			ktime_to_ns(ktime_get()),
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
	}
	if (!old_sleep && new_state != PM_SUSPEND_ON) {
		state |= SUSPEND_REQUESTED;
		queue_work(suspend_work_queue, &early_suspend_work);
	} else if (old_sleep && new_state == PM_SUSPEND_ON) {
		state &= ~SUSPEND_REQUESTED;
		//wake_lock(&main_wake_lock);
		queue_work(suspend_work_queue, &late_resume_work);
	}
	requested_suspend_state = new_state;
	mutex_unlock(&early_suspend_lock);
}

suspend_state_t get_suspend_state(void)
{
	return requested_suspend_state;
}

static int __init wakelocks_init(void)
{
    int ret;
   
    wake_lock_init(&sys_sync_wake_lock, WAKE_LOCK_SUSPEND, "sys_sync");

    sys_sync_work_queue = create_singlethread_workqueue("fs_sync");
    if (sys_sync_work_queue == NULL) {
        pr_err("[wakelocks_init] fs_sync workqueue create failed\n");
    }

    suspend_work_queue = create_singlethread_workqueue("suspend");
    if (suspend_work_queue == NULL) {
        ret = -ENOMEM;
        goto err_suspend_work_queue;
    }
    return 0;

err_suspend_work_queue:

    return ret;
}

static void  __exit wakelocks_exit(void)
{
    destroy_workqueue(suspend_work_queue);
    destroy_workqueue(sys_sync_work_queue);
}

core_initcall(wakelocks_init);
module_exit(wakelocks_exit);
