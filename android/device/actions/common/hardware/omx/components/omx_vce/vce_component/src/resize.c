#include <sys/ioctl.h>
#include "omx_comp_debug_levels.h"
#include "resize.h"
#include "vce_cfg.h"
#include "log.h"

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

static int resize_get_ratios(int mdstw,int mdsth,int *srcw,int *srch,unsigned int *bx,unsigned int *by,unsigned int *cv)
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
#ifdef MOD_FOR_UNALIGN_RES
	msrcw = ALIGN_SRC_WIDTH(msrcw);
	msrch = ALIGN_SRC_HEIGHT(msrch);
#else
	msrcw = msrcw & (~0xf);
	msrch = msrch & (~0xf);
#endif

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

static void resize_set_reg(vce_input_t* vce_in, resize_input_t* resize_in, resize_output_t* resize_out)
{
	memset(vce_in, 0, sizeof(vce_input_t));

	int srcstride = resize_in->srcstride;
	int mSrcw = resize_in->srcw;
	int mSrch = resize_in->srch;
	int mDstw = resize_in->dstw;
	int mDsth = resize_in->dsth;
	int issemi = resize_in->issemi;
	int isyuv420 = resize_in->isyuv420;

	unsigned long mSrcY = resize_in->srcy;
	unsigned long mSrcCb = resize_in->srccb;
	unsigned long mSrcCr = resize_in->srccr;
	unsigned long strm_addr = resize_out->inter_phyaddr;
	unsigned int strm_len = resize_out->inter_len;

	/*PreView  mode*/
	vce_in->vce_status = 0;

	/*Vce CFG*/
	unsigned int vce_cfg = 0;
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
	unsigned int bx, by, cv;
	resize_get_ratios(mDstw, mDsth, &mSrcw, &mSrch, &bx, &by, &cv);

	vce_in->ups_rath = bx;
	vce_in->ups_ratv = by;

	/*上采样等控制*/
	unsigned int ups_ctl = 1;//5202 only support data from ddr
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
}

static int resize_one_time(resize_input_t* resize_in, resize_output_t*resize_out, int saclemode, int dir)
{
	if (saclemode == SCALE_TWO)
	{
		if (dir == x_dir)
		{
			resize_in->dstw = resize_in->srcw * 2;
			resize_in->dsth = resize_in->srch;
		}
		else
		{
			resize_in->dstw = resize_in->srcw;
			resize_in->dsth = resize_in->srch * 2;
		}
	}
	else
	{
		resize_in->dstw = resize_in->srcw * _VCE_SCALE_;
		resize_in->dsth = resize_in->srch * _VCE_SCALE_;
	}

	resize_out->interw = resize_in->dstw;
	resize_out->interh = resize_in->dsth;
	if (resize_in->isyuv420)
		resize_out->inter_len = resize_out->interw * resize_out->interh * 3 / 2;
	else
		resize_out->inter_len = resize_out->interw * resize_out->interh * 2;

	int vce_fd = resize_in->vce_fd;
	vce_input_t vce_in;
	vce_output_t vce_out;
	int ret = 0;

	resize_set_reg(&vce_in, resize_in, resize_out);
	//print_vce_input_t(&vce_in);
	//printf("vce_fd:%x\n",vce_fd);

	int try_i = 0;
	do
	{
		ret = ioctl(vce_fd, VCE_CMD_ENC_RUN, &vce_in);

		if ((ret == 0) || (try_i++ > 200))
			break;
		usleep(1000);
	} while (1);

	if (ret < 0)
	{
		DEBUG(DEB_LEV_ERR, "err!scale VCE_CMD_ENC_RUN fail! %s,%d!\n", __FILE__, __LINE__);
		return -1;
	}

	ret = ioctl(vce_fd, VCE_CMD_QUERY_FINISH, &vce_out);
	if (ret < 0)
	{
		DEBUG(DEB_LEV_ERR, "err!scale VCE_CMD_QUERY_FINISH fail! %s,%d!\n", __FILE__, __LINE__);
		return -1;
	}
	//print_vce_output_t(&vce_out);

	return ret;
}

int resize_check(int srcw, int srch, int dstw, int dsth, int* s_diff, int* s_dir, int* nead8scale)
{
	double scalew = ((double) dstw) / srcw;
	double scaleh = ((double) dsth) / srch;

	//printf("srcw:%d,srch:%d,dstw:%d,dsth:%d,scalew:%f,scaleh:%f!\n",srcw,srch,dstw,dsth,scalew,scaleh);

#ifndef IC_TYPE_GL5207
	/*错误判断*/
	if ((scalew < 1) || (scaleh < 1) || (scalew >= 32) || (scaleh >= 32))
	{
		DEBUG(DEB_LEV_ERR, "err!cannot suport this scale! %d,%d,%d,%d\n", dstw, srcw, dsth, srch);
		return -1;
	}
#else
	/*错误判断*/
	if ((scalew < 0.5) || (scaleh < 0.5) || (scalew > 8) || (scaleh > 8))
	{
		DEBUG(DEB_LEV_ERR, "err!cannot suport this scale! %d,%d,%d,%d\n", dstw, srcw, dsth, srch);
		return -1;
	}
	else
	{
		*nead8scale = 0;
		*s_diff = 0;
		return 0;
	}
#endif

	int s_w = 0;
	int s_h = 0;

	double rw = scalew;

	while (rw >= 2)
	{
		rw = rw / 2;
		s_w++;
	}

	double rh = scaleh;
	while (rh >= 2)
	{
		rh = rh / 2;
		s_h++;
	}

	double sw;
	double sh;

	if (s_w == s_h)
	{
		*s_diff = 0;
		*s_dir = x_dir;
		sw = scalew;
		sh = scaleh;
	}
	else if (s_w > s_h)
	{
		if (rw == 1)
		{
			s_w--;
			rw *= 2;
		}

		*s_diff = s_w - s_h;
		*s_dir = x_dir;
		sw = (1 << (s_w - *s_diff)) * rw;
		sh = scaleh;
	}
	else
	{
		if (rh == 1)
		{
			s_h--;
			rh *= 2;
		}

		*s_diff = s_h - s_w;
		*s_dir = y_dir;
		sw = scalew;
		sh = (1 << (s_h - *s_diff)) * rh;
	}

	if (MIN(sw, sh) >= 8)
	{
		*nead8scale = 1;
	}
	else
	{
		*nead8scale = 0;
	}

	if ((*nead8scale != 0) || (*s_diff != 0))
	{
		DEBUG(DEB_LEV_ERR,"srcw:%d,srch:%d,dstw:%d,dsth:%d,scalew:%f,scaleh:%f!\n",srcw,srch,dstw,dsth,scalew,scaleh);
	}

	return 0;
}

void resize_input_init(resize_input_t* resize_in, mng_internal_t *mng_info)
{
	enc_frame_t* enc_frame = mng_info->enc_frame;

	resize_in->video_phyaddr = (unsigned long) (enc_frame->src_planar_addr[0]);
	resize_in->video_viraddr = (unsigned long) (enc_frame->src_planar[0]);
	resize_in->srcy = (unsigned long) (enc_frame->src_planar_addr[0]);
	resize_in->srccb = (unsigned long) (enc_frame->src_planar_addr[1]);
	resize_in->srccr = (unsigned long) (enc_frame->src_planar_addr[2]);
	resize_in->srcstride = enc_frame->src_stride;
	resize_in->srcw = enc_frame->width;
	resize_in->srch = enc_frame->height;
	resize_in->dstw = mng_info->ndst_width;
	resize_in->dsth = mng_info->ndst_height;
	resize_in->issemi = mng_info->b_semi;
	if (mng_info->i_video_fmt == ENC_YUV422P)
		resize_in->isyuv420 = 0;
	else
		resize_in->isyuv420 = 1;
}

int resize_start(resize_input_t* resize_in,resize_output_t*resize_out,resize_buffer_t* resize_buf,int s_diff,int s_dir,int nead8scale)
{
	int ret = 0;
	int srcw = resize_in->srcw;
	int srch = resize_in->srch;
	int dstw = resize_in->dstw;
	int dsth = resize_in->dsth;
	int isyuv420 = resize_in->isyuv420;

	/*不需要中间任何处理*/
	if ((s_diff == 0) && (nead8scale == 0))
		return 0;

	/*计算需要申请空间大小*/
	int Inter_w;
	int Inter_h;

	if (s_dir)
	{
		Inter_w = srcw * (1 << s_diff);
		Inter_h = srch;
	}
	else
	{
		Inter_w = srcw;
		Inter_h = srch * (1 << s_diff);
	}

	if (nead8scale)
	{
		Inter_w = Inter_w * _VCE_SCALE_;
		Inter_h = Inter_h * _VCE_SCALE_;
	}

	/*申请空间*/
	unsigned long input_phy = 0;
	unsigned long input_vir = 0;
	unsigned long output_phy = 0;
	unsigned long output_vir = 0;
	unsigned int now_len = 0;

	if (isyuv420)
	{
		now_len = (Inter_w * Inter_h * 3 / 2) * 2;
	}
	else
	{
		now_len = (Inter_w * Inter_h * 2) * 2;
	}

	//printf("b4 scale_phy:%x,scale_vir:%x\n",resize_buf->scale_phy,resize_buf->scale_vir);

	if (resize_buf->scale_phy == 0)
	{
		resize_buf->scale_phy = (unsigned long) omx_malloc_phy(now_len, &resize_buf->scale_vir);
		resize_buf->last_len = now_len;
	}
	else if (resize_buf->last_len < now_len)
	{
		omx_free_phy((void*) (resize_buf->scale_phy));
		resize_buf->scale_phy = 0;
		resize_buf->scale_vir = 0;
		resize_buf->scale_phy = (unsigned long) omx_malloc_phy(now_len, &resize_buf->scale_vir);
		resize_buf->last_len = now_len;
	}

	//printf("aft scale_phy:%x,scale_vir:%x\n",resize_buf->scale_phy,resize_buf->scale_vir);

	input_phy = resize_buf->scale_phy;
	input_vir = resize_buf->scale_vir;
	output_phy = resize_buf->scale_phy + (now_len / 2);
	output_vir = resize_buf->scale_vir + (now_len / 2);

	if (resize_buf->scale_phy == 0)
	{
		DEBUG(DEB_LEV_ERR, "err!scale_phy malloc fail! %s,%d!\n", __FILE__, __LINE__);
		return -1;
	}

	/*开始缩放*/
	int i;
	unsigned long swap_t;

	resize_out->inter_phyaddr = output_phy;
	resize_out->inter_viraddr = output_vir;

	if (s_diff)
	{
		ret = resize_one_time(resize_in, resize_out, SCALE_TWO, s_dir);
		if (ret < 0)
		{
			DEBUG(DEB_LEV_ERR, "err!scale encoder fail! %s,%d!\n", __FILE__, __LINE__);
			goto out;
		}

		if (s_diff > 1)
		{
			for (i = 1; i < s_diff; i++)
			{
				if (s_dir == x_dir)
				{
					resize_in->srcw = resize_in->srcw * 2;
				}
				else
				{
					resize_in->srch = resize_in->srch * 2;
				}
				resize_in->srcstride = resize_in->srcw;

				if (i == 1)
				{
					resize_in->video_phyaddr = resize_out->inter_phyaddr;
					resize_in->video_viraddr = resize_out->inter_viraddr;
					resize_out->inter_phyaddr = input_phy;
					resize_out->inter_viraddr = input_vir;
				}
				else
				{
					SWAP_T(resize_in->video_phyaddr, resize_out->inter_phyaddr, swap_t);
					SWAP_T(resize_in->video_viraddr, resize_out->inter_viraddr, swap_t);
				}

				resize_in->srcy = resize_in->video_phyaddr;
				resize_in->srccb = resize_in->srcy + resize_in->srcw * resize_in->srch;
				if (isyuv420)
					resize_in->srccr = resize_in->srccb + resize_in->srcw * resize_in->srch / 4;
				else
					resize_in->srccr = resize_in->srccb + resize_in->srcw * resize_in->srch / 2;
				resize_in->srccr = resize_in->srccr & (~0x7);

				ret = resize_one_time(resize_in, resize_out, SCALE_TWO, s_dir);
				if (ret < 0)
				{
					DEBUG(DEB_LEV_ERR, "err!scale encoder fail! %s,%d!\n", __FILE__, __LINE__);
					goto out;
				}
			}
		}

		if (nead8scale)
		{
			if (s_dir == x_dir)
			{
				resize_in->srcw = resize_in->srcw * 2;
			}
			else
			{
				resize_in->srch = resize_in->srch * 2;
			}
			resize_in->srcstride = resize_in->srcw;

			if (s_diff == 1)
			{
				resize_in->video_phyaddr = resize_out->inter_phyaddr;
				resize_in->video_viraddr = resize_out->inter_viraddr;
				resize_out->inter_phyaddr = input_phy;
				resize_out->inter_viraddr = input_vir;
			}
			else
			{
				SWAP_T(resize_in->video_phyaddr, resize_out->inter_phyaddr, swap_t);
				SWAP_T(resize_in->video_viraddr, resize_out->inter_viraddr, swap_t);
			}

			resize_in->srcy = resize_in->video_phyaddr;
			resize_in->srccb = resize_in->srcy + resize_in->srcw * resize_in->srch;
			if (isyuv420)
				resize_in->srccr = resize_in->srccb + resize_in->srcw * resize_in->srch / 4;
			else
				resize_in->srccr = resize_in->srccb + resize_in->srcw * resize_in->srch / 2;
			resize_in->srccr = resize_in->srccr & (~0x7);

			ret = resize_one_time(resize_in, resize_out, SCALE_EIGHT, s_dir);
			if (ret < 0)
			{
				DEBUG(DEB_LEV_ERR, "err!scale encoder fail! %s,%d!\n", __FILE__, __LINE__);
				goto out;
			}
		}
	}
	else
	{
		ret = resize_one_time(resize_in, resize_out, SCALE_EIGHT, s_dir);
		if (ret < 0)
		{
			DEBUG(DEB_LEV_ERR, "err!scale encoder fail! %s,%d!\n", __FILE__, __LINE__);
			goto out;
		}
	}

	return 0;

out:
	if(resize_buf->scale_phy != 0)
	{
		omx_free_phy((void*) (resize_buf->scale_phy));
		resize_buf->scale_phy = 0;
		resize_buf->scale_vir = 0;
	}

	return ret;
}

void resize_get_output(resize_output_t*resize_out, mng_internal_t *mng_info)
{
	enc_frame_t* enc_frame = mng_info->enc_frame;

	enc_frame->i_semi = 1;
	enc_frame->src_stride = resize_out->interw;
	enc_frame->width = resize_out->interw;
	enc_frame->height = resize_out->interh;
	//printf("resize_out->interw:%d,resize_out->interh:%d\n",resize_out->interw,resize_out->interh);

	enc_frame->src_planar_addr[0] = (void *) (resize_out->inter_phyaddr);
	enc_frame->src_planar[0] = (unsigned char *) (resize_out->inter_viraddr);

	enc_frame->src_planar_addr[1] = (void *)( (unsigned char *)(enc_frame->src_planar_addr[0]) +  enc_frame->width *  enc_frame->height);
	enc_frame->src_planar[1] = enc_frame->src_planar[0] + enc_frame->width * enc_frame->height;

	if (mng_info->i_video_fmt == ENC_YUV422P)
	{
		enc_frame->src_planar_addr[2] = (void *)( (unsigned char *)(enc_frame->src_planar_addr[1]) +  enc_frame->width *  enc_frame->height / 2);
		enc_frame->src_planar[2] = enc_frame->src_planar[1] + enc_frame->width * enc_frame->height / 2;
	}
	else
	{
		enc_frame->src_planar_addr[2] = (void *)( (unsigned char *)(enc_frame->src_planar_addr[1]) +  enc_frame->width *  enc_frame->height / 4);
		enc_frame->src_planar[2] = enc_frame->src_planar[1] + enc_frame->width * enc_frame->height / 4;
	}
}

void resize_recover(mng_internal_t *mng_info)
{
	enc_frame_t* enc_frame = mng_info->enc_frame;

#ifdef MOD_FOR_UNALIGN_RES
	if(mng_info->b_store_in_video[0] == OMX_FALSE)
	{
		enc_frame->height = mng_info->rect_h;
		enc_frame->width = mng_info->rect_w;
	}
	enc_frame->i_semi = mng_info->b_semi;
	enc_frame->src_stride = mng_info->i_source_stride;
#else
	if (mng_info->b_store_in_video[0] == OMX_FALSE)
	{
		enc_frame->height = (mng_info->rect_h + 15) & (~0xf);
		enc_frame->width = (mng_info->rect_w + 15) & (~0xf);
	}

	enc_frame->i_semi = mng_info->b_semi;
	enc_frame->src_stride = mng_info->i_source_width;
#endif
}

void resize_free_buf(resize_buffer_t* resize_buf)
{
	if (resize_buf->scale_phy != 0)
	{
		omx_free_phy((void*) (resize_buf->scale_phy));
		resize_buf->scale_phy = 0;
		resize_buf->scale_vir = 0;
	}
}