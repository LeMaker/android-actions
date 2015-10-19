#ifndef __BISP_DRV__
#define __BISP_DRV__

typedef struct{
	int chid;
	int offset1;
	int offset2;
	int offset3;
	int y_r;
	int y_g;
	int y_b;
	int cb_r;
	int cb_g;
	int cb_b;
	int cr_r;
	int cr_g;
	int cr_b;
}bisp_csc_t;


typedef struct{
	int chid;
	int lsc_en;
	int vinc;
	int hinc;
	int xcenter;
	int ycenter;
	int xcenter_1;
	int scaleidx;
	unsigned int lsc_coeff[64];
}bisp_lsc_t;

typedef struct{
	int chid;
	short nr_en;
	short nr_level;
	short dpc_en;
	short dpc_level;
	int ee_en;
}bisp_nr_t;

typedef struct{
	int chid;
	unsigned int sx;
  unsigned int ex;
  unsigned int sy;
  unsigned int ey;
}bisp_region_t;

typedef struct{
	int chid;
	unsigned int bgain;
	unsigned int ggain;
	unsigned int rgain;
	unsigned int stat_addr;
	unsigned int stat_vddr;
}bisp_bg_t;

typedef struct {
	int chid;
	int wb_thr;
	int wb_thg;
	int wb_thb;
	unsigned int wb_cnt;
}bisp_wp_t;

typedef struct{
	int chid;
	int gc_en;
	unsigned int gc_coeff[16];
}bisp_gc_t;

typedef struct{
	int chid;
	int ow;
	int oh;
	int ofmt;
}bisp_outinfo_t;

typedef struct{
	int chid;
	int color_seq;
	int hsync;
	int vsync;
	int isp_mode;//not care here
	int ch_IF;//parallel/mipi
	int stat_en;
	int eis_en;
}bisp_info_t;

typedef struct{
	int chid;
	int cb;
	int cr;
	int yinc;
}bisp_color_t;


typedef struct{
	int hbank;
	int vbank;
}bisp_bank_t;


typedef struct{
	int chid;
	int b_en;
	unsigned int color_info[16];
}bisp_color_adjust_t;

typedef struct{
	int b_en;
	int vnc_num;
	unsigned int color_info[3][128];
}bisp_vnc_t;

typedef struct{
	int chid;
	unsigned int ads;
	unsigned int vds;
	unsigned int last_addr;
}bisp_ads_t;

typedef struct{
	int chid;
	unsigned int buffer_num;//<16
	unsigned int pHyaddr[16];
	unsigned int pViaddr[16];
}bisp_stat_buffers_info_t;

typedef struct{
	//是否有效统计，1 true，0 false
	int bPrepared;
	unsigned int pv[9];
}af_pv_t;
//算法参数设置，同寄存器
typedef struct{
	//图象的宽高
	int win_w;
	int win_h;
	//调焦算法
	int af_al;
	//采样精度
	int af_inc;
	//中断模式
	int af_mode;
	int rshift;
	int thold;
	int tshift;
	//窗个数-1
	int win_num;
	//窗口大小
	int win_size;
	//相对位置
	unsigned int af_pos[9];
	unsigned int noise_th;
}af_param_t;

typedef struct{
	int r_off;
	int g1_off;
	int g2_off;
	int b_off;
}blc_info_t;
typedef struct{
	char buf[16];
	int  rev[4];
}module_name_t;
#define BRAWISP_DRV_IOC_MAGIC_NUMBER             'P'
//#define BRI_SCH         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x100) //not set ch
#define BRI_SEN         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x101,bisp_info_t)
#define BRI_COP         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x102,bisp_region_t)
#define BRI_VNC         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x103,bisp_vnc_t) // not support here
#define BRI_LUT         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x104,bisp_lsc_t)
#define BRI_NR          _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x105,bisp_nr_t)
#define BRI_EE          _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x106,bisp_nr_t)
#define BRI_DPC         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x107,bisp_nr_t)
#define BRI_GN          _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x108,bisp_bg_t)
#define BRI_SRG         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x109,bisp_region_t)
#define BRI_SHG         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x10A,bisp_region_t)
#define BRI_WBP         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x10B,bisp_wp_t)
#define BRI_CSC         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x10C,bisp_csc_t)
#define BRI_CRP         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x10D,bisp_color_adjust_t)
#define BRI_CFX         _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x10E,bisp_color_t)
#define BRI_GC          _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x10F,bisp_gc_t)
#define BRI_HVB          _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x110,bisp_bank_t)
#define BRI_SBS          _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x120,bisp_stat_buffers_info_t)
#define BRI_FTB          _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x121,bisp_ads_t)
#define BRI_SID          _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x122,int)
#define BRI_BLC          _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x123,blc_info_t)
//完成irq和enable设置
#define AF_CMD_ENABLE          _IO(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x1000)
//disableirq和enable
#define AF_CMD_DISABLE         _IO(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x1001)
//获取统计信息
#define AF_CMD_GET_INFO        _IOR(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x1002,af_pv_t)
//设置窗口和算法参数，设置一次，运行中设置，需要等待一帧
#define AF_CMD_SET_WINPS     	 _IOW(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x1003,af_param_t)

#define BRI_GET_SDS     _IOWR(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x200,bisp_ads_t)
#define BRI_GET_YDS     _IOWR(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x201,bisp_ads_t)
#define BRI_GET_CRP     _IOWR(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x202,bisp_region_t)
#define BRI_GET_GN     _IOWR(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x203,bisp_bg_t)
#define BRI_GET_MODULE_NAME     _IOWR(BRAWISP_DRV_IOC_MAGIC_NUMBER, 0x204,module_name_t)

#endif
