#ifndef _GL520X_WIFI_PLAT_H_
#define _GL520X_WIFI_PLAT_H_
/*
 * Wi-Fi platform device
 */

#define WIFI_TYPE_BCMDHD 0x01
#define WIFI_TYPE_RTK    0x02

struct wlan_plat_data {
	int (*set_power)(struct wlan_plat_data *pdata, int on);
	int (*set_carddetect)(int open);
	int (*set_init)(struct wlan_plat_data *pdata);
	void (*set_exit)(struct wlan_plat_data *pdata);

	struct platform_device *parent;
	int wl_bt_ref_count;	/* power reference count */

	u32 wifi_bt_power_gpios; 	/* gpio for (WiFi & BT) power control */
	u32 wifi_en_gpios;			/* wifi ENABLE,PD(power down),DISABLE,RST */
};

#endif /* end of _GL520X_WIFI_PLAT_H_ */
