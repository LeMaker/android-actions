#ifndef __ATC260X_POWER_H__
#define __ATC260X_POWER_H__

#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/completion.h>
#include <linux/earlysuspend.h>
#include <linux/timer.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>

/* If defined, enable printk. */
//#define DEBUG
#ifdef DEBUG
#define power_dbg(format, arg...)       \
	pr_info(format , ## arg)
#else
#define power_dbg(format, arg...)       \
	do {} while (0)
#endif

#define power_err(...)        printk(KERN_ERR "ATC260X_POWER: " __VA_ARGS__);
#define power_info(...)       printk(KERN_INFO "ATC260X_POWER: " __VA_ARGS__);

#define     ATC260X_VBUS_VOLTAGE_THRESHOLD      3900        /* 3.9v */
#define     ATC260X_BAT_DETECT_DELAY_MS         300         /* wait 300ms for bat detect over */

struct gpio_pre_cfg;

enum BL_STATUS {
	BL_ON = 0,
	BL_OFF
};

typedef enum  {
	NO_PLUGED = 0,
	WALL_PLUGED,
	USB_PLUGED
} charger_state_t;

enum USB_PLUGED_TYPE {
	USB_NO_PLUGED = 0,
	USB_PLUGED_PC,
	USB_PLUGED_ADP
};

/*
* There are :
* 1.two interface:wall plugged only or wall and usb plugged or usb plugged only;
* 2.single usb:usb plugged and wall plugged.
* 3.single dcin:wall plugged only.
*/
enum SUPPORT_ADAPTOR_TYPE {
	SUPPORT_DCIN = 0x1,
	SUPPORT_USB = 0x2,
	SUPPORT_DCIN_USB = 0x3
};

enum VBUS_CONTROL_MODE {
	CURRENT_LIMITED,
	VOLTAGE_LIMITED,
	CANCEL_LIMITED
};

//keep sync with   arch/arm/mach-liger/pm.c ADAPTER_TYPE_NO_PLUGIN
#define ADAPTER_TYPE_NO_PLUGIN             0
#define ADAPTER_TYPE_WALL_PLUGIN           1 
#define ADAPTER_TYPE_PC_USB_PLUGIN         2
#define ADAPTER_TYPE_USB_ADAPTER_PLUGIN    3
#define ADAPTER_TYPE_USB_WALL_PLUGIN       4

struct atc260x_battery_thresholds {
	int vbat_charge_start;
	int vbat_charge_stop;
	int vbat_low;
	int vbat_crit;
	int trickle_charge_current;
	int constant_charge_current;    
};

struct atc260x_cfg_items {
	unsigned int bl_on_current_usb_pc;
	unsigned int bl_off_current_usb_pc;
	unsigned int bl_on_current_usb_adp;
	unsigned int bl_off_current_usb_adp;
	unsigned int bl_on_current_wall_adp;
	unsigned int bl_off_current_wall_adp;

	unsigned int backlight_on_vol_diff;
	unsigned int backlight_off_vol_diff;
	/*external DCDC flag */
	unsigned int ext_dcdc_exist;                		
	unsigned int support_adaptor_type;
	unsigned int enable_vbus_current_limited;
	unsigned int usb_adapter_as_ac;
	int boot_cap_threshold;
	int bl_on_voltage;
	
	//struct gpio_pre_cfg charger_led_cfg;
	//struct gpio_pre_cfg ext_chg_ctrl_cfg;

	int change_current_temp;
	int ot_shutoff_enable;

	int gpio_led_inner_pin;
	int gpio_led_active_low;
	bool charger_led_exist;
	int gpio_ext_chg_ctrl_pin;
	int gpio_ext_chg_ctrl_active_low;
	bool ext_charger_exist;
};

struct atc260x_charger {
	const char *name;
	struct power_supply psy;
	struct atc260x_dev *atc260x;
	struct device_node *node;
	struct workqueue_struct *charger_wq;
	struct delayed_work work;
	struct pwm_device *pwm;
	struct timer_list adjust_current_timer;
	struct atc260x_battery_thresholds thresholds;
	unsigned int interval;
	
	struct mutex lock;
#ifdef CONFIG_HAS_EARLYSUSPEND
		struct early_suspend early_suspend;
#endif

	struct wake_lock charger_wake_lock;
	struct wake_lock delay_lock;
	 int wakelock_acquired;

	struct atc260x_cfg_items cfg_items;

	struct completion check_complete;

	/* battery status */
	bool bat_is_exist;	
	int chg_type;
	int bat_mv;
	int bat_ma;
	int bat_temp;
	int chg_ma;
	int health;
	int battery_status;
	int tricle_current;
	int pre_bat_cap;
	int cur_bat_cap; 
	int bat_counter;   /* jiffies from last update the real battery voltage */
	int info_counter;  /*jiffies from last updat the battery info */
	
	/*charger status*/
	bool extern_power_online;  
	bool charge_on;
	bool charge_force_off;/*added by cxj @2014-1-5*/
	bool cv_enabled;
	int extra_chg_count;
	int vbus_mv;
	int wall_mv;
	int wall_v_thresh;
	int g_vbus_is_otg;
	int vbus_control_mode;
	
	int full_vol;
	int charger_cur_status;		// currently detected charger status, such as usb or wall whether online;
	int charger_pre_status;		// saved charger status of previous detectation.

	int cur_bl_status;
	int pre_bl_status;
	//charger_state_t chg_mode;
	enum USB_PLUGED_TYPE usb_pluged_type;
	bool usb_pluged_type_changed; 
	/*0, off; 1, on*/
	int led_status; 

	/*for hard gauge */
	int (*get_hw_gauge_cap)(void);
	int (*get_hw_gauge_volt)(void);
	int (*get_hw_gauge_cur)(void);
	int (*get_hw_gauge_temp)(void);

	/*for soft gauge*/
	int (*get_gauge_cap)(void);
	int (*get_gauge_volt)(void);
	int (*get_gauge_cur)(void);
	int (*get_gauge_temp)(void);
	int (*adjust_tp_para)(int);

	int (*read_adc)(struct atc260x_dev *atc260x, const char *channel_name);/*cxj*/
	int (*check_bat_online)(struct atc260x_dev *atc260x);
	int (*check_wall_online)(struct atc260x_charger *charger,  
		union power_supply_propval *val);
	int (*check_vbus_online)(struct atc260x_charger *charger, 
		union power_supply_propval *val);
	void (*update_phrase_type)(struct atc260x_charger *charger);
	void (*set_charge)(struct atc260x_dev *atc260x, int on);
	void (*set_vbus_ctl_mode)(struct atc260x_charger *charger, int mode);
	void (*update_charger_mode)(struct atc260x_charger *charger);
	int (*check_health_pre)(struct atc260x_dev *atc260x);
	int (*check_health)(struct atc260x_charger *charger, int *health);
	void (*set_constant_current)(struct atc260x_charger *charger, int set_current);
	int (*get_constant_current)(struct atc260x_charger *charger);
	void (*cv_set)(struct atc260x_charger *charger, int cv_val_mv);
	void (*set_apds_wall_pd)(struct atc260x_charger *charger, bool enable);
	void (*set_vbus_path)(struct atc260x_charger * charger, bool enable);
	int (*get_vbus_path)(struct atc260x_charger * charger);
	void (*set_apds_vbus_pd)(struct atc260x_charger *charger, bool enable);
	void (*charger_phy_init)(struct atc260x_charger *charger);
	void (*dump_regs)(struct atc260x_charger *charger);

	struct dentry *debug_file;
};

struct atc260x_battery
{
	struct power_supply psy;	
	struct atc260x_dev *atc260x;

	/*battery status*/
	bool bat_is_exist;  
	int chg_type;
	int bat_mv;
	int bat_ma;
	int bat_temp;
	int chg_ma;
	int health;

	int full_vol;
         int battery_status;
	int boot_cap_threshold;
	int pre_bat_cap;
	int cur_bat_cap;

	int bat_counter;   /* jiffies from last update the real battery voltage */
	int info_counter;  /*jiffies from last updat the battery info */

	/* function pointer of getting capacity */
	int (*get_gauge_cap)(void);
	/* function pointer of getting voltage */
	int (*get_gauge_volt)(void);
	/* function pointer of getting current */
	int (*get_gauge_cur)(void);
	/* function pointer of getting temperature */
	int (*get_gauge_temp)(void);

	int (*check_bat_online)(struct atc260x_dev *atc260x);
	void (*update_phrase_type)(struct atc260x_charger *charger);
	int (*check_health_pre)(struct atc260x_dev *atc260x);
	int (*check_health)(struct atc260x_charger *charger, int *health);
};

struct atc260x_power {
	struct atc260x_dev *atc260x;
	struct power_supply wall;
	struct power_supply usb;
	struct atc260x_charger charger;
};

#ifdef STORE_POWER_INFO
struct pmu_ic_info_test {
	int bat_v;			/*battery voltage*/
	int bat_i; 			/*battery current*/
	int diff_v;			/*different voltage*/
	int wall_v;    		/*wall voltage*/
	int pwm_level;    	/*pwm level*/
};
#endif

void atc2603a_get_info(struct atc260x_charger *charger);
//void atc2603b_get_info(struct atc260x_charger *charger) ;
void atc2603c_get_info(struct atc260x_charger *charger);

#define LOG_HEADER_BATTERY "bat_update" 
#define LOG_HEADER_HOTPLUG "hotplug"
#define LOG_HEADER_AWAKE "awake_timeout"
#define LOG_HEADER_SUSPEND "suspend"
#define LOG_HEADER_RESUME "resume"
#define LOG_HEADER_LOCK "lock"
#define LOG_HEADER_UNLOCK "unlock"
#define LOG_HEADER_CHARGER_ON "charge_on"
#define LOG_HEADER_CHARGER_OFF "charge_off"

extern int log_event_none(const char *header);
extern int log_event_int(const char *header, int intval);
extern int log_event_int_int(const char *header, int intval, int newval);
extern int log_event_string(const char *header, const char *content);
extern void log_event_dump(void);
extern void log_event_init(void);
#endif
