/*
 * linux/drivers/video/owl/dss/manager.c
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

#define DSS_SUBSYS_NAME "MANAGER"

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>

#include <video/owldss.h>

#include "dss.h"
#include "de.h"
#include "dss_features.h"

static int num_managers;
static struct owl_overlay_manager *managers;

/* protects dss_data */
//static spinlock_t data_lock;

/* lock for blocking functions */
static DEFINE_MUTEX(apply_lock);

static ssize_t manager_name_show(struct owl_overlay_manager *mgr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", mgr->name);
}

static ssize_t manager_display_show(struct owl_overlay_manager *mgr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n",
			mgr->device ? mgr->device->name : "<none>");
}

static ssize_t manager_display_store(struct owl_overlay_manager *mgr,
		const char *buf, size_t size)
{
	int r = 0;
	size_t len = size;
	struct owl_dss_device *dssdev = NULL;

	int match(struct owl_dss_device *dssdev, void *data)
	{
		const char *str = data;
		return sysfs_streq(dssdev->name, str);
	}

	if (buf[size-1] == '\n')
		--len;

	if (len > 0)
		dssdev = owl_dss_find_device((void *)buf, match);

	if (len > 0 && dssdev == NULL)
		return -EINVAL;

	if (dssdev)
		DSSDBG("display %s found\n", dssdev->name);

	if (mgr->device) {
		r = mgr->unset_device(mgr);
		if (r) {
			DSSERR("failed to unset display\n");
			goto put_device;
		}
	}

	if (dssdev) {
		r = mgr->set_device(mgr, dssdev);
		if (r) {
			DSSERR("failed to set manager\n");
			goto put_device;
		}

		r = mgr->apply(mgr);
		if (r) {
			DSSERR("failed to apply dispc config\n");
			goto put_device;
		}
	}

put_device:
	if (dssdev)
		owl_dss_put_device(dssdev);

	return r ? r : size;
}

static ssize_t manager_default_color_show(struct owl_overlay_manager *mgr,
					  char *buf)
{
	struct owl_overlay_manager_info info;

	mgr->get_manager_info(mgr, &info);

	return snprintf(buf, PAGE_SIZE, "%#x\n", info.default_color);
}

static ssize_t manager_default_color_store(struct owl_overlay_manager *mgr,
					   const char *buf, size_t size)
{
	struct owl_overlay_manager_info info;
	u32 color;
	int r;

	r = kstrtouint(buf, 0, &color);
	if (r)
		return r;

	mgr->get_manager_info(mgr, &info);

	info.default_color = color;

	r = mgr->set_manager_info(mgr, &info);
	if (r)
		return r;

	r = mgr->apply(mgr);
	if (r)
		return r;

	return size;
}

static const char *trans_key_type_str[] = {
	"gfx-destination",
	"video-source",
};

static ssize_t manager_trans_key_type_show(struct owl_overlay_manager *mgr,
					   char *buf)
{
	enum owl_dss_trans_key_type key_type;
	struct owl_overlay_manager_info info;

	mgr->get_manager_info(mgr, &info);

	key_type = info.trans_key_type;
	BUG_ON(key_type >= ARRAY_SIZE(trans_key_type_str));

	return snprintf(buf, PAGE_SIZE, "%s\n", trans_key_type_str[key_type]);
}

static ssize_t manager_trans_key_type_store(struct owl_overlay_manager *mgr,
					    const char *buf, size_t size)
{
	enum owl_dss_trans_key_type key_type;
	struct owl_overlay_manager_info info;
	int r;

	for (key_type = OWL_DSS_COLOR_KEY_GFX_DST;
			key_type < ARRAY_SIZE(trans_key_type_str); key_type++) {
		if (sysfs_streq(buf, trans_key_type_str[key_type]))
			break;
	}

	if (key_type == ARRAY_SIZE(trans_key_type_str))
		return -EINVAL;

	mgr->get_manager_info(mgr, &info);

	info.trans_key_type = key_type;

	r = mgr->set_manager_info(mgr, &info);
	if (r)
		return r;

	r = mgr->apply(mgr);
	if (r)
		return r;

	return size;
}

static ssize_t manager_trans_key_value_show(struct owl_overlay_manager *mgr,
					    char *buf)
{
	struct owl_overlay_manager_info info;

	mgr->get_manager_info(mgr, &info);

	return snprintf(buf, PAGE_SIZE, "%#x\n", info.trans_key);
}

static ssize_t manager_trans_key_value_store(struct owl_overlay_manager *mgr,
					     const char *buf, size_t size)
{
	struct owl_overlay_manager_info info;
	u32 key_value;
	int r;

	r = kstrtouint(buf, 0, &key_value);
	if (r)
		return r;

	mgr->get_manager_info(mgr, &info);

	info.trans_key = key_value;

	r = mgr->set_manager_info(mgr, &info);
	if (r)
		return r;

	r = mgr->apply(mgr);
	if (r)
		return r;

	return size;
}

static ssize_t manager_trans_key_enabled_show(struct owl_overlay_manager *mgr,
					      char *buf)
{
	struct owl_overlay_manager_info info;

	mgr->get_manager_info(mgr, &info);

	return snprintf(buf, PAGE_SIZE, "%d\n", info.trans_enabled);
}

static ssize_t manager_trans_key_enabled_store(struct owl_overlay_manager *mgr,
					       const char *buf, size_t size)
{
	struct owl_overlay_manager_info info;
	bool enable;
	int r;

	r = strtobool(buf, &enable);
	if (r)
		return r;

	mgr->get_manager_info(mgr, &info);

	info.trans_enabled = enable;

	r = mgr->set_manager_info(mgr, &info);
	if (r)
		return r;

	r = mgr->apply(mgr);
	if (r)
		return r;

	return size;
}

static ssize_t manager_alpha_blending_enabled_show(
		struct owl_overlay_manager *mgr, char *buf)
{
	struct owl_overlay_manager_info info;

	mgr->get_manager_info(mgr, &info);

	WARN_ON(!dss_has_feature(FEAT_ALPHA_FIXED_ZORDER));

	return snprintf(buf, PAGE_SIZE, "%d\n",
		info.partial_alpha_enabled);
}

static ssize_t manager_alpha_blending_enabled_store(
		struct owl_overlay_manager *mgr,
		const char *buf, size_t size)
{
	struct owl_overlay_manager_info info;
	bool enable;
	int r;

	WARN_ON(!dss_has_feature(FEAT_ALPHA_FIXED_ZORDER));

	r = strtobool(buf, &enable);
	if (r)
		return r;

	mgr->get_manager_info(mgr, &info);

	info.partial_alpha_enabled = enable;

	r = mgr->set_manager_info(mgr, &info);
	if (r)
		return r;

	r = mgr->apply(mgr);
	if (r)
		return r;

	return size;
}

struct manager_attribute {
	struct attribute attr;
	ssize_t (*show)(struct owl_overlay_manager *, char *);
	ssize_t	(*store)(struct owl_overlay_manager *, const char *, size_t);
};

#define MANAGER_ATTR(_name, _mode, _show, _store) \
	struct manager_attribute manager_attr_##_name = \
	__ATTR(_name, _mode, _show, _store)

static MANAGER_ATTR(name, S_IRUGO, manager_name_show, NULL);
static MANAGER_ATTR(display, S_IRUGO|S_IWUSR,
		manager_display_show, manager_display_store);
static MANAGER_ATTR(default_color, S_IRUGO|S_IWUSR,
		manager_default_color_show, manager_default_color_store);
static MANAGER_ATTR(trans_key_type, S_IRUGO|S_IWUSR,
		manager_trans_key_type_show, manager_trans_key_type_store);
static MANAGER_ATTR(trans_key_value, S_IRUGO|S_IWUSR,
		manager_trans_key_value_show, manager_trans_key_value_store);
static MANAGER_ATTR(trans_key_enabled, S_IRUGO|S_IWUSR,
		manager_trans_key_enabled_show,
		manager_trans_key_enabled_store);
static MANAGER_ATTR(alpha_blending_enabled, S_IRUGO|S_IWUSR,
		manager_alpha_blending_enabled_show,
		manager_alpha_blending_enabled_store);


static struct attribute *manager_sysfs_attrs[] = {
	&manager_attr_name.attr,
	&manager_attr_display.attr,
	&manager_attr_default_color.attr,
	&manager_attr_trans_key_type.attr,
	&manager_attr_trans_key_value.attr,
	&manager_attr_trans_key_enabled.attr,
	&manager_attr_alpha_blending_enabled.attr,
	NULL
};

static ssize_t manager_attr_show(struct kobject *kobj, struct attribute *attr,
		char *buf)
{
	struct owl_overlay_manager *manager;
	struct manager_attribute *manager_attr;

	manager = container_of(kobj, struct owl_overlay_manager, kobj);
	manager_attr = container_of(attr, struct manager_attribute, attr);

	if (!manager_attr->show)
		return -ENOENT;

	return manager_attr->show(manager, buf);
}

static ssize_t manager_attr_store(struct kobject *kobj, struct attribute *attr,
		const char *buf, size_t size)
{
	struct owl_overlay_manager *manager;
	struct manager_attribute *manager_attr;

	manager = container_of(kobj, struct owl_overlay_manager, kobj);
	manager_attr = container_of(attr, struct manager_attribute, attr);

	if (!manager_attr->store)
		return -ENOENT;

	return manager_attr->store(manager, buf, size);
}

static const struct sysfs_ops manager_sysfs_ops = {
	.show = manager_attr_show,
	.store = manager_attr_store,
};

static struct kobj_type manager_ktype = {
	.sysfs_ops = &manager_sysfs_ops,
	.default_attrs = manager_sysfs_attrs,
};

int dss_mgr_wait_for_go(struct owl_overlay_manager *mgr)
{
	unsigned long timeout = msecs_to_jiffies(500);
	struct mgr_priv_data * mp = NULL;
	enum de_irq_type irq;
	int rc = 0;
	
	if(!mgr){
		DSSERR("dss_mgr_wait_for_vsync but mgr is null\n",mgr->name);
		return -ENOENT;
	}	
	mp = get_mgr_priv(mgr);
	
	if(!mp->enabled ||!mgr->device){
		return 0;
	}
	
	switch(mgr->device->type){
		case OWL_DISPLAY_TYPE_LCD:
			irq = DE_IRQ_LCD_PRE;
			break;
		case OWL_DISPLAY_TYPE_DSI:
			irq = DE_IRQ_DSI_PRE;
			break;
		case OWL_DISPLAY_TYPE_CVBS:
			irq = DE_IRQ_CVBS_PRE;
			break;
		case OWL_DISPLAY_TYPE_HDMI:
			irq = DE_IRQ_HDMI_PRE;
			break;	
		case OWL_DISPLAY_TYPE_EDP:
		case OWL_DISPLAY_TYPE_YPBPR:
		default:
			DSSERR("display type ( %d )is not support \n",mgr->device->type);
		    return -ENOENT;						
	}
	rc = de_wait_for_irq_interruptible_timeout(irq, mgr->de_path_id,timeout);
	return rc ;
}

static int dss_mgr_wait_for_vsync(struct owl_overlay_manager *mgr,long long i64TimeStamp)
{
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
	const int wait_timeout = msecs_to_jiffies(500);	/* 16ms, TODO */
	
	int rc;
	
	if (mp->hw_sync_enable) {
		atomic_set(&mp->vsync_avail, 0);
		rc = wait_event_timeout(mp->wait_vsync,
				atomic_read(&mp->vsync_avail), wait_timeout);
		if (!rc)
			DSSDBG("%s timeout\n", __func__);
	}

	return 0 ;
}

int dss_mgr_set_device(struct owl_overlay_manager *mgr,
		struct owl_dss_device *dssdev)
{
	int r;

	mutex_lock(&apply_lock);

	if (dssdev->manager) {
		DSSERR("display '%s' already has a manager '%s'\n",
			       dssdev->name, dssdev->manager->name);
		r = -EINVAL;
		goto err;
	}

	if ((mgr->supported_displays & dssdev->type) == 0) {
		DSSERR("display '%s' does not support manager '%s'\n",
			       dssdev->name, mgr->name);
		r = -EINVAL;
		goto err;
	}

	dssdev->manager = mgr;
	mgr->device = dssdev;

	mutex_unlock(&apply_lock);

	return 0;
err:
	mutex_unlock(&apply_lock);
	return r;
}

int dss_mgr_unset_device(struct owl_overlay_manager *mgr)
{
	int r;

	mutex_lock(&apply_lock);

	if (!mgr->device) {
		DSSERR("failed to unset display, display not set.\n");
		r = -EINVAL;
		goto err;
	}

	/*
	 * Don't allow currently enabled displays to have the overlay manager
	 * pulled out from underneath them
	 */
	if (mgr->device->state != OWL_DSS_DISPLAY_DISABLED) {
		r = -EINVAL;
		goto err;
	}

	mgr->device->manager = NULL;
	mgr->device = NULL;

	mutex_unlock(&apply_lock);

	return 0;
err:
	mutex_unlock(&apply_lock);
	return r;
}

int dss_mgr_set_info(struct owl_overlay_manager *mgr,
		struct owl_overlay_manager_info *info)
{
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
	//unsigned long flags;
	int r;

	r = dss_mgr_simple_check(mgr, info);
	if (r)
		return r;

	//spin_lock_irqsave(&data_lock, flags);

	mp->user_info = *info;
	mp->user_info_dirty = true;

	//spin_unlock_irqrestore(&data_lock, flags);

	return 0;
}

void dss_mgr_get_info(struct owl_overlay_manager *mgr,
		struct owl_overlay_manager_info *info)
{
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
	//unsigned long flags;

	//spin_lock_irqsave(&data_lock, flags);

	*info = mp->user_info;

	//spin_unlock_irqrestore(&data_lock, flags);
}

int dss_cursor_set_info(struct owl_overlay_manager *mgr, struct owl_cursor_info *info)
{
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
	int r;

	mp->user_cursor = *info;
	mp->user_cursor_dirty = true;

	return 0;
}

void dss_cursor_get_info(struct owl_overlay_manager *mgr,struct owl_cursor_info *info)
{
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
	*info = mp->cursor;	
}

static void dss_mgr_write_regs(struct owl_overlay_manager *mgr)
{
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
	struct owl_overlay *ovl;
	struct owl_dss_device * dssdev = mgr->device;
	
	DSSDBG("dss_mgr_write_regs %d mp->enabled %d dssdev->type %d\n",
		mgr->id, mp->enabled, dssdev->type);

	if (!mp->enabled)
		return;

	WARN_ON(mp->busy);

	/* Commit overlay settings */
	DSSDBG("Commit overlay settings \n");
	list_for_each_entry(ovl, &mgr->overlays, list) {
		ovl->write_hw_regs(ovl);
	}
	
	/* commit cursor info*/
	if (mp->cursor_dirty) {
		de_mgr_cursor_setup(mgr->de_path_id, &mp->cursor);
	}
	
	de_mgr_set_path_size(mgr->de_path_id, mp->info.out_width, mp->info.out_height);
	
	if (dssdev){
		de_mgr_set_device_type(mgr->de_path_id,dssdev->type);
	}	
	
	if (mp->info_dirty) {
		DSSDBG("de_mgr_setup mgr->id %d mp->info %p\n",
			mgr->id, &mp->info);

		de_mgr_setup(mgr->de_path_id, &mp->info);

		mp->info_dirty = false;
	}
	DSSDBG("dss_mgr_write_regs end \n");
}
static int dss_check_settings_low(struct owl_overlay_manager *mgr, struct owl_dss_device *dssdev, bool applying)
{
	struct owl_overlay_info *oi;
	struct owl_overlay_manager_info *mi;
	struct owl_overlay *ovl;
	struct owl_overlay_info *ois[MAX_DSS_OVERLAYS];
	struct ovl_priv_data *op;
	struct mgr_priv_data *mp;

	mp = get_mgr_priv(mgr);

	if (applying && mp->user_info_dirty)
		mi = &mp->user_info;
	else
		mi = &mp->info;

	/* collect the infos to be tested into the array */
	list_for_each_entry(ovl, &mgr->overlays, list) {
		op = get_ovl_priv(ovl);

		if (!op->enabled && !op->enabling)
			oi = NULL;
		else if (applying && op->user_info_dirty)
			oi = &op->user_info;
		else
			oi = &op->info;

		ois[ovl->id] = oi;
	}

	return dss_mgr_check(mgr, dssdev, mi, ois);
}
static void dss_write_regs(struct owl_overlay_manager *mgr)
{
	struct mgr_priv_data *mp;
	int r;		
	mp = get_mgr_priv(mgr);

	DSSDBG("mp->enabled  %d mp->busy %d mgr->device %p\n",
		mp->enabled, mp->busy, mgr->device);
	if (!mp || !mp->enabled || mp->busy)
		return;
		
	DSSDBG("dss_check_settings %d \n", mgr->id);
	r = dss_check_settings_low(mgr, mgr->device, false);
	if (r) {
		DSSERR("cannot write registers for manager %s: "
				"illegal configuration\n", mgr->name);
		return;
	}
	DSSDBG("dss_mgr_write_regs %d \n", mgr->id);
	dss_mgr_write_regs(mgr);

	DSSDBG("dss_write_regs ok \n");
}

static bool is_fb_reserved_memory_freed = false;
extern void free_fb_reserved_memory(void);
static int dss_mgr_check_and_enable_mmu(struct owl_overlay_manager *mgr)
{
	const int num_ovls = owl_dss_get_num_overlays();
	int i = 0;
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
    if(!de_is_vb_valid(mgr->id, mgr->device->type))
    {
        return 0;
    }
	
    if(!mp->enabled || mp->mmu_state != MMU_STATE_PRE_ENABLE || !mp->shadow_info_dirty)
    {
    	return 0;
    }
    
    de_mgr_go(mgr->de_path_id);
    //if we enable mmu, we set all overlay mmu enable acquiescently!
	for (i = 0; i < num_ovls; i++) {
		struct owl_overlay *ovl = owl_dss_get_overlay(i);
		ovl->enable_overlay_mmu(ovl,true);			
	}
	mp->shadow_info_dirty = false;	
	mp->mmu_state = MMU_STATE_ENABLED;
	if (unlikely(!is_fb_reserved_memory_freed)) {
		free_fb_reserved_memory();
		is_fb_reserved_memory_freed = true;
		//printk("%s, fb reserved memory is freed.\n", __func__);
	}
	return 0;
}

static void dss_mgr_send_vsync_work(struct work_struct *work)
{
	struct mgr_priv_data *mp;
	struct owl_overlay_manager *mgr;
    
	mp = container_of(work, struct mgr_priv_data, vsync_work);
	mgr= container_of(mp, struct owl_overlay_manager, priv_data);
	
    	
	/* 
	 * update gamma info if need,
	 * no need care about synchronization problem,
	 * every thing will be okay in the next vsync.
	 *
	 * NOTE: should check VB status before set gamma table,
	 * if now is not in VB, skip this setting, or will blur the screen
	 */
	
	if (mgr->gamma_info_dirty && de_is_vb_valid(mgr->id, mgr->device->type)) {
		if (mgr->gamma_info.enabled) {
			de_set_gamma_table(mgr->de_path_id, (u32 *)mgr->gamma_info.value);
		}
		de_enable_gamma_table(mgr->de_path_id, mgr->gamma_info.enabled);
		mgr->gamma_info_dirty = false;
	} else if (mgr->gamma_info_dirty) {
		//DSSINFO("gamma lost because of VB invalid\n");
	}
}

static void dss_mgr_vsync_irq_handler(enum de_irq_type irq, void *data)
{
	struct owl_overlay_manager *mgr = (struct owl_overlay_manager *)data;
	
	struct mgr_priv_data * mp = get_mgr_priv(mgr);
	
	dss_mgr_check_and_enable_mmu(mgr);
	
	mp->timestamp = ktime_get();

	atomic_set(&mp->vsync_avail, 1);
	wake_up(&mp->wait_vsync);
	
	trace_vsync_point(0, ktime_to_ns(mp->timestamp));
	
	schedule_work(&mp->vsync_work);
}


static void dss_mgr_register_vsync_isr(struct owl_overlay_manager *mgr)
{
	enum de_irq_type irq;
	
	int r;
	
	if(mgr->device){
		irq = de_mgr_get_vsync_irq(mgr->device->type);
	}else{
		DSSERR("dss_mgr_register_vsync_isr  mgr->device  is NULL\n");
		irq = DE_IRQ_LCD_PRE;
	}
	r = owl_de_register_isr(irq, dss_mgr_vsync_irq_handler, mgr);
	
	WARN_ON(r);
}

static void dss_mgr_unregister_vsync_isr(struct owl_overlay_manager *mgr)
{
	enum de_irq_type irq;
	
	int r;

	if(mgr->device){
		irq = de_mgr_get_vsync_irq(mgr->device->type); 
	}else{
		DSSERR("dss_mgr_unregister_vsync_isr  mgr->device  is NULL\n");
		irq = DE_IRQ_LCD_PRE;
	}
	r = owl_de_unregister_isr(irq, dss_mgr_vsync_irq_handler, mgr);
	
	WARN_ON(r);
}

int dss_mgr_enable_hw_vsync(struct owl_overlay_manager *mgr, bool enable, struct device *dev)
{
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
	
	if (!mp)
		return -1;
	
	if (mp->hw_sync_enable == enable)
		return 0;
	
	mp->dev = dev;
	
	if (enable) {
		dss_mgr_register_vsync_isr(mgr);
		mp->hw_sync_enable = enable;
	} else {
		dss_mgr_unregister_vsync_isr(mgr);
		mp->hw_sync_enable = enable;

		atomic_set(&mp->vsync_avail, 1);
		wake_up(&mp->wait_vsync);
	}

	return 0;
}

static int dss_mgr_set_mmu_state(struct owl_overlay_manager *mgr, int state)
{
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
	
	if (!mp)
		return -1;
	
	if(mp->mmu_state < state)
	{
		mp->mmu_state = state;
	}
	
	return 0;
}

static void dss_set_go_bits(struct owl_overlay_manager *mgr)
{
	struct mgr_priv_data *mp;
	mp = get_mgr_priv(mgr);
	DSSDBG("dss_set_go_bits ~~~ %d mp->enabled %d mp->busy %d \n ",
		mgr->id, mp->enabled, mp->busy);
	if (!mp->enabled || mp->busy)
		return;

	mp->busy = true;
	
    // if will enable mmu, we need set fcr when mmu enable ,make sure in on vb single.
    if(mp->mmu_state != MMU_STATE_PRE_ENABLE)
    {
        de_mgr_go(mgr->id);
        mp->shadow_info_dirty = false;
    }
    else
    {
    	mp->shadow_info_dirty = true;
    }
						
	mp->busy = false;

}


static int dss_mgr_apply_mgr_info(struct owl_overlay_manager *mgr)
{
	struct mgr_priv_data *mp;
    
	mp = get_mgr_priv(mgr);
	DSSDBG("owl_dss_mgr_apply_mgr(%s),mp->user_info_dirty %d mp->user_cursor_dirty %d\n", mgr->name,mp->user_info_dirty,mp->user_cursor_dirty);
	
	if(mp->user_cursor_dirty)
	{
		mp->user_cursor_dirty = false;
		mp->cursor_dirty = true;
		mp->cursor = mp->user_cursor;
	}
	
	if (!mp->user_info_dirty)
		return 0;

	mp->user_info_dirty = false;
	mp->info_dirty = true;
	mp->info = mp->user_info;
	
	

    return 0;
}

static int owl_dss_mgr_apply(struct owl_overlay_manager *mgr)
{
	//unsigned long flags;
	struct owl_overlay *ovl;
	struct mgr_priv_data *mp;
	int r;
	
	DSSDBG("owl_dss_mgr_apply(%s)\n", mgr->name);
	mp = get_mgr_priv(mgr);
	
	if(!mp->enabled)
	{
		return 0;
	}
	mutex_lock(&mgr->apply_lock);

	r = dss_check_settings_low(mgr,  mgr->device, true);
	
	if (r) {
		mutex_unlock(&mgr->apply_lock);
		DSSDBG("failed to apply settings: illegal configuration.\n");
		return r;
	}

	/* Configure overlays */
	list_for_each_entry(ovl, &mgr->overlays, list)
		ovl->apply_overlay_info(ovl);

	/* Configure manager */
	mgr->apply_manager_info(mgr);

	dss_write_regs(mgr);
	DSSDBG("dss_write_regs ok  \n");
	dss_set_go_bits(mgr);	
	DSSDBG("dss_set_go_bits 0k \n");
	mutex_unlock(&mgr->apply_lock);
	return 0;
}


int dss_init_overlay_managers(struct platform_device *pdev)
{
	int i, r;

	num_managers = dss_feat_get_num_mgrs();

	managers = kzalloc(sizeof(struct owl_overlay_manager) * num_managers,
			GFP_KERNEL);

	BUG_ON(managers == NULL);

	for (i = 0; i < num_managers; ++i) {
		struct owl_overlay_manager *mgr = &managers[i];
		struct mgr_priv_data *mp = get_mgr_priv(mgr);
		switch (i) {
		case 0:
			mgr->name = "primary";
			mgr->id = OWL_DSS_OVL_MGR_PRIMARY;
			mgr->de_path_id = OWL_DSS_PATH1_ID;
			break;
		case 1:
			mgr->name = "external";
			mgr->id = OWL_DSS_OVL_MGR_EXTERNAL;
			mgr->de_path_id = OWL_DSS_PATH2_ID;
			break;
		}		
		mgr->set_device = &dss_mgr_set_device;
		mgr->unset_device = &dss_mgr_unset_device;
		mgr->apply = &owl_dss_mgr_apply;
		mgr->set_manager_info = &dss_mgr_set_info;
		mgr->get_manager_info = &dss_mgr_get_info;
		mgr->apply_manager_info = &dss_mgr_apply_mgr_info;
		
		mgr->set_cursor_info = &dss_cursor_set_info;
		mgr->get_cursor_info = &dss_cursor_get_info;
		
		mgr->enable_hw_vsync = &dss_mgr_enable_hw_vsync;
		mgr->set_mmu_state = &dss_mgr_set_mmu_state;

		mgr->wait_for_go = &dss_mgr_wait_for_go;
		mgr->wait_for_vsync = &dss_mgr_wait_for_vsync;

		mgr->caps = 0;
		mgr->supported_displays =
			dss_feat_get_supported_displays(mgr->id);
			
		mutex_init(&mgr->apply_lock);
		
		mp->hw_sync_enable =false;
		
		mp->mmu_state = MMU_STATE_NO_ENABLE;
		
		mp->shadow_info_dirty = false;
		
		atomic_set(&mp->vsync_avail, 0);
		init_waitqueue_head(&mp->wait_vsync);
		
		INIT_WORK(&mp->vsync_work,dss_mgr_send_vsync_work);
		
		INIT_LIST_HEAD(&mgr->overlays);

		r = kobject_init_and_add(&mgr->kobj, &manager_ktype,
				&pdev->dev.kobj, "manager%d", i);

		if (r)
			DSSERR("failed to create sysfs file\n");
	}

	return 0;
}

void dss_uninit_overlay_managers(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < num_managers; ++i) {
		struct owl_overlay_manager *mgr = &managers[i];

		kobject_del(&mgr->kobj);
		kobject_put(&mgr->kobj);
	}

	kfree(managers);
	managers = NULL;
	num_managers = 0;
}

int owl_dss_get_num_overlay_managers(void)
{
	return num_managers;
}
EXPORT_SYMBOL(owl_dss_get_num_overlay_managers);

struct owl_overlay_manager *owl_dss_get_overlay_manager(int num)
{
	if (num >= num_managers)
		return NULL;

	return &managers[num];
}
EXPORT_SYMBOL(owl_dss_get_overlay_manager);

int dss_mgr_simple_check(struct owl_overlay_manager *mgr,
		const struct owl_overlay_manager_info *info)
{
	if (dss_has_feature(FEAT_ALPHA_FIXED_ZORDER)) {
		/*
		 * OWL3 supports only graphics source transparency color key
		 * and alpha blending simultaneously. See TRM 15.4.2.4.2.2
		 * Alpha Mode.
		 */
		if (info->partial_alpha_enabled && info->trans_enabled
			&& info->trans_key_type != OWL_DSS_COLOR_KEY_GFX_DST) {
			DSSERR("check_manager: illegal transparency key\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int dss_mgr_check_zorder(struct owl_overlay_manager *mgr,
		struct owl_overlay_info **overlay_infos)
{
	struct owl_overlay *ovl1, *ovl2;
	struct owl_overlay_info *info1, *info2;

	list_for_each_entry(ovl1, &mgr->overlays, list) {
		info1 = overlay_infos[ovl1->id];

		if (info1 == NULL)
			continue;

		list_for_each_entry(ovl2, &mgr->overlays, list) {
			if (ovl1 == ovl2)
				continue;

			info2 = overlay_infos[ovl2->id];

			if (info2 == NULL)
				continue;

			if (info1->zorder == info2->zorder) {
				DSSERR("overlays %d and %d have the same "
						"zorder %d\n",
					ovl1->id, ovl2->id, info1->zorder);
				return -EINVAL;
			}
		}
	}

	return 0;
}
static int dss_mgr_check_display_size(struct owl_overlay_manager_info *info ,struct owl_dss_device *dssdev)
{
	u16 dw, dh;
	
	if(dssdev == NULL)
		return  0;
		
	dssdev->driver->get_resolution(dssdev, &dw, &dh);
	
	//there need check the dw and dh 
	info->out_width = dw;
	info->out_height = dh;
	return 0;
}

int dss_mgr_check(struct owl_overlay_manager *mgr,
		struct owl_dss_device *dssdev,
		struct owl_overlay_manager_info *info,
		struct owl_overlay_info **overlay_infos)
{
	struct owl_overlay *ovl;
	int r;

	if (dss_has_feature(FEAT_ALPHA_FREE_ZORDER)) {
		r = dss_mgr_check_zorder(mgr, overlay_infos);
		if (r)
			return r;
	}

	list_for_each_entry(ovl, &mgr->overlays, list) {
		struct owl_overlay_info *oi;
		int r;

		oi = overlay_infos[ovl->id];

		if (oi == NULL)
			continue;

		r = dss_ovl_check(ovl, oi, dssdev);
		if (r)
			return r;
	}
	
	r = dss_mgr_check_display_size(info,dssdev);
	
	if (r)
		return r;

	return 0;
}

int dss_mgr_enable(struct owl_overlay_manager *mgr)
{
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
	//unsigned long flags;
	u16 dw = 0 ;
	u16 dh = 0 ;

	mutex_lock(&apply_lock);

	if (mp->enabled)
		goto out;
	
	//spin_lock_irqsave(&data_lock, flags);

	mp->enabled = true;
	
	if(dss_check_channel_boot_inited(mgr->de_path_id))
	{
		goto out;
	}

	//spin_unlock_irqrestore(&data_lock, flags);
	if(mgr->device != NULL && mgr->device->driver != NULL){
		mgr->device->driver->get_resolution(mgr->device, &dw, &dh);
	}
	de_mgr_set_path_size(mgr->de_path_id,dw,dh);		
	de_mgr_enable(mgr->de_path_id, true);
	dss_set_go_bits(mgr);
out:
	mutex_unlock(&apply_lock);

	return 0;
}
EXPORT_SYMBOL(dss_mgr_enable);
void dss_mgr_disable(struct owl_overlay_manager *mgr)
{
	struct mgr_priv_data *mp = get_mgr_priv(mgr);
	//unsigned long flags;
	mutex_lock(&apply_lock);

	if (!mp->enabled)
		goto out;

	de_mgr_enable(mgr->de_path_id, false);

	//spin_lock_irqsave(&data_lock, flags);

	mp->enabled = false;

	//spin_unlock_irqrestore(&data_lock, flags);

out:
	mutex_unlock(&apply_lock);
}
EXPORT_SYMBOL(dss_mgr_disable);


void dss_mgr_set_gamma_table(enum owl_de_path_id channel,
				struct owl_gamma_info *gamma_info)
{
	struct owl_overlay_manager *mgr = owl_dss_get_overlay_manager(channel);		

	mgr->gamma_info = *gamma_info;
	mgr->gamma_info_dirty = true;
}
EXPORT_SYMBOL(dss_mgr_set_gamma_table);

/* just return the backup, do not read from register */
void dss_mgr_get_gamma_table(enum owl_de_path_id channel,
				struct owl_gamma_info *gamma_info)
{
	struct owl_overlay_manager *mgr = owl_dss_get_overlay_manager(channel);		

	*gamma_info = mgr->gamma_info;
}
EXPORT_SYMBOL(dss_mgr_get_gamma_table);


bool dss_check_channel_boot_inited(enum owl_de_path_id channel)
{
	return de_channel_check_boot_inited(channel);
}
EXPORT_SYMBOL(dss_check_channel_boot_inited);
