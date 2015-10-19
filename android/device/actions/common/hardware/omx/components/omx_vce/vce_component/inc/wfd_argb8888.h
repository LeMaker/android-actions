#ifndef  __WFD_ARGB8888_H__
#define __WFD_ARGB8888_H__

#include "frame_mng.h"
#include "omx_comp_debug_levels.h"
#include "vce_cfg.h"
#include "log.h"

static int wfd_argb8888_init(mng_internal_t *mng_info)
{
	unsigned long phy = 0;
	unsigned long vir = 0;
	unsigned int len;
	enc_frame_t* enc_frame;
	wfd_intput_t* wfd;

	wfd = &(mng_info->wfd_intput);
	enc_frame = mng_info->enc_frame;

	enc_frame->b_bld = 1;
	enc_frame->i_alpha = 255;
	enc_frame->i_bld_fmt = 1; /*ARGB8888*/
	enc_frame->bld_rect.x = 0;
	enc_frame->bld_rect.y = 0;
	/*应该为源大小*/
#ifdef MOD_FOR_UNALIGN_RES
	enc_frame->bld_rect.w = mng_info->i_source_width;
	enc_frame->bld_rect.h = mng_info->i_source_height;
#else
	enc_frame->bld_rect.w = mng_info->ndst_width;
	enc_frame->bld_rect.h = mng_info->ndst_height;
#endif

#ifdef  IC_TYPE_GL5203
	enc_frame->bld_rect.stride = mng_info->i_source_stride;
	enc_frame->bld_rect.dstw = mng_info->ndst_width;
	enc_frame->bld_rect.dsth = mng_info->ndst_height;
#endif

	/*内存申请*/
	wfd->wfd_w = mng_info->ndst_width / 7;
	wfd->wfd_w = (wfd->wfd_w + 0xf) & (~0xf);
	wfd->wfd_h = mng_info->ndst_height / 7;
	wfd->wfd_h = (wfd->wfd_h + 0xf) & (~0xf);

	/*mng_info->i_video_fmt设定为2,即yuv420*/
	DEBUG(DEB_LEV_ERR, "mng_info->i_video_fmt:%d!\n", mng_info->i_video_fmt);

	len = wfd->wfd_w * wfd->wfd_h * 3 / 2;
	phy = (unsigned long) omx_malloc_phy(len, &vir);
	if (phy == 0)
	{
		DEBUG(DEB_LEV_ERR, "err!scale_phy malloc fail! %s,%d!\n", __FILE__, __LINE__);
		return -1;
	}

	memset((void *) vir, 0, wfd->wfd_w * wfd->wfd_h); //Y:0
	memset((void *) (vir + wfd->wfd_w * wfd->wfd_h), 0x80, wfd->wfd_w * wfd->wfd_h / 2); //UV:128
	wfd->wfd_phy[0] = phy;
	wfd->wfd_phy[1] = wfd->wfd_phy[0] + wfd->wfd_w * wfd->wfd_h;
	wfd->wfd_phy[2] = wfd->wfd_phy[1] + wfd->wfd_w * wfd->wfd_h / 4;

	wfd->wfd_vir[0] = vir;
	wfd->wfd_vir[1] = wfd->wfd_vir[0] + wfd->wfd_w * wfd->wfd_h;
	wfd->wfd_vir[2] = wfd->wfd_vir[1] + wfd->wfd_w * wfd->wfd_h / 4;

	return 0;
}

static int wfd_sizes_check(int srcw, int srch, int dstw, int dsth)
{
	double scalew = ((double) dstw) / srcw;
	double scaleh = ((double) dsth) / srch;
	//printf("srcw:%d,srch:%d,dstw:%d,dsth:%d,scalew:%f,scaleh:%f!\n",srcw,srch,dstw,dsth,scalew,scaleh);

#ifdef IC_TYPE_GL5207
	/*错误判断*/
	if ((scalew < 0.5) || (scaleh < 17.0/32) || (scalew > 8) || (scaleh > 8))
	{
		DEBUG(DEB_LEV_ERR, "err!cannot suport this scale! %d,%d,%d,%d\n", dstw, srcw, dsth, srch);
		return -1;
	}
#else 
#ifdef IC_TYPE_GL5203
	/*错误判断*/
	if ((scalew < 0.5) || (scaleh < 0.5) || (scalew > 4) || (scaleh > 4))
	{
		DEBUG(DEB_LEV_ERR, "err!cannot suport this scale! %d,%d,%d,%d\n", dstw, srcw, dsth, srch);
		return -1;
	}
#endif
#endif

	return 0;
}

static void wfd_argb8888_set(mng_internal_t *mng_info, void* blend_intput)
{
	enc_frame_t* enc_frame;
	wfd_intput_t* wfd;

	wfd = &(mng_info->wfd_intput);
	enc_frame = mng_info->enc_frame;

	enc_frame->b_bld = 1;

#ifdef MOD_FOR_UNALIGN_RES
	enc_frame->src_stride = wfd->wfd_w;
#endif
	enc_frame->width = wfd->wfd_w;
	enc_frame->height = wfd->wfd_h;

	//DEBUG(DEB_LEV_ERR,"enc_frame->src_stride:%d,enc_frame->width:%d,enc_frame->height:%d\n",enc_frame->src_stride,enc_frame->width,enc_frame->height);

	/*物理地址*/
	enc_frame->src_planar_addr[0] = (void *) wfd->wfd_phy[0];
	enc_frame->src_planar_addr[1] = (void *) wfd->wfd_phy[1];
	enc_frame->src_planar_addr[2] = (void *) wfd->wfd_phy[2];

	/*虚拟地址*/
	enc_frame->src_planar[0] = (unsigned char *) wfd->wfd_vir[0];
	enc_frame->src_planar[1] = (unsigned char *) wfd->wfd_vir[1];
	enc_frame->src_planar[2] = (unsigned char *) wfd->wfd_vir[2];

	/*blend输入，物理地址*/
	enc_frame->bld_planar_addr[0] = blend_intput;
	enc_frame->bld_planar_addr[1] = 0;
}

static void wfd_argb8888_deinit(mng_internal_t *mng_info)
{
	wfd_intput_t* wfd;
	enc_frame_t* enc_frame;

	wfd = &(mng_info->wfd_intput);
	enc_frame = mng_info->enc_frame;
	enc_frame->b_bld = 0; /*必须清0,因为可能disable ports，改为yuv420输入，然后再enable ports*/

	if (wfd->wfd_phy[0] != 0)
	{
		omx_free_phy((void*) (wfd->wfd_phy[0]));
		wfd->wfd_phy[0] = 0;
		wfd->wfd_phy[1] = 0;
		wfd->wfd_phy[2] = 0;
		wfd->wfd_vir[0] = 0;
		wfd->wfd_vir[1] = 0;
		wfd->wfd_vir[2] = 0;
	}
}

//#include <time.h>
//static struct timeval tv0,tv1;
static void wfd_argb888_media_format_convert(unsigned char* src,int stride,int width,int height,unsigned char* tmpbuffer)
{
	int i,j;
	int argbs,rs,gs,bs;
	int argbd,rd,gd,bd;
	unsigned int *psrc = (unsigned int*)src;
	unsigned int *pdst = (unsigned int*)src;
	int prev_argbs = 0;
	int prev_argbd = 0;
	
	//int cnts = 0;
	//DEBUG(DEB_LEV_ERR, "To_BT601!%p,%d,%d,%d\n",src,stride,width,height);
	//gettimeofday(&tv0,NULL);
	
	/*若源buffer为cache类型，则不需copy，否则需copy*/
	if(tmpbuffer != NULL)
	{
		memcpy(tmpbuffer,src,stride*height*4);
		psrc =  (unsigned int*)tmpbuffer;
	}
	else
	{
		DEBUG(DEB_LEV_ERR, "Warning!tmpbuffer is NULL!%d\n",__LINE__);
	}
	
	/*第一个点*/
	prev_argbs = psrc[0];
	rs = (prev_argbs >> 16) & 0xff;
	gs = (prev_argbs >> 8) & 0xff;
	bs = (prev_argbs >> 0) & 0xff;
#ifdef IC_TYPE_GL5207
	rd = ((894* rs - 13*gs - 2*bs) >> 10) + 16;
	gd = ((-6*rs + 888*gs + 2*bs) >> 10) + 16;
	bd = ((-6*rs -12*gs+897*bs) >> 10) + 16;
#else
	rd = ((894* rs - 13*gs - 2*bs) >> 10) + 19;
	gd = ((-6*rs + 888*gs + 2*bs) >> 10) + 19;
	bd = ((-6*rs -12*gs+897*bs) >> 10) + 19;
#endif
	prev_argbd = (rd << 16) | (gd << 8) | (bd);
	
	/*其余点*/
	for (i = 0;i < height;i++)
	{
		for (j = 0; j < width;j++ )
		{
			argbs = psrc[i*stride + j];

			if(argbs == prev_argbs)
			{
				pdst[i*stride + j] = prev_argbd;
				//cnts++;
			}
			else
			{
				rs = (argbs >> 16) & 0xff;
				gs = (argbs >> 8) & 0xff;
				bs = (argbs >> 0) & 0xff;

#ifdef IC_TYPE_GL5207
				rd = ((894* rs - 13*gs - 2*bs) >> 10) + 16;
				gd = ((-6*rs + 888*gs + 2*bs) >> 10) + 16;
				bd = ((-6*rs -12*gs+897*bs) >> 10) + 16;
#else
				rd = ((894* rs - 13*gs - 2*bs) >> 10) + 19;
				gd = ((-6*rs + 888*gs + 2*bs) >> 10) + 19;
				bd = ((-6*rs -12*gs+897*bs) >> 10) + 19;
#endif

				/*不会超出范围*/
				//if(rd > 255) rd = 255;
				//if(rd < 0) rd = 0;
				//if(gd > 255) gd = 255;
				//if(gd < 0) gd = 0;
				//if(bd > 255) bd = 255;
				//if(bd < 0) bd = 0;

				argbd = (rd << 16) | (gd << 8) | (bd);
				pdst[i*stride + j] = argbd;
			
				/*保存*/
				prev_argbs = argbs;
				prev_argbd = argbd;
			}
		}
	}
	
	//DEBUG(DEB_LEV_ERR,"cnts:%f!\n", (float)cnts / height / width );
	//gettimeofday(&tv1,NULL);
	//DEBUG(DEB_LEV_ERR,"To_BT601!argb8888,tv_times:%ld us!\n",(tv1.tv_sec-tv0.tv_sec)*1000000 + (tv1.tv_usec-tv0.tv_usec) );
}

#endif
