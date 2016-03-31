/*
 * linux/drivers/video/owl/owlfb-sysfs.c
 *
 * Copyright (C) 2014 Actions Corporation
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
#include <mach/bootdev.h>
#include <linux/fb.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/mm.h>

#include <video/owldss.h>
#include <video/owlfb.h>

#include "owlfb.h"

extern int dss_mgr_enable(struct owl_overlay_manager *mgr);
extern int dss_mgr_disable(struct owl_overlay_manager *mgr);

static ssize_t show_mirror_to_hdmi(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;

	return snprintf(buf, PAGE_SIZE, "%d\n", fbdev->mirror_fb_id);
}

static ssize_t store_mirror_to_hdmi(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owl_overlay *ovl= NULL;
	bool mirror_to_hdmi;
	int r;
	int i = 0;
	int vid = 1;
	
	struct owl_overlay_manager *external_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_EXTERNAL);
	struct owl_dss_device *dssdev = external_mgr->device;
	
	if (owl_get_boot_mode() == OWL_BOOT_MODE_UPGRADE || dssdev == NULL) {
		printk("upgrade process hdmi disabled!!\n");
		return -ENODEV;
	}
	
	r = strtobool(buf, &mirror_to_hdmi);
	if (r)
		return r;
	if(ofbi->id != 0)
	{
		return r;
	}

	if (!lock_fb_info(fbi))
		return -ENODEV;

	fbdev->mirror_fb_id = mirror_to_hdmi;
	
	owlfb_get_mem_region(ofbi->region);
	DBG("store_mirror_to_hdmi %d\n",mirror_to_hdmi);
	if(mirror_to_hdmi){
		external_mgr->mirror_context = true;
		if(dssdev->driver->get_cable_status){
			if(dssdev->driver->get_cable_status(dssdev)){
				dssdev->driver->get_vid(dssdev,&vid);
				dssdev->driver->set_vid(dssdev,vid);		
				dssdev->driver->enable(dssdev);	
			}
		}else{
			if(dssdev->driver->get_vid)
			{
				dssdev->driver->get_vid(dssdev,&vid);
			}
			if(dssdev->driver->set_vid)
			{
				dssdev->driver->set_vid(dssdev,vid);
			}
			if(dssdev->driver->enable)
			{
				dssdev->driver->enable(dssdev);
			}
		}		
		external_mgr->link_fbi	= fbi;
		
    }else{
    	
    	dssdev->driver->disable(dssdev);
    	external_mgr->mirror_context = false;
    	external_mgr->link_fbi	= NULL;
    }   
    
	r = owlfb_apply_changes(fbi, 0);
	if (r)
		goto out;

	r = count;
out:
	owlfb_put_mem_region(ofbi->region);

	unlock_fb_info(fbi);

	return r;
}

static ssize_t show_overlays(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	ssize_t l = 0;
	int t;

	if (!lock_fb_info(fbi))
		return -ENODEV;
	owlfb_lock(fbdev);

	for (t = 0; t < ofbi->num_overlays; t++) {
		struct owl_overlay *ovl = ofbi->overlays[t];
		int ovlnum;

		for (ovlnum = 0; ovlnum < fbdev->num_overlays; ++ovlnum)
			if (ovl == fbdev->overlays[ovlnum])
				break;

		l += snprintf(buf + l, PAGE_SIZE - l, "%s%d",
				t == 0 ? "" : ",", ovlnum);
	}

	l += snprintf(buf + l, PAGE_SIZE - l, "\n");

	owlfb_unlock(fbdev);
	unlock_fb_info(fbi);

	return l;
}

static struct owlfb_info *get_overlay_fb(struct owlfb_device *fbdev,
		struct owl_overlay *ovl)
{
	int i, t;

	for (i = 0; i < fbdev->num_fbs; i++) {
		struct owlfb_info *ofbi = FB2OFB(fbdev->fbs[i]);

		for (t = 0; t < ofbi->num_overlays; t++) {
			if (ofbi->overlays[t] == ovl)
				return ofbi;
		}
	}

	return NULL;
}

static ssize_t store_overlays(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owl_overlay *ovls[OWLFB_MAX_OVL_PER_FB];
	struct owl_overlay *ovl;
	int num_ovls, r, i;
	int len;
	bool added = false;

	num_ovls = 0;

	len = strlen(buf);
	if (buf[len - 1] == '\n')
		len = len - 1;

	if (!lock_fb_info(fbi))
		return -ENODEV;
	owlfb_lock(fbdev);

	if (len > 0) {
		char *p = (char *)buf;
		int ovlnum;

		while (p < buf + len) {
			int found;
			if (num_ovls == OWLFB_MAX_OVL_PER_FB) {
				r = -EINVAL;
				goto out;
			}

			ovlnum = simple_strtoul(p, &p, 0);
			if (ovlnum > fbdev->num_overlays) {
				r = -EINVAL;
				goto out;
			}

			found = 0;
			for (i = 0; i < num_ovls; ++i) {
				if (ovls[i] == fbdev->overlays[ovlnum]) {
					found = 1;
					break;
				}
			}

			if (!found)
				ovls[num_ovls++] = fbdev->overlays[ovlnum];

			p++;
		}
	}

	for (i = 0; i < num_ovls; ++i) {
		struct owlfb_info *ofbi2 = get_overlay_fb(fbdev, ovls[i]);
		if (ofbi2 && ofbi2 != ofbi) {
			dev_err(fbdev->dev, "overlay already in use\n");
			r = -EINVAL;
			goto out;
		}
	}

	/* detach unused overlays */
	for (i = 0; i < ofbi->num_overlays; ++i) {
		int t, found;

		ovl = ofbi->overlays[i];

		found = 0;

		for (t = 0; t < num_ovls; ++t) {
			if (ovl == ovls[t]) {
				found = 1;
				break;
			}
		}

		if (found)
			continue;

		DBG("detaching %d\n", ofbi->overlays[i]->id);

		owlfb_get_mem_region(ofbi->region);

		owlfb_overlay_enable(ovl, 0);

		if (ovl->manager)
			ovl->manager->apply(ovl->manager);

		owlfb_put_mem_region(ofbi->region);

		for (t = i + 1; t < ofbi->num_overlays; t++) {
			ofbi->rotation[t-1] = ofbi->rotation[t];
			ofbi->overlays[t-1] = ofbi->overlays[t];
		}

		ofbi->num_overlays--;
		i--;
	}

	for (i = 0; i < num_ovls; ++i) {
		int t, found;

		ovl = ovls[i];

		found = 0;

		for (t = 0; t < ofbi->num_overlays; ++t) {
			if (ovl == ofbi->overlays[t]) {
				found = 1;
				break;
			}
		}

		if (found)
			continue;
		ofbi->rotation[ofbi->num_overlays] = 0;
		ofbi->overlays[ofbi->num_overlays++] = ovl;

		added = true;
	}

	if (added) {
		owlfb_get_mem_region(ofbi->region);

		r = owlfb_apply_changes(fbi, 0);

		owlfb_put_mem_region(ofbi->region);

		if (r)
			goto out;
	}

	r = count;
out:
	owlfb_unlock(fbdev);
	unlock_fb_info(fbi);

	return r;
}

static ssize_t show_overscan(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct owl_dss_device *display = fb2display(fbi);
	u16 overscan_w ,overscan_h;
	ssize_t l = 0;
	int t;

	if (!lock_fb_info(fbi))
		return -ENODEV;
	
	if(display != NULL && display->driver != NULL && display->driver->get_over_scan){
		display->driver->get_over_scan(display,&overscan_w ,&overscan_h);
		l += snprintf(buf + l, PAGE_SIZE - l, "%d,%d\n",overscan_w,overscan_h);
	}else{
		l += snprintf(buf + l, PAGE_SIZE - l, "%d,%d\n",0,0);
	}
	
	unlock_fb_info(fbi);

	return l;
}

static ssize_t store_overscan(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct owl_dss_device *display = fb2display(fbi);
	struct owlfb_info *ofbi = FB2OFB(fbi);
	int r = 0;
	u16 overscan_w ,overscan_h;

	if (!lock_fb_info(fbi))
		return -ENODEV;
	
	sscanf(buf,"%d,%d",&overscan_w,&overscan_h);
	
	if(display != NULL && display->driver != NULL && display->driver->set_over_scan){
		display->driver->set_over_scan(display,overscan_w ,overscan_h);
	}
	
	owlfb_get_mem_region(ofbi->region);
	
	owlfb_apply_changes(fbi, 0);
	
	owlfb_put_mem_region(ofbi->region);
out:
	unlock_fb_info(fbi);

	return count;
}

static ssize_t show_size(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct owlfb_info *ofbi = FB2OFB(fbi);

	return snprintf(buf, PAGE_SIZE, "%lu\n", ofbi->region->size);
}

static ssize_t store_size(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owlfb_mem_region *rg;
	unsigned long size;
	int r;
	int i;

	r = kstrtoul(buf, 0, &size);
	if (r)
		return r;

	size = PAGE_ALIGN(size);

	if (!lock_fb_info(fbi))
		return -ENODEV;

	rg = ofbi->region;

	down_write_nested(&rg->lock, rg->id);
	atomic_inc(&rg->lock_count);

	if (atomic_read(&rg->map_count)) {
		r = -EBUSY;
		goto out;
	}

	for (i = 0; i < fbdev->num_fbs; i++) {
		struct owlfb_info *ofbi2 = FB2OFB(fbdev->fbs[i]);
		int j;

		if (ofbi2->region != rg)
			continue;

		for (j = 0; j < ofbi2->num_overlays; j++) {
			struct owl_overlay *ovl;
			ovl = ofbi2->overlays[j];
			if (ovl->is_enabled(ovl)) {
				r = -EBUSY;
				goto out;
			}
		}
	}

	if (size != ofbi->region->size) {
		r = owlfb_realloc_fbmem(fbi, size, ofbi->region->type);
		if (r) {
			dev_err(dev, "realloc fbmem failed\n");
			goto out;
		}
	}

	r = count;
out:
	atomic_dec(&rg->lock_count);
	up_write(&rg->lock);

	unlock_fb_info(fbi);

	return r;
}

static ssize_t show_phys(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct owlfb_info *ofbi = FB2OFB(fbi);
	short offset = 0; 

	offset += snprintf(buf, PAGE_SIZE, "%0x\n", ofbi->region->paddr);
	return offset;
}

static ssize_t show_virt(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fb_info *fbi = dev_get_drvdata(dev);
	struct owlfb_info *ofbi = FB2OFB(fbi);

	return snprintf(buf, PAGE_SIZE, "%p\n", ofbi->region->vaddr);
}

static struct device_attribute owlfb_attrs[] = {
	__ATTR(mirror_to_hdmi, S_IRUGO | S_IWUSR, show_mirror_to_hdmi, store_mirror_to_hdmi),
	__ATTR(size, S_IRUGO | S_IWUSR, show_size, store_size),
	__ATTR(overlays, S_IRUGO | S_IWUSR, show_overlays, store_overlays),
	__ATTR(overscan, S_IRUGO | S_IWUSR, show_overscan, store_overscan),
	__ATTR(phys_addr, S_IRUGO, show_phys, NULL),
	__ATTR(virt_addr, S_IRUGO, show_virt, NULL),
};

int owlfb_create_sysfs(struct owlfb_device *fbdev)
{
	int i;
	int r;

	DBG("create sysfs for fbs\n");
	for (i = 0; i < fbdev->num_fbs; i++) {
		int t;
		for (t = 0; t < ARRAY_SIZE(owlfb_attrs); t++) {
			r = device_create_file(fbdev->fbs[i]->dev,
					&owlfb_attrs[t]);

			if (r) {
				dev_err(fbdev->dev, "failed to create sysfs "
						"file\n");
				return r;
			}
			
			#ifdef CONFIG_FB_DEFAULT_MIRROR_TO_HDMI
			store_mirror_to_hdmi(fbdev->fbs[i]->dev,
						&owlfb_attrs[0], "1", sizeof("1"));			
			#endif
		}
	}

	return 0;
}

void owlfb_remove_sysfs(struct owlfb_device *fbdev)
{
	int i, t;

	DBG("remove sysfs for fbs\n");
	for (i = 0; i < fbdev->num_fbs; i++) {
		for (t = 0; t < ARRAY_SIZE(owlfb_attrs); t++)
			device_remove_file(fbdev->fbs[i]->dev,
					&owlfb_attrs[t]);
	}
}

