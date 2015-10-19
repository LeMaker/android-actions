#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <mach/isp-owl.h>
#include <mach/module-owl.h>
#include <mach/clkname.h>
#include "module_comm.h"

#define CAMERA_COMMON_NAME           "sensor_common"
#define CAMERA_DETECT_NAME           "sensor_detect before probe"

#define ISP_FDT_COMPATIBLE           "actions,owl-isp"

#define CAMERA_MODULE_CLOCK     24000000
#define ISP_MODULE_CLOCK        60000000
#define CAMERA_REAR_NAME        "rear_camera"
#define CAMERA_FRONT_NAME        "front_camera"

static int  camera_front_offset  = -1;
static int  camera_rear_offset   = -1;



static struct clk *g_sensor_clk = NULL;
static struct clk *g_isp_clk    = NULL;

static char camera_front_name[32];
static char camera_rear_name[32];


/////////////
static struct sensor_pwd_info   g_spinfo;
static struct isp_regulators    g_isp_ir;
static int                      g_sens_channel = 0;   // sens0 or sens1
static struct i2c_adapter       *g_adap;
/////////////


//struct module_item_t g_module = CAMERA_ITEM_INIT(gc2035, 0x78, 1, 1, module_verify_pid);

static int get_parent_node_id(struct device_node *node,
    const char *property, const char *stem)
{
    struct device_node *pnode;
    unsigned int value = -ENODEV;

    pnode = of_parse_phandle(node, property, 0);
    if (NULL == pnode) {
        printk(KERN_ERR "err: fail to get node[%s]", property);
        return value;
    }
    value = of_alias_get_id(pnode, stem);

    return value;
}

static int init_common(void)
{
    struct device_node *fdt_node;
    int i2c_adap_id = 0;
    struct dts_regulator *dr = &g_isp_ir.avdd.regul;

	// GC_INFO("");
    // get i2c adapter
    fdt_node = of_find_compatible_node(NULL, NULL, CAMERA_COMMON_NAME);
    if (NULL == fdt_node) {
        GC_ERR("err: no ["CAMERA_COMMON_NAME"] in dts");
        return -1;
    }
    i2c_adap_id = get_parent_node_id(fdt_node, "i2c_adapter", "i2c");
    g_adap = i2c_get_adapter(i2c_adap_id);
	// GC_INFO("");
    // get sens channel
    if(of_property_read_u32(fdt_node, "channel", &g_sens_channel)) {
        g_sens_channel = 0;
    }
	//GC_INFO("");


    g_spinfo.flag = 0;
    g_spinfo.gpio_rear.num = -1;
    g_spinfo.gpio_front.num = -1;
    g_spinfo.gpio_reset.num = -1;
    g_spinfo.ch_clk[ISP_CHANNEL_0] = NULL;
    g_spinfo.ch_clk[ISP_CHANNEL_1] = NULL;

    dr->regul = NULL;
    g_isp_ir.avdd_use_gpio = 0;
    g_isp_ir.dvdd_use_gpio = 0;
    g_isp_ir.dvdd.regul = NULL;

    return 0;
}

static inline void set_gpio_level(struct dts_gpio *gpio, bool active) {
    if (active) {
        gpio_direction_output(gpio->num, gpio->active_level);
    } else {
        gpio_direction_output(gpio->num, !gpio->active_level);
    }
}

static int gpio_init(struct device_node *fdt_node,
                     const char *gpio_name, struct dts_gpio *gpio, bool active)
{
    enum of_gpio_flags flags;

    if (!of_find_property(fdt_node, gpio_name, NULL)) {
        GC_ERR("no config gpios:%s", gpio_name);
        goto fail;
    }
    gpio->num = of_get_named_gpio_flags(fdt_node, gpio_name, 0, &flags);
    gpio->active_level = !(flags & OF_GPIO_ACTIVE_LOW);

  // GC_INFO("%s: num-%d, active-%s", gpio_name, gpio->num, gpio->active_level ? "high" : "low");

    if (gpio_request(gpio->num, gpio_name)) {
        GC_ERR("fail to request gpio [%d]", gpio->num);
        gpio->num = -1;
        goto fail;
    }

    set_gpio_level(gpio, active);

   // GC_INFO("gpio value: 0x%x", gpio_get_value(gpio->num));

    return 0;
  fail:
    return -1;
}

static void gpio_exit(struct dts_gpio *gpio, bool active)
{
    //GC_INFO("gpio free:%d", gpio->num);
    if (gpio->num >= 0) {
        set_gpio_level(gpio, active);
        gpio_free(gpio->num);
    }
}

static int regulator_init(struct device_node *fdt_node,
                          const char *regul_name, const char *scope_name,
                          struct dts_regulator *dts_regul)
{
    unsigned int scope[2];
    const char *regul = NULL;

   // GC_INFO("");
    if (of_property_read_string(fdt_node, regul_name, &regul)) {
        GC_ERR("don't config %s", regul_name);
        goto fail;
    }
   // GC_INFO("%s", regul ? regul : "NULL");

    if (of_property_read_u32_array(fdt_node, scope_name, scope, 2)) {
        GC_ERR("fail to get %s", scope_name);
        goto fail;
    }
   // GC_INFO("min-%d, max-%d", scope[0], scope[1]);
    dts_regul->min = scope[0];
    dts_regul->max = scope[1];

    dts_regul->regul = regulator_get(NULL, regul);
    if (IS_ERR(dts_regul->regul)) {
        dts_regul->regul = NULL;
        GC_ERR("get regulator failed");
        goto fail;
    }

    regulator_set_voltage(dts_regul->regul, dts_regul->min, dts_regul->max);
    //regulator_enable(dts_regul->regul);
    //mdelay(5);
    return 0;

  fail:
    return -1;

}

static inline void regulator_exit(struct dts_regulator *dr)
{
    regulator_put(dr->regul);
    dr->regul = NULL;
}

static int isp_regulator_init(struct device_node *fdt_node, struct isp_regulators *ir)
{
    const char *avdd_src = NULL;

/*DVDD*/
    struct dts_gpio *dvdd_gpio = &ir->dvdd_gpio;
    //GC_INFO("");
    if (!gpio_init(fdt_node, "dvdd-gpios", dvdd_gpio, 0))/* poweroff */
        ir->dvdd_use_gpio = 1;
    else
        ir->dvdd_use_gpio = 0;

    if (regulator_init(fdt_node, "dvdd-regulator",
                "dvdd-regulator-scope", &ir->dvdd))
        goto fail;

/*AVDD*/
    if (of_property_read_string(fdt_node, "avdd-src", &avdd_src)) {
        GC_ERR("get avdd-src faild");
        goto fail;
    }

    if (!strcmp(avdd_src, "regulator")) {
  //      GC_INFO("avdd using regulator");
        ir->avdd_use_gpio = 0;

        if (regulator_init(fdt_node, "avdd-regulator",
                    "avdd-regulator-scope", &ir->avdd.regul))
            goto free_dvdd;
    } else if (!strcmp(avdd_src, "gpio")) {
        struct dts_gpio *gpio = &ir->avdd.gpio;
        ir->avdd_use_gpio = 1;

    gpio_init(fdt_node, "avdd-gpios", gpio, 0);

      //  GC_INFO("set - avdd gpio value: 0x%x", gpio_get_value(gpio->num));
    } else {
        //GC_INFO("needn't operate avdd manually");
    }

    return 0;

free_dvdd:
    regulator_exit(&ir->dvdd);
fail:
    return -1;
}

static void isp_regulator_exit(struct isp_regulators *ir)
{
   // GC_INFO("");
    if (ir->dvdd_use_gpio)
        gpio_exit(&ir->dvdd_gpio, 0);

    if (ir->dvdd.regul) {
        regulator_exit(&ir->dvdd);
    }

    if (ir->avdd_use_gpio) {
        gpio_exit(&ir->avdd.gpio, 0);
    } else {
        struct dts_regulator *dr = &ir->avdd.regul;

        if (dr->regul) {
            regulator_exit(dr);
        }
    }
}

static void isp_regulator_enable(struct isp_regulators *ir)
{
  //  GC_INFO("");
  	int ret = 0;
    if (ir->dvdd.regul) {
        ret = regulator_enable(ir->dvdd.regul);
        mdelay(1);
    }

    if (ir->avdd_use_gpio) {
        set_gpio_level(&ir->avdd.gpio, 1);
    } else {
        struct dts_regulator *dr = &ir->avdd.regul;
        if (dr->regul) {
            ret = regulator_enable(dr->regul);
            mdelay(1);
        }
    }

    if (ir->dvdd_use_gpio) {
        set_gpio_level(&ir->dvdd_gpio, 1);
    }
}

static void isp_regulator_disable(struct isp_regulators *ir)
{
    //GC_INFO("");
    if (ir->dvdd_use_gpio) {
        set_gpio_level(&ir->dvdd_gpio, 0);
    }

    if (ir->dvdd.regul) {
        regulator_disable(ir->dvdd.regul);
    }

    if (ir->avdd_use_gpio) {
        set_gpio_level(&ir->avdd.gpio, 0);
    } else {
        struct dts_regulator *dr = &ir->avdd.regul;
        if (dr->regul) {
            regulator_disable(dr->regul);
        }
    }
}
static int isp_gpio_init(struct device_node *fdt_node, struct sensor_pwd_info *spinfo)
{
    const char *sensors = NULL;

   // GC_INFO("");
    if (gpio_init(fdt_node, "reset-gpios", &spinfo->gpio_reset, 0)) {
        goto fail;
    }

    if (of_property_read_string(fdt_node, "sensors", &sensors)) {
        GC_ERR("get sensors faild");
        goto free_reset;
    }

    if (!strcmp(sensors, "front")) {
        // default is power-down
        if (gpio_init(fdt_node, "pwdn-front-gpios", &spinfo->gpio_front, 1)) {
            goto free_reset;
        }
        spinfo->flag = SENSOR_FRONT;
    } else if (!strcmp(sensors, "rear")) {
        if (gpio_init(fdt_node, "pwdn-rear-gpios", &spinfo->gpio_rear, 1)) {
            goto free_reset;
        }
        spinfo->flag = SENSOR_REAR;
    } else if (!strcmp(sensors, "dual")) {
        if (gpio_init(fdt_node, "pwdn-front-gpios", &spinfo->gpio_front, 1)) {
            goto free_reset;
        }
        if (gpio_init(fdt_node, "pwdn-rear-gpios", &spinfo->gpio_rear, 1)) {
            gpio_exit(&spinfo->gpio_front, 1);
            goto free_reset;
        }
        spinfo->flag = SENSOR_DUAL;
    } else {
        GC_ERR("sensors of dts is wrong");
        goto free_reset;
    }
    return 0;

  free_reset:
    gpio_exit(&spinfo->gpio_reset, 0);
  fail:
    return -1;
}

static void isp_gpio_exit(struct sensor_pwd_info *spinfo)
{
    //GC_INFO("");
    // only free valid gpio, so no need to check its existence.
    gpio_exit(&spinfo->gpio_front, 1);
    gpio_exit(&spinfo->gpio_rear, 1);
    gpio_exit(&spinfo->gpio_reset, 0);
}

static int isp_clk_init(void)
{
    struct clk *tmp = NULL;
    int ret = 0;

    module_reset(MODULE_RST_BISP);
    module_clk_enable(MOD_ID_BISP);

   // GC_INFO("");
    tmp = clk_get(NULL, CLKNAME_BISP_CLK);
    if (IS_ERR(tmp)) {
        ret = PTR_ERR(tmp);
        g_isp_clk = NULL;
        GC_ERR("get isp clock error (%d)", ret);
        return ret;
    }
    g_isp_clk = tmp;

    mdelay(1);
    
    return ret;
}

static int isp_clk_enable(void)
{
    int ret = 0;
    if (g_isp_clk != NULL) {
        clk_prepare(g_isp_clk);
        ret = clk_enable(g_isp_clk);  /*enable clk*/
        if (ret) {
            GC_ERR("si clock enable error (%d)", ret);
        }
        ret = clk_set_rate(g_isp_clk, ISP_MODULE_CLOCK); /*设置isp工作频率*/
    }
    
    return ret;    
}

static void isp_clk_disable(void)
{
    //GC_INFO("");
    if (g_isp_clk != NULL) {
        clk_disable(g_isp_clk);
        clk_unprepare(g_isp_clk);
        clk_put(g_isp_clk);
        module_clk_disable(MOD_ID_BISP);
        g_isp_clk = NULL;
    }
}

static int sensor_clk_init(void)
{
    struct clk *tmp = NULL;
    int ret = 0;

    module_clk_enable(MOD_ID_CSI);
    module_reset(MODULE_RST_CSI);

  //  GC_INFO("sensor channel:%d", g_sens_channel);
    if(g_sens_channel == 0)
        tmp = clk_get(NULL, CLKNAME_SENSOR_CLKOUT0);
    else
        tmp = clk_get(NULL, CLKNAME_SENSOR_CLKOUT1);
    if (IS_ERR(tmp)) {
        ret = PTR_ERR(tmp);
        GC_ERR("get isp-channel-%d clock error%d", g_sens_channel, ret);
        return ret;
    }
    g_sensor_clk = tmp;
 //   GC_INFO("sensor_clk:%p", g_sensor_clk);
    return ret;
}

static int sensor_clk_enable(void)
{
    int ret;

  //  GC_INFO("");
    if(g_sensor_clk == NULL) {
        GC_ERR("sensor clk not initialized!");
        return -1;
    }

    clk_prepare(g_sensor_clk);
    clk_enable(g_sensor_clk);

    ret = clk_set_rate(g_sensor_clk, CAMERA_MODULE_CLOCK); /*设置sensor频率*/
    if (ret) {
        GC_ERR("set isp clock error");
    }
    mdelay(2);
    return ret;
}

static void sensor_clk_disable(void)
{
   // GC_INFO("");
    if(g_sensor_clk == NULL) {
        GC_ERR("sensor clk not initialized!");
        return;
    }

    clk_disable(g_sensor_clk);
    clk_unprepare(g_sensor_clk);
    clk_put(g_sensor_clk);
    module_clk_disable(MOD_ID_CSI);
    g_sensor_clk = NULL;
}


static void sensor_power(bool front, bool on)
{
   // GC_INFO("%s sensor power %s", front ? "front" : "rear", on ? "on" : "off");
    if (front) {
        set_gpio_level(&g_spinfo.gpio_front, !on);
      //  sensor_reset();
    } else {
        set_gpio_level(&g_spinfo.gpio_rear, !on);
    }
    mdelay(2);
}
static int detect_init(void)
{
    struct device_node *fdt_node = NULL;
    int ret = 0;

   // GC_INFO("");

    if(init_common()) {
        return -1;
    }

    ret = isp_clk_init();
    if (ret) {
        GC_ERR("init isp clock error");
        goto exit;
    }

    ret = sensor_clk_init();
    if (ret) {
        GC_ERR("init sensor clock error");
        goto exit;
    }

    fdt_node = of_find_compatible_node(NULL, NULL, ISP_FDT_COMPATIBLE);
    if (NULL == fdt_node) {
        GC_ERR("err: no ["ISP_FDT_COMPATIBLE"] in dts");
        return -1;
    }

    ret = isp_gpio_init(fdt_node, &g_spinfo);
    if (ret) {
        GC_ERR("pwdn init error!");
        goto exit;
    }

    ret = isp_regulator_init(fdt_node, &g_isp_ir);
    if (ret) {
        GC_ERR("avdd init error!");
        goto exit;
    }

    ret = isp_clk_enable();
    if (ret) {
        GC_ERR("enable isp clock error");
        goto exit;
    }

    ret = sensor_clk_enable();
    if (ret) {
        GC_ERR("enable sensor clock error");
        goto exit;
    }

    isp_regulator_enable(&g_isp_ir);
    return ret;

exit:
    isp_clk_disable();
    sensor_clk_disable();
    isp_regulator_exit(&g_isp_ir);
    isp_gpio_exit(&g_spinfo);

    return ret;
}

 static void detect_deinit(void)
{
    isp_clk_disable();
    sensor_clk_disable();
	isp_regulator_disable(&g_isp_ir);
    isp_regulator_exit(&g_isp_ir);
    isp_gpio_exit(&g_spinfo);
   
}


static int detect_process(void)
{
    int ret = -1;
    ret = module_verify_pid(g_adap,NULL);
    if (ret == 0) {
        GC_INFO("detect  success!!!!!!!!");
    }
    else
    {
        GC_INFO("( failed.");
    }

    return ret;
}

#if 1
static ssize_t front_name_show(struct device *dev,  struct device_attribute *attr,  
        char *buf)  
{  
    return strlcpy(buf, camera_front_name, sizeof(camera_front_name)); 
}  

static ssize_t rear_name_show(struct device *dev,  struct device_attribute *attr,
        char *buf)  
{  
    return strlcpy(buf, camera_rear_name, sizeof(camera_rear_name)); 
}


static ssize_t front_offset_show(struct device *dev,  struct device_attribute *attr,  
        char *buf)  
{  
    return sprintf(buf, "%d", camera_front_offset); 
}  

static ssize_t rear_offset_show(struct device *dev,  struct device_attribute *attr,
        char *buf)  
{  
    return sprintf(buf, "%d", camera_rear_offset); 
}  

static DEVICE_ATTR(front_name,   0444, front_name_show,   NULL); 
static DEVICE_ATTR(rear_name,    0444, rear_name_show,    NULL);
static DEVICE_ATTR(front_offset, 0444, front_offset_show, NULL); 
static DEVICE_ATTR(rear_offset,  0444, rear_offset_show,  NULL);

#endif
static int  detect_work(void)
{
    int ret = 0;

   
       // GC_INFO("--------detect front sensor------");
        sensor_power(true, true);
        ret = detect_process();
		sensor_power(true, false);
		if(0 == ret)
			{
			#if 1
			 struct kobject             *front_kobj;
			 front_kobj = kobject_create_and_add(CAMERA_FRONT_NAME, NULL);
			 if (front_kobj == NULL) {  
       			 GC_ERR("kobject_create_and_add failed.");
        			ret = -ENOMEM;  
					return ret;
    			}  
			 	camera_front_offset = 1;
			 	sprintf(camera_front_name, "%s.ko", CAMERA_MODULE_NAME);
				ret = sysfs_create_file(front_kobj, &dev_attr_front_offset.attr);
			    ret = sysfs_create_file(front_kobj, &dev_attr_front_name.attr);
				#endif
			 return SENSOR_FRONT;
			}

      //  DBG_INFO("-------detect rear sensor-------");
        sensor_power(false, true);
        ret = detect_process();
		sensor_power(false, false);
        if (ret == 0) {
		struct kobject             *rear_kobj;
			 rear_kobj = kobject_create_and_add(CAMERA_REAR_NAME, NULL);
			 if (rear_kobj == NULL) {  
       			 GC_ERR("kobject_create_and_add failed.");
        			ret = -ENOMEM;  
					return ret;
    			}  
			 	camera_rear_offset = 0;
			 	ret = sysfs_create_file(rear_kobj, &dev_attr_rear_offset.attr);
			 	sprintf(camera_rear_name, "%s.ko", CAMERA_MODULE_NAME);
			    ret = sysfs_create_file(rear_kobj, &dev_attr_rear_name.attr);
		return SENSOR_REAR;
        }
		return ret;
}

