#ifndef __ISP_CTL_H__
#define __ISP_CTL_H__

#ifndef phys_addr_t
#define phys_addr_t unsigned int
#endif
#define MAX_TEMP_NUM  0x6
#define MAX_RES_NUM 0x8
#define _SUPPORT_FOCUS_NUM_ 0x9

typedef enum
{
    ACT_NONE = -0x1,
    ACT_D65 = 0x0,
    ACT_D50 = 0x1,
    ACT_TL84 = 0x2,
    ACT_CWF = 0x3,
    ACT_A = 0x4,
    ACT_H = 0x5,
    ACT_ALL = 0x6,
} act_isp_tempture_t;

typedef enum
{
    ACT_PREVIEW_MODE = 0x0,
    ACT_CAPTURE_MODE,
} act_isp_work_mode_t;

typedef enum
{
    ACT_AE_OFF = 0x0,
    ACT_AE_AUTO,
    ACT_AE_MANUAL,
} act_ae_algs_t;

typedef enum
{
    ACT_AE_METER_MEAN = 0x0,
    ACT_AE_METER_WT_CENTER,
    ACT_AE_METER_WT_SPOT,
} act_ae_meter_t;

typedef enum
{
    ACT_AE_TARGET_HIST = 0x0,
    ACT_AE_TARGET_MANUAL,
} act_ae_mean_t;

typedef enum
{
    ACT_SCENE_NONE = 0x0,
    ACT_SCENE_NIGHT,
    ACT_SCENE_SPORT,
    ACT_SCENE_CANDLE,
} act_scene_t;

typedef enum
{
    ACT_BANDING_NONE = 0x0,
    ACT_BANDING_AUTO,
    ACT_BANDING_50HZ,
    ACT_BANDING_60HZ,
} act_banding_t;

typedef enum
{
    ACT_WB_OFF = 0x0,
    ACT_WB_AUTO,
    ACT_WB_MANUAL,
} act_wb_algs_t;

typedef enum
{
    ACT_SOURCE_NONE = 0x0,
    ACT_SOURCE_SUNLIGHT,
    ACT_SOURCE_CLOUDY,
    ACT_SOURCE_SHADE,
    ACT_SOURCE_TUNGSTEN,
    ACT_SOURCE_FLUORESCENT,
    ACT_SOURCE_INCANDESCENT,
} act_light_source_t;

typedef enum
{
    ACT_BC_OFF = 0x0,
    ACT_BC_AUTO,
    ACT_BC_MANUAL,
} act_bc_algs_t;

typedef enum
{
    ACT_AF_OFF = 0x0,
    ACT_AF_AUTO,
    ACT_AF_MANUAL,
} act_af_algs_t;

typedef enum
{
    ACT_AF_CONTINUOUS = 0x0,
    ACT_AF_SINGLE,
} act_af_mode_t;

typedef enum
{
    ACT_AF_STATUS_OK = 0x0,
    ACT_AF_STATUS_UNFINISH,
    ACT_AF_STATUS_FAIL,
} act_af_status_t;

typedef enum
{
    ACT_S_EXP_GAIN = 0x0,
    ACT_G_EXP_GAIN,
    ACT_S_MOTOR_POS,
    ACT_G_MOTOR_POS,
    ACT_G_MOTOR_RNG,
} act_cb_msg_t;

typedef struct
{
    unsigned int nExpTime;
    unsigned int nExpGain;
    unsigned int nFlashTime;
    unsigned int nMotorPos;
    unsigned int nMotorRng;
} act_msg_info_t;

typedef int (*act_data_cb)(int msg_type, void *msg_info/*act_msg_info_t*/, void *user);

typedef struct
{
    act_data_cb nDataCb;
    void *pusr;
} act_cb_info_t;

typedef struct
{
    int sensor_id;
    int rev[16];
} act_sensor_info_t;

typedef struct
{
    int width;
    int height;
    int framerate;
    int work_mode;
} act_isp_video_info_t;

typedef struct
{
    int blc_en;
    int gr_cut;
    int r_cut;
    int b_cut;
    int gb_cut;
} act_blc_info_t;

typedef struct
{
    int min_iso;
    int max_iso;
    int dpc_en;
    int dpc_num;
    int dpc_thd;
    int nr_en;
    int nr_thd1;
    int nr_thd2;
} act_dpc_iso_info_t;

typedef struct
{
    int enable;
    int iso_num;
    act_dpc_iso_info_t dpc_iso[4];
} act_dpc_info_t;

typedef struct
{
    int clsc_en;
    act_isp_tempture_t coloridx;
    unsigned short lsc_table[64][256];
} act_clsc_info_t;

typedef struct
{
    int rgbgamma_en;
    act_isp_tempture_t coloridx;
    unsigned char rgb_gamma[3][64];
} act_rgb_gamma_info_t;

typedef struct
{
    int mcc_en;
    act_isp_tempture_t coloridx;
    int mcc[9];//2^14 Q10
} act_mcc_info_t;

typedef struct
{
    int fcs_en;
    act_isp_tempture_t coloridx;
    int fcs_gain[256]; //2^8 Q8
} act_fcs_info_t;

typedef struct
{
    int ccs_en;
    act_isp_tempture_t coloridx;
    int nEt;//0-30
    int nCt1;//0-5
    int nCt2;//0-32
    int nYt1;//0-255
} act_ccs_info_t;

typedef struct
{
    int bsch_en;
    act_isp_tempture_t coloridx;
    int nbrightness;
    int nstaturation;
    int ncontrast;
    int hue;
} act_bsch_info_t;

typedef struct
{
    int bCbFixed;
    int bCrFixed;
    int nCbVal;
    int nCrVal;
    int bYInvert;
} act_cfixed_info_t;

typedef struct
{
    int blc_en;
    int dpc_en;
    int lsc_en;
    int rgbgamma_en;
    int fcs_en;
    int ccs_en;
    int bsch_en;
    int abc_en;
} act_cfg_info_t;

typedef struct
{
    char ic_type[16];
    char version_num[16];
    int rev[4];
} act_version_info_t;

typedef struct
{
    int chid;
    int nSkipFrm;
    unsigned int nRawNum;
    unsigned int nRawSize;
    phys_addr_t pRawPhy[8];
    unsigned long pRawVir[8];
} act_rawstore_buffers_info_t;

typedef struct
{
    int FrameRate;
    int TimingHTS;
    int TimingVTS;
    int ShutterMin;
    int ShutterMax;
    int GainMin;
    int GainMax;
} act_ae_sensor_info_t;

typedef struct
{
    int yTargetLow;
    int yTargetHigh;
} act_ae_target_info_t;

typedef struct
{
    int nLowMaxPecent;
    int nHighMaxPecent;
    int nLowMinLevel;
    int nLowMaxLevel;
    int nHighMinLevel;
    int nHighMaxLevel;
    int nLowTargetLevel;
    int nHighTargetLevel;
} act_ae_meanhist_info_t;

typedef struct
{
    act_ae_target_info_t TargetCenterFix;
    int TargetCenterWeight[64];
} act_ae_center_info_t;

typedef struct
{
    int SlowGainYL;
    int SlowGainYH;
    int SlowGainStep1;
    int SlowGainStep2;
} act_ae_step_info_t;

typedef struct
{
    int bVarFrmRate;
    int nVarFrmRate;
    int nVarFrmRateThres[4]; /*7(128)*/
    int nVarFrmRateFPS[4]; /*8(256)*/
} act_ae_frate_ctrl_t;

typedef struct
{
    int FrameRate;
    int TimingHTS;
    int TimingVTS;
    int ShutterMin;
    int ShutterMax;
    int GainMin;
    int GainMax;

    int AlgsAE; /*act_ae_algs_t*/
    int ShutterManual;
    int GainManual;

    // 1. metering
    int AlgsAEMeter; /* act_ae_meter_t */

    // METER_MEAN
    int AlgsAEMeterMean; /*act_ae_mean_t*/
    act_ae_target_info_t TargetMeanFix;
    act_ae_meanhist_info_t TargetMeanHist;

    // METER_WT_CENTER
    act_ae_target_info_t TargetCenterFix;
    int TargetCenterWeight[64];

    // METER_WT_SPOT
    act_ae_target_info_t TargetSpotFix;
    int TargetSpotWeight[16];

    // 2. adjustment
    act_ae_step_info_t GainStep;
    act_ae_frate_ctrl_t FrmRateCtrl;
    int nFrameIntervalAEC;

    // flash
    int nRationFlash2Torch; /* x10 */

    // reserved
    int Reserved[16];
} act_ae_param_t;

typedef struct
{
    int shutter;
    int gain;
} act_ae_exp_info_t;

typedef struct
{
    int fix_en;
    int iso;
} act_ae_fixiso_info_t;

typedef struct
{
    unsigned int nMeanY4x4[16];
    unsigned int nHistY4x4[16][8];
    unsigned int nPostYHist[256];
} act_ae_image_info_t;

typedef struct
{
    unsigned int gain_id;/*0:pre 1:post*/
    unsigned int gain_r;/*Q8*/
    unsigned int gain_g;/*Q8*/
    unsigned int gain_b;/*Q8*/
} act_wbgain_info_t;

typedef struct
{
    int gain_default[3];
    int gain_min[3];
    int gain_max[3];
    int gain_step[3];
    int ctemp_weight[3];
    int rgbgain_weight[3];
} act_awb_info_t;

typedef struct
{
    float min_ratio; /*0 - 1.0f*/
    float rg_bot; /*0 - 4.0f*/
    float rg_top; /*0 - 4.0f*/
    float bg_bot; /*0 - 4.0f*/
    float bg_top; /*0 - 4.0f*/
    float rb_bot; /*0 - 4.0f*/
    float rb_top; /*0 - 4.0f*/
    int y_bot; /*0 - 255*/
    int y_top; /*0 - 255*/
} act_awb_zone_info_t;

typedef struct
{
    act_isp_tempture_t coloridx;
    int ytop; /*12bits*/
    int ybot; /*12bits*/
    int xs; /*12bits*/
    int xe; /*12bits*/
    int ys; /*12bits*/
    int ye; /*12bits*/
    int bmax;/*0 or 1*/
    int rmin;/*0 - 100*/
} act_awb_ct_info_t;

typedef struct
{
    unsigned int wp_thd;
    unsigned int b_top;
    unsigned int g_top;
    unsigned int r_top;
    unsigned int cr_thd;
    unsigned int cb_thd;
    unsigned int y_top;
    unsigned int y_bot;
    float wps_ratio[4]; /*0 - 1.0f*/
} act_wps_info_t;

typedef struct
{
    act_wps_info_t wps[4];
} act_awb_wps_info_t;

typedef struct
{
    int coloridx[4]; /*alg1 alg2 alg3 result*/
    unsigned int rgb_gain[4][3];/*alg1 alg2 alg3 result*/
} act_awb_res_info_t;

typedef struct
{
    unsigned int nAwbTempNum;
    unsigned int nAwbTempValid[MAX_TEMP_NUM];
    unsigned int nAwbTempIdx[MAX_TEMP_NUM];
} act_tempnum_info_t;

typedef struct
{
    float r_g[MAX_TEMP_NUM];
    float b_g[MAX_TEMP_NUM];
} act_wb_distribution_t;

typedef struct
{
    unsigned char gamma[256];
} act_bc_gamma_info_t;

typedef struct
{
    float ratio_cut; /*0 - 0.5f*/
    int down_cut; /*0 - 255*/
    int up_cut; /*0 - 255*/
    int updata_step; /*0 - 100*/
} act_abc_info_t;

typedef struct
{
    unsigned char gamma[256];
    unsigned int hist[256];
} act_bc_res_info_t;

typedef struct
{
    int bsupport;
    int l_pos;
    int r_pos;
} act_af_motor_info_t;

typedef struct
{
    int bfocused; /*bfocused_status*/
    int nrect_num;
    int brect_focused[_SUPPORT_FOCUS_NUM_];
    int blens_dis;
    int nmin_step;
    int nmax_step;
    int ncur_indx; /*当前位置*/
    int nfocused_ratio; /*0-100*/
} act_af_status_info_t;

typedef struct
{
    int nrect;
    int rect_fv[_SUPPORT_FOCUS_NUM_];
    int total_fv;
} act_af_fv_info_t;

typedef struct
{
    int nstride_w;
    int nstride_h;
    int nrect_num; 	//有效区域数目
    int nrect_w; 		//区域宽
    int nrect_h;
    int nrect_valid[_SUPPORT_FOCUS_NUM_];
    int nrect_offset_x[_SUPPORT_FOCUS_NUM_];
    int nrect_offset_y[_SUPPORT_FOCUS_NUM_];
} act_af_region_info_t;

typedef struct
{
    int af_al;
    int nr_thold;
    int tshift;

    int af_inc;
    int win_size;
    unsigned int start_x;
    unsigned int start_y;
    unsigned int skip_wx;
    unsigned int skip_wy;
} act_af_hw_info_t;

typedef struct
{
    int nrect_weights[_SUPPORT_FOCUS_NUM_];
} act_af_weights_info_t;

typedef struct
{
    int dir_pre;
    int coase_step;
    int fine_step;
    int velly_th[3];
} act_af_single_info_t;

typedef struct
{
    int wdet;
    int scene_det;
    int vary_search_time;
    int vary_keep_time;
    int mean_sharp_hl[2];
    int max_sharp_hl[2];
    int peak_area[4];
    int fall_area[4];
    int slow_area[4];
} act_af_continuous_info_t;

/*--------------------init param------------------------*/
typedef struct
{
    int clsc_en;
    unsigned short lsc_table[MAX_TEMP_NUM][64][256];
} act_clsc_param_t;

typedef struct
{
    int rgbgamma_en;
    unsigned char rgb_gamma[MAX_TEMP_NUM][3][64];
} act_rgb_gamma_param_t;

typedef struct
{
    int mcc_en;
    int mcc[MAX_TEMP_NUM][9];//2^14 Q10
} act_mcc_param_t;

typedef struct
{
    int fcs_en;
    int fcs_gain[MAX_TEMP_NUM][256]; //2^8 Q8
} act_fcs_param_t;

typedef struct
{
    int ccs_en;
    int nEt[MAX_TEMP_NUM];//0-30
    int nCt1[MAX_TEMP_NUM];//0-5
    int nCt2[MAX_TEMP_NUM];//0-32
    int nYt1[MAX_TEMP_NUM];//0-255
} act_ccs_param_t;

typedef struct
{
    int bsch_en;
    int nbrightness[MAX_TEMP_NUM];
    int nstaturation[MAX_TEMP_NUM];
    int ncontrast[MAX_TEMP_NUM];
    int hue[MAX_TEMP_NUM];
} act_bsch_param_t;

typedef struct
{
    int ytop[MAX_TEMP_NUM]; /*12bits*/
    int ybot[MAX_TEMP_NUM]; /*12bits*/
    int xs[MAX_TEMP_NUM]; /*12bits*/
    int xe[MAX_TEMP_NUM]; /*12bits*/
    int ys[MAX_TEMP_NUM]; /*12bits*/
    int ye[MAX_TEMP_NUM]; /*12bits*/
    int bmax[MAX_TEMP_NUM]; /*0 or 1*/
    int rmin[MAX_TEMP_NUM]; /*0 - 100*/
} act_awb_ct_param_t;

typedef struct
{
    int bfix_cur_tempure;/*act_isp_tempture_t*/
    act_tempnum_info_t awb_temp;
    act_awb_info_t awb_info;
    act_awb_zone_info_t awb_zone_info;
    act_awb_ct_param_t awb_ct_param;
    act_awb_wps_info_t awb_wps_info;
    act_wb_distribution_t wb_dsb;
} act_awb_param_t;

typedef struct
{
    int bc_algs;
    act_abc_info_t abc_info;
} act_abc_param_t;

typedef struct
{
    act_af_hw_info_t af_hw_info;
    act_af_weights_info_t af_weights_info;
} act_af_res_param_t;

typedef struct
{
    act_af_motor_info_t af_motor_info;
    act_af_single_info_t af_single_info;
    act_af_continuous_info_t af_cts_info;
} act_af_com_param_t;

typedef struct
{
    unsigned int nWidth;
    unsigned int nHeight;
    act_clsc_param_t clsc_param; /* CLSC */
    act_ae_param_t ae_param; 	/*AE */
    act_af_res_param_t af_res_param; /*AF */
} act_isp_res_param_t;

typedef struct
{
    int nres;
    act_isp_res_param_t res_param[MAX_RES_NUM];

    /*Pipeline setting*/
    act_blc_info_t blc_param;
    act_dpc_info_t dpc_param;
    unsigned int bpithd_param;
    act_rgb_gamma_param_t rgbgamma_param;
    act_mcc_param_t mcc_param;
    act_fcs_param_t fcs_param;
    act_ccs_param_t ccs_param;
    act_bsch_param_t bsch_param;

    /*AE setting*/
    int banding_on; /* act_banding_t */

    /*AWB setting*/
    act_awb_param_t awb_param;

    /*ABC setting*/
    act_abc_param_t abc_param;

    /*AF setting*/
    act_af_com_param_t af_com_param;
} act_isp_init_param_t;

typedef enum
{
    /* Base Cmd */
    ACT_ISP_S_SENSOR_ID, /* act_sensor_info_t */
    ACT_ISP_S_CB, /* act_cb_info_t */
    ACT_ISP_S_INIT_PARAM, /* act_isp_init_param_t */
    ACT_ISP_G_INIT_PARAM, /* act_isp_init_param_t* */
    ACT_ISP_S_VIDEOINFO, /* act_isp_video_info_t */
    ACT_ISP_S_START, /* NULL */
    ACT_ISP_S_PROCESS, /* NULL */
    ACT_ISP_S_RUN,  /* NULL */
    ACT_ISP_S_STOP,  /* NULL */

    /* Pipeline */
    ACT_ISP_S_BLC, /* act_blc_info_t */
    ACT_ISP_G_BLC, /* act_blc_info_t */
    ACT_ISP_S_DPC, /* act_dpc_info_t */
    ACT_ISP_G_DPC, /* act_dpc_info_t */
    ACT_ISP_S_CLSC, /* act_clsc_info_t */
    ACT_ISP_G_CLSC, /* act_clsc_info_t */
    ACT_ISP_S_BPITHD, /* unsigned int  */
    ACT_ISP_S_RGBGAMMA, /* act_rgb_gamma_info_t */
    ACT_ISP_G_RGBGAMMA, /* act_rgb_gamma_info_t */
    ACT_ISP_S_MCC,  /* act_mcc_info_t */
    ACT_ISP_G_MCC,  /* act_mcc_info_t */
    ACT_ISP_S_FCS, /* act_fcs_info_t */
    ACT_ISP_G_FCS, /* act_fcs_info_t */
    ACT_ISP_S_CCS, /* act_ccs_info_t */
    ACT_ISP_G_CCS, /* act_ccs_info_t */
    ACT_ISP_S_BRI, /* int (-100 to 100)  */
    ACT_ISP_S_CON, /* int (-100 to 100) */
    ACT_ISP_S_STA, /* int (-100 to 100) */
    ACT_ISP_S_HUE, /* int (-100 to 100) */
    ACT_ISP_S_BSCH, /* act_bsch_info_t */
    ACT_ISP_G_BSCH, /* act_bsch_info_t */
    ACT_ISP_S_CFX, /* act_cfixed_info_t */
    ACT_ISP_S_CFG, /* act_cfg_info_t */
    ACT_ISP_G_CFG, /* act_cfg_info_t */

    /* RAW&Others */
    ACT_ISP_S_RAW_BUFFERS, /*act_rawstore_buffers_info_t*/
    ACT_ISP_S_RAW_RSEN, /*int*/
    ACT_ISP_S_DEBUG, /* NULL */
    ACT_ISP_G_VER, /*act_version_info_t*/

    /* AE */
    ACT_ISP_S_AE_SENSOR_INFO, /*act_ae_sensor_info_t*/
    ACT_ISP_G_AE_SENSOR_INFO,
    ACT_ISP_S_AE_ALGS, /*act_ae_algs_t*/
    ACT_ISP_G_AE_ALGS,
    ACT_ISP_S_AE_METER, /*act_ae_meter_t*/
    ACT_ISP_G_AE_METER,
    ACT_ISP_S_AE_MEAN, /*act_ae_mean_t*/
    ACT_ISP_G_AE_MEAN,
    ACT_ISP_S_AE_MEAN_HS, /*act_ae_meanhist_info_t */
    ACT_ISP_G_AE_MEAN_HS,
    ACT_ISP_S_AE_MEAN_MN, /*act_ae_target_info_t*/
    ACT_ISP_G_AE_MEAN_MN,
    ACT_ISP_S_AE_CTR, /* act_ae_center_info_t */
    ACT_ISP_G_AE_CTR,
    ACT_ISP_S_AE_GAIN_STEP, /*act_ae_step_info_t*/
    ACT_ISP_G_AE_GAIN_STEP,
    ACT_ISP_S_AE_FRATE_CTRL, /*act_ae_frate_ctrl_t*/
    ACT_ISP_G_AE_FRATE_CTRL,
    ACT_ISP_S_AE_FIXISO, /* act_ae_fixiso_info_t */
    ACT_ISP_S_SCENE, /* act_scene_t */
    ACT_ISP_S_BANDING, /* act_banding_t */
	ACT_ISP_G_BANDING, /* act_banding_t */
    ACT_ISP_S_EXPOVAL, /* act_ae_exp_info_t */
    ACT_ISP_G_AE_IMGINFO, /* act_ae_image_info_t */
    ACT_ISP_S_FLASH_STROBE, /*0: off; 1: on*/

    /* AWB */
    ACT_ISP_S_WB_ALGS, /*act_wb_algs_t*/
    ACT_ISP_G_WB_ALGS, /*act_wb_algs_t*/
    ACT_ISP_S_LIGHT_SOURCE, /*act_light_source_t*/
    ACT_ISP_S_WBGAIN,/* act_wbgain_info_t */
    ACT_ISP_G_WBGAIN, /* act_wbgain_info_t */
    ACT_ISP_S_AWB, /*act_awb_info_t*/
    ACT_ISP_G_AWB, /*act_awb_info_t*/
    ACT_ISP_S_AWB_ZONE, /*act_awb_zone_info_t*/
    ACT_ISP_G_AWB_ZONE, /*act_awb_zone_info_t*/
    ACT_ISP_S_AWB_CT, /* act_awb_ct_info_t */
    ACT_ISP_G_AWB_CT, /* act_awb_ct_info_t */
    ACT_ISP_S_AWB_WPS, /*act_awb_wps_info_t*/
    ACT_ISP_G_AWB_WPS, /*act_awb_wps_info_t*/
    ACT_ISP_S_TEMP_NUM, /* act_tempnum_info_t */
    ACT_ISP_G_TEMP_NUM, /* act_tempnum_info_t */
    ACT_ISP_S_FIX_CTEMP, /* act_isp_tempture_t */
    ACT_ISP_G_FIX_CTEMP, /* act_isp_tempture_t */
    ACT_ISP_S_WB_DSB, /* act_wb_distribution_t */
    ACT_ISP_G_WB_DSB, /* act_wb_distribution_t */
    ACT_ISP_G_AWB_RES, /* act_awb_res_info_t */

    /* ABC */
    ACT_ISP_S_BC_ALGS, /* act_bc_algs_t */
    ACT_ISP_G_BC_ALGS, /* act_bc_algs_t */
    ACT_ISP_S_BC_GAMMA, /* act_bc_gamma_info_t */
    ACT_ISP_G_BC_RES, /* act_bc_res_info_t */
    ACT_ISP_S_ABC, /* act_abc_info_t */
    ACT_ISP_G_ABC, /* act_abc_info_t */

    /* AF */
    ACT_ISP_S_AF_ALGS, /* act_af_algs_t */
    ACT_ISP_S_AF_REGION, /* act_af_region_info_t */
    ACT_ISP_S_AF_MODE, /*act_af_mode_t*/
    ACT_ISP_S_AF_RUN,  /* NULL */
    ACT_ISP_S_AF_STOP,  /* NULL */
    ACT_ISP_G_AF_STATUS, /* act_af_status_info_t */

    ACT_ISP_S_AF_MOTOR, /*act_af_motor_info_t*/
    ACT_ISP_G_AF_MOTOR, /*act_af_motor_info_t*/
    ACT_ISP_S_AF_HWINFO, /* act_af_hw_info_t */
    ACT_ISP_G_AF_HWINFO, /* act_af_hw_info_t */
    ACT_ISP_S_AF_WEIGHTS, /* act_af_weights_info_t */
    ACT_ISP_G_AF_WEIGHTS, /* act_af_weights_info_t */
    ACT_ISP_S_AF_POS, /*unsigned int*/
    ACT_ISP_G_AF_FV, /* act_af_fv_info_t */
    ACT_ISP_S_AF_SINGLE, /*act_af_single_info_t*/
    ACT_ISP_G_AF_SINGLE, /*act_af_single_info_t*/
    ACT_ISP_S_AF_CTS, /*act_af_continuous_info_t*/
    ACT_ISP_G_AF_CTS, /*act_af_continuous_info_t*/
} act_isp_ctl_cmd_t;

void *act_isp_open(void);
int act_isp_cmd(void *handle, int cmd, unsigned long mParams);
int act_isp_close(void *handle);

#endif
