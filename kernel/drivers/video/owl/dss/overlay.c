/*
 * linux/drivers/video/owl/dss/overlay.c
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

#define DSS_SUBSYS_NAME "OVERLAY"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <video/owldss.h>

#include "dss.h"
#include "dss_features.h"
#include "de.h"

static int num_overlays;
static struct owl_overlay *overlays;

static ssize_t overlay_name_show(struct owl_overlay *ovl, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", ovl->name);
}

static ssize_t overlay_manager_show(struct owl_overlay *ovl, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n",
			ovl->manager ? ovl->manager->name : "<none>");
}

static ssize_t overlay_manager_store(struct owl_overlay *ovl, const char *buf,
		size_t size)
{
	int i, r;
	struct owl_overlay_manager *mgr = NULL;
	struct owl_overlay_manager *old_mgr;
	int len = size;

	if (buf[size-1] == '\n')
		--len;

	if (len > 0) {
		for (i = 0; i < owl_dss_get_num_overlay_managers(); ++i) {
			mgr = owl_dss_get_overlay_manager(i);

			if (sysfs_streq(buf, mgr->name))
				break;

			mgr = NULL;
		}
	}

	if (len > 0 && mgr == NULL)
		return -EINVAL;

	if (mgr)
		DSSDBG("manager %s found\n", mgr->name);

	if (mgr == ovl->manager)
		return size;

	old_mgr = ovl->manager;

	/* detach old manager */
	if (old_mgr) {
		r = ovl->unset_manager(ovl);
		if (r) {
			DSSERR("detach failed\n");
			goto err;
		}

		r = old_mgr->apply(old_mgr);
		if (r)
			goto err;
	}

	if (mgr) {
		r = ovl->set_manager(ovl, mgr);
		if (r) {
			DSSERR("Failed to attach overlay\n");
			goto err;
		}

		r = mgr->apply(mgr);
		if (r)
			goto err;
	}

	return size;

err:
	return r;
}

static ssize_t overlay_input_size_show(struct owl_overlay *ovl, char *buf)
{
	struct owl_overlay_info info;

	ovl->get_overlay_info(ovl, &info);

	return snprintf(buf, PAGE_SIZE, "%d,%d\n",
			info.width, info.height);
}

static ssize_t overlay_screen_width_show(struct owl_overlay *ovl, char *buf)
{
	struct owl_overlay_info info;

	ovl->get_overlay_info(ovl, &info);

	return snprintf(buf, PAGE_SIZE, "%d\n", info.screen_width);
}

static ssize_t overlay_position_show(struct owl_overlay *ovl, char *buf)
{
	struct owl_overlay_info info;

	ovl->get_overlay_info(ovl, &info);

	return snprintf(buf, PAGE_SIZE, "%d,%d\n",
			info.pos_x, info.pos_y);
}

static ssize_t overlay_position_store(struct owl_overlay *ovl,
		const char *buf, size_t size)
{
	int r;
	char *last;
	struct owl_overlay_info info;

	ovl->get_overlay_info(ovl, &info);

	info.pos_x = simple_strtoul(buf, &last, 10);
	++last;
	if (last - buf >= size)
		return -EINVAL;

	info.pos_y = simple_strtoul(last, &last, 10);

	r = ovl->set_overlay_info(ovl, &info);
	if (r)
		return r;

	if (ovl->manager) {
		r = ovl->manager->apply(ovl->manager);
		if (r)
			return r;
	}

	return size;
}

static ssize_t overlay_output_size_show(struct owl_overlay *ovl, char *buf)
{
	struct owl_overlay_info info;

	ovl->get_overlay_info(ovl, &info);

	return snprintf(buf, PAGE_SIZE, "%d,%d\n",
			info.out_width, info.out_height);
}

static ssize_t overlay_output_size_store(struct owl_overlay *ovl,
		const char *buf, size_t size)
{
	int r;
	char *last;
	struct owl_overlay_info info;

	ovl->get_overlay_info(ovl, &info);

	info.out_width = simple_strtoul(buf, &last, 10);
	++last;
	if (last - buf >= size)
		return -EINVAL;

	info.out_height = simple_strtoul(last, &last, 10);

	r = ovl->set_overlay_info(ovl, &info);
	if (r)
		return r;

	if (ovl->manager) {
		r = ovl->manager->apply(ovl->manager);
		if (r)
			return r;
	}

	return size;
}

static ssize_t overlay_enabled_show(struct owl_overlay *ovl, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", ovl->is_enabled(ovl));
}

static ssize_t overlay_enabled_store(struct owl_overlay *ovl, const char *buf,
		size_t size)
{
	int r;
	bool enable;

	r = strtobool(buf, &enable);
	if (r)
		return r;

	if (enable)
		r = ovl->enable(ovl);
	else
		r = ovl->disable(ovl);

	if (r)
		return r;

	return size;
}

static ssize_t overlay_global_alpha_show(struct owl_overlay *ovl, char *buf)
{
	struct owl_overlay_info info;

	ovl->get_overlay_info(ovl, &info);

	return snprintf(buf, PAGE_SIZE, "%d\n",
			info.global_alpha);
}

static ssize_t overlay_global_alpha_store(struct owl_overlay *ovl,
		const char *buf, size_t size)
{
	int r;
	u8 alpha;
	struct owl_overlay_info info;

	if ((ovl->caps & OWL_DSS_OVL_CAP_GLOBAL_ALPHA) == 0)
		return -ENODEV;

	r = kstrtou8(buf, 0, &alpha);
	if (r)
		return r;

	ovl->get_overlay_info(ovl, &info);

	info.global_alpha = alpha;

	r = ovl->set_overlay_info(ovl, &info);
	if (r)
		return r;

	if (ovl->manager) {
		r = ovl->manager->apply(ovl->manager);
		if (r)
			return r;
	}

	return size;
}

static ssize_t overlay_pre_mult_alpha_en_show(struct owl_overlay *ovl,
		char *buf)
{
	struct owl_overlay_info info;

	ovl->get_overlay_info(ovl, &info);

	return snprintf(buf, PAGE_SIZE, "%d\n",
			info.pre_mult_alpha_en);
}

static ssize_t overlay_pre_mult_alpha_en_store(struct owl_overlay *ovl,
		const char *buf, size_t size)
{
	int r;
	u8 alpha;
	struct owl_overlay_info info;

	if ((ovl->caps & OWL_DSS_OVL_CAP_PRE_MULT_ALPHA) == 0)
		return -ENODEV;

	r = kstrtou8(buf, 0, &alpha);
	if (r)
		return r;

	ovl->get_overlay_info(ovl, &info);

	info.pre_mult_alpha_en = alpha;

	r = ovl->set_overlay_info(ovl, &info);
	if (r)
		return r;

	if (ovl->manager) {
		r = ovl->manager->apply(ovl->manager);
		if (r)
			return r;
	}

	return size;
}

static ssize_t overlay_zorder_show(struct owl_overlay *ovl, char *buf)
{
	struct owl_overlay_info info;

	ovl->get_overlay_info(ovl, &info);

	return snprintf(buf, PAGE_SIZE, "%d\n", info.zorder);
}

static ssize_t overlay_zorder_store(struct owl_overlay *ovl,
		const char *buf, size_t size)
{
	int r;
	u8 zorder;
	struct owl_overlay_info info;

	if ((ovl->caps & OWL_DSS_OVL_CAP_ZORDER) == 0)
		return -ENODEV;

	r = kstrtou8(buf, 0, &zorder);
	if (r)
		return r;

	ovl->get_overlay_info(ovl, &info);

	info.zorder = zorder;

	r = ovl->set_overlay_info(ovl, &info);
	if (r)
		return r;

	if (ovl->manager) {
		r = ovl->manager->apply(ovl->manager);
		if (r)
			return r;
	}

	return size;
}

struct overlay_attribute {
	struct attribute attr;
	ssize_t (*show)(struct owl_overlay *, char *);
	ssize_t	(*store)(struct owl_overlay *, const char *, size_t);
};

#define OVERLAY_ATTR(_name, _mode, _show, _store) \
	struct overlay_attribute overlay_attr_##_name = \
	__ATTR(_name, _mode, _show, _store)

static OVERLAY_ATTR(name, S_IRUGO, overlay_name_show, NULL);
static OVERLAY_ATTR(manager, S_IRUGO|S_IWUSR,
		overlay_manager_show, overlay_manager_store);
static OVERLAY_ATTR(input_size, S_IRUGO, overlay_input_size_show, NULL);
static OVERLAY_ATTR(screen_width, S_IRUGO, overlay_screen_width_show, NULL);
static OVERLAY_ATTR(position, S_IRUGO|S_IWUSR,
		overlay_position_show, overlay_position_store);
static OVERLAY_ATTR(output_size, S_IRUGO|S_IWUSR,
		overlay_output_size_show, overlay_output_size_store);
static OVERLAY_ATTR(enabled, S_IRUGO|S_IWUSR,
		overlay_enabled_show, overlay_enabled_store);
static OVERLAY_ATTR(global_alpha, S_IRUGO|S_IWUSR,
		overlay_global_alpha_show, overlay_global_alpha_store);
static OVERLAY_ATTR(pre_mult_alpha_en, S_IRUGO|S_IWUSR,
		overlay_pre_mult_alpha_en_show,
		overlay_pre_mult_alpha_en_store);
static OVERLAY_ATTR(zorder, S_IRUGO|S_IWUSR,
		overlay_zorder_show, overlay_zorder_store);

static struct attribute *overlay_sysfs_attrs[] = {
	&overlay_attr_name.attr,
	&overlay_attr_manager.attr,
	&overlay_attr_input_size.attr,
	&overlay_attr_screen_width.attr,
	&overlay_attr_position.attr,
	&overlay_attr_output_size.attr,
	&overlay_attr_enabled.attr,
	&overlay_attr_global_alpha.attr,
	&overlay_attr_pre_mult_alpha_en.attr,
	&overlay_attr_zorder.attr,
	NULL
};

static ssize_t overlay_attr_show(struct kobject *kobj, struct attribute *attr,
		char *buf)
{
	struct owl_overlay *overlay;
	struct overlay_attribute *overlay_attr;

	overlay = container_of(kobj, struct owl_overlay, kobj);
	overlay_attr = container_of(attr, struct overlay_attribute, attr);

	if (!overlay_attr->show)
		return -ENOENT;

	return overlay_attr->show(overlay, buf);
}

static ssize_t overlay_attr_store(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t size)
{
	struct owl_overlay *overlay;
	struct overlay_attribute *overlay_attr;

	overlay = container_of(kobj, struct owl_overlay, kobj);
	overlay_attr = container_of(attr, struct overlay_attribute, attr);

	if (!overlay_attr->store)
		return -ENOENT;

	return overlay_attr->store(overlay, buf, size);
}

static const struct sysfs_ops overlay_sysfs_ops = {
	.show = overlay_attr_show,
	.store = overlay_attr_store,
};

static struct kobj_type overlay_ktype = {
	.sysfs_ops = &overlay_sysfs_ops,
	.default_attrs = overlay_sysfs_attrs,
};

int owl_dss_get_num_overlays(void)
{
	return num_overlays;
}
EXPORT_SYMBOL(owl_dss_get_num_overlays);

struct owl_overlay *owl_dss_get_overlay(int num)
{
	if (num >= num_overlays)
		return NULL;

	return &overlays[num];
}
EXPORT_SYMBOL(owl_dss_get_overlay);



bool dss_ovl_is_enabled(struct owl_overlay *ovl)
{
	struct ovl_priv_data *op = get_ovl_priv(ovl);
	//unsigned long flags;
	bool e;

	//spin_lock_irqsave(&data_lock, flags);

	e = op->enabled;

	//spin_unlock_irqrestore(&data_lock, flags);

	return e;
}

static void dss_apply_ovl_enable(struct owl_overlay *ovl, bool enable)
{
	struct ovl_priv_data *op;

	op = get_ovl_priv(ovl);

	if (op->enabled == enable)
		return;

	op->enabled = enable;
	op->extra_info_dirty = true;
}



int dss_ovl_enable(struct owl_overlay *ovl)
{
	struct ovl_priv_data *op = get_ovl_priv(ovl);

	int r;

	if (op->enabled) {
		r = 0;
		goto err1;
	}

	if (ovl->manager == NULL || ovl->manager->device == NULL) {
		r = -EINVAL;
		DSSERR("dss_ovl_enable %d ovl->manager %p \n", ovl->id,ovl->manager);
		goto err1;
	}
	
	dss_apply_ovl_enable(ovl, true);

	return 0;
err1:

	return r;
}

int dss_ovl_disable(struct owl_overlay *ovl)
{
	struct ovl_priv_data *op = get_ovl_priv(ovl);
	//unsigned long flags;
	int r;

	//mutex_lock(&apply_lock);
	//printk("dss_ovl_disable (%s) op->enabled %d\n",ovl->name,op->enabled);
	if (!op->enabled) {
		r = 0;
		goto err;
	}

	if (ovl->manager == NULL || ovl->manager->device == NULL) {
		r = -EINVAL;
		goto err;
	}

	//spin_lock_irqsave(&data_lock, flags);

	dss_apply_ovl_enable(ovl, false);

	//spin_unlock_irqrestore(&data_lock, flags);

	//mutex_unlock(&apply_lock);
	//printk("dss_ovl_disable (%s) op->enabled %d ok\n",ovl->name,op->enabled);
	return 0;

err:
	//mutex_unlock(&apply_lock);
	return r;
}
int  dss_ovl_set_manager(struct owl_overlay *ovl,
		struct owl_overlay_manager *mgr)
{
	struct ovl_priv_data *op = get_ovl_priv(ovl);
	//unsigned long flags;
	int r;

	if (!mgr)
		return -EINVAL;

	mutex_lock(&mgr->apply_lock);

	if (ovl->manager) {
		DSSERR("overlay '%s' already has a manager '%s'\n",
				ovl->name, ovl->manager->name);
		r = -EINVAL;
		goto err;
	}

	//spin_lock_irqsave(&data_lock, flags);

	if (op->enabled) {
		//spin_unlock_irqrestore(&data_lock, flags);
		DSSERR("overlay has to be disabled to change the manager\n");
		r = -EINVAL;
		goto err;
	}

	op->channel = mgr->de_path_id;
	op->extra_info_dirty = true;

	ovl->manager = mgr;
	
	list_add_tail(&ovl->list, &mgr->overlays);

	//spin_unlock_irqrestore(&data_lock, flags);

	/* XXX: When there is an overlay on a DSI manual update display, and
	 * the overlay is first disabled, then moved to tv, and enabled, we
	 * seem to get SYNC_LOST_DIGIT error.
	 *
	 * Waiting doesn't seem to help, but updating the manual update display
	 * after disabling the overlay seems to fix this. This hints that the
	 * overlay is perhaps somehow tied to the LCD output until the output
	 * is updated.
	 *
	 * Userspace workaround for this is to update the LCD after disabling
	 * the overlay, but before moving the overlay to TV.
	 */

	mutex_unlock(&mgr->apply_lock);

	return 0;
err:
	mutex_unlock(&mgr->apply_lock);
	return r;
}

int dss_ovl_unset_manager(struct owl_overlay *ovl)
{
	struct ovl_priv_data *op = get_ovl_priv(ovl);
	struct mutex * apply_lock = NULL;
	int r;

	if (!ovl->manager) {
		DSSERR("failed to detach overlay: manager not set\n");
		r = -EINVAL;
		return r;
	}
	
	apply_lock = &ovl->manager->apply_lock;
	
	mutex_lock(apply_lock);

	if (op->enabled) {
		//spin_unlock_irqrestore(&data_lock, flags);
		DSSERR("overlay has to be disabled to unset the manager\n");
		r = -EINVAL;
		goto err;
	}
	
	de_ovl_enable(op->channel,ovl->id, false);
	
	op->channel = -1;

	ovl->manager = NULL;
	
	list_del(&ovl->list);

	mutex_unlock(apply_lock);

	return 0;
err:
	mutex_unlock(apply_lock);
	return r;
}
int dss_ovl_set_info(struct owl_overlay *ovl,
		struct owl_overlay_info *info)
{
	struct ovl_priv_data *op = get_ovl_priv(ovl);
	//unsigned long flags;
	int r;

	r = dss_ovl_simple_check(ovl, info);
	if (r)
		return r;

	//spin_lock_irqsave(&data_lock, flags);

	op->user_info = *info;
	op->user_info_dirty = true;

	//spin_unlock_irqrestore(&data_lock, flags);

	return 0;
}

void dss_ovl_get_info(struct owl_overlay *ovl,
		struct owl_overlay_info *info)
{
	struct ovl_priv_data *op = get_ovl_priv(ovl);
	//unsigned long flags;

	//spin_lock_irqsave(&data_lock, flags);

	*info = op->user_info;

	//spin_unlock_irqrestore(&data_lock, flags);
}

static int dss_ovl_apply_info(struct owl_overlay *ovl)
{
	struct ovl_priv_data *op;

	op = get_ovl_priv(ovl);
	DSSDBG("owl_dss_mgr_apply_ovl(%s),op->user_info_dirty %d\n", ovl->name,op->user_info_dirty);
	if (!op->user_info_dirty)
		return 0;

	op->user_info_dirty = false;
	op->info_dirty = true;
	op->info = op->user_info;

    return 0;
}

static int dss_ovl_write_regs(struct owl_overlay *ovl)
{
	struct ovl_priv_data *op = get_ovl_priv(ovl);
	struct owl_overlay_info *oi;
	bool ilace = false;
	int r;

	DSSDBG("dss_ovl_write_regs %d op->enabled %d op->info_dirty %d \n", ovl->id,op->enabled,op->info_dirty);

	if (!op->enabled || !op->info_dirty)
		goto dss_ovl_write_regs_extra;

	oi = &op->info;

	//ilace = ovl->manager->device->type == OWL_DISPLAY_TYPE_VENC;

	r = de_ovl_setup(op->channel,ovl->id, oi, ilace);
	
	if (r) {
		/*
		 * We can't do much here, as this function can be called from
		 * vsync interrupt.
		 */
		DSSERR("dispc_ovl_setup failed for ovl %d\n", ovl->id);

		/* This will leave fifo configurations in a nonoptimal state */
		op->enabled = false;
		de_ovl_enable(op->channel,ovl->id, false);
		return r;
	}
	
	op->info_dirty = false;
	
dss_ovl_write_regs_extra:
			
	if (!op->extra_info_dirty)
		return 0;

	/* note: write also when op->enabled == false, so that the ovl gets
	 * disabled */
	
	de_ovl_enable(op->channel,ovl->id, op->enabled);

	op->extra_info_dirty = false;
	
	return 0;
	
}

static int dss_ovl_enable_mmu(struct owl_overlay *ovl,bool enable)
{
	struct ovl_priv_data *op = get_ovl_priv(ovl);
	struct owl_overlay_info *oi;
	
	oi = &op->info;
	
	if(oi->enable_mmu == enable)
	{
		return 0;
	}
	oi->enable_mmu = enable;
	
	owl_de_mmu_enable(ovl->id, oi->enable_mmu);		
	
	return 0;
	
}
void dss_init_overlays(struct platform_device *pdev)
{
	int i, r;

	num_overlays = dss_feat_get_num_ovls();

	overlays = kzalloc(sizeof(struct owl_overlay) * num_overlays,
			GFP_KERNEL);

	BUG_ON(overlays == NULL);

	for (i = 0; i < num_overlays; ++i) {
		struct owl_overlay *ovl = &overlays[i];
		struct ovl_priv_data * op = get_ovl_priv(ovl);
		op->info.global_alpha = 255;
		switch (i) {
		case 0:
			ovl->name = "vid1";
			ovl->id = OWL_DSS_VIDEO1;
			op->info.zorder = 0;
			break;
		case 1:
			ovl->name = "vid2";
			ovl->id = OWL_DSS_VIDEO2;
			op->info.zorder =
				dss_has_feature(FEAT_ALPHA_FREE_ZORDER) ? 3 : 0;
			break;
		case 2:
			ovl->name = "vid3";
			ovl->id = OWL_DSS_VIDEO3;
			op->info.zorder =
				dss_has_feature(FEAT_ALPHA_FREE_ZORDER) ? 2 : 0;
			break;
		case 3:
			ovl->name = "vid4";
			ovl->id = OWL_DSS_VIDEO4;
			op->info.zorder =
				dss_has_feature(FEAT_ALPHA_FREE_ZORDER) ? 1 : 0;
			break;
		}
		op->user_info = op->info;
		ovl->is_enabled = &dss_ovl_is_enabled;
		ovl->enable = &dss_ovl_enable;
		ovl->disable = &dss_ovl_disable;
		ovl->set_manager = &dss_ovl_set_manager;
		ovl->unset_manager = &dss_ovl_unset_manager;
		ovl->set_overlay_info = &dss_ovl_set_info;
		ovl->get_overlay_info = &dss_ovl_get_info;
		ovl->apply_overlay_info = &dss_ovl_apply_info;
		ovl->write_hw_regs = &dss_ovl_write_regs;
		ovl->enable_overlay_mmu = &dss_ovl_enable_mmu;

		ovl->caps = dss_feat_get_overlay_caps(ovl->id);
		ovl->supported_modes = 
			dss_feat_get_supported_color_modes(ovl->id);

		r = kobject_init_and_add(&ovl->kobj, &overlay_ktype,
				&pdev->dev.kobj, "overlay%d", i);

		if (r)
			DSSERR("failed to create sysfs file\n");
	}
}
void dss_uninit_overlays(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < num_overlays; ++i) {
		struct owl_overlay *ovl = &overlays[i];

		kobject_del(&ovl->kobj);
		kobject_put(&ovl->kobj);
	}

	kfree(overlays);
	overlays = NULL;
	num_overlays = 0;
}

int dss_ovl_simple_check(struct owl_overlay *ovl,
		const struct owl_overlay_info *info)
{
	if (info->paddr == 0 && info->vaddr == 0 && info->buffer_id == 0) {
		DSSERR("check_overlay: paddr/vaddr/fd cannot be 0\n");
		return -EINVAL;
	}

	if ((ovl->caps & OWL_DSS_OVL_CAP_SCALE) == 0) {
		if (info->out_width != 0 && info->width != info->out_width) {
			DSSERR("check_overlay: overlay %d doesn't support "
					"scaling\n", ovl->id);
			return -EINVAL;
		}

		if (info->out_height != 0 && info->height != info->out_height) {
			DSSERR("check_overlay: overlay %d doesn't support "
					"scaling\n", ovl->id);
			return -EINVAL;
		}
	}

	if ((ovl->supported_modes & info->color_mode) == 0 && info->color_mode != 0) {
		DSSERR("check_overlay: overlay %d doesn't support mode %d\n",
				ovl->id, info->color_mode);
		return -EINVAL;
	}

	if (info->zorder >= owl_dss_get_num_overlays()) {
		DSSERR("check_overlay: zorder %d too high\n", info->zorder);
		return -EINVAL;
	}

	return 0;
}

int dss_ovl_check(struct owl_overlay *ovl,
		struct owl_overlay_info *info, struct owl_dss_device *dssdev)
{
	u16 outw, outh;
	u16 dw, dh;

	if (dssdev == NULL || dssdev->driver == NULL)
		return 0;

	dssdev->driver->get_resolution(dssdev, &dw, &dh);
	
	if(info->img_width == 0 || info->img_height == 0){
		info->img_width = info->width;
		info->img_height = info->height;
	}

	if ((ovl->caps & OWL_DSS_OVL_CAP_SCALE) == 0) {
		outw = info->width;
		outh = info->height;
	} else {
		if (info->out_width == 0)
			outw = info->width;
		else
			outw = info->out_width;

		if (info->out_height == 0)
			outh = info->height;
		else
			outh = info->out_height;
	}

	if (dw < info->pos_x + outw) {
		DSSDBG("overlay %d horizontally not inside the display area "
				"(%d + %d >= %d)\n",
				ovl->id, info->pos_x, outw, dw);
		return -EINVAL;
	}

	if (dh < info->pos_y + outh) {
		DSSDBG("overlay %d vertically not inside the display area "
				"(%d + %d >= %d)\n",
				ovl->id, info->pos_y, outh, dh);
		return -EINVAL;
	}
	
	if(dssdev->driver->get_effect_parameter){
		info->lightness = dssdev->driver->get_effect_parameter(dssdev,OWL_DSS_VIDEO_LIGHTNESS);
		info->saturation = dssdev->driver->get_effect_parameter(dssdev,OWL_DSS_VIDEO_SATURATION);
		info->contrast = dssdev->driver->get_effect_parameter(dssdev,OWL_DSS_VIDEO_CONSTRAST);
		DSSDBG("get_effect_parameter lightness %d saturation %d contrast %d\n",info->lightness,info->saturation,info->contrast);
	}else{
		info->lightness = DEF_LIGHTNESS;
		info->saturation = DEF_SATURATION;
		info->contrast = DEF_CONTRAST;			
	}
	return 0;
}
