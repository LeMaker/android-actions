/*
 * common power management functions for camera sensors
 */
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/module.h>

#include <mach/clkname.h>
#include <mach/isp-owl.h>

// for bisp's tow soc camera host
static struct sensor_pwd_info *g_spinfo[2] = {NULL, NULL};

static inline struct sensor_pwd_info *to_spinfo(int host_id)
{
    return g_spinfo[!!host_id];
}

/* should be called before register host using soc_camera_host_register() */
void attach_sensor_pwd_info(struct device *dev, 
    struct sensor_pwd_info *pi, int host_id)
{
    int id = !!host_id;

    if (g_spinfo[id]) {
        dev_err(dev, "already register it [host id : %d]\n", host_id);
    }
    g_spinfo[id] = pi;
}
EXPORT_SYMBOL(attach_sensor_pwd_info);


void detach_sensor_pwd_info(struct device *dev, 
    struct sensor_pwd_info *pi, int host_id)
{
    int id = !!host_id;

    if (pi != g_spinfo[id]) {
        dev_err(dev, "sensor pwd info don't match with host id[%d]\n", host_id);
    }
    g_spinfo[id] = NULL;
}
EXPORT_SYMBOL(detach_sensor_pwd_info);

int owl_isp_reset_pin_state = 0;
EXPORT_SYMBOL(owl_isp_reset_pin_state);

void owl_isp_reset(struct device *dev, int host_id)
{
    struct sensor_pwd_info *pi = to_spinfo(host_id);
    struct dts_gpio *reset = &pi->gpio_reset;

    if(owl_isp_reset_pin_state){
        printk("%s():maybe has open some camera,skip it.\n",__FUNCTION__);
        return;
    }
    
	printk("%s() %d\n",__FUNCTION__,__LINE__);
	/* RESET */
	gpio_direction_output(reset->num, reset->active_level);
	msleep(10);
	gpio_direction_output(reset->num, !reset->active_level);
	msleep(10);
}
EXPORT_SYMBOL(owl_isp_reset);


int owl_isp_power_on(int channel, int rear, int host_id)
{
    struct sensor_pwd_info *pi = to_spinfo(host_id);
    struct dts_gpio *dgrear = &pi->gpio_rear;
    struct dts_gpio *dgfront = &pi->gpio_front;
    struct clk *sclk = pi->ch_clk[channel];
    int ret = 0;

    printk("%s(): %s, channel[%d], host[%d]\n",__func__,
        rear ? "rear" : "front", channel, host_id);
 
    if (rear) {
        gpio_direction_output(dgrear->num, !dgrear->active_level);
    } else {
        gpio_direction_output(dgfront->num, !dgfront->active_level);
    }

    clk_prepare(sclk);
	clk_enable(sclk);
	ret = clk_set_rate(sclk, OUTTO_SENSO_CLOCK);
	if(ret) {
		printk(KERN_ERR "%s() : set isp clock error: %d\n",__func__, ret);
		return ret;
	}
	printk("sensor clock is %dM\n",(int)(clk_get_rate(sclk) / 1000000));
	msleep(10);

	return ret;
}
EXPORT_SYMBOL(owl_isp_power_on);


int owl_isp_power_off(int channel, int rear, int host_id)
{
    struct sensor_pwd_info *pi = to_spinfo(host_id);
    struct dts_gpio *dgrear = &pi->gpio_rear;
    struct dts_gpio *dgfront = &pi->gpio_front;
    struct clk *sclk = pi->ch_clk[channel];

	printk("%s():%s, channel[%d], host[%d]\n",__func__,
        rear ? "rear" : "front", channel, host_id);
    if (sclk) {
        clk_disable(sclk);
        clk_unprepare(sclk);
    }

    if (rear) {
        gpio_direction_output(dgrear->num, dgrear->active_level);
    } else {
        gpio_direction_output(dgfront->num, dgfront->active_level);
    }

	return 0;
}
EXPORT_SYMBOL(owl_isp_power_off);


static int get_parent_node_id(struct device_node *node,
    const char *property, const char *stem)
{
    struct device_node *pnode;
    unsigned int value = -ENODEV;

    pnode = of_parse_phandle(node, property, 0);
    if (NULL == pnode) {
        printk(KERN_ERR "err: fail to get node[%s]\n", property);
        return value;
    }
    value = of_alias_get_id(pnode, stem);

    return value;
}

int parse_config_info(struct soc_camera_link *link,
    struct dts_sensor_config *dsc, const char *name)
{
    struct device_node *fdt_node;
    struct module_info *minfo = NULL;
    const char *bus_type;
    const char *data_type;


    fdt_node = of_find_compatible_node(NULL, NULL, name);
    if (NULL == fdt_node) {
        printk(KERN_ERR "err: no sensor [%s]\n", name);
        goto fail;
    }
    dsc->dn = fdt_node;

    //if (of_property_read_u32(fdt_node, "rear", &dsc->rear)) {
    //    printk(KERN_ERR "err: fail to get sensor position\n");
    //    goto fail;
    //}

    if (of_property_read_u32(fdt_node, "channel", &dsc->channel)) {
        printk(KERN_ERR "err: fail to get sensor channel\n");
        goto fail;
    }

    if (of_property_read_string(fdt_node, "bus_type", &bus_type)) {
        printk(KERN_ERR "err: faild to get sensor bus type\n");
        goto fail;
    }
    if (!strcmp(bus_type, "dvp")) {
        dsc->bus_type = V4L2_MBUS_PARALLEL;
    } else if (!strcmp(bus_type, "mipi")) {
        dsc->bus_type = V4L2_MBUS_CSI2;
    } else {
        printk(KERN_ERR "err: bus_type of sensor dts is wrong\n");
        goto fail;
    }

    if (of_property_read_string(fdt_node, "data_type", &data_type)) {
        printk(KERN_ERR "err: faild to get sensor output data type\n");
        goto fail;
    }
    if (!strcmp(data_type, "yuv")) {
        dsc->data_type = SENSOR_DATA_TYPE_YUV;
    } else if (!strcmp(data_type, "raw")) {
        dsc->data_type = SENSOR_DATA_TYPE_RAW;
    } else {
        printk(KERN_ERR "err: data_type of sensor dts is wrong\n");
        goto fail;
    }

    dsc->host = get_parent_node_id(fdt_node, "host", "isp");
    if (dsc->host < 0) {
        printk(KERN_ERR "err: fail to get host id\n");
        goto fail;
    }

    dsc->i2c_adapter = get_parent_node_id(fdt_node, "i2c_adapter", "i2c");
    if (dsc->i2c_adapter < 0) {
        printk(KERN_ERR "err: fail to get i2c adapter id\n");
        goto fail;
    }

    if(link) {
        minfo = link->priv;
        link->bus_id = dsc->host;
        link->i2c_adapter_id = dsc->i2c_adapter;

        minfo->flags &= ~(SENSOR_FLAG_DTS_MASK);

        if (V4L2_MBUS_PARALLEL == dsc->bus_type) {
            minfo->flags |= SENSOR_FLAG_DVP;
        } else {
            minfo->flags |= SENSOR_FLAG_MIPI;
        }

        /* first logic index is 0 in driver, but 1 in spec */
        if (ISP_CHANNEL_0 == dsc->channel) {
            minfo->flags |= SENSOR_FLAG_CHANNEL1;
        } else {
            minfo->flags |= SENSOR_FLAG_CHANNEL2;
        }

        if (SENSOR_DATA_TYPE_YUV == dsc->data_type) {
            minfo->flags |= SENSOR_FLAG_YUV;
        } else {
            minfo->flags |= SENSOR_FLAG_RAW;
        }
    }

    return 0;

fail:
    return -EINVAL;
}
EXPORT_SYMBOL(parse_config_info);

