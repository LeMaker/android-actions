#ifndef __ISP_IMX0219_H__
#define __ISP_IMX0219_H__

#include <string.h>
#include "isp_ctl.h"
#include "isp_imx0219_lsc.h"
#ifndef FIXED_14
#define FIXED_14 16384
#endif

static act_dpc_iso_info_t dpc_iso_table_imx0219[4] =
{
    {1, 4, 1, 6, 120, 0, 320, 320},
    {4, 8, 1, 6, 120, 0, 320, 320},
    {8, 16, 1, 6, 120, 0, 320, 320},
    {16, 32, 1, 6, 120, 0, 320, 320},
};

static unsigned char rgb_gamma_table_imx0219[6 * 3][64] =
{
	{0, 11, 22, 34, 45, 56, 67, 77, 85, 91, 97, 103, 108, 113, 117, 122, 126, 130, 134, 138, 141, 145, 148, 152, 155, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 188, 191, 194, 196, 199, 201, 204, 206, 209, 211, 214, 216, 218, 221, 223, 225, 227, 230, 232, 234, 236, 238, 240, 243, 245, 247, 249, 251, 253},
	{0, 11, 22, 34, 45, 56, 67, 77, 85, 91, 97, 103, 108, 113, 117, 122, 126, 130, 134, 138, 141, 145, 148, 152, 155, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 188, 191, 194, 196, 199, 201, 204, 206, 209, 211, 214, 216, 218, 221, 223, 225, 227, 230, 232, 234, 236, 238, 240, 243, 245, 247, 249, 251, 253},
	{0, 11, 22, 34, 45, 56, 67, 77, 85, 91, 97, 103, 108, 113, 117, 122, 126, 130, 134, 138, 141, 145, 148, 152, 155, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 188, 191, 194, 196, 199, 201, 204, 206, 209, 211, 214, 216, 218, 221, 223, 225, 227, 230, 232, 234, 236, 238, 240, 243, 245, 247, 249, 251, 253},
};

/*d65,d50,cwf,tl84,a,h*/
static int mcc_table_0x0219_imx0219[6][9] =
{
#if 1
    {2130, FIXED_14 - 979, FIXED_14 - 126, FIXED_14 - 341, 1799, FIXED_14 - 433, FIXED_14 - 10, FIXED_14 - 885, 1920},
    {2130, FIXED_14 - 979, FIXED_14 - 126, FIXED_14 - 341, 1799, FIXED_14 - 433, FIXED_14 - 10, FIXED_14 - 885, 1920},
    {2130, FIXED_14 - 979, FIXED_14 - 126, FIXED_14 - 341, 1799, FIXED_14 - 433, FIXED_14 - 10, FIXED_14 - 885, 1920},
    {2130, FIXED_14 - 979, FIXED_14 - 126, FIXED_14 - 341, 1799, FIXED_14 - 433, FIXED_14 - 10, FIXED_14 - 885, 1920},
    {1872, FIXED_14 - 580, FIXED_14 - 267, FIXED_14 - 624, 1986, FIXED_14 - 337, FIXED_14 - 110, FIXED_14 - 1809, 2944},
    {1872, FIXED_14 - 580, FIXED_14 - 267, FIXED_14 - 624, 1986, FIXED_14 - 337, FIXED_14 - 110, FIXED_14 - 1809, 2944}
#else
    {2000, FIXED_14 - 791, FIXED_14 - 185, FIXED_14 - 292, 1773, FIXED_14 - 457, 154, FIXED_14 - 1131, 2001},
    {2000, FIXED_14 - 791, FIXED_14 - 185, FIXED_14 - 292, 1773, FIXED_14 - 457, 154, FIXED_14 - 1131, 2001},
    {2000, FIXED_14 - 791, FIXED_14 - 185, FIXED_14 - 292, 1773, FIXED_14 - 457, 154, FIXED_14 - 1131, 2001},
    {2000, FIXED_14 - 791, FIXED_14 - 185, FIXED_14 - 292, 1773, FIXED_14 - 457, 154, FIXED_14 - 1131, 2001},
    {2000, FIXED_14 - 791, FIXED_14 - 185, FIXED_14 - 292, 1773, FIXED_14 - 457, 154, FIXED_14 - 1131, 2001},
    {2000, FIXED_14 - 791, FIXED_14 - 185, FIXED_14 - 292, 1773, FIXED_14 - 457, 154, FIXED_14 - 1131, 2001},
#endif
};

static int fcs_gain_table_imx0219[256] =
{
    255, 255, 255, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 250, 250, 250, 250, 250, 250, 250, 250, 249, 249, 249, 249, 249, 249, 249, 248, 248, 248, 248, 248, 248, 248, 247, 247, 247, 247, 247, 246, 246, 246, 246, 246, 245, 245, 245, 245, 244, 244, 244, 244, 243, 243, 243, 243, 242, 242, 242, 242, 241, 241, 241, 240, 240, 240, 239, 239, 239, 238, 238, 238, 237, 237, 236, 236, 236, 235, 235, 234, 234, 233, 233, 232, 232, 231, 231, 230, 230, 229, 229, 228, 228, 227, 227, 226, 225, 225, 224, 223, 223, 222, 221, 221, 220, 219, 218, 218, 217, 216, 215, 214, 213, 213, 212, 211, 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 199, 198, 197, 196, 195, 193, 192, 191, 189, 188, 187, 185, 184, 182, 181, 179, 178, 176, 174, 173, 171, 169, 167, 166, 164, 162, 160, 158, 156, 154, 152, 149, 147, 145, 143, 140, 138, 136, 133, 131, 128, 125, 123, 120, 117, 114, 111, 108, 105, 102, 99, 96, 93, 89, 86, 82, 79, 75, 71, 67, 64, 60, 56, 51, 47, 43, 39, 34, 29, 25, 20, 15, 10, 5, 0,
};

static act_awb_info_t awb_info_table_imx0219 =
{
    {406, 256, 393},
    {64, 64, 64},
    {1024, 1024, 1024},
    {50, 50, 50},
    {1, 0, 0},
    {1, 0, 0},
};

/*d65 d50 cwf tl84 a h*/
static act_awb_ct_param_t awb_ct_param_table_imx0219 =
{
    {1200, 1200, 1200, 1200, 1200, 1200},
    {600, 600, 600, 600, 600, 600},
    {230, 250, 253, 220, 260, 260},
    {270, 290, 290, 253, 300, 300},
    { - 155,  - 70, 12, 50, 102, 210},
    { - 65, 20, 102, 115, 210, 307},
    {1, 1, 1, 1, 1, 1},
    {80, 80, 80, 80, 80, 80},
};

static act_wps_info_t awb_wps_table_imx0219[4] =
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
static act_wb_distribution_t wb_dsb_table_imx0219 =
{
    {0.5653f, 0.6365f, 0.7200f, 0.8255f, 0.9111f, 1.0577f},
    {0.7277f, 0.5945f, 0.5141f, 0.5047f, 0.3990f, 0.3384f},
};

static int ae_param_imx0219[3][7 + 8 + 4] =
{
    {7168, 0x0d78, 0x0a82, 1, 0x0a7e, 64, 1024 * 2, /*hist param*/5, 95, 22, 233, 15, 30, 225, 240, /*slow gain*/64, 64, 4, 4},
    {1875, 0x0d78, 0x14c8, 1, 0x14c4, 64, 1024 * 2, /*hist param*/5, 95, 22, 233, 15, 30, 225, 240, /*slow gain*/64, 64, 4, 4},
    {7168, 0x0d78, 0x0a82, 1, 0x0a7e, 64, 1024 * 2, /*hist param*/5, 95, 22, 233, 15, 30, 225, 240, /*slow gain*/64, 64, 4, 4},
};//fps*256,

static act_af_res_param_t af_res_param_table_imx0219 =
{
	{0,16,3, 2,3,416,216,0,0},
	{{1,1,1,1,1,1,1,1,1}},
};

static  act_af_com_param_t af_com_param_table_imx0219 =
{
	{1,0,500},
	{2,20,8,{1000,1000,1000}},
	{80,5,3,8, {40000,100000},{60000,120000},  {6,60,6,1000}, {16,60,10,1000}, {20,60,20,1000}},
};

static void isphal_set_param_imx0219(act_isp_init_param_t *init_param)
{
    int i, j;

    init_param->nres = 3;
    init_param->res_param[0].nWidth = 1600;
    init_param->res_param[0].nHeight = 1200;
    init_param->res_param[1].nWidth = 3264;
    init_param->res_param[1].nHeight = 2448;
    init_param->res_param[2].nWidth = 800;
    init_param->res_param[2].nHeight = 600;

    for(i = 0; i <  init_param->nres; i++)
    {
        init_param->res_param[i].ae_param.FrameRate = ae_param_imx0219[i][0];
        init_param->res_param[i].ae_param.TimingHTS = ae_param_imx0219[i][1];
        init_param->res_param[i].ae_param.TimingVTS = ae_param_imx0219[i][2];
        init_param->res_param[i].ae_param.ShutterMin = ae_param_imx0219[i][3];
        init_param->res_param[i].ae_param.ShutterMax = ae_param_imx0219[i][4];
        init_param->res_param[i].ae_param.GainMin = ae_param_imx0219[i][5];
        init_param->res_param[i].ae_param.GainMax = ae_param_imx0219[i][6];

        init_param->res_param[i].ae_param.AlgsAE = ACT_AE_AUTO;
        init_param->res_param[i].ae_param.AlgsAEMeter = ACT_AE_METER_MEAN;
        init_param->res_param[i].ae_param.AlgsAEMeterMean = ACT_AE_TARGET_HIST;

        init_param->res_param[i].ae_param.TargetMeanFix.yTargetLow = 120;
        init_param->res_param[i].ae_param.TargetMeanFix.yTargetHigh = 130;
		for (j = 0; j < 64; j++) init_param->res_param[i].ae_param.TargetCenterWeight[j] = 64;

        init_param->res_param[i].ae_param.TargetMeanHist.nLowMaxPecent = ae_param_imx0219[i][7 + 0];
        init_param->res_param[i].ae_param.TargetMeanHist.nHighMaxPecent = ae_param_imx0219[i][7 + 1];
        init_param->res_param[i].ae_param.TargetMeanHist.nLowTargetLevel = ae_param_imx0219[i][7 + 2];
        init_param->res_param[i].ae_param.TargetMeanHist.nHighTargetLevel = ae_param_imx0219[i][7 + 3];
        init_param->res_param[i].ae_param.TargetMeanHist.nLowMinLevel = ae_param_imx0219[i][7 + 4];
        init_param->res_param[i].ae_param.TargetMeanHist.nLowMaxLevel = ae_param_imx0219[i][7 + 5];
        init_param->res_param[i].ae_param.TargetMeanHist.nHighMinLevel = ae_param_imx0219[i][7 + 6];
        init_param->res_param[i].ae_param.TargetMeanHist.nHighMaxLevel = ae_param_imx0219[i][7 + 7];
        init_param->res_param[i].ae_param.GainStep.SlowGainYL = ae_param_imx0219[i][15 + 0];
        init_param->res_param[i].ae_param.GainStep.SlowGainYH = ae_param_imx0219[i][15 + 1];
        init_param->res_param[i].ae_param.GainStep.SlowGainStep1 = ae_param_imx0219[i][15 + 2];
        init_param->res_param[i].ae_param.GainStep.SlowGainStep2 = ae_param_imx0219[i][15 + 3];
        
        init_param->res_param[i].ae_param.nFrameIntervalAEC = 0;
        
        init_param->res_param[i].ae_param.nRationFlash2Torch = 30;
    }

    init_param->blc_param.blc_en = 1;
    init_param->blc_param.r_cut = 255;
    init_param->blc_param.gr_cut = 255;
    init_param->blc_param.gb_cut = 255;
    init_param->blc_param.b_cut = 255;

    isphal_set_param_imx0219_lsc(init_param);

    init_param->dpc_param.enable = 1;
    init_param->dpc_param.iso_num = 4;
    for (i = 0; i < 4; i++)
    {
        init_param->dpc_param.dpc_iso[i] = dpc_iso_table_imx0219[i];
    }

    init_param->bpithd_param = 0;

    init_param->rgbgamma_param.rgbgamma_en = 1;
    for (i = 0; i < MAX_TEMP_NUM; i++)
    {
        for (j = 0; j < 3; j++)
        {
            memcpy(init_param->rgbgamma_param.rgb_gamma[i][j], rgb_gamma_table_imx0219[/*i*3+j*/j], 64 * sizeof(unsigned char));
        }
    }

    init_param->mcc_param.mcc_en = 1;
    for (i = 0; i < MAX_TEMP_NUM; i++)
    {
        memcpy(init_param->mcc_param.mcc[i], mcc_table_0x0219_imx0219[i], 9 * sizeof(int));
    }

    init_param->fcs_param.fcs_en = 1;
    for (i = 0; i < MAX_TEMP_NUM; i++)
    {
        memcpy(init_param->fcs_param.fcs_gain[i], fcs_gain_table_imx0219, 256 * sizeof(int));
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
        init_param->awb_param.awb_info.gain_default[i] = awb_info_table_imx0219.gain_default[i];
        init_param->awb_param.awb_info.gain_min[i] = awb_info_table_imx0219.gain_min[i];
        init_param->awb_param.awb_info.gain_max[i] = awb_info_table_imx0219.gain_max[i];
        init_param->awb_param.awb_info.gain_step[i] = awb_info_table_imx0219.gain_step[i];
        init_param->awb_param.awb_info.ctemp_weight[i] = awb_info_table_imx0219.ctemp_weight[i];
        init_param->awb_param.awb_info.rgbgain_weight[i] = awb_info_table_imx0219.rgbgain_weight[i];
    }

    init_param->awb_param.awb_zone_info.min_ratio = 0.05f;
    init_param->awb_param.awb_zone_info.rg_bot = 0.5239f;
    init_param->awb_param.awb_zone_info.rg_top = 1.0577f;
    init_param->awb_param.awb_zone_info.bg_bot = 0.3384f;
    init_param->awb_param.awb_zone_info.bg_top = 0.7494f;
    init_param->awb_param.awb_zone_info.rb_bot = 1.20f;
    init_param->awb_param.awb_zone_info.rb_top = 1.40f;
    init_param->awb_param.awb_zone_info.y_bot = 20;
    init_param->awb_param.awb_zone_info.y_top = 220;

    for (i = 0; i < MAX_TEMP_NUM; i++)
    {
        init_param->awb_param.awb_ct_param.ytop[i] = awb_ct_param_table_imx0219.ytop[i];
        init_param->awb_param.awb_ct_param.ybot[i] = awb_ct_param_table_imx0219.ybot[i];
        init_param->awb_param.awb_ct_param.xs[i] = awb_ct_param_table_imx0219.xs[i];
        init_param->awb_param.awb_ct_param.xe[i] = awb_ct_param_table_imx0219.xe[i];
        init_param->awb_param.awb_ct_param.ys[i] = awb_ct_param_table_imx0219.ys[i];
        init_param->awb_param.awb_ct_param.ye[i] = awb_ct_param_table_imx0219.ye[i];
		init_param->awb_param.awb_ct_param.bmax[i] = awb_ct_param_table_imx0219.bmax[i];
		init_param->awb_param.awb_ct_param.rmin[i] = awb_ct_param_table_imx0219.rmin[i];
    }

    for(i = 0; i < 4; i ++)
    {
		init_param->awb_param.awb_wps_info.wps[i] = awb_wps_table_imx0219[i];
    }
    
    init_param->awb_param.wb_dsb = wb_dsb_table_imx0219;

    /*abc*/
    init_param->abc_param.bc_algs = ACT_BC_AUTO;
    init_param->abc_param.abc_info.ratio_cut = 0.0005f;
    init_param->abc_param.abc_info.down_cut = 32;
    init_param->abc_param.abc_info.up_cut = 256 - 64;
    init_param->abc_param.abc_info.updata_step = 20;

	/*af*/
	init_param->res_param[0].af_res_param = af_res_param_table_imx0219;
	init_param->res_param[1].af_res_param = af_res_param_table_imx0219;
	init_param->af_com_param =af_com_param_table_imx0219;
}

#endif /* __ISP_IMX0219_H__ */