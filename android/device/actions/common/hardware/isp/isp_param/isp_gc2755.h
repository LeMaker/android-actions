#ifndef __ISP_GC2755_H__
#define __ISP_GC2755_H__

#include <string.h>
#include "isp_ctl.h"
#include "isp_gc2755_lsc.h"
#ifndef FIXED_14
#define FIXED_14 16384
#endif

static act_dpc_iso_info_t dpc_iso_table_gc2755[4] =
{
    {1, 4, 1, 6, 120, 0, 320, 320},
    {4, 8, 1, 6, 120, 0, 320, 320},
    {8, 16, 1, 6, 120, 0, 320, 320},
    {16, 32, 1, 6, 120, 0, 320, 320},
};

static unsigned char rgb_gamma_table_gc2755[6 * 3][64] =
{
	{0, 7, 15, 22, 29, 36, 44, 51, 58, 65, 72, 78, 85, 91, 97, 102, 107, 112, 117, 121, 126, 130, 134, 138, 142, 146, 149, 153, 156, 160, 163, 166, 170, 173, 176, 179, 182, 185, 187, 190, 193, 196, 198, 201, 204, 207, 209, 212, 215, 217, 220, 222, 225, 228, 230, 233, 235, 238, 240, 243, 245, 248, 251, 253},
	{0, 7, 15, 22, 29, 36, 44, 51, 58, 65, 72, 78, 85, 91, 97, 102, 107, 112, 117, 121, 126, 130, 134, 138, 142, 146, 149, 153, 156, 160, 163, 166, 170, 173, 176, 179, 182, 185, 187, 190, 193, 196, 198, 201, 204, 207, 209, 212, 215, 217, 220, 222, 225, 228, 230, 233, 235, 238, 240, 243, 245, 248, 251, 253},
	{0, 7, 15, 22, 29, 36, 44, 51, 58, 65, 72, 78, 85, 91, 97, 102, 107, 112, 117, 121, 126, 130, 134, 138, 142, 146, 149, 153, 156, 160, 163, 166, 170, 173, 176, 179, 182, 185, 187, 190, 193, 196, 198, 201, 204, 207, 209, 212, 215, 217, 220, 222, 225, 228, 230, 233, 235, 238, 240, 243, 245, 248, 251, 253},
};

/*d65,d50,cwf,tl84,a,h*/
static int mcc_table_0x0219_gc2755[6][9] =
{
    {1925, FIXED_14 - 638, FIXED_14 - 262, FIXED_14 - 274, 1754, FIXED_14 - 455, FIXED_14 - 73, FIXED_14 - 845, 1943},
    {1925, FIXED_14 - 638, FIXED_14 - 262, FIXED_14 - 274, 1754, FIXED_14 - 455, FIXED_14 - 73, FIXED_14 - 845, 1943},
    {1925, FIXED_14 - 638, FIXED_14 - 262, FIXED_14 - 274, 1754, FIXED_14 - 455, FIXED_14 - 73, FIXED_14 - 845, 1943},
    {1925, FIXED_14 - 638, FIXED_14 - 262, FIXED_14 - 274, 1754, FIXED_14 - 455, FIXED_14 - 73, FIXED_14 - 845, 1943},
    {1646, FIXED_14 - 184, FIXED_14 - 437, FIXED_14 - 431, 1721, FIXED_14 - 266, FIXED_14 - 115, FIXED_14 - 1594, 2734},
    {1646, FIXED_14 - 184, FIXED_14 - 437, FIXED_14 - 431, 1721, FIXED_14 - 266, FIXED_14 - 115, FIXED_14 - 1594, 2734},
};

static int fcs_gain_table_gc2755[256] =
{
    255, 255, 255, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 250, 250, 250, 250, 250, 250, 250, 250, 249, 249, 249, 249, 249, 249, 249, 248, 248, 248, 248, 248, 248, 248, 247, 247, 247, 247, 247, 246, 246, 246, 246, 246, 245, 245, 245, 245, 244, 244, 244, 244, 243, 243, 243, 243, 242, 242, 242, 242, 241, 241, 241, 240, 240, 240, 239, 239, 239, 238, 238, 238, 237, 237, 236, 236, 236, 235, 235, 234, 234, 233, 233, 232, 232, 231, 231, 230, 230, 229, 229, 228, 228, 227, 227, 226, 225, 225, 224, 223, 223, 222, 221, 221, 220, 219, 218, 218, 217, 216, 215, 214, 213, 213, 212, 211, 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 199, 198, 197, 196, 195, 193, 192, 191, 189, 188, 187, 185, 184, 182, 181, 179, 178, 176, 174, 173, 171, 169, 167, 166, 164, 162, 160, 158, 156, 154, 152, 149, 147, 145, 143, 140, 138, 136, 133, 131, 128, 125, 123, 120, 117, 114, 111, 108, 105, 102, 99, 96, 93, 89, 86, 82, 79, 75, 71, 67, 64, 60, 56, 51, 47, 43, 39, 34, 29, 25, 20, 15, 10, 5, 0,
};

static act_awb_info_t awb_info_table_gc2755 =
{
    {459, 256, 426},
    {64, 64, 64},
    {1024, 1024, 1024},
    {50, 50, 50},
    {1, 0, 0},
    {1, 0, 0},
};

/*d65 d50 cwf tl84 a h*/
static act_awb_ct_param_t awb_ct_param_table_gc2755 =
{
    {3360, 3360, 3360, 3360, 3360, 3360},
    {640, 640, 640, 640, 640, 640},
    {290, 278, 343, 272, 286, 249},
    {405, 392, 463, 391, 408, 384},
    { - 176,  - 57, 14, 69, 148, 212},
    { - 48, 55, 122, 165, 254, 348},
    {1, 1, 1, 1, 1, 1},
    {80, 80, 80, 80, 80, 80},
};

static act_wps_info_t awb_wps_table_gc2755[4] =
{
#if 0
    {320, 4080, 4080, 4080, 320, 320, 3520, 480},
    {640, 4080, 4080, 4080, 320, 320, 3520, 480},
    {960, 4080, 4080, 4080, 320, 320, 3520, 480},
    {1600, 4080, 4080, 4080, 320, 320, 3520, 480},
#else
	{520, 4080, 4080, 4080, 300, 50, 1000, 600,{0.05f,0.2f,0.4f,0.6f}},
    {480, 4080, 4080, 4080, 220, 160, 1000, 600,{0.05f,0.2f,0.4f,0.6f}},
    {520, 4080, 4080, 4080, 120, 220, 1000, 600,{0.05f,0.2f,0.4f,0.6f}},
    {500, 4080, 4080, 4080, 80, 280, 1000, 600,{0.05f,0.2f,0.4f,0.6f}},
#endif
};

/*d65 d50 cwf tl84 a h*/
static act_wb_distribution_t wb_dsb_table_gc2755 =
{
    {0.5150f, 0.6140f, 0.6250f, 0.7620f, 0.8460f, 1.0170f},
    {0.6620f, 0.5540f, 0.4860f, 0.4620f, 0.4010f, 0.3700f},
};

static int ae_param_gc2755[1][7 + 8 + 4] =
{
    {5120, 0x0de8, 0x03ed, 1, 0x03ed, 64, 64*32, /*hist param*/5, 95, 22, 233, 15, 30, 225, 240, /*slow gain*/64, 64, 4, 4},
};//fps*256,

static void isphal_set_param_gc2755(act_isp_init_param_t *init_param)
{
    int i, j;

    init_param->nres = 1;
    init_param->res_param[0].nWidth = 1920;
    init_param->res_param[0].nHeight = 1080;

    for(i = 0; i <  init_param->nres; i++)
    {
        init_param->res_param[i].ae_param.FrameRate = ae_param_gc2755[i][0];
        init_param->res_param[i].ae_param.TimingHTS = ae_param_gc2755[i][1];
        init_param->res_param[i].ae_param.TimingVTS = ae_param_gc2755[i][2];
        init_param->res_param[i].ae_param.ShutterMin = ae_param_gc2755[i][3];
        init_param->res_param[i].ae_param.ShutterMax = ae_param_gc2755[i][4];
        init_param->res_param[i].ae_param.GainMin = ae_param_gc2755[i][5];
        init_param->res_param[i].ae_param.GainMax = ae_param_gc2755[i][6];

        init_param->res_param[i].ae_param.AlgsAE = ACT_AE_AUTO;
        init_param->res_param[i].ae_param.AlgsAEMeter = ACT_AE_METER_MEAN;
        init_param->res_param[i].ae_param.AlgsAEMeterMean = ACT_AE_TARGET_HIST;

        init_param->res_param[i].ae_param.TargetMeanFix.yTargetLow = 120;
        init_param->res_param[i].ae_param.TargetMeanFix.yTargetHigh = 130;
		for (j = 0; j < 64; j++) init_param->res_param[i].ae_param.TargetCenterWeight[j] = 64;

        init_param->res_param[i].ae_param.TargetMeanHist.nLowMaxPecent = ae_param_gc2755[i][7 + 0];
        init_param->res_param[i].ae_param.TargetMeanHist.nHighMaxPecent = ae_param_gc2755[i][7 + 1];
        init_param->res_param[i].ae_param.TargetMeanHist.nLowTargetLevel = ae_param_gc2755[i][7 + 2];
        init_param->res_param[i].ae_param.TargetMeanHist.nHighTargetLevel = ae_param_gc2755[i][7 + 3];
        init_param->res_param[i].ae_param.TargetMeanHist.nLowMinLevel = ae_param_gc2755[i][7 + 4];
        init_param->res_param[i].ae_param.TargetMeanHist.nLowMaxLevel = ae_param_gc2755[i][7 + 5];
        init_param->res_param[i].ae_param.TargetMeanHist.nHighMinLevel = ae_param_gc2755[i][7 + 6];
        init_param->res_param[i].ae_param.TargetMeanHist.nHighMaxLevel = ae_param_gc2755[i][7 + 7];
        init_param->res_param[i].ae_param.GainStep.SlowGainYL = ae_param_gc2755[i][15 + 0];
        init_param->res_param[i].ae_param.GainStep.SlowGainYH = ae_param_gc2755[i][15 + 1];
        init_param->res_param[i].ae_param.GainStep.SlowGainStep1 = ae_param_gc2755[i][15 + 2];
        init_param->res_param[i].ae_param.GainStep.SlowGainStep2 = ae_param_gc2755[i][15 + 3];
        
        init_param->res_param[i].ae_param.nFrameIntervalAEC = 2;
        
        init_param->res_param[i].ae_param.nRationFlash2Torch = 30;
    }

    init_param->blc_param.blc_en = 1;
    init_param->blc_param.r_cut = 10;
    init_param->blc_param.gr_cut = 10;
    init_param->blc_param.gb_cut = 10;
    init_param->blc_param.b_cut = 10;

    isphal_set_param_gc2755_lsc(init_param);

    init_param->dpc_param.enable = 1;
    init_param->dpc_param.iso_num = 4;
    for (i = 0; i < 4; i++)
    {
        init_param->dpc_param.dpc_iso[i] = dpc_iso_table_gc2755[i];
    }

    init_param->bpithd_param = 0;

    init_param->rgbgamma_param.rgbgamma_en = 1;
    for (i = 0; i < MAX_TEMP_NUM; i++)
    {
        for (j = 0; j < 3; j++)
        {
            memcpy(init_param->rgbgamma_param.rgb_gamma[i][j], rgb_gamma_table_gc2755[/*i*3+j*/j], 64 * sizeof(unsigned char));
        }
    }

    init_param->mcc_param.mcc_en = 1;
    for (i = 0; i < MAX_TEMP_NUM; i++)
    {
        memcpy(init_param->mcc_param.mcc[i], mcc_table_0x0219_gc2755[i], 9 * sizeof(int));
    }

    init_param->fcs_param.fcs_en = 1;
    for (i = 0; i < MAX_TEMP_NUM; i++)
    {
        memcpy(init_param->fcs_param.fcs_gain[i], fcs_gain_table_gc2755, 256 * sizeof(int));
    }

    init_param->ccs_param.ccs_en = 1;
    for (i = 0; i < MAX_TEMP_NUM; i++)
    {
        init_param->ccs_param.nEt[i] = 1024 / 16;
        init_param->ccs_param.nCt1[i] = 16;
        init_param->ccs_param.nCt2[i] = 8;
        init_param->ccs_param.nYt1[i] = 10;
    }

	init_param->bsch_param.bsch_en = 1;
    for (i = 0; i < MAX_TEMP_NUM; i++)
    {
        init_param->bsch_param.nbrightness[i] = 0;
        init_param->bsch_param.nstaturation[i] = 4;
        init_param->bsch_param.ncontrast[i] = 0;
        init_param->bsch_param.hue[i] = 0;
    }

    init_param->banding_on = ACT_BANDING_50HZ;

    /*awb*/
	init_param->awb_param.bfix_cur_tempure = ACT_NONE;
    init_param->awb_param.awb_temp.nAwbTempNum = 6;
    for(i = 0; i < MAX_TEMP_NUM; i++)
    {
        init_param->awb_param.awb_temp.nAwbTempValid[i] = 1;
    }
	/*CTT Tools:iCam_Tempture_Xe*/
	init_param->awb_param.awb_temp.nAwbTempIdx[0] = 1;
	init_param->awb_param.awb_temp.nAwbTempIdx[1] = 2;
	init_param->awb_param.awb_temp.nAwbTempIdx[2] = 3;
	init_param->awb_param.awb_temp.nAwbTempIdx[3] = 4;
	init_param->awb_param.awb_temp.nAwbTempIdx[4] = 7;
	init_param->awb_param.awb_temp.nAwbTempIdx[5] = 8;

    for (i = 0; i < 3; i++)
    {
        init_param->awb_param.awb_info.gain_default[i] = awb_info_table_gc2755.gain_default[i];
        init_param->awb_param.awb_info.gain_min[i] = awb_info_table_gc2755.gain_min[i];
        init_param->awb_param.awb_info.gain_max[i] = awb_info_table_gc2755.gain_max[i];
        init_param->awb_param.awb_info.gain_step[i] = awb_info_table_gc2755.gain_step[i];
        init_param->awb_param.awb_info.ctemp_weight[i] = awb_info_table_gc2755.ctemp_weight[i];
        init_param->awb_param.awb_info.rgbgain_weight[i] = awb_info_table_gc2755.rgbgain_weight[i];
    }

    init_param->awb_param.awb_zone_info.min_ratio = 0.05f;
    init_param->awb_param.awb_zone_info.rg_bot = 0.4800f;
    init_param->awb_param.awb_zone_info.rg_top = 1.0200f;
    init_param->awb_param.awb_zone_info.bg_bot = 0.3650f;
    init_param->awb_param.awb_zone_info.bg_top = 0.6800f;
    init_param->awb_param.awb_zone_info.rb_bot = 1.12f;
    init_param->awb_param.awb_zone_info.rb_top = 1.35f;
    init_param->awb_param.awb_zone_info.y_bot = 20;
    init_param->awb_param.awb_zone_info.y_top = 220;

    for (i = 0; i < MAX_TEMP_NUM; i++)
    {
        init_param->awb_param.awb_ct_param.ytop[i] = awb_ct_param_table_gc2755.ytop[i];
        init_param->awb_param.awb_ct_param.ybot[i] = awb_ct_param_table_gc2755.ybot[i];
        init_param->awb_param.awb_ct_param.xs[i] = awb_ct_param_table_gc2755.xs[i];
        init_param->awb_param.awb_ct_param.xe[i] = awb_ct_param_table_gc2755.xe[i];
        init_param->awb_param.awb_ct_param.ys[i] = awb_ct_param_table_gc2755.ys[i];
        init_param->awb_param.awb_ct_param.ye[i] = awb_ct_param_table_gc2755.ye[i];
		init_param->awb_param.awb_ct_param.bmax[i] = awb_ct_param_table_gc2755.bmax[i];
		init_param->awb_param.awb_ct_param.rmin[i] = awb_ct_param_table_gc2755.rmin[i];
    }

    for(i = 0; i < 4; i ++)
    {
		init_param->awb_param.awb_wps_info.wps[i] = awb_wps_table_gc2755[i];
    }
    
    init_param->awb_param.wb_dsb = wb_dsb_table_gc2755;

    /*abc*/
    init_param->abc_param.bc_algs = ACT_BC_AUTO;
    init_param->abc_param.abc_info.ratio_cut = 0.0005f;
    init_param->abc_param.abc_info.down_cut = 32;
    init_param->abc_param.abc_info.up_cut = 256 - 64;
    init_param->abc_param.abc_info.updata_step = 20;
}

#endif /* __ISP_GC2755_H__ */