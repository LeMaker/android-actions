/*
 * Copyright 2013 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/power_supply.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <asm/system_misc.h>
#include <asm/smp_scu.h>
#include <asm/cacheflush.h>
#include <mach/hardware.h>
#include <mach/power.h>
#include <mach/powergate.h>
#include <linux/of_gpio.h>
#include <linux/mfd/atc260x/atc260x.h>
#include <linux/highmem.h>

#define OWL_BOOT_MAX_PWM_NUM	3
#define CMU_DEVCLKEN1_PWM0_EN	23




int owl_powergate_suspend(void);
int owl_powergate_resume(void);

static int _pmic_warn_null_cb(void)
{
	pr_err("[PM] owl_pmic_pm_ops not registered!\n");
	return -ENODEV;
}
static int _pmic_warn_null_cb_1ui(uint) __attribute__((alias("_pmic_warn_null_cb")));
static int _pmic_warn_null_cb_2ui(uint, uint) __attribute__((alias("_pmic_warn_null_cb")));
static int _pmic_warn_null_cb_3uip(uint*, uint*, uint*) __attribute__((alias("_pmic_warn_null_cb")));
static struct owl_pmic_pm_ops s_pmic_fallback_pm_ops = {
	.set_wakeup_src    = _pmic_warn_null_cb_2ui,
	.get_wakeup_src    = _pmic_warn_null_cb,
	.get_wakeup_flag   = _pmic_warn_null_cb,
	.shutdown_prepare  = _pmic_warn_null_cb,
	.powerdown         = _pmic_warn_null_cb_2ui,
	.reboot            = _pmic_warn_null_cb_1ui,
	.suspend_prepare   = _pmic_warn_null_cb,
	.suspend_enter     = _pmic_warn_null_cb,
	.suspend_wake      = _pmic_warn_null_cb,
	.suspend_finish    = _pmic_warn_null_cb,
	.get_bus_info      = _pmic_warn_null_cb_3uip,
};
static struct owl_pmic_pm_ops *s_pmic_pm_ops = &s_pmic_fallback_pm_ops;
static volatile unsigned long s_pmic_pending_aux_wakeup_src_mask = 0;
static volatile unsigned long s_pmic_pending_aux_wakeup_src_val = 0;

static void (*charger_adjust)(int pre_post, int dev_type, int param);

#define ADAPTER_TYPE_NO_PLUGIN             0
#define ADAPTER_TYPE_WALL_PLUGIN           1
#define ADAPTER_TYPE_PC_USB_PLUGIN         2
#define ADAPTER_TYPE_USB_ADAPTER_PLUGIN    3
#define ADAPTER_TYPE_USB_WALL_PLUGIN       4
#define GPIO_NAME_RESTART_CTRL "restart_ctrl"

#define PM_SYMBOL(symbol)  \
	EXPORT_SYMBOL(symbol)

static int _pm_tmp_judge_adapter_type(void)
{
	pr_err("[PM] judge_adapter_type not registered!\n");
	return -ENODEV;
}
static int (*s_judge_adapter_type)(void) = _pm_tmp_judge_adapter_type;
int set_judge_adapter_type_handle(void *handle)
{
	s_judge_adapter_type = handle;
	return 0;
}
EXPORT_SYMBOL_GPL(set_judge_adapter_type_handle);


void system_powerdown_withgpio(void)
{
	struct device_node *node;
	int restart_ctrl_gpio;
	enum of_gpio_flags flags;

	pr_err("%s\n", __func__);

	sys_sync();
	sys_sync();
	sys_sync();
	sys_sync();
	sys_sync();
	mdelay(1000);

	node = of_find_node_by_name(NULL, "atc260x-power");
	if (!node) {
		pr_err("cann't find power node\n");
		return;
	}

	restart_ctrl_gpio = of_get_named_gpio_flags(node, "system_restart_gpio",
						    0, &flags);
	if (restart_ctrl_gpio < 0) {
		pr_err("cann't find system_restart_gpio\n");
		return;
	}

	if (gpio_request(restart_ctrl_gpio, GPIO_NAME_RESTART_CTRL) < 0) {
		pr_err("%s: restart_ctrl_gpio%d request failed!\n",
			__func__, restart_ctrl_gpio);
		return;
	}

	pr_err("%s, restart_ctrl_gpio =%d, flag=%d\n",
		__func__, restart_ctrl_gpio, flags & OF_GPIO_ACTIVE_LOW);
	gpio_direction_output(restart_ctrl_gpio, flags & OF_GPIO_ACTIVE_LOW);

	while (1)
		;

	return;
}
EXPORT_SYMBOL_GPL(system_powerdown_withgpio);

bool owl_pm_is_battery_connected(void)
{
	int ret;
	struct power_supply *psy;
	union power_supply_propval val;

	/* Check if battery is known */
	psy = power_supply_get_by_name("battery");
	if (psy) {
		ret = psy->get_property(psy, POWER_SUPPLY_PROP_PRESENT, &val);
		if (!ret && val.intval) {
			pr_info("find battery connect\n");
			return true;
		}
	}
	
	pr_info("can not find battery connect\n");
	return false;
}

void owl_pm_halt(void)
{
	int deep_pwrdn, wakeup_src;

	pr_info("[PM] %s %d:\n", __func__, __LINE__);

	s_pmic_pm_ops->shutdown_prepare();

	/* default sleep mode and wakeup source */
	/* DO NOT add HARD_SWITCH source, we are here just because no batt switch. */
	wakeup_src = OWL_PMIC_WAKEUP_SRC_RESET |
		OWL_PMIC_WAKEUP_SRC_ONOFF_LONG;
	
	if(owl_pm_is_battery_connected())
		wakeup_src |= OWL_PMIC_WAKEUP_SRC_WALL_IN | OWL_PMIC_WAKEUP_SRC_VBUS_IN;

	/* if wall/usb is connect, cannot enter S4, only can enter S3 */
	deep_pwrdn = 0;
	if (!power_supply_is_system_supplied())
		deep_pwrdn = 1;

	/* Power off system */
	pr_info("Powering off (wakesrc: 0x%x, deep_pwrdn:%d)\n",
		wakeup_src, deep_pwrdn);

	s_pmic_pm_ops->set_wakeup_src(OWL_PMIC_WAKEUP_SRC_ALL, wakeup_src);
	s_pmic_pm_ops->powerdown(deep_pwrdn, false);

	/* never return to here */
	pr_err("[PM] %s() failed\n", __func__);
}
EXPORT_SYMBOL_GPL(owl_pm_halt);

static void owl_pm_halt_upgrade(void)
{
    int deep_pwrdn, wakeup_src;

    pr_info("[PM] %s %d:\n", __func__, __LINE__);

	s_pmic_pm_ops->shutdown_prepare();

    /* default sleep mode and wakeup source */
    /*xyl :alarm wakesource has some problem, so forbidden it until it's OK*/
    deep_pwrdn = 0;
    wakeup_src = OWL_PMIC_WAKEUP_SRC_RESET |
			OWL_PMIC_WAKEUP_SRC_ONOFF_LONG;

	/* Power off system */
	pr_info("Powering off (wakesrc: 0x%x, deep_pwrdn:%d)\n",
        wakeup_src, deep_pwrdn);

	s_pmic_pm_ops->set_wakeup_src(OWL_PMIC_WAKEUP_SRC_ALL, wakeup_src);
	s_pmic_pm_ops->powerdown(deep_pwrdn, true);

    /* never return to here */
    pr_err("[PM] %s() failed\n", __func__);
}

static void owl_pm_restart(char mode, const char *cmd)
{
	pr_info("[PM] %s() cmd: %s\n", __func__, cmd ? cmd : "<null>");

	s_pmic_pm_ops->shutdown_prepare();

	if (cmd) {
		if (!strcmp(cmd, "recovery")) {
			pr_info("cmd:%s----restart------\n", cmd);
			s_pmic_pm_ops->reboot(OWL_PMIC_REBOOT_TGT_RECOVERY);
		} else if (!strcmp(cmd, "adfu")) {
			pr_info("cmd:%s----restart------\n", cmd);
			s_pmic_pm_ops->reboot(OWL_PMIC_REBOOT_TGT_ADFU);
		} else if (!strcmp(cmd, "reboot")) {
			pr_info("cmd:%s----restart------\n", cmd);
			s_pmic_pm_ops->reboot(OWL_PMIC_REBOOT_TGT_SYS); /* no charger */
		} else if (!strcmp(cmd, "upgrade_halt")) {
			pr_info("cmd:%s----halt------\n", cmd);
    		owl_pm_halt_upgrade();
		}else if (!strcmp(cmd, "bootloader")) {
			pr_info("cmd:%s----restart------\n", cmd);
    		s_pmic_pm_ops->reboot(OWL_PMIC_REBOOT_TGT_BOOTLOADER);
		}else if (!strcmp(cmd, "fastboot")) {
			pr_info("cmd:%s----restart------\n", cmd);
    		s_pmic_pm_ops->reboot(OWL_PMIC_REBOOT_TGT_FASTBOOT);
		}			
	}

	/* fallback to reboot (no charger)*/
	s_pmic_pm_ops->reboot(OWL_PMIC_REBOOT_TGT_SYS);

	pr_err("[PM] %s() failed\n", __func__);
}


struct sleep_save {
	u32 reg;
	u32 val;
};

struct sleep_save owl_reg_save[] = {
//	{.reg = SPS_PG_CTL},
	{.reg = SPS_PWR_CTL},
#if 1
	{.reg = CMU_NANDPLL},
	{.reg = CMU_DISPLAYPLL},
	{.reg = CMU_AUDIOPLL},
	{.reg = CMU_TVOUTPLL},
	{.reg = CMU_SENSORCLK},
	{.reg = CMU_LCDCLK},
	{.reg = CMU_CSICLK},
	{.reg = CMU_DECLK},
	{.reg = CMU_BISPCLK},
	{.reg = CMU_VDECLK},
	{.reg = CMU_VCECLK},
	{.reg = CMU_GPU3DCLK},
	{.reg = CMU_NANDCCLK},
	{.reg = CMU_SD0CLK},
	{.reg = CMU_SD1CLK},
	{.reg = CMU_SD2CLK},
	{.reg = CMU_UART0CLK},
	{.reg = CMU_UART1CLK},
	{.reg = CMU_UART2CLK},
	{.reg = CMU_PWM4CLK},
	{.reg = CMU_PWM5CLK},
	{.reg = CMU_PWM0CLK},
	{.reg = CMU_PWM1CLK},
	{.reg = CMU_PWM2CLK},
	{.reg = CMU_PWM3CLK},
	{.reg = CMU_USBPLL},
	{.reg = CMU_LENSCLK},
	{.reg = CMU_GPU3DCLK},
	{.reg = CMU_ETHERNETPLL},
	{.reg = CMU_CVBSPLL},
	{.reg = CMU_DEVCLKEN0},
	{.reg = CMU_DEVCLKEN1},
	{.reg = CMU_DEVRST0},
	{.reg = CMU_DEVRST1},
	{.reg = CMU_UART3CLK},
	{.reg = CMU_UART4CLK},
	{.reg = CMU_UART5CLK},
	{.reg = CMU_UART6CLK},
#endif
	{.reg = T0_VAL},
	{.reg = T0_CTL},
	{.reg = T0_CMP},
	{.reg = MFP_CTL0},
	{.reg = MFP_CTL1},
	{.reg = MFP_CTL2},
	{.reg = MFP_CTL3},
	{.reg = INTC_EXTCTL},
	{.reg = PAD_PULLCTL0},
	{.reg = PAD_PULLCTL1},
	{.reg = PAD_PULLCTL2},
	{.reg = PAD_DRV0},
	{.reg = PAD_DRV1},
	{.reg = PAD_DRV2},
	{.reg = INTC_GPIOCTL},
	{.reg = GPIO_AOUTEN},
	{.reg = GPIO_AINEN},
	{.reg = GPIO_ADAT},
	{.reg = GPIO_BOUTEN},
	{.reg = GPIO_BINEN},
	{.reg = GPIO_BDAT},
	{.reg = GPIO_COUTEN},
	{.reg = GPIO_CINEN},
	{.reg = GPIO_CDAT},
	{.reg = GPIO_DOUTEN},
	{.reg = GPIO_DINEN},
	{.reg = GPIO_DDAT},
	{.reg = GPIO_EOUTEN},
	{.reg = GPIO_EINEN},
	{.reg = GPIO_EDAT},
	{.reg = INTC_GPIOA_MSK},
	{.reg = INTC_GPIOB_MSK},
	{.reg = INTC_GPIOC_MSK},
	{.reg = INTC_GPIOD_MSK},
	{.reg = INTC_GPIOE_MSK},
	{.reg = INTC_GPIOA_TYPE0},
	{.reg = INTC_GPIOA_TYPE1},
	{.reg = INTC_GPIOB_TYPE0},
	{.reg = INTC_GPIOC_TYPE1},
	{.reg = INTC_GPIOD_TYPE0},
	{.reg = INTC_GPIOD_TYPE1},
	{.reg = INTC_GPIOE_TYPE},
	{.reg = UART0_CTL},
	{.reg = UART1_CTL},
	{.reg = UART2_CTL},
	{.reg = UART3_CTL},
	{.reg = UART4_CTL},
	{.reg = UART5_CTL},
	{.reg = UART6_CTL},
	{.reg = USB3_P0_CTL},
};
struct mfp_t
{
    /* offs: 0x0 */
	unsigned char shift;
    /* offs: 0x1 */
	unsigned char mask;
    /* offs: 0x2 */
	unsigned char val;
    /* offs: 0x3 */
	unsigned char no;
};


struct pwm_config_t
{
    /* offs: 0x0 */
	unsigned long pwm_val;
    /* offs: 0x4 */
	struct mfp_t mfp;
} ;



static struct pwm_config_t owl_pwm_config_saved[3] =
{
/*0*/
		{
			.mfp = {
			.shift = 0,
			.mask = 0,
			.val = 0,
			.no = 0,
				},

		},
/*1*/
		{
			.mfp = {
			.shift = 0,
			.mask = 0,
			.val = 0,
			.no = 0,
			},

		},
/*2*/

		{
			.mfp = {
			.shift = 0,
			.mask = 0,
			.val = 0,
			.no = 0,
			},
		},
};

static unsigned int core_pll, dev_pll, bus_pll, bus1_pll;

static void owl_core_clk_save(void)
{
	core_pll = act_readl(CMU_COREPLL);
	if((core_pll&0x000000ff) > 0x32)
		core_pll = (core_pll&0xffffff00)|0x32;
	dev_pll  = act_readl(CMU_DEVPLL);
	bus_pll  = act_readl(CMU_BUSCLK);
	bus1_pll = act_readl(CMU_BUSCLK1);
}

static void owl_bus_clk_restore(void)
{
#define BUSCLK_CPUCLK_MASK	(0x3)
	unsigned int mask;
	unsigned int busclk = bus_pll & (~BUSCLK_CPUCLK_MASK);
	
	//打开noc时钟通路
#define DEVCLKEN_NOC1			(0x1 << 28)
	act_writel(act_readl(CMU_DEVCLKEN1) | DEVCLKEN_NOC1, CMU_DEVCLKEN1);	
	//将devclk源切到hosc
#define DEVCLKSS_MASK			(0x1 << 12)
#define DEVCLKSS_HOSC			(0x0 << 12)
	act_writel((act_readl(CMU_DEVPLL) & (~DEVCLKSS_MASK)) | DEVCLKSS_HOSC, CMU_DEVPLL);	
	// udelay(5);
	//待频率真正变为hosc时，取消noc分频
#define BUSCLK_DIVEN			(0x1 << 31)
	act_writel(act_readl(CMU_BUSCLK) & (~BUSCLK_DIVEN), CMU_BUSCLK);	
	// udelay(5);
	//取消noc分频后，调整noc分频系数
	mask = ~(BUSCLK_DIVEN | DEVCLKSS_MASK);
	act_writel((act_readl(CMU_BUSCLK) & (~mask)) | (busclk & mask), CMU_BUSCLK);	
	act_writel(bus1_pll, CMU_BUSCLK1);
	// udelay(5);
	//打开noc分频
	act_writel(act_readl(CMU_BUSCLK) | BUSCLK_DIVEN, CMU_BUSCLK);	
	//打开dev_pll并设置dev_pll频率
	act_writel((act_readl(CMU_DEVPLL) & DEVCLKSS_MASK) | (dev_pll & (~DEVCLKSS_MASK)), CMU_DEVPLL);	
	// udelay(70);
	udelay(1);
	//等待pll输出稳定后，将devclk源切到dev_pll
#define DEVCLKSS_DEVPLL			(0x1 << 12)
	act_writel(dev_pll, CMU_DEVPLL);
	// udelay(5);
}

static void owl_cpu_clk_restore(void)
{
	//cpuclk源切为hosc
#define BUSCLK_CPUCLK_MASK		(0x3)
#define BUSCLK_CPUCLK_HOSC		(0x1)
#define BUSCLK_CPUCLK_COREPLL	(0x2)

	act_writel((act_readl(CMU_BUSCLK) & (~BUSCLK_CPUCLK_MASK)) | BUSCLK_CPUCLK_HOSC, CMU_BUSCLK);
	udelay(5);
	//打开corepll，并设置频率
	act_writel(core_pll, CMU_COREPLL);
	udelay(70);
	//等待corepll稳定输出后，将cpuclk源切换为corepll
	act_writel((act_readl(CMU_BUSCLK) & (~BUSCLK_CPUCLK_MASK)) | BUSCLK_CPUCLK_COREPLL, CMU_BUSCLK);
}


void owl_pm_do_save(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(owl_reg_save); i++)
		owl_reg_save[i].val = act_readl(owl_reg_save[i].reg);
}
EXPORT_SYMBOL(owl_pm_do_save);

#if 0
pwm_config_0_pwm_val = <0>;
pwm_config_0_shift = <0>;
pwm_config_0_mask = <0>;
pwm_config_0_val = <0>;
pwm_config_0_no = <0>;
pwm_config_1_pwm_val = <0x102c3f>;
pwm_config_1_shift = <0x26>;
pwm_config_1_mask = <0x7>;
pwm_config_1_val = <0x3>;
pwm_config_1_no = <0x1>;
pwm_config_2_pwm_val = <0x10343f>;
pwm_config_2_shift = <0x26>;
pwm_config_2_mask = <0x7>;
pwm_config_2_val = <0x3>;
pwm_config_2_no = <0x1>;
#endif
void owl_pwm_config_save(void)
{
	int i, offset;
	for(i = 0; i < OWL_BOOT_MAX_PWM_NUM; i++)
	{
		offset = i * 4;
		owl_pwm_config_saved[i].pwm_val = act_readl(offset + PWM_CTL0);
	}
}


void owl_pwm_config_restore(void)
{

	unsigned int i, val, offset;

	for(i = 0; i < OWL_BOOT_MAX_PWM_NUM; i++)
	{
		//owl_printf("afinfo address is 0x%x, pwm_config is 0x%x\n", atc260x_pwm_config, &(atc260x_pwm_config->pwm_config[i]));
		if(owl_pwm_config_saved[i].pwm_val == 0)
			continue;

		printk("pwm%d val=0x%x\n", i, (unsigned int)owl_pwm_config_saved[i].pwm_val);

		val = act_readl(CMU_DEVCLKEN1);
		val |= 1 << (i + CMU_DEVCLKEN1_PWM0_EN);
		act_writel(val, CMU_DEVCLKEN1);

		offset = i * 4;
		act_writel(0x1000, offset + CMU_PWM0CLK);
		act_writel(owl_pwm_config_saved[i].pwm_val, offset + PWM_CTL0);
		act_readl(offset + PWM_CTL0);
	}

	for(i = 0; i < OWL_BOOT_MAX_PWM_NUM; i++)
	{
		if(owl_pwm_config_saved[i].mfp.mask != 0)
		{
			unsigned int mask = (unsigned int)owl_pwm_config_saved[i].mfp.mask << owl_pwm_config_saved[i].mfp.shift;
			unsigned int val = (unsigned int)owl_pwm_config_saved[i].mfp.val << owl_pwm_config_saved[i].mfp.shift;

			offset = owl_pwm_config_saved[i].mfp.no * 4;
			val = (act_readl(offset + MFP_CTL0) & (~mask)) | val;
			act_writel(val, offset + MFP_CTL0);
		}
	}
}

void owl_pm_do_restore(void)
{
	int i;
	act_writel(0, T0_CTL);
	for (i = 0; i < ARRAY_SIZE(owl_reg_save); i++)
		act_writel(owl_reg_save[i].val, owl_reg_save[i].reg);
}
EXPORT_SYMBOL(owl_pm_do_restore);

typedef void (*finish_suspend_t)(ulong cpu_type, ulong pmic_busio, ulong pmic_addr, ulong pmic_type);

//static void switch_jtag(void)
//{
//    act_writel((act_readl(CMU_BUSCLK) & (~0x3)) | 0x1, CMU_BUSCLK);
//    udelay(5);
//    act_writel((act_readl(CMU_COREPLL) & (~0xFF)) | 0x14, CMU_COREPLL);
//    udelay(5);
//    act_writel((act_readl(CMU_BUSCLK) & (~0x3)) | 0x2, CMU_BUSCLK);
//    udelay(5);
//
//    act_writel(act_readl(MFP_CTL1) & (~((0x7<<29) | (0x7<<26))), MFP_CTL1);
//    act_writel((act_readl(MFP_CTL2) & (~((0x3<<5) | (0x3<<7) | (0x7<<11) | (0x7<<17))))
//        | ((0x2<<5) | (0x3<<7) | (0x3<<11) | (0x3<<17)), MFP_CTL2);
//}

#include <asm/sections.h>

//#define STATS_CALC_CHECKSUM_TIME

#define K_TEXT_CSUM_INFO_START_ADDR 0xC0002800
#define K_TEXT_CSUM_INFO_END_ADDR (K_TEXT_CSUM_INFO_START_ADDR + 4)
#define K_TEXT_CSUM_INFO_CSUM (K_TEXT_CSUM_INFO_START_ADDR + 8)

unsigned long *ddr_checksum;
static int train_Msize = -1;
static int train_ces = -1;
static char train_save_buf[2][64];

static void get_ddr_train_option(void)
{
	train_Msize = (((act_readl(DMM_INTERLEAVE_CONFIG) >> 8) & 0x7) + 1) * 256;
	train_ces = ((act_readl(DMM_INTERLEAVE_CONFIG) >> 4) & 0x1) + 1;
	pr_alert("train_Msize:%d, train_ces:%d\n", train_Msize, train_ces);
}

void c_save_ddr_train_area(void)
{
	int train_offset;
	char *vddr;
	
	if(train_Msize <= 0 || train_ces <= 0 || train_ces > 2) {
		while(1) {
			pr_alert("train_Msize:%d, train_ces:%d\n", train_Msize, train_ces);
		}
	}
	
	vddr = kmap_atomic(pfn_to_page(PFN_DOWN(0x2000)));
	memcpy(train_save_buf[0], vddr, 64);
	kunmap_atomic(vddr);
	
	if(train_ces == 2) {		
		if((train_Msize) % 3 != 0) {
			train_offset = train_Msize / 2;	//train_ces为2
		} else {
			train_offset = (train_Msize / 3) * 2;	//train_ces为2
		}
		
		vddr = kmap_atomic(pfn_to_page(PFN_DOWN((train_offset<<20)+0x2000)));
		memcpy(train_save_buf[1], vddr, 64);
		kunmap_atomic(vddr);
	}
}

void c_restore_ddr_train_area(void)
{
	int train_offset;
	char *vddr;
	
	vddr = kmap_atomic(pfn_to_page(PFN_DOWN(0x2000)));
	memcpy(vddr, train_save_buf[0], 64);
	kunmap_atomic(vddr);
	
	if(train_ces == 2) {		
		if((train_Msize) % 3 != 0) {
			train_offset = train_Msize / 2;	//train_ces为2
		} else {
			train_offset = (train_Msize / 3) * 2;	//train_ces为2
		}
		
		vddr = kmap_atomic(pfn_to_page(PFN_DOWN((train_offset<<20)+0x2000)));
		memcpy(vddr, train_save_buf[1], 64);
		kunmap_atomic(vddr);
	}
	flush_cache_all();
}

int c_calc_ddr_checksum(void)
{
#ifdef STATS_CALC_CHECKSUM_TIME
	struct timeval start;
	struct timeval stop;	
	long time_us;
#endif
	unsigned long addr, checksum=0;

	get_ddr_train_option();

#ifdef STATS_CALC_CHECKSUM_TIME	
	do_gettimeofday(&start);
#endif
	for(addr=(unsigned long)_text; addr<(unsigned long)_etext; addr+=4) {
		checksum += *(volatile unsigned long *)addr;
	}
#ifdef STATS_CALC_CHECKSUM_TIME		
	do_gettimeofday(&stop);
	time_us = (stop.tv_sec - start.tv_sec)*1000000 + \
			 (stop.tv_usec- start.tv_usec);
	printk("used time: %ld ms\n", time_us/1000);
#endif
	
	ddr_checksum = (unsigned long *)K_TEXT_CSUM_INFO_START_ADDR;	
	*ddr_checksum = (unsigned long)_text;
	
	ddr_checksum = (unsigned long *)K_TEXT_CSUM_INFO_END_ADDR;
	*ddr_checksum = (unsigned long)_etext;
	
	ddr_checksum = (unsigned long *)K_TEXT_CSUM_INFO_CSUM;
	*ddr_checksum = checksum;
	pr_alert("start:0x%lx, end:0x%lx, checksum:0x%lx\n", (unsigned long)_text, (unsigned long)_etext, *ddr_checksum);
	
	return 0;
}

int c_check_ddr_checksum(void)
{
	unsigned long addr, checksum=0;
	
	for(addr=(unsigned long)_text; addr<(unsigned long)_etext; addr+=4) {
		checksum += *(volatile unsigned long *)addr;
	}
	
	ddr_checksum = (unsigned long *)K_TEXT_CSUM_INFO_CSUM;	
	if(*ddr_checksum != checksum) {
		pr_alert("checksum:0x%lx vs 0x%lx\n", *ddr_checksum, checksum);
		while(1);
	}
	pr_alert("ddr checksum ok\n");
    
	return 0;
}

void owl_switch_jtag(void);

static int owl_cpu_suspend(unsigned long cpu_state)
{
	static const ulong sc_i2c_iobase_tbl[] = {
		/* MMU is disabled when S2 i2c/spi code is call, */
		/* so use physical bus address here. */
		I2C0_BASE, I2C1_BASE, I2C2_BASE, I2C3_BASE,
	};
	static const ulong sc_spi_iobase_tbl[] = {
		SPI0_BASE, SPI1_BASE, SPI2_BASE, SPI3_BASE,
	};
	finish_suspend_t func;
	int ret, cpu_type;
	uint pmic_bus_num, pmic_addr, pmic_type;
	ulong pmic_bus_iobase;

	c_check_ddr_checksum();
	c_save_ddr_train_area();
	
	cpu_type = cpu_package();

	ret = s_pmic_pm_ops->get_bus_info(&pmic_bus_num, &pmic_addr, &pmic_type);
	while(ret);
	while(pmic_bus_num >= 4); /* @_@b */

	func = (finish_suspend_t)0xffff8000;
	switch(pmic_type) {
	case ATC260X_ICTYPE_2603A:
		pmic_bus_iobase = sc_spi_iobase_tbl[pmic_bus_num];
		break;
	case ATC260X_ICTYPE_2603C:
	case ATC260X_ICTYPE_2609A:
		pmic_bus_iobase = sc_i2c_iobase_tbl[pmic_bus_num];
		break;
	default:
		while(1);
	}

	/* switch jtag */
   owl_switch_jtag();

	memcpy((void *)func, (void *)owl_finish_suspend, 0x1000);
	flush_cache_all();

	pr_info("call owl_finish_suspend(%u, 0x%lx, 0x%x, %u)\n",
		cpu_type, pmic_bus_iobase, pmic_addr, pmic_type);
	func(cpu_type, pmic_bus_iobase, pmic_addr, pmic_type);

	return 0;
}

void config_inner_charger_current(int pre_post, int dev_type, int param)
{
	if (charger_adjust)
		(*charger_adjust)(pre_post, dev_type, param);
}
EXPORT_SYMBOL_GPL(config_inner_charger_current);

void pmic_charger_set_fun(void (*funp)(int, int, int))
{
	pr_err("[PM] set charger adjust fun\n");
	charger_adjust = funp;
}
EXPORT_SYMBOL_GPL(pmic_charger_set_fun);

/**
 *      pmic_suspend_set_ops - Set the global suspend method table.
 *      @ops:   Pointer to ops structure.
 */
void owl_pmic_set_pm_ops(struct owl_pmic_pm_ops *ops)
{
	pr_info("[PM] set pmic suspend ops 0x%lx\n", (ulong)ops);
	if(ops == NULL || IS_ERR(ops)) {
		s_pmic_pm_ops = &s_pmic_fallback_pm_ops;
	} else {
		s_pmic_pm_ops = ops;
	}
}
EXPORT_SYMBOL_GPL(owl_pmic_set_pm_ops);

int owl_pmic_setup_aux_wakeup_src(uint wakeup_src, uint on)
{
	uint aux_mask, bitpos;

	aux_mask = OWL_PMIC_WAKEUP_SRC_IR | OWL_PMIC_WAKEUP_SRC_ALARM |
		OWL_PMIC_WAKEUP_SRC_REMCON | OWL_PMIC_WAKEUP_SRC_TP |
		OWL_PMIC_WAKEUP_SRC_WKIRQ | OWL_PMIC_WAKEUP_SRC_SGPIOIRQ;
	if (wakeup_src & ~aux_mask)
		return -EINVAL;

	bitpos = __ffs(wakeup_src); /* one bit per call. */

	set_bit(bitpos, &s_pmic_pending_aux_wakeup_src_mask);
	if (on) {
		set_bit(bitpos, &s_pmic_pending_aux_wakeup_src_val);
	} else {
		clear_bit(bitpos, &s_pmic_pending_aux_wakeup_src_val);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(owl_pmic_setup_aux_wakeup_src);

PM_SYMBOL(owl_cpu_resume);

static void owl_set_wakeup_source(void)
{
	uint wakeup_src, wakeup_mask, append_aux_wakeup_src=0;
	int adapter_type = -1;

	adapter_type = (*s_judge_adapter_type)();
	pr_info("[PM] adapter_type: %d\n", adapter_type);
	if (adapter_type < 0) {
		pr_info("[PM] Err judge_adapter_type\n");
		wakeup_src = OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT;
		wakeup_mask = OWL_PMIC_WAKEUP_SRC_ALL;
	} else {
		switch (adapter_type) {
		case ADAPTER_TYPE_NO_PLUGIN:
			/* no adapter plugged in */
			wakeup_mask = OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT |
				OWL_PMIC_WAKEUP_SRC_VBUS_IN | OWL_PMIC_WAKEUP_SRC_VBUS_OUT |
				OWL_PMIC_WAKEUP_SRC_WALL_IN | OWL_PMIC_WAKEUP_SRC_WALL_OUT |
				OWL_PMIC_WAKEUP_SRC_ALARM;
			wakeup_src = OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT |
				OWL_PMIC_WAKEUP_SRC_ALARM |
				OWL_PMIC_WAKEUP_SRC_VBUS_IN | OWL_PMIC_WAKEUP_SRC_WALL_IN;
			append_aux_wakeup_src = 1;
			break;

		case ADAPTER_TYPE_WALL_PLUGIN:
			/* wall and full charged */
			wakeup_mask = OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT |
				OWL_PMIC_WAKEUP_SRC_VBUS_IN | OWL_PMIC_WAKEUP_SRC_VBUS_OUT |
				OWL_PMIC_WAKEUP_SRC_WALL_IN | OWL_PMIC_WAKEUP_SRC_WALL_OUT |
				OWL_PMIC_WAKEUP_SRC_ALARM;
			wakeup_src = OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT |OWL_PMIC_WAKEUP_SRC_ALARM|
				OWL_PMIC_WAKEUP_SRC_VBUS_IN | OWL_PMIC_WAKEUP_SRC_WALL_OUT;
			append_aux_wakeup_src = 1;
			break;

		case ADAPTER_TYPE_PC_USB_PLUGIN:
			/* usb connected to PC */
			/* same as adapter */
		case ADAPTER_TYPE_USB_ADAPTER_PLUGIN:
			/* usb adapter */
			wakeup_mask = OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT |
				OWL_PMIC_WAKEUP_SRC_VBUS_IN | OWL_PMIC_WAKEUP_SRC_VBUS_OUT |
				OWL_PMIC_WAKEUP_SRC_WALL_IN | OWL_PMIC_WAKEUP_SRC_WALL_OUT |
				OWL_PMIC_WAKEUP_SRC_ALARM;
			wakeup_src = OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT |OWL_PMIC_WAKEUP_SRC_ALARM|
				OWL_PMIC_WAKEUP_SRC_WALL_IN | OWL_PMIC_WAKEUP_SRC_VBUS_OUT;
			append_aux_wakeup_src = 1;
			break;

		case ADAPTER_TYPE_USB_WALL_PLUGIN:
			/* usb adapter & Wall adatper */
			wakeup_mask = OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT |
				OWL_PMIC_WAKEUP_SRC_VBUS_IN | OWL_PMIC_WAKEUP_SRC_VBUS_OUT |
				OWL_PMIC_WAKEUP_SRC_WALL_IN | OWL_PMIC_WAKEUP_SRC_WALL_OUT |
				OWL_PMIC_WAKEUP_SRC_ALARM;
			wakeup_src = OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT |OWL_PMIC_WAKEUP_SRC_ALARM|
				OWL_PMIC_WAKEUP_SRC_VBUS_OUT | OWL_PMIC_WAKEUP_SRC_WALL_OUT;
			append_aux_wakeup_src = 1;
			break;

		default:
			/* something wrong */
			pr_info("[PM] unknown adatper type\n");
			wakeup_src = OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT;
			wakeup_mask = OWL_PMIC_WAKEUP_SRC_ALL;
			break;
		}
	}

	if(!owl_pm_is_battery_connected())
		wakeup_src &= ~(OWL_PMIC_WAKEUP_SRC_WALL_IN | OWL_PMIC_WAKEUP_SRC_WALL_OUT
				| OWL_PMIC_WAKEUP_SRC_VBUS_IN | OWL_PMIC_WAKEUP_SRC_VBUS_OUT);

	/* add onoff long press wakeup source enabled,
	 *    when the adapter is plugged in and the battery is full.
	 */
	wakeup_src |= OWL_PMIC_WAKEUP_SRC_ONOFF_LONG;
	wakeup_mask |= OWL_PMIC_WAKEUP_SRC_ONOFF_LONG;

	if (append_aux_wakeup_src) {
		uint aux_mask = s_pmic_pending_aux_wakeup_src_mask; /* read only once */
		uint aux_val = s_pmic_pending_aux_wakeup_src_val;
		if (aux_mask != 0) {
			pr_info("[PM] append aux wakeup source, mask=0x%x val=0x%x\n",
				aux_mask, aux_val);
			wakeup_mask |= aux_mask;
			wakeup_src |= aux_val;
		}
	}

	pr_warn("[PM] wakesrc: 0x%x, wakeup_mask: 0x%x\n", wakeup_src, wakeup_mask);
	s_pmic_pm_ops->set_wakeup_src(wakeup_mask, wakeup_src);
}

static int owl_pm_prepare_late(void)
{
	pr_info("[PM] %s %d:\n", __func__, __LINE__);
	return s_pmic_pm_ops->suspend_prepare();
}

static int owl_pm_enter(suspend_state_t state)
{
	int ret;

	owl_pwm_config_save();
	owl_core_clk_save();
	ret = s_pmic_pm_ops->suspend_enter();
	if(ret) {
		pr_err("[PM] %s() PMIC enter suspend failed, ret=%d", __func__, ret);
		return ret;
	}
	owl_set_wakeup_source();

	owl_powergate_suspend();
	owl_pm_do_save();

	cpu_suspend(0, owl_cpu_suspend);
	owl_pwm_config_restore();
	owl_bus_clk_restore();
	owl_pm_do_restore();
	owl_cpu_clk_restore();
	
	c_restore_ddr_train_area();
	c_check_ddr_checksum();

	owl_powergate_resume();

#ifdef CONFIG_SMP
	scu_enable((void *)IO_ADDRESS(OWL_PA_SCU));
#endif

	/* DO NOT call s_pmic_pm_ops here, it's not ready at this moment! */

	return 0;
}

static void owl_pm_wake(void)
{
	pr_info("[PM] %s %d:\n", __func__, __LINE__);
	s_pmic_pm_ops->suspend_wake();
	s_pmic_pm_ops->set_wakeup_src(OWL_PMIC_WAKEUP_SRC_ONOFF_SHORT, 0);
}

static void owl_pm_finish(void)
{
	pr_info("[PM] %s %d:\n", __func__, __LINE__);
	s_pmic_pm_ops->suspend_finish();
}

int owl_pm_wakeup_flag(void)
{
	return s_pmic_pm_ops->get_wakeup_flag();
}
EXPORT_SYMBOL_GPL(owl_pm_wakeup_flag);

static const struct platform_suspend_ops owl_pm_ops = {
	.enter = owl_pm_enter,
	.prepare_late = owl_pm_prepare_late,
	.wake = owl_pm_wake,
	.finish = owl_pm_finish,
	.valid = suspend_valid_only_mem,
};
u32 get_phBaseAddr(void)
{
	u32 tmp;
	
	__asm__ __volatile__ ("mrc p15, 4, %0, c15, c0, 0":"=r"(tmp));
	return tmp;
}

// void init_WD_timer(unsigned int load_value, unsigned int auto_reload)
// Sets up the WD timer
// r0: initial load value
// r1:  IF 0 (AutoReload) ELSE (SingleShot)
void init_WD_timer(unsigned int load_value, unsigned int auto_reload)
{
	u32 phBaseAddr, wdCountAddr, wdModeAddr, tmp=0;
   	
   	phBaseAddr = get_phBaseAddr();
   	wdCountAddr = phBaseAddr+0x620;
   	wdModeAddr = phBaseAddr+0x628;
   	
   	act_writel(load_value, wdCountAddr);
   	if(auto_reload == 0)
   	{
   		tmp = 0x2;	
   	}
   	act_writel(tmp, wdModeAddr);
}

// void set_WD_mode(unsigned int mode)
// Sets up the WD timer  
// r0:  IF 0 (timer mode) ELSE (watchdog mode)
void set_WD_mode(unsigned int mode)
{
	u32 phBaseAddr, wdModeAddr;
	
	phBaseAddr = get_phBaseAddr();
   	wdModeAddr = phBaseAddr+0x628;
   	
   	if(mode == 0)
   	{
   		act_writel((act_readl(wdModeAddr) & 0xf7) | 0x4, wdModeAddr);
   	}
   	else
   	{
   		act_writel(act_readl(wdModeAddr) | 0x8, wdModeAddr);
   	}
}

// void start_WD_timer(void)
// Starts the WD timer
void start_WD_timer(void)
{
	u32 phBaseAddr, wdModeAddr;
	
	phBaseAddr = get_phBaseAddr();
   	wdModeAddr = phBaseAddr+0x628;
   	
	act_writel(act_readl(wdModeAddr) | 0x1, wdModeAddr);
}
void cpu_reset_to_brom(void)
{
	local_fiq_disable();
	arch_local_irq_disable(); 	
	init_WD_timer(0xA, 0x01);
	set_WD_mode(1);
	start_WD_timer();
	while(1)
	{
		asm volatile("wfe");
	}
}

int __init owl_pm_init(void)
{
	printk("[PM] %s() %d\n", __func__, __LINE__);
	
	c_calc_ddr_checksum();
	suspend_set_ops(&owl_pm_ops);
	pm_power_off = owl_pm_halt;
	arm_pm_restart = owl_pm_restart;

	return 0;
}
late_initcall(owl_pm_init);

static void __exit owl_pm_remove(void)
{

}
