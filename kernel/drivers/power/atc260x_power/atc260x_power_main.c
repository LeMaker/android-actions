/*
 * atc260x_power_main.c  --  Power driver for ATC260X
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
#include <linux/of_gpio.h>
#include <mach/gpio.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <linux/pwm.h>
#include <linux/earlysuspend.h>
#include <linux/fb.h>
#include <linux/earlysuspend.h>
#include <linux/fb.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/mfd/atc260x/atc260x.h>
#include <mach/bootdev.h>
#include "atc260x_power.h"

#define GPIO_NAME_LED_INNER_CHARGER "inner_charger_led_ctl"
#define GPIO_NAME_EXT_CTRL_CHARGER "ext_charger_ctl"

#define PMU_CHARGER_PHASE_PRECHARGE	(0)
#define PMU_CHARGER_PHASE_CONSTANT_CURRENT	(1)
#define PMU_CHARGER_PHASE_CONSTANT_VOLTAGE	(2)     

#define     ATC260X_SUPPLY_WALL          (0)
#define     ATC260X_SUPPLY_VBUS          (1)
#define     ATC260X_SUPPLY_BAT             (2)

/*pmu version, including a,b,c and d*/
#define	PMU_VERSION_A	0
#define	PMU_VERSION_B	1
#define	PMU_VERSION_C	2
#define	PMU_VERSION_D	3
/*different pmu version, different ratio */
#define 	PMU_VERSION_ABC_CC_RATIO	4
#define 	PMU_VERSION_D_CC_RATIO	3

/* update real battery voltage per 1min */
#define     ATC260X_INFO_UPDATE_INTERVAL         (1 * 60 * HZ)             			
#define     PWM_MAX_LEVEL                               63
#define     BL_POWER_PATH                             "/sys/class/backlight/backlight.3/bl_power"
/* when battery is full, need charge more than EXTRA_CHARGE_TIME*2 seconds.*/
#define 	EXTRA_CHARGE_TIME 900	

/*extern buck adjust coefficient*/ 
#define CONFIG_CURVE_ARG0	416000
#define CONFIG_CURVE_ARG1	3727
#define NO_CHARGER_PWM_LEVEL	32

#define PSY_NAME_WALL  "atc260x-wall"
#define PSY_NAME_USB   "atc260x-usb"

enum BACKLIGHT_CHANGE
{
	PRE_BL_ON,
	POST_BL_ON,
	ABORT_BL_ON
};

static enum power_supply_property atc260x_wall_props[] = 
{
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static enum power_supply_property atc260x_usb_props[] = 
{
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static struct atc260x_power *global_power_dev = NULL;

static  struct atc260x_charger * get_atc260x_charger(void)
{
    return global_power_dev ?  &global_power_dev->charger : NULL;
}
static struct atc260x_dev *get_atc260x_dev(void)
{
    return global_power_dev ? global_power_dev->atc260x: NULL;
}

int mini_chg = 0;
module_param(mini_chg, int, S_IRUSR);

static bool first_power_on = false;
static int count_completion = 0;
/*bat arv voltage before check bat online*/
int batv_before_check;
/*bat arv voltage after check bat online*/
int batv_after_check;

/*How long befor enable charger, because gas gauge need  0A correction*/
static  int charger_delay_counter = -1;
static int charger_once_on = false;
atomic_t adapter_type = ATOMIC_INIT(0);
/*record later resume state, if enter later resume cancel update work, otherwise schedule the work*/
static bool enter_later_resume = false;

static int atc260x_bat_check_status(struct atc260x_charger *charger, int *status);
static int atc260x_charger_check_online(struct atc260x_charger *charger);
extern int get_config(const char *key, char *buff, int len);
extern int set_judge_adapter_type_handle(void* handle);
static int get_pmu_ic_type(struct atc260x_charger *charger);
extern int owl_backlight_is_on(void);


int distinguish_adapter_type(void) 
{
	return atomic_read(&adapter_type);
}
/* modified by cxj @20141009 */
static int get_pmu_ic_type(struct atc260x_charger *charger)
{
	int pmu_type;
	pmu_type = atc260x_get_ic_type(charger->atc260x);
	power_dbg("[%s]pmu type is %d\n", __func__,pmu_type);
	
	return pmu_type;
}
/* added by cxj @20141009 */
static int atc260x_get_version(struct atc260x_charger *charger)
{
	int pmu_version;
	pmu_version = atc260x_get_ic_ver(charger->atc260x);
	power_dbg("[%s]pmu version is %d\n",__func__,pmu_version);
	return pmu_version;
} 
/* added functions:*_get_cap,*_get_vol,*_get_cur,*_get_temp
 * by tuhaoming @20150305 
 * support hard gauge as well as the soft one
*/
static int atc260x_power_get_cap(struct atc260x_charger *charger , int *data)
{
	int ret = -1;
	
   	if(charger->get_hw_gauge_cap)
    {
		*data = charger->get_hw_gauge_cap();
		ret = 0;
	}
	else if(charger->get_gauge_cap)
	{
		*data = charger->get_gauge_cap();
		ret = 1;
	}
	else
	{
		ret = -EFAULT;
	}

	return ret;
}

static int atc260x_power_get_vol(struct atc260x_charger *charger , int *data)
{
	int ret = -1;

	if (charger->get_hw_gauge_volt)
	{
		*data = charger->get_hw_gauge_volt();
		ret = 0;
	}
	else if (charger->get_gauge_volt)
	{
		*data = charger->get_gauge_volt();
		ret = 1;
	}
	else
	{
		ret = -EFAULT;
	}

	return ret;
}

static int atc260x_power_get_cur(struct atc260x_charger *charger , int *data)
{
	int ret = 0;
	
	if (charger->get_hw_gauge_cur)
	{
		*data = charger->get_hw_gauge_cur();
		ret = 0;

	}
	else if (charger->get_gauge_cur) 
	{
		*data = charger->get_gauge_cur();
		ret = 1;
	}
	else
	{
		ret = -EFAULT;
	}

	return ret;	
}

static int atc260x_power_get_temp(struct atc260x_charger *charger , int *data){
	int ret = 0;
	if (charger->get_hw_gauge_temp)
	{
		*data = charger->get_hw_gauge_temp();
		ret = 0;
	}
	else if (charger->get_gauge_temp) 
	{
		*data = charger->get_gauge_temp();
		ret = 1;
	}
	else
	{
		ret = -EFAULT;
	}
	
	return ret;
}
/**
 * get_pwm_level - calculate the pwm level according to the bat voltage.
 * @curve_arg0:	refer to the following fomula.
 * @curve_arg1: refer to the following fomula.
 * @vbat:	battery voltage
 * @bat_charger_diff:	the vol diff between bat and charger.
 * @return:	the pwm level.
 *
 * pwm_level = curve_arg0 * (batv + bat_charger_diff - curve_arg1) / 1000
 */
static unsigned int get_pwm_level(unsigned int curve_arg0, unsigned int curve_arg1, unsigned int vbat, unsigned int bat_charger_diff)
{
	unsigned int level = 0;

	if (vbat <= (curve_arg1 - bat_charger_diff)) 
	{
		return 0;
	}

	level = (curve_arg0 * (vbat + bat_charger_diff -curve_arg1)) / (1000 * 10000);
	
	if (level  > PWM_MAX_LEVEL) 
	{
		level = PWM_MAX_LEVEL;
	}
	
	return level;
}   

#ifdef CONFIG_DEBUG_FS
static int bat_debug_show(struct seq_file *s, void *data)
{
	struct atc260x_charger *charger = s->private;

	seq_printf(s, "charger is %s\n", charger->charge_on ? "on" : "off");
	if (charger->extern_power_online) 
	{
		seq_printf(s, "charge current = %dmA\n",
	   		charger->chg_ma);
	}

	seq_printf(s, "wall voltage = %d (mV)\n",
		charger->wall_mv);
	seq_printf(s, "vbus voltage = %d (mV)\n",
		charger->vbus_mv);
	seq_printf(s, "bat voltage = %d (mV)\n",
		charger->bat_mv);
	seq_printf(s, "bat current = %d (mA)\n",
		charger->bat_ma);

	return 0;
}

static int debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, bat_debug_show, inode->i_private);
}

static const struct file_operations bat_debug_fops = {
	.open       = debug_open,
	.read       = seq_read,
	.llseek     = seq_lseek,
	.release    = single_release,
};

static struct dentry *atc260x_bat_create_debugfs(struct atc260x_charger *charger)
{
	charger->debug_file = debugfs_create_file("charger", 0660, 0, charger,
	                     &bat_debug_fops);
	return charger->debug_file;
}

static void atc260x_bat_remove_debugfs(struct atc260x_charger *charger)
{
	debugfs_remove(charger->debug_file);
}
#else
static inline struct dentry *atc260x_bat_create_debugfs(struct atc260x_charger *charger)
{
    return NULL;
}
static inline void atc260x_bat_remove_debugfs(struct atc260x_charger *charger)
{
}
#endif

static int atc260x_power_check_online(struct atc260x_charger *charger, int supply,
                     union power_supply_propval *val)
{
	int ret = 0;

	switch (supply) {
	case ATC260X_SUPPLY_WALL:
	if (charger->cfg_items.support_adaptor_type == SUPPORT_USB)
	{
		val->intval = 0;
	}
	else
	{
		ret = charger->check_wall_online(charger, val);
	}
	
	if (ret < 0)
		return ret;
	break;

	case ATC260X_SUPPLY_VBUS:
	if (charger->cfg_items.support_adaptor_type == SUPPORT_DCIN)
	{
		val->intval = 0;
		break;	
	}
	ret = charger->check_vbus_online(charger, val);
	if (ret < 0)
		return ret;
	break;  
	
	case ATC260X_SUPPLY_BAT:
	val->intval = charger->check_bat_online(charger->atc260x);
	break;              

	default:
	break;

}

return 0;
}

static int atc260x_wall_get_prop(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
	struct atc260x_power *atc260x_power = dev_get_drvdata(psy->dev->parent);
	struct atc260x_charger *charger = &atc260x_power->charger;
	
	int ret = 0;

	if (first_power_on)
	{	
		wait_for_completion(&charger->check_complete);
		mutex_lock(&charger->lock);
		if(++count_completion == 3)
		{
			first_power_on = false;
			pr_info("[%s] first_power_on = false!\n",__func__);
		}	
		mutex_unlock(&charger->lock);		
	}

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (atc260x_power->charger.charger_cur_status & WALL_PLUGED)
			val->intval = 1;
		else if ((atomic_read(&adapter_type) == ADAPTER_TYPE_USB_ADAPTER_PLUGIN) &&
					charger->cfg_items.usb_adapter_as_ac)
			val->intval = 1;
		else
			val->intval = 0;
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = atc260x_power->charger.wall_mv;
		break;
	
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}



/**
 * atc260x_disable_vbus_id_if_needed - disable vbus path if needed
 *
 * disable vbus path in order to prevent rob large current from vbus, 
 * when dc5v and usb plugged in.
 */
static void atc260x_disable_vbus_id_if_needed(struct atc260x_charger *charger)
{
	/*if vbus is already cut(return 0) ,we do not need to set again*/
	if(!charger->get_vbus_path)
		return;
	/* scene 1 single charge mode*/
	if (charger->cfg_items.support_adaptor_type == SUPPORT_DCIN ||
		charger->cfg_items.support_adaptor_type == SUPPORT_USB)
	{
		charger->set_apds_vbus_pd(charger,true);
		goto disable_path;
	}
	/* scene 2 otg */
	if (charger->g_vbus_is_otg) {
		charger->set_apds_vbus_pd(charger, false);
		goto disable_path;
	}
	/* scene 3 USB+DCIN */
	if ((charger->charger_cur_status & WALL_PLUGED) && 
			(charger->charger_cur_status & USB_PLUGED)) {
		charger->set_apds_vbus_pd(charger, true);
		
		/*added by cxj @2014-12-27*/
		if (charger->cfg_items.ext_dcdc_exist)
			goto disable_path;
		else
		{
			if (charger->bat_is_exist)
			{
				if(charger->bat_mv > 3500 && charger->cur_bat_cap > 7)
					goto disable_path;
			}
		}
	}
	/* if all above is false,we make path connected default */
	charger->set_vbus_path(charger, true);
	charger->set_apds_vbus_pd(charger, true);
	return;
	
disable_path:
	charger->set_vbus_path(charger, false);
	return;
}

/**
 * atc260x_enable_vbusotg - enable vbus otg function or not.
 * @on : if true, disable the path between vbus and syspwr, otherwise enable the path. 
 *  
 * note: when enable the otg function,must shutdown the diode between vbusotg and syspwr, to avoiding loop circuit.
 */
int atc260x_enable_vbusotg(int on)
{
	struct atc260x_charger *charger = get_atc260x_charger();

	WARN_ON(charger->atc260x == NULL ||charger== NULL);
	
	if (charger == NULL)
		return -ENODEV;  	
	charger->g_vbus_is_otg = on;
	/* added by cxj @2015-01-04
	 * when usb disk plugging,we must set vbus off immediately
	 * or the current circuit may cause damage 
	 */
	atc260x_disable_vbus_id_if_needed(charger);
	power_dbg("\n[power] %s, %d, on: %d, g_vbus_is_otg: %d", __FUNCTION__, __LINE__, on, charger->g_vbus_is_otg);

	return 0;
}
EXPORT_SYMBOL_GPL(atc260x_enable_vbusotg);
static int atc260x_usb_get_prop(struct power_supply *psy,
                   enum power_supply_property psp,
                   union power_supply_propval *val)
{
	struct atc260x_power *atc260x_power = dev_get_drvdata(psy->dev->parent);
	struct atc260x_charger * charger = &atc260x_power->charger;
	int ret = 0;

	if (first_power_on)
	{	

		wait_for_completion(&charger->check_complete);
		mutex_lock(&charger->lock);
		if(++count_completion == 3)
		{
			first_power_on = false;
			pr_info("[%s] first_power_on = false!\n",__func__);
		}	
		mutex_unlock(&charger->lock);		
	}
	
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (atc260x_power->charger.charger_cur_status & USB_PLUGED) 
		{
			if (atomic_read(&adapter_type) == ADAPTER_TYPE_USB_ADAPTER_PLUGIN && 
				charger->cfg_items.usb_adapter_as_ac)
				val->intval = 0;
			else
				val->intval = 1;
		}
		else 
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		if (atc260x_power->charger.g_vbus_is_otg)
			val->intval = 0;
		else
			val->intval = atc260x_power->charger.vbus_mv;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}


static void system_stayawake(struct atc260x_charger *charger)
{

	if(!charger->wakelock_acquired)
	{
		wake_lock(&charger->charger_wake_lock);
		charger->wakelock_acquired=1;
		log_event_none(LOG_HEADER_LOCK);
	}
}
static void system_maysleep(struct atc260x_charger *charger)
{

	if(charger->wakelock_acquired)
	{
		wake_unlock(&charger->charger_wake_lock);
		charger->wakelock_acquired=0;
		log_event_none(LOG_HEADER_UNLOCK);
	}
}

/**
 * request_system_state - request to run continuously or sleep
 *
 * stay awake if bat is not full and adapter is online, also bat is online;
 * may sleep including: 1.bat is full although adapter is online; 
 *					  2.adapter is offline;
 *					  3.battery is offline.
 */
static int request_system_state(struct atc260x_charger *charger)
{

	int battery_full = 0;

	int charger_status_changed = charger->charger_cur_status!=charger->charger_pre_status;
	if ((charger->cur_bat_cap == 100))
	{
		battery_full=1;
	}

	/*awake 5s when adapter plug in/out in order to light on screen*/
	if (charger_status_changed)
	{
		log_event_none(LOG_HEADER_AWAKE);
		wake_lock_timeout(&charger->delay_lock, 5 * HZ);
	}
	
	/*no battery , we may wake unlock to sleep for CE certification*/
	if (!charger->bat_is_exist)
	{
		system_maysleep(charger);
		return 0;
	}
	
	/*not any adapter is connected, we should wake unlock to sleep*/
	if(!charger->extern_power_online)
	{
		system_maysleep(charger);
		return 0;
	}

	/*battery is full, we should wake unlock to sleep for CE certification*/
	if(battery_full)
	{
		system_maysleep(charger);
	}
	else
	{
		system_stayawake(charger);
	}  
	
	return 0;
}


void atc260x_charger_turn_on(void)
{
	struct atc260x_charger *charger = get_atc260x_charger();
	int wall_connected=charger->charger_cur_status &WALL_PLUGED;
	
	/*added by cxj @2015-1-5*/
	if (charger->charge_force_off)
		return;
		
	if (charger->charge_on)
		return;
	
	if (charger->cfg_items.ext_charger_exist) 
	{
		/*use external charger, such as tp5000*/
		if (wall_connected == WALL_PLUGED) 
		{
			/*WALL pluged, no matter USB pluged or not*/
			if (charger->cur_bl_status) {
				/* backlight is on, turn off external charger to prevent temprature too high*/
				__gpio_set_value(charger->cfg_items.gpio_ext_chg_ctrl_pin, 1);
			} 
			else 
			{
			      /*  backlight is off, First, turn off internal charger.	
				* Then, turn on external charger
				*/
				charger->set_charge(charger->atc260x, 0);
				msleep(5000);
				__gpio_set_value(charger->cfg_items.gpio_ext_chg_ctrl_pin, 0);
				msleep(5000);
			}               

			/* then turn on internal charger*/
			charger->set_charge(charger->atc260x, 1);
		} 
		else if (charger->charger_cur_status == USB_PLUGED) 
		{
			/*Only USB pluged.turn off external charger, no matter charged or not previously. */
			__gpio_set_value(charger->cfg_items.gpio_ext_chg_ctrl_pin, 1);

			/*then turn on internal charger.*/
			charger->set_charge(charger->atc260x, 1);
		}
	} 
	else 
	{
		/*use internal charger, such as 2603A.*/
		charger->set_charge(charger->atc260x, 1);
		charger->charge_on = true;
		/*set pwm level to  NO_CHARGER_PWM_LEVEL to improve power efficiency*/
		
		if (charger->cfg_items.ext_dcdc_exist)
		{
			if (IS_ERR_OR_NULL(charger->pwm))
			{
				pr_err("charger->pwm is NULL or ERR!!\n");
				return;
			}
			pwm_config(charger->pwm,
				((charger->pwm->period) >> 6) * NO_CHARGER_PWM_LEVEL,
				charger->pwm->period);
		}
	}
	
	log_event_none(LOG_HEADER_CHARGER_ON);
}
void atc260x_charger_turn_on_force(void)
{
	struct atc260x_charger *charger = get_atc260x_charger();
	charger->charge_force_off = false;
	if(charger_once_on)
		atc260x_charger_turn_on();
}
EXPORT_SYMBOL_GPL(atc260x_charger_turn_on_force);

void atc260x_charger_turn_off(void)
{	
	struct atc260x_charger *charger = get_atc260x_charger();
	
	if (!charger->charge_on)
		return;
	
	if (charger->cfg_items.ext_charger_exist) 
	{
		/*turn off external charger.*/
		__gpio_set_value(charger->cfg_items.gpio_ext_chg_ctrl_pin, 1);
	} 

	// turn off internal charger, no matter charged or not.
	charger->set_charge(charger->atc260x, 0);
	charger->charge_on = false;
	if (!charger->cfg_items.ext_charger_exist) 
	{
		/*charging is stopped, set pwm level = NO_CHARGER_PWM_LEVEL*/
		if (charger->cfg_items.ext_dcdc_exist)  
		{
			if (IS_ERR_OR_NULL(charger->pwm))
			{
				pr_err("charger->pwm is NULL or ERR!!\n");
				return;
			}
			pwm_config(charger->pwm,
				((charger->pwm->period) >> 6) * NO_CHARGER_PWM_LEVEL,
				charger->pwm->period);
		}
	}

	log_event_none(LOG_HEADER_CHARGER_OFF);
}
void atc260x_charger_turn_off_force(void)
{
	struct atc260x_charger *charger = get_atc260x_charger();
	charger->charge_force_off = true;
	atc260x_charger_turn_off();
	return;
}
EXPORT_SYMBOL_GPL(atc260x_charger_turn_off_force);


static void led_state_charge(struct atc260x_charger *charger){

    if(charger->led_status)
        return ;
    if (charger->cfg_items.charger_led_exist) {
      __gpio_set_value(charger->cfg_items.gpio_led_inner_pin, 1);
    }
    charger->led_status=1;
}
static void led_state_normal(struct atc260x_charger *charger){
	if(!charger->led_status)
		return ;   
	
	if (charger->cfg_items.charger_led_exist) 
	{
		__gpio_set_value(charger->cfg_items.gpio_led_inner_pin, 0);
	}    

	charger->led_status=0;
}

#define PWM_LEVEL_CHANGE_STEP    5
#define VOLT_DIFF_THRESHOLD      50
static int pwm_batv_count = 0;		
static int pwm_batv_sum = 0;	
static int pre_pwm_level = PWM_MAX_LEVEL;		
static void atc260x_power_update_pwm(struct atc260x_charger *charger)
{
	int data = 0;
	int ret = -1;

	int voltage_diff;
	int bat_avr_vol;
	int pwm_level;
	int wall_vol;
	int bat_vol;

	if (!charger->bat_is_exist)
	{
		if (charger->cfg_items.ext_dcdc_exist== 1) 
		{
			pwm_config(charger->pwm,
						((charger->pwm->period) >> 6) * PWM_MAX_LEVEL,
						charger->pwm->period);
			power_dbg("[%s]duty_ns = %d,priod_ns= %d\n",
						__func__,((charger->pwm->period) >> 6) * PWM_MAX_LEVEL,charger->pwm->period);
		}
		return ;
	}
	
	if (charger->cfg_items.ext_dcdc_exist== 1) 
	{
		if(charger->charge_on)
		{	
			ret = atc260x_power_get_vol(charger, &data);
			if(ret >= 0){
				pwm_batv_sum += data;
			}else{
				pwm_batv_sum += charger->read_adc(charger->atc260x, "BATV");
			}
			pwm_batv_count++;
		} 
		else 
		{
			pwm_batv_sum = 0;
			pwm_batv_count = 0;
		}

		if (!charger->cfg_items.ext_charger_exist) {
			/* if external charger not exist, then adjust pwm level every 20s during charging state.*/
			if (pwm_batv_count == 10)
			{	
				power_dbg("******time to config pwm*******\n");
				// adjust different voltage depend on backlight.
				if (charger->cur_bl_status == BL_OFF)
				{
					voltage_diff = charger->cfg_items.backlight_off_vol_diff;
				} 
				else
				{
					voltage_diff = charger->cfg_items.backlight_on_vol_diff;
				}

				bat_avr_vol = pwm_batv_sum /pwm_batv_count ;

				wall_vol = charger->read_adc(charger->atc260x, "WALLV");
				
				pwm_level = get_pwm_level(CONFIG_CURVE_ARG0, CONFIG_CURVE_ARG1, bat_avr_vol, voltage_diff);  
				
				power_dbg("wall_vol = %d,bat_vol =%d,pwm_level = %d\n",wall_vol,bat_avr_vol,pwm_level);
				power_dbg("wall_vol - bat_avr_vol = %d\n",wall_vol - bat_avr_vol);
				if (pre_pwm_level == PWM_MAX_LEVEL)
				{
					pre_pwm_level = pwm_level;//init
				}
				if(wall_vol - bat_avr_vol < voltage_diff)
				{
					if (pwm_level <= pre_pwm_level)
					{
						if (pre_pwm_level < PWM_MAX_LEVEL)
						{
							pre_pwm_level += PWM_LEVEL_CHANGE_STEP;
							if (pre_pwm_level >= PWM_MAX_LEVEL)
								pre_pwm_level = PWM_MAX_LEVEL-1;//avoid to be same as PWM_MAX_LEVEL
						}
							
						pwm_level = pre_pwm_level;
						power_dbg("wall+:the reconfig pwm level is:%d\n",pwm_level);
					}
				}
				else
				{
					pwm_level = pre_pwm_level;
				}
				
				if (wall_vol - bat_avr_vol > voltage_diff + VOLT_DIFF_THRESHOLD)
				{
					if (pwm_level >= pre_pwm_level)
					{
						if (pre_pwm_level < PWM_MAX_LEVEL)
						{
							pre_pwm_level -= PWM_LEVEL_CHANGE_STEP;
							if (pre_pwm_level < 0)
								pre_pwm_level = 0;
							pwm_level = pre_pwm_level;
						}
						else
							pre_pwm_level = pwm_level;
						power_dbg("wall-:the reconfig pwm level is:%d\n",pwm_level);
					}
				}

				pwm_config(charger->pwm,
					((charger->pwm->period) >> 6) * pwm_level,
					charger->pwm->period);
				
				wall_vol = charger->read_adc(charger->atc260x, "WALLV");
				bat_vol = charger->read_adc(charger->atc260x, "BATV");
				
				power_dbg("after pwm config,pwm_level=%d,wall_vol=%d,bat_vol=%d,wall_vol-bat_vol=%d\n",
					pwm_level,wall_vol,bat_vol,wall_vol-bat_vol);
				
				pre_pwm_level = pwm_level;
				
				pwm_batv_sum = 0;
				pwm_batv_count = 0;			    
			}
		}

	}    
}

/**
  * update_fast_charge_state - enalbe/disable fast charge
  * 
  * disable fast charge if bat is full. 
  */
static void update_fast_charge_state(struct atc260x_charger *charger)
{
	if (!charger->bat_is_exist)
	{
		return ;
	}
	
	if (charger->cur_bat_cap == 100) 
	{
		if (charger->cv_enabled) 
		{
			charger->cv_set(charger, 4200);
			charger->cv_enabled = false;
		}
	} 
	else 
	{
		if (charger->cv_enabled == false) 
		{
			charger->cv_set(charger, 4300);
			charger->cv_enabled = true;         
		}
	}
}


static void atc260x_update_voltage(struct atc260x_charger *charger){
    
    struct atc260x_power* power = container_of(charger, struct atc260x_power, charger);
    power->charger.wall_mv = charger->read_adc(power->atc260x, 
            "WALLV");
    power->charger.vbus_mv =  charger->read_adc(power->atc260x, 
            "VBUSV");

}

/*
 * update adatper type
 * pm driver will query adapter_type
 *
 */
static void update_adapter_type(struct atc260x_charger *charger){
	int chg_mode=charger->charger_cur_status;
	if (chg_mode == NO_PLUGED)
		atomic_set(&adapter_type, ADAPTER_TYPE_NO_PLUGIN);

	else if (chg_mode == WALL_PLUGED)
		atomic_set(&adapter_type, ADAPTER_TYPE_WALL_PLUGIN);

	else if(chg_mode == USB_PLUGED)
	{
		if((chg_mode & USB_PLUGED) 
			&& (charger->usb_pluged_type ==USB_PLUGED_PC)) 
		{
			atomic_set(&adapter_type, ADAPTER_TYPE_PC_USB_PLUGIN);
		}

		if ((chg_mode & USB_PLUGED) 
			&& (charger->usb_pluged_type == USB_PLUGED_ADP))
		{
			atomic_set(&adapter_type, ADAPTER_TYPE_USB_ADAPTER_PLUGIN);
		}
	}
	else if(chg_mode == (WALL_PLUGED | USB_PLUGED))
		atomic_set(&adapter_type, ADAPTER_TYPE_USB_WALL_PLUGIN); 
	else
		pr_err("do not support this adapter type!\n");
}

static int get_batv_avr(struct atc260x_charger *charger)
{
	int data = 0;
	int ret = -1;

	int sum = 0;
	int count = 0;
	for (; count < 3; count++)
	{
		ret = atc260x_power_get_vol(charger, &data);
		if(ret >= 0){
			sum += data;
		}else{
			sum += charger->read_adc(charger->atc260x, "BATV");
		}
		msleep(5);
	}

	return sum /count;
}

static void atc260x_check_bat_online(struct atc260x_charger *charger)
{
	int ret;
	
	if (first_power_on)
	{
		/*check if battery is online*/
		batv_before_check = get_batv_avr(charger);
		ret = charger->check_bat_online(charger->atc260x);
		if (ret <= 0) 
		{
			pr_warn("\n[power] No battery detected\n");
			charger->bat_is_exist = false;
		} 
		else 
		{
			pr_info("\n[power] Battery detected\n");
			charger->bat_is_exist = true;
		}
		batv_after_check = get_batv_avr(charger);
	}
}

/**
  * udpate battery capacity
  *
  */
static int update_battery_level(struct atc260x_charger *charger)
{
	int data = 0;
	int ret = -1;

	if (!charger->bat_is_exist)
		return -ENXIO;

	ret = atc260x_power_get_cap(charger , &data);
	if(ret >= 0) {
		charger->cur_bat_cap = data;
	} else {
		return -EFAULT;
	}
	return 0;
}


static void update_battery_state(struct atc260x_charger *charger)
{
	int data = 0;
	int ret = -1;

	int status;
	
	atc260x_check_bat_online(charger);
	
	/*phrase update*/
	charger->update_phrase_type(charger);


	/* update current*/
	ret = atc260x_power_get_cur(charger , &data);
	if(ret >= 0){
		charger->bat_ma = data;
	}else{
		atc260x_bat_check_status(charger, &status);
		if ((status == POWER_SUPPLY_STATUS_CHARGING) ||
			(status == POWER_SUPPLY_STATUS_FULL))
			charger->bat_ma = charger->read_adc(charger->atc260x, "CHGI");
	}
	

	/*update voltate*/
	ret = atc260x_power_get_vol(charger, &data);
	if(ret >= 0){
		charger->bat_mv = data;
	}else{
		/*pr_info("we do not get bat_mv from gauge\n");*/
		charger->bat_mv = charger->read_adc(charger->atc260x, "BATV");
	}

	ret = atc260x_power_get_temp(charger, &data);
	if(ret >= 0){
		charger->bat_temp = data;
	}else{
		//printk("temp has not rigest\n");
	}
	
	update_battery_level(charger);
}

/**
 *  update_charge_state - update charger current status
 *
 */
static void update_charge_state( struct atc260x_charger *charger)
{
	charger->charger_cur_status = atc260x_charger_check_online(charger);
	//ywwang todo
	if (charger->cfg_items.support_adaptor_type == SUPPORT_DCIN)
	{
		if (charger->charger_cur_status & WALL_PLUGED)  
		{
			charger->extern_power_online = true;
		}
		else
		{
			charger->extern_power_online = false;
		}
	}
	else  /* SUPPORT_DCIN_USB or SUPPORT_USB */
	{
		charger->extern_power_online = charger->charger_cur_status!= NO_PLUGED;
	}

	/*vbus/wall voltage*/ 
	atc260x_update_voltage(charger);

	if (owl_backlight_is_on()) 
	{
		charger->cur_bl_status = BL_ON;
	} 
	else 
	{
		charger->cur_bl_status = BL_OFF;
	}

	update_adapter_type(charger);
     
}


static void commit_charge_state(struct atc260x_charger *charger)
{
	charger->charger_pre_status = charger->charger_cur_status;
	charger->pre_bl_status = charger->cur_bl_status;
	charger->pre_bat_cap = charger->cur_bat_cap;
	
	charger->usb_pluged_type_changed = false;
	enter_later_resume = false;	
}

static void handle_tp_calibration( struct atc260x_charger *charger)
{
	int charger_changed= charger->charger_cur_status!=charger->charger_pre_status;
	int charger_on = charger->charger_cur_status !=NO_PLUGED;

	if (charger_changed &&charger->adjust_tp_para) 
	{ 
		charger->adjust_tp_para(charger_on);
	}
}

/**
 * turn_off_charger_if_needed - turn off charger under some conditions
 *
 * turn off charger under following conditions:
 * 1.battery is offline;
 * 2.all adapters are offline;
 * 3.bat capacity is full.
 */
static void turn_off_charger_if_needed(struct atc260x_charger *charger)
{
	/*battery is removed*/
	if (!charger->bat_is_exist) 
	{
		/* disable charger if battery is removed */
		if (charger->charge_on) 
		{
			led_state_normal(charger);
			atc260x_charger_turn_off();
			power_info("%s:battery doesn't exist,charger turn off\n", __func__);
			charger_once_on = false;
		}
		
		return;
	}

	/*charger hotplug*/
	if(!charger->extern_power_online && charger->charge_on)
	{
		led_state_normal(charger);
		atc260x_charger_turn_off();
		power_info("%s:hotplug-out, charger turn off\n", __func__);
		charger_once_on = false;
	}

	/*charger is on , but battery is full now,  we close led*/
	// ywwang  todo
	if (charger->extern_power_online && (charger->cur_bat_cap < 100)) 
	{
		led_state_charge(charger);
	} 
	else 
	{
		if(charger->charge_on)
		{
			led_state_normal(charger);
			atc260x_charger_turn_off();
			power_info("%s:power full,charger turn off\n", __func__);
			charger_once_on = false;
		}
	}

}

/**
 * full_vol_is_low - compare the bat vol that bat is full with current bat voltage
 * @return : if the diff less than 100mv return false, otherwise return true.

static bool bat_vol_is_too_high(struct atc260x_charger *charger)
{
#define BAT_VOL_DIFF (100)

	if (charger->cur_bat_cap == 100)
	{
		if (charger->full_vol - charger->bat_mv < BAT_VOL_DIFF)
			return true;
		else
			return false;
	}
	else
		return false;
}
 */
static void turn_on_charger_if_needed(struct atc260x_charger *charger)
{
	if (owl_get_boot_mode() == OWL_BOOT_MODE_UPGRADE) {
		//printk("turn_on_charger_if_needed: :upgrade exit charger\n");
		return;
	}

	if(!charger->bat_is_exist || 
		charger->charge_on ||
		!charger->extern_power_online || 
		charger->cur_bat_cap == 100 ||
		charger->battery_status != POWER_SUPPLY_HEALTH_GOOD)
	{
		charger_delay_counter=-1;
		return ;
	}

	WARN_ON(charger_delay_counter > 7);
	charger_delay_counter++;
	if (!charger->charge_on && charger_delay_counter == 7)
	{
		atc260x_charger_turn_on();
		if(charger->charge_on)
		{
			power_info("%s:charger turn on\n", __func__);
			charger_once_on = true;
			led_state_charge(charger);
		}

		charger_delay_counter = -1;
	}
}

/**
 * atc260x_set_future_current - set future constant current
 *
 * we need write the constant current  into reg.
 */
static unsigned int  atc260x_get_future_current(struct atc260x_charger *charger, int mode)
{
	unsigned int set_current = 0;
	
	switch (mode)
	{
		case WALL_PLUGED:
		if (charger->cur_bl_status == BL_ON) 
		{
			set_current = charger->cfg_items.bl_on_current_wall_adp;	
		} 
		else 
		{
			set_current = charger->cfg_items.bl_off_current_wall_adp;
		}
		break;

		case USB_PLUGED:
		if (charger->usb_pluged_type == USB_PLUGED_PC) 
		{
			// usb_pc pluged
			if (charger->cur_bl_status == BL_ON) 
			{
				set_current = charger->cfg_items.bl_on_current_usb_pc;	
			} 
			else 
			{
				set_current = charger->cfg_items.bl_off_current_usb_pc;
			}
		} 
		else if (charger->usb_pluged_type == USB_PLUGED_ADP)
		{
			// usb_adaptor pluged
			if (charger->cur_bl_status == BL_ON) 
			{
				set_current = charger->cfg_items.bl_on_current_usb_adp;	
			} 
			else 
			{
				set_current = charger->cfg_items.bl_off_current_usb_adp;
			}
		} 
		else 
		{
			// not sure, default as usb_pc pluged
			if (charger->cur_bl_status == BL_ON) 
			{
				set_current = charger->cfg_items.bl_on_current_usb_pc;	
			} 
			else 
			{
				set_current = charger->cfg_items.bl_off_current_usb_pc;
			}
		}	
		
		break;

		default:
			set_current = charger->cfg_items.bl_on_current_usb_pc;
			break;
			
	}

	return set_current;	
}

static int atc260x_real_current_to_binary(struct atc260x_charger *charger,int set_current)
{
	int ic_type;
	int ic_version;
	int binary_current;
	
	ic_type = atc260x_get_ic_type(charger->atc260x);
	ic_version = atc260x_get_version(charger);
	
	if(ic_type == ATC260X_ICTYPE_2603C)
	{
		if(set_current >=1 && set_current <= 2)
			binary_current = set_current;
		else if(set_current >= 4 && set_current <= 6)
			binary_current = set_current - 1;
		else if(set_current >= 8 && set_current <= 10)
			binary_current = set_current - 2;
		else if(set_current >= 12 && set_current <= 14)
			binary_current = set_current - 3;
		else if(set_current >= 16 && set_current <= 18)
			binary_current = set_current - 4;
		else if(set_current == 20)
			binary_current = set_current - 5;
		else
		{
			binary_current = 2;  /*200mA*/
			pr_warn("have not the responding value you are setting ,now set it to be 200mA!\n");
		}
	}
	else if (ic_type == ATC260X_ICTYPE_2603A)
	{
		if( ic_version == ATC260X_ICVER_A || 
			ic_version == ATC260X_ICVER_B || 
			ic_version == ATC260X_ICVER_C)
			binary_current = set_current;
		else
			binary_current = set_current * PMU_VERSION_D_CC_RATIO / 4;
	}
	else
	{
		pr_err("we do not support this ic_type!!\n");
		return -EINVAL;
	}
	return binary_current;
}

static void atc260x_charger_adjust_current(struct atc260x_charger *charger)
{
	unsigned int set_current = 0;
	unsigned int current_binary;
	if (charger->cfg_items.support_adaptor_type == SUPPORT_DCIN) 
	{
		if (charger->charger_cur_status & WALL_PLUGED) 
		{
			set_current = atc260x_get_future_current(charger, WALL_PLUGED);
		}
	}
	else if (charger->cfg_items.support_adaptor_type == SUPPORT_DCIN_USB)
	{
		if (charger->charger_cur_status & WALL_PLUGED) 
		{
			set_current = atc260x_get_future_current(charger, WALL_PLUGED);
		}
		else if (charger->charger_cur_status & USB_PLUGED)
		{
			set_current = atc260x_get_future_current(charger, USB_PLUGED);
		}
	}
	else if (charger->cfg_items.support_adaptor_type == SUPPORT_USB)
	{
		set_current = atc260x_get_future_current(charger, WALL_PLUGED);
	}
	else
	{
		pr_warn("%s: dont support adapter type? \n", __func__);
	}
	
	/*added by cxj @2014-12-24*/
	current_binary = atc260x_real_current_to_binary(charger, set_current);

	charger->set_constant_current(charger, current_binary);
	power_info("%s:set constant current:%dmA(binary:%x)\n", 
				__func__,  set_current * 100,current_binary);
}


static void change_charge_current_if_needed(struct atc260x_charger *charger)
{
	bool  bl_changed = (charger->cur_bl_status == charger->pre_bl_status) ? false : true;
	bool mode_changed = (charger->charger_cur_status == charger->charger_pre_status) ? false : true;
		
	if(charger->charger_cur_status == NO_PLUGED || !charger->bat_is_exist)
	{
		return ;
	}
	
	if(bl_changed || mode_changed || charger->usb_pluged_type_changed || enter_later_resume)
	{
		atc260x_charger_adjust_current(charger);  
		
	}
}


static void  report_power_supply_change(struct atc260x_charger *charger)
{
	struct atc260x_power *power;
	int battery_level_changed = charger->pre_bat_cap != charger->cur_bat_cap;
	int supply_changed = charger->charger_pre_status != charger->charger_cur_status;
	power = container_of(charger, struct atc260x_power, charger);

	if(battery_level_changed || (charger->cur_bat_cap == 0 && charger->bat_is_exist))
	{
		log_event_int(LOG_HEADER_BATTERY, charger->cur_bat_cap);
		power_supply_changed(&charger->psy);
	}
	else if (charger->info_counter > ATC260X_INFO_UPDATE_INTERVAL)
	{
		log_event_int(LOG_HEADER_BATTERY, charger->cur_bat_cap);

		power_supply_changed(&charger->psy);      
		charger->info_counter = 0;   
	}
	else
	{
		charger->info_counter += charger->interval;
	}

	if (first_power_on && (charger->info_counter == (8 * HZ))) 
	{
		power_supply_changed(&charger->psy);
		//first_power_on = false;
	}
	
	// avoid system unstable when temperature is too high.
	if (charger->bat_temp > 55) 
	{
		// ywwang: todo
		power_supply_changed(&charger->psy);
	}

	if(supply_changed)
	{
		log_event_int_int(LOG_HEADER_HOTPLUG, charger->charger_pre_status, charger->charger_cur_status);

		power_supply_changed(&power->wall); 
		power_supply_changed(&power->usb);
	}
    
    if (first_power_on)
	{

		complete_all(&charger->check_complete);

		//complete(&charger->check_usb_complete);

		//complete(&charger->check_bat_complete);

		power_dbg("%s:complete\n", __func__);
		//first_power_on = false;
	}
}

/**
 *new featrue:chenbo,20140506, set vbus control mode depend on usb type. 
 */
static void update_vbus_control_mode(struct atc260x_charger *charger)
{
	if (charger->cfg_items.support_adaptor_type == SUPPORT_DCIN_USB)
	{
		if (charger->charger_cur_status & USB_PLUGED)
		{
			if (charger->usb_pluged_type_changed)
			{
				if  (charger->usb_pluged_type == USB_PLUGED_PC)
				{
					if (charger->cfg_items.enable_vbus_current_limited)
					{
						charger->set_vbus_ctl_mode(charger,  CURRENT_LIMITED);
						charger->vbus_control_mode = CURRENT_LIMITED;
					}
					else
					{
						charger->set_vbus_ctl_mode(charger,  CANCEL_LIMITED);
						charger->vbus_control_mode = CANCEL_LIMITED;
					}
					
				}
				else if  (charger->usb_pluged_type == USB_PLUGED_ADP)
				{
					charger->set_vbus_ctl_mode(charger, VOLTAGE_LIMITED);
					charger->vbus_control_mode = VOLTAGE_LIMITED;
				}  
			}
		}
		else if (charger->charger_pre_status != charger->charger_cur_status)
		{
			charger->set_vbus_ctl_mode(charger, CURRENT_LIMITED);
			charger->vbus_control_mode = CURRENT_LIMITED;
		}
	}
}



static void atc260x_charging_monitor(struct work_struct *work)
{
	struct atc260x_charger *charger;
	struct atc260x_power *power;
	
	charger = container_of(work, struct atc260x_charger, work.work);
	power = container_of(charger, struct atc260x_power, charger);
	/* step 1 : check if any charge cable is connected*/
	update_battery_state(charger);
	update_charge_state(charger);
	
	atc260x_disable_vbus_id_if_needed(charger);

	update_fast_charge_state(charger);
	atc260x_power_update_pwm(charger);

	/*step 1.1  stay awak or allow sleep ?*/
	request_system_state(charger);

	/* step 2: handle tp calibration*/
	handle_tp_calibration(charger);

	/* step 3  turn off charger if there is no adapter connected with charger */
	turn_off_charger_if_needed(charger);

	/* step 4 turn on charger if any adapter  is dected */
	turn_on_charger_if_needed(charger);
	
	/* step 6 update vbus control mode if needed.*/
	update_vbus_control_mode(charger);

	/* step 5 dynamic change charge current : not to be too hot....*/
	change_charge_current_if_needed(charger);

	if ((charger->update_charger_mode != NULL) && charger->charge_on)
		charger->update_charger_mode(charger);

	/* step 7 monitor battery health status.*/
	if (charger->check_health)
		charger->check_health(charger, &charger->health);

	/* step 8 report battery info, including bat capacity, charger hotplug event, etc*/
	report_power_supply_change(charger);

	/* step 9 here we commit all status:  pre = cur*/
	commit_charge_state(charger);

	/* reschedule for the next time */
	queue_delayed_work(charger->charger_wq, &charger->work, msecs_to_jiffies(charger->interval * 1000));
}

static int atc260x_bat_check_status(struct atc260x_charger *charger, int *status)
{
	if (!charger->bat_is_exist ||
		(charger->battery_status != POWER_SUPPLY_HEALTH_GOOD)) 
	{
		*status = POWER_SUPPLY_STATUS_UNKNOWN;	
	} 
	else if (charger->extern_power_online) 
	{
		if (charger->cur_bat_cap == 100) 
		{
			*status = POWER_SUPPLY_STATUS_FULL;
		} 
		else 
		{
			*status = POWER_SUPPLY_STATUS_CHARGING;
		}
	}
	else 
	{
		*status = POWER_SUPPLY_STATUS_DISCHARGING;
	}

	return 0;
}

int atc260x_bat_check_type(struct atc260x_charger *charger, int *type)
{
	switch (charger->chg_type) 
	{
	case PMU_CHARGER_PHASE_PRECHARGE:
	    *type = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
	    break;
		
	case PMU_CHARGER_PHASE_CONSTANT_CURRENT:
	case PMU_CHARGER_PHASE_CONSTANT_VOLTAGE:       
	    *type = POWER_SUPPLY_CHARGE_TYPE_FAST;
	    break;
		
	default:
	    *type = POWER_SUPPLY_CHARGE_TYPE_NONE;
	    break;
	}

	return 0;
}


static int atc260x_bat_get_props(struct power_supply *psy,
                   enum power_supply_property psp,
                   union power_supply_propval *val)
{
	struct atc260x_power* power = dev_get_drvdata(psy->dev->parent);
	struct atc260x_charger* charger = &power->charger;
	int ret = 0;
	
	if (first_power_on)
	{	

		wait_for_completion(&charger->check_complete);
		mutex_lock(&charger->lock);
		if(++count_completion == 3)
		{
			first_power_on = false;
			pr_info("[%s] first_power_on = false!\n",__func__);
		}	
		mutex_unlock(&charger->lock);
	}
	
	switch (psp) 
	{
		case POWER_SUPPLY_PROP_STATUS:
			ret = atc260x_bat_check_status(charger, &val->intval);
			break;
			
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = charger->bat_is_exist;
			break;
			
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = charger->bat_is_exist;
			break;
			
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = charger->bat_mv * 1000;/* mV -> uV */  
			break;
			
		case POWER_SUPPLY_PROP_CURRENT_NOW:
			val->intval = charger->bat_ma;
			val->intval *= 1000;                /* mA -> uA */
			break;
			
		case POWER_SUPPLY_PROP_HEALTH:
			val->intval = charger->health;
			break;
			
		case POWER_SUPPLY_PROP_CHARGE_TYPE:
			ret = atc260x_bat_check_type(charger, &val->intval);
			break;
			
		case POWER_SUPPLY_PROP_CAPACITY:
			if (charger->battery_status != POWER_SUPPLY_HEALTH_GOOD)
				val->intval = -99;
			else
				val->intval = charger->cur_bat_cap;
			break;
			
		case POWER_SUPPLY_PROP_TECHNOLOGY:
			val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
			break;

		case POWER_SUPPLY_PROP_TEMP:
			val->intval = charger->bat_temp * 10;
			break;

		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static enum power_supply_property atc260x_bat_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_TEMP,
};

/*static void atc260x_bat_convert_thresholds(struct atc260x_charger *charger,
                          struct atc260x_battery_pdata *pdata)
{
	charger->thresholds.vbat_charge_start = pdata->vbat_charge_start;
	charger->thresholds.vbat_charge_stop = pdata->vbat_charge_stop;
	charger->thresholds.vbat_low = pdata->vbat_low;
	charger->thresholds.vbat_crit = pdata->vbat_crit;

	charger->thresholds.trickle_charge_current = 
		(pdata->trickle_charge_current ? : 200);
	charger->thresholds.constant_charge_current = 
		(pdata->constant_charge_current ? : 1000);
}*/
/* added by cxj@2014/10/14 */
static void atc260x_vbus_init(struct atc260x_charger *charger)
{
	//charger->set_apds_vbus_pd(charger, true);
	if(!charger->g_vbus_is_otg)
	{
		if (charger->cfg_items.support_adaptor_type == SUPPORT_USB) 
		{
			charger->set_vbus_path(charger, false);/* always cut */
			charger->set_apds_vbus_pd(charger, false);	
		}
		else
		{
			charger->set_vbus_path(charger, true);
			charger->set_apds_vbus_pd(charger, true);
		}	
	}
}
/* added by cxj@2014/10/14 */
static void atc260x_dcdc_init(struct atc260x_charger *charger)
{
	int pwm_level = PWM_MAX_LEVEL;
	if (charger->cfg_items.ext_dcdc_exist == 1) 
	{
		if (charger->cfg_items.ext_charger_exist) 
		{
			// if external charger exist, then set max value to pwm3. 
			pwm_config(charger->pwm,
					((charger->pwm->period) >> 6) * pwm_level,
					charger->pwm->period);
		} 
		else
		{
			pwm_level = get_pwm_level(CONFIG_CURVE_ARG0, CONFIG_CURVE_ARG1, 4500, 350);  
			pwm_config(charger->pwm,
					((charger->pwm->period) >> 6) * pwm_level,
					charger->pwm->period);
			pwm_enable(charger->pwm);
			power_dbg("[%s]duty_ns = %d,priod_ns= %d\n",
					__func__,((charger->pwm->period) >> 6) * pwm_level,charger->pwm->period);
		}
	}
}

static int atc260x_charger_init(struct atc260x_charger *charger)
{
	power_dbg("\n[power]  vbat_charge_start:%d mV, vbat_charge_stop:%d mV, "
	"\nvbat_low:%d mV, vbat_crit:%d mV, trickle_charge_current:%d mA, constant_charge_current:%d mA\n", 
	charger->thresholds.vbat_charge_start,
	charger->thresholds.vbat_charge_stop,
	charger->thresholds.vbat_low,
	charger->thresholds.vbat_crit,
	charger->thresholds.trickle_charge_current,
	charger->thresholds.constant_charge_current);

	charger->charger_phy_init(charger);
	if (charger->cfg_items.support_adaptor_type == SUPPORT_USB)
	{
		charger->set_vbus_ctl_mode(charger, VOLTAGE_LIMITED);
	}
	else
	{
		if (charger->cfg_items.enable_vbus_current_limited)
			charger->set_vbus_ctl_mode(charger, CURRENT_LIMITED);
		else
			charger->set_vbus_ctl_mode(charger, CANCEL_LIMITED);
	}

	charger->bat_mv = charger->read_adc(charger->atc260x, "BATV") * 1000;
	charger->bat_counter = 0;
	charger->info_counter = 0;
	power_dbg("\n[power] volatge(mV): wall %d  vbus %d  bat %d",
	charger->wall_mv, charger->vbus_mv, charger->bat_mv);

	charger->charge_on = false;
	charger->cur_bl_status = BL_ON;
	charger->pre_bl_status = -1;
	//charger->charger_pre_status = 0;
	charger->extra_chg_count = 0;
	charger->usb_pluged_type = USB_NO_PLUGED;
	charger->usb_pluged_type_changed = false;
	charger->led_status=0;
	charger->wakelock_acquired=0;
	
	charger->cv_enabled = true;/* added by cxj */
	charger->cv_set(charger, 4300);
	/*
	* vbus init
	*/
	atc260x_vbus_init(charger);
	/*
	* external dcdc init
	*/
	atc260x_dcdc_init(charger);

	return 0;
}

void act260x_set_get_cap_point(void *ptr)
{
	if (global_power_dev != NULL) 
	{ 
	    global_power_dev->charger.get_gauge_cap = ptr;
	}
}
EXPORT_SYMBOL_GPL(act260x_set_get_cap_point);

void act260x_set_get_volt_point(void *ptr)
{
	if (global_power_dev != NULL) 
	{ 
		global_power_dev->charger.get_gauge_volt = ptr;
	}
}
EXPORT_SYMBOL_GPL(act260x_set_get_volt_point);

void act260x_set_get_cur_point(void *ptr)
{
	if (global_power_dev != NULL) 
	{ 
		global_power_dev->charger.get_gauge_cur = ptr;
	}
}
EXPORT_SYMBOL_GPL(act260x_set_get_cur_point);

void act260x_set_get_temp_point(void *ptr)
{
	if (global_power_dev != NULL) 
	{ 
		global_power_dev->charger.get_gauge_temp = ptr;
	}
}
EXPORT_SYMBOL_GPL(act260x_set_get_temp_point);



void act260x_set_get_hw_cap_point(void *ptr)
{
	if (global_power_dev != NULL) 
	{ 
	    global_power_dev->charger.get_hw_gauge_cap = ptr;
	}
}
EXPORT_SYMBOL_GPL(act260x_set_get_hw_cap_point);

void act260x_set_get_hw_volt_point(void *ptr)
{
	if (global_power_dev != NULL) 
	{ 
		global_power_dev->charger.get_hw_gauge_volt = ptr;
	}
}
EXPORT_SYMBOL_GPL(act260x_set_get_hw_volt_point);

void act260x_set_get_hw_cur_point(void *ptr)
{
	if (global_power_dev != NULL) 
	{ 
		global_power_dev->charger.get_hw_gauge_cur = ptr;
	}
}
EXPORT_SYMBOL_GPL(act260x_set_get_hw_cur_point);

void act260x_set_get_hw_temp_point(void *ptr)
{
	if (global_power_dev != NULL) 
	{ 
		global_power_dev->charger.get_hw_gauge_temp = ptr;
	}
}
EXPORT_SYMBOL_GPL(act260x_set_get_hw_temp_point);

void atc260x_set_adjust_tp_para(void *ptr)
{
	if (global_power_dev != NULL) 
	{ 
		global_power_dev->charger.adjust_tp_para = ptr;
	}
}
EXPORT_SYMBOL_GPL(atc260x_set_adjust_tp_para);
/*modified by cxj@2015/1/12*/
int atc260x_get_charge_status(void)
{
	if (global_power_dev != NULL) 
	{
		return global_power_dev->charger.charge_on;
	}
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(atc260x_get_charge_status);

void atc260x_get_batv_from_charger(int *batv1, int *batv2)
{
	*batv1 = batv_before_check;
	*batv2 = batv_after_check;
}
EXPORT_SYMBOL_GPL(atc260x_get_batv_from_charger);

/* export to cap_gauge driver */
int pmu_reg_read(unsigned short reg)
{
    struct atc260x_dev *atc260x = get_atc260x_dev();
    int ret;
    
    ret = atc260x_reg_read(atc260x, reg);
    if (ret < 0)
    	return -1;
    	
    return ret;
}
EXPORT_SYMBOL_GPL(pmu_reg_read);

int pmu_reg_write(unsigned short reg, unsigned short val)
{
    struct atc260x_dev *atc260x = get_atc260x_dev();
    int ret;
    
    ret = atc260x_reg_write(atc260x, reg, val);
    if (ret < 0)
    	return -1;
    	
    return ret;
}
EXPORT_SYMBOL_GPL(pmu_reg_write);



void atc260x_set_usb_plugin_type(int type)
{
	if (global_power_dev != NULL) 
	{	
		global_power_dev->charger.usb_pluged_type = type;
		global_power_dev->charger.usb_pluged_type_changed = true;
		pr_info("[%s]usb_pluged_type = %d\n",__func__,global_power_dev->charger.usb_pluged_type);
	}
}
EXPORT_SYMBOL_GPL(atc260x_set_usb_plugin_type);

int get_chg_current_now(void)
{
	int ic_type;
	int ic_version;
	int chg_current_now;

	struct atc260x_charger *charger = get_atc260x_charger();
	ic_type = get_pmu_ic_type(charger);
	ic_version = atc260x_get_version(charger);
	chg_current_now = charger->get_constant_current(charger);
	if(ic_type == ATC260X_ICTYPE_2603C)
	{
		if (chg_current_now >= 1 && chg_current_now <= 2)
			chg_current_now += 0;
		else if (chg_current_now >= 3 && chg_current_now <= 5)
			chg_current_now += 1;
		else if (chg_current_now >= 6 && chg_current_now <= 8)
			chg_current_now += 2;
		else if (chg_current_now >= 9 && chg_current_now <= 11)
			chg_current_now += 3;
		else if (chg_current_now >= 12 && chg_current_now <= 14)
			chg_current_now += 4;
		else
			chg_current_now += 5;
	}
	else if (ic_type == ATC260X_ICTYPE_2603A)
	{
		if (ic_version == ATC260X_ICVER_A || 
			ic_version == ATC260X_ICVER_B ||
			ic_version == ATC260X_ICVER_C)
			chg_current_now *= 4 / PMU_VERSION_ABC_CC_RATIO;
		else
			chg_current_now *= 4 / PMU_VERSION_D_CC_RATIO +1;
	}
		
	chg_current_now *= 100;
	
	return chg_current_now;
}
EXPORT_SYMBOL_GPL(get_chg_current_now);
int atc260x_set_charger_current(int new, int *old)
{
	struct atc260x_charger *charger = get_atc260x_charger();
	int current_binary;

	*old = get_chg_current_now();
	current_binary = atc260x_real_current_to_binary(charger,new / 100);	
	charger->set_constant_current(charger, current_binary);
	pr_info("[%s]the current set to be:%d(%d)\n",__func__,current_binary,new);

	return 0;
}
EXPORT_SYMBOL_GPL(atc260x_set_charger_current);
/**
 * enable_adjust_current_switch - enable adjust current switch for monitor work.
 * 
 */
 /*do not need timer to queue_delayed_work,but in later resume directly
  *by cxj
static void enable_adjust_current_switch(unsigned long data)
{
	struct atc260x_charger *charger = (struct atc260x_charger *)data;
	printk("%s, enable monitor current adjust\n", __func__);
	
	queue_delayed_work(charger->charger_wq, &charger->work, 0 * HZ);
}
 */

static int atc260x_charger_check_wall_online(struct atc260x_charger *charger)
{
	struct atc260x_dev *atc260x = charger->atc260x;
	if ((charger->read_adc(atc260x, "WALLV") > charger->wall_v_thresh))
		return WALL_PLUGED;
	else 
		return NO_PLUGED;
}

static int atc260x_charger_check_usb_online(struct atc260x_charger *charger)
{
	struct atc260x_dev *atc260x = charger->atc260x;
	
	if (charger->cfg_items.support_adaptor_type == SUPPORT_DCIN)
		return NO_PLUGED;
	
	if (!charger->g_vbus_is_otg) 
	{
		if (charger->read_adc(atc260x, "VBUSV") > 
			ATC260X_VBUS_VOLTAGE_THRESHOLD) 
		{
			if (charger->usb_pluged_type == USB_NO_PLUGED)
			{
				charger->usb_pluged_type = USB_PLUGED_PC;
			} 

			return USB_PLUGED;
		} 
		else 
		{
			charger->usb_pluged_type = USB_NO_PLUGED;

			return NO_PLUGED;
		}
	}

	return NO_PLUGED;
}


static int atc260x_charger_check_online(struct atc260x_charger *charger)
{
	int  chg_mode = NO_PLUGED;

	chg_mode |= atc260x_charger_check_wall_online(charger);
	chg_mode |= atc260x_charger_check_usb_online(charger);
	
	return chg_mode;
}

static ssize_t show_boot_cap_threshold(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct atc260x_charger *charger = container_of(psy, struct atc260x_charger, psy);
	return snprintf(buf, PAGE_SIZE, "%d\n", charger->cfg_items.boot_cap_threshold);	
}

static ssize_t store_boot_cap_threshold(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return 0;	
	
}

static ssize_t show_charger_online(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct atc260x_charger *charger = container_of(psy, struct atc260x_charger, psy);
	union power_supply_propval wall_online, usb_online;

	atc260x_power_check_online(charger, ATC260X_SUPPLY_WALL, &wall_online);
	atc260x_power_check_online(charger, ATC260X_SUPPLY_VBUS, &usb_online);
	return snprintf(buf, PAGE_SIZE, "%d\n", usb_online.intval | wall_online.intval);	
}

static ssize_t store_charger_online(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return 0;	
	
}


static ssize_t show_dump(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct atc260x_charger *charger = container_of(psy, struct atc260x_charger, psy);

	//log_event_dump();
	printk("\n=========================debug information====================\n");
	/*base config*/
	printk("support_adaptor_type:%d\n",charger->cfg_items.support_adaptor_type);
	printk("usb_adapter_as_ac:%d\n",charger->cfg_items.usb_adapter_as_ac);
	printk("bl_on_current_usb_pc:%d\n",charger->cfg_items.bl_on_current_usb_pc);
	printk("bl_off_current_usb_pc:%d\n",charger->cfg_items.bl_off_current_usb_pc);
	printk("bl_on_current_usb_adp:%d\n",charger->cfg_items.bl_on_current_usb_adp);
	printk("bl_off_current_usb_adp:%d\n",charger->cfg_items.bl_off_current_usb_adp);
	printk("bl_on_current_wall_adp:%d\n",charger->cfg_items.bl_on_current_wall_adp);
	printk("bl_off_current_wall_adp:%d\n",charger->cfg_items.bl_off_current_wall_adp);
	printk("ext_dcdc_exist:%d\n",charger->cfg_items.ext_dcdc_exist);
	printk("enable_vbus_current_limited:%d\n",charger->cfg_items.enable_vbus_current_limited);
	/*PMU info*/
	switch(get_pmu_ic_type(charger))
	{
		case ATC260X_ICTYPE_2603A:printk("pmu type:ATC2603A\n");break;
		case ATC260X_ICTYPE_2603C:printk("pmu type:ATC2603C\n");break;
		default:printk("pmu:unknown\n");
	}
	switch(atc260x_get_version(charger))
	{
		case ATC260X_ICVER_A:printk("pmu version : A\n");break;
		case ATC260X_ICVER_B:printk("pmu version : B\n");break;
		case ATC260X_ICVER_C:printk("pmu version : C\n");break;
		case ATC260X_ICVER_D:printk("pmu version : D\n");break;
		default:printk("pmu version : unknown\n");
	}

	/* interval */
	printk("poll interval:%ds\n", charger->interval);
	printk("wakelock_acquired:%d\n", charger->wakelock_acquired);
	/*adapter*/
	if (charger->charger_cur_status & WALL_PLUGED)
	{	
		printk("chg_mode:WALL\n");
		if (charger->cfg_items.ext_dcdc_exist== 1) 
		{
			printk("PWM:duty_ns = %d,priod_ns= %d\n",
						((charger->pwm->period) >> 6) * PWM_MAX_LEVEL,charger->pwm->period);
			printk("wallv-batv:%d\n",charger->wall_mv-charger->bat_mv);
		}
	}

	if (charger->charger_cur_status & USB_PLUGED)
	{
		printk("chg_mode:USB\n");
	}

	if (charger->charger_cur_status & USB_PLUGED)
	{
		if (charger->usb_pluged_type == USB_PLUGED_PC)
			printk("usb pluged type:USB PC\n");
		else if (charger->usb_pluged_type == USB_PLUGED_ADP)
			printk("usb pluged type:USB ADAPTER\n");
	}
	/*vbus&wall*/
	printk("vbus_mv:%dmv\n", charger->vbus_mv);
	printk("wall_mv:%dmv\n", charger->wall_mv);

	/*vbus path*/
	printk("vbus path on/off:%d\n", charger->get_vbus_path(charger));
	printk("g_vbus_is_otg:%d\n", charger->g_vbus_is_otg);
	/*vbus control mode*/
	switch(charger->vbus_control_mode)
	{
		case CURRENT_LIMITED:printk("vbus_control_mode:CURRENT_LIMITED\n");break;
		case VOLTAGE_LIMITED:printk("vbus_control_mode:VOLTAGE_LIMITED\n");break;
		default:printk("vbus_control_mode:CANCEL_LIMITED\n");break;
	}
	/*fast charge*/
	printk("cv_enabled:%d\n", charger->cv_enabled);
	/*charger onoff */
	printk("charge_on:%d\n", charger->charge_on);
	/* battery */
	printk("bat_is_exist:%d\n", charger->bat_is_exist);
	printk("bat_mv:%dmv\n", charger->bat_mv);
	printk("bat_ma:%dmA\n", charger->bat_ma);
	printk("cur_bat_cap:%d%%\n", charger->cur_bat_cap);
	printk("bat_temp:%dC\n", charger->bat_temp);
	if (charger->health == POWER_SUPPLY_HEALTH_GOOD)
		printk("health:GOOD\n");
	else
		printk("health:BAD\n");
	printk("tricle_current:%dmA\n", charger->tricle_current);
	printk("constant charge current:%dmA\n",get_chg_current_now());
	/*backlight*/
	if (charger->cur_bl_status == BL_ON)
		printk("cur_bl_status: ON\n");
	else if (charger->cur_bl_status == BL_OFF)
		printk("cur_bl_status: OFF\n");

	printk("led_status:%d\n", charger->led_status);
	printk("\n=============================================================\n");
	return 0;	
}

static ssize_t store_dump(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return 0;	
}

static ssize_t show_bl_on_voltage(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct atc260x_charger *charger = container_of(psy, struct atc260x_charger, psy);
	return snprintf(buf, PAGE_SIZE, "%d\n", charger->cfg_items.bl_on_voltage * 1000);	
}

static ssize_t store_bl_on_voltage(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return 0;	
	
}

static struct device_attribute atc260x_power_attrs[] = {
	__ATTR(boot_cap_threshold, S_IRUGO | S_IWUSR, show_boot_cap_threshold, store_boot_cap_threshold),
	__ATTR(dump, S_IRUGO | S_IWUSR, show_dump, store_dump),
	 __ATTR(charger_online, S_IRUGO | S_IWUSR, show_charger_online, store_charger_online),
	__ATTR(bl_on_voltage, S_IRUGO | S_IWUSR, show_bl_on_voltage, store_bl_on_voltage),
};

int atc260x_power_create_sysfs(struct atc260x_charger *charger)
{
	int r, t;
	
	power_info("create sysfs for atc260x power\n");
	
	for (t = 0; t < ARRAY_SIZE(atc260x_power_attrs); t++) {
		r = device_create_file(charger->psy.dev, &atc260x_power_attrs[t]);
	
		if (r) {
			dev_err(charger->psy.dev, "failed to create sysfs file for atc260x power\n");
			return r;
		}
	}
	
	return 0;
}
void atc260x_power_remove_sysfs(struct atc260x_charger *charger)
{
	int  t;

	power_info("[%s]remove sysfs for atc260x charger\n", __func__);
	for (t = 0; t < ARRAY_SIZE(atc260x_power_attrs); t++) {
		device_remove_file(charger->psy.dev, &atc260x_power_attrs[t]);
	}
}

static int atc260x_get_cfg_item(struct atc260x_charger *charger)
{
	const __be32 *property;
	enum of_gpio_flags flags;
	int len;
	int ret = 0;
	int cur_ratio;
	
	/* adjust current depend on charger version.*/
	int chg_version = 0;
	int bl_on_current_usb_pc=0;
	int bl_off_current_usb_pc=0;
	int bl_on_current_usb_adp=0;
	int bl_off_current_usb_adp=0;
	int bl_on_current_wall_adp=0;
	int bl_off_current_wall_adp=0;
	
	/*added by cxj @2014/8/17:start here*/
	chg_version = atc260x_get_version(charger);
	BUG_ON(chg_version<0);
	if (get_pmu_ic_type(charger) == ATC260X_ICTYPE_2603A)
	{
	    if (chg_version == ATC260X_ICVER_A || 
		    chg_version == ATC260X_ICVER_B || 
		    chg_version == ATC260X_ICVER_C) 
		    cur_ratio = PMU_VERSION_ABC_CC_RATIO;
	    else
		    cur_ratio = PMU_VERSION_D_CC_RATIO;
	}
	else
	{
	    cur_ratio = PMU_VERSION_ABC_CC_RATIO;
	}
	/*end here */
	
	// step 1: get max charger current from dts
	property = of_get_property(charger->node, "bl_on_usb_pc_max_charge_current", &len);
	if (property && len == sizeof(int))
	{
		bl_on_current_usb_pc = (be32_to_cpup(property)) / 100;
		if(bl_on_current_usb_pc < 0 ||bl_on_current_usb_pc*cur_ratio > 4 * 15)
		{
			pr_warn("bl_on_current_usb_pc =%d \n", bl_on_current_usb_pc);
			bl_on_current_usb_pc = 3;
		}
	}
	else
	{
	     bl_on_current_usb_pc = 3;
	}

	property = of_get_property(charger->node, "bl_off_usb_pc_max_charge_current", &len);
	if (property && len == sizeof(int))
	{
	    bl_off_current_usb_pc = (be32_to_cpup(property))  / 100;
	    if(bl_off_current_usb_pc < 0 ||bl_off_current_usb_pc*cur_ratio > 4 * 15)
	    {
	         pr_warn("bl_off_current_usb_pc =%d \n", bl_off_current_usb_pc);
	         bl_off_current_usb_pc = 5;
	    }
	}
	else
	{
	     bl_on_current_usb_pc = 5;
	}

	property = of_get_property(charger->node, "bl_on_usb_adp_max_charge_current", &len);
	if (property && len == sizeof(int))
	{
	    bl_on_current_usb_adp = (be32_to_cpup(property))  / 100;
	    if(bl_on_current_usb_adp < 0 ||bl_on_current_usb_adp*cur_ratio > 4*15)
	    {
	         pr_warn("bl_on_current_usb_adp =%d \n", bl_on_current_usb_adp);
	         bl_on_current_usb_adp = 3;
	    }
	}
	else
	{
	     bl_on_current_usb_adp = 3;
	}

	property = of_get_property(charger->node, "bl_off_usb_adp_max_charge_current", &len);
	if (property && len == sizeof(int))
	{
	    bl_off_current_usb_adp = (be32_to_cpup(property))  / 100;
	    if(bl_off_current_usb_adp < 0 ||bl_off_current_usb_adp*cur_ratio > 4*15)
	    {
	         pr_warn("bl_off_current_usb_adp =%d \n", bl_off_current_usb_adp);
	         bl_off_current_usb_adp = 8;
	    }
	}
	else
	{
	     bl_off_current_usb_adp = 8;
	}

	property = of_get_property(charger->node, "bl_on_wall_adp_max_charge_current", &len);
	if (property && len == sizeof(int))
	{
	bl_on_current_wall_adp = (be32_to_cpup(property))  / 100;
	if(bl_on_current_wall_adp < 0 ||bl_on_current_wall_adp*cur_ratio > 4 * 15)
	{
		 pr_warn("bl_on_current_wall_adp =%d \n", bl_on_current_wall_adp);
		 bl_on_current_wall_adp = 3;
	}
	}
	else
	{
	 bl_on_current_wall_adp = 3;
	}

	property = of_get_property(charger->node, "bl_off_wall_adp_max_charge_current", &len);
	if (property && len == sizeof(int))
	{
	bl_off_current_wall_adp = (be32_to_cpup(property))  / 100;
	if(bl_off_current_wall_adp < 0 ||bl_off_current_wall_adp*cur_ratio > 4 * 15)
	{
		 pr_warn("bl_off_current_wall_adp =%d \n", bl_off_current_wall_adp);
		 bl_off_current_wall_adp = 15;
	}
	}
	else
	{
		bl_off_current_wall_adp = 15;
	}

	charger->cfg_items.bl_on_current_usb_pc = bl_on_current_usb_pc * cur_ratio / 4;
	charger->cfg_items.bl_off_current_usb_pc = bl_off_current_usb_pc * cur_ratio / 4;
	charger->cfg_items.bl_on_current_usb_adp = bl_on_current_usb_adp * cur_ratio / 4;
	charger->cfg_items.bl_off_current_usb_adp = bl_off_current_usb_adp * cur_ratio / 4;
	charger->cfg_items.bl_on_current_wall_adp = bl_on_current_wall_adp * cur_ratio / 4;
	charger->cfg_items.bl_off_current_wall_adp = bl_off_current_wall_adp * cur_ratio / 4;
	power_dbg("bl_on_current_usb_pc: %d \n", charger->cfg_items.bl_on_current_usb_pc);  
	power_dbg("bl_off_current_usb_pc: %d \n", charger->cfg_items.bl_off_current_usb_pc);    
	power_dbg("bl_on_current_usb_adp: %d \n",charger->cfg_items.bl_on_current_usb_adp); 
	power_dbg("bl_off_current_usb_adp: %d \n", charger->cfg_items.bl_off_current_usb_adp);  
	power_dbg("bl_on_current_wall_adp: %d \n", charger->cfg_items.bl_on_current_wall_adp);  
	power_dbg("bl_off_current_wall_adp: %d \n", charger->cfg_items.bl_off_current_wall_adp);    


	property = of_get_property(charger->node, "bl_on_voltage", &len);
	if (property && len == sizeof(int))
	{
		charger->cfg_items.bl_on_voltage = (be32_to_cpup(property));
	}
	else
	{
		charger->cfg_items.bl_on_voltage = 3300;
	}

	property = of_get_property(charger->node, "bl_on_voltage_diff", &len);
	if (property && len == sizeof(int))
	{
		charger->cfg_items.backlight_on_vol_diff = (be32_to_cpup(property));
		power_dbg("bl_on_voltage_diff: %d \n", charger->cfg_items.backlight_on_vol_diff); 
	}
	else
	{
		charger->cfg_items.backlight_on_vol_diff = 350;
	}

	property = of_get_property(charger->node, "bl_off_voltage_diff", &len);
	if (property && len == sizeof(int))
	{
		charger->cfg_items.backlight_off_vol_diff = (be32_to_cpup(property));
		power_dbg("bl_off_voltage_diff: %d \n", charger->cfg_items.backlight_off_vol_diff); 
	}
	else
	{
		charger->cfg_items.backlight_off_vol_diff = 400;
	}
	
	// adjust voltag diff depend on charger version.
	if (chg_version == ATC260X_ICVER_D) 
	{
		charger->cfg_items.backlight_off_vol_diff += 100;
	}

	property = of_get_property(charger->node, "ext_dcdc_exist", &len);
	if (property && len == sizeof(int))
	{
		charger->cfg_items.ext_dcdc_exist = (be32_to_cpup(property));
		power_dbg("ext_dcdc_exist: %d \n", charger->cfg_items.ext_dcdc_exist); 
	}
	else
	{
		charger->cfg_items.ext_dcdc_exist = 1;
	}
	if (charger->cfg_items.ext_dcdc_exist == 1) {
		charger->wall_v_thresh = 3400;
	} 
	else 
	{
		charger->wall_v_thresh = 4300;   /*modified by cxj @2015-01-16:4200-->4300*/
	}
    
	property = of_get_property(charger->node, "enable_vbus_current_limited", &len);
	if (property && len == sizeof(int))
	{
		charger->cfg_items.enable_vbus_current_limited = (be32_to_cpup(property));
		power_dbg("enable_vbus_current_limited: %d \n", charger->cfg_items.enable_vbus_current_limited); 
	}
	else
	{
		charger->cfg_items.enable_vbus_current_limited = 1;
	}
	
	property = of_get_property(charger->node, "usb_adapter_as_ac", &len);
	if (property && len == sizeof(int))
	{
		charger->cfg_items.usb_adapter_as_ac = (be32_to_cpup(property));
		power_dbg("usb_adapter_as_ac: %d \n", charger->cfg_items.usb_adapter_as_ac); 
	}
	else
	{
		charger->cfg_items.usb_adapter_as_ac = 1;
	}



	// get support adaptor type.
	property = of_get_property(charger->node, "support_adaptor_type", &len);
	if (property && len == sizeof(int))
	{
		charger->cfg_items.support_adaptor_type = (be32_to_cpup(property));
	}
	else
	{
		charger->cfg_items.support_adaptor_type = 3;
	}
	power_dbg("[power]support adaptor type: %d\n", charger->cfg_items.support_adaptor_type);
    
	//get low capacity boot threshold.don't boot to android ,if lower than  this value and usb plugged  
	property = of_get_property(charger->node, "boot_cap_threshold", &len);
	if (property && len == sizeof(int))
	{
		charger->cfg_items.boot_cap_threshold = (be32_to_cpup(property));
	}
	else
	{
		charger->cfg_items.boot_cap_threshold = 7;
	}
	power_dbg("[power]boot_cap_threshold: %d\n", charger->cfg_items.boot_cap_threshold);

	property = of_get_property(charger->node, "change_current_temp", &len);
	if (property && len == sizeof(int))
	{
		charger->cfg_items.change_current_temp = (be32_to_cpup(property));
	}
	else
	{
		charger->cfg_items.change_current_temp = 1;
	}
	power_dbg("[power]change_current_temp: %d\n", charger->cfg_items.change_current_temp);

	property = of_get_property(charger->node, "ot_shutoff_enable", &len);
	if (property && len == sizeof(int))
	{
		charger->cfg_items.ot_shutoff_enable = (be32_to_cpup(property));
	}
	else
	{
		charger->cfg_items.ot_shutoff_enable = 1;
	}
	power_dbg("[power]ot_shutoff_enable: %d\n", charger->cfg_items.ot_shutoff_enable);

	/*get gpio config for charge led*/
	charger->cfg_items.gpio_led_inner_pin = of_get_named_gpio_flags(charger->node, GPIO_NAME_LED_INNER_CHARGER, 0, &flags);
	if (charger->cfg_items.gpio_led_inner_pin >= 0)
	{
		charger->cfg_items.gpio_led_active_low = flags & OF_GPIO_ACTIVE_LOW;
		if (gpio_request(charger->cfg_items.gpio_led_inner_pin, GPIO_NAME_LED_INNER_CHARGER) < 0) 
		{
			power_err("[ATC260X] gpio_led_inner_pin =%d request failed!\n", charger->cfg_items.gpio_led_inner_pin);
		}
		else
		{
			charger->cfg_items.charger_led_exist = true;
			gpio_direction_output(charger->cfg_items.gpio_led_inner_pin, !charger->cfg_items.gpio_led_active_low);

		}
	}
	/* get gpio config for external charge control */
	charger->cfg_items.gpio_ext_chg_ctrl_pin = of_get_named_gpio_flags(charger->node, GPIO_NAME_EXT_CTRL_CHARGER, 0, &flags);
	if (charger->cfg_items.gpio_ext_chg_ctrl_pin >= 0)
	{
		charger->cfg_items.gpio_ext_chg_ctrl_active_low = flags & OF_GPIO_ACTIVE_LOW;
		if (gpio_request(charger->cfg_items.gpio_ext_chg_ctrl_pin, GPIO_NAME_EXT_CTRL_CHARGER) < 0) 
		{
			power_err("[ATC260X] gpio_ext_chg_ctrl_pin =%d request failed!\n", charger->cfg_items.gpio_ext_chg_ctrl_pin);
		}
		else
		{
			charger->cfg_items.ext_charger_exist = true;
			gpio_direction_output(charger->cfg_items.gpio_ext_chg_ctrl_pin, !charger->cfg_items.gpio_ext_chg_ctrl_active_low);

		}
	}
	
	return ret;
	
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void atc260x_power_early_suspend(struct early_suspend *handler)
{
	power_dbg("%s, %d, enter early_suspend\n", __FUNCTION__, __LINE__);
}

static void atc260x_power_later_resume(struct early_suspend *handler)
{
	struct atc260x_charger *charger = container_of(handler, struct atc260x_charger, early_suspend);
	wake_lock_timeout(&charger->delay_lock, 5 * HZ);
	power_dbg("%s, %d, enter later_resume\n", __FUNCTION__, __LINE__);
	cancel_delayed_work_sync(&charger->work);
	enter_later_resume = true;
	charger->set_constant_current(charger, charger->cfg_items.bl_on_current_usb_pc);
	/*mod_timer(&charger->adjust_current_timer, jiffies + msecs_to_jiffies(3000));*/
	queue_delayed_work(charger->charger_wq, &charger->work,msecs_to_jiffies(3000));
	
}
#endif  //CONFIG_HAS_EARLYSUSPEND


static  int atc260x_power_probe(struct platform_device *pdev)
{
	struct atc260x_dev *atc260x = dev_get_drvdata(pdev->dev.parent);
	struct atc260x_power *power;
	struct atc260x_charger *charger;    
	struct power_supply *usb;
	struct power_supply *wall;
	int ret;

	power = kzalloc(sizeof(struct atc260x_power), GFP_KERNEL);
	if (power == NULL)
		return -ENOMEM;

	global_power_dev = power;
	power->atc260x = atc260x;
	
	usb = &power->usb;
	wall = &power->wall;
	charger = &power->charger;
	charger->atc260x = atc260x;
	charger->node = pdev->dev.of_node;
	mutex_init(&charger->lock);
	init_completion(&charger->check_complete);
	//init_completion(&charger->check_usb_complete);
	//init_completion(&charger->check_wall_complete);

#ifdef CONFIG_HAS_EARLYSUSPEND
	charger->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	charger->early_suspend.suspend = atc260x_power_early_suspend;
	charger->early_suspend.resume = atc260x_power_later_resume;
	register_early_suspend(&charger->early_suspend);
#endif
	
	platform_set_drvdata(pdev, power);
	/* monitor interval is 2 seconds */
	charger->interval = 2;

	charger->debug_file = atc260x_bat_create_debugfs(charger);
	
	/*modified by cxj @20141009 */
	if (get_pmu_ic_type(charger) == ATC260X_ICTYPE_2603A)
	{
		atc2603a_get_info(charger);
	}
	else if (get_pmu_ic_type(charger) == ATC260X_ICTYPE_2603C)
	{
		atc2603c_get_info(charger);
	}
	else
	{
		//atc260x_get_2609A_info(charger);
		pr_err("we do not support the ATC2603C\n");
	}

	/*init battery power supply*/
	charger->psy.name = "battery";
	charger->psy.use_for_apm = 1;
	charger->psy.type = POWER_SUPPLY_TYPE_BATTERY;
	charger->psy.properties = atc260x_bat_props;
	charger->psy.num_properties = ARRAY_SIZE(atc260x_bat_props);        
	charger->psy.get_property = atc260x_bat_get_props;
	ret = power_supply_register(&pdev->dev, &charger->psy);
	power_dbg("%s:power_supply_register for bat success\n", __func__);
	if (ret)
		goto err_kmalloc;
	/*init wall power supply*/
	wall->name = PSY_NAME_WALL;
	wall->type = POWER_SUPPLY_TYPE_MAINS;
	wall->properties = atc260x_wall_props;
	wall->num_properties = ARRAY_SIZE(atc260x_wall_props);
	wall->get_property = atc260x_wall_get_prop;
	ret = power_supply_register(&pdev->dev, wall);
	if (ret)
		goto err_battery;
	/*init usb power supply*/
	usb->name = PSY_NAME_USB,
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->properties = atc260x_usb_props;
	usb->num_properties = ARRAY_SIZE(atc260x_usb_props);
	usb->get_property = atc260x_usb_get_prop;
	ret = power_supply_register(&pdev->dev, usb);
	if (ret)
		goto err_wall;         

	first_power_on = true;
	
	/*tricle value init 200mA*/
	charger->tricle_current = 200; 

	ret = atc260x_power_create_sysfs(charger);
	if (ret)
		goto err_usb;
	
	if (atc260x_get_cfg_item(charger) != 0)
		goto err_create_sysfs;

	if (charger->cfg_items.ext_dcdc_exist)
	{
		charger->pwm = pwm_get(&pdev->dev, NULL);
		if (IS_ERR(charger->pwm)) 
		{
			pr_err("%s, unable to request PWM!\n", __FUNCTION__);
			goto err_usb;
		}

		if (0 == charger->pwm->period) 
		{
			pr_err("%s, ext_dcdc_pwm->period is zero!\n", __FUNCTION__);
			goto err_usb;
		}
	}

	atc260x_charger_init(charger);
	
//	atc260x_check_bat_online(charger);
//	charger->charger_cur_status = atc260x_charger_check_online(charger);
	

	/*check if battery is healthy*/
	charger->battery_status = charger->check_health_pre(charger->atc260x);

	pr_info("charger=%p %d\n", charger, __LINE__);
	pr_info("set_vbus_path=%p set_apds_vbus_pd=%p\n", charger->set_vbus_path, 
	     charger->set_apds_vbus_pd);
	
	set_judge_adapter_type_handle((void *)distinguish_adapter_type);
	
	/*init wake_lock*/     
	wake_lock_init(&charger->charger_wake_lock, WAKE_LOCK_SUSPEND, "charger_lock");        
	wake_lock_init(&charger->delay_lock, WAKE_LOCK_SUSPEND, "delay_lock");     

	/*Create a work queue for the clmt*/
	charger->charger_wq =
		create_singlethread_workqueue("atc260x_charger_wq");
	if (charger->charger_wq == NULL) 
	{
		power_dbg("[%s]:failed to create work queue\n", __func__);
		goto err_destroy_wakelock;
	}
	INIT_DELAYED_WORK(&charger->work, atc260x_charging_monitor);
	queue_delayed_work(charger->charger_wq, &charger->work, 0 * HZ);

	return 0;
	
err_destroy_wakelock:
	wake_lock_destroy(&charger->charger_wake_lock);
	wake_lock_destroy(&charger->delay_lock);
err_create_sysfs:
	atc260x_power_remove_sysfs(charger);
err_usb:
	 power_supply_unregister(usb);
err_wall:
	//ywwang: todo:  free gpio already requested    
	power_supply_unregister(wall);
err_battery:
	power_supply_unregister(&charger->psy); 
err_kmalloc:
	kfree(power);
	platform_set_drvdata(pdev, NULL);
	global_power_dev=0;
	return ret;
}

static  int atc260x_power_remove(struct platform_device *pdev)
{
	struct atc260x_power *atc260x_power = platform_get_drvdata(pdev);

	if(!atc260x_power)
	{
		pr_warn("strange  atc260x_power is null ,probe maybe failed\n");
		return 0;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&atc260x_power->charger.early_suspend);
#endif

	cancel_delayed_work_sync(&atc260x_power->charger.work);
	atc260x_bat_remove_debugfs(&atc260x_power->charger);

	power_supply_unregister(&atc260x_power->charger.psy);
	power_supply_unregister(&atc260x_power->wall);
	power_supply_unregister(&atc260x_power->usb);

	kfree(atc260x_power);

	return 0;
}

/*record the time stamp of suspend*/
static struct timespec suspend_ts;      
static int atc260x_power_suspend(struct platform_device *pdev, pm_message_t m)
{
	struct atc260x_power *atc260x_power = platform_get_drvdata(pdev);
	struct atc260x_charger *charger = &atc260x_power->charger;

	dev_info(&pdev->dev, "atc260x_power_suspend()\n");
	pr_info("%s,cur_bat_cap:%d, bat_mv:%d\n", __func__, charger->cur_bat_cap, charger->bat_mv);
	cancel_delayed_work_sync(&atc260x_power->charger.work); 

	if (charger->cfg_items.ext_dcdc_exist)
		pwm_config(charger->pwm,
				((charger->pwm->period) >> 6) * PWM_MAX_LEVEL,
					charger->pwm->period);
	if (charger->cfg_items.charger_led_exist) 
	{
		gpio_free(charger->cfg_items.gpio_led_inner_pin);	
	}

	if (charger->cfg_items.ext_charger_exist) 
	{
		gpio_free(charger->cfg_items.gpio_ext_chg_ctrl_pin);	
	}
	log_event_none(LOG_HEADER_SUSPEND);
	getnstimeofday(&suspend_ts);

	return 0;
}

 /*during supend, consumption by  1% for every 10800 seconds*/
#define SUSPEND_SECONDS_1_PERCRNT   10800  
static int atc260x_power_resume(struct platform_device *pdev)
{
	struct timespec resume_ts;
	struct timespec sleeping_ts;
	unsigned int cap_percent = 0;
	struct atc260x_power *atc260x_power = platform_get_drvdata(pdev);
	struct atc260x_charger *charger = &atc260x_power->charger;
	
	wake_lock_timeout(&charger->delay_lock, 5 * HZ);/*added by cxj@2014/11/14*/
	
	dev_info(&pdev->dev, "atc260x_power_resume()\n");
	pr_info("%s,cur_bat_cap:%d, bat_mv:%d\n", __func__, charger->cur_bat_cap, charger->bat_mv);
	log_event_none(LOG_HEADER_RESUME);
	
	if (charger->cfg_items.charger_led_exist) 
	{
		gpio_request(charger->cfg_items.gpio_led_inner_pin, GPIO_NAME_LED_INNER_CHARGER);
		gpio_direction_output(charger->cfg_items.gpio_led_inner_pin, 0);
	}

	if (charger->cfg_items.ext_charger_exist) 
	{
		gpio_request(charger->cfg_items.gpio_ext_chg_ctrl_pin, GPIO_NAME_EXT_CTRL_CHARGER);
		gpio_direction_output(charger->cfg_items.gpio_ext_chg_ctrl_pin, 1);
	}

	//atc260x_charger_init(charger);
	atc260x_dcdc_init(charger);
	getnstimeofday(&resume_ts);
	sleeping_ts = timespec_sub(resume_ts, suspend_ts);
	cap_percent = (sleeping_ts.tv_sec / SUSPEND_SECONDS_1_PERCRNT);


	if (charger->cur_bat_cap <= cap_percent) 
	{
		charger->cur_bat_cap = 0;
		power_supply_changed(&charger->psy);
	} 
	else 
	{
		charger->cur_bat_cap -= cap_percent;
	}
	
	/*schedule_delayed_work(&charger->work, msecs_to_jiffies(0));*/
	queue_delayed_work(charger->charger_wq, &charger->work, msecs_to_jiffies(0));
	return 0;
}

static void atc260x_power_shutdown(struct platform_device *pdev)
{
	struct atc260x_power *atc260x_power = platform_get_drvdata(pdev);
	struct atc260x_charger *charger = &atc260x_power->charger;
	cancel_delayed_work_sync(&charger->work); 

	if (charger->cfg_items.charger_led_exist) 
	{
		gpio_free(charger->cfg_items.gpio_led_inner_pin);	
	}

	if (charger->cfg_items.ext_charger_exist) 
	{
		gpio_free(charger->cfg_items.gpio_ext_chg_ctrl_pin);	
	}

	if (charger->cfg_items.ext_dcdc_exist == 1) 
	{
		pwm_disable(charger->pwm);
		pwm_free(charger->pwm);
		charger->pwm = NULL;
	}
	if (charger->atc260x == NULL)
		return ;

	dev_info(&pdev->dev, "atc260x_power_shutdown()\n");
	pr_info("%s,cur_bat_cap:%d, bat_mv:%d\n", __func__, charger->cur_bat_cap, charger->bat_mv);
	
	if (charger->set_apds_wall_pd)
		charger->set_apds_wall_pd(charger, true);
	
	return;
}
/* added by cxj @20141009 */
static const struct of_device_id atc260x_power_match[] = {
	{ .compatible = "actions,atc2603a-power", },
	{ .compatible = "actions,atc2603c-power", },
	{ .compatible = "actions,atc2609a-power", },
	{},
};
MODULE_DEVICE_TABLE(of, atc260x_power_match);

static struct platform_driver atc260x_power_driver = 
{
	.probe      = atc260x_power_probe,
	.remove     = atc260x_power_remove,
	.driver     = 
	{
	    .name = "atc260x-power",
		.of_match_table = of_match_ptr(atc260x_power_match), /* by cxj */
	},
	.suspend    = atc260x_power_suspend,
	.resume     = atc260x_power_resume,
	.shutdown = atc260x_power_shutdown,
};

static int __init atc260x_power_init(void)
{
	//pr_info(" %s version: %s\n", THIS_MODULE->name, THIS_MODULE->version);
	log_event_init();
	return platform_driver_register(&atc260x_power_driver);
}
late_initcall(atc260x_power_init);

static void __exit atc260x_power_exit(void)
{
	wake_lock_destroy(&global_power_dev->charger.charger_wake_lock);
	wake_lock_destroy(&global_power_dev->charger.delay_lock);
	platform_driver_unregister(&atc260x_power_driver);    
}
module_exit(atc260x_power_exit);

MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("atc260x charger driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc260x-power");
MODULE_VERSION("Actions-v2-20140829102330");
