#ifndef _GL520X_WLAN_DEVICE_H_
#define _GL520X_WLAN_DEVICE_H_

/* sdio wifi detect */
extern void acts_wlan_status_check_register(struct gl520xmmc_host *host);
extern int acts_wlan_bt_power_init(struct wlan_plat_data *pdata);
extern void acts_wlan_bt_power_release(void);
extern struct platform_device acts_wlan_device;

#endif /* end of _GL520X_WLAN_DEVICE_H_ */
