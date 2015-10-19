#include <OMX_Core.h>
#include <OMX_Component.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <asm/unistd.h>
#include "tsemaphore.h"
#include "queue.h"
#include "omx_comp_debug_levels.h"
#include "extension_struct.h"
#include "frame_mng.h"
#include "face_engine.h"
#include "resize.h"
#include "vce_act_ext.h"
#include "video_mediadata.h"
#include "omx_malloc.h"
#include <dlfcn.h>   /* For dynamic loading */
#include "log.h"

#if Enable_Fix_ARGB8888
#include "wfd_argb8888.h"
#endif


int mng_open_count = 0;

/*-------vcehal库------*/
static void* pthandle = NULL;
void * (*vce_enc_open)();
int (*vce_enc_init)(void *, enc_param_t *);
int (*vce_enc_frame)(void *, enc_frame_t *, enc_stream_t *);
int (*vce_enc_out)(void *, enc_stream_t *);
int (*vce_enc_close)(void *);
int (*vce_enc_deinit)(void *);
int (*vce_enc_cmd)(void *, unsigned int, unsigned long int);

int vce_so_open()
{
	pthandle = (void*) dlopen("libACT_EncAPI.so", RTLD_LAZY | RTLD_GLOBAL);
	if (pthandle == NULL)
	{
		DEBUG(DEB_LEV_ERR, "open libACT_EncAPI.so  error!!\n");
		return -1;
	}

	vce_enc_open = dlsym(pthandle, "vce_enc_open");
	if (vce_enc_open == NULL)
	{
		DEBUG(DEB_LEV_ERR, "vce_enc_open err\n");
		dlclose(pthandle);
		pthandle = NULL;
		return -1;
	}

	vce_enc_init = dlsym(pthandle, "vce_enc_init");
	if (vce_enc_init == NULL)
	{
		DEBUG(DEB_LEV_ERR, "vce_enc_init err\n");
		dlclose(pthandle);
		pthandle = NULL;
		return -1;
	}

	vce_enc_frame = dlsym(pthandle, "vce_enc_frame");
	if (vce_enc_frame == NULL)
	{
		DEBUG(DEB_LEV_ERR, "vce_enc_frame err\n");
		dlclose(pthandle);
		pthandle = NULL;
		return -1;
	}

	vce_enc_out = dlsym(pthandle, "vce_enc_out");
	if (vce_enc_out == NULL)
	{
		DEBUG(DEB_LEV_ERR, "vce_enc_out err\n");
		dlclose(pthandle);
		pthandle = NULL;
		return -1;
	}

	vce_enc_close = dlsym(pthandle, "vce_enc_close");
	if (vce_enc_close == NULL)
	{
		DEBUG(DEB_LEV_ERR, "vce_enc_close err\n");
		dlclose(pthandle);
		pthandle = NULL;
		return -1;
	}

	vce_enc_deinit = dlsym(pthandle, "vce_enc_deinit");
	if (vce_enc_deinit == NULL)
	{
		DEBUG(DEB_LEV_ERR, "vce_enc_deinit err\n");
		dlclose(pthandle);
		pthandle = NULL;
		return -1;
	}

	vce_enc_cmd = dlsym(pthandle, "vce_enc_cmd");
	if (vce_enc_cmd == NULL)
	{
		DEBUG(DEB_LEV_ERR, "vce_enc_cmd err\n");
		dlclose(pthandle);
		pthandle = NULL;
		return -1;
	}

	return 0;
}

void vce_so_close()
{
	if (pthandle)
	{
		dlclose(pthandle);
		pthandle = NULL;
	}
}

/*-------facedet库------*/
static void* face_handle = NULL;
void * (*face_app_init)(ALFace_app_init_t *);
int (*face_app_run)(void *, unsigned int, unsigned long);
int (*face_mask_run)(void *, unsigned int, unsigned long);
int (*face_app_dispose)(void *);

int face_so_open()
{
	face_handle = (void*) dlopen("libACT_FD.so", RTLD_LAZY | RTLD_GLOBAL);
	if (face_handle == NULL)
	{
		DEBUG(DEB_LEV_ERR, "libACT_FD.so  error!!\n");
		return -1;
	}

	face_app_init = dlsym(face_handle, "face_app_init");
	if (face_app_init == NULL)
	{
		DEBUG(DEB_LEV_ERR, "face_app_init err\n");
		dlclose(face_handle);
		face_handle = NULL;
		return -1;
	}

	face_app_run = dlsym(face_handle, "face_app_run");
	if (face_app_run == NULL)
	{
		DEBUG(DEB_LEV_ERR, "face_app_run err\n");
		dlclose(face_handle);
		face_handle = NULL;
		return -1;
	}

	face_mask_run = dlsym(face_handle, "face_mask_run");
	if (face_mask_run == NULL)
	{
		DEBUG(DEB_LEV_ERR, "face_mask_run err\n");
		dlclose(face_handle);
		face_handle = NULL;
		return -1;
	}

	face_app_dispose = dlsym(face_handle, "face_app_dispose");
	if (face_app_dispose == NULL)
	{
		DEBUG(DEB_LEV_ERR, "face_app_dispose err\n");
		dlclose(face_handle);
		face_handle = NULL;
		return -1;
	}

	return 0;
}

void face_so_close()
{
	if (face_handle)
	{
		dlclose(face_handle);
		face_handle = NULL;
	}
}

int mng_open(mng_internal_t *mng_info)
{
	int ret = 0;
	memset(mng_info, 0, sizeof(mng_internal_t));

	mng_info->enc_frame = calloc(1, sizeof(enc_frame_t));
	if (mng_info->enc_frame == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!calloc fail!%s,%d\n",__FILE__,__LINE__ );
		goto err0;
	}
	mng_info->enc_strm = calloc(1, sizeof(enc_stream_t));
	if (mng_info->enc_strm == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!calloc fail!%s,%d\n",__FILE__,__LINE__ );
		goto err0;
	}
  /*清0*/
	memset(mng_info->enc_frame, 0, sizeof(enc_frame_t));
	memset(mng_info->enc_strm, 0, sizeof(enc_stream_t));
	
	mng_info->queue_enc = calloc(1, sizeof(queue_t));
	if (mng_info->queue_enc == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!calloc fail!%s,%d\n",__FILE__,__LINE__ );
		goto err0;
	}
	mng_info->queue_strm = calloc(1, sizeof(queue_t));
	if (mng_info->queue_strm == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!calloc fail!%s,%d\n",__FILE__,__LINE__ );
		goto err0;
	}
	mng_info->queue_prepare_video = calloc(1, sizeof(queue_t));
	if (mng_info->queue_prepare_video == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!calloc fail!%s,%d\n",__FILE__,__LINE__ );
		goto err0;
	}
#ifdef FIXED_CTS_4_4
	mng_info->queue_strm_return = calloc(1, sizeof(queue_t));
	if (mng_info->queue_strm_return == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!calloc fail!%s,%d\n",__FILE__,__LINE__ );
		goto err0;
	}
#endif

	mng_info->i_last_type = SLICE_TYPE_I;
	mng_info->bmvc = 0;
	mng_info->i_view_id = 0;
	mng_info->frame_rate = 1;
	mng_info->maxIinterval = 12;
	mng_info->bIDR = 1;

	if (mng_open_count == 0)
	{
		ret = vce_so_open();
		if (ret == -1)
		{
			DEBUG(DEB_LEV_ERR, "err!vce_so_open fail!%s,%d\n",__FILE__,__LINE__ );
			goto err0;
		}
	}

	mng_info->vce_handle = vce_enc_open();
	if (mng_info->vce_handle == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!vce_enc_open fail!%s,%d\n",__FILE__,__LINE__ );
		goto err1;
	}

	/*facedet*/
	mng_info->face_eng = NULL;
	mng_info->mFaceAppout.faces = NULL;

	mng_open_count++;

	pthread_mutex_init(&mng_info->filter_mutex, NULL);
	return 0;

err1:
	vce_so_close();

err0:
	if(mng_info->queue_enc)
	{
		//queue_deinit(mng_info->queue_enc);
		free(mng_info->queue_enc);
		mng_info->queue_enc = NULL;
	}

	if (mng_info->queue_strm)
	{
		//queue_deinit(mng_info->queue_strm);
		free(mng_info->queue_strm);
		mng_info->queue_strm = NULL;
	}

	if (mng_info->queue_prepare_video)
	{
		//queue_deinit(mng_info->queue_prepare_video);
		free(mng_info->queue_prepare_video);
		mng_info->queue_prepare_video = NULL;
	}

#ifdef FIXED_CTS_4_4
	if (mng_info->queue_strm_return)
	{
		free(mng_info->queue_strm_return);
		mng_info->queue_strm_return = NULL;
	}
#endif	

	if (mng_info->enc_frame)
	{
		free(mng_info->enc_frame);
		mng_info->enc_frame = NULL;
	}

	if (mng_info->enc_strm)
	{
		free(mng_info->enc_strm);
		mng_info->enc_strm = NULL;
	}

	return -1;
}

int mng_init(mng_internal_t *mng_info, void *enc_param)
{
	int ret = 0;
	pthread_mutex_lock(&mng_info->filter_mutex);

	enc_param_t* penc_param = (enc_param_t*) enc_param;
	enc_frame_t *enc_frame = mng_info->enc_frame;

	/*mng_info其他成员*/
	mng_info->bdownscale = penc_param->prp_param->b_downscale;
	mng_info->maxIinterval = penc_param->h264_param->kIntraPeroid;  /*更新*/
	mng_info->bmvc = penc_param->h264_param->b_mvc;                         /*更新*/
	mng_info->encFrm = 0;
	mng_info->cntFrm = 0;
	mng_info->i_last_type = SLICE_TYPE_I;
	mng_info->bexit = 0;
	mng_info->i_view_id = 0;
	mng_info->bIDR = 1;
	mng_info->b_encoded = 0;
	mng_info->rect_x = 0;
	mng_info->rect_y = 0;
	mng_info->rect_h = mng_info->i_source_height;
	mng_info->rect_w = mng_info->i_source_width;

#ifdef MOD_FOR_UNALIGN_RES
	if(penc_param->enc_codec == ENC_H264)
	{
		mng_info->ndst_width = ALIGN_DST_WIDTH(penc_param->h264_param->i_pic_width);
		mng_info->ndst_height = ALIGN_DST_HEIGHT(penc_param->h264_param->i_pic_height);
	}
	else if(penc_param->enc_codec == ENC_JPEG)
	{
		mng_info->ndst_width = ALIGN_DST_WIDTH(penc_param->jpg_param->i_pic_width);
		mng_info->ndst_height = ALIGN_DST_HEIGHT(penc_param->jpg_param->i_pic_height);
	}
	else
	{
		mng_info->ndst_width = ALIGN_DST_WIDTH(penc_param->prp_param->d_width);
		mng_info->ndst_height = ALIGN_DST_HEIGHT(penc_param->prp_param->d_height);
	}
#else
	if (penc_param->enc_codec == ENC_H264)
	{
		mng_info->ndst_width = penc_param->h264_param->i_pic_width;
		mng_info->ndst_height = penc_param->h264_param->i_pic_height;
	}
	else if (penc_param->enc_codec == ENC_JPEG)
	{
		mng_info->ndst_width = penc_param->jpg_param->i_pic_width;
		mng_info->ndst_height = penc_param->jpg_param->i_pic_height;
	}
	else
	{
		mng_info->ndst_width = penc_param->prp_param->d_width;
		mng_info->ndst_height = penc_param->prp_param->d_height;
	}
#endif

	DEBUG(/*DEB_LEV_ERR*/DEB_LEV_PARAMS, "mng_info->ndst_width:%d  mng_info->ndst_height:%d\n", mng_info->ndst_width,
			mng_info->ndst_height);

	mng_info->get_avc_info_ready = 0;

	/*mng_info的enc_frame成员*/
	enc_frame->type = SLICE_TYPE_I;
	enc_frame->view_id = 0;
	enc_frame->mdpb = 0;
	enc_frame->pts = 0;
	enc_frame->frame_cnt = 0;

	if (mng_info->frame_rate <= 0)
		mng_info->frame_rate = 1;
	enc_frame->frmtime = 1000000 / (mng_info->frame_rate);

#if Enable_Fix_SCALE
	enc_frame->i_semi = mng_info->b_semi;
#endif
	enc_frame->width = mng_info->i_source_width;
	enc_frame->height = mng_info->i_source_height;
#ifdef MOD_FOR_UNALIGN_RES
	enc_frame->src_stride = mng_info->i_source_stride;
#else
	enc_frame->src_stride = mng_info->i_source_width;
#endif

#ifdef IC_TYPE_GL5206
    /* gl5206 YUV stride 32 pixel align */
    if((mng_info->is_argb8888 == 0) && (mng_info->i_source_stride % 32 != 0))
    {
        mng_info->stride32_flag = 1;
        mng_info->i_source_stride = ALIGN_32(mng_info->i_source_stride);
        enc_frame->src_stride = mng_info->i_source_stride;
        mng_info->fbuffer_phy = omx_malloc_phy(mng_info->i_source_stride*mng_info->i_source_height*3/2,
            (unsigned long *)&mng_info->fbuffer_vir);
        if(mng_info->fbuffer_phy == NULL)
        {
            DEBUG(DEB_LEV_ERR, "err!omx_malloc_phy fail!%s  %d\n", __FILE__, __LINE__);
		    pthread_mutex_unlock(&mng_info->filter_mutex);
		    return -1;
        }
        
        DEBUG(/*DEB_LEV_ERR*/DEB_LEV_PARAMS,"mng_info->i_source_stride:%d, mng_info->i_source_width:%d\n",
            mng_info->i_source_stride, mng_info->i_source_width);
    }
#endif

	enc_frame->i_ds_lv = penc_param->prp_param->i_downscale_level;
	enc_frame->b_ds = penc_param->prp_param->b_downscale;

	enc_frame->fine_rect.x = 0;
	enc_frame->fine_rect.y = 0;
	enc_frame->fine_rect.w = enc_frame->width;
	enc_frame->fine_rect.h = enc_frame->height;
	enc_frame->fine_rect_en = 0;

	/*facedet的crop区域初始化*/
	mng_info->face_crop.x = 0;
	mng_info->face_crop.y = 0;
	mng_info->face_crop.cropw = mng_info->i_source_width;
	mng_info->face_crop.croph = mng_info->i_source_height;
	mng_info->face_crop.dstw = mng_info->i_source_width;
	mng_info->face_crop.dsth = mng_info->i_source_height;

#if  Enable_Fix_ARGB8888
	if(mng_info->is_argb8888)
	{
#ifndef IC_TYPE_GL5207
		ret = wfd_argb8888_init(mng_info);
		if(ret == -1)
		{
			DEBUG(DEB_LEV_ERR,"err!wfd_rgb888_init fail!%s  %d\n",__FILE__,__LINE__);
			pthread_mutex_unlock(&mng_info->filter_mutex);
			return -1;
		}
#endif
	}

	if(mng_info->is_formatconvert)
	{
		mng_info->formatbuffer = (unsigned char*)malloc(mng_info->i_source_stride*mng_info->i_source_height*4);
		if(mng_info->formatbuffer == NULL) 
		{
			/*允许分配内存失败*/
			DEBUG(DEB_LEV_ERR,"err!mng_info->formatbuffer malloc fail(%d,%d)!%s,%d\n",
				mng_info->i_source_stride,mng_info->i_source_height,__FILE__,__LINE__);
		}
	}
#endif

#ifdef IC_TYPE_GL5207
	unsigned long down_planar_vir;
	mng_info->enc_frame->down_planar_addr = omx_malloc_phy(160*1024,&down_planar_vir);
	if(mng_info->enc_frame->down_planar_addr == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!omx_malloc_phy fail!%s  %d\n", __FILE__, __LINE__);
        if(mng_info->fbuffer_phy)
        {
            omx_free_phy(mng_info->fbuffer_phy);
            mng_info->fbuffer_phy = NULL;
        }
		pthread_mutex_unlock(&mng_info->filter_mutex);
		return -1;
	}
    
#ifdef IC_TYPE_GL5206
    //for gl5206 & gl5209, temp buffer address is 32-aligned
    if((unsigned long)mng_info->enc_frame->down_planar_addr % 0x20)
    {
        DEBUG(DEB_LEV_ERR, "err!temp buffer address is not 32-aligned!%s  %d\n", __FILE__, __LINE__);
        pthread_mutex_unlock(&mng_info->filter_mutex);
        return -1;
    }
#endif

	mng_info->enc_frame->b_ds = 1;
	penc_param->prp_param->b_downscale = 1;
	penc_param->h264_param->b_cabac = 0;
	//printf("mng_info->enc_frame->down_planar_addr:%x\n",mng_info->enc_frame->down_planar_addr);
#endif

	queue_init(mng_info->queue_enc);
	queue_init(mng_info->queue_strm);
	queue_init(mng_info->queue_prepare_video);
#ifdef FIXED_CTS_4_4
	queue_init(mng_info->queue_strm_return);
#endif	

	ret = vce_enc_init(mng_info->vce_handle, enc_param);
	if (ret == -1)
	{
		DEBUG(DEB_LEV_ERR, "err!vce_enc_init fail!%s  %d\n", __FILE__, __LINE__);
		queue_deinit(mng_info->queue_enc);
		queue_deinit(mng_info->queue_strm);
		queue_deinit(mng_info->queue_prepare_video);
#ifdef FIXED_CTS_4_4
		queue_deinit(mng_info->queue_strm_return);
#endif

#ifdef IC_TYPE_GL5207
		omx_free_phy(mng_info->enc_frame->down_planar_addr);
		mng_info->enc_frame->down_planar_addr = NULL;
#endif
		pthread_mutex_unlock(&mng_info->filter_mutex);
		return -1;
	}

#if Enable_Fix_SCALE
	mng_info->resize_buf.scale_phy = 0;
	mng_info->resize_buf.scale_vir = 0;
	mng_info->resize_buf.scale_flag = 0;
	mng_info->resize_buf.nead_recover = 0;
	mng_info->resize_buf.last_len = 0;
#endif

	pthread_mutex_unlock(&mng_info->filter_mutex);
	return ret;
}

/* AUTO FRAME TYPE DECISION */
/*
 * 注意MVC需要考虑处理方式
 * */
int mng_try_encode(mng_internal_t *mng_info, OMX_PTR *pBVideo, OMX_PTR *pBStrm)
{
	int ret = 0;
	OMX_BUFFERHEADERTYPE *pBufferVideo = NULL;
	OMX_BUFFERHEADERTYPE *pBufferStream = NULL;
	OMX_U8 *pBufferVideo_VirAddr = NULL;
	OMX_U8 *pBufferVideo_PhyAddr = NULL;
	OMX_U8 *pBufferStream_VirAddr = NULL;
	OMX_U8 *pBufferStream_PhyAddr = NULL;

	int nVnum = 0;
	int nSnum = 0;
	int nPreP = 0;
	int yoffset_len = 0;
	int uvoffset_len = 0;
	int bDataIn = 0;
	video_metadata_t* metadata_handle = NULL;
	int maxIinterval;
	enc_frame_t *enc_frame = mng_info->enc_frame;
	enc_stream_t *enc_strm = mng_info->enc_strm;
	ALFace_img_t* pface_img = &(mng_info->face_img);

	DEBUG(DEB_LEV_PARAMS, "mng_info->bexit:%d  mng_info->b_encoded:%d\n", mng_info->bexit, mng_info->b_encoded);
	if (mng_info->bexit || mng_info->b_encoded)
		return -1;
	pthread_mutex_lock(&mng_info->filter_mutex);

	//DEBUG(DEB_LEV_FULL_SEQ,"Get!nVnum:%d  nSnum:%d\n",getquenelem(mng_info->queue_video),getquenelem(mng_info->queue_out));
	//DEBUG(DEB_LEV_FULL_SEQ,"mng_info->i_bframes:%d   bmvc:%d\n",mng_info->i_bframes,mng_info->bmvc);
	//DEBUG(DEB_LEV_FULL_SEQ,"mng_info->IDR_Refresh:%x  queue_prepare_video:%x\n",mng_info->IDR_Refresh,getquenelem(mng_info->queue_prepare_video));

	{
		maxIinterval = mng_info->maxIinterval;
		if (mng_info->bmvc)
			maxIinterval *= 2;
		if ((mng_info->IDR_Refresh == OMX_TRUE) && (getquenelem(mng_info->queue_prepare_video) == 0))
		{
			mng_info->bIDR = 1;
			mng_info->IDR_Refresh = OMX_FALSE;
		}

		if (mng_info->i_bframes > 0 && (mng_info->bIDR != 1))
		{
			/*有B帧,且当前不是IIDR帧,不即需要插入I or IDR帧*/
			/*获取已入列，要编B帧的帧数*/
			nPreP = getquenelem(mng_info->queue_prepare_video);
			if (nPreP == 0)
			{
				/*如果PREPARE QUEUE中没有图像，需要重新获取*/
				int i = 0;
				int nQueueNum = 0;
				nVnum = getquenelem(mng_info->queue_video);
				nSnum = getquenelem(mng_info->queue_out);

				if (mng_info->outport_enable)
				{
					if (!nVnum || !nSnum)
					{
						pthread_mutex_unlock(&mng_info->filter_mutex);
						return -1;
					}
				}
				else
				{
					if (!nVnum)
					{
						pthread_mutex_unlock(&mng_info->filter_mutex);
						return -1;
					}
				}

				/*计算nQueueNum*/
				nQueueNum = nVnum;

				if (mng_info->bmvc)
				{
					nQueueNum = (nQueueNum / 2) > (mng_info->i_bframes + 1) ? (mng_info->i_bframes + 1) : nQueueNum;
				}
				else
				{
					nQueueNum = nQueueNum > (mng_info->i_bframes + 1) ? (mng_info->i_bframes + 1) : nQueueNum;
				}

				/*等待准备好（mng_info->i_bframes+1）帧，再进行编码*/
				/*AVC可行 MVC待定*/
				if (nQueueNum < (mng_info->i_bframes + 1))
				{
					pthread_mutex_unlock(&mng_info->filter_mutex);
					return -1;
				}

				/*nQueueNum-1帧入列，将作为B帧编码*/
				if (nQueueNum > 1)
				{
					for (i = 0; i < nQueueNum - 1; i++)
					{
						tsem_down(mng_info->pInputSem);
						pBufferVideo = dequeue(mng_info->queue_video);
						queue(mng_info->queue_prepare_video, (OMX_PTR) pBufferVideo);

						mng_info->encFrm++;
					}
				}

				/*跳过nQueueNum-1帧，获取下一帧作为编码*/
				tsem_down(mng_info->pInputSem);
				pBufferVideo = dequeue(mng_info->queue_video);
				queue(mng_info->queue_enc, (OMX_PTR) pBufferVideo);

				if (mng_info->outport_enable)
				{
					tsem_down(mng_info->pOutputSem);
					pBufferStream = dequeue(mng_info->queue_out);
					queue(mng_info->queue_strm, (OMX_PTR) pBufferStream);
#if Enable_RingBuffer
					if(mng_info->ringbuf == OMX_TRUE)
					{
						ret = Get_UseRingBuffer_BuffersMng(mng_info->pOutBuffersMng_List,mng_info->bufferpool,pBufferStream,mng_info->ringbuf_size);
						if(ret != OMX_ErrorNone)
						{
							DEBUG(DEB_LEV_ERR,"err!can not get use ringbuffer!%s,%d\n",__FILE__,__LINE__);
							pthread_mutex_unlock(&mng_info->filter_mutex);
							return -1;
						}
					}
#endif
				}

				enc_frame->frame_cnt = mng_info->encFrm;
				mng_info->encFrm++;

				/*当前帧必为P帧*/
				mng_info->i_last_type = SLICE_TYPE_P;
				enc_frame->type = SLICE_TYPE_P;
			}
			else
			{
				/*已有待编B帧入列，可以编B帧了*/
				/*出列:pBufferStream,pBufferVideo,pBufferImage*/
				if (mng_info->outport_enable)
				{
					nSnum = getquenelem(mng_info->queue_out);
					if (nSnum == 0)
					{
						pthread_mutex_unlock(&mng_info->filter_mutex);
						return -1;
					}

					tsem_down(mng_info->pOutputSem);
					pBufferStream = dequeue(mng_info->queue_out);
					queue(mng_info->queue_strm, (OMX_PTR) pBufferStream);
#if Enable_RingBuffer
					if(mng_info->ringbuf == OMX_TRUE)
					{
						ret = Get_UseRingBuffer_BuffersMng(mng_info->pOutBuffersMng_List,mng_info->bufferpool,pBufferStream,mng_info->ringbuf_size);
						if(ret != OMX_ErrorNone)
						{
							DEBUG(DEB_LEV_ERR,"err!can not get use ringbuffer!%s,%d\n",__FILE__,__LINE__);
							pthread_mutex_unlock(&mng_info->filter_mutex);
							return -1;
						}
					}
#endif
				}

				pBufferVideo = dequeue(mng_info->queue_prepare_video);
				queue(mng_info->queue_enc, (OMX_PTR) pBufferVideo);

				enc_frame->frame_cnt = mng_info->encFrm - nPreP - 1;

				/*当前编码类型判决*/
				if (mng_info->bmvc)
				{
					if (mng_info->i_view_id == 0)
						enc_frame->type = mng_info->i_last_type;
					else
						enc_frame->type = SLICE_TYPE_B;

					if (mng_info->i_last_type == SLICE_TYPE_I)
					{
						if (mng_info->i_view_id == 0)
							enc_frame->type = SLICE_TYPE_P;
					}
				}
				else
				{
					enc_frame->type = SLICE_TYPE_B;
				}
				mng_info->i_last_type = enc_frame->type;

				/*该判断为了，B帧入列后，下次跑编码IDR帧的分支*/
				if (nPreP == 1 && mng_info->bIDR == 2)
				{
					mng_info->bIDR = 1;
				}
			}
		}
		else
		{
			/*没有B帧，或当前是IDR帧/P帧*/
			nVnum = getquenelem(mng_info->queue_video);
			nSnum = getquenelem(mng_info->queue_out);
			DEBUG(DEB_LEV_PARAMS, "nVnum:%d nSnum:%d\n", nVnum, nSnum);

			if (mng_info->outport_enable)
			{
				if (!nVnum || !nSnum)
				{
					pthread_mutex_unlock(&mng_info->filter_mutex);
					return -1;
				}
			}
			else
			{
				if (!nVnum)
				{
					pthread_mutex_unlock(&mng_info->filter_mutex);
					return -1;
				}
			}

			{
				tsem_down(mng_info->pInputSem);
				pBufferVideo = dequeue(mng_info->queue_video);
				queue(mng_info->queue_enc, (OMX_PTR) pBufferVideo);

				if (mng_info->outport_enable)
				{
					tsem_down(mng_info->pOutputSem);
					pBufferStream = dequeue(mng_info->queue_out);
					queue(mng_info->queue_strm, (OMX_PTR) pBufferStream);
#if Enable_RingBuffer
					if(mng_info->ringbuf == OMX_TRUE)
					{
						ret = Get_UseRingBuffer_BuffersMng(mng_info->pOutBuffersMng_List,mng_info->bufferpool,pBufferStream,mng_info->ringbuf_size);
						if(ret != OMX_ErrorNone)
						{
							DEBUG(DEB_LEV_ERR,"err!can not get use ringbuffer!%s,%d\n",__FILE__,__LINE__);
							pthread_mutex_unlock(&mng_info->filter_mutex);
							return -1;
						}
					}
#endif
				}
			}

			enc_frame->frame_cnt = mng_info->encFrm;
			mng_info->encFrm++;

			/*enc_frame->type = (mng_info->cntFrm%mng_info->maxIinterval) == 0?SLICE_TYPE_I:SLICE_TYPE_P;*/
			/*当前帧编码类型判决*/
			if (mng_info->bmvc)
			{
				if (mng_info->i_view_id == 0)
				{
					if (mng_info->bIDR)
					{
						enc_frame->type = SLICE_TYPE_I;
						mng_info->bIDR = 0;
					}
					else
					{
						enc_frame->type = SLICE_TYPE_P;
					}
				}
				else
				{
					enc_frame->type = SLICE_TYPE_P;
				}
			}
			else
			{
				enc_frame->type = SLICE_TYPE_P;
				if (mng_info->bIDR)
				{
					enc_frame->type = SLICE_TYPE_I;
					mng_info->bIDR = 0;
					mng_info->cntFrm = 0;
				}
			}
			mng_info->i_last_type = enc_frame->type;
		}

		if (mng_info->bmvc)
		{
			enc_frame->view_id = mng_info->i_view_id;
			mng_info->i_view_id = 1 - mng_info->i_view_id;
		}

		mng_info->cntFrm++;
		DEBUG(DEB_LEV_PARAMS, "mng_info->cntFrm:%lld , %d\n", mng_info->cntFrm, __LINE__);

		/*下帧是否为IDR类型判决*/
		if ((mng_info->cntFrm % maxIinterval) == 0)
		{
			nPreP = getquenelem(mng_info->queue_prepare_video);
			if (nPreP) /*若还没编完B帧*/
				mng_info->bIDR = 2;
			else
				mng_info->bIDR = 1;
		}

#ifndef IC_TYPE_GL5207
		/*not support single downscale*/
		enc_frame->b_ds = 0;
#endif
	}
	
	/*结束标志帧,直接返回*/
	if(pBufferVideo && pBufferVideo->nFilledLen == 0 && ((pBufferVideo->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) 
	{
		DEBUG(DEB_LEV_ERR, "mng_try_encode,eos!nFilledLen:%x,nFlags:%x,pBuffer:%p\n",
			(int)pBufferVideo->nFilledLen,(int)pBufferVideo->nFlags,pBufferVideo);

		*pBVideo = pBufferVideo;
		*pBStrm = pBufferStream;
		OMX_BUFFERHEADERTYPE *pBufferTmp = NULL;
		/*清queue_strm队列中的pBufferStream*/
		if (pBufferStream)
		{
			pBufferStream->nFilledLen = 0;
			do{
				pBufferTmp = dequeue(mng_info->queue_strm);
				if(pBufferTmp != pBufferStream)
				{
					queue(mng_info->queue_strm, (OMX_PTR) pBufferTmp);
				}
			}while(pBufferTmp != pBufferStream);
		}

		/*清queue_strm队列中的pBufferVideo*/
		do{
			pBufferTmp = dequeue(mng_info->queue_enc);
			if(pBufferTmp != pBufferVideo)
			{
				queue(mng_info->queue_strm, (OMX_PTR) pBufferTmp);
			}
		}while(pBufferTmp != pBufferVideo);
		pthread_mutex_unlock(&mng_info->filter_mutex);
		return -1;
	}
	
	{
		/*获取虚拟地址和物理地址*/
		pBufferVideo_VirAddr =  Get_VirAddr_BuffersMng(mng_info->pInBuffersMng_List,pBufferVideo,mng_info->b_store_in_video[0]);
		pBufferVideo_PhyAddr =  Get_PhyAddr_BuffersMng(mng_info->pInBuffersMng_List,pBufferVideo,mng_info->b_store_in_video[0]);
		DEBUG(DEB_LEV_PARAMS,"pBufferVideo_VirAddr:%p ,pBufferVideo_PhyAddr:%p\n",pBufferVideo_VirAddr,pBufferVideo_PhyAddr);
		if (pBufferVideo_VirAddr == NULL || pBufferVideo_PhyAddr == NULL)
		{
			DEBUG(DEB_LEV_ERR, "err!Get Addr fail,%p,%p!%s,%d\n",pBufferVideo_VirAddr,pBufferVideo_PhyAddr,__FILE__,__LINE__);
			pthread_mutex_unlock(&mng_info->filter_mutex);
			return -1;
		}

		if (pBufferStream)
		{
#if Enable_RingBuffer
			if(mng_info->ringbuf == OMX_TRUE)
			{
				pBufferStream_VirAddr = Get_VirAddr_BuffersMng(mng_info->pOutBuffersMng_List,pBufferStream,OMX_FALSE);
				pBufferStream_PhyAddr = Get_PhyAddr_BuffersMng(mng_info->pOutBuffersMng_List,pBufferStream,OMX_FALSE);
			}
			else
#endif
			{
				pBufferStream_VirAddr =  Get_VirAddr_BuffersMng(mng_info->pOutBuffersMng_List,pBufferStream,mng_info->b_store_in_video[1]);
				pBufferStream_PhyAddr =  Get_PhyAddr_BuffersMng(mng_info->pOutBuffersMng_List,pBufferStream,mng_info->b_store_in_video[1]);
			}
			DEBUG(DEB_LEV_PARAMS,"pBufferStream_VirAddr:%p ,pBufferStream_PhyAddr:%p\n",pBufferStream_VirAddr,pBufferStream_PhyAddr);
			if (pBufferStream_VirAddr == NULL || pBufferStream_PhyAddr == NULL)
			{
				DEBUG(DEB_LEV_ERR, "err!Get Addr fail,%p,%p!%s,%d\n",pBufferStream_VirAddr,pBufferStream_PhyAddr,__FILE__,__LINE__);
				pthread_mutex_unlock(&mng_info->filter_mutex);
				return -1;
			}
		}

#if Enable_Fix_SCALE
		if(mng_info->resize_buf.nead_recover)
		{
			resize_recover(mng_info);
		}
#endif

		/*Crop：Crop赋值和计算偏移量*/
		if (mng_info->b_store_in_video[0] == OMX_TRUE)
		{
			metadata_handle = (video_metadata_t*) (pBufferVideo->pBuffer);
			if (metadata_handle->metadataBufferType == 0)
			{
#ifdef   MOD_FOR_UNALIGN_RES
				/*crop区域设置*/
				mng_info->face_crop.x = ALIGN_SRC_OFFX(metadata_handle->off_x);
				mng_info->face_crop.y = ALIGN_SRC_OFFY(metadata_handle->off_y);
				mng_info->face_crop.cropw = ALIGN_SRC_WIDTH(metadata_handle->crop_w);
				mng_info->face_crop.croph = ALIGN_SRC_HEIGHT(metadata_handle->crop_h);
				enc_frame->height = ALIGN_SRC_HEIGHT(metadata_handle->crop_h);
				enc_frame->width = ALIGN_SRC_WIDTH(metadata_handle->crop_w);
#else
				/*crop区域设置*/
				mng_info->face_crop.x = metadata_handle->off_x;
				mng_info->face_crop.y = metadata_handle->off_y;
				mng_info->face_crop.cropw = metadata_handle->crop_w;
				mng_info->face_crop.croph = metadata_handle->crop_h;
				enc_frame->height = (metadata_handle->crop_h + 0x7) & (~0x7);
				enc_frame->width = (metadata_handle->crop_w + 0x7) & (~0x7);
#endif
			}
			else
			{
				enc_frame->height = mng_info->i_source_height;
				enc_frame->width = mng_info->i_source_width;
			}

#ifdef MOD_FOR_UNALIGN_RES
			yoffset_len = mng_info->i_source_stride * mng_info->face_crop.y + mng_info->face_crop.x;
			if(mng_info->i_video_fmt == ENC_YUV422P)
			{
				uvoffset_len = mng_info->i_source_stride * (mng_info->face_crop.y>>1) + mng_info->face_crop.x;
			}
			else
			{
				uvoffset_len = (mng_info->i_source_stride>>1) * (mng_info->face_crop.y>>1) + (mng_info->face_crop.x>>1);
			}
#else
			yoffset_len = mng_info->i_source_width * metadata_handle->off_y + metadata_handle->off_x;
			if (mng_info->i_video_fmt == ENC_YUV422P)
			{
				uvoffset_len = mng_info->i_source_width * (metadata_handle->off_y >> 1) + (metadata_handle->off_x);
			}
			else
			{
				uvoffset_len =  (mng_info->i_source_width>>1) * (metadata_handle->off_y>>1) + (metadata_handle->off_x>>1);
			}
#endif
			DEBUG(/*DEB_LEV_ERR*/DEB_LEV_PARAMS,"Crop!w:%d,h:%d,x:%d,y:%d\n",enc_frame->width,enc_frame->height,mng_info->face_crop.x,mng_info->face_crop.y);
		}
		else
		{
			if (mng_info->bchanged == 1)
			{
				/*crop区域设置：设置crop参数时已进行对齐*/
				mng_info->face_crop.x = mng_info->rect_x;
				mng_info->face_crop.y = mng_info->rect_y;
				mng_info->face_crop.cropw = mng_info->rect_w;
				mng_info->face_crop.croph = mng_info->rect_h;

#ifdef MOD_FOR_UNALIGN_RES
				enc_frame->height = mng_info->rect_h;
				enc_frame->width = mng_info->rect_w;
#else
				enc_frame->height = (mng_info->rect_h + 0x7) & (~0x7);
				enc_frame->width = (mng_info->rect_w + 0x7) & (~0x7);
#endif

#ifdef MOD_FOR_UNALIGN_RES
				yoffset_len = mng_info->i_source_stride * mng_info->face_crop.y + mng_info->face_crop.x;
				if(mng_info->i_video_fmt == ENC_YUV422P)
				{
					uvoffset_len = mng_info->i_source_stride * (mng_info->face_crop.y>>1) + (mng_info->face_crop.x);
				}
				else
				{
					uvoffset_len = (mng_info->i_source_stride>>1) * (mng_info->face_crop.y>>1) + (mng_info->face_crop.x>>1);
				}
#else
				yoffset_len = mng_info->i_source_width * mng_info->rect_y + mng_info->rect_x;
				if (mng_info->i_video_fmt == ENC_YUV422P)
				{
					uvoffset_len = mng_info->i_source_width * (mng_info->rect_y >> 1) + (mng_info->rect_x);
				}
				else
				{
					uvoffset_len = (mng_info->i_source_width >> 1) * (mng_info->rect_y >> 1) + (mng_info->rect_x >> 1);
				}
#endif
				mng_info->bchanged = 0;
			}
			DEBUG(DEB_LEV_PARAMS,"Crop!w:%d,h:%d,x:%d,y:%d\n",enc_frame->width,enc_frame->height,mng_info->rect_x,mng_info->rect_y);
		}

#ifndef IC_TYPE_GL5206
		/*8字节对齐*/
		yoffset_len = yoffset_len & (~0x7); 
		uvoffset_len = uvoffset_len & (~0x7);
#else
		/*32字节对齐*/
		yoffset_len = yoffset_len & (~0x1f); 
		uvoffset_len = uvoffset_len & (~0x1f);
#endif

		DEBUG(DEB_LEV_PARAMS, "yoffset:%d,uvoffset:%d\n", yoffset_len, uvoffset_len);
		DEBUG(DEB_LEV_PARAMS, "i_source_width:%d,i_source_height:%d,width%d,height:%d i_video_fmt:%d\n",mng_info->i_source_width,mng_info->i_source_height,
			enc_frame->width,enc_frame->height,mng_info->i_video_fmt);

		/*原Y:物理/虚拟地址*/
		enc_frame->src_planar_addr[0] = (void*) (pBufferVideo_PhyAddr);
		enc_frame->src_planar[0] = (unsigned char*) (pBufferVideo_VirAddr);

#ifdef IC_TYPE_GL5206
        if(mng_info->stride32_flag == 1)
        {
            unsigned char *src, *dst;
            int i;
            src = pBufferVideo_VirAddr;
            dst = mng_info->fbuffer_vir;
            for(i = 0; i < mng_info->i_source_height; i++)
            {
                memcpy(dst, src, mng_info->i_source_width);
                dst += mng_info->i_source_stride;
                src += mng_info->i_source_width;
            }
            if(mng_info->i_video_fmt == ENC_YUV411P)
            {
                if(!mng_info->b_semi)
                {
                    //yuv420p
                    for(i = 0; i < mng_info->i_source_height; i++)
                    {
                        memcpy(dst, src, mng_info->i_source_width/2);
                        dst += (mng_info->i_source_stride/2);
                        src += (mng_info->i_source_width/2);
                    }
                }
                else
                {
                    //yuv420sp
                    for(i = 0; i < mng_info->i_source_height/2; i++)
                    {
                        memcpy(dst, src, mng_info->i_source_width);
                        dst += mng_info->i_source_stride;
                        src += mng_info->i_source_width;
                    }
                }
            }
            else
            {
                DEBUG(DEB_LEV_ERR,"GL5206 stride 32-aligned, only YUV411! %s, %d", __FILE__, __LINE__);
                pthread_mutex_unlock(&mng_info->filter_mutex);
                return -1;
            }
            enc_frame->src_planar_addr[0] = mng_info->fbuffer_phy;
            enc_frame->src_planar[0] = mng_info->fbuffer_vir;
        }
#endif

#ifdef MOD_FOR_UNALIGN_RES  /*07.02*/
		/*原U:物理/虚拟地址*/
		enc_frame->src_planar_addr[1] = (void *)( (unsigned char *)(enc_frame->src_planar_addr[0]) +  mng_info->i_source_stride  *  mng_info->i_source_height);
		enc_frame->src_planar[1] = enc_frame->src_planar[0] + mng_info->i_source_stride * mng_info->i_source_height;

		/*原V:物理/虚拟地址*/
		if (mng_info->i_video_fmt == ENC_YUV422P)
		{
			enc_frame->src_planar_addr[2] = (void *)( (unsigned char *)(enc_frame->src_planar_addr[1]) +  mng_info->i_source_stride  *  mng_info->i_source_height/2);
			enc_frame->src_planar[2] = enc_frame->src_planar[1] +  mng_info->i_source_stride *  mng_info->i_source_height/2;
		}
		else
		{
			enc_frame->src_planar_addr[2] = (void *)( (unsigned char *)(enc_frame->src_planar_addr[1]) +  mng_info->i_source_stride  *  mng_info->i_source_height/4);
			enc_frame->src_planar[2] = enc_frame->src_planar[1] +  mng_info->i_source_stride *  mng_info->i_source_height/4;
		}
#else
		/*原U:物理/虚拟地址*/
		enc_frame->src_planar_addr[1] = (void *)( (unsigned char *)(enc_frame->src_planar_addr[0]) +  mng_info->i_source_width *  mng_info->i_source_height);
		enc_frame->src_planar[1] = enc_frame->src_planar[0] + mng_info->i_source_width * mng_info->i_source_height;

		/*原V:物理/虚拟地址*/
		if (mng_info->i_video_fmt == ENC_YUV422P)
		{
			enc_frame->src_planar_addr[2] = (void *)( (unsigned char *)(enc_frame->src_planar_addr[1]) +  mng_info->i_source_width *  mng_info->i_source_height/2);
			enc_frame->src_planar[2] = enc_frame->src_planar[1] +  mng_info->i_source_width *  mng_info->i_source_height/2;
		}
		else
		{
			enc_frame->src_planar_addr[2] = (void *)( (unsigned char *)(enc_frame->src_planar_addr[1]) +  mng_info->i_source_width *  mng_info->i_source_height/4);
			enc_frame->src_planar[2] = enc_frame->src_planar[1] +  mng_info->i_source_width *  mng_info->i_source_height/4;
		}
#endif

		/*偏移后YUV:物理地址*/
		enc_frame->src_planar_addr[0] = (unsigned char*) (enc_frame->src_planar_addr[0]) + yoffset_len;
		if (mng_info->b_semi == 0)
		{
			enc_frame->src_planar_addr[1] = (unsigned char*) (enc_frame->src_planar_addr[1]) + uvoffset_len;
			enc_frame->src_planar_addr[2] = (unsigned char*) (enc_frame->src_planar_addr[2]) + uvoffset_len;
		}
		else
		{
			enc_frame->src_planar_addr[1] = (unsigned char*) (enc_frame->src_planar_addr[1]) + uvoffset_len * 2;
			enc_frame->src_planar_addr[2] = enc_frame->src_planar_addr[1];
		}
		DEBUG(DEB_LEV_PARAMS,"src_phy_addr  0:%p,1:%p,2:%p,b_semi:%d\n",enc_frame->src_planar_addr[0] ,enc_frame->src_planar_addr[1] ,enc_frame->src_planar_addr[2],mng_info->b_semi);

		/*偏移后YUV:虚拟地址*/
		enc_frame->src_planar[0] = (unsigned char*) (enc_frame->src_planar[0]) + yoffset_len;
		if (mng_info->b_semi == 0)
		{
			enc_frame->src_planar[1] = (unsigned char*) (enc_frame->src_planar[1]) + uvoffset_len;
			enc_frame->src_planar[2] = (unsigned char*) (enc_frame->src_planar[2]) + uvoffset_len;
		}
		else
		{
			enc_frame->src_planar[1] = (unsigned char*) (enc_frame->src_planar[1]) + uvoffset_len * 2;
			enc_frame->src_planar[2] = enc_frame->src_planar[1];
		}
		DEBUG(DEB_LEV_PARAMS,"src_vir_addr  0:%p,1:%p,2:%p,b_semi:%d\n",enc_frame->src_planar[0] ,enc_frame->src_planar[1] ,enc_frame->src_planar[2],mng_info->b_semi);		

		/*人脸检测：初始化参数*/
		memset(pface_img, 0, sizeof(ALFace_img_t));
		pface_img->width = mng_info->i_source_width;
		pface_img->height = mng_info->i_source_height;
#ifdef MOD_FOR_UNALIGN_RES
		pface_img->stride = mng_info->i_source_stride;
#else
		pface_img->stride = mng_info->i_source_width;
#endif
		pface_img->img_fmt = mng_info->b_semi;
		pface_img->max_faces = 16;
		pface_img->dir = mng_info->fd_nAngle;
		pface_img->isfront = mng_info->fd_isFront;
		//DEBUG(DEB_LEV_ERR/*DEB_LEV_PARAMS*/,"pface_img!dir:%d,isfront:%d\n",pface_img->dir,pface_img->isfront);
		//DEBUG(DEB_LEV_ERR/*DEB_LEV_PARAMS*/,"face_img.width:%d,face_img.height:%d img_fmt:%d\n",pface_img->width,pface_img->height,pface_img->img_fmt);

		/*人脸检测：下采样输出地址*/
		pface_img->cur_img = (unsigned char*) (pBufferVideo_VirAddr);//should get virtal address

		/*人脸检测：准备数据*/
		if (mng_info->bface_en)
		{
			unsigned int status;
			face_cmd(mng_info->face_eng, FE_STATUS, (unsigned long) &status);
			DEBUG(DEB_LEV_PARAMS, "face!face_cmd status:%d\n", status);
			if ((status != SFE_BUSY) && (status != SFE_DATA_READY))
			{
				face_cmd(mng_info->face_eng, FE_DATAIN, (unsigned long) pface_img);
				face_cmd(mng_info->face_eng, FE_CROP, (unsigned long) &(mng_info->face_crop));
				bDataIn = 1;
			}
		}

#if Enable_Fix_SCALE
		/*图像缩放*/
		if(mng_info->outport_enable && (mng_info->is_argb8888 == 0) )
		{
			int s_diff = 0,s_dir = 0,nead8scale = 0;
			ret = resize_check(enc_frame->width,enc_frame->height,mng_info->ndst_width,
					mng_info->ndst_height,&s_diff,&s_dir,&nead8scale);
			if(ret < 0)
			{
				DEBUG(DEB_LEV_ERR,"err!cannot suport this scale! %s,%d!\n",__FILE__,__LINE__);
				pthread_mutex_unlock(&mng_info->filter_mutex);
				return -1;
			}

			//DEBUG(DEB_LEV_ERR,"s_diff:%d,nead8scale:%d,is_argb8888:%d!\n",s_diff,nead8scale,mng_info->is_argb8888);
			if( (s_diff != 0) || (nead8scale != 0))
			{
				/*init*/
				resize_input_t resize_in;
				resize_output_t resize_out;

				vce_enc_cmd(mng_info->vce_handle,ENC_GET_DRV_FD,(unsigned long)&(resize_in.vce_fd));
				resize_input_init(&resize_in,mng_info);

				/*run*/
				mng_info->resize_buf.scale_flag = 1;
				mng_info->resize_buf.nead_recover = 1;
				ret = resize_start(&resize_in,&resize_out,&(mng_info->resize_buf),s_diff,s_dir,nead8scale);
				if(ret < 0)
				{
					DEBUG(DEB_LEV_ERR,"err!resize_any_scale fail! %s,%d!\n",__FILE__,__LINE__);
					pthread_mutex_unlock(&mng_info->filter_mutex);
					return -1;
				}

				/*result*/
				resize_get_output(&resize_out,mng_info);
			}
			else
			{
				mng_info->resize_buf.nead_recover = 0;
			}
		}
#endif

#if Enable_Fix_ARGB8888
		/*wdf输入设置*/
		if(mng_info->outport_enable && mng_info->is_argb8888)
		{
#ifdef IC_TYPE_GL5203
			ret = wfd_sizes_check(enc_frame->width,enc_frame->height,mng_info->ndst_width,mng_info->ndst_height);
			if(ret < 0)
			{
				DEBUG(DEB_LEV_ERR,"err!cannot suport this scale! %s,%d!\n",__FILE__,__LINE__);
				pthread_mutex_unlock(&mng_info->filter_mutex);
				return -1;
			}
#endif

			if(mng_info->is_formatconvert)
			{
				wfd_argb888_media_format_convert(enc_frame->src_planar[0],mng_info->i_source_stride,
				mng_info->i_source_width,mng_info->i_source_height,mng_info->formatbuffer);
			}

#ifndef IC_TYPE_GL5207
#ifdef   IC_TYPE_GL5203
			/*crop*/
			enc_frame->bld_rect.w = enc_frame->width;
			enc_frame->bld_rect.h = enc_frame->height;
			int rgb_offset_len = (mng_info->i_source_stride * mng_info->face_crop.y + mng_info->face_crop.x)*4;
			rgb_offset_len = rgb_offset_len & (~0x7); 			/*8字节对齐*/
			void *blend_intput = (void *)(pBufferVideo_PhyAddr +  rgb_offset_len);
			wfd_argb8888_set(mng_info,blend_intput);
#else
			wfd_argb8888_set(mng_info,(void *)pBufferVideo_PhyAddr);
#endif
#endif

		}
#endif

		/*初始化输出流*/
		if (mng_info->frame_rate)
			enc_frame->frmtime = 1000000 / (mng_info->frame_rate);
		enc_frame->pts = pBufferVideo->nTimeStamp;
		if (pBufferStream)
		{
			pBufferStream->nTimeStamp = pBufferVideo->nTimeStamp;
			if (mng_info->b_store_in_video[1] == OMX_TRUE)
			{
#ifdef enable_gralloc
				/*
				int gwidth,gheight,gformat,gsize;
				VCE_OSAL_GetBufInfo( (OMX_PTR)((video_metadata_t*)(pBufferStream->pBuffer))->handle,
						&gwidth,&gheight,&gformat,&gsize);
				enc_strm->i_len = gsize;
				*/
				DEBUG(DEB_LEV_ERR,"err!the output port cannot suport StoreMediaData!%s,%d!\n",__FILE__,__LINE__);
				pthread_mutex_unlock(&mng_info->filter_mutex);
				return -1;
#else
				enc_strm->i_len = ((video_handle_t*) ((video_metadata_t*) (pBufferStream->pBuffer))->handle)->size;
#endif
			}
			else
				enc_strm->i_len = pBufferStream->nAllocLen;
			DEBUG(DEB_LEV_PARAMS, "enc_strm->i_len:%d\n", enc_strm->i_len);
		}
		enc_strm->i_flag = 0;
		enc_strm->i_len_next = 0;
		enc_strm->i_offset = 0;
		enc_strm->i_offset_next = 0;

		/*物理和虚拟地址*/
		if (pBufferStream)
		{
			enc_strm->phy_stream_buf = (unsigned long) (pBufferStream_PhyAddr);
			enc_strm->stream_buf = (unsigned char*) (pBufferStream_VirAddr);
		}
	}

	/*返回*/
	*pBVideo = pBufferVideo;
	*pBStrm = pBufferStream;
	if (pBufferStream)
		pBufferStream->nFilledLen = 0;

	/*启动编码*/
	//DEBUG(DEB_LEV_PARAMS,"mng_info->enc_frame!width:%d  height:%d type:%d frame_cnt:%lld  view_id:%d\n",mng_info->enc_frame->width,
	//mng_info->enc_frame->height,enc_frame->type,enc_frame->frame_cnt,enc_frame->view_id);
	//if (mng_info->bIDR != 0)
	//DEBUG(DEB_LEV_ERR,"mng_info->enc_frame!width:%d  height:%d type:%d\n",mng_info->enc_frame->width,mng_info->enc_frame->height,enc_frame->type);

	if (mng_info->outport_enable)
		ret = vce_enc_frame(mng_info->vce_handle, mng_info->enc_frame, mng_info->enc_strm);
	else
		ret = 0;
	if (ret < 0)DEBUG(DEB_LEV_ERR,"err!vce_enc_frame fail!%s,%d\n", __FILE__, __LINE__);

	/*启动人脸检测*/
	if (mng_info->bface_en && (bDataIn == 1))
	{
		DEBUG(DEB_LEV_PARAMS, "face!face_eng1:%p!\n", mng_info->face_eng);
		face_cmd(mng_info->face_eng, FE_RUN, 0);
		bDataIn = 0;
	}

	if (mng_info->outport_enable)
		mng_info->b_encoded = (ret == 0) ? 1 : 0;
	else
		mng_info->b_encoded = 0;

	pthread_mutex_unlock(&mng_info->filter_mutex);
	return ret;
}

int mng_get(mng_internal_t *mng_info, OMX_PTR *pBufferVideo, OMX_PTR *pBufferStrm)
{
	int ret = 0;
	int nVnum = 0;
	int nSnum = 0;
	OMX_BUFFERHEADERTYPE *mpBufferVideo = NULL;
	OMX_BUFFERHEADERTYPE *mpBufferStream = NULL;
	*pBufferStrm = NULL;
	
	if (mng_info->bexit == 0x100)
		return 0;
	nVnum = getquenelem(mng_info->queue_enc);
	nSnum = getquenelem(mng_info->queue_strm);

	pthread_mutex_lock(&mng_info->filter_mutex);

	if (mng_info->b_encoded)
	{
		//printf("b4 vce_enc_out!\n");
		ret = vce_enc_out(mng_info->vce_handle, mng_info->enc_strm);
		//printf("aft vce_enc_out!nVnum:%d nSnum:%d\n",nVnum,nSnum);

		if (ret == 0) /*获取成功*/
		{
			if (nVnum)
			{
				mpBufferVideo = dequeue(mng_info->queue_enc);
				if (mpBufferVideo)
					*pBufferVideo = mpBufferVideo;
			}

			if (nSnum)
			{
				mpBufferStream = dequeue(mng_info->queue_strm);
				if (mpBufferStream)
					*pBufferStrm = mpBufferStream;
			}

			if (mng_info->b_store_in_video[1] == OMX_TRUE)
			{
#ifdef enable_gralloc
				DEBUG(DEB_LEV_ERR,"err!the output port cannot suport StoreMediaData!%s,%d!\n",__FILE__,__LINE__);
				pthread_mutex_unlock(&mng_info->filter_mutex);
				return -1;
#else
				((video_metadata_t*)(((OMX_BUFFERHEADERTYPE*)(*pBufferStrm))->pBuffer))->vce_attribute.noffset = mng_info->enc_strm->i_offset;
				((video_metadata_t*)(((OMX_BUFFERHEADERTYPE*)(*pBufferStrm))->pBuffer))->vce_attribute.nfilledlen = mng_info->enc_strm->i_len;
#endif
			}
			else
			{
				((OMX_BUFFERHEADERTYPE*) (*pBufferStrm))->nOffset = mng_info->enc_strm->i_offset;
				((OMX_BUFFERHEADERTYPE*) (*pBufferStrm))->nFilledLen = mng_info->enc_strm->i_len;
			}

#if Enable_RingBuffer
			if(mng_info->ringbuf == OMX_TRUE)
			{
				move_wptr(mng_info->bufferpool,((OMX_BUFFERHEADERTYPE*)(*pBufferStrm))->nOffset+((OMX_BUFFERHEADERTYPE*)(*pBufferStrm))->nFilledLen);
			}
#endif

			DEBUG(DEB_LEV_PARAMS,"i_offset:%d,i_len:%d,i_flag:%x\n",mng_info->enc_strm->i_offset,mng_info->enc_strm->i_len,mng_info->enc_strm->i_flag );
			((OMX_BUFFERHEADERTYPE*) (*pBufferStrm))->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
			if (mng_info->enc_strm->i_flag & 0x100) /*I帧标志*/
			{
				((OMX_BUFFERHEADERTYPE*) (*pBufferStrm))->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
			}
		}
		else /*编码获取失败*/
		{
			DEBUG(DEB_LEV_ERR, "err!vce_enc_out fail!the queue has no BufferStrm!%s,%d\n", __FILE__, __LINE__);
			if (nVnum)
			{
				mpBufferVideo = dequeue(mng_info->queue_enc);
				if (mpBufferVideo)
					*pBufferVideo = mpBufferVideo;
			}

			if (nSnum)
			{
				/*nFilledLen强制为0*/
				mpBufferStream = dequeue(mng_info->queue_strm);
				if (mpBufferStream)
				{
					*pBufferStrm = mpBufferStream;
					((OMX_BUFFERHEADERTYPE*) (*pBufferStrm))->nOffset = 0;
					((OMX_BUFFERHEADERTYPE*) (*pBufferStrm))->nFilledLen = 0;
				}
			}
		}

		mng_info->b_encoded = 0;
	}
	else /*没有编码 或 编码失败(此情况不会进入mng_get内)*/
	{
		if (nVnum)
		{
			mpBufferVideo = dequeue(mng_info->queue_enc);
			if (mpBufferVideo)
				*pBufferVideo = mpBufferVideo;
		}

		if (nSnum)
		{
			if (mng_info->outport_enable )
			{
				/*结束标志帧*/
				if(mpBufferVideo && mpBufferVideo->nFilledLen == 0 &&((mpBufferVideo->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS))
				{
					DEBUG(DEB_LEV_ERR, "mng_get,eos!nFilledLen:%x,nFlags:%x,pBuffer:%p\n",
						(int)mpBufferVideo->nFilledLen,(int)mpBufferVideo->nFlags,mpBufferVideo);
				}
				else 
				{
					mpBufferStream = dequeue(mng_info->queue_strm);
					if (mpBufferStream)
						*pBufferStrm = mpBufferStream;
				}
			}
		}
	}

	DEBUG(DEB_LEV_PARAMS,"mng_get!nVnum :%d, nSnum :%d,b_encoded:%d, bexit:%d\n",nVnum , nSnum , mng_info->b_encoded , mng_info->bexit);

	if (nVnum == 0 && nSnum == 0 && mng_info->b_encoded == 0 && mng_info->bexit == 1)
	{
		int nPreP = getquenelem(mng_info->queue_prepare_video);
		DEBUG(DEB_LEV_PARAMS, "mng_get!nPreP :%d\n", nPreP);
		if (nPreP)
		{
			mpBufferVideo = dequeue(mng_info->queue_prepare_video);
			if (mpBufferVideo)
				*pBufferVideo = mpBufferVideo;
		}
		else
		{
			mng_info->bexit = 0x100; //all is finished
		}
	}

	pthread_mutex_unlock(&mng_info->filter_mutex);
	return ret;
}

int mng_cmd(mng_internal_t *mng_info, unsigned cmd, void *cmdata)
{
	int ret = 0;
	if (mng_info == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!mng_info is NULL!%s,%d\n",__FILE__,__LINE__ );
		return -1;
	}

	pthread_mutex_lock(&mng_info->filter_mutex);

	switch (cmd)
	{
		case GET_PREPARE_FRAMES:
		{
			unsigned int nPreP = getquenelem(mng_info->queue_prepare_video);
			*(unsigned int*) cmdata = nPreP;
		}
		break;

		/*获取人脸检测结果*/
		case GET_FD_INFO:
		{
			if (mng_info->bface_en && mng_info->face_eng)
			{
				int i = 0;
				ALFace_appout_t face_appout;
				memset(&face_appout, 0, sizeof(ALFace_appout_t));
				int rst = face_cmd(mng_info->face_eng, FE_DATAOUT, (unsigned long) &face_appout);
				if (rst == 0)
				{
					mng_info->mFaceAppout.number_of_faces = face_appout.faceout.RectNum;
					for (i = 0; i < face_appout.faceout.RectNum; i++)
					{
						mng_info->mFaceAppout.faces[i].rect[0] = face_appout.faceout.rt[i].x;
						mng_info->mFaceAppout.faces[i].rect[1] = face_appout.faceout.rt[i].y;
						mng_info->mFaceAppout.faces[i].rect[2] = face_appout.faceout.rt[i].x + face_appout.faceout.rt[i].w;
						mng_info->mFaceAppout.faces[i].rect[3] = face_appout.faceout.rt[i].y + face_appout.faceout.rt[i].h;

						mng_info->mFaceAppout.faces[i].id = 0/*i*/;
						mng_info->mFaceAppout.faces[i].score = 95;
						mng_info->mFaceAppout.faces[i].left_eye[0] = -2000;
						mng_info->mFaceAppout.faces[i].left_eye[1] = -2000;
						mng_info->mFaceAppout.faces[i].right_eye[0] = -2000;
						mng_info->mFaceAppout.faces[i].right_eye[1] = -2000;
						mng_info->mFaceAppout.faces[i].mouth[0] = -2000;
						mng_info->mFaceAppout.faces[i].mouth[1] = -2000;
						//printf("x:%d  y:%d  w:%d  h:%d\n",face_appout.faceout.rt[i].x,face_appout.faceout.rt[i].y,
						// face_appout.faceout.rt[i].w,face_appout.faceout.rt[i].h);
					}
					*(omx_camera_frame_metadata_t**) cmdata = &(mng_info->mFaceAppout);
				}
				else
				{
					pthread_mutex_unlock(&mng_info->filter_mutex);
					return -1;
				}
			}
			else
			{
				DEBUG(DEB_LEV_ERR,"Warning!face det is not open,can not get face info!%x,%p\n",mng_info->bface_en,mng_info->face_eng);
				pthread_mutex_unlock(&mng_info->filter_mutex);
				return -1;
			}
		}
		break;

		/*人脸检测设定：open or close*/
		case SET_FD:
		{
			int bEnable = *(unsigned int*) cmdata;
			if (bEnable == 1)
			{
				if (face_handle == NULL)
				{
					ret = face_so_open();
					if (ret == -1)
					{
						DEBUG(DEB_LEV_ERR, "err!face_so_open fail!%s,%d\n",__FILE__,__LINE__ );
						goto err0;
					}

					DEBUG(DEB_LEV_PARAMS, "face!face_handle:%p!\n", face_handle);
				}

				if (mng_info->face_eng == NULL)
				{
					mng_info->face_eng = face_open();
					if (mng_info->face_eng == NULL)
					{
						DEBUG(DEB_LEV_ERR, "err!face_open fail!%s,%d\n",__FILE__,__LINE__ );
						goto err1;
					}
					DEBUG(DEB_LEV_PARAMS, "face!face_eng:%p!\n", mng_info->face_eng);
				}

				if (mng_info->mFaceAppout.faces == NULL)
				{
					mng_info->mFaceAppout.faces = (omx_camera_face_t*) malloc(sizeof(omx_camera_face_t) * 64);
					if (mng_info->mFaceAppout.faces == NULL)
					{
						DEBUG(DEB_LEV_ERR, "err!malloc fail!%s,%d\n",__FILE__,__LINE__ );
						goto err2;
					}
					DEBUG(DEB_LEV_PARAMS, "face!mFaceAppout.faces:%p!\n", mng_info->mFaceAppout.faces);
				}

				ret = face_cmd(mng_info->face_eng, FE_OPEN, 0);
				if (ret == -1)
				{
					DEBUG(DEB_LEV_ERR, "err!FE_OPEN fail!%s,%d\n",__FILE__,__LINE__ );
					goto err3;
				}
				mng_info->bface_en = 1;
			}
			else
			{
				if (mng_info->face_eng)
					face_cmd(mng_info->face_eng, FE_CLOSE, 0);
				mng_info->bface_en = 0;
			}
		}
		break;

		default:
		break;
	}

	pthread_mutex_unlock(&mng_info->filter_mutex);
	return 0;

err3:
	free(mng_info->mFaceAppout.faces);
	mng_info->mFaceAppout.faces = NULL;
err2:
	face_dispose(mng_info->face_eng);
	mng_info->face_eng = NULL;
err1:
	face_so_close();
err0:
	pthread_mutex_unlock(&mng_info->filter_mutex);
	return ret;
}

int mng_deinit(mng_internal_t *mng_info)
{
	pthread_mutex_lock(&mng_info->filter_mutex);

	if (mng_info)
	{
		if (mng_info->queue_enc)
		{
			queue_deinit(mng_info->queue_enc);
		}

		if (mng_info->queue_strm)
		{
			queue_deinit(mng_info->queue_strm);
		}

		if (mng_info->queue_prepare_video)
		{
			queue_deinit(mng_info->queue_prepare_video);
		}
		
#ifdef FIXED_CTS_4_4
		if (mng_info->queue_strm_return)
		{
			queue_deinit(mng_info->queue_strm_return);
		}
#endif

		if (mng_info->vce_handle)
		{
			vce_enc_deinit(mng_info->vce_handle);
		}

#if  Enable_Fix_ARGB8888
		if(mng_info->is_argb8888)
		{
#ifndef IC_TYPE_GL5207
			wfd_argb8888_deinit(mng_info);
#endif
		}

		if(mng_info->formatbuffer != NULL)
		{
			free(mng_info->formatbuffer);
			mng_info->formatbuffer = NULL;
		}
#endif

#if Enable_Fix_SCALE
		if(mng_info->resize_buf.scale_flag == 1)
		{
			resize_free_buf(&(mng_info->resize_buf));
			mng_info->resize_buf.scale_flag = 0;
			mng_info->resize_buf.nead_recover = 0;
		}
#endif

#ifdef IC_TYPE_GL5207
		if(mng_info->enc_frame->down_planar_addr)
		{
			omx_free_phy(mng_info->enc_frame->down_planar_addr);
			mng_info->enc_frame->down_planar_addr = NULL;
		}
#endif

#ifdef IC_TYPE_GL5206
        if(mng_info->fbuffer_phy)
        {
            omx_free_phy(mng_info->fbuffer_phy);
            mng_info->fbuffer_phy = NULL;
        }
#endif
	}

	pthread_mutex_unlock(&mng_info->filter_mutex);
	return 0;
}

int mng_free(mng_internal_t *mng_info)
{
	if (mng_info)
	{
		pthread_mutex_lock(&mng_info->filter_mutex);
		mng_open_count--;

		if (mng_info->queue_enc)
		{
			//queue_deinit(mng_info->queue_enc);
			free(mng_info->queue_enc);
			mng_info->queue_enc = NULL;
		}

		if (mng_info->queue_strm)
		{
			//queue_deinit(mng_info->queue_strm);
			free(mng_info->queue_strm);
			mng_info->queue_strm = NULL;
		}

		if (mng_info->queue_prepare_video)
		{
			//queue_deinit(mng_info->queue_prepare_video);
			free(mng_info->queue_prepare_video);
			mng_info->queue_prepare_video = NULL;
		}

#ifdef FIXED_CTS_4_4
		if (mng_info->queue_strm_return)
		{
			free(mng_info->queue_strm_return);
			mng_info->queue_strm_return = NULL;
		}
#endif

		if (mng_info->enc_frame)
		{
			free(mng_info->enc_frame);
			mng_info->enc_frame = NULL;
		}

		if (mng_info->enc_strm)
		{
			free(mng_info->enc_strm);
			mng_info->enc_strm = NULL;
		}

		if (mng_info->face_eng)
		{
			if (mng_info->mFaceAppout.faces)
			{
				free(mng_info->mFaceAppout.faces);
				mng_info->mFaceAppout.faces = NULL;
			}

			face_dispose(mng_info->face_eng);
			mng_info->face_eng = NULL;
		}

		if (face_handle != NULL && mng_open_count == 0)
			face_so_close();

		DEBUG(DEB_LEV_FUNCTION_NAME, "int to vce_enc_close!\n");
		if (mng_info->vce_handle)
		{
			vce_enc_close(mng_info->vce_handle);
			mng_info->vce_handle = NULL;
		}
		DEBUG(DEB_LEV_FUNCTION_NAME, "out of vce_enc_close!\n");
		if (mng_open_count == 0)
			vce_so_close();

		pthread_mutex_unlock(&mng_info->filter_mutex);
		pthread_mutex_destroy(&mng_info->filter_mutex);
	}

	return 0;
}

int mng_get_avc_info(mng_internal_t *mng_info, OMX_PTR *pBStrm)
{
	pthread_mutex_lock(&mng_info->filter_mutex);
	int ret = 0;
	int nSnum = 0;
	enc_frame_t *enc_frame = mng_info->enc_frame;
	enc_stream_t *enc_strm = mng_info->enc_strm;
	OMX_BUFFERHEADERTYPE *pBufferStream = NULL;
	OMX_U8 *pBufferStream_VirAddr = NULL;
	OMX_U8 *pBufferStream_PhyAddr = NULL;

	nSnum = getquenelem(mng_info->queue_out);
	if (!nSnum)
	{
		pthread_mutex_unlock(&mng_info->filter_mutex);
		return -1;
	}

	tsem_down(mng_info->pOutputSem);
	pBufferStream = dequeue(mng_info->queue_out);
	if (!pBufferStream)
	{
		pthread_mutex_unlock(&mng_info->filter_mutex);
		return -1;
	}

#if Enable_RingBuffer
	if(mng_info->ringbuf == OMX_TRUE)
	{
		ret = Get_UseRingBuffer_BuffersMng(mng_info->pOutBuffersMng_List,mng_info->bufferpool,pBufferStream,mng_info->ringbuf_size);
		if(ret != OMX_ErrorNone)
		{
			DEBUG(DEB_LEV_ERR,"err!can not get use ringbuffer!%s,%d\n",__FILE__,__LINE__);
			pthread_mutex_unlock(&mng_info->filter_mutex);
			return -1;
		}
	}
#endif

	/*初始化输入流*/
	enc_frame->frame_cnt = 0;
	enc_frame->type = SLICE_TYPE_I;
	enc_frame->view_id = 0;
#ifndef IC_TYPE_GL5207
	enc_frame->b_ds = 0;
#endif
	enc_frame->b_bld = 0;
	if (mng_info->frame_rate)
		enc_frame->frmtime = 1000000 / (mng_info->frame_rate);
	enc_frame->pts = 0;

	/*获取物理和虚拟地址*/
#if Enable_RingBuffer
	if(mng_info->ringbuf == OMX_TRUE)
	{
		pBufferStream_VirAddr = Get_VirAddr_BuffersMng(mng_info->pOutBuffersMng_List,pBufferStream,OMX_FALSE);
		pBufferStream_PhyAddr = Get_PhyAddr_BuffersMng(mng_info->pOutBuffersMng_List,pBufferStream,OMX_FALSE);
	}
	else
#endif
	{
		pBufferStream_VirAddr =  Get_VirAddr_BuffersMng(mng_info->pOutBuffersMng_List,pBufferStream,mng_info->b_store_in_video[1]);
		pBufferStream_PhyAddr =  Get_PhyAddr_BuffersMng(mng_info->pOutBuffersMng_List,pBufferStream,mng_info->b_store_in_video[1]);
	}
	DEBUG(DEB_LEV_PARAMS,"mng_get_avc_info!pBufferStream_VirAddr:%p ,pBufferStream_PhyAddr:%p\n",pBufferStream_VirAddr,pBufferStream_PhyAddr);
	if (pBufferStream_VirAddr == NULL || pBufferStream_PhyAddr == NULL)
	{
		DEBUG(DEB_LEV_ERR, "err!Get Addr fail,%p,%p!%s,%d\n",pBufferStream_VirAddr,pBufferStream_PhyAddr,__FILE__,__LINE__);
		pthread_mutex_unlock(&mng_info->filter_mutex);
		return -1;
	}

	enc_strm->phy_stream_buf = (unsigned long) (pBufferStream_PhyAddr);
	enc_strm->stream_buf = (unsigned char*) (pBufferStream_VirAddr);

	/*初始化输出流*/
	if (mng_info->b_store_in_video[1] == OMX_TRUE)
	{
#ifdef enable_gralloc
		/*
		int gwidth,gheight,gformat,gsize;
		VCE_OSAL_GetBufInfo( (OMX_PTR)((video_metadata_t*)(pBufferStream->pBuffer))->handle,
				&gwidth,&gheight,&gformat,&gsize);
		enc_strm->i_len = gsize;
		*/
		DEBUG(DEB_LEV_ERR,"err!the output port cannot suport StoreMediaData!%s,%d!\n",__FILE__,__LINE__);
		pthread_mutex_unlock(&mng_info->filter_mutex);
		return -1;
#else
		enc_strm->i_len = ((video_handle_t*) ((video_metadata_t*) (pBufferStream->pBuffer))->handle)->size;
#endif
	}
	else
		enc_strm->i_len = pBufferStream->nAllocLen;
	DEBUG(DEB_LEV_PARAMS, "enc_strm->i_len:%d\n", enc_strm->i_len);
	enc_strm->i_flag = 0;
	enc_strm->i_len_next = 0;
	enc_strm->i_offset = 0;
	enc_strm->i_offset_next = 0;

	mng_info->enc_frame->avc_info_mode = MOD_ONLY_PPS_SPS;
	ret = vce_enc_frame(mng_info->vce_handle, mng_info->enc_frame, mng_info->enc_strm);

	/*返回*/
	*pBStrm = pBufferStream;

	if (mng_info->b_store_in_video[1] == OMX_TRUE)
	{
#ifdef enable_gralloc
		DEBUG(DEB_LEV_ERR,"err!the output port cannot suport StoreMediaData!%s,%d!\n",__FILE__,__LINE__);
		pthread_mutex_unlock(&mng_info->filter_mutex);
		return -1;
#else
		((video_metadata_t*) (pBufferStream->pBuffer))->vce_attribute.noffset = mng_info->enc_strm->i_offset;
		((video_metadata_t*) (pBufferStream->pBuffer))->vce_attribute.nfilledlen = mng_info->enc_strm->i_len;
#endif
	}
	else
	{
		pBufferStream->nFilledLen = mng_info->enc_strm->i_len;
		pBufferStream->nOffset = mng_info->enc_strm->i_offset;
	}

#if Enable_RingBuffer
	if(mng_info->ringbuf == OMX_TRUE)
	{
		move_wptr(mng_info->bufferpool,pBufferStream->nOffset + pBufferStream->nFilledLen);
	}
#endif

	((OMX_BUFFERHEADERTYPE*) pBufferStream)->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;

	DEBUG(DEB_LEV_PARAMS,"mng_get_avc_info!i_len:%x,i_offset:%x\n",mng_info->enc_strm->i_len,mng_info->enc_strm->i_offset);

	pthread_mutex_unlock(&mng_info->filter_mutex);
	return ret;
}