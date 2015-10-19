#ifndef __MODULE_DETECT_H__
#define __MODULE_DETECT_H__

#include <linux/i2c.h>

#define MAX_CAMERA_MODULE_NUM     32

#define CAMERA_ITEM_INIT(module_name, i2c_address, i2c_reg_width, \
        i2c_data_width, module_detect) \
{\
    .name         = #module_name,\
    .i2c_addr     = i2c_address,\
    .reg_width    = i2c_reg_width,\
    .data_width   = i2c_data_width,\
    .detect       = module_detect,\
    .need_detect  = false,\
}\

struct module_item_t {
    unsigned char *name;
    unsigned int  i2c_addr;
    unsigned int  reg_width;
    unsigned int  data_width;

    int           (*detect)(struct module_item_t *, struct i2c_adapter *i2c_adap);
    bool          need_detect;
};

extern struct module_item_t g_module_list[MAX_CAMERA_MODULE_NUM];
#endif
