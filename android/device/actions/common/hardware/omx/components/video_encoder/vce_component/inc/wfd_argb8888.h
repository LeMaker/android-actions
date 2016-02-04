#ifndef __WFD_ARGB8888_H__
#define __WFD_ARGB8888_H__

#include "frame_mng.h"
#include "omx_comp_debug_levels.h"
#include "vce_cfg.h"
#include "log.h"

static int wfd_sizes_check(int srcw, int srch, int dstw, int dsth)
{
	double scalew = ((double) dstw) / srcw;
	double scaleh = ((double) dsth) / srch;
	//printf("srcw:%d,srch:%d,dstw:%d,dsth:%d,scalew:%f,scaleh:%f!\n",srcw,srch,dstw,dsth,scalew,scaleh);

	/* check scale */
	if ((scalew < 0.5) || (scaleh < 17.0/32) || (scalew > 8) || (scaleh > 8))
	{
		DEBUG(DEB_LEV_ERR, "err!cannot suport this scale! %d,%d,%d,%d\n", dstw, srcw, dsth, srch);
		return -1;
	}

	return 0;
}

static void wfd_argb888_media_format_convert(unsigned char *src, int stride, int width, int height, unsigned char *tmpbuffer)
{
	int i,j;
	int argbs,rs,gs,bs;
	int argbd,rd,gd,bd;
	unsigned int *psrc = (unsigned int *)src;
	unsigned int *pdst = (unsigned int *)src;
	int prev_argbs = 0;
	int prev_argbd = 0;
	
	if(tmpbuffer != NULL)
	{
		memcpy(tmpbuffer, src, stride*height*4);
		psrc =  (unsigned int *)tmpbuffer;
	}
	else
	{
		DEBUG(DEB_LEV_ERR, "Warning!tmpbuffer is NULL!%d\n",__LINE__);
	}
	
	prev_argbs = psrc[0];
	rs = (prev_argbs >> 16) & 0xff;
	gs = (prev_argbs >> 8) & 0xff;
	bs = (prev_argbs >> 0) & 0xff;

	rd = ((894*rs - 13*gs - 2*bs) >> 10) + 16;
	gd = ((-6*rs + 888*gs + 2*bs) >> 10) + 16;
	bd = ((-6*rs - 12*gs + 897*bs) >> 10) + 16;

	prev_argbd = (rd << 16) | (gd << 8) | (bd);
	
	for (i = 0;i < height;i++)
	{
		for (j = 0; j < width;j++ )
		{
			argbs = psrc[i*stride + j];

			if(argbs == prev_argbs)
			{
				pdst[i*stride + j] = prev_argbd;
			}
			else
			{
				rs = (argbs >> 16) & 0xff;
				gs = (argbs >> 8) & 0xff;
				bs = (argbs >> 0) & 0xff;

				rd = ((894* rs - 13*gs - 2*bs) >> 10) + 16;
				gd = ((-6*rs + 888*gs + 2*bs) >> 10) + 16;
				bd = ((-6*rs - 12*gs + 897*bs) >> 10) + 16;

				//if(rd > 255) rd = 255;
				//if(rd < 0) rd = 0;
				//if(gd > 255) gd = 255;
				//if(gd < 0) gd = 0;
				//if(bd > 255) bd = 255;
				//if(bd < 0) bd = 0;

				argbd = (rd << 16) | (gd << 8) | (bd);
				pdst[i*stride + j] = argbd;

				prev_argbs = argbs;
				prev_argbd = argbd;
			}
		}
	}
}

#endif
