#ifndef __VCE_RESIZE_H__
#define __VCE_RESIZE_H__

#include "vce_drv.h"
#include "frame_mng.h"

#define  x_dir 1
#define  y_dir 0
#define  SWAP_T(x,y,z) {z = x;x = y;y = z;}
#undef    MIN
#define   MIN(x,y) ((x)<(y)?(x):(y))
#define  _VCE_SCALE_  4 /*8*/

enum
{
	SCALE_TWO,
	SCALE_EIGHT,
};

int resize_check(int srcw, int srch, int dstw, int dsth, int* s_diff, int* s_dir, int* nead8scale);
void resize_input_init(resize_input_t* resize_in, mng_internal_t *mng_info);
int resize_start(resize_input_t* resize_in,resize_output_t*resize_out,resize_buffer_t* resize_buf,int s_diff,int s_dir,int nead8scale);
void resize_get_output(resize_output_t*resize_out, mng_internal_t *mng_info);
void resize_free_buf(resize_buffer_t* resize_buf);
void resize_recover(mng_internal_t *mng_info);

#endif
