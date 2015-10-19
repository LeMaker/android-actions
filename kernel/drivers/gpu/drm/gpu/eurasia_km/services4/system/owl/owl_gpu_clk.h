#include <linux/version.h>

#include <linux/kernel.h>
#include <mach/hardware.h>
#include <asm/delay.h>
#include <linux/clk.h>
#include <mach/module-owl.h>
#include <linux/clk-provider.h>
#include <../liger_clock.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h>

enum gpu_clk_mode {
	GPU_IDLE = 0,
	GPU_NORMAL,
	GPU_PERFORMANCE,
	GPU_HIGH,
	STRATEGY = 0xff,
};

struct asoc_gpu{
	struct kobject kobj;	
	enum gpu_clk_mode clk_mode;	
	u32 normal_clk;
	char normal_pll[20];
	u32 performance_clk;	
	char performance_pll[20];
	u32 high_clk;
	char high_pll[20];
	u32 bactive_clk;
	u32 SGXDeviceIndex;
	u32 ClockChangeCounter;
	long current_coreclk;
	u32 strategy;
};

struct gpu_attribute {
	struct attribute attr;
	ssize_t (*show)(struct asoc_gpu *, char *);
	ssize_t	(*store)(struct asoc_gpu *, const char *, size_t);
};


extern void asoc_gpu_fun_init(void);
extern void owl_gpu_clk_enable(void);
extern void owl_gpu_clk_disable(void);

int asoc_gpu_clk_notify(struct notifier_block *nb, unsigned long event, void *data);
void asoc_gpu_clk_notifier_register(struct notifier_block *notifier);
void asoc_gpu_clk_notifier_unregister(struct notifier_block *notifier);

ssize_t g_c_show(struct asoc_gpu *gpu, char *buf);
ssize_t g_c_store(struct asoc_gpu *gpu,const char *buf, size_t count);
ssize_t gpu_coreclk_show(struct asoc_gpu *gpu, char *buf);
ssize_t cfun_sh(struct asoc_gpu *gpu, char *buf);
ssize_t cfun_st(struct asoc_gpu *gpu,const char *buf, size_t count);
ssize_t gpu_sysclk_show(struct asoc_gpu *gpu, char *buf);
ssize_t gpu_sysclk_store(struct asoc_gpu *gpu,const char *buf, size_t count);
ssize_t gpu_hydclk_show(struct asoc_gpu *gpu, char *buf);
ssize_t gpu_hydclk_store(struct asoc_gpu *gpu,const char *buf, size_t count);
ssize_t gpu_memclk_show(struct asoc_gpu *gpu, char *buf);
ssize_t gpu_memclk_store(struct asoc_gpu *gpu,const char *buf, size_t count);

extern int asoc_gpu_fun_add_attr(struct kobject *dev_kobj);
int asoc_gpu_fun_change(unsigned long *freq_wanted,const char *buf);
int asoc_gpu_fun_change_man(unsigned long *freq_wanted,const char *clkbuf,const char *pllbuf);
int asoc_gpu_fun_active(enum gpu_clk_mode mode);
int asoc_gpu_fun_adj_man(unsigned long freq_wanted,const char *pllbuf);
int asoc_gpu_fun_param_set(u32 freq_in_mhz,const char* pll,enum gpu_clk_mode clk_mode);

int set_gpu_vdd(struct regulator *gpu_regulator,int voltage);
extern int set_gpu_index(u32 index);
int asocPreClockChange(u32 SGXIndex);
int asocPostClockChange(u32 SGXIndex);
long GPU_Coreclk_Get(void);
