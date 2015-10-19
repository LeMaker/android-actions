
#ifndef __AOTG_PLAT_DATA_H__
#define __AOTG_PLAT_DATA_H__

struct aotg_plat_data {
	void __iomem *usbecs;
	void __iomem *usbpll;
	u32 usbpll_bits;
	void __iomem *devrst;
	u32 devrst_bits;
	int no_hs;
};

int aotg0_device_init(int power_only);
void aotg0_device_exit(int power_only);

int aotg1_device_init(int power_only);
void aotg1_device_exit(int power_only);
extern void aotg_hub_unregister(int dev_id);
extern int aotg_hub_register(int dev_id);

#endif
