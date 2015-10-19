#ifndef IMXCTL_ADAPTER_H
#define IMXCTL_ADAPTER_H


#define IMX_MAX_RES_NUM 3

#define IMX_ISO_BASE 64
#define IMX_MAX_ISOX 32
#define IMX_MAX_LEV_NUM 6

enum{
    IMX_YUV420SP,
    IMX_YUV420P
};

typedef struct
{
    int min_iso;
    int max_iso;
    
    int nr_enable;
    int nr_level_y;
    int nr_bl_size_y;
    int nr_level_uv;
    int nr_bl_size_uv;

    int ee_enable;
    /*
    0:hw(capture&preview use hw_ee) 
    1:sw(capture use sw_ee & preview use no_ee ) 
    2:hw+sw(capture use sw_ee & preview use hw_ee) 
    */
    int ee_algs; 
    float ee_k1_hw;
    float ee_k2_hw;
    float ee_k3_hw;
    int ee_t1_hw;
    int ee_t2_hw;
    float ee_k1_sw;
    float ee_k2_sw;
    float ee_k3_sw;
    int ee_t1_sw;
    int ee_t2_sw;
} act_isp_imx_iso_param_t;

typedef struct
{
    int width;
    int height;
    int nr_enable;
    int ee_enable;
    int min_shutter;
    int max_shutter;
    int min_gain;
    int max_gain;
    int iso_num;
    act_isp_imx_iso_param_t iso_param[IMX_MAX_LEV_NUM];
} act_isp_imx_param_t;

void *imxctl_open(void);
int imxctl_init(void *handle, int sensor_id);
int imxctl_set_param(void *handle, act_isp_imx_param_t *param);
int imxctl_get_param(void *handle, act_isp_imx_param_t *param);
int imxctl_process(void *handle, unsigned char *img_phy, unsigned char *img_vir, int width, int height, 
                int xoffset, int yoffset, int format, int shutter, int gain, int mode);
int imxctl_close(void *handle);

#endif
