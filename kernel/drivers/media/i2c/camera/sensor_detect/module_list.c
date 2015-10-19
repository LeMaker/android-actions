
#include "module_detect.h"

#define DBG_SEN_INFO(fmt, args...)  printk(KERN_INFO"[sensor_list] line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)
#define DBG_SEN_ERR(fmt, args...)   printk(KERN_ERR"[sensor_list] line:%d--%s() "fmt"\n", __LINE__, __FUNCTION__, ##args)

struct regval_t {
    unsigned char reg_array[2];
    unsigned char data_array[2];
};

static int camera_i2c_read(struct i2c_adapter *i2c_adap, unsigned int i2c_addr, unsigned char reg_width, unsigned char data_width,
					unsigned char *reg_array, unsigned char *data_array)
{
	struct i2c_msg msg;
	int ret = 0;
	
	msg.addr = i2c_addr >> 1;
	msg.flags = 0;
	msg.len   = reg_width;
	msg.buf   = reg_array;
	ret = i2c_transfer(i2c_adap, &msg, 1);
	if (ret < 0) {
		DBG_SEN_ERR("write register error %d", ret);
		return ret;
	}
	
	msg.flags = I2C_M_RD;
	msg.len   = data_width;
	msg.buf   = data_array;	
	ret = i2c_transfer(i2c_adap, &msg, 1);
	if (ret >= 0) {
        ret = 0;
	}
	else {
	    DBG_SEN_ERR("read register error %d", ret);
	}
	
	return ret;
}

static int camera_i2c_write(struct i2c_adapter *i2c_adap, unsigned int i2c_addr, unsigned char reg_width, unsigned char data_width,
					unsigned char *reg_array, unsigned char *data_array)
{
	struct i2c_msg msg;
	unsigned char data[4];
	int ret,i;
	
	for (i = 0; i < reg_width; i++) {
	    data[i] = reg_array[i];
	}
	
	for(i = reg_width; i < (reg_width + data_width); i++) {
		data[i] = data_array[i - reg_width];
	}
	
	msg.addr = i2c_addr >> 1;
	msg.flags = 0;
	msg.len   = reg_width + data_width;
	msg.buf   = data;    
	ret = i2c_transfer(i2c_adap, &msg, 1);
	if (ret > 0) {
		ret = 0;
	}
	else if (ret < 0) {
	    DBG_SEN_ERR("write register error %d", ret);
	}
	
	return ret;
}

static int module_detect_gc0308(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
    regval.reg_array[0]  = 0xfe;
	regval.data_array[0] = 0x00; 
	ret = camera_i2c_write(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    	
	regval.reg_array[0] = 0x00;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0x9b) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_gc0312(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
	unsigned int module_pid;
    int ret = 0;
    
    regval.reg_array[0]  = 0xfe;
	regval.data_array[0] = 0x00; 
	ret = camera_i2c_write(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    	
	
	regval.reg_array[0] = 0xf0;
		ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
		if (ret < 0) {
			return ret;
		}
		
		module_pid = regval.data_array[0] << 8;
		
		regval.reg_array[0] = 0xf1;
		ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
		if (ret < 0) {
			return ret;
		}
		
		 module_pid |= regval.data_array[0];
		
		if (module_pid == 0xb310) {
	
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_gc0328(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
    regval.reg_array[0]  = 0xfe;
	regval.data_array[0] = 0x00; 
	ret = camera_i2c_write(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    	
	regval.reg_array[0] = 0xf0;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0x9d) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_gc0329(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
    regval.reg_array[0]  = 0xfc;
	regval.data_array[0] = 0x16; 
	ret = camera_i2c_write(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    	
	regval.reg_array[0] = 0x00;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0xc0) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_gc2035(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0xf0;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid = regval.data_array[0] << 8;
    
    regval.reg_array[0] = 0xf1;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid |= regval.data_array[0];
    
	if (module_pid == 0x2035) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_gt2005(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
    regval.reg_array[0] = 0x00;
	regval.reg_array[1] = 0x00;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0x51) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_hi253(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
    regval.reg_array[0]  = 0x03;
	regval.data_array[0] = 0x00; 
	ret = camera_i2c_write(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    	
	regval.reg_array[0] = 0x04;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0x92) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}


static int module_detect_hi708(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;	
	regval.reg_array[0] = 0x04;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0x96) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_hi257(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
	regval.reg_array[0] = 0x04;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0xc4) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_hm2057(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0x00;
	regval.reg_array[1] = 0x01;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid = regval.data_array[0] << 8;

	regval.reg_array[0] = 0x00;
	regval.reg_array[1] = 0x02;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid |= regval.data_array[0];

	if (module_pid == 0x2056) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_ov2643(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0x0a;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid = regval.data_array[0] << 8;

	regval.reg_array[0] = 0x0b;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid |= regval.data_array[0];

	if (module_pid == 0x2643) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}
static int module_detect_ov2686(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0x30;
	regval.reg_array[1] = 0x0a;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid = regval.data_array[0] << 8;

	regval.reg_array[0] = 0x30;
	regval.reg_array[1] = 0x0b;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid |= regval.data_array[0];

	if (module_pid == 0x2685) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s,the module_pid is 0x%x", item->name,module_pid);
		ret = -ENODEV;
	}	
    
    return ret;
}


static int module_detect_ov5640(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0x30;
	regval.reg_array[1] = 0x0a;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid = regval.data_array[0] << 8;

	regval.reg_array[0] = 0x30;
	regval.reg_array[1] = 0x0b;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid |= regval.data_array[0];

	if (module_pid == 0x5640) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_sp0838(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
    regval.reg_array[0]  = 0xfd;
	regval.data_array[0] = 0x00; 
	ret = camera_i2c_write(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    	
	regval.reg_array[0] = 0x02;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0x27) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_sp2518(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
	regval.reg_array[0] = 0x02;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0x53) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_sp2519(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
	unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0x02;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid = regval.data_array[0] << 8;
    
    regval.reg_array[0] = 0xa0;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid |= regval.data_array[0];
    
	if (module_pid == 0x2519) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_sp0718(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
	regval.reg_array[0] = 0x02;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0x71) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}


static int module_detect_gc0311(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
	regval.reg_array[0] = 0xf0;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0xBB) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_gc2145(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0xf0;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid = regval.data_array[0] << 8;
    
    regval.reg_array[0] = 0xf1;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid |= regval.data_array[0];
    
	if (module_pid == 0x2145) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s, pid=0x%x", item->name, module_pid);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_gc2155(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;

	 regval.reg_array[0]  = 0xfe;
	regval.data_array[0] = 0x00; 
	ret = camera_i2c_write(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	regval.reg_array[0] = 0xf0;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid = regval.data_array[0] << 8;
    
    regval.reg_array[0] = 0xf1;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid |= regval.data_array[0];
    
	if (module_pid == 0x2155) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s, pid=0x%x", item->name, module_pid);
		ret = -ENODEV;
	}	
    
    return ret;
}


static int module_detect_siv120d(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
	regval.reg_array[0] = 0x01;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0x12) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_siv121d(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
	regval.reg_array[0] = 0x01;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0xDE) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_sp0a19(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
	regval.reg_array[0] = 0x02;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0xa6) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_siv121du(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;
    
	regval.reg_array[0] = 0x01;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
	if (regval.data_array[0] == 0xde) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_siv130b(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    int ret = 0;

	regval.reg_array[0] = 0x01;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	
	if (ret < 0) {
		return ret;
	}
	if (regval.data_array[0] == 0x1B) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s", item->name);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_gc2015(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
    unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;

    DBG_SEN_INFO("start detect %s, i2c addr:0x%02x",
            item->name, i2c_addr);

    regval.reg_array[0] = 0x00;
    ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
    if (ret < 0) {
        DBG_SEN_ERR("i2c read reg 0 error");
        return ret;
    }

    module_pid = regval.data_array[0] << 8;

    regval.reg_array[0] = 0x01;
    ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
    if (ret < 0) {
        DBG_SEN_ERR("i2c read reg 1 error");
        return ret;
    }

    module_pid |= (regval.data_array[0] & 0x1f);

    if (module_pid == 0x2005) {
        DBG_SEN_INFO("detect %s", item->name);
    } else {
        DBG_SEN_INFO("not detect %s, excepted values in reg[0x00, 0x01] is [0x20 0x05], actual:[0x%02x, 0x%02x]",
                item->name, regval.data_array[0], regval.data_array[1]);
        ret = -ENODEV;
    }

    return ret;
}

static int module_detect_gc0310(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
    unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;

    DBG_SEN_INFO("start detect %s, i2c addr:0x%02x\n",
            item->name, i2c_addr);

    regval.reg_array[0] = 0xf0;
    ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
    if (ret < 0) {
        DBG_SEN_ERR("i2c read reg 0 error\n");
        return ret;
    }

    module_pid = regval.data_array[0] << 8;

    regval.reg_array[0] = 0xf1;
    ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
    if (ret < 0) {
        DBG_SEN_ERR("i2c read reg 1 error\n");
        return ret;
    }

    module_pid |= regval.data_array[0];

    if (module_pid == 0xa310) {
        DBG_SEN_INFO("detect %s", item->name);
    } else {
        DBG_SEN_INFO("not detect %s, excepted values in reg[0xf0, 0xf1] is [0x3a 0x10], actual:[0x%02x, 0x%02x]",
                item->name, regval.data_array[0], regval.data_array[1]);
        ret = -ENODEV;
    }

    return ret;
}

static int module_detect_ov5642(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
    unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;

    regval.reg_array[0] = 0x30;
    regval.reg_array[1] = 0x0a;
    ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
    if (ret < 0) {
        return ret;
    }

    module_pid = regval.data_array[0] << 8;

    regval.reg_array[0] = 0x30;
    regval.reg_array[1] = 0x0b;
    ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
    if (ret < 0) {
        return ret;
    }

    module_pid |= regval.data_array[0];

    if (module_pid == 0x5642) {
        DBG_SEN_INFO("detect %s", item->name);
    } else {
        DBG_SEN_INFO("not detect %s", item->name);
        ret = -ENODEV;
    }

    return ret;
}

static int module_detect_bf3a03(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
	unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0xfc;//PIDH 
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    module_pid = regval.data_array[0] << 8;

	regval.reg_array[0] = 0xfd;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}

	module_pid |= regval.data_array[0];
	
	if (module_pid == 0x3a03) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s,pid=%x", item->name,module_pid);
		ret = -ENODEV;
	}	
    
    return ret;
}

static int module_detect_bf3920(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0xfc;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
	
    module_pid = regval.data_array[0] << 8;
    
    regval.reg_array[0] = 0xfd;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid |= regval.data_array[0];
    
	if (module_pid == 0x3920) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s, pid=0x%x", item->name, module_pid);
		ret = -ENODEV;
	}	
    
    return ret;

}

static int module_detect_bf3703(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0xfc;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}

    module_pid = regval.data_array[0] << 8;
    
    regval.reg_array[0] = 0xfd;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}
    
    module_pid |= regval.data_array[0];
    
	if (module_pid == 0x3703) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s, pid=0x%x", item->name, module_pid);
		ret = -ENODEV;
	}	
    
    return ret;

}

static int module_detect_soc5140(struct module_item_t *item, struct i2c_adapter *i2c_adap)
{
	unsigned int i2c_addr   = item->i2c_addr;
    unsigned int reg_width  = item->reg_width;
    unsigned int data_width = item->data_width;
    struct regval_t regval;
    unsigned int module_pid;
    int ret = 0;
    
	regval.reg_array[0] = 0x00;
	regval.reg_array[1] = 0x00;
	ret = camera_i2c_read(i2c_adap, i2c_addr, reg_width, data_width, regval.reg_array, regval.data_array);
	if (ret < 0) {
		return ret;
	}

    module_pid = regval.data_array[1] | regval.data_array[0] << 8;
    
	if (module_pid == 0x2880) {
	    DBG_SEN_INFO("detect %s", item->name);
	} else {
	    DBG_SEN_INFO("not detect %s, pid=0x%x", item->name, module_pid);
		ret = -ENODEV;
	}	
    
    return ret;

}


struct module_item_t g_module_list[MAX_CAMERA_MODULE_NUM] = {
    CAMERA_ITEM_INIT(gc0308, 0x42, 1, 1, module_detect_gc0308),
	CAMERA_ITEM_INIT(gc0312, 0x42, 1, 1, module_detect_gc0312),
    CAMERA_ITEM_INIT(gc0328, 0x42, 1, 1, module_detect_gc0328),
    CAMERA_ITEM_INIT(gc0329, 0x62, 1, 1, module_detect_gc0329),
    CAMERA_ITEM_INIT(gc2035, 0x78, 1, 1, module_detect_gc2035),
    CAMERA_ITEM_INIT(gt2005, 0x78, 2, 1, module_detect_gt2005),
    CAMERA_ITEM_INIT(hi253,  0x40, 1, 1, module_detect_hi253),
    CAMERA_ITEM_INIT(hi708,  0x60, 1, 1, module_detect_hi708),
    CAMERA_ITEM_INIT(hi257,  0x40, 1, 1, module_detect_hi257),
    CAMERA_ITEM_INIT(hm2057, 0x48, 2, 1, module_detect_hm2057),
    CAMERA_ITEM_INIT(ov2643, 0x60, 1, 1, module_detect_ov2643),
    CAMERA_ITEM_INIT(ov2686, 0x78, 2, 1, module_detect_ov2686),
    CAMERA_ITEM_INIT(ov5640, 0x78, 2, 1, module_detect_ov5640),
    CAMERA_ITEM_INIT(sp0838, 0x30, 1, 1, module_detect_sp0838),
    CAMERA_ITEM_INIT(sp0718, 0x42, 1, 1, module_detect_sp0718),
    CAMERA_ITEM_INIT(sp2518, 0x60, 1, 1, module_detect_sp2518),
    CAMERA_ITEM_INIT(sp2519, 0x60, 1, 1, module_detect_sp2519),
    CAMERA_ITEM_INIT(gc0311, 0x66, 1, 1, module_detect_gc0311),
    CAMERA_ITEM_INIT(gc2145, 0x78, 1, 1, module_detect_gc2145),
    CAMERA_ITEM_INIT(gc2155, 0x78, 1, 1, module_detect_gc2155),
    CAMERA_ITEM_INIT(siv120d, 0x66, 1, 1, module_detect_siv120d),
    CAMERA_ITEM_INIT(siv121d, 0x66, 1, 1, module_detect_siv121d),
    CAMERA_ITEM_INIT(sp0a19,  0x42, 1, 1, module_detect_sp0a19),
    CAMERA_ITEM_INIT(siv121du,0x66, 1, 1, module_detect_siv121du),
    CAMERA_ITEM_INIT(siv130b, 0x6F, 1, 1, module_detect_siv130b),
    CAMERA_ITEM_INIT(gc2015,  0x60, 1, 1, module_detect_gc2015),
    CAMERA_ITEM_INIT(gc0310,  0x42, 1, 1, module_detect_gc0310),
    CAMERA_ITEM_INIT(ov5642,  0x78, 2, 1, module_detect_ov5642),
    CAMERA_ITEM_INIT(bf3a03,  0xdc, 1, 1, module_detect_bf3a03),
    CAMERA_ITEM_INIT(bf3920,  0xdc, 1, 1, module_detect_bf3920),
    CAMERA_ITEM_INIT(bf3703,  0xdc, 1, 1, module_detect_bf3703),
    CAMERA_ITEM_INIT(soc5140,  0x78, 2, 2, module_detect_soc5140),

};
