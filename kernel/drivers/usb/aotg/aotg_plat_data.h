
#ifndef __AOTG_PLAT_DATA_H__
#define __AOTG_PLAT_DATA_H__

#include <linux/wakelock.h>

#define USB2_PLL_EN0           (1<<12)
#define USB2_PLL_EN1           (1<<13)
#define USB2_PHYCLK_EN0           (1<<10)
#define USB2_PHYCLK_EN1           (1<<11)
#define USB2_ECS_PLL_LDO_EN   (1<<7)

struct aotg_plat_data {
	void __iomem *usbecs;
	void __iomem *usbpll;
	u32 usbpll_bits;
	void __iomem *devrst;
	u32 devrst_bits;
	int no_hs;
};

struct aotg_uhost_mon_t {
	int id;
	void __iomem *usbecs;
	void __iomem *usbpll;

	struct timer_list hotplug_timer;

	struct workqueue_struct *aotg_dev_onoff;
	struct delayed_work aotg_dev_init;
	struct delayed_work aotg_dev_exit; 
	struct wake_lock aotg_wake_lock;

	unsigned int aotg_uhost_det;

	/* dp, dm state. */
	unsigned int old_state;
	unsigned int state;
};

enum aotg_mode_e {
	HCD_MODE,
	UDC_MODE,
};

int aotg0_device_init(int power_only);
void aotg0_device_exit(int power_only);

int aotg1_device_init(int power_only);
void aotg1_device_exit(int power_only);
extern void aotg_hub_unregister(int dev_id);
extern int aotg_hub_register(int dev_id);

#endif
