/* linux/arch/arm/mach-owl/hotplug-owl.c
 *
 * Copyright (c) 2012 actions Electronics Co., Ltd.
 *		http://www.actions-semi.com/
 *
 * gs702a - Dynamic CPU hotpluging
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/cpu.h>
#include <linux/percpu.h>
#include <linux/ktime.h>
#include <linux/tick.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
#include <linux/gpio.h>
#include <linux/cpufreq.h>
#include <linux/moduleparam.h>
#include <linux/cpufreq.h>
#include <linux/earlysuspend.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <asm-generic/cputime.h>
#include <linux/clk.h>

#include <mach/module-owl.h>
#include <mach/clkname.h>
#include <mach/clkname_priv.h>
#include <mach/cpu_map-owl.h>

#define CREATE_TRACE_POINTS
#include <trace/events/stand_hotplug.h>

/*structures*/
struct freq_tran_work {
	struct delayed_work work;
	unsigned int new;
};

struct early_suspend_work_t {
	struct delayed_work work;
	unsigned int flag;
};

struct cpu_time_info {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	unsigned int load;
};

struct cpu_hotplug_info {
	unsigned long nr_running;
	pid_t tgid;
};

struct cpu_hotplug_rat{
	int up_rate;
	int down_rate;
};

enum flag{
	HOTPLUG_NOP,
	HOTPLUG_IN,
	HOTPLUG_OUT
};

enum SUSPEND_STAT{
		UP_CPU1=0x1,
		DOWN_CPU1=0x11,
};


/*consts and macros*/
#define INPUT_MODE_DETECT 0
#define HOTPLUG_CPU_SUM CONFIG_NR_CPUS

#define TRANS_LOAD_H0 (20-5)
#define TRANS_LOAD_L1 (15-5)
#define TRANS_LOAD_H1 30
#define TRANS_LOAD_L2 20
#define TRANS_LOAD_H2 60
#define TRANS_LOAD_L3 45

//#define BOOT_DELAY	60
#define BOOT_DELAY	5

#define CHECK_DELAY_ON	 (.1*HZ * 2)
#define CHECK_DELAY_OFF  (.1*HZ)

#define TRANS_RQ 2
#define TRANS_LOAD_RQ 20

#define CPU_OFF 0
#define CPU_ON  1

#define HOTPLUG_UNLOCKED 0
#define HOTPLUG_LOCKED 1
#define PM_HOTPLUG_DEBUG 0
#define CPULOAD_TABLE (HOTPLUG_CPU_SUM + 1)
#define cputime64_sub(__a, __b)		((__a) - (__b))

/*debug abouts*/
#define PRINT_PLUG_INFO 1
#if PRINT_PLUG_INFO
//static unsigned int g_avg_load = 0;
#endif

#define DBG_PRINT(fmt, ...)\
	if(PM_HOTPLUG_DEBUG)			\
		printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)

#define ASSERT_HOTPLUG(v)\
if (v) printk("%s, %d erro\n", __FILE__, __LINE__)


/*static varables*/
#define MAX_DOWN_COUNT 1
#ifdef MAX_DOWN_COUNT
static int down_count = 0;
#endif

static unsigned int freq_min = 96000;
/* static unsigned int freq_threshold_for_cpuplug = 400000; */
static unsigned int freq_max = 0;
static unsigned int max_performance;

static struct workqueue_struct *hotplug_wq;
static struct delayed_work hotplug_work;

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend_work_t early_suspend_work;
static struct early_suspend early_suspend;
#endif

static struct freq_tran_work freq_trans_works;
static struct delayed_work detecting_freq_work;

static unsigned int fb_lock = 0;
static unsigned int user_lock = 1;
static unsigned int freq_lock = 1;
static unsigned int plug_mask = 0;
static unsigned int plug_test = 0;
#if INPUT_MODE_DETECT
static unsigned long input_interval = 0;
#endif

static int work_sequence_on = 0;
/* static unsigned int trans_rq= TRANS_RQ; */
static unsigned int trans_load_rq = TRANS_LOAD_RQ;
static unsigned int hotpluging_rate = CHECK_DELAY_OFF;

static struct clk *cpu_clk;
static DEFINE_MUTEX(hotplug_lock);
static DEFINE_PER_CPU(struct cpu_time_info, hotplug_cpu_time);
static struct cpu_hotplug_rat cpu_plug_rate_def[] = {
		{
			.up_rate   = .1*HZ,	//unused
			.down_rate = .1*HZ	//after 2->1, in
		},
		{
			.up_rate   = .1*HZ * 4,	//after 1->2, in or out
			.down_rate = .1*HZ * 4	//after 3->2, in or out
		},
		{
			.up_rate   = .1*HZ * 4,	//after 2->3, in or out
			.down_rate = .1*HZ * 2	//after 4->3, in or out
		},
		{
			.up_rate   = .1*HZ * 2,	//after 3->4, out
			.down_rate = .1*HZ		//unused
		},
};

/*load threshold*/
static const unsigned int threshold[CPULOAD_TABLE][2] = {
		{0, 60*1/4},
		{12*2/4, 66*2/4},
		{36*3/4, 60*3/4},
		{40*4/4, 100},
		{0, 0}
};

/*externs*/
extern unsigned long clk_get_rate(struct clk *clk);
extern struct clk *clk_get_sys(const char *dev_id, const char *con_id);
extern unsigned long get_cpu_nr_running(unsigned int cpu);
extern int __cpuinit cpu_up(unsigned int cpu);
extern int __ref cpu_down(unsigned int cpu);

static int test_cpu_not_mask(int cpu)
{
#if 0
		if(cpu == 1)
			return (plug_mask & (1<<1)) == 0;
		else if((cpu == 2) || (cpu == 3))
			return ((plug_mask & (1<<2)) == 0) && ((plug_mask & (1<<3)) == 0);
		return 1;
#endif
	return ((plug_mask & (1 << cpu)) == 0);
}

#if INPUT_MODE_DETECT
/*
在有用户输入的情况下，为了减少震荡，提高开cpu门槛，降低关cpu门槛；
*/
static int check_loading_for_up(unsigned int load, int online)
{
		if(time_after_eq(jiffies, input_interval))
		{
				printk("+");
				return load > threshold[online-1][1];
		}
		else
		{
				printk("^");
				return load > threshold[online-1][1] + 5;
		}
}
static int check_loading_for_down(unsigned int load, int online)
{
		if(time_after_eq(jiffies, input_interval))
		{
				printk("-");
				return load < threshold[online-1][0];
		}
		else
		{
				printk("|");
				return load < threshold[online-1][0] - 10;
		}
}
#else
static int check_loading_for_up(unsigned int load, int online)
{
		return load > threshold[online-1][1];
}
static int check_loading_for_down(unsigned int load, int online)
{
		return load < threshold[online-1][0];
}
#endif
/*kernel fuctions*/
static inline enum flag
standalone_hotplug(unsigned int load, unsigned long load_min)
{
	unsigned int cur_freq;
	unsigned int nr_online_cpu;
	unsigned int avg_load;

	cur_freq = clk_get_rate(cpu_clk) / 1000;

	nr_online_cpu = num_online_cpus();
	avg_load = (unsigned int)((cur_freq * load) / max_performance);
	DBG_PRINT("avg_load=%d, cur_freq=%d, nr_running()=%ld\n", avg_load, cur_freq, nr_running());
	DBG_PRINT("nr_online_cpu: %d, freq_max: %d\n", nr_online_cpu, freq_max);

	if (nr_online_cpu > 1 && check_loading_for_down(avg_load, nr_online_cpu))
	{
		trace_stand_hotplug_out(1, hotpluging_rate, avg_load*4/nr_online_cpu, nr_online_cpu, down_count, 0, 0, 0);
		goto cpu_plug_out;
	}
/* 	else if (nr_online_cpu > 1 && cur_freq <= freq_min)
	{
		trace_stand_hotplug_out(2, hotpluging_rate, 0, nr_online_cpu, down_count, 0, cur_freq, freq_min);
		goto cpu_plug_out;
	}	 */
	/* If total nr_running is less than cpu(on-state) number, hotplug do not hotplug-in */
	else if (nr_running() > nr_online_cpu &&
		   check_loading_for_up(avg_load, nr_online_cpu) && cur_freq > freq_min)
	{
		down_count = 0;
		// trace_stand_hotplug_out(5, hotpluging_rate, avg_load*4/nr_online_cpu, nr_online_cpu, down_count, 0, cur_freq, freq_min);
		trace_stand_hotplug_in(1, hotpluging_rate, avg_load*4/nr_online_cpu, nr_online_cpu, nr_running(), cur_freq, freq_min);
		return HOTPLUG_IN;
	}
	else if (nr_online_cpu > 1 && load_min < trans_load_rq)
	{
		/*If CPU(cpu_rq_min) load is less than trans_load_rq, hotplug-out*/
		if (nr_online_cpu*avg_load < 100*(nr_online_cpu-1))
		{
			trace_stand_hotplug_out(3, hotpluging_rate, avg_load*4/nr_online_cpu, nr_online_cpu, down_count, load_min, 0, 0);
			goto cpu_plug_out;
		}
	}

	return HOTPLUG_NOP;

cpu_plug_out:
	/*更低的负载应该加速关闭cpu*/
	if(avg_load <= 10)
		down_count += 2;
	else
		down_count += 1;

	if(down_count > MAX_DOWN_COUNT)
	{
		down_count = 0;
		trace_stand_hotplug_out(0, hotpluging_rate, 0, nr_online_cpu, 0, 0, 0, 0);
		return HOTPLUG_OUT;
	}
	else
	{
		return HOTPLUG_NOP;
	}
}

static __cpuinit void cpu_hot_plug_test_for_stability(void)
{
		unsigned nr_online_cpu = num_online_cpus();
		if (cpu_possible(3) && test_cpu_not_mask(3))
		{
			if(cpu_online(3) == CPU_OFF)
			{
				printk(KERN_DEBUG "cpu3 turning on start...");
				cpu_up(3);
				hotpluging_rate = cpu_plug_rate_def[nr_online_cpu].up_rate;
				printk(KERN_DEBUG "end\n");
			}
			else
			{
				printk(KERN_DEBUG "cpu3 turning off start...");
				cpu_down(3);
				hotpluging_rate = cpu_plug_rate_def[nr_online_cpu-1].down_rate;
				printk(KERN_DEBUG "end\n");
			}
		}
		if (cpu_possible(2) && test_cpu_not_mask(2))
		{
			if(cpu_online(2) == CPU_OFF)
			{
				printk(KERN_DEBUG "cpu2 turning on start...");
				cpu_up(2);
				hotpluging_rate = cpu_plug_rate_def[nr_online_cpu].up_rate;
				printk(KERN_DEBUG "end\n");
			}
			else
			{
				printk(KERN_DEBUG "cpu2 turning off start...");
				cpu_down(2);
				hotpluging_rate = cpu_plug_rate_def[nr_online_cpu-1].down_rate;
				printk(KERN_DEBUG "end\n");
			}
		}
		if(test_cpu_not_mask(1))
		{
			if(cpu_online(1) == CPU_OFF)
			{
				printk(KERN_DEBUG "cpu1 turning on start...");
				cpu_up(1);
				printk(KERN_DEBUG "end\n");
			}
			else
			{
				printk(KERN_DEBUG "cpu1 turning off start...");
				cpu_down(1);
				printk(KERN_DEBUG "end\n");
			}
		}
}

static __cpuinit void hotplug_timer(struct work_struct *work)
{
	struct cpu_hotplug_info tmp_hotplug_info[4];
	int i;
	unsigned int load = 0;
	unsigned int load_min = 100;
	//unsigned int select_off_cpu = 2;
	enum flag flag_hotplug;
	unsigned nr_online_cpu = num_online_cpus();

	mutex_lock(&hotplug_lock);

	/*just for test system stability*/
	if(plug_test)
	{
		cpu_hot_plug_test_for_stability();
		goto no_hotplug;
	}
#if 0
	if((freq_lock == 1) &&
		 ( ((fb_lock != 0) && (cpu_online(2) == CPU_OFF)) ||
		   ((fb_lock == 0) && (cpu_online(1) == CPU_OFF)) )
		)
	{
		/*cancel the work sequence*/
		work_sequence_on = 0;
		mutex_unlock(&hotplug_lock);
		//printk("disable cpu_hot_plug\n");
		return;
	}
#endif
#if 0
	struct timeval trace_start_tv;
	struct timeval trace_end_tv;
	do_gettimeofday(&trace_start_tv);
#endif
	if (user_lock == 1)
	{
		goto no_hotplug;
	}

	for_each_online_cpu(i) {
		struct cpu_time_info *tmp_info;
		cputime64_t cur_wall_time, cur_idle_time;
		unsigned int idle_time, wall_time;

		tmp_info = &per_cpu(hotplug_cpu_time, i);

		cur_idle_time = get_cpu_idle_time_us(i, &cur_wall_time);

		idle_time = (unsigned int)cputime64_sub(cur_idle_time,
							tmp_info->prev_cpu_idle);
		tmp_info->prev_cpu_idle = cur_idle_time;

		wall_time = (unsigned int)cputime64_sub(cur_wall_time,
							tmp_info->prev_cpu_wall);
		tmp_info->prev_cpu_wall = cur_wall_time;

		if (wall_time < idle_time)
			goto no_hotplug;

		if (wall_time == 0)	wall_time++;

		tmp_info->load = 100 * (wall_time - idle_time) / wall_time;

		load += tmp_info->load;
		/*find minimum runqueue length*/
		tmp_hotplug_info[i].nr_running = get_cpu_nr_running(i);

		if (load_min > tmp_info->load) {
			load_min = tmp_info->load;
		}
	}

	/*standallone hotplug*/
	flag_hotplug = standalone_hotplug(load, load_min);

	/*cpu hotplug*/
	if (flag_hotplug == HOTPLUG_IN)
	{
		if(test_cpu_not_mask(1) && (cpu_online(1) == CPU_OFF))
		{
			printk(KERN_DEBUG "cpu1 turning on start...");
			mutex_unlock(&hotplug_lock);
			cpu_up(1);
			mutex_lock(&hotplug_lock);
			hotpluging_rate = cpu_plug_rate_def[nr_online_cpu].up_rate;
			printk(KERN_DEBUG "end\n");
			goto no_hotplug;
		}
		if (cpu_possible(2) && test_cpu_not_mask(2) && (cpu_online(2) == CPU_OFF))
		{
			printk(KERN_DEBUG "cpu2 turning on start...");
			mutex_unlock(&hotplug_lock);
			cpu_up(2);/*up cpu2*/
			mutex_lock(&hotplug_lock);
			hotpluging_rate = cpu_plug_rate_def[nr_online_cpu].up_rate;
			printk(KERN_DEBUG "end\n");
			goto no_hotplug;
		}
		if (cpu_possible(3) && test_cpu_not_mask(3) && (cpu_online(3) == CPU_OFF))
		{
			printk(KERN_DEBUG "cpu3 turning on start...");
			mutex_unlock(&hotplug_lock);
			cpu_up(3);/*up cpu3*/
			mutex_lock(&hotplug_lock);
			hotpluging_rate = cpu_plug_rate_def[nr_online_cpu].up_rate;
			printk(KERN_DEBUG "end\n");
			goto no_hotplug;
		}
	}
	else if (flag_hotplug == HOTPLUG_OUT)
	{
		if (test_cpu_not_mask(3) && (cpu_online(3) == CPU_ON))
		{
			printk(KERN_DEBUG "cpu3 turning off start...");
			mutex_unlock(&hotplug_lock);
			cpu_down(3);/*down cpu3*/
			mutex_lock(&hotplug_lock);
			hotpluging_rate = cpu_plug_rate_def[nr_online_cpu-2].down_rate;
			printk(KERN_DEBUG "end\n");
			goto no_hotplug;
		}

		if (test_cpu_not_mask(2) && (cpu_online(2) == CPU_ON))
		{
			printk(KERN_DEBUG "cpu2 turning off start...");
			mutex_unlock(&hotplug_lock);
			cpu_down(2);/*down cpu2*/
			mutex_lock(&hotplug_lock);
			hotpluging_rate = cpu_plug_rate_def[nr_online_cpu-2].down_rate;
			printk(KERN_DEBUG "end\n");
			goto no_hotplug;
		}

	    if(cpu_possible(2))
	    {
	        if(fb_lock == 0)
	        {
	           if(test_cpu_not_mask(1) && (cpu_online(1) == CPU_ON))
	           {
					printk(KERN_DEBUG "cpu1 turning off start...");
					mutex_unlock(&hotplug_lock);
					cpu_down(1);
					mutex_lock(&hotplug_lock);
					hotpluging_rate = cpu_plug_rate_def[nr_online_cpu-2].down_rate;
					printk(KERN_DEBUG "end\n");
					goto no_hotplug;
	           }
	        }
	    }
	    else
	    {
	       if(test_cpu_not_mask(1) && (cpu_online(1) == CPU_ON))
	       {
				printk(KERN_DEBUG "cpu1 turning off start...");
				mutex_unlock(&hotplug_lock);
				cpu_down(1);
				mutex_lock(&hotplug_lock);
				hotpluging_rate = cpu_plug_rate_def[nr_online_cpu-2].down_rate;
				printk(KERN_NOTICE "end\n");
				goto no_hotplug;
			}
		}
	}

no_hotplug:
	queue_delayed_work_on(0, hotplug_wq, &hotplug_work, hotpluging_rate);
	mutex_unlock(&hotplug_lock);
#if 0
	do_gettimeofday(&trace_end_tv);
	long time_us = (trace_end_tv.tv_sec - trace_start_tv.tv_sec)*1000000 + \
			 (trace_end_tv.tv_usec- trace_start_tv.tv_usec);
	printk("used time: %d\n", time_us);
#endif
}

/*notifiers*/
static int owl_pm_hotplug_notifier_event(struct notifier_block *this,
					     unsigned long event, void *ptr)
{
	static unsigned user_lock_saved;

	switch (event) {
	case PM_SUSPEND_PREPARE:
	case PM_HIBERNATION_PREPARE:
	case PM_RESTORE_PREPARE:
		mutex_lock(&hotplug_lock);
		user_lock_saved = user_lock;
		user_lock = 1;
		DBG_PRINT("%s: saving pm_hotplug lock %x\n",
			__func__, user_lock_saved);
		disable_nonboot_cpus();
		mutex_unlock(&hotplug_lock);
		return NOTIFY_OK;
	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:
		mutex_lock(&hotplug_lock);
		DBG_PRINT("%s: restoring pm_hotplug lock %x\n",
			__func__, user_lock_saved);
		user_lock = user_lock_saved;
		mutex_unlock(&hotplug_lock);
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct notifier_block owl_pm_hotplug_notifier = {
	.notifier_call = owl_pm_hotplug_notifier_event,
};

static int hotplug_reboot_notifier_call(struct notifier_block *this,
					unsigned long code, void *_cmd)
{
	mutex_lock(&hotplug_lock);
	DBG_PRINT("%s: disabling pm hotplug\n", __func__);
	user_lock = 1;
	mutex_unlock(&hotplug_lock);

	return NOTIFY_DONE;
}

static struct notifier_block hotplug_reboot_notifier = {
	.notifier_call = hotplug_reboot_notifier_call,
};


#ifdef CONFIG_CPU_FREQ
static void get_cpu0_cpuinfo_policy(void)
{
	struct cpufreq_policy *cpu_policy;

	/* get cpu0 policy */
	cpu_policy = cpufreq_cpu_get(0);
	freq_max = cpu_policy->max;
	freq_min = cpu_policy->min;
	max_performance = freq_max * HOTPLUG_CPU_SUM;
	DBG_PRINT("%s, cpu_policy->max %d, cpu_policy->min %d\n",
		__func__, cpu_policy->max, cpu_policy->min);
	DBG_PRINT("%s, freq_min %d, freq_max %d, HOTPLUG_CPU_SUM: %d\n",
		__func__, freq_min, freq_max, HOTPLUG_CPU_SUM);
}

static int owl_cpufreq_for_cpuhotplug(struct notifier_block *nb,
				      unsigned long state, void *data)
{
	if (state == CPUFREQ_NOTIFY)
		get_cpu0_cpuinfo_policy();

	return NOTIFY_OK;
}

static struct notifier_block owl_cpufreq_for_cpuhotplug_nb = {
	.notifier_call = owl_cpufreq_for_cpuhotplug,
};
#else
static void get_cpu0_cpuinfo_policy(void) {}
#endif

static void start_cpu_hot_plug(int delay)
{
	queue_delayed_work_on(0, hotplug_wq, &hotplug_work, delay);
}

static int cpufreq_stat_notifier_trans(struct notifier_block *nb,
		unsigned long val, void *data)
{
	//int orgize_todo = 0;
	struct cpufreq_freqs *freq = data;

	if ((val != CPUFREQ_POSTCHANGE) || (freq->cpu != 0) )
		return 0;
	freq_trans_works.new = freq->new;
	queue_delayed_work_on(0, hotplug_wq, &(freq_trans_works.work), 0);
	return 0;
}

static void adjust_hot_plug_with_freq(struct work_struct *work)
{
/* 	struct freq_tran_work *freq_tran_workp = (struct freq_tran_work*)work;
	if ((freq_tran_workp->new > freq_max/2)
		&& (freq_tran_workp->new > freq_threshold_for_cpuplug))
	{
		if (mutex_is_locked(&hotplug_lock))
		mutex_lock(&hotplug_lock);
		if(work_sequence_on == 0)
				start_cpu_hot_plug(0);
		freq_lock = 0;
		work_sequence_on = 1;
		mutex_unlock(&hotplug_lock);
		//printk("enable cpu_hot_plug, freq: %d, thresh: %d\n", freq_tran_workp->new, freq_max/2);
	}
	else
	{
		mutex_lock(&hotplug_lock);
		freq_lock = 1;
		//printk( "freq_lock: %d, freq: %d, thresh: %d\n", freq_lock, freq_tran_workp->new, freq_max/2);
		mutex_unlock(&hotplug_lock);
	}
 */
}

static struct notifier_block notifier_freqtrans_block = {
	.notifier_call = cpufreq_stat_notifier_trans
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static __cpuinit void early_suspend_process(struct work_struct *work)
{
		struct early_suspend_work_t *tmp = (struct early_suspend_work_t *)work;
		switch(tmp->flag)
		{
			case UP_CPU1:
				//printk( "early_suspend_process UP_CPU1 start ...");
				//if(cpu_online(1) == CPU_OFF)
				//	cpu_up(1);
				mutex_lock(&hotplug_lock);
				fb_lock = 1;
				mutex_unlock(&hotplug_lock);
				//printk( "end\n");
				break;
			case DOWN_CPU1:
				//printk( "early_suspend_process DOWN start ...");
				//if(cpu_online(1) == CPU_ON)
				//	cpu_down(1);
				mutex_lock(&hotplug_lock);
				fb_lock = 0;
				mutex_unlock(&hotplug_lock);
				//printk( "end\n");
				break;
			default:
				printk("wrong falg: 0x%08x\n", tmp->flag);
		}
}

static void hotplug_early_suspend(struct early_suspend *h)
{
/* 	early_suspend_work.flag = DOWN_CPU1;
	printk( "hotplug_early_suspend\n");
	queue_delayed_work_on(0, hotplug_wq, &(early_suspend_work.work), 0); */
}

static void hotplug_late_resume(struct early_suspend *h)
{
/* 	early_suspend_work.flag = UP_CPU1;
	printk( "hotplug_late_resume\n");
	queue_delayed_work_on(0, hotplug_wq, &(early_suspend_work.work), 0); */
}
#endif

/*input about*/
struct cpuplug_inputopen {
	struct input_handle *handle;
	struct work_struct inputopen_work;
};
#if INPUT_MODE_DETECT
static struct cpuplug_inputopen inputopen;
static void cpuplug_input_event(struct input_handle *handle,
					    unsigned int type,
					    unsigned int code, int value)
{
	input_interval = jiffies + 30*HZ;
}

static void cpuplug_input_open(struct work_struct *w)
{
	struct cpuplug_inputopen *io =
		container_of(w, struct cpuplug_inputopen,
			     inputopen_work);
	int error;

	error = input_open_device(io->handle);
	if (error)
		input_unregister_handle(io->handle);
}

static int cpuplug_input_connect(struct input_handler *handler,
					     struct input_dev *dev,
					     const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	printk("%s: connect to %s\n", __func__, dev->name);
	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "cpuplug";

	error = input_register_handle(handle);
	if (error)
		goto err;

	inputopen.handle = handle;
	queue_work(hotplug_wq, &inputopen.inputopen_work);
	return 0;

err:
	kfree(handle);
	return error;
}

static void cpuplug_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id cpuplug_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			 INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
			    BIT_MASK(ABS_MT_POSITION_X) |
			    BIT_MASK(ABS_MT_POSITION_Y) },
	}, /* multi-touch touchscreen */
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
			 INPUT_DEVICE_ID_MATCH_ABSBIT,
		.keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
		.absbit = { [BIT_WORD(ABS_X)] =
			    BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) },
	}, /* touchpad */
	{ },
};

static struct input_handler cpuplug_input_handler = {
	.event          = cpuplug_input_event,
	.connect       = cpuplug_input_connect,
	.disconnect   = cpuplug_input_disconnect,
	.name          = "cpuplug",
	.id_table      = cpuplug_ids,
};
#endif

static void set_cpu_frequence(struct work_struct *work)
{
#if 0
	int i;
	unsigned int freq;
#endif
	struct cpufreq_frequency_table *table;

	table = cpufreq_frequency_get_table(0);
	if(NULL == table)
	{
		queue_delayed_work_on(0, hotplug_wq, &detecting_freq_work, HZ);
		DBG_PRINT("cpu hot plug set_cpu_frequence, cpufrequnce set failed\n");
		return;
	}

#if 0
	for (i = 0; table[i].frequency != CPUFREQ_TABLE_END; ) {
		freq = table[i].frequency;
		i++;
		if(table[i].frequency == CPUFREQ_TABLE_END)
			break;
		if (freq > freq_max)
			freq_max = freq;
		if (freq_min > freq)
			freq_min = freq;
	}
	/*get max frequence*/
	max_performance = freq_max * HOTPLUG_CPU_SUM;
	DBG_PRINT("freq_min %d\n", freq_min);
	DBG_PRINT("freq_max %d, HOTPLUG_CPU_SUM: %d\n", freq_max, HOTPLUG_CPU_SUM);
	DBG_PRINT(KERN_INFO "cpu hot plug set_cpu_frequence, cpufrequnce set ok\n");
#else
	get_cpu0_cpuinfo_policy();
#endif

	start_cpu_hot_plug(BOOT_DELAY * HZ);

	/*enable cpu hot plug*/
	mutex_lock(&hotplug_lock);
	user_lock = 0;
	freq_lock = 0;
	/*on default, sequence work is on duty*/
	work_sequence_on = 1;
	mutex_unlock(&hotplug_lock);

	if (cpufreq_register_notifier(&notifier_freqtrans_block, CPUFREQ_TRANSITION_NOTIFIER))
	{
		printk("cpufreq_register_notifier failed\n");
		return;
	}

#if INPUT_MODE_DETECT
	if (input_register_handler(&cpuplug_input_handler))
		printk("%s: failed to register input handler\n",	__func__);
#endif
	return;
}

/*sys interface*/
static ssize_t show_usr_lock(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%u\n", user_lock);
}

int set_user_lock(int val)
{
	int ret=0;
	if((val == 0) || (val == 1))
	{
		mutex_lock(&hotplug_lock);
		user_lock = val;
		mutex_unlock(&hotplug_lock);
		printk(KERN_DEBUG "%s: user_lock: %d\n", __func__, user_lock);	
	}
	else
	{
		ret = -EINVAL;
	}
	return ret;
}
EXPORT_SYMBOL(set_user_lock);

static ssize_t __ref store_usr_lock(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	ssize_t ret = count;
	
	switch (buf[0]) {
	case '0':
		set_user_lock(0);
		break;	
	case '1':
		set_user_lock(1);
		break;
	default:
		DBG_PRINT("%s: user_lock: %d\n", __func__, user_lock);
		ret = -EINVAL;
	}
	return count;
}
static DEVICE_ATTR(usr_lock, 0644, show_usr_lock,  store_usr_lock);
static ssize_t show_freq_lock(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%u\n", freq_lock);
}
static DEVICE_ATTR(freq_lock, 0644, show_freq_lock,  NULL);

static ssize_t __ref store_fb_lock(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	ssize_t ret = count;
	switch (buf[0]) {
	case '0':
		mutex_lock(&hotplug_lock);
		fb_lock = 0;
		mutex_unlock(&hotplug_lock);
		break;
	case '1':
		mutex_lock(&hotplug_lock);
		fb_lock = 1;
		mutex_unlock(&hotplug_lock);
		break;
	default:
		ret = -EINVAL;
	}
	return count;
}
static ssize_t show_fb_lock(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%u\n", fb_lock);
}
static DEVICE_ATTR(fb_lock, 0644, show_fb_lock, store_fb_lock);

static int time_flag_2 = 0;

static ssize_t show_time_2(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%u\n", time_flag_2);
}
static ssize_t __ref store_time_2(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	ssize_t ret = count;
	switch (buf[0]) {
	case '0':
		time_flag_2 = 0;
		break;
	case '1':
		time_flag_2 = 1;
		break;
	default:
		ret = -EINVAL;
	}
	return count;
}

static DEVICE_ATTR(opt2, 0644, show_time_2,  store_time_2);

/*sys interface*/
static ssize_t show_plug_mask(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "0x%01x\n", plug_mask);
}
/*extern int enable_plug_mask;
*/
int set_plug_mask(int val)
{
	int ret=0;
	mutex_lock(&hotplug_lock);
	plug_mask = val;
	mutex_unlock(&hotplug_lock);
	return ret;
}
EXPORT_SYMBOL(set_plug_mask);

static ssize_t __ref store_plug_mask(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
/*	if(enable_plug_mask == 0)
		return count;
*/
	mutex_lock(&hotplug_lock);
	sscanf(buf, "0x%01x", &plug_mask);
	mutex_unlock(&hotplug_lock);
	printk("plug_mask :0x%08x\n", plug_mask);
	return count;
}
static DEVICE_ATTR(plug_mask, 0644, show_plug_mask,  store_plug_mask);

static ssize_t show_plug_test(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "0x%01x\n", plug_test);
}
/*extern int enable_plug_mask;
*/
static ssize_t __ref store_plug_test(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
/*	if(enable_plug_mask == 0)
		return count;
*/
	mutex_lock(&hotplug_lock);
	sscanf(buf, "0x%01x", &plug_test);
	mutex_unlock(&hotplug_lock);
	printk("plug_test :0x%08x\n", plug_test);
	return count;
}
static DEVICE_ATTR(plug_test, 0644, show_plug_test,  store_plug_test);

static struct attribute *cpuplug_lock_attrs[] = {
	&dev_attr_usr_lock.attr,
	&dev_attr_fb_lock.attr,
	&dev_attr_freq_lock.attr,
	&dev_attr_plug_mask.attr,
	&dev_attr_plug_test.attr,
	&dev_attr_opt2.attr,
	NULL
};

static struct attribute_group cpuplug_attr_group = {
	.attrs = cpuplug_lock_attrs,
	.name = "autoplug",
};
/**
 * cpuplug_add_interface - add CPU global sysfs attributes
 */
int cpuplug_add_interface(struct device *dev)
{
	return sysfs_create_group(&dev->kobj, &cpuplug_attr_group);
}

/*inits*/
int owl_pm_hotplug_init(void)
{
	//unsigned int i;
	//unsigned int freq;
	return 0;
	//hotplug_wq = create_workqueue("dynamic hotplug");
	hotplug_wq = alloc_workqueue("dynamic hotplug", 0, 0);
	if (!hotplug_wq) {
		DBG_PRINT("Creation of hotplug work failed\n");
		return -EFAULT;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	/*early_suspend cpu hot plug*/
	INIT_DELAYED_WORK(&(early_suspend_work.work), early_suspend_process);
	early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	early_suspend.suspend = hotplug_early_suspend;
	early_suspend.resume  = hotplug_late_resume;
	register_early_suspend(&early_suspend);
#endif

	if(HOTPLUG_CPU_SUM <= 2)
	{
		/*work on default*/
		user_lock = 0;
		return 0;
	}


	//INIT_DELAYED_WORK(&hotplug_work, hotplug_timer);
	INIT_DELAYED_WORK(&hotplug_work, hotplug_timer);
	INIT_DELAYED_WORK(&(freq_trans_works.work), adjust_hot_plug_with_freq);
	INIT_DELAYED_WORK(&detecting_freq_work, set_cpu_frequence);
#if INPUT_MODE_DETECT
	INIT_WORK(&inputopen.inputopen_work, cpuplug_input_open);
#endif

	queue_delayed_work_on(0, hotplug_wq, &detecting_freq_work, HZ);

	/*default values when system booting*/
	user_lock = 1;
	freq_min = 96000;
	freq_max = clk_get_rate(cpu_clk) / 1000;
	max_performance = freq_max * HOTPLUG_CPU_SUM;


	DBG_PRINT("freq_min %d\n", freq_min);
	DBG_PRINT("freq_max %d, HOTPLUG_CPU_SUM: %d\n", freq_max, HOTPLUG_CPU_SUM);

	register_pm_notifier(&owl_pm_hotplug_notifier);
	register_reboot_notifier(&hotplug_reboot_notifier);
	cpufreq_register_notifier(&owl_cpufreq_for_cpuhotplug_nb,
		CPUFREQ_POLICY_NOTIFIER);

	/*on default, sequence work of detecting cpufreq driver is on duty*/
	work_sequence_on = 1;

	return 0;
}
late_initcall(owl_pm_hotplug_init);

static struct platform_device owl_pm_hotplug_device = {
	.name = "owl-dynamic-cpu-hotplug",
	.id = -1,
};

extern int clk_enable(struct clk *c);
static int __init owl_pm_hotplug_device_init(void)
{
	int ret;
	return 0;
	ret = platform_device_register(&owl_pm_hotplug_device);
	if (ret) {
		DBG_PRINT("failed at(%d)\n", __LINE__);
		return ret;
	}

	ret = cpuplug_add_interface(cpu_subsys.dev_root);
	if (ret) {
		DBG_PRINT("failed at(%d)\n", __LINE__);
		return ret;
	}

	DBG_PRINT("owl_pm_hotplug_device_init: %d\n", ret);

	cpu_clk = clk_get(NULL, CLKNAME_CPU_CLK);
	if (IS_ERR(cpu_clk))
	{
		DBG_PRINT("owl_pm_hotplug_device_init clk_get_sys failed\n");
		return PTR_ERR(cpu_clk);
  }
	/*clk_enable(cpu_clk);
	*/

	return ret;
}

late_initcall(owl_pm_hotplug_device_init);
