#ifndef __VCE_DRV__
#define __VCE_DRV__

/*
*  driver including UPS and VCE
*/
enum {
VCE_IDLE,
VCE_BUSY,
VCE_READY,
VCE_ERR
};

enum {
VCE_ERR_UNKOWN = -1,
VCE_ERR_BUSY = -100,
VCE_ERR_STM = -201,
VCE_ERR_STM_FULL = -202,
VCE_ERR_TIMEOUT = -203
};

typedef struct {
	unsigned int vce_status;
	unsigned int vce_cfg;
	unsigned int vce_param0;
	unsigned int vce_param1;
	unsigned int vce_strm;
	unsigned int vce_strm_addr;
	unsigned int vce_yaddr;
	unsigned int vce_list0;
	unsigned int vce_list1;
	unsigned int vce_me_param;
	unsigned int vce_swindow;
	unsigned int vce_scale_out;
	unsigned int vce_rect;
	unsigned int vce_rc_param1;
	unsigned int vce_rc_param2;
	unsigned int vce_rc_hdbits;
	unsigned int vce_ts_info;
	unsigned int vce_ts_header;
	unsigned int vce_ts_blu;
	unsigned int vce_ref_dhit;
	unsigned int vce_ref_dmiss;

	unsigned int ups_ctl;
	unsigned int ups_ifs;
	unsigned int ups_str;
	unsigned int ups_ofs;
	unsigned int ups_rath;
	unsigned int ups_ratv;
	unsigned int ups_yas;
	unsigned int ups_cacras;
	unsigned int ups_cras;
	unsigned int ups_bct;
	unsigned int ups_dab;
	unsigned int ups_dwh;
	unsigned int ups_sab0;
	unsigned int ups_sab1;
	unsigned int ups_rgb32_sr;
	unsigned int ups_blend_w;
	unsigned int input_fomat;
}vce_input_t;

typedef struct {
	unsigned int vce_strm;
	unsigned int vce_rc_param3;
	unsigned int vce_rc_hdbits;
	unsigned int strm_addr;
	unsigned int i_ts_offset;
	unsigned int i_ts_header;
	unsigned int vce_ref_dhit;
	unsigned int vce_ref_dmiss;
}vce_output_t;

typedef struct  {
	int width;
	int height;
	unsigned long freq;
}vce_multi_freq_t;

#define VCE_DRV_IOC_MAGIC_NUMBER             'v'
#define VCE_SET_ENABLE_CLK         _IO(VCE_DRV_IOC_MAGIC_NUMBER, 0x0)
#define VCE_SET_DISABLE_CLK        _IO(VCE_DRV_IOC_MAGIC_NUMBER, 0x1)
#define VCE_CMD_ENC_RUN            _IOW(VCE_DRV_IOC_MAGIC_NUMBER, 0x2,vce_input_t)
#define VCE_CMD_QUERY_FINISH       _IOR(VCE_DRV_IOC_MAGIC_NUMBER, 0x3,vce_output_t)
#define VCE_GET_ENC_STATUS         _IO(VCE_DRV_IOC_MAGIC_NUMBER, 0x4)
#define VCE_SET_FREQ               _IOW(VCE_DRV_IOC_MAGIC_NUMBER, 0x5,vce_multi_freq_t)
#define VCE_GET_FREQ               _IO(VCE_DRV_IOC_MAGIC_NUMBER, 0x6)
#define VCE_CHECK_VERSION          _IO(VCE_DRV_IOC_MAGIC_NUMBER, 0x7)

#endif
