/*
 * drivers/input/touchscreen/gslX680.c
 *
 * Copyright (c) 2012 Shanghai Basewin
 *	Guan Yuwei<guanyuwei@basewin.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */



#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
//#include <mach/gpio.h>
//#include <mach/gpio_data.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>

#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <linux/errno.h>
#include <linux/kernel.h>

#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/gpio.h>
#include <asm/prom.h>
#include <mach/gpio.h>
#include <linux/input/mt.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#include "gsl1680.h"

//#include <mach/irqs.h>
//#include <mach/system.h>
//#include <mach/hardware.h>

#define CFG_TP_USE_CONFIG 1

//default value
#define GSLX680_I2C_NAME 	"gslX680"
#define GSLX680_I2C_ADDR 	0x40

#define TP_RESET_PIN        OWL_GPIO_PORTB(3)
#define TP_I2C_ADAPTER    (1)
#define TP_I2C_ADDR         GSLX680_I2C_ADDR
#define TP_NAME			    GSLX680_I2C_NAME

#if CFG_TP_USE_CONFIG
#define TP_IRQ_PORT	  		OWL_EXT_IRQ_SIRQ0
#endif

#define TP_MAX_X              SCREEN_MAX_X
#define TP_MAX_Y              SCREEN_MAX_Y
static unsigned gpio_reset = 0;

//static struct i2c_client *gsl_ts_device;


#if CFG_TP_USE_CONFIG
struct tp_cfg_dts {
	unsigned int sirq;
	unsigned int i2cNum;
	unsigned int i2cAddr;
	unsigned int xMax;
	unsigned int yMax;
	unsigned int rotate; 
	unsigned int xRevert;
	unsigned int yRevert;
	unsigned int XYSwap;
	char const *regulator;
	unsigned int vol_max;
	unsigned int vol_min;
};
static struct tp_cfg_dts cfg_dts;
#endif

#define RESUME_INIT_CHIP_WORK
//#define GSL_DEBUG

#define REPORT_DATA_PROTOCOL_B

//#define HAVE_TOUCH_KEY
#define SLEEP_CLEAR_POINT
#define FILTER_POINT	/*防抖*/
#define RECORD_POINT
#ifdef FILTER_POINT
#define FILTER_MAX	9	
#endif


#define GSL_DATA_REG		0x80
#define GSL_STATUS_REG		0xe0
#define GSL_PAGE_REG		0xf0

#define PRESS_MAX    		255
#define MAX_FINGERS 		10
#define MAX_CONTACTS 		10	//如果只开5个手指，ID号也可能大于5
#define DMA_TRANS_LEN		0x10	//一次下载多少寄存器，0x20是一页32X4字节

struct mutex mutex;

static int power_is_on = 0;

#define GSL_MONITOR
#ifdef GSL_MONITOR
static struct delayed_work gsl_monitor_work;
static struct workqueue_struct *gsl_monitor_workqueue = NULL;
static char int_1st[4] = {0};
static char int_2nd[4] = {0};
static char bc_counter = 0;
static char b0_counter = 0;
static char i2c_lock_flag = 0;
#endif 
static struct i2c_client *gsl_client = NULL;

static struct gsl_ts *this_ts = NULL;

#ifdef HAVE_TOUCH_KEY
static u16 key = 0;

static int key_state_flag = 0;
struct key_data {
	u16 key;
	u16 x_min;
	u16 x_max;
	u16 y_min;
	u16 y_max;
};

const u16 key_array[]={
	KEY_BACK,
	KEY_HOME,
	KEY_MENU,
	KEY_SEARCH,
}; 
#define MAX_KEY_NUM     (sizeof(key_array)/sizeof(key_array[0]))

struct key_data gsl_key_data[MAX_KEY_NUM] = {
	{KEY_BACK, 2048, 2048, 2048, 2048},
	{KEY_HOME, 2048, 2048, 2048, 2048},	
	{KEY_MENU, 2048, 2048, 2048, 2048},
	{KEY_SEARCH, 2048, 2048, 2048, 2048},
};
#endif

struct gsl_ts_data {
	u8 x_index;
	u8 y_index;
	u8 z_index;
	u8 id_index;
	u8 touch_index;
	u8 data_reg;
	u8 status_reg;
	u8 data_size;
	u8 touch_bytes;
	u8 update_data;
	u8 touch_meta_data;
	u8 finger_size;
};

static struct gsl_ts_data devices[] = {
	{
		.x_index = 6,
		.y_index = 4,
		.z_index = 5,
		.id_index = 7,
		.data_reg = GSL_DATA_REG,
		.status_reg = GSL_STATUS_REG,
		.update_data = 0x4,
		.touch_bytes = 4,
		.touch_meta_data = 4,
		.finger_size = 70,
	},
};

struct gsl_ts {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
	struct workqueue_struct *wq;
#ifdef RESUME_INIT_CHIP_WORK
	struct work_struct init_work;
	struct workqueue_struct *init_wq;
#endif
//    struct work_struct gsl_init_work;
//    struct workqueue_struct *gsl_init_workqueue;

	struct gsl_ts_data *dd;
	u8 *touch_data;
	u8 device_id;
	int irq;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
};

#ifdef GSL_DEBUG 
#define print_info(fmt, args...)   \
        do{                              \
                printk(fmt, ##args);     \
        }while(0)
#else
#define print_info(fmt, args...)
#endif

static u32 id_sign[MAX_CONTACTS+1] = {0};
static u8 id_state_flag[MAX_CONTACTS+1] = {0};
static u8 id_state_old_flag[MAX_CONTACTS+1] = {0};
static u16 x_old[MAX_CONTACTS+1] = {0};
static u16 y_old[MAX_CONTACTS+1] = {0};
static u16 x_new = 0;
static u16 y_new = 0;

////POWER
#define CTP_POWER_ID			("ldo5")
#define CTP_POWER_MIN_VOL	(3100000)
#define CTP_POWER_MAX_VOL	(3110000)
static char ctp_power_name[] = CTP_POWER_ID;
	
volatile int current_val = 0;

static struct regulator *tp_regulator = NULL;
static inline void regulator_deinit(struct regulator *);
static struct regulator *regulator_init(const char *, int, int);

static struct regulator *regulator_init(const char *name, int minvol, int maxvol)
{
	struct regulator *power;
	int ret;

	power = regulator_get(NULL,name);
    	if (IS_ERR(power)) {
		printk("Nova err,regulator_get fail\n!!!");
		return NULL;
	}
 
	if (regulator_set_voltage(power, minvol, maxvol)) {
        	printk("Nova err,cannot set voltage\n!!!");
        	regulator_put(power);
        
		return NULL;
	}
	ret = regulator_enable(power);
	return (power);
}

static inline void regulator_deinit(struct regulator *power)
{
	regulator_disable(power);
	regulator_put(power);
}


static int gslX680_init(void)
{
    gpio_direction_output(gpio_reset, 1);
      	
	return 0;
}

static int gslX680_shutdown_low(void)
{
	gpio_direction_output(gpio_reset, 0);
	return 0;
}

static int gslX680_shutdown_high(void)
{
	gpio_direction_output(gpio_reset, 1);
	return 0;
}

static inline u16 join_bytes(u8 a, u8 b)
{
	u16 ab = 0;
	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}
/*
static u32 gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[2];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].flags = 0x00;
	xfer_msg[0].buf = &reg;

	xfer_msg[1].addr = client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags |= I2C_M_RD;
	xfer_msg[1].buf = buf;

	if (reg < 0x80) {
		i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg));
		msleep(5);
	}

	return i2c_transfer(client->adapter, xfer_msg, ARRAY_SIZE(xfer_msg)) == ARRAY_SIZE(xfer_msg) ? 0 : -EFAULT;
}
*/

static u32 gsl_write_interface(struct i2c_client *client, const u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];

	buf[0] = reg;

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags =0x00;
	xfer_msg[0].buf = buf;

	return i2c_transfer(client->adapter, xfer_msg, 1) == 1 ? 0 : -EFAULT;
}

static int gsl_ts_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	
	tmp_buf[0] = addr;
	bytelen++;
	
	if (datalen != 0 && pdata != NULL)
	{
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}
	
	ret = i2c_master_send(client, tmp_buf, bytelen);
	return ret;
}

static int gsl_ts_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	ret = gsl_ts_write(client, addr, NULL, 0);
	if (ret < 0)
	{
		printk("%s set data address fail!\n", __func__);
		return ret;
	}
	
	return i2c_master_recv(client, pdata, datalen);
}

static __inline__ void fw2buf(u8 *buf, const u32 *fw)
{
	u32 *u32_buf = (int *)buf;
	*u32_buf = *fw;
}

static void gsl_load_fw(struct i2c_client *client)
{
	u8 buf[DMA_TRANS_LEN*4 + 1] = {0};
	u8 send_flag = 1;
	u8 *cur = buf + 1;
	u32 source_line = 0;
	u32 source_len;
	struct fw_data *ptr_fw;
	
	printk("=============gsl_load_fw start==============\n");

	ptr_fw = (struct fw_data *)GSLX680_FW;
	source_len = ARRAY_SIZE(GSLX680_FW);

	for (source_line = 0; source_line < source_len; source_line++) 
	{
		/* init page trans, set the page val */
		if (GSL_PAGE_REG == ptr_fw[source_line].offset)
		{
			fw2buf(cur, &ptr_fw[source_line].val);
			gsl_write_interface(client, GSL_PAGE_REG, buf, 4);
			send_flag = 1;
		}
		else 
		{
			if (1 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20))
	    			buf[0] = (u8)ptr_fw[source_line].offset;

			fw2buf(cur, &ptr_fw[source_line].val);
			cur += 4;

			if (0 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20)) 
			{
	    			gsl_write_interface(client, buf[0], buf, cur - buf - 1);
	    			cur = buf + 1;
			}

			send_flag++;
		}
	}

	printk("=============gsl_load_fw end==============\n");

}


static int test_i2c(struct i2c_client *client)
{
	u8 read_buf = 0;
	u8 write_buf = 0x12;
	int ret, rc = 1;
	
	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if  (ret  < 0)  
    		rc --;
	else
		printk("I read reg 0xf0 is %x\n", read_buf);
	
	msleep(2);
	ret = gsl_ts_write(client, 0xf0, &write_buf, sizeof(write_buf));
	if(ret  >=  0 )
		printk("I write reg 0xf0 0x12\n");
	
	msleep(2);
	ret = gsl_ts_read( client, 0xf0, &read_buf, sizeof(read_buf) );
	if(ret <  0 )
		rc --;
	else
		printk("I read reg 0xf0 is 0x%x\n", read_buf);

	return rc;
}

static void startup_chip(struct i2c_client *client)
{
	u8 tmp = 0x00;

	gsl_ts_write(client, 0xe0, &tmp, 1);
	msleep(10);	
#ifdef GSL_NOID_VERSION
	gsl_DataInit(gsl_config_data_id);
#endif
}

static void reset_chip(struct i2c_client *client)
{
	u8 tmp = 0x88;
	u8 buf[4] = {0x00};
	
	gsl_ts_write(client, 0xe0, &tmp, sizeof(tmp));
	msleep(10);
	tmp = 0x04;
	gsl_ts_write(client, 0xe4, &tmp, sizeof(tmp));
	msleep(5);
	gsl_ts_write(client, 0xbc, buf, sizeof(buf));
	msleep(2);
}

static void clr_reg(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};

	write_buf[0] = 0x88;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(10);
	write_buf[0] = 0x03;
	gsl_ts_write(client, 0x80, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x04;
	gsl_ts_write(client, 0xe4, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x00;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(10);
}

static void init_chip(struct i2c_client *client)
{
	int rc;
	
	gslX680_shutdown_low();	
	msleep(20); 	
	gslX680_shutdown_high();	
	msleep(20); 		
	rc = test_i2c(client);
	if(rc < 0)
	{
		printk("------gslX680 test_i2c error------\n");	
		return;
	}	
	clr_reg(client);
	reset_chip(client);
	gsl_load_fw(client);			
	startup_chip(client);	
	reset_chip(client);	
	startup_chip(client);	
}

static void check_mem_data(struct i2c_client *client)
{
	u8 read_buf[4]  = {0};
	
	msleep(30);
	gsl_ts_read(client,0xb0, read_buf, sizeof(read_buf));
	
	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
	{
		printk("#########check mem read 0xb0 = %x %x %x %x #########\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip(client);
	}
}

#ifdef FILTER_POINT
static void filter_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;
	u16 filter_step_x = 0, filter_step_y = 0;
	
	id_sign[id] = id_sign[id] + 1;
	if(id_sign[id] == 1)
	{
		x_old[id] = x;
		y_old[id] = y;
	}
	
	x_err = x > x_old[id] ? (x -x_old[id]) : (x_old[id] - x);
	y_err = y > y_old[id] ? (y -y_old[id]) : (y_old[id] - y);

	if( (x_err > FILTER_MAX && y_err > FILTER_MAX/3) || (x_err > FILTER_MAX/3 && y_err > FILTER_MAX) )
	{
		filter_step_x = x_err;
		filter_step_y = y_err;
	}
	else
	{
		if(x_err > FILTER_MAX)
			filter_step_x = x_err; 
		if(y_err> FILTER_MAX)
			filter_step_y = y_err;
	}

	if(x_err <= 2*FILTER_MAX && y_err <= 2*FILTER_MAX)
	{
		filter_step_x >>= 2; 
		filter_step_y >>= 2;
	}
	else if(x_err <= 3*FILTER_MAX && y_err <= 3*FILTER_MAX)
	{
		filter_step_x >>= 1; 
		filter_step_y >>= 1;
	}	
	else if(x_err <= 4*FILTER_MAX && y_err <= 4*FILTER_MAX)
	{
		filter_step_x = filter_step_x*3/4; 
		filter_step_y = filter_step_y*3/4;
	}	
	
	x_new = x > x_old[id] ? (x_old[id] + filter_step_x) : (x_old[id] - filter_step_x);
	y_new = y > y_old[id] ? (y_old[id] + filter_step_y) : (y_old[id] - filter_step_y);

	x_old[id] = x_new;
	y_old[id] = y_new;
}
#else
static void record_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;

	id_sign[id]=id_sign[id]+1;
	
	if(id_sign[id]==1){
		x_old[id]=x;
		y_old[id]=y;
	}

	x = (x_old[id] + x)/2;
	y = (y_old[id] + y)/2;
		
	if(x>x_old[id]){
		x_err=x -x_old[id];
	}
	else{
		x_err=x_old[id]-x;
	}

	if(y>y_old[id]){
		y_err=y -y_old[id];
	}
	else{
		y_err=y_old[id]-y;
	}

	if( (x_err > 3 && y_err > 1) || (x_err > 1 && y_err > 3) ){
		x_new = x;     x_old[id] = x;
		y_new = y;     y_old[id] = y;
	}
	else{
		if(x_err > 3){
			x_new = x;     x_old[id] = x;
		}
		else
			x_new = x_old[id];
		if(y_err> 3){
			y_new = y;     y_old[id] = y;
		}
		else
			y_new = y_old[id];
	}

	if(id_sign[id]==1){
		x_new= x_old[id];
		y_new= y_old[id];
	}
	
}
#endif

#ifdef HAVE_TOUCH_KEY
static void report_key(struct gsl_ts *ts, u16 x, u16 y)
{
	u16 i = 0;

	for(i = 0; i < MAX_KEY_NUM; i++) 
	{
		if((gsl_key_data[i].x_min < x) && (x < gsl_key_data[i].x_max)&&(gsl_key_data[i].y_min < y) && (y < gsl_key_data[i].y_max))
		{
			key = gsl_key_data[i].key;	
			input_report_key(ts->input, key, 1);
			input_sync(ts->input); 		
			key_state_flag = 1;
			break;
		}
	}
}
#endif

#if CFG_TP_USE_CONFIG
static int tp_of_data_get(void)
{
    struct device_node *of_node;
    enum of_gpio_flags flags;
    unsigned int scope[2];
    int ret = -1;

    of_node = of_find_compatible_node(NULL, NULL, "gslX680");
    if (of_node==NULL){
        printk(KERN_ERR"%s,%d,find the gsxX680 dts err!\n",__func__, __LINE__);
        return -1;
    }

	/* load tp regulator */
	if (of_find_property(of_node, "tp_vcc", NULL)) {
		ret = of_property_read_string(of_node, "tp_vcc", &cfg_dts.regulator);
		if (ret < 0) {
			printk("can not read tp_vcc power source\n");
			cfg_dts.regulator = ctp_power_name;
		}

		if (of_property_read_u32_array(of_node, "vol_range", scope, 2)) {
			printk(" failed to get voltage range\n");
			scope[0] = CTP_POWER_MIN_VOL;
			scope[1] = CTP_POWER_MAX_VOL;
		}
		cfg_dts.vol_min=scope[0];
		cfg_dts.vol_max=scope[1];
	}
	
	/* load irq number */
    cfg_dts.sirq = irq_of_parse_and_map(of_node,0);
    if (cfg_dts.sirq < 0) {
        printk("No IRQ resource for tp\n");
		return -ENODEV;
	}

	/* load gpio info */
	if (!of_find_property(of_node, "reset_gpios", NULL)) {
		printk("<isp>err: no config gpios\n");
		goto fail;
	}
	gpio_reset = of_get_named_gpio_flags(of_node, "reset_gpios", 0, &flags);

	cfg_dts.i2cNum = TP_I2C_ADAPTER;
    
	/* load tp i2c addr */
	ret = of_property_read_u32(of_node, "reg", &cfg_dts.i2cAddr);
	if (ret) {
		printk(" failed to get i2c_addr\n");
		goto fail;
	}
	
	/* load other options */
	ret = of_property_read_u32(of_node, "x_pixel", &cfg_dts.xMax);
	if (ret) {
		printk("failed to get x_pixel\r\n,set default:1280");
		cfg_dts.xMax = SCREEN_MAX_X;
	}

	ret = of_property_read_u32(of_node, "y_pixel", &cfg_dts.yMax);
	if (ret) {
		printk("failed to get y_pixel\r\n,set default:800");
		cfg_dts.yMax = SCREEN_MAX_Y;
	}

	ret = of_property_read_u32(of_node, "x_revert_en", &cfg_dts.xRevert);
	if (ret) {
		printk("failed to get x_revert_en\r\n,set default:1280");
		cfg_dts.xRevert = 0;
	}

	ret = of_property_read_u32(of_node, "y_revert_en", &cfg_dts.yRevert);
	if (ret) {
		printk("failed to get y_revert_en\r\n,set default:800");
		cfg_dts.yRevert = 0;
	}

	ret = of_property_read_u32(of_node, "xy_swap_en", &cfg_dts.XYSwap);
	if (ret) {
		printk("failed to get xy_swap_en, set default:0\r\n");
		cfg_dts.XYSwap = 0;
	}
    
	ret = of_property_read_u32(of_node, "rotate_degree", &cfg_dts.rotate);
	if (ret) {
		printk("failed to get rotate, set default:0\r\n");
		cfg_dts.rotate = 0;
	}

	
	printk("gpio num:%d, reset level:%d, i2c_addr:%02x, irq_number:%d,x_pixel:%d, y_pixel:%d, max_point:%d, rotate:%d, i2cNum:%d\n",
		gpio_reset,
		0,
		cfg_dts.i2cAddr,
		cfg_dts.sirq,
		cfg_dts.xMax,
		cfg_dts.yMax,
		5,
		cfg_dts.rotate,
		cfg_dts.i2cNum);
    
        return 0;

fail:
	return -1;
    
}

#endif

static void report_data(struct gsl_ts *ts, u16 x, u16 y, u8 pressure, u8 id)
{
#ifdef HAVE_TOUCH_KEY
	if(x > cfg_dts.xMax || y > cfg_dts.yMax)
	{
		report_key(ts,x,y);
		return;
	}
#endif

#ifdef REPORT_DATA_PROTOCOL_B
	input_mt_slot(ts->input, id);	
	input_mt_report_slot_state(ts->input, MT_TOOL_FINGER, true);
	input_report_abs(ts->input, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
#endif
/* A protocol*/
}

static void gslX680_ts_worker(struct work_struct *work)
{
	int rc, i;
	u8 id, touches;
	u16 x, y;
	
	struct gsl_ts *ts = container_of(work, struct gsl_ts,work);
	
#ifdef GSL_NOID_VERSION
	u32 tmp1;
	u8 buf[4] = {0};
	//struct gsl_touch_info cinfo = {0};
	struct gsl_touch_info cinfo;
#endif

#ifdef GSL_MONITOR
	if(i2c_lock_flag != 0)
		goto i2c_lock_schedule;
	else
		i2c_lock_flag = 1;
#endif

	mutex_lock(&mutex);

	print_info("=====gslX680_ts_worker=====\n");

	rc = gsl_ts_read(ts->client, 0x80, ts->touch_data, ts->dd->data_size);

	if (rc < 0) 
	{
		dev_err(&ts->client->dev, "read failed.------func:%s,line:%d\n",__func__,__LINE__);
		goto schedule;
	}
		
	touches = ts->touch_data[ts->dd->touch_index];
	print_info("-----touches: %d -----\n", touches);		
#ifdef GSL_NOID_VERSION
	cinfo.finger_num = touches;
	print_info("tp-gsl  finger_num = %d\n",cinfo.finger_num);
	for(i = 0; i < (touches < MAX_CONTACTS ? touches : MAX_CONTACTS); i ++)
	{
		cinfo.x[i] = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		cinfo.y[i] = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i ]);
		print_info("tp-gsl  x = %d y = %d \n",cinfo.x[i],cinfo.y[i]);
	}
	cinfo.finger_num=(ts->touch_data[3]<<24)|(ts->touch_data[2]<<16)
		|(ts->touch_data[1]<<8)|(ts->touch_data[0]);
	gsl_alg_id_main(&cinfo);
	tmp1=gsl_mask_tiaoping();
	print_info("[tp-gsl] tmp1=%x\n",tmp1);
	if(tmp1>0&&tmp1<0xffffffff)
	{
		buf[0]=0xa;buf[1]=0;buf[2]=0;buf[3]=0;
		gsl_ts_write(ts->client,0xf0,buf,4);
		buf[0]=(u8)(tmp1 & 0xff);
		buf[1]=(u8)((tmp1>>8) & 0xff);
		buf[2]=(u8)((tmp1>>16) & 0xff);
		buf[3]=(u8)((tmp1>>24) & 0xff);
		print_info("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n",
			tmp1,buf[0],buf[1],buf[2],buf[3]);
		gsl_ts_write(ts->client,0x8,buf,4);
	}
	touches = cinfo.finger_num;
#endif
	
	//for(i = 1; i <= MAX_CONTACTS; i ++)
	for(i = 0; i < MAX_CONTACTS; i ++)
	{
		if(touches == 0)
			id_sign[i] = 0;	
		id_state_flag[i] = 0;
	}
	for(i= 0;i < (touches > MAX_FINGERS ? MAX_FINGERS : touches);i ++)
	{
	#ifdef GSL_NOID_VERSION
		id = cinfo.id[i]-1;//sunjl added,for slot id should be 0-9,not 1-10.
		y =  cinfo.x[i];
		x =  cinfo.y[i];
	#else
		y = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		x = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i ]);
		id = ts->touch_data[ts->dd->id_index + 4 * i] >> 4;
	#endif
		x=x*cfg_dts.xMax/2048;
		y=y*cfg_dts.yMax/2048;
	//printk("raw(x,y):(%d,%d)\n",x,y);
	#if CFG_TP_USE_CONFIG
    if (cfg_dts.XYSwap == 1)
    {
        int tmp;
        tmp=x;
        x=y;
        y=tmp;
    }
    
    if(cfg_dts.xRevert == 1)
    {   
        x = cfg_dts.xMax - x;
    }
    
    if(cfg_dts.yRevert == 1)
    {
        y = cfg_dts.yMax - y;
    }
    
    
    if(cfg_dts.rotate == 90) //anticlockwise 90 angle
    {
        int tmp;
        tmp = x;
        x = y;
        y = cfg_dts.xMax-tmp;
    }else if(cfg_dts.rotate == 180) //anticlockwise 180 angle
    {
        x = cfg_dts.xMax - x;
        y = cfg_dts.yMax - y;
    } else if(cfg_dts.rotate == 270) //anticlockwise 270 angle
    {
        int tmp;
        tmp = x;
        x = cfg_dts.yMax-y;
        y = tmp;
    }

	#endif
	//printk("new(x,y):(%d,%d)\n",x,y);

		if(0 <=id && id < MAX_CONTACTS)
		{
		#ifdef FILTER_POINT
			filter_point(x, y ,id);
			report_data(ts, x_new, y_new, 10, id);		
		#else 
			#if defined(RECORD_POINT)
				record_point(x, y , id);
				report_data(ts, x_new, y_new, 10, id);		
			#else
				report_data(ts, x, y, 10, id);		
			#endif
		#endif
			id_state_flag[id] = 1;
		}
	}
	
	for(i = 0; i < MAX_CONTACTS; i++)
	{	
		if( (0 == touches) || ((0 != id_state_old_flag[i]) && (0 == id_state_flag[i])) )
		{
		#ifdef REPORT_DATA_PROTOCOL_B
			input_mt_slot(ts->input, i);
			//input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(ts->input, MT_TOOL_FINGER, false);
		#endif
			id_sign[i]=0;
		}
		id_state_old_flag[i] = id_state_flag[i];
	}
/* A protocol*/

	input_sync(ts->input);

schedule:
	mutex_unlock(&mutex); 
#ifdef GSL_MONITOR
	i2c_lock_flag = 0;
i2c_lock_schedule:
#endif
	//enable_irq(ts->irq);

	print_info("=====gslX680_ts_worker end=====\n");
	return;	
}

#ifdef GSL_MONITOR
static void gsl_monitor_worker(struct work_struct *data)
{
	//char write_buf[4] = {0};
	char read_buf[4]  = {0};
	char download_flag = 0;
	
	print_info("----------------gsl_monitor_worker-----------------\n");	

	if(i2c_lock_flag != 0)
		goto queue_monitor_work;
	else
		i2c_lock_flag = 1;
		mutex_lock(&mutex); 
	gsl_ts_read(gsl_client, 0xb0, read_buf, 4);
	if(read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
		b0_counter ++;
	else
		b0_counter = 0;

	if(b0_counter > 1)
	{
		printk("======read 0xb0: %x %x %x %x ======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		download_flag = 1;
		b0_counter = 0;
		goto DOWNLOAD_FW;
	}
	
	gsl_ts_read(gsl_client, 0xb4, read_buf, 4);	
	int_2nd[3] = int_1st[3];
	int_2nd[2] = int_1st[2];
	int_2nd[1] = int_1st[1];
	int_2nd[0] = int_1st[0];
	int_1st[3] = read_buf[3];
	int_1st[2] = read_buf[2];
	int_1st[1] = read_buf[1];
	int_1st[0] = read_buf[0];

	if(int_1st[3] == int_2nd[3] && int_1st[2] == int_2nd[2] &&int_1st[1] == int_2nd[1] && int_1st[0] == int_2nd[0]) 
	{
		printk("======int_1st: %x %x %x %x , int_2nd: %x %x %x %x ======\n",int_1st[3], int_1st[2], int_1st[1], int_1st[0], int_2nd[3], int_2nd[2],int_2nd[1],int_2nd[0]);
		download_flag = 1;
		goto DOWNLOAD_FW;
	}

	gsl_ts_read(gsl_client, 0xbc, read_buf, 4);
	if(read_buf[3] != 0 || read_buf[2] != 0 || read_buf[1] != 0 || read_buf[0] != 0)
		bc_counter ++;
	else
		bc_counter = 0;

	if(bc_counter > 1) 
	{
		printk("======read 0xbc: %x %x %x %x ======\n",read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		download_flag = 1;
		bc_counter = 0;
	}
DOWNLOAD_FW:
	if(1 == download_flag)
	{		
		init_chip(gsl_client);
	}
	
	i2c_lock_flag = 0;
	
	mutex_unlock(&mutex); 
	
queue_monitor_work:
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 200);
}
#endif

#ifdef RESUME_INIT_CHIP_WORK
static void gslX680_init_worker (struct work_struct *work)
{
	init_chip(this_ts->client);
	check_mem_data(this_ts->client);
}
#endif

static irqreturn_t gsl_ts_irq(int irq, void *dev_id)
{	
	struct gsl_ts *ts = dev_id;
	print_info("========gslX680 Interrupt=========\n");				 

	if (!work_pending(&ts->work)) {
		queue_work(ts->wq, &ts->work);
	}else{
		//printk("[GSLX680] work is pending,lost the interrupt.\n");
	}
	return IRQ_HANDLED;
}

#if CFG_TP_USE_CONFIG

/********************TP DEBUG************************/

/**************************************************************************/
static ssize_t tp_rotate_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n",cfg_dts.rotate);
}
/**************************************************************************/
static ssize_t tp_rotate_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data=0;
	int error;
    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;
	cfg_dts.rotate=data;
    return count;
}
/**************************************************************************/
static ssize_t tp_xrevert_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n",cfg_dts.xRevert);
}
/**************************************************************************/
static ssize_t tp_xrevert_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data=0;
	int error;
    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;
	cfg_dts.xRevert=data;
    return count;
}

/**************************************************************************/
static ssize_t tp_yrevert_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n",cfg_dts.yRevert);
}
/**************************************************************************/
static ssize_t tp_yrevert_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data=0;
	int error;
    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;
	cfg_dts.yRevert=data;
    return count;
}

/**************************************************************************/
static ssize_t tp_xyswap_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n",cfg_dts.XYSwap);
}
/**************************************************************************/
static ssize_t tp_xyswap_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data=0;
	int error;
    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;
	cfg_dts.XYSwap=data;
    return count;
}

static DEVICE_ATTR(tp_rotate, S_IWUSR|S_IWGRP|S_IRUSR|S_IRGRP,tp_rotate_show, tp_rotate_store);
static DEVICE_ATTR(tp_xrevert, S_IWUSR|S_IWGRP|S_IRUSR|S_IRGRP,tp_xrevert_show, tp_xrevert_store);
static DEVICE_ATTR(tp_yrevert, S_IWUSR|S_IWGRP|S_IRUSR|S_IRGRP,tp_yrevert_show, tp_yrevert_store);
static DEVICE_ATTR(tp_xyswap, 	S_IWUSR|S_IWGRP|S_IRUSR|S_IRGRP,tp_xyswap_show, tp_xyswap_store);

static struct attribute *tp_attributes[] = { 
    &dev_attr_tp_rotate.attr,
	 &dev_attr_tp_xrevert.attr,
	  &dev_attr_tp_yrevert.attr,
	  &dev_attr_tp_xyswap.attr,
    NULL
};

static const struct attribute_group tp_attr_group = {
    .attrs  = tp_attributes,
};
#endif

static int gslX680_ts_init(struct i2c_client *client, struct gsl_ts *ts)
{
	struct input_dev *input_device;
	int rc = 0;
	
	printk("[GSLX680] Enter %s\n", __func__);

	ts->dd = &devices[ts->device_id];

	if (ts->device_id == 0) {
		ts->dd->data_size = MAX_FINGERS * ts->dd->touch_bytes + ts->dd->touch_meta_data;
		ts->dd->touch_index = 0;
	}

	ts->touch_data = kzalloc(ts->dd->data_size, GFP_KERNEL);
	if (!ts->touch_data) {
		pr_err("%s: Unable to allocate memory\n", __func__);
		return -ENOMEM;
	}

	input_device = input_allocate_device();
	if (!input_device) {
		rc = -ENOMEM;
		goto error_alloc_dev;
	}

	ts->input = input_device;
	input_device->name = TP_NAME;
	input_device->id.bustype = BUS_I2C;
	input_device->dev.parent = &client->dev;
	input_set_drvdata(input_device, ts);

	__set_bit(EV_ABS,input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_mt_init_slots(input_device, MAX_CONTACTS, 0);
	input_set_abs_params(input_device, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_device, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);

#if CFG_TP_USE_CONFIG
	if(cfg_dts.rotate == 90 || cfg_dts.rotate == 270)
	{
		input_set_abs_params(input_device, ABS_MT_POSITION_Y, 0, cfg_dts.xMax, 0, 0);
		input_set_abs_params(input_device, ABS_MT_POSITION_X, 0, cfg_dts.yMax, 0, 0);
	}
	else{
		input_set_abs_params(input_device, ABS_MT_POSITION_X, 0, cfg_dts.xMax, 0, 0);
		input_set_abs_params(input_device, ABS_MT_POSITION_Y, 0, cfg_dts.yMax, 0, 0);
	}
#else
    input_set_abs_params(input_device, ABS_MT_POSITION_X, 0, TP_MAX_X, 0, 0);
    input_set_abs_params(input_device, ABS_MT_POSITION_Y, 0, TP_MAX_Y, 0, 0);
#endif

#ifdef HAVE_TOUCH_KEY
	set_bit(EV_KEY, input_device->evbit);
	input_device->evbit[0] = BIT_MASK(EV_KEY);
	for (i = 0; i < MAX_KEY_NUM; i++)
		set_bit(key_array[i], input_device->keybit);
#endif

#if CFG_TP_USE_CONFIG
	client->irq = cfg_dts.sirq;
#else
		client->irq = TP_IRQ_PORT;
#endif
	ts->irq = client->irq;

	ts->wq = create_singlethread_workqueue("kworkqueue_ts");
	if (!ts->wq) {
		dev_err(&client->dev, "Could not create workqueue\n");
		goto error_wq_create;
	}
	flush_workqueue(ts->wq);	

	INIT_WORK(&ts->work, gslX680_ts_worker);
	
#ifdef RESUME_INIT_CHIP_WORK	
	ts->init_wq = create_singlethread_workqueue("ts_init_wq");
	if (!ts->init_wq) {
		dev_err(&client->dev, "Could not create ts_init_wq workqueue\n");
		goto error_wq_create;
	}
	flush_workqueue(ts->init_wq);	
	INIT_WORK(&ts->init_work, gslX680_init_worker);
#endif
	rc = input_register_device(input_device);
	if (rc)
		goto error_unreg_device;
	
	this_ts = ts;

#if CFG_TP_USE_CONFIG
	if (sysfs_create_group(&input_device->dev.kobj, &tp_attr_group) < 0){
	    printk("create tp sysfs group error!");		
	}
#endif
	return 0;

error_unreg_device:
	destroy_workqueue(ts->wq);
error_wq_create:
	input_free_device(input_device);
error_alloc_dev:
	kfree(ts->touch_data);
	return rc;
}

static int gsl_ts_suspend(struct device *dev)
{
	flush_workqueue(this_ts->init_wq);
    flush_work(&this_ts->init_work);

	printk("[GSLX680] Enter %s ########%d\n", __func__,__LINE__);
	gslX680_shutdown_low();
	if (tp_regulator){
		 current_val = regulator_get_voltage(tp_regulator);
		 regulator_disable(tp_regulator);
		 printk("Nova disable regulator %d\n",current_val);
         power_is_on = 0;
	}
	return 0;
}

static int gsl_ts_resume_early(struct device *dev)
{
    int ret;

    //printk("[GSLX680] Enter %s ########%d\n", __func__,__LINE__);
    pr_info("[GSLX680] Enter %s ########%d\n", __func__,__LINE__);
    if(1 == power_is_on)
    {
        printk("impossible: power is turned on!\n");
        return 0;
    }

	if (tp_regulator)
	{
	    regulator_set_voltage(tp_regulator, CTP_POWER_MIN_VOL, CTP_POWER_MAX_VOL);
	    ret = regulator_enable(tp_regulator);
	}

#ifdef RESUME_INIT_CHIP_WORK
	queue_work(this_ts->init_wq, &this_ts->init_work);
#endif

    power_is_on = 1;
    return 0;
}

static int gsl_ts_resume(struct device *dev)
{
	int ret;

	//printk("[GSLX680] Enter %s ########%d\n", __func__,__LINE__);
	pr_info("[GSLX680] Enter %s ########%d\n", __func__,__LINE__);

    if(1 == power_is_on)
    {
        printk(" power is turned on!\n");
        return 0;
    }
        
	if (tp_regulator)
	{
	    regulator_set_voltage(tp_regulator, CTP_POWER_MIN_VOL, CTP_POWER_MAX_VOL);
	    ret = regulator_enable(tp_regulator);
	}

	#ifdef RESUME_INIT_CHIP_WORK
	queue_work(this_ts->init_wq, &this_ts->init_work);
	#endif
    power_is_on = 1;
    return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void gsl_ts_early_suspend(struct early_suspend *h)
{
	int i=0;

	printk("[GSLX680] Enter %s ########%d\n", __func__,__LINE__);
	
	disable_irq(this_ts->irq);
	flush_workqueue(this_ts->wq);
    flush_work(&this_ts->work);
    
	#ifdef GSL_MONITOR
	printk( "gsl_ts_suspend () : cancel gsl_monitor_work\n");
	cancel_delayed_work_sync(&gsl_monitor_work);
	#endif

#ifdef SLEEP_CLEAR_POINT
	#ifdef REPORT_DATA_PROTOCOL_B
	for(i =1;i<=MAX_CONTACTS;i++)
	{	
		input_mt_slot(this_ts->input, i);
		input_mt_report_slot_state(this_ts->input, MT_TOOL_FINGER, false);
	}
	#endif
	input_sync(this_ts->input);	
#endif
}

static void gsl_ts_late_resume(struct early_suspend *h)
{
	enable_irq(this_ts->irq);
	
	//printk("[GSLX680] Enter %s ########%d\n", __func__,__LINE__);
	pr_info("[GSLX680] Enter %s ########%d\n", __func__,__LINE__);
	#ifdef GSL_MONITOR
	printk( "gsl_ts_resume () : queue gsl_monitor_work\n");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 500);
	#endif
}
#endif

#if 0  // used
static void gsl_init_work_func(struct work_struct *work)
{
	struct gsl_ts *ts = container_of(work, struct gsl_ts,gsl_init_work);

	gslX680_init();
	init_chip(ts->client);
	check_mem_data(ts->client);
}
#endif

static int gsl_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct gsl_ts *ts;
	int rc;

	printk("[GSLX680] Enter %s ########%d\n", __func__,__LINE__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -ENODEV;
	}
 
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (!ts){
		printk("kzalloc failed.\n");
		return -ENOMEM;
	}
	gsl_client = client;
	ts->client = client;
	i2c_set_clientdata(client, ts);
	ts->device_id = id->driver_data;

	rc = gslX680_ts_init(client, ts);
	if (rc < 0) {
		dev_err(&client->dev, "GSLX680 init failed\n");
		goto error_mutex_destroy;
	}	

	gslX680_init();
	init_chip(ts->client);
	check_mem_data(ts->client);

	mutex_init(&mutex); 
	//INIT_WORK(&ts->gsl_init_work, gsl_init_work_func);
	//ts->gsl_init_workqueue = create_singlethread_workqueue("gsl_init_workqueue");
	//queue_work(ts->gsl_init_workqueue, &ts->gsl_init_work);

	rc=  request_irq(client->irq, gsl_ts_irq, IRQF_TRIGGER_RISING | IRQF_DISABLED, client->name, ts);
	if (rc < 0) {
		printk( "gsl_probe: request irq failed\n");
		goto error_req_irq_fail;
	}
	
	/* create debug attribute */
	//rc = device_create_file(&ts->input->dev, &dev_attr_debug_enable);

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	//ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	ts->early_suspend.suspend = gsl_ts_early_suspend;
	ts->early_suspend.resume = gsl_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
	#ifdef GSL_MONITOR
	printk( "gsl_ts_probe () : queue gsl_monitor_workqueue\n");
	INIT_DELAYED_WORK(&gsl_monitor_work, gsl_monitor_worker);
	gsl_monitor_workqueue = create_singlethread_workqueue("gsl_monitor_workqueue");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 500);
	#endif
	
	printk("[GSLX680] End %s\n", __func__);

	return 0;

//exit_set_irq_mode:	
error_req_irq_fail:
	free_irq(ts->irq, ts);	

error_mutex_destroy:
	input_free_device(ts->input);
	kfree(ts);
	return rc;
}

static int gsl_ts_remove(struct i2c_client *client)
{
	struct gsl_ts *ts = i2c_get_clientdata(client);
	printk("==gsl_ts_remove=\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
	//cancel_work_sync(&ts->gsl_init_work);
	//destroy_workqueue(ts->gsl_init_workqueue);

	device_init_wakeup(&client->dev, 0);
	cancel_work_sync(&ts->work);
	free_irq(ts->irq, ts);
	destroy_workqueue(ts->wq);

	#ifdef RESUME_INIT_CHIP_WORK
	cancel_work_sync(&ts->init_work);
	destroy_workqueue(ts->init_wq);
	#endif

#if CFG_TP_USE_CONFIG
	sysfs_remove_group(&ts->input->dev.kobj, &tp_attr_group);
#endif
	input_unregister_device(ts->input);
	//device_remove_file(&ts->input->dev, &dev_attr_debug_enable);
	#ifdef GSL_MONITOR
	cancel_delayed_work_sync(&gsl_monitor_work);
	destroy_workqueue(gsl_monitor_workqueue);
	#endif

	kfree(ts->touch_data);
	kfree(ts);

	return 0;
}

static void gsl_ts_shutdown(struct i2c_client *client)
{
	printk("==gsl_ts_shutdown==\n");
    gsl_ts_remove(client);
	if ( tp_regulator ){
		regulator_deinit(tp_regulator);
		tp_regulator = NULL;
	}
}

static const struct i2c_device_id gsl_ts_id[] = {
	{GSLX680_I2C_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gsl_ts_id);
static unsigned short gsl_addresses[] = {
	GSLX680_I2C_ADDR,
	I2C_CLIENT_END,
};

static struct dev_pm_ops tp_pm_ops = {
    .resume_early = gsl_ts_resume_early,
    .suspend       = gsl_ts_suspend,
    .resume = gsl_ts_resume,
};

static struct of_device_id gsl_of_match[] = {
	{ .compatible = "gslX680" },
	{ }
};

static struct i2c_driver gsl_ts_driver = {
	.driver = {
		.name = GSLX680_I2C_NAME,
		.owner = THIS_MODULE,
		.pm = &tp_pm_ops,
		.of_match_table	= of_match_ptr(gsl_of_match),
	},
	.probe		= gsl_ts_probe,
	.remove		= gsl_ts_remove,
	.shutdown	=	gsl_ts_shutdown,
	.id_table	= gsl_ts_id,
	.address_list	= gsl_addresses,
};
static struct i2c_board_info tp_info = {
	.type	= GSLX680_I2C_NAME,
};

#if CFG_TP_USE_CONFIG
static int tp_config_init(void)
{
    cfg_dts.rotate=TP_ROTATE_DEFAULT;
    cfg_dts.xMax=TP_MAX_X;
    cfg_dts.yMax=TP_MAX_Y;
    cfg_dts.xRevert=TP_XREVERT;
    cfg_dts.yRevert=TP_YREVERT;
    cfg_dts.XYSwap=TP_XYSWAP;
    return 0;
}
#endif

static int __init gsl_ts_init(void)
{
	//struct i2c_adapter *adap = NULL;
	int ret;
	
	printk("==gsl_ts_init==\n");
	
#if CFG_TP_USE_CONFIG
	tp_config_init();

	ret = tp_of_data_get();
	if(ret<0) {
		printk("Please complete the TP configuration item!!!\n\n");
	}
	tp_info.addr = cfg_dts.i2cAddr;

	tp_regulator = regulator_init(cfg_dts.regulator, 
   		cfg_dts.vol_min, cfg_dts.vol_max);
#else
	gpio_reset = TP_RESET_PIN;
	tp_info.addr = TP_I2C_ADDR;

	tp_regulator = regulator_init(CTP_POWER_ID, 
   		CTP_POWER_MIN_VOL, CTP_POWER_MAX_VOL);
#endif

	if ( !tp_regulator ) {
	   printk("Nova tp init power failed");
	   ret = -EINVAL;
	   return ret;
	}

	gpio_request(gpio_reset, GSLX680_I2C_NAME);

#if 0
#if CFG_TP_USE_CONFIG
	adap = i2c_get_adapter(cfg_dts.i2cNum);
#else
	adap = i2c_get_adapter(TP_I2C_ADAPTER);
#endif
	gsl_ts_device = i2c_new_device(adap, &tp_info); 
#endif

	ret = i2c_add_driver(&gsl_ts_driver);
	printk("i2c_add_driver,ret=%d\n",ret);
	return ret;
}

static void __exit gsl_ts_exit(void)
{
	printk("==gsl_ts_exit==\n");
	
	i2c_del_driver(&gsl_ts_driver);
	if ( tp_regulator ){
		regulator_deinit(tp_regulator);
		tp_regulator = NULL;
	}
	gpio_free(gpio_reset);
    #if 0
	if(gsl_ts_device){
		i2c_unregister_device(gsl_ts_device);
		gsl_ts_device = NULL;
	}
    #endif
	return;
}

module_init(gsl_ts_init);
module_exit(gsl_ts_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GSLX680 touchscreen controller driver");
MODULE_AUTHOR("Guan Yuwei, guanyuwei@basewin.com");
MODULE_ALIAS("platform:gsl_ts");

