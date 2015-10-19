/*
 *  linux/drivers/mmc/core/bus.c
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *  Copyright (C) 2007 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  MMC card bus driver model
 */

#include <linux/export.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/pm_runtime.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <mach/bootdev.h>

#include "core.h"
#include "sdio_cis.h"
#include "bus.h"
#include "./../host/gl520x_mmc.h"

#define to_mmc_driver(d)	container_of(d, struct mmc_driver, drv)
/* wait 3 s to card init ok ,then goto tsd proble*/ 
#define WAIT_CARD_TIMEOUT 	300   

static ssize_t mmc_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mmc_card *card = mmc_dev_to_card(dev);

	switch (card->type) {
	case MMC_TYPE_MMC:
		return sprintf(buf, "MMC\n");
	case MMC_TYPE_SD:
		return sprintf(buf, "SD\n");
	case MMC_TYPE_SDIO:
		return sprintf(buf, "SDIO\n");
	case MMC_TYPE_SD_COMBO:
		return sprintf(buf, "SDcombo\n");
	default:
		return -EFAULT;
	}
}


static struct device_attribute mmc_dev_attrs[] = {
	__ATTR(type, S_IRUGO, mmc_type_show, NULL),
	__ATTR_NULL,
};

/*
 * This currently matches any MMC driver to any MMC card - drivers
 * themselves make the decision whether to drive this card in their
 * probe method.
 */
static int mmc_bus_match(struct device *dev, struct device_driver *drv)
{


	
	
	
	return 1;
}

static int
mmc_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct mmc_card *card = mmc_dev_to_card(dev);
	const char *type;
	int retval = 0;

	switch (card->type) {
	case MMC_TYPE_MMC:
		type = "MMC";
		break;
	case MMC_TYPE_SD:
		type = "SD";
		break;
	case MMC_TYPE_SDIO:
		type = "SDIO";
		break;
	case MMC_TYPE_SD_COMBO:
		type = "SDcombo";
		break;
	default:
		type = NULL;
	}

	if (type) {
		retval = add_uevent_var(env, "MMC_TYPE=%s", type);
		if (retval)
			return retval;
	}

	retval = add_uevent_var(env, "MMC_NAME=%s", mmc_card_name(card));
	if (retval)
		return retval;

	/*
	 * Request the mmc_block device.  Note: that this is a direct request
	 * for the module it carries no information as to what is inserted.
	 */
	retval = add_uevent_var(env, "MODALIAS=mmc:block");

	return retval;
}

static int mmc_bus_probe(struct device *dev)
{
	struct mmc_driver *drv = to_mmc_driver(dev->driver);
	struct mmc_card *card = mmc_dev_to_card(dev);

	return drv->probe(card);
}

static int mmc_bus_remove(struct device *dev)
{
	struct mmc_driver *drv = to_mmc_driver(dev->driver);
	struct mmc_card *card = mmc_dev_to_card(dev);

	drv->remove(card);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mmc_bus_suspend(struct device *dev)
{
	struct mmc_driver *drv = to_mmc_driver(dev->driver);
	struct mmc_card *card = mmc_dev_to_card(dev);
	int ret = 0;

	if (dev->driver && drv->suspend)
		ret = drv->suspend(card);
	return ret;
}

static int mmc_bus_resume(struct device *dev)
{
	struct mmc_driver *drv = to_mmc_driver(dev->driver);
	struct mmc_card *card = mmc_dev_to_card(dev);
	int ret = 0;

	if (dev->driver && drv->resume)
		ret = drv->resume(card);
	return ret;
}
#endif

#ifdef CONFIG_PM_RUNTIME

static int mmc_runtime_suspend(struct device *dev)
{
	struct mmc_card *card = mmc_dev_to_card(dev);

	return mmc_power_save_host(card->host);
}

static int mmc_runtime_resume(struct device *dev)
{
	struct mmc_card *card = mmc_dev_to_card(dev);

	return mmc_power_restore_host(card->host);
}

static int mmc_runtime_idle(struct device *dev)
{
	return pm_runtime_suspend(dev);
}

#endif /* !CONFIG_PM_RUNTIME */

static const struct dev_pm_ops mmc_bus_pm_ops = {
	SET_RUNTIME_PM_OPS(mmc_runtime_suspend, mmc_runtime_resume,
			mmc_runtime_idle)
	SET_SYSTEM_SLEEP_PM_OPS(mmc_bus_suspend, mmc_bus_resume)
};

static struct bus_type mmc_bus_type = {
	.name		= "mmc",
	.dev_attrs	= mmc_dev_attrs,
	.match		= mmc_bus_match,
	.uevent		= mmc_bus_uevent,
	.probe		= mmc_bus_probe,
	.remove		= mmc_bus_remove,
	.pm		= &mmc_bus_pm_ops,
};

int mmc_register_bus(void)
{
	return bus_register(&mmc_bus_type);
}

void mmc_unregister_bus(void)
{
	bus_unregister(&mmc_bus_type);
}

/**
 *	mmc_register_driver - register a media driver
 *	@drv: MMC media driver
 */
int mmc_register_driver(struct mmc_driver *drv)
{
	drv->drv.bus = &mmc_bus_type;
	return driver_register(&drv->drv);
}

EXPORT_SYMBOL(mmc_register_driver);

/**
 *	mmc_unregister_driver - unregister a media driver
 *	@drv: MMC media driver
 */
void mmc_unregister_driver(struct mmc_driver *drv)
{
	drv->drv.bus = &mmc_bus_type;
	driver_unregister(&drv->drv);
}

EXPORT_SYMBOL(mmc_unregister_driver);

static void mmc_release_card(struct device *dev)
{
	struct mmc_card *card = mmc_dev_to_card(dev);

	sdio_free_common_cis(card);

	if (card->info)
	kfree(card->info);

	kfree(card);
	card = NULL;
}

/*
 * Allocate and initialise a new MMC card structure.
 */
struct mmc_card *mmc_alloc_card(struct mmc_host *host, struct device_type *type)
{
	struct mmc_card *card;

	card = kzalloc(sizeof(struct mmc_card), GFP_KERNEL);
	if (!card)
		return ERR_PTR(-ENOMEM);

	card->host = host;

	device_initialize(&card->dev);

	card->dev.parent = mmc_classdev(host);
	card->dev.bus = &mmc_bus_type;
	card->dev.release = mmc_release_card;
	card->dev.type = type;

	return card;
}


struct mmc_card *slot0_card = NULL;
struct mmc_card *slot2_card = NULL;

int owl_set_carddev_match_name(void)
{
	int ret = 0;
	int boot_dev;
	int timeout = WAIT_CARD_TIMEOUT;
    boot_dev = owl_get_boot_dev();
	printk("%s: bootdev 0x%x\n", __FUNCTION__, boot_dev);
	
	switch (boot_dev) {
		
	case OWL_BOOTDEV_SD0:
		
		while((!slot0_card)&&(--timeout)){
			msleep(10);
		}
		if( timeout <= 0){
			printk("err:%s:slot0_card NULL\n",__FUNCTION__);
			ret = -1;
			goto out;
		}else{
			printk("%s,sd0 as boot card,sd0 name tsd_card\n", __FUNCTION__);
			dev_set_name(&slot0_card->dev,"%s","tsd_card");
		}
		break;
		
	case OWL_BOOTDEV_SD2:
		
		while((!slot2_card)&&(--timeout)){
			msleep(10);
		}
		if( timeout < 0){
			printk("err:%s:slot2_card NULL\n",__FUNCTION__);
			ret = -1;
			goto out;
		}else{
			printk("%s,sd2 as boot card,sd2 name tsd_card\n", __FUNCTION__);
			dev_set_name(&slot2_card->dev,"%s","tsd_card");
		}
			
		break;
		
	case OWL_BOOTDEV_SD02SD2:
		
		while((!slot2_card)&&(--timeout)){
			msleep(10);
		}
		if(timeout < 0){
			printk("err:%s:slot2_card NULL\n",__FUNCTION__);
			ret = -1;
			goto out;
		}else{
			printk("%s,sd0 as boot card,sd2 name card_to_card\n", __FUNCTION__);
			dev_set_name(&slot2_card->dev,"%s","card_to_card");
		}	
		break;	
		
	default:
		printk("ERR:%s: bootdev 0x%x\n", __FUNCTION__, boot_dev);
		ret = -1 ;	
		break;	
		
	}
out:
	return ret;
}

EXPORT_SYMBOL(owl_set_carddev_match_name);
/*
 * Register a new MMC card with the driver model.
 */
int mmc_add_card(struct mmc_card *card)
{
	int ret;
	const char *type;
	const char *uhs_bus_speed_mode = "";
	struct gl520xmmc_host *hcd;
    int boot_dev;
	static const char *const uhs_speeds[] = {
		[UHS_SDR12_BUS_SPEED] = "SDR12 ",
		[UHS_SDR25_BUS_SPEED] = "SDR25 ",
		[UHS_SDR50_BUS_SPEED] = "SDR50 ",
		[UHS_SDR104_BUS_SPEED] = "SDR104 ",
		[UHS_DDR50_BUS_SPEED] = "DDR50 ",
	};



    boot_dev = owl_get_boot_dev();
    printk("%s: bootdev 0x%x\n", __FUNCTION__, boot_dev);

	hcd = mmc_priv(card->host);
	if(SDC0_SLOT == hcd->id)
	{
		slot0_card = card;
        //if (boot_dev == OWL_BOOTDEV_NAND || boot_dev == OWL_BOOTDEV_SD2) {
  		printk("force sd0/sd1 host ext-card\n");
    		dev_set_name(&card->dev,"%s","sd_card");
        //}
        //else{
	//	dev_set_name(&card->dev, "%s", "sd_boot_card");
	//}
		    
	}
	else if(SDC1_SLOT == hcd->id)
	{
		 dev_set_name(&card->dev,"%s:%04x",mmc_hostname(card->host),card->rca);
	}
	else if(SDC2_SLOT == hcd->id)
	{
		slot2_card = card;	
		if(boot_dev == OWL_BOOTDEV_SD0){
			printk("force sd2(emmc) host ext-card\n");
			dev_set_name(&card->dev, "%s", "emmc");				
		} else{
			printk("force sd2(emmc) host emmc_boot_card\n");
			dev_set_name(&card->dev, "%s", "emmc_boot_card");
			//return 0;
		}
	}

	switch (card->type) {
	case MMC_TYPE_MMC:
		type = "MMC";
		break;
	case MMC_TYPE_SD:
		type = "SD";
		if (mmc_card_blockaddr(card)) {
			if (mmc_card_ext_capacity(card))
				type = "SDXC";
			else
				type = "SDHC";
		}
		break;
	case MMC_TYPE_SDIO:
		type = "SDIO";
		break;
	case MMC_TYPE_SD_COMBO:
		type = "SD-combo";
		if (mmc_card_blockaddr(card))
			type = "SDHC-combo";
		break;
	default:
		type = "?";
		break;
	}

	if (mmc_sd_card_uhs(card) &&
		(card->sd_bus_speed < ARRAY_SIZE(uhs_speeds)))
		uhs_bus_speed_mode = uhs_speeds[card->sd_bus_speed];

	if (mmc_host_is_spi(card->host)) {
		pr_info("%s: new %s%s%s card on SPI\n",
			mmc_hostname(card->host),
			mmc_card_highspeed(card) ? "high speed " : "",
			mmc_card_ddr_mode(card) ? "DDR " : "",
			type);
	} else {
		pr_info("%s: new %s%s%s%s%s card at address %04x\n",
			mmc_hostname(card->host),
			mmc_card_uhs(card) ? "ultra high speed " :
			(mmc_card_highspeed(card) ? "high speed " : ""),
			(mmc_card_hs200(card) ? "HS200 " : ""),
			mmc_card_ddr_mode(card) ? "DDR " : "",
			uhs_bus_speed_mode, type, card->rca);
	}

#ifdef CONFIG_DEBUG_FS
	mmc_add_card_debugfs(card);
#endif
	mmc_init_context_info(card->host);

	ret = device_add(&card->dev);
	if (ret)
		return ret;

	mmc_card_set_present(card);

	return 0;
}

/*
 * Unregister a new MMC card with the driver model, and
 * (eventually) free it.
 */
void mmc_remove_card(struct mmc_card *card)
{
#ifdef CONFIG_DEBUG_FS
	mmc_remove_card_debugfs(card);
#endif

	if (mmc_card_present(card)) {
		if (mmc_host_is_spi(card->host)) {
			pr_info("%s: SPI card removed\n",
				mmc_hostname(card->host));
		} else {
			pr_info("%s: card %04x removed\n",
				mmc_hostname(card->host), card->rca);
		}
		device_del(&card->dev);
	}

	put_device(&card->dev);
}

