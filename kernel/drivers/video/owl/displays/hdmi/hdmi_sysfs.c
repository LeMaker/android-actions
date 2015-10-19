/*
 * hdmi_sysfs.c
 *
 * HDMI OWL IP driver Library
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: Guo Long  <guolong@actions-semi.com>
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/poll.h>


#include "../../dss/dss_features.h"
#include "../../dss/dss.h"

#include "hdmi_ip.h"
#include "hdmi.h"

static ssize_t store_hdmi_src(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
	hdmi_send_uevent(0);	
	hdmi.ip_data.settings.hdmi_src = val;
	hdmi_send_uevent(1);		
	return r ? r : count;
}

static ssize_t show_hdmi_src(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", hdmi.ip_data.settings.hdmi_src);
}

static ssize_t store_hdmi_plug(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
		
	owldss_hdmi_hotplug_debug(hdmi.dssdev,val);	
	
	return r ? r : count;
}

static ssize_t store_hdmi_cable(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
		
	owldss_hdmi_cable_debug(hdmi.dssdev,val);	
	
	return r ? r : count;
}

static ssize_t store_hdmi_uevent(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
		
	owldss_hdmi_uevent_debug(hdmi.dssdev,val);	
	
	return r ? r : count;
}

static ssize_t show_hdmi_plug(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", hdmi.data.hdmi_sta);
}


static ssize_t show_TX_AMP(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val;
	val = readl(hdmi.ip_data.base+HDMI_TX_1);
	val = 0x0000000F&val;
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t store_TX_AMP(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val,tmp;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
  tmp=readl(hdmi.ip_data.base+HDMI_TX_1);
	tmp= ((tmp&0xFFFFFFF8)|val); 
	writel(tmp,hdmi.ip_data.base+HDMI_TX_1);
	return r ? r : count;
}

static ssize_t show_TX_IBIAS(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val;
	val = readl(hdmi.ip_data.base+HDMI_TX_1);
	val = ((0x00000030&val)>>4);
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t store_TX_IBIAS(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val,tmp;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
  tmp=readl(hdmi.ip_data.base+HDMI_TX_1);
	tmp= ((tmp&0xFFFFFFCF)|(val<<4)); 
	writel(tmp,hdmi.ip_data.base+HDMI_TX_1);
	return r ? r : count;
}

static ssize_t show_TER_EN(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val;
	val = readl(hdmi.ip_data.base+HDMI_TX_2);
	val = ((0x00020000&val)>>17);
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t store_TER_EN(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val,tmp;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
  tmp=readl(hdmi.ip_data.base+HDMI_TX_2);
	tmp= ((tmp&0xfffdffff)|(val<<17)); 
	writel(tmp,hdmi.ip_data.base+HDMI_TX_2);
	return r ? r : count;
}


static ssize_t show_TER_SET(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val;
	val = readl(hdmi.ip_data.base+HDMI_TX_2);
	val = ((0x000c0000&val)>>18);
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t store_TER_SET(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val,tmp;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
  tmp=readl(hdmi.ip_data.base+HDMI_TX_2);
	tmp= ((tmp&0xfff3ffff)|(val<<18)); 
	writel(tmp,hdmi.ip_data.base+HDMI_TX_2);
	return r ? r : count;
}



static ssize_t show_TMDS_LDO(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val;
	val = readl(hdmi.ip_data.base+HDMI_TX_2);
	val = ((0x0000000e&val)>>1);
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t store_TMDS_LDO(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val,tmp;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
  tmp=readl(hdmi.ip_data.base+HDMI_TX_2);
	tmp= ((tmp&0xfffffff1)|(val<<1)); 
	writel(tmp,hdmi.ip_data.base+HDMI_TX_2);
	return r ? r : count;
}


static ssize_t show_SLEW(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val;
	val = readl(hdmi.ip_data.base+HDMI_TX_2);
	val = ((0x000000c0&val)>>6);
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t store_SLEW(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val,tmp;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
  tmp=readl(hdmi.ip_data.base+HDMI_TX_2);
	tmp= ((tmp&0xffffff3f)|(val<<6)); 
	writel(tmp,hdmi.ip_data.base+HDMI_TX_2);
	return r ? r : count;
}


static ssize_t show_EMP_EN(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val;
	val = readl(hdmi.ip_data.base+HDMI_TX_1);
	val = ((0x00000040&val)>>6);
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t store_EMP_EN(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val,tmp;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
  tmp=readl(hdmi.ip_data.base+HDMI_TX_1);

	tmp= ((tmp&0xffffffbf)|(val<<6)); 

	writel(tmp,hdmi.ip_data.base+HDMI_TX_1);
	return r ? r : count;
}



static ssize_t show_EMP_SET(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val;
	val = readl(hdmi.ip_data.base+HDMI_TX_1);
	val = ((0x00000380&val)>>7);
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t store_EMP_SET(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val,tmp;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;
  tmp=readl(hdmi.ip_data.base+HDMI_TX_1);

	tmp= ((tmp&0xfffffc7f)|(val<<7)); 

	writel(tmp,hdmi.ip_data.base+HDMI_TX_1);
	return r ? r : count;
}


static ssize_t store_hdmi_vid(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;

	owldss_hdmi_display_disable(hdmi.dssdev);
	hdmi.dssdev->driver->set_vid(hdmi.dssdev, val);
	owldss_hdmi_display_enable(hdmi.dssdev);		
	return r ? r : count;
}

static ssize_t show_hdmi_vid(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	
	return snprintf(buf, PAGE_SIZE, "%d\n", hdmi.ip_data.cfg.cm.code);
}

static ssize_t show_hdmi_dump(struct device *dev, struct device_attribute *attr, char *buf)
{

	hdmi.ip_data.ops->dump_hdmi(&hdmi.ip_data);
	return 0;
}

static ssize_t show_edid_ok(struct device *dev, struct device_attribute *attr, char *buf)
{

	return snprintf(buf, PAGE_SIZE, "%d\n", hdmi.edid.read_ok);
}

static ssize_t show_edid_buf(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;
	for(i=0; i< 512; i++){
		DEBUG_ON("EDID_Buf[0x%x]=0x%x\n", i, hdmi.edid.EDID_Buf[i]);	
	}
	return 0;
}

static ssize_t store_hdcp_switch(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;

	hdmi_send_uevent(0);
	hdmi.dssdev->driver->enable_hdcp(hdmi.dssdev, val);
	hdmi_send_uevent(1);	
	return r ? r : count;
}

static ssize_t store_hpd_switch(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val;
	int r;
	r = kstrtoint(buf, 0, &val);
	if (r)
		return r;

	owldss_hdmi_display_enable_hotplug(hdmi.dssdev, val);	
	
	return r ? r : count;
}

static struct device_attribute asoc_hdmi_attrs[] = {
	__ATTR(reg_dump, S_IRUGO, show_hdmi_dump, NULL),
	__ATTR(edid_ok, S_IRUGO, show_edid_ok, NULL),
	__ATTR(edid_buf, S_IRUGO, show_edid_buf, NULL),
	__ATTR(TX_IBIAS, S_IRUGO| S_IWUSR, show_TX_IBIAS, store_TX_IBIAS),
	__ATTR(TX_AMP, S_IRUGO| S_IWUSR, show_TX_AMP, store_TX_AMP),
	__ATTR(TER_EN, S_IRUGO| S_IWUSR, show_TER_EN, store_TER_EN),
	__ATTR(TER_SET, S_IRUGO| S_IWUSR, show_TER_SET, store_TER_SET),
	__ATTR(TMDS_LDO, S_IRUGO| S_IWUSR, show_TMDS_LDO, store_TMDS_LDO),
	__ATTR(EMP_EN, S_IRUGO| S_IWUSR, show_EMP_EN, store_EMP_EN),
	__ATTR(SLEW, S_IRUGO| S_IWUSR, show_SLEW, store_SLEW),
	__ATTR(EMP_SET, S_IRUGO| S_IWUSR, show_EMP_SET, store_EMP_SET),
	__ATTR(hdcp_switch, S_IWUSR, NULL, store_hdcp_switch),
	__ATTR(vid, S_IRUGO | S_IWUSR, show_hdmi_vid, store_hdmi_vid),
	__ATTR(src, S_IRUGO | S_IWUSR, show_hdmi_src, store_hdmi_src),
	__ATTR(plug, S_IRUGO | S_IWUSR, show_hdmi_plug, store_hdmi_plug),
	__ATTR(cable, S_IRUGO | S_IWUSR, show_hdmi_plug, store_hdmi_cable),
	__ATTR(switchuevent, S_IRUGO | S_IWUSR, show_hdmi_plug, store_hdmi_uevent),
	__ATTR(hpd_switch, S_IWUSR, NULL, store_hpd_switch),
};

int owl_hdmi_create_sysfs(struct device *dev)
{
	int r, t;

	for (t = 0; t < ARRAY_SIZE(asoc_hdmi_attrs); t++) 
	{
		r = device_create_file(dev, &asoc_hdmi_attrs[t]);

		if (r) {
			dev_err(dev, "failed to create sysfs file\n");
			return r;
		}
	}

	return 0;
}

void asoc_hdmi_remove_sysfs(struct device *dev)
{
	int  t;
	
	for (t = 0; t < ARRAY_SIZE(asoc_hdmi_attrs); t++) 
	{
		device_remove_file(dev, &asoc_hdmi_attrs[t]);
	}
}

