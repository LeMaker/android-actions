/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __ANDROID_CVBS_INTERFACE_H_
#define __ANDROID_CVBS_INTERFACE_H_

#include <hardware/hardware.h>

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>



#define CVBS_HARDWARE_DEVICE   "/dev/cvbs"


#define CVBS_HARDWARE_MODULE_ID "cvbs"
#define CVBS_HARDWARE_TVOUT0	"cvbs0"
#define HAL_TVOUT_PAL    0x0
#define HAL_TVOUT_NTSC  0x1


/**
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
struct cvbs_module_t {
    struct hw_module_t common;
};

/**
 * Every device data structure must begin with hw_device_t
 * followed by module specific public methods and attributes.
 */
struct cvbs_device_t {
    struct hw_device_t common;
    int (*enable)(struct cvbs_device_t *dev);	
    int (*disable)(struct cvbs_device_t *dev);
    int (*is_connected)(struct cvbs_device_t *dev);
    int (*set_mode)(struct cvbs_device_t *dev, int mode);
};


/** convenience API for opening and closing a device */

static inline int cvbs_dev_open(const struct hw_module_t* module, 
        struct cvbs_device_t** device) {
    return module->methods->open(module, 
            CVBS_HARDWARE_TVOUT0, (struct hw_device_t**)device);
}

static inline int ypbpr_dev_close(struct cvbs_device_t* device) {
    return device->common.close(&device->common);
}


//__END_DECLS

#endif  // ANDROID_HDMI_INTERFACE_H

