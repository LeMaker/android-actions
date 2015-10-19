/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#if 0
#define CPUFREQ_DBG(format, ...) \
	pr_notice("owl cpufreq: " format, ## __VA_ARGS__)

#else /* DEBUG */
#define CPUFREQ_DBG(format, ...)
#endif

#define CPUFREQ_ERR(format, ...) \
	pr_err("owl cpufreq: " format, ## __VA_ARGS__)

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/opp.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <asm/cpu.h>

#include <mach/module-owl.h>
#include <mach/clkname.h>
#include <mach/clkname_priv.h>
#include <mach/cpu_map-owl.h>
#include <mach/dvfs.h>

#define CPU_REGULATOR_NAME	"cpuvdd"
#define CPU_VOLT_STEP_UV	25000

extern int CFURC(int user);
extern int CFRUC(int real);
extern int CF0AOT(struct device *dev, struct cpu0_opp_table *table, int table_size);

static int vdd_warp = 0;

static unsigned int transition_latency;

static struct device *cpu_dev;
static struct clk *cpu_clk;
static struct clk *core_pll;
static struct regulator *cpu_reg;
struct cpufreq_frequency_table *freq_table;
struct cpufreq_frequency_table *user_freq_table;
struct cpufreq_frequency_table *real_freq_table;

static unsigned int locking_frequency;
static bool frequency_locked;
static DEFINE_MUTEX(freq_table_mux);

int cpufreq_distinguish_max_freq;
#define CPUFREQ_DISPLAY_RES

#ifdef CPUFREQ_DISPLAY_RES
static struct cpu0_opp_table cpu0_table_l1[] = {
	/*khz		uV*/
	{1104000, 1175000},
	{ 900000, 1025000},
	{ 720000,  975000},
	{ 504000,  950000},
	{ 240000,  950000},
};

/* for 0x6、0x16、0x26 */
static struct cpu0_opp_table cpu0_table_l2[] = {
	/*khz		uV*/
	{1104000, 1175000},
	{ 900000, 1025000},
	{ 720000,  975000},
	{ 504000,  950000},
	{ 240000,  950000},
};

/* for 0x15、0x25、0x5、0x66 */
static struct cpu0_opp_table cpu0_table_l2_h1[] = {
	/*khz		uV*/
	{1104000, 1200000},
	{ 900000, 1050000},
	{ 720000, 1000000},
	{ 504000,  950000},
	{ 240000,  950000},
};

/* for 0x6、0x16、0x26 */
static struct cpu0_opp_table cpu0_table_l3[] = {
	/*khz		uV*/
	{1200000, 1250000},
	{1104000, 1175000},
	{ 900000, 1025000},
	{ 720000,  975000},
	{ 504000,  975000},
	{ 240000,  975000},
};

/* for 0x15、0x25、0x5、0x66 */
static struct cpu0_opp_table cpu0_table_l3_h1[] = {
	/*khz		uV*/
	{1200000, 1250000},
	{1104000, 1200000},
	{ 900000, 1050000},
	{ 720000, 1000000},
	{ 504000,  975000},
	{ 240000,  975000},
};

struct dvfs_cpu0_table {
	int dvfs;
	struct cpu0_opp_table *cpu0_table;
	int cpu0_table_size;
};

/* DISPLAY_RES_L1 */
static struct dvfs_cpu0_table dvfs_cpu0_table_l1[] = {
	{ATM7059A_L_1, cpu0_table_l2, ARRAY_SIZE(cpu0_table_l2)},
	{ATM7059B_L_1, cpu0_table_l2, ARRAY_SIZE(cpu0_table_l2)},
	{ATM7059B_L_2, cpu0_table_l2, ARRAY_SIZE(cpu0_table_l2)},
	{ATM7059B_L_3, cpu0_table_l2, ARRAY_SIZE(cpu0_table_l2)},
	{ATM7059B_L_4, cpu0_table_l2, ARRAY_SIZE(cpu0_table_l2)},
	{ATM7059B_L_5, cpu0_table_l2_h1, ARRAY_SIZE(cpu0_table_l2_h1)},
	{ATM7059B_L_6, cpu0_table_l2_h1, ARRAY_SIZE(cpu0_table_l2_h1)},
	{ATM7059B_L_7, cpu0_table_l2_h1, ARRAY_SIZE(cpu0_table_l2_h1)},
	{ATM7059B_L_8, cpu0_table_l2_h1, ARRAY_SIZE(cpu0_table_l2_h1)},
};

/* DISPLAY_RES_L2 */
static struct dvfs_cpu0_table dvfs_cpu0_table_l2[] = {
	{ATM7059A_L_1, cpu0_table_l2, ARRAY_SIZE(cpu0_table_l2)},
	{ATM7059B_L_1, cpu0_table_l2, ARRAY_SIZE(cpu0_table_l2)},
	{ATM7059B_L_2, cpu0_table_l2, ARRAY_SIZE(cpu0_table_l2)},
	{ATM7059B_L_3, cpu0_table_l2, ARRAY_SIZE(cpu0_table_l2)},
	{ATM7059B_L_4, cpu0_table_l2, ARRAY_SIZE(cpu0_table_l2)},
	{ATM7059B_L_5, cpu0_table_l2_h1, ARRAY_SIZE(cpu0_table_l2_h1)},
	{ATM7059B_L_6, cpu0_table_l2_h1, ARRAY_SIZE(cpu0_table_l2_h1)},
	{ATM7059B_L_7, cpu0_table_l2_h1, ARRAY_SIZE(cpu0_table_l2_h1)},
	{ATM7059B_L_8, cpu0_table_l2_h1, ARRAY_SIZE(cpu0_table_l2_h1)},
};

/* DISPLAY_RES_L3 */
static struct dvfs_cpu0_table dvfs_cpu0_table_l3[] = {
	{ATM7059A_L_1, cpu0_table_l3, ARRAY_SIZE(cpu0_table_l3)},
	{ATM7059B_L_1, cpu0_table_l3, ARRAY_SIZE(cpu0_table_l3)},
	{ATM7059B_L_2, cpu0_table_l3, ARRAY_SIZE(cpu0_table_l3)},
	{ATM7059B_L_3, cpu0_table_l3, ARRAY_SIZE(cpu0_table_l3)},
	{ATM7059B_L_4, cpu0_table_l3, ARRAY_SIZE(cpu0_table_l3)},
	{ATM7059B_L_5, cpu0_table_l3_h1, ARRAY_SIZE(cpu0_table_l3_h1)},
	{ATM7059B_L_6, cpu0_table_l3_h1, ARRAY_SIZE(cpu0_table_l3_h1)},
	{ATM7059B_L_7, cpu0_table_l3_h1, ARRAY_SIZE(cpu0_table_l3_h1)},
	{ATM7059B_L_8, cpu0_table_l3_h1, ARRAY_SIZE(cpu0_table_l3_h1)},
};

#else
static struct cpu0_opp_table cpu0_table[] = {
	/*khz		uV*/
	{1200000, 1225000},
	{1104000, 1175000},
	{1008000, 1100000},
	{ 900000, 1050000},
	{ 720000,  975000},
	{ 504000,  950000},
};
#endif

static struct cpu0_opp_table cpu0_user_table[] = {
	{1440000, 1250000},
	{1404000, 1200000},
	{1308000, 1175000},
	{1200000, 1150000},
	{1104000, 1100000},
	{ 900000, 1000000},
	{ 720000,  925000},
	{ 504000,  900000},
};

struct device cpu0_dev;
struct device cpu0_dev_user;

struct cpu0_opp_info {
	struct device *dev;
	struct cpu0_opp_table *cpu0_table;
	int cpu0_table_num;
};

static struct cpu0_opp_info cpu0_opp_info;
static struct cpu0_opp_info cpu0_opp_info_user;
static struct cpu0_opp_info cpu0_opp_info_vdd;

static struct cpu0_opp_info *cpu0_opp_info_cur;

static u32 cpuinfo_max_freq;
static u32 scaling_max_freq;
static u32 cpuinfo_min_freq;

/*
	find max_freq/min_freq in cpu0_opp_info
*/
static int find_cpu0_opp_info_max_min_freq(u32 *max_freq, u32 *scaling_max_freq, u32 *min_freq)
{
	u32 max_tmp = 0;
	u32 scaling_max_tmp = 0;
	u32 min_tmp = UINT_MAX;

	struct cpu0_opp_table *opp_table;
	int opp_table_num;

	opp_table = cpu0_opp_info.cpu0_table;
	opp_table_num = cpu0_opp_info.cpu0_table_num;
	if (opp_table[0].clk > max_tmp)
		max_tmp = opp_table[0].clk;

	if (opp_table[1].clk > scaling_max_tmp)
		scaling_max_tmp = opp_table[1].clk;

	if (opp_table[opp_table_num - 1].clk < min_tmp)
		min_tmp = opp_table[opp_table_num - 1].clk;

	if (max_tmp < min_tmp)
		return -EINVAL;

	*max_freq = max_tmp;

	if (cpufreq_distinguish_max_freq == 1)
		*scaling_max_freq = scaling_max_tmp;
	else
		*scaling_max_freq = max_tmp;
	*min_freq = min_tmp;
	return 0;
}

#ifdef CPUFREQ_DISPLAY_RES
enum display_res_type {
	DISPLAY_RES_L1,	/*800*480*/
	DISPLAY_RES_L2,	/*1024*600*/
	DISPLAY_RES_L3,	/*1280*800*/
};

char res_node_compatible[2][32] = {
	"actions,owl-lcd",
	"actions,owl-dsi",
};
char res_node_mode[2][32] = {
	"videomode-0",
	"videomode",
};

enum display_res_type get_display_res(void)
{
	int i;
	unsigned int xres, yres;
	struct device_node *np = NULL;
	struct device_node *mode_node = NULL;
	enum display_res_type display_res = DISPLAY_RES_L3;

	for (i = 0; i < 2; i++) {
		np = of_find_compatible_node(NULL, NULL, res_node_compatible[i]);
		if (!np)
			CPUFREQ_ERR("failed to find %s node\n", res_node_compatible[i]);
		else
			break;
	}
	if (np) {
		mode_node = of_parse_phandle(np, res_node_mode[i], 0);
		if (!mode_node)
			CPUFREQ_ERR("failed to find %s node\n", res_node_mode[i]);
	}
	if (mode_node) {
		if (of_property_read_u32(mode_node, "xres", &xres)) {
			CPUFREQ_ERR("failed to find xres\n");
			xres = 1280;
		}
		if (of_property_read_u32(mode_node, "yres", &yres)) {
			CPUFREQ_ERR("failed to find yres\n");
			yres = 800;
		}
		CPUFREQ_ERR("xres:%d, yres:%d\n", xres, yres);
		if (yres > xres)
			xres = yres;
		if (xres >= 1280)
			display_res = DISPLAY_RES_L3;
		else if (xres >= 1024)
			display_res = DISPLAY_RES_L2;
		else
			display_res = DISPLAY_RES_L1;
	}
	return display_res;
}
#endif

/*
	init cpu0_opp_info/cpu0_opp_info_user
*/
extern int asoc_get_dvfslevel(void);

static struct dvfs_cpu0_table *get_cpu_table(int dvfs, struct dvfs_cpu0_table *p_dvfs_cpu0_table, int table_size)
{
	int i;

	for (i = 0; i < table_size; i++) {
		if (p_dvfs_cpu0_table[i].dvfs == dvfs) {
			pr_debug("[%s],i:%d,dvfs:0x%x\n", __func__, i, dvfs);
			return &p_dvfs_cpu0_table[i];
		}
	}
	return NULL;
}

static int init_cpu0_opp_info(void)
{
	int i;
	
#ifdef CPUFREQ_DISPLAY_RES
	enum display_res_type display_res;
	struct dvfs_cpu0_table *cur_dvfs_cpu0_table = NULL;
	struct dvfs_cpu0_table *p_dvfs_cpu0_table = NULL;
	int table_size = 0;

	display_res = get_display_res();
	switch (display_res) {
	case DISPLAY_RES_L1:
		p_dvfs_cpu0_table = dvfs_cpu0_table_l1;
		table_size = ARRAY_SIZE(dvfs_cpu0_table_l1);
		cpufreq_distinguish_max_freq = 0;
	case DISPLAY_RES_L2:
		p_dvfs_cpu0_table = dvfs_cpu0_table_l2;
		table_size = ARRAY_SIZE(dvfs_cpu0_table_l2);
		cpufreq_distinguish_max_freq = 0;
	break;
	case DISPLAY_RES_L3:
		p_dvfs_cpu0_table = dvfs_cpu0_table_l3;
		table_size = ARRAY_SIZE(dvfs_cpu0_table_l3);
		cpufreq_distinguish_max_freq = 1;
	break;
	default:
	break;
	};

	cur_dvfs_cpu0_table = get_cpu_table(asoc_get_dvfslevel(), p_dvfs_cpu0_table, table_size);
	if (cur_dvfs_cpu0_table) {
		cpu0_opp_info.cpu0_table = cur_dvfs_cpu0_table->cpu0_table;
		cpu0_opp_info.cpu0_table_num = cur_dvfs_cpu0_table->cpu0_table_size;
	} else {
		CPUFREQ_ERR("fail to find cpu_table\n");
	}

#else
	cpu0_opp_info.cpu0_table = cpu0_table;
	cpu0_opp_info.cpu0_table_num = ARRAY_SIZE(cpu0_table);
#endif
	
	for(i = 0; i < cur_dvfs_cpu0_table->cpu0_table_size; i++)
		cur_dvfs_cpu0_table->cpu0_table[i].volt += vdd_warp;

	cpu0_opp_info.dev = &cpu0_dev;

	cpu0_opp_info_user.cpu0_table = cpu0_user_table;
	cpu0_opp_info_user.cpu0_table_num = ARRAY_SIZE(cpu0_user_table);
	cpu0_opp_info_user.dev = &cpu0_dev_user;

	return 0;
}

/*
	get cpu0_opp_info_cur according to online_cpus
*/
static int select_cur_opp_info(void)
{

	cpu0_opp_info_cur = &cpu0_opp_info;

	return 0;
}

static int CF0AOT_all(void)
{
	int ret;

	/* opp_add clk/volt of cpu0_opp_info*/
	ret = CF0AOT(cpu0_opp_info.dev, cpu0_opp_info.cpu0_table,
			cpu0_opp_info.cpu0_table_num);
	if (ret) {
		CPUFREQ_ERR("failed to init OPP table 4c: %d\n", ret);
		return ret;
	}

	return 0;
}

/*
	init user freq table by real freq table
*/
int user_freqtable_init(struct cpufreq_frequency_table *real_table)
{
	int i;
	int real_khz;
	int user_khz;
	int freq_num;

	real_freq_table = real_table;

	for (i = 0; real_table[i].frequency != CPUFREQ_TABLE_END; i++)
			;

	freq_num = i;

	user_freq_table = kzalloc(sizeof(struct cpufreq_frequency_table) *
				(freq_num + 1), GFP_KERNEL);

	for (i = 0; i < freq_num; i++) {
		real_khz = real_freq_table[i].frequency;
		/*get the user_khz*/
		user_khz = CFRUC(real_khz);

		/*make user freq table*/
		user_freq_table[i].index = real_freq_table[i].index;
		user_freq_table[i].frequency = user_khz;
	}

	user_freq_table[freq_num].index = real_freq_table[freq_num].index;
	user_freq_table[freq_num].frequency = CPUFREQ_TABLE_END;

	return 0;
}

int freqtable_acitons_cinit(struct cpufreq_frequency_table *table)
{
	user_freqtable_init(table);
	return 0;
}

int freqtable_acitons_cfree(void)
{
	kfree(user_freq_table);
	return 0;
}

static int owl_cpu0_clk_init(void)
{
	int ret;

	core_pll = clk_get(NULL, CLKNAME_COREPLL);
	if (IS_ERR(core_pll)) {
		ret = PTR_ERR(core_pll);
		CPUFREQ_ERR("failed to get core pll: %d\n", ret);
		return ret;
	}

	cpu_clk = clk_get(NULL, CLKNAME_CPU_CLK);
	if (IS_ERR(cpu_clk)) {
		ret = PTR_ERR(cpu_clk);
		CPUFREQ_ERR("failed to get cpu clk: %d\n", ret);
		return ret;
	}

	return 0;
}

static void owl_cpu0_clk_free(void)
{
	clk_put(core_pll);
	clk_put(cpu_clk);
}

static long owl_round_cpu0_clk(unsigned long rate)
{
	long freq_hz;
	freq_hz = clk_round_rate(core_pll, rate);
	return freq_hz;
}

static unsigned long owl_get_cpu0_clk(void)
{
	unsigned long freq_hz;
	freq_hz = clk_get_rate(cpu_clk);
	return freq_hz;
}

static int owl_set_cpu0_clk(unsigned long rate)
{
	int ret = 0;
	ret = clk_set_rate(core_pll, rate);
	return ret;
}

static int cpu0_cpuinfo_update(struct cpufreq_policy *policy, u32 max_freq, u32 min_freq)
{
	policy->min = policy->cpuinfo.min_freq = min_freq;
	policy->max = policy->cpuinfo.max_freq = max_freq;
	return 0;
}

static int cpu0_verify_speed(struct cpufreq_policy *policy)
{
	int ret;

	if (mutex_lock_interruptible(&freq_table_mux))
		return -EINVAL;

	ret = cpufreq_frequency_table_verify(policy, user_freq_table);

	mutex_unlock(&freq_table_mux);
	return ret;
}

static unsigned int cpu0_get_speed(unsigned int cpu)
{
	int freq_khz;

	freq_khz = owl_get_cpu0_clk() / 1000;
	return CFRUC(freq_khz);
}

#define volt_align(d, a) (((d) + (a - 1)) & ~(a - 1))

static int owl_cpufreq_scale(struct cpufreq_policy *policy,
			   unsigned int target_freq, unsigned int relation)
{
	struct cpufreq_freqs real_freqs, freqs;
	struct opp *opp;
	unsigned long freq_hz, volt = 0, volt_old = 0;
	unsigned long volt_delta = 0, volt_step = 0;
	unsigned long real_target_freq;
	unsigned int index;
	int ret = 0;

	CPUFREQ_DBG("%s , target freq = %d\n", __func__, target_freq);

	ret = cpufreq_frequency_table_target(policy, user_freq_table, target_freq, 
					     relation, &index);
	if (ret) {
		CPUFREQ_ERR("failed to match target freqency %d: %d\n",
		       target_freq, ret);
		goto target_out;
	}

	target_freq = user_freq_table[index].frequency;
	real_target_freq = CFURC(target_freq);

	freq_hz = owl_round_cpu0_clk(real_target_freq * 1000);
	if (freq_hz < 0)
		freq_hz = real_target_freq * 1000;

	real_freqs.new = freq_hz / 1000;
	real_freqs.old = owl_get_cpu0_clk() / 1000;

	if (real_freqs.old == real_freqs.new) {
		ret = 0;
		goto target_out;
	}

	freqs.new = CFRUC(real_freqs.new);
	freqs.old = CFRUC(real_freqs.old);

	cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);

	if (cpu_reg) {
		rcu_read_lock();
		opp = opp_find_freq_ceil(cpu0_opp_info_cur->dev, &freq_hz);
		if (IS_ERR(opp)) {
			rcu_read_unlock();
			CPUFREQ_ERR("failed to find OPP for %ld\n", freq_hz);
			real_freqs.new = real_freqs.old;
			freqs.new = CFRUC(real_freqs.new);
			ret = PTR_ERR(opp);
			goto post_notify;
		}
		volt = opp_get_voltage(opp);
		rcu_read_unlock();
		volt_old = regulator_get_voltage(cpu_reg);
	}

	CPUFREQ_DBG("%u MHz, %ld mV --> %u MHz, %ld mV\n",
		 real_freqs.old / 1000, volt_old ? volt_old / 1000 : -1,
		 real_freqs.new / 1000, volt ? volt / 1000 : -1);

	/* scaling up?  scale voltage before frequency */
	if (cpu_reg && (volt > volt_old)) {
		CPUFREQ_DBG("set voltage PRE freqscale\n");

		ret = regulator_set_voltage(cpu_reg, volt, INT_MAX);
		if (ret) {
			CPUFREQ_ERR("failed to scale voltage up: %d\n", ret);
			real_freqs.new = real_freqs.old;
			freqs.new = CFRUC(real_freqs.new);
			goto post_notify;
		}
		CPUFREQ_DBG("set voltage PRE freqscale OK\n");
	}

	ret = owl_set_cpu0_clk(real_freqs.new * 1000);

	if (ret) {
		CPUFREQ_ERR("failed to set clock rate: %d\n", ret);
		if (cpu_reg)
			regulator_set_voltage(cpu_reg, volt_old, INT_MAX);
		real_freqs.new = real_freqs.old;
		freqs.new = CFRUC(real_freqs.new);
		goto post_notify;
	}

	/* scaling down?  scale voltage after frequency */
	if (cpu_reg && (volt < volt_old)) {
		CPUFREQ_DBG("set voltage after freqscale\n");
		volt_delta = volt_old - volt;
		if (volt_delta >= 100*1000) {
			volt_step = volt + volt_align(volt_delta/2, 25*1000);
			ret = regulator_set_voltage(cpu_reg, volt_step, INT_MAX);
			if (ret == 0)
				ret = regulator_set_voltage(cpu_reg, volt, INT_MAX);
		} else
			ret = regulator_set_voltage(cpu_reg, volt, INT_MAX);

		if (ret) {
			CPUFREQ_ERR("failed to scale voltage down: %d\n", ret);
			owl_set_cpu0_clk(real_freqs.old * 1000);
			real_freqs.new = real_freqs.old;
			freqs.new = CFRUC(real_freqs.new);
		}
		CPUFREQ_DBG("set voltage after freqscale OK\n");
	}

post_notify:
	cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
target_out:
	return ret;
}

static int cpu0_set_target(struct cpufreq_policy *policy,
			   unsigned int target_freq, unsigned int relation)
{
	int ret = 0;

	if (mutex_lock_interruptible(&freq_table_mux))
		return -EINVAL;

	if (frequency_locked)
		goto target_out;

	ret = owl_cpufreq_scale(policy, target_freq, relation);

target_out:
	mutex_unlock(&freq_table_mux);
	return ret;
}

static int cpu0_cpufreq_reinit_vdd(struct cpu0_opp_info *new_info)
{
	struct cpufreq_policy *policy;
	int ret = 0;
	int real_freq_khz;
	unsigned int index;
	unsigned int old_max;
	unsigned int old_min;

	policy = cpufreq_cpu_get(0);

	old_max = policy->max;
	old_min = policy->min;

	if (mutex_lock_interruptible(&freq_table_mux))
		return -EINVAL;

	/*reinit cur_freq table*/
	opp_free_cpufreq_table(cpu0_opp_info_cur->dev, &freq_table);

	ret = opp_init_cpufreq_table(new_info->dev, &freq_table);
	if (ret) {
		CPUFREQ_ERR("failed to reinit cpufreq table: %d\n", ret);
		ret = -EINVAL;
		mutex_unlock(&freq_table_mux);
		goto reinit_out;
	}

	freqtable_acitons_cfree();
	freqtable_acitons_cinit(freq_table);

	mutex_unlock(&freq_table_mux);

	real_freq_khz = owl_get_cpu0_clk() / 1000;
	cpufreq_frequency_table_target(policy, real_freq_table, real_freq_khz,
					CPUFREQ_RELATION_L, &index);
	real_freq_khz = real_freq_table[index].frequency;
	/***************************************/

	ret = cpu0_cpuinfo_update(policy, CFRUC(cpuinfo_max_freq), CFRUC(cpuinfo_min_freq));
	if (ret) {
		CPUFREQ_ERR("invalid frequency table: %d\n", ret);
		ret = -EINVAL;
		mutex_unlock(&freq_table_mux);
		goto reinit_out;
	}

	policy->min = CFRUC(cpuinfo_min_freq);
	policy->max = CFRUC(scaling_max_freq);

	policy->cur = CFRUC(real_freq_khz);

	cpufreq_frequency_table_get_attr(user_freq_table, policy->cpu);

	cpu0_opp_info_cur = new_info;

	schedule_work(&policy->update);

	return ret;
reinit_out:
	return ret;
}

static int cpu0_cpufreq_init(struct cpufreq_policy *policy)
{
	int ret = 0;
	int real_freq_khz;
	unsigned int index;

	if (mutex_lock_interruptible(&freq_table_mux))
		return -EINVAL;

	if (policy->cpu != 0) {

		/*let cpu1,2,3 as the managered cpu*/
		policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
		cpumask_setall(policy->cpus);

		/*The newly booted cpu will corrupt loops_per_jiffy*/
		/*and its own percpu loops_per_jiffy is also wrong*/
		/*these 2 value were init calc value without cpufreq*/
		/*so we should change them back according to current*/
		/*freq.*/
		loops_per_jiffy =
			per_cpu(cpu_data, policy->cpu).loops_per_jiffy =
			per_cpu(cpu_data, 0).loops_per_jiffy;

		ret = 0;
		goto init_out;
	}
	/*****get the real cpufreq, align to real freq_table************/
	ret = cpu0_cpuinfo_update(policy, cpuinfo_max_freq, cpuinfo_min_freq);
	if (ret) {
		CPUFREQ_ERR("invalid frequency table: %d\n", ret);
		goto init_out;
	}

	real_freq_khz = owl_get_cpu0_clk() / 1000;
	cpufreq_frequency_table_target(policy, real_freq_table, real_freq_khz,
					CPUFREQ_RELATION_L, &index);
	real_freq_khz = real_freq_table[index].frequency;
	/***************************************/

	/***we need to report user freq table******/
	/***from now on we will not use real freq table******/
	ret = cpu0_cpuinfo_update(policy, CFRUC(cpuinfo_max_freq), CFRUC(cpuinfo_min_freq));
	if (ret) {
		CPUFREQ_ERR("invalid frequency table: %d\n", ret);
		goto init_out;
	}

	policy->min = CFRUC(cpuinfo_min_freq);
	policy->max = CFRUC(scaling_max_freq);

	policy->cpuinfo.transition_latency = transition_latency;
	policy->cur = CFRUC(real_freq_khz);
	/*
	 * The driver only supports the SMP configuartion where all processors
	 * share the clock and voltage and clock.  Use cpufreq affected_cpus
	 * interface to have all CPUs scaled together.
	 */
	policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
	cpumask_setall(policy->cpus);

	cpufreq_frequency_table_get_attr(user_freq_table, policy->cpu);

init_out:
	mutex_unlock(&freq_table_mux);
	return ret;
}

static int cpu0_cpufreq_exit(struct cpufreq_policy *policy)
{
	if (policy->cpu != 0)
		return 0;

	cpufreq_frequency_table_put_attr(policy->cpu);

	return 0;
}

static struct freq_attr cpufreq_freq_attr_vdd;
static struct freq_attr cpufreq_freq_attr_vdd_clk;

static struct freq_attr *cpu0_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	&cpufreq_freq_attr_vdd,
	&cpufreq_freq_attr_vdd_clk,
	NULL,
};

static struct cpufreq_driver cpu0_cpufreq_driver = {
	.flags = CPUFREQ_STICKY,
	.verify = cpu0_verify_speed,
	.target = cpu0_set_target,
	.get = cpu0_get_speed,
	.init = cpu0_cpufreq_init,
	.exit = cpu0_cpufreq_exit,
	.name = "owl_cpu0",
	.attr = cpu0_cpufreq_attr,
};

/**
 * owl_cpufreq_pm_notifier - block CPUFREQ's activities in suspend-resume
 *			context
 * @notifier
 * @pm_event
 * @v
 *
 * While frequency_locked == true, target() ignores every frequency but
 * locking_frequency. The locking_frequency value is the initial frequency,
 * which is set by the bootloader. In order to eliminate possible
 * inconsistency in clock values, we save and restore frequencies during
 * suspend and resume and block CPUFREQ activities. Note that the standard
 * suspend/resume cannot be used as they are too deep (syscore_ops) for
 * regulator actions.
 */
static int owl_cpufreq_pm_notifier(struct notifier_block *notifier,
				       unsigned long pm_event, void *v)
{
	int i;
#ifdef SUSPEND_LOCK_FREQ
	int ret;
#endif

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
	case PM_HIBERNATION_PREPARE:
	case PM_RESTORE_PREPARE:
		mutex_lock(&freq_table_mux);
		frequency_locked = true;
		mutex_unlock(&freq_table_mux);

		disable_nonboot_cpus();

#ifdef SUSPEND_LOCK_FREQ
		ret = owl_cpufreq_scale(cpufreq_cpu_get(0), locking_frequency, CPUFREQ_RELATION_L);
		if (ret < 0)
			return NOTIFY_BAD;
#endif
		break;

	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:
		for (i = 1; i < CONFIG_NR_CPUS; i++)
			cpu_up(i);
		mutex_lock(&freq_table_mux);
		frequency_locked = false;
		mutex_unlock(&freq_table_mux);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block owl_cpufreq_nb = {
	.notifier_call = owl_cpufreq_pm_notifier,
};
int owl_cpu0_cpufreq_driver_init(void)
{
	struct device_node *np;
	int ret;

	np = of_find_node_by_path("/cpus/cpu@0");
	if (!np) {
		CPUFREQ_ERR("failed to find cpu0 node\n");
		return -ENOENT;
	}

	cpu_dev = get_cpu_device(0);
	if (!cpu_dev) {
		CPUFREQ_ERR("failed to get cpu0 device\n");
		ret = -ENODEV;
		goto out_put_node;
	}

	cpu_dev->of_node = np;

	ret = owl_cpu0_clk_init();
	if (ret) {
		CPUFREQ_ERR("failed to get cpu0 clock: %d\n", ret);
		goto out_put_node;
	}
	cpu_reg = devm_regulator_get(cpu_dev, CPU_REGULATOR_NAME);
	if (IS_ERR(cpu_reg)) {
		CPUFREQ_ERR("failed to get cpu0 regulator\n");
		cpu_reg = NULL;
	}

	/*of_init_opp_table*/
	init_cpu0_opp_info();
	find_cpu0_opp_info_max_min_freq(&cpuinfo_max_freq, &scaling_max_freq, &cpuinfo_min_freq);
	select_cur_opp_info();

	ret = CF0AOT_all();
	if (ret)
		goto out_put_clk;

	ret = opp_init_cpufreq_table(cpu0_opp_info_cur->dev, &freq_table);
	if (ret) {
		CPUFREQ_ERR("failed to init cpufreq table: %d\n", ret);
		goto out_put_clk;
	}

	if (of_property_read_u32(np, "clock-latency", &transition_latency)) {
		/* transition_latency = CPUFREQ_ETERNAL; */
		transition_latency = 300 * 1000;
	}

	if (cpu_reg) {
		struct opp *opp;
		unsigned long min_uv, max_uv;
		int i;

		/*
		 * OPP is maintained in order of increasing frequency, and
		 * freq_table initialised from OPP is therefore sorted in the
		 * same order.
		 */
		for (i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++)
			;
		rcu_read_lock();
		opp = opp_find_freq_exact(cpu0_opp_info_cur->dev,
				freq_table[0].frequency * 1000, true);
		min_uv = opp_get_voltage(opp);
		opp = opp_find_freq_exact(cpu0_opp_info_cur->dev,
				freq_table[i-1].frequency * 1000, true);
		max_uv = opp_get_voltage(opp);
		rcu_read_unlock();
#ifdef CONFIG_REGULATOR
		ret = regulator_set_voltage_time(cpu_reg, min_uv, max_uv);
		if (ret > 0)
			transition_latency += ret;
#endif
	}
	freqtable_acitons_cinit(freq_table);

	locking_frequency = cpu0_get_speed(0);
	CPUFREQ_DBG("[%s], locking_frequency:%d\n", __func__, locking_frequency);
	register_pm_notifier(&owl_cpufreq_nb);
	
	ret = cpufreq_register_driver(&cpu0_cpufreq_driver);
	if (ret) {
		CPUFREQ_ERR("failed register driver: %d\n", ret);
		goto out_err_cpufreq;
	}

	of_node_put(np);
	return 0;

out_err_cpufreq:
	unregister_pm_notifier(&owl_cpufreq_nb);
	freqtable_acitons_cfree();
	opp_free_cpufreq_table(cpu0_opp_info_cur->dev, &freq_table);
out_put_clk:
	owl_cpu0_clk_free();
out_put_node:
	of_node_put(np);
	return ret;
}

void owl_cpu0_cpufreq_driver_exit(void)
{
	cpufreq_unregister_driver(&cpu0_cpufreq_driver);
	freqtable_acitons_cfree();
	opp_free_cpufreq_table(cpu0_opp_info_cur->dev, &freq_table);
	owl_cpu0_clk_free();
}

/***for test below*********/
static int find_cpu0_opp_info_max_min_freq_vdd(u32 *max_freq, u32 *scaling_max_freq, u32 *min_freq)
{
	struct cpu0_opp_table *opp_table;

	opp_table = cpu0_opp_info_vdd.cpu0_table;

	*max_freq = opp_table[0].clk;
	*scaling_max_freq = opp_table[0].clk;
	*min_freq = opp_table[0].clk;
	return 0;
}

static ssize_t show_vdd(struct cpufreq_policy *policy, char *buf)
{
	if (cpu0_opp_info_vdd.cpu0_table_num > 0)
		return snprintf(buf, PAGE_SIZE, "vdd=%d, clk=%d\n", (u32)cpu0_opp_info_vdd.cpu0_table[0].volt, (u32)cpu0_opp_info_vdd.cpu0_table[0].clk);
	else
		return snprintf(buf, PAGE_SIZE, "vdd not set yet\n");
}

static ssize_t store_vdd(struct cpufreq_policy *policy, const char *buf, size_t count)
{
	int i, ret;
	u32 vdd;

	vdd = simple_strtoul(buf, NULL, 10) * 1000;

	for (i = 0; i < cpu0_opp_info_user.cpu0_table_num; i++) {
		if (vdd == cpu0_opp_info_user.cpu0_table[i].volt) {
			cpu0_opp_info_vdd = cpu0_opp_info_user;
			cpu0_opp_info_vdd.cpu0_table = &cpu0_opp_info_user.cpu0_table[i];
			cpu0_opp_info_vdd.cpu0_table_num = 1;
			ret = CF0AOT(cpu0_opp_info_vdd.dev, cpu0_opp_info_vdd.cpu0_table,
					cpu0_opp_info_vdd.cpu0_table_num);
			if (ret)
				CPUFREQ_ERR("failed to init OPP table user: %d\n", ret);

			find_cpu0_opp_info_max_min_freq_vdd(&cpuinfo_max_freq, &scaling_max_freq, &cpuinfo_min_freq);
			cpu0_cpufreq_reinit_vdd(&cpu0_opp_info_vdd);
			break;
		}
	}

	if (i == cpu0_opp_info_user.cpu0_table_num) {
		if (cpu0_opp_info_vdd.cpu0_table_num > 0) {
			cpu0_opp_info_vdd.cpu0_table_num = 0;
			find_cpu0_opp_info_max_min_freq(&cpuinfo_max_freq, &scaling_max_freq, &cpuinfo_min_freq);
			cpu0_cpufreq_reinit_vdd(&cpu0_opp_info);
		}
		pr_info("can not find vdd %d in cpu0_opp_info_user, set to default\n", vdd);
	}

	return count;
}

static struct freq_attr cpufreq_freq_attr_vdd = {
	.attr = { .name = "vdd",
		  .mode = S_IRUGO | S_IWUSR,
		},
	.show = show_vdd,
	.store = store_vdd,
};

static u32 user_vdd, user_clk;
static ssize_t show_vdd_clk(struct cpufreq_policy *policy, char *buf)
{
	if ((user_vdd != 0) && (user_clk != 0))
		return snprintf(buf, PAGE_SIZE, "vdd=%d, clk=%d\n", user_vdd, user_clk);
	else
		return snprintf(buf, PAGE_SIZE, "vdd_clk not set yet, please echo vdd(mv),clk(KHz) > vdd_clk. For example:echo 1175,1296 > vdd_clk\n");
}

static ssize_t store_vdd_clk(struct cpufreq_policy *policy, const char *buf, size_t count)
{
	char *end;

	/* get vdd & clk from buf */
	user_vdd = simple_strtol(buf, &end, 10) * 1000;
	end++;
	user_clk = simple_strtoul(end, NULL, 10) * 1000;
	pr_info("user_vdd:%d,user_clk:%d\n", user_vdd, user_clk);

	if (regulator_get_voltage(cpu_reg) < user_vdd)
		regulator_set_voltage(cpu_reg, user_vdd, INT_MAX);

	owl_set_cpu0_clk(user_clk * 1000);

	if (regulator_get_voltage(cpu_reg) > user_vdd)
		regulator_set_voltage(cpu_reg, user_vdd, INT_MAX);

	return count;
}

static struct freq_attr cpufreq_freq_attr_vdd_clk = {
	.attr = { .name = "vdd_clk",
		  .mode = S_IRUGO | S_IWUSR,
		},
	.show = show_vdd_clk,
	.store = store_vdd_clk,
};

static int __init vdd_warp_setup(char *arg)
{
	int err, val;

	err = kstrtoint(arg, 0, &val);
	if(err)
		return err;
	
	vdd_warp = val * CPU_VOLT_STEP_UV;
	pr_alert("vdd_warp set as %duV\n", vdd_warp);
	return 1;
}
__setup("vdd_warp=", vdd_warp_setup);
