/*
 * Actions SOC ion heap device
 *
 * Copyright (C) 2014 Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/of.h>

#include "../ion.h"
#include "../ion_priv.h"
#include "../../uapi/ion-owl.h"

#if 0
#define OWL_ION_HEAP_NUM	3
static struct ion_platform_heap owl_ion_heaps[OWL_ION_HEAP_NUM] = {
    {
        .type = ION_HEAP_TYPE_CARVEOUT,
        .id = ION_HEAP_ID_FB,
        .name = "ion_fb",
        .base = 0,
        .size = 0,
    },

    {
        .type = ION_HEAP_TYPE_CARVEOUT,
        .id = ION_HEAP_ID_PMEM,
        .name = "ion_pmem",
        .base = 0,
        .size = 0,
    },

    {
        .type = ION_HEAP_TYPE_SYSTEM,
        .id = ION_HEAP_ID_SYSTEM,
        .name = "ion_system",
    },
};

static struct ion_platform_data owl_ion_data = {
	.nr = OWL_ION_HEAP_NUM,
	.heaps = owl_ion_heaps,
};
#endif

struct ion_device *owl_ion_device;
EXPORT_SYMBOL(owl_ion_device);

static int num_heaps;
static struct ion_heap **heaps;

static int owl_ion_get_phys(struct ion_client *client,
					unsigned int cmd,
					unsigned long arg)
{
	struct owl_ion_phys_data data;
	struct owl_ion_phys_data *user_data =
				(struct owl_ion_phys_data *)arg;
	struct ion_handle *handle;
	struct ion_buffer *buffer;
	int ret;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
		return -EFAULT;

	handle = ion_handle_get_by_id(client, data.handle);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	buffer = ion_handle_buffer(handle);
	ret = ion_phys(client, handle, &data.phys_addr, &data.size);
	ion_handle_put_outter(handle);
	if(ret < 0)
		return ret;

	if (copy_to_user(user_data, &data, sizeof(data)))
		return -EFAULT;

	return 0;
}

static long owl_ion_ioctl(struct ion_client *client,
				   unsigned int cmd,
				   unsigned long arg)
{
	int ret = -EINVAL;

	switch (cmd) {
	case OWL_ION_GET_PHY:
		ret = owl_ion_get_phys(client, cmd, arg);
		break;
	default:
		WARN(1, "Unknown custom ioctl\n");
		return -EINVAL;
	}
	return ret;
}

static int owl_ion_probe(struct platform_device *pdev)
{
	struct ion_platform_data *pdata = pdev->dev.platform_data;
	int err;
	int i;

	num_heaps = pdata->nr;

	heaps = kzalloc(sizeof(struct ion_heap *) * pdata->nr, GFP_KERNEL);

	owl_ion_device = ion_device_create(owl_ion_ioctl);
	if (IS_ERR_OR_NULL(owl_ion_device)) {
		kfree(heaps);
		return PTR_ERR(owl_ion_device);
	}

	/* create the heaps as specified in the board file */
	for (i = 0; i < num_heaps; i++) {
		struct ion_platform_heap *heap_data = &pdata->heaps[i];

		heaps[i] = ion_heap_create(heap_data);
		if (IS_ERR_OR_NULL(heaps[i])) {
			err = PTR_ERR(heaps[i]);
			goto err;
		}
		ion_device_add_heap(owl_ion_device, heaps[i]);
		pr_info("%s: add heap %s\n", __func__, heaps[i]->name);
	}

	platform_set_drvdata(pdev, owl_ion_device);
	return 0;
err:
	for (i = 0; i < num_heaps; i++) {
		if (heaps[i])
			ion_heap_destroy(heaps[i]);
	}
	kfree(heaps);
	return err;
}

static int owl_ion_remove(struct platform_device *pdev)
{
	struct ion_device *owl_ion_device = platform_get_drvdata(pdev);
	int i;

	ion_device_destroy(owl_ion_device);
	for (i = 0; i < num_heaps; i++)
		ion_heap_destroy(heaps[i]);
	kfree(heaps);
	return 0;
}

static struct platform_driver owl_ion_driver = {
	.probe = owl_ion_probe,
	.remove = owl_ion_remove,
	.driver = { .name = "ion-owl" }
};

static int __init owl_ion_init(void)
{
	return platform_driver_register(&owl_ion_driver);
}

static void __exit owl_ion_exit(void)
{
	platform_driver_unregister(&owl_ion_driver);
}

subsys_initcall(owl_ion_init);
module_exit(owl_ion_exit);

