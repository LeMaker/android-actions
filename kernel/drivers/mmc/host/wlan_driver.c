#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <linux/mfd/atc260x/atc260x.h>

#include "wlan_plat_data.h"

int g_wifi_type = 0;
static void (*wifi_hook)(void) = NULL;
static struct completion wlan_complete;

/* Wi-Fi platform driver */
static int acts_wlan_init(struct wlan_plat_data *pdata)
{
	int ret;

	if (pdata && pdata->set_init) {
		ret = pdata->set_init(pdata);
		if (ret < 0)
			pr_err("sdio wifi: init sdio wifi failed\n");
		return ret;
	}

	return 0;
}

int acts_wlan_set_power(struct wlan_plat_data *pdata, int on,
	unsigned long msec)
{
	if (pdata && pdata->set_power)
		pdata->set_power(pdata, on);

	if (msec)
		mdelay(msec);
	return 0;
}
EXPORT_SYMBOL(acts_wlan_set_power);

static int acts_wlan_carddetect(int on, struct wlan_plat_data *pdata)
{
	if (pdata && pdata->set_carddetect)
		pdata->set_carddetect(on);

	return 0;
}

static void acts_wlan_exit(struct wlan_plat_data *pdata)
{
	if (pdata && pdata->set_exit)
		pdata->set_exit(pdata);
}

static int wlan_probe(struct platform_device *pdev)
{
	struct wlan_plat_data *pdata =
		(struct wlan_plat_data *)(pdev->dev.platform_data);

	if (acts_wlan_init(pdata)) {
		pr_err("sdio wifi device probe failed\n");
		return -ENXIO;
	}

	config_inner_charger_current(DEV_CHARGER_PRE_CONFIG,
		DEV_CHARGER_CURRENT_WIFI, 1);
	acts_wlan_set_power(pdata, 1, 0);
	config_inner_charger_current(DEV_CHARGER_POST_CONFIG,
		DEV_CHARGER_CURRENT_WIFI, 1);

	if(g_wifi_type != WIFI_TYPE_BCMDHD) {
		printk("wlan card detection to detect SDIO card!");
		acts_wlan_carddetect(1, pdata);
	}

	complete(&wlan_complete);
	return 0;
}

static int wlan_remove(struct platform_device *pdev)
{
	struct wlan_plat_data *pdata =
		(struct wlan_plat_data *)(pdev->dev.platform_data);


	config_inner_charger_current(DEV_CHARGER_PRE_CONFIG,
		DEV_CHARGER_CURRENT_WIFI, 0);
	acts_wlan_set_power(pdata, 0, 0);
	config_inner_charger_current(DEV_CHARGER_POST_CONFIG,
		DEV_CHARGER_CURRENT_WIFI, 0);

	if(g_wifi_type != WIFI_TYPE_BCMDHD) {
		printk("wlan card detection to remove SDIO card!");
		acts_wlan_carddetect(0, pdata);
	}

	acts_wlan_exit(pdata);
	complete(&wlan_complete);
	return 0;
}


static void wlan_shutdown(struct platform_device * pdev)
{
	struct wlan_plat_data *pdata = (struct wlan_plat_data *)(pdev->dev.platform_data);

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	if(wifi_hook != NULL)
		wifi_hook();

	config_inner_charger_current(DEV_CHARGER_PRE_CONFIG,
		DEV_CHARGER_CURRENT_WIFI, 0);
	acts_wlan_set_power(pdata, 0, 0);
	config_inner_charger_current(DEV_CHARGER_POST_CONFIG,
		DEV_CHARGER_CURRENT_WIFI, 0);

	if(g_wifi_type != WIFI_TYPE_BCMDHD) {
		printk("wlan card detection to remove SDIO card!");
		acts_wlan_carddetect(0, pdata);
	}

	complete(&wlan_complete);
	return ;
}

static int wlan_suspend(struct platform_device *pdev, pm_message_t state)
{
	pr_info("##> %s\n", __func__);
	return 0;
}
static int wlan_resume(struct platform_device *pdev)
{
	pr_info("##> %s\n", __func__);
	return 0;
}

static struct platform_driver wlan_driver = {
	.probe		= wlan_probe,
	.remove		= wlan_remove,
	.shutdown   = wlan_shutdown,
	.suspend	= wlan_suspend,
	.resume		= wlan_resume,
	.driver		= {
		.name	= "gl520x_wlan",
	}
};

/* symbols exported to wifi driver
 *
 * Param :
 *  type : For wifi type, they define in "wlan_plat_data.h", please don't use '0'
 *     p : If in this module to callback wifi driver function, can call through it. Don't need can add value "NULL"
 */
int acts_wifi_init(int type, void *p)
{
	int ret;

	g_wifi_type = type;
	wifi_hook = p;

	init_completion(&wlan_complete);
	ret = platform_driver_register(&wlan_driver);
	if (!wait_for_completion_timeout(&wlan_complete,
			msecs_to_jiffies(3300))) {
		pr_err("%s: wifi driver register failed\n", __func__);
		goto fail;
	}

	return ret;
fail:
	platform_driver_unregister(&wlan_driver);
	return -1;
}
EXPORT_SYMBOL(acts_wifi_init);

void acts_wifi_cleanup(void)
{
	platform_driver_unregister(&wlan_driver);

	g_wifi_type = 0;
	wifi_hook = NULL;
}
EXPORT_SYMBOL(acts_wifi_cleanup);

