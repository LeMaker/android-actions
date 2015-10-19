/*
 * linux/drivers/video/owl/dss/dss.h
 *
 * Copyright (C) 2009 Actions Corporation
 * Author: Hui Wang  <wanghui@actions-semi.com>
 *
 * Some code and ideas taken from drivers/video/owl/ driver
 * by leopard.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __OWL_DSS_H
#define __OWL_DSS_H

#include <video/owldss.h>
#include <linux/platform_device.h>


/*{====================debug macro==========================*/
#ifdef CONFIG_OWL_DSS_DEBUG_SUPPORT
	#define DSS_DEBUG_ENABLE
#endif

#define DSS_DEBUG_ENABLE

#ifndef DSS_SUBSYS_NAME
	#define SUBSYS_NAME		"owl_dss: "
#else
	#define SUBSYS_NAME		"owl_dss " DSS_SUBSYS_NAME ": "
#endif

#ifdef DSS_DEBUG_ENABLE
	extern int owl_dss_debug;
	#define DSSDBG(format, ...) \
		do { \
			if (owl_dss_debug > 1) \
				printk(KERN_DEBUG SUBSYS_NAME format, ## __VA_ARGS__); \
	        } while (0)

	#define DSSDBGF(format, ...) \
		do { \
			if (owl_dss_debug > 1) \
				printk(KERN_DEBUG SUBSYS_NAME "%s," format,\
					 __func__, ## __VA_ARGS__); \
	        } while (0)

	#define DSSINFO(format, ...) \
		do { \
			if (owl_dss_debug > 0) \
				printk(KERN_INFO SUBSYS_NAME format, ## __VA_ARGS__); \
	        } while (0)

#else
	#define DSSDBG(format, ...)
	#define DSSDBGF(format, ...)
	#define DSSINFO(format, ...)
#endif

#define DSSERR(format, ...) \
	printk(KERN_ERR SUBSYS_NAME "error!" format, ## __VA_ARGS__);
/*====================debug macro end==========================}*/


/* core */
struct bus_type *dss_get_bus(void);

int dss_mgr_wait_for_go(struct owl_overlay_manager *mgr);

int dss_mgr_enable(struct owl_overlay_manager *mgr);
void dss_mgr_disable(struct owl_overlay_manager *mgr);
int dss_mgr_set_info(struct owl_overlay_manager *mgr,
		struct owl_overlay_manager_info *info);
void dss_mgr_get_info(struct owl_overlay_manager *mgr,
		struct owl_overlay_manager_info *info);
int dss_mgr_set_device(struct owl_overlay_manager *mgr,
		struct owl_dss_device *dssdev);
int dss_mgr_unset_device(struct owl_overlay_manager *mgr);

bool dss_ovl_is_enabled(struct owl_overlay *ovl);
int dss_ovl_enable(struct owl_overlay *ovl);
int dss_ovl_disable(struct owl_overlay *ovl);
int dss_ovl_set_info(struct owl_overlay *ovl,
		struct owl_overlay_info *info);
void dss_ovl_get_info(struct owl_overlay *ovl,
		struct owl_overlay_info *info);
int dss_ovl_set_manager(struct owl_overlay *ovl,
		struct owl_overlay_manager *mgr);
int dss_ovl_unset_manager(struct owl_overlay *ovl);

/* display */
int dss_suspend_all_devices(void);
int dss_resume_all_devices(void);
void dss_disable_all_devices(void);

void dss_init_device(struct platform_device *pdev,
		struct owl_dss_device *dssdev);
void dss_uninit_device(struct platform_device *pdev,
		struct owl_dss_device *dssdev);
bool dss_use_replication(struct owl_dss_device *dssdev,
		enum owl_color_mode mode);
void default_get_overlay_fifo_thresholds(enum owl_plane plane,
		u32 fifo_size, u32 burst_size,
		u32 *fifo_low, u32 *fifo_high);

/* manager */
int dss_init_overlay_managers(struct platform_device *pdev);
void dss_uninit_overlay_managers(struct platform_device *pdev);
int dss_mgr_simple_check(struct owl_overlay_manager *mgr,
		const struct owl_overlay_manager_info *info);
int dss_mgr_check(struct owl_overlay_manager *mgr,
		struct owl_dss_device *dssdev,
		struct owl_overlay_manager_info *info,
		struct owl_overlay_info **overlay_infos);

/* overlay */
void dss_init_overlays(struct platform_device *pdev);
void dss_uninit_overlays(struct platform_device *pdev);
void dss_recheck_connections(struct owl_dss_device *dssdev, bool force);
int dss_ovl_simple_check(struct owl_overlay *ovl,const struct owl_overlay_info *info);
int dss_ovl_check(struct owl_overlay *ovl,	struct owl_overlay_info *info, struct owl_dss_device *dssdev);

/* DSS */
int dss_init_platform_device(void)__init;
void dss_uninit_platform_device(void);

int dss_init_platform_driver(void)__init;
void dss_uninit_platform_driver(void);

int dss_runtime_get(void);
void dss_runtime_put(void);

void owl_de_suspend(void);
void owl_de_resume(void);

#ifdef CONFIG_VIDEO_OWL_MMU_SUPPORT
int mmu_init(void);

/* map virtual address to device address */
int mmu_va_to_da(u64 va, u32 length, u32 *da);

/* map ION fd to device address */
int mmu_fd_to_da(u64 buffer_id, u32 *da);

#else
static inline int mmu_init(void) { return -1; }
static inline int mmu_va_to_da(u64 va, u32 length, u32 *da) { return -1; }
static inline int mmu_fd_to_da(u64 buffer_id, u32 *da) { return -1; }
#endif

#endif
