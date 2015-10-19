#include "isp_imx0219_imx.h"
#include "isp_gc2755_imx.h"

#ifdef ANDROID
#include <utils/Log.h>
#define printf ALOGD
#define printf_err ALOGE
#define LOG_TAG "II_PARA"
#else
#include <stdio.h>
#define printf_err printf
#endif

enum
{
    IMX0219 = 0x0219,
    GC2755 = 0x2655,
};

int get_sensor_imx_param(int sensor_id, act_isp_imx_param_t *param)
{
    int ret = 0;
    
    switch(sensor_id)
    {
        case IMX0219:
            printf("init imx param for IMX0219\n");
            memcpy(param, isp_imx_param_imx0219, RES_NUM_IMX0219*sizeof(act_isp_imx_param_t));
            ret = RES_NUM_IMX0219;
            break;
            
        case GC2755:
            printf("init imx param for GC2755\n");
            memcpy(param, isp_imx_param_gc2755, RES_NUM_GC2755*sizeof(act_isp_imx_param_t));
            ret = RES_NUM_GC2755;
            break;
            
        default:
            printf_err("not support this sensor id 0x%04x", sensor_id);
            break;
    }

    return ret;
}
