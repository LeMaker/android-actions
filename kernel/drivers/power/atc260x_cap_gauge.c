/*
 * atc260x_cap_gauge.c  --  fuel gauge  driver for ATC260X
 *
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/gpio.h>
#include <linux/kfifo.h>
#include <linux/rtc.h>

#include <linux/inotify.h>  
#include <linux/suspend.h>
#include <linux/mutex.h>
#include <linux/reboot.h>
#include <asm/div64.h>

#include <linux/list.h>
#include <mach/gpio.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <mach/power.h>
#include <linux/earlysuspend.h>
#include <linux/mfd/atc260x/atc260x.h>
/* If defined, enable printk. */
//#define DEBUG
#ifdef DEBUG
#define GAUGE_DBG(format, arg...)       \
	 printk(format , ## arg)
#else
#define GAUGE_DBG(format, arg...)       \
	 do {} while (0)
#endif

#define GAUGE_ERR(...)				printk(KERN_ERR "ATC260X_GAUGE: " __VA_ARGS__);
#define GAUGE_WARNING(...)		printk(KERN_WARNING "ATC260X_GAUGE: " __VA_ARGS__);
#define GAUGE_NOTICE(...)			printk(KERN_NOTICE "ATC260X_GAUGE: " __VA_ARGS__);
#define GAUGE_INFO(...)			printk(KERN_INFO "ATC260X_GAUGE: " __VA_ARGS__);

/*rtc*/
#define     RTC_H_H_SHIFT               		(0)
#define     RTC_H_H_MASK                		(0x1f << RTC_H_H_SHIFT)
#define     RTC_MS_M_SHIFT             		(6)
#define     RTC_MS_M_MASK               		(0x3f << RTC_MS_M_SHIFT)
#define     RTC_MS_S_SHIFT             		(0)
#define     RTC_MS_S_MASK               		(0x3f << RTC_MS_S_SHIFT)
#define     RTC_H_H(h)                  			(((h) & RTC_H_H_MASK) >> RTC_H_H_SHIFT)
#define     RTC_H_VAL(h)                			(((h) << RTC_H_H_SHIFT))
#define     RTC_MS_M(ms)                		(((ms) & RTC_MS_M_MASK) >> RTC_MS_M_SHIFT)
#define     RTC_MS_S(ms)                		(((ms) & RTC_MS_S_MASK) >> RTC_MS_S_SHIFT)
#define     RTC_MS_VAL(m, s)            		(((m) << RTC_MS_M_SHIFT) | ((s) << RTC_MS_S_SHIFT))

/*ATC2603C_PMU_ICMADC*/
#define PMU_ICMADC_MASK (0x7ff )
#define PMU_ICMADC_SIGN_BIT (1 << 10 )
#define ADC_LSB_FOR_10mohm (1144)/*mA*/
#define ADC_LSB_FOR_20mohm (572)/*mA*/

#define PMU_VER_ABC_RATIO			3
#define PMU_VER_D_RATIO				4
#define PMU_CUR_RATIO_BASE			3


/*current threshold*/
#define CHARGE_CURRENT_THRESHOLD			(60)/*ma*/
#define DISCHARGE_CURRENT_THRESHOLD		(30)/*ma*/

/*full charge, full discharge*/
#define FULL_CHARGE_SOC						(100000)
#define EMPTY_DISCHARGE_SOC				(0)

/*adc up and down float value*/
#define BAT_VOL_VARIANCE_THRESHOLD		(20)/*mv*/
#define BAT_CUR_VARIANCE_THRESHOLD		(100)/*ma*/

/*update resistor, batv range */
#define BATV_RESISTOR_MIN					(3600)/*mv*/
#define BATV_RESISTOR_MAX					(4000)/*mv*/

/*update resistor batv threshold value, when discharging*/
#define UPDATE_RESISTOR_VOL_DIFF_THRESHOLD		(70)/*mv*/

#define FULL_CHARGE_OCV						(4180)/*mv*/
#define CONST_ROUNDING_500					(500)
#define CONST_ROUNDING						(1000)

/*added by cxj@2014-11-26:the max number of current or voltage samples*/
#define SAMPLES_COUNT                20

#define SOC_THRESHOLD 				1000
#define NOT_BEYOND_SOC_THRESHOLD_TIME	 30

#define SAMPLE_OUT 1
#define SAMPLE_TIME 120


#define TERMINAL_VOL_ADD 50

enum REG_TYPE
{
	PMU_SYS_CTL9,
	PMU_OV_INT_EN,
	PMU_BATIADC,
	PMU_IREFADC,
	PMU_ICMADC,
	RTC_MS,
	RTC_H,
};

enum INFO_TYPE
{
	CURRENT_TYPE,
	VOLTAGE_TYPE,
};

enum RSENSE_VALUE
{
	RSENSE_10mohm = 10,
	RSENSE_20mohm = 20
};


/**
 * dts_config_items - dts config items information.user can change these items to meet their
 *                            need.
 * @ capacity : nominal chemical capacity.
 * @ rsense : rsense resistor value(mohm).
 * @ taper_vol : the voltage that is colsed to full charge.
 * @ taper_cur : the current that is colsed to full charge.
 * @ stop_vol : the minimum voltage that tablet works normally.
 * @ print_switch : whether if turn on print switch or not, if true, then print to screen every interval.
 * @ log_switch : whether if turn on log switch or not, if true the log will save in sdcard.
 */
struct dts_config_items
{
	int capacity;
	int icm_available;
	int rsense;
	int taper_vol;
	int taper_current;
	int terminal_vol;
	int min_over_chg_protect_voltage;
	int suspend_current;
	int shutdown_current;
	int print_switch;
	int log_switch;
	
	int ocv_soc_config;
};

/**
 * data_group -  divide into  groups for gathered battery info, current/voltage.
 * @ index : the head index of every group.
 * @ num : the number of every group member.
 * @ count : the number of valid data within the group.
 * @ sum : the sum of all the data within the group.
 * @ avr_data : the average value of this group.
 *
 * data of every group have similar range, not excessive shaking.
 */
struct data_group
{
	int index;
	int num;
	int count;
	int sum;
	int avr_data;
};

/**
  * kfifo_data - the data will store into kfifo.
  * @ bat_vol : battery voltage.
  * @ bat_curr : battery current.
  * @ timestamp : current time stamp.
  */
struct kfifo_data
{
	int bat_vol;
	int bat_curr;
	struct timeval timestamp;
};

/**
 * atc260x_gauge_info - atc260x soft fuel gauge information
 *
 * @ atc260x : come from parent device, atc260x.
 * @ node : device node, in order to get property from dts file.
 * @ cfg_items : config items from dts file.
 * @ lock : prevent the concurrence for soc reading and soc writting.
 * @ firtst_product : whether if the first product.
 * @ current_ratio : current ratio depend on pmu version, if atc2603a abc, current ratio is 4, 
 *     othes version current ratio is 3.
 * @ ch_resistor : battery impedence, dc resistor mainly, under charging. 
 * @ disch_resistor : battery impedence, dc resistor mainly, under discharging.
 * @ curr_buf : save gathered battery current, charge/discharge.
 * @ vol_buf : save gathered battery voltage, charge/discharge.
 * @ ibatt_avg : the average value of curr_buf.
 * @ vbatt_avg : the average value of vol_buf.
 * @ status : there are 3 charge status:CHARGING, DISCHARGING, 
 * 			NO_CHARGEING_NO_DISCHARGING.
 * @ ocv : the open circut voltage of battery.
 * @ ocv_stop : the open circut voltage of battery, corresponding with terminal voltage.  
 * @ soc_last : save last soc.
 * @ soc_now : current  state of charge.
 * @ soc_real : the calclated state of charge really.
 * @ soc_filter : the state of charge filtered by soc_now and soc_real.
 * @ soc_show : the state of charge showing in UI, substracting down_step by soc_now.
 * @ soc_ref : the state of charge corresponding with batv.
 * @ index : indicate the position of soc_queue.
 * @ wq : create work queue for atc260x fuel gauge only.
 * @ work : be responsibe to update battery capacity.
 * @ interval : delayed work poll time interval.
 */
struct atc260x_gauge_info 
{
	struct atc260x_dev *atc260x;
	struct device_node *node;
	struct dts_config_items cfg_items;

	struct mutex lock;
	int store_gauge_info_switch_sav;
	bool first_product;
	int current_ratio;
	int ch_resistor;
	int disch_resistor;
	bool dich_r_change;
	
	int curr_buf[SAMPLES_COUNT];
	int vol_buf[SAMPLES_COUNT];
	int ibatt_avg;
	int vbatt_avg;

	int status;

	int ocv;
	int ocv_stop; 
	
	struct kfifo fifo; 
	struct kfifo down_curve;
	struct kfifo temp_down_curve;
	
	int soc_last;
	int soc_now;
	int soc_real;
	int soc_filter;
	int soc_ref;
	int soc_show;

#ifdef GAUGE_SOC_AVERAGE	
	int index;
	soc_queue[5];
#endif
	struct workqueue_struct *wq;
	struct delayed_work work;
	int interval;
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif

	 int (*filter_algorithm)(int *buf,  int len, int type);

};

extern void act260x_set_get_cap_point(void *ptr);
extern void act260x_set_get_volt_point(void *ptr);
extern void act260x_set_get_cur_point(void *ptr);

extern int atc260x_set_charger_current(int new, int *old);
extern void atc260x_charger_turn_off_force(void);
extern void atc260x_charger_turn_on_force(void);
extern bool atc260x_get_charge_status(void);
extern int get_chg_current_now(void);

static struct atc260x_gauge_info *global_gauge_info_ptr = NULL;
static int first_store_gauge_info;
static int taper_interval;
static int ic_type;
static struct timeval current_tick;
static int ch_resistor_calced = 0;
/*whether if board layout preserved cm related or not*/
static bool board_has_cm = false;
static bool icm_detect_loop;
static int icm_detect_cycle = 0;
static bool test_icm_enable = false;
/*added by cxj @2014-12-04*/
static int cost_time = 0;
/*added by cxj @2014-12-10*/
static bool resume_flag = false;
/* for full power schedule*/
bool full_power_dealing = false;
int next_do_count = 0;
int cycle_count = 0;
/*for suspend consume*/
int pre_charge_status;
static int first_calc_resistor = -1;


static int reg_offset_atc2603a[] =
{	
	0x09, /*0:PMU_SYS_CTL9*/
	0x2e, /*1:PMU_OV_INT_EN*/
	0x41, /*2:PMU_BATIADC*/
	0x4a, /*3:PMU_IREFADC*/
	0xff, /*4:NULL*/
	0x56, /*5:RTC_MS*/
	0x57, /*6:RTC_H*/
};

static int reg_offset_atc2603c[] =
{
	0x09, /*0:PMU_SYS_CTL9*/
	0x2e, /*1:PMU_OV_INT_EN*/
	0x41, /*2:PMU_BATIADC*/
	0x4a, /*3:PMU_IREFADC*/
	0x50, /*4:PMU_ICMADC*/
	0x56, /*5:RTC_MS*/    /*modified by cxj @20141118:wrong address*/
	0x57, /*6:RTC_H*/
};


/**
 *  ocv_soc_table : ocv soc mapping table 
 */
static  int ocv_soc_table[][2] =
{
	{3477, 1}, {3534, 2}, {3591, 3}, {3624, 4}, {3637, 5}, 
	{3649, 6}, {3661, 7},{3667, 8}, {3673, 9},{3677, 10}, 
	{3682, 11}, {3685, 12}, {3690, 13}, {3693, 14}, {3700, 15}, 
	{3706, 16}, {3712, 17}, {3716, 18}, {3722, 19}, {3728, 20},
	{3732, 21}, {3736, 22}, {3739, 23}, {3744, 24}, {3747, 25}, 
	{3751, 26}, {3755, 27}, {3758, 28}, {3761, 29}, {3765, 30},
	{3768, 31}, {3771, 32}, {3775, 33}, {3777, 34}, {3782, 35}, 
	{3784, 36}, {3788, 37}, {3791, 38}, {3793, 39}, {3794, 40},
	{3800, 41}, {3801, 42}, {3804, 43}, {3807, 44}, {3812, 45}, 
	{3815, 46}, {3819, 47}, {3823, 48}, {3825, 49}, {3830, 50},
	{3834, 51}, {3838, 52}, {3841, 53}, {3845, 54}, {3850, 55}, 
	{3854, 56}, {3858, 57}, {3864, 58}, {3870, 59}, {3874, 60},
	{3880, 61}, {3889, 62}, {3895, 63}, {3902, 64}, {3908, 65}, 
	{3916, 66}, {3926, 67}, {3933, 68}, {3940, 69}, {3947, 70},
	{3954, 71}, {3961, 72}, {3968, 73}, {3972, 74}, {3979, 75}, 
	{3985, 76}, {3992, 77}, {3997, 78}, {4005, 79}, {4012, 80},
	{4019, 81}, {4028, 82}, {4036, 83}, {4046, 84}, {4054, 85}, 
	{4061, 86}, {4068, 87}, {4075, 88}, {4084, 89}, {4090, 90},
	{4099, 91}, {4107, 92}, {4115, 93}, {4126, 94}, {4132, 95}, 
	{4141, 96}, {4152, 97}, {4160, 98}, {4170, 99}, {4180, 100},
};  

#define TABLE_LEN 10
static int ocv_soc_table_config[100][2];
static int ocv_soc_table_init(struct atc260x_gauge_info * info)
{
	int i,j;
	int ret;
	u32 ocv[TABLE_LEN];
	int config_items_count,item_number;
	char *config_node_name;
	char temp_item_number[3];
	char *src_item_number;
	temp_item_number[2] = '\0';
	
	for(config_items_count = 0; config_items_count < 10; config_items_count++)
	{
		char start_item_name[11]= "ocv_soc_";
		config_node_name = start_item_name;	
		
		item_number = config_items_count*10;
		temp_item_number[0] = item_number/10+'0';
		temp_item_number[1] = item_number%10+'0';
		src_item_number = temp_item_number;
		
		config_node_name = strcat(start_item_name, src_item_number);
		
		ret = of_property_read_u32_array(info->node,config_node_name,ocv,TABLE_LEN);
		if(ret)
		{
			GAUGE_ERR("get ocv from dts fail!!\n");
			return 0;
		}
		for(i = config_items_count*10, j = 0; i < config_items_count*10+10; i++,j++)
		{
			ocv_soc_table_config[i][0] = ocv[j];
			ocv_soc_table_config[i][1] = i+1;
			GAUGE_DBG("%d  %d\n",ocv_soc_table_config[i][0],ocv_soc_table_config[i][1]);
		}
	}
	info->cfg_items.ocv_soc_config = 1;
	return 1;
}


/* added by cxj @20141010 */
static int get_pmu_ic_type(struct atc260x_gauge_info *gauge)
{
	int pmu_type;
	pmu_type = atc260x_get_ic_type(gauge->atc260x);
	GAUGE_INFO("[%s]pmu type is %d\n", __func__,pmu_type);
	
	return pmu_type;
}

/* added by cxj @20141010 */
static int atc260x_get_version(struct atc260x_gauge_info *gauge)
{
	int pmu_version;
	pmu_version = atc260x_get_ic_ver(gauge->atc260x);
	GAUGE_INFO("[%s]pmu version is %d\n",__func__,pmu_version);
	return pmu_version;
}
static int gauge_reg_read(struct atc260x_dev *atc260x, unsigned short reg)
{
	int value = -EINVAL;

	if (reg == 0xff)
	{
		GAUGE_ERR("[%s]register reading err!\n",__func__);
		return value;
	}	
	
	if (ic_type == ATC260X_ICTYPE_2603A)
		value = atc260x_reg_read(atc260x, reg_offset_atc2603a[reg]);
	else if (ic_type == ATC260X_ICTYPE_2603C)
		value = atc260x_reg_read(atc260x, reg_offset_atc2603c[reg]);
	else
		GAUGE_WARNING("we do not support this ic type!\n");
		
	return value ;
}

static int gauge_reg_write(struct atc260x_dev *atc260x, unsigned short reg,
             unsigned short val)
{
	int ret = -EINVAL;

	if (reg == 0xff)
		return ret;
	
	if (ic_type == ATC260X_ICTYPE_2603A)
		ret = atc260x_reg_write(atc260x, reg_offset_atc2603a[reg], val);
	else if (ic_type == ATC260X_ICTYPE_2603C)
		ret = atc260x_reg_write(atc260x, reg_offset_atc2603c[reg], val);

	return ret;

}

static int store_batt_info(struct atc260x_gauge_info * info)
{
	u8 buf[200];
	struct file *filp;
	mm_segment_t fs;
	int offset = 0;
	int h, ms;

	fs = get_fs();
	set_fs(KERNEL_DS);	

	filp = filp_open("/mnt/sdcard/cap_gauge_info.log", O_CREAT | O_RDWR, 0644);	  
	if (IS_ERR(filp)) 
	{
		GAUGE_ERR("\n[cap_gauge] can't accessed sd card cap_gauge_info.log, exiting");
		return 0;
	}

	if (first_store_gauge_info == 1) 
	{
		memset(buf,0,200);
		offset = sprintf(buf, "time,status,bat_v,bat_i,r_ch,r_disch,bat_ocv,soc_real,soc_now,soc_filter,soc_show,soc_ref,board_has_cm\t\n");
		filp->f_op->llseek(filp, 0, SEEK_END);
		filp->f_op->write(filp, (char *)buf, offset + 1, &filp->f_pos);
		first_store_gauge_info = 0;
	}

	h = gauge_reg_read(info->atc260x, RTC_H);
	if (h < 0) 
	{
		GAUGE_ERR("\n[cap_gauge] Failed to read reg RTC_H: %d", h);
		return h;
	}

	ms = gauge_reg_read(info->atc260x, RTC_MS);
	if (ms < 0) 
	{
		GAUGE_ERR("\n[cap_gauge] Failed to read reg RTC_MS: %d", ms);
		return ms;
	}

	memset(buf,0,200);

	offset = sprintf(buf, "%02d:%02d:%02d,%d,%04d,%04d,%04d,%d,%d,%d,%d,%d,%d,%d,%d\t\n",
	RTC_H_H(h),RTC_MS_M(ms),RTC_MS_S(ms),
	info->status, 
	info->vbatt_avg,
	info->ibatt_avg,
	info->ch_resistor,
	info->disch_resistor,
	info->ocv,
	info->soc_real,
	info->soc_now,
	info->soc_filter,
	info->soc_show,
	info->soc_ref,
	board_has_cm);

	filp->f_op->llseek(filp, 0, SEEK_END);
	filp->f_op->write(filp, (char *)buf, offset + 1, &filp->f_pos);
	set_fs(fs);
	filp_close(filp, NULL);

	return 0;
}

static int  get_cfg_items(struct atc260x_gauge_info *info)
{
	const __be32 *property;
	int len;

	/*capacity*/
	property = of_get_property(info->node, "capacity", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.capacity = be32_to_cpup(property);
		if (info->cfg_items.capacity < 0)
		{
			GAUGE_ERR("[%s] cfg_items.capacity =%d \n", __func__, info->cfg_items.capacity);
			return -EINVAL;
		}
	}
	else
	{
		GAUGE_ERR("[%s] cfg_items.capacity not config\n", __func__);
		return -EINVAL; 
	}
	
	/*icm available*/
	property = of_get_property(info->node, "icm_available", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.icm_available = be32_to_cpup(property);
		GAUGE_DBG("[%s]cfg_items.icm_available =%d \n", __func__,  info->cfg_items.icm_available);
		if(info->cfg_items.icm_available < 0)
		{
			GAUGE_ERR("[%s]	cfg_items.icm_available =%d \n", __func__,  info->cfg_items.icm_available);
			info->cfg_items.icm_available = 0;
			return -EINVAL;
		}
	}
	else
	{
		GAUGE_ERR("[%s] cfg_items.icm_available not config\n", __func__);
		info->cfg_items.icm_available = 0;
		return -EINVAL;
	}
	
	/*rsense*/
	property = of_get_property(info->node, "icm_ohm_val", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.rsense = be32_to_cpup(property);
		if(info->cfg_items.rsense < 0)
		{
			GAUGE_ERR("[%s]	cfg_items.rsense =%d \n", __func__,  info->cfg_items.rsense);
			return -EINVAL;
		}
	}
	else
	{
		GAUGE_ERR("[%s] cfg_items.rsense not config\n", __func__);
		return -EINVAL;
	}

	/*taper_vol*/
	property = of_get_property(info->node, "taper_voltage", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.taper_vol = be32_to_cpup(property);
		if(info->cfg_items.taper_vol < 0)
		{
			GAUGE_WARNING("cfg_items.taper_vol =%d \n", info->cfg_items.taper_vol);
			info->cfg_items.taper_vol = 4180;
		}
	}
	else
	{
		GAUGE_WARNING("cfg_items.taper_vol not config\n");
	}
	/*taper_current*/
	property = of_get_property(info->node, "taper_current", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.taper_current = be32_to_cpup(property);
		if(info->cfg_items.taper_current < 0)
		{
			GAUGE_WARNING("cfg_items.taper_current =%d \n", info->cfg_items.taper_current);
			info->cfg_items.taper_current = info->cfg_items.capacity * 5 / 100;//0.05C
		}
	}
	else
	{
		GAUGE_WARNING("cfg_items.taper_current not config\n");
	}
	/*terminal_vol*/
	property = of_get_property(info->node, "terminal_voltage", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.terminal_vol = be32_to_cpup(property);
		if(info->cfg_items.terminal_vol < 0)
		{
			GAUGE_WARNING("cfg_items.terminal_vol =%d \n", info->cfg_items.terminal_vol);
			info->cfg_items.terminal_vol = 3450;
		}
	}
	else
	{
		GAUGE_WARNING("cfg_items.terminal_vol not config \n");
	}

	/* min_over_chg_protect_voltage */
	property = of_get_property(info->node, "min_over_chg_protect_voltage", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.min_over_chg_protect_voltage = be32_to_cpup(property);
		if(info->cfg_items.min_over_chg_protect_voltage < 0)
		{
			GAUGE_WARNING("cfg_items.min_over_chg_protect_voltage =%d \n", info->cfg_items.min_over_chg_protect_voltage);
			info->cfg_items.min_over_chg_protect_voltage = 4275;
		}
	}
	else
	{
		GAUGE_WARNING("cfg_items.min_over_chg_protect_voltage not config \n");
	}
	/* shutdown_current */
	property = of_get_property(info->node, "shutdown_current", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.shutdown_current = be32_to_cpup(property);
		if(info->cfg_items.shutdown_current < 0)
		{
			GAUGE_WARNING("cfg_items.shutdown_current =%d \n", info->cfg_items.shutdown_current);
			info->cfg_items.shutdown_current = 50;
		}
	}
	else
	{
		GAUGE_WARNING("cfg_items.shutdown_current not config \n");
	}
	/* suspend_current */
	property = of_get_property(info->node, "suspend_current", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.suspend_current = be32_to_cpup(property);
		if(info->cfg_items.suspend_current < 0)
		{
			GAUGE_WARNING("cfg_items.suspend_current =%d \n", info->cfg_items.suspend_current);
			info->cfg_items.suspend_current = 50;
		}
	}
	else
	{
		GAUGE_WARNING("cfg_items.suspend_current not config \n");
	}
	/*print_switch*/
	property = of_get_property(info->node, "print_switch", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.print_switch = be32_to_cpup(property);
		if(info->cfg_items.print_switch < 0)
		{
			GAUGE_WARNING("cfg_items.print_switch =%d \n", info->cfg_items.print_switch);
			info->cfg_items.print_switch = 0;
		}
	}
	else
	{
		GAUGE_WARNING("cfg_items.print_switch not config\n");
	}

	/*log_switch*/
	property = of_get_property(info->node, "log_switch", &len);
	if (property && len == sizeof(int))
	{
		info->cfg_items.log_switch = be32_to_cpup(property);
		if(info->cfg_items.log_switch < 0)
		{
			GAUGE_WARNING("cfg_items.log_switch =%d \n", info->cfg_items.log_switch);
			info->cfg_items.log_switch = 0;
		}
	}
	else
	{
		GAUGE_WARNING("cfg_items.log_switch not config\n");
	}
		
	return 0;
}

static int  get_stored_soc(struct atc260x_gauge_info *info)
{
	int data; 
	
	data = gauge_reg_read(info->atc260x, PMU_SYS_CTL9);
	if (data & 0x8000)	
	{
		data = (data >> 8) & 0x7f;
		GAUGE_DBG("[%s] get sotred soc:%d\n", __func__, data);
		return data;
	}
	else 
	{
		GAUGE_ERR("[%s] not store soc\n", __func__);
		return -EIO;
	}
}

static void store_soc(struct atc260x_gauge_info *info)
{
	int data;
	
	data = gauge_reg_read(info->atc260x, PMU_SYS_CTL9);
	data &= 0xff;
	data |= info->soc_show << 8;
	data |= 0x8000;
	gauge_reg_write(info->atc260x, PMU_SYS_CTL9, data);
}
/* get the time from year of setting time now (minus 1970)
*/
#define GREENWICH_YEAR 1970
#define ATC260X_RTC_NAME			("rtc0")
static unsigned long get_time_hour(struct atc260x_gauge_info *info) 
{
	struct timeval current_tick;
	unsigned long tick;
	struct rtc_time tm;
	struct rtc_device *rtc;
	int year,year_gap;
	
	rtc = rtc_class_open(ATC260X_RTC_NAME);
	if (rtc == NULL) {
		pr_err("%s: unable to open rtc device atc260x\n", __func__);
		return 0;
	}
	rtc_read_time(rtc, &tm);
	rtc_class_close(rtc);
	
	year = tm.tm_year + 1900;
	GAUGE_DBG("current year = %d\n",year);
	year_gap = year - GREENWICH_YEAR - 1;
	
	do_gettimeofday(&current_tick);
	GAUGE_DBG("[%s]timeofday =%lu hours\n",__func__,current_tick.tv_sec/3600);
	tick = current_tick.tv_sec / 3600 - (year_gap * 365 * 24);
	
	return tick;
}
static void store_time_pre_shutdown(struct atc260x_gauge_info *info)
{
	unsigned long systime_hour;
	systime_hour = get_time_hour(info);
	GAUGE_DBG("[%s]hours=%lu\n",__func__,systime_hour);
	/*bit 2~bit15*/
	systime_hour = (systime_hour << 2) | (gauge_reg_read(info->atc260x, PMU_OV_INT_EN)&(0x3));
	GAUGE_DBG("[%s]storing time =%lu\n",__func__,systime_hour);
	gauge_reg_write(info->atc260x, PMU_OV_INT_EN, systime_hour);
}

static int get_stored_time(struct atc260x_gauge_info *info)
{
	int stored_time;
	
	stored_time = gauge_reg_read(info->atc260x, PMU_OV_INT_EN);
	stored_time >>=  2;
	GAUGE_DBG("[%s]sotred time= %d hours\n",__func__,stored_time);
	return stored_time;
}

static bool  first_product(struct atc260x_dev *atc260x)
{
	int pmu_sys_ctl9 = gauge_reg_read(atc260x, PMU_SYS_CTL9);
	GAUGE_DBG("[%s]:pmu_sys_ctl9:0x%x\n", __func__, pmu_sys_ctl9);
	if (pmu_sys_ctl9 & 0x8000)	
	{
		return false;
	}
	else 
	{
		return true;
	}
}


/* modified by cxj @2014-10-31 */
static int  measure_atc2603c_current(struct atc260x_gauge_info *info)
{
	int bat_cur = 0;
	int adc_value;

	adc_value = gauge_reg_read(info->atc260x, PMU_ICMADC) & PMU_ICMADC_MASK;
	
	if(info->cfg_items.rsense == RSENSE_10mohm)
		bat_cur = ((adc_value & 0x3ff) * 4578 / 1024 )/1000;
	else if(info->cfg_items.rsense == RSENSE_20mohm)
		bat_cur = (adc_value & 0x3ff) * 2343 / 1024;
	else
		GAUGE_ERR("cannot find the responding config of resistor!\n");
		
	if(PMU_ICMADC_SIGN_BIT & adc_value)
		bat_cur = -bat_cur;
	else
		bat_cur = bat_cur;
	
	if (((bat_cur >= 0) && (bat_cur <= CHARGE_CURRENT_THRESHOLD)) ||
		((bat_cur <= 0) && (abs(bat_cur) <= DISCHARGE_CURRENT_THRESHOLD)))
	{
		return 0;
	}
	return bat_cur;
}

/**
 * measure_atc2603a_current - mesure atc2603a current, when discharge/charge.
 * 
 * when discharge, there are 2 pathes,  inner mos path and extern mos path, 
 * when battery servers as power supply for syspower,
 * inner_discharge_current / extern_discharge_current as follows table:
 * ---------------------------------------------------------------
 *| inner_current(mA) | inner_current/extern_current | inner_mos/extern_mos |
 *|---------------------------------------------------------------
 *|42                        |42/8                                   |8/42                           |
 *|---------------------------------------------------------------
 *|60                        |60/40                                 |40/60                          |
 *|----------------------------------------------------------------
 *|75                        |80/120                               |80/120                         |
 *|----------------------------------------------------------------
 *|116                      |120/380                              |380/120                       |
 *|----------------------------------------------------------------
 *|165                      |165/635                              |635/165                       |
 *|----------------------------------------------------------------
 *|200                      |200/800                              |800/200                       |
 *|----------------------------------------------------------------
 *
 */
 static int atc260x_read_adc(struct atc260x_dev *atc260x, const char *channel_name)
{
	int ret;
	int translate_data;
	unsigned int channel_num;
	
	channel_num = atc260x_auxadc_find_chan(atc260x,channel_name);
	if(channel_num < 0 )
	{
		GAUGE_ERR("[%s]not support this channel or the channel name is error!\n",__func__);
		return channel_num;
	}
	ret = atc260x_auxadc_get_translated(atc260x,channel_num, &translate_data);
	if(ret < 0)
		GAUGE_ERR("[%s]cannot get the correct translation data!\n",__func__);
	
	return translate_data;
}
static int  measure_atc2603a_current(struct atc260x_gauge_info *info)
{
	int ch_current;
	int disch_current;
	
	ch_current =  atc260x_read_adc(info->atc260x, "CHGI");
	ch_current = ch_current * info->current_ratio /PMU_CUR_RATIO_BASE;

	if (ch_current > CHARGE_CURRENT_THRESHOLD)  /*60ma*/
		return ch_current;
	
	disch_current  =  atc260x_read_adc(info->atc260x, "BATI");
	if (disch_current <= 50)
	{
		goto out;
	}
	else if ((disch_current > 50) &&
		(disch_current <= 60))
	{	
		disch_current = disch_current + disch_current * 40 /60;
	}
	else if ((disch_current > 60) &&
		(disch_current <= 75))
	{	
		disch_current = disch_current + disch_current * 120 /80;
	}
	else if ((disch_current > 75) &&
		(disch_current <= 116))
	{	
		disch_current = disch_current + disch_current * 380 /120;
	}
	else if ((disch_current > 116) &&
		(disch_current <= 165))
	{	
		disch_current = disch_current + disch_current * 635 /165;
	}
	else if ((disch_current > 165) &&
		(disch_current <= 200))
	{	
		disch_current = disch_current + disch_current * 800 /200;
	}
	else
	{
		disch_current = disch_current + disch_current * 800 /200;
	}
	
out:
	if (disch_current > DISCHARGE_CURRENT_THRESHOLD)  /*30ma*/
		return -disch_current;

	return 0;
	
}

int  measure_current(void)
{	
	int bat_curr = -EINVAL;
	if ((ic_type == ATC260X_ICTYPE_2603C) && board_has_cm)
	{
		bat_curr = measure_atc2603c_current(global_gauge_info_ptr);
	}
	else if ((ic_type == ATC260X_ICTYPE_2603A) ||
		((ic_type == ATC260X_ICTYPE_2603C) && (!board_has_cm)))
	{
		bat_curr = measure_atc2603a_current(global_gauge_info_ptr);
	}
	
	return bat_curr;
}

int  measure_iref_adc(void)
{	
	int iref_adc;

	iref_adc = gauge_reg_read(global_gauge_info_ptr->atc260x, 
		PMU_IREFADC);
	
	return   iref_adc;
}


int  measure_vbatt(void)
{
	return   atc260x_read_adc(global_gauge_info_ptr->atc260x, "BATV");
}

static int measure_vbatt_average(void)
{
	int vol_buf[SAMPLES_COUNT];
	int sum = 0;
	int i;
	
	for (i = 0; i < SAMPLES_COUNT; i++ )
	{
		vol_buf[i] = measure_vbatt();
		sum += vol_buf[i];
		usleep_range(2000,2200);
	}

	return sum / SAMPLES_COUNT;
}

static int measure_current_avr(void)
{
	int curr_buf[SAMPLES_COUNT];
	int sum = 0;
	int i;
	
	for (i = 0; i < SAMPLES_COUNT; i++ )
	{
		curr_buf[i] = measure_current();
		sum += curr_buf[i];
		usleep_range(2000,2200);
	}
	return sum / SAMPLES_COUNT;
}

static void get_charge_status(int *status)
{
	int data;
	
	data = measure_current();
	//data = measure_atc2603a_current(global_gauge_info_ptr);

	if (data < 0) 
	{
		*status =  POWER_SUPPLY_STATUS_DISCHARGING;
	}
	else if (data > 0)
	{
		*status = POWER_SUPPLY_STATUS_CHARGING;
	}
	else
	{
		*status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	}

	GAUGE_DBG("[%s] charge status:%d\n", __func__, *status);

}


void batt_info_dump(struct atc260x_gauge_info *info)
{
	printk("\n======================debug information========================\n");
	/* dts config */
	printk("[%s] capacity:%dmAh\n",__func__,info->cfg_items.capacity);
	printk("[%s] icm_available:%d\n",__func__,info->cfg_items.icm_available);
	printk("[%s] rsense:%dmohm\n",__func__,info->cfg_items.rsense);
	printk("[%s] taper_vol:%dmV\n",__func__,info->cfg_items.taper_vol);
	printk("[%s] taper_current:%dmA\n",__func__,info->cfg_items.taper_current);
	printk("[%s] terminal_vol:%dmV\n",__func__,info->cfg_items.terminal_vol);
	printk("[%s] min_over_chg_protect_voltage:%dmV\n",__func__,info->cfg_items.min_over_chg_protect_voltage);
	printk("[%s] suspend_current:%duA\n",__func__,info->cfg_items.suspend_current);
	printk("[%s] shutdown_current:%duA\n\n",__func__,info->cfg_items.shutdown_current);
	/* interval */
	printk("[%s] interval:%ds\n",__func__,info->interval);
	/* charge status */
	if (info->status == POWER_SUPPLY_STATUS_NOT_CHARGING)
		printk("[%s] charger status:POWER_SUPPLY_STATUS_NOT_DISCHARGING\n", __func__);
	else if (info->status == POWER_SUPPLY_STATUS_CHARGING)
		printk("[%s] charger status:POWER_SUPPLY_STATUS_CHARGING\n", __func__);
	else if (info->status == POWER_SUPPLY_STATUS_DISCHARGING)
		printk("[%s] charger status:POWER_SUPPLY_STATUS_DISCHARGING\n", __func__);
	/* battery */
	printk("[%s] measure bat voltage:%dmv\n", __func__, measure_vbatt());
	printk("[%s] measure charger/discharge current:%dmA\n", __func__, measure_current());
	printk("[%s] charge resistor:%dmohm\n", __func__, info->ch_resistor);
	printk("[%s] discharge resistor:%dmohm\n", __func__, info->disch_resistor);
	printk("[%s] ocv:%dmv\n", __func__, info->ocv);
	printk("[%s] ocv stop:%dmv\n", __func__, info->ocv_stop);
	printk("[%s] real soc:%d\n", __func__, info->soc_real);
	printk("[%s] now soc:%d\n", __func__, info->soc_now);
	printk("[%s] filter soc:%d\n", __func__, info->soc_filter);
	printk("[%s] show soc:%d\n", __func__, info->soc_show);
	printk("[%s] stopped soc:%d\n", __func__, info->soc_ref);
	printk("[%s] board_has_cm:%d\n",__func__,board_has_cm);
	printk("\n===============================================================\n");
}
static int get_responding_ocv(struct atc260x_gauge_info *info,int soc)
{
	int i;
	int count;
	int ocv_finded;
	int soc_finded;
	int ocv_add;
	int (*ocv_soc_table_p)[2];
	
	if(info->cfg_items.ocv_soc_config)
		ocv_soc_table_p = ocv_soc_table_config;
	else
		ocv_soc_table_p = ocv_soc_table;

	count = ARRAY_SIZE(ocv_soc_table);
	for (i = count -1; i >= 0; i--)
	{
		soc_finded = (*(ocv_soc_table_p + i))[1];

		if (soc >= soc_finded*1000)
		{
			if (i == count - 1)
			{
				ocv_finded = FULL_CHARGE_OCV;
				break;
			}
			ocv_finded = (*(ocv_soc_table_p + i))[0];
			ocv_add = (soc - soc_finded*1000) * ((*(ocv_soc_table_p + i + 1))[0] - ocv_finded)/1000;
			ocv_finded += ocv_add;
			GAUGE_DBG("ocv_finded=%d,ocv_add=%d\n",ocv_finded,ocv_add);
			break;
		}
	}
	
	return ocv_finded;
}
static void update_discharge_resistor(struct atc260x_gauge_info *info,int soc)
{
	int batv = measure_vbatt_average();
	int bati = -measure_current_avr();
	int resp_ocv = get_responding_ocv(info,soc);
	GAUGE_DBG("resp_ocv=%d,batv=%d,bati=%d\n",resp_ocv,batv,bati);
	if (resp_ocv > batv)
	{
		info->disch_resistor = (resp_ocv - batv)*1000/bati;
		if (info->disch_resistor > 500)
			info->disch_resistor = 500;
		else if (info->disch_resistor < 80)
			info->disch_resistor = 80;
	}
	else
		GAUGE_ERR("[%s]err:resp_ocv=%d,batv=%d!!",__func__,resp_ocv,batv);
	GAUGE_DBG("[%s]disch_resistor=%d\n",__func__,info->disch_resistor);
}

#define SLIP_DOWN_TIME  30
#define TERMINAL_VOLTAGE_ADD 0
/*smooth for discharge curve*/
static void down_curve_smooth(struct atc260x_gauge_info *info, int *soc)
{
	int down_step = 0;
	int down_step_diff;
	int weight;
	int vbatt_avg;
	int bati_avg;
	int bat_full;
	int data;

	/*calc down step, when battery voltage is low*/
	vbatt_avg = measure_vbatt_average();
	bati_avg = -measure_current_avr();
	if (bati_avg < 0)
	{
		down_step = 0;
		goto out;
	}
	
	if (vbatt_avg <= info->cfg_items.terminal_vol - 50)
	{
		GAUGE_INFO("[%s] battery voltage is %dmv\n", __func__, vbatt_avg);
		down_step = *soc;
		goto out;
	}
		
	bat_full = FULL_CHARGE_OCV - bati_avg * 250 / 1000;
	if (vbatt_avg >= bat_full)
		weight = 1000;
	else if (vbatt_avg > info->cfg_items.terminal_vol-TERMINAL_VOLTAGE_ADD)
	{
		weight = 1000 * ( vbatt_avg - info->cfg_items.terminal_vol+TERMINAL_VOLTAGE_ADD) /(bat_full - info->cfg_items.terminal_vol+TERMINAL_VOLTAGE_ADD);
	}
	else
		weight = 0;
	info->soc_ref = FULL_CHARGE_SOC / 1000 * weight; 
	info->soc_filter = (info->soc_real * weight  + info->soc_ref * (1000 - weight)) / 1000 ;
	GAUGE_DBG("weight=%d,soc_ref=%d,soc_filter=%d,soc_real=%d\n",
				weight,info->soc_ref,info->soc_filter,info->soc_real);
	
	if ((info->ocv >= FULL_CHARGE_OCV || info->soc_ref == FULL_CHARGE_SOC) && info->soc_now == FULL_CHARGE_SOC)
	{
		down_step = 0;
		goto out;
	}
	
	if (info->ocv <= 3200 + info->disch_resistor * 2000 /1000)
	{
		data = -measure_current();
		GAUGE_DBG("[%s] discharge current is %dmA\n", __func__, data);
		if (data >= 2000)
		{
			down_step = 1000;
			goto out;
		}	
	}

	/*calc down step, when battery voltage is high*/
	/*calculate the entire battery duration time(s) according to current system consumption*/
	
	if (!bati_avg) {
		down_step = 0;
		goto out;
	}
	down_step = (info->cfg_items.capacity * 3600) / bati_avg;
	GAUGE_DBG("[%s] all_time=capacity *3600/bat_cur=%d*3600/%d=%d\n", 
			__func__, info->cfg_items.capacity, bati_avg, down_step);
	/*calc reduced soc per second*/
	down_step = FULL_CHARGE_SOC /down_step;
	GAUGE_DBG("[%s] soc_per_sec=FULL_CHARGE_SOC/all_time=%d/all_time=%d\n", 
			__func__, FULL_CHARGE_SOC, down_step);
	/*calc the down step during interval*/
	down_step = down_step * info->interval;
	GAUGE_DBG("[%s] down_step=soc_per_sec*interval=soc_per_sec*%d=%d\n", 
			__func__,info->interval, down_step);
	
	if (info->soc_filter < info->soc_now)
	{
		if (info->soc_filter > 10000)
		{
			down_step_diff = (info->soc_now - info->soc_filter)/ (60 /info->interval);
			down_step = (down_step > down_step_diff) ? down_step:down_step_diff;
			if (info->soc_now - info->soc_filter > 10000)
			{
				GAUGE_DBG("soc_now>soc_filter+10%:down_step=0.5%\n");
				down_step = 500;
			}
			else
			{
				if (down_step >= 250)
					down_step = 250;
			}
		}
		else
		{
			down_step_diff = (info->soc_now - info->soc_filter)/ (30 /info->interval);
			down_step = (down_step > down_step_diff) ? down_step:down_step_diff;
			if (down_step >= 500)
				down_step = 500;
		}
			
		GAUGE_DBG("[%s]soc_filter(%d) < soc_now(%d),down_step=%d\n",__func__,info->soc_filter,info->soc_now,down_step);
	}
	else
	{
		if (info->soc_filter - info->soc_now > 2000)
		{
			GAUGE_DBG("soc_filter>soc_now+2000:R decrease!!");
			update_discharge_resistor(info,info->soc_now );
			down_step = 0;
		}
	}
	if (info->soc_real > info->soc_filter + 250)
	{
		GAUGE_DBG("soc_real>soc_filter:R decrease!!");
		update_discharge_resistor(info,info->soc_filter);
	}
	if (info->soc_now > info->soc_real + 250)
	{
		GAUGE_DBG("soc_now>soc_real:R increase!!");
		update_discharge_resistor(info,info->soc_now);
		info->dich_r_change = true;
	}

	if (down_step < 0)
		down_step = 0;
	else
	{
		if (info->soc_now - down_step < 1000)
			down_step = 0;
	}
out:
	*soc = *soc - down_step;
	GAUGE_DBG("[%s] down_step=%d, soc:%d\n", __func__, down_step, *soc);
}

static void soc_post_process(struct atc260x_gauge_info *info)
{
	int soc_last;
	
	if (info->soc_now  > FULL_CHARGE_SOC) 
	{
		info->soc_now = FULL_CHARGE_SOC;
	} 
	else if (info->soc_now < EMPTY_DISCHARGE_SOC) 
	{
		info->soc_now = EMPTY_DISCHARGE_SOC;
	}
	
	mutex_lock(&info->lock);
	info->soc_show = info->soc_now / 1000;	
	mutex_unlock(&info->lock);

	/*
	 * chenbo@20150514 
	 * save last soc
	 */
	soc_last = get_stored_soc(info);
	if ((soc_last >= 0) && 
		soc_last - info->soc_show > 1) {
		info->soc_last = soc_last;
	}
	
	store_soc(info);

	if (info->soc_show % 5)
		ch_resistor_calced = 0;
		
	if (info->cfg_items.print_switch)
	{
		batt_info_dump(info);
	}
	
	if (info->cfg_items.log_switch)
	{
		store_batt_info(info);
	}
	
}

static int get_threshold(int type)
{
	int threshold = 0 ;
	
	if (type == CURRENT_TYPE)
	{
		threshold = BAT_CUR_VARIANCE_THRESHOLD;
	}
	else if (type == VOLTAGE_TYPE)
	{
		threshold = BAT_VOL_VARIANCE_THRESHOLD;
	}
	else
		GAUGE_INFO("the parameter of 'type' do not support!\n");

	return threshold;
}

/*
 * filter for gathered batt current and batt voltage, including 2 steps:
 * step1 : divide the buf which lengh is len into several groups, 
 *            every group have similar data range;
 * step2: filterd for ervery group.
 */
static int filter_algorithm1(int *buf,  int len, int type)
{
	struct data_group group[10];
	int threshold = 0;
	int i;
	int j;
	int k;
	
    	if (!buf) 
	{
        		return -EINVAL;
    	}

	threshold = get_threshold(type);
		
	/*divided the data into several group*/
	group[0].index = 0;
	group[0].num = 1;
    	for (i = 1, j = 0;  i < len; i++) 
    	{
		if (abs(buf[i] - buf[i - 1]) > threshold)
		{
			group[++j].index = i;
		}

		group[j].num = (i + 1) - group[j].index;
    	}

	/*handle  every group*/
	for (i = 0; i < sizeof(group) / sizeof(struct data_group); i++)
	{
		GAUGE_DBG("group[%d].index=%d, group[%d].num=%d\n", i, group[i].index, i, group[i].num);
		
		if (group[i].num >= 5)
		{
			for (j = group[i].index; j <= (group[i].index + group[i].num + 1) /2; j++)
			{
				group[i].sum = buf[j];
				group[i].count = 1;
				for (k = j; k < group[i].index + group[i].num; k++)
				{
					if (abs(buf[k + 1] - buf[j]) < threshold)
					{
						group[i].sum+= buf[k + 1];
						group[i].count++;
						GAUGE_DBG(" buf[%d]:%d\n", k + 1,  buf[k + 1]);
					}
				}

				if (group[i].count >= (group[i].num + 1) / 2)
				{
					group[i].avr_data = group[i].sum  / group[i].count ;
					GAUGE_DBG("[%s] Average cur/vol=%d/%d=%d\n", 
						__func__, group[i].sum, group[i].count, group[i].avr_data);

					return group[i].avr_data;
				}
			}

			
		}

	}

	return -EINVAL;
}

/*filter for gathered batt current and batt voltage.
 * note : the process of filter_algorithm2 is looser than filter_algorithm1.
 
static int filter_algorithm2(int *buf,  int len, int type)
{
	int threshold = 0;
	int avr_data;
	int count;
	int sum;
	int j;
	int k;

	if (!buf) 
	{
        		return -EINVAL;
    	}
	
	threshold = get_threshold(type);
	
	for (j = 0; j <= (len + 1) /2; j++)
	{
		sum = buf[j];
		count = 1;
		for (k = j; k < len; k++)
		{
			if (abs(buf[k + 1] - buf[k]) < threshold)
			{
				sum+= buf[k + 1];
				count++;
			}
		}

		if (count >= (len + 1) / 2)
		{
			avr_data = sum  / count ;
			GAUGE_DBG("[%s] Average cur/vol=%d/%d=%d\n", 
				__func__, sum, count, avr_data);

			return avr_data;
		}
	}

	return -EINVAL;
}
*/
/*by testing ,we know that the current change depend the scene,but always not hardly
 *we just calculate the data near the average value
 *the one apart from the average by 100 will be discarded 
 */ 
 
 static int filter_algorithm3(int *buf,  int len, int type)
 {
	int threshold = 0;
	int avr_data;
	int count = 0;
	int sum = 0;
	int j;
	int k;
	
	if (!buf) 
        return -EINVAL;
	
	threshold = get_threshold(type);
	
	for(j = 0;j < len;j++)
		sum += buf[j];
	avr_data = sum / len;
	for(k = 0;k < len;k ++)
	{
		if(abs(buf[k] - avr_data) < threshold)
			count ++;
		else
			sum -= buf[k];
	}
	GAUGE_DBG("available data count:%d\n",count);
	if(count > len * 2 / 3)
		avr_data = sum / count;
	else
		return -EINVAL;
	
	return avr_data;
 }

static int filter_process(struct atc260x_gauge_info *info)
{
	struct kfifo_data fifo_data;
	
	fifo_data.bat_vol = 
		info->filter_algorithm(info->vol_buf, SAMPLES_COUNT, VOLTAGE_TYPE);
	fifo_data.bat_curr = 
		info->filter_algorithm(info->curr_buf, SAMPLES_COUNT, CURRENT_TYPE);
	fifo_data.timestamp.tv_sec = current_tick.tv_sec;
	fifo_data.timestamp.tv_usec = current_tick.tv_usec;
	GAUGE_DBG("[%s] the latest value: %d(vol), %d(cur)\n",
		__func__, fifo_data.bat_vol, fifo_data.bat_curr);
	
	/* modified by cxj@20141029 */
	if ((fifo_data.bat_vol == -EINVAL) ||
		(fifo_data.bat_curr == -EINVAL))
		return -EINVAL;
	
	kfifo_in(&info->fifo, &fifo_data, sizeof(struct kfifo_data));

	return 0;
}

/*
 * gather battery info, including voltage and current.
 */
static void  gather_battery_info(struct atc260x_gauge_info *info)
{
	int i;

	do_gettimeofday(&current_tick);
	
	for (i = 0; i < SAMPLES_COUNT; i++ )
	{
		info->vol_buf[i] = measure_vbatt();
		info->curr_buf[i] = measure_current();
		usleep_range(2000,2200);
	}
}


/* calculate resistor in charging case */
static int calc_charge_resistor(struct atc260x_gauge_info *info)
{
	struct kfifo_data fifo_data[2];
	int reg_chg_current;
	int data;
	int chg_current;
	int ret;

	chg_current = measure_current_avr();
	GAUGE_DBG("chg_current=%d\n",chg_current);
	if (chg_current < 0)
	{
		GAUGE_ERR("measure_current :data <0");
		goto out;
	}
	
	if (chg_current >= 500)
	{
		atc260x_set_charger_current(500, &reg_chg_current);
		msleep(2000);
		gather_battery_info(info);
		ret = filter_process(info);
		
		if (ret)
		{
			GAUGE_ERR("[%s]filter_process failed!\n",__func__);
			goto out;
		}
		
		atc260x_set_charger_current(100, &ret);
		msleep(1500);
		
		gather_battery_info(info);
		ret = filter_process(info);
		
		if (ret)
		{
			GAUGE_ERR("[%s]filter_process failed!\n",__func__);
			goto out;
		}
		
		atc260x_set_charger_current(reg_chg_current, &ret);
		msleep(500);
	}
	else
	{
		reg_chg_current = get_chg_current_now();
		gather_battery_info(info);
		ret = filter_process(info);
		GAUGE_DBG("[%s]turn off charger!\n",__func__);
		atc260x_charger_turn_off_force();
		msleep(500);
		
		gather_battery_info(info);
		ret = filter_process(info);
		
		if (ret)
		{
			GAUGE_ERR("[%s]filter_process failed!\n",__func__);
			goto out;
		}
		
		atc260x_set_charger_current(reg_chg_current, &ret);
		GAUGE_DBG("[%s]turn on charger!\n",__func__);
		atc260x_charger_turn_on_force();
		msleep(500);
	}

	if (kfifo_is_full(&info->fifo))
	{
		ret = kfifo_out(&info->fifo, &fifo_data[0], sizeof(struct kfifo_data));
		GAUGE_DBG("[%s] %d(vol), %d(cur), dequeue len:%d\n", 
			__func__, fifo_data[0].bat_vol, fifo_data[0].bat_curr, ret);
		ret = kfifo_out(&info->fifo, &fifo_data[1], sizeof(struct kfifo_data));
		GAUGE_DBG("[%s] %d(vol), %d(cur), dequeue len:%d\n", 
			__func__, fifo_data[1].bat_vol, fifo_data[1].bat_curr, ret);

		if ((fifo_data[0].bat_vol > fifo_data[1].bat_vol) && 
			(fifo_data[0].bat_curr > fifo_data[1].bat_curr)) 
		{
			/* calculate resistor :mohm*/
			data = 1000 * (fifo_data[0].bat_vol  - fifo_data[1].bat_vol)
			    / (fifo_data[0].bat_curr - fifo_data[1].bat_curr);
				
			GAUGE_DBG("fifo_data[0].bat_vol = %d, fifo_data[1].bat_vol = %d\n",fifo_data[0].bat_vol,fifo_data[1].bat_vol);
			GAUGE_DBG("fifo_data[0].bat_curr = %d, fifo_data[1].bat_curr = %d\n",fifo_data[0].bat_curr,fifo_data[1].bat_curr);
			GAUGE_DBG("here ch_resistor = %d\n",data);
			
			if (data <= 500)
				info->ch_resistor = data;
			else
				info->ch_resistor = 500;
			GAUGE_DBG("[%s] the latest charge resistor is %d\n", __func__, info->ch_resistor);

			return 0;
		}
	}

out:
	kfifo_reset(&info->fifo);
	return -EINVAL;
}

/* Calculate Open Circuit Voltage */
static int calc_ocv(struct atc260x_gauge_info *info)
{
	int i;
	int vbatt_sum;
	int ibatt_sum;
	int count = 0;

	switch (info->status)
	{
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		for (i = 0, vbatt_sum = 0; i < SAMPLES_COUNT; i++) 
		{
			vbatt_sum += info->vol_buf[i];
		}
		
		info->vbatt_avg = vbatt_sum / SAMPLES_COUNT;
		info->ocv = info->vbatt_avg;
		info->ocv_stop = info->cfg_items.terminal_vol+TERMINAL_VOL_ADD;
		info->ibatt_avg = 0;
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
	case POWER_SUPPLY_STATUS_DISCHARGING:
		for (i = 0, ibatt_sum = 0, vbatt_sum = 0; i < SAMPLES_COUNT; i++) 
		{
			vbatt_sum += info->vol_buf[i];
			if (info->status == POWER_SUPPLY_STATUS_CHARGING)
			{
				if (info->curr_buf[i] > 0)
				{
					ibatt_sum += info->curr_buf[i];
					count ++;
				}
			}
			else if (info->status == POWER_SUPPLY_STATUS_DISCHARGING)
			{
				if (info->curr_buf[i] < 0)
				{
					ibatt_sum += -info->curr_buf[i];
					count ++;
				}
			} 
			
		}
		info->vbatt_avg = vbatt_sum / SAMPLES_COUNT;
		/*added by cxj@20141101:division cannot be zero*/
		if(count != 0)
			info->ibatt_avg = ibatt_sum / count;
		else      /*if zero ,battery is of full power*/
		{
	      /*modified by cxj@20141113:cannot return -EINVAL,because of full capacity*/
			info->ocv = info->vbatt_avg;
			info->ibatt_avg = 0;
			return 0; 
		}
		
		if (info->status == POWER_SUPPLY_STATUS_CHARGING)
		{
			info->ocv = info->vbatt_avg - info->ibatt_avg * info->ch_resistor / 1000;
			GAUGE_DBG("[%s] ocv:%d\n", __func__, info->ocv);
		}
		else if (info->status == POWER_SUPPLY_STATUS_DISCHARGING)
		{	
			info->ocv = info->vbatt_avg + info->ibatt_avg * info->disch_resistor / 1000;
			info->ocv_stop = info->cfg_items.terminal_vol + TERMINAL_VOL_ADD + info->ibatt_avg * info->disch_resistor / 1000;
			GAUGE_DBG("[%s] ocv:%d, ocv stop:%d\n", 
				__func__, info->ocv, info->ocv_stop);
		}
		break;
	default:
		GAUGE_WARNING("[%s] charging status err!\n" ,__func__);
		return -EINVAL;
	}
	
	return 0;
	
}

/* Calculate State of Charge (percent points) */
static void calc_soc(struct atc260x_gauge_info *info, int ocv, int *soc)
{
	int i;
	int count;
	int soc_finded;
	int (*ocv_soc_table_p)[2];
	
	if(info->cfg_items.ocv_soc_config)
		ocv_soc_table_p = ocv_soc_table_config;
	else
		ocv_soc_table_p = ocv_soc_table;
		
	if(ocv < (*ocv_soc_table_p)[0])
	{
		*soc = 0;
		GAUGE_DBG("[%s] ocv:%d, soc:0(ocv is less than the minimum value, set soc zero)\n", 
			__func__, ocv);
		return;
	}
	count = ARRAY_SIZE(ocv_soc_table);
	
	for (i = count -1; i >= 0; i--) 
	{
		if (ocv >= (*(ocv_soc_table_p + i))[0])
		{
			if (i == count - 1)
			{
				*soc = FULL_CHARGE_SOC;
				break;
			}
			soc_finded = (*(ocv_soc_table_p + i))[1];
			GAUGE_DBG("soc_finded=%d\n",soc_finded);
			*soc = soc_finded*1000 + (ocv - (*(ocv_soc_table_p + i))[0])*
					((*(ocv_soc_table_p + i + 1))[1]-soc_finded)*1000/
					(((*(ocv_soc_table_p + i + 1))[0]) - (*(ocv_soc_table_p + i))[0]);
			GAUGE_DBG("[%s]ocv:%d, calc soc is %d\n", __func__, ocv, *soc);
			break;
		}
	}

}

#ifdef GAUGE_SOC_AVERAGE
static int soc_average(struct atc260x_gauge_info *info,  int soc)
{
	int soc_sum = 0;
	int soc_avr;
	int i;
	int size;
	
	size = sizeof(info->soc_queue) / sizeof(int);
	
	info->soc_queue[info->index++ % size] = soc;

	if (info->index < size -1)
	{
		return soc;
	}

	for (i = 0; i < size; i++)
	{
		soc_sum += info->soc_queue[i];
		GAUGE_DBG("[%s] info->soc_queue[%d] = %d\n", __func__, i,  info->soc_queue[i]);	
	}

	info->index = 0;
	soc_avr = soc_sum / size;
	GAUGE_DBG("[%s] average soc = %d /%d = %d\n", __func__, soc_sum, size, soc_avr);	

	return soc_avr;
}
#endif

static int generate_poll_interval(struct atc260x_gauge_info *info)
{
	int data;
	
	data = measure_vbatt_average();
	
	if (data >= info->cfg_items.terminal_vol + 100)
	{
		if (info->soc_show == 99 || info->soc_real == FULL_CHARGE_SOC || full_power_dealing)
		{
			info->interval = 5;
		}
		else
			info->interval = 10;
	}
	else if (data >= info->cfg_items.terminal_vol + 50)
	{
		info->interval = 5;
	}
	else
	{
		info->interval = 2;
	}

	GAUGE_DBG("[%s] bat_vol :%d, interval = %d\n", __func__, data, info->interval);
	
	return info->interval;
}

static void start_anew(struct atc260x_gauge_info *info)
{
	kfifo_reset(&info->fifo);
	ch_resistor_calced = 0;
	first_calc_resistor = -1;
	taper_interval = 0;

	kfifo_reset(&info->down_curve);
}
static void soc_grow_up(struct atc260x_gauge_info *info,int grow_step)
{
	if (info->soc_now == FULL_CHARGE_SOC)
		return ;
		
	if (info->soc_real > info->soc_now)
	{
		info->soc_now +=  grow_step;
		GAUGE_DBG("soc_now add up %d\n",grow_step);
	}
	if (info->soc_now > FULL_CHARGE_SOC)
		info->soc_now = FULL_CHARGE_SOC;
	
}

#define CLIMB_UP_TIME  30
static void soc_now_compensation(struct atc260x_gauge_info *info)
{
	int chg_current;
	int batv;
	int soc_up_step;
	int soc_compensate;
	int soc_to_show;
	
	int ratio = 1;
	chg_current = measure_current_avr();
	batv = measure_vbatt_average();
	/*interval * FULL_CHARGE_SOC / (info->cfg_items.capacity * 3600 / chg_current)*/
	soc_up_step = info->interval * FULL_CHARGE_SOC / 100 * chg_current / (info->cfg_items.capacity * 36);
	
	/*for test*/
	if (info->soc_real >= 90000 && info->soc_real <= 98000)
		ratio = 2;
	soc_up_step = soc_up_step / ratio;
	soc_to_show = info->soc_now + soc_up_step;
	GAUGE_DBG("soc_now = %d,soc_up_step = %d,soc_to_show=%d\n",info->soc_now,soc_up_step,soc_to_show);
	
	if ( info->soc_real < 90000)
	{
		if (info->soc_real >= soc_to_show + 2000)
		{
			soc_compensate = 800 % ((info->soc_real - soc_to_show)/(CLIMB_UP_TIME / info->interval));
			if (soc_up_step + soc_compensate > 1000)
				soc_compensate = 1000 - soc_up_step;
			GAUGE_DBG("soc_real >soc_to_show +2000:soc_compensate = %d\n",soc_compensate);
			soc_to_show += soc_compensate;
			GAUGE_DBG("finally:soc_now : %d\n",soc_to_show);		
		}
	}
	
	if (soc_to_show >= info->soc_real + 1000 )
	{
		soc_compensate = soc_up_step;
		GAUGE_DBG("soc_to_show > soc_real + 1000:soc_compensate = -%d\n",soc_compensate);	
		soc_to_show -= soc_compensate;
		GAUGE_DBG("finally:soc_now : %d\n",soc_to_show);
	}
	
	if (soc_to_show >= FULL_CHARGE_SOC - 1)
		info->soc_now = FULL_CHARGE_SOC - 1;
	else
		info->soc_now = soc_to_show;
}

static void full_power_schedule(struct atc260x_gauge_info *info)
{
	if(atc260x_get_charge_status())
	{
		atc260x_charger_turn_off_force();
		GAUGE_DBG("[%s]now turn off charger...\n",__func__);
		msleep(5000);
	}
	/* detecting battery voltage as ocv*/
	get_charge_status(&info->status);/* added by cxj @2015-01-28*/
	info->vbatt_avg = measure_vbatt_average();
	info->ocv = info->vbatt_avg;
	calc_soc(info, info->ocv, &info->soc_real);
	GAUGE_DBG("after 5 secs,bat_vol(ocv)= %d\n",info->vbatt_avg);	
	
	if(info->soc_real == FULL_CHARGE_SOC)
	{
		GAUGE_DBG("[%s]soc_real has been FULL_CHARGE_SOC!now minus 1000\n",__func__);
		info->soc_real = FULL_CHARGE_SOC - 1000;
	}
	
	if (info->vbatt_avg >= (info->cfg_items.taper_vol))
	{
		taper_interval += info->interval;
		GAUGE_DBG("[%s]taper_interval = %d\n",__func__,taper_interval);
		if (taper_interval >= 60)
		{
			GAUGE_DBG("Time's up,power is full!\n");
			info->soc_real = FULL_CHARGE_SOC;

			/* the right of finally turning off the charger is owned by charger driver
			 * now the soc_real is just up to FULL_CHARGE_SOC,so the it must be more than
			 * soc_now.meanwhile,calling the following function is needed,as the operation 
			 * to recover the flag of turn_off_force.
			 */
			GAUGE_DBG("turn on charger finally!\n");
			atc260x_charger_turn_on_force();
			
			full_power_dealing = false;
			taper_interval = 0;
		}	
	}
	else
	{
		cycle_count = taper_interval / info->interval + 1; 
		GAUGE_DBG("info->interval=%d,taper_interval=%d,cycle_count = %d\n",info->interval,taper_interval,cycle_count);
		atc260x_charger_turn_on_force();
	}
	return;
}
static bool emphasize_dealing(struct atc260x_gauge_info *info)
{
	GAUGE_DBG("next_do_count = %d,cycle_count = %d\n",next_do_count,cycle_count);
	if (cycle_count == 0)
		return true;
	else
	{
		if(next_do_count++ == (60 / info->interval - cycle_count))
		{
			next_do_count = 0;
			cycle_count = 0;
			return true;
		}
		else
			return false;
	}
}
static bool pre_full_power_schedule(struct atc260x_gauge_info *info)
{
	int bat_curr;
	int bat_vol;
	int chg_current;
	int current_set_now;
	bool full_power_test = false;
	
	bat_curr = measure_current_avr();	
	bat_vol = measure_vbatt_average();
	GAUGE_DBG("bat_curr=%d,bat_vol=%d\n",bat_curr,bat_vol);
	
	current_set_now = get_chg_current_now();
	GAUGE_DBG("current_set_now = %d\n",current_set_now);
	
	if(info->cfg_items.min_over_chg_protect_voltage >= 4275)
	{
		if (bat_vol > 4200)
		{
			if (current_set_now > 400)
			{
				if (bat_curr < info->cfg_items.taper_current)
				{
					GAUGE_DBG("current_set_now > 300 && bat_cur < %d:enter full power dealing\n",
								info->cfg_items.taper_current);
					full_power_test = true;
					taper_interval = 0;
				}
			}
			else
			{
				if (info->ocv >= info->cfg_items.taper_vol)
				{
					GAUGE_DBG("current_set_now<300 && ocv >= %d:enter full power dealing\n",
								info->cfg_items.taper_vol);
					full_power_test = true;
					taper_interval = 0;
				}
			}
		}
	}
	else
	{
		if (bat_vol > 4200)
		{
			if (current_set_now > 400)
			{
				if (bat_curr < 700)
				{
					GAUGE_DBG("now set charge current to be 400mA");
					atc260x_set_charger_current(400, &chg_current);
				}
			}
			else if (current_set_now == 400)
			{
				if (bat_curr < info->cfg_items.taper_current)
				{
					GAUGE_DBG("current_set_now = 400 && bat_curr < %d:enter full power\n",
								info->cfg_items.taper_current);
					full_power_test = true;
					taper_interval = 0;
				}
			}
			else
			{
				if (info->ocv >= info->cfg_items.taper_vol)
				{
					GAUGE_DBG("current_set_now < 400 && ocv >= %d:enter full power\n",
								info->cfg_items.taper_vol);
					full_power_test = true;
					taper_interval = 0;
				}
			}
		}
			
	}
	
	/*for test*/
	if (info->ocv >= info->cfg_items.taper_vol && info->soc_now == (FULL_CHARGE_SOC-1))
	{
		full_power_test = true;
		taper_interval = 0;
	}
	
	GAUGE_DBG("[%s]full_power_test =%d\n",__func__,full_power_test);
	full_power_dealing = full_power_test;
	return full_power_test;
}

static int  charge_process(struct atc260x_gauge_info *info)
{
	int ret;
	int current_set_now;
	int chg_current;
	
	if (info->soc_real == FULL_CHARGE_SOC)
	{
		if (info->cfg_items.log_switch)
		{
			gather_battery_info(info);
			calc_ocv(info);//calc batv,bati,ocv for log
		}
		current_set_now = get_chg_current_now();
		if (current_set_now != 100)
			atc260x_set_charger_current(100,&chg_current);
		soc_grow_up(info,500);
		return 0;
	}	
	/* added by cxj @2014-12-31
	 * NEW FEATURE£ºfull power dealing
	 */
	if(pre_full_power_schedule(info))
	{	
		info-> interval = 5;
		if (emphasize_dealing(info))
			full_power_schedule(info);
		return 0;
	}

	/*calc ch_resistor*/
	if ((!(info->soc_show % 5) && !ch_resistor_calced) || first_calc_resistor == -1)
	{
		ret = calc_charge_resistor(info);
		if (ret)
			GAUGE_ERR("[%s] calc charge resistor err\n", __func__);
		if (first_calc_resistor == -1)
		{
			GAUGE_DBG("calc resistor over!\n");
			first_calc_resistor = 0;
		}
		ch_resistor_calced = 1;
	}

	/*calc ocv*/
	gather_battery_info(info);
	ret = calc_ocv(info);
	if (ret)
	{
		GAUGE_ERR("[%s]calc ocv fail!\n",__func__);
		return ret;
	}
	
	/*calc soc*/
	calc_soc(info, info->ocv, &info->soc_real);
	
	soc_now_compensation(info);
	
	/*if the calc_soc has been FULL_CHARGE_SOC, we do not make it as it should be
	 * because the full power schedule will do it
	*/
	if(info->soc_real == FULL_CHARGE_SOC)
	{
		GAUGE_DBG("[%s]soc_real has been FULL_CHARGE_SOC!now minus 1000\n",__func__);
		info->soc_real = FULL_CHARGE_SOC - 1000;
	}
	
	return 0;
}


static int  discharge_process(struct atc260x_gauge_info *info)
{
	int ret;
	int soc_real_test;
	static int first_flag = 1;
	gather_battery_info(info);

	ret = calc_ocv(info);
	if (ret)
	{
		GAUGE_DBG("[%s]calc_ocv failed!\n",__func__);
		return ret;
	}
	
	if (first_flag)
	{
		calc_soc(info, info->ocv, &info->soc_real);
		soc_real_test = info->soc_real;
		first_flag = 0;
	}
	else
		calc_soc(info, info->ocv, &soc_real_test);/*not allow soc_real go up*/
	
	if (soc_real_test < info->soc_real)
	{
		info->soc_real = soc_real_test;
		
	}
	else
	{
		if (info->dich_r_change)
		{
			info->soc_real = soc_real_test;
			info->dich_r_change = false;
		}
	}
	GAUGE_DBG("soc_real:%d\n",info->soc_real);

	down_curve_smooth(info, &info->soc_now);

	return 0;
}

/*added by cxj @20141117*/
static int notcharging_process(struct atc260x_gauge_info *info)
{
	/* when step into full_power_schedule(this time,bat_vol>=4180mV),charger may be turned off,and
	 * status of charge will turn to be NOTDISCHARGING,and cannot dealing
	 * full power status in charge_process
	 * soc_now >0:avoid poor healthy status of battery,such as no battery ,short circuit.
	 * soc_now <FULL_CHARGE_SOC:avoid still stepping into full power dealing when capacity is 100%,
	*/
	if(info->soc_now == FULL_CHARGE_SOC || info->soc_now <= 0)
	{
		if (info->cfg_items.log_switch)
		{
			gather_battery_info(info);
			calc_ocv(info);//calc batv,bati,ocv for log
		}
		return 0;
	}
	
	gather_battery_info(info);
	calc_ocv(info);
	calc_soc(info, info->ocv, &info->soc_real);  
	/* 1.when the load capacity of adapter cannot support charging with more than 60mA,
	 * 2.when full_power_dealing finished,and then turn on charger ,but also,the load capacity is not enough,
	 * 3.when battery is being full,but did not ever step into full_power_dealing,and charging current<60mA .
	 * all above,contribute to going into this function,meanwhile charger is on.
	 */
	if (atc260x_get_charge_status())
	{
		GAUGE_DBG("charger is on,but current<60mA\n");
		if (info->ocv >= (info->cfg_items.taper_vol))
		{
			info->soc_now += 500;/* for 2&3*/
		}
		else
		{
			if (info->soc_real > info->soc_now + 500)
			{
				if (info->soc_now + 500 < FULL_CHARGE_SOC)  /*for 1*/
					info->soc_now += 500;
				
			}
			else if (info->soc_real < info->soc_now - 500)
			{
				if (info->soc_now - 500 >= 1000)
					info->soc_now -= 500;
			}
		}
		return 0;
	}
	
	if(info->soc_real == FULL_CHARGE_SOC)
	{
		GAUGE_DBG("[%s]soc_real has been FULL_CHARGE_SOC!now minus 1000\n",__func__);
		info->soc_real = FULL_CHARGE_SOC - 1000;
	}
	/* here,if charger is not on,it should be:
	 * 1.being in full_power_dealing
	 * 2.just plug in adapter a moment
	*/
	if(info->ocv >= (info->cfg_items.taper_vol))
	{
		soc_grow_up(info,500);
		GAUGE_DBG("[%s]interval=%d\n",__func__,info->interval);
		full_power_schedule(info);
	}
	else
	{
		if(!atc260x_get_charge_status())
		{
			GAUGE_DBG("[%s]now turn on charger...\n",__func__);
			atc260x_charger_turn_on_force();
		}
		cycle_count = taper_interval / info->interval + 1;
		GAUGE_DBG("[%s]taper_interval=%d,cycle_count=%d",__func__,taper_interval,cycle_count);
	}
	return 0;
}

/*detect if board has preserved cm layout*/
static void board_cm_pre_detect(struct atc260x_gauge_info *info)
{
	int bat_curr_icm;
	int bat_curr_normal;
	int bat_vol;
	int icm_ok;
	int ret;
	
	if (!info->cfg_items.icm_available)
		goto out;
	ret = atc260x_pstore_get(info->atc260x,ATC260X_PSTORE_TAG_GAUGE_ICM_EXIST,&icm_ok);
	GAUGE_DBG("[%s]icm_ok = %d\n",__func__,icm_ok);
	if (ret)
	{
		GAUGE_ERR("get icm_ok error!\n");
		goto out;
	}
	if (icm_ok == 1)
	{
		GAUGE_INFO("icm does not work normally!\n");
		goto out;
	}
	
	bat_vol = measure_vbatt();
	bat_curr_normal = measure_atc2603a_current(info);
	
	if (bat_curr_normal == 0)	
	{
		icm_detect_loop = true;
		return;	
	}
	bat_curr_icm = measure_atc2603c_current(info);	
	if(bat_curr_icm == 0)
	{
		board_has_cm = false;
		GAUGE_INFO("[%s]ICM do not exist or work normally\n",__func__);
	}
	else
	{	
		board_has_cm = true;
		GAUGE_DBG("[%s]ICM exist and work!\n",__func__);
	}	
	GAUGE_DBG("enter loop\n");
	icm_detect_loop = true;
	return;

out:
	GAUGE_DBG("never enter loop\n");
	icm_detect_loop = false;
	return;
}
static void board_cm_detect(struct atc260x_gauge_info *info)
{
	int normal_cur;
	int bat_vol;
	int icm_cur;
	int ret;

	normal_cur = measure_atc2603a_current(info);
	bat_vol = measure_vbatt();
	/* when it's in NOTDISCHARGING status,it's unnecessary to detect ICM */
	if(normal_cur == 0)
	{
		icm_detect_loop = true;
		return;
	}

	/*when not full ,we begin to detect icm
	 * the current detected was not real,just for tester to use,
	 * when test_icm_enable is true
	 */
	
	if (!test_icm_enable)
		icm_cur = measure_atc2603c_current(info);
	else
		icm_cur = 0;
	GAUGE_DBG("normal_cur =%d,icm_cur = %d\n",normal_cur,icm_cur);
	
	if(icm_cur == 0)
	{		
		if(board_has_cm)
		{	
			board_has_cm = false;/*change to ICH&IBAT*/
			icm_detect_cycle = 0;/*if not seriously zero,cancel accumulation*/
		}
		if(++icm_detect_cycle == 3)
		{
			GAUGE_DBG("icm_detect_cycle=%d\n",icm_detect_cycle);
			ret = atc260x_pstore_set(info->atc260x,ATC260X_PSTORE_TAG_GAUGE_ICM_EXIST,1);
			if(ret)
				GAUGE_ERR("Cann't write to the ICM_EXIST bit\n");

			icm_detect_loop = false;
		}
	}
	else
	{
		board_has_cm = true;
		icm_detect_loop = true;
	}
}

/* uA */
/*record the time stamp of suspend*/
static struct timespec suspend_ts; 
static void soc_update_after_resume(struct atc260x_gauge_info *info)
{
	struct timespec resume_ts;
	struct timespec sleeping_ts;

	unsigned int soc_consume;
	unsigned int soc_to_show;
	
	getnstimeofday(&resume_ts);
	sleeping_ts = timespec_sub(resume_ts, suspend_ts);
	
	soc_consume = (unsigned int)sleeping_ts.tv_sec * info->cfg_items.suspend_current /(info->cfg_items.capacity * 36);
	GAUGE_DBG("sleeping_ts.tv_sec(%ld)*FULL_CHARGE_SOC(%d)*SUSPEND_CURRENT(%d)/1000/(capacity(%d)*3600)=%u\n",
				sleeping_ts.tv_sec,FULL_CHARGE_SOC,info->cfg_items.suspend_current,info->cfg_items.capacity,soc_consume);

	if((soc_consume < 1000 && info->soc_now == FULL_CHARGE_SOC) || pre_charge_status == POWER_SUPPLY_STATUS_NOT_CHARGING)
		soc_consume = 0;
	
	soc_to_show = info->soc_now - soc_consume;
	if (soc_to_show > 0)
		info->soc_now = soc_to_show;
	else
		info->soc_now = 0;
	GAUGE_DBG("*******soc_now = %d\n",info->soc_now);
}
#define SHUTDOWN_TIME_THRESHOLD (30 * 24 * 3600)
static int soc_update_after_reboot(struct atc260x_gauge_info *info)
{
	unsigned long time_stored;
	unsigned long time_now;
	unsigned long time_space;

	unsigned int soc_consume;
	unsigned int soc_stored;
	unsigned int soc_to_show;
	int ret;
	int batv_avr;
	
	time_now = get_time_hour(info);
	time_stored = get_stored_time(info);
	time_space = (time_now - time_stored) * 3600;
	GAUGE_DBG("the time of shutdown is %lus\n",time_space);
	soc_stored = get_stored_soc(info) * 1000;
	
	if (time_space < SHUTDOWN_TIME_THRESHOLD)
	{
		soc_consume = time_space * info->cfg_items.shutdown_current /(info->cfg_items.capacity * 36);
		GAUGE_DBG("[%s]soc_consume = %d\n",__func__,soc_consume);

		soc_to_show = soc_stored - soc_consume;
		if(soc_to_show > 0)
			info->soc_now = soc_to_show + 500; /*500:avoid soc to be show being cut 1% suddenly*/
		else
			info->soc_now = 0;
	}
	else
	{
		ret = calc_ocv(info);
		if (ret)
		{
			GAUGE_ERR("[%s]calculate ocv failed!!\n",__func__);
			return ret;
		}
		calc_soc(info, info->ocv, &info->soc_real);

		/* 
		 * if bat voltage is equal with or less than stop voltage by 50mv, 
		 * use real soc to prevent abnormal  shutdown.
		 */
		batv_avr = measure_vbatt_average();
		if ((batv_avr < info->cfg_items.terminal_vol + 50) &&
			info->soc_real <= info->soc_now)
		{
			info->soc_now = info->soc_real;
			return 0;
		}

		if(info->soc_real < 15000)
		{
			if(soc_stored < info->soc_real)
				info->soc_now = soc_stored;
			else
				info->soc_now =  info->soc_real;
		}
		else
		{
			if(abs(soc_stored-info->soc_real) > 10000)
			{
				if(soc_stored < info->soc_real)
					info->soc_now = soc_stored;
				else
					info->soc_now =  info->soc_real;				
			}
			else
				info->soc_now = soc_stored;
		}

	}
	return 0;
}
static int  init_capacity(struct atc260x_gauge_info *info)
{
	int ret;
	int wakeup_flag;
	int soc_stored;
	
	/*added by cxj @2014-12-04
	 *if resume,we let cost_time to be zero,as from suspend to resume,it always not costs more than 30sec
	*/
	cost_time = 0;

	gather_battery_info(info);
	get_charge_status(&info->status);
	wakeup_flag = owl_pm_wakeup_flag();
	
	/* chenbo@20150608, 
	 * recalclate soc when boot due to  upgrade or onoff 8s or reset 
	 */
	if (info->first_product || 
		wakeup_flag == OWL_PMIC_WAKEUP_SRC_RESET)
	{
		if (wakeup_flag == OWL_PMIC_WAKEUP_SRC_RESET)
			GAUGE_INFO("%s onoff 8s wakeup or reset\n", __func__);
		ret = calc_ocv(info);
		if (ret)
		{
			GAUGE_ERR("[%s]calculate ocv failed!!\n",__func__);
			return ret;
		}
		calc_soc(info, info->ocv, &info->soc_real);
		
		info->soc_now = info->soc_real;
		
		if (info->first_product)
			info->first_product = false;
		GAUGE_INFO("[%s] usable soc:%d, current soc:%d\n",
		__func__, info->soc_real, info->soc_now);	
		/* chenbo@20150608, 
	 	*  recalclate soc when boot due to onoff 8s or reset 
	 	*/
		if (wakeup_flag == OWL_PMIC_WAKEUP_SRC_RESET)
		{
			soc_stored = get_stored_soc(info) * 1000;
			if (soc_stored < 0)
				return 0;
				
			if(info->soc_real < 15000 || 
				abs(soc_stored - info->soc_real) > 10000)
				info->soc_now = min(soc_stored, info->soc_real);
			else
				info->soc_now = soc_stored;
			GAUGE_INFO("[%s] soc_stored:%d, soc_real:%d\n",
				__func__, soc_stored, info->soc_real);
				
		}
		
	}
	else
	{
		if (resume_flag)
		{
			soc_update_after_resume(info);
			return 0;
		}
		ret = soc_update_after_reboot(info);
		if (ret)
		{
			GAUGE_INFO("soc update after reboot failed!\n");
			return ret;
		}
	}
	return 0;
}


static void gauge_update(struct work_struct *work)
{
	int charge_status;
	int ret;
	struct atc260x_gauge_info *info;
	info = container_of(work, struct atc260x_gauge_info, work.work);
	charge_status = info->status;
	/* added by cxj @2014-12-10
	 * when resuming, init_capacity cost much time,take it here
	*/
	if(resume_flag)
	{
		board_cm_pre_detect(info);
		
		ret = init_capacity(info);
		if (ret)
		{
			GAUGE_ERR("[%s] init_gauge failed\n",__func__);
			return;
		}	
		resume_flag = false;
		GAUGE_INFO("[%s]resume,init_capacity done!",__func__);
	}
	
	if (icm_detect_loop)
		board_cm_detect(info);
	
	/* if the status has been changed,we discard the kfifo data*/
	get_charge_status(&info->status);
	if (charge_status ^ info->status)
	{
		GAUGE_INFO("charge status changed!\n");
		start_anew(info);  
		 if (info->status == POWER_SUPPLY_STATUS_DISCHARGING)
			 update_discharge_resistor(info,info->soc_now);
	}
	
	/*modified by cxj@20141117*/
	
	if (info->status == POWER_SUPPLY_STATUS_CHARGING)
	{
		charge_process(info);
	}
	else if (info->status == POWER_SUPPLY_STATUS_DISCHARGING)
	{
		discharge_process(info);
	}
	else    /* POWER_SUPPLY_STATUS_NOT_CHARGING */
	{
		notcharging_process(info);
	}
	
	soc_post_process(info);
	generate_poll_interval(info);
	queue_delayed_work(info->wq, &info->work, info->interval * HZ);
}

static void get_current_ratio(struct atc260x_gauge_info *info)
{
	if (ic_type == ATC260X_ICTYPE_2603A)
	{
		if (atc260x_get_version(info) == ATC260X_ICVER_D) 
		{
			info->current_ratio = PMU_VER_D_RATIO;
		} 
		else 
		{
			info->current_ratio = PMU_VER_ABC_RATIO;
		} 
	}
	else if (ic_type == ATC260X_ICTYPE_2603C)
	{
		info->current_ratio = PMU_VER_ABC_RATIO;
	}
}
static void resistor_init(struct atc260x_gauge_info *info)
{
	info->ch_resistor = 150;
	info->disch_resistor = 250;
	GAUGE_INFO("[%s] inited ch_resistor : %d, inited disch_resistor:%d\n",
		__func__, info->ch_resistor, info->disch_resistor); 
}
static int  init_gauge(struct atc260x_gauge_info *info)
{
	int ret;
	/*added by cxj @20141009*/
	ic_type = get_pmu_ic_type(info);
	/*get pmu current ratio according to pmu version*/
	get_current_ratio(info);

	GAUGE_INFO("[%s] PMU current ratio:%d\n", __func__, info->current_ratio);

	start_anew(info);
	info->interval = 5;
	
	info->first_product = first_product(info->atc260x);
	GAUGE_INFO("[%s] first_product:%d\n", __func__, info->first_product);

	resistor_init(info);
	info->filter_algorithm = filter_algorithm3;
	
	/* detect cm before init_capacity */
	if(ic_type == ATC260X_ICTYPE_2603C)
		board_cm_pre_detect(info);
	
	ret = init_capacity(info);
	if (ret)
	{
		GAUGE_ERR("[%s]init_capacity failed!\n",__func__);
		return ret;
	}
	
	return 0;
}

int get_cap_gauge(void)
{
	return global_gauge_info_ptr->soc_show;
}

static void set_callback(void)
{
	act260x_set_get_cap_point((void *)get_cap_gauge);
	act260x_set_get_volt_point((void *)measure_vbatt);
	act260x_set_get_cur_point((void *)measure_current);
}

void gauge_reset(struct atc260x_gauge_info *info)
{
	int resistor;
	int data;
	
	resistor = gauge_reg_read(info->atc260x, PMU_OV_INT_EN);
	resistor = resistor | (0x3fff << 2);
	gauge_reg_write(info->atc260x, PMU_OV_INT_EN, resistor);

	data = gauge_reg_read(info->atc260x, PMU_SYS_CTL9);
	data = data & ~(0xff << 8);
	gauge_reg_write(info->atc260x, PMU_SYS_CTL9, data);
}

static ssize_t show_reset(struct device *dev,struct device_attribute *attr, char *buf)
{
	return 0;
}
static ssize_t store_reset(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct platform_device *pdev = (struct platform_device *)container_of(dev,struct platform_device,dev);
	struct atc260x_gauge_info *info = (struct atc260x_gauge_info *)platform_get_drvdata(pdev);

	gauge_reset(info);
	get_stored_time(info);
	get_stored_soc(info);
	first_product(info->atc260x);
			
	return count;
}

static ssize_t show_dump(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = (struct platform_device *)container_of(dev,struct platform_device,dev);
	struct atc260x_gauge_info *info = (struct atc260x_gauge_info *)platform_get_drvdata(pdev);
	batt_info_dump(info);
	return 0;
}
static ssize_t store_dump(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return count;
}

static ssize_t show_test_kfifo(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = (struct platform_device *)container_of(dev,struct platform_device,dev);
	struct atc260x_gauge_info *info = (struct atc260x_gauge_info *)platform_get_drvdata(pdev);
	struct kfifo_data kfifo_data;
	struct kfifo_data test;
	int ret;
	
	kfifo_data.bat_curr = 1000;
	kfifo_data.bat_vol = 3800;
	kfifo_in(&info->fifo, &kfifo_data, sizeof(struct kfifo_data));
	kfifo_data.bat_curr = 1500;
	kfifo_data.bat_vol = 4000;
	kfifo_in(&info->fifo, &kfifo_data, sizeof(struct kfifo_data));
	ret = kfifo_out(&info->fifo, &test, sizeof(struct kfifo_data));
	printk("%s : test.current :%d\n", __func__, test.bat_curr);
	printk("%s : test.voltage :%d\n", __func__, test.bat_vol);
	ret = kfifo_out_peek(&info->fifo, &test, sizeof(struct kfifo_data));
	printk("%s : peer,test.current :%d\n", __func__, test.bat_curr);
	printk("%s : peer,test.voltage :%d\n", __func__, test.bat_vol);
	return 0;
}
static ssize_t store_test_kfifo(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return count;
}

static ssize_t show_test_filter(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;

	int test_buf[10] = {550,560,    600,610,619,605, 608,    700,710,720};
	ret = filter_algorithm1(test_buf, 10, CURRENT_TYPE);
	printk("ret = %d\n", ret);
	
	return 0;
}
static ssize_t store_test_filter(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return count;
}

static ssize_t show_test_icm(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret,icm_ok;
	ret = atc260x_pstore_get(global_gauge_info_ptr->atc260x,ATC260X_PSTORE_TAG_GAUGE_ICM_EXIST,&icm_ok);
	printk("\n*********************************************\n");
	printk("  test_icm_enable:%d [1:enable 0:disable]\n",test_icm_enable);
	printk("  icm_detect_loop:%d [1:in loop 0:not in loop]\n",icm_detect_loop);
	printk("  board_has_cm   :%d [1:icm works 0:icm fails]\n",board_has_cm);
	printk("   ICM_EXIST     :%d [1:reg = 1 0:reg = 0]\n",icm_ok);
	printk("\n**********************************************\n");
	return sprintf(buf,"%d\n",board_has_cm);
}
static ssize_t store_test_icm(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int icm_status_to_fail;
	int ret;
	sscanf(buf,"%d\n",&icm_status_to_fail);
	if(icm_status_to_fail == 1)
	{
		test_icm_enable = true;
		printk("********icm is set to fail :%d********\n",icm_status_to_fail);
	}
	else if(icm_status_to_fail == 0)
	{
		test_icm_enable = false;
		icm_detect_loop = true;
		board_has_cm = true;
		icm_detect_cycle = 0;
		ret = atc260x_pstore_set(global_gauge_info_ptr->atc260x,ATC260X_PSTORE_TAG_GAUGE_ICM_EXIST,0);
		if(ret)
		{
			printk("Cann't write to the ICM_EXIST bit\n");
		}
		else
		{
			printk("write 0 to ICM_EXIST successfully\n");
		}
		printk("recover to work normally:%d\n",icm_status_to_fail);
		
	}
	else
		printk("please enter 0 or 1:0-normal work 1-icm fail\n");
		
	return count;
		
}
static ssize_t store_log_switch(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int log_switch;

	sscanf(buf,"%d\n",&log_switch);
	if (log_switch == 1 || log_switch == 0)
	{
		global_gauge_info_ptr->cfg_items.log_switch = log_switch;
		printk("now log_switch=%d\n",log_switch);
	}
	else
		printk("wrong parameter!\n");
	return count;
}
static ssize_t show_log_switch(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%d\n",global_gauge_info_ptr->cfg_items.log_switch);
}

static ssize_t store_print_switch(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int print_switch;
	
	sscanf(buf,"%d\n",&print_switch);
	if (print_switch == 1 || print_switch == 0)
	{
		global_gauge_info_ptr->cfg_items.print_switch = print_switch;
		printk("now print_switch=%d\n",print_switch);
	}
	else
		printk("wrong parameter!\n");
	return count;
}
static ssize_t show_print_switch(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%d\n",global_gauge_info_ptr->cfg_items.print_switch);
}

static struct device_attribute gauge_attrs[] = 
{
	__ATTR(test_filter, S_IRUGO | S_IWUSR, show_test_filter, store_test_filter),
	__ATTR(test_kfifo, S_IRUGO | S_IWUSR, show_test_kfifo, store_test_kfifo),
	__ATTR(reset, S_IRUGO | S_IWUSR, show_reset, store_reset),
	__ATTR(dump, S_IRUGO | S_IWUSR, show_dump, store_dump),
	__ATTR(test_icm, S_IRUGO | S_IWUSR, show_test_icm, store_test_icm),
	__ATTR(log_switch, S_IRUGO | S_IWUSR,show_log_switch,store_log_switch),
	__ATTR(print_switch, S_IRUGO | S_IWUSR,show_print_switch,store_print_switch),

};

int gauge_create_sysfs(struct device *dev)
{
	int r, t;
	
	GAUGE_INFO("[%s] create sysfs for gauge\n", __func__);
	
	for (t = 0; t < ARRAY_SIZE(gauge_attrs); t++) 
	{
		r = device_create_file(dev, &gauge_attrs[t]);
	
		if (r)
		{
			dev_err(dev, "failed to create sysfs file\n");
			return r;
		}
	}
	
	return 0;
}

void gauge_remove_sysfs(struct device *dev)
{
	int  t;

	GAUGE_INFO("[%s]remove sysfs for gauge\n", __func__);
	for (t = 0; t < ARRAY_SIZE(gauge_attrs); t++) 
	{
		device_remove_file(dev, &gauge_attrs[t]);
	}
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void atc260x_gauge_early_suspend(struct early_suspend *handler)
{
    printk("[%s-%d] enter early_suspend\n", __FUNCTION__, __LINE__);
}

static void atc260x_gauge_later_resume(struct early_suspend *handler)
{
    printk("[%s-%d] enter later_resume\n", __FUNCTION__, __LINE__);

}
#endif  //CONFIG_HAS_EARLYSUSPEND

static int atc260x_change_log_switch_pre_suspend(struct notifier_block *nb, unsigned long event, void *dummy)
{
	switch (event) {
	case PM_SUSPEND_PREPARE:
		if(global_gauge_info_ptr->cfg_items.log_switch == 1)
		{
			global_gauge_info_ptr->store_gauge_info_switch_sav = global_gauge_info_ptr->cfg_items.log_switch;
			global_gauge_info_ptr->cfg_items.log_switch = 0;
		}
		GAUGE_INFO("log_switch = %d\n",global_gauge_info_ptr->cfg_items.log_switch);
		return NOTIFY_OK;
	default:
		return NOTIFY_DONE;
	}
}
static int atc260x_change_log_switch_pre_shutdown(struct notifier_block *nb, unsigned long event, void *dummy)
{
	cancel_delayed_work_sync(&global_gauge_info_ptr->work);
	switch (event) {
	case SYS_POWER_OFF:
		if(global_gauge_info_ptr->cfg_items.log_switch == 1)
		{
			global_gauge_info_ptr->store_gauge_info_switch_sav = global_gauge_info_ptr->cfg_items.log_switch;
			global_gauge_info_ptr->cfg_items.log_switch = 0;
		}
		GAUGE_INFO("log_switch = %d\n",global_gauge_info_ptr->cfg_items.log_switch);
		/*
		 * chenbo@20150514
		 * restore last soc during discharging,
		 * if soc jump from some soc(>1%) to zero.
		 */
		if (global_gauge_info_ptr->soc_show == 0 &&
			global_gauge_info_ptr->soc_last > 0) {
			global_gauge_info_ptr->soc_show = global_gauge_info_ptr->soc_last;
			store_soc(global_gauge_info_ptr);
		}
		return NOTIFY_OK;
	default:
		return NOTIFY_DONE;
	}
}
static struct notifier_block atc260x_suspend_notify = {
	.notifier_call = atc260x_change_log_switch_pre_suspend,
};
static struct notifier_block atc260x_shutdown_notify = {
	.notifier_call = atc260x_change_log_switch_pre_shutdown,
};

static  int atc260x_gauge_probe(struct platform_device *pdev)
{

	struct atc260x_dev *atc260x = dev_get_drvdata(pdev->dev.parent);
	struct atc260x_gauge_info *info;
	int ret;
	
	info = kzalloc(sizeof(struct atc260x_gauge_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;	

	ret =  kfifo_alloc(&info->fifo, 2 * sizeof(struct kfifo_data), GFP_KERNEL);
	if (ret)
	{
		GAUGE_ERR("[%s] error kfifo_alloc\n", __func__);
		goto free;
	}
	
	info->atc260x = atc260x;
	info->node = pdev->dev.of_node;
	global_gauge_info_ptr = info;
	first_store_gauge_info = 1;

	mutex_init(&info->lock);
	
	platform_set_drvdata(pdev, info);
	
	ret = gauge_create_sysfs(&pdev->dev);
	if (ret)
	{
		GAUGE_ERR("gauge_create_sysfs failed!\n");
		goto free_fifo; 
	}

	if (get_cfg_items(info))
	{
		GAUGE_ERR("get_cfg_items failed!\n");
		goto remove_sysfs;
	}
	/*modified @2014-12-20*/
	ocv_soc_table_init(info);
	
   	if (init_gauge(info))
	{
		GAUGE_ERR("init_gauge failed!\n");
	 	goto free_fifo;
	}

	set_callback();

	register_pm_notifier(&atc260x_suspend_notify);
	register_reboot_notifier(&atc260x_shutdown_notify);
#ifdef CONFIG_HAS_EARLYSUSPEND
	info->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	info->early_suspend.suspend = atc260x_gauge_early_suspend;
	info->early_suspend.resume = atc260x_gauge_later_resume;
	register_early_suspend(&info->early_suspend);
#endif

	/*Create a work queue for fuel gauge*/
	info->wq =
		create_singlethread_workqueue("atc260x_gauge_wq");
	if (info->wq == NULL) 
	{
		GAUGE_ERR("[%s] failed to create work queue\n", __func__);
		goto remove_sysfs;
	}
	
	INIT_DELAYED_WORK(&info->work, gauge_update);
	queue_delayed_work(info->wq, &info->work, 0 * HZ);

	return 0;
	
remove_sysfs:
	gauge_remove_sysfs(&pdev->dev);
free_fifo:
	kfifo_free(&info->fifo);
free:
	kfree(info);

	 return ret;
}

static  int atc260x_gauge_remove(struct platform_device *pdev)
{

	struct atc260x_gauge_info *info = platform_get_drvdata(pdev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&info->early_suspend);
#endif
	kfifo_free(&info->fifo);
	kfree(info);

	return 0;
}


static int atc260x_gauge_suspend(struct platform_device *pdev, pm_message_t m)
{
	struct atc260x_gauge_info *info = platform_get_drvdata(pdev);
	get_charge_status(&pre_charge_status);
	GAUGE_DBG("pre_charge_status:%d[0:NOT_CHARGE,else:DISCHARGE]",pre_charge_status^POWER_SUPPLY_STATUS_NOT_CHARGING);
	cancel_delayed_work_sync(&info->work); 
	start_anew(info);
	getnstimeofday(&suspend_ts);
	return 0;
}

static int atc260x_gauge_resume(struct platform_device *pdev)
{
	struct atc260x_gauge_info *info = platform_get_drvdata(pdev);

	resume_flag = true;

	info->cfg_items.log_switch = info->store_gauge_info_switch_sav;
	GAUGE_DBG("=====resume:switch= %d\n",info->cfg_items.log_switch);
	
 	queue_delayed_work(info->wq, &info->work, 0 * HZ);
	
 	return 0;
}
static void atc260x_gauge_shutdown(struct platform_device *pdev)
{
	struct atc260x_gauge_info *info = platform_get_drvdata(pdev);
	store_time_pre_shutdown(info);
}
/* added by cxj @20141009 */
static const struct of_device_id atc260x_cap_gauge_match[] = {
	{ .compatible = "actions,atc2603a-cap-gauge", },
	{ .compatible = "actions,atc2603c-cap-gauge", },
	{ .compatible = "actions,atc2609a-cap-gauge", },
	{},
};
MODULE_DEVICE_TABLE(of, atc260x_cap_gauge_match);
static struct platform_driver atc260x_gauge_driver = {
	.probe      = atc260x_gauge_probe,
	.remove     = atc260x_gauge_remove,
	.driver     = 
	{
		.name = "atc260x-cap-gauge",
		.of_match_table = of_match_ptr(atc260x_cap_gauge_match), /* by cxj */
	},
	.suspend    = atc260x_gauge_suspend,
	.resume     = atc260x_gauge_resume,
	.shutdown   = atc260x_gauge_shutdown,
};

static int __init atc260x_gauge_init(void)
{
	GAUGE_INFO("========This is a new reconstructed gauge by actions==========\n");
	GAUGE_INFO("[%s] drv name:%s, drv version:%s\n", __func__, THIS_MODULE->name, THIS_MODULE->version);
	return platform_driver_register(&atc260x_gauge_driver);
}
module_init(atc260x_gauge_init);

static void __exit atc260x_gauge_exit(void)
{
	platform_driver_unregister(&atc260x_gauge_driver);    
}
module_exit(atc260x_gauge_exit);

/* Module information */
MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("ATC260X gauge drv");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc260x-cap-gauge");
MODULE_VERSION("Actions-v1-20150720151405");
