#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/mmc/host.h>

#include "wlan_plat_data.h"
#include "gl520x_mmc.h"

static void (*wifi_detect_func)(int card_present, void *dev_id);
static void *wifi_detect_param;


static void wlan_status_check(int card_present, void *dev_id)
{
	struct gl520xmmc_host *host = dev_id;

	pr_info("SDIO Wi-Fi check status, present (%d -> %d)\n",
		host->sdio_present, card_present);

	if (card_present == 0) {
		pr_info("MMC: emulate power off the SDIO card\n");
		module_reset(host->module_id);
	}

	host->sdio_present = card_present;
	mmc_detect_change(host->mmc, 0);
}

static int wlan_status_check_register(
	void (*callback)(int card_present, void *dev_id),
	void *dev_id)
{
	if (wifi_detect_func) {
		pr_err("wifi status check function has registered\n");
		return -EAGAIN;
	}
	wifi_detect_func = callback;
	wifi_detect_param = dev_id;
	return 0;
}

static struct wlan_plat_data *g_pdata;

struct wlan_plat_data *acts_get_wlan_plat_data(void)
{
	return g_pdata;
}
EXPORT_SYMBOL(acts_get_wlan_plat_data);

void acts_wlan_bt_power(int on)
{
	struct wlan_plat_data *pdata = g_pdata;

	if (gpio_is_valid(pdata->wifi_bt_power_gpios)) {
		if (on) {
			if (++pdata->wl_bt_ref_count == 1) {
				gpio_set_value(pdata->wifi_bt_power_gpios, 1);
				mdelay(20);
			}
		} else {
			if (--pdata->wl_bt_ref_count == 0)
				gpio_set_value(pdata->wifi_bt_power_gpios, 0);
		}

		pr_info("Wlan or BT power %s, ref count:%d\n",
			(on ? "on" : "off"), pdata->wl_bt_ref_count);
	}
}
EXPORT_SYMBOL(acts_wlan_bt_power);

static int wlan_init(struct wlan_plat_data *pdata)
{
	struct device_node *np = NULL;
	int ret;

	np = of_find_compatible_node(NULL, NULL, "wifi,bt,power,ctl");	
	if (NULL == np) {
		pr_err("No \"wifi,bt,power,ctl\" node found in dts\n");		
		goto fail;
	}

	/* wifi en */
	if (of_find_property(np, "wifi_en_gpios", NULL)) {
		pdata->wifi_en_gpios = of_get_named_gpio(np,
			"wifi_en_gpios", 0);
		if (gpio_is_valid(pdata->wifi_en_gpios)) {
			ret = gpio_request(pdata->wifi_en_gpios,
				"wifi_en_gpios");
			if (ret < 0) {
				pr_err("couldn't claim wifi_en_gpios pin\n");
				goto fail;
			}
			gpio_direction_output(pdata->wifi_en_gpios, 0);
		} else {
			pr_err("gpio for sdio wifi_en_gpios invalid.\n");
		}
	}

	return 0;

fail:
	return -ENXIO;
}

static int wlan_set_power(struct wlan_plat_data *pdata, int on)
{

	acts_wlan_bt_power(on);

	if (gpio_is_valid(pdata->wifi_en_gpios)) {
		if (on) {
			gpio_set_value(pdata->wifi_en_gpios, 1);
			mdelay(20);
		} else {
			gpio_set_value(pdata->wifi_en_gpios, 0);
		}
	}

	mdelay(20);

	return 0;
}

/*
 * Open or close wifi, from open
 */
static int wlan_card_detect(int open)
{
	if (wifi_detect_func)
		wifi_detect_func(open, wifi_detect_param);
	else
		pr_warn("SDIO Wi-Fi card detect error\n");
	return 0;
}

static void wlan_exit(struct wlan_plat_data *pdata)
{
	if (gpio_is_valid(pdata->wifi_en_gpios))
		gpio_free(pdata->wifi_en_gpios);
}

static struct wlan_plat_data wlan_control = {
	.set_init = wlan_init,
	.set_exit = wlan_exit,
	.set_power = wlan_set_power,
	.set_carddetect = wlan_card_detect,
};

static void wlan_platform_release(struct device *dev)
{
	return ;
}

struct platform_device acts_wlan_device = {
	.name	= "gl520x_wlan",
	.id	= 0,
	.dev	= {
		.release = wlan_platform_release,
		.platform_data = &wlan_control,
	},
};

int acts_wlan_bt_power_init(struct wlan_plat_data *pdata)
{
	struct device_node *np = NULL;
	int ret;

	g_pdata = pdata;

	np = of_find_compatible_node(NULL, NULL, "wifi,bt,power,ctl");	
	if (NULL == np) {
		pr_err("No \"wifi,bt,power,ctl\" node found in dts\n");		
		goto fail;
	}

	if (of_find_property(np, "wifi_bt_power_gpios", NULL)) {
		pdata->wifi_bt_power_gpios = of_get_named_gpio(np,
			"wifi_bt_power_gpios", 0);
		if (gpio_is_valid(pdata->wifi_bt_power_gpios)) {
			ret = gpio_request(pdata->wifi_bt_power_gpios,
				"wifi_bt_power_gpios");
			if (ret < 0) {
				pr_err("couldn't claim wifi power gpio pin\n");
				goto fail;
			}
			gpio_direction_output(pdata->wifi_bt_power_gpios, 0);
			pdata->wl_bt_ref_count = 0;
		} else {
			pr_err("gpio for sdio wifi power supply invalid.\n");
		}
	}
	return 0;

fail:
	return -ENXIO;
}

void acts_wlan_bt_power_release(void)
{
	struct wlan_plat_data *pdata = g_pdata;

	if (gpio_is_valid(pdata->wifi_bt_power_gpios))
		gpio_free(pdata->wifi_bt_power_gpios);
}

void acts_wlan_status_check_register(struct gl520xmmc_host *host)
{
	wlan_status_check_register(wlan_status_check, host);
}
