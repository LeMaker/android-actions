/* For ACTIONS android platform.
 * 
 * mir3da.c - Linux kernel modules for 3-Axis Accelerometer
 *
 * Copyright (C) 2011-2013 MiraMEMS Sensing Technology Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>  
#include <linux/ktime.h> 
//#include "../gsensor_common.h"
#include "mir3da_core.h"
#include "mir3da_cust.h"

#define MI_DATA(format, ...)            if(DEBUG_DATA&Log_level){printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_MSG(format, ...)             if(DEBUG_MSG&Log_level){printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_ERR(format, ...)             if(DEBUG_ERR&Log_level){printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);}
#define MI_FUN                          if(DEBUG_FUNC&Log_level){printk(KERN_ERR MI_TAG "%s is called, line: %d\n", __FUNCTION__,__LINE__);}
#define MI_ASSERT(expr)              	if (!(expr)) {printk(KERN_ERR "Assertion failed! %s,%d,%s,%s\n",__FILE__, __LINE__, __func__, #expr);}
//  for XML config
//#if CFG_GSENSOR_USE_CONFIG
#define CFG_GSENSOR_ADAP_ID          "gsensor.i2c_adap_id"
#define CFG_GSENSOR_POSITION         "gsensor.position"
#define CFG_GSENSOR_CALIBRATION   "gsensor.calibration"
#define CFG_GSENSOR_MOD_POSITION    "gsensor_"MIR3DA_DRV_NAME".position"

//extern int get_config(const char *key, char *buff, int len);*/
//#endif
//#define CFG_GSENSOR_CALIBRATION      "gsensor.calibration"
#define CFG_GSENSOR_USE_CONFIG				1
#define CFG_GSENSOR_CALIBFILE   "/data/data/com.actions.sensor.calib/files/gsensor_calib.txt"

#define MIR3DA_AUTO_CALIBRAE 0

#define MIR3DA_HRTIMER 1

struct mir3da_acc{
    int    x;
    int    y;
    int    z;
} ;

struct mir3da_data {
    struct i2c_client *mir3da_client;
    struct input_dev *input;
    atomic_t delay;
    atomic_t enable;
    struct mutex enable_mutex;
    struct hrtimer hr_timer;
    struct workqueue_struct *wq;
    struct delayed_work work;
    atomic_t position;
    atomic_t calibrated;
    struct mir3da_acc offset;
};

static MIR_HANDLE             		mir_handle;
extern int                           	Log_level;
//static struct i2c_board_info            mir3da_i2c_boardinfo = { I2C_BOARD_INFO(MIR3DA_DRV_NAME, MIR3DA_I2C_ADDR) };

/**************************************************************************/
#if MIR3DA_OFFSET_TEMP_SOLUTION
static char OffsetFileName[] = "/data/miraGSensorOffset.txt";
#define OFFSET_STRING_LEN               26
struct work_info
{
    char        tst1[20];
    char        tst2[20];
    char        buffer[OFFSET_STRING_LEN];
    struct      workqueue_struct *wq;
    struct      delayed_work read_work;
    struct      delayed_work write_work;
    struct      completion completion;
    int         len;
    int         rst;
};
/**************************************************************************/
static struct work_info m_work_info = {{0}};
static void sensor_write_work( struct work_struct *work )
{
    struct work_info*   pWorkInfo;
    struct file         *filep;
    u32                 orgfs;
    int                 ret;   

    orgfs = get_fs();
    set_fs(KERNEL_DS);

    pWorkInfo = container_of((struct delayed_work*)work, struct work_info, write_work);
    if (pWorkInfo == NULL){            
            MI_ERR("get pWorkInfo failed!");       
            return;
    }
    
    filep = filp_open(OffsetFileName, O_RDWR|O_CREAT, 0600);
    if (IS_ERR(filep)){
        MI_ERR("write, sys_open %s error!!.\n", OffsetFileName);
        ret =  -1;
    }
    else
    {   
        filep->f_op->write(filep, pWorkInfo->buffer, pWorkInfo->len, &filep->f_pos);
        filp_close(filep, NULL);
        ret = 0;        
    }
    
    set_fs(orgfs);   
    pWorkInfo->rst = ret;
    complete( &pWorkInfo->completion );
}
/**************************************************************************/
static void sensor_read_work( struct work_struct *work )
{
    u32 orgfs;
    struct file *filep;
    int ret; 
    struct work_info* pWorkInfo;
        
    orgfs = get_fs();
    set_fs(KERNEL_DS);
    
    pWorkInfo = container_of((struct delayed_work*)work, struct work_info, read_work);
    if (pWorkInfo == NULL){            
        MI_ERR("get pWorkInfo failed!");       
        return;
    }
 
    filep = filp_open(OffsetFileName, O_RDONLY, 0600);
    if (IS_ERR(filep)){
        //MI_ERR("read, sys_open %s error!!.\n",OffsetFileName);
        set_fs(orgfs);
        ret =  -1;
    }
    else{
        filep->f_op->read(filep, pWorkInfo->buffer,  sizeof(pWorkInfo->buffer), &filep->f_pos);
        filp_close(filep, NULL);    
        set_fs(orgfs);
        ret = 0;
    }

    pWorkInfo->rst = ret;
    MI_MSG("pWorkInfo->rst = %d\n", pWorkInfo->rst );
    complete( &(pWorkInfo->completion) );
}
/**************************************************************************/
static int sensor_sync_read(u8* offset)
{
    int     err;
    int     off[MIR3DA_OFFSET_LEN] = {0};
    struct work_info* pWorkInfo = &m_work_info;
     
    init_completion( &pWorkInfo->completion );
    queue_delayed_work( pWorkInfo->wq, &pWorkInfo->read_work, msecs_to_jiffies(0) );
    err = wait_for_completion_timeout( &pWorkInfo->completion, msecs_to_jiffies( 2000 ) );
    if ( err == 0 ){
        MI_ERR("wait_for_completion_timeout TIMEOUT");
        return -1;
    }

    if (pWorkInfo->rst != 0){
        //MI_ERR("work_info.rst  not equal 0");
        return pWorkInfo->rst;
    }
    
    sscanf(m_work_info.buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x", &off[0], &off[1], &off[2], &off[3], &off[4], &off[5],&off[6], &off[7], &off[8]);

    offset[0] = (u8)off[0];
    offset[1] = (u8)off[1];
    offset[2] = (u8)off[2];
    offset[3] = (u8)off[3];
    offset[4] = (u8)off[4];
    offset[5] = (u8)off[5];
    offset[6] = (u8)off[6];
    offset[7] = (u8)off[7];
    offset[8] = (u8)off[8];
    
    return 0;
}
/**************************************************************************/
static int sensor_sync_write(u8* off)
{
    int err = 0;
    struct work_info* pWorkInfo = &m_work_info;
       
    init_completion( &pWorkInfo->completion );
    
    sprintf(m_work_info.buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x\n", off[0],off[1],off[2],off[3],off[4],off[5],off[6],off[7],off[8]);
    
    pWorkInfo->len = sizeof(m_work_info.buffer);
        
    queue_delayed_work( pWorkInfo->wq, &pWorkInfo->write_work, msecs_to_jiffies(0) );
    err = wait_for_completion_timeout( &pWorkInfo->completion, msecs_to_jiffies( 2000 ) );
    if ( err == 0 ){
        MI_ERR("wait_for_completion_timeout TIMEOUT");
        return -1;
    }

    if (pWorkInfo->rst != 0){
        //MI_ERR("work_info.rst  not equal 0");
        return pWorkInfo->rst;
    }
    
    return 0;
}
#endif
/**************************************************************************/
#if MIR3DA_AUTO_CALIBRAE
static bool check_califile_exist(void)
{
    u32     orgfs = 0;
    struct  file *filep;
        
    orgfs = get_fs();
    set_fs(KERNEL_DS);

    filep = filp_open(OffsetFileName, O_RDONLY, 0600);
    if (IS_ERR(filep)) {
        //MI_ERR("%s read, sys_open %s error!!.\n",__func__,OffsetFileName);
        set_fs(orgfs);
        return false;
    }

    filp_close(filep, NULL);    
    set_fs(orgfs); 

    return true;
}
#endif
/**************************************************************************/
static ssize_t mir3da_enable_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
    int             ret;
    char            bEnable;
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);	
	
    ret = mir3da_get_enable(mir3da->mir3da_client, &bEnable);    
    if (ret < 0){
        ret = -EINVAL;
    }
    else{
        ret = sprintf(buf, "%d\n", bEnable);
    }

    return ret;
}
/**************************************************************************/
static void mir3da_do_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);
    int pre_enable = atomic_read(&mir3da->enable);

    mutex_lock(&mir3da->enable_mutex);

    MI_MSG("%s:enable =%d pre_enable=%d\n",__func__,enable,pre_enable);

    if (enable != pre_enable) {
    	mir3da_set_enable(mir3da->mir3da_client, enable);
        
    	if(enable){
        #if (MIR3DA_HRTIMER == 0)
            queue_delayed_work(mir3da->wq, &mir3da->work, msecs_to_jiffies(0));
        #else
            hrtimer_start(&mir3da->hr_timer, ktime_set(0, 0), HRTIMER_MODE_REL);
        #endif
    	}else{
    	#if MIR3DA_HRTIMER
    	    hrtimer_cancel(&mir3da->hr_timer);
        #endif
            cancel_delayed_work_sync(&mir3da->work);
    	}
        
        atomic_set(&mir3da->enable, enable);
    }
    mutex_unlock(&mir3da->enable_mutex);
}
/**************************************************************************/
static ssize_t mir3da_enable_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    unsigned long data=0;
    int error=0;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if ((data == 0) || (data == 1))
        mir3da_do_enable(dev, data);

    return count;
}
/**************************************************************************/
static ssize_t mir3da_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&mir3da->delay));
}
/**************************************************************************/
static ssize_t mir3da_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long data=0;
    int error=0;
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&mir3da->delay, (unsigned int) data);

    return count;
}
/**************************************************************************/
static ssize_t mir3da_axis_data_show(struct device *dev,
           struct device_attribute *attr, char *buf)
{
    int result;
    short x,y,z;
    int count = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);	
	
    result = mir3da_read_data(mir3da->mir3da_client, &x, &y, &z);
    if (result == 0)
        count += sprintf(buf+count, "x= %d;y=%d;z=%d\n", x,y,z);
    else
        count += sprintf(buf+count, "reading failed!");

    return count;
}
/**************************************************************************/
static ssize_t mir3da_reg_data_store(struct device *dev,
           struct device_attribute *attr, const char *buf, size_t count)
{
    int                 addr, data;
    int                 result;
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);	
	
    sscanf(buf, "0x%x, 0x%x\n", &addr, &data);
    
    result = mir3da_register_write(mir3da->mir3da_client, addr, data);
    
    MI_ASSERT(result==0);

    return count;
}
/**************************************************************************/
static ssize_t mir3da_reg_data_show(struct device *dev,
           struct device_attribute *attr, char *buf)
{
    MIR_HANDLE          handle = mir_handle;
        
    return mir3da_get_reg_data(handle, buf);
}
/**************************************************************************/
static ssize_t mir3da_offset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t count = 0;
    int rst = 0;
    u8 off[9] = {0};
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);	
	
    rst = mir3da_read_offset(mir3da->mir3da_client, off);
    if (!rst){
        count = sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n", off[0],off[1],off[2],off[3],off[4],off[5],off[6],off[7],off[8]);
    }
    return count;
}
/**************************************************************************/
static ssize_t mir3da_offset_store(struct device *dev, struct device_attribute *attr,
                    const char *buf, size_t count)
{
    int off[9] = {0};
    u8  offset[9] = {0};
    int rst = 0;
	
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);	
	
    sscanf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n", &off[0], &off[1], &off[2], &off[3], &off[4], &off[5],&off[6], &off[7], &off[8]);

    offset[0] = (u8)off[0];
    offset[1] = (u8)off[1];
    offset[2] = (u8)off[2];
    offset[3] = (u8)off[3];
    offset[4] = (u8)off[4];
    offset[5] = (u8)off[5];
    offset[6] = (u8)off[6];
    offset[7] = (u8)off[7];
    offset[8] = (u8)off[8];

    rst = mir3da_write_offset(mir3da->mir3da_client, offset);
    return count;
}
/**************************************************************************/
#if FILTER_AVERAGE_ENHANCE
static ssize_t mir3da_average_enhance_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
    int                             ret = 0;
    struct mir3da_filter_param_s    param = {0};

    ret = mir3da_get_filter_param(&param);
    ret |= sprintf(buf, "%d %d %d\n", param.filter_param_l, param.filter_param_h, param.filter_threhold);

    return ret;
}
/**************************************************************************/
static ssize_t mir3da_average_enhance_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{ 
    int                             ret = 0;
    struct mir3da_filter_param_s    param = {0};
    
    sscanf(buf, "%d %d %d\n", &param.filter_param_l, &param.filter_param_h, &param.filter_threhold);
    
    ret = mir3da_set_filter_param(&param);
    
    return count;
}
#endif 
/**************************************************************************/
#if MIR3DA_OFFSET_TEMP_SOLUTION
int bCaliResult = -1;
static ssize_t mir3da_calibrate_show(struct device *dev,struct device_attribute *attr,char *buf)
{
    int ret;       

    ret = sprintf(buf, "%d\n", bCaliResult);   
    return ret;
}
/**************************************************************************/
static ssize_t mir3da_calibrate_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    s8              z_dir = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);	
    
    z_dir = simple_strtol(buf, NULL, 10);
    bCaliResult = mir3da_calibrate(mir3da->mir3da_client, z_dir);
    
    return count;
}
#endif
/**************************************************************************/
static ssize_t mir3da_calibration_run_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);
    struct mir3da_acc acc;

    cycle_read_xyz(mir3da->mir3da_client,&(acc.x),&(acc.y),&(acc.z),20);	

    mir3da->offset.x = 0 - acc.x;
    mir3da->offset.y = 0 - acc.y;
    if (atomic_read(&mir3da->position) > 0) {
        mir3da->offset.z = LSG - acc.z;
    } else {
        mir3da->offset.z = (-LSG) - acc.z;
    }
	
    MI_MSG("fast calibration: %d %d %d\n", mir3da->offset.x,mir3da->offset.y, mir3da->offset.z);

    return count;
}
/**************************************************************************/
static ssize_t mir3da_calibration_reset_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);
    
    memset(&(mir3da->offset), 0, sizeof(struct mir3da_acc));

    MI_MSG( "reset fast calibration finished\n");
    return count;
}
/**************************************************************************/
static ssize_t mir3da_calibration_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);

    return sprintf(buf, "%d %d %d\n", mir3da->offset.x, mir3da->offset.y, mir3da->offset.z);
}
/**************************************************************************/
static ssize_t mir3da_calibration_value_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int data[3]={0};
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);

    sscanf(buf, "%d %d %d", &data[0], &data[1], &data[2]);
    mir3da->offset.x = (signed short) data[0];
    mir3da->offset.y = (signed short) data[1];
    mir3da->offset.z = (signed short) data[2];
    
    MI_MSG( "set fast calibration finished\n");
    return count;
}
/**************************************************************************/
static ssize_t mir3da_board_position_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);

    data = atomic_read(&(mir3da->position));

    return sprintf(buf, "%d\n", data);
}
/**************************************************************************/
static ssize_t mir3da_board_position_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data=0;
    int error=0;
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&(mir3da->position), (int) data);

    return count;
}
/**************************************************************************/
static ssize_t mir3da_log_level_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
    int ret;

    ret = sprintf(buf, "%d\n", Log_level);

    return ret;
}
/**************************************************************************/
static ssize_t mir3da_log_level_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    Log_level = simple_strtoul(buf, NULL, 10);    

    return count;
}
/**************************************************************************/
static ssize_t mir3da_primary_offset_show(struct device *dev,
                   struct device_attribute *attr, char *buf){    
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *mir3da = i2c_get_clientdata(client);  
    int x=0,y=0,z=0;
   
    mir3da_get_primary_offset(mir3da->mir3da_client,&x,&y,&z);

	return sprintf(buf, "x=%d ,y=%d ,z=%d\n",x,y,z);

}
/**************************************************************************/
static ssize_t mir3da_version_show(struct device *dev,
                   struct device_attribute *attr, char *buf){    

	return sprintf(buf, "%s_%s\n", DRI_VER, CORE_VER);
}
/**************************************************************************/
static ssize_t mir3da_vendor_show(struct device *dev,
                   struct device_attribute *attr, char *buf){
	return sprintf(buf, "%s\n", "MiraMEMS");
}
/**************************************************************************/
static DEVICE_ATTR(enable,          	S_IRUGO | S_IWUGO,  mir3da_enable_show,             mir3da_enable_store);
static DEVICE_ATTR(delay,      	        S_IRUGO | S_IWUGO,  mir3da_delay_show,              mir3da_delay_store);
static DEVICE_ATTR(axis_data,       	S_IRUGO|S_IWUSR|S_IWGRP,    mir3da_axis_data_show,          NULL);
static DEVICE_ATTR(reg_data,        	S_IRUGO|S_IWUSR|S_IWGRP,  mir3da_reg_data_show,           mir3da_reg_data_store);
static DEVICE_ATTR(log_level,       	S_IRUGO|S_IWUSR|S_IWGRP,  mir3da_log_level_show,          mir3da_log_level_store);
#if MIR3DA_OFFSET_TEMP_SOLUTION
static DEVICE_ATTR(offset,          	S_IRUGO|S_IWUSR|S_IWGRP,  mir3da_offset_show,             mir3da_offset_store);
static DEVICE_ATTR(calibrate_miraGSensor,       S_IRUGO|S_IWUSR|S_IWGRP,  mir3da_calibrate_show,          mir3da_calibrate_store);
#endif
#if FILTER_AVERAGE_ENHANCE
static DEVICE_ATTR(average_enhance,     S_IRUGO|S_IWUSR|S_IWGRP,  mir3da_average_enhance_show,    mir3da_average_enhance_store);
#endif
static DEVICE_ATTR(calibration_run, 	S_IWUGO | S_IRUGO,NULL, mir3da_calibration_run_store);
static DEVICE_ATTR(calibration_reset,	S_IWUGO | S_IRUGO, NULL, mir3da_calibration_reset_store);
static DEVICE_ATTR(calibration_value, 	S_IWUGO | S_IRUGO,mir3da_calibration_value_show,mir3da_calibration_value_store);
static DEVICE_ATTR(board_position, 	    S_IRUGO|S_IWUSR|S_IWGRP,mir3da_board_position_show, mir3da_board_position_store);
static DEVICE_ATTR(primary_offset,      S_IWUGO ,                     mir3da_primary_offset_show,     NULL);
static DEVICE_ATTR(version,          	S_IRUGO|S_IWUSR|S_IWGRP,            mir3da_version_show,            NULL);
static DEVICE_ATTR(vendor,           	S_IRUGO|S_IWUSR|S_IWGRP,            mir3da_vendor_show,             NULL); 
/**************************************************************************/
static struct attribute *mir3da_attributes[] = { 
    &dev_attr_enable.attr,
    &dev_attr_delay.attr,
    &dev_attr_axis_data.attr,
    &dev_attr_reg_data.attr,
    &dev_attr_log_level.attr,
#if MIR3DA_OFFSET_TEMP_SOLUTION
    &dev_attr_offset.attr,    
    &dev_attr_calibrate_miraGSensor.attr,
#endif
#if FILTER_AVERAGE_ENHANCE
    &dev_attr_average_enhance.attr,
#endif
    &dev_attr_calibration_run.attr,
    &dev_attr_calibration_reset.attr,
    &dev_attr_calibration_value.attr,
    &dev_attr_board_position.attr, 
    &dev_attr_primary_offset.attr,    
    &dev_attr_version.attr,
    &dev_attr_vendor.attr,
    NULL
};

static const struct attribute_group mir3da_attr_group = {
    .attrs  = mir3da_attributes,
};
/**************************************************************************/
int i2c_smbus_read(PLAT_HANDLE handle, u8 addr, u8 *data)
{
    int                 res = 0;
    struct i2c_client   *client = (struct i2c_client*)handle;
    
    *data = i2c_smbus_read_byte_data(client, addr);
    
    return res;
}
/**************************************************************************/
int i2c_smbus_read_block(PLAT_HANDLE handle, u8 addr, u8 count, u8 *data)
{
    int                 res = 0;
    struct i2c_client   *client = (struct i2c_client*)handle;
    
    res = i2c_smbus_read_i2c_block_data(client, addr, count, data);
    
    return res;
}
/**************************************************************************/
int i2c_smbus_write(PLAT_HANDLE handle, u8 addr, u8 data)
{
    int                 res = 0;
    struct i2c_client   *client = (struct i2c_client*)handle;
    
    res = i2c_smbus_write_byte_data(client, addr, data);
    
    return res;
}
/**************************************************************************/
void msdelay(int ms)
{
    mdelay(ms);
}
#if MIR3DA_OFFSET_TEMP_SOLUTION
MIR_GENERAL_OPS_DECLARE(ops_handle, i2c_smbus_read, i2c_smbus_read_block, i2c_smbus_write, sensor_sync_write, sensor_sync_read, msdelay, printk, sprintf);
#else
MIR_GENERAL_OPS_DECLARE(ops_handle, i2c_smbus_read, i2c_smbus_read_block, i2c_smbus_write, NULL, NULL, msdelay, printk, sprintf);
#endif
/**************************************************************************/
static void mir3da_axis_remap(struct i2c_client *client,short *x,short *y,short *z)
{
    short swap=0;
    struct mir3da_data *mir3da = i2c_get_clientdata(client);
    int position = atomic_read(&mir3da->position);

    switch (abs(position)) {
        case 1:
            break;
        case 2:
            swap = *x;
            *x = *y;
            *y = -swap; 
            break;
        case 3:
            *x = -(*x);
            *y = -(*y);
            break;
        case 4:
            swap = *x;
            *x = -(*y);
            *y = swap;
            break;
    }
    
    if (position < 0) {
        *z = -(*z);
        *x = -(*x);
    }
    
}
/**************************************************************************/
static int mir3da_read_file(char *path, char *buf, int size)
{
    struct file *filp;
    loff_t len, offset;
    int ret=0;
    mm_segment_t fs;

    filp = filp_open(path, O_RDWR, 0777);
    if (IS_ERR(filp)) {
        ret = PTR_ERR(filp);
        goto out;
    }

    len = vfs_llseek(filp, 0, SEEK_END);
    if (len > size) {
        len = size;
    }
    
    offset = vfs_llseek(filp, 0, SEEK_SET);

    fs=get_fs();
    set_fs(KERNEL_DS);

    ret = vfs_read(filp, (char __user *)buf, (size_t)len, &(filp->f_pos));

    set_fs(fs);

    filp_close(filp, NULL);    
out:
    return ret;
}
/**************************************************************************/
static int mir3da_load_user_calibration(struct i2c_client *client)
{
    char buffer[16]={0};
    int data[3]={0};
    int ret = 0;
	
    struct mir3da_data *mir3da = i2c_get_clientdata(client);    
    int calibrated = atomic_read(&mir3da->calibrated);
    
    if (calibrated) {
        goto usr_calib_end;
    } else {
        atomic_set(&mir3da->calibrated, 1);
    }

    ret = mir3da_read_file(CFG_GSENSOR_CALIBFILE, buffer, sizeof(buffer));
    if (ret <= 0) {
        MI_ERR("gsensor calibration file not exist!\n");
        goto usr_calib_end;
    }
    
    sscanf(buffer, "%d %d %d", &data[0], &data[1], &data[2]);
    mir3da->offset.x = (signed short) data[0];
    mir3da->offset.y = (signed short) data[1];
    mir3da->offset.z = (signed short) data[2];
    
    MI_MSG( "load cfg_calibration: %d %d %d\n", data[0], data[1], data[2]);
    
usr_calib_end:
    return ret;
}
/**************************************************************************/
#if MIR3DA_HRTIMER
enum hrtimer_restart mir3da_hrtimer_callback( struct hrtimer *timer )  
{  
    struct mir3da_data *mir3da = container_of((struct hrtimer *)timer, struct mir3da_data, hr_timer);
    int wq_delay = atomic_read(&mir3da->delay) - 3;

    queue_delayed_work(mir3da->wq, &mir3da->work, msecs_to_jiffies(0));
    hrtimer_forward_now(&mir3da->hr_timer, ktime_set(0, wq_delay * 1000000));
    
    return HRTIMER_RESTART;
}
#endif

/**************************************************************************/
static void mir3da_work_func(struct work_struct *work)
{
    int result=0;
    short x=0,y=0,z=0;
    struct mir3da_data *mir3da = container_of((struct delayed_work *)work, struct mir3da_data, work);
    int wq_delay = atomic_read(&mir3da->delay);


#if (MIR3DA_HRTIMER == 0)
    queue_delayed_work(mir3da->wq, &mir3da->work, msecs_to_jiffies(wq_delay - 3));
#endif
    
    mir3da_load_user_calibration(mir3da->mir3da_client);
	
    result = mir3da_read_data(mir3da->mir3da_client, &x,&y,&z);

    x += mir3da->offset.x;
    y += mir3da->offset.y;
    z += mir3da->offset.z;
	
    if (result == 0) {
	    mir3da_axis_remap(mir3da->mir3da_client,&x,&y,&z);
        
	    input_report_abs(mir3da->input, ABS_X, x);
	    input_report_abs(mir3da->input, ABS_Y, y);
	    input_report_abs(mir3da->input, ABS_Z, z);
	    input_sync(mir3da->input);
        
    } else {
        MI_ERR("mir3da_read_data failed!\n");
    }
}
/**************************************************************************/
static int  mir3da_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int result;
    int err = 0;
    const char * buf;
		long int temp;
    struct mir3da_data *data;
    struct input_dev *dev;
    int cfg_position =0;
    int cfg_calibration[3]={0};	
	
    MI_FUN;
    
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        MI_ERR("i2c_check_functionality error\n");
        goto exit;
    }

    data = kzalloc(sizeof(struct mir3da_data), GFP_KERNEL);
    if (!data) {
        result = -ENOMEM;
        goto exit;
    }

    if(mir3da_install_general_ops(&ops_handle)){
        MI_ERR("Install ops failed !\n");
        goto kfree_exit;
    }

    i2c_set_clientdata(client, data);
    data->mir3da_client = client;
    mutex_init(&data->enable_mutex);

#if CFG_GSENSOR_USE_CONFIG > 0
	err = of_property_read_string(client->dev.of_node, "position", &buf);
	if (err != 0) {
	  	printk(KERN_ERR"get position fail\n");
		cfg_position = -3;
	}
	else{
		err = strict_strtol(buf, 10, &temp);  
		cfg_position = (int)temp;
	}	
#else
	cfg_position = -3;
#endif

    atomic_set(&data->position, cfg_position);
    atomic_set(&data->calibrated, 0);

#if CFG_GSENSOR_USE_CONFIG > 0
    //get dts configures
    err = of_property_read_u32_array(client->dev.of_node, "calibration_table", cfg_calibration, 3);
    if (err != 0) {
        printk(KERN_ERR"get calibration fail\n");
        memset(cfg_calibration, 0, sizeof(cfg_calibration));
    }
#else
    memset(cfg_calibration, 0, sizeof(cfg_calibration));
#endif   

    data->offset.x = (signed short) cfg_calibration[0];
    data->offset.y = (signed short) cfg_calibration[1];
    data->offset.z = (signed short) cfg_calibration[2];

#if MIR3DA_OFFSET_TEMP_SOLUTION
    m_work_info.wq = create_singlethread_workqueue( "oo" );
    if(NULL==m_work_info.wq) {
        MI_ERR("Failed to create workqueue !");
        goto kfree_exit;
    }
    
    INIT_DELAYED_WORK( &m_work_info.read_work, sensor_read_work );
    INIT_DELAYED_WORK( &m_work_info.write_work, sensor_write_work );
#endif

    mir_handle = mir3da_core_init((PLAT_HANDLE)client);
    if(NULL == mir_handle){
        MI_ERR("chip init failed !\n");
        goto kfree_exit;
    }

#if MIR3DA_HRTIMER
    hrtimer_init( &data->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
    data->hr_timer.function = mir3da_hrtimer_callback; 
#endif

    data->wq = create_singlethread_workqueue( "mir3da_wq" );
    INIT_DELAYED_WORK(&data->work, mir3da_work_func);
    
    atomic_set(&data->delay, DELAY_INTERVAL_MAX);
    atomic_set(&data->enable, 0);

    dev = input_allocate_device();
    if (!dev){
	   MI_ERR("input allocate error!");
          return -ENOMEM;
     }		
    dev->name = MIR3DA_DRV_NAME;
    dev->id.bustype = BUS_I2C;

    input_set_capability(dev, EV_ABS, ABS_MISC);
    input_set_abs_params(dev, ABS_X, -16384, 16383, INPUT_FUZZ, INPUT_FLAT);
    input_set_abs_params(dev, ABS_Y, -16384, 16383, INPUT_FUZZ, INPUT_FLAT);
    input_set_abs_params(dev, ABS_Z, -16384, 16383, INPUT_FUZZ, INPUT_FLAT); 
    input_set_drvdata(dev, data);

    result = input_register_device(dev);
    if (result < 0) {
	 MI_ERR("input register error!");	
        input_free_device(dev);
        goto kfree_exit;
    }

    data->input = dev;
  
    result = sysfs_create_group(&data->input->dev.kobj, &mir3da_attr_group);
    if (result < 0){
	MI_ERR("create sysfs group error!");		
        goto error_sysfs;
    }
    
    return result;
error_sysfs:
    input_unregister_device(data->input);

kfree_exit:
    kfree(data);
exit:
    return -1;	
}
/**************************************************************************/
static int  mir3da_remove(struct i2c_client *client)
{
    struct mir3da_data *data = i2c_get_clientdata(client);

    MI_FUN; 	

    mir3da_set_enable(data->mir3da_client, false);
#if MIR3DA_HRTIMER    
    hrtimer_cancel(&data->hr_timer);
#endif
    cancel_delayed_work_sync(&data->work);    
    flush_workqueue(data->wq);
    destroy_workqueue(data->wq);
    sysfs_remove_group(&data->input->dev.kobj, &mir3da_attr_group);
    input_unregister_device(data->input);
#if MIR3DA_OFFSET_TEMP_SOLUTION
    flush_workqueue(m_work_info.wq);
    destroy_workqueue(m_work_info.wq);
#endif
    kfree(data);

    return 0;
}
/**************************************************************************/
static int mir3da_suspend(struct device *dev)
{
    int result = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *data = i2c_get_clientdata(client);

    MI_FUN;	

#if MIR3DA_HRTIMER    
    hrtimer_cancel(&data->hr_timer);
#endif
    cancel_delayed_work_sync(&data->work);

    result = mir3da_set_enable(data->mir3da_client, false);
    if(result) {
	     MI_ERR("%s: disable fail!!\n",__func__);	
            return result;
     }

//     cancel_delayed_work_sync(&data->work);
	
      return result;
}
/**************************************************************************/
static int mir3da_resume(struct device *dev)
{
    int result = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct mir3da_data *data = i2c_get_clientdata(client);
    int pre_enable = atomic_read(&data->enable);

    MI_FUN;	
	
    result = mir3da_chip_resume(data->mir3da_client);
    if(result) {
		MI_ERR("chip resume fail!!\n");
		return result;
    }

    if (pre_enable) {
        result = mir3da_set_enable(data->mir3da_client, true);
        if(result) {
    	     MI_ERR("%s: enable fail!!\n",__func__);		
                return result;
        }
    #if (MIR3DA_HRTIMER == 0)
        queue_delayed_work(data->wq, &data->work, msecs_to_jiffies(0));
    #else
        hrtimer_start(&data->hr_timer, ktime_set(0, 0), HRTIMER_MODE_REL);
    #endif        
    }
    
    return result;
}

static SIMPLE_DEV_PM_OPS(mir3da_pm_ops, mir3da_suspend, mir3da_resume);

/**************************************************************************/
static const struct i2c_device_id mir3da_id[] = {
    { MIR3DA_DRV_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, mir3da_id);

static struct of_device_id mir3da_of_match[] = {
	{ .compatible = "mir3da" },
	{ }
};

static struct i2c_driver mir3da_driver = {
    .driver = {
        .name    = MIR3DA_DRV_NAME,
        .owner    = THIS_MODULE,
        .pm    = &mir3da_pm_ops,
        .of_match_table	= of_match_ptr(mir3da_of_match),
    },
    .class        = I2C_CLASS_HWMON,
    .id_table = mir3da_id,    
    .probe    = mir3da_probe,
    .remove    = mir3da_remove,
};
/**************************************************************************/
static int __init mir3da_init(void)
{   
    int res;
    MI_MSG("mir3da init !\n");
    res = i2c_add_driver(&mir3da_driver);
    if (res < 0){
        MI_ERR("add mir3da i2c driver failed\n");
        return -ENODEV;
    }
	
    MI_MSG("add mir3da i2c driver ok !\n");
	
    return (res);
}
/**************************************************************************/
static void __exit mir3da_exit(void)
{
    MI_FUN;
	
    i2c_unregister_device(mir_handle);
    i2c_del_driver(&mir3da_driver);
}
/**************************************************************************/
MODULE_AUTHOR("MiraMEMS <lschen@miramems.com>");
MODULE_DESCRIPTION("MIR3DA 3-Axis Accelerometer driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

module_init(mir3da_init);
module_exit(mir3da_exit);
