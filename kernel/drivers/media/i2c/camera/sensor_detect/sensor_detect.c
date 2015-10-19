#include "module_list.c"
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

#define CAMERA_COMMON_NAME           "sensor_common"
#define CAMERA_DETECT_NAME           "sensor_detect"
#define CFG_CAMERA_DETECT_LIST       "sensor_detect_list"

#define ISP_FDT_COMPATIBLE           "actions,owl-isp"

#define SENSOR_FRONT 0x1
#define SENSOR_REAR 0x2
#define SENSOR_DUAL 0x4

#ifdef DETECT_DBG
#define DBG_INFO(fmt, args...)  printk(KERN_INFO"[" CAMERA_DETECT_NAME "] (INFO) line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)
#else
#define DBG_INFO(fmt, args...)
#endif

#define DBG_ERR(fmt, args...)   printk(KERN_ERR"[" CAMERA_DETECT_NAME "] (ERR) line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)

#define CAMERA_MODULE_CLOCK     24000000
#define ISP_MODULE_CLOCK        60000000

#define DELAY_INTERVAL            msecs_to_jiffies(2 * 1000)

static struct kobject              *front_kobj = NULL;
static struct kobject              *rear_kobj = NULL;
#define CAMERA_REAR_NAME        "rear_camera"
#define CAMERA_FRONT_NAME        "front_camera"



static char camera_front_name[32];
static char camera_rear_name[32];

static int  camera_detect_status = 0;
static int  camera_front_start   = 0;
static int  camera_rear_start    = 0;
static int  camera_front_offset  = -1;
static int  camera_rear_offset   = -1;

static bool g_front_detected      = false;
static bool g_rear_detected       = false;

static int  hot_plug_enable     = 0;
static int  detect_times        = 0;

static struct clk *g_sensor_clk = NULL;
static struct clk *g_isp_clk    = NULL;

/////////////
static struct delayed_work      g_work;
static struct sensor_pwd_info   g_spinfo;
static struct isp_regulators    g_isp_ir;
static int                      g_sens_channel = 0;   // sens0 or sens1
static struct i2c_adapter       *g_adap;
/////////////


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
    struct module_item_t *module_item;
    struct device_node *fdt_node;
    int i2c_adap_id = 0;
    struct dts_regulator *dr = &g_isp_ir.avdd.regul;

    int i = 0;
    int value = 0;

    // get i2c adapter
    fdt_node = of_find_compatible_node(NULL, NULL, CAMERA_COMMON_NAME);
    if (NULL == fdt_node) {
        DBG_ERR("err: no ["CAMERA_COMMON_NAME"] in dts");
        return -1;
    }
    i2c_adap_id = get_parent_node_id(fdt_node, "i2c_adapter", "i2c");
    g_adap = i2c_get_adapter(i2c_adap_id);

    // get sens channel
    if(of_property_read_u32(fdt_node, "channel", &g_sens_channel)) {
        g_sens_channel = 0;
    }

	#if 1
    // get detect sensor list
    fdt_node = of_find_compatible_node(NULL, NULL, CAMERA_DETECT_NAME);
    if (NULL == fdt_node) {
        DBG_ERR("err: no ["CAMERA_DETECT_NAME"] in dts");
        return -1;
    }
    if(of_property_read_u32(fdt_node, "hot_plugin_enable", &hot_plug_enable)) {
        DBG_ERR("no hot_plugin_enable node");
        hot_plug_enable = 0;
    }
    fdt_node = of_get_child_by_name(fdt_node, CFG_CAMERA_DETECT_LIST);
    if (NULL == fdt_node) {
        DBG_ERR("err: no ["CFG_CAMERA_DETECT_LIST"] in dts");
        return -1;
    }
	#endif
    for (i = 0; i < ARRAY_SIZE(g_module_list); i++) {
        module_item = &g_module_list[i];
        if(of_property_read_u32(fdt_node, module_item->name, &value))
            continue;
        module_item->need_detect = (bool)value;
    }


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
        DBG_ERR("no config gpios:%s", gpio_name);
        goto fail;
    }
    gpio->num = of_get_named_gpio_flags(fdt_node, gpio_name, 0, &flags);
    gpio->active_level = !(flags & OF_GPIO_ACTIVE_LOW);

    DBG_INFO("%s: num-%d, active-%s", gpio_name, gpio->num, gpio->active_level ? "high" : "low");

    if (gpio_request(gpio->num, gpio_name)) {
        DBG_ERR("fail to request gpio [%d]", gpio->num);
        gpio->num = -1;
        goto fail;
    }

    set_gpio_level(gpio, active);

    DBG_INFO("gpio value: 0x%x", gpio_get_value(gpio->num));

    return 0;
  fail:
    return -1;
}

static void gpio_exit(struct dts_gpio *gpio, bool active)
{
    DBG_INFO("gpio free:%d", gpio->num);
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

    DBG_INFO("");
    if (of_property_read_string(fdt_node, regul_name, &regul)) {
        DBG_ERR("don't config %s", regul_name);
        goto fail;
    }
    DBG_INFO("%s", regul ? regul : "NULL");

    if (of_property_read_u32_array(fdt_node, scope_name, scope, 2)) {
        DBG_ERR("fail to get %s", scope_name);
        goto fail;
    }
    DBG_INFO("min-%d, max-%d", scope[0], scope[1]);
    dts_regul->min = scope[0];
    dts_regul->max = scope[1];

    dts_regul->regul = regulator_get(NULL, regul);
    if (IS_ERR(dts_regul->regul)) {
        dts_regul->regul = NULL;
        DBG_ERR("get regulator failed");
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
    DBG_INFO("");
    if (!gpio_init(fdt_node, "dvdd-gpios", dvdd_gpio, 0))/* poweroff */
        ir->dvdd_use_gpio = 1;
    else
        ir->dvdd_use_gpio = 0;

    if (regulator_init(fdt_node, "dvdd-regulator",
                "dvdd-regulator-scope", &ir->dvdd))
        goto fail;

/*AVDD*/
    if (of_property_read_string(fdt_node, "avdd-src", &avdd_src)) {
        DBG_ERR("get avdd-src faild");
        goto fail;
    }

    if (!strcmp(avdd_src, "regulator")) {
        DBG_INFO("avdd using regulator");
        ir->avdd_use_gpio = 0;

        if (regulator_init(fdt_node, "avdd-regulator",
                    "avdd-regulator-scope", &ir->avdd.regul))
            goto free_dvdd;
    } else if (!strcmp(avdd_src, "gpio")) {
        struct dts_gpio *gpio = &ir->avdd.gpio;
        ir->avdd_use_gpio = 1;

        if (gpio_init(fdt_node, "avdd-gpios", gpio, 0))/* poweroff */
            goto fail;

        DBG_INFO("set - avdd gpio value: 0x%x", gpio_get_value(gpio->num));
    } else {
        DBG_INFO("needn't operate avdd manually");
    }

    return 0;

free_dvdd:
    regulator_exit(&ir->dvdd);
fail:
    return -1;
}

static void isp_regulator_exit(struct isp_regulators *ir)
{
    DBG_INFO("");
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
	int ret = 0;
    DBG_INFO("");
    if (ir->dvdd.regul) {
        ret = regulator_enable(ir->dvdd.regul);
        mdelay(5);
    }

    if (ir->avdd_use_gpio) {
        set_gpio_level(&ir->avdd.gpio, 1);
    } else {
        struct dts_regulator *dr = &ir->avdd.regul;
        if (dr->regul) {
            ret = regulator_enable(dr->regul);
            mdelay(5);
        }
    }

    if (ir->dvdd_use_gpio) {
        set_gpio_level(&ir->dvdd_gpio, 1);
    }
}

static void isp_regulator_disable(struct isp_regulators *ir)
{
    DBG_INFO("");
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

    DBG_INFO("");
    if (gpio_init(fdt_node, "reset-gpios", &spinfo->gpio_reset, 0)) {
        goto fail;
    }

    if (of_property_read_string(fdt_node, "sensors", &sensors)) {
        DBG_ERR("get sensors faild");
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
        DBG_ERR("sensors of dts is wrong");
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
    DBG_INFO("");
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

    DBG_INFO("");
    tmp = clk_get(NULL, CLKNAME_BISP_CLK);
    if (IS_ERR(tmp)) {
        ret = PTR_ERR(tmp);
        g_isp_clk = NULL;
        DBG_ERR("get isp clock error (%d)", ret);
        return ret;
    }
    g_isp_clk = tmp;

    mdelay(1);
    
    return ret;
}

static int isp_clk_enable(void)
{
    int ret = 0;

    DBG_INFO("");
    if (g_isp_clk != NULL) {
        clk_prepare(g_isp_clk);
        ret = clk_enable(g_isp_clk);  /*enable clk*/
        if (ret) {
            DBG_ERR("si clock enable error (%d)", ret);
        }
        ret = clk_set_rate(g_isp_clk, ISP_MODULE_CLOCK); /*设置isp工作频率*/
    }
    
    return ret;    
}

static void isp_clk_disable(void)
{
    DBG_INFO("");
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

    DBG_INFO("sensor channel:%d", g_sens_channel);
    if(g_sens_channel == 0)
        tmp = clk_get(NULL, CLKNAME_SENSOR_CLKOUT0);
    else
        tmp = clk_get(NULL, CLKNAME_SENSOR_CLKOUT1);
    if (IS_ERR(tmp)) {
        ret = PTR_ERR(tmp);
        DBG_ERR("get isp-channel-%d clock error%d", g_sens_channel, ret);
        return ret;
    }
    g_sensor_clk = tmp;
    DBG_INFO("sensor_clk:%p", g_sensor_clk);
    return ret;
}

static int sensor_clk_enable(void)
{
    int ret;

    DBG_INFO("");
    if(g_sensor_clk == NULL) {
        DBG_ERR("sensor clk not initialized!");
        return -1;
    }

    clk_prepare(g_sensor_clk);
    clk_enable(g_sensor_clk);

    ret = clk_set_rate(g_sensor_clk, CAMERA_MODULE_CLOCK); /*设置sensor频率*/
    if (ret) {
        DBG_ERR("set isp clock error");
    }
    mdelay(10);
    return ret;
}

static void sensor_clk_disable(void)
{
    DBG_INFO("");
    if(g_sensor_clk == NULL) {
        DBG_ERR("sensor clk not initialized!");
        return;
    }

    clk_disable(g_sensor_clk);
    clk_unprepare(g_sensor_clk);
    clk_put(g_sensor_clk);
    module_clk_disable(MOD_ID_CSI);
    g_sensor_clk = NULL;
}

static void sensor_reset(void)
{
    DBG_INFO("");
    set_gpio_level(&g_spinfo.gpio_reset, 1);
    mdelay(10);
    set_gpio_level(&g_spinfo.gpio_reset, 0);
    mdelay(10);
}

static void sensor_power(bool front, bool on)
{
    DBG_INFO("%s sensor power %s", front ? "front" : "rear", on ? "on" : "off");
    if (front) {
        set_gpio_level(&g_spinfo.gpio_front, !on);
        sensor_reset();
    } else {
        set_gpio_level(&g_spinfo.gpio_rear, !on);
    }
    mdelay(10);
}

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

static ssize_t status_show(struct device *dev,  struct device_attribute *attr,  
        char *buf)  
{  
    return sprintf(buf, "%d", camera_detect_status); 
}  

static DEVICE_ATTR(front_name,   0444, front_name_show,   NULL); 
static DEVICE_ATTR(rear_name,    0444, rear_name_show,    NULL);
static DEVICE_ATTR(front_offset, 0444, front_offset_show, NULL); 
static DEVICE_ATTR(rear_offset,  0444, rear_offset_show,  NULL);

static DEVICE_ATTR(status,       0444, status_show,       NULL); 

static int detect_init(void)
{
    struct device_node *fdt_node = NULL;
    int ret = 0;

    DBG_INFO("");

    if(init_common()) {
        return -1;
    }

    ret = isp_clk_init();
    if (ret) {
        DBG_ERR("init isp clock error");
        goto exit;
    }

    ret = sensor_clk_init();
    if (ret) {
        DBG_ERR("init sensor clock error");
        goto exit;
    }

    fdt_node = of_find_compatible_node(NULL, NULL, ISP_FDT_COMPATIBLE);
    if (NULL == fdt_node) {
        DBG_ERR("err: no ["ISP_FDT_COMPATIBLE"] in dts");
        return -1;
    }

    ret = isp_gpio_init(fdt_node, &g_spinfo);
    if (ret) {
        DBG_ERR("pwdn init error!");
        goto exit;
    }

    ret = isp_regulator_init(fdt_node, &g_isp_ir);
    if (ret) {
        DBG_ERR("avdd init error!");
        goto exit;
    }

    ret = isp_clk_enable();
    if (ret) {
        DBG_ERR("enable isp clock error");
        goto exit;
    }

    ret = sensor_clk_enable();
    if (ret) {
        DBG_ERR("enable sensor clock error");
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
    DBG_INFO("");
    isp_clk_disable();
    sensor_clk_disable();
	isp_regulator_disable(&g_isp_ir);
    isp_regulator_exit(&g_isp_ir);
    isp_gpio_exit(&g_spinfo);
    cancel_delayed_work(&g_work);
}

static int detect_process(int id, char * drvname)
{
    struct module_item_t *module_item;
    int ret = -1;

    module_item = &g_module_list[id];
    //module_item->power_on();

    ret = module_item->detect(module_item, g_adap);
    if (ret == 0) {
        sprintf(drvname, "%s.ko", module_item->name);
        DBG_INFO("detect (%s) success!!!!!!!!", module_item->name);
    }
    else
    {
        DBG_INFO("(%s) failed.", module_item->name);
    }

    //module_item->power_off();

    return ret;
}

static int detect_modules(char *drvname, int scan_start, int *offset)
{
    int ret = 0;
    int i = 0;

    DBG_INFO("");
    if (scan_start > 0 && scan_start < ARRAY_SIZE(g_module_list) ) {
        ret = detect_process(scan_start, drvname);
        if (ret == 0) {
            *offset = scan_start;
            return ret;
        }
    }

    for (i = 0; i < ARRAY_SIZE(g_module_list); i++) {
        if (g_module_list[i].need_detect) {
            ret = detect_process(i, drvname);
            if (ret == 0) {
                *offset = i;
                break;
            }
        }
    }

    return ret;
}

static void detect_work(struct work_struct *work)
{
    int ret = 0;

    DBG_INFO("");
    if (!g_front_detected)  {
        DBG_INFO("--------detect front sensor------");
        sensor_power(true, true);
        ret = detect_modules(camera_front_name, camera_front_start, &camera_front_offset);
        if (ret == 0) {
            g_front_detected = true;
        }
        sensor_power(true, false);
    }

    if (!g_rear_detected) {
        DBG_INFO("-------detect rear sensor-------");
        sensor_power(false, true);
        ret = detect_modules(camera_rear_name, camera_rear_start, &camera_rear_offset);
        if (ret == 0) {
            g_rear_detected = true;
        }
        sensor_power(false, false);
    }

    detect_times++;

    // stop detect when:
    // 1. both camera has been detected
    // 2. if hot_plug_enable, detected until satisfy 1, elsewhere
    // 3. detect_times less than 2
    if((!g_front_detected || !g_rear_detected) && (detect_times < 2 || hot_plug_enable)) {
        schedule_delayed_work(&g_work, DELAY_INTERVAL);
    } else {
        camera_detect_status = 1;
        detect_deinit();
    }
}

static int camera_detect_probe(struct platform_device *pdev)
{
    int ret = 0;
    const char *detect_status = NULL;
    struct device_node *fdt_node;
    
    DBG_INFO("");
    fdt_node = of_find_compatible_node(NULL, NULL, CAMERA_DETECT_NAME);
    if (NULL == fdt_node) {
        DBG_ERR("err: no ["CAMERA_DETECT_NAME"] in dts");
        return -1;
    }

    ret = of_property_read_string(fdt_node, "status", &detect_status);
    if (ret == 0 && strcmp(detect_status, "okay") != 0) {
        printk(KERN_DEBUG"%s camera detect disabled by DTS in %s\n",__func__,fdt_node->full_name);
        return -ENODEV;
    }

    ret = detect_init();
    if (ret) {
        DBG_ERR("module detect init error.");
        goto exit;
    }

    rear_kobj = kobject_create_and_add(CAMERA_REAR_NAME, NULL);
    if (rear_kobj == NULL) {  
        DBG_ERR("kobject_create_and_add failed.");
        ret = -ENOMEM;  
        goto kobject_create_err;
    }  
	front_kobj = kobject_create_and_add(CAMERA_FRONT_NAME, NULL);
    if (front_kobj == NULL) {  
        DBG_ERR("kobject_create_and_add failed.");
        ret = -ENOMEM;  
        goto kobject_create_err;
    }  
    
    ret = sysfs_create_file(front_kobj, &dev_attr_front_name.attr);
    if (ret < 0){
        DBG_ERR("sysfs_create_file front_name failed.");
        goto sysfs_create_err;
    }
	 ret = sysfs_create_file(front_kobj, &dev_attr_front_offset.attr);
    if (ret < 0){
        DBG_ERR("sysfs_create_file front_offset failed.");
        goto sysfs_create_err;
    }
    ret = sysfs_create_file(rear_kobj, &dev_attr_rear_name.attr);
    if (ret < 0){
        DBG_ERR("sysfs_create_file rear_name failed.");
        goto sysfs_create_err;
    }
	 ret = sysfs_create_file(rear_kobj, &dev_attr_rear_offset.attr);
    if (ret < 0){
        DBG_ERR("sysfs_create_file rear_offset failed.");
        goto sysfs_create_err;
    }

    ret = sysfs_create_file(rear_kobj, &dev_attr_status.attr);
    if (ret < 0){
        DBG_ERR("sysfs_create_file status failed.");
        goto sysfs_create_err;
    }  
    
    camera_detect_status = 0;
    INIT_DELAYED_WORK(&g_work, detect_work);
    schedule_delayed_work(&g_work, DELAY_INTERVAL);
    
    return 0;
    
sysfs_create_err:  
    kobject_del(rear_kobj);
	kobject_del(front_kobj);

kobject_create_err:

exit:
    detect_deinit();

    return ret;
}

static int camera_detect_suspend(struct platform_device *pdev, pm_message_t m)
{
    DBG_INFO("");
	if((!g_front_detected || !g_rear_detected) && ( hot_plug_enable))
	{
	cancel_delayed_work_sync(&g_work);
	}
    isp_regulator_disable(&g_isp_ir);
    set_gpio_level(&g_spinfo.gpio_front, 0);
    set_gpio_level(&g_spinfo.gpio_rear, 0);
    set_gpio_level(&g_spinfo.gpio_reset, 1);
    
    return 0;
}

static int camera_detect_resume(struct platform_device *pdev)
{
    DBG_INFO("");
    set_gpio_level(&g_spinfo.gpio_front, 1);
    set_gpio_level(&g_spinfo.gpio_rear, 1);
    set_gpio_level(&g_spinfo.gpio_reset, 0);
    isp_regulator_enable(&g_isp_ir);
	if((!g_front_detected || !g_rear_detected) && ( hot_plug_enable))
	{
	schedule_delayed_work(&g_work, DELAY_INTERVAL);
	}
    return 0;
}

static int camera_detect_remove(struct platform_device *pdev)
{
    cancel_delayed_work_sync(&g_work);

    sysfs_remove_file(front_kobj, &dev_attr_front_name.attr);
    sysfs_remove_file(rear_kobj, &dev_attr_rear_name.attr);
    sysfs_remove_file(rear_kobj, &dev_attr_status.attr);
    kobject_del(front_kobj);
	kobject_del(rear_kobj);
    detect_deinit();
    
    return 0;
}

static struct platform_driver camera_detect_driver = {
    .driver = {
        .name  = CAMERA_DETECT_NAME,
        .owner = THIS_MODULE,
    },
    .probe   = camera_detect_probe,
    .suspend = camera_detect_suspend,
    .resume  = camera_detect_resume,
    .remove  = camera_detect_remove,
};

static void detect_device_release(struct device * dev)
{
    return;
}

static struct platform_device camera_detect_device = {
    .name = CAMERA_DETECT_NAME,
    .dev = {
        .release = detect_device_release,
    }
};

/* module function */
static int __init camera_detect_init(void)
{
    int ret = 0;

    DBG_INFO("%s version: %s, 2014-07-16", THIS_MODULE->name, THIS_MODULE->version);
    ret = platform_device_register(&camera_detect_device);
    if (ret) {
        return ret;
    }
    
    ret = platform_driver_register(&camera_detect_driver);
    
    return ret;
}

module_init(camera_detect_init);

static void __exit camera_detect_exit(void)
{
    platform_driver_unregister(&camera_detect_driver);
    platform_device_unregister(&camera_detect_device);
}

module_exit(camera_detect_exit);
module_param(camera_front_start, int, S_IRUSR);
module_param(camera_rear_start, int, S_IRUSR);

MODULE_DESCRIPTION("Camera module detect driver");
MODULE_AUTHOR("Actions-semi");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
