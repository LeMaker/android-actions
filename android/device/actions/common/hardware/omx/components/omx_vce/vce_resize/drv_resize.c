#include <string.h>
#include <sys/ioctl.h>
#include "log.h"
#include "drv_resize.h"

#define ALIGN_SRC_HEIGHT(x)   (((x) + 0x0) & (~0x7))
#define ALIGN_SRC_WIDTH(x)    (((x) + 0x0) & (~0x7))

#if 0
static void print_vce_input_t(vce_input_t* vce_input)
{
	printf("vce_input_t1!vce_status:%x,vce_cfg:%x,vce_param0:%x,vce_param1:%x\n",
		vce_input->vce_status,vce_input->vce_cfg,vce_input->vce_param0,vce_input->vce_param1);

	printf("vce_input_t2!vce_strm:%x,vce_strm_addr:%x,vce_yaddr:%x,vce_list0:%x\n",
		vce_input->vce_strm,vce_input->vce_strm_addr,vce_input->vce_yaddr,vce_input->vce_list0);

	printf("vce_input_t3!vce_list1:%x,vce_me_param:%x,vce_swindow:%x,vce_scale_out:%x\n",
		vce_input->vce_list1,vce_input->vce_me_param,vce_input->vce_swindow,vce_input->vce_scale_out);

	printf("vce_input_t4!vce_rect:%x,vce_rc_param1:%x,vce_rc_param2:%x,vce_rc_hdbits:%x\n",
		vce_input->vce_rect,vce_input->vce_rc_param1,vce_input->vce_rc_param2,vce_input->vce_rc_hdbits);

	printf("vce_input_t5!vce_ts_info:%x,vce_ts_header:%x,vce_ts_blu:%x\n",
		vce_input->vce_ts_info,vce_input->vce_ts_header,vce_input->vce_ts_blu);

	printf("vce_input_t6!ups_ctl:%x,ups_ifs:%x,ups_str:%x,ups_ofs:%x\n",
		vce_input->ups_ctl,vce_input->ups_ifs,vce_input->ups_str,vce_input->ups_ofs);

	printf("vce_input_t7!ups_rath:%x,ups_ratv:%x,ups_yas:%x,ups_cacras:%x\n",
		vce_input->ups_rath,vce_input->ups_ratv,vce_input->ups_yas,vce_input->ups_cacras);

	printf("vce_input_t8!ups_cras:%x,ups_bct:%x,ups_dab:%x,ups_dwh:%x\n",
		vce_input->ups_cras,vce_input->ups_bct,vce_input->ups_dab,vce_input->ups_dwh);

	printf("vce_input_t9!ups_sab0:%x,ups_sab1:%x\n",
		vce_input->ups_sab0,vce_input->ups_sab1);
}

static void print_vce_output_t(vce_output_t* vce_output)
{
	printf("vce_output_t1!vce_strm:%x,vce_rc_param3:%x,vce_rc_hdbits:%x\n",
		vce_output->vce_strm,vce_output->vce_rc_param3,vce_output->vce_rc_hdbits);

	printf("vce_output_t2!strm_addr:%x,i_ts_offset:%x,i_ts_header:%x\n",
		vce_output->strm_addr,vce_output->i_ts_offset,vce_output->i_ts_header);
}
#endif

static int drv_resize_get_ratios(int mdstw,int mdsth,int *srcw,int *srch,unsigned int *bx,unsigned int *by,unsigned int *cv)
{
	int ret = 0;
	int cov_ratio = 1;
	int scale_level = 0;
	int msrcw = *srcw;
	int msrch = *srch;
	float scale_factor_h = (float) (mdstw / 8) / (float) (msrcw / 8);
	float scale_factor_v = (float) (mdsth / 8) / (float) (msrch / 8);
	float scale_factor = scale_factor_h;

	//printf("src scale ratio:%f X %f\n",scale_factor_h,scale_factor_v);
	while (scale_factor_h > 2 || scale_factor_v > 2)
	{
		if (scale_factor_v < scale_factor)
			scale_factor = scale_factor_v;
		while (scale_factor >= 2)
		{
			scale_factor /= 2;
			scale_level++;
			cov_ratio = cov_ratio * 2;
			scale_factor_h /= 2;
			scale_factor_v /= 2;
		}

		if (scale_factor_h <= 2 && scale_factor_v <= 2)
		{

		}
		else
		{
			scale_factor = scale_factor_h;
			if (scale_factor_h > scale_factor_v)
			{
				scale_factor_v = scale_factor_v * 2;
			}
			else
			{
				scale_factor_h = scale_factor_h * 2;
			}
			scale_factor = scale_factor_h;
		}
	}

	//printf("cov_ratio:%d,scale_factor_h:%f,scale_factor_v:%f\n",cov_ratio,scale_factor_h,scale_factor_v);

	/*限制条件*/
	msrcw = (unsigned int) ((float) mdstw / ((float) cov_ratio * scale_factor_h) + 0.5f);
	msrch = (unsigned int) ((float) mdsth / ((float) cov_ratio * scale_factor_v) + 0.5f);
//#ifdef MOD_FOR_UNALIGN_RES
	msrcw = ALIGN_SRC_WIDTH(msrcw);
	msrch = ALIGN_SRC_HEIGHT(msrch);
//#else
//	msrcw = msrcw & (~0xf);
//	msrch = msrch & (~0xf);
//#endif

	scale_factor = (float) cov_ratio;

	/*只需要做COV*/
	if (((int) (mdstw) == (msrcw * (int) scale_factor)) && ((int) (mdsth) == (msrch * (int) scale_factor)))
	{
		*bx = 0;
		*by = 0;
	}
	else
	{
		/*计算bilinear 倍数*/
		*bx = (unsigned int) (scale_factor * msrcw * (1 << 20)) / mdstw;
		*by = (unsigned int) (scale_factor * msrch * (1 << 20)) / mdsth;
		if (*bx >= (1 << 20))
			*bx = (1 << 20) - 1;
		if (*by >= (1 << 20))
			*by = (1 << 20) - 1;
	}
	*srcw = msrcw;
	*srch = msrch;
	*cv = scale_level;

	//printf("*bx :%x,*by:%x\n",*bx ,*by);
	return ret;
}

static void drv_resize_set_reg(vce_input_t* vce_in, drv_input_t* drv_in, drv_output_t* drv_out)
{
	int srcstride = drv_in->srcstride;
	int mSrcw = drv_in->srcw;
	int mSrch = drv_in->srch;
#ifndef IC_TYPE_GL5207
	int mDstw = drv_in->scale_w;
	int mDsth = drv_in->scale_h;
#else
	int mDstw = drv_out->dst_width;
	int mDsth = drv_out->dst_height;
#endif
	int issemi = drv_in->issemi;
	int isyuv420 = drv_in->isyuv420;

	unsigned long mSrcY = drv_in->srcy;
	unsigned long mSrcCb = drv_in->srccb;
	unsigned long mSrcCr = drv_in->srccr;
#ifndef IC_TYPE_GL5207
	unsigned int strm_len = drv_in->scale_len;
	unsigned long strm_addr = drv_in->scale_phy;
	unsigned long ds_phyaddr = drv_out->dst_phyaddr;
	int i_ds_lv = drv_in->i_ds_lv;
#else
	unsigned int strm_len = mDstw*mDsth*3/2;
	unsigned long strm_addr = drv_out->dst_phyaddr;
	unsigned long ds_phyaddr = drv_in->scale_phy;
#endif
	int b_ds = drv_in->b_ds;

	unsigned int ups_ctl;
	unsigned int vce_cfg;
	unsigned int bx, by, cv;

	/*Clear*/
	memset(vce_in, 0, sizeof(vce_input_t));

	/*PreView  mode*/
	vce_in->vce_status = 0;

#ifdef IC_TYPE_GL5207
	/*Input Fomat*/
	if(isyuv420)
	{
		vce_in->input_fomat = 2;
		if(issemi == 0)
		{
			vce_in->input_fomat = vce_in->input_fomat | (1<<31);
		}
	}
#endif

	/*Vce CFG*/
	vce_cfg = 0;
	vce_cfg = ((mDstw >> 4) << 20) | ((mDsth >> 4) << 8) | (1 << 5);
	if (issemi)
		vce_cfg |= (0 << 7);
	else
		vce_cfg |= (1 << 7);

	if (isyuv420)
		vce_cfg |= (0 << 4) | (2 << 0);
	else
		vce_cfg |= (1 << 4) | (5 << 0);
	vce_in->vce_cfg = vce_cfg;

	/*输出码流*/
	vce_in->vce_strm = strm_len;
	vce_in->vce_strm_addr = strm_addr;

	/*预览输入or输出地址？*/
	vce_in->vce_yaddr = strm_addr;

	/*输入大小*/
	vce_in->ups_ifs = (mSrcw >> 3) + ((mSrch >> 3) << 16);

	/*输入stride*/
	vce_in->ups_str = srcstride >> 3;

	/*输出大小*/
	vce_in->ups_ofs = (mDstw >> 4) + ((mDsth >> 4) << 16);

	/*横向和纵向放大倍数*/
	drv_resize_get_ratios(mDstw, mDsth, &mSrcw, &mSrch, &bx, &by, &cv);
#ifndef IC_TYPE_GL5207
	if (bx == 0 && by == 0 && srcstride != mSrcw)
	{
		/*mSrcW != mSrcStride即Crop, 必须都开启横向和纵向的双线性插值*/
		bx = (1 << 20) - 1;
		by = (1 << 20) - 1;
	}
#endif
	vce_in->ups_rath = bx;
	vce_in->ups_ratv = by;

	/*上采样等控制*/
	ups_ctl = 1; //5202 only support data from ddr
	if (bx != 0)
		ups_ctl |= (1 << 6);
	if (by != 0)
		ups_ctl |= (1 << 5);
	if (cv != 0)
	{
		ups_ctl |= (1 << 3);
		ups_ctl |= ((cv - 1) << 4);
		ups_ctl |= (1 << 2);
		ups_ctl |= (7 << 13);
		ups_ctl |= (15 << 7);
	}
	vce_in->ups_ctl = ups_ctl;

	/*输入地址*/
	vce_in->ups_yas = mSrcY;
	vce_in->ups_cacras = mSrcCb;
	vce_in->ups_cras = mSrcCr;

	/*dowmsclae*/
	if(b_ds)
	{
#ifndef IC_TYPE_GL5207
		vce_in->vce_param0 = ((i_ds_lv &0xf) << 16) |  (b_ds << 15);
#endif
		vce_in->vce_scale_out = ds_phyaddr;
	}
}

int drv_resize_run(drv_input_t* drv_in, drv_output_t* drv_out)
{
	int ret = 0,try_i = 0;
	vce_input_t vce_in;
	vce_output_t vce_out;
	int vcedrv_fd = drv_in->vcedrv_fd;
	
	drv_resize_set_reg(&vce_in, drv_in, drv_out);
	//print_vce_input_t(&vce_in);
	//printf("vcedrv_fd:%x\n",vcedrv_fd);
	
	do
	{
		ret = ioctl(vcedrv_fd, VCE_CMD_ENC_RUN, &vce_in);

		if ((ret == 0) || (try_i++ > 200))
			break;
		usleep(1000);
	} while (1);

	if (ret < 0)
	{
		printf("err!scale VCE_CMD_ENC_RUN fail! %s,%d!\n", __FILE__, __LINE__);
		return -1;
	}

	ret = ioctl(vcedrv_fd, VCE_CMD_QUERY_FINISH, &vce_out);
	if (ret < 0)
	{
		printf("err!scale VCE_CMD_QUERY_FINISH fail! %s,%d!\n", __FILE__, __LINE__);
		return -1;
	}
	//print_vce_output_t(&vce_out);

	return ret;
}

#ifdef IC_TYPE_GL5207
#define  Vce_Frequency_Base 660 /*MHZ*/
void get_vce_frep(vce_multi_freq_t* vce_frep)
{
	int sizes = vce_frep->width * vce_frep->height;
	//printf("1 get_vce_frep!width:%d,height:%d\n",vce_frep->width,vce_frep->height);
	if(sizes >= 1920*1080)
		vce_frep->freq = Vce_Frequency_Base/2;
	else if(sizes > 1280*720)
		vce_frep->freq = Vce_Frequency_Base/3;
	else if(sizes > 760*570)
		vce_frep->freq = Vce_Frequency_Base/4;
	else
		vce_frep->freq = Vce_Frequency_Base/6;
}
#else
#define  Vce_Frequency_Base 720 /*MHZ*/
void get_vce_frep(vce_multi_freq_t* vce_frep)
{
	int sizes = vce_frep->width * vce_frep->height;
	//printf("2 get_vce_frep!width:%d,height:%d\n", vce_frep->width, vce_frep->height);
	if (sizes > 1280 * 720)
		vce_frep->freq = Vce_Frequency_Base / 2;
	else if (sizes > 760 * 570)
		vce_frep->freq = Vce_Frequency_Base / 3;
	else
		vce_frep->freq = Vce_Frequency_Base / 4;
}
#endif

/*调频*/
void drv_resize_setfreq(vcedrv_freq_t* vcedrv_freq,drv_input_t* drv_in,drv_output_t* drv_out)
{
	int ret = 0;
    if(drv_in->srcw > drv_out->dst_width || drv_in->srch > drv_out->dst_height)
    {
    	if( (vcedrv_freq->vce_frep.width != drv_in->srcw) || 
    		(vcedrv_freq->vce_frep.height != drv_in->srch) || 
    		(vcedrv_freq->vce_frep_init == 0) )
    	{
    		vcedrv_freq->vce_frep_init = 1;
    		vcedrv_freq->vce_frep.width = drv_in->srcw;
    		vcedrv_freq->vce_frep.height = drv_in->srch;
    		get_vce_frep(&(vcedrv_freq->vce_frep));
    		//printf("vcedrv_freq->vce_frep.freq:%ld\n",vcedrv_freq->vce_frep.freq);
    		ret = ioctl(drv_in->vcedrv_fd, VCE_SET_FREQ, &vcedrv_freq->vce_frep);
    		if (ret < 0)
    		{
    			printf("Warning!ioctl VCE_SET_FREQ fail! %s,%d!\n", __FILE__, __LINE__);
    		}
    	}
    }
    else
    {
        if( (vcedrv_freq->vce_frep.width != drv_out->dst_width) || 
    		(vcedrv_freq->vce_frep.height != drv_out->dst_height) || 
    		(vcedrv_freq->vce_frep_init == 0) )
    	{
    		vcedrv_freq->vce_frep_init = 1;
    		vcedrv_freq->vce_frep.width = drv_out->dst_width;
    		vcedrv_freq->vce_frep.height = drv_out->dst_height;
    		get_vce_frep(&(vcedrv_freq->vce_frep));
    		//printf("vcedrv_freq->vce_frep.freq:%ld\n",vcedrv_freq->vce_frep.freq);
    		ret = ioctl(drv_in->vcedrv_fd, VCE_SET_FREQ, &vcedrv_freq->vce_frep);
    		if (ret < 0)
    		{
    			printf("Warning!ioctl VCE_SET_FREQ fail! %s,%d!\n", __FILE__, __LINE__);
    		}
    	}
    }
}