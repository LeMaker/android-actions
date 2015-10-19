#ifndef  __DRV_RESIZE_H__
#define  __DRV_RESIZE_H__

#include "vce_cfg.h"
#include "vce_drv.h"

typedef struct  drv_input_t
{
	int vcedrv_fd;

	unsigned long src_addr;
	unsigned long srcy;
	unsigned long srccb;
	unsigned long srccr;
	int srcstride;
	int srcw;
	int srch;

	unsigned long scale_phy;
	int scale_len;
#ifndef IC_TYPE_GL5207
	int scale_w;
	int scale_h;
#endif

	int issemi;
	int isyuv420;
	int enc_mode;
	int b_ds;
#ifndef IC_TYPE_GL5207
	int i_ds_lv;
#endif
}drv_input_t;

typedef struct drv_output_t
{
	int dst_width;
	int dst_height;
	unsigned long dst_phyaddr;
}drv_output_t;

typedef struct vcedrv_freq_t
{
	vce_multi_freq_t vce_frep;
	int vce_frep_init;
}vcedrv_freq_t;

int drv_resize_run(drv_input_t* drv_in, drv_output_t* drv_out);
void drv_resize_setfreq(vcedrv_freq_t* vcedrv_freq,drv_input_t* drv_in,drv_output_t* drv_out);

#endif