/*
 * Actions OWL SoCs dwc3 driver
 *
 * Copyright (c) 2015 Actions Semiconductor Co., Ltd.
 * tangshaoqing <tangshaoqing@actions-semi.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/usb/ch9.h>


int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	if (!strcmp(name, "usb_dnl_ums")) {
		dev->idVendor = __constant_cpu_to_le16(CONFIG_G_DNL_UMS_VENDOR_NUM);
		dev->idProduct = __constant_cpu_to_le16(CONFIG_G_DNL_UMS_PRODUCT_NUM);
	} else {
		dev->idVendor = __constant_cpu_to_le16(CONFIG_G_DNL_VENDOR_NUM);
		dev->idProduct = __constant_cpu_to_le16(CONFIG_G_DNL_PRODUCT_NUM);
	}

	return 0;
}

