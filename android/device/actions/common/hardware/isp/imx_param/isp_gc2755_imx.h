#ifndef ISP_GC2755_IMX_H
#define ISP_GC2755_IMX_H

#include "imxapi_adapter.h"

static act_isp_imx_param_t isp_imx_param_gc2755[] = 
{
    {
        1920,1080,1,1,
        1,0x07ff,64,64*32,
        3,
        1,4,
        1,1,1,4,4,
        /*enable,algs, k1_hw,k2_hw,k3_hw,t1_hw,t2_hw, k1_sw,k2_sw,k3_sw,t1_sw,t2_sw*/
        0,0, 0.0,0.0,0.0,7,15, 0.0,0.0,0.0,7,15,
        4,8,
        1,2,2,4,4,
        0,0, 0.0,0.0,0.0,7,15, 0.0,0.0,0.0,7,15,
        8,32,
        1,4,4,4,4,
        0,0, 0.0,0.0,0.0,7,15, 0.0,0.0,0.0,7,15,
    },
};

#define RES_NUM_GC2755 1

#endif