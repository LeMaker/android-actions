/*
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/memblock.h>
#include <linux/dma-contiguous.h>

#include "../../../drivers/staging/android/ion/ion.h"
#include "../../../drivers/staging/android/uapi/ion-owl.h"

#include <linux/pfn.h>
#include <linux/vmalloc.h>
#include <asm/setup.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#if defined(CONFIG_OF)
#include <linux/of_fdt.h>
#endif

#include <mach/dss-owl.h>
#include <mach/kinfo.h>


static struct ion_platform_heap owl_pdev_ion_heaps[] = {
    {
        .type = ION_HEAP_TYPE_CARVEOUT,
        .id = ION_HEAP_ID_FB,
        .name = "ion_fb",
        .base = 0,  /* Filled in by owl_reserve() */
        .size = 0,  /* Filled in by owl_reserve() */
    },

    {
#ifdef CONFIG_CMA
        .type = ION_HEAP_TYPE_DMA,
#else
        .type = ION_HEAP_TYPE_CARVEOUT,
#endif
        .id = ION_HEAP_ID_PMEM,
        .name = "ion_pmem",
        .base = 0,  /* Filled in by owl_reserve() */
        .size = 0,  /* Filled in by owl_reserve() */
        .priv = NULL,
    },

    {
        .type = ION_HEAP_TYPE_SYSTEM,
        .id = ION_HEAP_ID_SYSTEM,
        .name = "ion_system",
    },
};

static struct ion_platform_data owl_pdev_ion_data = {
	.nr = ARRAY_SIZE(owl_pdev_ion_heaps),
	.heaps = owl_pdev_ion_heaps,
};

static struct platform_device owl_pdev_ion_device = {
	.name = "ion-owl",
	.id = -1,
	.dev = {
		.platform_data = &owl_pdev_ion_data,
		.coherent_dma_mask = -1, /* no restrict */
	},
};

#ifndef CONFIG_OF
static struct resource owl_gpu_res[] = {
	{
		.name  = "gpu_irq",
		.start = OWL_IRQ_GPU_3D,
		.end   = OWL_IRQ_GPU_3D,
		.flags = IORESOURCE_IRQ,
	},
	{
		.name  = "gpu_base",
		.start = 0xB0300000,
		.end   = 0xB030FFFF,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device owl_gpu_device = {
	.name = "pvrsrvkm",
	.resource = owl_gpu_res,
	.num_resources = ARRAY_SIZE(owl_gpu_res),
};
#endif

/*
unsigned char *g_afinfo = NULL;
int afinfo_buf_len;
static unsigned long afinfo_phy;
static int __init afinfo_process(char *str)
{
	char *str_len;

	if (str == NULL || *str == '\0')
		return 0;

	str_len = strchr(str, ',');
	if (!str_len)
		return 0;
	if (*(str_len + 1) == 0)
		return 0;

	str_len = str_len + 1;

	afinfo_phy = simple_strtoul(str, NULL, 16);
	afinfo_buf_len = simple_strtoul(str_len, NULL, 16);

	return 1;
}
__setup("afinfo=", afinfo_process);

static int __init afinfo_init(void)
{
	void *tmp;

	g_afinfo = kmalloc(afinfo_buf_len, GFP_ATOMIC);
	tmp = ioremap(afinfo_phy, afinfo_buf_len);
	memcpy(g_afinfo, tmp, afinfo_buf_len);
	iounmap(tmp);

	pr_err("### afinfo_phy 0x%08lx, g_afinfo: 0x%p, afinfo_buf_len 0x%x\n", 
		afinfo_phy, g_afinfo, afinfo_buf_len);
	return 0;
}
arch_initcall(afinfo_init);
*/

static phys_addr_t s_phy_mem_size_saved;
phys_addr_t owl_get_phy_mem_size(void)
{
	return s_phy_mem_size_saved;
}
EXPORT_SYMBOL(owl_get_phy_mem_size);

static unsigned int owl_fb_start, owl_fb_size, owl_kinfo_start, owl_kinfo_size, owl_ion0_size, owl_ion1_size;

void __init earlyl_init_dt_get_ion_info(unsigned long node)
{
	__be32 *prop;

    if(owl_fb_size == 0) {
        prop = of_get_flat_dt_prop(node, "fb_heap_size", NULL);
    	if (prop) {
            owl_fb_size = be32_to_cpup(prop) * SZ_1M;
            printk("find owl_fb_size=0x%x\n", owl_fb_size);
        }
    }
    if(owl_kinfo_size == 0) {
        prop = of_get_flat_dt_prop(node, "kinfo_heap_size", NULL);
    	if (prop) {
            owl_kinfo_size = be32_to_cpup(prop) * SZ_1M;
            printk("find owl_kinfo_size=0x%x\n", owl_kinfo_size);
        }
    }
#ifdef CONFIG_ION
    if(owl_ion0_size == 0) {
        prop = of_get_flat_dt_prop(node, "carveout_heap_size", NULL);
    	if (prop) {
            owl_ion0_size = be32_to_cpup(prop) * SZ_1M;
            printk("find owl_ion0_size=0x%x\n", owl_ion0_size);
        }
    }
    if(owl_ion1_size == 0) {
        prop = of_get_flat_dt_prop(node, "dma_heap_size", NULL);
    	if (prop) {
            owl_ion1_size = be32_to_cpup(prop) * SZ_1M;
            printk("find owl_ion1_size=0x%x\n", owl_ion1_size);
        }
    }
#endif
}

int __init early_init_dt_scan_ion(unsigned long node, const char *uname,
				   int depth, void *data)
{
    static int prop_depth = 1;

	if (depth != prop_depth)
		return 0;

    if(depth == 1 && strcmp(uname, "reserved") == 0) {
        prop_depth = 2;
        return 0;
    }
    
    if(depth == 2) {
        if((phys_addr_t)data > (512 * 1024 * 1024)) {
            if(strcmp(uname, "normal") == 0) {
                earlyl_init_dt_get_ion_info(node);
                return 1;
            }
        } else {
            if(strcmp(uname, "tiny") == 0) {
                earlyl_init_dt_get_ion_info(node);
                return 1;
            }
        }
    }

	return 0;
}

extern phys_addr_t arm_lowmem_limit;
void __init owl_reserve(void)
{
	phys_addr_t phy_mem_size, phy_mem_end;
	unsigned int owl_ion0_start = 0;
	unsigned int owl_ion1_start = 0;

	phy_mem_size = memblock_phys_mem_size();
	if (phy_mem_size & (phy_mem_size - 1)) { /* != 2^n ? */
		uint _tmp = __fls(phy_mem_size);
		if (_tmp > 0 && (phy_mem_size & (1U << (_tmp - 1)))) {
			/* close to next boundary */
			_tmp++;
			phy_mem_size =
				(_tmp >= sizeof(phy_mem_size) * 8) ? phy_mem_size : (1U << _tmp);
		} else {
			phy_mem_size = 1U << _tmp;
		}
	}
	s_phy_mem_size_saved = phy_mem_size;
	phy_mem_end = arm_lowmem_limit;
	pr_info("%s: pyhsical memory size %u bytes, end @0x%x\n",
		__func__, phy_mem_size, phy_mem_end);

	memblock_reserve(0, 0x4000); /* reserve low 16K for DDR dqs training */

	of_scan_flat_dt(early_init_dt_scan_ion, (void*)phy_mem_size);

	phy_mem_end -= owl_fb_size;
#ifdef CONFIG_VIDEO_OWL_DSS
    owl_fb_start = phy_mem_end;
    memblock_reserve(owl_fb_start, owl_fb_size);
#endif

	phy_mem_end -= owl_kinfo_size;
    owl_kinfo_start = phy_mem_end;
    memblock_reserve(owl_kinfo_start, owl_kinfo_size);
    
#ifdef CONFIG_ION
	phy_mem_end -= owl_ion0_size;
	owl_ion0_start = phy_mem_end;
	owl_pdev_ion_data.heaps[0].base = owl_ion0_start;
	owl_pdev_ion_data.heaps[0].size = owl_ion0_size;

	/* ion_pmem */
#ifdef CONFIG_CMA
	phy_mem_end -= owl_ion1_size;
	owl_ion1_start = phy_mem_end; /* fake, not used. */
	owl_pdev_ion_data.heaps[1].base = 0;
	owl_pdev_ion_data.heaps[1].size = 0; /* prevent ion_reserve() from diging */
	owl_pdev_ion_data.heaps[1].priv = &(owl_pdev_ion_device.dev);
	dma_contiguous_set_global_reserve_size(owl_ion1_size); /* set size of the CMA global area */
#else /* no CMA */
	phy_mem_end -= owl_ion1_size;
	owl_ion1_start = phy_mem_end;
	owl_pdev_ion_data.heaps[1].base = owl_ion1_start;
	owl_pdev_ion_data.heaps[1].size = owl_ion1_size;
#endif
	ion_reserve(&owl_pdev_ion_data);
#endif

	printk(KERN_INFO "Reserved memory %uMB\n",
		(owl_ion0_size + owl_ion1_size) >> 20);
	printk(KERN_INFO 
	        "   FB:     0x%08x, %uMB\n"
	        "   KINFO:  0x%08x, %uMB\n"
	        "   ION0:   0x%08x, %uMB\n"
			"   ION1:   0x%08x, %uMB\n",
			owl_fb_start, owl_fb_size >> 20,
			owl_kinfo_start, owl_kinfo_size >> 20,
			owl_ion0_start, owl_ion0_size >> 20,
			owl_ion1_start, owl_ion1_size >> 20);
}

static void free_owl_reserved_memory(unsigned int free_start, unsigned int free_size)
{
    unsigned long n, start, end;
#ifdef	CONFIG_HIGHMEM
    unsigned long max_low = max_low_pfn + PHYS_PFN_OFFSET;
#endif

    start = free_start;
    end = free_start + free_size;
    if( (start >> PAGE_SHIFT) <= max_low ) {
        if( (end >> PAGE_SHIFT) > max_low )
            end = max_low << PAGE_SHIFT;
        n = free_reserved_area(__phys_to_virt(start), __phys_to_virt(end), 0, NULL);
        printk("free reserve pages %lu to buddy system\n", n);
    }

#ifdef	CONFIG_HIGHMEM
    start = free_start >> PAGE_SHIFT;
    end = (free_start + free_size) >> PAGE_SHIFT;
    if( end > max_low ) {
        if(start < max_low)
            start = max_low;
    	for (n = start; n < end; n++)
    		free_highmem_page(pfn_to_page(n));
        printk("free reserve high memory pages %lu to buddy system\n", end - start);
    }
#endif
}

#ifdef CONFIG_VIDEO_OWL_DSS
void free_fb_reserved_memory(void)
{
	if(owl_fb_size > 0) {
        free_owl_reserved_memory(owl_fb_start, owl_fb_size);
		owl_fb_size = 0;
	}
}
EXPORT_SYMBOL(free_fb_reserved_memory);
#endif

static struct platform_device *owl_common_devices[] __initdata = {
	&owl_pdev_ion_device,
#ifndef CONFIG_OF
	&owl_gpu_device,
#endif
};

unsigned long get_ion_reserved_mem_size(void)
{
	return (unsigned long)(owl_pdev_ion_data.heaps[0].size +
		owl_pdev_ion_data.heaps[1].size);
}

int __init owl_common_init(void)
{
	printk(KERN_INFO "%s()\n", __func__);

	platform_add_devices(owl_common_devices,
		ARRAY_SIZE(owl_common_devices));

	return 0;
}

arch_initcall(owl_common_init);


/***************/
#include <mach/kinfo.h>

struct kernel_reserve_info *kinfo;
EXPORT_SYMBOL(kinfo);

int __init owl_kinfo_init(void)
{
    struct page **pages;
    unsigned int pages_count, pfn, i;
    void __iomem * kinfo_vaddr;

    kinfo = kmalloc(sizeof(struct kernel_reserve_info), GFP_KERNEL);
    if(kinfo == NULL) {
        printk(KERN_ALERT "%s, kmalloc(%d) for kinfo failed!\n",
                __func__, sizeof(struct kernel_reserve_info));
        return -ENOMEM;
    }
    
    pages_count = owl_kinfo_size >> PAGE_SHIFT;
    pages = kmalloc(sizeof *pages * pages_count, GFP_KERNEL);
    if (!pages) {
        printk(KERN_ALERT "%s, kmalloc(%d) for pages failed!\n",
                __func__, sizeof *pages * pages_count);
        return -ENOMEM;
    }
    pfn = PFN_DOWN(owl_kinfo_start);
    for (i = 0; i < pages_count; ++i)
        pages[i] = pfn_to_page(pfn + i);
    kinfo_vaddr = vm_map_ram(pages, pages_count, -1, PAGE_KERNEL);
    if(kinfo_vaddr == NULL) {
        printk(KERN_ALERT "%s, ioremap(0x%x, 0x%x) for kinfo failed!\n",
                __func__, owl_kinfo_start, owl_kinfo_size);
        return -ENOMEM;
    }
    memcpy(kinfo, kinfo_vaddr, sizeof(struct kernel_reserve_info));
    vm_unmap_ram(kinfo_vaddr, pages_count);
    kfree(pages);
    
    free_owl_reserved_memory(owl_kinfo_start, owl_kinfo_size);
    return 0;
}
arch_initcall(owl_kinfo_init);


/***************/



/*0:atc2603 , 1:atc2609 , -1:NULL*/
static int pmu_id = -1;
static ssize_t pmu_id_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", pmu_id);
}

static DEVICE_ATTR(pmu_id, 0664, pmu_id_show, NULL);

int __init owl_pmu_detect(void)
{
	int ret;
	struct device_node *np;

	ret = sysfs_create_file(power_kobj, &dev_attr_pmu_id.attr);
	if (ret)
		pr_err("sysfs_create_file failed: %d\n", ret);

	np = of_find_node_by_name(NULL, "dcdc0");
	if (np) {
		pmu_id = 1;
		return 0;
	}

	np = of_find_node_by_name(NULL, "dcdc1");
	if (np) {
		pmu_id = 0;
		return 0;
	}

	pr_err("DTS without PMU node!\n");
	return -EINVAL;
}

late_initcall(owl_pmu_detect);

